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
 */

#ifndef __G_UNIX_MOUNTS_H__
#define __G_UNIX_MOUNTS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * GUnixMountEntry:
 *
 * Defines a Unix mount entry (e.g. <filename>/media/cdrom</filename>).
 * This corresponds roughly to a mtab entry.
 **/
typedef struct _GUnixMountEntry GUnixMountEntry;

#define XTYPE_UNIX_MOUNT_ENTRY (g_unix_mount_entry_get_type ())
XPL_AVAILABLE_IN_2_54
xtype_t g_unix_mount_entry_get_type (void) G_GNUC_CONST;

/**
 * GUnixMountPoint:
 *
 * Defines a Unix mount point (e.g. <filename>/dev</filename>).
 * This corresponds roughly to a fstab entry.
 **/
typedef struct _GUnixMountPoint GUnixMountPoint;

#define XTYPE_UNIX_MOUNT_POINT (g_unix_mount_point_get_type ())
XPL_AVAILABLE_IN_2_54
xtype_t g_unix_mount_point_get_type (void) G_GNUC_CONST;

/**
 * xunix_mount_monitor_t:
 *
 * Watches #GUnixMounts for changes.
 **/
typedef struct _xunix_mount_monitor      xunix_mount_monitor_t;
typedef struct _xunix_mount_monitor_class xunix_mount_monitor_class_t;

#define XTYPE_UNIX_MOUNT_MONITOR        (xunix_mount_monitor_get_type ())
#define G_UNIX_MOUNT_MONITOR(o)          (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_UNIX_MOUNT_MONITOR, xunix_mount_monitor_t))
#define G_UNIX_MOUNT_MONITOR_CLASS(k)    (XTYPE_CHECK_CLASS_CAST((k), XTYPE_UNIX_MOUNT_MONITOR, xunix_mount_monitor_class_t))
#define X_IS_UNIX_MOUNT_MONITOR(o)       (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_UNIX_MOUNT_MONITOR))
#define X_IS_UNIX_MOUNT_MONITOR_CLASS(k) (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_UNIX_MOUNT_MONITOR))
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xunix_mount_monitor_t, xobject_unref)

XPL_AVAILABLE_IN_ALL
void           g_unix_mount_free                    (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_2_54
GUnixMountEntry *g_unix_mount_copy                  (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
void           g_unix_mount_point_free              (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_2_54
GUnixMountPoint *g_unix_mount_point_copy            (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xint_t           g_unix_mount_compare                 (GUnixMountEntry    *mount1,
						     GUnixMountEntry    *mount2);
XPL_AVAILABLE_IN_ALL
const char *   g_unix_mount_get_mount_path          (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
const char *   g_unix_mount_get_device_path         (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_2_60
const char *   g_unix_mount_get_root_path           (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
const char *   g_unix_mount_get_fs_type             (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_2_58
const char *   g_unix_mount_get_options             (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_is_readonly             (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_is_system_internal      (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_guess_can_eject         (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_guess_should_display    (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
char *         g_unix_mount_guess_name              (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
xicon_t *        g_unix_mount_guess_icon              (GUnixMountEntry    *mount_entry);
XPL_AVAILABLE_IN_ALL
xicon_t *        g_unix_mount_guess_symbolic_icon     (GUnixMountEntry    *mount_entry);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixMountEntry, g_unix_mount_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixMountPoint, g_unix_mount_point_free)

XPL_AVAILABLE_IN_ALL
xint_t           g_unix_mount_point_compare           (GUnixMountPoint    *mount1,
						     GUnixMountPoint    *mount2);
XPL_AVAILABLE_IN_ALL
const char *   g_unix_mount_point_get_mount_path    (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
const char *   g_unix_mount_point_get_device_path   (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
const char *   g_unix_mount_point_get_fs_type       (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_2_32
const char *   g_unix_mount_point_get_options       (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_point_is_readonly       (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_point_is_user_mountable (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_point_is_loopback       (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_point_guess_can_eject   (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
char *         g_unix_mount_point_guess_name        (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xicon_t *        g_unix_mount_point_guess_icon        (GUnixMountPoint    *mount_point);
XPL_AVAILABLE_IN_ALL
xicon_t *        g_unix_mount_point_guess_symbolic_icon (GUnixMountPoint    *mount_point);


XPL_AVAILABLE_IN_ALL
xlist_t *        g_unix_mount_points_get              (xuint64_t            *time_read);
XPL_AVAILABLE_IN_2_66
GUnixMountPoint *g_unix_mount_point_at              (const char         *mount_path,
                                                     xuint64_t            *time_read);
XPL_AVAILABLE_IN_ALL
xlist_t *        g_unix_mounts_get                    (xuint64_t            *time_read);
XPL_AVAILABLE_IN_ALL
GUnixMountEntry *g_unix_mount_at                    (const char         *mount_path,
						     xuint64_t            *time_read);
XPL_AVAILABLE_IN_2_52
GUnixMountEntry *g_unix_mount_for                   (const char         *file_path,
                                                     xuint64_t            *time_read);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mounts_changed_since          (xuint64_t             time);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_mount_points_changed_since    (xuint64_t             time);

XPL_AVAILABLE_IN_ALL
xtype_t              xunix_mount_monitor_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_44
xunix_mount_monitor_t *xunix_mount_monitor_get            (void);
XPL_DEPRECATED_IN_2_44_FOR(xunix_mount_monitor_get)
xunix_mount_monitor_t *xunix_mount_monitor_new            (void);
XPL_DEPRECATED_IN_2_44
void               xunix_mount_monitor_set_rate_limit (xunix_mount_monitor_t *mount_monitor,
                                                        int                limit_msec);

XPL_AVAILABLE_IN_ALL
xboolean_t g_unix_is_mount_path_system_internal (const char *mount_path);
XPL_AVAILABLE_IN_2_56
xboolean_t g_unix_is_system_fs_type             (const char *fs_type);
XPL_AVAILABLE_IN_2_56
xboolean_t g_unix_is_system_device_path         (const char *device_path);

G_END_DECLS

#endif /* __G_UNIX_MOUNTS_H__ */
