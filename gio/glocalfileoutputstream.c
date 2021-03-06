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

/* Needed for the statx() calls in inline functions in glocalfileinfo.h */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include "glibintl.h"
#include "gioerror.h"
#include "gcancellable.h"
#include "glocalfileoutputstream.h"
#include "gfileinfo.h"
#include "glocalfileinfo.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#include "gfiledescriptorbased.h"
#include <sys/uio.h>
#endif

#include "glib-private.h"
#include "gioprivate.h"

#ifdef G_OS_WIN32
#include <io.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#else
#define HAVE_O_CLOEXEC 1
#endif

struct _GLocalFileOutputStreamPrivate {
  char *tmp_filename;
  char *original_filename;
  char *backup_filename;
  char *etag;
  xuint_t sync_on_close : 1;
  xuint_t do_close : 1;
  int fd;
};

#ifdef G_OS_UNIX
static void       xfile_descriptor_based_iface_init   (GFileDescriptorBasedIface *iface);
#endif

#define g_local_file_output_stream_get_type _g_local_file_output_stream_get_type
#ifdef G_OS_UNIX
G_DEFINE_TYPE_WITH_CODE (GLocalFileOutputStream, g_local_file_output_stream, XTYPE_FILE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (GLocalFileOutputStream)
			 G_IMPLEMENT_INTERFACE (XTYPE_FILE_DESCRIPTOR_BASED,
						xfile_descriptor_based_iface_init))
#else
G_DEFINE_TYPE_WITH_CODE (GLocalFileOutputStream, g_local_file_output_stream, XTYPE_FILE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (GLocalFileOutputStream))
#endif


/* Some of the file replacement code was based on the code from gedit,
 * relicenced to LGPL with permissions from the authors.
 */

#define BACKUP_EXTENSION "~"

static xssize_t     g_local_file_output_stream_write        (xoutput_stream_t      *stream,
							   const void         *buffer,
							   xsize_t               count,
							   xcancellable_t       *cancellable,
							   xerror_t            **error);
#ifdef G_OS_UNIX
static xboolean_t   g_local_file_output_stream_writev       (xoutput_stream_t       *stream,
							   const xoutput_vector_t *vectors,
							   xsize_t                n_vectors,
							   xsize_t               *bytes_written,
							   xcancellable_t        *cancellable,
							   xerror_t             **error);
#endif
static xboolean_t   g_local_file_output_stream_close        (xoutput_stream_t      *stream,
							   xcancellable_t       *cancellable,
							   xerror_t            **error);
static xfile_info_t *g_local_file_output_stream_query_info   (xfile_output_stream_t  *stream,
							   const char         *attributes,
							   xcancellable_t       *cancellable,
							   xerror_t            **error);
static char *     g_local_file_output_stream_get_etag     (xfile_output_stream_t  *stream);
static xoffset_t    g_local_file_output_stream_tell         (xfile_output_stream_t  *stream);
static xboolean_t   g_local_file_output_stream_can_seek     (xfile_output_stream_t  *stream);
static xboolean_t   g_local_file_output_stream_seek         (xfile_output_stream_t  *stream,
							   xoffset_t             offset,
							   xseek_type_t           type,
							   xcancellable_t       *cancellable,
							   xerror_t            **error);
static xboolean_t   g_local_file_output_stream_can_truncate (xfile_output_stream_t  *stream);
static xboolean_t   g_local_file_output_stream_truncate     (xfile_output_stream_t  *stream,
							   xoffset_t             size,
							   xcancellable_t       *cancellable,
							   xerror_t            **error);
#ifdef G_OS_UNIX
static int        g_local_file_output_stream_get_fd       (xfile_descriptor_based_t *stream);
#endif

static void
g_local_file_output_stream_finalize (xobject_t *object)
{
  GLocalFileOutputStream *file;

  file = G_LOCAL_FILE_OUTPUT_STREAM (object);

  g_free (file->priv->tmp_filename);
  g_free (file->priv->original_filename);
  g_free (file->priv->backup_filename);
  g_free (file->priv->etag);

  XOBJECT_CLASS (g_local_file_output_stream_parent_class)->finalize (object);
}

static void
g_local_file_output_stream_class_init (GLocalFileOutputStreamClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
  xoutput_stream_class_t *stream_class = G_OUTPUT_STREAM_CLASS (klass);
  xfile_output_stream_class_t *file_stream_class = XFILE_OUTPUT_STREAM_CLASS (klass);

  xobject_class->finalize = g_local_file_output_stream_finalize;

  stream_class->write_fn = g_local_file_output_stream_write;
#ifdef G_OS_UNIX
  stream_class->writev_fn = g_local_file_output_stream_writev;
#endif
  stream_class->close_fn = g_local_file_output_stream_close;
  file_stream_class->query_info = g_local_file_output_stream_query_info;
  file_stream_class->get_etag = g_local_file_output_stream_get_etag;
  file_stream_class->tell = g_local_file_output_stream_tell;
  file_stream_class->can_seek = g_local_file_output_stream_can_seek;
  file_stream_class->seek = g_local_file_output_stream_seek;
  file_stream_class->can_truncate = g_local_file_output_stream_can_truncate;
  file_stream_class->truncate_fn = g_local_file_output_stream_truncate;
}

#ifdef G_OS_UNIX
static void
xfile_descriptor_based_iface_init (GFileDescriptorBasedIface *iface)
{
  iface->get_fd = g_local_file_output_stream_get_fd;
}
#endif

static void
g_local_file_output_stream_init (GLocalFileOutputStream *stream)
{
  stream->priv = g_local_file_output_stream_get_instance_private (stream);
  stream->priv->do_close = TRUE;
}

static xssize_t
g_local_file_output_stream_write (xoutput_stream_t  *stream,
				  const void     *buffer,
				  xsize_t           count,
				  xcancellable_t   *cancellable,
				  xerror_t        **error)
{
  GLocalFileOutputStream *file;
  xssize_t res;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  while (1)
    {
      if (xcancellable_set_error_if_cancelled (cancellable, error))
	return -1;
      res = write (file->priv->fd, buffer, count);
      if (res == -1)
	{
          int errsv = errno;

	  if (errsv == EINTR)
	    continue;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to file: %s"),
		       xstrerror (errsv));
	}

      break;
    }

  return res;
}

/* On Windows there is no equivalent API for files. The closest API to that is
 * WriteFileGather() but it is useless in general: it requires, among other
 * things, that each chunk is the size of a whole page and in memory aligned
 * to a page. We can't possibly guarantee that in GLib.
 */
#ifdef G_OS_UNIX
/* Macro to check if struct iovec and xoutput_vector_t have the same ABI */
#define G_OUTPUT_VECTOR_IS_IOVEC (sizeof (struct iovec) == sizeof (xoutput_vector_t) && \
      G_SIZEOF_MEMBER (struct iovec, iov_base) == G_SIZEOF_MEMBER (xoutput_vector_t, buffer) && \
      G_STRUCT_OFFSET (struct iovec, iov_base) == G_STRUCT_OFFSET (xoutput_vector_t, buffer) && \
      G_SIZEOF_MEMBER (struct iovec, iov_len) == G_SIZEOF_MEMBER (xoutput_vector_t, size) && \
      G_STRUCT_OFFSET (struct iovec, iov_len) == G_STRUCT_OFFSET (xoutput_vector_t, size))

static xboolean_t
g_local_file_output_stream_writev (xoutput_stream_t        *stream,
				   const xoutput_vector_t  *vectors,
				   xsize_t                 n_vectors,
				   xsize_t                *bytes_written,
				   xcancellable_t         *cancellable,
				   xerror_t              **error)
{
  GLocalFileOutputStream *file;
  xssize_t res;
  struct iovec *iov;

  if (bytes_written)
    *bytes_written = 0;

  /* Clamp the number of vectors if more given than we can write in one go.
   * The caller has to handle short writes anyway.
   */
  if (n_vectors > G_IOV_MAX)
    n_vectors = G_IOV_MAX;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  if (G_OUTPUT_VECTOR_IS_IOVEC)
    {
      /* ABI is compatible */
      iov = (struct iovec *) vectors;
    }
  else
    {
      xsize_t i;

      /* ABI is incompatible */
      iov = g_newa (struct iovec, n_vectors);
      for (i = 0; i < n_vectors; i++)
        {
          iov[i].iov_base = (void *)vectors[i].buffer;
          iov[i].iov_len = vectors[i].size;
        }
    }

  while (1)
    {
      if (xcancellable_set_error_if_cancelled (cancellable, error))
        return FALSE;
      res = writev (file->priv->fd, iov, n_vectors);
      if (res == -1)
        {
          int errsv = errno;

          if (errsv == EINTR)
            continue;

          g_set_error (error, G_IO_ERROR,
                       g_io_error_from_errno (errsv),
                       _("Error writing to file: %s"),
                       xstrerror (errsv));
        }
      else if (bytes_written)
        {
          *bytes_written = res;
        }

      break;
    }

  return res != -1;
}
#endif

void
_g_local_file_output_stream_set_do_close (GLocalFileOutputStream *out,
					  xboolean_t do_close)
{
  out->priv->do_close = do_close;
}

xboolean_t
_g_local_file_output_stream_really_close (GLocalFileOutputStream *file,
					  xcancellable_t   *cancellable,
					  xerror_t        **error)
{
  GLocalFileStat final_stat;

  if (file->priv->sync_on_close &&
      g_fsync (file->priv->fd) != 0)
    {
      int errsv = errno;

      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error writing to file: %s"),
		   xstrerror (errsv));
      goto err_out;
    }

#ifdef G_OS_WIN32

  /* Must close before renaming on Windows, so just do the close first
   * in all cases for now.
   */
  if (XPL_PRIVATE_CALL (g_win32_fstat) (file->priv->fd, &final_stat) == 0)
    file->priv->etag = _g_local_file_info_create_etag (&final_stat);

  if (!g_close (file->priv->fd, NULL))
    {
      int errsv = errno;

      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error closing file: %s"),
		   xstrerror (errsv));
      return FALSE;
    }

#endif

  if (file->priv->tmp_filename)
    {
      /* We need to move the temp file to its final place,
       * and possibly create the backup file
       */

      if (file->priv->backup_filename)
	{
	  if (xcancellable_set_error_if_cancelled (cancellable, error))
	    goto err_out;

#ifdef G_OS_UNIX
	  /* create original -> backup link, the original is then renamed over */
	  if (g_unlink (file->priv->backup_filename) != 0 &&
	      errno != ENOENT)
	    {
              int errsv = errno;

	      g_set_error (error, G_IO_ERROR,
			   G_IO_ERROR_CANT_CREATE_BACKUP,
			   _("Error removing old backup link: %s"),
			   xstrerror (errsv));
	      goto err_out;
	    }

	  if (link (file->priv->original_filename, file->priv->backup_filename) != 0)
	    {
	      /*  link failed or is not supported, try rename  */
	      if (g_rename (file->priv->original_filename, file->priv->backup_filename) != 0)
		{
                  int errsv = errno;

	    	  g_set_error (error, G_IO_ERROR,
		    	       G_IO_ERROR_CANT_CREATE_BACKUP,
			       _("Error creating backup copy: %s"),
			       xstrerror (errsv));
	          goto err_out;
		}
	    }
#else
	    /* If link not supported, just rename... */
	  if (g_rename (file->priv->original_filename, file->priv->backup_filename) != 0)
	    {
              int errsv = errno;

	      g_set_error (error, G_IO_ERROR,
			   G_IO_ERROR_CANT_CREATE_BACKUP,
			   _("Error creating backup copy: %s"),
			   xstrerror (errsv));
	      goto err_out;
	    }
#endif
	}


      if (xcancellable_set_error_if_cancelled (cancellable, error))
	goto err_out;

      /* tmp -> original */
      if (g_rename (file->priv->tmp_filename, file->priv->original_filename) != 0)
	{
          int errsv = errno;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error renaming temporary file: %s"),
		       xstrerror (errsv));
	  goto err_out;
	}

      g_clear_pointer (&file->priv->tmp_filename, g_free);
    }

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    goto err_out;

#ifndef G_OS_WIN32		/* Already did the fstat() and close() above on Win32 */

  if (g_local_file_fstat (file->priv->fd, G_LOCAL_FILE_STAT_FIELD_MTIME, G_LOCAL_FILE_STAT_FIELD_ALL, &final_stat) == 0)
    file->priv->etag = _g_local_file_info_create_etag (&final_stat);

  if (!g_close (file->priv->fd, NULL))
    {
      int errsv = errno;

      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   _("Error closing file: %s"),
                   xstrerror (errsv));
      goto err_out;
    }

#endif

  return TRUE;
 err_out:

#ifndef G_OS_WIN32
  /* A simple try to close the fd in case we fail before the actual close */
  g_close (file->priv->fd, NULL);
#endif
  if (file->priv->tmp_filename)
    g_unlink (file->priv->tmp_filename);

  return FALSE;
}


static xboolean_t
g_local_file_output_stream_close (xoutput_stream_t  *stream,
				  xcancellable_t   *cancellable,
				  xerror_t        **error)
{
  GLocalFileOutputStream *file;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  if (file->priv->do_close)
    return _g_local_file_output_stream_really_close (file,
						     cancellable,
						     error);
  return TRUE;
}

static char *
g_local_file_output_stream_get_etag (xfile_output_stream_t *stream)
{
  GLocalFileOutputStream *file;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  return xstrdup (file->priv->etag);
}

static xoffset_t
g_local_file_output_stream_tell (xfile_output_stream_t *stream)
{
  GLocalFileOutputStream *file;
  off_t pos;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  pos = lseek (file->priv->fd, 0, SEEK_CUR);

  if (pos == (off_t)-1)
    return 0;

  return pos;
}

static xboolean_t
g_local_file_output_stream_can_seek (xfile_output_stream_t *stream)
{
  GLocalFileOutputStream *file;
  off_t pos;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  pos = lseek (file->priv->fd, 0, SEEK_CUR);

  if (pos == (off_t)-1 && errno == ESPIPE)
    return FALSE;

  return TRUE;
}

static int
seek_type_to_lseek (xseek_type_t type)
{
  switch (type)
    {
    default:
    case G_SEEK_CUR:
      return SEEK_CUR;

    case G_SEEK_SET:
      return SEEK_SET;

    case G_SEEK_END:
      return SEEK_END;
    }
}

static xboolean_t
g_local_file_output_stream_seek (xfile_output_stream_t  *stream,
				 xoffset_t             offset,
				 xseek_type_t           type,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  GLocalFileOutputStream *file;
  off_t pos;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  pos = lseek (file->priv->fd, offset, seek_type_to_lseek (type));

  if (pos == (off_t)-1)
    {
      int errsv = errno;

      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error seeking in file: %s"),
		   xstrerror (errsv));
      return FALSE;
    }

  return TRUE;
}

static xboolean_t
g_local_file_output_stream_can_truncate (xfile_output_stream_t *stream)
{
  /* We can't truncate pipes and stuff where we can't seek */
  return g_local_file_output_stream_can_seek (stream);
}

static xboolean_t
g_local_file_output_stream_truncate (xfile_output_stream_t  *stream,
				     xoffset_t             size,
				     xcancellable_t       *cancellable,
				     xerror_t            **error)
{
  GLocalFileOutputStream *file;
  int res;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

 restart:
#ifdef G_OS_WIN32
  res = g_win32_ftruncate (file->priv->fd, size);
#else
  res = ftruncate (file->priv->fd, size);
#endif

  if (res == -1)
    {
      int errsv = errno;

      if (errsv == EINTR)
	{
	  if (xcancellable_set_error_if_cancelled (cancellable, error))
	    return FALSE;
	  goto restart;
	}

      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error truncating file: %s"),
		   xstrerror (errsv));
      return FALSE;
    }

  return TRUE;
}


static xfile_info_t *
g_local_file_output_stream_query_info (xfile_output_stream_t  *stream,
				       const char         *attributes,
				       xcancellable_t       *cancellable,
				       xerror_t            **error)
{
  GLocalFileOutputStream *file;

  file = G_LOCAL_FILE_OUTPUT_STREAM (stream);

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  return _g_local_file_info_get_from_fd (file->priv->fd,
					 attributes,
					 error);
}

xfile_output_stream_t *
_g_local_file_output_stream_new (int fd)
{
  GLocalFileOutputStream *stream;

  stream = xobject_new (XTYPE_LOCAL_FILE_OUTPUT_STREAM, NULL);
  stream->priv->fd = fd;
  return XFILE_OUTPUT_STREAM (stream);
}

static void
set_error_from_open_errno (const char *filename,
                           xerror_t    **error)
{
  int errsv = errno;

  if (errsv == EINVAL)
    /* This must be an invalid filename, on e.g. FAT */
    g_set_error_literal (error, G_IO_ERROR,
                         G_IO_ERROR_INVALID_FILENAME,
                         _("Invalid filename"));
  else
    {
      char *display_name = xfilename_display_name (filename);
      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   _("Error opening file ???%s???: %s"),
		       display_name, xstrerror (errsv));
      g_free (display_name);
    }
}

static xfile_output_stream_t *
output_stream_open (const char    *filename,
                    xint_t           open_flags,
                    xuint_t          mode,
                    xcancellable_t  *cancellable,
                    xerror_t       **error)
{
  GLocalFileOutputStream *stream;
  xint_t fd;

  fd = g_open (filename, open_flags, mode);
  if (fd == -1)
    {
      set_error_from_open_errno (filename, error);
      return NULL;
    }

  stream = xobject_new (XTYPE_LOCAL_FILE_OUTPUT_STREAM, NULL);
  stream->priv->fd = fd;
  return XFILE_OUTPUT_STREAM (stream);
}

xfile_output_stream_t *
_g_local_file_output_stream_open  (const char        *filename,
				   xboolean_t          readable,
				   xcancellable_t      *cancellable,
				   xerror_t           **error)
{
  int open_flags;

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  open_flags = O_BINARY;
  if (readable)
    open_flags |= O_RDWR;
  else
    open_flags |= O_WRONLY;

  return output_stream_open (filename, open_flags, 0666, cancellable, error);
}

static xint_t
mode_from_flags_or_info (xfile_create_flags_t   flags,
                         xfile_info_t         *reference_info)
{
  if (flags & XFILE_CREATE_PRIVATE)
    return 0600;
  else if (reference_info && xfile_info_has_attribute (reference_info, "unix::mode"))
    return xfile_info_get_attribute_uint32 (reference_info, "unix::mode") & (~S_IFMT);
  else
    return 0666;
}

xfile_output_stream_t *
_g_local_file_output_stream_create  (const char        *filename,
				     xboolean_t          readable,
				     xfile_create_flags_t   flags,
                                     xfile_info_t         *reference_info,
				     xcancellable_t      *cancellable,
				     xerror_t           **error)
{
  int mode;
  int open_flags;

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  mode = mode_from_flags_or_info (flags, reference_info);

  open_flags = O_CREAT | O_EXCL | O_BINARY;
  if (readable)
    open_flags |= O_RDWR;
  else
    open_flags |= O_WRONLY;

  return output_stream_open (filename, open_flags, mode, cancellable, error);
}

xfile_output_stream_t *
_g_local_file_output_stream_append  (const char        *filename,
				     xfile_create_flags_t   flags,
				     xcancellable_t      *cancellable,
				     xerror_t           **error)
{
  int mode;

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  if (flags & XFILE_CREATE_PRIVATE)
    mode = 0600;
  else
    mode = 0666;

  return output_stream_open (filename, O_CREAT | O_APPEND | O_WRONLY | O_BINARY, mode,
                             cancellable, error);
}

static char *
create_backup_filename (const char *filename)
{
  return xstrconcat (filename, BACKUP_EXTENSION, NULL);
}

#define BUFSIZE	8192 /* size of normal write buffer */

static xboolean_t
copy_file_data (xint_t     sfd,
		xint_t     dfd,
		xerror_t **error)
{
  xboolean_t ret = TRUE;
  xpointer_t buffer;
  const xchar_t *write_buffer;
  xssize_t bytes_read;
  xssize_t bytes_to_write;
  xssize_t bytes_written;

  buffer = g_malloc (BUFSIZE);

  do
    {
      bytes_read = read (sfd, buffer, BUFSIZE);
      if (bytes_read == -1)
	{
          int errsv = errno;

	  if (errsv == EINTR)
	    continue;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error reading from file: %s"),
		       xstrerror (errsv));
	  ret = FALSE;
	  break;
	}

      bytes_to_write = bytes_read;
      write_buffer = buffer;

      do
	{
	  bytes_written = write (dfd, write_buffer, bytes_to_write);
	  if (bytes_written == -1)
	    {
              int errsv = errno;

	      if (errsv == EINTR)
		continue;

	      g_set_error (error, G_IO_ERROR,
			   g_io_error_from_errno (errsv),
			   _("Error writing to file: %s"),
			   xstrerror (errsv));
	      ret = FALSE;
	      break;
	    }

	  bytes_to_write -= bytes_written;
	  write_buffer += bytes_written;
	}
      while (bytes_to_write > 0);

    } while ((bytes_read != 0) && (ret == TRUE));

  g_free (buffer);

  return ret;
}

static int
handle_overwrite_open (const char    *filename,
		       xboolean_t       readable,
		       const char    *etag,
		       xboolean_t       create_backup,
		       char         **temp_filename,
		       xfile_create_flags_t flags,
                       xfile_info_t       *reference_info,
		       xcancellable_t  *cancellable,
		       xerror_t       **error)
{
  int fd = -1;
  GLocalFileStat original_stat;
  char *current_etag;
  xboolean_t is_symlink;
  int open_flags;
  int res;
  int mode;
  int errsv;
  xboolean_t replace_destination_set = (flags & XFILE_CREATE_REPLACE_DESTINATION);

  mode = mode_from_flags_or_info (flags, reference_info);

  /* We only need read access to the original file if we are creating a backup.
   * We also add O_CREAT to avoid a race if the file was just removed */
  if (create_backup || readable)
    open_flags = O_RDWR | O_CREAT | O_BINARY;
  else
    open_flags = O_WRONLY | O_CREAT | O_BINARY;

  /* Some systems have O_NOFOLLOW, which lets us avoid some races
   * when finding out if the file we opened was a symlink */
#ifdef O_NOFOLLOW
  is_symlink = FALSE;
  fd = g_open (filename, open_flags | O_NOFOLLOW, mode);
  errsv = errno;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
  if (fd == -1 && errsv == EMLINK)
#elif defined(__NetBSD__)
  if (fd == -1 && errsv == EFTYPE)
#else
  if (fd == -1 && errsv == ELOOP)
#endif
    {
      /* Could be a symlink, or it could be a regular ELOOP error,
       * but then the next open will fail too. */
      is_symlink = TRUE;
      if (!replace_destination_set)
        fd = g_open (filename, open_flags, mode);
    }
#else  /* if !O_NOFOLLOW */
  /* This is racy, but we do it as soon as possible to minimize the race */
  is_symlink = xfile_test (filename, XFILE_TEST_IS_SYMLINK);

  if (!is_symlink || !replace_destination_set)
    {
      fd = g_open (filename, open_flags, mode);
      errsv = errno;
    }
#endif

  if (fd == -1 &&
      (!is_symlink || !replace_destination_set))
    {
      char *display_name = xfilename_display_name (filename);
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error opening file ???%s???: %s"),
		   display_name, xstrerror (errsv));
      g_free (display_name);
      return -1;
    }

  if (!is_symlink)
    {
      res = g_local_file_fstat (fd,
                                G_LOCAL_FILE_STAT_FIELD_TYPE |
                                G_LOCAL_FILE_STAT_FIELD_MODE |
                                G_LOCAL_FILE_STAT_FIELD_UID |
                                G_LOCAL_FILE_STAT_FIELD_GID |
                                G_LOCAL_FILE_STAT_FIELD_MTIME |
                                G_LOCAL_FILE_STAT_FIELD_NLINK,
                                G_LOCAL_FILE_STAT_FIELD_ALL, &original_stat);
      errsv = errno;
    }
  else
    {
      res = g_local_file_lstat (filename,
                                G_LOCAL_FILE_STAT_FIELD_TYPE |
                                G_LOCAL_FILE_STAT_FIELD_MODE |
                                G_LOCAL_FILE_STAT_FIELD_UID |
                                G_LOCAL_FILE_STAT_FIELD_GID |
                                G_LOCAL_FILE_STAT_FIELD_MTIME |
                                G_LOCAL_FILE_STAT_FIELD_NLINK,
                                G_LOCAL_FILE_STAT_FIELD_ALL, &original_stat);
      errsv = errno;
    }

  if (res != 0)
    {
      char *display_name = xfilename_display_name (filename);
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error when getting information for file ???%s???: %s"),
		   display_name, xstrerror (errsv));
      g_free (display_name);
      goto error;
    }

  /* not a regular file */
  if (!S_ISREG (_g_stat_mode (&original_stat)))
    {
      if (S_ISDIR (_g_stat_mode (&original_stat)))
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_IS_DIRECTORY,
                               _("Target file is a directory"));
          goto error;
        }
      else if (!is_symlink ||
#ifdef S_ISLNK
               !S_ISLNK (_g_stat_mode (&original_stat))
#else
               FALSE
#endif
               )
        {
          g_set_error_literal (error,
                             G_IO_ERROR,
                             G_IO_ERROR_NOT_REGULAR_FILE,
                             _("Target file is not a regular file"));
          goto error;
        }
    }

  if (etag != NULL)
    {
      GLocalFileStat etag_stat;
      GLocalFileStat *etag_stat_pointer;

      /* The ETag is calculated on the details of the target file, for symlinks,
       * so we might need to stat() again. */
      if (is_symlink)
        {
          res = g_local_file_stat (filename,
                                   G_LOCAL_FILE_STAT_FIELD_MTIME,
                                   G_LOCAL_FILE_STAT_FIELD_ALL, &etag_stat);
          errsv = errno;

          if (res != 0)
            {
              char *display_name = xfilename_display_name (filename);
              g_set_error (error, G_IO_ERROR,
                           g_io_error_from_errno (errsv),
                           _("Error when getting information for file ???%s???: %s"),
                           display_name, xstrerror (errsv));
              g_free (display_name);
              goto error;
            }

          etag_stat_pointer = &etag_stat;
        }
      else
        etag_stat_pointer = &original_stat;

      /* Compare the ETags */
      current_etag = _g_local_file_info_create_etag (etag_stat_pointer);
      if (strcmp (etag, current_etag) != 0)
	{
	  g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_WRONG_ETAG,
                               _("The file was externally modified"));
	  g_free (current_etag);
          goto error;
	}
      g_free (current_etag);
    }

  /* We use two backup strategies.
   * The first one (which is faster) consist in saving to a
   * tmp file then rename the original file to the backup and the
   * tmp file to the original name. This is fast but doesn't work
   * when the file is a link (hard or symbolic) or when we can't
   * write to the current dir or can't set the permissions on the
   * new file.
   * The second strategy consist simply in copying the old file
   * to a backup file and rewrite the contents of the file.
   */

  if (replace_destination_set ||
      (!(_g_stat_nlink (&original_stat) > 1) && !is_symlink))
    {
      char *dirname, *tmp_filename;
      int tmpfd;

      dirname = g_path_get_dirname (filename);
      tmp_filename = g_build_filename (dirname, ".goutputstream-XXXXXX", NULL);
      g_free (dirname);

      tmpfd = g_mkstemp_full (tmp_filename, (readable ? O_RDWR : O_WRONLY) | O_BINARY, mode);
      if (tmpfd == -1)
	{
	  g_free (tmp_filename);
	  goto fallback_strategy;
	}

      /* try to keep permissions (unless replacing) */

      if (!replace_destination_set &&
	   (
#ifdef HAVE_FCHOWN
	    fchown (tmpfd, _g_stat_uid (&original_stat), _g_stat_gid (&original_stat)) == -1 ||
#endif
#ifdef HAVE_FCHMOD
	    fchmod (tmpfd, _g_stat_mode (&original_stat) & ~S_IFMT) == -1 ||
#endif
	    0
	    )
	  )
	{
          GLocalFileStat tmp_statbuf;
          int tres;

          tres = g_local_file_fstat (tmpfd,
                                     G_LOCAL_FILE_STAT_FIELD_TYPE |
                                     G_LOCAL_FILE_STAT_FIELD_MODE |
                                     G_LOCAL_FILE_STAT_FIELD_UID |
                                     G_LOCAL_FILE_STAT_FIELD_GID,
                                     G_LOCAL_FILE_STAT_FIELD_ALL, &tmp_statbuf);

	  /* Check that we really needed to change something */
	  if (tres != 0 ||
	      _g_stat_uid (&original_stat) != _g_stat_uid (&tmp_statbuf) ||
	      _g_stat_gid (&original_stat) != _g_stat_gid (&tmp_statbuf) ||
	      _g_stat_mode (&original_stat) != _g_stat_mode (&tmp_statbuf))
	    {
	      g_close (tmpfd, NULL);
	      g_unlink (tmp_filename);
	      g_free (tmp_filename);
	      goto fallback_strategy;
	    }
	}

      if (fd >= 0)
        g_close (fd, NULL);
      *temp_filename = tmp_filename;
      return tmpfd;
    }

 fallback_strategy:

  if (create_backup)
    {
#if defined(HAVE_FCHOWN) && defined(HAVE_FCHMOD)
      GLocalFileStat tmp_statbuf;
#endif
      char *backup_filename;
      int bfd;

      backup_filename = create_backup_filename (filename);

      if (g_unlink (backup_filename) == -1 && errno != ENOENT)
	{
	  g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_CANT_CREATE_BACKUP,
                               _("Backup file creation failed"));
	  g_free (backup_filename);
          goto error;
	}

      bfd = g_open (backup_filename,
		    O_WRONLY | O_CREAT | O_EXCL | O_BINARY,
		    _g_stat_mode (&original_stat) & 0777);

      if (bfd == -1)
	{
	  g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_CANT_CREATE_BACKUP,
                               _("Backup file creation failed"));
	  g_free (backup_filename);
          goto error;
	}

      /* If needed, Try to set the group of the backup same as the
       * original file. If this fails, set the protection
       * bits for the group same as the protection bits for
       * others. */
#if defined(HAVE_FCHOWN) && defined(HAVE_FCHMOD)
      if (g_local_file_fstat (bfd, G_LOCAL_FILE_STAT_FIELD_GID, G_LOCAL_FILE_STAT_FIELD_ALL, &tmp_statbuf) != 0)
	{
	  g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_CANT_CREATE_BACKUP,
                               _("Backup file creation failed"));
	  g_unlink (backup_filename);
	  g_close (bfd, NULL);
	  g_free (backup_filename);
          goto error;
	}

      if ((_g_stat_gid (&original_stat) != _g_stat_gid (&tmp_statbuf))  &&
	  fchown (bfd, (uid_t) -1, _g_stat_gid (&original_stat)) != 0)
	{
	  if (fchmod (bfd,
		      (_g_stat_mode (&original_stat) & 0707) |
		      ((_g_stat_mode (&original_stat) & 07) << 3)) != 0)
	    {
	      g_set_error_literal (error,
                                   G_IO_ERROR,
                                   G_IO_ERROR_CANT_CREATE_BACKUP,
                                   _("Backup file creation failed"));
	      g_unlink (backup_filename);
	      g_close (bfd, NULL);
	      g_free (backup_filename);
              goto error;
	    }
	}
#endif

      if (!copy_file_data (fd, bfd, NULL))
	{
	  g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_CANT_CREATE_BACKUP,
                               _("Backup file creation failed"));
	  g_unlink (backup_filename);
          g_close (bfd, NULL);
	  g_free (backup_filename);

          goto error;
	}

      g_close (bfd, NULL);
      g_free (backup_filename);

      /* Seek back to the start of the file after the backup copy */
      if (lseek (fd, 0, SEEK_SET) == -1)
	{
          int errsv = errno;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error seeking in file: %s"),
		       xstrerror (errsv));
          goto error;
	}
    }

  if (replace_destination_set)
    {
      g_close (fd, NULL);

      if (g_unlink (filename) != 0)
	{
	  int errsv = errno;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error removing old file: %s"),
		       xstrerror (errsv));
          goto error;
	}

      if (readable)
	open_flags = O_RDWR | O_CREAT | O_BINARY;
      else
	open_flags = O_WRONLY | O_CREAT | O_BINARY;
      fd = g_open (filename, open_flags, mode);
      if (fd == -1)
	{
	  int errsv = errno;
	  char *display_name = xfilename_display_name (filename);
	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error opening file ???%s???: %s"),
		       display_name, xstrerror (errsv));
	  g_free (display_name);
          goto error;
	}
    }
  else
    {
      /* Truncate the file at the start */
#ifdef G_OS_WIN32
      if (g_win32_ftruncate (fd, 0) == -1)
#else
	if (ftruncate (fd, 0) == -1)
#endif
	  {
	    int errsv = errno;

	    g_set_error (error, G_IO_ERROR,
			 g_io_error_from_errno (errsv),
			 _("Error truncating file: %s"),
			 xstrerror (errsv));
            goto error;
	  }
    }

  return fd;

error:
  if (fd >= 0)
    g_close (fd, NULL);

  return -1;
}

xfile_output_stream_t *
_g_local_file_output_stream_replace (const char        *filename,
				     xboolean_t          readable,
				     const char        *etag,
				     xboolean_t           create_backup,
				     xfile_create_flags_t   flags,
                                     xfile_info_t         *reference_info,
				     xcancellable_t      *cancellable,
				     xerror_t           **error)
{
  GLocalFileOutputStream *stream;
  int mode;
  int fd;
  char *temp_file;
  xboolean_t sync_on_close;
  int open_flags;

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  temp_file = NULL;

  mode = mode_from_flags_or_info (flags, reference_info);
  sync_on_close = FALSE;

  /* If the file doesn't exist, create it */
  open_flags = O_CREAT | O_EXCL | O_BINARY | O_CLOEXEC;
  if (readable)
    open_flags |= O_RDWR;
  else
    open_flags |= O_WRONLY;
  fd = g_open (filename, open_flags, mode);

  if (fd == -1 && errno == EEXIST)
    {
      /* The file already exists */
      fd = handle_overwrite_open (filename, readable, etag,
                                  create_backup, &temp_file,
                                  flags, reference_info,
                                  cancellable, error);
      if (fd == -1)
	return NULL;

      /* If the final destination exists, we want to sync the newly written
       * file to ensure the data is on disk when we rename over the destination.
       * otherwise if we get a system crash we can lose both the new and the
       * old file on some filesystems. (I.E. those that don't guarantee the
       * data is written to the disk before the metadata.)
       */
      sync_on_close = TRUE;
    }
  else if (fd == -1)
    {
      set_error_from_open_errno (filename, error);
      return NULL;
    }
#if !defined(HAVE_O_CLOEXEC) && defined(F_SETFD)
  else
    fcntl (fd, F_SETFD, FD_CLOEXEC);
#endif

  stream = xobject_new (XTYPE_LOCAL_FILE_OUTPUT_STREAM, NULL);
  stream->priv->fd = fd;
  stream->priv->sync_on_close = sync_on_close;
  stream->priv->tmp_filename = temp_file;
  if (create_backup)
    stream->priv->backup_filename = create_backup_filename (filename);
  stream->priv->original_filename =  xstrdup (filename);

  return XFILE_OUTPUT_STREAM (stream);
}

xint_t
_g_local_file_output_stream_get_fd (GLocalFileOutputStream *stream)
{
  xreturn_val_if_fail (X_IS_LOCAL_FILE_OUTPUT_STREAM (stream), -1);
  return stream->priv->fd;
}

#ifdef G_OS_UNIX
static int
g_local_file_output_stream_get_fd (xfile_descriptor_based_t *fd_based)
{
  GLocalFileOutputStream *stream = G_LOCAL_FILE_OUTPUT_STREAM (fd_based);
  return _g_local_file_output_stream_get_fd (stream);
}
#endif
