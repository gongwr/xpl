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

#ifndef __G_VOLUME_MONITOR_H__
#define __G_VOLUME_MONITOR_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_VOLUME_MONITOR         (g_volume_monitor_get_type ())
#define G_VOLUME_MONITOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_VOLUME_MONITOR, xvolume_monitor))
#define G_VOLUME_MONITOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_VOLUME_MONITOR, GVolumeMonitorClass))
#define G_VOLUME_MONITOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_VOLUME_MONITOR, GVolumeMonitorClass))
#define X_IS_VOLUME_MONITOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_VOLUME_MONITOR))
#define X_IS_VOLUME_MONITOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_VOLUME_MONITOR))

/**
 * G_VOLUME_MONITOR_EXTENSION_POINT_NAME:
 *
 * Extension point for volume monitor functionality.
 * See [Extending GIO][extending-gio].
 */
#define G_VOLUME_MONITOR_EXTENSION_POINT_NAME "gio-volume-monitor"

/**
 * xvolume_monitor_t:
 *
 * A Volume Monitor that watches for volume events.
 **/
typedef struct _GVolumeMonitorClass GVolumeMonitorClass;

struct _xvolume_monitor
{
  xobject_t parent_instance;

  /*< private >*/
  xpointer_t priv;
};

struct _GVolumeMonitorClass
{
  xobject_class_t parent_class;

  /*< public >*/
  /* signals */
  void      (* volume_added)         (xvolume_monitor_t *volume_monitor,
                                      xvolume_t        *volume);
  void      (* volume_removed)       (xvolume_monitor_t *volume_monitor,
                                      xvolume_t        *volume);
  void      (* volume_changed)       (xvolume_monitor_t *volume_monitor,
                                      xvolume_t        *volume);

  void      (* mount_added)          (xvolume_monitor_t *volume_monitor,
                                      xmount_t         *mount);
  void      (* mount_removed)        (xvolume_monitor_t *volume_monitor,
                                      xmount_t         *mount);
  void      (* mount_pre_unmount)    (xvolume_monitor_t *volume_monitor,
                                      xmount_t         *mount);
  void      (* mount_changed)        (xvolume_monitor_t *volume_monitor,
                                      xmount_t         *mount);

  void      (* drive_connected)      (xvolume_monitor_t *volume_monitor,
                                      xdrive_t	     *drive);
  void      (* drive_disconnected)   (xvolume_monitor_t *volume_monitor,
                                      xdrive_t         *drive);
  void      (* drive_changed)        (xvolume_monitor_t *volume_monitor,
                                      xdrive_t         *drive);

  /* Vtable */

  xboolean_t  (* is_supported)         (void);

  xlist_t   * (* get_connected_drives) (xvolume_monitor_t *volume_monitor);
  xlist_t   * (* get_volumes)          (xvolume_monitor_t *volume_monitor);
  xlist_t   * (* get_mounts)           (xvolume_monitor_t *volume_monitor);

  xvolume_t * (* get_volume_for_uuid)  (xvolume_monitor_t *volume_monitor,
                                      const char     *uuid);

  xmount_t  * (* get_mount_for_uuid)   (xvolume_monitor_t *volume_monitor,
                                      const char     *uuid);


  /* These arguments are unfortunately backwards by mistake (bug #520169). Deprecated in 2.20. */
  xvolume_t * (* adopt_orphan_mount)   (xmount_t         *mount,
                                      xvolume_monitor_t *volume_monitor);

  /* signal added in 2.17 */
  void      (* drive_eject_button)   (xvolume_monitor_t *volume_monitor,
                                      xdrive_t         *drive);

  /* signal added in 2.21 */
  void      (* drive_stop_button)   (xvolume_monitor_t *volume_monitor,
                                     xdrive_t         *drive);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t           g_volume_monitor_get_type             (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xvolume_monitor_t *g_volume_monitor_get                  (void);
XPL_AVAILABLE_IN_ALL
xlist_t *         g_volume_monitor_get_connected_drives (xvolume_monitor_t *volume_monitor);
XPL_AVAILABLE_IN_ALL
xlist_t *         g_volume_monitor_get_volumes          (xvolume_monitor_t *volume_monitor);
XPL_AVAILABLE_IN_ALL
xlist_t *         g_volume_monitor_get_mounts           (xvolume_monitor_t *volume_monitor);
XPL_AVAILABLE_IN_ALL
xvolume_t *       g_volume_monitor_get_volume_for_uuid  (xvolume_monitor_t *volume_monitor,
                                                       const char     *uuid);
XPL_AVAILABLE_IN_ALL
xmount_t *        g_volume_monitor_get_mount_for_uuid   (xvolume_monitor_t *volume_monitor,
                                                       const char     *uuid);

XPL_DEPRECATED
xvolume_t *       g_volume_monitor_adopt_orphan_mount   (xmount_t         *mount);

G_END_DECLS

#endif /* __G_VOLUME_MONITOR_H__ */
