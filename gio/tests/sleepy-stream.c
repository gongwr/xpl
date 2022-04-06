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

#include <gio/gio.h>
#include <string.h>

#define MAX_PIECE_SIZE  100
#define MAX_PIECES       60

static xchar_t *
cook_piece (void)
{
  char buffer[MAX_PIECE_SIZE * 2];
  xint_t symbols, i = 0;

  symbols = g_test_rand_int_range (1, MAX_PIECE_SIZE + 1);

  while (symbols--)
    {
      xint_t c = g_test_rand_int_range (0, 30);

      switch (c)
        {
         case 26:
          buffer[i++] = '\n';
          G_GNUC_FALLTHROUGH;
         case 27:
          buffer[i++] = '\r';
          break;

         case 28:
          buffer[i++] = '\r';
          G_GNUC_FALLTHROUGH;
         case 29:
          buffer[i++] = '\n';
          break;

         default:
          buffer[i++] = c + 'a';
          break;
        }

      g_assert_cmpint (i, <=, sizeof buffer);
    }

  return xstrndup (buffer, i);
}

static xchar_t **
cook_pieces (void)
{
  xchar_t **array;
  xint_t pieces;

  pieces = g_test_rand_int_range (0, MAX_PIECES + 1);
  array = g_new (char *, pieces + 1);
  array[pieces] = NULL;

  while (pieces--)
    array[pieces] = cook_piece ();

  return array;
}

typedef struct
{
  xinput_stream_t parent_instance;

  xboolean_t built_to_fail;
  xchar_t **pieces;
  xint_t index;

  const xchar_t *current;
} SleepyStream;

typedef GInputStreamClass SleepyStreamClass;

xtype_t sleepy_stream_get_type (void);

G_DEFINE_TYPE (SleepyStream, sleepy_stream, XTYPE_INPUT_STREAM)

static xssize_t
sleepy_stream_read (xinput_stream_t  *stream,
                    void          *buffer,
                    xsize_t          length,
                    xcancellable_t  *cancellable,
                    xerror_t       **error)
{
  SleepyStream *sleepy = (SleepyStream *) stream;

  if (sleepy->pieces[sleepy->index] == NULL)
    {
      if (sleepy->built_to_fail)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "fail");
          return -1;
        }
      else
        return 0;
    }
  else
    {
      if (!sleepy->current)
        sleepy->current = sleepy->pieces[sleepy->index++];

      length = MIN (strlen (sleepy->current), length);
      memcpy (buffer, sleepy->current, length);

      sleepy->current += length;
      if (*sleepy->current == '\0')
        sleepy->current = NULL;

      return length;
    }
}

static void
sleepy_stream_init (SleepyStream *sleepy)
{
  sleepy->pieces = cook_pieces ();
  sleepy->built_to_fail = FALSE;
  sleepy->index = 0;
}

static void
sleepy_stream_finalize (xobject_t *object)
{
  SleepyStream *sleepy = (SleepyStream *) object;

  xstrfreev (sleepy->pieces);
  G_OBJECT_CLASS (sleepy_stream_parent_class)
    ->finalize (object);
}

static void
sleepy_stream_class_init (SleepyStreamClass *class)
{
  G_OBJECT_CLASS (class)->finalize = sleepy_stream_finalize;
  class->read_fn = sleepy_stream_read;

  /* no read_async implementation.
   * main thread will sleep while read runs in a worker.
   */
}

static SleepyStream *
sleepy_stream_new (void)
{
  return xobject_new (sleepy_stream_get_type (), NULL);
}

static xboolean_t
read_line (xdata_input_stream_t  *stream,
           xstring_t           *string,
           const xchar_t       *eol,
           xerror_t           **error)
{
  xsize_t length;
  char *str;

  str = g_data_input_stream_read_line (stream, &length, NULL, error);

  if (str == NULL)
    return FALSE;

  g_assert (strstr (str, eol) == NULL);
  g_assert (strlen (str) == length);

  xstring_append (string, str);
  xstring_append (string, eol);
  g_free (str);

  return TRUE;
}

static void
build_comparison (xstring_t      *str,
                  SleepyStream *stream)
{
  /* build this for comparison */
  xint_t i;

  for (i = 0; stream->pieces[i]; i++)
    xstring_append (str, stream->pieces[i]);

  if (str->len && str->str[str->len - 1] != '\n')
    xstring_append_c (str, '\n');
}


static void
test (void)
{
  SleepyStream *stream = sleepy_stream_new ();
  xdata_input_stream_t *data;
  xerror_t *error = NULL;
  xstring_t *one;
  xstring_t *two;

  one = xstring_new (NULL);
  two = xstring_new (NULL);

  data = g_data_input_stream_new (G_INPUT_STREAM (stream));
  g_data_input_stream_set_newline_type (data, G_DATA_STREAM_NEWLINE_TYPE_LF);
  build_comparison (one, stream);

  while (read_line (data, two, "\n", &error));

  g_assert_cmpstr (one->str, ==, two->str);
  xstring_free (one, TRUE);
  xstring_free (two, TRUE);
  xobject_unref (stream);
  xobject_unref (data);
}

static xdata_input_stream_t *data;
static xstring_t *one, *two;
static xmain_loop_t *loop;
static const xchar_t *eol;

static void
asynch_ready (xobject_t      *object,
              xasync_result_t *result,
              xpointer_t      user_data)
{
  xerror_t *error = NULL;
  xsize_t length;
  xchar_t *str;

  g_assert (data == G_DATA_INPUT_STREAM (object));

  str = g_data_input_stream_read_line_finish (data, result, &length, &error);

  if (str == NULL)
    {
      xmain_loop_quit (loop);
      if (error)
        xerror_free (error);
    }
  else
    {
      g_assert (length == strlen (str));
      xstring_append (two, str);
      xstring_append (two, eol);
      g_free (str);

      /* MOAR!! */
      g_data_input_stream_read_line_async (data, 0, NULL, asynch_ready, NULL);
    }
}


static void
asynch (void)
{
  SleepyStream *sleepy = sleepy_stream_new ();

  data = g_data_input_stream_new (G_INPUT_STREAM (sleepy));
  one = xstring_new (NULL);
  two = xstring_new (NULL);
  eol = "\n";

  build_comparison (one, sleepy);
  g_data_input_stream_read_line_async (data, 0, NULL, asynch_ready, NULL);
  xmain_loop_run (loop = xmain_loop_new (NULL, FALSE));

  g_assert_cmpstr (one->str, ==, two->str);
  xstring_free (one, TRUE);
  xstring_free (two, TRUE);
  xobject_unref (sleepy);
  xobject_unref (data);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/filter-stream/input", test);
  g_test_add_func ("/filter-stream/async", asynch);

  return g_test_run();
}
