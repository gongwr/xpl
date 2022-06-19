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

#include "gsimpleasyncresult.h"
#include "gasyncresult.h"
#include "gcancellable.h"
#include "gioscheduler.h"
#include <gio/gioerror.h>
#include "glibintl.h"


/**
 * SECTION:gsimpleasyncresult
 * @short_description: Simple asynchronous results implementation
 * @include: gio/gio.h
 * @see_also: #xasync_result_t, #xtask_t
 *
 * As of GLib 2.46, #xsimple_async_result_t is deprecated in favor of
 * #xtask_t, which provides a simpler API.
 *
 * #xsimple_async_result_t implements #xasync_result_t.
 *
 * xsimple_async_result_t handles #GAsyncReadyCallbacks, error
 * reporting, operation cancellation and the final state of an operation,
 * completely transparent to the application. Results can be returned
 * as a pointer e.g. for functions that return data that is collected
 * asynchronously, a boolean value for checking the success or failure
 * of an operation, or a #xssize_t for operations which return the number
 * of bytes modified by the operation; all of the simple return cases
 * are covered.
 *
 * Most of the time, an application will not need to know of the details
 * of this API; it is handled transparently, and any necessary operations
 * are handled by #xasync_result_t's interface. However, if implementing a
 * new GIO module, for writing language bindings, or for complex
 * applications that need better control of how asynchronous operations
 * are completed, it is important to understand this functionality.
 *
 * GSimpleAsyncResults are tagged with the calling function to ensure
 * that asynchronous functions and their finishing functions are used
 * together correctly.
 *
 * To create a new #xsimple_async_result_t, call xsimple_async_result_new().
 * If the result needs to be created for a #xerror_t, use
 * xsimple_async_result_new_from_error() or
 * xsimple_async_result_new_take_error(). If a #xerror_t is not available
 * (e.g. the asynchronous operation's doesn't take a #xerror_t argument),
 * but the result still needs to be created for an error condition, use
 * xsimple_async_result_new_error() (or xsimple_async_result_set_error_va()
 * if your application or binding requires passing a variable argument list
 * directly), and the error can then be propagated through the use of
 * xsimple_async_result_propagate_error().
 *
 * An asynchronous operation can be made to ignore a cancellation event by
 * calling xsimple_async_result_set_handle_cancellation() with a
 * #xsimple_async_result_t for the operation and %FALSE. This is useful for
 * operations that are dangerous to cancel, such as close (which would
 * cause a leak if cancelled before being run).
 *
 * xsimple_async_result_t can integrate into GLib's event loop, #xmain_loop_t,
 * or it can use #GThreads.
 * xsimple_async_result_complete() will finish an I/O task directly
 * from the point where it is called. xsimple_async_result_complete_in_idle()
 * will finish it from an idle handler in the
 * [thread-default main context][g-main-context-push-thread-default]
 * where the #xsimple_async_result_t was created.
 * xsimple_async_result_run_in_thread() will run the job in a
 * separate thread and then use
 * xsimple_async_result_complete_in_idle() to deliver the result.
 *
 * To set the results of an asynchronous function,
 * xsimple_async_result_set_op_res_gpointer(),
 * xsimple_async_result_set_op_res_gboolean(), and
 * xsimple_async_result_set_op_res_gssize()
 * are provided, setting the operation's result to a xpointer_t, xboolean_t, or
 * xssize_t, respectively.
 *
 * Likewise, to get the result of an asynchronous function,
 * xsimple_async_result_get_op_res_gpointer(),
 * xsimple_async_result_get_op_res_gboolean(), and
 * xsimple_async_result_get_op_res_gssize() are
 * provided, getting the operation's result as a xpointer_t, xboolean_t, and
 * xssize_t, respectively.
 *
 * For the details of the requirements implementations must respect, see
 * #xasync_result_t.  A typical implementation of an asynchronous operation
 * using xsimple_async_result_t looks something like this:
 *
 * |[<!-- language="C" -->
 * static void
 * baked_cb (Cake    *cake,
 *           xpointer_t user_data)
 * {
 *   // In this example, this callback is not given a reference to the cake,
 *   // so the xsimple_async_result_t has to take a reference to it.
 *   xsimple_async_result_t *result = user_data;
 *
 *   if (cake == NULL)
 *     xsimple_async_result_set_error (result,
 *                                      BAKER_ERRORS,
 *                                      BAKER_ERROR_NO_FLOUR,
 *                                      "Go to the supermarket");
 *   else
 *     xsimple_async_result_set_op_res_gpointer (result,
 *                                                xobject_ref (cake),
 *                                                xobject_unref);
 *
 *
 *   // In this example, we assume that baked_cb is called as a callback from
 *   // the mainloop, so it's safe to complete the operation synchronously here.
 *   // If, however, _baker_prepare_cake () might call its callback without
 *   // first returning to the mainloop — inadvisable, but some APIs do so —
 *   // we would need to use xsimple_async_result_complete_in_idle().
 *   xsimple_async_result_complete (result);
 *   xobject_unref (result);
 * }
 *
 * void
 * baker_bake_cake_async (Baker              *self,
 *                        xuint_t               radius,
 *                        xasync_ready_callback_t callback,
 *                        xpointer_t            user_data)
 * {
 *   xsimple_async_result_t *simple;
 *   Cake               *cake;
 *
 *   if (radius < 3)
 *     {
 *       g_simple_async_report_error_in_idle (G_OBJECT (self),
 *                                            callback,
 *                                            user_data,
 *                                            BAKER_ERRORS,
 *                                            BAKER_ERROR_TOO_SMALL,
 *                                            "%ucm radius cakes are silly",
 *                                            radius);
 *       return;
 *     }
 *
 *   simple = xsimple_async_result_new (G_OBJECT (self),
 *                                       callback,
 *                                       user_data,
 *                                       baker_bake_cake_async);
 *   cake = _baker_get_cached_cake (self, radius);
 *
 *   if (cake != NULL)
 *     {
 *       xsimple_async_result_set_op_res_gpointer (simple,
 *                                                  xobject_ref (cake),
 *                                                  xobject_unref);
 *       xsimple_async_result_complete_in_idle (simple);
 *       xobject_unref (simple);
 *       // Drop the reference returned by _baker_get_cached_cake();
 *       // the xsimple_async_result_t has taken its own reference.
 *       xobject_unref (cake);
 *       return;
 *     }
 *
 *   _baker_prepare_cake (self, radius, baked_cb, simple);
 * }
 *
 * Cake *
 * baker_bake_cake_finish (Baker        *self,
 *                         xasync_result_t *result,
 *                         xerror_t      **error)
 * {
 *   xsimple_async_result_t *simple;
 *   Cake               *cake;
 *
 *   xreturn_val_if_fail (xsimple_async_result_is_valid (result,
 *                                                         G_OBJECT (self),
 *                                                         baker_bake_cake_async),
 *                         NULL);
 *
 *   simple = (xsimple_async_result_t *) result;
 *
 *   if (xsimple_async_result_propagate_error (simple, error))
 *     return NULL;
 *
 *   cake = CAKE (xsimple_async_result_get_op_res_gpointer (simple));
 *   return xobject_ref (cake);
 * }
 * ]|
 */

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void xsimple_async_result_async_result_iface_init (xasync_result_iface_t       *iface);

struct _GSimpleAsyncResult
{
  xobject_t parent_instance;

  xobject_t *source_object;
  xasync_ready_callback_t callback;
  xpointer_t user_data;
  xmain_context_t *context;
  xerror_t *error;
  xboolean_t failed;
  xboolean_t handle_cancellation;
  xcancellable_t *check_cancellable;

  xpointer_t source_tag;

  union {
    xpointer_t v_pointer;
    xboolean_t v_boolean;
    xssize_t   v_ssize;
  } op_res;

  xdestroy_notify_t destroy_op_res;
};

struct _GSimpleAsyncResultClass
{
  xobject_class_t parent_class;
};


G_DEFINE_TYPE_WITH_CODE (xsimple_async_result_t, xsimple_async_result, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_ASYNC_RESULT,
						xsimple_async_result_async_result_iface_init))

static void
clear_op_res (xsimple_async_result_t *simple)
{
  if (simple->destroy_op_res)
    simple->destroy_op_res (simple->op_res.v_pointer);
  simple->destroy_op_res = NULL;
  simple->op_res.v_ssize = 0;
}

static void
xsimple_async_result_finalize (xobject_t *object)
{
  xsimple_async_result_t *simple;

  simple = XSIMPLE_ASYNC_RESULT (object);

  if (simple->source_object)
    xobject_unref (simple->source_object);

  if (simple->check_cancellable)
    xobject_unref (simple->check_cancellable);

  xmain_context_unref (simple->context);

  clear_op_res (simple);

  if (simple->error)
    xerror_free (simple->error);

  XOBJECT_CLASS (xsimple_async_result_parent_class)->finalize (object);
}

static void
xsimple_async_result_class_init (xsimple_async_result_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = xsimple_async_result_finalize;
}

static void
xsimple_async_result_init (xsimple_async_result_t *simple)
{
  simple->handle_cancellation = TRUE;

  simple->context = xmain_context_ref_thread_default ();
}

/**
 * xsimple_async_result_new:
 * @source_object: (nullable): a #xobject_t, or %NULL.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @user_data: (closure): user data passed to @callback.
 * @source_tag: the asynchronous function.
 *
 * Creates a #xsimple_async_result_t.
 *
 * The common convention is to create the #xsimple_async_result_t in the
 * function that starts the asynchronous operation and use that same
 * function as the @source_tag.
 *
 * If your operation supports cancellation with #xcancellable_t (which it
 * probably should) then you should provide the user's cancellable to
 * xsimple_async_result_set_check_cancellable() immediately after
 * this function returns.
 *
 * Returns: a #xsimple_async_result_t.
 *
 * Deprecated: 2.46: Use xtask_new() instead.
 **/
xsimple_async_result_t *
xsimple_async_result_new (xobject_t             *source_object,
                           xasync_ready_callback_t  callback,
                           xpointer_t             user_data,
                           xpointer_t             source_tag)
{
  xsimple_async_result_t *simple;

  xreturn_val_if_fail (!source_object || X_IS_OBJECT (source_object), NULL);

  simple = xobject_new (XTYPE_SIMPLE_ASYNC_RESULT, NULL);
  simple->callback = callback;
  if (source_object)
    simple->source_object = xobject_ref (source_object);
  else
    simple->source_object = NULL;
  simple->user_data = user_data;
  simple->source_tag = source_tag;

  return simple;
}

/**
 * xsimple_async_result_new_from_error:
 * @source_object: (nullable): a #xobject_t, or %NULL.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @user_data: (closure): user data passed to @callback.
 * @error: a #xerror_t
 *
 * Creates a #xsimple_async_result_t from an error condition.
 *
 * Returns: a #xsimple_async_result_t.
 *
 * Deprecated: 2.46: Use xtask_new() and xtask_return_error() instead.
 **/
xsimple_async_result_t *
xsimple_async_result_new_from_error (xobject_t             *source_object,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data,
                                      const xerror_t        *error)
{
  xsimple_async_result_t *simple;

  xreturn_val_if_fail (!source_object || X_IS_OBJECT (source_object), NULL);

  simple = xsimple_async_result_new (source_object,
				      callback,
				      user_data, NULL);
  xsimple_async_result_set_from_error (simple, error);

  return simple;
}

/**
 * xsimple_async_result_new_take_error: (skip)
 * @source_object: (nullable): a #xobject_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): user data passed to @callback
 * @error: a #xerror_t
 *
 * Creates a #xsimple_async_result_t from an error condition, and takes over the
 * caller's ownership of @error, so the caller does not need to free it anymore.
 *
 * Returns: a #xsimple_async_result_t
 *
 * Since: 2.28
 *
 * Deprecated: 2.46: Use xtask_new() and xtask_return_error() instead.
 **/
xsimple_async_result_t *
xsimple_async_result_new_take_error (xobject_t             *source_object,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data,
                                      xerror_t              *error)
{
  xsimple_async_result_t *simple;

  xreturn_val_if_fail (!source_object || X_IS_OBJECT (source_object), NULL);

  simple = xsimple_async_result_new (source_object,
				      callback,
				      user_data, NULL);
  xsimple_async_result_take_error (simple, error);

  return simple;
}

/**
 * xsimple_async_result_new_error:
 * @source_object: (nullable): a #xobject_t, or %NULL.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @user_data: (closure): user data passed to @callback.
 * @domain: a #xquark.
 * @code: an error code.
 * @format: a string with format characters.
 * @...: a list of values to insert into @format.
 *
 * Creates a new #xsimple_async_result_t with a set error.
 *
 * Returns: a #xsimple_async_result_t.
 *
 * Deprecated: 2.46: Use xtask_new() and xtask_return_new_error() instead.
 **/
xsimple_async_result_t *
xsimple_async_result_new_error (xobject_t             *source_object,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data,
                                 xquark               domain,
                                 xint_t                 code,
                                 const char          *format,
                                 ...)
{
  xsimple_async_result_t *simple;
  va_list args;

  xreturn_val_if_fail (!source_object || X_IS_OBJECT (source_object), NULL);
  xreturn_val_if_fail (domain != 0, NULL);
  xreturn_val_if_fail (format != NULL, NULL);

  simple = xsimple_async_result_new (source_object,
				      callback,
				      user_data, NULL);

  va_start (args, format);
  xsimple_async_result_set_error_va (simple, domain, code, format, args);
  va_end (args);

  return simple;
}


static xpointer_t
xsimple_async_result_get_user_data (xasync_result_t *res)
{
  return XSIMPLE_ASYNC_RESULT (res)->user_data;
}

static xobject_t *
xsimple_async_result_get_source_object (xasync_result_t *res)
{
  if (XSIMPLE_ASYNC_RESULT (res)->source_object)
    return xobject_ref (XSIMPLE_ASYNC_RESULT (res)->source_object);
  return NULL;
}

static xboolean_t
xsimple_async_result_is_tagged (xasync_result_t *res,
				 xpointer_t      source_tag)
{
  return XSIMPLE_ASYNC_RESULT (res)->source_tag == source_tag;
}

static void
xsimple_async_result_async_result_iface_init (xasync_result_iface_t *iface)
{
  iface->get_user_data = xsimple_async_result_get_user_data;
  iface->get_source_object = xsimple_async_result_get_source_object;
  iface->is_tagged = xsimple_async_result_is_tagged;
}

/**
 * xsimple_async_result_set_handle_cancellation:
 * @simple: a #xsimple_async_result_t.
 * @handle_cancellation: a #xboolean_t.
 *
 * Sets whether to handle cancellation within the asynchronous operation.
 *
 * This function has nothing to do with
 * xsimple_async_result_set_check_cancellable().  It only refers to the
 * #xcancellable_t passed to xsimple_async_result_run_in_thread().
 *
 * Deprecated: 2.46
 **/
void
xsimple_async_result_set_handle_cancellation (xsimple_async_result_t *simple,
                                               xboolean_t            handle_cancellation)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  simple->handle_cancellation = handle_cancellation;
}

/**
 * xsimple_async_result_get_source_tag: (skip)
 * @simple: a #xsimple_async_result_t.
 *
 * Gets the source tag for the #xsimple_async_result_t.
 *
 * Returns: a #xpointer_t to the source object for the #xsimple_async_result_t.
 *
 * Deprecated: 2.46. Use #xtask_t and xtask_get_source_tag() instead.
 **/
xpointer_t
xsimple_async_result_get_source_tag (xsimple_async_result_t *simple)
{
  xreturn_val_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple), NULL);
  return simple->source_tag;
}

/**
 * xsimple_async_result_propagate_error:
 * @simple: a #xsimple_async_result_t.
 * @dest: (out): a location to propagate the error to.
 *
 * Propagates an error from within the simple asynchronous result to
 * a given destination.
 *
 * If the #xcancellable_t given to a prior call to
 * xsimple_async_result_set_check_cancellable() is cancelled then this
 * function will return %TRUE with @dest set appropriately.
 *
 * Returns: %TRUE if the error was propagated to @dest. %FALSE otherwise.
 *
 * Deprecated: 2.46: Use #xtask_t instead.
 **/
xboolean_t
xsimple_async_result_propagate_error (xsimple_async_result_t  *simple,
                                       xerror_t             **dest)
{
  xreturn_val_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple), FALSE);

  if (xcancellable_set_error_if_cancelled (simple->check_cancellable, dest))
    return TRUE;

  if (simple->failed)
    {
      g_propagate_error (dest, simple->error);
      simple->error = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * xsimple_async_result_set_op_res_gpointer: (skip)
 * @simple: a #xsimple_async_result_t.
 * @op_res: a pointer result from an asynchronous function.
 * @destroy_op_res: a #xdestroy_notify_t function.
 *
 * Sets the operation result within the asynchronous result to a pointer.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_pointer() instead.
 **/
void
xsimple_async_result_set_op_res_gpointer (xsimple_async_result_t *simple,
                                           xpointer_t            op_res,
                                           xdestroy_notify_t      destroy_op_res)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));

  clear_op_res (simple);
  simple->op_res.v_pointer = op_res;
  simple->destroy_op_res = destroy_op_res;
}

/**
 * xsimple_async_result_get_op_res_gpointer: (skip)
 * @simple: a #xsimple_async_result_t.
 *
 * Gets a pointer result as returned by the asynchronous function.
 *
 * Returns: a pointer from the result.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_propagate_pointer() instead.
 **/
xpointer_t
xsimple_async_result_get_op_res_gpointer (xsimple_async_result_t *simple)
{
  xreturn_val_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple), NULL);
  return simple->op_res.v_pointer;
}

/**
 * xsimple_async_result_set_op_res_gssize:
 * @simple: a #xsimple_async_result_t.
 * @op_res: a #xssize_t.
 *
 * Sets the operation result within the asynchronous result to
 * the given @op_res.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_int() instead.
 **/
void
xsimple_async_result_set_op_res_gssize (xsimple_async_result_t *simple,
                                         xssize_t              op_res)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  clear_op_res (simple);
  simple->op_res.v_ssize = op_res;
}

/**
 * xsimple_async_result_get_op_res_gssize:
 * @simple: a #xsimple_async_result_t.
 *
 * Gets a xssize_t from the asynchronous result.
 *
 * Returns: a xssize_t returned from the asynchronous function.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_propagate_int() instead.
 **/
xssize_t
xsimple_async_result_get_op_res_gssize (xsimple_async_result_t *simple)
{
  xreturn_val_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple), 0);
  return simple->op_res.v_ssize;
}

/**
 * xsimple_async_result_set_op_res_gboolean:
 * @simple: a #xsimple_async_result_t.
 * @op_res: a #xboolean_t.
 *
 * Sets the operation result to a boolean within the asynchronous result.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_boolean() instead.
 **/
void
xsimple_async_result_set_op_res_gboolean (xsimple_async_result_t *simple,
                                           xboolean_t            op_res)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  clear_op_res (simple);
  simple->op_res.v_boolean = !!op_res;
}

/**
 * xsimple_async_result_get_op_res_gboolean:
 * @simple: a #xsimple_async_result_t.
 *
 * Gets the operation result boolean from within the asynchronous result.
 *
 * Returns: %TRUE if the operation's result was %TRUE, %FALSE
 *     if the operation's result was %FALSE.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_propagate_boolean() instead.
 **/
xboolean_t
xsimple_async_result_get_op_res_gboolean (xsimple_async_result_t *simple)
{
  xreturn_val_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple), FALSE);
  return simple->op_res.v_boolean;
}

/**
 * xsimple_async_result_set_from_error:
 * @simple: a #xsimple_async_result_t.
 * @error: #xerror_t.
 *
 * Sets the result from a #xerror_t.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_error() instead.
 **/
void
xsimple_async_result_set_from_error (xsimple_async_result_t *simple,
                                      const xerror_t       *error)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  g_return_if_fail (error != NULL);

  if (simple->error)
    xerror_free (simple->error);
  simple->error = xerror_copy (error);
  simple->failed = TRUE;
}

/**
 * xsimple_async_result_take_error: (skip)
 * @simple: a #xsimple_async_result_t
 * @error: a #xerror_t
 *
 * Sets the result from @error, and takes over the caller's ownership
 * of @error, so the caller does not need to free it any more.
 *
 * Since: 2.28
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_error() instead.
 **/
void
xsimple_async_result_take_error (xsimple_async_result_t *simple,
                                  xerror_t             *error)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  g_return_if_fail (error != NULL);

  if (simple->error)
    xerror_free (simple->error);
  simple->error = error;
  simple->failed = TRUE;
}

/**
 * xsimple_async_result_set_error_va: (skip)
 * @simple: a #xsimple_async_result_t.
 * @domain: a #xquark (usually %G_IO_ERROR).
 * @code: an error code.
 * @format: a formatted error reporting string.
 * @args: va_list of arguments.
 *
 * Sets an error within the asynchronous result without a #xerror_t.
 * Unless writing a binding, see xsimple_async_result_set_error().
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_error() instead.
 **/
void
xsimple_async_result_set_error_va (xsimple_async_result_t *simple,
                                    xquark              domain,
                                    xint_t                code,
                                    const char         *format,
                                    va_list             args)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  g_return_if_fail (domain != 0);
  g_return_if_fail (format != NULL);

  if (simple->error)
    xerror_free (simple->error);
  simple->error = xerror_new_valist (domain, code, format, args);
  simple->failed = TRUE;
}

/**
 * xsimple_async_result_set_error: (skip)
 * @simple: a #xsimple_async_result_t.
 * @domain: a #xquark (usually %G_IO_ERROR).
 * @code: an error code.
 * @format: a formatted error reporting string.
 * @...: a list of variables to fill in @format.
 *
 * Sets an error within the asynchronous result without a #xerror_t.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_return_new_error() instead.
 **/
void
xsimple_async_result_set_error (xsimple_async_result_t *simple,
                                 xquark              domain,
                                 xint_t                code,
                                 const char         *format,
                                 ...)
{
  va_list args;

  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  g_return_if_fail (domain != 0);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  xsimple_async_result_set_error_va (simple, domain, code, format, args);
  va_end (args);
}

/**
 * xsimple_async_result_complete:
 * @simple: a #xsimple_async_result_t.
 *
 * Completes an asynchronous I/O job immediately. Must be called in
 * the thread where the asynchronous result was to be delivered, as it
 * invokes the callback directly. If you are in a different thread use
 * xsimple_async_result_complete_in_idle().
 *
 * Calling this function takes a reference to @simple for as long as
 * is needed to complete the call.
 *
 * Deprecated: 2.46: Use #xtask_t instead.
 **/
void
xsimple_async_result_complete (xsimple_async_result_t *simple)
{
#ifndef G_DISABLE_CHECKS
  xsource_t *current_source;
  xmain_context_t *current_context;
#endif

  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));

#ifndef G_DISABLE_CHECKS
  current_source = g_main_current_source ();
  if (current_source && !xsource_is_destroyed (current_source))
    {
      current_context = xsource_get_context (current_source);
      if (simple->context != current_context)
	g_warning ("xsimple_async_result_complete() called from wrong context!");
    }
#endif

  if (simple->callback)
    {
      xmain_context_push_thread_default (simple->context);
      simple->callback (simple->source_object,
			G_ASYNC_RESULT (simple),
			simple->user_data);
      xmain_context_pop_thread_default (simple->context);
    }
}

static xboolean_t
complete_in_idle_cb (xpointer_t data)
{
  xsimple_async_result_t *simple = XSIMPLE_ASYNC_RESULT (data);

  xsimple_async_result_complete (simple);

  return FALSE;
}

/**
 * xsimple_async_result_complete_in_idle:
 * @simple: a #xsimple_async_result_t.
 *
 * Completes an asynchronous function in an idle handler in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread that @simple was initially created in
 * (and re-pushes that context around the invocation of the callback).
 *
 * Calling this function takes a reference to @simple for as long as
 * is needed to complete the call.
 *
 * Deprecated: 2.46: Use #xtask_t instead.
 */
void
xsimple_async_result_complete_in_idle (xsimple_async_result_t *simple)
{
  xsource_t *source;

  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));

  xobject_ref (simple);

  source = g_idle_source_new ();
  xsource_set_priority (source, G_PRIORITY_DEFAULT);
  xsource_set_callback (source, complete_in_idle_cb, simple, xobject_unref);
  xsource_set_static_name (source, "[gio] complete_in_idle_cb");

  xsource_attach (source, simple->context);
  xsource_unref (source);
}

typedef struct {
  xsimple_async_result_t *simple;
  xcancellable_t *cancellable;
  xsimple_async_thread_func_t func;
} RunInThreadData;


static xboolean_t
complete_in_idle_cb_for_thread (xpointer_t _data)
{
  RunInThreadData *data = _data;
  xsimple_async_result_t *simple;

  simple = data->simple;

  if (simple->handle_cancellation &&
      xcancellable_is_cancelled (data->cancellable))
    xsimple_async_result_set_error (simple,
                                     G_IO_ERROR,
                                     G_IO_ERROR_CANCELLED,
                                     "%s", _("Operation was cancelled"));

  xsimple_async_result_complete (simple);

  if (data->cancellable)
    xobject_unref (data->cancellable);
  xobject_unref (data->simple);
  g_free (data);

  return FALSE;
}

static xboolean_t
run_in_thread (xio_scheduler_job_t *job,
               xcancellable_t    *c,
               xpointer_t         _data)
{
  RunInThreadData *data = _data;
  xsimple_async_result_t *simple = data->simple;
  xsource_t *source;

  if (simple->handle_cancellation &&
      xcancellable_is_cancelled (c))
    xsimple_async_result_set_error (simple,
                                     G_IO_ERROR,
                                     G_IO_ERROR_CANCELLED,
                                     "%s", _("Operation was cancelled"));
  else
    data->func (simple,
                simple->source_object,
                c);

  source = g_idle_source_new ();
  xsource_set_priority (source, G_PRIORITY_DEFAULT);
  xsource_set_callback (source, complete_in_idle_cb_for_thread, data, NULL);
  xsource_set_static_name (source, "[gio] complete_in_idle_cb_for_thread");

  xsource_attach (source, simple->context);
  xsource_unref (source);

  return FALSE;
}

/**
 * xsimple_async_result_run_in_thread: (skip)
 * @simple: a #xsimple_async_result_t.
 * @func: a #xsimple_async_thread_func_t.
 * @io_priority: the io priority of the request.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 *
 * Runs the asynchronous job in a separate thread and then calls
 * xsimple_async_result_complete_in_idle() on @simple to return
 * the result to the appropriate main loop.
 *
 * Calling this function takes a reference to @simple for as long as
 * is needed to run the job and report its completion.
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_run_in_thread() instead.
 */
void
xsimple_async_result_run_in_thread (xsimple_async_result_t     *simple,
                                     xsimple_async_thread_func_t  func,
                                     int                     io_priority,
                                     xcancellable_t           *cancellable)
{
  RunInThreadData *data;

  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  g_return_if_fail (func != NULL);

  data = g_new (RunInThreadData, 1);
  data->func = func;
  data->simple = xobject_ref (simple);
  data->cancellable = cancellable;
  if (cancellable)
    xobject_ref (cancellable);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  g_io_scheduler_push_job (run_in_thread, data, NULL, io_priority, cancellable);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

/**
 * xsimple_async_result_is_valid:
 * @result: the #xasync_result_t passed to the _finish function.
 * @source: (nullable): the #xobject_t passed to the _finish function.
 * @source_tag: (nullable): the asynchronous function.
 *
 * Ensures that the data passed to the _finish function of an async
 * operation is consistent.  Three checks are performed.
 *
 * First, @result is checked to ensure that it is really a
 * #xsimple_async_result_t.  Second, @source is checked to ensure that it
 * matches the source object of @result.  Third, @source_tag is
 * checked to ensure that it is equal to the @source_tag argument given
 * to xsimple_async_result_new() (which, by convention, is a pointer
 * to the _async function corresponding to the _finish function from
 * which this function is called).  (Alternatively, if either
 * @source_tag or @result's source tag is %NULL, then the source tag
 * check is skipped.)
 *
 * Returns: #TRUE if all checks passed or #FALSE if any failed.
 *
 * Since: 2.20
 *
 * Deprecated: 2.46: Use #xtask_t and xtask_is_valid() instead.
 **/
xboolean_t
xsimple_async_result_is_valid (xasync_result_t *result,
                                xobject_t      *source,
                                xpointer_t      source_tag)
{
  xsimple_async_result_t *simple;
  xobject_t *cmp_source;
  xpointer_t result_source_tag;

  if (!X_IS_SIMPLE_ASYNC_RESULT (result))
    return FALSE;
  simple = (xsimple_async_result_t *)result;

  cmp_source = xasync_result_get_source_object (result);
  if (cmp_source != source)
    {
      if (cmp_source != NULL)
        xobject_unref (cmp_source);
      return FALSE;
    }
  if (cmp_source != NULL)
    xobject_unref (cmp_source);

  result_source_tag = xsimple_async_result_get_source_tag (simple);
  return source_tag == NULL || result_source_tag == NULL ||
         source_tag == result_source_tag;
}

/**
 * g_simple_async_report_error_in_idle: (skip)
 * @object: (nullable): a #xobject_t, or %NULL.
 * @callback: a #xasync_ready_callback_t.
 * @user_data: user data passed to @callback.
 * @domain: a #xquark containing the error domain (usually %G_IO_ERROR).
 * @code: a specific error code.
 * @format: a formatted error reporting string.
 * @...: a list of variables to fill in @format.
 *
 * Reports an error in an asynchronous function in an idle function by
 * directly setting the contents of the #xasync_result_t with the given error
 * information.
 *
 * Deprecated: 2.46: Use xtask_report_error().
 **/
void
g_simple_async_report_error_in_idle (xobject_t             *object,
                                     xasync_ready_callback_t  callback,
                                     xpointer_t             user_data,
                                     xquark               domain,
                                     xint_t                 code,
                                     const char          *format,
                                     ...)
{
  xsimple_async_result_t *simple;
  va_list args;

  g_return_if_fail (!object || X_IS_OBJECT (object));
  g_return_if_fail (domain != 0);
  g_return_if_fail (format != NULL);

  simple = xsimple_async_result_new (object,
				      callback,
				      user_data, NULL);

  va_start (args, format);
  xsimple_async_result_set_error_va (simple, domain, code, format, args);
  va_end (args);
  xsimple_async_result_complete_in_idle (simple);
  xobject_unref (simple);
}

/**
 * g_simple_async_report_gerror_in_idle:
 * @object: (nullable): a #xobject_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @user_data: (closure): user data passed to @callback.
 * @error: the #xerror_t to report
 *
 * Reports an error in an idle function. Similar to
 * g_simple_async_report_error_in_idle(), but takes a #xerror_t rather
 * than building a new one.
 *
 * Deprecated: 2.46: Use xtask_report_error().
 **/
void
g_simple_async_report_gerror_in_idle (xobject_t *object,
				      xasync_ready_callback_t callback,
				      xpointer_t user_data,
				      const xerror_t *error)
{
  xsimple_async_result_t *simple;

  g_return_if_fail (!object || X_IS_OBJECT (object));
  g_return_if_fail (error != NULL);

  simple = xsimple_async_result_new_from_error (object,
						 callback,
						 user_data,
						 error);
  xsimple_async_result_complete_in_idle (simple);
  xobject_unref (simple);
}

/**
 * g_simple_async_report_take_gerror_in_idle: (skip)
 * @object: (nullable): a #xobject_t, or %NULL
 * @callback: a #xasync_ready_callback_t.
 * @user_data: user data passed to @callback.
 * @error: the #xerror_t to report
 *
 * Reports an error in an idle function. Similar to
 * g_simple_async_report_gerror_in_idle(), but takes over the caller's
 * ownership of @error, so the caller does not have to free it any more.
 *
 * Since: 2.28
 *
 * Deprecated: 2.46: Use xtask_report_error().
 **/
void
g_simple_async_report_take_gerror_in_idle (xobject_t *object,
                                           xasync_ready_callback_t callback,
                                           xpointer_t user_data,
                                           xerror_t *error)
{
  xsimple_async_result_t *simple;

  g_return_if_fail (!object || X_IS_OBJECT (object));
  g_return_if_fail (error != NULL);

  simple = xsimple_async_result_new_take_error (object,
                                                 callback,
                                                 user_data,
                                                 error);
  xsimple_async_result_complete_in_idle (simple);
  xobject_unref (simple);
}

/**
 * xsimple_async_result_set_check_cancellable:
 * @simple: a #xsimple_async_result_t
 * @check_cancellable: (nullable): a #xcancellable_t to check, or %NULL to unset
 *
 * Sets a #xcancellable_t to check before dispatching results.
 *
 * This function has one very specific purpose: the provided cancellable
 * is checked at the time of xsimple_async_result_propagate_error() If
 * it is cancelled, these functions will return an "Operation was
 * cancelled" error (%G_IO_ERROR_CANCELLED).
 *
 * Implementors of cancellable asynchronous functions should use this in
 * order to provide a guarantee to their callers that cancelling an
 * async operation will reliably result in an error being returned for
 * that operation (even if a positive result for the operation has
 * already been sent as an idle to the main context to be dispatched).
 *
 * The checking described above is done regardless of any call to the
 * unrelated xsimple_async_result_set_handle_cancellation() function.
 *
 * Since: 2.32
 *
 * Deprecated: 2.46: Use #xtask_t instead.
 **/
void
xsimple_async_result_set_check_cancellable (xsimple_async_result_t *simple,
                                             xcancellable_t *check_cancellable)
{
  g_return_if_fail (X_IS_SIMPLE_ASYNC_RESULT (simple));
  g_return_if_fail (check_cancellable == NULL || X_IS_CANCELLABLE (check_cancellable));

  g_clear_object (&simple->check_cancellable);
  if (check_cancellable)
    simple->check_cancellable = xobject_ref (check_cancellable);
}

G_GNUC_END_IGNORE_DEPRECATIONS
