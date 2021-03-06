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


static xboolean_t force = FALSE;
static xboolean_t empty = FALSE;
static xboolean_t restore = FALSE;
static xboolean_t list = FALSE;
static const GOptionEntry entries[] = {
  { "force", 'f', 0, G_OPTION_ARG_NONE, &force, N_("Ignore nonexistent files, never prompt"), NULL },
  { "empty", 0, 0, G_OPTION_ARG_NONE, &empty, N_("Empty the trash"), NULL },
  { "list", 0, 0, G_OPTION_ARG_NONE, &list, N_("List files in the trash with their original locations"), NULL },
  { "restore", 0, 0, G_OPTION_ARG_NONE, &restore, N_("Restore a file from trash to its original location (possibly "
                                                     "recreating the directory)"), NULL },
  G_OPTION_ENTRY_NULL
};

static void
delete_trash_file (xfile_t *file, xboolean_t del_file, xboolean_t del_children)
{
  xfile_info_t *info;
  xfile_t *child;
  xfile_enumerator_t *enumerator;

  g_return_if_fail (xfile_has_uri_scheme (file, "trash"));

  if (del_children)
    {
      enumerator = xfile_enumerate_children (file,
                                              XFILE_ATTRIBUTE_STANDARD_NAME ","
                                              XFILE_ATTRIBUTE_STANDARD_TYPE,
                                              XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                              NULL,
                                              NULL);
      if (enumerator)
        {
          while ((info = xfile_enumerator_next_file (enumerator, NULL, NULL)) != NULL)
            {
              child = xfile_get_child (file, xfile_info_get_name (info));

              /* The xfile_delete operation works differently for locations
               * provided by the trash backend as it prevents modifications of
               * trashed items. For that reason, it is enough to call
               * xfile_delete on top-level items only.
               */
              delete_trash_file (child, TRUE, FALSE);

              xobject_unref (child);
              xobject_unref (info);
            }
          xfile_enumerator_close (enumerator, NULL, NULL);
          xobject_unref (enumerator);
        }
    }

  if (del_file)
    xfile_delete (file, NULL, NULL);
}

static xboolean_t
restore_trash (xfile_t         *file,
               xboolean_t       force,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xfile_info_t *info = NULL;
  xfile_t *target = NULL;
  xfile_t *dir_target = NULL;
  xboolean_t ret = FALSE;
  xchar_t *orig_path = NULL;
  xerror_t *local_error = NULL;

  info = xfile_query_info (file, XFILE_ATTRIBUTE_TRASH_ORIG_PATH, XFILE_QUERY_INFO_NONE, cancellable, &local_error);
  if (local_error)
    {
      g_propagate_error (error, local_error);
      goto exit_func;
    }

  orig_path = xfile_info_get_attribute_as_string (info, XFILE_ATTRIBUTE_TRASH_ORIG_PATH);
  if (!orig_path)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, _("Unable to find original path"));
      goto exit_func;
    }

  target = xfile_new_for_commandline_arg (orig_path);
  g_free (orig_path);

  dir_target = xfile_get_parent (target);
  if (dir_target)
    {
      xfile_make_directory_with_parents (dir_target, cancellable, &local_error);
      if (xerror_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
        {
          g_clear_error (&local_error);
        }
      else if (local_error != NULL)
        {
          g_propagate_prefixed_error (error, local_error, _("Unable to recreate original location: "));
          goto exit_func;
        }
    }

  if (!xfile_move (file,
                    target,
                    force ? XFILE_COPY_OVERWRITE : XFILE_COPY_NONE,
                    cancellable,
                    NULL,
                    NULL,
                    &local_error))
    {
      g_propagate_prefixed_error (error, local_error, _("Unable to move file to its original location: "));
      goto exit_func;
    }
  ret = TRUE;

exit_func:
  g_clear_object (&target);
  g_clear_object (&dir_target);
  g_clear_object (&info);
  return ret;
}

static xboolean_t
trash_list (xfile_t         *file,
            xcancellable_t  *cancellable,
            xerror_t       **error)
{
  xfile_enumerator_t *enumerator;
  xfile_info_t *info;
  xerror_t *local_error = NULL;
  xboolean_t res;

  enumerator = xfile_enumerate_children (file,
                                          XFILE_ATTRIBUTE_STANDARD_NAME ","
                                          XFILE_ATTRIBUTE_TRASH_ORIG_PATH,
                                          XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          cancellable,
                                          &local_error);
  if (!enumerator)
    {
      g_propagate_error (error, local_error);
      return FALSE;
    }

  res = TRUE;
  while ((info = xfile_enumerator_next_file (enumerator, cancellable, &local_error)) != NULL)
    {
      const char *name;
      char *orig_path;
      char *uri;
      xfile_t* child;

      name = xfile_info_get_name (info);
      child = xfile_get_child (file, name);
      uri = xfile_get_uri (child);
      xobject_unref (child);
      orig_path = xfile_info_get_attribute_as_string (info, XFILE_ATTRIBUTE_TRASH_ORIG_PATH);

      g_print ("%s\t%s\n", uri, orig_path);

      xobject_unref (info);
      g_free (orig_path);
      g_free (uri);
    }

  if (local_error)
    {
      g_propagate_error (error, local_error);
      local_error = NULL;
      res = FALSE;
    }

  if (!xfile_enumerator_close (enumerator, cancellable, &local_error))
    {
      print_file_error (file, local_error->message);
      g_clear_error (&local_error);
      res = FALSE;
    }

  return res;
}

int
handle_trash (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xchar_t *param;
  xerror_t *error = NULL;
  int retval = 0;
  xfile_t *file;

  g_set_prgname ("gio trash");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("[%s???]", _("LOCATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("Move/Restore files or directories to the trash."));
  g_option_context_set_description (context,
      _("Note: for --restore switch, if the original location of the trashed file \n"
        "already exists, it will not be overwritten unless --force is set."));
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

  if (argc > 1)
    {
      int i;

      for (i = 1; i < argc; i++)
        {
          file = xfile_new_for_commandline_arg (argv[i]);
          error = NULL;
          if (restore)
            {
              if (!xfile_has_uri_scheme (file, "trash"))
                {
                  print_file_error (file, _("Location given doesn't start with trash:///"));
                  retval = 1;
                }
              else if (!restore_trash (file, force, NULL, &error))
                {
                  print_file_error (file, error->message);
                  retval = 1;
                }
            }
          else if (!xfile_trash (file, NULL, &error))
            {
              if (!force ||
                  !xerror_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
                {
                  print_file_error (file, error->message);
                  retval = 1;
                }
            }
          g_clear_error (&error);
          xobject_unref (file);
        }
    }
  else if (list)
    {
      xfile_t *file;
      file = xfile_new_for_uri ("trash:");
      trash_list (file, NULL, &error);
      if (error)
        {
          print_file_error (file, error->message);
          g_clear_error (&error);
          retval = 1;
        }
      xobject_unref (file);
    }
  else if (empty)
    {
      xfile_t *file;
      file = xfile_new_for_uri ("trash:");
      delete_trash_file (file, FALSE, TRUE);
      xobject_unref (file);
    }

  if (argc == 1 && !empty && !list)
    {
      show_help (context, _("No locations given"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  return retval;
}
