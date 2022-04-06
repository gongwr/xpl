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

#include <sys/types.h>

#include "gdbus-tests.h"

/* all tests rely on a shared mainloop */
static xmain_loop_t *loop = NULL;

#if 0
G_GNUC_UNUSED static void
_log (const xchar_t *format, ...)
{
  GTimeVal now;
  time_t now_time;
  struct tm *now_tm;
  xchar_t time_buf[128];
  xchar_t *str;
  va_list var_args;

  va_start (var_args, format);
  str = xstrdup_vprintf (format, var_args);
  va_end (var_args);

  g_get_current_time (&now);
  now_time = (time_t) now.tv_sec;
  now_tm = localtime (&now_time);
  strftime (time_buf, sizeof time_buf, "%H:%M:%S", now_tm);

  g_printerr ("%s.%06d: %s\n",
           time_buf, (xint_t) now.tv_usec / 1000,
           str);
  g_free (str);
}
#else
#define _log(...)
#endif

static xboolean_t
test_connection_quit_mainloop (xpointer_t user_data)
{
  xboolean_t *quit_mainloop_fired = user_data;  /* (atomic) */
  _log ("quit_mainloop_fired");
  g_atomic_int_set (quit_mainloop_fired, TRUE);
  xmain_loop_quit (loop);
  return G_SOURCE_CONTINUE;
}

/* ---------------------------------------------------------------------------------------------------- */
/* Connection life-cycle testing */
/* ---------------------------------------------------------------------------------------------------- */

static const xdbus_interface_info_t boo_interface_info =
{
  -1,
  "org.example.Boo",
  (xdbus_method_info_t **) NULL,
  (xdbus_signalInfo_t **) NULL,
  (xdbus_property_info_t **) NULL,
  NULL,
};

static const xdbus_interface_vtable_t boo_vtable =
{
  NULL, /* _method_call */
  NULL, /* _get_property */
  NULL,  /* _set_property */
  { 0 }
};

/* Runs in a worker thread. */
static xdbus_message_t *
some_filter_func (xdbus_connection_t *connection,
                  xdbus_message_t    *message,
                  xboolean_t         incoming,
                  xpointer_t         user_data)
{
  return message;
}

static void
on_name_owner_changed (xdbus_connection_t *connection,
                       const xchar_t     *sender_name,
                       const xchar_t     *object_path,
                       const xchar_t     *interface_name,
                       const xchar_t     *signal_name,
                       xvariant_t        *parameters,
                       xpointer_t         user_data)
{
}

static void
a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop (xpointer_t user_data)
{
  xboolean_t *val = user_data;  /* (atomic) */
  g_atomic_int_set (val, TRUE);
  _log ("destroynotify fired for %p", val);
  xmain_loop_quit (loop);
}

static void
test_connection_bus_failure (void)
{
  xdbus_connection_t *c;
  xerror_t *error = NULL;

  /*
   * Check for correct behavior when no bus is present
   *
   */
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_nonnull (error);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_assert_null (c);
  xerror_free (error);
}

static void
test_connection_life_cycle (void)
{
  xboolean_t ret;
  xdbus_connection_t *c;
  xdbus_connection_t *c2;
  xerror_t *error;
  xboolean_t on_signal_registration_freed_called;  /* (atomic) */
  xboolean_t on_filter_freed_called;  /* (atomic) */
  xboolean_t on_register_object_freed_called;  /* (atomic) */
  xboolean_t quit_mainloop_fired;  /* (atomic) */
  xuint_t quit_mainloop_id;
  xuint_t registration_id;

  error = NULL;

  /*
   *  Check for correct behavior when a bus is present
   */
  session_bus_up ();
  /* case 1 */
  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);
  g_assert_false (g_dbus_connection_is_closed (c));

  /*
   * Check that singleton handling work
   */
  error = NULL;
  c2 = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  g_assert_true (c == c2);
  xobject_unref (c2);

  /*
   * Check that private connections work
   */
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  g_assert_true (c != c2);
  xobject_unref (c2);

  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  g_assert_false (g_dbus_connection_is_closed (c2));
  ret = g_dbus_connection_close_sync (c2, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  _g_assert_signal_received (c2, "closed");
  g_assert_true (g_dbus_connection_is_closed (c2));
  ret = g_dbus_connection_close_sync (c2, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  xerror_free (error);
  g_assert_false (ret);
  xobject_unref (c2);

  /*
   * Check that the finalization code works
   *
   * (and that the xdestroy_notify_t for filters and objects and signal
   * registrations are run as expected)
   */
  error = NULL;
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c2);
  /* signal registration */
  g_atomic_int_set (&on_signal_registration_freed_called, FALSE);
  g_dbus_connection_signal_subscribe (c2,
                                      "org.freedesktop.DBus", /* bus name */
                                      "org.freedesktop.DBus", /* interface */
                                      "NameOwnerChanged",     /* member */
                                      "/org/freesktop/DBus",  /* path */
                                      NULL,                   /* arg0 */
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      on_name_owner_changed,
                                      (xpointer_t) &on_signal_registration_freed_called,
                                      a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop);
  /* filter func */
  g_atomic_int_set (&on_filter_freed_called, FALSE);
  g_dbus_connection_add_filter (c2,
                                some_filter_func,
                                (xpointer_t) &on_filter_freed_called,
                                a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop);
  /* object registration */
  g_atomic_int_set (&on_register_object_freed_called, FALSE);
  error = NULL;
  registration_id = g_dbus_connection_register_object (c2,
                                                       "/foo",
                                                       (xdbus_interface_info_t *) &boo_interface_info,
                                                       &boo_vtable,
                                                       (xpointer_t) &on_register_object_freed_called,
                                                       a_gdestroynotify_that_sets_a_gboolean_to_true_and_quits_loop,
                                                       &error);
  g_assert_no_error (error);
  g_assert_cmpuint (registration_id, >, 0);
  /* ok, finalize the connection and check that all the xdestroy_notify_t functions are invoked as expected */
  xobject_unref (c2);
  g_atomic_int_set (&quit_mainloop_fired, FALSE);
  quit_mainloop_id = g_timeout_add (30000, test_connection_quit_mainloop, (xpointer_t) &quit_mainloop_fired);
  _log ("destroynotifies for\n"
        " register_object %p\n"
        " filter          %p\n"
        " signal          %p",
        &on_register_object_freed_called,
        &on_filter_freed_called,
        &on_signal_registration_freed_called);
  while (TRUE)
    {
      if (g_atomic_int_get (&on_signal_registration_freed_called) &&
          g_atomic_int_get (&on_filter_freed_called) &&
          g_atomic_int_get (&on_register_object_freed_called))
        break;
      if (g_atomic_int_get (&quit_mainloop_fired))
        break;
      _log ("entering loop");
      xmain_loop_run (loop);
      _log ("exiting loop");
    }
  xsource_remove (quit_mainloop_id);
  g_assert_true (g_atomic_int_get (&on_signal_registration_freed_called));
  g_assert_true (g_atomic_int_get (&on_filter_freed_called));
  g_assert_true (g_atomic_int_get (&on_register_object_freed_called));
  g_assert_false (g_atomic_int_get (&quit_mainloop_fired));

  /*
   *  Check for correct behavior when the bus goes away
   *
   */
  g_assert_false (g_dbus_connection_is_closed (c));
  g_dbus_connection_set_exit_on_close (c, FALSE);
  session_bus_stop ();
  _g_assert_signal_received (c, "closed");
  g_assert_true (g_dbus_connection_is_closed (c));
  xobject_unref (c);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */
/* Test that sending and receiving messages work as expected */
/* ---------------------------------------------------------------------------------------------------- */

static void
msg_cb_expect_error_disconnected (xdbus_connection_t *connection,
                                  xasync_result_t    *res,
                                  xpointer_t         user_data)
{
  xerror_t *error;
  xvariant_t *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  xerror_free (error);
  g_assert_null (result);

  xmain_loop_quit (loop);
}

static void
msg_cb_expect_error_unknown_method (xdbus_connection_t *connection,
                                    xasync_result_t    *res,
                                    xpointer_t         user_data)
{
  xerror_t *error;
  xvariant_t *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
  g_assert_true (g_dbus_error_is_remote_error (error));
  xerror_free (error);
  g_assert_null (result);

  xmain_loop_quit (loop);
}

static void
msg_cb_expect_success (xdbus_connection_t *connection,
                       xasync_result_t    *res,
                       xpointer_t         user_data)
{
  xerror_t *error;
  xvariant_t *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  xvariant_unref (result);

  xmain_loop_quit (loop);
}

static void
msg_cb_expect_error_cancelled (xdbus_connection_t *connection,
                               xasync_result_t    *res,
                               xpointer_t         user_data)
{
  xerror_t *error;
  xvariant_t *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  xerror_free (error);
  g_assert_null (result);

  xmain_loop_quit (loop);
}

static void
msg_cb_expect_error_cancelled_2 (xdbus_connection_t *connection,
                                 xasync_result_t    *res,
                                 xpointer_t         user_data)
{
  xerror_t *error;
  xvariant_t *result;

  /* Make sure gdbusconnection isn't holding @connection's lock. (#747349) */
  g_dbus_connection_get_last_serial (connection);

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false (g_dbus_error_is_remote_error (error));
  xerror_free (error);
  g_assert_null (result);

  xmain_loop_quit (loop);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_send (void)
{
  xdbus_connection_t *c;
  xcancellable_t *ca;

  session_bus_up ();

  /* First, get an unopened connection */
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c);
  g_assert_false (g_dbus_connection_is_closed (c));

  /*
   * Check that we never actually send a message if the xcancellable_t
   * is already cancelled - i.e.  we should get G_IO_ERROR_CANCELLED
   * when the actual connection is not up.
   */
  ca = g_cancellable_new ();
  g_cancellable_cancel (ca);
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (xasync_ready_callback_t) msg_cb_expect_error_cancelled,
                          NULL);
  xmain_loop_run (loop);
  xobject_unref (ca);

  /*
   * Check that we get a reply to the GetId() method call.
   */
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) msg_cb_expect_success,
                          NULL);
  xmain_loop_run (loop);

  /*
   * Check that we get an error reply to the NonExistantMethod() method call.
   */
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "NonExistantMethod",     /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) msg_cb_expect_error_unknown_method,
                          NULL);
  xmain_loop_run (loop);

  /*
   * Check that cancellation works when the message is already in flight.
   */
  ca = g_cancellable_new ();
  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          ca,
                          (xasync_ready_callback_t) msg_cb_expect_error_cancelled_2,
                          NULL);
  g_cancellable_cancel (ca);
  xmain_loop_run (loop);
  xobject_unref (ca);

  /*
   * Check that we get an error when sending to a connection that is disconnected.
   */
  g_dbus_connection_set_exit_on_close (c, FALSE);
  session_bus_stop ();
  _g_assert_signal_received (c, "closed");
  g_assert_true (g_dbus_connection_is_closed (c));

  g_dbus_connection_call (c,
                          "org.freedesktop.DBus",  /* bus_name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetId",                 /* method name */
                          NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) msg_cb_expect_error_disconnected,
                          NULL);
  xmain_loop_run (loop);

  xobject_unref (c);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */
/* Connection signal tests */
/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_signal_handler (xdbus_connection_t  *connection,
                                const xchar_t      *sender_name,
                                const xchar_t      *object_path,
                                const xchar_t      *interface_name,
                                const xchar_t      *signal_name,
                                xvariant_t         *parameters,
                                xpointer_t         user_data)
{
  xint_t *counter = user_data;
  *counter += 1;

  /*g_debug ("in test_connection_signal_handler (sender=%s path=%s interface=%s member=%s)",
           sender_name,
           object_path,
           interface_name,
           signal_name);*/

  xmain_loop_quit (loop);
}

static void
test_connection_signals (void)
{
  xdbus_connection_t *c1;
  xdbus_connection_t *c2;
  xdbus_connection_t *c3;
  xuint_t s1;
  xuint_t s1b;
  xuint_t s2;
  xuint_t s3;
  xint_t count_s1;
  xint_t count_s1b;
  xint_t count_s2;
  xint_t count_name_owner_changed;
  xerror_t *error;
  xboolean_t ret;
  xvariant_t *result;
  xboolean_t quit_mainloop_fired;
  xuint_t quit_mainloop_id;

  error = NULL;

  /*
   * Bring up first separate connections
   */
  session_bus_up ();
  /* if running with dbus-monitor, it claims the name :1.0 - so if we don't run with the monitor
   * emulate this
   */
  if (g_getenv ("G_DBUS_MONITOR") == NULL)
    {
      c1 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
      g_assert_nonnull (c1);
      g_assert_false (g_dbus_connection_is_closed (c1));
      xobject_unref (c1);
    }
  c1 = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c1);
  g_assert_false (g_dbus_connection_is_closed (c1));
  g_assert_cmpstr (g_dbus_connection_get_unique_name (c1), ==, ":1.1");

  /*
   * Install two signal handlers for the first connection
   *
   *  - Listen to the signal "foo_t" from :1.2 (e.g. c2)
   *  - Listen to the signal "foo_t" from anyone (e.g. both c2 and c3)
   *
   * and then count how many times this signal handler was invoked.
   */
  s1 = g_dbus_connection_signal_subscribe (c1,
                                           ":1.2",
                                           "org.gtk.GDBus.ExampleInterface",
                                           "foo_t",
                                           "/org/gtk/GDBus/ExampleInterface",
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_s1,
                                           NULL);
  s2 = g_dbus_connection_signal_subscribe (c1,
                                           NULL, /* match any sender */
                                           "org.gtk.GDBus.ExampleInterface",
                                           "foo_t",
                                           "/org/gtk/GDBus/ExampleInterface",
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_s2,
                                           NULL);
  s3 = g_dbus_connection_signal_subscribe (c1,
                                           "org.freedesktop.DBus",  /* sender */
                                           "org.freedesktop.DBus",  /* interface */
                                           "NameOwnerChanged",      /* member */
                                           "/org/freedesktop/DBus", /* path */
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           test_connection_signal_handler,
                                           &count_name_owner_changed,
                                           NULL);
  /* Note that s1b is *just like* s1 - this is to catch a bug where N
   * subscriptions of the same rule causes N calls to each of the N
   * subscriptions instead of just 1 call to each of the N subscriptions.
   */
  s1b = g_dbus_connection_signal_subscribe (c1,
                                            ":1.2",
                                            "org.gtk.GDBus.ExampleInterface",
                                            "foo_t",
                                            "/org/gtk/GDBus/ExampleInterface",
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            test_connection_signal_handler,
                                            &count_s1b,
                                            NULL);
  g_assert_cmpuint (s1, !=, 0);
  g_assert_cmpuint (s1b, !=, 0);
  g_assert_cmpuint (s2, !=, 0);
  g_assert_cmpuint (s3, !=, 0);

  count_s1 = 0;
  count_s1b = 0;
  count_s2 = 0;
  count_name_owner_changed = 0;

  /*
   * Make c2 emit "foo_t" - we should catch it twice
   *
   * Note that there is no way to be sure that the signal subscriptions
   * on c1 are effective yet - for all we know, the AddMatch() messages
   * could sit waiting in a buffer somewhere between this process and
   * the message bus. And emitting signals on c2 (a completely other
   * socket!) will not necessarily change this.
   *
   * To ensure this is not the case, do a synchronous call on c1.
   */
  result = g_dbus_connection_call_sync (c1,
                                        "org.freedesktop.DBus",  /* bus name */
                                        "/org/freedesktop/DBus", /* object path */
                                        "org.freedesktop.DBus",  /* interface name */
                                        "GetId",                 /* method name */
                                        NULL,                    /* parameters */
                                        NULL,                    /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  xvariant_unref (result);

  /*
   * Bring up two other connections
   */
  c2 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c2);
  g_assert_false (g_dbus_connection_is_closed (c2));
  g_assert_cmpstr (g_dbus_connection_get_unique_name (c2), ==, ":1.2");
  c3 = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, NULL);
  g_assert_nonnull (c3);
  g_assert_false (g_dbus_connection_is_closed (c3));
  g_assert_cmpstr (g_dbus_connection_get_unique_name (c3), ==, ":1.3");

  /* now, emit the signal on c2 */
  ret = g_dbus_connection_emit_signal (c2,
                                       NULL, /* destination bus name */
                                       "/org/gtk/GDBus/ExampleInterface",
                                       "org.gtk.GDBus.ExampleInterface",
                                       "foo_t",
                                       NULL,
                                       &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  while (!(count_s1 >= 1 && count_s2 >= 1))
    xmain_loop_run (loop);
  g_assert_cmpint (count_s1, ==, 1);
  g_assert_cmpint (count_s2, ==, 1);

  /*
   * Make c3 emit "foo_t" - we should catch it only once
   */
  ret = g_dbus_connection_emit_signal (c3,
                                       NULL, /* destination bus name */
                                       "/org/gtk/GDBus/ExampleInterface",
                                       "org.gtk.GDBus.ExampleInterface",
                                       "foo_t",
                                       NULL,
                                       &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  while (!(count_s1 == 1 && count_s2 == 2))
    xmain_loop_run (loop);
  g_assert_cmpint (count_s1, ==, 1);
  g_assert_cmpint (count_s2, ==, 2);

  /*
   * Also to check the total amount of NameOwnerChanged signals - use a 5 second ceiling
   * to avoid spinning forever
   */
  quit_mainloop_fired = FALSE;
  quit_mainloop_id = g_timeout_add (30000, test_connection_quit_mainloop, &quit_mainloop_fired);
  while (count_name_owner_changed < 2 && !quit_mainloop_fired)
    xmain_loop_run (loop);
  xsource_remove (quit_mainloop_id);
  g_assert_cmpint (count_s1, ==, 1);
  g_assert_cmpint (count_s2, ==, 2);
  g_assert_cmpint (count_name_owner_changed, ==, 2);

  g_dbus_connection_signal_unsubscribe (c1, s1);
  g_dbus_connection_signal_unsubscribe (c1, s2);
  g_dbus_connection_signal_unsubscribe (c1, s3);
  g_dbus_connection_signal_unsubscribe (c1, s1b);

  xobject_unref (c1);
  xobject_unref (c2);
  xobject_unref (c3);

  session_bus_down ();
}

static void
test_match_rule (xdbus_connection_t  *connection,
                 GDBusSignalFlags  flags,
                 xchar_t            *arg0_rule,
                 xchar_t            *arg0,
                 xboolean_t          should_match)
{
  xuint_t subscription_ids[2];
  xint_t emissions = 0;
  xint_t matches = 0;
  xerror_t *error = NULL;

  subscription_ids[0] = g_dbus_connection_signal_subscribe (connection,
                                                            NULL, "org.gtk.ExampleInterface", "foo_t", "/",
                                                            NULL,
                                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                                            test_connection_signal_handler,
                                                            &emissions, NULL);
  subscription_ids[1] = g_dbus_connection_signal_subscribe (connection,
                                                            NULL, "org.gtk.ExampleInterface", "foo_t", "/",
                                                            arg0_rule,
                                                            flags,
                                                            test_connection_signal_handler,
                                                            &matches, NULL);
  g_assert_cmpint (subscription_ids[0], !=, 0);
  g_assert_cmpint (subscription_ids[1], !=, 0);

  g_dbus_connection_emit_signal (connection,
                                 NULL, "/", "org.gtk.ExampleInterface",
                                 "foo_t", xvariant_new ("(s)", arg0),
                                 &error);
  g_assert_no_error (error);

  /* synchronously ping a non-existent method to make sure the signals are dispatched */
  g_dbus_connection_call_sync (connection, "org.gtk.ExampleInterface", "/", "org.gtk.ExampleInterface",
                               "Bar", xvariant_new ("()"), G_VARIANT_TYPE_UNIT, G_DBUS_CALL_FLAGS_NONE,
                               -1, NULL, NULL);

  while (xmain_context_iteration (NULL, FALSE))
    ;

  g_assert_cmpint (emissions, ==, 1);
  g_assert_cmpint (matches, ==, should_match ? 1 : 0);

  g_dbus_connection_signal_unsubscribe (connection, subscription_ids[0]);
  g_dbus_connection_signal_unsubscribe (connection, subscription_ids[1]);
}

static void
test_connection_signal_match_rules (void)
{
  xdbus_connection_t *con;

  session_bus_up ();
  con = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_NONE, "foo", "foo", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_NONE, "foo", "bar", FALSE);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org.gtk", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org.gtk.Example", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE, "org.gtk", "org.gtk+", FALSE);

  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/", "/", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/", "", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk/Example", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/", "/org/gtk/Example", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk/", TRUE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk/Example", "/org/gtk", FALSE);
  test_match_rule (con, G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH, "/org/gtk+", "/org/gtk", FALSE);

  xobject_unref (con);
  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

/* Accessed both from the test code and the filter function (in a worker thread)
 * so all accesses must be atomic. */
typedef struct
{
  xasync_queue_t *incoming_queue;  /* (element-type xdbus_message_t) */
  xuint_t num_outgoing;  /* (atomic) */
} FilterData;

/* Runs in a worker thread. */
static xdbus_message_t *
filter_func (xdbus_connection_t *connection,
             xdbus_message_t    *message,
             xboolean_t         incoming,
             xpointer_t         user_data)
{
  FilterData *data = user_data;

  if (incoming)
    g_async_queue_push (data->incoming_queue, xobject_ref (message));
  else
    g_atomic_int_inc (&data->num_outgoing);

  return message;
}

static void
wait_for_filtered_reply (xasync_queue_t *incoming_queue,
                         xuint32_t      expected_serial)
{
  xdbus_message_t *popped_message = NULL;

  while ((popped_message = g_async_queue_pop (incoming_queue)) != NULL)
    {
      xuint32_t reply_serial = xdbus_message_get_reply_serial (popped_message);
      xobject_unref (popped_message);
      if (reply_serial == expected_serial)
        return;
    }

  g_assert_not_reached ();
}

typedef struct
{
  xboolean_t alter_incoming;
  xboolean_t alter_outgoing;
} FilterEffects;

/* Runs in a worker thread. */
static xdbus_message_t *
other_filter_func (xdbus_connection_t *connection,
                   xdbus_message_t    *message,
                   xboolean_t         incoming,
                   xpointer_t         user_data)
{
  const FilterEffects *effects = user_data;
  xdbus_message_t *ret;
  xboolean_t alter;

  if (incoming)
    alter = effects->alter_incoming;
  else
    alter = effects->alter_outgoing;

  if (alter)
    {
      xdbus_message_t *copy;
      xvariant_t *body;
      xchar_t *s;
      xchar_t *s2;

      copy = xdbus_message_copy (message, NULL);
      xobject_unref (message);

      body = xdbus_message_get_body (copy);
      xvariant_get (body, "(s)", &s);
      s2 = xstrdup_printf ("MOD: %s", s);
      xdbus_message_set_body (copy, xvariant_new ("(s)", s2));
      g_free (s2);
      g_free (s);

      ret = copy;
    }
  else
    {
      ret = message;
    }

  return ret;
}

static void
test_connection_filter_name_owner_changed_signal_handler (xdbus_connection_t  *connection,
                                                          const xchar_t      *sender_name,
                                                          const xchar_t      *object_path,
                                                          const xchar_t      *interface_name,
                                                          const xchar_t      *signal_name,
                                                          xvariant_t         *parameters,
                                                          xpointer_t         user_data)
{
  const xchar_t *name;
  const xchar_t *old_owner;
  const xchar_t *new_owner;

  xvariant_get (parameters,
                 "(&s&s&s)",
                 &name,
                 &old_owner,
                 &new_owner);

  if (xstrcmp0 (name, "com.example.TestService") == 0 && strlen (new_owner) > 0)
    {
      xmain_loop_quit (loop);
    }
}

static xboolean_t
test_connection_filter_on_timeout (xpointer_t user_data)
{
  g_printerr ("Timeout waiting 30 sec on service\n");
  g_assert_not_reached ();
  return G_SOURCE_REMOVE;
}

static void
test_connection_filter (void)
{
  xdbus_connection_t *c;
  FilterData data = { NULL, 0 };
  xdbus_message_t *m;
  xdbus_message_t *m2;
  xdbus_message_t *r;
  xerror_t *error;
  xuint_t filter_id;
  xuint_t timeout_mainloop_id;
  xuint_t signal_handler_id;
  FilterEffects effects;
  xvariant_t *result;
  const xchar_t *s;
  xuint32_t serial_temp;

  session_bus_up ();

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);

  data.incoming_queue = g_async_queue_new_full (xobject_unref);
  data.num_outgoing = 0;
  filter_id = g_dbus_connection_add_filter (c,
                                            filter_func,
                                            &data,
                                            NULL);

  m = xdbus_message_new_method_call ("org.freedesktop.DBus", /* name */
                                      "/org/freedesktop/DBus", /* path */
                                      "org.freedesktop.DBus", /* interface */
                                      "GetNameOwner");
  xdbus_message_set_body (m, xvariant_new ("(s)", "org.freedesktop.DBus"));
  error = NULL;
  g_dbus_connection_send_message (c, m, G_DBUS_SEND_MESSAGE_FLAGS_NONE, &serial_temp, &error);
  g_assert_no_error (error);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);

  m2 = xdbus_message_copy (m, &error);
  g_assert_no_error (error);
  g_dbus_connection_send_message (c, m2, G_DBUS_SEND_MESSAGE_FLAGS_NONE, &serial_temp, &error);
  xobject_unref (m2);
  g_assert_no_error (error);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);

  m2 = xdbus_message_copy (m, &error);
  g_assert_no_error (error);
  xdbus_message_set_serial (m2, serial_temp);
  /* lock the message to test PRESERVE_SERIAL flag. */
  xdbus_message_lock (m2);
  g_dbus_connection_send_message (c, m2, G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL, &serial_temp, &error);
  xobject_unref (m2);
  g_assert_no_error (error);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);

  m2 = xdbus_message_copy (m, &error);
  g_assert_no_error (error);
  r = g_dbus_connection_send_message_with_reply_sync (c,
                                                      m2,
                                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                      -1,
                                                      &serial_temp,
                                                      NULL, /* xcancellable_t */
                                                      &error);
  xobject_unref (m2);
  g_assert_no_error (error);
  g_assert_nonnull (r);
  xobject_unref (r);

  wait_for_filtered_reply (data.incoming_queue, serial_temp);
  g_assert_cmpint (g_async_queue_length (data.incoming_queue), ==, 0);

  g_dbus_connection_remove_filter (c, filter_id);

  m2 = xdbus_message_copy (m, &error);
  g_assert_no_error (error);
  r = g_dbus_connection_send_message_with_reply_sync (c,
                                                      m2,
                                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                      -1,
                                                      &serial_temp,
                                                      NULL, /* xcancellable_t */
                                                      &error);
  xobject_unref (m2);
  g_assert_no_error (error);
  g_assert_nonnull (r);
  xobject_unref (r);
  g_assert_cmpint (g_async_queue_length (data.incoming_queue), ==, 0);
  g_assert_cmpint (g_atomic_int_get (&data.num_outgoing), ==, 4);

  /* wait for service to be available */
  signal_handler_id = g_dbus_connection_signal_subscribe (c,
                                                          "org.freedesktop.DBus", /* sender */
                                                          "org.freedesktop.DBus",
                                                          "NameOwnerChanged",
                                                          "/org/freedesktop/DBus",
                                                          NULL, /* arg0 */
                                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                                          test_connection_filter_name_owner_changed_signal_handler,
                                                          NULL,
                                                          NULL);
  g_assert_cmpint (signal_handler_id, !=, 0);

  /* this is safe; testserver will exit once the bus goes away */
  g_assert_true (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  timeout_mainloop_id = g_timeout_add (30000, test_connection_filter_on_timeout, NULL);
  xmain_loop_run (loop);
  xsource_remove (timeout_mainloop_id);
  g_dbus_connection_signal_unsubscribe (c, signal_handler_id);

  /* now test some combinations... */
  filter_id = g_dbus_connection_add_filter (c,
                                            other_filter_func,
                                            &effects,
                                            NULL);
  /* -- */
  effects.alter_incoming = FALSE;
  effects.alter_outgoing = FALSE;
  error = NULL;
  result = g_dbus_connection_call_sync (c,
                                        "com.example.TestService",      /* bus name */
                                        "/com/example/test_object_t",      /* object path */
                                        "com.example.Frob",             /* interface name */
                                        "HelloWorld",                   /* method name */
                                        xvariant_new ("(s)", "Cat"),   /* parameters */
                                        G_VARIANT_TYPE ("(s)"),         /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  xvariant_get (result, "(&s)", &s);
  g_assert_cmpstr (s, ==, "You greeted me with 'Cat'. Thanks!");
  xvariant_unref (result);
  /* -- */
  effects.alter_incoming = TRUE;
  effects.alter_outgoing = TRUE;
  error = NULL;
  result = g_dbus_connection_call_sync (c,
                                        "com.example.TestService",      /* bus name */
                                        "/com/example/test_object_t",      /* object path */
                                        "com.example.Frob",             /* interface name */
                                        "HelloWorld",                   /* method name */
                                        xvariant_new ("(s)", "Cat"),   /* parameters */
                                        G_VARIANT_TYPE ("(s)"),         /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
  g_assert_no_error (error);
  xvariant_get (result, "(&s)", &s);
  g_assert_cmpstr (s, ==, "MOD: You greeted me with 'MOD: Cat'. Thanks!");
  xvariant_unref (result);


  g_dbus_connection_remove_filter (c, filter_id);

  xobject_unref (c);
  xobject_unref (m);
  g_async_queue_unref (data.incoming_queue);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

#define NUM_THREADS 50

static void
send_bogus_message (xdbus_connection_t *c, xuint32_t *out_serial)
{
  xdbus_message_t *m;
  xerror_t *error;

  m = xdbus_message_new_method_call ("org.freedesktop.DBus", /* name */
                                      "/org/freedesktop/DBus", /* path */
                                      "org.freedesktop.DBus", /* interface */
                                      "GetNameOwner");
  xdbus_message_set_body (m, xvariant_new ("(s)", "org.freedesktop.DBus"));
  error = NULL;
  g_dbus_connection_send_message (c, m, G_DBUS_SEND_MESSAGE_FLAGS_NONE, out_serial, &error);
  g_assert_no_error (error);
  xobject_unref (m);
}

#define SLEEP_USEC (100 * 1000)

static xpointer_t
serials_thread_func (xdbus_connection_t *c)
{
  xuint32_t message_serial;
  xuint_t i;

  /* No calls on this thread yet */
  g_assert_cmpint (g_dbus_connection_get_last_serial(c), ==, 0);

  /* Send a bogus message and store its serial */
  message_serial = 0;
  send_bogus_message (c, &message_serial);

  /* Give it some time to actually send the message out. 10 seconds
   * should be plenty, even on slow machines. */
  for (i = 0; i < 10 * G_USEC_PER_SEC / SLEEP_USEC; i++)
    {
      if (g_dbus_connection_get_last_serial(c) != 0)
        break;

      g_usleep (SLEEP_USEC);
    }

  g_assert_cmpint (g_dbus_connection_get_last_serial(c), !=, 0);
  g_assert_cmpint (g_dbus_connection_get_last_serial(c), ==, message_serial);

  return NULL;
}

static void
test_connection_serials (void)
{
  xdbus_connection_t *c;
  xerror_t *error;
  xthread_t *pool[NUM_THREADS];
  int i;

  session_bus_up ();

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);

  /* Status after initialization */
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 1);

  /* Send a bogus message */
  send_bogus_message (c, NULL);
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 2);

  /* Start the threads */
  for (i = 0; i < NUM_THREADS; i++)
    pool[i] = xthread_new (NULL, (GThreadFunc) serials_thread_func, c);

  /* Wait until threads are finished */
  for (i = 0; i < NUM_THREADS; i++)
      xthread_join (pool[i]);

  /* No calls in between on this thread, should be the last value */
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 2);

  send_bogus_message (c, NULL);

  /* All above calls + calls in threads */
  g_assert_cmpint (g_dbus_connection_get_last_serial (c), ==, 3 + NUM_THREADS);

  xobject_unref (c);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_connection_basic (void)
{
  xdbus_connection_t *connection;
  xerror_t *error;
  GDBusCapabilityFlags flags;
  GDBusConnectionFlags connection_flags;
  xchar_t *guid;
  xchar_t *name;
  xboolean_t closed;
  xboolean_t exit_on_close;
  xio_stream_t *stream;
  xcredentials_t *credentials;

  session_bus_up ();

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (connection);

  flags = g_dbus_connection_get_capabilities (connection);
  g_assert_true (flags == G_DBUS_CAPABILITY_FLAGS_NONE ||
                 flags == G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);

  connection_flags = g_dbus_connection_get_flags (connection);
  g_assert_cmpint (connection_flags, ==,
                   G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION);

  credentials = g_dbus_connection_get_peer_credentials (connection);
  g_assert_null (credentials);

  xobject_get (connection,
                "stream", &stream,
                "guid", &guid,
                "unique-name", &name,
                "closed", &closed,
                "exit-on-close", &exit_on_close,
                "capabilities", &flags,
                NULL);

  g_assert_true (X_IS_IO_STREAM (stream));
  g_assert_true (g_dbus_is_guid (guid));
  g_assert_true (g_dbus_is_unique_name (name));
  g_assert_false (closed);
  g_assert_true (exit_on_close);
  g_assert_true (flags == G_DBUS_CAPABILITY_FLAGS_NONE ||
                 flags == G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);
  xobject_unref (stream);
  g_free (name);
  g_free (guid);

  xobject_unref (connection);

  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  int ret;

  g_test_init (&argc, &argv, NULL);

  /* all the tests rely on a shared main loop */
  loop = xmain_loop_new (NULL, FALSE);

  g_test_dbus_unset ();

  /* gdbus cleanup is pretty racy due to worker threads, so always do this test first */
  g_test_add_func ("/gdbus/connection/bus-failure", test_connection_bus_failure);

  g_test_add_func ("/gdbus/connection/basic", test_connection_basic);
  g_test_add_func ("/gdbus/connection/life-cycle", test_connection_life_cycle);
  g_test_add_func ("/gdbus/connection/send", test_connection_send);
  g_test_add_func ("/gdbus/connection/signals", test_connection_signals);
  g_test_add_func ("/gdbus/connection/signal-match-rules", test_connection_signal_match_rules);
  g_test_add_func ("/gdbus/connection/filter", test_connection_filter);
  g_test_add_func ("/gdbus/connection/serials", test_connection_serials);
  ret = g_test_run();

  xmain_loop_unref (loop);
  return ret;
}
