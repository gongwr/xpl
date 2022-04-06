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

#include "gdtlsclientconnection.h"
#include "ginitable.h"
#include "gioenumtypes.h"
#include "gsocket.h"
#include "gsocketconnectable.h"
#include "gtlsbackend.h"
#include "gtlscertificate.h"
#include "glibintl.h"

/**
 * SECTION:gdtlsclientconnection
 * @short_description: DTLS client-side connection
 * @include: gio/gio.h
 *
 * #xdtls_client_connection_t is the client-side subclass of
 * #xdtls_connection_t, representing a client-side DTLS connection.
 *
 * Since: 2.48
 */

/**
 * xdtls_client_connection_t:
 *
 * Abstract base class for the backend-specific client connection
 * type.
 *
 * Since: 2.48
 */

G_DEFINE_INTERFACE (xdtls_client_connection, g_dtls_client_connection,
                    XTYPE_DTLS_CONNECTION)

static void
g_dtls_client_connection_default_init (GDtlsClientConnectionInterface *iface)
{
  /**
   * xdtls_client_connection_t:validation-flags:
   *
   * What steps to perform when validating a certificate received from
   * a server. Server certificates that fail to validate in any of the
   * ways indicated here will be rejected unless the application
   * overrides the default via #xdtls_connection_t::accept-certificate.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_flags ("validation-flags",
                                                           P_("Validation flags"),
                                                           P_("What certificate validation to perform"),
                                                           XTYPE_TLS_CERTIFICATE_FLAGS,
                                                           G_TLS_CERTIFICATE_VALIDATE_ALL,
                                                           G_PARAM_READWRITE |
                                                           G_PARAM_CONSTRUCT |
                                                           G_PARAM_STATIC_STRINGS));

  /**
   * xdtls_client_connection_t:server-identity:
   *
   * A #xsocket_connectable_t describing the identity of the server that
   * is expected on the other end of the connection.
   *
   * If the %G_TLS_CERTIFICATE_BAD_IDENTITY flag is set in
   * #xdtls_client_connection_t:validation-flags, this object will be used
   * to determine the expected identify of the remote end of the
   * connection; if #xdtls_client_connection_t:server-identity is not set,
   * or does not match the identity presented by the server, then the
   * %G_TLS_CERTIFICATE_BAD_IDENTITY validation will fail.
   *
   * In addition to its use in verifying the server certificate,
   * this is also used to give a hint to the server about what
   * certificate we expect, which is useful for servers that serve
   * virtual hosts.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_object ("server-identity",
                                                            P_("Server identity"),
                                                            P_("xsocket_connectable_t identifying the server"),
                                                            XTYPE_SOCKET_CONNECTABLE,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT |
                                                            G_PARAM_STATIC_STRINGS));

  /**
   * xdtls_client_connection_t:accepted-cas: (type GLib.List) (element-type GLib.ByteArray)
   *
   * A list of the distinguished names of the Certificate Authorities
   * that the server will accept client certificates signed by. If the
   * server requests a client certificate during the handshake, then
   * this property will be set after the handshake completes.
   *
   * Each item in the list is a #xbyte_array_t which contains the complete
   * subject DN of the certificate authority.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_pointer ("accepted-cas",
                                                             P_("Accepted CAs"),
                                                             P_("Distinguished names of the CAs the server accepts certificates from"),
                                                             G_PARAM_READABLE |
                                                             G_PARAM_STATIC_STRINGS));
}

/**
 * g_dtls_client_connection_new:
 * @base_socket: the #xdatagram_based_t to wrap
 * @server_identity: (nullable): the expected identity of the server
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Creates a new #xdtls_client_connection_t wrapping @base_socket which is
 * assumed to communicate with the server identified by @server_identity.
 *
 * Returns: (transfer full) (type xdtls_client_connection_t): the new
 *   #xdtls_client_connection_t, or %NULL on error
 *
 * Since: 2.48
 */
xdatagram_based_t *
g_dtls_client_connection_new (xdatagram_based_t      *base_socket,
                              xsocket_connectable_t  *server_identity,
                              xerror_t             **error)
{
  xobject_t *conn;
  xtls_backend_t *backend;

  backend = xtls_backend_get_default ();
  conn = xinitable_new (xtls_backend_get_dtls_client_connection_type (backend),
                         NULL, error,
                         "base-socket", base_socket,
                         "server-identity", server_identity,
                         NULL);
  return G_DATAGRAM_BASED (conn);
}

/**
 * g_dtls_client_connection_get_validation_flags:
 * @conn: the #xdtls_client_connection_t
 *
 * Gets @conn's validation flags
 *
 * Returns: the validation flags
 *
 * Since: 2.48
 */
GTlsCertificateFlags
g_dtls_client_connection_get_validation_flags (xdtls_client_connection_t *conn)
{
  GTlsCertificateFlags flags = 0;

  g_return_val_if_fail (X_IS_DTLS_CLIENT_CONNECTION (conn), 0);

  xobject_get (G_OBJECT (conn), "validation-flags", &flags, NULL);
  return flags;
}

/**
 * g_dtls_client_connection_set_validation_flags:
 * @conn: the #xdtls_client_connection_t
 * @flags: the #GTlsCertificateFlags to use
 *
 * Sets @conn's validation flags, to override the default set of
 * checks performed when validating a server certificate. By default,
 * %G_TLS_CERTIFICATE_VALIDATE_ALL is used.
 *
 * Since: 2.48
 */
void
g_dtls_client_connection_set_validation_flags (xdtls_client_connection_t  *conn,
                                               GTlsCertificateFlags   flags)
{
  g_return_if_fail (X_IS_DTLS_CLIENT_CONNECTION (conn));

  xobject_set (G_OBJECT (conn), "validation-flags", flags, NULL);
}

/**
 * g_dtls_client_connection_get_server_identity:
 * @conn: the #xdtls_client_connection_t
 *
 * Gets @conn's expected server identity
 *
 * Returns: (transfer none): a #xsocket_connectable_t describing the
 * expected server identity, or %NULL if the expected identity is not
 * known.
 *
 * Since: 2.48
 */
xsocket_connectable_t *
g_dtls_client_connection_get_server_identity (xdtls_client_connection_t *conn)
{
  xsocket_connectable_t *identity = NULL;

  g_return_val_if_fail (X_IS_DTLS_CLIENT_CONNECTION (conn), 0);

  xobject_get (G_OBJECT (conn), "server-identity", &identity, NULL);
  if (identity)
    xobject_unref (identity);
  return identity;
}

/**
 * g_dtls_client_connection_set_server_identity:
 * @conn: the #xdtls_client_connection_t
 * @identity: a #xsocket_connectable_t describing the expected server identity
 *
 * Sets @conn's expected server identity, which is used both to tell
 * servers on virtual hosts which certificate to present, and also
 * to let @conn know what name to look for in the certificate when
 * performing %G_TLS_CERTIFICATE_BAD_IDENTITY validation, if enabled.
 *
 * Since: 2.48
 */
void
g_dtls_client_connection_set_server_identity (xdtls_client_connection_t *conn,
                                              xsocket_connectable_t    *identity)
{
  g_return_if_fail (X_IS_DTLS_CLIENT_CONNECTION (conn));

  xobject_set (G_OBJECT (conn), "server-identity", identity, NULL);
}

/**
 * g_dtls_client_connection_get_accepted_cas:
 * @conn: the #xdtls_client_connection_t
 *
 * Gets the list of distinguished names of the Certificate Authorities
 * that the server will accept certificates from. This will be set
 * during the TLS handshake if the server requests a certificate.
 * Otherwise, it will be %NULL.
 *
 * Each item in the list is a #xbyte_array_t which contains the complete
 * subject DN of the certificate authority.
 *
 * Returns: (element-type xbyte_array_t) (transfer full): the list of
 * CA DNs. You should unref each element with xbyte_array_unref() and then
 * the free the list with xlist_free().
 *
 * Since: 2.48
 */
xlist_t *
g_dtls_client_connection_get_accepted_cas (xdtls_client_connection_t *conn)
{
  xlist_t *accepted_cas = NULL;

  g_return_val_if_fail (X_IS_DTLS_CLIENT_CONNECTION (conn), NULL);

  xobject_get (G_OBJECT (conn), "accepted-cas", &accepted_cas, NULL);
  return accepted_cas;
}
