/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include "gdbusinterface.h"
#include "gdbusinterfaceskeleton.h"
#include "gdbusobjectskeleton.h"
#include "gioenumtypes.h"
#include "gdbusprivate.h"
#include "gdbusmethodinvocation.h"
#include "gdbusconnection.h"
#include "gmarshal-internal.h"
#include "gtask.h"
#include "gioerror.h"

#include "glibintl.h"

/**
 * SECTION:gdbusinterfaceskeleton
 * @short_description: Service-side D-Bus interface
 * @include: gio/gio.h
 *
 * Abstract base class for D-Bus interfaces on the service side.
 */

struct _GDBusInterfaceSkeletonPrivate
{
  xmutex_t                      lock;

  xdbus_object_t                *object;
  GDBusInterfaceSkeletonFlags flags;

  xslist_t                     *connections;   /* List of ConnectionData */
  xchar_t                      *object_path;   /* The object path for this skeleton */
  xdbus_interface_vtable_t       *hooked_vtable;
};

typedef struct
{
  xdbus_connection_t *connection;
  xuint_t            registration_id;
} ConnectionData;

enum
{
  G_AUTHORIZE_METHOD_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_G_FLAGS
};

static xuint_t signals[LAST_SIGNAL] = {0};

static void     dbus_interface_interface_init                      (xdbus_interface_iface_t    *iface);

static void     set_object_path_locked                             (xdbus_interface_skeleton_t *interface_,
                                                                    const xchar_t            *object_path);
static void     remove_connection_locked                           (xdbus_interface_skeleton_t *interface_,
                                                                    xdbus_connection_t        *connection);
static void     skeleton_intercept_handle_method_call              (xdbus_connection_t        *connection,
                                                                    const xchar_t            *sender,
                                                                    const xchar_t            *object_path,
                                                                    const xchar_t            *interface_name,
                                                                    const xchar_t            *method_name,
                                                                    xvariant_t               *parameters,
                                                                    xdbus_method_invocation_t  *invocation,
                                                                    xpointer_t                user_data);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (xdbus_interface_skeleton_t, g_dbus_interface_skeleton, XTYPE_OBJECT,
                                  G_ADD_PRIVATE (xdbus_interface_skeleton_t)
                                  G_IMPLEMENT_INTERFACE (XTYPE_DBUS_INTERFACE, dbus_interface_interface_init))

static void
g_dbus_interface_skeleton_finalize (xobject_t *object)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (object);

  /* Hold the lock just in case any code we call verifies that the lock is held */
  g_mutex_lock (&interface->priv->lock);

  /* unexport from all connections if we're exported anywhere */
  while (interface->priv->connections != NULL)
    {
      ConnectionData *data = interface->priv->connections->data;
      remove_connection_locked (interface, data->connection);
    }

  set_object_path_locked (interface, NULL);

  g_mutex_unlock (&interface->priv->lock);

  g_free (interface->priv->hooked_vtable);

  if (interface->priv->object != NULL)
    xobject_remove_weak_pointer (G_OBJECT (interface->priv->object), (xpointer_t *) &interface->priv->object);

  g_mutex_clear (&interface->priv->lock);

  XOBJECT_CLASS (g_dbus_interface_skeleton_parent_class)->finalize (object);
}

static void
g_dbus_interface_skeleton_get_property (xobject_t      *object,
                                        xuint_t         prop_id,
                                        xvalue_t       *value,
                                        xparam_spec_t   *pspec)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (object);

  switch (prop_id)
    {
    case PROP_G_FLAGS:
      xvalue_set_flags (value, g_dbus_interface_skeleton_get_flags (interface));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_dbus_interface_skeleton_set_property (xobject_t      *object,
                                        xuint_t         prop_id,
                                        const xvalue_t *value,
                                        xparam_spec_t   *pspec)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (object);

  switch (prop_id)
    {
    case PROP_G_FLAGS:
      g_dbus_interface_skeleton_set_flags (interface, xvalue_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static xboolean_t
g_dbus_interface_skeleton_g_authorize_method_default (xdbus_interface_skeleton_t    *interface,
                                                      xdbus_method_invocation_t *invocation)
{
  return TRUE;
}

static void
g_dbus_interface_skeleton_class_init (GDBusInterfaceSkeletonClass *klass)
{
  xobject_class_t *xobject_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->finalize     = g_dbus_interface_skeleton_finalize;
  xobject_class->set_property = g_dbus_interface_skeleton_set_property;
  xobject_class->get_property = g_dbus_interface_skeleton_get_property;

  klass->g_authorize_method = g_dbus_interface_skeleton_g_authorize_method_default;

  /**
   * xdbus_interface_skeleton_t:g-flags:
   *
   * Flags from the #GDBusInterfaceSkeletonFlags enumeration.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_G_FLAGS,
                                   xparam_spec_flags ("g-flags",
                                                       "g-flags",
                                                       "Flags for the interface skeleton",
                                                       XTYPE_DBUS_INTERFACE_SKELETON_FLAGS,
                                                       G_DBUS_INTERFACE_SKELETON_FLAGS_NONE,
                                                       XPARAM_READABLE |
                                                       XPARAM_WRITABLE |
                                                       XPARAM_STATIC_STRINGS));

  /**
   * xdbus_interface_skeleton_t::g-authorize-method:
   * @interface: The #xdbus_interface_skeleton_t emitting the signal.
   * @invocation: A #xdbus_method_invocation_t.
   *
   * Emitted when a method is invoked by a remote caller and used to
   * determine if the method call is authorized.
   *
   * Note that this signal is emitted in a thread dedicated to
   * handling the method call so handlers are allowed to perform
   * blocking IO. This means that it is appropriate to call e.g.
   * [polkit_authority_check_authorization_sync()](http://hal.freedesktop.org/docs/polkit/PolkitAuthority.html#polkit-authority-check-authorization-sync)
   * with the
   * [POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION](http://hal.freedesktop.org/docs/polkit/PolkitAuthority.html#POLKIT-CHECK-AUTHORIZATION-FLAGS-ALLOW-USER-INTERACTION:CAPS)
   * flag set.
   *
   * If %FALSE is returned then no further handlers are run and the
   * signal handler must take a reference to @invocation and finish
   * handling the call (e.g. return an error via
   * xdbus_method_invocation_return_error()).
   *
   * Otherwise, if %TRUE is returned, signal emission continues. If no
   * handlers return %FALSE, then the method is dispatched. If
   * @interface has an enclosing #xdbus_object_skeleton_t, then the
   * #xdbus_object_skeleton_t::authorize-method signal handlers run before
   * the handlers for this signal.
   *
   * The default class handler just returns %TRUE.
   *
   * Please note that the common case is optimized: if no signals
   * handlers are connected and the default class handler isn't
   * overridden (for both @interface and the enclosing
   * #xdbus_object_skeleton_t, if any) and #xdbus_interface_skeleton_t:g-flags does
   * not have the
   * %G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD
   * flags set, no dedicated thread is ever used and the call will be
   * handled in the same thread as the object that @interface belongs
   * to was exported in.
   *
   * Returns: %TRUE if the call is authorized, %FALSE otherwise.
   *
   * Since: 2.30
   */
  signals[G_AUTHORIZE_METHOD_SIGNAL] =
    xsignal_new (I_("g-authorize-method"),
                  XTYPE_DBUS_INTERFACE_SKELETON,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GDBusInterfaceSkeletonClass, g_authorize_method),
                  _xsignal_accumulator_false_handled,
                  NULL,
                  _g_cclosure_marshal_BOOLEAN__OBJECT,
                  XTYPE_BOOLEAN,
                  1,
                  XTYPE_DBUS_METHOD_INVOCATION);
  xsignal_set_va_marshaller (signals[G_AUTHORIZE_METHOD_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_BOOLEAN__OBJECTv);
}

static void
g_dbus_interface_skeleton_init (xdbus_interface_skeleton_t *interface)
{
  interface->priv = g_dbus_interface_skeleton_get_instance_private (interface);
  g_mutex_init (&interface->priv->lock);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_interface_skeleton_get_flags:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets the #GDBusInterfaceSkeletonFlags that describes what the behavior
 * of @interface_
 *
 * Returns: One or more flags from the #GDBusInterfaceSkeletonFlags enumeration.
 *
 * Since: 2.30
 */
GDBusInterfaceSkeletonFlags
g_dbus_interface_skeleton_get_flags (xdbus_interface_skeleton_t  *interface_)
{
  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), G_DBUS_INTERFACE_SKELETON_FLAGS_NONE);
  return interface_->priv->flags;
}

/**
 * g_dbus_interface_skeleton_set_flags:
 * @interface_: A #xdbus_interface_skeleton_t.
 * @flags: Flags from the #GDBusInterfaceSkeletonFlags enumeration.
 *
 * Sets flags describing what the behavior of @skeleton should be.
 *
 * Since: 2.30
 */
void
g_dbus_interface_skeleton_set_flags (xdbus_interface_skeleton_t      *interface_,
                                     GDBusInterfaceSkeletonFlags  flags)
{
  g_return_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_));
  g_mutex_lock (&interface_->priv->lock);
  if (interface_->priv->flags != flags)
    {
      interface_->priv->flags = flags;
      g_mutex_unlock (&interface_->priv->lock);
      xobject_notify (G_OBJECT (interface_), "g-flags");
    }
  else
    {
      g_mutex_unlock (&interface_->priv->lock);
    }
}

/**
 * g_dbus_interface_skeleton_get_info:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets D-Bus introspection information for the D-Bus interface
 * implemented by @interface_.
 *
 * Returns: (transfer none): A #xdbus_interface_info_t (never %NULL). Do not free.
 *
 * Since: 2.30
 */
xdbus_interface_info_t *
g_dbus_interface_skeleton_get_info (xdbus_interface_skeleton_t *interface_)
{
  xdbus_interface_info_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), NULL);
  ret = G_DBUS_INTERFACE_SKELETON_GET_CLASS (interface_)->get_info (interface_);
  g_warn_if_fail (ret != NULL);
  return ret;
}

/**
 * g_dbus_interface_skeleton_get_vtable: (skip)
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets the interface vtable for the D-Bus interface implemented by
 * @interface_. The returned function pointers should expect @interface_
 * itself to be passed as @user_data.
 *
 * Returns: A #xdbus_interface_vtable_t (never %NULL).
 *
 * Since: 2.30
 */
xdbus_interface_vtable_t *
g_dbus_interface_skeleton_get_vtable (xdbus_interface_skeleton_t *interface_)
{
  xdbus_interface_vtable_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), NULL);
  ret = G_DBUS_INTERFACE_SKELETON_GET_CLASS (interface_)->get_vtable (interface_);
  g_warn_if_fail (ret != NULL);
  return ret;
}

/**
 * g_dbus_interface_skeleton_get_properties:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets all D-Bus properties for @interface_.
 *
 * Returns: (transfer full): A #xvariant_t of type
 * ['a{sv}'][G-VARIANT-TYPE-VARDICT:CAPS].
 * Free with xvariant_unref().
 *
 * Since: 2.30
 */
xvariant_t *
g_dbus_interface_skeleton_get_properties (xdbus_interface_skeleton_t *interface_)
{
  xvariant_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), NULL);
  ret = G_DBUS_INTERFACE_SKELETON_GET_CLASS (interface_)->get_properties (interface_);
  return xvariant_take_ref (ret);
}

/**
 * g_dbus_interface_skeleton_flush:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * If @interface_ has outstanding changes, request for these changes to be
 * emitted immediately.
 *
 * For example, an exported D-Bus interface may queue up property
 * changes and emit the
 * `org.freedesktop.DBus.Properties.PropertiesChanged`
 * signal later (e.g. in an idle handler). This technique is useful
 * for collapsing multiple property changes into one.
 *
 * Since: 2.30
 */
void
g_dbus_interface_skeleton_flush (xdbus_interface_skeleton_t *interface_)
{
  g_return_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_));
  G_DBUS_INTERFACE_SKELETON_GET_CLASS (interface_)->flush (interface_);
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_interface_info_t *
_g_dbus_interface_skeleton_get_info (xdbus_interface_t *interface_)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (interface_);
  return g_dbus_interface_skeleton_get_info (interface);
}

static xdbus_object_t *
g_dbus_interface_skeleton_get_object (xdbus_interface_t *interface_)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (interface_);
  xdbus_object_t *ret;
  g_mutex_lock (&interface->priv->lock);
  ret = interface->priv->object;
  g_mutex_unlock (&interface->priv->lock);
  return ret;
}

static xdbus_object_t *
g_dbus_interface_skeleton_dup_object (xdbus_interface_t *interface_)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (interface_);
  xdbus_object_t *ret;
  g_mutex_lock (&interface->priv->lock);
  ret = interface->priv->object;
  if (ret != NULL)
    xobject_ref (ret);
  g_mutex_unlock (&interface->priv->lock);
  return ret;
}

static void
g_dbus_interface_skeleton_set_object (xdbus_interface_t *interface_,
                                      xdbus_object_t    *object)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (interface_);
  g_mutex_lock (&interface->priv->lock);
  if (interface->priv->object != NULL)
    xobject_remove_weak_pointer (G_OBJECT (interface->priv->object), (xpointer_t *) &interface->priv->object);
  interface->priv->object = object;
  if (object != NULL)
    xobject_add_weak_pointer (G_OBJECT (interface->priv->object), (xpointer_t *) &interface->priv->object);
  g_mutex_unlock (&interface->priv->lock);
}

static void
dbus_interface_interface_init (xdbus_interface_iface_t *iface)
{
  iface->get_info    = _g_dbus_interface_skeleton_get_info;
  iface->get_object  = g_dbus_interface_skeleton_get_object;
  iface->dup_object  = g_dbus_interface_skeleton_dup_object;
  iface->set_object  = g_dbus_interface_skeleton_set_object;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xint_t ref_count;  /* (atomic) */
  xdbus_interface_skeleton_t       *interface;
  GDBusInterfaceMethodCallFunc  method_call_func;
  xdbus_method_invocation_t        *invocation;
} DispatchData;

static void
dispatch_data_unref (DispatchData *data)
{
  if (g_atomic_int_dec_and_test (&data->ref_count))
    g_slice_free (DispatchData, data);
}

static DispatchData *
dispatch_data_ref (DispatchData *data)
{
  g_atomic_int_inc (&data->ref_count);
  return data;
}

static xboolean_t
dispatch_invoke_in_context_func (xpointer_t user_data)
{
  DispatchData *data = user_data;
  data->method_call_func (xdbus_method_invocation_get_connection (data->invocation),
                          xdbus_method_invocation_get_sender (data->invocation),
                          xdbus_method_invocation_get_object_path (data->invocation),
                          xdbus_method_invocation_get_interface_name (data->invocation),
                          xdbus_method_invocation_get_method_name (data->invocation),
                          xdbus_method_invocation_get_parameters (data->invocation),
                          data->invocation,
                          xdbus_method_invocation_get_user_data (data->invocation));
  return FALSE;
}

static void
dispatch_in_thread_func (xtask_t        *task,
                         xpointer_t      source_object,
                         xpointer_t      task_data,
                         xcancellable_t *cancellable)
{
  DispatchData *data = task_data;
  GDBusInterfaceSkeletonFlags flags;
  xdbus_object_t *object;
  xboolean_t authorized;

  g_mutex_lock (&data->interface->priv->lock);
  flags = data->interface->priv->flags;
  object = data->interface->priv->object;
  if (object != NULL)
    xobject_ref (object);
  g_mutex_unlock (&data->interface->priv->lock);

  /* first check on the enclosing object (if any), then the interface */
  authorized = TRUE;
  if (object != NULL)
    {
      xsignal_emit_by_name (object,
                             "authorize-method",
                             data->interface,
                             data->invocation,
                             &authorized);
    }
  if (authorized)
    {
      xsignal_emit (data->interface,
                     signals[G_AUTHORIZE_METHOD_SIGNAL],
                     0,
                     data->invocation,
                     &authorized);
    }

  if (authorized)
    {
      xboolean_t run_in_thread;
      run_in_thread = (flags & G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);
      if (run_in_thread)
        {
          /* might as well just re-use the existing thread */
          data->method_call_func (xdbus_method_invocation_get_connection (data->invocation),
                                  xdbus_method_invocation_get_sender (data->invocation),
                                  xdbus_method_invocation_get_object_path (data->invocation),
                                  xdbus_method_invocation_get_interface_name (data->invocation),
                                  xdbus_method_invocation_get_method_name (data->invocation),
                                  xdbus_method_invocation_get_parameters (data->invocation),
                                  data->invocation,
                                  xdbus_method_invocation_get_user_data (data->invocation));
        }
      else
        {
          /* bah, back to original context */
          xmain_context_invoke_full (xtask_get_context (task),
                                      xtask_get_priority (task),
                                      dispatch_invoke_in_context_func,
                                      dispatch_data_ref (data),
                                      (xdestroy_notify_t) dispatch_data_unref);
        }
    }
  else
    {
      /* do nothing */
    }

  if (object != NULL)
    xobject_unref (object);
}

static void
g_dbus_interface_method_dispatch_helper (xdbus_interface_skeleton_t       *interface,
                                         GDBusInterfaceMethodCallFunc  method_call_func,
                                         xdbus_method_invocation_t        *invocation)
{
  xboolean_t has_handlers;
  xboolean_t has_default_class_handler;
  xboolean_t emit_authorized_signal;
  xboolean_t run_in_thread;
  GDBusInterfaceSkeletonFlags flags;
  xdbus_object_t *object;

  g_return_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface));
  g_return_if_fail (method_call_func != NULL);
  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));

  g_mutex_lock (&interface->priv->lock);
  flags = interface->priv->flags;
  object = interface->priv->object;
  if (object != NULL)
    xobject_ref (object);
  g_mutex_unlock (&interface->priv->lock);

  /* optimization for the common case where
   *
   *  a) no handler is connected and class handler is not overridden (both interface and object); and
   *  b) method calls are not dispatched in a thread
   */
  has_handlers = xsignal_has_handler_pending (interface,
                                               signals[G_AUTHORIZE_METHOD_SIGNAL],
                                               0,
                                               TRUE);
  has_default_class_handler = (G_DBUS_INTERFACE_SKELETON_GET_CLASS (interface)->g_authorize_method ==
                               g_dbus_interface_skeleton_g_authorize_method_default);

  emit_authorized_signal = (has_handlers || !has_default_class_handler);
  if (!emit_authorized_signal)
    {
      if (object != NULL)
        emit_authorized_signal = _xdbus_object_skeleton_has_authorize_method_handlers (G_DBUS_OBJECT_SKELETON (object));
    }

  run_in_thread = (flags & G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);
  if (!emit_authorized_signal && !run_in_thread)
    {
      method_call_func (xdbus_method_invocation_get_connection (invocation),
                        xdbus_method_invocation_get_sender (invocation),
                        xdbus_method_invocation_get_object_path (invocation),
                        xdbus_method_invocation_get_interface_name (invocation),
                        xdbus_method_invocation_get_method_name (invocation),
                        xdbus_method_invocation_get_parameters (invocation),
                        invocation,
                        xdbus_method_invocation_get_user_data (invocation));
    }
  else
    {
      xtask_t *task;
      DispatchData *data;

      data = g_slice_new0 (DispatchData);
      data->interface = interface;
      data->method_call_func = method_call_func;
      data->invocation = invocation;
      data->ref_count = 1;

      task = xtask_new (interface, NULL, NULL, NULL);
      xtask_set_source_tag (task, g_dbus_interface_method_dispatch_helper);
      xtask_set_name (task, "[gio] D-Bus interface method dispatch");
      xtask_set_task_data (task, data, (xdestroy_notify_t) dispatch_data_unref);
      xtask_run_in_thread (task, dispatch_in_thread_func);
      xobject_unref (task);
    }

  if (object != NULL)
    xobject_unref (object);
}

static void
skeleton_intercept_handle_method_call (xdbus_connection_t       *connection,
                                       const xchar_t           *sender,
                                       const xchar_t           *object_path,
                                       const xchar_t           *interface_name,
                                       const xchar_t           *method_name,
                                       xvariant_t              *parameters,
                                       xdbus_method_invocation_t *invocation,
                                       xpointer_t               user_data)
{
  xdbus_interface_skeleton_t *interface = G_DBUS_INTERFACE_SKELETON (user_data);
  g_dbus_interface_method_dispatch_helper (interface,
                                           g_dbus_interface_skeleton_get_vtable (interface)->method_call,
                                           invocation);
}

/* ---------------------------------------------------------------------------------------------------- */

static ConnectionData *
new_connection (xdbus_connection_t *connection,
                xuint_t            registration_id)
{
  ConnectionData *data;

  data = g_slice_new0 (ConnectionData);
  data->connection      = xobject_ref (connection);
  data->registration_id = registration_id;

  return data;
}

static void
free_connection (ConnectionData *data)
{
  if (data != NULL)
    {
      xobject_unref (data->connection);
      g_slice_free (ConnectionData, data);
    }
}

static xboolean_t
add_connection_locked (xdbus_interface_skeleton_t *interface_,
                       xdbus_connection_t        *connection,
                       xerror_t                **error)
{
  ConnectionData *data;
  xuint_t registration_id;
  xboolean_t ret = FALSE;

  if (interface_->priv->hooked_vtable == NULL)
    {
      /* Hook the vtable since we need to intercept method calls for
       * ::g-authorize-method and for dispatching in thread vs
       * context
       *
       * We need to wait until subclasses have had time to initialize
       * properly before building the hooked_vtable, so we create it
       * once at the last minute.
       */
      interface_->priv->hooked_vtable = g_memdup2 (g_dbus_interface_skeleton_get_vtable (interface_), sizeof (xdbus_interface_vtable_t));
      interface_->priv->hooked_vtable->method_call = skeleton_intercept_handle_method_call;
    }

  registration_id = xdbus_connection_register_object (connection,
                                                       interface_->priv->object_path,
                                                       g_dbus_interface_skeleton_get_info (interface_),
                                                       interface_->priv->hooked_vtable,
                                                       interface_,
                                                       NULL, /* user_data_free_func */
                                                       error);

  if (registration_id > 0)
    {
      data = new_connection (connection, registration_id);
      interface_->priv->connections = xslist_append (interface_->priv->connections, data);
      ret = TRUE;
    }

  return ret;
}

static void
remove_connection_locked (xdbus_interface_skeleton_t *interface_,
                          xdbus_connection_t        *connection)
{
  ConnectionData *data;
  xslist_t *l;

  /* Get the connection in the list and unregister ... */
  for (l = interface_->priv->connections; l != NULL; l = l->next)
    {
      data = l->data;
      if (data->connection == connection)
        {
          g_warn_if_fail (xdbus_connection_unregister_object (data->connection, data->registration_id));
          free_connection (data);
          interface_->priv->connections = xslist_delete_link (interface_->priv->connections, l);
          /* we are guaranteed that the connection is only added once, so bail out early */
          goto out;
        }
    }
 out:
  ;
}

static void
set_object_path_locked (xdbus_interface_skeleton_t *interface_,
                        const xchar_t            *object_path)
{
  if (xstrcmp0 (interface_->priv->object_path, object_path) != 0)
    {
      g_free (interface_->priv->object_path);
      interface_->priv->object_path = xstrdup (object_path);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_interface_skeleton_get_connection:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets the first connection that @interface_ is exported on, if any.
 *
 * Returns: (nullable) (transfer none): A #xdbus_connection_t or %NULL if @interface_ is
 * not exported anywhere. Do not free, the object belongs to @interface_.
 *
 * Since: 2.30
 */
xdbus_connection_t *
g_dbus_interface_skeleton_get_connection (xdbus_interface_skeleton_t *interface_)
{
  ConnectionData  *data;
  xdbus_connection_t *ret;

  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), NULL);
  g_mutex_lock (&interface_->priv->lock);

  ret = NULL;
  if (interface_->priv->connections != NULL)
    {
      data = interface_->priv->connections->data;
      if (data != NULL)
        ret = data->connection;
    }

  g_mutex_unlock (&interface_->priv->lock);

  return ret;
}

/**
 * g_dbus_interface_skeleton_get_connections:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets a list of the connections that @interface_ is exported on.
 *
 * Returns: (element-type xdbus_connection_t) (transfer full): A list of
 *   all the connections that @interface_ is exported on. The returned
 *   list should be freed with xlist_free() after each element has
 *   been freed with xobject_unref().
 *
 * Since: 2.32
 */
xlist_t *
g_dbus_interface_skeleton_get_connections (xdbus_interface_skeleton_t *interface_)
{
  xlist_t           *connections;
  xslist_t          *l;
  ConnectionData  *data;

  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), NULL);

  g_mutex_lock (&interface_->priv->lock);
  connections = NULL;

  for (l = interface_->priv->connections; l != NULL; l = l->next)
    {
      data        = l->data;
      connections = xlist_prepend (connections,
                                    /* Return a reference to each connection */
                                    xobject_ref (data->connection));
    }

  g_mutex_unlock (&interface_->priv->lock);

  return xlist_reverse (connections);
}

/**
 * g_dbus_interface_skeleton_has_connection:
 * @interface_: A #xdbus_interface_skeleton_t.
 * @connection: A #xdbus_connection_t.
 *
 * Checks if @interface_ is exported on @connection.
 *
 * Returns: %TRUE if @interface_ is exported on @connection, %FALSE otherwise.
 *
 * Since: 2.32
 */
xboolean_t
g_dbus_interface_skeleton_has_connection (xdbus_interface_skeleton_t     *interface_,
                                          xdbus_connection_t            *connection)
{
  xslist_t *l;
  xboolean_t ret = FALSE;

  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), FALSE);
  xreturn_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);

  g_mutex_lock (&interface_->priv->lock);

  for (l = interface_->priv->connections; l != NULL; l = l->next)
    {
      ConnectionData *data = l->data;
      if (data->connection == connection)
        {
          ret = TRUE;
          goto out;
        }
    }

 out:
  g_mutex_unlock (&interface_->priv->lock);
  return ret;
}

/**
 * g_dbus_interface_skeleton_get_object_path:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Gets the object path that @interface_ is exported on, if any.
 *
 * Returns: (nullable): A string owned by @interface_ or %NULL if @interface_ is not exported
 * anywhere. Do not free, the string belongs to @interface_.
 *
 * Since: 2.30
 */
const xchar_t *
g_dbus_interface_skeleton_get_object_path (xdbus_interface_skeleton_t *interface_)
{
  const xchar_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), NULL);
  g_mutex_lock (&interface_->priv->lock);
  ret = interface_->priv->object_path;
  g_mutex_unlock (&interface_->priv->lock);
  return ret;
}

/**
 * g_dbus_interface_skeleton_export:
 * @interface_: The D-Bus interface to export.
 * @connection: A #xdbus_connection_t to export @interface_ on.
 * @object_path: The path to export the interface at.
 * @error: Return location for error or %NULL.
 *
 * Exports @interface_ at @object_path on @connection.
 *
 * This can be called multiple times to export the same @interface_
 * onto multiple connections however the @object_path provided must be
 * the same for all connections.
 *
 * Use g_dbus_interface_skeleton_unexport() to unexport the object.
 *
 * Returns: %TRUE if the interface was exported on @connection, otherwise %FALSE with
 * @error set.
 *
 * Since: 2.30
 */
xboolean_t
g_dbus_interface_skeleton_export (xdbus_interface_skeleton_t  *interface_,
                                  xdbus_connection_t         *connection,
                                  const xchar_t             *object_path,
                                  xerror_t                 **error)
{
  xboolean_t ret = FALSE;

  xreturn_val_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_), FALSE);
  xreturn_val_if_fail (X_IS_DBUS_CONNECTION (connection), FALSE);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* Assert that the object path is the same for multiple connections here */
  xreturn_val_if_fail (interface_->priv->object_path == NULL ||
                        xstrcmp0 (interface_->priv->object_path, object_path) == 0, FALSE);

  g_mutex_lock (&interface_->priv->lock);

  /* Set the object path */
  set_object_path_locked (interface_, object_path);

  /* Add the connection */
  ret = add_connection_locked (interface_, connection, error);

  g_mutex_unlock (&interface_->priv->lock);
  return ret;
}

/**
 * g_dbus_interface_skeleton_unexport:
 * @interface_: A #xdbus_interface_skeleton_t.
 *
 * Stops exporting @interface_ on all connections it is exported on.
 *
 * To unexport @interface_ from only a single connection, use
 * g_dbus_interface_skeleton_unexport_from_connection()
 *
 * Since: 2.30
 */
void
g_dbus_interface_skeleton_unexport (xdbus_interface_skeleton_t *interface_)
{
  g_return_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_));
  g_return_if_fail (interface_->priv->connections != NULL);

  g_mutex_lock (&interface_->priv->lock);

  xassert (interface_->priv->object_path != NULL);
  xassert (interface_->priv->hooked_vtable != NULL);

  /* Remove all connections */
  while (interface_->priv->connections != NULL)
    {
      ConnectionData *data = interface_->priv->connections->data;
      remove_connection_locked (interface_, data->connection);
    }

  /* Unset the object path since there are no connections left */
  set_object_path_locked (interface_, NULL);

  g_mutex_unlock (&interface_->priv->lock);
}


/**
 * g_dbus_interface_skeleton_unexport_from_connection:
 * @interface_: A #xdbus_interface_skeleton_t.
 * @connection: A #xdbus_connection_t.
 *
 * Stops exporting @interface_ on @connection.
 *
 * To stop exporting on all connections the interface is exported on,
 * use g_dbus_interface_skeleton_unexport().
 *
 * Since: 2.32
 */
void
g_dbus_interface_skeleton_unexport_from_connection (xdbus_interface_skeleton_t *interface_,
                                                    xdbus_connection_t        *connection)
{
  g_return_if_fail (X_IS_DBUS_INTERFACE_SKELETON (interface_));
  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail (interface_->priv->connections != NULL);

  g_mutex_lock (&interface_->priv->lock);

  xassert (interface_->priv->object_path != NULL);
  xassert (interface_->priv->hooked_vtable != NULL);

  remove_connection_locked (interface_, connection);

  /* Reset the object path if we removed the last connection */
  if (interface_->priv->connections == NULL)
    set_object_path_locked (interface_, NULL);

  g_mutex_unlock (&interface_->priv->lock);
}

/* ---------------------------------------------------------------------------------------------------- */
