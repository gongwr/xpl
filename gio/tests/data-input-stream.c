/* GLib testing framework examples and tests
 * Copyright (C) 2008 Red Hat, Inc.
 * Authors: Tomas Bzatek <tbzatek@redhat.com>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 	0xFFF
#define MAX_BYTES	0x10000

static void
test_basic (void)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xint_t val;

  base_stream = g_memory_input_stream_new ();
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));

  xobject_get (stream, "byte-order", &val, NULL);
  g_assert_cmpint (val, ==, G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);
  xobject_set (stream, "byte-order", G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN, NULL);
  g_assert_cmpint (xdata_input_stream_get_byte_order (G_DATA_INPUT_STREAM (stream)), ==, G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN);

  xobject_get (stream, "newline-type", &val, NULL);
  g_assert_cmpint (val, ==, G_DATA_STREAM_NEWLINE_TYPE_LF);
  xobject_set (stream, "newline-type", G_DATA_STREAM_NEWLINE_TYPE_CR_LF, NULL);
  g_assert_cmpint (xdata_input_stream_get_newline_type (G_DATA_INPUT_STREAM (stream)), ==, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

  xobject_unref (stream);
  xobject_unref (base_stream);
}

static void
test_seek_to_start (xinput_stream_t *stream)
{
  xerror_t *error = NULL;
  xboolean_t res = xseekable_seek (G_SEEKABLE (stream), 0, G_SEEK_SET, NULL, &error);
  g_assert_cmpint (res, ==, TRUE);
  g_assert_no_error (error);
}

static void
test_read_lines (xdata_stream_newline_type_t newline_type)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xerror_t *error = NULL;
  char *data;
  int line;
  const char* lines[MAX_LINES];
  const char* endl[4] = {"\n", "\r", "\r\n", "\n"};

  /*  prepare data */
  int i;
  for (i = 0; i < MAX_LINES; i++)
    lines[i] = "some_text";

  base_stream = g_memory_input_stream_new ();
  xassert (base_stream != NULL);
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));
  xassert(stream != NULL);

  /*  Byte order testing */
  xdata_input_stream_set_byte_order (G_DATA_INPUT_STREAM (stream), G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);
  g_assert_cmpint (xdata_input_stream_get_byte_order (G_DATA_INPUT_STREAM (stream)), ==, G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);
  xdata_input_stream_set_byte_order (G_DATA_INPUT_STREAM (stream), G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN);
  g_assert_cmpint (xdata_input_stream_get_byte_order (G_DATA_INPUT_STREAM (stream)), ==, G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN);

  /*  Line ends testing */
  xdata_input_stream_set_newline_type (G_DATA_INPUT_STREAM (stream), newline_type);
  g_assert_cmpint (xdata_input_stream_get_newline_type (G_DATA_INPUT_STREAM (stream)), ==, newline_type);


  /*  Add sample data */
  for (i = 0; i < MAX_LINES; i++)
    g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (base_stream),
				    xstrconcat (lines[i], endl[newline_type], NULL), -1, g_free);

  /*  Seek to the start */
  test_seek_to_start (base_stream);

  /*  test_t read line */
  error = NULL;
  data = (char*)1;
  line = 0;
  while (data)
    {
      xsize_t length = -1;
      data = xdata_input_stream_read_line (G_DATA_INPUT_STREAM (stream), &length, NULL, &error);
      if (data)
	{
	  g_assert_cmpstr (data, ==, lines[line]);
          g_free (data);
	  g_assert_no_error (error);
	  line++;
	}
      if (error)
        xerror_free (error);
    }
  g_assert_cmpint (line, ==, MAX_LINES);


  xobject_unref (base_stream);
  xobject_unref (stream);
}

static void
test_read_lines_LF (void)
{
  test_read_lines (G_DATA_STREAM_NEWLINE_TYPE_LF);
}

static void
test_read_lines_CR (void)
{
  test_read_lines (G_DATA_STREAM_NEWLINE_TYPE_CR);
}

static void
test_read_lines_CR_LF (void)
{
  test_read_lines (G_DATA_STREAM_NEWLINE_TYPE_CR_LF);
}

static void
test_read_lines_any (void)
{
  test_read_lines (G_DATA_STREAM_NEWLINE_TYPE_ANY);
}

static void
test_read_lines_LF_valid_utf8 (void)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xerror_t *error = NULL;
  char *line;
  xuint_t n_lines = 0;

  base_stream = g_memory_input_stream_new ();
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));

  g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (base_stream),
				  "foo\nthis is valid UTF-8 ???!\nbar\n", -1, NULL);

  /*  test_t read line */
  error = NULL;
  while (TRUE)
    {
      xsize_t length = -1;
      line = xdata_input_stream_read_line_utf8 (G_DATA_INPUT_STREAM (stream), &length, NULL, &error);
      g_assert_no_error (error);
      if (line == NULL)
	break;
      n_lines++;
      g_free (line);
    }
  g_assert_cmpint (n_lines, ==, 3);

  xobject_unref (base_stream);
  xobject_unref (stream);
}

static void
test_read_lines_LF_invalid_utf8 (void)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xerror_t *error = NULL;
  char *line;
  xuint_t n_lines = 0;

  base_stream = g_memory_input_stream_new ();
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));

  g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (base_stream),
				  "foo\nthis is not valid UTF-8 \xE5 =(\nbar\n", -1, NULL);

  /*  test_t read line */
  error = NULL;
  while (TRUE)
    {
      xsize_t length = -1;
      line = xdata_input_stream_read_line_utf8 (G_DATA_INPUT_STREAM (stream), &length, NULL, &error);
      if (n_lines == 0)
	g_assert_no_error (error);
      else
	{
	  xassert (error != NULL);
	  g_clear_error (&error);
	  g_free (line);
	  break;
	}
      n_lines++;
      g_free (line);
    }
  g_assert_cmpint (n_lines, ==, 1);

  xobject_unref (base_stream);
  xobject_unref (stream);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_read_until (void)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xerror_t *error = NULL;
  char *data;
  int line;
  int i;

#define REPEATS			10   /* number of rounds */
#define DATA_STRING		" part1 # part2 $ part3 % part4 ^"
#define DATA_PART_LEN		7    /* number of characters between separators */
#define DATA_SEP		"#$%^"
#define DATA_SEP_LEN            4
  const int DATA_PARTS_NUM = DATA_SEP_LEN * REPEATS;

  base_stream = g_memory_input_stream_new ();
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));

  for (i = 0; i < REPEATS; i++)
    g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (base_stream), DATA_STRING, -1, NULL);

  /*  test_t stop characters */
  error = NULL;
  data = (char*)1;
  line = 0;
  while (data)
    {
      xsize_t length = -1;
      data = xdata_input_stream_read_until (G_DATA_INPUT_STREAM (stream), DATA_SEP, &length, NULL, &error);
      if (data)
	{
	  g_assert_cmpint (strlen (data), ==, DATA_PART_LEN);
          g_free (data);
	  g_assert_no_error (error);
	  line++;
	}
    }
  g_assert_no_error (error);
  g_assert_cmpint (line, ==, DATA_PARTS_NUM);

  xobject_unref (base_stream);
  xobject_unref (stream);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_read_upto (void)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xerror_t *error = NULL;
  char *data;
  int line;
  int i;
  xuchar_t stop_char;

#undef REPEATS
#undef DATA_STRING
#undef DATA_PART_LEN
#undef DATA_SEP
#undef DATA_SEP_LEN
#define REPEATS			10   /* number of rounds */
#define DATA_STRING		" part1 # part2 $ part3 \0 part4 ^"
#define DATA_PART_LEN		7    /* number of characters between separators */
#define DATA_SEP		"#$\0^"
#define DATA_SEP_LEN            4
  const int DATA_PARTS_NUM = DATA_SEP_LEN * REPEATS;

  base_stream = g_memory_input_stream_new ();
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));

  for (i = 0; i < REPEATS; i++)
    g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (base_stream), DATA_STRING, 32, NULL);

  /*  test_t stop characters */
  error = NULL;
  data = (char*)1;
  line = 0;
  while (data)
    {
      xsize_t length = -1;
      data = xdata_input_stream_read_upto (G_DATA_INPUT_STREAM (stream), DATA_SEP, DATA_SEP_LEN, &length, NULL, &error);
      if (data)
        {
          g_assert_cmpint (strlen (data), ==, DATA_PART_LEN);
          g_assert_no_error (error);
          line++;

          stop_char = xdata_input_stream_read_byte (G_DATA_INPUT_STREAM (stream), NULL, &error);
          xassert (memchr (DATA_SEP, stop_char, DATA_SEP_LEN) != NULL);
          g_assert_no_error (error);
        }
      g_free (data);
    }
  g_assert_no_error (error);
  g_assert_cmpint (line, ==, DATA_PARTS_NUM);

  xobject_unref (base_stream);
  xobject_unref (stream);
}
enum TestDataType {
  TEST_DATA_BYTE = 0,
  TEST_DATA_INT16,
  TEST_DATA_UINT16,
  TEST_DATA_INT32,
  TEST_DATA_UINT32,
  TEST_DATA_INT64,
  TEST_DATA_UINT64
};

/* The order is reversed to avoid -Wduplicated-branches. */
#define TEST_DATA_RETYPE_BUFF(a, t, v)	\
	 (a == TEST_DATA_UINT64	? (t) *(xuint64_t*)v :	\
	 (a == TEST_DATA_INT64	? (t) *(sint64_t*)v :	\
	 (a == TEST_DATA_UINT32	? (t) *(xuint32_t*)v :	\
	 (a == TEST_DATA_INT32	? (t) *(gint32*)v :	\
	 (a == TEST_DATA_UINT16	? (t) *(xuint16_t*)v :	\
	 (a == TEST_DATA_INT16	? (t) *(gint16*)v :	\
	 (t) *(xuchar_t*)v ))))))


static void
test_data_array (xinput_stream_t *stream, xinput_stream_t *base_stream,
		 xpointer_t buffer, int len,
		 enum TestDataType data_type, xdata_stream_byte_order_t byte_order)
{
  xerror_t *error = NULL;
  int pos = 0;
  int data_size = 1;
  sint64_t data;
  xdata_stream_byte_order_t native;
  xboolean_t swap;

  /*  Seek to start */
  test_seek_to_start (base_stream);

  /*  Set correct data size */
  switch (data_type)
    {
    case TEST_DATA_BYTE:
      data_size = 1;
      break;
    case TEST_DATA_INT16:
    case TEST_DATA_UINT16:
      data_size = 2;
      break;
    case TEST_DATA_INT32:
    case TEST_DATA_UINT32:
      data_size = 4;
      break;
    case TEST_DATA_INT64:
    case TEST_DATA_UINT64:
      data_size = 8;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /*  Set flag to swap bytes if needed */
  native = (G_BYTE_ORDER == G_BIG_ENDIAN) ? G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN : G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN;
  swap = (byte_order != G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN) && (byte_order != native);

  data = 1;
  while (data != 0)
    {
      switch (data_type)
	{
	case TEST_DATA_BYTE:
	  data = xdata_input_stream_read_byte (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  break;
	case TEST_DATA_INT16:
	  data = xdata_input_stream_read_int16 (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  if (swap)
	    data = (gint16)GUINT16_SWAP_LE_BE((gint16)data);
	  break;
	case TEST_DATA_UINT16:
	  data = xdata_input_stream_read_uint16 (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  if (swap)
	    data = (xuint16_t)GUINT16_SWAP_LE_BE((xuint16_t)data);
	  break;
	case TEST_DATA_INT32:
	  data = xdata_input_stream_read_int32 (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  if (swap)
	    data = (gint32)GUINT32_SWAP_LE_BE((gint32)data);
	  break;
	case TEST_DATA_UINT32:
	  data = xdata_input_stream_read_uint32 (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  if (swap)
	    data = (xuint32_t)GUINT32_SWAP_LE_BE((xuint32_t)data);
	  break;
	case TEST_DATA_INT64:
	  data = xdata_input_stream_read_int64 (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  if (swap)
	    data = (sint64_t)GUINT64_SWAP_LE_BE((sint64_t)data);
	  break;
	case TEST_DATA_UINT64:
	  data = xdata_input_stream_read_uint64 (G_DATA_INPUT_STREAM (stream), NULL, &error);
	  if (swap)
	    data = (xuint64_t)GUINT64_SWAP_LE_BE((xuint64_t)data);
	  break;
        default:
          g_assert_not_reached ();
          break;
	}
      if (!error)
	g_assert_cmpint (data, ==, TEST_DATA_RETYPE_BUFF(data_type, sint64_t, ((xuchar_t*)buffer + pos)));

      pos += data_size;
    }
  if (pos < len + 1)
    g_assert_no_error (error);
  if (error)
    xerror_free (error);
  g_assert_cmpint (pos - data_size, ==, len);
}

static void
test_read_int (void)
{
  xinput_stream_t *stream;
  xinput_stream_t *base_stream;
  xrand_t *randomizer;
  int i;
  xpointer_t buffer;

  randomizer = g_rand_new ();
  buffer = g_malloc0 (MAX_BYTES);

  /*  Fill in some random data */
  for (i = 0; i < MAX_BYTES; i++)
    {
      xuchar_t x = 0;
      while (! x)
	x = (xuchar_t)g_rand_int (randomizer);
      *(xuchar_t*)((xuchar_t*)buffer + sizeof(xuchar_t) * i) = x;
    }

  base_stream = g_memory_input_stream_new ();
  stream = G_INPUT_STREAM (xdata_input_stream_new (base_stream));
  g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (base_stream), buffer, MAX_BYTES, NULL);


  for (i = 0; i < 3; i++)
    {
      int j;
      xdata_input_stream_set_byte_order (G_DATA_INPUT_STREAM (stream), i);

      for (j = 0; j <= TEST_DATA_UINT64; j++)
	test_data_array (stream, base_stream, buffer, MAX_BYTES, j, i);
    }

  xobject_unref (base_stream);
  xobject_unref (stream);
  g_rand_free (randomizer);
  g_free (buffer);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/data-input-stream/basic", test_basic);
  g_test_add_func ("/data-input-stream/read-lines-LF", test_read_lines_LF);
  g_test_add_func ("/data-input-stream/read-lines-LF-valid-utf8", test_read_lines_LF_valid_utf8);
  g_test_add_func ("/data-input-stream/read-lines-LF-invalid-utf8", test_read_lines_LF_invalid_utf8);
  g_test_add_func ("/data-input-stream/read-lines-CR", test_read_lines_CR);
  g_test_add_func ("/data-input-stream/read-lines-CR-LF", test_read_lines_CR_LF);
  g_test_add_func ("/data-input-stream/read-lines-any", test_read_lines_any);
  g_test_add_func ("/data-input-stream/read-until", test_read_until);
  g_test_add_func ("/data-input-stream/read-upto", test_read_upto);
  g_test_add_func ("/data-input-stream/read-int", test_read_int);

  return g_test_run();
}
