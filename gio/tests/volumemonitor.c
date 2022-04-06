#include <gio/gio.h>

static xvolume_monitor_t *monitor;

static void
do_mount_tests (xdrive_t *drive, xvolume_t *volume, xmount_t *mount)
{
  xdrive_t *d;
  xvolume_t *v;
  xchar_t *name;
  xchar_t *uuid;

  name = g_mount_get_name (mount);
  g_assert (name != NULL);
  g_free (name);

  v = g_mount_get_volume (mount);
  g_assert (v == volume);
  if (v != NULL)
    xobject_unref (v);

  d = g_mount_get_drive (mount);
  g_assert (d == drive);
  if (d != NULL)
    xobject_unref (d);

  uuid = g_mount_get_uuid (mount);
  if (uuid)
    {
      xmount_t *m;
      m = g_volume_monitor_get_mount_for_uuid (monitor, uuid);
      g_assert (m == mount);
      xobject_unref (m);
      g_free (uuid);
    }
}

static void
do_volume_tests (xdrive_t *drive, xvolume_t *volume)
{
  xdrive_t *d;
  xchar_t *name;
  xmount_t *mount;
  xchar_t *uuid;

  name = g_volume_get_name (volume);
  g_assert (name != NULL);
  g_free (name);

  d = g_volume_get_drive (volume);
  g_assert (d == drive);
  if (d != NULL)
    xobject_unref (d);

  mount = g_volume_get_mount (volume);
  if (mount != NULL)
    {
      do_mount_tests (drive, volume, mount);
      xobject_unref (mount);
    }

  uuid = g_volume_get_uuid (volume);
  if (uuid)
    {
      xvolume_t *v;
      v = g_volume_monitor_get_volume_for_uuid (monitor, uuid);
      g_assert (v == volume);
      xobject_unref (v);
      g_free (uuid);
    }
}

static void
do_drive_tests (xdrive_t *drive)
{
  xlist_t *volumes, *l;
  xchar_t *name;
  xboolean_t has_volumes;

  g_assert (X_IS_DRIVE (drive));
  name = xdrive_get_name (drive);
  g_assert (name != NULL);
  g_free (name);

  has_volumes = xdrive_has_volumes (drive);
  volumes = xdrive_get_volumes (drive);
  g_assert (has_volumes == (volumes != NULL));
  for (l = volumes; l; l = l->next)
    {
      xvolume_t *volume = l->data;
      do_volume_tests (drive, volume);
    }

  xlist_free_full (volumes, xobject_unref);
}

static void
test_connected_drives (void)
{
  xlist_t *drives;
  xlist_t *l;

  drives = g_volume_monitor_get_connected_drives (monitor);

  for (l = drives; l; l = l->next)
    {
      xdrive_t *drive = l->data;
      do_drive_tests (drive);
    }

  xlist_free_full (drives, xobject_unref);
}

static void
test_volumes (void)
{
  xlist_t *volumes, *l;

  volumes = g_volume_monitor_get_volumes (monitor);

  for (l = volumes; l; l = l->next)
    {
      xvolume_t *volume = l->data;
      xdrive_t *drive;

      drive = g_volume_get_drive (volume);
      do_volume_tests (drive, volume);
      if (drive != NULL)
        xobject_unref (drive);
    }

  xlist_free_full (volumes, xobject_unref);
}

static void
test_mounts (void)
{
  xlist_t *mounts, *l;

  mounts = g_volume_monitor_get_mounts (monitor);

  for (l = mounts; l; l = l->next)
    {
      xmount_t *mount = l->data;
      xvolume_t *volume;
      xdrive_t *drive;

      drive = g_mount_get_drive (mount);
      volume = g_mount_get_volume (mount);
      do_mount_tests (drive, volume, mount);

      if (drive != NULL)
        xobject_unref (drive);
      if (volume != NULL)
        xobject_unref (volume);
    }

  xlist_free_full (mounts, xobject_unref);
}
int
main (int argc, char *argv[])
{
  xboolean_t ret;

  g_setenv ("GIO_USE_VFS", "local", FALSE);

  g_test_init (&argc, &argv, NULL);

  monitor = g_volume_monitor_get ();

  g_test_add_func ("/volumemonitor/connected_drives", test_connected_drives);
  g_test_add_func ("/volumemonitor/volumes", test_volumes);
  g_test_add_func ("/volumemonitor/mounts", test_mounts);

  ret = g_test_run ();

  xobject_unref (monitor);

  return ret;
}
