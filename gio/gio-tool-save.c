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

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gio-tool.h"

static char *etag = NULL;
static xboolean_t backup = FALSE;
static xboolean_t create = FALSE;
static xboolean_t append = FALSE;
static xboolean_t priv = FALSE;
static xboolean_t replace_dest = FALSE;
static xboolean_t print_etag = FALSE;

static const GOptionEntry entries[] =
{
  { "backup", 'b', 0, G_OPTION_ARG_NONE, &backup, N_("Backup existing destination files"), NULL },
  { "create", 'c', 0, G_OPTION_ARG_NONE, &create, N_("Only create if not existing"), NULL },
  { "append", 'a', 0, G_OPTION_ARG_NONE, &append, N_("Append to end of file"), NULL },
  { "private", 'p', 0, G_OPTION_ARG_NONE, &priv, N_("When creating, restrict access to the current user"), NULL },
  { "unlink", 'u', 0, G_OPTION_ARG_NONE, &replace_dest, N_("When replacing, replace as if the destination did not exist"), NULL },
  /* Translators: The "etag" is a token allowing to verify whether a file has been modified */
  { "print-etag", 'v', 0, G_OPTION_ARG_NONE, &print_etag, N_("Print new etag at end"), NULL },
  /* Translators: The "etag" is a token allowing to verify whether a file has been modified */
  { "etag", 'e', 0, G_OPTION_ARG_STRING, &etag, N_("The etag of the file being overwritten"), N_("ETAG") },
  G_OPTION_ENTRY_NULL
};

/* 256k minus malloc overhead */
#define STREAM_BUFFER_SIZE (1024*256 - 2*sizeof(xpointer_t))

static xboolean_t
save (xfile_t *file)
{
  xoutput_stream_t *out;
  xfile_create_flags_t flags;
  char *buffer;
  xssize_t res;
  xboolean_t close_res;
  xerror_t *error;
  xboolean_t save_res;

  error = NULL;

  flags = priv ? XFILE_CREATE_PRIVATE : XFILE_CREATE_NONE;
  flags |= replace_dest ? XFILE_CREATE_REPLACE_DESTINATION : 0;

  if (create)
    out = (xoutput_stream_t *)xfile_create (file, flags, NULL, &error);
  else if (append)
    out = (xoutput_stream_t *)xfile_append_to (file, flags, NULL, &error);
  else
    out = (xoutput_stream_t *)xfile_replace (file, etag, backup, flags, NULL, &error);
  if (out == NULL)
    {
      print_file_error (file, error->message);
      xerror_free (error);
      return FALSE;
    }

  buffer = g_malloc (STREAM_BUFFER_SIZE);
  save_res = TRUE;

  while (1)
    {
      res = read (STDIN_FILENO, buffer, STREAM_BUFFER_SIZE);
      if (res > 0)
	{
          xoutput_stream_write_all (out, buffer, res, NULL, NULL, &error);
          if (error != NULL)
            {
              save_res = FALSE;
              print_file_error (file, error->message);
              g_clear_error (&error);
              goto out;
            }
	}
      else if (res < 0)
	{
	  save_res = FALSE;
          print_error ("%s", _("Error reading from standard input"));
	  break;
	}
      else if (res == 0)
	break;
    }

 out:

  close_res = xoutput_stream_close (out, NULL, &error);
  if (!close_res)
    {
      save_res = FALSE;
      print_file_error (file, error->message);
      xerror_free (error);
    }

  if (close_res && print_etag)
    {
      char *etag;
      etag = xfile_output_stream_get_etag (XFILE_OUTPUT_STREAM (out));

      if (etag)
	g_print ("Etag: %s\n", etag);
      else
	/* Translators: The "etag" is a token allowing to verify whether a file has been modified */
	g_print (_("Etag not available\n"));
      g_free (etag);
    }

  xobject_unref (out);
  g_free (buffer);

  return save_res;
}

int
handle_save (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xerror_t *error = NULL;
  xfile_t *file;
  xboolean_t res;

  g_set_prgname ("gio save");

  /* Translators: commandline placeholder */
  context = g_option_context_new (_("DESTINATION"));
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("Read from standard input and save to DEST."));
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
      show_help (context, _("No destination given"));
      g_option_context_free (context);
      return 1;
    }

  if (argc > 2)
    {
      show_help (context, _("Too many arguments"));
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  file = xfile_new_for_commandline_arg (argv[1]);
  res = save (file);
  xobject_unref (file);

  return res ? 0 : 2;
}
