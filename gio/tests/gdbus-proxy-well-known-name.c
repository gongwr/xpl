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

/* all tests rely on a shared mainloop */
static xmain_loop_t *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */

static void
proxy_new_cb (xobject_t       *source_object,
              xasync_result_t  *res,
              xpointer_t       user_data)
{
  xdbus_proxy_t **ret = user_data;
  xerror_t *error;

  error = NULL;
  *ret = xdbus_proxy_new_finish (res, &error);
  g_assert_no_error (error);
  g_assert (ret != NULL);

  xmain_loop_quit (loop);
}

static void
test_proxy_well_known_name (void)
{
  xdbus_proxy_t *p;
  xdbus_proxy_t *p2;
  xdbus_proxy_t *ap;
  xdbus_proxy_t *ap2;
  xdbus_connection_t *c;
  xerror_t *error;
  xchar_t *name_owner;
  xchar_t **property_names;
  xvariant_t *variant;
  xvariant_t *result;

  session_bus_up ();

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (c != NULL);

  error = NULL;
  p = xdbus_proxy_new_sync (c,
                             G_DBUS_PROXY_FLAGS_NONE,
                             NULL,                      /* xdbus_interface_info_t* */
                             "com.example.TestService", /* name */
                             "/com/example/test_object_t", /* object path */
                             "com.example.Frob",        /* interface name */
                             NULL,                      /* xcancellable_t */
                             &error);
  g_assert_no_error (error);

  /* we shouldn't have a name owner nor any cached properties */
  g_assert_cmpstr (xdbus_proxy_get_name_owner (p), ==, NULL);
  g_assert (xdbus_proxy_get_cached_property_names (p) == NULL);

  /* also for async: we shouldn't have a name owner nor any cached properties */
  xdbus_proxy_new (c,
                    G_DBUS_PROXY_FLAGS_NONE,
                    NULL,                      /* xdbus_interface_info_t* */
                    "com.example.TestService", /* name */
                    "/com/example/test_object_t", /* object path */
                    "com.example.Frob",        /* interface name */
                    NULL,                      /* xcancellable_t */
                    (xasync_ready_callback_t) proxy_new_cb,
                    &ap);
  xmain_loop_run (loop);
  g_assert_cmpstr (xdbus_proxy_get_name_owner (ap), ==, NULL);
  g_assert (xdbus_proxy_get_cached_property_names (ap) == NULL);

  /* this is safe; testserver will exit once the bus goes away */
  g_assert (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  /* check that we get the notify::g-name-owner signal */
  _g_assert_property_notify (p, "g-name-owner");

  /* Now we should have a name owner as well as properties */
  name_owner = xdbus_proxy_get_name_owner (p);
  property_names = xdbus_proxy_get_cached_property_names (p);
  g_assert (g_dbus_is_unique_name (name_owner));
  g_assert (property_names != NULL && xstrv_length (property_names) > 0);
  g_free (name_owner);
  xstrfreev (property_names);

  /* if we create another proxy with the service being available, check that
   * it has a name owner and properties
   */
  error = NULL;
  p2 = xdbus_proxy_new_sync (c,
                              G_DBUS_PROXY_FLAGS_NONE,
                              NULL,                      /* xdbus_interface_info_t* */
                              "com.example.TestService", /* name */
                              "/com/example/test_object_t", /* object path */
                              "com.example.Frob",        /* interface name */
                              NULL,                      /* xcancellable_t */
                              &error);
  g_assert_no_error (error);
  name_owner = xdbus_proxy_get_name_owner (p2);
  property_names = xdbus_proxy_get_cached_property_names (p2);
  g_assert (g_dbus_is_unique_name (name_owner));
  g_assert (property_names != NULL && xstrv_length (property_names) > 0);
  g_free (name_owner);
  xstrfreev (property_names);

  /* also for async: we should have a name owner and cached properties */
  xdbus_proxy_new (c,
                    G_DBUS_PROXY_FLAGS_NONE,
                    NULL,                      /* xdbus_interface_info_t* */
                    "com.example.TestService", /* name */
                    "/com/example/test_object_t", /* object path */
                    "com.example.Frob",        /* interface name */
                    NULL,                      /* xcancellable_t */
                    (xasync_ready_callback_t) proxy_new_cb,
                    &ap2);
  xmain_loop_run (loop);
  name_owner = xdbus_proxy_get_name_owner (ap2);
  property_names = xdbus_proxy_get_cached_property_names (ap2);
  g_assert (g_dbus_is_unique_name (name_owner));
  g_assert (property_names != NULL && xstrv_length (property_names) > 0);
  g_free (name_owner);
  xstrfreev (property_names);

  /* Check property value is the initial value */
  variant = xdbus_proxy_get_cached_property (p, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 1);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (p2, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 1);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (ap, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 1);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (ap2, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 1);
  xvariant_unref (variant);

  /* Check that properties are updated on both p and p2 */
  result = xdbus_proxy_call_sync (p,
                                   "FrobSetProperty",
                                   xvariant_new ("(sv)",
                                                  "y",
                                                  xvariant_new_byte (42)),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);
  _g_assert_signal_received (p, "g-properties-changed");
  variant = xdbus_proxy_get_cached_property (p, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 42);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (p2, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 42);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (ap, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 42);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (ap2, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 42);
  xvariant_unref (variant);

  /* Nuke the service and check that we get the signal and then don't
   * have a name owner nor any cached properties
   */
  result = xdbus_proxy_call_sync (p,
                                   "Quit",
                                   NULL,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);
  /* and wait... */
  _g_assert_property_notify (p, "g-name-owner");
  /* now we shouldn't have a name owner nor any cached properties */
  g_assert_cmpstr (xdbus_proxy_get_name_owner (p), ==, NULL);
  g_assert (xdbus_proxy_get_cached_property_names (p) == NULL);
  g_assert (xdbus_proxy_get_cached_property (p, "y") == NULL);

  /* now bring back the server and wait for the proxy to be updated.. now
   * the 'y' property should be back at 1...
   */
  /* this is safe; testserver will exit once the bus goes away */
  g_assert (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  /* check that we get the notify::g-name-owner signal */
  _g_assert_property_notify (p, "g-name-owner");
  /* Now we should have a name owner as well as properties */
  name_owner = xdbus_proxy_get_name_owner (p);
  property_names = xdbus_proxy_get_cached_property_names (p);
  g_assert (g_dbus_is_unique_name (name_owner));
  g_assert (property_names != NULL && xstrv_length (property_names) > 0);
  g_free (name_owner);
  xstrfreev (property_names);
  /* and finally check the 'y' property */
  variant = xdbus_proxy_get_cached_property (p, "y");
  g_assert (variant != NULL);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 1);
  xvariant_unref (variant);

  xobject_unref (p2);
  xobject_unref (p);
  xobject_unref (ap2);
  xobject_unref (ap);

  xobject_unref (c);

  /* tear down bus */
  session_bus_down ();
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  xint_t ret;

  g_test_init (&argc, &argv, NULL);

  /* all the tests rely on a shared main loop */
  loop = xmain_loop_new (NULL, FALSE);

  g_test_dbus_unset ();

  g_test_add_func ("/gdbus/proxy-well-known-name", test_proxy_well_known_name);

  ret = g_test_run();
  return ret;
}
