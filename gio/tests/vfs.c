
/* Unit tests for xvfs_t
 * Copyright (C) 2011 Red Hat, Inc
 * Author: Matthias Clasen
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

#include <gio/gio.h>

static xfile_t *
test_vfs_parse_name (xvfs_t       *vfs,
                     const char *parse_name,
                     xpointer_t    user_data)
{
  xfile_t *file = NULL;

  if (xstrcmp0 ((parse_name), "testfile") == 0)
    {
      file = xfile_new_for_uri ("file:///");
      xobject_set_data (G_OBJECT (file), "testfile", GINT_TO_POINTER (1));
    }

  return file;
}

static xfile_t *
test_vfs_lookup (xvfs_t       *vfs,
                 const char *uri,
                 xpointer_t    user_data)
{
  xfile_t *file;
  file = xfile_new_for_uri ("file:///");
  xobject_set_data (G_OBJECT (file), "testfile", GINT_TO_POINTER (1));

  return file;
}

static void
test_register_scheme (void)
{
  xvfs_t *vfs;
  xfile_t *file;
  const xchar_t * const *schemes;
  xboolean_t res;

  vfs = xvfs_get_default ();
  g_assert_nonnull (vfs);
  g_assert_true (xvfs_is_active (vfs));

  schemes = xvfs_get_supported_uri_schemes (vfs);
  g_assert_false (xstrv_contains (schemes, "test"));

  res = xvfs_unregister_uri_scheme (vfs, "test");
  g_assert_false (res);

  res = xvfs_register_uri_scheme (vfs, "test",
                                   test_vfs_lookup, NULL, NULL,
                                   test_vfs_parse_name, NULL, NULL);
  g_assert_true (res);

  schemes = xvfs_get_supported_uri_schemes (vfs);
  g_assert_true (xstrv_contains (schemes, "test"));

  file = xfile_new_for_uri ("test:///foo");
  g_assert_cmpint (GPOINTER_TO_INT (xobject_get_data (G_OBJECT (file), "testfile")), ==, 1);
  xobject_unref (file);

  file = xfile_parse_name ("testfile");
  g_assert_cmpint (GPOINTER_TO_INT (xobject_get_data (G_OBJECT (file), "testfile")), ==, 1);
  xobject_unref (file);

  res = xvfs_register_uri_scheme (vfs, "test",
                                   test_vfs_lookup, NULL, NULL,
                                   test_vfs_parse_name, NULL, NULL);
  g_assert_false (res);

  res = xvfs_unregister_uri_scheme (vfs, "test");
  g_assert_true (res);

  file = xfile_new_for_uri ("test:///foo");
  g_assert_null (xobject_get_data (G_OBJECT (file), "testfile"));
  xobject_unref (file);
}

static void
test_local (void)
{
  xvfs_t *vfs;
  xfile_t *file;
  xchar_t **schemes;

  vfs = xvfs_get_local ();
  xassert (xvfs_is_active (vfs));

  file = xvfs_get_file_for_uri (vfs, "not a good uri");
  xassert (X_IS_FILE (file));
  xobject_unref (file);

  schemes = (xchar_t **)xvfs_get_supported_uri_schemes (vfs);

  xassert (xstrv_length (schemes) > 0);
  g_assert_cmpstr (schemes[0], ==, "file");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gvfs/local", test_local);
  g_test_add_func ("/gvfs/register-scheme", test_register_scheme);

  return g_test_run ();
}
