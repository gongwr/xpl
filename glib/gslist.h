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

#ifndef __G_SLIST_H__
#define __G_SLIST_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gmem.h>
#include <glib/gnode.h>

G_BEGIN_DECLS

typedef struct _GSList xslist_t;

struct _GSList
{
  xpointer_t data;
  xslist_t *next;
};

/* Singly linked lists
 */
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_alloc                   (void) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
void     xslist_free                    (xslist_t           *list);
XPL_AVAILABLE_IN_ALL
void     xslist_free_1                  (xslist_t           *list);
#define	 xslist_free1		         xslist_free_1
XPL_AVAILABLE_IN_ALL
void     xslist_free_full               (xslist_t           *list,
					  xdestroy_notify_t    free_func);
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_append                  (xslist_t           *list,
					  xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_prepend                 (xslist_t           *list,
					  xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_insert                  (xslist_t           *list,
					  xpointer_t          data,
					  xint_t              position) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_insert_sorted           (xslist_t           *list,
					  xpointer_t          data,
					  GCompareFunc      func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_insert_sorted_with_data (xslist_t           *list,
					  xpointer_t          data,
					  GCompareDataFunc  func,
					  xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_insert_before           (xslist_t           *slist,
					  xslist_t           *sibling,
					  xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_concat                  (xslist_t           *list1,
					  xslist_t           *list2) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_remove                  (xslist_t           *list,
					  xconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_remove_all              (xslist_t           *list,
					  xconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_remove_link             (xslist_t           *list,
					  xslist_t           *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_delete_link             (xslist_t           *list,
					  xslist_t           *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_reverse                 (xslist_t           *list) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_copy                    (xslist_t           *list) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_2_34
xslist_t*  xslist_copy_deep               (xslist_t            *list,
					  GCopyFunc         func,
					  xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_nth                     (xslist_t           *list,
					  xuint_t             n);
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_find                    (xslist_t           *list,
					  xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_find_custom             (xslist_t           *list,
					  xconstpointer     data,
					  GCompareFunc      func);
XPL_AVAILABLE_IN_ALL
xint_t     xslist_position                (xslist_t           *list,
					  xslist_t           *llink);
XPL_AVAILABLE_IN_ALL
xint_t     xslist_index                   (xslist_t           *list,
					  xconstpointer     data);
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_last                    (xslist_t           *list);
XPL_AVAILABLE_IN_ALL
xuint_t    xslist_length                  (xslist_t           *list);
XPL_AVAILABLE_IN_ALL
void     xslist_foreach                 (xslist_t           *list,
					  GFunc             func,
					  xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_sort                    (xslist_t           *list,
					  GCompareFunc      compare_func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xslist_t*  xslist_sort_with_data          (xslist_t           *list,
					  GCompareDataFunc  compare_func,
					  xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xpointer_t xslist_nth_data                (xslist_t           *list,
					  xuint_t             n);

XPL_AVAILABLE_IN_2_64
void     g_clear_slist                   (xslist_t          **slist_ptr,
                                          xdestroy_notify_t    destroy);

#define  g_clear_slist(slist_ptr, destroy)       \
  G_STMT_START {                                 \
    xslist_t *_slist;                              \
                                                 \
    _slist = *(slist_ptr);                       \
    if (_slist)                                  \
      {                                          \
        *slist_ptr = NULL;                       \
                                                 \
        if ((destroy) != NULL)                   \
          xslist_free_full (_slist, (destroy)); \
        else                                     \
          xslist_free (_slist);                 \
      }                                          \
  } G_STMT_END                                   \
  XPL_AVAILABLE_MACRO_IN_2_64

#define  xslist_next(slist)	         ((slist) ? (((xslist_t *)(slist))->next) : NULL)

G_END_DECLS

#endif /* __G_SLIST_H__ */
