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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gcontenttypeprivate.h"
#include "gthemedicon.h"
#include "gicon.h"
#include "gfile.h"
#include "gfileenumerator.h"
#include "gfileinfo.h"
#include "glibintl.h"
#include "glib-private.h"


/**
 * SECTION:gcontenttype
 * @short_description: Platform-specific content typing
 * @include: gio/gio.h
 *
 * A content type is a platform specific string that defines the type
 * of a file. On UNIX it is a
 * [MIME type](http://www.wikipedia.org/wiki/Internet_media_type)
 * like `text/plain` or `image/png`.
 * On Win32 it is an extension string like `.doc`, `.txt` or a perceived
 * string like `audio`. Such strings can be looked up in the registry at
 * `HKEY_CLASSES_ROOT`.
 * On macOS it is a [Uniform Type Identifier](https://en.wikipedia.org/wiki/Uniform_Type_Identifier)
 * such as `com.apple.application`.
 **/

#include <dirent.h>

#define XDG_PREFIX _gio_xdg
#include "xdgmime/xdgmime.h"

static void tree_magic_schedule_reload (void);

/* We lock this mutex whenever we modify global state in this module.
 * Taking and releasing this lock should always be associated with a pair of
 * g_begin_ignore_leaks()/g_end_ignore_leaks() calls, as any call into xdgmime
 * could trigger xdg_mime_init(), which makes a number of one-time allocations
 * which GLib can never free as it doesn’t know when is suitable to call
 * xdg_mime_shutdown(). */
G_LOCK_DEFINE_STATIC (gio_xdgmime);

xsize_t
_g_unix_content_type_get_sniff_len (void)
{
  xsize_t size;

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  size = xdg_mime_get_max_buffer_extents ();
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  return size;
}

xchar_t *
_g_unix_content_type_unalias (const xchar_t *type)
{
  xchar_t *res;

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  res = xstrdup (xdg_mime_unalias_mime_type (type));
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  return res;
}

xchar_t **
_g_unix_content_type_get_parents (const xchar_t *type)
{
  const xchar_t *umime;
  xchar_t **parents;
  xptr_array_t *array;
  int i;

  array = xptr_array_new ();

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();

  umime = xdg_mime_unalias_mime_type (type);

  xptr_array_add (array, xstrdup (umime));

  parents = xdg_mime_list_mime_parents (umime);
  for (i = 0; parents && parents[i] != NULL; i++)
    xptr_array_add (array, xstrdup (parents[i]));

  free (parents);

  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  xptr_array_add (array, NULL);

  return (xchar_t **)xptr_array_free (array, FALSE);
}

G_LOCK_DEFINE_STATIC (global_mime_dirs);
static xchar_t **global_mime_dirs = NULL;

static void
_g_content_type_set_mime_dirs_locked (const char * const *dirs)
{
  g_clear_pointer (&global_mime_dirs, xstrfreev);

  if (dirs != NULL)
    {
      global_mime_dirs = xstrdupv ((xchar_t **) dirs);
    }
  else
    {
      xptr_array_t *mime_dirs = xptr_array_new_with_free_func (g_free);
      const xchar_t * const *system_dirs = g_get_system_data_dirs ();

      xptr_array_add (mime_dirs, g_build_filename (g_get_user_data_dir (), "mime", NULL));
      for (; *system_dirs != NULL; system_dirs++)
        xptr_array_add (mime_dirs, g_build_filename (*system_dirs, "mime", NULL));
      xptr_array_add (mime_dirs, NULL);  /* NULL terminator */

      global_mime_dirs = (xchar_t **) xptr_array_free (mime_dirs, FALSE);
    }

  xdg_mime_set_dirs ((const xchar_t * const *) global_mime_dirs);
  tree_magic_schedule_reload ();
}

/**
 * g_content_type_set_mime_dirs:
 * @dirs: (array zero-terminated=1) (nullable): %NULL-terminated list of
 *    directories to load MIME data from, including any `mime/` subdirectory,
 *    and with the first directory to try listed first
 *
 * Set the list of directories used by GIO to load the MIME database.
 * If @dirs is %NULL, the directories used are the default:
 *
 *  - the `mime` subdirectory of the directory in `$XDG_DATA_HOME`
 *  - the `mime` subdirectory of every directory in `$XDG_DATA_DIRS`
 *
 * This function is intended to be used when writing tests that depend on
 * information stored in the MIME database, in order to control the data.
 *
 * Typically, in case your tests use %G_TEST_OPTION_ISOLATE_DIRS, but they
 * depend on the system’s MIME database, you should call this function
 * with @dirs set to %NULL before calling g_test_init(), for instance:
 *
 * |[<!-- language="C" -->
 *   // Load MIME data from the system
 *   g_content_type_set_mime_dirs (NULL);
 *   // Isolate the environment
 *   g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
 *
 *   …
 *
 *   return g_test_run ();
 * ]|
 *
 * Since: 2.60
 */
/*< private >*/
void
g_content_type_set_mime_dirs (const xchar_t * const *dirs)
{
  G_LOCK (global_mime_dirs);
  _g_content_type_set_mime_dirs_locked (dirs);
  G_UNLOCK (global_mime_dirs);
}

/**
 * g_content_type_get_mime_dirs:
 *
 * Get the list of directories which MIME data is loaded from. See
 * g_content_type_set_mime_dirs() for details.
 *
 * Returns: (transfer none) (array zero-terminated=1): %NULL-terminated list of
 *    directories to load MIME data from, including any `mime/` subdirectory,
 *    and with the first directory to try listed first
 * Since: 2.60
 */
/*< private >*/
const xchar_t * const *
g_content_type_get_mime_dirs (void)
{
  const xchar_t * const *mime_dirs;

  G_LOCK (global_mime_dirs);

  if (global_mime_dirs == NULL)
    _g_content_type_set_mime_dirs_locked (NULL);

  mime_dirs = (const xchar_t * const *) global_mime_dirs;

  G_UNLOCK (global_mime_dirs);

  g_assert (mime_dirs != NULL);
  return mime_dirs;
}

/**
 * g_content_type_equals:
 * @type1: a content type string
 * @type2: a content type string
 *
 * Compares two content types for equality.
 *
 * Returns: %TRUE if the two strings are identical or equivalent,
 *     %FALSE otherwise.
 */
xboolean_t
g_content_type_equals (const xchar_t *type1,
                       const xchar_t *type2)
{
  xboolean_t res;

  g_return_val_if_fail (type1 != NULL, FALSE);
  g_return_val_if_fail (type2 != NULL, FALSE);

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  res = xdg_mime_mime_type_equal (type1, type2);
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  return res;
}

/**
 * g_content_type_is_a:
 * @type: a content type string
 * @supertype: a content type string
 *
 * Determines if @type is a subset of @supertype.
 *
 * Returns: %TRUE if @type is a kind of @supertype,
 *     %FALSE otherwise.
 */
xboolean_t
g_content_type_is_a (const xchar_t *type,
                     const xchar_t *supertype)
{
  xboolean_t res;

  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (supertype != NULL, FALSE);

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  res = xdg_mime_mime_type_subclass (type, supertype);
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  return res;
}

/**
 * g_content_type_is_mime_type:
 * @type: a content type string
 * @mime_type: a mime type string
 *
 * Determines if @type is a subset of @mime_type.
 * Convenience wrapper around g_content_type_is_a().
 *
 * Returns: %TRUE if @type is a kind of @mime_type,
 *     %FALSE otherwise.
 *
 * Since: 2.52
 */
xboolean_t
g_content_type_is_mime_type (const xchar_t *type,
                             const xchar_t *mime_type)
{
  return g_content_type_is_a (type, mime_type);
}

/**
 * g_content_type_is_unknown:
 * @type: a content type string
 *
 * Checks if the content type is the generic "unknown" type.
 * On UNIX this is the "application/octet-stream" mimetype,
 * while on win32 it is "*" and on OSX it is a dynamic type
 * or octet-stream.
 *
 * Returns: %TRUE if the type is the unknown type.
 */
xboolean_t
g_content_type_is_unknown (const xchar_t *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  return strcmp (XDG_MIME_TYPE_UNKNOWN, type) == 0;
}


typedef enum {
  MIME_TAXTYPE_OTHER,
  MIME_TAXTYPE_COMMENT
} MimeTagType;

typedef struct {
  int current_type;
  int current_lang_level;
  int comment_lang_level;
  char *comment;
} MimeParser;


static int
language_level (const char *lang)
{
  const char * const *lang_list;
  int i;

  /* The returned list is sorted from most desirable to least
     desirable and always contains the default locale "C". */
  lang_list = g_get_language_names ();

  for (i = 0; lang_list[i]; i++)
    if (strcmp (lang_list[i], lang) == 0)
      return 1000-i;

  return 0;
}

static void
mime_info_start_element (xmarkup_parse_context_t  *context,
                         const xchar_t          *element_name,
                         const xchar_t         **attribute_names,
                         const xchar_t         **attribute_values,
                         xpointer_t              user_data,
                         xerror_t              **error)
{
  int i;
  const char *lang;
  MimeParser *parser = user_data;

  if (strcmp (element_name, "comment") == 0)
    {
      lang = "C";
      for (i = 0; attribute_names[i]; i++)
        if (strcmp (attribute_names[i], "xml:lang") == 0)
          {
            lang = attribute_values[i];
            break;
          }

      parser->current_lang_level = language_level (lang);
      parser->current_type = MIME_TAXTYPE_COMMENT;
    }
  else
    parser->current_type = MIME_TAXTYPE_OTHER;
}

static void
mime_info_end_element (xmarkup_parse_context_t  *context,
                       const xchar_t          *element_name,
                       xpointer_t              user_data,
                       xerror_t              **error)
{
  MimeParser *parser = user_data;

  parser->current_type = MIME_TAXTYPE_OTHER;
}

static void
mime_info_text (xmarkup_parse_context_t  *context,
                const xchar_t          *text,
                xsize_t                 text_len,
                xpointer_t              user_data,
                xerror_t              **error)
{
  MimeParser *parser = user_data;

  if (parser->current_type == MIME_TAXTYPE_COMMENT &&
      parser->current_lang_level > parser->comment_lang_level)
    {
      g_free (parser->comment);
      parser->comment = xstrndup (text, text_len);
      parser->comment_lang_level = parser->current_lang_level;
    }
}

static char *
load_comment_for_mime_helper (const char *dir,
                              const char *basename)
{
  xmarkup_parse_context_t *context;
  char *filename, *data;
  xsize_t len;
  xboolean_t res;
  MimeParser parse_data = {0};
  GMarkupParser parser = {
    mime_info_start_element,
    mime_info_end_element,
    mime_info_text,
    NULL,
    NULL
  };

  filename = g_build_filename (dir, basename, NULL);

  res = xfile_get_contents (filename,  &data,  &len,  NULL);
  g_free (filename);
  if (!res)
    return NULL;

  context = xmarkup_parse_context_new   (&parser, 0, &parse_data, NULL);
  res = xmarkup_parse_context_parse (context, data, len, NULL);
  g_free (data);
  xmarkup_parse_context_free (context);

  if (!res)
    return NULL;

  return parse_data.comment;
}


static char *
load_comment_for_mime (const char *mimetype)
{
  const char * const *dirs;
  char *basename;
  char *comment;
  xsize_t i;

  basename = xstrdup_printf ("%s.xml", mimetype);

  dirs = g_content_type_get_mime_dirs ();
  for (i = 0; dirs[i] != NULL; i++)
    {
      comment = load_comment_for_mime_helper (dirs[i], basename);
      if (comment)
        {
          g_free (basename);
          return comment;
        }
    }
  g_free (basename);

  return xstrdup_printf (_("%s type"), mimetype);
}

/**
 * g_content_type_get_description:
 * @type: a content type string
 *
 * Gets the human readable description of the content type.
 *
 * Returns: a short description of the content type @type. Free the
 *     returned string with g_free()
 */
xchar_t *
g_content_type_get_description (const xchar_t *type)
{
  static xhashtable_t *type_comment_cache = NULL;
  xchar_t *comment;

  g_return_val_if_fail (type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  type = xdg_mime_unalias_mime_type (type);
  g_end_ignore_leaks ();

  if (type_comment_cache == NULL)
    type_comment_cache = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);

  comment = xhash_table_lookup (type_comment_cache, type);
  comment = xstrdup (comment);
  G_UNLOCK (gio_xdgmime);

  if (comment != NULL)
    return comment;

  comment = load_comment_for_mime (type);

  G_LOCK (gio_xdgmime);
  xhash_table_insert (type_comment_cache,
                       xstrdup (type),
                       xstrdup (comment));
  G_UNLOCK (gio_xdgmime);

  return comment;
}

/**
 * g_content_type_get_mime_type:
 * @type: a content type string
 *
 * Gets the mime type for the content type, if one is registered.
 *
 * Returns: (nullable) (transfer full): the registered mime type for the
 *     given @type, or %NULL if unknown; free with g_free().
 */
char *
g_content_type_get_mime_type (const char *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  return xstrdup (type);
}

static xicon_t *
g_content_type_get_icon_internal (const xchar_t *type,
                                  xboolean_t     symbolic)
{
  char *mimetype_icon;
  char *generic_mimetype_icon = NULL;
  char *q;
  char *icon_names[6];
  int n = 0;
  xicon_t *themed_icon;
  const char  *xdg_icon;
  int i;

  g_return_val_if_fail (type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  xdg_icon = xdg_mime_get_icon (type);
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  if (xdg_icon)
    icon_names[n++] = xstrdup (xdg_icon);

  mimetype_icon = xstrdup (type);
  while ((q = strchr (mimetype_icon, '/')) != NULL)
    *q = '-';

  icon_names[n++] = mimetype_icon;

  generic_mimetype_icon = g_content_type_get_generic_icon_name (type);
  if (generic_mimetype_icon)
    icon_names[n++] = generic_mimetype_icon;

  if (symbolic)
    {
      for (i = 0; i < n; i++)
        {
          icon_names[n + i] = icon_names[i];
          icon_names[i] = xstrconcat (icon_names[i], "-symbolic", NULL);
        }

      n += n;
    }

  themed_icon = g_themed_icon_new_from_names (icon_names, n);

  for (i = 0; i < n; i++)
    g_free (icon_names[i]);

  return themed_icon;
}

/**
 * g_content_type_get_icon:
 * @type: a content type string
 *
 * Gets the icon for a content type.
 *
 * Returns: (transfer full): #xicon_t corresponding to the content type. Free the returned
 *     object with xobject_unref()
 */
xicon_t *
g_content_type_get_icon (const xchar_t *type)
{
  return g_content_type_get_icon_internal (type, FALSE);
}

/**
 * g_content_type_get_symbolic_icon:
 * @type: a content type string
 *
 * Gets the symbolic icon for a content type.
 *
 * Returns: (transfer full): symbolic #xicon_t corresponding to the content type.
 *     Free the returned object with xobject_unref()
 *
 * Since: 2.34
 */
xicon_t *
g_content_type_get_symbolic_icon (const xchar_t *type)
{
  return g_content_type_get_icon_internal (type, TRUE);
}

/**
 * g_content_type_get_generic_icon_name:
 * @type: a content type string
 *
 * Gets the generic icon name for a content type.
 *
 * See the
 * [shared-mime-info](http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec)
 * specification for more on the generic icon name.
 *
 * Returns: (nullable): the registered generic icon name for the given @type,
 *     or %NULL if unknown. Free with g_free()
 *
 * Since: 2.34
 */
xchar_t *
g_content_type_get_generic_icon_name (const xchar_t *type)
{
  const xchar_t *xdxicon_name;
  xchar_t *icon_name;

  g_return_val_if_fail (type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  xdxicon_name = xdg_mime_get_generic_icon (type);
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  if (!xdxicon_name)
    {
      const char *p;
      const char *suffix = "-x-generic";

      p = strchr (type, '/');
      if (p == NULL)
        p = type + strlen (type);

      icon_name = g_malloc (p - type + strlen (suffix) + 1);
      memcpy (icon_name, type, p - type);
      memcpy (icon_name + (p - type), suffix, strlen (suffix));
      icon_name[(p - type) + strlen (suffix)] = 0;
    }
  else
    {
      icon_name = xstrdup (xdxicon_name);
    }

  return icon_name;
}

/**
 * g_content_type_can_be_executable:
 * @type: a content type string
 *
 * Checks if a content type can be executable. Note that for instance
 * things like text files can be executables (i.e. scripts and batch files).
 *
 * Returns: %TRUE if the file type corresponds to a type that
 *     can be executable, %FALSE otherwise.
 */
xboolean_t
g_content_type_can_be_executable (const xchar_t *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  if (g_content_type_is_a (type, "application/x-executable")  ||
      g_content_type_is_a (type, "text/plain"))
    return TRUE;

  return FALSE;
}

static xboolean_t
looks_like_text (const xuchar_t *data, xsize_t data_size)
{
  xsize_t i;
  char c;

  for (i = 0; i < data_size; i++)
    {
      c = data[i];

      if (g_ascii_iscntrl (c) &&
          !g_ascii_isspace (c) &&
          c != '\b')
        return FALSE;
    }
  return TRUE;
}

/**
 * g_content_type_from_mime_type:
 * @mime_type: a mime type string
 *
 * Tries to find a content type based on the mime type name.
 *
 * Returns: (nullable): Newly allocated string with content type or
 *     %NULL. Free with g_free()
 *
 * Since: 2.18
 **/
xchar_t *
g_content_type_from_mime_type (const xchar_t *mime_type)
{
  char *umime;

  g_return_val_if_fail (mime_type != NULL, NULL);

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();
  /* mime type and content type are same on unixes */
  umime = xstrdup (xdg_mime_unalias_mime_type (mime_type));
  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  return umime;
}

/**
 * g_content_type_guess:
 * @filename: (nullable) (type filename): a path, or %NULL
 * @data: (nullable) (array length=data_size): a stream of data, or %NULL
 * @data_size: the size of @data
 * @result_uncertain: (out) (optional): return location for the certainty
 *     of the result, or %NULL
 *
 * Guesses the content type based on example data. If the function is
 * uncertain, @result_uncertain will be set to %TRUE. Either @filename
 * or @data may be %NULL, in which case the guess will be based solely
 * on the other argument.
 *
 * Returns: a string indicating a guessed content type for the
 *     given data. Free with g_free()
 */
xchar_t *
g_content_type_guess (const xchar_t  *filename,
                      const xuchar_t *data,
                      xsize_t         data_size,
                      xboolean_t     *result_uncertain)
{
  char *basename;
  const char *name_mimetypes[10], *sniffed_mimetype;
  char *mimetype;
  int i;
  int n_name_mimetypes;
  int sniffed_prio;

  sniffed_prio = 0;
  n_name_mimetypes = 0;
  sniffed_mimetype = XDG_MIME_TYPE_UNKNOWN;

  if (result_uncertain)
    *result_uncertain = FALSE;

  /* our test suite and potentially other code used -1 in the past, which is
   * not documented and not allowed; guard against that */
  g_return_val_if_fail (data_size != (xsize_t) -1, xstrdup (XDG_MIME_TYPE_UNKNOWN));

  G_LOCK (gio_xdgmime);
  g_begin_ignore_leaks ();

  if (filename)
    {
      i = strlen (filename);
      if (i > 0 && filename[i - 1] == '/')
        {
          name_mimetypes[0] = "inode/directory";
          name_mimetypes[1] = NULL;
          n_name_mimetypes = 1;
          if (result_uncertain)
            *result_uncertain = TRUE;
        }
      else
        {
          basename = g_path_get_basename (filename);
          n_name_mimetypes = xdg_mime_get_mime_types_from_file_name (basename, name_mimetypes, 10);
          g_free (basename);
        }
    }

  /* Got an extension match, and no conflicts. This is it. */
  if (n_name_mimetypes == 1)
    {
      xchar_t *s = xstrdup (name_mimetypes[0]);
      g_end_ignore_leaks ();
      G_UNLOCK (gio_xdgmime);
      return s;
    }

  if (data)
    {
      sniffed_mimetype = xdg_mime_get_mime_type_for_data (data, data_size, &sniffed_prio);
      if (sniffed_mimetype == XDG_MIME_TYPE_UNKNOWN &&
          data &&
          looks_like_text (data, data_size))
        sniffed_mimetype = "text/plain";

      /* For security reasons we don't ever want to sniff desktop files
       * where we know the filename and it doesn't have a .desktop extension.
       * This is because desktop files allow executing any application and
       * we don't want to make it possible to hide them looking like something
       * else.
       */
      if (filename != NULL &&
          strcmp (sniffed_mimetype, "application/x-desktop") == 0)
        sniffed_mimetype = "text/plain";
    }

  if (n_name_mimetypes == 0)
    {
      if (sniffed_mimetype == XDG_MIME_TYPE_UNKNOWN &&
          result_uncertain)
        *result_uncertain = TRUE;

      mimetype = xstrdup (sniffed_mimetype);
    }
  else
    {
      mimetype = NULL;
      if (sniffed_mimetype != XDG_MIME_TYPE_UNKNOWN)
        {
          if (sniffed_prio >= 80) /* High priority sniffing match, use that */
            mimetype = xstrdup (sniffed_mimetype);
          else
            {
              /* There are conflicts between the name matches and we
               * have a sniffed type, use that as a tie breaker.
               */
              for (i = 0; i < n_name_mimetypes; i++)
                {
                  if ( xdg_mime_mime_type_subclass (name_mimetypes[i], sniffed_mimetype))
                    {
                      /* This nametype match is derived from (or the same as)
                       * the sniffed type). This is probably it.
                       */
                      mimetype = xstrdup (name_mimetypes[i]);
                      break;
                    }
                }
            }
        }

      if (mimetype == NULL)
        {
          /* Conflicts, and sniffed type was no help or not there.
           * Guess on the first one
           */
          mimetype = xstrdup (name_mimetypes[0]);
          if (result_uncertain)
            *result_uncertain = TRUE;
        }
    }

  g_end_ignore_leaks ();
  G_UNLOCK (gio_xdgmime);

  return mimetype;
}

static void
enumerate_mimetypes_subdir (const char *dir,
                            const char *prefix,
                            xhashtable_t *mimetypes)
{
  DIR *d;
  struct dirent *ent;
  char *mimetype;

  d = opendir (dir);
  if (d)
    {
      while ((ent = readdir (d)) != NULL)
        {
          if (xstr_has_suffix (ent->d_name, ".xml"))
            {
              mimetype = xstrdup_printf ("%s/%.*s", prefix, (int) strlen (ent->d_name) - 4, ent->d_name);
              xhash_table_replace (mimetypes, mimetype, NULL);
            }
        }
      closedir (d);
    }
}

static void
enumerate_mimetypes_dir (const char *dir,
                         xhashtable_t *mimetypes)
{
  DIR *d;
  struct dirent *ent;
  const char *mimedir;
  char *name;

  mimedir = dir;

  d = opendir (mimedir);
  if (d)
    {
      while ((ent = readdir (d)) != NULL)
        {
          if (strcmp (ent->d_name, "packages") != 0)
            {
              name = g_build_filename (mimedir, ent->d_name, NULL);
              if (xfile_test (name, XFILE_TEST_IS_DIR))
                enumerate_mimetypes_subdir (name, ent->d_name, mimetypes);
              g_free (name);
            }
        }
      closedir (d);
    }
}

/**
 * g_content_types_get_registered:
 *
 * Gets a list of strings containing all the registered content types
 * known to the system. The list and its data should be freed using
 * `xlist_free_full (list, g_free)`.
 *
 * Returns: (element-type utf8) (transfer full): list of the registered
 *     content types
 */
xlist_t *
g_content_types_get_registered (void)
{
  const char * const *dirs;
  xhashtable_t *mimetypes;
  xhash_table_iter_t iter;
  xpointer_t key;
  xsize_t i;
  xlist_t *l;

  mimetypes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);

  dirs = g_content_type_get_mime_dirs ();
  for (i = 0; dirs[i] != NULL; i++)
    enumerate_mimetypes_dir (dirs[i], mimetypes);

  l = NULL;
  xhash_table_iter_init (&iter, mimetypes);
  while (xhash_table_iter_next (&iter, &key, NULL))
    {
      l = xlist_prepend (l, key);
      xhash_table_iter_steal (&iter);
    }

  xhash_table_destroy (mimetypes);

  return l;
}


/* tree magic data */
static xlist_t *tree_matches = NULL;
static xboolean_t need_reload = FALSE;

G_LOCK_DEFINE_STATIC (gio_treemagic);

typedef struct
{
  xchar_t *path;
  xfile_type_t type;
  xuint_t match_case : 1;
  xuint_t executable : 1;
  xuint_t non_empty  : 1;
  xuint_t on_disc    : 1;
  xchar_t *mimetype;
  xlist_t *matches;
} TreeMatchlet;

typedef struct
{
  xchar_t *contenttype;
  xint_t priority;
  xlist_t *matches;
} TreeMatch;


static void
tree_matchlet_free (TreeMatchlet *matchlet)
{
  xlist_free_full (matchlet->matches, (xdestroy_notify_t) tree_matchlet_free);
  g_free (matchlet->path);
  g_free (matchlet->mimetype);
  g_slice_free (TreeMatchlet, matchlet);
}

static void
tree_match_free (TreeMatch *match)
{
  xlist_free_full (match->matches, (xdestroy_notify_t) tree_matchlet_free);
  g_free (match->contenttype);
  g_slice_free (TreeMatch, match);
}

static TreeMatch *
parse_header (xchar_t *line)
{
  xint_t len;
  xchar_t *s;
  TreeMatch *match;

  len = strlen (line);

  if (line[0] != '[' || line[len - 1] != ']')
    return NULL;

  line[len - 1] = 0;
  s = strchr (line, ':');
  if (s == NULL)
    return NULL;

  match = g_slice_new0 (TreeMatch);
  match->priority = atoi (line + 1);
  match->contenttype = xstrdup (s + 1);

  return match;
}

static TreeMatchlet *
parse_match_line (xchar_t *line,
                  xint_t  *depth)
{
  xchar_t *s, *p;
  TreeMatchlet *matchlet;
  xchar_t **parts;
  xint_t i;

  matchlet = g_slice_new0 (TreeMatchlet);

  if (line[0] == '>')
    {
      *depth = 0;
      s = line;
    }
  else
    {
      *depth = atoi (line);
      s = strchr (line, '>');
      if (s == NULL)
        goto handle_error;
    }
  s += 2;
  p = strchr (s, '"');
  if (p == NULL)
    goto handle_error;
  *p = 0;

  matchlet->path = xstrdup (s);
  s = p + 1;
  parts = xstrsplit (s, ",", 0);
  if (strcmp (parts[0], "=file") == 0)
    matchlet->type = XFILE_TYPE_REGULAR;
  else if (strcmp (parts[0], "=directory") == 0)
    matchlet->type = XFILE_TYPE_DIRECTORY;
  else if (strcmp (parts[0], "=link") == 0)
    matchlet->type = XFILE_TYPE_SYMBOLIC_LINK;
  else
    matchlet->type = XFILE_TYPE_UNKNOWN;
  for (i = 1; parts[i]; i++)
    {
      if (strcmp (parts[i], "executable") == 0)
        matchlet->executable = 1;
      else if (strcmp (parts[i], "match-case") == 0)
        matchlet->match_case = 1;
      else if (strcmp (parts[i], "non-empty") == 0)
        matchlet->non_empty = 1;
      else if (strcmp (parts[i], "on-disc") == 0)
        matchlet->on_disc = 1;
      else
        matchlet->mimetype = xstrdup (parts[i]);
    }

  xstrfreev (parts);

  return matchlet;

handle_error:
  g_slice_free (TreeMatchlet, matchlet);
  return NULL;
}

static xint_t
cmp_match (xconstpointer a, xconstpointer b)
{
  const TreeMatch *aa = (const TreeMatch *)a;
  const TreeMatch *bb = (const TreeMatch *)b;

  return bb->priority - aa->priority;
}

static void
insert_match (TreeMatch *match)
{
  tree_matches = xlist_insert_sorted (tree_matches, match, cmp_match);
}

static void
insert_matchlet (TreeMatch    *match,
                 TreeMatchlet *matchlet,
                 xint_t          depth)
{
  if (depth == 0)
    match->matches = xlist_append (match->matches, matchlet);
  else
    {
      xlist_t *last;
      TreeMatchlet *m;

      last = xlist_last (match->matches);
      if (!last)
        {
          tree_matchlet_free (matchlet);
          g_warning ("can't insert tree matchlet at depth %d", depth);
          return;
        }

      m = (TreeMatchlet *) last->data;
      while (--depth > 0)
        {
          last = xlist_last (m->matches);
          if (!last)
            {
              tree_matchlet_free (matchlet);
              g_warning ("can't insert tree matchlet at depth %d", depth);
              return;
            }

          m = (TreeMatchlet *) last->data;
        }
      m->matches = xlist_append (m->matches, matchlet);
    }
}

static void
read_tree_magic_from_directory (const xchar_t *prefix)
{
  xchar_t *filename;
  xchar_t *text;
  xsize_t len;
  xchar_t **lines;
  xsize_t i;
  TreeMatch *match;
  TreeMatchlet *matchlet;
  xint_t depth;

  filename = g_build_filename (prefix, "treemagic", NULL);

  if (xfile_get_contents (filename, &text, &len, NULL))
    {
      if (strcmp (text, "MIME-TreeMagic") == 0)
        {
          lines = xstrsplit (text + strlen ("MIME-TreeMagic") + 2, "\n", 0);
          match = NULL;
          for (i = 0; lines[i] && lines[i][0]; i++)
            {
              if (lines[i][0] == '[' && (match = parse_header (lines[i])) != NULL)
                {
                  insert_match (match);
                }
              else if (match != NULL)
                {
                  matchlet = parse_match_line (lines[i], &depth);
                  if (matchlet == NULL)
                    {
                      g_warning ("%s: body corrupt; skipping", filename);
                      break;
                    }
                  insert_matchlet (match, matchlet, depth);
                }
              else
                {
                  g_warning ("%s: header corrupt; skipping", filename);
                  break;
                }
            }

          xstrfreev (lines);
        }
      else
        g_warning ("%s: header not found, skipping", filename);

      g_free (text);
    }

  g_free (filename);
}

static void
tree_magic_schedule_reload (void)
{
  need_reload = TRUE;
}

static void
xdg_mime_reload (void *user_data)
{
  tree_magic_schedule_reload ();
}

static void
tree_magic_shutdown (void)
{
  xlist_free_full (tree_matches, (xdestroy_notify_t) tree_match_free);
  tree_matches = NULL;
}

static void
tree_magic_init (void)
{
  static xboolean_t initialized = FALSE;
  xsize_t i;

  if (!initialized)
    {
      initialized = TRUE;

      xdg_mime_register_reload_callback (xdg_mime_reload, NULL, NULL);
      need_reload = TRUE;
    }

  if (need_reload)
    {
      const char * const *dirs;

      need_reload = FALSE;

      tree_magic_shutdown ();

      dirs = g_content_type_get_mime_dirs ();
      for (i = 0; dirs[i] != NULL; i++)
        read_tree_magic_from_directory (dirs[i]);
    }
}

/* a filtering enumerator */

typedef struct
{
  xchar_t *path;
  xint_t depth;
  xboolean_t ignore_case;
  xchar_t **components;
  xchar_t **case_components;
  xfile_enumerator_t **enumerators;
  xfile_t **children;
} Enumerator;

static xboolean_t
component_match (Enumerator  *e,
                 xint_t         depth,
                 const xchar_t *name)
{
  xchar_t *case_folded, *key;
  xboolean_t found;

  if (strcmp (name, e->components[depth]) == 0)
    return TRUE;

  if (!e->ignore_case)
    return FALSE;

  case_folded = xutf8_casefold (name, -1);
  key = xutf8_collate_key (case_folded, -1);

  found = strcmp (key, e->case_components[depth]) == 0;

  g_free (case_folded);
  g_free (key);

  return found;
}

static xfile_t *
next_match_recurse (Enumerator *e,
                    xint_t        depth)
{
  xfile_t *file;
  xfile_info_t *info;
  const xchar_t *name;

  while (TRUE)
    {
      if (e->enumerators[depth] == NULL)
        {
          if (depth > 0)
            {
              file = next_match_recurse (e, depth - 1);
              if (file)
                {
                  e->children[depth] = file;
                  e->enumerators[depth] = xfile_enumerate_children (file,
                                                                     XFILE_ATTRIBUTE_STANDARD_NAME,
                                                                     XFILE_QUERY_INFO_NONE,
                                                                     NULL,
                                                                     NULL);
                }
            }
          if (e->enumerators[depth] == NULL)
            return NULL;
        }

      while ((info = xfile_enumerator_next_file (e->enumerators[depth], NULL, NULL)))
        {
          name = xfile_info_get_name (info);
          if (component_match (e, depth, name))
            {
              file = xfile_get_child (e->children[depth], name);
              xobject_unref (info);
              return file;
            }
          xobject_unref (info);
        }

      xobject_unref (e->enumerators[depth]);
      e->enumerators[depth] = NULL;
      xobject_unref (e->children[depth]);
      e->children[depth] = NULL;
    }
}

static xfile_t *
enumerator_next (Enumerator *e)
{
  return next_match_recurse (e, e->depth - 1);
}

static Enumerator *
enumerator_new (xfile_t      *root,
                const char *path,
                xboolean_t    ignore_case)
{
  Enumerator *e;
  xint_t i;
  xchar_t *case_folded;

  e = g_new0 (Enumerator, 1);
  e->path = xstrdup (path);
  e->ignore_case = ignore_case;

  e->components = xstrsplit (e->path, G_DIR_SEPARATOR_S, -1);
  e->depth = xstrv_length (e->components);
  if (e->ignore_case)
    {
      e->case_components = g_new0 (char *, e->depth + 1);
      for (i = 0; e->components[i]; i++)
        {
          case_folded = xutf8_casefold (e->components[i], -1);
          e->case_components[i] = xutf8_collate_key (case_folded, -1);
          g_free (case_folded);
        }
    }

  e->children = g_new0 (xfile_t *, e->depth);
  e->children[0] = xobject_ref (root);
  e->enumerators = g_new0 (xfile_enumerator_t *, e->depth);
  e->enumerators[0] = xfile_enumerate_children (root,
                                                 XFILE_ATTRIBUTE_STANDARD_NAME,
                                                 XFILE_QUERY_INFO_NONE,
                                                 NULL,
                                                 NULL);

  return e;
}

static void
enumerator_free (Enumerator *e)
{
  xint_t i;

  for (i = 0; i < e->depth; i++)
    {
      if (e->enumerators[i])
        xobject_unref (e->enumerators[i]);
      if (e->children[i])
        xobject_unref (e->children[i]);
    }

  g_free (e->enumerators);
  g_free (e->children);
  xstrfreev (e->components);
  if (e->case_components)
    xstrfreev (e->case_components);
  g_free (e->path);
  g_free (e);
}

static xboolean_t
matchlet_match (TreeMatchlet *matchlet,
                xfile_t        *root)
{
  xfile_t *file;
  xfile_info_t *info;
  xboolean_t result;
  const xchar_t *attrs;
  Enumerator *e;
  xlist_t *l;

  e = enumerator_new (root, matchlet->path, !matchlet->match_case);

  do
    {
      file = enumerator_next (e);
      if (!file)
        {
          enumerator_free (e);
          return FALSE;
        }

      if (matchlet->mimetype)
        attrs = XFILE_ATTRIBUTE_STANDARD_TYPE ","
                XFILE_ATTRIBUTE_ACCESS_CAN_EXECUTE ","
                XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE;
      else
        attrs = XFILE_ATTRIBUTE_STANDARD_TYPE ","
                XFILE_ATTRIBUTE_ACCESS_CAN_EXECUTE;
      info = xfile_query_info (file,
                                attrs,
                                XFILE_QUERY_INFO_NONE,
                                NULL,
                                NULL);
      if (info)
        {
          result = TRUE;

          if (matchlet->type != XFILE_TYPE_UNKNOWN &&
              xfile_info_get_file_type (info) != matchlet->type)
            result = FALSE;

          if (matchlet->executable &&
              !xfile_info_get_attribute_boolean (info, XFILE_ATTRIBUTE_ACCESS_CAN_EXECUTE))
            result = FALSE;
        }
      else
        result = FALSE;

      if (result && matchlet->non_empty)
        {
          xfile_enumerator_t *child_enum;
          xfile_info_t *child_info;

          child_enum = xfile_enumerate_children (file,
                                                  XFILE_ATTRIBUTE_STANDARD_NAME,
                                                  XFILE_QUERY_INFO_NONE,
                                                  NULL,
                                                  NULL);

          if (child_enum)
            {
              child_info = xfile_enumerator_next_file (child_enum, NULL, NULL);
              if (child_info)
                xobject_unref (child_info);
              else
                result = FALSE;
              xobject_unref (child_enum);
            }
          else
            result = FALSE;
        }

      if (result && matchlet->mimetype)
        {
          if (strcmp (matchlet->mimetype, xfile_info_get_content_type (info)) != 0)
            result = FALSE;
        }

      if (info)
        xobject_unref (info);
      xobject_unref (file);
    }
  while (!result);

  enumerator_free (e);

  if (!matchlet->matches)
    return TRUE;

  for (l = matchlet->matches; l; l = l->next)
    {
      TreeMatchlet *submatchlet;

      submatchlet = l->data;
      if (matchlet_match (submatchlet, root))
        return TRUE;
    }

  return FALSE;
}

static void
match_match (TreeMatch    *match,
             xfile_t        *root,
             xptr_array_t    *types)
{
  xlist_t *l;

  for (l = match->matches; l; l = l->next)
    {
      TreeMatchlet *matchlet = l->data;
      if (matchlet_match (matchlet, root))
        {
          xptr_array_add (types, xstrdup (match->contenttype));
          break;
        }
    }
}

/**
 * g_content_type_guess_for_tree:
 * @root: the root of the tree to guess a type for
 *
 * Tries to guess the type of the tree with root @root, by
 * looking at the files it contains. The result is an array
 * of content types, with the best guess coming first.
 *
 * The types returned all have the form x-content/foo, e.g.
 * x-content/audio-cdda (for audio CDs) or x-content/image-dcf
 * (for a camera memory card). See the
 * [shared-mime-info](http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec)
 * specification for more on x-content types.
 *
 * This function is useful in the implementation of
 * g_mount_guess_content_type().
 *
 * Returns: (transfer full) (array zero-terminated=1): an %NULL-terminated
 *     array of zero or more content types. Free with xstrfreev()
 *
 * Since: 2.18
 */
xchar_t **
g_content_type_guess_for_tree (xfile_t *root)
{
  xptr_array_t *types;
  xlist_t *l;

  types = xptr_array_new ();

  G_LOCK (gio_treemagic);

  tree_magic_init ();
  for (l = tree_matches; l; l = l->next)
    {
      TreeMatch *match = l->data;
      match_match (match, root, types);
    }

  G_UNLOCK (gio_treemagic);

  xptr_array_add (types, NULL);

  return (xchar_t **)xptr_array_free (types, FALSE);
}
