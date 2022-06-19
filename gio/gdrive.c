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
#include "gdrive.h"
#include "gtask.h"
#include "gthemedicon.h"
#include "gasyncresult.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gdrive
 * @short_description: Drive management
 * @include: gio/gio.h
 *
 * #xdrive_t - this represent a piece of hardware connected to the machine.
 * It's generally only created for removable hardware or hardware with
 * removable media.
 *
 * #xdrive_t is a container class for #xvolume_t objects that stem from
 * the same piece of media. As such, #xdrive_t abstracts a drive with
 * (or without) removable media and provides operations for querying
 * whether media is available, determining whether media change is
 * automatically detected and ejecting the media.
 *
 * If the #xdrive_t reports that media isn't automatically detected, one
 * can poll for media; typically one should not do this periodically
 * as a poll for media operation is potentially expensive and may
 * spin up the drive creating noise.
 *
 * #xdrive_t supports starting and stopping drives with authentication
 * support for the former. This can be used to support a diverse set
 * of use cases including connecting/disconnecting iSCSI devices,
 * powering down external disk enclosures and starting/stopping
 * multi-disk devices such as RAID devices. Note that the actual
 * semantics and side-effects of starting/stopping a #xdrive_t may vary
 * according to implementation. To choose the correct verbs in e.g. a
 * file manager, use xdrive_get_start_stop_type().
 *
 * For porting from GnomeVFS note that there is no equivalent of
 * #xdrive_t in that API.
 **/

typedef xdrive_iface_t xdrive_interface_t;
G_DEFINE_INTERFACE(xdrive, xdrive, XTYPE_OBJECT)

static void
xdrive_default_init (xdrive_interface_t *iface)
{
  /**
   * xdrive_t::changed:
   * @drive: a #xdrive_t.
   *
   * Emitted when the drive's state has changed.
   **/
  xsignal_new (I_("changed"),
		XTYPE_DRIVE,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (xdrive_iface_t, changed),
		NULL, NULL,
		NULL,
		XTYPE_NONE, 0);

  /**
   * xdrive_t::disconnected:
   * @drive: a #xdrive_t.
   *
   * This signal is emitted when the #xdrive_t have been
   * disconnected. If the recipient is holding references to the
   * object they should release them so the object can be
   * finalized.
   **/
  xsignal_new (I_("disconnected"),
		XTYPE_DRIVE,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (xdrive_iface_t, disconnected),
		NULL, NULL,
		NULL,
		XTYPE_NONE, 0);

  /**
   * xdrive_t::eject-button:
   * @drive: a #xdrive_t.
   *
   * Emitted when the physical eject button (if any) of a drive has
   * been pressed.
   **/
  xsignal_new (I_("eject-button"),
		XTYPE_DRIVE,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (xdrive_iface_t, eject_button),
		NULL, NULL,
		NULL,
		XTYPE_NONE, 0);

  /**
   * xdrive_t::stop-button:
   * @drive: a #xdrive_t.
   *
   * Emitted when the physical stop button (if any) of a drive has
   * been pressed.
   *
   * Since: 2.22
   **/
  xsignal_new (I_("stop-button"),
		XTYPE_DRIVE,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (xdrive_iface_t, stop_button),
		NULL, NULL,
		NULL,
		XTYPE_NONE, 0);
}

/**
 * xdrive_get_name:
 * @drive: a #xdrive_t.
 *
 * Gets the name of @drive.
 *
 * Returns: a string containing @drive's name. The returned
 *     string should be freed when no longer needed.
 **/
char *
xdrive_get_name (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->get_name) (drive);
}

/**
 * xdrive_get_icon:
 * @drive: a #xdrive_t.
 *
 * Gets the icon for @drive.
 *
 * Returns: (transfer full): #xicon_t for the @drive.
 *    Free the returned object with xobject_unref().
 **/
xicon_t *
xdrive_get_icon (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->get_icon) (drive);
}

/**
 * xdrive_get_symbolic_icon:
 * @drive: a #xdrive_t.
 *
 * Gets the icon for @drive.
 *
 * Returns: (transfer full): symbolic #xicon_t for the @drive.
 *    Free the returned object with xobject_unref().
 *
 * Since: 2.34
 **/
xicon_t *
xdrive_get_symbolic_icon (xdrive_t *drive)
{
  xdrive_iface_t *iface;
  xicon_t *ret;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->get_symbolic_icon != NULL)
    ret = iface->get_symbolic_icon (drive);
  else
    ret = g_themed_icon_new_with_default_fallbacks ("drive-removable-media-symbolic");

  return ret;
}

/**
 * xdrive_has_volumes:
 * @drive: a #xdrive_t.
 *
 * Check if @drive has any mountable volumes.
 *
 * Returns: %TRUE if the @drive contains volumes, %FALSE otherwise.
 **/
xboolean_t
xdrive_has_volumes (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->has_volumes) (drive);
}

/**
 * xdrive_get_volumes:
 * @drive: a #xdrive_t.
 *
 * Get a list of mountable volumes for @drive.
 *
 * The returned list should be freed with xlist_free(), after
 * its elements have been unreffed with xobject_unref().
 *
 * Returns: (element-type xvolume_t) (transfer full): #xlist_t containing any #xvolume_t objects on the given @drive.
 **/
xlist_t *
xdrive_get_volumes (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->get_volumes) (drive);
}

/**
 * xdrive_is_media_check_automatic:
 * @drive: a #xdrive_t.
 *
 * Checks if @drive is capable of automatically detecting media changes.
 *
 * Returns: %TRUE if the @drive is capable of automatically detecting
 *     media changes, %FALSE otherwise.
 **/
xboolean_t
xdrive_is_media_check_automatic (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->is_media_check_automatic) (drive);
}

/**
 * xdrive_is_removable:
 * @drive: a #xdrive_t.
 *
 * Checks if the #xdrive_t and/or its media is considered removable by the user.
 * See xdrive_is_media_removable().
 *
 * Returns: %TRUE if @drive and/or its media is considered removable, %FALSE otherwise.
 *
 * Since: 2.50
 **/
xboolean_t
xdrive_is_removable (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);
  if (iface->is_removable != NULL)
    return iface->is_removable (drive);

  return FALSE;
}

/**
 * xdrive_is_media_removable:
 * @drive: a #xdrive_t.
 *
 * Checks if the @drive supports removable media.
 *
 * Returns: %TRUE if @drive supports removable media, %FALSE otherwise.
 **/
xboolean_t
xdrive_is_media_removable (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->is_media_removable) (drive);
}

/**
 * xdrive_has_media:
 * @drive: a #xdrive_t.
 *
 * Checks if the @drive has media. Note that the OS may not be polling
 * the drive for media changes; see xdrive_is_media_check_automatic()
 * for more details.
 *
 * Returns: %TRUE if @drive has media, %FALSE otherwise.
 **/
xboolean_t
xdrive_has_media (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->has_media) (drive);
}

/**
 * xdrive_can_eject:
 * @drive: a #xdrive_t.
 *
 * Checks if a drive can be ejected.
 *
 * Returns: %TRUE if the @drive can be ejected, %FALSE otherwise.
 **/
xboolean_t
xdrive_can_eject (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->can_eject == NULL)
    return FALSE;

  return (* iface->can_eject) (drive);
}

/**
 * xdrive_can_poll_for_media:
 * @drive: a #xdrive_t.
 *
 * Checks if a drive can be polled for media changes.
 *
 * Returns: %TRUE if the @drive can be polled for media changes,
 *     %FALSE otherwise.
 **/
xboolean_t
xdrive_can_poll_for_media (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->poll_for_media == NULL)
    return FALSE;

  return (* iface->can_poll_for_media) (drive);
}

/**
 * xdrive_eject:
 * @drive: a #xdrive_t.
 * @flags: flags affecting the unmount if required for eject
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data to pass to @callback
 *
 * Asynchronously ejects a drive.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xdrive_eject_finish() to obtain the
 * result of the operation.
 *
 * Deprecated: 2.22: Use xdrive_eject_with_operation() instead.
 **/
void
xdrive_eject (xdrive_t              *drive,
	       xmount_unmount_flags_t   flags,
	       xcancellable_t        *cancellable,
	       xasync_ready_callback_t  callback,
	       xpointer_t             user_data)
{
  xdrive_iface_t *iface;

  g_return_if_fail (X_IS_DRIVE (drive));

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->eject == NULL)
    {
      xtask_report_new_error (drive, callback, user_data,
                               xdrive_eject_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("drive doesn’t implement eject"));
      return;
    }

  (* iface->eject) (drive, flags, cancellable, callback, user_data);
}

/**
 * xdrive_eject_finish:
 * @drive: a #xdrive_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, or %NULL
 *
 * Finishes ejecting a drive.
 *
 * Returns: %TRUE if the drive has been ejected successfully,
 *     %FALSE otherwise.
 *
 * Deprecated: 2.22: Use xdrive_eject_with_operation_finish() instead.
 **/
xboolean_t
xdrive_eject_finish (xdrive_t        *drive,
		      xasync_result_t  *result,
		      xerror_t       **error)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);
  xreturn_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xdrive_eject_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->eject_finish) (drive, result, error);
}

/**
 * xdrive_eject_with_operation:
 * @drive: a #xdrive_t.
 * @flags: flags affecting the unmount if required for eject
 * @mount_operation: (nullable): a #xmount_operation_t or %NULL to avoid
 *     user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data passed to @callback.
 *
 * Ejects a drive. This is an asynchronous operation, and is
 * finished by calling xdrive_eject_with_operation_finish() with the @drive
 * and #xasync_result_t data returned in the @callback.
 *
 * Since: 2.22
 **/
void
xdrive_eject_with_operation (xdrive_t              *drive,
                              xmount_unmount_flags_t   flags,
                              xmount_operation_t     *mount_operation,
                              xcancellable_t        *cancellable,
                              xasync_ready_callback_t  callback,
                              xpointer_t             user_data)
{
  xdrive_iface_t *iface;

  g_return_if_fail (X_IS_DRIVE (drive));

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->eject == NULL && iface->eject_with_operation == NULL)
    {
      xtask_report_new_error (drive, callback, user_data,
                               xdrive_eject_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               /* Translators: This is an error
                                * message for drive objects that
                                * don't implement any of eject or eject_with_operation. */
                               _("drive doesn’t implement eject or eject_with_operation"));
      return;
    }

  if (iface->eject_with_operation != NULL)
    (* iface->eject_with_operation) (drive, flags, mount_operation, cancellable, callback, user_data);
  else
    (* iface->eject) (drive, flags, cancellable, callback, user_data);
}

/**
 * xdrive_eject_with_operation_finish:
 * @drive: a #xdrive_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Finishes ejecting a drive. If any errors occurred during the operation,
 * @error will be set to contain the errors and %FALSE will be returned.
 *
 * Returns: %TRUE if the drive was successfully ejected. %FALSE otherwise.
 *
 * Since: 2.22
 **/
xboolean_t
xdrive_eject_with_operation_finish (xdrive_t        *drive,
                                     xasync_result_t  *result,
                                     xerror_t       **error)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);
  xreturn_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xdrive_eject_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XDRIVE_GET_IFACE (drive);
  if (iface->eject_with_operation_finish != NULL)
    return (* iface->eject_with_operation_finish) (drive, result, error);
  else
    return (* iface->eject_finish) (drive, result, error);
}

/**
 * xdrive_poll_for_media:
 * @drive: a #xdrive_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data to pass to @callback
 *
 * Asynchronously polls @drive to see if media has been inserted or removed.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xdrive_poll_for_media_finish() to obtain the
 * result of the operation.
 **/
void
xdrive_poll_for_media (xdrive_t              *drive,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xdrive_iface_t *iface;

  g_return_if_fail (X_IS_DRIVE (drive));

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->poll_for_media == NULL)
    {
      xtask_report_new_error (drive, callback, user_data,
                               xdrive_poll_for_media,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("drive doesn’t implement polling for media"));
      return;
    }

  (* iface->poll_for_media) (drive, cancellable, callback, user_data);
}

/**
 * xdrive_poll_for_media_finish:
 * @drive: a #xdrive_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an operation started with xdrive_poll_for_media() on a drive.
 *
 * Returns: %TRUE if the drive has been poll_for_mediaed successfully,
 *     %FALSE otherwise.
 **/
xboolean_t
xdrive_poll_for_media_finish (xdrive_t        *drive,
                               xasync_result_t  *result,
                               xerror_t       **error)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);
  xreturn_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xdrive_poll_for_media))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->poll_for_media_finish) (drive, result, error);
}

/**
 * xdrive_get_identifier:
 * @drive: a #xdrive_t
 * @kind: the kind of identifier to return
 *
 * Gets the identifier of the given kind for @drive. The only
 * identifier currently available is
 * %XDRIVE_IDENTIFIER_KIND_UNIX_DEVICE.
 *
 * Returns: (nullable) (transfer full): a newly allocated string containing the
 *     requested identifier, or %NULL if the #xdrive_t
 *     doesn't have this kind of identifier.
 */
char *
xdrive_get_identifier (xdrive_t     *drive,
			const char *kind)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);
  xreturn_val_if_fail (kind != NULL, NULL);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->get_identifier == NULL)
    return NULL;

  return (* iface->get_identifier) (drive, kind);
}

/**
 * xdrive_enumerate_identifiers:
 * @drive: a #xdrive_t
 *
 * Gets the kinds of identifiers that @drive has.
 * Use xdrive_get_identifier() to obtain the identifiers
 * themselves.
 *
 * Returns: (transfer full) (array zero-terminated=1): a %NULL-terminated
 *     array of strings containing kinds of identifiers. Use xstrfreev()
 *     to free.
 */
char **
xdrive_enumerate_identifiers (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);
  iface = XDRIVE_GET_IFACE (drive);

  if (iface->enumerate_identifiers == NULL)
    return NULL;

  return (* iface->enumerate_identifiers) (drive);
}

/**
 * xdrive_get_start_stop_type:
 * @drive: a #xdrive_t.
 *
 * Gets a hint about how a drive can be started/stopped.
 *
 * Returns: A value from the #GDriveStartStopType enumeration.
 *
 * Since: 2.22
 */
GDriveStartStopType
xdrive_get_start_stop_type (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->get_start_stop_type == NULL)
    return XDRIVE_START_STOP_TYPE_UNKNOWN;

  return (* iface->get_start_stop_type) (drive);
}


/**
 * xdrive_can_start:
 * @drive: a #xdrive_t.
 *
 * Checks if a drive can be started.
 *
 * Returns: %TRUE if the @drive can be started, %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xdrive_can_start (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->can_start == NULL)
    return FALSE;

  return (* iface->can_start) (drive);
}

/**
 * xdrive_can_start_degraded:
 * @drive: a #xdrive_t.
 *
 * Checks if a drive can be started degraded.
 *
 * Returns: %TRUE if the @drive can be started degraded, %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xdrive_can_start_degraded (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->can_start_degraded == NULL)
    return FALSE;

  return (* iface->can_start_degraded) (drive);
}

/**
 * xdrive_start:
 * @drive: a #xdrive_t.
 * @flags: flags affecting the start operation.
 * @mount_operation: (nullable): a #xmount_operation_t or %NULL to avoid
 *     user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data to pass to @callback
 *
 * Asynchronously starts a drive.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xdrive_start_finish() to obtain the
 * result of the operation.
 *
 * Since: 2.22
 */
void
xdrive_start (xdrive_t              *drive,
               GDriveStartFlags     flags,
               xmount_operation_t     *mount_operation,
               xcancellable_t        *cancellable,
               xasync_ready_callback_t  callback,
               xpointer_t             user_data)
{
  xdrive_iface_t *iface;

  g_return_if_fail (X_IS_DRIVE (drive));

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->start == NULL)
    {
      xtask_report_new_error (drive, callback, user_data,
                               xdrive_start,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("drive doesn’t implement start"));
      return;
    }

  (* iface->start) (drive, flags, mount_operation, cancellable, callback, user_data);
}

/**
 * xdrive_start_finish:
 * @drive: a #xdrive_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, or %NULL
 *
 * Finishes starting a drive.
 *
 * Returns: %TRUE if the drive has been started successfully,
 *     %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xdrive_start_finish (xdrive_t         *drive,
                      xasync_result_t   *result,
                      xerror_t        **error)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);
  xreturn_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xdrive_start))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->start_finish) (drive, result, error);
}

/**
 * xdrive_can_stop:
 * @drive: a #xdrive_t.
 *
 * Checks if a drive can be stopped.
 *
 * Returns: %TRUE if the @drive can be stopped, %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xdrive_can_stop (xdrive_t *drive)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->can_stop == NULL)
    return FALSE;

  return (* iface->can_stop) (drive);
}

/**
 * xdrive_stop:
 * @drive: a #xdrive_t.
 * @flags: flags affecting the unmount if required for stopping.
 * @mount_operation: (nullable): a #xmount_operation_t or %NULL to avoid
 *     user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (nullable): a #xasync_ready_callback_t, or %NULL.
 * @user_data: user data to pass to @callback
 *
 * Asynchronously stops a drive.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xdrive_stop_finish() to obtain the
 * result of the operation.
 *
 * Since: 2.22
 */
void
xdrive_stop (xdrive_t               *drive,
              xmount_unmount_flags_t    flags,
              xmount_operation_t      *mount_operation,
              xcancellable_t         *cancellable,
              xasync_ready_callback_t   callback,
              xpointer_t              user_data)
{
  xdrive_iface_t *iface;

  g_return_if_fail (X_IS_DRIVE (drive));

  iface = XDRIVE_GET_IFACE (drive);

  if (iface->stop == NULL)
    {
      xtask_report_new_error (drive, callback, user_data,
                               xdrive_start,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("drive doesn’t implement stop"));
      return;
    }

  (* iface->stop) (drive, flags, mount_operation, cancellable, callback, user_data);
}

/**
 * xdrive_stop_finish:
 * @drive: a #xdrive_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, or %NULL
 *
 * Finishes stopping a drive.
 *
 * Returns: %TRUE if the drive has been stopped successfully,
 *     %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xdrive_stop_finish (xdrive_t        *drive,
                     xasync_result_t  *result,
                     xerror_t       **error)
{
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), FALSE);
  xreturn_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xdrive_start))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XDRIVE_GET_IFACE (drive);

  return (* iface->stop_finish) (drive, result, error);
}

/**
 * xdrive_get_sort_key:
 * @drive: A #xdrive_t.
 *
 * Gets the sort key for @drive, if any.
 *
 * Returns: (nullable): Sorting key for @drive or %NULL if no such key is available.
 *
 * Since: 2.32
 */
const xchar_t *
xdrive_get_sort_key (xdrive_t  *drive)
{
  const xchar_t *ret = NULL;
  xdrive_iface_t *iface;

  xreturn_val_if_fail (X_IS_DRIVE (drive), NULL);

  iface = XDRIVE_GET_IFACE (drive);
  if (iface->get_sort_key != NULL)
    ret = iface->get_sort_key (drive);

  return ret;
}
