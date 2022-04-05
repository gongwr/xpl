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

#ifndef __XDRIVE_H__
#define __XDRIVE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * XDRIVE_IDENTIFIER_KIND_UNIX_DEVICE:
 *
 * The string used to obtain a Unix device path with xdrive_get_identifier().
 *
 * Since: 2.58
 */
#define XDRIVE_IDENTIFIER_KIND_UNIX_DEVICE "unix-device"

#define XTYPE_DRIVE           (xdrive_get_type ())
#define G_DRIVE(obj)           (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_DRIVE, xdrive_t))
#define X_IS_DRIVE(obj)        (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_DRIVE))
#define XDRIVE_GET_IFACE(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_DRIVE, xdrive_iface_t))

/**
 * xdrive_iface_t:
 * @x_iface: The parent interface.
 * @changed: Signal emitted when the drive is changed.
 * @disconnected: The removed signal that is emitted when the #xdrive_t have been disconnected. If the recipient is holding references to the object they should release them so the object can be finalized.
 * @eject_button: Signal emitted when the physical eject button (if any) of a drive have been pressed.
 * @get_name: Returns the name for the given #xdrive_t.
 * @get_icon: Returns a #xicon_t for the given #xdrive_t.
 * @has_volumes: Returns %TRUE if the #xdrive_t has mountable volumes.
 * @get_volumes: Returns a list #xlist_t of #GVolume for the #xdrive_t.
 * @is_removable: Returns %TRUE if the #xdrive_t and/or its media is considered removable by the user. Since 2.50.
 * @is_media_removable: Returns %TRUE if the #xdrive_t supports removal and insertion of media.
 * @has_media: Returns %TRUE if the #xdrive_t has media inserted.
 * @is_media_check_automatic: Returns %TRUE if the #xdrive_t is capable of automatically detecting media changes.
 * @can_poll_for_media: Returns %TRUE if the #xdrive_t is capable of manually polling for media change.
 * @can_eject: Returns %TRUE if the #xdrive_t can eject media.
 * @eject: Ejects a #xdrive_t.
 * @eject_finish: Finishes an eject operation.
 * @poll_for_media: Poll for media insertion/removal on a #xdrive_t.
 * @poll_for_media_finish: Finishes a media poll operation.
 * @get_identifier: Returns the identifier of the given kind, or %NULL if
 *    the #xdrive_t doesn't have one.
 * @enumerate_identifiers: Returns an array strings listing the kinds
 *    of identifiers which the #xdrive_t has.
 * @get_start_stop_type: Gets a #GDriveStartStopType with details about starting/stopping the drive. Since 2.22.
 * @can_stop: Returns %TRUE if a #xdrive_t can be stopped. Since 2.22.
 * @stop: Stops a #xdrive_t. Since 2.22.
 * @stop_finish: Finishes a stop operation. Since 2.22.
 * @can_start: Returns %TRUE if a #xdrive_t can be started. Since 2.22.
 * @can_start_degraded: Returns %TRUE if a #xdrive_t can be started degraded. Since 2.22.
 * @start: Starts a #xdrive_t. Since 2.22.
 * @start_finish: Finishes a start operation. Since 2.22.
 * @stop_button: Signal emitted when the physical stop button (if any) of a drive have been pressed. Since 2.22.
 * @eject_with_operation: Starts ejecting a #xdrive_t using a #xmount_operation_t. Since 2.22.
 * @eject_with_operation_finish: Finishes an eject operation using a #xmount_operation_t. Since 2.22.
 * @get_sort_key: Gets a key used for sorting #xdrive_t instances or %NULL if no such key exists. Since 2.32.
 * @get_symbolic_icon: Returns a symbolic #xicon_t for the given #xdrive_t. Since 2.34.
 *
 * Interface for creating #xdrive_t implementations.
 */
typedef struct _xdrive_iface    xdrive_iface_t;

struct _xdrive_iface
{
  xtype_interface_t x_iface;

  /* signals */
  void     (* changed)                  (xdrive_t              *drive);
  void     (* disconnected)             (xdrive_t              *drive);
  void     (* eject_button)             (xdrive_t              *drive);

  /* Virtual Table */
  char *   (* get_name)                 (xdrive_t              *drive);
  xicon_t *  (* get_icon)                 (xdrive_t              *drive);
  xboolean_t (* has_volumes)              (xdrive_t              *drive);
  xlist_t *  (* get_volumes)              (xdrive_t              *drive);
  xboolean_t (* is_media_removable)       (xdrive_t              *drive);
  xboolean_t (* has_media)                (xdrive_t              *drive);
  xboolean_t (* is_media_check_automatic) (xdrive_t              *drive);
  xboolean_t (* can_eject)                (xdrive_t              *drive);
  xboolean_t (* can_poll_for_media)       (xdrive_t              *drive);
  void     (* eject)                    (xdrive_t              *drive,
                                         xmount_unmount_flags_t   flags,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data);
  xboolean_t (* eject_finish)             (xdrive_t              *drive,
                                         xasync_result_t        *result,
                                         xerror_t             **error);
  void     (* poll_for_media)           (xdrive_t              *drive,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data);
  xboolean_t (* poll_for_media_finish)    (xdrive_t              *drive,
                                         xasync_result_t        *result,
                                         xerror_t             **error);

  char *   (* get_identifier)           (xdrive_t              *drive,
                                         const char          *kind);
  char **  (* enumerate_identifiers)    (xdrive_t              *drive);

  GDriveStartStopType (* get_start_stop_type) (xdrive_t        *drive);

  xboolean_t (* can_start)                (xdrive_t              *drive);
  xboolean_t (* can_start_degraded)       (xdrive_t              *drive);
  void     (* start)                    (xdrive_t              *drive,
                                         GDriveStartFlags     flags,
                                         xmount_operation_t     *mount_operation,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data);
  xboolean_t (* start_finish)             (xdrive_t              *drive,
                                         xasync_result_t        *result,
                                         xerror_t             **error);

  xboolean_t (* can_stop)                 (xdrive_t              *drive);
  void     (* stop)                     (xdrive_t              *drive,
                                         xmount_unmount_flags_t   flags,
                                         xmount_operation_t     *mount_operation,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data);
  xboolean_t (* stop_finish)              (xdrive_t              *drive,
                                         xasync_result_t        *result,
                                         xerror_t             **error);
  /* signal, not VFunc */
  void     (* stop_button)              (xdrive_t              *drive);

  void        (* eject_with_operation)      (xdrive_t              *drive,
                                             xmount_unmount_flags_t   flags,
                                             xmount_operation_t     *mount_operation,
                                             xcancellable_t        *cancellable,
                                             xasync_ready_callback_t  callback,
                                             xpointer_t             user_data);
  xboolean_t    (* eject_with_operation_finish) (xdrive_t            *drive,
                                             xasync_result_t        *result,
                                             xerror_t             **error);

  const xchar_t * (* get_sort_key)        (xdrive_t              *drive);
  xicon_t *       (* get_symbolic_icon)   (xdrive_t              *drive);
  xboolean_t      (* is_removable)        (xdrive_t              *drive);

};

XPL_AVAILABLE_IN_ALL
xtype_t    xdrive_get_type                 (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
char *   xdrive_get_name                 (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xicon_t *  xdrive_get_icon                 (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xicon_t *  xdrive_get_symbolic_icon        (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_has_volumes              (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xlist_t *  xdrive_get_volumes              (xdrive_t               *drive);
XPL_AVAILABLE_IN_2_50
xboolean_t xdrive_is_removable             (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_is_media_removable       (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_has_media                (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_is_media_check_automatic (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_can_poll_for_media       (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_can_eject                (xdrive_t               *drive);
XPL_DEPRECATED_FOR(xdrive_eject_with_operation)
void     xdrive_eject                    (xdrive_t               *drive,
                                           xmount_unmount_flags_t    flags,
                                           xcancellable_t         *cancellable,
                                           xasync_ready_callback_t   callback,
                                           xpointer_t              user_data);

XPL_DEPRECATED_FOR(xdrive_eject_with_operation_finish)
xboolean_t xdrive_eject_finish             (xdrive_t               *drive,
                                           xasync_result_t         *result,
                                           xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void     xdrive_poll_for_media           (xdrive_t               *drive,
                                           xcancellable_t         *cancellable,
                                           xasync_ready_callback_t   callback,
                                           xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_poll_for_media_finish    (xdrive_t               *drive,
                                           xasync_result_t         *result,
                                           xerror_t              **error);
XPL_AVAILABLE_IN_ALL
char *   xdrive_get_identifier           (xdrive_t              *drive,
                                           const char          *kind);
XPL_AVAILABLE_IN_ALL
char **  xdrive_enumerate_identifiers    (xdrive_t              *drive);

XPL_AVAILABLE_IN_ALL
GDriveStartStopType xdrive_get_start_stop_type (xdrive_t        *drive);

XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_can_start                (xdrive_t              *drive);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_can_start_degraded       (xdrive_t              *drive);
XPL_AVAILABLE_IN_ALL
void     xdrive_start                    (xdrive_t              *drive,
                                           GDriveStartFlags     flags,
                                           xmount_operation_t     *mount_operation,
                                           xcancellable_t        *cancellable,
                                           xasync_ready_callback_t  callback,
                                           xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_start_finish             (xdrive_t               *drive,
                                           xasync_result_t         *result,
                                           xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_can_stop                 (xdrive_t               *drive);
XPL_AVAILABLE_IN_ALL
void     xdrive_stop                     (xdrive_t               *drive,
                                           xmount_unmount_flags_t    flags,
                                           xmount_operation_t      *mount_operation,
                                           xcancellable_t         *cancellable,
                                           xasync_ready_callback_t   callback,
                                           xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xdrive_stop_finish              (xdrive_t               *drive,
                                           xasync_result_t         *result,
                                           xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void        xdrive_eject_with_operation      (xdrive_t              *drive,
                                               xmount_unmount_flags_t   flags,
                                               xmount_operation_t     *mount_operation,
                                               xcancellable_t        *cancellable,
                                               xasync_ready_callback_t  callback,
                                               xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t    xdrive_eject_with_operation_finish (xdrive_t            *drive,
                                               xasync_result_t        *result,
                                               xerror_t             **error);

XPL_AVAILABLE_IN_2_32
const xchar_t *xdrive_get_sort_key         (xdrive_t               *drive);

G_END_DECLS

#endif /* __XDRIVE_H__ */
