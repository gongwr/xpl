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
#define G_MOUNT(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_MOUNT, xmount))
#define X_IS_MOUNT(obj)         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_MOUNT))
#define G_MOUNT_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_MOUNT, GMountIface))

typedef struct _GMountIface    GMountIface;

/**
 * GMountIface:
 * @x_iface: The parent interface.
 * @changed: Changed signal that is emitted when the mount's state has changed.
 * @unmounted: The unmounted signal that is emitted when the #xmount_t have been unmounted. If the recipient is holding references to the object they should release them so the object can be finalized.
 * @pre_unmount: The ::pre-unmount signal that is emitted when the #xmount_t will soon be emitted. If the recipient is somehow holding the mount open by keeping an open file on it it should close the file.
 * @get_root: Gets a #xfile_t to the root directory of the #xmount_t.
 * @get_name: Gets a string containing the name of the #xmount_t.
 * @get_icon: Gets a #xicon_t for the #xmount_t.
 * @get_uuid: Gets the UUID for the #xmount_t. The reference is typically based on the file system UUID for the mount in question and should be considered an opaque string. Returns %NULL if there is no UUID available.
 * @get_volume: Gets a #xvolume_t the mount is located on. Returns %NULL if the #xmount_t is not associated with a #xvolume_t.
 * @get_drive: Gets a #xdrive_t the volume of the mount is located on. Returns %NULL if the #xmount_t is not associated with a #xdrive_t or a #xvolume_t. This is convenience method for getting the #xvolume_t and using that to get the #xdrive_t.
 * @can_unmount: Checks if a #xmount_t can be unmounted.
 * @can_eject: Checks if a #xmount_t can be ejected.
 * @unmount: Starts unmounting a #xmount_t.
 * @unmount_finish: Finishes an unmounting operation.
 * @eject: Starts ejecting a #xmount_t.
 * @eject_finish: Finishes an eject operation.
 * @remount: Starts remounting a #xmount_t.
 * @remount_finish: Finishes a remounting operation.
 * @guess_content_type: Starts guessing the type of the content of a #xmount_t.
 *     See g_mount_guess_content_type() for more information on content
 *     type guessing. This operation was added in 2.18.
 * @guess_content_type_finish: Finishes a content type guessing operation. Added in 2.18.
 * @guess_content_type_sync: Synchronous variant of @guess_content_type. Added in 2.18
 * @unmount_with_operation: Starts unmounting a #xmount_t using a #xmount_operation_t. Since 2.22.
 * @unmount_with_operation_finish: Finishes an unmounting operation using a #xmount_operation_t. Since 2.22.
 * @eject_with_operation: Starts ejecting a #xmount_t using a #xmount_operation_t. Since 2.22.
 * @eject_with_operation_finish: Finishes an eject operation using a #xmount_operation_t. Since 2.22.
 * @get_default_location: Gets a #xfile_t indication a start location that can be use as the entry point for this mount. Since 2.24.
 * @get_sort_key: Gets a key used for sorting #xmount_t instance or %NULL if no such key exists. Since 2.32.
 * @get_symbolic_icon: Gets a symbolic #xicon_t for the #xmount_t. Since 2.34.
 *
 * Interface for implementing operations for mounts.
 **/
struct _GMountIface
{
  xtype_interface_t x_iface;

  /* signals */

  void        (* changed)                   (xmount_t              *mount);
  void        (* unmounted)                 (xmount_t              *mount);

  /* Virtual Table */

  xfile_t     * (* get_root)                  (xmount_t              *mount);
  char      * (* get_name)                  (xmount_t              *mount);
  xicon_t     * (* get_icon)                  (xmount_t              *mount);
  char      * (* get_uuid)                  (xmount_t              *mount);
  xvolume_t   * (* get_volume)                (xmount_t              *mount);
  xdrive_t    * (* get_drive)                 (xmount_t              *mount);
  xboolean_t    (* can_unmount)               (xmount_t              *mount);
  xboolean_t    (* can_eject)                 (xmount_t              *mount);

  void        (* unmount)                   (xmount_t              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* unmount_finish)            (xmount_t              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* eject)                     (xmount_t              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* eject_finish)              (xmount_t              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* remount)                   (xmount_t              *mount,
                                             GMountMountFlags     flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* remount_finish)            (xmount_t              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* guess_content_type)        (xmount_t              *mount,
                                             xboolean_t             force_rescan,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xchar_t    ** (* guess_content_type_finish) (xmount_t              *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);
  xchar_t    ** (* guess_content_type_sync)   (xmount_t              *mount,
                                             xboolean_t             force_rescan,
                                             xcancellable_t        *cancellable,
                                             xerror_t             **error);

  /* Signal, not VFunc */
  void        (* pre_unmount)               (xmount_t              *mount);

  void        (* unmount_with_operation)    (xmount_t              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* unmount_with_operation_finish) (xmount_t          *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  void        (* eject_with_operation)      (xmount_t              *mount,
                                             xmount_unmount_flags_t   flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* eject_with_operation_finish) (xmount_t            *mount,
                                             xasync_result_t        *result,
                                             xerror_t             **error);
  xfile_t     * (* get_default_location)      (xmount_t              *mount);

  const xchar_t * (* get_sort_key)            (xmount_t              *mount);
  xicon_t       * (* get_symbolic_icon)       (xmount_t              *mount);
};

XPL_AVAILABLE_IN_ALL
xtype_t       g_mount_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xfile_t     * g_mount_get_root                  (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xfile_t     * g_mount_get_default_location      (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
char      * g_mount_get_name                  (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xicon_t     * g_mount_get_icon                  (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xicon_t     * g_mount_get_symbolic_icon         (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
char      * g_mount_get_uuid                  (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xvolume_t   * g_mount_get_volume                (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xdrive_t    * g_mount_get_drive                 (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_can_unmount               (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_can_eject                 (xmount_t              *mount);

XPL_DEPRECATED_FOR(g_mount_unmount_with_operation)
void        g_mount_unmount                   (xmount_t              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);

XPL_DEPRECATED_FOR(g_mount_unmount_with_operation_finish)
xboolean_t    g_mount_unmount_finish            (xmount_t              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_DEPRECATED_FOR(g_mount_eject_with_operation)
void        g_mount_eject                     (xmount_t              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);

XPL_DEPRECATED_FOR(g_mount_eject_with_operation_finish)
xboolean_t    g_mount_eject_finish              (xmount_t              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void        g_mount_remount                   (xmount_t              *mount,
                                               GMountMountFlags     flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_remount_finish            (xmount_t              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void        g_mount_guess_content_type        (xmount_t              *mount,
                                               xboolean_t             force_rescan,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xchar_t    ** g_mount_guess_content_type_finish (xmount_t              *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xchar_t    ** g_mount_guess_content_type_sync   (xmount_t              *mount,
                                               xboolean_t             force_rescan,
                                               xcancellable_t        *cancellable,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_is_shadowed               (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
void        g_mount_shadow                    (xmount_t              *mount);
XPL_AVAILABLE_IN_ALL
void        g_mount_unshadow                  (xmount_t              *mount);

XPL_AVAILABLE_IN_ALL
void        g_mount_unmount_with_operation    (xmount_t              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_unmount_with_operation_finish (xmount_t          *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
void        g_mount_eject_with_operation      (xmount_t              *mount,
                                               xmount_unmount_flags_t   flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_mount_eject_with_operation_finish (xmount_t            *mount,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_ALL
const xchar_t *g_mount_get_sort_key             (xmount_t              *mount);

G_END_DECLS

#endif /* __G_MOUNT_H__ */
