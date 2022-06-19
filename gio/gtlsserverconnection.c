/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2010 Red Hat, Inc
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
 */

#include "config.h"
#include "glib.h"

#include "gtlsserverconnection.h"
#include "ginitable.h"
#include "gioenumtypes.h"
#include "gsocket.h"
#include "gtlsbackend.h"
#include "gtlscertificate.h"
#include "glibintl.h"

/**
 * SECTION:gtlsserverconnection
 * @short_description: TLS server-side connection
 * @include: gio/gio.h
 *
 * #xtls_server_connection_t is the server-side subclass of #xtls_connection_t,
 * representing a server-side TLS connection.
 *
 * Since: 2.28
 */

G_DEFINE_INTERFACE (xtls_server_connection, xtls_server_connection, XTYPE_TLS_CONNECTION)

static void
xtls_server_connection_default_init (xtls_server_connection_interface_t *iface)
{
  /**
   * xtls_server_connection_t:authentication-mode:
   *
   * The #GTlsAuthenticationMode for the server. This can be changed
   * before calling xtls_connection_handshake() if you want to
   * rehandshake with a different mode from the initial handshake.
   *
   * Since: 2.28
   */
  xobject_interface_install_property (iface,
				       xparam_spec_enum ("authentication-mode",
							  P_("Authentication Mode"),
							  P_("The client authentication mode"),
							  XTYPE_TLS_AUTHENTICATION_MODE,
							  G_TLS_AUTHENTICATION_NONE,
							  XPARAM_READWRITE |
							  XPARAM_STATIC_STRINGS));
}

/**
 * xtls_server_connection_new:
 * @base_io_stream: the #xio_stream_t to wrap
 * @certificate: (nullable): the default server certificate, or %NULL
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Creates a new #xtls_server_connection_t wrapping @base_io_stream (which
 * must have pollable input and output streams).
 *
 * See the documentation for #xtls_connection_t:base-io-stream for restrictions
 * on when application code can run operations on the @base_io_stream after
 * this function has returned.
 *
 * Returns: (transfer full) (type xtls_server_connection_t): the new
 * #xtls_server_connection_t, or %NULL on error
 *
 * Since: 2.28
 */
xio_stream_t *
xtls_server_connection_new (xio_stream_t        *base_io_stream,
			     xtls_certificate_t  *certificate,
			     xerror_t          **error)
{
  xobject_t *conn;
  xtls_backend_t *backend;

  backend = xtls_backend_get_default ();
  conn = xinitable_new (xtls_backend_get_server_connection_type (backend),
			 NULL, error,
			 "base-io-stream", base_io_stream,
			 "certificate", certificate,
			 NULL);
  return XIO_STREAM (conn);
}
