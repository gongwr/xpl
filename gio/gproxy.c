/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora Ltd.
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
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"

#include "gproxy.h"

#include "giomodule.h"
#include "giomodule-priv.h"
#include "glibintl.h"

/**
 * SECTION:gproxy
 * @short_description: Interface for proxy handling
 * @include: gio/gio.h
 *
 * A #xproxy_t handles connecting to a remote host via a given type of
 * proxy server. It is implemented by the 'gio-proxy' extension point.
 * The extensions are named after their proxy protocol name. As an
 * example, a SOCKS5 proxy implementation can be retrieved with the
 * name 'socks5' using the function
 * g_io_extension_point_get_extension_by_name().
 *
 * Since: 2.26
 **/

G_DEFINE_INTERFACE (xproxy, xproxy, XTYPE_OBJECT)

static void
xproxy_default_init (xproxy_interface_t *iface)
{
}

/**
 * xproxy_get_default_for_protocol:
 * @protocol: the proxy protocol name (e.g. http, socks, etc)
 *
 * Find the `gio-proxy` extension point for a proxy implementation that supports
 * the specified protocol.
 *
 * Returns: (nullable) (transfer full): return a #xproxy_t or NULL if protocol
 *               is not supported.
 *
 * Since: 2.26
 **/
xproxy_t *
xproxy_get_default_for_protocol (const xchar_t *protocol)
{
  xio_extension_point_t *ep;
  xio_extension_t *extension;

  /* Ensure proxy modules loaded */
  _xio_modules_ensure_loaded ();

  ep = g_io_extension_point_lookup (G_PROXY_EXTENSION_POINT_NAME);

  extension = g_io_extension_point_get_extension_by_name (ep, protocol);

  if (extension)
      return xobject_new (g_io_extension_get_type (extension), NULL);

  return NULL;
}

/**
 * xproxy_connect:
 * @proxy: a #xproxy_t
 * @connection: a #xio_stream_t
 * @proxy_address: a #xproxy_address_t
 * @cancellable: (nullable): a #xcancellable_t
 * @error: return #xerror_t
 *
 * Given @connection to communicate with a proxy (eg, a
 * #xsocket_connection_t that is connected to the proxy server), this
 * does the necessary handshake to connect to @proxy_address, and if
 * required, wraps the #xio_stream_t to handle proxy payload.
 *
 * Returns: (transfer full): a #xio_stream_t that will replace @connection. This might
 *               be the same as @connection, in which case a reference
 *               will be added.
 *
 * Since: 2.26
 */
xio_stream_t *
xproxy_connect (xproxy_t            *proxy,
		 xio_stream_t         *connection,
		 xproxy_address_t     *proxy_address,
		 xcancellable_t      *cancellable,
		 xerror_t           **error)
{
  xproxy_interface_t *iface;

  g_return_val_if_fail (X_IS_PROXY (proxy), NULL);

  iface = G_PROXY_GET_IFACE (proxy);

  return (* iface->connect) (proxy,
			     connection,
			     proxy_address,
			     cancellable,
			     error);
}

/**
 * xproxy_connect_async:
 * @proxy: a #xproxy_t
 * @connection: a #xio_stream_t
 * @proxy_address: a #xproxy_address_t
 * @cancellable: (nullable): a #xcancellable_t
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): callback data
 *
 * Asynchronous version of xproxy_connect().
 *
 * Since: 2.26
 */
void
xproxy_connect_async (xproxy_t               *proxy,
		       xio_stream_t            *connection,
		       xproxy_address_t        *proxy_address,
		       xcancellable_t         *cancellable,
		       xasync_ready_callback_t   callback,
		       xpointer_t              user_data)
{
  xproxy_interface_t *iface;

  g_return_if_fail (X_IS_PROXY (proxy));

  iface = G_PROXY_GET_IFACE (proxy);

  (* iface->connect_async) (proxy,
			    connection,
			    proxy_address,
			    cancellable,
			    callback,
			    user_data);
}

/**
 * xproxy_connect_finish:
 * @proxy: a #xproxy_t
 * @result: a #xasync_result_t
 * @error: return #xerror_t
 *
 * See xproxy_connect().
 *
 * Returns: (transfer full): a #xio_stream_t.
 *
 * Since: 2.26
 */
xio_stream_t *
xproxy_connect_finish (xproxy_t       *proxy,
			xasync_result_t *result,
			xerror_t      **error)
{
  xproxy_interface_t *iface;

  g_return_val_if_fail (X_IS_PROXY (proxy), NULL);

  iface = G_PROXY_GET_IFACE (proxy);

  return (* iface->connect_finish) (proxy, result, error);
}

/**
 * xproxy_supports_hostname:
 * @proxy: a #xproxy_t
 *
 * Some proxy protocols expect to be passed a hostname, which they
 * will resolve to an IP address themselves. Others, like SOCKS4, do
 * not allow this. This function will return %FALSE if @proxy is
 * implementing such a protocol. When %FALSE is returned, the caller
 * should resolve the destination hostname first, and then pass a
 * #xproxy_address_t containing the stringified IP address to
 * xproxy_connect() or xproxy_connect_async().
 *
 * Returns: %TRUE if hostname resolution is supported.
 *
 * Since: 2.26
 */
xboolean_t
xproxy_supports_hostname (xproxy_t *proxy)
{
  xproxy_interface_t *iface;

  g_return_val_if_fail (X_IS_PROXY (proxy), FALSE);

  iface = G_PROXY_GET_IFACE (proxy);

  return (* iface->supports_hostname) (proxy);
}
