/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2011 Collabora Ltd.
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
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#include <locale.h>

#include <gio/gio.h>

#include "glib/glib-private.h"

/* How long to wait in ms for each iteration */
#define WAIT_ITERATION (10)

static xint_t num_async_operations = 0;

typedef struct
{
  xuint_t iterations_requested;  /* construct-only */
  xuint_t iterations_done;  /* (atomic) */
} MockOperationData;

static void
mock_operation_free (xpointer_t user_data)
{
  MockOperationData *data = user_data;
  g_free (data);
}

static void
mock_operation_thread (xtask_t        *task,
                       xpointer_t      source_object,
                       xpointer_t      task_data,
                       xcancellable_t *cancellable)
{
  MockOperationData *data = task_data;
  xuint_t i;

  for (i = 0; i < data->iterations_requested; i++)
    {
      if (xcancellable_is_cancelled (cancellable))
        break;
      if (g_test_verbose ())
        g_test_message ("THRD: %u iteration %u", data->iterations_requested, i);
      g_usleep (WAIT_ITERATION * 1000);
    }

  if (g_test_verbose ())
    g_test_message ("THRD: %u stopped at %u", data->iterations_requested, i);
  g_atomic_int_add (&data->iterations_done, i);

  xtask_return_boolean (task, TRUE);
}

static xboolean_t
mock_operation_timeout (xpointer_t user_data)
{
  xtask_t *task;
  MockOperationData *data;
  xboolean_t done = FALSE;
  xuint_t iterations_done;

  task = XTASK (user_data);
  data = xtask_get_task_data (task);
  iterations_done = g_atomic_int_get (&data->iterations_done);

  if (iterations_done >= data->iterations_requested)
      done = TRUE;

  if (xcancellable_is_cancelled (xtask_get_cancellable (task)))
      done = TRUE;

  if (done)
    {
      if (g_test_verbose ())
        g_test_message ("LOOP: %u stopped at %u",
                        data->iterations_requested, iterations_done);
      xtask_return_boolean (task, TRUE);
      return XSOURCE_REMOVE;
    }
  else
    {
      g_atomic_int_inc (&data->iterations_done);
      if (g_test_verbose ())
        g_test_message ("LOOP: %u iteration %u",
                        data->iterations_requested, iterations_done + 1);
      return G_SOURCE_CONTINUE;
    }
}

static void
mock_operation_async (xuint_t                wait_iterations,
                      xboolean_t             run_in_thread,
                      xcancellable_t        *cancellable,
                      xasync_ready_callback_t  callback,
                      xpointer_t             user_data)
{
  xtask_t *task;
  MockOperationData *data;

  task = xtask_new (NULL, cancellable, callback, user_data);
  data = g_new0 (MockOperationData, 1);
  data->iterations_requested = wait_iterations;
  xtask_set_task_data (task, data, mock_operation_free);

  if (run_in_thread)
    {
      xtask_run_in_thread (task, mock_operation_thread);
      if (g_test_verbose ())
        g_test_message ("THRD: %d started", wait_iterations);
    }
  else
    {
      g_timeout_add_full (G_PRIORITY_DEFAULT, WAIT_ITERATION, mock_operation_timeout,
                          xobject_ref (task), xobject_unref);
      if (g_test_verbose ())
        g_test_message ("LOOP: %d started", wait_iterations);
    }

  xobject_unref (task);
}

static xuint_t
mock_operation_finish (xasync_result_t  *result,
                       xerror_t       **error)
{
  MockOperationData *data;
  xtask_t *task;

  g_assert_true (xtask_is_valid (result, NULL));

  /* This test expects the return value to be iterations_done even
   * when an error is set.
   */
  task = XTASK (result);
  data = xtask_get_task_data (task);

  xtask_propagate_boolean (task, error);
  return g_atomic_int_get (&data->iterations_done);
}

static void
on_mock_operation_ready (xobject_t      *source,
                         xasync_result_t *result,
                         xpointer_t      user_data)
{
  xuint_t iterations_requested;
  xuint_t iterations_done;
  xerror_t *error = NULL;

  iterations_requested = GPOINTER_TO_UINT (user_data);
  iterations_done = mock_operation_finish (result, &error);

  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  xerror_free (error);

  g_assert_cmpint (iterations_requested, >, iterations_done);
  num_async_operations--;
  xmain_context_wakeup (NULL);
}

static void
test_cancel_multiple_concurrent (void)
{
  xcancellable_t *cancellable;
  xuint_t i, iterations;

  if (!g_test_thorough ())
    {
      g_test_skip ("Not running timing heavy test");
      return;
    }

  cancellable = xcancellable_new ();

  for (i = 0; i < 45; i++)
    {
      iterations = i + 10;
      mock_operation_async (iterations, g_random_boolean (), cancellable,
                            on_mock_operation_ready, GUINT_TO_POINTER (iterations));
      num_async_operations++;
    }

  /* Wait for the threads to start up */
  while (num_async_operations != 45)
    xmain_context_iteration (NULL, TRUE);
  g_assert_cmpint (num_async_operations, ==, 45);\

  if (g_test_verbose ())
    g_test_message ("CANCEL: %d operations", num_async_operations);
  xcancellable_cancel (cancellable);
  g_assert_true (xcancellable_is_cancelled (cancellable));

  /* Wait for all operations to be cancelled */
  while (num_async_operations != 0)
    xmain_context_iteration (NULL, TRUE);
  g_assert_cmpint (num_async_operations, ==, 0);

  xobject_unref (cancellable);
}

static void
test_cancel_null (void)
{
  xcancellable_cancel (NULL);
}

typedef struct
{
  xcond_t cond;
  xmutex_t mutex;
  xboolean_t thread_ready;
  xasync_queue_t *cancellable_source_queue;  /* (owned) (element-type GCancellableSource) */
} ThreadedDisposeData;

static xboolean_t
cancelled_cb (xcancellable_t *cancellable,
              xpointer_t      user_data)
{
  /* Nothing needs to be done here. */
  return G_SOURCE_CONTINUE;
}

static xpointer_t
threaded_dispose_thread_cb (xpointer_t user_data)
{
  ThreadedDisposeData *data = user_data;
  xsource_t *cancellable_source;

  g_mutex_lock (&data->mutex);
  data->thread_ready = TRUE;
  g_cond_broadcast (&data->cond);
  g_mutex_unlock (&data->mutex);

  while ((cancellable_source = g_async_queue_pop (data->cancellable_source_queue)) != (xpointer_t) 1)
    {
      /* Race with cancellation of the cancellable. */
      xsource_unref (cancellable_source);
    }

  return NULL;
}

static void
test_cancellable_source_threaded_dispose (void)
{
#ifdef _XPL_ADDRESS_SANITIZER
  g_test_incomplete ("FIXME: Leaks lots of GCancellableSource objects, see glib#2309");
  (void) cancelled_cb;
  (void) threaded_dispose_thread_cb;
#else
  ThreadedDisposeData data;
  xthread_t *thread = NULL;
  xuint_t i;
  xptr_array_t *cancellables_pending_unref = xptr_array_new_with_free_func (xobject_unref);

  g_test_summary ("test_t a thread race between disposing of a GCancellableSource "
                  "(in one thread) and cancelling the xcancellable_t it refers "
                  "to (in another thread)");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1841");

  /* Create a new thread and wait until it’s ready to execute. Each iteration of
   * the test will pass it a new #GCancellableSource. */
  g_cond_init (&data.cond);
  g_mutex_init (&data.mutex);
  data.cancellable_source_queue = g_async_queue_new_full ((xdestroy_notify_t) xsource_unref);
  data.thread_ready = FALSE;

  g_mutex_lock (&data.mutex);
  thread = xthread_new ("/cancellable-source/threaded-dispose",
                         threaded_dispose_thread_cb, &data);

  while (!data.thread_ready)
    g_cond_wait (&data.cond, &data.mutex);
  g_mutex_unlock (&data.mutex);

  for (i = 0; i < 100000; i++)
    {
      xcancellable_t *cancellable = NULL;
      xsource_t *cancellable_source = NULL;

      /* Create a cancellable and a cancellable source for it. For this test,
       * there’s no need to attach the source to a #xmain_context_t. */
      cancellable = xcancellable_new ();
      cancellable_source = xcancellable_source_new (cancellable);
      xsource_set_callback (cancellable_source, G_SOURCE_FUNC (cancelled_cb), NULL, NULL);

      /* Send it to the thread and wait until it’s ready to execute before
       * cancelling our cancellable. */
      g_async_queue_push (data.cancellable_source_queue, g_steal_pointer (&cancellable_source));

      /* Race with disposal of the cancellable source. */
      xcancellable_cancel (cancellable);

      /* This thread can’t drop its reference to the #xcancellable_t here, as it
       * might not be the final reference (depending on how the race is
       * resolved: #GCancellableSource holds a strong ref on the #xcancellable_t),
       * and at this point we can’t guarantee to support disposing of a
       * #xcancellable_t in a different thread from where it’s created, especially
       * when signal handlers are connected to it.
       *
       * So this is a workaround for a disposal-in-another-thread bug for
       * #xcancellable_t, but there’s no hope of debugging and resolving it with
       * this test setup, and the bug is orthogonal to what’s being tested here
       * (a race between #xcancellable_t and #GCancellableSource). */
      xptr_array_add (cancellables_pending_unref, g_steal_pointer (&cancellable));
    }

  /* Indicate that the test has finished. Can’t use %NULL as #xasync_queue_t
   * doesn’t allow that.*/
  g_async_queue_push (data.cancellable_source_queue, (xpointer_t) 1);

  xthread_join (g_steal_pointer (&thread));

  xassert (g_async_queue_length (data.cancellable_source_queue) == 0);
  g_async_queue_unref (data.cancellable_source_queue);
  g_mutex_clear (&data.mutex);
  g_cond_clear (&data.cond);

  xptr_array_unref (cancellables_pending_unref);
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/cancellable/multiple-concurrent", test_cancel_multiple_concurrent);
  g_test_add_func ("/cancellable/null", test_cancel_null);
  g_test_add_func ("/cancellable-source/threaded-dispose", test_cancellable_source_threaded_dispose);

  return g_test_run ();
}
