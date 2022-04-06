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

#ifndef __G_REL_H__
#define __G_REL_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef struct _GRelation       GRelation;
typedef struct _GTuples         GTuples;

struct _GTuples
{
  xuint_t len;
};

/* GRelation
 *
 * Indexed Relations.  Imagine a really simple table in a
 * database.  Relations are not ordered.  This data type is meant for
 * maintaining a N-way mapping.
 *
 * g_relation_new() creates a relation with FIELDS fields
 *
 * g_relation_destroy() frees all resources
 * g_tuples_destroy() frees the result of g_relation_select()
 *
 * g_relation_index() indexes relation FIELD with the provided
 *   equality and hash functions.  this must be done before any
 *   calls to insert are made.
 *
 * g_relation_insert() inserts a new tuple.  you are expected to
 *   provide the right number of fields.
 *
 * g_relation_delete() deletes all relations with KEY in FIELD
 * g_relation_select() returns ...
 * g_relation_count() counts ...
 */

XPL_DEPRECATED_IN_2_26
GRelation* g_relation_new     (xint_t         fields);
XPL_DEPRECATED_IN_2_26
void       g_relation_destroy (GRelation   *relation);
XPL_DEPRECATED_IN_2_26
void       g_relation_index   (GRelation   *relation,
                               xint_t         field,
                               GHashFunc    hash_func,
                               GEqualFunc   key_equal_func);
XPL_DEPRECATED_IN_2_26
void       g_relation_insert  (GRelation   *relation,
                               ...);
XPL_DEPRECATED_IN_2_26
xint_t       g_relation_delete  (GRelation   *relation,
                               xconstpointer  key,
                               xint_t         field);
XPL_DEPRECATED_IN_2_26
GTuples*   g_relation_select  (GRelation   *relation,
                               xconstpointer  key,
                               xint_t         field);
XPL_DEPRECATED_IN_2_26
xint_t       g_relation_count   (GRelation   *relation,
                               xconstpointer  key,
                               xint_t         field);
XPL_DEPRECATED_IN_2_26
xboolean_t   g_relation_exists  (GRelation   *relation,
                               ...);
XPL_DEPRECATED_IN_2_26
void       g_relation_print   (GRelation   *relation);
XPL_DEPRECATED_IN_2_26
void       g_tuples_destroy   (GTuples     *tuples);
XPL_DEPRECATED_IN_2_26
xpointer_t   g_tuples_index     (GTuples     *tuples,
                               xint_t         index_,
                               xint_t         field);

G_END_DECLS

#endif /* __G_REL_H__ */
