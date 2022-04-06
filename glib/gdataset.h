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

#ifndef __G_DATASET_H__
#define __G_DATASET_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gquark.h>

G_BEGIN_DECLS

typedef struct _GData           GData;

typedef void            (*GDataForeachFunc)     (xquark         key_id,
                                                 xpointer_t       data,
                                                 xpointer_t       user_data);

/* Keyed Data List
 */
XPL_AVAILABLE_IN_ALL
void     g_datalist_init                (GData            **datalist);
XPL_AVAILABLE_IN_ALL
void     g_datalist_clear               (GData            **datalist);
XPL_AVAILABLE_IN_ALL
xpointer_t g_datalist_id_get_data         (GData            **datalist,
					 xquark             key_id);
XPL_AVAILABLE_IN_ALL
void     g_datalist_id_set_data_full    (GData            **datalist,
					 xquark             key_id,
					 xpointer_t           data,
					 xdestroy_notify_t     destroy_func);

typedef xpointer_t (*GDuplicateFunc) (xpointer_t data, xpointer_t user_data);

XPL_AVAILABLE_IN_2_34
xpointer_t g_datalist_id_dup_data         (GData            **datalist,
                                         xquark             key_id,
                                         GDuplicateFunc     dup_func,
					 xpointer_t           user_data);
XPL_AVAILABLE_IN_2_34
xboolean_t g_datalist_id_replace_data     (GData            **datalist,
                                         xquark             key_id,
                                         xpointer_t           oldval,
                                         xpointer_t           newval,
                                         xdestroy_notify_t     destroy,
					 xdestroy_notify_t    *old_destroy);

XPL_AVAILABLE_IN_ALL
xpointer_t g_datalist_id_remove_no_notify (GData            **datalist,
					 xquark             key_id);
XPL_AVAILABLE_IN_ALL
void     g_datalist_foreach             (GData            **datalist,
					 GDataForeachFunc   func,
					 xpointer_t           user_data);

/**
 * G_DATALIST_FLAGS_MASK:
 *
 * A bitmask that restricts the possible flags passed to
 * g_datalist_set_flags(). Passing a flags value where
 * flags & ~G_DATALIST_FLAGS_MASK != 0 is an error.
 */
#define G_DATALIST_FLAGS_MASK 0x3

XPL_AVAILABLE_IN_ALL
void     g_datalist_set_flags           (GData            **datalist,
					 xuint_t              flags);
XPL_AVAILABLE_IN_ALL
void     g_datalist_unset_flags         (GData            **datalist,
					 xuint_t              flags);
XPL_AVAILABLE_IN_ALL
xuint_t    g_datalist_get_flags           (GData            **datalist);

#define   g_datalist_id_set_data(dl, q, d)      \
     g_datalist_id_set_data_full ((dl), (q), (d), NULL)
#define   g_datalist_id_remove_data(dl, q)      \
     g_datalist_id_set_data ((dl), (q), NULL)
#define   g_datalist_set_data_full(dl, k, d, f) \
     g_datalist_id_set_data_full ((dl), g_quark_from_string (k), (d), (f))
#define   g_datalist_remove_no_notify(dl, k)    \
     g_datalist_id_remove_no_notify ((dl), g_quark_try_string (k))
#define   g_datalist_set_data(dl, k, d)         \
     g_datalist_set_data_full ((dl), (k), (d), NULL)
#define   g_datalist_remove_data(dl, k)         \
     g_datalist_id_set_data ((dl), g_quark_try_string (k), NULL)

/* Location Associated Keyed Data
 */
XPL_AVAILABLE_IN_ALL
void      g_dataset_destroy             (xconstpointer    dataset_location);
XPL_AVAILABLE_IN_ALL
xpointer_t  g_dataset_id_get_data         (xconstpointer    dataset_location,
                                         xquark           key_id);
XPL_AVAILABLE_IN_ALL
xpointer_t  g_datalist_get_data            (GData	 **datalist,
					  const xchar_t *key);
XPL_AVAILABLE_IN_ALL
void      g_dataset_id_set_data_full    (xconstpointer    dataset_location,
                                         xquark           key_id,
                                         xpointer_t         data,
                                         xdestroy_notify_t   destroy_func);
XPL_AVAILABLE_IN_ALL
xpointer_t  g_dataset_id_remove_no_notify (xconstpointer    dataset_location,
                                         xquark           key_id);
XPL_AVAILABLE_IN_ALL
void      g_dataset_foreach             (xconstpointer    dataset_location,
                                         GDataForeachFunc func,
                                         xpointer_t         user_data);
#define   g_dataset_id_set_data(l, k, d)        \
     g_dataset_id_set_data_full ((l), (k), (d), NULL)
#define   g_dataset_id_remove_data(l, k)        \
     g_dataset_id_set_data ((l), (k), NULL)
#define   g_dataset_get_data(l, k)              \
     (g_dataset_id_get_data ((l), g_quark_try_string (k)))
#define   g_dataset_set_data_full(l, k, d, f)   \
     g_dataset_id_set_data_full ((l), g_quark_from_string (k), (d), (f))
#define   g_dataset_remove_no_notify(l, k)      \
     g_dataset_id_remove_no_notify ((l), g_quark_try_string (k))
#define   g_dataset_set_data(l, k, d)           \
     g_dataset_set_data_full ((l), (k), (d), NULL)
#define   g_dataset_remove_data(l, k)           \
     g_dataset_id_set_data ((l), g_quark_try_string (k), NULL)

G_END_DECLS

#endif /* __G_DATASET_H__ */
