/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008, 2009 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

/**
 * SECTION:gtcpconnection
 * @title: xtcp_connection_t
 * @short_description: A TCP xsocket_connection_t
 * @include: gio/gio.h
 * @see_also: #xsocket_connection_t.
 *
 * This is the subclass of #xsocket_connection_t that is created
 * for TCP/IP sockets.
 *
 * Since: 2.22
 */

#include "config.h"
#include "gtcpconnection.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "giostream.h"
#include "glibintl.h"

struct _GTcpConnectionPrivate
{
  xuint_t graceful_disconnect : 1;
};

G_DEFINE_TYPE_WITH_CODE (xtcp_connection, g_tcp_connection,
			 XTYPE_SOCKET_CONNECTION,
                         G_ADD_PRIVATE (xtcp_connection_t)
  xsocket_connection_factory_register_type (g_define_type_id,
					     XSOCKET_FAMILY_IPV4,
					     XSOCKET_TYPE_STREAM,
					     XSOCKET_PROTOCOL_DEFAULT);
  xsocket_connection_factory_register_type (g_define_type_id,
					     XSOCKET_FAMILY_IPV6,
					     XSOCKET_TYPE_STREAM,
					     XSOCKET_PROTOCOL_DEFAULT);
  xsocket_connection_factory_register_type (g_define_type_id,
					     XSOCKET_FAMILY_IPV4,
					     XSOCKET_TYPE_STREAM,
					     XSOCKET_PROTOCOL_TCP);
  xsocket_connection_factory_register_type (g_define_type_id,
					     XSOCKET_FAMILY_IPV6,
					     XSOCKET_TYPE_STREAM,
					     XSOCKET_PROTOCOL_TCP);
			 );

static xboolean_t g_tcp_connection_close       (xio_stream_t            *stream,
					      xcancellable_t         *cancellable,
					      xerror_t              **error);
static void     g_tcp_connection_close_async (xio_stream_t            *stream,
					      int                   io_priority,
					      xcancellable_t         *cancellable,
					      xasync_ready_callback_t   callback,
					      xpointer_t              user_data);


enum
{
  PROP_0,
  PROP_GRACEFUL_DISCONNECT
};

static void
g_tcp_connection_init (xtcp_connection_t *connection)
{
  connection->priv = g_tcp_connection_get_instance_private (connection);
  connection->priv->graceful_disconnect = FALSE;
}

static void
g_tcp_connection_get_property (xobject_t    *object,
			       xuint_t       prop_id,
			       xvalue_t     *value,
			       xparam_spec_t *pspec)
{
  xtcp_connection_t *connection = G_TCP_CONNECTION (object);

  switch (prop_id)
    {
      case PROP_GRACEFUL_DISCONNECT:
	xvalue_set_boolean (value, connection->priv->graceful_disconnect);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tcp_connection_set_property (xobject_t      *object,
			       xuint_t         prop_id,
			       const xvalue_t *value,
			       xparam_spec_t   *pspec)
{
  xtcp_connection_t *connection = G_TCP_CONNECTION (object);

  switch (prop_id)
    {
      case PROP_GRACEFUL_DISCONNECT:
	g_tcp_connection_set_graceful_disconnect (connection,
						  xvalue_get_boolean (value));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tcp_connection_class_init (GTcpConnectionClass *class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (class);
  xio_stream_class_t *stream_class = XIO_STREAM_CLASS (class);

  gobject_class->set_property = g_tcp_connection_set_property;
  gobject_class->get_property = g_tcp_connection_get_property;

  stream_class->close_fn = g_tcp_connection_close;
  stream_class->close_async = g_tcp_connection_close_async;

  xobject_class_install_property (gobject_class, PROP_GRACEFUL_DISCONNECT,
				   g_param_spec_boolean ("graceful-disconnect",
							 P_("Graceful Disconnect"),
							 P_("Whether or not close does a graceful disconnect"),
							 FALSE,
							 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static xboolean_t
g_tcp_connection_close (xio_stream_t     *stream,
			xcancellable_t  *cancellable,
			xerror_t       **error)
{
  xtcp_connection_t *connection = G_TCP_CONNECTION (stream);
  xsocket_t *socket;
  char buffer[1024];
  xssize_t ret;
  xboolean_t had_error;

  socket = xsocket_connection_get_socket (XSOCKET_CONNECTION (stream));
  had_error = FALSE;

  if (connection->priv->graceful_disconnect &&
      !xcancellable_is_cancelled (cancellable) /* Cancelled -> close fast */)
    {
      if (!xsocket_shutdown (socket, FALSE, TRUE, error))
	{
	  error = NULL; /* Ignore further errors */
	  had_error = TRUE;
	}
      else
	{
	  while (TRUE)
	    {
	      ret = xsocket_receive_with_blocking (socket,  buffer, sizeof (buffer),
						    TRUE, cancellable, error);
	      if (ret < 0)
		{
		  had_error = TRUE;
		  error = NULL;
		  break;
		}
	      if (ret == 0)
		break;
	    }
	}
    }

  return XIO_STREAM_CLASS (g_tcp_connection_parent_class)
    ->close_fn (stream, cancellable, error) && !had_error;
}

/* consumes @error */
static void
async_close_finish (xtask_t    *task,
                    xerror_t   *error)
{
  xio_stream_class_t *parent = XIO_STREAM_CLASS (g_tcp_connection_parent_class);
  xio_stream_t *stream = xtask_get_source_object (task);
  xcancellable_t *cancellable = xtask_get_cancellable (task);

  /* Close underlying stream, ignoring further errors if we already
   * have one.
   */
  if (error)
    parent->close_fn (stream, cancellable, NULL);
  else
    parent->close_fn (stream, cancellable, &error);

  if (error)
    xtask_return_error (task, error);
  else
    xtask_return_boolean (task, TRUE);
}


static xboolean_t
close_read_ready (xsocket_t        *socket,
		  xio_condition_t    condition,
		  xtask_t          *task)
{
  xerror_t *error = NULL;
  char buffer[1024];
  xssize_t ret;

  ret = xsocket_receive_with_blocking (socket,  buffer, sizeof (buffer),
                                        FALSE, xtask_get_cancellable (task),
                                        &error);
  if (ret < 0)
    {
      if (xerror_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
	{
	  xerror_free (error);
	  return TRUE;
	}
      else
	{
	  async_close_finish (task, error);
	  xobject_unref (task);
	  return FALSE;
	}
    }

  if (ret == 0)
    {
      async_close_finish (task, NULL);
      return FALSE;
    }

  return TRUE;
}


static void
g_tcp_connection_close_async (xio_stream_t           *stream,
			      int                  io_priority,
			      xcancellable_t        *cancellable,
			      xasync_ready_callback_t  callback,
			      xpointer_t             user_data)
{
  xtcp_connection_t *connection = G_TCP_CONNECTION (stream);
  xsocket_t *socket;
  xsource_t *source;
  xerror_t *error;
  xtask_t *task;

  if (connection->priv->graceful_disconnect &&
      !xcancellable_is_cancelled (cancellable) /* Cancelled -> close fast */)
    {
      task = xtask_new (stream, cancellable, callback, user_data);
      xtask_set_source_tag (task, g_tcp_connection_close_async);
      xtask_set_priority (task, io_priority);

      socket = xsocket_connection_get_socket (XSOCKET_CONNECTION (stream));

      error = NULL;
      if (!xsocket_shutdown (socket, FALSE, TRUE, &error))
	{
	  xtask_return_error (task, error);
	  xobject_unref (task);
	  return;
	}

      source = xsocket_create_source (socket, G_IO_IN, cancellable);
      xtask_attach_source (task, source, (xsource_func_t) close_read_ready);
      xsource_unref (source);

      return;
    }

  XIO_STREAM_CLASS (g_tcp_connection_parent_class)
    ->close_async (stream, io_priority, cancellable, callback, user_data);
}

/**
 * g_tcp_connection_set_graceful_disconnect:
 * @connection: a #xtcp_connection_t
 * @graceful_disconnect: Whether to do graceful disconnects or not
 *
 * This enables graceful disconnects on close. A graceful disconnect
 * means that we signal the receiving end that the connection is terminated
 * and wait for it to close the connection before closing the connection.
 *
 * A graceful disconnect means that we can be sure that we successfully sent
 * all the outstanding data to the other end, or get an error reported.
 * However, it also means we have to wait for all the data to reach the
 * other side and for it to acknowledge this by closing the socket, which may
 * take a while. For this reason it is disabled by default.
 *
 * Since: 2.22
 */
void
g_tcp_connection_set_graceful_disconnect (xtcp_connection_t *connection,
					  xboolean_t        graceful_disconnect)
{
  graceful_disconnect = !!graceful_disconnect;
  if (graceful_disconnect != connection->priv->graceful_disconnect)
    {
      connection->priv->graceful_disconnect = graceful_disconnect;
      xobject_notify (G_OBJECT (connection), "graceful-disconnect");
    }
}

/**
 * g_tcp_connection_get_graceful_disconnect:
 * @connection: a #xtcp_connection_t
 *
 * Checks if graceful disconnects are used. See
 * g_tcp_connection_set_graceful_disconnect().
 *
 * Returns: %TRUE if graceful disconnect is used on close, %FALSE otherwise
 *
 * Since: 2.22
 */
xboolean_t
g_tcp_connection_get_graceful_disconnect (xtcp_connection_t *connection)
{
  return connection->priv->graceful_disconnect;
}
