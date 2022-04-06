/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#include "config.h"
#include "gsocketconnectable.h"
#include "glibintl.h"


/**
 * SECTION:gsocketconnectable
 * @short_description: Interface for potential socket endpoints
 * @include: gio/gio.h
 *
 * Objects that describe one or more potential socket endpoints
 * implement #xsocket_connectable_t. Callers can then use
 * xsocket_connectable_enumerate() to get a #xsocket_address_enumerator_t
 * to try out each socket address in turn until one succeeds, as shown
 * in the sample code below.
 *
 * |[<!-- language="C" -->
 * MyConnectionType *
 * connect_to_host (const char    *hostname,
 *                  xuint16_t        port,
 *                  xcancellable_t  *cancellable,
 *                  xerror_t       **error)
 * {
 *   MyConnection *conn = NULL;
 *   xsocket_connectable_t *addr;
 *   xsocket_address_enumerator_t *enumerator;
 *   xsocket_address_t *sockaddr;
 *   xerror_t *conn_error = NULL;
 *
 *   addr = g_network_address_new (hostname, port);
 *   enumerator = xsocket_connectable_enumerate (addr);
 *   xobject_unref (addr);
 *
 *   // Try each sockaddr until we succeed. Record the first connection error,
 *   // but not any further ones (since they'll probably be basically the same
 *   // as the first).
 *   while (!conn && (sockaddr = xsocket_address_enumerator_next (enumerator, cancellable, error))
 *     {
 *       conn = connect_to_sockaddr (sockaddr, conn_error ? NULL : &conn_error);
 *       xobject_unref (sockaddr);
 *     }
 *   xobject_unref (enumerator);
 *
 *   if (conn)
 *     {
 *       if (conn_error)
 *         {
 *           // We couldn't connect to the first address, but we succeeded
 *           // in connecting to a later address.
 *           xerror_free (conn_error);
 *         }
 *       return conn;
 *     }
 *   else if (error)
 *     {
 *       /// Either initial lookup failed, or else the caller cancelled us.
 *       if (conn_error)
 *         xerror_free (conn_error);
 *       return NULL;
 *     }
 *   else
 *     {
 *       xerror_propagate (error, conn_error);
 *       return NULL;
 *     }
 * }
 * ]|
 */


typedef xsocket_connectable_iface_t xsocket_connectable_interface_t;
G_DEFINE_INTERFACE (xsocket_connectable, xsocket_connectable, XTYPE_OBJECT)

static void
xsocket_connectable_default_init (xsocket_connectable_interface_t *iface)
{
}

/**
 * xsocket_connectable_enumerate:
 * @connectable: a #xsocket_connectable_t
 *
 * Creates a #xsocket_address_enumerator_t for @connectable.
 *
 * Returns: (transfer full): a new #xsocket_address_enumerator_t.
 *
 * Since: 2.22
 */
xsocket_address_enumerator_t *
xsocket_connectable_enumerate (xsocket_connectable_t *connectable)
{
  xsocket_connectable_iface_t *iface;

  g_return_val_if_fail (X_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = XSOCKET_CONNECTABLE_GET_IFACE (connectable);

  return (* iface->enumerate) (connectable);
}

/**
 * xsocket_connectable_proxy_enumerate:
 * @connectable: a #xsocket_connectable_t
 *
 * Creates a #xsocket_address_enumerator_t for @connectable that will
 * return a #xproxy_address_t for each of its addresses that you must connect
 * to via a proxy.
 *
 * If @connectable does not implement
 * xsocket_connectable_proxy_enumerate(), this will fall back to
 * calling xsocket_connectable_enumerate().
 *
 * Returns: (transfer full): a new #xsocket_address_enumerator_t.
 *
 * Since: 2.26
 */
xsocket_address_enumerator_t *
xsocket_connectable_proxy_enumerate (xsocket_connectable_t *connectable)
{
  xsocket_connectable_iface_t *iface;

  g_return_val_if_fail (X_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = XSOCKET_CONNECTABLE_GET_IFACE (connectable);

  if (iface->proxy_enumerate)
    return (* iface->proxy_enumerate) (connectable);
  else
    return (* iface->enumerate) (connectable);
}

/**
 * xsocket_connectable_to_string:
 * @connectable: a #xsocket_connectable_t
 *
 * Format a #xsocket_connectable_t as a string. This is a human-readable format for
 * use in debugging output, and is not a stable serialization format. It is not
 * suitable for use in user interfaces as it exposes too much information for a
 * user.
 *
 * If the #xsocket_connectable_t implementation does not support string formatting,
 * the implementation’s type name will be returned as a fallback.
 *
 * Returns: (transfer full): the formatted string
 *
 * Since: 2.48
 */
xchar_t *
xsocket_connectable_to_string (xsocket_connectable_t *connectable)
{
  xsocket_connectable_iface_t *iface;

  g_return_val_if_fail (X_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = XSOCKET_CONNECTABLE_GET_IFACE (connectable);

  if (iface->to_string != NULL)
    return iface->to_string (connectable);
  else
    return xstrdup (G_OBJECT_TYPE_NAME (connectable));
}
