/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __G_TLS_CLIENT_CONNECTION_H__
#define __G_TLS_CLIENT_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gtlsconnection.h>

G_BEGIN_DECLS

#define XTYPE_TLS_CLIENT_CONNECTION                (g_tls_client_connection_get_type ())
#define G_TLS_CLIENT_CONNECTION(inst)               (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TLS_CLIENT_CONNECTION, GTlsClientConnection))
#define X_IS_TLS_CLIENT_CONNECTION(inst)            (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TLS_CLIENT_CONNECTION))
#define G_TLS_CLIENT_CONNECTION_GET_INTERFACE(inst) (XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_TLS_CLIENT_CONNECTION, GTlsClientConnectionInterface))

typedef struct _GTlsClientConnectionInterface GTlsClientConnectionInterface;

/**
 * GTlsClientConnectionInterface:
 * @x_iface: The parent interface.
 * @copy_session_state: Copies session state from one #GTlsClientConnection to another.
 *
 * vtable for a #GTlsClientConnection implementation.
 *
 * Since: 2.26
 */
struct _GTlsClientConnectionInterface
{
  xtype_interface_t x_iface;

  void     ( *copy_session_state )     (GTlsClientConnection       *conn,
                                        GTlsClientConnection       *source);
};

XPL_AVAILABLE_IN_ALL
xtype_t                 g_tls_client_connection_get_type             (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xio_stream_t *           g_tls_client_connection_new                  (xio_stream_t               *base_io_stream,
								    GSocketConnectable      *server_identity,
								    xerror_t                 **error);

XPL_DEPRECATED_IN_2_72
GTlsCertificateFlags  g_tls_client_connection_get_validation_flags (GTlsClientConnection    *conn);
XPL_DEPRECATED_IN_2_72
void                  g_tls_client_connection_set_validation_flags (GTlsClientConnection    *conn,
								    GTlsCertificateFlags     flags);
XPL_AVAILABLE_IN_ALL
GSocketConnectable   *g_tls_client_connection_get_server_identity  (GTlsClientConnection    *conn);
XPL_AVAILABLE_IN_ALL
void                  g_tls_client_connection_set_server_identity  (GTlsClientConnection    *conn,
								    GSocketConnectable      *identity);
XPL_DEPRECATED_IN_2_56
xboolean_t              g_tls_client_connection_get_use_ssl3         (GTlsClientConnection    *conn);
XPL_DEPRECATED_IN_2_56
void                  g_tls_client_connection_set_use_ssl3         (GTlsClientConnection    *conn,
								    xboolean_t                 use_ssl3);
XPL_AVAILABLE_IN_ALL
xlist_t *               g_tls_client_connection_get_accepted_cas     (GTlsClientConnection    *conn);

XPL_AVAILABLE_IN_2_46
void                  g_tls_client_connection_copy_session_state   (GTlsClientConnection    *conn,
                                                                    GTlsClientConnection    *source);

G_END_DECLS

#endif /* __G_TLS_CLIENT_CONNECTION_H__ */
