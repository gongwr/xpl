/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#include <locale.h>
#include <gio/gio.h>

#include <string.h>
#include <unistd.h>

#include "gdbus-tests.h"

#ifdef G_OS_UNIX
#include <gio/gunixconnection.h>
#include <gio/gnetworkingprivate.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixfdlist.h>
#endif

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
server_on_allow_mechanism (xdbus_auth_observer_t *observer,
                           const xchar_t       *mechanism,
                           xpointer_t           user_data)
{
  const xchar_t *allowed_mechanism = user_data;
  if (allowed_mechanism == NULL || xstrcmp0 (mechanism, allowed_mechanism) == 0)
    return TRUE;
  else
    return FALSE;
}

/* pass NULL to allow any mechanism */
static xdbus_server_t *
server_new_for_mechanism (const xchar_t *allowed_mechanism)
{
  xchar_t *addr;
  xchar_t *guid;
  xdbus_server_t *server;
  xdbus_auth_observer_t *auth_observer;
  xerror_t *error;
  xdbus_server_flags_t flags;

  guid = g_dbus_generate_guid ();

#ifdef G_OS_UNIX
  if (g_unix_socket_address_abstract_names_supported ())
    {
      addr = xstrdup ("unix:tmpdir=/tmp/gdbus-test-");
    }
  else
    {
      xchar_t *tmpdir;
      tmpdir = g_dir_make_tmp ("gdbus-test-XXXXXX", NULL);
      addr = xstrdup_printf ("unix:tmpdir=%s", tmpdir);
      g_free (tmpdir);
    }
#else
  addr = xstrdup ("nonce-tcp:");
#endif

  auth_observer = xdbus_auth_observer_new ();

  flags = G_DBUS_SERVER_FLAGS_NONE;
  if (xstrcmp0 (allowed_mechanism, "ANONYMOUS") == 0)
    flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;

  error = NULL;
  server = xdbus_server_new_sync (addr,
                                   flags,
                                   guid,
                                   auth_observer,
                                   NULL, /* cancellable */
                                   &error);
  g_assert_no_error (error);
  xassert (server != NULL);

  xsignal_connect (auth_observer,
                    "allow-mechanism",
                    G_CALLBACK (server_on_allow_mechanism),
                    (xpointer_t) allowed_mechanism);

  g_free (addr);
  g_free (guid);
  xobject_unref (auth_observer);

  return server;
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
test_auth_on_new_connection (xdbus_server_t     *server,
                             xdbus_connection_t *connection,
                             xpointer_t         user_data)
{
  xmain_loop_t *loop = user_data;
  xmain_loop_quit (loop);
  return FALSE;
}

static xboolean_t
test_auth_on_timeout (xpointer_t user_data)
{
  xerror ("Timeout waiting for client");
  g_assert_not_reached ();
  return XSOURCE_REMOVE;
}


typedef struct
{
  const xchar_t *address;
  const xchar_t *allowed_client_mechanism;
  const xchar_t *allowed_server_mechanism;
} TestAuthData;

static xpointer_t
test_auth_client_thread_func (xpointer_t user_data)
{
  TestAuthData *data = user_data;
  xdbus_connection_t *c = NULL;
  xerror_t *error = NULL;
  xdbus_auth_observer_t *auth_observer = NULL;

  auth_observer = xdbus_auth_observer_new ();

  xsignal_connect (auth_observer,
                    "allow-mechanism",
                    G_CALLBACK (server_on_allow_mechanism),
                    (xpointer_t) data->allowed_client_mechanism);

  c = xdbus_connection_new_for_address_sync (data->address,
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              auth_observer,
                                              NULL, /* xcancellable_t */
                                              &error);
  g_assert_no_error (error);
  xassert (c != NULL);
  g_clear_object (&c);
  g_clear_object (&auth_observer);
  return NULL;
}

static void
test_auth_mechanism (const xchar_t *allowed_client_mechanism,
                     const xchar_t *allowed_server_mechanism)
{
  xdbus_server_t *server;
  xmain_loop_t *loop;
  xthread_t *client_thread;
  TestAuthData data;
  xuint_t timeout_id;

  server = server_new_for_mechanism (allowed_server_mechanism);

  loop = xmain_loop_new (NULL, FALSE);

  xsignal_connect (server,
                    "new-connection",
                    G_CALLBACK (test_auth_on_new_connection),
                    loop);

  timeout_id = g_timeout_add_seconds (5, test_auth_on_timeout, NULL);

  data.allowed_client_mechanism = allowed_client_mechanism;
  data.allowed_server_mechanism = allowed_server_mechanism;
  data.address = xdbus_server_get_client_address (server);

  /* run the D-Bus client in a thread */
  client_thread = xthread_new ("gdbus-client-thread",
                                test_auth_client_thread_func,
                                &data);

  xdbus_server_start (server);

  xmain_loop_run (loop);

  xdbus_server_stop (server);

  xthread_join (client_thread);
  xsource_remove (timeout_id);

  while (xmain_context_iteration (NULL, FALSE));
  xmain_loop_unref (loop);

  xobject_unref (server);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
auth_client_external (void)
{
  test_auth_mechanism ("EXTERNAL", NULL);
}

static void
auth_client_dbus_cookie_sha1 (void)
{
  test_auth_mechanism ("DBUS_COOKIE_SHA1", NULL);
}

static void
auth_server_anonymous (void)
{
  test_auth_mechanism (NULL, "ANONYMOUS");
}

static void
auth_server_external (void)
{
  test_auth_mechanism (NULL, "EXTERNAL");
}

static void
auth_server_dbus_cookie_sha1 (void)
{
  test_auth_mechanism (NULL, "DBUS_COOKIE_SHA1");
}

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t *temp_dbus_keyrings_dir = NULL;

static void
temp_dbus_keyrings_setup (void)
{
  xerror_t *error = NULL;

  xassert (temp_dbus_keyrings_dir == NULL);
  temp_dbus_keyrings_dir = g_dir_make_tmp ("gdbus-test-dbus-keyrings-XXXXXX", &error);
  g_assert_no_error (error);
  xassert (temp_dbus_keyrings_dir != NULL);
  g_setenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR", temp_dbus_keyrings_dir, TRUE);
  g_setenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR_IGNORE_PERMISSION", "1", TRUE);
}

static void
temp_dbus_keyrings_teardown (void)
{
  xdir_t *dir;
  xerror_t *error = NULL;
  const xchar_t *name;

  xassert (temp_dbus_keyrings_dir != NULL);

  dir = g_dir_open (temp_dbus_keyrings_dir, 0, &error);
  g_assert_no_error (error);
  xassert (dir != NULL);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      xchar_t *path = g_build_filename (temp_dbus_keyrings_dir, name, NULL);
      xassert (unlink (path) == 0);
      g_free (path);
    }
  g_dir_close (dir);
  xassert (rmdir (temp_dbus_keyrings_dir) == 0);

  g_free (temp_dbus_keyrings_dir);
  temp_dbus_keyrings_dir = NULL;
  g_unsetenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR");
  g_unsetenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR_IGNORE_PERMISSION");
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  xint_t ret;

  setlocale (LC_ALL, "C");

  temp_dbus_keyrings_setup ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gdbus/auth/client/EXTERNAL",         auth_client_external);
  g_test_add_func ("/gdbus/auth/client/DBUS_COOKIE_SHA1", auth_client_dbus_cookie_sha1);
  g_test_add_func ("/gdbus/auth/server/ANONYMOUS",        auth_server_anonymous);
  g_test_add_func ("/gdbus/auth/server/EXTERNAL",         auth_server_external);
  g_test_add_func ("/gdbus/auth/server/DBUS_COOKIE_SHA1", auth_server_dbus_cookie_sha1);

  /* TODO: we currently don't have tests for
   *
   *  - DBUS_COOKIE_SHA1 timeouts (and clock changes etc)
   *  - interoperability with libdbus-1 implementations of authentication methods (both client and server)
   */

  ret = g_test_run();

  temp_dbus_keyrings_teardown ();

  return ret;
}
