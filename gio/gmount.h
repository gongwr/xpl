/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2008 Red Hat, Inc.
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

#ifndef __G_MOUNT_H__
#define __G_MOUNT_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_MOUNT            (g_mount_get_type ())
#define G_MOUNT(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_MOUNT, GMount))
#define X_IS_MOUNT(obj)         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_MOUNT))
#define G_MOUNT_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_MOUNT, GMountIface))

typedef struct _GMountIface    GMountIface;

/**
 * GMountIface:
 * @x_iface: The parent interface.
 * @changed: Changed signal that is emitted when the mount's state has changed.
 * @unmounted: The unmounted signal that is emitted when the #GMount have been unmounted. If the recipient is holding references to the object they should release them so the object can be finalized.
 * @pre_unmount: The ::pre-unmount signal that is emitted when the #GMount will soon be emitted. If the recipient is somehow holding the mount open by keeping an open file on it it should close the file.
 * @get_root: Gets a #xfile_t to the root directory of the #GMount.
 * @get_name: Gets a string containing the name of the #GMount.
 * @get_icon: Gets a #xicon_t for the #GMount.
 * @get_uuid: Gets the UUID for the #GMount. The reference is typically based on the file system UUID for the mount in question and should be considered an opaque string. Returns %NULL if there is no UUID available.
 * @get_volume: Gets a #GVolume the mount is located on. Returns %NULL if the #GMount is not associated with a #GVolume.
 * @get_drive: Gets a #xdrive_t the volume of the mount is located on. Returns %NULL if the #GMount is not associated with a #xdrive_t or a #GVolume. This is convenience method for getting the #GVolume and using that to get the #xdrive_t.
 * @can_unmount: Checks if a #GMount can be unmounted.
 * @can_eject: Checks if a #GMount can be ejected.
 * @unmount: Starts unmounting a #GMount.
 * @unmount_finish: Finishes an unmounting operation.
 * @eject: Starts ejecting a #GMount.
 * @eject_finish: Finishes an eject operation.
 * @remount: Starts remounting a #GMount.
 * @remount_finish: Finishes a remounting operation.
 * @guess_content_type: Starts guessing the type of the content of a #GMount.
 *     See g_mount_guess_content_type() for more information on content
 *     type guessing. This operation was added in 2.18.
 * @guess_content_type_finish: Finishes a content type guessing operation. Added in 2.18.
 * @guess_content_type_sync: Synchronous variant of @guess_content_type. Added in 2.18
 * @unmount_with_operation: Starts unmounting a #GMount using a #xmount_operation_t. Since 2.22.
 * @unmount_with_operation_finish: Finishes an unmounting operation using a #xmount_operation_t. Since 2.22.
 * @eject_with_operation: Starts ejecting a #GMount using a #xmount_operation_t. Since 2.22.
 * @eject_with_operation_finish: Finishes an eject operation using a #xmount_operation_t. Since 2.22.
 * @get_default_location: Gets a #xfile_t indication a start location that can be use as the entry point for this mount. Since 2.24.
 * @get_sort_key: Gets a key used for sorting #GMount instance or %NULL if no such key exists. Since 2.32.
 * @get_symbolic_icon: Gets a symbolic #xicon_t for the #GMount. Since 2.34.
 *
 * Interface for implementing operations for mounts.
 **/
struct _GMountIface
{
  xtype_interface_t x_iface;

  /* signals */

  void        (* changed)                   (GMount              *mount);
  void        (* unmounted)                 (GMount              *mount);

  /* Virtual Table */

  xfile_t     * (* get_root)                  (GMount              *mount);
  char      * (* get_name)                  (GMount              *mount);
  xicon_t     * (* get_icon)                  (GMount              *mount);
  char      * (* get_uuid)                  (GMount              *mount);
  GVolume   * (* get_volume)                (GMount              *mount);
  xdrive_t    * (* get_drive)                 (GMount              *mount);
  xboolean_t    (* can_unmount)               (GMount              *mount);
  xboolean_t    (* can_eject)                 (GMount              *mount);

  void        (* unmount)                   (GMount              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* unmount_finish)            (GMount              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* eject)                     (GMount              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* eject_finish)              (GMount              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* remount)                   (GMount              *mount,
                                             GMountMountFlags     flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* remount_finish)            (GMount              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* guess_content_type)        (GMount              *mount,
                                             xboolean_t             force_rescan,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xchar_t    ** (* guess_content_type_finish) (GMount              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);
  xchar_t    ** (* guess_content_type_sync)   (GMount              *mount,
                                             xboolean_t             force_rescan,
                                             xcancellable_t        *cancellable,
                                             xerror_t             **error);

  /* Signal, not VFunc */
  void        (* pre_unmount)               (GMount              *mount);

  void        (* unmount_with_operation)    (GMount              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* unmount_with_operation_finish) (GMount          *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* eject_with_operation)      (GMount              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* eject_with_operation_finish) (GMount            *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);
  xfile_t     * (* get_default_location)      (GMount              *mount);

  const xchar_t * (* get_sort_key)            (GMount              *mount);
  xicon_t       * (* get_symbolic_icon)       (GMount              *mount);
};

XPL_AVAILABLE_IN_ALL
xtype_t       g_mount_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xfile_t     * g_mount_get_root                  (GMount              *mount);
XPL_AVAILABLE_IN_ALL
xfile_t     * g_mount_get_default_location      (GMount              *mount);
XPL_AVAILABLE_IN_ALL
char      * g_mount_get_name                  (GMount              *mount);
XPL_AVAILABLE_IN_ALL
xicon_t     * g_mount_get_icon                  (GMount              *mount);
XPL_AVAILABLE_IN_ALL
xicon_t     * g_mount_get_symbolic_icon         (GMount              *mount);
XPL_AVAILABLE_IN_ALL
char      * g_mount_get_uuid                  (GMount              *mount);
XPL_AVAILABLE_IN_ALL
GVolume   * g_mount_get_volume                (GMount              *mount);
XPL_AVAILABLE_IN_ALL
xdrive_t    * g_mount_get_drive                 (GMount              *mount);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_can_unmount               (GMount              *mount);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_can_eject                 (GMount              *mount);

XPL_DEPRECATED_FOR(g_mount_unmount_with_operation)
void        g_mount_unmount                   (GMount              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);

XPL_DEPRECATED_FOR(g_mount_unmount_with_operation_finish)
xboolean_t    g_mount_unmount_finish            (GMount              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_DEPRECATED_FOR(g_mount_eject_with_operation)
void        g_mount_eject                     (GMount              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);

XPL_DEPRECATED_FOR(g_mount_eject_with_operation_finish)
xboolean_t    g_mount_eject_finish              (GMount              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void        g_mount_remount                   (GMount              *mount,
                                               GMountMountFlags     flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_remount_finish            (GMount              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void        g_mount_guess_content_type        (GMount              *mount,
                                               xboolean_t             force_rescan,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xchar_t    ** g_mount_guess_content_type_finish (GMount              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xchar_t    ** g_mount_guess_content_type_sync   (GMount              *mount,
                                               xboolean_t             force_rescan,
                                               xcancellable_t        *cancellable,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_is_shadowed               (GMount              *mount);
XPL_AVAILABLE_IN_ALL
void        g_mount_shadow                    (GMount              *mount);
XPL_AVAILABLE_IN_ALL
void        g_mount_unshadow                  (GMount              *mount);

XPL_AVAILABLE_IN_ALL
void        g_mount_unmount_with_operation    (GMount              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_unmount_with_operation_finish (GMount          *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void        g_mount_eject_with_operation      (GMount              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_eject_with_operation_finish (GMount            *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
const xchar_t *g_mount_get_sort_key             (GMount              *mount);

G_END_DECLS

#endif /* __G_MOUNT_H__ */
