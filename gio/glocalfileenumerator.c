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

#include <glib.h>
#include <glocalfileenumerator.h>
#include <glocalfileinfo.h>
#include <glocalfile.h>
#include <gioerror.h>
#include <string.h>
#include <stdlib.h>
#include "glibintl.h"


#define CHUNK_SIZE 1000

#ifdef G_OS_WIN32
#define USE_GDIR
#endif

#ifndef USE_GDIR

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

typedef struct {
  char *name;
  long inode;
  xfile_type_t type;
} DirEntry;

#endif

struct _xlocal_file_enumerator
{
  xfile_enumerator_t parent;

  xfile_attribute_matcher_t *matcher;
  xfile_attribute_matcher_t *reduced_matcher;
  char *filename;
  char *attributes;
  xfile_query_info_flags_t flags;

  xboolean_t got_parent_info;
  GLocalParentFileInfo parent_info;

#ifdef USE_GDIR
  xdir_t *dir;
#else
  DIR *dir;
  DirEntry *entries;
  int entries_pos;
  xboolean_t at_end;
#endif

  xboolean_t follow_symlinks;
};

#define xlocal_file_enumerator_get_type _xlocal_file_enumerator_get_type
XDEFINE_TYPE (xlocal_file_enumerator, xlocal_file_enumerator, XTYPE_FILE_ENUMERATOR)

static xfile_info_t *xlocal_file_enumerator_next_file (xfile_enumerator_t  *enumerator,
						     xcancellable_t     *cancellable,
						     xerror_t          **error);
static xboolean_t   xlocal_file_enumerator_close     (xfile_enumerator_t  *enumerator,
						     xcancellable_t     *cancellable,
						     xerror_t          **error);


static void
free_entries (xlocal_file_enumerator_t *local)
{
#ifndef USE_GDIR
  int i;

  if (local->entries != NULL)
    {
      for (i = 0; local->entries[i].name != NULL; i++)
	g_free (local->entries[i].name);

      g_free (local->entries);
    }
#endif
}

static void
xlocal_file_enumerator_finalize (xobject_t *object)
{
  xlocal_file_enumerator_t *local;

  local = G_LOCAL_FILE_ENUMERATOR (object);

  if (local->got_parent_info)
    _g_local_file_info_free_parent_info (&local->parent_info);
  g_free (local->filename);
  xfile_attribute_matcher_unref (local->matcher);
  xfile_attribute_matcher_unref (local->reduced_matcher);
  if (local->dir)
    {
#ifdef USE_GDIR
      g_dir_close (local->dir);
#else
      closedir (local->dir);
#endif
      local->dir = NULL;
    }

  free_entries (local);

  XOBJECT_CLASS (xlocal_file_enumerator_parent_class)->finalize (object);
}


static void
xlocal_file_enumerator_class_init (xlocal_file_enumerator_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
  xfile_enumerator_class_t *enumerator_class = XFILE_ENUMERATOR_CLASS (klass);

  xobject_class->finalize = xlocal_file_enumerator_finalize;

  enumerator_class->next_file = xlocal_file_enumerator_next_file;
  enumerator_class->close_fn = xlocal_file_enumerator_close;
}

static void
xlocal_file_enumerator_init (xlocal_file_enumerator_t *local)
{
}

#ifdef USE_GDIR
static void
convert_file_to_io_error (xerror_t **error,
			  xerror_t  *file_error)
{
  int new_code;

  if (file_error == NULL)
    return;

  new_code = G_IO_ERROR_FAILED;

  if (file_error->domain == XFILE_ERROR)
    {
      switch (file_error->code)
        {
        case XFILE_ERROR_NOENT:
          new_code = G_IO_ERROR_NOT_FOUND;
          break;
        case XFILE_ERROR_ACCES:
          new_code = G_IO_ERROR_PERMISSION_DENIED;
          break;
        case XFILE_ERROR_NOTDIR:
          new_code = G_IO_ERROR_NOT_DIRECTORY;
          break;
        case XFILE_ERROR_MFILE:
          new_code = G_IO_ERROR_TOO_MANY_OPEN_FILES;
          break;
        default:
          break;
        }
    }

  g_set_error_literal (error, G_IO_ERROR,
                       new_code,
                       file_error->message);
}
#else
static xfile_attribute_matcher_t *
xfile_attribute_matcher_subtract_attributes (xfile_attribute_matcher_t *matcher,
                                              const char *           attributes)
{
  xfile_attribute_matcher_t *result, *tmp;

  tmp = xfile_attribute_matcher_new (attributes);
  result = xfile_attribute_matcher_subtract (matcher, tmp);
  xfile_attribute_matcher_unref (tmp);

  return result;
}
#endif

xfile_enumerator_t *
_xlocal_file_enumerator_new (GLocalFile *file,
			      const char           *attributes,
			      xfile_query_info_flags_t   flags,
			      xcancellable_t         *cancellable,
			      xerror_t              **error)
{
  xlocal_file_enumerator_t *local;
  char *filename = xfile_get_path (XFILE (file));

#ifdef USE_GDIR
  xerror_t *dir_error;
  xdir_t *dir;

  dir_error = NULL;
  dir = g_dir_open (filename, 0, error != NULL ? &dir_error : NULL);
  if (dir == NULL)
    {
      if (error != NULL)
	{
	  convert_file_to_io_error (error, dir_error);
	  xerror_free (dir_error);
	}
      g_free (filename);
      return NULL;
    }
#else
  DIR *dir;
  int errsv;

  dir = opendir (filename);
  if (dir == NULL)
    {
      xchar_t *utf8_filename;
      errsv = errno;

      utf8_filename = xfilename_to_utf8 (filename, -1, NULL, NULL, NULL);
      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   "Error opening directory '%s': %s",
                   utf8_filename, xstrerror (errsv));
      g_free (utf8_filename);
      g_free (filename);
      return NULL;
    }

#endif

  local = xobject_new (XTYPE_LOCAL_FILE_ENUMERATOR,
                        "container", file,
                        NULL);

  local->dir = dir;
  local->filename = filename;
  local->matcher = xfile_attribute_matcher_new (attributes);
#ifndef USE_GDIR
  local->reduced_matcher = xfile_attribute_matcher_subtract_attributes (local->matcher,
                                                                         G_LOCAL_FILE_INFO_NOSTAT_ATTRIBUTES","
                                                                         "standard::type");
#endif
  local->flags = flags;

  return XFILE_ENUMERATOR (local);
}

#ifndef USE_GDIR
static int
sort_by_inode (const void *_a, const void *_b)
{
  const DirEntry *a, *b;

  a = _a;
  b = _b;
  return a->inode - b->inode;
}

#ifdef HAVE_STRUCT_DIRENT_D_TYPE
static xfile_type_t
file_type_from_dirent (char d_type)
{
  switch (d_type)
    {
    case DT_BLK:
    case DT_CHR:
    case DT_FIFO:
    case DT_SOCK:
      return XFILE_TYPE_SPECIAL;
    case DT_DIR:
      return XFILE_TYPE_DIRECTORY;
    case DT_LNK:
      return XFILE_TYPE_SYMBOLIC_LINK;
    case DT_REG:
      return XFILE_TYPE_REGULAR;
    case DT_UNKNOWN:
    default:
      return XFILE_TYPE_UNKNOWN;
    }
}
#endif

static const char *
next_file_helper (xlocal_file_enumerator_t *local, xfile_type_t *file_type)
{
  struct dirent *entry;
  const char *filename;
  int i;

  if (local->at_end)
    return NULL;

  if (local->entries == NULL ||
      (local->entries[local->entries_pos].name == NULL))
    {
      if (local->entries == NULL)
	local->entries = g_new (DirEntry, CHUNK_SIZE + 1);
      else
	{
	  /* Restart by clearing old names */
	  for (i = 0; local->entries[i].name != NULL; i++)
	    g_free (local->entries[i].name);
	}

      for (i = 0; i < CHUNK_SIZE; i++)
	{
	  entry = readdir (local->dir);
	  while (entry
		 && (0 == strcmp (entry->d_name, ".") ||
		     0 == strcmp (entry->d_name, "..")))
	    entry = readdir (local->dir);

	  if (entry)
	    {
	      local->entries[i].name = xstrdup (entry->d_name);
	      local->entries[i].inode = entry->d_ino;
#if HAVE_STRUCT_DIRENT_D_TYPE
              local->entries[i].type = file_type_from_dirent (entry->d_type);
#else
              local->entries[i].type = XFILE_TYPE_UNKNOWN;
#endif
	    }
	  else
	    break;
	}
      local->entries[i].name = NULL;
      local->entries_pos = 0;

      qsort (local->entries, i, sizeof (DirEntry), sort_by_inode);
    }

  filename = local->entries[local->entries_pos].name;
  if (filename == NULL)
    local->at_end = TRUE;

  *file_type = local->entries[local->entries_pos].type;

  local->entries_pos++;

  return filename;
}

#endif

static xfile_info_t *
xlocal_file_enumerator_next_file (xfile_enumerator_t  *enumerator,
				   xcancellable_t     *cancellable,
				   xerror_t          **error)
{
  xlocal_file_enumerator_t *local = G_LOCAL_FILE_ENUMERATOR (enumerator);
  const char *filename;
  char *path;
  xfile_info_t *info;
  xerror_t *my_error;
  xfile_type_t file_type;

  if (!local->got_parent_info)
    {
      _g_local_file_info_get_parent_info (local->filename, local->matcher, &local->parent_info);
      local->got_parent_info = TRUE;
    }

 next_file:

#ifdef USE_GDIR
  filename = g_dir_read_name (local->dir);
  file_type = XFILE_TYPE_UNKNOWN;
#else
  filename = next_file_helper (local, &file_type);
#endif

  if (filename == NULL)
    return NULL;

  my_error = NULL;
  path = g_build_filename (local->filename, filename, NULL);
  if (file_type == XFILE_TYPE_UNKNOWN ||
      (file_type == XFILE_TYPE_SYMBOLIC_LINK && !(local->flags & XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS)))
    {
      info = _g_local_file_info_get (filename, path,
                                     local->matcher,
                                     local->flags,
                                     &local->parent_info,
                                     &my_error);
    }
  else
    {
      info = _g_local_file_info_get (filename, path,
                                     local->reduced_matcher,
                                     local->flags,
                                     &local->parent_info,
                                     &my_error);
      if (info)
        {
          _g_local_file_info_get_nostat (info, filename, path, local->matcher);
          xfile_info_set_file_type (info, file_type);
          if (file_type == XFILE_TYPE_SYMBOLIC_LINK)
            xfile_info_set_is_symlink (info, TRUE);
        }
    }
  g_free (path);

  if (info == NULL)
    {
      /* Failed to get info */
      /* If the file does not exist there might have been a race where
       * the file was removed between the readdir and the stat, so we
       * ignore the file. */
      if (xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
	{
	  xerror_free (my_error);
	  goto next_file;
	}
      else
	g_propagate_error (error, my_error);
    }

  return info;
}

static xboolean_t
xlocal_file_enumerator_close (xfile_enumerator_t  *enumerator,
			       xcancellable_t     *cancellable,
			       xerror_t          **error)
{
  xlocal_file_enumerator_t *local = G_LOCAL_FILE_ENUMERATOR (enumerator);

  if (local->dir)
    {
#ifdef USE_GDIR
      g_dir_close (local->dir);
#else
      closedir (local->dir);
#endif
      local->dir = NULL;
    }

  return TRUE;
}
