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

#include <glib.h>
#include "gunionvolumemonitor.h"
#include "gmountprivate.h"
#include "giomodule-priv.h"
#ifdef G_OS_UNIX
#include "gunixvolumemonitor.h"
#endif
#include "gnativevolumemonitor.h"

#include "glibintl.h"


struct _xunion_volume_monitor {
  xvolume_monitor_t parent;

  xlist_t *monitors;
};

static void xunion_volume_monitor_remove_monitor (xunion_volume_monitor_t *union_monitor,
						   xvolume_monitor_t *child_monitor);


#define xunion_volume_monitor_get_type _xunion_volume_monitor_get_type
XDEFINE_TYPE (xunion_volume_monitor, xunion_volume_monitor, XTYPE_VOLUME_MONITOR)

static GRecMutex the_volume_monitor_mutex;

static xunion_volume_monitor_t *the_volume_monitor = NULL;

static void
xunion_volume_monitor_finalize (xobject_t *object)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;

  monitor = XUNION_VOLUME_MONITOR (object);

  while (monitor->monitors != NULL)
    {
      child_monitor = monitor->monitors->data;
      xunion_volume_monitor_remove_monitor (monitor,
                                             child_monitor);
      xobject_unref (child_monitor);
    }

  XOBJECT_CLASS (xunion_volume_monitor_parent_class)->finalize (object);
}

static void
xunion_volume_monitor_dispose (xobject_t *object)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;
  xlist_t *l;

  monitor = XUNION_VOLUME_MONITOR (object);

  g_rec_mutex_lock (&the_volume_monitor_mutex);
  the_volume_monitor = NULL;

  for (l = monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;
      xobject_run_dispose (G_OBJECT (child_monitor));
    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  XOBJECT_CLASS (xunion_volume_monitor_parent_class)->dispose (object);
}

static xlist_t *
get_mounts (xvolume_monitor_t *volume_monitor)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;
  xlist_t *res;
  xlist_t *l;

  monitor = XUNION_VOLUME_MONITOR (volume_monitor);

  res = NULL;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  for (l = monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;

      res = xlist_concat (res, g_volume_monitor_get_mounts (child_monitor));
    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return res;
}

static xlist_t *
get_volumes (xvolume_monitor_t *volume_monitor)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;
  xlist_t *res;
  xlist_t *l;

  monitor = XUNION_VOLUME_MONITOR (volume_monitor);

  res = NULL;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  for (l = monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;

      res = xlist_concat (res, g_volume_monitor_get_volumes (child_monitor));
    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return res;
}

static xlist_t *
get_connected_drives (xvolume_monitor_t *volume_monitor)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;
  xlist_t *res;
  xlist_t *l;

  monitor = XUNION_VOLUME_MONITOR (volume_monitor);

  res = NULL;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  for (l = monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;

      res = xlist_concat (res, g_volume_monitor_get_connected_drives (child_monitor));
    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return res;
}

static xvolume_t *
get_volume_for_uuid (xvolume_monitor_t *volume_monitor, const char *uuid)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;
  xvolume_t *volume;
  xlist_t *l;

  monitor = XUNION_VOLUME_MONITOR (volume_monitor);

  volume = NULL;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  for (l = monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;

      volume = g_volume_monitor_get_volume_for_uuid (child_monitor, uuid);
      if (volume != NULL)
        break;

    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return volume;
}

static xmount_t *
get_mount_for_uuid (xvolume_monitor_t *volume_monitor, const char *uuid)
{
  xunion_volume_monitor_t *monitor;
  xvolume_monitor_t *child_monitor;
  xmount_t *mount;
  xlist_t *l;

  monitor = XUNION_VOLUME_MONITOR (volume_monitor);

  mount = NULL;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  for (l = monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;

      mount = g_volume_monitor_get_mount_for_uuid (child_monitor, uuid);
      if (mount != NULL)
        break;

    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return mount;
}

static void
xunion_volume_monitor_class_init (GUnionVolumeMonitorClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
  GVolumeMonitorClass *monitor_class = G_VOLUME_MONITOR_CLASS (klass);

  xobject_class->finalize = xunion_volume_monitor_finalize;
  xobject_class->dispose = xunion_volume_monitor_dispose;

  monitor_class->get_connected_drives = get_connected_drives;
  monitor_class->get_volumes = get_volumes;
  monitor_class->get_mounts = get_mounts;
  monitor_class->get_volume_for_uuid = get_volume_for_uuid;
  monitor_class->get_mount_for_uuid = get_mount_for_uuid;
}

static void
child_volume_added (xvolume_monitor_t      *child_monitor,
                    xvolume_t    *child_volume,
                    xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "volume-added",
			 child_volume);
}

static void
child_volume_removed (xvolume_monitor_t      *child_monitor,
                      xvolume_t    *child_volume,
                      xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "volume-removed",
			 child_volume);
}

static void
child_volume_changed (xvolume_monitor_t      *child_monitor,
                      xvolume_t    *child_volume,
                      xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "volume-changed",
			 child_volume);
}

static void
child_mount_added (xvolume_monitor_t      *child_monitor,
                   xmount_t              *child_mount,
                   xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
                         "mount-added",
                         child_mount);
}

static void
child_mount_removed (xvolume_monitor_t      *child_monitor,
                     xmount_t              *child_mount,
                     xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "mount-removed",
			 child_mount);
}

static void
child_mount_pre_unmount (xvolume_monitor_t       *child_monitor,
                          xmount_t              *child_mount,
                          xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "mount-pre-unmount",
			 child_mount);
}


static void
child_mount_changed (xvolume_monitor_t       *child_monitor,
                      xmount_t              *child_mount,
                      xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "mount-changed",
			 child_mount);
}

static void
child_drive_connected (xvolume_monitor_t      *child_monitor,
                       xdrive_t              *child_drive,
                       xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "drive-connected",
			 child_drive);
}

static void
child_drive_disconnected (xvolume_monitor_t      *child_monitor,
                          xdrive_t              *child_drive,
                          xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
			 "drive-disconnected",
			 child_drive);
}

static void
child_drive_changed (xvolume_monitor_t      *child_monitor,
                     xdrive_t             *child_drive,
                     xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
                         "drive-changed",
                         child_drive);
}

static void
child_drive_eject_button (xvolume_monitor_t      *child_monitor,
                          xdrive_t             *child_drive,
                          xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
                         "drive-eject-button",
                         child_drive);
}

static void
child_drive_stop_button (xvolume_monitor_t      *child_monitor,
                         xdrive_t             *child_drive,
                         xunion_volume_monitor_t *union_monitor)
{
  xsignal_emit_by_name (union_monitor,
                         "drive-stop-button",
                         child_drive);
}

static void
xunion_volume_monitor_add_monitor (xunion_volume_monitor_t *union_monitor,
                                    xvolume_monitor_t      *volume_monitor)
{
  if (xlist_find (union_monitor->monitors, volume_monitor))
    return;

  union_monitor->monitors =
    xlist_prepend (union_monitor->monitors,
		    xobject_ref (volume_monitor));

  xsignal_connect (volume_monitor, "volume-added", (xcallback_t)child_volume_added, union_monitor);
  xsignal_connect (volume_monitor, "volume-removed", (xcallback_t)child_volume_removed, union_monitor);
  xsignal_connect (volume_monitor, "volume-changed", (xcallback_t)child_volume_changed, union_monitor);
  xsignal_connect (volume_monitor, "mount-added", (xcallback_t)child_mount_added, union_monitor);
  xsignal_connect (volume_monitor, "mount-removed", (xcallback_t)child_mount_removed, union_monitor);
  xsignal_connect (volume_monitor, "mount-pre-unmount", (xcallback_t)child_mount_pre_unmount, union_monitor);
  xsignal_connect (volume_monitor, "mount-changed", (xcallback_t)child_mount_changed, union_monitor);
  xsignal_connect (volume_monitor, "drive-connected", (xcallback_t)child_drive_connected, union_monitor);
  xsignal_connect (volume_monitor, "drive-disconnected", (xcallback_t)child_drive_disconnected, union_monitor);
  xsignal_connect (volume_monitor, "drive-changed", (xcallback_t)child_drive_changed, union_monitor);
  xsignal_connect (volume_monitor, "drive-eject-button", (xcallback_t)child_drive_eject_button, union_monitor);
  xsignal_connect (volume_monitor, "drive-stop-button", (xcallback_t)child_drive_stop_button, union_monitor);
}

static void
xunion_volume_monitor_remove_monitor (xunion_volume_monitor_t *union_monitor,
                                       xvolume_monitor_t      *child_monitor)
{
  xlist_t *l;

  l = xlist_find (union_monitor->monitors, child_monitor);
  if (l == NULL)
    return;

  union_monitor->monitors = xlist_delete_link (union_monitor->monitors, l);

  xsignal_handlers_disconnect_by_func (child_monitor, child_volume_added, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_volume_removed, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_volume_changed, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_mount_added, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_mount_removed, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_mount_pre_unmount, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_mount_changed, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_drive_connected, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_drive_disconnected, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_drive_changed, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_drive_eject_button, union_monitor);
  xsignal_handlers_disconnect_by_func (child_monitor, child_drive_stop_button, union_monitor);
}

static xtype_t
get_default_native_class (xpointer_t data)
{
  return _xio_module_get_default_type (G_NATIVE_VOLUME_MONITOR_EXTENSION_POINT_NAME,
                                        "GIO_USE_VOLUME_MONITOR",
                                        G_STRUCT_OFFSET (GVolumeMonitorClass, is_supported));
}

/* We return the class, with a ref taken.
 * This way we avoid unloading the class/module
 * between selecting the type and creating the
 * instance on the first call.
 */
static GNativeVolumeMonitorClass *
get_native_class (void)
{
  static GOnce once_init = G_ONCE_INIT;
  xtype_class_t *type_class;

  type_class = NULL;
  g_once (&once_init, (GThreadFunc)get_default_native_class, &type_class);

  if (type_class == NULL && once_init.retval != GUINT_TO_POINTER(XTYPE_INVALID))
    type_class = xtype_class_ref ((xtype_t)once_init.retval);

  return (GNativeVolumeMonitorClass *)type_class;
}

static void
xunion_volume_monitor_init (xunion_volume_monitor_t *union_monitor)
{
}

static void
populate_union_monitor (xunion_volume_monitor_t *union_monitor)
{
  xvolume_monitor_t *monitor;
  GNativeVolumeMonitorClass *native_class;
  GVolumeMonitorClass *klass;
  xio_extension_point_t *ep;
  xio_extension_t *extension;
  xlist_t *l;

  native_class = get_native_class ();

  if (native_class != NULL)
    {
      monitor = xobject_new (XTYPE_FROM_CLASS (native_class), NULL);
      xunion_volume_monitor_add_monitor (union_monitor, monitor);
      xobject_unref (monitor);
      xtype_class_unref (native_class);
    }

  ep = g_io_extension_point_lookup (G_VOLUME_MONITOR_EXTENSION_POINT_NAME);
  for (l = g_io_extension_point_get_extensions (ep); l != NULL; l = l->next)
    {
      extension = l->data;

      klass = G_VOLUME_MONITOR_CLASS (g_io_extension_ref_class (extension));
      if (klass->is_supported == NULL || klass->is_supported())
	{
	  monitor = xobject_new (g_io_extension_get_type (extension), NULL);
	  xunion_volume_monitor_add_monitor (union_monitor, monitor);
	  xobject_unref (monitor);
	}
      xtype_class_unref (klass);
    }
}

static xunion_volume_monitor_t *
xunion_volume_monitor_new (void)
{
  xunion_volume_monitor_t *monitor;

  monitor = xobject_new (XTYPE_UNION_VOLUME_MONITOR, NULL);

  return monitor;
}

/**
 * g_volume_monitor_get:
 *
 * Gets the volume monitor used by gio.
 *
 * Returns: (transfer full): a reference to the #xvolume_monitor_t used by gio. Call
 *    xobject_unref() when done with it.
 **/
xvolume_monitor_t *
g_volume_monitor_get (void)
{
  xvolume_monitor_t *vm;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  if (the_volume_monitor)
    vm = G_VOLUME_MONITOR (xobject_ref (the_volume_monitor));
  else
    {
      the_volume_monitor = xunion_volume_monitor_new ();
      populate_union_monitor (the_volume_monitor);
      vm = G_VOLUME_MONITOR (the_volume_monitor);
    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return vm;
}

xmount_t *
_g_mount_get_for_mount_path (const xchar_t  *mount_path,
			     xcancellable_t *cancellable)
{
  GNativeVolumeMonitorClass *klass;
  xmount_t *mount;

  klass = get_native_class ();
  if (klass == NULL)
    return NULL;

  mount = NULL;

  if (klass->get_mount_for_mount_path)
    mount = klass->get_mount_for_mount_path (mount_path, cancellable);

  /* TODO: How do we know this succeeded? Keep in mind that the native
   *       volume monitor may fail (e.g. not being able to connect to
   *       udisks). Is the get_mount_for_mount_path() method allowed to
   *       return NULL? Seems like it is ... probably the method needs
   *       to take a boolean and write if it succeeds or not.. Messy.
   *       Very messy.
   */

  xtype_class_unref (klass);

  return mount;
}

/**
 * g_volume_monitor_adopt_orphan_mount:
 * @mount: a #xmount_t object to find a parent for
 *
 * This function should be called by any #xvolume_monitor_t
 * implementation when a new #xmount_t object is created that is not
 * associated with a #xvolume_t object. It must be called just before
 * emitting the @mount_added signal.
 *
 * If the return value is not %NULL, the caller must associate the
 * returned #xvolume_t object with the #xmount_t. This involves returning
 * it in its g_mount_get_volume() implementation. The caller must
 * also listen for the "removed" signal on the returned object
 * and give up its reference when handling that signal
 *
 * Similarly, if implementing g_volume_monitor_adopt_orphan_mount(),
 * the implementor must take a reference to @mount and return it in
 * its g_volume_get_mount() implemented. Also, the implementor must
 * listen for the "unmounted" signal on @mount and give up its
 * reference upon handling that signal.
 *
 * There are two main use cases for this function.
 *
 * One is when implementing a user space file system driver that reads
 * blocks of a block device that is already represented by the native
 * volume monitor (for example a CD Audio file system driver). Such
 * a driver will generate its own #xmount_t object that needs to be
 * associated with the #xvolume_t object that represents the volume.
 *
 * The other is for implementing a #xvolume_monitor_t whose sole purpose
 * is to return #xvolume_t objects representing entries in the users
 * "favorite servers" list or similar.
 *
 * Returns: (transfer full): the #xvolume_t object that is the parent for @mount or %NULL
 * if no wants to adopt the #xmount_t.
 *
 * Deprecated: 2.20: Instead of using this function, #xvolume_monitor_t
 * implementations should instead create shadow mounts with the URI of
 * the mount they intend to adopt. See the proxy volume monitor in
 * gvfs for an example of this. Also see g_mount_is_shadowed(),
 * g_mount_shadow() and g_mount_unshadow() functions.
 */
xvolume_t *
g_volume_monitor_adopt_orphan_mount (xmount_t *mount)
{
  xvolume_monitor_t *child_monitor;
  GVolumeMonitorClass *child_monitor_class;
  xvolume_t *volume;
  xlist_t *l;

  xreturn_val_if_fail (mount != NULL, NULL);

  if (the_volume_monitor == NULL)
    return NULL;

  volume = NULL;

  g_rec_mutex_lock (&the_volume_monitor_mutex);

  for (l = the_volume_monitor->monitors; l != NULL; l = l->next)
    {
      child_monitor = l->data;
      child_monitor_class = G_VOLUME_MONITOR_GET_CLASS (child_monitor);

      if (child_monitor_class->adopt_orphan_mount != NULL)
        {
          volume = child_monitor_class->adopt_orphan_mount (mount, child_monitor);
          if (volume != NULL)
            break;
        }
    }

  g_rec_mutex_unlock (&the_volume_monitor_mutex);

  return volume;
}
