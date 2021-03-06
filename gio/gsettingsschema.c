/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2011 Canonical Limited
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "glib-private.h"
#include "gsettingsschema-internal.h"
#include "gsettings.h"

#include "gvdb/gvdb-reader.h"
#include "strinfo.c"

#include <glibintl.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

/**
 * SECTION:gsettingsschema
 * @short_description: Introspecting and controlling the loading
 *     of xsettings_t schemas
 * @include: gio/gio.h
 *
 * The #xsettings_schema_source_t and #xsettings_schema_t APIs provide a
 * mechanism for advanced control over the loading of schemas and a
 * mechanism for introspecting their content.
 *
 * Plugin loading systems that wish to provide plugins a way to access
 * settings face the problem of how to make the schemas for these
 * settings visible to xsettings_t.  Typically, a plugin will want to ship
 * the schema along with itself and it won't be installed into the
 * standard system directories for schemas.
 *
 * #xsettings_schema_source_t provides a mechanism for dealing with this by
 * allowing the creation of a new 'schema source' from which schemas can
 * be acquired.  This schema source can then become part of the metadata
 * associated with the plugin and queried whenever the plugin requires
 * access to some settings.
 *
 * Consider the following example:
 *
 * |[<!-- language="C" -->
 * typedef struct
 * {
 *    ...
 *    xsettings_schema_source_t *schema_source;
 *    ...
 * } Plugin;
 *
 * Plugin *
 * initialise_plugin (const xchar_t *dir)
 * {
 *   Plugin *plugin;
 *
 *   ...
 *
 *   plugin->schema_source =
 *     g_settings_schema_source_new_from_directory (dir,
 *       g_settings_schema_source_get_default (), FALSE, NULL);
 *
 *   ...
 *
 *   return plugin;
 * }
 *
 * ...
 *
 * xsettings_t *
 * plugin_get_settings (Plugin      *plugin,
 *                      const xchar_t *schema_id)
 * {
 *   xsettings_schema_t *schema;
 *
 *   if (schema_id == NULL)
 *     schema_id = plugin->identifier;
 *
 *   schema = g_settings_schema_source_lookup (plugin->schema_source,
 *                                             schema_id, FALSE);
 *
 *   if (schema == NULL)
 *     {
 *       ... disable the plugin or abort, etc ...
 *     }
 *
 *   return g_settings_new_full (schema, NULL, NULL);
 * }
 * ]|
 *
 * The code above shows how hooks should be added to the code that
 * initialises (or enables) the plugin to create the schema source and
 * how an API can be added to the plugin system to provide a convenient
 * way for the plugin to access its settings, using the schemas that it
 * ships.
 *
 * From the standpoint of the plugin, it would need to ensure that it
 * ships a gschemas.compiled file as part of itself, and then simply do
 * the following:
 *
 * |[<!-- language="C" -->
 * {
 *   xsettings_t *settings;
 *   xint_t some_value;
 *
 *   settings = plugin_get_settings (self, NULL);
 *   some_value = g_settings_get_int (settings, "some-value");
 *   ...
 * }
 * ]|
 *
 * It's also possible that the plugin system expects the schema source
 * files (ie: .gschema.xml files) instead of a gschemas.compiled file.
 * In that case, the plugin loading system must compile the schemas for
 * itself before attempting to create the settings source.
 *
 * Since: 2.32
 **/

/**
 * xsettings_schema_key_t:
 *
 * #xsettings_schema_key_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

/**
 * xsettings_schema_t:
 *
 * This is an opaque structure type.  You may not access it directly.
 *
 * Since: 2.32
 **/
struct _GSettingsSchema
{
  xsettings_schema_source_t *source;
  const xchar_t *gettext_domain;
  const xchar_t *path;
  xquark *items;
  xint_t n_items;
  GvdbTable *table;
  xchar_t *id;

  xsettings_schema_t *extends;

  xint_t ref_count;
};

/**
 * XTYPE_SETTINGS_SCHEMA_SOURCE:
 *
 * A boxed #xtype_t corresponding to #xsettings_schema_source_t.
 *
 * Since: 2.32
 **/
G_DEFINE_BOXED_TYPE (xsettings_schema_source_t, g_settings_schema_source, g_settings_schema_source_ref, g_settings_schema_source_unref)

/**
 * XTYPE_SETTINGS_SCHEMA:
 *
 * A boxed #xtype_t corresponding to #xsettings_schema_t.
 *
 * Since: 2.32
 **/
G_DEFINE_BOXED_TYPE (xsettings_schema_t, g_settings_schema, g_settings_schema_ref, g_settings_schema_unref)

/**
 * xsettings_schema_source_t:
 *
 * This is an opaque structure type.  You may not access it directly.
 *
 * Since: 2.32
 **/
struct _GSettingsSchemaSource
{
  xsettings_schema_source_t *parent;
  xchar_t *directory;
  GvdbTable *table;
  xhashtable_t **text_tables;

  xint_t ref_count;
};

static xsettings_schema_source_t *schema_sources;

/**
 * g_settings_schema_source_ref:
 * @source: a #xsettings_schema_source_t
 *
 * Increase the reference count of @source, returning a new reference.
 *
 * Returns: (transfer full) (not nullable): a new reference to @source
 *
 * Since: 2.32
 **/
xsettings_schema_source_t *
g_settings_schema_source_ref (xsettings_schema_source_t *source)
{
  g_atomic_int_inc (&source->ref_count);

  return source;
}

/**
 * g_settings_schema_source_unref:
 * @source: a #xsettings_schema_source_t
 *
 * Decrease the reference count of @source, possibly freeing it.
 *
 * Since: 2.32
 **/
void
g_settings_schema_source_unref (xsettings_schema_source_t *source)
{
  if (g_atomic_int_dec_and_test (&source->ref_count))
    {
      if (source == schema_sources)
        xerror ("g_settings_schema_source_unref() called too many times on the default schema source");

      if (source->parent)
        g_settings_schema_source_unref (source->parent);
      gvdb_table_free (source->table);
      g_free (source->directory);

      if (source->text_tables)
        {
          xhash_table_unref (source->text_tables[0]);
          xhash_table_unref (source->text_tables[1]);
          g_free (source->text_tables);
        }

      g_slice_free (xsettings_schema_source_t, source);
    }
}

/**
 * g_settings_schema_source_new_from_directory:
 * @directory: (type filename): the filename of a directory
 * @parent: (nullable): a #xsettings_schema_source_t, or %NULL
 * @trusted: %TRUE, if the directory is trusted
 * @error: a pointer to a #xerror_t pointer set to %NULL, or %NULL
 *
 * Attempts to create a new schema source corresponding to the contents
 * of the given directory.
 *
 * This function is not required for normal uses of #xsettings_t but it
 * may be useful to authors of plugin management systems.
 *
 * The directory should contain a file called `gschemas.compiled` as
 * produced by the [glib-compile-schemas][glib-compile-schemas] tool.
 *
 * If @trusted is %TRUE then `gschemas.compiled` is trusted not to be
 * corrupted. This assumption has a performance advantage, but can result
 * in crashes or inconsistent behaviour in the case of a corrupted file.
 * Generally, you should set @trusted to %TRUE for files installed by the
 * system and to %FALSE for files in the home directory.
 *
 * In either case, an empty file or some types of corruption in the file will
 * result in %XFILE_ERROR_INVAL being returned.
 *
 * If @parent is non-%NULL then there are two effects.
 *
 * First, if g_settings_schema_source_lookup() is called with the
 * @recursive flag set to %TRUE and the schema can not be found in the
 * source, the lookup will recurse to the parent.
 *
 * Second, any references to other schemas specified within this
 * source (ie: `child` or `extends`) references may be resolved
 * from the @parent.
 *
 * For this second reason, except in very unusual situations, the
 * @parent should probably be given as the default schema source, as
 * returned by g_settings_schema_source_get_default().
 *
 * Since: 2.32
 **/
xsettings_schema_source_t *
g_settings_schema_source_new_from_directory (const xchar_t            *directory,
                                             xsettings_schema_source_t  *parent,
                                             xboolean_t                trusted,
                                             xerror_t                **error)
{
  xsettings_schema_source_t *source;
  GvdbTable *table;
  xchar_t *filename;

  filename = g_build_filename (directory, "gschemas.compiled", NULL);
  table = gvdb_table_new (filename, trusted, error);
  g_free (filename);

  if (table == NULL)
    return NULL;

  source = g_slice_new (xsettings_schema_source_t);
  source->directory = xstrdup (directory);
  source->parent = parent ? g_settings_schema_source_ref (parent) : NULL;
  source->text_tables = NULL;
  source->table = table;
  source->ref_count = 1;

  return source;
}

static void
try_prepend_dir (const xchar_t *directory)
{
  xsettings_schema_source_t *source;

  source = g_settings_schema_source_new_from_directory (directory, schema_sources, TRUE, NULL);

  /* If we successfully created it then prepend it to the global list */
  if (source != NULL)
    schema_sources = source;
}

static void
try_prepend_data_dir (const xchar_t *directory)
{
  xchar_t *dirname = g_build_filename (directory, "glib-2.0", "schemas", NULL);
  try_prepend_dir (dirname);
  g_free (dirname);
}

static void
initialise_schema_sources (void)
{
  static xsize_t initialised;

  /* need a separate variable because 'schema_sources' may legitimately
   * be null if we have zero valid schema sources
   */
  if G_UNLIKELY (g_once_init_enter (&initialised))
    {
      xboolean_t is_setuid = XPL_PRIVATE_CALL (g_check_setuid) ();
      const xchar_t * const *dirs;
      const xchar_t *path;
      xchar_t **extra_schema_dirs;
      xint_t i;

      /* iterate in reverse: count up, then count down */
      dirs = g_get_system_data_dirs ();
      for (i = 0; dirs[i]; i++);

      while (i--)
        try_prepend_data_dir (dirs[i]);

      try_prepend_data_dir (g_get_user_data_dir ());

      /* Disallow loading extra schemas if running as setuid, as that could
       * allow reading privileged files. */
      if (!is_setuid && (path = g_getenv ("GSETTINGS_SCHEMA_DIR")) != NULL)
        {
          extra_schema_dirs = xstrsplit (path, G_SEARCHPATH_SEPARATOR_S, 0);
          for (i = 0; extra_schema_dirs[i]; i++);

          while (i--)
            try_prepend_dir (extra_schema_dirs[i]);

          xstrfreev (extra_schema_dirs);
        }

      g_once_init_leave (&initialised, TRUE);
    }
}

/**
 * g_settings_schema_source_get_default:
 *
 * Gets the default system schema source.
 *
 * This function is not required for normal uses of #xsettings_t but it
 * may be useful to authors of plugin management systems or to those who
 * want to introspect the content of schemas.
 *
 * If no schemas are installed, %NULL will be returned.
 *
 * The returned source may actually consist of multiple schema sources
 * from different directories, depending on which directories were given
 * in `XDG_DATA_DIRS` and `GSETTINGS_SCHEMA_DIR`. For this reason, all
 * lookups performed against the default source should probably be done
 * recursively.
 *
 * Returns: (transfer none) (nullable): the default schema source
 *
 * Since: 2.32
 **/
xsettings_schema_source_t *
g_settings_schema_source_get_default (void)
{
  initialise_schema_sources ();

  return schema_sources;
}

/**
 * g_settings_schema_source_lookup:
 * @source: a #xsettings_schema_source_t
 * @schema_id: a schema ID
 * @recursive: %TRUE if the lookup should be recursive
 *
 * Looks up a schema with the identifier @schema_id in @source.
 *
 * This function is not required for normal uses of #xsettings_t but it
 * may be useful to authors of plugin management systems or to those who
 * want to introspect the content of schemas.
 *
 * If the schema isn't found directly in @source and @recursive is %TRUE
 * then the parent sources will also be checked.
 *
 * If the schema isn't found, %NULL is returned.
 *
 * Returns: (nullable) (transfer full): a new #xsettings_schema_t
 *
 * Since: 2.32
 **/
xsettings_schema_t *
g_settings_schema_source_lookup (xsettings_schema_source_t *source,
                                 const xchar_t           *schema_id,
                                 xboolean_t               recursive)
{
  xsettings_schema_t *schema;
  GvdbTable *table;
  const xchar_t *extends;

  xreturn_val_if_fail (source != NULL, NULL);
  xreturn_val_if_fail (schema_id != NULL, NULL);

  table = gvdb_table_get_table (source->table, schema_id);

  if (table == NULL && recursive)
    for (source = source->parent; source; source = source->parent)
      if ((table = gvdb_table_get_table (source->table, schema_id)))
        break;

  if (table == NULL)
    return NULL;

  schema = g_slice_new0 (xsettings_schema_t);
  schema->source = g_settings_schema_source_ref (source);
  schema->ref_count = 1;
  schema->id = xstrdup (schema_id);
  schema->table = table;
  schema->path = g_settings_schema_get_string (schema, ".path");
  schema->gettext_domain = g_settings_schema_get_string (schema, ".gettext-domain");

  if (schema->gettext_domain)
    bind_textdomain_codeset (schema->gettext_domain, "UTF-8");

  extends = g_settings_schema_get_string (schema, ".extends");
  if (extends)
    {
      schema->extends = g_settings_schema_source_lookup (source, extends, TRUE);
      if (schema->extends == NULL)
        g_warning ("Schema '%s' extends schema '%s' but we could not find it", schema_id, extends);
    }

  return schema;
}

typedef struct
{
  xhashtable_t *summaries;
  xhashtable_t *descriptions;
  xslist_t     *gettext_domain;
  xslist_t     *schema_id;
  xslist_t     *key_name;
  xstring_t    *string;
} TextTableParseInfo;

static const xchar_t *
get_attribute_value (xslist_t *list)
{
  xslist_t *node;

  for (node = list; node; node = node->next)
    if (node->data)
      return node->data;

  return NULL;
}

static void
pop_attribute_value (xslist_t **list)
{
  xchar_t *top;

  top = (*list)->data;
  *list = xslist_remove (*list, top);

  g_free (top);
}

static void
push_attribute_value (xslist_t      **list,
                      const xchar_t  *value)
{
  *list = xslist_prepend (*list, xstrdup (value));
}

static void
start_element (xmarkup_parse_context_t  *context,
               const xchar_t          *element_name,
               const xchar_t         **attribute_names,
               const xchar_t         **attribute_values,
               xpointer_t              user_data,
               xerror_t              **error)
{
  TextTableParseInfo *info = user_data;
  const xchar_t *gettext_domain = NULL;
  const xchar_t *schema_id = NULL;
  const xchar_t *key_name = NULL;
  xint_t i;

  for (i = 0; attribute_names[i]; i++)
    {
      if (xstr_equal (attribute_names[i], "gettext-domain"))
        gettext_domain = attribute_values[i];
      else if (xstr_equal (attribute_names[i], "id"))
        schema_id = attribute_values[i];
      else if (xstr_equal (attribute_names[i], "name"))
        key_name = attribute_values[i];
    }

  push_attribute_value (&info->gettext_domain, gettext_domain);
  push_attribute_value (&info->schema_id, schema_id);
  push_attribute_value (&info->key_name, key_name);

  if (info->string)
    {
      xstring_free (info->string, TRUE);
      info->string = NULL;
    }

  if (xstr_equal (element_name, "summary") || xstr_equal (element_name, "description"))
    info->string = xstring_new (NULL);
}

static xchar_t *
normalise_whitespace (const xchar_t *orig)
{
  /* We normalise by the same rules as in intltool:
   *
   *   sub cleanup {
   *       s/^\s+//;
   *       s/\s+$//;
   *       s/\s+/ /g;
   *       return $_;
   *   }
   *
   *   $message = join "\n\n", map &cleanup, split/\n\s*\n+/, $message;
   *
   * Where \s is an ascii space character.
   *
   * We aim for ease of implementation over efficiency -- this code is
   * not run in normal applications.
   */
  static xregex_t *cleanup[3];
  static xregex_t *splitter;
  xchar_t **lines;
  xchar_t *result;
  xint_t i;

  if (g_once_init_enter (&splitter))
    {
      xregex_t *s;

      cleanup[0] = xregex_new ("^\\s+", 0, 0, 0);
      cleanup[1] = xregex_new ("\\s+$", 0, 0, 0);
      cleanup[2] = xregex_new ("\\s+", 0, 0, 0);
      s = xregex_new ("\\n\\s*\\n+", 0, 0, 0);

      g_once_init_leave (&splitter, s);
    }

  lines = xregex_split (splitter, orig, 0);
  for (i = 0; lines[i]; i++)
    {
      xchar_t *a, *b, *c;

      a = xregex_replace_literal (cleanup[0], lines[i], -1, 0, "", 0, 0);
      b = xregex_replace_literal (cleanup[1], a, -1, 0, "", 0, 0);
      c = xregex_replace_literal (cleanup[2], b, -1, 0, " ", 0, 0);
      g_free (lines[i]);
      g_free (a);
      g_free (b);
      lines[i] = c;
    }

  result = xstrjoinv ("\n\n", lines);
  xstrfreev (lines);

  return result;
}

static void
end_element (xmarkup_parse_context_t *context,
             const xchar_t *element_name,
             xpointer_t user_data,
             xerror_t **error)
{
  TextTableParseInfo *info = user_data;

  pop_attribute_value (&info->gettext_domain);
  pop_attribute_value (&info->schema_id);
  pop_attribute_value (&info->key_name);

  if (info->string)
    {
      xhashtable_t *source_table = NULL;
      const xchar_t *gettext_domain;
      const xchar_t *schema_id;
      const xchar_t *key_name;

      gettext_domain = get_attribute_value (info->gettext_domain);
      schema_id = get_attribute_value (info->schema_id);
      key_name = get_attribute_value (info->key_name);

      if (xstr_equal (element_name, "summary"))
        source_table = info->summaries;
      else if (xstr_equal (element_name, "description"))
        source_table = info->descriptions;

      if (source_table && schema_id && key_name)
        {
          xhashtable_t *schema_table;
          xchar_t *normalised;

          schema_table = xhash_table_lookup (source_table, schema_id);

          if (schema_table == NULL)
            {
              schema_table = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);
              xhash_table_insert (source_table, xstrdup (schema_id), schema_table);
            }

          normalised = normalise_whitespace (info->string->str);

          if (gettext_domain && normalised[0])
            {
              xchar_t *translated;

              translated = xstrdup (g_dgettext (gettext_domain, normalised));
              g_free (normalised);
              normalised = translated;
            }

          xhash_table_insert (schema_table, xstrdup (key_name), normalised);
        }

      xstring_free (info->string, TRUE);
      info->string = NULL;
    }
}

static void
text (xmarkup_parse_context_t  *context,
      const xchar_t          *text,
      xsize_t                 text_len,
      xpointer_t              user_data,
      xerror_t              **error)
{
  TextTableParseInfo *info = user_data;

  if (info->string)
    xstring_append_len (info->string, text, text_len);
}

static void
parse_into_text_tables (const xchar_t *directory,
                        xhashtable_t  *summaries,
                        xhashtable_t  *descriptions)
{
  GMarkupParser parser = { start_element, end_element, text, NULL, NULL };
  TextTableParseInfo info = { summaries, descriptions, NULL, NULL, NULL, NULL };
  const xchar_t *basename;
  xdir_t *dir;

  dir = g_dir_open (directory, 0, NULL);
  while ((basename = g_dir_read_name (dir)))
    {
      xchar_t *filename;
      xchar_t *contents;
      xsize_t size;

      filename = g_build_filename (directory, basename, NULL);
      if (xfile_get_contents (filename, &contents, &size, NULL))
        {
          xmarkup_parse_context_t *context;

          context = xmarkup_parse_context_new (&parser, G_MARKUP_TREAT_CDATA_AS_TEXT, &info, NULL);
          /* Ignore errors here, this is best effort only. */
          if (xmarkup_parse_context_parse (context, contents, size, NULL))
            (void) xmarkup_parse_context_end_parse (context, NULL);
          xmarkup_parse_context_free (context);

          /* Clean up dangling stuff in case there was an error. */
          xslist_free_full (info.gettext_domain, g_free);
          xslist_free_full (info.schema_id, g_free);
          xslist_free_full (info.key_name, g_free);

          info.gettext_domain = NULL;
          info.schema_id = NULL;
          info.key_name = NULL;

          if (info.string)
            {
              xstring_free (info.string, TRUE);
              info.string = NULL;
            }

          g_free (contents);
        }

      g_free (filename);
    }

  g_dir_close (dir);
}

static xhashtable_t **
g_settings_schema_source_get_text_tables (xsettings_schema_source_t *source)
{
  if (g_once_init_enter (&source->text_tables))
    {
      xhashtable_t **text_tables;

      text_tables = g_new (xhashtable_t *, 2);
      text_tables[0] = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xhash_table_unref);
      text_tables[1] = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xhash_table_unref);

      if (source->directory)
        parse_into_text_tables (source->directory, text_tables[0], text_tables[1]);

      g_once_init_leave (&source->text_tables, text_tables);
    }

  return source->text_tables;
}

/**
 * g_settings_schema_source_list_schemas:
 * @source: a #xsettings_schema_source_t
 * @recursive: if we should recurse
 * @non_relocatable: (out) (transfer full) (array zero-terminated=1): the
 *   list of non-relocatable schemas, in no defined order
 * @relocatable: (out) (transfer full) (array zero-terminated=1): the list
 *   of relocatable schemas, in no defined order
 *
 * Lists the schemas in a given source.
 *
 * If @recursive is %TRUE then include parent sources.  If %FALSE then
 * only include the schemas from one source (ie: one directory).  You
 * probably want %TRUE.
 *
 * Non-relocatable schemas are those for which you can call
 * g_settings_new().  Relocatable schemas are those for which you must
 * use g_settings_new_with_path().
 *
 * Do not call this function from normal programs.  This is designed for
 * use by database editors, commandline tools, etc.
 *
 * Since: 2.40
 **/
void
g_settings_schema_source_list_schemas (xsettings_schema_source_t   *source,
                                       xboolean_t                 recursive,
                                       xchar_t                 ***non_relocatable,
                                       xchar_t                 ***relocatable)
{
  xhashtable_t *single, *reloc;
  xsettings_schema_source_t *s;

  /* We use hash tables to avoid duplicate listings for schemas that
   * appear in more than one file.
   */
  single = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);
  reloc = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);

  for (s = source; s; s = s->parent)
    {
      xchar_t **list;
      xint_t i;

      list = gvdb_table_list (s->table, "");

      /* empty schema cache file? */
      if (list == NULL)
        continue;

      for (i = 0; list[i]; i++)
        {
          if (!xhash_table_contains (single, list[i]) &&
              !xhash_table_contains (reloc, list[i]))
            {
              xchar_t *schema;
              GvdbTable *table;

              schema = xstrdup (list[i]);

              table = gvdb_table_get_table (s->table, list[i]);
              xassert (table != NULL);

              if (gvdb_table_has_value (table, ".path"))
                xhash_table_add (single, schema);
              else
                xhash_table_add (reloc, schema);

              gvdb_table_free (table);
            }
        }

      xstrfreev (list);

      /* Only the first source if recursive not requested */
      if (!recursive)
        break;
    }

  if (non_relocatable)
    {
      *non_relocatable = (xchar_t **) xhash_table_get_keys_as_array (single, NULL);
      xhash_table_steal_all (single);
    }

  if (relocatable)
    {
      *relocatable = (xchar_t **) xhash_table_get_keys_as_array (reloc, NULL);
      xhash_table_steal_all (reloc);
    }

  xhash_table_unref (single);
  xhash_table_unref (reloc);
}

static xchar_t **non_relocatable_schema_list;
static xchar_t **relocatable_schema_list;
static xsize_t schema_lists_initialised;

static void
ensure_schema_lists (void)
{
  if (g_once_init_enter (&schema_lists_initialised))
    {
      initialise_schema_sources ();

      g_settings_schema_source_list_schemas (schema_sources, TRUE,
                                             &non_relocatable_schema_list,
                                             &relocatable_schema_list);

      g_once_init_leave (&schema_lists_initialised, TRUE);
    }
}

/**
 * g_settings_list_schemas:
 *
 * Deprecated.
 *
 * Returns: (element-type utf8) (transfer none) (not nullable): a list of
 *   #xsettings_t schemas that are available, in no defined order.  The list
 *   must not be modified or freed.
 *
 * Since: 2.26
 *
 * Deprecated: 2.40: Use g_settings_schema_source_list_schemas() instead.
 * If you used g_settings_list_schemas() to check for the presence of
 * a particular schema, use g_settings_schema_source_lookup() instead
 * of your whole loop.
 **/
const xchar_t * const *
g_settings_list_schemas (void)
{
  ensure_schema_lists ();

  return (const xchar_t **) non_relocatable_schema_list;
}

/**
 * g_settings_list_relocatable_schemas:
 *
 * Deprecated.
 *
 * Returns: (element-type utf8) (transfer none) (not nullable): a list of
 *   relocatable #xsettings_t schemas that are available, in no defined order.
 *   The list must not be modified or freed.
 *
 * Since: 2.28
 *
 * Deprecated: 2.40: Use g_settings_schema_source_list_schemas() instead
 **/
const xchar_t * const *
g_settings_list_relocatable_schemas (void)
{
  ensure_schema_lists ();

  return (const xchar_t **) relocatable_schema_list;
}

/**
 * g_settings_schema_ref:
 * @schema: a #xsettings_schema_t
 *
 * Increase the reference count of @schema, returning a new reference.
 *
 * Returns: (transfer full) (not nullable): a new reference to @schema
 *
 * Since: 2.32
 **/
xsettings_schema_t *
g_settings_schema_ref (xsettings_schema_t *schema)
{
  g_atomic_int_inc (&schema->ref_count);

  return schema;
}

/**
 * g_settings_schema_unref:
 * @schema: a #xsettings_schema_t
 *
 * Decrease the reference count of @schema, possibly freeing it.
 *
 * Since: 2.32
 **/
void
g_settings_schema_unref (xsettings_schema_t *schema)
{
  if (g_atomic_int_dec_and_test (&schema->ref_count))
    {
      if (schema->extends)
        g_settings_schema_unref (schema->extends);

      g_settings_schema_source_unref (schema->source);
      gvdb_table_free (schema->table);
      g_free (schema->items);
      g_free (schema->id);

      g_slice_free (xsettings_schema_t, schema);
    }
}

const xchar_t *
g_settings_schema_get_string (xsettings_schema_t *schema,
                              const xchar_t     *key)
{
  const xchar_t *result = NULL;
  xvariant_t *value;

  if ((value = gvdb_table_get_raw_value (schema->table, key)))
    {
      result = xvariant_get_string (value, NULL);
      xvariant_unref (value);
    }

  return result;
}

xsettings_schema_t *
g_settings_schema_get_child_schema (xsettings_schema_t *schema,
                                    const xchar_t     *name)
{
  const xchar_t *child_id;
  xchar_t *child_name;

  child_name = xstrconcat (name, "/", NULL);
  child_id = g_settings_schema_get_string (schema, child_name);

  g_free (child_name);

  if (child_id == NULL)
    return NULL;

  return g_settings_schema_source_lookup (schema->source, child_id, TRUE);
}

xvariant_iter_t *
g_settings_schema_get_value (xsettings_schema_t *schema,
                             const xchar_t     *key)
{
  xsettings_schema_t *s = schema;
  xvariant_iter_t *iter;
  xvariant_t *value = NULL;

  xreturn_val_if_fail (schema != NULL, NULL);

  for (s = schema; s; s = s->extends)
    if ((value = gvdb_table_get_raw_value (s->table, key)))
      break;

  if G_UNLIKELY (value == NULL || !xvariant_is_of_type (value, G_VARIANT_TYPE_TUPLE))
    xerror ("Settings schema '%s' does not contain a key named '%s'", schema->id, key);

  iter = xvariant_iter_new (value);
  xvariant_unref (value);

  return iter;
}

/**
 * g_settings_schema_get_path:
 * @schema: a #xsettings_schema_t
 *
 * Gets the path associated with @schema, or %NULL.
 *
 * Schemas may be single-instance or relocatable.  Single-instance
 * schemas correspond to exactly one set of keys in the backend
 * database: those located at the path returned by this function.
 *
 * Relocatable schemas can be referenced by other schemas and can
 * therefore describe multiple sets of keys at different locations.  For
 * relocatable schemas, this function will return %NULL.
 *
 * Returns: (nullable) (transfer none): the path of the schema, or %NULL
 *
 * Since: 2.32
 **/
const xchar_t *
g_settings_schema_get_path (xsettings_schema_t *schema)
{
  return schema->path;
}

const xchar_t *
g_settings_schema_get_gettext_domain (xsettings_schema_t *schema)
{
  return schema->gettext_domain;
}

/**
 * g_settings_schema_has_key:
 * @schema: a #xsettings_schema_t
 * @name: the name of a key
 *
 * Checks if @schema has a key named @name.
 *
 * Returns: %TRUE if such a key exists
 *
 * Since: 2.40
 **/
xboolean_t
g_settings_schema_has_key (xsettings_schema_t *schema,
                           const xchar_t     *key)
{
  return gvdb_table_has_value (schema->table, key);
}

/**
 * g_settings_schema_list_children:
 * @schema: a #xsettings_schema_t
 *
 * Gets the list of children in @schema.
 *
 * You should free the return value with xstrfreev() when you are done
 * with it.
 *
 * Returns: (not nullable) (transfer full) (element-type utf8): a list of
 *    the children on @settings, in no defined order
 *
 * Since: 2.44
 */
xchar_t **
g_settings_schema_list_children (xsettings_schema_t *schema)
{
  const xquark *keys;
  xchar_t **strv;
  xint_t n_keys;
  xint_t i, j;

  xreturn_val_if_fail (schema != NULL, NULL);

  keys = g_settings_schema_list (schema, &n_keys);
  strv = g_new (xchar_t *, n_keys + 1);
  for (i = j = 0; i < n_keys; i++)
    {
      const xchar_t *key = g_quark_to_string (keys[i]);

      if (xstr_has_suffix (key, "/"))
        {
          xsize_t length = strlen (key);

          strv[j] = g_memdup2 (key, length);
          strv[j][length - 1] = '\0';
          j++;
        }
    }
  strv[j] = NULL;

  return strv;
}

/**
 * g_settings_schema_list_keys:
 * @schema: a #xsettings_schema_t
 *
 * Introspects the list of keys on @schema.
 *
 * You should probably not be calling this function from "normal" code
 * (since you should already know what keys are in your schema).  This
 * function is intended for introspection reasons.
 *
 * Returns: (not nullable) (transfer full) (element-type utf8): a list
 *   of the keys on @schema, in no defined order
 *
 * Since: 2.46
 */
xchar_t **
g_settings_schema_list_keys (xsettings_schema_t *schema)
{
  const xquark *keys;
  xchar_t **strv;
  xint_t n_keys;
  xint_t i, j;

  xreturn_val_if_fail (schema != NULL, NULL);

  keys = g_settings_schema_list (schema, &n_keys);
  strv = g_new (xchar_t *, n_keys + 1);
  for (i = j = 0; i < n_keys; i++)
    {
      const xchar_t *key = g_quark_to_string (keys[i]);

      if (!xstr_has_suffix (key, "/"))
        strv[j++] = xstrdup (key);
    }
  strv[j] = NULL;

  return strv;
}

const xquark *
g_settings_schema_list (xsettings_schema_t *schema,
                        xint_t            *n_items)
{
  if (schema->items == NULL)
    {
      xsettings_schema_t *s;
      xhash_table_iter_t iter;
      xhashtable_t *items;
      xpointer_t name;
      xint_t len;
      xint_t i;

      items = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);

      for (s = schema; s; s = s->extends)
        {
          xchar_t **list;

          list = gvdb_table_list (s->table, "");

          if (list)
            {
              for (i = 0; list[i]; i++)
                xhash_table_add (items, list[i]); /* transfer ownership */

              g_free (list); /* free container only */
            }
        }

      /* Do a first pass to eliminate child items that do not map to
       * valid schemas (ie: ones that would crash us if we actually
       * tried to create them).
       */
      xhash_table_iter_init (&iter, items);
      while (xhash_table_iter_next (&iter, &name, NULL))
        if (xstr_has_suffix (name, "/"))
          {
            xsettings_schema_source_t *source;
            xvariant_t *child_schema;
            GvdbTable *child_table;

            child_schema = gvdb_table_get_raw_value (schema->table, name);
            if (!child_schema)
              continue;

            child_table = NULL;

            for (source = schema->source; source; source = source->parent)
              if ((child_table = gvdb_table_get_table (source->table, xvariant_get_string (child_schema, NULL))))
                break;

            xvariant_unref (child_schema);

            /* Schema is not found -> remove it from the list */
            if (child_table == NULL)
              {
                xhash_table_iter_remove (&iter);
                continue;
              }

            /* Make sure the schema is relocatable or at the
             * expected path
             */
            if (gvdb_table_has_value (child_table, ".path"))
              {
                xvariant_t *path;
                xchar_t *expected;
                xboolean_t same;

                path = gvdb_table_get_raw_value (child_table, ".path");
                expected = xstrconcat (schema->path, name, NULL);
                same = xstr_equal (expected, xvariant_get_string (path, NULL));
                xvariant_unref (path);
                g_free (expected);

                /* Schema is non-relocatable and did not have the
                 * expected path -> remove it from the list
                 */
                if (!same)
                  xhash_table_iter_remove (&iter);
              }

            gvdb_table_free (child_table);
          }

      /* Now create the list */
      len = xhash_table_size (items);
      schema->items = g_new (xquark, len);
      i = 0;
      xhash_table_iter_init (&iter, items);

      while (xhash_table_iter_next (&iter, &name, NULL))
        schema->items[i++] = g_quark_from_string (name);
      schema->n_items = i;
      xassert (i == len);

      xhash_table_unref (items);
    }

  *n_items = schema->n_items;
  return schema->items;
}

/**
 * g_settings_schema_get_id:
 * @schema: a #xsettings_schema_t
 *
 * Get the ID of @schema.
 *
 * Returns: (not nullable) (transfer none): the ID
 **/
const xchar_t *
g_settings_schema_get_id (xsettings_schema_t *schema)
{
  return schema->id;
}

static inline void
endian_fixup (xvariant_t **value)
{
#if G_BYTE_ORDER == G_BIG_ENDIAN
  xvariant_t *tmp;

  tmp = xvariant_byteswap (*value);
  xvariant_unref (*value);
  *value = tmp;
#endif
}

void
g_settings_schema_key_init (xsettings_schema_key_t *key,
                            xsettings_schema_t    *schema,
                            const xchar_t        *name)
{
  xvariant_iter_t *iter;
  xvariant_t *data;
  xuchar_t code;

  memset (key, 0, sizeof *key);

  iter = g_settings_schema_get_value (schema, name);

  key->schema = g_settings_schema_ref (schema);
  key->default_value = xvariant_iter_next_value (iter);
  endian_fixup (&key->default_value);
  key->type = xvariant_get_type (key->default_value);
  key->name = g_intern_string (name);

  while (xvariant_iter_next (iter, "(y*)", &code, &data))
    {
      switch (code)
        {
        case 'l':
          /* translation requested */
          xvariant_get (data, "(y&s)", &key->lc_char, &key->unparsed);
          break;

        case 'e':
          /* enumerated types... */
          key->is_enum = TRUE;
          goto choice;

        case 'f':
          /* flags... */
          key->is_flags = TRUE;
          goto choice;

        choice: case 'c':
          /* ..., choices, aliases */
          key->strinfo = xvariant_get_fixed_array (data, &key->strinfo_length, sizeof (xuint32_t));
          break;

        case 'r':
          xvariant_get (data, "(**)", &key->minimum, &key->maximum);
          endian_fixup (&key->minimum);
          endian_fixup (&key->maximum);
          break;

        case 'd':
          xvariant_get (data, "@a{sv}", &key->desktop_overrides);
          endian_fixup (&key->desktop_overrides);
          break;

        default:
          g_warning ("unknown schema extension '%c'", code);
          break;
        }

      xvariant_unref (data);
    }

  xvariant_iter_free (iter);
}

void
g_settings_schema_key_clear (xsettings_schema_key_t *key)
{
  if (key->minimum)
    xvariant_unref (key->minimum);

  if (key->maximum)
    xvariant_unref (key->maximum);

  if (key->desktop_overrides)
    xvariant_unref (key->desktop_overrides);

  xvariant_unref (key->default_value);

  g_settings_schema_unref (key->schema);
}

xboolean_t
g_settings_schema_key_type_check (xsettings_schema_key_t *key,
                                  xvariant_t           *value)
{
  xreturn_val_if_fail (value != NULL, FALSE);

  return xvariant_is_of_type (value, key->type);
}

xvariant_t *
g_settings_schema_key_range_fixup (xsettings_schema_key_t *key,
                                   xvariant_t           *value)
{
  const xchar_t *target;

  if (g_settings_schema_key_range_check (key, value))
    return xvariant_ref (value);

  if (key->strinfo == NULL)
    return NULL;

  if (xvariant_is_container (value))
    {
      xvariant_builder_t builder;
      xvariant_iter_t iter;
      xvariant_t *child;

      xvariant_iter_init (&iter, value);
      xvariant_builder_init (&builder, xvariant_get_type (value));

      while ((child = xvariant_iter_next_value (&iter)))
        {
          xvariant_t *fixed;

          fixed = g_settings_schema_key_range_fixup (key, child);
          xvariant_unref (child);

          if (fixed == NULL)
            {
              xvariant_builder_clear (&builder);
              return NULL;
            }

          xvariant_builder_add_value (&builder, fixed);
          xvariant_unref (fixed);
        }

      return xvariant_ref_sink (xvariant_builder_end (&builder));
    }

  target = strinfo_string_from_alias (key->strinfo, key->strinfo_length,
                                      xvariant_get_string (value, NULL));
  return target ? xvariant_ref_sink (xvariant_new_string (target)) : NULL;
}

xvariant_t *
g_settings_schema_key_get_translated_default (xsettings_schema_key_t *key)
{
  const xchar_t *translated;
  xerror_t *error = NULL;
  const xchar_t *domain;
  xvariant_t *value;

  domain = g_settings_schema_get_gettext_domain (key->schema);

  if (key->lc_char == '\0')
    /* translation not requested for this key */
    return NULL;

  if (key->lc_char == 't')
    translated = g_dcgettext (domain, key->unparsed, LC_TIME);
  else
    translated = g_dgettext (domain, key->unparsed);

  if (translated == key->unparsed)
    /* the default value was not translated */
    return NULL;

  /* try to parse the translation of the unparsed default */
  value = xvariant_parse (key->type, translated, NULL, NULL, &error);

  if (value == NULL)
    {
      g_warning ("Failed to parse translated string '%s' for "
                 "key '%s' in schema '%s': %s", translated, key->name,
                 g_settings_schema_get_id (key->schema), error->message);
      g_warning ("Using untranslated default instead.");
      xerror_free (error);
    }

  else if (!g_settings_schema_key_range_check (key, value))
    {
      g_warning ("Translated default '%s' for key '%s' in schema '%s' "
                 "is outside of valid range", key->unparsed, key->name,
                 g_settings_schema_get_id (key->schema));
      xvariant_unref (value);
      value = NULL;
    }

  return value;
}

xvariant_t *
g_settings_schema_key_get_per_desktop_default (xsettings_schema_key_t *key)
{
  static const xchar_t * const *current_desktops;
  xvariant_t *value = NULL;
  xint_t i;

  if (!key->desktop_overrides)
    return NULL;

  if (g_once_init_enter (&current_desktops))
    {
      const xchar_t *xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
      xchar_t **tmp;

      if (xdg_current_desktop != NULL && xdg_current_desktop[0] != '\0')
        tmp = xstrsplit (xdg_current_desktop, G_SEARCHPATH_SEPARATOR_S, -1);
      else
        tmp = g_new0 (xchar_t *, 0 + 1);

      g_once_init_leave (&current_desktops, (const xchar_t **) tmp);
    }

  for (i = 0; value == NULL && current_desktops[i] != NULL; i++)
    value = xvariant_lookup_value (key->desktop_overrides, current_desktops[i], NULL);

  return value;
}

xint_t
g_settings_schema_key_to_enum (xsettings_schema_key_t *key,
                               xvariant_t           *value)
{
  xboolean_t it_worked G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
  xuint_t result;

  it_worked = strinfo_enum_from_string (key->strinfo, key->strinfo_length,
                                        xvariant_get_string (value, NULL),
                                        &result);

  /* 'value' can only come from the backend after being filtered for validity,
   * from the translation after being filtered for validity, or from the schema
   * itself (which the schema compiler checks for validity).  If this assertion
   * fails then it's really a bug in xsettings_t or the schema compiler...
   */
  xassert (it_worked);

  return result;
}

/* Returns a new floating #xvariant_t. */
xvariant_t *
g_settings_schema_key_from_enum (xsettings_schema_key_t *key,
                                 xint_t                value)
{
  const xchar_t *string;

  string = strinfo_string_from_enum (key->strinfo, key->strinfo_length, value);

  if (string == NULL)
    return NULL;

  return xvariant_new_string (string);
}

xuint_t
g_settings_schema_key_to_flags (xsettings_schema_key_t *key,
                                xvariant_t           *value)
{
  xvariant_iter_t iter;
  const xchar_t *flag;
  xuint_t result;

  result = 0;
  xvariant_iter_init (&iter, value);
  while (xvariant_iter_next (&iter, "&s", &flag))
    {
      xboolean_t it_worked G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
      xuint_t flaxvalue;

      it_worked = strinfo_enum_from_string (key->strinfo, key->strinfo_length, flag, &flaxvalue);
      /* as in g_settings_to_enum() */
      xassert (it_worked);

      result |= flaxvalue;
    }

  return result;
}

/* Returns a new floating #xvariant_t. */
xvariant_t *
g_settings_schema_key_from_flags (xsettings_schema_key_t *key,
                                  xuint_t               value)
{
  xvariant_builder_t builder;
  xint_t i;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("as"));

  for (i = 0; i < 32; i++)
    if (value & (1u << i))
      {
        const xchar_t *string;

        string = strinfo_string_from_enum (key->strinfo, key->strinfo_length, 1u << i);

        if (string == NULL)
          {
            xvariant_builder_clear (&builder);
            return NULL;
          }

        xvariant_builder_add (&builder, "s", string);
      }

  return xvariant_builder_end (&builder);
}

G_DEFINE_BOXED_TYPE (xsettings_schema_key_t, g_settings_schema_key, g_settings_schema_key_ref, g_settings_schema_key_unref)

/**
 * g_settings_schema_key_ref:
 * @key: a #xsettings_schema_key_t
 *
 * Increase the reference count of @key, returning a new reference.
 *
 * Returns: (not nullable) (transfer full): a new reference to @key
 *
 * Since: 2.40
 **/
xsettings_schema_key_t *
g_settings_schema_key_ref (xsettings_schema_key_t *key)
{
  xreturn_val_if_fail (key != NULL, NULL);

  g_atomic_int_inc (&key->ref_count);

  return key;
}

/**
 * g_settings_schema_key_unref:
 * @key: a #xsettings_schema_key_t
 *
 * Decrease the reference count of @key, possibly freeing it.
 *
 * Since: 2.40
 **/
void
g_settings_schema_key_unref (xsettings_schema_key_t *key)
{
  g_return_if_fail (key != NULL);

  if (g_atomic_int_dec_and_test (&key->ref_count))
    {
      g_settings_schema_key_clear (key);

      g_slice_free (xsettings_schema_key_t, key);
    }
}

/**
 * g_settings_schema_get_key:
 * @schema: a #xsettings_schema_t
 * @name: the name of a key
 *
 * Gets the key named @name from @schema.
 *
 * It is a programmer error to request a key that does not exist.  See
 * g_settings_schema_list_keys().
 *
 * Returns: (not nullable) (transfer full): the #xsettings_schema_key_t for @name
 *
 * Since: 2.40
 **/
xsettings_schema_key_t *
g_settings_schema_get_key (xsettings_schema_t *schema,
                           const xchar_t     *name)
{
  xsettings_schema_key_t *key;

  xreturn_val_if_fail (schema != NULL, NULL);
  xreturn_val_if_fail (name != NULL, NULL);

  key = g_slice_new (xsettings_schema_key_t);
  g_settings_schema_key_init (key, schema, name);
  key->ref_count = 1;

  return key;
}

/**
 * g_settings_schema_key_get_name:
 * @key: a #xsettings_schema_key_t
 *
 * Gets the name of @key.
 *
 * Returns: (not nullable) (transfer none): the name of @key.
 *
 * Since: 2.44
 */
const xchar_t *
g_settings_schema_key_get_name (xsettings_schema_key_t *key)
{
  xreturn_val_if_fail (key != NULL, NULL);

  return key->name;
}

/**
 * g_settings_schema_key_get_summary:
 * @key: a #xsettings_schema_key_t
 *
 * Gets the summary for @key.
 *
 * If no summary has been provided in the schema for @key, returns
 * %NULL.
 *
 * The summary is a short description of the purpose of the key; usually
 * one short sentence.  Summaries can be translated and the value
 * returned from this function is is the current locale.
 *
 * This function is slow.  The summary and description information for
 * the schemas is not stored in the compiled schema database so this
 * function has to parse all of the source XML files in the schema
 * directory.
 *
 * Returns: (nullable) (transfer none): the summary for @key, or %NULL
 *
 * Since: 2.34
 **/
const xchar_t *
g_settings_schema_key_get_summary (xsettings_schema_key_t *key)
{
  xhashtable_t **text_tables;
  xhashtable_t *summaries;

  text_tables = g_settings_schema_source_get_text_tables (key->schema->source);
  summaries = xhash_table_lookup (text_tables[0], key->schema->id);

  return summaries ? xhash_table_lookup (summaries, key->name) : NULL;
}

/**
 * g_settings_schema_key_get_description:
 * @key: a #xsettings_schema_key_t
 *
 * Gets the description for @key.
 *
 * If no description has been provided in the schema for @key, returns
 * %NULL.
 *
 * The description can be one sentence to several paragraphs in length.
 * Paragraphs are delimited with a double newline.  Descriptions can be
 * translated and the value returned from this function is is the
 * current locale.
 *
 * This function is slow.  The summary and description information for
 * the schemas is not stored in the compiled schema database so this
 * function has to parse all of the source XML files in the schema
 * directory.
 *
 * Returns: (nullable) (transfer none): the description for @key, or %NULL
 *
 * Since: 2.34
 **/
const xchar_t *
g_settings_schema_key_get_description (xsettings_schema_key_t *key)
{
  xhashtable_t **text_tables;
  xhashtable_t *descriptions;

  text_tables = g_settings_schema_source_get_text_tables (key->schema->source);
  descriptions = xhash_table_lookup (text_tables[1], key->schema->id);

  return descriptions ? xhash_table_lookup (descriptions, key->name) : NULL;
}

/**
 * g_settings_schema_key_get_value_type:
 * @key: a #xsettings_schema_key_t
 *
 * Gets the #xvariant_type_t of @key.
 *
 * Returns: (not nullable) (transfer none): the type of @key
 *
 * Since: 2.40
 **/
const xvariant_type_t *
g_settings_schema_key_get_value_type (xsettings_schema_key_t *key)
{
  xreturn_val_if_fail (key, NULL);

  return key->type;
}

/**
 * g_settings_schema_key_get_default_value:
 * @key: a #xsettings_schema_key_t
 *
 * Gets the default value for @key.
 *
 * Note that this is the default value according to the schema.  System
 * administrator defaults and lockdown are not visible via this API.
 *
 * Returns: (not nullable) (transfer full): the default value for the key
 *
 * Since: 2.40
 **/
xvariant_t *
g_settings_schema_key_get_default_value (xsettings_schema_key_t *key)
{
  xvariant_t *value;

  xreturn_val_if_fail (key, NULL);

  value = g_settings_schema_key_get_translated_default (key);

  if (!value)
    value = g_settings_schema_key_get_per_desktop_default (key);

  if (!value)
    value = xvariant_ref (key->default_value);

  return value;
}

/**
 * g_settings_schema_key_get_range:
 * @key: a #xsettings_schema_key_t
 *
 * Queries the range of a key.
 *
 * This function will return a #xvariant_t that fully describes the range
 * of values that are valid for @key.
 *
 * The type of #xvariant_t returned is `(sv)`. The string describes
 * the type of range restriction in effect. The type and meaning of
 * the value contained in the variant depends on the string.
 *
 * If the string is `'type'` then the variant contains an empty array.
 * The element type of that empty array is the expected type of value
 * and all values of that type are valid.
 *
 * If the string is `'enum'` then the variant contains an array
 * enumerating the possible values. Each item in the array is
 * a possible valid value and no other values are valid.
 *
 * If the string is `'flags'` then the variant contains an array. Each
 * item in the array is a value that may appear zero or one times in an
 * array to be used as the value for this key. For example, if the
 * variant contained the array `['x', 'y']` then the valid values for
 * the key would be `[]`, `['x']`, `['y']`, `['x', 'y']` and
 * `['y', 'x']`.
 *
 * Finally, if the string is `'range'` then the variant contains a pair
 * of like-typed values -- the minimum and maximum permissible values
 * for this key.
 *
 * This information should not be used by normal programs.  It is
 * considered to be a hint for introspection purposes.  Normal programs
 * should already know what is permitted by their own schema.  The
 * format may change in any way in the future -- but particularly, new
 * forms may be added to the possibilities described above.
 *
 * You should free the returned value with xvariant_unref() when it is
 * no longer needed.
 *
 * Returns: (not nullable) (transfer full): a #xvariant_t describing the range
 *
 * Since: 2.40
 **/
xvariant_t *
g_settings_schema_key_get_range (xsettings_schema_key_t *key)
{
  const xchar_t *type;
  xvariant_t *range;

  if (key->minimum)
    {
      range = xvariant_new ("(**)", key->minimum, key->maximum);
      type = "range";
    }
  else if (key->strinfo)
    {
      range = strinfo_enumerate (key->strinfo, key->strinfo_length);
      type = key->is_flags ? "flags" : "enum";
    }
  else
    {
      range = xvariant_new_array (key->type, NULL, 0);
      type = "type";
    }

  return xvariant_ref_sink (xvariant_new ("(sv)", type, range));
}

/**
 * g_settings_schema_key_range_check:
 * @key: a #xsettings_schema_key_t
 * @value: the value to check
 *
 * Checks if the given @value is within the
 * permitted range for @key.
 *
 * It is a programmer error if @value is not of the correct type — you
 * must check for this first.
 *
 * Returns: %TRUE if @value is valid for @key
 *
 * Since: 2.40
 **/
xboolean_t
g_settings_schema_key_range_check (xsettings_schema_key_t *key,
                                   xvariant_t           *value)
{
  if (key->minimum == NULL && key->strinfo == NULL)
    return TRUE;

  if (xvariant_is_container (value))
    {
      xboolean_t ok = TRUE;
      xvariant_iter_t iter;
      xvariant_t *child;

      xvariant_iter_init (&iter, value);
      while (ok && (child = xvariant_iter_next_value (&iter)))
        {
          ok = g_settings_schema_key_range_check (key, child);
          xvariant_unref (child);
        }

      return ok;
    }

  if (key->minimum)
    {
      return xvariant_compare (key->minimum, value) <= 0 &&
             xvariant_compare (value, key->maximum) <= 0;
    }

  return strinfo_is_string_valid (key->strinfo, key->strinfo_length,
                                  xvariant_get_string (value, NULL));
}
