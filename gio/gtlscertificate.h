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

#ifndef __G_TLS_CERTIFICATE_H__
#define __G_TLS_CERTIFICATE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_TLS_CERTIFICATE            (g_tls_certificate_get_type ())
#define G_TLS_CERTIFICATE(inst)           (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TLS_CERTIFICATE, GTlsCertificate))
#define G_TLS_CERTIFICATE_CLASS(class)    (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_TLS_CERTIFICATE, GTlsCertificateClass))
#define X_IS_TLS_CERTIFICATE(inst)        (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TLS_CERTIFICATE))
#define X_IS_TLS_CERTIFICATE_CLASS(class) (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_TLS_CERTIFICATE))
#define G_TLS_CERTIFICATE_GET_CLASS(inst) (XTYPE_INSTANCE_GET_CLASS ((inst), XTYPE_TLS_CERTIFICATE, GTlsCertificateClass))

typedef struct _GTlsCertificateClass   GTlsCertificateClass;
typedef struct _GTlsCertificatePrivate GTlsCertificatePrivate;

struct _GTlsCertificate {
  xobject_t parent_instance;

  GTlsCertificatePrivate *priv;
};

struct _GTlsCertificateClass
{
  xobject_class_t parent_class;

  GTlsCertificateFlags  (* verify) (GTlsCertificate     *cert,
				    GSocketConnectable  *identity,
				    GTlsCertificate     *trusted_ca);

  /*< private >*/
  /* Padding for future expansion */
  xpointer_t padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t                 g_tls_certificate_get_type           (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GTlsCertificate      *g_tls_certificate_new_from_pem       (const xchar_t         *data,
							    gssize               length,
							    xerror_t             **error);
XPL_AVAILABLE_IN_2_72
GTlsCertificate      *g_tls_certificate_new_from_pkcs12      (const guint8      *data,
                                                              xsize_t              length,
                                                              const xchar_t       *password,
                                                              xerror_t           **error);
XPL_AVAILABLE_IN_2_72
GTlsCertificate      *g_tls_certificate_new_from_file_with_password (const xchar_t  *file,
                                                                     const xchar_t  *password,
                                                                     xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GTlsCertificate      *g_tls_certificate_new_from_file      (const xchar_t         *file,
							    xerror_t             **error);
XPL_AVAILABLE_IN_ALL
GTlsCertificate      *g_tls_certificate_new_from_files     (const xchar_t         *cert_file,
							    const xchar_t         *key_file,
							    xerror_t             **error);
XPL_AVAILABLE_IN_2_68
GTlsCertificate      *g_tls_certificate_new_from_pkcs11_uris (const xchar_t       *pkcs11_uri,
                                                              const xchar_t       *private_key_pkcs11_uri,
                                                              xerror_t           **error);

XPL_AVAILABLE_IN_ALL
xlist_t                *g_tls_certificate_list_new_from_file (const xchar_t         *file,
							    xerror_t             **error);

XPL_AVAILABLE_IN_ALL
GTlsCertificate      *g_tls_certificate_get_issuer         (GTlsCertificate     *cert);

XPL_AVAILABLE_IN_ALL
GTlsCertificateFlags  g_tls_certificate_verify             (GTlsCertificate     *cert,
							    GSocketConnectable  *identity,
							    GTlsCertificate     *trusted_ca);

XPL_AVAILABLE_IN_2_34
xboolean_t              g_tls_certificate_is_same            (GTlsCertificate     *cert_one,
                                                            GTlsCertificate     *cert_two);

XPL_AVAILABLE_IN_2_70
GDateTime            *g_tls_certificate_get_not_valid_before (GTlsCertificate     *cert);

XPL_AVAILABLE_IN_2_70
GDateTime            *g_tls_certificate_get_not_valid_after  (GTlsCertificate     *cert);

XPL_AVAILABLE_IN_2_70
xchar_t                *g_tls_certificate_get_subject_name     (GTlsCertificate     *cert);

XPL_AVAILABLE_IN_2_70
xchar_t                *g_tls_certificate_get_issuer_name      (GTlsCertificate     *cert);

XPL_AVAILABLE_IN_2_70
GPtrArray            *g_tls_certificate_get_dns_names        (GTlsCertificate     *cert);

XPL_AVAILABLE_IN_2_70
GPtrArray            *g_tls_certificate_get_ip_addresses     (GTlsCertificate     *cert);

G_END_DECLS

#endif /* __G_TLS_CERTIFICATE_H__ */
