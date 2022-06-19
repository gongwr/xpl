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
#include "gvolumemonitor.h"
#include "gvolume.h"
#include "gmount.h"
#include "gdrive.h"
#include "glibintl.h"


/**
 * SECTION:gvolumemonitor
 * @short_description: Volume Monitor
 * @include: gio/gio.h
 * @see_also: #xfile_monitor_t
 *
 * #xvolume_monitor_t is for listing the user interesting devices and volumes
 * on the computer. In other words, what a file selector or file manager
 * would show in a sidebar.
 *
 * #xvolume_monitor_t is not
 * [thread-default-context aware][g-main-context-push-thread-default],
 * and so should not be used other than from the main thread, with no
 * thread-default-context active.
 *
 * In order to receive updates about volumes and mounts monitored through GVFS,
 * a main loop must be running.
 **/

XDEFINE_TYPE (xvolume_monitor, g_volume_monitor, XTYPE_OBJECT)

enum {
  VOLUME_ADDED,
  VOLUME_REMOVED,
  VOLUME_CHANGED,
  MOUNT_ADDED,
  MOUNT_REMOVED,
  MOUNT_PRE_UNMOUNT,
  MOUNT_CHANGED,
  DRIVE_CONNECTED,
  DRIVE_DISCONNECTED,
  DRIVE_CHANGED,
  DRIVE_EJECT_BUTTON,
  DRIVE_STOP_BUTTON,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };


static void
g_volume_monitor_finalize (xobject_t *object)
{
  XOBJECT_CLASS (g_volume_monitor_parent_class)->finalize (object);
}

static void
g_volume_monitor_class_init (GVolumeMonitorClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = g_volume_monitor_finalize;

  /**
   * xvolume_monitor_t::volume-added:
   * @volume_monitor: The volume monitor emitting the signal.
   * @volume: a #xvolume_t that was added.
   *
   * Emitted when a mountable volume is added to the system.
   **/
  signals[VOLUME_ADDED] = xsignal_new (I_("volume-added"),
                                        XTYPE_VOLUME_MONITOR,
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (GVolumeMonitorClass, volume_added),
                                        NULL, NULL,
                                        NULL,
                                        XTYPE_NONE, 1, XTYPE_VOLUME);

  /**
   * xvolume_monitor_t::volume-removed:
   * @volume_monitor: The volume monitor emitting the signal.
   * @volume: a #xvolume_t that was removed.
   *
   * Emitted when a mountable volume is removed from the system.
   **/
  signals[VOLUME_REMOVED] = xsignal_new (I_("volume-removed"),
                                          XTYPE_VOLUME_MONITOR,
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (GVolumeMonitorClass, volume_removed),
                                          NULL, NULL,
                                          NULL,
                                          XTYPE_NONE, 1, XTYPE_VOLUME);

  /**
   * xvolume_monitor_t::volume-changed:
   * @volume_monitor: The volume monitor emitting the signal.
   * @volume: a #xvolume_t that changed.
   *
   * Emitted when mountable volume is changed.
   **/
  signals[VOLUME_CHANGED] = xsignal_new (I_("volume-changed"),
                                          XTYPE_VOLUME_MONITOR,
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (GVolumeMonitorClass, volume_changed),
                                          NULL, NULL,
                                          NULL,
                                          XTYPE_NONE, 1, XTYPE_VOLUME);

  /**
   * xvolume_monitor_t::mount-added:
   * @volume_monitor: The volume monitor emitting the signal.
   * @mount: a #xmount_t that was added.
   *
   * Emitted when a mount is added.
   **/
  signals[MOUNT_ADDED] = xsignal_new (I_("mount-added"),
                                       XTYPE_VOLUME_MONITOR,
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (GVolumeMonitorClass, mount_added),
                                       NULL, NULL,
                                       NULL,
                                       XTYPE_NONE, 1, XTYPE_MOUNT);

  /**
   * xvolume_monitor_t::mount-removed:
   * @volume_monitor: The volume monitor emitting the signal.
   * @mount: a #xmount_t that was removed.
   *
   * Emitted when a mount is removed.
   **/
  signals[MOUNT_REMOVED] = xsignal_new (I_("mount-removed"),
                                         XTYPE_VOLUME_MONITOR,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GVolumeMonitorClass, mount_removed),
                                         NULL, NULL,
                                         NULL,
                                         XTYPE_NONE, 1, XTYPE_MOUNT);

  /**
   * xvolume_monitor_t::mount-pre-unmount:
   * @volume_monitor: The volume monitor emitting the signal.
   * @mount: a #xmount_t that is being unmounted.
   *
   * May be emitted when a mount is about to be removed.
   *
   * This signal depends on the backend and is only emitted if
   * GIO was used to unmount.
   **/
  signals[MOUNT_PRE_UNMOUNT] = xsignal_new (I_("mount-pre-unmount"),
                                             XTYPE_VOLUME_MONITOR,
                                             G_SIGNAL_RUN_LAST,
                                             G_STRUCT_OFFSET (GVolumeMonitorClass, mount_pre_unmount),
                                             NULL, NULL,
                                             NULL,
                                             XTYPE_NONE, 1, XTYPE_MOUNT);

  /**
   * xvolume_monitor_t::mount-changed:
   * @volume_monitor: The volume monitor emitting the signal.
   * @mount: a #xmount_t that changed.
   *
   * Emitted when a mount changes.
   **/
  signals[MOUNT_CHANGED] = xsignal_new (I_("mount-changed"),
                                         XTYPE_VOLUME_MONITOR,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GVolumeMonitorClass, mount_changed),
                                         NULL, NULL,
                                         NULL,
                                         XTYPE_NONE, 1, XTYPE_MOUNT);

  /**
   * xvolume_monitor_t::drive-connected:
   * @volume_monitor: The volume monitor emitting the signal.
   * @drive: a #xdrive_t that was connected.
   *
   * Emitted when a drive is connected to the system.
   **/
  signals[DRIVE_CONNECTED] = xsignal_new (I_("drive-connected"),
					   XTYPE_VOLUME_MONITOR,
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (GVolumeMonitorClass, drive_connected),
					   NULL, NULL,
					   NULL,
					   XTYPE_NONE, 1, XTYPE_DRIVE);

  /**
   * xvolume_monitor_t::drive-disconnected:
   * @volume_monitor: The volume monitor emitting the signal.
   * @drive: a #xdrive_t that was disconnected.
   *
   * Emitted when a drive is disconnected from the system.
   **/
  signals[DRIVE_DISCONNECTED] = xsignal_new (I_("drive-disconnected"),
					      XTYPE_VOLUME_MONITOR,
					      G_SIGNAL_RUN_LAST,
					      G_STRUCT_OFFSET (GVolumeMonitorClass, drive_disconnected),
					      NULL, NULL,
					      NULL,
					      XTYPE_NONE, 1, XTYPE_DRIVE);

  /**
   * xvolume_monitor_t::drive-changed:
   * @volume_monitor: The volume monitor emitting the signal.
   * @drive: the drive that changed
   *
   * Emitted when a drive changes.
   **/
  signals[DRIVE_CHANGED] = xsignal_new (I_("drive-changed"),
                                         XTYPE_VOLUME_MONITOR,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GVolumeMonitorClass, drive_changed),
                                         NULL, NULL,
                                         NULL,
                                         XTYPE_NONE, 1, XTYPE_DRIVE);

  /**
   * xvolume_monitor_t::drive-eject-button:
   * @volume_monitor: The volume monitor emitting the signal.
   * @drive: the drive where the eject button was pressed
   *
   * Emitted when the eject button is pressed on @drive.
   *
   * Since: 2.18
   **/
  signals[DRIVE_EJECT_BUTTON] = xsignal_new (I_("drive-eject-button"),
                                              XTYPE_VOLUME_MONITOR,
                                              G_SIGNAL_RUN_LAST,
                                              G_STRUCT_OFFSET (GVolumeMonitorClass, drive_eject_button),
                                              NULL, NULL,
                                              NULL,
                                              XTYPE_NONE, 1, XTYPE_DRIVE);

  /**
   * xvolume_monitor_t::drive-stop-button:
   * @volume_monitor: The volume monitor emitting the signal.
   * @drive: the drive where the stop button was pressed
   *
   * Emitted when the stop button is pressed on @drive.
   *
   * Since: 2.22
   **/
  signals[DRIVE_STOP_BUTTON] = xsignal_new (I_("drive-stop-button"),
                                             XTYPE_VOLUME_MONITOR,
                                             G_SIGNAL_RUN_LAST,
                                             G_STRUCT_OFFSET (GVolumeMonitorClass, drive_stop_button),
                                             NULL, NULL,
                                             NULL,
                                             XTYPE_NONE, 1, XTYPE_DRIVE);

}

static void
g_volume_monitor_init (xvolume_monitor_t *monitor)
{
}


/**
 * g_volume_monitor_get_connected_drives:
 * @volume_monitor: a #xvolume_monitor_t.
 *
 * Gets a list of drives connected to the system.
 *
 * The returned list should be freed with xlist_free(), after
 * its elements have been unreffed with xobject_unref().
 *
 * Returns: (element-type xdrive_t) (transfer full): a #xlist_t of connected #xdrive_t objects.
 **/
xlist_t *
g_volume_monitor_get_connected_drives (xvolume_monitor_t *volume_monitor)
{
  GVolumeMonitorClass *class;

  xreturn_val_if_fail (X_IS_VOLUME_MONITOR (volume_monitor), NULL);

  class = G_VOLUME_MONITOR_GET_CLASS (volume_monitor);

  return class->get_connected_drives (volume_monitor);
}

/**
 * g_volume_monitor_get_volumes:
 * @volume_monitor: a #xvolume_monitor_t.
 *
 * Gets a list of the volumes on the system.
 *
 * The returned list should be freed with xlist_free(), after
 * its elements have been unreffed with xobject_unref().
 *
 * Returns: (element-type xvolume_t) (transfer full): a #xlist_t of #xvolume_t objects.
 **/
xlist_t *
g_volume_monitor_get_volumes (xvolume_monitor_t *volume_monitor)
{
  GVolumeMonitorClass *class;

  xreturn_val_if_fail (X_IS_VOLUME_MONITOR (volume_monitor), NULL);

  class = G_VOLUME_MONITOR_GET_CLASS (volume_monitor);

  return class->get_volumes (volume_monitor);
}

/**
 * g_volume_monitor_get_mounts:
 * @volume_monitor: a #xvolume_monitor_t.
 *
 * Gets a list of the mounts on the system.
 *
 * The returned list should be freed with xlist_free(), after
 * its elements have been unreffed with xobject_unref().
 *
 * Returns: (element-type xmount_t) (transfer full): a #xlist_t of #xmount_t objects.
 **/
xlist_t *
g_volume_monitor_get_mounts (xvolume_monitor_t *volume_monitor)
{
  GVolumeMonitorClass *class;

  xreturn_val_if_fail (X_IS_VOLUME_MONITOR (volume_monitor), NULL);

  class = G_VOLUME_MONITOR_GET_CLASS (volume_monitor);

  return class->get_mounts (volume_monitor);
}

/**
 * g_volume_monitor_get_volume_for_uuid:
 * @volume_monitor: a #xvolume_monitor_t.
 * @uuid: the UUID to look for
 *
 * Finds a #xvolume_t object by its UUID (see g_volume_get_uuid())
 *
 * Returns: (nullable) (transfer full): a #xvolume_t or %NULL if no such volume is available.
 *     Free the returned object with xobject_unref().
 **/
xvolume_t *
g_volume_monitor_get_volume_for_uuid (xvolume_monitor_t *volume_monitor,
                                      const char     *uuid)
{
  GVolumeMonitorClass *class;

  xreturn_val_if_fail (X_IS_VOLUME_MONITOR (volume_monitor), NULL);
  xreturn_val_if_fail (uuid != NULL, NULL);

  class = G_VOLUME_MONITOR_GET_CLASS (volume_monitor);

  return class->get_volume_for_uuid (volume_monitor, uuid);
}

/**
 * g_volume_monitor_get_mount_for_uuid:
 * @volume_monitor: a #xvolume_monitor_t.
 * @uuid: the UUID to look for
 *
 * Finds a #xmount_t object by its UUID (see g_mount_get_uuid())
 *
 * Returns: (nullable) (transfer full): a #xmount_t or %NULL if no such mount is available.
 *     Free the returned object with xobject_unref().
 **/
xmount_t *
g_volume_monitor_get_mount_for_uuid (xvolume_monitor_t *volume_monitor,
                                     const char     *uuid)
{
  GVolumeMonitorClass *class;

  xreturn_val_if_fail (X_IS_VOLUME_MONITOR (volume_monitor), NULL);
  xreturn_val_if_fail (uuid != NULL, NULL);

  class = G_VOLUME_MONITOR_GET_CLASS (volume_monitor);

  return class->get_mount_for_uuid (volume_monitor, uuid);
}
