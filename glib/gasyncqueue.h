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

typedef struct _GAsyncQueue GAsyncQueue;

XPL_AVAILABLE_IN_ALL
GAsyncQueue *g_async_queue_new                  (void);
XPL_AVAILABLE_IN_ALL
GAsyncQueue *g_async_queue_new_full             (GDestroyNotify item_free_func);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_lock                 (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_unlock               (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
GAsyncQueue *g_async_queue_ref                  (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_unref                (GAsyncQueue      *queue);

XPL_DEPRECATED_FOR(g_async_queue_ref)
void         g_async_queue_ref_unlocked         (GAsyncQueue      *queue);

XPL_DEPRECATED_FOR(g_async_queue_unref)
void         g_async_queue_unref_and_unlock     (GAsyncQueue      *queue);

XPL_AVAILABLE_IN_ALL
void         g_async_queue_push                 (GAsyncQueue      *queue,
                                                 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_push_unlocked        (GAsyncQueue      *queue,
                                                 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_push_sorted          (GAsyncQueue      *queue,
                                                 xpointer_t          data,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_push_sorted_unlocked (GAsyncQueue      *queue,
                                                 xpointer_t          data,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_pop                  (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_pop_unlocked         (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_try_pop              (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_try_pop_unlocked     (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_timeout_pop          (GAsyncQueue      *queue,
                                                 guint64           timeout);
XPL_AVAILABLE_IN_ALL
xpointer_t     g_async_queue_timeout_pop_unlocked (GAsyncQueue      *queue,
                                                 guint64           timeout);
XPL_AVAILABLE_IN_ALL
xint_t         g_async_queue_length               (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
xint_t         g_async_queue_length_unlocked      (GAsyncQueue      *queue);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_sort                 (GAsyncQueue      *queue,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
void         g_async_queue_sort_unlocked        (GAsyncQueue      *queue,
                                                 GCompareDataFunc  func,
                                                 xpointer_t          user_data);

XPL_AVAILABLE_IN_2_46
xboolean_t     g_async_queue_remove               (GAsyncQueue      *queue,
                                                 xpointer_t          item);
XPL_AVAILABLE_IN_2_46
xboolean_t     g_async_queue_remove_unlocked      (GAsyncQueue      *queue,
                                                 xpointer_t          item);
XPL_AVAILABLE_IN_2_46
void         g_async_queue_push_front           (GAsyncQueue      *queue,
                                                 xpointer_t          item);
XPL_AVAILABLE_IN_2_46
void         g_async_queue_push_front_unlocked  (GAsyncQueue      *queue,
                                                 xpointer_t          item);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_FOR(g_async_queue_timeout_pop)
xpointer_t     g_async_queue_timed_pop            (GAsyncQueue      *queue,
                                                 GTimeVal         *end_time);
XPL_DEPRECATED_FOR(g_async_queue_timeout_pop_unlocked)
xpointer_t     g_async_queue_timed_pop_unlocked   (GAsyncQueue      *queue,
                                                 GTimeVal         *end_time);
G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS

#endif /* __G_ASYNCQUEUE_H__ */
