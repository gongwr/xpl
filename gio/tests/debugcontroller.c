/* GLib testing framework examples and tests
 *
 * Copyright © 2022 Endless OS Foundation, LLC
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Author: Philip Withnall <pwithnall@endlessos.org>
 */

#include <gio/gio.h>
#include <locale.h>


static void
test_dbus_basic (void)
{
  xtest_dbus_t *bus;
  xdbus_connection_t *connection = NULL, *connection2 = NULL;
  GDebugControllerDBus *controller = NULL;
  xboolean_t old_value;
  xboolean_t debug_enabled;
  xerror_t *local_error = NULL;

  g_test_summary ("Smoketest for construction and setting of a #GDebugControllerDBus.");

  /* Set up a test session bus and connection. */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &local_error);
  g_assert_no_error (local_error);

  /* Create a controller for this process. */
  controller = g_debug_controller_dbus_new (connection, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (controller);
  g_assert_true (X_IS_DEBUG_CONTROLLER_DBUS (controller));

  /* Try enabling and disabling debug output from within the process. */
  old_value = g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller));

  g_debug_controller_set_debug_enabled (G_DEBUG_CONTROLLER (controller), TRUE);
  g_assert_true (g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller)));

  g_debug_controller_set_debug_enabled (G_DEBUG_CONTROLLER (controller), FALSE);
  g_assert_false (g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller)));

  /* Reset the debug state and check using xobject_get(), to exercise that. */
  g_debug_controller_set_debug_enabled (G_DEBUG_CONTROLLER (controller), old_value);

  xobject_get (G_OBJECT (controller),
                "debug-enabled", &debug_enabled,
                "connection", &connection2,
                NULL);
  g_assert_true (debug_enabled == old_value);
  g_assert_true (connection2 == connection);
  g_clear_object (&connection2);

  g_debug_controller_dbus_stop (controller);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_finalize_object (controller);
  g_clear_object (&connection);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

static void
test_dbus_duplicate (void)
{
  xtest_dbus_t *bus;
  xdbus_connection_t *connection = NULL;
  GDebugControllerDBus *controller1 = NULL, *controller2 = NULL;
  xerror_t *local_error = NULL;

  g_test_summary ("Test that creating a second #GDebugControllerDBus on the same D-Bus connection fails.");

  /* Set up a test session bus and connection. */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &local_error);
  g_assert_no_error (local_error);

  /* Create a controller for this process. */
  controller1 = g_debug_controller_dbus_new (connection, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (controller1);

  /* And try creating a second one. */
  controller2 = g_debug_controller_dbus_new (connection, NULL, &local_error);
  g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS);
  g_assert_null (controller2);
  g_clear_error (&local_error);

  g_debug_controller_dbus_stop (controller1);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_finalize_object (controller1);
  g_clear_object (&connection);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

static void
async_result_cb (xobject_t      *source_object,
                 xasync_result_t *result,
                 xpointer_t      user_data)
{
  xasync_result_t **result_out = user_data;

  g_assert_null (*result_out);
  *result_out = xobject_ref (result);

  xmain_context_wakeup (xmain_context_get_thread_default ());
}

static xboolean_t
authorize_false_cb (GDebugControllerDBus  *debug_controller,
                    xdbus_method_invocation_t *invocation,
                    xpointer_t               user_data)
{
  return FALSE;
}

static xboolean_t
authorize_true_cb (GDebugControllerDBus  *debug_controller,
                   xdbus_method_invocation_t *invocation,
                   xpointer_t               user_data)
{
  return TRUE;
}

static void
notify_debug_enabled_cb (xobject_t    *object,
                         xparam_spec_t *pspec,
                         xpointer_t    user_data)
{
  xuint_t *notify_count_out = user_data;

  *notify_count_out = *notify_count_out + 1;
}

static void
properties_changed_cb (xdbus_connection_t *connection,
                       const xchar_t     *sender_name,
                       const xchar_t     *object_path,
                       const xchar_t     *interface_name,
                       const xchar_t     *signal_name,
                       xvariant_t        *parameters,
                       xpointer_t         user_data)
{
  xuint_t *properties_changed_count_out = user_data;

  *properties_changed_count_out = *properties_changed_count_out + 1;
  xmain_context_wakeup (xmain_context_get_thread_default ());
}

static void
test_dbus_properties (void)
{
  xtest_dbus_t *bus;
  xdbus_connection_t *controller_connection = NULL;
  xdbus_connection_t *remote_connection = NULL;
  GDebugControllerDBus *controller = NULL;
  xboolean_t old_value;
  xasync_result_t *result = NULL;
  xvariant_t *reply = NULL;
  xvariant_t *debug_enabled_variant = NULL;
  xboolean_t debug_enabled;
  xerror_t *local_error = NULL;
  gulong handler_id;
  gulong notify_id;
  xuint_t notify_count = 0;
  xuint_t properties_changed_id;
  xuint_t properties_changed_count = 0;

  g_test_summary ("Test getting and setting properties on a #GDebugControllerDBus.");

  /* Set up a test session bus and connection. Set up a separate second
   * connection to simulate a remote peer. */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  controller_connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &local_error);
  g_assert_no_error (local_error);

  remote_connection = g_dbus_connection_new_for_address_sync (g_test_dbus_get_bus_address (bus),
                                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                              G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                              NULL,
                                                              NULL,
                                                              &local_error);
  g_assert_no_error (local_error);

  /* Create a controller for this process. */
  controller = g_debug_controller_dbus_new (controller_connection, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (controller);
  g_assert_true (X_IS_DEBUG_CONTROLLER_DBUS (controller));

  old_value = g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller));
  notify_id = g_signal_connect (controller, "notify::debug-enabled", G_CALLBACK (notify_debug_enabled_cb), &notify_count);

  properties_changed_id = g_dbus_connection_signal_subscribe (remote_connection,
                                                              g_dbus_connection_get_unique_name (controller_connection),
                                                              "org.freedesktop.DBus.Properties",
                                                              "PropertiesChanged",
                                                              "/org/gtk/Debugging",
                                                              NULL,
                                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                                              properties_changed_cb,
                                                              &properties_changed_count,
                                                              NULL);

  /* Get the debug status remotely. */
  g_dbus_connection_call (remote_connection,
                          g_dbus_connection_get_unique_name (controller_connection),
                          "/org/gtk/Debugging",
                          "org.freedesktop.DBus.Properties",
                          "Get",
                          xvariant_new ("(ss)", "org.gtk.Debugging", "DebugEnabled"),
                          G_VARIANT_TYPE ("(v)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          async_result_cb,
                          &result);
  g_assert_no_error (local_error);

  while (result == NULL)
    xmain_context_iteration (NULL, TRUE);

  reply = g_dbus_connection_call_finish (remote_connection, result, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&result);

  xvariant_get (reply, "(v)", &debug_enabled_variant);
  debug_enabled = xvariant_get_boolean (debug_enabled_variant);
  g_assert_true (debug_enabled == old_value);
  g_assert_cmpuint (notify_count, ==, 0);
  g_assert_cmpuint (properties_changed_count, ==, 0);

  g_clear_pointer (&debug_enabled_variant, xvariant_unref);
  g_clear_pointer (&reply, xvariant_unref);

  /* Set the debug status remotely. The first attempt should fail due to no
   * authorisation handler being connected. The second should fail due to the
   * now-connected handler returning %FALSE. The third attempt should
   * succeed. */
  g_dbus_connection_call (remote_connection,
                          g_dbus_connection_get_unique_name (controller_connection),
                          "/org/gtk/Debugging",
                          "org.gtk.Debugging",
                          "SetDebugEnabled",
                          xvariant_new ("(b)", !old_value),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          async_result_cb,
                          &result);

  while (result == NULL)
    xmain_context_iteration (NULL, TRUE);

  reply = g_dbus_connection_call_finish (remote_connection, result, &local_error);
  g_assert_error (local_error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED);
  g_clear_object (&result);
  g_clear_error (&local_error);

  g_assert_true (g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller)) == old_value);
  g_assert_cmpuint (notify_count, ==, 0);
  g_assert_cmpuint (properties_changed_count, ==, 0);

  g_clear_pointer (&debug_enabled_variant, xvariant_unref);
  g_clear_pointer (&reply, xvariant_unref);

  /* Attach an authorisation handler and try again. */
  handler_id = g_signal_connect (controller, "authorize", G_CALLBACK (authorize_false_cb), NULL);

  g_dbus_connection_call (remote_connection,
                          g_dbus_connection_get_unique_name (controller_connection),
                          "/org/gtk/Debugging",
                          "org.gtk.Debugging",
                          "SetDebugEnabled",
                          xvariant_new ("(b)", !old_value),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          async_result_cb,
                          &result);

  while (result == NULL)
    xmain_context_iteration (NULL, TRUE);

  reply = g_dbus_connection_call_finish (remote_connection, result, &local_error);
  g_assert_error (local_error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED);
  g_clear_object (&result);
  g_clear_error (&local_error);

  g_assert_true (g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller)) == old_value);
  g_assert_cmpuint (notify_count, ==, 0);
  g_assert_cmpuint (properties_changed_count, ==, 0);

  g_clear_pointer (&debug_enabled_variant, xvariant_unref);
  g_clear_pointer (&reply, xvariant_unref);

  g_signal_handler_disconnect (controller, handler_id);
  handler_id = 0;

  /* Attach another signal handler which will grant access, and try again. */
  handler_id = g_signal_connect (controller, "authorize", G_CALLBACK (authorize_true_cb), NULL);

  g_dbus_connection_call (remote_connection,
                          g_dbus_connection_get_unique_name (controller_connection),
                          "/org/gtk/Debugging",
                          "org.gtk.Debugging",
                          "SetDebugEnabled",
                          xvariant_new ("(b)", !old_value),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          async_result_cb,
                          &result);

  while (result == NULL)
    xmain_context_iteration (NULL, TRUE);

  reply = g_dbus_connection_call_finish (remote_connection, result, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&result);

  g_assert_true (g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller)) == !old_value);
  g_assert_cmpuint (notify_count, ==, 1);
  g_assert_cmpuint (properties_changed_count, ==, 1);

  g_clear_pointer (&debug_enabled_variant, xvariant_unref);
  g_clear_pointer (&reply, xvariant_unref);

  g_signal_handler_disconnect (controller, handler_id);
  handler_id = 0;

  /* Set the debug status locally. */
  g_debug_controller_set_debug_enabled (G_DEBUG_CONTROLLER (controller), old_value);
  g_assert_true (g_debug_controller_get_debug_enabled (G_DEBUG_CONTROLLER (controller)) == old_value);
  g_assert_cmpuint (notify_count, ==, 2);

  while (properties_changed_count != 2)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (properties_changed_count, ==, 2);

  g_signal_handler_disconnect (controller, notify_id);
  notify_id = 0;

  g_dbus_connection_signal_unsubscribe (remote_connection, properties_changed_id);
  properties_changed_id = 0;

  g_debug_controller_dbus_stop (controller);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_finalize_object (controller);
  g_clear_object (&controller_connection);
  g_clear_object (&remote_connection);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/debug-controller/dbus/basic", test_dbus_basic);
  g_test_add_func ("/debug-controller/dbus/duplicate", test_dbus_duplicate);
  g_test_add_func ("/debug-controller/dbus/properties", test_dbus_properties);

  return g_test_run ();
}
