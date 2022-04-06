/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2009 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Authors: David Zeuthen <davidz@redhat.com>
 */

/**
 * SECTION:gunixcredentialsmessage
 * @title: xunix_credentials_message_t
 * @short_description: A xsocket_control_message_t containing credentials
 * @include: gio/gunixcredentialsmessage.h
 * @see_also: #GUnixConnection, #xsocket_control_message_t
 *
 * This #xsocket_control_message_t contains a #xcredentials_t instance.  It
 * may be sent using xsocket_send_message() and received using
 * xsocket_receive_message() over UNIX sockets (ie: sockets in the
 * %XSOCKET_FAMILY_UNIX family).
 *
 * For an easier way to send and receive credentials over
 * stream-oriented UNIX sockets, see
 * g_unix_connection_send_credentials() and
 * g_unix_connection_receive_credentials(). To receive credentials of
 * a foreign process connected to a socket, use
 * xsocket_get_credentials().
 *
 * Since GLib 2.72, #GUnixCredentialMessage is available on all platforms. It
 * requires underlying system support (such as Windows 10 with `AF_UNIX`) at run
 * time.
 *
 * Before GLib 2.72, `<gio/gunixcredentialsmessage.h>` belonged to the UNIX-specific
 * GIO interfaces, thus you had to use the `gio-unix-2.0.pc` pkg-config file
 * when using it. This is no longer necessary since GLib 2.72.
 */

#include "config.h"

/* ---------------------------------------------------------------------------------------------------- */

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gunixcredentialsmessage.h"
#include "gcredentials.h"
#include "gcredentialsprivate.h"
#include "gnetworking.h"

#include "glibintl.h"

struct _GUnixCredentialsMessagePrivate
{
  xcredentials_t *credentials;
};

enum
{
  PROP_0,
  PROP_CREDENTIALS
};

G_DEFINE_TYPE_WITH_PRIVATE (xunix_credentials_message_t, g_unix_credentials_message, XTYPE_SOCKET_CONTROL_MESSAGE)

static xsize_t
g_unix_credentials_message_get_size (xsocket_control_message_t *message)
{
#if G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED
  return G_CREDENTIALS_NATIVE_SIZE;
#else
  return 0;
#endif
}

static int
g_unix_credentials_message_get_level (xsocket_control_message_t *message)
{
#if G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED
  return SOL_SOCKET;
#else
  return 0;
#endif
}

static int
g_unix_credentials_message_get_msg_type (xsocket_control_message_t *message)
{
#if G_CREDENTIALS_USE_LINUX_UCRED
  return SCM_CREDENTIALS;
#elif G_CREDENTIALS_USE_FREEBSD_CMSGCRED
  return SCM_CREDS;
#elif G_CREDENTIALS_USE_NETBSD_UNPCBID
  return SCM_CREDS;
#elif G_CREDENTIALS_USE_SOLARIS_UCRED
  return SCM_UCRED;
#elif G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED
  #error "G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED is set but there is no msg_type defined for this platform"
#else
  /* includes G_CREDENTIALS_USE_APPLE_XUCRED */
  return 0;
#endif
}

static xsocket_control_message_t *
g_unix_credentials_message_deserialize (xint_t     level,
                                        xint_t     type,
                                        xsize_t    size,
                                        xpointer_t data)
{
#if G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED
  xsocket_control_message_t *message;
  xcredentials_t *credentials;

  if (level != SOL_SOCKET || type != g_unix_credentials_message_get_msg_type (NULL))
    return NULL;

  if (size != G_CREDENTIALS_NATIVE_SIZE)
    {
      g_warning ("Expected a credentials struct of %" G_GSIZE_FORMAT " bytes but "
                 "got %" G_GSIZE_FORMAT " bytes of data",
                 G_CREDENTIALS_NATIVE_SIZE, size);
      return NULL;
    }

  credentials = xcredentials_new ();
  xcredentials_set_native (credentials, G_CREDENTIALS_NATIVE_TYPE, data);

  if (xcredentials_get_unix_user (credentials, NULL) == (uid_t) -1)
    {
      /* This happens on Linux if the remote side didn't pass the credentials */
      xobject_unref (credentials);
      return NULL;
    }

  message = g_unix_credentials_message_new_with_credentials (credentials);
  xobject_unref (credentials);

  return message;

#else /* !G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED */

  return NULL;
#endif
}

static void
g_unix_credentials_message_serialize (xsocket_control_message_t *_message,
                                      xpointer_t               data)
{
#if G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED
  xunix_credentials_message_t *message = G_UNIX_CREDENTIALS_MESSAGE (_message);

  memcpy (data,
          xcredentials_get_native (message->priv->credentials,
                                    G_CREDENTIALS_NATIVE_TYPE),
          G_CREDENTIALS_NATIVE_SIZE);
#endif
}

static void
g_unix_credentials_message_finalize (xobject_t *object)
{
  xunix_credentials_message_t *message = G_UNIX_CREDENTIALS_MESSAGE (object);

  if (message->priv->credentials != NULL)
    xobject_unref (message->priv->credentials);

  G_OBJECT_CLASS (g_unix_credentials_message_parent_class)->finalize (object);
}

static void
g_unix_credentials_message_init (xunix_credentials_message_t *message)
{
  message->priv = g_unix_credentials_message_get_instance_private (message);
}

static void
g_unix_credentials_message_get_property (xobject_t    *object,
                                         xuint_t       prop_id,
                                         xvalue_t     *value,
                                         xparam_spec_t *pspec)
{
  xunix_credentials_message_t *message = G_UNIX_CREDENTIALS_MESSAGE (object);

  switch (prop_id)
    {
    case PROP_CREDENTIALS:
      xvalue_set_object (value, message->priv->credentials);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_unix_credentials_message_set_property (xobject_t      *object,
                                         xuint_t         prop_id,
                                         const xvalue_t *value,
                                         xparam_spec_t   *pspec)
{
  xunix_credentials_message_t *message = G_UNIX_CREDENTIALS_MESSAGE (object);

  switch (prop_id)
    {
    case PROP_CREDENTIALS:
      message->priv->credentials = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_unix_credentials_message_constructed (xobject_t *object)
{
  xunix_credentials_message_t *message = G_UNIX_CREDENTIALS_MESSAGE (object);

  if (message->priv->credentials == NULL)
    message->priv->credentials = xcredentials_new ();

  if (G_OBJECT_CLASS (g_unix_credentials_message_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (g_unix_credentials_message_parent_class)->constructed (object);
}

static void
g_unix_credentials_message_class_init (GUnixCredentialsMessageClass *class)
{
  xsocket_control_message_class_t *scm_class;
  xobject_class_t *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->get_property = g_unix_credentials_message_get_property;
  gobject_class->set_property = g_unix_credentials_message_set_property;
  gobject_class->finalize = g_unix_credentials_message_finalize;
  gobject_class->constructed = g_unix_credentials_message_constructed;

  scm_class = XSOCKET_CONTROL_MESSAGE_CLASS (class);
  scm_class->get_size = g_unix_credentials_message_get_size;
  scm_class->get_level = g_unix_credentials_message_get_level;
  scm_class->get_type = g_unix_credentials_message_get_msg_type;
  scm_class->serialize = g_unix_credentials_message_serialize;
  scm_class->deserialize = g_unix_credentials_message_deserialize;

  /**
   * xunix_credentials_message_t:credentials:
   *
   * The credentials stored in the message.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_CREDENTIALS,
                                   g_param_spec_object ("credentials",
                                                        P_("Credentials"),
                                                        P_("The credentials stored in the message"),
                                                        XTYPE_CREDENTIALS,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_unix_credentials_message_is_supported:
 *
 * Checks if passing #xcredentials_t on a #xsocket_t is supported on this platform.
 *
 * Returns: %TRUE if supported, %FALSE otherwise
 *
 * Since: 2.26
 */
xboolean_t
g_unix_credentials_message_is_supported (void)
{
#if G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED
  return TRUE;
#else
  return FALSE;
#endif
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_unix_credentials_message_new:
 *
 * Creates a new #xunix_credentials_message_t with credentials matching the current processes.
 *
 * Returns: a new #xunix_credentials_message_t
 *
 * Since: 2.26
 */
xsocket_control_message_t *
g_unix_credentials_message_new (void)
{
  g_return_val_if_fail (g_unix_credentials_message_is_supported (), NULL);
  return xobject_new (XTYPE_UNIX_CREDENTIALS_MESSAGE,
                       NULL);
}

/**
 * g_unix_credentials_message_new_with_credentials:
 * @credentials: A #xcredentials_t object.
 *
 * Creates a new #xunix_credentials_message_t holding @credentials.
 *
 * Returns: a new #xunix_credentials_message_t
 *
 * Since: 2.26
 */
xsocket_control_message_t *
g_unix_credentials_message_new_with_credentials (xcredentials_t *credentials)
{
  g_return_val_if_fail (X_IS_CREDENTIALS (credentials), NULL);
  g_return_val_if_fail (g_unix_credentials_message_is_supported (), NULL);
  return xobject_new (XTYPE_UNIX_CREDENTIALS_MESSAGE,
                       "credentials", credentials,
                       NULL);
}

/**
 * g_unix_credentials_message_get_credentials:
 * @message: A #xunix_credentials_message_t.
 *
 * Gets the credentials stored in @message.
 *
 * Returns: (transfer none): A #xcredentials_t instance. Do not free, it is owned by @message.
 *
 * Since: 2.26
 */
xcredentials_t *
g_unix_credentials_message_get_credentials (xunix_credentials_message_t *message)
{
  g_return_val_if_fail (X_IS_UNIX_CREDENTIALS_MESSAGE (message), NULL);
  return message->priv->credentials;
}
