/* XPL - Library of useful routines for C programming
 * gmappedfile.c: Simplified wrapper around the mmap() function.
 *
 * Copyright 2005 Matthias Clasen
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
 */

#include "config.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#include "glibconfig.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>

#undef fstat
#define fstat(a,b) _fstati64(a,b)
#undef stat
#define stat _stati64

#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif

#endif

#include "gconvert.h"
#include "gerror.h"
#include "gfileutils.h"
#include "gmappedfile.h"
#include "gmem.h"
#include "gmessages.h"
#include "gstdio.h"
#include "gstrfuncs.h"
#include "gatomic.h"

#include "glibintl.h"


#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

/**
 * xmapped_file_t:
 *
 * The #xmapped_file_t represents a file mapping created with
 * xmapped_file_new(). It has only private members and should
 * not be accessed directly.
 */

struct _GMappedFile
{
  xchar_t *contents;
  xsize_t  length;
  xpointer_t free_func;
  int    ref_count;
#ifdef G_OS_WIN32
  HANDLE mapping;
#endif
};

static void
xmapped_file_destroy (xmapped_file_t *file)
{
  if (file->length)
    {
#ifdef HAVE_MMAP
      munmap (file->contents, file->length);
#endif
#ifdef G_OS_WIN32
      UnmapViewOfFile (file->contents);
      CloseHandle (file->mapping);
#endif
    }

  g_slice_free (xmapped_file_t, file);
}

static xmapped_file_t*
mapped_file_new_from_fd (int           fd,
			 xboolean_t      writable,
                         const xchar_t  *filename,
                         xerror_t      **error)
{
  xmapped_file_t *file;
  struct stat st;

  file = g_slice_new0 (xmapped_file_t);
  file->ref_count = 1;
  file->free_func = xmapped_file_destroy;

  if (fstat (fd, &st) == -1)
    {
      int save_errno = errno;
      xchar_t *display_filename = filename ? xfilename_display_name (filename) : NULL;

      g_set_error (error,
                   XFILE_ERROR,
                   xfile_error_from_errno (save_errno),
                   _("Failed to get attributes of file “%s%s%s%s”: fstat() failed: %s"),
		   display_filename ? display_filename : "fd",
		   display_filename ? "' " : "",
		   display_filename ? display_filename : "",
		   display_filename ? "'" : "",
		   xstrerror (save_errno));
      g_free (display_filename);
      goto out;
    }

  /* mmap() on size 0 will fail with EINVAL, so we avoid calling mmap()
   * in that case -- but only if we have a regular file; we still want
   * attempts to mmap a character device to fail, for example.
   */
  if (st.st_size == 0 && S_ISREG (st.st_mode))
    {
      file->length = 0;
      file->contents = NULL;
      return file;
    }

  file->contents = MAP_FAILED;

#ifdef HAVE_MMAP
  if (sizeof (st.st_size) > sizeof (xsize_t) && st.st_size > (off_t) G_MAXSIZE)
    {
      errno = EINVAL;
    }
  else
    {
      file->length = (xsize_t) st.st_size;
      file->contents = (xchar_t *) mmap (NULL,  file->length,
				       writable ? PROT_READ|PROT_WRITE : PROT_READ,
				       MAP_PRIVATE, fd, 0);
    }
#endif
#ifdef G_OS_WIN32
  file->length = st.st_size;
  file->mapping = CreateFileMapping ((HANDLE) _get_osfhandle (fd), NULL,
				     writable ? PAGE_WRITECOPY : PAGE_READONLY,
				     0, 0,
				     NULL);
  if (file->mapping != NULL)
    {
      file->contents = MapViewOfFile (file->mapping,
				      writable ? FILE_MAP_COPY : FILE_MAP_READ,
				      0, 0,
				      0);
      if (file->contents == NULL)
	{
	  file->contents = MAP_FAILED;
	  CloseHandle (file->mapping);
	  file->mapping = NULL;
	}
    }
#endif


  if (file->contents == MAP_FAILED)
    {
      int save_errno = errno;
      xchar_t *display_filename = filename ? xfilename_display_name (filename) : NULL;

      g_set_error (error,
		   XFILE_ERROR,
		   xfile_error_from_errno (save_errno),
		   _("Failed to map %s%s%s%s: mmap() failed: %s"),
		   display_filename ? display_filename : "fd",
		   display_filename ? "' " : "",
		   display_filename ? display_filename : "",
		   display_filename ? "'" : "",
		   xstrerror (save_errno));
      g_free (display_filename);
      goto out;
    }

  return file;

 out:
  g_slice_free (xmapped_file_t, file);

  return NULL;
}

/**
 * xmapped_file_new:
 * @filename: (type filename): The path of the file to load, in the GLib
 *     filename encoding
 * @writable: whether the mapping should be writable
 * @error: return location for a #xerror_t, or %NULL
 *
 * Maps a file into memory. On UNIX, this is using the mmap() function.
 *
 * If @writable is %TRUE, the mapped buffer may be modified, otherwise
 * it is an error to modify the mapped buffer. Modifications to the buffer
 * are not visible to other processes mapping the same file, and are not
 * written back to the file.
 *
 * Note that modifications of the underlying file might affect the contents
 * of the #xmapped_file_t. Therefore, mapping should only be used if the file
 * will not be modified, or if all modifications of the file are done
 * atomically (e.g. using xfile_set_contents()).
 *
 * If @filename is the name of an empty, regular file, the function
 * will successfully return an empty #xmapped_file_t. In other cases of
 * size 0 (e.g. device files such as /dev/null), @error will be set
 * to the #GFileError value %XFILE_ERROR_INVAL.
 *
 * Returns: a newly allocated #xmapped_file_t which must be unref'd
 *    with xmapped_file_unref(), or %NULL if the mapping failed.
 *
 * Since: 2.8
 */
xmapped_file_t *
xmapped_file_new (const xchar_t  *filename,
		   xboolean_t      writable,
		   xerror_t      **error)
{
  xmapped_file_t *file;
  int fd;

  xreturn_val_if_fail (filename != NULL, NULL);
  xreturn_val_if_fail (!error || *error == NULL, NULL);

  fd = g_open (filename, (writable ? O_RDWR : O_RDONLY) | _O_BINARY, 0);
  if (fd == -1)
    {
      int save_errno = errno;
      xchar_t *display_filename = xfilename_display_name (filename);

      g_set_error (error,
                   XFILE_ERROR,
                   xfile_error_from_errno (save_errno),
                   _("Failed to open file “%s”: open() failed: %s"),
                   display_filename,
		   xstrerror (save_errno));
      g_free (display_filename);
      return NULL;
    }

  file = mapped_file_new_from_fd (fd, writable, filename, error);

  close (fd);

  return file;
}


/**
 * xmapped_file_new_from_fd:
 * @fd: The file descriptor of the file to load
 * @writable: whether the mapping should be writable
 * @error: return location for a #xerror_t, or %NULL
 *
 * Maps a file into memory. On UNIX, this is using the mmap() function.
 *
 * If @writable is %TRUE, the mapped buffer may be modified, otherwise
 * it is an error to modify the mapped buffer. Modifications to the buffer
 * are not visible to other processes mapping the same file, and are not
 * written back to the file.
 *
 * Note that modifications of the underlying file might affect the contents
 * of the #xmapped_file_t. Therefore, mapping should only be used if the file
 * will not be modified, or if all modifications of the file are done
 * atomically (e.g. using xfile_set_contents()).
 *
 * Returns: a newly allocated #xmapped_file_t which must be unref'd
 *    with xmapped_file_unref(), or %NULL if the mapping failed.
 *
 * Since: 2.32
 */
xmapped_file_t *
xmapped_file_new_from_fd (xint_t          fd,
			   xboolean_t      writable,
			   xerror_t      **error)
{
  return mapped_file_new_from_fd (fd, writable, NULL, error);
}

/**
 * xmapped_file_get_length:
 * @file: a #xmapped_file_t
 *
 * Returns the length of the contents of a #xmapped_file_t.
 *
 * Returns: the length of the contents of @file.
 *
 * Since: 2.8
 */
xsize_t
xmapped_file_get_length (xmapped_file_t *file)
{
  xreturn_val_if_fail (file != NULL, 0);

  return file->length;
}

/**
 * xmapped_file_get_contents:
 * @file: a #xmapped_file_t
 *
 * Returns the contents of a #xmapped_file_t.
 *
 * Note that the contents may not be zero-terminated,
 * even if the #xmapped_file_t is backed by a text file.
 *
 * If the file is empty then %NULL is returned.
 *
 * Returns: the contents of @file, or %NULL.
 *
 * Since: 2.8
 */
xchar_t *
xmapped_file_get_contents (xmapped_file_t *file)
{
  xreturn_val_if_fail (file != NULL, NULL);

  return file->contents;
}

/**
 * xmapped_file_free:
 * @file: a #xmapped_file_t
 *
 * This call existed before #xmapped_file_t had refcounting and is currently
 * exactly the same as xmapped_file_unref().
 *
 * Since: 2.8
 * Deprecated:2.22: Use xmapped_file_unref() instead.
 */
void
xmapped_file_free (xmapped_file_t *file)
{
  xmapped_file_unref (file);
}

/**
 * xmapped_file_ref:
 * @file: a #xmapped_file_t
 *
 * Increments the reference count of @file by one.  It is safe to call
 * this function from any thread.
 *
 * Returns: the passed in #xmapped_file_t.
 *
 * Since: 2.22
 **/
xmapped_file_t *
xmapped_file_ref (xmapped_file_t *file)
{
  xreturn_val_if_fail (file != NULL, NULL);

  g_atomic_int_inc (&file->ref_count);

  return file;
}

/**
 * xmapped_file_unref:
 * @file: a #xmapped_file_t
 *
 * Decrements the reference count of @file by one.  If the reference count
 * drops to 0, unmaps the buffer of @file and frees it.
 *
 * It is safe to call this function from any thread.
 *
 * Since 2.22
 **/
void
xmapped_file_unref (xmapped_file_t *file)
{
  g_return_if_fail (file != NULL);

  if (g_atomic_int_dec_and_test (&file->ref_count))
    xmapped_file_destroy (file);
}

/**
 * xmapped_file_get_bytes:
 * @file: a #xmapped_file_t
 *
 * Creates a new #xbytes_t which references the data mapped from @file.
 * The mapped contents of the file must not be modified after creating this
 * bytes object, because a #xbytes_t should be immutable.
 *
 * Returns: (transfer full): A newly allocated #xbytes_t referencing data
 *     from @file
 *
 * Since: 2.34
 **/
xbytes_t *
xmapped_file_get_bytes (xmapped_file_t *file)
{
  xreturn_val_if_fail (file != NULL, NULL);

  return xbytes_new_with_free_func (file->contents,
				     file->length,
				     (xdestroy_notify_t) xmapped_file_unref,
				     xmapped_file_ref (file));
}
