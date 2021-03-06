/*
 * Copyright 2012-2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#include <gio/gio.h>
#include <string.h>

static xmain_loop_t *loop;
static xthread_t *main_thread;
static xssize_t magic;

/* We need objects for a few tests where we don't care what type
 * they are, just that they're GObjects.
 */
#define g_dummy_object_new xsocket_client_new

static xboolean_t
idle_quit_loop (xpointer_t user_data)
{
  xmain_loop_quit (loop);
  return FALSE;
}

static void
completed_cb (xobject_t    *gobject,
              xparam_spec_t *pspec,
              xpointer_t    user_data)
{
	xboolean_t *notification_emitted = user_data;
	*notification_emitted = TRUE;
}

static void
wait_for_completed_notification (xtask_t *task)
{
  xboolean_t notification_emitted = FALSE;
  xboolean_t is_completed = FALSE;

  /* Hold a ref. so we can check the :completed property afterwards. */
  xobject_ref (task);

  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);
  g_idle_add (idle_quit_loop, NULL);
  xmain_loop_run (loop);
  g_assert_true (notification_emitted);

  g_assert_true (xtask_get_completed (task));
  xobject_get (G_OBJECT (task), "completed", &is_completed, NULL);
  g_assert_true (is_completed);

  xobject_unref (task);
}

/* test_basic */

static void
basic_callback (xobject_t      *object,
                xasync_result_t *result,
                xpointer_t      user_data)
{
  xssize_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  *result_out = xtask_propagate_int (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static xboolean_t
basic_return (xpointer_t user_data)
{
  xtask_t *task = user_data;

  xtask_return_int (task, magic);
  xobject_unref (task);

  return FALSE;
}

static void
basic_destroy_notify (xpointer_t user_data)
{
  xboolean_t *destroyed = user_data;

  *destroyed = TRUE;
}

static void
test_basic (void)
{
  xtask_t *task;
  xssize_t result;
  xboolean_t task_data_destroyed = FALSE;
  xboolean_t notification_emitted = FALSE;

  task = xtask_new (NULL, NULL, basic_callback, &result);
  xtask_set_task_data (task, &task_data_destroyed, basic_destroy_notify);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  g_idle_add (basic_return, task);
  xmain_loop_run (loop);

  g_assert_cmpint (result, ==, magic);
  xassert (task_data_destroyed == TRUE);
  g_assert_true (notification_emitted);
  xassert (task == NULL);
}

/* test_error */

static void
error_callback (xobject_t      *object,
                xasync_result_t *result,
                xpointer_t      user_data)
{
  xssize_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  *result_out = xtask_propagate_int (XTASK (result), &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
  xerror_free (error);

  xassert (xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static xboolean_t
error_return (xpointer_t user_data)
{
  xtask_t *task = user_data;

  xtask_return_new_error (task,
                           G_IO_ERROR, G_IO_ERROR_FAILED,
                           "Failed");
  xobject_unref (task);

  return FALSE;
}

static void
error_destroy_notify (xpointer_t user_data)
{
  xboolean_t *destroyed = user_data;

  *destroyed = TRUE;
}

static void
test_error (void)
{
  xtask_t *task;
  xssize_t result;
  xboolean_t first_task_data_destroyed = FALSE;
  xboolean_t second_task_data_destroyed = FALSE;
  xboolean_t notification_emitted = FALSE;

  task = xtask_new (NULL, NULL, error_callback, &result);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xassert (first_task_data_destroyed == FALSE);
  xtask_set_task_data (task, &first_task_data_destroyed, error_destroy_notify);
  xassert (first_task_data_destroyed == FALSE);

  /* Calling xtask_set_task_data() again will destroy the first data */
  xtask_set_task_data (task, &second_task_data_destroyed, error_destroy_notify);
  xassert (first_task_data_destroyed == TRUE);
  xassert (second_task_data_destroyed == FALSE);

  g_idle_add (error_return, task);
  xmain_loop_run (loop);

  g_assert_cmpint (result, ==, -1);
  xassert (second_task_data_destroyed == TRUE);
  g_assert_true (notification_emitted);
  xassert (task == NULL);
}

/* test_return_from_same_iteration: calling xtask_return_* from the
 * loop iteration the task was created in defers completion until the
 * next iteration.
 */
xboolean_t same_result = FALSE;
xboolean_t same_notification_emitted = FALSE;

static void
same_callback (xobject_t      *object,
               xasync_result_t *result,
               xpointer_t      user_data)
{
  xboolean_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  *result_out = xtask_propagate_boolean (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static xboolean_t
same_start (xpointer_t user_data)
{
  xpointer_t *weak_pointer = user_data;
  xtask_t *task;

  task = xtask_new (NULL, NULL, same_callback, &same_result);
  *weak_pointer = task;
  xobject_add_weak_pointer (G_OBJECT (task), weak_pointer);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &same_notification_emitted);

  xtask_return_boolean (task, TRUE);
  xobject_unref (task);

  /* same_callback should not have been invoked yet */
  xassert (same_result == FALSE);
  xassert (*weak_pointer == task);
  g_assert_false (same_notification_emitted);

  return FALSE;
}

static void
test_return_from_same_iteration (void)
{
  xpointer_t weak_pointer;

  g_idle_add (same_start, &weak_pointer);
  xmain_loop_run (loop);

  xassert (same_result == TRUE);
  xassert (weak_pointer == NULL);
  g_assert_true (same_notification_emitted);
}

/* test_return_from_toplevel: calling xtask_return_* from outside any
 * main loop completes the task inside the main loop.
 */
xboolean_t toplevel_notification_emitted = FALSE;

static void
toplevel_callback (xobject_t      *object,
              xasync_result_t *result,
              xpointer_t      user_data)
{
  xboolean_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  *result_out = xtask_propagate_boolean (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static void
test_return_from_toplevel (void)
{
  xtask_t *task;
  xboolean_t result = FALSE;

  task = xtask_new (NULL, NULL, toplevel_callback, &result);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &toplevel_notification_emitted);

  xtask_return_boolean (task, TRUE);
  xobject_unref (task);

  /* toplevel_callback should not have been invoked yet */
  xassert (result == FALSE);
  xassert (task != NULL);
  g_assert_false (toplevel_notification_emitted);

  xmain_loop_run (loop);

  xassert (result == TRUE);
  xassert (task == NULL);
  g_assert_true (toplevel_notification_emitted);
}

/* test_return_from_anon_thread: calling xtask_return_* from a
 * thread with no thread-default main context will complete the
 * task in the task's context/thread.
 */

xboolean_t anon_thread_notification_emitted = FALSE;
xthread_t *anon_thread;

static void
anon_callback (xobject_t      *object,
               xasync_result_t *result,
               xpointer_t      user_data)
{
  xssize_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  xassert (xthread_self () == main_thread);

  *result_out = xtask_propagate_int (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static xpointer_t
anon_thread_func (xpointer_t user_data)
{
  xtask_t *task = user_data;

  xtask_return_int (task, magic);
  xobject_unref (task);

  return NULL;
}

static xboolean_t
anon_start (xpointer_t user_data)
{
  xtask_t *task = user_data;

  anon_thread = xthread_new ("test_return_from_anon_thread",
                              anon_thread_func, task);
  return FALSE;
}

static void
test_return_from_anon_thread (void)
{
  xtask_t *task;
  xssize_t result = 0;

  task = xtask_new (NULL, NULL, anon_callback, &result);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb,
                    &anon_thread_notification_emitted);

  g_idle_add (anon_start, task);
  xmain_loop_run (loop);

  xthread_join (anon_thread);

  g_assert_cmpint (result, ==, magic);
  xassert (task == NULL);
  g_assert_true (anon_thread_notification_emitted);
}

/* test_return_from_wronxthread: calling xtask_return_* from a
 * thread with its own thread-default main context will complete the
 * task in the task's context/thread.
 */

xboolean_t wronxthread_notification_emitted = FALSE;
xthread_t *wronxthread;

static void
wrong_callback (xobject_t      *object,
               xasync_result_t *result,
               xpointer_t      user_data)
{
  xssize_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  xassert (xthread_self () == main_thread);

  *result_out = xtask_propagate_int (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static xpointer_t
wronxthread_func (xpointer_t user_data)
{
  xtask_t *task = user_data;
  xmain_context_t *context;

  context = xmain_context_new ();
  xmain_context_push_thread_default (context);

  xassert (xtask_get_context (task) != context);

  xtask_return_int (task, magic);
  xobject_unref (task);

  xmain_context_pop_thread_default (context);
  xmain_context_unref (context);

  return NULL;
}

static xboolean_t
wrong_start (xpointer_t user_data)
{
  xtask_t *task = user_data;

  wronxthread = xthread_new ("test_return_from_anon_thread",
                               wronxthread_func, task);
  return FALSE;
}

static void
test_return_from_wronxthread (void)
{
  xtask_t *task;
  xssize_t result = 0;

  task = xtask_new (NULL, NULL, wrong_callback, &result);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb,
                    &wronxthread_notification_emitted);

  g_idle_add (wrong_start, task);
  xmain_loop_run (loop);

  xthread_join (wronxthread);

  g_assert_cmpint (result, ==, magic);
  xassert (task == NULL);
  g_assert_true (wronxthread_notification_emitted);
}

/* test_no_callback */

static void
test_no_callback (void)
{
  xtask_t *task;

  task = xtask_new (NULL, NULL, NULL, NULL);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);

  xtask_return_boolean (task, TRUE);
  xobject_unref (task);

  /* Even though there???s no callback, the :completed notification has to
   * happen in an idle handler. */
  g_assert_nonnull (task);
  wait_for_completed_notification (task);
  g_assert_null (task);
}

/* test_report_error */

static void test_report_error (void);
xboolean_t error_notification_emitted = FALSE;

static void
report_callback (xobject_t      *object,
                 xasync_result_t *result,
                 xpointer_t      user_data)
{
  xpointer_t *weak_pointer = user_data;
  xerror_t *error = NULL;
  xssize_t ret;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (xasync_result_is_tagged (result, test_report_error));
  xassert (xtask_get_source_tag (XTASK (result)) == test_report_error);
  xassert (xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  ret = xtask_propagate_int (XTASK (result), &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_assert_cmpint (ret, ==, -1);
  xerror_free (error);

  xassert (xtask_had_error (XTASK (result)));

  *weak_pointer = result;
  xobject_add_weak_pointer (G_OBJECT (result), weak_pointer);
  xsignal_connect (result, "notify::completed",
                    (xcallback_t) completed_cb, &error_notification_emitted);

  xmain_loop_quit (loop);
}

static void
test_report_error (void)
{
  xpointer_t weak_pointer = (xpointer_t)-1;

  xtask_report_new_error (NULL, report_callback, &weak_pointer,
                           test_report_error,
                           G_IO_ERROR, G_IO_ERROR_FAILED,
                           "Failed");
  xmain_loop_run (loop);

  xassert (weak_pointer == NULL);
  g_assert_true (error_notification_emitted);
}

/* test_priority: tasks complete in priority order */

static int counter = 0;

static void
priority_callback (xobject_t      *object,
                   xasync_result_t *result,
                   xpointer_t      user_data)
{
  xssize_t *ret_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  xtask_propagate_boolean (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  *ret_out = ++counter;

  if (counter == 3)
    xmain_loop_quit (loop);
}

static void
test_priority (void)
{
  xtask_t *t1, *t2, *t3;
  xssize_t ret1, ret2, ret3;

  /* t2 has higher priority than either t1 or t3, so we can't
   * accidentally pass the test just by completing the tasks in the
   * order they were created (or in reverse order).
   */

  t1 = xtask_new (NULL, NULL, priority_callback, &ret1);
  xtask_set_priority (t1, G_PRIORITY_DEFAULT);
  xtask_return_boolean (t1, TRUE);
  xobject_unref (t1);

  t2 = xtask_new (NULL, NULL, priority_callback, &ret2);
  xtask_set_priority (t2, G_PRIORITY_HIGH);
  xtask_return_boolean (t2, TRUE);
  xobject_unref (t2);

  t3 = xtask_new (NULL, NULL, priority_callback, &ret3);
  xtask_set_priority (t3, G_PRIORITY_LOW);
  xtask_return_boolean (t3, TRUE);
  xobject_unref (t3);

  xmain_loop_run (loop);

  g_assert_cmpint (ret2, ==, 1);
  g_assert_cmpint (ret1, ==, 2);
  g_assert_cmpint (ret3, ==, 3);
}

/* test_t that getting and setting the task name works. */
static void name_callback (xobject_t      *object,
                           xasync_result_t *result,
                           xpointer_t      user_data);

static void
test_name (void)
{
  xtask_t *t1 = NULL;
  xchar_t *name1 = NULL;

  t1 = xtask_new (NULL, NULL, name_callback, &name1);
  xtask_set_name (t1, "some task");
  xtask_return_boolean (t1, TRUE);
  xobject_unref (t1);

  xmain_loop_run (loop);

  g_assert_cmpstr (name1, ==, "some task");

  g_free (name1);
}

static void
name_callback (xobject_t      *object,
               xasync_result_t *result,
               xpointer_t      user_data)
{
  xchar_t **name_out = user_data;
  xerror_t *local_error = NULL;

  g_assert_null (*name_out);
  *name_out = xstrdup (xtask_get_name (XTASK (result)));

  xtask_propagate_boolean (XTASK (result), &local_error);
  g_assert_no_error (local_error);

  xmain_loop_quit (loop);
}

/* test_asynchronous_cancellation: cancelled tasks are returned
 * asynchronously, i.e. not from inside the xcancellable_t::cancelled
 * handler.
 *
 * The test is set up further below in test_asynchronous_cancellation.
 */

/* asynchronous_cancellation_callback represents the callback that the
 * caller of a typical asynchronous API would have passed. See
 * test_asynchronous_cancellation.
 */
static void
asynchronous_cancellation_callback (xobject_t      *object,
                                    xasync_result_t *result,
                                    xpointer_t      user_data)
{
  xerror_t *error = NULL;
  xuint_t run_task_id;

  g_assert_null (object);
  g_assert_true (xtask_is_valid (result, object));
  g_assert_true (xasync_result_get_user_data (result) == user_data);
  g_assert_true (xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  run_task_id = GPOINTER_TO_UINT (xtask_get_task_data (XTASK (result)));
  g_assert_cmpuint (run_task_id, ==, 0);

  xtask_propagate_boolean (XTASK (result), &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);

  g_assert_true (xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

/* asynchronous_cancellation_cancel_task represents a user cancelling
 * the ongoing operation. To make it somewhat realistic it is delayed
 * by 50ms via a timeout xsource_t. See test_asynchronous_cancellation.
 */
static xboolean_t
asynchronous_cancellation_cancel_task (xpointer_t user_data)
{
  xcancellable_t *cancellable;
  xtask_t *task = XTASK (user_data);

  cancellable = xtask_get_cancellable (task);
  g_assert_true (X_IS_CANCELLABLE (cancellable));

  xcancellable_cancel (cancellable);
  g_assert_false (xtask_get_completed (task));

  return XSOURCE_REMOVE;
}

/* asynchronous_cancellation_cancelled is the xcancellable_t::cancelled
 * handler that's used by the asynchronous implementation for
 * cancelling itself.
 */
static void
asynchronous_cancellation_cancelled (xcancellable_t *cancellable,
                                     xpointer_t      user_data)
{
  xtask_t *task = XTASK (user_data);
  xuint_t run_task_id;

  g_assert_true (cancellable == xtask_get_cancellable (task));

  run_task_id = GPOINTER_TO_UINT (xtask_get_task_data (task));
  g_assert_cmpuint (run_task_id, !=, 0);

  xsource_remove (run_task_id);
  xtask_set_task_data (task, GUINT_TO_POINTER (0), NULL);

  xtask_return_boolean (task, FALSE);
  g_assert_false (xtask_get_completed (task));
}

/* asynchronous_cancellation_run_task represents the actual
 * asynchronous work being done in an idle xsource_t as was mentioned
 * above. This is effectively meant to be an infinite loop so that
 * the only way to break out of it is via cancellation.
 */
static xboolean_t
asynchronous_cancellation_run_task (xpointer_t user_data)
{
  xcancellable_t *cancellable;
  xtask_t *task = XTASK (user_data);

  cancellable = xtask_get_cancellable (task);
  g_assert_true (X_IS_CANCELLABLE (cancellable));
  g_assert_false (xcancellable_is_cancelled (cancellable));

  return G_SOURCE_CONTINUE;
}

/* test_t that cancellation is always asynchronous. The completion callback for
 * a #xtask_t must not be called from inside the cancellation handler.
 *
 * The body of the loop inside test_asynchronous_cancellation
 * represents what would have been a typical asynchronous API call,
 * and its implementation. They are fused together without an API
 * boundary. The actual work done by this asynchronous API is
 * represented by an idle xsource_t.
 */
static void
test_asynchronous_cancellation (void)
{
  xuint_t i;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1608");

  /* Run a few times to shake out any timing issues between the
   * cancellation and task sources.
   */
  for (i = 0; i < 5; i++)
    {
      xcancellable_t *cancellable;
      xtask_t *task;
      xboolean_t notification_emitted = FALSE;
      xuint_t run_task_id;

      cancellable = xcancellable_new ();

      task = xtask_new (NULL, cancellable, asynchronous_cancellation_callback, NULL);
      xcancellable_connect (cancellable, (xcallback_t) asynchronous_cancellation_cancelled, task, NULL);
      xsignal_connect (task, "notify::completed", (xcallback_t) completed_cb, &notification_emitted);

      run_task_id = g_idle_add (asynchronous_cancellation_run_task, task);
      xsource_set_name_by_id (run_task_id, "[test_asynchronous_cancellation] run_task");
      xtask_set_task_data (task, GUINT_TO_POINTER (run_task_id), NULL);

      g_timeout_add (50, asynchronous_cancellation_cancel_task, task);

      xmain_loop_run (loop);

      g_assert_true (xtask_get_completed (task));
      g_assert_true (notification_emitted);

      xobject_unref (cancellable);
      xobject_unref (task);
    }
}

/* test_check_cancellable: cancellation overrides return value */

enum {
  CANCEL_BEFORE     = (1 << 1),
  CANCEL_AFTER      = (1 << 2),
  CHECK_CANCELLABLE = (1 << 3)
};
#define NUM_CANCEL_TESTS (CANCEL_BEFORE | CANCEL_AFTER | CHECK_CANCELLABLE)

static void
cancel_callback (xobject_t      *object,
                 xasync_result_t *result,
                 xpointer_t      user_data)
{
  int state = GPOINTER_TO_INT (user_data);
  xtask_t *task;
  xcancellable_t *cancellable;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);

  task = XTASK (result);
  cancellable = xtask_get_cancellable (task);
  xassert (X_IS_CANCELLABLE (cancellable));

  if (state & (CANCEL_BEFORE | CANCEL_AFTER))
    xassert (xcancellable_is_cancelled (cancellable));
  else
    xassert (!xcancellable_is_cancelled (cancellable));

  if (state & CHECK_CANCELLABLE)
    xassert (xtask_get_check_cancellable (task));
  else
    xassert (!xtask_get_check_cancellable (task));

  if (xtask_propagate_boolean (task, &error))
    {
      xassert (!xcancellable_is_cancelled (cancellable) ||
                !xtask_get_check_cancellable (task));
    }
  else
    {
      xassert (xcancellable_is_cancelled (cancellable) &&
                xtask_get_check_cancellable (task));
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
      xerror_free (error);
    }

  xmain_loop_quit (loop);
}

static void
test_check_cancellable (void)
{
  xtask_t *task;
  xcancellable_t *cancellable;
  int state;

  cancellable = xcancellable_new ();

  for (state = 0; state <= NUM_CANCEL_TESTS; state++)
    {
      task = xtask_new (NULL, cancellable, cancel_callback,
                         GINT_TO_POINTER (state));
      xtask_set_check_cancellable (task, (state & CHECK_CANCELLABLE) != 0);

      if (state & CANCEL_BEFORE)
        xcancellable_cancel (cancellable);
      xtask_return_boolean (task, TRUE);
      if (state & CANCEL_AFTER)
        xcancellable_cancel (cancellable);

      xmain_loop_run (loop);
      xobject_unref (task);
      xcancellable_reset (cancellable);
    }

  xobject_unref (cancellable);
}

/* test_return_if_cancelled */

static void
return_if_cancelled_callback (xobject_t      *object,
                              xasync_result_t *result,
                              xpointer_t      user_data)
{
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  xtask_propagate_boolean (XTASK (result), &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);

  xassert (xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static void
test_return_if_cancelled (void)
{
  xtask_t *task;
  xcancellable_t *cancellable;
  xboolean_t cancelled;
  xboolean_t notification_emitted = FALSE;

  cancellable = xcancellable_new ();

  task = xtask_new (NULL, cancellable, return_if_cancelled_callback, NULL);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xcancellable_cancel (cancellable);
  cancelled = xtask_return_error_if_cancelled (task);
  xassert (cancelled);
  g_assert_false (notification_emitted);
  xmain_loop_run (loop);
  xobject_unref (task);
  g_assert_true (notification_emitted);
  xcancellable_reset (cancellable);

  notification_emitted = FALSE;

  task = xtask_new (NULL, cancellable, return_if_cancelled_callback, NULL);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xtask_set_check_cancellable (task, FALSE);
  xcancellable_cancel (cancellable);
  cancelled = xtask_return_error_if_cancelled (task);
  xassert (cancelled);
  g_assert_false (notification_emitted);
  xmain_loop_run (loop);
  xobject_unref (task);
  g_assert_true (notification_emitted);
  xobject_unref (cancellable);
}

/* test_run_in_thread */

static xmutex_t run_in_thread_mutex;
static xcond_t run_in_thread_cond;

static void
task_weak_notify (xpointer_t  user_data,
                  xobject_t  *ex_task)
{
  xboolean_t *weak_notify_ran = user_data;

  g_mutex_lock (&run_in_thread_mutex);
  g_atomic_int_set (weak_notify_ran, TRUE);
  g_cond_signal (&run_in_thread_cond);
  g_mutex_unlock (&run_in_thread_mutex);
}

static void
run_in_thread_callback (xobject_t      *object,
                        xasync_result_t *result,
                        xpointer_t      user_data)
{
  xboolean_t *done = user_data;
  xerror_t *error = NULL;
  xssize_t ret;

  xassert (xthread_self () == main_thread);

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));
  g_assert_cmpstr (xtask_get_name (XTASK (result)), ==, "test_run_in_thread name");

  ret = xtask_propagate_int (XTASK (result), &error);
  g_assert_no_error (error);
  g_assert_cmpint (ret, ==, magic);

  xassert (!xtask_had_error (XTASK (result)));

  *done = TRUE;
  xmain_loop_quit (loop);
}

static void
run_in_thread_thread (xtask_t        *task,
                      xpointer_t      source_object,
                      xpointer_t      task_data,
                      xcancellable_t *cancellable)
{
  xboolean_t *thread_ran = task_data;

  xassert (source_object == xtask_get_source_object (task));
  xassert (task_data == xtask_get_task_data (task));
  xassert (cancellable == xtask_get_cancellable (task));
  g_assert_false (xtask_get_completed (task));
  g_assert_cmpstr (xtask_get_name (task), ==, "test_run_in_thread name");

  xassert (xthread_self () != main_thread);

  g_mutex_lock (&run_in_thread_mutex);
  g_atomic_int_set (thread_ran, TRUE);
  g_cond_signal (&run_in_thread_cond);
  g_mutex_unlock (&run_in_thread_mutex);

  xtask_return_int (task, magic);
}

static void
test_run_in_thread (void)
{
  xtask_t *task;
  xboolean_t thread_ran = FALSE;  /* (atomic) */
  xboolean_t weak_notify_ran = FALSE;  /* (atomic) */
  xboolean_t notification_emitted = FALSE;
  xboolean_t done = FALSE;

  task = xtask_new (NULL, NULL, run_in_thread_callback, &done);
  xtask_set_name (task, "test_run_in_thread name");
  xobject_weak_ref (G_OBJECT (task), task_weak_notify, (xpointer_t)&weak_notify_ran);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xtask_set_task_data (task, (xpointer_t)&thread_ran, NULL);
  xtask_run_in_thread (task, run_in_thread_thread);

  g_mutex_lock (&run_in_thread_mutex);
  while (!g_atomic_int_get (&thread_ran))
    g_cond_wait (&run_in_thread_cond, &run_in_thread_mutex);
  g_mutex_unlock (&run_in_thread_mutex);

  xassert (done == FALSE);
  g_assert_false (g_atomic_int_get (&weak_notify_ran));

  xmain_loop_run (loop);

  xassert (done == TRUE);
  g_assert_true (notification_emitted);

  g_assert_cmpstr (xtask_get_name (task), ==, "test_run_in_thread name");

  xobject_unref (task);

  g_mutex_lock (&run_in_thread_mutex);
  while (!g_atomic_int_get (&weak_notify_ran))
    g_cond_wait (&run_in_thread_cond, &run_in_thread_mutex);
  g_mutex_unlock (&run_in_thread_mutex);
}

/* test_run_in_thread_sync */

static void
run_in_thread_sync_callback (xobject_t      *object,
                             xasync_result_t *result,
                             xpointer_t      user_data)
{
  /* xtask_run_in_thread_sync() does not invoke the task's callback */
  g_assert_not_reached ();
}

static void
run_in_thread_sync_thread (xtask_t        *task,
                           xpointer_t      source_object,
                           xpointer_t      task_data,
                           xcancellable_t *cancellable)
{
  xboolean_t *thread_ran = task_data;

  xassert (source_object == xtask_get_source_object (task));
  xassert (task_data == xtask_get_task_data (task));
  xassert (cancellable == xtask_get_cancellable (task));
  g_assert_false (xtask_get_completed (task));

  xassert (xthread_self () != main_thread);

  g_atomic_int_set (thread_ran, TRUE);
  xtask_return_int (task, magic);
}

static void
test_run_in_thread_sync (void)
{
  xtask_t *task;
  xboolean_t thread_ran = FALSE;
  xssize_t ret;
  xboolean_t notification_emitted = FALSE;
  xerror_t *error = NULL;

  task = xtask_new (NULL, NULL, run_in_thread_sync_callback, NULL);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb,
                    &notification_emitted);

  xtask_set_task_data (task, &thread_ran, NULL);
  xtask_run_in_thread_sync (task, run_in_thread_sync_thread);

  g_assert_true (g_atomic_int_get (&thread_ran));
  xassert (task != NULL);
  xassert (!xtask_had_error (task));
  g_assert_true (xtask_get_completed (task));
  g_assert_true (notification_emitted);

  ret = xtask_propagate_int (task, &error);
  g_assert_no_error (error);
  g_assert_cmpint (ret, ==, magic);

  xassert (!xtask_had_error (task));

  xobject_unref (task);
}

/* test_run_in_thread_priority */

static xmutex_t fake_task_mutex, last_fake_task_mutex;
static xint_t sequence_number = 0;

static void
quit_main_loop_callback (xobject_t      *object,
                         xasync_result_t *result,
                         xpointer_t      user_data)
{
  xerror_t *error = NULL;
  xboolean_t ret;

  xassert (xthread_self () == main_thread);

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  ret = xtask_propagate_boolean (XTASK (result), &error);
  g_assert_no_error (error);
  g_assert_cmpint (ret, ==, TRUE);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static void
set_sequence_number_thread (xtask_t        *task,
                            xpointer_t      source_object,
                            xpointer_t      task_data,
                            xcancellable_t *cancellable)
{
  xint_t *seq_no_p = task_data;

  *seq_no_p = ++sequence_number;
  xtask_return_boolean (task, TRUE);
}

static void
fake_task_thread (xtask_t        *task,
                  xpointer_t      source_object,
                  xpointer_t      task_data,
                  xcancellable_t *cancellable)
{
  xmutex_t *mutex = task_data;

  g_mutex_lock (mutex);
  g_mutex_unlock (mutex);
  xtask_return_boolean (task, TRUE);
}

#define XTASK_THREAD_POOL_SIZE 10
static int fake_tasks_running;

static void
fake_task_callback (xobject_t      *source,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  if (--fake_tasks_running == 0)
    xmain_loop_quit (loop);
}

static void
clog_up_thread_pool (void)
{
  xtask_t *task;
  int i;

  xthread_pool_stop_unused_threads ();

  g_mutex_lock (&fake_task_mutex);
  for (i = 0; i < XTASK_THREAD_POOL_SIZE - 1; i++)
    {
      task = xtask_new (NULL, NULL, fake_task_callback, NULL);
      xtask_set_task_data (task, &fake_task_mutex, NULL);
      g_assert_cmpint (xtask_get_priority (task), ==, G_PRIORITY_DEFAULT);
      xtask_set_priority (task, G_PRIORITY_HIGH * 2);
      g_assert_cmpint (xtask_get_priority (task), ==, G_PRIORITY_HIGH * 2);
      xtask_run_in_thread (task, fake_task_thread);
      xobject_unref (task);
      fake_tasks_running++;
    }

  g_mutex_lock (&last_fake_task_mutex);
  task = xtask_new (NULL, NULL, NULL, NULL);
  xtask_set_task_data (task, &last_fake_task_mutex, NULL);
  xtask_set_priority (task, G_PRIORITY_HIGH * 2);
  xtask_run_in_thread (task, fake_task_thread);
  xobject_unref (task);
}

static void
uncloxthread_pool (void)
{
  g_mutex_unlock (&fake_task_mutex);
  xmain_loop_run (loop);
}

static void
test_run_in_thread_priority (void)
{
  xtask_t *task;
  xcancellable_t *cancellable;
  int seq_a, seq_b, seq_c, seq_d;

  clog_up_thread_pool ();

  /* Queue three more tasks that we'll arrange to have run serially */
  task = xtask_new (NULL, NULL, NULL, NULL);
  xtask_set_task_data (task, &seq_a, NULL);
  xtask_run_in_thread (task, set_sequence_number_thread);
  xobject_unref (task);

  task = xtask_new (NULL, NULL, quit_main_loop_callback, NULL);
  xtask_set_task_data (task, &seq_b, NULL);
  xtask_set_priority (task, G_PRIORITY_LOW);
  xtask_run_in_thread (task, set_sequence_number_thread);
  xobject_unref (task);

  task = xtask_new (NULL, NULL, NULL, NULL);
  xtask_set_task_data (task, &seq_c, NULL);
  xtask_set_priority (task, G_PRIORITY_HIGH);
  xtask_run_in_thread (task, set_sequence_number_thread);
  xobject_unref (task);

  cancellable = xcancellable_new ();
  task = xtask_new (NULL, cancellable, NULL, NULL);
  xtask_set_task_data (task, &seq_d, NULL);
  xtask_run_in_thread (task, set_sequence_number_thread);
  xcancellable_cancel (cancellable);
  xobject_unref (cancellable);
  xobject_unref (task);

  /* Let the last fake task complete; the four other tasks will then
   * complete serially, in the order D, C, A, B, and B will quit the
   * main loop.
   */
  g_mutex_unlock (&last_fake_task_mutex);
  xmain_loop_run (loop);

  g_assert_cmpint (seq_d, ==, 1);
  g_assert_cmpint (seq_c, ==, 2);
  g_assert_cmpint (seq_a, ==, 3);
  g_assert_cmpint (seq_b, ==, 4);

  uncloxthread_pool ();
}

/* test_run_in_thread_nested: task threads that block waiting on
 * other task threads will not cause the thread pool to starve.
 */

static void
run_nested_task_thread (xtask_t        *task,
                        xpointer_t      source_object,
                        xpointer_t      task_data,
                        xcancellable_t *cancellable)
{
  xtask_t *nested;
  int *nested_tasks_left = task_data;

  if ((*nested_tasks_left)--)
    {
      nested = xtask_new (NULL, NULL, NULL, NULL);
      xtask_set_task_data (nested, nested_tasks_left, NULL);
      xtask_run_in_thread_sync (nested, run_nested_task_thread);
      xobject_unref (nested);
    }

  xtask_return_boolean (task, TRUE);
}

static void
test_run_in_thread_nested (void)
{
  xtask_t *task;
  int nested_tasks_left = 2;

  clog_up_thread_pool ();

  task = xtask_new (NULL, NULL, quit_main_loop_callback, NULL);
  xtask_set_task_data (task, &nested_tasks_left, NULL);
  xtask_run_in_thread (task, run_nested_task_thread);
  xobject_unref (task);

  g_mutex_unlock (&last_fake_task_mutex);
  xmain_loop_run (loop);

  uncloxthread_pool ();
}

/* test_run_in_thread_overflow: if you queue lots and lots and lots of
 * tasks, they won't all run at once.
 */
static xmutex_t overflow_mutex;
static xuint_t overflow_completed;

static void
run_overflow_task_thread (xtask_t        *task,
                          xpointer_t      source_object,
                          xpointer_t      task_data,
                          xcancellable_t *cancellable)
{
  xchar_t *result = task_data;

  if (xtask_return_error_if_cancelled (task))
    {
      *result = 'X';
    }
  else
    {
      /* Block until the main thread is ready. */
      g_mutex_lock (&overflow_mutex);
      g_mutex_unlock (&overflow_mutex);

      *result = '.';

      xtask_return_boolean (task, TRUE);
    }

  g_atomic_int_inc (&overflow_completed);
}

#define NUM_OVERFLOW_TASKS 1024

static void
test_run_in_thread_overflow (void)
{
  xcancellable_t *cancellable;
  xtask_t *task;
  xchar_t buf[NUM_OVERFLOW_TASKS + 1];
  xint_t i;

  /* Queue way too many tasks and then sleep for a bit. The first 10
   * tasks will be dispatched to threads and will then block on
   * overflow_mutex, so more threads will be created while this thread
   * is sleeping. Then we cancel the cancellable, unlock the mutex,
   * wait for all of the tasks to complete, and make sure that we got
   * the behavior we expected.
   */

  memset (buf, 0, sizeof (buf));
  cancellable = xcancellable_new ();

  g_mutex_lock (&overflow_mutex);

  for (i = 0; i < NUM_OVERFLOW_TASKS; i++)
    {
      task = xtask_new (NULL, cancellable, NULL, NULL);
      xtask_set_task_data (task, buf + i, NULL);
      xtask_run_in_thread (task, run_overflow_task_thread);
      xobject_unref (task);
    }

  if (g_test_slow ())
    g_usleep (5000000); /* 5 s */
  else
    g_usleep (500000);  /* 0.5 s */
  xcancellable_cancel (cancellable);
  xobject_unref (cancellable);

  g_mutex_unlock (&overflow_mutex);

  /* Wait for all tasks to complete. */
  while (g_atomic_int_get (&overflow_completed) != NUM_OVERFLOW_TASKS)
    g_usleep (1000);

  g_assert_cmpint (strlen (buf), ==, NUM_OVERFLOW_TASKS);

  i = strspn (buf, ".");
  /* Given the sleep times above, i should be 14 for normal, 40 for
   * slow. But if the machine is too slow/busy then the scheduling
   * might get messed up and we'll get more or fewer threads than
   * expected. But there are limits to how messed up it could
   * plausibly get (and we hope that if gtask is actually broken then
   * it will exceed those limits).
   */
  g_assert_cmpint (i, >=, 10);
  if (g_test_slow ())
    g_assert_cmpint (i, <, 50);
  else
    g_assert_cmpint (i, <, 20);

  g_assert_cmpint (i + strspn (buf + i, "X"), ==, NUM_OVERFLOW_TASKS);
}

/* test_return_on_cancel */

xmutex_t roc_init_mutex, roc_finish_mutex;
xcond_t roc_init_cond, roc_finish_cond;

typedef enum {
  THREAD_STARTING,
  THREAD_RUNNING,
  THREAD_CANCELLED,
  THREAD_COMPLETED
} ThreadState;

static void
return_on_cancel_callback (xobject_t      *object,
                           xasync_result_t *result,
                           xpointer_t      user_data)
{
  xboolean_t *callback_ran = user_data;
  xerror_t *error = NULL;
  xssize_t ret;

  xassert (xthread_self () == main_thread);

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  ret = xtask_propagate_int (XTASK (result), &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_assert_cmpint (ret, ==, -1);

  xassert (xtask_had_error (XTASK (result)));

  *callback_ran = TRUE;
  xmain_loop_quit (loop);
}

static void
return_on_cancel_thread (xtask_t        *task,
                         xpointer_t      source_object,
                         xpointer_t      task_data,
                         xcancellable_t *cancellable)
{
  ThreadState *state = task_data;

  xassert (source_object == xtask_get_source_object (task));
  xassert (task_data == xtask_get_task_data (task));
  xassert (cancellable == xtask_get_cancellable (task));

  xassert (xthread_self () != main_thread);

  g_mutex_lock (&roc_init_mutex);
  *state = THREAD_RUNNING;
  g_cond_signal (&roc_init_cond);
  g_mutex_unlock (&roc_init_mutex);

  g_mutex_lock (&roc_finish_mutex);

  if (!xtask_get_return_on_cancel (task) ||
      xtask_set_return_on_cancel (task, FALSE))
    {
      *state = THREAD_COMPLETED;
      xtask_return_int (task, magic);
    }
  else
    *state = THREAD_CANCELLED;

  g_cond_signal (&roc_finish_cond);
  g_mutex_unlock (&roc_finish_mutex);
}

static void
test_return_on_cancel (void)
{
  xtask_t *task;
  xcancellable_t *cancellable;
  ThreadState thread_state;  /* (atomic) */
  xboolean_t weak_notify_ran = FALSE;  /* (atomic) */
  xboolean_t callback_ran;
  xboolean_t notification_emitted = FALSE;

  cancellable = xcancellable_new ();

  /* If return-on-cancel is FALSE (default), the task does not return
   * early.
   */
  callback_ran = FALSE;
  g_atomic_int_set (&thread_state, THREAD_STARTING);
  task = xtask_new (NULL, cancellable, return_on_cancel_callback, &callback_ran);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xtask_set_task_data (task, (xpointer_t)&thread_state, NULL);
  g_mutex_lock (&roc_init_mutex);
  g_mutex_lock (&roc_finish_mutex);
  xtask_run_in_thread (task, return_on_cancel_thread);
  xobject_unref (task);

  while (g_atomic_int_get (&thread_state) == THREAD_STARTING)
    g_cond_wait (&roc_init_cond, &roc_init_mutex);
  g_mutex_unlock (&roc_init_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);
  xassert (callback_ran == FALSE);

  xcancellable_cancel (cancellable);
  g_mutex_unlock (&roc_finish_mutex);
  xmain_loop_run (loop);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_COMPLETED);
  xassert (callback_ran == TRUE);
  g_assert_true (notification_emitted);

  xcancellable_reset (cancellable);

  /* If return-on-cancel is TRUE, it does return early */
  callback_ran = FALSE;
  notification_emitted = FALSE;
  g_atomic_int_set (&thread_state, THREAD_STARTING);
  task = xtask_new (NULL, cancellable, return_on_cancel_callback, &callback_ran);
  xobject_weak_ref (G_OBJECT (task), task_weak_notify, (xpointer_t)&weak_notify_ran);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);
  xtask_set_return_on_cancel (task, TRUE);

  xtask_set_task_data (task, (xpointer_t)&thread_state, NULL);
  g_mutex_lock (&roc_init_mutex);
  g_mutex_lock (&roc_finish_mutex);
  xtask_run_in_thread (task, return_on_cancel_thread);
  xobject_unref (task);

  while (g_atomic_int_get (&thread_state) == THREAD_STARTING)
    g_cond_wait (&roc_init_cond, &roc_init_mutex);
  g_mutex_unlock (&roc_init_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);
  xassert (callback_ran == FALSE);

  xcancellable_cancel (cancellable);
  xmain_loop_run (loop);
  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);
  xassert (callback_ran == TRUE);

  g_assert_false (g_atomic_int_get (&weak_notify_ran));

  while (g_atomic_int_get (&thread_state) == THREAD_RUNNING)
    g_cond_wait (&roc_finish_cond, &roc_finish_mutex);
  g_mutex_unlock (&roc_finish_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_CANCELLED);
  g_mutex_lock (&run_in_thread_mutex);
  while (!g_atomic_int_get (&weak_notify_ran))
    g_cond_wait (&run_in_thread_cond, &run_in_thread_mutex);
  g_mutex_unlock (&run_in_thread_mutex);

  g_assert_true (notification_emitted);
  xcancellable_reset (cancellable);

  /* If the task is already cancelled before it starts, it returns
   * immediately, but the thread func still runs.
   */
  callback_ran = FALSE;
  notification_emitted = FALSE;
  g_atomic_int_set (&thread_state, THREAD_STARTING);
  task = xtask_new (NULL, cancellable, return_on_cancel_callback, &callback_ran);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);
  xtask_set_return_on_cancel (task, TRUE);

  xcancellable_cancel (cancellable);

  xtask_set_task_data (task, (xpointer_t)&thread_state, NULL);
  g_mutex_lock (&roc_init_mutex);
  g_mutex_lock (&roc_finish_mutex);
  xtask_run_in_thread (task, return_on_cancel_thread);
  xobject_unref (task);

  xmain_loop_run (loop);
  xassert (callback_ran == TRUE);

  while (g_atomic_int_get (&thread_state) == THREAD_STARTING)
    g_cond_wait (&roc_init_cond, &roc_init_mutex);
  g_mutex_unlock (&roc_init_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);

  while (g_atomic_int_get (&thread_state) == THREAD_RUNNING)
    g_cond_wait (&roc_finish_cond, &roc_finish_mutex);
  g_mutex_unlock (&roc_finish_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_CANCELLED);
  g_assert_true (notification_emitted);

  xobject_unref (cancellable);
}

/* test_return_on_cancel_sync */

static xpointer_t
cancel_sync_runner_thread (xpointer_t task)
{
  xtask_run_in_thread_sync (task, return_on_cancel_thread);
  return NULL;
}

static void
test_return_on_cancel_sync (void)
{
  xtask_t *task;
  xcancellable_t *cancellable;
  ThreadState thread_state;  /* (atomic) */
  xthread_t *runner_thread;
  xssize_t ret;
  xerror_t *error = NULL;

  cancellable = xcancellable_new ();

  /* If return-on-cancel is FALSE, the task does not return early.
   */
  g_atomic_int_set (&thread_state, THREAD_STARTING);
  task = xtask_new (NULL, cancellable, run_in_thread_sync_callback, NULL);

  xtask_set_task_data (task, (xpointer_t)&thread_state, NULL);
  g_mutex_lock (&roc_init_mutex);
  g_mutex_lock (&roc_finish_mutex);
  runner_thread = xthread_new ("return-on-cancel-sync runner thread",
                                cancel_sync_runner_thread, task);

  while (g_atomic_int_get (&thread_state) == THREAD_STARTING)
    g_cond_wait (&roc_init_cond, &roc_init_mutex);
  g_mutex_unlock (&roc_init_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);

  xcancellable_cancel (cancellable);
  g_mutex_unlock (&roc_finish_mutex);
  xthread_join (runner_thread);
  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_COMPLETED);

  ret = xtask_propagate_int (task, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_assert_cmpint (ret, ==, -1);

  xobject_unref (task);

  xcancellable_reset (cancellable);

  /* If return-on-cancel is TRUE, it does return early */
  g_atomic_int_set (&thread_state, THREAD_STARTING);
  task = xtask_new (NULL, cancellable, run_in_thread_sync_callback, NULL);
  xtask_set_return_on_cancel (task, TRUE);

  xtask_set_task_data (task, (xpointer_t)&thread_state, NULL);
  g_mutex_lock (&roc_init_mutex);
  g_mutex_lock (&roc_finish_mutex);
  runner_thread = xthread_new ("return-on-cancel-sync runner thread",
                                cancel_sync_runner_thread, task);

  while (g_atomic_int_get (&thread_state) == THREAD_STARTING)
    g_cond_wait (&roc_init_cond, &roc_init_mutex);
  g_mutex_unlock (&roc_init_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);

  xcancellable_cancel (cancellable);
  xthread_join (runner_thread);
  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);

  ret = xtask_propagate_int (task, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_assert_cmpint (ret, ==, -1);

  xobject_unref (task);

  while (g_atomic_int_get (&thread_state) == THREAD_RUNNING)
    g_cond_wait (&roc_finish_cond, &roc_finish_mutex);
  g_mutex_unlock (&roc_finish_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_CANCELLED);

  xcancellable_reset (cancellable);

  /* If the task is already cancelled before it starts, it returns
   * immediately, but the thread func still runs.
   */
  g_atomic_int_set (&thread_state, THREAD_STARTING);
  task = xtask_new (NULL, cancellable, run_in_thread_sync_callback, NULL);
  xtask_set_return_on_cancel (task, TRUE);

  xcancellable_cancel (cancellable);

  xtask_set_task_data (task, (xpointer_t)&thread_state, NULL);
  g_mutex_lock (&roc_init_mutex);
  g_mutex_lock (&roc_finish_mutex);
  runner_thread = xthread_new ("return-on-cancel-sync runner thread",
                                cancel_sync_runner_thread, task);

  xthread_join (runner_thread);
  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_STARTING);

  ret = xtask_propagate_int (task, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_assert_cmpint (ret, ==, -1);

  xobject_unref (task);

  while (g_atomic_int_get (&thread_state) == THREAD_STARTING)
    g_cond_wait (&roc_init_cond, &roc_init_mutex);
  g_mutex_unlock (&roc_init_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_RUNNING);

  while (g_atomic_int_get (&thread_state) == THREAD_RUNNING)
    g_cond_wait (&roc_finish_cond, &roc_finish_mutex);
  g_mutex_unlock (&roc_finish_mutex);

  g_assert_cmpint (g_atomic_int_get (&thread_state), ==, THREAD_CANCELLED);

  xobject_unref (cancellable);
}

/* test_return_on_cancel_atomic: turning return-on-cancel on/off is
 * non-racy
 */

xmutex_t roca_mutex_1, roca_mutex_2;
xcond_t roca_cond_1, roca_cond_2;

static void
return_on_cancel_atomic_callback (xobject_t      *object,
                                  xasync_result_t *result,
                                  xpointer_t      user_data)
{
  xboolean_t *callback_ran = user_data;
  xerror_t *error = NULL;
  xssize_t ret;

  xassert (xthread_self () == main_thread);

  xassert (object == NULL);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  ret = xtask_propagate_int (XTASK (result), &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_assert_cmpint (ret, ==, -1);

  xassert (xtask_had_error (XTASK (result)));

  *callback_ran = TRUE;
  xmain_loop_quit (loop);
}

static void
return_on_cancel_atomic_thread (xtask_t        *task,
                                xpointer_t      source_object,
                                xpointer_t      task_data,
                                xcancellable_t *cancellable)
{
  xint_t *state = task_data;  /* (atomic) */

  xassert (source_object == xtask_get_source_object (task));
  xassert (task_data == xtask_get_task_data (task));
  xassert (cancellable == xtask_get_cancellable (task));
  g_assert_false (xtask_get_completed (task));

  xassert (xthread_self () != main_thread);
  g_assert_cmpint (g_atomic_int_get (state), ==, 0);

  g_mutex_lock (&roca_mutex_1);
  g_atomic_int_set (state, 1);
  g_cond_signal (&roca_cond_1);
  g_mutex_unlock (&roca_mutex_1);

  g_mutex_lock (&roca_mutex_2);
  if (xtask_set_return_on_cancel (task, FALSE))
    g_atomic_int_set (state, 2);
  else
    g_atomic_int_set (state, 3);
  g_cond_signal (&roca_cond_2);
  g_mutex_unlock (&roca_mutex_2);

  g_mutex_lock (&roca_mutex_1);
  if (xtask_set_return_on_cancel (task, TRUE))
    g_atomic_int_set (state, 4);
  else
    g_atomic_int_set (state, 5);
  g_cond_signal (&roca_cond_1);
  g_mutex_unlock (&roca_mutex_1);

  g_mutex_lock (&roca_mutex_2);
  if (xtask_set_return_on_cancel (task, TRUE))
    g_atomic_int_set (state, 6);
  else
    g_atomic_int_set (state, 7);
  g_cond_signal (&roca_cond_2);
  g_mutex_unlock (&roca_mutex_2);

  xtask_return_int (task, magic);
}

static void
test_return_on_cancel_atomic (void)
{
  xtask_t *task;
  xcancellable_t *cancellable;
  xint_t state;  /* (atomic) */
  xboolean_t notification_emitted = FALSE;
  xboolean_t callback_ran;

  cancellable = xcancellable_new ();
  g_mutex_lock (&roca_mutex_1);
  g_mutex_lock (&roca_mutex_2);

  /* If we don't cancel it, each set_return_on_cancel() call will succeed */
  g_atomic_int_set (&state, 0);
  callback_ran = FALSE;
  task = xtask_new (NULL, cancellable, return_on_cancel_atomic_callback, &callback_ran);
  xtask_set_return_on_cancel (task, TRUE);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xtask_set_task_data (task, (xpointer_t)&state, NULL);
  xtask_run_in_thread (task, return_on_cancel_atomic_thread);
  xobject_unref (task);

  g_assert_cmpint (g_atomic_int_get (&state), ==, 0);

  while (g_atomic_int_get (&state) == 0)
    g_cond_wait (&roca_cond_1, &roca_mutex_1);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 1);

  while (g_atomic_int_get (&state) == 1)
    g_cond_wait (&roca_cond_2, &roca_mutex_2);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 2);

  while (g_atomic_int_get (&state) == 2)
    g_cond_wait (&roca_cond_1, &roca_mutex_1);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 4);

  while (g_atomic_int_get (&state) == 4)
    g_cond_wait (&roca_cond_2, &roca_mutex_2);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 6);

  /* callback assumes there'll be a cancelled error */
  xcancellable_cancel (cancellable);

  xassert (callback_ran == FALSE);
  xmain_loop_run (loop);
  xassert (callback_ran == TRUE);
  g_assert_true (notification_emitted);

  xcancellable_reset (cancellable);


  /* If we cancel while it's temporarily not return-on-cancel, the
   * task won't complete right away, and further
   * xtask_set_return_on_cancel() calls will return FALSE.
   */
  g_atomic_int_set (&state, 0);
  callback_ran = FALSE;
  notification_emitted = FALSE;
  task = xtask_new (NULL, cancellable, return_on_cancel_atomic_callback, &callback_ran);
  xtask_set_return_on_cancel (task, TRUE);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  xtask_set_task_data (task, (xpointer_t)&state, NULL);
  xtask_run_in_thread (task, return_on_cancel_atomic_thread);

  g_assert_cmpint (g_atomic_int_get (&state), ==, 0);

  while (g_atomic_int_get (&state) == 0)
    g_cond_wait (&roca_cond_1, &roca_mutex_1);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 1);
  xassert (xtask_get_return_on_cancel (task));

  while (g_atomic_int_get (&state) == 1)
    g_cond_wait (&roca_cond_2, &roca_mutex_2);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 2);
  xassert (!xtask_get_return_on_cancel (task));

  xcancellable_cancel (cancellable);
  g_idle_add (idle_quit_loop, NULL);
  xmain_loop_run (loop);
  xassert (callback_ran == FALSE);

  while (g_atomic_int_get (&state) == 2)
    g_cond_wait (&roca_cond_1, &roca_mutex_1);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 5);
  xassert (!xtask_get_return_on_cancel (task));

  xmain_loop_run (loop);
  xassert (callback_ran == TRUE);
  g_assert_true (notification_emitted);

  while (g_atomic_int_get (&state) == 5)
    g_cond_wait (&roca_cond_2, &roca_mutex_2);
  g_assert_cmpint (g_atomic_int_get (&state), ==, 7);

  xobject_unref (cancellable);
  g_mutex_unlock (&roca_mutex_1);
  g_mutex_unlock (&roca_mutex_2);
  xobject_unref (task);
}

/* test_return_pointer: memory management of pointer returns */

static void
test_return_pointer (void)
{
  xobject_t *object, *ret;
  xtask_t *task;
  xcancellable_t *cancellable;
  xerror_t *error = NULL;

  /* If we don't read back the return value, the task will
   * run its destroy notify.
   */
  object = (xobject_t *)g_dummy_object_new ();
  g_assert_cmpint (object->ref_count, ==, 1);
  xobject_add_weak_pointer (object, (xpointer_t *)&object);

  task = xtask_new (NULL, NULL, NULL, NULL);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xtask_return_pointer (task, object, xobject_unref);
  g_assert_cmpint (object->ref_count, ==, 1);

  /* Task and object are reffed until the :completed notification in idle. */
  xobject_unref (task);
  g_assert_nonnull (task);
  g_assert_nonnull (object);

  wait_for_completed_notification (task);

  g_assert_null (task);
  g_assert_null (object);

  /* Likewise, if the return value is overwritten by an error */
  object = (xobject_t *)g_dummy_object_new ();
  g_assert_cmpint (object->ref_count, ==, 1);
  xobject_add_weak_pointer (object, (xpointer_t *)&object);

  cancellable = xcancellable_new ();
  task = xtask_new (NULL, cancellable, NULL, NULL);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xtask_return_pointer (task, object, xobject_unref);
  g_assert_cmpint (object->ref_count, ==, 1);
  xcancellable_cancel (cancellable);
  g_assert_cmpint (object->ref_count, ==, 1);

  ret = xtask_propagate_pointer (task, &error);
  xassert (ret == NULL);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);
  g_assert_cmpint (object->ref_count, ==, 1);

  xobject_unref (task);
  xobject_unref (cancellable);
  g_assert_nonnull (task);
  g_assert_nonnull (object);

  wait_for_completed_notification (task);

  g_assert_null (task);
  g_assert_null (object);

  /* If we read back the return value, we steal its ref */
  object = (xobject_t *)g_dummy_object_new ();
  g_assert_cmpint (object->ref_count, ==, 1);
  xobject_add_weak_pointer (object, (xpointer_t *)&object);

  task = xtask_new (NULL, NULL, NULL, NULL);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xtask_return_pointer (task, object, xobject_unref);
  g_assert_cmpint (object->ref_count, ==, 1);

  ret = xtask_propagate_pointer (task, &error);
  g_assert_no_error (error);
  xassert (ret == object);
  g_assert_cmpint (object->ref_count, ==, 1);

  xobject_unref (task);
  g_assert_nonnull (task);
  g_assert_cmpint (object->ref_count, ==, 1);
  xobject_unref (object);
  xassert (object == NULL);

  wait_for_completed_notification (task);
  g_assert_null (task);
}

static void
test_return_value (void)
{
  xobject_t *object;
  xvalue_t value = G_VALUE_INIT;
  xvalue_t ret = G_VALUE_INIT;
  xtask_t *task;
  xerror_t *error = NULL;

  object = (xobject_t *)g_dummy_object_new ();
  g_assert_cmpint (object->ref_count, ==, 1);
  xobject_add_weak_pointer (object, (xpointer_t *)&object);

  xvalue_init (&value, XTYPE_OBJECT);
  xvalue_set_object (&value, object);
  g_assert_cmpint (object->ref_count, ==, 2);

  task = xtask_new (NULL, NULL, NULL, NULL);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xtask_return_value (task, &value);
  g_assert_cmpint (object->ref_count, ==, 3);

  g_assert_true (xtask_propagate_value (task, &ret, &error));
  g_assert_no_error (error);
  g_assert_true (xvalue_get_object (&ret) == object);
  g_assert_cmpint (object->ref_count, ==, 3);

  xobject_unref (task);
  g_assert_nonnull (task);
  wait_for_completed_notification (task);
  g_assert_null (task);

  g_assert_cmpint (object->ref_count, ==, 3);
  xvalue_unset (&ret);
  g_assert_cmpint (object->ref_count, ==, 2);
  xvalue_unset (&value);
  g_assert_cmpint (object->ref_count, ==, 1);
  xobject_unref (object);
  g_assert_null (object);
}

/* test_object_keepalive: xtask_t takes a ref on its source object */

static xobject_t *keepalive_object;

static void
keepalive_callback (xobject_t      *object,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  xssize_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == keepalive_object);
  xassert (xtask_is_valid (result, object));
  xassert (xasync_result_get_user_data (result) == user_data);
  xassert (!xtask_had_error (XTASK (result)));
  g_assert_false (xtask_get_completed (XTASK (result)));

  *result_out = xtask_propagate_int (XTASK (result), &error);
  g_assert_no_error (error);

  xassert (!xtask_had_error (XTASK (result)));

  xmain_loop_quit (loop);
}

static void
test_object_keepalive (void)
{
  xobject_t *object;
  xtask_t *task;
  xssize_t result;
  int ref_count;
  xboolean_t notification_emitted = FALSE;

  keepalive_object = object = (xobject_t *)g_dummy_object_new ();
  xobject_add_weak_pointer (object, (xpointer_t *)&object);

  task = xtask_new (object, NULL, keepalive_callback, &result);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);
  xsignal_connect (task, "notify::completed",
                    (xcallback_t) completed_cb, &notification_emitted);

  ref_count = object->ref_count;
  g_assert_cmpint (ref_count, >, 1);

  xassert (xtask_get_source_object (task) == object);
  xassert (xasync_result_get_source_object (G_ASYNC_RESULT (task)) == object);
  g_assert_cmpint (object->ref_count, ==, ref_count + 1);
  xobject_unref (object);

  xobject_unref (object);
  xassert (object != NULL);

  xtask_return_int (task, magic);
  xmain_loop_run (loop);

  xassert (object != NULL);
  g_assert_cmpint (result, ==, magic);
  g_assert_true (notification_emitted);

  xobject_unref (task);
  xassert (task == NULL);
  xassert (object == NULL);
}

/* test_legacy_error: legacy xsimple_async_result_t handling */
static void test_legacy_error (void);

static void
legacy_error_callback (xobject_t      *object,
                       xasync_result_t *result,
                       xpointer_t      user_data)
{
  xssize_t *result_out = user_data;
  xerror_t *error = NULL;

  xassert (object == NULL);
  xassert (xasync_result_is_tagged (result, test_legacy_error));
  xassert (xasync_result_get_user_data (result) == user_data);

  if (xasync_result_legacy_propagate_error (result, &error))
    {
      xassert (!xtask_is_valid (result, object));
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      xassert (xsimple_async_result_is_valid (result, object, test_legacy_error));
      G_GNUC_END_IGNORE_DEPRECATIONS;

      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
      *result_out = -2;
      g_clear_error (&error);
    }
  else
    {
      xassert (xtask_is_valid (result, object));

      *result_out = xtask_propagate_int (XTASK (result), NULL);
      /* Might be error, might not */
    }

  xmain_loop_quit (loop);
}

static xboolean_t
legacy_error_return (xpointer_t user_data)
{
  if (X_IS_TASK (user_data))
    {
      xtask_t *task = user_data;

      xtask_return_int (task, magic);
      xobject_unref (task);
    }
  else
    {
      xsimple_async_result_t *simple = XSIMPLE_ASYNC_RESULT (user_data);

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      xsimple_async_result_set_error (simple,
                                       G_IO_ERROR,
                                       G_IO_ERROR_FAILED,
                                       "Failed");
      xsimple_async_result_complete (simple);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      xobject_unref (simple);
    }

  return FALSE;
}

static void
test_legacy_error (void)
{
  xtask_t *task;
  xsimple_async_result_t *simple;
  xssize_t result;

  /* xtask_t success */
  task = xtask_new (NULL, NULL, legacy_error_callback, &result);
  xtask_set_source_tag (task, test_legacy_error);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);

  g_idle_add (legacy_error_return, task);
  xmain_loop_run (loop);

  g_assert_cmpint (result, ==, magic);
  xassert (task == NULL);

  /* xtask_t error */
  task = xtask_new (NULL, NULL, legacy_error_callback, &result);
  xtask_set_source_tag (task, test_legacy_error);
  xobject_add_weak_pointer (G_OBJECT (task), (xpointer_t *)&task);

  xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                           "Failed");
  xobject_unref (task);
  xmain_loop_run (loop);

  g_assert_cmpint (result, ==, -1);
  xassert (task == NULL);

  /* xsimple_async_result_t error */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  simple = xsimple_async_result_new (NULL, legacy_error_callback, &result,
                                      test_legacy_error);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  xobject_add_weak_pointer (G_OBJECT (simple), (xpointer_t *)&simple);

  g_idle_add (legacy_error_return, simple);
  xmain_loop_run (loop);

  g_assert_cmpint (result, ==, -2);
  xassert (simple == NULL);
}

/* Various helper functions for the return tests below. */
static void
task_complete_cb (xobject_t *source,
                  xasync_result_t *result,
                  xpointer_t user_data)
{
  xtask_t *task = XTASK (result);
  xuint_t *calls = user_data;

  g_assert_cmpint (++*calls, <=, 1);

  /* Propagate the result, so it???s removed from the task???s internal state. */
  xtask_propagate_boolean (task, NULL);
}

static void
return_twice (xtask_t *task)
{
  xboolean_t error_first = GPOINTER_TO_UINT (xtask_get_task_data (task));

  if (error_first)
    {
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_UNKNOWN, "oh no");
      xtask_return_boolean (task, TRUE);
    }
  else
    {
      xtask_return_boolean (task, TRUE);
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_UNKNOWN, "oh no");
    }
}

static xboolean_t
idle_cb (xpointer_t user_data)
{
  xtask_t *task = user_data;
  return_twice (task);
  xobject_unref (task);

  return XSOURCE_REMOVE;
}

static void
test_return_permutation (xboolean_t error_first,
                         xboolean_t return_in_idle)
{
  xuint_t calls = 0;
  xtask_t *task = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1525");

  task = xtask_new (NULL, NULL, task_complete_cb, &calls);
  xtask_set_task_data (task, GUINT_TO_POINTER (error_first), NULL);

  if (return_in_idle)
    g_idle_add (idle_cb, xobject_ref (task));
  else
    return_twice (task);

  while (calls == 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpint (calls, ==, 1);

  xobject_unref (task);
}

/* test_t that calling xtask_return_boolean() after xtask_return_error(), when
 * returning in an idle callback, correctly results in a critical warning. */
static void
test_return_in_idle_error_first (void)
{
  if (g_test_subprocess ())
    {
      test_return_permutation (TRUE, TRUE);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*assertion '!task->ever_returned' failed*");
}

/* test_t that calling xtask_return_error() after xtask_return_boolean(), when
 * returning in an idle callback, correctly results in a critical warning. */
static void
test_return_in_idle_value_first (void)
{
  if (g_test_subprocess ())
    {
      test_return_permutation (FALSE, TRUE);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*assertion '!task->ever_returned' failed*");
}

/* test_t that calling xtask_return_boolean() after xtask_return_error(), when
 * returning synchronously, correctly results in a critical warning. */
static void
test_return_error_first (void)
{
  if (g_test_subprocess ())
    {
      test_return_permutation (TRUE, FALSE);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*assertion '!task->ever_returned' failed*");
}

/* test_t that calling xtask_return_error() after xtask_return_boolean(), when
 * returning synchronously, correctly results in a critical warning. */
static void
test_return_value_first (void)
{
  if (g_test_subprocess ())
    {
      test_return_permutation (FALSE, FALSE);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*assertion '!task->ever_returned' failed*");
}

int
main (int argc, char **argv)
{
  int ret;

  g_test_init (&argc, &argv, NULL);

  loop = xmain_loop_new (NULL, FALSE);
  main_thread = xthread_self ();
  magic = g_get_monotonic_time ();

  g_test_add_func ("/gtask/basic", test_basic);
  g_test_add_func ("/gtask/error", test_error);
  g_test_add_func ("/gtask/return-from-same-iteration", test_return_from_same_iteration);
  g_test_add_func ("/gtask/return-from-toplevel", test_return_from_toplevel);
  g_test_add_func ("/gtask/return-from-anon-thread", test_return_from_anon_thread);
  g_test_add_func ("/gtask/return-from-wrong-thread", test_return_from_wronxthread);
  g_test_add_func ("/gtask/no-callback", test_no_callback);
  g_test_add_func ("/gtask/report-error", test_report_error);
  g_test_add_func ("/gtask/priority", test_priority);
  g_test_add_func ("/gtask/name", test_name);
  g_test_add_func ("/gtask/asynchronous-cancellation", test_asynchronous_cancellation);
  g_test_add_func ("/gtask/check-cancellable", test_check_cancellable);
  g_test_add_func ("/gtask/return-if-cancelled", test_return_if_cancelled);
  g_test_add_func ("/gtask/run-in-thread", test_run_in_thread);
  g_test_add_func ("/gtask/run-in-thread-sync", test_run_in_thread_sync);
  g_test_add_func ("/gtask/run-in-thread-priority", test_run_in_thread_priority);
  g_test_add_func ("/gtask/run-in-thread-nested", test_run_in_thread_nested);
  g_test_add_func ("/gtask/run-in-thread-overflow", test_run_in_thread_overflow);
  g_test_add_func ("/gtask/return-on-cancel", test_return_on_cancel);
  g_test_add_func ("/gtask/return-on-cancel-sync", test_return_on_cancel_sync);
  g_test_add_func ("/gtask/return-on-cancel-atomic", test_return_on_cancel_atomic);
  g_test_add_func ("/gtask/return-pointer", test_return_pointer);
  g_test_add_func ("/gtask/return-value", test_return_value);
  g_test_add_func ("/gtask/object-keepalive", test_object_keepalive);
  g_test_add_func ("/gtask/legacy-error", test_legacy_error);
  g_test_add_func ("/gtask/return/in-idle/error-first", test_return_in_idle_error_first);
  g_test_add_func ("/gtask/return/in-idle/value-first", test_return_in_idle_value_first);
  g_test_add_func ("/gtask/return/error-first", test_return_error_first);
  g_test_add_func ("/gtask/return/value-first", test_return_value_first);

  ret = g_test_run();

  xmain_loop_unref (loop);

  return ret;
}
