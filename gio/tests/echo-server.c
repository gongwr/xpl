#include <gio/gio.h>
#include <string.h>

#define MESSAGE "Welcome to the echo service!\n"

int port = 7777;
static GOptionEntry cmd_entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_INT, &port,
   "Local port to bind to", NULL},
  G_OPTION_ENTRY_NULL
};


static xboolean_t
handler (xthreaded_socket_service_t *service,
         xsocket_connection_t      *connection,
         xsocket_listener_t        *listener,
         xpointer_t                user_data)
{
  xoutput_stream_t *out;
  xinput_stream_t *in;
  char buffer[1024];
  xssize_t size;

  out = g_io_stream_get_output_stream (XIO_STREAM (connection));
  in = g_io_stream_get_input_stream (XIO_STREAM (connection));

  xoutput_stream_write_all (out, MESSAGE, strlen (MESSAGE),
                             NULL, NULL, NULL);

  while (0 < (size = xinput_stream_read (in, buffer,
                                          sizeof buffer, NULL, NULL)))
    xoutput_stream_write (out, buffer, size, NULL, NULL);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  xsocket_service_t *service;
  xoption_context_t *context;
  xerror_t *error = NULL;

  context = g_option_context_new (" - test_t xsocket_t server stuff");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  service = xthreaded_socket_service_new (10);

  if (!xsocket_listener_add_inet_port (XSOCKET_LISTENER (service),
					port,
					NULL,
					&error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  g_print ("Echo service listening on port %d\n", port);

  xsignal_connect (service, "run", G_CALLBACK (handler), NULL);

  xmain_loop_run (xmain_loop_new (NULL, FALSE));
  g_assert_not_reached ();
}
