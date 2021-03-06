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

#include "gdtlsconnection.h"
#include "gcancellable.h"
#include "gioenumtypes.h"
#include "gsocket.h"
#include "gtlsbackend.h"
#include "gtlscertificate.h"
#include "gtlsconnection.h"
#include "gdtlsclientconnection.h"
#include "gtlsdatabase.h"
#include "gtlsinteraction.h"
#include "glibintl.h"
#include "gmarshal-internal.h"

/**
 * SECTION:gdtlsconnection
 * @short_description: DTLS connection type
 * @include: gio/gio.h
 *
 * #xdtls_connection_t is the base DTLS connection class type, which wraps
 * a #xdatagram_based_t and provides DTLS encryption on top of it. Its
 * subclasses, #xdtls_client_connection_t and #xdtls_server_connection_t,
 * implement client-side and server-side DTLS, respectively.
 *
 * For TLS support, see #xtls_connection_t.
 *
 * As DTLS is datagram based, #xdtls_connection_t implements #xdatagram_based_t,
 * presenting a datagram-socket-like API for the encrypted connection. This
 * operates over a base datagram connection, which is also a #xdatagram_based_t
 * (#xdtls_connection_t:base-socket).
 *
 * To close a DTLS connection, use xdtls_connection_close().
 *
 * Neither #xdtls_server_connection_t or #xdtls_client_connection_t set the peer address
 * on their base #xdatagram_based_t if it is a #xsocket_t — it is up to the caller to
 * do that if they wish. If they do not, and xsocket_close() is called on the
 * base socket, the #xdtls_connection_t will not raise a %G_IO_ERROR_NOT_CONNECTED
 * error on further I/O.
 *
 * Since: 2.48
 */

/**
 * xdtls_connection_t:
 *
 * Abstract base class for the backend-specific #xdtls_client_connection_t
 * and #xdtls_server_connection_t types.
 *
 * Since: 2.48
 */

G_DEFINE_INTERFACE (xdtls_connection, xdtls_connection, XTYPE_DATAGRAM_BASED)

enum {
  ACCEPT_CERTIFICATE,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

enum {
  PROP_BASE_SOCKET = 1,
  PROP_REQUIRE_CLOSE_NOTIFY,
  PROP_REHANDSHAKE_MODE,
  PROP_DATABASE,
  PROP_INTERACTION,
  PROP_CERTIFICATE,
  PROP_PEER_CERTIFICATE,
  PROP_PEER_CERTIFICATE_ERRORS,
  PROP_PROTOCOL_VERSION,
  PROP_CIPHERSUITE_NAME,
};

static void
xdtls_connection_default_init (xdtls_connection_interface_t *iface)
{
  /**
   * xdtls_connection_t:base-socket:
   *
   * The #xdatagram_based_t that the connection wraps. Note that this may be any
   * implementation of #xdatagram_based_t, not just a #xsocket_t.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_object ("base-socket",
                                                            P_("Base Socket"),
                                                            P_("The xdatagram_based_t that the connection wraps"),
                                                            XTYPE_DATAGRAM_BASED,
                                                            XPARAM_READWRITE |
                                                            XPARAM_CONSTRUCT_ONLY |
                                                            XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:database: (nullable)
   *
   * The certificate database to use when verifying this TLS connection.
   * If no certificate database is set, then the default database will be
   * used. See xtls_backend_get_default_database().
   *
   * When using a non-default database, #xdtls_connection_t must fall back to using
   * the #xtls_database_t to perform certificate verification using
   * xtls_database_verify_chain(), which means certificate verification will
   * not be able to make use of TLS session context. This may be less secure.
   * For example, if you create your own #xtls_database_t that just wraps the
   * default #xtls_database_t, you might expect that you have not changed anything,
   * but this is not true because you may have altered the behavior of
   * #xdtls_connection_t by causing it to use xtls_database_verify_chain(). See the
   * documentation of xtls_database_verify_chain() for more details on specific
   * security checks that may not be performed. Accordingly, setting a
   * non-default database is discouraged except for specialty applications with
   * unusual security requirements.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_object ("database",
                                                            P_("Database"),
                                                            P_("Certificate database to use for looking up or verifying certificates"),
                                                            XTYPE_TLS_DATABASE,
                                                            XPARAM_READWRITE |
                                                            XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:interaction: (nullable)
   *
   * A #xtls_interaction_t object to be used when the connection or certificate
   * database need to interact with the user. This will be used to prompt the
   * user for passwords where necessary.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_object ("interaction",
                                                            P_("Interaction"),
                                                            P_("Optional object for user interaction"),
                                                            XTYPE_TLS_INTERACTION,
                                                            XPARAM_READWRITE |
                                                            XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:require-close-notify:
   *
   * Whether or not proper TLS close notification is required.
   * See xdtls_connection_set_require_close_notify().
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_boolean ("require-close-notify",
                                                             P_("Require close notify"),
                                                             P_("Whether to require proper TLS close notification"),
                                                             TRUE,
                                                             XPARAM_READWRITE |
                                                             XPARAM_CONSTRUCT |
                                                             XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:rehandshake-mode:
   *
   * The rehandshaking mode. See
   * xdtls_connection_set_rehandshake_mode().
   *
   * Since: 2.48
   *
   * Deprecated: 2.60: The rehandshake mode is ignored.
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_enum ("rehandshake-mode",
                                                          P_("Rehandshake mode"),
                                                          P_("When to allow rehandshaking"),
                                                          XTYPE_TLS_REHANDSHAKE_MODE,
                                                          G_TLS_REHANDSHAKE_NEVER,
                                                          XPARAM_READWRITE |
                                                          XPARAM_CONSTRUCT |
                                                          XPARAM_STATIC_STRINGS |
                                                          XPARAM_DEPRECATED));
  /**
   * xdtls_connection_t:certificate:
   *
   * The connection's certificate; see
   * xdtls_connection_set_certificate().
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_object ("certificate",
                                                            P_("Certificate"),
                                                            P_("The connection’s certificate"),
                                                            XTYPE_TLS_CERTIFICATE,
                                                            XPARAM_READWRITE |
                                                            XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:peer-certificate: (nullable)
   *
   * The connection's peer's certificate, after the TLS handshake has
   * completed or failed. Note in particular that this is not yet set
   * during the emission of #xdtls_connection_t::accept-certificate.
   *
   * (You can watch for a #xobject_t::notify signal on this property to
   * detect when a handshake has occurred.)
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_object ("peer-certificate",
                                                            P_("Peer Certificate"),
                                                            P_("The connection’s peer’s certificate"),
                                                            XTYPE_TLS_CERTIFICATE,
                                                            XPARAM_READABLE |
                                                            XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:peer-certificate-errors:
   *
   * The errors noticed while verifying
   * #xdtls_connection_t:peer-certificate. Normally this should be 0, but
   * it may not be if #xdtls_client_connection_t:validation-flags is not
   * %G_TLS_CERTIFICATE_VALIDATE_ALL, or if
   * #xdtls_connection_t::accept-certificate overrode the default
   * behavior.
   *
   * GLib guarantees that if certificate verification fails, at least
   * one error will be set, but it does not guarantee that all possible
   * errors will be set. Accordingly, you may not safely decide to
   * ignore any particular type of error. For example, it would be
   * incorrect to mask %G_TLS_CERTIFICATE_EXPIRED if you want to allow
   * expired certificates, because this could potentially be the only
   * error flag set even if other problems exist with the certificate.
   *
   * Since: 2.48
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_flags ("peer-certificate-errors",
                                                           P_("Peer Certificate Errors"),
                                                           P_("Errors found with the peer’s certificate"),
                                                           XTYPE_TLS_CERTIFICATE_FLAGS,
                                                           0,
                                                           XPARAM_READABLE |
                                                           XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:advertised-protocols: (nullable)
   *
   * The list of application-layer protocols that the connection
   * advertises that it is willing to speak. See
   * xdtls_connection_set_advertised_protocols().
   *
   * Since: 2.60
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_boxed ("advertised-protocols",
                                                           P_("Advertised Protocols"),
                                                           P_("Application-layer protocols available on this connection"),
                                                           XTYPE_STRV,
                                                           XPARAM_READWRITE |
                                                           XPARAM_STATIC_STRINGS));
  /**
   * xdtls_connection_t:negotiated-protocol:
   *
   * The application-layer protocol negotiated during the TLS
   * handshake. See xdtls_connection_get_negotiated_protocol().
   *
   * Since: 2.60
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_string ("negotiated-protocol",
                                                            P_("Negotiated Protocol"),
                                                            P_("Application-layer protocol negotiated for this connection"),
                                                            NULL,
                                                            XPARAM_READABLE |
                                                            XPARAM_STATIC_STRINGS));

  /**
   * xdtls_connection_t:protocol-version:
   *
   * The DTLS protocol version in use. See xdtls_connection_get_protocol_version().
   *
   * Since: 2.70
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_enum ("protocol-version",
                                                          P_("Protocol Version"),
                                                          P_("DTLS protocol version negotiated for this connection"),
                                                          XTYPE_TLS_PROTOCOL_VERSION,
                                                          G_TLS_PROTOCOL_VERSION_UNKNOWN,
                                                          XPARAM_READABLE |
                                                          XPARAM_STATIC_STRINGS));

  /**
   * xdtls_connection_t:ciphersuite-name: (nullable)
   *
   * The name of the DTLS ciphersuite in use. See xdtls_connection_get_ciphersuite_name().
   *
   * Since: 2.70
   */
  xobject_interface_install_property (iface,
                                       xparam_spec_string ("ciphersuite-name",
                                                            P_("Ciphersuite Name"),
                                                            P_("Name of ciphersuite negotiated for this connection"),
                                                            NULL,
                                                            XPARAM_READABLE |
                                                            XPARAM_STATIC_STRINGS));

  /**
   * xdtls_connection_t::accept-certificate:
   * @conn: a #xdtls_connection_t
   * @peer_cert: the peer's #xtls_certificate_t
   * @errors: the problems with @peer_cert.
   *
   * Emitted during the TLS handshake after the peer certificate has
   * been received. You can examine @peer_cert's certification path by
   * calling xtls_certificate_get_issuer() on it.
   *
   * For a client-side connection, @peer_cert is the server's
   * certificate, and the signal will only be emitted if the
   * certificate was not acceptable according to @conn's
   * #xdtls_client_connection_t:validation_flags. If you would like the
   * certificate to be accepted despite @errors, return %TRUE from the
   * signal handler. Otherwise, if no handler accepts the certificate,
   * the handshake will fail with %G_TLS_ERROR_BAD_CERTIFICATE.
   *
   * GLib guarantees that if certificate verification fails, this signal
   * will be emitted with at least one error will be set in @errors, but
   * it does not guarantee that all possible errors will be set.
   * Accordingly, you may not safely decide to ignore any particular
   * type of error. For example, it would be incorrect to ignore
   * %G_TLS_CERTIFICATE_EXPIRED if you want to allow expired
   * certificates, because this could potentially be the only error flag
   * set even if other problems exist with the certificate.
   *
   * For a server-side connection, @peer_cert is the certificate
   * presented by the client, if this was requested via the server's
   * #xdtls_server_connection_t:authentication_mode. On the server side,
   * the signal is always emitted when the client presents a
   * certificate, and the certificate will only be accepted if a
   * handler returns %TRUE.
   *
   * Note that if this signal is emitted as part of asynchronous I/O
   * in the main thread, then you should not attempt to interact with
   * the user before returning from the signal handler. If you want to
   * let the user decide whether or not to accept the certificate, you
   * would have to return %FALSE from the signal handler on the first
   * attempt, and then after the connection attempt returns a
   * %G_TLS_ERROR_BAD_CERTIFICATE, you can interact with the user, and
   * if the user decides to accept the certificate, remember that fact,
   * create a new connection, and return %TRUE from the signal handler
   * the next time.
   *
   * If you are doing I/O in another thread, you do not
   * need to worry about this, and can simply block in the signal
   * handler until the UI thread returns an answer.
   *
   * Returns: %TRUE to accept @peer_cert (which will also
   * immediately end the signal emission). %FALSE to allow the signal
   * emission to continue, which will cause the handshake to fail if
   * no one else overrides it.
   *
   * Since: 2.48
   */
  signals[ACCEPT_CERTIFICATE] =
    xsignal_new (I_("accept-certificate"),
                  XTYPE_DTLS_CONNECTION,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xdtls_connection_interface_t, accept_certificate),
                  xsignal_accumulator_true_handled, NULL,
                  _g_cclosure_marshal_BOOLEAN__OBJECT_FLAGS,
                  XTYPE_BOOLEAN, 2,
                  XTYPE_TLS_CERTIFICATE,
                  XTYPE_TLS_CERTIFICATE_FLAGS);
  xsignal_set_va_marshaller (signals[ACCEPT_CERTIFICATE],
                              XTYPE_FROM_INTERFACE (iface),
                              _g_cclosure_marshal_BOOLEAN__OBJECT_FLAGSv);
}

/**
 * xdtls_connection_set_database:
 * @conn: a #xdtls_connection_t
 * @database: (nullable): a #xtls_database_t
 *
 * Sets the certificate database that is used to verify peer certificates.
 * This is set to the default database by default. See
 * xtls_backend_get_default_database(). If set to %NULL, then
 * peer certificate validation will always set the
 * %G_TLS_CERTIFICATE_UNKNOWN_CA error (meaning
 * #xdtls_connection_t::accept-certificate will always be emitted on
 * client-side connections, unless that bit is not set in
 * #xdtls_client_connection_t:validation-flags).
 *
 * There are nonintuitive security implications when using a non-default
 * database. See #xdtls_connection_t:database for details.
 *
 * Since: 2.48
 */
void
xdtls_connection_set_database (xdtls_connection_t *conn,
                                xtls_database_t    *database)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));
  g_return_if_fail (database == NULL || X_IS_TLS_DATABASE (database));

  xobject_set (G_OBJECT (conn),
                "database", database,
                NULL);
}

/**
 * xdtls_connection_get_database:
 * @conn: a #xdtls_connection_t
 *
 * Gets the certificate database that @conn uses to verify
 * peer certificates. See xdtls_connection_set_database().
 *
 * Returns: (transfer none) (nullable): the certificate database that @conn uses or %NULL
 *
 * Since: 2.48
 */
xtls_database_t*
xdtls_connection_get_database (xdtls_connection_t *conn)
{
  xtls_database_t *database = NULL;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), NULL);

  xobject_get (G_OBJECT (conn),
                "database", &database,
                NULL);
  if (database)
    xobject_unref (database);
  return database;
}

/**
 * xdtls_connection_set_certificate:
 * @conn: a #xdtls_connection_t
 * @certificate: the certificate to use for @conn
 *
 * This sets the certificate that @conn will present to its peer
 * during the TLS handshake. For a #xdtls_server_connection_t, it is
 * mandatory to set this, and that will normally be done at construct
 * time.
 *
 * For a #xdtls_client_connection_t, this is optional. If a handshake fails
 * with %G_TLS_ERROR_CERTIFICATE_REQUIRED, that means that the server
 * requires a certificate, and if you try connecting again, you should
 * call this method first. You can call
 * xdtls_client_connection_get_accepted_cas() on the failed connection
 * to get a list of Certificate Authorities that the server will
 * accept certificates from.
 *
 * (It is also possible that a server will allow the connection with
 * or without a certificate; in that case, if you don't provide a
 * certificate, you can tell that the server requested one by the fact
 * that xdtls_client_connection_get_accepted_cas() will return
 * non-%NULL.)
 *
 * Since: 2.48
 */
void
xdtls_connection_set_certificate (xdtls_connection_t *conn,
                                   xtls_certificate_t *certificate)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));
  g_return_if_fail (X_IS_TLS_CERTIFICATE (certificate));

  xobject_set (G_OBJECT (conn), "certificate", certificate, NULL);
}

/**
 * xdtls_connection_get_certificate:
 * @conn: a #xdtls_connection_t
 *
 * Gets @conn's certificate, as set by
 * xdtls_connection_set_certificate().
 *
 * Returns: (transfer none) (nullable): @conn's certificate, or %NULL
 *
 * Since: 2.48
 */
xtls_certificate_t *
xdtls_connection_get_certificate (xdtls_connection_t *conn)
{
  xtls_certificate_t *certificate;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), NULL);

  xobject_get (G_OBJECT (conn), "certificate", &certificate, NULL);
  if (certificate)
    xobject_unref (certificate);

  return certificate;
}

/**
 * xdtls_connection_set_interaction:
 * @conn: a connection
 * @interaction: (nullable): an interaction object, or %NULL
 *
 * Set the object that will be used to interact with the user. It will be used
 * for things like prompting the user for passwords.
 *
 * The @interaction argument will normally be a derived subclass of
 * #xtls_interaction_t. %NULL can also be provided if no user interaction
 * should occur for this connection.
 *
 * Since: 2.48
 */
void
xdtls_connection_set_interaction (xdtls_connection_t *conn,
                                   xtls_interaction_t *interaction)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));
  g_return_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction));

  xobject_set (G_OBJECT (conn), "interaction", interaction, NULL);
}

/**
 * xdtls_connection_get_interaction:
 * @conn: a connection
 *
 * Get the object that will be used to interact with the user. It will be used
 * for things like prompting the user for passwords. If %NULL is returned, then
 * no user interaction will occur for this connection.
 *
 * Returns: (transfer none) (nullable): The interaction object.
 *
 * Since: 2.48
 */
xtls_interaction_t *
xdtls_connection_get_interaction (xdtls_connection_t       *conn)
{
  xtls_interaction_t *interaction = NULL;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), NULL);

  xobject_get (G_OBJECT (conn), "interaction", &interaction, NULL);
  if (interaction)
    xobject_unref (interaction);

  return interaction;
}

/**
 * xdtls_connection_get_peer_certificate:
 * @conn: a #xdtls_connection_t
 *
 * Gets @conn's peer's certificate after the handshake has completed
 * or failed. (It is not set during the emission of
 * #xdtls_connection_t::accept-certificate.)
 *
 * Returns: (transfer none) (nullable): @conn's peer's certificate, or %NULL
 *
 * Since: 2.48
 */
xtls_certificate_t *
xdtls_connection_get_peer_certificate (xdtls_connection_t *conn)
{
  xtls_certificate_t *peer_certificate;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), NULL);

  xobject_get (G_OBJECT (conn), "peer-certificate", &peer_certificate, NULL);
  if (peer_certificate)
    xobject_unref (peer_certificate);

  return peer_certificate;
}

/**
 * xdtls_connection_get_peer_certificate_errors:
 * @conn: a #xdtls_connection_t
 *
 * Gets the errors associated with validating @conn's peer's
 * certificate, after the handshake has completed or failed. (It is
 * not set during the emission of #xdtls_connection_t::accept-certificate.)
 *
 * Returns: @conn's peer's certificate errors
 *
 * Since: 2.48
 */
xtls_certificate_flags_t
xdtls_connection_get_peer_certificate_errors (xdtls_connection_t *conn)
{
  xtls_certificate_flags_t errors;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), 0);

  xobject_get (G_OBJECT (conn), "peer-certificate-errors", &errors, NULL);
  return errors;
}

/**
 * xdtls_connection_set_require_close_notify:
 * @conn: a #xdtls_connection_t
 * @require_close_notify: whether or not to require close notification
 *
 * Sets whether or not @conn expects a proper TLS close notification
 * before the connection is closed. If this is %TRUE (the default),
 * then @conn will expect to receive a TLS close notification from its
 * peer before the connection is closed, and will return a
 * %G_TLS_ERROR_EOF error if the connection is closed without proper
 * notification (since this may indicate a network error, or
 * man-in-the-middle attack).
 *
 * In some protocols, the application will know whether or not the
 * connection was closed cleanly based on application-level data
 * (because the application-level data includes a length field, or is
 * somehow self-delimiting); in this case, the close notify is
 * redundant and may be omitted. You
 * can use xdtls_connection_set_require_close_notify() to tell @conn
 * to allow an "unannounced" connection close, in which case the close
 * will show up as a 0-length read, as in a non-TLS
 * #xdatagram_based_t, and it is up to the application to check that
 * the data has been fully received.
 *
 * Note that this only affects the behavior when the peer closes the
 * connection; when the application calls xdtls_connection_close_async() on
 * @conn itself, this will send a close notification regardless of the
 * setting of this property. If you explicitly want to do an unclean
 * close, you can close @conn's #xdtls_connection_t:base-socket rather
 * than closing @conn itself.
 *
 * Since: 2.48
 */
void
xdtls_connection_set_require_close_notify (xdtls_connection_t *conn,
                                            xboolean_t         require_close_notify)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));

  xobject_set (G_OBJECT (conn),
                "require-close-notify", require_close_notify,
                NULL);
}

/**
 * xdtls_connection_get_require_close_notify:
 * @conn: a #xdtls_connection_t
 *
 * Tests whether or not @conn expects a proper TLS close notification
 * when the connection is closed. See
 * xdtls_connection_set_require_close_notify() for details.
 *
 * Returns: %TRUE if @conn requires a proper TLS close notification.
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_get_require_close_notify (xdtls_connection_t *conn)
{
  xboolean_t require_close_notify;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), TRUE);

  xobject_get (G_OBJECT (conn),
                "require-close-notify", &require_close_notify,
                NULL);
  return require_close_notify;
}

/**
 * xdtls_connection_set_rehandshake_mode:
 * @conn: a #xdtls_connection_t
 * @mode: the rehandshaking mode
 *
 * Since GLib 2.64, changing the rehandshake mode is no longer supported
 * and will have no effect. With TLS 1.3, rehandshaking has been removed from
 * the TLS protocol, replaced by separate post-handshake authentication and
 * rekey operations.
 *
 * Since: 2.48
 *
 * Deprecated: 2.60. Changing the rehandshake mode is no longer
 *   required for compatibility. Also, rehandshaking has been removed
 *   from the TLS protocol in TLS 1.3.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
xdtls_connection_set_rehandshake_mode (xdtls_connection_t     *conn,
                                        GTlsRehandshakeMode  mode)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));

  xobject_set (G_OBJECT (conn),
                "rehandshake-mode", G_TLS_REHANDSHAKE_SAFELY,
                NULL);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xdtls_connection_get_rehandshake_mode:
 * @conn: a #xdtls_connection_t
 *
 * Gets @conn rehandshaking mode. See
 * xdtls_connection_set_rehandshake_mode() for details.
 *
 * Returns: %G_TLS_REHANDSHAKE_SAFELY
 *
 * Since: 2.48
 *
 * Deprecated: 2.64. Changing the rehandshake mode is no longer
 *   required for compatibility. Also, rehandshaking has been removed
 *   from the TLS protocol in TLS 1.3.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
GTlsRehandshakeMode
xdtls_connection_get_rehandshake_mode (xdtls_connection_t *conn)
{
  GTlsRehandshakeMode mode;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), G_TLS_REHANDSHAKE_SAFELY);

  /* Continue to call xobject_get(), even though the return value is
   * ignored, so that behavior doesn’t change for derived classes.
   */
  xobject_get (G_OBJECT (conn),
                "rehandshake-mode", &mode,
                NULL);
  return G_TLS_REHANDSHAKE_SAFELY;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xdtls_connection_handshake:
 * @conn: a #xdtls_connection_t
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a #xerror_t, or %NULL
 *
 * Attempts a TLS handshake on @conn.
 *
 * On the client side, it is never necessary to call this method;
 * although the connection needs to perform a handshake after
 * connecting, #xdtls_connection_t will handle this for you automatically
 * when you try to send or receive data on the connection. You can call
 * xdtls_connection_handshake() manually if you want to know whether
 * the initial handshake succeeded or failed (as opposed to just
 * immediately trying to use @conn to read or write, in which case,
 * if it fails, it may not be possible to tell if it failed before
 * or after completing the handshake), but beware that servers may reject
 * client authentication after the handshake has completed, so a
 * successful handshake does not indicate the connection will be usable.
 *
 * Likewise, on the server side, although a handshake is necessary at
 * the beginning of the communication, you do not need to call this
 * function explicitly unless you want clearer error reporting.
 *
 * Previously, calling xdtls_connection_handshake() after the initial
 * handshake would trigger a rehandshake; however, this usage was
 * deprecated in GLib 2.60 because rehandshaking was removed from the
 * TLS protocol in TLS 1.3. Since GLib 2.64, calling this function after
 * the initial handshake will no longer do anything.
 *
 * #xdtls_connection_t::accept_certificate may be emitted during the
 * handshake.
 *
 * Returns: success or failure
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_handshake (xdtls_connection_t  *conn,
                             xcancellable_t     *cancellable,
                             xerror_t          **error)
{
  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);

  return XDTLS_CONNECTION_GET_INTERFACE (conn)->handshake (conn, cancellable,
                                                            error);
}

/**
 * xdtls_connection_handshake_async:
 * @conn: a #xdtls_connection_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the handshake is complete
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously performs a TLS handshake on @conn. See
 * xdtls_connection_handshake() for more information.
 *
 * Since: 2.48
 */
void
xdtls_connection_handshake_async (xdtls_connection_t      *conn,
                                   int                   io_priority,
                                   xcancellable_t         *cancellable,
                                   xasync_ready_callback_t   callback,
                                   xpointer_t              user_data)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));

  XDTLS_CONNECTION_GET_INTERFACE (conn)->handshake_async (conn, io_priority,
                                                           cancellable,
                                                           callback, user_data);
}

/**
 * xdtls_connection_handshake_finish:
 * @conn: a #xdtls_connection_t
 * @result: a #xasync_result_t.
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous TLS handshake operation. See
 * xdtls_connection_handshake() for more information.
 *
 * Returns: %TRUE on success, %FALSE on failure, in which
 * case @error will be set.
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_handshake_finish (xdtls_connection_t  *conn,
                                    xasync_result_t     *result,
                                    xerror_t          **error)
{
  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);

  return XDTLS_CONNECTION_GET_INTERFACE (conn)->handshake_finish (conn,
                                                                   result,
                                                                   error);
}

/**
 * xdtls_connection_shutdown:
 * @conn: a #xdtls_connection_t
 * @shutdown_read: %TRUE to stop reception of incoming datagrams
 * @shutdown_write: %TRUE to stop sending outgoing datagrams
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a #xerror_t, or %NULL
 *
 * Shut down part or all of a DTLS connection.
 *
 * If @shutdown_read is %TRUE then the receiving side of the connection is shut
 * down, and further reading is disallowed. Subsequent calls to
 * g_datagram_based_receive_messages() will return %G_IO_ERROR_CLOSED.
 *
 * If @shutdown_write is %TRUE then the sending side of the connection is shut
 * down, and further writing is disallowed. Subsequent calls to
 * g_datagram_based_send_messages() will return %G_IO_ERROR_CLOSED.
 *
 * It is allowed for both @shutdown_read and @shutdown_write to be TRUE — this
 * is equivalent to calling xdtls_connection_close().
 *
 * If @cancellable is cancelled, the #xdtls_connection_t may be left
 * partially-closed and any pending untransmitted data may be lost. Call
 * xdtls_connection_shutdown() again to complete closing the #xdtls_connection_t.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_shutdown (xdtls_connection_t  *conn,
                            xboolean_t          shutdown_read,
                            xboolean_t          shutdown_write,
                            xcancellable_t     *cancellable,
                            xerror_t          **error)
{
  xdtls_connection_interface_t *iface;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);
  xreturn_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable),
                        FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!shutdown_read && !shutdown_write)
    return TRUE;

  iface = XDTLS_CONNECTION_GET_INTERFACE (conn);
  xassert (iface->shutdown != NULL);

  return iface->shutdown (conn, shutdown_read, shutdown_write,
                          cancellable, error);
}

/**
 * xdtls_connection_shutdown_async:
 * @conn: a #xdtls_connection_t
 * @shutdown_read: %TRUE to stop reception of incoming datagrams
 * @shutdown_write: %TRUE to stop sending outgoing datagrams
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the shutdown operation is complete
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously shut down part or all of the DTLS connection. See
 * xdtls_connection_shutdown() for more information.
 *
 * Since: 2.48
 */
void
xdtls_connection_shutdown_async (xdtls_connection_t      *conn,
                                  xboolean_t              shutdown_read,
                                  xboolean_t              shutdown_write,
                                  int                   io_priority,
                                  xcancellable_t         *cancellable,
                                  xasync_ready_callback_t   callback,
                                  xpointer_t              user_data)
{
  xdtls_connection_interface_t *iface;

  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  iface = XDTLS_CONNECTION_GET_INTERFACE (conn);
  xassert (iface->shutdown_async != NULL);

  iface->shutdown_async (conn, TRUE, TRUE, io_priority, cancellable,
                         callback, user_data);
}

/**
 * xdtls_connection_shutdown_finish:
 * @conn: a #xdtls_connection_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous TLS shutdown operation. See
 * xdtls_connection_shutdown() for more information.
 *
 * Returns: %TRUE on success, %FALSE on failure, in which
 * case @error will be set
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_shutdown_finish (xdtls_connection_t  *conn,
                                   xasync_result_t     *result,
                                   xerror_t          **error)
{
  xdtls_connection_interface_t *iface;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  iface = XDTLS_CONNECTION_GET_INTERFACE (conn);
  xassert (iface->shutdown_finish != NULL);

  return iface->shutdown_finish (conn, result, error);
}

/**
 * xdtls_connection_close:
 * @conn: a #xdtls_connection_t
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a #xerror_t, or %NULL
 *
 * Close the DTLS connection. This is equivalent to calling
 * xdtls_connection_shutdown() to shut down both sides of the connection.
 *
 * Closing a #xdtls_connection_t waits for all buffered but untransmitted data to
 * be sent before it completes. It then sends a `close_notify` DTLS alert to the
 * peer and may wait for a `close_notify` to be received from the peer. It does
 * not close the underlying #xdtls_connection_t:base-socket; that must be closed
 * separately.
 *
 * Once @conn is closed, all other operations will return %G_IO_ERROR_CLOSED.
 * Closing a #xdtls_connection_t multiple times will not return an error.
 *
 * #GDtlsConnections will be automatically closed when the last reference is
 * dropped, but you might want to call this function to make sure resources are
 * released as early as possible.
 *
 * If @cancellable is cancelled, the #xdtls_connection_t may be left
 * partially-closed and any pending untransmitted data may be lost. Call
 * xdtls_connection_close() again to complete closing the #xdtls_connection_t.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_close (xdtls_connection_t  *conn,
                         xcancellable_t     *cancellable,
                         xerror_t          **error)
{
  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);
  xreturn_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable),
                        FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  return XDTLS_CONNECTION_GET_INTERFACE (conn)->shutdown (conn, TRUE, TRUE,
                                                           cancellable, error);
}

/**
 * xdtls_connection_close_async:
 * @conn: a #xdtls_connection_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the close operation is complete
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously close the DTLS connection. See xdtls_connection_close() for
 * more information.
 *
 * Since: 2.48
 */
void
xdtls_connection_close_async (xdtls_connection_t      *conn,
                               int                   io_priority,
                               xcancellable_t         *cancellable,
                               xasync_ready_callback_t   callback,
                               xpointer_t              user_data)
{
  g_return_if_fail (X_IS_DTLS_CONNECTION (conn));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  XDTLS_CONNECTION_GET_INTERFACE (conn)->shutdown_async (conn, TRUE, TRUE,
                                                          io_priority,
                                                          cancellable,
                                                          callback, user_data);
}

/**
 * xdtls_connection_close_finish:
 * @conn: a #xdtls_connection_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous TLS close operation. See xdtls_connection_close()
 * for more information.
 *
 * Returns: %TRUE on success, %FALSE on failure, in which
 * case @error will be set
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_close_finish (xdtls_connection_t  *conn,
                                xasync_result_t     *result,
                                xerror_t          **error)
{
  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  return XDTLS_CONNECTION_GET_INTERFACE (conn)->shutdown_finish (conn, result,
                                                                  error);
}

/**
 * xdtls_connection_emit_accept_certificate:
 * @conn: a #xdtls_connection_t
 * @peer_cert: the peer's #xtls_certificate_t
 * @errors: the problems with @peer_cert
 *
 * Used by #xdtls_connection_t implementations to emit the
 * #xdtls_connection_t::accept-certificate signal.
 *
 * Returns: %TRUE if one of the signal handlers has returned
 *     %TRUE to accept @peer_cert
 *
 * Since: 2.48
 */
xboolean_t
xdtls_connection_emit_accept_certificate (xdtls_connection_t      *conn,
                                           xtls_certificate_t      *peer_cert,
                                           xtls_certificate_flags_t  errors)
{
  xboolean_t accept = FALSE;

  xsignal_emit (conn, signals[ACCEPT_CERTIFICATE], 0,
                 peer_cert, errors, &accept);
  return accept;
}

/**
 * xdtls_connection_set_advertised_protocols:
 * @conn: a #xdtls_connection_t
 * @protocols: (array zero-terminated=1) (nullable): a %NULL-terminated
 *   array of ALPN protocol names (eg, "http/1.1", "h2"), or %NULL
 *
 * Sets the list of application-layer protocols to advertise that the
 * caller is willing to speak on this connection. The
 * Application-Layer Protocol Negotiation (ALPN) extension will be
 * used to negotiate a compatible protocol with the peer; use
 * xdtls_connection_get_negotiated_protocol() to find the negotiated
 * protocol after the handshake.  Specifying %NULL for the the value
 * of @protocols will disable ALPN negotiation.
 *
 * See [IANA TLS ALPN Protocol IDs](https://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml#alpn-protocol-ids)
 * for a list of registered protocol IDs.
 *
 * Since: 2.60
 */
void
xdtls_connection_set_advertised_protocols (xdtls_connection_t     *conn,
                                            const xchar_t * const *protocols)
{
  xdtls_connection_interface_t *iface;

  iface = XDTLS_CONNECTION_GET_INTERFACE (conn);
  if (iface->set_advertised_protocols == NULL)
    return;

  iface->set_advertised_protocols (conn, protocols);
}

/**
 * xdtls_connection_get_negotiated_protocol:
 * @conn: a #xdtls_connection_t
 *
 * Gets the name of the application-layer protocol negotiated during
 * the handshake.
 *
 * If the peer did not use the ALPN extension, or did not advertise a
 * protocol that matched one of @conn's protocols, or the TLS backend
 * does not support ALPN, then this will be %NULL. See
 * xdtls_connection_set_advertised_protocols().
 *
 * Returns: (nullable): the negotiated protocol, or %NULL
 *
 * Since: 2.60
 */
const xchar_t *
xdtls_connection_get_negotiated_protocol (xdtls_connection_t *conn)
{
  xdtls_connection_interface_t *iface;

  iface = XDTLS_CONNECTION_GET_INTERFACE (conn);
  if (iface->get_negotiated_protocol == NULL)
    return NULL;

  return iface->get_negotiated_protocol (conn);
}

/**
 * xdtls_connection_get_channel_binding_data:
 * @conn: a #xdtls_connection_t
 * @type: #GTlsChannelBindingType type of data to fetch
 * @data: (out callee-allocates)(optional)(transfer none): #xbyte_array_t is
 *        filled with the binding data, or %NULL
 * @error: a #xerror_t pointer, or %NULL
 *
 * Query the TLS backend for TLS channel binding data of @type for @conn.
 *
 * This call retrieves TLS channel binding data as specified in RFC
 * [5056](https://tools.ietf.org/html/rfc5056), RFC
 * [5929](https://tools.ietf.org/html/rfc5929), and related RFCs.  The
 * binding data is returned in @data.  The @data is resized by the callee
 * using #xbyte_array_t buffer management and will be freed when the @data
 * is destroyed by xbyte_array_unref(). If @data is %NULL, it will only
 * check whether TLS backend is able to fetch the data (e.g. whether @type
 * is supported by the TLS backend). It does not guarantee that the data
 * will be available though.  That could happen if TLS connection does not
 * support @type or the binding data is not available yet due to additional
 * negotiation or input required.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 *
 * Since: 2.66
 */
xboolean_t
xdtls_connection_get_channel_binding_data (xdtls_connection_t         *conn,
                                            GTlsChannelBindingType   type,
                                            xbyte_array_t              *data,
                                            xerror_t                 **error)
{
  xdtls_connection_interface_t *iface;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  iface = XDTLS_CONNECTION_GET_INTERFACE (conn);
  if (iface->get_binding_data == NULL)
    {
      g_set_error_literal (error, G_TLS_CHANNEL_BINDING_ERROR,
          G_TLS_CHANNEL_BINDING_ERROR_NOT_IMPLEMENTED,
          _("TLS backend does not implement TLS binding retrieval"));
      return FALSE;
    }

  return iface->get_binding_data (conn, type, data, error);
}

/**
 * xdtls_connection_get_protocol_version:
 * @conn: a #GDTlsConnection
 *
 * Returns the current DTLS protocol version, which may be
 * %G_TLS_PROTOCOL_VERSION_UNKNOWN if the connection has not handshaked, or
 * has been closed, or if the TLS backend has implemented a protocol version
 * that is not a recognized #GTlsProtocolVersion.
 *
 * Returns: The current DTLS protocol version
 *
 * Since: 2.70
 */
GTlsProtocolVersion
xdtls_connection_get_protocol_version (xdtls_connection_t *conn)
{
  GTlsProtocolVersion protocol_version;
  xenum_class_t *enum_class;
  xenum_value_t *enum_value;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), G_TLS_PROTOCOL_VERSION_UNKNOWN);

  xobject_get (G_OBJECT (conn),
                "protocol-version", &protocol_version,
                NULL);

  /* Convert unknown values to G_TLS_PROTOCOL_VERSION_UNKNOWN. */
  enum_class = xtype_class_peek_static (XTYPE_TLS_PROTOCOL_VERSION);
  enum_value = xenum_get_value (enum_class, protocol_version);
  return enum_value ? protocol_version : G_TLS_PROTOCOL_VERSION_UNKNOWN;
}

/**
 * xdtls_connection_get_ciphersuite_name:
 * @conn: a #GDTlsConnection
 *
 * Returns the name of the current DTLS ciphersuite, or %NULL if the
 * connection has not handshaked or has been closed. Beware that the TLS
 * backend may use any of multiple different naming conventions, because
 * OpenSSL and GnuTLS have their own ciphersuite naming conventions that
 * are different from each other and different from the standard, IANA-
 * registered ciphersuite names. The ciphersuite name is intended to be
 * displayed to the user for informative purposes only, and parsing it
 * is not recommended.
 *
 * Returns: (nullable): The name of the current DTLS ciphersuite, or %NULL
 *
 * Since: 2.70
 */
xchar_t *
xdtls_connection_get_ciphersuite_name (xdtls_connection_t *conn)
{
  xchar_t *ciphersuite_name;

  xreturn_val_if_fail (X_IS_DTLS_CONNECTION (conn), NULL);

  xobject_get (G_OBJECT (conn),
                "ciphersuite-name", &ciphersuite_name,
                NULL);

  return g_steal_pointer (&ciphersuite_name);
}
