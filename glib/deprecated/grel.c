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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"

/* we know we are deprecated here, no need for warnings */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include "grel.h"

#include <glib/gmessages.h>
#include <glib/gtestutils.h>
#include <glib/gstring.h>
#include <glib/gslice.h>
#include <glib/ghash.h>

#include <stdarg.h>
#include <string.h>

/**
 * SECTION:relations
 * @title: Relations and Tuples
 * @short_description: tables of data which can be indexed on any
 *                     number of fields
 *
 * A #GRelation is a table of data which can be indexed on any number
 * of fields, rather like simple database tables. A #GRelation contains
 * a number of records, called tuples. Each record contains a number of
 * fields. Records are not ordered, so it is not possible to find the
 * record at a particular index.
 *
 * Note that #GRelation tables are currently limited to 2 fields.
 *
 * To create a GRelation, use g_relation_new().
 *
 * To specify which fields should be indexed, use g_relation_index().
 * Note that this must be called before any tuples are added to the
 * #GRelation.
 *
 * To add records to a #GRelation use g_relation_insert().
 *
 * To determine if a given record appears in a #GRelation, use
 * g_relation_exists(). Note that fields are compared directly, so
 * pointers must point to the exact same position (i.e. different
 * copies of the same string will not match.)
 *
 * To count the number of records which have a particular value in a
 * given field, use g_relation_count().
 *
 * To get all the records which have a particular value in a given
 * field, use g_relation_select(). To access fields of the resulting
 * records, use g_tuples_index(). To free the resulting records use
 * g_tuples_destroy().
 *
 * To delete all records which have a particular value in a given
 * field, use g_relation_delete().
 *
 * To destroy the #GRelation, use g_relation_destroy().
 *
 * To help debug #GRelation objects, use g_relation_print().
 *
 * GRelation has been marked as deprecated, since this API has never
 * been fully implemented, is not very actively maintained and rarely
 * used.
 **/

typedef struct _GRealTuples        GRealTuples;

/**
 * GRelation:
 *
 * The #GRelation struct is an opaque data structure to represent a
 * [Relation][glib-Relations-and-Tuples]. It should
 * only be accessed via the following functions.
 **/
struct _GRelation
{
  xint_t fields;
  xint_t current_field;

  xhashtable_t   *all_tuples;
  xhashtable_t  **hashed_tuple_tables;

  xint_t count;
};

/**
 * GTuples:
 * @len: the number of records that matched.
 *
 * The #GTuples struct is used to return records (or tuples) from the
 * #GRelation by g_relation_select(). It only contains one public
 * member - the number of records that matched. To access the matched
 * records, you must use g_tuples_index().
 **/
struct _GRealTuples
{
  xint_t      len;
  xint_t      width;
  xpointer_t *data;
};

static xboolean_t
tuple_equal_2 (xconstpointer v_a,
	       xconstpointer v_b)
{
  xpointer_t* a = (xpointer_t*) v_a;
  xpointer_t* b = (xpointer_t*) v_b;

  return a[0] == b[0] && a[1] == b[1];
}

static xuint_t
tuple_hash_2 (xconstpointer v_a)
{
#if XPL_SIZEOF_VOID_P > XPL_SIZEOF_LONG
  /* In practise this snippet has been written for 64-bit Windows
   * where ints are 32 bits, pointers 64 bits. More exotic platforms
   * need more tweaks.
   */
  xuint_t* a = (xuint_t*) v_a;

  return (a[0] ^ a[1] ^ a[2] ^ a[3]);
#else
  xpointer_t* a = (xpointer_t*) v_a;

  return (gulong)a[0] ^ (gulong)a[1];
#endif
}

static GHashFunc
tuple_hash (xint_t fields)
{
  switch (fields)
    {
    case 2:
      return tuple_hash_2;
    default:
      xerror ("no tuple hash for %d", fields);
    }

  return NULL;
}

static GEqualFunc
tuple_equal (xint_t fields)
{
  switch (fields)
    {
    case 2:
      return tuple_equal_2;
    default:
      xerror ("no tuple equal for %d", fields);
    }

  return NULL;
}

/**
 * g_relation_new:
 * @fields: the number of fields.
 *
 * Creates a new #GRelation with the given number of fields. Note that
 * currently the number of fields must be 2.
 *
 * Returns: a new #GRelation.
 *
 * Deprecated: 2.26: Rarely used API
 **/
GRelation*
g_relation_new (xint_t fields)
{
  GRelation* rel = g_new0 (GRelation, 1);

  rel->fields = fields;
  rel->all_tuples = xhash_table_new (tuple_hash (fields), tuple_equal (fields));
  rel->hashed_tuple_tables = g_new0 (xhashtable_t*, fields);

  return rel;
}

static void
relation_delete_value_tuple (xpointer_t tuple_key,
                             xpointer_t tuple_value,
                             xpointer_t user_data)
{
  GRelation *relation = user_data;
  xpointer_t *tuple = tuple_value;
  g_slice_free1 (relation->fields * sizeof (xpointer_t), tuple);
}

static void
g_relation_free_array (xpointer_t key, xpointer_t value, xpointer_t user_data)
{
  xhash_table_destroy ((xhashtable_t*) value);
}

/**
 * g_relation_destroy:
 * @relation: a #GRelation.
 *
 * Destroys the #GRelation, freeing all memory allocated. However, it
 * does not free memory allocated for the tuple data, so you should
 * free that first if appropriate.
 *
 * Deprecated: 2.26: Rarely used API
 **/
void
g_relation_destroy (GRelation *relation)
{
  xint_t i;

  if (relation)
    {
      for (i = 0; i < relation->fields; i += 1)
	{
	  if (relation->hashed_tuple_tables[i])
	    {
	      xhash_table_foreach (relation->hashed_tuple_tables[i], g_relation_free_array, NULL);
	      xhash_table_destroy (relation->hashed_tuple_tables[i]);
	    }
	}

      xhash_table_foreach (relation->all_tuples, relation_delete_value_tuple, relation);
      xhash_table_destroy (relation->all_tuples);

      g_free (relation->hashed_tuple_tables);
      g_free (relation);
    }
}

/**
 * g_relation_index:
 * @relation: a #GRelation.
 * @field: the field to index, counting from 0.
 * @hash_func: a function to produce a hash value from the field data.
 * @key_equal_func: a function to compare two values of the given field.
 *
 * Creates an index on the given field. Note that this must be called
 * before any records are added to the #GRelation.
 *
 * Deprecated: 2.26: Rarely used API
 **/
void
g_relation_index (GRelation   *relation,
		  xint_t         field,
		  GHashFunc    hash_func,
		  GEqualFunc   key_equal_func)
{
  g_return_if_fail (relation != NULL);

  g_return_if_fail (relation->count == 0 && relation->hashed_tuple_tables[field] == NULL);

  relation->hashed_tuple_tables[field] = xhash_table_new (hash_func, key_equal_func);
}

/**
 * g_relation_insert:
 * @relation: a #GRelation.
 * @...: the fields of the record to add. These must match the
 *       number of fields in the #GRelation, and of type #xpointer_t
 *       or #xconstpointer.
 *
 * Inserts a record into a #GRelation.
 *
 * Deprecated: 2.26: Rarely used API
 **/
void
g_relation_insert (GRelation   *relation,
		   ...)
{
  xpointer_t* tuple = g_slice_alloc (relation->fields * sizeof (xpointer_t));
  va_list args;
  xint_t i;

  va_start (args, relation);

  for (i = 0; i < relation->fields; i += 1)
    tuple[i] = va_arg (args, xpointer_t);

  va_end (args);

  xhash_table_insert (relation->all_tuples, tuple, tuple);

  relation->count += 1;

  for (i = 0; i < relation->fields; i += 1)
    {
      xhashtable_t *table;
      xpointer_t    key;
      xhashtable_t *per_key_table;

      table = relation->hashed_tuple_tables[i];

      if (table == NULL)
	continue;

      key = tuple[i];
      per_key_table = xhash_table_lookup (table, key);

      if (per_key_table == NULL)
	{
	  per_key_table = xhash_table_new (tuple_hash (relation->fields), tuple_equal (relation->fields));
	  xhash_table_insert (table, key, per_key_table);
	}

      xhash_table_insert (per_key_table, tuple, tuple);
    }
}

static void
g_relation_delete_tuple (xpointer_t tuple_key,
			 xpointer_t tuple_value,
			 xpointer_t user_data)
{
  xpointer_t      *tuple = (xpointer_t*) tuple_value;
  GRelation     *relation = (GRelation *) user_data;
  xint_t           j;

  g_assert (tuple_key == tuple_value);

  for (j = 0; j < relation->fields; j += 1)
    {
      xhashtable_t *one_table = relation->hashed_tuple_tables[j];
      xpointer_t    one_key;
      xhashtable_t *per_key_table;

      if (one_table == NULL)
	continue;

      if (j == relation->current_field)
	/* can't delete from the table we're foreaching in */
	continue;

      one_key = tuple[j];

      per_key_table = xhash_table_lookup (one_table, one_key);

      xhash_table_remove (per_key_table, tuple);
    }

  if (xhash_table_remove (relation->all_tuples, tuple))
    g_slice_free1 (relation->fields * sizeof (xpointer_t), tuple);

  relation->count -= 1;
}

/**
 * g_relation_delete:
 * @relation: a #GRelation.
 * @key: the value to compare with.
 * @field: the field of each record to match.
 *
 * Deletes any records from a #GRelation that have the given key value
 * in the given field.
 *
 * Returns: the number of records deleted.
 *
 * Deprecated: 2.26: Rarely used API
 **/
xint_t
g_relation_delete  (GRelation     *relation,
		    xconstpointer  key,
		    xint_t           field)
{
  xhashtable_t *table;
  xhashtable_t *key_table;
  xint_t        count;

  g_return_val_if_fail (relation != NULL, 0);

  table = relation->hashed_tuple_tables[field];
  count = relation->count;

  g_return_val_if_fail (table != NULL, 0);

  key_table = xhash_table_lookup (table, key);

  if (!key_table)
    return 0;

  relation->current_field = field;

  xhash_table_foreach (key_table, g_relation_delete_tuple, relation);

  xhash_table_remove (table, key);

  xhash_table_destroy (key_table);

  /* @@@ FIXME: Remove empty hash tables. */

  return count - relation->count;
}

static void
g_relation_select_tuple (xpointer_t tuple_key,
			 xpointer_t tuple_value,
			 xpointer_t user_data)
{
  xpointer_t    *tuple = (xpointer_t*) tuple_value;
  GRealTuples *tuples = (GRealTuples*) user_data;
  xint_t stride = sizeof (xpointer_t) * tuples->width;

  g_assert (tuple_key == tuple_value);

  memcpy (tuples->data + (tuples->len * tuples->width),
	  tuple,
	  stride);

  tuples->len += 1;
}

/**
 * g_relation_select:
 * @relation: a #GRelation.
 * @key: the value to compare with.
 * @field: the field of each record to match.
 *
 * Returns all of the tuples which have the given key in the given
 * field. Use g_tuples_index() to access the returned records. The
 * returned records should be freed with g_tuples_destroy().
 *
 * Returns: the records (tuples) that matched.
 *
 * Deprecated: 2.26: Rarely used API
 **/
GTuples*
g_relation_select (GRelation     *relation,
		   xconstpointer  key,
		   xint_t           field)
{
  xhashtable_t  *table;
  xhashtable_t  *key_table;
  GRealTuples *tuples;
  xint_t count;

  g_return_val_if_fail (relation != NULL, NULL);

  table = relation->hashed_tuple_tables[field];

  g_return_val_if_fail (table != NULL, NULL);

  tuples = g_new0 (GRealTuples, 1);
  key_table = xhash_table_lookup (table, key);

  if (!key_table)
    return (GTuples*)tuples;

  count = g_relation_count (relation, key, field);

  tuples->data = g_malloc (sizeof (xpointer_t) * relation->fields * count);
  tuples->width = relation->fields;

  xhash_table_foreach (key_table, g_relation_select_tuple, tuples);

  g_assert (count == tuples->len);

  return (GTuples*)tuples;
}

/**
 * g_relation_count:
 * @relation: a #GRelation.
 * @key: the value to compare with.
 * @field: the field of each record to match.
 *
 * Returns the number of tuples in a #GRelation that have the given
 * value in the given field.
 *
 * Returns: the number of matches.
 *
 * Deprecated: 2.26: Rarely used API
 **/
xint_t
g_relation_count (GRelation     *relation,
		  xconstpointer  key,
		  xint_t           field)
{
  xhashtable_t  *table;
  xhashtable_t  *key_table;

  g_return_val_if_fail (relation != NULL, 0);

  table = relation->hashed_tuple_tables[field];

  g_return_val_if_fail (table != NULL, 0);

  key_table = xhash_table_lookup (table, key);

  if (!key_table)
    return 0;

  return xhash_table_size (key_table);
}

/**
 * g_relation_exists:
 * @relation: a #GRelation.
 * @...: the fields of the record to compare. The number must match
 *       the number of fields in the #GRelation.
 *
 * Returns %TRUE if a record with the given values exists in a
 * #GRelation. Note that the values are compared directly, so that, for
 * example, two copies of the same string will not match.
 *
 * Returns: %TRUE if a record matches.
 *
 * Deprecated: 2.26: Rarely used API
 **/
xboolean_t
g_relation_exists (GRelation   *relation, ...)
{
  xpointer_t *tuple = g_slice_alloc (relation->fields * sizeof (xpointer_t));
  va_list args;
  xint_t i;
  xboolean_t result;

  va_start(args, relation);

  for (i = 0; i < relation->fields; i += 1)
    tuple[i] = va_arg(args, xpointer_t);

  va_end(args);

  result = xhash_table_lookup (relation->all_tuples, tuple) != NULL;

  g_slice_free1 (relation->fields * sizeof (xpointer_t), tuple);

  return result;
}

/**
 * g_tuples_destroy:
 * @tuples: the tuple data to free.
 *
 * Frees the records which were returned by g_relation_select(). This
 * should always be called after g_relation_select() when you are
 * finished with the records. The records are not removed from the
 * #GRelation.
 *
 * Deprecated: 2.26: Rarely used API
 **/
void
g_tuples_destroy (GTuples *tuples0)
{
  GRealTuples *tuples = (GRealTuples*) tuples0;

  if (tuples)
    {
      g_free (tuples->data);
      g_free (tuples);
    }
}

/**
 * g_tuples_index:
 * @tuples: the tuple data, returned by g_relation_select().
 * @index_: the index of the record.
 * @field: the field to return.
 *
 * Gets a field from the records returned by g_relation_select(). It
 * returns the given field of the record at the given index. The
 * returned value should not be changed.
 *
 * Returns: the field of the record.
 *
 * Deprecated: 2.26: Rarely used API
 **/
xpointer_t
g_tuples_index (GTuples     *tuples0,
		xint_t         index,
		xint_t         field)
{
  GRealTuples *tuples = (GRealTuples*) tuples0;

  g_return_val_if_fail (tuples0 != NULL, NULL);
  g_return_val_if_fail (field < tuples->width, NULL);

  return tuples->data[index * tuples->width + field];
}

/* Print
 */

static void
g_relation_print_one (xpointer_t tuple_key,
		      xpointer_t tuple_value,
		      xpointer_t user_data)
{
  xint_t i;
  xstring_t *xstring;
  GRelation* rel = (GRelation*) user_data;
  xpointer_t* tuples = (xpointer_t*) tuple_value;

  xstring = xstring_new ("[");

  for (i = 0; i < rel->fields; i += 1)
    {
      xstring_append_printf (xstring, "%p", tuples[i]);

      if (i < (rel->fields - 1))
	xstring_append (xstring, ",");
    }

  xstring_append (xstring, "]");
  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "%s", xstring->str);
  xstring_free (xstring, TRUE);
}

static void
g_relation_print_index (xpointer_t tuple_key,
			xpointer_t tuple_value,
			xpointer_t user_data)
{
  GRelation* rel = (GRelation*) user_data;
  xhashtable_t* table = (xhashtable_t*) tuple_value;

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "*** key %p", tuple_key);

  xhash_table_foreach (table,
			g_relation_print_one,
			rel);
}

/**
 * g_relation_print:
 * @relation: a #GRelation.
 *
 * Outputs information about all records in a #GRelation, as well as
 * the indexes. It is for debugging.
 *
 * Deprecated: 2.26: Rarely used API
 **/
void
g_relation_print (GRelation *relation)
{
  xint_t i;

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "*** all tuples (%d)", relation->count);

  xhash_table_foreach (relation->all_tuples,
			g_relation_print_one,
			relation);

  for (i = 0; i < relation->fields; i += 1)
    {
      if (relation->hashed_tuple_tables[i] == NULL)
	continue;

      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "*** index %d", i);

      xhash_table_foreach (relation->hashed_tuple_tables[i],
			    g_relation_print_index,
			    relation);
    }

}
