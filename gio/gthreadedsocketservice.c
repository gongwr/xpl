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
 * SECTION:gthreadedsocketservice
 * @title: xthreaded_socket_service_t
 * @short_description: A threaded xsocket_service_t
 * @include: gio/gio.h
 * @see_also: #xsocket_service_t.
 *
 * A #xthreaded_socket_service_t is a simple subclass of #xsocket_service_t
 * that handles incoming connections by creating a worker thread and
 * dispatching the connection to it by emitting the
 * #xthreaded_socket_service_t::run signal in the new thread.
 *
 * The signal handler may perform blocking IO and need not return
 * until the connection is closed.
 *
 * The service is implemented using a thread pool, so there is a
 * limited amount of threads available to serve incoming requests.
 * The service automatically stops the #xsocket_service_t from accepting
 * new connections when all threads are busy.
 *
 * As with #xsocket_service_t, you may connect to #xthreaded_socket_service_t::run,
 * or subclass and override the default handler.
 */

#include "config.h"
#include "gsocketconnection.h"
#include "gthreadedsocketservice.h"
#include "glibintl.h"
#include "gmarshal-internal.h"

struct _GThreadedSocketServicePrivate
{
  GThreadPool *thread_pool;
  int max_threads;
  xint_t job_count;
};

static xuint_t xthreaded_socket_service_run_signal;

G_DEFINE_TYPE_WITH_PRIVATE (xthreaded_socket_service_t,
                            xthreaded_socket_service,
                            XTYPE_SOCKET_SERVICE)

typedef enum
{
  PROP_MAX_THREADS = 1,
} GThreadedSocketServiceProperty;

G_LOCK_DEFINE_STATIC(job_count);

typedef struct
{
  xthreaded_socket_service_t *service;  /* (owned) */
  xsocket_connection_t *connection;  /* (owned) */
  xobject_t *source_object;  /* (owned) (nullable) */
} GThreadedSocketServiceData;

static void
xthreaded_socket_service_data_free (GThreadedSocketServiceData *data)
{
  g_clear_object (&data->service);
  g_clear_object (&data->connection);
  g_clear_object (&data->source_object);
  g_slice_free (GThreadedSocketServiceData, data);
}

static void
xthreaded_socket_service_func (xpointer_t job_data,
                                xpointer_t user_data)
{
  GThreadedSocketServiceData *data = job_data;
  xboolean_t result;

  g_signal_emit (data->service, xthreaded_socket_service_run_signal,
                 0, data->connection, data->source_object, &result);

  G_LOCK (job_count);
  if (data->service->priv->job_count-- == data->service->priv->max_threads)
    xsocket_service_start (XSOCKET_SERVICE (data->service));
  G_UNLOCK (job_count);

  xthreaded_socket_service_data_free (data);
}

static xboolean_t
xthreaded_socket_service_incoming (xsocket_service_t    *service,
                                    xsocket_connection_t *connection,
                                    xobject_t           *source_object)
{
  xthreaded_socket_service_t *threaded;
  GThreadedSocketServiceData *data;
  xerror_t *local_error = NULL;

  threaded = G_THREADED_SOCKET_SERVICE (service);

  data = g_slice_new0 (GThreadedSocketServiceData);
  data->service = xobject_ref (threaded);
  data->connection = xobject_ref (connection);
  data->source_object = (source_object != NULL) ? xobject_ref (source_object) : NULL;

  G_LOCK (job_count);
  if (++threaded->priv->job_count == threaded->priv->max_threads)
    xsocket_service_stop (service);
  G_UNLOCK (job_count);

  if (!xthread_pool_push (threaded->priv->thread_pool, data, &local_error))
    {
      g_warning ("Error handling incoming socket: %s", local_error->message);
      xthreaded_socket_service_data_free (data);
    }

  g_clear_error (&local_error);

  return FALSE;
}

static void
xthreaded_socket_service_init (xthreaded_socket_service_t *service)
{
  service->priv = xthreaded_socket_service_get_instance_private (service);
  service->priv->max_threads = 10;
}

static void
xthreaded_socket_service_constructed (xobject_t *object)
{
  xthreaded_socket_service_t *service = G_THREADED_SOCKET_SERVICE (object);

  service->priv->thread_pool =
    xthread_pool_new  (xthreaded_socket_service_func,
			NULL,
			service->priv->max_threads,
			FALSE,
			NULL);
}


static void
xthreaded_socket_service_finalize (xobject_t *object)
{
  xthreaded_socket_service_t *service = G_THREADED_SOCKET_SERVICE (object);

  /* All jobs in the pool hold a reference to this #xthreaded_socket_service_t, so
   * this should only be called once the pool is empty: */
  xthread_pool_free (service->priv->thread_pool, FALSE, FALSE);

  G_OBJECT_CLASS (xthreaded_socket_service_parent_class)
    ->finalize (object);
}

static void
xthreaded_socket_service_get_property (xobject_t    *object,
					xuint_t       prop_id,
					xvalue_t     *value,
					xparam_spec_t *pspec)
{
  xthreaded_socket_service_t *service = G_THREADED_SOCKET_SERVICE (object);

  switch ((GThreadedSocketServiceProperty) prop_id)
    {
      case PROP_MAX_THREADS:
	xvalue_set_int (value, service->priv->max_threads);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xthreaded_socket_service_set_property (xobject_t      *object,
					xuint_t         prop_id,
					const xvalue_t *value,
					xparam_spec_t   *pspec)
{
  xthreaded_socket_service_t *service = G_THREADED_SOCKET_SERVICE (object);

  switch ((GThreadedSocketServiceProperty) prop_id)
    {
      case PROP_MAX_THREADS:
	service->priv->max_threads = xvalue_get_int (value);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
xthreaded_socket_service_class_init (GThreadedSocketServiceClass *class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (class);
  GSocketServiceClass *ss_class = &class->parent_class;

  gobject_class->constructed = xthreaded_socket_service_constructed;
  gobject_class->finalize = xthreaded_socket_service_finalize;
  gobject_class->set_property = xthreaded_socket_service_set_property;
  gobject_class->get_property = xthreaded_socket_service_get_property;

  ss_class->incoming = xthreaded_socket_service_incoming;

  /**
   * xthreaded_socket_service_t::run:
   * @service: the #xthreaded_socket_service_t.
   * @connection: a new #xsocket_connection_t object.
   * @source_object: (nullable): the source_object passed to xsocket_listener_add_address().
   *
   * The ::run signal is emitted in a worker thread in response to an
   * incoming connection. This thread is dedicated to handling
   * @connection and may perform blocking IO. The signal handler need
   * not return until the connection is closed.
   *
   * Returns: %TRUE to stop further signal handlers from being called
   */
  xthreaded_socket_service_run_signal =
    g_signal_new (I_("run"), XTYPE_FROM_CLASS (class), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GThreadedSocketServiceClass, run),
		  g_signal_accumulator_true_handled, NULL,
		  _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECT,
		  XTYPE_BOOLEAN,
		  2, XTYPE_SOCKET_CONNECTION, XTYPE_OBJECT);
  g_signal_set_va_marshaller (xthreaded_socket_service_run_signal,
			      XTYPE_FROM_CLASS (class),
			      _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECTv);

  xobject_class_install_property (gobject_class, PROP_MAX_THREADS,
				   g_param_spec_int ("max-threads",
						     P_("Max threads"),
						     P_("The max number of threads handling clients for this service"),
						     -1,
						     G_MAXINT,
						     10,
						     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/**
 * xthreaded_socket_service_new:
 * @max_threads: the maximal number of threads to execute concurrently
 *   handling incoming clients, -1 means no limit
 *
 * Creates a new #xthreaded_socket_service_t with no listeners. Listeners
 * must be added with one of the #xsocket_listener_t "add" methods.
 *
 * Returns: a new #xsocket_service_t.
 *
 * Since: 2.22
 */
xsocket_service_t *
xthreaded_socket_service_new (int max_threads)
{
  return xobject_new (XTYPE_THREADED_SOCKET_SERVICE,
		       "max-threads", max_threads,
		       NULL);
}
