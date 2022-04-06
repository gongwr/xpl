/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2010 Novell, Inc.
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
 *
 * Authors: Vincent Untz <vuntz@gnome.org>
 *          Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include <glib.h>
#include <glibintl.h>

#include <stdio.h>
#include <string.h>

#include "gfile.h"
#include "gfileinfo.h"
#include "gfileenumerator.h"
#include "gfilemonitor.h"
#include "gsimplepermission.h"
#include "gsettingsbackendinternal.h"
#include "giomodule-priv.h"
#include "gportalsupport.h"


#define XTYPE_KEYFILE_SETTINGS_BACKEND      (g_keyfile_settings_backend_get_type ())
#define G_KEYFILE_SETTINGS_BACKEND(inst)     (XTYPE_CHECK_INSTANCE_CAST ((inst),      \
                                              XTYPE_KEYFILE_SETTINGS_BACKEND,         \
                                              GKeyfileSettingsBackend))
#define X_IS_KEYFILE_SETTINGS_BACKEND(inst)  (XTYPE_CHECK_INSTANCE_TYPE ((inst),      \
                                              XTYPE_KEYFILE_SETTINGS_BACKEND))


typedef GSettingsBackendClass GKeyfileSettingsBackendClass;

typedef enum {
  PROP_FILENAME = 1,
  PROP_ROOT_PATH,
  PROP_ROOT_GROUP,
  PROP_DEFAULTS_DIR
} GKeyfileSettingsBackendProperty;

typedef struct
{
  xsettings_backend_t   parent_instance;

  xkey_file_t          *keyfile;
  xpermission_t       *permission;
  xboolean_t           writable;
  char              *defaults_dir;
  xkey_file_t          *system_keyfile;
  xhashtable_t        *system_locks; /* Used as a set, owning the strings it contains */

  xchar_t             *prefix;
  xsize_t              prefix_len;
  xchar_t             *root_group;
  xsize_t              root_group_len;

  xfile_t             *file;
  xfile_monitor_t      *file_monitor;
  xuint8_t             digest[32];
  xfile_t             *dir;
  xfile_monitor_t      *dir_monitor;
} GKeyfileSettingsBackend;

#ifdef G_OS_WIN32
#define EXTENSION_PRIORITY 10
#else
#define EXTENSION_PRIORITY (glib_should_use_portal () && !glib_has_dconf_access_in_sandbox () ? 110 : 10)
#endif

G_DEFINE_TYPE_WITH_CODE (GKeyfileSettingsBackend,
                         g_keyfile_settings_backend,
                         XTYPE_SETTINGS_BACKEND,
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
                                                         g_define_type_id, "keyfile", EXTENSION_PRIORITY))

static void
compute_checksum (xuint8_t        *digest,
                  xconstpointer  contents,
                  xsize_t          length)
{
  xchecksum_t *checksum;
  xsize_t len = 32;

  checksum = xchecksum_new (G_CHECKSUM_SHA256);
  xchecksum_update (checksum, contents, length);
  xchecksum_get_digest (checksum, digest, &len);
  xchecksum_free (checksum);
  g_assert (len == 32);
}

static xboolean_t
g_keyfile_settings_backend_keyfile_write (GKeyfileSettingsBackend  *kfsb,
                                          xerror_t                  **error)
{
  xchar_t *contents;
  xsize_t length;
  xboolean_t success;

  contents = xkey_file_to_data (kfsb->keyfile, &length, NULL);
  success = xfile_replace_contents (kfsb->file, contents, length, NULL, FALSE,
                                     XFILE_CREATE_REPLACE_DESTINATION |
                                     XFILE_CREATE_PRIVATE,
                                     NULL, NULL, error);

  compute_checksum (kfsb->digest, contents, length);
  g_free (contents);

  return success;
}

static xboolean_t
group_name_matches (const xchar_t *group_name,
                    const xchar_t *prefix)
{
  /* sort of like xstr_has_prefix() except that it must be an exact
   * match or the prefix followed by '/'.
   *
   * for example 'a' is a prefix of 'a' and 'a/b' but not 'ab'.
   */
  xint_t i;

  for (i = 0; prefix[i]; i++)
    if (prefix[i] != group_name[i])
      return FALSE;

  return group_name[i] == '\0' || group_name[i] == '/';
}

static xboolean_t
convert_path (GKeyfileSettingsBackend  *kfsb,
              const xchar_t              *key,
              xchar_t                   **group,
              xchar_t                   **basename)
{
  xsize_t key_len = strlen (key);
  const xchar_t *last_slash;

  if (key_len < kfsb->prefix_len ||
      memcmp (key, kfsb->prefix, kfsb->prefix_len) != 0)
    return FALSE;

  key_len -= kfsb->prefix_len;
  key += kfsb->prefix_len;

  last_slash = strrchr (key, '/');

  /* Disallow empty group names or key names */
  if (key_len == 0 ||
      (last_slash != NULL &&
       (*(last_slash + 1) == '\0' ||
        last_slash == key)))
    return FALSE;

  if (kfsb->root_group)
    {
      /* if a root_group was specified, make sure the user hasn't given
       * a path that ghosts that group name
       */
      if (last_slash != NULL && last_slash - key >= 0 &&
          (xsize_t) (last_slash - key) == kfsb->root_group_len &&
          memcmp (key, kfsb->root_group, last_slash - key) == 0)
        return FALSE;
    }
  else
    {
      /* if no root_group was given, ensure that the user gave a path */
      if (last_slash == NULL)
        return FALSE;
    }

  if (group)
    {
      if (last_slash != NULL)
        {
          *group = g_memdup2 (key, (last_slash - key) + 1);
          (*group)[(last_slash - key)] = '\0';
        }
      else
        *group = xstrdup (kfsb->root_group);
    }

  if (basename)
    {
      if (last_slash != NULL)
        *basename = g_memdup2 (last_slash + 1, key_len - (last_slash - key));
      else
        *basename = xstrdup (key);
    }

  return TRUE;
}

static xboolean_t
path_is_valid (GKeyfileSettingsBackend *kfsb,
               const xchar_t             *path)
{
  return convert_path (kfsb, path, NULL, NULL);
}

static xvariant_t *
get_from_keyfile (GKeyfileSettingsBackend *kfsb,
                  const xvariant_type_t      *type,
                  const xchar_t             *key)
{
  xvariant_t *return_value = NULL;
  xchar_t *group, *name;

  if (convert_path (kfsb, key, &group, &name))
    {
      xchar_t *str;
      xchar_t *sysstr;

      g_assert (*name);

      sysstr = xkey_file_get_value (kfsb->system_keyfile, group, name, NULL);
      str = xkey_file_get_value (kfsb->keyfile, group, name, NULL);
      if (sysstr &&
          (xhash_table_contains (kfsb->system_locks, key) ||
           str == NULL))
        {
          g_free (str);
          str = g_steal_pointer (&sysstr);
        }

      if (str)
        {
          return_value = xvariant_parse (type, str, NULL, NULL, NULL);

          /* As a special case, support values of type %G_VARIANT_TYPE_STRING
           * not being quoted, since users keep forgetting to do it and then
           * getting confused. */
          if (return_value == NULL &&
              xvariant_type_equal (type, G_VARIANT_TYPE_STRING) &&
              str[0] != '\"')
            {
              xstring_t *s = xstring_sized_new (strlen (str) + 2);
              char *p = str;

              xstring_append_c (s, '\"');
              while (*p)
                {
                  if (*p == '\"')
                    xstring_append_c (s, '\\');
                  xstring_append_c (s, *p);
                  p++;
                }
              xstring_append_c (s, '\"');
              return_value = xvariant_parse (type, s->str, NULL, NULL, NULL);
              xstring_free (s, TRUE);
            }
          g_free (str);
        }

      g_free (sysstr);

      g_free (group);
      g_free (name);
    }

  return return_value;
}

static xboolean_t
set_to_keyfile (GKeyfileSettingsBackend *kfsb,
                const xchar_t             *key,
                xvariant_t                *value)
{
  xchar_t *group, *name;

  if (xhash_table_contains (kfsb->system_locks, key))
    return FALSE;

  if (convert_path (kfsb, key, &group, &name))
    {
      if (value)
        {
          xchar_t *str = xvariant_print (value, FALSE);
          xkey_file_set_value (kfsb->keyfile, group, name, str);
          xvariant_unref (xvariant_ref_sink (value));
          g_free (str);
        }
      else
        {
          if (*name == '\0')
            {
              xchar_t **groups;
              xint_t i;

              groups = xkey_file_get_groups (kfsb->keyfile, NULL);

              for (i = 0; groups[i]; i++)
                if (group_name_matches (groups[i], group))
                  xkey_file_remove_group (kfsb->keyfile, groups[i], NULL);

              xstrfreev (groups);
            }
          else
            xkey_file_remove_key (kfsb->keyfile, group, name, NULL);
        }

      g_free (group);
      g_free (name);

      return TRUE;
    }

  return FALSE;
}

static xvariant_t *
g_keyfile_settings_backend_read (xsettings_backend_t   *backend,
                                 const xchar_t        *key,
                                 const xvariant_type_t *expected_type,
                                 xboolean_t            default_value)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (backend);

  if (default_value)
    return NULL;

  return get_from_keyfile (kfsb, expected_type, key);
}

typedef struct
{
  GKeyfileSettingsBackend *kfsb;
  xboolean_t failed;
} WriteManyData;

static xboolean_t
g_keyfile_settings_backend_write_one (xpointer_t key,
                                      xpointer_t value,
                                      xpointer_t user_data)
{
  WriteManyData *data = user_data;
  xboolean_t success G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  success = set_to_keyfile (data->kfsb, key, value);
  g_assert (success);

  return FALSE;
}

static xboolean_t
g_keyfile_settings_backend_check_one (xpointer_t key,
                                      xpointer_t value,
                                      xpointer_t user_data)
{
  WriteManyData *data = user_data;

  return data->failed = xhash_table_contains (data->kfsb->system_locks, key) ||
                        !path_is_valid (data->kfsb, key);
}

static xboolean_t
g_keyfile_settings_backend_write_tree (xsettings_backend_t *backend,
                                       xtree_t            *tree,
                                       xpointer_t          origin_tag)
{
  WriteManyData data = { G_KEYFILE_SETTINGS_BACKEND (backend), 0 };
  xboolean_t success;
  xerror_t *error = NULL;

  if (!data.kfsb->writable)
    return FALSE;

  xtree_foreach (tree, g_keyfile_settings_backend_check_one, &data);

  if (data.failed)
    return FALSE;

  xtree_foreach (tree, g_keyfile_settings_backend_write_one, &data);
  success = g_keyfile_settings_backend_keyfile_write (data.kfsb, &error);
  if (error)
    {
      g_warning ("Failed to write keyfile to %s: %s", xfile_peek_path (data.kfsb->file), error->message);
      xerror_free (error);
    }

  g_settings_backend_changed_tree (backend, tree, origin_tag);

  return success;
}

static xboolean_t
g_keyfile_settings_backend_write (xsettings_backend_t *backend,
                                  const xchar_t      *key,
                                  xvariant_t         *value,
                                  xpointer_t          origin_tag)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (backend);
  xboolean_t success;
  xerror_t *error = NULL;

  if (!kfsb->writable)
    return FALSE;

  success = set_to_keyfile (kfsb, key, value);

  if (success)
    {
      g_settings_backend_changed (backend, key, origin_tag);
      success = g_keyfile_settings_backend_keyfile_write (kfsb, &error);
      if (error)
        {
          g_warning ("Failed to write keyfile to %s: %s", xfile_peek_path (kfsb->file), error->message);
          xerror_free (error);
        }
    }

  return success;
}

static void
g_keyfile_settings_backend_reset (xsettings_backend_t *backend,
                                  const xchar_t      *key,
                                  xpointer_t          origin_tag)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (backend);
  xerror_t *error = NULL;

  if (set_to_keyfile (kfsb, key, NULL))
    {
      g_keyfile_settings_backend_keyfile_write (kfsb, &error);
      if (error)
        {
          g_warning ("Failed to write keyfile to %s: %s", xfile_peek_path (kfsb->file), error->message);
          xerror_free (error);
        }
    }

  g_settings_backend_changed (backend, key, origin_tag);
}

static xboolean_t
g_keyfile_settings_backend_get_writable (xsettings_backend_t *backend,
                                         const xchar_t      *name)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (backend);

  return kfsb->writable &&
         !xhash_table_contains (kfsb->system_locks, name) &&
         path_is_valid (kfsb, name);
}

static xpermission_t *
g_keyfile_settings_backend_get_permission (xsettings_backend_t *backend,
                                           const xchar_t      *path)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (backend);

  return xobject_ref (kfsb->permission);
}

static void
keyfile_to_tree (GKeyfileSettingsBackend *kfsb,
                 xtree_t                   *tree,
                 xkey_file_t                *keyfile,
                 xboolean_t                 dup_check)
{
  xchar_t **groups;
  xint_t i;

  groups = xkey_file_get_groups (keyfile, NULL);
  for (i = 0; groups[i]; i++)
    {
      xboolean_t is_root_group;
      xchar_t **keys;
      xint_t j;

      is_root_group = xstrcmp0 (kfsb->root_group, groups[i]) == 0;

      /* reject group names that will form invalid key names */
      if (!is_root_group &&
          (xstr_has_prefix (groups[i], "/") ||
           xstr_has_suffix (groups[i], "/") || strstr (groups[i], "//")))
        continue;

      keys = xkey_file_get_keys (keyfile, groups[i], NULL, NULL);
      g_assert (keys != NULL);

      for (j = 0; keys[j]; j++)
        {
          xchar_t *path, *value;

          /* reject key names with slashes in them */
          if (strchr (keys[j], '/'))
            continue;

          if (is_root_group)
            path = xstrdup_printf ("%s%s", kfsb->prefix, keys[j]);
          else
            path = xstrdup_printf ("%s%s/%s", kfsb->prefix, groups[i], keys[j]);

          value = xkey_file_get_value (keyfile, groups[i], keys[j], NULL);

          if (dup_check && xstrcmp0 (xtree_lookup (tree, path), value) == 0)
            {
              xtree_remove (tree, path);
              g_free (value);
              g_free (path);
            }
          else
            xtree_insert (tree, path, value);
        }

      xstrfreev (keys);
    }
  xstrfreev (groups);
}

static void
g_keyfile_settings_backend_keyfile_reload (GKeyfileSettingsBackend *kfsb)
{
  xuint8_t digest[32];
  xchar_t *contents;
  xsize_t length;

  contents = NULL;
  length = 0;

  xfile_load_contents (kfsb->file, NULL, &contents, &length, NULL, NULL);
  compute_checksum (digest, contents, length);

  if (memcmp (kfsb->digest, digest, sizeof digest) != 0)
    {
      xkey_file_t *keyfiles[2];
      xtree_t *tree;

      tree = xtree_new_full ((GCompareDataFunc) strcmp, NULL,
                              g_free, g_free);

      keyfiles[0] = kfsb->keyfile;
      keyfiles[1] = xkey_file_new ();

      if (length > 0)
        xkey_file_load_from_data (keyfiles[1], contents, length,
                                   G_KEY_FILE_KEEP_COMMENTS |
                                   G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

      keyfile_to_tree (kfsb, tree, keyfiles[0], FALSE);
      keyfile_to_tree (kfsb, tree, keyfiles[1], TRUE);
      xkey_file_free (keyfiles[0]);
      kfsb->keyfile = keyfiles[1];

      if (xtree_nnodes (tree) > 0)
        g_settings_backend_changed_tree (&kfsb->parent_instance, tree, NULL);

      xtree_unref (tree);

      memcpy (kfsb->digest, digest, sizeof digest);
    }

  g_free (contents);
}

static void
g_keyfile_settings_backend_keyfile_writable (GKeyfileSettingsBackend *kfsb)
{
  xfile_info_t *fileinfo;
  xboolean_t writable;

  fileinfo = xfile_query_info (kfsb->dir, "access::*", 0, NULL, NULL);

  if (fileinfo)
    {
      writable =
        xfile_info_get_attribute_boolean (fileinfo, XFILE_ATTRIBUTE_ACCESS_CAN_WRITE) &&
        xfile_info_get_attribute_boolean (fileinfo, XFILE_ATTRIBUTE_ACCESS_CAN_EXECUTE);
      xobject_unref (fileinfo);
    }
  else
    writable = FALSE;

  if (writable != kfsb->writable)
    {
      kfsb->writable = writable;
      g_settings_backend_path_writable_changed (&kfsb->parent_instance, "/");
    }
}

static void
g_keyfile_settings_backend_finalize (xobject_t *object)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (object);

  xkey_file_free (kfsb->keyfile);
  xobject_unref (kfsb->permission);
  xkey_file_unref (kfsb->system_keyfile);
  xhash_table_unref (kfsb->system_locks);
  g_free (kfsb->defaults_dir);

  if (kfsb->file_monitor)
    {
      xfile_monitor_cancel (kfsb->file_monitor);
      xobject_unref (kfsb->file_monitor);
    }
  xobject_unref (kfsb->file);

  if (kfsb->dir_monitor)
    {
      xfile_monitor_cancel (kfsb->dir_monitor);
      xobject_unref (kfsb->dir_monitor);
    }
  xobject_unref (kfsb->dir);

  g_free (kfsb->root_group);
  g_free (kfsb->prefix);

  G_OBJECT_CLASS (g_keyfile_settings_backend_parent_class)->finalize (object);
}

static void
g_keyfile_settings_backend_init (GKeyfileSettingsBackend *kfsb)
{
}

static void
file_changed (xfile_monitor_t      *monitor,
              xfile_t             *file,
              xfile_t             *other_file,
              xfile_monitor_event_t  event_type,
              xpointer_t           user_data)
{
  GKeyfileSettingsBackend *kfsb = user_data;

  /* Ignore file deletions, let the xkey_file_t content remain in tact. */
  if (event_type != XFILE_MONITOR_EVENT_DELETED)
    g_keyfile_settings_backend_keyfile_reload (kfsb);
}

static void
dir_changed (xfile_monitor_t       *monitor,
              xfile_t             *file,
              xfile_t             *other_file,
              xfile_monitor_event_t  event_type,
              xpointer_t           user_data)
{
  GKeyfileSettingsBackend *kfsb = user_data;

  g_keyfile_settings_backend_keyfile_writable (kfsb);
}

static void
load_system_settings (GKeyfileSettingsBackend *kfsb)
{
  xerror_t *error = NULL;
  const char *dir = "/etc/glib-2.0/settings";
  char *path;
  char *contents;

  kfsb->system_keyfile = xkey_file_new ();
  kfsb->system_locks = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);

  if (kfsb->defaults_dir)
    dir = kfsb->defaults_dir;

  path = g_build_filename (dir, "defaults", NULL);

  /* The defaults are in the same keyfile format that we use for the settings.
   * It can be produced from a dconf database using: dconf dump
   */
  if (!xkey_file_load_from_file (kfsb->system_keyfile, path, G_KEY_FILE_NONE, &error))
    {
      if (!xerror_matches (error, XFILE_ERROR, XFILE_ERROR_NOENT))
        g_warning ("Failed to read %s: %s", path, error->message);
      g_clear_error (&error);
    }
  else
    g_debug ("Loading default settings from %s", path);

  g_free (path);

  path = g_build_filename (dir, "locks", NULL);

  /* The locks file is a text file containing a list paths to lock, one per line.
   * It can be produced from a dconf database using: dconf list-locks
   */
  if (!xfile_get_contents (path, &contents, NULL, &error))
    {
      if (!xerror_matches (error, XFILE_ERROR, XFILE_ERROR_NOENT))
        g_warning ("Failed to read %s: %s", path, error->message);
      g_clear_error (&error);
    }
  else
    {
      char **lines;
      xsize_t i;

      g_debug ("Loading locks from %s", path);

      lines = xstrsplit (contents, "\n", 0);
      for (i = 0; lines[i]; i++)
        {
          char *line = lines[i];
          if (line[0] == '#' || line[0] == '\0')
            {
              g_free (line);
              continue;
            }

          g_debug ("Locking key %s", line);
          xhash_table_add (kfsb->system_locks, g_steal_pointer (&line));
        }

      g_free (lines);
    }
  g_free (contents);

  g_free (path);
}

static void
g_keyfile_settings_backend_constructed (xobject_t *object)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (object);
  xerror_t *error = NULL;
  const char *path;

  if (kfsb->file == NULL)
    {
      char *filename = g_build_filename (g_get_user_config_dir (),
                                         "glib-2.0", "settings", "keyfile",
                                         NULL);
      kfsb->file = xfile_new_for_path (filename);
      g_free (filename);
    }

  if (kfsb->prefix == NULL)
    {
      kfsb->prefix = xstrdup ("/");
      kfsb->prefix_len = 1;
    }

  kfsb->keyfile = xkey_file_new ();
  kfsb->permission = g_simple_permission_new (TRUE);

  kfsb->dir = xfile_get_parent (kfsb->file);
  path = xfile_peek_path (kfsb->dir);
  if (g_mkdir_with_parents (path, 0700) == -1)
    g_warning ("Failed to create %s: %s", path, xstrerror (errno));

  kfsb->file_monitor = xfile_monitor (kfsb->file, XFILE_MONITOR_NONE, NULL, &error);
  if (!kfsb->file_monitor)
    {
      g_warning ("Failed to create file monitor for %s: %s", xfile_peek_path (kfsb->file), error->message);
      g_clear_error (&error);
    }
  else
    {
      g_signal_connect (kfsb->file_monitor, "changed",
                        G_CALLBACK (file_changed), kfsb);
    }

  kfsb->dir_monitor = xfile_monitor (kfsb->dir, XFILE_MONITOR_NONE, NULL, &error);
  if (!kfsb->dir_monitor)
    {
      g_warning ("Failed to create file monitor for %s: %s", xfile_peek_path (kfsb->file), error->message);
      g_clear_error (&error);
    }
  else
    {
      g_signal_connect (kfsb->dir_monitor, "changed",
                        G_CALLBACK (dir_changed), kfsb);
    }

  compute_checksum (kfsb->digest, NULL, 0);

  g_keyfile_settings_backend_keyfile_writable (kfsb);
  g_keyfile_settings_backend_keyfile_reload (kfsb);

  load_system_settings (kfsb);
}

static void
g_keyfile_settings_backend_set_property (xobject_t      *object,
                                         xuint_t         prop_id,
                                         const xvalue_t *value,
                                         xparam_spec_t   *pspec)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (object);

  switch ((GKeyfileSettingsBackendProperty)prop_id)
    {
    case PROP_FILENAME:
      /* Construct only. */
      g_assert (kfsb->file == NULL);
      if (xvalue_get_string (value))
        kfsb->file = xfile_new_for_path (xvalue_get_string (value));
      break;

    case PROP_ROOT_PATH:
      /* Construct only. */
      g_assert (kfsb->prefix == NULL);
      kfsb->prefix = xvalue_dup_string (value);
      if (kfsb->prefix)
        kfsb->prefix_len = strlen (kfsb->prefix);
      break;

    case PROP_ROOT_GROUP:
      /* Construct only. */
      g_assert (kfsb->root_group == NULL);
      kfsb->root_group = xvalue_dup_string (value);
      if (kfsb->root_group)
        kfsb->root_group_len = strlen (kfsb->root_group);
      break;

    case PROP_DEFAULTS_DIR:
      /* Construct only. */
      g_assert (kfsb->defaults_dir == NULL);
      kfsb->defaults_dir = xvalue_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_keyfile_settings_backend_get_property (xobject_t    *object,
                                         xuint_t       prop_id,
                                         xvalue_t     *value,
                                         xparam_spec_t *pspec)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (object);

  switch ((GKeyfileSettingsBackendProperty)prop_id)
    {
    case PROP_FILENAME:
      xvalue_set_string (value, xfile_peek_path (kfsb->file));
      break;

    case PROP_ROOT_PATH:
      xvalue_set_string (value, kfsb->prefix);
      break;

    case PROP_ROOT_GROUP:
      xvalue_set_string (value, kfsb->root_group);
      break;

    case PROP_DEFAULTS_DIR:
      xvalue_set_string (value, kfsb->defaults_dir);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_keyfile_settings_backend_class_init (GKeyfileSettingsBackendClass *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = g_keyfile_settings_backend_finalize;
  object_class->constructed = g_keyfile_settings_backend_constructed;
  object_class->get_property = g_keyfile_settings_backend_get_property;
  object_class->set_property = g_keyfile_settings_backend_set_property;

  class->read = g_keyfile_settings_backend_read;
  class->write = g_keyfile_settings_backend_write;
  class->write_tree = g_keyfile_settings_backend_write_tree;
  class->reset = g_keyfile_settings_backend_reset;
  class->get_writable = g_keyfile_settings_backend_get_writable;
  class->get_permission = g_keyfile_settings_backend_get_permission;
  /* No need to implement subscribed/unsubscribe: the only point would be to
   * stop monitoring the file when there's no xsettings_t anymore, which is no
   * big win.
   */

  /**
   * GKeyfileSettingsBackend:filename:
   *
   * The location where the settings are stored on disk.
   *
   * Defaults to `$XDG_CONFIG_HOME/glib-2.0/settings/keyfile`.
   */
  xobject_class_install_property (object_class,
                                   PROP_FILENAME,
                                   g_param_spec_string ("filename",
                                                        P_("Filename"),
                                                        P_("The filename"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * GKeyfileSettingsBackend:root-path:
   *
   * All settings read to or written from the backend must fall under the
   * path given in @root_path (which must start and end with a slash and
   * not contain two consecutive slashes).  @root_path may be "/".
   *
   * Defaults to "/".
   */
  xobject_class_install_property (object_class,
                                   PROP_ROOT_PATH,
                                   g_param_spec_string ("root-path",
                                                        P_("Root path"),
                                                        P_("The root path"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * GKeyfileSettingsBackend:root-group:
   *
   * If @root_group is non-%NULL then it specifies the name of the keyfile
   * group used for keys that are written directly below the root path.
   *
   * Defaults to NULL.
   */
  xobject_class_install_property (object_class,
                                   PROP_ROOT_GROUP,
                                   g_param_spec_string ("root-group",
                                                        P_("Root group"),
                                                        P_("The root group"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * GKeyfileSettingsBackend:default-dir:
   *
   * The directory where the system defaults and locks are located.
   *
   * Defaults to `/etc/glib-2.0/settings`.
   */
  xobject_class_install_property (object_class,
                                   PROP_DEFAULTS_DIR,
                                   g_param_spec_string ("defaults-dir",
                                                        P_("Default dir"),
                                                        P_("Defaults dir"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}

/**
 * g_keyfile_settings_backend_new:
 * @filename: the filename of the keyfile
 * @root_path: the path under which all settings keys appear
 * @root_group: (nullable): the group name corresponding to
 *              @root_path, or %NULL
 *
 * Creates a keyfile-backed #xsettings_backend_t.
 *
 * The filename of the keyfile to use is given by @filename.
 *
 * All settings read to or written from the backend must fall under the
 * path given in @root_path (which must start and end with a slash and
 * not contain two consecutive slashes).  @root_path may be "/".
 *
 * If @root_group is non-%NULL then it specifies the name of the keyfile
 * group used for keys that are written directly below @root_path.  For
 * example, if @root_path is "/apps/example/" and @root_group is
 * "toplevel", then settings the key "/apps/example/enabled" to a value
 * of %TRUE will cause the following to appear in the keyfile:
 *
 * |[
 *   [toplevel]
 *   enabled=true
 * ]|
 *
 * If @root_group is %NULL then it is not permitted to store keys
 * directly below the @root_path.
 *
 * For keys not stored directly below @root_path (ie: in a sub-path),
 * the name of the subpath (with the final slash stripped) is used as
 * the name of the keyfile group.  To continue the example, if
 * "/apps/example/profiles/default/font-size" were set to
 * 12 then the following would appear in the keyfile:
 *
 * |[
 *   [profiles/default]
 *   font-size=12
 * ]|
 *
 * The backend will refuse writes (and return writability as being
 * %FALSE) for keys outside of @root_path and, in the event that
 * @root_group is %NULL, also for keys directly under @root_path.
 * Writes will also be refused if the backend detects that it has the
 * inability to rewrite the keyfile (ie: the containing directory is not
 * writable).
 *
 * There is no checking done for your key namespace clashing with the
 * syntax of the key file format.  For example, if you have '[' or ']'
 * characters in your path names or '=' in your key names you may be in
 * trouble.
 *
 * The backend reads default values from a keyfile called `defaults` in
 * the directory specified by the #GKeyfileSettingsBackend:defaults-dir property,
 * and a list of locked keys from a text file with the name `locks` in
 * the same location.
 *
 * Returns: (transfer full): a keyfile-backed #xsettings_backend_t
 **/
xsettings_backend_t *
g_keyfile_settings_backend_new (const xchar_t *filename,
                                const xchar_t *root_path,
                                const xchar_t *root_group)
{
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (root_path != NULL, NULL);
  g_return_val_if_fail (xstr_has_prefix (root_path, "/"), NULL);
  g_return_val_if_fail (xstr_has_suffix (root_path, "/"), NULL);
  g_return_val_if_fail (strstr (root_path, "//") == NULL, NULL);

  return G_SETTINGS_BACKEND (xobject_new (XTYPE_KEYFILE_SETTINGS_BACKEND,
                                           "filename", filename,
                                           "root-path", root_path,
                                           "root-group", root_group,
                                           NULL));
}
