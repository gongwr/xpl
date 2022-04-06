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


static const GOptionEntry entries[] = {
  G_OPTION_ENTRY_NULL
};

int
handle_rename (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xerror_t *error = NULL;
  xfile_t *file;
  xfile_t *new_file;
  int retval = 0;
  xchar_t *param;

  g_set_prgname ("gio rename");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("%s %s", _("LOCATION"), _("NAME"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);

  g_option_context_set_summary (context, _("Rename a file."));
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

  if (argc < 3)
    {
      show_help (context, _("Missing argument"));
      g_option_context_free (context);
      return 1;
    }
  if (argc > 3)
    {
      show_help (context, _("Too many arguments"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  file = xfile_new_for_commandline_arg (argv[1]);
  new_file = xfile_set_display_name (file, argv[2], NULL, &error);

  if (new_file == NULL)
    {
      print_error ("%s", error->message);
      xerror_free (error);
      retval = 1;
    }
  else
    {
      char *uri = xfile_get_uri (new_file);
      g_print (_("Rename successful. New uri: %s\n"), uri);
      xobject_unref (new_file);
      g_free (uri);
    }

  xobject_unref (file);

  return retval;
}
