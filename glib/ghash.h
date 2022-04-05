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

typedef struct _GHashTable  GHashTable;

typedef xboolean_t  (*GHRFunc)  (xpointer_t  key,
                               xpointer_t  value,
                               xpointer_t  user_data);

typedef struct _GHashTableIter GHashTableIter;

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
GHashTable* g_hash_table_new               (GHashFunc       hash_func,
                                            GEqualFunc      key_equal_func);
XPL_AVAILABLE_IN_ALL
GHashTable* g_hash_table_new_full          (GHashFunc       hash_func,
                                            GEqualFunc      key_equal_func,
                                            GDestroyNotify  key_destroy_func,
                                            GDestroyNotify  value_destroy_func);
XPL_AVAILABLE_IN_2_72
GHashTable *g_hash_table_new_similar       (GHashTable     *other_hash_table);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_destroy           (GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_insert            (GHashTable     *hash_table,
                                            xpointer_t        key,
                                            xpointer_t        value);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_replace           (GHashTable     *hash_table,
                                            xpointer_t        key,
                                            xpointer_t        value);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_add               (GHashTable     *hash_table,
                                            xpointer_t        key);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_remove            (GHashTable     *hash_table,
                                            gconstpointer   key);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_remove_all        (GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_steal             (GHashTable     *hash_table,
                                            gconstpointer   key);
XPL_AVAILABLE_IN_2_58
xboolean_t    g_hash_table_steal_extended    (GHashTable     *hash_table,
                                            gconstpointer   lookup_key,
                                            xpointer_t       *stolen_key,
                                            xpointer_t       *stolen_value);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_steal_all         (GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
xpointer_t    g_hash_table_lookup            (GHashTable     *hash_table,
                                            gconstpointer   key);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_contains          (GHashTable     *hash_table,
                                            gconstpointer   key);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_lookup_extended   (GHashTable     *hash_table,
                                            gconstpointer   lookup_key,
                                            xpointer_t       *orig_key,
                                            xpointer_t       *value);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_foreach           (GHashTable     *hash_table,
                                            GHFunc          func,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t    g_hash_table_find              (GHashTable     *hash_table,
                                            GHRFunc         predicate,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xuint_t       g_hash_table_foreach_remove    (GHashTable     *hash_table,
                                            GHRFunc         func,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xuint_t       g_hash_table_foreach_steal     (GHashTable     *hash_table,
                                            GHRFunc         func,
                                            xpointer_t        user_data);
XPL_AVAILABLE_IN_ALL
xuint_t       g_hash_table_size              (GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
xlist_t *     g_hash_table_get_keys          (GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
xlist_t *     g_hash_table_get_values        (GHashTable     *hash_table);
XPL_AVAILABLE_IN_2_40
xpointer_t *  g_hash_table_get_keys_as_array (GHashTable     *hash_table,
                                            xuint_t          *length);

XPL_AVAILABLE_IN_ALL
void        g_hash_table_iter_init         (GHashTableIter *iter,
                                            GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
xboolean_t    g_hash_table_iter_next         (GHashTableIter *iter,
                                            xpointer_t       *key,
                                            xpointer_t       *value);
XPL_AVAILABLE_IN_ALL
GHashTable* g_hash_table_iter_get_hash_table (GHashTableIter *iter);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_iter_remove       (GHashTableIter *iter);
XPL_AVAILABLE_IN_2_30
void        g_hash_table_iter_replace      (GHashTableIter *iter,
                                            xpointer_t        value);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_iter_steal        (GHashTableIter *iter);

XPL_AVAILABLE_IN_ALL
GHashTable* g_hash_table_ref               (GHashTable     *hash_table);
XPL_AVAILABLE_IN_ALL
void        g_hash_table_unref             (GHashTable     *hash_table);

#define g_hash_table_freeze(hash_table) ((void)0) XPL_DEPRECATED_MACRO_IN_2_26
#define g_hash_table_thaw(hash_table) ((void)0) XPL_DEPRECATED_MACRO_IN_2_26

/* Hash Functions
 */
XPL_AVAILABLE_IN_ALL
xboolean_t g_str_equal    (gconstpointer  v1,
                         gconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_str_hash     (gconstpointer  v);

XPL_AVAILABLE_IN_ALL
xboolean_t g_int_equal    (gconstpointer  v1,
                         gconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_int_hash     (gconstpointer  v);

XPL_AVAILABLE_IN_ALL
xboolean_t g_int64_equal  (gconstpointer  v1,
                         gconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_int64_hash   (gconstpointer  v);

XPL_AVAILABLE_IN_ALL
xboolean_t g_double_equal (gconstpointer  v1,
                         gconstpointer  v2);
XPL_AVAILABLE_IN_ALL
xuint_t    g_double_hash  (gconstpointer  v);

XPL_AVAILABLE_IN_ALL
xuint_t    g_direct_hash  (gconstpointer  v) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t g_direct_equal (gconstpointer  v1,
                         gconstpointer  v2) G_GNUC_CONST;

G_END_DECLS

#endif /* __G_HASH_H__ */
