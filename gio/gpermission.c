/*
 * Copyright © 2010 Codethink Limited
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gpermission.h"

#include "gioerror.h"
#include "gioenums.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "glibintl.h"


/**
 * SECTION:gpermission
 * @title: xpermission_t
 * @short_description: An object representing the permission
 *     to perform a certain action
 * @include: gio/gio.h
 *
 * A #xpermission_t represents the status of the caller's permission to
 * perform a certain action.
 *
 * You can query if the action is currently allowed and if it is
 * possible to acquire the permission so that the action will be allowed
 * in the future.
 *
 * There is also an API to actually acquire the permission and one to
 * release it.
 *
 * As an example, a #xpermission_t might represent the ability for the
 * user to write to a #xsettings_t object.  This #xpermission_t object could
 * then be used to decide if it is appropriate to show a "Click here to
 * unlock" button in a dialog and to provide the mechanism to invoke
 * when that button is clicked.
 **/

/**
 * xpermission_t:
 *
 * #xpermission_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

struct _GPermissionPrivate
{
  xboolean_t allowed;
  xboolean_t can_acquire;
  xboolean_t can_release;
};

enum  {
  PROP_NONE,
  PROP_ALLOWED,
  PROP_CAN_ACQUIRE,
  PROP_CAN_RELEASE
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xpermission_t, g_permission, XTYPE_OBJECT)

/**
 * g_permission_acquire:
 * @permission: a #xpermission_t instance
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Attempts to acquire the permission represented by @permission.
 *
 * The precise method by which this happens depends on the permission
 * and the underlying authentication mechanism.  A simple example is
 * that a dialog may appear asking the user to enter their password.
 *
 * You should check with g_permission_get_can_acquire() before calling
 * this function.
 *
 * If the permission is acquired then %TRUE is returned.  Otherwise,
 * %FALSE is returned and @error is set appropriately.
 *
 * This call is blocking, likely for a very long time (in the case that
 * user interaction is required).  See g_permission_acquire_async() for
 * the non-blocking version.
 *
 * Returns: %TRUE if the permission was successfully acquired
 *
 * Since: 2.26
 */
xboolean_t
g_permission_acquire (xpermission_t   *permission,
                      xcancellable_t  *cancellable,
                      xerror_t       **error)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return G_PERMISSION_GET_CLASS (permission)
    ->acquire (permission, cancellable, error);
}

/**
 * g_permission_acquire_async:
 * @permission: a #xpermission_t instance
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: the #xasync_ready_callback_t to call when done
 * @user_data: the user data to pass to @callback
 *
 * Attempts to acquire the permission represented by @permission.
 *
 * This is the first half of the asynchronous version of
 * g_permission_acquire().
 *
 * Since: 2.26
 **/
void
g_permission_acquire_async (xpermission_t         *permission,
                            xcancellable_t        *cancellable,
                            xasync_ready_callback_t  callback,
                            xpointer_t             user_data)
{
  g_return_if_fail (X_IS_PERMISSION (permission));
  G_PERMISSION_GET_CLASS (permission)
    ->acquire_async (permission, cancellable, callback, user_data);
}

/**
 * g_permission_acquire_finish:
 * @permission: a #xpermission_t instance
 * @result: the #xasync_result_t given to the #xasync_ready_callback_t
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Collects the result of attempting to acquire the permission
 * represented by @permission.
 *
 * This is the second half of the asynchronous version of
 * g_permission_acquire().
 *
 * Returns: %TRUE if the permission was successfully acquired
 *
 * Since: 2.26
 **/
xboolean_t
g_permission_acquire_finish (xpermission_t   *permission,
                             xasync_result_t  *result,
                             xerror_t       **error)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return G_PERMISSION_GET_CLASS (permission)
    ->acquire_finish (permission, result, error);
}

/**
 * g_permission_release:
 * @permission: a #xpermission_t instance
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Attempts to release the permission represented by @permission.
 *
 * The precise method by which this happens depends on the permission
 * and the underlying authentication mechanism.  In most cases the
 * permission will be dropped immediately without further action.
 *
 * You should check with g_permission_get_can_release() before calling
 * this function.
 *
 * If the permission is released then %TRUE is returned.  Otherwise,
 * %FALSE is returned and @error is set appropriately.
 *
 * This call is blocking, likely for a very long time (in the case that
 * user interaction is required).  See g_permission_release_async() for
 * the non-blocking version.
 *
 * Returns: %TRUE if the permission was successfully released
 *
 * Since: 2.26
 **/
xboolean_t
g_permission_release (xpermission_t   *permission,
                      xcancellable_t  *cancellable,
                      xerror_t       **error)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return G_PERMISSION_GET_CLASS (permission)
    ->release (permission, cancellable, error);
}

/**
 * g_permission_release_async:
 * @permission: a #xpermission_t instance
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: the #xasync_ready_callback_t to call when done
 * @user_data: the user data to pass to @callback
 *
 * Attempts to release the permission represented by @permission.
 *
 * This is the first half of the asynchronous version of
 * g_permission_release().
 *
 * Since: 2.26
 **/
void
g_permission_release_async (xpermission_t         *permission,
                            xcancellable_t        *cancellable,
                            xasync_ready_callback_t  callback,
                            xpointer_t             user_data)
{
  g_return_if_fail (X_IS_PERMISSION (permission));
  G_PERMISSION_GET_CLASS (permission)
    ->release_async (permission, cancellable, callback, user_data);
}

/**
 * g_permission_release_finish:
 * @permission: a #xpermission_t instance
 * @result: the #xasync_result_t given to the #xasync_ready_callback_t
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Collects the result of attempting to release the permission
 * represented by @permission.
 *
 * This is the second half of the asynchronous version of
 * g_permission_release().
 *
 * Returns: %TRUE if the permission was successfully released
 *
 * Since: 2.26
 **/
xboolean_t
g_permission_release_finish (xpermission_t   *permission,
                             xasync_result_t  *result,
                             xerror_t       **error)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return G_PERMISSION_GET_CLASS (permission)
    ->release_finish (permission, result, error);
}

/**
 * g_permission_get_allowed:
 * @permission: a #xpermission_t instance
 *
 * Gets the value of the 'allowed' property.  This property is %TRUE if
 * the caller currently has permission to perform the action that
 * @permission represents the permission to perform.
 *
 * Returns: the value of the 'allowed' property
 *
 * Since: 2.26
 **/
xboolean_t
g_permission_get_allowed (xpermission_t *permission)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return permission->priv->allowed;
}

/**
 * g_permission_get_can_acquire:
 * @permission: a #xpermission_t instance
 *
 * Gets the value of the 'can-acquire' property.  This property is %TRUE
 * if it is generally possible to acquire the permission by calling
 * g_permission_acquire().
 *
 * Returns: the value of the 'can-acquire' property
 *
 * Since: 2.26
 **/
xboolean_t
g_permission_get_can_acquire (xpermission_t *permission)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return permission->priv->can_acquire;
}

/**
 * g_permission_get_can_release:
 * @permission: a #xpermission_t instance
 *
 * Gets the value of the 'can-release' property.  This property is %TRUE
 * if it is generally possible to release the permission by calling
 * g_permission_release().
 *
 * Returns: the value of the 'can-release' property
 *
 * Since: 2.26
 **/
xboolean_t
g_permission_get_can_release (xpermission_t *permission)
{
  xreturn_val_if_fail (X_IS_PERMISSION (permission), FALSE);
  return permission->priv->can_release;
}

/**
 * g_permission_impl_update:
 * @permission: a #xpermission_t instance
 * @allowed: the new value for the 'allowed' property
 * @can_acquire: the new value for the 'can-acquire' property
 * @can_release: the new value for the 'can-release' property
 *
 * This function is called by the #xpermission_t implementation to update
 * the properties of the permission.  You should never call this
 * function except from a #xpermission_t implementation.
 *
 * xobject_t notify signals are generated, as appropriate.
 *
 * Since: 2.26
 **/
void
g_permission_impl_update (xpermission_t *permission,
                          xboolean_t     allowed,
                          xboolean_t     can_acquire,
                          xboolean_t     can_release)
{
  xobject_t *object;

  g_return_if_fail (X_IS_PERMISSION (permission));

  object = G_OBJECT (permission);
  xobject_freeze_notify (object);

  allowed = allowed != FALSE;
  if (allowed != permission->priv->allowed)
    {
      permission->priv->allowed = allowed;
      xobject_notify (object, "allowed");
    }

  can_acquire = can_acquire != FALSE;
  if (can_acquire != permission->priv->can_acquire)
    {
      permission->priv->can_acquire = can_acquire;
      xobject_notify (object, "can-acquire");
    }

  can_release = can_release != FALSE;
  if (can_release != permission->priv->can_release)
    {
      permission->priv->can_release = can_release;
      xobject_notify (object, "can-release");
    }

  xobject_thaw_notify (object);
}

static void
g_permission_get_property (xobject_t *object, xuint_t prop_id,
                           xvalue_t *value, xparam_spec_t *pspec)
{
  xpermission_t *permission = G_PERMISSION (object);

  switch (prop_id)
    {
    case PROP_ALLOWED:
      xvalue_set_boolean (value, permission->priv->allowed);
      break;

    case PROP_CAN_ACQUIRE:
      xvalue_set_boolean (value, permission->priv->can_acquire);
      break;

    case PROP_CAN_RELEASE:
      xvalue_set_boolean (value, permission->priv->can_release);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
g_permission_init (xpermission_t *permission)
{
  permission->priv = g_permission_get_instance_private (permission);
}

static xboolean_t
acquire_or_release (xpermission_t   *permission,
                    xcancellable_t  *cancellable,
                    xerror_t       **error)
{
  g_set_error_literal  (error,
                        G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                        "Can't acquire or release permission");
  return FALSE;
}

static void
acquire_or_release_async (xpermission_t         *permission,
                          xcancellable_t        *cancellable,
                          xasync_ready_callback_t  callback,
                          xpointer_t             user_data)
{
  xtask_report_new_error (permission,
                           callback, user_data,
                           NULL,
                           G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           "Can't acquire or release permission");
}

static xboolean_t
acquire_or_release_finish (xpermission_t   *permission,
                           xasync_result_t  *result,
                           xerror_t       **error)
{
  return xtask_propagate_boolean (XTASK (result), error);
}

static void
g_permission_class_init (GPermissionClass *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->get_property = g_permission_get_property;

  class->acquire = acquire_or_release;
  class->release = acquire_or_release;
  class->acquire_async = acquire_or_release_async;
  class->release_async = acquire_or_release_async;
  class->acquire_finish = acquire_or_release_finish;
  class->release_finish = acquire_or_release_finish;

  /**
   * xpermission_t:allowed:
   *
   * %TRUE if the caller currently has permission to perform the action that
   * @permission represents the permission to perform.
   */
   xobject_class_install_property (object_class, PROP_ALLOWED,
     xparam_spec_boolean ("allowed",
                           P_("Is allowed"),
                           P_("If the caller is allowed to perform the action"),
                           FALSE,
                           XPARAM_STATIC_STRINGS | XPARAM_READABLE));

  /**
   * xpermission_t:can-acquire:
   *
   * %TRUE if it is generally possible to acquire the permission by calling
   * g_permission_acquire().
   */
   xobject_class_install_property (object_class, PROP_CAN_ACQUIRE,
     xparam_spec_boolean ("can-acquire",
                           P_("Can acquire"),
                           P_("If calling g_permission_acquire() makes sense"),
                           FALSE,
                           XPARAM_STATIC_STRINGS | XPARAM_READABLE));

  /**
   * xpermission_t:can-release:
   *
   * %TRUE if it is generally possible to release the permission by calling
   * g_permission_release().
   */
   xobject_class_install_property (object_class, PROP_CAN_RELEASE,
     xparam_spec_boolean ("can-release",
                           P_("Can release"),
                           P_("If calling g_permission_release() makes sense"),
                           FALSE,
                           XPARAM_STATIC_STRINGS | XPARAM_READABLE));
}
