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
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "gio-tool.h"
#include "glib/glib-private.h"

void
print_error (const xchar_t *format, ...)
{
  xchar_t *message;
  va_list args;

  va_start (args, format);
  message = xstrdup_vprintf (format, args);
  va_end (args);

  g_printerr ("gio: %s\n", message);
  g_free (message);
}

void
print_file_error (xfile_t *file, const xchar_t *message)
{
  xchar_t *uri;

  uri = xfile_get_uri (file);
  print_error ("%s: %s", uri, message);
  g_free (uri);
}

void
show_help (xoption_context_t *context, const char *message)
{
  char *help;

  if (message)
    g_printerr ("gio: %s\n\n", message);

  help = g_option_context_get_help (context, TRUE, NULL);
  g_printerr ("%s", help);
  g_free (help);
}

const char *
file_type_to_string (xfile_type_t type)
{
  switch (type)
    {
    case XFILE_TYPE_UNKNOWN:
      return "unknown";
    case XFILE_TYPE_REGULAR:
      return "regular";
    case XFILE_TYPE_DIRECTORY:
      return "directory";
    case XFILE_TYPE_SYMBOLIC_LINK:
      return "symlink";
    case XFILE_TYPE_SPECIAL:
      return "special";
    case XFILE_TYPE_SHORTCUT:
      return "shortcut";
    case XFILE_TYPE_MOUNTABLE:
      return "mountable";
    default:
      return "invalid type";
    }
}

const char *
attribute_type_to_string (xfile_attribute_type_t type)
{
  switch (type)
    {
    case XFILE_ATTRIBUTE_TYPE_INVALID:
      return "invalid";
    case XFILE_ATTRIBUTE_TYPE_STRING:
      return "string";
    case XFILE_ATTRIBUTE_TYPE_BYTE_STRING:
      return "bytestring";
    case XFILE_ATTRIBUTE_TYPE_BOOLEAN:
      return "boolean";
    case XFILE_ATTRIBUTE_TYPE_UINT32:
      return "uint32";
    case XFILE_ATTRIBUTE_TYPE_INT32:
      return "int32";
    case XFILE_ATTRIBUTE_TYPE_UINT64:
      return "uint64";
    case XFILE_ATTRIBUTE_TYPE_INT64:
      return "int64";
    case XFILE_ATTRIBUTE_TYPE_OBJECT:
      return "object";
    default:
      return "unknown type";
    }
}

xfile_attribute_type_t
attribute_type_from_string (const char *str)
{
  if (strcmp (str, "string") == 0)
    return XFILE_ATTRIBUTE_TYPE_STRING;
  if (strcmp (str, "stringv") == 0)
    return XFILE_ATTRIBUTE_TYPE_STRINGV;
  if (strcmp (str, "bytestring") == 0)
    return XFILE_ATTRIBUTE_TYPE_BYTE_STRING;
  if (strcmp (str, "boolean") == 0)
    return XFILE_ATTRIBUTE_TYPE_BOOLEAN;
  if (strcmp (str, "uint32") == 0)
    return XFILE_ATTRIBUTE_TYPE_UINT32;
  if (strcmp (str, "int32") == 0)
    return XFILE_ATTRIBUTE_TYPE_INT32;
  if (strcmp (str, "uint64") == 0)
    return XFILE_ATTRIBUTE_TYPE_UINT64;
  if (strcmp (str, "int64") == 0)
    return XFILE_ATTRIBUTE_TYPE_INT64;
  if (strcmp (str, "object") == 0)
    return XFILE_ATTRIBUTE_TYPE_OBJECT;
  if (strcmp (str, "unset") == 0)
    return XFILE_ATTRIBUTE_TYPE_INVALID;
  return -1;
}

char *
attribute_flags_to_string (xfile_attribute_info_flags_t flags)
{
  xstring_t *s;
  xsize_t i;
  xboolean_t first;
  struct {
    xuint32_t mask;
    char *descr;
  } flag_descr[] = {
    {
      XFILE_ATTRIBUTE_INFO_COPY_WITH_FILE,
      N_("Copy with file")
    },
    {
      XFILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED,
      N_("Keep with file when moved")
    }
  };

  first = TRUE;

  s = xstring_new ("");
  for (i = 0; i < G_N_ELEMENTS (flag_descr); i++)
    {
      if (flags & flag_descr[i].mask)
        {
          if (!first)
            xstring_append (s, ", ");
          xstring_append (s, gettext (flag_descr[i].descr));
          first = FALSE;
        }
    }

  return xstring_free (s, FALSE);
}

xboolean_t
file_is_dir (xfile_t *file)
{
  xfile_info_t *info;
  xboolean_t res;

  info = xfile_query_info (file, XFILE_ATTRIBUTE_STANDARD_TYPE, 0, NULL, NULL);
  res = info && xfile_info_get_file_type (info) == XFILE_TYPE_DIRECTORY;
  if (info)
    xobject_unref (info);
  return res;
}


static int
handle_version (int argc, char *argv[], xboolean_t do_help)
{
  if (do_help || argc > 1)
    {
      if (!do_help)
        g_printerr ("gio: %s\n\n", _("???version??? takes no arguments"));

      g_printerr ("%s\n", _("Usage:"));
      g_printerr ("  gio version\n");
      g_printerr ("\n");
      g_printerr ("%s\n", _("Print version information and exit."));

      return do_help ? 0 : 2;
    }

  g_print ("%d.%d.%d\n", glib_major_version, glib_minor_version, glib_micro_version);

  return 0;
}

static void
usage (void)
{
  g_printerr ("%s\n", _("Usage:"));
  g_printerr ("  gio %s %s\n", _("COMMAND"), _("[ARGS???]"));
  g_printerr ("\n");
  g_printerr ("%s\n", _("Commands:"));
  g_printerr ("  help     %s\n", _("Print help"));
  g_printerr ("  version  %s\n", _("Print version"));
  g_printerr ("  cat      %s\n", _("Concatenate files to standard output"));
  g_printerr ("  copy     %s\n", _("Copy one or more files"));
  g_printerr ("  info     %s\n", _("Show information about locations"));
  g_printerr ("  launch   %s\n", _("Launch an application from a desktop file"));
  g_printerr ("  list     %s\n", _("List the contents of locations"));
  g_printerr ("  mime     %s\n", _("Get or set the handler for a mimetype"));
  g_printerr ("  mkdir    %s\n", _("Create directories"));
  g_printerr ("  monitor  %s\n", _("Monitor files and directories for changes"));
  g_printerr ("  mount    %s\n", _("Mount or unmount the locations"));
  g_printerr ("  move     %s\n", _("Move one or more files"));
  g_printerr ("  open     %s\n", _("Open files with the default application"));
  g_printerr ("  rename   %s\n", _("Rename a file"));
  g_printerr ("  remove   %s\n", _("Delete one or more files"));
  g_printerr ("  save     %s\n", _("Read from standard input and save"));
  g_printerr ("  set      %s\n", _("Set a file attribute"));
  g_printerr ("  trash    %s\n", _("Move files or directories to the trash"));
  g_printerr ("  tree     %s\n", _("Lists the contents of locations in a tree"));
  g_printerr ("\n");
  g_printerr (_("Use %s to get detailed help.\n"), "???gio help COMMAND???");
  exit (1);
}

int
main (int argc, char **argv)
{
  const char *command;
  xboolean_t do_help;

#ifdef G_OS_WIN32
  xchar_t *localedir;
#endif

  setlocale (LC_ALL, XPL_DEFAULT_LOCALE);
  textdomain (GETTEXT_PACKAGE);

#ifdef G_OS_WIN32
  localedir = _glib_get_locale_dir ();
  bindtextdomain (GETTEXT_PACKAGE, localedir);
  g_free (localedir);
#else
  bindtextdomain (GETTEXT_PACKAGE, XPL_LOCALE_DIR);
#endif

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  if (argc < 2)
    {
      usage ();
      return 1;
    }

  command = argv[1];
  argc -= 1;
  argv += 1;

  do_help = FALSE;
  if (xstr_equal (command, "help"))
    {
      if (argc == 1)
        {
          usage ();
          return 0;
        }
      else
        {
          command = argv[1];
          do_help = TRUE;
        }
    }
  else if (xstr_equal (command, "--help"))
    {
      usage ();
      return 0;
    }
  else if (xstr_equal (command, "--version"))
    command = "version";

  if (xstr_equal (command, "version"))
    return handle_version (argc, argv, do_help);
  else if (xstr_equal (command, "cat"))
    return handle_cat (argc, argv, do_help);
  else if (xstr_equal (command, "copy"))
    return handle_copy (argc, argv, do_help);
  else if (xstr_equal (command, "info"))
    return handle_info (argc, argv, do_help);
  else if (xstr_equal (command, "launch"))
    return handle_launch (argc, argv, do_help);
  else if (xstr_equal (command, "list"))
    return handle_list (argc, argv, do_help);
  else if (xstr_equal (command, "mime"))
    return handle_mime (argc, argv, do_help);
  else if (xstr_equal (command, "mkdir"))
    return handle_mkdir (argc, argv, do_help);
  else if (xstr_equal (command, "monitor"))
    return handle_monitor (argc, argv, do_help);
  else if (xstr_equal (command, "mount"))
    return handle_mount (argc, argv, do_help);
  else if (xstr_equal (command, "move"))
    return handle_move (argc, argv, do_help);
  else if (xstr_equal (command, "open"))
    return handle_open (argc, argv, do_help);
  else if (xstr_equal (command, "rename"))
    return handle_rename (argc, argv, do_help);
  else if (xstr_equal (command, "remove"))
    return handle_remove (argc, argv, do_help);
  else if (xstr_equal (command, "save"))
    return handle_save (argc, argv, do_help);
  else if (xstr_equal (command, "set"))
    return handle_set (argc, argv, do_help);
  else if (xstr_equal (command, "trash"))
    return handle_trash (argc, argv, do_help);
  else if (xstr_equal (command, "tree"))
    return handle_tree (argc, argv, do_help);
  else
    usage ();

  return 1;
}
