/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2011 Red Hat, Inc.
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

#include <gio/gio.h>
#include <glib/gstdio.h>

#include <gio/gcredentialsprivate.h>
#include <gio/gunixconnection.h>

#ifdef G_OS_UNIX
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <gio/gnetworking.h>
#endif

#ifdef G_OS_WIN32
#include "giowin32-afunix.h"
#endif

#include "gnetworkingprivate.h"

static xboolean_t ipv6_supported;

typedef struct {
  xsocket_t *server;
  xsocket_t *client;
  xsocket_family_t family;
  xthread_t *thread;
  xmain_loop_t *loop;
  xcancellable_t *cancellable; /* to shut down dgram echo server thread */
} IPTestData;

static xpointer_t
echo_server_dgram_thread (xpointer_t user_data)
{
  IPTestData *data = user_data;
  xsocket_address_t *sa;
  xcancellable_t *cancellable = data->cancellable;
  xsocket_t *sock;
  xerror_t *error = NULL;
  xssize_t nread, nwrote;
  xchar_t buf[128];

  sock = data->server;

  while (TRUE)
    {
      nread = xsocket_receive_from (sock, &sa, buf, sizeof (buf), cancellable, &error);
      if (error && xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        break;
      g_assert_no_error (error);
      g_assert_cmpint (nread, >=, 0);

      nwrote = xsocket_send_to (sock, sa, buf, nread, cancellable, &error);
      if (error && xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        break;
      g_assert_no_error (error);
      g_assert_cmpint (nwrote, ==, nread);

      xobject_unref (sa);
    }

  g_clear_error (&error);

  return NULL;
}

static xpointer_t
echo_server_thread (xpointer_t user_data)
{
  IPTestData *data = user_data;
  xsocket_t *sock;
  xerror_t *error = NULL;
  xssize_t nread, nwrote;
  xchar_t buf[128];

  sock = xsocket_accept (data->server, NULL, &error);
  g_assert_no_error (error);

  while (TRUE)
    {
      nread = xsocket_receive (sock, buf, sizeof (buf), NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (nread, >=, 0);

      if (nread == 0)
	break;

      nwrote = xsocket_send (sock, buf, nread, NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (nwrote, ==, nread);
    }

  xsocket_close (sock, &error);
  g_assert_no_error (error);
  xobject_unref (sock);
  return NULL;
}

static IPTestData *
create_server_full (xsocket_family_t   family,
                    xsocket_type_t     socket_type,
                    GThreadFunc     server_thread,
                    xboolean_t        v4mapped,
                    xerror_t        **error)
{
  IPTestData *data;
  xsocket_t *server;
  xsocket_address_t *addr;
  xinet_address_t *iaddr;

  data = g_slice_new (IPTestData);
  data->family = family;

  data->server = server = xsocket_new (family,
					socket_type,
					XSOCKET_PROTOCOL_DEFAULT,
					error);
  if (server == NULL)
    goto error;

  g_assert_cmpint (xsocket_get_family (server), ==, family);
  g_assert_cmpint (xsocket_get_socket_type (server), ==, socket_type);
  g_assert_cmpint (xsocket_get_protocol (server), ==, XSOCKET_PROTOCOL_DEFAULT);

  xsocket_set_blocking (server, TRUE);

#if defined (IPPROTO_IPV6) && defined (IPV6_V6ONLY)
  if (v4mapped)
    {
      xsocket_set_option (data->server, IPPROTO_IPV6, IPV6_V6ONLY, FALSE, NULL);
      if (!xsocket_speaks_ipv4 (data->server))
        {
          g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               "IPv6-only server cannot speak IPv4");
          goto error;
        }
    }
#endif

  if (v4mapped)
    iaddr = xinet_address_new_any (family);
  else
    iaddr = xinet_address_new_loopback (family);
  addr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);

  g_assert_cmpint (g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)), ==, 0);
  if (!xsocket_bind (server, addr, TRUE, error))
    {
      xobject_unref (addr);
      goto error;
    }
  xobject_unref (addr);

  addr = xsocket_get_local_address (server, error);
  if (addr == NULL)
    goto error;
  g_assert_cmpint (g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)), !=, 0);
  xobject_unref (addr);

  if (socket_type == XSOCKET_TYPE_STREAM)
    {
      if (!xsocket_listen (server, error))
        goto error;
    }
  else
    {
      data->cancellable = xcancellable_new ();
    }

  data->thread = xthread_new ("server", server_thread, data);

  return data;

error:
  g_clear_object (&data->server);
  g_slice_free (IPTestData, data);

  return NULL;
}

static IPTestData *
create_server (xsocket_family_t   family,
               GThreadFunc     server_thread,
               xboolean_t        v4mapped,
               xerror_t        **error)
{
  return create_server_full (family, XSOCKET_TYPE_STREAM, server_thread, v4mapped, error);
}

static const xchar_t *testbuf = "0123456789abcdef";

static xboolean_t
test_ip_async_read_ready (xsocket_t      *client,
			  xio_condition_t  cond,
			  xpointer_t      user_data)
{
  IPTestData *data = user_data;
  xerror_t *error = NULL;
  xssize_t len;
  xchar_t buf[128];

  g_assert_cmpint (cond, ==, G_IO_IN);

  len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  xmain_loop_quit (data->loop);

  return FALSE;
}

static xboolean_t
test_ip_async_write_ready (xsocket_t      *client,
			   xio_condition_t  cond,
			   xpointer_t      user_data)
{
  IPTestData *data = user_data;
  xerror_t *error = NULL;
  xsource_t *source;
  xssize_t len;

  g_assert_cmpint (cond, ==, G_IO_OUT);

  len = xsocket_send (client, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  source = xsocket_create_source (client, G_IO_IN, NULL);
  xsource_set_callback (source, (xsource_func_t)test_ip_async_read_ready,
			 data, NULL);
  xsource_attach (source, NULL);
  xsource_unref (source);

  return FALSE;
}

static xboolean_t
test_ip_async_timed_out (xsocket_t      *client,
			 xio_condition_t  cond,
			 xpointer_t      user_data)
{
  IPTestData *data = user_data;
  xerror_t *error = NULL;
  xsource_t *source;
  xssize_t len;
  xchar_t buf[128];

  if (data->family == XSOCKET_FAMILY_IPV4)
    {
      g_assert_cmpint (cond, ==, G_IO_IN);
      len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_cmpint (len, ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
      g_clear_error (&error);
    }

  source = xsocket_create_source (client, G_IO_OUT, NULL);
  xsource_set_callback (source, (xsource_func_t)test_ip_async_write_ready,
			 data, NULL);
  xsource_attach (source, NULL);
  xsource_unref (source);

  return FALSE;
}

static xboolean_t
test_ip_async_connected (xsocket_t      *client,
			 xio_condition_t  cond,
			 xpointer_t      user_data)
{
  IPTestData *data = user_data;
  xerror_t *error = NULL;
  xsource_t *source;
  xssize_t len;
  xchar_t buf[128];

  xsocket_check_connect_result (client, &error);
  g_assert_no_error (error);
  /* We do this after the check_connect_result, since that will give a
   * more useful assertion in case of error.
   */
  g_assert_cmpint (cond, ==, G_IO_OUT);

  g_assert (xsocket_is_connected (client));

  /* This adds 1 second to "make check", so let's just only do it once. */
  if (data->family == XSOCKET_FAMILY_IPV4)
    {
      len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_cmpint (len, ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
      g_clear_error (&error);

      source = xsocket_create_source (client, G_IO_IN, NULL);
      xsource_set_callback (source, (xsource_func_t)test_ip_async_timed_out,
			     data, NULL);
      xsource_attach (source, NULL);
      xsource_unref (source);
    }
  else
    test_ip_async_timed_out (client, 0, data);

  return FALSE;
}

static xboolean_t
idle_test_ip_async_connected (xpointer_t user_data)
{
  IPTestData *data = user_data;

  return test_ip_async_connected (data->client, G_IO_OUT, data);
}

static void
test_ip_async (xsocket_family_t family)
{
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_address_t *addr;
  xsource_t *source;
  xssize_t len;
  xchar_t buf[128];

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }
  g_assert_nonnull (data);

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = xsocket_new (family,
			 XSOCKET_TYPE_STREAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);
  data->client = client;

  g_assert_cmpint (xsocket_get_family (client), ==, family);
  g_assert_cmpint (xsocket_get_socket_type (client), ==, XSOCKET_TYPE_STREAM);
  g_assert_cmpint (xsocket_get_protocol (client), ==, XSOCKET_PROTOCOL_DEFAULT);

  xsocket_set_blocking (client, FALSE);
  xsocket_set_timeout (client, 1);

  if (xsocket_connect (client, addr, NULL, &error))
    {
      g_assert_no_error (error);
      g_idle_add (idle_test_ip_async_connected, data);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
      g_clear_error (&error);
      source = xsocket_create_source (client, G_IO_OUT, NULL);
      xsource_set_callback (source, (xsource_func_t)test_ip_async_connected,
			     data, NULL);
      xsource_attach (source, NULL);
      xsource_unref (source);
    }
  xobject_unref (addr);

  data->loop = xmain_loop_new (NULL, TRUE);
  xmain_loop_run (data->loop);
  xmain_loop_unref (data->loop);

  xsocket_shutdown (client, FALSE, TRUE, &error);
  g_assert_no_error (error);

  xthread_join (data->thread);

  if (family == XSOCKET_FAMILY_IPV4)
    {
      /* test_t that reading on a remote-closed socket gets back 0 bytes. */
      len = xsocket_receive_with_blocking (client, buf, sizeof (buf),
					    TRUE, NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (len, ==, 0);
    }
  else
    {
      /* test_t that writing to a remote-closed socket gets back CONNECTION_CLOSED. */
      len = xsocket_send_with_blocking (client, testbuf, strlen (testbuf) + 1,
					 TRUE, NULL, &error);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED);
      g_assert_cmpint (len, ==, -1);
      g_clear_error (&error);
    }

  xsocket_close (client, &error);
  g_assert_no_error (error);
  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  xobject_unref (data->server);
  xobject_unref (client);

  g_slice_free (IPTestData, data);
}

static void
test_ipv4_async (void)
{
  test_ip_async (XSOCKET_FAMILY_IPV4);
}

static void
test_ipv6_async (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_async (XSOCKET_FAMILY_IPV6);
}

static const xchar_t testbuf2[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static void
test_ip_sync (xsocket_family_t family)
{
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_address_t *addr;
  xssize_t len;
  xchar_t buf[128];

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = xsocket_new (family,
			 XSOCKET_TYPE_STREAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_family (client), ==, family);
  g_assert_cmpint (xsocket_get_socket_type (client), ==, XSOCKET_TYPE_STREAM);
  g_assert_cmpint (xsocket_get_protocol (client), ==, XSOCKET_PROTOCOL_DEFAULT);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  xsocket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_assert (xsocket_is_connected (client));
  xobject_unref (addr);

  /* This adds 1 second to "make check", so let's just only do it once. */
  if (family == XSOCKET_FAMILY_IPV4)
    {
      len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_cmpint (len, ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
      g_clear_error (&error);
    }

  len = xsocket_send (client, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  {
    xoutput_vector_t v[7] = { { NULL, 0 }, };

    v[0].buffer = testbuf2 + 0;
    v[0].size = 3;
    v[1].buffer = testbuf2 + 3;
    v[1].size = 5;
    v[2].buffer = testbuf2 + 3 + 5;
    v[2].size = 0;
    v[3].buffer = testbuf2 + 3 + 5;
    v[3].size = 6;
    v[4].buffer = testbuf2 + 3 + 5 + 6;
    v[4].size = 2;
    v[5].buffer = testbuf2 + 3 + 5 + 6 + 2;
    v[5].size = 1;
    v[6].buffer = testbuf2 + 3 + 5 + 6 + 2 + 1;
    v[6].size = strlen (testbuf2) - (3 + 5 + 6 + 2 + 1);

    len = xsocket_send_message (client, NULL, v, G_N_ELEMENTS (v), NULL, 0, 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));

    memset (buf, 0, sizeof (buf));
    len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));
    g_assert_cmpstr (testbuf2, ==, buf);
  }

  xsocket_shutdown (client, FALSE, TRUE, &error);
  g_assert_no_error (error);

  xthread_join (data->thread);

  if (family == XSOCKET_FAMILY_IPV4)
    {
      /* test_t that reading on a remote-closed socket gets back 0 bytes. */
      len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (len, ==, 0);
    }
  else
    {
      /* test_t that writing to a remote-closed socket gets back CONNECTION_CLOSED. */
      len = xsocket_send (client, testbuf, strlen (testbuf) + 1, NULL, &error);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED);
      g_assert_cmpint (len, ==, -1);
      g_clear_error (&error);
    }

  xsocket_close (client, &error);
  g_assert_no_error (error);
  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  xobject_unref (data->server);
  xobject_unref (client);

  g_slice_free (IPTestData, data);
}

static void
test_ipv4_sync (void)
{
  test_ip_sync (XSOCKET_FAMILY_IPV4);
}

static void
test_ipv6_sync (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_sync (XSOCKET_FAMILY_IPV6);
}

static void
test_ip_sync_dgram (xsocket_family_t family)
{
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_address_t *dest_addr;
  xssize_t len;
  xchar_t buf[128];

  data = create_server_full (family, XSOCKET_TYPE_DATAGRAM,
                             echo_server_dgram_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  dest_addr = xsocket_get_local_address (data->server, &error);

  client = xsocket_new (family,
			 XSOCKET_TYPE_DATAGRAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_family (client), ==, family);
  g_assert_cmpint (xsocket_get_socket_type (client), ==, XSOCKET_TYPE_DATAGRAM);
  g_assert_cmpint (xsocket_get_protocol (client), ==, XSOCKET_PROTOCOL_DEFAULT);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  len = xsocket_send_to (client, dest_addr, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  len = xsocket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  {
    xoutput_message_t m[3] = { { NULL, NULL, 0, 0, NULL, 0 }, };
    xinput_message_t im[3] = { { NULL, NULL, 0, 0, 0, NULL, 0 }, };
    xoutput_vector_t v[7] = { { NULL, 0 }, };
    xinput_vector_t iv[7] = { { NULL, 0 }, };

    v[0].buffer = testbuf2 + 0;
    v[0].size = 3;
    v[1].buffer = testbuf2 + 3;
    v[1].size = 5;
    v[2].buffer = testbuf2 + 3 + 5;
    v[2].size = 0;
    v[3].buffer = testbuf2 + 3 + 5;
    v[3].size = 6;
    v[4].buffer = testbuf2 + 3 + 5 + 6;
    v[4].size = 2;
    v[5].buffer = testbuf2 + 3 + 5 + 6 + 2;
    v[5].size = 1;
    v[6].buffer = testbuf2 + 3 + 5 + 6 + 2 + 1;
    v[6].size = strlen (testbuf2) - (3 + 5 + 6 + 2 + 1);

    iv[0].buffer = buf + 0;
    iv[0].size = 3;
    iv[1].buffer = buf + 3;
    iv[1].size = 5;
    iv[2].buffer = buf + 3 + 5;
    iv[2].size = 0;
    iv[3].buffer = buf + 3 + 5;
    iv[3].size = 6;
    iv[4].buffer = buf + 3 + 5 + 6;
    iv[4].size = 2;
    iv[5].buffer = buf + 3 + 5 + 6 + 2;
    iv[5].size = 1;
    iv[6].buffer = buf + 3 + 5 + 6 + 2 + 1;
    iv[6].size = sizeof (buf) - (3 + 5 + 6 + 2 + 1);

    len = xsocket_send_message (client, dest_addr, v, G_N_ELEMENTS (v), NULL, 0, 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));

    memset (buf, 0, sizeof (buf));
    len = xsocket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));
    g_assert_cmpstr (testbuf2, ==, buf);

    m[0].vectors = &v[0];
    m[0].num_vectors = 1;
    m[0].address = dest_addr;
    m[1].vectors = &v[0];
    m[1].num_vectors = 6;
    m[1].address = dest_addr;
    m[2].vectors = &v[6];
    m[2].num_vectors = 1;
    m[2].address = dest_addr;

    len = xsocket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, G_N_ELEMENTS (m));
    g_assert_cmpint (m[0].bytes_sent, ==, 3);
    g_assert_cmpint (m[1].bytes_sent, ==, 17);
    g_assert_cmpint (m[2].bytes_sent, ==, v[6].size);

    memset (buf, 0, sizeof (buf));
    len = xsocket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, 3);

    memset (buf, 0, sizeof (buf));
    len = xsocket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    /* v[0].size + v[1].size + v[2].size + v[3].size + v[4].size + v[5].size */
    g_assert_cmpint (len, ==, 17);
    g_assert (memcmp (testbuf2, buf, 17) == 0);

    memset (buf, 0, sizeof (buf));
    len = xsocket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, v[6].size);
    g_assert_cmpstr (buf, ==, v[6].buffer);

    /* reset since we're re-using the message structs */
    m[0].bytes_sent = 0;
    m[1].bytes_sent = 0;
    m[2].bytes_sent = 0;

    /* now try receiving multiple messages */
    len = xsocket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, G_N_ELEMENTS (m));
    g_assert_cmpint (m[0].bytes_sent, ==, 3);
    g_assert_cmpint (m[1].bytes_sent, ==, 17);
    g_assert_cmpint (m[2].bytes_sent, ==, v[6].size);

    im[0].vectors = &iv[0];
    im[0].num_vectors = 1;
    im[1].vectors = &iv[0];
    im[1].num_vectors = 6;
    im[2].vectors = &iv[6];
    im[2].num_vectors = 1;

    memset (buf, 0, sizeof (buf));
    len = xsocket_receive_messages (client, im, G_N_ELEMENTS (im), 0,
                                     NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, G_N_ELEMENTS (im));

    g_assert_cmpuint (im[0].bytes_received, ==, 3);
    /* v[0].size + v[1].size + v[2].size + v[3].size + v[4].size + v[5].size */
    g_assert_cmpuint (im[1].bytes_received, ==, 17);
    g_assert_cmpuint (im[2].bytes_received, ==, v[6].size);

    /* reset since we're re-using the message structs */
    m[0].bytes_sent = 0;
    m[1].bytes_sent = 0;
    m[2].bytes_sent = 0;

    /* now try to generate an early return by omitting the destination address on [1] */
    m[1].address = NULL;
    len = xsocket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, 1);

    g_assert_cmpint (m[0].bytes_sent, ==, 3);
    g_assert_cmpint (m[1].bytes_sent, ==, 0);
    g_assert_cmpint (m[2].bytes_sent, ==, 0);

    /* reset since we're re-using the message structs */
    m[0].bytes_sent = 0;
    m[1].bytes_sent = 0;
    m[2].bytes_sent = 0;

    /* now try to generate an error by omitting all destination addresses */
    m[0].address = NULL;
    m[1].address = NULL;
    m[2].address = NULL;
    len = xsocket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    /* This error code may vary between platforms and over time; it is not guaranteed API: */
#ifndef G_OS_WIN32
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
#else
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED);
#endif
    g_clear_error (&error);
    g_assert_cmpint (len, ==, -1);

    g_assert_cmpint (m[0].bytes_sent, ==, 0);
    g_assert_cmpint (m[1].bytes_sent, ==, 0);
    g_assert_cmpint (m[2].bytes_sent, ==, 0);

    len = xsocket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_cmpint (len, ==, 3);
  }

  xcancellable_cancel (data->cancellable);

  xthread_join (data->thread);

  xsocket_close (client, &error);
  g_assert_no_error (error);
  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  xobject_unref (data->server);
  xobject_unref (data->cancellable);
  xobject_unref (client);
  xobject_unref (dest_addr);

  g_slice_free (IPTestData, data);
}

static void
test_ipv4_sync_dgram (void)
{
  test_ip_sync_dgram (XSOCKET_FAMILY_IPV4);
}

static void
test_ipv6_sync_dgram (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_sync_dgram (XSOCKET_FAMILY_IPV6);
}

static xpointer_t
cancellable_thread_cb (xpointer_t data)
{
  xcancellable_t *cancellable = data;

  g_usleep (0.1 * G_USEC_PER_SEC);
  xcancellable_cancel (cancellable);
  xobject_unref (cancellable);

  return NULL;
}

static void
test_ip_sync_dgram_timeouts (xsocket_family_t family)
{
  xerror_t *error = NULL;
  xsocket_t *client = NULL;
  xcancellable_t *cancellable = NULL;
  xthread_t *cancellable_thread = NULL;
  xssize_t len;
#ifdef G_OS_WIN32
  xinet_address_t *iaddr;
  xsocket_address_t *addr;
#endif

  client = xsocket_new (family,
                         XSOCKET_TYPE_DATAGRAM,
                         XSOCKET_PROTOCOL_DEFAULT,
                         &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_family (client), ==, family);
  g_assert_cmpint (xsocket_get_socket_type (client), ==, XSOCKET_TYPE_DATAGRAM);
  g_assert_cmpint (xsocket_get_protocol (client), ==, XSOCKET_PROTOCOL_DEFAULT);

#ifdef G_OS_WIN32
  /* Winsock can't recv() on unbound udp socket */
  iaddr = xinet_address_new_loopback (family);
  addr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);
  xsocket_bind (client, addr, TRUE, &error);
  xobject_unref (addr);
  g_assert_no_error (error);
#endif

  /* No overall timeout: test the per-operation timeouts instead. */
  xsocket_set_timeout (client, 0);

  cancellable = xcancellable_new ();

  /* Check for timeouts when no server is running. */
  {
    sint64_t start_time;
    xinput_message_t im = { NULL, NULL, 0, 0, 0, NULL, 0 };
    xinput_vector_t iv = { NULL, 0 };
    xuint8_t buf[128];

    iv.buffer = buf;
    iv.size = sizeof (buf);

    im.vectors = &iv;
    im.num_vectors = 1;

    memset (buf, 0, sizeof (buf));

    /* Try a non-blocking read. */
    xsocket_set_blocking (client, FALSE);
    len = xsocket_receive_messages (client, &im, 1, 0  /* flags */,
                                     NULL, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_assert_cmpint (len, ==, -1);
    g_clear_error (&error);

    /* Try a timeout read. Can’t really validate the time taken more than
     * checking it’s positive. */
    xsocket_set_timeout (client, 1);
    xsocket_set_blocking (client, TRUE);
    start_time = g_get_monotonic_time ();
    len = xsocket_receive_messages (client, &im, 1, 0  /* flags */,
                                     NULL, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
    g_assert_cmpint (len, ==, -1);
    g_assert_cmpint (g_get_monotonic_time () - start_time, >, 0);
    g_clear_error (&error);

    /* Try a blocking read, cancelled from another thread. */
    xsocket_set_timeout (client, 0);
    cancellable_thread = xthread_new ("cancellable",
                                       cancellable_thread_cb,
                                       xobject_ref (cancellable));

    start_time = g_get_monotonic_time ();
    len = xsocket_receive_messages (client, &im, 1, 0  /* flags */,
                                     cancellable, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_cmpint (len, ==, -1);
    g_assert_cmpint (g_get_monotonic_time () - start_time, >, 0);
    g_clear_error (&error);

    xthread_join (cancellable_thread);
  }

  xsocket_close (client, &error);
  g_assert_no_error (error);

  xobject_unref (client);
  xobject_unref (cancellable);
}

static void
test_ipv4_sync_dgram_timeouts (void)
{
  test_ip_sync_dgram_timeouts (XSOCKET_FAMILY_IPV4);
}

static void
test_ipv6_sync_dgram_timeouts (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_sync_dgram_timeouts (XSOCKET_FAMILY_IPV6);
}

static xpointer_t
graceful_server_thread (xpointer_t user_data)
{
  IPTestData *data = user_data;
  xsocket_t *sock;
  xerror_t *error = NULL;
  xssize_t len;

  sock = xsocket_accept (data->server, NULL, &error);
  g_assert_no_error (error);

  len = xsocket_send (sock, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  return sock;
}

static void
test_close_graceful (void)
{
  xsocket_family_t family = XSOCKET_FAMILY_IPV4;
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client, *server;
  xsocket_address_t *addr;
  xssize_t len;
  xchar_t buf[128];

  data = create_server (family, graceful_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = xsocket_new (family,
			 XSOCKET_TYPE_STREAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_family (client), ==, family);
  g_assert_cmpint (xsocket_get_socket_type (client), ==, XSOCKET_TYPE_STREAM);
  g_assert_cmpint (xsocket_get_protocol (client), ==, XSOCKET_PROTOCOL_DEFAULT);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  xsocket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_assert (xsocket_is_connected (client));
  xobject_unref (addr);

  server = xthread_join (data->thread);

  /* similar to g_tcp_connection_set_graceful_disconnect(), but explicit */
  xsocket_shutdown (server, FALSE, TRUE, &error);
  g_assert_no_error (error);

  /* we must timeout */
  xsocket_condition_wait (client, G_IO_HUP, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_clear_error (&error);

  /* check that the remaining data is received */
  len = xsocket_receive (client, buf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  /* and only then the connection is closed */
  len = xsocket_receive (client, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, 0);

  xsocket_close (server, &error);
  g_assert_no_error (error);

  xsocket_close (client, &error);
  g_assert_no_error (error);

  xobject_unref (server);
  xobject_unref (data->server);
  xobject_unref (client);

  g_slice_free (IPTestData, data);
}

#if defined (IPPROTO_IPV6) && defined (IPV6_V6ONLY)
static xpointer_t
v4mapped_server_thread (xpointer_t user_data)
{
  IPTestData *data = user_data;
  xsocket_t *sock;
  xerror_t *error = NULL;
  xsocket_address_t *addr;

  sock = xsocket_accept (data->server, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_family (sock), ==, XSOCKET_FAMILY_IPV6);

  addr = xsocket_get_local_address (sock, &error);
  g_assert_no_error (error);
  g_assert_cmpint (xsocket_address_get_family (addr), ==, XSOCKET_FAMILY_IPV4);
  xobject_unref (addr);

  addr = xsocket_get_remote_address (sock, &error);
  g_assert_no_error (error);
  g_assert_cmpint (xsocket_address_get_family (addr), ==, XSOCKET_FAMILY_IPV4);
  xobject_unref (addr);

  xsocket_close (sock, &error);
  g_assert_no_error (error);
  xobject_unref (sock);
  return NULL;
}

static void
test_ipv6_v4mapped (void)
{
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_address_t *addr, *v4addr;
  xinet_address_t *iaddr;

  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  data = create_server (XSOCKET_FAMILY_IPV6, v4mapped_server_thread, TRUE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  client = xsocket_new (XSOCKET_FAMILY_IPV4,
			 XSOCKET_TYPE_STREAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);
  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  v4addr = g_inet_socket_address_new (iaddr, g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)));
  xobject_unref (iaddr);
  xobject_unref (addr);

  xsocket_connect (client, v4addr, NULL, &error);
  g_assert_no_error (error);
  g_assert (xsocket_is_connected (client));

  xthread_join (data->thread);

  xsocket_close (client, &error);
  g_assert_no_error (error);
  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  xobject_unref (data->server);
  xobject_unref (client);
  xobject_unref (v4addr);

  g_slice_free (IPTestData, data);
}
#endif

static void
test_timed_wait (void)
{
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_address_t *addr;
  sint64_t start_time;
  xint_t poll_duration;

  if (!g_test_thorough ())
    {
      g_test_skip ("Not running timing heavy test");
      return;
    }

  data = create_server (XSOCKET_FAMILY_IPV4, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = xsocket_new (XSOCKET_FAMILY_IPV4,
			 XSOCKET_TYPE_STREAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  xsocket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (addr);

  start_time = g_get_monotonic_time ();
  xsocket_condition_timed_wait (client, G_IO_IN, 100000 /* 100 ms */,
				 NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_clear_error (&error);
  poll_duration = g_get_monotonic_time () - start_time;

  g_assert_cmpint (poll_duration, >=, 98000);
  g_assert_cmpint (poll_duration, <, 112000);

  xsocket_close (client, &error);
  g_assert_no_error (error);

  xthread_join (data->thread);

  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  xobject_unref (data->server);
  xobject_unref (client);

  g_slice_free (IPTestData, data);
}

static int
duplicate_fd (int fd)
{
#ifdef G_OS_WIN32
  HANDLE newfd;

  if (!DuplicateHandle (GetCurrentProcess (),
                        (HANDLE)fd,
                        GetCurrentProcess (),
                        &newfd,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS))
    {
      return -1;
    }

  return (int)newfd;
#else
  return dup (fd);
#endif
}

static void
test_fd_reuse (void)
{
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_t *client2;
  xsocket_address_t *addr;
  int fd;
  xssize_t len;
  xchar_t buf[128];

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=741707");

  data = create_server (XSOCKET_FAMILY_IPV4, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = xsocket_new (XSOCKET_FAMILY_IPV4,
                         XSOCKET_TYPE_STREAM,
                         XSOCKET_PROTOCOL_DEFAULT,
                         &error);
  g_assert_no_error (error);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  xsocket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_assert (xsocket_is_connected (client));
  xobject_unref (addr);

  /* we have to dup otherwise the fd gets closed twice on unref */
  fd = duplicate_fd (xsocket_get_fd (client));
  client2 = xsocket_new_from_fd (fd, &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_family (client2), ==, xsocket_get_family (client));
  g_assert_cmpint (xsocket_get_socket_type (client2), ==, xsocket_get_socket_type (client));
  g_assert_cmpint (xsocket_get_protocol (client2), ==, XSOCKET_PROTOCOL_TCP);

  len = xsocket_send (client2, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  len = xsocket_receive (client2, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  xsocket_shutdown (client, FALSE, TRUE, &error);
  g_assert_no_error (error);
  /* The semantics of dup()+shutdown() are ambiguous; this call will succeed
   * on Linux, but return ENOTCONN on OS X.
   */
  xsocket_shutdown (client2, FALSE, TRUE, NULL);

  xthread_join (data->thread);

  xsocket_close (client, &error);
  g_assert_no_error (error);
  xsocket_close (client2, &error);
  g_assert_no_error (error);
  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  g_assert_cmpint (xsocket_get_fd (client), ==, -1);
  g_assert_cmpint (xsocket_get_fd (client2), ==, -1);
  g_assert_cmpint (xsocket_get_fd (data->server), ==, -1);

  xobject_unref (data->server);
  xobject_unref (client);
  xobject_unref (client2);

  g_slice_free (IPTestData, data);
}

static void
test_sockaddr (void)
{
  struct sockaddr_in6 sin6, gsin6;
  xsocket_address_t *saddr;
  xinet_socket_address_t *isaddr;
  xinet_address_t *iaddr;
  xerror_t *error = NULL;

  memset (&sin6, 0, sizeof (sin6));
  sin6.sin6_family = AF_INET6;
  sin6.sin6_addr = in6addr_loopback;
  sin6.sin6_port = g_htons (42);
  sin6.sin6_scope_id = 17;
  sin6.sin6_flowinfo = 1729;

  saddr = xsocket_address_new_from_native (&sin6, sizeof (sin6));
  g_assert (X_IS_INET_SOCKET_ADDRESS (saddr));

  isaddr = G_INET_SOCKET_ADDRESS (saddr);
  iaddr = g_inet_socket_address_get_address (isaddr);
  g_assert_cmpint (xinet_address_get_family (iaddr), ==, XSOCKET_FAMILY_IPV6);
  g_assert (xinet_address_get_is_loopback (iaddr));

  g_assert_cmpint (g_inet_socket_address_get_port (isaddr), ==, 42);
  g_assert_cmpint (g_inet_socket_address_get_scope_id (isaddr), ==, 17);
  g_assert_cmpint (g_inet_socket_address_get_flowinfo (isaddr), ==, 1729);

  xsocket_address_to_native (saddr, &gsin6, sizeof (gsin6), &error);
  g_assert_no_error (error);

  g_assert (memcmp (&sin6.sin6_addr, &gsin6.sin6_addr, sizeof (struct in6_addr)) == 0);
  g_assert_cmpint (sin6.sin6_port, ==, gsin6.sin6_port);
  g_assert_cmpint (sin6.sin6_scope_id, ==, gsin6.sin6_scope_id);
  g_assert_cmpint (sin6.sin6_flowinfo, ==, gsin6.sin6_flowinfo);

  xobject_unref (saddr);
}

static void
bind_win32_unixfd (int fd)
{
#ifdef G_OS_WIN32
  xint_t len, ret;
  struct sockaddr_un addr;

  memset (&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  len = g_snprintf (addr.sun_path, sizeof addr.sun_path, "%s" G_DIR_SEPARATOR_S "%d.sock", g_get_tmp_dir (), fd);
  g_assert_cmpint (len, <=, sizeof addr.sun_path);
  ret = bind (fd, (struct sockaddr *)&addr, sizeof addr);
  g_assert_cmpint (ret, ==, 0);
  g_remove (addr.sun_path);
#endif
}

static void
test_unix_from_fd (void)
{
  xint_t fd;
  xerror_t *error;
  xsocket_t *s;

  fd = socket (AF_UNIX, SOCK_STREAM, 0);
  g_assert_cmpint (fd, !=, -1);

  bind_win32_unixfd (fd);

  error = NULL;
  s = xsocket_new_from_fd (fd, &error);
  g_assert_no_error (error);
  g_assert_cmpint (xsocket_get_family (s), ==, XSOCKET_FAMILY_UNIX);
  g_assert_cmpint (xsocket_get_socket_type (s), ==, XSOCKET_TYPE_STREAM);
  g_assert_cmpint (xsocket_get_protocol (s), ==, XSOCKET_PROTOCOL_DEFAULT);
  xobject_unref (s);
}

static void
test_unix_connection (void)
{
  xint_t fd;
  xerror_t *error;
  xsocket_t *s;
  xsocket_connection_t *c;

  fd = socket (AF_UNIX, SOCK_STREAM, 0);
  g_assert_cmpint (fd, !=, -1);

  bind_win32_unixfd (fd);

  error = NULL;
  s = xsocket_new_from_fd (fd, &error);
  g_assert_no_error (error);
  c = xsocket_connection_factory_create_connection (s);
  g_assert (X_IS_UNIX_CONNECTION (c));
  xobject_unref (c);
  xobject_unref (s);
}

#ifdef G_OS_UNIX
static xsocket_connection_t *
create_connection_for_fd (int fd)
{
  xerror_t *err = NULL;
  xsocket_t *socket;
  xsocket_connection_t *connection;

  socket = xsocket_new_from_fd (fd, &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (socket));
  connection = xsocket_connection_factory_create_connection (socket);
  g_assert (X_IS_UNIX_CONNECTION (connection));
  xobject_unref (socket);
  return connection;
}

#define TEST_DATA "failure to say failure to say 'i love gnome-panel!'."

static void
test_unix_connection_ancillary_data (void)
{
  xerror_t *err = NULL;
  xint_t pv[2], sv[3];
  xint_t status, fd, len;
  char buffer[1024];
  pid_t pid;

  status = pipe (pv);
  g_assert_cmpint (status, ==, 0);

  status = socketpair (PF_UNIX, SOCK_STREAM, 0, sv);
  g_assert_cmpint (status, ==, 0);

  pid = fork ();
  g_assert_cmpint (pid, >=, 0);

  /* Child: close its copy of the write end of the pipe, receive it
   * again from the parent over the socket, and write some text to it.
   *
   * Parent: send the write end of the pipe (still open for the
   * parent) over the socket, close it, and read some text from the
   * read end of the pipe.
   */
  if (pid == 0)
    {
      xsocket_connection_t *connection;

      close (sv[1]);
      connection = create_connection_for_fd (sv[0]);

      status = close (pv[1]);
      g_assert_cmpint (status, ==, 0);

      err = NULL;
      fd = g_unix_connection_receive_fd (G_UNIX_CONNECTION (connection), NULL,
					 &err);
      g_assert_no_error (err);
      g_assert_cmpint (fd, >, -1);
      xobject_unref (connection);

      do
	len = write (fd, TEST_DATA, sizeof (TEST_DATA));
      while (len == -1 && errno == EINTR);
      g_assert_cmpint (len, ==, sizeof (TEST_DATA));
      exit (0);
    }
  else
    {
      xsocket_connection_t *connection;

      close (sv[0]);
      connection = create_connection_for_fd (sv[1]);

      err = NULL;
      g_unix_connection_send_fd (G_UNIX_CONNECTION (connection), pv[1], NULL,
				 &err);
      g_assert_no_error (err);
      xobject_unref (connection);

      status = close (pv[1]);
      g_assert_cmpint (status, ==, 0);

      memset (buffer, 0xff, sizeof buffer);
      do
	len = read (pv[0], buffer, sizeof buffer);
      while (len == -1 && errno == EINTR);

      g_assert_cmpint (len, ==, sizeof (TEST_DATA));
      g_assert_cmpstr (buffer, ==, TEST_DATA);

      waitpid (pid, &status, 0);
      g_assert (WIFEXITED (status));
      g_assert_cmpint (WEXITSTATUS (status), ==, 0);
    }

  /* TODO: add test for g_unix_connection_send_credentials() and
   * g_unix_connection_receive_credentials().
   */
}
#endif

static xboolean_t
postmortem_source_cb (xsocket_t      *socket,
                      xio_condition_t  condition,
                      xpointer_t      user_data)
{
  xboolean_t *been_here = user_data;

  g_assert_cmpint (condition, ==, G_IO_NVAL);

  *been_here = TRUE;
  return FALSE;
}

static void
test_source_postmortem (void)
{
  xmain_context_t *context;
  xsocket_t *socket;
  xsource_t *source;
  xerror_t *error = NULL;
  xboolean_t callback_visited = FALSE;

  socket = xsocket_new (XSOCKET_FAMILY_UNIX, XSOCKET_TYPE_STREAM, XSOCKET_PROTOCOL_DEFAULT, &error);
  g_assert_no_error (error);

  context = xmain_context_new ();

  source = xsocket_create_source (socket, G_IO_IN, NULL);
  xsource_set_callback (source, (xsource_func_t) postmortem_source_cb,
                         &callback_visited, NULL);
  xsource_attach (source, context);
  xsource_unref (source);

  xsocket_close (socket, &error);
  g_assert_no_error (error);
  xobject_unref (socket);

  /* test_t that, after a socket is closed, its source callback should be called
   * exactly once. */
  xmain_context_iteration (context, FALSE);
  g_assert (callback_visited);
  g_assert (!xmain_context_pending (context));

  xmain_context_unref (context);
}

static void
test_reuse_tcp (void)
{
  xsocket_t *sock1, *sock2;
  xerror_t *error = NULL;
  xinet_address_t *iaddr;
  xsocket_address_t *addr;

  sock1 = xsocket_new (XSOCKET_FAMILY_IPV4,
                        XSOCKET_TYPE_STREAM,
                        XSOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);
  xsocket_bind (sock1, addr, TRUE, &error);
  xobject_unref (addr);
  g_assert_no_error (error);

  xsocket_listen (sock1, &error);
  g_assert_no_error (error);

  sock2 = xsocket_new (XSOCKET_FAMILY_IPV4,
                        XSOCKET_TYPE_STREAM,
                        XSOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  addr = xsocket_get_local_address (sock1, &error);
  g_assert_no_error (error);
  xsocket_bind (sock2, addr, TRUE, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_ADDRESS_IN_USE);
  g_clear_error (&error);
  xobject_unref (addr);

  xobject_unref (sock1);
  xobject_unref (sock2);
}

static void
test_reuse_udp (void)
{
  xsocket_t *sock1, *sock2;
  xerror_t *error = NULL;
  xinet_address_t *iaddr;
  xsocket_address_t *addr;

  sock1 = xsocket_new (XSOCKET_FAMILY_IPV4,
                        XSOCKET_TYPE_DATAGRAM,
                        XSOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);
  xsocket_bind (sock1, addr, TRUE, &error);
  xobject_unref (addr);
  g_assert_no_error (error);

  sock2 = xsocket_new (XSOCKET_FAMILY_IPV4,
                        XSOCKET_TYPE_DATAGRAM,
                        XSOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  addr = xsocket_get_local_address (sock1, &error);
  g_assert_no_error (error);
  xsocket_bind (sock2, addr, TRUE, &error);
  xobject_unref (addr);
  g_assert_no_error (error);

  xobject_unref (sock1);
  xobject_unref (sock2);
}

static void
test_get_available (xconstpointer user_data)
{
  xsocket_type_t socket_type = GPOINTER_TO_UINT (user_data);
  xerror_t *err = NULL;
  xsocket_t *listener, *server, *client;
  xinet_address_t *addr;
  xsocket_address_t *saddr, *boundaddr;
  xchar_t data[] = "0123456789abcdef";
  xchar_t buf[34];
  xssize_t nread;

  listener = xsocket_new (XSOCKET_FAMILY_IPV4,
                           socket_type,
                           XSOCKET_PROTOCOL_DEFAULT,
                           &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (listener));

  client = xsocket_new (XSOCKET_FAMILY_IPV4,
                         socket_type,
                         XSOCKET_PROTOCOL_DEFAULT,
                         &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (client));

  if (socket_type == XSOCKET_TYPE_STREAM)
    {
      xsocket_set_option (client, IPPROTO_TCP, TCP_NODELAY, TRUE, &err);
      g_assert_no_error (err);
    }

  addr = xinet_address_new_any (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, 0);

  xsocket_bind (listener, saddr, TRUE, &err);
  g_assert_no_error (err);
  xobject_unref (saddr);
  xobject_unref (addr);

  boundaddr = xsocket_get_local_address (listener, &err);
  g_assert_no_error (err);

  addr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (boundaddr)));
  xobject_unref (addr);
  xobject_unref (boundaddr);

  if (socket_type == XSOCKET_TYPE_STREAM)
    {
      xsocket_listen (listener, &err);
      g_assert_no_error (err);
      xsocket_connect (client, saddr, NULL, &err);
      g_assert_no_error (err);

      server = xsocket_accept (listener, NULL, &err);
      g_assert_no_error (err);
      xsocket_set_blocking (server, FALSE);
      xobject_unref (listener);
    }
  else
    server = listener;

  xsocket_send_to (client, saddr, data, sizeof (data), NULL, &err);
  g_assert_no_error (err);

  while (!xsocket_condition_wait (server, G_IO_IN, NULL, NULL))
    ;
  g_assert_cmpint (xsocket_get_available_bytes (server), ==, sizeof (data));

  xsocket_send_to (client, saddr, data, sizeof (data), NULL, &err);
  g_assert_no_error (err);

  /* We need to wait until the data has actually been copied into the
   * server socket's buffers, but xsocket_condition_wait() won't help
   * here since the socket is definitely already readable. So there's
   * a race condition in checking its available bytes. In the TCP
   * case, we poll for a bit until the new data shows up. In the UDP
   * case, there's not much we can do, but at least the failure mode
   * is passes-when-it-shouldn't, not fails-when-it-shouldn't.
   */
  if (socket_type == XSOCKET_TYPE_STREAM)
    {
      int tries;

      for (tries = 0; tries < 100; tries++)
        {
          xssize_t res = xsocket_get_available_bytes (server);
          if ((res == -1) || ((xsize_t) res > sizeof (data)))
            break;
          g_usleep (100000);
        }

      g_assert_cmpint (xsocket_get_available_bytes (server), ==, 2 * sizeof (data));
    }
  else
    {
      g_usleep (100000);
      g_assert_cmpint (xsocket_get_available_bytes (server), ==, sizeof (data));
    }

  g_assert_cmpint (sizeof (buf), >=, 2 * sizeof (data));
  nread = xsocket_receive (server, buf, sizeof (buf), NULL, &err);
  g_assert_no_error (err);

  if (socket_type == XSOCKET_TYPE_STREAM)
    {
      g_assert_cmpint (nread, ==, 2 * sizeof (data));
      g_assert_cmpint (xsocket_get_available_bytes (server), ==, 0);
    }
  else
    {
      g_assert_cmpint (nread, ==, sizeof (data));
      g_assert_cmpint (xsocket_get_available_bytes (server), ==, sizeof (data));
    }

  nread = xsocket_receive (server, buf, sizeof (buf), NULL, &err);
  if (socket_type == XSOCKET_TYPE_STREAM)
    {
      g_assert_cmpint (nread, ==, -1);
      g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
      g_clear_error (&err);
    }
  else
    {
      g_assert_cmpint (nread, ==, sizeof (data));
      g_assert_no_error (err);
    }

  g_assert_cmpint (xsocket_get_available_bytes (server), ==, 0);

  xsocket_close (server, &err);
  g_assert_no_error (err);

  xobject_unref (saddr);
  xobject_unref (server);
  xobject_unref (client);
}

typedef struct {
  xinput_stream_t *is;
  xoutput_stream_t *os;
  const xuint8_t *write_data;
  xuint8_t *read_data;
} TestReadWriteData;

static xpointer_t
test_read_write_write_thread (xpointer_t user_data)
{
  TestReadWriteData *data = user_data;
  xsize_t bytes_written;
  xerror_t *error = NULL;
  xboolean_t res;

  res = xoutput_stream_write_all (data->os, data->write_data, 1024, &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_written, ==, 1024);

  return NULL;
}

static xpointer_t
test_read_write_read_thread (xpointer_t user_data)
{
  TestReadWriteData *data = user_data;
  xsize_t bytes_read;
  xerror_t *error = NULL;
  xboolean_t res;

  res = xinput_stream_read_all (data->is, data->read_data, 1024, &bytes_read, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 1024);

  return NULL;
}

static xpointer_t
test_read_write_writev_thread (xpointer_t user_data)
{
  TestReadWriteData *data = user_data;
  xsize_t bytes_written;
  xerror_t *error = NULL;
  xboolean_t res;
  xoutput_vector_t vectors[3];

  vectors[0].buffer = data->write_data;
  vectors[0].size = 256;
  vectors[1].buffer = data->write_data + 256;
  vectors[1].size = 256;
  vectors[2].buffer = data->write_data + 512;
  vectors[2].size = 512;

  res = xoutput_stream_writev_all (data->os, vectors, G_N_ELEMENTS (vectors), &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_written, ==, 1024);

  return NULL;
}

/* test if normal read/write/writev via the xsocket_t*Streams works on TCP sockets */
static void
test_read_write (xconstpointer user_data)
{
  xboolean_t writev = GPOINTER_TO_INT (user_data);
  xerror_t *err = NULL;
  xsocket_t *listener, *server, *client;
  xinet_address_t *addr;
  xsocket_address_t *saddr, *boundaddr;
  TestReadWriteData data;
  xuint8_t data_write[1024], data_read[1024];
  xsocket_connection_t *server_stream, *client_stream;
  xthread_t *write_thread, *read_thread;
  xuint_t i;

  listener = xsocket_new (XSOCKET_FAMILY_IPV4,
                           XSOCKET_TYPE_STREAM,
                           XSOCKET_PROTOCOL_DEFAULT,
                           &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (listener));

  client = xsocket_new (XSOCKET_FAMILY_IPV4,
                         XSOCKET_TYPE_STREAM,
                         XSOCKET_PROTOCOL_DEFAULT,
                         &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (client));

  addr = xinet_address_new_any (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, 0);

  xsocket_bind (listener, saddr, TRUE, &err);
  g_assert_no_error (err);
  xobject_unref (saddr);
  xobject_unref (addr);

  boundaddr = xsocket_get_local_address (listener, &err);
  g_assert_no_error (err);

  xsocket_listen (listener, &err);
  g_assert_no_error (err);

  addr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (boundaddr)));
  xobject_unref (addr);
  xobject_unref (boundaddr);

  xsocket_connect (client, saddr, NULL, &err);
  g_assert_no_error (err);

  server = xsocket_accept (listener, NULL, &err);
  g_assert_no_error (err);
  xsocket_set_blocking (server, FALSE);
  xobject_unref (listener);

  server_stream = xsocket_connection_factory_create_connection (server);
  g_assert_nonnull (server_stream);
  client_stream = xsocket_connection_factory_create_connection (client);
  g_assert_nonnull (client_stream);

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  data.is = g_io_stream_get_input_stream (XIO_STREAM (server_stream));
  data.os = g_io_stream_get_output_stream (XIO_STREAM (client_stream));
  data.read_data = data_read;
  data.write_data = data_write;

  if (writev)
    write_thread = xthread_new ("writer", test_read_write_writev_thread, &data);
  else
    write_thread = xthread_new ("writer", test_read_write_write_thread, &data);
  read_thread = xthread_new ("reader", test_read_write_read_thread, &data);

  xthread_join (write_thread);
  xthread_join (read_thread);

  g_assert_cmpmem (data_write, sizeof data_write, data_read, sizeof data_read);

  xsocket_close (server, &err);
  g_assert_no_error (err);

  xobject_unref (server_stream);
  xobject_unref (client_stream);

  xobject_unref (saddr);
  xobject_unref (server);
  xobject_unref (client);
}

#ifdef SO_NOSIGPIPE
static void
test_nosigpipe (void)
{
  xsocket_t *sock;
  xerror_t *error = NULL;
  xint_t value;

  sock = xsocket_new (AF_INET,
                       XSOCKET_TYPE_STREAM,
                       XSOCKET_PROTOCOL_DEFAULT,
                       &error);
  g_assert_no_error (error);

  xsocket_get_option (sock, SOL_SOCKET, SO_NOSIGPIPE, &value, &error);
  g_assert_no_error (error);
  g_assert_true (value);

  xobject_unref (sock);
}
#endif

#if G_CREDENTIALS_SUPPORTED
static xpointer_t client_setup_thread (xpointer_t user_data);

static void
test_credentials_tcp_client (void)
{
  const xsocket_family_t family = XSOCKET_FAMILY_IPV4;
  IPTestData *data;
  xerror_t *error = NULL;
  xsocket_t *client;
  xsocket_address_t *addr;
  xcredentials_t *creds;

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = xsocket_new (family,
			 XSOCKET_TYPE_STREAM,
			 XSOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  xsocket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (addr);

  creds = xsocket_get_credentials (client, &error);
  if (creds != NULL)
    {
      xchar_t *str = xcredentials_to_string (creds);
      g_print ("Supported on this OS: %s\n", str);
      g_free (str);
      g_clear_object (&creds);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_print ("Unsupported on this OS: %s\n", error->message);
      g_clear_error (&error);
    }

  xsocket_close (client, &error);
  g_assert_no_error (error);

  xthread_join (data->thread);

  xsocket_close (data->server, &error);
  g_assert_no_error (error);

  xobject_unref (data->server);
  xobject_unref (client);

  g_slice_free (IPTestData, data);
}

static void
test_credentials_tcp_server (void)
{
  const xsocket_family_t family = XSOCKET_FAMILY_IPV4;
  IPTestData *data;
  xsocket_t *server;
  xerror_t *error = NULL;
  xsocket_address_t *addr = NULL;
  xinet_address_t *iaddr = NULL;
  xsocket_t *sock = NULL;
  xcredentials_t *creds;

  data = g_slice_new0 (IPTestData);
  data->family = family;
  data->server = server = xsocket_new (family,
					XSOCKET_TYPE_STREAM,
					XSOCKET_PROTOCOL_DEFAULT,
					&error);
  if (error != NULL)
    goto skip;

  xsocket_set_blocking (server, TRUE);

  iaddr = xinet_address_new_loopback (family);
  addr = g_inet_socket_address_new (iaddr, 0);

  if (!xsocket_bind (server, addr, TRUE, &error))
    goto skip;

  if (!xsocket_listen (server, &error))
    goto skip;

  data->thread = xthread_new ("client", client_setup_thread, data);

  sock = xsocket_accept (server, NULL, &error);
  g_assert_no_error (error);

  creds = xsocket_get_credentials (sock, &error);
  if (creds != NULL)
    {
      xchar_t *str = xcredentials_to_string (creds);
      g_print ("Supported on this OS: %s\n", str);
      g_free (str);
      g_clear_object (&creds);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_print ("Unsupported on this OS: %s\n", error->message);
      g_clear_error (&error);
    }

  goto beach;

skip:
  g_test_skip_printf ("Failed to create server: %s", error->message);
  goto beach;

beach:
  {
    g_clear_error (&error);

    g_clear_object (&sock);
    g_clear_object (&addr);
    g_clear_object (&iaddr);

    g_clear_pointer (&data->thread, xthread_join);
    g_clear_object (&data->server);
    g_clear_object (&data->client);

    g_slice_free (IPTestData, data);
  }
}

static xpointer_t
client_setup_thread (xpointer_t user_data)
{
  IPTestData *data = user_data;
  xsocket_address_t *addr;
  xsocket_t *client;
  xerror_t *error = NULL;

  addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  data->client = client = xsocket_new (data->family,
					XSOCKET_TYPE_STREAM,
					XSOCKET_PROTOCOL_DEFAULT,
					&error);
  g_assert_no_error (error);

  xsocket_set_blocking (client, TRUE);
  xsocket_set_timeout (client, 1);

  xsocket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);

  xobject_unref (addr);

  return NULL;
}

#ifdef G_OS_WIN32
/*
 * _g_win32_socketpair:
 *
 * Create a pair of connected sockets, similar to POSIX/BSD socketpair().
 *
 * Windows does not (yet) provide a socketpair() function. However, since the
 * introduction of AF_UNIX sockets, it is possible to implement a fairly close
 * function.
 */
static xint_t
_g_win32_socketpair (xint_t            domain,
                     xint_t            type,
                     xint_t            protocol,
                     xint_t            sv[2])
{
  struct sockaddr_un addr = { 0, };
  socklen_t socklen;
  SOCKET listener = INVALID_SOCKET;
  SOCKET client = INVALID_SOCKET;
  SOCKET server = INVALID_SOCKET;
  xchar_t *path = NULL;
  int tmpfd, rv = -1;
  u_long arg, br;

  g_return_val_if_fail (sv != NULL, -1);

  addr.sun_family = AF_UNIX;
  socklen = sizeof (addr);

  tmpfd = xfile_open_tmp (NULL, &path, NULL);
  if (tmpfd == -1)
    {
      WSASetLastError (WSAEACCES);
      goto out;
    }

  g_close (tmpfd, NULL);

  if (strlen (path) >= sizeof (addr.sun_path))
    {
      WSASetLastError (WSAEACCES);
      goto out;
    }

  strncpy (addr.sun_path, path, sizeof (addr.sun_path) - 1);

  listener = socket (domain, type, protocol);
  if (listener == INVALID_SOCKET)
    goto out;

  if (DeleteFile (path) == 0)
    {
      if (GetLastError () != ERROR_FILE_NOT_FOUND)
        goto out;
    }

  if (bind (listener, (struct sockaddr *) &addr, socklen) == SOCKET_ERROR)
    goto out;

  if (listen (listener, 1) == SOCKET_ERROR)
    goto out;

  client = socket (domain, type, protocol);
  if (client == INVALID_SOCKET)
    goto out;

  arg = 1;
  if (ioctlsocket (client, FIONBIO, &arg) == SOCKET_ERROR)
    goto out;

  if (connect (client, (struct sockaddr *) &addr, socklen) == SOCKET_ERROR &&
      WSAGetLastError () != WSAEWOULDBLOCK)
    goto out;

  server = accept (listener, NULL, NULL);
  if (server == INVALID_SOCKET)
    goto out;

  arg = 0;
  if (ioctlsocket (client, FIONBIO, &arg) == SOCKET_ERROR)
    goto out;

  if (WSAIoctl (server, SIO_AF_UNIX_GETPEERPID,
                NULL, 0U,
                &arg, sizeof (arg), &br,
                NULL, NULL) == SOCKET_ERROR || arg != GetCurrentProcessId ())
    {
      WSASetLastError (WSAEACCES);
      goto out;
    }

  sv[0] = server;
  server = INVALID_SOCKET;
  sv[1] = client;
  client = INVALID_SOCKET;
  rv = 0;

 out:
  if (listener != INVALID_SOCKET)
    closesocket (listener);
  if (client != INVALID_SOCKET)
    closesocket (client);
  if (server != INVALID_SOCKET)
    closesocket (server);

  DeleteFile (path);
  g_free (path);
  return rv;
}
#endif /* G_OS_WIN32 */

static void
test_credentials_unix_socketpair (void)
{
  xint_t fds[2];
  xint_t status;
  xsocket_t *sock;
  xerror_t *error = NULL;
  xcredentials_t *creds;

#ifdef G_OS_WIN32
  status = _g_win32_socketpair (PF_UNIX, SOCK_STREAM, 0, fds);
#else
  status = socketpair (PF_UNIX, SOCK_STREAM, 0, fds);
#endif
  g_assert_cmpint (status, ==, 0);

  sock = xsocket_new_from_fd (fds[0], &error);

  creds = xsocket_get_credentials (sock, &error);
  if (creds != NULL)
    {
      xchar_t *str = xcredentials_to_string (creds);
      g_print ("Supported on this OS: %s\n", str);
      g_free (str);
      g_clear_object (&creds);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_print ("Unsupported on this OS: %s\n", error->message);
      g_clear_error (&error);
    }

  xobject_unref (sock);
  g_close (fds[1], NULL);
}
#endif

int
main (int   argc,
      char *argv[])
{
  xsocket_t *sock;
  xerror_t *error = NULL;

  g_test_init (&argc, &argv, NULL);

  sock = xsocket_new (XSOCKET_FAMILY_IPV6,
                       XSOCKET_TYPE_STREAM,
                       XSOCKET_PROTOCOL_DEFAULT,
                       &error);
  if (sock != NULL)
    {
      ipv6_supported = TRUE;
      xobject_unref (sock);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_clear_error (&error);
    }

  g_test_add_func ("/socket/ipv4_sync", test_ipv4_sync);
  g_test_add_func ("/socket/ipv4_async", test_ipv4_async);
  g_test_add_func ("/socket/ipv6_sync", test_ipv6_sync);
  g_test_add_func ("/socket/ipv6_async", test_ipv6_async);
  g_test_add_func ("/socket/ipv4_sync/datagram", test_ipv4_sync_dgram);
  g_test_add_func ("/socket/ipv4_sync/datagram/timeouts", test_ipv4_sync_dgram_timeouts);
  g_test_add_func ("/socket/ipv6_sync/datagram", test_ipv6_sync_dgram);
  g_test_add_func ("/socket/ipv6_sync/datagram/timeouts", test_ipv6_sync_dgram_timeouts);
#if defined (IPPROTO_IPV6) && defined (IPV6_V6ONLY)
  g_test_add_func ("/socket/ipv6_v4mapped", test_ipv6_v4mapped);
#endif
  g_test_add_func ("/socket/close_graceful", test_close_graceful);
  g_test_add_func ("/socket/timed_wait", test_timed_wait);
  g_test_add_func ("/socket/fd_reuse", test_fd_reuse);
  g_test_add_func ("/socket/address", test_sockaddr);
  g_test_add_func ("/socket/unix-from-fd", test_unix_from_fd);
  g_test_add_func ("/socket/unix-connection", test_unix_connection);
#ifdef G_OS_UNIX
  g_test_add_func ("/socket/unix-connection-ancillary-data", test_unix_connection_ancillary_data);
#endif
  g_test_add_func ("/socket/source-postmortem", test_source_postmortem);
  g_test_add_func ("/socket/reuse/tcp", test_reuse_tcp);
  g_test_add_func ("/socket/reuse/udp", test_reuse_udp);
  g_test_add_data_func ("/socket/get_available/datagram", GUINT_TO_POINTER (XSOCKET_TYPE_DATAGRAM),
                        test_get_available);
  g_test_add_data_func ("/socket/get_available/stream", GUINT_TO_POINTER (XSOCKET_TYPE_STREAM),
                        test_get_available);
  g_test_add_data_func ("/socket/read_write", GUINT_TO_POINTER (FALSE),
                        test_read_write);
  g_test_add_data_func ("/socket/read_writev", GUINT_TO_POINTER (TRUE),
                        test_read_write);
#ifdef SO_NOSIGPIPE
  g_test_add_func ("/socket/nosigpipe", test_nosigpipe);
#endif
#if G_CREDENTIALS_SUPPORTED
  g_test_add_func ("/socket/credentials/tcp_client", test_credentials_tcp_client);
  g_test_add_func ("/socket/credentials/tcp_server", test_credentials_tcp_server);
  g_test_add_func ("/socket/credentials/unix_socketpair", test_credentials_unix_socketpair);
#endif

  return g_test_run();
}
