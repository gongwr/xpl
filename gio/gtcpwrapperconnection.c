/* GIO - GLib Input, Output and Streaming Library
 *
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
 * Authors: Nicolas Dufresne <nicolas.dufresne@colllabora.co.uk>
 */

/**
 * SECTION:gtcpwrapperconnection
 * @title: xtcp_wrapper_connection_t
 * @short_description: Wrapper for non-xsocket_connection_t-based,
 *     xsocket_t-based GIOStreams
 * @include: gio/gio.h
 * @see_also: #xsocket_connection_t.
 *
 * A #xtcp_wrapper_connection_t can be used to wrap a #xio_stream_t that is
 * based on a #xsocket_t, but which is not actually a
 * #xsocket_connection_t. This is used by #xsocket_client_t so that it can
 * always return a #xsocket_connection_t, even when the connection it has
 * actually created is not directly a #xsocket_connection_t.
 *
 * Since: 2.28
 */

/**
 * xtcp_wrapper_connection_t:
 *
 * #xtcp_wrapper_connection_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

#include "config.h"

#include "gtcpwrapperconnection.h"

#include "gtcpconnection.h"
#include "glibintl.h"

struct _GTcpWrapperConnectionPrivate
{
  xio_stream_t *base_io_stream;
};

G_DEFINE_TYPE_WITH_PRIVATE (xtcp_wrapper_connection_t, g_tcp_wrapper_connection, XTYPE_TCP_CONNECTION)

enum
{
  PROP_NONE,
  PROP_BASE_IO_STREAM
};

static xinput_stream_t *
g_tcp_wrapper_connection_get_input_stream (xio_stream_t *io_stream)
{
  xtcp_wrapper_connection_t *connection = G_TCP_WRAPPER_CONNECTION (io_stream);

  return g_io_stream_get_input_stream (connection->priv->base_io_stream);
}

static xoutput_stream_t *
g_tcp_wrapper_connection_get_output_stream (xio_stream_t *io_stream)
{
  xtcp_wrapper_connection_t *connection = G_TCP_WRAPPER_CONNECTION (io_stream);

  return g_io_stream_get_output_stream (connection->priv->base_io_stream);
}

static void
g_tcp_wrapper_connection_get_property (xobject_t    *object,
				       xuint_t       prop_id,
				       xvalue_t     *value,
				       xparam_spec_t *pspec)
{
  xtcp_wrapper_connection_t *connection = G_TCP_WRAPPER_CONNECTION (object);

  switch (prop_id)
    {
     case PROP_BASE_IO_STREAM:
      xvalue_set_object (value, connection->priv->base_io_stream);
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tcp_wrapper_connection_set_property (xobject_t      *object,
                                        xuint_t         prop_id,
                                        const xvalue_t *value,
                                        xparam_spec_t   *pspec)
{
  xtcp_wrapper_connection_t *connection = G_TCP_WRAPPER_CONNECTION (object);

  switch (prop_id)
    {
     case PROP_BASE_IO_STREAM:
      connection->priv->base_io_stream = xvalue_dup_object (value);
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tcp_wrapper_connection_finalize (xobject_t *object)
{
  xtcp_wrapper_connection_t *connection = G_TCP_WRAPPER_CONNECTION (object);

  if (connection->priv->base_io_stream)
    xobject_unref (connection->priv->base_io_stream);

  G_OBJECT_CLASS (g_tcp_wrapper_connection_parent_class)->finalize (object);
}

static void
g_tcp_wrapper_connection_class_init (GTcpWrapperConnectionClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  xio_stream_class_t *stream_class = XIO_STREAM_CLASS (klass);

  gobject_class->set_property = g_tcp_wrapper_connection_set_property;
  gobject_class->get_property = g_tcp_wrapper_connection_get_property;
  gobject_class->finalize = g_tcp_wrapper_connection_finalize;

  stream_class->get_input_stream = g_tcp_wrapper_connection_get_input_stream;
  stream_class->get_output_stream = g_tcp_wrapper_connection_get_output_stream;

  xobject_class_install_property (gobject_class,
                                   PROP_BASE_IO_STREAM,
                                   g_param_spec_object ("base-io-stream",
			                                P_("Base IO Stream"),
			                                P_("The wrapped xio_stream_t"),
                                                        XTYPE_IO_STREAM,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
g_tcp_wrapper_connection_init (xtcp_wrapper_connection_t *connection)
{
  connection->priv = g_tcp_wrapper_connection_get_instance_private (connection);
}

/**
 * g_tcp_wrapper_connection_new:
 * @base_io_stream: the #xio_stream_t to wrap
 * @socket: the #xsocket_t associated with @base_io_stream
 *
 * Wraps @base_io_stream and @socket together as a #xsocket_connection_t.
 *
 * Returns: the new #xsocket_connection_t.
 *
 * Since: 2.28
 */
xsocket_connection_t *
g_tcp_wrapper_connection_new (xio_stream_t *base_io_stream,
			      xsocket_t   *socket)
{
  g_return_val_if_fail (X_IS_IO_STREAM (base_io_stream), NULL);
  g_return_val_if_fail (X_IS_SOCKET (socket), NULL);
  g_return_val_if_fail (xsocket_get_family (socket) == XSOCKET_FAMILY_IPV4 ||
			xsocket_get_family (socket) == XSOCKET_FAMILY_IPV6, NULL);
  g_return_val_if_fail (xsocket_get_socket_type (socket) == XSOCKET_TYPE_STREAM, NULL);

  return xobject_new (XTYPE_TCP_WRAPPER_CONNECTION,
		       "base-io-stream", base_io_stream,
		       "socket", socket,
		       NULL);
}

/**
 * g_tcp_wrapper_connection_get_base_io_stream:
 * @conn: a #xtcp_wrapper_connection_t
 *
 * Gets @conn's base #xio_stream_t
 *
 * Returns: (transfer none): @conn's base #xio_stream_t
 */
xio_stream_t *
g_tcp_wrapper_connection_get_base_io_stream (xtcp_wrapper_connection_t *conn)
{
  g_return_val_if_fail (X_IS_TCP_WRAPPER_CONNECTION (conn), NULL);

  return conn->priv->base_io_stream;
}
