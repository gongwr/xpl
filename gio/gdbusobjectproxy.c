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

#include "gdbusobject.h"
#include "gdbusobjectproxy.h"
#include "gdbusconnection.h"
#include "gdbusprivate.h"
#include "gdbusutils.h"
#include "gdbusproxy.h"

#include "glibintl.h"

/**
 * SECTION:gdbusobjectproxy
 * @short_description: Client-side D-Bus object
 * @include: gio/gio.h
 *
 * A #xdbus_object_proxy_t is an object used to represent a remote object
 * with one or more D-Bus interfaces. Normally, you don't instantiate
 * a #xdbus_object_proxy_t yourself - typically #xdbus_object_manager_client_t
 * is used to obtain it.
 *
 * Since: 2.30
 */

struct _xdbus_object_proxy_private
{
  xmutex_t lock;
  xhashtable_t *map_name_to_iface;
  xchar_t *object_path;
  xdbus_connection_t *connection;
};

enum
{
  PROP_0,
  PROP_G_OBJECT_PATH,
  PROP_G_CONNECTION
};

static void dbus_object_interface_init (xdbus_object_iface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xdbus_object_proxy, xdbus_object_proxy, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xdbus_object_proxy)
                         G_IMPLEMENT_INTERFACE (XTYPE_DBUS_OBJECT, dbus_object_interface_init))

static void
xdbus_object_proxy_finalize (xobject_t *object)
{
  xdbus_object_proxy_t *proxy = G_DBUS_OBJECT_PROXY (object);

  xhash_table_unref (proxy->priv->map_name_to_iface);

  g_clear_object (&proxy->priv->connection);

  g_free (proxy->priv->object_path);

  g_mutex_clear (&proxy->priv->lock);

  if (XOBJECT_CLASS (xdbus_object_proxy_parent_class)->finalize != NULL)
    XOBJECT_CLASS (xdbus_object_proxy_parent_class)->finalize (object);
}

static void
xdbus_object_proxy_get_property (xobject_t    *object,
                                  xuint_t       prop_id,
                                  xvalue_t     *value,
                                  xparam_spec_t *pspec)
{
  xdbus_object_proxy_t *proxy = G_DBUS_OBJECT_PROXY (object);

  switch (prop_id)
    {
    case PROP_G_OBJECT_PATH:
      g_mutex_lock (&proxy->priv->lock);
      xvalue_set_string (value, proxy->priv->object_path);
      g_mutex_unlock (&proxy->priv->lock);
      break;

    case PROP_G_CONNECTION:
      xvalue_set_object (value, xdbus_object_proxy_get_connection (proxy));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (proxy, prop_id, pspec);
      break;
    }
}

static void
xdbus_object_proxy_set_property (xobject_t       *object,
                                  xuint_t          prop_id,
                                  const xvalue_t  *value,
                                  xparam_spec_t    *pspec)
{
  xdbus_object_proxy_t *proxy = G_DBUS_OBJECT_PROXY (object);

  switch (prop_id)
    {
    case PROP_G_OBJECT_PATH:
      g_mutex_lock (&proxy->priv->lock);
      proxy->priv->object_path = xvalue_dup_string (value);
      g_mutex_unlock (&proxy->priv->lock);
      break;

    case PROP_G_CONNECTION:
      g_mutex_lock (&proxy->priv->lock);
      proxy->priv->connection = xvalue_dup_object (value);
      g_mutex_unlock (&proxy->priv->lock);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (proxy, prop_id, pspec);
      break;
    }
}

static void
xdbus_object_proxy_class_init (xdbus_object_proxy_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize     = xdbus_object_proxy_finalize;
  xobject_class->set_property = xdbus_object_proxy_set_property;
  xobject_class->get_property = xdbus_object_proxy_get_property;

  /**
   * xdbus_object_proxy_t:g-object-path:
   *
   * The object path of the proxy.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_G_OBJECT_PATH,
                                   xparam_spec_string ("g-object-path",
                                                        "Object Path",
                                                        "The object path of the proxy",
                                                        NULL,
                                                        XPARAM_READWRITE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xdbus_object_proxy_t:g-connection:
   *
   * The connection of the proxy.
   *
   * Since: 2.30
   */
  xobject_class_install_property (xobject_class,
                                   PROP_G_CONNECTION,
                                   xparam_spec_object ("g-connection",
                                                        "Connection",
                                                        "The connection of the proxy",
                                                        XTYPE_DBUS_CONNECTION,
                                                        XPARAM_READWRITE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_STRINGS));
}

static void
xdbus_object_proxy_init (xdbus_object_proxy_t *proxy)
{
  proxy->priv = xdbus_object_proxy_get_instance_private (proxy);
  g_mutex_init (&proxy->priv->lock);
  proxy->priv->map_name_to_iface = xhash_table_new_full (xstr_hash,
                                                          xstr_equal,
                                                          g_free,
                                                          (xdestroy_notify_t) xobject_unref);
}

static const xchar_t *
xdbus_object_proxy_get_object_path (xdbus_object_t *object)
{
  xdbus_object_proxy_t *proxy = G_DBUS_OBJECT_PROXY (object);
  const xchar_t *ret;
  g_mutex_lock (&proxy->priv->lock);
  ret = proxy->priv->object_path;
  g_mutex_unlock (&proxy->priv->lock);
  return ret;
}

/**
 * xdbus_object_proxy_get_connection:
 * @proxy: a #xdbus_object_proxy_t
 *
 * Gets the connection that @proxy is for.
 *
 * Returns: (transfer none): A #xdbus_connection_t. Do not free, the
 *   object is owned by @proxy.
 *
 * Since: 2.30
 */
xdbus_connection_t *
xdbus_object_proxy_get_connection (xdbus_object_proxy_t *proxy)
{
  xdbus_connection_t *ret;
  xreturn_val_if_fail (X_IS_DBUS_OBJECT_PROXY (proxy), NULL);
  g_mutex_lock (&proxy->priv->lock);
  ret = proxy->priv->connection;
  g_mutex_unlock (&proxy->priv->lock);
  return ret;
}

static xdbus_interface_t *
xdbus_object_proxy_get_interface (xdbus_object_t *object,
                                   const xchar_t *interface_name)
{
  xdbus_object_proxy_t *proxy = G_DBUS_OBJECT_PROXY (object);
  xdbus_proxy_t *ret;

  xreturn_val_if_fail (X_IS_DBUS_OBJECT_PROXY (proxy), NULL);
  xreturn_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);

  g_mutex_lock (&proxy->priv->lock);
  ret = xhash_table_lookup (proxy->priv->map_name_to_iface, interface_name);
  if (ret != NULL)
    xobject_ref (ret);
  g_mutex_unlock (&proxy->priv->lock);

  return (xdbus_interface_t *) ret; /* TODO: proper cast */
}

static xlist_t *
xdbus_object_proxy_get_interfaces (xdbus_object_t *object)
{
  xdbus_object_proxy_t *proxy = G_DBUS_OBJECT_PROXY (object);
  xlist_t *ret;

  xreturn_val_if_fail (X_IS_DBUS_OBJECT_PROXY (proxy), NULL);

  ret = NULL;

  g_mutex_lock (&proxy->priv->lock);
  ret = xhash_table_get_values (proxy->priv->map_name_to_iface);
  xlist_foreach (ret, (GFunc) xobject_ref, NULL);
  g_mutex_unlock (&proxy->priv->lock);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_object_proxy_new:
 * @connection: a #xdbus_connection_t
 * @object_path: the object path
 *
 * Creates a new #xdbus_object_proxy_t for the given connection and
 * object path.
 *
 * Returns: a new #xdbus_object_proxy_t
 *
 * Since: 2.30
 */
xdbus_object_proxy_t *
xdbus_object_proxy_new (xdbus_connection_t *connection,
                         const xchar_t     *object_path)
{
  xreturn_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), NULL);
  return G_DBUS_OBJECT_PROXY (xobject_new (XTYPE_DBUS_OBJECT_PROXY,
                                            "g-object-path", object_path,
                                            "g-connection", connection,
                                            NULL));
}

void
_xdbus_object_proxy_add_interface (xdbus_object_proxy_t *proxy,
                                    xdbus_proxy_t       *interface_proxy)
{
  const xchar_t *interface_name;
  xdbus_proxy_t *interface_proxy_to_remove;

  g_return_if_fail (X_IS_DBUS_OBJECT_PROXY (proxy));
  g_return_if_fail (X_IS_DBUS_PROXY (interface_proxy));

  g_mutex_lock (&proxy->priv->lock);

  interface_name = xdbus_proxy_get_interface_name (interface_proxy);
  interface_proxy_to_remove = xhash_table_lookup (proxy->priv->map_name_to_iface, interface_name);
  if (interface_proxy_to_remove != NULL)
    {
      xobject_ref (interface_proxy_to_remove);
      g_warn_if_fail (xhash_table_remove (proxy->priv->map_name_to_iface, interface_name));
    }
  xhash_table_insert (proxy->priv->map_name_to_iface,
                       xstrdup (interface_name),
                       xobject_ref (interface_proxy));
  xobject_ref (interface_proxy);

  g_mutex_unlock (&proxy->priv->lock);

  if (interface_proxy_to_remove != NULL)
    {
      xsignal_emit_by_name (proxy, "interface-removed", interface_proxy_to_remove);
      xobject_unref (interface_proxy_to_remove);
    }

  xsignal_emit_by_name (proxy, "interface-added", interface_proxy);
  xobject_unref (interface_proxy);
}

void
_xdbus_object_proxy_remove_interface (xdbus_object_proxy_t *proxy,
                                       const xchar_t      *interface_name)
{
  xdbus_proxy_t *interface_proxy;

  g_return_if_fail (X_IS_DBUS_OBJECT_PROXY (proxy));
  g_return_if_fail (g_dbus_is_interface_name (interface_name));

  g_mutex_lock (&proxy->priv->lock);

  interface_proxy = xhash_table_lookup (proxy->priv->map_name_to_iface, interface_name);
  if (interface_proxy != NULL)
    {
      xobject_ref (interface_proxy);
      g_warn_if_fail (xhash_table_remove (proxy->priv->map_name_to_iface, interface_name));
      g_mutex_unlock (&proxy->priv->lock);
      xsignal_emit_by_name (proxy, "interface-removed", interface_proxy);
      xobject_unref (interface_proxy);
    }
  else
    {
      g_mutex_unlock (&proxy->priv->lock);
    }
}

static void
dbus_object_interface_init (xdbus_object_iface_t *iface)
{
  iface->get_object_path       = xdbus_object_proxy_get_object_path;
  iface->get_interfaces        = xdbus_object_proxy_get_interfaces;
  iface->get_interface         = xdbus_object_proxy_get_interface;
}
