/*
 * Copyright 2015 Red Hat, Inc.
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
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include <gio/gio.h>
#include <gi18n.h>

#ifdef G_OS_UNIX
#include <gio/gunixmounts.h>
#endif

#include "gio-tool.h"

static xboolean_t writable = FALSE;
static xboolean_t filesystem = FALSE;
static char *attributes = NULL;
static xboolean_t nofollow_symlinks = FALSE;

static const GOptionEntry entries[] = {
  { "query-writable", 'w', 0, G_OPTION_ARG_NONE, &writable, N_("List writable attributes"), NULL },
  { "filesystem", 'f', 0, G_OPTION_ARG_NONE, &filesystem, N_("Get file system info"), NULL },
  { "attributes", 'a', 0, G_OPTION_ARG_STRING, &attributes, N_("The attributes to get"), N_("ATTRIBUTES") },
  { "nofollow-symlinks", 'n', 0, G_OPTION_ARG_NONE, &nofollow_symlinks, N_("Don’t follow symbolic links"), NULL },
  G_OPTION_ENTRY_NULL
};

static char *
escape_string (const char *in)
{
  xstring_t *str;
  static char *hex_digits = "0123456789abcdef";
  unsigned char c;


  str = xstring_new ("");

  while ((c = *in++) != 0)
    {
      if (c >= 32 && c <= 126 && c != '\\')
        xstring_append_c (str, c);
      else
        {
          xstring_append (str, "\\x");
          xstring_append_c (str, hex_digits[(c >> 4) & 0xf]);
          xstring_append_c (str, hex_digits[c & 0xf]);
        }
    }

  return xstring_free (str, FALSE);
}

static void
show_attributes (xfile_info_t *info)
{
  char **attributes;
  char *s;
  int i;

  attributes = xfile_info_list_attributes (info, NULL);

  g_print (_("attributes:\n"));
  for (i = 0; attributes[i] != NULL; i++)
    {
      /* list the icons in order rather than displaying "xthemed_icon_t:0x8df7200" */
      if (strcmp (attributes[i], "standard::icon") == 0 ||
          strcmp (attributes[i], "standard::symbolic-icon") == 0)
        {
          xicon_t *icon;
          int j;
          const char * const *names = NULL;

          if (strcmp (attributes[i], "standard::symbolic-icon") == 0)
            icon = xfile_info_get_symbolic_icon (info);
          else
            icon = xfile_info_get_icon (info);

          /* only look up names if xthemed_icon_t */
          if (X_IS_THEMED_ICON(icon))
            {
              names = g_themed_icon_get_names (G_THEMED_ICON (icon));
              g_print ("  %s: ", attributes[i]);
              for (j = 0; names[j] != NULL; j++)
                g_print ("%s%s", names[j], (names[j+1] == NULL)?"":", ");
              g_print ("\n");
            }
          else
            {
              s = xfile_info_get_attribute_as_string (info, attributes[i]);
              g_print ("  %s: %s\n", attributes[i], s);
              g_free (s);
            }
        }
      else
        {
          s = xfile_info_get_attribute_as_string (info, attributes[i]);
          g_print ("  %s: %s\n", attributes[i], s);
          g_free (s);
        }
    }
  xstrfreev (attributes);
}

static void
show_info (xfile_t *file, xfile_info_t *info)
{
  const char *name, *type;
  char *escaped, *uri;
  xoffset_t size;
  const char *path;
#ifdef G_OS_UNIX
  GUnixMountEntry *entry;
#endif

  name = xfile_info_get_display_name (info);
  if (name)
    /* Translators: This is a noun and represents and attribute of a file */
    g_print (_("display name: %s\n"), name);

  name = xfile_info_get_edit_name (info);
  if (name)
    /* Translators: This is a noun and represents and attribute of a file */
    g_print (_("edit name: %s\n"), name);

  name = xfile_info_get_name (info);
  if (name)
    {
      escaped = escape_string (name);
      g_print (_("name: %s\n"), escaped);
      g_free (escaped);
    }

  if (xfile_info_has_attribute (info, XFILE_ATTRIBUTE_STANDARD_TYPE))
    {
      type = file_type_to_string (xfile_info_get_file_type (info));
      g_print (_("type: %s\n"), type);
    }

  if (xfile_info_has_attribute (info, XFILE_ATTRIBUTE_STANDARD_SIZE))
    {
      size = xfile_info_get_size (info);
      g_print (_("size: "));
      g_print (" %"G_GUINT64_FORMAT"\n", (xuint64_t)size);
    }

  if (xfile_info_get_is_hidden (info))
    g_print (_("hidden\n"));

  uri = xfile_get_uri (file);
  g_print (_("uri: %s\n"), uri);
  g_free (uri);

  path = xfile_peek_path (file);
  if (path)
    {
      g_print (_("local path: %s\n"), path);

#ifdef G_OS_UNIX
      entry = g_unix_mount_at (path, NULL);
      if (entry == NULL)
        entry = g_unix_mount_for (path, NULL);
      if (entry != NULL)
        {
          xchar_t *device;
          const xchar_t *root;
          xchar_t *root_string = NULL;
          xchar_t *mount;
          xchar_t *fs;
          const xchar_t *options;
          xchar_t *options_string = NULL;

          device = xstrescape (g_unix_mount_get_device_path (entry), NULL);
          root = g_unix_mount_get_root_path (entry);
          if (root != NULL && xstrcmp0 (root, "/") != 0)
            {
              escaped = xstrescape (root, NULL);
              root_string = xstrconcat ("[", escaped, "]", NULL);
              g_free (escaped);
            }
          mount = xstrescape (g_unix_mount_get_mount_path (entry), NULL);
          fs = xstrescape (g_unix_mount_get_fs_type (entry), NULL);

          options = g_unix_mount_get_options (entry);
          if (options != NULL)
            {
              options_string = xstrescape (options, NULL);
            }

          g_print (_("unix mount: %s%s %s %s %s\n"), device,
                   root_string ? root_string : "", mount, fs,
                   options_string ? options_string : "");

          g_free (device);
          g_free (root_string);
          g_free (mount);
          g_free (fs);
          g_free (options_string);

          g_unix_mount_free (entry);
        }
#endif
    }

  show_attributes (info);
}

static xboolean_t
query_info (xfile_t *file)
{
  xfile_query_info_flags_t flags;
  xfile_info_t *info;
  xerror_t *error;

  if (file == NULL)
    return FALSE;

  if (attributes == NULL)
    attributes = "*";

  flags = 0;
  if (nofollow_symlinks)
    flags |= XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS;

  error = NULL;
  if (filesystem)
    info = xfile_query_filesystem_info (file, attributes, NULL, &error);
  else
    info = xfile_query_info (file, attributes, flags, NULL, &error);

  if (info == NULL)
    {
      print_file_error (file, error->message);
      xerror_free (error);
      return FALSE;
    }

  if (filesystem)
    show_attributes (info);
  else
    show_info (file, info);

  xobject_unref (info);

  return TRUE;
}

static xboolean_t
get_writable_info (xfile_t *file)
{
  xfile_attribute_info_list_t *list;
  xerror_t *error;
  int i;
  char *flags;

  if (file == NULL)
    return FALSE;

  error = NULL;

  list = xfile_query_settable_attributes (file, NULL, &error);
  if (list == NULL)
    {
      print_file_error (file, error->message);
      xerror_free (error);
      return FALSE;
    }

  if (list->n_infos > 0)
    {
      g_print (_("Settable attributes:\n"));
      for (i = 0; i < list->n_infos; i++)
        {
          flags = attribute_flags_to_string (list->infos[i].flags);
          g_print (" %s (%s%s%s)\n",
                   list->infos[i].name,
                   attribute_type_to_string (list->infos[i].type),
                   (*flags != 0)?", ":"", flags);
          g_free (flags);
        }
    }

  xfile_attribute_info_list_unref (list);

  list = xfile_query_writable_namespaces (file, NULL, &error);
  if (list == NULL)
    {
      print_file_error (file, error->message);
      xerror_free (error);
      return FALSE;
    }

  if (list->n_infos > 0)
    {
      g_print (_("Writable attribute namespaces:\n"));
      for (i = 0; i < list->n_infos; i++)
        {
          flags = attribute_flags_to_string (list->infos[i].flags);
          g_print (" %s (%s%s%s)\n",
                   list->infos[i].name,
                   attribute_type_to_string (list->infos[i].type),
                   (*flags != 0)?", ":"", flags);
          g_free (flags);
        }
    }

  xfile_attribute_info_list_unref (list);

  return TRUE;
}

int
handle_info (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xchar_t *param;
  xerror_t *error = NULL;
  xboolean_t res;
  xint_t i;
  xfile_t *file;

  g_set_prgname ("gio info");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("%s…", _("LOCATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("Show information about locations."));
  g_option_context_set_description (context,
      _("gio info is similar to the traditional ls utility, but using GIO\n"
        "locations instead of local files: for example, you can use something\n"
        "like smb://server/resource/file.txt as location. File attributes can\n"
        "be specified with their GIO name, e.g. standard::icon, or just by\n"
        "namespace, e.g. unix, or by “*”, which matches all attributes"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  if (do_help)
    {
      show_help (context, NULL);
      g_option_context_free (context);
      return 0;
    }

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      show_help (context, error->message);
      xerror_free (error);
      g_option_context_free (context);
      return 1;
    }

  if (argc < 2)
    {
      show_help (context, _("No locations given"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  res = TRUE;
  for (i = 1; i < argc; i++)
    {
      file = xfile_new_for_commandline_arg (argv[i]);
      if (writable)
        res &= get_writable_info (file);
      else
        res &= query_info (file);
      xobject_unref (file);
    }

  return res ? 0 : 2;
}
