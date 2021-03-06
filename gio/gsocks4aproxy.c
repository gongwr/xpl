 /* GIO - GLib Input, Output and Streaming Library
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
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"

#include "gsocks4aproxy.h"

#include <string.h>

#include "giomodule.h"
#include "giomodule-priv.h"
#include "giostream.h"
#include "ginetaddress.h"
#include "ginputstream.h"
#include "glibintl.h"
#include "goutputstream.h"
#include "gproxy.h"
#include "gproxyaddress.h"
#include "gtask.h"

#define SOCKS4_VERSION		  4

#define SOCKS4_CMD_CONNECT	  1
#define SOCKS4_CMD_BIND		  2

#define SOCKS4_MAX_LEN		  255

#define SOCKS4_REP_VERSION	  0
#define SOCKS4_REP_GRANTED	  90
#define SOCKS4_REP_REJECTED       91
#define SOCKS4_REP_NO_IDENT       92
#define SOCKS4_REP_BAD_IDENT      93

static void g_socks4a_proxy_iface_init (xproxy_interface_t *proxy_iface);

#define g_socks4a_proxy_get_type _g_socks4a_proxy_get_type
G_DEFINE_TYPE_WITH_CODE (GSocks4aProxy, g_socks4a_proxy, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY,
						g_socks4a_proxy_iface_init)
			 _xio_modules_ensure_extension_points_registered ();
			 g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "socks4a",
							 0))

static void
g_socks4a_proxy_finalize (xobject_t *object)
{
  /* must chain up */
  XOBJECT_CLASS (g_socks4a_proxy_parent_class)->finalize (object);
}

static void
g_socks4a_proxy_init (GSocks4aProxy *proxy)
{
  proxy->supports_hostname = TRUE;
}

/*                                                             |-> SOCKSv4a only
 * +----+----+----+----+----+----+----+----+----+----+....+----+------+....+------+
 * | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL| HOST |    | NULL |
 * +----+----+----+----+----+----+----+----+----+----+....+----+------+....+------+
 *    1    1      2              4           variable       1    variable
 */
#define SOCKS4_CONN_MSG_LEN	    (9 + SOCKS4_MAX_LEN * 2)
static xint_t
set_connect_msg (xuint8_t      *msg,
		 const xchar_t *hostname,
		 xuint16_t      port,
		 const char  *username,
		 xerror_t     **error)
{
  xinet_address_t *addr;
  xuint_t len = 0;
  xsize_t addr_len;
  xboolean_t is_ip;
  const xchar_t *ip;

  msg[len++] = SOCKS4_VERSION;
  msg[len++] = SOCKS4_CMD_CONNECT;

    {
      xuint16_t hp = g_htons (port);
      memcpy (msg + len, &hp, 2);
      len += 2;
    }

  is_ip = g_hostname_is_ip_address (hostname);

  if (is_ip)
    ip = hostname;
  else
    ip = "0.0.0.1";

  addr = xinet_address_new_from_string (ip);
  addr_len = xinet_address_get_native_size (addr);

  if (addr_len != 4)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
		  _("SOCKSv4 does not support IPv6 address ???%s???"),
		  ip);
      xobject_unref (addr);
      return -1;
    }

  memcpy (msg + len, xinet_address_to_bytes (addr), addr_len);
  len += addr_len;

  xobject_unref (addr);

  if (username)
    {
      xsize_t user_len = strlen (username);

      if (user_len > SOCKS4_MAX_LEN)
	{
	  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			       _("Username is too long for SOCKSv4 protocol"));
	  return -1;
	}

      memcpy (msg + len, username, user_len);
      len += user_len;
    }

  msg[len++] = '\0';

  if (!is_ip)
    {
      xsize_t host_len = strlen (hostname);

      if (host_len > SOCKS4_MAX_LEN)
	{
	  g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
		       _("Hostname ???%s??? is too long for SOCKSv4 protocol"),
		       hostname);
	  return -1;
	}

      memcpy (msg + len, hostname, host_len);
      len += host_len;
      msg[len++] = '\0';
    }

  return len;
}

/*
 * +----+----+----+----+----+----+----+----+
 * | VN | CD | DSTPORT |      DSTIP        |
 * +----+----+----+----+----+----+----+----+
 *    1    1      2              4
 */
#define SOCKS4_CONN_REP_LEN	  8
static xboolean_t
parse_connect_reply (const xuint8_t *data, xerror_t **error)
{
  if (data[0] != SOCKS4_REP_VERSION)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("The server is not a SOCKSv4 proxy server."));
      return FALSE;
    }

  if (data[1] != SOCKS4_REP_GRANTED)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("Connection through SOCKSv4 server was rejected"));
      return FALSE;
    }

  return TRUE;
}

static xio_stream_t *
g_socks4a_proxy_connect (xproxy_t            *proxy,
			 xio_stream_t         *io_stream,
			 xproxy_address_t     *proxy_address,
			 xcancellable_t      *cancellable,
			 xerror_t           **error)
{
  xinput_stream_t *in;
  xoutput_stream_t *out;
  const xchar_t *hostname;
  xuint16_t port;
  const xchar_t *username;

  hostname = xproxy_address_get_destination_hostname (proxy_address);
  port = xproxy_address_get_destination_port (proxy_address);
  username = xproxy_address_get_username (proxy_address);

  in = g_io_stream_get_input_stream (io_stream);
  out = g_io_stream_get_output_stream (io_stream);

  /* Send SOCKS4 connection request */
    {
      xuint8_t msg[SOCKS4_CONN_MSG_LEN];
      xint_t len;

      len = set_connect_msg (msg, hostname, port, username, error);

      if (len < 0)
	goto error;

      if (!xoutput_stream_write_all (out, msg, len, NULL,
				      cancellable, error))
	goto error;
    }

  /* Read SOCKS4 response */
    {
      xuint8_t data[SOCKS4_CONN_REP_LEN];

      if (!xinput_stream_read_all (in, data, SOCKS4_CONN_REP_LEN, NULL,
				    cancellable, error))
	goto error;

      if (!parse_connect_reply (data, error))
	goto error;
    }

  return xobject_ref (io_stream);

error:
  return NULL;
}


typedef struct
{
  xio_stream_t *io_stream;

  /* For connecting */
  xuint8_t *buffer;
  xssize_t length;
  xssize_t offset;

} ConnectAsyncData;

static void connect_msg_write_cb      (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);
static void connect_reply_read_cb     (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);

static void
free_connect_data (ConnectAsyncData *data)
{
  xobject_unref (data->io_stream);
  g_slice_free (ConnectAsyncData, data);
}

static void
do_read (xasync_ready_callback_t callback, xtask_t *task, ConnectAsyncData *data)
{
   xinput_stream_t *in;
   in = g_io_stream_get_input_stream (data->io_stream);
   xinput_stream_read_async (in,
			      data->buffer + data->offset,
			      data->length - data->offset,
			      xtask_get_priority (task),
			      xtask_get_cancellable (task),
			      callback, task);
}

static void
do_write (xasync_ready_callback_t callback, xtask_t *task, ConnectAsyncData *data)
{
  xoutput_stream_t *out;
  out = g_io_stream_get_output_stream (data->io_stream);
  xoutput_stream_write_async (out,
			       data->buffer + data->offset,
			       data->length - data->offset,
			       xtask_get_priority (task),
			       xtask_get_cancellable (task),
			       callback, task);
}



static void
g_socks4a_proxy_connect_async (xproxy_t               *proxy,
			       xio_stream_t            *io_stream,
			       xproxy_address_t        *proxy_address,
			       xcancellable_t         *cancellable,
			       xasync_ready_callback_t   callback,
			       xpointer_t              user_data)
{
  xerror_t *error = NULL;
  xtask_t *task;
  ConnectAsyncData *data;
  const xchar_t *hostname;
  xuint16_t port;
  const xchar_t *username;

  data = g_slice_new0 (ConnectAsyncData);
  data->io_stream = xobject_ref (io_stream);

  task = xtask_new (proxy, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_socks4a_proxy_connect_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t) free_connect_data);

  hostname = xproxy_address_get_destination_hostname (proxy_address);
  port = xproxy_address_get_destination_port (proxy_address);
  username = xproxy_address_get_username (proxy_address);

  data->buffer = g_malloc0 (SOCKS4_CONN_MSG_LEN);
  data->length = set_connect_msg (data->buffer,
				  hostname, port, username,
				  &error);
  data->offset = 0;

  if (data->length < 0)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
    }
  else
    {
      do_write (connect_msg_write_cb, task, data);
    }
}

static void
connect_msg_write_cb (xobject_t      *source,
		      xasync_result_t *result,
		      xpointer_t      user_data)
{
  xtask_t *task = user_data;
  ConnectAsyncData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t written;

  written = xoutput_stream_write_finish (G_OUTPUT_STREAM (source),
					  result, &error);

  if (written < 0)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  data->offset += written;

  if (data->offset == data->length)
    {
      g_free (data->buffer);

      data->buffer = g_malloc0 (SOCKS4_CONN_REP_LEN);
      data->length = SOCKS4_CONN_REP_LEN;
      data->offset = 0;

      do_read (connect_reply_read_cb, task, data);
    }
  else
    {
      do_write (connect_msg_write_cb, task, data);
    }
}

static void
connect_reply_read_cb (xobject_t       *source,
		       xasync_result_t  *result,
		       xpointer_t       user_data)
{
  xtask_t *task = user_data;
  ConnectAsyncData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t read;

  read = xinput_stream_read_finish (G_INPUT_STREAM (source),
				     result, &error);

  if (read < 0)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  data->offset += read;

  if (data->offset == data->length)
    {
      if (!parse_connect_reply (data->buffer, &error))
	{
	  xtask_return_error (task, error);
	  xobject_unref (task);
	  return;
	}
      else
	{
	  xtask_return_pointer (task, xobject_ref (data->io_stream), xobject_unref);
	  xobject_unref (task);
	  return;
	}
    }
  else
    {
      do_read (connect_reply_read_cb, task, data);
    }
}

static xio_stream_t *g_socks4a_proxy_connect_finish (xproxy_t       *proxy,
						  xasync_result_t *result,
						  xerror_t      **error);

static xio_stream_t *
g_socks4a_proxy_connect_finish (xproxy_t       *proxy,
			        xasync_result_t *result,
			        xerror_t      **error)
{
  return xtask_propagate_pointer (XTASK (result), error);
}

static xboolean_t
g_socks4a_proxy_supports_hostname (xproxy_t *proxy)
{
  return G_SOCKS4A_PROXY (proxy)->supports_hostname;
}

static void
g_socks4a_proxy_class_init (GSocks4aProxyClass *class)
{
  xobject_class_t *object_class;

  object_class = (xobject_class_t *) class;
  object_class->finalize = g_socks4a_proxy_finalize;
}

static void
g_socks4a_proxy_iface_init (xproxy_interface_t *proxy_iface)
{
  proxy_iface->connect  = g_socks4a_proxy_connect;
  proxy_iface->connect_async = g_socks4a_proxy_connect_async;
  proxy_iface->connect_finish = g_socks4a_proxy_connect_finish;
  proxy_iface->supports_hostname = g_socks4a_proxy_supports_hostname;
}
