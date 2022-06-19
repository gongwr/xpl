#include <glib/glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <string.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h> /* for close() */
#endif

static const char *original_data = "This is some test data that we can put in a file...";
static const char *new_data = "new data..";

static void
verify_pos (xio_stream_t *iostream, xoffset_t expected_pos)
{
  xoffset_t pos;

  pos = xseekable_tell (G_SEEKABLE (iostream));
  g_assert_cmpint (pos, ==, expected_pos);

  pos = xseekable_tell (G_SEEKABLE (g_io_stream_get_input_stream (iostream)));
  g_assert_cmpint (pos, ==, expected_pos);

  pos = xseekable_tell (G_SEEKABLE (g_io_stream_get_output_stream (iostream)));
  g_assert_cmpint (pos, ==, expected_pos);
}

static void
verify_iostream (xfile_io_stream_t *file_iostream)
{
  xboolean_t res;
  xssize_t skipped;
  xio_stream_t *iostream;
  xerror_t *error;
  xinput_stream_t *in;
  xoutput_stream_t *out;
  char buffer[1024];
  xsize_t n_bytes;
  char *modified_data;

  iostream = XIO_STREAM (file_iostream);

  verify_pos (iostream, 0);

  in = g_io_stream_get_input_stream (iostream);
  out = g_io_stream_get_output_stream (iostream);

  res = xinput_stream_read_all (in, buffer, 20, &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpmem (buffer, n_bytes, original_data, 20);

  verify_pos (iostream, 20);

  res = xseekable_seek (G_SEEKABLE (iostream),
			 -10, G_SEEK_END,
			 NULL, NULL);
  xassert (res);
  verify_pos (iostream, strlen (original_data) - 10);

  res = xinput_stream_read_all (in, buffer, 20, &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpmem (buffer, n_bytes, original_data + strlen (original_data) - 10, 10);

  verify_pos (iostream, strlen (original_data));

  res = xseekable_seek (G_SEEKABLE (iostream),
			 10, G_SEEK_SET,
			 NULL, NULL);

  res = xinput_stream_skip (in, 5, NULL, NULL);
  xassert (res == 5);
  verify_pos (iostream, 15);

  skipped = xinput_stream_skip (in, 10000, NULL, NULL);
  g_assert_cmpint (skipped, >=, 0);
  xassert ((xsize_t) skipped == strlen (original_data) - 15);
  verify_pos (iostream, strlen (original_data));

  res = xseekable_seek (G_SEEKABLE (iostream),
			 10, G_SEEK_SET,
			 NULL, NULL);

  verify_pos (iostream, 10);

  res = xoutput_stream_write_all (out, new_data, strlen (new_data),
				   &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpint (n_bytes, ==, strlen (new_data));

  verify_pos (iostream, 10 + strlen (new_data));

  res = xseekable_seek (G_SEEKABLE (iostream),
			 0, G_SEEK_SET,
			 NULL, NULL);
  xassert (res);
  verify_pos (iostream, 0);

  res = xinput_stream_read_all (in, buffer, strlen (original_data), &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpint ((int)n_bytes, ==, strlen (original_data));
  buffer[n_bytes] = 0;

  modified_data = xstrdup (original_data);
  memcpy (modified_data + 10, new_data, strlen (new_data));
  g_assert_cmpstr (buffer, ==, modified_data);

  verify_pos (iostream, strlen (original_data));

  res = xseekable_seek (G_SEEKABLE (iostream),
			 0, G_SEEK_SET,
			 NULL, NULL);
  xassert (res);
  verify_pos (iostream, 0);

  res = xoutput_stream_close (out, NULL, NULL);
  xassert (res);

  res = xinput_stream_read_all (in, buffer, 15, &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpmem (buffer, n_bytes, modified_data, 15);

  error = NULL;
  res = xoutput_stream_write_all (out, new_data, strlen (new_data),
				   &n_bytes, NULL, &error);
  xassert (!res);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  xerror_free (error);

  error = NULL;
  res = g_io_stream_close (iostream, NULL, &error);
  xassert (res);
  g_assert_no_error (error);

  g_free (modified_data);
}

static void
test_xfile_open_readwrite (void)
{
  char *tmp_file;
  int fd;
  xboolean_t res;
  xfile_io_stream_t *file_iostream;
  char *path;
  xfile_t *file;
  xerror_t *error;

  fd = xfile_open_tmp ("readwrite_XXXXXX",
			&tmp_file, NULL);
  xassert (fd != -1);
  close (fd);

  res = xfile_set_contents (tmp_file,
			     original_data, -1, NULL);
  xassert (res);

  path = g_build_filename (g_get_tmp_dir (), "g-a-nonexisting-file", NULL);
  file = xfile_new_for_path (path);
  g_free (path);
  error = NULL;
  file_iostream = xfile_open_readwrite (file, NULL, &error);
  xassert (file_iostream == NULL);
  xassert (xerror_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND));
  xerror_free (error);
  xobject_unref (file);

  file = xfile_new_for_path (tmp_file);
  error = NULL;
  file_iostream = xfile_open_readwrite (file, NULL, &error);
  xassert (file_iostream != NULL);
  xobject_unref (file);

  verify_iostream (file_iostream);

  xobject_unref (file_iostream);

  g_unlink (tmp_file);
  g_free (tmp_file);
}

static void
test_xfile_create_readwrite (void)
{
  char *tmp_file;
  int fd;
  xboolean_t res;
  xfile_io_stream_t *file_iostream;
  xoutput_stream_t *out;
  xfile_t *file;
  xerror_t *error;
  xsize_t n_bytes;

  fd = xfile_open_tmp ("readwrite_XXXXXX",
			&tmp_file, NULL);
  xassert (fd != -1);
  close (fd);

  file = xfile_new_for_path (tmp_file);
  error = NULL;
  file_iostream = xfile_create_readwrite (file, 0, NULL, &error);
  xassert (file_iostream == NULL);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
  xerror_free (error);

  g_unlink (tmp_file);
  file_iostream = xfile_create_readwrite (file, 0, NULL, &error);
  xassert (file_iostream != NULL);

  out = g_io_stream_get_output_stream (XIO_STREAM (file_iostream));
  res = xoutput_stream_write_all (out, original_data, strlen (original_data),
				   &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpint (n_bytes, ==, strlen (original_data));

  res = xseekable_seek (G_SEEKABLE (file_iostream),
			 0, G_SEEK_SET,
			 NULL, NULL);
  xassert (res);

  verify_iostream (file_iostream);

  xobject_unref (file_iostream);
  xobject_unref (file);

  g_unlink (tmp_file);
  g_free (tmp_file);
}

static void
test_xfile_replace_readwrite (void)
{
  char *tmp_file, *backup, *data;
  int fd;
  xboolean_t res;
  xfile_io_stream_t *file_iostream;
  xinput_stream_t *in;
  xoutput_stream_t *out;
  xfile_t *file;
  xerror_t *error;
  char buffer[1024];
  xsize_t n_bytes;

  fd = xfile_open_tmp ("readwrite_XXXXXX",
			&tmp_file, NULL);
  xassert (fd != -1);
  close (fd);

  res = xfile_set_contents (tmp_file,
			     new_data, -1, NULL);
  xassert (res);

  file = xfile_new_for_path (tmp_file);
  error = NULL;
  file_iostream = xfile_replace_readwrite (file, NULL,
					    TRUE, 0, NULL, &error);
  xassert (file_iostream != NULL);

  in = g_io_stream_get_input_stream (XIO_STREAM (file_iostream));

  /* Ensure its empty */
  res = xinput_stream_read_all (in, buffer, sizeof buffer, &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpint ((int)n_bytes, ==, 0);

  out = g_io_stream_get_output_stream (XIO_STREAM (file_iostream));
  res = xoutput_stream_write_all (out, original_data, strlen (original_data),
				   &n_bytes, NULL, NULL);
  xassert (res);
  g_assert_cmpint (n_bytes, ==, strlen (original_data));

  res = xseekable_seek (G_SEEKABLE (file_iostream),
			 0, G_SEEK_SET,
			 NULL, NULL);
  xassert (res);

  verify_iostream (file_iostream);

  xobject_unref (file_iostream);
  xobject_unref (file);

  backup = xstrconcat (tmp_file, "~", NULL);
  res = xfile_get_contents (backup,
			     &data,
			     NULL, NULL);
  xassert (res);
  g_assert_cmpstr (data, ==, new_data);
  g_free (data);
  g_unlink (backup);
  g_free (backup);

  g_unlink (tmp_file);
  g_free (tmp_file);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/readwrite/test_xfile_open_readwrite",
		   test_xfile_open_readwrite);
  g_test_add_func ("/readwrite/test_xfile_create_readwrite",
		   test_xfile_create_readwrite);
  g_test_add_func ("/readwrite/test_xfile_replace_readwrite",
		   test_xfile_replace_readwrite);

  return g_test_run();
}
