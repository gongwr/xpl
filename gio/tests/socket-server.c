#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

xmain_loop_t *loop;

int port = 7777;
xboolean_t verbose = FALSE;
xboolean_t dont_reuse_address = FALSE;
xboolean_t non_blocking = FALSE;
xboolean_t use_udp = FALSE;
int cancel_timeout = 0;
int read_timeout = 0;
int delay = 0;
xboolean_t unix_socket = FALSE;
const char *tls_cert_file = NULL;

static GOptionEntry cmd_entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_INT, &port,
   "Local port to bind to", NULL},
  {"cancel", 'c', 0, G_OPTION_ARG_INT, &cancel_timeout,
   "Cancel any op after the specified amount of seconds", NULL},
  {"udp", 'u', 0, G_OPTION_ARG_NONE, &use_udp,
   "Use udp instead of tcp", NULL},
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Be verbose", NULL},
  {"no-reuse", 0, 0, G_OPTION_ARG_NONE, &dont_reuse_address,
   "Don't SOADDRREUSE", NULL},
  {"non-blocking", 'n', 0, G_OPTION_ARG_NONE, &non_blocking,
   "Enable non-blocking i/o", NULL},
#ifdef G_OS_UNIX
  {"unix", 'U', 0, G_OPTION_ARG_NONE, &unix_socket,
   "Use a unix socket instead of IP", NULL},
#endif
  {"delay", 'd', 0, G_OPTION_ARG_INT, &delay,
   "Delay responses by the specified number of seconds", NULL},
  {"timeout", 't', 0, G_OPTION_ARG_INT, &read_timeout,
   "Time out reads after the specified number of seconds", NULL},
  {"tls", 'T', 0, G_OPTION_ARG_STRING, &tls_cert_file,
   "Use TLS (SSL) with indicated server certificate", "CERTFILE"},
  G_OPTION_ENTRY_NULL
};

#include "socket-common.c"

int
main (int argc,
      char *argv[])
{
  xsocket_t *socket, *new_socket, *recv_socket;
  xsocket_address_t *src_address;
  xsocket_address_t *address;
  xsocket_type_t socket_type;
  xsocket_family_t socket_family;
  xerror_t *error = NULL;
  xoption_context_t *context;
  xcancellable_t *cancellable;
  char *display_addr;
  xtls_certificate_t *tlscert = NULL;
  xio_stream_t *connection;
  xinput_stream_t *istream;
  xoutput_stream_t *ostream;

  context = g_option_context_new (" - test_t xsocket_t server stuff");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (unix_socket && argc != 2)
    {
      g_printerr ("%s: %s\n", argv[0], "Need to specify unix socket name");
      return 1;
    }

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

  if (tls_cert_file)
    {
      if (use_udp)
	{
	  g_printerr ("DTLS (TLS over UDP) is not supported");
	  return 1;
	}

      tlscert = xtls_certificate_new_from_file (tls_cert_file, &error);
      if (!tlscert)
	{
	  g_printerr ("Could not read server certificate '%s': %s\n",
		      tls_cert_file, error->message);
	  return 1;
	}
    }

  loop = xmain_loop_new (NULL, FALSE);

  if (use_udp)
    socket_type = XSOCKET_TYPE_DATAGRAM;
  else
    socket_type = XSOCKET_TYPE_STREAM;

  if (unix_socket)
    socket_family = XSOCKET_FAMILY_UNIX;
  else
    socket_family = XSOCKET_FAMILY_IPV4;

  socket = xsocket_new (socket_family, socket_type, 0, &error);

  if (socket == NULL)
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (non_blocking)
    xsocket_set_blocking (socket, FALSE);

  if (unix_socket)
    {
      src_address = socket_address_from_string (argv[1]);
      if (src_address == NULL)
	{
	  g_printerr ("%s: Could not parse '%s' as unix socket name\n", argv[0], argv[1]);
	  return 1;
	}
    }
  else
    {
      src_address = g_inet_socket_address_new (xinet_address_new_any (XSOCKET_FAMILY_IPV4), port);
    }

  if (!xsocket_bind (socket, src_address, !dont_reuse_address, &error))
    {
      g_printerr ("Can't bind socket: %s\n", error->message);
      return 1;
    }
  xobject_unref (src_address);

  if (!use_udp)
    {
      if (!xsocket_listen (socket, &error))
	{
	  g_printerr ("Can't listen on socket: %s\n", error->message);
	  return 1;
	}

      address = xsocket_get_local_address (socket, &error);
      if (!address)
	{
	  g_printerr ("Error getting local address: %s\n",
		      error->message);
	  return 1;
	}
      display_addr = socket_address_to_string (address);
      g_print ("listening on %s...\n", display_addr);
      g_free (display_addr);

      ensure_socket_condition (socket, G_IO_IN, cancellable);
      new_socket = xsocket_accept (socket, cancellable, &error);
      if (!new_socket)
	{
	  g_printerr ("Error accepting socket: %s\n",
		      error->message);
	  return 1;
	}

      if (non_blocking)
	xsocket_set_blocking (new_socket, FALSE);
      if (read_timeout)
	xsocket_set_timeout (new_socket, read_timeout);

      address = xsocket_get_remote_address (new_socket, &error);
      if (!address)
	{
	  g_printerr ("Error getting remote address: %s\n",
		      error->message);
	  return 1;
	}

      display_addr = socket_address_to_string (address);
      g_print ("got a new connection from %s\n", display_addr);
      g_free(display_addr);
      xobject_unref (address);

      recv_socket = new_socket;

      connection = XIO_STREAM (xsocket_connection_factory_create_connection (recv_socket));
      xobject_unref (new_socket);
    }
  else
    {
      recv_socket = socket;
      connection = NULL;
    }

  if (tlscert)
    {
      xio_stream_t *tls_conn;

      tls_conn = xtls_server_connection_new (connection, tlscert, &error);
      if (!tls_conn)
	{
	  g_printerr ("Could not create TLS connection: %s\n",
		      error->message);
	  return 1;
	}

      if (!xtls_connection_handshake (G_TLS_CONNECTION (tls_conn),
				       cancellable, &error))
	{
	  g_printerr ("Error during TLS handshake: %s\n",
		      error->message);
	  return 1;
       }

      xobject_unref (connection);
      connection = tls_conn;
    }

  if (connection)
    {
      istream = g_io_stream_get_input_stream (connection);
      ostream = g_io_stream_get_output_stream (connection);
    }
  else
    {
      xassert (use_udp);
      istream = NULL;
      ostream = NULL;
    }

  while (TRUE)
    {
      xchar_t buffer[4096];
      xssize_t size;
      xsize_t to_send;

      if (use_udp)
	{
	  ensure_socket_condition (recv_socket, G_IO_IN, cancellable);
	  size = xsocket_receive_from (recv_socket, &address,
					buffer, sizeof buffer,
					cancellable, &error);
	}
      else
	{
	  ensure_connection_condition (connection, G_IO_IN, cancellable);
	  size = xinput_stream_read (istream,
				      buffer, sizeof buffer,
				      cancellable, &error);
	}

      if (size < 0)
	{
	  g_printerr ("Error receiving from socket: %s\n",
		      error->message);
	  return 1;
	}

      if (size == 0)
	break;

      g_print ("received %" G_GSSIZE_FORMAT " bytes of data", size);
      if (use_udp)
	g_print (" from %s", socket_address_to_string (address));
      g_print ("\n");

      if (verbose)
	g_print ("-------------------------\n"
		 "%.*s\n"
		 "-------------------------\n",
		 (int)size, buffer);

      to_send = size;

      if (delay)
	{
	  if (verbose)
	    g_print ("delaying %d seconds before response\n", delay);
	  g_usleep (1000 * 1000 * delay);
	}

      while (to_send > 0)
	{
	  if (use_udp)
	    {
	      ensure_socket_condition (recv_socket, G_IO_OUT, cancellable);
	      size = xsocket_send_to (recv_socket, address,
				       buffer, to_send, cancellable, &error);
	    }
	  else
	    {
	      ensure_connection_condition (connection, G_IO_OUT, cancellable);
	      size = xoutput_stream_write (ostream,
					    buffer, to_send,
					    cancellable, &error);
	    }

	  if (size < 0)
	    {
	      if (xerror_matches (error,
				   G_IO_ERROR,
				   G_IO_ERROR_WOULD_BLOCK))
		{
		  g_print ("socket send would block, handling\n");
		  xerror_free (error);
		  error = NULL;
		  continue;
		}
	      else
		{
		  g_printerr ("Error sending to socket: %s\n",
			      error->message);
		  return 1;
		}
	    }

	  g_print ("sent %" G_GSSIZE_FORMAT " bytes of data\n", size);

	  if (size == 0)
	    {
	      g_printerr ("Unexpected short write\n");
	      return 1;
	    }

	  to_send -= size;
	}
    }

  g_print ("connection closed\n");

  if (connection)
    {
      if (!g_io_stream_close (connection, NULL, &error))
	{
	  g_printerr ("Error closing connection stream: %s\n",
		      error->message);
	  return 1;
	}
      xobject_unref (connection);
    }

  if (!xsocket_close (socket, &error))
    {
      g_printerr ("Error closing socket: %s\n",
		  error->message);
      return 1;
    }
  xobject_unref (socket);

  return 0;
}
