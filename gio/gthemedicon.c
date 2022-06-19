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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include <string.h>

#include "gthemedicon.h"
#include "gicon.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gthemedicon
 * @short_description: Icon theming support
 * @include: gio/gio.h
 * @see_also: #xicon_t, #xloadable_icon_t
 *
 * #xthemed_icon_t is an implementation of #xicon_t that supports icon themes.
 * #xthemed_icon_t contains a list of all of the icons present in an icon
 * theme, so that icons can be looked up quickly. #xthemed_icon_t does
 * not provide actual pixmaps for icons, just the icon names.
 * Ideally something like gtk_icon_theme_choose_icon() should be used to
 * resolve the list of names so that fallback icons work nicely with
 * themes that inherit other themes.
 **/

static void g_themed_icon_icon_iface_init (xicon_iface_t *iface);

struct _GThemedIcon
{
  xobject_t parent_instance;

  char     **init_names;
  char     **names;
  xboolean_t   use_default_fallbacks;
};

struct _GThemedIconClass
{
  xobject_class_t parent_class;
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_NAMES,
  PROP_USE_DEFAULT_FALLBACKS
};

static void g_themed_icon_update_names (xthemed_icon_t *themed);

G_DEFINE_TYPE_WITH_CODE (xthemed_icon, g_themed_icon, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_ICON,
						g_themed_icon_icon_iface_init))

static void
g_themed_icon_get_property (xobject_t    *object,
                            xuint_t       prop_id,
                            xvalue_t     *value,
                            xparam_spec_t *pspec)
{
  xthemed_icon_t *icon = G_THEMED_ICON (object);

  switch (prop_id)
    {
      case PROP_NAMES:
        xvalue_set_boxed (value, icon->init_names);
        break;

      case PROP_USE_DEFAULT_FALLBACKS:
        xvalue_set_boolean (value, icon->use_default_fallbacks);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_themed_icon_set_property (xobject_t      *object,
                            xuint_t         prop_id,
                            const xvalue_t *value,
                            xparam_spec_t   *pspec)
{
  xthemed_icon_t *icon = G_THEMED_ICON (object);
  xchar_t **names;
  const xchar_t *name;

  switch (prop_id)
    {
      case PROP_NAME:
        name = xvalue_get_string (value);

        if (!name)
          break;

        if (icon->init_names)
          xstrfreev (icon->init_names);

        icon->init_names = g_new (char *, 2);
        icon->init_names[0] = xstrdup (name);
        icon->init_names[1] = NULL;
        break;

      case PROP_NAMES:
        names = xvalue_dup_boxed (value);

        if (!names)
          break;

        if (icon->init_names)
          xstrfreev (icon->init_names);

        icon->init_names = names;
        break;

      case PROP_USE_DEFAULT_FALLBACKS:
        icon->use_default_fallbacks = xvalue_get_boolean (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_themed_icon_constructed (xobject_t *object)
{
  g_themed_icon_update_names (G_THEMED_ICON (object));
}

static void
g_themed_icon_finalize (xobject_t *object)
{
  xthemed_icon_t *themed;

  themed = G_THEMED_ICON (object);

  xstrfreev (themed->init_names);
  xstrfreev (themed->names);

  XOBJECT_CLASS (g_themed_icon_parent_class)->finalize (object);
}

static void
g_themed_icon_class_init (GThemedIconClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = g_themed_icon_finalize;
  xobject_class->constructed = g_themed_icon_constructed;
  xobject_class->set_property = g_themed_icon_set_property;
  xobject_class->get_property = g_themed_icon_get_property;

  /**
   * xthemed_icon_t:name:
   *
   * The icon name.
   */
  xobject_class_install_property (xobject_class, PROP_NAME,
                                   xparam_spec_string ("name",
                                                        P_("name"),
                                                        P_("The name of the icon"),
                                                        NULL,
                                                        XPARAM_CONSTRUCT_ONLY | XPARAM_WRITABLE | XPARAM_STATIC_NAME | XPARAM_STATIC_BLURB | XPARAM_STATIC_NICK));

  /**
   * xthemed_icon_t:names:
   *
   * A %NULL-terminated array of icon names.
   */
  xobject_class_install_property (xobject_class, PROP_NAMES,
                                   xparam_spec_boxed ("names",
                                                       P_("names"),
                                                       P_("An array containing the icon names"),
                                                       XTYPE_STRV,
                                                       XPARAM_CONSTRUCT_ONLY | XPARAM_READWRITE | XPARAM_STATIC_NAME | XPARAM_STATIC_BLURB | XPARAM_STATIC_NICK));

  /**
   * xthemed_icon_t:use-default-fallbacks:
   *
   * Whether to use the default fallbacks found by shortening the icon name
   * at '-' characters. If the "names" array has more than one element,
   * ignores any past the first.
   *
   * For example, if the icon name was "gnome-dev-cdrom-audio", the array
   * would become
   * |[<!-- language="C" -->
   * {
   *   "gnome-dev-cdrom-audio",
   *   "gnome-dev-cdrom",
   *   "gnome-dev",
   *   "gnome",
   *   NULL
   * };
   * ]|
   */
  xobject_class_install_property (xobject_class, PROP_USE_DEFAULT_FALLBACKS,
                                   xparam_spec_boolean ("use-default-fallbacks",
                                                         P_("use default fallbacks"),
                                                         P_("Whether to use default fallbacks found by shortening the name at “-” characters. Ignores names after the first if multiple names are given."),
                                                         FALSE,
                                                         XPARAM_CONSTRUCT_ONLY | XPARAM_READWRITE | XPARAM_STATIC_NAME | XPARAM_STATIC_BLURB | XPARAM_STATIC_NICK));
}

static void
g_themed_icon_init (xthemed_icon_t *themed)
{
  themed->init_names = NULL;
  themed->names      = NULL;
}

/**
 * g_themed_icon_update_names:
 * @themed: a #xthemed_icon_t.
 *
 * Update the actual icon name list, based on the requested names (from
 * construction, or later added with g_themed_icon_prepend_name() and
 * g_themed_icon_append_name()).
 * The order of the list matters, indicating priority:
 * - The first requested icon is first in priority.
 * - If "use-default-fallbacks" is #TRUE, then it is followed by all its
 *   fallbacks (starting from top to lower context levels).
 * - Then next requested icons, and optionally their fallbacks, follow.
 * - Finally all the style variants (symbolic or regular, opposite to whatever
 *   is the requested style) follow in the same order.
 *
 * An icon is not added twice in the list if it was previously added.
 *
 * For instance, if requested names are:
 * [ "some-icon-symbolic", "some-other-icon" ]
 * and use-default-fallbacks is TRUE, the final name list shall be:
 * [ "some-icon-symbolic", "some-symbolic", "some-other-icon",
 *   "some-other", "some", "some-icon", "some-other-icon-symbolic",
 *   "some-other-symbolic" ]
 *
 * Returns: (transfer full) (type xthemed_icon_t): a new #xthemed_icon_t
 **/
static void
g_themed_icon_update_names (xthemed_icon_t *themed)
{
  xlist_t *names    = NULL;
  xlist_t *variants = NULL;
  xlist_t *iter;
  xuint_t  i;

  g_return_if_fail (themed->init_names != NULL && themed->init_names[0] != NULL);

  for (i = 0; themed->init_names[i]; i++)
    {
      xchar_t    *name;
      xboolean_t  is_symbolic;

      is_symbolic = xstr_has_suffix (themed->init_names[i], "-symbolic");
      if (is_symbolic)
        name = xstrndup (themed->init_names[i], strlen (themed->init_names[i]) - 9);
      else
        name = xstrdup (themed->init_names[i]);

      if (xlist_find_custom (names, name, (GCompareFunc) xstrcmp0))
        {
          g_free (name);
          continue;
        }

      if (is_symbolic)
        names = xlist_prepend (names, xstrdup (themed->init_names[i]));
      else
        names = xlist_prepend (names, name);

      if (themed->use_default_fallbacks)
        {
          char *dashp;
          char *last;

          last = name;

          while ((dashp = strrchr (last, '-')) != NULL)
            {
              xchar_t *tmp = last;
              xchar_t *fallback;

              last = xstrndup (last, dashp - last);
              if (is_symbolic)
                {
                  g_free (tmp);
                  fallback = xstrdup_printf ("%s-symbolic", last);
                }
              else
                fallback = last;
              if (xlist_find_custom (names, fallback, (GCompareFunc) xstrcmp0))
                {
                  g_free (fallback);
                  break;
                }
              names = xlist_prepend (names, fallback);
            }
          if (is_symbolic)
            g_free (last);
        }
      else if (is_symbolic)
        g_free (name);
    }
  for (iter = names; iter; iter = iter->next)
    {
      xchar_t    *name = (xchar_t *) iter->data;
      xchar_t    *variant;
      xboolean_t  is_symbolic;

      is_symbolic = xstr_has_suffix (name, "-symbolic");
      if (is_symbolic)
        variant = xstrndup (name, strlen (name) - 9);
      else
        variant = xstrdup_printf ("%s-symbolic", name);
      if (xlist_find_custom (names, variant, (GCompareFunc) xstrcmp0) ||
          xlist_find_custom (variants, variant, (GCompareFunc) xstrcmp0))
        {
          g_free (variant);
          continue;
        }

      variants = xlist_prepend (variants, variant);
    }
  names = xlist_reverse (names);

  xstrfreev (themed->names);
  themed->names = g_new (char *, xlist_length (names) + xlist_length (variants) + 1);

  for (iter = names, i = 0; iter; iter = iter->next, i++)
    themed->names[i] = iter->data;
  for (iter = variants; iter; iter = iter->next, i++)
    themed->names[i] = iter->data;
  themed->names[i] = NULL;

  xlist_free (names);
  xlist_free (variants);

  xobject_notify (G_OBJECT (themed), "names");
}

/**
 * g_themed_icon_new:
 * @iconname: a string containing an icon name.
 *
 * Creates a new themed icon for @iconname.
 *
 * Returns: (transfer full) (type xthemed_icon_t): a new #xthemed_icon_t.
 **/
xicon_t *
g_themed_icon_new (const char *iconname)
{
  xreturn_val_if_fail (iconname != NULL, NULL);

  return XICON (xobject_new (XTYPE_THEMED_ICON, "name", iconname, NULL));
}

/**
 * g_themed_icon_new_from_names:
 * @iconnames: (array length=len): an array of strings containing icon names.
 * @len: the length of the @iconnames array, or -1 if @iconnames is
 *     %NULL-terminated
 *
 * Creates a new themed icon for @iconnames.
 *
 * Returns: (transfer full) (type xthemed_icon_t): a new #xthemed_icon_t
 **/
xicon_t *
g_themed_icon_new_from_names (char **iconnames,
                              int    len)
{
  xicon_t *icon;

  xreturn_val_if_fail (iconnames != NULL, NULL);

  if (len >= 0)
    {
      char **names;
      int i;

      names = g_new (char *, len + 1);

      for (i = 0; i < len; i++)
        names[i] = iconnames[i];

      names[i] = NULL;

      icon = XICON (xobject_new (XTYPE_THEMED_ICON, "names", names, NULL));

      g_free (names);
    }
  else
    icon = XICON (xobject_new (XTYPE_THEMED_ICON, "names", iconnames, NULL));

  return icon;
}

/**
 * g_themed_icon_new_with_default_fallbacks:
 * @iconname: a string containing an icon name
 *
 * Creates a new themed icon for @iconname, and all the names
 * that can be created by shortening @iconname at '-' characters.
 *
 * In the following example, @icon1 and @icon2 are equivalent:
 * |[<!-- language="C" -->
 * const char *names[] = {
 *   "gnome-dev-cdrom-audio",
 *   "gnome-dev-cdrom",
 *   "gnome-dev",
 *   "gnome"
 * };
 *
 * icon1 = g_themed_icon_new_from_names (names, 4);
 * icon2 = g_themed_icon_new_with_default_fallbacks ("gnome-dev-cdrom-audio");
 * ]|
 *
 * Returns: (transfer full) (type xthemed_icon_t): a new #xthemed_icon_t.
 */
xicon_t *
g_themed_icon_new_with_default_fallbacks (const char *iconname)
{
  xreturn_val_if_fail (iconname != NULL, NULL);

  return XICON (xobject_new (XTYPE_THEMED_ICON, "name", iconname, "use-default-fallbacks", TRUE, NULL));
}


/**
 * g_themed_icon_get_names:
 * @icon: a #xthemed_icon_t.
 *
 * Gets the names of icons from within @icon.
 *
 * Returns: (transfer none): a list of icon names.
 */
const char * const *
g_themed_icon_get_names (xthemed_icon_t *icon)
{
  xreturn_val_if_fail (X_IS_THEMED_ICON (icon), NULL);
  return (const char * const *)icon->names;
}

/**
 * g_themed_icon_append_name:
 * @icon: a #xthemed_icon_t
 * @iconname: name of icon to append to list of icons from within @icon.
 *
 * Append a name to the list of icons from within @icon.
 *
 * Note that doing so invalidates the hash computed by prior calls
 * to xicon_hash().
 */
void
g_themed_icon_append_name (xthemed_icon_t *icon,
                           const char  *iconname)
{
  xuint_t num_names;

  g_return_if_fail (X_IS_THEMED_ICON (icon));
  g_return_if_fail (iconname != NULL);

  num_names = xstrv_length (icon->init_names);
  icon->init_names = g_realloc (icon->init_names, sizeof (char*) * (num_names + 2));
  icon->init_names[num_names] = xstrdup (iconname);
  icon->init_names[num_names + 1] = NULL;

  g_themed_icon_update_names (icon);
}

/**
 * g_themed_icon_prepend_name:
 * @icon: a #xthemed_icon_t
 * @iconname: name of icon to prepend to list of icons from within @icon.
 *
 * Prepend a name to the list of icons from within @icon.
 *
 * Note that doing so invalidates the hash computed by prior calls
 * to xicon_hash().
 *
 * Since: 2.18
 */
void
g_themed_icon_prepend_name (xthemed_icon_t *icon,
                            const char  *iconname)
{
  xuint_t num_names;
  xchar_t **names;
  xint_t i;

  g_return_if_fail (X_IS_THEMED_ICON (icon));
  g_return_if_fail (iconname != NULL);

  num_names = xstrv_length (icon->init_names);
  names = g_new (char*, num_names + 2);
  for (i = 0; icon->init_names[i]; i++)
    names[i + 1] = icon->init_names[i];
  names[0] = xstrdup (iconname);
  names[num_names + 1] = NULL;

  g_free (icon->init_names);
  icon->init_names = names;

  g_themed_icon_update_names (icon);
}

static xuint_t
g_themed_icon_hash (xicon_t *icon)
{
  xthemed_icon_t *themed = G_THEMED_ICON (icon);
  xuint_t hash;
  int i;

  hash = 0;

  for (i = 0; themed->names[i] != NULL; i++)
    hash ^= xstr_hash (themed->names[i]);

  return hash;
}

static xboolean_t
g_themed_icon_equal (xicon_t *icon1,
                     xicon_t *icon2)
{
  xthemed_icon_t *themed1 = G_THEMED_ICON (icon1);
  xthemed_icon_t *themed2 = G_THEMED_ICON (icon2);
  int i;

  for (i = 0; themed1->names[i] != NULL && themed2->names[i] != NULL; i++)
    {
      if (!xstr_equal (themed1->names[i], themed2->names[i]))
	return FALSE;
    }

  return themed1->names[i] == NULL && themed2->names[i] == NULL;
}


static xboolean_t
g_themed_icon_to_tokens (xicon_t *icon,
			 xptr_array_t *tokens,
                         xint_t  *out_version)
{
  xthemed_icon_t *themed_icon = G_THEMED_ICON (icon);
  int n;

  xreturn_val_if_fail (out_version != NULL, FALSE);

  *out_version = 0;

  for (n = 0; themed_icon->names[n] != NULL; n++)
    xptr_array_add (tokens,
		     xstrdup (themed_icon->names[n]));

  return TRUE;
}

static xicon_t *
g_themed_icon_from_tokens (xchar_t  **tokens,
                           xint_t     num_tokens,
                           xint_t     version,
                           xerror_t **error)
{
  xicon_t *icon;
  xchar_t **names;
  int n;

  icon = NULL;

  if (version != 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Can’t handle version %d of xthemed_icon_t encoding"),
                   version);
      goto out;
    }

  names = g_new0 (xchar_t *, num_tokens + 1);
  for (n = 0; n < num_tokens; n++)
    names[n] = tokens[n];
  names[n] = NULL;

  icon = g_themed_icon_new_from_names (names, num_tokens);
  g_free (names);

 out:
  return icon;
}

static xvariant_t *
g_themed_icon_serialize (xicon_t *icon)
{
  xthemed_icon_t *themed_icon = G_THEMED_ICON (icon);

  return xvariant_new ("(sv)", "themed", xvariant_new ("^as", themed_icon->names));
}

static void
g_themed_icon_icon_iface_init (xicon_iface_t *iface)
{
  iface->hash = g_themed_icon_hash;
  iface->equal = g_themed_icon_equal;
  iface->to_tokens = g_themed_icon_to_tokens;
  iface->from_tokens = g_themed_icon_from_tokens;
  iface->serialize = g_themed_icon_serialize;
}
