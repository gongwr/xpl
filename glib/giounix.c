/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * giounix.c: IO Channels using unix file descriptors
 * Copyright 1998 Owen Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"

#define _POSIX_SOURCE		/* for SSIZE_MAX */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <glib/gstdio.h>

#include "giochannel.h"

#include "gerror.h"
#include "gfileutils.h"
#include "gstrfuncs.h"
#include "gtestutils.h"

/*
 * Unix IO Channels
 */

typedef struct _GIOUnixChannel GIOUnixChannel;
typedef struct _GIOUnixWatch GIOUnixWatch;

struct _GIOUnixChannel
{
  xio_channel_t channel;
  xint_t fd;
};

struct _GIOUnixWatch
{
  xsource_t       source;
  xpollfd_t       pollfd;
  xio_channel_t   *channel;
  xio_condition_t  condition;
};


static GIOStatus	g_io_unix_read		(xio_channel_t   *channel,
						 xchar_t        *buf,
						 xsize_t         count,
						 xsize_t        *bytes_read,
						 xerror_t      **err);
static GIOStatus	g_io_unix_write		(xio_channel_t   *channel,
						 const xchar_t  *buf,
						 xsize_t         count,
						 xsize_t        *bytes_written,
						 xerror_t      **err);
static GIOStatus	g_io_unix_seek		(xio_channel_t   *channel,
						 sint64_t        offset,
						 xseek_type_t     type,
						 xerror_t      **err);
static GIOStatus	g_io_unix_close		(xio_channel_t   *channel,
						 xerror_t      **err);
static void		g_io_unix_free		(xio_channel_t   *channel);
static xsource_t*		g_io_unix_create_watch	(xio_channel_t   *channel,
						 xio_condition_t  condition);
static GIOStatus	g_io_unix_set_flags	(xio_channel_t   *channel,
                       				 GIOFlags      flags,
						 xerror_t      **err);
static GIOFlags 	g_io_unix_get_flags	(xio_channel_t   *channel);

static xboolean_t g_io_unix_prepare  (xsource_t     *source,
				    xint_t        *timeout);
static xboolean_t g_io_unix_check    (xsource_t     *source);
static xboolean_t g_io_unix_dispatch (xsource_t     *source,
				    xsource_func_t  callback,
				    xpointer_t     user_data);
static void     g_io_unix_finalize (xsource_t     *source);

xsource_funcs_t g_io_watch_funcs = {
  g_io_unix_prepare,
  g_io_unix_check,
  g_io_unix_dispatch,
  g_io_unix_finalize,
  NULL, NULL
};

static GIOFuncs unix_channel_funcs = {
  g_io_unix_read,
  g_io_unix_write,
  g_io_unix_seek,
  g_io_unix_close,
  g_io_unix_create_watch,
  g_io_unix_free,
  g_io_unix_set_flags,
  g_io_unix_get_flags,
};

static xboolean_t
g_io_unix_prepare (xsource_t  *source,
		   xint_t     *timeout)
{
  GIOUnixWatch *watch = (GIOUnixWatch *)source;
  xio_condition_t buffer_condition = g_io_channel_get_buffer_condition (watch->channel);

  *timeout = -1;

  /* Only return TRUE here if _all_ bits in watch->condition will be set
   */
  return ((watch->condition & buffer_condition) == watch->condition);
}

static xboolean_t
g_io_unix_check (xsource_t  *source)
{
  GIOUnixWatch *watch = (GIOUnixWatch *)source;
  xio_condition_t buffer_condition = g_io_channel_get_buffer_condition (watch->channel);
  xio_condition_t poll_condition = watch->pollfd.revents;

  return ((poll_condition | buffer_condition) & watch->condition);
}

static xboolean_t
g_io_unix_dispatch (xsource_t     *source,
		    xsource_func_t  callback,
		    xpointer_t     user_data)

{
  GIOFunc func = (GIOFunc)callback;
  GIOUnixWatch *watch = (GIOUnixWatch *)source;
  xio_condition_t buffer_condition = g_io_channel_get_buffer_condition (watch->channel);

  if (!func)
    {
      g_warning ("IO watch dispatched without callback. "
		 "You must call xsource_connect().");
      return FALSE;
    }

  return (*func) (watch->channel,
		  (watch->pollfd.revents | buffer_condition) & watch->condition,
		  user_data);
}

static void
g_io_unix_finalize (xsource_t *source)
{
  GIOUnixWatch *watch = (GIOUnixWatch *)source;

  g_io_channel_unref (watch->channel);
}

static GIOStatus
g_io_unix_read (xio_channel_t *channel,
		xchar_t      *buf,
		xsize_t       count,
		xsize_t      *bytes_read,
		xerror_t    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  xssize_t result;

  if (count > SSIZE_MAX) /* At least according to the Debian manpage for read */
    count = SSIZE_MAX;

 retry:
  result = read (unix_channel->fd, buf, count);

  if (result < 0)
    {
      int errsv = errno;
      *bytes_read = 0;

      switch (errsv)
        {
#ifdef EINTR
          case EINTR:
            goto retry;
#endif
#ifdef EAGAIN
          case EAGAIN:
            return G_IO_STATUS_AGAIN;
#endif
          default:
            g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                                 g_io_channel_error_from_errno (errsv),
                                 xstrerror (errsv));
            return G_IO_STATUS_ERROR;
        }
    }

  *bytes_read = result;

  return (result > 0) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF;
}

static GIOStatus
g_io_unix_write (xio_channel_t  *channel,
		 const xchar_t *buf,
		 xsize_t       count,
		 xsize_t      *bytes_written,
		 xerror_t    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  xssize_t result;

 retry:
  result = write (unix_channel->fd, buf, count);

  if (result < 0)
    {
      int errsv = errno;
      *bytes_written = 0;

      switch (errsv)
        {
#ifdef EINTR
          case EINTR:
            goto retry;
#endif
#ifdef EAGAIN
          case EAGAIN:
            return G_IO_STATUS_AGAIN;
#endif
          default:
            g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                                 g_io_channel_error_from_errno (errsv),
                                 xstrerror (errsv));
            return G_IO_STATUS_ERROR;
        }
    }

  *bytes_written = result;

  return G_IO_STATUS_NORMAL;
}

static GIOStatus
g_io_unix_seek (xio_channel_t *channel,
		sint64_t      offset,
		xseek_type_t   type,
                xerror_t    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  int whence;
  off_t tmp_offset;
  off_t result;

  switch (type)
    {
    case G_SEEK_SET:
      whence = SEEK_SET;
      break;
    case G_SEEK_CUR:
      whence = SEEK_CUR;
      break;
    case G_SEEK_END:
      whence = SEEK_END;
      break;
    default:
      whence = -1; /* Shut the compiler up */
      g_assert_not_reached ();
    }

  tmp_offset = offset;
  if (tmp_offset != offset)
    {
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (EINVAL),
                           xstrerror (EINVAL));
      return G_IO_STATUS_ERROR;
    }

  result = lseek (unix_channel->fd, tmp_offset, whence);

  if (result < 0)
    {
      int errsv = errno;
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (errsv),
                           xstrerror (errsv));
      return G_IO_STATUS_ERROR;
    }

  return G_IO_STATUS_NORMAL;
}


static GIOStatus
g_io_unix_close (xio_channel_t *channel,
		 xerror_t    **err)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;

  if (close (unix_channel->fd) < 0)
    {
      int errsv = errno;
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (errsv),
                           xstrerror (errsv));
      return G_IO_STATUS_ERROR;
    }

  return G_IO_STATUS_NORMAL;
}

static void
g_io_unix_free (xio_channel_t *channel)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;

  g_free (unix_channel);
}

static xsource_t *
g_io_unix_create_watch (xio_channel_t   *channel,
			xio_condition_t  condition)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  xsource_t *source;
  GIOUnixWatch *watch;


  source = xsource_new (&g_io_watch_funcs, sizeof (GIOUnixWatch));
  xsource_set_static_name (source, "xio_channel_t (Unix)");
  watch = (GIOUnixWatch *)source;

  watch->channel = channel;
  g_io_channel_ref (channel);

  watch->condition = condition;

  watch->pollfd.fd = unix_channel->fd;
  watch->pollfd.events = condition;

  xsource_add_poll (source, &watch->pollfd);

  return source;
}

static GIOStatus
g_io_unix_set_flags (xio_channel_t *channel,
                     GIOFlags    flags,
                     xerror_t    **err)
{
  xlong_t fcntl_flags;
  GIOUnixChannel *unix_channel = (GIOUnixChannel *) channel;

  fcntl_flags = 0;

  if (flags & G_IO_FLAG_APPEND)
    fcntl_flags |= O_APPEND;
  if (flags & G_IO_FLAG_NONBLOCK)
#ifdef O_NONBLOCK
    fcntl_flags |= O_NONBLOCK;
#else
    fcntl_flags |= O_NDELAY;
#endif

  if (fcntl (unix_channel->fd, F_SETFL, fcntl_flags) == -1)
    {
      int errsv = errno;
      g_set_error_literal (err, G_IO_CHANNEL_ERROR,
                           g_io_channel_error_from_errno (errsv),
                           xstrerror (errsv));
      return G_IO_STATUS_ERROR;
    }

  return G_IO_STATUS_NORMAL;
}

static GIOFlags
g_io_unix_get_flags (xio_channel_t *channel)
{
  GIOFlags flags = 0;
  xlong_t fcntl_flags;
  GIOUnixChannel *unix_channel = (GIOUnixChannel *) channel;

  fcntl_flags = fcntl (unix_channel->fd, F_GETFL);

  if (fcntl_flags == -1)
    {
      int err = errno;
      g_warning (G_STRLOC "Error while getting flags for FD: %s (%d)",
		 xstrerror (err), err);
      return 0;
    }

  if (fcntl_flags & O_APPEND)
    flags |= G_IO_FLAG_APPEND;
#ifdef O_NONBLOCK
  if (fcntl_flags & O_NONBLOCK)
#else
  if (fcntl_flags & O_NDELAY)
#endif
    flags |= G_IO_FLAG_NONBLOCK;

  switch (fcntl_flags & (O_RDONLY | O_WRONLY | O_RDWR))
    {
      case O_RDONLY:
        channel->is_readable = TRUE;
        channel->is_writeable = FALSE;
        break;
      case O_WRONLY:
        channel->is_readable = FALSE;
        channel->is_writeable = TRUE;
        break;
      case O_RDWR:
        channel->is_readable = TRUE;
        channel->is_writeable = TRUE;
        break;
      default:
        g_assert_not_reached ();
    }

  return flags;
}

xio_channel_t *
g_io_channel_new_file (const xchar_t *filename,
                       const xchar_t *mode,
                       xerror_t     **error)
{
  int fid, flags;
  mode_t create_mode;
  xio_channel_t *channel;
  enum { /* Cheesy hack */
    MODE_R = 1 << 0,
    MODE_W = 1 << 1,
    MODE_A = 1 << 2,
    MODE_PLUS = 1 << 3,
    MODE_R_PLUS = MODE_R | MODE_PLUS,
    MODE_W_PLUS = MODE_W | MODE_PLUS,
    MODE_A_PLUS = MODE_A | MODE_PLUS
  } mode_num;
  struct stat buffer;

  xreturn_val_if_fail (filename != NULL, NULL);
  xreturn_val_if_fail (mode != NULL, NULL);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL), NULL);

  switch (mode[0])
    {
      case 'r':
        mode_num = MODE_R;
        break;
      case 'w':
        mode_num = MODE_W;
        break;
      case 'a':
        mode_num = MODE_A;
        break;
      default:
        g_warning ("Invalid GIOFileMode %s.", mode);
        return NULL;
    }

  switch (mode[1])
    {
      case '\0':
        break;
      case '+':
        if (mode[2] == '\0')
          {
            mode_num |= MODE_PLUS;
            break;
          }
        G_GNUC_FALLTHROUGH;
      default:
        g_warning ("Invalid GIOFileMode %s.", mode);
        return NULL;
    }

  switch (mode_num)
    {
      case MODE_R:
        flags = O_RDONLY;
        break;
      case MODE_W:
        flags = O_WRONLY | O_TRUNC | O_CREAT;
        break;
      case MODE_A:
        flags = O_WRONLY | O_APPEND | O_CREAT;
        break;
      case MODE_R_PLUS:
        flags = O_RDWR;
        break;
      case MODE_W_PLUS:
        flags = O_RDWR | O_TRUNC | O_CREAT;
        break;
      case MODE_A_PLUS:
        flags = O_RDWR | O_APPEND | O_CREAT;
        break;
      case MODE_PLUS:
      default:
        g_assert_not_reached ();
        flags = 0;
    }

  create_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  fid = g_open (filename, flags, create_mode);
  if (fid == -1)
    {
      int err = errno;
      g_set_error_literal (error, XFILE_ERROR,
                           xfile_error_from_errno (err),
                           xstrerror (err));
      return (xio_channel_t *)NULL;
    }

  if (fstat (fid, &buffer) == -1) /* In case someone opens a FIFO */
    {
      int err = errno;
      close (fid);
      g_set_error_literal (error, XFILE_ERROR,
                           xfile_error_from_errno (err),
                           xstrerror (err));
      return (xio_channel_t *)NULL;
    }

  channel = (xio_channel_t *) g_new (GIOUnixChannel, 1);

  channel->is_seekable = S_ISREG (buffer.st_mode) || S_ISCHR (buffer.st_mode)
                         || S_ISBLK (buffer.st_mode);

  switch (mode_num)
    {
      case MODE_R:
        channel->is_readable = TRUE;
        channel->is_writeable = FALSE;
        break;
      case MODE_W:
      case MODE_A:
        channel->is_readable = FALSE;
        channel->is_writeable = TRUE;
        break;
      case MODE_R_PLUS:
      case MODE_W_PLUS:
      case MODE_A_PLUS:
        channel->is_readable = TRUE;
        channel->is_writeable = TRUE;
        break;
      case MODE_PLUS:
      default:
        g_assert_not_reached ();
    }

  g_io_channel_init (channel);
  channel->close_on_unref = TRUE; /* must be after g_io_channel_init () */
  channel->funcs = &unix_channel_funcs;

  ((GIOUnixChannel *) channel)->fd = fid;
  return channel;
}

/**
 * g_io_channel_unix_new:
 * @fd: a file descriptor.
 *
 * Creates a new #xio_channel_t given a file descriptor. On UNIX systems
 * this works for plain files, pipes, and sockets.
 *
 * The returned #xio_channel_t has a reference count of 1.
 *
 * The default encoding for #xio_channel_t is UTF-8. If your application
 * is reading output from a command using via pipe, you may need to set
 * the encoding to the encoding of the current locale (see
 * g_get_charset()) with the g_io_channel_set_encoding() function.
 * By default, the fd passed will not be closed when the final reference
 * to the #xio_channel_t data structure is dropped.
 *
 * If you want to read raw binary data without interpretation, then
 * call the g_io_channel_set_encoding() function with %NULL for the
 * encoding argument.
 *
 * This function is available in GLib on Windows, too, but you should
 * avoid using it on Windows. The domain of file descriptors and
 * sockets overlap. There is no way for GLib to know which one you mean
 * in case the argument you pass to this function happens to be both a
 * valid file descriptor and socket. If that happens a warning is
 * issued, and GLib assumes that it is the file descriptor you mean.
 *
 * Returns: a new #xio_channel_t.
 **/
xio_channel_t *
g_io_channel_unix_new (xint_t fd)
{
  struct stat buffer;
  GIOUnixChannel *unix_channel = g_new (GIOUnixChannel, 1);
  xio_channel_t *channel = (xio_channel_t *)unix_channel;

  g_io_channel_init (channel);
  channel->funcs = &unix_channel_funcs;

  unix_channel->fd = fd;

  /* I'm not sure if fstat on a non-file (e.g., socket) works
   * it should be safe to say if it fails, the fd isn't seekable.
   */
  /* Newer UNIX versions support S_ISSOCK(), fstat() will probably
   * succeed in most cases.
   */
  if (fstat (unix_channel->fd, &buffer) == 0)
    channel->is_seekable = S_ISREG (buffer.st_mode) || S_ISCHR (buffer.st_mode)
                           || S_ISBLK (buffer.st_mode);
  else /* Assume not seekable */
    channel->is_seekable = FALSE;

  g_io_unix_get_flags (channel); /* Sets is_readable, is_writeable */

  return channel;
}

/**
 * g_io_channel_unix_get_fd:
 * @channel: a #xio_channel_t, created with g_io_channel_unix_new().
 *
 * Returns the file descriptor of the #xio_channel_t.
 *
 * On Windows this function returns the file descriptor or socket of
 * the #xio_channel_t.
 *
 * Returns: the file descriptor of the #xio_channel_t.
 **/
xint_t
g_io_channel_unix_get_fd (xio_channel_t *channel)
{
  GIOUnixChannel *unix_channel = (GIOUnixChannel *)channel;
  return unix_channel->fd;
}
