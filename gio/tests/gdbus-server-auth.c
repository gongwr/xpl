/*
 * Copyright 2019 Collabora Ltd.
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

#include "config.h"

#include <errno.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

/* For G_CREDENTIALS_*_SUPPORTED */
#include <gio/gcredentialsprivate.h>

#ifdef HAVE_DBUS1
#include <dbus/dbus.h>
#endif

typedef enum
{
  INTEROP_FLAGS_EXTERNAL = (1 << 0),
  INTEROP_FLAGS_ANONYMOUS = (1 << 1),
  INTEROP_FLAGS_SHA1 = (1 << 2),
  INTEROP_FLAGS_TCP = (1 << 3),
  INTEROP_FLAGS_LIBDBUS = (1 << 4),
  INTEROP_FLAGS_ABSTRACT = (1 << 5),
  INTEROP_FLAGS_REQUIRE_SAME_USER = (1 << 6),
  INTEROP_FLAGS_NONE = 0
} InteropFlags;

static xboolean_t
allow_external_cb (G_GNUC_UNUSED xdbus_auth_observer_t *observer,
                   const char *mechanism,
                   G_GNUC_UNUSED xpointer_t user_data)
{
  if (xstrcmp0 (mechanism, "EXTERNAL") == 0)
    {
      g_debug ("Accepting EXTERNAL authentication");
      return TRUE;
    }
  else
    {
      g_debug ("Rejecting \"%s\" authentication: not EXTERNAL", mechanism);
      return FALSE;
    }
}

static xboolean_t
allow_anonymous_cb (G_GNUC_UNUSED xdbus_auth_observer_t *observer,
                    const char *mechanism,
                    G_GNUC_UNUSED xpointer_t user_data)
{
  if (xstrcmp0 (mechanism, "ANONYMOUS") == 0)
    {
      g_debug ("Accepting ANONYMOUS authentication");
      return TRUE;
    }
  else
    {
      g_debug ("Rejecting \"%s\" authentication: not ANONYMOUS", mechanism);
      return FALSE;
    }
}

static xboolean_t
allow_sha1_cb (G_GNUC_UNUSED xdbus_auth_observer_t *observer,
               const char *mechanism,
               G_GNUC_UNUSED xpointer_t user_data)
{
  if (xstrcmp0 (mechanism, "DBUS_COOKIE_SHA1") == 0)
    {
      g_debug ("Accepting DBUS_COOKIE_SHA1 authentication");
      return TRUE;
    }
  else
    {
      g_debug ("Rejecting \"%s\" authentication: not DBUS_COOKIE_SHA1",
               mechanism);
      return FALSE;
    }
}

static xboolean_t
allow_any_mechanism_cb (G_GNUC_UNUSED xdbus_auth_observer_t *observer,
                        const char *mechanism,
                        G_GNUC_UNUSED xpointer_t user_data)
{
  g_debug ("Accepting \"%s\" authentication", mechanism);
  return TRUE;
}

static xboolean_t
authorize_any_authenticated_peer_cb (G_GNUC_UNUSED xdbus_auth_observer_t *observer,
                                     G_GNUC_UNUSED xio_stream_t *stream,
                                     xcredentials_t *credentials,
                                     G_GNUC_UNUSED xpointer_t user_data)
{
  if (credentials == NULL)
    {
      g_debug ("Authorizing peer with no credentials");
    }
  else
    {
      xchar_t *str = xcredentials_to_string (credentials);

      g_debug ("Authorizing peer with credentials: %s", str);
      g_free (str);
    }

  return TRUE;
}

static xdbus_message_t *
whoami_filter_cb (xdbus_connection_t *connection,
                  xdbus_message_t *message,
                  xboolean_t incoming,
                  G_GNUC_UNUSED xpointer_t user_data)
{
  if (!incoming)
    return message;

  if (xdbus_message_get_message_type (message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL &&
      xstrcmp0 (xdbus_message_get_member (message), "WhoAmI") == 0)
    {
      xdbus_message_t *reply = xdbus_message_new_method_reply (message);
      sint64_t uid = -1;
      sint64_t pid = -1;
#ifdef G_OS_UNIX
      xcredentials_t *credentials = xdbus_connection_get_peer_credentials (connection);

      if (credentials != NULL)
        {
          uid = (sint64_t) xcredentials_get_unix_user (credentials, NULL);
          pid = (sint64_t) xcredentials_get_unix_pid (credentials, NULL);
        }
#endif

      xdbus_message_set_body (reply,
                               xvariant_new ("(xx)", uid, pid));
      xdbus_connection_send_message (connection, reply,
                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                      NULL, NULL);
      xobject_unref (reply);

      /* handled */
      xobject_unref (message);
      return NULL;
    }

  return message;
}

static xboolean_t
new_connection_cb (G_GNUC_UNUSED xdbus_server_t *server,
                   xdbus_connection_t *connection,
                   G_GNUC_UNUSED xpointer_t user_data)
{
  xcredentials_t *credentials = xdbus_connection_get_peer_credentials (connection);

  if (credentials == NULL)
    {
      g_debug ("New connection from peer with no credentials");
    }
  else
    {
      xchar_t *str = xcredentials_to_string (credentials);

      g_debug ("New connection from peer with credentials: %s", str);
      g_free (str);
    }

  xobject_ref (connection);
  xdbus_connection_add_filter (connection, whoami_filter_cb, NULL, NULL);
  return TRUE;
}

#ifdef HAVE_DBUS1
typedef struct
{
  DBusError error;
  DBusConnection *conn;
  DBusMessage *call;
  DBusMessage *reply;
} LibdbusCall;

static void
libdbus_call_task_cb (xtask_t *task,
                      G_GNUC_UNUSED xpointer_t source_object,
                      xpointer_t task_data,
                      G_GNUC_UNUSED xcancellable_t *cancellable)
{
  LibdbusCall *libdbus_call = task_data;

  libdbus_call->reply = dbus_connection_send_with_reply_and_block (libdbus_call->conn,
                                                                   libdbus_call->call,
                                                                   -1,
                                                                   &libdbus_call->error);
}
#endif /* HAVE_DBUS1 */

static void
store_result_cb (G_GNUC_UNUSED xobject_t *source_object,
                 xasync_result_t *res,
                 xpointer_t user_data)
{
  xasync_result_t **result = user_data;

  g_assert_nonnull (result);
  g_assert_null (*result);
  *result = xobject_ref (res);
}

static void
assert_expected_uid_pid (InteropFlags flags,
                         sint64_t uid,
                         sint64_t pid)
{
#ifdef G_OS_UNIX
  if (flags & (INTEROP_FLAGS_ANONYMOUS | INTEROP_FLAGS_SHA1 | INTEROP_FLAGS_TCP))
    {
      /* No assertion. There is no guarantee whether credentials will be
       * passed even though we didn't send them. Conversely, if
       * credentials were not passed,
       * xdbus_connection_get_peer_credentials() always returns the
       * credentials of the socket, and not the uid that a
       * client might have proved it has by using DBUS_COOKIE_SHA1. */
    }
  else    /* We should prefer EXTERNAL whenever it is allowed. */
    {
#ifdef __linux__
      /* We know that both GDBus and libdbus support full credentials-passing
       * on Linux. */
      g_assert_cmpint (uid, ==, getuid ());
      g_assert_cmpint (pid, ==, getpid ());
#elif defined(__APPLE__)
      /* We know (or at least suspect) that both GDBus and libdbus support
       * passing the uid only on macOS. */
      g_assert_cmpint (uid, ==, getuid ());
      /* No pid here */
#else
      g_test_message ("Please open a merge request to add appropriate "
                      "assertions for your platform");
#endif
    }
#endif /* G_OS_UNIX */
}

static void
do_test_server_auth (InteropFlags flags)
{
  xerror_t *error = NULL;
  xchar_t *tmpdir = NULL;
  xchar_t *listenable_address = NULL;
  xdbus_server_t *server = NULL;
  xdbus_auth_observer_t *observer = NULL;
  xdbus_server_flags_t server_flags = G_DBUS_SERVER_FLAGS_RUN_IN_THREAD;
  xchar_t *guid = NULL;
  const char *connectable_address;
  xdbus_connection_t *client = NULL;
  xasync_result_t *result = NULL;
  xvariant_t *tuple = NULL;
  sint64_t uid, pid;
#ifdef HAVE_DBUS1
  /* GNOME/glib#1831 seems to involve a race condition, so try a few times
   * to see if we can trigger it. */
  xsize_t i;
  xsize_t n = 20;
#endif

  if (flags & INTEROP_FLAGS_TCP)
    {
      listenable_address = xstrdup ("tcp:host=127.0.0.1");
    }
  else
    {
#ifdef G_OS_UNIX
      xchar_t *escaped;

      tmpdir = g_dir_make_tmp ("gdbus-server-auth-XXXXXX", &error);
      g_assert_no_error (error);
      escaped = g_dbus_address_escape_value (tmpdir);
      listenable_address = xstrdup_printf ("unix:%s=%s",
                                            (flags & INTEROP_FLAGS_ABSTRACT) ? "tmpdir" : "dir",
                                            escaped);
      g_free (escaped);
#else
      g_test_skip ("unix: addresses only work on Unix");
      goto out;
#endif
    }

  g_test_message ("Testing GDBus server at %s / libdbus client, with flags: "
                  "external:%s "
                  "anonymous:%s "
                  "sha1:%s "
                  "abstract:%s "
                  "tcp:%s",
                  listenable_address,
                  (flags & INTEROP_FLAGS_EXTERNAL) ? "true" : "false",
                  (flags & INTEROP_FLAGS_ANONYMOUS) ? "true" : "false",
                  (flags & INTEROP_FLAGS_SHA1) ? "true" : "false",
                  (flags & INTEROP_FLAGS_ABSTRACT) ? "true" : "false",
                  (flags & INTEROP_FLAGS_TCP) ? "true" : "false");

#if !defined(G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED) \
  && !defined(G_CREDENTIALS_SOCKET_GET_CREDENTIALS_SUPPORTED)
  if (flags & INTEROP_FLAGS_EXTERNAL)
    {
      g_test_skip ("EXTERNAL authentication not implemented on this platform");
      goto out;
    }
#endif

  if (flags & INTEROP_FLAGS_ANONYMOUS)
    server_flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
  if (flags & INTEROP_FLAGS_REQUIRE_SAME_USER)
    server_flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER;

  observer = xdbus_auth_observer_new ();

  if (flags & INTEROP_FLAGS_EXTERNAL)
    xsignal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_external_cb), NULL);
  else if (flags & INTEROP_FLAGS_ANONYMOUS)
    xsignal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_anonymous_cb), NULL);
  else if (flags & INTEROP_FLAGS_SHA1)
    xsignal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_sha1_cb), NULL);
  else
    xsignal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_any_mechanism_cb), NULL);

  xsignal_connect (observer, "authorize-authenticated-peer",
                    G_CALLBACK (authorize_any_authenticated_peer_cb),
                    NULL);

  guid = g_dbus_generate_guid ();
  server = xdbus_server_new_sync (listenable_address,
                                   server_flags,
                                   guid,
                                   observer,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (server);
  xsignal_connect (server, "new-connection", G_CALLBACK (new_connection_cb), NULL);
  xdbus_server_start (server);
  connectable_address = xdbus_server_get_client_address (server);
  g_test_message ("Connectable address: %s", connectable_address);

  result = NULL;
  xdbus_connection_new_for_address (connectable_address,
                                     G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                     NULL, NULL, store_result_cb, &result);

  while (result == NULL)
    xmain_context_iteration (NULL, TRUE);

  client = xdbus_connection_new_for_address_finish (result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (client);
  g_clear_object (&result);

  xdbus_connection_call (client, NULL, "/", "com.example.test_t", "WhoAmI",
                          NULL, G_VARIANT_TYPE ("(xx)"),
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL, store_result_cb,
                          &result);

  while (result == NULL)
    xmain_context_iteration (NULL, TRUE);

  tuple = xdbus_connection_call_finish (client, result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (tuple);
  g_clear_object (&result);
  g_clear_object (&client);

  uid = -2;
  pid = -2;
  xvariant_get (tuple, "(xx)", &uid, &pid);

  g_debug ("Server says GDBus client is uid %" G_GINT64_FORMAT ", pid %" G_GINT64_FORMAT,
           uid, pid);

  assert_expected_uid_pid (flags, uid, pid);

  g_clear_pointer (&tuple, xvariant_unref);

#ifdef HAVE_DBUS1
  for (i = 0; i < n; i++)
    {
      LibdbusCall libdbus_call = { DBUS_ERROR_INIT, NULL, NULL, NULL };
      xtask_t *task;

      /* The test suite uses %G_TEST_OPTION_ISOLATE_DIRS, which sets
       * `HOME=/dev/null` and leaves g_get_home_dir() pointing to the per-test
       * temp home directory. Unfortunately, libdbus doesn???t allow the home dir
       * to be overridden except using the environment, so copy the per-test
       * temp home directory back there so that libdbus uses the same
       * `$HOME/.dbus-keyrings` path as GLib. This is not thread-safe. */
      g_setenv ("HOME", g_get_home_dir (), TRUE);

      libdbus_call.conn = dbus_connection_open_private (connectable_address,
                                                        &libdbus_call.error);
      g_assert_cmpstr (libdbus_call.error.name, ==, NULL);
      g_assert_nonnull (libdbus_call.conn);

      libdbus_call.call = dbus_message_new_method_call (NULL, "/",
                                                        "com.example.test_t",
                                                        "WhoAmI");

      if (libdbus_call.call == NULL)
        xerror ("Out of memory");

      result = NULL;
      task = xtask_new (NULL, NULL, store_result_cb, &result);
      xtask_set_task_data (task, &libdbus_call, NULL);
      xtask_run_in_thread (task, libdbus_call_task_cb);

      while (result == NULL)
        xmain_context_iteration (NULL, TRUE);

      g_clear_object (&result);

      g_assert_cmpstr (libdbus_call.error.name, ==, NULL);
      g_assert_nonnull (libdbus_call.reply);

      uid = -2;
      pid = -2;
      dbus_message_get_args (libdbus_call.reply, &libdbus_call.error,
                             DBUS_TYPE_INT64, &uid,
                             DBUS_TYPE_INT64, &pid,
                             DBUS_TYPE_INVALID);
      g_assert_cmpstr (libdbus_call.error.name, ==, NULL);

      g_debug ("Server says libdbus client %" G_GSIZE_FORMAT " is uid %" G_GINT64_FORMAT ", pid %" G_GINT64_FORMAT,
               i, uid, pid);
      assert_expected_uid_pid (flags | INTEROP_FLAGS_LIBDBUS, uid, pid);

      dbus_connection_close (libdbus_call.conn);
      dbus_connection_unref (libdbus_call.conn);
      dbus_message_unref (libdbus_call.call);
      dbus_message_unref (libdbus_call.reply);
      g_clear_object (&task);
    }
#else /* !HAVE_DBUS1 */
  g_test_skip ("Testing interop with libdbus not supported");
#endif /* !HAVE_DBUS1 */

  /* No practical effect, just to avoid -Wunused-label under some
   * combinations of #ifdefs */
  goto out;

out:
  if (server != NULL)
    xdbus_server_stop (server);

  if (tmpdir != NULL)
    g_assert_cmpstr (g_rmdir (tmpdir) == 0 ? "OK" : xstrerror (errno),
                     ==, "OK");

  g_clear_object (&server);
  g_clear_object (&observer);
  g_free (guid);
  g_free (listenable_address);
  g_free (tmpdir);
}

static void
test_server_auth (void)
{
  do_test_server_auth (INTEROP_FLAGS_NONE);
}

static void
test_server_auth_abstract (void)
{
  do_test_server_auth (INTEROP_FLAGS_ABSTRACT);
}

static void
test_server_auth_tcp (void)
{
  do_test_server_auth (INTEROP_FLAGS_TCP);
}

static void
test_server_auth_anonymous (void)
{
  do_test_server_auth (INTEROP_FLAGS_ANONYMOUS);
}

static void
test_server_auth_anonymous_tcp (void)
{
  do_test_server_auth (INTEROP_FLAGS_ANONYMOUS | INTEROP_FLAGS_TCP);
}

static void
test_server_auth_external (void)
{
  do_test_server_auth (INTEROP_FLAGS_EXTERNAL);
}

static void
test_server_auth_external_require_same_user (void)
{
  do_test_server_auth (INTEROP_FLAGS_EXTERNAL | INTEROP_FLAGS_REQUIRE_SAME_USER);
}

static void
test_server_auth_sha1 (void)
{
  do_test_server_auth (INTEROP_FLAGS_SHA1);
}

static void
test_server_auth_sha1_tcp (void)
{
  do_test_server_auth (INTEROP_FLAGS_SHA1 | INTEROP_FLAGS_TCP);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gdbus/server-auth", test_server_auth);
  g_test_add_func ("/gdbus/server-auth/abstract", test_server_auth_abstract);
  g_test_add_func ("/gdbus/server-auth/tcp", test_server_auth_tcp);
  g_test_add_func ("/gdbus/server-auth/anonymous", test_server_auth_anonymous);
  g_test_add_func ("/gdbus/server-auth/anonymous/tcp", test_server_auth_anonymous_tcp);
  g_test_add_func ("/gdbus/server-auth/external", test_server_auth_external);
  g_test_add_func ("/gdbus/server-auth/external/require-same-user", test_server_auth_external_require_same_user);
  g_test_add_func ("/gdbus/server-auth/sha1", test_server_auth_sha1);
  g_test_add_func ("/gdbus/server-auth/sha1/tcp", test_server_auth_sha1_tcp);

  return g_test_run();
}
