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

#include "gdbusauthmechanism.h"
#include "gcredentials.h"
#include "gdbuserror.h"
#include "gioenumtypes.h"
#include "giostream.h"

#include "glibintl.h"

/* ---------------------------------------------------------------------------------------------------- */

struct _xdbus_auth_mechanism_private
{
  xio_stream_t *stream;
  xcredentials_t *credentials;
};

enum
{
  PROP_0,
  PROP_STREAM,
  PROP_CREDENTIALS
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xdbus_auth_mechanism_t, _xdbus_auth_mechanism, XTYPE_OBJECT)

/* ---------------------------------------------------------------------------------------------------- */

static void
_xdbus_auth_mechanism_finalize (xobject_t *object)
{
  xdbus_auth_mechanism_t *mechanism = XDBUS_AUTH_MECHANISM (object);

  if (mechanism->priv->stream != NULL)
    xobject_unref (mechanism->priv->stream);
  if (mechanism->priv->credentials != NULL)
    xobject_unref (mechanism->priv->credentials);

  XOBJECT_CLASS (_xdbus_auth_mechanism_parent_class)->finalize (object);
}

static void
_xdbus_auth_mechanism_get_property (xobject_t    *object,
                                     xuint_t       prop_id,
                                     xvalue_t     *value,
                                     xparam_spec_t *pspec)
{
  xdbus_auth_mechanism_t *mechanism = XDBUS_AUTH_MECHANISM (object);

  switch (prop_id)
    {
    case PROP_STREAM:
      xvalue_set_object (value, mechanism->priv->stream);
      break;

    case PROP_CREDENTIALS:
      xvalue_set_object (value, mechanism->priv->credentials);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_xdbus_auth_mechanism_set_property (xobject_t      *object,
                                     xuint_t         prop_id,
                                     const xvalue_t *value,
                                     xparam_spec_t   *pspec)
{
  xdbus_auth_mechanism_t *mechanism = XDBUS_AUTH_MECHANISM (object);

  switch (prop_id)
    {
    case PROP_STREAM:
      mechanism->priv->stream = xvalue_dup_object (value);
      break;

    case PROP_CREDENTIALS:
      mechanism->priv->credentials = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_xdbus_auth_mechanism_class_init (xdbus_auth_mechanism_class_t *klass)
{
  xobject_class_t *xobject_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->get_property = _xdbus_auth_mechanism_get_property;
  xobject_class->set_property = _xdbus_auth_mechanism_set_property;
  xobject_class->finalize     = _xdbus_auth_mechanism_finalize;

  xobject_class_install_property (xobject_class,
                                   PROP_STREAM,
                                   xparam_spec_object ("stream",
                                                        P_("IO Stream"),
                                                        P_("The underlying xio_stream_t used for I/O"),
                                                        XTYPE_IO_STREAM,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_NAME |
                                                        XPARAM_STATIC_BLURB |
                                                        XPARAM_STATIC_NICK));

  /**
   * xdbus_auth_mechanism_t:credentials:
   *
   * If authenticating as a server, this property contains the
   * received credentials, if any.
   *
   * If authenticating as a client, the property contains the
   * credentials that were sent, if any.
   */
  xobject_class_install_property (xobject_class,
                                   PROP_CREDENTIALS,
                                   xparam_spec_object ("credentials",
                                                        P_("Credentials"),
                                                        P_("The credentials of the remote peer"),
                                                        XTYPE_CREDENTIALS,
                                                        XPARAM_READABLE |
                                                        XPARAM_WRITABLE |
                                                        XPARAM_CONSTRUCT_ONLY |
                                                        XPARAM_STATIC_NAME |
                                                        XPARAM_STATIC_BLURB |
                                                        XPARAM_STATIC_NICK));
}

static void
_xdbus_auth_mechanism_init (xdbus_auth_mechanism_t *mechanism)
{
  mechanism->priv = _xdbus_auth_mechanism_get_instance_private (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */

xio_stream_t *
_xdbus_auth_mechanism_get_stream (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return mechanism->priv->stream;
}

xcredentials_t *
_xdbus_auth_mechanism_get_credentials (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return mechanism->priv->credentials;
}

/* ---------------------------------------------------------------------------------------------------- */

const xchar_t *
_xdbus_auth_mechanism_get_name (xtype_t mechanism_type)
{
  const xchar_t *name;
  xdbus_auth_mechanism_class_t *klass;

  xreturn_val_if_fail (xtype_is_a (mechanism_type, XTYPE_DBUS_AUTH_MECHANISM), NULL);

  klass = xtype_class_ref (mechanism_type);
  xassert (klass != NULL);
  name = klass->get_name ();
  //xtype_class_unref (klass);

  return name;
}

xint_t
_xdbus_auth_mechanism_get_priority (xtype_t mechanism_type)
{
  xint_t priority;
  xdbus_auth_mechanism_class_t *klass;

  xreturn_val_if_fail (xtype_is_a (mechanism_type, XTYPE_DBUS_AUTH_MECHANISM), 0);

  klass = xtype_class_ref (mechanism_type);
  xassert (klass != NULL);
  priority = klass->get_priority ();
  //xtype_class_unref (klass);

  return priority;
}

/* ---------------------------------------------------------------------------------------------------- */

xboolean_t
_xdbus_auth_mechanism_is_supported (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), FALSE);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->is_supported (mechanism);
}

xchar_t *
_xdbus_auth_mechanism_encode_data (xdbus_auth_mechanism_t *mechanism,
                                    const xchar_t        *data,
                                    xsize_t               data_len,
                                    xsize_t              *out_data_len)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->encode_data (mechanism, data, data_len, out_data_len);
}


xchar_t *
_xdbus_auth_mechanism_decode_data (xdbus_auth_mechanism_t *mechanism,
                                    const xchar_t        *data,
                                    xsize_t               data_len,
                                    xsize_t              *out_data_len)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->decode_data (mechanism, data, data_len, out_data_len);
}

/* ---------------------------------------------------------------------------------------------------- */

xdbus_auth_mechanism_state_t

_xdbus_auth_mechanism_server_get_state (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->server_get_state (mechanism);
}

void
_xdbus_auth_mechanism_server_initiate (xdbus_auth_mechanism_t *mechanism,
                                        const xchar_t        *initial_response,
                                        xsize_t               initial_response_len)
{
  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism));
  XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->server_initiate (mechanism, initial_response, initial_response_len);
}

void
_xdbus_auth_mechanism_server_data_receive (xdbus_auth_mechanism_t *mechanism,
                                            const xchar_t        *data,
                                            xsize_t               data_len)
{
  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism));
  XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->server_data_receive (mechanism, data, data_len);
}

xchar_t *
_xdbus_auth_mechanism_server_data_send (xdbus_auth_mechanism_t *mechanism,
                                         xsize_t              *out_data_len)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->server_data_send (mechanism, out_data_len);
}

xchar_t *
_xdbus_auth_mechanism_server_get_reject_reason (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->server_get_reject_reason (mechanism);
}

void
_xdbus_auth_mechanism_server_shutdown (xdbus_auth_mechanism_t *mechanism)
{
  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism));
  XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->server_shutdown (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */

xdbus_auth_mechanism_state_t

_xdbus_auth_mechanism_client_get_state (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_get_state (mechanism);
}

xchar_t *
_xdbus_auth_mechanism_client_initiate (xdbus_auth_mechanism_t *mechanism,
                                        xsize_t              *out_initial_response_len)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_initiate (mechanism,
                                                                       out_initial_response_len);
}

void
_xdbus_auth_mechanism_client_data_receive (xdbus_auth_mechanism_t *mechanism,
                                            const xchar_t        *data,
                                            xsize_t               data_len)
{
  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism));
  XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_data_receive (mechanism, data, data_len);
}

xchar_t *
_xdbus_auth_mechanism_client_data_send (xdbus_auth_mechanism_t *mechanism,
                                         xsize_t              *out_data_len)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism), NULL);
  return XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_data_send (mechanism, out_data_len);
}

void
_xdbus_auth_mechanism_client_shutdown (xdbus_auth_mechanism_t *mechanism)
{
  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM (mechanism));
  XDBUS_AUTH_MECHANISM_GET_CLASS (mechanism)->client_shutdown (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */
