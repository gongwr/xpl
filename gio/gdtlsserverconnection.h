/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2010 Red Hat, Inc.
 * Copyright © 2015 Collabora, Ltd.
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

#ifndef __G_DTLS_SERVER_CONNECTION_H__
#define __G_DTLS_SERVER_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gdtlsconnection.h>

G_BEGIN_DECLS

#define XTYPE_DTLS_SERVER_CONNECTION                (g_dtls_server_connection_get_type ())
#define G_DTLS_SERVER_CONNECTION(inst)               (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_DTLS_SERVER_CONNECTION, xdtls_server_connection))
#define X_IS_DTLS_SERVER_CONNECTION(inst)            (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_DTLS_SERVER_CONNECTION))
#define G_DTLS_SERVER_CONNECTION_GET_INTERFACE(inst) (XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_DTLS_SERVER_CONNECTION, GDtlsServerConnectionInterface))

/**
 * xdtls_server_connection_t:
 *
 * DTLS server-side connection. This is the server-side implementation
 * of a #xdtls_connection_t.
 *
 * Since: 2.48
 */
typedef struct _GDtlsServerConnectionInterface GDtlsServerConnectionInterface;

/**
 * GDtlsServerConnectionInterface:
 * @x_iface: The parent interface.
 *
 * vtable for a #xdtls_server_connection_t implementation.
 *
 * Since: 2.48
 */
struct _GDtlsServerConnectionInterface
{
  xtype_interface_t x_iface;
};

XPL_AVAILABLE_IN_2_48
xtype_t           g_dtls_server_connection_get_type (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_48
xdatagram_based_t *g_dtls_server_connection_new      (xdatagram_based_t   *base_socket,
                                                   xtls_certificate_t  *certificate,
                                                   xerror_t          **error);

G_END_DECLS

#endif /* __G_DTLS_SERVER_CONNECTION_H__ */
