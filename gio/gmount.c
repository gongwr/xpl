/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#include "config.h"

#include <string.h>

#include "gmount.h"
#include "gmountprivate.h"
#include "gthemedicon.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gmount
 * @short_description: Mount management
 * @include: gio/gio.h
 * @see_also: xvolume_t, GUnixMountEntry, GUnixMountPoint
 *
 * The #xmount_t interface represents user-visible mounts. Note, when
 * porting from GnomeVFS, #xmount_t is the moral equivalent of #GnomeVFSVolume.
 *
 * #xmount_t is a "mounted" filesystem that you can access. Mounted is in
 * quotes because it's not the same as a unix mount, it might be a gvfs
 * mount, but you can still access the files on it if you use GIO. Might or
 * might not be related to a volume object.
 *
 * Unmounting a #xmount_t instance is an asynchronous operation. For
 * more information about asynchronous operations, see #xasync_result_t
 * and #xtask_t. To unmount a #xmount_t instance, first call
 * g_mount_unmount_with_operation() with (at least) the #xmount_t instance and a
 * #xasync_ready_callback_t.  The callback will be fired when the
 * operation has resolved (either with success or failure), and a
 * #xasync_result_t structure will be passed to the callback.  That
 * callback should then call g_mount_unmount_with_operation_finish() with the #xmount_t
 * and the #xasync_result_t data to see if the operation was completed
 * successfully.  If an @error is present when g_mount_unmount_with_operation_finish()
 * is called, then it will be filled with any error information.
 **/

typedef GMountIface GMountInterface;
G_DEFINE_INTERFACE (xmount, g_mount, XTYPE_OBJECT)

static void
g_mount_default_init (GMountInterface *iface)
{
  /**
   * xmount_t::changed:
   * @mount: the object on which the signal is emitted
   *
   * Emitted when the mount has been changed.
   **/
  xsignal_new (I_("changed"),
                XTYPE_MOUNT,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GMountIface, changed),
                NULL, NULL,
                NULL,
                XTYPE_NONE, 0);

  /**
   * xmount_t::unmounted:
   * @mount: the object on which the signal is emitted
   *
   * This signal is emitted when the #xmount_t have been
   * unmounted. If the recipient is holding references to the
   * object they should release them so the object can be
   * finalized.
   **/
  xsignal_new (I_("unmounted"),
                XTYPE_MOUNT,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GMountIface, unmounted),
                NULL, NULL,
                NULL,
                XTYPE_NONE, 0);
  /**
   * xmount_t::pre-unmount:
   * @mount: the object on which the signal is emitted
   *
   * This signal may be emitted when the #xmount_t is about to be
   * unmounted.
   *
   * This signal depends on the backend and is only emitted if
   * GIO was used to unmount.
   *
   * Since: 2.22
   **/
  xsignal_new (I_("pre-unmount"),
                XTYPE_MOUNT,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (GMountIface, pre_unmount),
                NULL, NULL,
                NULL,
                XTYPE_NONE, 0);
}

/**
 * g_mount_get_root:
 * @mount: a #xmount_t.
 *
 * Gets the root directory on @mount.
 *
 * Returns: (transfer full): a #xfile_t.
 *      The returned object should be unreffed with
 *      xobject_unref() when no longer needed.
 **/
xfile_t *
g_mount_get_root (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->get_root) (mount);
}

/**
 * g_mount_get_default_location:
 * @mount: a #xmount_t.
 *
 * Gets the default location of @mount. The default location of the given
 * @mount is a path that reflects the main entry point for the user (e.g.
 * the home directory, or the root of the volume).
 *
 * Returns: (transfer full): a #xfile_t.
 *      The returned object should be unreffed with
 *      xobject_unref() when no longer needed.
 **/
xfile_t *
g_mount_get_default_location (xmount_t *mount)
{
  GMountIface *iface;
  xfile_t       *file;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  /* Fallback to get_root when default_location () is not available */
  if (iface->get_default_location)
    file = (* iface->get_default_location) (mount);
  else
    file = (* iface->get_root) (mount);

  return file;
}

/**
 * g_mount_get_name:
 * @mount: a #xmount_t.
 *
 * Gets the name of @mount.
 *
 * Returns: the name for the given @mount.
 *     The returned string should be freed with g_free()
 *     when no longer needed.
 **/
char *
g_mount_get_name (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->get_name) (mount);
}

/**
 * g_mount_get_icon:
 * @mount: a #xmount_t.
 *
 * Gets the icon for @mount.
 *
 * Returns: (transfer full): a #xicon_t.
 *      The returned object should be unreffed with
 *      xobject_unref() when no longer needed.
 **/
xicon_t *
g_mount_get_icon (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->get_icon) (mount);
}


/**
 * g_mount_get_symbolic_icon:
 * @mount: a #xmount_t.
 *
 * Gets the symbolic icon for @mount.
 *
 * Returns: (transfer full): a #xicon_t.
 *      The returned object should be unreffed with
 *      xobject_unref() when no longer needed.
 *
 * Since: 2.34
 **/
xicon_t *
g_mount_get_symbolic_icon (xmount_t *mount)
{
  GMountIface *iface;
  xicon_t *ret;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->get_symbolic_icon != NULL)
    ret = iface->get_symbolic_icon (mount);
  else
    ret = g_themed_icon_new_with_default_fallbacks ("folder-remote-symbolic");

  return ret;
}

/**
 * g_mount_get_uuid:
 * @mount: a #xmount_t.
 *
 * Gets the UUID for the @mount. The reference is typically based on
 * the file system UUID for the mount in question and should be
 * considered an opaque string. Returns %NULL if there is no UUID
 * available.
 *
 * Returns: (nullable) (transfer full): the UUID for @mount or %NULL if no UUID
 *     can be computed.
 *     The returned string should be freed with g_free()
 *     when no longer needed.
 **/
char *
g_mount_get_uuid (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->get_uuid) (mount);
}

/**
 * g_mount_get_volume:
 * @mount: a #xmount_t.
 *
 * Gets the volume for the @mount.
 *
 * Returns: (transfer full) (nullable): a #xvolume_t or %NULL if @mount is not
 *      associated with a volume.
 *      The returned object should be unreffed with
 *      xobject_unref() when no longer needed.
 **/
xvolume_t *
g_mount_get_volume (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->get_volume) (mount);
}

/**
 * g_mount_get_drive:
 * @mount: a #xmount_t.
 *
 * Gets the drive for the @mount.
 *
 * This is a convenience method for getting the #xvolume_t and then
 * using that object to get the #xdrive_t.
 *
 * Returns: (transfer full) (nullable): a #xdrive_t or %NULL if @mount is not
 *      associated with a volume or a drive.
 *      The returned object should be unreffed with
 *      xobject_unref() when no longer needed.
 **/
xdrive_t *
g_mount_get_drive (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->get_drive) (mount);
}

/**
 * g_mount_can_unmount:
 * @mount: a #xmount_t.
 *
 * Checks if @mount can be unmounted.
 *
 * Returns: %TRUE if the @mount can be unmounted.
 **/
xboolean_t
g_mount_can_unmount (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->can_unmount) (mount);
}

/**
 * g_mount_can_eject:
 * @mount: a #xmount_t.
 *
 * Checks if @mount can be ejected.
 *
 * Returns: %TRUE if the @mount can be ejected.
 **/
xboolean_t
g_mount_can_eject (xmount_t *mount)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);

  iface = G_MOUNT_GET_IFACE (mount);

  return (* iface->can_eject) (mount);
}

/**
 * g_mount_unmount:
 * @mount: a #xmount_t.
 * @flags: flags affecting the operation
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data passed to @callback.
 *
 * Unmounts a mount. This is an asynchronous operation, and is
 * finished by calling g_mount_unmount_finish() with the @mount
 * and #xasync_result_t data returned in the @callback.
 *
 * Deprecated: 2.22: Use g_mount_unmount_with_operation() instead.
 **/
void
g_mount_unmount (xmount_t              *mount,
                 xmount_unmount_flags_t   flags,
                 xcancellable_t        *cancellable,
                 xasync_ready_callback_t  callback,
                 xpointer_t             user_data)
{
  GMountIface *iface;

  g_return_if_fail (X_IS_MOUNT (mount));

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->unmount == NULL)
    {
      xtask_report_new_error (mount, callback, user_data,
                               g_mount_unmount_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for mount objects that
                                * don't implement unmount. */
                               _("mount doesn’t implement “unmount”"));
      return;
    }

  (* iface->unmount) (mount, flags, cancellable, callback, user_data);
}

/**
 * g_mount_unmount_finish:
 * @mount: a #xmount_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Finishes unmounting a mount. If any errors occurred during the operation,
 * @error will be set to contain the errors and %FALSE will be returned.
 *
 * Returns: %TRUE if the mount was successfully unmounted. %FALSE otherwise.
 *
 * Deprecated: 2.22: Use g_mount_unmount_with_operation_finish() instead.
 **/
xboolean_t
g_mount_unmount_finish (xmount_t        *mount,
                        xasync_result_t  *result,
                        xerror_t       **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, g_mount_unmount_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = G_MOUNT_GET_IFACE (mount);
  return (* iface->unmount_finish) (mount, result, error);
}


/**
 * g_mount_eject:
 * @mount: a #xmount_t.
 * @flags: flags affecting the unmount if required for eject
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data passed to @callback.
 *
 * Ejects a mount. This is an asynchronous operation, and is
 * finished by calling g_mount_eject_finish() with the @mount
 * and #xasync_result_t data returned in the @callback.
 *
 * Deprecated: 2.22: Use g_mount_eject_with_operation() instead.
 **/
void
g_mount_eject (xmount_t              *mount,
               xmount_unmount_flags_t   flags,
               xcancellable_t        *cancellable,
               xasync_ready_callback_t  callback,
               xpointer_t             user_data)
{
  GMountIface *iface;

  g_return_if_fail (X_IS_MOUNT (mount));

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->eject == NULL)
    {
      xtask_report_new_error (mount, callback, user_data,
                               g_mount_eject_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for mount objects that
                                * don't implement eject. */
                               _("mount doesn’t implement “eject”"));
      return;
    }

  (* iface->eject) (mount, flags, cancellable, callback, user_data);
}

/**
 * g_mount_eject_finish:
 * @mount: a #xmount_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Finishes ejecting a mount. If any errors occurred during the operation,
 * @error will be set to contain the errors and %FALSE will be returned.
 *
 * Returns: %TRUE if the mount was successfully ejected. %FALSE otherwise.
 *
 * Deprecated: 2.22: Use g_mount_eject_with_operation_finish() instead.
 **/
xboolean_t
g_mount_eject_finish (xmount_t        *mount,
                      xasync_result_t  *result,
                      xerror_t       **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, g_mount_eject_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = G_MOUNT_GET_IFACE (mount);
  return (* iface->eject_finish) (mount, result, error);
}

/**
 * g_mount_unmount_with_operation:
 * @mount: a #xmount_t.
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t or %NULL to avoid
 *     user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data passed to @callback.
 *
 * Unmounts a mount. This is an asynchronous operation, and is
 * finished by calling g_mount_unmount_with_operation_finish() with the @mount
 * and #xasync_result_t data returned in the @callback.
 *
 * Since: 2.22
 **/
void
g_mount_unmount_with_operation (xmount_t              *mount,
                                xmount_unmount_flags_t   flags,
                                xmount_operation_t     *mount_operation,
                                xcancellable_t        *cancellable,
                                xasync_ready_callback_t  callback,
                                xpointer_t             user_data)
{
  GMountIface *iface;

  g_return_if_fail (X_IS_MOUNT (mount));

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->unmount == NULL && iface->unmount_with_operation == NULL)
    {
      xtask_report_new_error (mount, callback, user_data,
                               g_mount_unmount_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for mount objects that
                                * don't implement any of unmount or unmount_with_operation. */
                               _("mount doesn’t implement “unmount” or “unmount_with_operation”"));
      return;
    }

  if (iface->unmount_with_operation != NULL)
    (* iface->unmount_with_operation) (mount, flags, mount_operation, cancellable, callback, user_data);
  else
    (* iface->unmount) (mount, flags, cancellable, callback, user_data);
}

/**
 * g_mount_unmount_with_operation_finish:
 * @mount: a #xmount_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Finishes unmounting a mount. If any errors occurred during the operation,
 * @error will be set to contain the errors and %FALSE will be returned.
 *
 * Returns: %TRUE if the mount was successfully unmounted. %FALSE otherwise.
 *
 * Since: 2.22
 **/
xboolean_t
g_mount_unmount_with_operation_finish (xmount_t        *mount,
                                       xasync_result_t  *result,
                                       xerror_t       **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, g_mount_unmount_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = G_MOUNT_GET_IFACE (mount);
  if (iface->unmount_with_operation_finish != NULL)
    return (* iface->unmount_with_operation_finish) (mount, result, error);
  else
    return (* iface->unmount_finish) (mount, result, error);
}


/**
 * g_mount_eject_with_operation:
 * @mount: a #xmount_t.
 * @flags: flags affecting the unmount if required for eject
 * @mount_operation: (nullable): a #xmount_operation_t or %NULL to avoid
 *     user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data passed to @callback.
 *
 * Ejects a mount. This is an asynchronous operation, and is
 * finished by calling g_mount_eject_with_operation_finish() with the @mount
 * and #xasync_result_t data returned in the @callback.
 *
 * Since: 2.22
 **/
void
g_mount_eject_with_operation (xmount_t              *mount,
                              xmount_unmount_flags_t   flags,
                              xmount_operation_t     *mount_operation,
                              xcancellable_t        *cancellable,
                              xasync_ready_callback_t  callback,
                              xpointer_t             user_data)
{
  GMountIface *iface;

  g_return_if_fail (X_IS_MOUNT (mount));

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->eject == NULL && iface->eject_with_operation == NULL)
    {
      xtask_report_new_error (mount, callback, user_data,
                               g_mount_eject_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for mount objects that
                                * don't implement any of eject or eject_with_operation. */
                               _("mount doesn’t implement “eject” or “eject_with_operation”"));
      return;
    }

  if (iface->eject_with_operation != NULL)
    (* iface->eject_with_operation) (mount, flags, mount_operation, cancellable, callback, user_data);
  else
    (* iface->eject) (mount, flags, cancellable, callback, user_data);
}

/**
 * g_mount_eject_with_operation_finish:
 * @mount: a #xmount_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Finishes ejecting a mount. If any errors occurred during the operation,
 * @error will be set to contain the errors and %FALSE will be returned.
 *
 * Returns: %TRUE if the mount was successfully ejected. %FALSE otherwise.
 *
 * Since: 2.22
 **/
xboolean_t
g_mount_eject_with_operation_finish (xmount_t        *mount,
                                     xasync_result_t  *result,
                                     xerror_t       **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, g_mount_eject_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = G_MOUNT_GET_IFACE (mount);
  if (iface->eject_with_operation_finish != NULL)
    return (* iface->eject_with_operation_finish) (mount, result, error);
  else
    return (* iface->eject_finish) (mount, result, error);
}

/**
 * g_mount_remount:
 * @mount: a #xmount_t.
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t or %NULL to avoid
 *     user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data passed to @callback.
 *
 * Remounts a mount. This is an asynchronous operation, and is
 * finished by calling g_mount_remount_finish() with the @mount
 * and #GAsyncResults data returned in the @callback.
 *
 * Remounting is useful when some setting affecting the operation
 * of the volume has been changed, as these may need a remount to
 * take affect. While this is semantically equivalent with unmounting
 * and then remounting not all backends might need to actually be
 * unmounted.
 **/
void
g_mount_remount (xmount_t              *mount,
                 GMountMountFlags     flags,
                 xmount_operation_t     *mount_operation,
                 xcancellable_t        *cancellable,
                 xasync_ready_callback_t  callback,
                 xpointer_t             user_data)
{
  GMountIface *iface;

  g_return_if_fail (X_IS_MOUNT (mount));

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->remount == NULL)
    {
      xtask_report_new_error (mount, callback, user_data,
                               g_mount_remount,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for mount objects that
                                * don't implement remount. */
                               _("mount doesn’t implement “remount”"));
      return;
    }

  (* iface->remount) (mount, flags, mount_operation, cancellable, callback, user_data);
}

/**
 * g_mount_remount_finish:
 * @mount: a #xmount_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Finishes remounting a mount. If any errors occurred during the operation,
 * @error will be set to contain the errors and %FALSE will be returned.
 *
 * Returns: %TRUE if the mount was successfully remounted. %FALSE otherwise.
 **/
xboolean_t
g_mount_remount_finish (xmount_t        *mount,
                        xasync_result_t  *result,
                        xerror_t       **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, g_mount_remount))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = G_MOUNT_GET_IFACE (mount);
  return (* iface->remount_finish) (mount, result, error);
}

/**
 * g_mount_guess_content_type:
 * @mount: a #xmount_t
 * @force_rescan: Whether to force a rescan of the content.
 *     Otherwise a cached result will be used if available
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @callback: a #xasync_ready_callback_t
 * @user_data: user data passed to @callback
 *
 * Tries to guess the type of content stored on @mount. Returns one or
 * more textual identifiers of well-known content types (typically
 * prefixed with "x-content/"), e.g. x-content/image-dcf for camera
 * memory cards. See the
 * [shared-mime-info](http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec)
 * specification for more on x-content types.
 *
 * This is an asynchronous operation (see
 * g_mount_guess_content_type_sync() for the synchronous version), and
 * is finished by calling g_mount_guess_content_type_finish() with the
 * @mount and #xasync_result_t data returned in the @callback.
 *
 * Since: 2.18
 */
void
g_mount_guess_content_type (xmount_t              *mount,
                            xboolean_t             force_rescan,
                            xcancellable_t        *cancellable,
                            xasync_ready_callback_t  callback,
                            xpointer_t             user_data)
{
  GMountIface *iface;

  g_return_if_fail (X_IS_MOUNT (mount));

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->guess_content_type == NULL)
    {
      xtask_report_new_error (mount, callback, user_data,
                               g_mount_guess_content_type,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for mount objects that
                                * don't implement content type guessing. */
                               _("mount doesn’t implement content type guessing"));
      return;
    }

  (* iface->guess_content_type) (mount, force_rescan, cancellable, callback, user_data);
}

/**
 * g_mount_guess_content_type_finish:
 * @mount: a #xmount_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore
 *
 * Finishes guessing content types of @mount. If any errors occurred
 * during the operation, @error will be set to contain the errors and
 * %FALSE will be returned. In particular, you may get an
 * %G_IO_ERROR_NOT_SUPPORTED if the mount does not support content
 * guessing.
 *
 * Returns: (transfer full) (element-type utf8): a %NULL-terminated array of content types or %NULL on error.
 *     Caller should free this array with xstrfreev() when done with it.
 *
 * Since: 2.18
 **/
xchar_t **
g_mount_guess_content_type_finish (xmount_t        *mount,
                                   xasync_result_t  *result,
                                   xerror_t       **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, g_mount_guess_content_type))
    return xtask_propagate_pointer (XTASK (result), error);

  iface = G_MOUNT_GET_IFACE (mount);
  return (* iface->guess_content_type_finish) (mount, result, error);
}

/**
 * g_mount_guess_content_type_sync:
 * @mount: a #xmount_t
 * @force_rescan: Whether to force a rescan of the content.
 *     Otherwise a cached result will be used if available
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore
 *
 * Tries to guess the type of content stored on @mount. Returns one or
 * more textual identifiers of well-known content types (typically
 * prefixed with "x-content/"), e.g. x-content/image-dcf for camera
 * memory cards. See the
 * [shared-mime-info](http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec)
 * specification for more on x-content types.
 *
 * This is a synchronous operation and as such may block doing IO;
 * see g_mount_guess_content_type() for the asynchronous version.
 *
 * Returns: (transfer full) (element-type utf8): a %NULL-terminated array of content types or %NULL on error.
 *     Caller should free this array with xstrfreev() when done with it.
 *
 * Since: 2.18
 */
char **
g_mount_guess_content_type_sync (xmount_t              *mount,
                                 xboolean_t             force_rescan,
                                 xcancellable_t        *cancellable,
                                 xerror_t             **error)
{
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);

  if (iface->guess_content_type_sync == NULL)
    {
      g_set_error_literal (error,
                           G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           /* Translators: This is an error
                            * message for mount objects that
                            * don't implement content type guessing. */
                           _("mount doesn’t implement synchronous content type guessing"));

      return NULL;
    }

  return (* iface->guess_content_type_sync) (mount, force_rescan, cancellable, error);
}

G_LOCK_DEFINE_STATIC (priv_lock);

/* only access this structure when holding priv_lock */
typedef struct
{
  xint_t shadow_ref_count;
} GMountPrivate;

static void
free_private (GMountPrivate *private)
{
  G_LOCK (priv_lock);
  g_free (private);
  G_UNLOCK (priv_lock);
}

/* may only be called when holding priv_lock */
static GMountPrivate *
get_private (xmount_t *mount)
{
  GMountPrivate *private;

  private = xobject_get_data (G_OBJECT (mount), "g-mount-private");
  if (G_LIKELY (private != NULL))
    goto out;

  private = g_new0 (GMountPrivate, 1);
  xobject_set_data_full (G_OBJECT (mount),
                          "g-mount-private",
                          private,
                          (xdestroy_notify_t) free_private);

 out:
  return private;
}

/**
 * g_mount_is_shadowed:
 * @mount: A #xmount_t.
 *
 * Determines if @mount is shadowed. Applications or libraries should
 * avoid displaying @mount in the user interface if it is shadowed.
 *
 * A mount is said to be shadowed if there exists one or more user
 * visible objects (currently #xmount_t objects) with a root that is
 * inside the root of @mount.
 *
 * One application of shadow mounts is when exposing a single file
 * system that is used to address several logical volumes. In this
 * situation, a #xvolume_monitor_t implementation would create two
 * #xvolume_t objects (for example, one for the camera functionality of
 * the device and one for a SD card reader on the device) with
 * activation URIs `gphoto2://[usb:001,002]/store1/`
 * and `gphoto2://[usb:001,002]/store2/`. When the
 * underlying mount (with root
 * `gphoto2://[usb:001,002]/`) is mounted, said
 * #xvolume_monitor_t implementation would create two #xmount_t objects
 * (each with their root matching the corresponding volume activation
 * root) that would shadow the original mount.
 *
 * The proxy monitor in xvfs_t 2.26 and later, automatically creates and
 * manage shadow mounts (and shadows the underlying mount) if the
 * activation root on a #xvolume_t is set.
 *
 * Returns: %TRUE if @mount is shadowed.
 *
 * Since: 2.20
 **/
xboolean_t
g_mount_is_shadowed (xmount_t *mount)
{
  GMountPrivate *priv;
  xboolean_t ret;

  g_return_val_if_fail (X_IS_MOUNT (mount), FALSE);

  G_LOCK (priv_lock);
  priv = get_private (mount);
  ret = (priv->shadow_ref_count > 0);
  G_UNLOCK (priv_lock);

  return ret;
}

/**
 * g_mount_shadow:
 * @mount: A #xmount_t.
 *
 * Increments the shadow count on @mount. Usually used by
 * #xvolume_monitor_t implementations when creating a shadow mount for
 * @mount, see g_mount_is_shadowed() for more information. The caller
 * will need to emit the #xmount_t::changed signal on @mount manually.
 *
 * Since: 2.20
 **/
void
g_mount_shadow (xmount_t *mount)
{
  GMountPrivate *priv;

  g_return_if_fail (X_IS_MOUNT (mount));

  G_LOCK (priv_lock);
  priv = get_private (mount);
  priv->shadow_ref_count += 1;
  G_UNLOCK (priv_lock);
}

/**
 * g_mount_unshadow:
 * @mount: A #xmount_t.
 *
 * Decrements the shadow count on @mount. Usually used by
 * #xvolume_monitor_t implementations when destroying a shadow mount for
 * @mount, see g_mount_is_shadowed() for more information. The caller
 * will need to emit the #xmount_t::changed signal on @mount manually.
 *
 * Since: 2.20
 **/
void
g_mount_unshadow (xmount_t *mount)
{
  GMountPrivate *priv;

  g_return_if_fail (X_IS_MOUNT (mount));

  G_LOCK (priv_lock);
  priv = get_private (mount);
  priv->shadow_ref_count -= 1;
  if (priv->shadow_ref_count < 0)
    g_warning ("Shadow ref count on xmount_t is negative");
  G_UNLOCK (priv_lock);
}

/**
 * g_mount_get_sort_key:
 * @mount: A #xmount_t.
 *
 * Gets the sort key for @mount, if any.
 *
 * Returns: (nullable): Sorting key for @mount or %NULL if no such key is available.
 *
 * Since: 2.32
 */
const xchar_t *
g_mount_get_sort_key (xmount_t  *mount)
{
  const xchar_t *ret = NULL;
  GMountIface *iface;

  g_return_val_if_fail (X_IS_MOUNT (mount), NULL);

  iface = G_MOUNT_GET_IFACE (mount);
  if (iface->get_sort_key != NULL)
    ret = iface->get_sort_key (mount);

  return ret;
}
