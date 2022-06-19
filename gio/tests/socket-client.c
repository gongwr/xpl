#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gtlsconsoleinteraction.h"

xmain_loop_t *loop;

xboolean_t verbose = FALSE;
xboolean_t non_blocking = FALSE;
xboolean_t use_udp = FALSE;
int cancel_timeout = 0;
int read_timeout = 0;
xboolean_t unix_socket = FALSE;
xboolean_t tls = FALSE;

static GOptionEntry cmd_entries[] = {
  {"cancel", 'c', 0, G_OPTION_ARG_INT, &cancel_timeout,
   "Cancel any op after the specified amount of seconds", NULL},
  {"udp", 'u', 0, G_OPTION_ARG_NONE, &use_udp,
   "Use udp instead of tcp", NULL},
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Be verbose", NULL},
  {"non-blocking", 'n', 0, G_OPTION_ARG_NONE, &non_blocking,
   "Enable non-blocking i/o", NULL},
#ifdef G_OS_UNIX
  {"unix", 'U', 0, G_OPTION_ARG_NONE, &unix_socket,
   "Use a unix socket instead of IP", NULL},
#endif
  {"timeout", 't', 0, G_OPTION_ARG_INT, &read_timeout,
   "Time out reads after the specified number of seconds", NULL},
  {"tls", 'T', 0, G_OPTION_ARG_NONE, &tls,
   "Use TLS (SSL)", NULL},
  G_OPTION_ENTRY_NULL
};

#include "socket-common.c"

static xboolean_t
accept_certificate (xtls_client_connection_t *conn,
		    xtls_certificate_t      *cert,
		    xtls_certificate_flags_t  errors,
		    xpointer_t              user_data)
{
  g_print ("Certificate would have been rejected ( ");
  if (errors & G_TLS_CERTIFICATE_UNKNOWN_CA)
    g_print ("unknown-ca ");
  if (errors & G_TLS_CERTIFICATE_BAD_IDENTITY)
    g_print ("bad-identity ");
  if (errors & G_TLS_CERTIFICATE_NOT_ACTIVATED)
    g_print ("not-activated ");
  if (errors & G_TLS_CERTIFICATE_EXPIRED)
    g_print ("expired ");
  if (errors & G_TLS_CERTIFICATE_REVOKED)
    g_print ("revoked ");
  if (errors & G_TLS_CERTIFICATE_INSECURE)
    g_print ("insecure ");
  g_print (") but accepting anyway.\n");

  return TRUE;
}

static xtls_certificate_t *
lookup_client_certificate (xtls_client_connection_t  *conn,
			   xerror_t               **error)
{
  xlist_t *l, *accepted;
  xlist_t *c, *certificates;
  xtls_database_t *database;
  xtls_certificate_t *certificate = NULL;
  xtls_connection_t *base;

  accepted = xtls_client_connection_get_accepted_cas (conn);
  for (l = accepted; l != NULL; l = xlist_next (l))
    {
      base = G_TLS_CONNECTION (conn);
      database = xtls_connection_get_database (base);
      certificates = xtls_database_lookup_certificates_issued_by (database, l->data,
                                                                   xtls_connection_get_interaction (base),
                                                                   G_TLS_DATABASE_LOOKUP_KEYPAIR,
                                                                   NULL, error);
      if (error && *error)
        break;

      if (certificates)
          certificate = xobject_ref (certificates->data);

      for (c = certificates; c != NULL; c = xlist_next (c))
        xobject_unref (c->data);
      xlist_free (certificates);
    }

  for (l = accepted; l != NULL; l = xlist_next (l))
    xbyte_array_unref (l->data);
  xlist_free (accepted);

  if (certificate == NULL && error && !*error)
    g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_CERTIFICATE_REQUIRED,
                         "Server requested a certificate, but could not find relevant certificate in database.");
  return certificate;
}

static xboolean_t
make_connection (const char       *argument,
		 xtls_certificate_t  *certificate,
		 xcancellable_t     *cancellable,
		 xsocket_t         **socket,
		 xsocket_address_t  **address,
		 xio_stream_t       **connection,
		 xinput_stream_t    **istream,
		 xoutput_stream_t   **ostream,
		 xerror_t          **error)
{
  xsocket_type_t socket_type;
  xsocket_family_t socket_family;
  xsocket_address_enumerator_t *enumerator;
  xsocket_connectable_t *connectable;
  xsocket_address_t *src_address;
  xtls_interaction_t *interaction;
  xerror_t *err = NULL;

  if (use_udp)
    socket_type = XSOCKET_TYPE_DATAGRAM;
  else
    socket_type = XSOCKET_TYPE_STREAM;

  if (unix_socket)
    socket_family = XSOCKET_FAMILY_UNIX;
  else
    socket_family = XSOCKET_FAMILY_IPV4;

  *socket = xsocket_new (socket_family, socket_type, 0, error);
  if (*socket == NULL)
    return FALSE;

  if (read_timeout)
    xsocket_set_timeout (*socket, read_timeout);

  if (unix_socket)
    {
      xsocket_address_t *addr;

      addr = socket_address_from_string (argument);
      if (addr == NULL)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Could not parse '%s' as unix socket name", argument);
          return FALSE;
        }
      connectable = XSOCKET_CONNECTABLE (addr);
    }
  else
    {
      connectable = g_network_address_parse (argument, 7777, error);
      if (connectable == NULL)
        return FALSE;
    }

  enumerator = xsocket_connectable_enumerate (connectable);
  while (TRUE)
    {
      *address = xsocket_address_enumerator_next (enumerator, cancellable, error);
      if (*address == NULL)
        {
          if (error != NULL && *error == NULL)
            g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "No more addresses to try");
          return FALSE;
        }

      if (xsocket_connect (*socket, *address, cancellable, &err))
        break;
      g_message ("Connection to %s failed: %s, trying next", socket_address_to_string (*address), err->message);
      g_clear_error (&err);

      xobject_unref (*address);
    }
  xobject_unref (enumerator);

  g_print ("Connected to %s\n",
           socket_address_to_string (*address));

  src_address = xsocket_get_local_address (*socket, error);
  if (!src_address)
    {
      g_prefix_error (error, "Error getting local address: ");
      return FALSE;
    }

  g_print ("local address: %s\n",
           socket_address_to_string (src_address));
  xobject_unref (src_address);

  if (use_udp)
    {
      *connection = NULL;
      *istream = NULL;
      *ostream = NULL;
    }
  else
    *connection = XIO_STREAM (xsocket_connection_factory_create_connection (*socket));

  if (tls)
    {
      xio_stream_t *tls_conn;

      tls_conn = xtls_client_connection_new (*connection, connectable, error);
      if (!tls_conn)
        {
          g_prefix_error (error, "Could not create TLS connection: ");
          return FALSE;
        }

      xsignal_connect (tls_conn, "accept-certificate",
                        G_CALLBACK (accept_certificate), NULL);

      interaction = xtls_console_interaction_new ();
      xtls_connection_set_interaction (G_TLS_CONNECTION (tls_conn), interaction);
      xobject_unref (interaction);

      if (certificate)
        xtls_connection_set_certificate (G_TLS_CONNECTION (tls_conn), certificate);

      xobject_unref (*connection);
      *connection = XIO_STREAM (tls_conn);

      if (!xtls_connection_handshake (G_TLS_CONNECTION (tls_conn),
                                       cancellable, error))
        {
          g_prefix_error (error, "Error during TLS handshake: ");
          return FALSE;
        }
    }
  xobject_unref (connectable);

  if (*connection)
    {
      *istream = g_io_stream_get_input_stream (*connection);
      *ostream = g_io_stream_get_output_stream (*connection);
    }

  return TRUE;
}

int
main (int argc,
      char *argv[])
{
  xsocket_t *socket;
  xsocket_address_t *address;
  xerror_t *error = NULL;
  xoption_context_t *context;
  xcancellable_t *cancellable;
  xio_stream_t *connection;
  xinput_stream_t *istream;
  xoutput_stream_t *ostream;
  xsocket_address_t *src_address;
  xtls_certificate_t *certificate = NULL;
  xint_t i;

  address = NULL;
  connection = NULL;

  context = g_option_context_new (" <hostname>[:port] - test_t xsocket_t client stuff");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (argc != 2)
    {
      g_printerr ("%s: %s\n", argv[0], "Need to specify hostname / unix socket name");
      return 1;
    }

  if (use_udp && tls)
    {
      g_printerr ("DTLS (TLS over UDP) is not supported");
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

  loop = xmain_loop_new (NULL, FALSE);

  for (i = 0; i < 2; i++)
    {
      if (make_connection (argv[1], certificate, cancellable, &socket, &address,
                           &connection, &istream, &ostream, &error))
          break;

      if (xerror_matches (error, G_TLS_ERROR, G_TLS_ERROR_CERTIFICATE_REQUIRED))
        {
          g_clear_error (&error);
          certificate = lookup_client_certificate (G_TLS_CLIENT_CONNECTION (connection), &error);
          if (certificate != NULL)
            continue;
        }

      g_printerr ("%s: %s", argv[0], error->message);
      return 1;
    }

  /* TODO: test_t non-blocking connect/handshake */
  if (non_blocking)
    xsocket_set_blocking (socket, FALSE);

  while (TRUE)
    {
      xchar_t buffer[4096];
      xssize_t size;
      xsize_t to_send;

      if (fgets (buffer, sizeof buffer, stdin) == NULL)
	break;

      to_send = strlen (buffer);
      while (to_send > 0)
	{
	  if (use_udp)
	    {
	      ensure_socket_condition (socket, G_IO_OUT, cancellable);
	      size = xsocket_send_to (socket, address,
				       buffer, to_send,
				       cancellable, &error);
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

      if (use_udp)
	{
	  ensure_socket_condition (socket, G_IO_IN, cancellable);
	  size = xsocket_receive_from (socket, &src_address,
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
	g_print (" from %s", socket_address_to_string (src_address));
      g_print ("\n");

      if (verbose)
	g_print ("-------------------------\n"
		 "%.*s"
		 "-------------------------\n",
		 (int)size, buffer);

    }

  g_print ("closing socket\n");

  if (connection)
    {
      if (!g_io_stream_close (connection, cancellable, &error))
	{
	  g_printerr ("Error closing connection: %s\n",
		      error->message);
	  return 1;
	}
      xobject_unref (connection);
    }
  else
    {
      if (!xsocket_close (socket, &error))
	{
	  g_printerr ("Error closing socket: %s\n",
		      error->message);
	  return 1;
	}
    }

  xobject_unref (socket);
  xobject_unref (address);

  return 0;
}
