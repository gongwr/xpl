/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *         David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib.h>
#include "gunixvolume.h"
#include "gunixmount.h"
#include "gunixmounts.h"
#include "gthemedicon.h"
#include "gvolume.h"
#include "gvolumemonitor.h"
#include "gtask.h"
#include "gioerror.h"
#include "glibintl.h"
/* for BUFSIZ */
#include <stdio.h>


struct _GUnixVolume {
  xobject_t parent;

  xvolume_monitor_t *volume_monitor;
  GUnixMount     *mount; /* owned by volume monitor */

  char *device_path;
  char *mount_path;
  xboolean_t can_eject;

  char *identifier;
  char *identifier_type;

  char *name;
  xicon_t *icon;
  xicon_t *symbolic_icon;
};

static void g_unix_volume_volume_iface_init (GVolumeIface *iface);

#define g_unix_volume_get_type _g_unix_volume_get_type
G_DEFINE_TYPE_WITH_CODE (GUnixVolume, g_unix_volume, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_VOLUME,
						g_unix_volume_volume_iface_init))

static void
g_unix_volume_finalize (xobject_t *object)
{
  GUnixVolume *volume;

  volume = G_UNIX_VOLUME (object);

  if (volume->volume_monitor != NULL)
    xobject_unref (volume->volume_monitor);

  if (volume->mount)
    _g_unix_mount_unset_volume (volume->mount, volume);

  xobject_unref (volume->icon);
  xobject_unref (volume->symbolic_icon);
  g_free (volume->name);
  g_free (volume->mount_path);
  g_free (volume->device_path);
  g_free (volume->identifier);
  g_free (volume->identifier_type);

  G_OBJECT_CLASS (g_unix_volume_parent_class)->finalize (object);
}

static void
g_unix_volume_class_init (GUnixVolumeClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_unix_volume_finalize;
}

static void
g_unix_volume_init (GUnixVolume *unix_volume)
{
}

GUnixVolume *
_g_unix_volume_new (xvolume_monitor_t  *volume_monitor,
                    GUnixMountPoint *mountpoint)
{
  GUnixVolume *volume;

  if (!(g_unix_mount_point_is_user_mountable (mountpoint) ||
	xstr_has_prefix (g_unix_mount_point_get_device_path (mountpoint), "/vol/")) ||
      g_unix_mount_point_is_loopback (mountpoint))
    return NULL;

  volume = xobject_new (XTYPE_UNIX_VOLUME, NULL);
  volume->volume_monitor = volume_monitor != NULL ? xobject_ref (volume_monitor) : NULL;
  volume->mount_path = xstrdup (g_unix_mount_point_get_mount_path (mountpoint));
  volume->device_path = xstrdup (g_unix_mount_point_get_device_path (mountpoint));
  volume->can_eject = g_unix_mount_point_guess_can_eject (mountpoint);

  volume->name = g_unix_mount_point_guess_name (mountpoint);
  volume->icon = g_unix_mount_point_guess_icon (mountpoint);
  volume->symbolic_icon = g_unix_mount_point_guess_symbolic_icon (mountpoint);


  if (strcmp (g_unix_mount_point_get_fs_type (mountpoint), "nfs") == 0)
    {
      volume->identifier_type = xstrdup (G_VOLUME_IDENTIFIER_KIND_NFS_MOUNT);
      volume->identifier = xstrdup (volume->device_path);
    }
  else if (xstr_has_prefix (volume->device_path, "LABEL="))
    {
      volume->identifier_type = xstrdup (G_VOLUME_IDENTIFIER_KIND_LABEL);
      volume->identifier = xstrdup (volume->device_path + 6);
    }
  else if (xstr_has_prefix (volume->device_path, "UUID="))
    {
      volume->identifier_type = xstrdup (G_VOLUME_IDENTIFIER_KIND_UUID);
      volume->identifier = xstrdup (volume->device_path + 5);
    }
  else if (g_path_is_absolute (volume->device_path))
    {
      volume->identifier_type = xstrdup (G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
      volume->identifier = xstrdup (volume->device_path);
    }

  return volume;
}

void
_g_unix_volume_disconnected (GUnixVolume *volume)
{
  if (volume->mount)
    {
      _g_unix_mount_unset_volume (volume->mount, volume);
      volume->mount = NULL;
    }
}

void
_g_unix_volume_set_mount (GUnixVolume *volume,
                          GUnixMount  *mount)
{
  if (volume->mount == mount)
    return;

  if (volume->mount)
    _g_unix_mount_unset_volume (volume->mount, volume);

  volume->mount = mount;

  /* TODO: Emit changed in idle to avoid locking issues */
  g_signal_emit_by_name (volume, "changed");
  if (volume->volume_monitor != NULL)
    g_signal_emit_by_name (volume->volume_monitor, "volume-changed", volume);
}

void
_g_unix_volume_unset_mount (GUnixVolume  *volume,
                            GUnixMount *mount)
{
  if (volume->mount == mount)
    {
      volume->mount = NULL;
      /* TODO: Emit changed in idle to avoid locking issues */
      g_signal_emit_by_name (volume, "changed");
      if (volume->volume_monitor != NULL)
        g_signal_emit_by_name (volume->volume_monitor, "volume-changed", volume);
    }
}

static xicon_t *
g_unix_volume_get_icon (xvolume_t *volume)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  return xobject_ref (unix_volume->icon);
}

static xicon_t *
g_unix_volume_get_symbolic_icon (xvolume_t *volume)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  return xobject_ref (unix_volume->symbolic_icon);
}

static char *
g_unix_volume_get_name (xvolume_t *volume)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  return xstrdup (unix_volume->name);
}

static char *
g_unix_volume_get_uuid (xvolume_t *volume)
{
  return NULL;
}

static xboolean_t
g_unix_volume_can_mount (xvolume_t *volume)
{
  return TRUE;
}

static xboolean_t
g_unix_volume_can_eject (xvolume_t *volume)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  return unix_volume->can_eject;
}

static xboolean_t
g_unix_volume_should_automount (xvolume_t *volume)
{
  /* We automount all local volumes because we don't even
   * make the internal stuff visible
   */
  return TRUE;
}

static xdrive_t *
g_unix_volume_get_drive (xvolume_t *volume)
{
  return NULL;
}

static xmount_t *
g_unix_volume_get_mount (xvolume_t *volume)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);

  if (unix_volume->mount != NULL)
    return xobject_ref (G_MOUNT (unix_volume->mount));

  return NULL;
}


xboolean_t
_g_unix_volume_has_mount_path (GUnixVolume *volume,
                               const char  *mount_path)
{
  return strcmp (volume->mount_path, mount_path) == 0;
}

static void
eject_mount_done (xobject_t      *source,
                  xasync_result_t *result,
                  xpointer_t      user_data)
{
  xsubprocess_t *subprocess = G_SUBPROCESS (source);
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xchar_t *stderr_str;
  GUnixVolume *unix_volume;

  if (!xsubprocess_communicate_utf8_finish (subprocess, result, NULL, &stderr_str, &error))
    {
      xtask_return_error (task, error);
      xerror_free (error);
    }
  else /* successful communication */
    {
      if (!xsubprocess_get_successful (subprocess))
        /* ...but bad exit code */
        xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", stderr_str);
      else
        {
          /* ...and successful exit code */
          unix_volume = G_UNIX_VOLUME (xtask_get_source_object (task));
          _g_unix_volume_monitor_update (G_UNIX_VOLUME_MONITOR (unix_volume->volume_monitor));
          xtask_return_boolean (task, TRUE);
        }

      g_free (stderr_str);
    }

  xobject_unref (task);
}

static void
eject_mount_do (xvolume_t              *volume,
                xcancellable_t         *cancellable,
                xasync_ready_callback_t   callback,
                xpointer_t              user_data,
                const xchar_t * const  *argv,
                const xchar_t          *task_name)
{
  xsubprocess_t *subprocess;
  xerror_t *error = NULL;
  xtask_t *task;

  task = xtask_new (volume, cancellable, callback, user_data);
  xtask_set_source_tag (task, eject_mount_do);
  xtask_set_name (task, task_name);

  if (xtask_return_error_if_cancelled (task))
    {
      xobject_unref (task);
      return;
    }

  subprocess = xsubprocess_newv (argv, G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &error);
  g_assert_no_error (error);

  xsubprocess_communicate_utf8_async (subprocess, NULL,
                                       xtask_get_cancellable (task),
                                       eject_mount_done, task);
}

static void
g_unix_volume_mount (xvolume_t            *volume,
                     GMountMountFlags    flags,
                     xmount_operation_t     *mount_operation,
                     xcancellable_t        *cancellable,
                     xasync_ready_callback_t  callback,
                     xpointer_t             user_data)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  const xchar_t *argv[] = { "mount", NULL, NULL };

  if (unix_volume->mount_path != NULL)
    argv[1] = unix_volume->mount_path;
  else
    argv[1] = unix_volume->device_path;

  eject_mount_do (volume, cancellable, callback, user_data, argv, "[gio] mount volume");
}

static xboolean_t
g_unix_volume_mount_finish (xvolume_t        *volume,
                            xasync_result_t  *result,
                            xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, volume), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static void
g_unix_volume_eject (xvolume_t             *volume,
                     xmount_unmount_flags_t   flags,
                     xcancellable_t        *cancellable,
                     xasync_ready_callback_t  callback,
                     xpointer_t             user_data)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  const xchar_t *argv[] = { "eject", NULL, NULL };

  argv[1] = unix_volume->device_path;

  eject_mount_do (volume, cancellable, callback, user_data, argv, "[gio] eject volume");
}

static xboolean_t
g_unix_volume_eject_finish (xvolume_t       *volume,
                            xasync_result_t  *result,
                            xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, volume), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static xchar_t *
g_unix_volume_get_identifier (xvolume_t     *volume,
                              const xchar_t *kind)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);

  if (unix_volume->identifier_type != NULL &&
      strcmp (kind, unix_volume->identifier_type) == 0)
    return xstrdup (unix_volume->identifier);

  return NULL;
}

static xchar_t **
g_unix_volume_enumerate_identifiers (xvolume_t *volume)
{
  GUnixVolume *unix_volume = G_UNIX_VOLUME (volume);
  xchar_t **res;

  if (unix_volume->identifier_type)
    {
      res = g_new (xchar_t *, 2);
      res[0] = xstrdup (unix_volume->identifier_type);
      res[1] = NULL;
    }
  else
    {
      res = g_new (xchar_t *, 1);
      res[0] = NULL;
    }

  return res;
}

static void
g_unix_volume_volume_iface_init (GVolumeIface *iface)
{
  iface->get_name = g_unix_volume_get_name;
  iface->get_icon = g_unix_volume_get_icon;
  iface->get_symbolic_icon = g_unix_volume_get_symbolic_icon;
  iface->get_uuid = g_unix_volume_get_uuid;
  iface->get_drive = g_unix_volume_get_drive;
  iface->get_mount = g_unix_volume_get_mount;
  iface->can_mount = g_unix_volume_can_mount;
  iface->can_eject = g_unix_volume_can_eject;
  iface->should_automount = g_unix_volume_should_automount;
  iface->mount_fn = g_unix_volume_mount;
  iface->mount_finish = g_unix_volume_mount_finish;
  iface->eject = g_unix_volume_eject;
  iface->eject_finish = g_unix_volume_eject_finish;
  iface->get_identifier = g_unix_volume_get_identifier;
  iface->enumerate_identifiers = g_unix_volume_enumerate_identifiers;
}
