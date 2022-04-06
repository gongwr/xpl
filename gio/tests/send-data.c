#include <gio/gio.h>
#include <string.h>
#include <stdio.h>

xmain_loop_t *loop;

int cancel_timeout = 0;
int io_timeout = 0;
xboolean_t async = FALSE;
xboolean_t graceful = FALSE;
xboolean_t verbose = FALSE;
static GOptionEntry cmd_entries[] = {
  {"cancel", 'c', 0, G_OPTION_ARG_INT, &cancel_timeout,
   "Cancel any op after the specified amount of seconds", NULL},
  {"async", 'a', 0, G_OPTION_ARG_NONE, &async,
   "Use async ops", NULL},
  {"graceful-disconnect", 'g', 0, G_OPTION_ARG_NONE, &graceful,
   "Use graceful disconnect", NULL},
  {"timeout", 't', 0, G_OPTION_ARG_INT, &io_timeout,
   "Time out socket I/O after the specified number of seconds", NULL},
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Verbose debugging output", NULL},
  G_OPTION_ENTRY_NULL
};

static xpointer_t
cancel_thread (xpointer_t data)
{
  xcancellable_t *cancellable = data;

  g_usleep (1000*1000*cancel_timeout);
  g_print ("Cancelling\n");
  xcancellable_cancel (cancellable);
  return NULL;
}

static char *
socket_address_to_string (xsocket_address_t *address)
{
  xinet_address_t *inet_address;
  char *str, *res;
  int port;

  inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (address));
  str = xinet_address_to_string (inet_address);
  port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (address));
  res = xstrdup_printf ("%s:%d", str, port);
  g_free (str);
  return res;
}

static void
async_cb (xobject_t *source_object,
	  xasync_result_t *res,
	  xpointer_t user_data)
{
  xasync_result_t **resp = user_data;
  *resp = xobject_ref (res);
  xmain_loop_quit (loop);
}

static void
socket_client_event (xsocket_client_t *client,
		     GSocketClientEvent event,
		     xsocket_connectable_t *connectable,
		     xsocket_connection_t *connection)
{
  static xenum_class_t *event_class;
  sint64_t now_us;

  if (!event_class)
    event_class = xtype_class_ref (XTYPE_SOCKET_CLIENT_EVENT);

  now_us = g_get_real_time ();
  g_print ("%" G_GINT64_FORMAT " xsocket_client_t => %s [%s]\n",
	  now_us,
	  xenum_get_value (event_class, event)->value_nick,
	  connection ? G_OBJECT_TYPE_NAME (connection) : "");
}

int
main (int argc, char *argv[])
{
  xoption_context_t *context;
  xsocket_client_t *client;
  xsocket_connection_t *connection;
  xsocket_address_t *address;
  xcancellable_t *cancellable;
  xoutput_stream_t *out;
  xerror_t *error = NULL;
  char buffer[1000];

  context = g_option_context_new (" <hostname>[:port] - send data to tcp host");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (argc != 2)
    {
      g_printerr ("%s: %s\n", argv[0], "Need to specify hostname");
      return 1;
    }

  if (async)
    loop = xmain_loop_new (NULL, FALSE);

  if (cancel_timeout)
    {
      xthread_t *thread;
      cancellable = xcancellable_new ();
      thread = xthread_new ("cancel", cancel_thread, cancellable);
      xthread_unref (thread);
    }
  else
    {
      cancellable = NULL;
    }

  client = xsocket_client_new ();
  if (io_timeout)
    xsocket_client_set_timeout (client, io_timeout);
  if (verbose)
    xsignal_connect (client, "event", G_CALLBACK (socket_client_event), NULL);

  if (async)
    {
      xasync_result_t *res;
      xsocket_client_connect_to_host_async (client, argv[1], 7777,
					     cancellable, async_cb, &res);
      xmain_loop_run (loop);
      connection = xsocket_client_connect_to_host_finish (client, res, &error);
      xobject_unref (res);
    }
  else
    {
      connection = xsocket_client_connect_to_host (client,
						    argv[1],
						    7777,
						    cancellable, &error);
    }
  if (connection == NULL)
    {
      g_printerr ("%s can't connect: %s\n", argv[0], error->message);
      return 1;
    }
  xobject_unref (client);

  address = xsocket_connection_get_remote_address (connection, &error);
  if (!address)
    {
      g_printerr ("Error getting remote address: %s\n",
		  error->message);
      return 1;
    }
  g_print ("Connected to address: %s\n",
	   socket_address_to_string (address));
  xobject_unref (address);

  if (graceful)
    g_tcp_connection_set_graceful_disconnect (G_TCP_CONNECTION (connection), TRUE);

  out = g_io_stream_get_output_stream (XIO_STREAM (connection));

  while (fgets(buffer, sizeof (buffer), stdin) != NULL)
    {
      /* FIXME if (async) */
      if (!xoutput_stream_write_all (out, buffer, strlen (buffer),
				      NULL, cancellable, &error))
	{
	  g_warning ("send error: %s",  error->message);
	  xerror_free (error);
	  error = NULL;
	}
    }

  g_print ("closing stream\n");
  if (async)
    {
      xasync_result_t *res;
      g_io_stream_close_async (XIO_STREAM (connection),
			       0, cancellable, async_cb, &res);
      xmain_loop_run (loop);
      if (!g_io_stream_close_finish (XIO_STREAM (connection),
				     res, &error))
	{
	  xobject_unref (res);
	  g_warning ("close error: %s",  error->message);
	  return 1;
	}
      xobject_unref (res);
    }
  else
    {
      if (!g_io_stream_close (XIO_STREAM (connection), cancellable, &error))
	{
	  g_warning ("close error: %s",  error->message);
	  return 1;
	}
    }

  xobject_unref (connection);

  return 0;
}
