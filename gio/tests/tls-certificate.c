/* GLib testing framework examples and tests
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
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
 */

#include "config.h"

#include <gio/gio.h>

#include "gtesttlsbackend.h"

typedef struct
{
  xchar_t *cert_pems[3];
  xchar_t *cert_crlf_pem;
  xchar_t *key_pem;
  xchar_t *key_crlf_pem;
  xchar_t *key8_pem;
} Reference;

static void
pem_parser (const Reference *ref)
{
  xtls_certificate_t *cert;
  xchar_t *pem;
  xsize_t pem_len = 0;
  xchar_t *parsed_cert_pem = NULL;
  xchar_t *parsed_key_pem = NULL;
  xerror_t *error = NULL;

  /* Check PEM parsing in certificate, private key order. */
  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert-key.pem", NULL), &pem, &pem_len, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pem);
  g_assert_cmpuint (pem_len, >=, 10);

  cert = xtls_certificate_new_from_pem (pem, -1, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_cmpstr (parsed_key_pem, ==, ref->key_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  xobject_unref (cert);

  /* Make sure length is respected and parser detect invalid PEM
   * when cert is truncated. */
  cert = xtls_certificate_new_from_pem (pem, 10, &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);

  /* Make sure length is respected and parser detect invalid PEM
   * when cert exists but key is truncated. */
  cert = xtls_certificate_new_from_pem (pem, pem_len - 10, &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_free (pem);

  /* Check PEM parsing in private key, certificate order */
  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "cert-tests", "key-cert.pem", NULL), &pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pem);

  cert = xtls_certificate_new_from_pem (pem, -1, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_cmpstr (parsed_key_pem, ==, ref->key_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  g_free (pem);
  xobject_unref (cert);

  /* Check certificate only PEM */
  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL), &pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pem);

  cert = xtls_certificate_new_from_pem (pem, -1, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_null (parsed_key_pem);

  g_free (pem);
  xobject_unref (cert);

  /* Check error with private key only PEM */
  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "cert-tests", "key.pem", NULL), &pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pem);

  cert = xtls_certificate_new_from_pem (pem, -1, &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_assert_null (cert);
  g_free (pem);
}

static void
pem_parser_handles_chain (const Reference *ref)
{
  xtls_certificate_t *cert;
  xtls_certificate_t *issuer;
  xtls_certificate_t *original_cert;
  xchar_t *pem;
  xchar_t *parsed_cert_pem = NULL;
  xchar_t *parsed_key_pem = NULL;
  xerror_t *error = NULL;

  /* Check that a chain with exactly three certificates is returned */
  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert-list.pem", NULL), &pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pem);

  cert = original_cert = xtls_certificate_new_from_pem (pem, -1, &error);
  g_free (pem);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);

  /* Make sure the private key was parsed */
  g_assert_cmpstr (parsed_key_pem, ==, ref->key_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  /* Now test the second cert */
  issuer = xtls_certificate_get_issuer (cert);
  g_assert_nonnull (issuer);

  cert = issuer;
  issuer = xtls_certificate_get_issuer (cert);
  g_assert_nonnull (issuer);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[1]);
  g_clear_pointer (&parsed_cert_pem, g_free);

  /* Only the first cert should have a private key */
  g_assert_null (parsed_key_pem);

  /* Now test the final cert */
  cert = issuer;
  issuer = xtls_certificate_get_issuer (cert);
  g_assert_null (issuer);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[2]);
  g_clear_pointer (&parsed_cert_pem, g_free);

  g_assert_null (parsed_key_pem);

  xobject_unref (original_cert);
}

static void
pem_parser_no_sentinel (void)
{
  xtls_certificate_t *cert;
  xchar_t *pem;
  xsize_t pem_len = 0;
  xchar_t *pem_copy;
  xerror_t *error = NULL;

  /* Check certificate from not-nul-terminated PEM */
  xfile_get_contents (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL), &pem, &pem_len, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pem);
  g_assert_cmpuint (pem_len, >=, 10);

  pem_copy = g_new (char, pem_len);
  /* Do not copy the terminating nul: */
  memmove (pem_copy, pem, pem_len);
  g_free (pem);

  /* Check whether the parser respects the @length parameter.
   * pem_copy is allocated exactly pem_len bytes, so accessing memory
   * outside its bounds will be detected by, for example, valgrind or
   * asan. */
  cert = xtls_certificate_new_from_pem (pem_copy, pem_len, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  g_free (pem_copy);
  xobject_unref (cert);
}

static void
from_file (const Reference *ref)
{
  xtls_certificate_t *cert;
  xchar_t *parsed_cert_pem = NULL;
  xchar_t *parsed_key_pem = NULL;
  xerror_t *error = NULL;

  cert = xtls_certificate_new_from_file (g_test_get_filename (G_TEST_DIST, "cert-tests", "key-cert.pem", NULL),
                                          &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_cmpstr (parsed_key_pem, ==, ref->key_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  xobject_unref (cert);
}

static void
from_files (const Reference *ref)
{
  xtls_certificate_t *cert;
  xchar_t *parsed_cert_pem = NULL;
  xchar_t *parsed_key_pem = NULL;
  xerror_t *error = NULL;

  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key.pem", NULL),
                                           &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_cmpstr (parsed_key_pem, ==, ref->key_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  xobject_unref (cert);

  /* Missing private key */
  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "cert2.pem", NULL),
                                           &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_assert_null (cert);

  /* Missing header private key */
  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key_missing-header.pem", NULL),
                                           &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_assert_null (cert);

  /* Missing footer private key */
  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key_missing-footer.pem", NULL),
                                           &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_assert_null (cert);

  /* Missing certificate */
  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "key.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key.pem", NULL),
                                           &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_assert_null (cert);

  /* Using this method twice with a file containing both private key and
   * certificate as a way to enforce private key presence is a fair use
   */
  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "key-cert.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key-cert.pem", NULL),
                                           &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);
  xobject_unref (cert);
}

static void
from_files_crlf (const Reference *ref)
{
  xtls_certificate_t *cert;
  xchar_t *parsed_cert_pem = NULL;
  xchar_t *parsed_key_pem = NULL;
  xerror_t *error = NULL;

  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert-crlf.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key-crlf.pem", NULL),
                                           &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_crlf_pem);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_cmpstr (parsed_key_pem, ==, ref->key_crlf_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  xobject_unref (cert);
}

static void
from_files_pkcs8 (const Reference *ref)
{
  xtls_certificate_t *cert;
  xchar_t *parsed_cert_pem = NULL;
  xchar_t *parsed_key_pem = NULL;
  xerror_t *error = NULL;

  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key8.pem", NULL),
                                           &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert,
      "certificate-pem", &parsed_cert_pem,
      "private-key-pem", &parsed_key_pem,
      NULL);
  g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[0]);
  g_clear_pointer (&parsed_cert_pem, g_free);
  g_assert_cmpstr (parsed_key_pem, ==, ref->key8_pem);
  g_clear_pointer (&parsed_key_pem, g_free);

  xobject_unref (cert);
}

static void
from_files_pkcs8enc (const Reference *ref)
{
  xtls_certificate_t *cert;
  xerror_t *error = NULL;

  /* Mare sure an error is returned for encrypted key */
  cert = xtls_certificate_new_from_files (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL),
                                           g_test_get_filename (G_TEST_DIST, "cert-tests", "key8enc.pem", NULL),
                                           &error);
  g_assert_error (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE);
  g_clear_error (&error);
  g_assert_null (cert);
}

static void
list_from_file (const Reference *ref)
{
  xlist_t *list, *l;
  xerror_t *error = NULL;
  int i;

  list = xtls_certificate_list_new_from_file (g_test_get_filename (G_TEST_DIST, "cert-tests", "cert-list.pem", NULL),
                                               &error);
  g_assert_no_error (error);
  g_assert_cmpint (xlist_length (list), ==, 3);

  l = list;
  for (i = 0; i < 3; i++)
    {
      xtls_certificate_t *cert = l->data;
      xchar_t *parsed_cert_pem = NULL;
      xobject_get (cert,
          "certificate-pem", &parsed_cert_pem,
          NULL);
      g_assert_cmpstr (parsed_cert_pem, ==, ref->cert_pems[i]);
      g_free (parsed_cert_pem);
      l = xlist_next (l);
    }

  xlist_free_full (list, xobject_unref);

  /* Empty list is not an error */
  list = xtls_certificate_list_new_from_file (g_test_get_filename (G_TEST_DIST, "cert-tests", "nothing.pem", NULL),
                                               &error);
  g_assert_no_error (error);
  g_assert_cmpint (xlist_length (list), ==, 0);
}

static void
from_pkcs11_uri (void)
{
  xerror_t *error = NULL;
  xtls_certificate_t *cert;
  xchar_t *pkcs11_uri = NULL;

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  xobject_get (cert, "pkcs11-uri", &pkcs11_uri, NULL);
  g_assert_cmpstr ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", ==, pkcs11_uri);
  g_free (pkcs11_uri);

  xobject_unref (cert);
}

static void
from_unsupported_pkcs11_uri (void)
{
  xerror_t *error = NULL;
  xtls_certificate_t *cert;

  /* This is a magic value in gtesttlsbackend.c simulating an unsupported backend */
  cert = xtls_certificate_new_from_pkcs11_uris ("unsupported", NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_assert_null (cert);

  g_clear_error (&error);
}

static void
not_valid_before (void)
{
  const xchar_t *EXPECTED_NOT_VALID_BEFORE = "2020-10-12T17:49:44Z";

  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  xdatetime_t *actual;
  xchar_t *actual_str;

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  actual = xtls_certificate_get_not_valid_before (cert);
  g_assert_nonnull (actual);
  actual_str = xdate_time_format_iso8601 (actual);
  g_assert_cmpstr (actual_str, ==, EXPECTED_NOT_VALID_BEFORE);
  g_free (actual_str);
  xdate_time_unref (actual);
  xobject_unref (cert);
}

static void
not_valid_after (void)
{
  const xchar_t *EXPECTED_NOT_VALID_AFTER = "2045-10-06T17:49:44Z";

  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  xdatetime_t *actual;
  xchar_t *actual_str;

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  actual = xtls_certificate_get_not_valid_after (cert);
  g_assert_nonnull (actual);
  actual_str = xdate_time_format_iso8601 (actual);
  g_assert_cmpstr (actual_str, ==, EXPECTED_NOT_VALID_AFTER);
  g_free (actual_str);
  xdate_time_unref (actual);
  xobject_unref (cert);
}

static void
subject_name (void)
{
  const xchar_t *EXPECTED_SUBJECT_NAME = "DC=COM,DC=EXAMPLE,CN=server.example.com";

  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  xchar_t *actual;

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  actual = xtls_certificate_get_subject_name (cert);
  g_assert_nonnull (actual);
  g_assert_cmpstr (actual, ==, EXPECTED_SUBJECT_NAME);
  g_free (actual);
  xobject_unref (cert);
}

static void
issuer_name (void)
{
  const xchar_t *EXPECTED_ISSUER_NAME = "DC=COM,DC=EXAMPLE,OU=Certificate Authority,CN=ca.example.com,emailAddress=ca@example.com";

  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  xchar_t *actual;

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  actual = xtls_certificate_get_issuer_name (cert);
  g_assert_nonnull (actual);
  g_assert_cmpstr (actual, ==, EXPECTED_ISSUER_NAME);
  g_free (actual);
  xobject_unref (cert);
}

static void
dns_names (void)
{
  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  xptr_array_t *actual;
  const xchar_t *dns_name = "a.example.com";
  xbytes_t *expected = xbytes_new_static (dns_name, strlen (dns_name));

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  actual = xtls_certificate_get_dns_names (cert);
  g_assert_nonnull (actual);
  g_assert_cmpuint (actual->len, ==, 1);
  g_assert_true (xptr_array_find_with_equal_func (actual, expected, (GEqualFunc)xbytes_equal, NULL));

  xptr_array_unref (actual);
  xbytes_unref (expected);
  xobject_unref (cert);
}

static void
ip_addresses (void)
{
  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  xptr_array_t *actual;
  xinet_address_t *expected = xinet_address_new_from_string ("192.0.2.1");

  cert = xtls_certificate_new_from_pkcs11_uris ("pkcs11:model=p11-kit-trust;manufacturer=PKCS%2311%20Kit;serial=1;token=ca-bundle.crt", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (cert);

  actual = xtls_certificate_get_ip_addresses (cert);
  g_assert_nonnull (actual);
  g_assert_cmpuint (actual->len, ==, 1);
  g_assert_true (xptr_array_find_with_equal_func (actual, expected, (GEqualFunc)xinet_address_equal, NULL));

  xptr_array_free (actual, TRUE);
  xobject_unref (expected);
  xobject_unref (cert);
}

static void
from_pkcs12 (void)
{
  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  const xuint8_t data[1] = { 0 };

  /* This simply fails because our test backend doesn't support this
   * property. This reflects using a backend that doesn't support it.
   * The real test lives in glib-networking. */
  cert = xtls_certificate_new_from_pkcs12 (data, 1, NULL, &error);

  g_assert_null (cert);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  xerror_free (error);
}

static void
from_pkcs12_file (void)
{
  xtls_certificate_t *cert;
  xerror_t *error = NULL;
  char *path = g_test_build_filename (G_TEST_DIST, "cert-tests", "key-cert-password-123.p12", NULL);

  /* Fails on our test backend, see from_pkcs12() above. */
  cert = xtls_certificate_new_from_file_with_password (path, "123", &error);
  g_assert_null (cert);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  /* Just for coverage. */
  cert = xtls_certificate_new_from_file (path, &error);
  g_assert_null (cert);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  xerror_free (error);

  g_free (path);
}

int
main (int   argc,
      char *argv[])
{
  int rtv;
  Reference ref;
  xerror_t *error = NULL;
  xchar_t *path;

  g_test_init (&argc, &argv, NULL);

  _g_test_tls_backend_get_type ();

  /* Load reference PEM */
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "cert1.pem", NULL);
  xfile_get_contents (path, &ref.cert_pems[0], NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.cert_pems[0]);
  g_free (path);
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "cert2.pem", NULL);
  xfile_get_contents (path, &ref.cert_pems[1], NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.cert_pems[1]);
  g_free (path);
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "cert3.pem", NULL);
  xfile_get_contents (path, &ref.cert_pems[2], NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.cert_pems[2]);
  g_free (path);
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "cert-crlf.pem", NULL);
  xfile_get_contents (path, &ref.cert_crlf_pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.cert_crlf_pem);
  g_free (path);
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "key.pem", NULL);
  xfile_get_contents (path, &ref.key_pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.key_pem);
  g_free (path);
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "key-crlf.pem", NULL);
  xfile_get_contents (path, &ref.key_crlf_pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.key_crlf_pem);
  g_free (path);
  path = g_test_build_filename (G_TEST_DIST, "cert-tests", "key8.pem", NULL);
  xfile_get_contents (path, &ref.key8_pem, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (ref.key8_pem);
  g_free (path);

  g_test_add_data_func ("/tls-certificate/pem-parser",
                        &ref, (GTestDataFunc)pem_parser);
  g_test_add_data_func ("/tls-certificate/pem-parser-handles-chain",
                        &ref, (GTestDataFunc)pem_parser_handles_chain);
  g_test_add_data_func ("/tls-certificate/from_file",
                        &ref, (GTestDataFunc)from_file);
  g_test_add_data_func ("/tls-certificate/from_files",
                        &ref, (GTestDataFunc)from_files);
  g_test_add_data_func ("/tls-certificate/from_files_crlf",
                        &ref, (GTestDataFunc)from_files_crlf);
  g_test_add_data_func ("/tls-certificate/from_files_pkcs8",
                        &ref, (GTestDataFunc)from_files_pkcs8);
  g_test_add_data_func ("/tls-certificate/from_files_pkcs8enc",
                        &ref, (GTestDataFunc)from_files_pkcs8enc);
  g_test_add_data_func ("/tls-certificate/list_from_file",
                        &ref, (GTestDataFunc)list_from_file);
  g_test_add_func ("/tls-certificate/pkcs11-uri",
                   from_pkcs11_uri);
  g_test_add_func ("/tls-certificate/pkcs11-uri-unsupported",
                   from_unsupported_pkcs11_uri);
  g_test_add_func ("/tls-certificate/from_pkcs12",
                   from_pkcs12);
  g_test_add_func ("/tls-certificate/from_pkcs12_file",
                   from_pkcs12_file);
  g_test_add_func ("/tls-certificate/not-valid-before",
                   not_valid_before);
  g_test_add_func ("/tls-certificate/not-valid-after",
                   not_valid_after);
  g_test_add_func ("/tls-certificate/subject-name",
                   subject_name);
  g_test_add_func ("/tls-certificate/issuer-name",
                   issuer_name);
  g_test_add_func ("/tls-certificate/dns-names",
                   dns_names);
  g_test_add_func ("/tls-certificate/ip-addresses",
                   ip_addresses);
  g_test_add_func ("/tls-certificate/pem-parser-no-sentinel",
                   pem_parser_no_sentinel);

  rtv = g_test_run();

  g_free (ref.cert_pems[0]);
  g_free (ref.cert_pems[1]);
  g_free (ref.cert_pems[2]);
  g_free (ref.cert_crlf_pem);
  g_free (ref.key_pem);
  g_free (ref.key_crlf_pem);
  g_free (ref.key8_pem);

  return rtv;
}
