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
#include <gfileinputstream.h>
#include <gseekable.h>
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gfileinputstream
 * @short_description: File input streaming operations
 * @include: gio/gio.h
 * @see_also: #xinput_stream_t, #xdata_input_stream_t, #xseekable__t
 *
 * xfile_input_stream_t provides input streams that take their
 * content from a file.
 *
 * xfile_input_stream_t implements #xseekable__t, which allows the input
 * stream to jump to arbitrary positions in the file, provided the
 * filesystem of the file allows it. To find the position of a file
 * input stream, use xseekable_tell(). To find out if a file input
 * stream supports seeking, use xseekable_can_seek().
 * To position a file input stream, use xseekable_seek().
 **/

static void       xfile_input_stream_seekable_iface_init    (xseekable_iface_t       *iface);
static xoffset_t    xfile_input_stream_seekable_tell          (xseekable__t            *seekable);
static xboolean_t   xfile_input_stream_seekable_can_seek      (xseekable__t            *seekable);
static xboolean_t   xfile_input_stream_seekable_seek          (xseekable__t            *seekable,
							      xoffset_t               offset,
							      GSeekType             type,
							      xcancellable_t         *cancellable,
							      xerror_t              **error);
static xboolean_t   xfile_input_stream_seekable_can_truncate  (xseekable__t            *seekable);
static xboolean_t   xfile_input_stream_seekable_truncate      (xseekable__t            *seekable,
							      xoffset_t               offset,
							      xcancellable_t         *cancellable,
							      xerror_t              **error);
static void       xfile_input_stream_real_query_info_async  (xfile_input_stream_t     *stream,
							      const char           *attributes,
							      int                   io_priority,
							      xcancellable_t         *cancellable,
							      xasync_ready_callback_t   callback,
							      xpointer_t              user_data);
static xfile_info_t *xfile_input_stream_real_query_info_finish (xfile_input_stream_t     *stream,
							      xasync_result_t         *result,
							      xerror_t              **error);


struct _GFileInputStreamPrivate {
  xasync_ready_callback_t outstanding_callback;
};

G_DEFINE_TYPE_WITH_CODE (xfile_input_stream, xfile_input_stream, XTYPE_INPUT_STREAM,
                         G_ADD_PRIVATE (xfile_input_stream_t)
			 G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
						xfile_input_stream_seekable_iface_init))

static void
xfile_input_stream_class_init (GFileInputStreamClass *klass)
{
  klass->query_info_async = xfile_input_stream_real_query_info_async;
  klass->query_info_finish = xfile_input_stream_real_query_info_finish;
}

static void
xfile_input_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell = xfile_input_stream_seekable_tell;
  iface->can_seek = xfile_input_stream_seekable_can_seek;
  iface->seek = xfile_input_stream_seekable_seek;
  iface->can_truncate = xfile_input_stream_seekable_can_truncate;
  iface->truncate_fn = xfile_input_stream_seekable_truncate;
}

static void
xfile_input_stream_init (xfile_input_stream_t *stream)
{
  stream->priv = xfile_input_stream_get_instance_private (stream);
}

/**
 * xfile_input_stream_query_info:
 * @stream: a #xfile_input_stream_t.
 * @attributes: a file attribute query string.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Queries a file input stream the given @attributes. This function blocks
 * while querying the stream. For the asynchronous (non-blocking) version
 * of this function, see xfile_input_stream_query_info_async(). While the
 * stream is blocked, the stream will set the pending flag internally, and
 * any other operations on the stream will fail with %G_IO_ERROR_PENDING.
 *
 * Returns: (transfer full): a #xfile_info_t, or %NULL on error.
 **/
xfile_info_t *
xfile_input_stream_query_info (xfile_input_stream_t  *stream,
                                const char        *attributes,
                                xcancellable_t      *cancellable,
                                xerror_t           **error)
{
  GFileInputStreamClass *class;
  xinput_stream_t *input_stream;
  xfile_info_t *info;

  g_return_val_if_fail (X_IS_FILE_INPUT_STREAM (stream), NULL);

  input_stream = G_INPUT_STREAM (stream);

  if (!xinput_stream_set_pending (input_stream, error))
    return NULL;

  info = NULL;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  class = XFILE_INPUT_STREAM_GET_CLASS (stream);
  if (class->query_info)
    info = class->query_info (stream, attributes, cancellable, error);
  else
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         _("Stream doesn’t support query_info"));

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (input_stream);

  return info;
}

static void
async_ready_callback_wrapper (xobject_t      *source_object,
                              xasync_result_t *res,
                              xpointer_t      user_data)
{
  xfile_input_stream_t *stream = XFILE_INPUT_STREAM (source_object);

  xinput_stream_clear_pending (G_INPUT_STREAM (stream));
  if (stream->priv->outstanding_callback)
    (*stream->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (stream);
}

/**
 * xfile_input_stream_query_info_async:
 * @stream: a #xfile_input_stream_t.
 * @attributes: a file attribute query string.
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Queries the stream information asynchronously.
 * When the operation is finished @callback will be called.
 * You can then call xfile_input_stream_query_info_finish()
 * to get the result of the operation.
 *
 * For the synchronous version of this function,
 * see xfile_input_stream_query_info().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be set
 *
 **/
void
xfile_input_stream_query_info_async (xfile_input_stream_t    *stream,
                                      const char          *attributes,
                                      int                  io_priority,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  GFileInputStreamClass *klass;
  xinput_stream_t *input_stream;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_FILE_INPUT_STREAM (stream));

  input_stream = G_INPUT_STREAM (stream);

  if (!xinput_stream_set_pending (input_stream, &error))
    {
      xtask_report_error (stream, callback, user_data,
                           xfile_input_stream_query_info_async,
                           error);
      return;
    }

  klass = XFILE_INPUT_STREAM_GET_CLASS (stream);

  stream->priv->outstanding_callback = callback;
  xobject_ref (stream);
  klass->query_info_async (stream, attributes, io_priority, cancellable,
                           async_ready_callback_wrapper, user_data);
}

/**
 * xfile_input_stream_query_info_finish:
 * @stream: a #xfile_input_stream_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring,
 *     or %NULL to ignore.
 *
 * Finishes an asynchronous info query operation.
 *
 * Returns: (transfer full): #xfile_info_t.
 **/
xfile_info_t *
xfile_input_stream_query_info_finish (xfile_input_stream_t  *stream,
                                       xasync_result_t      *result,
                                       xerror_t           **error)
{
  GFileInputStreamClass *class;

  g_return_val_if_fail (X_IS_FILE_INPUT_STREAM (stream), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, xfile_input_stream_query_info_async))
    return xtask_propagate_pointer (XTASK (result), error);

  class = XFILE_INPUT_STREAM_GET_CLASS (stream);
  return class->query_info_finish (stream, result, error);
}

static xoffset_t
xfile_input_stream_tell (xfile_input_stream_t *stream)
{
  GFileInputStreamClass *class;
  xoffset_t offset;

  g_return_val_if_fail (X_IS_FILE_INPUT_STREAM (stream), 0);

  class = XFILE_INPUT_STREAM_GET_CLASS (stream);

  offset = 0;
  if (class->tell)
    offset = class->tell (stream);

  return offset;
}

static xoffset_t
xfile_input_stream_seekable_tell (xseekable__t *seekable)
{
  return xfile_input_stream_tell (XFILE_INPUT_STREAM (seekable));
}

static xboolean_t
xfile_input_stream_can_seek (xfile_input_stream_t *stream)
{
  GFileInputStreamClass *class;
  xboolean_t can_seek;

  g_return_val_if_fail (X_IS_FILE_INPUT_STREAM (stream), FALSE);

  class = XFILE_INPUT_STREAM_GET_CLASS (stream);

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
xfile_input_stream_seekable_can_seek (xseekable__t *seekable)
{
  return xfile_input_stream_can_seek (XFILE_INPUT_STREAM (seekable));
}

static xboolean_t
xfile_input_stream_seek (xfile_input_stream_t  *stream,
			  xoffset_t            offset,
			  GSeekType          type,
			  xcancellable_t      *cancellable,
			  xerror_t           **error)
{
  GFileInputStreamClass *class;
  xinput_stream_t *input_stream;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE_INPUT_STREAM (stream), FALSE);

  input_stream = G_INPUT_STREAM (stream);
  class = XFILE_INPUT_STREAM_GET_CLASS (stream);

  if (!class->seek)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Seek not supported on stream"));
      return FALSE;
    }

  if (!xinput_stream_set_pending (input_stream, error))
    return FALSE;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = class->seek (stream, offset, type, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  xinput_stream_clear_pending (input_stream);

  return res;
}

static xboolean_t
xfile_input_stream_seekable_seek (xseekable__t     *seekable,
				   xoffset_t        offset,
				   GSeekType      type,
				   xcancellable_t  *cancellable,
				   xerror_t       **error)
{
  return xfile_input_stream_seek (XFILE_INPUT_STREAM (seekable),
				   offset, type, cancellable, error);
}

static xboolean_t
xfile_input_stream_seekable_can_truncate (xseekable__t *seekable)
{
  return FALSE;
}

static xboolean_t
xfile_input_stream_seekable_truncate (xseekable__t     *seekable,
				       xoffset_t        offset,
				       xcancellable_t  *cancellable,
				       xerror_t       **error)
{
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Truncate not allowed on input stream"));
  return FALSE;
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
  xfile_input_stream_t *stream = source_object;
  const char *attributes = task_data;
  GFileInputStreamClass *class;
  xerror_t *error = NULL;
  xfile_info_t *info = NULL;

  class = XFILE_INPUT_STREAM_GET_CLASS (stream);
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
xfile_input_stream_real_query_info_async (xfile_input_stream_t    *stream,
                                           const char          *attributes,
                                           int                  io_priority,
                                           xcancellable_t        *cancellable,
                                           xasync_ready_callback_t  callback,
                                           xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_input_stream_real_query_info_async);
  xtask_set_task_data (task, xstrdup (attributes), g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, query_info_async_thread);
  xobject_unref (task);
}

static xfile_info_t *
xfile_input_stream_real_query_info_finish (xfile_input_stream_t  *stream,
                                            xasync_result_t      *res,
                                            xerror_t           **error)
{
  g_return_val_if_fail (xtask_is_valid (res, stream), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}
