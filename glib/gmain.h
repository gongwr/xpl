/* gmain.h - the GLib Main loop
 * Copyright (C) 1998-2000 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_MAIN_H__
#define __G_MAIN_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gpoll.h>
#include <glib/gslist.h>
#include <glib/gthread.h>

G_BEGIN_DECLS

typedef enum /*< flags >*/
{
  G_IO_IN	XPL_SYSDEF_POLLIN,
  G_IO_OUT	XPL_SYSDEF_POLLOUT,
  G_IO_PRI	XPL_SYSDEF_POLLPRI,
  G_IO_ERR	XPL_SYSDEF_POLLERR,
  G_IO_HUP	XPL_SYSDEF_POLLHUP,
  G_IO_NVAL	XPL_SYSDEF_POLLNVAL
} xio_condition_t;

/**
 * GMainContextFlags:
 * @XMAIN_CONTEXT_FLAGS_NONE: Default behaviour.
 * @XMAIN_CONTEXT_FLAGS_OWNERLESS_POLLING: Assume that polling for events will
 * free the thread to process other jobs. That's useful if you're using
 * `xmain_context_{prepare,query,check,dispatch}` to integrate xmain_context_t in
 * other event loops.
 *
 * Flags to pass to xmain_context_new_with_flags() which affect the behaviour
 * of a #xmain_context_t.
 *
 * Since: 2.72
 */
XPL_AVAILABLE_TYPE_IN_2_72
typedef enum /*< flags >*/
{
  XMAIN_CONTEXT_FLAGS_NONE = 0,
  XMAIN_CONTEXT_FLAGS_OWNERLESS_POLLING = 1
} GMainContextFlags;


/**
 * xmain_context_t:
 *
 * The `xmain_context_t` struct is an opaque data
 * type representing a set of sources to be handled in a main loop.
 */
typedef struct _GMainContext            xmain_context_t;

/**
 * xmain_loop_t:
 *
 * The `xmain_loop_t` struct is an opaque data type
 * representing the main event loop of a GLib or GTK+ application.
 */
typedef struct _GMainLoop               xmain_loop_t;

/**
 * xsource_t:
 *
 * The `xsource_t` struct is an opaque data type
 * representing an event source.
 */
typedef struct _GSource                 xsource_t;
typedef struct _GSourcePrivate          xsource_private_t;

/**
 * xsource_callback_funcs_t:
 * @ref: Called when a reference is added to the callback object
 * @unref: Called when a reference to the callback object is dropped
 * @get: Called to extract the callback function and data from the
 *     callback object.
 *
 * The `xsource_callback_funcs_t` struct contains
 * functions for managing callback objects.
 */
typedef struct _GSourceCallbackFuncs    xsource_callback_funcs_t;

/**
 * xsource_funcs_t:
 * @prepare: Called before all the file descriptors are polled. If the
 *     source can determine that it is ready here (without waiting for the
 *     results of the poll() call) it should return %TRUE. It can also return
 *     a @timeout_ value which should be the maximum timeout (in milliseconds)
 *     which should be passed to the poll() call. The actual timeout used will
 *     be -1 if all sources returned -1, or it will be the minimum of all
 *     the @timeout_ values returned which were >= 0.  Since 2.36 this may
 *     be %NULL, in which case the effect is as if the function always returns
 *     %FALSE with a timeout of -1.  If @prepare returns a
 *     timeout and the source also has a ready time set, then the
 *     lower of the two will be used.
 * @check: Called after all the file descriptors are polled. The source
 *     should return %TRUE if it is ready to be dispatched. Note that some
 *     time may have passed since the previous prepare function was called,
 *     so the source should be checked again here.  Since 2.36 this may
 *     be %NULL, in which case the effect is as if the function always returns
 *     %FALSE.
 * @dispatch: Called to dispatch the event source, after it has returned
 *     %TRUE in either its @prepare or its @check function, or if a ready time
 *     has been reached. The @dispatch function receives a callback function and
 *     user data. The callback function may be %NULL if the source was never
 *     connected to a callback using xsource_set_callback(). The @dispatch
 *     function should call the callback function with @user_data and whatever
 *     additional parameters are needed for this type of event source. The
 *     return value of the @dispatch function should be %G_SOURCE_REMOVE if the
 *     source should be removed or %G_SOURCE_CONTINUE to keep it.
 * @finalize: Called when the source is finalized. At this point, the source
 *     will have been destroyed, had its callback cleared, and have been removed
 *     from its #xmain_context_t, but it will still have its final reference count,
 *     so methods can be called on it from within this function.
 *
 * The `xsource_funcs_t` struct contains a table of
 * functions used to handle event sources in a generic manner.
 *
 * For idle sources, the prepare and check functions always return %TRUE
 * to indicate that the source is always ready to be processed. The prepare
 * function also returns a timeout value of 0 to ensure that the poll() call
 * doesn't block (since that would be time wasted which could have been spent
 * running the idle function).
 *
 * For timeout sources, the prepare and check functions both return %TRUE
 * if the timeout interval has expired. The prepare function also returns
 * a timeout value to ensure that the poll() call doesn't block too long
 * and miss the next timeout.
 *
 * For file descriptor sources, the prepare function typically returns %FALSE,
 * since it must wait until poll() has been called before it knows whether
 * any events need to be processed. It sets the returned timeout to -1 to
 * indicate that it doesn't mind how long the poll() call blocks. In the
 * check function, it tests the results of the poll() call to see if the
 * required condition has been met, and returns %TRUE if so.
 */
typedef struct _GSourceFuncs            xsource_funcs_t;

/**
 * xpid_t:
 *
 * A type which is used to hold a process identification.
 *
 * On UNIX, processes are identified by a process id (an integer),
 * while Windows uses process handles (which are pointers).
 *
 * xpid_t is used in GLib only for descendant processes spawned with
 * the g_spawn functions.
 */
/* defined in glibconfig.h */

/**
 * G_PID_FORMAT:
 *
 * A format specifier that can be used in printf()-style format strings
 * when printing a #xpid_t.
 *
 * Since: 2.50
 */
/* defined in glibconfig.h */

/**
 * xsource_func_t:
 * @user_data: data passed to the function, set when the source was
 *     created with one of the above functions
 *
 * Specifies the type of function passed to g_timeout_add(),
 * g_timeout_add_full(), g_idle_add(), and g_idle_add_full().
 *
 * When calling xsource_set_callback(), you may need to cast a function of a
 * different type to this type. Use G_SOURCE_FUNC() to avoid warnings about
 * incompatible function types.
 *
 * Returns: %FALSE if the source should be removed. %G_SOURCE_CONTINUE and
 * %G_SOURCE_REMOVE are more memorable names for the return value.
 */
typedef xboolean_t (*xsource_func_t)       (xpointer_t user_data);

/**
 * G_SOURCE_FUNC:
 * @f: a function pointer.
 *
 * Cast a function pointer to a #xsource_func_t, suppressing warnings from GCC 8
 * onwards with `-Wextra` or `-Wcast-function-type` enabled about the function
 * types being incompatible.
 *
 * For example, the correct type of callback for a source created by
 * g_child_watch_source_new() is #GChildWatchFunc, which accepts more arguments
 * than #xsource_func_t. Casting the function with `(xsource_func_t)` to call
 * xsource_set_callback() will trigger a warning, even though it will be cast
 * back to the correct type before it is called by the source.
 *
 * Since: 2.58
 */
#define G_SOURCE_FUNC(f) ((xsource_func_t) (void (*)(void)) (f)) XPL_AVAILABLE_MACRO_IN_2_58

/**
 * GChildWatchFunc:
 * @pid: the process id of the child process
 * @wait_status: Status information about the child process, encoded
 *               in a platform-specific manner
 * @user_data: user data passed to g_child_watch_add()
 *
 * Prototype of a #GChildWatchSource callback, called when a child
 * process has exited.
 *
 * To interpret @wait_status, see the documentation
 * for g_spawn_check_wait_status(). In particular,
 * on Unix platforms, note that it is usually not equal
 * to the integer passed to `exit()` or returned from `main()`.
 */
typedef void     (*GChildWatchFunc)   (xpid_t     pid,
                                       xint_t     wait_status,
                                       xpointer_t user_data);


/**
 * GSourceDisposeFunc:
 * @source: #xsource_t that is currently being disposed
 *
 * Dispose function for @source. See xsource_set_dispose_function() for
 * details.
 *
 * Since: 2.64
 */
XPL_AVAILABLE_TYPE_IN_2_64
typedef void (*GSourceDisposeFunc)       (xsource_t *source);

struct _GSource
{
  /*< private >*/
  xpointer_t callback_data;
  xsource_callback_funcs_t *callback_funcs;

  const xsource_funcs_t *source_funcs;
  xuint_t ref_count;

  xmain_context_t *context;

  xint_t priority;
  xuint_t flags;
  xuint_t source_id;

  xslist_t *poll_fds;

  xsource_t *prev;
  xsource_t *next;

  char    *name;

  xsource_private_t *priv;
};

struct _GSourceCallbackFuncs
{
  void (*ref)   (xpointer_t     cb_data);
  void (*unref) (xpointer_t     cb_data);
  void (*get)   (xpointer_t     cb_data,
                 xsource_t     *source,
                 xsource_func_t *func,
                 xpointer_t    *data);
};

/**
 * GSourceDummyMarshal:
 *
 * This is just a placeholder for #GClosureMarshal,
 * which cannot be used here for dependency reasons.
 */
typedef void (*GSourceDummyMarshal) (void);

struct _GSourceFuncs
{
  xboolean_t (*prepare)  (xsource_t    *source,
                        xint_t       *timeout_);/* Can be NULL */
  xboolean_t (*check)    (xsource_t    *source);/* Can be NULL */
  xboolean_t (*dispatch) (xsource_t    *source,
                        xsource_func_t callback,
                        xpointer_t    user_data);
  void     (*finalize) (xsource_t    *source); /* Can be NULL */

  /*< private >*/
  /* For use by xsource_set_closure */
  xsource_func_t     closure_callback;
  GSourceDummyMarshal closure_marshal; /* Really is of type GClosureMarshal */
};

/* Standard priorities */

/**
 * G_PRIORITY_HIGH:
 *
 * Use this for high priority event sources.
 *
 * It is not used within GLib or GTK+.
 */
#define G_PRIORITY_HIGH            -100

/**
 * G_PRIORITY_DEFAULT:
 *
 * Use this for default priority event sources.
 *
 * In GLib this priority is used when adding timeout functions
 * with g_timeout_add(). In GDK this priority is used for events
 * from the X server.
 */
#define G_PRIORITY_DEFAULT          0

/**
 * G_PRIORITY_HIGH_IDLE:
 *
 * Use this for high priority idle functions.
 *
 * GTK+ uses %G_PRIORITY_HIGH_IDLE + 10 for resizing operations,
 * and %G_PRIORITY_HIGH_IDLE + 20 for redrawing operations. (This is
 * done to ensure that any pending resizes are processed before any
 * pending redraws, so that widgets are not redrawn twice unnecessarily.)
 */
#define G_PRIORITY_HIGH_IDLE        100

/**
 * G_PRIORITY_DEFAULT_IDLE:
 *
 * Use this for default priority idle functions.
 *
 * In GLib this priority is used when adding idle functions with
 * g_idle_add().
 */
#define G_PRIORITY_DEFAULT_IDLE     200

/**
 * G_PRIORITY_LOW:
 *
 * Use this for very low priority background tasks.
 *
 * It is not used within GLib or GTK+.
 */
#define G_PRIORITY_LOW              300

/**
 * G_SOURCE_REMOVE:
 *
 * Use this macro as the return value of a #xsource_func_t to remove
 * the #xsource_t from the main loop.
 *
 * Since: 2.32
 */
#define G_SOURCE_REMOVE         FALSE

/**
 * G_SOURCE_CONTINUE:
 *
 * Use this macro as the return value of a #xsource_func_t to leave
 * the #xsource_t in the main loop.
 *
 * Since: 2.32
 */
#define G_SOURCE_CONTINUE       TRUE

/* xmain_context_t: */

XPL_AVAILABLE_IN_ALL
xmain_context_t *xmain_context_new       (void);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_2_72
xmain_context_t *xmain_context_new_with_flags (GMainContextFlags flags);
G_GNUC_END_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_ALL
xmain_context_t *xmain_context_ref       (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
void          xmain_context_unref     (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
xmain_context_t *xmain_context_default   (void);

XPL_AVAILABLE_IN_ALL
xboolean_t      xmain_context_iteration (xmain_context_t *context,
                                        xboolean_t      may_block);
XPL_AVAILABLE_IN_ALL
xboolean_t      xmain_context_pending   (xmain_context_t *context);

/* For implementation of legacy interfaces
 */
XPL_AVAILABLE_IN_ALL
xsource_t      *xmain_context_find_source_by_id              (xmain_context_t *context,
                                                             xuint_t         source_id);
XPL_AVAILABLE_IN_ALL
xsource_t      *xmain_context_find_source_by_user_data       (xmain_context_t *context,
                                                             xpointer_t      user_data);
XPL_AVAILABLE_IN_ALL
xsource_t      *xmain_context_find_source_by_funcs_user_data (xmain_context_t *context,
                                                             xsource_funcs_t *funcs,
                                                             xpointer_t      user_data);

/* Low level functions for implementing custom main loops.
 */
XPL_AVAILABLE_IN_ALL
void     xmain_context_wakeup  (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
xboolean_t xmain_context_acquire (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
void     xmain_context_release (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
xboolean_t xmain_context_is_owner (xmain_context_t *context);
XPL_DEPRECATED_IN_2_58_FOR(xmain_context_is_owner)
xboolean_t xmain_context_wait    (xmain_context_t *context,
                                 xcond_t        *cond,
                                 xmutex_t       *mutex);

XPL_AVAILABLE_IN_ALL
xboolean_t xmain_context_prepare  (xmain_context_t *context,
                                  xint_t         *priority);
XPL_AVAILABLE_IN_ALL
xint_t     xmain_context_query    (xmain_context_t *context,
                                  xint_t          max_priority,
                                  xint_t         *timeout_,
                                  xpollfd_t      *fds,
                                  xint_t          n_fds);
XPL_AVAILABLE_IN_ALL
xboolean_t     xmain_context_check    (xmain_context_t *context,
                                      xint_t          max_priority,
                                      xpollfd_t      *fds,
                                      xint_t          n_fds);
XPL_AVAILABLE_IN_ALL
void     xmain_context_dispatch (xmain_context_t *context);

XPL_AVAILABLE_IN_ALL
void     xmain_context_set_poll_func (xmain_context_t *context,
                                       GPollFunc     func);
XPL_AVAILABLE_IN_ALL
GPollFunc xmain_context_get_poll_func (xmain_context_t *context);

/* Low level functions for use by source implementations
 */
XPL_AVAILABLE_IN_ALL
void     xmain_context_add_poll    (xmain_context_t *context,
                                     xpollfd_t      *fd,
                                     xint_t          priority);
XPL_AVAILABLE_IN_ALL
void     xmain_context_remove_poll (xmain_context_t *context,
                                     xpollfd_t      *fd);

XPL_AVAILABLE_IN_ALL
xint_t     g_main_depth               (void);
XPL_AVAILABLE_IN_ALL
xsource_t *g_main_current_source      (void);

/* GMainContexts for other threads
 */
XPL_AVAILABLE_IN_ALL
void          xmain_context_push_thread_default (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
void          xmain_context_pop_thread_default  (xmain_context_t *context);
XPL_AVAILABLE_IN_ALL
xmain_context_t *xmain_context_get_thread_default  (void);
XPL_AVAILABLE_IN_ALL
xmain_context_t *xmain_context_ref_thread_default  (void);

/**
 * xmain_context_pusher_t:
 *
 * Opaque type. See xmain_context_pusher_new() for details.
 *
 * Since: 2.64
 */
typedef void xmain_context_pusher_t XPL_AVAILABLE_TYPE_IN_2_64;

/**
 * xmain_context_pusher_new:
 * @main_context: (transfer none): a main context to push
 *
 * Push @main_context as the new thread-default main context for the current
 * thread, using xmain_context_push_thread_default(), and return a new
 * #xmain_context_pusher_t. Pop with xmain_context_pusher_free(). Using
 * xmain_context_pop_thread_default() on @main_context while a
 * #xmain_context_pusher_t exists for it can lead to undefined behaviour.
 *
 * Using two #GMainContextPushers in the same scope is not allowed, as it leads
 * to an undefined pop order.
 *
 * This is intended to be used with x_autoptr().  Note that x_autoptr()
 * is only available when using GCC or clang, so the following example
 * will only work with those compilers:
 * |[
 * typedef struct
 * {
 *   ...
 *   xmain_context_t *context;
 *   ...
 * } xobject_t;
 *
 * static void
 * my_object_do_stuff (xobject_t *self)
 * {
 *   x_autoptr(xmain_context_pusher) pusher = xmain_context_pusher_new (self->context);
 *
 *   // Code with main context as the thread default here
 *
 *   if (cond)
 *     // No need to pop
 *     return;
 *
 *   // Optionally early pop
 *   g_clear_pointer (&pusher, xmain_context_pusher_free);
 *
 *   // Code with main context no longer the thread default here
 * }
 * ]|
 *
 * Returns: (transfer full): a #xmain_context_pusher_t
 * Since: 2.64
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_64
static inline xmain_context_pusher_t *
xmain_context_pusher_new (xmain_context_t *main_context)
{
  xmain_context_push_thread_default (main_context);
  return (xmain_context_pusher_t *) main_context;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xmain_context_pusher_free:
 * @pusher: (transfer full): a #xmain_context_pusher_t
 *
 * Pop @pusher’s main context as the thread default main context.
 * See xmain_context_pusher_new() for details.
 *
 * This will pop the #xmain_context_t as the current thread-default main context,
 * but will not call xmain_context_unref() on it.
 *
 * Since: 2.64
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_64
static inline void
xmain_context_pusher_free (xmain_context_pusher_t *pusher)
{
  xmain_context_pop_thread_default ((xmain_context_t *) pusher);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/* xmain_loop_t: */

XPL_AVAILABLE_IN_ALL
xmain_loop_t *xmain_loop_new        (xmain_context_t *context,
                                   xboolean_t      is_running);
XPL_AVAILABLE_IN_ALL
void       xmain_loop_run        (xmain_loop_t    *loop);
XPL_AVAILABLE_IN_ALL
void       xmain_loop_quit       (xmain_loop_t    *loop);
XPL_AVAILABLE_IN_ALL
xmain_loop_t *xmain_loop_ref        (xmain_loop_t    *loop);
XPL_AVAILABLE_IN_ALL
void       xmain_loop_unref      (xmain_loop_t    *loop);
XPL_AVAILABLE_IN_ALL
xboolean_t   xmain_loop_is_running (xmain_loop_t    *loop);
XPL_AVAILABLE_IN_ALL
xmain_context_t *xmain_loop_get_context (xmain_loop_t    *loop);

/* xsource_t: */

XPL_AVAILABLE_IN_ALL
xsource_t *xsource_new             (xsource_funcs_t   *source_funcs,
                                   xuint_t           struct_size);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_2_64
void     xsource_set_dispose_function (xsource_t            *source,
                                        GSourceDisposeFunc  dispose);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
xsource_t *xsource_ref             (xsource_t        *source);
XPL_AVAILABLE_IN_ALL
void     xsource_unref           (xsource_t        *source);

XPL_AVAILABLE_IN_ALL
xuint_t    xsource_attach          (xsource_t        *source,
                                   xmain_context_t   *context);
XPL_AVAILABLE_IN_ALL
void     xsource_destroy         (xsource_t        *source);

XPL_AVAILABLE_IN_ALL
void     xsource_set_priority    (xsource_t        *source,
                                   xint_t            priority);
XPL_AVAILABLE_IN_ALL
xint_t     xsource_get_priority    (xsource_t        *source);
XPL_AVAILABLE_IN_ALL
void     xsource_set_can_recurse (xsource_t        *source,
                                   xboolean_t        can_recurse);
XPL_AVAILABLE_IN_ALL
xboolean_t xsource_get_can_recurse (xsource_t        *source);
XPL_AVAILABLE_IN_ALL
xuint_t    xsource_get_id          (xsource_t        *source);

XPL_AVAILABLE_IN_ALL
xmain_context_t *xsource_get_context (xsource_t       *source);

XPL_AVAILABLE_IN_ALL
void     xsource_set_callback    (xsource_t        *source,
                                   xsource_func_t     func,
                                   xpointer_t        data,
                                   xdestroy_notify_t  notify);

XPL_AVAILABLE_IN_ALL
void     xsource_set_funcs       (xsource_t        *source,
                                   xsource_funcs_t   *funcs);
XPL_AVAILABLE_IN_ALL
xboolean_t xsource_is_destroyed    (xsource_t        *source);

XPL_AVAILABLE_IN_ALL
void                 xsource_set_name       (xsource_t        *source,
                                              const char     *name);
XPL_AVAILABLE_IN_2_70
void                 xsource_set_static_name (xsource_t        *source,
                                               const char     *name);
XPL_AVAILABLE_IN_ALL
const char *         xsource_get_name       (xsource_t        *source);
XPL_AVAILABLE_IN_ALL
void                 xsource_set_name_by_id (xuint_t           tag,
                                              const char     *name);

XPL_AVAILABLE_IN_2_36
void                 xsource_set_ready_time (xsource_t        *source,
                                              sint64_t          ready_time);
XPL_AVAILABLE_IN_2_36
sint64_t               xsource_get_ready_time (xsource_t        *source);

#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_2_36
xpointer_t             xsource_add_unix_fd    (xsource_t        *source,
                                              xint_t            fd,
                                              xio_condition_t    events);
XPL_AVAILABLE_IN_2_36
void                 xsource_modify_unix_fd (xsource_t        *source,
                                              xpointer_t        tag,
                                              xio_condition_t    new_events);
XPL_AVAILABLE_IN_2_36
void                 xsource_remove_unix_fd (xsource_t        *source,
                                              xpointer_t        tag);
XPL_AVAILABLE_IN_2_36
xio_condition_t         xsource_query_unix_fd  (xsource_t        *source,
                                              xpointer_t        tag);
#endif

/* Used to implement xsource_connect_closure and internally*/
XPL_AVAILABLE_IN_ALL
void xsource_set_callback_indirect (xsource_t              *source,
                                     xpointer_t              callback_data,
                                     xsource_callback_funcs_t *callback_funcs);

XPL_AVAILABLE_IN_ALL
void     xsource_add_poll            (xsource_t        *source,
				       xpollfd_t        *fd);
XPL_AVAILABLE_IN_ALL
void     xsource_remove_poll         (xsource_t        *source,
				       xpollfd_t        *fd);

XPL_AVAILABLE_IN_ALL
void     xsource_add_child_source    (xsource_t        *source,
				       xsource_t        *child_source);
XPL_AVAILABLE_IN_ALL
void     xsource_remove_child_source (xsource_t        *source,
				       xsource_t        *child_source);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_28_FOR(xsource_get_time)
void     xsource_get_current_time (xsource_t        *source,
                                    GTimeVal       *timeval);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
sint64_t   xsource_get_time         (xsource_t        *source);

 /* void xsource_connect_closure (xsource_t        *source,
                                  xclosure_t       *closure);
 */

/* Specific source types
 */
XPL_AVAILABLE_IN_ALL
xsource_t *g_idle_source_new        (void);
XPL_AVAILABLE_IN_ALL
xsource_t *g_child_watch_source_new (xpid_t pid);
XPL_AVAILABLE_IN_ALL
xsource_t *g_timeout_source_new     (xuint_t interval);
XPL_AVAILABLE_IN_ALL
xsource_t *g_timeout_source_new_seconds (xuint_t interval);

/* Miscellaneous functions
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_62_FOR(g_get_real_time)
void   g_get_current_time                 (GTimeVal       *result);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
sint64_t g_get_monotonic_time               (void);
XPL_AVAILABLE_IN_ALL
sint64_t g_get_real_time                    (void);


/* Source manipulation by ID */
XPL_AVAILABLE_IN_ALL
xboolean_t xsource_remove                     (xuint_t          tag);
XPL_AVAILABLE_IN_ALL
xboolean_t xsource_remove_by_user_data        (xpointer_t       user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xsource_remove_by_funcs_user_data  (xsource_funcs_t  *funcs,
                                              xpointer_t       user_data);

/**
 * GClearHandleFunc:
 * @handle_id: the handle ID to clear
 *
 * Specifies the type of function passed to g_clear_handle_id().
 * The implementation is expected to free the resource identified
 * by @handle_id; for instance, if @handle_id is a #xsource_t ID,
 * xsource_remove() can be used.
 *
 * Since: 2.56
 */
typedef void (* GClearHandleFunc) (xuint_t handle_id);

XPL_AVAILABLE_IN_2_56
void    g_clear_handle_id (xuint_t           *tag_ptr,
                           GClearHandleFunc clear_func);

#define g_clear_handle_id(tag_ptr, clear_func)             \
  G_STMT_START {                                           \
    G_STATIC_ASSERT (sizeof *(tag_ptr) == sizeof (xuint_t)); \
    xuint_t *_tag_ptr = (xuint_t *) (tag_ptr);                 \
    xuint_t _handle_id;                                      \
                                                           \
    _handle_id = *_tag_ptr;                                \
    if (_handle_id > 0)                                    \
      {                                                    \
        *_tag_ptr = 0;                                     \
        clear_func (_handle_id);                           \
      }                                                    \
  } G_STMT_END                                             \
  XPL_AVAILABLE_MACRO_IN_2_56

/* Idles, child watchers and timeouts */
XPL_AVAILABLE_IN_ALL
xuint_t    g_timeout_add_full         (xint_t            priority,
                                     xuint_t           interval,
                                     xsource_func_t     function,
                                     xpointer_t        data,
                                     xdestroy_notify_t  notify);
XPL_AVAILABLE_IN_ALL
xuint_t    g_timeout_add              (xuint_t           interval,
                                     xsource_func_t     function,
                                     xpointer_t        data);
XPL_AVAILABLE_IN_ALL
xuint_t    g_timeout_add_seconds_full (xint_t            priority,
                                     xuint_t           interval,
                                     xsource_func_t     function,
                                     xpointer_t        data,
                                     xdestroy_notify_t  notify);
XPL_AVAILABLE_IN_ALL
xuint_t    g_timeout_add_seconds      (xuint_t           interval,
                                     xsource_func_t     function,
                                     xpointer_t        data);
XPL_AVAILABLE_IN_ALL
xuint_t    g_child_watch_add_full     (xint_t            priority,
                                     xpid_t            pid,
                                     GChildWatchFunc function,
                                     xpointer_t        data,
                                     xdestroy_notify_t  notify);
XPL_AVAILABLE_IN_ALL
xuint_t    g_child_watch_add          (xpid_t            pid,
                                     GChildWatchFunc function,
                                     xpointer_t        data);
XPL_AVAILABLE_IN_ALL
xuint_t    g_idle_add                 (xsource_func_t     function,
                                     xpointer_t        data);
XPL_AVAILABLE_IN_ALL
xuint_t    g_idle_add_full            (xint_t            priority,
                                     xsource_func_t     function,
                                     xpointer_t        data,
                                     xdestroy_notify_t  notify);
XPL_AVAILABLE_IN_ALL
xboolean_t g_idle_remove_by_data      (xpointer_t        data);

XPL_AVAILABLE_IN_ALL
void     xmain_context_invoke_full (xmain_context_t   *context,
                                     xint_t            priority,
                                     xsource_func_t     function,
                                     xpointer_t        data,
                                     xdestroy_notify_t  notify);
XPL_AVAILABLE_IN_ALL
void     xmain_context_invoke      (xmain_context_t   *context,
                                     xsource_func_t     function,
                                     xpointer_t        data);

XPL_AVAILABLE_STATIC_INLINE_IN_2_70
static inline int
g_steal_fd (int *fd_ptr)
{
  int fd = *fd_ptr;
  *fd_ptr = -1;
  return fd;
}

/* Hook for xclosure_t / xsource_t integration. Don't touch */
XPL_VAR xsource_funcs_t g_timeout_funcs;
XPL_VAR xsource_funcs_t g_child_watch_funcs;
XPL_VAR xsource_funcs_t g_idle_funcs;
#ifdef G_OS_UNIX
XPL_VAR xsource_funcs_t g_unix_signal_funcs;
XPL_VAR xsource_funcs_t g_unix_fd_source_funcs;
#endif

G_END_DECLS

#endif /* __G_MAIN_H__ */
