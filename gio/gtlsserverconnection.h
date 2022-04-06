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

#ifndef __G_TLS_SERVER_CONNECTION_H__
#define __G_TLS_SERVER_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gtlsconnection.h>

G_BEGIN_DECLS

#define XTYPE_TLS_SERVER_CONNECTION                (xtls_server_connection_get_type ())
#define G_TLS_SERVER_CONNECTION(inst)               (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TLS_SERVER_CONNECTION, xtls_server_connection))
#define X_IS_TLS_SERVER_CONNECTION(inst)            (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TLS_SERVER_CONNECTION))
#define G_TLS_SERVER_CONNECTION_GET_INTERFACE(inst) (XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_TLS_SERVER_CONNECTION, xtls_server_connection_interface_t))

/**
 * xtls_server_connection_t:
 *
 * TLS server-side connection. This is the server-side implementation
 * of a #xtls_connection_t.
 *
 * Since: 2.28
 */
typedef struct _xtls_server_connection_interface xtls_server_connection_interface_t;

/**
 * xtls_server_connection_interface_t:
 * @x_iface: The parent interface.
 *
 * vtable for a #xtls_server_connection_t implementation.
 *
 * Since: 2.26
 */
struct _xtls_server_connection_interface
{
  xtype_interface_t x_iface;

};

XPL_AVAILABLE_IN_ALL
xtype_t                 xtls_server_connection_get_type                 (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xio_stream_t *           xtls_server_connection_new                      (xio_stream_t        *base_io_stream,
									xtls_certificate_t  *certificate,
									xerror_t          **error);

G_END_DECLS

#endif /* __G_TLS_SERVER_CONNECTION_H__ */
