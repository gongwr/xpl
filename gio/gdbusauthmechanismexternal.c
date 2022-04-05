/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <string.h>

#include "gdbusauthmechanismexternal.h"
#include "gcredentials.h"
#include "gdbuserror.h"
#include "gioenumtypes.h"

#include "glibintl.h"

#ifdef G_OS_WIN32
#include "gwin32sid.h"
#endif

struct _GDBusAuthMechanismExternalPrivate
{
  xboolean_t is_client;
  xboolean_t is_server;
  GDBusAuthMechanismState state;
};

static xint_t                     mechanism_get_priority              (void);
static const xchar_t             *mechanism_get_name                  (void);

static xboolean_t                 mechanism_is_supported              (GDBusAuthMechanism   *mechanism);
static xchar_t                   *mechanism_encode_data               (GDBusAuthMechanism   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len,
                                                                     xsize_t                *out_data_len);
static xchar_t                   *mechanism_decode_data               (GDBusAuthMechanism   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len,
                                                                     xsize_t                *out_data_len);
static GDBusAuthMechanismState  mechanism_server_get_state          (GDBusAuthMechanism   *mechanism);
static void                     mechanism_server_initiate           (GDBusAuthMechanism   *mechanism,
                                                                     const xchar_t          *initial_response,
                                                                     xsize_t                 initial_response_len);
static void                     mechanism_server_data_receive       (GDBusAuthMechanism   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len);
static xchar_t                   *mechanism_server_data_send          (GDBusAuthMechanism   *mechanism,
                                                                     xsize_t                *out_data_len);
static xchar_t                   *mechanism_server_get_reject_reason  (GDBusAuthMechanism   *mechanism);
static void                     mechanism_server_shutdown           (GDBusAuthMechanism   *mechanism);
static GDBusAuthMechanismState  mechanism_client_get_state          (GDBusAuthMechanism   *mechanism);
static xchar_t                   *mechanism_client_initiate           (GDBusAuthMechanism   *mechanism,
                                                                     xsize_t                *out_initial_response_len);
static void                     mechanism_client_data_receive       (GDBusAuthMechanism   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len);
static xchar_t                   *mechanism_client_data_send          (GDBusAuthMechanism   *mechanism,
                                                                     xsize_t                *out_data_len);
static void                     mechanism_client_shutdown           (GDBusAuthMechanism   *mechanism);

/* ---------------------------------------------------------------------------------------------------- */

G_DEFINE_TYPE_WITH_PRIVATE (GDBusAuthMechanismExternal, _g_dbus_auth_mechanism_external, XTYPE_DBUS_AUTH_MECHANISM)

/* ---------------------------------------------------------------------------------------------------- */

static void
_g_dbus_auth_mechanism_external_finalize (xobject_t *object)
{
  //GDBusAuthMechanismExternal *mechanism = G_DBUS_AUTH_MECHANISM_EXTERNAL (object);

  if (G_OBJECT_CLASS (_g_dbus_auth_mechanism_external_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (_g_dbus_auth_mechanism_external_parent_class)->finalize (object);
}

static void
_g_dbus_auth_mechanism_external_class_init (GDBusAuthMechanismExternalClass *klass)
{
  xobject_class_t *gobject_class;
  GDBusAuthMechanismClass *mechanism_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = _g_dbus_auth_mechanism_external_finalize;

  mechanism_class = G_DBUS_AUTH_MECHANISM_CLASS (klass);
  mechanism_class->get_name                  = mechanism_get_name;
  mechanism_class->get_priority              = mechanism_get_priority;
  mechanism_class->is_supported              = mechanism_is_supported;
  mechanism_class->encode_data               = mechanism_encode_data;
  mechanism_class->decode_data               = mechanism_decode_data;
  mechanism_class->server_get_state          = mechanism_server_get_state;
  mechanism_class->server_initiate           = mechanism_server_initiate;
  mechanism_class->server_data_receive       = mechanism_server_data_receive;
  mechanism_class->server_data_send          = mechanism_server_data_send;
  mechanism_class->server_get_reject_reason  = mechanism_server_get_reject_reason;
  mechanism_class->server_shutdown           = mechanism_server_shutdown;
  mechanism_class->client_get_state          = mechanism_client_get_state;
  mechanism_class->client_initiate           = mechanism_client_initiate;
  mechanism_class->client_data_receive       = mechanism_client_data_receive;
  mechanism_class->client_data_send          = mechanism_client_data_send;
  mechanism_class->client_shutdown           = mechanism_client_shutdown;
}

static void
_g_dbus_auth_mechanism_external_init (GDBusAuthMechanismExternal *mechanism)
{
  mechanism->priv = _g_dbus_auth_mechanism_external_get_instance_private (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
mechanism_is_supported (GDBusAuthMechanism *mechanism)
{
  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), FALSE);

#if defined(G_OS_WIN32)
  /* all that is required is current process SID */
  return TRUE;
#else
  /* This mechanism is only available if credentials has been exchanged */
  if (_g_dbus_auth_mechanism_get_credentials (mechanism) != NULL)
    return TRUE;
  else
    return FALSE;
#endif
}

static xint_t
mechanism_get_priority (void)
{
  /* We prefer EXTERNAL to most other mechanism (DBUS_COOKIE_SHA1 and ANONYMOUS) */
  return 100;
}

static const xchar_t *
mechanism_get_name (void)
{
  return "EXTERNAL";
}

static xchar_t *
mechanism_encode_data (GDBusAuthMechanism   *mechanism,
                       const xchar_t          *data,
                       xsize_t                 data_len,
                       xsize_t                *out_data_len)
{
  return NULL;
}


static xchar_t *
mechanism_decode_data (GDBusAuthMechanism   *mechanism,
                       const xchar_t          *data,
                       xsize_t                 data_len,
                       xsize_t                *out_data_len)
{
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

static GDBusAuthMechanismState
mechanism_server_get_state (GDBusAuthMechanism   *mechanism)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), G_DBUS_AUTH_MECHANISM_STATE_INVALID);
  g_return_val_if_fail (m->priv->is_server && !m->priv->is_client, G_DBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static xboolean_t
data_matches_credentials (const xchar_t  *data,
                          xsize_t         data_len,
                          GCredentials *credentials)
{
  xboolean_t match;

  match = FALSE;

  if (credentials == NULL)
    goto out;

  if (data == NULL || data_len == 0)
    goto out;

#if defined(G_OS_UNIX)
  {
    gint64 alleged_uid;
    xchar_t *endp;

    /* on UNIX, this is the uid as a string in base 10 */
    alleged_uid = g_ascii_strtoll (data, &endp, 10);
    if (*endp == '\0')
      {
        if (g_credentials_get_unix_user (credentials, NULL) == alleged_uid)
          {
            match = TRUE;
          }
      }
  }
#else
  /* TODO: Dont know how to compare credentials on this OS. Please implement. */
#endif

 out:
  return match;
}

static void
mechanism_server_initiate (GDBusAuthMechanism   *mechanism,
                           const xchar_t          *initial_response,
                           xsize_t                 initial_response_len)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (!m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = TRUE;

  if (initial_response != NULL)
    {
      if (data_matches_credentials (initial_response,
                                    initial_response_len,
                                    _g_dbus_auth_mechanism_get_credentials (mechanism)))
        {
          m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_ACCEPTED;
        }
      else
        {
          m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_REJECTED;
        }
    }
  else
    {
      m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA;
    }
}

static void
mechanism_server_data_receive (GDBusAuthMechanism   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);
  g_return_if_fail (m->priv->state == G_DBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  if (data_matches_credentials (data,
                                data_len,
                                _g_dbus_auth_mechanism_get_credentials (mechanism)))
    {
      m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_ACCEPTED;
    }
  else
    {
      m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_REJECTED;
    }
}

static xchar_t *
mechanism_server_data_send (GDBusAuthMechanism   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  g_return_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  g_return_val_if_fail (m->priv->state == G_DBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  /* can never end up here because we are never in the HAVE_DATA_TO_SEND state */
  g_assert_not_reached ();

  return NULL;
}

static xchar_t *
mechanism_server_get_reject_reason (GDBusAuthMechanism   *mechanism)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  g_return_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  g_return_val_if_fail (m->priv->state == G_DBUS_AUTH_MECHANISM_STATE_REJECTED, NULL);

  /* can never end up here because we are never in the REJECTED state */
  g_assert_not_reached ();

  return NULL;
}

static void
mechanism_server_shutdown (GDBusAuthMechanism   *mechanism)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

static GDBusAuthMechanismState
mechanism_client_get_state (GDBusAuthMechanism   *mechanism)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), G_DBUS_AUTH_MECHANISM_STATE_INVALID);
  g_return_val_if_fail (m->priv->is_client && !m->priv->is_server, G_DBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static xchar_t *
mechanism_client_initiate (GDBusAuthMechanism   *mechanism,
                           xsize_t                *out_initial_response_len)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);
  xchar_t *initial_response = NULL;
#if defined(G_OS_UNIX)
  GCredentials *credentials;
#endif

  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  g_return_val_if_fail (!m->priv->is_server && !m->priv->is_client, NULL);

  m->priv->is_client = TRUE;
  m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_REJECTED;

  *out_initial_response_len = 0;

  /* return the uid */
#if defined(G_OS_UNIX)
  credentials = _g_dbus_auth_mechanism_get_credentials (mechanism);
  g_assert (credentials != NULL);

  initial_response = g_strdup_printf ("%" G_GINT64_FORMAT, (gint64) g_credentials_get_unix_user (credentials, NULL));
#elif defined(G_OS_WIN32)
  initial_response = _g_win32_current_process_sid_string (NULL);
#else
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
#warning Dont know how to send credentials on this OS. The EXTERNAL D-Bus authentication mechanism will not work.
#pragma GCC diagnostic pop
#endif
#endif
  if (initial_response)
    {
      m->priv->state = G_DBUS_AUTH_MECHANISM_STATE_ACCEPTED;
      *out_initial_response_len = strlen (initial_response);
    }
  return initial_response;
}

static void
mechanism_client_data_receive (GDBusAuthMechanism   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);
  g_return_if_fail (m->priv->state == G_DBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  /* can never end up here because we are never in the WAITING_FOR_DATA state */
  g_assert_not_reached ();
}

static xchar_t *
mechanism_client_data_send (GDBusAuthMechanism   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  g_return_val_if_fail (m->priv->is_client && !m->priv->is_server, NULL);
  g_return_val_if_fail (m->priv->state == G_DBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  /* can never end up here because we are never in the HAVE_DATA_TO_SEND state */
  g_assert_not_reached ();

  return NULL;
}

static void
mechanism_client_shutdown (GDBusAuthMechanism   *mechanism)
{
  GDBusAuthMechanismExternal *m = G_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);

  m->priv->is_client = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */
