/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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
#include "gasyncinitable.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "gtask.h"
#include "glibintl.h"


/**
 * SECTION:gasyncinitable
 * @short_description: Asynchronously failable object initialization interface
 * @include: gio/gio.h
 * @see_also: #xinitable_t
 *
 * This is the asynchronous version of #xinitable_t; it behaves the same
 * in all ways except that initialization is asynchronous. For more details
 * see the descriptions on #xinitable_t.
 *
 * A class may implement both the #xinitable_t and #xasync_initable_t interfaces.
 *
 * Users of objects implementing this are not intended to use the interface
 * method directly; instead it will be used automatically in various ways.
 * For C applications you generally just call xasync_initable_new_async()
 * directly, or indirectly via a foo_thing_new_async() wrapper. This will call
 * xasync_initable_init_async() under the cover, calling back with %NULL and
 * a set %xerror_t on failure.
 *
 * A typical implementation might look something like this:
 *
 * |[<!-- language="C" -->
 * enum {
 *    NOT_INITIALIZED,
 *    INITIALIZING,
 *    INITIALIZED
 * };
 *
 * static void
 * _foo_ready_cb (foo_t *self)
 * {
 *   xlist_t *l;
 *
 *   self->priv->state = INITIALIZED;
 *
 *   for (l = self->priv->init_results; l != NULL; l = l->next)
 *     {
 *       xtask_t *task = l->data;
 *
 *       if (self->priv->success)
 *         xtask_return_boolean (task, TRUE);
 *       else
 *         xtask_return_new_error (task, ...);
 *       xobject_unref (task);
 *     }
 *
 *   xlist_free (self->priv->init_results);
 *   self->priv->init_results = NULL;
 * }
 *
 * static void
 * foo_init_async (xasync_initable_t       *initable,
 *                 int                   io_priority,
 *                 xcancellable_t         *cancellable,
 *                 xasync_ready_callback_t   callback,
 *                 xpointer_t              user_data)
 * {
 *   foo_t *self = FOO (initable);
 *   xtask_t *task;
 *
 *   task = xtask_new (initable, cancellable, callback, user_data);
 *   xtask_set_name (task, G_STRFUNC);
 *
 *   switch (self->priv->state)
 *     {
 *       case NOT_INITIALIZED:
 *         _foo_get_ready (self);
 *         self->priv->init_results = xlist_append (self->priv->init_results,
 *                                                   task);
 *         self->priv->state = INITIALIZING;
 *         break;
 *       case INITIALIZING:
 *         self->priv->init_results = xlist_append (self->priv->init_results,
 *                                                   task);
 *         break;
 *       case INITIALIZED:
 *         if (!self->priv->success)
 *           xtask_return_new_error (task, ...);
 *         else
 *           xtask_return_boolean (task, TRUE);
 *         xobject_unref (task);
 *         break;
 *     }
 * }
 *
 * static xboolean_t
 * foo_init_finish (xasync_initable_t       *initable,
 *                  xasync_result_t         *result,
 *                  xerror_t              **error)
 * {
 *   g_return_val_if_fail (xtask_is_valid (result, initable), FALSE);
 *
 *   return xtask_propagate_boolean (XTASK (result), error);
 * }
 *
 * static void
 * foo_async_initable_iface_init (xpointer_t x_iface,
 *                                xpointer_t data)
 * {
 *   xasync_initable_iface_t *iface = x_iface;
 *
 *   iface->init_async = foo_init_async;
 *   iface->init_finish = foo_init_finish;
 * }
 * ]|
 */

static void     xasync_initable_real_init_async  (xasync_initable_t       *initable,
						   int                   io_priority,
						   xcancellable_t         *cancellable,
						   xasync_ready_callback_t   callback,
						   xpointer_t              user_data);
static xboolean_t xasync_initable_real_init_finish (xasync_initable_t       *initable,
						   xasync_result_t         *res,
						   xerror_t              **error);


typedef xasync_initable_iface_t xasync_initable_interface_t;
G_DEFINE_INTERFACE (xasync_initable, xasync_initable, XTYPE_OBJECT)


static void
xasync_initable_default_init (xasync_initable_interface_t *iface)
{
  iface->init_async = xasync_initable_real_init_async;
  iface->init_finish = xasync_initable_real_init_finish;
}

/**
 * xasync_initable_init_async:
 * @initable: a #xasync_initable_t.
 * @io_priority: the [I/O priority][io-priority] of the operation
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Starts asynchronous initialization of the object implementing the
 * interface. This must be done before any real use of the object after
 * initial construction. If the object also implements #xinitable_t you can
 * optionally call xinitable_init() instead.
 *
 * This method is intended for language bindings. If writing in C,
 * xasync_initable_new_async() should typically be used instead.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call xasync_initable_init_finish() to get the result of the
 * initialization.
 *
 * Implementations may also support cancellation. If @cancellable is not
 * %NULL, then initialization can be cancelled by triggering the cancellable
 * object from another thread. If the operation was cancelled, the error
 * %G_IO_ERROR_CANCELLED will be returned. If @cancellable is not %NULL, and
 * the object doesn't support cancellable initialization, the error
 * %G_IO_ERROR_NOT_SUPPORTED will be returned.
 *
 * As with #xinitable_t, if the object is not initialized, or initialization
 * returns with an error, then all operations on the object except
 * xobject_ref() and xobject_unref() are considered to be invalid, and
 * have undefined behaviour. They will often fail with g_critical() or
 * g_warning(), but this must not be relied on.
 *
 * Callers should not assume that a class which implements #xasync_initable_t can
 * be initialized multiple times; for more information, see xinitable_init().
 * If a class explicitly supports being initialized multiple times,
 * implementation requires yielding all subsequent calls to init_async() on the
 * results of the first call.
 *
 * For classes that also support the #xinitable_t interface, the default
 * implementation of this method will run the xinitable_init() function
 * in a thread, so if you want to support asynchronous initialization via
 * threads, just implement the #xasync_initable_t interface without overriding
 * any interface methods.
 *
 * Since: 2.22
 */
void
xasync_initable_init_async (xasync_initable_t      *initable,
			     int                  io_priority,
			     xcancellable_t        *cancellable,
			     xasync_ready_callback_t  callback,
			     xpointer_t             user_data)
{
  xasync_initable_iface_t *iface;

  g_return_if_fail (X_IS_ASYNC_INITABLE (initable));

  iface = XASYNC_INITABLE_GET_IFACE (initable);

  (* iface->init_async) (initable, io_priority, cancellable, callback, user_data);
}

/**
 * xasync_initable_init_finish:
 * @initable: a #xasync_initable_t.
 * @res: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes asynchronous initialization and returns the result.
 * See xasync_initable_init_async().
 *
 * Returns: %TRUE if successful. If an error has occurred, this function
 * will return %FALSE and set @error appropriately if present.
 *
 * Since: 2.22
 */
xboolean_t
xasync_initable_init_finish (xasync_initable_t  *initable,
			      xasync_result_t    *res,
			      xerror_t         **error)
{
  xasync_initable_iface_t *iface;

  g_return_val_if_fail (X_IS_ASYNC_INITABLE (initable), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), FALSE);

  if (xasync_result_legacy_propagate_error (res, error))
    return FALSE;

  iface = XASYNC_INITABLE_GET_IFACE (initable);

  return (* iface->init_finish) (initable, res, error);
}

static void
async_init_thread (xtask_t        *task,
                   xpointer_t      source_object,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  xerror_t *error = NULL;

  if (xinitable_init (XINITABLE (source_object), cancellable, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xasync_initable_real_init_async (xasync_initable_t      *initable,
				  int                  io_priority,
				  xcancellable_t        *cancellable,
				  xasync_ready_callback_t  callback,
				  xpointer_t             user_data)
{
  xtask_t *task;

  g_return_if_fail (X_IS_INITABLE (initable));

  task = xtask_new (initable, cancellable, callback, user_data);
  xtask_set_source_tag (task, xasync_initable_real_init_async);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, async_init_thread);
  xobject_unref (task);
}

static xboolean_t
xasync_initable_real_init_finish (xasync_initable_t  *initable,
				   xasync_result_t    *res,
				   xerror_t         **error)
{
  /* For backward compatibility we have to process GSimpleAsyncResults
   * even though xasync_initable_real_init_async doesn't generate
   * them any more.
   */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (X_IS_SIMPLE_ASYNC_RESULT (res))
    {
      xsimple_async_result_t *simple = XSIMPLE_ASYNC_RESULT (res);
      if (xsimple_async_result_propagate_error (simple, error))
        return FALSE;
      else
        return TRUE;
    }
  G_GNUC_END_IGNORE_DEPRECATIONS

  g_return_val_if_fail (xtask_is_valid (res, initable), FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}

/**
 * xasync_initable_new_async:
 * @object_type: a #xtype_t supporting #xasync_initable_t.
 * @io_priority: the [I/O priority][io-priority] of the operation
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @callback: a #xasync_ready_callback_t to call when the initialization is
 *     finished
 * @user_data: the data to pass to callback function
 * @first_property_name: (nullable): the name of the first property, or %NULL if no
 *     properties
 * @...:  the value of the first property, followed by other property
 *    value pairs, and ended by %NULL.
 *
 * Helper function for constructing #xasync_initable_t object. This is
 * similar to xobject_new() but also initializes the object asynchronously.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call xasync_initable_new_finish() to get the new object and check
 * for any errors.
 *
 * Since: 2.22
 */
void
xasync_initable_new_async (xtype_t                object_type,
			    int                  io_priority,
			    xcancellable_t        *cancellable,
			    xasync_ready_callback_t  callback,
			    xpointer_t             user_data,
			    const xchar_t         *first_property_name,
			    ...)
{
  va_list var_args;

  va_start (var_args, first_property_name);
  xasync_initable_new_valist_async (object_type,
				     first_property_name, var_args,
				     io_priority, cancellable,
				     callback, user_data);
  va_end (var_args);
}

/**
 * xasync_initable_newv_async:
 * @object_type: a #xtype_t supporting #xasync_initable_t.
 * @n_parameters: the number of parameters in @parameters
 * @parameters: the parameters to use to construct the object
 * @io_priority: the [I/O priority][io-priority] of the operation
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @callback: a #xasync_ready_callback_t to call when the initialization is
 *     finished
 * @user_data: the data to pass to callback function
 *
 * Helper function for constructing #xasync_initable_t object. This is
 * similar to xobject_newv() but also initializes the object asynchronously.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call xasync_initable_new_finish() to get the new object and check
 * for any errors.
 *
 * Since: 2.22
 * Deprecated: 2.54: Use xobject_new_with_properties() and
 * xasync_initable_init_async() instead. See #GParameter for more information.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
xasync_initable_newv_async (xtype_t                object_type,
			     xuint_t                n_parameters,
			     GParameter          *parameters,
			     int                  io_priority,
			     xcancellable_t        *cancellable,
			     xasync_ready_callback_t  callback,
			     xpointer_t             user_data)
{
  xobject_t *obj;

  g_return_if_fail (XTYPE_IS_ASYNC_INITABLE (object_type));

  obj = xobject_newv (object_type, n_parameters, parameters);

  xasync_initable_init_async (XASYNC_INITABLE (obj),
			       io_priority, cancellable,
			       callback, user_data);
  xobject_unref (obj); /* Passed ownership to async call */
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xasync_initable_new_valist_async:
 * @object_type: a #xtype_t supporting #xasync_initable_t.
 * @first_property_name: the name of the first property, followed by
 * the value, and other property value pairs, and ended by %NULL.
 * @var_args: The var args list generated from @first_property_name.
 * @io_priority: the [I/O priority][io-priority] of the operation
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @callback: a #xasync_ready_callback_t to call when the initialization is
 *     finished
 * @user_data: the data to pass to callback function
 *
 * Helper function for constructing #xasync_initable_t object. This is
 * similar to xobject_new_valist() but also initializes the object
 * asynchronously.
 *
 * When the initialization is finished, @callback will be called. You can
 * then call xasync_initable_new_finish() to get the new object and check
 * for any errors.
 *
 * Since: 2.22
 */
void
xasync_initable_new_valist_async (xtype_t                object_type,
				   const xchar_t         *first_property_name,
				   va_list              var_args,
				   int                  io_priority,
				   xcancellable_t        *cancellable,
				   xasync_ready_callback_t  callback,
				   xpointer_t             user_data)
{
  xobject_t *obj;

  g_return_if_fail (XTYPE_IS_ASYNC_INITABLE (object_type));

  obj = xobject_new_valist (object_type,
			     first_property_name,
			     var_args);

  xasync_initable_init_async (XASYNC_INITABLE (obj),
			       io_priority, cancellable,
			       callback, user_data);
  xobject_unref (obj); /* Passed ownership to async call */
}

/**
 * xasync_initable_new_finish:
 * @initable: the #xasync_initable_t from the callback
 * @res: the #xasync_result_t from the callback
 * @error: return location for errors, or %NULL to ignore
 *
 * Finishes the async construction for the various xasync_initable_new
 * calls, returning the created object or %NULL on error.
 *
 * Returns: (type xobject_t.Object) (transfer full): a newly created #xobject_t,
 *      or %NULL on error. Free with xobject_unref().
 *
 * Since: 2.22
 */
xobject_t *
xasync_initable_new_finish (xasync_initable_t  *initable,
			     xasync_result_t    *res,
			     xerror_t         **error)
{
  if (xasync_initable_init_finish (initable, res, error))
    return xobject_ref (G_OBJECT (initable));
  else
    return NULL;
}
