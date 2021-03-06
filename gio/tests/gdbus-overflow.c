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

/* used in test_overflow */
#ifdef G_OS_UNIX
#include <gio/gunixconnection.h>
#include <errno.h>
#endif

#ifdef G_OS_UNIX
static xboolean_t is_unix = TRUE;
#else
static xboolean_t is_unix = FALSE;
#endif

static xchar_t *tmp_address = NULL;
static xchar_t *test_guid = NULL;
static xmain_loop_t *loop = NULL;

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
  "    <signal name='PeerSignal'>"
  "      <arg type='s' name='a_string'/>"
  "    </signal>"
  "    <property type='s' name='PeerProperty' access='read'/>"
  "  </interface>"
  "</node>";
static xdbus_interface_info_t *test_interface_introspection_data = NULL;


#ifdef G_OS_UNIX

/* Chosen to be big enough to overflow the socket buffer */
#define OVERFLOW_NUM_SIGNALS 5000
#define OVERFLOW_TIMEOUT_SEC 10

static xdbus_message_t *
overflow_filter_func (xdbus_connection_t *connection,
                      xdbus_message_t    *message,
                      xboolean_t         incoming,
                      xpointer_t         user_data)
{
  xint_t *counter = user_data;  /* (atomic) */
  g_atomic_int_inc (counter);
  return message;
}

static xboolean_t
overflow_on_500ms_later_func (xpointer_t user_data)
{
  xmain_loop_quit (loop);
  return XSOURCE_REMOVE;
}

static void
test_overflow (void)
{
  xint_t sv[2];
  xint_t n;
  xsocket_t *socket;
  xsocket_connection_t *socket_connection;
  xdbus_connection_t *producer, *consumer;
  xerror_t *error;
  xtimer_t *timer;
  xint_t n_messages_received;  /* (atomic) */
  xint_t n_messages_sent;  /* (atomic) */

  g_assert_cmpint (socketpair (AF_UNIX, SOCK_STREAM, 0, sv), ==, 0);

  error = NULL;
  socket = xsocket_new_from_fd (sv[0], &error);
  g_assert_no_error (error);
  socket_connection = xsocket_connection_factory_create_connection (socket);
  xassert (socket_connection != NULL);
  xobject_unref (socket);
  producer = xdbus_connection_new_sync (XIO_STREAM (socket_connection),
					 NULL, /* guid */
					 G_DBUS_CONNECTION_FLAGS_NONE,
					 NULL, /* xdbus_auth_observer_t */
					 NULL, /* xcancellable_t */

					 &error);
  xdbus_connection_set_exit_on_close (producer, TRUE);
  g_assert_no_error (error);
  xobject_unref (socket_connection);
  g_atomic_int_set (&n_messages_sent, 0);
  xdbus_connection_add_filter (producer, overflow_filter_func, (xpointer_t) &n_messages_sent, NULL);

  /* send enough data that we get an EAGAIN */
  for (n = 0; n < OVERFLOW_NUM_SIGNALS; n++)
    {
      error = NULL;
      xdbus_connection_emit_signal (producer,
                                     NULL, /* destination */
                                     "/org/foo/Object",
                                     "org.foo.Interface",
                                     "Member",
                                     xvariant_new ("(s)", "a string"),
                                     &error);
      g_assert_no_error (error);
    }

  /* sleep for 0.5 sec (to allow the GDBus IO thread to fill up the
   * kernel buffers) and verify that n_messages_sent <
   * OVERFLOW_NUM_SIGNALS
   *
   * This is to verify that not all the submitted messages have been
   * sent to the underlying transport.
   */
  g_timeout_add (500, overflow_on_500ms_later_func, NULL);
  xmain_loop_run (loop);
  g_assert_cmpint (g_atomic_int_get (&n_messages_sent), <, OVERFLOW_NUM_SIGNALS);

  /* now suck it all out as a client, and add it up */
  socket = xsocket_new_from_fd (sv[1], &error);
  g_assert_no_error (error);
  socket_connection = xsocket_connection_factory_create_connection (socket);
  xassert (socket_connection != NULL);
  xobject_unref (socket);
  consumer = xdbus_connection_new_sync (XIO_STREAM (socket_connection),
					 NULL, /* guid */
					 G_DBUS_CONNECTION_FLAGS_DELAY_MESSAGE_PROCESSING,
					 NULL, /* xdbus_auth_observer_t */
					 NULL, /* xcancellable_t */
					 &error);
  g_assert_no_error (error);
  xobject_unref (socket_connection);
  g_atomic_int_set (&n_messages_received, 0);
  xdbus_connection_add_filter (consumer, overflow_filter_func, (xpointer_t) &n_messages_received, NULL);
  xdbus_connection_start_message_processing (consumer);

  timer = g_timer_new ();
  g_timer_start (timer);

  while (g_atomic_int_get (&n_messages_received) < OVERFLOW_NUM_SIGNALS && g_timer_elapsed (timer, NULL) < OVERFLOW_TIMEOUT_SEC)
      xmain_context_iteration (NULL, FALSE);

  g_assert_cmpint (g_atomic_int_get (&n_messages_sent), ==, OVERFLOW_NUM_SIGNALS);
  g_assert_cmpint (g_atomic_int_get (&n_messages_received), ==, OVERFLOW_NUM_SIGNALS);

  g_timer_destroy (timer);
  xobject_unref (consumer);
  xobject_unref (producer);
}
#else
static void
test_overflow (void)
{
  /* TODO: test this with e.g. GWin32InputStream/GWin32OutputStream */
}
#endif

/* ---------------------------------------------------------------------------------------------------- */


int
main (int   argc,
      char *argv[])
{
  xint_t ret;
  xdbus_node_info_t *introspection_data = NULL;
  xchar_t *tmpdir = NULL;

  g_test_init (&argc, &argv, NULL);

  introspection_data = g_dbus_node_info_new_for_xml (test_interface_introspection_xml, NULL);
  xassert (introspection_data != NULL);
  test_interface_introspection_data = introspection_data->interfaces[0];

  test_guid = g_dbus_generate_guid ();

  if (is_unix)
    {
      if (g_unix_socket_address_abstract_names_supported ())
	tmp_address = xstrdup ("unix:tmpdir=/tmp/gdbus-test-");
      else
	{
	  tmpdir = g_dir_make_tmp ("gdbus-test-XXXXXX", NULL);
	  tmp_address = xstrdup_printf ("unix:tmpdir=%s", tmpdir);
	}
    }
  else
    tmp_address = xstrdup ("nonce-tcp:");

  /* all the tests rely on a shared main loop */
  loop = xmain_loop_new (NULL, FALSE);

  g_test_add_func ("/gdbus/overflow", test_overflow);

  ret = g_test_run();

  xmain_loop_unref (loop);
  g_free (test_guid);
  g_dbus_node_info_unref (introspection_data);
  if (is_unix)
    g_free (tmp_address);
  if (tmpdir)
    {
      g_rmdir (tmpdir);
      g_free (tmpdir);
    }

  return ret;
}
