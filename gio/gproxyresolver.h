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

#ifndef __G_PROXY_RESOLVER_H__
#define __G_PROXY_RESOLVER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_PROXY_RESOLVER         (xproxy_resolver_get_type ())
#define G_PROXY_RESOLVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_PROXY_RESOLVER, xproxy_resolver))
#define X_IS_PROXY_RESOLVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_PROXY_RESOLVER))
#define G_PROXY_RESOLVER_GET_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_PROXY_RESOLVER, xproxy_resolver_interface_t))

/**
 * G_PROXY_RESOLVER_EXTENSION_POINT_NAME:
 *
 * Extension point for proxy resolving functionality.
 * See [Extending GIO][extending-gio].
 */
#define G_PROXY_RESOLVER_EXTENSION_POINT_NAME "gio-proxy-resolver"

typedef struct _xproxy_resolver_interface_t xproxy_resolver_interface_t;

struct _xproxy_resolver_interface_t {
  xtype_interface_t x_iface;

  /* Virtual Table */
  xboolean_t (* is_supported)  (xproxy_resolver_t       *resolver);

  xchar_t	** (* lookup)        (xproxy_resolver_t       *resolver,
			      const xchar_t          *uri,
			      xcancellable_t         *cancellable,
			      xerror_t              **error);

  void     (* lookup_async)  (xproxy_resolver_t       *resolver,
			      const xchar_t          *uri,
			      xcancellable_t         *cancellable,
			      xasync_ready_callback_t   callback,
			      xpointer_t              user_data);

  xchar_t	** (* lookup_finish) (xproxy_resolver_t       *resolver,
			      xasync_result_t         *result,
			      xerror_t              **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t		xproxy_resolver_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xproxy_resolver_t *xproxy_resolver_get_default    (void);

XPL_AVAILABLE_IN_ALL
xboolean_t        xproxy_resolver_is_supported   (xproxy_resolver_t       *resolver);
XPL_AVAILABLE_IN_ALL
xchar_t	      **xproxy_resolver_lookup		(xproxy_resolver_t       *resolver,
						 const xchar_t          *uri,
						 xcancellable_t         *cancellable,
						 xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void		xproxy_resolver_lookup_async   (xproxy_resolver_t       *resolver,
						 const xchar_t          *uri,
						 xcancellable_t         *cancellable,
						 xasync_ready_callback_t   callback,
						 xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xchar_t	      **xproxy_resolver_lookup_finish  (xproxy_resolver_t       *resolver,
						 xasync_result_t         *result,
						 xerror_t              **error);


G_END_DECLS

#endif /* __G_PROXY_RESOLVER_H__ */
