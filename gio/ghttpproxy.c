/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora, Ltd.
 * Copyright (C) 2014 Red Hat, Inc.
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
 * Author:  Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 *          Marc-André Lureau <marcandre.lureau@redhat.com>
 */

#include "config.h"

#include "ghttpproxy.h"

#include <string.h>
#include <stdlib.h>

#include "giomodule.h"
#include "giomodule-priv.h"
#include "giostream.h"
#include "ginputstream.h"
#include "glibintl.h"
#include "goutputstream.h"
#include "gproxy.h"
#include "gproxyaddress.h"
#include "gsocketconnectable.h"
#include "gtask.h"
#include "gtlsclientconnection.h"
#include "gtlsconnection.h"


struct _GHttpProxy
{
  xobject_t parent;
};

struct _GHttpProxyClass
{
  xobject_class_t parent_class;
};

static void g_http_proxy_iface_init (xproxy_interface_t *proxy_iface);

#define g_http_proxy_get_type _g_http_proxy_get_type
G_DEFINE_TYPE_WITH_CODE (GHttpProxy, g_http_proxy, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_PROXY,
                                                g_http_proxy_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "http",
                                                         0))

static void
g_http_proxy_init (GHttpProxy *proxy)
{
}

static xchar_t *
create_request (xproxy_address_t  *proxy_address,
                xboolean_t       *has_cred,
                xerror_t        **error)
{
  const xchar_t *hostname;
  xint_t port;
  const xchar_t *username;
  const xchar_t *password;
  xstring_t *request;
  xchar_t *ascii_hostname;

  if (has_cred)
    *has_cred = FALSE;

  hostname = xproxy_address_get_destination_hostname (proxy_address);
  ascii_hostname = g_hostname_to_ascii (hostname);
  if (!ascii_hostname)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           _("Invalid hostname"));
      return NULL;
    }
  port = xproxy_address_get_destination_port (proxy_address);
  username = xproxy_address_get_username (proxy_address);
  password = xproxy_address_get_password (proxy_address);

  request = xstring_new (NULL);

  xstring_append_printf (request,
                          "CONNECT %s:%i HTTP/1.0\r\n"
                          "Host: %s:%i\r\n"
                          "Proxy-Connection: keep-alive\r\n"
                          "User-Agent: GLib/%i.%i\r\n",
                          ascii_hostname, port,
                          ascii_hostname, port,
                          XPL_MAJOR_VERSION, XPL_MINOR_VERSION);
  g_free (ascii_hostname);

  if (username != NULL && password != NULL)
    {
      xchar_t *cred;
      xchar_t *base64_cred;

      if (has_cred)
        *has_cred = TRUE;

      cred = xstrdup_printf ("%s:%s", username, password);
      base64_cred = g_base64_encode ((xuchar_t *) cred, strlen (cred));
      g_free (cred);
      xstring_append_printf (request,
                              "Proxy-Authorization: Basic %s\r\n",
                              base64_cred);
      g_free (base64_cred);
    }

  xstring_append (request, "\r\n");

  return xstring_free (request, FALSE);
}

static xboolean_t
check_reply (const xchar_t  *buffer,
             xboolean_t      has_cred,
             xerror_t      **error)
{
  xint_t err_code;
  const xchar_t *ptr = buffer + 7;

  if (strncmp (buffer, "HTTP/1.", 7) != 0 || (*ptr != '0' && *ptr != '1'))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
                           _("Bad HTTP proxy reply"));
      return FALSE;
    }

  ptr++;
  while (*ptr == ' ')
    ptr++;

  err_code = atoi (ptr);

  if (err_code < 200 || err_code >= 300)
    {
      switch (err_code)
        {
          case 403:
            g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_NOT_ALLOWED,
                                 _("HTTP proxy connection not allowed"));
            break;
          case 407:
            if (has_cred)
              g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_AUTH_FAILED,
                                   _("HTTP proxy authentication failed"));
            else
              g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_NEED_AUTH,
                                   _("HTTP proxy authentication required"));
            break;
          default:
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
                         _("HTTP proxy connection failed: %i"), err_code);
        }

      return FALSE;
    }

  return TRUE;
}

#define HTTP_END_MARKER "\r\n\r\n"

static xio_stream_t *
g_http_proxy_connect (xproxy_t         *proxy,
                      xio_stream_t      *io_stream,
                      xproxy_address_t  *proxy_address,
                      xcancellable_t   *cancellable,
                      xerror_t        **error)
{
  xinput_stream_t *in;
  xoutput_stream_t *out;
  xchar_t *buffer = NULL;
  xsize_t buffer_length;
  xsize_t bytes_read;
  xboolean_t has_cred;
  xio_stream_t *tlsconn = NULL;

  if (X_IS_HTTPS_PROXY (proxy))
    {
      tlsconn = xtls_client_connection_new (io_stream,
                                             XSOCKET_CONNECTABLE (proxy_address),
                                             error);
      if (!tlsconn)
        goto error;

#ifdef DEBUG
      {
        xtls_certificate_flags_t tls_validation_flags = G_TLS_CERTIFICATE_VALIDATE_ALL;

        tls_validation_flags &= ~(G_TLS_CERTIFICATE_UNKNOWN_CA | G_TLS_CERTIFICATE_BAD_IDENTITY);
        xtls_client_connection_set_validation_flags (G_TLS_CLIENT_CONNECTION (tlsconn),
                                                      tls_validation_flags);
      }
#endif

      if (!xtls_connection_handshake (G_TLS_CONNECTION (tlsconn), cancellable, error))
        goto error;

      io_stream = tlsconn;
    }

  in = g_io_stream_get_input_stream (io_stream);
  out = g_io_stream_get_output_stream (io_stream);

  buffer = create_request (proxy_address, &has_cred, error);
  if (!buffer)
    goto error;
  if (!xoutput_stream_write_all (out, buffer, strlen (buffer), NULL,
                                  cancellable, error))
    goto error;

  g_free (buffer);

  bytes_read = 0;
  buffer_length = 1024;
  buffer = g_malloc (buffer_length);

  /* Read byte-by-byte instead of using xdata_input_stream_t
   * since we do not want to read beyond the end marker
   */
  do
    {
      xssize_t signed_nread;
      xsize_t nread;

      signed_nread =
          xinput_stream_read (in, buffer + bytes_read, 1, cancellable, error);
      if (signed_nread == -1)
        goto error;

      nread = signed_nread;
      if (nread == 0)
        break;

      ++bytes_read;

      if (bytes_read == buffer_length)
        {
          /* HTTP specifications does not defines any upper limit for
           * headers. But, the most usual size used seems to be 8KB.
           * Yet, the biggest we found was Tomcat's HTTP headers whose
           * size is 48K. So, for a reasonable error margin, let's accept
           * a header with a twice as large size but no more: 96KB */
          if (buffer_length > 98304)
            {
              g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
                                   _("HTTP proxy response too big"));
              goto error;
            }
          buffer_length = 2 * buffer_length;
          buffer = g_realloc (buffer, buffer_length);
        }

      *(buffer + bytes_read) = '\0';

      if (xstr_has_suffix (buffer, HTTP_END_MARKER))
        break;
    }
  while (TRUE);

  if (bytes_read == 0)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
                           _("HTTP proxy server closed connection unexpectedly."));
      goto error;
    }

  if (!check_reply (buffer, has_cred, error))
    goto error;

  g_free (buffer);

  xobject_ref (io_stream);
  g_clear_object (&tlsconn);

  return io_stream;

error:
  g_clear_object (&tlsconn);
  g_free (buffer);
  return NULL;
}

typedef struct
{
  xio_stream_t *io_stream;
  xproxy_address_t *proxy_address;
} ConnectAsyncData;

static void
free_connect_data (ConnectAsyncData *data)
{
  xobject_unref (data->io_stream);
  xobject_unref (data->proxy_address);
  g_slice_free (ConnectAsyncData, data);
}

static void
connect_thread (xtask_t        *task,
                xpointer_t      source_object,
                xpointer_t      task_data,
                xcancellable_t *cancellable)
{
  xproxy_t *proxy = source_object;
  ConnectAsyncData *data = task_data;
  xio_stream_t *res;
  xerror_t *error = NULL;

  res = g_http_proxy_connect (proxy, data->io_stream, data->proxy_address,
                              cancellable, &error);

  if (res == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, res, xobject_unref);
}

static void
g_http_proxy_connect_async (xproxy_t              *proxy,
                            xio_stream_t           *io_stream,
                            xproxy_address_t       *proxy_address,
                            xcancellable_t        *cancellable,
                            xasync_ready_callback_t  callback,
                            xpointer_t             user_data)
{
  ConnectAsyncData *data;
  xtask_t *task;

  data = g_slice_new0 (ConnectAsyncData);
  data->io_stream = xobject_ref (io_stream);
  data->proxy_address = xobject_ref (proxy_address);

  task = xtask_new (proxy, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_http_proxy_connect_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t) free_connect_data);

  xtask_run_in_thread (task, connect_thread);
  xobject_unref (task);
}

static xio_stream_t *
g_http_proxy_connect_finish (xproxy_t        *proxy,
                             xasync_result_t  *result,
                             xerror_t       **error)
{
  return xtask_propagate_pointer (XTASK (result), error);
}

static xboolean_t
g_http_proxy_supports_hostname (xproxy_t *proxy)
{
  return TRUE;
}

static void
g_http_proxy_class_init (GHttpProxyClass *class)
{
}

static void
g_http_proxy_iface_init (xproxy_interface_t *proxy_iface)
{
  proxy_iface->connect = g_http_proxy_connect;
  proxy_iface->connect_async = g_http_proxy_connect_async;
  proxy_iface->connect_finish = g_http_proxy_connect_finish;
  proxy_iface->supports_hostname = g_http_proxy_supports_hostname;
}

struct _GHttpsProxy
{
  GHttpProxy parent;
};

struct _GHttpsProxyClass
{
  GHttpProxyClass parent_class;
};

#define g_https_proxy_get_type _g_https_proxy_get_type
G_DEFINE_TYPE_WITH_CODE (GHttpsProxy, g_https_proxy, XTYPE_HTTP_PROXY,
                         G_IMPLEMENT_INTERFACE (XTYPE_PROXY,
                                                g_http_proxy_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "https",
                                                         0))

static void
g_https_proxy_init (GHttpsProxy *proxy)
{
}

static void
g_https_proxy_class_init (GHttpsProxyClass *class)
{
}
