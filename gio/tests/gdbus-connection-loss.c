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

/* all tests rely on a shared mainloop */
static xmain_loop_t *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */
/* Check that pending calls fail with G_IO_ERROR_CLOSED if the connection is closed  */
/* See https://bugzilla.gnome.org/show_bug.cgi?id=660637 */
/* ---------------------------------------------------------------------------------------------------- */

static void
sleep_cb (xobject_t      *source_object,
          xasync_result_t *res,
          xpointer_t      user_data)
{
  xerror_t **error = user_data;
  xvariant_t *result;

  result = xdbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, error);
  xassert (result == NULL);
  xmain_loop_quit (loop);
}

static xboolean_t
on_timeout (xpointer_t user_data)
{
  /* tear down bus */
  session_bus_stop ();
  return XSOURCE_REMOVE;
}

static void
test_connection_loss (void)
{
  xdbus_proxy_t *proxy;
  xerror_t *error;

  error = NULL;
  proxy = xdbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,                      /* xdbus_interface_info_t */
                                 "com.example.TestService", /* name */
                                 "/com/example/test_object_t", /* object path */
                                 "com.example.Frob",        /* interface */
                                 NULL, /* xcancellable_t */
                                 &error);
  g_assert_no_error (error);

  error = NULL;
  xdbus_proxy_call (proxy,
                     "Sleep",
                     xvariant_new ("(i)", 100 * 1000 /* msec */),
                     G_DBUS_CALL_FLAGS_NONE,
                     10 * 1000 /* msec */,
                     NULL, /* xcancellable_t */
                     sleep_cb,
                     &error);

  /* Make sure we don't exit when the bus goes away */
  xdbus_connection_set_exit_on_close (c, FALSE);

  /* Nuke the connection to the bus */
  g_timeout_add (100 /* ms */, on_timeout, NULL);

  xmain_loop_run (loop);

  /* If we didn't act on connection-loss we'd be getting G_IO_ERROR_TIMEOUT
   * generated locally. So if we get G_IO_ERROR_CLOSED it means that we
   * are acting correctly on connection loss.
   */
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  xassert (!g_dbus_error_is_remote_error (error));
  g_clear_error (&error);

  xobject_unref (proxy);
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

  /* all the tests rely on a shared main loop */
  loop = xmain_loop_new (NULL, FALSE);

  session_bus_up ();

  /* this is safe; testserver will exit once the bus goes away */
  path = g_test_build_filename (G_TEST_BUILT, "gdbus-testserver", NULL);
  xassert (g_spawn_command_line_async (path, NULL));
  g_free (path);

  /* Create the connection in the main thread */
  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  xassert (c != NULL);

  ensure_gdbus_testserver_up (c, NULL);

  g_test_add_func ("/gdbus/connection-loss", test_connection_loss);

  ret = g_test_run();

  xobject_unref (c);

  session_bus_down ();

  xmain_loop_unref (loop);

  return ret;
}
