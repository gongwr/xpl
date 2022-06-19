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

#define XTYPE_TLS_CLIENT_CONNECTION                (xtls_client_connection_get_type ())
#define G_TLS_CLIENT_CONNECTION(inst)               (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TLS_CLIENT_CONNECTION, xtls_client_connection))
#define X_IS_TLS_CLIENT_CONNECTION(inst)            (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TLS_CLIENT_CONNECTION))
#define G_TLS_CLIENT_CONNECTION_GET_INTERFACE(inst) (XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_TLS_CLIENT_CONNECTION, xtls_client_connection_interface_t))

typedef struct _xtls_client_connection_interface xtls_client_connection_interface_t;

/**
 * xtls_client_connection_interface_t:
 * @x_iface: The parent interface.
 * @copy_session_state: Copies session state from one #xtls_client_connection_t to another.
 *
 * vtable for a #xtls_client_connection_t implementation.
 *
 * Since: 2.26
 */
struct _xtls_client_connection_interface
{
  xtype_interface_t x_iface;

  void     ( *copy_session_state )     (xtls_client_connection_t       *conn,
                                        xtls_client_connection_t       *source);
};

XPL_AVAILABLE_IN_ALL
xtype_t                 xtls_client_connection_get_type             (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xio_stream_t *           xtls_client_connection_new                  (xio_stream_t               *base_io_stream,
								    xsocket_connectable_t      *server_identity,
								    xerror_t                 **error);

XPL_DEPRECATED_IN_2_72
xtls_certificate_flags_t  xtls_client_connection_get_validation_flags (xtls_client_connection_t    *conn);
XPL_DEPRECATED_IN_2_72
void                  xtls_client_connection_set_validation_flags (xtls_client_connection_t    *conn,
								    xtls_certificate_flags_t     flags);
XPL_AVAILABLE_IN_ALL
xsocket_connectable_t   *xtls_client_connection_get_server_identity  (xtls_client_connection_t    *conn);
XPL_AVAILABLE_IN_ALL
void                  xtls_client_connection_set_server_identity  (xtls_client_connection_t    *conn,
								    xsocket_connectable_t      *identity);
XPL_DEPRECATED_IN_2_56
xboolean_t              xtls_client_connection_get_use_ssl3         (xtls_client_connection_t    *conn);
XPL_DEPRECATED_IN_2_56
void                  xtls_client_connection_set_use_ssl3         (xtls_client_connection_t    *conn,
								    xboolean_t                 use_ssl3);
XPL_AVAILABLE_IN_ALL
xlist_t *               xtls_client_connection_get_accepted_cas     (xtls_client_connection_t    *conn);

XPL_AVAILABLE_IN_2_46
void                  xtls_client_connection_copy_session_state   (xtls_client_connection_t    *conn,
                                                                    xtls_client_connection_t    *source);

G_END_DECLS

#endif /* __G_TLS_CLIENT_CONNECTION_H__ */
