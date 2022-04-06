/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2007 Jürg Billeter
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
 * Author: Christian Kellner <gicmo@gnome.org>
 */

#include "config.h"
#include "gbufferedinputstream.h"
#include "ginputstream.h"
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "gseekable.h"
#include "gioerror.h"
#include <string.h>
#include "glibintl.h"


/**
 * SECTION:gbufferedinputstream
 * @short_description: Buffered Input Stream
 * @include: gio/gio.h
 * @see_also: #xfilter_input_stream_t, #xinput_stream_t
 *
 * Buffered input stream implements #xfilter_input_stream_t and provides
 * for buffered reads.
 *
 * By default, #xbuffered_input_stream's buffer size is set at 4 kilobytes.
 *
 * To create a buffered input stream, use xbuffered_input_stream_new(),
 * or xbuffered_input_stream_new_sized() to specify the buffer's size at
 * construction.
 *
 * To get the size of a buffer within a buffered input stream, use
 * xbuffered_input_stream_get_buffer_size(). To change the size of a
 * buffered input stream's buffer, use
 * xbuffered_input_stream_set_buffer_size(). Note that the buffer's size
 * cannot be reduced below the size of the data within the buffer.
 */


#define DEFAULT_BUFFER_SIZE 4096

struct _GBufferedInputStreamPrivate {
  xuint8_t *buffer;
  xsize_t   len;
  xsize_t   pos;
  xsize_t   end;
  xasync_ready_callback_t outstanding_callback;
};

enum {
  PROP_0,
  PROP_BUFSIZE
};

static void xbuffered_input_stream_set_property  (xobject_t      *object,
                                                   xuint_t         prop_id,
                                                   const xvalue_t *value,
                                                   xparam_spec_t   *pspec);

static void xbuffered_input_stream_get_property  (xobject_t      *object,
                                                   xuint_t         prop_id,
                                                   xvalue_t       *value,
                                                   xparam_spec_t   *pspec);
static void xbuffered_input_stream_finalize      (xobject_t *object);


static xssize_t xbuffered_input_stream_skip             (xinput_stream_t          *stream,
                                                        xsize_t                  count,
                                                        xcancellable_t          *cancellable,
                                                        xerror_t               **error);
static void   xbuffered_input_stream_skip_async       (xinput_stream_t          *stream,
                                                        xsize_t                  count,
                                                        int                    io_priority,
                                                        xcancellable_t          *cancellable,
                                                        xasync_ready_callback_t    callback,
                                                        xpointer_t               user_data);
static xssize_t xbuffered_input_stream_skip_finish      (xinput_stream_t          *stream,
                                                        xasync_result_t          *result,
                                                        xerror_t               **error);
static xssize_t xbuffered_input_stream_read             (xinput_stream_t          *stream,
                                                        void                  *buffer,
                                                        xsize_t                  count,
                                                        xcancellable_t          *cancellable,
                                                        xerror_t               **error);
static xssize_t xbuffered_input_stream_real_fill        (xbuffered_input_stream  *stream,
                                                        xssize_t                 count,
                                                        xcancellable_t          *cancellable,
                                                        xerror_t               **error);
static void   xbuffered_input_stream_real_fill_async  (xbuffered_input_stream  *stream,
                                                        xssize_t                 count,
                                                        int                    io_priority,
                                                        xcancellable_t          *cancellable,
                                                        xasync_ready_callback_t    callback,
                                                        xpointer_t               user_data);
static xssize_t xbuffered_input_stream_real_fill_finish (xbuffered_input_stream  *stream,
                                                        xasync_result_t          *result,
                                                        xerror_t               **error);

static void     xbuffered_input_stream_seekable_iface_init (xseekable_iface_t  *iface);
static xoffset_t  xbuffered_input_stream_tell                (xseekable__t       *seekable);
static xboolean_t xbuffered_input_stream_can_seek            (xseekable__t       *seekable);
static xboolean_t xbuffered_input_stream_seek                (xseekable__t       *seekable,
							     xoffset_t          offset,
							     GSeekType        type,
							     xcancellable_t    *cancellable,
							     xerror_t         **error);
static xboolean_t xbuffered_input_stream_can_truncate        (xseekable__t       *seekable);
static xboolean_t xbuffered_input_stream_truncate            (xseekable__t       *seekable,
							     xoffset_t          offset,
							     xcancellable_t    *cancellable,
							     xerror_t         **error);

static void compact_buffer (xbuffered_input_stream *stream);

G_DEFINE_TYPE_WITH_CODE (xbuffered_input_stream,
			 xbuffered_input_stream,
			 XTYPE_FILTER_INPUT_STREAM,
                         G_ADD_PRIVATE (xbuffered_input_stream)
			 G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
						xbuffered_input_stream_seekable_iface_init))

static void
xbuffered_input_stream_class_init (xbuffered_input_stream_class_t *klass)
{
  xobject_class_t *object_class;
  GInputStreamClass *istream_class;
  xbuffered_input_stream_class_t *bstream_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = xbuffered_input_stream_get_property;
  object_class->set_property = xbuffered_input_stream_set_property;
  object_class->finalize     = xbuffered_input_stream_finalize;

  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->skip = xbuffered_input_stream_skip;
  istream_class->skip_async  = xbuffered_input_stream_skip_async;
  istream_class->skip_finish = xbuffered_input_stream_skip_finish;
  istream_class->read_fn = xbuffered_input_stream_read;

  bstream_class = G_BUFFERED_INPUT_STREAM_CLASS (klass);
  bstream_class->fill = xbuffered_input_stream_real_fill;
  bstream_class->fill_async = xbuffered_input_stream_real_fill_async;
  bstream_class->fill_finish = xbuffered_input_stream_real_fill_finish;

  xobject_class_install_property (object_class,
                                   PROP_BUFSIZE,
                                   g_param_spec_uint ("buffer-size",
                                                      P_("Buffer Size"),
                                                      P_("The size of the backend buffer"),
                                                      1,
                                                      G_MAXUINT,
                                                      DEFAULT_BUFFER_SIZE,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));


}

/**
 * xbuffered_input_stream_get_buffer_size:
 * @stream: a #xbuffered_input_stream
 *
 * Gets the size of the input buffer.
 *
 * Returns: the current buffer size.
 */
xsize_t
xbuffered_input_stream_get_buffer_size (xbuffered_input_stream  *stream)
{
  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), 0);

  return stream->priv->len;
}

/**
 * xbuffered_input_stream_set_buffer_size:
 * @stream: a #xbuffered_input_stream
 * @size: a #xsize_t
 *
 * Sets the size of the internal buffer of @stream to @size, or to the
 * size of the contents of the buffer. The buffer can never be resized
 * smaller than its current contents.
 */
void
xbuffered_input_stream_set_buffer_size (xbuffered_input_stream *stream,
                                         xsize_t                 size)
{
  xbuffered_input_stream_private_t *priv;
  xsize_t in_buffer;
  xuint8_t *buffer;

  g_return_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream));

  priv = stream->priv;

  if (priv->len == size)
    return;

  if (priv->buffer)
    {
      in_buffer = priv->end - priv->pos;

      /* Never resize smaller than current buffer contents */
      size = MAX (size, in_buffer);

      buffer = g_malloc (size);
      memcpy (buffer, priv->buffer + priv->pos, in_buffer);
      priv->len = size;
      priv->pos = 0;
      priv->end = in_buffer;
      g_free (priv->buffer);
      priv->buffer = buffer;
    }
  else
    {
      priv->len = size;
      priv->pos = 0;
      priv->end = 0;
      priv->buffer = g_malloc (size);
    }

  xobject_notify (G_OBJECT (stream), "buffer-size");
}

static void
xbuffered_input_stream_set_property (xobject_t      *object,
                                      xuint_t         prop_id,
                                      const xvalue_t *value,
                                      xparam_spec_t   *pspec)
{
  xbuffered_input_stream        *bstream;

  bstream = G_BUFFERED_INPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_BUFSIZE:
      xbuffered_input_stream_set_buffer_size (bstream, xvalue_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xbuffered_input_stream_get_property (xobject_t    *object,
                                      xuint_t       prop_id,
                                      xvalue_t     *value,
                                      xparam_spec_t *pspec)
{
  xbuffered_input_stream_private_t *priv;
  xbuffered_input_stream        *bstream;

  bstream = G_BUFFERED_INPUT_STREAM (object);
  priv = bstream->priv;

  switch (prop_id)
    {
    case PROP_BUFSIZE:
      xvalue_set_uint (value, priv->len);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xbuffered_input_stream_finalize (xobject_t *object)
{
  xbuffered_input_stream_private_t *priv;
  xbuffered_input_stream        *stream;

  stream = G_BUFFERED_INPUT_STREAM (object);
  priv = stream->priv;

  g_free (priv->buffer);

  G_OBJECT_CLASS (xbuffered_input_stream_parent_class)->finalize (object);
}

static void
xbuffered_input_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell         = xbuffered_input_stream_tell;
  iface->can_seek     = xbuffered_input_stream_can_seek;
  iface->seek         = xbuffered_input_stream_seek;
  iface->can_truncate = xbuffered_input_stream_can_truncate;
  iface->truncate_fn  = xbuffered_input_stream_truncate;
}

static void
xbuffered_input_stream_init (xbuffered_input_stream *stream)
{
  stream->priv = xbuffered_input_stream_get_instance_private (stream);
}


/**
 * xbuffered_input_stream_new:
 * @base_stream: a #xinput_stream_t
 *
 * Creates a new #xinput_stream_t from the given @base_stream, with
 * a buffer set to the default size (4 kilobytes).
 *
 * Returns: a #xinput_stream_t for the given @base_stream.
 */
xinput_stream_t *
xbuffered_input_stream_new (xinput_stream_t *base_stream)
{
  xinput_stream_t *stream;

  g_return_val_if_fail (X_IS_INPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_BUFFERED_INPUT_STREAM,
                         "base-stream", base_stream,
                         NULL);

  return stream;
}

/**
 * xbuffered_input_stream_new_sized:
 * @base_stream: a #xinput_stream_t
 * @size: a #xsize_t
 *
 * Creates a new #xbuffered_input_stream from the given @base_stream,
 * with a buffer set to @size.
 *
 * Returns: a #xinput_stream_t.
 */
xinput_stream_t *
xbuffered_input_stream_new_sized (xinput_stream_t *base_stream,
                                   xsize_t         size)
{
  xinput_stream_t *stream;

  g_return_val_if_fail (X_IS_INPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_BUFFERED_INPUT_STREAM,
                         "base-stream", base_stream,
                         "buffer-size", (xuint_t)size,
                         NULL);

  return stream;
}

/**
 * xbuffered_input_stream_fill:
 * @stream: a #xbuffered_input_stream
 * @count: the number of bytes that will be read from the stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to read @count bytes from the stream into the buffer.
 * Will block during this read.
 *
 * If @count is zero, returns zero and does nothing. A value of @count
 * larger than %G_MAXSSIZE will cause a %G_IO_ERROR_INVALID_ARGUMENT error.
 *
 * On success, the number of bytes read into the buffer is returned.
 * It is not an error if this is not the same as the requested size, as it
 * can happen e.g. near the end of a file. Zero is returned on end of file
 * (or if @count is zero),  but never otherwise.
 *
 * If @count is -1 then the attempted read size is equal to the number of
 * bytes that are required to fill the buffer.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error -1 is returned and @error is set accordingly.
 *
 * For the asynchronous, non-blocking, version of this function, see
 * xbuffered_input_stream_fill_async().
 *
 * Returns: the number of bytes read into @stream's buffer, up to @count,
 *     or -1 on error.
 */
xssize_t
xbuffered_input_stream_fill (xbuffered_input_stream  *stream,
                              xssize_t                 count,
                              xcancellable_t          *cancellable,
                              xerror_t               **error)
{
  xbuffered_input_stream_class_t *class;
  xinput_stream_t *input_stream;
  xssize_t res;

  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), -1);

  input_stream = G_INPUT_STREAM (stream);

  if (count < -1)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                   _("Too large count value passed to %s"), G_STRFUNC);
      return -1;
    }

  if (!xinput_stream_set_pending (input_stream, error))
    return -1;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);
  res = class->fill (stream, count, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (input_stream);

  return res;
}

static void
async_fill_callback_wrapper (xobject_t      *source_object,
                             xasync_result_t *res,
                             xpointer_t      user_data)
{
  xbuffered_input_stream *stream = G_BUFFERED_INPUT_STREAM (source_object);

  xinput_stream_clear_pending (G_INPUT_STREAM (stream));
  (*stream->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (stream);
}

/**
 * xbuffered_input_stream_fill_async:
 * @stream: a #xbuffered_input_stream
 * @count: the number of bytes that will be read from the stream
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): a #xpointer_t
 *
 * Reads data into @stream's buffer asynchronously, up to @count size.
 * @io_priority can be used to prioritize reads. For the synchronous
 * version of this function, see xbuffered_input_stream_fill().
 *
 * If @count is -1 then the attempted read size is equal to the number
 * of bytes that are required to fill the buffer.
 */
void
xbuffered_input_stream_fill_async (xbuffered_input_stream *stream,
                                    xssize_t                count,
                                    int                   io_priority,
                                    xcancellable_t         *cancellable,
                                    xasync_ready_callback_t   callback,
                                    xpointer_t              user_data)
{
  xbuffered_input_stream_class_t *class;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream));

  if (count == 0)
    {
      xtask_t *task;

      task = xtask_new (stream, cancellable, callback, user_data);
      xtask_set_source_tag (task, xbuffered_input_stream_fill_async);
      xtask_return_int (task, 0);
      xobject_unref (task);
      return;
    }

  if (count < -1)
    {
      xtask_report_new_error (stream, callback, user_data,
                               xbuffered_input_stream_fill_async,
                               G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
			       _("Too large count value passed to %s"),
			       G_STRFUNC);
      return;
    }

  if (!xinput_stream_set_pending (G_INPUT_STREAM (stream), &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xbuffered_input_stream_fill_async,
                           error);
      return;
    }

  class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);

  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  class->fill_async (stream, count, io_priority, cancellable,
                     async_fill_callback_wrapper, user_data);
}

/**
 * xbuffered_input_stream_fill_finish:
 * @stream: a #xbuffered_input_stream
 * @result: a #xasync_result_t
 * @error: a #xerror_t
 *
 * Finishes an asynchronous read.
 *
 * Returns: a #xssize_t of the read stream, or `-1` on an error.
 */
xssize_t
xbuffered_input_stream_fill_finish (xbuffered_input_stream  *stream,
                                     xasync_result_t          *result,
                                     xerror_t               **error)
{
  xbuffered_input_stream_class_t *class;

  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), -1);

  if (xasync_result_legacy_propagate_error (result, error))
    return -1;
  else if (xasync_result_is_tagged (result, xbuffered_input_stream_fill_async))
    return xtask_propagate_int (XTASK (result), error);

  class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);
  return class->fill_finish (stream, result, error);
}

/**
 * xbuffered_input_stream_get_available:
 * @stream: #xbuffered_input_stream
 *
 * Gets the size of the available data within the stream.
 *
 * Returns: size of the available stream.
 */
xsize_t
xbuffered_input_stream_get_available (xbuffered_input_stream *stream)
{
  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), -1);

  return stream->priv->end - stream->priv->pos;
}

/**
 * xbuffered_input_stream_peek:
 * @stream: a #xbuffered_input_stream
 * @buffer: (array length=count) (element-type xuint8_t): a pointer to
 *   an allocated chunk of memory
 * @offset: a #xsize_t
 * @count: a #xsize_t
 *
 * Peeks in the buffer, copying data of size @count into @buffer,
 * offset @offset bytes.
 *
 * Returns: a #xsize_t of the number of bytes peeked, or -1 on error.
 */
xsize_t
xbuffered_input_stream_peek (xbuffered_input_stream *stream,
                              void                 *buffer,
                              xsize_t                 offset,
                              xsize_t                 count)
{
  xsize_t available;
  xsize_t end;

  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), -1);
  g_return_val_if_fail (buffer != NULL, -1);

  available = xbuffered_input_stream_get_available (stream);

  if (offset > available)
    return 0;

  end = MIN (offset + count, available);
  count = end - offset;

  memcpy (buffer, stream->priv->buffer + stream->priv->pos + offset, count);
  return count;
}

/**
 * xbuffered_input_stream_peek_buffer:
 * @stream: a #xbuffered_input_stream
 * @count: (out): a #xsize_t to get the number of bytes available in the buffer
 *
 * Returns the buffer with the currently available bytes. The returned
 * buffer must not be modified and will become invalid when reading from
 * the stream or filling the buffer.
 *
 * Returns: (array length=count) (element-type xuint8_t) (transfer none):
 *          read-only buffer
 */
const void*
xbuffered_input_stream_peek_buffer (xbuffered_input_stream *stream,
                                     xsize_t                *count)
{
  xbuffered_input_stream_private_t *priv;

  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), NULL);

  priv = stream->priv;

  if (count)
    *count = priv->end - priv->pos;

  return priv->buffer + priv->pos;
}

static void
compact_buffer (xbuffered_input_stream *stream)
{
  xbuffered_input_stream_private_t *priv;
  xsize_t current_size;

  priv = stream->priv;

  current_size = priv->end - priv->pos;

  memmove (priv->buffer, priv->buffer + priv->pos, current_size);

  priv->pos = 0;
  priv->end = current_size;
}

static xssize_t
xbuffered_input_stream_real_fill (xbuffered_input_stream  *stream,
                                   xssize_t                 count,
                                   xcancellable_t          *cancellable,
                                   xerror_t               **error)
{
  xbuffered_input_stream_private_t *priv;
  xinput_stream_t *base_stream;
  xssize_t nread;
  xsize_t in_buffer;

  priv = stream->priv;

  if (count == -1)
    count = priv->len;

  in_buffer = priv->end - priv->pos;

  /* Never fill more than can fit in the buffer */
  count = MIN ((xsize_t) count, priv->len - in_buffer);

  /* If requested length does not fit at end, compact */
  if (priv->len - priv->end < (xsize_t) count)
    compact_buffer (stream);

  base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;
  nread = xinput_stream_read (base_stream,
                               priv->buffer + priv->end,
                               count,
                               cancellable,
                               error);

  if (nread > 0)
    priv->end += nread;

  return nread;
}

static xssize_t
xbuffered_input_stream_skip (xinput_stream_t  *stream,
                              xsize_t          count,
                              xcancellable_t  *cancellable,
                              xerror_t       **error)
{
  xbuffered_input_stream        *bstream;
  xbuffered_input_stream_private_t *priv;
  xbuffered_input_stream_class_t *class;
  xinput_stream_t *base_stream;
  xsize_t available, bytes_skipped;
  xssize_t nread;

  bstream = G_BUFFERED_INPUT_STREAM (stream);
  priv = bstream->priv;

  available = priv->end - priv->pos;

  if (count <= available)
    {
      priv->pos += count;
      return count;
    }

  /* Full request not available, skip all currently available and
   * request refill for more
   */

  priv->pos = 0;
  priv->end = 0;
  bytes_skipped = available;
  count -= available;

  if (bytes_skipped > 0)
    error = NULL; /* Ignore further errors if we already read some data */

  if (count > priv->len)
    {
      /* Large request, shortcut buffer */

      base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;

      nread = xinput_stream_skip (base_stream,
                                   count,
                                   cancellable,
                                   error);

      if (nread < 0 && bytes_skipped == 0)
        return -1;

      if (nread > 0)
        bytes_skipped += nread;

      return bytes_skipped;
    }

  class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);
  nread = class->fill (bstream, priv->len, cancellable, error);

  if (nread < 0)
    {
      if (bytes_skipped == 0)
        return -1;
      else
        return bytes_skipped;
    }

  available = priv->end - priv->pos;
  count = MIN (count, available);

  bytes_skipped += count;
  priv->pos += count;

  return bytes_skipped;
}

static xssize_t
xbuffered_input_stream_read (xinput_stream_t *stream,
                              void         *buffer,
                              xsize_t         count,
                              xcancellable_t *cancellable,
                              xerror_t      **error)
{
  xbuffered_input_stream        *bstream;
  xbuffered_input_stream_private_t *priv;
  xbuffered_input_stream_class_t *class;
  xinput_stream_t *base_stream;
  xsize_t available, bytes_read;
  xssize_t nread;

  bstream = G_BUFFERED_INPUT_STREAM (stream);
  priv = bstream->priv;

  available = priv->end - priv->pos;

  if (count <= available)
    {
      memcpy (buffer, priv->buffer + priv->pos, count);
      priv->pos += count;
      return count;
    }

  /* Full request not available, read all currently available and
   * request refill for more
   */

  memcpy (buffer, priv->buffer + priv->pos, available);
  priv->pos = 0;
  priv->end = 0;
  bytes_read = available;
  count -= available;

  if (bytes_read > 0)
    error = NULL; /* Ignore further errors if we already read some data */

  if (count > priv->len)
    {
      /* Large request, shortcut buffer */

      base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;

      nread = xinput_stream_read (base_stream,
                                   (char *)buffer + bytes_read,
                                   count,
                                   cancellable,
                                   error);

      if (nread < 0 && bytes_read == 0)
        return -1;

      if (nread > 0)
        bytes_read += nread;

      return bytes_read;
    }

  class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);
  nread = class->fill (bstream, priv->len, cancellable, error);
  if (nread < 0)
    {
      if (bytes_read == 0)
        return -1;
      else
        return bytes_read;
    }

  available = priv->end - priv->pos;
  count = MIN (count, available);

  memcpy ((char *)buffer + bytes_read, (char *)priv->buffer + priv->pos, count);
  bytes_read += count;
  priv->pos += count;

  return bytes_read;
}

static xoffset_t
xbuffered_input_stream_tell (xseekable__t *seekable)
{
  xbuffered_input_stream        *bstream;
  xbuffered_input_stream_private_t *priv;
  xinput_stream_t *base_stream;
  xseekable__t    *base_stream_seekable;
  xsize_t available;
  xoffset_t base_offset;

  bstream = G_BUFFERED_INPUT_STREAM (seekable);
  priv = bstream->priv;

  base_stream = G_FILTER_INPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    return 0;
  base_stream_seekable = G_SEEKABLE (base_stream);

  available = priv->end - priv->pos;
  base_offset = xseekable_tell (base_stream_seekable);

  return base_offset - available;
}

static xboolean_t
xbuffered_input_stream_can_seek (xseekable__t *seekable)
{
  xinput_stream_t *base_stream;

  base_stream = G_FILTER_INPUT_STREAM (seekable)->base_stream;
  return X_IS_SEEKABLE (base_stream) && xseekable_can_seek (G_SEEKABLE (base_stream));
}

static xboolean_t
xbuffered_input_stream_seek (xseekable__t     *seekable,
			      xoffset_t        offset,
			      GSeekType      type,
			      xcancellable_t  *cancellable,
			      xerror_t       **error)
{
  xbuffered_input_stream        *bstream;
  xbuffered_input_stream_private_t *priv;
  xinput_stream_t *base_stream;
  xseekable__t *base_stream_seekable;

  bstream = G_BUFFERED_INPUT_STREAM (seekable);
  priv = bstream->priv;

  base_stream = G_FILTER_INPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Seek not supported on base stream"));
      return FALSE;
    }

  base_stream_seekable = G_SEEKABLE (base_stream);

  if (type == G_SEEK_CUR)
    {
      if (offset <= (xoffset_t) (priv->end - priv->pos) &&
          offset >= (xoffset_t) -priv->pos)
	{
	  priv->pos += offset;
	  return TRUE;
	}
      else
	{
	  offset -= priv->end - priv->pos;
	}
    }

  if (xseekable_seek (base_stream_seekable, offset, type, cancellable, error))
    {
      priv->pos = 0;
      priv->end = 0;
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static xboolean_t
xbuffered_input_stream_can_truncate (xseekable__t *seekable)
{
  return FALSE;
}

static xboolean_t
xbuffered_input_stream_truncate (xseekable__t     *seekable,
				  xoffset_t        offset,
				  xcancellable_t  *cancellable,
				  xerror_t       **error)
{
  g_set_error_literal (error,
		       G_IO_ERROR,
		       G_IO_ERROR_NOT_SUPPORTED,
		       _("Cannot truncate xbuffered_input_stream"));
  return FALSE;
}

/**
 * xbuffered_input_stream_read_byte:
 * @stream: a #xbuffered_input_stream
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Tries to read a single byte from the stream or the buffer. Will block
 * during this read.
 *
 * On success, the byte read from the stream is returned. On end of stream
 * -1 is returned but it's not an exceptional error and @error is not set.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 *
 * On error -1 is returned and @error is set accordingly.
 *
 * Returns: the byte read from the @stream, or -1 on end of stream or error.
 */
int
xbuffered_input_stream_read_byte (xbuffered_input_stream  *stream,
                                   xcancellable_t          *cancellable,
                                   xerror_t               **error)
{
  xbuffered_input_stream_private_t *priv;
  xbuffered_input_stream_class_t *class;
  xinput_stream_t *input_stream;
  xsize_t available;
  xssize_t nread;

  g_return_val_if_fail (X_IS_BUFFERED_INPUT_STREAM (stream), -1);

  priv = stream->priv;
  input_stream = G_INPUT_STREAM (stream);

  if (xinput_stream_is_closed (input_stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Stream is already closed"));
      return -1;
    }

  if (!xinput_stream_set_pending (input_stream, error))
    return -1;

  available = priv->end - priv->pos;

  if (available != 0)
    {
      xinput_stream_clear_pending (input_stream);
      return priv->buffer[priv->pos++];
    }

  /* Byte not available, request refill for more */

  if (cancellable)
    g_cancellable_push_current (cancellable);

  priv->pos = 0;
  priv->end = 0;

  class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);
  nread = class->fill (stream, priv->len, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (input_stream);

  if (nread <= 0)
    return -1; /* error or end of stream */

  return priv->buffer[priv->pos++];
}

/* ************************** */
/* Async stuff implementation */
/* ************************** */

static void
fill_async_callback (xobject_t      *source_object,
                     xasync_result_t *result,
                     xpointer_t      user_data)
{
  xerror_t *error;
  xssize_t res;
  xtask_t *task = user_data;

  error = NULL;
  res = xinput_stream_read_finish (G_INPUT_STREAM (source_object),
                                    result, &error);
  if (res == -1)
    xtask_return_error (task, error);
  else
    {
      xbuffered_input_stream *stream;
      xbuffered_input_stream_private_t *priv;

      stream = xtask_get_source_object (task);
      priv = G_BUFFERED_INPUT_STREAM (stream)->priv;

      g_assert_cmpint (priv->end + res, <=, priv->len);
      priv->end += res;

      xtask_return_int (task, res);
    }

  xobject_unref (task);
}

static void
xbuffered_input_stream_real_fill_async (xbuffered_input_stream *stream,
                                         xssize_t                count,
                                         int                   io_priority,
                                         xcancellable_t         *cancellable,
                                         xasync_ready_callback_t   callback,
                                         xpointer_t              user_data)
{
  xbuffered_input_stream_private_t *priv;
  xinput_stream_t *base_stream;
  xtask_t *task;
  xsize_t in_buffer;

  priv = stream->priv;

  if (count == -1)
    count = priv->len;

  in_buffer = priv->end - priv->pos;

  /* Never fill more than can fit in the buffer */
  count = MIN ((xsize_t) count, priv->len - in_buffer);

  /* If requested length does not fit at end, compact */
  if (priv->len - priv->end < (xsize_t) count)
    compact_buffer (stream);

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xbuffered_input_stream_real_fill_async);

  base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;
  xinput_stream_read_async (base_stream,
                             priv->buffer + priv->end,
                             count,
                             io_priority,
                             cancellable,
                             fill_async_callback,
                             task);
}

static xssize_t
xbuffered_input_stream_real_fill_finish (xbuffered_input_stream *stream,
                                          xasync_result_t         *result,
                                          xerror_t              **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), -1);

  return xtask_propagate_int (XTASK (result), error);
}

typedef struct
{
  xsize_t bytes_skipped;
  xsize_t count;
} SkipAsyncData;

static void
free_skip_async_data (xpointer_t _data)
{
  SkipAsyncData *data = _data;
  g_slice_free (SkipAsyncData, data);
}

static void
large_skip_callback (xobject_t      *source_object,
                     xasync_result_t *result,
                     xpointer_t      user_data)
{
  xtask_t *task = XTASK (user_data);
  SkipAsyncData *data;
  xerror_t *error;
  xssize_t nread;

  data = xtask_get_task_data (task);

  error = NULL;
  nread = xinput_stream_skip_finish (G_INPUT_STREAM (source_object),
                                      result, &error);

  /* Only report the error if we've not already read some data */
  if (nread < 0 && data->bytes_skipped == 0)
    xtask_return_error (task, error);
  else
    {
      if (error)
	xerror_free (error);

      if (nread > 0)
	data->bytes_skipped += nread;

      xtask_return_int (task, data->bytes_skipped);
    }

  xobject_unref (task);
}

static void
skip_fill_buffer_callback (xobject_t      *source_object,
                           xasync_result_t *result,
                           xpointer_t      user_data)
{
  xtask_t *task = XTASK (user_data);
  xbuffered_input_stream *bstream;
  xbuffered_input_stream_private_t *priv;
  SkipAsyncData *data;
  xerror_t *error;
  xssize_t nread;
  xsize_t available;

  bstream = G_BUFFERED_INPUT_STREAM (source_object);
  priv = bstream->priv;

  data = xtask_get_task_data (task);

  error = NULL;
  nread = xbuffered_input_stream_fill_finish (bstream,
                                               result, &error);

  if (nread < 0 && data->bytes_skipped == 0)
    xtask_return_error (task, error);
  else
    {
      if (error)
	xerror_free (error);

      if (nread > 0)
	{
	  available = priv->end - priv->pos;
	  data->count = MIN (data->count, available);

	  data->bytes_skipped += data->count;
	  priv->pos += data->count;
	}

      g_assert (data->bytes_skipped <= G_MAXSSIZE);
      xtask_return_int (task, data->bytes_skipped);
    }

  xobject_unref (task);
}

static void
xbuffered_input_stream_skip_async (xinput_stream_t        *stream,
                                    xsize_t                count,
                                    int                  io_priority,
                                    xcancellable_t        *cancellable,
                                    xasync_ready_callback_t  callback,
                                    xpointer_t             user_data)
{
  xbuffered_input_stream *bstream;
  xbuffered_input_stream_private_t *priv;
  xbuffered_input_stream_class_t *class;
  xinput_stream_t *base_stream;
  xsize_t available;
  xtask_t *task;
  SkipAsyncData *data;

  bstream = G_BUFFERED_INPUT_STREAM (stream);
  priv = bstream->priv;

  data = g_slice_new (SkipAsyncData);
  data->bytes_skipped = 0;
  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xbuffered_input_stream_skip_async);
  xtask_set_task_data (task, data, free_skip_async_data);

  available = priv->end - priv->pos;

  if (count <= available)
    {
      priv->pos += count;

      xtask_return_int (task, count);
      xobject_unref (task);
      return;
    }

  /* Full request not available, skip all currently available
   * and request refill for more
   */

  priv->pos = 0;
  priv->end = 0;

  count -= available;

  data->bytes_skipped = available;
  data->count = count;

  if (count > priv->len)
    {
      /* Large request, shortcut buffer */
      base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;

      /* If 'count > G_MAXSSIZE then 'xinput_stream_skip_async()'
       * will return an error anyway before calling this.
       * Assert that this is never called for too big `count` for clarity. */
      g_assert ((xssize_t) count >= 0);
      xinput_stream_skip_async (base_stream,
                                 count,
                                 io_priority, cancellable,
                                 large_skip_callback,
                                 task);
    }
  else
    {
      class = G_BUFFERED_INPUT_STREAM_GET_CLASS (stream);
      class->fill_async (bstream, priv->len, io_priority, cancellable,
                         skip_fill_buffer_callback, task);
    }
}

static xssize_t
xbuffered_input_stream_skip_finish (xinput_stream_t   *stream,
                                     xasync_result_t   *result,
                                     xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), -1);

  return xtask_propagate_int (XTASK (result), error);
}
