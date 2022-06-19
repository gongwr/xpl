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

#include "config.h"

#include <gio/gio.h>
#include <unistd.h>
#include <string.h>

/* for open(2) */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/* for g_unlink() */
#include <glib/gstdio.h>

#include <gio/gnetworking.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixfdlist.h>
#include <gio/gcredentialsprivate.h>

#ifdef G_OS_UNIX
#include <gio/gunixconnection.h>
#include <errno.h>
#endif

#include "gdbus-tests.h"

#include "gdbus-object-manager-example/objectmanager-gen.h"

#ifdef G_OS_UNIX
static xboolean_t is_unix = TRUE;
#else
static xboolean_t is_unix = FALSE;
#endif

static xchar_t *tmpdir = NULL;
static xchar_t *tmp_address = NULL;
static xchar_t *test_guid = NULL;
static xmutex_t service_loop_lock;
static xcond_t service_loop_cond;
static xmain_loop_t *service_loop = NULL;
static xdbus_server_t *server = NULL;
static xmain_loop_t *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */
/* test_t that peer-to-peer connections work */
/* ---------------------------------------------------------------------------------------------------- */


typedef struct
{
  xboolean_t accept_connection;
  xint_t num_connection_attempts;
  xptr_array_t *current_connections;
  xuint_t num_method_calls;
  xboolean_t signal_received;
} PeerData;

/* This needs to be enough to usually take more than one write(),
 * to reproduce
 * <https://gitlab.gnome.org/GNOME/glib/-/issues/2074>.
 * 1 MiB ought to be enough. */
#define BIG_MESSAGE_ARRAY_SIZE (1024 * 1024)

static const xchar_t *test_interface_introspection_xml =
  "<node>"
  "  <interface name='org.gtk.GDBus.PeerTestInterface'>"
  "    <method name='HelloPeer'>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <method name='EmitSignal'/>"
  "    <method name='EmitSignalWithNameSet'/>"
  "    <method name='OpenFile'>"
  "      <arg type='s' name='path' direction='in'/>"
  "    </method>"
  "    <method name='OpenFileWithBigMessage'>"
  "      <arg type='s' name='path' direction='in'/>"
  "      <arg type='h' name='handle' direction='out'/>"
  "      <arg type='ay' name='junk' direction='out'/>"
  "    </method>"
  "    <signal name='PeerSignal'>"
  "      <arg type='s' name='a_string'/>"
  "    </signal>"
  "    <property type='s' name='PeerProperty' access='read'/>"
  "  </interface>"
  "</node>";
static xdbus_interface_info_t *test_interface_introspection_data = NULL;

static void
test_interface_method_call (xdbus_connection_t       *connection,
                            const xchar_t           *sender,
                            const xchar_t           *object_path,
                            const xchar_t           *interface_name,
                            const xchar_t           *method_name,
                            xvariant_t              *parameters,
                            xdbus_method_invocation_t *invocation,
                            xpointer_t               user_data)
{
  PeerData *data = user_data;
  const xdbus_method_info_t *info;

  data->num_method_calls++;

  g_assert_cmpstr (object_path, ==, "/org/gtk/GDBus/PeerTestObject");
  g_assert_cmpstr (interface_name, ==, "org.gtk.GDBus.PeerTestInterface");

  info = xdbus_method_invocation_get_method_info (invocation);
  g_assert_cmpstr (info->name, ==, method_name);

  if (xstrcmp0 (method_name, "HelloPeer") == 0)
    {
      const xchar_t *greeting;
      xchar_t *response;

      xvariant_get (parameters, "(&s)", &greeting);

      response = xstrdup_printf ("You greeted me with '%s'.",
                                  greeting);
      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new ("(s)", response));
      g_free (response);
    }
  else if (xstrcmp0 (method_name, "EmitSignal") == 0)
    {
      xerror_t *error;

      error = NULL;
      xdbus_connection_emit_signal (connection,
                                     NULL,
                                     "/org/gtk/GDBus/PeerTestObject",
                                     "org.gtk.GDBus.PeerTestInterface",
                                     "PeerSignal",
                                     NULL,
                                     &error);
      g_assert_no_error (error);
      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "EmitSignalWithNameSet") == 0)
    {
      xerror_t *error;
      xboolean_t ret;
      xdbus_message_t *message;

      message = xdbus_message_new_signal ("/org/gtk/GDBus/PeerTestObject",
                                           "org.gtk.GDBus.PeerTestInterface",
                                           "PeerSignalWithNameSet");
      xdbus_message_set_sender (message, ":1.42");

      error = NULL;
      ret = xdbus_connection_send_message (connection, message, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, &error);
      g_assert_no_error (error);
      xassert (ret);
      xobject_unref (message);

      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "OpenFile") == 0 ||
           xstrcmp0 (method_name, "OpenFileWithBigMessage") == 0)
    {
#ifdef G_OS_UNIX
      const xchar_t *path;
      xdbus_message_t *reply;
      xerror_t *error;
      xint_t fd;
      xunix_fd_list_t *fd_list;

      xvariant_get (parameters, "(&s)", &path);

      fd_list = g_unix_fd_list_new ();

      error = NULL;

      fd = g_open (path, O_RDONLY, 0);
      xassert (fd != -1);
      g_unix_fd_list_append (fd_list, fd, &error);
      g_assert_no_error (error);
      close (fd);

      reply = xdbus_message_new_method_reply (xdbus_method_invocation_get_message (invocation));
      xdbus_message_set_unix_fd_list (reply, fd_list);
      xobject_unref (fd_list);
      xobject_unref (invocation);

      if (xstrcmp0 (method_name, "OpenFileWithBigMessage") == 0)
        {
          char *junk;

          junk = g_new0 (char, BIG_MESSAGE_ARRAY_SIZE);
          xdbus_message_set_body (reply,
                                   xvariant_new ("(h@ay)",
                                                  0,
                                                  xvariant_new_fixed_array (G_VARIANT_TYPE_BYTE,
                                                                             junk,
                                                                             BIG_MESSAGE_ARRAY_SIZE,
                                                                             1)));
          g_free (junk);
        }

      error = NULL;
      xdbus_connection_send_message (connection,
                                      reply,
                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                      NULL, /* out_serial */
                                      &error);
      g_assert_no_error (error);
      xobject_unref (reply);
#else
      xdbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.NotOnUnix",
                                                  "Your OS does not support file descriptor passing");
#endif
    }
  else
    {
      g_assert_not_reached ();
    }
}

static xvariant_t *
test_interface_get_property (xdbus_connection_t  *connection,
                             const xchar_t      *sender,
                             const xchar_t      *object_path,
                             const xchar_t      *interface_name,
                             const xchar_t      *property_name,
                             xerror_t          **error,
                             xpointer_t          user_data)
{
  g_assert_cmpstr (object_path, ==, "/org/gtk/GDBus/PeerTestObject");
  g_assert_cmpstr (interface_name, ==, "org.gtk.GDBus.PeerTestInterface");
  g_assert_cmpstr (property_name, ==, "PeerProperty");

  return xvariant_new_string ("ThePropertyValue");
}


static const xdbus_interface_vtable_t test_interface_vtable =
{
  test_interface_method_call,
  test_interface_get_property,
  NULL,  /* set_property */
  { 0 }
};

static void
on_proxy_signal_received (xdbus_proxy_t *proxy,
                          xchar_t      *sender_name,
                          xchar_t      *signal_name,
                          xvariant_t   *parameters,
                          xpointer_t    user_data)
{
  PeerData *data = user_data;

  data->signal_received = TRUE;

  xassert (sender_name == NULL);
  g_assert_cmpstr (signal_name, ==, "PeerSignal");
  xmain_loop_quit (loop);
}

static void
on_proxy_signal_received_with_name_set (xdbus_proxy_t *proxy,
                                        xchar_t      *sender_name,
                                        xchar_t      *signal_name,
                                        xvariant_t   *parameters,
                                        xpointer_t    user_data)
{
  PeerData *data = user_data;

  data->signal_received = TRUE;

  g_assert_cmpstr (sender_name, ==, ":1.42");
  g_assert_cmpstr (signal_name, ==, "PeerSignalWithNameSet");
  xmain_loop_quit (loop);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
setup_test_address (void)
{
  if (is_unix)
    {
      g_test_message ("Testing with unix:dir address");
      tmpdir = g_dir_make_tmp ("gdbus-test-XXXXXX", NULL);
      tmp_address = xstrdup_printf ("unix:dir=%s", tmpdir);
    }
  else
    tmp_address = xstrdup ("nonce-tcp:host=127.0.0.1");
}

#ifdef G_OS_UNIX
static void
setup_tmpdir_test_address (void)
{
  g_test_message ("Testing with unix:tmpdir address");
  tmpdir = g_dir_make_tmp ("gdbus-test-XXXXXX", NULL);
  tmp_address = xstrdup_printf ("unix:tmpdir=%s", tmpdir);
}

static void
setup_path_test_address (void)
{
  g_test_message ("Testing with unix:path address");
  tmpdir = g_dir_make_tmp ("gdbus-test-XXXXXX", NULL);
  tmp_address = xstrdup_printf ("unix:path=%s/gdbus-peer-socket", tmpdir);
}
#endif

static void
teardown_test_address (void)
{
  g_free (tmp_address);
  if (tmpdir)
    {
      /* Ensuring the rmdir succeeds also ensures any sockets created on the
       * filesystem are also deleted.
       */
      g_assert_cmpstr (g_rmdir (tmpdir) == 0 ? "OK" : xstrerror (errno),
                       ==, "OK");
      g_clear_pointer (&tmpdir, g_free);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
on_authorize_authenticated_peer (xdbus_auth_observer_t *observer,
                                 xio_stream_t         *stream,
                                 xcredentials_t      *credentials,
                                 xpointer_t           user_data)
{
  PeerData *data = user_data;
  xboolean_t authorized;

  data->num_connection_attempts++;

  authorized = TRUE;
  if (!data->accept_connection)
    {
      authorized = FALSE;
      xmain_loop_quit (loop);
    }

  return authorized;
}

/* Runs in thread we created xdbus_server_t in (since we didn't pass G_DBUS_SERVER_FLAGS_RUN_IN_THREAD) */
static xboolean_t
on_new_connection (xdbus_server_t *server,
                   xdbus_connection_t *connection,
                   xpointer_t user_data)
{
  PeerData *data = user_data;
  xerror_t *error = NULL;
  xuint_t reg_id;

  //g_printerr ("Client connected.\n"
  //         "Negotiated capabilities: unix-fd-passing=%d\n",
  //         xdbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);

  xptr_array_add (data->current_connections, xobject_ref (connection));

#if G_CREDENTIALS_SUPPORTED
    {
      xcredentials_t *credentials;

      credentials = xdbus_connection_get_peer_credentials (connection);

      xassert (credentials != NULL);
      g_assert_cmpuint (xcredentials_get_unix_user (credentials, NULL), ==,
                        getuid ());
#if G_CREDENTIALS_HAS_PID
      g_assert_cmpint (xcredentials_get_unix_pid (credentials, &error), ==,
                       getpid ());
      g_assert_no_error (error);
#else
      g_assert_cmpint (xcredentials_get_unix_pid (credentials, &error), ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_clear_error (&error);
#endif
    }
#endif

  /* export object on the newly established connection */
  reg_id = xdbus_connection_register_object (connection,
                                              "/org/gtk/GDBus/PeerTestObject",
                                              test_interface_introspection_data,
                                              &test_interface_vtable,
                                              data,
                                              NULL, /* xdestroy_notify_t for data */
                                              &error);
  g_assert_no_error (error);
  xassert (reg_id > 0);

  xmain_loop_quit (loop);

  return TRUE;
}

/* We don't tell the main thread about the new xdbus_server_t until it has
 * had a chance to start listening. */
static xboolean_t
idle_in_service_loop (xpointer_t loop)
{
  xassert (service_loop == NULL);
  g_mutex_lock (&service_loop_lock);
  service_loop = loop;
  g_cond_broadcast (&service_loop_cond);
  g_mutex_unlock (&service_loop_lock);

  return XSOURCE_REMOVE;
}

static void
run_service_loop (xmain_context_t *service_context)
{
  xmain_loop_t *loop;
  xsource_t *source;

  xassert (service_loop == NULL);

  loop = xmain_loop_new (service_context, FALSE);
  source = g_idle_source_new ();
  xsource_set_callback (source, idle_in_service_loop, loop, NULL);
  xsource_attach (source, service_context);
  xsource_unref (source);
  xmain_loop_run (loop);
}

static void
teardown_service_loop (void)
{
  g_mutex_lock (&service_loop_lock);
  g_clear_pointer (&service_loop, xmain_loop_unref);
  g_mutex_unlock (&service_loop_lock);
}

static void
await_service_loop (void)
{
  g_mutex_lock (&service_loop_lock);
  while (service_loop == NULL)
    g_cond_wait (&service_loop_cond, &service_loop_lock);
  g_mutex_unlock (&service_loop_lock);
}

static xpointer_t
service_thread_func (xpointer_t user_data)
{
  PeerData *data = user_data;
  xmain_context_t *service_context;
  xdbus_auth_observer_t *observer, *o;
  xerror_t *error;
  xdbus_server_flags_t f;
  xchar_t *a, *g;
  xboolean_t b;

  service_context = xmain_context_new ();
  xmain_context_push_thread_default (service_context);

  error = NULL;
  observer = xdbus_auth_observer_new ();
  server = xdbus_server_new_sync (tmp_address,
                                   G_DBUS_SERVER_FLAGS_NONE,
                                   test_guid,
                                   observer,
                                   NULL, /* cancellable */
                                   &error);
  g_assert_no_error (error);

  xsignal_connect (server,
                    "new-connection",
                    G_CALLBACK (on_new_connection),
                    data);
  xsignal_connect (observer,
                    "authorize-authenticated-peer",
                    G_CALLBACK (on_authorize_authenticated_peer),
                    data);

  g_assert_cmpint (xdbus_server_get_flags (server), ==, G_DBUS_SERVER_FLAGS_NONE);
  g_assert_cmpstr (xdbus_server_get_guid (server), ==, test_guid);
  xobject_get (server,
                "flags", &f,
                "address", &a,
                "guid", &g,
                "active", &b,
                "authentication-observer", &o,
                NULL);
  g_assert_cmpint (f, ==, G_DBUS_SERVER_FLAGS_NONE);
  g_assert_cmpstr (a, ==, tmp_address);
  g_assert_cmpstr (g, ==, test_guid);
  xassert (!b);
  xassert (o == observer);
  g_free (a);
  g_free (g);
  xobject_unref (o);

  xobject_unref (observer);

  xdbus_server_start (server);

  run_service_loop (service_context);

  xmain_context_pop_thread_default (service_context);

  teardown_service_loop ();
  xmain_context_unref (service_context);

  /* test code specifically unrefs the server - see below */
  xassert (server == NULL);

  return NULL;
}

#if 0
static xboolean_t
on_incoming_connection (xsocket_service_t     *service,
                        xsocket_connection_t  *socket_connection,
                        xobject_t            *source_object,
                        xpointer_t           user_data)
{
  PeerData *data = user_data;

  if (data->accept_connection)
    {
      xerror_t *error;
      xuint_t reg_id;
      xdbus_connection_t *connection;

      error = NULL;
      connection = xdbus_connection_new_sync (XIO_STREAM (socket_connection),
                                               test_guid,
                                               G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER,
                                               NULL, /* cancellable */
                                               &error);
      g_assert_no_error (error);

      xptr_array_add (data->current_connections, connection);

      /* export object on the newly established connection */
      error = NULL;
      reg_id = xdbus_connection_register_object (connection,
                                                  "/org/gtk/GDBus/PeerTestObject",
                                                  &test_interface_introspection_data,
                                                  &test_interface_vtable,
                                                  data,
                                                  NULL, /* xdestroy_notify_t for data */
                                                  &error);
      g_assert_no_error (error);
      xassert (reg_id > 0);

    }
  else
    {
      /* don't do anything */
    }

  data->num_connection_attempts++;

  xmain_loop_quit (loop);

  /* stops other signal handlers from being invoked */
  return TRUE;
}

static xpointer_t
service_thread_func (xpointer_t data)
{
  xmain_context_t *service_context;
  xchar_t *socket_path;
  xsocket_address_t *address;
  xerror_t *error;

  service_context = xmain_context_new ();
  xmain_context_push_thread_default (service_context);

  socket_path = xstrdup_printf ("/tmp/gdbus-test-pid-%d", getpid ());
  address = g_unix_socket_address_new (socket_path);

  service = xsocket_service_new ();
  error = NULL;
  xsocket_listener_add_address (XSOCKET_LISTENER (service),
                                 address,
                                 XSOCKET_TYPE_STREAM,
                                 XSOCKET_PROTOCOL_DEFAULT,
                                 NULL, /* source_object */
                                 NULL, /* effective_address */
                                 &error);
  g_assert_no_error (error);
  xsignal_connect (service,
                    "incoming",
                    G_CALLBACK (on_incoming_connection),
                    data);
  xsocket_service_start (service);

  run_service_loop (service_context);

  xmain_context_pop_thread_default (service_context);

  teardown_service_loop ();
  xmain_context_unref (service_context);

  xobject_unref (address);
  g_free (socket_path);
  return NULL;
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

#if 0
static xboolean_t
check_connection (xpointer_t user_data)
{
  PeerData *data = user_data;
  xuint_t n;

  for (n = 0; n < data->current_connections->len; n++)
    {
      xdbus_connection_t *c;
      xio_stream_t *stream;

      c = G_DBUS_CONNECTION (data->current_connections->pdata[n]);
      stream = xdbus_connection_get_stream (c);

      g_debug ("In check_connection for %d: connection %p, stream %p", n, c, stream);
      g_debug ("closed = %d", g_io_stream_is_closed (stream));

      xsocket_t *socket;
      socket = xsocket_connection_get_socket (XSOCKET_CONNECTION (stream));
      g_debug ("socket_closed = %d", xsocket_is_closed (socket));
      g_debug ("socket_condition_check = %d", xsocket_condition_check (socket, G_IO_IN|G_IO_OUT|G_IO_ERR|G_IO_HUP));

      xchar_t buf[128];
      xerror_t *error;
      xssize_t num_read;
      error = NULL;
      num_read = xinput_stream_read (g_io_stream_get_input_stream (stream),
                                      buf,
                                      128,
                                      NULL,
                                      &error);
      if (num_read < 0)
        {
          g_debug ("error: %s", error->message);
          xerror_free (error);
        }
      else
        {
          g_debug ("no error, read %d bytes", (xint_t) num_read);
        }
    }

  return XSOURCE_REMOVE;
}

static xboolean_t
on_do_disconnect_in_idle (xpointer_t data)
{
  xdbus_connection_t *c = G_DBUS_CONNECTION (data);
  g_debug ("GDC %p has ref_count %d", c, G_OBJECT (c)->ref_count);
  xdbus_connection_disconnect (c);
  xobject_unref (c);
  return XSOURCE_REMOVE;
}
#endif

#ifdef G_OS_UNIX
static xchar_t *
read_all_from_fd (xint_t fd, xsize_t *out_len, xerror_t **error)
{
  xstring_t *str;
  xchar_t buf[64];
  xssize_t num_read;

  str = xstring_new (NULL);

  do
    {
      int errsv;

      num_read = read (fd, buf, sizeof (buf));
      errsv = errno;
      if (num_read == -1)
        {
          if (errsv == EAGAIN || errsv == EWOULDBLOCK)
            continue;
          g_set_error (error,
                       G_IO_ERROR,
                       g_io_error_from_errno (errsv),
                       "Failed reading %d bytes into offset %d: %s",
                       (xint_t) sizeof (buf),
                       (xint_t) str->len,
                       xstrerror (errsv));
          goto error;
        }
      else if (num_read > 0)
        {
          xstring_append_len (str, buf, num_read);
        }
      else if (num_read == 0)
        {
          break;
        }
    }
  while (TRUE);

  if (out_len != NULL)
    *out_len = str->len;
  return xstring_free (str, FALSE);

 error:
  if (out_len != NULL)
    *out_len = 0;
  xstring_free (str, TRUE);
  return NULL;
}
#endif

static void
do_test_peer (void)
{
  xdbus_connection_t *c;
  xdbus_connection_t *c2;
  xdbus_proxy_t *proxy;
  xerror_t *error;
  PeerData data;
  xvariant_t *value;
  xvariant_t *result;
  const xchar_t *s;
  xthread_t *service_thread;
  xulong_t signal_handler_id;
  xsize_t i;

  memset (&data, '\0', sizeof (PeerData));
  data.current_connections = xptr_array_new_with_free_func (xobject_unref);

  /* first try to connect when there is no server */
  error = NULL;
  c = xdbus_connection_new_for_address_sync (is_unix ? "unix:path=/tmp/gdbus-test-does-not-exist-pid" :
                                              /* NOTE: Even if something is listening on port 12345 the connection
                                               * will fail because the nonce file doesn't exist */
                                              "nonce-tcp:host=127.0.0.1,port=12345,noncefile=this-does-not-exist-gdbus",
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  _g_assert_error_domain (error, G_IO_ERROR);
  xassert (!g_dbus_error_is_remote_error (error));
  g_clear_error (&error);
  xassert (c == NULL);

  /* bring up a server - we run the server in a different thread to avoid deadlocks */
  service_thread = xthread_new ("test_peer",
                                 service_thread_func,
                                 &data);
  await_service_loop ();
  xassert (server != NULL);

  /* bring up a connection and accept it */
  data.accept_connection = TRUE;
  error = NULL;
  c = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (server),
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  g_assert_no_error (error);
  xassert (c != NULL);
  while (data.current_connections->len < 1)
    xmain_loop_run (loop);
  g_assert_cmpint (data.current_connections->len, ==, 1);
  g_assert_cmpint (data.num_connection_attempts, ==, 1);
  xassert (xdbus_connection_get_unique_name (c) == NULL);
  g_assert_cmpstr (xdbus_connection_get_guid (c), ==, test_guid);

  /* check that we create a proxy, read properties, receive signals and invoke
   * the HelloPeer() method. Since the server runs in another thread it's fine
   * to use synchronous blocking API here.
   */
  error = NULL;
  proxy = xdbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 NULL, /* bus_name */
                                 "/org/gtk/GDBus/PeerTestObject",
                                 "org.gtk.GDBus.PeerTestInterface",
                                 NULL, /* xcancellable_t */
                                 &error);
  g_assert_no_error (error);
  xassert (proxy != NULL);
  error = NULL;
  value = xdbus_proxy_get_cached_property (proxy, "PeerProperty");
  g_assert_cmpstr (xvariant_get_string (value, NULL), ==, "ThePropertyValue");

  /* try invoking a method */
  error = NULL;
  result = xdbus_proxy_call_sync (proxy,
                                   "HelloPeer",
                                   xvariant_new ("(s)", "Hey Peer!"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,  /* xcancellable_t */
                                   &error);
  g_assert_no_error (error);
  xvariant_get (result, "(&s)", &s);
  g_assert_cmpstr (s, ==, "You greeted me with 'Hey Peer!'.");
  xvariant_unref (result);
  g_assert_cmpint (data.num_method_calls, ==, 1);

  /* make the other peer emit a signal - catch it */
  signal_handler_id = xsignal_connect (proxy,
                                        "g-signal",
                                        G_CALLBACK (on_proxy_signal_received),
                                        &data);
  xassert (!data.signal_received);
  xdbus_proxy_call (proxy,
                     "EmitSignal",
                     NULL,  /* no arguments */
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,  /* xcancellable_t */
                     NULL,  /* xasync_ready_callback_t - we don't care about the result */
                     NULL); /* user_data */
  xmain_loop_run (loop);
  xassert (data.signal_received);
  g_assert_cmpint (data.num_method_calls, ==, 2);
  xsignal_handler_disconnect (proxy, signal_handler_id);

  /* Also ensure that messages with the sender header-field set gets
   * delivered to the proxy - note that this doesn't really make sense
   * e.g. names are meaning-less in a peer-to-peer case... but we
   * support it because it makes sense in certain bridging
   * applications - see e.g. #623815.
   */
  signal_handler_id = xsignal_connect (proxy,
                                        "g-signal",
                                        G_CALLBACK (on_proxy_signal_received_with_name_set),
                                        &data);
  data.signal_received = FALSE;
  xdbus_proxy_call (proxy,
                     "EmitSignalWithNameSet",
                     NULL,  /* no arguments */
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,  /* xcancellable_t */
                     NULL,  /* xasync_ready_callback_t - we don't care about the result */
                     NULL); /* user_data */
  xmain_loop_run (loop);
  xassert (data.signal_received);
  g_assert_cmpint (data.num_method_calls, ==, 3);
  xsignal_handler_disconnect (proxy, signal_handler_id);

  /*
   * Check for UNIX fd passing.
   *
   * The first time through, we use a very simple method call. Note that
   * because this does not have a G_VARIANT_TYPE_HANDLE in the message body
   * to refer to the fd, it is a GDBus-specific idiom that would not
   * interoperate with libdbus or sd-bus
   * (see <https://gitlab.gnome.org/GNOME/glib/-/merge_requests/1726>).
   *
   * The second time, we call a method that returns a fd attached to a
   * large message, to reproduce
   * <https://gitlab.gnome.org/GNOME/glib/-/issues/2074>. It also happens
   * to follow the more usual pattern for D-Bus messages containing a
   * G_VARIANT_TYPE_HANDLE to refer to attached fds.
   */
  for (i = 0; i < 2; i++)
    {
#ifdef G_OS_UNIX
      xdbus_message_t *method_call_message;
      xdbus_message_t *method_reply_message;
      xunix_fd_list_t *fd_list;
      xint_t fd;
      xchar_t *buf;
      xsize_t len;
      xchar_t *buf2;
      xsize_t len2;
      const char *testfile = g_test_get_filename (G_TEST_DIST, "file.c", NULL);
      const char *method = "OpenFile";
      xvariant_t *body;

      if (i == 1)
        method = "OpenFileWithBigMessage";

      method_call_message = xdbus_message_new_method_call (NULL, /* name */
                                                            "/org/gtk/GDBus/PeerTestObject",
                                                            "org.gtk.GDBus.PeerTestInterface",
                                                            method);
      xdbus_message_set_body (method_call_message, xvariant_new ("(s)", testfile));
      error = NULL;
      method_reply_message = xdbus_connection_send_message_with_reply_sync (c,
                                                                             method_call_message,
                                                                             G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                                             -1,
                                                                             NULL, /* out_serial */
                                                                             NULL, /* cancellable */
                                                                             &error);
      g_assert_no_error (error);
      xassert (xdbus_message_get_message_type (method_reply_message) == G_DBUS_MESSAGE_TYPE_METHOD_RETURN);

      body = xdbus_message_get_body (method_reply_message);

      if (i == 1)
        {
          gint32 handle = -1;
          xvariant_t *junk = NULL;

          g_assert_cmpstr (xvariant_get_type_string (body), ==, "(hay)");
          xvariant_get (body, "(h@ay)", &handle, &junk);
          g_assert_cmpint (handle, ==, 0);
          g_assert_cmpuint (xvariant_n_children (junk), ==, BIG_MESSAGE_ARRAY_SIZE);
          xvariant_unref (junk);
        }
      else
        {
          g_assert_null (body);
        }

      fd_list = xdbus_message_get_unix_fd_list (method_reply_message);
      xassert (fd_list != NULL);
      g_assert_cmpint (g_unix_fd_list_get_length (fd_list), ==, 1);
      error = NULL;
      fd = g_unix_fd_list_get (fd_list, 0, &error);
      g_assert_no_error (error);
      xobject_unref (method_call_message);
      xobject_unref (method_reply_message);

      error = NULL;
      len = 0;
      buf = read_all_from_fd (fd, &len, &error);
      g_assert_no_error (error);
      xassert (buf != NULL);
      close (fd);

      error = NULL;
      xfile_get_contents (testfile,
                           &buf2,
                           &len2,
                           &error);
      g_assert_no_error (error);
      g_assert_cmpmem (buf, len, buf2, len2);
      g_free (buf2);
      g_free (buf);
#else
      /* We do the same number of iterations on non-Unix, so that
       * the method call count will match. In this case we use
       * OpenFile both times, because the difference between this
       * and OpenFileWithBigMessage is only relevant on Unix. */
      error = NULL;
      result = xdbus_proxy_call_sync (proxy,
                                       "OpenFile",
                                       xvariant_new ("(s)", "boo"),
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,  /* xcancellable_t */
                                       &error);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_DBUS_ERROR);
      xassert (result == NULL);
      xerror_free (error);
#endif /* G_OS_UNIX */
    }

  /* Check that xsocket_get_credentials() work - (though this really
   * should be in socket.c)
   */
  {
    xsocket_t *socket;
    xcredentials_t *credentials;
    socket = xsocket_connection_get_socket (XSOCKET_CONNECTION (xdbus_connection_get_stream (c)));
    xassert (X_IS_SOCKET (socket));
    error = NULL;
    credentials = xsocket_get_credentials (socket, &error);

#if G_CREDENTIALS_SOCKET_GET_CREDENTIALS_SUPPORTED
    g_assert_no_error (error);
    xassert (X_IS_CREDENTIALS (credentials));

    g_assert_cmpuint (xcredentials_get_unix_user (credentials, NULL), ==,
                      getuid ());
#if G_CREDENTIALS_HAS_PID
    g_assert_cmpint (xcredentials_get_unix_pid (credentials, &error), ==,
                     getpid ());
    g_assert_no_error (error);
#else
    g_assert_cmpint (xcredentials_get_unix_pid (credentials, &error), ==, -1);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
    g_clear_error (&error);
#endif
    xobject_unref (credentials);
#else
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
    xassert (credentials == NULL);
#endif
  }


  /* bring up a connection - don't accept it - this should fail
   */
  data.accept_connection = FALSE;
  error = NULL;
  c2 = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (server),
                                               G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                               NULL, /* xdbus_auth_observer_t */
                                               NULL, /* cancellable */
                                               &error);
  _g_assert_error_domain (error, G_IO_ERROR);
  xerror_free (error);
  xassert (c2 == NULL);

#if 0
  /* TODO: THIS TEST DOESN'T WORK YET */

  /* bring up a connection - accept it.. then disconnect from the client side - check
   * that the server side gets the disconnect signal.
   */
  error = NULL;
  data.accept_connection = TRUE;
  c2 = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (server),
                                               G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                               NULL, /* xdbus_auth_observer_t */
                                               NULL, /* cancellable */
                                               &error);
  g_assert_no_error (error);
  xassert (c2 != NULL);
  xassert (!xdbus_connection_get_is_disconnected (c2));
  while (data.num_connection_attempts < 3)
    xmain_loop_run (loop);
  g_assert_cmpint (data.current_connections->len, ==, 2);
  g_assert_cmpint (data.num_connection_attempts, ==, 3);
  xassert (!xdbus_connection_get_is_disconnected (G_DBUS_CONNECTION (data.current_connections->pdata[1])));
  g_idle_add (on_do_disconnect_in_idle, c2);
  g_debug ("==================================================");
  g_debug ("==================================================");
  g_debug ("==================================================");
  g_debug ("waiting for disconnect on connection %p, stream %p",
           data.current_connections->pdata[1],
           xdbus_connection_get_stream (data.current_connections->pdata[1]));

  g_timeout_add (2000, check_connection, &data);
  //_g_assert_signal_received (G_DBUS_CONNECTION (data.current_connections->pdata[1]), "closed");
  xmain_loop_run (loop);
  xassert (xdbus_connection_get_is_disconnected (G_DBUS_CONNECTION (data.current_connections->pdata[1])));
  xptr_array_set_size (data.current_connections, 1); /* remove disconnected connection object */
#endif

  /* unref the server and stop listening for new connections
   *
   * This won't bring down the established connections - check that c is still connected
   * by invoking a method
   */
  //xsocket_service_stop (service);
  //xobject_unref (service);
  xdbus_server_stop (server);
  xobject_unref (server);
  server = NULL;

  error = NULL;
  result = xdbus_proxy_call_sync (proxy,
                                   "HelloPeer",
                                   xvariant_new ("(s)", "Hey Again Peer!"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,  /* xcancellable_t */
                                   &error);
  g_assert_no_error (error);
  xvariant_get (result, "(&s)", &s);
  g_assert_cmpstr (s, ==, "You greeted me with 'Hey Again Peer!'.");
  xvariant_unref (result);
  g_assert_cmpint (data.num_method_calls, ==, 6);

#if 0
  /* TODO: THIS TEST DOESN'T WORK YET */

  /* now disconnect from the server side - check that the client side gets the signal */
  g_assert_cmpint (data.current_connections->len, ==, 1);
  xassert (G_DBUS_CONNECTION (data.current_connections->pdata[0]) != c);
  xdbus_connection_disconnect (G_DBUS_CONNECTION (data.current_connections->pdata[0]));
  if (!xdbus_connection_get_is_disconnected (c))
    _g_assert_signal_received (c, "closed");
  xassert (xdbus_connection_get_is_disconnected (c));
#endif

  xobject_unref (c);
  xptr_array_unref (data.current_connections);
  xobject_unref (proxy);

  xmain_loop_quit (service_loop);
  xthread_join (service_thread);
}

static void
test_peer (void)
{
  test_guid = g_dbus_generate_guid ();
  loop = xmain_loop_new (NULL, FALSE);

  /* Run this test multiple times using different address formats to ensure
   * they all work.
   */
  setup_test_address ();
  do_test_peer ();
  teardown_test_address ();

#ifdef G_OS_UNIX
  setup_tmpdir_test_address ();
  do_test_peer ();
  teardown_test_address ();

  setup_path_test_address ();
  do_test_peer ();
  teardown_test_address ();
#endif

  xmain_loop_unref (loop);
  g_free (test_guid);
}

/* ---------------------------------------------------------------------------------------------------- */

#define VALID_GUID "0123456789abcdef0123456789abcdef"

static void
test_peer_invalid_server (void)
{
  xdbus_server_t *server;

  if (!g_test_undefined ())
    {
      g_test_skip ("Not exercising programming errors");
      return;
    }

  if (g_test_subprocess ())
    {
      /* This assumes we are not going to run out of xdbus_server_flags_t
       * any time soon */
      server = xdbus_server_new_sync ("tcp:", (xdbus_server_flags_t) (1 << 30),
                                       VALID_GUID,
                                       NULL, NULL, NULL);
      g_assert_null (server);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*CRITICAL*G_DBUS_SERVER_FLAGS_ALL*");
    }
}

static void
test_peer_invalid_conn_stream_sync (void)
{
  xsocket_t *sock;
  xsocket_connection_t *socket_conn;
  xio_stream_t *iostream;
  xdbus_connection_t *conn;

  if (!g_test_undefined ())
    {
      g_test_skip ("Not exercising programming errors");
      return;
    }

  sock = xsocket_new (XSOCKET_FAMILY_IPV4,
                       XSOCKET_TYPE_STREAM,
                       XSOCKET_PROTOCOL_TCP,
                       NULL);

  if (sock == NULL)
    {
      g_test_skip ("TCP not available?");
      return;
    }

  socket_conn = xsocket_connection_factory_create_connection (sock);
  g_assert_nonnull (socket_conn);
  iostream = XIO_STREAM (socket_conn);
  g_assert_nonnull (iostream);

  if (g_test_subprocess ())
    {
      /* This assumes we are not going to run out of GDBusConnectionFlags
       * any time soon */
      conn = xdbus_connection_new_sync (iostream, VALID_GUID,
                                         (GDBusConnectionFlags) (1 << 30),
                                         NULL, NULL, NULL);
      g_assert_null (conn);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*CRITICAL*G_DBUS_CONNECTION_FLAGS_ALL*");
    }

  g_clear_object (&sock);
  g_clear_object (&socket_conn);
}

static void
test_peer_invalid_conn_stream_async (void)
{
  xsocket_t *sock;
  xsocket_connection_t *socket_conn;
  xio_stream_t *iostream;

  if (!g_test_undefined ())
    {
      g_test_skip ("Not exercising programming errors");
      return;
    }

  sock = xsocket_new (XSOCKET_FAMILY_IPV4,
                       XSOCKET_TYPE_STREAM,
                       XSOCKET_PROTOCOL_TCP,
                       NULL);

  if (sock == NULL)
    {
      g_test_skip ("TCP not available?");
      return;
    }

  socket_conn = xsocket_connection_factory_create_connection (sock);
  g_assert_nonnull (socket_conn);
  iostream = XIO_STREAM (socket_conn);
  g_assert_nonnull (iostream);

  if (g_test_subprocess ())
    {
      xdbus_connection_new (iostream, VALID_GUID,
                             (GDBusConnectionFlags) (1 << 30),
                             NULL, NULL, NULL, NULL);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*CRITICAL*G_DBUS_CONNECTION_FLAGS_ALL*");
    }

  g_clear_object (&sock);
  g_clear_object (&socket_conn);
}

static void
test_peer_invalid_conn_addr_sync (void)
{
  xdbus_connection_t *conn;

  if (!g_test_undefined ())
    {
      g_test_skip ("Not exercising programming errors");
      return;
    }

  if (g_test_subprocess ())
    {
      conn = xdbus_connection_new_for_address_sync ("tcp:",
                                                     (GDBusConnectionFlags) (1 << 30),
                                                     NULL, NULL, NULL);
      g_assert_null (conn);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*CRITICAL*G_DBUS_CONNECTION_FLAGS_ALL*");
    }
}

static void
test_peer_invalid_conn_addr_async (void)
{
  if (!g_test_undefined ())
    {
      g_test_skip ("Not exercising programming errors");
      return;
    }

  if (g_test_subprocess ())
    {
      xdbus_connection_new_for_address ("tcp:",
                                         (GDBusConnectionFlags) (1 << 30),
                                         NULL, NULL, NULL, NULL);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*CRITICAL*G_DBUS_CONNECTION_FLAGS_ALL*");
    }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_peer_signals (void)
{
  xdbus_connection_t *c;
  xdbus_proxy_t *proxy;
  xerror_t *error = NULL;
  PeerData data;
  xthread_t *service_thread;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1620");

  test_guid = g_dbus_generate_guid ();
  loop = xmain_loop_new (NULL, FALSE);

  setup_test_address ();
  memset (&data, '\0', sizeof (PeerData));
  data.current_connections = xptr_array_new_with_free_func (xobject_unref);

  /* bring up a server - we run the server in a different thread to avoid deadlocks */
  service_thread = xthread_new ("test_peer",
                                 service_thread_func,
                                 &data);
  await_service_loop ();
  g_assert_nonnull (server);

  /* bring up a connection and accept it */
  data.accept_connection = TRUE;
  c = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (server),
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  g_assert_no_error (error);
  g_assert_nonnull (c);
  while (data.current_connections->len < 1)
    xmain_loop_run (loop);
  g_assert_cmpint (data.current_connections->len, ==, 1);
  g_assert_cmpint (data.num_connection_attempts, ==, 1);
  g_assert_null (xdbus_connection_get_unique_name (c));
  g_assert_cmpstr (xdbus_connection_get_guid (c), ==, test_guid);

  /* Check that we can create a proxy with a non-NULL bus name, even though it's
   * irrelevant in the non-message-bus case. Since the server runs in another
   * thread it's fine to use synchronous blocking API here.
   */
  proxy = xdbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                 G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                 NULL,
                                 ":1.1", /* bus_name */
                                 "/org/gtk/GDBus/PeerTestObject",
                                 "org.gtk.GDBus.PeerTestInterface",
                                 NULL, /* xcancellable_t */
                                 &error);
  g_assert_no_error (error);
  g_assert_nonnull (proxy);

  /* unref the server and stop listening for new connections */
  xdbus_server_stop (server);
  g_clear_object (&server);

  xobject_unref (c);
  xptr_array_unref (data.current_connections);
  xobject_unref (proxy);

  xmain_loop_quit (service_loop);
  xthread_join (service_thread);

  teardown_test_address ();

  xmain_loop_unref (loop);
  g_free (test_guid);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xdbus_server_t *server;
  xmain_context_t *context;
  xmain_loop_t *loop;

  xlist_t *connections;
} DmpData;

static void
dmp_data_free (DmpData *data)
{
  xmain_loop_unref (data->loop);
  xmain_context_unref (data->context);
  xobject_unref (data->server);
  xlist_free_full (data->connections, xobject_unref);
  g_free (data);
}

static void
dmp_on_method_call (xdbus_connection_t       *connection,
                    const xchar_t           *sender,
                    const xchar_t           *object_path,
                    const xchar_t           *interface_name,
                    const xchar_t           *method_name,
                    xvariant_t              *parameters,
                    xdbus_method_invocation_t *invocation,
                    xpointer_t               user_data)
{
  //DmpData *data = user_data;
  gint32 first;
  gint32 second;
  xvariant_get (parameters,
                 "(ii)",
                 &first,
                 &second);
  xdbus_method_invocation_return_value (invocation,
                                         xvariant_new ("(i)", first + second));
}

static const xdbus_interface_vtable_t dmp_interface_vtable =
{
  dmp_on_method_call,
  NULL,  /* get_property */
  NULL,  /* set_property */
  { 0 }
};


/* Runs in thread we created xdbus_server_t in (since we didn't pass G_DBUS_SERVER_FLAGS_RUN_IN_THREAD) */
static xboolean_t
dmp_on_new_connection (xdbus_server_t     *server,
                       xdbus_connection_t *connection,
                       xpointer_t         user_data)
{
  DmpData *data = user_data;
  xdbus_node_info_t *node;
  xerror_t *error;

  /* accept the connection */
  data->connections = xlist_prepend (data->connections, xobject_ref (connection));

  error = NULL;
  node = g_dbus_node_info_new_for_xml ("<node>"
                                       "  <interface name='org.gtk.GDBus.DmpInterface'>"
                                       "    <method name='AddPair'>"
                                       "      <arg type='i' name='first' direction='in'/>"
                                       "      <arg type='i' name='second' direction='in'/>"
                                       "      <arg type='i' name='sum' direction='out'/>"
                                       "    </method>"
                                       "  </interface>"
                                       "</node>",
                                       &error);
  g_assert_no_error (error);

  /* sleep 100ms before exporting an object - this is to test that
   * G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING really works
   * (xdbus_server_t uses this feature).
   */
  usleep (100 * 1000);

  /* export an object */
  error = NULL;
  xdbus_connection_register_object (connection,
                                     "/dmp/test",
                                     node->interfaces[0],
                                     &dmp_interface_vtable,
                                     data,
                                     NULL,
                                     &error);
  g_dbus_node_info_unref (node);

  return TRUE;
}

static xpointer_t
dmp_thread_func (xpointer_t user_data)
{
  DmpData *data = user_data;
  xerror_t *error;
  xchar_t *guid;

  data->context = xmain_context_new ();
  xmain_context_push_thread_default (data->context);

  error = NULL;
  guid = g_dbus_generate_guid ();
  data->server = xdbus_server_new_sync (tmp_address,
                                         G_DBUS_SERVER_FLAGS_NONE,
                                         guid,
                                         NULL, /* xdbus_auth_observer_t */
                                         NULL, /* xcancellable_t */
                                         &error);
  g_assert_no_error (error);
  xsignal_connect (data->server,
                    "new-connection",
                    G_CALLBACK (dmp_on_new_connection),
                    data);

  xdbus_server_start (data->server);

  data->loop = xmain_loop_new (data->context, FALSE);
  xmain_loop_run (data->loop);

  xdbus_server_stop (data->server);
  xmain_context_pop_thread_default (data->context);

  g_free (guid);
  return NULL;
}

static void
delayed_message_processing (void)
{
  xerror_t *error;
  DmpData *data;
  xthread_t *service_thread;
  xuint_t n;

  test_guid = g_dbus_generate_guid ();
  loop = xmain_loop_new (NULL, FALSE);

  setup_test_address ();

  data = g_new0 (DmpData, 1);

  service_thread = xthread_new ("dmp",
                                 dmp_thread_func,
                                 data);
  while (data->server == NULL || !xdbus_server_is_active (data->server))
    xthread_yield ();

  for (n = 0; n < 5; n++)
    {
      xdbus_connection_t *c;
      xvariant_t *res;
      gint32 val;

      error = NULL;
      c = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (data->server),
                                                  G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                  NULL, /* xdbus_auth_observer_t */
                                                  NULL, /* xcancellable_t */
                                                  &error);
      g_assert_no_error (error);

      error = NULL;
      res = xdbus_connection_call_sync (c,
                                         NULL,    /* bus name */
                                         "/dmp/test",
                                         "org.gtk.GDBus.DmpInterface",
                                         "AddPair",
                                         xvariant_new ("(ii)", 2, n),
                                         G_VARIANT_TYPE ("(i)"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1, /* timeout_msec */
                                         NULL, /* xcancellable_t */
                                         &error);
      g_assert_no_error (error);
      xvariant_get (res, "(i)", &val);
      g_assert_cmpint (val, ==, 2 + n);
      xvariant_unref (res);
      xobject_unref (c);
  }

  xmain_loop_quit (data->loop);
  xthread_join (service_thread);
  dmp_data_free (data);
  teardown_test_address ();

  xmain_loop_unref (loop);
  g_free (test_guid);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
nonce_tcp_on_authorize_authenticated_peer (xdbus_auth_observer_t *observer,
                                           xio_stream_t         *stream,
                                           xcredentials_t      *credentials,
                                           xpointer_t           user_data)
{
  PeerData *data = user_data;
  xboolean_t authorized;

  data->num_connection_attempts++;

  authorized = TRUE;
  if (!data->accept_connection)
    {
      authorized = FALSE;
      xmain_loop_quit (loop);
    }

  return authorized;
}

/* Runs in thread we created xdbus_server_t in (since we didn't pass G_DBUS_SERVER_FLAGS_RUN_IN_THREAD) */
static xboolean_t
nonce_tcp_on_new_connection (xdbus_server_t *server,
                             xdbus_connection_t *connection,
                             xpointer_t user_data)
{
  PeerData *data = user_data;

  xptr_array_add (data->current_connections, xobject_ref (connection));

  xmain_loop_quit (loop);

  return TRUE;
}

static xpointer_t
nonce_tcp_service_thread_func (xpointer_t user_data)
{
  PeerData *data = user_data;
  xmain_context_t *service_context;
  xdbus_auth_observer_t *observer;
  xerror_t *error;

  service_context = xmain_context_new ();
  xmain_context_push_thread_default (service_context);

  error = NULL;
  observer = xdbus_auth_observer_new ();
  server = xdbus_server_new_sync ("nonce-tcp:host=127.0.0.1",
                                   G_DBUS_SERVER_FLAGS_NONE,
                                   test_guid,
                                   observer,
                                   NULL, /* cancellable */
                                   &error);
  g_assert_no_error (error);

  xsignal_connect (server,
                    "new-connection",
                    G_CALLBACK (nonce_tcp_on_new_connection),
                    data);
  xsignal_connect (observer,
                    "authorize-authenticated-peer",
                    G_CALLBACK (nonce_tcp_on_authorize_authenticated_peer),
                    data);
  xobject_unref (observer);

  xdbus_server_start (server);

  run_service_loop (service_context);

  xmain_context_pop_thread_default (service_context);

  teardown_service_loop ();
  xmain_context_unref (service_context);

  /* test code specifically unrefs the server - see below */
  xassert (server == NULL);

  return NULL;
}

static void
test_nonce_tcp (void)
{
  PeerData data;
  xerror_t *error;
  xthread_t *service_thread;
  xdbus_connection_t *c;
  xchar_t *s;
  xchar_t *nonce_file;
  xboolean_t res;
  const xchar_t *address;

  test_guid = g_dbus_generate_guid ();
  loop = xmain_loop_new (NULL, FALSE);

  memset (&data, '\0', sizeof (PeerData));
  data.current_connections = xptr_array_new_with_free_func (xobject_unref);

  error = NULL;
  server = NULL;
  service_thread = xthread_new ("nonce-tcp-service",
                                 nonce_tcp_service_thread_func,
                                 &data);
  await_service_loop ();
  xassert (server != NULL);

  /* bring up a connection and accept it */
  data.accept_connection = TRUE;
  error = NULL;
  c = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (server),
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  g_assert_no_error (error);
  xassert (c != NULL);
  while (data.current_connections->len < 1)
    xthread_yield ();
  g_assert_cmpint (data.current_connections->len, ==, 1);
  g_assert_cmpint (data.num_connection_attempts, ==, 1);
  xassert (xdbus_connection_get_unique_name (c) == NULL);
  g_assert_cmpstr (xdbus_connection_get_guid (c), ==, test_guid);
  xobject_unref (c);

  /* now, try to subvert the nonce file (this assumes noncefile is the last key/value pair)
   */

  address = xdbus_server_get_client_address (server);

  s = strstr (address, "noncefile=");
  xassert (s != NULL);
  s += sizeof "noncefile=" - 1;
  nonce_file = xstrdup (s);

  /* First try invalid data in the nonce file - this will actually
   * make the client send this and the server will reject it. The way
   * it works is that if the nonce doesn't match, the server will
   * simply close the connection. So, from the client point of view,
   * we can see a variety of errors.
   */
  error = NULL;
  res = xfile_set_contents (nonce_file,
                             "0123456789012345",
                             -1,
                             &error);
  g_assert_no_error (error);
  xassert (res);
  c = xdbus_connection_new_for_address_sync (address,
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  _g_assert_error_domain (error, G_IO_ERROR);
  xerror_free (error);
  xassert (c == NULL);

  /* Then try with a nonce-file of incorrect length - this will make
   * the client complain - we won't even try connecting to the server
   * for this
   */
  error = NULL;
  res = xfile_set_contents (nonce_file,
                             "0123456789012345_",
                             -1,
                             &error);
  g_assert_no_error (error);
  xassert (res);
  c = xdbus_connection_new_for_address_sync (address,
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  xerror_free (error);
  xassert (c == NULL);

  /* Finally try with no nonce-file at all */
  g_assert_cmpint (g_unlink (nonce_file), ==, 0);
  error = NULL;
  c = xdbus_connection_new_for_address_sync (address,
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              NULL, /* xdbus_auth_observer_t */
                                              NULL, /* cancellable */
                                              &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  xerror_free (error);
  xassert (c == NULL);

  /* Recreate the nonce-file so we can ensure the server deletes it when stopped. */
  g_assert_cmpint (g_creat (nonce_file, 0600), !=, -1);

  xdbus_server_stop (server);
  xobject_unref (server);
  server = NULL;

  g_assert_false (xfile_test (nonce_file, XFILE_TEST_EXISTS));
  g_free (nonce_file);

  xmain_loop_quit (service_loop);
  xthread_join (service_thread);

  xptr_array_unref (data.current_connections);

  xmain_loop_unref (loop);
  g_free (test_guid);
}

static void
test_credentials (void)
{
  xcredentials_t *c1, *c2;
  xerror_t *error;
  xchar_t *desc;

  c1 = xcredentials_new ();
  c2 = xcredentials_new ();

  error = NULL;
  if (xcredentials_set_unix_user (c2, getuid (), &error))
    g_assert_no_error (error);

  g_clear_error (&error);
  xassert (xcredentials_is_same_user (c1, c2, &error));
  g_assert_no_error (error);

  desc = xcredentials_to_string (c1);
  xassert (desc != NULL);
  g_free (desc);

  xobject_unref (c1);
  xobject_unref (c2);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
tcp_anonymous_on_new_connection (xdbus_server_t     *server,
                                 xdbus_connection_t *connection,
                                 xpointer_t         user_data)
{
  xboolean_t *seen_connection = user_data;
  *seen_connection = TRUE;
  return TRUE;
}

static xpointer_t
tcp_anonymous_service_thread_func (xpointer_t user_data)
{
  xboolean_t *seen_connection = user_data;
  xmain_context_t *service_context;
  xerror_t *error;

  service_context = xmain_context_new ();
  xmain_context_push_thread_default (service_context);

  error = NULL;
  server = xdbus_server_new_sync ("tcp:host=127.0.0.1",
                                   G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS,
                                   test_guid,
                                   NULL, /* GDBusObserver* */
                                   NULL, /* xcancellable_t* */
                                   &error);
  g_assert_no_error (error);

  xsignal_connect (server,
                    "new-connection",
                    G_CALLBACK (tcp_anonymous_on_new_connection),
                    seen_connection);

  xdbus_server_start (server);

  run_service_loop (service_context);

  xmain_context_pop_thread_default (service_context);

  teardown_service_loop ();
  xmain_context_unref (service_context);

  return NULL;
}

static void
test_tcp_anonymous (void)
{
  xboolean_t seen_connection;
  xthread_t *service_thread;
  xdbus_connection_t *connection;
  xerror_t *error;

  test_guid = g_dbus_generate_guid ();
  loop = xmain_loop_new (NULL, FALSE);

  seen_connection = FALSE;
  service_thread = xthread_new ("tcp-anon-service",
                                 tcp_anonymous_service_thread_func,
                                 &seen_connection);
  await_service_loop ();
  xassert (server != NULL);

  error = NULL;
  connection = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (server),
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                       NULL, /* xdbus_auth_observer_t* */
                                                       NULL, /* xcancellable_t */
                                                       &error);
  g_assert_no_error (error);
  xassert (connection != NULL);

  while (!seen_connection)
    xthread_yield ();

  xobject_unref (connection);

  xmain_loop_quit (service_loop);
  xdbus_server_stop (server);
  xobject_unref (server);
  server = NULL;

  xthread_join (service_thread);

  xmain_loop_unref (loop);
  g_free (test_guid);
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_server_t *codegen_server = NULL;

static xboolean_t
codegen_on_animal_poke (ExampleAnimal          *animal,
                        xdbus_method_invocation_t  *invocation,
                        xboolean_t                make_sad,
                        xboolean_t                make_happy,
                        xpointer_t                user_data)
{
  if ((make_sad && make_happy) || (!make_sad && !make_happy))
    {
      xmain_loop_quit (service_loop);

      xdbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.Examples.ObjectManager.Error.Failed",
                                                  "Exactly one of make_sad or make_happy must be TRUE");
      goto out;
    }

  if (make_sad)
    {
      if (xstrcmp0 (example_animal_get_mood (animal), "Sad") == 0)
        {
          xdbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Examples.ObjectManager.Error.SadAnimalIsSad",
                                                      "Sad animal is already sad");
          goto out;
        }

      example_animal_set_mood (animal, "Sad");
      example_animal_complete_poke (animal, invocation);
      goto out;
    }

  if (make_happy)
    {
      if (xstrcmp0 (example_animal_get_mood (animal), "Happy") == 0)
        {
          xdbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Examples.ObjectManager.Error.HappyAnimalIsHappy",
                                                      "Happy animal is already happy");
          goto out;
        }

      example_animal_set_mood (animal, "Happy");
      example_animal_complete_poke (animal, invocation);
      goto out;
    }

  g_assert_not_reached ();

 out:
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

/* Runs in thread we created xdbus_server_t in (since we didn't pass G_DBUS_SERVER_FLAGS_RUN_IN_THREAD) */
static xboolean_t
codegen_on_new_connection (xdbus_server_t *server,
                           xdbus_connection_t *connection,
                           xpointer_t user_data)
{
  ExampleAnimal *animal = user_data;
  xerror_t        *error = NULL;

  /* g_printerr ("Client connected.\n" */
  /*          "Negotiated capabilities: unix-fd-passing=%d\n", */
  /*          xdbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING); */

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (animal), connection,
                                    "/Example/Animals/000", &error);
  g_assert_no_error (error);

  return TRUE;
}

static xpointer_t
codegen_service_thread_func (xpointer_t user_data)
{
  xmain_context_t   *service_context;
  ExampleAnimal  *animal;
  xerror_t         *error = NULL;

  service_context = xmain_context_new ();
  xmain_context_push_thread_default (service_context);

  /* Create the animal in the right thread context */
  animal = example_animal_skeleton_new ();

  /* Handle Poke() D-Bus method invocations on the .Animal interface */
  xsignal_connect (animal, "handle-poke",
                    G_CALLBACK (codegen_on_animal_poke),
                    NULL); /* user_data */

  codegen_server = xdbus_server_new_sync (tmp_address,
                                           G_DBUS_SERVER_FLAGS_NONE,
                                           test_guid,
                                           NULL, /* observer */
                                           NULL, /* cancellable */
                                           &error);
  g_assert_no_error (error);
  xdbus_server_start (codegen_server);

  xsignal_connect (codegen_server, "new-connection",
                    G_CALLBACK (codegen_on_new_connection),
                    animal);

  run_service_loop (service_context);

  xobject_unref (animal);

  xmain_context_pop_thread_default (service_context);

  teardown_service_loop ();
  xmain_context_unref (service_context);

  xdbus_server_stop (codegen_server);
  xobject_unref (codegen_server);
  codegen_server = NULL;

  return NULL;
}


static xboolean_t
codegen_quit_mainloop_timeout (xpointer_t data)
{
  xmain_loop_quit (loop);
  return XSOURCE_REMOVE;
}

static void
codegen_test_peer (void)
{
  xdbus_connection_t     *connection;
  ExampleAnimal       *animal1, *animal2;
  xthread_t             *service_thread;
  xerror_t              *error = NULL;
  xvariant_t            *value;
  const xchar_t         *s;

  test_guid = g_dbus_generate_guid ();
  loop = xmain_loop_new (NULL, FALSE);

  setup_test_address ();

  /* bring up a server - we run the server in a different thread to avoid deadlocks */
  service_thread = xthread_new ("codegen_test_peer",
                                 codegen_service_thread_func,
                                 NULL);
  await_service_loop ();
  xassert (codegen_server != NULL);

  /* Get an animal 1 ...  */
  connection = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (codegen_server),
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                       NULL, /* xdbus_auth_observer_t */
                                                       NULL, /* cancellable */
                                                       &error);
  g_assert_no_error (error);
  xassert (connection != NULL);

  animal1 = example_animal_proxy_new_sync (connection, 0, NULL,
                                           "/Example/Animals/000", NULL, &error);
  g_assert_no_error (error);
  xassert (animal1 != NULL);
  xobject_unref (connection);

  /* Get animal 2 ...  */
  connection = xdbus_connection_new_for_address_sync (xdbus_server_get_client_address (codegen_server),
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                       NULL, /* xdbus_auth_observer_t */
                                                       NULL, /* cancellable */
                                                       &error);
  g_assert_no_error (error);
  xassert (connection != NULL);

  animal2 = example_animal_proxy_new_sync (connection, 0, NULL,
                                           "/Example/Animals/000", NULL, &error);
  g_assert_no_error (error);
  xassert (animal2 != NULL);
  xobject_unref (connection);

  /* Make animal sad via animal1  */
  example_animal_call_poke_sync (animal1, TRUE, FALSE, NULL, &error);
  g_assert_no_error (error);

  /* Poke server and make sure animal is updated */
  value = xdbus_proxy_call_sync (G_DBUS_PROXY (animal1),
                                  "org.freedesktop.DBus.Peer.Ping",
                                  NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                                  NULL, &error);
  g_assert_no_error (error);
  xassert (value != NULL);
  xvariant_unref (value);

  /* Give the proxies a chance to refresh in the default main loop */
  g_timeout_add (100, codegen_quit_mainloop_timeout, NULL);
  xmain_loop_run (loop);

  /* Assert animals are sad */
  g_assert_cmpstr (example_animal_get_mood (animal1), ==, "Sad");
  g_assert_cmpstr (example_animal_get_mood (animal2), ==, "Sad");

  /* Make animal happy via animal2  */
  example_animal_call_poke_sync (animal2, FALSE, TRUE, NULL, &error);
  g_assert_no_error (error);

  /* Some random unrelated call, just to get some test coverage */
  value = xdbus_proxy_call_sync (G_DBUS_PROXY (animal2),
                                  "org.freedesktop.DBus.Peer.GetMachineId",
                                  NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                                  NULL, &error);
  g_assert_no_error (error);
  xvariant_get (value, "(&s)", &s);
  g_test_message ("Machine ID: %s", s);
  /* It's valid for machine-id inside containers to be empty, so we
   * need to test for that possibility
   */
  xassert ((s == NULL || *s == '\0') || g_dbus_is_guid (s));
  xvariant_unref (value);

  /* Poke server and make sure animal is updated */
  value = xdbus_proxy_call_sync (G_DBUS_PROXY (animal2),
                                  "org.freedesktop.DBus.Peer.Ping",
                                  NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                                  NULL, &error);
  g_assert_no_error (error);
  xassert (value != NULL);
  xvariant_unref (value);

  /* Give the proxies a chance to refresh in the default main loop */
  g_timeout_add (1000, codegen_quit_mainloop_timeout, NULL);
  xmain_loop_run (loop);

  /* Assert animals are happy */
  g_assert_cmpstr (example_animal_get_mood (animal1), ==, "Happy");
  g_assert_cmpstr (example_animal_get_mood (animal2), ==, "Happy");

  /* This final call making the animal happy and sad will cause
   * the server to quit, when the server quits we dont get property
   * change notifications anyway because those are done from an idle handler
   */
  example_animal_call_poke_sync (animal2, TRUE, TRUE, NULL, &error);
  g_clear_error (&error);

  xobject_unref (animal1);
  xobject_unref (animal2);
  xthread_join (service_thread);

  teardown_test_address ();

  xmain_loop_unref (loop);
  g_free (test_guid);
}

/* ---------------------------------------------------------------------------------------------------- */


int
main (int   argc,
      char *argv[])
{
  xint_t ret;
  xdbus_node_info_t *introspection_data = NULL;

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  introspection_data = g_dbus_node_info_new_for_xml (test_interface_introspection_xml, NULL);
  xassert (introspection_data != NULL);
  test_interface_introspection_data = introspection_data->interfaces[0];

  g_test_add_func ("/gdbus/peer-to-peer", test_peer);
  g_test_add_func ("/gdbus/peer-to-peer/invalid/server",
                   test_peer_invalid_server);
  g_test_add_func ("/gdbus/peer-to-peer/invalid/conn/stream/async",
                   test_peer_invalid_conn_stream_async);
  g_test_add_func ("/gdbus/peer-to-peer/invalid/conn/stream/sync",
                   test_peer_invalid_conn_stream_sync);
  g_test_add_func ("/gdbus/peer-to-peer/invalid/conn/addr/async",
                   test_peer_invalid_conn_addr_async);
  g_test_add_func ("/gdbus/peer-to-peer/invalid/conn/addr/sync",
                   test_peer_invalid_conn_addr_sync);
  g_test_add_func ("/gdbus/peer-to-peer/signals", test_peer_signals);
  g_test_add_func ("/gdbus/delayed-message-processing", delayed_message_processing);
  g_test_add_func ("/gdbus/nonce-tcp", test_nonce_tcp);

  g_test_add_func ("/gdbus/tcp-anonymous", test_tcp_anonymous);
  g_test_add_func ("/gdbus/credentials", test_credentials);
  g_test_add_func ("/gdbus/codegen-peer-to-peer", codegen_test_peer);

  ret = g_test_run ();

  g_dbus_node_info_unref (introspection_data);

  return ret;
}
