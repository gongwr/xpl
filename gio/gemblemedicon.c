/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Matthias Clasen <mclasen@redhat.com>
 *         Clemens N. Buss <cebuzz@gmail.com>
 */

#include <config.h>

#include <string.h>

#include "gemblemedicon.h"
#include "glibintl.h"
#include "gioerror.h"


/**
 * SECTION:gemblemedicon
 * @short_description: Icon with emblems
 * @include: gio/gio.h
 * @see_also: #xicon_t, #xloadable_icon_t, #xthemed_icon_t, #xemblem_t
 *
 * #xemblemed_icon_t is an implementation of #xicon_t that supports
 * adding an emblem to an icon. Adding multiple emblems to an
 * icon is ensured via g_emblemed_icon_add_emblem().
 *
 * Note that #xemblemed_icon_t allows no control over the position
 * of the emblems. See also #xemblem_t for more information.
 **/

enum {
  PROP_GICON = 1,
  NUM_PROPERTIES
};

struct _xemblemed_icon_tPrivate {
  xicon_t *icon;
  xlist_t *emblems;
};

static xparam_spec_t *properties[NUM_PROPERTIES] = { NULL, };

static void g_emblemed_icon_icon_iface_init (xicon_iface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xemblemed_icon, g_emblemed_icon, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xemblemed_icon_t)
                         G_IMPLEMENT_INTERFACE (XTYPE_ICON,
                         g_emblemed_icon_icon_iface_init))


static void
g_emblemed_icon_finalize (xobject_t *object)
{
  xemblemed_icon_t *emblemed;

  emblemed = G_EMBLEMED_ICON (object);

  g_clear_object (&emblemed->priv->icon);
  xlist_free_full (emblemed->priv->emblems, xobject_unref);

  (*G_OBJECT_CLASS (g_emblemed_icon_parent_class)->finalize) (object);
}

static void
g_emblemed_icon_set_property (xobject_t  *object,
                              xuint_t property_id,
                              const xvalue_t *value,
                              xparam_spec_t *pspec)
{
  xemblemed_icon_t *self = G_EMBLEMED_ICON (object);

  switch (property_id)
    {
    case PROP_GICON:
      self->priv->icon = xvalue_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
g_emblemed_icon_get_property (xobject_t  *object,
                              xuint_t property_id,
                              xvalue_t *value,
                              xparam_spec_t *pspec)
{
  xemblemed_icon_t *self = G_EMBLEMED_ICON (object);

  switch (property_id)
    {
    case PROP_GICON:
      xvalue_set_object (value, self->priv->icon);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
g_emblemed_icon_class_init (xemblemed_icon_tClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_emblemed_icon_finalize;
  gobject_class->set_property = g_emblemed_icon_set_property;
  gobject_class->get_property = g_emblemed_icon_get_property;

  properties[PROP_GICON] =
    g_param_spec_object ("gicon",
                         P_("The base xicon_t"),
                         P_("The xicon_t to attach emblems to"),
                         XTYPE_ICON,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  xobject_class_install_properties (gobject_class, NUM_PROPERTIES, properties);
}

static void
g_emblemed_icon_init (xemblemed_icon_t *emblemed)
{
  emblemed->priv = g_emblemed_icon_get_instance_private (emblemed);
}

/**
 * g_emblemed_icon_new:
 * @icon: a #xicon_t
 * @emblem: (nullable): a #xemblem_t, or %NULL
 *
 * Creates a new emblemed icon for @icon with the emblem @emblem.
 *
 * Returns: (transfer full) (type xemblemed_icon_t): a new #xicon_t
 *
 * Since: 2.18
 **/
xicon_t *
g_emblemed_icon_new (xicon_t   *icon,
                     xemblem_t *emblem)
{
  xemblemed_icon_t *emblemed;

  g_return_val_if_fail (X_IS_ICON (icon), NULL);
  g_return_val_if_fail (!X_IS_EMBLEM (icon), NULL);

  emblemed = G_EMBLEMED_ICON (xobject_new (XTYPE_EMBLEMED_ICON,
                                            "gicon", icon,
                                            NULL));

  if (emblem != NULL)
    g_emblemed_icon_add_emblem (emblemed, emblem);

  return XICON (emblemed);
}


/**
 * g_emblemed_icon_get_icon:
 * @emblemed: a #xemblemed_icon_t
 *
 * Gets the main icon for @emblemed.
 *
 * Returns: (transfer none): a #xicon_t that is owned by @emblemed
 *
 * Since: 2.18
 **/
xicon_t *
g_emblemed_icon_get_icon (xemblemed_icon_t *emblemed)
{
  g_return_val_if_fail (X_IS_EMBLEMED_ICON (emblemed), NULL);

  return emblemed->priv->icon;
}

/**
 * g_emblemed_icon_get_emblems:
 * @emblemed: a #xemblemed_icon_t
 *
 * Gets the list of emblems for the @icon.
 *
 * Returns: (element-type Gio.Emblem) (transfer none): a #xlist_t of
 *     #GEmblems that is owned by @emblemed
 *
 * Since: 2.18
 **/

xlist_t *
g_emblemed_icon_get_emblems (xemblemed_icon_t *emblemed)
{
  g_return_val_if_fail (X_IS_EMBLEMED_ICON (emblemed), NULL);

  return emblemed->priv->emblems;
}

/**
 * g_emblemed_icon_clear_emblems:
 * @emblemed: a #xemblemed_icon_t
 *
 * Removes all the emblems from @icon.
 *
 * Since: 2.28
 **/
void
g_emblemed_icon_clear_emblems (xemblemed_icon_t *emblemed)
{
  g_return_if_fail (X_IS_EMBLEMED_ICON (emblemed));

  if (emblemed->priv->emblems == NULL)
    return;

  xlist_free_full (emblemed->priv->emblems, xobject_unref);
  emblemed->priv->emblems = NULL;
}

static xint_t
xemblem_comp (xemblem_t *a,
               xemblem_t *b)
{
  xuint_t hash_a = xicon_hash (XICON (a));
  xuint_t hash_b = xicon_hash (XICON (b));

  if(hash_a < hash_b)
    return -1;

  if(hash_a == hash_b)
    return 0;

  return 1;
}

/**
 * g_emblemed_icon_add_emblem:
 * @emblemed: a #xemblemed_icon_t
 * @emblem: a #xemblem_t
 *
 * Adds @emblem to the #xlist_t of #GEmblems.
 *
 * Since: 2.18
 **/
void
g_emblemed_icon_add_emblem (xemblemed_icon_t *emblemed,
                            xemblem_t       *emblem)
{
  g_return_if_fail (X_IS_EMBLEMED_ICON (emblemed));
  g_return_if_fail (X_IS_EMBLEM (emblem));

  xobject_ref (emblem);
  emblemed->priv->emblems = xlist_insert_sorted (emblemed->priv->emblems, emblem,
                                                  (GCompareFunc) xemblem_comp);
}

static xuint_t
g_emblemed_icon_hash (xicon_t *icon)
{
  xemblemed_icon_t *emblemed = G_EMBLEMED_ICON (icon);
  xlist_t *list;
  xuint_t hash = xicon_hash (emblemed->priv->icon);

  for (list = emblemed->priv->emblems; list != NULL; list = list->next)
    hash ^= xicon_hash (XICON (list->data));

  return hash;
}

static xboolean_t
g_emblemed_icon_equal (xicon_t *icon1,
                       xicon_t *icon2)
{
  xemblemed_icon_t *emblemed1 = G_EMBLEMED_ICON (icon1);
  xemblemed_icon_t *emblemed2 = G_EMBLEMED_ICON (icon2);
  xlist_t *list1, *list2;

  if (!xicon_equal (emblemed1->priv->icon, emblemed2->priv->icon))
    return FALSE;

  list1 = emblemed1->priv->emblems;
  list2 = emblemed2->priv->emblems;

  while (list1 && list2)
  {
    if (!xicon_equal (XICON (list1->data), XICON (list2->data)))
        return FALSE;

    list1 = list1->next;
    list2 = list2->next;
  }

  return list1 == NULL && list2 == NULL;
}

static xboolean_t
g_emblemed_icon_to_tokens (xicon_t *icon,
                           xptr_array_t *tokens,
                           xint_t  *out_version)
{
  xemblemed_icon_t *emblemed_icon = G_EMBLEMED_ICON (icon);
  xlist_t *l;
  char *s;

  /* xemblemed_icon_ts are encoded as
   *
   *   <encoded_icon> [<encoded_emblem_icon>]*
   */

  g_return_val_if_fail (out_version != NULL, FALSE);

  *out_version = 0;

  s = xicon_to_string (emblemed_icon->priv->icon);
  if (s == NULL)
    return FALSE;

  xptr_array_add (tokens, s);

  for (l = emblemed_icon->priv->emblems; l != NULL; l = l->next)
    {
      xicon_t *emblem_icon = XICON (l->data);

      s = xicon_to_string (emblem_icon);
      if (s == NULL)
        return FALSE;

      xptr_array_add (tokens, s);
    }

  return TRUE;
}

static xicon_t *
g_emblemed_icon_from_tokens (xchar_t  **tokens,
                             xint_t     num_tokens,
                             xint_t     version,
                             xerror_t **error)
{
  xemblemed_icon_t *emblemed_icon;
  int n;

  emblemed_icon = NULL;

  if (version != 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Can’t handle version %d of xemblemed_icon_t encoding"),
                   version);
      goto fail;
    }

  if (num_tokens < 1)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Malformed number of tokens (%d) in xemblemed_icon_t encoding"),
                   num_tokens);
      goto fail;
    }

  emblemed_icon = xobject_new (XTYPE_EMBLEMED_ICON, NULL);
  emblemed_icon->priv->icon = xicon_new_for_string (tokens[0], error);
  if (emblemed_icon->priv->icon == NULL)
    goto fail;

  for (n = 1; n < num_tokens; n++)
    {
      xicon_t *emblem;

      emblem = xicon_new_for_string (tokens[n], error);
      if (emblem == NULL)
        goto fail;

      if (!X_IS_EMBLEM (emblem))
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Expected a xemblem_t for xemblemed_icon_t"));
          xobject_unref (emblem);
          goto fail;
        }

      emblemed_icon->priv->emblems = xlist_append (emblemed_icon->priv->emblems, emblem);
    }

  return XICON (emblemed_icon);

 fail:
  if (emblemed_icon != NULL)
    xobject_unref (emblemed_icon);
  return NULL;
}

static xvariant_t *
g_emblemed_icon_serialize (xicon_t *icon)
{
  xemblemed_icon_t *emblemed_icon = G_EMBLEMED_ICON (icon);
  xvariant_builder_t builder;
  xvariant_t *icon_data;
  xlist_t *node;

  icon_data = xicon_serialize (emblemed_icon->priv->icon);
  if (!icon_data)
    return NULL;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("(va(va{sv}))"));

  xvariant_builder_add (&builder, "v", icon_data);
  xvariant_unref (icon_data);

  xvariant_builder_open (&builder, G_VARIANT_TYPE ("a(va{sv})"));
  for (node = emblemed_icon->priv->emblems; node != NULL; node = node->next)
    {
      icon_data = xicon_serialize (node->data);
      if (icon_data)
        {
          /* We know how emblems serialize, so do a tweak here to
           * reduce some of the variant wrapping and redundant storage
           * of 'emblem' over and again...
           */
          if (xvariant_is_of_type (icon_data, G_VARIANT_TYPE ("(sv)")))
            {
              const xchar_t *name;
              xvariant_t *content;

              xvariant_get (icon_data, "(&sv)", &name, &content);

              if (xstr_equal (name, "emblem") && xvariant_is_of_type (content, G_VARIANT_TYPE ("(va{sv})")))
                xvariant_builder_add (&builder, "@(va{sv})", content);

              xvariant_unref (content);
            }

          xvariant_unref (icon_data);
        }
    }
  xvariant_builder_close (&builder);

  return xvariant_new ("(sv)", "emblemed", xvariant_builder_end (&builder));
}

static void
g_emblemed_icon_icon_iface_init (xicon_iface_t *iface)
{
  iface->hash = g_emblemed_icon_hash;
  iface->equal = g_emblemed_icon_equal;
  iface->to_tokens = g_emblemed_icon_to_tokens;
  iface->from_tokens = g_emblemed_icon_from_tokens;
  iface->serialize = g_emblemed_icon_serialize;
}
