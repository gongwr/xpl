/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * licence, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

#ifndef G_OS_UNIX
#error This is a Unix-specific test
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gunixmounts.h>

/* test_t that xfile_trash() returns G_IO_ERROR_NOT_SUPPORTED for files on system mounts. */
static void
test_trash_not_supported (void)
{
  xfile_t *file;
  xfile_io_stream_t *stream;
  GUnixMountEntry *mount;
  xfile_info_t *info;
  xerror_t *error = NULL;
  xboolean_t ret;
  xchar_t *parent_dirname;
  GStatBuf parent_stat, home_stat;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/251");

  /* The test assumes that tmp file is located on system internal mount. */
  file = xfile_new_tmp ("test-trashXXXXXX", &stream, &error);
  parent_dirname = g_path_get_dirname (xfile_peek_path (file));
  g_assert_no_error (error);
  g_assert_cmpint (g_stat (parent_dirname, &parent_stat), ==, 0);
  g_test_message ("File: %s (parent st_dev: %" G_GUINT64_FORMAT ")",
                  xfile_peek_path (file), (xuint64_t) parent_stat.st_dev);

  g_assert_cmpint (g_stat (g_get_home_dir (), &home_stat), ==, 0);
  g_test_message ("Home: %s (st_dev: %" G_GUINT64_FORMAT ")",
                  g_get_home_dir (), (xuint64_t) home_stat.st_dev);

  if (parent_stat.st_dev == home_stat.st_dev)
    {
      g_test_skip ("The file has to be on another filesystem than the home trash to run this test");

      g_free (parent_dirname);
      xobject_unref (stream);
      xobject_unref (file);

      return;
    }

  mount = g_unix_mount_for (xfile_peek_path (file), NULL);
  g_assert_true (mount == NULL || g_unix_mount_is_system_internal (mount));
  g_test_message ("Mount: %s", (mount != NULL) ? g_unix_mount_get_mount_path (mount) : "(null)");
  g_clear_pointer (&mount, g_unix_mount_free);

  /* xfile_trash() shouldn't be supported on system internal mounts,
   * because those are not monitored by gvfsd-trash.
   */
  ret = xfile_trash (file, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_test_message ("Error: %s", error->message);
  g_assert_false (ret);
  g_clear_error (&error);

  info = xfile_query_info (file,
                            XFILE_ATTRIBUTE_ACCESS_CAN_TRASH,
                            XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            NULL,
                            &error);
  g_assert_no_error (error);

  g_assert_false (xfile_info_get_attribute_boolean (info,
                                                     XFILE_ATTRIBUTE_ACCESS_CAN_TRASH));

  g_io_stream_close (XIO_STREAM (stream), NULL, &error);
  g_assert_no_error (error);

  g_free (parent_dirname);
  xobject_unref (info);
  xobject_unref (stream);
  xobject_unref (file);
}

/* test_t that symlinks are properly expaned when looking for topdir (e.g. for trash folder). */
static void
test_trash_symlinks (void)
{
  xfile_t *symlink;
  GUnixMountEntry *target_mount, *tmp_mount, *symlink_mount, *target_over_symlink_mount;
  xchar_t *target, *tmp, *target_over_symlink;
  xerror_t *error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1522");

  target = g_build_filename (g_get_home_dir (), ".local", NULL);

  if (!xfile_test (target, XFILE_TEST_IS_DIR))
    {
      g_test_skip_printf ("Directory '%s' does not exist", target);
      g_free (target);
      return;
    }

  target_mount = g_unix_mount_for (target, NULL);

  if (target_mount == NULL)
    {
      g_test_skip_printf ("Unable to determine mount point for %s", target);
      g_free (target);
      return;
    }

  g_assert_nonnull (target_mount);
  g_test_message ("Target: %s (mount: %s)", target, g_unix_mount_get_mount_path (target_mount));

  tmp = g_dir_make_tmp ("test-trashXXXXXX", &error);
  g_assert_no_error (error);
  g_assert_nonnull (tmp);
  tmp_mount = g_unix_mount_for (tmp, NULL);

  if (tmp_mount == NULL)
    {
      g_test_skip_printf ("Unable to determine mount point for %s", tmp);
      g_unix_mount_free (target_mount);
      g_free (target);
      g_free (tmp);
      return;
    }

  g_assert_nonnull (tmp_mount);
  g_test_message ("Tmp: %s (mount: %s)", tmp, g_unix_mount_get_mount_path (tmp_mount));

  if (g_unix_mount_compare (target_mount, tmp_mount) == 0)
    {
      g_test_skip ("The tmp has to be on another mount than the home to run this test");

      g_unix_mount_free (tmp_mount);
      g_free (tmp);
      g_unix_mount_free (target_mount);
      g_free (target);

      return;
    }

  symlink = xfile_new_build_filename (tmp, "symlink", NULL);
  xfile_make_symbolic_link (symlink, g_get_home_dir (), NULL, &error);
  g_assert_no_error (error);

  symlink_mount = g_unix_mount_for (xfile_peek_path (symlink), NULL);
  g_assert_nonnull (symlink_mount);
  g_test_message ("Symlink: %s (mount: %s)", xfile_peek_path (symlink), g_unix_mount_get_mount_path (symlink_mount));

  g_assert_cmpint (g_unix_mount_compare (symlink_mount, tmp_mount), ==, 0);

  target_over_symlink = g_build_filename (xfile_peek_path (symlink),
                                          ".local",
                                          NULL);
  target_over_symlink_mount = g_unix_mount_for (target_over_symlink, NULL);
  g_assert_nonnull (symlink_mount);
  g_test_message ("Target over symlink: %s (mount: %s)", target_over_symlink, g_unix_mount_get_mount_path (target_over_symlink_mount));

  g_assert_cmpint (g_unix_mount_compare (target_over_symlink_mount, target_mount), ==, 0);

  g_unix_mount_free (target_over_symlink_mount);
  g_unix_mount_free (symlink_mount);
  g_free (target_over_symlink);
  xobject_unref (symlink);
  g_unix_mount_free (tmp_mount);
  g_free (tmp);
  g_unix_mount_free (target_mount);
  g_free (target);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/trash/not-supported", test_trash_not_supported);
  g_test_add_func ("/trash/symlinks", test_trash_symlinks);

  return g_test_run ();
}
