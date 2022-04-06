/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gmain.c: Main loop abstraction, timeouts, and idle functions
 * Copyright 1998 Owen Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"
#include "glibconfig.h"
#include "glib_trace.h"

/* Uncomment the next line (and the corresponding line in gpoll.c) to
 * enable debugging printouts if the environment variable
 * G_MAIN_POLL_DEBUG is set to some value.
 */
/* #define G_MAIN_POLL_DEBUG */

#ifdef _WIN32
/* Always enable debugging printout on Windows, as it is more often
 * needed there...
 */
#define G_MAIN_POLL_DEBUG
#endif

#ifdef G_OS_UNIX
#include "glib-unix.h"
#include <pthread.h>
#ifdef HAVE_EVENTFD
#include <sys/eventfd.h>
#endif
#endif

#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef G_OS_UNIX
#include <unistd.h>
#endif /* G_OS_UNIX */
#include <errno.h>
#include <string.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif /* G_OS_WIN32 */

#ifdef HAVE_MACH_MACH_TIME_H
#include <mach/mach_time.h>
#endif

#include "glib_trace.h"

#include "gmain.h"

#include "garray.h"
#include "giochannel.h"
#include "ghash.h"
#include "ghook.h"
#include "gqueue.h"
#include "gstrfuncs.h"
#include "gtestutils.h"
#include "gthreadprivate.h"
#include "gtrace-private.h"

#ifdef G_OS_WIN32
#include "gwin32.h"
#endif

#ifdef  G_MAIN_POLL_DEBUG
#include "gtimer.h"
#endif

#include "gwakeup.h"
#include "gmain-internal.h"
#include "glib-init.h"
#include "glib-private.h"

/**
 * SECTION:main
 * @title: The Main Event Loop
 * @short_description: manages all available sources of events
 *
 * The main event loop manages all the available sources of events for
 * GLib and GTK+ applications. These events can come from any number of
 * different types of sources such as file descriptors (plain files,
 * pipes or sockets) and timeouts. New types of event sources can also
 * be added using xsource_attach().
 *
 * To allow multiple independent sets of sources to be handled in
 * different threads, each source is associated with a #xmain_context_t.
 * A #xmain_context_t can only be running in a single thread, but
 * sources can be added to it and removed from it from other threads. All
 * functions which operate on a #xmain_context_t or a built-in #xsource_t are
 * thread-safe.
 *
 * Each event source is assigned a priority. The default priority,
 * %G_PRIORITY_DEFAULT, is 0. Values less than 0 denote higher priorities.
 * Values greater than 0 denote lower priorities. Events from high priority
 * sources are always processed before events from lower priority sources: if
 * several sources are ready to dispatch, the ones with equal-highest priority
 * will be dispatched on the current #xmain_context_t iteration, and the rest wait
 * until a subsequent #xmain_context_t iteration when they have the highest
 * priority of the sources which are ready for dispatch.
 *
 * Idle functions can also be added, and assigned a priority. These will
 * be run whenever no events with a higher priority are ready to be dispatched.
 *
 * The #xmain_loop_t data type represents a main event loop. A xmain_loop_t is
 * created with xmain_loop_new(). After adding the initial event sources,
 * xmain_loop_run() is called. This continuously checks for new events from
 * each of the event sources and dispatches them. Finally, the processing of
 * an event from one of the sources leads to a call to xmain_loop_quit() to
 * exit the main loop, and xmain_loop_run() returns.
 *
 * It is possible to create new instances of #xmain_loop_t recursively.
 * This is often used in GTK+ applications when showing modal dialog
 * boxes. Note that event sources are associated with a particular
 * #xmain_context_t, and will be checked and dispatched for all main
 * loops associated with that xmain_context_t.
 *
 * GTK+ contains wrappers of some of these functions, e.g. gtk_main(),
 * gtk_main_quit() and gtk_events_pending().
 *
 * ## Creating new source types
 *
 * One of the unusual features of the #xmain_loop_t functionality
 * is that new types of event source can be created and used in
 * addition to the builtin type of event source. A new event source
 * type is used for handling GDK events. A new source type is created
 * by "deriving" from the #xsource_t structure. The derived type of
 * source is represented by a structure that has the #xsource_t structure
 * as a first element, and other elements specific to the new source
 * type. To create an instance of the new source type, call
 * xsource_new() passing in the size of the derived structure and
 * a table of functions. These #xsource_funcs_t determine the behavior of
 * the new source type.
 *
 * New source types basically interact with the main context
 * in two ways. Their prepare function in #xsource_funcs_t can set a timeout
 * to determine the maximum amount of time that the main loop will sleep
 * before checking the source again. In addition, or as well, the source
 * can add file descriptors to the set that the main context checks using
 * xsource_add_poll().
 *
 * ## Customizing the main loop iteration
 *
 * Single iterations of a #xmain_context_t can be run with
 * xmain_context_iteration(). In some cases, more detailed control
 * of exactly how the details of the main loop work is desired, for
 * instance, when integrating the #xmain_loop_t with an external main loop.
 * In such cases, you can call the component functions of
 * xmain_context_iteration() directly. These functions are
 * xmain_context_prepare(), xmain_context_query(),
 * xmain_context_check() and xmain_context_dispatch().
 *
 * If the event loop thread releases #xmain_context_t ownership until the results
 * required by xmain_context_check() are ready you must create a context with
 * the flag %XMAIN_CONTEXT_FLAGS_OWNERLESS_POLLING or else you'll lose
 * xsource_attach() notifications. This happens for instance when you integrate
 * the GLib event loop into implementations that follow the proactor pattern
 * (i.e. in these contexts the `poll()` implementation will reclaim the thread for
 * other tasks until the results are ready). One example of the proactor pattern
 * is the Boost.Asio library.
 *
 * ## State of a Main Context # {#mainloop-states}
 *
 * The operation of these functions can best be seen in terms
 * of a state diagram, as shown in this image.
 *
 * ![](mainloop-states.gif)
 *
 * On UNIX, the GLib mainloop is incompatible with fork(). Any program
 * using the mainloop must either exec() or exit() from the child
 * without returning to the mainloop.
 *
 * ## Memory management of sources # {#mainloop-memory-management}
 *
 * There are two options for memory management of the user data passed to a
 * #xsource_t to be passed to its callback on invocation. This data is provided
 * in calls to g_timeout_add(), g_timeout_add_full(), g_idle_add(), etc. and
 * more generally, using xsource_set_callback(). This data is typically an
 * object which ‘owns’ the timeout or idle callback, such as a widget or a
 * network protocol implementation. In many cases, it is an error for the
 * callback to be invoked after this owning object has been destroyed, as that
 * results in use of freed memory.
 *
 * The first, and preferred, option is to store the source ID returned by
 * functions such as g_timeout_add() or xsource_attach(), and explicitly
 * remove that source from the main context using xsource_remove() when the
 * owning object is finalized. This ensures that the callback can only be
 * invoked while the object is still alive.
 *
 * The second option is to hold a strong reference to the object in the
 * callback, and to release it in the callback’s #xdestroy_notify_t. This ensures
 * that the object is kept alive until after the source is finalized, which is
 * guaranteed to be after it is invoked for the final time. The #xdestroy_notify_t
 * is another callback passed to the ‘full’ variants of #xsource_t functions (for
 * example, g_timeout_add_full()). It is called when the source is finalized,
 * and is designed for releasing references like this.
 *
 * One important caveat of this second approach is that it will keep the object
 * alive indefinitely if the main loop is stopped before the #xsource_t is
 * invoked, which may be undesirable.
 */

/* Types */

typedef struct _GTimeoutSource GTimeoutSource;
typedef struct _GChildWatchSource GChildWatchSource;
typedef struct _GUnixSignalWatchSource GUnixSignalWatchSource;
typedef struct _GPollRec GPollRec;
typedef struct _GSourceCallback xsource_callback_t;

typedef enum
{
  G_SOURCE_READY = 1 << G_HOOK_FLAG_USER_SHIFT,
  G_SOURCE_CAN_RECURSE = 1 << (G_HOOK_FLAG_USER_SHIFT + 1),
  G_SOURCE_BLOCKED = 1 << (G_HOOK_FLAG_USER_SHIFT + 2)
} GSourceFlags;

typedef struct _GSourceList GSourceList;

struct _GSourceList
{
  xsource_t *head, *tail;
  xint_t priority;
};

typedef struct _GMainWaiter GMainWaiter;

struct _GMainWaiter
{
  xcond_t *cond;
  xmutex_t *mutex;
};

typedef struct _GMainDispatch GMainDispatch;

struct _GMainDispatch
{
  xint_t depth;
  xsource_t *source;
};

#ifdef G_MAIN_POLL_DEBUG
xboolean_t _g_main_poll_debug = FALSE;
#endif

struct _GMainContext
{
  /* The following lock is used for both the list of sources
   * and the list of poll records
   */
  xmutex_t mutex;
  xcond_t cond;
  xthread_t *owner;
  xuint_t owner_count;
  GMainContextFlags flags;
  xslist_t *waiters;

  xint_t ref_count;  /* (atomic) */

  xhashtable_t *sources;              /* xuint_t -> xsource_t */

  xptr_array_t *pending_dispatches;
  xint_t timeout;			/* Timeout for current iteration */

  xuint_t next_id;
  xlist_t *source_lists;
  xint_t in_check_or_prepare;

  GPollRec *poll_records;
  xuint_t n_poll_records;
  xpollfd_t *cached_poll_array;
  xuint_t cached_poll_array_size;

  GWakeup *wakeup;

  xpollfd_t wake_up_rec;

/* Flag indicating whether the set of fd's changed during a poll */
  xboolean_t poll_changed;

  GPollFunc poll_func;

  gint64   time;
  xboolean_t time_is_fresh;
};

struct _GSourceCallback
{
  xint_t ref_count;  /* (atomic) */
  xsource_func_t func;
  xpointer_t    data;
  xdestroy_notify_t notify;
};

struct _GMainLoop
{
  xmain_context_t *context;
  xboolean_t is_running; /* (atomic) */
  xint_t ref_count;  /* (atomic) */
};

struct _GTimeoutSource
{
  xsource_t     source;
  /* Measured in seconds if 'seconds' is TRUE, or milliseconds otherwise. */
  xuint_t       interval;
  xboolean_t    seconds;
};

struct _GChildWatchSource
{
  xsource_t     source;
  xpid_t        pid;
  xint_t        child_status;
#ifdef G_OS_WIN32
  xpollfd_t     poll;
#else /* G_OS_WIN32 */
  xboolean_t    child_exited; /* (atomic) */
#endif /* G_OS_WIN32 */
};

struct _GUnixSignalWatchSource
{
  xsource_t     source;
  int         signum;
  xboolean_t    pending; /* (atomic) */
};

struct _GPollRec
{
  xpollfd_t *fd;
  GPollRec *prev;
  GPollRec *next;
  xint_t priority;
};

struct _GSourcePrivate
{
  xslist_t *child_sources;
  xsource_t *parent_source;

  gint64 ready_time;

  /* This is currently only used on UNIX, but we always declare it (and
   * let it remain empty on Windows) to avoid #ifdef all over the place.
   */
  xslist_t *fds;

  GSourceDisposeFunc dispose;

  xboolean_t static_name;
};

typedef struct _GSourceIter
{
  xmain_context_t *context;
  xboolean_t may_modify;
  xlist_t *current_list;
  xsource_t *source;
} GSourceIter;

#define LOCK_CONTEXT(context) g_mutex_lock (&context->mutex)
#define UNLOCK_CONTEXT(context) g_mutex_unlock (&context->mutex)
#define G_THREAD_SELF xthread_self ()

#define SOURCE_DESTROYED(source) (((source)->flags & G_HOOK_FLAG_ACTIVE) == 0)
#define SOURCE_BLOCKED(source) (((source)->flags & G_SOURCE_BLOCKED) != 0)

/* Forward declarations */

static void xsource_unref_internal             (xsource_t      *source,
						 xmain_context_t *context,
						 xboolean_t      have_lock);
static void xsource_destroy_internal           (xsource_t      *source,
						 xmain_context_t *context,
						 xboolean_t      have_lock);
static void xsource_set_priority_unlocked      (xsource_t      *source,
						 xmain_context_t *context,
						 xint_t          priority);
static void g_child_source_remove_internal      (xsource_t      *child_source,
                                                 xmain_context_t *context);

static void xmain_context_poll                 (xmain_context_t *context,
						 xint_t          timeout,
						 xint_t          priority,
						 xpollfd_t      *fds,
						 xint_t          n_fds);
static void xmain_context_add_poll_unlocked    (xmain_context_t *context,
						 xint_t          priority,
						 xpollfd_t      *fd);
static void xmain_context_remove_poll_unlocked (xmain_context_t *context,
						 xpollfd_t      *fd);

static void     xsource_iter_init  (GSourceIter   *iter,
				     xmain_context_t  *context,
				     xboolean_t       may_modify);
static xboolean_t xsource_iter_next  (GSourceIter   *iter,
				     xsource_t      **source);
static void     xsource_iter_clear (GSourceIter   *iter);

static xboolean_t g_timeout_dispatch (xsource_t     *source,
				    xsource_func_t  callback,
				    xpointer_t     user_data);
static xboolean_t g_child_watch_prepare  (xsource_t     *source,
				        xint_t        *timeout);
static xboolean_t g_child_watch_check    (xsource_t     *source);
static xboolean_t g_child_watch_dispatch (xsource_t     *source,
					xsource_func_t  callback,
					xpointer_t     user_data);
static void     g_child_watch_finalize (xsource_t     *source);
#ifdef G_OS_UNIX
static void g_unix_signal_handler (int signum);
static xboolean_t g_unix_signal_watch_prepare  (xsource_t     *source,
					      xint_t        *timeout);
static xboolean_t g_unix_signal_watch_check    (xsource_t     *source);
static xboolean_t g_unix_signal_watch_dispatch (xsource_t     *source,
					      xsource_func_t  callback,
					      xpointer_t     user_data);
static void     g_unix_signal_watch_finalize  (xsource_t     *source);
#endif
static xboolean_t g_idle_prepare     (xsource_t     *source,
				    xint_t        *timeout);
static xboolean_t g_idle_check       (xsource_t     *source);
static xboolean_t g_idle_dispatch    (xsource_t     *source,
				    xsource_func_t  callback,
				    xpointer_t     user_data);

static void block_source (xsource_t *source);

static xmain_context_t *glib_worker_context;

#ifndef G_OS_WIN32


/* UNIX signals work by marking one of these variables then waking the
 * worker context to check on them and dispatch accordingly.
 *
 * Both variables must be accessed using atomic primitives, unless those atomic
 * primitives are implemented using fallback mutexes (as those aren’t safe in
 * an interrupt context).
 *
 * If using atomic primitives, the variables must be of type `int` (so they’re
 * the right size for the atomic primitives). Otherwise, use `sig_atomic_t` if
 * it’s available, which is guaranteed to be async-signal-safe (but it’s *not*
 * guaranteed to be thread-safe, which is why we use atomic primitives if
 * possible).
 *
 * Typically, `sig_atomic_t` is a typedef to `int`, but that’s not the case on
 * FreeBSD, so we can’t use it unconditionally if it’s defined.
 */
#if (defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)) || !defined(HAVE_SIG_ATOMIC_T)
static volatile int unix_signal_pending[NSIG];
static volatile int any_unix_signal_pending;
#else
static volatile sig_atomic_t unix_signal_pending[NSIG];
static volatile sig_atomic_t any_unix_signal_pending;
#endif

/* Guards all the data below */
G_LOCK_DEFINE_STATIC (unix_signal_lock);
static xuint_t unix_signal_refcount[NSIG];
static xslist_t *unix_signal_watches;
static xslist_t *unix_child_watches;

xsource_funcs_t g_unix_signal_funcs =
{
  g_unix_signal_watch_prepare,
  g_unix_signal_watch_check,
  g_unix_signal_watch_dispatch,
  g_unix_signal_watch_finalize,
  NULL, NULL
};
#endif /* !G_OS_WIN32 */
G_LOCK_DEFINE_STATIC (main_context_list);
static xslist_t *main_context_list = NULL;

xsource_funcs_t g_timeout_funcs =
{
  NULL, /* prepare */
  NULL, /* check */
  g_timeout_dispatch,
  NULL, NULL, NULL
};

xsource_funcs_t g_child_watch_funcs =
{
  g_child_watch_prepare,
  g_child_watch_check,
  g_child_watch_dispatch,
  g_child_watch_finalize,
  NULL, NULL
};

xsource_funcs_t g_idle_funcs =
{
  g_idle_prepare,
  g_idle_check,
  g_idle_dispatch,
  NULL, NULL, NULL
};

/**
 * xmain_context_ref:
 * @context: a #xmain_context_t
 *
 * Increases the reference count on a #xmain_context_t object by one.
 *
 * Returns: the @context that was passed in (since 2.6)
 **/
xmain_context_t *
xmain_context_ref (xmain_context_t *context)
{
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&context->ref_count) > 0, NULL);

  g_atomic_int_inc (&context->ref_count);

  return context;
}

static inline void
poll_rec_list_free (xmain_context_t *context,
		    GPollRec     *list)
{
  g_slice_free_chain (GPollRec, list, next);
}

/**
 * xmain_context_unref:
 * @context: a #xmain_context_t
 *
 * Decreases the reference count on a #xmain_context_t object by one. If
 * the result is zero, free the context and free all associated memory.
 **/
void
xmain_context_unref (xmain_context_t *context)
{
  GSourceIter iter;
  xsource_t *source;
  xlist_t *sl_iter;
  xslist_t *s_iter, *remaininxsources = NULL;
  GSourceList *list;
  xuint_t i;

  g_return_if_fail (context != NULL);
  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);

  if (!g_atomic_int_dec_and_test (&context->ref_count))
    return;

  G_LOCK (main_context_list);
  main_context_list = xslist_remove (main_context_list, context);
  G_UNLOCK (main_context_list);

  /* Free pending dispatches */
  for (i = 0; i < context->pending_dispatches->len; i++)
    xsource_unref_internal (context->pending_dispatches->pdata[i], context, FALSE);

  /* xsource_iter_next() assumes the context is locked. */
  LOCK_CONTEXT (context);

  /* First collect all remaining sources from the sources lists and store a
   * new reference in a separate list. Also set the context of the sources
   * to NULL so that they can't access a partially destroyed context anymore.
   *
   * We have to do this first so that we have a strong reference to all
   * sources and destroying them below does not also free them, and so that
   * none of the sources can access the context from their finalize/dispose
   * functions. */
  xsource_iter_init (&iter, context, FALSE);
  while (xsource_iter_next (&iter, &source))
    {
      source->context = NULL;
      remaininxsources = xslist_prepend (remaininxsources, xsource_ref (source));
    }
  xsource_iter_clear (&iter);

  /* Next destroy all sources. As we still hold a reference to all of them,
   * this won't cause any of them to be freed yet and especially prevents any
   * source that unrefs another source from its finalize function to be freed.
   */
  for (s_iter = remaininxsources; s_iter; s_iter = s_iter->next)
    {
      source = s_iter->data;
      xsource_destroy_internal (source, context, TRUE);
    }

  for (sl_iter = context->source_lists; sl_iter; sl_iter = sl_iter->next)
    {
      list = sl_iter->data;
      g_slice_free (GSourceList, list);
    }
  xlist_free (context->source_lists);

  xhash_table_destroy (context->sources);

  UNLOCK_CONTEXT (context);
  g_mutex_clear (&context->mutex);

  xptr_array_free (context->pending_dispatches, TRUE);
  g_free (context->cached_poll_array);

  poll_rec_list_free (context, context->poll_records);

  g_wakeup_free (context->wakeup);
  g_cond_clear (&context->cond);

  g_free (context);

  /* And now finally get rid of our references to the sources. This will cause
   * them to be freed unless something else still has a reference to them. Due
   * to setting the context pointers in the sources to NULL above, this won't
   * ever access the context or the internal linked list inside the xsource_t.
   * We already removed the sources completely from the context above. */
  for (s_iter = remaininxsources; s_iter; s_iter = s_iter->next)
    {
      source = s_iter->data;
      xsource_unref_internal (source, NULL, FALSE);
    }
  xslist_free (remaininxsources);
}

/* Helper function used by mainloop/overflow test.
 */
xmain_context_t *
xmain_context_new_with_next_id (xuint_t next_id)
{
  xmain_context_t *ret = xmain_context_new ();

  ret->next_id = next_id;

  return ret;
}

/**
 * xmain_context_new:
 *
 * Creates a new #xmain_context_t structure.
 *
 * Returns: the new #xmain_context_t
 **/
xmain_context_t *
xmain_context_new (void)
{
  return xmain_context_new_with_flags (XMAIN_CONTEXT_FLAGS_NONE);
}

/**
 * xmain_context_new_with_flags:
 * @flags: a bitwise-OR combination of #GMainContextFlags flags that can only be
 *         set at creation time.
 *
 * Creates a new #xmain_context_t structure.
 *
 * Returns: (transfer full): the new #xmain_context_t
 *
 * Since: 2.72
 */
xmain_context_t *
xmain_context_new_with_flags (GMainContextFlags flags)
{
  static xsize_t initialised;
  xmain_context_t *context;

  if (g_once_init_enter (&initialised))
    {
#ifdef G_MAIN_POLL_DEBUG
      if (g_getenv ("G_MAIN_POLL_DEBUG") != NULL)
        _g_main_poll_debug = TRUE;
#endif

      g_once_init_leave (&initialised, TRUE);
    }

  context = g_new0 (xmain_context_t, 1);

  TRACE (XPL_MAIN_CONTEXT_NEW (context));

  g_mutex_init (&context->mutex);
  g_cond_init (&context->cond);

  context->sources = xhash_table_new (NULL, NULL);
  context->owner = NULL;
  context->flags = flags;
  context->waiters = NULL;

  context->ref_count = 1;

  context->next_id = 1;

  context->source_lists = NULL;

  context->poll_func = g_poll;

  context->cached_poll_array = NULL;
  context->cached_poll_array_size = 0;

  context->pending_dispatches = xptr_array_new ();

  context->time_is_fresh = FALSE;

  context->wakeup = g_wakeup_new ();
  g_wakeup_get_pollfd (context->wakeup, &context->wake_up_rec);
  xmain_context_add_poll_unlocked (context, 0, &context->wake_up_rec);

  G_LOCK (main_context_list);
  main_context_list = xslist_append (main_context_list, context);

#ifdef G_MAIN_POLL_DEBUG
  if (_g_main_poll_debug)
    g_print ("created context=%p\n", context);
#endif

  G_UNLOCK (main_context_list);

  return context;
}

/**
 * xmain_context_default:
 *
 * Returns the global default main context. This is the main context
 * used for main loop functions when a main loop is not explicitly
 * specified, and corresponds to the "main" main loop. See also
 * xmain_context_get_thread_default().
 *
 * Returns: (transfer none): the global default main context.
 **/
xmain_context_t *
xmain_context_default (void)
{
  static xmain_context_t *default_main_context = NULL;

  if (g_once_init_enter (&default_main_context))
    {
      xmain_context_t *context;

      context = xmain_context_new ();

      TRACE (XPL_MAIN_CONTEXT_DEFAULT (context));

#ifdef G_MAIN_POLL_DEBUG
      if (_g_main_poll_debug)
        g_print ("default context=%p\n", context);
#endif

      g_once_init_leave (&default_main_context, context);
    }

  return default_main_context;
}

static void
free_context (xpointer_t data)
{
  xmain_context_t *context = data;

  TRACE (XPL_MAIN_CONTEXT_FREE (context));

  xmain_context_release (context);
  if (context)
    xmain_context_unref (context);
}

static void
free_context_stack (xpointer_t data)
{
  g_queue_free_full((xqueue_t *) data, (xdestroy_notify_t) free_context);
}

static GPrivate thread_context_stack = G_PRIVATE_INIT (free_context_stack);

/**
 * xmain_context_push_thread_default:
 * @context: (nullable): a #xmain_context_t, or %NULL for the global default context
 *
 * Acquires @context and sets it as the thread-default context for the
 * current thread. This will cause certain asynchronous operations
 * (such as most [gio][gio]-based I/O) which are
 * started in this thread to run under @context and deliver their
 * results to its main loop, rather than running under the global
 * default context in the main thread. Note that calling this function
 * changes the context returned by xmain_context_get_thread_default(),
 * not the one returned by xmain_context_default(), so it does not affect
 * the context used by functions like g_idle_add().
 *
 * Normally you would call this function shortly after creating a new
 * thread, passing it a #xmain_context_t which will be run by a
 * #xmain_loop_t in that thread, to set a new default context for all
 * async operations in that thread. In this case you may not need to
 * ever call xmain_context_pop_thread_default(), assuming you want the
 * new #xmain_context_t to be the default for the whole lifecycle of the
 * thread.
 *
 * If you don't have control over how the new thread was created (e.g.
 * in the new thread isn't newly created, or if the thread life
 * cycle is managed by a #GThreadPool), it is always suggested to wrap
 * the logic that needs to use the new #xmain_context_t inside a
 * xmain_context_push_thread_default() / xmain_context_pop_thread_default()
 * pair, otherwise threads that are re-used will end up never explicitly
 * releasing the #xmain_context_t reference they hold.
 *
 * In some cases you may want to schedule a single operation in a
 * non-default context, or temporarily use a non-default context in
 * the main thread. In that case, you can wrap the call to the
 * asynchronous operation inside a
 * xmain_context_push_thread_default() /
 * xmain_context_pop_thread_default() pair, but it is up to you to
 * ensure that no other asynchronous operations accidentally get
 * started while the non-default context is active.
 *
 * Beware that libraries that predate this function may not correctly
 * handle being used from a thread with a thread-default context. Eg,
 * see xfile_supports_thread_contexts().
 *
 * Since: 2.22
 **/
void
xmain_context_push_thread_default (xmain_context_t *context)
{
  xqueue_t *stack;
  xboolean_t acquired_context;

  acquired_context = xmain_context_acquire (context);
  g_return_if_fail (acquired_context);

  if (context == xmain_context_default ())
    context = NULL;
  else if (context)
    xmain_context_ref (context);

  stack = g_private_get (&thread_context_stack);
  if (!stack)
    {
      stack = g_queue_new ();
      g_private_set (&thread_context_stack, stack);
    }

  g_queue_push_head (stack, context);

  TRACE (XPL_MAIN_CONTEXT_PUSH_THREAD_DEFAULT (context));
}

/**
 * xmain_context_pop_thread_default:
 * @context: (nullable): a #xmain_context_t object, or %NULL
 *
 * Pops @context off the thread-default context stack (verifying that
 * it was on the top of the stack).
 *
 * Since: 2.22
 **/
void
xmain_context_pop_thread_default (xmain_context_t *context)
{
  xqueue_t *stack;

  if (context == xmain_context_default ())
    context = NULL;

  stack = g_private_get (&thread_context_stack);

  g_return_if_fail (stack != NULL);
  g_return_if_fail (g_queue_peek_head (stack) == context);

  TRACE (XPL_MAIN_CONTEXT_POP_THREAD_DEFAULT (context));

  g_queue_pop_head (stack);

  xmain_context_release (context);
  if (context)
    xmain_context_unref (context);
}

/**
 * xmain_context_get_thread_default:
 *
 * Gets the thread-default #xmain_context_t for this thread. Asynchronous
 * operations that want to be able to be run in contexts other than
 * the default one should call this method or
 * xmain_context_ref_thread_default() to get a #xmain_context_t to add
 * their #GSources to. (Note that even in single-threaded
 * programs applications may sometimes want to temporarily push a
 * non-default context, so it is not safe to assume that this will
 * always return %NULL if you are running in the default thread.)
 *
 * If you need to hold a reference on the context, use
 * xmain_context_ref_thread_default() instead.
 *
 * Returns: (transfer none) (nullable): the thread-default #xmain_context_t, or
 * %NULL if the thread-default context is the global default context.
 *
 * Since: 2.22
 **/
xmain_context_t *
xmain_context_get_thread_default (void)
{
  xqueue_t *stack;

  stack = g_private_get (&thread_context_stack);
  if (stack)
    return g_queue_peek_head (stack);
  else
    return NULL;
}

/**
 * xmain_context_ref_thread_default:
 *
 * Gets the thread-default #xmain_context_t for this thread, as with
 * xmain_context_get_thread_default(), but also adds a reference to
 * it with xmain_context_ref(). In addition, unlike
 * xmain_context_get_thread_default(), if the thread-default context
 * is the global default context, this will return that #xmain_context_t
 * (with a ref added to it) rather than returning %NULL.
 *
 * Returns: (transfer full): the thread-default #xmain_context_t. Unref
 *     with xmain_context_unref() when you are done with it.
 *
 * Since: 2.32
 */
xmain_context_t *
xmain_context_ref_thread_default (void)
{
  xmain_context_t *context;

  context = xmain_context_get_thread_default ();
  if (!context)
    context = xmain_context_default ();
  return xmain_context_ref (context);
}

/* Hooks for adding to the main loop */

/**
 * xsource_new:
 * @source_funcs: structure containing functions that implement
 *                the sources behavior.
 * @struct_size: size of the #xsource_t structure to create.
 *
 * Creates a new #xsource_t structure. The size is specified to
 * allow creating structures derived from #xsource_t that contain
 * additional data. The size passed in must be at least
 * `sizeof (xsource_t)`.
 *
 * The source will not initially be associated with any #xmain_context_t
 * and must be added to one with xsource_attach() before it will be
 * executed.
 *
 * Returns: the newly-created #xsource_t.
 **/
xsource_t *
xsource_new (xsource_funcs_t *source_funcs,
	      xuint_t         struct_size)
{
  xsource_t *source;

  g_return_val_if_fail (source_funcs != NULL, NULL);
  g_return_val_if_fail (struct_size >= sizeof (xsource_t), NULL);

  source = (xsource_t*) g_malloc0 (struct_size);
  source->priv = g_slice_new0 (xsource_private_t);
  source->source_funcs = source_funcs;
  source->ref_count = 1;

  source->priority = G_PRIORITY_DEFAULT;

  source->flags = G_HOOK_FLAG_ACTIVE;

  source->priv->ready_time = -1;

  /* NULL/0 initialization for all other fields */

  TRACE (XPL_SOURCE_NEW (source, source_funcs->prepare, source_funcs->check,
                          source_funcs->dispatch, source_funcs->finalize,
                          struct_size));

  return source;
}

/**
 * xsource_set_dispose_function:
 * @source: A #xsource_t to set the dispose function on
 * @dispose: #GSourceDisposeFunc to set on the source
 *
 * Set @dispose as dispose function on @source. @dispose will be called once
 * the reference count of @source reaches 0 but before any of the state of the
 * source is freed, especially before the finalize function is called.
 *
 * This means that at this point @source is still a valid #xsource_t and it is
 * allow for the reference count to increase again until @dispose returns.
 *
 * The dispose function can be used to clear any "weak" references to the
 * @source in other data structures in a thread-safe way where it is possible
 * for another thread to increase the reference count of @source again while
 * it is being freed.
 *
 * The finalize function can not be used for this purpose as at that point
 * @source is already partially freed and not valid anymore.
 *
 * This should only ever be called from #xsource_t implementations.
 *
 * Since: 2.64
 **/
void
xsource_set_dispose_function (xsource_t            *source,
			       GSourceDisposeFunc  dispose)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (source->priv->dispose == NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  source->priv->dispose = dispose;
}

/* Holds context's lock */
static void
xsource_iter_init (GSourceIter  *iter,
		    xmain_context_t *context,
		    xboolean_t      may_modify)
{
  iter->context = context;
  iter->current_list = NULL;
  iter->source = NULL;
  iter->may_modify = may_modify;
}

/* Holds context's lock */
static xboolean_t
xsource_iter_next (GSourceIter *iter, xsource_t **source)
{
  xsource_t *next_source;

  if (iter->source)
    next_source = iter->source->next;
  else
    next_source = NULL;

  if (!next_source)
    {
      if (iter->current_list)
	iter->current_list = iter->current_list->next;
      else
	iter->current_list = iter->context->source_lists;

      if (iter->current_list)
	{
	  GSourceList *source_list = iter->current_list->data;

	  next_source = source_list->head;
	}
    }

  /* Note: unreffing iter->source could potentially cause its
   * GSourceList to be removed from source_lists (if iter->source is
   * the only source in its list, and it is destroyed), so we have to
   * keep it reffed until after we advance iter->current_list, above.
   *
   * Also we first have to ref the next source before unreffing the
   * previous one as unreffing the previous source can potentially
   * free the next one.
   */
  if (next_source && iter->may_modify)
    xsource_ref (next_source);

  if (iter->source && iter->may_modify)
    xsource_unref_internal (iter->source, iter->context, TRUE);
  iter->source = next_source;

  *source = iter->source;
  return *source != NULL;
}

/* Holds context's lock. Only necessary to call if you broke out of
 * the xsource_iter_next() loop early.
 */
static void
xsource_iter_clear (GSourceIter *iter)
{
  if (iter->source && iter->may_modify)
    {
      xsource_unref_internal (iter->source, iter->context, TRUE);
      iter->source = NULL;
    }
}

/* Holds context's lock
 */
static GSourceList *
find_source_list_for_priority (xmain_context_t *context,
			       xint_t          priority,
			       xboolean_t      create)
{
  xlist_t *iter, *last;
  GSourceList *source_list;

  last = NULL;
  for (iter = context->source_lists; iter != NULL; last = iter, iter = iter->next)
    {
      source_list = iter->data;

      if (source_list->priority == priority)
	return source_list;

      if (source_list->priority > priority)
	{
	  if (!create)
	    return NULL;

	  source_list = g_slice_new0 (GSourceList);
	  source_list->priority = priority;
	  context->source_lists = xlist_insert_before (context->source_lists,
							iter,
							source_list);
	  return source_list;
	}
    }

  if (!create)
    return NULL;

  source_list = g_slice_new0 (GSourceList);
  source_list->priority = priority;

  if (!last)
    context->source_lists = xlist_append (NULL, source_list);
  else
    {
      /* This just appends source_list to the end of
       * context->source_lists without having to walk the list again.
       */
      last = xlist_append (last, source_list);
      (void) last;
    }
  return source_list;
}

/* Holds context's lock
 */
static void
source_add_to_context (xsource_t      *source,
		       xmain_context_t *context)
{
  GSourceList *source_list;
  xsource_t *prev, *next;

  source_list = find_source_list_for_priority (context, source->priority, TRUE);

  if (source->priv->parent_source)
    {
      g_assert (source_list->head != NULL);

      /* Put the source immediately before its parent */
      prev = source->priv->parent_source->prev;
      next = source->priv->parent_source;
    }
  else
    {
      prev = source_list->tail;
      next = NULL;
    }

  source->next = next;
  if (next)
    next->prev = source;
  else
    source_list->tail = source;

  source->prev = prev;
  if (prev)
    prev->next = source;
  else
    source_list->head = source;
}

/* Holds context's lock
 */
static void
source_remove_from_context (xsource_t      *source,
			    xmain_context_t *context)
{
  GSourceList *source_list;

  source_list = find_source_list_for_priority (context, source->priority, FALSE);
  g_return_if_fail (source_list != NULL);

  if (source->prev)
    source->prev->next = source->next;
  else
    source_list->head = source->next;

  if (source->next)
    source->next->prev = source->prev;
  else
    source_list->tail = source->prev;

  source->prev = NULL;
  source->next = NULL;

  if (source_list->head == NULL)
    {
      context->source_lists = xlist_remove (context->source_lists, source_list);
      g_slice_free (GSourceList, source_list);
    }
}

static xuint_t
xsource_attach_unlocked (xsource_t      *source,
                          xmain_context_t *context,
                          xboolean_t      do_wakeup)
{
  xslist_t *tmp_list;
  xuint_t id;

  /* The counter may have wrapped, so we must ensure that we do not
   * reuse the source id of an existing source.
   */
  do
    id = context->next_id++;
  while (id == 0 || xhash_table_contains (context->sources, GUINT_TO_POINTER (id)));

  source->context = context;
  source->source_id = id;
  xsource_ref (source);

  xhash_table_insert (context->sources, GUINT_TO_POINTER (id), source);

  source_add_to_context (source, context);

  if (!SOURCE_BLOCKED (source))
    {
      tmp_list = source->poll_fds;
      while (tmp_list)
        {
          xmain_context_add_poll_unlocked (context, source->priority, tmp_list->data);
          tmp_list = tmp_list->next;
        }

      for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next)
        xmain_context_add_poll_unlocked (context, source->priority, tmp_list->data);
    }

  tmp_list = source->priv->child_sources;
  while (tmp_list)
    {
      xsource_attach_unlocked (tmp_list->data, context, FALSE);
      tmp_list = tmp_list->next;
    }

  /* If another thread has acquired the context, wake it up since it
   * might be in poll() right now.
   */
  if (do_wakeup &&
      (context->flags & XMAIN_CONTEXT_FLAGS_OWNERLESS_POLLING ||
       (context->owner && context->owner != G_THREAD_SELF)))
    {
      g_wakeup_signal (context->wakeup);
    }

  g_trace_mark (G_TRACE_CURRENT_TIME, 0,
                "GLib", "xsource_attach",
                "%s to context %p",
                (xsource_get_name (source) != NULL) ? xsource_get_name (source) : "(unnamed)",
                context);

  return source->source_id;
}

/**
 * xsource_attach:
 * @source: a #xsource_t
 * @context: (nullable): a #xmain_context_t (if %NULL, the default context will be used)
 *
 * Adds a #xsource_t to a @context so that it will be executed within
 * that context. Remove it by calling xsource_destroy().
 *
 * This function is safe to call from any thread, regardless of which thread
 * the @context is running in.
 *
 * Returns: the ID (greater than 0) for the source within the
 *   #xmain_context_t.
 **/
xuint_t
xsource_attach (xsource_t      *source,
		 xmain_context_t *context)
{
  xuint_t result = 0;

  g_return_val_if_fail (source != NULL, 0);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, 0);
  g_return_val_if_fail (source->context == NULL, 0);
  g_return_val_if_fail (!SOURCE_DESTROYED (source), 0);

  if (!context)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);

  result = xsource_attach_unlocked (source, context, TRUE);

  TRACE (XPL_MAIN_SOURCE_ATTACH (xsource_get_name (source), source, context,
                                  result));

  UNLOCK_CONTEXT (context);

  return result;
}

static void
xsource_destroy_internal (xsource_t      *source,
			   xmain_context_t *context,
			   xboolean_t      have_lock)
{
  TRACE (XPL_MAIN_SOURCE_DESTROY (xsource_get_name (source), source,
                                   context));

  if (!have_lock)
    LOCK_CONTEXT (context);

  if (!SOURCE_DESTROYED (source))
    {
      xslist_t *tmp_list;
      xpointer_t old_cb_data;
      xsource_callback_funcs_t *old_cb_funcs;

      source->flags &= ~G_HOOK_FLAG_ACTIVE;

      old_cb_data = source->callback_data;
      old_cb_funcs = source->callback_funcs;

      source->callback_data = NULL;
      source->callback_funcs = NULL;

      if (old_cb_funcs)
	{
	  UNLOCK_CONTEXT (context);
	  old_cb_funcs->unref (old_cb_data);
	  LOCK_CONTEXT (context);
	}

      if (!SOURCE_BLOCKED (source))
	{
	  tmp_list = source->poll_fds;
	  while (tmp_list)
	    {
	      xmain_context_remove_poll_unlocked (context, tmp_list->data);
	      tmp_list = tmp_list->next;
	    }

          for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next)
            xmain_context_remove_poll_unlocked (context, tmp_list->data);
	}

      while (source->priv->child_sources)
        g_child_source_remove_internal (source->priv->child_sources->data, context);

      if (source->priv->parent_source)
        g_child_source_remove_internal (source, context);

      xsource_unref_internal (source, context, TRUE);
    }

  if (!have_lock)
    UNLOCK_CONTEXT (context);
}

/**
 * xsource_destroy:
 * @source: a #xsource_t
 *
 * Removes a source from its #xmain_context_t, if any, and mark it as
 * destroyed.  The source cannot be subsequently added to another
 * context. It is safe to call this on sources which have already been
 * removed from their context.
 *
 * This does not unref the #xsource_t: if you still hold a reference, use
 * xsource_unref() to drop it.
 *
 * This function is safe to call from any thread, regardless of which thread
 * the #xmain_context_t is running in.
 *
 * If the source is currently attached to a #xmain_context_t, destroying it
 * will effectively unset the callback similar to calling xsource_set_callback().
 * This can mean, that the data's #xdestroy_notify_t gets called right away.
 */
void
xsource_destroy (xsource_t *source)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);

  context = source->context;

  if (context)
    xsource_destroy_internal (source, context, FALSE);
  else
    source->flags &= ~G_HOOK_FLAG_ACTIVE;
}

/**
 * xsource_get_id:
 * @source: a #xsource_t
 *
 * Returns the numeric ID for a particular source. The ID of a source
 * is a positive integer which is unique within a particular main loop
 * context. The reverse
 * mapping from ID to source is done by xmain_context_find_source_by_id().
 *
 * You can only call this function while the source is associated to a
 * #xmain_context_t instance; calling this function before xsource_attach()
 * or after xsource_destroy() yields undefined behavior. The ID returned
 * is unique within the #xmain_context_t instance passed to xsource_attach().
 *
 * Returns: the ID (greater than 0) for the source
 **/
xuint_t
xsource_get_id (xsource_t *source)
{
  xuint_t result;

  g_return_val_if_fail (source != NULL, 0);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, 0);
  g_return_val_if_fail (source->context != NULL, 0);

  LOCK_CONTEXT (source->context);
  result = source->source_id;
  UNLOCK_CONTEXT (source->context);

  return result;
}

/**
 * xsource_get_context:
 * @source: a #xsource_t
 *
 * Gets the #xmain_context_t with which the source is associated.
 *
 * You can call this on a source that has been destroyed, provided
 * that the #xmain_context_t it was attached to still exists (in which
 * case it will return that #xmain_context_t). In particular, you can
 * always call this function on the source returned from
 * g_main_current_source(). But calling this function on a source
 * whose #xmain_context_t has been destroyed is an error.
 *
 * Returns: (transfer none) (nullable): the #xmain_context_t with which the
 *               source is associated, or %NULL if the context has not
 *               yet been added to a source.
 **/
xmain_context_t *
xsource_get_context (xsource_t *source)
{
  g_return_val_if_fail (source != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, NULL);
  g_return_val_if_fail (source->context != NULL || !SOURCE_DESTROYED (source), NULL);

  return source->context;
}

/**
 * xsource_add_poll:
 * @source:a #xsource_t
 * @fd: a #xpollfd_t structure holding information about a file
 *      descriptor to watch.
 *
 * Adds a file descriptor to the set of file descriptors polled for
 * this source. This is usually combined with xsource_new() to add an
 * event source. The event source's check function will typically test
 * the @revents field in the #xpollfd_t struct and return %TRUE if events need
 * to be processed.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * Using this API forces the linear scanning of event sources on each
 * main loop iteration.  Newly-written event sources should try to use
 * xsource_add_unix_fd() instead of this API.
 **/
void
xsource_add_poll (xsource_t *source,
		   xpollfd_t *fd)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (fd != NULL);
  g_return_if_fail (!SOURCE_DESTROYED (source));

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  source->poll_fds = xslist_prepend (source->poll_fds, fd);

  if (context)
    {
      if (!SOURCE_BLOCKED (source))
	xmain_context_add_poll_unlocked (context, source->priority, fd);
      UNLOCK_CONTEXT (context);
    }
}

/**
 * xsource_remove_poll:
 * @source:a #xsource_t
 * @fd: a #xpollfd_t structure previously passed to xsource_add_poll().
 *
 * Removes a file descriptor from the set of file descriptors polled for
 * this source.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 **/
void
xsource_remove_poll (xsource_t *source,
		      xpollfd_t *fd)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (fd != NULL);
  g_return_if_fail (!SOURCE_DESTROYED (source));

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  source->poll_fds = xslist_remove (source->poll_fds, fd);

  if (context)
    {
      if (!SOURCE_BLOCKED (source))
	xmain_context_remove_poll_unlocked (context, fd);
      UNLOCK_CONTEXT (context);
    }
}

/**
 * xsource_add_child_source:
 * @source:a #xsource_t
 * @child_source: a second #xsource_t that @source should "poll"
 *
 * Adds @child_source to @source as a "polled" source; when @source is
 * added to a #xmain_context_t, @child_source will be automatically added
 * with the same priority, when @child_source is triggered, it will
 * cause @source to dispatch (in addition to calling its own
 * callback), and when @source is destroyed, it will destroy
 * @child_source as well. (@source will also still be dispatched if
 * its own prepare/check functions indicate that it is ready.)
 *
 * If you don't need @child_source to do anything on its own when it
 * triggers, you can call xsource_set_dummy_callback() on it to set a
 * callback that does nothing (except return %TRUE if appropriate).
 *
 * @source will hold a reference on @child_source while @child_source
 * is attached to it.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * Since: 2.28
 **/
void
xsource_add_child_source (xsource_t *source,
			   xsource_t *child_source)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (child_source != NULL);
  g_return_if_fail (g_atomic_int_get (&child_source->ref_count) > 0);
  g_return_if_fail (!SOURCE_DESTROYED (source));
  g_return_if_fail (!SOURCE_DESTROYED (child_source));
  g_return_if_fail (child_source->context == NULL);
  g_return_if_fail (child_source->priv->parent_source == NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  TRACE (XPL_SOURCE_ADD_CHILD_SOURCE (source, child_source));

  source->priv->child_sources = xslist_prepend (source->priv->child_sources,
						 xsource_ref (child_source));
  child_source->priv->parent_source = source;
  xsource_set_priority_unlocked (child_source, NULL, source->priority);
  if (SOURCE_BLOCKED (source))
    block_source (child_source);

  if (context)
    {
      xsource_attach_unlocked (child_source, context, TRUE);
      UNLOCK_CONTEXT (context);
    }
}

static void
g_child_source_remove_internal (xsource_t *child_source,
                                xmain_context_t *context)
{
  xsource_t *parent_source = child_source->priv->parent_source;

  parent_source->priv->child_sources =
    xslist_remove (parent_source->priv->child_sources, child_source);
  child_source->priv->parent_source = NULL;

  xsource_destroy_internal (child_source, context, TRUE);
  xsource_unref_internal (child_source, context, TRUE);
}

/**
 * xsource_remove_child_source:
 * @source:a #xsource_t
 * @child_source: a #xsource_t previously passed to
 *     xsource_add_child_source().
 *
 * Detaches @child_source from @source and destroys it.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * Since: 2.28
 **/
void
xsource_remove_child_source (xsource_t *source,
			      xsource_t *child_source)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (child_source != NULL);
  g_return_if_fail (g_atomic_int_get (&child_source->ref_count) > 0);
  g_return_if_fail (child_source->priv->parent_source == source);
  g_return_if_fail (!SOURCE_DESTROYED (source));
  g_return_if_fail (!SOURCE_DESTROYED (child_source));

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  g_child_source_remove_internal (child_source, context);

  if (context)
    UNLOCK_CONTEXT (context);
}

static void
xsource_callback_ref (xpointer_t cb_data)
{
  xsource_callback_t *callback = cb_data;

  g_atomic_int_inc (&callback->ref_count);
}

static void
xsource_callback_unref (xpointer_t cb_data)
{
  xsource_callback_t *callback = cb_data;

  if (g_atomic_int_dec_and_test (&callback->ref_count))
    {
      if (callback->notify)
        callback->notify (callback->data);
      g_free (callback);
    }
}

static void
xsource_callback_get (xpointer_t     cb_data,
		       xsource_t     *source,
		       xsource_func_t *func,
		       xpointer_t    *data)
{
  xsource_callback_t *callback = cb_data;

  *func = callback->func;
  *data = callback->data;
}

static xsource_callback_funcs_t xsource_callback_funcs = {
  xsource_callback_ref,
  xsource_callback_unref,
  xsource_callback_get,
};

/**
 * xsource_set_callback_indirect:
 * @source: the source
 * @callback_data: pointer to callback data "object"
 * @callback_funcs: functions for reference counting @callback_data
 *                  and getting the callback and data
 *
 * Sets the callback function storing the data as a refcounted callback
 * "object". This is used internally. Note that calling
 * xsource_set_callback_indirect() assumes
 * an initial reference count on @callback_data, and thus
 * @callback_funcs->unref will eventually be called once more
 * than @callback_funcs->ref.
 *
 * It is safe to call this function multiple times on a source which has already
 * been attached to a context. The changes will take effect for the next time
 * the source is dispatched after this call returns.
 **/
void
xsource_set_callback_indirect (xsource_t              *source,
				xpointer_t              callback_data,
				xsource_callback_funcs_t *callback_funcs)
{
  xmain_context_t *context;
  xpointer_t old_cb_data;
  xsource_callback_funcs_t *old_cb_funcs;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (callback_funcs != NULL || callback_data == NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  if (callback_funcs != &xsource_callback_funcs)
    {
      TRACE (XPL_SOURCE_SET_CALLBACK_INDIRECT (source, callback_data,
                                                callback_funcs->ref,
                                                callback_funcs->unref,
                                                callback_funcs->get));
    }

  old_cb_data = source->callback_data;
  old_cb_funcs = source->callback_funcs;

  source->callback_data = callback_data;
  source->callback_funcs = callback_funcs;

  if (context)
    UNLOCK_CONTEXT (context);

  if (old_cb_funcs)
    old_cb_funcs->unref (old_cb_data);
}

/**
 * xsource_set_callback:
 * @source: the source
 * @func: a callback function
 * @data: the data to pass to callback function
 * @notify: (nullable): a function to call when @data is no longer in use, or %NULL.
 *
 * Sets the callback function for a source. The callback for a source is
 * called from the source's dispatch function.
 *
 * The exact type of @func depends on the type of source; ie. you
 * should not count on @func being called with @data as its first
 * parameter. Cast @func with G_SOURCE_FUNC() to avoid warnings about
 * incompatible function types.
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle memory management of @data.
 *
 * Typically, you won't use this function. Instead use functions specific
 * to the type of source you are using, such as g_idle_add() or g_timeout_add().
 *
 * It is safe to call this function multiple times on a source which has already
 * been attached to a context. The changes will take effect for the next time
 * the source is dispatched after this call returns.
 *
 * Note that xsource_destroy() for a currently attached source has the effect
 * of also unsetting the callback.
 **/
void
xsource_set_callback (xsource_t        *source,
		       xsource_func_t     func,
		       xpointer_t        data,
		       xdestroy_notify_t  notify)
{
  xsource_callback_t *new_callback;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);

  TRACE (XPL_SOURCE_SET_CALLBACK (source, func, data, notify));

  new_callback = g_new (xsource_callback_t, 1);

  new_callback->ref_count = 1;
  new_callback->func = func;
  new_callback->data = data;
  new_callback->notify = notify;

  xsource_set_callback_indirect (source, new_callback, &xsource_callback_funcs);
}


/**
 * xsource_set_funcs:
 * @source: a #xsource_t
 * @funcs: the new #xsource_funcs_t
 *
 * Sets the source functions (can be used to override
 * default implementations) of an unattached source.
 *
 * Since: 2.12
 */
void
xsource_set_funcs (xsource_t     *source,
	           xsource_funcs_t *funcs)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (source->context == NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (funcs != NULL);

  source->source_funcs = funcs;
}

static void
xsource_set_priority_unlocked (xsource_t      *source,
				xmain_context_t *context,
				xint_t          priority)
{
  xslist_t *tmp_list;

  g_return_if_fail (source->priv->parent_source == NULL ||
		    source->priv->parent_source->priority == priority);

  TRACE (XPL_SOURCE_SET_PRIORITY (source, context, priority));

  if (context)
    {
      /* Remove the source from the context's source and then
       * add it back after so it is sorted in the correct place
       */
      source_remove_from_context (source, source->context);
    }

  source->priority = priority;

  if (context)
    {
      source_add_to_context (source, source->context);

      if (!SOURCE_BLOCKED (source))
	{
	  tmp_list = source->poll_fds;
	  while (tmp_list)
	    {
	      xmain_context_remove_poll_unlocked (context, tmp_list->data);
	      xmain_context_add_poll_unlocked (context, priority, tmp_list->data);

	      tmp_list = tmp_list->next;
	    }

          for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next)
            {
              xmain_context_remove_poll_unlocked (context, tmp_list->data);
              xmain_context_add_poll_unlocked (context, priority, tmp_list->data);
            }
	}
    }

  if (source->priv->child_sources)
    {
      tmp_list = source->priv->child_sources;
      while (tmp_list)
	{
	  xsource_set_priority_unlocked (tmp_list->data, context, priority);
	  tmp_list = tmp_list->next;
	}
    }
}

/**
 * xsource_set_priority:
 * @source: a #xsource_t
 * @priority: the new priority.
 *
 * Sets the priority of a source. While the main loop is being run, a
 * source will be dispatched if it is ready to be dispatched and no
 * sources at a higher (numerically smaller) priority are ready to be
 * dispatched.
 *
 * A child source always has the same priority as its parent.  It is not
 * permitted to change the priority of a source once it has been added
 * as a child of another source.
 **/
void
xsource_set_priority (xsource_t  *source,
		       xint_t      priority)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (source->priv->parent_source == NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);
  xsource_set_priority_unlocked (source, context, priority);
  if (context)
    UNLOCK_CONTEXT (context);
}

/**
 * xsource_get_priority:
 * @source: a #xsource_t
 *
 * Gets the priority of a source.
 *
 * Returns: the priority of the source
 **/
xint_t
xsource_get_priority (xsource_t *source)
{
  g_return_val_if_fail (source != NULL, 0);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, 0);

  return source->priority;
}

/**
 * xsource_set_ready_time:
 * @source: a #xsource_t
 * @ready_time: the monotonic time at which the source will be ready,
 *              0 for "immediately", -1 for "never"
 *
 * Sets a #xsource_t to be dispatched when the given monotonic time is
 * reached (or passed).  If the monotonic time is in the past (as it
 * always will be if @ready_time is 0) then the source will be
 * dispatched immediately.
 *
 * If @ready_time is -1 then the source is never woken up on the basis
 * of the passage of time.
 *
 * Dispatching the source does not reset the ready time.  You should do
 * so yourself, from the source dispatch function.
 *
 * Note that if you have a pair of sources where the ready time of one
 * suggests that it will be delivered first but the priority for the
 * other suggests that it would be delivered first, and the ready time
 * for both sources is reached during the same main context iteration,
 * then the order of dispatch is undefined.
 *
 * It is a no-op to call this function on a #xsource_t which has already been
 * destroyed with xsource_destroy().
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * Since: 2.36
 **/
void
xsource_set_ready_time (xsource_t *source,
                         gint64   ready_time)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  if (source->priv->ready_time == ready_time)
    {
      if (context)
        UNLOCK_CONTEXT (context);

      return;
    }

  source->priv->ready_time = ready_time;

  TRACE (XPL_SOURCE_SET_READY_TIME (source, ready_time));

  if (context)
    {
      /* Quite likely that we need to change the timeout on the poll */
      if (!SOURCE_BLOCKED (source))
        g_wakeup_signal (context->wakeup);
      UNLOCK_CONTEXT (context);
    }
}

/**
 * xsource_get_ready_time:
 * @source: a #xsource_t
 *
 * Gets the "ready time" of @source, as set by
 * xsource_set_ready_time().
 *
 * Any time before the current monotonic time (including 0) is an
 * indication that the source will fire immediately.
 *
 * Returns: the monotonic ready time, -1 for "never"
 **/
gint64
xsource_get_ready_time (xsource_t *source)
{
  g_return_val_if_fail (source != NULL, -1);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, -1);

  return source->priv->ready_time;
}

/**
 * xsource_set_can_recurse:
 * @source: a #xsource_t
 * @can_recurse: whether recursion is allowed for this source
 *
 * Sets whether a source can be called recursively. If @can_recurse is
 * %TRUE, then while the source is being dispatched then this source
 * will be processed normally. Otherwise, all processing of this
 * source is blocked until the dispatch function returns.
 **/
void
xsource_set_can_recurse (xsource_t  *source,
			  xboolean_t  can_recurse)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  if (can_recurse)
    source->flags |= G_SOURCE_CAN_RECURSE;
  else
    source->flags &= ~G_SOURCE_CAN_RECURSE;

  if (context)
    UNLOCK_CONTEXT (context);
}

/**
 * xsource_get_can_recurse:
 * @source: a #xsource_t
 *
 * Checks whether a source is allowed to be called recursively.
 * see xsource_set_can_recurse().
 *
 * Returns: whether recursion is allowed.
 **/
xboolean_t
xsource_get_can_recurse (xsource_t  *source)
{
  g_return_val_if_fail (source != NULL, FALSE);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, FALSE);

  return (source->flags & G_SOURCE_CAN_RECURSE) != 0;
}

static void
xsource_set_name_full (xsource_t    *source,
                        const char *name,
                        xboolean_t    is_static)
{
  xmain_context_t *context;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  TRACE (XPL_SOURCE_SET_NAME (source, name));

  /* setting back to NULL is allowed, just because it's
   * weird if get_name can return NULL but you can't
   * set that.
   */

  if (!source->priv->static_name)
    g_free (source->name);

  if (is_static)
    source->name = (char *)name;
  else
    source->name = xstrdup (name);

  source->priv->static_name = is_static;

  if (context)
    UNLOCK_CONTEXT (context);
}

/**
 * xsource_set_name:
 * @source: a #xsource_t
 * @name: debug name for the source
 *
 * Sets a name for the source, used in debugging and profiling.
 * The name defaults to #NULL.
 *
 * The source name should describe in a human-readable way
 * what the source does. For example, "X11 event queue"
 * or "GTK+ repaint idle handler" or whatever it is.
 *
 * It is permitted to call this function multiple times, but is not
 * recommended due to the potential performance impact.  For example,
 * one could change the name in the "check" function of a #xsource_funcs_t
 * to include details like the event type in the source name.
 *
 * Use caution if changing the name while another thread may be
 * accessing it with xsource_get_name(); that function does not copy
 * the value, and changing the value will free it while the other thread
 * may be attempting to use it.
 *
 * Also see xsource_set_static_name().
 *
 * Since: 2.26
 **/
void
xsource_set_name (xsource_t    *source,
                   const char *name)
{
  xsource_set_name_full (source, name, FALSE);
}

/**
 * xsource_set_static_name:
 * @source: a #xsource_t
 * @name: debug name for the source
 *
 * A variant of xsource_set_name() that does not
 * duplicate the @name, and can only be used with
 * string literals.
 *
 * Since: 2.70
 */
void
xsource_set_static_name (xsource_t    *source,
                          const char *name)
{
  xsource_set_name_full (source, name, TRUE);
}

/**
 * xsource_get_name:
 * @source: a #xsource_t
 *
 * Gets a name for the source, used in debugging and profiling.  The
 * name may be #NULL if it has never been set with xsource_set_name().
 *
 * Returns: (nullable): the name of the source
 *
 * Since: 2.26
 **/
const char *
xsource_get_name (xsource_t *source)
{
  g_return_val_if_fail (source != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, NULL);

  return source->name;
}

/**
 * xsource_set_name_by_id:
 * @tag: a #xsource_t ID
 * @name: debug name for the source
 *
 * Sets the name of a source using its ID.
 *
 * This is a convenience utility to set source names from the return
 * value of g_idle_add(), g_timeout_add(), etc.
 *
 * It is a programmer error to attempt to set the name of a non-existent
 * source.
 *
 * More specifically: source IDs can be reissued after a source has been
 * destroyed and therefore it is never valid to use this function with a
 * source ID which may have already been removed.  An example is when
 * scheduling an idle to run in another thread with g_idle_add(): the
 * idle may already have run and been removed by the time this function
 * is called on its (now invalid) source ID.  This source ID may have
 * been reissued, leading to the operation being performed against the
 * wrong source.
 *
 * Since: 2.26
 **/
void
xsource_set_name_by_id (xuint_t           tag,
                         const char     *name)
{
  xsource_t *source;

  g_return_if_fail (tag > 0);

  source = xmain_context_find_source_by_id (NULL, tag);
  if (source == NULL)
    return;

  xsource_set_name (source, name);
}


/**
 * xsource_ref:
 * @source: a #xsource_t
 *
 * Increases the reference count on a source by one.
 *
 * Returns: @source
 **/
xsource_t *
xsource_ref (xsource_t *source)
{
  g_return_val_if_fail (source != NULL, NULL);
  /* We allow ref_count == 0 here to allow the dispose function to resurrect
   * the xsource_t if needed */
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) >= 0, NULL);

  g_atomic_int_inc (&source->ref_count);

  return source;
}

/* xsource_unref() but possible to call within context lock
 */
static void
xsource_unref_internal (xsource_t      *source,
			 xmain_context_t *context,
			 xboolean_t      have_lock)
{
  xpointer_t old_cb_data = NULL;
  xsource_callback_funcs_t *old_cb_funcs = NULL;

  g_return_if_fail (source != NULL);

  if (!have_lock && context)
    LOCK_CONTEXT (context);

  if (g_atomic_int_dec_and_test (&source->ref_count))
    {
      /* If there's a dispose function, call this first */
      if (source->priv->dispose)
        {
          /* Temporarily increase the ref count again so that xsource_t methods
           * can be called from dispose(). */
          g_atomic_int_inc (&source->ref_count);
          if (context)
            UNLOCK_CONTEXT (context);
          source->priv->dispose (source);
          if (context)
            LOCK_CONTEXT (context);

          /* Now the reference count might be bigger than 0 again, in which
           * case we simply return from here before freeing the source */
          if (!g_atomic_int_dec_and_test (&source->ref_count))
            {
              if (!have_lock && context)
                UNLOCK_CONTEXT (context);
              return;
            }
        }

      TRACE (XPL_SOURCE_BEFORE_FREE (source, context,
                                      source->source_funcs->finalize));

      old_cb_data = source->callback_data;
      old_cb_funcs = source->callback_funcs;

      source->callback_data = NULL;
      source->callback_funcs = NULL;

      if (context)
	{
	  if (!SOURCE_DESTROYED (source))
	    g_warning (G_STRLOC ": ref_count == 0, but source was still attached to a context!");
	  source_remove_from_context (source, context);

          xhash_table_remove (context->sources, GUINT_TO_POINTER (source->source_id));
	}

      if (source->source_funcs->finalize)
	{
          xint_t old_ref_count;

          /* Temporarily increase the ref count again so that xsource_t methods
           * can be called from finalize(). */
          g_atomic_int_inc (&source->ref_count);
	  if (context)
	    UNLOCK_CONTEXT (context);
	  source->source_funcs->finalize (source);
	  if (context)
	    LOCK_CONTEXT (context);
          old_ref_count = g_atomic_int_add (&source->ref_count, -1);
          g_warn_if_fail (old_ref_count == 1);
	}

      if (old_cb_funcs)
        {
          xint_t old_ref_count;

          /* Temporarily increase the ref count again so that xsource_t methods
           * can be called from callback_funcs.unref(). */
          g_atomic_int_inc (&source->ref_count);
          if (context)
            UNLOCK_CONTEXT (context);

          old_cb_funcs->unref (old_cb_data);

          if (context)
            LOCK_CONTEXT (context);
          old_ref_count = g_atomic_int_add (&source->ref_count, -1);
          g_warn_if_fail (old_ref_count == 1);
        }

      if (!source->priv->static_name)
        g_free (source->name);
      source->name = NULL;

      xslist_free (source->poll_fds);
      source->poll_fds = NULL;

      xslist_free_full (source->priv->fds, g_free);

      while (source->priv->child_sources)
        {
          xsource_t *child_source = source->priv->child_sources->data;

          source->priv->child_sources =
            xslist_remove (source->priv->child_sources, child_source);
          child_source->priv->parent_source = NULL;

          xsource_unref_internal (child_source, context, TRUE);
        }

      g_slice_free (xsource_private_t, source->priv);
      source->priv = NULL;

      g_free (source);
    }

  if (!have_lock && context)
    UNLOCK_CONTEXT (context);
}

/**
 * xsource_unref:
 * @source: a #xsource_t
 *
 * Decreases the reference count of a source by one. If the
 * resulting reference count is zero the source and associated
 * memory will be destroyed.
 **/
void
xsource_unref (xsource_t *source)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);

  xsource_unref_internal (source, source->context, FALSE);
}

/**
 * xmain_context_find_source_by_id:
 * @context: (nullable): a #xmain_context_t (if %NULL, the default context will be used)
 * @source_id: the source ID, as returned by xsource_get_id().
 *
 * Finds a #xsource_t given a pair of context and ID.
 *
 * It is a programmer error to attempt to look up a non-existent source.
 *
 * More specifically: source IDs can be reissued after a source has been
 * destroyed and therefore it is never valid to use this function with a
 * source ID which may have already been removed.  An example is when
 * scheduling an idle to run in another thread with g_idle_add(): the
 * idle may already have run and been removed by the time this function
 * is called on its (now invalid) source ID.  This source ID may have
 * been reissued, leading to the operation being performed against the
 * wrong source.
 *
 * Returns: (transfer none): the #xsource_t
 **/
xsource_t *
xmain_context_find_source_by_id (xmain_context_t *context,
                                  xuint_t         source_id)
{
  xsource_t *source;

  g_return_val_if_fail (source_id > 0, NULL);

  if (context == NULL)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);
  source = xhash_table_lookup (context->sources, GUINT_TO_POINTER (source_id));
  UNLOCK_CONTEXT (context);

  if (source && SOURCE_DESTROYED (source))
    source = NULL;

  return source;
}

/**
 * xmain_context_find_source_by_funcs_user_data:
 * @context: (nullable): a #xmain_context_t (if %NULL, the default context will be used).
 * @funcs: the @source_funcs passed to xsource_new().
 * @user_data: the user data from the callback.
 *
 * Finds a source with the given source functions and user data.  If
 * multiple sources exist with the same source function and user data,
 * the first one found will be returned.
 *
 * Returns: (transfer none): the source, if one was found, otherwise %NULL
 **/
xsource_t *
xmain_context_find_source_by_funcs_user_data (xmain_context_t *context,
					       xsource_funcs_t *funcs,
					       xpointer_t      user_data)
{
  GSourceIter iter;
  xsource_t *source;

  g_return_val_if_fail (funcs != NULL, NULL);

  if (context == NULL)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);

  xsource_iter_init (&iter, context, FALSE);
  while (xsource_iter_next (&iter, &source))
    {
      if (!SOURCE_DESTROYED (source) &&
	  source->source_funcs == funcs &&
	  source->callback_funcs)
	{
	  xsource_func_t callback;
	  xpointer_t callback_data;

	  source->callback_funcs->get (source->callback_data, source, &callback, &callback_data);

	  if (callback_data == user_data)
	    break;
	}
    }
  xsource_iter_clear (&iter);

  UNLOCK_CONTEXT (context);

  return source;
}

/**
 * xmain_context_find_source_by_user_data:
 * @context: a #xmain_context_t
 * @user_data: the user_data for the callback.
 *
 * Finds a source with the given user data for the callback.  If
 * multiple sources exist with the same user data, the first
 * one found will be returned.
 *
 * Returns: (transfer none): the source, if one was found, otherwise %NULL
 **/
xsource_t *
xmain_context_find_source_by_user_data (xmain_context_t *context,
					 xpointer_t      user_data)
{
  GSourceIter iter;
  xsource_t *source;

  if (context == NULL)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);

  xsource_iter_init (&iter, context, FALSE);
  while (xsource_iter_next (&iter, &source))
    {
      if (!SOURCE_DESTROYED (source) &&
	  source->callback_funcs)
	{
	  xsource_func_t callback;
	  xpointer_t callback_data = NULL;

	  source->callback_funcs->get (source->callback_data, source, &callback, &callback_data);

	  if (callback_data == user_data)
	    break;
	}
    }
  xsource_iter_clear (&iter);

  UNLOCK_CONTEXT (context);

  return source;
}

/**
 * xsource_remove:
 * @tag: the ID of the source to remove.
 *
 * Removes the source with the given ID from the default main context. You must
 * use xsource_destroy() for sources added to a non-default main context.
 *
 * The ID of a #xsource_t is given by xsource_get_id(), or will be
 * returned by the functions xsource_attach(), g_idle_add(),
 * g_idle_add_full(), g_timeout_add(), g_timeout_add_full(),
 * g_child_watch_add(), g_child_watch_add_full(), g_io_add_watch(), and
 * g_io_add_watch_full().
 *
 * It is a programmer error to attempt to remove a non-existent source.
 *
 * More specifically: source IDs can be reissued after a source has been
 * destroyed and therefore it is never valid to use this function with a
 * source ID which may have already been removed.  An example is when
 * scheduling an idle to run in another thread with g_idle_add(): the
 * idle may already have run and been removed by the time this function
 * is called on its (now invalid) source ID.  This source ID may have
 * been reissued, leading to the operation being performed against the
 * wrong source.
 *
 * Returns: %TRUE if the source was found and removed.
 **/
xboolean_t
xsource_remove (xuint_t tag)
{
  xsource_t *source;

  g_return_val_if_fail (tag > 0, FALSE);

  source = xmain_context_find_source_by_id (NULL, tag);
  if (source)
    xsource_destroy (source);
  else
    g_critical ("Source ID %u was not found when attempting to remove it", tag);

  return source != NULL;
}

/**
 * xsource_remove_by_user_data:
 * @user_data: the user_data for the callback.
 *
 * Removes a source from the default main loop context given the user
 * data for the callback. If multiple sources exist with the same user
 * data, only one will be destroyed.
 *
 * Returns: %TRUE if a source was found and removed.
 **/
xboolean_t
xsource_remove_by_user_data (xpointer_t user_data)
{
  xsource_t *source;

  source = xmain_context_find_source_by_user_data (NULL, user_data);
  if (source)
    {
      xsource_destroy (source);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * xsource_remove_by_funcs_user_data:
 * @funcs: The @source_funcs passed to xsource_new()
 * @user_data: the user data for the callback
 *
 * Removes a source from the default main loop context given the
 * source functions and user data. If multiple sources exist with the
 * same source functions and user data, only one will be destroyed.
 *
 * Returns: %TRUE if a source was found and removed.
 **/
xboolean_t
xsource_remove_by_funcs_user_data (xsource_funcs_t *funcs,
				    xpointer_t      user_data)
{
  xsource_t *source;

  g_return_val_if_fail (funcs != NULL, FALSE);

  source = xmain_context_find_source_by_funcs_user_data (NULL, funcs, user_data);
  if (source)
    {
      xsource_destroy (source);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * g_clear_handle_id: (skip)
 * @tag_ptr: (not nullable): a pointer to the handler ID
 * @clear_func: (not nullable): the function to call to clear the handler
 *
 * Clears a numeric handler, such as a #xsource_t ID.
 *
 * @tag_ptr must be a valid pointer to the variable holding the handler.
 *
 * If the ID is zero then this function does nothing.
 * Otherwise, clear_func() is called with the ID as a parameter, and the tag is
 * set to zero.
 *
 * A macro is also included that allows this function to be used without
 * pointer casts.
 *
 * Since: 2.56
 */
#undef g_clear_handle_id
void
g_clear_handle_id (xuint_t            *tag_ptr,
                   GClearHandleFunc  clear_func)
{
  xuint_t _handle_id;

  _handle_id = *tag_ptr;
  if (_handle_id > 0)
    {
      *tag_ptr = 0;
      clear_func (_handle_id);
    }
}

#ifdef G_OS_UNIX
/**
 * xsource_add_unix_fd:
 * @source: a #xsource_t
 * @fd: the fd to monitor
 * @events: an event mask
 *
 * Monitors @fd for the IO events in @events.
 *
 * The tag returned by this function can be used to remove or modify the
 * monitoring of the fd using xsource_remove_unix_fd() or
 * xsource_modify_unix_fd().
 *
 * It is not necessary to remove the fd before destroying the source; it
 * will be cleaned up automatically.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * As the name suggests, this function is not available on Windows.
 *
 * Returns: (not nullable): an opaque tag
 *
 * Since: 2.36
 **/
xpointer_t
xsource_add_unix_fd (xsource_t      *source,
                      xint_t          fd,
                      xio_condition_t  events)
{
  xmain_context_t *context;
  xpollfd_t *poll_fd;

  g_return_val_if_fail (source != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, NULL);
  g_return_val_if_fail (!SOURCE_DESTROYED (source), NULL);

  poll_fd = g_new (xpollfd_t, 1);
  poll_fd->fd = fd;
  poll_fd->events = events;
  poll_fd->revents = 0;

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  source->priv->fds = xslist_prepend (source->priv->fds, poll_fd);

  if (context)
    {
      if (!SOURCE_BLOCKED (source))
        xmain_context_add_poll_unlocked (context, source->priority, poll_fd);
      UNLOCK_CONTEXT (context);
    }

  return poll_fd;
}

/**
 * xsource_modify_unix_fd:
 * @source: a #xsource_t
 * @tag: (not nullable): the tag from xsource_add_unix_fd()
 * @new_events: the new event mask to watch
 *
 * Updates the event mask to watch for the fd identified by @tag.
 *
 * @tag is the tag returned from xsource_add_unix_fd().
 *
 * If you want to remove a fd, don't set its event mask to zero.
 * Instead, call xsource_remove_unix_fd().
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * As the name suggests, this function is not available on Windows.
 *
 * Since: 2.36
 **/
void
xsource_modify_unix_fd (xsource_t      *source,
                         xpointer_t      tag,
                         xio_condition_t  new_events)
{
  xmain_context_t *context;
  xpollfd_t *poll_fd;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (xslist_find (source->priv->fds, tag));

  context = source->context;
  poll_fd = tag;

  poll_fd->events = new_events;

  if (context)
    xmain_context_wakeup (context);
}

/**
 * xsource_remove_unix_fd:
 * @source: a #xsource_t
 * @tag: (not nullable): the tag from xsource_add_unix_fd()
 *
 * Reverses the effect of a previous call to xsource_add_unix_fd().
 *
 * You only need to call this if you want to remove an fd from being
 * watched while keeping the same source around.  In the normal case you
 * will just want to destroy the source.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * As the name suggests, this function is not available on Windows.
 *
 * Since: 2.36
 **/
void
xsource_remove_unix_fd (xsource_t  *source,
                         xpointer_t  tag)
{
  xmain_context_t *context;
  xpollfd_t *poll_fd;

  g_return_if_fail (source != NULL);
  g_return_if_fail (g_atomic_int_get (&source->ref_count) > 0);
  g_return_if_fail (xslist_find (source->priv->fds, tag));

  context = source->context;
  poll_fd = tag;

  if (context)
    LOCK_CONTEXT (context);

  source->priv->fds = xslist_remove (source->priv->fds, poll_fd);

  if (context)
    {
      if (!SOURCE_BLOCKED (source))
        xmain_context_remove_poll_unlocked (context, poll_fd);

      UNLOCK_CONTEXT (context);
    }

  g_free (poll_fd);
}

/**
 * xsource_query_unix_fd:
 * @source: a #xsource_t
 * @tag: (not nullable): the tag from xsource_add_unix_fd()
 *
 * Queries the events reported for the fd corresponding to @tag on
 * @source during the last poll.
 *
 * The return value of this function is only defined when the function
 * is called from the check or dispatch functions for @source.
 *
 * This API is only intended to be used by implementations of #xsource_t.
 * Do not call this API on a #xsource_t that you did not create.
 *
 * As the name suggests, this function is not available on Windows.
 *
 * Returns: the conditions reported on the fd
 *
 * Since: 2.36
 **/
xio_condition_t
xsource_query_unix_fd (xsource_t  *source,
                        xpointer_t  tag)
{
  xpollfd_t *poll_fd;

  g_return_val_if_fail (source != NULL, 0);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, 0);
  g_return_val_if_fail (xslist_find (source->priv->fds, tag), 0);

  poll_fd = tag;

  return poll_fd->revents;
}
#endif /* G_OS_UNIX */

/**
 * g_get_current_time:
 * @result: #GTimeVal structure in which to store current time.
 *
 * Equivalent to the UNIX gettimeofday() function, but portable.
 *
 * You may find g_get_real_time() to be more convenient.
 *
 * Deprecated: 2.62: #GTimeVal is not year-2038-safe. Use g_get_real_time()
 *    instead.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
g_get_current_time (GTimeVal *result)
{
  gint64 tv;

  g_return_if_fail (result != NULL);

  tv = g_get_real_time ();

  result->tv_sec = tv / 1000000;
  result->tv_usec = tv % 1000000;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * g_get_real_time:
 *
 * Queries the system wall-clock time.
 *
 * This call is functionally equivalent to g_get_current_time() except
 * that the return value is often more convenient than dealing with a
 * #GTimeVal.
 *
 * You should only use this call if you are actually interested in the real
 * wall-clock time.  g_get_monotonic_time() is probably more useful for
 * measuring intervals.
 *
 * Returns: the number of microseconds since January 1, 1970 UTC.
 *
 * Since: 2.28
 **/
gint64
g_get_real_time (void)
{
#ifndef G_OS_WIN32
  struct timeval r;

  /* this is required on alpha, there the timeval structs are ints
   * not longs and a cast only would fail horribly */
  gettimeofday (&r, NULL);

  return (((gint64) r.tv_sec) * 1000000) + r.tv_usec;
#else
  FILETIME ft;
  xuint64_t time64;

  GetSystemTimeAsFileTime (&ft);
  memmove (&time64, &ft, sizeof (FILETIME));

  /* Convert from 100s of nanoseconds since 1601-01-01
   * to Unix epoch. This is Y2038 safe.
   */
  time64 -= G_GINT64_CONSTANT (116444736000000000);
  time64 /= 10;

  return time64;
#endif
}

/**
 * g_get_monotonic_time:
 *
 * Queries the system monotonic time.
 *
 * The monotonic clock will always increase and doesn't suffer
 * discontinuities when the user (or NTP) changes the system time.  It
 * may or may not continue to tick during times where the machine is
 * suspended.
 *
 * We try to use the clock that corresponds as closely as possible to
 * the passage of time as measured by system calls such as poll() but it
 * may not always be possible to do this.
 *
 * Returns: the monotonic time, in microseconds
 *
 * Since: 2.28
 **/
#if defined (G_OS_WIN32)
/* NOTE:
 * time_usec = ticks_since_boot * usec_per_sec / ticks_per_sec
 *
 * Doing (ticks_since_boot * usec_per_sec) before the division can overflow 64 bits
 * (ticks_since_boot  / ticks_per_sec) and then multiply would not be accurate enough.
 * So for now we calculate (usec_per_sec / ticks_per_sec) and use floating point
 */
static xdouble_t g_monotonic_usec_per_tick = 0;

void
g_clock_win32_init (void)
{
  LARGE_INTEGER freq;

  if (!QueryPerformanceFrequency (&freq) || freq.QuadPart == 0)
    {
      /* The documentation says that this should never happen */
      g_assert_not_reached ();
      return;
    }

  g_monotonic_usec_per_tick = (xdouble_t)G_USEC_PER_SEC / freq.QuadPart;
}

gint64
g_get_monotonic_time (void)
{
  if (G_LIKELY (g_monotonic_usec_per_tick != 0))
    {
      LARGE_INTEGER ticks;

      if (QueryPerformanceCounter (&ticks))
        return (gint64)(ticks.QuadPart * g_monotonic_usec_per_tick);

      g_warning ("QueryPerformanceCounter Failed (%lu)", GetLastError ());
      g_monotonic_usec_per_tick = 0;
    }

  return 0;
}
#elif defined(HAVE_MACH_MACH_TIME_H) /* Mac OS */
gint64
g_get_monotonic_time (void)
{
  mach_timebase_info_data_t timebase_info;
  xuint64_t val;

  /* we get nanoseconds from mach_absolute_time() using timebase_info */
  mach_timebase_info (&timebase_info);
  val = mach_absolute_time ();

  if (timebase_info.numer != timebase_info.denom)
    {
#ifdef HAVE_UINT128_T
      val = ((__uint128_t) val * (__uint128_t) timebase_info.numer) / timebase_info.denom / 1000;
#else
      xuint64_t t_high, t_low;
      xuint64_t result_high, result_low;

      /* 64 bit x 32 bit / 32 bit with 96-bit intermediate
       * algorithm lifted from qemu */
      t_low = (val & 0xffffffffLL) * (xuint64_t) timebase_info.numer;
      t_high = (val >> 32) * (xuint64_t) timebase_info.numer;
      t_high += (t_low >> 32);
      result_high = t_high / (xuint64_t) timebase_info.denom;
      result_low = (((t_high % (xuint64_t) timebase_info.denom) << 32) +
                    (t_low & 0xffffffff)) /
                   (xuint64_t) timebase_info.denom;
      val = ((result_high << 32) | result_low) / 1000;
#endif
    }
  else
    {
      /* nanoseconds to microseconds */
      val = val / 1000;
    }

  return val;
}
#else
gint64
g_get_monotonic_time (void)
{
  struct timespec ts;
  xint_t result;

  result = clock_gettime (CLOCK_MONOTONIC, &ts);

  if G_UNLIKELY (result != 0)
    xerror ("GLib requires working CLOCK_MONOTONIC");

  return (((gint64) ts.tv_sec) * 1000000) + (ts.tv_nsec / 1000);
}
#endif

static void
g_main_dispatch_free (xpointer_t dispatch)
{
  g_free (dispatch);
}

/* Running the main loop */

static GMainDispatch *
get_dispatch (void)
{
  static GPrivate depth_private = G_PRIVATE_INIT (g_main_dispatch_free);
  GMainDispatch *dispatch;

  dispatch = g_private_get (&depth_private);

  if (!dispatch)
    dispatch = g_private_set_alloc0 (&depth_private, sizeof (GMainDispatch));

  return dispatch;
}

/**
 * g_main_depth:
 *
 * Returns the depth of the stack of calls to
 * xmain_context_dispatch() on any #xmain_context_t in the current thread.
 *  That is, when called from the toplevel, it gives 0. When
 * called from within a callback from xmain_context_iteration()
 * (or xmain_loop_run(), etc.) it returns 1. When called from within
 * a callback to a recursive call to xmain_context_iteration(),
 * it returns 2. And so forth.
 *
 * This function is useful in a situation like the following:
 * Imagine an extremely simple "garbage collected" system.
 *
 * |[<!-- language="C" -->
 * static xlist_t *free_list;
 *
 * xpointer_t
 * allocate_memory (xsize_t size)
 * {
 *   xpointer_t result = g_malloc (size);
 *   free_list = xlist_prepend (free_list, result);
 *   return result;
 * }
 *
 * void
 * free_allocated_memory (void)
 * {
 *   xlist_t *l;
 *   for (l = free_list; l; l = l->next);
 *     g_free (l->data);
 *   xlist_free (free_list);
 *   free_list = NULL;
 *  }
 *
 * [...]
 *
 * while (TRUE);
 *  {
 *    xmain_context_iteration (NULL, TRUE);
 *    free_allocated_memory();
 *   }
 * ]|
 *
 * This works from an application, however, if you want to do the same
 * thing from a library, it gets more difficult, since you no longer
 * control the main loop. You might think you can simply use an idle
 * function to make the call to free_allocated_memory(), but that
 * doesn't work, since the idle function could be called from a
 * recursive callback. This can be fixed by using g_main_depth()
 *
 * |[<!-- language="C" -->
 * xpointer_t
 * allocate_memory (xsize_t size)
 * {
 *   FreeListBlock *block = g_new (FreeListBlock, 1);
 *   block->mem = g_malloc (size);
 *   block->depth = g_main_depth ();
 *   free_list = xlist_prepend (free_list, block);
 *   return block->mem;
 * }
 *
 * void
 * free_allocated_memory (void)
 * {
 *   xlist_t *l;
 *
 *   int depth = g_main_depth ();
 *   for (l = free_list; l; );
 *     {
 *       xlist_t *next = l->next;
 *       FreeListBlock *block = l->data;
 *       if (block->depth > depth)
 *         {
 *           g_free (block->mem);
 *           g_free (block);
 *           free_list = xlist_delete_link (free_list, l);
 *         }
 *
 *       l = next;
 *     }
 *   }
 * ]|
 *
 * There is a temptation to use g_main_depth() to solve
 * problems with reentrancy. For instance, while waiting for data
 * to be received from the network in response to a menu item,
 * the menu item might be selected again. It might seem that
 * one could make the menu item's callback return immediately
 * and do nothing if g_main_depth() returns a value greater than 1.
 * However, this should be avoided since the user then sees selecting
 * the menu item do nothing. Furthermore, you'll find yourself adding
 * these checks all over your code, since there are doubtless many,
 * many things that the user could do. Instead, you can use the
 * following techniques:
 *
 * 1. Use gtk_widget_set_sensitive() or modal dialogs to prevent
 *    the user from interacting with elements while the main
 *    loop is recursing.
 *
 * 2. Avoid main loop recursion in situations where you can't handle
 *    arbitrary  callbacks. Instead, structure your code so that you
 *    simply return to the main loop and then get called again when
 *    there is more work to do.
 *
 * Returns: The main loop recursion level in the current thread
 */
int
g_main_depth (void)
{
  GMainDispatch *dispatch = get_dispatch ();
  return dispatch->depth;
}

/**
 * g_main_current_source:
 *
 * Returns the currently firing source for this thread.
 *
 * Returns: (transfer none) (nullable): The currently firing source or %NULL.
 *
 * Since: 2.12
 */
xsource_t *
g_main_current_source (void)
{
  GMainDispatch *dispatch = get_dispatch ();
  return dispatch->source;
}

/**
 * xsource_is_destroyed:
 * @source: a #xsource_t
 *
 * Returns whether @source has been destroyed.
 *
 * This is important when you operate upon your objects
 * from within idle handlers, but may have freed the object
 * before the dispatch of your idle handler.
 *
 * |[<!-- language="C" -->
 * static xboolean_t
 * idle_callback (xpointer_t data)
 * {
 *   SomeWidget *self = data;
 *
 *   g_mutex_lock (&self->idle_id_mutex);
 *   // do stuff with self
 *   self->idle_id = 0;
 *   g_mutex_unlock (&self->idle_id_mutex);
 *
 *   return G_SOURCE_REMOVE;
 * }
 *
 * static void
 * some_widget_do_stuff_later (SomeWidget *self)
 * {
 *   g_mutex_lock (&self->idle_id_mutex);
 *   self->idle_id = g_idle_add (idle_callback, self);
 *   g_mutex_unlock (&self->idle_id_mutex);
 * }
 *
 * static void
 * some_widget_init (SomeWidget *self)
 * {
 *   g_mutex_init (&self->idle_id_mutex);
 *
 *   // ...
 * }
 *
 * static void
 * some_widget_finalize (xobject_t *object)
 * {
 *   SomeWidget *self = SOME_WIDGET (object);
 *
 *   if (self->idle_id)
 *     xsource_remove (self->idle_id);
 *
 *   g_mutex_clear (&self->idle_id_mutex);
 *
 *   G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * ]|
 *
 * This will fail in a multi-threaded application if the
 * widget is destroyed before the idle handler fires due
 * to the use after free in the callback. A solution, to
 * this particular problem, is to check to if the source
 * has already been destroy within the callback.
 *
 * |[<!-- language="C" -->
 * static xboolean_t
 * idle_callback (xpointer_t data)
 * {
 *   SomeWidget *self = data;
 *
 *   g_mutex_lock (&self->idle_id_mutex);
 *   if (!xsource_is_destroyed (g_main_current_source ()))
 *     {
 *       // do stuff with self
 *     }
 *   g_mutex_unlock (&self->idle_id_mutex);
 *
 *   return FALSE;
 * }
 * ]|
 *
 * Calls to this function from a thread other than the one acquired by the
 * #xmain_context_t the #xsource_t is attached to are typically redundant, as the
 * source could be destroyed immediately after this function returns. However,
 * once a source is destroyed it cannot be un-destroyed, so this function can be
 * used for opportunistic checks from any thread.
 *
 * Returns: %TRUE if the source has been destroyed
 *
 * Since: 2.12
 */
xboolean_t
xsource_is_destroyed (xsource_t *source)
{
  g_return_val_if_fail (source != NULL, TRUE);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, TRUE);
  return SOURCE_DESTROYED (source);
}

/* Temporarily remove all this source's file descriptors from the
 * poll(), so that if data comes available for one of the file descriptors
 * we don't continually spin in the poll()
 */
/* HOLDS: source->context's lock */
static void
block_source (xsource_t *source)
{
  xslist_t *tmp_list;

  g_return_if_fail (!SOURCE_BLOCKED (source));

  source->flags |= G_SOURCE_BLOCKED;

  if (source->context)
    {
      tmp_list = source->poll_fds;
      while (tmp_list)
        {
          xmain_context_remove_poll_unlocked (source->context, tmp_list->data);
          tmp_list = tmp_list->next;
        }

      for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next)
        xmain_context_remove_poll_unlocked (source->context, tmp_list->data);
    }

  if (source->priv && source->priv->child_sources)
    {
      tmp_list = source->priv->child_sources;
      while (tmp_list)
	{
	  block_source (tmp_list->data);
	  tmp_list = tmp_list->next;
	}
    }
}

/* HOLDS: source->context's lock */
static void
unblock_source (xsource_t *source)
{
  xslist_t *tmp_list;

  g_return_if_fail (SOURCE_BLOCKED (source)); /* Source already unblocked */
  g_return_if_fail (!SOURCE_DESTROYED (source));

  source->flags &= ~G_SOURCE_BLOCKED;

  tmp_list = source->poll_fds;
  while (tmp_list)
    {
      xmain_context_add_poll_unlocked (source->context, source->priority, tmp_list->data);
      tmp_list = tmp_list->next;
    }

  for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next)
    xmain_context_add_poll_unlocked (source->context, source->priority, tmp_list->data);

  if (source->priv && source->priv->child_sources)
    {
      tmp_list = source->priv->child_sources;
      while (tmp_list)
	{
	  unblock_source (tmp_list->data);
	  tmp_list = tmp_list->next;
	}
    }
}

/* HOLDS: context's lock */
static void
g_main_dispatch (xmain_context_t *context)
{
  GMainDispatch *current = get_dispatch ();
  xuint_t i;

  for (i = 0; i < context->pending_dispatches->len; i++)
    {
      xsource_t *source = context->pending_dispatches->pdata[i];

      context->pending_dispatches->pdata[i] = NULL;
      g_assert (source);

      source->flags &= ~G_SOURCE_READY;

      if (!SOURCE_DESTROYED (source))
	{
	  xboolean_t was_in_call;
	  xpointer_t user_data = NULL;
	  xsource_func_t callback = NULL;
	  xsource_callback_funcs_t *cb_funcs;
	  xpointer_t cb_data;
	  xboolean_t need_destroy;

	  xboolean_t (*dispatch) (xsource_t *,
				xsource_func_t,
				xpointer_t);
          xsource_t *prev_source;
          gint64 begin_time_nsec G_GNUC_UNUSED;

	  dispatch = source->source_funcs->dispatch;
	  cb_funcs = source->callback_funcs;
	  cb_data = source->callback_data;

	  if (cb_funcs)
	    cb_funcs->ref (cb_data);

	  if ((source->flags & G_SOURCE_CAN_RECURSE) == 0)
	    block_source (source);

	  was_in_call = source->flags & G_HOOK_FLAG_IN_CALL;
	  source->flags |= G_HOOK_FLAG_IN_CALL;

	  if (cb_funcs)
	    cb_funcs->get (cb_data, source, &callback, &user_data);

	  UNLOCK_CONTEXT (context);

          /* These operations are safe because 'current' is thread-local
           * and not modified from anywhere but this function.
           */
          prev_source = current->source;
          current->source = source;
          current->depth++;

          begin_time_nsec = G_TRACE_CURRENT_TIME;

          TRACE (XPL_MAIN_BEFORE_DISPATCH (xsource_get_name (source), source,
                                            dispatch, callback, user_data));
          need_destroy = !(* dispatch) (source, callback, user_data);
          TRACE (XPL_MAIN_AFTER_DISPATCH (xsource_get_name (source), source,
                                           dispatch, need_destroy));

          g_trace_mark (begin_time_nsec, G_TRACE_CURRENT_TIME - begin_time_nsec,
                        "GLib", "xsource_t.dispatch",
                        "%s ⇒ %s",
                        (xsource_get_name (source) != NULL) ? xsource_get_name (source) : "(unnamed)",
                        need_destroy ? "destroy" : "keep");

          current->source = prev_source;
          current->depth--;

	  if (cb_funcs)
	    cb_funcs->unref (cb_data);

 	  LOCK_CONTEXT (context);

	  if (!was_in_call)
	    source->flags &= ~G_HOOK_FLAG_IN_CALL;

	  if (SOURCE_BLOCKED (source) && !SOURCE_DESTROYED (source))
	    unblock_source (source);

	  /* Note: this depends on the fact that we can't switch
	   * sources from one main context to another
	   */
	  if (need_destroy && !SOURCE_DESTROYED (source))
	    {
	      g_assert (source->context == context);
	      xsource_destroy_internal (source, context, TRUE);
	    }
	}

      xsource_unref_internal (source, context, TRUE);
    }

  xptr_array_set_size (context->pending_dispatches, 0);
}

/**
 * xmain_context_acquire:
 * @context: a #xmain_context_t
 *
 * Tries to become the owner of the specified context.
 * If some other thread is the owner of the context,
 * returns %FALSE immediately. Ownership is properly
 * recursive: the owner can require ownership again
 * and will release ownership when xmain_context_release()
 * is called as many times as xmain_context_acquire().
 *
 * You must be the owner of a context before you
 * can call xmain_context_prepare(), xmain_context_query(),
 * xmain_context_check(), xmain_context_dispatch().
 *
 * Returns: %TRUE if the operation succeeded, and
 *   this thread is now the owner of @context.
 **/
xboolean_t
xmain_context_acquire (xmain_context_t *context)
{
  xboolean_t result = FALSE;
  xthread_t *self = G_THREAD_SELF;

  if (context == NULL)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);

  if (!context->owner)
    {
      context->owner = self;
      g_assert (context->owner_count == 0);
      TRACE (XPL_MAIN_CONTEXT_ACQUIRE (context, TRUE  /* success */));
    }

  if (context->owner == self)
    {
      context->owner_count++;
      result = TRUE;
    }
  else
    {
      TRACE (XPL_MAIN_CONTEXT_ACQUIRE (context, FALSE  /* failure */));
    }

  UNLOCK_CONTEXT (context);

  return result;
}

/**
 * xmain_context_release:
 * @context: a #xmain_context_t
 *
 * Releases ownership of a context previously acquired by this thread
 * with xmain_context_acquire(). If the context was acquired multiple
 * times, the ownership will be released only when xmain_context_release()
 * is called as many times as it was acquired.
 **/
void
xmain_context_release (xmain_context_t *context)
{
  if (context == NULL)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);

  context->owner_count--;
  if (context->owner_count == 0)
    {
      TRACE (XPL_MAIN_CONTEXT_RELEASE (context));

      context->owner = NULL;

      if (context->waiters)
	{
	  GMainWaiter *waiter = context->waiters->data;
	  xboolean_t loop_internal_waiter = (waiter->mutex == &context->mutex);
	  context->waiters = xslist_delete_link (context->waiters,
						  context->waiters);
	  if (!loop_internal_waiter)
	    g_mutex_lock (waiter->mutex);

	  g_cond_signal (waiter->cond);

	  if (!loop_internal_waiter)
	    g_mutex_unlock (waiter->mutex);
	}
    }

  UNLOCK_CONTEXT (context);
}

static xboolean_t
xmain_context_wait_internal (xmain_context_t *context,
                              xcond_t        *cond,
                              xmutex_t       *mutex)
{
  xboolean_t result = FALSE;
  xthread_t *self = G_THREAD_SELF;
  xboolean_t loop_internal_waiter;

  if (context == NULL)
    context = xmain_context_default ();

  loop_internal_waiter = (mutex == &context->mutex);

  if (!loop_internal_waiter)
    LOCK_CONTEXT (context);

  if (context->owner && context->owner != self)
    {
      GMainWaiter waiter;

      waiter.cond = cond;
      waiter.mutex = mutex;

      context->waiters = xslist_append (context->waiters, &waiter);

      if (!loop_internal_waiter)
        UNLOCK_CONTEXT (context);
      g_cond_wait (cond, mutex);
      if (!loop_internal_waiter)
        LOCK_CONTEXT (context);

      context->waiters = xslist_remove (context->waiters, &waiter);
    }

  if (!context->owner)
    {
      context->owner = self;
      g_assert (context->owner_count == 0);
    }

  if (context->owner == self)
    {
      context->owner_count++;
      result = TRUE;
    }

  if (!loop_internal_waiter)
    UNLOCK_CONTEXT (context);

  return result;
}

/**
 * xmain_context_wait:
 * @context: a #xmain_context_t
 * @cond: a condition variable
 * @mutex: a mutex, currently held
 *
 * Tries to become the owner of the specified context,
 * as with xmain_context_acquire(). But if another thread
 * is the owner, atomically drop @mutex and wait on @cond until
 * that owner releases ownership or until @cond is signaled, then
 * try again (once) to become the owner.
 *
 * Returns: %TRUE if the operation succeeded, and
 *   this thread is now the owner of @context.
 * Deprecated: 2.58: Use xmain_context_is_owner() and separate locking instead.
 */
xboolean_t
xmain_context_wait (xmain_context_t *context,
                     xcond_t        *cond,
                     xmutex_t       *mutex)
{
  if (context == NULL)
    context = xmain_context_default ();

  if (G_UNLIKELY (cond != &context->cond || mutex != &context->mutex))
    {
      static xboolean_t warned;

      if (!warned)
        {
          g_critical ("WARNING!! xmain_context_wait() will be removed in a future release.  "
                      "If you see this message, please file a bug immediately.");
          warned = TRUE;
        }
    }

  return xmain_context_wait_internal (context, cond, mutex);
}

/**
 * xmain_context_prepare:
 * @context: a #xmain_context_t
 * @priority: (out) (optional): location to store priority of highest priority
 *            source already ready.
 *
 * Prepares to poll sources within a main loop. The resulting information
 * for polling is determined by calling xmain_context_query ().
 *
 * You must have successfully acquired the context with
 * xmain_context_acquire() before you may call this function.
 *
 * Returns: %TRUE if some source is ready to be dispatched
 *               prior to polling.
 **/
xboolean_t
xmain_context_prepare (xmain_context_t *context,
			xint_t         *priority)
{
  xuint_t i;
  xint_t n_ready = 0;
  xint_t current_priority = G_MAXINT;
  xsource_t *source;
  GSourceIter iter;

  if (context == NULL)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);

  context->time_is_fresh = FALSE;

  if (context->in_check_or_prepare)
    {
      g_warning ("xmain_context_prepare() called recursively from within a source's check() or "
		 "prepare() member.");
      UNLOCK_CONTEXT (context);
      return FALSE;
    }

  TRACE (XPL_MAIN_CONTEXT_BEFORE_PREPARE (context));

#if 0
  /* If recursing, finish up current dispatch, before starting over */
  if (context->pending_dispatches)
    {
      if (dispatch)
	g_main_dispatch (context, &current_time);

      UNLOCK_CONTEXT (context);
      return TRUE;
    }
#endif

  /* If recursing, clear list of pending dispatches */

  for (i = 0; i < context->pending_dispatches->len; i++)
    {
      if (context->pending_dispatches->pdata[i])
        xsource_unref_internal ((xsource_t *)context->pending_dispatches->pdata[i], context, TRUE);
    }
  xptr_array_set_size (context->pending_dispatches, 0);

  /* Prepare all sources */

  context->timeout = -1;

  xsource_iter_init (&iter, context, TRUE);
  while (xsource_iter_next (&iter, &source))
    {
      xint_t source_timeout = -1;

      if (SOURCE_DESTROYED (source) || SOURCE_BLOCKED (source))
	continue;
      if ((n_ready > 0) && (source->priority > current_priority))
	break;

      if (!(source->flags & G_SOURCE_READY))
	{
	  xboolean_t result;
	  xboolean_t (* prepare) (xsource_t  *source,
                                xint_t     *timeout);

          prepare = source->source_funcs->prepare;

          if (prepare)
            {
              gint64 begin_time_nsec G_GNUC_UNUSED;

              context->in_check_or_prepare++;
              UNLOCK_CONTEXT (context);

              begin_time_nsec = G_TRACE_CURRENT_TIME;

              result = (* prepare) (source, &source_timeout);
              TRACE (XPL_MAIN_AFTER_PREPARE (source, prepare, source_timeout));

              g_trace_mark (begin_time_nsec, G_TRACE_CURRENT_TIME - begin_time_nsec,
                            "GLib", "xsource_t.prepare",
                            "%s ⇒ %s",
                            (xsource_get_name (source) != NULL) ? xsource_get_name (source) : "(unnamed)",
                            result ? "ready" : "unready");

              LOCK_CONTEXT (context);
              context->in_check_or_prepare--;
            }
          else
            {
              source_timeout = -1;
              result = FALSE;
            }

          if (result == FALSE && source->priv->ready_time != -1)
            {
              if (!context->time_is_fresh)
                {
                  context->time = g_get_monotonic_time ();
                  context->time_is_fresh = TRUE;
                }

              if (source->priv->ready_time <= context->time)
                {
                  source_timeout = 0;
                  result = TRUE;
                }
              else
                {
                  gint64 timeout;

                  /* rounding down will lead to spinning, so always round up */
                  timeout = (source->priv->ready_time - context->time + 999) / 1000;

                  if (source_timeout < 0 || timeout < source_timeout)
                    source_timeout = MIN (timeout, G_MAXINT);
                }
            }

	  if (result)
	    {
	      xsource_t *ready_source = source;

	      while (ready_source)
		{
		  ready_source->flags |= G_SOURCE_READY;
		  ready_source = ready_source->priv->parent_source;
		}
	    }
	}

      if (source->flags & G_SOURCE_READY)
	{
	  n_ready++;
	  current_priority = source->priority;
	  context->timeout = 0;
	}

      if (source_timeout >= 0)
	{
	  if (context->timeout < 0)
	    context->timeout = source_timeout;
	  else
	    context->timeout = MIN (context->timeout, source_timeout);
	}
    }
  xsource_iter_clear (&iter);

  TRACE (XPL_MAIN_CONTEXT_AFTER_PREPARE (context, current_priority, n_ready));

  UNLOCK_CONTEXT (context);

  if (priority)
    *priority = current_priority;

  return (n_ready > 0);
}

/**
 * xmain_context_query:
 * @context: a #xmain_context_t
 * @max_priority: maximum priority source to check
 * @timeout_: (out): location to store timeout to be used in polling
 * @fds: (out caller-allocates) (array length=n_fds): location to
 *       store #xpollfd_t records that need to be polled.
 * @n_fds: (in): length of @fds.
 *
 * Determines information necessary to poll this main loop. You should
 * be careful to pass the resulting @fds array and its length @n_fds
 * as is when calling xmain_context_check(), as this function relies
 * on assumptions made when the array is filled.
 *
 * You must have successfully acquired the context with
 * xmain_context_acquire() before you may call this function.
 *
 * Returns: the number of records actually stored in @fds,
 *   or, if more than @n_fds records need to be stored, the number
 *   of records that need to be stored.
 **/
xint_t
xmain_context_query (xmain_context_t *context,
		      xint_t          max_priority,
		      xint_t         *timeout,
		      xpollfd_t      *fds,
		      xint_t          n_fds)
{
  xint_t n_poll;
  GPollRec *pollrec, *lastpollrec;
  gushort events;

  LOCK_CONTEXT (context);

  TRACE (XPL_MAIN_CONTEXT_BEFORE_QUERY (context, max_priority));

  /* fds is filled sequentially from poll_records. Since poll_records
   * are incrementally sorted by file descriptor identifier, fds will
   * also be incrementally sorted.
   */
  n_poll = 0;
  lastpollrec = NULL;
  for (pollrec = context->poll_records; pollrec; pollrec = pollrec->next)
    {
      if (pollrec->priority > max_priority)
        continue;

      /* In direct contradiction to the Unix98 spec, IRIX runs into
       * difficulty if you pass in POLLERR, POLLHUP or POLLNVAL
       * flags in the events field of the pollfd while it should
       * just ignoring them. So we mask them out here.
       */
      events = pollrec->fd->events & ~(G_IO_ERR|G_IO_HUP|G_IO_NVAL);

      /* This optimization --using the same xpollfd_t to poll for more
       * than one poll record-- relies on the poll records being
       * incrementally sorted.
       */
      if (lastpollrec && pollrec->fd->fd == lastpollrec->fd->fd)
        {
          if (n_poll - 1 < n_fds)
            fds[n_poll - 1].events |= events;
        }
      else
        {
          if (n_poll < n_fds)
            {
              fds[n_poll].fd = pollrec->fd->fd;
              fds[n_poll].events = events;
              fds[n_poll].revents = 0;
            }

          n_poll++;
        }

      lastpollrec = pollrec;
    }

  context->poll_changed = FALSE;

  if (timeout)
    {
      *timeout = context->timeout;
      if (*timeout != 0)
        context->time_is_fresh = FALSE;
    }

  TRACE (XPL_MAIN_CONTEXT_AFTER_QUERY (context, context->timeout,
                                        fds, n_poll));

  UNLOCK_CONTEXT (context);

  return n_poll;
}

/**
 * xmain_context_check:
 * @context: a #xmain_context_t
 * @max_priority: the maximum numerical priority of sources to check
 * @fds: (array length=n_fds): array of #xpollfd_t's that was passed to
 *       the last call to xmain_context_query()
 * @n_fds: return value of xmain_context_query()
 *
 * Passes the results of polling back to the main loop. You should be
 * careful to pass @fds and its length @n_fds as received from
 * xmain_context_query(), as this functions relies on assumptions
 * on how @fds is filled.
 *
 * You must have successfully acquired the context with
 * xmain_context_acquire() before you may call this function.
 *
 * Returns: %TRUE if some sources are ready to be dispatched.
 **/
xboolean_t
xmain_context_check (xmain_context_t *context,
		      xint_t          max_priority,
		      xpollfd_t      *fds,
		      xint_t          n_fds)
{
  xsource_t *source;
  GSourceIter iter;
  GPollRec *pollrec;
  xint_t n_ready = 0;
  xint_t i;

  LOCK_CONTEXT (context);

  if (context->in_check_or_prepare)
    {
      g_warning ("xmain_context_check() called recursively from within a source's check() or "
		 "prepare() member.");
      UNLOCK_CONTEXT (context);
      return FALSE;
    }

  TRACE (XPL_MAIN_CONTEXT_BEFORE_CHECK (context, max_priority, fds, n_fds));

  for (i = 0; i < n_fds; i++)
    {
      if (fds[i].fd == context->wake_up_rec.fd)
        {
          if (fds[i].revents)
            {
              TRACE (XPL_MAIN_CONTEXT_WAKEUP_ACKNOWLEDGE (context));
              g_wakeup_acknowledge (context->wakeup);
            }
          break;
        }
    }

  /* If the set of poll file descriptors changed, bail out
   * and let the main loop rerun
   */
  if (context->poll_changed)
    {
      TRACE (XPL_MAIN_CONTEXT_AFTER_CHECK (context, 0));

      UNLOCK_CONTEXT (context);
      return FALSE;
    }

  /* The linear iteration below relies on the assumption that both
   * poll records and the fds array are incrementally sorted by file
   * descriptor identifier.
   */
  pollrec = context->poll_records;
  i = 0;
  while (pollrec && i < n_fds)
    {
      /* Make sure that fds is sorted by file descriptor identifier. */
      g_assert (i <= 0 || fds[i - 1].fd < fds[i].fd);

      /* Skip until finding the first GPollRec matching the current xpollfd_t. */
      while (pollrec && pollrec->fd->fd != fds[i].fd)
        pollrec = pollrec->next;

      /* Update all consecutive GPollRecs that match. */
      while (pollrec && pollrec->fd->fd == fds[i].fd)
        {
          if (pollrec->priority <= max_priority)
            {
              pollrec->fd->revents =
                fds[i].revents & (pollrec->fd->events | G_IO_ERR | G_IO_HUP | G_IO_NVAL);
            }
          pollrec = pollrec->next;
        }

      /* Iterate to next xpollfd_t. */
      i++;
    }

  xsource_iter_init (&iter, context, TRUE);
  while (xsource_iter_next (&iter, &source))
    {
      if (SOURCE_DESTROYED (source) || SOURCE_BLOCKED (source))
	continue;
      if ((n_ready > 0) && (source->priority > max_priority))
	break;

      if (!(source->flags & G_SOURCE_READY))
	{
          xboolean_t result;
          xboolean_t (* check) (xsource_t *source);

          check = source->source_funcs->check;

          if (check)
            {
              gint64 begin_time_nsec G_GNUC_UNUSED;

              /* If the check function is set, call it. */
              context->in_check_or_prepare++;
              UNLOCK_CONTEXT (context);

              begin_time_nsec = G_TRACE_CURRENT_TIME;

              result = (* check) (source);

              TRACE (XPL_MAIN_AFTER_CHECK (source, check, result));

              g_trace_mark (begin_time_nsec, G_TRACE_CURRENT_TIME - begin_time_nsec,
                            "GLib", "xsource_t.check",
                            "%s ⇒ %s",
                            (xsource_get_name (source) != NULL) ? xsource_get_name (source) : "(unnamed)",
                            result ? "dispatch" : "ignore");

              LOCK_CONTEXT (context);
              context->in_check_or_prepare--;
            }
          else
            result = FALSE;

          if (result == FALSE)
            {
              xslist_t *tmp_list;

              /* If not already explicitly flagged ready by ->check()
               * (or if we have no check) then we can still be ready if
               * any of our fds poll as ready.
               */
              for (tmp_list = source->priv->fds; tmp_list; tmp_list = tmp_list->next)
                {
                  xpollfd_t *pollfd = tmp_list->data;

                  if (pollfd->revents)
                    {
                      result = TRUE;
                      break;
                    }
                }
            }

          if (result == FALSE && source->priv->ready_time != -1)
            {
              if (!context->time_is_fresh)
                {
                  context->time = g_get_monotonic_time ();
                  context->time_is_fresh = TRUE;
                }

              if (source->priv->ready_time <= context->time)
                result = TRUE;
            }

	  if (result)
	    {
	      xsource_t *ready_source = source;

	      while (ready_source)
		{
		  ready_source->flags |= G_SOURCE_READY;
		  ready_source = ready_source->priv->parent_source;
		}
	    }
	}

      if (source->flags & G_SOURCE_READY)
	{
          xsource_ref (source);
	  xptr_array_add (context->pending_dispatches, source);

	  n_ready++;

          /* never dispatch sources with less priority than the first
           * one we choose to dispatch
           */
          max_priority = source->priority;
	}
    }
  xsource_iter_clear (&iter);

  TRACE (XPL_MAIN_CONTEXT_AFTER_CHECK (context, n_ready));

  UNLOCK_CONTEXT (context);

  return n_ready > 0;
}

/**
 * xmain_context_dispatch:
 * @context: a #xmain_context_t
 *
 * Dispatches all pending sources.
 *
 * You must have successfully acquired the context with
 * xmain_context_acquire() before you may call this function.
 **/
void
xmain_context_dispatch (xmain_context_t *context)
{
  LOCK_CONTEXT (context);

  TRACE (XPL_MAIN_CONTEXT_BEFORE_DISPATCH (context));

  if (context->pending_dispatches->len > 0)
    {
      g_main_dispatch (context);
    }

  TRACE (XPL_MAIN_CONTEXT_AFTER_DISPATCH (context));

  UNLOCK_CONTEXT (context);
}

/* HOLDS context lock */
static xboolean_t
xmain_context_iterate (xmain_context_t *context,
			xboolean_t      block,
			xboolean_t      dispatch,
			xthread_t      *self)
{
  xint_t max_priority;
  xint_t timeout;
  xboolean_t some_ready;
  xint_t nfds, allocated_nfds;
  xpollfd_t *fds = NULL;
  gint64 begin_time_nsec G_GNUC_UNUSED;

  UNLOCK_CONTEXT (context);

  begin_time_nsec = G_TRACE_CURRENT_TIME;

  if (!xmain_context_acquire (context))
    {
      xboolean_t got_ownership;

      LOCK_CONTEXT (context);

      if (!block)
	return FALSE;

      got_ownership = xmain_context_wait_internal (context,
                                                    &context->cond,
                                                    &context->mutex);

      if (!got_ownership)
	return FALSE;
    }
  else
    LOCK_CONTEXT (context);

  if (!context->cached_poll_array)
    {
      context->cached_poll_array_size = context->n_poll_records;
      context->cached_poll_array = g_new (xpollfd_t, context->n_poll_records);
    }

  allocated_nfds = context->cached_poll_array_size;
  fds = context->cached_poll_array;

  UNLOCK_CONTEXT (context);

  xmain_context_prepare (context, &max_priority);

  while ((nfds = xmain_context_query (context, max_priority, &timeout, fds,
				       allocated_nfds)) > allocated_nfds)
    {
      LOCK_CONTEXT (context);
      g_free (fds);
      context->cached_poll_array_size = allocated_nfds = nfds;
      context->cached_poll_array = fds = g_new (xpollfd_t, nfds);
      UNLOCK_CONTEXT (context);
    }

  if (!block)
    timeout = 0;

  xmain_context_poll (context, timeout, max_priority, fds, nfds);

  some_ready = xmain_context_check (context, max_priority, fds, nfds);

  if (dispatch)
    xmain_context_dispatch (context);

  xmain_context_release (context);

  g_trace_mark (begin_time_nsec, G_TRACE_CURRENT_TIME - begin_time_nsec,
                "GLib", "xmain_context_iterate",
                "Context %p, %s ⇒ %s", context, block ? "blocking" : "non-blocking", some_ready ? "dispatched" : "nothing");

  LOCK_CONTEXT (context);

  return some_ready;
}

/**
 * xmain_context_pending:
 * @context: (nullable): a #xmain_context_t (if %NULL, the default context will be used)
 *
 * Checks if any sources have pending events for the given context.
 *
 * Returns: %TRUE if events are pending.
 **/
xboolean_t
xmain_context_pending (xmain_context_t *context)
{
  xboolean_t retval;

  if (!context)
    context = xmain_context_default();

  LOCK_CONTEXT (context);
  retval = xmain_context_iterate (context, FALSE, FALSE, G_THREAD_SELF);
  UNLOCK_CONTEXT (context);

  return retval;
}

/**
 * xmain_context_iteration:
 * @context: (nullable): a #xmain_context_t (if %NULL, the default context will be used)
 * @may_block: whether the call may block.
 *
 * Runs a single iteration for the given main loop. This involves
 * checking to see if any event sources are ready to be processed,
 * then if no events sources are ready and @may_block is %TRUE, waiting
 * for a source to become ready, then dispatching the highest priority
 * events sources that are ready. Otherwise, if @may_block is %FALSE
 * sources are not waited to become ready, only those highest priority
 * events sources will be dispatched (if any), that are ready at this
 * given moment without further waiting.
 *
 * Note that even when @may_block is %TRUE, it is still possible for
 * xmain_context_iteration() to return %FALSE, since the wait may
 * be interrupted for other reasons than an event source becoming ready.
 *
 * Returns: %TRUE if events were dispatched.
 **/
xboolean_t
xmain_context_iteration (xmain_context_t *context, xboolean_t may_block)
{
  xboolean_t retval;

  if (!context)
    context = xmain_context_default();

  LOCK_CONTEXT (context);
  retval = xmain_context_iterate (context, may_block, TRUE, G_THREAD_SELF);
  UNLOCK_CONTEXT (context);

  return retval;
}

/**
 * xmain_loop_new:
 * @context: (nullable): a #xmain_context_t  (if %NULL, the default context will be used).
 * @is_running: set to %TRUE to indicate that the loop is running. This
 * is not very important since calling xmain_loop_run() will set this to
 * %TRUE anyway.
 *
 * Creates a new #xmain_loop_t structure.
 *
 * Returns: a new #xmain_loop_t.
 **/
xmain_loop_t *
xmain_loop_new (xmain_context_t *context,
		 xboolean_t      is_running)
{
  xmain_loop_t *loop;

  if (!context)
    context = xmain_context_default();

  xmain_context_ref (context);

  loop = g_new0 (xmain_loop_t, 1);
  loop->context = context;
  loop->is_running = is_running != FALSE;
  loop->ref_count = 1;

  TRACE (XPL_MAIN_LOOP_NEW (loop, context));

  return loop;
}

/**
 * xmain_loop_ref:
 * @loop: a #xmain_loop_t
 *
 * Increases the reference count on a #xmain_loop_t object by one.
 *
 * Returns: @loop
 **/
xmain_loop_t *
xmain_loop_ref (xmain_loop_t *loop)
{
  g_return_val_if_fail (loop != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&loop->ref_count) > 0, NULL);

  g_atomic_int_inc (&loop->ref_count);

  return loop;
}

/**
 * xmain_loop_unref:
 * @loop: a #xmain_loop_t
 *
 * Decreases the reference count on a #xmain_loop_t object by one. If
 * the result is zero, free the loop and free all associated memory.
 **/
void
xmain_loop_unref (xmain_loop_t *loop)
{
  g_return_if_fail (loop != NULL);
  g_return_if_fail (g_atomic_int_get (&loop->ref_count) > 0);

  if (!g_atomic_int_dec_and_test (&loop->ref_count))
    return;

  xmain_context_unref (loop->context);
  g_free (loop);
}

/**
 * xmain_loop_run:
 * @loop: a #xmain_loop_t
 *
 * Runs a main loop until xmain_loop_quit() is called on the loop.
 * If this is called for the thread of the loop's #xmain_context_t,
 * it will process events from the loop, otherwise it will
 * simply wait.
 **/
void
xmain_loop_run (xmain_loop_t *loop)
{
  xthread_t *self = G_THREAD_SELF;

  g_return_if_fail (loop != NULL);
  g_return_if_fail (g_atomic_int_get (&loop->ref_count) > 0);

  /* Hold a reference in case the loop is unreffed from a callback function */
  g_atomic_int_inc (&loop->ref_count);

  if (!xmain_context_acquire (loop->context))
    {
      xboolean_t got_ownership = FALSE;

      /* Another thread owns this context */
      LOCK_CONTEXT (loop->context);

      g_atomic_int_set (&loop->is_running, TRUE);

      while (g_atomic_int_get (&loop->is_running) && !got_ownership)
        got_ownership = xmain_context_wait_internal (loop->context,
                                                      &loop->context->cond,
                                                      &loop->context->mutex);

      if (!g_atomic_int_get (&loop->is_running))
	{
	  UNLOCK_CONTEXT (loop->context);
	  if (got_ownership)
	    xmain_context_release (loop->context);
	  xmain_loop_unref (loop);
	  return;
	}

      g_assert (got_ownership);
    }
  else
    LOCK_CONTEXT (loop->context);

  if (loop->context->in_check_or_prepare)
    {
      g_warning ("xmain_loop_run(): called recursively from within a source's "
		 "check() or prepare() member, iteration not possible.");
      xmain_loop_unref (loop);
      return;
    }

  g_atomic_int_set (&loop->is_running, TRUE);
  while (g_atomic_int_get (&loop->is_running))
    xmain_context_iterate (loop->context, TRUE, TRUE, self);

  UNLOCK_CONTEXT (loop->context);

  xmain_context_release (loop->context);

  xmain_loop_unref (loop);
}

/**
 * xmain_loop_quit:
 * @loop: a #xmain_loop_t
 *
 * Stops a #xmain_loop_t from running. Any calls to xmain_loop_run()
 * for the loop will return.
 *
 * Note that sources that have already been dispatched when
 * xmain_loop_quit() is called will still be executed.
 **/
void
xmain_loop_quit (xmain_loop_t *loop)
{
  g_return_if_fail (loop != NULL);
  g_return_if_fail (g_atomic_int_get (&loop->ref_count) > 0);

  LOCK_CONTEXT (loop->context);
  g_atomic_int_set (&loop->is_running, FALSE);
  g_wakeup_signal (loop->context->wakeup);

  g_cond_broadcast (&loop->context->cond);

  UNLOCK_CONTEXT (loop->context);

  TRACE (XPL_MAIN_LOOP_QUIT (loop));
}

/**
 * xmain_loop_is_running:
 * @loop: a #xmain_loop_t.
 *
 * Checks to see if the main loop is currently being run via xmain_loop_run().
 *
 * Returns: %TRUE if the mainloop is currently being run.
 **/
xboolean_t
xmain_loop_is_running (xmain_loop_t *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  g_return_val_if_fail (g_atomic_int_get (&loop->ref_count) > 0, FALSE);

  return g_atomic_int_get (&loop->is_running);
}

/**
 * xmain_loop_get_context:
 * @loop: a #xmain_loop_t.
 *
 * Returns the #xmain_context_t of @loop.
 *
 * Returns: (transfer none): the #xmain_context_t of @loop
 **/
xmain_context_t *
xmain_loop_get_context (xmain_loop_t *loop)
{
  g_return_val_if_fail (loop != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&loop->ref_count) > 0, NULL);

  return loop->context;
}

/* HOLDS: context's lock */
static void
xmain_context_poll (xmain_context_t *context,
		     xint_t          timeout,
		     xint_t          priority,
		     xpollfd_t      *fds,
		     xint_t          n_fds)
{
#ifdef  G_MAIN_POLL_DEBUG
  xtimer_t *poll_timer;
  GPollRec *pollrec;
  xint_t i;
#endif

  GPollFunc poll_func;

  if (n_fds || timeout != 0)
    {
      int ret, errsv;

#ifdef	G_MAIN_POLL_DEBUG
      poll_timer = NULL;
      if (_g_main_poll_debug)
	{
	  g_print ("polling context=%p n=%d timeout=%d\n",
		   context, n_fds, timeout);
	  poll_timer = g_timer_new ();
	}
#endif

      LOCK_CONTEXT (context);

      poll_func = context->poll_func;

      UNLOCK_CONTEXT (context);
      ret = (*poll_func) (fds, n_fds, timeout);
      errsv = errno;
      if (ret < 0 && errsv != EINTR)
	{
#ifndef G_OS_WIN32
	  g_warning ("poll(2) failed due to: %s.",
		     xstrerror (errsv));
#else
	  /* If g_poll () returns -1, it has already called g_warning() */
#endif
	}

#ifdef	G_MAIN_POLL_DEBUG
      if (_g_main_poll_debug)
	{
	  LOCK_CONTEXT (context);

	  g_print ("g_main_poll(%d) timeout: %d - elapsed %12.10f seconds",
		   n_fds,
		   timeout,
		   g_timer_elapsed (poll_timer, NULL));
	  g_timer_destroy (poll_timer);
	  pollrec = context->poll_records;

	  while (pollrec != NULL)
	    {
	      i = 0;
	      while (i < n_fds)
		{
		  if (fds[i].fd == pollrec->fd->fd &&
		      pollrec->fd->events &&
		      fds[i].revents)
		    {
		      g_print (" [" G_POLLFD_FORMAT " :", fds[i].fd);
		      if (fds[i].revents & G_IO_IN)
			g_print ("i");
		      if (fds[i].revents & G_IO_OUT)
			g_print ("o");
		      if (fds[i].revents & G_IO_PRI)
			g_print ("p");
		      if (fds[i].revents & G_IO_ERR)
			g_print ("e");
		      if (fds[i].revents & G_IO_HUP)
			g_print ("h");
		      if (fds[i].revents & G_IO_NVAL)
			g_print ("n");
		      g_print ("]");
		    }
		  i++;
		}
	      pollrec = pollrec->next;
	    }
	  g_print ("\n");

	  UNLOCK_CONTEXT (context);
	}
#endif
    } /* if (n_fds || timeout != 0) */
}

/**
 * xmain_context_add_poll:
 * @context: (nullable): a #xmain_context_t (or %NULL for the default context)
 * @fd: a #xpollfd_t structure holding information about a file
 *      descriptor to watch.
 * @priority: the priority for this file descriptor which should be
 *      the same as the priority used for xsource_attach() to ensure that the
 *      file descriptor is polled whenever the results may be needed.
 *
 * Adds a file descriptor to the set of file descriptors polled for
 * this context. This will very seldom be used directly. Instead
 * a typical event source will use xsource_add_unix_fd() instead.
 **/
void
xmain_context_add_poll (xmain_context_t *context,
			 xpollfd_t      *fd,
			 xint_t          priority)
{
  if (!context)
    context = xmain_context_default ();

  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);
  g_return_if_fail (fd);

  LOCK_CONTEXT (context);
  xmain_context_add_poll_unlocked (context, priority, fd);
  UNLOCK_CONTEXT (context);
}

/* HOLDS: main_loop_lock */
static void
xmain_context_add_poll_unlocked (xmain_context_t *context,
				  xint_t          priority,
				  xpollfd_t      *fd)
{
  GPollRec *prevrec, *nextrec;
  GPollRec *newrec = g_slice_new (GPollRec);

  /* This file descriptor may be checked before we ever poll */
  fd->revents = 0;
  newrec->fd = fd;
  newrec->priority = priority;

  /* Poll records are incrementally sorted by file descriptor identifier. */
  prevrec = NULL;
  nextrec = context->poll_records;
  while (nextrec)
    {
      if (nextrec->fd->fd > fd->fd)
        break;
      prevrec = nextrec;
      nextrec = nextrec->next;
    }

  if (prevrec)
    prevrec->next = newrec;
  else
    context->poll_records = newrec;

  newrec->prev = prevrec;
  newrec->next = nextrec;

  if (nextrec)
    nextrec->prev = newrec;

  context->n_poll_records++;

  context->poll_changed = TRUE;

  /* Now wake up the main loop if it is waiting in the poll() */
  g_wakeup_signal (context->wakeup);
}

/**
 * xmain_context_remove_poll:
 * @context:a #xmain_context_t
 * @fd: a #xpollfd_t descriptor previously added with xmain_context_add_poll()
 *
 * Removes file descriptor from the set of file descriptors to be
 * polled for a particular context.
 **/
void
xmain_context_remove_poll (xmain_context_t *context,
			    xpollfd_t      *fd)
{
  if (!context)
    context = xmain_context_default ();

  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);
  g_return_if_fail (fd);

  LOCK_CONTEXT (context);
  xmain_context_remove_poll_unlocked (context, fd);
  UNLOCK_CONTEXT (context);
}

static void
xmain_context_remove_poll_unlocked (xmain_context_t *context,
				     xpollfd_t      *fd)
{
  GPollRec *pollrec, *prevrec, *nextrec;

  prevrec = NULL;
  pollrec = context->poll_records;

  while (pollrec)
    {
      nextrec = pollrec->next;
      if (pollrec->fd == fd)
	{
	  if (prevrec != NULL)
	    prevrec->next = nextrec;
	  else
	    context->poll_records = nextrec;

	  if (nextrec != NULL)
	    nextrec->prev = prevrec;

	  g_slice_free (GPollRec, pollrec);

	  context->n_poll_records--;
	  break;
	}
      prevrec = pollrec;
      pollrec = nextrec;
    }

  context->poll_changed = TRUE;

  /* Now wake up the main loop if it is waiting in the poll() */
  g_wakeup_signal (context->wakeup);
}

/**
 * xsource_get_current_time:
 * @source:  a #xsource_t
 * @timeval: #GTimeVal structure in which to store current time.
 *
 * This function ignores @source and is otherwise the same as
 * g_get_current_time().
 *
 * Deprecated: 2.28: use xsource_get_time() instead
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
xsource_get_current_time (xsource_t  *source,
			   GTimeVal *timeval)
{
  g_get_current_time (timeval);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xsource_get_time:
 * @source: a #xsource_t
 *
 * Gets the time to be used when checking this source. The advantage of
 * calling this function over calling g_get_monotonic_time() directly is
 * that when checking multiple sources, GLib can cache a single value
 * instead of having to repeatedly get the system monotonic time.
 *
 * The time here is the system monotonic time, if available, or some
 * other reasonable alternative otherwise.  See g_get_monotonic_time().
 *
 * Returns: the monotonic time in microseconds
 *
 * Since: 2.28
 **/
gint64
xsource_get_time (xsource_t *source)
{
  xmain_context_t *context;
  gint64 result;

  g_return_val_if_fail (source != NULL, 0);
  g_return_val_if_fail (g_atomic_int_get (&source->ref_count) > 0, 0);
  g_return_val_if_fail (source->context != NULL, 0);

  context = source->context;

  LOCK_CONTEXT (context);

  if (!context->time_is_fresh)
    {
      context->time = g_get_monotonic_time ();
      context->time_is_fresh = TRUE;
    }

  result = context->time;

  UNLOCK_CONTEXT (context);

  return result;
}

/**
 * xmain_context_set_poll_func:
 * @context: a #xmain_context_t
 * @func: the function to call to poll all file descriptors
 *
 * Sets the function to use to handle polling of file descriptors. It
 * will be used instead of the poll() system call
 * (or GLib's replacement function, which is used where
 * poll() isn't available).
 *
 * This function could possibly be used to integrate the GLib event
 * loop with an external event loop.
 **/
void
xmain_context_set_poll_func (xmain_context_t *context,
			      GPollFunc     func)
{
  if (!context)
    context = xmain_context_default ();

  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);

  LOCK_CONTEXT (context);

  if (func)
    context->poll_func = func;
  else
    context->poll_func = g_poll;

  UNLOCK_CONTEXT (context);
}

/**
 * xmain_context_get_poll_func:
 * @context: a #xmain_context_t
 *
 * Gets the poll function set by xmain_context_set_poll_func().
 *
 * Returns: the poll function
 **/
GPollFunc
xmain_context_get_poll_func (xmain_context_t *context)
{
  GPollFunc result;

  if (!context)
    context = xmain_context_default ();

  g_return_val_if_fail (g_atomic_int_get (&context->ref_count) > 0, NULL);

  LOCK_CONTEXT (context);
  result = context->poll_func;
  UNLOCK_CONTEXT (context);

  return result;
}

/**
 * xmain_context_wakeup:
 * @context: a #xmain_context_t
 *
 * If @context is currently blocking in xmain_context_iteration()
 * waiting for a source to become ready, cause it to stop blocking
 * and return.  Otherwise, cause the next invocation of
 * xmain_context_iteration() to return without blocking.
 *
 * This API is useful for low-level control over #xmain_context_t; for
 * example, integrating it with main loop implementations such as
 * #xmain_loop_t.
 *
 * Another related use for this function is when implementing a main
 * loop with a termination condition, computed from multiple threads:
 *
 * |[<!-- language="C" -->
 *   #define NUM_TASKS 10
 *   static xint_t tasks_remaining = NUM_TASKS;  // (atomic)
 *   ...
 *
 *   while (g_atomic_int_get (&tasks_remaining) != 0)
 *     xmain_context_iteration (NULL, TRUE);
 * ]|
 *
 * Then in a thread:
 * |[<!-- language="C" -->
 *   perform_work();
 *
 *   if (g_atomic_int_dec_and_test (&tasks_remaining))
 *     xmain_context_wakeup (NULL);
 * ]|
 **/
void
xmain_context_wakeup (xmain_context_t *context)
{
  if (!context)
    context = xmain_context_default ();

  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);

  TRACE (XPL_MAIN_CONTEXT_WAKEUP (context));

  g_wakeup_signal (context->wakeup);
}

/**
 * xmain_context_is_owner:
 * @context: a #xmain_context_t
 *
 * Determines whether this thread holds the (recursive)
 * ownership of this #xmain_context_t. This is useful to
 * know before waiting on another thread that may be
 * blocking to get ownership of @context.
 *
 * Returns: %TRUE if current thread is owner of @context.
 *
 * Since: 2.10
 **/
xboolean_t
xmain_context_is_owner (xmain_context_t *context)
{
  xboolean_t is_owner;

  if (!context)
    context = xmain_context_default ();

  LOCK_CONTEXT (context);
  is_owner = context->owner == G_THREAD_SELF;
  UNLOCK_CONTEXT (context);

  return is_owner;
}

/* Timeouts */

static void
g_timeout_set_expiration (GTimeoutSource *timeout_source,
                          gint64          current_time)
{
  gint64 expiration;

  if (timeout_source->seconds)
    {
      gint64 remainder;
      static xint_t timer_perturb = -1;

      if (timer_perturb == -1)
        {
          /*
           * we want a per machine/session unique 'random' value; try the dbus
           * address first, that has a UUID in it. If there is no dbus, use the
           * hostname for hashing.
           */
          const char *session_bus_address = g_getenv ("DBUS_SESSION_BUS_ADDRESS");
          if (!session_bus_address)
            session_bus_address = g_getenv ("HOSTNAME");
          if (session_bus_address)
            timer_perturb = ABS ((xint_t) xstr_hash (session_bus_address)) % 1000000;
          else
            timer_perturb = 0;
        }

      expiration = current_time + (xuint64_t) timeout_source->interval * 1000 * 1000;

      /* We want the microseconds part of the timeout to land on the
       * 'timer_perturb' mark, but we need to make sure we don't try to
       * set the timeout in the past.  We do this by ensuring that we
       * always only *increase* the expiration time by adding a full
       * second in the case that the microsecond portion decreases.
       */
      expiration -= timer_perturb;

      remainder = expiration % 1000000;
      if (remainder >= 1000000/4)
        expiration += 1000000;

      expiration -= remainder;
      expiration += timer_perturb;
    }
  else
    {
      expiration = current_time + (xuint64_t) timeout_source->interval * 1000;
    }

  xsource_set_ready_time ((xsource_t *) timeout_source, expiration);
}

static xboolean_t
g_timeout_dispatch (xsource_t     *source,
                    xsource_func_t  callback,
                    xpointer_t     user_data)
{
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;
  xboolean_t again;

  if (!callback)
    {
      g_warning ("Timeout source dispatched without callback. "
                 "You must call xsource_set_callback().");
      return FALSE;
    }

  again = callback (user_data);

  TRACE (XPL_TIMEOUT_DISPATCH (source, source->context, callback, user_data, again));

  if (again)
    g_timeout_set_expiration (timeout_source, xsource_get_time (source));

  return again;
}

/**
 * g_timeout_source_new:
 * @interval: the timeout interval in milliseconds.
 *
 * Creates a new timeout source.
 *
 * The source will not initially be associated with any #xmain_context_t
 * and must be added to one with xsource_attach() before it will be
 * executed.
 *
 * The interval given is in terms of monotonic time, not wall clock
 * time.  See g_get_monotonic_time().
 *
 * Returns: the newly-created timeout source
 **/
xsource_t *
g_timeout_source_new (xuint_t interval)
{
  xsource_t *source = xsource_new (&g_timeout_funcs, sizeof (GTimeoutSource));
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  timeout_source->interval = interval;
  g_timeout_set_expiration (timeout_source, g_get_monotonic_time ());

  return source;
}

/**
 * g_timeout_source_new_seconds:
 * @interval: the timeout interval in seconds
 *
 * Creates a new timeout source.
 *
 * The source will not initially be associated with any #xmain_context_t
 * and must be added to one with xsource_attach() before it will be
 * executed.
 *
 * The scheduling granularity/accuracy of this timeout source will be
 * in seconds.
 *
 * The interval given is in terms of monotonic time, not wall clock time.
 * See g_get_monotonic_time().
 *
 * Returns: the newly-created timeout source
 *
 * Since: 2.14
 **/
xsource_t *
g_timeout_source_new_seconds (xuint_t interval)
{
  xsource_t *source = xsource_new (&g_timeout_funcs, sizeof (GTimeoutSource));
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  timeout_source->interval = interval;
  timeout_source->seconds = TRUE;

  g_timeout_set_expiration (timeout_source, g_get_monotonic_time ());

  return source;
}


/**
 * g_timeout_add_full: (rename-to g_timeout_add)
 * @priority: the priority of the timeout source. Typically this will be in
 *   the range between %G_PRIORITY_DEFAULT and %G_PRIORITY_HIGH.
 * @interval: the time between calls to the function, in milliseconds
 *   (1/1000ths of a second)
 * @function: function to call
 * @data: data to pass to @function
 * @notify: (nullable): function to call when the timeout is removed, or %NULL
 *
 * Sets a function to be called at regular intervals, with the given
 * priority.  The function is called repeatedly until it returns
 * %FALSE, at which point the timeout is automatically destroyed and
 * the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to 'catch up' time lost in delays).
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle the return value and memory management of @data.
 *
 * This internally creates a main loop source using g_timeout_source_new()
 * and attaches it to the global #xmain_context_t using xsource_attach(), so
 * the callback will be invoked in whichever thread is running that main
 * context. You can do these steps manually if you need greater control or to
 * use a custom main context.
 *
 * The interval given is in terms of monotonic time, not wall clock time.
 * See g_get_monotonic_time().
 *
 * Returns: the ID (greater than 0) of the event source.
 **/
xuint_t
g_timeout_add_full (xint_t           priority,
		    xuint_t          interval,
		    xsource_func_t    function,
		    xpointer_t       data,
		    xdestroy_notify_t notify)
{
  xsource_t *source;
  xuint_t id;

  g_return_val_if_fail (function != NULL, 0);

  source = g_timeout_source_new (interval);

  if (priority != G_PRIORITY_DEFAULT)
    xsource_set_priority (source, priority);

  xsource_set_callback (source, function, data, notify);
  id = xsource_attach (source, NULL);

  TRACE (XPL_TIMEOUT_ADD (source, xmain_context_default (), id, priority, interval, function, data));

  xsource_unref (source);

  return id;
}

/**
 * g_timeout_add:
 * @interval: the time between calls to the function, in milliseconds
 *    (1/1000ths of a second)
 * @function: function to call
 * @data: data to pass to @function
 *
 * Sets a function to be called at regular intervals, with the default
 * priority, %G_PRIORITY_DEFAULT.
 *
 * The given @function is called repeatedly until it returns %G_SOURCE_REMOVE
 * or %FALSE, at which point the timeout is automatically destroyed and the
 * function will not be called again. The first call to the function will be
 * at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to 'catch up' time lost in delays).
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle the return value and memory management of @data.
 *
 * If you want to have a timer in the "seconds" range and do not care
 * about the exact time of the first call of the timer, use the
 * g_timeout_add_seconds() function; this function allows for more
 * optimizations and more efficient system power usage.
 *
 * This internally creates a main loop source using g_timeout_source_new()
 * and attaches it to the global #xmain_context_t using xsource_attach(), so
 * the callback will be invoked in whichever thread is running that main
 * context. You can do these steps manually if you need greater control or to
 * use a custom main context.
 *
 * It is safe to call this function from any thread.
 *
 * The interval given is in terms of monotonic time, not wall clock
 * time.  See g_get_monotonic_time().
 *
 * Returns: the ID (greater than 0) of the event source.
 **/
xuint_t
g_timeout_add (xuint32_t        interval,
	       xsource_func_t    function,
	       xpointer_t       data)
{
  return g_timeout_add_full (G_PRIORITY_DEFAULT,
			     interval, function, data, NULL);
}

/**
 * g_timeout_add_seconds_full: (rename-to g_timeout_add_seconds)
 * @priority: the priority of the timeout source. Typically this will be in
 *   the range between %G_PRIORITY_DEFAULT and %G_PRIORITY_HIGH.
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data: data to pass to @function
 * @notify: (nullable): function to call when the timeout is removed, or %NULL
 *
 * Sets a function to be called at regular intervals, with @priority.
 *
 * The function is called repeatedly until it returns %G_SOURCE_REMOVE
 * or %FALSE, at which point the timeout is automatically destroyed and
 * the function will not be called again.
 *
 * Unlike g_timeout_add(), this function operates at whole second granularity.
 * The initial starting point of the timer is determined by the implementation
 * and the implementation is expected to group multiple timers together so that
 * they fire all at the same time. To allow this grouping, the @interval to the
 * first timer is rounded and can deviate up to one second from the specified
 * interval. Subsequent timer iterations will generally run at the specified
 * interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given @interval
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle the return value and memory management of @data.
 *
 * If you want timing more precise than whole seconds, use g_timeout_add()
 * instead.
 *
 * The grouping of timers to fire at the same time results in a more power
 * and CPU efficient behavior so if your timer is in multiples of seconds
 * and you don't require the first timer exactly one second from now, the
 * use of g_timeout_add_seconds() is preferred over g_timeout_add().
 *
 * This internally creates a main loop source using
 * g_timeout_source_new_seconds() and attaches it to the main loop context
 * using xsource_attach(). You can do these steps manually if you need
 * greater control.
 *
 * It is safe to call this function from any thread.
 *
 * The interval given is in terms of monotonic time, not wall clock
 * time.  See g_get_monotonic_time().
 *
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.14
 **/
xuint_t
g_timeout_add_seconds_full (xint_t           priority,
                            xuint32_t        interval,
                            xsource_func_t    function,
                            xpointer_t       data,
                            xdestroy_notify_t notify)
{
  xsource_t *source;
  xuint_t id;

  g_return_val_if_fail (function != NULL, 0);

  source = g_timeout_source_new_seconds (interval);

  if (priority != G_PRIORITY_DEFAULT)
    xsource_set_priority (source, priority);

  xsource_set_callback (source, function, data, notify);
  id = xsource_attach (source, NULL);
  xsource_unref (source);

  return id;
}

/**
 * g_timeout_add_seconds:
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data: data to pass to @function
 *
 * Sets a function to be called at regular intervals with the default
 * priority, %G_PRIORITY_DEFAULT.
 *
 * The function is called repeatedly until it returns %G_SOURCE_REMOVE
 * or %FALSE, at which point the timeout is automatically destroyed
 * and the function will not be called again.
 *
 * This internally creates a main loop source using
 * g_timeout_source_new_seconds() and attaches it to the main loop context
 * using xsource_attach(). You can do these steps manually if you need
 * greater control. Also see g_timeout_add_seconds_full().
 *
 * It is safe to call this function from any thread.
 *
 * Note that the first call of the timer may not be precise for timeouts
 * of one second. If you need finer precision and have such a timeout,
 * you may want to use g_timeout_add() instead.
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle the return value and memory management of @data.
 *
 * The interval given is in terms of monotonic time, not wall clock
 * time.  See g_get_monotonic_time().
 *
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.14
 **/
xuint_t
g_timeout_add_seconds (xuint_t       interval,
                       xsource_func_t function,
                       xpointer_t    data)
{
  g_return_val_if_fail (function != NULL, 0);

  return g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, interval, function, data, NULL);
}

/* Child watch functions */

#ifdef G_OS_WIN32

static xboolean_t
g_child_watch_prepare (xsource_t *source,
		       xint_t    *timeout)
{
  *timeout = -1;
  return FALSE;
}

static xboolean_t
g_child_watch_check (xsource_t  *source)
{
  GChildWatchSource *child_watch_source;
  xboolean_t child_exited;

  child_watch_source = (GChildWatchSource *) source;

  child_exited = child_watch_source->poll.revents & G_IO_IN;

  if (child_exited)
    {
      DWORD child_status;

      /*
       * Note: We do _not_ check for the special value of STILL_ACTIVE
       * since we know that the process has exited and doing so runs into
       * problems if the child process "happens to return STILL_ACTIVE(259)"
       * as Microsoft's Platform SDK puts it.
       */
      if (!GetExitCodeProcess (child_watch_source->pid, &child_status))
        {
	  xchar_t *emsg = g_win32_error_message (GetLastError ());
	  g_warning (G_STRLOC ": GetExitCodeProcess() failed: %s", emsg);
	  g_free (emsg);

	  child_watch_source->child_status = -1;
	}
      else
	child_watch_source->child_status = child_status;
    }

  return child_exited;
}

static void
g_child_watch_finalize (xsource_t *source)
{
}

#else /* G_OS_WIN32 */

static void
wake_source (xsource_t *source)
{
  xmain_context_t *context;

  /* This should be thread-safe:
   *
   *  - if the source is currently being added to a context, that
   *    context will be woken up anyway
   *
   *  - if the source is currently being destroyed, we simply need not
   *    to crash:
   *
   *    - the memory for the source will remain valid until after the
   *      source finalize function was called (which would remove the
   *      source from the global list which we are currently holding the
   *      lock for)
   *
   *    - the xmain_context_t will either be NULL or point to a live
   *      xmain_context_t
   *
   *    - the xmain_context_t will remain valid since we hold the
   *      main_context_list lock
   *
   *  Since we are holding a lot of locks here, don't try to enter any
   *  more xmain_context_t functions for fear of dealock -- just hit the
   *  GWakeup and run.  Even if that's safe now, it could easily become
   *  unsafe with some very minor changes in the future, and signal
   *  handling is not the most well-tested codepath.
   */
  G_LOCK(main_context_list);
  context = source->context;
  if (context)
    g_wakeup_signal (context->wakeup);
  G_UNLOCK(main_context_list);
}

static void
dispatch_unix_signals_unlocked (void)
{
  xboolean_t pending[NSIG];
  xslist_t *node;
  xint_t i;

  /* clear this first in case another one arrives while we're processing */
  g_atomic_int_set (&any_unix_signal_pending, 0);

  /* We atomically test/clear the bit from the global array in case
   * other signals arrive while we are dispatching.
   *
   * We then can safely use our own array below without worrying about
   * races.
   */
  for (i = 0; i < NSIG; i++)
    {
      /* Be very careful with (the volatile) unix_signal_pending.
       *
       * We must ensure that it's not possible that we clear it without
       * handling the signal.  We therefore must ensure that our pending
       * array has a field set (ie: we will do something about the
       * signal) before we clear the item in unix_signal_pending.
       *
       * Note specifically: we must check _our_ array.
       */
      pending[i] = g_atomic_int_compare_and_exchange (&unix_signal_pending[i], 1, 0);
    }

  /* handle GChildWatchSource instances */
  if (pending[SIGCHLD])
    {
      /* The only way we can do this is to scan all of the children.
       *
       * The docs promise that we will not reap children that we are not
       * explicitly watching, so that ties our hands from calling
       * waitpid(-1).  We also can't use siginfo's si_pid field since if
       * multiple SIGCHLD arrive at the same time, one of them can be
       * dropped (since a given UNIX signal can only be pending once).
       */
      for (node = unix_child_watches; node; node = node->next)
        {
          GChildWatchSource *source = node->data;

          if (!g_atomic_int_get (&source->child_exited))
            {
              pid_t pid;
              do
                {
                  g_assert (source->pid > 0);

                  pid = waitpid (source->pid, &source->child_status, WNOHANG);
                  if (pid > 0)
                    {
                      g_atomic_int_set (&source->child_exited, TRUE);
                      wake_source ((xsource_t *) source);
                    }
                  else if (pid == -1 && errno == ECHILD)
                    {
                      g_warning ("GChildWatchSource: Exit status of a child process was requested but ECHILD was received by waitpid(). See the documentation of g_child_watch_source_new() for possible causes.");
                      source->child_status = 0;
                      g_atomic_int_set (&source->child_exited, TRUE);
                      wake_source ((xsource_t *) source);
                    }
                }
              while (pid == -1 && errno == EINTR);
            }
        }
    }

  /* handle GUnixSignalWatchSource instances */
  for (node = unix_signal_watches; node; node = node->next)
    {
      GUnixSignalWatchSource *source = node->data;

      if (pending[source->signum] &&
          g_atomic_int_compare_and_exchange (&source->pending, FALSE, TRUE))
        {
          wake_source ((xsource_t *) source);
        }
    }

}

static void
dispatch_unix_signals (void)
{
  G_LOCK(unix_signal_lock);
  dispatch_unix_signals_unlocked ();
  G_UNLOCK(unix_signal_lock);
}

static xboolean_t
g_child_watch_prepare (xsource_t *source,
		       xint_t    *timeout)
{
  GChildWatchSource *child_watch_source;

  child_watch_source = (GChildWatchSource *) source;

  return g_atomic_int_get (&child_watch_source->child_exited);
}

static xboolean_t
g_child_watch_check (xsource_t *source)
{
  GChildWatchSource *child_watch_source;

  child_watch_source = (GChildWatchSource *) source;

  return g_atomic_int_get (&child_watch_source->child_exited);
}

static xboolean_t
g_unix_signal_watch_prepare (xsource_t *source,
			     xint_t    *timeout)
{
  GUnixSignalWatchSource *unix_signal_source;

  unix_signal_source = (GUnixSignalWatchSource *) source;

  return g_atomic_int_get (&unix_signal_source->pending);
}

static xboolean_t
g_unix_signal_watch_check (xsource_t  *source)
{
  GUnixSignalWatchSource *unix_signal_source;

  unix_signal_source = (GUnixSignalWatchSource *) source;

  return g_atomic_int_get (&unix_signal_source->pending);
}

static xboolean_t
g_unix_signal_watch_dispatch (xsource_t    *source,
			      xsource_func_t callback,
			      xpointer_t    user_data)
{
  GUnixSignalWatchSource *unix_signal_source;
  xboolean_t again;

  unix_signal_source = (GUnixSignalWatchSource *) source;

  if (!callback)
    {
      g_warning ("Unix signal source dispatched without callback. "
		 "You must call xsource_set_callback().");
      return FALSE;
    }

  g_atomic_int_set (&unix_signal_source->pending, FALSE);

  again = (callback) (user_data);

  return again;
}

static void
ref_unix_signal_handler_unlocked (int signum)
{
  /* Ensure we have the worker context */
  g_get_worker_context ();
  unix_signal_refcount[signum]++;
  if (unix_signal_refcount[signum] == 1)
    {
      struct sigaction action;
      action.sa_handler = g_unix_signal_handler;
      sigemptyset (&action.sa_mask);
#ifdef SA_RESTART
      action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
#else
      action.sa_flags = SA_NOCLDSTOP;
#endif
      sigaction (signum, &action, NULL);
    }
}

static void
unref_unix_signal_handler_unlocked (int signum)
{
  unix_signal_refcount[signum]--;
  if (unix_signal_refcount[signum] == 0)
    {
      struct sigaction action;
      memset (&action, 0, sizeof (action));
      action.sa_handler = SIG_DFL;
      sigemptyset (&action.sa_mask);
      sigaction (signum, &action, NULL);
    }
}

/* Return a const string to avoid allocations. We lose precision in the case the
 * @signum is unrecognised, but that’ll do. */
static const xchar_t *
signum_to_string (int signum)
{
  /* See `man 0P signal.h` */
#define SIGNAL(s) \
    case (s): \
      return ("GUnixSignalSource: " #s);
  switch (signum)
    {
    /* These signals are guaranteed to exist by POSIX. */
    SIGNAL (SIGABRT)
    SIGNAL (SIGFPE)
    SIGNAL (SIGILL)
    SIGNAL (SIGINT)
    SIGNAL (SIGSEGV)
    SIGNAL (SIGTERM)
    /* Frustratingly, these are not, and hence for brevity the list is
     * incomplete. */
#ifdef SIGALRM
    SIGNAL (SIGALRM)
#endif
#ifdef SIGCHLD
    SIGNAL (SIGCHLD)
#endif
#ifdef SIGHUP
    SIGNAL (SIGHUP)
#endif
#ifdef SIGKILL
    SIGNAL (SIGKILL)
#endif
#ifdef SIGPIPE
    SIGNAL (SIGPIPE)
#endif
#ifdef SIGQUIT
    SIGNAL (SIGQUIT)
#endif
#ifdef SIGSTOP
    SIGNAL (SIGSTOP)
#endif
#ifdef SIGUSR1
    SIGNAL (SIGUSR1)
#endif
#ifdef SIGUSR2
    SIGNAL (SIGUSR2)
#endif
#ifdef SIGPOLL
    SIGNAL (SIGPOLL)
#endif
#ifdef SIGPROF
    SIGNAL (SIGPROF)
#endif
#ifdef SIGTRAP
    SIGNAL (SIGTRAP)
#endif
    default:
      return "GUnixSignalSource: Unrecognized signal";
    }
#undef SIGNAL
}

xsource_t *
_g_main_create_unix_signal_watch (int signum)
{
  xsource_t *source;
  GUnixSignalWatchSource *unix_signal_source;

  source = xsource_new (&g_unix_signal_funcs, sizeof (GUnixSignalWatchSource));
  unix_signal_source = (GUnixSignalWatchSource *) source;

  unix_signal_source->signum = signum;
  unix_signal_source->pending = FALSE;

  /* Set a default name on the source, just in case the caller does not. */
  xsource_set_static_name (source, signum_to_string (signum));

  G_LOCK (unix_signal_lock);
  ref_unix_signal_handler_unlocked (signum);
  unix_signal_watches = xslist_prepend (unix_signal_watches, unix_signal_source);
  dispatch_unix_signals_unlocked ();
  G_UNLOCK (unix_signal_lock);

  return source;
}

static void
g_unix_signal_watch_finalize (xsource_t    *source)
{
  GUnixSignalWatchSource *unix_signal_source;

  unix_signal_source = (GUnixSignalWatchSource *) source;

  G_LOCK (unix_signal_lock);
  unref_unix_signal_handler_unlocked (unix_signal_source->signum);
  unix_signal_watches = xslist_remove (unix_signal_watches, source);
  G_UNLOCK (unix_signal_lock);
}

static void
g_child_watch_finalize (xsource_t *source)
{
  G_LOCK (unix_signal_lock);
  unix_child_watches = xslist_remove (unix_child_watches, source);
  unref_unix_signal_handler_unlocked (SIGCHLD);
  G_UNLOCK (unix_signal_lock);
}

#endif /* G_OS_WIN32 */

static xboolean_t
g_child_watch_dispatch (xsource_t    *source,
			xsource_func_t callback,
			xpointer_t    user_data)
{
  GChildWatchSource *child_watch_source;
  GChildWatchFunc child_watch_callback = (GChildWatchFunc) callback;

  child_watch_source = (GChildWatchSource *) source;

  if (!callback)
    {
      g_warning ("Child watch source dispatched without callback. "
		 "You must call xsource_set_callback().");
      return FALSE;
    }

  (child_watch_callback) (child_watch_source->pid, child_watch_source->child_status, user_data);

  /* We never keep a child watch source around as the child is gone */
  return FALSE;
}

#ifndef G_OS_WIN32

static void
g_unix_signal_handler (int signum)
{
  xint_t saved_errno = errno;

#if defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
  g_atomic_int_set (&unix_signal_pending[signum], 1);
  g_atomic_int_set (&any_unix_signal_pending, 1);
#else
#warning "Can't use atomics in g_unix_signal_handler(): Unix signal handling will be racy"
  unix_signal_pending[signum] = 1;
  any_unix_signal_pending = 1;
#endif

  g_wakeup_signal (glib_worker_context->wakeup);

  errno = saved_errno;
}

#endif /* !G_OS_WIN32 */

/**
 * g_child_watch_source_new:
 * @pid: process to watch. On POSIX the positive pid of a child process. On
 * Windows a handle for a process (which doesn't have to be a child).
 *
 * Creates a new child_watch source.
 *
 * The source will not initially be associated with any #xmain_context_t
 * and must be added to one with xsource_attach() before it will be
 * executed.
 *
 * Note that child watch sources can only be used in conjunction with
 * `g_spawn...` when the %G_SPAWN_DO_NOT_REAP_CHILD flag is used.
 *
 * Note that on platforms where #xpid_t must be explicitly closed
 * (see g_spawn_close_pid()) @pid must not be closed while the
 * source is still active. Typically, you will want to call
 * g_spawn_close_pid() in the callback function for the source.
 *
 * On POSIX platforms, the following restrictions apply to this API
 * due to limitations in POSIX process interfaces:
 *
 * * @pid must be a child of this process
 * * @pid must be positive
 * * the application must not call `waitpid` with a non-positive
 *   first argument, for instance in another thread
 * * the application must not wait for @pid to exit by any other
 *   mechanism, including `waitpid(pid, ...)` or a second child-watch
 *   source for the same @pid
 * * the application must not ignore `SIGCHLD`
 *
 * If any of those conditions are not met, this and related APIs will
 * not work correctly. This can often be diagnosed via a GLib warning
 * stating that `ECHILD` was received by `waitpid`.
 *
 * Calling `waitpid` for specific processes other than @pid remains a
 * valid thing to do.
 *
 * Returns: the newly-created child watch source
 *
 * Since: 2.4
 **/
xsource_t *
g_child_watch_source_new (xpid_t pid)
{
  xsource_t *source;
  GChildWatchSource *child_watch_source;

#ifndef G_OS_WIN32
  g_return_val_if_fail (pid > 0, NULL);
#endif

  source = xsource_new (&g_child_watch_funcs, sizeof (GChildWatchSource));
  child_watch_source = (GChildWatchSource *)source;

  /* Set a default name on the source, just in case the caller does not. */
  xsource_set_static_name (source, "GChildWatchSource");

  child_watch_source->pid = pid;

#ifdef G_OS_WIN32
  child_watch_source->poll.fd = (gintptr) pid;
  child_watch_source->poll.events = G_IO_IN;

  xsource_add_poll (source, &child_watch_source->poll);
#else /* G_OS_WIN32 */
  G_LOCK (unix_signal_lock);
  ref_unix_signal_handler_unlocked (SIGCHLD);
  unix_child_watches = xslist_prepend (unix_child_watches, child_watch_source);
  if (waitpid (pid, &child_watch_source->child_status, WNOHANG) > 0)
    child_watch_source->child_exited = TRUE;
  G_UNLOCK (unix_signal_lock);
#endif /* G_OS_WIN32 */

  return source;
}

/**
 * g_child_watch_add_full: (rename-to g_child_watch_add)
 * @priority: the priority of the idle source. Typically this will be in the
 *   range between %G_PRIORITY_DEFAULT_IDLE and %G_PRIORITY_HIGH_IDLE.
 * @pid: process to watch. On POSIX the positive pid of a child process. On
 * Windows a handle for a process (which doesn't have to be a child).
 * @function: function to call
 * @data: data to pass to @function
 * @notify: (nullable): function to call when the idle is removed, or %NULL
 *
 * Sets a function to be called when the child indicated by @pid
 * exits, at the priority @priority.
 *
 * If you obtain @pid from g_spawn_async() or g_spawn_async_with_pipes()
 * you will need to pass %G_SPAWN_DO_NOT_REAP_CHILD as flag to
 * the spawn function for the child watching to work.
 *
 * In many programs, you will want to call g_spawn_check_wait_status()
 * in the callback to determine whether or not the child exited
 * successfully.
 *
 * Also, note that on platforms where #xpid_t must be explicitly closed
 * (see g_spawn_close_pid()) @pid must not be closed while the source
 * is still active.  Typically, you should invoke g_spawn_close_pid()
 * in the callback function for the source.
 *
 * GLib supports only a single callback per process id.
 * On POSIX platforms, the same restrictions mentioned for
 * g_child_watch_source_new() apply to this function.
 *
 * This internally creates a main loop source using
 * g_child_watch_source_new() and attaches it to the main loop context
 * using xsource_attach(). You can do these steps manually if you
 * need greater control.
 *
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.4
 **/
xuint_t
g_child_watch_add_full (xint_t            priority,
			xpid_t            pid,
			GChildWatchFunc function,
			xpointer_t        data,
			xdestroy_notify_t  notify)
{
  xsource_t *source;
  xuint_t id;

  g_return_val_if_fail (function != NULL, 0);
#ifndef G_OS_WIN32
  g_return_val_if_fail (pid > 0, 0);
#endif

  source = g_child_watch_source_new (pid);

  if (priority != G_PRIORITY_DEFAULT)
    xsource_set_priority (source, priority);

  xsource_set_callback (source, (xsource_func_t) function, data, notify);
  id = xsource_attach (source, NULL);
  xsource_unref (source);

  return id;
}

/**
 * g_child_watch_add:
 * @pid: process id to watch. On POSIX the positive pid of a child
 *   process. On Windows a handle for a process (which doesn't have
 *   to be a child).
 * @function: function to call
 * @data: data to pass to @function
 *
 * Sets a function to be called when the child indicated by @pid
 * exits, at a default priority, %G_PRIORITY_DEFAULT.
 *
 * If you obtain @pid from g_spawn_async() or g_spawn_async_with_pipes()
 * you will need to pass %G_SPAWN_DO_NOT_REAP_CHILD as flag to
 * the spawn function for the child watching to work.
 *
 * Note that on platforms where #xpid_t must be explicitly closed
 * (see g_spawn_close_pid()) @pid must not be closed while the
 * source is still active. Typically, you will want to call
 * g_spawn_close_pid() in the callback function for the source.
 *
 * GLib supports only a single callback per process id.
 * On POSIX platforms, the same restrictions mentioned for
 * g_child_watch_source_new() apply to this function.
 *
 * This internally creates a main loop source using
 * g_child_watch_source_new() and attaches it to the main loop context
 * using xsource_attach(). You can do these steps manually if you
 * need greater control.
 *
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.4
 **/
xuint_t
g_child_watch_add (xpid_t            pid,
		   GChildWatchFunc function,
		   xpointer_t        data)
{
  return g_child_watch_add_full (G_PRIORITY_DEFAULT, pid, function, data, NULL);
}


/* Idle functions */

static xboolean_t
g_idle_prepare  (xsource_t  *source,
		 xint_t     *timeout)
{
  *timeout = 0;

  return TRUE;
}

static xboolean_t
g_idle_check    (xsource_t  *source)
{
  return TRUE;
}

static xboolean_t
g_idle_dispatch (xsource_t    *source,
		 xsource_func_t callback,
		 xpointer_t    user_data)
{
  xboolean_t again;

  if (!callback)
    {
      g_warning ("Idle source dispatched without callback. "
		 "You must call xsource_set_callback().");
      return FALSE;
    }

  again = callback (user_data);

  TRACE (XPL_IDLE_DISPATCH (source, source->context, callback, user_data, again));

  return again;
}

/**
 * g_idle_source_new:
 *
 * Creates a new idle source.
 *
 * The source will not initially be associated with any #xmain_context_t
 * and must be added to one with xsource_attach() before it will be
 * executed. Note that the default priority for idle sources is
 * %G_PRIORITY_DEFAULT_IDLE, as compared to other sources which
 * have a default priority of %G_PRIORITY_DEFAULT.
 *
 * Returns: the newly-created idle source
 **/
xsource_t *
g_idle_source_new (void)
{
  xsource_t *source;

  source = xsource_new (&g_idle_funcs, sizeof (xsource_t));
  xsource_set_priority (source, G_PRIORITY_DEFAULT_IDLE);

  /* Set a default name on the source, just in case the caller does not. */
  xsource_set_static_name (source, "GIdleSource");

  return source;
}

/**
 * g_idle_add_full: (rename-to g_idle_add)
 * @priority: the priority of the idle source. Typically this will be in the
 *   range between %G_PRIORITY_DEFAULT_IDLE and %G_PRIORITY_HIGH_IDLE.
 * @function: function to call
 * @data: data to pass to @function
 * @notify: (nullable): function to call when the idle is removed, or %NULL
 *
 * Adds a function to be called whenever there are no higher priority
 * events pending.
 *
 * If the function returns %G_SOURCE_REMOVE or %FALSE it is automatically
 * removed from the list of event sources and will not be called again.
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle the return value and memory management of @data.
 *
 * This internally creates a main loop source using g_idle_source_new()
 * and attaches it to the global #xmain_context_t using xsource_attach(), so
 * the callback will be invoked in whichever thread is running that main
 * context. You can do these steps manually if you need greater control or to
 * use a custom main context.
 *
 * Returns: the ID (greater than 0) of the event source.
 **/
xuint_t
g_idle_add_full (xint_t           priority,
		 xsource_func_t    function,
		 xpointer_t       data,
		 xdestroy_notify_t notify)
{
  xsource_t *source;
  xuint_t id;

  g_return_val_if_fail (function != NULL, 0);

  source = g_idle_source_new ();

  if (priority != G_PRIORITY_DEFAULT_IDLE)
    xsource_set_priority (source, priority);

  xsource_set_callback (source, function, data, notify);
  id = xsource_attach (source, NULL);

  TRACE (XPL_IDLE_ADD (source, xmain_context_default (), id, priority, function, data));

  xsource_unref (source);

  return id;
}

/**
 * g_idle_add:
 * @function: function to call
 * @data: data to pass to @function.
 *
 * Adds a function to be called whenever there are no higher priority
 * events pending to the default main loop. The function is given the
 * default idle priority, %G_PRIORITY_DEFAULT_IDLE.  If the function
 * returns %FALSE it is automatically removed from the list of event
 * sources and will not be called again.
 *
 * See [memory management of sources][mainloop-memory-management] for details
 * on how to handle the return value and memory management of @data.
 *
 * This internally creates a main loop source using g_idle_source_new()
 * and attaches it to the global #xmain_context_t using xsource_attach(), so
 * the callback will be invoked in whichever thread is running that main
 * context. You can do these steps manually if you need greater control or to
 * use a custom main context.
 *
 * Returns: the ID (greater than 0) of the event source.
 **/
xuint_t
g_idle_add (xsource_func_t    function,
	    xpointer_t       data)
{
  return g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, function, data, NULL);
}

/**
 * g_idle_remove_by_data:
 * @data: the data for the idle source's callback.
 *
 * Removes the idle function with the given data.
 *
 * Returns: %TRUE if an idle source was found and removed.
 **/
xboolean_t
g_idle_remove_by_data (xpointer_t data)
{
  return xsource_remove_by_funcs_user_data (&g_idle_funcs, data);
}

/**
 * xmain_context_invoke:
 * @context: (nullable): a #xmain_context_t, or %NULL
 * @function: function to call
 * @data: data to pass to @function
 *
 * Invokes a function in such a way that @context is owned during the
 * invocation of @function.
 *
 * If @context is %NULL then the global default main context — as
 * returned by xmain_context_default() — is used.
 *
 * If @context is owned by the current thread, @function is called
 * directly.  Otherwise, if @context is the thread-default main context
 * of the current thread and xmain_context_acquire() succeeds, then
 * @function is called and xmain_context_release() is called
 * afterwards.
 *
 * In any other case, an idle source is created to call @function and
 * that source is attached to @context (presumably to be run in another
 * thread).  The idle source is attached with %G_PRIORITY_DEFAULT
 * priority.  If you want a different priority, use
 * xmain_context_invoke_full().
 *
 * Note that, as with normal idle functions, @function should probably
 * return %FALSE.  If it returns %TRUE, it will be continuously run in a
 * loop (and may prevent this call from returning).
 *
 * Since: 2.28
 **/
void
xmain_context_invoke (xmain_context_t *context,
                       xsource_func_t   function,
                       xpointer_t      data)
{
  xmain_context_invoke_full (context,
                              G_PRIORITY_DEFAULT,
                              function, data, NULL);
}

/**
 * xmain_context_invoke_full:
 * @context: (nullable): a #xmain_context_t, or %NULL
 * @priority: the priority at which to run @function
 * @function: function to call
 * @data: data to pass to @function
 * @notify: (nullable): a function to call when @data is no longer in use, or %NULL.
 *
 * Invokes a function in such a way that @context is owned during the
 * invocation of @function.
 *
 * This function is the same as xmain_context_invoke() except that it
 * lets you specify the priority in case @function ends up being
 * scheduled as an idle and also lets you give a #xdestroy_notify_t for @data.
 *
 * @notify should not assume that it is called from any particular
 * thread or with any particular context acquired.
 *
 * Since: 2.28
 **/
void
xmain_context_invoke_full (xmain_context_t   *context,
                            xint_t            priority,
                            xsource_func_t     function,
                            xpointer_t        data,
                            xdestroy_notify_t  notify)
{
  g_return_if_fail (function != NULL);

  if (!context)
    context = xmain_context_default ();

  if (xmain_context_is_owner (context))
    {
      while (function (data));
      if (notify != NULL)
        notify (data);
    }

  else
    {
      xmain_context_t *thread_default;

      thread_default = xmain_context_get_thread_default ();

      if (!thread_default)
        thread_default = xmain_context_default ();

      if (thread_default == context && xmain_context_acquire (context))
        {
          while (function (data));

          xmain_context_release (context);

          if (notify != NULL)
            notify (data);
        }
      else
        {
          xsource_t *source;

          source = g_idle_source_new ();
          xsource_set_priority (source, priority);
          xsource_set_callback (source, function, data, notify);
          xsource_attach (source, context);
          xsource_unref (source);
        }
    }
}

static xpointer_t
glib_worker_main (xpointer_t data)
{
  while (TRUE)
    {
      xmain_context_iteration (glib_worker_context, TRUE);

#ifdef G_OS_UNIX
      if (g_atomic_int_get (&any_unix_signal_pending))
        dispatch_unix_signals ();
#endif
    }

  return NULL; /* worst GCC warning message ever... */
}

xmain_context_t *
g_get_worker_context (void)
{
  static xsize_t initialised;

  if (g_once_init_enter (&initialised))
    {
      /* mask all signals in the worker thread */
#ifdef G_OS_UNIX
      sigset_t prev_mask;
      sigset_t all;

      sigfillset (&all);
      pthread_sigmask (SIG_SETMASK, &all, &prev_mask);
#endif
      glib_worker_context = xmain_context_new ();
      xthread_new ("gmain", glib_worker_main, NULL);
#ifdef G_OS_UNIX
      pthread_sigmask (SIG_SETMASK, &prev_mask, NULL);
#endif
      g_once_init_leave (&initialised, TRUE);
    }

  return glib_worker_context;
}

/**
 * g_steal_fd:
 * @fd_ptr: (not optional) (inout): A pointer to a file descriptor
 *
 * Sets @fd_ptr to `-1`, returning the value that was there before.
 *
 * Conceptually, this transfers the ownership of the file descriptor
 * from the referenced variable to the caller of the function (i.e.
 * ‘steals’ the reference). This is very similar to g_steal_pointer(),
 * but for file descriptors.
 *
 * On POSIX platforms, this function is async-signal safe
 * (see [`signal(7)`](man:signal(7)) and
 * [`signal-safety(7)`](man:signal-safety(7))), making it safe to call from a
 * signal handler or a #GSpawnChildSetupFunc.
 *
 * Returns: the value that @fd_ptr previously had
 * Since: 2.70
 */
