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

#include "gtlsbackend.h"
#include "gtlsdatabase.h"
#include "gdummytlsbackend.h"
#include "gioenumtypes.h"
#include "giomodule-priv.h"

/**
 * SECTION:gtls
 * @title: TLS Overview
 * @short_description: TLS (aka SSL) support for xsocket_connection_t
 * @include: gio/gio.h
 *
 * #xtls_connection_t and related classes provide TLS (Transport Layer
 * Security, previously known as SSL, Secure Sockets Layer) support for
 * gio-based network streams.
 *
 * #xdtls_connection_t and related classes provide DTLS (Datagram TLS) support for
 * GIO-based network sockets, using the #xdatagram_based_t interface. The TLS and
 * DTLS APIs are almost identical, except TLS is stream-based and DTLS is
 * datagram-based. They share certificate and backend infrastructure.
 *
 * In the simplest case, for a client TLS connection, you can just set the
 * #xsocket_client_t:tls flag on a #xsocket_client_t, and then any
 * connections created by that client will have TLS negotiated
 * automatically, using appropriate default settings, and rejecting
 * any invalid or self-signed certificates (unless you change that
 * default by setting the #xsocket_client_t:tls-validation-flags
 * property). The returned object will be a #xtcp_wrapper_connection_t,
 * which wraps the underlying #xtls_client_connection_t.
 *
 * For greater control, you can create your own #xtls_client_connection_t,
 * wrapping a #xsocket_connection_t (or an arbitrary #xio_stream_t with
 * pollable input and output streams) and then connect to its signals,
 * such as #xtls_connection_t::accept-certificate, before starting the
 * handshake.
 *
 * Server-side TLS is similar, using #xtls_server_connection_t. At the
 * moment, there is no support for automatically wrapping server-side
 * connections in the way #xsocket_client_t does for client-side
 * connections.
 */

/**
 * SECTION:gtlsbackend
 * @title: xtls_backend_t
 * @short_description: TLS backend implementation
 * @include: gio/gio.h
 *
 * TLS (Transport Layer Security, aka SSL) and DTLS backend.
 *
 * Since: 2.28
 */

/**
 * xtls_backend_t:
 *
 * TLS (Transport Layer Security, aka SSL) and DTLS backend. This is an
 * internal type used to coordinate the different classes implemented
 * by a TLS backend.
 *
 * Since: 2.28
 */

G_DEFINE_INTERFACE (xtls_backend, xtls_backend, XTYPE_OBJECT)

static xtls_database_t *default_database;
G_LOCK_DEFINE_STATIC (default_database_lock);

static void
xtls_backend_default_init (xtls_backend_interface_t *iface)
{
}

static xtls_backend_t *tls_backend_default_singleton = NULL;  /* (owned) (atomic) */

/**
 * xtls_backend_get_default:
 *
 * Gets the default #xtls_backend_t for the system.
 *
 * Returns: (not nullable) (transfer none): a #xtls_backend_t, which will be a
 *     dummy object if no TLS backend is available
 *
 * Since: 2.28
 */
xtls_backend_t *
xtls_backend_get_default (void)
{
  if (g_once_init_enter (&tls_backend_default_singleton))
    {
      xtls_backend_t *singleton;

      singleton = _xio_module_get_default (G_TLS_BACKEND_EXTENSION_POINT_NAME,
                                            "GIO_USE_TLS",
                                            NULL);

      g_once_init_leave (&tls_backend_default_singleton, singleton);
    }

  return tls_backend_default_singleton;
}

/**
 * xtls_backend_supports_tls:
 * @backend: the #xtls_backend_t
 *
 * Checks if TLS is supported; if this returns %FALSE for the default
 * #xtls_backend_t, it means no "real" TLS backend is available.
 *
 * Returns: whether or not TLS is supported
 *
 * Since: 2.28
 */
xboolean_t
xtls_backend_supports_tls (xtls_backend_t *backend)
{
  if (G_TLS_BACKEND_GET_INTERFACE (backend)->supports_tls)
    return G_TLS_BACKEND_GET_INTERFACE (backend)->supports_tls (backend);
  else if (X_IS_DUMMY_TLS_BACKEND (backend))
    return FALSE;
  else
    return TRUE;
}

/**
 * xtls_backend_supports_dtls:
 * @backend: the #xtls_backend_t
 *
 * Checks if DTLS is supported. DTLS support may not be available even if TLS
 * support is available, and vice-versa.
 *
 * Returns: whether DTLS is supported
 *
 * Since: 2.48
 */
xboolean_t
xtls_backend_supports_dtls (xtls_backend_t *backend)
{
  if (G_TLS_BACKEND_GET_INTERFACE (backend)->supports_dtls)
    return G_TLS_BACKEND_GET_INTERFACE (backend)->supports_dtls (backend);

  return FALSE;
}

/**
 * xtls_backend_get_default_database:
 * @backend: the #xtls_backend_t
 *
 * Gets the default #xtls_database_t used to verify TLS connections.
 *
 * Returns: (transfer full): the default database, which should be
 *               unreffed when done.
 *
 * Since: 2.30
 */
xtls_database_t *
xtls_backend_get_default_database (xtls_backend_t *backend)
{
  xtls_database_t *db;

  g_return_val_if_fail (X_IS_TLS_BACKEND (backend), NULL);

  /* This method was added later, so accept the (remote) possibility it can be NULL */
  if (!G_TLS_BACKEND_GET_INTERFACE (backend)->get_default_database)
    return NULL;

  G_LOCK (default_database_lock);

  if (!default_database)
    default_database = G_TLS_BACKEND_GET_INTERFACE (backend)->get_default_database (backend);
  db = default_database ? xobject_ref (default_database) : NULL;
  G_UNLOCK (default_database_lock);

  return db;
}

/**
 * xtls_backend_set_default_database:
 * @backend: the #xtls_backend_t
 * @database: (nullable): the #xtls_database_t
 *
 * Set the default #xtls_database_t used to verify TLS connections
 *
 * Any subsequent call to xtls_backend_get_default_database() will return
 * the database set in this call.  Existing databases and connections are not
 * modified.
 *
 * Setting a %NULL default database will reset to using the system default
 * database as if xtls_backend_set_default_database() had never been called.
 *
 * Since: 2.60
 */
void
xtls_backend_set_default_database (xtls_backend_t  *backend,
                                    xtls_database_t *database)
{
  g_return_if_fail (X_IS_TLS_BACKEND (backend));
  g_return_if_fail (database == NULL || X_IS_TLS_DATABASE (database));

  G_LOCK (default_database_lock);
  g_set_object (&default_database, database);
  G_UNLOCK (default_database_lock);
}

/**
 * xtls_backend_get_certificate_type:
 * @backend: the #xtls_backend_t
 *
 * Gets the #xtype_t of @backend's #xtls_certificate_t implementation.
 *
 * Returns: the #xtype_t of @backend's #xtls_certificate_t
 *   implementation.
 *
 * Since: 2.28
 */
xtype_t
xtls_backend_get_certificate_type (xtls_backend_t *backend)
{
  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_certificate_type ();
}

/**
 * xtls_backend_get_client_connection_type:
 * @backend: the #xtls_backend_t
 *
 * Gets the #xtype_t of @backend's #xtls_client_connection_t implementation.
 *
 * Returns: the #xtype_t of @backend's #xtls_client_connection_t
 *   implementation.
 *
 * Since: 2.28
 */
xtype_t
xtls_backend_get_client_connection_type (xtls_backend_t *backend)
{
  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_client_connection_type ();
}

/**
 * xtls_backend_get_server_connection_type:
 * @backend: the #xtls_backend_t
 *
 * Gets the #xtype_t of @backend's #xtls_server_connection_t implementation.
 *
 * Returns: the #xtype_t of @backend's #xtls_server_connection_t
 *   implementation.
 *
 * Since: 2.28
 */
xtype_t
xtls_backend_get_server_connection_type (xtls_backend_t *backend)
{
  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_server_connection_type ();
}

/**
 * xtls_backend_get_dtls_client_connection_type:
 * @backend: the #xtls_backend_t
 *
 * Gets the #xtype_t of @backend’s #xdtls_client_connection_t implementation.
 *
 * Returns: the #xtype_t of @backend’s #xdtls_client_connection_t
 *   implementation, or %XTYPE_INVALID if this backend doesn’t support DTLS.
 *
 * Since: 2.48
 */
xtype_t
xtls_backend_get_dtls_client_connection_type (xtls_backend_t *backend)
{
  xtls_backend_interface_t *iface;

  g_return_val_if_fail (X_IS_TLS_BACKEND (backend), XTYPE_INVALID);

  iface = G_TLS_BACKEND_GET_INTERFACE (backend);
  if (iface->get_dtls_client_connection_type == NULL)
    return XTYPE_INVALID;

  return iface->get_dtls_client_connection_type ();
}

/**
 * xtls_backend_get_dtls_server_connection_type:
 * @backend: the #xtls_backend_t
 *
 * Gets the #xtype_t of @backend’s #xdtls_server_connection_t implementation.
 *
 * Returns: the #xtype_t of @backend’s #xdtls_server_connection_t
 *   implementation, or %XTYPE_INVALID if this backend doesn’t support DTLS.
 *
 * Since: 2.48
 */
xtype_t
xtls_backend_get_dtls_server_connection_type (xtls_backend_t *backend)
{
  xtls_backend_interface_t *iface;

  g_return_val_if_fail (X_IS_TLS_BACKEND (backend), XTYPE_INVALID);

  iface = G_TLS_BACKEND_GET_INTERFACE (backend);
  if (iface->get_dtls_server_connection_type == NULL)
    return XTYPE_INVALID;

  return iface->get_dtls_server_connection_type ();
}

/**
 * xtls_backend_get_file_database_type:
 * @backend: the #xtls_backend_t
 *
 * Gets the #xtype_t of @backend's #xtls_file_database_t implementation.
 *
 * Returns: the #xtype_t of backend's #xtls_file_database_t implementation.
 *
 * Since: 2.30
 */
xtype_t
xtls_backend_get_file_database_type (xtls_backend_t *backend)
{
  g_return_val_if_fail (X_IS_TLS_BACKEND (backend), 0);

  /* This method was added later, so accept the (remote) possibility it can be NULL */
  if (!G_TLS_BACKEND_GET_INTERFACE (backend)->get_file_database_type)
    return 0;

  return G_TLS_BACKEND_GET_INTERFACE (backend)->get_file_database_type ();
}
