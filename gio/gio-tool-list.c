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

#include "gio-tool.h"


static char *attributes = NULL;
static xboolean_t show_hidden = FALSE;
static xboolean_t show_long = FALSE;
static xboolean_t nofollow_symlinks = FALSE;
static xboolean_t print_display_names = FALSE;
static xboolean_t print_uris = FALSE;

static const GOptionEntry entries[] = {
  { "attributes", 'a', 0, G_OPTION_ARG_STRING, &attributes, N_("The attributes to get"), N_("ATTRIBUTES") },
  { "hidden", 'h', 0, G_OPTION_ARG_NONE, &show_hidden, N_("Show hidden files"), NULL },
  { "long", 'l', 0, G_OPTION_ARG_NONE, &show_long, N_("Use a long listing format"), NULL },
  { "nofollow-symlinks", 'n', 0, G_OPTION_ARG_NONE, &nofollow_symlinks, N_("Don’t follow symbolic links"), NULL},
  { "print-display-names", 'd', 0, G_OPTION_ARG_NONE, &print_display_names, N_("Print display names"), NULL },
  { "print-uris", 'u', 0, G_OPTION_ARG_NONE, &print_uris, N_("Print full URIs"), NULL},
  G_OPTION_ENTRY_NULL
};

static void
show_file_listing (xfile_info_t *info, xfile_t *parent)
{
  const char *name, *type;
  char *uri = NULL;
  xoffset_t size;
  char **attributes;
  int i;
  xboolean_t first_attr;
  xfile_t *child;

  if ((xfile_info_get_is_hidden (info)) && !show_hidden)
    return;

  if (print_display_names)
    name = xfile_info_get_display_name (info);
  else
    name = xfile_info_get_name (info);

  if (name == NULL)
    name = "";

  if (print_uris) {
    child = xfile_get_child (parent, name);
    uri = xfile_get_uri (child);
    xobject_unref (child);
  }

  size = xfile_info_get_size (info);
  type = file_type_to_string (xfile_info_get_file_type (info));
  if (show_long)
    g_print ("%s\t%"G_GUINT64_FORMAT"\t(%s)", print_uris? uri: name, (xuint64_t)size, type);
  else
    g_print ("%s", print_uris? uri: name);

  if (print_uris)
    g_free (uri);

  first_attr = TRUE;
  attributes = xfile_info_list_attributes (info, NULL);
  for (i = 0 ; attributes[i] != NULL; i++)
    {
      char *val_as_string;

      if (!show_long ||
          (!print_display_names && strcmp (attributes[i], XFILE_ATTRIBUTE_STANDARD_NAME) == 0) ||
          (print_display_names && strcmp (attributes[i], XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME) == 0) ||
          strcmp (attributes[i], XFILE_ATTRIBUTE_STANDARD_SIZE) == 0 ||
          strcmp (attributes[i], XFILE_ATTRIBUTE_STANDARD_TYPE) == 0 ||
          strcmp (attributes[i], XFILE_ATTRIBUTE_STANDARD_IS_HIDDEN) == 0)
        continue;

      if (first_attr)
        {
          g_print ("\t");
          first_attr = FALSE;
        }
      else
        g_print (" ");
      val_as_string = xfile_info_get_attribute_as_string (info, attributes[i]);
      g_print ("%s=%s", attributes[i], val_as_string);
      g_free (val_as_string);
    }

  xstrfreev (attributes);

  g_print ("\n");
}

static xboolean_t
list (xfile_t *file)
{
  xfile_enumerator_t *enumerator;
  xfile_info_t *info;
  xerror_t *error;
  xboolean_t res;

  error = NULL;
  enumerator = xfile_enumerate_children (file,
                                          attributes,
                                          nofollow_symlinks ? XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS : 0,
                                          NULL,
                                          &error);
  if (enumerator == NULL)
    {
      print_file_error (file, error->message);
      xerror_free (error);
      return FALSE;
    }

  res = TRUE;
  while ((info = xfile_enumerator_next_file (enumerator, NULL, &error)) != NULL)
    {
      show_file_listing (info, file);
      xobject_unref (info);
    }

  if (error)
    {
      print_file_error (file, error->message);
      g_clear_error (&error);
      res = FALSE;
    }

  if (!xfile_enumerator_close (enumerator, NULL, &error))
    {
      print_file_error (file, error->message);
      g_clear_error (&error);
      res = FALSE;
    }

  return res;
}

int
handle_list (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xchar_t *param;
  xerror_t *error = NULL;
  xboolean_t res;
  xint_t i;
  xfile_t *file;

  g_set_prgname ("gio list");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("[%s…]", _("LOCATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("List the contents of the locations."));
  g_option_context_set_description (context,
      _("gio list is similar to the traditional ls utility, but using GIO\n"
        "locations instead of local files: for example, you can use something\n"
        "like smb://server/resource/file.txt as location. File attributes can\n"
        "be specified with their GIO name, e.g. standard::icon"));
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

  g_option_context_free (context);

  if (attributes != NULL)
    show_long = TRUE;

  attributes = xstrconcat (!print_display_names ? XFILE_ATTRIBUTE_STANDARD_NAME "," : "",
                            print_display_names ? XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," : "",
                            XFILE_ATTRIBUTE_STANDARD_TYPE ","
                            XFILE_ATTRIBUTE_STANDARD_SIZE ","
                            XFILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                            attributes != NULL ? "," : "",
                            attributes,
                            NULL);

  res = TRUE;
  if (argc > 1)
    {
      for (i = 1; i < argc; i++)
        {
          file = xfile_new_for_commandline_arg (argv[i]);
          res &= list (file);
          xobject_unref (file);
        }
    }
  else
    {
      char *cwd;

      cwd = g_get_current_dir ();
      file = xfile_new_for_path (cwd);
      res = list (file);
      xobject_unref (file);
      g_free (cwd);
    }

  g_free (attributes);

  return res ? 0 : 2;
}
