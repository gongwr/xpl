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
#include "glib.h"
#include <gioerror.h>
#include "glib-private.h"
#include "gcancellable.h"
#include "glibintl.h"


/**
 * SECTION:gcancellable
 * @short_description: Thread-safe Operation Cancellation Stack
 * @include: gio/gio.h
 *
 * xcancellable_t is a thread-safe operation cancellation stack used
 * throughout GIO to allow for cancellation of synchronous and
 * asynchronous operations.
 */

enum {
  CANCELLED,
  LAST_SIGNAL
};

struct _GCancellablePrivate
{
  /* Atomic so that g_cancellable_is_cancelled does not require holding the mutex. */
  xboolean_t cancelled;
  /* Access to fields below is protected by cancellable_mutex. */
  xuint_t cancelled_running : 1;
  xuint_t cancelled_running_waiting : 1;

  xuint_t fd_refcount;
  GWakeup *wakeup;
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (xcancellable_t, g_cancellable, XTYPE_OBJECT)

static GPrivate current_cancellable;
static GMutex cancellable_mutex;
static GCond cancellable_cond;

static void
g_cancellable_finalize (xobject_t *object)
{
  xcancellable_t *cancellable = G_CANCELLABLE (object);

  if (cancellable->priv->wakeup)
    XPL_PRIVATE_CALL (g_wakeup_free) (cancellable->priv->wakeup);

  G_OBJECT_CLASS (g_cancellable_parent_class)->finalize (object);
}

static void
g_cancellable_class_init (GCancellableClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_cancellable_finalize;

  /**
   * xcancellable_t::cancelled:
   * @cancellable: a #xcancellable_t.
   *
   * Emitted when the operation has been cancelled.
   *
   * Can be used by implementations of cancellable operations. If the
   * operation is cancelled from another thread, the signal will be
   * emitted in the thread that cancelled the operation, not the
   * thread that is running the operation.
   *
   * Note that disconnecting from this signal (or any signal) in a
   * multi-threaded program is prone to race conditions. For instance
   * it is possible that a signal handler may be invoked even after
   * a call to g_signal_handler_disconnect() for that handler has
   * already returned.
   *
   * There is also a problem when cancellation happens right before
   * connecting to the signal. If this happens the signal will
   * unexpectedly not be emitted, and checking before connecting to
   * the signal leaves a race condition where this is still happening.
   *
   * In order to make it safe and easy to connect handlers there
   * are two helper functions: g_cancellable_connect() and
   * g_cancellable_disconnect() which protect against problems
   * like this.
   *
   * An example of how to us this:
   * |[<!-- language="C" -->
   *     // Make sure we don't do unnecessary work if already cancelled
   *     if (g_cancellable_set_error_if_cancelled (cancellable, error))
   *       return;
   *
   *     // Set up all the data needed to be able to handle cancellation
   *     // of the operation
   *     my_data = my_data_new (...);
   *
   *     id = 0;
   *     if (cancellable)
   *       id = g_cancellable_connect (cancellable,
   *     			      G_CALLBACK (cancelled_handler)
   *     			      data, NULL);
   *
   *     // cancellable operation here...
   *
   *     g_cancellable_disconnect (cancellable, id);
   *
   *     // cancelled_handler is never called after this, it is now safe
   *     // to free the data
   *     my_data_free (my_data);
   * ]|
   *
   * Note that the cancelled signal is emitted in the thread that
   * the user cancelled from, which may be the main thread. So, the
   * cancellable signal should not do something that can block.
   */
  signals[CANCELLED] =
    g_signal_new (I_("cancelled"),
		  XTYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GCancellableClass, cancelled),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 0);

}

static void
g_cancellable_init (xcancellable_t *cancellable)
{
  cancellable->priv = g_cancellable_get_instance_private (cancellable);
}

/**
 * g_cancellable_new:
 *
 * Creates a new #xcancellable_t object.
 *
 * Applications that want to start one or more operations
 * that should be cancellable should create a #xcancellable_t
 * and pass it to the operations.
 *
 * One #xcancellable_t can be used in multiple consecutive
 * operations or in multiple concurrent operations.
 *
 * Returns: a #xcancellable_t.
 **/
xcancellable_t *
g_cancellable_new (void)
{
  return g_object_new (XTYPE_CANCELLABLE, NULL);
}

/**
 * g_cancellable_push_current:
 * @cancellable: a #xcancellable_t object
 *
 * Pushes @cancellable onto the cancellable stack. The current
 * cancellable can then be received using g_cancellable_get_current().
 *
 * This is useful when implementing cancellable operations in
 * code that does not allow you to pass down the cancellable object.
 *
 * This is typically called automatically by e.g. #xfile_t operations,
 * so you rarely have to call this yourself.
 **/
void
g_cancellable_push_current (xcancellable_t *cancellable)
{
  GSList *l;

  g_return_if_fail (cancellable != NULL);

  l = g_private_get (&current_cancellable);
  l = g_slist_prepend (l, cancellable);
  g_private_set (&current_cancellable, l);
}

/**
 * g_cancellable_pop_current:
 * @cancellable: a #xcancellable_t object
 *
 * Pops @cancellable off the cancellable stack (verifying that @cancellable
 * is on the top of the stack).
 **/
void
g_cancellable_pop_current (xcancellable_t *cancellable)
{
  GSList *l;

  l = g_private_get (&current_cancellable);

  g_return_if_fail (l != NULL);
  g_return_if_fail (l->data == cancellable);

  l = g_slist_delete_link (l, l);
  g_private_set (&current_cancellable, l);
}

/**
 * g_cancellable_get_current:
 *
 * Gets the top cancellable from the stack.
 *
 * Returns: (nullable) (transfer none): a #xcancellable_t from the top
 * of the stack, or %NULL if the stack is empty.
 **/
xcancellable_t *
g_cancellable_get_current  (void)
{
  GSList *l;

  l = g_private_get (&current_cancellable);
  if (l == NULL)
    return NULL;

  return G_CANCELLABLE (l->data);
}

/**
 * g_cancellable_reset:
 * @cancellable: a #xcancellable_t object.
 *
 * Resets @cancellable to its uncancelled state.
 *
 * If cancellable is currently in use by any cancellable operation
 * then the behavior of this function is undefined.
 *
 * Note that it is generally not a good idea to reuse an existing
 * cancellable for more operations after it has been cancelled once,
 * as this function might tempt you to do. The recommended practice
 * is to drop the reference to a cancellable after cancelling it,
 * and let it die with the outstanding async operations. You should
 * create a fresh cancellable for further async operations.
 **/
void
g_cancellable_reset (xcancellable_t *cancellable)
{
  GCancellablePrivate *priv;

  g_return_if_fail (X_IS_CANCELLABLE (cancellable));

  g_mutex_lock (&cancellable_mutex);

  priv = cancellable->priv;

  while (priv->cancelled_running)
    {
      priv->cancelled_running_waiting = TRUE;
      g_cond_wait (&cancellable_cond, &cancellable_mutex);
    }

  if (g_atomic_int_get (&priv->cancelled))
    {
      if (priv->wakeup)
        XPL_PRIVATE_CALL (g_wakeup_acknowledge) (priv->wakeup);

      g_atomic_int_set (&priv->cancelled, FALSE);
    }

  g_mutex_unlock (&cancellable_mutex);
}

/**
 * g_cancellable_is_cancelled:
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 *
 * Checks if a cancellable job has been cancelled.
 *
 * Returns: %TRUE if @cancellable is cancelled,
 * FALSE if called with %NULL or if item is not cancelled.
 **/
xboolean_t
g_cancellable_is_cancelled (xcancellable_t *cancellable)
{
  return cancellable != NULL && g_atomic_int_get (&cancellable->priv->cancelled);
}

/**
 * g_cancellable_set_error_if_cancelled:
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @error: #xerror_t to append error state to
 *
 * If the @cancellable is cancelled, sets the error to notify
 * that the operation was cancelled.
 *
 * Returns: %TRUE if @cancellable was cancelled, %FALSE if it was not
 */
xboolean_t
g_cancellable_set_error_if_cancelled (xcancellable_t  *cancellable,
                                      xerror_t       **error)
{
  if (g_cancellable_is_cancelled (cancellable))
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_CANCELLED,
                           _("Operation was cancelled"));
      return TRUE;
    }

  return FALSE;
}

/**
 * g_cancellable_get_fd:
 * @cancellable: a #xcancellable_t.
 *
 * Gets the file descriptor for a cancellable job. This can be used to
 * implement cancellable operations on Unix systems. The returned fd will
 * turn readable when @cancellable is cancelled.
 *
 * You are not supposed to read from the fd yourself, just check for
 * readable status. Reading to unset the readable status is done
 * with g_cancellable_reset().
 *
 * After a successful return from this function, you should use
 * g_cancellable_release_fd() to free up resources allocated for
 * the returned file descriptor.
 *
 * See also g_cancellable_make_pollfd().
 *
 * Returns: A valid file descriptor. `-1` if the file descriptor
 * is not supported, or on errors.
 **/
int
g_cancellable_get_fd (xcancellable_t *cancellable)
{
  GPollFD pollfd;
#ifndef G_OS_WIN32
  xboolean_t retval G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
#endif

  if (cancellable == NULL)
	  return -1;

#ifdef G_OS_WIN32
  pollfd.fd = -1;
#else
  retval = g_cancellable_make_pollfd (cancellable, &pollfd);
  g_assert (retval);
#endif

  return pollfd.fd;
}

/**
 * g_cancellable_make_pollfd:
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @pollfd: a pointer to a #GPollFD
 *
 * Creates a #GPollFD corresponding to @cancellable; this can be passed
 * to g_poll() and used to poll for cancellation. This is useful both
 * for unix systems without a native poll and for portability to
 * windows.
 *
 * When this function returns %TRUE, you should use
 * g_cancellable_release_fd() to free up resources allocated for the
 * @pollfd. After a %FALSE return, do not call g_cancellable_release_fd().
 *
 * If this function returns %FALSE, either no @cancellable was given or
 * resource limits prevent this function from allocating the necessary
 * structures for polling. (On Linux, you will likely have reached
 * the maximum number of file descriptors.) The suggested way to handle
 * these cases is to ignore the @cancellable.
 *
 * You are not supposed to read from the fd yourself, just check for
 * readable status. Reading to unset the readable status is done
 * with g_cancellable_reset().
 *
 * Returns: %TRUE if @pollfd was successfully initialized, %FALSE on
 *          failure to prepare the cancellable.
 *
 * Since: 2.22
 **/
xboolean_t
g_cancellable_make_pollfd (xcancellable_t *cancellable, GPollFD *pollfd)
{
  g_return_val_if_fail (pollfd != NULL, FALSE);
  if (cancellable == NULL)
    return FALSE;
  g_return_val_if_fail (X_IS_CANCELLABLE (cancellable), FALSE);

  g_mutex_lock (&cancellable_mutex);

  cancellable->priv->fd_refcount++;

  if (cancellable->priv->wakeup == NULL)
    {
      cancellable->priv->wakeup = XPL_PRIVATE_CALL (g_wakeup_new) ();

      if (g_atomic_int_get (&cancellable->priv->cancelled))
        XPL_PRIVATE_CALL (g_wakeup_signal) (cancellable->priv->wakeup);
    }

  XPL_PRIVATE_CALL (g_wakeup_get_pollfd) (cancellable->priv->wakeup, pollfd);

  g_mutex_unlock (&cancellable_mutex);

  return TRUE;
}

/**
 * g_cancellable_release_fd:
 * @cancellable: a #xcancellable_t
 *
 * Releases a resources previously allocated by g_cancellable_get_fd()
 * or g_cancellable_make_pollfd().
 *
 * For compatibility reasons with older releases, calling this function
 * is not strictly required, the resources will be automatically freed
 * when the @cancellable is finalized. However, the @cancellable will
 * block scarce file descriptors until it is finalized if this function
 * is not called. This can cause the application to run out of file
 * descriptors when many #GCancellables are used at the same time.
 *
 * Since: 2.22
 **/
void
g_cancellable_release_fd (xcancellable_t *cancellable)
{
  GCancellablePrivate *priv;

  if (cancellable == NULL)
    return;

  g_return_if_fail (X_IS_CANCELLABLE (cancellable));

  priv = cancellable->priv;

  g_mutex_lock (&cancellable_mutex);
  g_assert (priv->fd_refcount > 0);

  priv->fd_refcount--;
  if (priv->fd_refcount == 0)
    {
      XPL_PRIVATE_CALL (g_wakeup_free) (priv->wakeup);
      priv->wakeup = NULL;
    }

  g_mutex_unlock (&cancellable_mutex);
}

/**
 * g_cancellable_cancel:
 * @cancellable: (nullable): a #xcancellable_t object.
 *
 * Will set @cancellable to cancelled, and will emit the
 * #xcancellable_t::cancelled signal. (However, see the warning about
 * race conditions in the documentation for that signal if you are
 * planning to connect to it.)
 *
 * This function is thread-safe. In other words, you can safely call
 * it from a thread other than the one running the operation that was
 * passed the @cancellable.
 *
 * If @cancellable is %NULL, this function returns immediately for convenience.
 *
 * The convention within GIO is that cancelling an asynchronous
 * operation causes it to complete asynchronously. That is, if you
 * cancel the operation from the same thread in which it is running,
 * then the operation's #xasync_ready_callback_t will not be invoked until
 * the application returns to the main loop.
 **/
void
g_cancellable_cancel (xcancellable_t *cancellable)
{
  GCancellablePrivate *priv;

  if (cancellable == NULL || g_cancellable_is_cancelled (cancellable))
    return;

  priv = cancellable->priv;

  g_mutex_lock (&cancellable_mutex);

  if (g_atomic_int_get (&priv->cancelled))
    {
      g_mutex_unlock (&cancellable_mutex);
      return;
    }

  g_atomic_int_set (&priv->cancelled, TRUE);
  priv->cancelled_running = TRUE;

  if (priv->wakeup)
    XPL_PRIVATE_CALL (g_wakeup_signal) (priv->wakeup);

  g_mutex_unlock (&cancellable_mutex);

  g_object_ref (cancellable);
  g_signal_emit (cancellable, signals[CANCELLED], 0);

  g_mutex_lock (&cancellable_mutex);

  priv->cancelled_running = FALSE;
  if (priv->cancelled_running_waiting)
    g_cond_broadcast (&cancellable_cond);
  priv->cancelled_running_waiting = FALSE;

  g_mutex_unlock (&cancellable_mutex);

  g_object_unref (cancellable);
}

/**
 * g_cancellable_connect:
 * @cancellable: A #xcancellable_t.
 * @callback: The #GCallback to connect.
 * @data: Data to pass to @callback.
 * @data_destroy_func: (nullable): Free function for @data or %NULL.
 *
 * Convenience function to connect to the #xcancellable_t::cancelled
 * signal. Also handles the race condition that may happen
 * if the cancellable is cancelled right before connecting.
 *
 * @callback is called at most once, either directly at the
 * time of the connect if @cancellable is already cancelled,
 * or when @cancellable is cancelled in some thread.
 *
 * @data_destroy_func will be called when the handler is
 * disconnected, or immediately if the cancellable is already
 * cancelled.
 *
 * See #xcancellable_t::cancelled for details on how to use this.
 *
 * Since GLib 2.40, the lock protecting @cancellable is not held when
 * @callback is invoked.  This lifts a restriction in place for
 * earlier GLib versions which now makes it easier to write cleanup
 * code that unconditionally invokes e.g. g_cancellable_cancel().
 *
 * Returns: The id of the signal handler or 0 if @cancellable has already
 *          been cancelled.
 *
 * Since: 2.22
 */
gulong
g_cancellable_connect (xcancellable_t   *cancellable,
		       GCallback       callback,
		       xpointer_t        data,
		       GDestroyNotify  data_destroy_func)
{
  gulong id;

  g_return_val_if_fail (X_IS_CANCELLABLE (cancellable), 0);

  g_mutex_lock (&cancellable_mutex);

  if (g_atomic_int_get (&cancellable->priv->cancelled))
    {
      void (*_callback) (xcancellable_t *cancellable,
                         xpointer_t      user_data);

      g_mutex_unlock (&cancellable_mutex);

      _callback = (void *)callback;
      id = 0;

      _callback (cancellable, data);

      if (data_destroy_func)
        data_destroy_func (data);
    }
  else
    {
      id = g_signal_connect_data (cancellable, "cancelled",
                                  callback, data,
                                  (GClosureNotify) data_destroy_func,
                                  0);

      g_mutex_unlock (&cancellable_mutex);
    }


  return id;
}

/**
 * g_cancellable_disconnect:
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @handler_id: Handler id of the handler to be disconnected, or `0`.
 *
 * Disconnects a handler from a cancellable instance similar to
 * g_signal_handler_disconnect().  Additionally, in the event that a
 * signal handler is currently running, this call will block until the
 * handler has finished.  Calling this function from a
 * #xcancellable_t::cancelled signal handler will therefore result in a
 * deadlock.
 *
 * This avoids a race condition where a thread cancels at the
 * same time as the cancellable operation is finished and the
 * signal handler is removed. See #xcancellable_t::cancelled for
 * details on how to use this.
 *
 * If @cancellable is %NULL or @handler_id is `0` this function does
 * nothing.
 *
 * Since: 2.22
 */
void
g_cancellable_disconnect (xcancellable_t  *cancellable,
			  gulong         handler_id)
{
  GCancellablePrivate *priv;

  if (handler_id == 0 ||  cancellable == NULL)
    return;

  g_mutex_lock (&cancellable_mutex);

  priv = cancellable->priv;

  while (priv->cancelled_running)
    {
      priv->cancelled_running_waiting = TRUE;
      g_cond_wait (&cancellable_cond, &cancellable_mutex);
    }

  g_signal_handler_disconnect (cancellable, handler_id);

  g_mutex_unlock (&cancellable_mutex);
}

typedef struct {
  GSource       source;

  xcancellable_t *cancellable;
  gulong        cancelled_handler;
  /* Protected by cancellable_mutex: */
  xboolean_t      resurrected_during_cancellation;
} GCancellableSource;

/*
 * The reference count of the GSource might be 0 at this point but it is not
 * finalized yet and its dispose function did not run yet, or otherwise we
 * would have disconnected the signal handler already and due to the signal
 * emission lock it would be impossible to call the signal handler at that
 * point. That is: at this point we either have a fully valid GSource, or
 * it's not disposed or finalized yet and we can still resurrect it as needed.
 *
 * As such we first ensure that we have a strong reference to the GSource in
 * here before calling any other GSource API.
 */
static void
cancellable_source_cancelled (xcancellable_t *cancellable,
			      xpointer_t      user_data)
{
  GSource *source = user_data;
  GCancellableSource *cancellable_source = (GCancellableSource *) source;

  g_mutex_lock (&cancellable_mutex);

  /* Drop the reference added in cancellable_source_dispose(); see the comment there.
   * The reference must be dropped after unlocking @cancellable_mutex since
   * it could be the final reference, and the dispose function takes
   * @cancellable_mutex. */
  if (cancellable_source->resurrected_during_cancellation)
    {
      cancellable_source->resurrected_during_cancellation = FALSE;
      g_mutex_unlock (&cancellable_mutex);
      g_source_unref (source);
      return;
    }

  g_source_ref (source);
  g_mutex_unlock (&cancellable_mutex);
  g_source_set_ready_time (source, 0);
  g_source_unref (source);
}

static xboolean_t
cancellable_source_dispatch (GSource     *source,
			     GSourceFunc  callback,
			     xpointer_t     user_data)
{
  GCancellableSourceFunc func = (GCancellableSourceFunc)callback;
  GCancellableSource *cancellable_source = (GCancellableSource *)source;

  g_source_set_ready_time (source, -1);
  return (*func) (cancellable_source->cancellable, user_data);
}

static void
cancellable_source_dispose (GSource *source)
{
  GCancellableSource *cancellable_source = (GCancellableSource *)source;

  g_mutex_lock (&cancellable_mutex);

  if (cancellable_source->cancellable)
    {
      if (cancellable_source->cancellable->priv->cancelled_running)
        {
          /* There can be a race here: if thread A has called
           * g_cancellable_cancel() and has got as far as committing to call
           * cancellable_source_cancelled(), then thread B drops the final
           * ref on the GCancellableSource before g_source_ref() is called in
           * cancellable_source_cancelled(), then cancellable_source_dispose()
           * will run through and the GCancellableSource will be finalised
           * before cancellable_source_cancelled() gets to g_source_ref(). It
           * will then be left in a state where it’s committed to using a
           * dangling GCancellableSource pointer.
           *
           * Eliminate that race by resurrecting the #GSource temporarily, and
           * then dropping that reference in cancellable_source_cancelled(),
           * which should be guaranteed to fire because we’re inside a
           * @cancelled_running block.
           */
          g_source_ref (source);
          cancellable_source->resurrected_during_cancellation = TRUE;
        }

      g_clear_signal_handler (&cancellable_source->cancelled_handler,
                              cancellable_source->cancellable);
      g_clear_object (&cancellable_source->cancellable);
    }

  g_mutex_unlock (&cancellable_mutex);
}

static xboolean_t
cancellable_source_closure_callback (xcancellable_t *cancellable,
				     xpointer_t      data)
{
  GClosure *closure = data;

  GValue params = G_VALUE_INIT;
  GValue result_value = G_VALUE_INIT;
  xboolean_t result;

  g_value_init (&result_value, XTYPE_BOOLEAN);

  g_value_init (&params, XTYPE_CANCELLABLE);
  g_value_set_object (&params, cancellable);

  g_closure_invoke (closure, &result_value, 1, &params, NULL);

  result = g_value_get_boolean (&result_value);
  g_value_unset (&result_value);
  g_value_unset (&params);

  return result;
}

static GSourceFuncs cancellable_source_funcs =
{
  NULL,
  NULL,
  cancellable_source_dispatch,
  NULL,
  (GSourceFunc)cancellable_source_closure_callback,
  NULL,
};

/**
 * g_cancellable_source_new:
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 *
 * Creates a source that triggers if @cancellable is cancelled and
 * calls its callback of type #GCancellableSourceFunc. This is
 * primarily useful for attaching to another (non-cancellable) source
 * with g_source_add_child_source() to add cancellability to it.
 *
 * For convenience, you can call this with a %NULL #xcancellable_t,
 * in which case the source will never trigger.
 *
 * The new #GSource will hold a reference to the #xcancellable_t.
 *
 * Returns: (transfer full): the new #GSource.
 *
 * Since: 2.28
 */
GSource *
g_cancellable_source_new (xcancellable_t *cancellable)
{
  GSource *source;
  GCancellableSource *cancellable_source;

  source = g_source_new (&cancellable_source_funcs, sizeof (GCancellableSource));
  g_source_set_static_name (source, "xcancellable_t");
  g_source_set_dispose_function (source, cancellable_source_dispose);
  cancellable_source = (GCancellableSource *)source;

  if (cancellable)
    {
      cancellable_source->cancellable = g_object_ref (cancellable);

      /* We intentionally don't use g_cancellable_connect() here,
       * because we don't want the "at most once" behavior.
       */
      cancellable_source->cancelled_handler =
        g_signal_connect (cancellable, "cancelled",
                          G_CALLBACK (cancellable_source_cancelled),
                          source);
      if (g_cancellable_is_cancelled (cancellable))
        g_source_set_ready_time (source, 0);
    }

  return source;
}
