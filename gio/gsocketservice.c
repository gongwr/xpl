/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2009 Codethink Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

/**
 * SECTION:gsocketservice
 * @title: xsocket_service_t
 * @short_description: Make it easy to implement a network service
 * @include: gio/gio.h
 * @see_also: #xthreaded_socket_service_t, #xsocket_listener_t.
 *
 * A #xsocket_service_t is an object that represents a service that
 * is provided to the network or over local sockets.  When a new
 * connection is made to the service the #xsocket_service_t::incoming
 * signal is emitted.
 *
 * A #xsocket_service_t is a subclass of #xsocket_listener_t and you need
 * to add the addresses you want to accept connections on with the
 * #xsocket_listener_t APIs.
 *
 * There are two options for implementing a network service based on
 * #xsocket_service_t. The first is to create the service using
 * xsocket_service_new() and to connect to the #xsocket_service_t::incoming
 * signal. The second is to subclass #xsocket_service_t and override the
 * default signal handler implementation.
 *
 * In either case, the handler must immediately return, or else it
 * will block additional incoming connections from being serviced.
 * If you are interested in writing connection handlers that contain
 * blocking code then see #xthreaded_socket_service_t.
 *
 * The socket service runs on the main loop of the
 * [thread-default context][g-main-context-push-thread-default-context]
 * of the thread it is created in, and is not
 * threadsafe in general. However, the calls to start and stop the
 * service are thread-safe so these can be used from threads that
 * handle incoming clients.
 *
 * Since: 2.22
 */

#include "config.h"
#include "gsocketservice.h"

#include <gio/gio.h>
#include "gsocketlistener.h"
#include "gsocketconnection.h"
#include "glibintl.h"
#include "gmarshal-internal.h"

struct _GSocketServicePrivate
{
  xcancellable_t *cancellable;
  xuint_t active : 1;
  xuint_t outstanding_accept : 1;
};

static xuint_t xsocket_service_incoming_signal;

G_LOCK_DEFINE_STATIC(active);

G_DEFINE_TYPE_WITH_PRIVATE (xsocket_service_t, xsocket_service, XTYPE_SOCKET_LISTENER)

enum
{
  PROP_0,
  PROP_ACTIVE
};

static void xsocket_service_ready (xobject_t      *object,
				    xasync_result_t *result,
				    xpointer_t      user_data);

static xboolean_t
xsocket_service_real_incoming (xsocket_service_t    *service,
                                xsocket_connection_t *connection,
                                xobject_t           *source_object)
{
  return FALSE;
}

static void
xsocket_service_init (xsocket_service_t *service)
{
  service->priv = xsocket_service_get_instance_private (service);
  service->priv->cancellable = xcancellable_new ();
  service->priv->active = TRUE;
}

static void
xsocket_service_finalize (xobject_t *object)
{
  xsocket_service_t *service = XSOCKET_SERVICE (object);

  xobject_unref (service->priv->cancellable);

  G_OBJECT_CLASS (xsocket_service_parent_class)
    ->finalize (object);
}

static void
do_accept (xsocket_service_t  *service)
{
  xsocket_listener_accept_async (XSOCKET_LISTENER (service),
				  service->priv->cancellable,
				  xsocket_service_ready, NULL);
  service->priv->outstanding_accept = TRUE;
}

static xboolean_t
get_active (xsocket_service_t *service)
{
  xboolean_t active;

  G_LOCK (active);
  active = service->priv->active;
  G_UNLOCK (active);

  return active;
}

static void
set_active (xsocket_service_t *service, xboolean_t active)
{
  xboolean_t notify = FALSE;

  active = !!active;

  G_LOCK (active);

  if (active != service->priv->active)
    {
      service->priv->active = active;
      notify = TRUE;

      if (active)
        {
          if (service->priv->outstanding_accept)
            xcancellable_cancel (service->priv->cancellable);
          else
            do_accept (service);
        }
      else
        {
          if (service->priv->outstanding_accept)
            xcancellable_cancel (service->priv->cancellable);
        }
    }

  G_UNLOCK (active);

  if (notify)
    xobject_notify (G_OBJECT (service), "active");
}

static void
xsocket_service_get_property (xobject_t    *object,
                               xuint_t       prop_id,
                               xvalue_t     *value,
                               xparam_spec_t *pspec)
{
  xsocket_service_t *service = XSOCKET_SERVICE (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      xvalue_set_boolean (value, get_active (service));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xsocket_service_set_property (xobject_t      *object,
                               xuint_t         prop_id,
                               const xvalue_t *value,
                               xparam_spec_t   *pspec)
{
  xsocket_service_t *service = XSOCKET_SERVICE (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      set_active (service, xvalue_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xsocket_service_changed (xsocket_listener_t *listener)
{
  xsocket_service_t  *service = XSOCKET_SERVICE (listener);

  G_LOCK (active);

  if (service->priv->active)
    {
      if (service->priv->outstanding_accept)
	xcancellable_cancel (service->priv->cancellable);
      else
	do_accept (service);
    }

  G_UNLOCK (active);
}

/**
 * xsocket_service_is_active:
 * @service: a #xsocket_service_t
 *
 * Check whether the service is active or not. An active
 * service will accept new clients that connect, while
 * a non-active service will let connecting clients queue
 * up until the service is started.
 *
 * Returns: %TRUE if the service is active, %FALSE otherwise
 *
 * Since: 2.22
 */
xboolean_t
xsocket_service_is_active (xsocket_service_t *service)
{
  g_return_val_if_fail (X_IS_SOCKET_SERVICE (service), FALSE);

  return get_active (service);
}

/**
 * xsocket_service_start:
 * @service: a #xsocket_service_t
 *
 * Restarts the service, i.e. start accepting connections
 * from the added sockets when the mainloop runs. This only needs
 * to be called after the service has been stopped from
 * xsocket_service_stop().
 *
 * This call is thread-safe, so it may be called from a thread
 * handling an incoming client request.
 *
 * Since: 2.22
 */
void
xsocket_service_start (xsocket_service_t *service)
{
  g_return_if_fail (X_IS_SOCKET_SERVICE (service));

  set_active (service, TRUE);
}

/**
 * xsocket_service_stop:
 * @service: a #xsocket_service_t
 *
 * Stops the service, i.e. stops accepting connections
 * from the added sockets when the mainloop runs.
 *
 * This call is thread-safe, so it may be called from a thread
 * handling an incoming client request.
 *
 * Note that this only stops accepting new connections; it does not
 * close the listening sockets, and you can call
 * xsocket_service_start() again later to begin listening again. To
 * close the listening sockets, call xsocket_listener_close(). (This
 * will happen automatically when the #xsocket_service_t is finalized.)
 *
 * This must be called before calling xsocket_listener_close() as
 * the socket service will start accepting connections immediately
 * when a new socket is added.
 *
 * Since: 2.22
 */
void
xsocket_service_stop (xsocket_service_t *service)
{
  g_return_if_fail (X_IS_SOCKET_SERVICE (service));

  set_active (service, FALSE);
}

static xboolean_t
xsocket_service_incoming (xsocket_service_t    *service,
                           xsocket_connection_t *connection,
                           xobject_t           *source_object)
{
  xboolean_t result;

  xsignal_emit (service, xsocket_service_incoming_signal,
                 0, connection, source_object, &result);
  return result;
}

static void
xsocket_service_class_init (GSocketServiceClass *class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (class);
  GSocketListenerClass *listener_class = XSOCKET_LISTENER_CLASS (class);

  gobject_class->finalize = xsocket_service_finalize;
  gobject_class->set_property = xsocket_service_set_property;
  gobject_class->get_property = xsocket_service_get_property;
  listener_class->changed = xsocket_service_changed;
  class->incoming = xsocket_service_real_incoming;

  /**
   * xsocket_service_t::incoming:
   * @service: the #xsocket_service_t
   * @connection: a new #xsocket_connection_t object
   * @source_object: (nullable): the source_object passed to
   *     xsocket_listener_add_address()
   *
   * The ::incoming signal is emitted when a new incoming connection
   * to @service needs to be handled. The handler must initiate the
   * handling of @connection, but may not block; in essence,
   * asynchronous operations must be used.
   *
   * @connection will be unreffed once the signal handler returns,
   * so you need to ref it yourself if you are planning to use it.
   *
   * Returns: %TRUE to stop other handlers from being called
   *
   * Since: 2.22
   */
  xsocket_service_incoming_signal =
    xsignal_new (I_("incoming"), XTYPE_FROM_CLASS (class), G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GSocketServiceClass, incoming),
                  xsignal_accumulator_true_handled, NULL,
                  _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECT,
                  XTYPE_BOOLEAN,
                  2, XTYPE_SOCKET_CONNECTION, XTYPE_OBJECT);
  xsignal_set_va_marshaller (xsocket_service_incoming_signal,
                              XTYPE_FROM_CLASS (class),
                              _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECTv);

  /**
   * xsocket_service_t:active:
   *
   * Whether the service is currently accepting connections.
   *
   * Since: 2.46
   */
  xobject_class_install_property (gobject_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the service is currently accepting connections"),
                                                         TRUE,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
xsocket_service_ready (xobject_t      *object,
                        xasync_result_t *result,
                        xpointer_t      user_data)
{
  xsocket_listener_t *listener = XSOCKET_LISTENER (object);
  xsocket_service_t *service = XSOCKET_SERVICE (object);
  xsocket_connection_t *connection;
  xobject_t *source_object;
  xerror_t *error = NULL;

  connection = xsocket_listener_accept_finish (listener, result, &source_object, &error);
  if (error)
    {
      if (!xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
	g_warning ("fail: %s", error->message);
      xerror_free (error);
    }
  else
    {
      xsocket_service_incoming (service, connection, source_object);
      xobject_unref (connection);
    }

  G_LOCK (active);

  xcancellable_reset (service->priv->cancellable);

  /* requeue */
  service->priv->outstanding_accept = FALSE;
  if (service->priv->active)
    do_accept (service);

  G_UNLOCK (active);
}

/**
 * xsocket_service_new:
 *
 * Creates a new #xsocket_service_t with no sockets to listen for.
 * New listeners can be added with e.g. xsocket_listener_add_address()
 * or xsocket_listener_add_inet_port().
 *
 * New services are created active, there is no need to call
 * xsocket_service_start(), unless xsocket_service_stop() has been
 * called before.
 *
 * Returns: a new #xsocket_service_t.
 *
 * Since: 2.22
 */
xsocket_service_t *
xsocket_service_new (void)
{
  return xobject_new (XTYPE_SOCKET_SERVICE, NULL);
}
