/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
#include <glib.h>
#include "glibintl.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#include <gio/gio.h>

static xresolver_t *resolver;
static xcancellable_t *cancellable;
static xmain_loop_t *loop;
static int nlookups = 0;
static xboolean_t synchronous = FALSE;
static xuint_t connectable_count = 0;
static GResolverRecordType record_type = 0;

static G_NORETURN void
usage (void)
{
	fprintf (stderr, "Usage: resolver [-s] [hostname | IP | service/protocol/domain ] ...\n");
	fprintf (stderr, "Usage: resolver [-s] [-t MX|TXT|NS|SOA] rrname ...\n");
	fprintf (stderr, "       resolver [-s] -c NUMBER [hostname | IP | service/protocol/domain ]\n");
	fprintf (stderr, "       Use -s to do synchronous lookups.\n");
	fprintf (stderr, "       Use -c NUMBER (and only a single resolvable argument) to test xsocket_connectable_t.\n");
	fprintf (stderr, "       The given NUMBER determines how many times the connectable will be enumerated.\n");
	fprintf (stderr, "       Use -t with MX, TXT, NS or SOA to look up DNS records of those types.\n");
	exit (1);
}

G_LOCK_DEFINE_STATIC (response);

static void
done_lookup (void)
{
  nlookups--;
  if (nlookups == 0)
    {
      /* In the sync case we need to make sure we don't call
       * xmain_loop_quit before the loop is actually running...
       */
      g_idle_add ((xsource_func_t)xmain_loop_quit, loop);
    }
}

static void
print_resolved_name (const char *phys,
                     char       *name,
                     xerror_t     *error)
{
  G_LOCK (response);
  printf ("Address: %s\n", phys);
  if (error)
    {
      printf ("Error:   %s\n", error->message);
      xerror_free (error);
    }
  else
    {
      printf ("Name:    %s\n", name);
      g_free (name);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_addresses (const char *name,
                          xlist_t      *addresses,
			  xerror_t     *error)
{
  char *phys;
  xlist_t *a;

  G_LOCK (response);
  printf ("Name:    %s\n", name);
  if (error)
    {
      printf ("Error:   %s\n", error->message);
      xerror_free (error);
    }
  else
    {
      for (a = addresses; a; a = a->next)
	{
	  phys = xinet_address_to_string (a->data);
	  printf ("Address: %s\n", phys);
	  g_free (phys);
          xobject_unref (a->data);
	}
      xlist_free (addresses);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_service (const char *service,
                        xlist_t      *targets,
			xerror_t     *error)
{
  xlist_t *t;

  G_LOCK (response);
  printf ("Service: %s\n", service);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      xerror_free (error);
    }
  else
    {
      for (t = targets; t; t = t->next)
	{
	  printf ("%s:%u (pri %u, weight %u)\n",
		  g_srv_target_get_hostname (t->data),
		  (xuint_t)g_srv_target_get_port (t->data),
		  (xuint_t)g_srv_target_get_priority (t->data),
		  (xuint_t)g_srv_target_get_weight (t->data));
          g_srv_target_free (t->data);
	}
      xlist_free (targets);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_mx (const char *rrname,
                   xlist_t      *records,
                   xerror_t     *error)
{
  const xchar_t *hostname;
  xuint16_t priority;
  xlist_t *t;

  G_LOCK (response);
  printf ("Domain: %s\n", rrname);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      xerror_free (error);
    }
  else if (!records)
    {
      printf ("no MX records\n");
    }
  else
    {
      for (t = records; t; t = t->next)
        {
          xvariant_get (t->data, "(q&s)", &priority, &hostname);
          printf ("%s (pri %u)\n", hostname, (xuint_t)priority);
          xvariant_unref (t->data);
        }
      xlist_free (records);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_txt (const char *rrname,
                    xlist_t      *records,
                    xerror_t     *error)
{
  const xchar_t **contents;
  xlist_t *t;
  xint_t i;

  G_LOCK (response);
  printf ("Domain: %s\n", rrname);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      xerror_free (error);
    }
  else if (!records)
    {
      printf ("no TXT records\n");
    }
  else
    {
      for (t = records; t; t = t->next)
        {
          if (t != records)
            printf ("\n");
          xvariant_get (t->data, "(^a&s)", &contents);
          for (i = 0; contents[i] != NULL; i++)
            printf ("%s\n", contents[i]);
          xvariant_unref (t->data);
          g_free (contents);
        }
      xlist_free (records);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_soa (const char *rrname,
                    xlist_t      *records,
                    xerror_t     *error)
{
  xlist_t *t;
  const xchar_t *primary_ns;
  const xchar_t *administrator;
  xuint32_t serial, refresh, retry, expire, ttl;

  G_LOCK (response);
  printf ("Zone: %s\n", rrname);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      xerror_free (error);
    }
  else if (!records)
    {
      printf ("no SOA records\n");
    }
  else
    {
      for (t = records; t; t = t->next)
        {
          xvariant_get (t->data, "(&s&suuuuu)", &primary_ns, &administrator,
                         &serial, &refresh, &retry, &expire, &ttl);
          printf ("%s %s (serial %u, refresh %u, retry %u, expire %u, ttl %u)\n",
                  primary_ns, administrator, (xuint_t)serial, (xuint_t)refresh,
                  (xuint_t)retry, (xuint_t)expire, (xuint_t)ttl);
          xvariant_unref (t->data);
        }
      xlist_free (records);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
print_resolved_ns (const char *rrname,
                    xlist_t      *records,
                    xerror_t     *error)
{
  xlist_t *t;
  const xchar_t *hostname;

  G_LOCK (response);
  printf ("Zone: %s\n", rrname);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      xerror_free (error);
    }
  else if (!records)
    {
      printf ("no NS records\n");
    }
  else
    {
      for (t = records; t; t = t->next)
        {
          xvariant_get (t->data, "(&s)", &hostname);
          printf ("%s\n", hostname);
          xvariant_unref (t->data);
        }
      xlist_free (records);
    }
  printf ("\n");

  done_lookup ();
  G_UNLOCK (response);
}

static void
lookup_one_sync (const char *arg)
{
  xerror_t *error = NULL;

  if (record_type != 0)
    {
      xlist_t *records;

      records = g_resolver_lookup_records (resolver, arg, record_type, cancellable, &error);
      switch (record_type)
      {
        case G_RESOLVER_RECORD_MX:
          print_resolved_mx (arg, records, error);
          break;
        case G_RESOLVER_RECORD_SOA:
          print_resolved_soa (arg, records, error);
          break;
        case G_RESOLVER_RECORD_NS:
          print_resolved_ns (arg, records, error);
          break;
        case G_RESOLVER_RECORD_TXT:
          print_resolved_txt (arg, records, error);
          break;
        default:
          g_warn_if_reached ();
          break;
      }
    }
  else if (strchr (arg, '/'))
    {
      xlist_t *targets;
      /* service/protocol/domain */
      char **parts = xstrsplit (arg, "/", 3);

      if (!parts || !parts[2])
	usage ();

      targets = g_resolver_lookup_service (resolver,
                                           parts[0], parts[1], parts[2],
                                           cancellable, &error);
      print_resolved_service (arg, targets, error);
    }
  else if (g_hostname_is_ip_address (arg))
    {
      xinet_address_t *addr = xinet_address_new_from_string (arg);
      char *name;

      name = g_resolver_lookup_by_address (resolver, addr, cancellable, &error);
      print_resolved_name (arg, name, error);
      xobject_unref (addr);
    }
  else
    {
      xlist_t *addresses;

      addresses = g_resolver_lookup_by_name (resolver, arg, cancellable, &error);
      print_resolved_addresses (arg, addresses, error);
    }
}

static xpointer_t
lookup_thread (xpointer_t arg)
{
  lookup_one_sync (arg);
  return NULL;
}

static void
start_sync_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    {
      xthread_t *thread;
      thread = xthread_new ("lookup", lookup_thread, argv[i]);
      xthread_unref (thread);
    }
}

static void
lookup_by_addr_callback (xobject_t *source, xasync_result_t *result,
                         xpointer_t user_data)
{
  const char *phys = user_data;
  xerror_t *error = NULL;
  char *name;

  name = g_resolver_lookup_by_address_finish (resolver, result, &error);
  print_resolved_name (phys, name, error);
}

static void
lookup_by_name_callback (xobject_t *source, xasync_result_t *result,
                         xpointer_t user_data)
{
  const char *name = user_data;
  xerror_t *error = NULL;
  xlist_t *addresses;

  addresses = g_resolver_lookup_by_name_finish (resolver, result, &error);
  print_resolved_addresses (name, addresses, error);
}

static void
lookup_service_callback (xobject_t *source, xasync_result_t *result,
			 xpointer_t user_data)
{
  const char *service = user_data;
  xerror_t *error = NULL;
  xlist_t *targets;

  targets = g_resolver_lookup_service_finish (resolver, result, &error);
  print_resolved_service (service, targets, error);
}

static void
lookup_records_callback (xobject_t      *source,
                         xasync_result_t *result,
                         xpointer_t      user_data)
{
  const char *arg = user_data;
  xerror_t *error = NULL;
  xlist_t *records;

  records = g_resolver_lookup_records_finish (resolver, result, &error);

  switch (record_type)
  {
    case G_RESOLVER_RECORD_MX:
      print_resolved_mx (arg, records, error);
      break;
    case G_RESOLVER_RECORD_SOA:
      print_resolved_soa (arg, records, error);
      break;
    case G_RESOLVER_RECORD_NS:
      print_resolved_ns (arg, records, error);
      break;
    case G_RESOLVER_RECORD_TXT:
      print_resolved_txt (arg, records, error);
      break;
    default:
      g_warn_if_reached ();
      break;
  }
}

static void
start_async_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    {
      if (record_type != 0)
	{
	  g_resolver_lookup_records_async (resolver, argv[i], record_type,
	                                   cancellable, lookup_records_callback, argv[i]);
	}
      else if (strchr (argv[i], '/'))
	{
	  /* service/protocol/domain */
	  char **parts = xstrsplit (argv[i], "/", 3);

	  if (!parts || !parts[2])
	    usage ();

	  g_resolver_lookup_service_async (resolver,
					   parts[0], parts[1], parts[2],
					   cancellable,
					   lookup_service_callback, argv[i]);
	}
      else if (g_hostname_is_ip_address (argv[i]))
	{
          xinet_address_t *addr = xinet_address_new_from_string (argv[i]);

	  g_resolver_lookup_by_address_async (resolver, addr, cancellable,
                                              lookup_by_addr_callback, argv[i]);
	  xobject_unref (addr);
	}
      else
	{
	  g_resolver_lookup_by_name_async (resolver, argv[i], cancellable,
                                           lookup_by_name_callback,
                                           argv[i]);
	}

      /* Stress-test the reloading code */
      g_signal_emit_by_name (resolver, "reload");
    }
}

static void
print_connectable_sockaddr (xsocket_address_t *sockaddr,
                            xerror_t         *error)
{
  char *phys;

  if (error)
    {
      printf ("Error:   %s\n", error->message);
      xerror_free (error);
    }
  else if (!X_IS_INET_SOCKET_ADDRESS (sockaddr))
    {
      printf ("Error:   Unexpected sockaddr type '%s'\n", xtype_name_from_instance ((GTypeInstance *)sockaddr));
      xobject_unref (sockaddr);
    }
  else
    {
      xinet_socket_address_t *isa = G_INET_SOCKET_ADDRESS (sockaddr);
      phys = xinet_address_to_string (g_inet_socket_address_get_address (isa));
      printf ("Address: %s%s%s:%d\n",
              strchr (phys, ':') ? "[" : "", phys, strchr (phys, ':') ? "]" : "",
              g_inet_socket_address_get_port (isa));
      g_free (phys);
      xobject_unref (sockaddr);
    }
}

static void
do_sync_connectable (xsocket_address_enumerator_t *enumerator)
{
  xsocket_address_t *sockaddr;
  xerror_t *error = NULL;

  while ((sockaddr = xsocket_address_enumerator_next (enumerator, cancellable, &error)))
    print_connectable_sockaddr (sockaddr, error);

  xobject_unref (enumerator);
  done_lookup ();
}

static void do_async_connectable (xsocket_address_enumerator_t *enumerator);

static void
got_next_async (xobject_t *source, xasync_result_t *result, xpointer_t user_data)
{
  xsocket_address_enumerator_t *enumerator = XSOCKET_ADDRESS_ENUMERATOR (source);
  xsocket_address_t *sockaddr;
  xerror_t *error = NULL;

  sockaddr = xsocket_address_enumerator_next_finish (enumerator, result, &error);
  if (sockaddr || error)
    print_connectable_sockaddr (sockaddr, error);
  if (sockaddr)
    do_async_connectable (enumerator);
  else
    {
      xobject_unref (enumerator);
      done_lookup ();
    }
}

static void
do_async_connectable (xsocket_address_enumerator_t *enumerator)
{
  xsocket_address_enumerator_next_async (enumerator, cancellable,
                                          got_next_async, NULL);
}

static void
do_connectable (const char *arg, xboolean_t synchronous, xuint_t count)
{
  char **parts;
  xsocket_connectable_t *connectable;
  xsocket_address_enumerator_t *enumerator;

  if (strchr (arg, '/'))
    {
      /* service/protocol/domain */
      parts = xstrsplit (arg, "/", 3);
      if (!parts || !parts[2])
	usage ();

      connectable = g_network_service_new (parts[0], parts[1], parts[2]);
    }
  else
    {
      xuint16_t port;

      parts = xstrsplit (arg, ":", 2);
      if (parts && parts[1])
	{
	  arg = parts[0];
	  port = strtoul (parts[1], NULL, 10);
	}
      else
	port = 0;

      if (g_hostname_is_ip_address (arg))
	{
	  xinet_address_t *addr = xinet_address_new_from_string (arg);
	  xsocket_address_t *sockaddr = g_inet_socket_address_new (addr, port);

	  xobject_unref (addr);
	  connectable = XSOCKET_CONNECTABLE (sockaddr);
	}
      else
        connectable = g_network_address_new (arg, port);
    }

  while (count--)
    {
      enumerator = xsocket_connectable_enumerate (connectable);

      if (synchronous)
        do_sync_connectable (enumerator);
      else
        do_async_connectable (enumerator);
    }

  xobject_unref (connectable);
}

#ifdef G_OS_UNIX
static int cancel_fds[2];

static void
interrupted (int sig)
{
  xssize_t c;

  signal (SIGINT, SIG_DFL);
  c = write (cancel_fds[1], "x", 1);
  g_assert_cmpint(c, ==, 1);
}

static xboolean_t
async_cancel (xio_channel_t *source, xio_condition_t cond, xpointer_t cancel)
{
  g_cancellable_cancel (cancel);
  return FALSE;
}
#endif


static xboolean_t
record_type_arg (const xchar_t *option_name,
                 const xchar_t *value,
                 xpointer_t data,
                 xerror_t **error)
{
  if (g_ascii_strcasecmp (value, "MX") == 0) {
    record_type = G_RESOLVER_RECORD_MX;
  } else if (g_ascii_strcasecmp (value, "TXT") == 0) {
    record_type = G_RESOLVER_RECORD_TXT;
  } else if (g_ascii_strcasecmp (value, "SOA") == 0) {
    record_type = G_RESOLVER_RECORD_SOA;
  } else if (g_ascii_strcasecmp (value, "NS") == 0) {
    record_type = G_RESOLVER_RECORD_NS;
  } else {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Specify MX, TXT, NS or SOA for the special record lookup types");
      return FALSE;
  }

  return TRUE;
}

static const GOptionEntry option_entries[] = {
  { "synchronous", 's', 0, G_OPTION_ARG_NONE, &synchronous, "Synchronous connections", NULL },
  { "connectable", 'c', 0, G_OPTION_ARG_INT, &connectable_count, "Connectable count", "C" },
  { "special-type", 't', 0, G_OPTION_ARG_CALLBACK, record_type_arg, "Record type like MX, TXT, NS or SOA", "RR" },
  G_OPTION_ENTRY_NULL,
};

int
main (int argc, char **argv)
{
  xoption_context_t *context;
  xerror_t *error = NULL;
#ifdef G_OS_UNIX
  xio_channel_t *chan;
  xuint_t watch;
#endif

  context = g_option_context_new ("lookups ...");
  g_option_context_add_main_entries (context, option_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      xerror_free (error);
      usage();
    }

  if (argc < 2 || (argc > 2 && connectable_count))
    usage ();

  resolver = g_resolver_get_default ();

  cancellable = g_cancellable_new ();

#ifdef G_OS_UNIX
  /* Set up cancellation; we want to cancel if the user ^C's the
   * program, but we can't cancel directly from an interrupt.
   */
  signal (SIGINT, interrupted);

  if (pipe (cancel_fds) == -1)
    {
      perror ("pipe");
      exit (1);
    }
  chan = g_io_channel_unix_new (cancel_fds[0]);
  watch = g_io_add_watch (chan, G_IO_IN, async_cancel, cancellable);
  g_io_channel_unref (chan);
#endif

  nlookups = argc - 1;
  loop = xmain_loop_new (NULL, TRUE);

  if (connectable_count)
    {
      nlookups = connectable_count;
      do_connectable (argv[1], synchronous, connectable_count);
    }
  else
    {
      if (synchronous)
        start_sync_lookups (argv + 1, argc - 1);
      else
        start_async_lookups (argv + 1, argc - 1);
    }

  xmain_loop_run (loop);
  xmain_loop_unref (loop);

#ifdef G_OS_UNIX
  xsource_remove (watch);
#endif
  xobject_unref (cancellable);
  g_option_context_free (context);

  return 0;
}
