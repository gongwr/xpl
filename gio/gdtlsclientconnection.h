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

#ifndef __G_DTLS_CLIENT_CONNECTION_H__
#define __G_DTLS_CLIENT_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gdtlsconnection.h>

G_BEGIN_DECLS

#define XTYPE_DTLS_CLIENT_CONNECTION                (g_dtls_client_connection_get_type ())
#define G_DTLS_CLIENT_CONNECTION(inst)               (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_DTLS_CLIENT_CONNECTION, GDtlsClientConnection))
#define X_IS_DTLS_CLIENT_CONNECTION(inst)            (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_DTLS_CLIENT_CONNECTION))
#define G_DTLS_CLIENT_CONNECTION_GET_INTERFACE(inst) (XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_DTLS_CLIENT_CONNECTION, GDtlsClientConnectionInterface))

typedef struct _GDtlsClientConnectionInterface GDtlsClientConnectionInterface;

/**
 * GDtlsClientConnectionInterface:
 * @x_iface: The parent interface.
 *
 * vtable for a #GDtlsClientConnection implementation.
 *
 * Since: 2.48
 */
struct _GDtlsClientConnectionInterface
{
  xtype_interface_t x_iface;
};

XPL_AVAILABLE_IN_2_48
xtype_t                g_dtls_client_connection_get_type             (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_48
GDatagramBased      *g_dtls_client_connection_new                  (GDatagramBased         *base_socket,
                                                                    GSocketConnectable     *server_identity,
                                                                    xerror_t                **error);

XPL_AVAILABLE_IN_2_48
GTlsCertificateFlags g_dtls_client_connection_get_validation_flags (GDtlsClientConnection  *conn);
XPL_AVAILABLE_IN_2_48
void                 g_dtls_client_connection_set_validation_flags (GDtlsClientConnection  *conn,
                                                                    GTlsCertificateFlags    flags);
XPL_AVAILABLE_IN_2_48
GSocketConnectable  *g_dtls_client_connection_get_server_identity  (GDtlsClientConnection  *conn);
XPL_AVAILABLE_IN_2_48
void                 g_dtls_client_connection_set_server_identity  (GDtlsClientConnection  *conn,
                                                                    GSocketConnectable     *identity);
XPL_AVAILABLE_IN_2_48
xlist_t *              g_dtls_client_connection_get_accepted_cas     (GDtlsClientConnection  *conn);


G_END_DECLS

#endif /* __G_DTLS_CLIENT_CONNECTION_H__ */
