/*
 * Copyright © 2010 Codethink Limited
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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __gvdb_builder_h__
#define __gvdb_builder_h__

#include <gio/gio.h>

typedef struct _GvdbItem GvdbItem;

G_GNUC_INTERNAL
xhashtable_t *            gvdb_hash_table_new                             (xhashtable_t    *parent,
                                                                         const xchar_t   *key);

G_GNUC_INTERNAL
GvdbItem *              gvdb_hash_table_insert                          (xhashtable_t    *table,
                                                                         const xchar_t   *key);
G_GNUC_INTERNAL
void                    gvdb_hash_table_insert_string                   (xhashtable_t    *table,
                                                                         const xchar_t   *key,
                                                                         const xchar_t   *value);

G_GNUC_INTERNAL
void                    gvdb_item_set_value                             (GvdbItem      *item,
                                                                         xvariant_t      *value);
G_GNUC_INTERNAL
void                    gvdb_item_set_hash_table                        (GvdbItem      *item,
                                                                         xhashtable_t    *table);
G_GNUC_INTERNAL
void                    gvdb_item_set_parent                            (GvdbItem      *item,
                                                                         GvdbItem      *parent);

G_GNUC_INTERNAL
xboolean_t                gvdb_table_write_contents                       (xhashtable_t     *table,
                                                                         const xchar_t    *filename,
                                                                         xboolean_t        byteswap,
                                                                         xerror_t        **error);
G_GNUC_INTERNAL
void                    gvdb_table_write_contents_async                 (xhashtable_t          *table,
                                                                         const xchar_t         *filename,
                                                                         xboolean_t             byteswap,
                                                                         xcancellable_t        *cancellable,
                                                                         xasync_ready_callback_t  callback,
                                                                         xpointer_t             user_data);
G_GNUC_INTERNAL
xboolean_t                gvdb_table_write_contents_finish                (xhashtable_t          *table,
                                                                         xasync_result_t        *result,
                                                                         xerror_t             **error);

#endif /* __gvdb_builder_h__ */
