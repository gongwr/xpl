#include <gio/gio.h>

static void
test_write (void)
{
  xoutput_stream_t *base;
  xoutput_stream_t *out;
  xerror_t *error;
  const xchar_t buffer[] = "abcdefghijklmnopqrstuvwxyz";

  base = g_memory_output_stream_new (g_malloc0 (20), 20, NULL, g_free);
  out = xbuffered_output_stream_new (base);

  g_assert_cmpint (xbuffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), ==, 4096);
  xassert (!xbuffered_output_stream_get_auto_grow (G_BUFFERED_OUTPUT_STREAM (out)));
  xobject_set (out, "auto-grow", TRUE, NULL);
  xassert (xbuffered_output_stream_get_auto_grow (G_BUFFERED_OUTPUT_STREAM (out)));
  xobject_set (out, "auto-grow", FALSE, NULL);

  xbuffered_output_stream_set_buffer_size (G_BUFFERED_OUTPUT_STREAM (out), 16);
  g_assert_cmpint (xbuffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), ==, 16);

  error = NULL;
  g_assert_cmpint (xoutput_stream_write (out, buffer, 10, NULL, &error), ==, 10);
  g_assert_no_error (error);

  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);

  g_assert_cmpint (xoutput_stream_write (out, buffer + 10, 10, NULL, &error), ==, 6);
  g_assert_no_error (error);

  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);
  xassert (xoutput_stream_flush (out, NULL, &error));
  g_assert_no_error (error);
  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 16);

  g_assert_cmpstr (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (base)), ==, "abcdefghijklmnop");

  xobject_unref (out);
  xobject_unref (base);
}

static void
test_grow (void)
{
  xoutput_stream_t *base;
  xoutput_stream_t *out;
  xerror_t *error;
  const xchar_t buffer[] = "abcdefghijklmnopqrstuvwxyz";
  xint_t size;
  xboolean_t grow;

  base = g_memory_output_stream_new (g_malloc0 (30), 30, g_realloc, g_free);
  out = xbuffered_output_stream_new_sized (base, 16);

  xbuffered_output_stream_set_auto_grow (G_BUFFERED_OUTPUT_STREAM (out), TRUE);

  xobject_get (out, "buffer-size", &size, "auto-grow", &grow, NULL);
  g_assert_cmpint (size, ==, 16);
  xassert (grow);

  xassert (xseekable_can_seek (G_SEEKABLE (out)));

  error = NULL;
  g_assert_cmpint (xoutput_stream_write (out, buffer, 10, NULL, &error), ==, 10);
  g_assert_no_error (error);

  g_assert_cmpint (xbuffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), ==, 16);
  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);

  g_assert_cmpint (xoutput_stream_write (out, buffer + 10, 10, NULL, &error), ==, 10);
  g_assert_no_error (error);

  g_assert_cmpint (xbuffered_output_stream_get_buffer_size (G_BUFFERED_OUTPUT_STREAM (out)), >=, 20);
  g_assert_cmpint (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (base)), ==, 0);

  xassert (xoutput_stream_flush (out, NULL, &error));
  g_assert_no_error (error);

  g_assert_cmpstr (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (base)), ==, "abcdefghijklmnopqrst");

  xobject_unref (out);
  xobject_unref (base);
}

static void
test_close (void)
{
  xoutput_stream_t *base;
  xoutput_stream_t *out;
  xerror_t *error;

  base = g_memory_output_stream_new (g_malloc0 (30), 30, g_realloc, g_free);
  out = xbuffered_output_stream_new (base);

  xassert (g_filter_output_stream_get_close_base_stream (G_FILTER_OUTPUT_STREAM (out)));

  error = NULL;
  xassert (xoutput_stream_close (out, NULL, &error));
  g_assert_no_error (error);
  xassert (xoutput_stream_is_closed (base));

  xobject_unref (out);
  xobject_unref (base);

  base = g_memory_output_stream_new (g_malloc0 (30), 30, g_realloc, g_free);
  out = xbuffered_output_stream_new (base);

  g_filter_output_stream_set_close_base_stream (G_FILTER_OUTPUT_STREAM (out), FALSE);

  error = NULL;
  xassert (xoutput_stream_close (out, NULL, &error));
  g_assert_no_error (error);
  xassert (!xoutput_stream_is_closed (base));

  xobject_unref (out);
  xobject_unref (base);
}

static void
test_seek (void)
{
  xmemory_output_stream_t *base;
  xoutput_stream_t *out;
  xseekable__t *seekable;
  xerror_t *error;
  xsize_t bytes_written;
  xboolean_t ret;
  const xchar_t buffer[] = "abcdefghijklmnopqrstuvwxyz";

  base = G_MEMORY_OUTPUT_STREAM (g_memory_output_stream_new (g_malloc0 (30), 30, NULL, g_free));
  out = xbuffered_output_stream_new_sized (G_OUTPUT_STREAM (base), 8);
  seekable = G_SEEKABLE (out);
  error = NULL;

  /* Write data */
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 0);
  ret = xoutput_stream_write_all (out, buffer, 4, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_written, ==, 4);
  xassert (ret);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 4);
  g_assert_cmpint (g_memory_output_stream_get_data_size (base), ==, 0);

  /* Forward relative seek */
  ret = xseekable_seek (seekable, 2, G_SEEK_CUR, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 6);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[0]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[1]);
  g_assert_cmpint ('c', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[2]);
  g_assert_cmpint ('d', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[3]);
  ret = xoutput_stream_write_all (out, buffer, 2, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (bytes_written, ==, 2);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 8);

  /* Backward relative seek */
  ret = xseekable_seek (seekable, -4, G_SEEK_CUR, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 4);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[0]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[1]);
  g_assert_cmpint ('c', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[2]);
  g_assert_cmpint ('d', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[3]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[6]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[7]);
  ret = xoutput_stream_write_all (out, buffer, 2, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (bytes_written, ==, 2);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 6);

  /* From start */
  ret = xseekable_seek (seekable, 2, G_SEEK_SET, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 2);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[0]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[1]);
  g_assert_cmpint ('c', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[2]);
  g_assert_cmpint ('d', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[3]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[4]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[5]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[6]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[7]);
  ret = xoutput_stream_write_all (out, buffer, 2, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (bytes_written, ==, 2);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 4);

  /* From end */
  ret = xseekable_seek (seekable, 6 - 30, G_SEEK_END, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 6);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[0]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[1]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[2]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[3]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[4]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[5]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[6]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[7]);
  ret = xoutput_stream_write_all (out, buffer + 2, 2, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (bytes_written, ==, 2);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 8);

  /* Check flush */
  ret = xoutput_stream_flush (out, NULL, &error);
  g_assert_no_error (error);
  xassert (ret);
  g_assert_cmpint (xseekable_tell (G_SEEKABLE (out)), ==, 8);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[0]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[1]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[2]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[3]);
  g_assert_cmpint ('a', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[4]);
  g_assert_cmpint ('b', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[5]);
  g_assert_cmpint ('c', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[6]);
  g_assert_cmpint ('d', ==, ((xchar_t *)g_memory_output_stream_get_data (base))[7]);

  xobject_unref (out);
  xobject_unref (base);
}

static void
test_truncate(void)
{
  xmemory_output_stream_t *base_stream;
  xoutput_stream_t *stream;
  xseekable__t *seekable;
  xerror_t *error;
  xsize_t bytes_written;
  xuchar_t *stream_data;
  xsize_t len;
  xboolean_t res;

  len = 8;

  /* Create objects */
  stream_data = g_malloc0 (len);
  base_stream = G_MEMORY_OUTPUT_STREAM (g_memory_output_stream_new (stream_data, len, g_realloc, g_free));
  stream = xbuffered_output_stream_new_sized (G_OUTPUT_STREAM (base_stream), 8);
  seekable = G_SEEKABLE (stream);

  xassert (xseekable_can_truncate (seekable));

  /* Write */
  g_assert_cmpint (g_memory_output_stream_get_size (base_stream), ==, len);
  g_assert_cmpint (g_memory_output_stream_get_data_size (base_stream), ==, 0);

  error = NULL;
  res = xoutput_stream_write_all (stream, "ab", 2, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  xassert (res);
  res = xoutput_stream_write_all (stream, "cd", 2, &bytes_written, NULL, &error);
  g_assert_no_error (error);
  xassert (res);

  res = xoutput_stream_flush (stream, NULL, &error);
  g_assert_no_error (error);
  xassert (res);

  g_assert_cmpint (g_memory_output_stream_get_size (base_stream), ==, len);
  g_assert_cmpint (g_memory_output_stream_get_data_size (base_stream), ==, 4);
  stream_data = g_memory_output_stream_get_data (base_stream);
  g_assert_cmpint (stream_data[0], ==, 'a');
  g_assert_cmpint (stream_data[1], ==, 'b');
  g_assert_cmpint (stream_data[2], ==, 'c');
  g_assert_cmpint (stream_data[3], ==, 'd');

  /* Truncate at position */
  res = xseekable_truncate (seekable, 4, NULL, &error);
  g_assert_no_error (error);
  xassert (res);
  g_assert_cmpint (g_memory_output_stream_get_size (base_stream), ==, 4);
  g_assert_cmpint (g_memory_output_stream_get_data_size (base_stream), ==, 4);
  stream_data = g_memory_output_stream_get_data (base_stream);
  g_assert_cmpint (stream_data[0], ==, 'a');
  g_assert_cmpint (stream_data[1], ==, 'b');
  g_assert_cmpint (stream_data[2], ==, 'c');
  g_assert_cmpint (stream_data[3], ==, 'd');

  /* Truncate beyond position */
  res = xseekable_truncate (seekable, 6, NULL, &error);
  g_assert_no_error (error);
  xassert (res);
  g_assert_cmpint (g_memory_output_stream_get_size (base_stream), ==, 6);
  g_assert_cmpint (g_memory_output_stream_get_data_size (base_stream), ==, 6);
  stream_data = g_memory_output_stream_get_data (base_stream);
  g_assert_cmpint (stream_data[0], ==, 'a');
  g_assert_cmpint (stream_data[1], ==, 'b');
  g_assert_cmpint (stream_data[2], ==, 'c');
  g_assert_cmpint (stream_data[3], ==, 'd');

  /* Truncate before position */
  res = xseekable_truncate (seekable, 2, NULL, &error);
  g_assert_no_error (error);
  xassert (res);
  g_assert_cmpint (g_memory_output_stream_get_size (base_stream), ==, 2);
  g_assert_cmpint (g_memory_output_stream_get_data_size (base_stream), ==, 2);
  stream_data = g_memory_output_stream_get_data (base_stream);
  g_assert_cmpint (stream_data[0], ==, 'a');
  g_assert_cmpint (stream_data[1], ==, 'b');

  xobject_unref (stream);
  xobject_unref (base_stream);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/buffered-output-stream/write", test_write);
  g_test_add_func ("/buffered-output-stream/grow", test_grow);
  g_test_add_func ("/buffered-output-stream/seek", test_seek);
  g_test_add_func ("/buffered-output-stream/truncate", test_truncate);
  g_test_add_func ("/filter-output-stream/close", test_close);

  return g_test_run ();
}
