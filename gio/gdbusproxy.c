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

#include <stdlib.h>
#include <string.h>

#include "gdbusutils.h"
#include "gdbusproxy.h"
#include "gioenumtypes.h"
#include "gdbusconnection.h"
#include "gdbuserror.h"
#include "gdbusprivate.h"
#include "ginitable.h"
#include "gasyncinitable.h"
#include "gioerror.h"
#include "gtask.h"
#include "gcancellable.h"
#include "gdbusinterface.h"
#include "gasyncresult.h"

#ifdef G_OS_UNIX
#include "gunixfdlist.h"
#endif

#include "glibintl.h"
#include "gmarshal-internal.h"

/**
 * SECTION:gdbusproxy
 * @short_description: Client-side D-Bus interface proxy
 * @include: gio/gio.h
 *
 * #xdbus_proxy_t is a base class used for proxies to access a D-Bus
 * interface on a remote object. A #xdbus_proxy_t can be constructed for
 * both well-known and unique names.
 *
 * By default, #xdbus_proxy_t will cache all properties (and listen to
 * changes) of the remote object, and proxy all signals that get
 * emitted. This behaviour can be changed by passing suitable
 * #xdbus_proxy_flags_t when the proxy is created. If the proxy is for a
 * well-known name, the property cache is flushed when the name owner
 * vanishes and reloaded when a name owner appears.
 *
 * The unique name owner of the proxy's name is tracked and can be read from
 * #xdbus_proxy_t:g-name-owner. Connect to the #xobject_t::notify signal to
 * get notified of changes. Additionally, only signals and property
 * changes emitted from the current name owner are considered and
 * calls are always sent to the current name owner. This avoids a
 * number of race conditions when the name is lost by one owner and
 * claimed by another. However, if no name owner currently exists,
 * then calls will be sent to the well-known name which may result in
 * the message bus launching an owner (unless
 * %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START is set).
 *
 * If the proxy is for a stateless D-Bus service, where the name owner may
 * be started and stopped between calls, the #xdbus_proxy_t:g-name-owner tracking
 * of #xdbus_proxy_t will cause the proxy to drop signal and property changes from
 * the service after it has restarted for the first time. When interacting
 * with a stateless D-Bus service, do not use #xdbus_proxy_t — use direct D-Bus
 * method calls and signal connections.
 *
 * The generic #xdbus_proxy_t::g-properties-changed and
 * #xdbus_proxy_t::g-signal signals are not very convenient to work with.
 * Therefore, the recommended way of working with proxies is to subclass
 * #xdbus_proxy_t, and have more natural properties and signals in your derived
 * class. This [example][gdbus-example-gdbus-codegen] shows how this can
 * easily be done using the [gdbus-codegen][gdbus-codegen] tool.
 *
 * A #xdbus_proxy_t instance can be used from multiple threads but note
 * that all signals (e.g. #xdbus_proxy_t::g-signal, #xdbus_proxy_t::g-properties-changed
 * and #xobject_t::notify) are emitted in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread where the instance was constructed.
 *
 * An example using a proxy for a well-known name can be found in
 * [gdbus-example-watch-proxy.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-watch-proxy.c)
 */

/* lock protecting the mutable properties: name_owner, timeout_msec,
 * expected_interface, and the properties hash table
 */
G_LOCK_DEFINE_STATIC (properties_lock);

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

/* ---------------------------------------------------------------------------------------------------- */

struct _xdbus_proxy_private
{
  xbus_type_t bus_type;
  xdbus_proxy_flags_t flags;
  xdbus_connection_t *connection;

  xchar_t *name;
  /* mutable, protected by properties_lock */
  xchar_t *name_owner;
  xchar_t *object_path;
  xchar_t *interface_name;
  /* mutable, protected by properties_lock */
  xint_t timeout_msec;

  xuint_t name_owner_changed_subscription_id;

  xcancellable_t *get_all_cancellable;

  /* xchar_t* -> xvariant_t*, protected by properties_lock */
  xhashtable_t *properties;

  /* mutable, protected by properties_lock */
  xdbus_interface_info_t *expected_interface;

  xuint_t properties_changed_subscription_id;
  xuint_t signals_subscription_id;

  xboolean_t initialized;

  /* mutable, protected by properties_lock */
  xdbus_object_t *object;
};

enum
{
  PROP_0,
  PROP_G_CONNECTION,
  PROP_G_BUS_TYPE,
  PROP_G_NAME,
  PROP_G_NAME_OWNER,
  PROP_G_FLAGS,
  PROP_G_OBJECT_PATH,
  PROP_G_INTERFACE_NAME,
  PROP_G_DEFAULT_TIMEOUT,
  PROP_G_INTERFACE_INFO
};

enum
{
  PROPERTIES_CHANGED_SIGNAL,
  SIGNAL_SIGNAL,
  LAST_SIGNAL,
};

static xuint_t signals[LAST_SIGNAL] = {0};

static void dbus_interface_iface_init (xdbus_interface_iface_t *dbus_interface_iface);
static void initable_iface_init       (xinitable_iface_t *initable_iface);
static void async_initable_iface_init (xasync_initable_iface_t *async_initable_iface);

G_DEFINE_TYPE_WITH_CODE (xdbus_proxy, xdbus_proxy, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xdbus_proxy)
                         G_IMPLEMENT_INTERFACE (XTYPE_DBUS_INTERFACE, dbus_interface_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_ASYNC_INITABLE, async_initable_iface_init))

static void
xdbus_proxy_finalize (xobject_t *object)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (object);

  g_warn_if_fail (proxy->priv->get_all_cancellable == NULL);

  if (proxy->priv->name_owner_changed_subscription_id > 0)
    xdbus_connection_signal_unsubscribe (proxy->priv->connection,
                                          proxy->priv->name_owner_changed_subscription_id);

  if (proxy->priv->properties_changed_subscription_id > 0)
    xdbus_connection_signal_unsubscribe (proxy->priv->connection,
                                          proxy->priv->properties_changed_subscription_id);

  if (proxy->priv->signals_subscription_id > 0)
    xdbus_connection_signal_unsubscribe (proxy->priv->connection,
                                          proxy->priv->signals_subscription_id);

  if (proxy->priv->connection != NULL)
    xobject_unref (proxy->priv->connection);
  g_free (proxy->priv->name);
  g_free (proxy->priv->name_owner);
  g_free (proxy->priv->object_path);
  g_free (proxy->priv->interface_name);
  if (proxy->priv->properties != NULL)
    xhash_table_unref (proxy->priv->properties);

  if (proxy->priv->expected_interface != NULL)
    {
      g_dbus_interface_info_cache_release (proxy->priv->expected_interface);
      g_dbus_interface_info_unref (proxy->priv->expected_interface);
    }

  if (proxy->priv->object != NULL)
    xobject_remove_weak_pointer (G_OBJECT (proxy->priv->object), (xpointer_t *) &proxy->priv->object);

  G_OBJECT_CLASS (xdbus_proxy_parent_class)->finalize (object);
}

static void
xdbus_proxy_get_property (xobject_t    *object,
                           xuint_t       prop_id,
                           xvalue_t     *value,
                           xparam_spec_t *pspec)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (object);

  switch (prop_id)
    {
    case PROP_G_CONNECTION:
      xvalue_set_object (value, proxy->priv->connection);
      break;

    case PROP_G_FLAGS:
      xvalue_set_flags (value, proxy->priv->flags);
      break;

    case PROP_G_NAME:
      xvalue_set_string (value, proxy->priv->name);
      break;

    case PROP_G_NAME_OWNER:
      xvalue_take_string (value, xdbus_proxy_get_name_owner (proxy));
      break;

    case PROP_G_OBJECT_PATH:
      xvalue_set_string (value, proxy->priv->object_path);
      break;

    case PROP_G_INTERFACE_NAME:
      xvalue_set_string (value, proxy->priv->interface_name);
      break;

    case PROP_G_DEFAULT_TIMEOUT:
      xvalue_set_int (value, xdbus_proxy_get_default_timeout (proxy));
      break;

    case PROP_G_INTERFACE_INFO:
      xvalue_set_boxed (value, xdbus_proxy_get_interface_info (proxy));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdbus_proxy_set_property (xobject_t      *object,
                           xuint_t         prop_id,
                           const xvalue_t *value,
                           xparam_spec_t   *pspec)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (object);

  switch (prop_id)
    {
    case PROP_G_CONNECTION:
      proxy->priv->connection = xvalue_dup_object (value);
      break;

    case PROP_G_FLAGS:
      proxy->priv->flags = xvalue_get_flags (value);
      break;

    case PROP_G_NAME:
      proxy->priv->name = xvalue_dup_string (value);
      break;

    case PROP_G_OBJECT_PATH:
      proxy->priv->object_path = xvalue_dup_string (value);
      break;

    case PROP_G_INTERFACE_NAME:
      proxy->priv->interface_name = xvalue_dup_string (value);
      break;

    case PROP_G_DEFAULT_TIMEOUT:
      xdbus_proxy_set_default_timeout (proxy, xvalue_get_int (value));
      break;

    case PROP_G_INTERFACE_INFO:
      xdbus_proxy_set_interface_info (proxy, xvalue_get_boxed (value));
      break;

    case PROP_G_BUS_TYPE:
      proxy->priv->bus_type = xvalue_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdbus_proxy_class_init (GDBusProxyClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = xdbus_proxy_finalize;
  gobject_class->set_property = xdbus_proxy_set_property;
  gobject_class->get_property = xdbus_proxy_get_property;

  /* Note that all property names are prefixed to avoid collisions with D-Bus property names
   * in derived classes */

  /**
   * xdbus_proxy_t:g-interface-info:
   *
   * Ensure that interactions with this proxy conform to the given
   * interface. This is mainly to ensure that malformed data received
   * from the other peer is ignored. The given #xdbus_interface_info_t is
   * said to be the "expected interface".
   *
   * The checks performed are:
   * - When completing a method call, if the type signature of
   *   the reply message isn't what's expected, the reply is
   *   discarded and the #xerror_t is set to %G_IO_ERROR_INVALID_ARGUMENT.
   *
   * - Received signals that have a type signature mismatch are dropped and
   *   a warning is logged via g_warning().
   *
   * - Properties received via the initial `GetAll()` call or via the
   *   `::PropertiesChanged` signal (on the
   *   [org.freedesktop.DBus.Properties](http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties)
   *   interface) or set using xdbus_proxy_set_cached_property()
   *   with a type signature mismatch are ignored and a warning is
   *   logged via g_warning().
   *
   * Note that these checks are never done on methods, signals and
   * properties that are not referenced in the given
   * #xdbus_interface_info_t, since extending a D-Bus interface on the
   * service-side is not considered an ABI break.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_INTERFACE_INFO,
                                   g_param_spec_boxed ("g-interface-info",
                                                       P_("Interface Information"),
                                                       P_("Interface Information"),
                                                       XTYPE_DBUS_INTERFACE_INFO,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-connection:
   *
   * The #xdbus_connection_t the proxy is for.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_CONNECTION,
                                   g_param_spec_object ("g-connection",
                                                        P_("g-connection"),
                                                        P_("The connection the proxy is for"),
                                                        XTYPE_DBUS_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-bus-type:
   *
   * If this property is not %G_BUS_TYPE_NONE, then
   * #xdbus_proxy_t:g-connection must be %NULL and will be set to the
   * #xdbus_connection_t obtained by calling g_bus_get() with the value
   * of this property.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_BUS_TYPE,
                                   g_param_spec_enum ("g-bus-type",
                                                      P_("Bus Type"),
                                                      P_("The bus to connect to, if any"),
                                                      XTYPE_BUS_TYPE,
                                                      G_BUS_TYPE_NONE,
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB |
                                                      G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-flags:
   *
   * Flags from the #xdbus_proxy_flags_t enumeration.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_FLAGS,
                                   g_param_spec_flags ("g-flags",
                                                       P_("g-flags"),
                                                       P_("Flags for the proxy"),
                                                       XTYPE_DBUS_PROXY_FLAGS,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-name:
   *
   * The well-known or unique name that the proxy is for.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_NAME,
                                   g_param_spec_string ("g-name",
                                                        P_("g-name"),
                                                        P_("The well-known or unique name that the proxy is for"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-name-owner:
   *
   * The unique name that owns #xdbus_proxy_t:g-name or %NULL if no-one
   * currently owns that name. You may connect to #xobject_t::notify signal to
   * track changes to this property.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_NAME_OWNER,
                                   g_param_spec_string ("g-name-owner",
                                                        P_("g-name-owner"),
                                                        P_("The unique name for the owner"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-object-path:
   *
   * The object path the proxy is for.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_OBJECT_PATH,
                                   g_param_spec_string ("g-object-path",
                                                        P_("g-object-path"),
                                                        P_("The object path the proxy is for"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-interface-name:
   *
   * The D-Bus interface name the proxy is for.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_INTERFACE_NAME,
                                   g_param_spec_string ("g-interface-name",
                                                        P_("g-interface-name"),
                                                        P_("The D-Bus interface name the proxy is for"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t:g-default-timeout:
   *
   * The timeout to use if -1 (specifying default timeout) is passed
   * as @timeout_msec in the xdbus_proxy_call() and
   * xdbus_proxy_call_sync() functions.
   *
   * This allows applications to set a proxy-wide timeout for all
   * remote method invocations on the proxy. If this property is -1,
   * the default timeout (typically 25 seconds) is used. If set to
   * %G_MAXINT, then no timeout is used.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_G_DEFAULT_TIMEOUT,
                                   g_param_spec_int ("g-default-timeout",
                                                     P_("Default Timeout"),
                                                     P_("Timeout for remote method invocation"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_BLURB |
                                                     G_PARAM_STATIC_NICK));

  /**
   * xdbus_proxy_t::g-properties-changed:
   * @proxy: The #xdbus_proxy_t emitting the signal.
   * @changed_properties: A #xvariant_t containing the properties that changed (type: `a{sv}`)
   * @invalidated_properties: A %NULL terminated array of properties that was invalidated
   *
   * Emitted when one or more D-Bus properties on @proxy changes. The
   * local cache has already been updated when this signal fires. Note
   * that both @changed_properties and @invalidated_properties are
   * guaranteed to never be %NULL (either may be empty though).
   *
   * If the proxy has the flag
   * %G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES set, then
   * @invalidated_properties will always be empty.
   *
   * This signal corresponds to the
   * `PropertiesChanged` D-Bus signal on the
   * `org.freedesktop.DBus.Properties` interface.
   *
   * Since: 2.26
   */
  signals[PROPERTIES_CHANGED_SIGNAL] = xsignal_new (I_("g-properties-changed"),
                                                     XTYPE_DBUS_PROXY,
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                                                     G_STRUCT_OFFSET (GDBusProxyClass, g_properties_changed),
                                                     NULL,
                                                     NULL,
                                                     _g_cclosure_marshal_VOID__VARIANT_BOXED,
                                                     XTYPE_NONE,
                                                     2,
                                                     XTYPE_VARIANT,
                                                     XTYPE_STRV | G_SIGNAL_TYPE_STATIC_SCOPE);
  xsignal_set_va_marshaller (signals[PROPERTIES_CHANGED_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__VARIANT_BOXEDv);

  /**
   * xdbus_proxy_t::g-signal:
   * @proxy: The #xdbus_proxy_t emitting the signal.
   * @sender_name: (nullable): The sender of the signal or %NULL if the connection is not a bus connection.
   * @signal_name: The name of the signal.
   * @parameters: A #xvariant_t tuple with parameters for the signal.
   *
   * Emitted when a signal from the remote object and interface that @proxy is for, has been received.
   *
   * Since 2.72 this signal supports detailed connections. You can connect to
   * the detailed signal `g-signal::x` in order to receive callbacks only when
   * signal `x` is received from the remote object.
   *
   * Since: 2.26
   */
  signals[SIGNAL_SIGNAL] = xsignal_new (I_("g-signal"),
                                         XTYPE_DBUS_PROXY,
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED | G_SIGNAL_MUST_COLLECT,
                                         G_STRUCT_OFFSET (GDBusProxyClass, g_signal),
                                         NULL,
                                         NULL,
                                         _g_cclosure_marshal_VOID__STRING_STRING_VARIANT,
                                         XTYPE_NONE,
                                         3,
                                         XTYPE_STRING,
                                         XTYPE_STRING,
                                         XTYPE_VARIANT);
  xsignal_set_va_marshaller (signals[SIGNAL_SIGNAL],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__STRING_STRING_VARIANTv);

}

static void
xdbus_proxy_init (xdbus_proxy_t *proxy)
{
  proxy->priv = xdbus_proxy_get_instance_private (proxy);
  proxy->priv->properties = xhash_table_new_full (xstr_hash,
                                                   xstr_equal,
                                                   g_free,
                                                   (xdestroy_notify_t) xvariant_unref);
}

/* ---------------------------------------------------------------------------------------------------- */

static xint_t
property_name_sort_func (const xchar_t **a,
                         const xchar_t **b)
{
  return xstrcmp0 (*a, *b);
}

/**
 * xdbus_proxy_get_cached_property_names:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the names of all cached properties on @proxy.
 *
 * Returns: (transfer full) (nullable) (array zero-terminated=1): A
 *          %NULL-terminated array of strings or %NULL if
 *          @proxy has no cached properties. Free the returned array with
 *          xstrfreev().
 *
 * Since: 2.26
 */
xchar_t **
xdbus_proxy_get_cached_property_names (xdbus_proxy_t  *proxy)
{
  xchar_t **names;
  xptr_array_t *p;
  xhash_table_iter_t iter;
  const xchar_t *key;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);

  G_LOCK (properties_lock);

  names = NULL;
  if (xhash_table_size (proxy->priv->properties) == 0)
    goto out;

  p = xptr_array_new ();

  xhash_table_iter_init (&iter, proxy->priv->properties);
  while (xhash_table_iter_next (&iter, (xpointer_t) &key, NULL))
    xptr_array_add (p, xstrdup (key));
  xptr_array_sort (p, (GCompareFunc) property_name_sort_func);
  xptr_array_add (p, NULL);

  names = (xchar_t **) xptr_array_free (p, FALSE);

 out:
  G_UNLOCK (properties_lock);
  return names;
}

/* properties_lock must be held for as long as you will keep the
 * returned value
 */
static const xdbus_property_info_t *
lookup_property_info (xdbus_proxy_t  *proxy,
                      const xchar_t *property_name)
{
  const xdbus_property_info_t *info = NULL;

  if (proxy->priv->expected_interface == NULL)
    goto out;

  info = g_dbus_interface_info_lookup_property (proxy->priv->expected_interface, property_name);

 out:
  return info;
}

/**
 * xdbus_proxy_get_cached_property:
 * @proxy: A #xdbus_proxy_t.
 * @property_name: Property name.
 *
 * Looks up the value for a property from the cache. This call does no
 * blocking IO.
 *
 * If @proxy has an expected interface (see
 * #xdbus_proxy_t:g-interface-info) and @property_name is referenced by
 * it, then @value is checked against the type of the property.
 *
 * Returns: (transfer full) (nullable): A reference to the #xvariant_t instance
 *    that holds the value for @property_name or %NULL if the value is not in
 *    the cache. The returned reference must be freed with xvariant_unref().
 *
 * Since: 2.26
 */
xvariant_t *
xdbus_proxy_get_cached_property (xdbus_proxy_t   *proxy,
                                  const xchar_t  *property_name)
{
  const xdbus_property_info_t *info;
  xvariant_t *value;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  G_LOCK (properties_lock);

  value = xhash_table_lookup (proxy->priv->properties, property_name);
  if (value == NULL)
    goto out;

  info = lookup_property_info (proxy, property_name);
  if (info != NULL)
    {
      const xchar_t *type_string = xvariant_get_type_string (value);
      if (xstrcmp0 (type_string, info->signature) != 0)
        {
          g_warning ("Trying to get property %s with type %s but according to the expected "
                     "interface the type is %s",
                     property_name,
                     type_string,
                     info->signature);
          value = NULL;
          goto out;
        }
    }

  xvariant_ref (value);

 out:
  G_UNLOCK (properties_lock);
  return value;
}

/**
 * xdbus_proxy_set_cached_property:
 * @proxy: A #xdbus_proxy_t
 * @property_name: Property name.
 * @value: (nullable): Value for the property or %NULL to remove it from the cache.
 *
 * If @value is not %NULL, sets the cached value for the property with
 * name @property_name to the value in @value.
 *
 * If @value is %NULL, then the cached value is removed from the
 * property cache.
 *
 * If @proxy has an expected interface (see
 * #xdbus_proxy_t:g-interface-info) and @property_name is referenced by
 * it, then @value is checked against the type of the property.
 *
 * If the @value #xvariant_t is floating, it is consumed. This allows
 * convenient 'inline' use of xvariant_new(), e.g.
 * |[<!-- language="C" -->
 *  xdbus_proxy_set_cached_property (proxy,
 *                                    "SomeProperty",
 *                                    xvariant_new ("(si)",
 *                                                  "A String",
 *                                                  42));
 * ]|
 *
 * Normally you will not need to use this method since @proxy
 * is tracking changes using the
 * `org.freedesktop.DBus.Properties.PropertiesChanged`
 * D-Bus signal. However, for performance reasons an object may
 * decide to not use this signal for some properties and instead
 * use a proprietary out-of-band mechanism to transmit changes.
 *
 * As a concrete example, consider an object with a property
 * `ChatroomParticipants` which is an array of strings. Instead of
 * transmitting the same (long) array every time the property changes,
 * it is more efficient to only transmit the delta using e.g. signals
 * `ChatroomParticipantJoined(String name)` and
 * `ChatroomParticipantParted(String name)`.
 *
 * Since: 2.26
 */
void
xdbus_proxy_set_cached_property (xdbus_proxy_t   *proxy,
                                  const xchar_t  *property_name,
                                  xvariant_t     *value)
{
  const xdbus_property_info_t *info;

  g_return_if_fail (X_IS_DBUS_PROXY (proxy));
  g_return_if_fail (property_name != NULL);

  G_LOCK (properties_lock);

  if (value != NULL)
    {
      info = lookup_property_info (proxy, property_name);
      if (info != NULL)
        {
          if (xstrcmp0 (info->signature, xvariant_get_type_string (value)) != 0)
            {
              g_warning ("Trying to set property %s of type %s but according to the expected "
			 "interface the type is %s",
                         property_name,
                         xvariant_get_type_string (value),
                         info->signature);
              goto out;
            }
        }
      xhash_table_insert (proxy->priv->properties,
                           xstrdup (property_name),
                           xvariant_ref_sink (value));
    }
  else
    {
      xhash_table_remove (proxy->priv->properties, property_name);
    }

 out:
  G_UNLOCK (properties_lock);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_signal_received (xdbus_connection_t *connection,
                    const xchar_t     *sender_name,
                    const xchar_t     *object_path,
                    const xchar_t     *interface_name,
                    const xchar_t     *signal_name,
                    xvariant_t        *parameters,
                    xpointer_t         user_data)
{
  GWeakRef *proxy_weak = user_data;
  xdbus_proxy_t *proxy;

  proxy = G_DBUS_PROXY (g_weak_ref_get (proxy_weak));
  if (proxy == NULL)
    return;

  if (!proxy->priv->initialized)
    goto out;

  G_LOCK (properties_lock);

  if (proxy->priv->name_owner != NULL && xstrcmp0 (sender_name, proxy->priv->name_owner) != 0)
    {
      G_UNLOCK (properties_lock);
      goto out;
    }

  if (proxy->priv->expected_interface != NULL)
    {
      const xdbus_signalInfo_t *info;
      info = g_dbus_interface_info_lookup_signal (proxy->priv->expected_interface, signal_name);
      if (info != NULL)
        {
          xvariant_type_t *expected_type;
          expected_type = _g_dbus_compute_complete_signature (info->args);
          if (!xvariant_type_equal (expected_type, xvariant_get_type (parameters)))
            {
              xchar_t *expected_type_string = xvariant_type_dup_string (expected_type);
              g_warning ("Dropping signal %s of type %s since the type from the expected interface is %s",
                         info->name,
                         xvariant_get_type_string (parameters),
                         expected_type_string);
              g_free (expected_type_string);
              xvariant_type_free (expected_type);
              G_UNLOCK (properties_lock);
              goto out;
            }
          xvariant_type_free (expected_type);
        }
    }

  G_UNLOCK (properties_lock);

  xsignal_emit (proxy,
                 signals[SIGNAL_SIGNAL],
                 g_quark_try_string (signal_name),
                 sender_name,
                 signal_name,
                 parameters);

 out:
  g_clear_object (&proxy);
}

/* ---------------------------------------------------------------------------------------------------- */

/* must hold properties_lock */
static void
insert_property_checked (xdbus_proxy_t  *proxy,
			 xchar_t *property_name,
			 xvariant_t *value)
{
  if (proxy->priv->expected_interface != NULL)
    {
      const xdbus_property_info_t *info;
      info = g_dbus_interface_info_lookup_property (proxy->priv->expected_interface, property_name);
      /* Only check known properties */
      if (info != NULL)
        {
          /* Warn about properties with the wrong type */
          if (xstrcmp0 (info->signature, xvariant_get_type_string (value)) != 0)
            {
              g_warning ("Received property %s with type %s does not match expected type "
                         "%s in the expected interface",
                         property_name,
                         xvariant_get_type_string (value),
                         info->signature);
              goto invalid;
            }
        }
    }

  xhash_table_insert (proxy->priv->properties,
		       property_name, /* adopts string */
		       value); /* adopts value */

  return;

 invalid:
  xvariant_unref (value);
  g_free (property_name);
}

typedef struct
{
  xdbus_proxy_t *proxy;
  xchar_t *prop_name;
} InvalidatedPropGetData;

static void
invalidated_property_get_cb (xdbus_connection_t *connection,
                             xasync_result_t    *res,
                             xpointer_t         user_data)
{
  InvalidatedPropGetData *data = user_data;
  const xchar_t *invalidated_properties[] = {NULL};
  xvariant_builder_t builder;
  xvariant_t *value = NULL;
  xvariant_t *unpacked_value = NULL;

  /* errors are fine, the other end could have disconnected */
  value = xdbus_connection_call_finish (connection, res, NULL);
  if (value == NULL)
    {
      goto out;
    }

  if (!xvariant_is_of_type (value, G_VARIANT_TYPE ("(v)")))
    {
      g_warning ("Expected type '(v)' for Get() reply, got '%s'", xvariant_get_type_string (value));
      goto out;
    }

  xvariant_get (value, "(v)", &unpacked_value);

  /* synthesize the a{sv} in the PropertiesChanged signal */
  xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
  xvariant_builder_add (&builder, "{sv}", data->prop_name, unpacked_value);

  G_LOCK (properties_lock);
  insert_property_checked (data->proxy,
                           data->prop_name,  /* adopts string */
                           unpacked_value);  /* adopts value */
  data->prop_name = NULL;
  G_UNLOCK (properties_lock);

  xsignal_emit (data->proxy,
                 signals[PROPERTIES_CHANGED_SIGNAL], 0,
                 xvariant_builder_end (&builder), /* consumed */
                 invalidated_properties);


 out:
  if (value != NULL)
    xvariant_unref (value);
  xobject_unref (data->proxy);
  g_free (data->prop_name);
  g_slice_free (InvalidatedPropGetData, data);
}

static void
on_properties_changed (xdbus_connection_t *connection,
                       const xchar_t     *sender_name,
                       const xchar_t     *object_path,
                       const xchar_t     *interface_name,
                       const xchar_t     *signal_name,
                       xvariant_t        *parameters,
                       xpointer_t         user_data)
{
  GWeakRef *proxy_weak = user_data;
  xboolean_t emit_g_signal = FALSE;
  xdbus_proxy_t *proxy;
  const xchar_t *interface_name_for_signal;
  xvariant_t *changed_properties;
  xchar_t **invalidated_properties;
  xvariant_iter_t iter;
  xchar_t *key;
  xvariant_t *value;
  xuint_t n;

  changed_properties = NULL;
  invalidated_properties = NULL;

  proxy = G_DBUS_PROXY (g_weak_ref_get (proxy_weak));
  if (proxy == NULL)
    return;

  if (!proxy->priv->initialized)
    goto out;

  G_LOCK (properties_lock);

  if (proxy->priv->name_owner != NULL && xstrcmp0 (sender_name, proxy->priv->name_owner) != 0)
    {
      G_UNLOCK (properties_lock);
      goto out;
    }

  if (!xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(sa{sv}as)")))
    {
      g_warning ("Value for PropertiesChanged signal with type '%s' does not match '(sa{sv}as)'",
                 xvariant_get_type_string (parameters));
      G_UNLOCK (properties_lock);
      goto out;
    }

  xvariant_get (parameters,
                 "(&s@a{sv}^a&s)",
                 &interface_name_for_signal,
                 &changed_properties,
                 &invalidated_properties);

  if (xstrcmp0 (interface_name_for_signal, proxy->priv->interface_name) != 0)
    {
      G_UNLOCK (properties_lock);
      goto out;
    }

  xvariant_iter_init (&iter, changed_properties);
  while (xvariant_iter_next (&iter, "{sv}", &key, &value))
    {
      insert_property_checked (proxy,
			       key, /* adopts string */
			       value); /* adopts value */
      emit_g_signal = TRUE;
    }

  if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES)
    {
      if (proxy->priv->name_owner != NULL)
        {
          for (n = 0; invalidated_properties[n] != NULL; n++)
            {
              InvalidatedPropGetData *data;
              data = g_slice_new0 (InvalidatedPropGetData);
              data->proxy = xobject_ref (proxy);
              data->prop_name = xstrdup (invalidated_properties[n]);
              xdbus_connection_call (proxy->priv->connection,
                                      proxy->priv->name_owner,
                                      proxy->priv->object_path,
                                      "org.freedesktop.DBus.Properties",
                                      "Get",
                                      xvariant_new ("(ss)", proxy->priv->interface_name, data->prop_name),
                                      G_VARIANT_TYPE ("(v)"),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,           /* timeout */
                                      NULL,         /* xcancellable_t */
                                      (xasync_ready_callback_t) invalidated_property_get_cb,
                                      data);
            }
        }
    }
  else
    {
      emit_g_signal = TRUE;
      for (n = 0; invalidated_properties[n] != NULL; n++)
        {
          xhash_table_remove (proxy->priv->properties, invalidated_properties[n]);
        }
    }

  G_UNLOCK (properties_lock);

  if (emit_g_signal)
    {
      xsignal_emit (proxy, signals[PROPERTIES_CHANGED_SIGNAL],
                     0,
                     changed_properties,
                     invalidated_properties);
    }

 out:
  g_clear_pointer (&changed_properties, xvariant_unref);
  g_free (invalidated_properties);
  g_clear_object (&proxy);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
process_get_all_reply (xdbus_proxy_t *proxy,
                       xvariant_t   *result)
{
  xvariant_iter_t *iter;
  xchar_t *key;
  xvariant_t *value;
  xuint_t num_properties;

  if (!xvariant_is_of_type (result, G_VARIANT_TYPE ("(a{sv})")))
    {
      g_warning ("Value for GetAll reply with type '%s' does not match '(a{sv})'",
                 xvariant_get_type_string (result));
      goto out;
    }

  G_LOCK (properties_lock);

  xvariant_get (result, "(a{sv})", &iter);
  while (xvariant_iter_next (iter, "{sv}", &key, &value))
    {
      insert_property_checked (proxy,
			       key, /* adopts string */
			       value); /* adopts value */
    }
  xvariant_iter_free (iter);

  num_properties = xhash_table_size (proxy->priv->properties);
  G_UNLOCK (properties_lock);

  /* Synthesize ::g-properties-changed changed */
  if (num_properties > 0)
    {
      xvariant_t *changed_properties;
      const xchar_t *invalidated_properties[1] = {NULL};

      xvariant_get (result,
                     "(@a{sv})",
                     &changed_properties);
      xsignal_emit (proxy, signals[PROPERTIES_CHANGED_SIGNAL],
                     0,
                     changed_properties,
                     invalidated_properties);
      xvariant_unref (changed_properties);
    }

 out:
  ;
}

typedef struct
{
  xdbus_proxy_t *proxy;
  xcancellable_t *cancellable;
  xchar_t *name_owner;
} load_properties_on_name_owner_changed_data_t;

static void
on_name_owner_changed_get_all_cb (xdbus_connection_t *connection,
                                  xasync_result_t    *res,
                                  xpointer_t         user_data)
{
  load_properties_on_name_owner_changed_data_t *data = user_data;
  xvariant_t *result;
  xerror_t *error;
  xboolean_t cancelled;

  cancelled = FALSE;

  error = NULL;
  result = xdbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)
        cancelled = TRUE;
      /* We just ignore if GetAll() is failing. Because this might happen
       * if the object has no properties at all. Or if the caller is
       * not authorized to see the properties.
       *
       * Either way, apps can know about this by using
       * get_cached_property_names() or get_cached_property().
       */
      if (G_UNLIKELY (_g_dbus_debug_proxy ()))
        {
          g_debug ("error: %d %d %s",
                   error->domain,
                   error->code,
                   error->message);
        }
      xerror_free (error);
    }

  /* and finally we can notify */
  if (!cancelled)
    {
      G_LOCK (properties_lock);
      g_free (data->proxy->priv->name_owner);
      data->proxy->priv->name_owner = g_steal_pointer (&data->name_owner);
      xhash_table_remove_all (data->proxy->priv->properties);
      G_UNLOCK (properties_lock);
      if (result != NULL)
        {
          process_get_all_reply (data->proxy, result);
          xvariant_unref (result);
        }

      xobject_notify (G_OBJECT (data->proxy), "g-name-owner");
    }

  if (data->cancellable == data->proxy->priv->get_all_cancellable)
    data->proxy->priv->get_all_cancellable = NULL;

  xobject_unref (data->proxy);
  xobject_unref (data->cancellable);
  g_free (data->name_owner);
  g_free (data);
}

static void
on_name_owner_changed (xdbus_connection_t *connection,
                       const xchar_t      *sender_name,
                       const xchar_t      *object_path,
                       const xchar_t      *interface_name,
                       const xchar_t      *signal_name,
                       xvariant_t         *parameters,
                       xpointer_t          user_data)
{
  GWeakRef *proxy_weak = user_data;
  xdbus_proxy_t *proxy;
  const xchar_t *old_owner;
  const xchar_t *new_owner;

  proxy = G_DBUS_PROXY (g_weak_ref_get (proxy_weak));
  if (proxy == NULL)
    return;

  /* if we are already trying to load properties, cancel that */
  if (proxy->priv->get_all_cancellable != NULL)
    {
      xcancellable_cancel (proxy->priv->get_all_cancellable);
      proxy->priv->get_all_cancellable = NULL;
    }

  xvariant_get (parameters,
                 "(&s&s&s)",
                 NULL,
                 &old_owner,
                 &new_owner);

  if (strlen (new_owner) == 0)
    {
      G_LOCK (properties_lock);
      g_free (proxy->priv->name_owner);
      proxy->priv->name_owner = NULL;

      /* Synthesize ::g-properties-changed changed */
      if (!(proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES) &&
          xhash_table_size (proxy->priv->properties) > 0)
        {
          xvariant_builder_t builder;
          xptr_array_t *invalidated_properties;
          xhash_table_iter_t iter;
          const xchar_t *key;

          /* Build changed_properties (always empty) and invalidated_properties ... */
          xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));

          invalidated_properties = xptr_array_new_with_free_func (g_free);
          xhash_table_iter_init (&iter, proxy->priv->properties);
          while (xhash_table_iter_next (&iter, (xpointer_t) &key, NULL))
            xptr_array_add (invalidated_properties, xstrdup (key));
          xptr_array_add (invalidated_properties, NULL);

          /* ... throw out the properties ... */
          xhash_table_remove_all (proxy->priv->properties);

          G_UNLOCK (properties_lock);

          /* ... and finally emit the ::g-properties-changed signal */
          xsignal_emit (proxy, signals[PROPERTIES_CHANGED_SIGNAL],
                         0,
                         xvariant_builder_end (&builder) /* consumed */,
                         (const xchar_t* const *) invalidated_properties->pdata);
          xptr_array_unref (invalidated_properties);
        }
      else
        {
          G_UNLOCK (properties_lock);
        }
      xobject_notify (G_OBJECT (proxy), "g-name-owner");
    }
  else
    {
      G_LOCK (properties_lock);

      /* ignore duplicates - this can happen when activating the service */
      if (xstrcmp0 (new_owner, proxy->priv->name_owner) == 0)
        {
          G_UNLOCK (properties_lock);
          goto out;
        }

      if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES)
        {
          g_free (proxy->priv->name_owner);
          proxy->priv->name_owner = xstrdup (new_owner);

          xhash_table_remove_all (proxy->priv->properties);
          G_UNLOCK (properties_lock);
          xobject_notify (G_OBJECT (proxy), "g-name-owner");
        }
      else
        {
          load_properties_on_name_owner_changed_data_t *data;

          G_UNLOCK (properties_lock);

          /* start loading properties.. only then emit notify::g-name-owner .. we
           * need to be able to cancel this in the event another NameOwnerChanged
           * signal suddenly happens
           */

          g_assert (proxy->priv->get_all_cancellable == NULL);
          proxy->priv->get_all_cancellable = xcancellable_new ();
          data = g_new0 (load_properties_on_name_owner_changed_data_t, 1);
          data->proxy = xobject_ref (proxy);
          data->cancellable = proxy->priv->get_all_cancellable;
          data->name_owner = xstrdup (new_owner);
          xdbus_connection_call (proxy->priv->connection,
                                  data->name_owner,
                                  proxy->priv->object_path,
                                  "org.freedesktop.DBus.Properties",
                                  "GetAll",
                                  xvariant_new ("(s)", proxy->priv->interface_name),
                                  G_VARIANT_TYPE ("(a{sv})"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,           /* timeout */
                                  proxy->priv->get_all_cancellable,
                                  (xasync_ready_callback_t) on_name_owner_changed_get_all_cb,
                                  data);
        }
    }

 out:
  g_clear_object (&proxy);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
async_init_get_all_cb (xdbus_connection_t *connection,
                       xasync_result_t    *res,
                       xpointer_t         user_data)
{
  xtask_t *task = user_data;
  xvariant_t *result;
  xerror_t *error;

  error = NULL;
  result = xdbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      /* We just ignore if GetAll() is failing. Because this might happen
       * if the object has no properties at all. Or if the caller is
       * not authorized to see the properties.
       *
       * Either way, apps can know about this by using
       * get_cached_property_names() or get_cached_property().
       */
      if (G_UNLIKELY (_g_dbus_debug_proxy ()))
        {
          g_debug ("error: %d %d %s",
                   error->domain,
                   error->code,
                   error->message);
        }
      xerror_free (error);
    }

  xtask_return_pointer (task, result,
                         (xdestroy_notify_t) xvariant_unref);
  xobject_unref (task);
}

static void
async_init_data_set_name_owner (xtask_t       *task,
                                const xchar_t *name_owner)
{
  xdbus_proxy_t *proxy = xtask_get_source_object (task);
  xboolean_t get_all;

  if (name_owner != NULL)
    {
      G_LOCK (properties_lock);
      /* Must free first, since on_name_owner_changed() could run before us */
      g_free (proxy->priv->name_owner);
      proxy->priv->name_owner = xstrdup (name_owner);
      G_UNLOCK (properties_lock);
    }

  get_all = TRUE;

  if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES)
    {
      /* Don't load properties if the API user doesn't want them */
      get_all = FALSE;
    }
  else if (name_owner == NULL && proxy->priv->name != NULL)
    {
      /* Don't attempt to load properties if the name_owner is NULL (which
       * usually means the name isn't owned), unless name is also NULL (which
       * means we actually wanted to talk to the directly-connected process -
       * either dbus-daemon or a peer - instead of going via dbus-daemon)
       */
        get_all = FALSE;
    }

  if (get_all)
    {
      /* load all properties asynchronously */
      xdbus_connection_call (proxy->priv->connection,
                              name_owner,
                              proxy->priv->object_path,
                              "org.freedesktop.DBus.Properties",
                              "GetAll",
                              xvariant_new ("(s)", proxy->priv->interface_name),
                              G_VARIANT_TYPE ("(a{sv})"),
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,           /* timeout */
                              xtask_get_cancellable (task),
                              (xasync_ready_callback_t) async_init_get_all_cb,
                              task);
    }
  else
    {
      xtask_return_pointer (task, NULL, NULL);
      xobject_unref (task);
    }
}

static void
async_init_get_name_owner_cb (xdbus_connection_t *connection,
                              xasync_result_t    *res,
                              xpointer_t         user_data)
{
  xtask_t *task = user_data;
  xerror_t *error;
  xvariant_t *result;

  error = NULL;
  result = xdbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      if (error->domain == G_DBUS_ERROR &&
          error->code == G_DBUS_ERROR_NAME_HAS_NO_OWNER)
        {
          xerror_free (error);
          async_init_data_set_name_owner (task, NULL);
        }
      else
        {
          xtask_return_error (task, error);
          xobject_unref (task);
        }
    }
  else
    {
      /* borrowed from result to avoid an extra copy */
      const xchar_t *name_owner;

      xvariant_get (result, "(&s)", &name_owner);
      async_init_data_set_name_owner (task, name_owner);
      xvariant_unref (result);
    }
}

static void
async_init_call_get_name_owner (xtask_t *task)
{
  xdbus_proxy_t *proxy = xtask_get_source_object (task);

  xdbus_connection_call (proxy->priv->connection,
                          "org.freedesktop.DBus",  /* name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface */
                          "GetNameOwner",
                          xvariant_new ("(s)",
                                         proxy->priv->name),
                          G_VARIANT_TYPE ("(s)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,           /* timeout */
                          xtask_get_cancellable (task),
                          (xasync_ready_callback_t) async_init_get_name_owner_cb,
                          task);
}

static void
async_init_start_service_by_name_cb (xdbus_connection_t *connection,
                                     xasync_result_t    *res,
                                     xpointer_t         user_data)
{
  xtask_t *task = user_data;
  xdbus_proxy_t *proxy = xtask_get_source_object (task);
  xerror_t *error;
  xvariant_t *result;

  error = NULL;
  result = xdbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      /* Errors are not unexpected; the bus will reply e.g.
       *
       *   org.freedesktop.DBus.Error.ServiceUnknown: The name org.gnome.Epiphany2
       *   was not provided by any .service files
       *
       * or (see #677718)
       *
       *   org.freedesktop.systemd1.Masked: Unit polkit.service is masked.
       *
       * This doesn't mean that the name doesn't have an owner, just
       * that it's not provided by a .service file or can't currently
       * be started.
       *
       * In particular, in both cases, it could be that a service
       * owner will actually appear later. So instead of erroring out,
       * we just proceed to invoke GetNameOwner() if dealing with the
       * kind of errors above.
       */
      if (error->domain == G_DBUS_ERROR && error->code == G_DBUS_ERROR_SERVICE_UNKNOWN)
        {
          xerror_free (error);
        }
      else
        {
          xchar_t *remote_error = g_dbus_error_get_remote_error (error);
          if (xstrcmp0 (remote_error, "org.freedesktop.systemd1.Masked") == 0)
            {
              xerror_free (error);
              g_free (remote_error);
            }
          else
            {
              g_dbus_error_strip_remote_error (error);
              g_prefix_error (&error,
                              _("Error calling StartServiceByName for %s: "),
                              proxy->priv->name);
              g_free (remote_error);
              goto failed;
            }
        }
    }
  else
    {
      xuint32_t start_service_result;
      xvariant_get (result,
                     "(u)",
                     &start_service_result);
      xvariant_unref (result);
      if (start_service_result == 1 ||  /* DBUS_START_REPLY_SUCCESS */
          start_service_result == 2)    /* DBUS_START_REPLY_ALREADY_RUNNING */
        {
          /* continue to invoke GetNameOwner() */
        }
      else
        {
          error = xerror_new (G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               _("Unexpected reply %d from StartServiceByName(\"%s\") method"),
                               start_service_result,
                               proxy->priv->name);
          goto failed;
        }
    }

  async_init_call_get_name_owner (task);
  return;

 failed:
  g_warn_if_fail (error != NULL);
  xtask_return_error (task, error);
  xobject_unref (task);
}

static void
async_init_call_start_service_by_name (xtask_t *task)
{
  xdbus_proxy_t *proxy = xtask_get_source_object (task);

  xdbus_connection_call (proxy->priv->connection,
                          "org.freedesktop.DBus",  /* name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface */
                          "StartServiceByName",
                          xvariant_new ("(su)",
                                         proxy->priv->name,
                                         0),
                          G_VARIANT_TYPE ("(u)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,           /* timeout */
                          xtask_get_cancellable (task),
                          (xasync_ready_callback_t) async_init_start_service_by_name_cb,
                          task);
}

static void
async_initable_init_second_async (xasync_initable_t      *initable,
                                  xint_t                 io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (initable);
  xtask_t *task;

  task = xtask_new (proxy, cancellable, callback, user_data);
  xtask_set_source_tag (task, async_initable_init_second_async);
  xtask_set_name (task, "[gio] D-Bus proxy init");
  xtask_set_priority (task, io_priority);

  /* Check name ownership asynchronously - possibly also start the service */
  if (proxy->priv->name == NULL)
    {
      /* Do nothing */
      async_init_data_set_name_owner (task, NULL);
    }
  else if (g_dbus_is_unique_name (proxy->priv->name))
    {
      async_init_data_set_name_owner (task, proxy->priv->name);
    }
  else
    {
      if ((proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START) ||
          (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION))
        {
          async_init_call_get_name_owner (task);
        }
      else
        {
          async_init_call_start_service_by_name (task);
        }
    }
}

static xboolean_t
async_initable_init_second_finish (xasync_initable_t  *initable,
                                   xasync_result_t    *res,
                                   xerror_t         **error)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (initable);
  xtask_t *task = XTASK (res);
  xvariant_t *result;
  xboolean_t ret;

  ret = !xtask_had_error (task);

  result = xtask_propagate_pointer (task, error);
  if (result != NULL)
    {
      process_get_all_reply (proxy, result);
      xvariant_unref (result);
    }

  proxy->priv->initialized = TRUE;
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
async_initable_init_first (xasync_initable_t *initable)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (initable);
  GDBusSignalFlags signal_flags = G_DBUS_SIGNAL_FLAGS_NONE;

  if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_NO_MATCH_RULE)
    signal_flags |= G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE;

  if (!(proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES))
    {
      /* subscribe to PropertiesChanged() */
      proxy->priv->properties_changed_subscription_id =
        xdbus_connection_signal_subscribe (proxy->priv->connection,
                                            proxy->priv->name,
                                            "org.freedesktop.DBus.Properties",
                                            "PropertiesChanged",
                                            proxy->priv->object_path,
                                            proxy->priv->interface_name,
                                            signal_flags,
                                            on_properties_changed,
                                            weak_ref_new (G_OBJECT (proxy)),
                                            (xdestroy_notify_t) weak_ref_free);
    }

  if (!(proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS))
    {
      /* subscribe to all signals for the object */
      proxy->priv->signals_subscription_id =
        xdbus_connection_signal_subscribe (proxy->priv->connection,
                                            proxy->priv->name,
                                            proxy->priv->interface_name,
                                            NULL,                        /* member */
                                            proxy->priv->object_path,
                                            NULL,                        /* arg0 */
                                            signal_flags,
                                            on_signal_received,
                                            weak_ref_new (G_OBJECT (proxy)),
                                            (xdestroy_notify_t) weak_ref_free);
    }

  if (proxy->priv->name != NULL &&
      (xdbus_connection_get_flags (proxy->priv->connection) & G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION))
    {
      proxy->priv->name_owner_changed_subscription_id =
        xdbus_connection_signal_subscribe (proxy->priv->connection,
                                            "org.freedesktop.DBus",  /* name */
                                            "org.freedesktop.DBus",  /* interface */
                                            "NameOwnerChanged",      /* signal name */
                                            "/org/freedesktop/DBus", /* path */
                                            proxy->priv->name,       /* arg0 */
                                            signal_flags,
                                            on_name_owner_changed,
                                            weak_ref_new (G_OBJECT (proxy)),
                                            (xdestroy_notify_t) weak_ref_free);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

/* initialization is split into two parts - the first is the
 * non-blocking part that requires the callers xmain_context_t - the
 * second is a blocking part async part that doesn't require the
 * callers xmain_context_t.. we do this split so the code can be reused
 * in the xinitable_t implementation below.
 *
 * Note that obtaining a xdbus_connection_t is not shared between the two
 * paths.
 */

static void
init_second_async_cb (xobject_t       *source_object,
		      xasync_result_t  *res,
		      xpointer_t       user_data)
{
  xtask_t *task = user_data;
  xerror_t *error = NULL;

  if (async_initable_init_second_finish (XASYNC_INITABLE (source_object), res, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

static void
get_connection_cb (xobject_t       *source_object,
                   xasync_result_t  *res,
                   xpointer_t       user_data)
{
  xtask_t *task = user_data;
  xdbus_proxy_t *proxy = xtask_get_source_object (task);
  xerror_t *error;

  error = NULL;
  proxy->priv->connection = g_bus_get_finish (res, &error);
  if (proxy->priv->connection == NULL)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
    }
  else
    {
      async_initable_init_first (XASYNC_INITABLE (proxy));
      async_initable_init_second_async (XASYNC_INITABLE (proxy),
                                        xtask_get_priority (task),
                                        xtask_get_cancellable (task),
                                        init_second_async_cb,
                                        task);
    }
}

static void
async_initable_init_async (xasync_initable_t      *initable,
                           xint_t                 io_priority,
                           xcancellable_t        *cancellable,
                           xasync_ready_callback_t  callback,
                           xpointer_t             user_data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (initable);
  xtask_t *task;

  task = xtask_new (proxy, cancellable, callback, user_data);
  xtask_set_source_tag (task, async_initable_init_async);
  xtask_set_name (task, "[gio] D-Bus proxy init");
  xtask_set_priority (task, io_priority);

  if (proxy->priv->bus_type != G_BUS_TYPE_NONE)
    {
      g_assert (proxy->priv->connection == NULL);

      g_bus_get (proxy->priv->bus_type,
                 cancellable,
                 get_connection_cb,
                 task);
    }
  else
    {
      async_initable_init_first (initable);
      async_initable_init_second_async (initable, io_priority, cancellable,
                                        init_second_async_cb, task);
    }
}

static xboolean_t
async_initable_init_finish (xasync_initable_t  *initable,
                            xasync_result_t    *res,
                            xerror_t         **error)
{
  return xtask_propagate_boolean (XTASK (res), error);
}

static void
async_initable_iface_init (xasync_initable_iface_t *async_initable_iface)
{
  async_initable_iface->init_async = async_initable_init_async;
  async_initable_iface->init_finish = async_initable_init_finish;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xmain_context_t *context;
  xmain_loop_t *loop;
  xasync_result_t *res;
} InitableAsyncInitableData;

static void
async_initable_init_async_cb (xobject_t      *source_object,
                              xasync_result_t *res,
                              xpointer_t      user_data)
{
  InitableAsyncInitableData *data = user_data;
  data->res = xobject_ref (res);
  xmain_loop_quit (data->loop);
}

/* Simply reuse the xasync_initable_t implementation but run the first
 * part (that is non-blocking and requires the callers xmain_context_t)
 * with the callers xmain_context_t.. and the second with a private
 * xmain_context_t (bug 621310 is slightly related).
 *
 * Note that obtaining a xdbus_connection_t is not shared between the two
 * paths.
 */
static xboolean_t
initable_init (xinitable_t     *initable,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (initable);
  InitableAsyncInitableData *data;
  xboolean_t ret;

  ret = FALSE;

  if (proxy->priv->bus_type != G_BUS_TYPE_NONE)
    {
      g_assert (proxy->priv->connection == NULL);
      proxy->priv->connection = g_bus_get_sync (proxy->priv->bus_type,
                                                cancellable,
                                                error);
      if (proxy->priv->connection == NULL)
        goto out;
    }

  async_initable_init_first (XASYNC_INITABLE (initable));

  data = g_new0 (InitableAsyncInitableData, 1);
  data->context = xmain_context_new ();
  data->loop = xmain_loop_new (data->context, FALSE);

  xmain_context_push_thread_default (data->context);

  async_initable_init_second_async (XASYNC_INITABLE (initable),
                                    G_PRIORITY_DEFAULT,
                                    cancellable,
                                    async_initable_init_async_cb,
                                    data);

  xmain_loop_run (data->loop);

  ret = async_initable_init_second_finish (XASYNC_INITABLE (initable),
                                           data->res,
                                           error);

  xmain_context_pop_thread_default (data->context);

  xmain_context_unref (data->context);
  xmain_loop_unref (data->loop);
  xobject_unref (data->res);
  g_free (data);

 out:

  return ret;
}

static void
initable_iface_init (xinitable_iface_t *initable_iface)
{
  initable_iface->init = initable_init;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_proxy_new:
 * @connection: A #xdbus_connection_t.
 * @flags: Flags used when constructing the proxy.
 * @info: (nullable): A #xdbus_interface_info_t specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: (nullable): A bus name (well-known or unique) or %NULL if @connection is not a message bus connection.
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @callback: Callback function to invoke when the proxy is ready.
 * @user_data: User data to pass to @callback.
 *
 * Creates a proxy for accessing @interface_name on the remote object
 * at @object_path owned by @name at @connection and asynchronously
 * loads D-Bus properties unless the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES flag is used. Connect to
 * the #xdbus_proxy_t::g-properties-changed signal to get notified about
 * property changes.
 *
 * If the %G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS flag is not set, also sets up
 * match rules for signals. Connect to the #xdbus_proxy_t::g-signal signal
 * to handle signals from the remote object.
 *
 * If both %G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES and
 * %G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS are set, this constructor is
 * guaranteed to complete immediately without blocking.
 *
 * If @name is a well-known name and the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START and %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION
 * flags aren't set and no name owner currently exists, the message bus
 * will be requested to launch a name owner for the name.
 *
 * This is a failable asynchronous constructor - when the proxy is
 * ready, @callback will be invoked and you can use
 * xdbus_proxy_new_finish() to get the result.
 *
 * See xdbus_proxy_new_sync() and for a synchronous version of this constructor.
 *
 * #xdbus_proxy_t is used in this [example][gdbus-wellknown-proxy].
 *
 * Since: 2.26
 */
void
xdbus_proxy_new (xdbus_connection_t     *connection,
                  xdbus_proxy_flags_t      flags,
                  xdbus_interface_info_t  *info,
                  const xchar_t         *name,
                  const xchar_t         *object_path,
                  const xchar_t         *interface_name,
                  xcancellable_t        *cancellable,
                  xasync_ready_callback_t  callback,
                  xpointer_t             user_data)
{
  _g_dbus_initialize ();

  g_return_if_fail (X_IS_DBUS_CONNECTION (connection));
  g_return_if_fail ((name == NULL && xdbus_connection_get_unique_name (connection) == NULL) || g_dbus_is_name (name));
  g_return_if_fail (xvariant_is_object_path (object_path));
  g_return_if_fail (g_dbus_is_interface_name (interface_name));

  xasync_initable_new_async (XTYPE_DBUS_PROXY,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "g-flags", flags,
                              "g-interface-info", info,
                              "g-name", name,
                              "g-connection", connection,
                              "g-object-path", object_path,
                              "g-interface-name", interface_name,
                              NULL);
}

/**
 * xdbus_proxy_new_finish:
 * @res: A #xasync_result_t obtained from the #xasync_ready_callback_t function passed to xdbus_proxy_new().
 * @error: Return location for error or %NULL.
 *
 * Finishes creating a #xdbus_proxy_t.
 *
 * Returns: (transfer full): A #xdbus_proxy_t or %NULL if @error is set.
 *    Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_proxy_t *
xdbus_proxy_new_finish (xasync_result_t  *res,
                         xerror_t       **error)
{
  xobject_t *object;
  xobject_t *source_object;

  source_object = xasync_result_get_source_object (res);
  g_assert (source_object != NULL);

  object = xasync_initable_new_finish (XASYNC_INITABLE (source_object),
                                        res,
                                        error);
  xobject_unref (source_object);

  if (object != NULL)
    return G_DBUS_PROXY (object);
  else
    return NULL;
}

/**
 * xdbus_proxy_new_sync:
 * @connection: A #xdbus_connection_t.
 * @flags: Flags used when constructing the proxy.
 * @info: (nullable): A #xdbus_interface_info_t specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: (nullable): A bus name (well-known or unique) or %NULL if @connection is not a message bus connection.
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: (nullable): Return location for error or %NULL.
 *
 * Creates a proxy for accessing @interface_name on the remote object
 * at @object_path owned by @name at @connection and synchronously
 * loads D-Bus properties unless the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES flag is used.
 *
 * If the %G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS flag is not set, also sets up
 * match rules for signals. Connect to the #xdbus_proxy_t::g-signal signal
 * to handle signals from the remote object.
 *
 * If both %G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES and
 * %G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS are set, this constructor is
 * guaranteed to return immediately without blocking.
 *
 * If @name is a well-known name and the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START and %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION
 * flags aren't set and no name owner currently exists, the message bus
 * will be requested to launch a name owner for the name.
 *
 * This is a synchronous failable constructor. See xdbus_proxy_new()
 * and xdbus_proxy_new_finish() for the asynchronous version.
 *
 * #xdbus_proxy_t is used in this [example][gdbus-wellknown-proxy].
 *
 * Returns: (transfer full): A #xdbus_proxy_t or %NULL if error is set.
 *    Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_proxy_t *
xdbus_proxy_new_sync (xdbus_connection_t     *connection,
                       xdbus_proxy_flags_t      flags,
                       xdbus_interface_info_t  *info,
                       const xchar_t         *name,
                       const xchar_t         *object_path,
                       const xchar_t         *interface_name,
                       xcancellable_t        *cancellable,
                       xerror_t             **error)
{
  xinitable_t *initable;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail ((name == NULL && xdbus_connection_get_unique_name (connection) == NULL) ||
                        g_dbus_is_name (name), NULL);
  g_return_val_if_fail (xvariant_is_object_path (object_path), NULL);
  g_return_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);

  initable = xinitable_new (XTYPE_DBUS_PROXY,
                             cancellable,
                             error,
                             "g-flags", flags,
                             "g-interface-info", info,
                             "g-name", name,
                             "g-connection", connection,
                             "g-object-path", object_path,
                             "g-interface-name", interface_name,
                             NULL);
  if (initable != NULL)
    return G_DBUS_PROXY (initable);
  else
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_proxy_new_for_bus:
 * @bus_type: A #xbus_type_t.
 * @flags: Flags used when constructing the proxy.
 * @info: (nullable): A #xdbus_interface_info_t specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: A bus name (well-known or unique).
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @callback: Callback function to invoke when the proxy is ready.
 * @user_data: User data to pass to @callback.
 *
 * Like xdbus_proxy_new() but takes a #xbus_type_t instead of a #xdbus_connection_t.
 *
 * #xdbus_proxy_t is used in this [example][gdbus-wellknown-proxy].
 *
 * Since: 2.26
 */
void
xdbus_proxy_new_for_bus (xbus_type_t             bus_type,
                          xdbus_proxy_flags_t      flags,
                          xdbus_interface_info_t  *info,
                          const xchar_t         *name,
                          const xchar_t         *object_path,
                          const xchar_t         *interface_name,
                          xcancellable_t        *cancellable,
                          xasync_ready_callback_t  callback,
                          xpointer_t             user_data)
{
  _g_dbus_initialize ();

  g_return_if_fail (g_dbus_is_name (name));
  g_return_if_fail (xvariant_is_object_path (object_path));
  g_return_if_fail (g_dbus_is_interface_name (interface_name));

  xasync_initable_new_async (XTYPE_DBUS_PROXY,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "g-flags", flags,
                              "g-interface-info", info,
                              "g-name", name,
                              "g-bus-type", bus_type,
                              "g-object-path", object_path,
                              "g-interface-name", interface_name,
                              NULL);
}

/**
 * xdbus_proxy_new_for_bus_finish:
 * @res: A #xasync_result_t obtained from the #xasync_ready_callback_t function passed to xdbus_proxy_new_for_bus().
 * @error: Return location for error or %NULL.
 *
 * Finishes creating a #xdbus_proxy_t.
 *
 * Returns: (transfer full): A #xdbus_proxy_t or %NULL if @error is set.
 *    Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_proxy_t *
xdbus_proxy_new_for_bus_finish (xasync_result_t  *res,
                                 xerror_t       **error)
{
  return xdbus_proxy_new_finish (res, error);
}

/**
 * xdbus_proxy_new_for_bus_sync:
 * @bus_type: A #xbus_type_t.
 * @flags: Flags used when constructing the proxy.
 * @info: (nullable): A #xdbus_interface_info_t specifying the minimal interface
 *        that @proxy conforms to or %NULL.
 * @name: A bus name (well-known or unique).
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Like xdbus_proxy_new_sync() but takes a #xbus_type_t instead of a #xdbus_connection_t.
 *
 * #xdbus_proxy_t is used in this [example][gdbus-wellknown-proxy].
 *
 * Returns: (transfer full): A #xdbus_proxy_t or %NULL if error is set.
 *    Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_proxy_t *
xdbus_proxy_new_for_bus_sync (xbus_type_t             bus_type,
                               xdbus_proxy_flags_t      flags,
                               xdbus_interface_info_t  *info,
                               const xchar_t         *name,
                               const xchar_t         *object_path,
                               const xchar_t         *interface_name,
                               xcancellable_t        *cancellable,
                               xerror_t             **error)
{
  xinitable_t *initable;

  _g_dbus_initialize ();

  g_return_val_if_fail (g_dbus_is_name (name), NULL);
  g_return_val_if_fail (xvariant_is_object_path (object_path), NULL);
  g_return_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);

  initable = xinitable_new (XTYPE_DBUS_PROXY,
                             cancellable,
                             error,
                             "g-flags", flags,
                             "g-interface-info", info,
                             "g-name", name,
                             "g-bus-type", bus_type,
                             "g-object-path", object_path,
                             "g-interface-name", interface_name,
                             NULL);
  if (initable != NULL)
    return G_DBUS_PROXY (initable);
  else
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_proxy_get_connection:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the connection @proxy is for.
 *
 * Returns: (transfer none) (not nullable): A #xdbus_connection_t owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
xdbus_connection_t *
xdbus_proxy_get_connection (xdbus_proxy_t *proxy)
{
  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->connection;
}

/**
 * xdbus_proxy_get_flags:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the flags that @proxy was constructed with.
 *
 * Returns: Flags from the #xdbus_proxy_flags_t enumeration.
 *
 * Since: 2.26
 */
xdbus_proxy_flags_t
xdbus_proxy_get_flags (xdbus_proxy_t *proxy)
{
  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), 0);
  return proxy->priv->flags;
}

/**
 * xdbus_proxy_get_name:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the name that @proxy was constructed for.
 *
 * When connected to a message bus, this will usually be non-%NULL.
 * However, it may be %NULL for a proxy that communicates using a peer-to-peer
 * pattern.
 *
 * Returns: (nullable): A string owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_proxy_get_name (xdbus_proxy_t *proxy)
{
  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->name;
}

/**
 * xdbus_proxy_get_name_owner:
 * @proxy: A #xdbus_proxy_t.
 *
 * The unique name that owns the name that @proxy is for or %NULL if
 * no-one currently owns that name. You may connect to the
 * #xobject_t::notify signal to track changes to the
 * #xdbus_proxy_t:g-name-owner property.
 *
 * Returns: (transfer full) (nullable): The name owner or %NULL if no name
 *    owner exists. Free with g_free().
 *
 * Since: 2.26
 */
xchar_t *
xdbus_proxy_get_name_owner (xdbus_proxy_t *proxy)
{
  xchar_t *ret;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);

  G_LOCK (properties_lock);
  ret = xstrdup (proxy->priv->name_owner);
  G_UNLOCK (properties_lock);
  return ret;
}

/**
 * xdbus_proxy_get_object_path:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the object path @proxy is for.
 *
 * Returns: (not nullable): A string owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_proxy_get_object_path (xdbus_proxy_t *proxy)
{
  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->object_path;
}

/**
 * xdbus_proxy_get_interface_name:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the D-Bus interface name @proxy is for.
 *
 * Returns: (not nullable): A string owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_proxy_get_interface_name (xdbus_proxy_t *proxy)
{
  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->interface_name;
}

/**
 * xdbus_proxy_get_default_timeout:
 * @proxy: A #xdbus_proxy_t.
 *
 * Gets the timeout to use if -1 (specifying default timeout) is
 * passed as @timeout_msec in the xdbus_proxy_call() and
 * xdbus_proxy_call_sync() functions.
 *
 * See the #xdbus_proxy_t:g-default-timeout property for more details.
 *
 * Returns: Timeout to use for @proxy.
 *
 * Since: 2.26
 */
xint_t
xdbus_proxy_get_default_timeout (xdbus_proxy_t *proxy)
{
  xint_t ret;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), -1);

  G_LOCK (properties_lock);
  ret = proxy->priv->timeout_msec;
  G_UNLOCK (properties_lock);
  return ret;
}

/**
 * xdbus_proxy_set_default_timeout:
 * @proxy: A #xdbus_proxy_t.
 * @timeout_msec: Timeout in milliseconds.
 *
 * Sets the timeout to use if -1 (specifying default timeout) is
 * passed as @timeout_msec in the xdbus_proxy_call() and
 * xdbus_proxy_call_sync() functions.
 *
 * See the #xdbus_proxy_t:g-default-timeout property for more details.
 *
 * Since: 2.26
 */
void
xdbus_proxy_set_default_timeout (xdbus_proxy_t *proxy,
                                  xint_t        timeout_msec)
{
  g_return_if_fail (X_IS_DBUS_PROXY (proxy));
  g_return_if_fail (timeout_msec == -1 || timeout_msec >= 0);

  G_LOCK (properties_lock);

  if (proxy->priv->timeout_msec != timeout_msec)
    {
      proxy->priv->timeout_msec = timeout_msec;
      G_UNLOCK (properties_lock);

      xobject_notify (G_OBJECT (proxy), "g-default-timeout");
    }
  else
    {
      G_UNLOCK (properties_lock);
    }
}

/**
 * xdbus_proxy_get_interface_info:
 * @proxy: A #xdbus_proxy_t
 *
 * Returns the #xdbus_interface_info_t, if any, specifying the interface
 * that @proxy conforms to. See the #xdbus_proxy_t:g-interface-info
 * property for more details.
 *
 * Returns: (transfer none) (nullable): A #xdbus_interface_info_t or %NULL.
 *    Do not unref the returned object, it is owned by @proxy.
 *
 * Since: 2.26
 */
xdbus_interface_info_t *
xdbus_proxy_get_interface_info (xdbus_proxy_t *proxy)
{
  xdbus_interface_info_t *ret;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);

  G_LOCK (properties_lock);
  ret = proxy->priv->expected_interface;
  G_UNLOCK (properties_lock);
  /* FIXME: returning a borrowed ref with no guarantee that nobody will
   * call xdbus_proxy_set_interface_info() and make it invalid...
   */
  return ret;
}

/**
 * xdbus_proxy_set_interface_info:
 * @proxy: A #xdbus_proxy_t
 * @info: (transfer none) (nullable): Minimum interface this proxy conforms to
 *    or %NULL to unset.
 *
 * Ensure that interactions with @proxy conform to the given
 * interface. See the #xdbus_proxy_t:g-interface-info property for more
 * details.
 *
 * Since: 2.26
 */
void
xdbus_proxy_set_interface_info (xdbus_proxy_t         *proxy,
                                 xdbus_interface_info_t *info)
{
  g_return_if_fail (X_IS_DBUS_PROXY (proxy));
  G_LOCK (properties_lock);

  if (proxy->priv->expected_interface != NULL)
    {
      g_dbus_interface_info_cache_release (proxy->priv->expected_interface);
      g_dbus_interface_info_unref (proxy->priv->expected_interface);
    }
  proxy->priv->expected_interface = info != NULL ? g_dbus_interface_info_ref (info) : NULL;
  if (proxy->priv->expected_interface != NULL)
    g_dbus_interface_info_cache_build (proxy->priv->expected_interface);

  G_UNLOCK (properties_lock);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
maybe_split_method_name (const xchar_t  *method_name,
                         xchar_t       **out_interface_name,
                         const xchar_t **out_method_name)
{
  xboolean_t was_split;

  was_split = FALSE;
  g_assert (out_interface_name != NULL);
  g_assert (out_method_name != NULL);
  *out_interface_name = NULL;
  *out_method_name = NULL;

  if (strchr (method_name, '.') != NULL)
    {
      xchar_t *p;
      xchar_t *last_dot;

      p = xstrdup (method_name);
      last_dot = strrchr (p, '.');
      *last_dot = '\0';

      *out_interface_name = p;
      *out_method_name = last_dot + 1;

      was_split = TRUE;
    }

  return was_split;
}

typedef struct
{
  xvariant_t *value;
#ifdef G_OS_UNIX
  xunix_fd_list_t *fd_list;
#endif
} ReplyData;

static void
reply_data_free (ReplyData *data)
{
  xvariant_unref (data->value);
#ifdef G_OS_UNIX
  if (data->fd_list != NULL)
    xobject_unref (data->fd_list);
#endif
  g_slice_free (ReplyData, data);
}

static void
reply_cb (xdbus_connection_t *connection,
          xasync_result_t    *res,
          xpointer_t         user_data)
{
  xtask_t *task = user_data;
  xvariant_t *value;
  xerror_t *error;
#ifdef G_OS_UNIX
  xunix_fd_list_t *fd_list;
#endif

  error = NULL;
#ifdef G_OS_UNIX
  value = xdbus_connection_call_with_unix_fd_list_finish (connection,
                                                           &fd_list,
                                                           res,
                                                           &error);
#else
  value = xdbus_connection_call_finish (connection,
                                         res,
                                         &error);
#endif
  if (error != NULL)
    {
      xtask_return_error (task, error);
    }
  else
    {
      ReplyData *data;
      data = g_slice_new0 (ReplyData);
      data->value = value;
#ifdef G_OS_UNIX
      data->fd_list = fd_list;
#endif
      xtask_return_pointer (task, data, (xdestroy_notify_t) reply_data_free);
    }

  xobject_unref (task);
}

/* properties_lock must be held for as long as you will keep the
 * returned value
 */
static const xdbus_method_info_t *
lookup_method_info (xdbus_proxy_t  *proxy,
                    const xchar_t *method_name)
{
  const xdbus_method_info_t *info = NULL;

  if (proxy->priv->expected_interface == NULL)
    goto out;

  info = g_dbus_interface_info_lookup_method (proxy->priv->expected_interface, method_name);

out:
  return info;
}

/* properties_lock must be held for as long as you will keep the
 * returned value
 */
static const xchar_t *
get_destination_for_call (xdbus_proxy_t *proxy)
{
  const xchar_t *ret;

  ret = NULL;

  /* If proxy->priv->name is a unique name, then proxy->priv->name_owner
   * is never NULL and always the same as proxy->priv->name. We use this
   * knowledge to avoid checking if proxy->priv->name is a unique or
   * well-known name.
   */
  ret = proxy->priv->name_owner;
  if (ret != NULL)
    goto out;

  if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
    goto out;

  ret = proxy->priv->name;

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
xdbus_proxy_call_internal (xdbus_proxy_t          *proxy,
                            const xchar_t         *method_name,
                            xvariant_t            *parameters,
                            GDBusCallFlags       flags,
                            xint_t                 timeout_msec,
                            xunix_fd_list_t         *fd_list,
                            xcancellable_t        *cancellable,
                            xasync_ready_callback_t  callback,
                            xpointer_t             user_data)
{
  xtask_t *task;
  xboolean_t was_split;
  xchar_t *split_interface_name;
  const xchar_t *split_method_name;
  const xchar_t *target_method_name;
  const xchar_t *target_interface_name;
  xchar_t *destination;
  xvariant_type_t *reply_type;
  xasync_ready_callback_t my_callback;

  g_return_if_fail (X_IS_DBUS_PROXY (proxy));
  g_return_if_fail (g_dbus_is_member_name (method_name) || g_dbus_is_interface_name (method_name));
  g_return_if_fail (parameters == NULL || xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE));
  g_return_if_fail (timeout_msec == -1 || timeout_msec >= 0);
#ifdef G_OS_UNIX
  g_return_if_fail (fd_list == NULL || X_IS_UNIX_FD_LIST (fd_list));
#else
  g_return_if_fail (fd_list == NULL);
#endif

  reply_type = NULL;
  split_interface_name = NULL;

  /* xdbus_connection_call() is optimised for the case of a NULL
   * callback.  If we get a NULL callback from our user then make sure
   * we pass along a NULL callback for ourselves as well.
   */
  if (callback != NULL)
    {
      my_callback = (xasync_ready_callback_t) reply_cb;
      task = xtask_new (proxy, cancellable, callback, user_data);
      xtask_set_source_tag (task, xdbus_proxy_call_internal);
      xtask_set_name (task, "[gio] D-Bus proxy call");
    }
  else
    {
      my_callback = NULL;
      task = NULL;
    }

  G_LOCK (properties_lock);

  was_split = maybe_split_method_name (method_name, &split_interface_name, &split_method_name);
  target_method_name = was_split ? split_method_name : method_name;
  target_interface_name = was_split ? split_interface_name : proxy->priv->interface_name;

  /* Warn if method is unexpected (cf. :g-interface-info) */
  if (!was_split)
    {
      const xdbus_method_info_t *expected_method_info;
      expected_method_info = lookup_method_info (proxy, target_method_name);
      if (expected_method_info != NULL)
        reply_type = _g_dbus_compute_complete_signature (expected_method_info->out_args);
    }

  destination = NULL;
  if (proxy->priv->name != NULL)
    {
      destination = xstrdup (get_destination_for_call (proxy));
      if (destination == NULL)
        {
          if (task != NULL)
            {
              xtask_return_new_error (task,
                                       G_IO_ERROR,
                                       G_IO_ERROR_FAILED,
                                       _("Cannot invoke method; proxy is for the well-known name %s without an owner, and proxy was constructed with the G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START flag"),
                                       proxy->priv->name);
              xobject_unref (task);
            }
          G_UNLOCK (properties_lock);
          goto out;
        }
    }

  G_UNLOCK (properties_lock);

#ifdef G_OS_UNIX
  xdbus_connection_call_with_unix_fd_list (proxy->priv->connection,
                                            destination,
                                            proxy->priv->object_path,
                                            target_interface_name,
                                            target_method_name,
                                            parameters,
                                            reply_type,
                                            flags,
                                            timeout_msec == -1 ? proxy->priv->timeout_msec : timeout_msec,
                                            fd_list,
                                            cancellable,
                                            my_callback,
                                            task);
#else
  xdbus_connection_call (proxy->priv->connection,
                          destination,
                          proxy->priv->object_path,
                          target_interface_name,
                          target_method_name,
                          parameters,
                          reply_type,
                          flags,
                          timeout_msec == -1 ? proxy->priv->timeout_msec : timeout_msec,
                          cancellable,
                          my_callback,
                          task);
#endif

 out:
  if (reply_type != NULL)
    xvariant_type_free (reply_type);

  g_free (destination);
  g_free (split_interface_name);
}

static xvariant_t *
xdbus_proxy_call_finish_internal (xdbus_proxy_t    *proxy,
                                   xunix_fd_list_t  **out_fd_list,
                                   xasync_result_t  *res,
                                   xerror_t       **error)
{
  xvariant_t *value;
  ReplyData *data;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  g_return_val_if_fail (xtask_is_valid (res, proxy), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  value = NULL;

  data = xtask_propagate_pointer (XTASK (res), error);
  if (!data)
    goto out;

  value = xvariant_ref (data->value);
#ifdef G_OS_UNIX
  if (out_fd_list != NULL)
    *out_fd_list = data->fd_list != NULL ? xobject_ref (data->fd_list) : NULL;
#endif
  reply_data_free (data);

 out:
  return value;
}

static xvariant_t *
xdbus_proxy_call_sync_internal (xdbus_proxy_t      *proxy,
                                 const xchar_t     *method_name,
                                 xvariant_t        *parameters,
                                 GDBusCallFlags   flags,
                                 xint_t             timeout_msec,
                                 xunix_fd_list_t     *fd_list,
                                 xunix_fd_list_t    **out_fd_list,
                                 xcancellable_t    *cancellable,
                                 xerror_t         **error)
{
  xvariant_t *ret;
  xboolean_t was_split;
  xchar_t *split_interface_name;
  const xchar_t *split_method_name;
  const xchar_t *target_method_name;
  const xchar_t *target_interface_name;
  xchar_t *destination;
  xvariant_type_t *reply_type;

  g_return_val_if_fail (X_IS_DBUS_PROXY (proxy), NULL);
  g_return_val_if_fail (g_dbus_is_member_name (method_name) || g_dbus_is_interface_name (method_name), NULL);
  g_return_val_if_fail (parameters == NULL || xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE), NULL);
  g_return_val_if_fail (timeout_msec == -1 || timeout_msec >= 0, NULL);
#ifdef G_OS_UNIX
  g_return_val_if_fail (fd_list == NULL || X_IS_UNIX_FD_LIST (fd_list), NULL);
#else
  g_return_val_if_fail (fd_list == NULL, NULL);
#endif
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  reply_type = NULL;

  G_LOCK (properties_lock);

  was_split = maybe_split_method_name (method_name, &split_interface_name, &split_method_name);
  target_method_name = was_split ? split_method_name : method_name;
  target_interface_name = was_split ? split_interface_name : proxy->priv->interface_name;

  /* Warn if method is unexpected (cf. :g-interface-info) */
  if (!was_split)
    {
      const xdbus_method_info_t *expected_method_info;
      expected_method_info = lookup_method_info (proxy, target_method_name);
      if (expected_method_info != NULL)
        reply_type = _g_dbus_compute_complete_signature (expected_method_info->out_args);
    }

  destination = NULL;
  if (proxy->priv->name != NULL)
    {
      destination = xstrdup (get_destination_for_call (proxy));
      if (destination == NULL)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       _("Cannot invoke method; proxy is for the well-known name %s without an owner, and proxy was constructed with the G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START flag"),
                       proxy->priv->name);
          ret = NULL;
          G_UNLOCK (properties_lock);
          goto out;
        }
    }

  G_UNLOCK (properties_lock);

#ifdef G_OS_UNIX
  ret = xdbus_connection_call_with_unix_fd_list_sync (proxy->priv->connection,
                                                       destination,
                                                       proxy->priv->object_path,
                                                       target_interface_name,
                                                       target_method_name,
                                                       parameters,
                                                       reply_type,
                                                       flags,
                                                       timeout_msec == -1 ? proxy->priv->timeout_msec : timeout_msec,
                                                       fd_list,
                                                       out_fd_list,
                                                       cancellable,
                                                       error);
#else
  ret = xdbus_connection_call_sync (proxy->priv->connection,
                                     destination,
                                     proxy->priv->object_path,
                                     target_interface_name,
                                     target_method_name,
                                     parameters,
                                     reply_type,
                                     flags,
                                     timeout_msec == -1 ? proxy->priv->timeout_msec : timeout_msec,
                                     cancellable,
                                     error);
#endif

 out:
  if (reply_type != NULL)
    xvariant_type_free (reply_type);

  g_free (destination);
  g_free (split_interface_name);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_proxy_call:
 * @proxy: A #xdbus_proxy_t.
 * @method_name: Name of method to invoke.
 * @parameters: (nullable): A #xvariant_t tuple with parameters for the signal or %NULL if not passing parameters.
 * @flags: Flags from the #GDBusCallFlags enumeration.
 * @timeout_msec: The timeout in milliseconds (with %G_MAXINT meaning
 *                "infinite") or -1 to use the proxy default timeout.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @callback: (nullable): A #xasync_ready_callback_t to call when the request is satisfied or %NULL if you don't
 * care about the result of the method invocation.
 * @user_data: The data to pass to @callback.
 *
 * Asynchronously invokes the @method_name method on @proxy.
 *
 * If @method_name contains any dots, then @name is split into interface and
 * method name parts. This allows using @proxy for invoking methods on
 * other interfaces.
 *
 * If the #xdbus_connection_t associated with @proxy is closed then
 * the operation will fail with %G_IO_ERROR_CLOSED. If
 * @cancellable is canceled, the operation will fail with
 * %G_IO_ERROR_CANCELLED. If @parameters contains a value not
 * compatible with the D-Bus protocol, the operation fails with
 * %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * If the @parameters #xvariant_t is floating, it is consumed. This allows
 * convenient 'inline' use of xvariant_new(), e.g.:
 * |[<!-- language="C" -->
 *  xdbus_proxy_call (proxy,
 *                     "TwoStrings",
 *                     xvariant_new ("(ss)",
 *                                    "Thing One",
 *                                    "Thing Two"),
 *                     G_DBUS_CALL_FLAGS_NONE,
 *                     -1,
 *                     NULL,
 *                     (xasync_ready_callback_t) two_strings_done,
 *                     &data);
 * ]|
 *
 * If @proxy has an expected interface (see
 * #xdbus_proxy_t:g-interface-info) and @method_name is referenced by it,
 * then the return value is checked against the return type.
 *
 * This is an asynchronous method. When the operation is finished,
 * @callback will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this method from.
 * You can then call xdbus_proxy_call_finish() to get the result of
 * the operation. See xdbus_proxy_call_sync() for the synchronous
 * version of this method.
 *
 * If @callback is %NULL then the D-Bus method call message will be sent with
 * the %G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED flag set.
 *
 * Since: 2.26
 */
void
xdbus_proxy_call (xdbus_proxy_t          *proxy,
                   const xchar_t         *method_name,
                   xvariant_t            *parameters,
                   GDBusCallFlags       flags,
                   xint_t                 timeout_msec,
                   xcancellable_t        *cancellable,
                   xasync_ready_callback_t  callback,
                   xpointer_t             user_data)
{
  xdbus_proxy_call_internal (proxy, method_name, parameters, flags, timeout_msec, NULL, cancellable, callback, user_data);
}

/**
 * xdbus_proxy_call_finish:
 * @proxy: A #xdbus_proxy_t.
 * @res: A #xasync_result_t obtained from the #xasync_ready_callback_t passed to xdbus_proxy_call().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with xdbus_proxy_call().
 *
 * Returns: %NULL if @error is set. Otherwise a #xvariant_t tuple with
 * return values. Free with xvariant_unref().
 *
 * Since: 2.26
 */
xvariant_t *
xdbus_proxy_call_finish (xdbus_proxy_t    *proxy,
                          xasync_result_t  *res,
                          xerror_t       **error)
{
  return xdbus_proxy_call_finish_internal (proxy, NULL, res, error);
}

/**
 * xdbus_proxy_call_sync:
 * @proxy: A #xdbus_proxy_t.
 * @method_name: Name of method to invoke.
 * @parameters: (nullable): A #xvariant_t tuple with parameters for the signal
 *              or %NULL if not passing parameters.
 * @flags: Flags from the #GDBusCallFlags enumeration.
 * @timeout_msec: The timeout in milliseconds (with %G_MAXINT meaning
 *                "infinite") or -1 to use the proxy default timeout.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the @method_name method on @proxy.
 *
 * If @method_name contains any dots, then @name is split into interface and
 * method name parts. This allows using @proxy for invoking methods on
 * other interfaces.
 *
 * If the #xdbus_connection_t associated with @proxy is disconnected then
 * the operation will fail with %G_IO_ERROR_CLOSED. If
 * @cancellable is canceled, the operation will fail with
 * %G_IO_ERROR_CANCELLED. If @parameters contains a value not
 * compatible with the D-Bus protocol, the operation fails with
 * %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * If the @parameters #xvariant_t is floating, it is consumed. This allows
 * convenient 'inline' use of xvariant_new(), e.g.:
 * |[<!-- language="C" -->
 *  xdbus_proxy_call_sync (proxy,
 *                          "TwoStrings",
 *                          xvariant_new ("(ss)",
 *                                         "Thing One",
 *                                         "Thing Two"),
 *                          G_DBUS_CALL_FLAGS_NONE,
 *                          -1,
 *                          NULL,
 *                          &error);
 * ]|
 *
 * The calling thread is blocked until a reply is received. See
 * xdbus_proxy_call() for the asynchronous version of this
 * method.
 *
 * If @proxy has an expected interface (see
 * #xdbus_proxy_t:g-interface-info) and @method_name is referenced by it,
 * then the return value is checked against the return type.
 *
 * Returns: %NULL if @error is set. Otherwise a #xvariant_t tuple with
 * return values. Free with xvariant_unref().
 *
 * Since: 2.26
 */
xvariant_t *
xdbus_proxy_call_sync (xdbus_proxy_t      *proxy,
                        const xchar_t     *method_name,
                        xvariant_t        *parameters,
                        GDBusCallFlags   flags,
                        xint_t             timeout_msec,
                        xcancellable_t    *cancellable,
                        xerror_t         **error)
{
  return xdbus_proxy_call_sync_internal (proxy, method_name, parameters, flags, timeout_msec, NULL, NULL, cancellable, error);
}

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_UNIX

/**
 * xdbus_proxy_call_with_unix_fd_list:
 * @proxy: A #xdbus_proxy_t.
 * @method_name: Name of method to invoke.
 * @parameters: (nullable): A #xvariant_t tuple with parameters for the signal or %NULL if not passing parameters.
 * @flags: Flags from the #GDBusCallFlags enumeration.
 * @timeout_msec: The timeout in milliseconds (with %G_MAXINT meaning
 *                "infinite") or -1 to use the proxy default timeout.
 * @fd_list: (nullable): A #xunix_fd_list_t or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @callback: (nullable): A #xasync_ready_callback_t to call when the request is satisfied or %NULL if you don't
 * care about the result of the method invocation.
 * @user_data: The data to pass to @callback.
 *
 * Like xdbus_proxy_call() but also takes a #xunix_fd_list_t object.
 *
 * This method is only available on UNIX.
 *
 * Since: 2.30
 */
void
xdbus_proxy_call_with_unix_fd_list (xdbus_proxy_t          *proxy,
                                     const xchar_t         *method_name,
                                     xvariant_t            *parameters,
                                     GDBusCallFlags       flags,
                                     xint_t                 timeout_msec,
                                     xunix_fd_list_t         *fd_list,
                                     xcancellable_t        *cancellable,
                                     xasync_ready_callback_t  callback,
                                     xpointer_t             user_data)
{
  xdbus_proxy_call_internal (proxy, method_name, parameters, flags, timeout_msec, fd_list, cancellable, callback, user_data);
}

/**
 * xdbus_proxy_call_with_unix_fd_list_finish:
 * @proxy: A #xdbus_proxy_t.
 * @out_fd_list: (out) (optional): Return location for a #xunix_fd_list_t or %NULL.
 * @res: A #xasync_result_t obtained from the #xasync_ready_callback_t passed to xdbus_proxy_call_with_unix_fd_list().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with xdbus_proxy_call_with_unix_fd_list().
 *
 * Returns: %NULL if @error is set. Otherwise a #xvariant_t tuple with
 * return values. Free with xvariant_unref().
 *
 * Since: 2.30
 */
xvariant_t *
xdbus_proxy_call_with_unix_fd_list_finish (xdbus_proxy_t    *proxy,
                                            xunix_fd_list_t  **out_fd_list,
                                            xasync_result_t  *res,
                                            xerror_t       **error)
{
  return xdbus_proxy_call_finish_internal (proxy, out_fd_list, res, error);
}

/**
 * xdbus_proxy_call_with_unix_fd_list_sync:
 * @proxy: A #xdbus_proxy_t.
 * @method_name: Name of method to invoke.
 * @parameters: (nullable): A #xvariant_t tuple with parameters for the signal
 *              or %NULL if not passing parameters.
 * @flags: Flags from the #GDBusCallFlags enumeration.
 * @timeout_msec: The timeout in milliseconds (with %G_MAXINT meaning
 *                "infinite") or -1 to use the proxy default timeout.
 * @fd_list: (nullable): A #xunix_fd_list_t or %NULL.
 * @out_fd_list: (out) (optional): Return location for a #xunix_fd_list_t or %NULL.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Like xdbus_proxy_call_sync() but also takes and returns #xunix_fd_list_t objects.
 *
 * This method is only available on UNIX.
 *
 * Returns: %NULL if @error is set. Otherwise a #xvariant_t tuple with
 * return values. Free with xvariant_unref().
 *
 * Since: 2.30
 */
xvariant_t *
xdbus_proxy_call_with_unix_fd_list_sync (xdbus_proxy_t      *proxy,
                                          const xchar_t     *method_name,
                                          xvariant_t        *parameters,
                                          GDBusCallFlags   flags,
                                          xint_t             timeout_msec,
                                          xunix_fd_list_t     *fd_list,
                                          xunix_fd_list_t    **out_fd_list,
                                          xcancellable_t    *cancellable,
                                          xerror_t         **error)
{
  return xdbus_proxy_call_sync_internal (proxy, method_name, parameters, flags, timeout_msec, fd_list, out_fd_list, cancellable, error);
}

#endif /* G_OS_UNIX */

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_interface_info_t *
_xdbus_proxy_get_info (xdbus_interface_t *interface)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (interface);
  return xdbus_proxy_get_interface_info (proxy);
}

static xdbus_object_t *
_xdbus_proxy_get_object (xdbus_interface_t *interface)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (interface);
  return proxy->priv->object;
}

static xdbus_object_t *
_xdbus_proxy_dup_object (xdbus_interface_t *interface)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (interface);
  xdbus_object_t *ret = NULL;

  G_LOCK (properties_lock);
  if (proxy->priv->object != NULL)
    ret = xobject_ref (proxy->priv->object);
  G_UNLOCK (properties_lock);
  return ret;
}

static void
_xdbus_proxy_set_object (xdbus_interface_t *interface,
                          xdbus_object_t    *object)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (interface);
  G_LOCK (properties_lock);
  if (proxy->priv->object != NULL)
    xobject_remove_weak_pointer (G_OBJECT (proxy->priv->object), (xpointer_t *) &proxy->priv->object);
  proxy->priv->object = object;
  if (proxy->priv->object != NULL)
    xobject_add_weak_pointer (G_OBJECT (proxy->priv->object), (xpointer_t *) &proxy->priv->object);
  G_UNLOCK (properties_lock);
}

static void
dbus_interface_iface_init (xdbus_interface_iface_t *dbus_interface_iface)
{
  dbus_interface_iface->get_info   = _xdbus_proxy_get_info;
  dbus_interface_iface->get_object = _xdbus_proxy_get_object;
  dbus_interface_iface->dup_object = _xdbus_proxy_dup_object;
  dbus_interface_iface->set_object = _xdbus_proxy_set_object;
}

/* ---------------------------------------------------------------------------------------------------- */
