/* GLib testing framework examples and tests
 *
 * Copyright (C) 2010 Collabora, Ltd.
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
 * Authors: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>
#include <glib.h>

#include "glibintl.h"

#ifdef G_OS_UNIX
#include "gio/gunixsocketaddress.h"
#endif

static const xchar_t *info = NULL;
static xcancellable_t *cancellable = NULL;
static xint_t return_value = 0;

static G_NORETURN void
usage (void)
{
  fprintf (stderr, "Usage: proxy [-s] (uri|host:port|ip:port|path|srv/protocol/domain)\n");
  fprintf (stderr, "       Use -t to enable threading.\n");
  fprintf (stderr, "       Use -s to do synchronous lookups.\n");
  fprintf (stderr, "       Use -c to cancel operation.\n");
  fprintf (stderr, "       Use -e to use enumerator.\n");
  fprintf (stderr, "       Use -inet to use xinet_socket_address_t enumerator (ip:port).\n");
#ifdef G_OS_UNIX
  fprintf (stderr, "       Use -unix to use GUnixSocketAddress enumerator (path).\n");
#endif
  fprintf (stderr, "       Use -proxyaddr tp use xproxy_address_t enumerator "
                   "(ip:port:protocol:dest_host:dest_port[:username[:password]]).\n");
  fprintf (stderr, "       Use -netaddr to use xnetwork_address_t enumerator (host:port).\n");
  fprintf (stderr, "       Use -neturi to use xnetwork_address_t enumerator (uri).\n");
  fprintf (stderr, "       Use -netsrv to use xnetwork_service_t enumerator (srv/protocol/domain).\n");
  fprintf (stderr, "       Use -connect to create a connection using xsocket_client_t object (uri).\n");
  exit (1);
}

static void
print_and_free_error (xerror_t *error)
{
  fprintf (stderr, "Failed to obtain proxies: %s\n", error->message);
  xerror_free (error);
  return_value = 1;
}

static void
print_proxies (const xchar_t *info, xchar_t **proxies)
{
  printf ("Proxies for URI '%s' are:\n", info);

  if (proxies == NULL || proxies[0] == NULL)
    printf ("\tnone\n");
  else
    for (; proxies[0]; proxies++)
      printf ("\t%s\n", proxies[0]);
}

static void
_proxy_lookup_cb (xobject_t *source_object,
		  xasync_result_t *result,
		  xpointer_t user_data)
{
  xerror_t *error = NULL;
  xchar_t **proxies;
  xmain_loop_t *loop = user_data;

  proxies = xproxy_resolver_lookup_finish (G_PROXY_RESOLVER (source_object),
					    result,
					    &error);
  if (error)
    {
      print_and_free_error (error);
    }
  else
    {
      print_proxies (info, proxies);
      xstrfreev (proxies);
    }

  xmain_loop_quit (loop);
}

static void
use_resolver (xboolean_t synchronous)
{
  xproxy_resolver_t *resolver;

  resolver = xproxy_resolver_get_default ();

  if (synchronous)
    {
      xerror_t *error = NULL;
      xchar_t **proxies;

      proxies = xproxy_resolver_lookup (resolver, info, cancellable, &error);

      if (error)
	  print_and_free_error (error);
      else
	  print_proxies (info, proxies);

      xstrfreev (proxies);
    }
  else
    {
      xmain_loop_t *loop = xmain_loop_new (NULL, FALSE);

      xproxy_resolver_lookup_async (resolver,
				     info,
				     cancellable,
				     _proxy_lookup_cb,
				     loop);

      xmain_loop_run (loop);
      xmain_loop_unref (loop);
    }
}

static void
print_proxy_address (xsocket_address_t *sockaddr)
{
  xproxy_address_t *proxy = NULL;

  if (sockaddr == NULL)
    {
      printf ("\tdirect://\n");
      return;
    }

  if (X_IS_PROXY_ADDRESS (sockaddr))
    {
      proxy = G_PROXY_ADDRESS (sockaddr);
      printf ("\t%s://", xproxy_address_get_protocol(proxy));
    }
  else
    {
      printf ("\tdirect://");
    }

  if (X_IS_INET_SOCKET_ADDRESS (sockaddr))
    {
      xinet_address_t *inetaddr;
      xuint_t port;
      xchar_t *addr;

      xobject_get (sockaddr,
		    "address", &inetaddr,
		    "port", &port,
		    NULL);

      addr = xinet_address_to_string (inetaddr);

      printf ("%s:%u", addr, port);

      g_free (addr);
    }

  if (proxy)
    {
      if (xproxy_address_get_username(proxy))
        printf (" (Username: %s  Password: %s)",
                xproxy_address_get_username(proxy),
                xproxy_address_get_password(proxy));
      printf (" (Hostname: %s, Port: %i)",
              xproxy_address_get_destination_hostname (proxy),
              xproxy_address_get_destination_port (proxy));
    }

  printf ("\n");
}

static void
_proxy_enumerate_cb (xobject_t *object,
		     xasync_result_t *result,
		     xpointer_t user_data)
{
  xerror_t *error = NULL;
  xmain_loop_t *loop = user_data;
  xsocket_address_enumerator_t *enumerator = XSOCKET_ADDRESS_ENUMERATOR (object);
  xsocket_address_t *sockaddr;

  sockaddr = xsocket_address_enumerator_next_finish (enumerator,
						      result,
						      &error);
  if (sockaddr)
    {
      print_proxy_address (sockaddr);
      xsocket_address_enumerator_next_async (enumerator,
                                              cancellable,
					      _proxy_enumerate_cb,
					      loop);
      xobject_unref (sockaddr);
    }
  else
    {
      if (error)
	print_and_free_error (error);

      xmain_loop_quit (loop);
    }
}

static void
run_with_enumerator (xboolean_t synchronous, xsocket_address_enumerator_t *enumerator)
{
  xerror_t *error = NULL;

  if (synchronous)
    {
      xsocket_address_t *sockaddr;

      while ((sockaddr = xsocket_address_enumerator_next (enumerator,
							   cancellable,
							   &error)))
	{
	  print_proxy_address (sockaddr);
	  xobject_unref (sockaddr);
	}

      if (error)
	print_and_free_error (error);
    }
  else
    {
      xmain_loop_t *loop = xmain_loop_new (NULL, FALSE);

      xsocket_address_enumerator_next_async (enumerator,
                                              cancellable,
					      _proxy_enumerate_cb,
					      loop);
      xmain_loop_run (loop);
      xmain_loop_unref (loop);
    }
}

static void
use_enumerator (xboolean_t synchronous)
{
  xsocket_address_enumerator_t *enumerator;

  enumerator = xobject_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
			     "uri", info,
			     NULL);

  printf ("Proxies for URI '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}

static void
use_inet_address (xboolean_t synchronous)
{
  xsocket_address_enumerator_t *enumerator;
  xsocket_address_t *sockaddr;
  xinet_address_t *addr = NULL;
  xuint_t port = 0;
  xchar_t **ip_and_port;

  ip_and_port = xstrsplit (info, ":", 2);

  if (ip_and_port[0])
    {
      addr = xinet_address_new_from_string (ip_and_port[0]);
      if (ip_and_port [1])
	port = strtoul (ip_and_port [1], NULL, 10);
    }

  xstrfreev (ip_and_port);

  if (addr == NULL || port <= 0 || port >= 65535)
    {
      fprintf (stderr, "Bad 'ip:port' parameter '%s'\n", info);
      if (addr)
	xobject_unref (addr);
      return_value = 1;
      return;
    }

  sockaddr = g_inet_socket_address_new (addr, port);
  xobject_unref (addr);

  enumerator =
    xsocket_connectable_proxy_enumerate (XSOCKET_CONNECTABLE (sockaddr));
  xobject_unref (sockaddr);

  printf ("Proxies for ip and port '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}

#ifdef G_OS_UNIX
static void
use_unix_address (xboolean_t synchronous)
{
  xsocket_address_enumerator_t *enumerator;
  xsocket_address_t *sockaddr;

  sockaddr = g_unix_socket_address_new_with_type (info, -1, G_UNIX_SOCKET_ADDRESS_ABSTRACT);

  if (sockaddr == NULL)
    {
      fprintf (stderr, "Failed to create unix socket with name '%s'\n", info);
      return_value = 1;
      return;
    }

  enumerator =
    xsocket_connectable_proxy_enumerate (XSOCKET_CONNECTABLE (sockaddr));
  xobject_unref (sockaddr);

  printf ("Proxies for path '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}
#endif

static void
use_proxy_address (xboolean_t synchronous)
{
  xsocket_address_enumerator_t *enumerator;
  xsocket_address_t *sockaddr;
  xinet_address_t *addr;
  xuint_t port = 0;
  xchar_t *protocol;
  xchar_t *dest_host;
  xuint_t dest_port;
  xchar_t *username = NULL;
  xchar_t *password = NULL;
  xchar_t **split_info;

  split_info = xstrsplit (info, ":", 7);

  if (!split_info[0]
      || !split_info[1]
      || !split_info[2]
      || !split_info[3]
      || !split_info[4])
    {
      fprintf (stderr, "Bad 'ip:port:protocol:dest_host:dest_port' parameter '%s'\n", info);
      return_value = 1;
      return;
    }

  addr = xinet_address_new_from_string (split_info[0]);
  port = strtoul (split_info [1], NULL, 10);
  protocol = xstrdup (split_info[2]);
  dest_host = xstrdup (split_info[3]);
  dest_port = strtoul (split_info[4], NULL, 10);

  if (split_info[5])
    {
      username = xstrdup (split_info[5]);
      if (split_info[6])
        password = xstrdup (split_info[6]);
    }

  xstrfreev (split_info);

  sockaddr = xproxy_address_new (addr, port,
                                  protocol, dest_host, dest_port,
                                  username, password);

  xobject_unref (addr);
  g_free (protocol);
  g_free (dest_host);
  g_free (username);
  g_free (password);

  enumerator =
    xsocket_connectable_proxy_enumerate (XSOCKET_CONNECTABLE (sockaddr));
  xobject_unref (sockaddr);

  printf ("Proxies for ip and port '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}

static void
use_network_address (xboolean_t synchronous)
{
  xerror_t *error = NULL;
  xsocket_address_enumerator_t *enumerator;
  xsocket_connectable_t *connectable;

  connectable = g_network_address_parse (info, -1, &error);

  if (error)
    {
      print_and_free_error (error);
      return;
    }

  enumerator = xsocket_connectable_proxy_enumerate (connectable);
  xobject_unref (connectable);

  printf ("Proxies for hostname and port '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}

static void
use_network_uri (xboolean_t synchronous)
{
  xerror_t *error = NULL;
  xsocket_address_enumerator_t *enumerator;
  xsocket_connectable_t *connectable;

  connectable = g_network_address_parse_uri (info, 0, &error);

  if (error)
    {
      print_and_free_error (error);
      return;
    }

  enumerator = xsocket_connectable_proxy_enumerate (connectable);
  xobject_unref (connectable);

  printf ("Proxies for URI '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}

static void
use_network_service (xboolean_t synchronous)
{
  xsocket_address_enumerator_t *enumerator;
  xsocket_connectable_t *connectable = NULL;
  xchar_t **split;

  split = xstrsplit (info, "/", 3);

  if (split[0] && split[1] && split[2])
    connectable = g_network_service_new (split[0], split[1], split[2]);

  xstrfreev (split);

  if (connectable == NULL)
    {
       fprintf (stderr, "Bad 'srv/protocol/domain' parameter '%s'\n", info);
       return_value = 1;
       return;
    }

  enumerator = xsocket_connectable_proxy_enumerate (connectable);
  xobject_unref (connectable);

  printf ("Proxies for hostname and port '%s' are:\n", info);
  run_with_enumerator (synchronous, enumerator);

  xobject_unref (enumerator);
}

static void
_socket_connect_cb (xobject_t *object,
		    xasync_result_t *result,
		    xpointer_t user_data)
{
  xerror_t *error = NULL;
  xmain_loop_t *loop = user_data;
  xsocket_client_t *client = XSOCKET_CLIENT (object);
  xsocket_connection_t *connection;

  connection = xsocket_client_connect_to_uri_finish (client,
						      result,
						      &error);
  if (connection)
    {
      xsocket_address_t *proxy_addr;
      proxy_addr = xsocket_connection_get_remote_address (connection, NULL);
      print_proxy_address (proxy_addr);
    }
  else
    {
      print_and_free_error (error);
    }

  xmain_loop_quit (loop);
}

static void
use_socket_client (xboolean_t synchronous)
{
  xerror_t *error = NULL;
  xsocket_client_t *client;

  client = xsocket_client_new ();

  printf ("Proxies for URI '%s' are:\n", info);

  if (synchronous)
    {
      xsocket_connection_t *connection;
      xsocket_address_t *proxy_addr;

      connection = xsocket_client_connect_to_uri (client,
						   info,
						   0,
      						   cancellable,
						   &error);

      if (connection)
	{
	  proxy_addr = xsocket_connection_get_remote_address (connection, NULL);
	  print_proxy_address (proxy_addr);
	}
      else
	{
	  print_and_free_error (error);
	}
    }
  else
    {
      xmain_loop_t *loop = xmain_loop_new (NULL, FALSE);

      xsocket_client_connect_to_uri_async (client,
					    info,
					    0,
					    cancellable,
					    _socket_connect_cb,
					    loop);

      xmain_loop_run (loop);
      xmain_loop_unref (loop);
    }

  xobject_unref (client);
}

typedef enum
{
  USE_RESOLVER,
  USE_ENUMERATOR,
#ifdef G_OS_UNIX
  USE_UNIX_SOCKET_ADDRESS,
#endif
  USE_INET_SOCKET_ADDRESS,
  USE_PROXY_ADDRESS,
  USE_NETWORK_ADDRESS,
  USE_NETWORK_URI,
  USE_NETWORK_SERVICE,
  USE_SOCKET_CLIENT,
} ProxyTestType;

xint_t
main (xint_t argc, xchar_t **argv)
{
  xboolean_t synchronous = FALSE;
  xboolean_t cancel = FALSE;
  ProxyTestType type = USE_RESOLVER;

  while (argc >= 2 && argv[1][0] == '-')
    {
      if (!strcmp (argv[1], "-s"))
        synchronous = TRUE;
      else if (!strcmp (argv[1], "-c"))
        cancel = TRUE;
      else if (!strcmp (argv[1], "-e"))
        type = USE_ENUMERATOR;
      else if (!strcmp (argv[1], "-inet"))
        type = USE_INET_SOCKET_ADDRESS;
#ifdef G_OS_UNIX
      else if (!strcmp (argv[1], "-unix"))
        type = USE_UNIX_SOCKET_ADDRESS;
#endif
      else if (!strcmp (argv[1], "-proxyaddr"))
        type = USE_PROXY_ADDRESS;
      else if (!strcmp (argv[1], "-netaddr"))
        type = USE_NETWORK_ADDRESS;
      else if (!strcmp (argv[1], "-neturi"))
        type = USE_NETWORK_URI;
      else if (!strcmp (argv[1], "-netsrv"))
        type = USE_NETWORK_SERVICE;
      else if (!strcmp (argv[1], "-connect"))
        type = USE_SOCKET_CLIENT;
      else
	usage ();

      argv++;
      argc--;
    }

  if (argc != 2)
    usage ();

  /* Save URI for asynchronous callback */
  info = argv[1];

  if (cancel)
    {
      cancellable = g_cancellable_new ();
      g_cancellable_cancel (cancellable);
    }

  switch (type)
    {
    case USE_RESOLVER:
      use_resolver (synchronous);
      break;
    case USE_ENUMERATOR:
      use_enumerator (synchronous);
      break;
    case USE_INET_SOCKET_ADDRESS:
      use_inet_address (synchronous);
      break;
#ifdef G_OS_UNIX
    case USE_UNIX_SOCKET_ADDRESS:
      use_unix_address (synchronous);
      break;
#endif
    case USE_PROXY_ADDRESS:
      use_proxy_address (synchronous);
      break;
    case USE_NETWORK_ADDRESS:
      use_network_address (synchronous);
      break;
    case USE_NETWORK_URI:
      use_network_uri (synchronous);
      break;
    case USE_NETWORK_SERVICE:
      use_network_service (synchronous);
      break;
    case USE_SOCKET_CLIENT:
      use_socket_client (synchronous);
      break;
    }

  return return_value;
}
