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

#ifndef __G_ASYNCQUEUE_H__
#define __G_ASYNCQUEUE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gthread.h>

G_BEGIN_DECLS

typedef struct _GAsyncQueue xasync_queue_t;

XPL_AVAILABLE_IN_ALL
xasync_queue_t *g_async_queue_new                  (void);
XPL_AVAILABLE_IN_ALL
xasync_queue_t *g_async_queue_new_full             (xdestroy_notify_t item_free_func);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_lock                 (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_unlock               (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
xasync_queue_t *g_async_queue_ref                  (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_unref                (xasync_queue_t      *queue);

XPL_DEPRECATED_FOR(g_async_queue_ref)
void         g_async_queue_ref_unlocked         (xasync_queue_t      *queue);

XPL_DEPRECATED_FOR(g_async_queue_unref)
void         g_async_queue_unref_and_unlock     (xasync_queue_t      *queue);

XPL_AVAILABLE_IN_ALL
void         g_async_queue_push                 (xasync_queue_t      *queue,
                                                 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_push_unlocked        (xasync_queue_t      *queue,
                                                 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_push_sorted          (xasync_queue_t      *queue,
                                                 xpointer_t          data,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_push_sorted_unlocked (xasync_queue_t      *queue,
                                                 xpointer_t          data,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_pop                  (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_pop_unlocked         (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_try_pop              (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_try_pop_unlocked     (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_timeout_pop          (xasync_queue_t      *queue,
                                                 xuint64_t           timeout);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_timeout_pop_unlocked (xasync_queue_t      *queue,
                                                 xuint64_t           timeout);
XPL_AVAILABLE_IN_ALL
xint_t         g_async_queue_length               (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
xint_t         g_async_queue_length_unlocked      (xasync_queue_t      *queue);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_sort                 (xasync_queue_t      *queue,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_sort_unlocked        (xasync_queue_t      *queue,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);

XPL_AVAILABLE_IN_2_46
xboolean_t     g_async_queue_remove               (xasync_queue_t      *queue,
                                                 xpointer_t          item);
XPL_AVAILABLE_IN_2_46
xboolean_t     g_async_queue_remove_unlocked      (xasync_queue_t      *queue,
                                                 xpointer_t          item);
XPL_AVAILABLE_IN_2_46
void         g_async_queue_push_front           (xasync_queue_t      *queue,
                                                 xpointer_t          item);
XPL_AVAILABLE_IN_2_46
void         g_async_queue_push_front_unlocked  (xasync_queue_t      *queue,
                                                 xpointer_t          item);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_FOR(g_async_queue_timeout_pop)
xpointer_t     g_async_queue_timed_pop            (xasync_queue_t      *queue,
                                                 GTimeVal         *end_time);
XPL_DEPRECATED_FOR(g_async_queue_timeout_pop_unlocked)
xpointer_t     g_async_queue_timed_pop_unlocked   (xasync_queue_t      *queue,
                                                 GTimeVal         *end_time);
G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS

#endif /* __G_ASYNCQUEUE_H__ */
