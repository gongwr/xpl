/* GLib testing framework examples and tests
 *
 * Copyright 2012 Red Hat, Inc.
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

#include <string.h>

#define XPL_VERSION_MIN_REQUIRED XPL_VERSION_2_34
#include <gio/gio.h>

/* Overview:
 *
 * We have an echo server, two proxy servers, two GProxy
 * implementations, and two GProxyResolver implementations.
 *
 * The echo server runs at @server.server_addr (on
 * @server.server_port).
 *
 * The two proxy servers, A and B, run on @proxy_a.port and
 * @proxy_b.port, with @proxy_a.uri and @proxy_b.uri pointing to them.
 * The "negotiation" with the two proxies is just sending the single
 * letter "a" or "b" and receiving it back in uppercase; the proxy
 * then connects to @server_addr.
 *
 * Proxy A supports "alpha://" URIs, and does not support hostname
 * resolution, and Proxy B supports "beta://" URIs, and does support
 * hostname resolution (but it just ignores the hostname and always
 * connects to @server_addr anyway).
 *
 * The default GProxyResolver (GTestProxyResolver) looks at its URI
 * and returns [ "direct://" ] for "simple://" URIs, and [
 * proxy_a.uri, proxy_b.uri ] for other URIs. The other GProxyResolver
 * (GTestAltProxyResolver) always returns [ proxy_a.uri ].
 */

typedef struct {
  xchar_t *proxy_command;
  xchar_t *supported_protocol;

  xsocket_t *server;
  GThread *thread;
  xcancellable_t *cancellable;
  xchar_t *uri;
  gushort port;

  xsocket_t *client_sock, *server_sock;
  GMainLoop *loop;

  xerror_t *last_error;
} ProxyData;

static ProxyData proxy_a, proxy_b;

typedef struct {
  xsocket_t *server;
  GThread *server_thread;
  xcancellable_t *cancellable;
  xsocket_address_t *server_addr;
  gushort server_port;
} ServerData;

static ServerData server;

static xchar_t **last_proxies;

static GSocketClient *client;


/**************************************/
/* Test GProxyResolver implementation */
/**************************************/

typedef struct {
  xobject_t parent_instance;
} GTestProxyResolver;

typedef struct {
  xobject_class_t parent_class;
} GTestProxyResolverClass;

static void g_test_proxy_resolver_iface_init (GProxyResolverInterface *iface);

static xtype_t _g_test_proxy_resolver_get_type (void);
#define g_test_proxy_resolver_get_type _g_test_proxy_resolver_get_type
G_DEFINE_TYPE_WITH_CODE (GTestProxyResolver, g_test_proxy_resolver, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY_RESOLVER,
						g_test_proxy_resolver_iface_init)
			 g_io_extension_point_implement (G_PROXY_RESOLVER_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "test",
							 0))

static void
g_test_proxy_resolver_init (GTestProxyResolver *resolver)
{
}

static xboolean_t
g_test_proxy_resolver_is_supported (GProxyResolver *resolver)
{
  return TRUE;
}

static xchar_t **
g_test_proxy_resolver_lookup (GProxyResolver  *resolver,
			      const xchar_t     *uri,
			      xcancellable_t    *cancellable,
			      xerror_t         **error)
{
  xchar_t **proxies;

  g_assert (last_proxies == NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  proxies = g_new (xchar_t *, 3);

  if (!strncmp (uri, "simple://", 4))
    {
      proxies[0] = g_strdup ("direct://");
      proxies[1] = NULL;
    }
  else
    {
      /* Proxy A can only deal with "alpha://" URIs, not
       * "beta://", but we always return both URIs
       * anyway so we can test error handling when the first
       * fails.
       */
      proxies[0] = g_strdup (proxy_a.uri);
      proxies[1] = g_strdup (proxy_b.uri);
      proxies[2] = NULL;
    }

  last_proxies = g_strdupv (proxies);

  return proxies;
}

static void
g_test_proxy_resolver_lookup_async (GProxyResolver      *resolver,
				    const xchar_t         *uri,
				    xcancellable_t        *cancellable,
				    xasync_ready_callback_t  callback,
				    xpointer_t             user_data)
{
  xerror_t *error = NULL;
  GTask *task;
  xchar_t **proxies;

  proxies = g_proxy_resolver_lookup (resolver, uri, cancellable, &error);

  task = g_task_new (resolver, NULL, callback, user_data);
  if (proxies == NULL)
    g_task_return_error (task, error);
  else
    g_task_return_pointer (task, proxies, (GDestroyNotify) g_strfreev);

  g_object_unref (task);
}

static xchar_t **
g_test_proxy_resolver_lookup_finish (GProxyResolver     *resolver,
				     xasync_result_t       *result,
				     xerror_t            **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
g_test_proxy_resolver_class_init (GTestProxyResolverClass *resolver_class)
{
}

static void
g_test_proxy_resolver_iface_init (GProxyResolverInterface *iface)
{
  iface->is_supported = g_test_proxy_resolver_is_supported;
  iface->lookup = g_test_proxy_resolver_lookup;
  iface->lookup_async = g_test_proxy_resolver_lookup_async;
  iface->lookup_finish = g_test_proxy_resolver_lookup_finish;
}

/****************************/
/* Alternate GProxyResolver */
/****************************/

typedef GTestProxyResolver GTestAltProxyResolver;
typedef GTestProxyResolverClass GTestAltProxyResolverClass;

static void g_test_alt_proxy_resolver_iface_init (GProxyResolverInterface *iface);

static xtype_t _g_test_alt_proxy_resolver_get_type (void);
#define g_test_alt_proxy_resolver_get_type _g_test_alt_proxy_resolver_get_type
G_DEFINE_TYPE_WITH_CODE (GTestAltProxyResolver, g_test_alt_proxy_resolver, g_test_proxy_resolver_get_type (),
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY_RESOLVER,
						g_test_alt_proxy_resolver_iface_init);
                         )

static void
g_test_alt_proxy_resolver_init (GTestProxyResolver *resolver)
{
}

static xchar_t **
g_test_alt_proxy_resolver_lookup (GProxyResolver  *resolver,
                                  const xchar_t     *uri,
                                  xcancellable_t    *cancellable,
                                  xerror_t         **error)
{
  xchar_t **proxies;

  proxies = g_new (xchar_t *, 2);

  proxies[0] = g_strdup (proxy_a.uri);
  proxies[1] = NULL;

  last_proxies = g_strdupv (proxies);

  return proxies;
}

static void
g_test_alt_proxy_resolver_class_init (GTestProxyResolverClass *resolver_class)
{
}

static void
g_test_alt_proxy_resolver_iface_init (GProxyResolverInterface *iface)
{
  iface->lookup = g_test_alt_proxy_resolver_lookup;
}


/****************************************/
/* Test proxy implementation base class */
/****************************************/

typedef struct {
  xobject_t parent;

  ProxyData *proxy_data;
} GProxyBase;

typedef struct {
  xobject_class_t parent_class;
} GProxyBaseClass;

static xtype_t _g_proxy_base_get_type (void);
#define g_proxy_base_get_type _g_proxy_base_get_type
G_DEFINE_ABSTRACT_TYPE (GProxyBase, g_proxy_base, XTYPE_OBJECT)

static void
g_proxy_base_init (GProxyBase *proxy)
{
}

static xio_stream_t *
g_proxy_base_connect (GProxy            *proxy,
		      xio_stream_t         *io_stream,
		      GProxyAddress     *proxy_address,
		      xcancellable_t      *cancellable,
		      xerror_t           **error)
{
  ProxyData *data = ((GProxyBase *) proxy)->proxy_data;
  const xchar_t *protocol;
  xoutput_stream_t *ostream;
  xinput_stream_t *istream;
  xchar_t response;

  g_assert_no_error (data->last_error);

  protocol = g_proxy_address_get_destination_protocol (proxy_address);
  if (strcmp (protocol, data->supported_protocol) != 0)
    {
      g_set_error_literal (&data->last_error,
			   G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			   "Unsupported protocol");
      goto fail;
    }

  ostream = g_io_stream_get_output_stream (io_stream);
  if (g_output_stream_write (ostream, data->proxy_command, 1, cancellable,
			     &data->last_error) != 1)
    goto fail;

  istream = g_io_stream_get_input_stream (io_stream);
  if (g_input_stream_read (istream, &response, 1, cancellable,
			   &data->last_error) != 1)
    goto fail;

  if (response != g_ascii_toupper (*data->proxy_command))
    {
      g_set_error_literal (&data->last_error,
			   G_IO_ERROR, G_IO_ERROR_FAILED,
			   "Failed");
      goto fail;
    }

  return g_object_ref (io_stream);

 fail:
  g_propagate_error (error, g_error_copy (data->last_error));
  return NULL;
}

static void
g_proxy_base_connect_async (GProxy               *proxy,
			    xio_stream_t            *io_stream,
			    GProxyAddress        *proxy_address,
			    xcancellable_t         *cancellable,
			    xasync_ready_callback_t   callback,
			    xpointer_t              user_data)
{
  xerror_t *error = NULL;
  GTask *task;
  xio_stream_t *proxy_io_stream;

  task = g_task_new (proxy, NULL, callback, user_data);

  proxy_io_stream = g_proxy_connect (proxy, io_stream, proxy_address,
				     cancellable, &error);
  if (proxy_io_stream)
    g_task_return_pointer (task, proxy_io_stream, g_object_unref);
  else
    g_task_return_error (task, error);
  g_object_unref (task);
}

static xio_stream_t *
g_proxy_base_connect_finish (GProxy        *proxy,
			     xasync_result_t  *result,
			     xerror_t       **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
g_proxy_base_class_init (GProxyBaseClass *class)
{
}


/********************************************/
/* Test proxy implementation #1 ("Proxy A") */
/********************************************/

typedef GProxyBase GProxyA;
typedef GProxyBaseClass GProxyAClass;

static void g_proxy_a_iface_init (GProxyInterface *proxy_iface);

static xtype_t _g_proxy_a_get_type (void);
#define g_proxy_a_get_type _g_proxy_a_get_type
G_DEFINE_TYPE_WITH_CODE (GProxyA, g_proxy_a, g_proxy_base_get_type (),
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY,
						g_proxy_a_iface_init)
			 g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "proxy-a",
							 0))

static void
g_proxy_a_init (GProxyA *proxy)
{
  ((GProxyBase *) proxy)->proxy_data = &proxy_a;
}

static xboolean_t
g_proxy_a_supports_hostname (GProxy *proxy)
{
  return FALSE;
}

static void
g_proxy_a_class_init (GProxyAClass *class)
{
}

static void
g_proxy_a_iface_init (GProxyInterface *proxy_iface)
{
  proxy_iface->connect = g_proxy_base_connect;
  proxy_iface->connect_async = g_proxy_base_connect_async;
  proxy_iface->connect_finish = g_proxy_base_connect_finish;
  proxy_iface->supports_hostname = g_proxy_a_supports_hostname;
}

/********************************************/
/* Test proxy implementation #2 ("Proxy B") */
/********************************************/

typedef GProxyBase GProxyB;
typedef GProxyBaseClass GProxyBClass;

static void g_proxy_b_iface_init (GProxyInterface *proxy_iface);

static xtype_t _g_proxy_b_get_type (void);
#define g_proxy_b_get_type _g_proxy_b_get_type
G_DEFINE_TYPE_WITH_CODE (GProxyB, g_proxy_b, g_proxy_base_get_type (),
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY,
						g_proxy_b_iface_init)
			 g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "proxy-b",
							 0))

static void
g_proxy_b_init (GProxyB *proxy)
{
  ((GProxyBase *) proxy)->proxy_data = &proxy_b;
}

static xboolean_t
g_proxy_b_supports_hostname (GProxy *proxy)
{
  return TRUE;
}

static void
g_proxy_b_class_init (GProxyBClass *class)
{
}

static void
g_proxy_b_iface_init (GProxyInterface *proxy_iface)
{
  proxy_iface->connect = g_proxy_base_connect;
  proxy_iface->connect_async = g_proxy_base_connect_async;
  proxy_iface->connect_finish = g_proxy_base_connect_finish;
  proxy_iface->supports_hostname = g_proxy_b_supports_hostname;
}


/***********************************/
/* The proxy server implementation */
/***********************************/

static xboolean_t
proxy_bytes (xsocket_t      *socket,
	     GIOCondition  condition,
	     xpointer_t      user_data)
{
  ProxyData *proxy = user_data;
  gssize nread, nwrote, total;
  xchar_t buffer[8];
  xsocket_t *out_socket;
  xerror_t *error = NULL;

  nread = xsocket_receive_with_blocking (socket, buffer, sizeof (buffer),
					  TRUE, NULL, &error);
  if (nread == -1)
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
      return FALSE;
    }
  else
    g_assert_no_error (error);

  if (nread == 0)
    {
      g_main_loop_quit (proxy->loop);
      return FALSE;
    }

  if (socket == proxy->client_sock)
    out_socket = proxy->server_sock;
  else
    out_socket = proxy->client_sock;

  for (total = 0; total < nread; total += nwrote)
    {
      nwrote = xsocket_send_with_blocking (out_socket,
					    buffer + total, nread - total,
					    TRUE, NULL, &error);
      g_assert_no_error (error);
    }

  return TRUE;
}

static xpointer_t
proxy_thread (xpointer_t user_data)
{
  ProxyData *proxy = user_data;
  xerror_t *error = NULL;
  gssize nread, nwrote;
  xchar_t command[2] = { 0, 0 };
  GMainContext *context;
  GSource *read_source, *write_source;

  context = g_main_context_new ();
  proxy->loop = g_main_loop_new (context, FALSE);

  while (TRUE)
    {
      proxy->client_sock = xsocket_accept (proxy->server, proxy->cancellable, &error);
      if (!proxy->client_sock)
	{
	  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
          g_error_free (error);
	  break;
	}
      else
	g_assert_no_error (error);

      nread = xsocket_receive (proxy->client_sock, command, 1, NULL, &error);
      g_assert_no_error (error);

      if (nread == 0)
	{
	  g_clear_object (&proxy->client_sock);
	  continue;
	}

      g_assert_cmpint (nread, ==, 1);
      g_assert_cmpstr (command, ==, proxy->proxy_command);

      *command = g_ascii_toupper (*command);
      nwrote = xsocket_send (proxy->client_sock, command, 1, NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (nwrote, ==, 1);

      proxy->server_sock = xsocket_new (XSOCKET_FAMILY_IPV4,
					 XSOCKET_TYPE_STREAM,
					 XSOCKET_PROTOCOL_DEFAULT,
					 &error);
      g_assert_no_error (error);
      xsocket_connect (proxy->server_sock, server.server_addr, NULL, &error);
      g_assert_no_error (error);

      read_source = xsocket_create_source (proxy->client_sock, G_IO_IN, NULL);
      g_source_set_callback (read_source, (GSourceFunc)proxy_bytes, proxy, NULL);
      g_source_attach (read_source, context);

      write_source = xsocket_create_source (proxy->server_sock, G_IO_IN, NULL);
      g_source_set_callback (write_source, (GSourceFunc)proxy_bytes, proxy, NULL);
      g_source_attach (write_source, context);

      g_main_loop_run (proxy->loop);

      xsocket_close (proxy->client_sock, &error);
      g_assert_no_error (error);
      g_clear_object (&proxy->client_sock);

      xsocket_close (proxy->server_sock, &error);
      g_assert_no_error (error);
      g_clear_object (&proxy->server_sock);

      g_source_destroy (read_source);
      g_source_unref (read_source);
      g_source_destroy (write_source);
      g_source_unref (write_source);
    }

  g_main_loop_unref (proxy->loop);
  g_main_context_unref (context);

  g_object_unref (proxy->server);
  g_object_unref (proxy->cancellable);

  g_free (proxy->proxy_command);
  g_free (proxy->supported_protocol);
  g_free (proxy->uri);

  return NULL;
}

static void
create_proxy (ProxyData    *proxy,
	      xchar_t         proxy_protocol,
	      const xchar_t  *destination_protocol,
	      xcancellable_t *cancellable)
{
  xerror_t *error = NULL;
  xsocket_address_t *addr;
  xinet_address_t *iaddr;

  proxy->proxy_command = g_strdup_printf ("%c", proxy_protocol);
  proxy->supported_protocol = g_strdup (destination_protocol);
  proxy->cancellable = g_object_ref (cancellable);

  proxy->server = xsocket_new (XSOCKET_FAMILY_IPV4,
				XSOCKET_TYPE_STREAM,
				XSOCKET_PROTOCOL_DEFAULT,
				&error);
  g_assert_no_error (error);

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);

  xsocket_bind (proxy->server, addr, TRUE, &error);
  g_assert_no_error (error);
  g_object_unref (addr);

  addr = xsocket_get_local_address (proxy->server, &error);
  proxy->port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr));
  proxy->uri = g_strdup_printf ("proxy-%c://127.0.0.1:%u",
				g_ascii_tolower (proxy_protocol),
				proxy->port);
  g_object_unref (addr);

  xsocket_listen (proxy->server, &error);
  g_assert_no_error (error);

  proxy->thread = g_thread_new ("proxy", proxy_thread, proxy);
}



/**************************/
/* The actual echo server */
/**************************/

static xpointer_t
echo_server_thread (xpointer_t user_data)
{
  ServerData *data = user_data;
  xsocket_t *sock;
  xerror_t *error = NULL;
  gssize nread, nwrote;
  xchar_t buf[128];

  while (TRUE)
    {
      sock = xsocket_accept (data->server, data->cancellable, &error);
      if (!sock)
	{
	  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
          g_error_free (error);
	  break;
	}
      else
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
      g_object_unref (sock);
    }

  g_object_unref (data->server);
  g_object_unref (data->server_addr);
  g_object_unref (data->cancellable);

  return NULL;
}

static void
create_server (ServerData *data, xcancellable_t *cancellable)
{
  xerror_t *error = NULL;
  xsocket_address_t *addr;
  xinet_address_t *iaddr;

  data->cancellable = g_object_ref (cancellable);

  data->server = xsocket_new (XSOCKET_FAMILY_IPV4,
			       XSOCKET_TYPE_STREAM,
			       XSOCKET_PROTOCOL_DEFAULT,
			       &error);
  g_assert_no_error (error);

  xsocket_set_blocking (data->server, TRUE);
  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);

  xsocket_bind (data->server, addr, TRUE, &error);
  g_assert_no_error (error);
  g_object_unref (addr);

  data->server_addr = xsocket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  data->server_port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (data->server_addr));

  xsocket_listen (data->server, &error);
  g_assert_no_error (error);

  data->server_thread = g_thread_new ("server", echo_server_thread, data);
}


/******************************************************************/
/* Now a GResolver implementation, so the can't-resolve test will */
/* pass even if you have an evil DNS-faking ISP.                  */
/******************************************************************/

typedef GResolver GFakeResolver;
typedef GResolverClass GFakeResolverClass;

static xtype_t g_fake_resolver_get_type (void);
G_DEFINE_TYPE (GFakeResolver, g_fake_resolver, XTYPE_RESOLVER)

static void
g_fake_resolver_init (GFakeResolver *gtr)
{
}

static xlist_t *
g_fake_resolver_lookup_by_name (GResolver     *resolver,
				const xchar_t   *hostname,
				xcancellable_t  *cancellable,
				xerror_t       **error)
{
  if (!strcmp (hostname, "example.com"))
    return g_list_prepend (NULL, xinet_address_new_from_string ("127.0.0.1"));
  else
    {
      /* Anything else is expected to fail. */
      g_set_error (error,
                   G_RESOLVER_ERROR,
                   G_RESOLVER_ERROR_NOT_FOUND,
                   "Not found");
      return NULL;
    }
}

static void
g_fake_resolver_lookup_by_name_async (GResolver           *resolver,
				      const xchar_t         *hostname,
				      xcancellable_t        *cancellable,
				      xasync_ready_callback_t  callback,
				      xpointer_t             user_data)
{
  GTask *task;

  task = g_task_new (resolver, cancellable, callback, user_data);

  if (!strcmp (hostname, "example.com"))
    {
      xlist_t *result;

      result = g_list_prepend (NULL, xinet_address_new_from_string ("127.0.0.1"));
      g_task_return_pointer (task, result, (GDestroyNotify) g_resolver_free_addresses);
    }
  else
    {
      g_task_return_new_error (task,
                               G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                               "Not found");
    }
  g_object_unref (task);
}

static void
g_fake_resolver_lookup_by_name_with_flags_async (GResolver               *resolver,
                                                 const xchar_t             *hostname,
                                                 GResolverNameLookupFlags flags,
                                                 xcancellable_t            *cancellable,
                                                 xasync_ready_callback_t      callback,
                                                 xpointer_t                 user_data)
{
  /* Note this isn't a real implementation as it ignores the flags */
  g_fake_resolver_lookup_by_name_async (resolver,
                                        hostname,
                                        cancellable,
                                        callback,
                                        user_data);
}

static xlist_t *
g_fake_resolver_lookup_by_name_finish (GResolver            *resolver,
				       xasync_result_t         *result,
				       xerror_t              **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
g_fake_resolver_class_init (GFakeResolverClass *fake_class)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (fake_class);

  resolver_class->lookup_by_name                   = g_fake_resolver_lookup_by_name;
  resolver_class->lookup_by_name_async             = g_fake_resolver_lookup_by_name_async;
  resolver_class->lookup_by_name_finish            = g_fake_resolver_lookup_by_name_finish;
  resolver_class->lookup_by_name_with_flags_async  = g_fake_resolver_lookup_by_name_with_flags_async;
  resolver_class->lookup_by_name_with_flags_finish = g_fake_resolver_lookup_by_name_finish;
}



/****************************************/
/* We made it! Now for the actual test! */
/****************************************/

static void
setup_test (xpointer_t fixture,
	    gconstpointer user_data)
{
}

static void
teardown_test (xpointer_t fixture,
	       gconstpointer user_data)
{
  if (last_proxies)
    {
      g_strfreev (last_proxies);
      last_proxies = NULL;
    }
  g_clear_error (&proxy_a.last_error);
  g_clear_error (&proxy_b.last_error);
}


static const xchar_t *testbuf = "0123456789abcdef";

static void
do_echo_test (xsocket_connection_t *conn)
{
  xio_stream_t *iostream = XIO_STREAM (conn);
  xinput_stream_t *istream = g_io_stream_get_input_stream (iostream);
  xoutput_stream_t *ostream = g_io_stream_get_output_stream (iostream);
  gssize nread;
  xsize_t nwrote, total;
  xchar_t buf[128];
  xerror_t *error = NULL;

  g_output_stream_write_all (ostream, testbuf, strlen (testbuf),
			     &nwrote, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nwrote, ==, strlen (testbuf));

  for (total = 0; total < nwrote; total += nread)
    {
      nread = g_input_stream_read (istream,
				   buf + total, sizeof (buf) - total,
				   NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (nread, >, 0);
    }

  buf[total] = '\0';
  g_assert_cmpstr (buf, ==, testbuf);
}

static void
async_got_conn (xobject_t      *source,
		xasync_result_t *result,
		xpointer_t      user_data)
{
  xsocket_connection_t **conn = user_data;
  xerror_t *error = NULL;

  *conn = xsocket_client_connect_finish (XSOCKET_CLIENT (source),
					  result, &error);
  g_assert_no_error (error);
}

static void
async_got_error (xobject_t      *source,
		 xasync_result_t *result,
		 xpointer_t      user_data)
{
  xerror_t **error = user_data;

  g_assert (error != NULL && *error == NULL);
  xsocket_client_connect_finish (XSOCKET_CLIENT (source),
				  result, error);
  g_assert (*error != NULL);
}


static void
assert_direct (xsocket_connection_t *conn)
{
  xsocket_address_t *addr;
  xerror_t *error = NULL;

  g_assert_cmpint (g_strv_length (last_proxies), ==, 1);
  g_assert_cmpstr (last_proxies[0], ==, "direct://");
  g_assert_no_error (proxy_a.last_error);
  g_assert_no_error (proxy_b.last_error);

  addr = xsocket_connection_get_remote_address (conn, &error);
  g_assert_no_error (error);
  g_assert (addr != NULL && !X_IS_PROXY_ADDRESS (addr));
  g_object_unref (addr);

  addr = xsocket_connection_get_local_address (conn, &error);
  g_assert_no_error (error);
  g_object_unref (addr);

  g_assert (xsocket_connection_is_connected (conn));
}

static void
test_direct_sync (xpointer_t fixture,
		  gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xchar_t *uri;
  xerror_t *error = NULL;

  /* The simple:// URI should not require any proxy. */

  uri = g_strdup_printf ("simple://127.0.0.1:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_free (uri);
  g_assert_no_error (error);

  assert_direct (conn);
  do_echo_test (conn);
  g_object_unref (conn);
}

static void
test_direct_async (xpointer_t fixture,
		   gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xchar_t *uri;

  /* The simple:// URI should not require any proxy. */
  uri = g_strdup_printf ("simple://127.0.0.1:%u", server.server_port);
  conn = NULL;
  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_conn, &conn);
  g_free (uri);
  while (conn == NULL)
    g_main_context_iteration (NULL, TRUE);

  assert_direct (conn);
  do_echo_test (conn);
  g_object_unref (conn);
}

static void
assert_single (xsocket_connection_t *conn)
{
  xsocket_address_t *addr;
  const xchar_t *proxy_uri;
  gushort proxy_port;
  xerror_t *error = NULL;

  g_assert_cmpint (g_strv_length (last_proxies), ==, 2);
  g_assert_cmpstr (last_proxies[0], ==, proxy_a.uri);
  g_assert_cmpstr (last_proxies[1], ==, proxy_b.uri);
  g_assert_no_error (proxy_a.last_error);
  g_assert_no_error (proxy_b.last_error);

  addr = xsocket_connection_get_remote_address (conn, &error);
  g_assert_no_error (error);
  g_assert (X_IS_PROXY_ADDRESS (addr));
  proxy_uri = g_proxy_address_get_uri (G_PROXY_ADDRESS (addr));
  g_assert_cmpstr (proxy_uri, ==, proxy_a.uri);
  proxy_port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr));
  g_assert_cmpint (proxy_port, ==, proxy_a.port);

  g_object_unref (addr);
}

static void
test_single_sync (xpointer_t fixture,
		  gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xerror_t *error = NULL;
  xchar_t *uri;

  /* The alpha:// URI should be proxied via Proxy A */
  uri = g_strdup_printf ("alpha://127.0.0.1:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_free (uri);
  g_assert_no_error (error);

  assert_single (conn);

  do_echo_test (conn);
  g_object_unref (conn);
}

static void
test_single_async (xpointer_t fixture,
		   gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xchar_t *uri;

  /* The alpha:// URI should be proxied via Proxy A */
  uri = g_strdup_printf ("alpha://127.0.0.1:%u", server.server_port);
  conn = NULL;
  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_conn, &conn);
  g_free (uri);
  while (conn == NULL)
    g_main_context_iteration (NULL, TRUE);

  assert_single (conn);
  do_echo_test (conn);
  g_object_unref (conn);
}

static void
assert_multiple (xsocket_connection_t *conn)
{
  xsocket_address_t *addr;
  const xchar_t *proxy_uri;
  gushort proxy_port;
  xerror_t *error = NULL;

  g_assert_cmpint (g_strv_length (last_proxies), ==, 2);
  g_assert_cmpstr (last_proxies[0], ==, proxy_a.uri);
  g_assert_cmpstr (last_proxies[1], ==, proxy_b.uri);
  g_assert_error (proxy_a.last_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_assert_no_error (proxy_b.last_error);

  addr = xsocket_connection_get_remote_address (conn, &error);
  g_assert_no_error (error);
  g_assert (X_IS_PROXY_ADDRESS (addr));
  proxy_uri = g_proxy_address_get_uri (G_PROXY_ADDRESS (addr));
  g_assert_cmpstr (proxy_uri, ==, proxy_b.uri);
  proxy_port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr));
  g_assert_cmpint (proxy_port, ==, proxy_b.port);

  g_object_unref (addr);
}

static void
test_multiple_sync (xpointer_t fixture,
		    gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xerror_t *error = NULL;
  xchar_t *uri;

  /* The beta:// URI should be proxied via Proxy B, after failing
   * via Proxy A.
   */
  uri = g_strdup_printf ("beta://127.0.0.1:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_free (uri);
  g_assert_no_error (error);

  assert_multiple (conn);
  do_echo_test (conn);
  g_object_unref (conn);
}

static void
test_multiple_async (xpointer_t fixture,
		     gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xchar_t *uri;

  /* The beta:// URI should be proxied via Proxy B, after failing
   * via Proxy A.
   */
  uri = g_strdup_printf ("beta://127.0.0.1:%u", server.server_port);
  conn = NULL;
  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_conn, &conn);
  g_free (uri);
  while (conn == NULL)
    g_main_context_iteration (NULL, TRUE);

  assert_multiple (conn);
  do_echo_test (conn);
  g_object_unref (conn);
}

static void
test_dns (xpointer_t fixture,
	  gconstpointer user_data)
{
  xsocket_connection_t *conn;
  xerror_t *error = NULL;
  xchar_t *uri;

  /* The simple:// and alpha:// URIs should fail with a DNS error,
   * but the beta:// URI should succeed, because we pass it to
   * Proxy B without trying to resolve it first
   */

  /* simple */
  uri = g_strdup_printf ("simple://no-such-host.xx:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_assert_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND);
  g_clear_error (&error);

  g_assert_no_error (proxy_a.last_error);
  g_assert_no_error (proxy_b.last_error);
  teardown_test (NULL, NULL);

  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_error, &error);
  while (error == NULL)
    g_main_context_iteration (NULL, TRUE);
  g_assert_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND);
  g_clear_error (&error);
  g_free (uri);

  g_assert_no_error (proxy_a.last_error);
  g_assert_no_error (proxy_b.last_error);
  teardown_test (NULL, NULL);

  /* alpha */
  uri = g_strdup_printf ("alpha://no-such-host.xx:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  /* Since Proxy A fails, @client will try Proxy B too, which won't
   * load an alpha:// URI.
   */
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  g_assert_no_error (proxy_a.last_error);
  g_assert_error (proxy_b.last_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  teardown_test (NULL, NULL);

  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_error, &error);
  while (error == NULL)
    g_main_context_iteration (NULL, TRUE);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
  g_free (uri);

  g_assert_no_error (proxy_a.last_error);
  g_assert_error (proxy_b.last_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  teardown_test (NULL, NULL);

  /* beta */
  uri = g_strdup_printf ("beta://no-such-host.xx:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_assert_no_error (error);

  g_assert_no_error (proxy_a.last_error);
  g_assert_no_error (proxy_b.last_error);

  do_echo_test (conn);
  g_clear_object (&conn);
  teardown_test (NULL, NULL);

  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_conn, &conn);
  while (conn == NULL)
    g_main_context_iteration (NULL, TRUE);
  g_free (uri);

  g_assert_no_error (proxy_a.last_error);
  g_assert_no_error (proxy_b.last_error);

  do_echo_test (conn);
  g_clear_object (&conn);
  teardown_test (NULL, NULL);
}

static void
assert_override (xsocket_connection_t *conn)
{
  g_assert_cmpint (g_strv_length (last_proxies), ==, 1);
  g_assert_cmpstr (last_proxies[0], ==, proxy_a.uri);

  if (conn)
    g_assert_no_error (proxy_a.last_error);
  else
    g_assert_error (proxy_a.last_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
}

static void
test_override (xpointer_t fixture,
               gconstpointer user_data)
{
  GProxyResolver *alt_resolver;
  xsocket_connection_t *conn;
  xerror_t *error = NULL;
  xchar_t *uri;

  g_assert (xsocket_client_get_proxy_resolver (client) == g_proxy_resolver_get_default ());
  alt_resolver = g_object_new (g_test_alt_proxy_resolver_get_type (), NULL);
  xsocket_client_set_proxy_resolver (client, alt_resolver);
  g_assert (xsocket_client_get_proxy_resolver (client) == alt_resolver);

  /* Alt proxy resolver always returns Proxy A, so alpha:// should
   * succeed, and simple:// and beta:// should fail.
   */

  /* simple */
  uri = g_strdup_printf ("simple://127.0.0.1:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
  assert_override (conn);
  teardown_test (NULL, NULL);

  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_error, &error);
  while (error == NULL)
    g_main_context_iteration (NULL, TRUE);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
  assert_override (conn);
  g_free (uri);
  teardown_test (NULL, NULL);

  /* alpha */
  uri = g_strdup_printf ("alpha://127.0.0.1:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_assert_no_error (error);
  assert_override (conn);
  do_echo_test (conn);
  g_clear_object (&conn);
  teardown_test (NULL, NULL);

  conn = NULL;
  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_conn, &conn);
  while (conn == NULL)
    g_main_context_iteration (NULL, TRUE);
  assert_override (conn);
  do_echo_test (conn);
  g_clear_object (&conn);
  g_free (uri);
  teardown_test (NULL, NULL);

  /* beta */
  uri = g_strdup_printf ("beta://127.0.0.1:%u", server.server_port);
  conn = xsocket_client_connect_to_uri (client, uri, 0, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
  assert_override (conn);
  teardown_test (NULL, NULL);

  xsocket_client_connect_to_uri_async (client, uri, 0, NULL,
					async_got_error, &error);
  while (error == NULL)
    g_main_context_iteration (NULL, TRUE);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);
  assert_override (conn);
  g_free (uri);
  teardown_test (NULL, NULL);

  g_assert (xsocket_client_get_proxy_resolver (client) == alt_resolver);
  xsocket_client_set_proxy_resolver (client, NULL);
  g_assert (xsocket_client_get_proxy_resolver (client) == g_proxy_resolver_get_default ());
  g_object_unref (alt_resolver);
}

static void
assert_destination_port (GSocketAddressEnumerator *etor,
                         guint16                   port)
{
  xsocket_address_t *addr;
  GProxyAddress *paddr;
  xerror_t *error = NULL;

  while ((addr = xsocket_address_enumerator_next (etor, NULL, &error)))
    {
      g_assert_no_error (error);

      g_assert (X_IS_PROXY_ADDRESS (addr));
      paddr = G_PROXY_ADDRESS (addr);
      g_assert_cmpint (g_proxy_address_get_destination_port (paddr), ==, port);
      g_object_unref (addr);
    }
  g_assert_no_error (error);
}

static void
test_proxy_enumerator_ports (void)
{
  GSocketAddressEnumerator *etor;

  etor = g_object_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
                       "uri", "http://example.com/",
                       NULL);
  assert_destination_port (etor, 0);
  g_object_unref (etor);

  /* Have to call this to clear last_proxies so the next call to
   * g_test_proxy_resolver_lookup() won't assert.
   */
  teardown_test (NULL, NULL);

  etor = g_object_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
                       "uri", "http://example.com:8080/",
                       NULL);
  assert_destination_port (etor, 8080);
  g_object_unref (etor);

  teardown_test (NULL, NULL);

  etor = g_object_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
                       "uri", "http://example.com/",
                       "default-port", 80,
                       NULL);
  assert_destination_port (etor, 80);
  g_object_unref (etor);

  teardown_test (NULL, NULL);

  etor = g_object_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
                       "uri", "http://example.com:8080/",
                       "default-port", 80,
                       NULL);
  assert_destination_port (etor, 8080);
  g_object_unref (etor);

  teardown_test (NULL, NULL);
}

int
main (int   argc,
      char *argv[])
{
  GResolver *fake_resolver;
  xcancellable_t *cancellable;
  xint_t result;

  g_test_init (&argc, &argv, NULL);

  /* Register stuff. The dummy g_proxy_get_default_for_protocol() call
   * is to force _xio_modules_ensure_extension_points_registered() to
   * get called, so we can then register a proxy resolver extension
   * point.
   */
  g_proxy_get_default_for_protocol ("foo");
  g_test_proxy_resolver_get_type ();
  g_proxy_a_get_type ();
  g_proxy_b_get_type ();
  g_setenv ("GIO_USE_PROXY_RESOLVER", "test", TRUE);

  fake_resolver = g_object_new (g_fake_resolver_get_type (), NULL);
  g_resolver_set_default (fake_resolver);

  cancellable = g_cancellable_new ();
  create_server (&server, cancellable);
  create_proxy (&proxy_a, 'a', "alpha", cancellable);
  create_proxy (&proxy_b, 'b', "beta", cancellable);

  client = xsocket_client_new ();
  g_assert_cmpint (xsocket_client_get_enable_proxy (client), ==, TRUE);

  g_test_add_vtable ("/proxy/direct_sync", 0, NULL, setup_test, test_direct_sync, teardown_test);
  g_test_add_vtable ("/proxy/direct_async", 0, NULL, setup_test, test_direct_async, teardown_test);
  g_test_add_vtable ("/proxy/single_sync", 0, NULL, setup_test, test_single_sync, teardown_test);
  g_test_add_vtable ("/proxy/single_async", 0, NULL, setup_test, test_single_async, teardown_test);
  g_test_add_vtable ("/proxy/multiple_sync", 0, NULL, setup_test, test_multiple_sync, teardown_test);
  g_test_add_vtable ("/proxy/multiple_async", 0, NULL, setup_test, test_multiple_async, teardown_test);
  g_test_add_vtable ("/proxy/dns", 0, NULL, setup_test, test_dns, teardown_test);
  g_test_add_vtable ("/proxy/override", 0, NULL, setup_test, test_override, teardown_test);
  g_test_add_func ("/proxy/enumerator-ports", test_proxy_enumerator_ports);

  result = g_test_run();

  g_object_unref (client);

  g_cancellable_cancel (cancellable);
  g_thread_join (proxy_a.thread);
  g_thread_join (proxy_b.thread);
  g_thread_join (server.server_thread);

  g_object_unref (cancellable);

  return result;
}
