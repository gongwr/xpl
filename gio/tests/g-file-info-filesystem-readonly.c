/* Testcase for bug in GIO function xfile_query_filesystem_info()
 * Author: Nelson Benítez León
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

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gunixmounts.h>

static xboolean_t
run (xerror_t **error,
     const xchar_t *argv0,
     ...)
{
  xptr_array_t *args;
  const xchar_t *arg;
  va_list ap;
  xsubprocess_t *subprocess;
  xchar_t *command_line = NULL;
  xboolean_t success;

  args = xptr_array_new ();

  va_start (ap, argv0);
  xptr_array_add (args, (xchar_t *) argv0);
  while ((arg = va_arg (ap, const xchar_t *)))
    xptr_array_add (args, (xchar_t *) arg);
  xptr_array_add (args, NULL);
  va_end (ap);

  command_line = xstrjoinv (" ", (xchar_t **) args->pdata);
  g_test_message ("Running command `%s`", command_line);
  g_free (command_line);

  subprocess = xsubprocess_newv ((const xchar_t * const *) args->pdata, G_SUBPROCESS_FLAGS_NONE, error);
  xptr_array_free (args, TRUE);

  if (subprocess == NULL)
    return FALSE;

  success = xsubprocess_wait_check (subprocess, NULL, error);
  xobject_unref (subprocess);

  return success;
}

static void
assert_remove (const xchar_t *file)
{
  if (g_remove (file) != 0)
    xerror ("failed to remove %s: %s", file, xstrerror (errno));
}

static xboolean_t
fuse_module_loaded (void)
{
  char *contents = NULL;
  xboolean_t ret;

  if (!xfile_get_contents ("/proc/modules", &contents, NULL, NULL) ||
      contents == NULL)
    {
      g_free (contents);
      return FALSE;
    }

  ret = (strstr (contents, "\nfuse ") != NULL);
  g_free (contents);
  return ret;
}

static void
test_filesystem_readonly (xconstpointer with_mount_monitor)
{
  xfile_info_t *file_info;
  xfile_t *mounted_file;
  GUnixMountMonitor *mount_monitor = NULL;
  xchar_t *bindfs, *fusermount;
  xchar_t *curdir, *dir_to_mount, *dir_mountpoint;
  xchar_t *file_in_mount, *file_in_mountpoint;
  xerror_t *error = NULL;

  /* installed by package 'bindfs' in Fedora */
  bindfs = g_find_program_in_path ("bindfs");

  /* installed by package 'fuse' in Fedora */
  fusermount = g_find_program_in_path ("fusermount");

  if (bindfs == NULL || fusermount == NULL)
    {
      /* We need these because "mount --bind" requires root privileges */
      g_test_skip ("'bindfs' and 'fusermount' commands are needed to run this test");
      g_free (fusermount);
      g_free (bindfs);
      return;
    }

  /* If the fuse module is loaded but there's no /dev/fuse, then we're
   * we're probably in a rootless container and won't be able to
   * use bindfs to run our tests */
  if (fuse_module_loaded () &&
      !xfile_test ("/dev/fuse", XFILE_TEST_EXISTS))
    {
      g_test_skip ("fuse support is needed to run this test (rootless container?)");
      g_free (fusermount);
      g_free (bindfs);
      return;
    }

  curdir = g_get_current_dir ();
  dir_to_mount = xstrdup_printf ("%s/dir_bindfs_to_mount", curdir);
  file_in_mount = xstrdup_printf ("%s/example.txt", dir_to_mount);
  dir_mountpoint = xstrdup_printf ("%s/dir_bindfs_mountpoint", curdir);

  g_mkdir (dir_to_mount, 0777);
  g_mkdir (dir_mountpoint, 0777);
  if (! xfile_set_contents (file_in_mount, "Example", -1, NULL))
    {
      g_test_skip ("Failed to create file needed to proceed further with the test");

      g_free (dir_mountpoint);
      g_free (file_in_mount);
      g_free (dir_to_mount);
      g_free (curdir);
      g_free (fusermount);
      g_free (bindfs);
      return;
    }

  if (with_mount_monitor)
    mount_monitor = g_unix_mount_monitor_get ();

  /* Use bindfs, which does not need root privileges, to mount the contents of one dir
   * into another dir (and do the mount as readonly as per passed '-o ro' option) */
  if (!run (&error, bindfs, "-n", "-o", "ro", dir_to_mount, dir_mountpoint, NULL))
    {
      xchar_t *skip_message = xstrdup_printf ("Failed to run bindfs to set up test: %s", error->message);
      g_test_skip (skip_message);

      g_free (skip_message);
      g_clear_error (&error);

      g_clear_object (&mount_monitor);
      g_free (dir_mountpoint);
      g_free (file_in_mount);
      g_free (dir_to_mount);
      g_free (curdir);
      g_free (fusermount);
      g_free (bindfs);

      return;
    }

  /* Let's check now, that the file is in indeed in a readonly filesystem */
  file_in_mountpoint = xstrdup_printf ("%s/example.txt", dir_mountpoint);
  mounted_file = xfile_new_for_path (file_in_mountpoint);

  if (with_mount_monitor)
    {
     /* Let UnixMountMonitor process its 'mounts-changed'
      * signal triggered by mount operation above */
      while (xmain_context_iteration (NULL, FALSE));
    }

  file_info = xfile_query_filesystem_info (mounted_file,
                                            XFILE_ATTRIBUTE_FILESYSTEM_READONLY, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (file_info);
  if (! xfile_info_get_attribute_boolean (file_info, XFILE_ATTRIBUTE_FILESYSTEM_READONLY))
    {
      g_test_skip ("Failed to create readonly file needed to proceed further with the test");

      g_clear_object (&file_info);
      g_clear_object (&mounted_file);
      g_free (file_in_mountpoint);
      g_clear_object (&mount_monitor);
      g_free (dir_mountpoint);
      g_free (file_in_mount);
      g_free (dir_to_mount);
      g_free (curdir);
      g_free (fusermount);
      g_free (bindfs);

      return;
    }

  /* Now we unmount, and mount again but this time rw (not readonly) */
  run (&error, fusermount, "-z", "-u", dir_mountpoint, NULL);
  g_assert_no_error (error);
  run (&error, bindfs, "-n", dir_to_mount, dir_mountpoint, NULL);
  g_assert_no_error (error);

  if (with_mount_monitor)
    {
     /* Let UnixMountMonitor process its 'mounts-changed' signal
      * triggered by mount/umount operations above */
      while (xmain_context_iteration (NULL, FALSE));
    }

  /* Now let's test if GIO will report the new filesystem state */
  g_clear_object (&file_info);
  g_clear_object (&mounted_file);
  mounted_file = xfile_new_for_path (file_in_mountpoint);
  file_info = xfile_query_filesystem_info (mounted_file,
                                            XFILE_ATTRIBUTE_FILESYSTEM_READONLY, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (file_info);

  g_assert_false (xfile_info_get_attribute_boolean (file_info, XFILE_ATTRIBUTE_FILESYSTEM_READONLY));

  /* Clean up */
  g_clear_object (&mount_monitor);
  g_clear_object (&file_info);
  g_clear_object (&mounted_file);
  run (&error, fusermount, "-z", "-u", dir_mountpoint, NULL);
  g_assert_no_error (error);

  assert_remove (file_in_mount);
  assert_remove (dir_to_mount);
  assert_remove (dir_mountpoint);

  g_free (bindfs);
  g_free (fusermount);
  g_free (curdir);
  g_free (dir_to_mount);
  g_free (dir_mountpoint);
  g_free (file_in_mount);
  g_free (file_in_mountpoint);
}

int
main (int argc, char *argv[])
{
  /* To avoid unnecessary D-Bus calls, see http://goo.gl/ir56j2 */
  g_setenv ("GIO_USE_VFS", "local", FALSE);

  g_test_init (&argc, &argv, NULL);

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=787731");

  g_test_add_data_func ("/g-file-info-filesystem-readonly/test-fs-ro",
                        GINT_TO_POINTER (FALSE), test_filesystem_readonly);

  /* This second test is using a running GUnixMountMonitor, so the calls to:
   *  g_unix_mount_get(&time_read) - To fill the time_read parameter
   *  g_unix_mounts_changed_since()
   *
   * made from inside xfile_query_filesystem_info() will use the mount_poller_time
   * from the monitoring of /proc/self/mountinfo , while in the previous test new
   * created timestamps are returned from those g_unix_mount* functions. */
  g_test_add_data_func ("/g-file-info-filesystem-readonly/test-fs-ro-with-mount-monitor",
                        GINT_TO_POINTER (TRUE), test_filesystem_readonly);

  return g_test_run ();
}
