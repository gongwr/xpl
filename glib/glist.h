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

#ifndef __XLIST_H__
#define __XLIST_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gmem.h>
#include <glib/gnode.h>

G_BEGIN_DECLS

typedef struct _GList xlist_t;

struct _GList
{
  xpointer_t data;
  xlist_t *next;
  xlist_t *prev;
};

/* Doubly linked lists
 */
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_alloc                   (void) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
void     xlist_free                    (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
void     xlist_free_1                  (xlist_t            *list);
#define  xlist_free1                   xlist_free_1
XPL_AVAILABLE_IN_ALL
void     xlist_free_full               (xlist_t            *list,
					 xdestroy_notify_t    free_func);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_append                  (xlist_t            *list,
					 xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_prepend                 (xlist_t            *list,
					 xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_insert                  (xlist_t            *list,
					 xpointer_t          data,
					 xint_t              position) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_insert_sorted           (xlist_t            *list,
					 xpointer_t          data,
					 GCompareFunc      func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_insert_sorted_with_data (xlist_t            *list,
					 xpointer_t          data,
					 GCompareDataFunc  func,
					 xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_insert_before           (xlist_t            *list,
					 xlist_t            *sibling,
					 xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_2_62
xlist_t*   xlist_insert_before_link      (xlist_t            *list,
					 xlist_t            *sibling,
					 xlist_t            *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_concat                  (xlist_t            *list1,
					 xlist_t            *list2) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_remove                  (xlist_t            *list,
					 xconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_remove_all              (xlist_t            *list,
					 xconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_remove_link             (xlist_t            *list,
					 xlist_t            *llink) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_delete_link             (xlist_t            *list,
					 xlist_t            *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_reverse                 (xlist_t            *list) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_copy                    (xlist_t            *list) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_2_34
xlist_t*   xlist_copy_deep               (xlist_t            *list,
					 GCopyFunc         func,
					 xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_nth                     (xlist_t            *list,
					 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_nth_prev                (xlist_t            *list,
					 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_find                    (xlist_t            *list,
					 xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_find_custom             (xlist_t            *list,
					 xconstpointer     data,
					 GCompareFunc      func);
XPL_AVAILABLE_IN_ALL
xint_t     xlist_position                (xlist_t            *list,
					 xlist_t            *llink);
XPL_AVAILABLE_IN_ALL
xint_t     xlist_index                   (xlist_t            *list,
					 xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_last                    (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_first                   (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
xuint_t    xlist_length                  (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
void     xlist_foreach                 (xlist_t            *list,
					 GFunc             func,
					 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_sort                    (xlist_t            *list,
					 GCompareFunc      compare_func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   xlist_sort_with_data          (xlist_t            *list,
					 GCompareDataFunc  compare_func,
					 xpointer_t          user_data)  G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xpointer_t xlist_nth_data                (xlist_t            *list,
					 xuint_t             n);

XPL_AVAILABLE_IN_2_64
void     g_clear_list                   (xlist_t           **list_ptr,
                                         xdestroy_notify_t    destroy);

#define  g_clear_list(list_ptr, destroy)       \
  G_STMT_START {                               \
    xlist_t *_list;                              \
                                               \
    _list = *(list_ptr);                       \
    if (_list)                                 \
      {                                        \
        *list_ptr = NULL;                      \
                                               \
        if ((destroy) != NULL)                 \
          xlist_free_full (_list, (destroy)); \
        else                                   \
          xlist_free (_list);                 \
      }                                        \
  } G_STMT_END                                 \
  XPL_AVAILABLE_MACRO_IN_2_64


#define xlist_previous(list)	        ((list) ? (((xlist_t *)(list))->prev) : NULL)
#define xlist_next(list)	        ((list) ? (((xlist_t *)(list))->next) : NULL)

G_END_DECLS

#endif /* __XLIST_H__ */
