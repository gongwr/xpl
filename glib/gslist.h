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

typedef struct _GSList GSList;

struct _GSList
{
  xpointer_t data;
  GSList *next;
};

/* Singly linked lists
 */
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_alloc                   (void) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
void     g_slist_free                    (GSList           *list);
XPL_AVAILABLE_IN_ALL
void     g_slist_free_1                  (GSList           *list);
#define	 g_slist_free1		         g_slist_free_1
XPL_AVAILABLE_IN_ALL
void     g_slist_free_full               (GSList           *list,
					  GDestroyNotify    free_func);
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_append                  (GSList           *list,
					  xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_prepend                 (GSList           *list,
					  xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_insert                  (GSList           *list,
					  xpointer_t          data,
					  xint_t              position) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_insert_sorted           (GSList           *list,
					  xpointer_t          data,
					  GCompareFunc      func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_insert_sorted_with_data (GSList           *list,
					  xpointer_t          data,
					  GCompareDataFunc  func,
					  xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_insert_before           (GSList           *slist,
					  GSList           *sibling,
					  xpointer_t          data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_concat                  (GSList           *list1,
					  GSList           *list2) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_remove                  (GSList           *list,
					  gconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_remove_all              (GSList           *list,
					  gconstpointer     data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_remove_link             (GSList           *list,
					  GSList           *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_delete_link             (GSList           *list,
					  GSList           *link_) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_reverse                 (GSList           *list) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_copy                    (GSList           *list) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_2_34
GSList*  g_slist_copy_deep               (GSList            *list,
					  GCopyFunc         func,
					  xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_nth                     (GSList           *list,
					  xuint_t             n);
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_find                    (GSList           *list,
					  gconstpointer     data);
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_find_custom             (GSList           *list,
					  gconstpointer     data,
					  GCompareFunc      func);
XPL_AVAILABLE_IN_ALL
xint_t     g_slist_position                (GSList           *list,
					  GSList           *llink);
XPL_AVAILABLE_IN_ALL
xint_t     g_slist_index                   (GSList           *list,
					  gconstpointer     data);
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_last                    (GSList           *list);
XPL_AVAILABLE_IN_ALL
xuint_t    g_slist_length                  (GSList           *list);
XPL_AVAILABLE_IN_ALL
void     g_slist_foreach                 (GSList           *list,
					  GFunc             func,
					  xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_sort                    (GSList           *list,
					  GCompareFunc      compare_func) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
GSList*  g_slist_sort_with_data          (GSList           *list,
					  GCompareDataFunc  compare_func,
					  xpointer_t          user_data) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xpointer_t g_slist_nth_data                (GSList           *list,
					  xuint_t             n);

XPL_AVAILABLE_IN_2_64
void     g_clear_slist                   (GSList          **slist_ptr,
                                          GDestroyNotify    destroy);

#define  g_clear_slist(slist_ptr, destroy)       \
  G_STMT_START {                                 \
    GSList *_slist;                              \
                                                 \
    _slist = *(slist_ptr);                       \
    if (_slist)                                  \
      {                                          \
        *slist_ptr = NULL;                       \
                                                 \
        if ((destroy) != NULL)                   \
          g_slist_free_full (_slist, (destroy)); \
        else                                     \
          g_slist_free (_slist);                 \
      }                                          \
  } G_STMT_END                                   \
  XPL_AVAILABLE_MACRO_IN_2_64

#define  g_slist_next(slist)	         ((slist) ? (((GSList *)(slist))->next) : NULL)

G_END_DECLS

#endif /* __G_SLIST_H__ */
