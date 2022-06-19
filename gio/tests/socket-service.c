/* GLib testing framework examples and tests
 *
 * Copyright 2014 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>

static void
active_notify_cb (xsocket_service_t *service,
                  xparam_spec_t     *pspec,
                  xpointer_t        data)
{
  xboolean_t *success = (xboolean_t *)data;

  if (xsocket_service_is_active (service))
    *success = TRUE;
}

static void
connected_cb (xobject_t      *client,
              xasync_result_t *result,
              xpointer_t      user_data)
{
  xsocket_service_t *service = XSOCKET_SERVICE (user_data);
  xsocket_connection_t *conn;
  xerror_t *error = NULL;

  g_assert_true (xsocket_service_is_active (service));

  conn = xsocket_client_connect_finish (XSOCKET_CLIENT (client), result, &error);
  g_assert_no_error (error);
  xobject_unref (conn);

  xsocket_service_stop (service);
  g_assert_false (xsocket_service_is_active (service));
}

static void
test_start_stop (void)
{
  xboolean_t success = FALSE;
  xinet_address_t *iaddr;
  xsocket_address_t *saddr, *listening_addr;
  xsocket_service_t *service;
  xerror_t *error = NULL;
  xsocket_client_t *client;

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);

  /* instantiate with xobject_new so we can pass active = false */
  service = xobject_new (XTYPE_SOCKET_SERVICE, "active", FALSE, NULL);
  g_assert_false (xsocket_service_is_active (service));

  xsignal_connect (service, "notify::active", G_CALLBACK (active_notify_cb), &success);

  xsocket_listener_add_address (XSOCKET_LISTENER (service),
                                 saddr,
                                 XSOCKET_TYPE_STREAM,
                                 XSOCKET_PROTOCOL_TCP,
                                 NULL,
                                 &listening_addr,
                                 &error);
  g_assert_no_error (error);
  xobject_unref (saddr);

  client = xsocket_client_new ();
  xsocket_client_connect_async (client,
                                 XSOCKET_CONNECTABLE (listening_addr),
                                 NULL,
                                 connected_cb, service);
  xobject_unref (client);
  xobject_unref (listening_addr);

  xsocket_service_start (service);
  g_assert_true (xsocket_service_is_active (service));

  do
    xmain_context_iteration (NULL, TRUE);
  while (!success);

  xobject_unref (service);
}

xmutex_t mutex_712570;
xcond_t cond_712570;
xboolean_t finalized;  /* (atomic) */

xtype_t test_threaded_socket_service_get_type (void);
typedef xthreaded_socket_service_t TestThreadedSocketService;
typedef GThreadedSocketServiceClass TestThreadedSocketServiceClass;

XDEFINE_TYPE (TestThreadedSocketService, test_threaded_socket_service, XTYPE_THREADED_SOCKET_SERVICE)

static void
test_threaded_socket_service_init (TestThreadedSocketService *service)
{
}

static void
test_threaded_socket_service_finalize (xobject_t *object)
{
  XOBJECT_CLASS (test_threaded_socket_service_parent_class)->finalize (object);

  /* Signal the main thread that finalization completed successfully
   * rather than hanging.
   */
  g_atomic_int_set (&finalized, TRUE);
  g_cond_signal (&cond_712570);
  g_mutex_unlock (&mutex_712570);
}

static void
test_threaded_socket_service_class_init (TestThreadedSocketServiceClass *klass)
{
  xobject_class_t *object_class = XOBJECT_CLASS (klass);

  object_class->finalize = test_threaded_socket_service_finalize;
}

static xboolean_t
connection_cb (xthreaded_socket_service_t *service,
               xsocket_connection_t      *connection,
               xobject_t                *source_object,
               xpointer_t                user_data)
{
  xmain_loop_t *loop = user_data;

  /* Since the connection attempt has come through to be handled, stop the main
   * thread waiting for it; this causes the #xsocket_service_t to be stopped. */
  xmain_loop_quit (loop);

  /* Block until the main thread has dropped its ref to @service, so that we
   * will drop the final ref from this thread.
   */
  g_mutex_lock (&mutex_712570);

  /* The service should now have 1 ref owned by the current "run"
   * signal emission, and another added by xthreaded_socket_service_t for
   * this thread. Both will be dropped after we return.
   */
  g_assert_cmpint (G_OBJECT (service)->ref_count, ==, 2);

  return FALSE;
}

static void
client_connected_cb (xobject_t      *client,
                     xasync_result_t *result,
                     xpointer_t      user_data)
{
  xsocket_connection_t *conn;
  xerror_t *error = NULL;

  conn = xsocket_client_connect_finish (XSOCKET_CLIENT (client), result, &error);
  g_assert_no_error (error);

  xobject_unref (conn);
}

static void
test_threaded_712570 (void)
{
  xsocket_service_t *service;
  xsocket_address_t *addr, *listening_addr;
  xmain_loop_t *loop;
  xsocket_client_t *client;
  xerror_t *error = NULL;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=712570");

  g_mutex_lock (&mutex_712570);

  service = xobject_new (test_threaded_socket_service_get_type (), NULL);

  addr = g_inet_socket_address_new_from_string ("127.0.0.1", 0);
  xsocket_listener_add_address (XSOCKET_LISTENER (service),
                                 addr,
                                 XSOCKET_TYPE_STREAM,
                                 XSOCKET_PROTOCOL_TCP,
                                 NULL,
                                 &listening_addr,
                                 &error);
  g_assert_no_error (error);
  xobject_unref (addr);

  loop = xmain_loop_new (NULL, FALSE);
  xsignal_connect (service, "run", G_CALLBACK (connection_cb), loop);

  client = xsocket_client_new ();
  xsocket_client_connect_async (client,
                                 XSOCKET_CONNECTABLE (listening_addr),
                                 NULL,
                                 client_connected_cb, loop);
  xobject_unref (client);
  xobject_unref (listening_addr);

  xmain_loop_run (loop);
  xmain_loop_unref (loop);

  /* Stop the service and then wait for it to asynchronously cancel
   * its outstanding accept() call (and drop the associated ref).
   * At least one main context iteration is required in some circumstances
   * to ensure that the cancellation actually happens.
   */
  xsocket_service_stop (XSOCKET_SERVICE (service));
  g_assert_false (xsocket_service_is_active (XSOCKET_SERVICE (service)));

  do
    xmain_context_iteration (NULL, TRUE);
  while (G_OBJECT (service)->ref_count > 3);

  /* Wait some more iterations, as #xtask_t results are deferred to the next
   * #xmain_context_t iteration, and propagation of a #xtask_t result takes an
   * additional ref on the source object. */
  xmain_context_iteration (NULL, FALSE);

  /* Drop our ref, then unlock the mutex and wait for the service to be
   * finalized. (Without the fix for 712570 it would hang forever here.)
   */
  xobject_unref (service);

  while (!g_atomic_int_get (&finalized))
    g_cond_wait (&cond_712570, &mutex_712570);
  g_mutex_unlock (&mutex_712570);
}

static void
closed_read_write_async_cb (xsocket_connection_t *conn,
                            xasync_result_t      *result,
                            xpointer_t           user_data)
{
  xerror_t *error = NULL;
  xboolean_t res;

  res = g_io_stream_close_finish (XIO_STREAM (conn), result, &error);
  g_assert_no_error (error);
  g_assert_true (res);
}

typedef struct {
  xsocket_connection_t *conn;
  xuint8_t *data;
} WriteAsyncData;

static void
written_read_write_async_cb (xoutput_stream_t *ostream,
                             xasync_result_t  *result,
                             xpointer_t       user_data)
{
  WriteAsyncData *data = user_data;
  xerror_t *error = NULL;
  xboolean_t res;
  xsize_t bytes_written;
  xsocket_connection_t *conn;

  conn = data->conn;

  g_free (data->data);
  g_free (data);

  res = xoutput_stream_write_all_finish (ostream, result, &bytes_written, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (bytes_written, ==, 20);

  g_io_stream_close_async (XIO_STREAM (conn),
                           G_PRIORITY_DEFAULT,
                           NULL,
                           (xasync_ready_callback_t) closed_read_write_async_cb,
                           NULL);
  xobject_unref (conn);
}

static void
connected_read_write_async_cb (xobject_t      *client,
                               xasync_result_t *result,
                               xpointer_t      user_data)
{
  xsocket_connection_t *conn;
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  WriteAsyncData *data;
  xsize_t i;
  xsocket_connection_t **sconn = user_data;

  conn = xsocket_client_connect_finish (XSOCKET_CLIENT (client), result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (conn);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (conn));

  data = g_new0 (WriteAsyncData, 1);
  data->conn = conn;
  data->data = g_new0 (xuint8_t, 20);
  for (i = 0; i < 20; i++)
    data->data[i] = i;

  xoutput_stream_write_all_async (ostream,
                                   data->data,
                                   20,
                                   G_PRIORITY_DEFAULT,
                                   NULL,
                                   (xasync_ready_callback_t) written_read_write_async_cb,
                                   data /* stolen */);

  *sconn = xobject_ref (conn);
}

typedef struct {
  xsocket_connection_t *conn;
  xoutput_vector_t *vectors;
  xuint_t n_vectors;
  xuint8_t *data;
} WritevAsyncData;

static void
writtenv_read_write_async_cb (xoutput_stream_t *ostream,
                              xasync_result_t  *result,
                              xpointer_t       user_data)
{
  WritevAsyncData *data = user_data;
  xerror_t *error = NULL;
  xboolean_t res;
  xsize_t bytes_written;
  xsocket_connection_t *conn;

  conn = data->conn;
  g_free (data->data);
  g_free (data->vectors);
  g_free (data);

  res = xoutput_stream_writev_all_finish (ostream, result, &bytes_written, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (bytes_written, ==, 20);

  g_io_stream_close_async (XIO_STREAM (conn),
                           G_PRIORITY_DEFAULT,
                           NULL,
                           (xasync_ready_callback_t) closed_read_write_async_cb,
                           NULL);
  xobject_unref (conn);
}

static void
connected_read_writev_async_cb (xobject_t      *client,
                               xasync_result_t *result,
                               xpointer_t      user_data)
{
  xsocket_connection_t *conn;
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  WritevAsyncData *data;
  xsize_t i;
  xsocket_connection_t **sconn = user_data;

  conn = xsocket_client_connect_finish (XSOCKET_CLIENT (client), result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (conn);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (conn));

  data = g_new0 (WritevAsyncData, 1);
  data->conn = conn;
  data->vectors = g_new0 (xoutput_vector_t, 3);
  data->n_vectors = 3;
  data->data = g_new0 (xuint8_t, 20);
  for (i = 0; i < 20; i++)
    data->data[i] = i;

  data->vectors[0].buffer = data->data;
  data->vectors[0].size = 5;
  data->vectors[1].buffer = data->data + 5;
  data->vectors[1].size = 10;
  data->vectors[2].buffer = data->data + 15;
  data->vectors[2].size = 5;

  xoutput_stream_writev_all_async (ostream,
                                    data->vectors,
                                    data->n_vectors,
                                    G_PRIORITY_DEFAULT,
                                    NULL,
                                    (xasync_ready_callback_t) writtenv_read_write_async_cb,
                                    data /* stolen */);

  *sconn = xobject_ref (conn);
}

typedef struct {
  xsocket_connection_t *conn;
  xuint8_t *data;
} ReadAsyncData;

static void
read_read_write_async_cb (xinput_stream_t *istream,
                          xasync_result_t *result,
                          xpointer_t      user_data)
{
  ReadAsyncData *data = user_data;
  xerror_t *error = NULL;
  xboolean_t res;
  xsize_t bytes_read;
  xsocket_connection_t *conn;
  const xuint8_t expected_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };

  res = xinput_stream_read_all_finish (istream, result, &bytes_read, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (expected_data, sizeof expected_data, data->data, bytes_read);

  conn = data->conn;
  xobject_set_data (G_OBJECT (conn), "test-data-read", GINT_TO_POINTER (TRUE));

  g_free (data->data);
  g_free (data);

  g_io_stream_close_async (XIO_STREAM (conn),
                           G_PRIORITY_DEFAULT,
                           NULL,
                           (xasync_ready_callback_t) closed_read_write_async_cb,
                           NULL);
  xobject_unref (conn);
}

static void
incoming_read_write_async_cb (xsocket_service_t    *service,
                              xsocket_connection_t *conn,
                              xobject_t           *source_object,
                              xpointer_t           user_data)
{
  ReadAsyncData *data;
  xsocket_connection_t **cconn = user_data;
  xinput_stream_t *istream;

  istream = g_io_stream_get_input_stream (XIO_STREAM (conn));

  data = g_new0 (ReadAsyncData, 1);
  data->conn = xobject_ref (conn);
  data->data = g_new0 (xuint8_t, 20);

  xinput_stream_read_all_async (istream,
                                 data->data,
                                 20,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 (xasync_ready_callback_t) read_read_write_async_cb,
                                 data /* stolen */);

  *cconn = xobject_ref (conn);
}

static void
test_read_write_async_internal (xboolean_t writev)
{
  xinet_address_t *iaddr;
  xsocket_address_t *saddr, *listening_addr;
  xsocket_service_t *service;
  xerror_t *error = NULL;
  xsocket_client_t *client;
  xsocket_connection_t *sconn = NULL, *cconn = NULL;

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);

  service = xsocket_service_new ();

  xsocket_listener_add_address (XSOCKET_LISTENER (service),
                                 saddr,
                                 XSOCKET_TYPE_STREAM,
                                 XSOCKET_PROTOCOL_TCP,
                                 NULL,
                                 &listening_addr,
                                 &error);
  g_assert_no_error (error);
  xobject_unref (saddr);

  xsignal_connect (service, "incoming", G_CALLBACK (incoming_read_write_async_cb), &sconn);

  client = xsocket_client_new ();

  if (writev)
    xsocket_client_connect_async (client,
                                   XSOCKET_CONNECTABLE (listening_addr),
                                   NULL,
                                   connected_read_writev_async_cb,
                                   &cconn);
  else
    xsocket_client_connect_async (client,
                                   XSOCKET_CONNECTABLE (listening_addr),
                                   NULL,
                                   connected_read_write_async_cb,
                                   &cconn);

  xobject_unref (client);
  xobject_unref (listening_addr);

  xsocket_service_start (service);
  g_assert_true (xsocket_service_is_active (service));

  do
    {
      xmain_context_iteration (NULL, TRUE);
    }
  while (!sconn || !cconn ||
         !g_io_stream_is_closed (XIO_STREAM (sconn)) ||
         !g_io_stream_is_closed (XIO_STREAM (cconn)));

  g_assert_true (GPOINTER_TO_INT (xobject_get_data (G_OBJECT (sconn), "test-data-read")));

  xobject_unref (sconn);
  xobject_unref (cconn);
  xobject_unref (service);
}

/* test_t if connecting to a socket service and asynchronously writing data on
 * one side followed by reading the same data on the other side of the
 * connection works correctly
 */
static void
test_read_write_async (void)
{
  test_read_write_async_internal (FALSE);
}

/* test_t if connecting to a socket service and asynchronously writing data on
 * one side followed by reading the same data on the other side of the
 * connection works correctly. This uses writev() instead of normal write().
 */
static void
test_read_writev_async (void)
{
  test_read_write_async_internal (TRUE);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/socket-service/start-stop", test_start_stop);
  g_test_add_func ("/socket-service/threaded/712570", test_threaded_712570);
  g_test_add_func ("/socket-service/read_write_async", test_read_write_async);
  g_test_add_func ("/socket-service/read_writev_async", test_read_writev_async);

  return g_test_run();
}
