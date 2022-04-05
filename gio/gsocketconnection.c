/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 Christian Kellner, Samuel Cormier-Iijima
 *           © 2008 codethink
 * Copyright © 2009 Red Hat, Inc
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
 *          Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include "gsocketconnection.h"

#include "gsocketoutputstream.h"
#include "gsocketinputstream.h"
#include "gioprivate.h"
#include <gio/giostream.h>
#include <gio/gtask.h>
#include "gunixconnection.h"
#include "gtcpconnection.h"
#include "glibintl.h"


/**
 * SECTION:gsocketconnection
 * @short_description: A socket connection
 * @include: gio/gio.h
 * @see_also: #xio_stream_t, #GSocketClient, #GSocketListener
 *
 * #xsocket_connection_t is a #xio_stream_t for a connected socket. They
 * can be created either by #GSocketClient when connecting to a host,
 * or by #GSocketListener when accepting a new client.
 *
 * The type of the #xsocket_connection_t object returned from these calls
 * depends on the type of the underlying socket that is in use. For
 * instance, for a TCP/IP connection it will be a #GTcpConnection.
 *
 * Choosing what type of object to construct is done with the socket
 * connection factory, and it is possible for 3rd parties to register
 * custom socket connection types for specific combination of socket
 * family/type/protocol using xsocket_connection_factory_register_type().
 *
 * To close a #xsocket_connection_t, use g_io_stream_close(). Closing both
 * substreams of the #xio_stream_t separately will not close the underlying
 * #xsocket_t.
 *
 * Since: 2.22
 */

enum
{
  PROP_NONE,
  PROP_SOCKET,
};

struct _xsocket_connection_private_t
{
  xsocket_t       *socket;
  xinput_stream_t  *input_stream;
  xoutput_stream_t *output_stream;

  xsocket_address_t *cached_remote_address;

  xboolean_t       in_dispose;
};

static xboolean_t xsocket_connection_close         (xio_stream_t            *stream,
						   xcancellable_t         *cancellable,
						   xerror_t              **error);
static void     xsocket_connection_close_async   (xio_stream_t            *stream,
						   int                   io_priority,
						   xcancellable_t         *cancellable,
						   xasync_ready_callback_t   callback,
						   xpointer_t              user_data);
static xboolean_t xsocket_connection_close_finish  (xio_stream_t            *stream,
						   xasync_result_t         *result,
						   xerror_t              **error);

G_DEFINE_TYPE_WITH_PRIVATE (xsocket_connection_t, xsocket_connection, XTYPE_IO_STREAM)

static xinput_stream_t *
xsocket_connection_get_input_stream (xio_stream_t *io_stream)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (io_stream);

  if (connection->priv->input_stream == NULL)
    connection->priv->input_stream = (xinput_stream_t *)
      _xsocket_input_stream_new (connection->priv->socket);

  return connection->priv->input_stream;
}

static xoutput_stream_t *
xsocket_connection_get_output_stream (xio_stream_t *io_stream)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (io_stream);

  if (connection->priv->output_stream == NULL)
    connection->priv->output_stream = (xoutput_stream_t *)
      _xsocket_output_stream_new (connection->priv->socket);

  return connection->priv->output_stream;
}

/**
 * xsocket_connection_is_connected:
 * @connection: a #xsocket_connection_t
 *
 * Checks if @connection is connected. This is equivalent to calling
 * xsocket_is_connected() on @connection's underlying #xsocket_t.
 *
 * Returns: whether @connection is connected
 *
 * Since: 2.32
 */
xboolean_t
xsocket_connection_is_connected (xsocket_connection_t  *connection)
{
  return xsocket_is_connected (connection->priv->socket);
}

/**
 * xsocket_connection_connect:
 * @connection: a #xsocket_connection_t
 * @address: a #xsocket_address_t specifying the remote address.
 * @cancellable: (nullable): a %xcancellable_t or %NULL
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Connect @connection to the specified remote address.
 *
 * Returns: %TRUE if the connection succeeded, %FALSE on error
 *
 * Since: 2.32
 */
xboolean_t
xsocket_connection_connect (xsocket_connection_t  *connection,
			     xsocket_address_t     *address,
			     xcancellable_t       *cancellable,
			     xerror_t            **error)
{
  g_return_val_if_fail (X_IS_SOCKET_CONNECTION (connection), FALSE);
  g_return_val_if_fail (X_IS_SOCKET_ADDRESS (address), FALSE);

  return xsocket_connect (connection->priv->socket, address,
			   cancellable, error);
}

static xboolean_t xsocket_connection_connect_callback (xsocket_t      *socket,
						      GIOCondition  condition,
						      xpointer_t      user_data);

/**
 * xsocket_connection_connect_async:
 * @connection: a #xsocket_connection_t
 * @address: a #xsocket_address_t specifying the remote address.
 * @cancellable: (nullable): a %xcancellable_t or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): user data for the callback
 *
 * Asynchronously connect @connection to the specified remote address.
 *
 * This clears the #xsocket_t:blocking flag on @connection's underlying
 * socket if it is currently set.
 *
 * Use xsocket_connection_connect_finish() to retrieve the result.
 *
 * Since: 2.32
 */
void
xsocket_connection_connect_async (xsocket_connection_t   *connection,
				   xsocket_address_t      *address,
				   xcancellable_t        *cancellable,
				   xasync_ready_callback_t  callback,
				   xpointer_t             user_data)
{
  GTask *task;
  xerror_t *tmp_error = NULL;

  g_return_if_fail (X_IS_SOCKET_CONNECTION (connection));
  g_return_if_fail (X_IS_SOCKET_ADDRESS (address));

  task = g_task_new (connection, cancellable, callback, user_data);
  g_task_set_source_tag (task, xsocket_connection_connect_async);

  xsocket_set_blocking (connection->priv->socket, FALSE);

  if (xsocket_connect (connection->priv->socket, address,
			cancellable, &tmp_error))
    {
      g_task_return_boolean (task, TRUE);
      g_object_unref (task);
    }
  else if (g_error_matches (tmp_error, G_IO_ERROR, G_IO_ERROR_PENDING))
    {
      GSource *source;

      g_error_free (tmp_error);
      source = xsocket_create_source (connection->priv->socket,
				       G_IO_OUT, cancellable);
      g_task_attach_source (task, source,
			    (GSourceFunc) xsocket_connection_connect_callback);
      g_source_unref (source);
    }
  else
    {
      g_task_return_error (task, tmp_error);
      g_object_unref (task);
    }
}

static xboolean_t
xsocket_connection_connect_callback (xsocket_t      *socket,
				      GIOCondition  condition,
				      xpointer_t      user_data)
{
  GTask *task = user_data;
  xsocket_connection_t *connection = g_task_get_source_object (task);
  xerror_t *error = NULL;

  if (xsocket_check_connect_result (connection->priv->socket, &error))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, error);

  g_object_unref (task);
  return FALSE;
}

/**
 * xsocket_connection_connect_finish:
 * @connection: a #xsocket_connection_t
 * @result: the #xasync_result_t
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Gets the result of a xsocket_connection_connect_async() call.
 *
 * Returns: %TRUE if the connection succeeded, %FALSE on error
 *
 * Since: 2.32
 */
xboolean_t
xsocket_connection_connect_finish (xsocket_connection_t  *connection,
				    xasync_result_t       *result,
				    xerror_t            **error)
{
  g_return_val_if_fail (X_IS_SOCKET_CONNECTION (connection), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, connection), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * xsocket_connection_get_socket:
 * @connection: a #xsocket_connection_t
 *
 * Gets the underlying #xsocket_t object of the connection.
 * This can be useful if you want to do something unusual on it
 * not supported by the #xsocket_connection_t APIs.
 *
 * Returns: (transfer none): a #xsocket_t or %NULL on error.
 *
 * Since: 2.22
 */
xsocket_t *
xsocket_connection_get_socket (xsocket_connection_t *connection)
{
  g_return_val_if_fail (X_IS_SOCKET_CONNECTION (connection), NULL);

  return connection->priv->socket;
}

/**
 * xsocket_connection_get_local_address:
 * @connection: a #xsocket_connection_t
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Try to get the local address of a socket connection.
 *
 * Returns: (transfer full): a #xsocket_address_t or %NULL on error.
 *     Free the returned object with g_object_unref().
 *
 * Since: 2.22
 */
xsocket_address_t *
xsocket_connection_get_local_address (xsocket_connection_t  *connection,
				       xerror_t            **error)
{
  return xsocket_get_local_address (connection->priv->socket, error);
}

/**
 * xsocket_connection_get_remote_address:
 * @connection: a #xsocket_connection_t
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Try to get the remote address of a socket connection.
 *
 * Since GLib 2.40, when used with xsocket_client_connect() or
 * xsocket_client_connect_async(), during emission of
 * %XSOCKET_CLIENT_CONNECTING, this function will return the remote
 * address that will be used for the connection.  This allows
 * applications to print e.g. "Connecting to example.com
 * (10.42.77.3)...".
 *
 * Returns: (transfer full): a #xsocket_address_t or %NULL on error.
 *     Free the returned object with g_object_unref().
 *
 * Since: 2.22
 */
xsocket_address_t *
xsocket_connection_get_remote_address (xsocket_connection_t  *connection,
					xerror_t            **error)
{
  if (!xsocket_is_connected (connection->priv->socket))
    {
      return connection->priv->cached_remote_address ?
        g_object_ref (connection->priv->cached_remote_address) : NULL;
    }
  return xsocket_get_remote_address (connection->priv->socket, error);
}

/* Private API allowing applications to retrieve the resolved address
 * now, before we start connecting.
 *
 * https://bugzilla.gnome.org/show_bug.cgi?id=712547
 */
void
xsocket_connection_set_cached_remote_address (xsocket_connection_t *connection,
                                               xsocket_address_t    *address)
{
  g_clear_object (&connection->priv->cached_remote_address);
  connection->priv->cached_remote_address = address ? g_object_ref (address) : NULL;
}

static void
xsocket_connection_get_property (xobject_t    *object,
                                  xuint_t       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (object);

  switch (prop_id)
    {
     case PROP_SOCKET:
      g_value_set_object (value, connection->priv->socket);
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsocket_connection_set_property (xobject_t      *object,
                                  xuint_t         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (object);

  switch (prop_id)
    {
     case PROP_SOCKET:
      connection->priv->socket = G_SOCKET (g_value_dup_object (value));
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsocket_connection_constructed (xobject_t *object)
{
#ifndef G_DISABLE_ASSERT
  xsocket_connection_t *connection = XSOCKET_CONNECTION (object);
#endif

  g_assert (connection->priv->socket != NULL);
}

static void
xsocket_connection_dispose (xobject_t *object)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (object);

  connection->priv->in_dispose = TRUE;

  g_clear_object (&connection->priv->cached_remote_address);

  G_OBJECT_CLASS (xsocket_connection_parent_class)
    ->dispose (object);

  connection->priv->in_dispose = FALSE;
}

static void
xsocket_connection_finalize (xobject_t *object)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (object);

  if (connection->priv->input_stream)
    g_object_unref (connection->priv->input_stream);

  if (connection->priv->output_stream)
    g_object_unref (connection->priv->output_stream);

  g_object_unref (connection->priv->socket);

  G_OBJECT_CLASS (xsocket_connection_parent_class)
    ->finalize (object);
}

static void
xsocket_connection_class_init (xsocket_connection_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  xio_stream_class_t *stream_class = XIO_STREAM_CLASS (klass);

  gobject_class->set_property = xsocket_connection_set_property;
  gobject_class->get_property = xsocket_connection_get_property;
  gobject_class->constructed = xsocket_connection_constructed;
  gobject_class->finalize = xsocket_connection_finalize;
  gobject_class->dispose = xsocket_connection_dispose;

  stream_class->get_input_stream = xsocket_connection_get_input_stream;
  stream_class->get_output_stream = xsocket_connection_get_output_stream;
  stream_class->close_fn = xsocket_connection_close;
  stream_class->close_async = xsocket_connection_close_async;
  stream_class->close_finish = xsocket_connection_close_finish;

  g_object_class_install_property (gobject_class,
                                   PROP_SOCKET,
                                   g_param_spec_object ("socket",
			                                P_("Socket"),
			                                P_("The underlying xsocket_t"),
                                                        XTYPE_SOCKET,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
xsocket_connection_init (xsocket_connection_t *connection)
{
  connection->priv = xsocket_connection_get_instance_private (connection);
}

static xboolean_t
xsocket_connection_close (xio_stream_t     *stream,
			   xcancellable_t  *cancellable,
			   xerror_t       **error)
{
  xsocket_connection_t *connection = XSOCKET_CONNECTION (stream);

  if (connection->priv->output_stream)
    g_output_stream_close (connection->priv->output_stream,
			   cancellable, NULL);
  if (connection->priv->input_stream)
    g_input_stream_close (connection->priv->input_stream,
			  cancellable, NULL);

  /* Don't close the underlying socket if this is being called
   * as part of dispose(); when destroying the xsocket_connection_t,
   * we only want to close the socket if we're holding the last
   * reference on it, and in that case it will close itself when
   * we unref it in finalize().
   */
  if (connection->priv->in_dispose)
    return TRUE;

  return xsocket_close (connection->priv->socket, error);
}


static void
xsocket_connection_close_async (xio_stream_t           *stream,
				 int                  io_priority,
				 xcancellable_t        *cancellable,
				 xasync_ready_callback_t  callback,
				 xpointer_t             user_data)
{
  GTask *task;
  xio_stream_class_t *class;
  xerror_t *error;

  class = XIO_STREAM_GET_CLASS (stream);

  task = g_task_new (stream, cancellable, callback, user_data);
  g_task_set_source_tag (task, xsocket_connection_close_async);

  /* socket close is not blocked, just do it! */
  error = NULL;
  if (class->close_fn &&
      !class->close_fn (stream, cancellable, &error))
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

static xboolean_t
xsocket_connection_close_finish (xio_stream_t     *stream,
				  xasync_result_t  *result,
				  xerror_t       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

typedef struct {
  xsocket_family_t socket_family;
  xsocket_type_t socket_type;
  int protocol;
  xtype_t implementation;
} ConnectionFactory;

static xuint_t
connection_factory_hash (gconstpointer key)
{
  const ConnectionFactory *factory = key;
  xuint_t h;

  h = factory->socket_family ^ (factory->socket_type << 4) ^ (factory->protocol << 8);
  /* This is likely to be small, so spread over whole
     hash space to get some distribution */
  h = h ^ (h << 8) ^ (h << 16) ^ (h << 24);

  return h;
}

static xboolean_t
connection_factory_equal (gconstpointer _a,
			  gconstpointer _b)
{
  const ConnectionFactory *a = _a;
  const ConnectionFactory *b = _b;

  if (a->socket_family != b->socket_family)
    return FALSE;

  if (a->socket_type != b->socket_type)
    return FALSE;

  if (a->protocol != b->protocol)
    return FALSE;

  return TRUE;
}

static GHashTable *connection_factories = NULL;
G_LOCK_DEFINE_STATIC(connection_factories);

/**
 * xsocket_connection_factory_register_type:
 * @g_type: a #xtype_t, inheriting from %XTYPE_SOCKET_CONNECTION
 * @family: a #xsocket_family_t
 * @type: a #xsocket_type_t
 * @protocol: a protocol id
 *
 * Looks up the #xtype_t to be used when creating socket connections on
 * sockets with the specified @family, @type and @protocol.
 *
 * If no type is registered, the #xsocket_connection_t base type is returned.
 *
 * Since: 2.22
 */
void
xsocket_connection_factory_register_type (xtype_t         g_type,
					   xsocket_family_t family,
					   xsocket_type_t   type,
					   xint_t          protocol)
{
  ConnectionFactory *factory;

  g_return_if_fail (g_type_is_a (g_type, XTYPE_SOCKET_CONNECTION));

  G_LOCK (connection_factories);

  if (connection_factories == NULL)
    connection_factories = g_hash_table_new_full (connection_factory_hash,
						  connection_factory_equal,
						  (GDestroyNotify)g_free,
						  NULL);

  factory = g_new0 (ConnectionFactory, 1);
  factory->socket_family = family;
  factory->socket_type = type;
  factory->protocol = protocol;
  factory->implementation = g_type;

  g_hash_table_insert (connection_factories,
		       factory, factory);

  G_UNLOCK (connection_factories);
}

static void
init_builtin_types (void)
{
  g_type_ensure (XTYPE_UNIX_CONNECTION);
  g_type_ensure (XTYPE_TCP_CONNECTION);
}

/**
 * xsocket_connection_factory_lookup_type:
 * @family: a #xsocket_family_t
 * @type: a #xsocket_type_t
 * @protocol_id: a protocol id
 *
 * Looks up the #xtype_t to be used when creating socket connections on
 * sockets with the specified @family, @type and @protocol_id.
 *
 * If no type is registered, the #xsocket_connection_t base type is returned.
 *
 * Returns: a #xtype_t
 *
 * Since: 2.22
 */
xtype_t
xsocket_connection_factory_lookup_type (xsocket_family_t family,
					 xsocket_type_t   type,
					 xint_t          protocol_id)
{
  ConnectionFactory *factory, key;
  xtype_t g_type;

  init_builtin_types ();

  G_LOCK (connection_factories);

  g_type = XTYPE_SOCKET_CONNECTION;

  if (connection_factories)
    {
      key.socket_family = family;
      key.socket_type = type;
      key.protocol = protocol_id;

      factory = g_hash_table_lookup (connection_factories, &key);
      if (factory)
	g_type = factory->implementation;
    }

  G_UNLOCK (connection_factories);

  return g_type;
}

/**
 * xsocket_connection_factory_create_connection:
 * @socket: a #xsocket_t
 *
 * Creates a #xsocket_connection_t subclass of the right type for
 * @socket.
 *
 * Returns: (transfer full): a #xsocket_connection_t
 *
 * Since: 2.22
 */
xsocket_connection_t *
xsocket_connection_factory_create_connection (xsocket_t *socket)
{
  xtype_t type;

  type = xsocket_connection_factory_lookup_type (xsocket_get_family (socket),
						  xsocket_get_socket_type (socket),
						  xsocket_get_protocol (socket));
  return g_object_new (type, "socket", socket, NULL);
}
