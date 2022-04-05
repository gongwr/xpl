/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __G_TLS_CONNECTION_H__
#define __G_TLS_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giostream.h>

G_BEGIN_DECLS

#define XTYPE_TLS_CONNECTION            (g_tls_connection_get_type ())
#define G_TLS_CONNECTION(inst)           (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TLS_CONNECTION, GTlsConnection))
#define G_TLS_CONNECTION_CLASS(class)    (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_TLS_CONNECTION, GTlsConnectionClass))
#define X_IS_TLS_CONNECTION(inst)        (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TLS_CONNECTION))
#define X_IS_TLS_CONNECTION_CLASS(class) (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_TLS_CONNECTION))
#define G_TLS_CONNECTION_GET_CLASS(inst) (XTYPE_INSTANCE_GET_CLASS ((inst), XTYPE_TLS_CONNECTION, GTlsConnectionClass))

typedef struct _GTlsConnectionClass   GTlsConnectionClass;
typedef struct _GTlsConnectionPrivate GTlsConnectionPrivate;

struct _GTlsConnection {
  xio_stream_t parent_instance;

  GTlsConnectionPrivate *priv;
};

/**
 * GTlsConnectionClass:
 * @parent_class: The parent class.
 * @accept_certificate: Check whether to accept a certificate.
 * @handshake: Perform a handshake operation.
 * @handshake_async: Start an asynchronous handshake operation.
 * @handshake_finish: Finish an asynchronous handshake operation.
 * @get_binding_data: Retrieve TLS channel binding data (Since: 2.66)
 * @get_negotiated_protocol: Get ALPN-negotiated protocol (Since: 2.70)
 *
 * The class structure for the #GTlsConnection type.
 *
 * Since: 2.28
 */
struct _GTlsConnectionClass
{
  xio_stream_class_t parent_class;

  /* signals */
  xboolean_t          ( *accept_certificate) (GTlsConnection       *connection,
					    GTlsCertificate      *peer_cert,
					    GTlsCertificateFlags  errors);

  /* methods */
  xboolean_t ( *handshake )        (GTlsConnection       *conn,
				  xcancellable_t         *cancellable,
				  xerror_t              **error);

  void     ( *handshake_async )  (GTlsConnection       *conn,
				  int                   io_priority,
				  xcancellable_t         *cancellable,
				  xasync_ready_callback_t   callback,
				  xpointer_t              user_data);
  xboolean_t ( *handshake_finish ) (GTlsConnection       *conn,
				  xasync_result_t         *result,
				  xerror_t              **error);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  xboolean_t ( *get_binding_data)  (GTlsConnection          *conn,
                                  GTlsChannelBindingType   type,
                                  GByteArray              *data,
                                  xerror_t                 **error);
G_GNUC_END_IGNORE_DEPRECATIONS

  const xchar_t *(*get_negotiated_protocol) (GTlsConnection *conn);

  /*< private >*/
  /* Padding for future expansion */
  xpointer_t padding[6];
};

XPL_AVAILABLE_IN_ALL
xtype_t                 g_tls_connection_get_type                    (void) G_GNUC_CONST;

XPL_DEPRECATED
void                  g_tls_connection_set_use_system_certdb       (GTlsConnection       *conn,
                                                                    xboolean_t              use_system_certdb);
XPL_DEPRECATED
xboolean_t              g_tls_connection_get_use_system_certdb       (GTlsConnection       *conn);

XPL_AVAILABLE_IN_ALL
void                  g_tls_connection_set_database                (GTlsConnection       *conn,
								    GTlsDatabase         *database);
XPL_AVAILABLE_IN_ALL
GTlsDatabase *        g_tls_connection_get_database                (GTlsConnection       *conn);

XPL_AVAILABLE_IN_ALL
void                  g_tls_connection_set_certificate             (GTlsConnection       *conn,
                                                                    GTlsCertificate      *certificate);
XPL_AVAILABLE_IN_ALL
GTlsCertificate      *g_tls_connection_get_certificate             (GTlsConnection       *conn);

XPL_AVAILABLE_IN_ALL
void                  g_tls_connection_set_interaction             (GTlsConnection       *conn,
                                                                    GTlsInteraction      *interaction);
XPL_AVAILABLE_IN_ALL
GTlsInteraction *     g_tls_connection_get_interaction             (GTlsConnection       *conn);

XPL_AVAILABLE_IN_ALL
GTlsCertificate      *g_tls_connection_get_peer_certificate        (GTlsConnection       *conn);
XPL_AVAILABLE_IN_ALL
GTlsCertificateFlags  g_tls_connection_get_peer_certificate_errors (GTlsConnection       *conn);

XPL_AVAILABLE_IN_ALL
void                  g_tls_connection_set_require_close_notify    (GTlsConnection       *conn,
								    xboolean_t              require_close_notify);
XPL_AVAILABLE_IN_ALL
xboolean_t              g_tls_connection_get_require_close_notify    (GTlsConnection       *conn);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_60
void                  g_tls_connection_set_rehandshake_mode        (GTlsConnection       *conn,
								    GTlsRehandshakeMode   mode);
XPL_DEPRECATED_IN_2_60
GTlsRehandshakeMode   g_tls_connection_get_rehandshake_mode        (GTlsConnection       *conn);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_2_60
void                  g_tls_connection_set_advertised_protocols    (GTlsConnection       *conn,
                                                                    const xchar_t * const  *protocols);

XPL_AVAILABLE_IN_2_60
const xchar_t *         g_tls_connection_get_negotiated_protocol     (GTlsConnection       *conn);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_2_66
xboolean_t              g_tls_connection_get_channel_binding_data    (GTlsConnection          *conn,
                                                                    GTlsChannelBindingType   type,
                                                                    GByteArray              *data,
                                                                    xerror_t                 **error);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
xboolean_t              g_tls_connection_handshake                   (GTlsConnection       *conn,
								    xcancellable_t         *cancellable,
								    xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void                  g_tls_connection_handshake_async             (GTlsConnection       *conn,
								    int                   io_priority,
								    xcancellable_t         *cancellable,
								    xasync_ready_callback_t   callback,
								    xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t              g_tls_connection_handshake_finish            (GTlsConnection       *conn,
								    xasync_result_t         *result,
								    xerror_t              **error);

XPL_AVAILABLE_IN_2_70
GTlsProtocolVersion   g_tls_connection_get_protocol_version        (GTlsConnection       *conn);

XPL_AVAILABLE_IN_2_70
xchar_t *               g_tls_connection_get_ciphersuite_name        (GTlsConnection       *conn);

/**
 * G_TLS_ERROR:
 *
 * Error domain for TLS. Errors in this domain will be from the
 * #GTlsError enumeration. See #xerror_t for more information on error
 * domains.
 */
#define G_TLS_ERROR (g_tls_error_quark ())
XPL_AVAILABLE_IN_ALL
GQuark g_tls_error_quark (void);

/**
 * G_TLS_CHANNEL_BINDING_ERROR:
 *
 * Error domain for TLS channel binding. Errors in this domain will be from the
 * #GTlsChannelBindingError enumeration. See #xerror_t for more information on error
 * domains.
 *
 * Since: 2.66
 */
#define G_TLS_CHANNEL_BINDING_ERROR (g_tls_channel_binding_error_quark ())
XPL_AVAILABLE_IN_2_66
GQuark g_tls_channel_binding_error_quark (void);

/*< protected >*/
XPL_AVAILABLE_IN_ALL
xboolean_t              g_tls_connection_emit_accept_certificate     (GTlsConnection       *conn,
								    GTlsCertificate      *peer_cert,
								    GTlsCertificateFlags  errors);

G_END_DECLS

#endif /* __G_TLS_CONNECTION_H__ */
