/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 codethink
 * Copyright © 2009 Red Hat, Inc.
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "giostream.h"
#include "gasyncresult.h"
#include "gioprivate.h"
#include "gtask.h"

/**
 * SECTION:giostream
 * @short_description: Base class for implementing read/write streams
 * @include: gio/gio.h
 * @see_also: #xinput_stream_t, #xoutput_stream_t
 *
 * xio_stream_t represents an object that has both read and write streams.
 * Generally the two streams act as separate input and output streams,
 * but they share some common resources and state. For instance, for
 * seekable streams, both streams may use the same position.
 *
 * Examples of #xio_stream_t objects are #xsocket_connection_t, which represents
 * a two-way network connection; and #xfile_io_stream_t, which represents a
 * file handle opened in read-write mode.
 *
 * To do the actual reading and writing you need to get the substreams
 * with g_io_stream_get_input_stream() and g_io_stream_get_output_stream().
 *
 * The #xio_stream_t object owns the input and the output streams, not the other
 * way around, so keeping the substreams alive will not keep the #xio_stream_t
 * object alive. If the #xio_stream_t object is freed it will be closed, thus
 * closing the substreams, so even if the substreams stay alive they will
 * always return %G_IO_ERROR_CLOSED for all operations.
 *
 * To close a stream use g_io_stream_close() which will close the common
 * stream object and also the individual substreams. You can also close
 * the substreams themselves. In most cases this only marks the
 * substream as closed, so further I/O on it fails but common state in the
 * #xio_stream_t may still be open. However, some streams may support
 * "half-closed" states where one direction of the stream is actually shut down.
 *
 * Operations on #GIOStreams cannot be started while another operation on the
 * #xio_stream_t or its substreams is in progress. Specifically, an application can
 * read from the #xinput_stream_t and write to the #xoutput_stream_t simultaneously
 * (either in separate threads, or as asynchronous operations in the same
 * thread), but an application cannot start any #xio_stream_t operation while there
 * is a #xio_stream_t, #xinput_stream_t or #xoutput_stream_t operation in progress, and
 * an application can’t start any #xinput_stream_t or #xoutput_stream_t operation
 * while there is a #xio_stream_t operation in progress.
 *
 * This is a product of individual stream operations being associated with a
 * given #xmain_context_t (the thread-default context at the time the operation was
 * started), rather than entire streams being associated with a single
 * #xmain_context_t.
 *
 * GIO may run operations on #GIOStreams from other (worker) threads, and this
 * may be exposed to application code in the behaviour of wrapper streams, such
 * as #xbuffered_input_stream or #xtls_connection_t. With such wrapper APIs,
 * application code may only run operations on the base (wrapped) stream when
 * the wrapper stream is idle. Note that the semantics of such operations may
 * not be well-defined due to the state the wrapper stream leaves the base
 * stream in (though they are guaranteed not to crash).
 *
 * Since: 2.22
 */

enum
{
  PROP_0,
  PROP_INPUT_STREAM,
  PROP_OUTPUT_STREAM,
  PROP_CLOSED
};

struct _xio_stream_private {
  xuint_t closed : 1;
  xuint_t pending : 1;
};

static xboolean_t g_io_stream_real_close        (xio_stream_t            *stream,
					       xcancellable_t         *cancellable,
					       xerror_t              **error);
static void     g_io_stream_real_close_async  (xio_stream_t            *stream,
					       int                   io_priority,
					       xcancellable_t         *cancellable,
					       xasync_ready_callback_t   callback,
					       xpointer_t              user_data);
static xboolean_t g_io_stream_real_close_finish (xio_stream_t            *stream,
					       xasync_result_t         *result,
					       xerror_t              **error);

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xio_stream_t, g_io_stream, XTYPE_OBJECT)

static void
g_io_stream_dispose (xobject_t *object)
{
  xio_stream_t *stream;

  stream = XIO_STREAM (object);

  if (!stream->priv->closed)
    g_io_stream_close (stream, NULL, NULL);

  G_OBJECT_CLASS (g_io_stream_parent_class)->dispose (object);
}

static void
g_io_stream_init (xio_stream_t *stream)
{
  stream->priv = g_io_stream_get_instance_private (stream);
}

static void
g_io_stream_get_property (xobject_t    *object,
			  xuint_t       prop_id,
			  xvalue_t     *value,
			  xparam_spec_t *pspec)
{
  xio_stream_t *stream = XIO_STREAM (object);

  switch (prop_id)
    {
      case PROP_CLOSED:
        xvalue_set_boolean (value, stream->priv->closed);
        break;

      case PROP_INPUT_STREAM:
        xvalue_set_object (value, g_io_stream_get_input_stream (stream));
        break;

      case PROP_OUTPUT_STREAM:
        xvalue_set_object (value, g_io_stream_get_output_stream (stream));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_io_stream_class_init (xio_stream_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = g_io_stream_dispose;
  gobject_class->get_property = g_io_stream_get_property;

  klass->close_fn = g_io_stream_real_close;
  klass->close_async = g_io_stream_real_close_async;
  klass->close_finish = g_io_stream_real_close_finish;

  xobject_class_install_property (gobject_class, PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         P_("Closed"),
                                                         P_("Is the stream closed"),
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class, PROP_INPUT_STREAM,
				   g_param_spec_object ("input-stream",
							P_("Input stream"),
							P_("The xinput_stream_t to read from"),
							XTYPE_INPUT_STREAM,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  xobject_class_install_property (gobject_class, PROP_OUTPUT_STREAM,
				   g_param_spec_object ("output-stream",
							P_("Output stream"),
							P_("The xoutput_stream_t to write to"),
							XTYPE_OUTPUT_STREAM,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

/**
 * g_io_stream_is_closed:
 * @stream: a #xio_stream_t
 *
 * Checks if a stream is closed.
 *
 * Returns: %TRUE if the stream is closed.
 *
 * Since: 2.22
 */
xboolean_t
g_io_stream_is_closed (xio_stream_t *stream)
{
  g_return_val_if_fail (X_IS_IO_STREAM (stream), TRUE);

  return stream->priv->closed;
}

/**
 * g_io_stream_get_input_stream:
 * @stream: a #xio_stream_t
 *
 * Gets the input stream for this object. This is used
 * for reading.
 *
 * Returns: (transfer none): a #xinput_stream_t, owned by the #xio_stream_t.
 * Do not free.
 *
 * Since: 2.22
 */
xinput_stream_t *
g_io_stream_get_input_stream (xio_stream_t *stream)
{
  xio_stream_class_t *klass;

  klass = XIO_STREAM_GET_CLASS (stream);

  g_assert (klass->get_input_stream != NULL);

  return klass->get_input_stream (stream);
}

/**
 * g_io_stream_get_output_stream:
 * @stream: a #xio_stream_t
 *
 * Gets the output stream for this object. This is used for
 * writing.
 *
 * Returns: (transfer none): a #xoutput_stream_t, owned by the #xio_stream_t.
 * Do not free.
 *
 * Since: 2.22
 */
xoutput_stream_t *
g_io_stream_get_output_stream (xio_stream_t *stream)
{
  xio_stream_class_t *klass;

  klass = XIO_STREAM_GET_CLASS (stream);

  g_assert (klass->get_output_stream != NULL);
  return klass->get_output_stream (stream);
}

/**
 * g_io_stream_has_pending:
 * @stream: a #xio_stream_t
 *
 * Checks if a stream has pending actions.
 *
 * Returns: %TRUE if @stream has pending actions.
 *
 * Since: 2.22
 **/
xboolean_t
g_io_stream_has_pending (xio_stream_t *stream)
{
  g_return_val_if_fail (X_IS_IO_STREAM (stream), FALSE);

  return stream->priv->pending;
}

/**
 * g_io_stream_set_pending:
 * @stream: a #xio_stream_t
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore
 *
 * Sets @stream to have actions pending. If the pending flag is
 * already set or @stream is closed, it will return %FALSE and set
 * @error.
 *
 * Returns: %TRUE if pending was previously unset and is now set.
 *
 * Since: 2.22
 */
xboolean_t
g_io_stream_set_pending (xio_stream_t  *stream,
			 xerror_t    **error)
{
  g_return_val_if_fail (X_IS_IO_STREAM (stream), FALSE);

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
 * g_io_stream_clear_pending:
 * @stream: a #xio_stream_t
 *
 * Clears the pending flag on @stream.
 *
 * Since: 2.22
 */
void
g_io_stream_clear_pending (xio_stream_t *stream)
{
  g_return_if_fail (X_IS_IO_STREAM (stream));

  stream->priv->pending = FALSE;
}

static xboolean_t
g_io_stream_real_close (xio_stream_t     *stream,
			xcancellable_t  *cancellable,
			xerror_t       **error)
{
  xboolean_t res;

  res = xoutput_stream_close (g_io_stream_get_output_stream (stream),
			       cancellable, error);

  /* If this errored out, unset error so that we don't report
     further errors, but still do the following ops */
  if (error != NULL && *error != NULL)
    error = NULL;

  res &= xinput_stream_close (g_io_stream_get_input_stream (stream),
			       cancellable, error);

  return res;
}

/**
 * g_io_stream_close:
 * @stream: a #xio_stream_t
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Closes the stream, releasing resources related to it. This will also
 * close the individual input and output streams, if they are not already
 * closed.
 *
 * Once the stream is closed, all other operations will return
 * %G_IO_ERROR_CLOSED. Closing a stream multiple times will not
 * return an error.
 *
 * Closing a stream will automatically flush any outstanding buffers
 * in the stream.
 *
 * Streams will be automatically closed when the last reference
 * is dropped, but you might want to call this function to make sure
 * resources are released as early as possible.
 *
 * Some streams might keep the backing store of the stream (e.g. a file
 * descriptor) open after the stream is closed. See the documentation for
 * the individual stream for details.
 *
 * On failure the first error that happened will be reported, but the
 * close operation will finish as much as possible. A stream that failed
 * to close will still return %G_IO_ERROR_CLOSED for all operations.
 * Still, it is important to check and report the error to the user,
 * otherwise there might be a loss of data as all data might not be written.
 *
 * If @cancellable is not NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 * Cancelling a close will still leave the stream closed, but some streams
 * can use a faster close that doesn't block to e.g. check errors.
 *
 * The default implementation of this method just calls close on the
 * individual input/output streams.
 *
 * Returns: %TRUE on success, %FALSE on failure
 *
 * Since: 2.22
 */
xboolean_t
g_io_stream_close (xio_stream_t     *stream,
		   xcancellable_t  *cancellable,
		   xerror_t       **error)
{
  xio_stream_class_t *class;
  xboolean_t res;

  g_return_val_if_fail (X_IS_IO_STREAM (stream), FALSE);

  class = XIO_STREAM_GET_CLASS (stream);

  if (stream->priv->closed)
    return TRUE;

  if (!g_io_stream_set_pending (stream, error))
    return FALSE;

  if (cancellable)
    g_cancellable_push_current (cancellable);

  res = TRUE;
  if (class->close_fn)
    res = class->close_fn (stream, cancellable, error);

  if (cancellable)
    g_cancellable_pop_current (cancellable);

  stream->priv->closed = TRUE;
  g_io_stream_clear_pending (stream);

  return res;
}

static void
async_ready_close_callback_wrapper (xobject_t      *source_object,
                                    xasync_result_t *res,
                                    xpointer_t      user_data)
{
  xio_stream_t *stream = XIO_STREAM (source_object);
  xio_stream_class_t *klass = XIO_STREAM_GET_CLASS (stream);
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xboolean_t success;

  stream->priv->closed = TRUE;
  g_io_stream_clear_pending (stream);

  if (xasync_result_legacy_propagate_error (res, &error))
    success = FALSE;
  else
    success = klass->close_finish (stream, res, &error);

  if (error)
    xtask_return_error (task, error);
  else
    xtask_return_boolean (task, success);

  xobject_unref (task);
}

/**
 * g_io_stream_close_async:
 * @stream: a #xio_stream_t
 * @io_priority: the io priority of the request
 * @cancellable: (nullable): optional cancellable object
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Requests an asynchronous close of the stream, releasing resources
 * related to it. When the operation is finished @callback will be
 * called. You can then call g_io_stream_close_finish() to get
 * the result of the operation.
 *
 * For behaviour details see g_io_stream_close().
 *
 * The asynchronous methods have a default fallback that uses threads
 * to implement asynchronicity, so they are optional for inheriting
 * classes. However, if you override one you must override all.
 *
 * Since: 2.22
 */
void
g_io_stream_close_async (xio_stream_t           *stream,
			 int                  io_priority,
			 xcancellable_t        *cancellable,
			 xasync_ready_callback_t  callback,
			 xpointer_t             user_data)
{
  xio_stream_class_t *class;
  xerror_t *error = NULL;
  xtask_t *task;

  g_return_if_fail (X_IS_IO_STREAM (stream));

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_io_stream_close_async);

  if (stream->priv->closed)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
      return;
    }

  if (!g_io_stream_set_pending (stream, &error))
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  class = XIO_STREAM_GET_CLASS (stream);

  class->close_async (stream, io_priority, cancellable,
		      async_ready_close_callback_wrapper, task);
}

/**
 * g_io_stream_close_finish:
 * @stream: a #xio_stream_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *    ignore
 *
 * Closes a stream.
 *
 * Returns: %TRUE if stream was successfully closed, %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
g_io_stream_close_finish (xio_stream_t     *stream,
			  xasync_result_t  *result,
			  xerror_t       **error)
{
  g_return_val_if_fail (X_IS_IO_STREAM (stream), FALSE);
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}


static void
close_async_thread (xtask_t        *task,
                    xpointer_t      source_object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xio_stream_t *stream = source_object;
  xio_stream_class_t *class;
  xerror_t *error = NULL;
  xboolean_t result;

  class = XIO_STREAM_GET_CLASS (stream);
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

typedef struct
{
  xerror_t *error;
  xint_t pending;
} CloseAsyncData;

static void
stream_close_complete (xobject_t      *source,
                       xasync_result_t *result,
                       xpointer_t      user_data)
{
  xtask_t *task = user_data;
  CloseAsyncData *data;

  data = xtask_get_task_data (task);
  data->pending--;

  if (X_IS_OUTPUT_STREAM (source))
    {
      xerror_t *error = NULL;

      /* Match behaviour with the sync route and give precedent to the
       * error returned from closing the output stream.
       */
      xoutput_stream_close_finish (G_OUTPUT_STREAM (source), result, &error);
      if (error)
        {
          if (data->error)
            xerror_free (data->error);
          data->error = error;
        }
    }
  else
    xinput_stream_close_finish (G_INPUT_STREAM (source), result, data->error ? NULL : &data->error);

  if (data->pending == 0)
    {
      if (data->error)
        xtask_return_error (task, data->error);
      else
        xtask_return_boolean (task, TRUE);

      g_slice_free (CloseAsyncData, data);
      xobject_unref (task);
    }
}

static void
g_io_stream_real_close_async (xio_stream_t           *stream,
			      int                  io_priority,
			      xcancellable_t        *cancellable,
			      xasync_ready_callback_t  callback,
			      xpointer_t             user_data)
{
  xinput_stream_t *input;
  xoutput_stream_t *output;
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_io_stream_real_close_async);
  xtask_set_check_cancellable (task, FALSE);
  xtask_set_priority (task, io_priority);

  input = g_io_stream_get_input_stream (stream);
  output = g_io_stream_get_output_stream (stream);

  if (xinput_stream_async_close_is_via_threads (input) && xoutput_stream_async_close_is_via_threads (output))
    {
      /* No sense in dispatching to the thread twice -- just do it all
       * in one go.
       */
      xtask_run_in_thread (task, close_async_thread);
      xobject_unref (task);
    }
  else
    {
      CloseAsyncData *data;

      /* We should avoid dispatching to another thread in case either
       * object that would not do it for itself because it may not be
       * threadsafe.
       */
      data = g_slice_new (CloseAsyncData);
      data->error = NULL;
      data->pending = 2;

      xtask_set_task_data (task, data, NULL);
      xinput_stream_close_async (input, io_priority, cancellable, stream_close_complete, task);
      xoutput_stream_close_async (output, io_priority, cancellable, stream_close_complete, task);
    }
}

static xboolean_t
g_io_stream_real_close_finish (xio_stream_t     *stream,
			       xasync_result_t  *result,
			       xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

typedef struct
{
  xio_stream_t *stream1;
  xio_stream_t *stream2;
  xio_stream_splice_flags_t flags;
  xint_t io_priority;
  xcancellable_t *cancellable;
  gulong cancelled_id;
  xcancellable_t *op1_cancellable;
  xcancellable_t *op2_cancellable;
  xuint_t completed;
  xerror_t *error;
} SpliceContext;

static void
splice_context_free (SpliceContext *ctx)
{
  xobject_unref (ctx->stream1);
  xobject_unref (ctx->stream2);
  if (ctx->cancellable != NULL)
    xobject_unref (ctx->cancellable);
  xobject_unref (ctx->op1_cancellable);
  xobject_unref (ctx->op2_cancellable);
  g_clear_error (&ctx->error);
  g_slice_free (SpliceContext, ctx);
}

static void
splice_complete (xtask_t         *task,
                 SpliceContext *ctx)
{
  if (ctx->cancelled_id != 0)
    g_cancellable_disconnect (ctx->cancellable, ctx->cancelled_id);
  ctx->cancelled_id = 0;

  if (ctx->error != NULL)
    {
      xtask_return_error (task, ctx->error);
      ctx->error = NULL;
    }
  else
    xtask_return_boolean (task, TRUE);
}

static void
splice_close_cb (xobject_t      *iostream,
                 xasync_result_t *res,
                 xpointer_t      user_data)
{
  xtask_t *task = user_data;
  SpliceContext *ctx = xtask_get_task_data (task);
  xerror_t *error = NULL;

  g_io_stream_close_finish (XIO_STREAM (iostream), res, &error);

  ctx->completed++;

  /* Keep the first error that occurred */
  if (error != NULL && ctx->error == NULL)
    ctx->error = error;
  else
    g_clear_error (&error);

  /* If all operations are done, complete now */
  if (ctx->completed == 4)
    splice_complete (task, ctx);

  xobject_unref (task);
}

static void
splice_cb (xobject_t      *ostream,
           xasync_result_t *res,
           xpointer_t      user_data)
{
  xtask_t *task = user_data;
  SpliceContext *ctx = xtask_get_task_data (task);
  xerror_t *error = NULL;

  xoutput_stream_splice_finish (G_OUTPUT_STREAM (ostream), res, &error);

  ctx->completed++;

  /* ignore cancellation error if it was not requested by the user */
  if (error != NULL &&
      xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) &&
      (ctx->cancellable == NULL ||
       !g_cancellable_is_cancelled (ctx->cancellable)))
    g_clear_error (&error);

  /* Keep the first error that occurred */
  if (error != NULL && ctx->error == NULL)
    ctx->error = error;
  else
    g_clear_error (&error);

   if (ctx->completed == 1 &&
       (ctx->flags & G_IO_STREAM_SPLICE_WAIT_FOR_BOTH) == 0)
    {
      /* We don't want to wait for the 2nd operation to finish, cancel it */
      g_cancellable_cancel (ctx->op1_cancellable);
      g_cancellable_cancel (ctx->op2_cancellable);
    }
  else if (ctx->completed == 2)
    {
      if (ctx->cancellable == NULL ||
          !g_cancellable_is_cancelled (ctx->cancellable))
        {
          g_cancellable_reset (ctx->op1_cancellable);
          g_cancellable_reset (ctx->op2_cancellable);
        }

      /* Close the IO streams if needed */
      if ((ctx->flags & G_IO_STREAM_SPLICE_CLOSE_STREAM1) != 0)
	{
	  g_io_stream_close_async (ctx->stream1,
                                   xtask_get_priority (task),
                                   ctx->op1_cancellable,
                                   splice_close_cb, xobject_ref (task));
	}
      else
        ctx->completed++;

      if ((ctx->flags & G_IO_STREAM_SPLICE_CLOSE_STREAM2) != 0)
	{
	  g_io_stream_close_async (ctx->stream2,
                                   xtask_get_priority (task),
                                   ctx->op2_cancellable,
                                   splice_close_cb, xobject_ref (task));
	}
      else
        ctx->completed++;

      /* If all operations are done, complete now */
      if (ctx->completed == 4)
        splice_complete (task, ctx);
    }

  xobject_unref (task);
}

static void
splice_cancelled_cb (xcancellable_t *cancellable,
                     xtask_t        *task)
{
  SpliceContext *ctx;

  ctx = xtask_get_task_data (task);
  g_cancellable_cancel (ctx->op1_cancellable);
  g_cancellable_cancel (ctx->op2_cancellable);
}

/**
 * g_io_stream_splice_async:
 * @stream1: a #xio_stream_t.
 * @stream2: a #xio_stream_t.
 * @flags: a set of #xio_stream_splice_flags_t.
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @user_data: (closure): user data passed to @callback.
 *
 * Asynchronously splice the output stream of @stream1 to the input stream of
 * @stream2, and splice the output stream of @stream2 to the input stream of
 * @stream1.
 *
 * When the operation is finished @callback will be called.
 * You can then call g_io_stream_splice_finish() to get the
 * result of the operation.
 *
 * Since: 2.28
 **/
void
g_io_stream_splice_async (xio_stream_t            *stream1,
                          xio_stream_t            *stream2,
                          xio_stream_splice_flags_t  flags,
                          xint_t                  io_priority,
                          xcancellable_t         *cancellable,
                          xasync_ready_callback_t   callback,
                          xpointer_t              user_data)
{
  xtask_t *task;
  SpliceContext *ctx;
  xinput_stream_t *istream;
  xoutput_stream_t *ostream;

  if (cancellable != NULL && g_cancellable_is_cancelled (cancellable))
    {
      xtask_report_new_error (NULL, callback, user_data,
                               g_io_stream_splice_async,
                               G_IO_ERROR, G_IO_ERROR_CANCELLED,
                               "Operation has been cancelled");
      return;
    }

  ctx = g_slice_new0 (SpliceContext);
  ctx->stream1 = xobject_ref (stream1);
  ctx->stream2 = xobject_ref (stream2);
  ctx->flags = flags;
  ctx->op1_cancellable = g_cancellable_new ();
  ctx->op2_cancellable = g_cancellable_new ();
  ctx->completed = 0;

  task = xtask_new (NULL, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_io_stream_splice_async);
  xtask_set_task_data (task, ctx, (xdestroy_notify_t) splice_context_free);

  if (cancellable != NULL)
    {
      ctx->cancellable = xobject_ref (cancellable);
      ctx->cancelled_id = g_cancellable_connect (cancellable,
          G_CALLBACK (splice_cancelled_cb), xobject_ref (task),
          xobject_unref);
    }

  istream = g_io_stream_get_input_stream (stream1);
  ostream = g_io_stream_get_output_stream (stream2);
  xoutput_stream_splice_async (ostream, istream, G_OUTPUT_STREAM_SPLICE_NONE,
      io_priority, ctx->op1_cancellable, splice_cb,
      xobject_ref (task));

  istream = g_io_stream_get_input_stream (stream2);
  ostream = g_io_stream_get_output_stream (stream1);
  xoutput_stream_splice_async (ostream, istream, G_OUTPUT_STREAM_SPLICE_NONE,
      io_priority, ctx->op2_cancellable, splice_cb,
      xobject_ref (task));

  xobject_unref (task);
}

/**
 * g_io_stream_splice_finish:
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an asynchronous io stream splice operation.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.28
 **/
xboolean_t
g_io_stream_splice_finish (xasync_result_t  *result,
                           xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (result, NULL), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}
