/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __G_TLS_BACKEND_H__
#define __G_TLS_BACKEND_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * G_TLS_BACKEND_EXTENSION_POINT_NAME:
 *
 * Extension point for TLS functionality via #xtls_backend_t.
 * See [Extending GIO][extending-gio].
 */
#define G_TLS_BACKEND_EXTENSION_POINT_NAME "gio-tls-backend"

#define XTYPE_TLS_BACKEND               (xtls_backend_get_type ())
#define G_TLS_BACKEND(obj)               (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_TLS_BACKEND, xtls_backend))
#define X_IS_TLS_BACKEND(obj)	         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_TLS_BACKEND))
#define G_TLS_BACKEND_GET_INTERFACE(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_TLS_BACKEND, xtls_backend_interface_t))

typedef struct _GTlsBackend          xtls_backend_t;
typedef struct _xtls_backend_interface xtls_backend_interface_t;

/**
 * xtls_backend_interface_t:
 * @x_iface: The parent interface.
 * @supports_tls: returns whether the backend supports TLS.
 * @supports_dtls: returns whether the backend supports DTLS
 * @get_default_database: returns a default #xtls_database_t instance.
 * @get_certificate_type: returns the #xtls_certificate_t implementation type
 * @get_client_connection_type: returns the #xtls_client_connection_t implementation type
 * @get_server_connection_type: returns the #xtls_server_connection_t implementation type
 * @get_file_database_type: returns the #xtls_file_database_t implementation type.
 * @get_dtls_client_connection_type: returns the #xdtls_client_connection_t implementation type
 * @get_dtls_server_connection_type: returns the #xdtls_server_connection_t implementation type
 *
 * Provides an interface for describing TLS-related types.
 *
 * Since: 2.28
 */
struct _xtls_backend_interface
{
  xtype_interface_t x_iface;

  /* methods */
  xboolean_t       ( *supports_tls)               (xtls_backend_t *backend);
  xtype_t          ( *get_certificate_type)       (void);
  xtype_t          ( *get_client_connection_type) (void);
  xtype_t          ( *get_server_connection_type) (void);
  xtype_t          ( *get_file_database_type)     (void);
  xtls_database_t * ( *get_default_database)       (xtls_backend_t *backend);
  xboolean_t       ( *supports_dtls)              (xtls_backend_t *backend);
  xtype_t          ( *get_dtls_client_connection_type) (void);
  xtype_t          ( *get_dtls_server_connection_type) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t          xtls_backend_get_type                   (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xtls_backend_t *  xtls_backend_get_default                (void);

XPL_AVAILABLE_IN_ALL
xtls_database_t * xtls_backend_get_default_database       (xtls_backend_t *backend);
XPL_AVAILABLE_IN_2_60
void           xtls_backend_set_default_database       (xtls_backend_t  *backend,
                                                         xtls_database_t *database);

XPL_AVAILABLE_IN_ALL
xboolean_t       xtls_backend_supports_tls               (xtls_backend_t *backend);
XPL_AVAILABLE_IN_2_48
xboolean_t       xtls_backend_supports_dtls              (xtls_backend_t *backend);

XPL_AVAILABLE_IN_ALL
xtype_t          xtls_backend_get_certificate_type       (xtls_backend_t *backend);
XPL_AVAILABLE_IN_ALL
xtype_t          xtls_backend_get_client_connection_type (xtls_backend_t *backend);
XPL_AVAILABLE_IN_ALL
xtype_t          xtls_backend_get_server_connection_type (xtls_backend_t *backend);
XPL_AVAILABLE_IN_ALL
xtype_t          xtls_backend_get_file_database_type     (xtls_backend_t *backend);

XPL_AVAILABLE_IN_2_48
xtype_t          xtls_backend_get_dtls_client_connection_type (xtls_backend_t *backend);
XPL_AVAILABLE_IN_2_48
xtype_t          xtls_backend_get_dtls_server_connection_type (xtls_backend_t *backend);

G_END_DECLS

#endif /* __G_TLS_BACKEND_H__ */
