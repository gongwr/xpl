/* GLib testing framework examples and tests
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#include <gio/gio.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <dlfcn.h>
#include <fcntl.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#endif

xmain_loop_t *loop;
xpollable_input_stream_t *in;
xoutput_stream_t *out;

static xboolean_t
poll_source_callback (xpollable_input_stream_t *in,
		      xpointer_t              user_data)
{
  xerror_t *error = NULL;
  char buf[2];
  xssize_t nread;
  xboolean_t *success = user_data;

  g_assert_true (g_pollable_input_stream_is_readable (G_POLLABLE_INPUT_STREAM (in)));

  nread = g_pollable_input_stream_read_nonblocking (in, buf, 2, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nread, ==, 2);
  g_assert_cmpstr (buf, ==, "x");
  g_assert_false (g_pollable_input_stream_is_readable (G_POLLABLE_INPUT_STREAM (in)));

  *success = TRUE;
  return XSOURCE_REMOVE;
}

static xboolean_t
check_source_readability_callback (xpointer_t user_data)
{
  xboolean_t expected = GPOINTER_TO_INT (user_data);
  xboolean_t readable;

  readable = g_pollable_input_stream_is_readable (in);
  g_assert_cmpint (readable, ==, expected);
  return XSOURCE_REMOVE;
}

static xboolean_t
write_callback (xpointer_t user_data)
{
  const char *buf = "x";
  xssize_t nwrote;
  xerror_t *error = NULL;

  g_assert_true (xpollable_output_stream_is_writable (G_POLLABLE_OUTPUT_STREAM (out)));

  nwrote = xoutput_stream_write (out, buf, 2, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nwrote, ==, 2);
  g_assert_true (xpollable_output_stream_is_writable (G_POLLABLE_OUTPUT_STREAM (out)));

/* Give the pipe a few ticks to propagate the write for sockets. On my
 * iMac i7, 40 works, 30 doesn't. */
  g_usleep (80L);

  check_source_readability_callback (GINT_TO_POINTER (TRUE));

  return XSOURCE_REMOVE;
}

static xboolean_t
check_source_and_quit_callback (xpointer_t user_data)
{
  check_source_readability_callback (user_data);
  xmain_loop_quit (loop);
  return XSOURCE_REMOVE;
}

static void
test_streams (void)
{
  xboolean_t readable;
  xerror_t *error = NULL;
  char buf[1];
  xssize_t nread;
  xsource_t *poll_source;
  xboolean_t success = FALSE;

  xassert (g_pollable_input_stream_can_poll (in));
  xassert (xpollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (out)));

  readable = g_pollable_input_stream_is_readable (in);
  xassert (!readable);

  nread = g_pollable_input_stream_read_nonblocking (in, buf, 1, NULL, &error);
  g_assert_cmpint (nread, ==, -1);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
  g_clear_error (&error);

  /* Create 4 sources, in decreasing order of priority:
   *   1. poll source on @in
   *   2. idle source that checks if @in is readable once
   *      (it won't be) and then removes itself
   *   3. idle source that writes a byte to @out, checks that
   *      @in is now readable, and removes itself
   *   4. idle source that checks if @in is readable once
   *      (it won't be, since the poll source will fire before
   *      this one does) and then quits the loop.
   *
   * If the poll source triggers before it should, then it will get a
   * %G_IO_ERROR_WOULD_BLOCK, and if check() fails in either
   * direction, we will catch it at some point.
   */

  poll_source = g_pollable_input_stream_create_source (in, NULL);
  xsource_set_priority (poll_source, 1);
  xsource_set_callback (poll_source, (xsource_func_t) poll_source_callback, &success, NULL);
  xsource_attach (poll_source, NULL);
  xsource_unref (poll_source);

  g_idle_add_full (2, check_source_readability_callback, GINT_TO_POINTER (FALSE), NULL);
  g_idle_add_full (3, write_callback, NULL, NULL);
  g_idle_add_full (4, check_source_and_quit_callback, GINT_TO_POINTER (FALSE), NULL);

  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);
  xmain_loop_unref (loop);

  g_assert_cmpint (success, ==, TRUE);
}

#ifdef G_OS_UNIX

#define g_assert_not_pollable(fd) \
  G_STMT_START {                                                        \
    in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (fd, FALSE)); \
    out = g_unix_output_stream_new (fd, FALSE);                         \
                                                                        \
    xassert (!g_pollable_input_stream_can_poll (in));                  \
    xassert (!xpollable_output_stream_can_poll (                      \
        G_POLLABLE_OUTPUT_STREAM (out)));                               \
                                                                        \
    g_clear_object (&in);                                               \
    g_clear_object (&out);                                              \
  } G_STMT_END

static void
test_pollable_unix_pipe (void)
{
  int pipefds[2], status;

  g_test_summary ("test_t that pipes are considered pollable, just like sockets");

  status = pipe (pipefds);
  g_assert_cmpint (status, ==, 0);

  in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (pipefds[0], TRUE));
  out = g_unix_output_stream_new (pipefds[1], TRUE);

  test_streams ();

  xobject_unref (in);
  xobject_unref (out);
}

static void
test_pollable_unix_pty (void)
{
  int (*openpty_impl) (int *, int *, char *, void *, void *);
  int a, b, status;
#ifdef LIBUTIL_SONAME
  void *handle;
#endif

  g_test_summary ("test_t that PTYs are considered pollable");

#ifdef LIBUTIL_SONAME
  handle = dlopen (LIBUTIL_SONAME, RTLD_GLOBAL | RTLD_LAZY);
  g_assert_nonnull (handle);
#endif

  openpty_impl = dlsym (RTLD_DEFAULT, "openpty");
  if (openpty_impl == NULL)
    {
      g_test_skip ("System does not support openpty()");
      goto close_libutil;
    }

  status = openpty_impl (&a, &b, NULL, NULL, NULL);
  if (status == -1)
    {
      g_test_skip ("Unable to open PTY");
      goto close_libutil;
    }

  in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (a, TRUE));
  out = g_unix_output_stream_new (b, TRUE);

  test_streams ();

  xobject_unref (in);
  xobject_unref (out);

  close (a);
  close (b);

close_libutil:
#ifdef LIBUTIL_SONAME
  dlclose (handle);
#else
  return;
#endif
}

static void
test_pollable_unix_file (void)
{
  int fd;

  g_test_summary ("test_t that regular files are not considered pollable");

  fd = g_open ("/etc/hosts", O_RDONLY, 0);
  if (fd == -1)
    {
      g_test_skip ("Unable to open /etc/hosts");
      return;
    }

  g_assert_not_pollable (fd);

  close (fd);
}

static void
test_pollable_unix_nulldev (void)
{
  int fd;

  g_test_summary ("test_t that /dev/null is not considered pollable, but only if "
                  "on a system where we are able to tell it apart from devices "
                  "that actually implement poll");

#if defined (HAVE_EPOLL_CREATE) || defined (HAVE_KQUEUE)
  fd = g_open ("/dev/null", O_RDWR, 0);
  g_assert_cmpint (fd, !=, -1);

  g_assert_not_pollable (fd);

  close (fd);
#else
  g_test_skip ("Cannot detect /dev/null as non-pollable on this system");
#endif
}

static void
test_pollable_converter (void)
{
  xconverter_t *converter;
  xerror_t *error = NULL;
  xinput_stream_t *ibase;
  int pipefds[2], status;

  status = pipe (pipefds);
  g_assert_cmpint (status, ==, 0);

  ibase = G_INPUT_STREAM (g_unix_input_stream_new (pipefds[0], TRUE));
  converter = XCONVERTER (xcharset_converter_new ("UTF-8", "UTF-8", &error));
  g_assert_no_error (error);

  in = G_POLLABLE_INPUT_STREAM (xconverter_input_stream_new (ibase, converter));
  xobject_unref (converter);
  xobject_unref (ibase);

  out = g_unix_output_stream_new (pipefds[1], TRUE);

  test_streams ();

  xobject_unref (in);
  xobject_unref (out);
}

#endif

static void
client_connected (xobject_t      *source,
		  xasync_result_t *result,
		  xpointer_t      user_data)
{
  xsocket_client_t *client = XSOCKET_CLIENT (source);
  xsocket_connection_t **conn = user_data;
  xerror_t *error = NULL;

  *conn = xsocket_client_connect_finish (client, result, &error);
  g_assert_no_error (error);
}

static void
server_connected (xobject_t      *source,
		  xasync_result_t *result,
		  xpointer_t      user_data)
{
  xsocket_listener_t *listener = XSOCKET_LISTENER (source);
  xsocket_connection_t **conn = user_data;
  xerror_t *error = NULL;

  *conn = xsocket_listener_accept_finish (listener, result, NULL, &error);
  g_assert_no_error (error);
}

static void
test_pollable_socket (void)
{
  xinet_address_t *iaddr;
  xsocket_address_t *saddr, *effective_address;
  xsocket_listener_t *listener;
  xsocket_client_t *client;
  xerror_t *error = NULL;
  xsocket_connection_t *client_conn = NULL, *server_conn = NULL;

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);

  listener = xsocket_listener_new ();
  xsocket_listener_add_address (listener, saddr,
				 XSOCKET_TYPE_STREAM,
				 XSOCKET_PROTOCOL_TCP,
				 NULL,
				 &effective_address,
				 &error);
  g_assert_no_error (error);
  xobject_unref (saddr);

  client = xsocket_client_new ();

  xsocket_client_connect_async (client,
				 XSOCKET_CONNECTABLE (effective_address),
				 NULL, client_connected, &client_conn);
  xsocket_listener_accept_async (listener, NULL,
				  server_connected, &server_conn);

  while (!client_conn || !server_conn)
    xmain_context_iteration (NULL, TRUE);

  in = G_POLLABLE_INPUT_STREAM (g_io_stream_get_input_stream (XIO_STREAM (client_conn)));
  out = g_io_stream_get_output_stream (XIO_STREAM (server_conn));

  test_streams ();

  xobject_unref (client_conn);
  xobject_unref (server_conn);
  xobject_unref (client);
  xobject_unref (listener);
  xobject_unref (effective_address);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#ifdef G_OS_UNIX
  g_test_add_func ("/pollable/unix/pipe", test_pollable_unix_pipe);
  g_test_add_func ("/pollable/unix/pty", test_pollable_unix_pty);
  g_test_add_func ("/pollable/unix/file", test_pollable_unix_file);
  g_test_add_func ("/pollable/unix/nulldev", test_pollable_unix_nulldev);
  g_test_add_func ("/pollable/converter", test_pollable_converter);
#endif
  g_test_add_func ("/pollable/socket", test_pollable_socket);

  return g_test_run();
}
