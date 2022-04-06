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

#ifndef __G_QUEUE_H__
#define __G_QUEUE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/glist.h>

G_BEGIN_DECLS

typedef struct _xqueue xqueue_t;

/**
 * xqueue_t:
 * @head: a pointer to the first element of the queue
 * @tail: a pointer to the last element of the queue
 * @length: the number of elements in the queue
 *
 * Contains the public fields of a
 * [Queue][glib-Double-ended-Queues].
 */
struct _xqueue
{
  xlist_t *head;
  xlist_t *tail;
  xuint_t  length;
};

/**
 * G_QUEUE_INIT:
 *
 * A statically-allocated #xqueue_t must be initialized with this
 * macro before it can be used. This macro can be used to initialize
 * a variable, but it cannot be assigned to a variable. In that case
 * you have to use g_queue_init().
 *
 * |[
 * xqueue_t my_queue = G_QUEUE_INIT;
 * ]|
 *
 * Since: 2.14
 */
#define G_QUEUE_INIT { NULL, NULL, 0 }

/* Queues
 */
XPL_AVAILABLE_IN_ALL
xqueue_t*  g_queue_new            (void);
XPL_AVAILABLE_IN_ALL
void     g_queue_free           (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
void     g_queue_free_full      (xqueue_t           *queue,
				xdestroy_notify_t    free_func);
XPL_AVAILABLE_IN_ALL
void     g_queue_init           (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
void     g_queue_clear          (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xboolean_t g_queue_is_empty       (xqueue_t           *queue);
XPL_AVAILABLE_IN_2_60
void     g_queue_clear_full     (xqueue_t           *queue,
                                 xdestroy_notify_t   free_func);
XPL_AVAILABLE_IN_ALL
xuint_t    g_queue_get_length     (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
void     g_queue_reverse        (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xqueue_t * g_queue_copy           (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
void     g_queue_foreach        (xqueue_t           *queue,
                                 GFunc             func,
                                 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xlist_t *  g_queue_find           (xqueue_t           *queue,
                                 xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xlist_t *  g_queue_find_custom    (xqueue_t           *queue,
                                 xconstpointer     data,
                                 GCompareFunc      func);
XPL_AVAILABLE_IN_ALL
void     g_queue_sort           (xqueue_t           *queue,
                                 GCompareDataFunc  compare_func,
                                 xpointer_t          user_data);

XPL_AVAILABLE_IN_ALL
void     g_queue_push_head      (xqueue_t           *queue,
                                 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void     g_queue_push_tail      (xqueue_t           *queue,
                                 xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void     g_queue_push_nth       (xqueue_t           *queue,
                                 xpointer_t          data,
                                 xint_t              n);
XPL_AVAILABLE_IN_ALL
xpointer_t g_queue_pop_head       (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t g_queue_pop_tail       (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t g_queue_pop_nth        (xqueue_t           *queue,
                                 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xpointer_t g_queue_peek_head      (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t g_queue_peek_tail      (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xpointer_t g_queue_peek_nth       (xqueue_t           *queue,
                                 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xint_t     g_queue_index          (xqueue_t           *queue,
                                 xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xboolean_t g_queue_remove         (xqueue_t           *queue,
                                 xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xuint_t    g_queue_remove_all     (xqueue_t           *queue,
                                 xconstpointer     data);
XPL_AVAILABLE_IN_ALL
void     g_queue_insert_before  (xqueue_t           *queue,
                                 xlist_t            *sibling,
                                 xpointer_t          data);
XPL_AVAILABLE_IN_2_62
void     g_queue_insert_before_link
                                (xqueue_t           *queue,
                                 xlist_t            *sibling,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
void     g_queue_insert_after   (xqueue_t           *queue,
                                 xlist_t            *sibling,
                                 xpointer_t          data);
XPL_AVAILABLE_IN_2_62
void     g_queue_insert_after_link
                                (xqueue_t           *queue,
                                 xlist_t            *sibling,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
void     g_queue_insert_sorted  (xqueue_t           *queue,
                                 xpointer_t          data,
                                 GCompareDataFunc  func,
                                 xpointer_t          user_data);

XPL_AVAILABLE_IN_ALL
void     g_queue_push_head_link (xqueue_t           *queue,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
void     g_queue_push_tail_link (xqueue_t           *queue,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
void     g_queue_push_nth_link  (xqueue_t           *queue,
                                 xint_t              n,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_queue_pop_head_link  (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_queue_pop_tail_link  (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_queue_pop_nth_link   (xqueue_t           *queue,
                                 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_queue_peek_head_link (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_queue_peek_tail_link (xqueue_t           *queue);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_queue_peek_nth_link  (xqueue_t           *queue,
                                 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xint_t     g_queue_link_index     (xqueue_t           *queue,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
void     g_queue_unlink         (xqueue_t           *queue,
                                 xlist_t            *link_);
XPL_AVAILABLE_IN_ALL
void     g_queue_delete_link    (xqueue_t           *queue,
                                 xlist_t            *link_);

G_END_DECLS

#endif /* __G_QUEUE_H__ */
