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

#include "gpollableoutputstream.h"
#include "gasynchelper.h"
#include "gfiledescriptorbased.h"
#include "glibintl.h"

/**
 * SECTION:gpollableoutputstream
 * @short_description: Interface for pollable output streams
 * @include: gio/gio.h
 * @see_also: #xoutput_stream_t, #xfile_descriptor_based_t, #xpollable_input_stream_t
 *
 * #xpollable_output_stream_t is implemented by #GOutputStreams that
 * can be polled for readiness to write. This can be used when
 * interfacing with a non-GIO API that expects
 * UNIX-file-descriptor-style asynchronous I/O rather than GIO-style.
 *
 * Since: 2.28
 */

G_DEFINE_INTERFACE (xpollable_output_stream, xpollable_output_stream, XTYPE_OUTPUT_STREAM)

static xboolean_t xpollable_output_stream_default_can_poll           (xpollable_output_stream_t *stream);
static xssize_t   xpollable_output_stream_default_write_nonblocking  (xpollable_output_stream_t  *stream,
								     const void             *buffer,
								     xsize_t                   count,
								     xerror_t                **error);
static GPollableReturn xpollable_output_stream_default_writev_nonblocking (xpollable_output_stream_t  *stream,
									    const xoutput_vector_t    *vectors,
									    xsize_t                   n_vectors,
									    xsize_t                  *bytes_written,
									    xerror_t                **error);

static void
xpollable_output_stream_default_init (xpollable_output_stream_interface_t *iface)
{
  iface->can_poll           = xpollable_output_stream_default_can_poll;
  iface->write_nonblocking  = xpollable_output_stream_default_write_nonblocking;
  iface->writev_nonblocking = xpollable_output_stream_default_writev_nonblocking;
}

static xboolean_t
xpollable_output_stream_default_can_poll (xpollable_output_stream_t *stream)
{
  return TRUE;
}

/**
 * xpollable_output_stream_can_poll:
 * @stream: a #xpollable_output_stream_t.
 *
 * Checks if @stream is actually pollable. Some classes may implement
 * #xpollable_output_stream_t but have only certain instances of that
 * class be pollable. If this method returns %FALSE, then the behavior
 * of other #xpollable_output_stream_t methods is undefined.
 *
 * For any given stream, the value returned by this method is constant;
 * a stream cannot switch from pollable to non-pollable or vice versa.
 *
 * Returns: %TRUE if @stream is pollable, %FALSE if not.
 *
 * Since: 2.28
 */
xboolean_t
xpollable_output_stream_can_poll (xpollable_output_stream_t *stream)
{
  g_return_val_if_fail (X_IS_POLLABLE_OUTPUT_STREAM (stream), FALSE);

  return G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream)->can_poll (stream);
}

/**
 * xpollable_output_stream_is_writable:
 * @stream: a #xpollable_output_stream_t.
 *
 * Checks if @stream can be written.
 *
 * Note that some stream types may not be able to implement this 100%
 * reliably, and it is possible that a call to xoutput_stream_write()
 * after this returns %TRUE would still block. To guarantee
 * non-blocking behavior, you should always use
 * xpollable_output_stream_write_nonblocking(), which will return a
 * %G_IO_ERROR_WOULD_BLOCK error rather than blocking.
 *
 * Returns: %TRUE if @stream is writable, %FALSE if not. If an error
 *   has occurred on @stream, this will result in
 *   xpollable_output_stream_is_writable() returning %TRUE, and the
 *   next attempt to write will return the error.
 *
 * Since: 2.28
 */
xboolean_t
xpollable_output_stream_is_writable (xpollable_output_stream_t *stream)
{
  g_return_val_if_fail (X_IS_POLLABLE_OUTPUT_STREAM (stream), FALSE);

  return G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream)->is_writable (stream);
}

/**
 * xpollable_output_stream_create_source:
 * @stream: a #xpollable_output_stream_t.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 *
 * Creates a #xsource_t that triggers when @stream can be written, or
 * @cancellable is triggered or an error occurs. The callback on the
 * source is of the #xpollable_source_func_t type.
 *
 * As with xpollable_output_stream_is_writable(), it is possible that
 * the stream may not actually be writable even after the source
 * triggers, so you should use xpollable_output_stream_write_nonblocking()
 * rather than xoutput_stream_write() from the callback.
 *
 * Returns: (transfer full): a new #xsource_t
 *
 * Since: 2.28
 */
xsource_t *
xpollable_output_stream_create_source (xpollable_output_stream_t *stream,
					xcancellable_t          *cancellable)
{
  g_return_val_if_fail (X_IS_POLLABLE_OUTPUT_STREAM (stream), NULL);

  return G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream)->
	  create_source (stream, cancellable);
}

static xssize_t
xpollable_output_stream_default_write_nonblocking (xpollable_output_stream_t  *stream,
						    const void             *buffer,
						    xsize_t                   count,
						    xerror_t                **error)
{
  if (!xpollable_output_stream_is_writable (stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK,
                           xstrerror (EAGAIN));
      return -1;
    }

  return G_OUTPUT_STREAM_GET_CLASS (stream)->
    write_fn (G_OUTPUT_STREAM (stream), buffer, count, NULL, error);
}

static GPollableReturn
xpollable_output_stream_default_writev_nonblocking (xpollable_output_stream_t  *stream,
						     const xoutput_vector_t    *vectors,
						     xsize_t                   n_vectors,
						     xsize_t                  *bytes_written,
						     xerror_t                **error)
{
  xsize_t _bytes_written = 0;
  xpollable_output_stream_interface_t *iface = G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream);
  xsize_t i;
  xerror_t *err = NULL;

  for (i = 0; i < n_vectors; i++)
    {
      xssize_t res;

      /* Would we overflow here? In that case simply return and let the caller
       * handle this like a short write */
      if (_bytes_written > G_MAXSIZE - vectors[i].size)
        break;

      res = iface->write_nonblocking (stream, vectors[i].buffer, vectors[i].size, &err);
      if (res == -1)
        {
          if (bytes_written)
            *bytes_written = _bytes_written;

          /* If something was written already we handle this like a short
           * write and assume that the next call would either give the same
           * error again or successfully finish writing without errors or data
           * loss
           */
          if (_bytes_written > 0)
            {
              g_clear_error (&err);
              return G_POLLABLE_RETURN_OK;
            }
          else if (xerror_matches (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
            {
              g_clear_error (&err);
              return G_POLLABLE_RETURN_WOULD_BLOCK;
            }
          else
            {
              g_propagate_error (error, err);
              return G_POLLABLE_RETURN_FAILED;
            }
        }

      _bytes_written += res;
      /* if we had a short write break the loop here */
      if ((xsize_t) res < vectors[i].size)
        break;
    }

  if (bytes_written)
    *bytes_written = _bytes_written;

  return G_POLLABLE_RETURN_OK;
}

/**
 * xpollable_output_stream_write_nonblocking:
 * @stream: a #xpollable_output_stream_t
 * @buffer: (array length=count) (element-type xuint8_t): a buffer to write
 *     data from
 * @count: the number of bytes you want to write
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Attempts to write up to @count bytes from @buffer to @stream, as
 * with xoutput_stream_write(). If @stream is not currently writable,
 * this will immediately return %G_IO_ERROR_WOULD_BLOCK, and you can
 * use xpollable_output_stream_create_source() to create a #xsource_t
 * that will be triggered when @stream is writable.
 *
 * Note that since this method never blocks, you cannot actually
 * use @cancellable to cancel it. However, it will return an error
 * if @cancellable has already been cancelled when you call, which
 * may happen if you call this method after a source triggers due
 * to having been cancelled.
 *
 * Also note that if %G_IO_ERROR_WOULD_BLOCK is returned some underlying
 * transports like D/TLS require that you re-send the same @buffer and
 * @count in the next write call.
 *
 * Virtual: write_nonblocking
 * Returns: the number of bytes written, or -1 on error (including
 *   %G_IO_ERROR_WOULD_BLOCK).
 */
xssize_t
xpollable_output_stream_write_nonblocking (xpollable_output_stream_t  *stream,
					    const void             *buffer,
					    xsize_t                   count,
					    xcancellable_t           *cancellable,
					    xerror_t                **error)
{
  xssize_t res;

  g_return_val_if_fail (X_IS_POLLABLE_OUTPUT_STREAM (stream), -1);
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

  res = G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream)->
    write_nonblocking (stream, buffer, count, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  return res;
}

/**
 * xpollable_output_stream_writev_nonblocking:
 * @stream: a #xpollable_output_stream_t
 * @vectors: (array length=n_vectors): the buffer containing the #GOutputVectors to write.
 * @n_vectors: the number of vectors to write
 * @bytes_written: (out) (optional): location to store the number of bytes that were
 *     written to the stream
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Attempts to write the bytes contained in the @n_vectors @vectors to @stream,
 * as with xoutput_stream_writev(). If @stream is not currently writable,
 * this will immediately return %@G_POLLABLE_RETURN_WOULD_BLOCK, and you can
 * use xpollable_output_stream_create_source() to create a #xsource_t
 * that will be triggered when @stream is writable. @error will *not* be
 * set in that case.
 *
 * Note that since this method never blocks, you cannot actually
 * use @cancellable to cancel it. However, it will return an error
 * if @cancellable has already been cancelled when you call, which
 * may happen if you call this method after a source triggers due
 * to having been cancelled.
 *
 * Also note that if %G_POLLABLE_RETURN_WOULD_BLOCK is returned some underlying
 * transports like D/TLS require that you re-send the same @vectors and
 * @n_vectors in the next write call.
 *
 * Virtual: writev_nonblocking
 *
 * Returns: %@G_POLLABLE_RETURN_OK on success, %G_POLLABLE_RETURN_WOULD_BLOCK
 * if the stream is not currently writable (and @error is *not* set), or
 * %G_POLLABLE_RETURN_FAILED if there was an error in which case @error will
 * be set.
 *
 * Since: 2.60
 */
GPollableReturn
xpollable_output_stream_writev_nonblocking (xpollable_output_stream_t  *stream,
					     const xoutput_vector_t    *vectors,
					     xsize_t                   n_vectors,
					     xsize_t                  *bytes_written,
					     xcancellable_t           *cancellable,
					     xerror_t                **error)
{
  xpollable_output_stream_interface_t *iface;
  GPollableReturn res;
  xsize_t _bytes_written = 0;

  if (bytes_written)
    *bytes_written = 0;

  g_return_val_if_fail (X_IS_POLLABLE_OUTPUT_STREAM (stream), G_POLLABLE_RETURN_FAILED);
  g_return_val_if_fail (vectors != NULL || n_vectors == 0, G_POLLABLE_RETURN_FAILED);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), G_POLLABLE_RETURN_FAILED);
  g_return_val_if_fail (error == NULL || *error == NULL, G_POLLABLE_RETURN_FAILED);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return G_POLLABLE_RETURN_FAILED;

  if (n_vectors == 0)
    return G_POLLABLE_RETURN_OK;

  iface = G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream);
  g_return_val_if_fail (iface->writev_nonblocking != NULL, G_POLLABLE_RETURN_FAILED);

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = iface->
    writev_nonblocking (stream, vectors, n_vectors, &_bytes_written, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  if (res == G_POLLABLE_RETURN_FAILED)
    g_warn_if_fail (error == NULL || (*error != NULL && !xerror_matches (*error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK)));
  else if (res == G_POLLABLE_RETURN_WOULD_BLOCK)
    g_warn_if_fail (error == NULL || *error == NULL);

  /* in case of not-OK nothing must've been written */
  g_warn_if_fail (res == G_POLLABLE_RETURN_OK || _bytes_written == 0);

  if (bytes_written)
    *bytes_written = _bytes_written;

  return res;
}
