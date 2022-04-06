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

#include "config.h"

#include "gdummytlsbackend.h"

#include <glib.h>

#include "gasyncresult.h"
#include "gcancellable.h"
#include "ginitable.h"
#include "gdtlsclientconnection.h"
#include "gdtlsconnection.h"
#include "gdtlsserverconnection.h"
#include "gtlsbackend.h"
#include "gtlscertificate.h"
#include "gtlsclientconnection.h"
#include "gtlsdatabase.h"
#include "gtlsfiledatabase.h"
#include "gtlsserverconnection.h"

#include "giomodule.h"
#include "giomodule-priv.h"

#include "glibintl.h"

static xtype_t _xdummy_tls_certificate_get_type (void);
static xtype_t _xdummy_tls_connection_get_type (void);
static xtype_t _xdummy_dtls_connection_get_type (void);
static xtype_t _xdummy_tls_database_get_type (void);

struct _xdummy_tls_backend {
  xobject_t       parent_instance;
  xtls_database_t *database;
};

static void xdummy_tls_backend_iface_init (xtls_backend_interface_t *iface);

#define xdummy_tls_backend_get_type _xdummy_tls_backend_get_type
G_DEFINE_TYPE_WITH_CODE (xdummy_tls_backend_t, xdummy_tls_backend, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_BACKEND,
						xdummy_tls_backend_iface_init)
			 _xio_modules_ensure_extension_points_registered ();
			 g_io_extension_point_implement (G_TLS_BACKEND_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "dummy",
							 -100);)

static void
xdummy_tls_backend_init (xdummy_tls_backend_t *dummy)
{
}

static void
xdummy_tls_backend_finalize (xobject_t *object)
{
  xdummy_tls_backend_t *dummy = G_DUMMY_TLS_BACKEND (object);

  g_clear_object (&dummy->database);

  G_OBJECT_CLASS (xdummy_tls_backend_parent_class)->finalize (object);
}

static void
xdummy_tls_backend_class_init (GDummyTlsBackendClass *backend_class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (backend_class);

  object_class->finalize = xdummy_tls_backend_finalize;
}

static xtls_database_t *
xdummy_tls_backend_get_default_database (xtls_backend_t *backend)
{
  xdummy_tls_backend_t *dummy = G_DUMMY_TLS_BACKEND (backend);

  if (g_once_init_enter (&dummy->database))
    {
      xtls_database_t *tlsdb;

      tlsdb = xobject_new (_xdummy_tls_database_get_type (), NULL);
      g_once_init_leave (&dummy->database, tlsdb);
    }

  return xobject_ref (dummy->database);
}

static void
xdummy_tls_backend_iface_init (xtls_backend_interface_t *iface)
{
  iface->get_certificate_type = _xdummy_tls_certificate_get_type;
  iface->get_client_connection_type = _xdummy_tls_connection_get_type;
  iface->get_server_connection_type = _xdummy_tls_connection_get_type;
  iface->get_dtls_client_connection_type = _xdummy_dtls_connection_get_type;
  iface->get_dtls_server_connection_type = _xdummy_dtls_connection_get_type;
  iface->get_file_database_type = _xdummy_tls_database_get_type;
  iface->get_default_database = xdummy_tls_backend_get_default_database;
}

/* Dummy certificate type */

typedef struct _GDummyTlsCertificate      GDummyTlsCertificate;
typedef struct _GDummyTlsCertificateClass GDummyTlsCertificateClass;

struct _GDummyTlsCertificate {
  xtls_certificate_t parent_instance;
};

struct _GDummyTlsCertificateClass {
  GTlsCertificateClass parent_class;
};

enum
{
  PROP_CERTIFICATE_0,

  PROP_CERT_CERTIFICATE,
  PROP_CERT_CERTIFICATE_PEM,
  PROP_CERT_PRIVATE_KEY,
  PROP_CERT_PRIVATE_KEY_PEM,
  PROP_CERT_ISSUER
};

static void xdummy_tls_certificate_initable_iface_init (xinitable_iface_t *iface);

#define xdummy_tls_certificate_get_type _xdummy_tls_certificate_get_type
G_DEFINE_TYPE_WITH_CODE (GDummyTlsCertificate, xdummy_tls_certificate, XTYPE_TLS_CERTIFICATE,
			 G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
						xdummy_tls_certificate_initable_iface_init))

static void
xdummy_tls_certificate_get_property (xobject_t    *object,
				      xuint_t       prop_id,
				      xvalue_t     *value,
				      xparam_spec_t *pspec)
{
  /* We need to define this method to make xobject_t happy, but it will
   * never be possible to construct a working GDummyTlsCertificate, so
   * it doesn't have to do anything useful.
   */
}

static void
xdummy_tls_certificate_set_property (xobject_t      *object,
				      xuint_t         prop_id,
				      const xvalue_t *value,
				      xparam_spec_t   *pspec)
{
  /* Just ignore all attempts to set properties. */
}

static void
xdummy_tls_certificate_class_init (GDummyTlsCertificateClass *certificate_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (certificate_class);

  gobject_class->get_property = xdummy_tls_certificate_get_property;
  gobject_class->set_property = xdummy_tls_certificate_set_property;

  xobject_class_override_property (gobject_class, PROP_CERT_CERTIFICATE, "certificate");
  xobject_class_override_property (gobject_class, PROP_CERT_CERTIFICATE_PEM, "certificate-pem");
  xobject_class_override_property (gobject_class, PROP_CERT_PRIVATE_KEY, "private-key");
  xobject_class_override_property (gobject_class, PROP_CERT_PRIVATE_KEY_PEM, "private-key-pem");
  xobject_class_override_property (gobject_class, PROP_CERT_ISSUER, "issuer");
}

static void
xdummy_tls_certificate_init (GDummyTlsCertificate *certificate)
{
}

static xboolean_t
xdummy_tls_certificate_initable_init (xinitable_t       *initable,
				       xcancellable_t    *cancellable,
				       xerror_t         **error)
{
  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_UNAVAILABLE,
		       _("TLS support is not available"));
  return FALSE;
}

static void
xdummy_tls_certificate_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = xdummy_tls_certificate_initable_init;
}

/* Dummy connection type; since xtls_client_connection_t and
 * xtls_server_connection_t are just interfaces, we can implement them
 * both on a single object.
 */

typedef struct _GDummyTlsConnection      GDummyTlsConnection;
typedef struct _GDummyTlsConnectionClass GDummyTlsConnectionClass;

struct _GDummyTlsConnection {
  xtls_connection_t parent_instance;
};

struct _GDummyTlsConnectionClass {
  GTlsConnectionClass parent_class;
};

enum
{
  PROP_CONNECTION_0,

  PROP_CONN_BASE_IO_STREAM,
  PROP_CONN_USE_SYSTEM_CERTDB,
  PROP_CONN_REQUIRE_CLOSE_NOTIFY,
  PROP_CONN_REHANDSHAKE_MODE,
  PROP_CONN_CERTIFICATE,
  PROP_CONN_DATABASE,
  PROP_CONN_INTERACTION,
  PROP_CONN_PEER_CERTIFICATE,
  PROP_CONN_PEER_CERTIFICATE_ERRORS,
  PROP_CONN_VALIDATION_FLAGS,
  PROP_CONN_SERVER_IDENTITY,
  PROP_CONN_USE_SSL3,
  PROP_CONN_ACCEPTED_CAS,
  PROP_CONN_AUTHENTICATION_MODE,
  PROP_CONN_ADVERTISED_PROTOCOLS,
  PROP_CONN_NEGOTIATED_PROTOCOL,
};

static void xdummy_tls_connection_initable_iface_init (xinitable_iface_t *iface);

#define xdummy_tls_connection_get_type _xdummy_tls_connection_get_type
G_DEFINE_TYPE_WITH_CODE (GDummyTlsConnection, xdummy_tls_connection, XTYPE_TLS_CONNECTION,
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_CLIENT_CONNECTION, NULL)
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_SERVER_CONNECTION, NULL)
			 G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
						xdummy_tls_connection_initable_iface_init))

static void
xdummy_tls_connection_get_property (xobject_t    *object,
				     xuint_t       prop_id,
				     xvalue_t     *value,
				     xparam_spec_t *pspec)
{
}

static void
xdummy_tls_connection_set_property (xobject_t      *object,
				     xuint_t         prop_id,
				     const xvalue_t *value,
				     xparam_spec_t   *pspec)
{
}

static xboolean_t
xdummy_tls_connection_close (xio_stream_t     *stream,
			      xcancellable_t  *cancellable,
			      xerror_t       **error)
{
  return TRUE;
}

static void
xdummy_tls_connection_class_init (GDummyTlsConnectionClass *connection_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (connection_class);
  xio_stream_class_t *io_stream_class = XIO_STREAM_CLASS (connection_class);

  gobject_class->get_property = xdummy_tls_connection_get_property;
  gobject_class->set_property = xdummy_tls_connection_set_property;

  /* Need to override this because when initable_init fails it will
   * dispose the connection, which will close it, which would
   * otherwise try to close its input/output streams, which don't
   * exist.
   */
  io_stream_class->close_fn = xdummy_tls_connection_close;

  xobject_class_override_property (gobject_class, PROP_CONN_BASE_IO_STREAM, "base-io-stream");
  xobject_class_override_property (gobject_class, PROP_CONN_USE_SYSTEM_CERTDB, "use-system-certdb");
  xobject_class_override_property (gobject_class, PROP_CONN_REQUIRE_CLOSE_NOTIFY, "require-close-notify");
  xobject_class_override_property (gobject_class, PROP_CONN_REHANDSHAKE_MODE, "rehandshake-mode");
  xobject_class_override_property (gobject_class, PROP_CONN_CERTIFICATE, "certificate");
  xobject_class_override_property (gobject_class, PROP_CONN_DATABASE, "database");
  xobject_class_override_property (gobject_class, PROP_CONN_INTERACTION, "interaction");
  xobject_class_override_property (gobject_class, PROP_CONN_PEER_CERTIFICATE, "peer-certificate");
  xobject_class_override_property (gobject_class, PROP_CONN_PEER_CERTIFICATE_ERRORS, "peer-certificate-errors");
  xobject_class_override_property (gobject_class, PROP_CONN_VALIDATION_FLAGS, "validation-flags");
  xobject_class_override_property (gobject_class, PROP_CONN_SERVER_IDENTITY, "server-identity");
  xobject_class_override_property (gobject_class, PROP_CONN_USE_SSL3, "use-ssl3");
  xobject_class_override_property (gobject_class, PROP_CONN_ACCEPTED_CAS, "accepted-cas");
  xobject_class_override_property (gobject_class, PROP_CONN_AUTHENTICATION_MODE, "authentication-mode");
  xobject_class_override_property (gobject_class, PROP_CONN_ADVERTISED_PROTOCOLS, "advertised-protocols");
  xobject_class_override_property (gobject_class, PROP_CONN_NEGOTIATED_PROTOCOL, "negotiated-protocol");
}

static void
xdummy_tls_connection_init (GDummyTlsConnection *connection)
{
}

static xboolean_t
xdummy_tls_connection_initable_init (xinitable_t       *initable,
				      xcancellable_t    *cancellable,
				      xerror_t         **error)
{
  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_UNAVAILABLE,
		       _("TLS support is not available"));
  return FALSE;
}

static void
xdummy_tls_connection_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = xdummy_tls_connection_initable_init;
}

/* Dummy DTLS connection type; since xdtls_client_connection_t and
 * xdtls_server_connection_t are just interfaces, we can implement them
 * both on a single object.
 */

typedef struct _GDummyDtlsConnection      GDummyDtlsConnection;
typedef struct _GDummyDtlsConnectionClass GDummyDtlsConnectionClass;

struct _GDummyDtlsConnection {
  xobject_t parent_instance;
};

struct _GDummyDtlsConnectionClass {
  xobject_class_t parent_class;
};

enum
{
  PROP_DTLS_CONN_BASE_SOCKET = 1,
  PROP_DTLS_CONN_REQUIRE_CLOSE_NOTIFY,
  PROP_DTLS_CONN_REHANDSHAKE_MODE,
  PROP_DTLS_CONN_CERTIFICATE,
  PROP_DTLS_CONN_DATABASE,
  PROP_DTLS_CONN_INTERACTION,
  PROP_DTLS_CONN_PEER_CERTIFICATE,
  PROP_DTLS_CONN_PEER_CERTIFICATE_ERRORS,
  PROP_DTLS_CONN_VALIDATION_FLAGS,
  PROP_DTLS_CONN_SERVER_IDENTITY,
  PROP_DTLS_CONN_ENABLE_NEGOTIATION,
  PROP_DTLS_CONN_ACCEPTED_CAS,
  PROP_DTLS_CONN_AUTHENTICATION_MODE,
};

static void g_dummy_dtls_connection_initable_iface_init (xinitable_iface_t *iface);

#define g_dummy_dtls_connection_get_type _xdummy_dtls_connection_get_type
G_DEFINE_TYPE_WITH_CODE (GDummyDtlsConnection, g_dummy_dtls_connection, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_DTLS_CONNECTION, NULL);
                         G_IMPLEMENT_INTERFACE (XTYPE_DTLS_CLIENT_CONNECTION, NULL);
                         G_IMPLEMENT_INTERFACE (XTYPE_DTLS_SERVER_CONNECTION, NULL);
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                g_dummy_dtls_connection_initable_iface_init);)

static void
g_dummy_dtls_connection_get_property (xobject_t    *object,
                                      xuint_t       prop_id,
                                      xvalue_t     *value,
                                      xparam_spec_t *pspec)
{
}

static void
g_dummy_dtls_connection_set_property (xobject_t      *object,
                                      xuint_t         prop_id,
                                      const xvalue_t *value,
                                      xparam_spec_t   *pspec)
{
}

static void
g_dummy_dtls_connection_class_init (GDummyDtlsConnectionClass *connection_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (connection_class);

  gobject_class->get_property = g_dummy_dtls_connection_get_property;
  gobject_class->set_property = g_dummy_dtls_connection_set_property;

  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_BASE_SOCKET, "base-socket");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_REQUIRE_CLOSE_NOTIFY, "require-close-notify");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_REHANDSHAKE_MODE, "rehandshake-mode");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_CERTIFICATE, "certificate");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_DATABASE, "database");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_INTERACTION, "interaction");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_PEER_CERTIFICATE, "peer-certificate");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_PEER_CERTIFICATE_ERRORS, "peer-certificate-errors");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_VALIDATION_FLAGS, "validation-flags");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_SERVER_IDENTITY, "server-identity");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_ACCEPTED_CAS, "accepted-cas");
  xobject_class_override_property (gobject_class, PROP_DTLS_CONN_AUTHENTICATION_MODE, "authentication-mode");
}

static void
g_dummy_dtls_connection_init (GDummyDtlsConnection *connection)
{
}

static xboolean_t
g_dummy_dtls_connection_initable_init (xinitable_t       *initable,
                                       xcancellable_t    *cancellable,
                                       xerror_t         **error)
{
  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_UNAVAILABLE,
                       _("DTLS support is not available"));
  return FALSE;
}

static void
g_dummy_dtls_connection_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = g_dummy_dtls_connection_initable_init;
}

/* Dummy database type.
 */

typedef struct _GDummyTlsDatabase      GDummyTlsDatabase;
typedef struct _GDummyTlsDatabaseClass GDummyTlsDatabaseClass;

struct _GDummyTlsDatabase {
  xtls_database_t parent_instance;
};

struct _GDummyTlsDatabaseClass {
  GTlsDatabaseClass parent_class;
};

enum
{
  PROP_DATABASE_0,

  PROP_ANCHORS,
};

static void xdummy_tls_database_file_database_iface_init (xtls_file_database_interface_t *iface);
static void xdummy_tls_database_initable_iface_init (xinitable_iface_t *iface);

#define xdummy_tls_database_get_type _xdummy_tls_database_get_type
G_DEFINE_TYPE_WITH_CODE (GDummyTlsDatabase, xdummy_tls_database, XTYPE_TLS_DATABASE,
                         G_IMPLEMENT_INTERFACE (XTYPE_TLS_FILE_DATABASE,
                                                xdummy_tls_database_file_database_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xdummy_tls_database_initable_iface_init))


static void
xdummy_tls_database_get_property (xobject_t    *object,
                                   xuint_t       prop_id,
                                   xvalue_t     *value,
                                   xparam_spec_t *pspec)
{
  /* We need to define this method to make xobject_t happy, but it will
   * never be possible to construct a working GDummyTlsDatabase, so
   * it doesn't have to do anything useful.
   */
}

static void
xdummy_tls_database_set_property (xobject_t      *object,
                                   xuint_t         prop_id,
                                   const xvalue_t *value,
                                   xparam_spec_t   *pspec)
{
  /* Just ignore all attempts to set properties. */
}

static void
xdummy_tls_database_class_init (GDummyTlsDatabaseClass *database_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (database_class);

  gobject_class->get_property = xdummy_tls_database_get_property;
  gobject_class->set_property = xdummy_tls_database_set_property;

  xobject_class_override_property (gobject_class, PROP_ANCHORS, "anchors");
}

static void
xdummy_tls_database_init (GDummyTlsDatabase *database)
{
}

static void
xdummy_tls_database_file_database_iface_init (xtls_file_database_interface_t  *iface)
{
}

static xboolean_t
xdummy_tls_database_initable_init (xinitable_t       *initable,
                                    xcancellable_t    *cancellable,
                                    xerror_t         **error)
{
  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_UNAVAILABLE,
                       _("TLS support is not available"));
  return FALSE;
}

static void
xdummy_tls_database_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = xdummy_tls_database_initable_init;
}
