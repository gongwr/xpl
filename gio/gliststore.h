/*
 * Copyright 2015 Lars Uebernickel
 * Copyright 2015 Ryan Lortie
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Lars Uebernickel <lars@uebernic.de>
 *     Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __XLIST_STORE_H__
#define __XLIST_STORE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_LIST_STORE (xlist_store_get_type ())
XPL_AVAILABLE_IN_2_44
G_DECLARE_FINAL_TYPE(xlist_store_t, xlist_store, G, LIST_STORE, xobject_t)

XPL_AVAILABLE_IN_2_44
xlist_store_t *            xlist_store_new                                (xtype_t       item_type);

XPL_AVAILABLE_IN_2_44
void                    xlist_store_insert                             (xlist_store_t *store,
                                                                         xuint_t       position,
                                                                         xpointer_t    item);

XPL_AVAILABLE_IN_2_44
xuint_t                   xlist_store_insert_sorted                      (xlist_store_t       *store,
                                                                         xpointer_t          item,
                                                                         GCompareDataFunc  compare_func,
                                                                         xpointer_t          user_data);

XPL_AVAILABLE_IN_2_46
void                   xlist_store_sort                                (xlist_store_t       *store,
                                                                         GCompareDataFunc  compare_func,
                                                                         xpointer_t          user_data);

XPL_AVAILABLE_IN_2_44
void                    xlist_store_append                             (xlist_store_t *store,
                                                                         xpointer_t    item);

XPL_AVAILABLE_IN_2_44
void                    xlist_store_remove                             (xlist_store_t *store,
                                                                         xuint_t       position);

XPL_AVAILABLE_IN_2_44
void                    xlist_store_remove_all                         (xlist_store_t *store);

XPL_AVAILABLE_IN_2_44
void                    xlist_store_splice                             (xlist_store_t *store,
                                                                         xuint_t       position,
                                                                         xuint_t       n_removals,
                                                                         xpointer_t   *additions,
                                                                         xuint_t       n_additions);

XPL_AVAILABLE_IN_2_64
xboolean_t                xlist_store_find                               (xlist_store_t *store,
                                                                         xpointer_t    item,
                                                                         xuint_t      *position);

XPL_AVAILABLE_IN_2_64
xboolean_t                xlist_store_find_with_equal_func               (xlist_store_t *store,
                                                                         xpointer_t    item,
                                                                         GEqualFunc  equal_func,
                                                                         xuint_t      *position);

G_END_DECLS

#endif /* __XLIST_STORE_H__ */
