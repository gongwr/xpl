/*

Usage examples (modulo addresses / credentials).

UNIX domain socket transport:

 Server:
   $ ./gdbus-example-peer --server --address unix:abstract=myaddr
   Server is listening at: unix:abstract=myaddr
   Client connected.
   Peer credentials: xcredentials_t:unix-user=500,unix-group=500,unix-process=13378
   Negotiated capabilities: unix-fd-passing=1
   Client said: Hey, it's 1273093080 already!

 Client:
   $ ./gdbus-example-peer --address unix:abstract=myaddr
   Connected.
   Negotiated capabilities: unix-fd-passing=1
   Server said: You said 'Hey, it's 1273093080 already!'. KTHXBYE!

Nonce-secured TCP transport on the same host:

 Server:
   $ ./gdbus-example-peer --server --address nonce-tcp:
   Server is listening at: nonce-tcp:host=localhost,port=43077,noncefile=/tmp/gdbus-nonce-file-X1ZNCV
   Client connected.
   Peer credentials: (no credentials received)
   Negotiated capabilities: unix-fd-passing=0
   Client said: Hey, it's 1273093206 already!

 Client:
   $ ./gdbus-example-peer -address nonce-tcp:host=localhost,port=43077,noncefile=/tmp/gdbus-nonce-file-X1ZNCV
   Connected.
   Negotiated capabilities: unix-fd-passing=0
   Server said: You said 'Hey, it's 1273093206 already!'. KTHXBYE!

TCP transport on two different hosts with a shared home directory:

 Server:
   host1 $ ./gdbus-example-peer --server --address tcp:host=0.0.0.0
   Server is listening at: tcp:host=0.0.0.0,port=46314
   Client connected.
   Peer credentials: (no credentials received)
   Negotiated capabilities: unix-fd-passing=0
   Client said: Hey, it's 1273093337 already!

 Client:
   host2 $ ./gdbus-example-peer -a tcp:host=host1,port=46314
   Connected.
   Negotiated capabilities: unix-fd-passing=0
   Server said: You said 'Hey, it's 1273093337 already!'. KTHXBYE!

TCP transport on two different hosts without authentication:

 Server:
   host1 $ ./gdbus-example-peer --server --address tcp:host=0.0.0.0 --allow-anonymous
   Server is listening at: tcp:host=0.0.0.0,port=59556
   Client connected.
   Peer credentials: (no credentials received)
   Negotiated capabilities: unix-fd-passing=0
   Client said: Hey, it's 1273093652 already!

 Client:
   host2 $ ./gdbus-example-peer -a tcp:host=host1,port=59556
   Connected.
   Negotiated capabilities: unix-fd-passing=0
   Server said: You said 'Hey, it's 1273093652 already!'. KTHXBYE!

 */

#include <gio/gio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_node_info_t *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const xchar_t introspection_xml[] =
  "<node>"
  "  <interface name='org.gtk.GDBus.TestPeerInterface'>"
  "    <method name='HelloWorld'>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

/* ---------------------------------------------------------------------------------------------------- */

static void
handle_method_call (xdbus_connection_t       *connection,
                    const xchar_t           *sender,
                    const xchar_t           *object_path,
                    const xchar_t           *interface_name,
                    const xchar_t           *method_name,
                    xvariant_t              *parameters,
                    xdbus_method_invocation_t *invocation,
                    xpointer_t               user_data)
{
  if (xstrcmp0 (method_name, "HelloWorld") == 0)
    {
      const xchar_t *greeting;
      xchar_t *response;

      xvariant_get (parameters, "(&s)", &greeting);
      response = xstrdup_printf ("You said '%s'. KTHXBYE!", greeting);
      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new ("(s)", response));
      g_free (response);
      g_print ("Client said: %s\n", greeting);
    }
}

static const xdbus_interface_vtable_t interface_vtable =
{
  handle_method_call,
  NULL,
  NULL,
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static void
connection_closed (xdbus_connection_t *connection,
                   xboolean_t remote_peer_vanished,
                   xerror_t *Error,
                   xpointer_t user_data)
{
  g_print ("Client disconnected.\n");
  xobject_unref (connection);
}

static xboolean_t
on_new_connection (xdbus_server_t *server,
                   xdbus_connection_t *connection,
                   xpointer_t user_data)
{
  xuint_t registration_id;
  xcredentials_t *credentials;
  xchar_t *s;

  credentials = xdbus_connection_get_peer_credentials (connection);
  if (credentials == NULL)
    s = xstrdup ("(no credentials received)");
  else
    s = xcredentials_to_string (credentials);


  g_print ("Client connected.\n"
           "Peer credentials: %s\n"
           "Negotiated capabilities: unix-fd-passing=%d\n",
           s,
           xdbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);

  xobject_ref (connection);
  xsignal_connect (connection, "closed", G_CALLBACK (connection_closed), NULL);
  registration_id = xdbus_connection_register_object (connection,
                                                       "/org/gtk/GDBus/test_object_t",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL,  /* user_data */
                                                       NULL,  /* user_data_free_func */
                                                       NULL); /* xerror_t** */
  xassert (registration_id > 0);

  return TRUE;
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
allow_mechanism_cb (xdbus_auth_observer_t *observer,
                    const xchar_t *mechanism,
                    G_GNUC_UNUSED xpointer_t user_data)
{
  /*
   * In a production xdbus_server_t that only needs to work on modern Unix
   * platforms, consider requiring EXTERNAL (credentials-passing),
   * which is the recommended authentication mechanism for AF_UNIX
   * sockets:
   *
   * if (xstrcmp0 (mechanism, "EXTERNAL") == 0)
   *   return TRUE;
   *
   * return FALSE;
   *
   * For this example we accept everything.
   */

  g_print ("Considering whether to accept %s authentication...\n", mechanism);
  return TRUE;
}

static xboolean_t
authorize_authenticated_peer_cb (xdbus_auth_observer_t *observer,
                                 G_GNUC_UNUSED xio_stream_t *stream,
                                 xcredentials_t *credentials,
                                 G_GNUC_UNUSED xpointer_t user_data)
{
  xboolean_t authorized = FALSE;

  g_print ("Considering whether to authorize authenticated peer...\n");

  if (credentials != NULL)
    {
      xcredentials_t *own_credentials;
      xchar_t *credentials_string = NULL;

      credentials_string = xcredentials_to_string (credentials);
      g_print ("Peer's credentials: %s\n", credentials_string);
      g_free (credentials_string);

      own_credentials = xcredentials_new ();

      credentials_string = xcredentials_to_string (own_credentials);
      g_print ("Server's credentials: %s\n", credentials_string);
      g_free (credentials_string);

      if (xcredentials_is_same_user (credentials, own_credentials, NULL))
        authorized = TRUE;

      xobject_unref (own_credentials);
    }

  if (!authorized)
    {
      /* In most servers you'd want to reject this, but for this example
       * we allow it. */
      g_print ("A server would often not want to authorize this identity\n");
      g_print ("Authorizing it anyway for demonstration purposes\n");
      authorized = TRUE;
    }

  return authorized;
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int argc, char *argv[])
{
  xint_t ret;
  xboolean_t opt_server;
  xchar_t *opt_address;
  xoption_context_t *opt_context;
  xboolean_t opt_allow_anonymous;
  xerror_t *error;
  GOptionEntry opt_entries[] =
    {
      { "server", 's', 0, G_OPTION_ARG_NONE, &opt_server, "Start a server instead of a client", NULL },
      { "address", 'a', 0, G_OPTION_ARG_STRING, &opt_address, "D-Bus address to use", NULL },
      { "allow-anonymous", 'n', 0, G_OPTION_ARG_NONE, &opt_allow_anonymous, "Allow anonymous authentication", NULL },
      G_OPTION_ENTRY_NULL
    };

  ret = 1;

  opt_address = NULL;
  opt_server = FALSE;
  opt_allow_anonymous = FALSE;

  opt_context = g_option_context_new ("peer-to-peer example");
  error = NULL;
  g_option_context_add_main_entries (opt_context, opt_entries, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &error))
    {
      g_printerr ("Error parsing options: %s\n", error->message);
      xerror_free (error);
      goto out;
    }
  if (opt_address == NULL)
    {
      g_printerr ("Incorrect usage, try --help.\n");
      goto out;
    }
  if (!opt_server && opt_allow_anonymous)
    {
      g_printerr ("The --allow-anonymous option only makes sense when used with --server.\n");
      goto out;
    }

  /* We are lazy here - we don't want to manually provide
   * the introspection data structures - so we just build
   * them from XML.
   */
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  xassert (introspection_data != NULL);

  if (opt_server)
    {
      xdbus_auth_observer_t *observer;
      xdbus_server_t *server;
      xchar_t *guid;
      xmain_loop_t *loop;
      xdbus_server_flags_t server_flags;

      guid = g_dbus_generate_guid ();

      server_flags = G_DBUS_SERVER_FLAGS_NONE;
      if (opt_allow_anonymous)
        server_flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;

      observer = xdbus_auth_observer_new ();
      xsignal_connect (observer, "allow-mechanism", G_CALLBACK (allow_mechanism_cb), NULL);
      xsignal_connect (observer, "authorize-authenticated-peer", G_CALLBACK (authorize_authenticated_peer_cb), NULL);

      error = NULL;
      server = xdbus_server_new_sync (opt_address,
                                       server_flags,
                                       guid,
                                       observer,
                                       NULL, /* xcancellable_t */
                                       &error);
      xdbus_server_start (server);

      xobject_unref (observer);
      g_free (guid);

      if (server == NULL)
        {
          g_printerr ("Error creating server at address %s: %s\n", opt_address, error->message);
          xerror_free (error);
          goto out;
        }
      g_print ("Server is listening at: %s\n", xdbus_server_get_client_address (server));
      xsignal_connect (server,
                        "new-connection",
                        G_CALLBACK (on_new_connection),
                        NULL);

      loop = xmain_loop_new (NULL, FALSE);
      xmain_loop_run (loop);

      xobject_unref (server);
      xmain_loop_unref (loop);
    }
  else
    {
      xdbus_connection_t *connection;
      const xchar_t *greeting_response;
      xvariant_t *value;
      xchar_t *greeting;

      error = NULL;
      connection = xdbus_connection_new_for_address_sync (opt_address,
                                                           G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                           NULL, /* xdbus_auth_observer_t */
                                                           NULL, /* xcancellable_t */
                                                           &error);
      if (connection == NULL)
        {
          g_printerr ("Error connecting to D-Bus address %s: %s\n", opt_address, error->message);
          xerror_free (error);
          goto out;
        }

      g_print ("Connected.\n"
               "Negotiated capabilities: unix-fd-passing=%d\n",
               xdbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING);

      greeting = xstrdup_printf ("Hey, it's %" G_GINT64_FORMAT " already!",
                                  g_get_real_time () / G_USEC_PER_SEC);
      value = xdbus_connection_call_sync (connection,
                                           NULL, /* bus_name */
                                           "/org/gtk/GDBus/test_object_t",
                                           "org.gtk.GDBus.TestPeerInterface",
                                           "HelloWorld",
                                           xvariant_new ("(s)", greeting),
                                           G_VARIANT_TYPE ("(s)"),
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);
      if (value == NULL)
        {
          g_printerr ("Error invoking HelloWorld(): %s\n", error->message);
          xerror_free (error);
          goto out;
        }
      xvariant_get (value, "(&s)", &greeting_response);
      g_print ("Server said: %s\n", greeting_response);
      xvariant_unref (value);

      xobject_unref (connection);
    }
  g_dbus_node_info_unref (introspection_data);

  ret = 0;

 out:
  return ret;
}
