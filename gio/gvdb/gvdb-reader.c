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

#include "gvdb-reader.h"
#include "gvdb-format.h"

#include <string.h>

struct _GvdbTable {
  xbytes_t *bytes;

  const xchar_t *data;
  xsize_t size;

  xboolean_t byteswapped;
  xboolean_t trusted;

  const guint32_le *bloom_words;
  xuint32_t n_bloom_words;
  xuint_t bloom_shift;

  const guint32_le *hash_buckets;
  xuint32_t n_buckets;

  struct gvdb_hash_item *hash_items;
  xuint32_t n_hash_items;
};

static const xchar_t *
gvdb_table_item_get_key (GvdbTable                   *file,
                         const struct gvdb_hash_item *item,
                         xsize_t                       *size)
{
  xuint32_t start, end;

  start = guint32_from_le (item->key_start);
  *size = guint16_from_le (item->key_size);
  end = start + *size;

  if G_UNLIKELY (start > end || end > file->size)
    return NULL;

  return file->data + start;
}

static xconstpointer
gvdb_table_dereference (GvdbTable                 *file,
                        const struct gvdb_pointer *pointer,
                        xint_t                       alignment,
                        xsize_t                     *size)
{
  xuint32_t start, end;

  start = guint32_from_le (pointer->start);
  end = guint32_from_le (pointer->end);

  if G_UNLIKELY (start > end || end > file->size || start & (alignment - 1))
    return NULL;

  *size = end - start;

  return file->data + start;
}

static void
gvdb_table_setup_root (GvdbTable                 *file,
                       const struct gvdb_pointer *pointer)
{
  const struct gvdb_hash_header *header;
  xuint32_t n_bloom_words;
  xuint32_t n_buckets;
  xsize_t size;

  header = gvdb_table_dereference (file, pointer, 4, &size);

  if G_UNLIKELY (header == NULL || size < sizeof *header)
    return;

  size -= sizeof *header;

  n_bloom_words = guint32_from_le (header->n_bloom_words);
  n_buckets = guint32_from_le (header->n_buckets);
  n_bloom_words &= (1u << 27) - 1;

  if G_UNLIKELY (n_bloom_words * sizeof (guint32_le) > size)
    return;

  file->bloom_words = (xpointer_t) (header + 1);
  size -= n_bloom_words * sizeof (guint32_le);
  file->n_bloom_words = n_bloom_words;

  if G_UNLIKELY (n_buckets > G_MAXUINT / sizeof (guint32_le) ||
                 n_buckets * sizeof (guint32_le) > size)
    return;

  file->hash_buckets = file->bloom_words + file->n_bloom_words;
  size -= n_buckets * sizeof (guint32_le);
  file->n_buckets = n_buckets;

  if G_UNLIKELY (size % sizeof (struct gvdb_hash_item))
    return;

  file->hash_items = (xpointer_t) (file->hash_buckets + n_buckets);
  file->n_hash_items = size / sizeof (struct gvdb_hash_item);
}

/**
 * gvdb_table_new_from_bytes:
 * @bytes: the #xbytes_t with the data
 * @trusted: if the contents of @bytes are trusted
 * @error: %NULL, or a pointer to a %NULL #xerror_t
 *
 * Creates a new #GvdbTable from the contents of @bytes.
 *
 * This call can fail if the header contained in @bytes is invalid or if @bytes
 * is empty; if so, %XFILE_ERROR_INVAL will be returned.
 *
 * You should call gvdb_table_free() on the return result when you no
 * longer require it.
 *
 * Returns: a new #GvdbTable
 **/
GvdbTable *
gvdb_table_new_from_bytes (xbytes_t    *bytes,
                           xboolean_t   trusted,
                           xerror_t   **error)
{
  const struct gvdb_header *header;
  GvdbTable *file;

  file = g_slice_new0 (GvdbTable);
  file->bytes = xbytes_ref (bytes);
  file->data = xbytes_get_data (bytes, &file->size);
  file->trusted = trusted;

  if (file->size < sizeof (struct gvdb_header))
    goto invalid;

  header = (xpointer_t) file->data;

  if (header->signature[0] == GVDB_SIGNATURE0 &&
      header->signature[1] == GVDB_SIGNATURE1 &&
      guint32_from_le (header->version) == 0)
    file->byteswapped = FALSE;

  else if (header->signature[0] == GVDB_SWAPPED_SIGNATURE0 &&
           header->signature[1] == GVDB_SWAPPED_SIGNATURE1 &&
           guint32_from_le (header->version) == 0)
    file->byteswapped = TRUE;

  else
    goto invalid;

  gvdb_table_setup_root (file, &header->root);

  return file;

invalid:
  g_set_error_literal (error, XFILE_ERROR, XFILE_ERROR_INVAL, "invalid gvdb header");

  xbytes_unref (file->bytes);

  g_slice_free (GvdbTable, file);

  return NULL;
}

/**
 * gvdb_table_new:
 * @filename: a filename
 * @trusted: if the contents of @bytes are trusted
 * @error: %NULL, or a pointer to a %NULL #xerror_t
 *
 * Creates a new #GvdbTable using the #xmapped_file_t for @filename as the
 * #xbytes_t.
 *
 * This function will fail if the file cannot be opened.
 * In that case, the #xerror_t that is returned will be an error from
 * xmapped_file_new().
 *
 * An empty or corrupt file will result in %XFILE_ERROR_INVAL.
 *
 * Returns: a new #GvdbTable
 **/
GvdbTable *
gvdb_table_new (const xchar_t  *filename,
                xboolean_t      trusted,
                xerror_t      **error)
{
  xmapped_file_t *mapped;
  GvdbTable *table;
  xbytes_t *bytes;

  mapped = xmapped_file_new (filename, FALSE, error);
  if (!mapped)
    return NULL;

  bytes = xmapped_file_get_bytes (mapped);
  table = gvdb_table_new_from_bytes (bytes, trusted, error);
  xmapped_file_unref (mapped);
  xbytes_unref (bytes);

  g_prefix_error (error, "%s: ", filename);

  return table;
}

static xboolean_t
gvdb_table_bloom_filter (GvdbTable *file,
                          xuint32_t    hash_value)
{
  xuint32_t word, mask;

  if (file->n_bloom_words == 0)
    return TRUE;

  word = (hash_value / 32) % file->n_bloom_words;
  mask = 1 << (hash_value & 31);
  mask |= 1 << ((hash_value >> file->bloom_shift) & 31);

  return (guint32_from_le (file->bloom_words[word]) & mask) == mask;
}

static xboolean_t
gvdb_table_check_name (GvdbTable             *file,
                       struct gvdb_hash_item *item,
                       const xchar_t           *key,
                       xuint_t                  key_length)
{
  const xchar_t *this_key;
  xsize_t this_size;
  xuint32_t parent;

  this_key = gvdb_table_item_get_key (file, item, &this_size);

  if G_UNLIKELY (this_key == NULL || this_size > key_length)
    return FALSE;

  key_length -= this_size;

  if G_UNLIKELY (memcmp (this_key, key + key_length, this_size) != 0)
    return FALSE;

  parent = guint32_from_le (item->parent);
  if (key_length == 0 && parent == 0xffffffffu)
    return TRUE;

  if G_LIKELY (parent < file->n_hash_items && this_size > 0)
    return gvdb_table_check_name (file,
                                   &file->hash_items[parent],
                                   key, key_length);

  return FALSE;
}

static const struct gvdb_hash_item *
gvdb_table_lookup (GvdbTable   *file,
                   const xchar_t *key,
                   xchar_t        type)
{
  xuint32_t hash_value = 5381;
  xuint_t key_length;
  xuint32_t bucket;
  xuint32_t lastno;
  xuint32_t itemno;

  if G_UNLIKELY (file->n_buckets == 0 || file->n_hash_items == 0)
    return NULL;

  for (key_length = 0; key[key_length]; key_length++)
    hash_value = (hash_value * 33) + ((signed char *) key)[key_length];

  if (!gvdb_table_bloom_filter (file, hash_value))
    return NULL;

  bucket = hash_value % file->n_buckets;
  itemno = guint32_from_le (file->hash_buckets[bucket]);

  if (bucket == file->n_buckets - 1 ||
      (lastno = guint32_from_le(file->hash_buckets[bucket + 1])) > file->n_hash_items)
    lastno = file->n_hash_items;

  while G_LIKELY (itemno < lastno)
    {
      struct gvdb_hash_item *item = &file->hash_items[itemno];

      if (hash_value == guint32_from_le (item->hash_value))
        if G_LIKELY (gvdb_table_check_name (file, item, key, key_length))
          if G_LIKELY (item->type == type)
            return item;

      itemno++;
    }

  return NULL;
}

static xboolean_t
gvdb_table_list_from_item (GvdbTable                    *table,
                           const struct gvdb_hash_item  *item,
                           const guint32_le            **list,
                           xuint_t                        *length)
{
  xsize_t size;

  *list = gvdb_table_dereference (table, &item->value.pointer, 4, &size);

  if G_LIKELY (*list == NULL || size % 4)
    return FALSE;

  *length = size / 4;

  return TRUE;
}

/**
 * gvdb_table_get_names:
 * @table: a #GvdbTable
 * @length: (out) (optional): the number of items returned, or %NULL
 *
 * Gets a list of all names contained in @table.
 *
 * No call to gvdb_table_get_table(), gvdb_table_list() or
 * gvdb_table_get_value() will succeed unless it is for one of the
 * names returned by this function.
 *
 * Note that some names that are returned may still fail for all of the
 * above calls in the case of the corrupted file.  Note also that the
 * returned strings may not be utf8.
 *
 * Returns: (array length=length): a %NULL-terminated list of strings, of length @length
 **/
xchar_t **
gvdb_table_get_names (GvdbTable *table,
                      xsize_t     *length)
{
  xchar_t **names;
  xuint_t n_names;
  xuint_t filled;
  xuint_t total;
  xuint_t i;

  /* We generally proceed by iterating over the list of items in the
   * hash table (in order of appearance) recording them into an array.
   *
   * Each item has a parent item (except root items).  The parent item
   * forms part of the name of the item.  We could go fetching the
   * parent item chain at the point that we encounter each item but then
   * we would need to implement some sort of recursion along with checks
   * for self-referential items.
   *
   * Instead, we do a number of passes.  Each pass will build up one
   * level of names (starting from the root).  We continue to do passes
   * until no more items are left.  The first pass will only add root
   * items and each further pass will only add items whose direct parent
   * is an item added in the immediately previous pass.  It's also
   * possible that items get filled if they follow their parent within a
   * particular pass.
   *
   * At most we will have a number of passes equal to the depth of the
   * tree.  Self-referential items will never be filled in (since their
   * parent will have never been filled in).  We continue until we have
   * a pass that fills in no additional items.
   *
   * This takes an O(n) algorithm and turns it into O(n*m) where m is
   * the depth of the tree, but typically the tree won't be very
   * deep and the constant factor of this algorithm is lower (and the
   * complexity of coding it, as well).
   */

  n_names = table->n_hash_items;
  names = g_new0 (xchar_t *, n_names + 1);

  /* 'names' starts out all-NULL.  On each pass we record the number
   * of items changed from NULL to non-NULL in 'filled' so we know if we
   * should repeat the loop.  'total' counts the total number of items
   * filled.  If 'total' ends up equal to 'n_names' then we know that
   * 'names' has been completely filled.
   */

  total = 0;
  do
    {
      /* Loop until we have filled no more entries */
      filled = 0;

      for (i = 0; i < n_names; i++)
        {
          const struct gvdb_hash_item *item = &table->hash_items[i];
          const xchar_t *name;
          xsize_t name_length;
          xuint32_t parent;

          /* already got it on a previous pass */
          if (names[i] != NULL)
            continue;

          parent = guint32_from_le (item->parent);

          if (parent == 0xffffffffu)
            {
              /* it's a root item */
              name = gvdb_table_item_get_key (table, item, &name_length);

              if (name != NULL)
                {
                  names[i] = xstrndup (name, name_length);
                  filled++;
                }
            }

          else if (parent < n_names && names[parent] != NULL)
            {
              /* It's a non-root item whose parent was filled in already.
               *
               * Calculate the name of this item by combining it with
               * its parent name.
               */
              name = gvdb_table_item_get_key (table, item, &name_length);

              if (name != NULL)
                {
                  const xchar_t *parent_name = names[parent];
                  xsize_t parent_length;
                  xchar_t *fullname;

                  parent_length = strlen (parent_name);
                  fullname = g_malloc (parent_length + name_length + 1);
                  memcpy (fullname, parent_name, parent_length);
                  memcpy (fullname + parent_length, name, name_length);
                  fullname[parent_length + name_length] = '\0';
                  names[i] = fullname;
                  filled++;
                }
            }
        }

      total += filled;
    }
  while (filled && total < n_names);

  /* If the table was corrupted then 'names' may have holes in it.
   * Collapse those.
   */
  if G_UNLIKELY (total != n_names)
    {
      xptr_array_t *fixed_names;

      fixed_names = xptr_array_sized_new (n_names + 1  /* NULL terminator */);
      for (i = 0; i < n_names; i++)
        if (names[i] != NULL)
          xptr_array_add (fixed_names, names[i]);

      g_free (names);
      n_names = fixed_names->len;
      xptr_array_add (fixed_names, NULL);
      names = (xchar_t **) xptr_array_free (fixed_names, FALSE);
    }

  if (length)
    {
      G_STATIC_ASSERT (sizeof (*length) >= sizeof (n_names));
      *length = n_names;
    }

  return names;
}

/**
 * gvdb_table_list:
 * @file: a #GvdbTable
 * @key: a string
 *
 * List all of the keys that appear below @key.  The nesting of keys
 * within the hash file is defined by the program that created the hash
 * file.  One thing is constant: each item in the returned array can be
 * concatenated to @key to obtain the full name of that key.
 *
 * It is not possible to tell from this function if a given key is
 * itself a path, a value, or another hash table; you are expected to
 * know this for yourself.
 *
 * You should call xstrfreev() on the return result when you no longer
 * require it.
 *
 * Returns: a %NULL-terminated string array
 **/
xchar_t **
gvdb_table_list (GvdbTable   *file,
                 const xchar_t *key)
{
  const struct gvdb_hash_item *item;
  const guint32_le *list;
  xchar_t **strv;
  xuint_t length;
  xuint_t i;

  if ((item = gvdb_table_lookup (file, key, 'L')) == NULL)
    return NULL;

  if (!gvdb_table_list_from_item (file, item, &list, &length))
    return NULL;

  strv = g_new (xchar_t *, length + 1);
  for (i = 0; i < length; i++)
    {
      xuint32_t itemno = guint32_from_le (list[i]);

      if (itemno < file->n_hash_items)
        {
          const struct gvdb_hash_item *item;
          const xchar_t *string;
          xsize_t strsize;

          item = file->hash_items + itemno;

          string = gvdb_table_item_get_key (file, item, &strsize);

          if (string != NULL)
            strv[i] = xstrndup (string, strsize);
          else
            strv[i] = g_malloc0 (1);
        }
      else
        strv[i] = g_malloc0 (1);
    }

  strv[i] = NULL;

  return strv;
}

/**
 * gvdb_table_has_value:
 * @file: a #GvdbTable
 * @key: a string
 *
 * Checks for a value named @key in @file.
 *
 * Note: this function does not consider non-value nodes (other hash
 * tables, for example).
 *
 * Returns: %TRUE if @key is in the table
 **/
xboolean_t
gvdb_table_has_value (GvdbTable    *file,
                      const xchar_t  *key)
{
  static const struct gvdb_hash_item *item;
  xsize_t size;

  item = gvdb_table_lookup (file, key, 'v');

  if (item == NULL)
    return FALSE;

  return gvdb_table_dereference (file, &item->value.pointer, 8, &size) != NULL;
}

static xvariant_t *
gvdb_table_value_from_item (GvdbTable                   *table,
                            const struct gvdb_hash_item *item)
{
  xvariant_t *variant, *value;
  xconstpointer data;
  xbytes_t *bytes;
  xsize_t size;

  data = gvdb_table_dereference (table, &item->value.pointer, 8, &size);

  if G_UNLIKELY (data == NULL)
    return NULL;

  bytes = xbytes_new_from_bytes (table->bytes, ((xchar_t *) data) - table->data, size);
  variant = xvariant_new_from_bytes (G_VARIANT_TYPE_VARIANT, bytes, table->trusted);
  value = xvariant_get_variant (variant);
  xvariant_unref (variant);
  xbytes_unref (bytes);

  return value;
}

/**
 * gvdb_table_get_value:
 * @file: a #GvdbTable
 * @key: a string
 *
 * Looks up a value named @key in @file.
 *
 * If the value is not found then %NULL is returned.  Otherwise, a new
 * #xvariant_t instance is returned.  The #xvariant_t does not depend on the
 * continued existence of @file.
 *
 * You should call xvariant_unref() on the return result when you no
 * longer require it.
 *
 * Returns: a #xvariant_t, or %NULL
 **/
xvariant_t *
gvdb_table_get_value (GvdbTable    *file,
                      const xchar_t  *key)
{
  const struct gvdb_hash_item *item;
  xvariant_t *value;

  if ((item = gvdb_table_lookup (file, key, 'v')) == NULL)
    return NULL;

  value = gvdb_table_value_from_item (file, item);

  if (value && file->byteswapped)
    {
      xvariant_t *tmp;

      tmp = xvariant_byteswap (value);
      xvariant_unref (value);
      value = tmp;
    }

  return value;
}

/**
 * gvdb_table_get_raw_value:
 * @table: a #GvdbTable
 * @key: a string
 *
 * Looks up a value named @key in @file.
 *
 * This call is equivalent to gvdb_table_get_value() except that it
 * never byteswaps the value.
 *
 * Returns: a #xvariant_t, or %NULL
 **/
xvariant_t *
gvdb_table_get_raw_value (GvdbTable   *table,
                          const xchar_t *key)
{
  const struct gvdb_hash_item *item;

  if ((item = gvdb_table_lookup (table, key, 'v')) == NULL)
    return NULL;

  return gvdb_table_value_from_item (table, item);
}

/**
 * gvdb_table_get_table:
 * @file: a #GvdbTable
 * @key: a string
 *
 * Looks up the hash table named @key in @file.
 *
 * The toplevel hash table in a #GvdbTable can contain reference to
 * child hash tables (and those can contain further references...).
 *
 * If @key is not found in @file then %NULL is returned.  Otherwise, a
 * new #GvdbTable is returned, referring to the child hashtable as
 * contained in the file.  This newly-created #GvdbTable does not depend
 * on the continued existence of @file.
 *
 * You should call gvdb_table_free() on the return result when you no
 * longer require it.
 *
 * Returns: a new #GvdbTable, or %NULL
 **/
GvdbTable *
gvdb_table_get_table (GvdbTable   *file,
                      const xchar_t *key)
{
  const struct gvdb_hash_item *item;
  GvdbTable *new;

  item = gvdb_table_lookup (file, key, 'H');

  if (item == NULL)
    return NULL;

  new = g_slice_new0 (GvdbTable);
  new->bytes = xbytes_ref (file->bytes);
  new->byteswapped = file->byteswapped;
  new->trusted = file->trusted;
  new->data = file->data;
  new->size = file->size;

  gvdb_table_setup_root (new, &item->value.pointer);

  return new;
}

/**
 * gvdb_table_free:
 * @file: a #GvdbTable
 *
 * Frees @file.
 **/
void
gvdb_table_free (GvdbTable *file)
{
  xbytes_unref (file->bytes);
  g_slice_free (GvdbTable, file);
}

/**
 * gvdb_table_is_valid:
 * @table: a #GvdbTable
 *
 * Checks if the table is still valid.
 *
 * An on-disk GVDB can be marked as invalid.  This happens when the file
 * has been replaced.  The appropriate action is typically to reopen the
 * file.
 *
 * Returns: %TRUE if @table is still valid
 **/
xboolean_t
gvdb_table_is_valid (GvdbTable *table)
{
  return !!*table->data;
}
