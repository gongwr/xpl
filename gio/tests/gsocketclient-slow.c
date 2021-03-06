/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2018 Igalia S.L.
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

#include <gio/gio.h>

static void
on_connected (xobject_t      *source_object,
              xasync_result_t *result,
              xpointer_t      user_data)
{
  xsocket_connection_t *conn;
  xerror_t *error = NULL;

  conn = xsocket_client_connect_to_uri_finish (XSOCKET_CLIENT (source_object), result, &error);
  g_assert_no_error (error);

  xobject_unref (conn);
  xmain_loop_quit (user_data);
}

static void
test_happy_eyeballs (void)
{
  xsocket_client_t *client;
  xsocket_service_t *service;
  xerror_t *error = NULL;
  xuint16_t port;
  xmain_loop_t *loop;

  loop = xmain_loop_new (NULL, FALSE);

  service = xsocket_service_new ();
  port = xsocket_listener_add_any_inet_port (XSOCKET_LISTENER (service), NULL, &error);
  g_assert_no_error (error);
  xsocket_service_start (service);

  /* All of the magic here actually happens in slow-connect-preload.c
   * which as you would guess is preloaded. So this is just making a
   * normal connection that happens to take 600ms each time. This will
   * trigger the logic to make multiple parallel connections.
   */
  client = xsocket_client_new ();
  xsocket_client_connect_to_host_async (client, "localhost", port, NULL, on_connected, loop);
  xmain_loop_run (loop);

  xmain_loop_unref (loop);
  xobject_unref (service);
  xobject_unref (client);
}

static void
on_connected_cancelled (xobject_t      *source_object,
                        xasync_result_t *result,
                        xpointer_t      user_data)
{
  xsocket_connection_t *conn;
  xerror_t *error = NULL;

  conn = xsocket_client_connect_to_uri_finish (XSOCKET_CLIENT (source_object), result, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null (conn);

  xerror_free (error);
  xmain_loop_quit (user_data);
}

typedef struct
{
  xcancellable_t *cancellable;
  xboolean_t completed;
} EventCallbackData;

static void
on_event (xsocket_client_t      *client,
          GSocketClientEvent  event,
          xsocket_connectable_t *connectable,
          xio_stream_t          *connection,
          EventCallbackData  *data)
{
  if (data->cancellable && event == XSOCKET_CLIENT_CONNECTED)
    {
      xcancellable_cancel (data->cancellable);
    }
  else if (event == XSOCKET_CLIENT_COMPLETE)
    {
      data->completed = TRUE;
      g_assert_null (connection);
    }
}

static void
test_happy_eyeballs_cancel_delayed (void)
{
  xsocket_client_t *client;
  xsocket_service_t *service;
  xerror_t *error = NULL;
  xuint16_t port;
  xmain_loop_t *loop;
  EventCallbackData data = { NULL, FALSE };

  /* This just tests that cancellation works as expected, still emits the completed signal,
   * and never returns a connection */

  loop = xmain_loop_new (NULL, FALSE);

  service = xsocket_service_new ();
  port = xsocket_listener_add_any_inet_port (XSOCKET_LISTENER (service), NULL, &error);
  g_assert_no_error (error);
  xsocket_service_start (service);

  client = xsocket_client_new ();
  data.cancellable = xcancellable_new ();
  xsocket_client_connect_to_host_async (client, "localhost", port, data.cancellable, on_connected_cancelled, loop);
  xsignal_connect (client, "event", G_CALLBACK (on_event), &data);
  xmain_loop_run (loop);

  g_assert_true (data.completed);
  xmain_loop_unref (loop);
  xobject_unref (service);
  xobject_unref (client);
  xobject_unref (data.cancellable);
}

static void
test_happy_eyeballs_cancel_instant (void)
{
  xsocket_client_t *client;
  xsocket_service_t *service;
  xerror_t *error = NULL;
  xuint16_t port;
  xmain_loop_t *loop;
  xcancellable_t *cancel;
  EventCallbackData data = { NULL, FALSE };

  /* This tests the same things as above, test_happy_eyeballs_cancel_delayed(), but
   * with different timing since it sends an already cancelled cancellable */

  loop = xmain_loop_new (NULL, FALSE);

  service = xsocket_service_new ();
  port = xsocket_listener_add_any_inet_port (XSOCKET_LISTENER (service), NULL, &error);
  g_assert_no_error (error);
  xsocket_service_start (service);

  client = xsocket_client_new ();
  cancel = xcancellable_new ();
  xcancellable_cancel (cancel);
  xsocket_client_connect_to_host_async (client, "localhost", port, cancel, on_connected_cancelled, loop);
  xsignal_connect (client, "event", G_CALLBACK (on_event), &data);
  xmain_loop_run (loop);

  g_assert_true (data.completed);
  xmain_loop_unref (loop);
  xobject_unref (service);
  xobject_unref (client);
  xobject_unref (cancel);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/socket-client/happy-eyeballs/slow", test_happy_eyeballs);
  g_test_add_func ("/socket-client/happy-eyeballs/cancellation/instant", test_happy_eyeballs_cancel_instant);
  g_test_add_func ("/socket-client/happy-eyeballs/cancellation/delayed", test_happy_eyeballs_cancel_delayed);


  return g_test_run ();
}
