#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h>
#endif

static void
test_basic (void)
{
  xmapped_file_t *file;
  xerror_t *error;

  error = NULL;
  file = xmapped_file_new (g_test_get_filename (G_TEST_DIST, "empty", NULL), FALSE, &error);
  g_assert_no_error (error);

  xmapped_file_ref (file);
  xmapped_file_unref (file);

  xmapped_file_unref (file);
}

static void
test_empty (void)
{
  xmapped_file_t *file;
  xerror_t *error;

  error = NULL;
  file = xmapped_file_new (g_test_get_filename (G_TEST_DIST, "empty", NULL), FALSE, &error);
  g_assert_no_error (error);

  g_assert_null (xmapped_file_get_contents (file));

  xmapped_file_free (file);
}

#ifdef G_OS_UNIX
static void
test_device (void)
{
  xerror_t *error = NULL;
  xmapped_file_t *file;

  file = xmapped_file_new ("/dev/null", FALSE, &error);
  g_assert_true (xerror_matches (error, XFILE_ERROR, XFILE_ERROR_INVAL) ||
                 xerror_matches (error, XFILE_ERROR, XFILE_ERROR_NODEV) ||
                 xerror_matches (error, XFILE_ERROR, XFILE_ERROR_NOMEM));
  g_assert_null (file);
  xerror_free (error);
}
#endif

static void
test_nonexisting (void)
{
  xmapped_file_t *file;
  xerror_t *error;

  error = NULL;
  file = xmapped_file_new ("no-such-file", FALSE, &error);
  g_assert_error (error, XFILE_ERROR, XFILE_ERROR_NOENT);
  g_clear_error (&error);
  g_assert_null (file);
}

static void
test_writable (void)
{
  xmapped_file_t *file;
  xerror_t *error = NULL;
  xchar_t *contents;
  xsize_t len;
  const xchar_t *old = "MMMMMMMMMMMMMMMMMMMMMMMMM";
  const xchar_t *new = "abcdefghijklmnopqrstuvxyz";
  xchar_t *tmp_copy_path;

  tmp_copy_path = g_build_filename (g_get_tmp_dir (), "glib-test-4096-random-bytes", NULL);

  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "4096-random-bytes", NULL), &contents, &len, &error);
  g_assert_no_error (error);
  xfile_set_contents (tmp_copy_path, contents, len, &error);
  g_assert_no_error (error);

  g_free (contents);

  file = xmapped_file_new (tmp_copy_path, TRUE, &error);
  g_assert_no_error (error);

  contents = xmapped_file_get_contents (file);
  g_assert_cmpuint (strncmp (contents, old, strlen (old)), ==, 0);

  memcpy (contents, new, strlen (new));
  g_assert_cmpuint (strncmp (contents, new, strlen (new)), ==, 0);

  xmapped_file_free (file);

  error = NULL;
  file = xmapped_file_new (tmp_copy_path, FALSE, &error);
  g_assert_no_error (error);

  contents = xmapped_file_get_contents (file);
  g_assert_cmpuint (strncmp (contents, old, strlen (old)), ==, 0);

  xmapped_file_free (file);

  g_free (tmp_copy_path);
}

static void
test_writable_fd (void)
{
  xmapped_file_t *file;
  xerror_t *error = NULL;
  xchar_t *contents;
  const xchar_t *old = "MMMMMMMMMMMMMMMMMMMMMMMMM";
  const xchar_t *new = "abcdefghijklmnopqrstuvxyz";
  xsize_t len;
  int fd;
  xchar_t *tmp_copy_path;

  tmp_copy_path = g_build_filename (g_get_tmp_dir (), "glib-test-4096-random-bytes", NULL);

  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "4096-random-bytes", NULL), &contents, &len, &error);
  g_assert_no_error (error);
  xfile_set_contents (tmp_copy_path, contents, len, &error);
  g_assert_no_error (error);

  g_free (contents);

  fd = g_open (tmp_copy_path, O_RDWR, 0);
  g_assert_cmpint (fd, !=, -1);
  file = xmapped_file_new_from_fd (fd, TRUE, &error);
  g_assert_no_error (error);

  contents = xmapped_file_get_contents (file);
  g_assert_cmpuint (strncmp (contents, old, strlen (old)), ==, 0);

  memcpy (contents, new, strlen (new));
  g_assert_cmpuint (strncmp (contents, new, strlen (new)), ==, 0);

  xmapped_file_free (file);
  close (fd);

  error = NULL;
  fd = g_open (tmp_copy_path, O_RDWR, 0);
  g_assert_cmpint (fd, !=, -1);
  file = xmapped_file_new_from_fd (fd, TRUE, &error);
  g_assert_no_error (error);

  contents = xmapped_file_get_contents (file);
  g_assert_cmpuint (strncmp (contents, old, strlen (old)), ==, 0);

  xmapped_file_free (file);

  g_free (tmp_copy_path);
}

static void
test_gbytes (void)
{
  xmapped_file_t *file;
  xbytes_t *bytes;
  xerror_t *error;

  error = NULL;
  file = xmapped_file_new (g_test_get_filename (G_TEST_DIST, "empty", NULL), FALSE, &error);
  g_assert_no_error (error);

  bytes = xmapped_file_get_bytes (file);
  xmapped_file_unref (file);

  g_assert_cmpint (xbytes_get_size (bytes), ==, 0);
  xbytes_unref (bytes);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/mappedfile/basic", test_basic);
  g_test_add_func ("/mappedfile/empty", test_empty);
#ifdef G_OS_UNIX
  g_test_add_func ("/mappedfile/device", test_device);
#endif
  g_test_add_func ("/mappedfile/nonexisting", test_nonexisting);
  g_test_add_func ("/mappedfile/writable", test_writable);
  g_test_add_func ("/mappedfile/writable_fd", test_writable_fd);
  g_test_add_func ("/mappedfile/gbytes", test_gbytes);

  return g_test_run ();
}
