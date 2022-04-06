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


static xboolean_t show_hidden = FALSE;
static xboolean_t follow_symlinks = FALSE;

static const GOptionEntry entries[] = {
  { "hidden", 'h', 0, G_OPTION_ARG_NONE, &show_hidden, N_("Show hidden files"), NULL },
  { "follow-symlinks", 'l', 0, G_OPTION_ARG_NONE, &follow_symlinks, N_("Follow symbolic links, mounts and shortcuts"), NULL },
  G_OPTION_ENTRY_NULL
};

static xint_t
sort_info_by_name (xfile_info_t *a, xfile_info_t *b)
{
  const char *na;
  const char *nb;

  na = xfile_info_get_name (a);
  nb = xfile_info_get_name (b);

  if (na == NULL)
    na = "";
  if (nb == NULL)
    nb = "";

  return strcmp (na, nb);
}

static void
do_tree (xfile_t *f, unsigned int level, xuint64_t pattern)
{
  xfile_enumerator_t *enumerator;
  xerror_t *error = NULL;
  unsigned int n;
  xfile_info_t *info;

  info = xfile_query_info (f,
			    XFILE_ATTRIBUTE_STANDARD_TYPE ","
			    XFILE_ATTRIBUTE_STANDARD_TARGET_URI,
			    0,
			    NULL, NULL);
  if (info != NULL)
    {
      if (xfile_info_get_attribute_uint32 (info, XFILE_ATTRIBUTE_STANDARD_TYPE) == XFILE_TYPE_MOUNTABLE)
	{
	  /* don't process mountables; we avoid these by getting the target_uri below */
	  xobject_unref (info);
	  return;
	}
      xobject_unref (info);
    }

  enumerator = xfile_enumerate_children (f,
					  XFILE_ATTRIBUTE_STANDARD_NAME ","
					  XFILE_ATTRIBUTE_STANDARD_TYPE ","
					  XFILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
					  XFILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
					  XFILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET ","
					  XFILE_ATTRIBUTE_STANDARD_TARGET_URI,
					  0,
					  NULL,
					  &error);
  if (enumerator != NULL)
    {
      xlist_t *l;
      xlist_t *info_list;

      info_list = NULL;
      while ((info = xfile_enumerator_next_file (enumerator, NULL, NULL)) != NULL)
	{
	  if (xfile_info_get_is_hidden (info) && !show_hidden)
	    {
	      xobject_unref (info);
	    }
	  else
	    {
	      info_list = xlist_prepend (info_list, info);
	    }
	}
      xfile_enumerator_close (enumerator, NULL, NULL);

      info_list = xlist_sort (info_list, (GCompareFunc) sort_info_by_name);

      for (l = info_list; l != NULL; l = l->next)
	{
	  const char *name;
	  const char *target_uri;
	  xfile_type_t type;
	  xboolean_t is_last_item;

	  info = l->data;
	  is_last_item = (l->next == NULL);

	  name = xfile_info_get_name (info);
	  type = xfile_info_get_attribute_uint32 (info, XFILE_ATTRIBUTE_STANDARD_TYPE);
	  if (name != NULL)
	    {

	      for (n = 0; n < level; n++)
		{
		  if (pattern & (1<<n))
		    {
		      g_print ("|   ");
		    }
		  else
		    {
		      g_print ("    ");
		    }
		}

	      if (is_last_item)
		{
		  g_print ("`-- %s", name);
		}
	      else
		{
		  g_print ("|-- %s", name);
		}

	      target_uri = xfile_info_get_attribute_string (info, XFILE_ATTRIBUTE_STANDARD_TARGET_URI);
	      if (target_uri != NULL)
		{
		  g_print (" -> %s", target_uri);
		}
	      else
		{
		  if (xfile_info_get_is_symlink (info))
		    {
		      const char *target;
		      target = xfile_info_get_symlink_target (info);
		      g_print (" -> %s", target);
		    }
		}

	      g_print ("\n");

	      if ((type & XFILE_TYPE_DIRECTORY) &&
		  (follow_symlinks || !xfile_info_get_is_symlink (info)))
		{
		  xuint64_t new_pattern;
		  xfile_t *child;

		  if (is_last_item)
		    new_pattern = pattern;
		  else
		    new_pattern = pattern | (1<<level);

		  child = NULL;
		  if (target_uri != NULL)
		    {
		      if (follow_symlinks)
			child = xfile_new_for_uri (target_uri);
		    }
		  else
		    {
		      child = xfile_get_child (f, name);
		    }

		  if (child != NULL)
		    {
		      do_tree (child, level + 1, new_pattern);
		      xobject_unref (child);
		    }
		}
	    }
	  xobject_unref (info);
	}
      xlist_free (info_list);
    }
  else
    {
      for (n = 0; n < level; n++)
	{
	  if (pattern & (1<<n))
	    {
	      g_print ("|   ");
	    }
	  else
	    {
	      g_print ("    ");
	    }
	}

      g_print ("    [%s]\n", error->message);

      xerror_free (error);
    }
}

static void
tree (xfile_t *f)
{
  char *uri;

  uri = xfile_get_uri (f);
  g_print ("%s\n", uri);
  g_free (uri);

  do_tree (f, 0, 0);
}

int
handle_tree (int argc, char *argv[], xboolean_t do_help)
{
  xoption_context_t *context;
  xerror_t *error = NULL;
  xfile_t *file;
  xchar_t *param;
  int i;

  g_set_prgname ("gio tree");

  /* Translators: commandline placeholder */
  param = xstrdup_printf ("[%s…]", _("LOCATION"));
  context = g_option_context_new (param);
  g_free (param);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_summary (context,
      _("List contents of directories in a tree-like format."));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  if (do_help)
    {
      show_help (context, NULL);
      g_option_context_free (context);
      return 0;
    }

  g_option_context_parse (context, &argc, &argv, &error);

  if (error != NULL)
    {
      show_help (context, error->message);
      xerror_free (error);
      g_option_context_free (context);
      return 1;
    }

  g_option_context_free (context);

  if (argc > 1)
    {
      for (i = 1; i < argc; i++)
        {
          file = xfile_new_for_commandline_arg (argv[i]);
          tree (file);
          xobject_unref (file);
        }
    }
  else
    {
      char *cwd;

      cwd = g_get_current_dir ();
      file = xfile_new_for_path (cwd);
      g_free (cwd);
      tree (file);
      xobject_unref (file);
    }

  return 0;
}
