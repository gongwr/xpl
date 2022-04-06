/* GIO - GLib Input, IO and Streaming Library
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
#include <gfileiostream.h>
#include <gseekable.h>
#include "gasyncresult.h"
#include "gtask.h"
#include "gcancellable.h"
#include "gioerror.h"
#include "gfileoutputstream.h"
#include "glibintl.h"


/**
 * SECTION:gfileiostream
 * @short_description:  File read and write streaming operations
 * @include: gio/gio.h
 * @see_also: #xio_stream_t, #xfile_input_stream_t, #xfile_output_stream_t, #xseekable__t
 *
 * xfile_io_stream_t provides io streams that both read and write to the same
 * file handle.
 *
 * xfile_io_stream_t implements #xseekable__t, which allows the io
 * stream to jump to arbitrary positions in the file and to truncate
 * the file, provided the filesystem of the file supports these
 * operations.
 *
 * To find the position of a file io stream, use
 * xseekable_tell().
 *
 * To find out if a file io stream supports seeking, use xseekable_can_seek().
 * To position a file io stream, use xseekable_seek().
 * To find out if a file io stream supports truncating, use
 * xseekable_can_truncate(). To truncate a file io
 * stream, use xseekable_truncate().
 *
 * The default implementation of all the #xfile_io_stream_t operations
 * and the implementation of #xseekable__t just call into the same operations
 * on the output stream.
 * Since: 2.22
 **/

static void       xfile_io_stream_seekable_iface_init    (xseekable_iface_t       *iface);
static xoffset_t    xfile_io_stream_seekable_tell          (xseekable__t            *seekable);
static xboolean_t   xfile_io_stream_seekable_can_seek      (xseekable__t            *seekable);
static xboolean_t   xfile_io_stream_seekable_seek          (xseekable__t            *seekable,
							   xoffset_t               offset,
							   xseek_type_t             type,
							   xcancellable_t         *cancellable,
							   xerror_t              **error);
static xboolean_t   xfile_io_stream_seekable_can_truncate  (xseekable__t            *seekable);
static xboolean_t   xfile_io_stream_seekable_truncate      (xseekable__t            *seekable,
							   xoffset_t               offset,
							   xcancellable_t         *cancellable,
							   xerror_t              **error);
static void       xfile_io_stream_real_query_info_async  (xfile_io_stream_t    *stream,
							   const char           *attributes,
							   int                   io_priority,
							   xcancellable_t         *cancellable,
							   xasync_ready_callback_t   callback,
							   xpointer_t              user_data);
static xfile_info_t *xfile_io_stream_real_query_info_finish (xfile_io_stream_t    *stream,
							   xasync_result_t         *result,
							   xerror_t              **error);

struct _GFileIOStreamPrivate {
  xasync_ready_callback_t outstanding_callback;
};

G_DEFINE_TYPE_WITH_CODE (xfile_io_stream, xfile_io_stream, XTYPE_IO_STREAM,
                         G_ADD_PRIVATE (xfile_io_stream_t)
			 G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
						xfile_io_stream_seekable_iface_init))

static void
xfile_io_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell = xfile_io_stream_seekable_tell;
  iface->can_seek = xfile_io_stream_seekable_can_seek;
  iface->seek = xfile_io_stream_seekable_seek;
  iface->can_truncate = xfile_io_stream_seekable_can_truncate;
  iface->truncate_fn = xfile_io_stream_seekable_truncate;
}

static void
xfile_io_stream_init (xfile_io_stream_t *stream)
{
  stream->priv = xfile_io_stream_get_instance_private (stream);
}

/**
 * xfile_io_stream_query_info:
 * @stream: a #xfile_io_stream_t.
 * @attributes: a file attribute query string.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Queries a file io stream for the given @attributes.
 * This function blocks while querying the stream. For the asynchronous
 * version of this function, see xfile_io_stream_query_info_async().
 * While the stream is blocked, the stream will set the pending flag
 * internally, and any other operations on the stream will fail with
 * %G_IO_ERROR_PENDING.
 *
 * Can fail if the stream was already closed (with @error being set to
 * %G_IO_ERROR_CLOSED), the stream has pending operations (with @error being
 * set to %G_IO_ERROR_PENDING), or if querying info is not supported for
 * the stream's interface (with @error being set to %G_IO_ERROR_NOT_SUPPORTED). I
 * all cases of failure, %NULL will be returned.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be set, and %NULL will
 * be returned.
 *
 * Returns: (transfer full): a #xfile_info_t for the @stream, or %NULL on error.
 *
 * Since: 2.22
 **/
xfile_info_t *
xfile_io_stream_query_info (xfile_io_stream_t      *stream,
			     const char             *attributes,
			     xcancellable_t           *cancellable,
			     xerror_t                **error)
{
  GFileIOStreamClass *class;
  xio_stream_t *io_stream;
  xfile_info_t *info;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), NULL);

  io_stream = XIO_STREAM (stream);

  if (!g_io_stream_set_pending (io_stream, error))
    return NULL;

  info = NULL;

  if (cancellable)
    xcancellable_push_current (cancellable);

  class = XFILE_IO_STREAM_GET_CLASS (stream);
  if (class->query_info)
    info = class->query_info (stream, attributes, cancellable, error);
  else
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         _("Stream doesn’t support query_info"));

  if (cancellable)
    xcancellable_pop_current (cancellable);

  g_io_stream_clear_pending (io_stream);

  return info;
}

static void
async_ready_callback_wrapper (xobject_t *source_object,
			      xasync_result_t *res,
			      xpointer_t      user_data)
{
  xfile_io_stream_t *stream = XFILE_IO_STREAM (source_object);

  g_io_stream_clear_pending (XIO_STREAM (stream));
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (stream);
}

/**
 * xfile_io_stream_query_info_async:
 * @stream: a #xfile_io_stream_t.
 * @attributes: a file attribute query string.
 * @io_priority: the [I/O priority][gio-GIOScheduler] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously queries the @stream for a #xfile_info_t. When completed,
 * @callback will be called with a #xasync_result_t which can be used to
 * finish the operation with xfile_io_stream_query_info_finish().
 *
 * For the synchronous version of this function, see
 * xfile_io_stream_query_info().
 *
 * Since: 2.22
 **/
void
xfile_io_stream_query_info_async (xfile_io_stream_t     *stream,
					  const char           *attributes,
					  int                   io_priority,
					  xcancellable_t         *cancellable,
					  xasync_ready_callback_t   callback,
					  xpointer_t              user_data)
{
  GFileIOStreamClass *klass;
  xio_stream_t *io_stream;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_FILE_IO_STREAM (stream));

  io_stream = XIO_STREAM (stream);

  if (!g_io_stream_set_pending (io_stream, &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xfile_io_stream_query_info_async,
                           error);
      return;
    }

  klass = XFILE_IO_STREAM_GET_CLASS (stream);

  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  klass->query_info_async (stream, attributes, io_priority, cancellable,
                           async_ready_callback_wrapper, user_data);
}

/**
 * xfile_io_stream_query_info_finish:
 * @stream: a #xfile_io_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Finalizes the asynchronous query started
 * by xfile_io_stream_query_info_async().
 *
 * Returns: (transfer full): A #xfile_info_t for the finished query.
 *
 * Since: 2.22
 **/
xfile_info_t *
xfile_io_stream_query_info_finish (xfile_io_stream_t     *stream,
				    xasync_result_t         *result,
				    xerror_t              **error)
{
  GFileIOStreamClass *class;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, xfile_io_stream_query_info_async))
    return xtask_propagate_pointer (XTASK (result), error);

  class = XFILE_IO_STREAM_GET_CLASS (stream);
  return class->query_info_finish (stream, result, error);
}

/**
 * xfile_io_stream_get_etag:
 * @stream: a #xfile_io_stream_t.
 *
 * Gets the entity tag for the file when it has been written.
 * This must be called after the stream has been written
 * and closed, as the etag can change while writing.
 *
 * Returns: (nullable) (transfer full): the entity tag for the stream.
 *
 * Since: 2.22
 **/
char *
xfile_io_stream_get_etag (xfile_io_stream_t  *stream)
{
  GFileIOStreamClass *class;
  xio_stream_t *io_stream;
  char *etag;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), NULL);

  io_stream = XIO_STREAM (stream);

  if (!g_io_stream_is_closed (io_stream))
    {
      g_warning ("stream is not closed yet, can't get etag");
      return NULL;
    }

  etag = NULL;

  class = XFILE_IO_STREAM_GET_CLASS (stream);
  if (class->get_etag)
    etag = class->get_etag (stream);

  return etag;
}

static xoffset_t
xfile_io_stream_tell (xfile_io_stream_t  *stream)
{
  GFileIOStreamClass *class;
  xoffset_t offset;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), 0);

  class = XFILE_IO_STREAM_GET_CLASS (stream);

  offset = 0;
  if (class->tell)
    offset = class->tell (stream);

  return offset;
}

static xoffset_t
xfile_io_stream_seekable_tell (xseekable__t *seekable)
{
  return xfile_io_stream_tell (XFILE_IO_STREAM (seekable));
}

static xboolean_t
xfile_io_stream_can_seek (xfile_io_stream_t  *stream)
{
  GFileIOStreamClass *class;
  xboolean_t can_seek;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), FALSE);

  class = XFILE_IO_STREAM_GET_CLASS (stream);

  can_seek = FALSE;
  if (class->seek)
    {
      can_seek = TRUE;
      if (class->can_seek)
	can_seek = class->can_seek (stream);
    }

  return can_seek;
}

static xboolean_t
xfile_io_stream_seekable_can_seek (xseekable__t *seekable)
{
  return xfile_io_stream_can_seek (XFILE_IO_STREAM (seekable));
}

static xboolean_t
xfile_io_stream_seek (xfile_io_stream_t  *stream,
		       xoffset_t             offset,
		       xseek_type_t           type,
		       xcancellable_t       *cancellable,
		       xerror_t            **error)
{
  GFileIOStreamClass *class;
  xio_stream_t *io_stream;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), FALSE);

  io_stream = XIO_STREAM (stream);
  class = XFILE_IO_STREAM_GET_CLASS (stream);

  if (!class->seek)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Seek not supported on stream"));
      return FALSE;
    }

  if (!g_io_stream_set_pending (io_stream, error))
    return FALSE;

  if (cancellable)
    xcancellable_push_current (cancellable);

  res = class->seek (stream, offset, type, cancellable, error);

  if (cancellable)
    xcancellable_pop_current (cancellable);

  g_io_stream_clear_pending (io_stream);

  return res;
}

static xboolean_t
xfile_io_stream_seekable_seek (xseekable__t  *seekable,
				    xoffset_t     offset,
				    xseek_type_t   type,
				    xcancellable_t  *cancellable,
				    xerror_t    **error)
{
  return xfile_io_stream_seek (XFILE_IO_STREAM (seekable),
				offset, type, cancellable, error);
}

static xboolean_t
xfile_io_stream_can_truncate (xfile_io_stream_t  *stream)
{
  GFileIOStreamClass *class;
  xboolean_t can_truncate;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), FALSE);

  class = XFILE_IO_STREAM_GET_CLASS (stream);

  can_truncate = FALSE;
  if (class->truncate_fn)
    {
      can_truncate = TRUE;
      if (class->can_truncate)
	can_truncate = class->can_truncate (stream);
    }

  return can_truncate;
}

static xboolean_t
xfile_io_stream_seekable_can_truncate (xseekable__t  *seekable)
{
  return xfile_io_stream_can_truncate (XFILE_IO_STREAM (seekable));
}

static xboolean_t
xfile_io_stream_truncate (xfile_io_stream_t  *stream,
			   xoffset_t             size,
			   xcancellable_t       *cancellable,
			   xerror_t            **error)
{
  GFileIOStreamClass *class;
  xio_stream_t *io_stream;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE_IO_STREAM (stream), FALSE);

  io_stream = XIO_STREAM (stream);
  class = XFILE_IO_STREAM_GET_CLASS (stream);

  if (!class->truncate_fn)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Truncate not supported on stream"));
      return FALSE;
    }

  if (!g_io_stream_set_pending (io_stream, error))
    return FALSE;

  if (cancellable)
    xcancellable_push_current (cancellable);

  res = class->truncate_fn (stream, size, cancellable, error);

  if (cancellable)
    xcancellable_pop_current (cancellable);

  g_io_stream_clear_pending (io_stream);

  return res;
}

static xboolean_t
xfile_io_stream_seekable_truncate (xseekable__t     *seekable,
				    xoffset_t        size,
				    xcancellable_t  *cancellable,
				    xerror_t       **error)
{
  return xfile_io_stream_truncate (XFILE_IO_STREAM (seekable),
					size, cancellable, error);
}
/*****************************************************
 *   Default implementations based on output stream  *
 *****************************************************/

static xoffset_t
xfile_io_stream_real_tell (xfile_io_stream_t    *stream)
{
  xoutput_stream_t *out;
  xseekable__t *seekable;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  seekable = G_SEEKABLE (out);

  return xseekable_tell (seekable);
}

static xboolean_t
xfile_io_stream_real_can_seek (xfile_io_stream_t    *stream)
{
  xoutput_stream_t *out;
  xseekable__t *seekable;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  seekable = G_SEEKABLE (out);

  return xseekable_can_seek (seekable);
}

static xboolean_t
xfile_io_stream_real_seek (xfile_io_stream_t    *stream,
			    xoffset_t           offset,
			    xseek_type_t         type,
			    xcancellable_t     *cancellable,
			    xerror_t          **error)
{
  xoutput_stream_t *out;
  xseekable__t *seekable;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  seekable = G_SEEKABLE (out);

  return xseekable_seek (seekable, offset, type, cancellable, error);
}

static  xboolean_t
xfile_io_stream_real_can_truncate (xfile_io_stream_t    *stream)
{
  xoutput_stream_t *out;
  xseekable__t *seekable;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  seekable = G_SEEKABLE (out);

  return xseekable_can_truncate (seekable);
}

static xboolean_t
xfile_io_stream_real_truncate_fn (xfile_io_stream_t    *stream,
				   xoffset_t               size,
				   xcancellable_t         *cancellable,
				   xerror_t              **error)
{
  xoutput_stream_t *out;
  xseekable__t *seekable;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  seekable = G_SEEKABLE (out);

  return xseekable_truncate (seekable, size, cancellable, error);
}

static char *
xfile_io_stream_real_get_etag (xfile_io_stream_t    *stream)
{
  xoutput_stream_t *out;
  xfile_output_stream_t *file_out;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  file_out = XFILE_OUTPUT_STREAM (out);

  return xfile_output_stream_get_etag (file_out);
}

static xfile_info_t *
xfile_io_stream_real_query_info (xfile_io_stream_t    *stream,
				  const char           *attributes,
				  xcancellable_t         *cancellable,
				  xerror_t              **error)
{
  xoutput_stream_t *out;
  xfile_output_stream_t *file_out;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  file_out = XFILE_OUTPUT_STREAM (out);

  return xfile_output_stream_query_info (file_out,
					  attributes, cancellable, error);
}

typedef struct {
  xobject_t *object;
  xasync_ready_callback_t callback;
  xpointer_t user_data;
} AsyncOpWrapper;

static AsyncOpWrapper *
async_op_wrapper_new (xpointer_t object,
		      xasync_ready_callback_t callback,
		      xpointer_t user_data)
{
  AsyncOpWrapper *data;

  data = g_new0 (AsyncOpWrapper, 1);
  data->object = xobject_ref (object);
  data->callback = callback;
  data->user_data = user_data;

  return data;
}

static void
async_op_wrapper_callback (xobject_t *source_object,
			   xasync_result_t *res,
			   xpointer_t user_data)
{
  AsyncOpWrapper *data  = user_data;
  data->callback (data->object, res, data->user_data);
  xobject_unref (data->object);
  g_free (data);
}

static void
xfile_io_stream_real_query_info_async (xfile_io_stream_t     *stream,
					const char           *attributes,
					int                   io_priority,
					xcancellable_t         *cancellable,
					xasync_ready_callback_t   callback,
					xpointer_t              user_data)
{
  xoutput_stream_t *out;
  xfile_output_stream_t *file_out;
  AsyncOpWrapper *data;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  file_out = XFILE_OUTPUT_STREAM (out);

  data = async_op_wrapper_new (stream, callback, user_data);
  xfile_output_stream_query_info_async (file_out,
					 attributes, io_priority,
					 cancellable, async_op_wrapper_callback, data);
}

static xfile_info_t *
xfile_io_stream_real_query_info_finish (xfile_io_stream_t     *stream,
					 xasync_result_t      *res,
					 xerror_t           **error)
{
  xoutput_stream_t *out;
  xfile_output_stream_t *file_out;

  out = g_io_stream_get_output_stream (XIO_STREAM (stream));
  file_out = XFILE_OUTPUT_STREAM (out);

  return xfile_output_stream_query_info_finish (file_out, res, error);
}

static void
xfile_io_stream_class_init (GFileIOStreamClass *klass)
{
  klass->tell = xfile_io_stream_real_tell;
  klass->can_seek = xfile_io_stream_real_can_seek;
  klass->seek = xfile_io_stream_real_seek;
  klass->can_truncate = xfile_io_stream_real_can_truncate;
  klass->truncate_fn = xfile_io_stream_real_truncate_fn;
  klass->query_info = xfile_io_stream_real_query_info;
  klass->query_info_async = xfile_io_stream_real_query_info_async;
  klass->query_info_finish = xfile_io_stream_real_query_info_finish;
  klass->get_etag = xfile_io_stream_real_get_etag;
}
