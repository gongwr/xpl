#include <glib.h>

static void
test_dir_read (void)
{
  xdir_t *dir;
  xerror_t *error;
  xchar_t *first;
  const xchar_t *name;

  error = NULL;
  dir = g_dir_open (".", 0, &error);
  g_assert_no_error (error);

  first = NULL;
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (first == NULL)
        first = xstrdup (name);
      g_assert_cmpstr (name, !=, ".");
      g_assert_cmpstr (name, !=, "..");
    }

  g_dir_rewind (dir);
  g_assert_cmpstr (g_dir_read_name (dir), ==, first);

  g_free (first);
  g_dir_close (dir);
}

static void
test_dir_nonexisting (void)
{
  xdir_t *dir;
  xerror_t *error;

  error = NULL;
  dir = g_dir_open ("/pfrkstrf", 0, &error);
  xassert (dir == NULL);
  g_assert_error (error, XFILE_ERROR, XFILE_ERROR_NOENT);
  xerror_free (error);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/dir/read", test_dir_read);
  g_test_add_func ("/dir/nonexisting", test_dir_nonexisting);

  return g_test_run ();
}
