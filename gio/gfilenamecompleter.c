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
#include "gfilenamecompleter.h"
#include "gfileenumerator.h"
#include "gfileattribute.h"
#include "gfile.h"
#include "gfileinfo.h"
#include "gcancellable.h"
#include <string.h>
#include "glibintl.h"


/**
 * SECTION:gfilenamecompleter
 * @short_description: Filename Completer
 * @include: gio/gio.h
 *
 * Completes partial file and directory names given a partial string by
 * looking in the file system for clues. Can return a list of possible
 * completion strings for widget implementations.
 *
 **/

enum {
  GOT_COMPLETION_DATA,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

typedef struct {
  xfilename_completer_t *completer;
  xfile_enumerator_t *enumerator;
  xcancellable_t *cancellable;
  xboolean_t should_escape;
  xfile_t *dir;
  xlist_t *basenames;
  xboolean_t dirs_only;
} LoadBasenamesData;

struct _xfilename_completer {
  xobject_t parent;

  xfile_t *basenames_dir;
  xboolean_t basenames_are_escaped;
  xboolean_t dirs_only;
  xlist_t *basenames;

  LoadBasenamesData *basename_loader;
};

G_DEFINE_TYPE (xfilename_completer, xfilename_completer, XTYPE_OBJECT)

static void cancel_load_basenames (xfilename_completer_t *completer);

static void
xfilename_completer_finalize (xobject_t *object)
{
  xfilename_completer_t *completer;

  completer = XFILENAME_COMPLETER (object);

  cancel_load_basenames (completer);

  if (completer->basenames_dir)
    xobject_unref (completer->basenames_dir);

  xlist_free_full (completer->basenames, g_free);

  G_OBJECT_CLASS (xfilename_completer_parent_class)->finalize (object);
}

static void
xfilename_completer_class_init (xfilename_completer_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = xfilename_completer_finalize;
  /**
   * xfilename_completer_t::got-completion-data:
   *
   * Emitted when the file name completion information comes available.
   **/
  signals[GOT_COMPLETION_DATA] = g_signal_new (I_("got-completion-data"),
					  XTYPE_FILENAME_COMPLETER,
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (xfilename_completer_class_t, got_completion_data),
					  NULL, NULL,
					  NULL,
					  XTYPE_NONE, 0);
}

static void
xfilename_completer_init (xfilename_completer_t *completer)
{
}

/**
 * xfilename_completer_new:
 *
 * Creates a new filename completer.
 *
 * Returns: a #xfilename_completer_t.
 **/
xfilename_completer_t *
xfilename_completer_new (void)
{
  return xobject_new (XTYPE_FILENAME_COMPLETER, NULL);
}

static char *
longest_common_prefix (char *a, char *b)
{
  char *start;

  start = a;

  while (xutf8_get_char (a) == xutf8_get_char (b))
    {
      a = xutf8_next_char (a);
      b = xutf8_next_char (b);
    }

  return xstrndup (start, a - start);
}

static void
load_basenames_data_free (LoadBasenamesData *data)
{
  if (data->enumerator)
    xobject_unref (data->enumerator);

  xobject_unref (data->cancellable);
  xobject_unref (data->dir);

  xlist_free_full (data->basenames, g_free);

  g_free (data);
}

static void
got_more_files (xobject_t *source_object,
		xasync_result_t *res,
		xpointer_t user_data)
{
  LoadBasenamesData *data = user_data;
  xlist_t *infos, *l;
  xfile_info_t *info;
  const char *name;
  xboolean_t append_slash;
  char *t;
  char *basename;

  if (data->completer == NULL)
    {
      /* Was cancelled */
      load_basenames_data_free (data);
      return;
    }

  infos = xfile_enumerator_next_files_finish (data->enumerator, res, NULL);

  for (l = infos; l != NULL; l = l->next)
    {
      info = l->data;

      if (data->dirs_only &&
	  xfile_info_get_file_type (info) != XFILE_TYPE_DIRECTORY)
	{
	  xobject_unref (info);
	  continue;
	}

      append_slash = xfile_info_get_file_type (info) == XFILE_TYPE_DIRECTORY;
      name = xfile_info_get_name (info);
      if (name == NULL)
	{
	  xobject_unref (info);
	  continue;
	}


      if (data->should_escape)
	basename = xuri_escape_string (name,
					XURI_RESERVED_CHARS_ALLOWED_IN_PATH,
					TRUE);
      else
	/* If not should_escape, must be a local filename, convert to utf8 */
	basename = xfilename_to_utf8 (name, -1, NULL, NULL, NULL);

      if (basename)
	{
	  if (append_slash)
	    {
	      t = basename;
	      basename = xstrconcat (basename, "/", NULL);
	      g_free (t);
	    }

	  data->basenames = xlist_prepend (data->basenames, basename);
	}

      xobject_unref (info);
    }

  xlist_free (infos);

  if (infos)
    {
      /* Not last, get more files */
      xfile_enumerator_next_files_async (data->enumerator,
					  100,
					  0,
					  data->cancellable,
					  got_more_files, data);
    }
  else
    {
      data->completer->basename_loader = NULL;

      if (data->completer->basenames_dir)
	xobject_unref (data->completer->basenames_dir);
      xlist_free_full (data->completer->basenames, g_free);

      data->completer->basenames_dir = xobject_ref (data->dir);
      data->completer->basenames = data->basenames;
      data->completer->basenames_are_escaped = data->should_escape;
      data->basenames = NULL;

      xfile_enumerator_close_async (data->enumerator, 0, NULL, NULL, NULL);

      g_signal_emit (data->completer, signals[GOT_COMPLETION_DATA], 0);
      load_basenames_data_free (data);
    }
}


static void
got_enum (xobject_t *source_object,
	  xasync_result_t *res,
	  xpointer_t user_data)
{
  LoadBasenamesData *data = user_data;

  if (data->completer == NULL)
    {
      /* Was cancelled */
      load_basenames_data_free (data);
      return;
    }

  data->enumerator = xfile_enumerate_children_finish (XFILE (source_object), res, NULL);

  if (data->enumerator == NULL)
    {
      data->completer->basename_loader = NULL;

      if (data->completer->basenames_dir)
	xobject_unref (data->completer->basenames_dir);
      xlist_free_full (data->completer->basenames, g_free);

      /* Mark up-to-date with no basenames */
      data->completer->basenames_dir = xobject_ref (data->dir);
      data->completer->basenames = NULL;
      data->completer->basenames_are_escaped = data->should_escape;

      load_basenames_data_free (data);
      return;
    }

  xfile_enumerator_next_files_async (data->enumerator,
				      100,
				      0,
				      data->cancellable,
				      got_more_files, data);
}

static void
schedule_load_basenames (xfilename_completer_t *completer,
			 xfile_t *dir,
			 xboolean_t should_escape)
{
  LoadBasenamesData *data;

  cancel_load_basenames (completer);

  data = g_new0 (LoadBasenamesData, 1);
  data->completer = completer;
  data->cancellable = g_cancellable_new ();
  data->dir = xobject_ref (dir);
  data->should_escape = should_escape;
  data->dirs_only = completer->dirs_only;

  completer->basename_loader = data;

  xfile_enumerate_children_async (dir,
				   XFILE_ATTRIBUTE_STANDARD_NAME "," XFILE_ATTRIBUTE_STANDARD_TYPE,
				   0, 0,
				   data->cancellable,
				   got_enum, data);
}

static void
cancel_load_basenames (xfilename_completer_t *completer)
{
  LoadBasenamesData *loader;

  if (completer->basename_loader)
    {
      loader = completer->basename_loader;
      loader->completer = NULL;

      g_cancellable_cancel (loader->cancellable);

      completer->basename_loader = NULL;
    }
}


/* Returns a list of possible matches and the basename to use for it */
static xlist_t *
init_completion (xfilename_completer_t *completer,
		 const char *initial_text,
		 char **basename_out)
{
  xboolean_t should_escape;
  xfile_t *file, *parent;
  char *basename;
  char *t;
  int len;

  *basename_out = NULL;

  should_escape = ! (g_path_is_absolute (initial_text) || *initial_text == '~');

  len = strlen (initial_text);

  if (len > 0 &&
      initial_text[len - 1] == '/')
    return NULL;

  file = xfile_parse_name (initial_text);
  parent = xfile_get_parent (file);
  if (parent == NULL)
    {
      xobject_unref (file);
      return NULL;
    }

  if (completer->basenames_dir == NULL ||
      completer->basenames_are_escaped != should_escape ||
      !xfile_equal (parent, completer->basenames_dir))
    {
      schedule_load_basenames (completer, parent, should_escape);
      xobject_unref (file);
      return NULL;
    }

  basename = xfile_get_basename (file);
  if (should_escape)
    {
      t = basename;
      basename = xuri_escape_string (basename, XURI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
      g_free (t);
    }
  else
    {
      t = basename;
      basename = xfilename_to_utf8 (basename, -1, NULL, NULL, NULL);
      g_free (t);

      if (basename == NULL)
	return NULL;
    }

  *basename_out = basename;

  return completer->basenames;
}

/**
 * xfilename_completer_get_completion_suffix:
 * @completer: the filename completer.
 * @initial_text: text to be completed.
 *
 * Obtains a completion for @initial_text from @completer.
 *
 * Returns: (nullable) (transfer full): a completed string, or %NULL if no
 *     completion exists. This string is not owned by GIO, so remember to g_free()
 *     it when finished.
 **/
char *
xfilename_completer_get_completion_suffix (xfilename_completer_t *completer,
					    const char *initial_text)
{
  xlist_t *possible_matches, *l;
  char *prefix;
  char *suffix;
  char *possible_match;
  char *lcp;

  g_return_val_if_fail (X_IS_FILENAME_COMPLETER (completer), NULL);
  g_return_val_if_fail (initial_text != NULL, NULL);

  possible_matches = init_completion (completer, initial_text, &prefix);

  suffix = NULL;

  for (l = possible_matches; l != NULL; l = l->next)
    {
      possible_match = l->data;

      if (xstr_has_prefix (possible_match, prefix))
	{
	  if (suffix == NULL)
	    suffix = xstrdup (possible_match + strlen (prefix));
	  else
	    {
	      lcp = longest_common_prefix (suffix,
					   possible_match + strlen (prefix));
	      g_free (suffix);
	      suffix = lcp;

	      if (*suffix == 0)
		break;
	    }
	}
    }

  g_free (prefix);

  return suffix;
}

/**
 * xfilename_completer_get_completions:
 * @completer: the filename completer.
 * @initial_text: text to be completed.
 *
 * Gets an array of completion strings for a given initial text.
 *
 * Returns: (array zero-terminated=1) (transfer full): array of strings with possible completions for @initial_text.
 * This array must be freed by xstrfreev() when finished.
 **/
char **
xfilename_completer_get_completions (xfilename_completer_t *completer,
				      const char         *initial_text)
{
  xlist_t *possible_matches, *l;
  char *prefix;
  char *possible_match;
  xptr_array_t *res;

  g_return_val_if_fail (X_IS_FILENAME_COMPLETER (completer), NULL);
  g_return_val_if_fail (initial_text != NULL, NULL);

  possible_matches = init_completion (completer, initial_text, &prefix);

  res = xptr_array_new ();
  for (l = possible_matches; l != NULL; l = l->next)
    {
      possible_match = l->data;

      if (xstr_has_prefix (possible_match, prefix))
	xptr_array_add (res,
			 xstrconcat (initial_text, possible_match + strlen (prefix), NULL));
    }

  g_free (prefix);

  xptr_array_add (res, NULL);

  return (char**)xptr_array_free (res, FALSE);
}

/**
 * xfilename_completer_set_dirs_only:
 * @completer: the filename completer.
 * @dirs_only: a #xboolean_t.
 *
 * If @dirs_only is %TRUE, @completer will only
 * complete directory names, and not file names.
 **/
void
xfilename_completer_set_dirs_only (xfilename_completer_t *completer,
				    xboolean_t dirs_only)
{
  g_return_if_fail (X_IS_FILENAME_COMPLETER (completer));

  completer->dirs_only = dirs_only;
}
