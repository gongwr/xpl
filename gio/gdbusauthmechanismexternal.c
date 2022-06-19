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
  xdbus_auth_mechanism_state_t
 state;
};

static xint_t                     mechanism_get_priority              (void);
static const xchar_t             *mechanism_get_name                  (void);

static xboolean_t                 mechanism_is_supported              (xdbus_auth_mechanism_t   *mechanism);
static xchar_t                   *mechanism_encode_data               (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len,
                                                                     xsize_t                *out_data_len);
static xchar_t                   *mechanism_decode_data               (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len,
                                                                     xsize_t                *out_data_len);
static xdbus_auth_mechanism_state_t
  mechanism_server_get_state          (xdbus_auth_mechanism_t   *mechanism);
static void                     mechanism_server_initiate           (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *initial_response,
                                                                     xsize_t                 initial_response_len);
static void                     mechanism_server_data_receive       (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len);
static xchar_t                   *mechanism_server_data_send          (xdbus_auth_mechanism_t   *mechanism,
                                                                     xsize_t                *out_data_len);
static xchar_t                   *mechanism_server_get_reject_reason  (xdbus_auth_mechanism_t   *mechanism);
static void                     mechanism_server_shutdown           (xdbus_auth_mechanism_t   *mechanism);
static xdbus_auth_mechanism_state_t
  mechanism_client_get_state          (xdbus_auth_mechanism_t   *mechanism);
static xchar_t                   *mechanism_client_initiate           (xdbus_auth_mechanism_t   *mechanism,
                                                                     xsize_t                *out_initial_response_len);
static void                     mechanism_client_data_receive       (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len);
static xchar_t                   *mechanism_client_data_send          (xdbus_auth_mechanism_t   *mechanism,
                                                                     xsize_t                *out_data_len);
static void                     mechanism_client_shutdown           (xdbus_auth_mechanism_t   *mechanism);

/* ---------------------------------------------------------------------------------------------------- */

G_DEFINE_TYPE_WITH_PRIVATE (GDBusAuthMechanismExternal, _xdbus_auth_mechanism_external, XTYPE_DBUS_AUTH_MECHANISM)

/* ---------------------------------------------------------------------------------------------------- */

static void
_xdbus_auth_mechanism_external_finalize (xobject_t *object)
{
  //GDBusAuthMechanismExternal *mechanism = XDBUS_AUTH_MECHANISM_EXTERNAL (object);

  if (XOBJECT_CLASS (_xdbus_auth_mechanism_external_parent_class)->finalize != NULL)
    XOBJECT_CLASS (_xdbus_auth_mechanism_external_parent_class)->finalize (object);
}

static void
_xdbus_auth_mechanism_external_class_init (GDBusAuthMechanismExternalClass *klass)
{
  xobject_class_t *xobject_class;
  xdbus_auth_mechanism_class_t *mechanism_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->finalize = _xdbus_auth_mechanism_external_finalize;

  mechanism_class = XDBUS_AUTH_MECHANISM_CLASS (klass);
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
_xdbus_auth_mechanism_external_init (GDBusAuthMechanismExternal *mechanism)
{
  mechanism->priv = _xdbus_auth_mechanism_external_get_instance_private (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
mechanism_is_supported (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), FALSE);

#if defined(G_OS_WIN32)
  /* all that is required is current process SID */
  return TRUE;
#else
  /* This mechanism is only available if credentials has been exchanged */
  if (_xdbus_auth_mechanism_get_credentials (mechanism) != NULL)
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
mechanism_encode_data (xdbus_auth_mechanism_t   *mechanism,
                       const xchar_t          *data,
                       xsize_t                 data_len,
                       xsize_t                *out_data_len)
{
  return NULL;
}


static xchar_t *
mechanism_decode_data (xdbus_auth_mechanism_t   *mechanism,
                       const xchar_t          *data,
                       xsize_t                 data_len,
                       xsize_t                *out_data_len)
{
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_auth_mechanism_state_t

mechanism_server_get_state (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, XDBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static xboolean_t
data_matches_credentials (const xchar_t  *data,
                          xsize_t         data_len,
                          xcredentials_t *credentials)
{
  xboolean_t match;

  match = FALSE;

  if (credentials == NULL)
    goto out;

  if (data == NULL || data_len == 0)
    goto out;

#if defined(G_OS_UNIX)
  {
    sint64_t alleged_uid;
    xchar_t *endp;

    /* on UNIX, this is the uid as a string in base 10 */
    alleged_uid = g_ascii_strtoll (data, &endp, 10);
    if (*endp == '\0')
      {
        if (xcredentials_get_unix_user (credentials, NULL) == alleged_uid)
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
mechanism_server_initiate (xdbus_auth_mechanism_t   *mechanism,
                           const xchar_t          *initial_response,
                           xsize_t                 initial_response_len)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (!m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = TRUE;

  if (initial_response != NULL)
    {
      if (data_matches_credentials (initial_response,
                                    initial_response_len,
                                    _xdbus_auth_mechanism_get_credentials (mechanism)))
        {
          m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;
        }
      else
        {
          m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
        }
    }
  else
    {
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA;
    }
}

static void
mechanism_server_data_receive (xdbus_auth_mechanism_t   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);
  g_return_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  if (data_matches_credentials (data,
                                data_len,
                                _xdbus_auth_mechanism_get_credentials (mechanism)))
    {
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;
    }
  else
    {
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
    }
}

static xchar_t *
mechanism_server_data_send (xdbus_auth_mechanism_t   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  /* can never end up here because we are never in the HAVE_DATA_TO_SEND state */
  g_assert_not_reached ();

  return NULL;
}

static xchar_t *
mechanism_server_get_reject_reason (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_REJECTED, NULL);

  /* can never end up here because we are never in the REJECTED state */
  g_assert_not_reached ();

  return NULL;
}

static void
mechanism_server_shutdown (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_auth_mechanism_state_t

mechanism_client_get_state (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  xreturn_val_if_fail (m->priv->is_client && !m->priv->is_server, XDBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static xchar_t *
mechanism_client_initiate (xdbus_auth_mechanism_t   *mechanism,
                           xsize_t                *out_initial_response_len)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);
  xchar_t *initial_response = NULL;
#if defined(G_OS_UNIX)
  xcredentials_t *credentials;
#endif

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  xreturn_val_if_fail (!m->priv->is_server && !m->priv->is_client, NULL);

  m->priv->is_client = TRUE;
  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;

  *out_initial_response_len = 0;

  /* return the uid */
#if defined(G_OS_UNIX)
  credentials = _xdbus_auth_mechanism_get_credentials (mechanism);
  xassert (credentials != NULL);

  initial_response = xstrdup_printf ("%" G_GINT64_FORMAT, (sint64_t) xcredentials_get_unix_user (credentials, NULL));
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
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;
      *out_initial_response_len = strlen (initial_response);
    }
  return initial_response;
}

static void
mechanism_client_data_receive (xdbus_auth_mechanism_t   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);
  g_return_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  /* can never end up here because we are never in the WAITING_FOR_DATA state */
  g_assert_not_reached ();
}

static xchar_t *
mechanism_client_data_send (xdbus_auth_mechanism_t   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_client && !m->priv->is_server, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  /* can never end up here because we are never in the HAVE_DATA_TO_SEND state */
  g_assert_not_reached ();

  return NULL;
}

static void
mechanism_client_shutdown (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismExternal *m = XDBUS_AUTH_MECHANISM_EXTERNAL (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_EXTERNAL (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);

  m->priv->is_client = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */
