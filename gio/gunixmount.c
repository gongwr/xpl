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
#include "gsubprocess.h"
#include "gioenums.h"
#include "gunixvolumemonitor.h"
#include "gunixmount.h"
#include "gunixmounts.h"
#include "gunixvolume.h"
#include "gmountprivate.h"
#include "gmount.h"
#include "gfile.h"
#include "gvolumemonitor.h"
#include "gthemedicon.h"
#include "gioerror.h"
#include "glibintl.h"
/* for BUFSIZ */
#include <stdio.h>


struct _GUnixMount {
  xobject_t parent;

  xvolume_monitor_t   *volume_monitor;

  GUnixVolume      *volume; /* owned by volume monitor */

  char *name;
  xicon_t *icon;
  xicon_t *symbolic_icon;
  char *device_path;
  char *mount_path;

  xboolean_t can_eject;
};

static void g_unix_mount_mount_iface_init (GMountIface *iface);

#define g_unix_mount_get_type _g_unix_mount_get_type
G_DEFINE_TYPE_WITH_CODE (GUnixMount, g_unix_mount, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_MOUNT,
						g_unix_mount_mount_iface_init))


static void
g_unix_mount_finalize (xobject_t *object)
{
  GUnixMount *mount;

  mount = G_UNIX_MOUNT (object);

  if (mount->volume_monitor != NULL)
    xobject_unref (mount->volume_monitor);

  if (mount->volume)
    _g_unix_volume_unset_mount (mount->volume, mount);

  /* TODO: g_warn_if_fail (volume->volume == NULL); */
  xobject_unref (mount->icon);
  xobject_unref (mount->symbolic_icon);
  g_free (mount->name);
  g_free (mount->device_path);
  g_free (mount->mount_path);

  G_OBJECT_CLASS (g_unix_mount_parent_class)->finalize (object);
}

static void
g_unix_mount_class_init (GUnixMountClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_unix_mount_finalize;
}

static void
g_unix_mount_init (GUnixMount *unix_mount)
{
}

GUnixMount *
_g_unix_mount_new (xvolume_monitor_t  *volume_monitor,
                   GUnixMountEntry *mount_entry,
                   GUnixVolume     *volume)
{
  GUnixMount *mount;

  /* No volume for mount: Ignore internal things */
  if (volume == NULL && !g_unix_mount_guess_should_display (mount_entry))
    return NULL;

  mount = xobject_new (XTYPE_UNIX_MOUNT, NULL);
  mount->volume_monitor = volume_monitor != NULL ? xobject_ref (volume_monitor) : NULL;
  mount->device_path = xstrdup (g_unix_mount_get_device_path (mount_entry));
  mount->mount_path = xstrdup (g_unix_mount_get_mount_path (mount_entry));
  mount->can_eject = g_unix_mount_guess_can_eject (mount_entry);

  mount->name = g_unix_mount_guess_name (mount_entry);
  mount->icon = g_unix_mount_guess_icon (mount_entry);
  mount->symbolic_icon = g_unix_mount_guess_symbolic_icon (mount_entry);

  /* need to do this last */
  mount->volume = volume;
  if (volume != NULL)
    _g_unix_volume_set_mount (volume, mount);

  return mount;
}

void
_g_unix_mount_unmounted (GUnixMount *mount)
{
  if (mount->volume != NULL)
    {
      _g_unix_volume_unset_mount (mount->volume, mount);
      mount->volume = NULL;
      xsignal_emit_by_name (mount, "changed");
      /* there's really no need to emit mount_changed on the volume monitor
       * as we're going to be deleted.. */
    }
}

void
_g_unix_mount_unset_volume (GUnixMount *mount,
                            GUnixVolume  *volume)
{
  if (mount->volume == volume)
    {
      mount->volume = NULL;
      /* TODO: Emit changed in idle to avoid locking issues */
      xsignal_emit_by_name (mount, "changed");
      if (mount->volume_monitor != NULL)
        xsignal_emit_by_name (mount->volume_monitor, "mount-changed", mount);
    }
}

static xfile_t *
g_unix_mount_get_root (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);

  return xfile_new_for_path (unix_mount->mount_path);
}

static xicon_t *
g_unix_mount_get_icon (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);

  return xobject_ref (unix_mount->icon);
}

static xicon_t *
g_unix_mount_get_symbolic_icon (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);

  return xobject_ref (unix_mount->symbolic_icon);
}

static char *
g_unix_mount_get_uuid (xmount_t *mount)
{
  return NULL;
}

static char *
g_unix_mount_get_name (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);

  return xstrdup (unix_mount->name);
}

xboolean_t
_g_unix_mount_has_mount_path (GUnixMount *mount,
                              const char  *mount_path)
{
  return strcmp (mount->mount_path, mount_path) == 0;
}

static xdrive_t *
g_unix_mount_get_drive (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);

  if (unix_mount->volume != NULL)
    return g_volume_get_drive (G_VOLUME (unix_mount->volume));

  return NULL;
}

static xvolume_t *
g_unix_mount_get_volume (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);

  if (unix_mount->volume)
    return G_VOLUME (xobject_ref (unix_mount->volume));

  return NULL;
}

static xboolean_t
g_unix_mount_can_unmount (xmount_t *mount)
{
  return TRUE;
}

static xboolean_t
g_unix_mount_can_eject (xmount_t *mount)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);
  return unix_mount->can_eject;
}

static void
eject_unmount_done (xobject_t      *source,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  xsubprocess_t *subprocess = G_SUBPROCESS (source);
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xchar_t *stderr_str;

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
        /* ...and successful exit code */
        xtask_return_boolean (task, TRUE);

      g_free (stderr_str);
    }

  xobject_unref (task);
}

static xboolean_t
eject_unmount_do_cb (xpointer_t user_data)
{
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xsubprocess_t *subprocess;
  const xchar_t **argv;

  argv = xtask_get_task_data (task);

  if (xtask_return_error_if_cancelled (task))
    {
      xobject_unref (task);
      return G_SOURCE_REMOVE;
    }

  subprocess = xsubprocess_newv (argv, G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &error);
  g_assert_no_error (error);

  xsubprocess_communicate_utf8_async (subprocess, NULL,
                                       xtask_get_cancellable (task),
                                       eject_unmount_done, task);

  return G_SOURCE_REMOVE;
}

static void
eject_unmount_do (xmount_t              *mount,
                  xcancellable_t        *cancellable,
                  xasync_ready_callback_t  callback,
                  xpointer_t             user_data,
                  char               **argv,
                  const xchar_t         *task_name)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);
  xtask_t *task;
  xsource_t *timeout;

  task = xtask_new (mount, cancellable, callback, user_data);
  xtask_set_source_tag (task, eject_unmount_do);
  xtask_set_name (task, task_name);
  xtask_set_task_data (task, xstrdupv (argv), (xdestroy_notify_t) xstrfreev);

  if (unix_mount->volume_monitor != NULL)
    xsignal_emit_by_name (unix_mount->volume_monitor, "mount-pre-unmount", mount);

  xsignal_emit_by_name (mount, "pre-unmount", 0);

  timeout = g_timeout_source_new (500);
  xtask_attach_source (task, timeout, (xsource_func_t) eject_unmount_do_cb);
  xsource_unref (timeout);
}

static void
g_unix_mount_unmount (xmount_t             *mount,
                      xmount_unmount_flags_t flags,
                      xcancellable_t        *cancellable,
                      xasync_ready_callback_t  callback,
                      xpointer_t             user_data)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);
  char *argv[] = {"umount", NULL, NULL};

  if (unix_mount->mount_path != NULL)
    argv[1] = unix_mount->mount_path;
  else
    argv[1] = unix_mount->device_path;

  eject_unmount_do (mount, cancellable, callback, user_data, argv, "[gio] unmount mount");
}

static xboolean_t
g_unix_mount_unmount_finish (xmount_t       *mount,
                             xasync_result_t  *result,
                             xerror_t       **error)
{
  return xtask_propagate_boolean (XTASK (result), error);
}

static void
g_unix_mount_eject (xmount_t             *mount,
                    xmount_unmount_flags_t flags,
                    xcancellable_t        *cancellable,
                    xasync_ready_callback_t  callback,
                    xpointer_t             user_data)
{
  GUnixMount *unix_mount = G_UNIX_MOUNT (mount);
  char *argv[] = {"eject", NULL, NULL};

  if (unix_mount->mount_path != NULL)
    argv[1] = unix_mount->mount_path;
  else
    argv[1] = unix_mount->device_path;

  eject_unmount_do (mount, cancellable, callback, user_data, argv, "[gio] eject mount");
}

static xboolean_t
g_unix_mount_eject_finish (xmount_t       *mount,
                           xasync_result_t  *result,
                           xerror_t       **error)
{
  return xtask_propagate_boolean (XTASK (result), error);
}

static void
g_unix_mount_mount_iface_init (GMountIface *iface)
{
  iface->get_root = g_unix_mount_get_root;
  iface->get_name = g_unix_mount_get_name;
  iface->get_icon = g_unix_mount_get_icon;
  iface->get_symbolic_icon = g_unix_mount_get_symbolic_icon;
  iface->get_uuid = g_unix_mount_get_uuid;
  iface->get_drive = g_unix_mount_get_drive;
  iface->get_volume = g_unix_mount_get_volume;
  iface->can_unmount = g_unix_mount_can_unmount;
  iface->can_eject = g_unix_mount_can_eject;
  iface->unmount = g_unix_mount_unmount;
  iface->unmount_finish = g_unix_mount_unmount_finish;
  iface->eject = g_unix_mount_eject;
  iface->eject_finish = g_unix_mount_eject_finish;
}
