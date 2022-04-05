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

#ifndef __G_NETWORK_ADDRESS_H__
#define __G_NETWORK_ADDRESS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_NETWORK_ADDRESS         (g_network_address_get_type ())
#define G_NETWORK_ADDRESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_NETWORK_ADDRESS, GNetworkAddress))
#define G_NETWORK_ADDRESS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_NETWORK_ADDRESS, GNetworkAddressClass))
#define X_IS_NETWORK_ADDRESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_NETWORK_ADDRESS))
#define X_IS_NETWORK_ADDRESS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_NETWORK_ADDRESS))
#define G_NETWORK_ADDRESS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_NETWORK_ADDRESS, GNetworkAddressClass))

typedef struct _GNetworkAddressClass   GNetworkAddressClass;
typedef struct _GNetworkAddressPrivate GNetworkAddressPrivate;

struct _GNetworkAddress
{
  xobject_t parent_instance;

  /*< private >*/
  GNetworkAddressPrivate *priv;
};

struct _GNetworkAddressClass
{
  xobject_class_t parent_class;

};

XPL_AVAILABLE_IN_ALL
xtype_t               g_network_address_get_type     (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GSocketConnectable *g_network_address_new          (const xchar_t      *hostname,
						    guint16           port);
XPL_AVAILABLE_IN_2_44
GSocketConnectable *g_network_address_new_loopback (guint16           port);
XPL_AVAILABLE_IN_ALL
GSocketConnectable *g_network_address_parse        (const xchar_t      *host_and_port,
						    guint16           default_port,
						    xerror_t          **error);
XPL_AVAILABLE_IN_ALL
GSocketConnectable *g_network_address_parse_uri    (const xchar_t      *uri,
    						    guint16           default_port,
						    xerror_t          **error);
XPL_AVAILABLE_IN_ALL
const xchar_t        *g_network_address_get_hostname (GNetworkAddress  *addr);
XPL_AVAILABLE_IN_ALL
guint16             g_network_address_get_port     (GNetworkAddress  *addr);
XPL_AVAILABLE_IN_ALL
const xchar_t        *g_network_address_get_scheme   (GNetworkAddress  *addr);


G_END_DECLS

#endif /* __G_NETWORK_ADDRESS_H__ */
