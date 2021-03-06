/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#ifndef __G_PROXY_H__
#define __G_PROXY_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_PROXY		(xproxy_get_type ())
#define G_PROXY(o)		(XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_PROXY, xproxy))
#define X_IS_PROXY(o)		(XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_PROXY))
#define G_PROXY_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_PROXY, xproxy_interface_t))

/**
 * G_PROXY_EXTENSION_POINT_NAME:
 *
 * Extension point for proxy functionality.
 * See [Extending GIO][extending-gio].
 *
 * Since: 2.26
 */
#define G_PROXY_EXTENSION_POINT_NAME "gio-proxy"

/**
 * xproxy_t:
 *
 * Interface that handles proxy connection and payload.
 *
 * Since: 2.26
 */
typedef struct _xproxy_interface xproxy_interface_t;

/**
 * xproxy_interface_t:
 * @x_iface: The parent interface.
 * @connect: Connect to proxy server and wrap (if required) the #connection
 *           to handle payload.
 * @connect_async: Same as connect() but asynchronous.
 * @connect_finish: Returns the result of connect_async()
 * @supports_hostname: Returns whether the proxy supports hostname lookups.
 *
 * Provides an interface for handling proxy connection and payload.
 *
 * Since: 2.26
 */
struct _xproxy_interface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xio_stream_t * (* connect)           (xproxy_t               *proxy,
				     xio_stream_t            *connection,
				     xproxy_address_t        *proxy_address,
				     xcancellable_t         *cancellable,
				     xerror_t              **error);

  void        (* connect_async)     (xproxy_t               *proxy,
				     xio_stream_t            *connection,
				     xproxy_address_t	  *proxy_address,
				     xcancellable_t         *cancellable,
				     xasync_ready_callback_t   callback,
				     xpointer_t              user_data);

  xio_stream_t * (* connect_finish)    (xproxy_t               *proxy,
				     xasync_result_t         *result,
				     xerror_t              **error);

  xboolean_t    (* supports_hostname) (xproxy_t             *proxy);
};

XPL_AVAILABLE_IN_ALL
xtype_t      xproxy_get_type                 (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xproxy_t    *xproxy_get_default_for_protocol (const xchar_t *protocol);

XPL_AVAILABLE_IN_ALL
xio_stream_t *xproxy_connect           (xproxy_t               *proxy,
				      xio_stream_t            *connection,
				      xproxy_address_t        *proxy_address,
				      xcancellable_t         *cancellable,
				      xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void       xproxy_connect_async     (xproxy_t               *proxy,
				      xio_stream_t            *connection,
				      xproxy_address_t        *proxy_address,
				      xcancellable_t         *cancellable,
				      xasync_ready_callback_t   callback,
				      xpointer_t              user_data);

XPL_AVAILABLE_IN_ALL
xio_stream_t *xproxy_connect_finish    (xproxy_t               *proxy,
				      xasync_result_t         *result,
				      xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t   xproxy_supports_hostname (xproxy_t               *proxy);

G_END_DECLS

#endif /* __G_PROXY_H__ */
