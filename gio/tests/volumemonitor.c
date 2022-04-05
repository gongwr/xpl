#include <gio/gio.h>

static GVolumeMonitor *monitor;

static void
do_mount_tests (xdrive_t *drive, GVolume *volume, GMount *mount)
{
  xdrive_t *d;
  GVolume *v;
  xchar_t *name;
  xchar_t *uuid;

  name = g_mount_get_name (mount);
  g_assert (name != NULL);
  g_free (name);

  v = g_mount_get_volume (mount);
  g_assert (v == volume);
  if (v != NULL)
    g_object_unref (v);

  d = g_mount_get_drive (mount);
  g_assert (d == drive);
  if (d != NULL)
    g_object_unref (d);

  uuid = g_mount_get_uuid (mount);
  if (uuid)
    {
      GMount *m;
      m = g_volume_monitor_get_mount_for_uuid (monitor, uuid);
      g_assert (m == mount);
      g_object_unref (m);
      g_free (uuid);
    }
}

static void
do_volume_tests (xdrive_t *drive, GVolume *volume)
{
  xdrive_t *d;
  xchar_t *name;
  GMount *mount;
  xchar_t *uuid;

  name = g_volume_get_name (volume);
  g_assert (name != NULL);
  g_free (name);

  d = g_volume_get_drive (volume);
  g_assert (d == drive);
  if (d != NULL)
    g_object_unref (d);

  mount = g_volume_get_mount (volume);
  if (mount != NULL)
    {
      do_mount_tests (drive, volume, mount);
      g_object_unref (mount);
    }

  uuid = g_volume_get_uuid (volume);
  if (uuid)
    {
      GVolume *v;
      v = g_volume_monitor_get_volume_for_uuid (monitor, uuid);
      g_assert (v == volume);
      g_object_unref (v);
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
      GVolume *volume = l->data;
      do_volume_tests (drive, volume);
    }

  g_list_free_full (volumes, g_object_unref);
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

  g_list_free_full (drives, g_object_unref);
}

static void
test_volumes (void)
{
  xlist_t *volumes, *l;

  volumes = g_volume_monitor_get_volumes (monitor);

  for (l = volumes; l; l = l->next)
    {
      GVolume *volume = l->data;
      xdrive_t *drive;

      drive = g_volume_get_drive (volume);
      do_volume_tests (drive, volume);
      if (drive != NULL)
        g_object_unref (drive);
    }

  g_list_free_full (volumes, g_object_unref);
}

static void
test_mounts (void)
{
  xlist_t *mounts, *l;

  mounts = g_volume_monitor_get_mounts (monitor);

  for (l = mounts; l; l = l->next)
    {
      GMount *mount = l->data;
      GVolume *volume;
      xdrive_t *drive;

      drive = g_mount_get_drive (mount);
      volume = g_mount_get_volume (mount);
      do_mount_tests (drive, volume, mount);

      if (drive != NULL)
        g_object_unref (drive);
      if (volume != NULL)
        g_object_unref (volume);
    }

  g_list_free_full (mounts, g_object_unref);
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

  g_object_unref (monitor);

  return ret;
}
