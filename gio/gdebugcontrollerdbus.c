/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2021 Endless OS Foundation, LLC
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include <gio/gio.h>
#include "gdebugcontroller.h"
#include "gdebugcontrollerdbus.h"
#include "giomodule-priv.h"
#include "gi18n.h"
#include "gio/gdbusprivate.h"
#include "gio/gmarshal-internal.h"

/**
 * SECTION:gdebugcontrollerdbus
 * @title: xdebug_controller_dbus_t
 * @short_description: Debugging controller D-Bus implementation
 * @include: gio/gio.h
 *
 * #xdebug_controller_dbus_t is an implementation of #xdebug_controller_t which exposes
 * debug settings as a D-Bus object.
 *
 * It is a #xinitable_t object, and will register an object at
 * `/org/gtk/Debugging` on the bus given as
 * #xdebug_controller_dbus_t:connection once it’s initialized. The object will be
 * unregistered when the last reference to the #xdebug_controller_dbus_t is dropped.
 *
 * This D-Bus object can be used by remote processes to enable or disable debug
 * output in this process. Remote processes calling
 * `org.gtk.Debugging.SetDebugEnabled()` will affect the value of
 * #xdebug_controller_t:debug-enabled and, by default, g_log_get_debug_enabled().
 * default.
 *
 * By default, all processes will be able to call `SetDebugEnabled()`. If this
 * process is privileged, or might expose sensitive information in its debug
 * output, you may want to restrict the ability to enable debug output to
 * privileged users or processes.
 *
 * One option is to install a D-Bus security policy which restricts access to
 * `SetDebugEnabled()`, installing something like the following in
 * `$datadir/dbus-1/system.d/`:
 * |[<!-- language="XML" -->
 * <?xml version="1.0"?> <!--*-nxml-*-->
 * <!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 *      "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
 * <busconfig>
 *   <policy user="root">
 *     <allow send_destination="com.example.MyService" send_interface="org.gtk.Debugging"/>
 *   </policy>
 *   <policy context="default">
 *     <deny send_destination="com.example.MyService" send_interface="org.gtk.Debugging"/>
 *   </policy>
 * </busconfig>
 * ]|
 *
 * This will prevent the `SetDebugEnabled()` method from being called by all
 * except root. It will not prevent the `DebugEnabled` property from being read,
 * as it’s accessed through the `org.freedesktop.DBus.Properties` interface.
 *
 * Another option is to use polkit to allow or deny requests on a case-by-case
 * basis, allowing for the possibility of dynamic authorisation. To do this,
 * connect to the #xdebug_controller_dbus_t::authorize signal and query polkit in
 * it:
 * |[<!-- language="C" -->
 *   x_autoptr(xerror) child_error = NULL;
 *   x_autoptr(xdbus_connection_t) connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, NULL);
 *   xulong_t debug_controller_authorize_id = 0;
 *
 *   // Set up the debug controller.
 *   debug_controller = XDEBUG_CONTROLLER (xdebug_controller_dbus_new (priv->connection, NULL, &child_error));
 *   if (debug_controller == NULL)
 *     {
 *       xerror ("Could not register debug controller on bus: %s"),
 *                child_error->message);
 *     }
 *
 *   debug_controller_authorize_id = xsignal_connect (debug_controller,
 *                                                     "authorize",
 *                                                     G_CALLBACK (debug_controller_authorize_cb),
 *                                                     self);
 *
 *   static xboolean_t
 *   debug_controller_authorize_cb (xdebug_controller_dbus_t  *debug_controller,
 *                                  xdbus_method_invocation_t *invocation,
 *                                  xpointer_t               user_data)
 *   {
 *     x_autoptr(PolkitAuthority) authority = NULL;
 *     x_autoptr(PolkitSubject) subject = NULL;
 *     x_autoptr(PolkitAuthorizationResult) auth_result = NULL;
 *     x_autoptr(xerror) local_error = NULL;
 *     xdbus_message_t *message;
 *     GDBusMessageFlags message_flags;
 *     PolkitCheckAuthorizationFlags flags = POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE;
 *
 *     message = xdbus_method_invocation_get_message (invocation);
 *     message_flags = xdbus_message_get_flags (message);
 *
 *     authority = polkit_authority_get_sync (NULL, &local_error);
 *     if (authority == NULL)
 *       {
 *         g_warning ("Failed to get polkit authority: %s", local_error->message);
 *         return FALSE;
 *       }
 *
 *     if (message_flags & G_DBUS_MESSAGE_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION)
 *       flags |= POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION;
 *
 *     subject = polkit_system_bus_name_new (xdbus_method_invocation_get_sender (invocation));
 *
 *     auth_result = polkit_authority_check_authorization_sync (authority,
 *                                                              subject,
 *                                                              "com.example.MyService.set-debug-enabled",
 *                                                              NULL,
 *                                                              flags,
 *                                                              NULL,
 *                                                              &local_error);
 *     if (auth_result == NULL)
 *       {
 *         g_warning ("Failed to get check polkit authorization: %s", local_error->message);
 *         return FALSE;
 *       }
 *
 *     return polkit_authorization_result_get_is_authorized (auth_result);
 *   }
 * ]|
 *
 * Since: 2.72
 */

static const xchar_t org_gtk_Debugging_xml[] =
  "<node>"
    "<interface name='org.gtk.Debugging'>"
      "<property name='DebugEnabled' type='b' access='read'/>"
      "<method name='SetDebugEnabled'>"
        "<arg type='b' name='debug-enabled' direction='in'/>"
      "</method>"
    "</interface>"
  "</node>";

static xdbus_interface_info_t *org_gtk_Debugging;

#define XDEBUG_CONTROLLER_DBUS_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, xinitable_t))

static void xdebug_controller_dbus_iface_init (xdebug_controller_tInterface *iface);
static void xdebug_controller_dbus_initable_iface_init (xinitable_iface_t *iface);
static xboolean_t xdebug_controller_dbus_authorize_default (xdebug_controller_dbus_t  *self,
                                                           xdbus_method_invocation_t *invocation);

typedef enum
{
  PROP_CONNECTION = 1,
  /* Overrides: */
  PROP_DEBUG_ENABLED,
} xdebug_controller_dbus_property_t;

static xparam_spec_t *props[PROP_CONNECTION + 1] = { NULL, };

typedef enum
{
  SIGNAL_AUTHORIZE,
} xdebug_controller_dbus_signal_t;

static xuint_t signals[SIGNAL_AUTHORIZE + 1] = {0};

typedef struct
{
  xobject_t parent_instance;

  xcancellable_t *cancellable;  /* (owned) */
  xdbus_connection_t *connection;  /* (owned) */
  xuint_t object_id;
  xptr_array_t *pending_authorize_tasks;  /* (element-type GWeakRef) (owned) (nullable) */

  xboolean_t debug_enabled;
} xdebug_controller_dbus_private_t;

G_DEFINE_TYPE_WITH_CODE (xdebug_controller_dbus, xdebug_controller_dbus, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xdebug_controller_dbus)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xdebug_controller_dbus_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_DEBUG_CONTROLLER,
                                                xdebug_controller_dbus_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (XDEBUG_CONTROLLER_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "dbus",
                                                         30))

static void
xdebug_controller_dbus_init (xdebug_controller_dbus_t *self)
{
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  priv->cancellable = xcancellable_new ();
}

static void
set_debug_enabled (xdebug_controller_dbus_t *self,
                   xboolean_t              debug_enabled)
{
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  if (xcancellable_is_cancelled (priv->cancellable))
    return;

  if (debug_enabled != priv->debug_enabled)
    {
      xvariant_builder_t builder;

      priv->debug_enabled = debug_enabled;

      /* Change the default log writer’s behaviour in GLib. */
      g_log_set_debug_enabled (debug_enabled);

      /* Notify internally and externally of the property change. */
      xobject_notify (G_OBJECT (self), "debug-enabled");

      xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
      xvariant_builder_add (&builder, "{sv}", "DebugEnabled", xvariant_new_boolean (priv->debug_enabled));

      xdbus_connection_emit_signal (priv->connection,
                                     NULL,
                                     "/org/gtk/Debugging",
                                     "org.freedesktop.DBus.Properties",
                                     "PropertiesChanged",
                                     xvariant_new ("(sa{sv}as)",
                                                    "org.gtk.Debugging",
                                                    &builder,
                                                    NULL),
                                     NULL);

      g_debug ("Debug output %s", debug_enabled ? "enabled" : "disabled");
    }
}

/* Called in the #xmain_context_t which was default when the #xdebug_controller_dbus_t
 * was initialised. */
static xvariant_t *
dbus_get_property (xdbus_connection_t  *connection,
                   const xchar_t      *sender,
                   const xchar_t      *object_path,
                   const xchar_t      *interface_name,
                   const xchar_t      *property_name,
                   xerror_t          **error,
                   xpointer_t          user_data)
{
  xdebug_controller_dbus_t *self = user_data;
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  if (xstr_equal (property_name, "DebugEnabled"))
    return xvariant_new_boolean (priv->debug_enabled);

  g_assert_not_reached ();

  return NULL;
}

static GWeakRef *
weak_ref_new (xobject_t *obj)
{
  GWeakRef *weak_ref = g_new0 (GWeakRef, 1);

  g_weak_ref_init (weak_ref, obj);

  return g_steal_pointer (&weak_ref);
}

static void
weak_ref_free (GWeakRef *weak_ref)
{
  g_weak_ref_clear (weak_ref);
  g_free (weak_ref);
}

/* Called in the #xmain_context_t which was default when the #xdebug_controller_dbus_t
 * was initialised. */
static void
garbage_collect_weak_refs (xdebug_controller_dbus_t *self)
{
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);
  xuint_t i;

  if (priv->pending_authorize_tasks == NULL)
    return;

  /* Iterate in reverse order so that if we remove an element the hole won’t be
   * filled by an element we haven’t checked yet. */
  for (i = priv->pending_authorize_tasks->len; i > 0; i--)
    {
      GWeakRef *weak_ref = xptr_array_index (priv->pending_authorize_tasks, i - 1);
      xobject_t *obj = g_weak_ref_get (weak_ref);

      if (obj == NULL)
        xptr_array_remove_index_fast (priv->pending_authorize_tasks, i - 1);
      else
        xobject_unref (obj);
    }

  /* Don’t need to keep the array around any more if it’s empty. */
  if (priv->pending_authorize_tasks->len == 0)
    g_clear_pointer (&priv->pending_authorize_tasks, xptr_array_unref);
}

/* Called in a worker thread. */
static void
authorize_task_cb (xtask_t        *task,
                   xpointer_t      source_object,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  xdebug_controller_dbus_t *self = XDEBUG_CONTROLLER_DBUS (source_object);
  xdbus_method_invocation_t *invocation = G_DBUS_METHOD_INVOCATION (task_data);
  xboolean_t authorized = TRUE;

  xsignal_emit (self, signals[SIGNAL_AUTHORIZE], 0, invocation, &authorized);

  xtask_return_boolean (task, authorized);
}

/* Called in the #xmain_context_t which was default when the #xdebug_controller_dbus_t
 * was initialised. */
static void
authorize_cb (xobject_t      *object,
              xasync_result_t *result,
              xpointer_t      user_data)
{
  xdebug_controller_dbus_t *self = XDEBUG_CONTROLLER_DBUS (object);
  xdebug_controller_dbus_private_t *priv G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
  xtask_t *task = XTASK (result);
  xdbus_method_invocation_t *invocation = xtask_get_task_data (task);
  xvariant_t *parameters = xdbus_method_invocation_get_parameters (invocation);
  xboolean_t enabled = FALSE;
  xboolean_t authorized;

  priv = xdebug_controller_dbus_get_instance_private (self);
  authorized = xtask_propagate_boolean (task, NULL);

  if (!authorized)
    {
      xerror_t *local_error = xerror_new (G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED,
                                         _("Not authorized to change debug settings"));
      xdbus_method_invocation_take_error (invocation, g_steal_pointer (&local_error));
    }
  else
    {
      /* Update the property value. */
      xvariant_get (parameters, "(b)", &enabled);
      set_debug_enabled (self, enabled);

      xdbus_method_invocation_return_value (invocation, NULL);
    }

  /* The xtask_t will stay alive for a bit longer as the worker thread is
   * potentially still in the process of dropping its reference to it. */
  xassert (priv->pending_authorize_tasks != NULL && priv->pending_authorize_tasks->len > 0);
}

/* Called in the #xmain_context_t which was default when the #xdebug_controller_dbus_t
 * was initialised. */
static void
dbus_method_call (xdbus_connection_t       *connection,
                  const xchar_t           *sender,
                  const xchar_t           *object_path,
                  const xchar_t           *interface_name,
                  const xchar_t           *method_name,
                  xvariant_t              *parameters,
                  xdbus_method_invocation_t *invocation,
                  xpointer_t               user_data)
{
  xdebug_controller_dbus_t *self = user_data;
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);
  xdebug_controller_tDBusClass *klass = XDEBUG_CONTROLLER_DBUS_GET_CLASS (self);

  /* Only on the org.gtk.Debugging interface */
  if (xstr_equal (method_name, "SetDebugEnabled"))
    {
      xtask_t *task = NULL;

      task = xtask_new (self, priv->cancellable, authorize_cb, NULL);
      xtask_set_source_tag (task, dbus_method_call);
      xtask_set_task_data (task, xobject_ref (invocation), (xdestroy_notify_t) xobject_unref);

      /* Track the pending #xtask_t with a weak ref as its final strong ref could
       * be dropped from this thread or an arbitrary #xtask_t worker thread. The
       * weak refs will be evaluated in xdebug_controller_dbus_stop(). */
      if (priv->pending_authorize_tasks == NULL)
        priv->pending_authorize_tasks = xptr_array_new_with_free_func ((xdestroy_notify_t) weak_ref_free);
      xptr_array_add (priv->pending_authorize_tasks, weak_ref_new (G_OBJECT (task)));

      /* Take the opportunity to clean up a bit. */
      garbage_collect_weak_refs (self);

      /* Check the calling peer is authorised to change the debug mode. So that
       * the signal handler can block on checking polkit authorisation (which
       * definitely involves D-Bus calls, and might involve user interaction),
       * emit the #xdebug_controller_dbus_t::authorize signal in a worker thread, so
       * that handlers can synchronously block it. This is similar to how
       * #xdbus_interface_skeleton_t::g-authorize-method works.
       *
       * If no signal handlers are connected, don’t bother running the worker
       * thread, and just return a default value of %FALSE. Fail closed. */
      if (xsignal_has_handler_pending (self, signals[SIGNAL_AUTHORIZE], 0, FALSE) ||
          klass->authorize != xdebug_controller_dbus_authorize_default)
        xtask_run_in_thread (task, authorize_task_cb);
      else
        xtask_return_boolean (task, FALSE);

      g_clear_object (&task);
    }
  else
    g_assert_not_reached ();
}

static xboolean_t
xdebug_controller_dbus_initable_init (xinitable_t     *initable,
                                       xcancellable_t  *cancellable,
                                       xerror_t       **error)
{
  xdebug_controller_dbus_t *self = XDEBUG_CONTROLLER_DBUS (initable);
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);
  static const xdbus_interface_vtable_t vtable = {
    dbus_method_call,
    dbus_get_property,
    NULL /* set_property */,
    { 0 }
  };

  if (org_gtk_Debugging == NULL)
    {
      xerror_t *local_error = NULL;
      xdbus_node_info_t *info;

      info = g_dbus_node_info_new_for_xml (org_gtk_Debugging_xml, &local_error);
      if G_UNLIKELY (info == NULL)
        xerror ("%s", local_error->message);
      org_gtk_Debugging = g_dbus_node_info_lookup_interface (info, "org.gtk.Debugging");
      xassert (org_gtk_Debugging != NULL);
      g_dbus_interface_info_ref (org_gtk_Debugging);
      g_dbus_node_info_unref (info);
    }

  priv->object_id = xdbus_connection_register_object (priv->connection,
                                                       "/org/gtk/Debugging",
                                                       org_gtk_Debugging,
                                                       &vtable, self, NULL, error);
  if (priv->object_id == 0)
    return FALSE;

  return TRUE;
}

static void
xdebug_controller_dbus_get_property (xobject_t    *object,
                                      xuint_t       prop_id,
                                      xvalue_t     *value,
                                      xparam_spec_t *pspec)
{
  xdebug_controller_dbus_t *self = XDEBUG_CONTROLLER_DBUS (object);
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  switch ((xdebug_controller_dbus_property_t) prop_id)
    {
    case PROP_CONNECTION:
      xvalue_set_object (value, priv->connection);
      break;
    case PROP_DEBUG_ENABLED:
      xvalue_set_boolean (value, priv->debug_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdebug_controller_dbus_set_property (xobject_t      *object,
                                      xuint_t         prop_id,
                                      const xvalue_t *value,
                                      xparam_spec_t   *pspec)
{
  xdebug_controller_dbus_t *self = XDEBUG_CONTROLLER_DBUS (object);
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  switch ((xdebug_controller_dbus_property_t) prop_id)
    {
    case PROP_CONNECTION:
      /* Construct only */
      xassert (priv->connection == NULL);
      priv->connection = xvalue_dup_object (value);
      break;
    case PROP_DEBUG_ENABLED:
      set_debug_enabled (self, xvalue_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdebug_controller_dbus_dispose (xobject_t *object)
{
  xdebug_controller_dbus_t *self = XDEBUG_CONTROLLER_DBUS (object);
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  xdebug_controller_dbus_stop (self);
  xassert (priv->pending_authorize_tasks == NULL);
  g_clear_object (&priv->connection);
  g_clear_object (&priv->cancellable);

  XOBJECT_CLASS (xdebug_controller_dbus_parent_class)->dispose (object);
}

static xboolean_t
xdebug_controller_dbus_authorize_default (xdebug_controller_dbus_t  *self,
                                           xdbus_method_invocation_t *invocation)
{
  return TRUE;
}

static void
xdebug_controller_dbus_class_init (xdebug_controller_tDBusClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->get_property = xdebug_controller_dbus_get_property;
  xobject_class->set_property = xdebug_controller_dbus_set_property;
  xobject_class->dispose = xdebug_controller_dbus_dispose;

  klass->authorize = xdebug_controller_dbus_authorize_default;

  /**
   * xdebug_controller_dbus_t:connection:
   *
   * The D-Bus connection to expose the debugging interface on.
   *
   * Typically this will be the same connection (to the system or session bus)
   * which the rest of the application or service’s D-Bus objects are registered
   * on.
   *
   * Since: 2.72
   */
  props[PROP_CONNECTION] =
      xparam_spec_object ("connection", "D-Bus Connection",
                           "The D-Bus connection to expose the debugging interface on.",
                           XTYPE_DBUS_CONNECTION,
                           XPARAM_READWRITE |
                           XPARAM_CONSTRUCT_ONLY |
                           XPARAM_STATIC_STRINGS);

  xobject_class_install_properties (xobject_class, G_N_ELEMENTS (props), props);

  xobject_class_override_property (xobject_class, PROP_DEBUG_ENABLED, "debug-enabled");

  /**
   * xdebug_controller_dbus_t::authorize:
   * @controller: The #xdebug_controller_dbus_t emitting the signal.
   * @invocation: A #xdbus_method_invocation_t.
   *
   * Emitted when a D-Bus peer is trying to change the debug settings and used
   * to determine if that is authorized.
   *
   * This signal is emitted in a dedicated worker thread, so handlers are
   * allowed to perform blocking I/O. This means that, for example, it is
   * appropriate to call `polkit_authority_check_authorization_sync()` to check
   * authorization using polkit.
   *
   * If %FALSE is returned then no further handlers are run and the request to
   * change the debug settings is rejected.
   *
   * Otherwise, if %TRUE is returned, signal emission continues. If no handlers
   * return %FALSE, then the debug settings are allowed to be changed.
   *
   * Signal handlers must not modify @invocation, or cause it to return a value.
   *
   * The default class handler just returns %TRUE.
   *
   * Returns: %TRUE if the call is authorized, %FALSE otherwise.
   *
   * Since: 2.72
   */
  signals[SIGNAL_AUTHORIZE] =
    xsignal_new ("authorize",
                  XTYPE_DEBUG_CONTROLLER_DBUS,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xdebug_controller_tDBusClass, authorize),
                  _xsignal_accumulator_false_handled,
                  NULL,
                  _g_cclosure_marshal_BOOLEAN__OBJECT,
                  XTYPE_BOOLEAN,
                  1,
                  XTYPE_DBUS_METHOD_INVOCATION);
  xsignal_set_va_marshaller (signals[SIGNAL_AUTHORIZE],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_BOOLEAN__OBJECTv);
}

static void
xdebug_controller_dbus_iface_init (xdebug_controller_tInterface *iface)
{
}

static void
xdebug_controller_dbus_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = xdebug_controller_dbus_initable_init;
}

/**
 * xdebug_controller_dbus_new:
 * @connection: a #xdbus_connection_t to register the debug object on
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Create a new #xdebug_controller_dbus_t and synchronously initialize it.
 *
 * Initializing the object will export the debug object on @connection. The
 * object will remain registered until the last reference to the
 * #xdebug_controller_dbus_t is dropped.
 *
 * Initialization may fail if registering the object on @connection fails.
 *
 * Returns: (nullable) (transfer full): a new #xdebug_controller_dbus_t, or %NULL
 *   on failure
 * Since: 2.72
 */
xdebug_controller_dbus_t *
xdebug_controller_dbus_new (xdbus_connection_t  *connection,
                             xcancellable_t     *cancellable,
                             xerror_t          **error)
{
  xreturn_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  xreturn_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  xreturn_val_if_fail (error == NULL || *error == NULL, NULL);

  return xinitable_new (XTYPE_DEBUG_CONTROLLER_DBUS,
                         cancellable,
                         error,
                         "connection", connection,
                         NULL);
}

/**
 * xdebug_controller_dbus_stop:
 * @self: a #xdebug_controller_dbus_t
 *
 * Stop the debug controller, unregistering its object from the bus.
 *
 * Any pending method calls to the object will complete successfully, but new
 * ones will return an error. This method will block until all pending
 * #xdebug_controller_dbus_t::authorize signals have been handled. This is expected
 * to not take long, as it will just be waiting for threads to join. If any
 * #xdebug_controller_dbus_t::authorize signal handlers are still executing in other
 * threads, this will block until after they have returned.
 *
 * This method will be called automatically when the final reference to the
 * #xdebug_controller_dbus_t is dropped. You may want to call it explicitly to know
 * when the controller has been fully removed from the bus, or to break
 * reference count cycles.
 *
 * Calling this method from within a #xdebug_controller_dbus_t::authorize signal
 * handler will cause a deadlock and must not be done.
 *
 * Since: 2.72
 */
void
xdebug_controller_dbus_stop (xdebug_controller_dbus_t *self)
{
  xdebug_controller_dbus_private_t *priv = xdebug_controller_dbus_get_instance_private (self);

  xcancellable_cancel (priv->cancellable);

  if (priv->object_id != 0)
    {
      xdbus_connection_unregister_object (priv->connection, priv->object_id);
      priv->object_id = 0;
    }

  /* Wait for any pending authorize tasks to finish. These will just be waiting
   * for threads to join at this point, as the D-Bus object has been
   * unregistered and the cancellable cancelled.
   *
   * The loop will never terminate if xdebug_controller_dbus_stop() is
   * called from within an ::authorize callback.
   *
   * See discussion in https://gitlab.gnome.org/GNOME/glib/-/merge_requests/2486 */
  while (priv->pending_authorize_tasks != NULL)
    {
      garbage_collect_weak_refs (self);
      xthread_yield ();
    }
}
