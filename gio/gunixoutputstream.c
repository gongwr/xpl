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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/uio.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/glib-unix.h>
#include "gioerror.h"
#include "gunixoutputstream.h"
#include "gcancellable.h"
#include "gasynchelper.h"
#include "gfiledescriptorbased.h"
#include "glibintl.h"
#include "gioprivate.h"
#include "giounix-private.h"


/**
 * SECTION:gunixoutputstream
 * @short_description: Streaming output operations for UNIX file descriptors
 * @include: gio/gunixoutputstream.h
 * @see_also: #xoutput_stream_t
 *
 * #GUnixOutputStream implements #xoutput_stream_t for writing to a UNIX
 * file descriptor, including asynchronous operations. (If the file
 * descriptor refers to a socket or pipe, this will use poll() to do
 * asynchronous I/O. If it refers to a regular file, it will fall back
 * to doing asynchronous I/O in another thread.)
 *
 * Note that `<gio/gunixoutputstream.h>` belongs to the UNIX-specific GIO
 * interfaces, thus you have to use the `gio-unix-2.0.pc` pkg-config file
 * when using it.
 */

enum {
  PROP_0,
  PROP_FD,
  PROP_CLOSE_FD
};

struct _GUnixOutputStreamPrivate {
  int fd;
  xuint_t close_fd : 1;
  xuint_t can_poll : 1;
};

static void g_unix_output_stream_pollable_iface_init (xpollable_output_stream_interface_t *iface);
static void g_unix_output_stream_file_descriptor_based_iface_init (GFileDescriptorBasedIface *iface);

G_DEFINE_TYPE_WITH_CODE (GUnixOutputStream, g_unix_output_stream, XTYPE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (GUnixOutputStream)
			 G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_OUTPUT_STREAM,
						g_unix_output_stream_pollable_iface_init)
			 G_IMPLEMENT_INTERFACE (XTYPE_FILE_DESCRIPTOR_BASED,
						g_unix_output_stream_file_descriptor_based_iface_init)
			 )

static void     g_unix_output_stream_set_property (xobject_t              *object,
						   xuint_t                 prop_id,
						   const xvalue_t         *value,
						   xparam_spec_t           *pspec);
static void     g_unix_output_stream_get_property (xobject_t              *object,
						   xuint_t                 prop_id,
						   xvalue_t               *value,
						   xparam_spec_t           *pspec);
static xssize_t   g_unix_output_stream_write        (xoutput_stream_t        *stream,
						   const void           *buffer,
						   xsize_t                 count,
						   xcancellable_t         *cancellable,
						   xerror_t              **error);
static xboolean_t g_unix_output_stream_writev       (xoutput_stream_t        *stream,
						   const xoutput_vector_t  *vectors,
						   xsize_t                 n_vectors,
						   xsize_t                *bytes_written,
						   xcancellable_t         *cancellable,
						   xerror_t              **error);
static xboolean_t g_unix_output_stream_close        (xoutput_stream_t        *stream,
						   xcancellable_t         *cancellable,
						   xerror_t              **error);

static xboolean_t g_unix_output_stream_pollable_can_poll      (xpollable_output_stream_t *stream);
static xboolean_t g_unix_output_stream_pollable_is_writable   (xpollable_output_stream_t *stream);
static xsource_t *g_unix_output_stream_pollable_create_source (xpollable_output_stream_t *stream,
							     xcancellable_t         *cancellable);
static GPollableReturn g_unix_output_stream_pollable_writev_nonblocking (xpollable_output_stream_t  *stream,
									 const xoutput_vector_t    *vectors,
									 xsize_t                   n_vectors,
									 xsize_t                  *bytes_written,
									 xerror_t                **error);

static void
g_unix_output_stream_class_init (GUnixOutputStreamClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  xoutput_stream_class_t *stream_class = G_OUTPUT_STREAM_CLASS (klass);

  gobject_class->get_property = g_unix_output_stream_get_property;
  gobject_class->set_property = g_unix_output_stream_set_property;

  stream_class->write_fn = g_unix_output_stream_write;
  stream_class->writev_fn = g_unix_output_stream_writev;
  stream_class->close_fn = g_unix_output_stream_close;

   /**
   * GUnixOutputStream:fd:
   *
   * The file descriptor that the stream writes to.
   *
   * Since: 2.20
   */
  xobject_class_install_property (gobject_class,
				   PROP_FD,
				   g_param_spec_int ("fd",
						     P_("File descriptor"),
						     P_("The file descriptor to write to"),
						     G_MININT, G_MAXINT, -1,
						     G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

  /**
   * GUnixOutputStream:close-fd:
   *
   * Whether to close the file descriptor when the stream is closed.
   *
   * Since: 2.20
   */
  xobject_class_install_property (gobject_class,
				   PROP_CLOSE_FD,
				   g_param_spec_boolean ("close-fd",
							 P_("Close file descriptor"),
							 P_("Whether to close the file descriptor when the stream is closed"),
							 TRUE,
							 G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void
g_unix_output_stream_pollable_iface_init (xpollable_output_stream_interface_t *iface)
{
  iface->can_poll = g_unix_output_stream_pollable_can_poll;
  iface->is_writable = g_unix_output_stream_pollable_is_writable;
  iface->create_source = g_unix_output_stream_pollable_create_source;
  iface->writev_nonblocking = g_unix_output_stream_pollable_writev_nonblocking;
}

static void
g_unix_output_stream_file_descriptor_based_iface_init (GFileDescriptorBasedIface *iface)
{
  iface->get_fd = (int (*) (xfile_descriptor_based_t *))g_unix_output_stream_get_fd;
}

static void
g_unix_output_stream_set_property (xobject_t         *object,
				   xuint_t            prop_id,
				   const xvalue_t    *value,
				   xparam_spec_t      *pspec)
{
  GUnixOutputStream *unix_stream;

  unix_stream = G_UNIX_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_FD:
      unix_stream->priv->fd = xvalue_get_int (value);
      unix_stream->priv->can_poll = _g_fd_is_pollable (unix_stream->priv->fd);
      break;
    case PROP_CLOSE_FD:
      unix_stream->priv->close_fd = xvalue_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_unix_output_stream_get_property (xobject_t    *object,
				   xuint_t       prop_id,
				   xvalue_t     *value,
				   xparam_spec_t *pspec)
{
  GUnixOutputStream *unix_stream;

  unix_stream = G_UNIX_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_FD:
      xvalue_set_int (value, unix_stream->priv->fd);
      break;
    case PROP_CLOSE_FD:
      xvalue_set_boolean (value, unix_stream->priv->close_fd);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_unix_output_stream_init (GUnixOutputStream *unix_stream)
{
  unix_stream->priv = g_unix_output_stream_get_instance_private (unix_stream);
  unix_stream->priv->fd = -1;
  unix_stream->priv->close_fd = TRUE;
}

/**
 * g_unix_output_stream_new:
 * @fd: a UNIX file descriptor
 * @close_fd: %TRUE to close the file descriptor when done
 *
 * Creates a new #GUnixOutputStream for the given @fd.
 *
 * If @close_fd, is %TRUE, the file descriptor will be closed when
 * the output stream is destroyed.
 *
 * Returns: a new #xoutput_stream_t
 **/
xoutput_stream_t *
g_unix_output_stream_new (xint_t     fd,
			  xboolean_t close_fd)
{
  GUnixOutputStream *stream;

  g_return_val_if_fail (fd != -1, NULL);

  stream = xobject_new (XTYPE_UNIX_OUTPUT_STREAM,
			 "fd", fd,
			 "close-fd", close_fd,
			 NULL);

  return G_OUTPUT_STREAM (stream);
}

/**
 * g_unix_output_stream_set_close_fd:
 * @stream: a #GUnixOutputStream
 * @close_fd: %TRUE to close the file descriptor when done
 *
 * Sets whether the file descriptor of @stream shall be closed
 * when the stream is closed.
 *
 * Since: 2.20
 */
void
g_unix_output_stream_set_close_fd (GUnixOutputStream *stream,
                                   xboolean_t           close_fd)
{
  g_return_if_fail (X_IS_UNIX_OUTPUT_STREAM (stream));

  close_fd = close_fd != FALSE;
  if (stream->priv->close_fd != close_fd)
    {
      stream->priv->close_fd = close_fd;
      xobject_notify (G_OBJECT (stream), "close-fd");
    }
}

/**
 * g_unix_output_stream_get_close_fd:
 * @stream: a #GUnixOutputStream
 *
 * Returns whether the file descriptor of @stream will be
 * closed when the stream is closed.
 *
 * Returns: %TRUE if the file descriptor is closed when done
 *
 * Since: 2.20
 */
xboolean_t
g_unix_output_stream_get_close_fd (GUnixOutputStream *stream)
{
  g_return_val_if_fail (X_IS_UNIX_OUTPUT_STREAM (stream), FALSE);

  return stream->priv->close_fd;
}

/**
 * g_unix_output_stream_get_fd:
 * @stream: a #GUnixOutputStream
 *
 * Return the UNIX file descriptor that the stream writes to.
 *
 * Returns: The file descriptor of @stream
 *
 * Since: 2.20
 */
xint_t
g_unix_output_stream_get_fd (GUnixOutputStream *stream)
{
  g_return_val_if_fail (X_IS_UNIX_OUTPUT_STREAM (stream), -1);

  return stream->priv->fd;
}

static xssize_t
g_unix_output_stream_write (xoutput_stream_t  *stream,
			    const void     *buffer,
			    xsize_t           count,
			    xcancellable_t   *cancellable,
			    xerror_t        **error)
{
  GUnixOutputStream *unix_stream;
  xssize_t res = -1;
  xpollfd_t poll_fds[2];
  int nfds = 0;
  int poll_ret;

  unix_stream = G_UNIX_OUTPUT_STREAM (stream);

  poll_fds[0].fd = unix_stream->priv->fd;
  poll_fds[0].events = G_IO_OUT;
  nfds++;

  if (unix_stream->priv->can_poll &&
      xcancellable_make_pollfd (cancellable, &poll_fds[1]))
    nfds++;

  while (1)
    {
      int errsv;

      poll_fds[0].revents = poll_fds[1].revents = 0;
      do
        {
          poll_ret = g_poll (poll_fds, nfds, -1);
          errsv = errno;
        }
      while (poll_ret == -1 && errsv == EINTR);

      if (poll_ret == -1)
	{
	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to file descriptor: %s"),
		       xstrerror (errsv));
	  break;
	}

      if (xcancellable_set_error_if_cancelled (cancellable, error))
	break;

      if (!poll_fds[0].revents)
	continue;

      res = write (unix_stream->priv->fd, buffer, count);
      errsv = errno;
      if (res == -1)
	{
	  if (errsv == EINTR || errsv == EAGAIN)
	    continue;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to file descriptor: %s"),
		       xstrerror (errsv));
	}

      break;
    }

  if (nfds == 2)
    xcancellable_release_fd (cancellable);
  return res;
}

/* Macro to check if struct iovec and xoutput_vector_t have the same ABI */
#define G_OUTPUT_VECTOR_IS_IOVEC (sizeof (struct iovec) == sizeof (xoutput_vector_t) && \
      G_SIZEOF_MEMBER (struct iovec, iov_base) == G_SIZEOF_MEMBER (xoutput_vector_t, buffer) && \
      G_STRUCT_OFFSET (struct iovec, iov_base) == G_STRUCT_OFFSET (xoutput_vector_t, buffer) && \
      G_SIZEOF_MEMBER (struct iovec, iov_len) == G_SIZEOF_MEMBER (xoutput_vector_t, size) && \
      G_STRUCT_OFFSET (struct iovec, iov_len) == G_STRUCT_OFFSET (xoutput_vector_t, size))

static xboolean_t
g_unix_output_stream_writev (xoutput_stream_t        *stream,
			     const xoutput_vector_t  *vectors,
			     xsize_t                 n_vectors,
			     xsize_t                *bytes_written,
			     xcancellable_t         *cancellable,
			     xerror_t              **error)
{
  GUnixOutputStream *unix_stream;
  xssize_t res = -1;
  xpollfd_t poll_fds[2];
  int nfds = 0;
  int poll_ret;
  struct iovec *iov;

  if (bytes_written)
    *bytes_written = 0;

  /* Clamp the number of vectors if more given than we can write in one go.
   * The caller has to handle short writes anyway.
   */
  if (n_vectors > G_IOV_MAX)
    n_vectors = G_IOV_MAX;

  unix_stream = G_UNIX_OUTPUT_STREAM (stream);

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

  poll_fds[0].fd = unix_stream->priv->fd;
  poll_fds[0].events = G_IO_OUT;
  nfds++;

  if (unix_stream->priv->can_poll &&
      xcancellable_make_pollfd (cancellable, &poll_fds[1]))
    nfds++;

  while (1)
    {
      int errsv;

      poll_fds[0].revents = poll_fds[1].revents = 0;
      do
        {
          poll_ret = g_poll (poll_fds, nfds, -1);
          errsv = errno;
        }
      while (poll_ret == -1 && errsv == EINTR);

      if (poll_ret == -1)
	{
	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to file descriptor: %s"),
		       xstrerror (errsv));
	  break;
	}

      if (xcancellable_set_error_if_cancelled (cancellable, error))
	break;

      if (!poll_fds[0].revents)
	continue;

      res = writev (unix_stream->priv->fd, iov, n_vectors);
      errsv = errno;
      if (res == -1)
	{
	  if (errsv == EINTR || errsv == EAGAIN)
	    continue;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to file descriptor: %s"),
		       xstrerror (errsv));
	}

      if (bytes_written)
        *bytes_written = res;

      break;
    }

  if (nfds == 2)
    xcancellable_release_fd (cancellable);
  return res != -1;
}

static xboolean_t
g_unix_output_stream_close (xoutput_stream_t  *stream,
			    xcancellable_t   *cancellable,
			    xerror_t        **error)
{
  GUnixOutputStream *unix_stream;
  int res;

  unix_stream = G_UNIX_OUTPUT_STREAM (stream);

  if (!unix_stream->priv->close_fd)
    return TRUE;

  /* This might block during the close. Doesn't seem to be a way to avoid it though. */
  res = close (unix_stream->priv->fd);
  if (res == -1)
    {
      int errsv = errno;

      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errsv),
		   _("Error closing file descriptor: %s"),
		   xstrerror (errsv));
    }

  return res != -1;
}

static xboolean_t
g_unix_output_stream_pollable_can_poll (xpollable_output_stream_t *stream)
{
  return G_UNIX_OUTPUT_STREAM (stream)->priv->can_poll;
}

static xboolean_t
g_unix_output_stream_pollable_is_writable (xpollable_output_stream_t *stream)
{
  GUnixOutputStream *unix_stream = G_UNIX_OUTPUT_STREAM (stream);
  xpollfd_t poll_fd;
  xint_t result;

  poll_fd.fd = unix_stream->priv->fd;
  poll_fd.events = G_IO_OUT;
  poll_fd.revents = 0;

  do
    result = g_poll (&poll_fd, 1, 0);
  while (result == -1 && errno == EINTR);

  return poll_fd.revents != 0;
}

static xsource_t *
g_unix_output_stream_pollable_create_source (xpollable_output_stream_t *stream,
					     xcancellable_t          *cancellable)
{
  GUnixOutputStream *unix_stream = G_UNIX_OUTPUT_STREAM (stream);
  xsource_t *inner_source, *cancellable_source, *pollable_source;

  pollable_source = g_pollable_source_new (G_OBJECT (stream));

  inner_source = g_unix_fd_source_new (unix_stream->priv->fd, G_IO_OUT);
  xsource_set_dummy_callback (inner_source);
  xsource_add_child_source (pollable_source, inner_source);
  xsource_unref (inner_source);

  if (cancellable)
    {
      cancellable_source = xcancellable_source_new (cancellable);
      xsource_set_dummy_callback (cancellable_source);
      xsource_add_child_source (pollable_source, cancellable_source);
      xsource_unref (cancellable_source);
    }

  return pollable_source;
}

static GPollableReturn
g_unix_output_stream_pollable_writev_nonblocking (xpollable_output_stream_t  *stream,
						  const xoutput_vector_t    *vectors,
						  xsize_t                   n_vectors,
						  xsize_t                  *bytes_written,
						  xerror_t                **error)
{
  GUnixOutputStream *unix_stream = G_UNIX_OUTPUT_STREAM (stream);
  struct iovec *iov;
  xssize_t res = -1;

  if (!xpollable_output_stream_is_writable (stream))
    {
      *bytes_written = 0;
      return G_POLLABLE_RETURN_WOULD_BLOCK;
    }

  /* Clamp the number of vectors if more given than we can write in one go.
   * The caller has to handle short writes anyway.
   */
  if (n_vectors > G_IOV_MAX)
    n_vectors = G_IOV_MAX;

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
      int errsv;

      res = writev (unix_stream->priv->fd, iov, n_vectors);
      errsv = errno;
      if (res == -1)
	{
	  if (errsv == EINTR)
	    continue;

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errsv),
		       _("Error writing to file descriptor: %s"),
		       xstrerror (errsv));
	}

      if (bytes_written)
        *bytes_written = res;

      break;
    }

  return res != -1 ? G_POLLABLE_RETURN_OK : G_POLLABLE_RETURN_FAILED;
}
