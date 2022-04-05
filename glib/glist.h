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

#ifndef __G_LIST_H__
#define __G_LIST_H__

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
xlist_t*   g_list_alloc                   (void) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
void     g_list_free                    (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
void     g_list_free_1                  (xlist_t            *list);
#define  g_list_free1                   g_list_free_1
XPL_AVAILABLE_IN_ALL
void     g_list_free_full               (xlist_t            *list,
					 GDestroyNotify    free_func);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_append                  (xlist_t            *list,
					 xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_prepend                 (xlist_t            *list,
					 xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_insert                  (xlist_t            *list,
					 xpointer_t          data,
					 xint_t              position) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_insert_sorted           (xlist_t            *list,
					 xpointer_t          data,
					 GCompareFunc      func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_insert_sorted_with_data (xlist_t            *list,
					 xpointer_t          data,
					 GCompareDataFunc  func,
					 xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_insert_before           (xlist_t            *list,
					 xlist_t            *sibling,
					 xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_2_62
xlist_t*   g_list_insert_before_link      (xlist_t            *list,
					 xlist_t            *sibling,
					 xlist_t            *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_concat                  (xlist_t            *list1,
					 xlist_t            *list2) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_remove                  (xlist_t            *list,
					 gconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_remove_all              (xlist_t            *list,
					 gconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_remove_link             (xlist_t            *list,
					 xlist_t            *llink) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_delete_link             (xlist_t            *list,
					 xlist_t            *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_reverse                 (xlist_t            *list) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_copy                    (xlist_t            *list) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_2_34
xlist_t*   g_list_copy_deep               (xlist_t            *list,
					 GCopyFunc         func,
					 xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_nth                     (xlist_t            *list,
					 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_nth_prev                (xlist_t            *list,
					 xuint_t             n);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_find                    (xlist_t            *list,
					 gconstpointer     data);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_find_custom             (xlist_t            *list,
					 gconstpointer     data,
					 GCompareFunc      func);
XPL_AVAILABLE_IN_ALL
xint_t     g_list_position                (xlist_t            *list,
					 xlist_t            *llink);
XPL_AVAILABLE_IN_ALL
xint_t     g_list_index                   (xlist_t            *list,
					 gconstpointer     data);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_last                    (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_first                   (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
xuint_t    g_list_length                  (xlist_t            *list);
XPL_AVAILABLE_IN_ALL
void     g_list_foreach                 (xlist_t            *list,
					 GFunc             func,
					 xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_sort                    (xlist_t            *list,
					 GCompareFunc      compare_func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xlist_t*   g_list_sort_with_data          (xlist_t            *list,
					 GCompareDataFunc  compare_func,
					 xpointer_t          user_data)  G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xpointer_t g_list_nth_data                (xlist_t            *list,
					 xuint_t             n);

XPL_AVAILABLE_IN_2_64
void     g_clear_list                   (xlist_t           **list_ptr,
                                         GDestroyNotify    destroy);

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
          g_list_free_full (_list, (destroy)); \
        else                                   \
          g_list_free (_list);                 \
      }                                        \
  } G_STMT_END                                 \
  XPL_AVAILABLE_MACRO_IN_2_64


#define g_list_previous(list)	        ((list) ? (((xlist_t *)(list))->prev) : NULL)
#define g_list_next(list)	        ((list) ? (((xlist_t *)(list))->next) : NULL)

G_END_DECLS

#endif /* __G_LIST_H__ */
