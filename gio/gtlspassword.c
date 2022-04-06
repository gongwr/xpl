/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2011 Collabora, Ltd.
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
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#include "config.h"
#include "glib.h"
#include "glibintl.h"

#include "gioenumtypes.h"
#include "gtlspassword.h"

#include <string.h>

/**
 * SECTION:gtlspassword
 * @title: xtls_password_t
 * @short_description: TLS Passwords for prompting
 * @include: gio/gio.h
 *
 * Holds a password used in TLS.
 */

/**
 * xtls_password_t:
 *
 * An abstract interface representing a password used in TLS. Often used in
 * user interaction such as unlocking a key storage token.
 *
 * Since: 2.30
 */

enum
{
  PROP_0,
  PROP_FLAGS,
  PROP_DESCRIPTION,
  PROP_WARNING
};

struct _GTlsPasswordPrivate
{
  xuchar_t *value;
  xsize_t length;
  xdestroy_notify_t destroy;
  GTlsPasswordFlags flags;
  xchar_t *description;
  xchar_t *warning;
};

G_DEFINE_TYPE_WITH_PRIVATE (xtls_password_t, xtls_password, XTYPE_OBJECT)

static void
xtls_password_init (xtls_password_t *password)
{
  password->priv = xtls_password_get_instance_private (password);
}

static const xuchar_t *
xtls_password_real_get_value (xtls_password_t  *password,
                               xsize_t         *length)
{
  if (length)
    *length = password->priv->length;
  return password->priv->value;
}

static void
xtls_password_real_set_value (xtls_password_t   *password,
                               xuchar_t         *value,
                               xssize_t          length,
                               xdestroy_notify_t  destroy)
{
  if (password->priv->destroy)
      (password->priv->destroy) (password->priv->value);
  password->priv->destroy = NULL;
  password->priv->value = NULL;
  password->priv->length = 0;

  if (length < 0)
    length = strlen ((xchar_t*) value);

  password->priv->value = value;
  password->priv->length = length;
  password->priv->destroy = destroy;
}

static const xchar_t*
xtls_password_real_get_default_warning (xtls_password_t  *password)
{
  GTlsPasswordFlags flags;

  flags = xtls_password_get_flags (password);

  if (flags & G_TLS_PASSWORD_FINAL_TRY)
    return _("This is the last chance to enter the password correctly before your access is locked out.");
  if (flags & G_TLS_PASSWORD_MANY_TRIES)
    /* Translators: This is not the 'This is the last chance' string. It is
     * displayed when more than one attempt is allowed. */
    return _("Several passwords entered have been incorrect, and your access will be locked out after further failures.");
  if (flags & G_TLS_PASSWORD_RETRY)
    return _("The password entered is incorrect.");

  return NULL;
}

static void
xtls_password_get_property (xobject_t    *object,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  xtls_password_t *password = G_TLS_PASSWORD (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      xvalue_set_flags (value, xtls_password_get_flags (password));
      break;
    case PROP_WARNING:
      xvalue_set_string (value, xtls_password_get_warning (password));
      break;
    case PROP_DESCRIPTION:
      xvalue_set_string (value, xtls_password_get_description (password));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xtls_password_set_property (xobject_t      *object,
                             xuint_t         prop_id,
                             const xvalue_t *value,
                             xparam_spec_t   *pspec)
{
  xtls_password_t *password = G_TLS_PASSWORD (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      xtls_password_set_flags (password, xvalue_get_flags (value));
      break;
    case PROP_WARNING:
      xtls_password_set_warning (password, xvalue_get_string (value));
      break;
    case PROP_DESCRIPTION:
      xtls_password_set_description (password, xvalue_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xtls_password_finalize (xobject_t *object)
{
  xtls_password_t *password = G_TLS_PASSWORD (object);

  xtls_password_real_set_value (password, NULL, 0, NULL);
  g_free (password->priv->warning);
  g_free (password->priv->description);

  G_OBJECT_CLASS (xtls_password_parent_class)->finalize (object);
}

static void
xtls_password_class_init (GTlsPasswordClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  klass->get_value = xtls_password_real_get_value;
  klass->set_value = xtls_password_real_set_value;
  klass->get_default_warning = xtls_password_real_get_default_warning;

  gobject_class->get_property = xtls_password_get_property;
  gobject_class->set_property = xtls_password_set_property;
  gobject_class->finalize = xtls_password_finalize;

  xobject_class_install_property (gobject_class, PROP_FLAGS,
				   g_param_spec_flags ("flags",
						       P_("Flags"),
						       P_("Flags about the password"),
						       XTYPE_TLS_PASSWORD_FLAGS,
						       G_TLS_PASSWORD_NONE,
						       G_PARAM_READWRITE |
						       G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class, PROP_DESCRIPTION,
				   g_param_spec_string ("description",
							P_("Description"),
							P_("Description of what the password is for"),
							NULL,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class, PROP_WARNING,
				   g_param_spec_string ("warning",
							P_("Warning"),
							P_("Warning about the password"),
							NULL,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

}

/**
 * xtls_password_new:
 * @flags: the password flags
 * @description: description of what the password is for
 *
 * Create a new #xtls_password_t object.
 *
 * Returns: (transfer full): The newly allocated password object
 */
xtls_password_t *
xtls_password_new (GTlsPasswordFlags  flags,
                    const xchar_t       *description)
{
  return xobject_new (XTYPE_TLS_PASSWORD,
                       "flags", flags,
                       "description", description,
                       NULL);
}

/**
 * xtls_password_get_value: (virtual get_value)
 * @password: a #xtls_password_t object
 * @length: (optional): location to place the length of the password.
 *
 * Get the password value. If @length is not %NULL then it will be
 * filled in with the length of the password value. (Note that the
 * password value is not nul-terminated, so you can only pass %NULL
 * for @length in contexts where you know the password will have a
 * certain fixed length.)
 *
 * Returns: (array length=length): The password value (owned by the password object).
 *
 * Since: 2.30
 */
const xuchar_t *
xtls_password_get_value (xtls_password_t  *password,
                          xsize_t         *length)
{
  g_return_val_if_fail (X_IS_TLS_PASSWORD (password), NULL);
  return G_TLS_PASSWORD_GET_CLASS (password)->get_value (password, length);
}

/**
 * xtls_password_set_value:
 * @password: a #xtls_password_t object
 * @value: (array length=length): the new password value
 * @length: the length of the password, or -1
 *
 * Set the value for this password. The @value will be copied by the password
 * object.
 *
 * Specify the @length, for a non-nul-terminated password. Pass -1 as
 * @length if using a nul-terminated password, and @length will be
 * calculated automatically. (Note that the terminating nul is not
 * considered part of the password in this case.)
 *
 * Since: 2.30
 */
void
xtls_password_set_value (xtls_password_t  *password,
                          const xuchar_t  *value,
                          xssize_t         length)
{
  g_return_if_fail (X_IS_TLS_PASSWORD (password));

  if (length < 0)
    {
      /* FIXME: xtls_password_set_value_full() doesn’t support unsigned xsize_t */
      xsize_t length_unsigned = strlen ((xchar_t *) value);
      g_return_if_fail (length_unsigned <= G_MAXSSIZE);
      length = (xssize_t) length_unsigned;
    }

  xtls_password_set_value_full (password, g_memdup2 (value, (xsize_t) length), length, g_free);
}

/**
 * xtls_password_set_value_full:
 * @password: a #xtls_password_t object
 * @value: (array length=length): the value for the password
 * @length: the length of the password, or -1
 * @destroy: (nullable): a function to use to free the password.
 *
 * Provide the value for this password.
 *
 * The @value will be owned by the password object, and later freed using
 * the @destroy function callback.
 *
 * Specify the @length, for a non-nul-terminated password. Pass -1 as
 * @length if using a nul-terminated password, and @length will be
 * calculated automatically. (Note that the terminating nul is not
 * considered part of the password in this case.)
 *
 * Virtual: set_value
 * Since: 2.30
 */
void
xtls_password_set_value_full (xtls_password_t   *password,
                               xuchar_t         *value,
                               xssize_t          length,
                               xdestroy_notify_t  destroy)
{
  g_return_if_fail (X_IS_TLS_PASSWORD (password));
  G_TLS_PASSWORD_GET_CLASS (password)->set_value (password, value,
                                                  length, destroy);
}

/**
 * xtls_password_get_flags:
 * @password: a #xtls_password_t object
 *
 * Get flags about the password.
 *
 * Returns: The flags about the password.
 *
 * Since: 2.30
 */
GTlsPasswordFlags
xtls_password_get_flags (xtls_password_t *password)
{
  g_return_val_if_fail (X_IS_TLS_PASSWORD (password), G_TLS_PASSWORD_NONE);
  return password->priv->flags;
}

/**
 * xtls_password_set_flags:
 * @password: a #xtls_password_t object
 * @flags: The flags about the password
 *
 * Set flags about the password.
 *
 * Since: 2.30
 */
void
xtls_password_set_flags (xtls_password_t      *password,
                          GTlsPasswordFlags  flags)
{
  g_return_if_fail (X_IS_TLS_PASSWORD (password));

  password->priv->flags = flags;

  xobject_notify (G_OBJECT (password), "flags");
}

/**
 * xtls_password_get_description:
 * @password: a #xtls_password_t object
 *
 * Get a description string about what the password will be used for.
 *
 * Returns: The description of the password.
 *
 * Since: 2.30
 */
const xchar_t*
xtls_password_get_description (xtls_password_t *password)
{
  g_return_val_if_fail (X_IS_TLS_PASSWORD (password), NULL);
  return password->priv->description;
}

/**
 * xtls_password_set_description:
 * @password: a #xtls_password_t object
 * @description: The description of the password
 *
 * Set a description string about what the password will be used for.
 *
 * Since: 2.30
 */
void
xtls_password_set_description (xtls_password_t      *password,
                                const xchar_t       *description)
{
  xchar_t *copy;

  g_return_if_fail (X_IS_TLS_PASSWORD (password));

  copy = xstrdup (description);
  g_free (password->priv->description);
  password->priv->description = copy;

  xobject_notify (G_OBJECT (password), "description");
}

/**
 * xtls_password_get_warning:
 * @password: a #xtls_password_t object
 *
 * Get a user readable translated warning. Usually this warning is a
 * representation of the password flags returned from
 * xtls_password_get_flags().
 *
 * Returns: The warning.
 *
 * Since: 2.30
 */
const xchar_t *
xtls_password_get_warning (xtls_password_t      *password)
{
  g_return_val_if_fail (X_IS_TLS_PASSWORD (password), NULL);

  if (password->priv->warning == NULL)
    return G_TLS_PASSWORD_GET_CLASS (password)->get_default_warning (password);

  return password->priv->warning;
}

/**
 * xtls_password_set_warning:
 * @password: a #xtls_password_t object
 * @warning: The user readable warning
 *
 * Set a user readable translated warning. Usually this warning is a
 * representation of the password flags returned from
 * xtls_password_get_flags().
 *
 * Since: 2.30
 */
void
xtls_password_set_warning (xtls_password_t      *password,
                            const xchar_t       *warning)
{
  xchar_t *copy;

  g_return_if_fail (X_IS_TLS_PASSWORD (password));

  copy = xstrdup (warning);
  g_free (password->priv->warning);
  password->priv->warning = copy;

  xobject_notify (G_OBJECT (password), "warning");
}
