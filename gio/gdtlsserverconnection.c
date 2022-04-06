/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2010 Red Hat, Inc
 * Copyright © 2015 Collabora, Ltd.
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

#include "gdtlsserverconnection.h"
#include "ginitable.h"
#include "gioenumtypes.h"
#include "gsocket.h"
#include "gtlsbackend.h"
#include "gtlscertificate.h"
#include "glibintl.h"

/**
 * SECTION:gdtlsserverconnection
 * @short_description: DTLS server-side connection
 * @include: gio/gio.h
 *
 * #xdtls_server_connection_t is the server-side subclass of #xdtls_connection_t,
 * representing a server-side DTLS connection.
 *
 * Since: 2.48
 */

G_DEFINE_INTERFACE (xdtls_server_connection, g_dtls_server_connection,
                    XTYPE_DTLS_CONNECTION)

static void
g_dtls_server_connection_default_init (GDtlsServerConnectionInterface *iface)
{
  /**
   * xdtls_server_connection_t:authentication-mode:
   *
   * The #GTlsAuthenticationMode for the server. This can be changed
   * before calling g_dtls_connection_handshake() if you want to
   * rehandshake with a different mode from the initial handshake.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_enum ("authentication-mode",
                                                          P_("Authentication Mode"),
                                                          P_("The client authentication mode"),
                                                          XTYPE_TLS_AUTHENTICATION_MODE,
                                                          G_TLS_AUTHENTICATION_NONE,
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_STATIC_STRINGS));
}

/**
 * g_dtls_server_connection_new:
 * @base_socket: the #xdatagram_based_t to wrap
 * @certificate: (nullable): the default server certificate, or %NULL
 * @error: #xerror_t for error reporting, or %NULL to ignore
 *
 * Creates a new #xdtls_server_connection_t wrapping @base_socket.
 *
 * Returns: (transfer full) (type xdtls_server_connection_t): the new
 *   #xdtls_server_connection_t, or %NULL on error
 *
 * Since: 2.48
 */
xdatagram_based_t *
g_dtls_server_connection_new (xdatagram_based_t   *base_socket,
                              xtls_certificate_t  *certificate,
                              xerror_t          **error)
{
  xobject_t *conn;
  xtls_backend_t *backend;

  backend = xtls_backend_get_default ();
  conn = xinitable_new (xtls_backend_get_dtls_server_connection_type (backend),
                         NULL, error,
                         "base-socket", base_socket,
                         "certificate", certificate,
                         NULL);
  return G_DATAGRAM_BASED (conn);
}
