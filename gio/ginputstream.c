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
#include "glibintl.h"

#include "ginputstream.h"
#include "gioprivate.h"
#include "gseekable.h"
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gioerror.h"
#include "gpollableinputstream.h"

/**
 * SECTION:ginputstream
 * @short_description: Base class for implementing streaming input
 * @include: gio/gio.h
 *
 * #xinput_stream_t has functions to read from a stream (xinput_stream_read()),
 * to close a stream (xinput_stream_close()) and to skip some content
 * (xinput_stream_skip()).
 *
 * To copy the content of an input stream to an output stream without
 * manually handling the reads and writes, use xoutput_stream_splice().
 *
 * See the documentation for #xio_stream_t for details of thread safety of
 * streaming APIs.
 *
 * All of these functions have async variants too.
 **/

struct _GInputStreamPrivate {
  xuint_t closed : 1;
  xuint_t pending : 1;
  xasync_ready_callback_t outstanding_callback;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xinput_stream, xinput_stream, XTYPE_OBJECT)

static xssize_t   xinput_stream_real_skip         (xinput_stream_t         *stream,
						  xsize_t                 count,
						  xcancellable_t         *cancellable,
						  xerror_t              **error);
static void     xinput_stream_real_read_async   (xinput_stream_t         *stream,
						  void                 *buffer,
						  xsize_t                 count,
						  int                   io_priority,
						  xcancellable_t         *cancellable,
						  xasync_ready_callback_t   callback,
						  xpointer_t              user_data);
static xssize_t   xinput_stream_real_read_finish  (xinput_stream_t         *stream,
						  xasync_result_t         *result,
						  xerror_t              **error);
static void     xinput_stream_real_skip_async   (xinput_stream_t         *stream,
						  xsize_t                 count,
						  int                   io_priority,
						  xcancellable_t         *cancellable,
						  xasync_ready_callback_t   callback,
						  xpointer_t              data);
static xssize_t   xinput_stream_real_skip_finish  (xinput_stream_t         *stream,
						  xasync_result_t         *result,
						  xerror_t              **error);
static void     xinput_stream_real_close_async  (xinput_stream_t         *stream,
						  int                   io_priority,
						  xcancellable_t         *cancellable,
						  xasync_ready_callback_t   callback,
						  xpointer_t              data);
static xboolean_t xinput_stream_real_close_finish (xinput_stream_t         *stream,
						  xasync_result_t         *result,
						  xerror_t              **error);

static void
xinput_stream_dispose (xobject_t *object)
{
  xinput_stream_t *stream;

  stream = G_INPUT_STREAM (object);

  if (!stream->priv->closed)
    xinput_stream_close (stream, NULL, NULL);

  G_OBJECT_CLASS (xinput_stream_parent_class)->dispose (object);
}


static void
xinput_stream_class_init (GInputStreamClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = xinput_stream_dispose;

  klass->skip = xinput_stream_real_skip;
  klass->read_async = xinput_stream_real_read_async;
  klass->read_finish = xinput_stream_real_read_finish;
  klass->skip_async = xinput_stream_real_skip_async;
  klass->skip_finish = xinput_stream_real_skip_finish;
  klass->close_async = xinput_stream_real_close_async;
  klass->close_finish = xinput_stream_real_close_finish;
}

static void
xinput_stream_init (xinput_stream_t *stream)
{
  stream->priv = xinput_stream_get_instance_private (stream);
}

/**
 * xinput_stream_read:
 * @stream: a #xinput_stream_t.
 * @buffer: (array length=count) (element-type xuint8_t) (out caller-allocates):
 *     a buffer to read data into (which should be at least count bytes long).
 * @count: (in): the number of bytes that will be read from the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to read @count bytes from the stream into the buffer starting at
 * @buffer. Will block during this read.
 *
 * If count is zero returns zero and does nothing. A value of @count
 * larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes read into the buffer is returned.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file. Zero is returned on end of file
 * (or if @count is zero),  but never otherwise.
 *
 * The returned @buffer is not a nul-terminated string, it can contain nul bytes
 * at any position, and this function doesn't nul-terminate the @buffer.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error -1 is returned and @error is set accordingly.
 *
 * Returns: Number of bytes read, or -1 on error, or 0 on end of file.
 **/
xssize_t
xinput_stream_read  (xinput_stream_t  *stream,
		      void          *buffer,
		      xsize_t          count,
		      xcancellable_t  *cancellable,
		      xerror_t       **error)
{
  GInputStreamClass *class;
  xssize_t res;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (buffer != NULL, 0);

  if (count == 0)
    return 0;

  if (((xssize_t) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (class->read_fn == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Input stream doesn’t implement read"));
      return -1;
    }

  if (!xinput_stream_set_pending (stream, error))
    return -1;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = class->read_fn (stream, buffer, count, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (stream);

  return res;
}

/**
 * xinput_stream_read_all:
 * @stream: a #xinput_stream_t.
 * @buffer: (array length=count) (element-type xuint8_t) (out caller-allocates):
 *     a buffer to read data into (which should be at least count bytes long).
 * @count: (in): the number of bytes that will be read from the stream
 * @bytes_read: (out): location to store the number of bytes that was read from the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to read @count bytes from the stream into the buffer starting at
 * @buffer. Will block during this read.
 *
 * This function is similar to xinput_stream_read(), except it tries to
 * read as many bytes as requested, only stopping on an error or end of stream.
 *
 * On a successful read of @count bytes, or if we reached the end of the
 * stream,  %TRUE is returned, and @bytes_read is set to the number of bytes
 * read into @buffer.
 *
 * If there is an error during the operation %FALSE is returned and @error
 * is set to indicate the error status.
 *
 * As a special exception to the normal conventions for functions that
 * use #xerror_t, if this function returns %FALSE (and sets @error) then
 * @bytes_read will be set to the number of bytes that were successfully
 * read before the error was encountered.  This functionality is only
 * available from C.  If you need it from another language then you must
 * write your own loop around xinput_stream_read().
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 **/
xboolean_t
xinput_stream_read_all (xinput_stream_t  *stream,
			 void          *buffer,
			 xsize_t          count,
			 xsize_t         *bytes_read,
			 xcancellable_t  *cancellable,
			 xerror_t       **error)
{
  xsize_t _bytes_read;
  xssize_t res;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (buffer != NULL, FALSE);

  _bytes_read = 0;
  while (_bytes_read < count)
    {
      res = xinput_stream_read (stream, (char *)buffer + _bytes_read, count - _bytes_read,
				 cancellable, error);
      if (res == -1)
	{
	  if (bytes_read)
	    *bytes_read = _bytes_read;
	  return FALSE;
	}

      if (res == 0)
	break;

      _bytes_read += res;
    }

  if (bytes_read)
    *bytes_read = _bytes_read;
  return TRUE;
}

/**
 * xinput_stream_read_bytes:
 * @stream: a #xinput_stream_t.
 * @count: maximum number of bytes that will be read from the stream. Common
 * values include 4096 and 8192.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Like xinput_stream_read(), this tries to read @count bytes from
 * the stream in a blocking fashion. However, rather than reading into
 * a user-supplied buffer, this will create a new #xbytes_t containing
 * the data that was read. This may be easier to use from language
 * bindings.
 *
 * If count is zero, returns a zero-length #xbytes_t and does nothing. A
 * value of @count larger than %G_MAXSSIZE will cause a
 * %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, a new #xbytes_t is returned. It is not an error if the
 * size of this object is not the same as the requested size, as it
 * can happen e.g. near the end of a file. A zero-length #xbytes_t is
 * returned on end of file (or if @count is zero), but never
 * otherwise.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error %NULL is returned and @error is set accordingly.
 *
 * Returns: (transfer full): a new #xbytes_t, or %NULL on error
 *
 * Since: 2.34
 **/
xbytes_t *
xinput_stream_read_bytes (xinput_stream_t  *stream,
			   xsize_t          count,
			   xcancellable_t  *cancellable,
			   xerror_t       **error)
{
  guchar *buf;
  xssize_t nread;

  buf = g_malloc (count);
  nread = xinput_stream_read (stream, buf, count, cancellable, error);
  if (nread == -1)
    {
      g_free (buf);
      return NULL;
    }
  else if (nread == 0)
    {
      g_free (buf);
      return xbytes_new_static ("", 0);
    }
  else
    return xbytes_new_take (buf, nread);
}

/**
 * xinput_stream_skip:
 * @stream: a #xinput_stream_t.
 * @count: the number of bytes that will be skipped from the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to skip @count bytes from the stream. Will block during the operation.
 *
 * This is identical to xinput_stream_read(), from a behaviour standpoint,
 * but the bytes that are skipped are not returned to the user. Some
 * streams have an implementation that is more efficient than reading the data.
 *
 * This function is optional for inherited classes, as the default implementation
 * emulates it using read.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * Returns: Number of bytes skipped, or -1 on error
 **/
xssize_t
xinput_stream_skip (xinput_stream_t  *stream,
		     xsize_t          count,
		     xcancellable_t  *cancellable,
		     xerror_t       **error)
{
  GInputStreamClass *class;
  xssize_t res;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), -1);

  if (count == 0)
    return 0;

  if (((xssize_t) count) < 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (!xinput_stream_set_pending (stream, error))
    return -1;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = class->skip (stream, count, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (stream);

  return res;
}

static xssize_t
xinput_stream_real_skip (xinput_stream_t  *stream,
			  xsize_t          count,
			  xcancellable_t  *cancellable,
			  xerror_t       **error)
{
  GInputStreamClass *class;
  xssize_t ret, read_bytes;
  char buffer[8192];
  xerror_t *my_error;

  if (X_IS_SEEKABLE (stream) && xseekable_can_seek (G_SEEKABLE (stream)))
    {
      xseekable__t *seekable = G_SEEKABLE (stream);
      xoffset_t start, end;
      xboolean_t success;

      /* xseekable_seek() may try to set pending itself */
      stream->priv->pending = FALSE;

      start = xseekable_tell (seekable);

      if (xseekable_seek (G_SEEKABLE (stream),
                           0,
                           G_SEEK_END,
                           cancellable,
                           NULL))
        {
          end = xseekable_tell (seekable);
          g_assert (start >= 0);
          g_assert (end >= start);
          if (start > (xoffset_t) (G_MAXOFFSET - count) ||
              (xoffset_t) (start + count) > end)
            {
              stream->priv->pending = TRUE;
              return end - start;
            }

          success = xseekable_seek (G_SEEKABLE (stream),
                                     start + count,
                                     G_SEEK_SET,
                                     cancellable,
                                     error);
          stream->priv->pending = TRUE;

          if (success)
            return count;
          else
            return -1;
        }
    }

  /* If not seekable, or seek failed, fall back to reading data: */

  class = G_INPUT_STREAM_GET_CLASS (stream);

  read_bytes = 0;
  while (1)
    {
      my_error = NULL;

      ret = class->read_fn (stream, buffer, MIN (sizeof (buffer), count),
                            cancellable, &my_error);
      if (ret == -1)
	{
	  if (read_bytes > 0 &&
	      my_error->domain == G_IO_ERROR &&
	      my_error->code == G_IO_ERROR_CANCELLED)
	    {
	      xerror_free (my_error);
	      return read_bytes;
	    }

	  g_propagate_error (error, my_error);
	  return -1;
	}

      count -= ret;
      read_bytes += ret;

      if (ret == 0 || count == 0)
        return read_bytes;
    }
}

/**
 * xinput_stream_close:
 * @stream: A #xinput_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Closes the stream, releasing resources related to it.
 *
 * Once the stream is closed, all other operations will return %G_IO_ERROR_CLOSED.
 * Closing a stream multiple times will not return an error.
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
 * is important to check and report the error to the user.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 * Cancelling a close will still leave the stream closed, but some streams
 * can use a faster close that doesn't block to e.g. check errors.
 *
 * Returns: %TRUE on success, %FALSE on failure
 **/
xboolean_t
xinput_stream_close (xinput_stream_t  *stream,
		      xcancellable_t  *cancellable,
		      xerror_t       **error)
{
  GInputStreamClass *class;
  xboolean_t res;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);

  class = G_INPUT_STREAM_GET_CLASS (stream);

  if (stream->priv->closed)
    return TRUE;

  res = TRUE;

  if (!xinput_stream_set_pending (stream, error))
    return FALSE;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  if (class->close_fn)
    res = class->close_fn (stream, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (stream);

  stream->priv->closed = TRUE;

  return res;
}

static void
async_ready_callback_wrapper (xobject_t      *source_object,
			      xasync_result_t *res,
			      xpointer_t      user_data)
{
  xinput_stream_t *stream = G_INPUT_STREAM (source_object);

  xinput_stream_clear_pending (stream);
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (stream);
}

static void
async_ready_close_callback_wrapper (xobject_t      *source_object,
				    xasync_result_t *res,
				    xpointer_t      user_data)
{
  xinput_stream_t *stream = G_INPUT_STREAM (source_object);

  xinput_stream_clear_pending (stream);
  stream->priv->closed = TRUE;
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (stream);
}

/**
 * xinput_stream_read_async:
 * @stream: A #xinput_stream_t.
 * @buffer: (array length=count) (element-type xuint8_t) (out caller-allocates):
 *     a buffer to read data into (which should be at least count bytes long).
 * @count: (in): the number of bytes that will be read from the stream
 * @io_priority: the [I/O priority][io-priority]
 * of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous read of @count bytes from the stream into the buffer
 * starting at @buffer. When the operation is finished @callback will be called.
 * You can then call xinput_stream_read_finish() to get the result of the
 * operation.
 *
 * During an async request no other sync and async calls are allowed on @stream, and will
 * result in %G_IO_ERROR_PENDING errors.
 *
 * A value of @count larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes read into the buffer will be passed to the
 * callback. It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file, but generally we try to read
 * as many bytes as requested. Zero is returned on end of file
 * (or if @count is zero),  but never otherwise.
 *
 * Any outstanding i/o request with higher priority (lower numerical value) will
 * be executed before an outstanding request with lower priority. Default
 * priority is %G_PRIORITY_DEFAULT.
 *
 * The asynchronous methods have a default fallback that uses threads to implement
 * asynchronicity, so they are optional for inheriting classes. However, if you
 * override one you must override all.
 **/
void
xinput_stream_read_async (xinput_stream_t        *stream,
			   void                *buffer,
			   xsize_t                count,
			   int                  io_priority,
			   xcancellable_t        *cancellable,
			   xasync_ready_callback_t  callback,
			   xpointer_t             user_data)
{
  GInputStreamClass *class;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_INPUT_STREAM (stream));
  g_return_if_fail (buffer != NULL);

  if (count == 0)
    {
      xtask_t *task;

      task = xtask_new (stream, cancellable, callback, user_data);
      xtask_set_source_tag (task, xinput_stream_read_async);
      xtask_return_int (task, 0);
      xobject_unref (task);
      return;
    }

  if (((xssize_t) count) < 0)
    {
      xtask_report_new_error (stream, callback, user_data,
                               xinput_stream_read_async,
                               G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                               _("Too large count value passed to %s"),
                               G_STRFUNC);
      return;
    }

  if (!xinput_stream_set_pending (stream, &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xinput_stream_read_async,
                           error);
      return;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  class->read_async (stream, buffer, count, io_priority, cancellable,
		     async_ready_callback_wrapper, user_data);
}

/**
 * xinput_stream_read_finish:
 * @stream: a #xinput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an asynchronous stream read operation.
 *
 * Returns: number of bytes read in, or -1 on error, or 0 on end of file.
 **/
xssize_t
xinput_stream_read_finish (xinput_stream_t  *stream,
			    xasync_result_t  *result,
			    xerror_t       **error)
{
  GInputStreamClass *class;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), -1);

  if (xasync_result_legacy_propagate_error (result, error))
    return -1;
  else if (xasync_result_is_tagged (result, xinput_stream_read_async))
    return xtask_propagate_int (XTASK (result), error);

  class = G_INPUT_STREAM_GET_CLASS (stream);
  return class->read_finish (stream, result, error);
}

typedef struct
{
  xchar_t *buffer;
  xsize_t to_read;
  xsize_t bytes_read;
} AsyncReadAll;

static void
free_async_read_all (xpointer_t data)
{
  g_slice_free (AsyncReadAll, data);
}

static void
read_all_callback (xobject_t      *stream,
                   xasync_result_t *result,
                   xpointer_t      user_data)
{
  xtask_t *task = user_data;
  AsyncReadAll *data = xtask_get_task_data (task);
  xboolean_t got_eof = FALSE;

  if (result)
    {
      xerror_t *error = NULL;
      xssize_t nread;

      nread = xinput_stream_read_finish (G_INPUT_STREAM (stream), result, &error);

      if (nread == -1)
        {
          xtask_return_error (task, error);
          xobject_unref (task);
          return;
        }

      g_assert_cmpint (nread, <=, data->to_read);
      data->to_read -= nread;
      data->bytes_read += nread;
      got_eof = (nread == 0);
    }

  if (got_eof || data->to_read == 0)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
    }

  else
    xinput_stream_read_async (G_INPUT_STREAM (stream),
                               data->buffer + data->bytes_read,
                               data->to_read,
                               xtask_get_priority (task),
                               xtask_get_cancellable (task),
                               read_all_callback, task);
}


static void
read_all_async_thread (xtask_t        *task,
                       xpointer_t      source_object,
                       xpointer_t      task_data,
                       xcancellable_t *cancellable)
{
  xinput_stream_t *stream = source_object;
  AsyncReadAll *data = task_data;
  xerror_t *error = NULL;

  if (xinput_stream_read_all (stream, data->buffer, data->to_read, &data->bytes_read,
                               xtask_get_cancellable (task), &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

/**
 * xinput_stream_read_all_async:
 * @stream: A #xinput_stream_t
 * @buffer: (array length=count) (element-type xuint8_t) (out caller-allocates):
 *     a buffer to read data into (which should be at least count bytes long)
 * @count: (in): the number of bytes that will be read from the stream
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous read of @count bytes from the stream into the
 * buffer starting at @buffer.
 *
 * This is the asynchronous equivalent of xinput_stream_read_all().
 *
 * Call xinput_stream_read_all_finish() to collect the result.
 *
 * Any outstanding I/O request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * Since: 2.44
 **/
void
xinput_stream_read_all_async (xinput_stream_t        *stream,
                               void                *buffer,
                               xsize_t                count,
                               int                  io_priority,
                               xcancellable_t        *cancellable,
                               xasync_ready_callback_t  callback,
                               xpointer_t             user_data)
{
  AsyncReadAll *data;
  xtask_t *task;

  g_return_if_fail (X_IS_INPUT_STREAM (stream));
  g_return_if_fail (buffer != NULL || count == 0);

  task = xtask_new (stream, cancellable, callback, user_data);
  data = g_slice_new0 (AsyncReadAll);
  data->buffer = buffer;
  data->to_read = count;

  xtask_set_source_tag (task, xinput_stream_read_all_async);
  xtask_set_task_data (task, data, free_async_read_all);
  xtask_set_priority (task, io_priority);

  /* If async reads are going to be handled via the threadpool anyway
   * then we may as well do it with a single dispatch instead of
   * bouncing in and out.
   */
  if (xinput_stream_async_read_is_via_threads (stream))
    {
      xtask_run_in_thread (task, read_all_async_thread);
      xobject_unref (task);
    }
  else
    read_all_callback (G_OBJECT (stream), NULL, task);
}

/**
 * xinput_stream_read_all_finish:
 * @stream: a #xinput_stream_t
 * @result: a #xasync_result_t
 * @bytes_read: (out): location to store the number of bytes that was read from the stream
 * @error: a #xerror_t location to store the error occurring, or %NULL to ignore
 *
 * Finishes an asynchronous stream read operation started with
 * xinput_stream_read_all_async().
 *
 * As a special exception to the normal conventions for functions that
 * use #xerror_t, if this function returns %FALSE (and sets @error) then
 * @bytes_read will be set to the number of bytes that were successfully
 * read before the error was encountered.  This functionality is only
 * available from C.  If you need it from another language then you must
 * write your own loop around xinput_stream_read_async().
 *
 * Returns: %TRUE on success, %FALSE if there was an error
 *
 * Since: 2.44
 **/
xboolean_t
xinput_stream_read_all_finish (xinput_stream_t  *stream,
                                xasync_result_t  *result,
                                xsize_t         *bytes_read,
                                xerror_t       **error)
{
  xtask_t *task;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  task = XTASK (result);

  if (bytes_read)
    {
      AsyncReadAll *data = xtask_get_task_data (task);

      *bytes_read = data->bytes_read;
    }

  return xtask_propagate_boolean (task, error);
}

static void
read_bytes_callback (xobject_t      *stream,
		     xasync_result_t *result,
		     xpointer_t      user_data)
{
  xtask_t *task = user_data;
  guchar *buf = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t nread;
  xbytes_t *bytes = NULL;

  nread = xinput_stream_read_finish (G_INPUT_STREAM (stream),
				      result, &error);
  if (nread == -1)
    {
      g_free (buf);
      xtask_return_error (task, error);
    }
  else if (nread == 0)
    {
      g_free (buf);
      bytes = xbytes_new_static ("", 0);
    }
  else
    bytes = xbytes_new_take (buf, nread);

  if (bytes)
    xtask_return_pointer (task, bytes, (xdestroy_notify_t)xbytes_unref);

  xobject_unref (task);
}

/**
 * xinput_stream_read_bytes_async:
 * @stream: A #xinput_stream_t.
 * @count: the number of bytes that will be read from the stream
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous read of @count bytes from the stream into a
 * new #xbytes_t. When the operation is finished @callback will be
 * called. You can then call xinput_stream_read_bytes_finish() to get the
 * result of the operation.
 *
 * During an async request no other sync and async calls are allowed
 * on @stream, and will result in %G_IO_ERROR_PENDING errors.
 *
 * A value of @count larger than %G_MAXSSIZE will cause a
 * %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the new #xbytes_t will be passed to the callback. It is
 * not an error if this is smaller than the requested size, as it can
 * happen e.g. near the end of a file, but generally we try to read as
 * many bytes as requested. Zero is returned on end of file (or if
 * @count is zero), but never otherwise.
 *
 * Any outstanding I/O request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * Since: 2.34
 **/
void
xinput_stream_read_bytes_async (xinput_stream_t          *stream,
				 xsize_t                  count,
				 int                    io_priority,
				 xcancellable_t          *cancellable,
				 xasync_ready_callback_t    callback,
				 xpointer_t               user_data)
{
  xtask_t *task;
  guchar *buf;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xinput_stream_read_bytes_async);

  buf = g_malloc (count);
  xtask_set_task_data (task, buf, NULL);

  xinput_stream_read_async (stream, buf, count,
                             io_priority, cancellable,
                             read_bytes_callback, task);
}

/**
 * xinput_stream_read_bytes_finish:
 * @stream: a #xinput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *   ignore.
 *
 * Finishes an asynchronous stream read-into-#xbytes_t operation.
 *
 * Returns: (transfer full): the newly-allocated #xbytes_t, or %NULL on error
 *
 * Since: 2.34
 **/
xbytes_t *
xinput_stream_read_bytes_finish (xinput_stream_t  *stream,
				  xasync_result_t  *result,
				  xerror_t       **error)
{
  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), NULL);
  g_return_val_if_fail (xtask_is_valid (result, stream), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

/**
 * xinput_stream_skip_async:
 * @stream: A #xinput_stream_t.
 * @count: the number of bytes that will be skipped from the stream
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request an asynchronous skip of @count bytes from the stream.
 * When the operation is finished @callback will be called.
 * You can then call xinput_stream_skip_finish() to get the result
 * of the operation.
 *
 * During an async request no other sync and async calls are allowed,
 * and will result in %G_IO_ERROR_PENDING errors.
 *
 * A value of @count larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes skipped will be passed to the callback.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file, but generally we try to skip
 * as many bytes as requested. Zero is returned on end of file
 * (or if @count is zero), but never otherwise.
 *
 * Any outstanding i/o request with higher priority (lower numerical value)
 * will be executed before an outstanding request with lower priority.
 * Default priority is %G_PRIORITY_DEFAULT.
 *
 * The asynchronous methods have a default fallback that uses threads to
 * implement asynchronicity, so they are optional for inheriting classes.
 * However, if you override one, you must override all.
 **/
void
xinput_stream_skip_async (xinput_stream_t        *stream,
			   xsize_t                count,
			   int                  io_priority,
			   xcancellable_t        *cancellable,
			   xasync_ready_callback_t  callback,
			   xpointer_t             user_data)
{
  GInputStreamClass *class;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_INPUT_STREAM (stream));

  if (count == 0)
    {
      xtask_t *task;

      task = xtask_new (stream, cancellable, callback, user_data);
      xtask_set_source_tag (task, xinput_stream_skip_async);
      xtask_return_int (task, 0);
      xobject_unref (task);
      return;
    }

  if (((xssize_t) count) < 0)
    {
      xtask_report_new_error (stream, callback, user_data,
                               xinput_stream_skip_async,
                               G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                               _("Too large count value passed to %s"),
                               G_STRFUNC);
      return;
    }

  if (!xinput_stream_set_pending (stream, &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xinput_stream_skip_async,
                           error);
      return;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  class->skip_async (stream, count, io_priority, cancellable,
		     async_ready_callback_wrapper, user_data);
}

/**
 * xinput_stream_skip_finish:
 * @stream: a #xinput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes a stream skip operation.
 *
 * Returns: the size of the bytes skipped, or `-1` on error.
 **/
xssize_t
xinput_stream_skip_finish (xinput_stream_t  *stream,
			    xasync_result_t  *result,
			    xerror_t       **error)
{
  GInputStreamClass *class;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), -1);

  if (xasync_result_legacy_propagate_error (result, error))
    return -1;
  else if (xasync_result_is_tagged (result, xinput_stream_skip_async))
    return xtask_propagate_int (XTASK (result), error);

  class = G_INPUT_STREAM_GET_CLASS (stream);
  return class->skip_finish (stream, result, error);
}

/**
 * xinput_stream_close_async:
 * @stream: A #xinput_stream_t.
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional cancellable object
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Requests an asynchronous closes of the stream, releasing resources related to it.
 * When the operation is finished @callback will be called.
 * You can then call xinput_stream_close_finish() to get the result of the
 * operation.
 *
 * For behaviour details see xinput_stream_close().
 *
 * The asynchronous methods have a default fallback that uses threads to implement
 * asynchronicity, so they are optional for inheriting classes. However, if you
 * override one you must override all.
 **/
void
xinput_stream_close_async (xinput_stream_t        *stream,
			    int                  io_priority,
			    xcancellable_t        *cancellable,
			    xasync_ready_callback_t  callback,
			    xpointer_t             user_data)
{
  GInputStreamClass *class;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_INPUT_STREAM (stream));

  if (stream->priv->closed)
    {
      xtask_t *task;

      task = xtask_new (stream, cancellable, callback, user_data);
      xtask_set_source_tag (task, xinput_stream_close_async);
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
      return;
    }

  if (!xinput_stream_set_pending (stream, &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xinput_stream_close_async,
                           error);
      return;
    }

  class = G_INPUT_STREAM_GET_CLASS (stream);
  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  class->close_async (stream, io_priority, cancellable,
		      async_ready_close_callback_wrapper, user_data);
}

/**
 * xinput_stream_close_finish:
 * @stream: a #xinput_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes closing a stream asynchronously, started from xinput_stream_close_async().
 *
 * Returns: %TRUE if the stream was closed successfully.
 **/
xboolean_t
xinput_stream_close_finish (xinput_stream_t  *stream,
			     xasync_result_t  *result,
			     xerror_t       **error)
{
  GInputStreamClass *class;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xinput_stream_close_async))
    return xtask_propagate_boolean (XTASK (result), error);

  class = G_INPUT_STREAM_GET_CLASS (stream);
  return class->close_finish (stream, result, error);
}

/**
 * xinput_stream_is_closed:
 * @stream: input stream.
 *
 * Checks if an input stream is closed.
 *
 * Returns: %TRUE if the stream is closed.
 **/
xboolean_t
xinput_stream_is_closed (xinput_stream_t *stream)
{
  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), TRUE);

  return stream->priv->closed;
}

/**
 * xinput_stream_has_pending:
 * @stream: input stream.
 *
 * Checks if an input stream has pending actions.
 *
 * Returns: %TRUE if @stream has pending actions.
 **/
xboolean_t
xinput_stream_has_pending (xinput_stream_t *stream)
{
  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), TRUE);

  return stream->priv->pending;
}

/**
 * xinput_stream_set_pending:
 * @stream: input stream
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
xinput_stream_set_pending (xinput_stream_t *stream, xerror_t **error)
{
  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);

  if (stream->priv->closed)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Stream is already closed"));
      return FALSE;
    }

  if (stream->priv->pending)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
		/* Translators: This is an error you get if there is already an
		 * operation running against this stream when you try to start
		 * one */
		 _("Stream has outstanding operation"));
      return FALSE;
    }

  stream->priv->pending = TRUE;
  return TRUE;
}

/**
 * xinput_stream_clear_pending:
 * @stream: input stream
 *
 * Clears the pending flag on @stream.
 **/
void
xinput_stream_clear_pending (xinput_stream_t *stream)
{
  g_return_if_fail (X_IS_INPUT_STREAM (stream));

  stream->priv->pending = FALSE;
}

/*< internal >
 * xinput_stream_async_read_is_via_threads:
 * @stream: input stream
 *
 * Checks if an input stream's read_async function uses threads.
 *
 * Returns: %TRUE if @stream's read_async function uses threads.
 **/
xboolean_t
xinput_stream_async_read_is_via_threads (xinput_stream_t *stream)
{
  GInputStreamClass *class;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);

  class = G_INPUT_STREAM_GET_CLASS (stream);

  return (class->read_async == xinput_stream_real_read_async &&
      !(X_IS_POLLABLE_INPUT_STREAM (stream) &&
        g_pollable_input_stream_can_poll (G_POLLABLE_INPUT_STREAM (stream))));
}

/*< internal >
 * xinput_stream_async_close_is_via_threads:
 * @stream: input stream
 *
 * Checks if an input stream's close_async function uses threads.
 *
 * Returns: %TRUE if @stream's close_async function uses threads.
 **/
xboolean_t
xinput_stream_async_close_is_via_threads (xinput_stream_t *stream)
{
  GInputStreamClass *class;

  g_return_val_if_fail (X_IS_INPUT_STREAM (stream), FALSE);

  class = G_INPUT_STREAM_GET_CLASS (stream);

  return class->close_async == xinput_stream_real_close_async;
}

/********************************************
 *   Default implementation of async ops    *
 ********************************************/

typedef struct {
  void   *buffer;
  xsize_t   count;
} ReadData;

static void
free_read_data (ReadData *op)
{
  g_slice_free (ReadData, op);
}

static void
read_async_thread (xtask_t        *task,
                   xpointer_t      source_object,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  xinput_stream_t *stream = source_object;
  ReadData *op = task_data;
  GInputStreamClass *class;
  xerror_t *error = NULL;
  xssize_t nread;

  class = G_INPUT_STREAM_GET_CLASS (stream);

  nread = class->read_fn (stream,
                          op->buffer, op->count,
                          xtask_get_cancellable (task),
                          &error);
  if (nread == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, nread);
}

static void read_async_pollable (xpollable_input_stream_t *stream,
                                 xtask_t                *task);

static xboolean_t
read_async_pollable_ready (xpollable_input_stream_t *stream,
			   xpointer_t              user_data)
{
  xtask_t *task = user_data;

  read_async_pollable (stream, task);
  return FALSE;
}

static void
read_async_pollable (xpollable_input_stream_t *stream,
                     xtask_t                *task)
{
  ReadData *op = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t nread;

  if (xtask_return_error_if_cancelled (task))
    return;

  nread = G_POLLABLE_INPUT_STREAM_GET_INTERFACE (stream)->
    read_nonblocking (stream, op->buffer, op->count, &error);

  if (xerror_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      xsource_t *source;

      xerror_free (error);

      source = g_pollable_input_stream_create_source (stream,
                                                      xtask_get_cancellable (task));
      xtask_attach_source (task, source,
                            (xsource_func_t) read_async_pollable_ready);
      xsource_unref (source);
      return;
    }

  if (nread == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, nread);
  /* xinput_stream_real_read_async() unrefs task */
}


static void
xinput_stream_real_read_async (xinput_stream_t        *stream,
				void                *buffer,
				xsize_t                count,
				int                  io_priority,
				xcancellable_t        *cancellable,
				xasync_ready_callback_t  callback,
				xpointer_t             user_data)
{
  xtask_t *task;
  ReadData *op;

  op = g_slice_new0 (ReadData);
  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xinput_stream_real_read_async);
  xtask_set_task_data (task, op, (xdestroy_notify_t) free_read_data);
  xtask_set_priority (task, io_priority);
  op->buffer = buffer;
  op->count = count;

  if (!xinput_stream_async_read_is_via_threads (stream))
    read_async_pollable (G_POLLABLE_INPUT_STREAM (stream), task);
  else
    xtask_run_in_thread (task, read_async_thread);
  xobject_unref (task);
}

static xssize_t
xinput_stream_real_read_finish (xinput_stream_t  *stream,
				 xasync_result_t  *result,
				 xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), -1);

  return xtask_propagate_int (XTASK (result), error);
}


static void
skip_async_thread (xtask_t        *task,
                   xpointer_t      source_object,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  xinput_stream_t *stream = source_object;
  xsize_t count = GPOINTER_TO_SIZE (task_data);
  GInputStreamClass *class;
  xerror_t *error = NULL;
  xssize_t ret;

  class = G_INPUT_STREAM_GET_CLASS (stream);
  ret = class->skip (stream, count,
                     xtask_get_cancellable (task),
                     &error);
  if (ret == -1)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, ret);
}

typedef struct {
  char buffer[8192];
  xsize_t count;
  xsize_t count_skipped;
} SkipFallbackAsyncData;

static void
skip_callback_wrapper (xobject_t      *source_object,
		       xasync_result_t *res,
		       xpointer_t      user_data)
{
  GInputStreamClass *class;
  xtask_t *task = user_data;
  SkipFallbackAsyncData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;
  xssize_t ret;

  ret = xinput_stream_read_finish (G_INPUT_STREAM (source_object), res, &error);

  if (ret > 0)
    {
      data->count -= ret;
      data->count_skipped += ret;

      if (data->count > 0)
	{
	  class = G_INPUT_STREAM_GET_CLASS (source_object);
	  class->read_async (G_INPUT_STREAM (source_object),
                             data->buffer, MIN (8192, data->count),
                             xtask_get_priority (task),
                             xtask_get_cancellable (task),
                             skip_callback_wrapper, task);
	  return;
	}
    }

  if (ret == -1 &&
      xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) &&
      data->count_skipped)
    {
      /* No error, return partial read */
      g_clear_error (&error);
    }

  if (error)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, data->count_skipped);
  xobject_unref (task);
 }

static void
xinput_stream_real_skip_async (xinput_stream_t        *stream,
				xsize_t                count,
				int                  io_priority,
				xcancellable_t        *cancellable,
				xasync_ready_callback_t  callback,
				xpointer_t             user_data)
{
  GInputStreamClass *class;
  SkipFallbackAsyncData *data;
  xtask_t *task;

  class = G_INPUT_STREAM_GET_CLASS (stream);

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xinput_stream_real_skip_async);
  xtask_set_priority (task, io_priority);

  if (xinput_stream_async_read_is_via_threads (stream))
    {
      /* Read is thread-using async fallback.
       * Make skip use threads too, so that we can use a possible sync skip
       * implementation. */
      xtask_set_task_data (task, GSIZE_TO_POINTER (count), NULL);

      xtask_run_in_thread (task, skip_async_thread);
      xobject_unref (task);
    }
  else
    {
      /* TODO: Skip fallback uses too much memory, should do multiple read calls */

      /* There is a custom async read function, lets use that. */
      data = g_new (SkipFallbackAsyncData, 1);
      data->count = count;
      data->count_skipped = 0;
      xtask_set_task_data (task, data, g_free);
      xtask_set_check_cancellable (task, FALSE);
      class->read_async (stream, data->buffer, MIN (8192, count), io_priority, cancellable,
			 skip_callback_wrapper, task);
    }

}

static xssize_t
xinput_stream_real_skip_finish (xinput_stream_t  *stream,
				 xasync_result_t  *result,
				 xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), -1);

  return xtask_propagate_int (XTASK (result), error);
}

static void
close_async_thread (xtask_t        *task,
                    xpointer_t      source_object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xinput_stream_t *stream = source_object;
  GInputStreamClass *class;
  xerror_t *error = NULL;
  xboolean_t result;

  class = G_INPUT_STREAM_GET_CLASS (stream);
  if (class->close_fn)
    {
      result = class->close_fn (stream,
                                xtask_get_cancellable (task),
                                &error);
      if (!result)
        {
          xtask_return_error (task, error);
          return;
        }
    }

  xtask_return_boolean (task, TRUE);
}

static void
xinput_stream_real_close_async (xinput_stream_t        *stream,
				 int                  io_priority,
				 xcancellable_t        *cancellable,
				 xasync_ready_callback_t  callback,
				 xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xinput_stream_real_close_async);
  xtask_set_check_cancellable (task, FALSE);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, close_async_thread);
  xobject_unref (task);
}

static xboolean_t
xinput_stream_real_close_finish (xinput_stream_t  *stream,
				  xasync_result_t  *result,
				  xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}
