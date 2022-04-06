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

#ifndef __G_VOLUME_H__
#define __G_VOLUME_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * G_VOLUME_IDENTIFIER_KIND_HAL_UDI:
 *
 * The string used to obtain a Hal UDI with g_volume_get_identifier().
 *
 * Deprecated: 2.58: Do not use, HAL is deprecated.
 */
#define G_VOLUME_IDENTIFIER_KIND_HAL_UDI "hal-udi" XPL_DEPRECATED_MACRO_IN_2_58

/**
 * G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE:
 *
 * The string used to obtain a Unix device path with g_volume_get_identifier().
 */
#define G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE "unix-device"

/**
 * G_VOLUME_IDENTIFIER_KIND_LABEL:
 *
 * The string used to obtain a filesystem label with g_volume_get_identifier().
 */
#define G_VOLUME_IDENTIFIER_KIND_LABEL "label"

/**
 * G_VOLUME_IDENTIFIER_KIND_UUID:
 *
 * The string used to obtain a UUID with g_volume_get_identifier().
 */
#define G_VOLUME_IDENTIFIER_KIND_UUID "uuid"

/**
 * G_VOLUME_IDENTIFIER_KIND_NFS_MOUNT:
 *
 * The string used to obtain a NFS mount with g_volume_get_identifier().
 */
#define G_VOLUME_IDENTIFIER_KIND_NFS_MOUNT "nfs-mount"

/**
 * G_VOLUME_IDENTIFIER_KIND_CLASS:
 *
 * The string used to obtain the volume class with g_volume_get_identifier().
 *
 * Known volume classes include `device`, `network`, and `loop`. Other
 * classes may be added in the future.
 *
 * This is intended to be used by applications to classify #xvolume_t
 * instances into different sections - for example a file manager or
 * file chooser can use this information to show `network` volumes under
 * a "Network" heading and `device` volumes under a "Devices" heading.
 */
#define G_VOLUME_IDENTIFIER_KIND_CLASS "class"


#define XTYPE_VOLUME            (g_volume_get_type ())
#define G_VOLUME(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_VOLUME, xvolume))
#define X_IS_VOLUME(obj)         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_VOLUME))
#define G_VOLUME_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_VOLUME, GVolumeIface))

/**
 * GVolumeIface:
 * @x_iface: The parent interface.
 * @changed: Changed signal that is emitted when the volume's state has changed.
 * @removed: The removed signal that is emitted when the #xvolume_t have been removed. If the recipient is holding references to the object they should release them so the object can be finalized.
 * @get_name: Gets a string containing the name of the #xvolume_t.
 * @get_icon: Gets a #xicon_t for the #xvolume_t.
 * @get_uuid: Gets the UUID for the #xvolume_t. The reference is typically based on the file system UUID for the mount in question and should be considered an opaque string. Returns %NULL if there is no UUID available.
 * @get_drive: Gets a #xdrive_t the volume is located on. Returns %NULL if the #xvolume_t is not associated with a #xdrive_t.
 * @get_mount: Gets a #xmount_t representing the mounted volume. Returns %NULL if the #xvolume_t is not mounted.
 * @can_mount: Returns %TRUE if the #xvolume_t can be mounted.
 * @can_eject: Checks if a #xvolume_t can be ejected.
 * @mount_fn: Mounts a given #xvolume_t.
 *     #xvolume_t implementations must emit the #xmount_operation_t::aborted
 *     signal before completing a mount operation that is aborted while
 *     awaiting input from the user through a #xmount_operation_t instance.
 * @mount_finish: Finishes a mount operation.
 * @eject: Ejects a given #xvolume_t.
 * @eject_finish: Finishes an eject operation.
 * @get_identifier: Returns the [identifier][volume-identifier] of the given kind, or %NULL if
 *    the #xvolume_t doesn't have one.
 * @enumerate_identifiers: Returns an array strings listing the kinds
 *    of [identifiers][volume-identifier] which the #xvolume_t has.
 * @should_automount: Returns %TRUE if the #xvolume_t should be automatically mounted.
 * @get_activation_root: Returns the activation root for the #xvolume_t if it is known in advance or %NULL if
 *   it is not known.
 * @eject_with_operation: Starts ejecting a #xvolume_t using a #xmount_operation_t. Since 2.22.
 * @eject_with_operation_finish: Finishes an eject operation using a #xmount_operation_t. Since 2.22.
 * @get_sort_key: Gets a key used for sorting #xvolume_t instance or %NULL if no such key exists. Since 2.32.
 * @get_symbolic_icon: Gets a symbolic #xicon_t for the #xvolume_t. Since 2.34.
 *
 * Interface for implementing operations for mountable volumes.
 **/
typedef struct _GVolumeIface    GVolumeIface;

struct _GVolumeIface
{
  xtype_interface_t x_iface;

  /* signals */

  void        (* changed)               (xvolume_t             *volume);
  void        (* removed)               (xvolume_t             *volume);

  /* Virtual Table */

  char      * (* get_name)              (xvolume_t             *volume);
  xicon_t     * (* get_icon)              (xvolume_t             *volume);
  char      * (* get_uuid)              (xvolume_t             *volume);
  xdrive_t    * (* get_drive)             (xvolume_t             *volume);
  xmount_t    * (* get_mount)             (xvolume_t             *volume);
  xboolean_t    (* can_mount)             (xvolume_t             *volume);
  xboolean_t    (* can_eject)             (xvolume_t             *volume);
  void        (* mount_fn)              (xvolume_t             *volume,
                                         GMountMountFlags     flags,
                                         xmount_operation_t     *mount_operation,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data);
  xboolean_t    (* mount_finish)          (xvolume_t             *volume,
                                         xasync_result_t        *result,
                                         xerror_t             **error);
  void        (* eject)                 (xvolume_t             *volume,
                                         xmount_unmount_flags_t   flags,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data);
  xboolean_t    (* eject_finish)          (xvolume_t             *volume,
                                         xasync_result_t        *result,
                                         xerror_t             **error);

  char      * (* get_identifier)        (xvolume_t             *volume,
                                         const char          *kind);
  char     ** (* enumerate_identifiers) (xvolume_t             *volume);

  xboolean_t    (* should_automount)      (xvolume_t             *volume);

  xfile_t     * (* get_activation_root)   (xvolume_t             *volume);

  void        (* eject_with_operation)      (xvolume_t             *volume,
                                             xmount_unmount_flags_t   flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* eject_with_operation_finish) (xvolume_t           *volume,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  const xchar_t * (* get_sort_key)        (xvolume_t             *volume);
  xicon_t       * (* get_symbolic_icon)   (xvolume_t             *volume);
};

XPL_AVAILABLE_IN_ALL
xtype_t    g_volume_get_type              (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
char *   g_volume_get_name              (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xicon_t *  g_volume_get_icon              (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xicon_t *  g_volume_get_symbolic_icon     (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
char *   g_volume_get_uuid              (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xdrive_t * g_volume_get_drive             (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xmount_t * g_volume_get_mount             (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xboolean_t g_volume_can_mount             (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xboolean_t g_volume_can_eject             (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
xboolean_t g_volume_should_automount      (xvolume_t              *volume);
XPL_AVAILABLE_IN_ALL
void     g_volume_mount                 (xvolume_t              *volume,
					 GMountMountFlags      flags,
					 xmount_operation_t      *mount_operation,
					 xcancellable_t         *cancellable,
					 xasync_ready_callback_t   callback,
					 xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t g_volume_mount_finish          (xvolume_t              *volume,
					 xasync_result_t         *result,
					 xerror_t              **error);
XPL_DEPRECATED_FOR(g_volume_eject_with_operation)
void     g_volume_eject                 (xvolume_t              *volume,
                                         xmount_unmount_flags_t    flags,
                                         xcancellable_t         *cancellable,
                                         xasync_ready_callback_t   callback,
                                         xpointer_t              user_data);

XPL_DEPRECATED_FOR(g_volume_eject_with_operation_finish)
xboolean_t g_volume_eject_finish          (xvolume_t              *volume,
                                         xasync_result_t         *result,
                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
char *   g_volume_get_identifier        (xvolume_t              *volume,
					 const char           *kind);
XPL_AVAILABLE_IN_ALL
char **  g_volume_enumerate_identifiers (xvolume_t              *volume);

XPL_AVAILABLE_IN_ALL
xfile_t *  g_volume_get_activation_root   (xvolume_t              *volume);

XPL_AVAILABLE_IN_ALL
void        g_volume_eject_with_operation     (xvolume_t             *volume,
                                               xmount_unmount_flags_t   flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_volume_eject_with_operation_finish (xvolume_t          *volume,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_2_32
const xchar_t *g_volume_get_sort_key            (xvolume_t              *volume);

G_END_DECLS

#endif /* __G_VOLUME_H__ */
