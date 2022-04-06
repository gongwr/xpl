/* GIO - GLib Input, Output and Streaming Library
 * Copyright © 2010 Collabora Ltd.
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
 *
 */

#ifndef __G_TCP_WRAPPER_CONNECTION_H__
#define __G_TCP_WRAPPER_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gtcpconnection.h>

G_BEGIN_DECLS

#define XTYPE_TCP_WRAPPER_CONNECTION            (g_tcp_wrapper_connection_get_type ())
#define G_TCP_WRAPPER_CONNECTION(inst)           (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                  XTYPE_TCP_WRAPPER_CONNECTION, xtcp_wrapper_connection))
#define G_TCP_WRAPPER_CONNECTION_CLASS(class)    (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                  XTYPE_TCP_WRAPPER_CONNECTION, GTcpWrapperConnectionClass))
#define X_IS_TCP_WRAPPER_CONNECTION(inst)        (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                  XTYPE_TCP_WRAPPER_CONNECTION))
#define X_IS_TCP_WRAPPER_CONNECTION_CLASS(class) (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                  XTYPE_TCP_WRAPPER_CONNECTION))
#define G_TCP_WRAPPER_CONNECTION_GET_CLASS(inst) (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                  XTYPE_TCP_WRAPPER_CONNECTION, GTcpWrapperConnectionClass))

typedef struct _GTcpWrapperConnectionPrivate GTcpWrapperConnectionPrivate;
typedef struct _GTcpWrapperConnectionClass   GTcpWrapperConnectionClass;

struct _GTcpWrapperConnectionClass
{
  GTcpConnectionClass parent_class;
};

struct _GTcpWrapperConnection
{
  xtcp_connection_t parent_instance;
  GTcpWrapperConnectionPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t              g_tcp_wrapper_connection_get_type (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_connection_t *g_tcp_wrapper_connection_new                (xio_stream_t             *base_io_stream,
								xsocket_t               *socket);
XPL_AVAILABLE_IN_ALL
xio_stream_t         *g_tcp_wrapper_connection_get_base_io_stream (xtcp_wrapper_connection_t *conn);

G_END_DECLS

#endif /* __G_TCP_WRAPPER_CONNECTION_H__ */
