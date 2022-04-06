/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Clemens N. Buss <cebuzz@gmail.com>
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

#include <config.h>

#include "gicon.h"
#include "gemblem.h"
#include "glibintl.h"
#include "gioenums.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include <stdlib.h>
#include <string.h>


/**
 * SECTION:gemblem
 * @short_description: An object for emblems
 * @include: gio/gio.h
 * @see_also: #xicon_t, #xemblemed_icon_t, #xloadable_icon_t, #xthemed_icon_t
 *
 * #xemblem_t is an implementation of #xicon_t that supports
 * having an emblem, which is an icon with additional properties.
 * It can than be added to a #xemblemed_icon_t.
 *
 * Currently, only metainformation about the emblem's origin is
 * supported. More may be added in the future.
 */

static void xemblem_iface_init (xicon_iface_t *iface);

struct _GEmblem
{
  xobject_t parent_instance;

  xicon_t *icon;
  GEmblemOrigin origin;
};

struct _GEmblemClass
{
  xobject_class_t parent_class;
};

enum
{
  PROP_0_GEMBLEM,
  PROP_ICON,
  PROP_ORIGIN
};

G_DEFINE_TYPE_WITH_CODE (xemblem, g_emblem, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_ICON, xemblem_iface_init))

static void
xemblem_get_property (xobject_t    *object,
                       xuint_t       prop_id,
                       xvalue_t     *value,
                       xparam_spec_t *pspec)
{
  xemblem_t *emblem = G_EMBLEM (object);

  switch (prop_id)
    {
      case PROP_ICON:
        xvalue_set_object (value, emblem->icon);
	break;

      case PROP_ORIGIN:
        xvalue_set_enum (value, emblem->origin);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
  }
}

static void
xemblem_set_property (xobject_t      *object,
                       xuint_t         prop_id,
                       const xvalue_t *value,
                       xparam_spec_t   *pspec)
{
  xemblem_t *emblem = G_EMBLEM (object);

  switch (prop_id)
    {
      case PROP_ICON:
        emblem->icon = xvalue_dup_object (value);
        break;

      case PROP_ORIGIN:
        emblem->origin = xvalue_get_enum (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
xemblem_finalize (xobject_t *object)
{
  xemblem_t *emblem = G_EMBLEM (object);

  if (emblem->icon)
    xobject_unref (emblem->icon);

  (*G_OBJECT_CLASS (xemblem_parent_class)->finalize) (object);
}

static void
xemblem_class_init (GEmblemClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = xemblem_finalize;
  gobject_class->set_property = xemblem_set_property;
  gobject_class->get_property = xemblem_get_property;

  xobject_class_install_property (gobject_class,
                                   PROP_ORIGIN,
                                   g_param_spec_enum ("origin",
                                                      P_("xemblem_t’s origin"),
                                                      P_("Tells which origin the emblem is derived from"),
                                                      XTYPE_EMBLEM_ORIGIN,
                                                      G_EMBLEM_ORIGIN_UNKNOWN,
                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class,
                                   PROP_ICON,
                                   g_param_spec_object ("icon",
                                                      P_("The icon of the emblem"),
                                                      P_("The actual icon of the emblem"),
                                                      XTYPE_OBJECT,
                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
xemblem_init (xemblem_t *emblem)
{
}

/**
 * xemblem_new:
 * @icon: a xicon_t containing the icon.
 *
 * Creates a new emblem for @icon.
 *
 * Returns: a new #xemblem_t.
 *
 * Since: 2.18
 */
xemblem_t *
xemblem_new (xicon_t *icon)
{
  xemblem_t* emblem;

  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (X_IS_ICON (icon), NULL);
  g_return_val_if_fail (!X_IS_EMBLEM (icon), NULL);

  emblem = xobject_new (XTYPE_EMBLEM, NULL);
  emblem->icon = xobject_ref (icon);
  emblem->origin = G_EMBLEM_ORIGIN_UNKNOWN;

  return emblem;
}

/**
 * xemblem_new_with_origin:
 * @icon: a xicon_t containing the icon.
 * @origin: a GEmblemOrigin enum defining the emblem's origin
 *
 * Creates a new emblem for @icon.
 *
 * Returns: a new #xemblem_t.
 *
 * Since: 2.18
 */
xemblem_t *
xemblem_new_with_origin (xicon_t         *icon,
                          GEmblemOrigin  origin)
{
  xemblem_t* emblem;

  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (X_IS_ICON (icon), NULL);
  g_return_val_if_fail (!X_IS_EMBLEM (icon), NULL);

  emblem = xobject_new (XTYPE_EMBLEM, NULL);
  emblem->icon = xobject_ref (icon);
  emblem->origin = origin;

  return emblem;
}

/**
 * xemblem_get_icon:
 * @emblem: a #xemblem_t from which the icon should be extracted.
 *
 * Gives back the icon from @emblem.
 *
 * Returns: (transfer none): a #xicon_t. The returned object belongs to
 *          the emblem and should not be modified or freed.
 *
 * Since: 2.18
 */
xicon_t *
xemblem_get_icon (xemblem_t *emblem)
{
  g_return_val_if_fail (X_IS_EMBLEM (emblem), NULL);

  return emblem->icon;
}


/**
 * xemblem_get_origin:
 * @emblem: a #xemblem_t
 *
 * Gets the origin of the emblem.
 *
 * Returns: (transfer none): the origin of the emblem
 *
 * Since: 2.18
 */
GEmblemOrigin
xemblem_get_origin (xemblem_t *emblem)
{
  g_return_val_if_fail (X_IS_EMBLEM (emblem), G_EMBLEM_ORIGIN_UNKNOWN);

  return emblem->origin;
}

static xuint_t
xemblem_hash (xicon_t *icon)
{
  xemblem_t *emblem = G_EMBLEM (icon);
  xuint_t hash;

  hash  = xicon_hash (xemblem_get_icon (emblem));
  hash ^= emblem->origin;

  return hash;
}

static xboolean_t
xemblem_equal (xicon_t *icon1,
                xicon_t *icon2)
{
  xemblem_t *emblem1 = G_EMBLEM (icon1);
  xemblem_t *emblem2 = G_EMBLEM (icon2);

  return emblem1->origin == emblem2->origin &&
         xicon_equal (emblem1->icon, emblem2->icon);
}

static xboolean_t
xemblem_to_tokens (xicon_t *icon,
		    xptr_array_t *tokens,
		    xint_t  *out_version)
{
  xemblem_t *emblem = G_EMBLEM (icon);
  char *s;

  /* xemblem_t are encoded as
   *
   * <origin> <icon>
   */

  g_return_val_if_fail (out_version != NULL, FALSE);

  *out_version = 0;

  s = xicon_to_string (emblem->icon);
  if (s == NULL)
    return FALSE;

  xptr_array_add (tokens, s);

  s = xstrdup_printf ("%d", emblem->origin);
  xptr_array_add (tokens, s);

  return TRUE;
}

static xicon_t *
xemblem_from_tokens (xchar_t  **tokens,
		      xint_t     num_tokens,
		      xint_t     version,
		      xerror_t **error)
{
  xemblem_t *emblem;
  xicon_t *icon;
  GEmblemOrigin origin;

  emblem = NULL;

  if (version != 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Can’t handle version %d of xemblem_t encoding"),
                   version);
      return NULL;
    }

  if (num_tokens != 2)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Malformed number of tokens (%d) in xemblem_t encoding"),
                   num_tokens);
      return NULL;
    }

  icon = xicon_new_for_string (tokens[0], error);

  if (icon == NULL)
    return NULL;

  origin = atoi (tokens[1]);

  emblem = xemblem_new_with_origin (icon, origin);
  xobject_unref (icon);

  return XICON (emblem);
}

static xvariant_t *
xemblem_serialize (xicon_t *icon)
{
  xemblem_t *emblem = G_EMBLEM (icon);
  xvariant_t *icon_data;
  xenum_value_t *origin;
  xvariant_t *result;

  icon_data = xicon_serialize (emblem->icon);
  if (!icon_data)
    return NULL;

  origin = xenum_get_value (xtype_class_peek (XTYPE_EMBLEM_ORIGIN), emblem->origin);
  result = xvariant_new_parsed ("('emblem', <(%v, {'origin': <%s>})>)",
                                 icon_data, origin ? origin->value_nick : "unknown");
  xvariant_unref (icon_data);

  return result;
}

static void
xemblem_iface_init (xicon_iface_t *iface)
{
  iface->hash  = xemblem_hash;
  iface->equal = xemblem_equal;
  iface->to_tokens = xemblem_to_tokens;
  iface->from_tokens = xemblem_from_tokens;
  iface->serialize = xemblem_serialize;
}
