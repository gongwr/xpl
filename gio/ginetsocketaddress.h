/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 */

#ifndef __G_INET_SOCKET_ADDRESS_H__
#define __G_INET_SOCKET_ADDRESS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gsocketaddress.h>

G_BEGIN_DECLS

#define XTYPE_INET_SOCKET_ADDRESS         (g_inet_socket_address_get_type ())
#define G_INET_SOCKET_ADDRESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_INET_SOCKET_ADDRESS, xinet_socket_address))
#define G_INET_SOCKET_ADDRESS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_INET_SOCKET_ADDRESS, GInetSocketAddressClass))
#define X_IS_INET_SOCKET_ADDRESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_INET_SOCKET_ADDRESS))
#define X_IS_INET_SOCKET_ADDRESS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_INET_SOCKET_ADDRESS))
#define G_INET_SOCKET_ADDRESS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_INET_SOCKET_ADDRESS, GInetSocketAddressClass))

typedef struct _GInetSocketAddressClass   GInetSocketAddressClass;
typedef struct _GInetSocketAddressPrivate GInetSocketAddressPrivate;

struct _GInetSocketAddress
{
  xsocket_address_t parent_instance;

  /*< private >*/
  GInetSocketAddressPrivate *priv;
};

struct _GInetSocketAddressClass
{
  GSocketAddressClass parent_class;
};

XPL_AVAILABLE_IN_ALL
xtype_t           g_inet_socket_address_get_type        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_address_t *g_inet_socket_address_new             (xinet_address_t       *address,
                                                       xuint16_t             port);
XPL_AVAILABLE_IN_2_40
xsocket_address_t *g_inet_socket_address_new_from_string (const char         *address,
                                                       xuint_t               port);

XPL_AVAILABLE_IN_ALL
xinet_address_t *  g_inet_socket_address_get_address     (xinet_socket_address_t *address);
XPL_AVAILABLE_IN_ALL
xuint16_t         g_inet_socket_address_get_port        (xinet_socket_address_t *address);

XPL_AVAILABLE_IN_2_32
xuint32_t         g_inet_socket_address_get_flowinfo    (xinet_socket_address_t *address);
XPL_AVAILABLE_IN_2_32
xuint32_t         g_inet_socket_address_get_scope_id    (xinet_socket_address_t *address);

G_END_DECLS

#endif /* __G_INET_SOCKET_ADDRESS_H__ */
