#include <gio/gio.h>
#include <string.h>

static int port = 8080;
static char *root = NULL;
static GOptionEntry cmd_entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_INT, &port,
   "Local port to bind to", NULL},
  G_OPTION_ENTRY_NULL
};

static void
send_error (xoutput_stream_t *out,
	    int error_code,
	    const char *reason)
{
  char *res;

  res = xstrdup_printf ("HTTP/1.0 %d %s\r\n\r\n"
			 "<html><head><title>%d %s</title></head>"
			 "<body>%s</body></html>",
			 error_code, reason,
			 error_code, reason,
			 reason);
  xoutput_stream_write_all (out, res, strlen (res), NULL, NULL, NULL);
  g_free (res);
}

static xboolean_t
handler (xthreaded_socket_service_t *service,
	 xsocket_connection_t      *connection,
	 xsocket_listener_t        *listener,
	 xpointer_t                user_data)
{
  xoutput_stream_t *out;
  xinput_stream_t *in;
  xfile_input_stream_t *file_in;
  xdata_input_stream_t *data;
  char *line, *escaped, *tmp, *query, *unescaped, *path, *version;
  xfile_t *f;
  xerror_t *error;
  xfile_info_t *info;
  xstring_t *s;

  in = g_io_stream_get_input_stream (XIO_STREAM (connection));
  out = g_io_stream_get_output_stream (XIO_STREAM (connection));

  data = xdata_input_stream_new (in);
  /* Be tolerant of input */
  xdata_input_stream_set_newline_type (data, G_DATA_STREAM_NEWLINE_TYPE_ANY);

  line = xdata_input_stream_read_line (data, NULL, NULL, NULL);

  if (line == NULL)
    {
      send_error (out, 400, "Invalid request");
      goto out;
    }

  if (!xstr_has_prefix (line, "GET "))
    {
      send_error (out, 501, "Only GET implemented");
      goto out;
    }

  escaped = line + 4; /* Skip "GET " */

  version = NULL;
  tmp = strchr (escaped, ' ');
  if (tmp == NULL)
    {
      send_error (out, 400, "Bad Request");
      goto out;
    }
  *tmp = 0;

  version = tmp + 1;
  if (!xstr_has_prefix (version, "HTTP/1."))
    {
      send_error(out, 505, "HTTP Version Not Supported");
      goto out;
    }

  query = strchr (escaped, '?');
  if (query != NULL)
    *query++ = 0;

  unescaped = xuri_unescape_string (escaped, NULL);
  path = g_build_filename (root, unescaped, NULL);
  g_free (unescaped);
  f = xfile_new_for_path (path);
  g_free (path);

  error = NULL;
  file_in = xfile_read (f, NULL, &error);
  if (file_in == NULL)
    {
      send_error (out, 404, error->message);
      xerror_free (error);
      goto out;
    }

  s = xstring_new ("HTTP/1.0 200 OK\r\n");
  info = xfile_input_stream_query_info (file_in,
					 XFILE_ATTRIBUTE_STANDARD_SIZE ","
					 XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					 NULL, NULL);
  if (info)
    {
      const char *content_type;
      char *mime_type;

      if (xfile_info_has_attribute (info, XFILE_ATTRIBUTE_STANDARD_SIZE))
	xstring_append_printf (s, "Content-Length: %"G_GINT64_FORMAT"\r\n",
				xfile_info_get_size (info));
      content_type = xfile_info_get_content_type (info);
      if (content_type)
	{
	  mime_type = g_content_type_get_mime_type (content_type);
	  if (mime_type)
	    {
	      xstring_append_printf (s, "Content-Type: %s\r\n",
				      mime_type);
	      g_free (mime_type);
	    }
	}
    }
  xstring_append (s, "\r\n");

  if (xoutput_stream_write_all (out,
				 s->str, s->len,
				 NULL, NULL, NULL))
    {
      xoutput_stream_splice (out,
			      G_INPUT_STREAM (file_in),
			      0, NULL, NULL);
    }
  xstring_free (s, TRUE);

  xinput_stream_close (G_INPUT_STREAM (file_in), NULL, NULL);
  xobject_unref (file_in);

 out:
  xobject_unref (data);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  xsocket_service_t *service;
  xoption_context_t *context;
  xerror_t *error = NULL;

  context = g_option_context_new ("<http root dir> - Simple HTTP server");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (argc != 2)
    {
      g_printerr ("Root directory not specified\n");
      return 1;
    }

  root = xstrdup (argv[1]);

  service = xthreaded_socket_service_new (10);
  if (!xsocket_listener_add_inet_port (XSOCKET_LISTENER (service),
					port,
					NULL,
					&error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  g_print ("Http server listening on port %d\n", port);

  xsignal_connect (service, "run", G_CALLBACK (handler), NULL);

  xmain_loop_run (xmain_loop_new (NULL, FALSE));
  g_assert_not_reached ();
}
