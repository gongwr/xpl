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
#include <string.h>
#include "goutputstream.h"
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "ginputstream.h"
#include "gioerror.h"
#include "gioprivate.h"
#include "glibintl.h"
#include "gpollableoutputstream.h"

/**
 * SECTION:goutputstream
 * @short_description: Base class for implementing streaming output
 * @include: gio/gio.h
 *
 * #xoutput_stream_t has functions to write to a stream (xoutput_stream_write()),
 * to close a stream (xoutput_stream_close()) and to flush pending writes
 * (xoutput_stream_flush()).
 *
 * To copy the content of an input stream to an output stream without
 * manually handling the reads and writes, use xoutput_stream_splice().
 *
 * See the documentation for #xio_stream_t for details of thread safety of
 * streaming APIs.
 *
 * All of these functions have async variants too.
 **/

struct _xoutput_stream_private {
  xuint_t closed : 1;
  xuint_t pending : 1;
  xuint_t closing : 1;
  xasync_ready_callback_t outstanding_callback;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xoutput_stream, xoutput_stream, XTYPE_OBJECT)

static xssize_t   xoutput_stream_real_splice        (xoutput_stream_t             *stream,
						    xinput_stream_t              *source,
						    GOutputStreamSpliceFlags   flags,
						    xcancellable_t              *cancellable,
						    xerror_t                   **error);
static void     xoutput_stream_real_write_async   (xoutput_stream_t             *stream,
						    const void                *buffer,
						    xsize_t                      count,
						    int                        io_priority,
						    xcancellable_t              *cancellable,
						    xasync_ready_callback_t        callback,
						    xpointer_t                   data);
static xssize_t   xoutput_stream_real_write_finish  (xoutput_stream_t             *stream,
						    xasync_result_t              *result,
						    xerror_t                   **error);
static xboolean_t xoutput_stream_real_writev        (xoutput_stream_t             *stream,
						    const xoutput_vector_t       *vectors,
						    xsize_t                      n_vectors,
						    xsize_t                     *bytes_written,
						    xcancellable_t              *cancellable,
						    xerror_t                   **error);
static void     xoutput_stream_real_writev_async  (xoutput_stream_t             *stream,
						    const xoutput_vector_t       *vectors,
						    xsize_t                      n_vectors,
						    int                        io_priority,
						    xcancellable_t              *cancellable,
						    xasync_ready_callback_t        callback,
						    xpointer_t                   data);
static xboolean_t xoutput_stream_real_writev_finish (xoutput_stream_t             *stream,
						    xasync_result_t              *result,
						    xsize_t                     *bytes_written,
						    xerror_t                   **error);
static void     xoutput_stream_real_splice_async  (xoutput_stream_t             *stream,
						    xinput_stream_t              *source,
						    GOutputStreamSpliceFlags   flags,
						    int                        io_priority,
						    xcancellable_t              *cancellable,
						    xasync_ready_callback_t        callback,
						    xpointer_t                   data);
static xssize_t   xoutput_stream_real_splice_finish (xoutput_stream_t             *stream,
						    xasync_result_t              *result,
						    xerror_t                   **error);
static void     xoutput_stream_real_flush_async   (xoutput_stream_t             *stream,
						    int                        io_priority,
						    xcancellable_t              *cancellable,
						    xasync_ready_callback_t        callback,
						    xpointer_t                   data);
static xboolean_t xoutput_stream_real_flush_finish  (xoutput_stream_t             *stream,
						    xasync_result_t              *result,
						    xerror_t                   **error);
static void     xoutput_stream_real_close_async   (xoutput_stream_t             *stream,
						    int                        io_priority,
						    xcancellable_t              *cancellable,
						    xasync_ready_callback_t        callback,
						    xpointer_t                   data);
static xboolean_t xoutput_stream_real_close_finish  (xoutput_stream_t             *stream,
						    xasync_result_t              *result,
						    xerror_t                   **error);
static xboolean_t xoutput_stream_internal_close     (xoutput_stream_t             *stream,
                                                    xcancellable_t              *cancellable,
                                                    xerror_t                   **error);
static void     xoutput_stream_internal_close_async (xoutput_stream_t           *stream,
                                                      int                      io_priority,
                                                      xcancellable_t            *cancellable,
                                                      xasync_ready_callback_t      callback,
                                                      xpointer_t                 data);
static xboolean_t xoutput_stream_internal_close_finish (xoutput_stream_t          *stream,
                                                       xasync_result_t           *result,
                                                       xerror_t                **error);

static void
xoutput_stream_dispose (xobject_t *object)
{
  xoutput_stream_t *stream;

  stream = G_OUTPUT_STREAM (object);

  if (!stream->priv->closed)
    xoutput_stream_close (stream, NULL, NULL);

  G_OBJECT_CLASS (xoutput_stream_parent_class)->dispose (object);
}

static void
xoutput_stream_class_init (xoutput_stream_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = xoutput_stream_dispose;

  klass->splice = xoutput_stream_real_splice;

  klass->write_async = xoutput_stream_real_write_async;
  klass->write_finish = xoutput_stream_real_write_finish;
  klass->writev_fn = xoutput_stream_real_writev;
  klass->writev_async = xoutput_stream_real_writev_async;
  klass->writev_finish = xoutput_stream_real_writev_finish;
  klass->splice_async = xoutput_stream_real_splice_async;
  klass->splice_finish = xoutput_stream_real_splice_finish;
  klass->flush_async = xoutput_stream_real_flush_async;
  klass->flush_finish = xoutput_stream_real_flush_finish;
  klass->close_async = xoutput_stream_real_close_async;
  klass->close_finish = xoutput_stream_real_close_finish;
}

static void
xoutput_stream_init (xoutput_stream_t *stream)
{
  stream->priv = xoutput_stream_get_instance_private (stream);
}

/**
 * xoutput_stream_write:
 * @stream: a #xoutput_stream_t.
 * @buffer: (array length=count) (element-type xuint8_t): the buffer containing the data to write.
 * @count: the number of bytes to write
 * @cancellable: (nullable): optional cancellable object
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to write @count bytes from @buffer into the stream. Will block
 * during the operation.
 *
 * If count is 0, returns 0 and does nothing. A value of @count
 * larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes written to the stream is returned.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. on a partial I/O error, or if there is not enough
 * storage in the stream. All writes block until at least one byte
 * is written or an error occurs; 0 is never returned (unless
 * @count is 0).
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error -1 is returned and @error is set accordingly.
 *
 * Virtual: write_fn
 *
 * Returns: Number of bytes written, or -1 on error
 **/
xssize_t
xoutput_stream_write (xoutput_stream_t  *stream,
		       const void     *buffer,
		       xsize_t           count,
		       xcancellable_t   *cancellable,
		       xerror_t        **error)
{
  xoutput_stream_class_t *class;
  xssize_t res;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (buffer != NULL, 0);

  if (count == 0)
    return 0;

  if (((xssize_t) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (class->write_fn == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Output stream doesn’t implement write"));
      return -1;
    }

  if (!xoutput_stream_set_pending (stream, error))
    return -1;

  if (cancellable)
    xcancellable_push_current (cancellable);

  res = class->write_fn (stream, buffer, count, cancellable, error);

  if (cancellable)
    xcancellable_pop_current (cancellable);

  xoutput_stream_clear_pending (stream);

  return res;
}

/**
 * xoutput_stream_write_all:
 * @stream: a #xoutput_stream_t.
 * @buffer: (array length=count) (element-type xuint8_t): the buffer containing the data to write.
 * @count: the number of bytes to write
 * @bytes_written: (out) (optional): location to store the number of bytes that was
 *     written to the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to write @count bytes from @buffer into the stream. Will block
 * during the operation.
 *
 * This function is similar to xoutput_stream_write(), except it tries to
 * write as many bytes as requested, only stopping on an error.
 *
 * On a successful write of @count bytes, %TRUE is returned, and @bytes_written
 * is set to @count.
 *
 * If there is an error during the operation %FALSE is returned and @error
 * is set to indicate the error status.
 *
 * As a special exception to the normal conventions for functions that
 * use #xerror_t, if this function returns %FALSE (and sets @error) then
 * @bytes_written will be set to the number of bytes that were
 * successfully written before the error was encountered.  This
 * functionality is only available from C.  If you need it from another
 * language then you must write your own loop around
 * xoutput_stream_write().
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 **/
xboolean_t
xoutput_stream_write_all (xoutput_stream_t  *stream,
			   const void     *buffer,
			   xsize_t           count,
			   xsize_t          *bytes_written,
			   xcancellable_t   *cancellable,
			   xerror_t        **error)
{
  xsize_t _bytes_written;
  xssize_t res;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (buffer != NULL || count == 0, FALSE);

  _bytes_written = 0;
  while (_bytes_written < count)
    {
      res = xoutput_stream_write (stream, (char *)buffer + _bytes_written, count - _bytes_written,
				   cancellable, error);
      if (res == -1)
	{
	  if (bytes_written)
	    *bytes_written = _bytes_written;
	  return FALSE;
	}
      g_return_val_if_fail (res > 0, FALSE);

      _bytes_written += res;
    }

  if (bytes_written)
    *bytes_written = _bytes_written;

  return TRUE;
}

/**
 * xoutput_stream_writev:
 * @stream: a #xoutput_stream_t.
 * @vectors: (array length=n_vectors): the buffer containing the #GOutputVectors to write.
 * @n_vectors: the number of vectors to write
 * @bytes_written: (out) (optional): location to store the number of bytes that were
 *     written to the stream
 * @cancellable: (nullable): optional cancellable object
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to write the bytes contained in the @n_vectors @vectors into the
 * stream. Will block during the operation.
 *
 * If @n_vectors is 0 or the sum of all bytes in @vectors is 0, returns 0 and
 * does nothing.
 *
 * On success, the number of bytes written to the stream is returned.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. on a partial I/O error, or if there is not enough
 * storage in the stream. All writes block until at least one byte
 * is written or an error occurs; 0 is never returned (unless
 * @n_vectors is 0 or the sum of all bytes in @vectors is 0).
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * Some implementations of xoutput_stream_writev() may have limitations on the
 * aggregate buffer size, and will return %G_IO_ERROR_INVALID_ARGUMENT if these
 * are exceeded. For example, when writing to a local file on UNIX platforms,
 * the aggregate buffer size must not exceed %G_MAXSSIZE bytes.
 *
 * Virtual: writev_fn
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 *
 * Since: 2.60
 */
xboolean_t
xoutput_stream_writev (xoutput_stream_t        *stream,
		        const xoutput_vector_t  *vectors,
		        xsize_t                 n_vectors,
		        xsize_t                *bytes_written,
		        xcancellable_t         *cancellable,
		        xerror_t              **error)
{
  xoutput_stream_class_t *class;
  xboolean_t res;
  xsize_t _bytes_written = 0;

  if (bytes_written)
    *bytes_written = 0;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (vectors != NULL || n_vectors == 0, FALSE);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (n_vectors == 0)
    return TRUE;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  g_return_val_if_fail (class->writev_fn != NULL, FALSE);

  if (!xoutput_stream_set_pending (stream, error))
    return FALSE;

  if (cancellable)
    xcancellable_push_current (cancellable);

  res = class->writev_fn (stream, vectors, n_vectors, &_bytes_written, cancellable, error);

  g_warn_if_fail (res || _bytes_written == 0);
  g_warn_if_fail (res || (error == NULL || *error != NULL));

  if (cancellable)
    xcancellable_pop_current (cancellable);

  xoutput_stream_clear_pending (stream);

  if (bytes_written)
    *bytes_written = _bytes_written;

  return res;
}

/**
 * xoutput_stream_writev_all:
 * @stream: a #xoutput_stream_t.
 * @vectors: (array length=n_vectors): the buffer containing the #GOutputVectors to write.
 * @n_vectors: the number of vectors to write
 * @bytes_written: (out) (optional): location to store the number of bytes that were
 *     written to the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to write the bytes contained in the @n_vectors @vectors into the
 * stream. Will block during the operation.
 *
 * This function is similar to xoutput_stream_writev(), except it tries to
 * write as many bytes as requested, only stopping on an error.
 *
 * On a successful write of all @n_vectors vectors, %TRUE is returned, and
 * @bytes_written is set to the sum of all the sizes of @vectors.
 *
 * If there is an error during the operation %FALSE is returned and @error
 * is set to indicate the error status.
 *
 * As a special exception to the normal conventions for functions that
 * use #xerror_t, if this function returns %FALSE (and sets @error) then
 * @bytes_written will be set to the number of bytes that were
 * successfully written before the error was encountered.  This
 * functionality is only available from C. If you need it from another
 * language then you must write your own loop around
 * xoutput_stream_write().
 *
 * The content of the individual elements of @vectors might be changed by this
 * function.
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 *
 * Since: 2.60
 */
xboolean_t
xoutput_stream_writev_all (xoutput_stream_t  *stream,
			    xoutput_vector_t  *vectors,
			    xsize_t           n_vectors,
			    xsize_t          *bytes_written,
			    xcancellable_t   *cancellable,
			    xerror_t        **error)
{
  xsize_t _bytes_written = 0;
  xsize_t i, to_be_written = 0;

  if (bytes_written)
    *bytes_written = 0;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (vectors != NULL || n_vectors == 0, FALSE);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* We can't write more than G_MAXSIZE bytes overall, otherwise we
   * would overflow the bytes_written counter */
  for (i = 0; i < n_vectors; i++)
    {
       if (to_be_written > G_MAXSIZE - vectors[i].size)
         {
           g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                        _("Sum of vectors passed to %s too large"), G_STRFUNC);
           return FALSE;
         }
       to_be_written += vectors[i].size;
    }

  _bytes_written = 0;
  while (n_vectors > 0 && to_be_written > 0)
    {
      xsize_t n_written = 0;
      xboolean_t res;

      res = xoutput_stream_writev (stream, vectors, n_vectors, &n_written, cancellable, error);

      if (!res)
        {
          if (bytes_written)
            *bytes_written = _bytes_written;
          return FALSE;
        }

      g_return_val_if_fail (n_written > 0, FALSE);
      _bytes_written += n_written;

      /* skip vectors that have been written in full */
      while (n_vectors > 0 && n_written >= vectors[0].size)
        {
          n_written -= vectors[0].size;
          ++vectors;
          --n_vectors;
        }
      /* skip partially written vector data */
      if (n_written > 0 && n_vectors > 0)
        {
          vectors[0].size -= n_written;
          vectors[0].buffer = ((xuint8_t *) vectors[0].buffer) + n_written;
        }
    }

  if (bytes_written)
    *bytes_written = _bytes_written;

  return TRUE;
}

/**
 * xoutput_stream_printf:
 * @stream: a #xoutput_stream_t.
 * @bytes_written: (out) (optional): location to store the number of bytes that was
 *     written to the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 * @format: the format string. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * This is a utility function around xoutput_stream_write_all(). It
 * uses xstrdup_vprintf() to turn @format and @... into a string that
 * is then written to @stream.
 *
 * See the documentation of xoutput_stream_write_all() about the
 * behavior of the actual write operation.
 *
 * Note that partial writes cannot be properly checked with this
 * function due to the variable length of the written string, if you
 * need precise control over partial write failures, you need to
 * create you own printf()-like wrapper around xoutput_stream_write()
 * or xoutput_stream_write_all().
 *
 * Since: 2.40
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 **/
xboolean_t
xoutput_stream_printf (xoutput_stream_t  *stream,
                        xsize_t          *bytes_written,
                        xcancellable_t   *cancellable,
                        xerror_t        **error,
                        const xchar_t    *format,
                        ...)
{
  va_list  args;
  xboolean_t success;

  va_start (args, format);
  success = xoutput_stream_vprintf (stream, bytes_written, cancellable,
                                     error, format, args);
  va_end (args);

  return success;
}

/**
 * xoutput_stream_vprintf:
 * @stream: a #xoutput_stream_t.
 * @bytes_written: (out) (optional): location to store the number of bytes that was
 *     written to the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 * @format: the format string. See the printf() documentation
 * @args: the parameters to insert into the format string
 *
 * This is a utility function around xoutput_stream_write_all(). It
 * uses xstrdup_vprintf() to turn @format and @args into a string that
 * is then written to @stream.
 *
 * See the documentation of xoutput_stream_write_all() about the
 * behavior of the actual write operation.
 *
 * Note that partial writes cannot be properly checked with this
 * function due to the variable length of the written string, if you
 * need precise control over partial write failures, you need to
 * create you own printf()-like wrapper around xoutput_stream_write()
 * or xoutput_stream_write_all().
 *
 * Since: 2.40
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 **/
xboolean_t
xoutput_stream_vprintf (xoutput_stream_t  *stream,
                         xsize_t          *bytes_written,
                         xcancellable_t   *cancellable,
                         xerror_t        **error,
                         const xchar_t    *format,
                         va_list         args)
{
  xchar_t    *text;
  xboolean_t  success;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (format != NULL, FALSE);

  text = xstrdup_vprintf (format, args);
  success = xoutput_stream_write_all (stream,
                                       text, strlen (text),
                                       bytes_written, cancellable, error);
  g_free (text);

  return success;
}

/**
 * xoutput_stream_write_bytes:
 * @stream: a #xoutput_stream_t.
 * @bytes: the #xbytes_t to write
 * @cancellable: (nullable): optional cancellable object
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * A wrapper function for xoutput_stream_write() which takes a
 * #xbytes_t as input.  This can be more convenient for use by language
 * bindings or in other cases where the refcounted nature of #xbytes_t
 * is helpful over a bare pointer interface.
 *
 * However, note that this function may still perform partial writes,
 * just like xoutput_stream_write().  If that occurs, to continue
 * writing, you will need to create a new #xbytes_t containing just the
 * remaining bytes, using xbytes_new_from_bytes(). Passing the same
 * #xbytes_t instance multiple times potentially can result in duplicated
 * data in the output stream.
 *
 * Returns: Number of bytes written, or -1 on error
 **/
xssize_t
xoutput_stream_write_bytes (xoutput_stream_t  *stream,
			     xbytes_t         *bytes,
			     xcancellable_t   *cancellable,
			     xerror_t        **error)
{
  xsize_t size;
  xconstpointer data;

  data = xbytes_get_data (bytes, &size);

  return xoutput_stream_write (stream,
                                data, size,
				cancellable,
				error);
}

/**
 * xoutput_stream_flush:
 * @stream: a #xoutput_stream_t.
 * @cancellable: (nullable): optional cancellable object
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Forces a write of all user-space buffered data for the given
 * @stream. Will block during the operation. Closing the stream will
 * implicitly cause a flush.
 *
 * This function is optional for inherited classes.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE on success, %FALSE on error
 **/
xboolean_t
xoutput_stream_flush (xoutput_stream_t  *stream,
                       xcancellable_t   *cancellable,
                       xerror_t        **error)
{
  xoutput_stream_class_t *class;
  xboolean_t res;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  if (!xoutput_stream_set_pending (stream, error))
    return FALSE;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  res = TRUE;
  if (class->flush)
    {
      if (cancellable)
	xcancellable_push_current (cancellable);

      res = class->flush (stream, cancellable, error);

      if (cancellable)
	xcancellable_pop_current (cancellable);
    }

  xoutput_stream_clear_pending (stream);

  return res;
}

/**
 * xoutput_stream_splice:
 * @stream: a #xoutput_stream_t.
 * @source: a #xinput_stream_t.
 * @flags: a set of #GOutputStreamSpliceFlags.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Splices an input stream into an output stream.
 *
 * Returns: a #xssize_t containing the size of the data spliced, or
 *     -1 if an error occurred. Note that if the number of bytes
 *     spliced is greater than %G_MAXSSIZE, then that will be
 *     returned, and there is no way to determine the actual number
 *     of bytes spliced.
 **/
xssize_t
xoutput_stream_splice (xoutput_stream_t             *stream,
			xinput_stream_t              *source,
			GOutputStreamSpliceFlags   flags,
			xcancellable_t              *cancellable,
			xerror_t                   **error)
{
  xoutput_stream_class_t *class;
  xssize_t bytes_copied;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (X_IS_INPUT_STREAM (source), -1);

  if (xinput_stream_is_closed (source))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Source stream is already closed"));
      return -1;
    }

  if (!xoutput_stream_set_pending (stream, error))
    return -1;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (cancellable)
    xcancellable_push_current (cancellable);

  bytes_copied = class->splice (stream, source, flags, cancellable, error);

  if (cancellable)
    xcancellable_pop_current (cancellable);

  xoutput_stream_clear_pending (stream);

  return bytes_copied;
}

static xssize_t
xoutput_stream_real_splice (xoutput_stream_t             *stream,
                             xinput_stream_t              *source,
                             GOutputStreamSpliceFlags   flags,
                             xcancellable_t              *cancellable,
                             xerror_t                   **error)
{
  xoutput_stream_class_t *class = G_OUTPUT_STREAM_GET_CLASS (stream);
  xssize_t n_read, n_written;
  xsize_t bytes_copied;
  char buffer[8192], *p;
  xboolean_t res;

  bytes_copied = 0;
  if (class->write_fn == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Output stream doesn’t implement write"));
      res = FALSE;
      goto notsupported;
    }

  res = TRUE;
  do
    {
      n_read = xinput_stream_read (source, buffer, sizeof (buffer), cancellable, error);
      if (n_read == -1)
	{
	  res = FALSE;
	  break;
	}

      if (n_read == 0)
	break;

      p = buffer;
      while (n_read > 0)
	{
	  n_written = class->write_fn (stream, p, n_read, cancellable, error);
	  if (n_written == -1)
	    {
	      res = FALSE;
	      break;
	    }

	  p += n_written;
	  n_read -= n_written;
	  bytes_copied += n_written;
	}

      if (bytes_copied > G_MAXSSIZE)
	bytes_copied = G_MAXSSIZE;
    }
  while (res);

 notsupported:
  if (!res)
    error = NULL; /* Ignore further errors */

  if (flags & G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE)
    {
      /* Don't care about errors in source here */
      xinput_stream_close (source, cancellable, NULL);
    }

  if (flags & G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET)
    {
      /* But write errors on close are bad! */
      if (!xoutput_stream_internal_close (stream, cancellable, error))
        res = FALSE;
    }

  if (res)
    return bytes_copied;

  return -1;
}

/* Must always be called inside
 * xoutput_stream_set_pending()/xoutput_stream_clear_pending(). */
static xboolean_t
xoutput_stream_internal_close (xoutput_stream_t  *stream,
                                xcancellable_t   *cancellable,
                                xerror_t        **error)
{
  xoutput_stream_class_t *class;
  xboolean_t res;

  if (stream->priv->closed)
    return TRUE;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  stream->priv->closing = TRUE;

  if (cancellable)
    xcancellable_push_current (cancellable);

  if (class->flush)
    res = class->flush (stream, cancellable, error);
  else
    res = TRUE;

  if (!res)
    {
      /* flushing caused the error that we want to return,
       * but we still want to close the underlying stream if possible
       */
      if (class->close_fn)
        class->close_fn (stream, cancellable, NULL);
    }
  else
    {
      res = TRUE;
      if (class->close_fn)
        res = class->close_fn (stream, cancellable, error);
    }

  if (cancellable)
    xcancellable_pop_current (cancellable);

  stream->priv->closing = FALSE;
  stream->priv->closed = TRUE;

  return res;
}

/**
 * xoutput_stream_close:
 * @stream: A #xoutput_stream_t.
 * @cancellable: (nullable): optional cancellable object
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Closes the stream, releasing resources related to it.
 *
 * Once the stream is closed, all other operations will return %G_IO_ERROR_CLOSED.
 * Closing a stream multiple times will not return an error.
 *
 * Closing a stream will automatically flush any outstanding buffers in the
 * stream.
 *
 * Streams will be automatically closed when the last reference
 * is dropped, but you might want to call this function to make sure
 * resources are released as early as possible.
 *
 * Some streams might keep the backing store of the stream (e.g. a file descriptor)
 * open after the stream is closed. See the documentation for the individual
 * stream for details.
 *
 * On failure the first error that happened will be reported, but the close
 * operation will finish as much as possible. A stream that failed to
 * close will still return %G_IO_ERROR_CLOSED for all operations. Still, it
 * is important to check and report the error to the user, otherwise
 * there might be a loss of data as all data might not be written.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 * Cancelling a close will still leave the stream closed, but there some streams
 * can use a faster close that doesn't block to e.g. check errors. On
 * cancellation (as with any error) there is no guarantee that all written
 * data will reach the target.
 *
 * Returns: %TRUE on success, %FALSE on failure
 **/
xboolean_t
xoutput_stream_close (xoutput_stream_t  *stream,
		       xcancellable_t   *cancellable,
		       xerror_t        **error)
{
  xboolean_t res;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  if (stream->priv->closed)
    return TRUE;

  if (!xoutput_stream_set_pending (stream, error))
    return FALSE;

  res = xoutput_stream_internal_close (stream, cancellable, error);

  xoutput_stream_clear_pending (stream);

  return res;
}

static void
async_ready_write_callback_wrapper (xobject_t      *source_object,
                                    xasync_result_t *res,
                                    xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  xoutput_stream_class_t *class;
  xtask_t *task = user_data;
  xssize_t nwrote;
  xerror_t *error = NULL;

  xoutput_stream_clear_pending (stream);

  if (xasync_result_legacy_propagate_error (res, &error))
    nwrote = -1;
  else
    {
      class = G_OUTPUT_STREAM_GET_CLASS (stream);
      nwrote = class->write_finish (stream, res, &error);
    }

  if (nwrote >= 0)
    xtask_return_int (task, nwrote);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * xoutput_stream_write_async:
 * @stream: A #xoutput_stream_t.
 * @buffer: (array length=count) (element-type xuint8_t): the buffer containing the data to write.
 * @count: the number of bytes to write
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous write of @count bytes from @buffer into
 * the stream. When the operation is finished @callback will be called.
 * You can then call xoutput_stream_write_finish() to get the result of the
 * operation.
 *
 * During an async request no other sync and async calls are allowed,
 * and will result in %G_IO_ERROR_PENDING errors.
 *
 * A value of @count larger than %G_MAXSSIZE will cause a
 * %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes written will be passed to the
 * @callback. It is not an error if this is not the same as the
 * requested size, as it can happen e.g. on a partial I/O error,
 * but generally we try to write as many bytes as requested.
 *
 * You are guaranteed that this method will never fail with
 * %G_IO_ERROR_WOULD_BLOCK - if @stream can't accept more data, the
 * method will just wait until this changes.
 *
 * Any outstanding I/O request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * The asynchronous methods have a default fallback that uses threads
 * to implement asynchronicity, so they are optional for inheriting
 * classes. However, if you override one you must override all.
 *
 * For the synchronous, blocking version of this function, see
 * xoutput_stream_write().
 *
 * Note that no copy of @buffer will be made, so it must stay valid
 * until @callback is called. See xoutput_stream_write_bytes_async()
 * for a #xbytes_t version that will automatically hold a reference to
 * the contents (without copying) for the duration of the call.
 */
void
xoutput_stream_write_async (xoutput_stream_t       *stream,
			     const void          *buffer,
			     xsize_t                count,
			     int                  io_priority,
			     xcancellable_t        *cancellable,
			     xasync_ready_callback_t  callback,
			     xpointer_t             user_data)
{
  xoutput_stream_class_t *class;
  xerror_t *error = NULL;
  xtask_t *task;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (buffer != NULL);

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xoutput_stream_write_async);
  xtask_set_priority (task, io_priority);

  if (count == 0)
    {
      xtask_return_int (task, 0);
      xobject_unref (task);
      return;
    }

  if (((xssize_t) count) < 0)
    {
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                               _("Too large count value passed to %s"),
                               G_STRFUNC);
      xobject_unref (task);
      return;
    }

  if (!xoutput_stream_set_pending (stream, &error))
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  class->write_async (stream, buffer, count, io_priority, cancellable,
                      async_ready_write_callback_wrapper, task);
}

/**
 * xoutput_stream_write_finish:
 * @stream: a #xoutput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes a stream write operation.
 *
 * Returns: a #xssize_t containing the number of bytes written to the stream.
 **/
xssize_t
xoutput_stream_write_finish (xoutput_stream_t  *stream,
                              xasync_result_t   *result,
                              xerror_t        **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (xasync_result_is_tagged (result, xoutput_stream_write_async), FALSE);

  /* @result is always the xtask_t created by xoutput_stream_write_async();
   * we called class->write_finish() from async_ready_write_callback_wrapper.
   */
  return xtask_propagate_int (XTASK (result), error);
}

typedef struct
{
  const xuint8_t *buffer;
  xsize_t to_write;
  xsize_t bytes_written;
} AsyncWriteAll;

static void
free_async_write_all (xpointer_t data)
{
  g_slice_free (AsyncWriteAll, data);
}

static void
write_all_callback (xobject_t      *stream,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  xtask_t *task = user_data;
  AsyncWriteAll *data = xtask_get_task_data (task);

  if (result)
    {
      xerror_t *error = NULL;
      xssize_t nwritten;

      nwritten = xoutput_stream_write_finish (G_OUTPUT_STREAM (stream), result, &error);

      if (nwritten == -1)
        {
          xtask_return_error (task, error);
          xobject_unref (task);
          return;
        }

      g_assert_cmpint (nwritten, <=, data->to_write);
      g_warn_if_fail (nwritten > 0);

      data->to_write -= nwritten;
      data->bytes_written += nwritten;
    }

  if (data->to_write == 0)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
    }
  else
    xoutput_stream_write_async (G_OUTPUT_STREAM (stream),
                                 data->buffer + data->bytes_written,
                                 data->to_write,
                                 xtask_get_priority (task),
                                 xtask_get_cancellable (task),
                                 write_all_callback, task);
}

static void
write_all_async_thread (xtask_t        *task,
                        xpointer_t      source_object,
                        xpointer_t      task_data,
                        xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = source_object;
  AsyncWriteAll *data = task_data;
  xerror_t *error = NULL;

  if (xoutput_stream_write_all (stream, data->buffer, data->to_write, &data->bytes_written,
                                 xtask_get_cancellable (task), &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

/**
 * xoutput_stream_write_all_async:
 * @stream: A #xoutput_stream_t
 * @buffer: (array length=count) (element-type xuint8_t): the buffer containing the data to write
 * @count: the number of bytes to write
 * @io_priority: the io priority of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous write of @count bytes from @buffer into
 * the stream. When the operation is finished @callback will be called.
 * You can then call xoutput_stream_write_all_finish() to get the result of the
 * operation.
 *
 * This is the asynchronous version of xoutput_stream_write_all().
 *
 * Call xoutput_stream_write_all_finish() to collect the result.
 *
 * Any outstanding I/O request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * Note that no copy of @buffer will be made, so it must stay valid
 * until @callback is called.
 *
 * Since: 2.44
 */
void
xoutput_stream_write_all_async (xoutput_stream_t       *stream,
                                 const void          *buffer,
                                 xsize_t                count,
                                 int                  io_priority,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  AsyncWriteAll *data;
  xtask_t *task;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (buffer != NULL || count == 0);

  task = xtask_new (stream, cancellable, callback, user_data);
  data = g_slice_new0 (AsyncWriteAll);
  data->buffer = buffer;
  data->to_write = count;

  xtask_set_source_tag (task, xoutput_stream_write_all_async);
  xtask_set_task_data (task, data, free_async_write_all);
  xtask_set_priority (task, io_priority);

  /* If async writes are going to be handled via the threadpool anyway
   * then we may as well do it with a single dispatch instead of
   * bouncing in and out.
   */
  if (xoutput_stream_async_write_is_via_threads (stream))
    {
      xtask_run_in_thread (task, write_all_async_thread);
      xobject_unref (task);
    }
  else
    write_all_callback (G_OBJECT (stream), NULL, task);
}

/**
 * xoutput_stream_write_all_finish:
 * @stream: a #xoutput_stream_t
 * @result: a #xasync_result_t
 * @bytes_written: (out) (optional): location to store the number of bytes that was written to the stream
 * @error: a #xerror_t location to store the error occurring, or %NULL to ignore.
 *
 * Finishes an asynchronous stream write operation started with
 * xoutput_stream_write_all_async().
 *
 * As a special exception to the normal conventions for functions that
 * use #xerror_t, if this function returns %FALSE (and sets @error) then
 * @bytes_written will be set to the number of bytes that were
 * successfully written before the error was encountered.  This
 * functionality is only available from C.  If you need it from another
 * language then you must write your own loop around
 * xoutput_stream_write_async().
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 *
 * Since: 2.44
 **/
xboolean_t
xoutput_stream_write_all_finish (xoutput_stream_t  *stream,
                                  xasync_result_t   *result,
                                  xsize_t          *bytes_written,
                                  xerror_t        **error)
{
  xtask_t *task;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  task = XTASK (result);

  if (bytes_written)
    {
      AsyncWriteAll *data = (AsyncWriteAll *)xtask_get_task_data (task);

      *bytes_written = data->bytes_written;
    }

  return xtask_propagate_boolean (task, error);
}

/**
 * xoutput_stream_writev_async:
 * @stream: A #xoutput_stream_t.
 * @vectors: (array length=n_vectors): the buffer containing the #GOutputVectors to write.
 * @n_vectors: the number of vectors to write
 * @io_priority: the I/O priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous write of the bytes contained in @n_vectors @vectors into
 * the stream. When the operation is finished @callback will be called.
 * You can then call xoutput_stream_writev_finish() to get the result of the
 * operation.
 *
 * During an async request no other sync and async calls are allowed,
 * and will result in %G_IO_ERROR_PENDING errors.
 *
 * On success, the number of bytes written will be passed to the
 * @callback. It is not an error if this is not the same as the
 * requested size, as it can happen e.g. on a partial I/O error,
 * but generally we try to write as many bytes as requested.
 *
 * You are guaranteed that this method will never fail with
 * %G_IO_ERROR_WOULD_BLOCK — if @stream can't accept more data, the
 * method will just wait until this changes.
 *
 * Any outstanding I/O request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * The asynchronous methods have a default fallback that uses threads
 * to implement asynchronicity, so they are optional for inheriting
 * classes. However, if you override one you must override all.
 *
 * For the synchronous, blocking version of this function, see
 * xoutput_stream_writev().
 *
 * Note that no copy of @vectors will be made, so it must stay valid
 * until @callback is called.
 *
 * Since: 2.60
 */
void
xoutput_stream_writev_async (xoutput_stream_t             *stream,
			      const xoutput_vector_t       *vectors,
			      xsize_t                      n_vectors,
			      int                        io_priority,
			      xcancellable_t              *cancellable,
			      xasync_ready_callback_t        callback,
			      xpointer_t                   user_data)
{
  xoutput_stream_class_t *class;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (vectors != NULL || n_vectors == 0);
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  g_return_if_fail (class->writev_async != NULL);

  class->writev_async (stream, vectors, n_vectors, io_priority, cancellable,
                       callback, user_data);
}

/**
 * xoutput_stream_writev_finish:
 * @stream: a #xoutput_stream_t.
 * @result: a #xasync_result_t.
 * @bytes_written: (out) (optional): location to store the number of bytes that were written to the stream
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes a stream writev operation.
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 *
 * Since: 2.60
 */
xboolean_t
xoutput_stream_writev_finish (xoutput_stream_t  *stream,
                               xasync_result_t   *result,
                               xsize_t          *bytes_written,
                               xerror_t        **error)
{
  xoutput_stream_class_t *class;
  xboolean_t res;
  xsize_t _bytes_written = 0;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  g_return_val_if_fail (class->writev_finish != NULL, FALSE);

  res = class->writev_finish (stream, result, &_bytes_written, error);

  g_warn_if_fail (res || _bytes_written == 0);
  g_warn_if_fail (res || (error == NULL || *error != NULL));

  if (bytes_written)
    *bytes_written = _bytes_written;

  return res;
}

typedef struct
{
  xoutput_vector_t *vectors;
  xsize_t n_vectors; /* (unowned) */
  xsize_t bytes_written;
} AsyncWritevAll;

static void
free_async_writev_all (xpointer_t data)
{
  g_slice_free (AsyncWritevAll, data);
}

static void
writev_all_callback (xobject_t      *stream,
                     xasync_result_t *result,
                     xpointer_t      user_data)
{
  xtask_t *task = user_data;
  AsyncWritevAll *data = xtask_get_task_data (task);
  xint_t priority = xtask_get_priority (task);
  xcancellable_t *cancellable = xtask_get_cancellable (task);

  if (result)
    {
      xerror_t *error = NULL;
      xboolean_t res;
      xsize_t n_written = 0;

      res = xoutput_stream_writev_finish (G_OUTPUT_STREAM (stream), result, &n_written, &error);

      if (!res)
        {
          xtask_return_error (task, g_steal_pointer (&error));
          xobject_unref (task);
          return;
        }

      g_warn_if_fail (n_written > 0);
      data->bytes_written += n_written;

      /* skip vectors that have been written in full */
      while (data->n_vectors > 0 && n_written >= data->vectors[0].size)
        {
          n_written -= data->vectors[0].size;
          ++data->vectors;
          --data->n_vectors;
        }
      /* skip partially written vector data */
      if (n_written > 0 && data->n_vectors > 0)
        {
          data->vectors[0].size -= n_written;
          data->vectors[0].buffer = ((xuint8_t *) data->vectors[0].buffer) + n_written;
        }
    }

  if (data->n_vectors == 0)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
    }
  else
    xoutput_stream_writev_async (G_OUTPUT_STREAM (stream),
                                  data->vectors,
                                  data->n_vectors,
                                  priority,
                                  cancellable,
                                  writev_all_callback, g_steal_pointer (&task));
}

static void
writev_all_async_thread (xtask_t        *task,
                         xpointer_t      source_object,
                         xpointer_t      task_data,
                         xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  AsyncWritevAll *data = task_data;
  xerror_t *error = NULL;

  if (xoutput_stream_writev_all (stream, data->vectors, data->n_vectors, &data->bytes_written,
                                  xtask_get_cancellable (task), &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, g_steal_pointer (&error));
}

/**
 * xoutput_stream_writev_all_async:
 * @stream: A #xoutput_stream_t
 * @vectors: (array length=n_vectors): the buffer containing the #GOutputVectors to write.
 * @n_vectors: the number of vectors to write
 * @io_priority: the I/O priority of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous write of the bytes contained in the @n_vectors @vectors into
 * the stream. When the operation is finished @callback will be called.
 * You can then call xoutput_stream_writev_all_finish() to get the result of the
 * operation.
 *
 * This is the asynchronous version of xoutput_stream_writev_all().
 *
 * Call xoutput_stream_writev_all_finish() to collect the result.
 *
 * Any outstanding I/O request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * Note that no copy of @vectors will be made, so it must stay valid
 * until @callback is called. The content of the individual elements
 * of @vectors might be changed by this function.
 *
 * Since: 2.60
 */
void
xoutput_stream_writev_all_async (xoutput_stream_t       *stream,
                                  xoutput_vector_t       *vectors,
                                  xsize_t                n_vectors,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  AsyncWritevAll *data;
  xtask_t *task;
  xsize_t i, to_be_written = 0;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (vectors != NULL || n_vectors == 0);
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  task = xtask_new (stream, cancellable, callback, user_data);
  data = g_slice_new0 (AsyncWritevAll);
  data->vectors = vectors;
  data->n_vectors = n_vectors;

  xtask_set_source_tag (task, xoutput_stream_writev_all_async);
  xtask_set_task_data (task, data, free_async_writev_all);
  xtask_set_priority (task, io_priority);

  /* We can't write more than G_MAXSIZE bytes overall, otherwise we
   * would overflow the bytes_written counter */
  for (i = 0; i < n_vectors; i++)
    {
       if (to_be_written > G_MAXSIZE - vectors[i].size)
         {
           xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                    _("Sum of vectors passed to %s too large"),
                                    G_STRFUNC);
           xobject_unref (task);
           return;
         }
       to_be_written += vectors[i].size;
    }

  /* If async writes are going to be handled via the threadpool anyway
   * then we may as well do it with a single dispatch instead of
   * bouncing in and out.
   */
  if (xoutput_stream_async_writev_is_via_threads (stream))
    {
      xtask_run_in_thread (task, writev_all_async_thread);
      xobject_unref (task);
    }
  else
    writev_all_callback (G_OBJECT (stream), NULL, g_steal_pointer (&task));
}

/**
 * xoutput_stream_writev_all_finish:
 * @stream: a #xoutput_stream_t
 * @result: a #xasync_result_t
 * @bytes_written: (out) (optional): location to store the number of bytes that were written to the stream
 * @error: a #xerror_t location to store the error occurring, or %NULL to ignore.
 *
 * Finishes an asynchronous stream write operation started with
 * xoutput_stream_writev_all_async().
 *
 * As a special exception to the normal conventions for functions that
 * use #xerror_t, if this function returns %FALSE (and sets @error) then
 * @bytes_written will be set to the number of bytes that were
 * successfully written before the error was encountered.  This
 * functionality is only available from C.  If you need it from another
 * language then you must write your own loop around
 * xoutput_stream_writev_async().
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 *
 * Since: 2.60
 */
xboolean_t
xoutput_stream_writev_all_finish (xoutput_stream_t  *stream,
                                   xasync_result_t   *result,
                                   xsize_t          *bytes_written,
                                   xerror_t        **error)
{
  xtask_t *task;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  task = XTASK (result);

  if (bytes_written)
    {
      AsyncWritevAll *data = (AsyncWritevAll *)xtask_get_task_data (task);

      *bytes_written = data->bytes_written;
    }

  return xtask_propagate_boolean (task, error);
}

static void
write_bytes_callback (xobject_t      *stream,
                      xasync_result_t *result,
                      xpointer_t      user_data)
{
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xssize_t nwrote;

  nwrote = xoutput_stream_write_finish (G_OUTPUT_STREAM (stream),
                                         result, &error);
  if (nwrote == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, nwrote);
  xobject_unref (task);
}

/**
 * xoutput_stream_write_bytes_async:
 * @stream: A #xoutput_stream_t.
 * @bytes: The bytes to write
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * This function is similar to xoutput_stream_write_async(), but
 * takes a #xbytes_t as input.  Due to the refcounted nature of #xbytes_t,
 * this allows the stream to avoid taking a copy of the data.
 *
 * However, note that this function may still perform partial writes,
 * just like xoutput_stream_write_async(). If that occurs, to continue
 * writing, you will need to create a new #xbytes_t containing just the
 * remaining bytes, using xbytes_new_from_bytes(). Passing the same
 * #xbytes_t instance multiple times potentially can result in duplicated
 * data in the output stream.
 *
 * For the synchronous, blocking version of this function, see
 * xoutput_stream_write_bytes().
 **/
void
xoutput_stream_write_bytes_async (xoutput_stream_t       *stream,
				   xbytes_t              *bytes,
				   int                  io_priority,
				   xcancellable_t        *cancellable,
				   xasync_ready_callback_t  callback,
				   xpointer_t             user_data)
{
  xtask_t *task;
  xsize_t size;
  xconstpointer data;

  data = xbytes_get_data (bytes, &size);

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xoutput_stream_write_bytes_async);
  xtask_set_task_data (task, xbytes_ref (bytes),
                        (xdestroy_notify_t) xbytes_unref);

  xoutput_stream_write_async (stream,
                               data, size,
                               io_priority,
                               cancellable,
                               write_bytes_callback,
                               task);
}

/**
 * xoutput_stream_write_bytes_finish:
 * @stream: a #xoutput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes a stream write-from-#xbytes_t operation.
 *
 * Returns: a #xssize_t containing the number of bytes written to the stream.
 **/
xssize_t
xoutput_stream_write_bytes_finish (xoutput_stream_t  *stream,
				    xasync_result_t   *result,
				    xerror_t        **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), -1);
  g_return_val_if_fail (xtask_is_valid (result, stream), -1);

  return xtask_propagate_int (XTASK (result), error);
}

static void
async_ready_splice_callback_wrapper (xobject_t      *source_object,
                                     xasync_result_t *res,
                                     xpointer_t     _data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  xoutput_stream_class_t *class;
  xtask_t *task = _data;
  xssize_t nspliced;
  xerror_t *error = NULL;

  xoutput_stream_clear_pending (stream);

  if (xasync_result_legacy_propagate_error (res, &error))
    nspliced = -1;
  else
    {
      class = G_OUTPUT_STREAM_GET_CLASS (stream);
      nspliced = class->splice_finish (stream, res, &error);
    }

  if (nspliced >= 0)
    xtask_return_int (task, nspliced);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * xoutput_stream_splice_async:
 * @stream: a #xoutput_stream_t.
 * @source: a #xinput_stream_t.
 * @flags: a set of #GOutputStreamSpliceFlags.
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @user_data: (closure): user data passed to @callback.
 *
 * Splices a stream asynchronously.
 * When the operation is finished @callback will be called.
 * You can then call xoutput_stream_splice_finish() to get the
 * result of the operation.
 *
 * For the synchronous, blocking version of this function, see
 * xoutput_stream_splice().
 **/
void
xoutput_stream_splice_async (xoutput_stream_t            *stream,
			      xinput_stream_t             *source,
			      GOutputStreamSpliceFlags  flags,
			      int                       io_priority,
			      xcancellable_t             *cancellable,
			      xasync_ready_callback_t       callback,
			      xpointer_t                  user_data)
{
  xoutput_stream_class_t *class;
  xtask_t *task;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));
  g_return_if_fail (X_IS_INPUT_STREAM (source));

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xoutput_stream_splice_async);
  xtask_set_priority (task, io_priority);
  xtask_set_task_data (task, xobject_ref (source), xobject_unref);

  if (xinput_stream_is_closed (source))
    {
      xtask_return_new_error (task,
                               G_IO_ERROR, G_IO_ERROR_CLOSED,
                               _("Source stream is already closed"));
      xobject_unref (task);
      return;
    }

  if (!xoutput_stream_set_pending (stream, &error))
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  class->splice_async (stream, source, flags, io_priority, cancellable,
                       async_ready_splice_callback_wrapper, task);
}

/**
 * xoutput_stream_splice_finish:
 * @stream: a #xoutput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an asynchronous stream splice operation.
 *
 * Returns: a #xssize_t of the number of bytes spliced. Note that if the
 *     number of bytes spliced is greater than %G_MAXSSIZE, then that
 *     will be returned, and there is no way to determine the actual
 *     number of bytes spliced.
 **/
xssize_t
xoutput_stream_splice_finish (xoutput_stream_t  *stream,
			       xasync_result_t   *result,
			       xerror_t        **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (xasync_result_is_tagged (result, xoutput_stream_splice_async), FALSE);

  /* @result is always the xtask_t created by xoutput_stream_splice_async();
   * we called class->splice_finish() from async_ready_splice_callback_wrapper.
   */
  return xtask_propagate_int (XTASK (result), error);
}

static void
async_ready_flush_callback_wrapper (xobject_t      *source_object,
                                    xasync_result_t *res,
                                    xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  xoutput_stream_class_t *class;
  xtask_t *task = user_data;
  xboolean_t flushed;
  xerror_t *error = NULL;

  xoutput_stream_clear_pending (stream);

  if (xasync_result_legacy_propagate_error (res, &error))
    flushed = FALSE;
  else
    {
      class = G_OUTPUT_STREAM_GET_CLASS (stream);
      flushed = class->flush_finish (stream, res, &error);
    }

  if (flushed)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * xoutput_stream_flush_async:
 * @stream: a #xoutput_stream_t.
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Forces an asynchronous write of all user-space buffered data for
 * the given @stream.
 * For behaviour details see xoutput_stream_flush().
 *
 * When the operation is finished @callback will be
 * called. You can then call xoutput_stream_flush_finish() to get the
 * result of the operation.
 **/
void
xoutput_stream_flush_async (xoutput_stream_t       *stream,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data)
{
  xoutput_stream_class_t *class;
  xtask_t *task;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xoutput_stream_flush_async);
  xtask_set_priority (task, io_priority);

  if (!xoutput_stream_set_pending (stream, &error))
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (class->flush_async == NULL)
    {
      xoutput_stream_clear_pending (stream);
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
      return;
    }

  class->flush_async (stream, io_priority, cancellable,
                      async_ready_flush_callback_wrapper, task);
}

/**
 * xoutput_stream_flush_finish:
 * @stream: a #xoutput_stream_t.
 * @result: a xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes flushing an output stream.
 *
 * Returns: %TRUE if flush operation succeeded, %FALSE otherwise.
 **/
xboolean_t
xoutput_stream_flush_finish (xoutput_stream_t  *stream,
                              xasync_result_t   *result,
                              xerror_t        **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (xasync_result_is_tagged (result, xoutput_stream_flush_async), FALSE);

  /* @result is always the xtask_t created by xoutput_stream_flush_async();
   * we called class->flush_finish() from async_ready_flush_callback_wrapper.
   */
  return xtask_propagate_boolean (XTASK (result), error);
}


static void
async_ready_close_callback_wrapper (xobject_t      *source_object,
                                    xasync_result_t *res,
                                    xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  xoutput_stream_class_t *class;
  xtask_t *task = user_data;
  xerror_t *error = xtask_get_task_data (task);

  stream->priv->closing = FALSE;
  stream->priv->closed = TRUE;

  if (!error && !xasync_result_legacy_propagate_error (res, &error))
    {
      class = G_OUTPUT_STREAM_GET_CLASS (stream);

      class->close_finish (stream, res,
                           error ? NULL : &error);
    }

  if (error != NULL)
    xtask_return_error (task, error);
  else
    xtask_return_boolean (task, TRUE);
  xobject_unref (task);
}

static void
async_ready_close_flushed_callback_wrapper (xobject_t      *source_object,
                                            xasync_result_t *res,
                                            xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  xoutput_stream_class_t *class;
  xtask_t *task = user_data;
  xerror_t *error = NULL;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (!xasync_result_legacy_propagate_error (res, &error))
    {
      class->flush_finish (stream, res, &error);
    }

  /* propagate the possible error */
  if (error)
    xtask_set_task_data (task, error, NULL);

  /* we still close, even if there was a flush error */
  class->close_async (stream,
                      xtask_get_priority (task),
                      xtask_get_cancellable (task),
                      async_ready_close_callback_wrapper, task);
}

static void
real_close_async_cb (xobject_t      *source_object,
                     xasync_result_t *res,
                     xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (source_object);
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xboolean_t ret;

  xoutput_stream_clear_pending (stream);

  ret = xoutput_stream_internal_close_finish (stream, res, &error);

  if (error != NULL)
    xtask_return_error (task, error);
  else
    xtask_return_boolean (task, ret);

  xobject_unref (task);
}

/**
 * xoutput_stream_close_async:
 * @stream: A #xoutput_stream_t.
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional cancellable object
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Requests an asynchronous close of the stream, releasing resources
 * related to it. When the operation is finished @callback will be
 * called. You can then call xoutput_stream_close_finish() to get
 * the result of the operation.
 *
 * For behaviour details see xoutput_stream_close().
 *
 * The asynchronous methods have a default fallback that uses threads
 * to implement asynchronicity, so they are optional for inheriting
 * classes. However, if you override one you must override all.
 **/
void
xoutput_stream_close_async (xoutput_stream_t       *stream,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data)
{
  xtask_t *task;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xoutput_stream_close_async);
  xtask_set_priority (task, io_priority);

  if (!xoutput_stream_set_pending (stream, &error))
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  xoutput_stream_internal_close_async (stream, io_priority, cancellable,
                                        real_close_async_cb, task);
}

/* Must always be called inside
 * xoutput_stream_set_pending()/xoutput_stream_clear_pending().
 */
void
xoutput_stream_internal_close_async (xoutput_stream_t       *stream,
                                      int                  io_priority,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  xoutput_stream_class_t *class;
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xoutput_stream_internal_close_async);
  xtask_set_priority (task, io_priority);

  if (stream->priv->closed)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
      return;
    }

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  stream->priv->closing = TRUE;

  /* Call close_async directly if there is no need to flush, or if the flush
     can be done sync (in the output stream async close thread) */
  if (class->flush_async == NULL ||
      (class->flush_async == xoutput_stream_real_flush_async &&
       (class->flush == NULL || class->close_async == xoutput_stream_real_close_async)))
    {
      class->close_async (stream, io_priority, cancellable,
                          async_ready_close_callback_wrapper, task);
    }
  else
    {
      /* First do an async flush, then do the async close in the callback
         wrapper (see async_ready_close_flushed_callback_wrapper) */
      class->flush_async (stream, io_priority, cancellable,
                          async_ready_close_flushed_callback_wrapper, task);
    }
}

static xboolean_t
xoutput_stream_internal_close_finish (xoutput_stream_t  *stream,
                                       xasync_result_t   *result,
                                       xerror_t        **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (xasync_result_is_tagged (result, xoutput_stream_internal_close_async), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

/**
 * xoutput_stream_close_finish:
 * @stream: a #xoutput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Closes an output stream.
 *
 * Returns: %TRUE if stream was successfully closed, %FALSE otherwise.
 **/
xboolean_t
xoutput_stream_close_finish (xoutput_stream_t  *stream,
                              xasync_result_t   *result,
                              xerror_t        **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (xasync_result_is_tagged (result, xoutput_stream_close_async), FALSE);

  /* @result is always the xtask_t created by xoutput_stream_close_async();
   * we called class->close_finish() from async_ready_close_callback_wrapper.
   */
  return xtask_propagate_boolean (XTASK (result), error);
}

/**
 * xoutput_stream_is_closed:
 * @stream: a #xoutput_stream_t.
 *
 * Checks if an output stream has already been closed.
 *
 * Returns: %TRUE if @stream is closed. %FALSE otherwise.
 **/
xboolean_t
xoutput_stream_is_closed (xoutput_stream_t *stream)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), TRUE);

  return stream->priv->closed;
}

/**
 * xoutput_stream_is_closing:
 * @stream: a #xoutput_stream_t.
 *
 * Checks if an output stream is being closed. This can be
 * used inside e.g. a flush implementation to see if the
 * flush (or other i/o operation) is called from within
 * the closing operation.
 *
 * Returns: %TRUE if @stream is being closed. %FALSE otherwise.
 *
 * Since: 2.24
 **/
xboolean_t
xoutput_stream_is_closing (xoutput_stream_t *stream)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), TRUE);

  return stream->priv->closing;
}

/**
 * xoutput_stream_has_pending:
 * @stream: a #xoutput_stream_t.
 *
 * Checks if an output stream has pending actions.
 *
 * Returns: %TRUE if @stream has pending actions.
 **/
xboolean_t
xoutput_stream_has_pending (xoutput_stream_t *stream)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  return stream->priv->pending;
}

/**
 * xoutput_stream_set_pending:
 * @stream: a #xoutput_stream_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Sets @stream to have actions pending. If the pending flag is
 * already set or @stream is closed, it will return %FALSE and set
 * @error.
 *
 * Returns: %TRUE if pending was previously unset and is now set.
 **/
xboolean_t
xoutput_stream_set_pending (xoutput_stream_t *stream,
			     xerror_t **error)
{
  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  if (stream->priv->closed)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Stream is already closed"));
      return FALSE;
    }

  if (stream->priv->pending)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
                           /* Translators: This is an error you get if there is
                            * already an operation running against this stream when
                            * you try to start one */
                           _("Stream has outstanding operation"));
      return FALSE;
    }

  stream->priv->pending = TRUE;
  return TRUE;
}

/**
 * xoutput_stream_clear_pending:
 * @stream: output stream
 *
 * Clears the pending flag on @stream.
 **/
void
xoutput_stream_clear_pending (xoutput_stream_t *stream)
{
  g_return_if_fail (X_IS_OUTPUT_STREAM (stream));

  stream->priv->pending = FALSE;
}

/*< internal >
 * xoutput_stream_async_write_is_via_threads:
 * @stream: a #xoutput_stream_t.
 *
 * Checks if an output stream's write_async function uses threads.
 *
 * Returns: %TRUE if @stream's write_async function uses threads.
 **/
xboolean_t
xoutput_stream_async_write_is_via_threads (xoutput_stream_t *stream)
{
  xoutput_stream_class_t *class;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  return (class->write_async == xoutput_stream_real_write_async &&
      !(X_IS_POLLABLE_OUTPUT_STREAM (stream) &&
        xpollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (stream))));
}

/*< internal >
 * xoutput_stream_async_writev_is_via_threads:
 * @stream: a #xoutput_stream_t.
 *
 * Checks if an output stream's writev_async function uses threads.
 *
 * Returns: %TRUE if @stream's writev_async function uses threads.
 **/
xboolean_t
xoutput_stream_async_writev_is_via_threads (xoutput_stream_t *stream)
{
  xoutput_stream_class_t *class;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  return (class->writev_async == xoutput_stream_real_writev_async &&
      !(X_IS_POLLABLE_OUTPUT_STREAM (stream) &&
        xpollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (stream))));
}

/*< internal >
 * xoutput_stream_async_close_is_via_threads:
 * @stream: output stream
 *
 * Checks if an output stream's close_async function uses threads.
 *
 * Returns: %TRUE if @stream's close_async function uses threads.
 **/
xboolean_t
xoutput_stream_async_close_is_via_threads (xoutput_stream_t *stream)
{
  xoutput_stream_class_t *class;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (stream), FALSE);

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  return class->close_async == xoutput_stream_real_close_async;
}

/********************************************
 *   Default implementation of sync ops    *
 ********************************************/
static xboolean_t
xoutput_stream_real_writev (xoutput_stream_t         *stream,
                             const xoutput_vector_t   *vectors,
                             xsize_t                  n_vectors,
                             xsize_t                 *bytes_written,
                             xcancellable_t          *cancellable,
                             xerror_t               **error)
{
  xoutput_stream_class_t *class;
  xsize_t _bytes_written = 0;
  xsize_t i;
  xerror_t *err = NULL;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  if (bytes_written)
    *bytes_written = 0;

  for (i = 0; i < n_vectors; i++)
    {
      xssize_t res = 0;

      /* Would we overflow here? In that case simply return and let the caller
       * handle this like a short write */
      if (_bytes_written > G_MAXSIZE - vectors[i].size)
        break;

      res = class->write_fn (stream, vectors[i].buffer, vectors[i].size, cancellable, &err);

      if (res == -1)
        {
          /* If we already wrote something  we handle this like a short write
           * and assume that on the next call the same error happens again, or
           * everything finishes successfully without data loss then
           */
          if (_bytes_written > 0)
            {
              if (bytes_written)
                *bytes_written = _bytes_written;

              g_clear_error (&err);
              return TRUE;
            }

          g_propagate_error (error, err);
          return FALSE;
        }

      _bytes_written += res;
      /* if we had a short write break the loop here */
      if ((xsize_t) res < vectors[i].size)
        break;
    }

  if (bytes_written)
    *bytes_written = _bytes_written;

  return TRUE;
}

/********************************************
 *   Default implementation of async ops    *
 ********************************************/

typedef struct {
  const void         *buffer;
  xsize_t               count_requested;
  xssize_t              count_written;
} WriteData;

static void
free_write_data (WriteData *op)
{
  g_slice_free (WriteData, op);
}

static void
write_async_thread (xtask_t        *task,
                    xpointer_t      source_object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = source_object;
  WriteData *op = task_data;
  xoutput_stream_class_t *class;
  xerror_t *error = NULL;
  xssize_t count_written;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  count_written = class->write_fn (stream, op->buffer, op->count_requested,
                                   cancellable, &error);
  if (count_written == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, count_written);
}

static void write_async_pollable (xpollable_output_stream_t *stream,
                                  xtask_t                 *task);

static xboolean_t
write_async_pollable_ready (xpollable_output_stream_t *stream,
			    xpointer_t               user_data)
{
  xtask_t *task = user_data;

  write_async_pollable (stream, task);
  return FALSE;
}

static void
write_async_pollable (xpollable_output_stream_t *stream,
                      xtask_t                 *task)
{
  xerror_t *error = NULL;
  WriteData *op = xtask_get_task_data (task);
  xssize_t count_written;

  if (xtask_return_error_if_cancelled (task))
    return;

  count_written = G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream)->
    write_nonblocking (stream, op->buffer, op->count_requested, &error);

  if (xerror_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      xsource_t *source;

      xerror_free (error);

      source = xpollable_output_stream_create_source (stream,
                                                       xtask_get_cancellable (task));
      xtask_attach_source (task, source,
                            (xsource_func_t) write_async_pollable_ready);
      xsource_unref (source);
      return;
    }

  if (count_written == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, count_written);
}

static void
xoutput_stream_real_write_async (xoutput_stream_t       *stream,
                                  const void          *buffer,
                                  xsize_t                count,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xtask_t *task;
  WriteData *op;

  op = g_slice_new0 (WriteData);
  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_check_cancellable (task, FALSE);
  xtask_set_task_data (task, op, (xdestroy_notify_t) free_write_data);
  op->buffer = buffer;
  op->count_requested = count;

  if (!xoutput_stream_async_write_is_via_threads (stream))
    write_async_pollable (G_POLLABLE_OUTPUT_STREAM (stream), task);
  else
    xtask_run_in_thread (task, write_async_thread);
  xobject_unref (task);
}

static xssize_t
xoutput_stream_real_write_finish (xoutput_stream_t  *stream,
                                   xasync_result_t   *result,
                                   xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_int (XTASK (result), error);
}

typedef struct {
  const xoutput_vector_t *vectors;
  xsize_t                n_vectors; /* (unowned) */
  xsize_t                bytes_written;
} WritevData;

static void
free_writev_data (WritevData *op)
{
  g_slice_free (WritevData, op);
}

static void
writev_async_thread (xtask_t        *task,
                     xpointer_t      source_object,
                     xpointer_t      task_data,
                     xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = source_object;
  WritevData *op = task_data;
  xoutput_stream_class_t *class;
  xerror_t *error = NULL;
  xboolean_t res;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  res = class->writev_fn (stream, op->vectors, op->n_vectors,
                          &op->bytes_written, cancellable, &error);

  g_warn_if_fail (res || op->bytes_written == 0);
  g_warn_if_fail (res || error != NULL);

  if (!res)
    xtask_return_error (task, g_steal_pointer (&error));
  else
    xtask_return_boolean (task, TRUE);
}

static void writev_async_pollable (xpollable_output_stream_t *stream,
                                   xtask_t                 *task);

static xboolean_t
writev_async_pollable_ready (xpollable_output_stream_t *stream,
			     xpointer_t               user_data)
{
  xtask_t *task = user_data;

  writev_async_pollable (stream, task);
  return G_SOURCE_REMOVE;
}

static void
writev_async_pollable (xpollable_output_stream_t *stream,
                       xtask_t                 *task)
{
  xerror_t *error = NULL;
  WritevData *op = xtask_get_task_data (task);
  GPollableReturn res;
  xsize_t bytes_written = 0;

  if (xtask_return_error_if_cancelled (task))
    return;

  res = G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE (stream)->
    writev_nonblocking (stream, op->vectors, op->n_vectors, &bytes_written, &error);

  switch (res)
    {
    case G_POLLABLE_RETURN_WOULD_BLOCK:
        {
          xsource_t *source;

          g_warn_if_fail (error == NULL);
          g_warn_if_fail (bytes_written == 0);

          source = xpollable_output_stream_create_source (stream,
                                                           xtask_get_cancellable (task));
          xtask_attach_source (task, source,
                                (xsource_func_t) writev_async_pollable_ready);
          xsource_unref (source);
        }
        break;
      case G_POLLABLE_RETURN_OK:
        g_warn_if_fail (error == NULL);
        op->bytes_written = bytes_written;
        xtask_return_boolean (task, TRUE);
        break;
      case G_POLLABLE_RETURN_FAILED:
        g_warn_if_fail (bytes_written == 0);
        g_warn_if_fail (error != NULL);
        xtask_return_error (task, g_steal_pointer (&error));
        break;
      default:
        g_assert_not_reached ();
    }
}

static void
xoutput_stream_real_writev_async (xoutput_stream_t        *stream,
                                   const xoutput_vector_t  *vectors,
                                   xsize_t                 n_vectors,
                                   int                   io_priority,
                                   xcancellable_t         *cancellable,
                                   xasync_ready_callback_t   callback,
                                   xpointer_t              user_data)
{
  xtask_t *task;
  WritevData *op;
  xerror_t *error = NULL;

  op = g_slice_new0 (WritevData);
  task = xtask_new (stream, cancellable, callback, user_data);
  op->vectors = vectors;
  op->n_vectors = n_vectors;

  xtask_set_check_cancellable (task, FALSE);
  xtask_set_source_tag (task, xoutput_stream_writev_async);
  xtask_set_priority (task, io_priority);
  xtask_set_task_data (task, op, (xdestroy_notify_t) free_writev_data);

  if (n_vectors == 0)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
      return;
    }

  if (!xoutput_stream_set_pending (stream, &error))
    {
      xtask_return_error (task, g_steal_pointer (&error));
      xobject_unref (task);
      return;
    }

  if (!xoutput_stream_async_writev_is_via_threads (stream))
    writev_async_pollable (G_POLLABLE_OUTPUT_STREAM (stream), task);
  else
    xtask_run_in_thread (task, writev_async_thread);

  xobject_unref (task);
}

static xboolean_t
xoutput_stream_real_writev_finish (xoutput_stream_t   *stream,
                                    xasync_result_t    *result,
                                    xsize_t           *bytes_written,
                                    xerror_t         **error)
{
  xtask_t *task;

  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);
  g_return_val_if_fail (xasync_result_is_tagged (result, xoutput_stream_writev_async), FALSE);

  xoutput_stream_clear_pending (stream);

  task = XTASK (result);

  if (bytes_written)
    {
      WritevData *op = xtask_get_task_data (task);

      *bytes_written = op->bytes_written;
    }

  return xtask_propagate_boolean (task, error);
}

typedef struct {
  xinput_stream_t *source;
  GOutputStreamSpliceFlags flags;
  xuint_t istream_closed : 1;
  xuint_t ostream_closed : 1;
  xssize_t n_read;
  xssize_t n_written;
  xsize_t bytes_copied;
  xerror_t *error;
  xuint8_t *buffer;
} SpliceData;

static void
free_splice_data (SpliceData *op)
{
  g_clear_pointer (&op->buffer, g_free);
  xobject_unref (op->source);
  g_clear_error (&op->error);
  g_free (op);
}

static void
real_splice_async_complete_cb (xtask_t *task)
{
  SpliceData *op = xtask_get_task_data (task);

  if (op->flags & G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE &&
      !op->istream_closed)
    return;

  if (op->flags & G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET &&
      !op->ostream_closed)
    return;

  if (op->error != NULL)
    {
      xtask_return_error (task, op->error);
      op->error = NULL;
    }
  else
    {
      xtask_return_int (task, op->bytes_copied);
    }

  xobject_unref (task);
}

static void
real_splice_async_close_input_cb (xobject_t      *source,
                                  xasync_result_t *res,
                                  xpointer_t      user_data)
{
  xtask_t *task = user_data;
  SpliceData *op = xtask_get_task_data (task);

  xinput_stream_close_finish (G_INPUT_STREAM (source), res, NULL);
  op->istream_closed = TRUE;

  real_splice_async_complete_cb (task);
}

static void
real_splice_async_close_output_cb (xobject_t      *source,
                                   xasync_result_t *res,
                                   xpointer_t      user_data)
{
  xtask_t *task = XTASK (user_data);
  SpliceData *op = xtask_get_task_data (task);
  xerror_t **error = (op->error == NULL) ? &op->error : NULL;

  xoutput_stream_internal_close_finish (G_OUTPUT_STREAM (source), res, error);
  op->ostream_closed = TRUE;

  real_splice_async_complete_cb (task);
}

static void
real_splice_async_complete (xtask_t *task)
{
  SpliceData *op = xtask_get_task_data (task);
  xboolean_t done = TRUE;

  if (op->flags & G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE)
    {
      done = FALSE;
      xinput_stream_close_async (op->source, xtask_get_priority (task),
                                  xtask_get_cancellable (task),
                                  real_splice_async_close_input_cb, task);
    }

  if (op->flags & G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET)
    {
      done = FALSE;
      xoutput_stream_internal_close_async (xtask_get_source_object (task),
                                            xtask_get_priority (task),
                                            xtask_get_cancellable (task),
                                            real_splice_async_close_output_cb,
                                            task);
    }

  if (done)
    real_splice_async_complete_cb (task);
}

static void real_splice_async_read_cb (xobject_t      *source,
                                       xasync_result_t *res,
                                       xpointer_t      user_data);

static void
real_splice_async_write_cb (xobject_t      *source,
                            xasync_result_t *res,
                            xpointer_t      user_data)
{
  xoutput_stream_class_t *class;
  xtask_t *task = XTASK (user_data);
  SpliceData *op = xtask_get_task_data (task);
  xssize_t ret;

  class = G_OUTPUT_STREAM_GET_CLASS (xtask_get_source_object (task));

  ret = class->write_finish (G_OUTPUT_STREAM (source), res, &op->error);

  if (ret == -1)
    {
      real_splice_async_complete (task);
      return;
    }

  op->n_written += ret;
  op->bytes_copied += ret;
  if (op->bytes_copied > G_MAXSSIZE)
    op->bytes_copied = G_MAXSSIZE;

  if (op->n_written < op->n_read)
    {
      class->write_async (xtask_get_source_object (task),
                          op->buffer + op->n_written,
                          op->n_read - op->n_written,
                          xtask_get_priority (task),
                          xtask_get_cancellable (task),
                          real_splice_async_write_cb, task);
      return;
    }

  xinput_stream_read_async (op->source, op->buffer, 8192,
                             xtask_get_priority (task),
                             xtask_get_cancellable (task),
                             real_splice_async_read_cb, task);
}

static void
real_splice_async_read_cb (xobject_t      *source,
                           xasync_result_t *res,
                           xpointer_t      user_data)
{
  xoutput_stream_class_t *class;
  xtask_t *task = XTASK (user_data);
  SpliceData *op = xtask_get_task_data (task);
  xssize_t ret;

  class = G_OUTPUT_STREAM_GET_CLASS (xtask_get_source_object (task));

  ret = xinput_stream_read_finish (op->source, res, &op->error);
  if (ret == -1 || ret == 0)
    {
      real_splice_async_complete (task);
      return;
    }

  op->n_read = ret;
  op->n_written = 0;

  class->write_async (xtask_get_source_object (task), op->buffer,
                      op->n_read, xtask_get_priority (task),
                      xtask_get_cancellable (task),
                      real_splice_async_write_cb, task);
}

static void
splice_async_thread (xtask_t        *task,
                     xpointer_t      source_object,
                     xpointer_t      task_data,
                     xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = source_object;
  SpliceData *op = task_data;
  xoutput_stream_class_t *class;
  xerror_t *error = NULL;
  xssize_t bytes_copied;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  bytes_copied = class->splice (stream,
                                op->source,
                                op->flags,
                                cancellable,
                                &error);
  if (bytes_copied == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, bytes_copied);
}

static void
xoutput_stream_real_splice_async (xoutput_stream_t             *stream,
                                   xinput_stream_t              *source,
                                   GOutputStreamSpliceFlags   flags,
                                   int                        io_priority,
                                   xcancellable_t              *cancellable,
                                   xasync_ready_callback_t        callback,
                                   xpointer_t                   user_data)
{
  xtask_t *task;
  SpliceData *op;

  op = g_new0 (SpliceData, 1);
  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_task_data (task, op, (xdestroy_notify_t)free_splice_data);
  op->flags = flags;
  op->source = xobject_ref (source);

  if (xinput_stream_async_read_is_via_threads (source) &&
      xoutput_stream_async_write_is_via_threads (stream))
    {
      xtask_run_in_thread (task, splice_async_thread);
      xobject_unref (task);
    }
  else
    {
      op->buffer = g_malloc (8192);
      xinput_stream_read_async (op->source, op->buffer, 8192,
                                 xtask_get_priority (task),
                                 xtask_get_cancellable (task),
                                 real_splice_async_read_cb, task);
    }
}

static xssize_t
xoutput_stream_real_splice_finish (xoutput_stream_t  *stream,
                                    xasync_result_t   *result,
                                    xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_int (XTASK (result), error);
}


static void
flush_async_thread (xtask_t        *task,
                    xpointer_t      source_object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = source_object;
  xoutput_stream_class_t *class;
  xboolean_t result;
  xerror_t *error = NULL;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);
  result = TRUE;
  if (class->flush)
    result = class->flush (stream, cancellable, &error);

  if (result)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xoutput_stream_real_flush_async (xoutput_stream_t       *stream,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, flush_async_thread);
  xobject_unref (task);
}

static xboolean_t
xoutput_stream_real_flush_finish (xoutput_stream_t  *stream,
                                   xasync_result_t   *result,
                                   xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static void
close_async_thread (xtask_t        *task,
                    xpointer_t      source_object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xoutput_stream_t *stream = source_object;
  xoutput_stream_class_t *class;
  xerror_t *error = NULL;
  xboolean_t result = TRUE;

  class = G_OUTPUT_STREAM_GET_CLASS (stream);

  /* Do a flush here if there is a flush function, and we did not have to do
   * an async flush before (see xoutput_stream_close_async)
   */
  if (class->flush != NULL &&
      (class->flush_async == NULL ||
       class->flush_async == xoutput_stream_real_flush_async))
    {
      result = class->flush (stream, cancellable, &error);
    }

  /* Auto handling of cancellation disabled, and ignore
     cancellation, since we want to close things anyway, although
     possibly in a quick-n-dirty way. At least we never want to leak
     open handles */

  if (class->close_fn)
    {
      /* Make sure to close, even if the flush failed (see sync close) */
      if (!result)
        class->close_fn (stream, cancellable, NULL);
      else
        result = class->close_fn (stream, cancellable, &error);
    }

  if (result)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xoutput_stream_real_close_async (xoutput_stream_t       *stream,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, close_async_thread);
  xobject_unref (task);
}

static xboolean_t
xoutput_stream_real_close_finish (xoutput_stream_t  *stream,
                                   xasync_result_t   *result,
                                   xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}
