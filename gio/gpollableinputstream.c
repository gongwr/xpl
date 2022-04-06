/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#include <errno.h>

#include "gpollableinputstream.h"
#include "gasynchelper.h"
#include "glibintl.h"

/**
 * SECTION:gpollableinputstream
 * @short_description: Interface for pollable input streams
 * @include: gio/gio.h
 * @see_also: #xinput_stream_t, #xpollable_output_stream_t, #xfile_descriptor_based_t
 *
 * #xpollable_input_stream_t is implemented by #GInputStreams that
 * can be polled for readiness to read. This can be used when
 * interfacing with a non-GIO API that expects
 * UNIX-file-descriptor-style asynchronous I/O rather than GIO-style.
 *
 * Since: 2.28
 */

G_DEFINE_INTERFACE (xpollable_input_stream, g_pollable_input_stream, XTYPE_INPUT_STREAM)

static xboolean_t g_pollable_input_stream_default_can_poll         (xpollable_input_stream_t *stream);
static xssize_t   g_pollable_input_stream_default_read_nonblocking (xpollable_input_stream_t  *stream,
								  void                  *buffer,
								  xsize_t                  count,
								  xerror_t               **error);

static void
g_pollable_input_stream_default_init (GPollableInputStreamInterface *iface)
{
  iface->can_poll         = g_pollable_input_stream_default_can_poll;
  iface->read_nonblocking = g_pollable_input_stream_default_read_nonblocking;
}

static xboolean_t
g_pollable_input_stream_default_can_poll (xpollable_input_stream_t *stream)
{
  return TRUE;
}

/**
 * g_pollable_input_stream_can_poll:
 * @stream: a #xpollable_input_stream_t.
 *
 * Checks if @stream is actually pollable. Some classes may implement
 * #xpollable_input_stream_t but have only certain instances of that class
 * be pollable. If this method returns %FALSE, then the behavior of
 * other #xpollable_input_stream_t methods is undefined.
 *
 * For any given stream, the value returned by this method is constant;
 * a stream cannot switch from pollable to non-pollable or vice versa.
 *
 * Returns: %TRUE if @stream is pollable, %FALSE if not.
 *
 * Since: 2.28
 */
xboolean_t
g_pollable_input_stream_can_poll (xpollable_input_stream_t *stream)
{
  g_return_val_if_fail (X_IS_POLLABLE_INPUT_STREAM (stream), FALSE);

  return G_POLLABLE_INPUT_STREAM_GET_INTERFACE (stream)->can_poll (stream);
}

/**
 * g_pollable_input_stream_is_readable:
 * @stream: a #xpollable_input_stream_t.
 *
 * Checks if @stream can be read.
 *
 * Note that some stream types may not be able to implement this 100%
 * reliably, and it is possible that a call to xinput_stream_read()
 * after this returns %TRUE would still block. To guarantee
 * non-blocking behavior, you should always use
 * g_pollable_input_stream_read_nonblocking(), which will return a
 * %G_IO_ERROR_WOULD_BLOCK error rather than blocking.
 *
 * Returns: %TRUE if @stream is readable, %FALSE if not. If an error
 *   has occurred on @stream, this will result in
 *   g_pollable_input_stream_is_readable() returning %TRUE, and the
 *   next attempt to read will return the error.
 *
 * Since: 2.28
 */
xboolean_t
g_pollable_input_stream_is_readable (xpollable_input_stream_t *stream)
{
  g_return_val_if_fail (X_IS_POLLABLE_INPUT_STREAM (stream), FALSE);

  return G_POLLABLE_INPUT_STREAM_GET_INTERFACE (stream)->is_readable (stream);
}

/**
 * g_pollable_input_stream_create_source:
 * @stream: a #xpollable_input_stream_t.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 *
 * Creates a #xsource_t that triggers when @stream can be read, or
 * @cancellable is triggered or an error occurs. The callback on the
 * source is of the #xpollable_source_func_t type.
 *
 * As with g_pollable_input_stream_is_readable(), it is possible that
 * the stream may not actually be readable even after the source
 * triggers, so you should use g_pollable_input_stream_read_nonblocking()
 * rather than xinput_stream_read() from the callback.
 *
 * Returns: (transfer full): a new #xsource_t
 *
 * Since: 2.28
 */
xsource_t *
g_pollable_input_stream_create_source (xpollable_input_stream_t *stream,
				       xcancellable_t         *cancellable)
{
  g_return_val_if_fail (X_IS_POLLABLE_INPUT_STREAM (stream), NULL);

  return G_POLLABLE_INPUT_STREAM_GET_INTERFACE (stream)->
	  create_source (stream, cancellable);
}

static xssize_t
g_pollable_input_stream_default_read_nonblocking (xpollable_input_stream_t  *stream,
						  void                  *buffer,
						  xsize_t                  count,
						  xerror_t               **error)
{
  if (!g_pollable_input_stream_is_readable (stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK,
                           xstrerror (EAGAIN));
      return -1;
    }

  return G_INPUT_STREAM_GET_CLASS (stream)->
    read_fn (G_INPUT_STREAM (stream), buffer, count, NULL, error);
}

/**
 * g_pollable_input_stream_read_nonblocking:
 * @stream: a #xpollable_input_stream_t
 * @buffer: (array length=count) (element-type xuint8_t) (out caller-allocates): a
 *     buffer to read data into (which should be at least @count bytes long).
 * @count: (in): the number of bytes you want to read
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Attempts to read up to @count bytes from @stream into @buffer, as
 * with xinput_stream_read(). If @stream is not currently readable,
 * this will immediately return %G_IO_ERROR_WOULD_BLOCK, and you can
 * use g_pollable_input_stream_create_source() to create a #xsource_t
 * that will be triggered when @stream is readable.
 *
 * Note that since this method never blocks, you cannot actually
 * use @cancellable to cancel it. However, it will return an error
 * if @cancellable has already been cancelled when you call, which
 * may happen if you call this method after a source triggers due
 * to having been cancelled.
 *
 * Virtual: read_nonblocking
 * Returns: the number of bytes read, or -1 on error (including
 *   %G_IO_ERROR_WOULD_BLOCK).
 */
xssize_t
g_pollable_input_stream_read_nonblocking (xpollable_input_stream_t  *stream,
					  void                  *buffer,
					  xsize_t                  count,
					  xcancellable_t          *cancellable,
					  xerror_t               **error)
{
  xssize_t res;

  g_return_val_if_fail (X_IS_POLLABLE_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (buffer != NULL, 0);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return -1;

  if (count == 0)
    return 0;

  if (((xssize_t) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = G_POLLABLE_INPUT_STREAM_GET_INTERFACE (stream)->
    read_nonblocking (stream, buffer, count, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  return res;
}
