/*
 * Copyright © 2009 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include <string.h>
#include <glib/glib.h>
#include <gio/gio.h>

/* xfilter_input_stream_t and xfilter_output_stream_t are abstract, so define
 * minimal subclasses for testing. (This used to use
 * xbuffered_input_stream_t and xbuffered_output_stream_t, but those have
 * their own test program, and they override some methods, meaning the
 * core filter stream functionality wasn't getting fully tested.)
 */

xtype_t test_filter_input_stream_get_type (void);
xtype_t test_filter_output_stream_get_type (void);

#define TEST_TYPE_FILTER_INPUT_STREAM  (test_filter_input_stream_get_type ())
#define TEST_FILTER_INPUT_STREAM(o)    (XTYPE_CHECK_INSTANCE_CAST ((o), TEST_TYPE_FILTER_INPUT_STREAM, TestFilterInputStream))
#define TEST_TYPE_FILTER_OUTPUT_STREAM (test_filter_output_stream_get_type ())
#define TEST_FILTER_OUTPUT_STREAM(o)   (XTYPE_CHECK_INSTANCE_CAST ((o), TEST_TYPE_FILTER_OUTPUT_STREAM, TestFilterOutputStream))

typedef xfilter_input_stream_t       TestFilterInputStream;
typedef GFilterInputStreamClass  TestFilterInputStreamClass;
typedef xfilter_output_stream_t      TestFilterOutputStream;
typedef xfilter_output_stream_class_t TestFilterOutputStreamClass;

XDEFINE_TYPE (TestFilterInputStream, test_filter_input_stream, XTYPE_FILTER_INPUT_STREAM)
XDEFINE_TYPE (TestFilterOutputStream, test_filter_output_stream, XTYPE_FILTER_OUTPUT_STREAM)

static void
test_filter_input_stream_init (TestFilterInputStream *stream)
{
}

static void
test_filter_input_stream_class_init (TestFilterInputStreamClass *klass)
{
}

static void
test_filter_output_stream_init (TestFilterOutputStream *stream)
{
}

static void
test_filter_output_stream_class_init (TestFilterOutputStreamClass *klass)
{
}

/* Now the tests */

static void
test_input_filter (void)
{
  xinput_stream_t *base, *f1, *f2, *s;
  xboolean_t close_base;
  xchar_t buf[1024];
  xerror_t *error = NULL;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=568394");
  base = g_memory_input_stream_new_from_data ("abcdefghijk", -1, NULL);
  f1 = xobject_new (TEST_TYPE_FILTER_INPUT_STREAM,
                     "base-stream", base,
                     "close-base-stream", FALSE,
                     NULL);
  f2 = xobject_new (TEST_TYPE_FILTER_INPUT_STREAM,
                     "base-stream", base,
                     NULL);

  xassert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f1)) == base);
  xassert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f2)) == base);

  xassert (!xinput_stream_is_closed (base));
  xassert (!xinput_stream_is_closed (f1));
  xassert (!xinput_stream_is_closed (f2));

  xobject_get (f1,
                "close-base-stream", &close_base,
                "base-stream", &s,
                NULL);
  xassert (!close_base);
  xassert (s == base);
  xobject_unref (s);

  xobject_unref (f1);

  xassert (!xinput_stream_is_closed (base));
  xassert (!xinput_stream_is_closed (f2));

  xinput_stream_skip (f2, 3, NULL, &error);
  g_assert_no_error (error);

  memset (buf, 0, 1024);
  xinput_stream_read_all (f2, buf, 1024, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (buf, ==, "defghijk");

  xobject_unref (f2);

  xassert (xinput_stream_is_closed (base));

  xobject_unref (base);
}

static void
test_output_filter (void)
{
  xoutput_stream_t *base, *f1, *f2;

  base = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  f1 = xobject_new (TEST_TYPE_FILTER_OUTPUT_STREAM,
                     "base-stream", base,
                     "close-base-stream", FALSE,
                     NULL);
  f2 = xobject_new (TEST_TYPE_FILTER_OUTPUT_STREAM,
                     "base-stream", base,
                     NULL);

  xassert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f1)) == base);
  xassert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f2)) == base);

  xassert (!xoutput_stream_is_closed (base));
  xassert (!xoutput_stream_is_closed (f1));
  xassert (!xoutput_stream_is_closed (f2));

  xobject_unref (f1);

  xassert (!xoutput_stream_is_closed (base));
  xassert (!xoutput_stream_is_closed (f2));

  xobject_unref (f2);

  xassert (xoutput_stream_is_closed (base));

  xobject_unref (base);
}

xpointer_t expected_obj;
xpointer_t expected_data;
xboolean_t callback_happened;
xmain_loop_t *loop;

static void
return_result_cb (xobject_t      *object,
                  xasync_result_t *result,
                  xpointer_t      user_data)
{
  xasync_result_t **ret = user_data;

  *ret = xobject_ref (result);
  xmain_loop_quit (loop);
}

static void
in_cb (xobject_t      *object,
       xasync_result_t *result,
       xpointer_t      user_data)
{
  xerror_t *error = NULL;

  xassert (object == expected_obj);
  xassert (user_data == expected_data);
  xassert (callback_happened == FALSE);

  xinput_stream_close_finish (expected_obj, result, &error);
  xassert (error == NULL);

  callback_happened = TRUE;
  xmain_loop_quit (loop);
}

static void
test_input_async (void)
{
  xinput_stream_t *base, *f1, *f2;
  char buf[20];
  xasync_result_t *result = NULL;
  xerror_t *error = NULL;

  loop = xmain_loop_new (NULL, FALSE);

  base = g_memory_input_stream_new_from_data ("abcdefghijklmnopqrstuvwxyz", -1, NULL);
  f1 = xobject_new (TEST_TYPE_FILTER_INPUT_STREAM,
                     "base-stream", base,
                     "close-base-stream", FALSE,
                     NULL);
  f2 = xobject_new (TEST_TYPE_FILTER_INPUT_STREAM,
                     "base-stream", base,
                     NULL);

  xassert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f1)) == base);
  xassert (g_filter_input_stream_get_base_stream (G_FILTER_INPUT_STREAM (f2)) == base);


  memset (buf, 0, sizeof (buf));
  xinput_stream_read_async (f1, buf, 10, G_PRIORITY_DEFAULT,
                             NULL, return_result_cb, &result);
  xmain_loop_run (loop);
  g_assert_cmpint (xinput_stream_read_finish (f1, result, &error), ==, 10);
  g_assert_cmpstr (buf, ==, "abcdefghij");
  g_assert_no_error (error);
  g_clear_object (&result);

  g_assert_cmpint (xseekable_tell (G_SEEKABLE (base)), ==, 10);

  xinput_stream_skip_async (f2, 10, G_PRIORITY_DEFAULT,
                             NULL, return_result_cb, &result);
  xmain_loop_run (loop);
  g_assert_cmpint (xinput_stream_skip_finish (f2, result, &error), ==, 10);
  g_assert_no_error (error);
  g_clear_object (&result);

  g_assert_cmpint (xseekable_tell (G_SEEKABLE (base)), ==, 20);

  memset (buf, 0, sizeof (buf));
  xinput_stream_read_async (f1, buf, 10, G_PRIORITY_DEFAULT,
                             NULL, return_result_cb, &result);
  xmain_loop_run (loop);
  g_assert_cmpint (xinput_stream_read_finish (f1, result, &error), ==, 6);
  g_assert_cmpstr (buf, ==, "uvwxyz");
  g_assert_no_error (error);
  g_clear_object (&result);

  g_assert_cmpint (xseekable_tell (G_SEEKABLE (base)), ==, 26);


  xassert (!xinput_stream_is_closed (base));
  xassert (!xinput_stream_is_closed (f1));
  xassert (!xinput_stream_is_closed (f2));

  expected_obj = f1;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  xinput_stream_close_async (f1, 0, NULL, in_cb, expected_data);

  xassert (callback_happened == FALSE);
  xmain_loop_run (loop);
  xassert (callback_happened == TRUE);

  xassert (!xinput_stream_is_closed (base));
  xassert (!xinput_stream_is_closed (f2));
  g_free (expected_data);
  xobject_unref (f1);
  xassert (!xinput_stream_is_closed (base));
  xassert (!xinput_stream_is_closed (f2));

  expected_obj = f2;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  xinput_stream_close_async (f2, 0, NULL, in_cb, expected_data);

  xassert (callback_happened == FALSE);
  xmain_loop_run (loop);
  xassert (callback_happened == TRUE);

  xassert (xinput_stream_is_closed (base));
  xassert (xinput_stream_is_closed (f2));
  g_free (expected_data);
  xobject_unref (f2);

  xassert (xinput_stream_is_closed (base));
  xobject_unref (base);
  xmain_loop_unref (loop);
}

static void
out_cb (xobject_t      *object,
        xasync_result_t *result,
        xpointer_t      user_data)
{
  xerror_t *error = NULL;

  xassert (object == expected_obj);
  xassert (user_data == expected_data);
  xassert (callback_happened == FALSE);

  xoutput_stream_close_finish (expected_obj, result, &error);
  xassert (error == NULL);

  callback_happened = TRUE;
  xmain_loop_quit (loop);
}

static void
test_output_async (void)
{
  xoutput_stream_t *base, *f1, *f2;
  xasync_result_t *result = NULL;
  xerror_t *error = NULL;

  loop = xmain_loop_new (NULL, FALSE);

  base = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  f1 = xobject_new (TEST_TYPE_FILTER_OUTPUT_STREAM,
                     "base-stream", base,
                     "close-base-stream", FALSE,
                     NULL);
  f2 = xobject_new (TEST_TYPE_FILTER_OUTPUT_STREAM,
                     "base-stream", base,
                     NULL);

  xassert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f1)) == base);
  xassert (g_filter_output_stream_get_base_stream (G_FILTER_OUTPUT_STREAM (f2)) == base);


  xoutput_stream_write_async (f1, "abcdefghijklm", 13, G_PRIORITY_DEFAULT,
                               NULL, return_result_cb, &result);
  xmain_loop_run (loop);
  g_assert_cmpint (xoutput_stream_write_finish (f1, result, &error), ==, 13);
  g_assert_no_error (error);
  g_clear_object (&result);

  g_assert_cmpint (xseekable_tell (G_SEEKABLE (base)), ==, 13);

  xoutput_stream_write_async (f2, "nopqrstuvwxyz", 13, G_PRIORITY_DEFAULT,
                               NULL, return_result_cb, &result);
  xmain_loop_run (loop);
  g_assert_cmpint (xoutput_stream_write_finish (f2, result, &error), ==, 13);
  g_assert_no_error (error);
  g_clear_object (&result);

  g_assert_cmpint (xseekable_tell (G_SEEKABLE (base)), ==, 26);

  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 26);
  xoutput_stream_write (base, "\0", 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (base)), ==, "abcdefghijklmnopqrstuvwxyz");


  xassert (!xoutput_stream_is_closed (base));
  xassert (!xoutput_stream_is_closed (f1));
  xassert (!xoutput_stream_is_closed (f2));

  expected_obj = f1;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  xoutput_stream_close_async (f1, 0, NULL, out_cb, expected_data);

  xassert (callback_happened == FALSE);
  xmain_loop_run (loop);
  xassert (callback_happened == TRUE);

  xassert (!xoutput_stream_is_closed (base));
  xassert (!xoutput_stream_is_closed (f2));
  g_free (expected_data);
  xobject_unref (f1);
  xassert (!xoutput_stream_is_closed (base));
  xassert (!xoutput_stream_is_closed (f2));

  expected_obj = f2;
  expected_data = g_malloc (20);
  callback_happened = FALSE;
  xoutput_stream_close_async (f2, 0, NULL, out_cb, expected_data);

  xassert (callback_happened == FALSE);
  xmain_loop_run (loop);
  xassert (callback_happened == TRUE);

  xassert (xoutput_stream_is_closed (base));
  xassert (xoutput_stream_is_closed (f2));
  g_free (expected_data);
  xobject_unref (f2);

  xassert (xoutput_stream_is_closed (base));
  xobject_unref (base);
  xmain_loop_unref (loop);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/filter-stream/input", test_input_filter);
  g_test_add_func ("/filter-stream/output", test_output_filter);
  g_test_add_func ("/filter-stream/async-input", test_input_async);
  g_test_add_func ("/filter-stream/async-output", test_output_async);

  return g_test_run();
}
