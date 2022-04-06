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

#ifndef __G_CACHE_H__
#define __G_CACHE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/glist.h>

G_BEGIN_DECLS

typedef struct _GCache          GCache XPL_DEPRECATED_TYPE_IN_2_26_FOR(xhashtable_t);

typedef xpointer_t        (*GCacheNewFunc)        (xpointer_t       key) XPL_DEPRECATED_TYPE_IN_2_26;
typedef xpointer_t        (*GCacheDupFunc)        (xpointer_t       value) XPL_DEPRECATED_TYPE_IN_2_26;
typedef void            (*GCacheDestroyFunc)    (xpointer_t       value) XPL_DEPRECATED_TYPE_IN_2_26;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* Caches
 */
XPL_DEPRECATED
GCache*  g_cache_new           (GCacheNewFunc      value_new_func,
                                GCacheDestroyFunc  value_destroy_func,
                                GCacheDupFunc      key_dup_func,
                                GCacheDestroyFunc  key_destroy_func,
                                GHashFunc          hash_key_func,
                                GHashFunc          hash_value_func,
                                GEqualFunc         key_equal_func);
XPL_DEPRECATED
void     g_cache_destroy       (GCache            *cache);
XPL_DEPRECATED
xpointer_t g_cache_insert        (GCache            *cache,
                                xpointer_t           key);
XPL_DEPRECATED
void     g_cache_remove        (GCache            *cache,
                                xconstpointer      value);
XPL_DEPRECATED
void     g_cache_key_foreach   (GCache            *cache,
                                GHFunc             func,
                                xpointer_t           user_data);
XPL_DEPRECATED
void     g_cache_value_foreach (GCache            *cache,
                                GHFunc             func,
                                xpointer_t           user_data);

G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS

#endif /* __G_CACHE_H__ */
