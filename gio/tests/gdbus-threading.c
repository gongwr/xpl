/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include <gio/gio.h>
#include <unistd.h>
#include <string.h>

#include "gdbus-tests.h"

/* all tests rely on a global connection */
static xdbus_connection_t *c = NULL;

typedef struct
{
  xmain_context_t *context;
  xboolean_t timed_out;
} TimeoutData;

static xboolean_t
timeout_cb (xpointer_t user_data)
{
  TimeoutData *data = user_data;

  data->timed_out = TRUE;
  xmain_context_wakeup (data->context);

  return G_SOURCE_REMOVE;
}

/* Check that the given @connection has only one ref, waiting to let any pending
 * unrefs complete first. This is typically used on the shared connection, to
 * ensure it’s in a correct state before beginning the next test. */
static void
(assert_connection_has_one_ref) (xdbus_connection_t *connection,
                                 xmain_context_t    *context,
                                 const xchar_t     *calling_function)
{
  xsource_t *timeout_source = NULL;
  TimeoutData data = { context, FALSE };

  if (g_atomic_int_get (&G_OBJECT (connection)->ref_count) == 1)
    return;

  timeout_source = g_timeout_source_new_seconds (3);
  xsource_set_callback (timeout_source, timeout_cb, &data, NULL);
  xsource_attach (timeout_source, context);

  while (g_atomic_int_get (&G_OBJECT (connection)->ref_count) != 1 && !data.timed_out)
    {
      g_debug ("refcount of %p is not right (%u rather than 1) in %s(), sleeping",
               connection, g_atomic_int_get (&G_OBJECT (connection)->ref_count), calling_function);
      xmain_context_iteration (NULL, TRUE);
    }

  xsource_destroy (timeout_source);
  xsource_unref (timeout_source);

  if (g_atomic_int_get (&G_OBJECT (connection)->ref_count) != 1)
    xerror ("connection %p had too many refs (%u rather than 1) in %s()",
             connection, g_atomic_int_get (&G_OBJECT (connection)->ref_count), calling_function);
}

/* Macro wrapper to add in the calling function name */
#define assert_connection_has_one_ref(connection, context) \
  (assert_connection_has_one_ref) (connection, context, G_STRFUNC)

/* ---------------------------------------------------------------------------------------------------- */
/* Ensure that signal and method replies are delivered in the right thread */
/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
  xthread_t *thread;
  xmain_context_t *context;
  xuint_t signal_count;
  xboolean_t unsubscribe_complete;
  xasync_result_t *async_result;
} DeliveryData;

static void
async_result_cb (xdbus_connection_t *connection,
                 xasync_result_t    *res,
                 xpointer_t         user_data)
{
  DeliveryData *data = user_data;

  data->async_result = xobject_ref (res);

  g_assert_true (xthread_self () == data->thread);

  xmain_context_wakeup (data->context);
}

static void
signal_handler (xdbus_connection_t *connection,
                const xchar_t      *sender_name,
                const xchar_t      *object_path,
                const xchar_t      *interface_name,
                const xchar_t      *signal_name,
                xvariant_t         *parameters,
                xpointer_t         user_data)
{
  DeliveryData *data = user_data;

  g_assert_true (xthread_self () == data->thread);

  data->signal_count++;

  xmain_context_wakeup (data->context);
}

static void
signal_data_free_cb (xpointer_t user_data)
{
  DeliveryData *data = user_data;

  g_assert_true (xthread_self () == data->thread);

  data->unsubscribe_complete = TRUE;

  xmain_context_wakeup (data->context);
}

static xpointer_t
test_delivery_in_thread_func (xpointer_t _data)
{
  xmain_context_t *thread_context;
  DeliveryData data;
  xcancellable_t *ca;
  xuint_t subscription_id;
  xerror_t *error = NULL;
  xvariant_t *result_variant = NULL;

  thread_context = xmain_context_new ();
  xmain_context_push_thread_default (thread_context);

  data.thread = xthread_self ();
  data.context = thread_context;
  data.signal_count = 0;
  data.unsubscribe_complete = FALSE;
  data.async_result = NULL;

  /* ---------------------------------------------------------------------------------------------------- */

  /*
   * Check that we get a reply to the GetId() method call.
   */
  xdbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) async_result_cb,
                          &data);
  while (data.async_result == NULL)
    xmain_context_iteration (thread_context, TRUE);

  result_variant = xdbus_connection_call_finish (c, data.async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (result_variant);
  g_clear_pointer (&result_variant, xvariant_unref);
  g_clear_object (&data.async_result);

  /*
   * Check that we never actually send a message if the xcancellable_t
   * is already cancelled - i.e.  we should get G_IO_ERROR_CANCELLED
   * when the actual connection is not up.
   */
  ca = xcancellable_new ();
  xcancellable_cancel (ca);
  xdbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (xasync_ready_callback_t) async_result_cb,
                          &data);
  while (data.async_result == NULL)
    xmain_context_iteration (thread_context, TRUE);

  result_variant = xdbus_connection_call_finish (c, data.async_result, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_clear_error (&error);
  g_assert_null (result_variant);
  g_clear_object (&data.async_result);

  xobject_unref (ca);

  /*
   * Check that cancellation works when the message is already in flight.
   */
  ca = xcancellable_new ();
  xdbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (xasync_ready_callback_t) async_result_cb,
                          &data);
  xcancellable_cancel (ca);

  while (data.async_result == NULL)
    xmain_context_iteration (thread_context, TRUE);

  result_variant = xdbus_connection_call_finish (c, data.async_result, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_clear_error (&error);
  g_assert_null (result_variant);
  g_clear_object (&data.async_result);

  xobject_unref (ca);

  /*
   * Check that signals are delivered to the correct thread.
   *
   * First we subscribe to the signal, then we call EmitSignal(). This should
   * cause a TestSignal emission from the testserver.
   */
  subscription_id = xdbus_connection_signal_subscribe (c,
                                                        "com.example.TestService", /* sender */
                                                        "com.example.Frob",        /* interface */
                                                        "TestSignal",              /* member */
                                                        "/com/example/test_object_t", /* path */
                                                        NULL,
                                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                                        signal_handler,
                                                        &data,
                                                        signal_data_free_cb);
  g_assert_cmpuint (subscription_id, !=, 0);
  g_assert_cmpuint (data.signal_count, ==, 0);

  xdbus_connection_call (c,
                          "com.example.TestService", /* bus_name */
                          "/com/example/test_object_t", /* object path */
                          "com.example.Frob",        /* interface name */
                          "EmitSignal",              /* method name */
                          xvariant_new_parsed ("('hello', @o '/com/example/test_object_t')"),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) async_result_cb,
                          &data);
  while (data.async_result == NULL || data.signal_count < 1)
    xmain_context_iteration (thread_context, TRUE);

  result_variant = xdbus_connection_call_finish (c, data.async_result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (result_variant);
  g_clear_pointer (&result_variant, xvariant_unref);
  g_clear_object (&data.async_result);

  g_assert_cmpuint (data.signal_count, ==, 1);

  xdbus_connection_signal_unsubscribe (c, subscription_id);
  subscription_id = 0;

  while (!data.unsubscribe_complete)
    xmain_context_iteration (thread_context, TRUE);
  g_assert_true (data.unsubscribe_complete);

  /* ---------------------------------------------------------------------------------------------------- */

  xmain_context_pop_thread_default (thread_context);
  xmain_context_unref (thread_context);

  return NULL;
}

static void
test_delivery_in_thread (void)
{
  xthread_t *thread;

  thread = xthread_new ("deliver",
                         test_delivery_in_thread_func,
                         NULL);

  xthread_join (thread);

  assert_connection_has_one_ref (c, NULL);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
  xdbus_proxy_t *proxy;
  xint_t msec;
  xuint_t num;
  xboolean_t async;

  xmain_loop_t *thread_loop;
  xthread_t *thread;
} SyncThreadData;

static void
sleep_cb (xdbus_proxy_t   *proxy,
          xasync_result_t *res,
          xpointer_t      user_data)
{
  SyncThreadData *data = user_data;
  xerror_t *error;
  xvariant_t *result;

  error = NULL;
  result = xdbus_proxy_call_finish (proxy,
                                     res,
                                     &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);

  g_assert_true (data->thread == xthread_self ());

  xmain_loop_quit (data->thread_loop);

  //g_debug ("async cb (%p)", xthread_self ());
}

static xpointer_t
test_sleep_in_thread_func (xpointer_t _data)
{
  SyncThreadData *data = _data;
  xmain_context_t *thread_context;
  xuint_t n;

  thread_context = xmain_context_new ();
  data->thread_loop = xmain_loop_new (thread_context, FALSE);
  xmain_context_push_thread_default (thread_context);

  data->thread = xthread_self ();

  for (n = 0; n < data->num; n++)
    {
      if (data->async)
        {
          //g_debug ("invoking async (%p)", xthread_self ());
          xdbus_proxy_call (data->proxy,
                             "Sleep",
                             xvariant_new ("(i)", data->msec),
                             G_DBUS_CALL_FLAGS_NONE,
                             -1,
                             NULL,
                             (xasync_ready_callback_t) sleep_cb,
                             data);
          xmain_loop_run (data->thread_loop);
          if (g_test_verbose ())
            g_printerr ("A");
          //g_debug ("done invoking async (%p)", xthread_self ());
        }
      else
        {
          xerror_t *error;
          xvariant_t *result;

          error = NULL;
          //g_debug ("invoking sync (%p)", xthread_self ());
          result = xdbus_proxy_call_sync (data->proxy,
                                           "Sleep",
                                           xvariant_new ("(i)", data->msec),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);
          if (g_test_verbose ())
            g_printerr ("S");
          //g_debug ("done invoking sync (%p)", xthread_self ());
          g_assert_no_error (error);
          g_assert_nonnull (result);
          g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
          xvariant_unref (result);
        }
    }

  xmain_context_pop_thread_default (thread_context);
  xmain_loop_unref (data->thread_loop);
  xmain_context_unref (thread_context);

  return NULL;
}

static void
test_method_calls_on_proxy (xdbus_proxy_t *proxy)
{
  xuint_t n, divisor;

  /*
   * Check that multiple threads can do calls without interfering with
   * each other. We do this by creating three threads that call the
   * Sleep() method on the server (which handles it asynchronously, e.g.
   * it won't block other requests) with different sleep durations and
   * a number of times. We do this so each set of calls add up to 4000
   * milliseconds.
   *
   * The dbus test server that this code calls into uses glib timeouts
   * to do the sleeping which have only a granularity of 1ms.  It is
   * therefore possible to lose as much as 40ms; the test could finish
   * in slightly less than 4 seconds.
   *
   * We run this test twice - first with async calls in each thread, then
   * again with sync calls
   */

  if (g_test_thorough ())
    divisor = 1;
  else
    divisor = 10;

  for (n = 0; n < 2; n++)
    {
      xboolean_t do_async;
      xthread_t *thread1;
      xthread_t *thread2;
      xthread_t *thread3;
      SyncThreadData data1;
      SyncThreadData data2;
      SyncThreadData data3;
      sint64_t start_time, end_time;
      xuint_t elapsed_msec;

      do_async = (n == 0);

      start_time = g_get_real_time ();

      data1.proxy = proxy;
      data1.msec = 40;
      data1.num = 100 / divisor;
      data1.async = do_async;
      thread1 = xthread_new ("sleep",
                              test_sleep_in_thread_func,
                              &data1);

      data2.proxy = proxy;
      data2.msec = 20;
      data2.num = 200 / divisor;
      data2.async = do_async;
      thread2 = xthread_new ("sleep2",
                              test_sleep_in_thread_func,
                              &data2);

      data3.proxy = proxy;
      data3.msec = 100;
      data3.num = 40 / divisor;
      data3.async = do_async;
      thread3 = xthread_new ("sleep3",
                              test_sleep_in_thread_func,
                              &data3);

      xthread_join (thread1);
      xthread_join (thread2);
      xthread_join (thread3);

      end_time = g_get_real_time ();

      elapsed_msec = (end_time - start_time) / 1000;

      //g_debug ("Elapsed time for %s = %d msec", n == 0 ? "async" : "sync", elapsed_msec);

      /* elapsed_msec should be 4000 msec +/- change for overhead/inaccuracy */
      g_assert_cmpint (elapsed_msec, >=, 3950 / divisor);
      g_assert_cmpint (elapsed_msec,  <, 30000 / divisor);

      if (g_test_verbose ())
        g_printerr (" ");
    }
}

static void
test_method_calls_in_thread (void)
{
  xdbus_proxy_t *proxy;
  xdbus_connection_t *connection;
  xerror_t *error;

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                               NULL,
                               &error);
  g_assert_no_error (error);
  error = NULL;
  proxy = xdbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,                      /* xdbus_interface_info_t */
                                 "com.example.TestService", /* name */
                                 "/com/example/test_object_t", /* object path */
                                 "com.example.Frob",        /* interface */
                                 NULL, /* xcancellable_t */
                                 &error);
  g_assert_no_error (error);

  test_method_calls_on_proxy (proxy);

  xobject_unref (proxy);
  xobject_unref (connection);

  if (g_test_verbose ())
    g_printerr ("\n");

  assert_connection_has_one_ref (c, NULL);
}

#define SLEEP_MIN_USEC 1
#define SLEEP_MAX_USEC 10

/* Can run in any thread */
static void
ensure_connection_works (xdbus_connection_t *conn)
{
  xvariant_t *v;
  xerror_t *error = NULL;

  v = xdbus_connection_call_sync (conn, "org.freedesktop.DBus",
      "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetId", NULL, NULL, 0, -1,
      NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (v);
  g_assert_true (xvariant_is_of_type (v, G_VARIANT_TYPE ("(s)")));
  xvariant_unref (v);
}

/**
 * get_sync_in_thread:
 * @data: (type xuint_t): delay in microseconds
 *
 * Sleep for a short time, then get a session bus connection and call
 * a method on it.
 *
 * Runs in a non-main thread.
 *
 * Returns: (transfer full): the connection
 */
static xpointer_t
get_sync_in_thread (xpointer_t data)
{
  xuint_t delay = GPOINTER_TO_UINT (data);
  xerror_t *error = NULL;
  xdbus_connection_t *conn;

  g_usleep (delay);

  conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);

  ensure_connection_works (conn);

  return conn;
}

static void
test_threaded_singleton (void)
{
  xuint_t i, n;
  xuint_t unref_wins = 0;
  xuint_t get_wins = 0;

  if (g_test_thorough ())
    n = 100000;
  else
    n = 1000;

  for (i = 0; i < n; i++)
    {
      xthread_t *thread;
      xuint_t unref_delay, get_delay;
      xdbus_connection_t *new_conn;

      /* We want to be the last ref, so let it finish setting up */
      assert_connection_has_one_ref (c, NULL);

      if (g_test_verbose () && (i % (n/50)) == 0)
        g_printerr ("%u%%\n", ((i * 100) / n));

      /* Delay for a random time on each side of the race, to perturb the
       * timing. Ideally, we want each side to win half the races; these
       * timings are about right on smcv's laptop.
       */
      unref_delay = g_random_int_range (SLEEP_MIN_USEC, SLEEP_MAX_USEC);
      get_delay = g_random_int_range (SLEEP_MIN_USEC / 2, SLEEP_MAX_USEC / 2);

      /* One half of the race is to call g_bus_get_sync... */
      thread = xthread_new ("get_sync_in_thread", get_sync_in_thread,
          GUINT_TO_POINTER (get_delay));

      /* ... and the other half is to unref the shared connection, which must
       * have exactly one ref at this point
       */
      g_usleep (unref_delay);
      xobject_unref (c);

      /* Wait for the thread to run; see what it got */
      new_conn = xthread_join (thread);

      /* If the thread won the race, it will have kept the same connection,
       * and it'll have one ref
       */
      if (new_conn == c)
        {
          get_wins++;
        }
      else
        {
          unref_wins++;
          /* c is invalid now, but new_conn is suitable for the
           * next round
           */
          c = new_conn;
        }

      ensure_connection_works (c);
    }

  if (g_test_verbose ())
    g_printerr ("Unref won %u races; Get won %u races\n", unref_wins, get_wins);
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  xerror_t *error;
  xint_t ret;
  xchar_t *path;

  g_test_init (&argc, &argv, NULL);

  session_bus_up ();

  /* this is safe; testserver will exit once the bus goes away */
  path = g_test_build_filename (G_TEST_BUILT, "gdbus-testserver", NULL);
  g_assert_true (g_spawn_command_line_async (path, NULL));
  g_free (path);

  /* Create the connection in the main thread */
  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);

  ensure_gdbus_testserver_up (c, NULL);

  g_test_add_func ("/gdbus/delivery-in-thread", test_delivery_in_thread);
  g_test_add_func ("/gdbus/method-calls-in-thread", test_method_calls_in_thread);
  g_test_add_func ("/gdbus/threaded-singleton", test_threaded_singleton);

  ret = g_test_run();

  xobject_unref (c);

  /* tear down bus */
  session_bus_down ();

  return ret;
}
