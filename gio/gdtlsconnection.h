/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2010 Red Hat, Inc.
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

#ifndef __XDTLS_CONNECTION_H__
#define __XDTLS_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gdatagrambased.h>

G_BEGIN_DECLS

#define XTYPE_DTLS_CONNECTION                (xdtls_connection_get_type ())
#define XDTLS_CONNECTION(inst)               (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_DTLS_CONNECTION, xdtls_connection_t))
#define X_IS_DTLS_CONNECTION(inst)            (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_DTLS_CONNECTION))
#define XDTLS_CONNECTION_GET_INTERFACE(inst) (XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_DTLS_CONNECTION, xdtls_connection_interface_t))

typedef struct _xdtls_connection_interface xdtls_connection_interface_t;

/**
 * xdtls_connection_interface_t:
 * @x_iface: The parent interface.
 * @accept_certificate: Check whether to accept a certificate.
 * @handshake: Perform a handshake operation.
 * @handshake_async: Start an asynchronous handshake operation.
 * @handshake_finish: Finish an asynchronous handshake operation.
 * @shutdown: Shut down one or both directions of the connection.
 * @shutdown_async: Start an asynchronous shutdown operation.
 * @shutdown_finish: Finish an asynchronous shutdown operation.
 * @set_advertised_protocols: Set APLN protocol list (Since: 2.60)
 * @get_negotiated_protocol: Get ALPN-negotiated protocol (Since: 2.60)
 * @get_binding_data: Retrieve TLS channel binding data (Since: 2.66)
 *
 * Virtual method table for a #xdtls_connection_t implementation.
 *
 * Since: 2.48
 */
struct _xdtls_connection_interface
{
  xtype_interface_t x_iface;

  /* signals */
  xboolean_t (*accept_certificate) (xdtls_connection_t       *connection,
                                  xtls_certificate_t       *peer_cert,
                                  xtls_certificate_flags_t   errors);

  /* methods */
  xboolean_t (*handshake)          (xdtls_connection_t       *conn,
                                  xcancellable_t          *cancellable,
                                  xerror_t               **error);

  void     (*handshake_async)    (xdtls_connection_t       *conn,
                                  int                    io_priority,
                                  xcancellable_t          *cancellable,
                                  xasync_ready_callback_t    callback,
                                  xpointer_t               user_data);
  xboolean_t (*handshake_finish)   (xdtls_connection_t       *conn,
                                  xasync_result_t          *result,
                                  xerror_t               **error);

  xboolean_t (*shutdown)           (xdtls_connection_t       *conn,
                                  xboolean_t               shutdown_read,
                                  xboolean_t               shutdown_write,
                                  xcancellable_t          *cancellable,
                                  xerror_t               **error);

  void     (*shutdown_async)     (xdtls_connection_t       *conn,
                                  xboolean_t               shutdown_read,
                                  xboolean_t               shutdown_write,
                                  int                    io_priority,
                                  xcancellable_t          *cancellable,
                                  xasync_ready_callback_t    callback,
                                  xpointer_t               user_data);
  xboolean_t (*shutdown_finish)    (xdtls_connection_t       *conn,
                                  xasync_result_t          *result,
                                  xerror_t               **error);

  void (*set_advertised_protocols)        (xdtls_connection_t     *conn,
                                           const xchar_t * const *protocols);
  const xchar_t *(*get_negotiated_protocol) (xdtls_connection_t     *conn);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  xboolean_t  (*get_binding_data)  (xdtls_connection_t         *conn,
                                  GTlsChannelBindingType   type,
                                  xbyte_array_t              *data,
                                  xerror_t                 **error);
G_GNUC_END_IGNORE_DEPRECATIONS
};

XPL_AVAILABLE_IN_2_48
xtype_t                 xdtls_connection_get_type                    (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_set_database                (xdtls_connection_t       *conn,
                                                                     xtls_database_t          *database);
XPL_AVAILABLE_IN_2_48
xtls_database_t         *xdtls_connection_get_database                (xdtls_connection_t       *conn);

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_set_certificate             (xdtls_connection_t       *conn,
                                                                     xtls_certificate_t       *certificate);
XPL_AVAILABLE_IN_2_48
xtls_certificate_t      *xdtls_connection_get_certificate             (xdtls_connection_t       *conn);

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_set_interaction             (xdtls_connection_t       *conn,
                                                                     xtls_interaction_t       *interaction);
XPL_AVAILABLE_IN_2_48
xtls_interaction_t      *xdtls_connection_get_interaction             (xdtls_connection_t       *conn);

XPL_AVAILABLE_IN_2_48
xtls_certificate_t      *xdtls_connection_get_peer_certificate        (xdtls_connection_t       *conn);
XPL_AVAILABLE_IN_2_48
xtls_certificate_flags_t  xdtls_connection_get_peer_certificate_errors (xdtls_connection_t       *conn);

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_set_require_close_notify    (xdtls_connection_t       *conn,
                                                                     xboolean_t               require_close_notify);
XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_get_require_close_notify    (xdtls_connection_t       *conn);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_60
void                  xdtls_connection_set_rehandshake_mode        (xdtls_connection_t       *conn,
                                                                     GTlsRehandshakeMode    mode);
XPL_DEPRECATED_IN_2_60
GTlsRehandshakeMode   xdtls_connection_get_rehandshake_mode        (xdtls_connection_t       *conn);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_handshake                   (xdtls_connection_t       *conn,
                                                                     xcancellable_t          *cancellable,
                                                                     xerror_t               **error);

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_handshake_async             (xdtls_connection_t       *conn,
                                                                     int                    io_priority,
                                                                     xcancellable_t          *cancellable,
                                                                     xasync_ready_callback_t    callback,
                                                                     xpointer_t               user_data);
XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_handshake_finish            (xdtls_connection_t       *conn,
                                                                     xasync_result_t          *result,
                                                                     xerror_t               **error);

XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_shutdown                    (xdtls_connection_t       *conn,
                                                                     xboolean_t               shutdown_read,
                                                                     xboolean_t               shutdown_write,
                                                                     xcancellable_t          *cancellable,
                                                                     xerror_t               **error);

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_shutdown_async              (xdtls_connection_t       *conn,
                                                                     xboolean_t               shutdown_read,
                                                                     xboolean_t               shutdown_write,
                                                                     int                    io_priority,
                                                                     xcancellable_t          *cancellable,
                                                                     xasync_ready_callback_t    callback,
                                                                     xpointer_t               user_data);
XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_shutdown_finish             (xdtls_connection_t       *conn,
                                                                     xasync_result_t          *result,
                                                                     xerror_t               **error);

XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_close                       (xdtls_connection_t       *conn,
                                                                     xcancellable_t          *cancellable,
                                                                     xerror_t               **error);

XPL_AVAILABLE_IN_2_48
void                  xdtls_connection_close_async                 (xdtls_connection_t       *conn,
                                                                     int                    io_priority,
                                                                     xcancellable_t          *cancellable,
                                                                     xasync_ready_callback_t    callback,
                                                                     xpointer_t               user_data);
XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_close_finish                (xdtls_connection_t       *conn,
                                                                     xasync_result_t          *result,
                                                                     xerror_t               **error);

/*< protected >*/
XPL_AVAILABLE_IN_2_48
xboolean_t              xdtls_connection_emit_accept_certificate     (xdtls_connection_t       *conn,
                                                                     xtls_certificate_t       *peer_cert,
                                                                     xtls_certificate_flags_t   errors);
XPL_AVAILABLE_IN_2_60
void                  xdtls_connection_set_advertised_protocols    (xdtls_connection_t     *conn,
                                                                     const xchar_t * const *protocols);

XPL_AVAILABLE_IN_2_60
const xchar_t *          xdtls_connection_get_negotiated_protocol     (xdtls_connection_t    *conn);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_2_66
xboolean_t              xdtls_connection_get_channel_binding_data    (xdtls_connection_t         *conn,
                                                                     GTlsChannelBindingType   type,
                                                                     xbyte_array_t              *data,
                                                                     xerror_t                 **error);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_2_70
GTlsProtocolVersion   xdtls_connection_get_protocol_version        (xdtls_connection_t       *conn);

XPL_AVAILABLE_IN_2_70
xchar_t *               xdtls_connection_get_ciphersuite_name        (xdtls_connection_t       *conn);

G_END_DECLS

#endif /* __XDTLS_CONNECTION_H__ */
