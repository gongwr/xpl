/* #included into both socket-client.c and socket-server.c */

#ifdef G_OS_UNIX
static const char *unix_socket_address_types[] = {
  "invalid",
  "anonymous",
  "path",
  "abstract",
  "padded"
};
#endif

static char *
socket_address_to_string (xsocket_address_t *address)
{
  char *res = NULL;

  if (X_IS_INET_SOCKET_ADDRESS (address))
    {
      xinet_address_t *inet_address;
      char *str;
      int port;

      inet_address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (address));
      str = xinet_address_to_string (inet_address);
      port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (address));
      res = g_strdup_printf ("%s:%d", str, port);
      g_free (str);
    }
#ifdef G_OS_UNIX
  else if (X_IS_UNIX_SOCKET_ADDRESS (address))
    {
      GUnixSocketAddress *uaddr = G_UNIX_SOCKET_ADDRESS (address);

      res = g_strdup_printf ("%s:%s",
			     unix_socket_address_types[g_unix_socket_address_get_address_type (uaddr)],
			     g_unix_socket_address_get_path (uaddr));
    }
#endif

  return res;
}

static xsocket_address_t *
socket_address_from_string (const char *name)
{
#ifdef G_OS_UNIX
  xsize_t i, len;

  for (i = 0; i < G_N_ELEMENTS (unix_socket_address_types); i++)
    {
      len = strlen (unix_socket_address_types[i]);
      if (!strncmp (name, unix_socket_address_types[i], len) &&
	  name[len] == ':')
	{
	  return g_unix_socket_address_new_with_type (name + len + 1, -1,
						      (GUnixSocketAddressType)i);
	}
    }
#endif
  return NULL;
}

static xboolean_t
source_ready (GPollableInputStream *stream,
	      xpointer_t              data)
{
  g_main_loop_quit (loop);
  return FALSE;
}

static void
ensure_socket_condition (xsocket_t      *socket,
			 GIOCondition  condition,
			 xcancellable_t *cancellable)
{
  GSource *source;

  if (!non_blocking)
    return;

  source = xsocket_create_source (socket, condition, cancellable);
  g_source_set_callback (source,
			 (GSourceFunc) source_ready,
			 NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);
  g_main_loop_run (loop);
}

static void
ensure_connection_condition (xio_stream_t    *stream,
			     GIOCondition  condition,
			     xcancellable_t *cancellable)
{
  GSource *source;

  if (!non_blocking)
    return;

  if (condition & G_IO_IN)
    source = g_pollable_input_stream_create_source (G_POLLABLE_INPUT_STREAM (g_io_stream_get_input_stream (stream)), cancellable);
  else
    source = g_pollable_output_stream_create_source (G_POLLABLE_OUTPUT_STREAM (g_io_stream_get_output_stream (stream)), cancellable);

  g_source_set_callback (source,
			 (GSourceFunc) source_ready,
			 NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);
  g_main_loop_run (loop);
}

static xpointer_t
cancel_thread (xpointer_t data)
{
  xcancellable_t *cancellable = data;

  g_usleep (1000*1000*cancel_timeout);
  g_print ("Cancelling\n");
  g_cancellable_cancel (cancellable);
  return NULL;
}
