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

static xchar_t **watch_dirs;
static xchar_t **watch_files;
static xchar_t **watch_direct;
static xchar_t **watch_silent;
static xchar_t **watch_default;
static xboolean_t no_moves;
static xboolean_t mounts;

static const GOptionEntry entries[] = {
  { "dir", 'd', 0, G_OPTION_ARXFILENAME_ARRAY, &watch_dirs,
      N_("Monitor a directory (default: depends on type)"), N_("LOCATION") },
  { "file", 'f', 0, G_OPTION_ARXFILENAME_ARRAY, &watch_files,
      N_("Monitor a file (default: depends on type)"), N_("LOCATION") },
  { "direct", 'D', 0, G_OPTION_ARXFILENAME_ARRAY, &watch_direct,
      N_("Monitor a file directly (notices changes made via hardlinks)"), N_("LOCATION") },
  { "silent", 's', 0, G_OPTION_ARXFILENAME_ARRAY, &watch_silent,
      N_("Monitors a file directly, but doesn’t report changes"), N_("LOCATION") },
  { "no-moves", 'n', 0, G_OPTION_ARG_NONE, &no_moves,
      N_("Report moves and renames as simple deleted/created events"), NULL },
  { "mounts", 'm', 0, G_OPTION_ARG_NONE, &mounts,
      N_("Watch for mount events"), NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARXFILENAME_ARRAY, &watch_default,
      NULL, NULL },
  G_OPTION_ENTRY_NULL
};

static void
watch_callback (xfile_monitor_t      *monitor,
                xfile_t             *child,
                xfile_t             *other,
                xfile_monitor_event_t  event_type,
                xpointer_t           user_data)
{
  xchar_t *child_str;
  xchar_t *other_str;

  g_assert (child);

  if (xfile_is_native (child))
    child_str = xfile_get_path (child);
  else
    child_str = xfile_get_uri (child);

  if (other)
    {
      if (xfile_is_native (other))
        other_str = xfile_get_path (other);
      else
        other_str = xfile_get_uri (other);
    }
  else
    other_str = xstrdup ("(none)");

  g_print ("%s: ", (xchar_t *) user_data);
  switch (event_type)
    {
    case XFILE_MONITOR_EVENT_CHANGED:
      g_assert (!other);
      g_print ("%s: changed", child_str);
      break;
    case XFILE_MONITOR_EVENT_CHANGES_DONE_HINT:
      g_assert (!other);
      g_print ("%s: changes done", child_str);
      break;
    case XFILE_MONITOR_EVENT_DELETED:
      g_assert (!other);
      g_print ("%s: deleted", child_str);
      break;
    case XFILE_MONITOR_EVENT_CREATED:
      g_assert (!other);
      g_print ("%s: created", child_str);
      break;
    case XFILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
      g_assert (!other);
      g_print ("%s: attributes changed", child_str);
      break;
    case XFILE_MONITOR_EVENT_PRE_UNMOUNT:
      g_assert (!other);
      g_print ("%s: pre-unmount", child_str);
      break;
    case XFILE_MONITOR_EVENT_UNMOUNTED:
      g_assert (!other);
      g_print ("%s: unmounted", child_str);
      break;
    case XFILE_MONITOR_EVENT_MOVED_IN:
      g_print ("%s: moved in", child_str);
      if (other)
        g_print (" (from %s)", other_str);
      break;
    case XFILE_MONITOR_EVENT_MOVED_OUT:
      g_print ("%s: moved out", child_str);
      if (other)
        g_print (" (to %s)", other_str);
      break;
    case XFILE_MONITOR_EVENT_RENAMED:
      g_assert (other);
      g_print ("%s: renamed to %s\n", child_str, other_str);
      break;

    case XFILE_MONITOR_EVENT_MOVED:
    default:
      g_assert_not_reached ();
    }

  g_free (child_str);
  g_free (other_str);
  g_print ("\n");
}

typedef enum
{
  WATCH_DIR,
  WATCH_FILE,
  WATCH_AUTO
} WatchType;

static xboolean_t
add_watch (const xchar_t       *cmdline,
           WatchType          watch_type,
           xfile_monitor_flags_t  flags,
           xboolean_t           connect_handler)
{
  xfile_monitor_t *monitor = NULL;
  xerror_t *error = NULL;
  xfile_t *file;

  file = xfile_new_for_commandline_arg (cmdline);

  if (watch_type == WATCH_AUTO)
    {
      xfile_info_t *info;
      xuint32_t type;

      info = xfile_query_info (file, XFILE_ATTRIBUTE_STANDARD_TYPE, XFILE_QUERY_INFO_NONE, NULL, &error);
      if (!info)
        goto err;

      type = xfile_info_get_attribute_uint32 (info, XFILE_ATTRIBUTE_STANDARD_TYPE);
      watch_type = (type == XFILE_TYPE_DIRECTORY) ? WATCH_DIR : WATCH_FILE;
    }

  if (watch_type == WATCH_DIR)
    monitor = xfile_monitor_directory (file, flags, NULL, &error);
  else
    monitor = xfile_monitor (file, flags, NULL, &error);

  if (!monitor)
    goto err;

  if (connect_handler)
    xsignal_connect (monitor, "changed", G_CALLBACK (watch_callback), xstrdup (cmdline));

  monitor = NULL; /* leak */
  xobject_unref (file);

  return TRUE;

err:
  print_file_error (file, error->message);
  xerror_free (error);
  xobject_unref (file);

  return FALSE;
}

int
handle_monitor (int argc, xchar_t *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xchar_t *param;
  xerror_t *error = NULL;
  xfile_monitor_flags_t flags;
  xuint_t i;

  g_set_prgname ("gio monitor");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("%s…", _("LOCATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
    _("Monitor files or directories for changes."));
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

  if (!watch_dirs && !watch_files && !watch_direct && !watch_silent && !watch_default)
    {
      show_help (context, _("No locations given"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  flags = (no_moves ? 0 : XFILE_MONITOR_WATCH_MOVES) |
          (mounts ? XFILE_MONITOR_WATCH_MOUNTS : 0);

  if (watch_dirs)
    {
      for (i = 0; watch_dirs[i]; i++)
        if (!add_watch (watch_dirs[i], WATCH_DIR, flags, TRUE))
          return 1;
    }

  if (watch_files)
    {
      for (i = 0; watch_files[i]; i++)
        if (!add_watch (watch_files[i], WATCH_FILE, flags, TRUE))
          return 1;
    }

  if (watch_direct)
    {
      for (i = 0; watch_direct[i]; i++)
        if (!add_watch (watch_direct[i], WATCH_FILE, flags | XFILE_MONITOR_WATCH_HARD_LINKS, TRUE))
          return 1;
    }

  if (watch_silent)
    {
      for (i = 0; watch_silent[i]; i++)
        if (!add_watch (watch_silent[i], WATCH_FILE, flags | XFILE_MONITOR_WATCH_HARD_LINKS, FALSE))
          return 1;
    }

  if (watch_default)
    {
      for (i = 0; watch_default[i]; i++)
        if (!add_watch (watch_default[i], WATCH_AUTO, flags, TRUE))
          return 1;
    }

  while (TRUE)
    xmain_context_iteration (NULL, TRUE);

  return 0;
}
