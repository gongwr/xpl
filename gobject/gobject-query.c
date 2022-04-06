/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib-object.h>
#include <glib/gprintf.h>


static xchar_t *indent_inc = NULL;
static xuint_t spacing = 1;
static FILE *f_out = NULL;
static xtype_t root = 0;
static xboolean_t recursion = TRUE;

#if 0
#  define	O_SPACE	"\\as"
#  define	O_ESPACE " "
#  define	O_BRANCH "\\aE"
#  define	O_VLINE "\\al"
#  define	O_LLEAF	"\\aL"
#  define	O_KEY_FILL "_"
#else
#  define	O_SPACE	" "
#  define	O_ESPACE ""
#  define	O_BRANCH "+"
#  define	O_VLINE "|"
#  define	O_LLEAF	"`"
#  define	O_KEY_FILL "_"
#endif

static void
show_nodes (xtype_t        type,
	    xtype_t        sibling,
	    const xchar_t *indent)
{
  xtype_t   *children;
  xuint_t i;

  if (!type)
    return;

  children = xtype_children (type, NULL);

  if (type != root)
    for (i = 0; i < spacing; i++)
      g_fprintf (f_out, "%s%s\n", indent, O_VLINE);

  g_fprintf (f_out, "%s%s%s%s",
	   indent,
	   sibling ? O_BRANCH : (type != root ? O_LLEAF : O_SPACE),
	   O_ESPACE,
	   xtype_name (type));

  for (i = strlen (xtype_name (type)); i <= strlen (indent_inc); i++)
    fputs (O_KEY_FILL, f_out);

  fputc ('\n', f_out);

  if (children && recursion)
    {
      xchar_t *new_indent;
      xtype_t   *child;

      if (sibling)
	new_indent = xstrconcat (indent, O_VLINE, indent_inc, NULL);
      else
	new_indent = xstrconcat (indent, O_SPACE, indent_inc, NULL);

      for (child = children; *child; child++)
	show_nodes (child[0], child[1], new_indent);

      g_free (new_indent);
    }

  g_free (children);
}

static xint_t
help (xchar_t *arg)
{
  g_fprintf (stderr, "usage: gobject-query <qualifier> [-r <type>] [-{i|b} \"\"] [-s #] [-{h|x|y}]\n");
  g_fprintf (stderr, "       -r       specify root type\n");
  g_fprintf (stderr, "       -n       don't descend type tree\n");
  g_fprintf (stderr, "       -h       guess what ;)\n");
  g_fprintf (stderr, "       -b       specify indent string\n");
  g_fprintf (stderr, "       -i       specify incremental indent string\n");
  g_fprintf (stderr, "       -s       specify line spacing\n");
  g_fprintf (stderr, "qualifiers:\n");
  g_fprintf (stderr, "       froots   iterate over fundamental roots\n");
  g_fprintf (stderr, "       tree     print type tree\n");

  return arg != NULL;
}

int
main (xint_t   argc,
      xchar_t *argv[])
{
  GLogLevelFlags fatal_mask;
  xboolean_t gen_froots = 0;
  xboolean_t gen_tree = 0;
  xint_t i;
  const xchar_t *iindent = "";

  f_out = stdout;

  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
  g_log_set_always_fatal (fatal_mask);

  root = XTYPE_OBJECT;

  for (i = 1; i < argc; i++)
    {
      if (strcmp ("-s", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    spacing = atoi (argv[i]);
	}
      else if (strcmp ("-i", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    {
	      char *p;
	      xuint_t n;

	      p = argv[i];
	      while (*p)
		p++;
	      n = p - argv[i];
	      indent_inc = g_new (xchar_t, n * strlen (O_SPACE) + 1);
	      *indent_inc = 0;
	      while (n)
		{
		  n--;
		  strcpy (indent_inc, O_SPACE);
		}
	    }
	}
      else if (strcmp ("-b", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    iindent = argv[i];
	}
      else if (strcmp ("-r", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    root = xtype_from_name (argv[i]);
	}
      else if (strcmp ("-n", argv[i]) == 0)
	{
	  recursion = FALSE;
	}
      else if (strcmp ("froots", argv[i]) == 0)
	{
	  gen_froots = 1;
	}
      else if (strcmp ("tree", argv[i]) == 0)
	{
	  gen_tree = 1;
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else if (strcmp ("--help", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	return help (argv[i]);
    }

  if (!gen_froots && !gen_tree)
    return help ((argc > 0) ? argv[i-1] : NULL);

  if (!indent_inc)
    {
      indent_inc = g_new (xchar_t, strlen (O_SPACE) + 1);
      *indent_inc = 0;
      strcpy (indent_inc, O_SPACE);
    }

  if (gen_tree)
    show_nodes (root, 0, iindent);
  if (gen_froots)
    {
      root = ~0;
      for (i = 0; i <= XTYPE_FUNDAMENTAL_MAX; i += XTYPE_MAKE_FUNDAMENTAL (1))
	{
	  const xchar_t *name = xtype_name (i);

	  if (name)
	    show_nodes (i, 0, iindent);
	}
    }

  return 0;
}
