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
#include <gfileoutputstream.h>
#include <gseekable.h>
#include "gasyncresult.h"
#include "gtask.h"
#include "gcancellable.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gfileoutputstream
 * @short_description: File output streaming operations
 * @include: gio/gio.h
 * @see_also: #xoutput_stream_t, #xdata_output_stream_t, #xseekable__t
 *
 * xfile_output_stream_t provides output streams that write their
 * content to a file.
 *
 * xfile_output_stream_t implements #xseekable__t, which allows the output
 * stream to jump to arbitrary positions in the file and to truncate
 * the file, provided the filesystem of the file supports these
 * operations.
 *
 * To find the position of a file output stream, use xseekable_tell().
 * To find out if a file output stream supports seeking, use
 * xseekable_can_seek().To position a file output stream, use
 * xseekable_seek(). To find out if a file output stream supports
 * truncating, use xseekable_can_truncate(). To truncate a file output
 * stream, use xseekable_truncate().
 **/

static void       xfile_output_stream_seekable_iface_init    (xseekable_iface_t       *iface);
static xoffset_t    xfile_output_stream_seekable_tell          (xseekable__t            *seekable);
static xboolean_t   xfile_output_stream_seekable_can_seek      (xseekable__t            *seekable);
static xboolean_t   xfile_output_stream_seekable_seek          (xseekable__t            *seekable,
							       xoffset_t               offset,
							       xseek_type_t             type,
							       xcancellable_t         *cancellable,
							       xerror_t              **error);
static xboolean_t   xfile_output_stream_seekable_can_truncate  (xseekable__t            *seekable);
static xboolean_t   xfile_output_stream_seekable_truncate      (xseekable__t            *seekable,
							       xoffset_t               offset,
							       xcancellable_t         *cancellable,
							       xerror_t              **error);
static void       xfile_output_stream_real_query_info_async  (xfile_output_stream_t    *stream,
							       const char           *attributes,
							       int                   io_priority,
							       xcancellable_t         *cancellable,
							       xasync_ready_callback_t   callback,
							       xpointer_t              user_data);
static xfile_info_t *xfile_output_stream_real_query_info_finish (xfile_output_stream_t    *stream,
							       xasync_result_t         *result,
							       xerror_t              **error);

struct _GFileOutputStreamPrivate {
  xasync_ready_callback_t outstanding_callback;
};

G_DEFINE_TYPE_WITH_CODE (xfile_output_stream, xfile_output_stream, XTYPE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (xfile_output_stream_t)
			 G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
						xfile_output_stream_seekable_iface_init));

static void
xfile_output_stream_class_init (GFileOutputStreamClass *klass)
{
  klass->query_info_async = xfile_output_stream_real_query_info_async;
  klass->query_info_finish = xfile_output_stream_real_query_info_finish;
}

static void
xfile_output_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell = xfile_output_stream_seekable_tell;
  iface->can_seek = xfile_output_stream_seekable_can_seek;
  iface->seek = xfile_output_stream_seekable_seek;
  iface->can_truncate = xfile_output_stream_seekable_can_truncate;
  iface->truncate_fn = xfile_output_stream_seekable_truncate;
}

static void
xfile_output_stream_init (xfile_output_stream_t *stream)
{
  stream->priv = xfile_output_stream_get_instance_private (stream);
}

/**
 * xfile_output_stream_query_info:
 * @stream: a #xfile_output_stream_t.
 * @attributes: a file attribute query string.
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Queries a file output stream for the given @attributes.
 * This function blocks while querying the stream. For the asynchronous
 * version of this function, see xfile_output_stream_query_info_async().
 * While the stream is blocked, the stream will set the pending flag
 * internally, and any other operations on the stream will fail with
 * %G_IO_ERROR_PENDING.
 *
 * Can fail if the stream was already closed (with @error being set to
 * %G_IO_ERROR_CLOSED), the stream has pending operations (with @error being
 * set to %G_IO_ERROR_PENDING), or if querying info is not supported for
 * the stream's interface (with @error being set to %G_IO_ERROR_NOT_SUPPORTED). In
 * all cases of failure, %NULL will be returned.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be set, and %NULL will
 * be returned.
 *
 * Returns: (transfer full): a #xfile_info_t for the @stream, or %NULL on error.
 **/
xfile_info_t *
xfile_output_stream_query_info (xfile_output_stream_t      *stream,
				    const char             *attributes,
				    xcancellable_t           *cancellable,
				    xerror_t                **error)
{
  GFileOutputStreamClass *class;
  xoutput_stream_t *output_stream;
  xfile_info_t *info;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), NULL);

  output_stream = G_OUTPUT_STREAM (stream);

  if (!xoutput_stream_set_pending (output_stream, error))
    return NULL;

  info = NULL;

  if (cancellable)
    xcancellable_push_current (cancellable);

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);
  if (class->query_info)
    info = class->query_info (stream, attributes, cancellable, error);
  else
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         _("Stream doesn’t support query_info"));

  if (cancellable)
    xcancellable_pop_current (cancellable);

  xoutput_stream_clear_pending (output_stream);

  return info;
}

static void
async_ready_callback_wrapper (xobject_t *source_object,
			      xasync_result_t *res,
			      xpointer_t      user_data)
{
  xfile_output_stream_t *stream = XFILE_OUTPUT_STREAM (source_object);

  xoutput_stream_clear_pending (G_OUTPUT_STREAM (stream));
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (stream);
}

/**
 * xfile_output_stream_query_info_async:
 * @stream: a #xfile_output_stream_t.
 * @attributes: a file attribute query string.
 * @io_priority: the [I/O priority][gio-GIOScheduler] of the request
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @callback: callback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously queries the @stream for a #xfile_info_t. When completed,
 * @callback will be called with a #xasync_result_t which can be used to
 * finish the operation with xfile_output_stream_query_info_finish().
 *
 * For the synchronous version of this function, see
 * xfile_output_stream_query_info().
 *
 **/
void
xfile_output_stream_query_info_async (xfile_output_stream_t     *stream,
					  const char           *attributes,
					  int                   io_priority,
					  xcancellable_t         *cancellable,
					  xasync_ready_callback_t   callback,
					  xpointer_t              user_data)
{
  GFileOutputStreamClass *klass;
  xoutput_stream_t *output_stream;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_FILE_OUTPUT_STREAM (stream));

  output_stream = G_OUTPUT_STREAM (stream);

  if (!xoutput_stream_set_pending (output_stream, &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xfile_output_stream_query_info_async,
                           error);
      return;
    }

  klass = XFILE_OUTPUT_STREAM_GET_CLASS (stream);

  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  klass->query_info_async (stream, attributes, io_priority, cancellable,
                           async_ready_callback_wrapper, user_data);
}

/**
 * xfile_output_stream_query_info_finish:
 * @stream: a #xfile_output_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Finalizes the asynchronous query started
 * by xfile_output_stream_query_info_async().
 *
 * Returns: (transfer full): A #xfile_info_t for the finished query.
 **/
xfile_info_t *
xfile_output_stream_query_info_finish (xfile_output_stream_t     *stream,
					   xasync_result_t         *result,
					   xerror_t              **error)
{
  GFileOutputStreamClass *class;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, xfile_output_stream_query_info_async))
    return xtask_propagate_pointer (XTASK (result), error);

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);
  return class->query_info_finish (stream, result, error);
}

/**
 * xfile_output_stream_get_etag:
 * @stream: a #xfile_output_stream_t.
 *
 * Gets the entity tag for the file when it has been written.
 * This must be called after the stream has been written
 * and closed, as the etag can change while writing.
 *
 * Returns: (nullable) (transfer full): the entity tag for the stream.
 **/
char *
xfile_output_stream_get_etag (xfile_output_stream_t  *stream)
{
  GFileOutputStreamClass *class;
  xoutput_stream_t *output_stream;
  char *etag;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), NULL);

  output_stream = G_OUTPUT_STREAM (stream);

  if (!xoutput_stream_is_closed (output_stream))
    {
      g_warning ("stream is not closed yet, can't get etag");
      return NULL;
    }

  etag = NULL;

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);
  if (class->get_etag)
    etag = class->get_etag (stream);

  return etag;
}

static xoffset_t
xfile_output_stream_tell (xfile_output_stream_t  *stream)
{
  GFileOutputStreamClass *class;
  xoffset_t offset;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), 0);

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);

  offset = 0;
  if (class->tell)
    offset = class->tell (stream);

  return offset;
}

static xoffset_t
xfile_output_stream_seekable_tell (xseekable__t *seekable)
{
  return xfile_output_stream_tell (XFILE_OUTPUT_STREAM (seekable));
}

static xboolean_t
xfile_output_stream_can_seek (xfile_output_stream_t  *stream)
{
  GFileOutputStreamClass *class;
  xboolean_t can_seek;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), FALSE);

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);

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
xfile_output_stream_seekable_can_seek (xseekable__t *seekable)
{
  return xfile_output_stream_can_seek (XFILE_OUTPUT_STREAM (seekable));
}

static xboolean_t
xfile_output_stream_seek (xfile_output_stream_t  *stream,
			   xoffset_t             offset,
			   xseek_type_t           type,
			   xcancellable_t       *cancellable,
			   xerror_t            **error)
{
  GFileOutputStreamClass *class;
  xoutput_stream_t *output_stream;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), FALSE);

  output_stream = G_OUTPUT_STREAM (stream);
  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);

  if (!class->seek)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Seek not supported on stream"));
      return FALSE;
    }

  if (!xoutput_stream_set_pending (output_stream, error))
    return FALSE;

  if (cancellable)
    xcancellable_push_current (cancellable);

  res = class->seek (stream, offset, type, cancellable, error);

  if (cancellable)
    xcancellable_pop_current (cancellable);

  xoutput_stream_clear_pending (output_stream);

  return res;
}

static xboolean_t
xfile_output_stream_seekable_seek (xseekable__t  *seekable,
				    xoffset_t     offset,
				    xseek_type_t   type,
				    xcancellable_t  *cancellable,
				    xerror_t    **error)
{
  return xfile_output_stream_seek (XFILE_OUTPUT_STREAM (seekable),
				    offset, type, cancellable, error);
}

static xboolean_t
xfile_output_stream_can_truncate (xfile_output_stream_t  *stream)
{
  GFileOutputStreamClass *class;
  xboolean_t can_truncate;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), FALSE);

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);

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
xfile_output_stream_seekable_can_truncate (xseekable__t  *seekable)
{
  return xfile_output_stream_can_truncate (XFILE_OUTPUT_STREAM (seekable));
}

static xboolean_t
xfile_output_stream_truncate (xfile_output_stream_t  *stream,
			       xoffset_t             size,
			       xcancellable_t       *cancellable,
			       xerror_t            **error)
{
  GFileOutputStreamClass *class;
  xoutput_stream_t *output_stream;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE_OUTPUT_STREAM (stream), FALSE);

  output_stream = G_OUTPUT_STREAM (stream);
  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);

  if (!class->truncate_fn)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Truncate not supported on stream"));
      return FALSE;
    }

  if (!xoutput_stream_set_pending (output_stream, error))
    return FALSE;

  if (cancellable)
    xcancellable_push_current (cancellable);

  res = class->truncate_fn (stream, size, cancellable, error);

  if (cancellable)
    xcancellable_pop_current (cancellable);

  xoutput_stream_clear_pending (output_stream);

  return res;
}

static xboolean_t
xfile_output_stream_seekable_truncate (xseekable__t     *seekable,
					xoffset_t        size,
					xcancellable_t  *cancellable,
					xerror_t       **error)
{
  return xfile_output_stream_truncate (XFILE_OUTPUT_STREAM (seekable),
					size, cancellable, error);
}
/********************************************
 *   Default implementation of async ops    *
 ********************************************/

static void
query_info_async_thread (xtask_t        *task,
                         xpointer_t      source_object,
                         xpointer_t      task_data,
                         xcancellable_t *cancellable)
{
  xfile_output_stream_t *stream = source_object;
  const char *attributes = task_data;
  GFileOutputStreamClass *class;
  xerror_t *error = NULL;
  xfile_info_t *info = NULL;

  class = XFILE_OUTPUT_STREAM_GET_CLASS (stream);
  if (class->query_info)
    info = class->query_info (stream, attributes, cancellable, &error);
  else
    g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         _("Stream doesn’t support query_info"));

  if (info == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, info, xobject_unref);
}

static void
xfile_output_stream_real_query_info_async (xfile_output_stream_t     *stream,
					       const char           *attributes,
					       int                   io_priority,
					       xcancellable_t         *cancellable,
					       xasync_ready_callback_t   callback,
					       xpointer_t              user_data)
{
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_output_stream_real_query_info_async);
  xtask_set_task_data (task, xstrdup (attributes), g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, query_info_async_thread);
  xobject_unref (task);
}

static xfile_info_t *
xfile_output_stream_real_query_info_finish (xfile_output_stream_t     *stream,
					     xasync_result_t         *res,
					     xerror_t              **error)
{
  g_return_val_if_fail (xtask_is_valid (res, stream), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}
