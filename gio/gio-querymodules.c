/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include "giomodule.h"
#include "giomodule-priv.h"

#include <gstdio.h>
#include <errno.h>
#include <locale.h>

#include "glib/glib-private.h"

static xboolean_t
is_valid_module_name (const xchar_t *basename)
{
#if !defined(G_OS_WIN32) && !defined(G_WITH_CYGWIN)
  return
    xstr_has_prefix (basename, "lib") &&
    xstr_has_suffix (basename, ".so");
#else
  return xstr_has_suffix (basename, ".dll");
#endif
}

static void
query_dir (const char *dirname)
{
  xstring_t *data;
  xdir_t *dir;
  xlist_t *list = NULL, *iterator = NULL;
  const char *name;
  char *cachename;
  char **(* query)  (void);
  xerror_t *error;
  int i;

  if (!g_module_supported ())
    return;

  error = NULL;
  dir = g_dir_open (dirname, 0, &error);
  if (!dir)
    {
      g_printerr ("Unable to open directory %s: %s\n", dirname, error->message);
      xerror_free (error);
      return;
    }

  data = xstring_new ("");

  while ((name = g_dir_read_name (dir)))
    list = xlist_prepend (list, xstrdup (name));

  list = xlist_sort (list, (GCompareFunc) xstrcmp0);
  for (iterator = list; iterator; iterator = iterator->next)
    {
      GModule *module;
      xchar_t     *path;
      char **extension_points;

      name = iterator->data;
      if (!is_valid_module_name (name))
	continue;

      path = g_build_filename (dirname, name, NULL);
      module = g_module_open_full (path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL, &error);
      g_free (path);

      if (module)
	{
	  xchar_t *modulename;
	  xchar_t *symname;

	  modulename = _xio_module_extract_name (name);
	  symname = xstrconcat ("g_io_", modulename, "_query", NULL);
	  g_module_symbol (module, symname, (xpointer_t) &query);
	  g_free (symname);
	  g_free (modulename);

	  if (!query)
	    {
	      /* Fallback to old name */
	      g_module_symbol (module, "xio_module_query", (xpointer_t) &query);
	    }

	  if (query)
	    {
	      extension_points = query ();

	      if (extension_points)
		{
		  xstring_append_printf (data, "%s: ", name);

		  for (i = 0; extension_points[i] != NULL; i++)
		    xstring_append_printf (data, "%s%s", i == 0 ? "" : ",", extension_points[i]);

		  xstring_append (data, "\n");
		  xstrfreev (extension_points);
		}
	    }

	  g_module_close (module);
	}
      else
        {
          g_debug ("Failed to open module %s: %s", name, error->message);
        }

      g_clear_error (&error);
    }

  g_dir_close (dir);
  xlist_free_full (list, g_free);

  cachename = g_build_filename (dirname, "giomodule.cache", NULL);

  if (data->len > 0)
    {
      error = NULL;

      if (!xfile_set_contents (cachename, data->str, data->len, &error))
        {
          g_printerr ("Unable to create %s: %s\n", cachename, error->message);
          xerror_free (error);
        }
    }
  else
    {
      if (g_unlink (cachename) != 0 && errno != ENOENT)
        {
          int errsv = errno;
          g_printerr ("Unable to unlink %s: %s\n", cachename, xstrerror (errsv));
        }
    }

  g_free (cachename);
  xstring_free (data, TRUE);
}

int
main (xint_t   argc,
      xchar_t *argv[])
{
  int i;

  if (argc <= 1)
    {
      g_print ("Usage: gio-querymodules <directory1> [<directory2> ...]\n");
      g_print ("Will update giomodule.cache in the listed directories\n");
      return 1;
    }

  setlocale (LC_ALL, XPL_DEFAULT_LOCALE);

  /* Be defensive and ensure we're linked to xobject_t */
  xtype_ensure (XTYPE_OBJECT);

  for (i = 1; i < argc; i++)
    query_dir (argv[i]);

  return 0;
}
