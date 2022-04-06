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
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "glibintl.h"


/**
 * SECTION:gasyncresult
 * @short_description: Asynchronous Function Results
 * @include: gio/gio.h
 * @see_also: #xtask_t
 *
 * Provides a base class for implementing asynchronous function results.
 *
 * Asynchronous operations are broken up into two separate operations
 * which are chained together by a #xasync_ready_callback_t. To begin
 * an asynchronous operation, provide a #xasync_ready_callback_t to the
 * asynchronous function. This callback will be triggered when the
 * operation has completed, and must be run in a later iteration of
 * the [thread-default main context][g-main-context-push-thread-default]
 * from where the operation was initiated. It will be passed a
 * #xasync_result_t instance filled with the details of the operation's
 * success or failure, the object the asynchronous function was
 * started for and any error codes returned. The asynchronous callback
 * function is then expected to call the corresponding "_finish()"
 * function, passing the object the function was called for, the
 * #xasync_result_t instance, and (optionally) an @error to grab any
 * error conditions that may have occurred.
 *
 * The "_finish()" function for an operation takes the generic result
 * (of type #xasync_result_t) and returns the specific result that the
 * operation in question yields (e.g. a #xfile_enumerator_t for a
 * "enumerate children" operation). If the result or error status of the
 * operation is not needed, there is no need to call the "_finish()"
 * function; GIO will take care of cleaning up the result and error
 * information after the #xasync_ready_callback_t returns. You can pass
 * %NULL for the #xasync_ready_callback_t if you don't need to take any
 * action at all after the operation completes. Applications may also
 * take a reference to the #xasync_result_t and call "_finish()" later;
 * however, the "_finish()" function may be called at most once.
 *
 * Example of a typical asynchronous operation flow:
 * |[<!-- language="C" -->
 * void _theoretical_frobnitz_async (Theoretical         *t,
 *                                   xcancellable_t        *c,
 *                                   xasync_ready_callback_t  cb,
 *                                   xpointer_t             u);
 *
 * xboolean_t _theoretical_frobnitz_finish (Theoretical   *t,
 *                                        xasync_result_t  *res,
 *                                        xerror_t       **e);
 *
 * static void
 * frobnitz_result_func (xobject_t      *source_object,
 *			 xasync_result_t *res,
 *			 xpointer_t      user_data)
 * {
 *   xboolean_t success = FALSE;
 *
 *   success = _theoretical_frobnitz_finish (source_object, res, NULL);
 *
 *   if (success)
 *     g_printf ("Hurray!\n");
 *   else
 *     g_printf ("Uh oh!\n");
 *
 *   ...
 *
 * }
 *
 * int main (int argc, void *argv[])
 * {
 *    ...
 *
 *    _theoretical_frobnitz_async (theoretical_data,
 *                                 NULL,
 *                                 frobnitz_result_func,
 *                                 NULL);
 *
 *    ...
 * }
 * ]|
 *
 * The callback for an asynchronous operation is called only once, and is
 * always called, even in the case of a cancelled operation. On cancellation
 * the result is a %G_IO_ERROR_CANCELLED error.
 *
 * ## I/O Priority # {#io-priority}
 *
 * Many I/O-related asynchronous operations have a priority parameter,
 * which is used in certain cases to determine the order in which
 * operations are executed. They are not used to determine system-wide
 * I/O scheduling. Priorities are integers, with lower numbers indicating
 * higher priority. It is recommended to choose priorities between
 * %G_PRIORITY_LOW and %G_PRIORITY_HIGH, with %G_PRIORITY_DEFAULT
 * as a default.
 */

typedef xasync_result_iface_t xasync_result_interface_t;
G_DEFINE_INTERFACE (xasync_result, xasync_result, XTYPE_OBJECT)

static void
xasync_result_default_init (xasync_result_interface_t *iface)
{
}

/**
 * xasync_result_get_user_data:
 * @res: a #xasync_result_t.
 *
 * Gets the user data from a #xasync_result_t.
 *
 * Returns: (transfer full): the user data for @res.
 **/
xpointer_t
xasync_result_get_user_data (xasync_result_t *res)
{
  xasync_result_iface_t *iface;

  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  iface = G_ASYNC_RESULT_GET_IFACE (res);

  return (* iface->get_user_data) (res);
}

/**
 * xasync_result_get_source_object:
 * @res: a #xasync_result_t
 *
 * Gets the source object from a #xasync_result_t.
 *
 * Returns: (transfer full) (nullable): a new reference to the source
 *    object for the @res, or %NULL if there is none.
 */
xobject_t *
xasync_result_get_source_object (xasync_result_t *res)
{
  xasync_result_iface_t *iface;

  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  iface = G_ASYNC_RESULT_GET_IFACE (res);

  return (* iface->get_source_object) (res);
}

/**
 * xasync_result_legacy_propagate_error:
 * @res: a #xasync_result_t
 * @error: (out): a location to propagate the error to.
 *
 * If @res is a #xsimple_async_result_t, this is equivalent to
 * xsimple_async_result_propagate_error(). Otherwise it returns
 * %FALSE.
 *
 * This can be used for legacy error handling in async *_finish()
 * wrapper functions that traditionally handled #xsimple_async_result_t
 * error returns themselves rather than calling into the virtual method.
 * This should not be used in new code; #xasync_result_t errors that are
 * set by virtual methods should also be extracted by virtual methods,
 * to enable subclasses to chain up correctly.
 *
 * Returns: %TRUE if @error is has been filled in with an error from
 *   @res, %FALSE if not.
 *
 * Since: 2.34
 **/
xboolean_t
xasync_result_legacy_propagate_error (xasync_result_t  *res,
				       xerror_t       **error)
{
  /* This doesn't use a vmethod, because it's only for code that used
   * to use xsimple_async_result_t. (But it's a xasync_result_t method so
   * that callers don't need to worry about xsimple_async_result_t
   * deprecation warnings in the future.)
   */

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (X_IS_SIMPLE_ASYNC_RESULT (res))
    {
      return xsimple_async_result_propagate_error (XSIMPLE_ASYNC_RESULT (res),
						    error);
    }
  else
    return FALSE;
  G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * xasync_result_is_tagged:
 * @res: a #xasync_result_t
 * @source_tag: an application-defined tag
 *
 * Checks if @res has the given @source_tag (generally a function
 * pointer indicating the function @res was created by).
 *
 * Returns: %TRUE if @res has the indicated @source_tag, %FALSE if
 *   not.
 *
 * Since: 2.34
 **/
xboolean_t
xasync_result_is_tagged (xasync_result_t  *res,
			  xpointer_t       source_tag)
{
  xasync_result_iface_t *iface;

  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), FALSE);

  iface = G_ASYNC_RESULT_GET_IFACE (res);

  if (!iface->is_tagged)
    return FALSE;

  return (* iface->is_tagged) (res, source_tag);
}
