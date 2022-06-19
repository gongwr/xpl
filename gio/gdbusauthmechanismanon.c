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

#include "gdbusauthmechanismanon.h"
#include "gdbuserror.h"
#include "gioenumtypes.h"

#include "glibintl.h"

struct _GDBusAuthMechanismAnonPrivate
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

G_DEFINE_TYPE_WITH_PRIVATE (GDBusAuthMechanismAnon, _xdbus_auth_mechanism_anon, XTYPE_DBUS_AUTH_MECHANISM)

/* ---------------------------------------------------------------------------------------------------- */

static void
_xdbus_auth_mechanism_anon_finalize (xobject_t *object)
{
  //GDBusAuthMechanismAnon *mechanism = XDBUS_AUTH_MECHANISM_ANON (object);

  if (XOBJECT_CLASS (_xdbus_auth_mechanism_anon_parent_class)->finalize != NULL)
    XOBJECT_CLASS (_xdbus_auth_mechanism_anon_parent_class)->finalize (object);
}

static void
_xdbus_auth_mechanism_anon_class_init (GDBusAuthMechanismAnonClass *klass)
{
  xobject_class_t *xobject_class;
  xdbus_auth_mechanism_class_t *mechanism_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->finalize = _xdbus_auth_mechanism_anon_finalize;

  mechanism_class = XDBUS_AUTH_MECHANISM_CLASS (klass);
  mechanism_class->get_priority              = mechanism_get_priority;
  mechanism_class->get_name                  = mechanism_get_name;
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
_xdbus_auth_mechanism_anon_init (GDBusAuthMechanismAnon *mechanism)
{
  mechanism->priv = _xdbus_auth_mechanism_anon_get_instance_private (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */


static xint_t
mechanism_get_priority (void)
{
  /* We prefer ANONYMOUS to most other mechanism (such as DBUS_COOKIE_SHA1) but not to EXTERNAL */
  return 50;
}


static const xchar_t *
mechanism_get_name (void)
{
  return "ANONYMOUS";
}

static xboolean_t
mechanism_is_supported (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), FALSE);
  return TRUE;
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
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, XDBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static void
mechanism_server_initiate (xdbus_auth_mechanism_t   *mechanism,
                           const xchar_t          *initial_response,
                           xsize_t                 initial_response_len)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism));
  g_return_if_fail (!m->priv->is_server && !m->priv->is_client);

  //g_debug ("ANONYMOUS: initial_response was '%s'", initial_response);

  m->priv->is_server = TRUE;
  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;
}

static void
mechanism_server_data_receive (xdbus_auth_mechanism_t   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);
  g_return_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  /* can never end up here because we are never in the WAITING_FOR_DATA state */
  g_assert_not_reached ();
}

static xchar_t *
mechanism_server_data_send (xdbus_auth_mechanism_t   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  /* can never end up here because we are never in the HAVE_DATA_TO_SEND state */
  g_assert_not_reached ();

  return NULL;
}

static xchar_t *
mechanism_server_get_reject_reason (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_REJECTED, NULL);

  /* can never end up here because we are never in the REJECTED state */
  g_assert_not_reached ();

  return NULL;
}

static void
mechanism_server_shutdown (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_auth_mechanism_state_t

mechanism_client_get_state (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  xreturn_val_if_fail (m->priv->is_client && !m->priv->is_server, XDBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static xchar_t *
mechanism_client_initiate (xdbus_auth_mechanism_t   *mechanism,
                           xsize_t                *out_initial_response_len)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);
  xchar_t *result;

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), NULL);
  xreturn_val_if_fail (!m->priv->is_server && !m->priv->is_client, NULL);

  m->priv->is_client = TRUE;
  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;

  /* just return our library name and version */
  result = xstrdup ("GDBus 0.1");
  *out_initial_response_len = strlen (result);

  return result;
}

static void
mechanism_client_data_receive (xdbus_auth_mechanism_t   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);
  g_return_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  /* can never end up here because we are never in the WAITING_FOR_DATA state */
  g_assert_not_reached ();
}

static xchar_t *
mechanism_client_data_send (xdbus_auth_mechanism_t   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_client && !m->priv->is_server, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  /* can never end up here because we are never in the HAVE_DATA_TO_SEND state */
  g_assert_not_reached ();

  return NULL;
}

static void
mechanism_client_shutdown (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismAnon *m = XDBUS_AUTH_MECHANISM_ANON (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_ANON (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);

  m->priv->is_client = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */
