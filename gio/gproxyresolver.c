/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora, Ltd.
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

#include "gproxyresolver.h"

#include <glib.h>
#include "glibintl.h"

#include "gasyncresult.h"
#include "gcancellable.h"
#include "gtask.h"
#include "giomodule.h"
#include "gioerror.h"
#include "giomodule-priv.h"
#include "gnetworkingprivate.h"

/**
 * SECTION:gproxyresolver
 * @short_description: Asynchronous and cancellable network proxy resolver
 * @include: gio/gio.h
 *
 * #xproxy_resolver_t provides synchronous and asynchronous network proxy
 * resolution. #xproxy_resolver_t is used within #xsocket_client_t through
 * the method xsocket_connectable_proxy_enumerate().
 *
 * Implementations of #xproxy_resolver_t based on libproxy and GNOME settings can
 * be found in glib-networking. GIO comes with an implementation for use inside
 * Flatpak portals.
 */

/**
 * xproxy_resolver_interface_t:
 * @x_iface: The parent interface.
 * @is_supported: the virtual function pointer for xproxy_resolver_is_supported()
 * @lookup: the virtual function pointer for xproxy_resolver_lookup()
 * @lookup_async: the virtual function pointer for
 *  xproxy_resolver_lookup_async()
 * @lookup_finish: the virtual function pointer for
 *  xproxy_resolver_lookup_finish()
 *
 * The virtual function table for #xproxy_resolver_t.
 */

G_DEFINE_INTERFACE (xproxy_resolver, xproxy_resolver, XTYPE_OBJECT)

static void
xproxy_resolver_default_init (xproxy_resolver_interface_t *iface)
{
}

static xproxy_resolver_t *proxy_resolver_default_singleton = NULL;  /* (owned) (atomic) */

/**
 * xproxy_resolver_get_default:
 *
 * Gets the default #xproxy_resolver_t for the system.
 *
 * Returns: (not nullable) (transfer none): the default #xproxy_resolver_t, which
 *     will be a dummy object if no proxy resolver is available
 *
 * Since: 2.26
 */
xproxy_resolver_t *
xproxy_resolver_get_default (void)
{
  if (g_once_init_enter (&proxy_resolver_default_singleton))
    {
      xproxy_resolver_t *singleton;

      singleton = _xio_module_get_default (G_PROXY_RESOLVER_EXTENSION_POINT_NAME,
                                            "GIO_USE_PROXY_RESOLVER",
                                            (xio_module_verify_func_t) xproxy_resolver_is_supported);

      g_once_init_leave (&proxy_resolver_default_singleton, singleton);
    }

  return proxy_resolver_default_singleton;
}

/**
 * xproxy_resolver_is_supported:
 * @resolver: a #xproxy_resolver_t
 *
 * Checks if @resolver can be used on this system. (This is used
 * internally; xproxy_resolver_get_default() will only return a proxy
 * resolver that returns %TRUE for this method.)
 *
 * Returns: %TRUE if @resolver is supported.
 *
 * Since: 2.26
 */
xboolean_t
xproxy_resolver_is_supported (xproxy_resolver_t *resolver)
{
  xproxy_resolver_interface_t *iface;

  xreturn_val_if_fail (X_IS_PROXY_RESOLVER (resolver), FALSE);

  iface = G_PROXY_RESOLVER_GET_IFACE (resolver);

  return (* iface->is_supported) (resolver);
}

/**
 * xproxy_resolver_lookup:
 * @resolver: a #xproxy_resolver_t
 * @uri: a URI representing the destination to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks into the system proxy configuration to determine what proxy,
 * if any, to use to connect to @uri. The returned proxy URIs are of
 * the form `<protocol>://[user[:password]@]host:port` or
 * `direct://`, where <protocol> could be http, rtsp, socks
 * or other proxying protocol.
 *
 * If you don't know what network protocol is being used on the
 * socket, you should use `none` as the URI protocol.
 * In this case, the resolver might still return a generic proxy type
 * (such as SOCKS), but would not return protocol-specific proxy types
 * (such as http).
 *
 * `direct://` is used when no proxy is needed.
 * Direct connection should not be attempted unless it is part of the
 * returned array of proxies.
 *
 * Returns: (transfer full) (array zero-terminated=1): A
 *               NULL-terminated array of proxy URIs. Must be freed
 *               with xstrfreev().
 *
 * Since: 2.26
 */
xchar_t **
xproxy_resolver_lookup (xproxy_resolver_t  *resolver,
			 const xchar_t     *uri,
			 xcancellable_t    *cancellable,
			 xerror_t         **error)
{
  xproxy_resolver_interface_t *iface;

  xreturn_val_if_fail (X_IS_PROXY_RESOLVER (resolver), NULL);
  xreturn_val_if_fail (uri != NULL, NULL);

  if (!xuri_is_valid (uri, XURI_FLAGS_NONE, NULL))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                   "Invalid URI ???%s???", uri);
      return NULL;
    }

  iface = G_PROXY_RESOLVER_GET_IFACE (resolver);

  return (* iface->lookup) (resolver, uri, cancellable, error);
}

/**
 * xproxy_resolver_lookup_async:
 * @resolver: a #xproxy_resolver_t
 * @uri: a URI representing the destination to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): callback to call after resolution completes
 * @user_data: (closure): data for @callback
 *
 * Asynchronous lookup of proxy. See xproxy_resolver_lookup() for more
 * details.
 *
 * Since: 2.26
 */
void
xproxy_resolver_lookup_async (xproxy_resolver_t      *resolver,
			       const xchar_t         *uri,
			       xcancellable_t        *cancellable,
			       xasync_ready_callback_t  callback,
			       xpointer_t             user_data)
{
  xproxy_resolver_interface_t *iface;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_PROXY_RESOLVER (resolver));
  g_return_if_fail (uri != NULL);

  if (!xuri_is_valid (uri, XURI_FLAGS_NONE, NULL))
    {
      g_set_error (&error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                   "Invalid URI ???%s???", uri);
      xtask_report_error (resolver, callback, user_data,
                           xproxy_resolver_lookup_async,
                           g_steal_pointer (&error));
      return;
    }

  iface = G_PROXY_RESOLVER_GET_IFACE (resolver);

  (* iface->lookup_async) (resolver, uri, cancellable, callback, user_data);
}

/**
 * xproxy_resolver_lookup_finish:
 * @resolver: a #xproxy_resolver_t
 * @result: the result passed to your #xasync_ready_callback_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Call this function to obtain the array of proxy URIs when
 * xproxy_resolver_lookup_async() is complete. See
 * xproxy_resolver_lookup() for more details.
 *
 * Returns: (transfer full) (array zero-terminated=1): A
 *               NULL-terminated array of proxy URIs. Must be freed
 *               with xstrfreev().
 *
 * Since: 2.26
 */
xchar_t **
xproxy_resolver_lookup_finish (xproxy_resolver_t     *resolver,
				xasync_result_t       *result,
				xerror_t            **error)
{
  xproxy_resolver_interface_t *iface;

  xreturn_val_if_fail (X_IS_PROXY_RESOLVER (resolver), NULL);

  iface = G_PROXY_RESOLVER_GET_IFACE (resolver);

  return (* iface->lookup_finish) (resolver, result, error);
}
