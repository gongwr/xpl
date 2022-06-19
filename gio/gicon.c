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
#include <stdlib.h>
#include <string.h>

#include "gicon.h"
#include "gthemedicon.h"
#include "gfileicon.h"
#include "gemblemedicon.h"
#include "gbytesicon.h"
#include "gfile.h"
#include "gioerror.h"
#include "gioenumtypes.h"
#include "gvfs.h"

#include "glibintl.h"


/* There versioning of this is implicit, version 1 would be ".1 " */
#define XICON_SERIALIZATION_MAGIC0 ". "

/**
 * SECTION:gicon
 * @short_description: Interface for icons
 * @include: gio/gio.h
 *
 * #xicon_t is a very minimal interface for icons. It provides functions
 * for checking the equality of two icons, hashing of icons and
 * serializing an icon to and from strings.
 *
 * #xicon_t does not provide the actual pixmap for the icon as this is out
 * of GIO's scope, however implementations of #xicon_t may contain the name
 * of an icon (see #xthemed_icon_t), or the path to an icon (see #xloadable_icon_t).
 *
 * To obtain a hash of a #xicon_t, see xicon_hash().
 *
 * To check if two #GIcons are equal, see xicon_equal().
 *
 * For serializing a #xicon_t, use xicon_serialize() and
 * xicon_deserialize().
 *
 * If you want to consume #xicon_t (for example, in a toolkit) you must
 * be prepared to handle at least the three following cases:
 * #xloadable_icon_t, #xthemed_icon_t and #xemblemed_icon_t.  It may also make
 * sense to have fast-paths for other cases (like handling #GdkPixbuf
 * directly, for example) but all compliant #xicon_t implementations
 * outside of GIO must implement #xloadable_icon_t.
 *
 * If your application or library provides one or more #xicon_t
 * implementations you need to ensure that your new implementation also
 * implements #xloadable_icon_t.  Additionally, you must provide an
 * implementation of xicon_serialize() that gives a result that is
 * understood by xicon_deserialize(), yielding one of the built-in icon
 * types.
 **/

typedef xicon_iface_t GIconInterface;
G_DEFINE_INTERFACE(xicon_t, xicon, XTYPE_OBJECT)

static void
xicon_default_init (GIconInterface *iface)
{
}

/**
 * xicon_hash:
 * @icon: (not nullable): #xconstpointer to an icon object.
 *
 * Gets a hash for an icon.
 *
 * Virtual: hash
 * Returns: a #xuint_t containing a hash for the @icon, suitable for
 * use in a #xhashtable_t or similar data structure.
 **/
xuint_t
xicon_hash (xconstpointer icon)
{
  xicon_iface_t *iface;

  xreturn_val_if_fail (X_IS_ICON (icon), 0);

  iface = XICON_GET_IFACE (icon);

  return (* iface->hash) ((xicon_t *)icon);
}

/**
 * xicon_equal:
 * @icon1: (nullable): pointer to the first #xicon_t.
 * @icon2: (nullable): pointer to the second #xicon_t.
 *
 * Checks if two icons are equal.
 *
 * Returns: %TRUE if @icon1 is equal to @icon2. %FALSE otherwise.
 **/
xboolean_t
xicon_equal (xicon_t *icon1,
	      xicon_t *icon2)
{
  xicon_iface_t *iface;

  if (icon1 == NULL && icon2 == NULL)
    return TRUE;

  if (icon1 == NULL || icon2 == NULL)
    return FALSE;

  if (XTYPE_FROM_INSTANCE (icon1) != XTYPE_FROM_INSTANCE (icon2))
    return FALSE;

  iface = XICON_GET_IFACE (icon1);

  return (* iface->equal) (icon1, icon2);
}

static xboolean_t
xicon_to_string_tokenized (xicon_t *icon, xstring_t *s)
{
  xptr_array_t *tokens;
  xint_t version;
  xicon_iface_t *icon_iface;
  xuint_t i;

  xreturn_val_if_fail (icon != NULL, FALSE);
  xreturn_val_if_fail (X_IS_ICON (icon), FALSE);

  icon_iface = XICON_GET_IFACE (icon);
  if (icon_iface->to_tokens == NULL)
    return FALSE;

  tokens = xptr_array_new ();
  if (!icon_iface->to_tokens (icon, tokens, &version))
    {
      xptr_array_free (tokens, TRUE);
      return FALSE;
    }

  /* format: TypeName[.Version] <token_0> .. <token_N-1>
     version 0 is implicit and can be omitted
     all the tokens are url escaped to ensure they have no spaces in them */

  xstring_append (s, xtype_name_from_instance ((GTypeInstance *)icon));
  if (version != 0)
    xstring_append_printf (s, ".%d", version);

  for (i = 0; i < tokens->len; i++)
    {
      char *token;

      token = xptr_array_index (tokens, i);

      xstring_append_c (s, ' ');
      /* We really only need to escape spaces here, so allow lots of otherwise reserved chars */
      xstring_append_uri_escaped (s, token,
				   XURI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);

      g_free (token);
    }

  xptr_array_free (tokens, TRUE);

  return TRUE;
}

/**
 * xicon_to_string:
 * @icon: a #xicon_t.
 *
 * Generates a textual representation of @icon that can be used for
 * serialization such as when passing @icon to a different process or
 * saving it to persistent storage. Use xicon_new_for_string() to
 * get @icon back from the returned string.
 *
 * The encoding of the returned string is proprietary to #xicon_t except
 * in the following two cases
 *
 * - If @icon is a #xfile_icon_t, the returned string is a native path
 *   (such as `/path/to/my icon.png`) without escaping
 *   if the #xfile_t for @icon is a native file.  If the file is not
 *   native, the returned string is the result of xfile_get_uri()
 *   (such as `sftp://path/to/my%20icon.png`).
 *
 * - If @icon is a #xthemed_icon_t with exactly one name and no fallbacks,
 *   the encoding is simply the name (such as `network-server`).
 *
 * Virtual: to_tokens
 * Returns: (nullable): An allocated NUL-terminated UTF8 string or
 * %NULL if @icon can't be serialized. Use g_free() to free.
 *
 * Since: 2.20
 */
xchar_t *
xicon_to_string (xicon_t *icon)
{
  xchar_t *ret;

  xreturn_val_if_fail (icon != NULL, NULL);
  xreturn_val_if_fail (X_IS_ICON (icon), NULL);

  ret = NULL;

  if (X_IS_FILE_ICON (icon))
    {
      xfile_t *file;

      file = xfile_icon_get_file (XFILE_ICON (icon));
      if (xfile_is_native (file))
	{
	  ret = xfile_get_path (file);
	  if (!xutf8_validate (ret, -1, NULL))
	    {
	      g_free (ret);
	      ret = NULL;
	    }
	}
      else
        ret = xfile_get_uri (file);
    }
  else if (X_IS_THEMED_ICON (icon))
    {
      char     **names                 = NULL;
      xboolean_t   use_default_fallbacks = FALSE;

      xobject_get (G_OBJECT (icon),
                    "names",                 &names,
                    "use-default-fallbacks", &use_default_fallbacks,
                    NULL);
      /* Themed icon initialized with a single name and no fallbacks. */
      if (names != NULL &&
	  names[0] != NULL &&
	  names[0][0] != '.' && /* Allowing icons starting with dot would break XICON_SERIALIZATION_MAGIC0 */
	  xutf8_validate (names[0], -1, NULL) && /* Only return utf8 strings */
          names[1] == NULL &&
          ! use_default_fallbacks)
	ret = xstrdup (names[0]);

      xstrfreev (names);
    }

  if (ret == NULL)
    {
      xstring_t *s;

      s = xstring_new (XICON_SERIALIZATION_MAGIC0);

      if (xicon_to_string_tokenized (icon, s))
	ret = xstring_free (s, FALSE);
      else
	xstring_free (s, TRUE);
    }

  return ret;
}

static xicon_t *
xicon_new_from_tokens (char   **tokens,
			xerror_t **error)
{
  xicon_t *icon;
  char *typename, *version_str;
  xtype_t type;
  xpointer_t klass;
  xicon_iface_t *icon_iface;
  xint_t version;
  char *endp;
  int num_tokens;
  int i;

  icon = NULL;
  klass = NULL;

  num_tokens = xstrv_length (tokens);

  if (num_tokens < 1)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Wrong number of tokens (%d)"),
                   num_tokens);
      goto out;
    }

  typename = tokens[0];
  version_str = strchr (typename, '.');
  if (version_str)
    {
      *version_str = 0;
      version_str += 1;
    }


  type = xtype_from_name (tokens[0]);
  if (type == 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("No type for class name %s"),
                   tokens[0]);
      goto out;
    }

  if (!xtype_is_a (type, XTYPE_ICON))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Type %s does not implement the xicon_t interface"),
                   tokens[0]);
      goto out;
    }

  klass = xtype_class_ref (type);
  if (klass == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Type %s is not classed"),
                   tokens[0]);
      goto out;
    }

  version = 0;
  if (version_str)
    {
      version = strtol (version_str, &endp, 10);
      if (endp == NULL || *endp != '\0')
	{
	  g_set_error (error,
		       G_IO_ERROR,
		       G_IO_ERROR_INVALID_ARGUMENT,
		       _("Malformed version number: %s"),
		       version_str);
	  goto out;
	}
    }

  icon_iface = xtype_interface_peek (klass, XTYPE_ICON);
  xassert (icon_iface != NULL);

  if (icon_iface->from_tokens == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Type %s does not implement from_tokens() on the xicon_t interface"),
                   tokens[0]);
      goto out;
    }

  for (i = 1;  i < num_tokens; i++)
    {
      char *escaped;

      escaped = tokens[i];
      tokens[i] = xuri_unescape_string (escaped, NULL);
      g_free (escaped);
    }

  icon = icon_iface->from_tokens (tokens + 1, num_tokens - 1, version, error);

 out:
  if (klass != NULL)
    xtype_class_unref (klass);
  return icon;
}

static void
ensure_builtin_icon_types (void)
{
  xtype_ensure (XTYPE_THEMED_ICON);
  xtype_ensure (XTYPE_FILE_ICON);
  xtype_ensure (XTYPE_EMBLEMED_ICON);
  xtype_ensure (XTYPE_EMBLEM);
}

/* handles the 'simple' cases: xfile_icon_t and xthemed_icon_t */
static xicon_t *
xicon_new_for_string_simple (const xchar_t *str)
{
  xchar_t *scheme;
  xicon_t *icon;

  if (str[0] == '.')
    return NULL;

  /* handle special xfile_icon_t and xthemed_icon_t cases */
  scheme = xuri_parse_scheme (str);
  if (scheme != NULL || str[0] == '/' || str[0] == G_DIR_SEPARATOR)
    {
      xfile_t *location;
      location = xfile_new_for_commandline_arg (str);
      icon = xfile_icon_new (location);
      xobject_unref (location);
    }
  else
    icon = g_themed_icon_new (str);

  g_free (scheme);

  return icon;
}

/**
 * xicon_new_for_string:
 * @str: A string obtained via xicon_to_string().
 * @error: Return location for error.
 *
 * Generate a #xicon_t instance from @str. This function can fail if
 * @str is not valid - see xicon_to_string() for discussion.
 *
 * If your application or library provides one or more #xicon_t
 * implementations you need to ensure that each #xtype_t is registered
 * with the type system prior to calling xicon_new_for_string().
 *
 * Returns: (transfer full): An object implementing the #xicon_t
 *          interface or %NULL if @error is set.
 *
 * Since: 2.20
 **/
xicon_t *
xicon_new_for_string (const xchar_t   *str,
                       xerror_t       **error)
{
  xicon_t *icon = NULL;

  xreturn_val_if_fail (str != NULL, NULL);

  icon = xicon_new_for_string_simple (str);
  if (icon)
    return icon;

  ensure_builtin_icon_types ();

  if (xstr_has_prefix (str, XICON_SERIALIZATION_MAGIC0))
    {
      xchar_t **tokens;

      /* handle tokenized encoding */
      tokens = xstrsplit (str + sizeof (XICON_SERIALIZATION_MAGIC0) - 1, " ", 0);
      icon = xicon_new_from_tokens (tokens, error);
      xstrfreev (tokens);
    }
  else
    g_set_error_literal (error,
                         G_IO_ERROR,
                         G_IO_ERROR_INVALID_ARGUMENT,
                         _("Can’t handle the supplied version of the icon encoding"));

  return icon;
}

static xemblem_t *
xicon_deserialize_emblem (xvariant_t *value)
{
  xvariant_t *emblem_metadata;
  xvariant_t *emblem_data;
  const xchar_t *origin_nick;
  xicon_t *emblem_icon;
  xemblem_t *emblem;

  xvariant_get (value, "(v@a{sv})", &emblem_data, &emblem_metadata);

  emblem = NULL;

  emblem_icon = xicon_deserialize (emblem_data);
  if (emblem_icon != NULL)
    {
      /* Check if we should create it with an origin. */
      if (xvariant_lookup (emblem_metadata, "origin", "&s", &origin_nick))
        {
          xenum_class_t *origin_class;
          xenum_value_t *origin_value;

          origin_class = xtype_class_ref (XTYPE_EMBLEM_ORIGIN);
          origin_value = xenum_get_value_by_nick (origin_class, origin_nick);
          if (origin_value)
            emblem = xemblem_new_with_origin (emblem_icon, origin_value->value);
          xtype_class_unref (origin_class);
        }

      /* We didn't create it with an origin, so do it without. */
      if (emblem == NULL)
        emblem = xemblem_new (emblem_icon);

      xobject_unref (emblem_icon);
    }

  xvariant_unref (emblem_metadata);
  xvariant_unref (emblem_data);

  return emblem;
}

static xicon_t *
xicon_deserialize_emblemed (xvariant_t *value)
{
  xvariant_iter_t *emblems;
  xvariant_t *icon_data;
  xicon_t *main_icon;
  xicon_t *icon;

  xvariant_get (value, "(va(va{sv}))", &icon_data, &emblems);
  main_icon = xicon_deserialize (icon_data);

  if (main_icon)
    {
      xvariant_t *emblem_data;

      icon = g_emblemed_icon_new (main_icon, NULL);

      while ((emblem_data = xvariant_iter_next_value (emblems)))
        {
          xemblem_t *emblem;

          emblem = xicon_deserialize_emblem (emblem_data);

          if (emblem)
            {
              g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (icon), emblem);
              xobject_unref (emblem);
            }

          xvariant_unref (emblem_data);
        }

      xobject_unref (main_icon);
    }
  else
    icon = NULL;

  xvariant_iter_free (emblems);
  xvariant_unref (icon_data);

  return icon;
}

/**
 * xicon_deserialize:
 * @value: (transfer none): a #xvariant_t created with xicon_serialize()
 *
 * Deserializes a #xicon_t previously serialized using xicon_serialize().
 *
 * Returns: (nullable) (transfer full): a #xicon_t, or %NULL when deserialization fails.
 *
 * Since: 2.38
 */
xicon_t *
xicon_deserialize (xvariant_t *value)
{
  const xchar_t *tag;
  xvariant_t *val;
  xicon_t *icon;

  xreturn_val_if_fail (value != NULL, NULL);
  xreturn_val_if_fail (xvariant_is_of_type (value, G_VARIANT_TYPE_STRING) ||
                        xvariant_is_of_type (value, G_VARIANT_TYPE ("(sv)")), NULL);

  /* Handle some special cases directly so that people can hard-code
   * stuff into xmenu_model_t xml files without resorting to using xvariant_t
   * text format to describe one of the explicitly-tagged possibilities
   * below.
   */
  if (xvariant_is_of_type (value, G_VARIANT_TYPE_STRING))
    return xicon_new_for_string_simple (xvariant_get_string (value, NULL));

  /* Otherwise, use the tagged union format */
  xvariant_get (value, "(&sv)", &tag, &val);

  icon = NULL;

  if (xstr_equal (tag, "file") && xvariant_is_of_type (val, G_VARIANT_TYPE_STRING))
    {
      xfile_t *file;

      file = xfile_new_for_commandline_arg (xvariant_get_string (val, NULL));
      icon = xfile_icon_new (file);
      xobject_unref (file);
    }
  else if (xstr_equal (tag, "themed") && xvariant_is_of_type (val, G_VARIANT_TYPE_STRING_ARRAY))
    {
      const xchar_t **names;
      xsize_t size;

      names = xvariant_get_strv (val, &size);
      icon = g_themed_icon_new_from_names ((xchar_t **) names, size);
      g_free (names);
    }
  else if (xstr_equal (tag, "bytes") && xvariant_is_of_type (val, G_VARIANT_TYPE_BYTESTRING))
    {
      xbytes_t *bytes;

      bytes = xvariant_get_data_as_bytes (val);
      icon = xbytes_icon_new (bytes);
      xbytes_unref (bytes);
    }
  else if (xstr_equal (tag, "emblem") && xvariant_is_of_type (val, G_VARIANT_TYPE ("(va{sv})")))
    {
      xemblem_t *emblem;

      emblem = xicon_deserialize_emblem (val);
      if (emblem)
        icon = XICON (emblem);
    }
  else if (xstr_equal (tag, "emblemed") && xvariant_is_of_type (val, G_VARIANT_TYPE ("(va(va{sv}))")))
    {
      icon = xicon_deserialize_emblemed (val);
    }
  else if (xstr_equal (tag, "gvfs"))
    {
      xvfs_class_t *class;
      xvfs_t *vfs;

      vfs = xvfs_get_default ();
      class = XVFS_GET_CLASS (vfs);
      if (class->deserialize_icon)
        icon = (* class->deserialize_icon) (vfs, val);
    }

  xvariant_unref (val);

  return icon;
}

/**
 * xicon_serialize:
 * @icon: a #xicon_t
 *
 * Serializes a #xicon_t into a #xvariant_t. An equivalent #xicon_t can be retrieved
 * back by calling xicon_deserialize() on the returned value.
 * As serialization will avoid using raw icon data when possible, it only
 * makes sense to transfer the #xvariant_t between processes on the same machine,
 * (as opposed to over the network), and within the same file system namespace.
 *
 * Returns: (nullable) (transfer full): a #xvariant_t, or %NULL when serialization fails. The #xvariant_t will not be floating.
 *
 * Since: 2.38
 */
xvariant_t *
xicon_serialize (xicon_t *icon)
{
  GIconInterface *iface;
  xvariant_t *result;

  iface = XICON_GET_IFACE (icon);

  if (!iface->serialize)
    {
      g_critical ("xicon_serialize() on icon type '%s' is not implemented", G_OBJECT_TYPE_NAME (icon));
      return NULL;
    }

  result = (* iface->serialize) (icon);

  if (result)
    {
      xvariant_take_ref (result);

      if (!xvariant_is_of_type (result, G_VARIANT_TYPE ("(sv)")))
        {
          g_critical ("xicon_serialize() on icon type '%s' returned xvariant_t of type '%s' but it must return "
                      "one with type '(sv)'", G_OBJECT_TYPE_NAME (icon), xvariant_get_type_string (result));
          xvariant_unref (result);
          result = NULL;
        }
    }

  return result;
}
