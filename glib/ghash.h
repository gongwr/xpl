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

#ifndef __G_HASH_H__
#define __G_HASH_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>
#include <glib/glist.h>

G_BEGIN_DECLS

typedef struct _GHashTable  xhashtable_t;

typedef xboolean_t  (*GHRFunc)  (xpointer_t  key,
                               xpointer_t  value,
                               xpointer_t  user_data);

typedef struct _GHashTableIter xhash_table_iter_t;

struct _GHashTableIter
{
  /*< private >*/
  xpointer_t      dummy1;
  xpointer_t      dummy2;
  xpointer_t      dummy3;
  int           dummy4;
  xboolean_t      dummy5;
  xpointer_t      dummy6;
};

XPL_AVAILABLE_IN_ALL
xhashtable_t* xhash_table_new               (GHashFunc       hash_func,
                                            GEqualFunc      key_equal_func);
XPL_AVAILABLE_IN_ALL
xhashtable_t* xhash_table_new_full          (GHashFunc       hash_func,
                                            GEqualFunc      key_equal_func,
                                            xdestroy_notify_t  key_destroy_func,
                                            xdestroy_notify_t  value_destroy_func);
XPL_AVAILABLE_IN_2_72
xhashtable_t *xhash_table_new_similar       (xhashtable_t     *other_hash_table);
XPL_AVAILABLE_IN_ALL
void        xhash_table_destroy           (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_insert            (xhashtable_t     *hash_table,
                                            xpointer_t        key,
                                            xpointer_t        value);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_replace           (xhashtable_t     *hash_table,
                                            xpointer_t        key,
                                            xpointer_t        value);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_add               (xhashtable_t     *hash_table,
                                            xpointer_t        key);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_remove            (xhashtable_t     *hash_table,
                                            xconstpointer   key);
XPL_AVAILABLE_IN_ALL
void        xhash_table_remove_all        (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_steal             (xhashtable_t     *hash_table,
                                            xconstpointer   key);
XPL_AVAILABLE_IN_2_58
xboolean_t    xhash_table_steal_extended    (xhashtable_t     *hash_table,
                                            xconstpointer   lookup_key,
                                            xpointer_t       *stolen_key,
                                            xpointer_t       *stolen_value);
XPL_AVAILABLE_IN_ALL
void        xhash_table_steal_all         (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
xpointer_t    xhash_table_lookup            (xhashtable_t     *hash_table,
                                            xconstpointer   key);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_contains          (xhashtable_t     *hash_table,
                                            xconstpointer   key);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_lookup_extended   (xhashtable_t     *hash_table,
                                            xconstpointer   lookup_key,
                                            xpointer_t       *orig_key,
                                            xpointer_t       *value);
XPL_AVAILABLE_IN_ALL
void        xhash_table_foreach           (xhashtable_t     *hash_table,
                                            GHFunc          func,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t    xhash_table_find              (xhashtable_t     *hash_table,
                                            GHRFunc         predicate,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xuint_t       xhash_table_foreach_remove    (xhashtable_t     *hash_table,
                                            GHRFunc         func,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xuint_t       xhash_table_foreach_steal     (xhashtable_t     *hash_table,
                                            GHRFunc         func,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xuint_t       xhash_table_size              (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
xlist_t *     xhash_table_get_keys          (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
xlist_t *     xhash_table_get_values        (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_2_40
xpointer_t *  xhash_table_get_keys_as_array (xhashtable_t     *hash_table,
                                            xuint_t          *length);

XPL_AVAILABLE_IN_ALL
void        xhash_table_iter_init         (xhash_table_iter_t *iter,
                                            xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
xboolean_t    xhash_table_iter_next         (xhash_table_iter_t *iter,
                                            xpointer_t       *key,
                                            xpointer_t       *value);
XPL_AVAILABLE_IN_ALL
xhashtable_t* xhash_table_iter_get_hash_table (xhash_table_iter_t *iter);
XPL_AVAILABLE_IN_ALL
void        xhash_table_iter_remove       (xhash_table_iter_t *iter);
XPL_AVAILABLE_IN_2_30
void        xhash_table_iter_replace      (xhash_table_iter_t *iter,
                                            xpointer_t        value);
XPL_AVAILABLE_IN_ALL
void        xhash_table_iter_steal        (xhash_table_iter_t *iter);

XPL_AVAILABLE_IN_ALL
xhashtable_t* xhash_table_ref               (xhashtable_t     *hash_table);
XPL_AVAILABLE_IN_ALL
void        xhash_table_unref             (xhashtable_t     *hash_table);

#define xhash_table_freeze(hash_table) ((void)0) XPL_DEPRECATED_MACRO_IN_2_26
#define xhash_table_thaw(hash_table) ((void)0) XPL_DEPRECATED_MACRO_IN_2_26

/* Hash Functions
 */
XPL_AVAILABLE_IN_ALL
xboolean_t xstr_equal    (xconstpointer  v1,
                         xconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    xstr_hash     (xconstpointer  v);

XPL_AVAILABLE_IN_ALL
xboolean_t g_int_equal    (xconstpointer  v1,
                         xconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_int_hash     (xconstpointer  v);

XPL_AVAILABLE_IN_ALL
xboolean_t g_int64_equal  (xconstpointer  v1,
                         xconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_int64_hash   (xconstpointer  v);

XPL_AVAILABLE_IN_ALL
xboolean_t g_double_equal (xconstpointer  v1,
                         xconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_double_hash  (xconstpointer  v);

XPL_AVAILABLE_IN_ALL
xuint_t    g_direct_hash  (xconstpointer  v) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t g_direct_equal (xconstpointer  v1,
                         xconstpointer  v2) G_GNUC_CONST;

G_END_DECLS

#endif /* __G_HASH_H__ */
