 /* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008, 2010 Collabora, Ltd.
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
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
 * Author:  Youness Alaoui <youness.alaoui@collabora.co.uk
 *
 * Contributors:
 *	    Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"

#include "gsocks5proxy.h"

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

#define SOCKS5_VERSION		  0x05

#define SOCKS5_CMD_CONNECT	  0x01
#define SOCKS5_CMD_BIND		  0x02
#define SOCKS5_CMD_UDP_ASSOCIATE  0x03

#define SOCKS5_ATYP_IPV4	  0x01
#define SOCKS5_ATYP_DOMAINNAME	  0x03
#define SOCKS5_ATYP_IPV6	  0x04

#define SOCKS5_AUTH_VERSION	  0x01

#define SOCKS5_AUTH_NONE	  0x00
#define SOCKS5_AUTH_GSSAPI	  0x01
#define SOCKS5_AUTH_USR_PASS	  0x02
#define SOCKS5_AUTH_NO_ACCEPT	  0xff

#define SOCKS5_MAX_LEN		  255
#define SOCKS5_RESERVED		  0x00

#define SOCKS5_REP_SUCCEEDED	  0x00
#define SOCKS5_REP_SRV_FAILURE    0x01
#define SOCKS5_REP_NOT_ALLOWED    0x02
#define SOCKS5_REP_NET_UNREACH    0x03
#define SOCKS5_REP_HOST_UNREACH   0x04
#define SOCKS5_REP_REFUSED        0x05
#define SOCKS5_REP_TTL_EXPIRED    0x06
#define SOCKS5_REP_CMD_NOT_SUP    0x07
#define SOCKS5_REP_ATYPE_NOT_SUP  0x08


struct _GSocks5Proxy
{
  xobject_t parent;
};

struct _GSocks5ProxyClass
{
  xobject_class_t parent_class;
};

static void g_socks5_proxy_iface_init (xproxy_interface_t *proxy_iface);

#define g_socks5_proxy_get_type _g_socks5_proxy_get_type
G_DEFINE_TYPE_WITH_CODE (GSocks5Proxy, g_socks5_proxy, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY,
						g_socks5_proxy_iface_init)
			 _xio_modules_ensure_extension_points_registered ();
			 g_io_extension_point_implement (G_PROXY_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "socks5",
							 0))

static void
g_socks5_proxy_finalize (xobject_t *object)
{
  /* must chain up */
  G_OBJECT_CLASS (g_socks5_proxy_parent_class)->finalize (object);
}

static void
g_socks5_proxy_init (GSocks5Proxy *proxy)
{
}

/*
 * +----+----------+----------+
 * |VER | NMETHODS | METHODS  |
 * +----+----------+----------+
 * | 1  |    1     | 1 to 255 |
 * +----+----------+----------+
 */
#define SOCKS5_NEGO_MSG_LEN	  4
static xint_t
set_nego_msg (xuint8_t *msg, xboolean_t has_auth)
{
  xint_t len = 3;

  msg[0] = SOCKS5_VERSION;
  msg[1] = 0x01; /* number of methods supported */
  msg[2] = SOCKS5_AUTH_NONE;

  /* add support for authentication method */
  if (has_auth)
    {
      msg[1] = 0x02; /* number of methods supported */
      msg[3] = SOCKS5_AUTH_USR_PASS;
      len++;
    }

  return len;
}


/*
 * +----+--------+
 * |VER | METHOD |
 * +----+--------+
 * | 1  |   1    |
 * +----+--------+
 */
#define SOCKS5_NEGO_REP_LEN	  2
static xboolean_t
parse_nego_reply (const xuint8_t *data,
		  xboolean_t     has_auth,
		  xboolean_t    *must_auth,
		  xerror_t     **error)
{
  if (data[0] != SOCKS5_VERSION)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("The server is not a SOCKSv5 proxy server."));
      return FALSE;
    }

  switch (data[1])
    {
      case SOCKS5_AUTH_NONE:
	*must_auth = FALSE;
	break;

      case SOCKS5_AUTH_USR_PASS:
	if (!has_auth)
	  {
	    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_NEED_AUTH,
			   _("The SOCKSv5 proxy requires authentication."));
	    return FALSE;
	  }
	*must_auth = TRUE;
	break;

      case SOCKS5_AUTH_NO_ACCEPT:
        if (!has_auth)
          {
            /* The server has said it accepts none of our authentication methods,
             * but given the slightly odd implementation of set_nego_msg(), we
             * actually only gave it the choice of %SOCKS5_AUTH_NONE, since the
             * caller specified no username or password.
             * Return %G_IO_ERROR_PROXY_NEED_AUTH so the caller knows that if
             * they specify a username and password and try again, authentication
             * might succeed (since we’ll send %SOCKS5_AUTH_USR_PASS next time). */
            g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_NEED_AUTH,
                                 _("The SOCKSv5 proxy requires authentication."));
            return FALSE;
          }
        G_GNUC_FALLTHROUGH;
      case SOCKS5_AUTH_GSSAPI:
      default:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_AUTH_FAILED,
			     _("The SOCKSv5 proxy requires an authentication "
			       "method that is not supported by GLib."));
	return FALSE;
	break;
    }

  return TRUE;
}

#define SOCKS5_AUTH_MSG_LEN       515
static xint_t
set_auth_msg (xuint8_t	  *msg,
	      const xchar_t *username,
	      const xchar_t *password,
	      xerror_t **error)
{
  xint_t len = 0;
  xint_t ulen = 0; /* username length */
  xint_t plen = 0; /* Password length */

  if (username)
    ulen = strlen (username);

  if (password)
    plen = strlen (password);

  if (ulen > SOCKS5_MAX_LEN || plen > SOCKS5_MAX_LEN)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("Username or password is too long for SOCKSv5 "
			     "protocol."));
      return -1;
    }

  msg[len++] = SOCKS5_AUTH_VERSION;
  msg[len++] = ulen;

  if (ulen > 0)
    memcpy (msg + len, username, ulen);

  len += ulen;
  msg[len++] = plen;

  if (plen > 0)
    memcpy (msg + len, password, plen);

  len += plen;

  return len;
}


static xboolean_t
check_auth_status (const xuint8_t *data, xerror_t **error)
{
  if (data[0] != SOCKS5_AUTH_VERSION
      || data[1] != SOCKS5_REP_SUCCEEDED)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_AUTH_FAILED,
			   _("SOCKSv5 authentication failed due to wrong "
			     "username or password."));
      return FALSE;
    }
  return TRUE;
}

/*
 * +----+-----+-------+------+----------+----------+
 * |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
 * +----+-----+-------+------+----------+----------+
 * | 1  |  1  | X'00' |  1   | Variable |    2     |
 * +----+-----+-------+------+----------+----------+
 * DST.ADDR is a string with first byte being the size. So DST.ADDR may not be
 * longer then 256 bytes.
 */
#define SOCKS5_CONN_MSG_LEN	  262
static xint_t
set_connect_msg (xuint8_t       *msg,
		 const xchar_t *hostname,
		 xuint16_t      port,
		 xerror_t     **error)
{
  xuint_t len = 0;

  msg[len++] = SOCKS5_VERSION;
  msg[len++] = SOCKS5_CMD_CONNECT;
  msg[len++] = SOCKS5_RESERVED;

  if (g_hostname_is_ip_address (hostname))
    {
      xinet_address_t *addr = xinet_address_new_from_string (hostname);
      xsize_t addr_len = xinet_address_get_native_size (addr);

      /* We are cheating for simplicity, here's the logic:
       *   1 = IPV4 = 4 bytes / 4
       *   4 = IPV6 = 16 bytes / 4 */
      msg[len++] = addr_len / 4;
      memcpy (msg + len, xinet_address_to_bytes (addr), addr_len);
      len += addr_len;

      xobject_unref (addr);
    }
  else
    {
      xsize_t host_len = strlen (hostname);

      if (host_len > SOCKS5_MAX_LEN)
	{
	  g_set_error (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
		       _("Hostname “%s” is too long for SOCKSv5 protocol"),
		       hostname);
	  return -1;
	}

      msg[len++] = SOCKS5_ATYP_DOMAINNAME;
      msg[len++] = (xuint8_t) host_len;
      memcpy (msg + len, hostname, host_len);
      len += host_len;
    }

    {
      xuint16_t hp = g_htons (port);
      memcpy (msg + len, &hp, 2);
      len += 2;
    }

  return len;
}

/*
 * +----+-----+-------+------+----------+----------+
 * |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
 * +----+-----+-------+------+----------+----------+
 * | 1  |  1  | X'00' |  1   | Variable |    2     |
 * +----+-----+-------+------+----------+----------+
 * This reply need to be read by small part to determine size. Buffer
 * size is determined in function of the biggest part to read.
 *
 * The parser only requires 4 bytes.
 */
#define SOCKS5_CONN_REP_LEN	  257
static xboolean_t
parse_connect_reply (const xuint8_t *data, xint_t *atype, xerror_t **error)
{
  if (data[0] != SOCKS5_VERSION)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("The server is not a SOCKSv5 proxy server."));
      return FALSE;
    }

  switch (data[1])
    {
      case SOCKS5_REP_SUCCEEDED:
	if (data[2] != SOCKS5_RESERVED)
	  {
	    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("The server is not a SOCKSv5 proxy server."));
	    return FALSE;
	  }

	switch (data[3])
	  {
	  case SOCKS5_ATYP_IPV4:
	  case SOCKS5_ATYP_IPV6:
	  case SOCKS5_ATYP_DOMAINNAME:
	    *atype = data[3];
	    break;

	  default:
	    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			   _("The SOCKSv5 proxy server uses unknown address type."));
	    return FALSE;
	  }
	break;

      case SOCKS5_REP_SRV_FAILURE:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			     _("Internal SOCKSv5 proxy server error."));
	return FALSE;
	break;

      case SOCKS5_REP_NOT_ALLOWED:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_NOT_ALLOWED,
			     _("SOCKSv5 connection not allowed by ruleset."));
	return FALSE;
	break;

      case SOCKS5_REP_TTL_EXPIRED:
      case SOCKS5_REP_HOST_UNREACH:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE,
			     _("Host unreachable through SOCKSv5 server."));
	return FALSE;
	break;

      case SOCKS5_REP_NET_UNREACH:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NETWORK_UNREACHABLE,
			     _("Network unreachable through SOCKSv5 proxy."));
	return FALSE;
	break;

      case SOCKS5_REP_REFUSED:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_REFUSED,
			     _("Connection refused through SOCKSv5 proxy."));
	return FALSE;
	break;

      case SOCKS5_REP_CMD_NOT_SUP:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			     _("SOCKSv5 proxy does not support “connect” command."));
	return FALSE;
	break;

      case SOCKS5_REP_ATYPE_NOT_SUP:
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			     _("SOCKSv5 proxy does not support provided address type."));
	return FALSE;
	break;

      default: /* Unknown error */
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PROXY_FAILED,
			     _("Unknown SOCKSv5 proxy error."));
	return FALSE;
	break;
    }

  return TRUE;
}

static xio_stream_t *
g_socks5_proxy_connect (xproxy_t            *proxy,
			xio_stream_t         *io_stream,
			xproxy_address_t     *proxy_address,
			xcancellable_t      *cancellable,
			xerror_t          **error)
{
  xboolean_t has_auth;
  xinput_stream_t *in;
  xoutput_stream_t *out;
  const xchar_t *hostname;
  xuint16_t port;
  const xchar_t *username;
  const xchar_t *password;

  hostname = xproxy_address_get_destination_hostname (proxy_address);
  port = xproxy_address_get_destination_port (proxy_address);
  username = xproxy_address_get_username (proxy_address);
  password = xproxy_address_get_password (proxy_address);

  has_auth = username || password;

  in = g_io_stream_get_input_stream (io_stream);
  out = g_io_stream_get_output_stream (io_stream);

  /* Send SOCKS5 handshake */
    {
      xuint8_t msg[SOCKS5_NEGO_MSG_LEN];
      xint_t len;

      len = set_nego_msg (msg, has_auth);

      if (!xoutput_stream_write_all (out, msg, len, NULL,
				      cancellable, error))
	goto error;
    }

  /* Receive SOCKS5 response and reply with authentication if required */
    {
      xuint8_t data[SOCKS5_NEGO_REP_LEN];
      xboolean_t must_auth = FALSE;

      if (!xinput_stream_read_all (in, data, sizeof (data), NULL,
				    cancellable, error))
	goto error;

      if (!parse_nego_reply (data, has_auth, &must_auth, error))
	  goto error;

      if (must_auth)
	{
	  xuint8_t msg[SOCKS5_AUTH_MSG_LEN];
	  xint_t len;

	  len = set_auth_msg (msg, username, password, error);

	  if (len < 0)
	    goto error;

	  if (!xoutput_stream_write_all (out, msg, len, NULL,
					  cancellable, error))
	    goto error;

	  if (!xinput_stream_read_all (in, data, sizeof (data), NULL,
					cancellable, error))
	    goto error;

	  if (!check_auth_status (data, error))
	    goto error;
	}
    }

  /* Send SOCKS5 connection request */
    {
      xuint8_t msg[SOCKS5_CONN_MSG_LEN];
      xint_t len;

      len = set_connect_msg (msg, hostname, port, error);

      if (len < 0)
	goto error;

      if (!xoutput_stream_write_all (out, msg, len, NULL,
				      cancellable, error))
	goto error;
    }

  /* Read SOCKS5 response */
    {
      xuint8_t data[SOCKS5_CONN_REP_LEN];
      xint_t atype;

      if (!xinput_stream_read_all (in, data, 4 /* VER, REP, RSV, ATYP */, NULL,
				    cancellable, error))
	goto error;

      if (!parse_connect_reply (data, &atype, error))
	goto error;

      switch (atype)
	{
	  case SOCKS5_ATYP_IPV4:
	    if (!xinput_stream_read_all (in, data,
					  4 /* IPv4 length */ + 2 /* port */,
					  NULL, cancellable, error))
	      goto error;
	    break;

	  case SOCKS5_ATYP_IPV6:
	    if (!xinput_stream_read_all (in, data,
					  16 /* IPv6 length */ + 2 /* port */,
					  NULL, cancellable, error))
	      goto error;
	    break;

	  case SOCKS5_ATYP_DOMAINNAME:
	    if (!xinput_stream_read_all (in, data, 1 /* domain name length */,
					  NULL, cancellable, error))
	      goto error;
	    if (!xinput_stream_read_all (in, data,
					  data[0] /* domain name length */ + 2 /* port */,
					  NULL, cancellable, error))
	      goto error;
	    break;
	}
    }

  return xobject_ref (io_stream);

error:
  return NULL;
}


typedef struct
{
  xio_stream_t *io_stream;
  xchar_t *hostname;
  xuint16_t port;
  xchar_t *username;
  xchar_t *password;
  xuint8_t *buffer;
  xssize_t length;
  xssize_t offset;
} ConnectAsyncData;

static void nego_msg_write_cb	      (xobject_t          *source,
				       xasync_result_t     *res,
				       xpointer_t          user_data);
static void nego_reply_read_cb	      (xobject_t          *source,
				       xasync_result_t     *res,
				       xpointer_t          user_data);
static void auth_msg_write_cb	      (xobject_t          *source,
				       xasync_result_t     *res,
				       xpointer_t          user_data);
static void auth_reply_read_cb	      (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);
static void send_connect_msg	      (xtask_t            *task);
static void connect_msg_write_cb      (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);
static void connect_reply_read_cb     (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);
static void connect_addr_len_read_cb  (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);
static void connect_addr_read_cb      (xobject_t          *source,
				       xasync_result_t     *result,
				       xpointer_t          user_data);

static void
free_connect_data (ConnectAsyncData *data)
{
  xobject_unref (data->io_stream);

  g_free (data->hostname);
  g_free (data->username);
  g_free (data->password);
  g_free (data->buffer);

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
g_socks5_proxy_connect_async (xproxy_t               *proxy,
			      xio_stream_t            *io_stream,
			      xproxy_address_t        *proxy_address,
			      xcancellable_t         *cancellable,
			      xasync_ready_callback_t   callback,
			      xpointer_t              user_data)
{
  xtask_t *task;
  ConnectAsyncData *data;

  data = g_slice_new0 (ConnectAsyncData);
  data->io_stream = xobject_ref (io_stream);

  task = xtask_new (proxy, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_socks5_proxy_connect_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t) free_connect_data);

  xobject_get (G_OBJECT (proxy_address),
		"destination-hostname", &data->hostname,
		"destination-port", &data->port,
		"username", &data->username,
		"password", &data->password,
		NULL);

  data->buffer = g_malloc0 (SOCKS5_NEGO_MSG_LEN);
  data->length = set_nego_msg (data->buffer,
			       data->username || data->password);
  data->offset = 0;

  do_write (nego_msg_write_cb, task, data);
}


static void
nego_msg_write_cb (xobject_t      *source,
		   xasync_result_t *res,
		   xpointer_t      user_data)
{
  xtask_t *task = user_data;
  ConnectAsyncData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t written;

  written = xoutput_stream_write_finish (G_OUTPUT_STREAM (source),
					  res, &error);

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

      data->buffer = g_malloc0 (SOCKS5_NEGO_REP_LEN);
      data->length = SOCKS5_NEGO_REP_LEN;
      data->offset = 0;

      do_read (nego_reply_read_cb, task, data);
    }
  else
    {
      do_write (nego_msg_write_cb, task, data);
    }
}

static void
nego_reply_read_cb (xobject_t      *source,
		    xasync_result_t *res,
		    xpointer_t      user_data)
{
  xtask_t *task = user_data;
  ConnectAsyncData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t read;

  read = xinput_stream_read_finish (G_INPUT_STREAM (source),
				     res, &error);

  if (read < 0)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  if (read == 0)
    {
      xtask_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CONNECTION_CLOSED,
                               "Connection to SOCKSv5 proxy server lost");
      xobject_unref (task);
      return;
    }

  data->offset += read;

  if (data->offset == data->length)
    {
      xerror_t *error = NULL;
      xboolean_t must_auth = FALSE;
      xboolean_t has_auth = data->username || data->password;

      if (!parse_nego_reply (data->buffer, has_auth, &must_auth, &error))
	{
	  xtask_return_error (task, error);
	  xobject_unref (task);
	  return;
	}

      if (must_auth)
	{
	  g_free (data->buffer);

	  data->buffer = g_malloc0 (SOCKS5_AUTH_MSG_LEN);
	  data->length = set_auth_msg (data->buffer,
				       data->username,
				       data->password,
				       &error);
	  data->offset = 0;

	  if (data->length < 0)
	    {
	      xtask_return_error (task, error);
	      xobject_unref (task);
	      return;
	    }

	  do_write (auth_msg_write_cb, task, data);
	}
      else
	{
	  send_connect_msg (task);
	}
    }
  else
    {
      do_read (nego_reply_read_cb, task, data);
    }
}

static void
auth_msg_write_cb (xobject_t      *source,
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

      data->buffer = g_malloc0 (SOCKS5_NEGO_REP_LEN);
      data->length = SOCKS5_NEGO_REP_LEN;
      data->offset = 0;

      do_read (auth_reply_read_cb, task, data);
    }
  else
    {
      do_write (auth_msg_write_cb, task, data);
    }
}

static void
auth_reply_read_cb (xobject_t      *source,
		    xasync_result_t *result,
		    xpointer_t      user_data)
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

  if (read == 0)
    {
      xtask_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CONNECTION_CLOSED,
                               "Connection to SOCKSv5 proxy server lost");
      xobject_unref (task);
      return;
    }

  data->offset += read;

  if (data->offset == data->length)
    {
      if (!check_auth_status (data->buffer, &error))
	{
	  xtask_return_error (task, error);
	  xobject_unref (task);
	  return;
	}

      send_connect_msg (task);
    }
  else
    {
      do_read (auth_reply_read_cb, task, data);
    }
}

static void
send_connect_msg (xtask_t *task)
{
  ConnectAsyncData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;

  g_free (data->buffer);

  data->buffer = g_malloc0 (SOCKS5_CONN_MSG_LEN);
  data->length = set_connect_msg (data->buffer,
				  data->hostname,
				  data->port,
				  &error);
  data->offset = 0;

  if (data->length < 0)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  do_write (connect_msg_write_cb, task, data);
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

      data->buffer = g_malloc0 (SOCKS5_CONN_REP_LEN);
      data->length = 4;
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

  if (read == 0)
    {
      xtask_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CONNECTION_CLOSED,
                               "Connection to SOCKSv5 proxy server lost");
      xobject_unref (task);
      return;
    }

  data->offset += read;

  if (data->offset == data->length)
    {
      xint_t atype;

      if (!parse_connect_reply (data->buffer, &atype, &error))
	{
	  xtask_return_error (task, error);
	  xobject_unref (task);
	  return;
	}

      switch (atype)
	{
	  case SOCKS5_ATYP_IPV4:
	    data->length = 6;
	    data->offset = 0;
	    do_read (connect_addr_read_cb, task, data);
	    break;

	  case SOCKS5_ATYP_IPV6:
	    data->length = 18;
	    data->offset = 0;
	    do_read (connect_addr_read_cb, task, data);
	    break;

	  case SOCKS5_ATYP_DOMAINNAME:
	    data->length = 1;
	    data->offset = 0;
	    do_read (connect_addr_len_read_cb, task, data);
	    break;
	}
    }
  else
    {
      do_read (connect_reply_read_cb, task, data);
    }
}

static void
connect_addr_len_read_cb (xobject_t      *source,
			  xasync_result_t *result,
			  xpointer_t      user_data)
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

  if (read == 0)
    {
      xtask_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CONNECTION_CLOSED,
                               "Connection to SOCKSv5 proxy server lost");
      xobject_unref (task);
      return;
    }

  data->length = data->buffer[0] + 2;
  data->offset = 0;

  do_read (connect_addr_read_cb, task, data);
}

static void
connect_addr_read_cb (xobject_t      *source,
		      xasync_result_t *result,
		      xpointer_t      user_data)
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

  if (read == 0)
    {
      xtask_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CONNECTION_CLOSED,
                               "Connection to SOCKSv5 proxy server lost");
      xobject_unref (task);
      return;
    }

  data->offset += read;

  if (data->offset == data->length)
    {
      xtask_return_pointer (task, xobject_ref (data->io_stream), xobject_unref);
      xobject_unref (task);
      return;
    }
  else
    {
      do_read (connect_reply_read_cb, task, data);
    }
}

static xio_stream_t *
g_socks5_proxy_connect_finish (xproxy_t       *proxy,
			       xasync_result_t *result,
			       xerror_t      **error)
{
  return xtask_propagate_pointer (XTASK (result), error);
}

static xboolean_t
g_socks5_proxy_supports_hostname (xproxy_t *proxy)
{
  return TRUE;
}

static void
g_socks5_proxy_class_init (GSocks5ProxyClass *class)
{
  xobject_class_t *object_class;

  object_class = (xobject_class_t *) class;
  object_class->finalize = g_socks5_proxy_finalize;
}

static void
g_socks5_proxy_iface_init (xproxy_interface_t *proxy_iface)
{
  proxy_iface->connect  = g_socks5_proxy_connect;
  proxy_iface->connect_async = g_socks5_proxy_connect_async;
  proxy_iface->connect_finish = g_socks5_proxy_connect_finish;
  proxy_iface->supports_hostname = g_socks5_proxy_supports_hostname;
}
