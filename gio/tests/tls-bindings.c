/*
 * Copyright 2020 (C) Ruslan N. Marchenko <me@ruff.mobi>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include <gio/gio.h>

#include "gtesttlsbackend.h"

static void
get_tls_channel_binding (void)
{
  xtls_backend_t *backend;
  xchar_t *not_null = "NOT_NULL";
  xtls_connection_t *tls = NULL;
  xerror_t *error = NULL;

  backend = xtls_backend_get_default ();
  g_assert_nonnull (backend);

  /* check unimplemented xtls_connection_t API sanity */
  tls = G_TLS_CONNECTION (xobject_new (
          xtls_backend_get_client_connection_type (backend), NULL));
  g_assert_nonnull (tls);

  g_assert_false (xtls_connection_get_channel_binding_data (tls,
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, NULL));

  g_assert_false (xtls_connection_get_channel_binding_data (tls,
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, &error));
  g_assert_error (error, G_TLS_CHANNEL_BINDING_ERROR,
                         G_TLS_CHANNEL_BINDING_ERROR_NOT_IMPLEMENTED);
  g_clear_error (&error);

  if (g_test_subprocess ())
    g_assert_false (xtls_connection_get_channel_binding_data (tls,
            G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, (xerror_t **)&not_null));

  xobject_unref (tls);
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*GLib-GIO-CRITICAL*");
}

static void
get_dtls_channel_binding (void)
{
  xtls_backend_t *backend;
  xchar_t *not_null = "NOT_NULL";
  xdtls_connection_t *dtls = NULL;
  xerror_t *error = NULL;

  backend = xtls_backend_get_default ();
  g_assert_nonnull (backend);

  /* repeat for the dtls now */
  dtls = G_DTLS_CONNECTION (xobject_new (
          xtls_backend_get_dtls_client_connection_type (backend), NULL));
  g_assert_nonnull (dtls);

  g_assert_false (g_dtls_connection_get_channel_binding_data (dtls,
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, NULL));

  g_assert_false (g_dtls_connection_get_channel_binding_data (dtls,
          G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, &error));
  g_assert_error (error, G_TLS_CHANNEL_BINDING_ERROR,
                         G_TLS_CHANNEL_BINDING_ERROR_NOT_IMPLEMENTED);
  g_clear_error (&error);

  if (g_test_subprocess ())
    g_assert_false (g_dtls_connection_get_channel_binding_data (dtls,
            G_TLS_CHANNEL_BINDING_TLS_UNIQUE, NULL, (xerror_t **)&not_null));

  xobject_unref (dtls);
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*GLib-GIO-CRITICAL*");
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  _g_test_tls_backend_get_type ();

  g_test_add_func ("/tls-connection/get-tls-channel-binding", get_tls_channel_binding);
  g_test_add_func ("/tls-connection/get-dtls-channel-binding", get_dtls_channel_binding);

  return g_test_run ();
}
