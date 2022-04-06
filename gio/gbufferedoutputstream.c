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
 * Author: Christian Kellner <gicmo@gnome.org>
 */

#include "config.h"
#include "gbufferedoutputstream.h"
#include "goutputstream.h"
#include "gseekable.h"
#include "gtask.h"
#include "string.h"
#include "gioerror.h"
#include "glibintl.h"

/**
 * SECTION:gbufferedoutputstream
 * @short_description: Buffered Output Stream
 * @include: gio/gio.h
 * @see_also: #xfilter_output_stream_t, #xoutput_stream_t
 *
 * Buffered output stream implements #xfilter_output_stream_t and provides
 * for buffered writes.
 *
 * By default, #xbuffered_output_stream_t's buffer size is set at 4 kilobytes.
 *
 * To create a buffered output stream, use g_buffered_output_stream_new(),
 * or g_buffered_output_stream_new_sized() to specify the buffer's size
 * at construction.
 *
 * To get the size of a buffer within a buffered input stream, use
 * g_buffered_output_stream_get_buffer_size(). To change the size of a
 * buffered output stream's buffer, use
 * g_buffered_output_stream_set_buffer_size(). Note that the buffer's
 * size cannot be reduced below the size of the data within the buffer.
 **/

#define DEFAULT_BUFFER_SIZE 4096

struct _GBufferedOutputStreamPrivate {
  xuint8_t *buffer;
  xsize_t   len;
  xoffset_t pos;
  xboolean_t auto_grow;
};

enum {
  PROP_0,
  PROP_BUFSIZE,
  PROP_AUTO_GROW
};

static void     g_buffered_output_stream_set_property (xobject_t      *object,
                                                       xuint_t         prop_id,
                                                       const xvalue_t *value,
                                                       xparam_spec_t   *pspec);

static void     g_buffered_output_stream_get_property (xobject_t    *object,
                                                       xuint_t       prop_id,
                                                       xvalue_t     *value,
                                                       xparam_spec_t *pspec);
static void     g_buffered_output_stream_finalize     (xobject_t *object);


static xssize_t   g_buffered_output_stream_write        (xoutput_stream_t *stream,
                                                       const void    *buffer,
                                                       xsize_t          count,
                                                       xcancellable_t  *cancellable,
                                                       xerror_t       **error);
static xboolean_t g_buffered_output_stream_flush        (xoutput_stream_t    *stream,
                                                       xcancellable_t  *cancellable,
                                                       xerror_t          **error);
static xboolean_t g_buffered_output_stream_close        (xoutput_stream_t  *stream,
                                                       xcancellable_t   *cancellable,
                                                       xerror_t        **error);

static void     g_buffered_output_stream_flush_async  (xoutput_stream_t        *stream,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              data);
static xboolean_t g_buffered_output_stream_flush_finish (xoutput_stream_t        *stream,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);
static void     g_buffered_output_stream_close_async  (xoutput_stream_t        *stream,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              data);
static xboolean_t g_buffered_output_stream_close_finish (xoutput_stream_t        *stream,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

static void     g_buffered_output_stream_seekable_iface_init (xseekable_iface_t  *iface);
static xoffset_t  g_buffered_output_stream_tell                (xseekable__t       *seekable);
static xboolean_t g_buffered_output_stream_can_seek            (xseekable__t       *seekable);
static xboolean_t g_buffered_output_stream_seek                (xseekable__t       *seekable,
							      xoffset_t          offset,
							      GSeekType        type,
							      xcancellable_t    *cancellable,
							      xerror_t         **error);
static xboolean_t g_buffered_output_stream_can_truncate        (xseekable__t       *seekable);
static xboolean_t g_buffered_output_stream_truncate            (xseekable__t       *seekable,
							      xoffset_t          offset,
							      xcancellable_t    *cancellable,
							      xerror_t         **error);

G_DEFINE_TYPE_WITH_CODE (xbuffered_output_stream_t,
			 g_buffered_output_stream,
			 XTYPE_FILTER_OUTPUT_STREAM,
                         G_ADD_PRIVATE (xbuffered_output_stream_t)
			 G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
						g_buffered_output_stream_seekable_iface_init))


static void
g_buffered_output_stream_class_init (GBufferedOutputStreamClass *klass)
{
  xobject_class_t *object_class;
  GOutputStreamClass *ostream_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = g_buffered_output_stream_get_property;
  object_class->set_property = g_buffered_output_stream_set_property;
  object_class->finalize     = g_buffered_output_stream_finalize;

  ostream_class = G_OUTPUT_STREAM_CLASS (klass);
  ostream_class->write_fn = g_buffered_output_stream_write;
  ostream_class->flush = g_buffered_output_stream_flush;
  ostream_class->close_fn = g_buffered_output_stream_close;
  ostream_class->flush_async  = g_buffered_output_stream_flush_async;
  ostream_class->flush_finish = g_buffered_output_stream_flush_finish;
  ostream_class->close_async  = g_buffered_output_stream_close_async;
  ostream_class->close_finish = g_buffered_output_stream_close_finish;

  xobject_class_install_property (object_class,
                                   PROP_BUFSIZE,
                                   g_param_spec_uint ("buffer-size",
                                                      P_("Buffer Size"),
                                                      P_("The size of the backend buffer"),
                                                      1,
                                                      G_MAXUINT,
                                                      DEFAULT_BUFFER_SIZE,
                                                      G_PARAM_READWRITE|G_PARAM_CONSTRUCT|
                                                      G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

  xobject_class_install_property (object_class,
                                   PROP_AUTO_GROW,
                                   g_param_spec_boolean ("auto-grow",
                                                         P_("Auto-grow"),
                                                         P_("Whether the buffer should automatically grow"),
                                                         FALSE,
                                                         G_PARAM_READWRITE|
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

}

/**
 * g_buffered_output_stream_get_buffer_size:
 * @stream: a #xbuffered_output_stream_t.
 *
 * Gets the size of the buffer in the @stream.
 *
 * Returns: the current size of the buffer.
 **/
xsize_t
g_buffered_output_stream_get_buffer_size (xbuffered_output_stream_t *stream)
{
  g_return_val_if_fail (X_IS_BUFFERED_OUTPUT_STREAM (stream), -1);

  return stream->priv->len;
}

/**
 * g_buffered_output_stream_set_buffer_size:
 * @stream: a #xbuffered_output_stream_t.
 * @size: a #xsize_t.
 *
 * Sets the size of the internal buffer to @size.
 **/
void
g_buffered_output_stream_set_buffer_size (xbuffered_output_stream_t *stream,
                                          xsize_t                  size)
{
  GBufferedOutputStreamPrivate *priv;
  xuint8_t *buffer;

  g_return_if_fail (X_IS_BUFFERED_OUTPUT_STREAM (stream));

  priv = stream->priv;

  if (size == priv->len)
    return;

  if (priv->buffer)
    {
      size = (priv->pos > 0) ? MAX (size, (xsize_t) priv->pos) : size;

      buffer = g_malloc (size);
      memcpy (buffer, priv->buffer, priv->pos);
      g_free (priv->buffer);
      priv->buffer = buffer;
      priv->len = size;
      /* Keep old pos */
    }
  else
    {
      priv->buffer = g_malloc (size);
      priv->len = size;
      priv->pos = 0;
    }

  xobject_notify (G_OBJECT (stream), "buffer-size");
}

/**
 * g_buffered_output_stream_get_auto_grow:
 * @stream: a #xbuffered_output_stream_t.
 *
 * Checks if the buffer automatically grows as data is added.
 *
 * Returns: %TRUE if the @stream's buffer automatically grows,
 * %FALSE otherwise.
 **/
xboolean_t
g_buffered_output_stream_get_auto_grow (xbuffered_output_stream_t *stream)
{
  g_return_val_if_fail (X_IS_BUFFERED_OUTPUT_STREAM (stream), FALSE);

  return stream->priv->auto_grow;
}

/**
 * g_buffered_output_stream_set_auto_grow:
 * @stream: a #xbuffered_output_stream_t.
 * @auto_grow: a #xboolean_t.
 *
 * Sets whether or not the @stream's buffer should automatically grow.
 * If @auto_grow is true, then each write will just make the buffer
 * larger, and you must manually flush the buffer to actually write out
 * the data to the underlying stream.
 **/
void
g_buffered_output_stream_set_auto_grow (xbuffered_output_stream_t *stream,
                                        xboolean_t               auto_grow)
{
  GBufferedOutputStreamPrivate *priv;
  g_return_if_fail (X_IS_BUFFERED_OUTPUT_STREAM (stream));
  priv = stream->priv;
  auto_grow = auto_grow != FALSE;
  if (priv->auto_grow != auto_grow)
    {
      priv->auto_grow = auto_grow;
      xobject_notify (G_OBJECT (stream), "auto-grow");
    }
}

static void
g_buffered_output_stream_set_property (xobject_t      *object,
                                       xuint_t         prop_id,
                                       const xvalue_t *value,
                                       xparam_spec_t   *pspec)
{
  xbuffered_output_stream_t *stream;

  stream = G_BUFFERED_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_BUFSIZE:
      g_buffered_output_stream_set_buffer_size (stream, xvalue_get_uint (value));
      break;

    case PROP_AUTO_GROW:
      g_buffered_output_stream_set_auto_grow (stream, xvalue_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_buffered_output_stream_get_property (xobject_t    *object,
                                       xuint_t       prop_id,
                                       xvalue_t     *value,
                                       xparam_spec_t *pspec)
{
  xbuffered_output_stream_t *buffered_stream;
  GBufferedOutputStreamPrivate *priv;

  buffered_stream = G_BUFFERED_OUTPUT_STREAM (object);
  priv = buffered_stream->priv;

  switch (prop_id)
    {
    case PROP_BUFSIZE:
      xvalue_set_uint (value, priv->len);
      break;

    case PROP_AUTO_GROW:
      xvalue_set_boolean (value, priv->auto_grow);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_buffered_output_stream_finalize (xobject_t *object)
{
  xbuffered_output_stream_t *stream;
  GBufferedOutputStreamPrivate *priv;

  stream = G_BUFFERED_OUTPUT_STREAM (object);
  priv = stream->priv;

  g_free (priv->buffer);

  G_OBJECT_CLASS (g_buffered_output_stream_parent_class)->finalize (object);
}

static void
g_buffered_output_stream_init (xbuffered_output_stream_t *stream)
{
  stream->priv = g_buffered_output_stream_get_instance_private (stream);
}

static void
g_buffered_output_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell         = g_buffered_output_stream_tell;
  iface->can_seek     = g_buffered_output_stream_can_seek;
  iface->seek         = g_buffered_output_stream_seek;
  iface->can_truncate = g_buffered_output_stream_can_truncate;
  iface->truncate_fn  = g_buffered_output_stream_truncate;
}

/**
 * g_buffered_output_stream_new:
 * @base_stream: a #xoutput_stream_t.
 *
 * Creates a new buffered output stream for a base stream.
 *
 * Returns: a #xoutput_stream_t for the given @base_stream.
 **/
xoutput_stream_t *
g_buffered_output_stream_new (xoutput_stream_t *base_stream)
{
  xoutput_stream_t *stream;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_BUFFERED_OUTPUT_STREAM,
                         "base-stream", base_stream,
                         NULL);

  return stream;
}

/**
 * g_buffered_output_stream_new_sized:
 * @base_stream: a #xoutput_stream_t.
 * @size: a #xsize_t.
 *
 * Creates a new buffered output stream with a given buffer size.
 *
 * Returns: a #xoutput_stream_t with an internal buffer set to @size.
 **/
xoutput_stream_t *
g_buffered_output_stream_new_sized (xoutput_stream_t *base_stream,
                                    xsize_t          size)
{
  xoutput_stream_t *stream;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_BUFFERED_OUTPUT_STREAM,
                         "base-stream", base_stream,
                         "buffer-size", size,
                         NULL);

  return stream;
}

static xboolean_t
flush_buffer (xbuffered_output_stream_t  *stream,
              xcancellable_t           *cancellable,
              xerror_t                 **error)
{
  GBufferedOutputStreamPrivate *priv;
  xoutput_stream_t                *base_stream;
  xboolean_t                      res;
  xsize_t                         bytes_written;
  xsize_t                         count;

  priv = stream->priv;
  bytes_written = 0;
  base_stream = G_FILTER_OUTPUT_STREAM (stream)->base_stream;

  g_return_val_if_fail (X_IS_OUTPUT_STREAM (base_stream), FALSE);

  res = xoutput_stream_write_all (base_stream,
                                   priv->buffer,
                                   priv->pos,
                                   &bytes_written,
                                   cancellable,
                                   error);

  count = priv->pos - bytes_written;

  if (count > 0)
    memmove (priv->buffer, priv->buffer + bytes_written, count);

  priv->pos -= bytes_written;

  return res;
}

static xssize_t
g_buffered_output_stream_write  (xoutput_stream_t *stream,
                                 const void    *buffer,
                                 xsize_t          count,
                                 xcancellable_t  *cancellable,
                                 xerror_t       **error)
{
  xbuffered_output_stream_t        *bstream;
  GBufferedOutputStreamPrivate *priv;
  xboolean_t res;
  xsize_t    n;
  xsize_t new_size;

  bstream = G_BUFFERED_OUTPUT_STREAM (stream);
  priv = bstream->priv;

  n = priv->len - priv->pos;

  if (priv->auto_grow && n < count)
    {
      new_size = MAX (priv->len * 2, priv->len + count);
      g_buffered_output_stream_set_buffer_size (bstream, new_size);
    }
  else if (n == 0)
    {
      res = flush_buffer (bstream, cancellable, error);

      if (res == FALSE)
	return -1;
    }

  n = priv->len - priv->pos;

  count = MIN (count, n);
  memcpy (priv->buffer + priv->pos, buffer, count);
  priv->pos += count;

  return count;
}

static xboolean_t
g_buffered_output_stream_flush (xoutput_stream_t  *stream,
                                xcancellable_t   *cancellable,
                                xerror_t        **error)
{
  xbuffered_output_stream_t *bstream;
  xoutput_stream_t                *base_stream;
  xboolean_t res;

  bstream = G_BUFFERED_OUTPUT_STREAM (stream);
  base_stream = G_FILTER_OUTPUT_STREAM (stream)->base_stream;

  res = flush_buffer (bstream, cancellable, error);

  if (res == FALSE)
    return FALSE;

  res = xoutput_stream_flush (base_stream, cancellable, error);

  return res;
}

static xboolean_t
g_buffered_output_stream_close (xoutput_stream_t  *stream,
                                xcancellable_t   *cancellable,
                                xerror_t        **error)
{
  xbuffered_output_stream_t        *bstream;
  xoutput_stream_t                *base_stream;
  xboolean_t                      res;

  bstream = G_BUFFERED_OUTPUT_STREAM (stream);
  base_stream = G_FILTER_OUTPUT_STREAM (bstream)->base_stream;
  res = flush_buffer (bstream, cancellable, error);

  if (g_filter_output_stream_get_close_base_stream (G_FILTER_OUTPUT_STREAM (stream)))
    {
      /* report the first error but still close the stream */
      if (res)
        res = xoutput_stream_close (base_stream, cancellable, error);
      else
        xoutput_stream_close (base_stream, cancellable, NULL);
    }

  return res;
}

static xoffset_t
g_buffered_output_stream_tell (xseekable__t *seekable)
{
  xbuffered_output_stream_t        *bstream;
  GBufferedOutputStreamPrivate *priv;
  xoutput_stream_t *base_stream;
  xseekable__t    *base_stream_seekable;
  xoffset_t base_offset;

  bstream = G_BUFFERED_OUTPUT_STREAM (seekable);
  priv = bstream->priv;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    return 0;

  base_stream_seekable = G_SEEKABLE (base_stream);

  base_offset = xseekable_tell (base_stream_seekable);
  return base_offset + priv->pos;
}

static xboolean_t
g_buffered_output_stream_can_seek (xseekable__t *seekable)
{
  xoutput_stream_t *base_stream;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  return X_IS_SEEKABLE (base_stream) && xseekable_can_seek (G_SEEKABLE (base_stream));
}

static xboolean_t
g_buffered_output_stream_seek (xseekable__t     *seekable,
			       xoffset_t        offset,
			       GSeekType      type,
			       xcancellable_t  *cancellable,
			       xerror_t       **error)
{
  xbuffered_output_stream_t *bstream;
  xoutput_stream_t *base_stream;
  xseekable__t *base_stream_seekable;
  xboolean_t flushed;

  bstream = G_BUFFERED_OUTPUT_STREAM (seekable);

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Seek not supported on base stream"));
      return FALSE;
    }

  base_stream_seekable = G_SEEKABLE (base_stream);
  flushed = flush_buffer (bstream, cancellable, error);
  if (!flushed)
    return FALSE;

  return xseekable_seek (base_stream_seekable, offset, type, cancellable, error);
}

static xboolean_t
g_buffered_output_stream_can_truncate (xseekable__t *seekable)
{
  xoutput_stream_t *base_stream;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  return X_IS_SEEKABLE (base_stream) && xseekable_can_truncate (G_SEEKABLE (base_stream));
}

static xboolean_t
g_buffered_output_stream_truncate (xseekable__t     *seekable,
				   xoffset_t        offset,
				   xcancellable_t  *cancellable,
				   xerror_t       **error)
{
  xbuffered_output_stream_t        *bstream;
  xoutput_stream_t *base_stream;
  xseekable__t *base_stream_seekable;
  xboolean_t flushed;

  bstream = G_BUFFERED_OUTPUT_STREAM (seekable);
  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Truncate not supported on base stream"));
      return FALSE;
    }

  base_stream_seekable = G_SEEKABLE (base_stream);

  flushed = flush_buffer (bstream, cancellable, error);
  if (!flushed)
    return FALSE;
  return xseekable_truncate (base_stream_seekable, offset, cancellable, error);
}

/* ************************** */
/* Async stuff implementation */
/* ************************** */

/* TODO: This should be using the base class async ops, not threads */

typedef struct {

  xuint_t flush_stream : 1;
  xuint_t close_stream : 1;

} FlushData;

static void
free_flush_data (xpointer_t data)
{
  g_slice_free (FlushData, data);
}

/* This function is used by all three (i.e.
 * _write, _flush, _close) functions since
 * all of them will need to flush the buffer
 * and so closing and writing is just a special
 * case of flushing + some addition stuff */
static void
flush_buffer_thread (xtask_t        *task,
                     xpointer_t      object,
                     xpointer_t      task_data,
                     xcancellable_t *cancellable)
{
  xbuffered_output_stream_t *stream;
  xoutput_stream_t *base_stream;
  FlushData     *fdata;
  xboolean_t       res;
  xerror_t        *error = NULL;

  stream = G_BUFFERED_OUTPUT_STREAM (object);
  fdata = task_data;
  base_stream = G_FILTER_OUTPUT_STREAM (stream)->base_stream;

  res = flush_buffer (stream, cancellable, &error);

  /* if flushing the buffer didn't work don't even bother
   * to flush the stream but just report that error */
  if (res && fdata->flush_stream)
    res = xoutput_stream_flush (base_stream, cancellable, &error);

  if (fdata->close_stream)
    {

      /* if flushing the buffer or the stream returned
       * an error report that first error but still try
       * close the stream */
      if (g_filter_output_stream_get_close_base_stream (G_FILTER_OUTPUT_STREAM (stream)))
        {
          if (res == FALSE)
            xoutput_stream_close (base_stream, cancellable, NULL);
          else
            res = xoutput_stream_close (base_stream, cancellable, &error);
        }
    }

  if (res == FALSE)
    xtask_return_error (task, error);
  else
    xtask_return_boolean (task, TRUE);
}

static void
g_buffered_output_stream_flush_async (xoutput_stream_t        *stream,
                                      int                   io_priority,
                                      xcancellable_t         *cancellable,
                                      xasync_ready_callback_t   callback,
                                      xpointer_t              data)
{
  xtask_t *task;
  FlushData *fdata;

  fdata = g_slice_new0 (FlushData);
  fdata->flush_stream = TRUE;
  fdata->close_stream = FALSE;

  task = xtask_new (stream, cancellable, callback, data);
  xtask_set_source_tag (task, g_buffered_output_stream_flush_async);
  xtask_set_task_data (task, fdata, free_flush_data);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, flush_buffer_thread);
  xobject_unref (task);
}

static xboolean_t
g_buffered_output_stream_flush_finish (xoutput_stream_t        *stream,
                                       xasync_result_t         *result,
                                       xerror_t              **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static void
g_buffered_output_stream_close_async (xoutput_stream_t        *stream,
                                      int                   io_priority,
                                      xcancellable_t         *cancellable,
                                      xasync_ready_callback_t   callback,
                                      xpointer_t              data)
{
  xtask_t *task;
  FlushData *fdata;

  fdata = g_slice_new0 (FlushData);
  fdata->close_stream = TRUE;

  task = xtask_new (stream, cancellable, callback, data);
  xtask_set_source_tag (task, g_buffered_output_stream_close_async);
  xtask_set_task_data (task, fdata, free_flush_data);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, flush_buffer_thread);
  xobject_unref (task);
}

static xboolean_t
g_buffered_output_stream_close_finish (xoutput_stream_t        *stream,
                                       xasync_result_t         *result,
                                       xerror_t              **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}
