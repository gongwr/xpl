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

#include "gcancellable.h"
#include "gdbusobjectmanager.h"
#include "gdbusobjectmanagerclient.h"
#include "gdbusobject.h"
#include "gdbusprivate.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "ginitable.h"
#include "gasyncresult.h"
#include "gasyncinitable.h"
#include "gdbusconnection.h"
#include "gdbusutils.h"
#include "gdbusobject.h"
#include "gdbusobjectproxy.h"
#include "gdbusproxy.h"
#include "gdbusinterface.h"

#include "glibintl.h"
#include "gmarshal-internal.h"

/**
 * SECTION:gdbusobjectmanagerclient
 * @short_description: Client-side object manager
 * @include: gio/gio.h
 *
 * #xdbus_object_manager_client_t is used to create, monitor and delete object
 * proxies for remote objects exported by a #xdbus_object_manager_server_t (or any
 * code implementing the
 * [org.freedesktop.DBus.ObjectManager](http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)
 * interface).
 *
 * Once an instance of this type has been created, you can connect to
 * the #xdbus_object_manager_t::object-added and
 * #xdbus_object_manager_t::object-removed signals and inspect the
 * #xdbus_object_proxy_t objects returned by
 * g_dbus_object_manager_get_objects().
 *
 * If the name for a #xdbus_object_manager_client_t is not owned by anyone at
 * object construction time, the default behavior is to request the
 * message bus to launch an owner for the name. This behavior can be
 * disabled using the %G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START
 * flag. It's also worth noting that this only works if the name of
 * interest is activatable in the first place. E.g. in some cases it
 * is not possible to launch an owner for the requested name. In this
 * case, #xdbus_object_manager_client_t object construction still succeeds but
 * there will be no object proxies
 * (e.g. g_dbus_object_manager_get_objects() returns the empty list) and
 * the #xdbus_object_manager_client_t:name-owner property is %NULL.
 *
 * The owner of the requested name can come and go (for example
 * consider a system service being restarted) – #xdbus_object_manager_client_t
 * handles this case too; simply connect to the #xobject_t::notify
 * signal to watch for changes on the #xdbus_object_manager_client_t:name-owner
 * property. When the name owner vanishes, the behavior is that
 * #xdbus_object_manager_client_t:name-owner is set to %NULL (this includes
 * emission of the #xobject_t::notify signal) and then
 * #xdbus_object_manager_t::object-removed signals are synthesized
 * for all currently existing object proxies. Since
 * #xdbus_object_manager_client_t:name-owner is %NULL when this happens, you can
 * use this information to disambiguate a synthesized signal from a
 * genuine signal caused by object removal on the remote
 * #xdbus_object_manager_t. Similarly, when a new name owner appears,
 * #xdbus_object_manager_t::object-added signals are synthesized
 * while #xdbus_object_manager_client_t:name-owner is still %NULL. Only when all
 * object proxies have been added, the #xdbus_object_manager_client_t:name-owner
 * is set to the new name owner (this includes emission of the
 * #xobject_t::notify signal).  Furthermore, you are guaranteed that
 * #xdbus_object_manager_client_t:name-owner will alternate between a name owner
 * (e.g. `:1.42`) and %NULL even in the case where
 * the name of interest is atomically replaced
 *
 * Ultimately, #xdbus_object_manager_client_t is used to obtain #xdbus_proxy_t
 * instances. All signals (including the
 * org.freedesktop.DBus.Properties::PropertiesChanged signal)
 * delivered to #xdbus_proxy_t instances are guaranteed to originate
 * from the name owner. This guarantee along with the behavior
 * described above, means that certain race conditions including the
 * "half the proxy is from the old owner and the other half is from
 * the new owner" problem cannot happen.
 *
 * To avoid having the application connect to signals on the returned
 * #xdbus_object_proxy_t and #xdbus_proxy_t objects, the
 * #xdbus_object_t::interface-added,
 * #xdbus_object_t::interface-removed,
 * #xdbus_proxy_t::g-properties-changed and
 * #xdbus_proxy_t::g-signal signals
 * are also emitted on the #xdbus_object_manager_client_t instance managing these
 * objects. The signals emitted are
 * #xdbus_object_manager_t::interface-added,
 * #xdbus_object_manager_t::interface-removed,
 * #xdbus_object_manager_client_t::interface-proxy-properties-changed and
 * #xdbus_object_manager_client_t::interface-proxy-signal.
 *
 * Note that all callbacks and signals are emitted in the
 * [thread-default main context][g-main-context-push-thread-default]
 * that the #xdbus_object_manager_client_t object was constructed
 * in. Additionally, the #xdbus_object_proxy_t and #xdbus_proxy_t objects
 * originating from the #xdbus_object_manager_client_t object will be created in
 * the same context and, consequently, will deliver signals in the
 * same main loop.
 */

struct _GDBusObjectManagerClientPrivate
{
  xmutex_t lock;

  xbus_type_t bus_type;
  xdbus_connection_t *connection;
  xchar_t *object_path;
  xchar_t *name;
  xchar_t *name_owner;
  GDBusObjectManagerClientFlags flags;

  xdbus_proxy_t *control_proxy;

  xhashtable_t *map_object_path_to_object_proxy;

  xuint_t signal_subscription_id;
  xchar_t *match_rule;

  xdbus_proxy_type_func_t get_proxy_type_func;
  xpointer_t get_proxy_type_user_data;
  xdestroy_notify_t get_proxy_type_destroy_notify;

  xulong_t name_owner_signal_id;
  xulong_t signal_signal_id;
  xcancellable_t *cancel;
};

enum
{
  PROP_0,
  PROP_BUS_TYPE,
  PROP_CONNECTION,
  PROP_FLAGS,
  PROP_OBJECT_PATH,
  PROP_NAME,
  PROP_NAME_OWNER,
  PROP_GET_PROXY_TYPE_FUNC,
  PROP_GET_PROXY_TYPE_USER_DATA,
  PROP_GET_PROXY_TYPE_DESTROY_NOTIFY
};

enum
{
  INTERFACE_PROXY_SIGNAL_SIGNAL,
  INTERFACE_PROXY_PROPERTIES_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

static void initable_iface_init       (xinitable_iface_t *initable_iface);
static void async_initable_iface_init (xasync_initable_iface_t *async_initable_iface);
static void dbus_object_manager_interface_init (xdbus_object_manager_iface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xdbus_object_manager_client, xdbus_object_manager_client, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xdbus_object_manager_client)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_ASYNC_INITABLE, async_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_DBUS_OBJECT_MANAGER, dbus_object_manager_interface_init))

static void maybe_unsubscribe_signals (xdbus_object_manager_client_t *manager);

static void on_control_proxy_g_signal (xdbus_proxy_t   *proxy,
                                       const xchar_t  *sender_name,
                                       const xchar_t  *signal_name,
                                       xvariant_t     *parameters,
                                       xpointer_t      user_data);

static void process_get_all_result (xdbus_object_manager_client_t *manager,
                                    xvariant_t          *value,
                                    const xchar_t       *name_owner);

static void
xdbus_object_manager_client_dispose (xobject_t *object)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (object);

  if (manager->priv->cancel != NULL)
    {
      xcancellable_cancel (manager->priv->cancel);
      g_clear_object (&manager->priv->cancel);
    }

  XOBJECT_CLASS (xdbus_object_manager_client_parent_class)->dispose (object);
}

static void
xdbus_object_manager_client_finalize (xobject_t *object)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (object);

  maybe_unsubscribe_signals (manager);

  xhash_table_unref (manager->priv->map_object_path_to_object_proxy);

  if (manager->priv->control_proxy != NULL && manager->priv->signal_signal_id != 0)
    xsignal_handler_disconnect (manager->priv->control_proxy,
                                 manager->priv->signal_signal_id);
  manager->priv->signal_signal_id = 0;

  if (manager->priv->control_proxy != NULL && manager->priv->name_owner_signal_id != 0)
    xsignal_handler_disconnect (manager->priv->control_proxy,
                                 manager->priv->name_owner_signal_id);
  manager->priv->name_owner_signal_id = 0;

  g_clear_object (&manager->priv->control_proxy);

  if (manager->priv->connection != NULL)
    xobject_unref (manager->priv->connection);
  g_free (manager->priv->object_path);
  g_free (manager->priv->name);
  g_free (manager->priv->name_owner);

  if (manager->priv->get_proxy_type_destroy_notify != NULL)
    manager->priv->get_proxy_type_destroy_notify (manager->priv->get_proxy_type_user_data);

  g_mutex_clear (&manager->priv->lock);

  if (XOBJECT_CLASS (xdbus_object_manager_client_parent_class)->finalize != NULL)
    XOBJECT_CLASS (xdbus_object_manager_client_parent_class)->finalize (object);
}

static void
xdbus_object_manager_client_get_property (xobject_t    *_object,
                                           xuint_t       prop_id,
                                           xvalue_t     *value,
                                           xparam_spec_t *pspec)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (_object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      xvalue_set_object (value, xdbus_object_manager_client_get_connection (manager));
      break;

    case PROP_OBJECT_PATH:
      xvalue_set_string (value, g_dbus_object_manager_get_object_path (G_DBUS_OBJECT_MANAGER (manager)));
      break;

    case PROP_NAME:
      xvalue_set_string (value, xdbus_object_manager_client_get_name (manager));
      break;

    case PROP_FLAGS:
      xvalue_set_flags (value, xdbus_object_manager_client_get_flags (manager));
      break;

    case PROP_NAME_OWNER:
      xvalue_take_string (value, xdbus_object_manager_client_get_name_owner (manager));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (manager, prop_id, pspec);
      break;
    }
}

static void
xdbus_object_manager_client_set_property (xobject_t       *_object,
                                           xuint_t          prop_id,
                                           const xvalue_t  *value,
                                           xparam_spec_t    *pspec)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (_object);
  const xchar_t *name;

  switch (prop_id)
    {
    case PROP_BUS_TYPE:
      manager->priv->bus_type = xvalue_get_enum (value);
      break;

    case PROP_CONNECTION:
      if (xvalue_get_object (value) != NULL)
        {
          xassert (manager->priv->connection == NULL);
          xassert (X_IS_DBUS_CONNECTION (xvalue_get_object (value)));
          manager->priv->connection = xvalue_dup_object (value);
        }
      break;

    case PROP_OBJECT_PATH:
      xassert (manager->priv->object_path == NULL);
      xassert (xvariant_is_object_path (xvalue_get_string (value)));
      manager->priv->object_path = xvalue_dup_string (value);
      break;

    case PROP_NAME:
      xassert (manager->priv->name == NULL);
      name = xvalue_get_string (value);
      xassert (name == NULL || g_dbus_is_name (name));
      manager->priv->name = xstrdup (name);
      break;

    case PROP_FLAGS:
      manager->priv->flags = xvalue_get_flags (value);
      break;

    case PROP_GET_PROXY_TYPE_FUNC:
      manager->priv->get_proxy_type_func = xvalue_get_pointer (value);
      break;

    case PROP_GET_PROXY_TYPE_USER_DATA:
      manager->priv->get_proxy_type_user_data = xvalue_get_pointer (value);
      break;

    case PROP_GET_PROXY_TYPE_DESTROY_NOTIFY:
      manager->priv->get_proxy_type_destroy_notify = xvalue_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (manager, prop_id, pspec);
      break;
    }
}

static void
xdbus_object_manager_client_class_init (GDBusObjectManagerClientClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->dispose      = xdbus_object_manager_client_dispose;
  xobject_class->finalize     = xdbus_object_manager_client_finalize;
  xobject_class->set_property = xdbus_object_manager_client_set_property;
  xobject_class->get_property = xdbus_object_manager_client_get_property;

  /**
   * xdbus_object_manager_client_t:connection:
   *
   * The #xdbus_connection_t to use.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_CONNECTION,
                                   xparam_spec_object ("connection",
                                                        "Connection",
                                                        "The connection to use",
                                                        XTYPE_DBUS_CONNECTION,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t:bus-type:
   *
   * If this property is not %G_BUS_TYPE_NONE, then
   * #xdbus_object_manager_client_t:connection must be %NULL and will be set to the
   * #xdbus_connection_t obtained by calling g_bus_get() with the value
   * of this property.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_BUS_TYPE,
                                   xparam_spec_enum ("bus-type",
                                                      "Bus Type",
                                                      "The bus to connect to, if any",
                                                      XTYPE_BUS_TYPE,
                                                      G_BUS_TYPE_NONE,
                                                      XPARAM_WRITABLE |
                                                      XPARAM_CONSTRUCT_ONLY |
                                                      XPARAM_STATIC_NAME |
                                                      XPARAM_STATIC_BLURB |
                                                      XPARAM_STATIC_NICK));

  /**
   * xdbus_object_manager_client_t:flags:
   *
   * Flags from the #GDBusObjectManagerClientFlags enumeration.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_FLAGS,
                                   xparam_spec_flags ("flags",
                                                       "Flags",
                                                       "Flags for the proxy manager",
                                                       XTYPE_DBUS_OBJECT_MANAGER_CLIENT_FLAGS,
                                                       G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                       XPARAM_READABLE |
                                                       XPARAM_WRITABLE |
                                                       XPARAM_CONSTRUCT_ONLY |
                                                       XPARAM_STATIC_NAME |
                                                       XPARAM_STATIC_BLURB |
                                                       XPARAM_STATIC_NICK));

  /**
   * xdbus_object_manager_client_t:object-path:
   *
   * The object path the manager is for.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_OBJECT_PATH,
                                   xparam_spec_string ("object-path",
                                                        "Object Path",
                                                        "The object path of the control object",
                                                        NULL,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t:name:
   *
   * The well-known name or unique name that the manager is for.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_NAME,
                                   xparam_spec_string ("name",
                                                        "Name",
                                                        "Name that the manager is for",
                                                        NULL,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t:name-owner:
   *
   * The unique name that owns #xdbus_object_manager_client_t:name or %NULL if
   * no-one is currently owning the name. Connect to the
   * #xobject_t::notify signal to track changes to this property.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_NAME_OWNER,
                                   xparam_spec_string ("name-owner",
                                                        "Name Owner",
                                                        "The owner of the name we are watching",
                                                        NULL,
                                                        XPARAM_READABLE |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t:get-proxy-type-func:
   *
   * The #xdbus_proxy_type_func_t to use when determining what #xtype_t to
   * use for interface proxies or %NULL.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_GET_PROXY_TYPE_FUNC,
                                   xparam_spec_pointer ("get-proxy-type-func",
                                                         "xdbus_proxy_type_func_t Function Pointer",
                                                         "The xdbus_proxy_type_func_t pointer to use",
                                                         XPARAM_READABLE |
                                                         XPARAM_WRITABLE |
                                                         XPARAM_CONSTRUCT_ONLY |
                                                         XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t:get-proxy-type-user-data:
   *
   * The #xpointer_t user_data to pass to #xdbus_object_manager_client_t:get-proxy-type-func.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_GET_PROXY_TYPE_USER_DATA,
                                   xparam_spec_pointer ("get-proxy-type-user-data",
                                                         "xdbus_proxy_type_func_t User Data",
                                                         "The xdbus_proxy_type_func_t user_data",
                                                         XPARAM_READABLE |
                                                         XPARAM_WRITABLE |
                                                         XPARAM_CONSTRUCT_ONLY |
                                                         XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t:get-proxy-type-destroy-notify:
   *
   * A #xdestroy_notify_t for the #xpointer_t user_data in #xdbus_object_manager_client_t:get-proxy-type-user-data.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_GET_PROXY_TYPE_DESTROY_NOTIFY,
                                   xparam_spec_pointer ("get-proxy-type-destroy-notify",
                                                         "xdbus_proxy_type_func_t user data free function",
                                                         "The xdbus_proxy_type_func_t user data free function",
                                                         XPARAM_READABLE |
                                                         XPARAM_WRITABLE |
                                                         XPARAM_CONSTRUCT_ONLY |
                                                         XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_manager_client_t::interface-proxy-signal:
   * @manager: The #xdbus_object_manager_client_t emitting the signal.
   * @object_proxy: The #xdbus_object_proxy_t on which an interface is emitting a D-Bus signal.
   * @interface_proxy: The #xdbus_proxy_t that is emitting a D-Bus signal.
   * @sender_name: The sender of the signal or NULL if the connection is not a bus connection.
   * @signal_name: The signal name.
   * @parameters: A #xvariant_t tuple with parameters for the signal.
   *
   * Emitted when a D-Bus signal is received on @interface_proxy.
   *
   * This signal exists purely as a convenience to avoid having to
   * connect signals to all interface proxies managed by @manager.
   *
   * This signal is emitted in the
   * [thread-default main context][g-main-context-push-thread-default]
   * that @manager was constructed in.
   *
   * Since: 2.30
   */
  signals[INTERFACE_PROXY_SIGNAL_SIGNAL] =
    xsignal_new (I_("interface-proxy-signal"),
                  XTYPE_DBUS_OBJECT_MANAGER_CLIENT,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GDBusObjectManagerClientClass, interface_proxy_signal),
                  NULL,
                  NULL,
                  _g_cclosure_marshal_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT,
                  XTYPE_NONE,
                  5,
                  XTYPE_DBUS_OBJECT_PROXY,
                  XTYPE_DBUS_PROXY,
                  XTYPE_STRING,
                  XTYPE_STRING,
                  XTYPE_VARIANT);
  xsignal_set_va_marshaller (signals[INTERFACE_PROXY_SIGNAL_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_OBJECT_STRING_STRING_VARIANTv);

  /**
   * xdbus_object_manager_client_t::interface-proxy-properties-changed:
   * @manager: The #xdbus_object_manager_client_t emitting the signal.
   * @object_proxy: The #xdbus_object_proxy_t on which an interface has properties that are changing.
   * @interface_proxy: The #xdbus_proxy_t that has properties that are changing.
   * @changed_properties: A #xvariant_t containing the properties that changed (type: `a{sv}`).
   * @invalidated_properties: (array zero-terminated=1) (element-type utf8): A %NULL terminated
   *   array of properties that were invalidated.
   *
   * Emitted when one or more D-Bus properties on proxy changes. The
   * local cache has already been updated when this signal fires. Note
   * that both @changed_properties and @invalidated_properties are
   * guaranteed to never be %NULL (either may be empty though).
   *
   * This signal exists purely as a convenience to avoid having to
   * connect signals to all interface proxies managed by @manager.
   *
   * This signal is emitted in the
   * [thread-default main context][g-main-context-push-thread-default]
   * that @manager was constructed in.
   *
   * Since: 2.30
   */
  signals[INTERFACE_PROXY_PROPERTIES_CHANGED_SIGNAL] =
    xsignal_new (I_("interface-proxy-properties-changed"),
                  XTYPE_DBUS_OBJECT_MANAGER_CLIENT,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GDBusObjectManagerClientClass, interface_proxy_properties_changed),
                  NULL,
                  NULL,
                  _g_cclosure_marshal_VOID__OBJECT_OBJECT_VARIANT_BOXED,
                  XTYPE_NONE,
                  4,
                  XTYPE_DBUS_OBJECT_PROXY,
                  XTYPE_DBUS_PROXY,
                  XTYPE_VARIANT,
                  XTYPE_STRV);
  xsignal_set_va_marshaller (signals[INTERFACE_PROXY_PROPERTIES_CHANGED_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_OBJECT_VARIANT_BOXEDv);
}

static void
xdbus_object_manager_client_init (xdbus_object_manager_client_t *manager)
{
  manager->priv = xdbus_object_manager_client_get_instance_private (manager);
  g_mutex_init (&manager->priv->lock);
  manager->priv->map_object_path_to_object_proxy = xhash_table_new_full (xstr_hash,
                                                                          xstr_equal,
                                                                          g_free,
                                                                          (xdestroy_notify_t) xobject_unref);
  manager->priv->cancel = xcancellable_new ();
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_object_manager_client_new_sync:
 * @connection: A #xdbus_connection_t.
 * @flags: Zero or more flags from the #GDBusObjectManagerClientFlags enumeration.
 * @name: (nullable): The owner of the control object (unique or well-known name), or %NULL when not using a message bus connection.
 * @object_path: The object path of the control object.
 * @get_proxy_type_func: (nullable): A #xdbus_proxy_type_func_t function or %NULL to always construct #xdbus_proxy_t proxies.
 * @get_proxy_type_user_data: User data to pass to @get_proxy_type_func.
 * @get_proxy_type_destroy_notify: (nullable): Free function for @get_proxy_type_user_data or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL
 * @error: Return location for error or %NULL.
 *
 * Creates a new #xdbus_object_manager_client_t object.
 *
 * This is a synchronous failable constructor - the calling thread is
 * blocked until a reply is received. See xdbus_object_manager_client_new()
 * for the asynchronous version.
 *
 * Returns: (transfer full) (type xdbus_object_manager_client_t): A
 *   #xdbus_object_manager_client_t object or %NULL if @error is set. Free
 *   with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_object_manager_t *
xdbus_object_manager_client_new_sync (xdbus_connection_t               *connection,
                                       GDBusObjectManagerClientFlags  flags,
                                       const xchar_t                   *name,
                                       const xchar_t                   *object_path,
                                       xdbus_proxy_type_func_t             get_proxy_type_func,
                                       xpointer_t                       get_proxy_type_user_data,
                                       xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                       xcancellable_t                  *cancellable,
                                       xerror_t                       **error)
{
  xinitable_t *initable;

  xreturn_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  xreturn_val_if_fail ((name == NULL && xdbus_connection_get_unique_name (connection) == NULL) ||
                        g_dbus_is_name (name), NULL);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), NULL);
  xreturn_val_if_fail (error == NULL || *error == NULL, NULL);

  initable = xinitable_new (XTYPE_DBUS_OBJECT_MANAGER_CLIENT,
                             cancellable,
                             error,
                             "connection", connection,
                             "flags", flags,
                             "name", name,
                             "object-path", object_path,
                             "get-proxy-type-func", get_proxy_type_func,
                             "get-proxy-type-user-data", get_proxy_type_user_data,
                             "get-proxy-type-destroy-notify", get_proxy_type_destroy_notify,
                             NULL);
  if (initable != NULL)
    return G_DBUS_OBJECT_MANAGER (initable);
  else
    return NULL;
}

/**
 * xdbus_object_manager_client_new:
 * @connection: A #xdbus_connection_t.
 * @flags: Zero or more flags from the #GDBusObjectManagerClientFlags enumeration.
 * @name: The owner of the control object (unique or well-known name).
 * @object_path: The object path of the control object.
 * @get_proxy_type_func: (nullable): A #xdbus_proxy_type_func_t function or %NULL to always construct #xdbus_proxy_t proxies.
 * @get_proxy_type_user_data: User data to pass to @get_proxy_type_func.
 * @get_proxy_type_destroy_notify: (nullable): Free function for @get_proxy_type_user_data or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL
 * @callback: A #xasync_ready_callback_t to call when the request is satisfied.
 * @user_data: The data to pass to @callback.
 *
 * Asynchronously creates a new #xdbus_object_manager_client_t object.
 *
 * This is an asynchronous failable constructor. When the result is
 * ready, @callback will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from. You can
 * then call xdbus_object_manager_client_new_finish() to get the result. See
 * xdbus_object_manager_client_new_sync() for the synchronous version.
 *
 * Since: 2.30
 */
void
xdbus_object_manager_client_new (xdbus_connection_t               *connection,
                                  GDBusObjectManagerClientFlags  flags,
                                  const xchar_t                   *name,
                                  const xchar_t                   *object_path,
                                  xdbus_proxy_type_func_t             get_proxy_type_func,
                                  xpointer_t                       get_proxy_type_user_data,
                                  xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                  xcancellable_t                  *cancellable,
                                  xasync_ready_callback_t            callback,
                                  xpointer_t                       user_data)
{
  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail ((name == NULL && xdbus_connection_get_unique_name (connection) == NULL) ||
                        g_dbus_is_name (name));
  g_return_if_fail (xvariant_is_object_path (object_path));

  xasync_initable_new_async (XTYPE_DBUS_OBJECT_MANAGER_CLIENT,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "connection", connection,
                              "flags", flags,
                              "name", name,
                              "object-path", object_path,
                              "get-proxy-type-func", get_proxy_type_func,
                              "get-proxy-type-user-data", get_proxy_type_user_data,
                              "get-proxy-type-destroy-notify", get_proxy_type_destroy_notify,
                              NULL);
}

/**
 * xdbus_object_manager_client_new_finish:
 * @res: A #xasync_result_t obtained from the #xasync_ready_callback_t passed to xdbus_object_manager_client_new().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with xdbus_object_manager_client_new().
 *
 * Returns: (transfer full) (type xdbus_object_manager_client_t): A
 *   #xdbus_object_manager_client_t object or %NULL if @error is set. Free
 *   with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_object_manager_t *
xdbus_object_manager_client_new_finish (xasync_result_t   *res,
                                         xerror_t        **error)
{
  xobject_t *object;
  xobject_t *source_object;

  source_object = xasync_result_get_source_object (res);
  xassert (source_object != NULL);

  object = xasync_initable_new_finish (XASYNC_INITABLE (source_object),
                                        res,
                                        error);
  xobject_unref (source_object);

  if (object != NULL)
    return G_DBUS_OBJECT_MANAGER (object);
  else
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_object_manager_client_new_for_bus_sync:
 * @bus_type: A #xbus_type_t.
 * @flags: Zero or more flags from the #GDBusObjectManagerClientFlags enumeration.
 * @name: The owner of the control object (unique or well-known name).
 * @object_path: The object path of the control object.
 * @get_proxy_type_func: (nullable): A #xdbus_proxy_type_func_t function or %NULL to always construct #xdbus_proxy_t proxies.
 * @get_proxy_type_user_data: User data to pass to @get_proxy_type_func.
 * @get_proxy_type_destroy_notify: (nullable): Free function for @get_proxy_type_user_data or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL
 * @error: Return location for error or %NULL.
 *
 * Like xdbus_object_manager_client_new_sync() but takes a #xbus_type_t instead
 * of a #xdbus_connection_t.
 *
 * This is a synchronous failable constructor - the calling thread is
 * blocked until a reply is received. See xdbus_object_manager_client_new_for_bus()
 * for the asynchronous version.
 *
 * Returns: (transfer full) (type xdbus_object_manager_client_t): A
 *   #xdbus_object_manager_client_t object or %NULL if @error is set. Free
 *   with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_object_manager_t *
xdbus_object_manager_client_new_for_bus_sync (xbus_type_t                       bus_type,
                                               GDBusObjectManagerClientFlags  flags,
                                               const xchar_t                   *name,
                                               const xchar_t                   *object_path,
                                               xdbus_proxy_type_func_t             get_proxy_type_func,
                                               xpointer_t                       get_proxy_type_user_data,
                                               xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                               xcancellable_t                  *cancellable,
                                               xerror_t                       **error)
{
  xinitable_t *initable;

  xreturn_val_if_fail (bus_type != G_BUS_TYPE_NONE, NULL);
  xreturn_val_if_fail (g_dbus_is_name (name), NULL);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), NULL);
  xreturn_val_if_fail (error == NULL || *error == NULL, NULL);

  initable = xinitable_new (XTYPE_DBUS_OBJECT_MANAGER_CLIENT,
                             cancellable,
                             error,
                             "bus-type", bus_type,
                             "flags", flags,
                             "name", name,
                             "object-path", object_path,
                             "get-proxy-type-func", get_proxy_type_func,
                             "get-proxy-type-user-data", get_proxy_type_user_data,
                             "get-proxy-type-destroy-notify", get_proxy_type_destroy_notify,
                             NULL);
  if (initable != NULL)
    return G_DBUS_OBJECT_MANAGER (initable);
  else
    return NULL;
}

/**
 * xdbus_object_manager_client_new_for_bus:
 * @bus_type: A #xbus_type_t.
 * @flags: Zero or more flags from the #GDBusObjectManagerClientFlags enumeration.
 * @name: The owner of the control object (unique or well-known name).
 * @object_path: The object path of the control object.
 * @get_proxy_type_func: (nullable): A #xdbus_proxy_type_func_t function or %NULL to always construct #xdbus_proxy_t proxies.
 * @get_proxy_type_user_data: User data to pass to @get_proxy_type_func.
 * @get_proxy_type_destroy_notify: (nullable): Free function for @get_proxy_type_user_data or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL
 * @callback: A #xasync_ready_callback_t to call when the request is satisfied.
 * @user_data: The data to pass to @callback.
 *
 * Like xdbus_object_manager_client_new() but takes a #xbus_type_t instead of a
 * #xdbus_connection_t.
 *
 * This is an asynchronous failable constructor. When the result is
 * ready, @callback will be invoked in the
 * [thread-default main loop][g-main-context-push-thread-default]
 * of the thread you are calling this method from. You can
 * then call xdbus_object_manager_client_new_for_bus_finish() to get the result. See
 * xdbus_object_manager_client_new_for_bus_sync() for the synchronous version.
 *
 * Since: 2.30
 */
void
xdbus_object_manager_client_new_for_bus (xbus_type_t                       bus_type,
                                          GDBusObjectManagerClientFlags  flags,
                                          const xchar_t                   *name,
                                          const xchar_t                   *object_path,
                                          xdbus_proxy_type_func_t             get_proxy_type_func,
                                          xpointer_t                       get_proxy_type_user_data,
                                          xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                          xcancellable_t                  *cancellable,
                                          xasync_ready_callback_t            callback,
                                          xpointer_t                       user_data)
{
  g_return_if_fail (bus_type != G_BUS_TYPE_NONE);
  g_return_if_fail (g_dbus_is_name (name));
  g_return_if_fail (xvariant_is_object_path (object_path));

  xasync_initable_new_async (XTYPE_DBUS_OBJECT_MANAGER_CLIENT,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "bus-type", bus_type,
                              "flags", flags,
                              "name", name,
                              "object-path", object_path,
                              "get-proxy-type-func", get_proxy_type_func,
                              "get-proxy-type-user-data", get_proxy_type_user_data,
                              "get-proxy-type-destroy-notify", get_proxy_type_destroy_notify,
                              NULL);
}

/**
 * xdbus_object_manager_client_new_for_bus_finish:
 * @res: A #xasync_result_t obtained from the #xasync_ready_callback_t passed to xdbus_object_manager_client_new_for_bus().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with xdbus_object_manager_client_new_for_bus().
 *
 * Returns: (transfer full) (type xdbus_object_manager_client_t): A
 *   #xdbus_object_manager_client_t object or %NULL if @error is set. Free
 *   with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_object_manager_t *
xdbus_object_manager_client_new_for_bus_finish (xasync_result_t   *res,
                                                 xerror_t        **error)
{
  xobject_t *object;
  xobject_t *source_object;

  source_object = xasync_result_get_source_object (res);
  xassert (source_object != NULL);

  object = xasync_initable_new_finish (XASYNC_INITABLE (source_object),
                                        res,
                                        error);
  xobject_unref (source_object);

  if (object != NULL)
    return G_DBUS_OBJECT_MANAGER (object);
  else
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_object_manager_client_get_connection:
 * @manager: A #xdbus_object_manager_client_t
 *
 * Gets the #xdbus_connection_t used by @manager.
 *
 * Returns: (transfer none): A #xdbus_connection_t object. Do not free,
 *   the object belongs to @manager.
 *
 * Since: 2.30
 */
xdbus_connection_t *
xdbus_object_manager_client_get_connection (xdbus_object_manager_client_t *manager)
{
  xdbus_connection_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager), NULL);
  g_mutex_lock (&manager->priv->lock);
  ret = manager->priv->connection;
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}

/**
 * xdbus_object_manager_client_get_name:
 * @manager: A #xdbus_object_manager_client_t
 *
 * Gets the name that @manager is for, or %NULL if not a message bus
 * connection.
 *
 * Returns: A unique or well-known name. Do not free, the string
 * belongs to @manager.
 *
 * Since: 2.30
 */
const xchar_t *
xdbus_object_manager_client_get_name (xdbus_object_manager_client_t *manager)
{
  const xchar_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager), NULL);
  g_mutex_lock (&manager->priv->lock);
  ret = manager->priv->name;
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}

/**
 * xdbus_object_manager_client_get_flags:
 * @manager: A #xdbus_object_manager_client_t
 *
 * Gets the flags that @manager was constructed with.
 *
 * Returns: Zero of more flags from the #GDBusObjectManagerClientFlags
 * enumeration.
 *
 * Since: 2.30
 */
GDBusObjectManagerClientFlags
xdbus_object_manager_client_get_flags (xdbus_object_manager_client_t *manager)
{
  GDBusObjectManagerClientFlags ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager), G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE);
  g_mutex_lock (&manager->priv->lock);
  ret = manager->priv->flags;
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}

/**
 * xdbus_object_manager_client_get_name_owner:
 * @manager: A #xdbus_object_manager_client_t.
 *
 * The unique name that owns the name that @manager is for or %NULL if
 * no-one currently owns that name. You can connect to the
 * #xobject_t::notify signal to track changes to the
 * #xdbus_object_manager_client_t:name-owner property.
 *
 * Returns: (nullable): The name owner or %NULL if no name owner
 * exists. Free with g_free().
 *
 * Since: 2.30
 */
xchar_t *
xdbus_object_manager_client_get_name_owner (xdbus_object_manager_client_t *manager)
{
  xchar_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager), NULL);
  g_mutex_lock (&manager->priv->lock);
  ret = xstrdup (manager->priv->name_owner);
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/* signal handler for all objects we manage - we dispatch signals
 * from here to the objects
 */
static void
signal_cb (xdbus_connection_t *connection,
           const xchar_t     *sender_name,
           const xchar_t     *object_path,
           const xchar_t     *interface_name,
           const xchar_t     *signal_name,
           xvariant_t        *parameters,
           xpointer_t         user_data)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (user_data);
  xdbus_object_proxy_t *object_proxy;
  xdbus_interface_t *interface;

  g_mutex_lock (&manager->priv->lock);
  object_proxy = xhash_table_lookup (manager->priv->map_object_path_to_object_proxy, object_path);
  if (object_proxy == NULL)
    {
      g_mutex_unlock (&manager->priv->lock);
      goto out;
    }
  xobject_ref (object_proxy);
  g_mutex_unlock (&manager->priv->lock);

  //g_debug ("yay, signal_cb %s %s: %s\n", signal_name, object_path, xvariant_print (parameters, TRUE));

  xobject_ref (manager);
  if (xstrcmp0 (interface_name, "org.freedesktop.DBus.Properties") == 0)
    {
      if (xstrcmp0 (signal_name, "PropertiesChanged") == 0)
        {
          const xchar_t *interface_name;
          xvariant_t *changed_properties;
          const xchar_t **invalidated_properties;

          xvariant_get (parameters,
                         "(&s@a{sv}^a&s)",
                         &interface_name,
                         &changed_properties,
                         &invalidated_properties);

          interface = g_dbus_object_get_interface (G_DBUS_OBJECT (object_proxy), interface_name);
          if (interface != NULL)
            {
              xvariant_iter_t property_iter;
              const xchar_t *property_name;
              xvariant_t *property_value;
              xuint_t n;

              /* update caches... */
              xvariant_iter_init (&property_iter, changed_properties);
              while (xvariant_iter_next (&property_iter,
                                          "{&sv}",
                                          &property_name,
                                          &property_value))
                {
                  xdbus_proxy_set_cached_property (G_DBUS_PROXY (interface),
                                                    property_name,
                                                    property_value);
                  xvariant_unref (property_value);
                }

              for (n = 0; invalidated_properties[n] != NULL; n++)
                {
                  xdbus_proxy_set_cached_property (G_DBUS_PROXY (interface),
                                                    invalidated_properties[n],
                                                    NULL);
                }
              /* ... and then synthesize the signal */
              xsignal_emit_by_name (interface,
                                     "g-properties-changed",
                                     changed_properties,
                                     invalidated_properties);
              xsignal_emit (manager,
                             signals[INTERFACE_PROXY_PROPERTIES_CHANGED_SIGNAL],
                             0,
                             object_proxy,
                             interface,
                             changed_properties,
                             invalidated_properties);
              xobject_unref (interface);
            }
          xvariant_unref (changed_properties);
          g_free (invalidated_properties);
        }
    }
  else
    {
      /* regular signal - just dispatch it */
      interface = g_dbus_object_get_interface (G_DBUS_OBJECT (object_proxy), interface_name);
      if (interface != NULL)
        {
          xsignal_emit_by_name (interface,
                                 "g-signal",
                                 sender_name,
                                 signal_name,
                                 parameters);
          xsignal_emit (manager,
                         signals[INTERFACE_PROXY_SIGNAL_SIGNAL],
                         0,
                         object_proxy,
                         interface,
                         sender_name,
                         signal_name,
                         parameters);
          xobject_unref (interface);
        }
    }
  xobject_unref (manager);

 out:
  g_clear_object (&object_proxy);
}

static void
subscribe_signals (xdbus_object_manager_client_t *manager,
                   const xchar_t *name_owner)
{
  xerror_t *error = NULL;
  xvariant_t *ret;

  g_return_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_return_if_fail (manager->priv->signal_subscription_id == 0);
  g_return_if_fail (name_owner == NULL || g_dbus_is_unique_name (name_owner));

  if (name_owner != NULL)
    {
      /* Only add path_namespace if it's non-'/'. This removes a no-op key from
       * the match rule, and also works around a D-Bus bug where
       * path_namespace='/' matches nothing in D-Bus versions < 1.6.18.
       *
       * See: https://bugs.freedesktop.org/show_bug.cgi?id=70799 */
      if (xstr_equal (manager->priv->object_path, "/"))
        {
          manager->priv->match_rule = xstrdup_printf ("type='signal',sender='%s'",
                                                       name_owner);
        }
      else
        {
          manager->priv->match_rule = xstrdup_printf ("type='signal',sender='%s',path_namespace='%s'",
                                                       name_owner, manager->priv->object_path);
        }

      /* The bus daemon may not implement path_namespace so gracefully
       * handle this by using a fallback triggered if @error is set. */
      ret = xdbus_connection_call_sync (manager->priv->connection,
                                         "org.freedesktop.DBus",
                                         "/org/freedesktop/DBus",
                                         "org.freedesktop.DBus",
                                         "AddMatch",
                                         xvariant_new ("(s)",
                                                        manager->priv->match_rule),
                                         NULL, /* reply_type */
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1, /* default timeout */
                                         NULL, /* TODO: Cancellable */
                                         &error);

      /* yay, bus daemon supports path_namespace */
      if (ret != NULL)
        xvariant_unref (ret);
    }

  if (error == NULL)
    {
      /* still need to ask xdbus_connection_t for the callbacks */
      manager->priv->signal_subscription_id =
        xdbus_connection_signal_subscribe (manager->priv->connection,
                                            name_owner,
                                            NULL, /* interface */
                                            NULL, /* member */
                                            NULL, /* path - TODO: really want wildcard support here */
                                            NULL, /* arg0 */
                                            G_DBUS_SIGNAL_FLAGS_NONE |
                                            G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                            signal_cb,
                                            manager,
                                            NULL); /* user_data_free_func */

    }
  else
    {
      /* TODO: we could report this to the user
      g_warning ("Message bus daemon does not support path_namespace: %s (%s %d)",
                 error->message,
                 g_quark_to_string (error->domain),
                 error->code);
      */

      xerror_free (error);

      /* no need to call RemoveMatch when done since it didn't work */
      g_free (manager->priv->match_rule);
      manager->priv->match_rule = NULL;

      /* Fallback is to subscribe to *all* signals from the name owner which
       * is rather wasteful. It's probably not a big practical problem because
       * users typically want all objects that the name owner supplies.
       */
      manager->priv->signal_subscription_id =
        xdbus_connection_signal_subscribe (manager->priv->connection,
                                            name_owner,
                                            NULL, /* interface */
                                            NULL, /* member */
                                            NULL, /* path - TODO: really want wildcard support here */
                                            NULL, /* arg0 */
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            signal_cb,
                                            manager,
                                            NULL); /* user_data_free_func */
    }
}

static void
maybe_unsubscribe_signals (xdbus_object_manager_client_t *manager)
{
  g_return_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager));

  if (manager->priv->signal_subscription_id > 0)
    {
      xdbus_connection_signal_unsubscribe (manager->priv->connection,
                                            manager->priv->signal_subscription_id);
      manager->priv->signal_subscription_id = 0;
    }

  if (manager->priv->match_rule != NULL)
    {
      /* Since the AddMatch call succeeded this is guaranteed to not
       * fail - therefore, don't bother checking the return value
       */
      xdbus_connection_call (manager->priv->connection,
                              "org.freedesktop.DBus",
                              "/org/freedesktop/DBus",
                              "org.freedesktop.DBus",
                              "RemoveMatch",
                              xvariant_new ("(s)",
                                             manager->priv->match_rule),
                              NULL, /* reply_type */
                              G_DBUS_CALL_FLAGS_NONE,
                              -1, /* default timeout */
                              NULL, /* xcancellable_t */
                              NULL, /* xasync_ready_callback_t */
                              NULL); /* user data */
      g_free (manager->priv->match_rule);
      manager->priv->match_rule = NULL;
    }

}

/* ---------------------------------------------------------------------------------------------------- */

static GWeakRef *
weak_ref_new (xobject_t *object)
{
  GWeakRef *weak_ref = g_new0 (GWeakRef, 1);
  g_weak_ref_init (weak_ref, object);
  return g_steal_pointer (&weak_ref);
}

static void
weak_ref_free (GWeakRef *weak_ref)
{
  g_weak_ref_clear (weak_ref);
  g_free (weak_ref);
}

static void
on_get_managed_objects_finish (xobject_t      *source,
                               xasync_result_t *result,
                               xpointer_t      user_data)
{

  xdbus_proxy_t *proxy = G_DBUS_PROXY (source);
  GWeakRef *manager_weak = user_data;
  xdbus_object_manager_client_t *manager;
  xerror_t *error = NULL;
  xvariant_t *value = NULL;
  xchar_t *new_name_owner = NULL;

  value = xdbus_proxy_call_finish (proxy, result, &error);

  manager = G_DBUS_OBJECT_MANAGER_CLIENT (g_weak_ref_get (manager_weak));
  /* Manager got disposed, nothing to do */
  if (manager == NULL)
    {
      goto out;
    }

  new_name_owner = xdbus_proxy_get_name_owner (manager->priv->control_proxy);
  if (value == NULL)
    {
      maybe_unsubscribe_signals (manager);
      if (!xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_warning ("Error calling GetManagedObjects() when name owner %s for name %s came back: %s",
                     new_name_owner,
                     manager->priv->name,
                     error->message);
        }
    }
  else
    {
      process_get_all_result (manager, value, new_name_owner);
    }

  /* do the :name-owner notify *AFTER* emitting ::object-proxy-added signals - this
   * way the user knows that the signals were emitted because the name owner came back
   */
  g_mutex_lock (&manager->priv->lock);
  manager->priv->name_owner = g_steal_pointer (&new_name_owner);
  g_mutex_unlock (&manager->priv->lock);
  xobject_notify (G_OBJECT (manager), "name-owner");

  xobject_unref (manager);
 out:
  g_clear_error (&error);
  g_clear_pointer (&value, xvariant_unref);
  weak_ref_free (manager_weak);
}

static void
on_notify_g_name_owner (xobject_t    *object,
                        xparam_spec_t *pspec,
                        xpointer_t    user_data)
{
  GWeakRef *manager_weak = user_data;
  xdbus_object_manager_client_t *manager = NULL;
  xchar_t *old_name_owner;
  xchar_t *new_name_owner;

  manager = G_DBUS_OBJECT_MANAGER_CLIENT (g_weak_ref_get (manager_weak));
  if (manager == NULL)
    return;

  g_mutex_lock (&manager->priv->lock);
  old_name_owner = manager->priv->name_owner;
  new_name_owner = xdbus_proxy_get_name_owner (manager->priv->control_proxy);
  manager->priv->name_owner = NULL;

  if (xstrcmp0 (old_name_owner, new_name_owner) != 0)
    {
      xlist_t *l;
      xlist_t *proxies;

      /* remote manager changed; nuke all local proxies  */
      proxies = xhash_table_get_values (manager->priv->map_object_path_to_object_proxy);
      xlist_foreach (proxies, (GFunc) xobject_ref, NULL);
      xhash_table_remove_all (manager->priv->map_object_path_to_object_proxy);

      g_mutex_unlock (&manager->priv->lock);

      /* do the :name-owner notify with a NULL name - this way the user knows
       * the ::object-proxy-removed following is because the name owner went
       * away
       */
      xobject_notify (G_OBJECT (manager), "name-owner");

      for (l = proxies; l != NULL; l = l->next)
        {
          xdbus_object_proxy_t *object_proxy = G_DBUS_OBJECT_PROXY (l->data);
          xsignal_emit_by_name (manager, "object-removed", object_proxy);
        }
      xlist_free_full (proxies, xobject_unref);

      /* nuke local filter */
      maybe_unsubscribe_signals (manager);
    }
  else
    {
      g_mutex_unlock (&manager->priv->lock);
    }

  if (new_name_owner != NULL)
    {
      //g_debug ("repopulating for %s", new_name_owner);

      subscribe_signals (manager,
                         new_name_owner);
      xdbus_proxy_call (manager->priv->control_proxy,
                         "GetManagedObjects",
                         NULL, /* parameters */
                         G_DBUS_CALL_FLAGS_NONE,
                         -1,
                         manager->priv->cancel,
                         on_get_managed_objects_finish,
                         weak_ref_new (G_OBJECT (manager)));
    }
  g_free (new_name_owner);
  g_free (old_name_owner);
  xobject_unref (manager);
}

static xboolean_t
initable_init (xinitable_t     *initable,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (initable);
  xboolean_t ret;
  xvariant_t *value;
  xdbus_proxy_flags_t proxy_flags;

  ret = FALSE;

  if (manager->priv->bus_type != G_BUS_TYPE_NONE)
    {
      xassert (manager->priv->connection == NULL);
      manager->priv->connection = g_bus_get_sync (manager->priv->bus_type, cancellable, error);
      if (manager->priv->connection == NULL)
        goto out;
    }

  proxy_flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
  if (manager->priv->flags & G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START)
    proxy_flags |= G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;

  manager->priv->control_proxy = xdbus_proxy_new_sync (manager->priv->connection,
                                                        proxy_flags,
                                                        NULL, /* xdbus_interface_info_t* */
                                                        manager->priv->name,
                                                        manager->priv->object_path,
                                                        "org.freedesktop.DBus.ObjectManager",
                                                        cancellable,
                                                        error);
  if (manager->priv->control_proxy == NULL)
    goto out;

  /* Use weak refs here. The @control_proxy will emit its signals in the current
   * #xmain_context_t (since we constructed it just above). However, the user may
   * drop the last external reference to this #xdbus_object_manager_client_t in
   * another thread between a signal being emitted and scheduled in an idle
   * callback in this #xmain_context_t, and that idle callback being invoked. We
   * can’t use a strong reference here, as there’s no
   * xdbus_object_manager_client_disconnect() (or similar) method to tell us
   * when the last external reference to this object has been dropped, so we
   * can’t break a strong reference count cycle. So use weak refs. */
  manager->priv->name_owner_signal_id =
      xsignal_connect_data (G_OBJECT (manager->priv->control_proxy),
                            "notify::g-name-owner",
                            G_CALLBACK (on_notify_g_name_owner),
                            weak_ref_new (G_OBJECT (manager)),
                            (xclosure_notify_t) weak_ref_free,
                            0  /* flags */);

  manager->priv->signal_signal_id =
      xsignal_connect_data (manager->priv->control_proxy,
                            "g-signal",
                            G_CALLBACK (on_control_proxy_g_signal),
                            weak_ref_new (G_OBJECT (manager)),
                            (xclosure_notify_t) weak_ref_free,
                            0  /* flags */);

  manager->priv->name_owner = xdbus_proxy_get_name_owner (manager->priv->control_proxy);
  if (manager->priv->name_owner == NULL && manager->priv->name != NULL)
    {
      /* it's perfectly fine if there's no name owner.. we're just going to
       * wait until one is ready
       */
    }
  else
    {
      /* yay, we can get the objects */
      subscribe_signals (manager,
                         manager->priv->name_owner);
      value = xdbus_proxy_call_sync (manager->priv->control_proxy,
                                      "GetManagedObjects",
                                      NULL, /* parameters */
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      cancellable,
                                      error);
      if (value == NULL)
        {
          maybe_unsubscribe_signals (manager);

          g_warn_if_fail (manager->priv->signal_signal_id != 0);
          xsignal_handler_disconnect (manager->priv->control_proxy,
                                       manager->priv->signal_signal_id);
          manager->priv->signal_signal_id = 0;

          g_warn_if_fail (manager->priv->name_owner_signal_id != 0);
          xsignal_handler_disconnect (manager->priv->control_proxy,
                                       manager->priv->name_owner_signal_id);
          manager->priv->name_owner_signal_id = 0;

          xobject_unref (manager->priv->control_proxy);
          manager->priv->control_proxy = NULL;

          goto out;
        }

      process_get_all_result (manager, value, manager->priv->name_owner);
      xvariant_unref (value);
    }

  ret = TRUE;

 out:
  return ret;
}

static void
initable_iface_init (xinitable_iface_t *initable_iface)
{
  initable_iface->init = initable_init;
}

static void
async_initable_iface_init (xasync_initable_iface_t *async_initable_iface)
{
  /* for now, just use default: run xinitable_t code in thread */
}

/* ---------------------------------------------------------------------------------------------------- */

static void
add_interfaces (xdbus_object_manager_client_t *manager,
                const xchar_t       *object_path,
                xvariant_t          *ifaces_and_properties,
                const xchar_t       *name_owner)
{
  xdbus_object_proxy_t *op;
  xboolean_t added;
  xvariant_iter_t iter;
  const xchar_t *interface_name;
  xvariant_t *properties;
  xlist_t *interface_added_signals, *l;
  xdbus_proxy_t *interface_proxy;

  g_return_if_fail (name_owner == NULL || g_dbus_is_unique_name (name_owner));

  g_mutex_lock (&manager->priv->lock);

  interface_added_signals = NULL;
  added = FALSE;

  op = xhash_table_lookup (manager->priv->map_object_path_to_object_proxy, object_path);
  if (op == NULL)
    {
      xtype_t object_proxy_type;
      if (manager->priv->get_proxy_type_func != NULL)
        {
          object_proxy_type = manager->priv->get_proxy_type_func (manager,
                                                                  object_path,
                                                                  NULL,
                                                                  manager->priv->get_proxy_type_user_data);
          g_warn_if_fail (xtype_is_a (object_proxy_type, XTYPE_DBUS_OBJECT_PROXY));
        }
      else
        {
          object_proxy_type = XTYPE_DBUS_OBJECT_PROXY;
        }
      op = xobject_new (object_proxy_type,
                         "g-connection", manager->priv->connection,
                         "g-object-path", object_path,
                         NULL);
      added = TRUE;
    }
  xobject_ref (op);

  xvariant_iter_init (&iter, ifaces_and_properties);
  while (xvariant_iter_next (&iter,
                              "{&s@a{sv}}",
                              &interface_name,
                              &properties))
    {
      xerror_t *error;
      xtype_t interface_proxy_type;

      if (manager->priv->get_proxy_type_func != NULL)
        {
          interface_proxy_type = manager->priv->get_proxy_type_func (manager,
                                                                     object_path,
                                                                     interface_name,
                                                                     manager->priv->get_proxy_type_user_data);
          g_warn_if_fail (xtype_is_a (interface_proxy_type, XTYPE_DBUS_PROXY));
        }
      else
        {
          interface_proxy_type = XTYPE_DBUS_PROXY;
        }

      /* this is fine - there is no blocking IO because we pass DO_NOT_LOAD_PROPERTIES and
       * DO_NOT_CONNECT_SIGNALS and use a unique name
       */
      error = NULL;
      interface_proxy = xinitable_new (interface_proxy_type,
                                        NULL, /* xcancellable_t */
                                        &error,
                                        "g-connection", manager->priv->connection,
                                        "g-flags", G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                                   G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                        "g-name", name_owner,
                                        "g-object-path", object_path,
                                        "g-interface-name", interface_name,
                                        NULL);
      if (interface_proxy == NULL)
        {
          g_warning ("%s: Error constructing proxy for path %s and interface %s: %s",
                     G_STRLOC,
                     object_path,
                     interface_name,
                     error->message);
          xerror_free (error);
        }
      else
        {
          xvariant_iter_t property_iter;
          const xchar_t *property_name;
          xvariant_t *property_value;

          /* associate the interface proxy with the object */
          g_dbus_interface_set_object (G_DBUS_INTERFACE (interface_proxy),
                                       G_DBUS_OBJECT (op));

          xvariant_iter_init (&property_iter, properties);
          while (xvariant_iter_next (&property_iter,
                                      "{&sv}",
                                      &property_name,
                                      &property_value))
            {
              xdbus_proxy_set_cached_property (interface_proxy,
                                                property_name,
                                                property_value);
              xvariant_unref (property_value);
            }

          _xdbus_object_proxy_add_interface (op, interface_proxy);
          if (!added)
            interface_added_signals = xlist_append (interface_added_signals, xobject_ref (interface_proxy));
          xobject_unref (interface_proxy);
        }
      xvariant_unref (properties);
    }

  if (added)
    {
      xhash_table_insert (manager->priv->map_object_path_to_object_proxy,
                           xstrdup (object_path),
                           op);
    }

  g_mutex_unlock (&manager->priv->lock);

  /* now that we don't hold the lock any more, emit signals */
  xobject_ref (manager);
  for (l = interface_added_signals; l != NULL; l = l->next)
    {
      interface_proxy = G_DBUS_PROXY (l->data);
      xsignal_emit_by_name (manager, "interface-added", op, interface_proxy);
      xobject_unref (interface_proxy);
    }
  xlist_free (interface_added_signals);

  if (added)
    xsignal_emit_by_name (manager, "object-added", op);

  xobject_unref (manager);
  xobject_unref (op);
}

static void
remove_interfaces (xdbus_object_manager_client_t   *manager,
                   const xchar_t         *object_path,
                   const xchar_t *const  *interface_names)
{
  xdbus_object_proxy_t *op;
  xlist_t *interfaces;
  xuint_t n;
  xuint_t num_interfaces;
  xuint_t num_interfaces_to_remove;

  g_mutex_lock (&manager->priv->lock);

  op = xhash_table_lookup (manager->priv->map_object_path_to_object_proxy, object_path);
  if (op == NULL)
    {
      g_debug ("%s: Processing InterfaceRemoved signal for path %s but no object proxy exists",
               G_STRLOC,
               object_path);
      g_mutex_unlock (&manager->priv->lock);
      return;
    }

  interfaces = g_dbus_object_get_interfaces (G_DBUS_OBJECT (op));
  num_interfaces = xlist_length (interfaces);
  xlist_free_full (interfaces, xobject_unref);

  num_interfaces_to_remove = xstrv_length ((xchar_t **) interface_names);

  /* see if we are going to completety remove the object */
  xobject_ref (manager);
  if (num_interfaces_to_remove == num_interfaces)
    {
      xobject_ref (op);
      g_warn_if_fail (xhash_table_remove (manager->priv->map_object_path_to_object_proxy, object_path));
      g_mutex_unlock (&manager->priv->lock);
      xsignal_emit_by_name (manager, "object-removed", op);
      xobject_unref (op);
    }
  else
    {
      xobject_ref (op);
      g_mutex_unlock (&manager->priv->lock);
      for (n = 0; interface_names != NULL && interface_names[n] != NULL; n++)
        {
          xdbus_interface_t *interface;
          interface = g_dbus_object_get_interface (G_DBUS_OBJECT (op), interface_names[n]);
          _xdbus_object_proxy_remove_interface (op, interface_names[n]);
          if (interface != NULL)
            {
              xsignal_emit_by_name (manager, "interface-removed", op, interface);
              xobject_unref (interface);
            }
        }
      xobject_unref (op);
    }
  xobject_unref (manager);
}

static void
process_get_all_result (xdbus_object_manager_client_t *manager,
                        xvariant_t          *value,
                        const xchar_t       *name_owner)
{
  xvariant_t *arg0;
  const xchar_t *object_path;
  xvariant_t *ifaces_and_properties;
  xvariant_iter_t iter;

  g_return_if_fail (name_owner == NULL || g_dbus_is_unique_name (name_owner));

  arg0 = xvariant_get_child_value (value, 0);
  xvariant_iter_init (&iter, arg0);
  while (xvariant_iter_next (&iter,
                              "{&o@a{sa{sv}}}",
                              &object_path,
                              &ifaces_and_properties))
    {
      add_interfaces (manager, object_path, ifaces_and_properties, name_owner);
      xvariant_unref (ifaces_and_properties);
    }
  xvariant_unref (arg0);
}

static void
on_control_proxy_g_signal (xdbus_proxy_t   *proxy,
                           const xchar_t  *sender_name,
                           const xchar_t  *signal_name,
                           xvariant_t     *parameters,
                           xpointer_t      user_data)
{
  GWeakRef *manager_weak = user_data;
  xdbus_object_manager_client_t *manager = NULL;
  const xchar_t *object_path;

  manager = G_DBUS_OBJECT_MANAGER_CLIENT (g_weak_ref_get (manager_weak));
  if (manager == NULL)
    return;

  //g_debug ("yay, g_signal %s: %s\n", signal_name, xvariant_print (parameters, TRUE));

  if (xstrcmp0 (signal_name, "InterfacesAdded") == 0)
    {
      xvariant_t *ifaces_and_properties;
      xvariant_get (parameters,
                     "(&o@a{sa{sv}})",
                     &object_path,
                     &ifaces_and_properties);
      add_interfaces (manager, object_path, ifaces_and_properties, manager->priv->name_owner);
      xvariant_unref (ifaces_and_properties);
    }
  else if (xstrcmp0 (signal_name, "InterfacesRemoved") == 0)
    {
      const xchar_t **ifaces;
      xvariant_get (parameters,
                     "(&o^a&s)",
                     &object_path,
                     &ifaces);
      remove_interfaces (manager, object_path, ifaces);
      g_free (ifaces);
    }

  xobject_unref (manager);
}

/* ---------------------------------------------------------------------------------------------------- */

static const xchar_t *
xdbus_object_manager_client_get_object_path (xdbus_object_manager_t *_manager)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (_manager);
  return manager->priv->object_path;
}

static xdbus_object_t *
xdbus_object_manager_client_get_object (xdbus_object_manager_t *_manager,
                                         const xchar_t        *object_path)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (_manager);
  xdbus_object_t *ret;

  g_mutex_lock (&manager->priv->lock);
  ret = xhash_table_lookup (manager->priv->map_object_path_to_object_proxy, object_path);
  if (ret != NULL)
    xobject_ref (ret);
  g_mutex_unlock (&manager->priv->lock);
  return ret;
}

static xdbus_interface_t *
xdbus_object_manager_client_get_interface  (xdbus_object_manager_t  *_manager,
                                             const xchar_t         *object_path,
                                             const xchar_t         *interface_name)
{
  xdbus_interface_t *ret;
  xdbus_object_t *object;

  ret = NULL;

  object = g_dbus_object_manager_get_object (_manager, object_path);
  if (object == NULL)
    goto out;

  ret = g_dbus_object_get_interface (object, interface_name);
  xobject_unref (object);

 out:
  return ret;
}

static xlist_t *
xdbus_object_manager_client_get_objects (xdbus_object_manager_t *_manager)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (_manager);
  xlist_t *ret;

  xreturn_val_if_fail (X_IS_DBUS_OBJECT_MANAGER_CLIENT (manager), NULL);

  g_mutex_lock (&manager->priv->lock);
  ret = xhash_table_get_values (manager->priv->map_object_path_to_object_proxy);
  xlist_foreach (ret, (GFunc) xobject_ref, NULL);
  g_mutex_unlock (&manager->priv->lock);

  return ret;
}


static void
dbus_object_manager_interface_init (xdbus_object_manager_iface_t *iface)
{
  iface->get_object_path = xdbus_object_manager_client_get_object_path;
  iface->get_objects     = xdbus_object_manager_client_get_objects;
  iface->get_object      = xdbus_object_manager_client_get_object;
  iface->get_interface   = xdbus_object_manager_client_get_interface;
}

/* ---------------------------------------------------------------------------------------------------- */
