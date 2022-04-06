/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_THREADPOOL_H__
#define __G_THREADPOOL_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gthread.h>

G_BEGIN_DECLS

typedef struct _GThreadPool GThreadPool;

/* Thread Pools
 */

struct _GThreadPool
{
  GFunc func;
  xpointer_t user_data;
  xboolean_t exclusive;
};

XPL_AVAILABLE_IN_ALL
GThreadPool *   xthread_pool_new               (GFunc            func,
                                                 xpointer_t         user_data,
                                                 xint_t             max_threads,
                                                 xboolean_t         exclusive,
                                                 xerror_t         **error);
XPL_AVAILABLE_IN_2_70
GThreadPool *   xthread_pool_new_full          (GFunc            func,
                                                 xpointer_t         user_data,
                                                 xdestroy_notify_t   item_free_func,
                                                 xint_t             max_threads,
                                                 xboolean_t         exclusive,
                                                 xerror_t         **error);
XPL_AVAILABLE_IN_ALL
void            xthread_pool_free              (GThreadPool     *pool,
                                                 xboolean_t         immediate,
                                                 xboolean_t         wait_);
XPL_AVAILABLE_IN_ALL
xboolean_t        xthread_pool_push              (GThreadPool     *pool,
                                                 xpointer_t         data,
                                                 xerror_t         **error);
XPL_AVAILABLE_IN_ALL
xuint_t           xthread_pool_unprocessed       (GThreadPool     *pool);
XPL_AVAILABLE_IN_ALL
void            xthread_pool_set_sort_function (GThreadPool      *pool,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_2_46
xboolean_t        xthread_pool_move_to_front     (GThreadPool      *pool,
                                                 xpointer_t          data);

XPL_AVAILABLE_IN_ALL
xboolean_t        xthread_pool_set_max_threads   (GThreadPool     *pool,
                                                 xint_t             max_threads,
                                                 xerror_t         **error);
XPL_AVAILABLE_IN_ALL
xint_t            xthread_pool_get_max_threads   (GThreadPool     *pool);
XPL_AVAILABLE_IN_ALL
xuint_t           xthread_pool_get_num_threads   (GThreadPool     *pool);

XPL_AVAILABLE_IN_ALL
void            xthread_pool_set_max_unused_threads (xint_t  max_threads);
XPL_AVAILABLE_IN_ALL
xint_t            xthread_pool_get_max_unused_threads (void);
XPL_AVAILABLE_IN_ALL
xuint_t           xthread_pool_get_num_unused_threads (void);
XPL_AVAILABLE_IN_ALL
void            xthread_pool_stop_unused_threads    (void);
XPL_AVAILABLE_IN_ALL
void            xthread_pool_set_max_idle_time      (xuint_t interval);
XPL_AVAILABLE_IN_ALL
xuint_t           xthread_pool_get_max_idle_time      (void);

G_END_DECLS

#endif /* __G_THREADPOOL_H__ */
