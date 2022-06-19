/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2011 Collabora Ltd.
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

#include "gtesttlsbackend.h"

#include <glib.h>

static xtype_t _g_test_tls_certificate_get_type (void);
static xtype_t _g_test_tls_connection_get_type (void);
static xtls_database_t * _g_test_tls_backend_get_default_database (xtls_backend_t * backend);
static xtype_t _g_test_tls_database_get_type (void);

struct _GTestTlsBackend {
  xobject_t parent_instance;
};

static void g_test_tls_backend_iface_init (xtls_backend_interface_t *iface);

#define g_test_tls_backend_get_type _g_test_tls_backend_get_type
G_DEFINE_TYPE_WITH_CODE (GTestTlsBackend, g_test_tls_backend, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_BACKEND,
						g_test_tls_backend_iface_init)
                         g_io_extension_point_set_required_type (
                           g_io_extension_point_register (G_TLS_BACKEND_EXTENSION_POINT_NAME),
                           XTYPE_TLS_BACKEND);
			 g_io_extension_point_implement (G_TLS_BACKEND_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "test",
							 999);)

static void
g_test_tls_backend_init (GTestTlsBackend *backend)
{
}

static void
g_test_tls_backend_class_init (GTestTlsBackendClass *backend_class)
{
}

static void
g_test_tls_backend_iface_init (xtls_backend_interface_t *iface)
{
  iface->get_certificate_type = _g_test_tls_certificate_get_type;
  iface->get_client_connection_type = _g_test_tls_connection_get_type;
  iface->get_server_connection_type = _g_test_tls_connection_get_type;
  iface->get_dtls_client_connection_type = _g_test_tls_connection_get_type;
  iface->get_dtls_server_connection_type = _g_test_tls_connection_get_type;
  iface->get_default_database = _g_test_tls_backend_get_default_database;
  iface->get_file_database_type = _g_test_tls_database_get_type;
}

static xtls_database_t *
_g_test_tls_backend_get_default_database (xtls_backend_t * backend)
{
  static xtls_database_t *default_db;
  xerror_t *error = NULL;

  if (!default_db)
    {
      default_db = xinitable_new (_g_test_tls_database_get_type (),
                                   NULL,
                                   &error,
                                   NULL);
      g_assert_no_error (error);
    }

  return default_db;
}

/* test_t certificate type */

typedef struct _GTestTlsCertificate      GTestTlsCertificate;
typedef struct _GTestTlsCertificateClass GTestTlsCertificateClass;

struct _GTestTlsCertificate {
  xtls_certificate_t parent_instance;
  xchar_t *key_pem;
  xchar_t *cert_pem;
  xtls_certificate_t *issuer;
  xchar_t *pkcs11_uri;
  xchar_t *private_key_pkcs11_uri;
};

struct _GTestTlsCertificateClass {
  GTlsCertificateClass parent_class;
};

enum
{
  PROP_CERT_CERTIFICATE = 1,
  PROP_CERT_CERTIFICATE_PEM,
  PROP_CERT_PRIVATE_KEY,
  PROP_CERT_PRIVATE_KEY_PEM,
  PROP_CERT_ISSUER,
  PROP_CERT_PKCS11_URI,
  PROP_CERT_PRIVATE_KEY_PKCS11_URI,
  PROP_CERT_NOT_VALID_BEFORE,
  PROP_CERT_NOT_VALID_AFTER,
  PROP_CERT_SUBJECT_NAME,
  PROP_CERT_ISSUER_NAME,
  PROP_CERT_DNS_NAMES,
  PROP_CERT_IP_ADDRESSES,
};

static void g_test_tls_certificate_initable_iface_init (xinitable_iface_t *iface);

#define g_test_tls_certificate_get_type _g_test_tls_certificate_get_type
G_DEFINE_TYPE_WITH_CODE (GTestTlsCertificate, g_test_tls_certificate, XTYPE_TLS_CERTIFICATE,
			 G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
						g_test_tls_certificate_initable_iface_init))

static xtls_certificate_flags_t
g_test_tls_certificate_verify (xtls_certificate_t     *cert,
                               xsocket_connectable_t  *identity,
                               xtls_certificate_t     *trusted_ca)
{
  /* For now, all of the tests expect the certificate to verify */
  return 0;
}

static void
g_test_tls_certificate_get_property (xobject_t    *object,
				      xuint_t       prop_id,
				      xvalue_t     *value,
				      xparam_spec_t *pspec)
{
  GTestTlsCertificate *cert = (GTestTlsCertificate *) object;
  xptr_array_t *data = NULL;
  const xchar_t *dns_name = "a.example.com";

  switch (prop_id)
    {
    case PROP_CERT_CERTIFICATE_PEM:
      xvalue_set_string (value, cert->cert_pem);
      break;
    case PROP_CERT_PRIVATE_KEY_PEM:
      xvalue_set_string (value, cert->key_pem);
      break;
    case PROP_CERT_ISSUER:
      xvalue_set_object (value, cert->issuer);
      break;
    case PROP_CERT_PKCS11_URI:
      /* This test value simulates a backend that ignores the value
         because it is unsupported */
      if (xstrcmp0 (cert->pkcs11_uri, "unsupported") != 0)
        xvalue_set_string (value, cert->pkcs11_uri);
      break;
    case PROP_CERT_PRIVATE_KEY_PKCS11_URI:
      xvalue_set_string (value, cert->private_key_pkcs11_uri);
      break;
    case PROP_CERT_NOT_VALID_BEFORE:
      xvalue_take_boxed (value, xdate_time_new_from_iso8601 ("2020-10-12T17:49:44Z", NULL));
      break;
    case PROP_CERT_NOT_VALID_AFTER:
      xvalue_take_boxed (value, xdate_time_new_from_iso8601 ("2045-10-06T17:49:44Z", NULL));
      break;
    case PROP_CERT_SUBJECT_NAME:
      xvalue_set_string (value, "DC=COM,DC=EXAMPLE,CN=server.example.com");
      break;
    case PROP_CERT_ISSUER_NAME:
      xvalue_set_string (value, "DC=COM,DC=EXAMPLE,OU=Certificate Authority,CN=ca.example.com,emailAddress=ca@example.com");
      break;
    case PROP_CERT_DNS_NAMES:
      data = xptr_array_new_with_free_func ((xdestroy_notify_t)xbytes_unref);
      xptr_array_add (data, xbytes_new_static (dns_name, strlen (dns_name)));
      xvalue_take_boxed (value, data);
      break;
    case PROP_CERT_IP_ADDRESSES:
      data = xptr_array_new_with_free_func (xobject_unref);
      xptr_array_add (data, xinet_address_new_from_string ("192.0.2.1"));
      xvalue_take_boxed (value, data);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
g_test_tls_certificate_set_property (xobject_t      *object,
				      xuint_t         prop_id,
				      const xvalue_t *value,
				      xparam_spec_t   *pspec)
{
  GTestTlsCertificate *cert = (GTestTlsCertificate *) object;

  switch (prop_id)
    {
    case PROP_CERT_CERTIFICATE_PEM:
      cert->cert_pem = xvalue_dup_string (value);
      break;
    case PROP_CERT_PRIVATE_KEY_PEM:
      cert->key_pem = xvalue_dup_string (value);
      break;
    case PROP_CERT_ISSUER:
      cert->issuer = xvalue_dup_object (value);
      break;
    case PROP_CERT_PKCS11_URI:
      cert->pkcs11_uri = xvalue_dup_string (value);
      break;
    case PROP_CERT_PRIVATE_KEY_PKCS11_URI:
      cert->private_key_pkcs11_uri = xvalue_dup_string (value);
      break;
    case PROP_CERT_CERTIFICATE:
    case PROP_CERT_PRIVATE_KEY:
      /* ignore */
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
g_test_tls_certificate_finalize (xobject_t *object)
{
  GTestTlsCertificate *cert = (GTestTlsCertificate *) object;

  g_free (cert->cert_pem);
  g_free (cert->key_pem);
  g_free (cert->pkcs11_uri);
  g_free (cert->private_key_pkcs11_uri);
  g_clear_object (&cert->issuer);

  XOBJECT_CLASS (g_test_tls_certificate_parent_class)->finalize (object);
}

static void
g_test_tls_certificate_class_init (GTestTlsCertificateClass *test_class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (test_class);
  GTlsCertificateClass *certificate_class = G_TLS_CERTIFICATE_CLASS (test_class);

  xobject_class->get_property = g_test_tls_certificate_get_property;
  xobject_class->set_property = g_test_tls_certificate_set_property;
  xobject_class->finalize = g_test_tls_certificate_finalize;

  certificate_class->verify = g_test_tls_certificate_verify;

  xobject_class_override_property (xobject_class, PROP_CERT_CERTIFICATE, "certificate");
  xobject_class_override_property (xobject_class, PROP_CERT_CERTIFICATE_PEM, "certificate-pem");
  xobject_class_override_property (xobject_class, PROP_CERT_PRIVATE_KEY, "private-key");
  xobject_class_override_property (xobject_class, PROP_CERT_PRIVATE_KEY_PEM, "private-key-pem");
  xobject_class_override_property (xobject_class, PROP_CERT_ISSUER, "issuer");
  xobject_class_override_property (xobject_class, PROP_CERT_PKCS11_URI, "pkcs11-uri");
  xobject_class_override_property (xobject_class, PROP_CERT_PRIVATE_KEY_PKCS11_URI, "private-key-pkcs11-uri");
  xobject_class_override_property (xobject_class, PROP_CERT_NOT_VALID_BEFORE, "not-valid-before");
  xobject_class_override_property (xobject_class, PROP_CERT_NOT_VALID_AFTER, "not-valid-after");
  xobject_class_override_property (xobject_class, PROP_CERT_SUBJECT_NAME, "subject-name");
  xobject_class_override_property (xobject_class, PROP_CERT_ISSUER_NAME, "issuer-name");
  xobject_class_override_property (xobject_class, PROP_CERT_DNS_NAMES, "dns-names");
  xobject_class_override_property (xobject_class, PROP_CERT_IP_ADDRESSES, "ip-addresses");
}

static void
g_test_tls_certificate_init (GTestTlsCertificate *certificate)
{
}

static xboolean_t
g_test_tls_certificate_initable_init (xinitable_t       *initable,
				       xcancellable_t    *cancellable,
				       xerror_t         **error)
{
  return TRUE;
}

static void
g_test_tls_certificate_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = g_test_tls_certificate_initable_init;
}

/* Dummy connection type; since xtls_client_connection_t and
 * xtls_server_connection_t are just interfaces, we can implement them
 * both on a single object.
 */

typedef struct _GTestTlsConnection      GTestTlsConnection;
typedef struct _GTestTlsConnectionClass GTestTlsConnectionClass;

struct _GTestTlsConnection {
  xtls_connection_t parent_instance;
};

struct _GTestTlsConnectionClass {
  GTlsConnectionClass parent_class;
};

enum
{
  PROP_CONN_BASE_IO_STREAM = 1,
  PROP_CONN_BASE_SOCKET,
  PROP_CONN_USE_SYSTEM_CERTDB,
  PROP_CONN_REQUIRE_CLOSE_NOTIFY,
  PROP_CONN_REHANDSHAKE_MODE,
  PROP_CONN_CERTIFICATE,
  PROP_CONN_PEER_CERTIFICATE,
  PROP_CONN_PEER_CERTIFICATE_ERRORS,
  PROP_CONN_VALIDATION_FLAGS,
  PROP_CONN_SERVER_IDENTITY,
  PROP_CONN_USE_SSL3,
  PROP_CONN_ACCEPTED_CAS,
  PROP_CONN_AUTHENTICATION_MODE
};

static void g_test_tls_connection_initable_iface_init (xinitable_iface_t *iface);

#define g_test_tls_connection_get_type _g_test_tls_connection_get_type
G_DEFINE_TYPE_WITH_CODE (GTestTlsConnection, g_test_tls_connection, XTYPE_TLS_CONNECTION,
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_CLIENT_CONNECTION, NULL)
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_SERVER_CONNECTION, NULL)
                         G_IMPLEMENT_INTERFACE (XTYPE_DATAGRAM_BASED, NULL)
			 G_IMPLEMENT_INTERFACE (XTYPE_DTLS_CONNECTION, NULL)
			 G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
						g_test_tls_connection_initable_iface_init))

static void
g_test_tls_connection_get_property (xobject_t    *object,
				     xuint_t       prop_id,
				     xvalue_t     *value,
				     xparam_spec_t *pspec)
{
}

static void
g_test_tls_connection_set_property (xobject_t      *object,
				     xuint_t         prop_id,
				     const xvalue_t *value,
				     xparam_spec_t   *pspec)
{
}

static xboolean_t
g_test_tls_connection_close (xio_stream_t     *stream,
			      xcancellable_t  *cancellable,
			      xerror_t       **error)
{
  return TRUE;
}

static void
g_test_tls_connection_class_init (GTestTlsConnectionClass *connection_class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (connection_class);
  xio_stream_class_t *io_stream_class = XIO_STREAM_CLASS (connection_class);

  xobject_class->get_property = g_test_tls_connection_get_property;
  xobject_class->set_property = g_test_tls_connection_set_property;

  /* Need to override this because when initable_init fails it will
   * dispose the connection, which will close it, which would
   * otherwise try to close its input/output streams, which don't
   * exist.
   */
  io_stream_class->close_fn = g_test_tls_connection_close;

  xobject_class_override_property (xobject_class, PROP_CONN_BASE_IO_STREAM, "base-io-stream");
  xobject_class_override_property (xobject_class, PROP_CONN_BASE_SOCKET, "base-socket");
  xobject_class_override_property (xobject_class, PROP_CONN_USE_SYSTEM_CERTDB, "use-system-certdb");
  xobject_class_override_property (xobject_class, PROP_CONN_REQUIRE_CLOSE_NOTIFY, "require-close-notify");
  xobject_class_override_property (xobject_class, PROP_CONN_REHANDSHAKE_MODE, "rehandshake-mode");
  xobject_class_override_property (xobject_class, PROP_CONN_CERTIFICATE, "certificate");
  xobject_class_override_property (xobject_class, PROP_CONN_PEER_CERTIFICATE, "peer-certificate");
  xobject_class_override_property (xobject_class, PROP_CONN_PEER_CERTIFICATE_ERRORS, "peer-certificate-errors");
  xobject_class_override_property (xobject_class, PROP_CONN_VALIDATION_FLAGS, "validation-flags");
  xobject_class_override_property (xobject_class, PROP_CONN_SERVER_IDENTITY, "server-identity");
  xobject_class_override_property (xobject_class, PROP_CONN_USE_SSL3, "use-ssl3");
  xobject_class_override_property (xobject_class, PROP_CONN_ACCEPTED_CAS, "accepted-cas");
  xobject_class_override_property (xobject_class, PROP_CONN_AUTHENTICATION_MODE, "authentication-mode");
}

static void
g_test_tls_connection_init (GTestTlsConnection *connection)
{
}

static xboolean_t
g_test_tls_connection_initable_init (xinitable_t       *initable,
				      xcancellable_t    *cancellable,
				      xerror_t         **error)
{
  g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_UNAVAILABLE,
		       "TLS Connection support is not available");
  return FALSE;
}

static void
g_test_tls_connection_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = g_test_tls_connection_initable_init;
}

/* test_t database type */

typedef struct _GTestTlsDatabase      GTestTlsDatabase;
typedef struct _GTestTlsDatabaseClass GTestTlsDatabaseClass;

struct _GTestTlsDatabase {
  xtls_database_t parent_instance;
  xchar_t *anchors;
};

struct _GTestTlsDatabaseClass {
  GTlsDatabaseClass parent_class;
};

enum
{
  PROP_DATABASE_ANCHORS = 1,
};

static void g_test_tls_database_initable_iface_init (xinitable_iface_t *iface);
static void g_test_tls_file_database_file_database_interface_init (xinitable_iface_t *iface);

#define g_test_tls_database_get_type _g_test_tls_database_get_type
G_DEFINE_TYPE_WITH_CODE (GTestTlsDatabase, g_test_tls_database, XTYPE_TLS_DATABASE,
			 G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
						g_test_tls_database_initable_iface_init);
			 G_IMPLEMENT_INTERFACE (XTYPE_TLS_FILE_DATABASE,
						g_test_tls_file_database_file_database_interface_init))

static void
g_test_tls_database_get_property (xobject_t    *object,
                                  xuint_t       prop_id,
                                  xvalue_t     *value,
                                  xparam_spec_t *pspec)
{
  GTestTlsDatabase *db = (GTestTlsDatabase *) object;

  switch (prop_id)
    {
    case PROP_DATABASE_ANCHORS:
      xvalue_set_string (value, db->anchors);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
g_test_tls_database_set_property (xobject_t      *object,
                                  xuint_t         prop_id,
                                  const xvalue_t *value,
                                  xparam_spec_t   *pspec)
{
  GTestTlsDatabase *db = (GTestTlsDatabase *) object;

  switch (prop_id)
    {
    case PROP_DATABASE_ANCHORS:
      g_free (db->anchors);
      db->anchors = xvalue_dup_string (value);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
g_test_tls_database_finalize (xobject_t *object)
{
  GTestTlsDatabase *db = (GTestTlsDatabase *) object;

  g_free (db->anchors);

  XOBJECT_CLASS (g_test_tls_database_parent_class)->finalize (object);
}

static void
g_test_tls_database_class_init (GTestTlsDatabaseClass *test_class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (test_class);

  xobject_class->get_property = g_test_tls_database_get_property;
  xobject_class->set_property = g_test_tls_database_set_property;
  xobject_class->finalize = g_test_tls_database_finalize;

  xobject_class_override_property (xobject_class, PROP_DATABASE_ANCHORS, "anchors");
}

static void
g_test_tls_database_init (GTestTlsDatabase *database)
{
}

static xboolean_t
g_test_tls_database_initable_init (xinitable_t       *initable,
                                   xcancellable_t    *cancellable,
                                   xerror_t         **error)
{
  return TRUE;
}

static void
g_test_tls_file_database_file_database_interface_init (xinitable_iface_t *iface)
{
}

static void
g_test_tls_database_initable_iface_init (xinitable_iface_t  *iface)
{
  iface->init = g_test_tls_database_initable_init;
}
