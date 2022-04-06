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
#include <stdio.h>
#if 0
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#endif

#include "gio-tool.h"

static xboolean_t no_target_directory = FALSE;
static xboolean_t progress = FALSE;
static xboolean_t interactive = FALSE;
static xboolean_t preserve = FALSE;
static xboolean_t backup = FALSE;
static xboolean_t no_dereference = FALSE;
static xboolean_t default_permissions = FALSE;

static const GOptionEntry entries[] = {
  { "no-target-directory", 'T', 0, G_OPTION_ARG_NONE, &no_target_directory, N_("No target directory"), NULL },
  { "progress", 'p', 0, G_OPTION_ARG_NONE, &progress, N_("Show progress"), NULL },
  { "interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive, N_("Prompt before overwrite"), NULL },
  { "preserve", 'p', 0, G_OPTION_ARG_NONE, &preserve, N_("Preserve all attributes"), NULL },
  { "backup", 'b', 0, G_OPTION_ARG_NONE, &backup, N_("Backup existing destination files"), NULL },
  { "no-dereference", 'P', 0, G_OPTION_ARG_NONE, &no_dereference, N_("Never follow symbolic links"), NULL },
  { "default-permissions", 0, 0, G_OPTION_ARG_NONE, &default_permissions, N_("Use default permissions for the destination"), NULL },
  G_OPTION_ENTRY_NULL
};

static sint64_t start_time;
static sint64_t previous_time;

static void
show_progress (xoffset_t current_num_bytes,
               xoffset_t total_num_bytes,
               xpointer_t user_data)
{
  sint64_t tv;
  char *current_size, *total_size, *rate;

  tv = g_get_monotonic_time ();
  if (tv - previous_time < (G_USEC_PER_SEC / 5) &&
      current_num_bytes != total_num_bytes)
    return;

  current_size = g_format_size (current_num_bytes);
  total_size = g_format_size (total_num_bytes);
  rate = g_format_size (current_num_bytes /
                        MAX ((tv - start_time) / G_USEC_PER_SEC, 1));
  g_print ("\r\033[K");
  g_print (_("Transferred %s out of %s (%s/s)"), current_size, total_size, rate);

  previous_time = tv;

  g_free (current_size);
  g_free (total_size);
  g_free (rate);
}

int
handle_copy (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xerror_t *error = NULL;
  char *param;
  xfile_t *source, *dest, *target;
  xboolean_t dest_is_dir;
  char *basename;
  char *uri;
  int i;
  xfile_copy_flags_t flags;
  int retval = 0;

  g_set_prgname ("gio copy");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("%s… %s", _("SOURCE"), _("DESTINATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("Copy one or more files from SOURCE to DESTINATION."));
  g_option_context_set_description (context,
      _("gio copy is similar to the traditional cp utility, but using GIO\n"
        "locations instead of local files: for example, you can use something\n"
        "like smb://server/resource/file.txt as location."));
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
      show_help (context, NULL);
      g_option_context_free (context);
      return 1;
    }

  dest = xfile_new_for_commandline_arg (argv[argc - 1]);

  if (no_target_directory && argc > 3)
    {
      show_help (context, NULL);
      xobject_unref (dest);
      g_option_context_free (context);
      return 1;
    }

  dest_is_dir = file_is_dir (dest);
  if (!dest_is_dir && argc > 3)
    {
      char *message;

      message = xstrdup_printf (_("Destination %s is not a directory"), argv[argc - 1]);
      show_help (context, message);
      g_free (message);
      xobject_unref (dest);
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  for (i = 1; i < argc - 1; i++)
    {
      source = xfile_new_for_commandline_arg (argv[i]);
      if (dest_is_dir && !no_target_directory)
        {
          basename = xfile_get_basename (source);
          target = xfile_get_child (dest, basename);
          g_free (basename);
        }
      else
        target = xobject_ref (dest);

      flags = 0;
      if (backup)
        flags |= XFILE_COPY_BACKUP;
      if (!interactive)
        flags |= XFILE_COPY_OVERWRITE;
      if (no_dereference)
        flags |= XFILE_COPY_NOFOLLOW_SYMLINKS;
      if (preserve)
        flags |= XFILE_COPY_ALL_METADATA;
      if (default_permissions)
        flags |= XFILE_COPY_TARGET_DEFAULT_PERMS;

      error = NULL;
      start_time = g_get_monotonic_time ();

      if (!xfile_copy (source, target, flags, NULL, progress ? show_progress : NULL, NULL, &error))
        {
          if (interactive && xerror_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
            {
              char line[16];

              xerror_free (error);
              error = NULL;

              uri = xfile_get_uri (target);
              g_print (_("%s: overwrite “%s”? "), argv[0], uri);
              g_free (uri);

              if (fgets (line, sizeof (line), stdin) &&
                  (line[0] == 'y' || line[0] == 'Y'))
                {
                  flags |= XFILE_COPY_OVERWRITE;
                  start_time = g_get_monotonic_time ();
                  if (!xfile_copy (source, target, flags, NULL, progress ? show_progress : NULL, NULL, &error))
                    goto copy_failed;
                }
            }
          else
            {
            copy_failed:
              print_file_error (source, error->message);
              xerror_free (error);
              retval = 1;
            }
        }

     if (progress && retval == 0)
        g_print ("\n");

      xobject_unref (source);
      xobject_unref (target);
    }

  xobject_unref (dest);

  return retval;
}
