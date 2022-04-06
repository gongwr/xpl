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
 * Authors: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#ifndef __G_PROXY_ADDRESS_H__
#define __G_PROXY_ADDRESS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/ginetsocketaddress.h>

G_BEGIN_DECLS

#define XTYPE_PROXY_ADDRESS         (xproxy_address_get_type ())
#define G_PROXY_ADDRESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_PROXY_ADDRESS, xproxy_address))
#define G_PROXY_ADDRESS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_PROXY_ADDRESS, GProxyAddressClass))
#define X_IS_PROXY_ADDRESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_PROXY_ADDRESS))
#define X_IS_PROXY_ADDRESS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_PROXY_ADDRESS))
#define G_PROXY_ADDRESS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_PROXY_ADDRESS, GProxyAddressClass))

typedef struct _GProxyAddressClass   GProxyAddressClass;
typedef struct _GProxyAddressPrivate GProxyAddressPrivate;

struct _GProxyAddress
{
  xinet_socket_address_t parent_instance;

  /*< private >*/
  GProxyAddressPrivate *priv;
};

struct _GProxyAddressClass
{
  GInetSocketAddressClass parent_class;
};


XPL_AVAILABLE_IN_ALL
xtype_t           xproxy_address_get_type    (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_address_t *xproxy_address_new         (xinet_address_t *inetaddr,
					     xuint16_t       port,
					     const xchar_t  *protocol,
					     const xchar_t  *dest_hostname,
		                             xuint16_t       dest_port,
					     const xchar_t  *username,
					     const xchar_t  *password);

XPL_AVAILABLE_IN_ALL
const xchar_t    *xproxy_address_get_protocol                (xproxy_address_t *proxy);
XPL_AVAILABLE_IN_2_34
const xchar_t    *xproxy_address_get_destination_protocol    (xproxy_address_t *proxy);
XPL_AVAILABLE_IN_ALL
const xchar_t    *xproxy_address_get_destination_hostname    (xproxy_address_t *proxy);
XPL_AVAILABLE_IN_ALL
xuint16_t         xproxy_address_get_destination_port        (xproxy_address_t *proxy);
XPL_AVAILABLE_IN_ALL
const xchar_t    *xproxy_address_get_username                (xproxy_address_t *proxy);
XPL_AVAILABLE_IN_ALL
const xchar_t    *xproxy_address_get_password                (xproxy_address_t *proxy);

XPL_AVAILABLE_IN_2_34
const xchar_t    *xproxy_address_get_uri                     (xproxy_address_t *proxy);

G_END_DECLS

#endif /* __G_PROXY_ADDRESS_H__ */
