/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011-2018 Red Hat, Inc.
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
 */

#include "config.h"
#include "gio_trace.h"

#include "gtask.h"

#include "gasyncresult.h"
#include "gcancellable.h"
#include "glib-private.h"
#include "gtrace-private.h"

#include "glibintl.h"

#include <string.h>

/**
 * SECTION:gtask
 * @short_description: Cancellable synchronous or asynchronous task
 *     and result
 * @include: gio/gio.h
 * @see_also: #xasync_result_t
 *
 * A #xtask_t represents and manages a cancellable "task".
 *
 * ## Asynchronous operations
 *
 * The most common usage of #xtask_t is as a #xasync_result_t, to
 * manage data during an asynchronous operation. You call
 * xtask_new() in the "start" method, followed by
 * xtask_set_task_data() and the like if you need to keep some
 * additional data associated with the task, and then pass the
 * task object around through your asynchronous operation.
 * Eventually, you will call a method such as
 * xtask_return_pointer() or xtask_return_error(), which will
 * save the value you give it and then invoke the task's callback
 * function in the
 * [thread-default main context][g-main-context-push-thread-default]
 * where it was created (waiting until the next iteration of the main
 * loop first, if necessary). The caller will pass the #xtask_t back to
 * the operation's finish function (as a #xasync_result_t), and you can
 * use xtask_propagate_pointer() or the like to extract the
 * return value.
 *
 * Using #xtask_t requires the thread-default #xmain_context_t from when the
 * #xtask_t was constructed to be running at least until the task has completed
 * and its data has been freed.
 *
 * Here is an example for using xtask_t as a xasync_result_t:
 * |[<!-- language="C" -->
 *     typedef struct {
 *       CakeFrostingType frosting;
 *       char *message;
 *     } DecorationData;
 *
 *     static void
 *     decoration_data_free (DecorationData *decoration)
 *     {
 *       g_free (decoration->message);
 *       g_slice_free (DecorationData, decoration);
 *     }
 *
 *     static void
 *     baked_cb (Cake     *cake,
 *               xpointer_t  user_data)
 *     {
 *       xtask_t *task = user_data;
 *       DecorationData *decoration = xtask_get_task_data (task);
 *       xerror_t *error = NULL;
 *
 *       if (cake == NULL)
 *         {
 *           xtask_return_new_error (task, BAKER_ERROR, BAKER_ERROR_NO_FLOUR,
 *                                    "Go to the supermarket");
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       if (!cake_decorate (cake, decoration->frosting, decoration->message, &error))
 *         {
 *           xobject_unref (cake);
 *           // xtask_return_error() takes ownership of error
 *           xtask_return_error (task, error);
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       xtask_return_pointer (task, cake, xobject_unref);
 *       xobject_unref (task);
 *     }
 *
 *     void
 *     baker_bake_cake_async (Baker               *self,
 *                            xuint_t                radius,
 *                            CakeFlavor           flavor,
 *                            CakeFrostingType     frosting,
 *                            const char          *message,
 *                            xcancellable_t        *cancellable,
 *                            xasync_ready_callback_t  callback,
 *                            xpointer_t             user_data)
 *     {
 *       xtask_t *task;
 *       DecorationData *decoration;
 *       Cake  *cake;
 *
 *       task = xtask_new (self, cancellable, callback, user_data);
 *       if (radius < 3)
 *         {
 *           xtask_return_new_error (task, BAKER_ERROR, BAKER_ERROR_TOO_SMALL,
 *                                    "%ucm radius cakes are silly",
 *                                    radius);
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       cake = _baker_get_cached_cake (self, radius, flavor, frosting, message);
 *       if (cake != NULL)
 *         {
 *           // _baker_get_cached_cake() returns a reffed cake
 *           xtask_return_pointer (task, cake, xobject_unref);
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       decoration = g_slice_new (DecorationData);
 *       decoration->frosting = frosting;
 *       decoration->message = xstrdup (message);
 *       xtask_set_task_data (task, decoration, (xdestroy_notify_t) decoration_data_free);
 *
 *       _baker_begin_cake (self, radius, flavor, cancellable, baked_cb, task);
 *     }
 *
 *     Cake *
 *     baker_bake_cake_finish (Baker         *self,
 *                             xasync_result_t  *result,
 *                             xerror_t       **error)
 *     {
 *       g_return_val_if_fail (xtask_is_valid (result, self), NULL);
 *
 *       return xtask_propagate_pointer (XTASK (result), error);
 *     }
 * ]|
 *
 * ## Chained asynchronous operations
 *
 * #xtask_t also tries to simplify asynchronous operations that
 * internally chain together several smaller asynchronous
 * operations. xtask_get_cancellable(), xtask_get_context(),
 * and xtask_get_priority() allow you to get back the task's
 * #xcancellable_t, #xmain_context_t, and [I/O priority][io-priority]
 * when starting a new subtask, so you don't have to keep track
 * of them yourself. xtask_attach_source() simplifies the case
 * of waiting for a source to fire (automatically using the correct
 * #xmain_context_t and priority).
 *
 * Here is an example for chained asynchronous operations:
 *   |[<!-- language="C" -->
 *     typedef struct {
 *       Cake *cake;
 *       CakeFrostingType frosting;
 *       char *message;
 *     } BakingData;
 *
 *     static void
 *     decoration_data_free (BakingData *bd)
 *     {
 *       if (bd->cake)
 *         xobject_unref (bd->cake);
 *       g_free (bd->message);
 *       g_slice_free (BakingData, bd);
 *     }
 *
 *     static void
 *     decorated_cb (Cake         *cake,
 *                   xasync_result_t *result,
 *                   xpointer_t      user_data)
 *     {
 *       xtask_t *task = user_data;
 *       xerror_t *error = NULL;
 *
 *       if (!cake_decorate_finish (cake, result, &error))
 *         {
 *           xobject_unref (cake);
 *           xtask_return_error (task, error);
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       // baking_data_free() will drop its ref on the cake, so we have to
 *       // take another here to give to the caller.
 *       xtask_return_pointer (task, xobject_ref (cake), xobject_unref);
 *       xobject_unref (task);
 *     }
 *
 *     static xboolean_t
 *     decorator_ready (xpointer_t user_data)
 *     {
 *       xtask_t *task = user_data;
 *       BakingData *bd = xtask_get_task_data (task);
 *
 *       cake_decorate_async (bd->cake, bd->frosting, bd->message,
 *                            xtask_get_cancellable (task),
 *                            decorated_cb, task);
 *
 *       return G_SOURCE_REMOVE;
 *     }
 *
 *     static void
 *     baked_cb (Cake     *cake,
 *               xpointer_t  user_data)
 *     {
 *       xtask_t *task = user_data;
 *       BakingData *bd = xtask_get_task_data (task);
 *       xerror_t *error = NULL;
 *
 *       if (cake == NULL)
 *         {
 *           xtask_return_new_error (task, BAKER_ERROR, BAKER_ERROR_NO_FLOUR,
 *                                    "Go to the supermarket");
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       bd->cake = cake;
 *
 *       // Bail out now if the user has already cancelled
 *       if (xtask_return_error_if_cancelled (task))
 *         {
 *           xobject_unref (task);
 *           return;
 *         }
 *
 *       if (cake_decorator_available (cake))
 *         decorator_ready (task);
 *       else
 *         {
 *           xsource_t *source;
 *
 *           source = cake_decorator_wait_source_new (cake);
 *           // Attach @source to @task's xmain_context_t and have it call
 *           // decorator_ready() when it is ready.
 *           xtask_attach_source (task, source, decorator_ready);
 *           xsource_unref (source);
 *         }
 *     }
 *
 *     void
 *     baker_bake_cake_async (Baker               *self,
 *                            xuint_t                radius,
 *                            CakeFlavor           flavor,
 *                            CakeFrostingType     frosting,
 *                            const char          *message,
 *                            xint_t                 priority,
 *                            xcancellable_t        *cancellable,
 *                            xasync_ready_callback_t  callback,
 *                            xpointer_t             user_data)
 *     {
 *       xtask_t *task;
 *       BakingData *bd;
 *
 *       task = xtask_new (self, cancellable, callback, user_data);
 *       xtask_set_priority (task, priority);
 *
 *       bd = g_slice_new0 (BakingData);
 *       bd->frosting = frosting;
 *       bd->message = xstrdup (message);
 *       xtask_set_task_data (task, bd, (xdestroy_notify_t) baking_data_free);
 *
 *       _baker_begin_cake (self, radius, flavor, cancellable, baked_cb, task);
 *     }
 *
 *     Cake *
 *     baker_bake_cake_finish (Baker         *self,
 *                             xasync_result_t  *result,
 *                             xerror_t       **error)
 *     {
 *       g_return_val_if_fail (xtask_is_valid (result, self), NULL);
 *
 *       return xtask_propagate_pointer (XTASK (result), error);
 *     }
 * ]|
 *
 * ## Asynchronous operations from synchronous ones
 *
 * You can use xtask_run_in_thread() to turn a synchronous
 * operation into an asynchronous one, by running it in a thread.
 * When it completes, the result will be dispatched to the
 * [thread-default main context][g-main-context-push-thread-default]
 * where the #xtask_t was created.
 *
 * Running a task in a thread:
 *   |[<!-- language="C" -->
 *     typedef struct {
 *       xuint_t radius;
 *       CakeFlavor flavor;
 *       CakeFrostingType frosting;
 *       char *message;
 *     } CakeData;
 *
 *     static void
 *     cake_data_free (CakeData *cake_data)
 *     {
 *       g_free (cake_data->message);
 *       g_slice_free (CakeData, cake_data);
 *     }
 *
 *     static void
 *     bake_cake_thread (xtask_t         *task,
 *                       xpointer_t       source_object,
 *                       xpointer_t       task_data,
 *                       xcancellable_t  *cancellable)
 *     {
 *       Baker *self = source_object;
 *       CakeData *cake_data = task_data;
 *       Cake *cake;
 *       xerror_t *error = NULL;
 *
 *       cake = bake_cake (baker, cake_data->radius, cake_data->flavor,
 *                         cake_data->frosting, cake_data->message,
 *                         cancellable, &error);
 *       if (cake)
 *         xtask_return_pointer (task, cake, xobject_unref);
 *       else
 *         xtask_return_error (task, error);
 *     }
 *
 *     void
 *     baker_bake_cake_async (Baker               *self,
 *                            xuint_t                radius,
 *                            CakeFlavor           flavor,
 *                            CakeFrostingType     frosting,
 *                            const char          *message,
 *                            xcancellable_t        *cancellable,
 *                            xasync_ready_callback_t  callback,
 *                            xpointer_t             user_data)
 *     {
 *       CakeData *cake_data;
 *       xtask_t *task;
 *
 *       cake_data = g_slice_new (CakeData);
 *       cake_data->radius = radius;
 *       cake_data->flavor = flavor;
 *       cake_data->frosting = frosting;
 *       cake_data->message = xstrdup (message);
 *       task = xtask_new (self, cancellable, callback, user_data);
 *       xtask_set_task_data (task, cake_data, (xdestroy_notify_t) cake_data_free);
 *       xtask_run_in_thread (task, bake_cake_thread);
 *       xobject_unref (task);
 *     }
 *
 *     Cake *
 *     baker_bake_cake_finish (Baker         *self,
 *                             xasync_result_t  *result,
 *                             xerror_t       **error)
 *     {
 *       g_return_val_if_fail (xtask_is_valid (result, self), NULL);
 *
 *       return xtask_propagate_pointer (XTASK (result), error);
 *     }
 * ]|
 *
 * ## Adding cancellability to uncancellable tasks
 *
 * Finally, xtask_run_in_thread() and xtask_run_in_thread_sync()
 * can be used to turn an uncancellable operation into a
 * cancellable one. If you call xtask_set_return_on_cancel(),
 * passing %TRUE, then if the task's #xcancellable_t is cancelled,
 * it will return control back to the caller immediately, while
 * allowing the task thread to continue running in the background
 * (and simply discarding its result when it finally does finish).
 * Provided that the task thread is careful about how it uses
 * locks and other externally-visible resources, this allows you
 * to make "GLib-friendly" asynchronous and cancellable
 * synchronous variants of blocking APIs.
 *
 * Cancelling a task:
 *   |[<!-- language="C" -->
 *     static void
 *     bake_cake_thread (xtask_t         *task,
 *                       xpointer_t       source_object,
 *                       xpointer_t       task_data,
 *                       xcancellable_t  *cancellable)
 *     {
 *       Baker *self = source_object;
 *       CakeData *cake_data = task_data;
 *       Cake *cake;
 *       xerror_t *error = NULL;
 *
 *       cake = bake_cake (baker, cake_data->radius, cake_data->flavor,
 *                         cake_data->frosting, cake_data->message,
 *                         &error);
 *       if (error)
 *         {
 *           xtask_return_error (task, error);
 *           return;
 *         }
 *
 *       // If the task has already been cancelled, then we don't want to add
 *       // the cake to the cake cache. Likewise, we don't  want to have the
 *       // task get cancelled in the middle of updating the cache.
 *       // xtask_set_return_on_cancel() will return %TRUE here if it managed
 *       // to disable return-on-cancel, or %FALSE if the task was cancelled
 *       // before it could.
 *       if (xtask_set_return_on_cancel (task, FALSE))
 *         {
 *           // If the caller cancels at this point, their
 *           // xasync_ready_callback_t won't be invoked until we return,
 *           // so we don't have to worry that this code will run at
 *           // the same time as that code does. But if there were
 *           // other functions that might look at the cake cache,
 *           // then we'd probably need a xmutex_t here as well.
 *           baker_add_cake_to_cache (baker, cake);
 *           xtask_return_pointer (task, cake, xobject_unref);
 *         }
 *     }
 *
 *     void
 *     baker_bake_cake_async (Baker               *self,
 *                            xuint_t                radius,
 *                            CakeFlavor           flavor,
 *                            CakeFrostingType     frosting,
 *                            const char          *message,
 *                            xcancellable_t        *cancellable,
 *                            xasync_ready_callback_t  callback,
 *                            xpointer_t             user_data)
 *     {
 *       CakeData *cake_data;
 *       xtask_t *task;
 *
 *       cake_data = g_slice_new (CakeData);
 *
 *       ...
 *
 *       task = xtask_new (self, cancellable, callback, user_data);
 *       xtask_set_task_data (task, cake_data, (xdestroy_notify_t) cake_data_free);
 *       xtask_set_return_on_cancel (task, TRUE);
 *       xtask_run_in_thread (task, bake_cake_thread);
 *     }
 *
 *     Cake *
 *     baker_bake_cake_sync (Baker               *self,
 *                           xuint_t                radius,
 *                           CakeFlavor           flavor,
 *                           CakeFrostingType     frosting,
 *                           const char          *message,
 *                           xcancellable_t        *cancellable,
 *                           xerror_t             **error)
 *     {
 *       CakeData *cake_data;
 *       xtask_t *task;
 *       Cake *cake;
 *
 *       cake_data = g_slice_new (CakeData);
 *
 *       ...
 *
 *       task = xtask_new (self, cancellable, NULL, NULL);
 *       xtask_set_task_data (task, cake_data, (xdestroy_notify_t) cake_data_free);
 *       xtask_set_return_on_cancel (task, TRUE);
 *       xtask_run_in_thread_sync (task, bake_cake_thread);
 *
 *       cake = xtask_propagate_pointer (task, error);
 *       xobject_unref (task);
 *       return cake;
 *     }
 * ]|
 *
 * ## Porting from xsimple_async_result_t
 *
 * #xtask_t's API attempts to be simpler than #xsimple_async_result_t's
 * in several ways:
 * - You can save task-specific data with xtask_set_task_data(), and
 *   retrieve it later with xtask_get_task_data(). This replaces the
 *   abuse of xsimple_async_result_set_op_res_gpointer() for the same
 *   purpose with #xsimple_async_result_t.
 * - In addition to the task data, #xtask_t also keeps track of the
 *   [priority][io-priority], #xcancellable_t, and
 *   #xmain_context_t associated with the task, so tasks that consist of
 *   a chain of simpler asynchronous operations will have easy access
 *   to those values when starting each sub-task.
 * - xtask_return_error_if_cancelled() provides simplified
 *   handling for cancellation. In addition, cancellation
 *   overrides any other #xtask_t return value by default, like
 *   #xsimple_async_result_t does when
 *   xsimple_async_result_set_check_cancellable() is called.
 *   (You can use xtask_set_check_cancellable() to turn off that
 *   behavior.) On the other hand, xtask_run_in_thread()
 *   guarantees that it will always run your
 *   `task_func`, even if the task's #xcancellable_t
 *   is already cancelled before the task gets a chance to run;
 *   you can start your `task_func` with a
 *   xtask_return_error_if_cancelled() check if you need the
 *   old behavior.
 * - The "return" methods (eg, xtask_return_pointer())
 *   automatically cause the task to be "completed" as well, and
 *   there is no need to worry about the "complete" vs "complete
 *   in idle" distinction. (#xtask_t automatically figures out
 *   whether the task's callback can be invoked directly, or
 *   if it needs to be sent to another #xmain_context_t, or delayed
 *   until the next iteration of the current #xmain_context_t.)
 * - The "finish" functions for #xtask_t based operations are generally
 *   much simpler than #xsimple_async_result_t ones, normally consisting
 *   of only a single call to xtask_propagate_pointer() or the like.
 *   Since xtask_propagate_pointer() "steals" the return value from
 *   the #xtask_t, it is not necessary to juggle pointers around to
 *   prevent it from being freed twice.
 * - With #xsimple_async_result_t, it was common to call
 *   xsimple_async_result_propagate_error() from the
 *   `_finish()` wrapper function, and have
 *   virtual method implementations only deal with successful
 *   returns. This behavior is deprecated, because it makes it
 *   difficult for a subclass to chain to a parent class's async
 *   methods. Instead, the wrapper function should just be a
 *   simple wrapper, and the virtual method should call an
 *   appropriate `xtask_propagate_` function.
 *   Note that wrapper methods can now use
 *   xasync_result_legacy_propagate_error() to do old-style
 *   #xsimple_async_result_t error-returning behavior, and
 *   xasync_result_is_tagged() to check if a result is tagged as
 *   having come from the `_async()` wrapper
 *   function (for "short-circuit" results, such as when passing
 *   0 to xinput_stream_read_async()).
 */

/**
 * xtask_t:
 *
 * The opaque object representing a synchronous or asynchronous task
 * and its result.
 */

struct _GTask {
  xobject_t parent_instance;

  xpointer_t source_object;
  xpointer_t source_tag;
  xchar_t *name;  /* (owned); may only be modified before the #xtask_t is threaded */

  xpointer_t task_data;
  xdestroy_notify_t task_data_destroy;

  xmain_context_t *context;
  sint64_t creation_time;
  xint_t priority;
  xcancellable_t *cancellable;

  xasync_ready_callback_t callback;
  xpointer_t callback_data;

  xtask_thread_func_t task_func;
  xmutex_t lock;
  xcond_t cond;

  /* This can’t be in the bit field because we access it from TRACE(). */
  xboolean_t thread_cancelled;

  /* Protected by the lock when task is threaded: */
  xboolean_t thread_complete : 1;
  xboolean_t return_on_cancel : 1;
  xboolean_t : 0;

  /* Unprotected, but written to when task runs in thread: */
  xboolean_t completed : 1;
  xboolean_t had_error : 1;
  xboolean_t result_set : 1;
  xboolean_t ever_returned : 1;
  xboolean_t : 0;

  /* Read-only once task runs in thread: */
  xboolean_t check_cancellable : 1;
  xboolean_t synchronous : 1;
  xboolean_t blocking_other_task : 1;

  xerror_t *error;
  union {
    xpointer_t pointer;
    xssize_t   size;
    xboolean_t boolean;
  } result;
  xdestroy_notify_t result_destroy;
};

#define XTASK_IS_THREADED(task) ((task)->task_func != NULL)

struct _xtask_class
{
  xobject_class_t parent_class;
};

typedef enum
{
  PROP_COMPLETED = 1,
} GTaskProperty;

static void xtask_async_result_iface_init (xasync_result_iface_t *iface);
static void xtask_thread_pool_init (void);

G_DEFINE_TYPE_WITH_CODE (xtask, xtask, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_ASYNC_RESULT,
                                                xtask_async_result_iface_init);
                         xtask_thread_pool_init ();)

static GThreadPool *task_pool;
static xmutex_t task_pool_mutex;
static GPrivate task_private = G_PRIVATE_INIT (NULL);
static xsource_t *task_pool_manager;
static xuint64_t task_wait_time;
static xint_t tasks_running;

static xuint_t task_pool_max_counter;
static xuint_t tasks_running_counter;

/* When the task pool fills up and blocks, and the program keeps
 * queueing more tasks, we will slowly add more threads to the pool
 * (in case the existing tasks are trying to queue subtasks of their
 * own) until tasks start completing again. These "overflow" threads
 * will only run one task apiece, and then exit, so the pool will
 * eventually get back down to its base size.
 *
 * The base and multiplier below gives us 10 extra threads after about
 * a second of blocking, 30 after 5 seconds, 100 after a minute, and
 * 200 after 20 minutes.
 *
 * We specify maximum pool size of 330 to increase the waiting time up
 * to around 30 minutes.
 */
#define XTASK_POOL_SIZE 10
#define XTASK_WAIT_TIME_BASE 100000
#define XTASK_WAIT_TIME_MULTIPLIER 1.03
#define XTASK_WAIT_TIME_MAX_POOL_SIZE 330

static void
xtask_init (xtask_t *task)
{
  task->check_cancellable = TRUE;
}

static void
xtask_finalize (xobject_t *object)
{
  xtask_t *task = XTASK (object);

  g_clear_object (&task->source_object);
  g_clear_object (&task->cancellable);
  g_free (task->name);

  if (task->context)
    xmain_context_unref (task->context);

  if (task->task_data_destroy)
    task->task_data_destroy (task->task_data);

  if (task->result_destroy && task->result.pointer)
    task->result_destroy (task->result.pointer);

  if (task->error)
      xerror_free (task->error);

  if (XTASK_IS_THREADED (task))
    {
      g_mutex_clear (&task->lock);
      g_cond_clear (&task->cond);
    }

  G_OBJECT_CLASS (xtask_parent_class)->finalize (object);
}

/**
 * xtask_new:
 * @source_object: (nullable) (type xobject_t): the #xobject_t that owns
 *   this task, or %NULL.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @callback_data: (closure): user data passed to @callback.
 *
 * Creates a #xtask_t acting on @source_object, which will eventually be
 * used to invoke @callback in the current
 * [thread-default main context][g-main-context-push-thread-default].
 *
 * Call this in the "start" method of your asynchronous method, and
 * pass the #xtask_t around throughout the asynchronous operation. You
 * can use xtask_set_task_data() to attach task-specific data to the
 * object, which you can retrieve later via xtask_get_task_data().
 *
 * By default, if @cancellable is cancelled, then the return value of
 * the task will always be %G_IO_ERROR_CANCELLED, even if the task had
 * already completed before the cancellation. This allows for
 * simplified handling in cases where cancellation may imply that
 * other objects that the task depends on have been destroyed. If you
 * do not want this behavior, you can use
 * xtask_set_check_cancellable() to change it.
 *
 * Returns: a #xtask_t.
 *
 * Since: 2.36
 */
xtask_t *
xtask_new (xpointer_t              source_object,
            xcancellable_t         *cancellable,
            xasync_ready_callback_t   callback,
            xpointer_t              callback_data)
{
  xtask_t *task;
  xsource_t *source;

  task = xobject_new (XTYPE_TASK, NULL);
  task->source_object = source_object ? xobject_ref (source_object) : NULL;
  task->cancellable = cancellable ? xobject_ref (cancellable) : NULL;
  task->callback = callback;
  task->callback_data = callback_data;
  task->context = xmain_context_ref_thread_default ();

  source = g_main_current_source ();
  if (source)
    task->creation_time = xsource_get_time (source);

  TRACE (GIO_TASK_NEW (task, source_object, cancellable,
                       callback, callback_data));

  return task;
}

/**
 * xtask_report_error:
 * @source_object: (nullable) (type xobject_t): the #xobject_t that owns
 *   this task, or %NULL.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @callback_data: (closure): user data passed to @callback.
 * @source_tag: an opaque pointer indicating the source of this task
 * @error: (transfer full): error to report
 *
 * Creates a #xtask_t and then immediately calls xtask_return_error()
 * on it. Use this in the wrapper function of an asynchronous method
 * when you want to avoid even calling the virtual method. You can
 * then use xasync_result_is_tagged() in the finish method wrapper to
 * check if the result there is tagged as having been created by the
 * wrapper method, and deal with it appropriately if so.
 *
 * See also xtask_report_new_error().
 *
 * Since: 2.36
 */
void
xtask_report_error (xpointer_t             source_object,
                     xasync_ready_callback_t  callback,
                     xpointer_t             callback_data,
                     xpointer_t             source_tag,
                     xerror_t              *error)
{
  xtask_t *task;

  task = xtask_new (source_object, NULL, callback, callback_data);
  xtask_set_source_tag (task, source_tag);
  xtask_set_name (task, G_STRFUNC);
  xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * xtask_report_new_error:
 * @source_object: (nullable) (type xobject_t): the #xobject_t that owns
 *   this task, or %NULL.
 * @callback: (scope async): a #xasync_ready_callback_t.
 * @callback_data: (closure): user data passed to @callback.
 * @source_tag: an opaque pointer indicating the source of this task
 * @domain: a #xquark.
 * @code: an error code.
 * @format: a string with format characters.
 * @...: a list of values to insert into @format.
 *
 * Creates a #xtask_t and then immediately calls
 * xtask_return_new_error() on it. Use this in the wrapper function
 * of an asynchronous method when you want to avoid even calling the
 * virtual method. You can then use xasync_result_is_tagged() in the
 * finish method wrapper to check if the result there is tagged as
 * having been created by the wrapper method, and deal with it
 * appropriately if so.
 *
 * See also xtask_report_error().
 *
 * Since: 2.36
 */
void
xtask_report_new_error (xpointer_t             source_object,
                         xasync_ready_callback_t  callback,
                         xpointer_t             callback_data,
                         xpointer_t             source_tag,
                         xquark               domain,
                         xint_t                 code,
                         const char          *format,
                         ...)
{
  xerror_t *error;
  va_list ap;

  va_start (ap, format);
  error = xerror_new_valist (domain, code, format, ap);
  va_end (ap);

  xtask_report_error (source_object, callback, callback_data,
                       source_tag, error);
}

/**
 * xtask_set_task_data:
 * @task: the #xtask_t
 * @task_data: (nullable): task-specific data
 * @task_data_destroy: (nullable): #xdestroy_notify_t for @task_data
 *
 * Sets @task's task data (freeing the existing task data, if any).
 *
 * Since: 2.36
 */
void
xtask_set_task_data (xtask_t          *task,
                      xpointer_t        task_data,
                      xdestroy_notify_t  task_data_destroy)
{
  g_return_if_fail (X_IS_TASK (task));

  if (task->task_data_destroy)
    task->task_data_destroy (task->task_data);

  task->task_data = task_data;
  task->task_data_destroy = task_data_destroy;

  TRACE (GIO_TASK_SET_TASK_DATA (task, task_data, task_data_destroy));
}

/**
 * xtask_set_priority:
 * @task: the #xtask_t
 * @priority: the [priority][io-priority] of the request
 *
 * Sets @task's priority. If you do not call this, it will default to
 * %G_PRIORITY_DEFAULT.
 *
 * This will affect the priority of #GSources created with
 * xtask_attach_source() and the scheduling of tasks run in threads,
 * and can also be explicitly retrieved later via
 * xtask_get_priority().
 *
 * Since: 2.36
 */
void
xtask_set_priority (xtask_t *task,
                     xint_t   priority)
{
  g_return_if_fail (X_IS_TASK (task));

  task->priority = priority;

  TRACE (GIO_TASK_SET_PRIORITY (task, priority));
}

/**
 * xtask_set_check_cancellable:
 * @task: the #xtask_t
 * @check_cancellable: whether #xtask_t will check the state of
 *   its #xcancellable_t for you.
 *
 * Sets or clears @task's check-cancellable flag. If this is %TRUE
 * (the default), then xtask_propagate_pointer(), etc, and
 * xtask_had_error() will check the task's #xcancellable_t first, and
 * if it has been cancelled, then they will consider the task to have
 * returned an "Operation was cancelled" error
 * (%G_IO_ERROR_CANCELLED), regardless of any other error or return
 * value the task may have had.
 *
 * If @check_cancellable is %FALSE, then the #xtask_t will not check the
 * cancellable itself, and it is up to @task's owner to do this (eg,
 * via xtask_return_error_if_cancelled()).
 *
 * If you are using xtask_set_return_on_cancel() as well, then
 * you must leave check-cancellable set %TRUE.
 *
 * Since: 2.36
 */
void
xtask_set_check_cancellable (xtask_t    *task,
                              xboolean_t  check_cancellable)
{
  g_return_if_fail (X_IS_TASK (task));
  g_return_if_fail (check_cancellable || !task->return_on_cancel);

  task->check_cancellable = check_cancellable;
}

static void xtask_thread_complete (xtask_t *task);

/**
 * xtask_set_return_on_cancel:
 * @task: the #xtask_t
 * @return_on_cancel: whether the task returns automatically when
 *   it is cancelled.
 *
 * Sets or clears @task's return-on-cancel flag. This is only
 * meaningful for tasks run via xtask_run_in_thread() or
 * xtask_run_in_thread_sync().
 *
 * If @return_on_cancel is %TRUE, then cancelling @task's
 * #xcancellable_t will immediately cause it to return, as though the
 * task's #xtask_thread_func_t had called
 * xtask_return_error_if_cancelled() and then returned.
 *
 * This allows you to create a cancellable wrapper around an
 * uninterruptible function. The #xtask_thread_func_t just needs to be
 * careful that it does not modify any externally-visible state after
 * it has been cancelled. To do that, the thread should call
 * xtask_set_return_on_cancel() again to (atomically) set
 * return-on-cancel %FALSE before making externally-visible changes;
 * if the task gets cancelled before the return-on-cancel flag could
 * be changed, xtask_set_return_on_cancel() will indicate this by
 * returning %FALSE.
 *
 * You can disable and re-enable this flag multiple times if you wish.
 * If the task's #xcancellable_t is cancelled while return-on-cancel is
 * %FALSE, then calling xtask_set_return_on_cancel() to set it %TRUE
 * again will cause the task to be cancelled at that point.
 *
 * If the task's #xcancellable_t is already cancelled before you call
 * xtask_run_in_thread()/xtask_run_in_thread_sync(), then the
 * #xtask_thread_func_t will still be run (for consistency), but the task
 * will also be completed right away.
 *
 * Returns: %TRUE if @task's return-on-cancel flag was changed to
 *   match @return_on_cancel. %FALSE if @task has already been
 *   cancelled.
 *
 * Since: 2.36
 */
xboolean_t
xtask_set_return_on_cancel (xtask_t    *task,
                             xboolean_t  return_on_cancel)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);
  g_return_val_if_fail (task->check_cancellable || !return_on_cancel, FALSE);

  if (!XTASK_IS_THREADED (task))
    {
      task->return_on_cancel = return_on_cancel;
      return TRUE;
    }

  g_mutex_lock (&task->lock);
  if (task->thread_cancelled)
    {
      if (return_on_cancel && !task->return_on_cancel)
        {
          g_mutex_unlock (&task->lock);
          xtask_thread_complete (task);
        }
      else
        g_mutex_unlock (&task->lock);
      return FALSE;
    }
  task->return_on_cancel = return_on_cancel;
  g_mutex_unlock (&task->lock);

  return TRUE;
}

/**
 * xtask_set_source_tag:
 * @task: the #xtask_t
 * @source_tag: an opaque pointer indicating the source of this task
 *
 * Sets @task's source tag.
 *
 * You can use this to tag a task return
 * value with a particular pointer (usually a pointer to the function
 * doing the tagging) and then later check it using
 * xtask_get_source_tag() (or xasync_result_is_tagged()) in the
 * task's "finish" function, to figure out if the response came from a
 * particular place.
 *
 * A macro wrapper around this function will automatically set the
 * task’s name to the string form of @source_tag if it’s not already
 * set, for convenience.
 *
 * Since: 2.36
 */
void
(xtask_set_source_tag) (xtask_t    *task,
                         xpointer_t  source_tag)
{
  g_return_if_fail (X_IS_TASK (task));

  task->source_tag = source_tag;

  TRACE (GIO_TASK_SET_SOURCE_TAG (task, source_tag));
}

/**
 * xtask_set_name:
 * @task: a #xtask_t
 * @name: (nullable): a human readable name for the task, or %NULL to unset it
 *
 * Sets @task’s name, used in debugging and profiling. The name defaults to
 * %NULL.
 *
 * The task name should describe in a human readable way what the task does.
 * For example, ‘Open file’ or ‘Connect to network host’. It is used to set the
 * name of the #xsource_t used for idle completion of the task.
 *
 * This function may only be called before the @task is first used in a thread
 * other than the one it was constructed in. It is called automatically by
 * xtask_set_source_tag() if not called already.
 *
 * Since: 2.60
 */
void
xtask_set_name (xtask_t       *task,
                 const xchar_t *name)
{
  xchar_t *new_name;

  g_return_if_fail (X_IS_TASK (task));

  new_name = xstrdup (name);
  g_free (task->name);
  task->name = g_steal_pointer (&new_name);
}

/**
 * xtask_get_source_object:
 * @task: a #xtask_t
 *
 * Gets the source object from @task. Like
 * xasync_result_get_source_object(), but does not ref the object.
 *
 * Returns: (transfer none) (nullable) (type xobject_t): @task's source object, or %NULL
 *
 * Since: 2.36
 */
xpointer_t
xtask_get_source_object (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  return task->source_object;
}

static xobject_t *
xtask_ref_source_object (xasync_result_t *res)
{
  xtask_t *task = XTASK (res);

  if (task->source_object)
    return xobject_ref (task->source_object);
  else
    return NULL;
}

/**
 * xtask_get_task_data:
 * @task: a #xtask_t
 *
 * Gets @task's `task_data`.
 *
 * Returns: (transfer none): @task's `task_data`.
 *
 * Since: 2.36
 */
xpointer_t
xtask_get_task_data (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  return task->task_data;
}

/**
 * xtask_get_priority:
 * @task: a #xtask_t
 *
 * Gets @task's priority
 *
 * Returns: @task's priority
 *
 * Since: 2.36
 */
xint_t
xtask_get_priority (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), G_PRIORITY_DEFAULT);

  return task->priority;
}

/**
 * xtask_get_context:
 * @task: a #xtask_t
 *
 * Gets the #xmain_context_t that @task will return its result in (that
 * is, the context that was the
 * [thread-default main context][g-main-context-push-thread-default]
 * at the point when @task was created).
 *
 * This will always return a non-%NULL value, even if the task's
 * context is the default #xmain_context_t.
 *
 * Returns: (transfer none): @task's #xmain_context_t
 *
 * Since: 2.36
 */
xmain_context_t *
xtask_get_context (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  return task->context;
}

/**
 * xtask_get_cancellable:
 * @task: a #xtask_t
 *
 * Gets @task's #xcancellable_t
 *
 * Returns: (transfer none): @task's #xcancellable_t
 *
 * Since: 2.36
 */
xcancellable_t *
xtask_get_cancellable (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  return task->cancellable;
}

/**
 * xtask_get_check_cancellable:
 * @task: the #xtask_t
 *
 * Gets @task's check-cancellable flag. See
 * xtask_set_check_cancellable() for more details.
 *
 * Since: 2.36
 */
xboolean_t
xtask_get_check_cancellable (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);

  /* Convert from a bit field to a boolean. */
  return task->check_cancellable ? TRUE : FALSE;
}

/**
 * xtask_get_return_on_cancel:
 * @task: the #xtask_t
 *
 * Gets @task's return-on-cancel flag. See
 * xtask_set_return_on_cancel() for more details.
 *
 * Since: 2.36
 */
xboolean_t
xtask_get_return_on_cancel (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);

  /* Convert from a bit field to a boolean. */
  return task->return_on_cancel ? TRUE : FALSE;
}

/**
 * xtask_get_source_tag:
 * @task: a #xtask_t
 *
 * Gets @task's source tag. See xtask_set_source_tag().
 *
 * Returns: (transfer none): @task's source tag
 *
 * Since: 2.36
 */
xpointer_t
xtask_get_source_tag (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  return task->source_tag;
}

/**
 * xtask_get_name:
 * @task: a #xtask_t
 *
 * Gets @task’s name. See xtask_set_name().
 *
 * Returns: (nullable) (transfer none): @task’s name, or %NULL
 * Since: 2.60
 */
const xchar_t *
xtask_get_name (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  return task->name;
}

static void
xtask_return_now (xtask_t *task)
{
  TRACE (GIO_TASK_BEFORE_RETURN (task, task->source_object, task->callback,
                                 task->callback_data));

  xmain_context_push_thread_default (task->context);

  if (task->callback != NULL)
    {
      task->callback (task->source_object,
                      G_ASYNC_RESULT (task),
                      task->callback_data);
    }

  task->completed = TRUE;
  xobject_notify (G_OBJECT (task), "completed");

  xmain_context_pop_thread_default (task->context);
}

static xboolean_t
complete_in_idle_cb (xpointer_t task)
{
  xtask_return_now (task);
  xobject_unref (task);
  return FALSE;
}

typedef enum {
  XTASK_RETURN_SUCCESS,
  XTASK_RETURN_ERROR,
  XTASK_RETURN_FROM_THREAD
} xtask_return_type_t;

static void
xtask_return (xtask_t           *task,
               xtask_return_type_t  type)
{
  xsource_t *source;
  xchar_t *source_name = NULL;

  if (type != XTASK_RETURN_FROM_THREAD)
    task->ever_returned = TRUE;

  if (type == XTASK_RETURN_SUCCESS)
    task->result_set = TRUE;

  if (task->synchronous)
    return;

  /* Normally we want to invoke the task's callback when its return
   * value is set. But if the task is running in a thread, then we
   * want to wait until after the task_func returns, to simplify
   * locking/refcounting/etc.
   */
  if (XTASK_IS_THREADED (task) && type != XTASK_RETURN_FROM_THREAD)
    return;

  xobject_ref (task);

  /* See if we can complete the task immediately. First, we have to be
   * running inside the task's thread/xmain_context_t.
   */
  source = g_main_current_source ();
  if (source && xsource_get_context (source) == task->context)
    {
      /* Second, we can only complete immediately if this is not the
       * same iteration of the main loop that the task was created in.
       */
      if (xsource_get_time (source) > task->creation_time)
        {
          /* Finally, if the task has been cancelled, we shouldn't
           * return synchronously from inside the
           * xcancellable_t::cancelled handler. It's easier to run
           * another iteration of the main loop than tracking how the
           * cancellation was handled.
           */
          if (!xcancellable_is_cancelled (task->cancellable))
            {
              xtask_return_now (task);
              xobject_unref (task);
              return;
            }
        }
    }

  /* Otherwise, complete in the next iteration */
  source = g_idle_source_new ();
  source_name = xstrdup_printf ("[gio] %s complete_in_idle_cb",
                                 (task->name != NULL) ? task->name : "(unnamed)");
  xsource_set_name (source, source_name);
  g_free (source_name);
  xtask_attach_source (task, source, complete_in_idle_cb);
  xsource_unref (source);
}


/**
 * xtask_thread_func_t:
 * @task: the #xtask_t
 * @source_object: (type xobject_t): @task's source object
 * @task_data: @task's task data
 * @cancellable: @task's #xcancellable_t, or %NULL
 *
 * The prototype for a task function to be run in a thread via
 * xtask_run_in_thread() or xtask_run_in_thread_sync().
 *
 * If the return-on-cancel flag is set on @task, and @cancellable gets
 * cancelled, then the #xtask_t will be completed immediately (as though
 * xtask_return_error_if_cancelled() had been called), without
 * waiting for the task function to complete. However, the task
 * function will continue running in its thread in the background. The
 * function therefore needs to be careful about how it uses
 * externally-visible state in this case. See
 * xtask_set_return_on_cancel() for more details.
 *
 * Other than in that case, @task will be completed when the
 * #xtask_thread_func_t returns, not when it calls a
 * `xtask_return_` function.
 *
 * Since: 2.36
 */

static void task_thread_cancelled (xcancellable_t *cancellable,
                                   xpointer_t      user_data);

static void
xtask_thread_complete (xtask_t *task)
{
  g_mutex_lock (&task->lock);
  if (task->thread_complete)
    {
      /* The task belatedly completed after having been cancelled
       * (or was cancelled in the midst of being completed).
       */
      g_mutex_unlock (&task->lock);
      return;
    }

  TRACE (GIO_TASK_AFTER_RUN_IN_THREAD (task, task->thread_cancelled));

  task->thread_complete = TRUE;
  g_mutex_unlock (&task->lock);

  if (task->cancellable)
    xsignal_handlers_disconnect_by_func (task->cancellable, task_thread_cancelled, task);

  if (task->synchronous)
    g_cond_signal (&task->cond);
  else
    xtask_return (task, XTASK_RETURN_FROM_THREAD);
}

static xboolean_t
task_pool_manager_timeout (xpointer_t user_data)
{
  g_mutex_lock (&task_pool_mutex);
  xthread_pool_set_max_threads (task_pool, tasks_running + 1, NULL);
  g_trace_set_int64_counter (task_pool_max_counter, tasks_running + 1);
  xsource_set_ready_time (task_pool_manager, -1);
  g_mutex_unlock (&task_pool_mutex);

  return TRUE;
}

static void
xtask_thread_setup (void)
{
  g_private_set (&task_private, GUINT_TO_POINTER (TRUE));
  g_mutex_lock (&task_pool_mutex);
  tasks_running++;

  g_trace_set_int64_counter (tasks_running_counter, tasks_running);

  if (tasks_running == XTASK_POOL_SIZE)
    task_wait_time = XTASK_WAIT_TIME_BASE;
  else if (tasks_running > XTASK_POOL_SIZE && tasks_running < XTASK_WAIT_TIME_MAX_POOL_SIZE)
    task_wait_time *= XTASK_WAIT_TIME_MULTIPLIER;

  if (tasks_running >= XTASK_POOL_SIZE)
    xsource_set_ready_time (task_pool_manager, g_get_monotonic_time () + task_wait_time);

  g_mutex_unlock (&task_pool_mutex);
}

static void
xtask_thread_cleanup (void)
{
  xint_t tasks_pending;

  g_mutex_lock (&task_pool_mutex);
  tasks_pending = xthread_pool_unprocessed (task_pool);

  if (tasks_running > XTASK_POOL_SIZE)
    {
      xthread_pool_set_max_threads (task_pool, tasks_running - 1, NULL);
      g_trace_set_int64_counter (task_pool_max_counter, tasks_running - 1);
    }
  else if (tasks_running + tasks_pending < XTASK_POOL_SIZE)
    xsource_set_ready_time (task_pool_manager, -1);

  if (tasks_running > XTASK_POOL_SIZE && tasks_running < XTASK_WAIT_TIME_MAX_POOL_SIZE)
    task_wait_time /= XTASK_WAIT_TIME_MULTIPLIER;

  tasks_running--;

  g_trace_set_int64_counter (tasks_running_counter, tasks_running);

  g_mutex_unlock (&task_pool_mutex);
  g_private_set (&task_private, GUINT_TO_POINTER (FALSE));
}

static void
xtask_thread_pool_thread (xpointer_t thread_data,
                           xpointer_t pool_data)
{
  xtask_t *task = thread_data;

  xtask_thread_setup ();

  task->task_func (task, task->source_object, task->task_data,
                   task->cancellable);
  xtask_thread_complete (task);
  xobject_unref (task);

  xtask_thread_cleanup ();
}

static void
task_thread_cancelled (xcancellable_t *cancellable,
                       xpointer_t      user_data)
{
  xtask_t *task = user_data;

  /* Move this task to the front of the queue - no need for
   * a complete resorting of the queue.
   */
  xthread_pool_move_to_front (task_pool, task);

  g_mutex_lock (&task->lock);
  task->thread_cancelled = TRUE;

  if (!task->return_on_cancel)
    {
      g_mutex_unlock (&task->lock);
      return;
    }

  /* We don't actually set task->error; xtask_return_error() doesn't
   * use a lock, and xtask_propagate_error() will call
   * xcancellable_set_error_if_cancelled() anyway.
   */
  g_mutex_unlock (&task->lock);
  xtask_thread_complete (task);
}

static void
task_thread_cancelled_disconnect_notify (xpointer_t  task,
                                         xclosure_t *closure)
{
  xobject_unref (task);
}

static void
xtask_start_task_thread (xtask_t           *task,
                          xtask_thread_func_t  task_func)
{
  g_mutex_init (&task->lock);
  g_cond_init (&task->cond);

  g_mutex_lock (&task->lock);

  TRACE (GIO_TASK_BEFORE_RUN_IN_THREAD (task, task_func));

  task->task_func = task_func;

  if (task->cancellable)
    {
      if (task->return_on_cancel &&
          xcancellable_set_error_if_cancelled (task->cancellable,
                                                &task->error))
        {
          task->thread_cancelled = task->thread_complete = TRUE;
          TRACE (GIO_TASK_AFTER_RUN_IN_THREAD (task, task->thread_cancelled));
          xthread_pool_push (task_pool, xobject_ref (task), NULL);
          return;
        }

      /* This introduces a reference count loop between the xtask_t and
       * xcancellable_t, but is necessary to avoid a race on finalising the xtask_t
       * between task_thread_cancelled() (in one thread) and
       * xtask_thread_complete() (in another).
       *
       * Accordingly, the signal handler *must* be removed once the task has
       * completed.
       */
      xsignal_connect_data (task->cancellable, "cancelled",
                             G_CALLBACK (task_thread_cancelled),
                             xobject_ref (task),
                             task_thread_cancelled_disconnect_notify, 0);
    }

  if (g_private_get (&task_private))
    task->blocking_other_task = TRUE;
  xthread_pool_push (task_pool, xobject_ref (task), NULL);
}

/**
 * xtask_run_in_thread:
 * @task: a #xtask_t
 * @task_func: (scope async): a #xtask_thread_func_t
 *
 * Runs @task_func in another thread. When @task_func returns, @task's
 * #xasync_ready_callback_t will be invoked in @task's #xmain_context_t.
 *
 * This takes a ref on @task until the task completes.
 *
 * See #xtask_thread_func_t for more details about how @task_func is handled.
 *
 * Although GLib currently rate-limits the tasks queued via
 * xtask_run_in_thread(), you should not assume that it will always
 * do this. If you have a very large number of tasks to run (several tens of
 * tasks), but don't want them to all run at once, you should only queue a
 * limited number of them (around ten) at a time.
 *
 * Since: 2.36
 */
void
xtask_run_in_thread (xtask_t           *task,
                      xtask_thread_func_t  task_func)
{
  g_return_if_fail (X_IS_TASK (task));

  xobject_ref (task);
  xtask_start_task_thread (task, task_func);

  /* The task may already be cancelled, or xthread_pool_push() may
   * have failed.
   */
  if (task->thread_complete)
    {
      g_mutex_unlock (&task->lock);
      xtask_return (task, XTASK_RETURN_FROM_THREAD);
    }
  else
    g_mutex_unlock (&task->lock);

  xobject_unref (task);
}

/**
 * xtask_run_in_thread_sync:
 * @task: a #xtask_t
 * @task_func: (scope async): a #xtask_thread_func_t
 *
 * Runs @task_func in another thread, and waits for it to return or be
 * cancelled. You can use xtask_propagate_pointer(), etc, afterward
 * to get the result of @task_func.
 *
 * See #xtask_thread_func_t for more details about how @task_func is handled.
 *
 * Normally this is used with tasks created with a %NULL
 * `callback`, but note that even if the task does
 * have a callback, it will not be invoked when @task_func returns.
 * #xtask_t:completed will be set to %TRUE just before this function returns.
 *
 * Although GLib currently rate-limits the tasks queued via
 * xtask_run_in_thread_sync(), you should not assume that it will
 * always do this. If you have a very large number of tasks to run,
 * but don't want them to all run at once, you should only queue a
 * limited number of them at a time.
 *
 * Since: 2.36
 */
void
xtask_run_in_thread_sync (xtask_t           *task,
                           xtask_thread_func_t  task_func)
{
  g_return_if_fail (X_IS_TASK (task));

  xobject_ref (task);

  task->synchronous = TRUE;
  xtask_start_task_thread (task, task_func);

  while (!task->thread_complete)
    g_cond_wait (&task->cond, &task->lock);

  g_mutex_unlock (&task->lock);

  TRACE (GIO_TASK_BEFORE_RETURN (task, task->source_object,
                                 NULL  /* callback */,
                                 NULL  /* callback data */));

  /* Notify of completion in this thread. */
  task->completed = TRUE;
  xobject_notify (G_OBJECT (task), "completed");

  xobject_unref (task);
}

/**
 * xtask_attach_source:
 * @task: a #xtask_t
 * @source: the source to attach
 * @callback: the callback to invoke when @source triggers
 *
 * A utility function for dealing with async operations where you need
 * to wait for a #xsource_t to trigger. Attaches @source to @task's
 * #xmain_context_t with @task's [priority][io-priority], and sets @source's
 * callback to @callback, with @task as the callback's `user_data`.
 *
 * It will set the @source’s name to the task’s name (as set with
 * xtask_set_name()), if one has been set.
 *
 * This takes a reference on @task until @source is destroyed.
 *
 * Since: 2.36
 */
void
xtask_attach_source (xtask_t       *task,
                      xsource_t     *source,
                      xsource_func_t  callback)
{
  g_return_if_fail (X_IS_TASK (task));

  xsource_set_callback (source, callback,
                         xobject_ref (task), xobject_unref);
  xsource_set_priority (source, task->priority);
  if (task->name != NULL)
    xsource_set_name (source, task->name);

  xsource_attach (source, task->context);
}


static xboolean_t
xtask_propagate_error (xtask_t   *task,
                        xerror_t **error)
{
  xboolean_t error_set;

  if (task->check_cancellable &&
      xcancellable_set_error_if_cancelled (task->cancellable, error))
    error_set = TRUE;
  else if (task->error)
    {
      g_propagate_error (error, task->error);
      task->error = NULL;
      task->had_error = TRUE;
      error_set = TRUE;
    }
  else
    error_set = FALSE;

  TRACE (GIO_TASK_PROPAGATE (task, error_set));

  return error_set;
}

/**
 * xtask_return_pointer:
 * @task: a #xtask_t
 * @result: (nullable) (transfer full): the pointer result of a task
 *     function
 * @result_destroy: (nullable): a #xdestroy_notify_t function.
 *
 * Sets @task's result to @result and completes the task. If @result
 * is not %NULL, then @result_destroy will be used to free @result if
 * the caller does not take ownership of it with
 * xtask_propagate_pointer().
 *
 * "Completes the task" means that for an ordinary asynchronous task
 * it will either invoke the task's callback, or else queue that
 * callback to be invoked in the proper #xmain_context_t, or in the next
 * iteration of the current #xmain_context_t. For a task run via
 * xtask_run_in_thread() or xtask_run_in_thread_sync(), calling this
 * method will save @result to be returned to the caller later, but
 * the task will not actually be completed until the #xtask_thread_func_t
 * exits.
 *
 * Note that since the task may be completed before returning from
 * xtask_return_pointer(), you cannot assume that @result is still
 * valid after calling this, unless you are still holding another
 * reference on it.
 *
 * Since: 2.36
 */
void
xtask_return_pointer (xtask_t          *task,
                       xpointer_t        result,
                       xdestroy_notify_t  result_destroy)
{
  g_return_if_fail (X_IS_TASK (task));
  g_return_if_fail (!task->ever_returned);

  task->result.pointer = result;
  task->result_destroy = result_destroy;

  xtask_return (task, XTASK_RETURN_SUCCESS);
}

/**
 * xtask_propagate_pointer:
 * @task: a #xtask_t
 * @error: return location for a #xerror_t
 *
 * Gets the result of @task as a pointer, and transfers ownership
 * of that value to the caller.
 *
 * If the task resulted in an error, or was cancelled, then this will
 * instead return %NULL and set @error.
 *
 * Since this method transfers ownership of the return value (or
 * error) to the caller, you may only call it once.
 *
 * Returns: (transfer full): the task result, or %NULL on error
 *
 * Since: 2.36
 */
xpointer_t
xtask_propagate_pointer (xtask_t   *task,
                          xerror_t **error)
{
  g_return_val_if_fail (X_IS_TASK (task), NULL);

  if (xtask_propagate_error (task, error))
    return NULL;

  g_return_val_if_fail (task->result_set, NULL);

  task->result_destroy = NULL;
  task->result_set = FALSE;
  return task->result.pointer;
}

/**
 * xtask_return_int:
 * @task: a #xtask_t.
 * @result: the integer (#xssize_t) result of a task function.
 *
 * Sets @task's result to @result and completes the task (see
 * xtask_return_pointer() for more discussion of exactly what this
 * means).
 *
 * Since: 2.36
 */
void
xtask_return_int (xtask_t  *task,
                   xssize_t  result)
{
  g_return_if_fail (X_IS_TASK (task));
  g_return_if_fail (!task->ever_returned);

  task->result.size = result;

  xtask_return (task, XTASK_RETURN_SUCCESS);
}

/**
 * xtask_propagate_int:
 * @task: a #xtask_t.
 * @error: return location for a #xerror_t
 *
 * Gets the result of @task as an integer (#xssize_t).
 *
 * If the task resulted in an error, or was cancelled, then this will
 * instead return -1 and set @error.
 *
 * Since this method transfers ownership of the return value (or
 * error) to the caller, you may only call it once.
 *
 * Returns: the task result, or -1 on error
 *
 * Since: 2.36
 */
xssize_t
xtask_propagate_int (xtask_t   *task,
                      xerror_t **error)
{
  g_return_val_if_fail (X_IS_TASK (task), -1);

  if (xtask_propagate_error (task, error))
    return -1;

  g_return_val_if_fail (task->result_set, -1);

  task->result_set = FALSE;
  return task->result.size;
}

/**
 * xtask_return_boolean:
 * @task: a #xtask_t.
 * @result: the #xboolean_t result of a task function.
 *
 * Sets @task's result to @result and completes the task (see
 * xtask_return_pointer() for more discussion of exactly what this
 * means).
 *
 * Since: 2.36
 */
void
xtask_return_boolean (xtask_t    *task,
                       xboolean_t  result)
{
  g_return_if_fail (X_IS_TASK (task));
  g_return_if_fail (!task->ever_returned);

  task->result.boolean = result;

  xtask_return (task, XTASK_RETURN_SUCCESS);
}

/**
 * xtask_propagate_boolean:
 * @task: a #xtask_t.
 * @error: return location for a #xerror_t
 *
 * Gets the result of @task as a #xboolean_t.
 *
 * If the task resulted in an error, or was cancelled, then this will
 * instead return %FALSE and set @error.
 *
 * Since this method transfers ownership of the return value (or
 * error) to the caller, you may only call it once.
 *
 * Returns: the task result, or %FALSE on error
 *
 * Since: 2.36
 */
xboolean_t
xtask_propagate_boolean (xtask_t   *task,
                          xerror_t **error)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);

  if (xtask_propagate_error (task, error))
    return FALSE;

  g_return_val_if_fail (task->result_set, FALSE);

  task->result_set = FALSE;
  return task->result.boolean;
}

/**
 * xtask_return_error:
 * @task: a #xtask_t.
 * @error: (transfer full): the #xerror_t result of a task function.
 *
 * Sets @task's result to @error (which @task assumes ownership of)
 * and completes the task (see xtask_return_pointer() for more
 * discussion of exactly what this means).
 *
 * Note that since the task takes ownership of @error, and since the
 * task may be completed before returning from xtask_return_error(),
 * you cannot assume that @error is still valid after calling this.
 * Call xerror_copy() on the error if you need to keep a local copy
 * as well.
 *
 * See also xtask_return_new_error().
 *
 * Since: 2.36
 */
void
xtask_return_error (xtask_t  *task,
                     xerror_t *error)
{
  g_return_if_fail (X_IS_TASK (task));
  g_return_if_fail (!task->ever_returned);
  g_return_if_fail (error != NULL);

  task->error = error;

  xtask_return (task, XTASK_RETURN_ERROR);
}

/**
 * xtask_return_new_error:
 * @task: a #xtask_t.
 * @domain: a #xquark.
 * @code: an error code.
 * @format: a string with format characters.
 * @...: a list of values to insert into @format.
 *
 * Sets @task's result to a new #xerror_t created from @domain, @code,
 * @format, and the remaining arguments, and completes the task (see
 * xtask_return_pointer() for more discussion of exactly what this
 * means).
 *
 * See also xtask_return_error().
 *
 * Since: 2.36
 */
void
xtask_return_new_error (xtask_t           *task,
                         xquark           domain,
                         xint_t             code,
                         const char      *format,
                         ...)
{
  xerror_t *error;
  va_list args;

  va_start (args, format);
  error = xerror_new_valist (domain, code, format, args);
  va_end (args);

  xtask_return_error (task, error);
}

/**
 * xtask_return_error_if_cancelled:
 * @task: a #xtask_t
 *
 * Checks if @task's #xcancellable_t has been cancelled, and if so, sets
 * @task's error accordingly and completes the task (see
 * xtask_return_pointer() for more discussion of exactly what this
 * means).
 *
 * Returns: %TRUE if @task has been cancelled, %FALSE if not
 *
 * Since: 2.36
 */
xboolean_t
xtask_return_error_if_cancelled (xtask_t *task)
{
  xerror_t *error = NULL;

  g_return_val_if_fail (X_IS_TASK (task), FALSE);
  g_return_val_if_fail (!task->ever_returned, FALSE);

  if (xcancellable_set_error_if_cancelled (task->cancellable, &error))
    {
      /* We explicitly set task->error so this works even when
       * check-cancellable is not set.
       */
      g_clear_error (&task->error);
      task->error = error;

      xtask_return (task, XTASK_RETURN_ERROR);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * xtask_had_error:
 * @task: a #xtask_t.
 *
 * Tests if @task resulted in an error.
 *
 * Returns: %TRUE if the task resulted in an error, %FALSE otherwise.
 *
 * Since: 2.36
 */
xboolean_t
xtask_had_error (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);

  if (task->error != NULL || task->had_error)
    return TRUE;

  if (task->check_cancellable && xcancellable_is_cancelled (task->cancellable))
    return TRUE;

  return FALSE;
}

static void
value_free (xpointer_t value)
{
  xvalue_unset (value);
  g_free (value);
}

/**
 * xtask_return_value:
 * @task: a #xtask_t
 * @result: (nullable) (transfer none): the #xvalue_t result of
 *                                      a task function
 *
 * Sets @task's result to @result (by copying it) and completes the task.
 *
 * If @result is %NULL then a #xvalue_t of type %XTYPE_POINTER
 * with a value of %NULL will be used for the result.
 *
 * This is a very generic low-level method intended primarily for use
 * by language bindings; for C code, xtask_return_pointer() and the
 * like will normally be much easier to use.
 *
 * Since: 2.64
 */
void
xtask_return_value (xtask_t  *task,
                     xvalue_t *result)
{
  xvalue_t *value;

  g_return_if_fail (X_IS_TASK (task));
  g_return_if_fail (!task->ever_returned);

  value = g_new0 (xvalue_t, 1);

  if (result == NULL)
    {
      xvalue_init (value, XTYPE_POINTER);
      xvalue_set_pointer (value, NULL);
    }
  else
    {
      xvalue_init (value, G_VALUE_TYPE (result));
      xvalue_copy (result, value);
    }

  xtask_return_pointer (task, value, value_free);
}

/**
 * xtask_propagate_value:
 * @task: a #xtask_t
 * @value: (out caller-allocates): return location for the #xvalue_t
 * @error: return location for a #xerror_t
 *
 * Gets the result of @task as a #xvalue_t, and transfers ownership of
 * that value to the caller. As with xtask_return_value(), this is
 * a generic low-level method; xtask_propagate_pointer() and the like
 * will usually be more useful for C code.
 *
 * If the task resulted in an error, or was cancelled, then this will
 * instead set @error and return %FALSE.
 *
 * Since this method transfers ownership of the return value (or
 * error) to the caller, you may only call it once.
 *
 * Returns: %TRUE if @task succeeded, %FALSE on error.
 *
 * Since: 2.64
 */
xboolean_t
xtask_propagate_value (xtask_t   *task,
                        xvalue_t  *value,
                        xerror_t **error)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (xtask_propagate_error (task, error))
    return FALSE;

  g_return_val_if_fail (task->result_set, FALSE);
  g_return_val_if_fail (task->result_destroy == value_free, FALSE);

  memcpy (value, task->result.pointer, sizeof (xvalue_t));
  g_free (task->result.pointer);

  task->result_destroy = NULL;
  task->result_set = FALSE;

  return TRUE;
}

/**
 * xtask_get_completed:
 * @task: a #xtask_t.
 *
 * Gets the value of #xtask_t:completed. This changes from %FALSE to %TRUE after
 * the task’s callback is invoked, and will return %FALSE if called from inside
 * the callback.
 *
 * Returns: %TRUE if the task has completed, %FALSE otherwise.
 *
 * Since: 2.44
 */
xboolean_t
xtask_get_completed (xtask_t *task)
{
  g_return_val_if_fail (X_IS_TASK (task), FALSE);

  /* Convert from a bit field to a boolean. */
  return task->completed ? TRUE : FALSE;
}

/**
 * xtask_is_valid:
 * @result: (type Gio.AsyncResult): A #xasync_result_t
 * @source_object: (nullable) (type xobject_t): the source object
 *   expected to be associated with the task
 *
 * Checks that @result is a #xtask_t, and that @source_object is its
 * source object (or that @source_object is %NULL and @result has no
 * source object). This can be used in g_return_if_fail() checks.
 *
 * Returns: %TRUE if @result and @source_object are valid, %FALSE
 * if not
 *
 * Since: 2.36
 */
xboolean_t
xtask_is_valid (xpointer_t result,
                 xpointer_t source_object)
{
  if (!X_IS_TASK (result))
    return FALSE;

  return XTASK (result)->source_object == source_object;
}

static xint_t
xtask_compare_priority (xconstpointer a,
                         xconstpointer b,
                         xpointer_t      user_data)
{
  const xtask_t *ta = a;
  const xtask_t *tb = b;
  xboolean_t a_cancelled, b_cancelled;

  /* Tasks that are causing other tasks to block have higher
   * priority.
   */
  if (ta->blocking_other_task && !tb->blocking_other_task)
    return -1;
  else if (tb->blocking_other_task && !ta->blocking_other_task)
    return 1;

  /* Let already-cancelled tasks finish right away */
  a_cancelled = (ta->check_cancellable &&
                 xcancellable_is_cancelled (ta->cancellable));
  b_cancelled = (tb->check_cancellable &&
                 xcancellable_is_cancelled (tb->cancellable));
  if (a_cancelled && !b_cancelled)
    return -1;
  else if (b_cancelled && !a_cancelled)
    return 1;

  /* Lower priority == run sooner == negative return value */
  return ta->priority - tb->priority;
}

static xboolean_t
trivial_source_dispatch (xsource_t     *source,
                         xsource_func_t  callback,
                         xpointer_t     user_data)
{
  return callback (user_data);
}

xsource_funcs_t trivial_source_funcs = {
  NULL, /* prepare */
  NULL, /* check */
  trivial_source_dispatch,
  NULL, /* finalize */
  NULL, /* closure */
  NULL  /* marshal */
};

static void
xtask_thread_pool_init (void)
{
  task_pool = xthread_pool_new (xtask_thread_pool_thread, NULL,
                                 XTASK_POOL_SIZE, FALSE, NULL);
  g_assert (task_pool != NULL);

  xthread_pool_set_sort_function (task_pool, xtask_compare_priority, NULL);

  task_pool_manager = xsource_new (&trivial_source_funcs, sizeof (xsource_t));
  xsource_set_static_name (task_pool_manager, "xtask_t thread pool manager");
  xsource_set_callback (task_pool_manager, task_pool_manager_timeout, NULL, NULL);
  xsource_set_ready_time (task_pool_manager, -1);
  xsource_attach (task_pool_manager,
                   XPL_PRIVATE_CALL (g_get_worker_context ()));
  xsource_unref (task_pool_manager);
}

static void
xtask_get_property (xobject_t    *object,
                     xuint_t       prop_id,
                     xvalue_t     *value,
                     xparam_spec_t *pspec)
{
  xtask_t *task = XTASK (object);

  switch ((GTaskProperty) prop_id)
    {
    case PROP_COMPLETED:
      xvalue_set_boolean (value, xtask_get_completed (task));
      break;
    }
}

static void
xtask_class_init (xtask_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xtask_get_property;
  gobject_class->finalize = xtask_finalize;

  /**
   * xtask_t:completed:
   *
   * Whether the task has completed, meaning its callback (if set) has been
   * invoked. This can only happen after xtask_return_pointer(),
   * xtask_return_error() or one of the other return functions have been called
   * on the task.
   *
   * This property is guaranteed to change from %FALSE to %TRUE exactly once.
   *
   * The #xobject_t::notify signal for this change is emitted in the same main
   * context as the task’s callback, immediately after that callback is invoked.
   *
   * Since: 2.44
   */
  xobject_class_install_property (gobject_class, PROP_COMPLETED,
    g_param_spec_boolean ("completed",
                          P_("Task completed"),
                          P_("Whether the task has completed yet"),
                          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  if (G_UNLIKELY (task_pool_max_counter == 0))
    {
      /* We use two counters to track characteristics of the xtask_t thread pool.
       * task pool max size - the value of xthread_pool_set_max_threads()
       * tasks running - the number of running threads
       */
      task_pool_max_counter = g_trace_define_int64_counter ("GIO", "task pool max size", "Maximum number of threads allowed in the xtask_t thread pool; see xthread_pool_set_max_threads()");
      tasks_running_counter = g_trace_define_int64_counter ("GIO", "tasks running", "Number of currently running tasks in the xtask_t thread pool");
    }
}

static xpointer_t
xtask_get_user_data (xasync_result_t *res)
{
  return XTASK (res)->callback_data;
}

static xboolean_t
xtask_is_tagged (xasync_result_t *res,
                  xpointer_t      source_tag)
{
  return XTASK (res)->source_tag == source_tag;
}

static void
xtask_async_result_iface_init (xasync_result_iface_t *iface)
{
  iface->get_user_data = xtask_get_user_data;
  iface->get_source_object = xtask_ref_source_object;
  iface->is_tagged = xtask_is_tagged;
}
