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

#include "gvdb-builder.h"
#include "gvdb-format.h"

#include <glib.h>
#include <fcntl.h>
#if !defined(G_OS_WIN32) || !defined(_MSC_VER)
#include <unistd.h>
#endif
#include <string.h>


struct _GvdbItem
{
  xchar_t *key;
  xuint32_t hash_value;
  guint32_le assigned_index;
  GvdbItem *parent;
  GvdbItem *sibling;
  GvdbItem *next;

  /* one of:
   * this:
   */
  xvariant_t *value;

  /* this: */
  xhashtable_t *table;

  /* or this: */
  GvdbItem *child;
};

static void
gvdb_item_free (xpointer_t data)
{
  GvdbItem *item = data;

  g_free (item->key);

  if (item->value)
    xvariant_unref (item->value);

  if (item->table)
    xhash_table_unref (item->table);

  g_slice_free (GvdbItem, item);
}

xhashtable_t *
gvdb_hash_table_new (xhashtable_t  *parent,
                     const xchar_t *name_in_parent)
{
  xhashtable_t *table;

  table = xhash_table_new_full (xstr_hash, xstr_equal,
                                 g_free, gvdb_item_free);

  if (parent)
    {
      GvdbItem *item;

      item = gvdb_hash_table_insert (parent, name_in_parent);
      gvdb_item_set_hash_table (item, table);
    }

  return table;
}

static xuint32_t
djb_hash (const xchar_t *key)
{
  xuint32_t hash_value = 5381;

  while (*key)
    hash_value = hash_value * 33 + *(signed char *)key++;

  return hash_value;
}

GvdbItem *
gvdb_hash_table_insert (xhashtable_t  *table,
                        const xchar_t *key)
{
  GvdbItem *item;

  item = g_slice_new0 (GvdbItem);
  item->key = xstrdup (key);
  item->hash_value = djb_hash (key);

  xhash_table_insert (table, xstrdup (key), item);

  return item;
}

void
gvdb_hash_table_insert_string (xhashtable_t  *table,
                               const xchar_t *key,
                               const xchar_t *value)
{
  GvdbItem *item;

  item = gvdb_hash_table_insert (table, key);
  gvdb_item_set_value (item, xvariant_new_string (value));
}

void
gvdb_item_set_value (GvdbItem *item,
                     xvariant_t *value)
{
  g_return_if_fail (!item->value && !item->table && !item->child);

  item->value = xvariant_ref_sink (value);
}

void
gvdb_item_set_hash_table (GvdbItem   *item,
                          xhashtable_t *table)
{
  g_return_if_fail (!item->value && !item->table && !item->child);

  item->table = xhash_table_ref (table);
}

void
gvdb_item_set_parent (GvdbItem *item,
                      GvdbItem *parent)
{
  GvdbItem **node;

  g_return_if_fail (xstr_has_prefix (item->key, parent->key));
  g_return_if_fail (!parent->value && !parent->table);
  g_return_if_fail (!item->parent && !item->sibling);

  for (node = &parent->child; *node; node = &(*node)->sibling)
    if (strcmp ((*node)->key, item->key) > 0)
      break;

  item->parent = parent;
  item->sibling = *node;
  *node = item;
}

typedef struct
{
  GvdbItem **buckets;
  xint_t n_buckets;
} HashTable;

static HashTable *
hash_table_new (xint_t n_buckets)
{
  HashTable *table;

  table = g_slice_new (HashTable);
  table->buckets = g_new0 (GvdbItem *, n_buckets);
  table->n_buckets = n_buckets;

  return table;
}

static void
hash_table_free (HashTable *table)
{
  g_free (table->buckets);

  g_slice_free (HashTable, table);
}

static void
hash_table_insert (xpointer_t key,
                   xpointer_t value,
                   xpointer_t data)
{
  xuint32_t hash_value, bucket;
  HashTable *table = data;
  GvdbItem *item = value;

  hash_value = djb_hash (key);
  bucket = hash_value % table->n_buckets;
  item->next = table->buckets[bucket];
  table->buckets[bucket] = item;
}

static guint32_le
item_to_index (GvdbItem *item)
{
  if (item != NULL)
    return item->assigned_index;

  return guint32_to_le ((xuint32_t) -1);
}

typedef struct
{
  xqueue_t *chunks;
  xuint64_t offset;
  xboolean_t byteswap;
} FileBuilder;

typedef struct
{
  xsize_t offset;
  xsize_t size;
  xpointer_t data;
} FileChunk;

static xpointer_t
file_builder_allocate (FileBuilder         *fb,
                       xuint_t                alignment,
                       xsize_t                size,
                       struct gvdb_pointer *pointer)
{
  FileChunk *chunk;

  if (size == 0)
    return NULL;

  fb->offset += (xuint64_t) (-fb->offset) & (alignment - 1);
  chunk = g_slice_new (FileChunk);
  chunk->offset = fb->offset;
  chunk->size = size;
  chunk->data = g_malloc (size);

  pointer->start = guint32_to_le (fb->offset);
  fb->offset += size;
  pointer->end = guint32_to_le (fb->offset);

  g_queue_push_tail (fb->chunks, chunk);

  return chunk->data;
}

static void
file_builder_add_value (FileBuilder         *fb,
                        xvariant_t            *value,
                        struct gvdb_pointer *pointer)
{
  xvariant_t *variant, *normal;
  xpointer_t data;
  xsize_t size;

  if (fb->byteswap)
    {
      value = xvariant_byteswap (value);
      variant = xvariant_new_variant (value);
      xvariant_unref (value);
    }
  else
    variant = xvariant_new_variant (value);

  normal = xvariant_get_normal_form (variant);
  xvariant_unref (variant);

  size = xvariant_get_size (normal);
  data = file_builder_allocate (fb, 8, size, pointer);
  xvariant_store (normal, data);
  xvariant_unref (normal);
}

static void
file_builder_add_string (FileBuilder *fb,
                         const xchar_t *string,
                         guint32_le  *start,
                         guint16_le  *size)
{
  FileChunk *chunk;
  xsize_t length;

  length = strlen (string);

  chunk = g_slice_new (FileChunk);
  chunk->offset = fb->offset;
  chunk->size = length;
  chunk->data = g_malloc (length);
  if (length != 0)
    memcpy (chunk->data, string, length);

  *start = guint32_to_le (fb->offset);
  *size = guint16_to_le (length);
  fb->offset += length;

  g_queue_push_tail (fb->chunks, chunk);
}

static void
file_builder_allocate_for_hash (FileBuilder            *fb,
                                xsize_t                   n_buckets,
                                xsize_t                   n_items,
                                xuint_t                   bloom_shift,
                                xsize_t                   n_bloom_words,
                                guint32_le            **bloom_filter,
                                guint32_le            **hash_buckets,
                                struct gvdb_hash_item **hash_items,
                                struct gvdb_pointer    *pointer)
{
  guint32_le bloom_hdr, table_hdr;
  xuchar_t *data;
  xsize_t size;

  xassert (n_bloom_words < (1u << 27));

  bloom_hdr = guint32_to_le (bloom_shift << 27 | n_bloom_words);
  table_hdr = guint32_to_le (n_buckets);

  size = sizeof bloom_hdr + sizeof table_hdr +
         n_bloom_words * sizeof (guint32_le) +
         n_buckets     * sizeof (guint32_le) +
         n_items       * sizeof (struct gvdb_hash_item);

  data = file_builder_allocate (fb, 4, size, pointer);

#define chunk(s) (size -= (s), data += (s), data - (s))
  memcpy (chunk (sizeof bloom_hdr), &bloom_hdr, sizeof bloom_hdr);
  memcpy (chunk (sizeof table_hdr), &table_hdr, sizeof table_hdr);
  *bloom_filter = (guint32_le *) chunk (n_bloom_words * sizeof (guint32_le));
  *hash_buckets = (guint32_le *) chunk (n_buckets * sizeof (guint32_le));
  *hash_items = (struct gvdb_hash_item *) chunk (n_items *
                  sizeof (struct gvdb_hash_item));
  xassert (size == 0);
#undef chunk

  memset (*bloom_filter, 0, n_bloom_words * sizeof (guint32_le));
  memset (*hash_buckets, 0, n_buckets * sizeof (guint32_le));
  memset (*hash_items, 0, n_items * sizeof (struct gvdb_hash_item));

  /* NOTE - the code to actually fill in the bloom filter here is missing.
   * Patches welcome!
   *
   * http://en.wikipedia.org/wiki/Bloom_filter
   * http://0pointer.de/blog/projects/bloom.html
   */
}

static void
file_builder_add_hash (FileBuilder         *fb,
                       xhashtable_t          *table,
                       struct gvdb_pointer *pointer)
{
  guint32_le *buckets, *bloom_filter;
  struct gvdb_hash_item *items;
  HashTable *mytable;
  GvdbItem *item;
  xuint32_t index;
  xint_t bucket;

  mytable = hash_table_new (xhash_table_size (table));
  xhash_table_foreach (table, hash_table_insert, mytable);
  index = 0;

  for (bucket = 0; bucket < mytable->n_buckets; bucket++)
    for (item = mytable->buckets[bucket]; item; item = item->next)
      item->assigned_index = guint32_to_le (index++);

  file_builder_allocate_for_hash (fb, mytable->n_buckets, index, 5, 0,
                                  &bloom_filter, &buckets, &items, pointer);

  index = 0;
  for (bucket = 0; bucket < mytable->n_buckets; bucket++)
    {
      buckets[bucket] = guint32_to_le (index);

      for (item = mytable->buckets[bucket]; item; item = item->next)
        {
          struct gvdb_hash_item *entry = items++;
          const xchar_t *basename;

          xassert (index == guint32_from_le (item->assigned_index));
          entry->hash_value = guint32_to_le (item->hash_value);
          entry->parent = item_to_index (item->parent);
          entry->unused = 0;

          if (item->parent != NULL)
            basename = item->key + strlen (item->parent->key);
          else
            basename = item->key;

          file_builder_add_string (fb, basename,
                                   &entry->key_start,
                                   &entry->key_size);

          if (item->value != NULL)
            {
              xassert (item->child == NULL && item->table == NULL);

              file_builder_add_value (fb, item->value, &entry->value.pointer);
              entry->type = 'v';
            }

          if (item->child != NULL)
            {
              xuint32_t children = 0, i = 0;
              guint32_le *offsets;
              GvdbItem *child;

              xassert (item->table == NULL);

              for (child = item->child; child; child = child->sibling)
                children++;

              offsets = file_builder_allocate (fb, 4, 4 * children,
                                               &entry->value.pointer);
              entry->type = 'L';

              for (child = item->child; child; child = child->sibling)
                offsets[i++] = child->assigned_index;

              xassert (children == i);
            }

          if (item->table != NULL)
            {
              entry->type = 'H';
              file_builder_add_hash (fb, item->table, &entry->value.pointer);
            }

          index++;
        }
    }

  hash_table_free (mytable);
}

static FileBuilder *
file_builder_new (xboolean_t byteswap)
{
  FileBuilder *builder;

  builder = g_slice_new (FileBuilder);
  builder->chunks = g_queue_new ();
  builder->offset = sizeof (struct gvdb_header);
  builder->byteswap = byteswap;

  return builder;
}

static void
file_builder_free (FileBuilder *fb)
{
  g_queue_free (fb->chunks);
  g_slice_free (FileBuilder, fb);
}

static xstring_t *
file_builder_serialise (FileBuilder          *fb,
                        struct gvdb_pointer   root)
{
  struct gvdb_header header = { { 0, 0 }, { 0 }, { 0 }, { { 0 }, { 0 } } };
  xstring_t *result;

  memset (&header, 0, sizeof (header));

  if (fb->byteswap)
    {
      header.signature[0] = GVDB_SWAPPED_SIGNATURE0;
      header.signature[1] = GVDB_SWAPPED_SIGNATURE1;
    }
  else
    {
      header.signature[0] = GVDB_SIGNATURE0;
      header.signature[1] = GVDB_SIGNATURE1;
    }

  result = xstring_new (NULL);

  header.root = root;
  xstring_append_len (result, (xpointer_t) &header, sizeof header);

  while (!g_queue_is_empty (fb->chunks))
    {
      FileChunk *chunk = g_queue_pop_head (fb->chunks);

      if (result->len != chunk->offset)
        {
          xchar_t zero[8] = { 0, };

          xassert (chunk->offset > result->len);
          xassert (chunk->offset - result->len < 8);

          xstring_append_len (result, zero, chunk->offset - result->len);
          xassert (result->len == chunk->offset);
        }

      xstring_append_len (result, chunk->data, chunk->size);
      g_free (chunk->data);

      g_slice_free (FileChunk, chunk);
    }

  return result;
}

xboolean_t
gvdb_table_write_contents (xhashtable_t   *table,
                           const xchar_t  *filename,
                           xboolean_t      byteswap,
                           xerror_t      **error)
{
  struct gvdb_pointer root;
  xboolean_t status;
  FileBuilder *fb;
  xstring_t *str;

  xreturn_val_if_fail (table != NULL, FALSE);
  xreturn_val_if_fail (filename != NULL, FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  fb = file_builder_new (byteswap);
  file_builder_add_hash (fb, table, &root);
  str = file_builder_serialise (fb, root);
  file_builder_free (fb);

  status = xfile_set_contents (filename, str->str, str->len, error);
  xstring_free (str, TRUE);

  return status;
}

typedef struct {
  xbytes_t *contents;  /* (owned) */
  xfile_t  *file;      /* (owned) */
} WriteContentsData;

static WriteContentsData *
write_contents_data_new (xbytes_t *contents,
                         xfile_t  *file)
{
  WriteContentsData *data;

  data = g_slice_new (WriteContentsData);
  data->contents = xbytes_ref (contents);
  data->file = xobject_ref (file);

  return data;
}

static void
write_contents_data_free (WriteContentsData *data)
{
  xbytes_unref (data->contents);
  xobject_unref (data->file);
  g_slice_free (WriteContentsData, data);
}

static void
replace_contents_cb (xobject_t      *source_object,
                     xasync_result_t *result,
                     xpointer_t      user_data)
{
  xtask_t *task = user_data;
  WriteContentsData *data = xtask_get_task_data (task);
  xerror_t *error = NULL;

  g_return_if_fail (xtask_get_source_tag (task) == gvdb_table_write_contents_async);

  if (!xfile_replace_contents_finish (data->file, result, NULL, &error))
    xtask_return_error (task, g_steal_pointer (&error));
  else
    xtask_return_boolean (task, TRUE);

  xobject_unref (task);
}

void
gvdb_table_write_contents_async (xhashtable_t          *table,
                                 const xchar_t         *filename,
                                 xboolean_t             byteswap,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  struct gvdb_pointer root;
  FileBuilder *fb;
  WriteContentsData *data;
  xstring_t *str;
  xbytes_t *bytes;
  xfile_t *file;
  xtask_t *task;

  g_return_if_fail (table != NULL);
  g_return_if_fail (filename != NULL);
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  fb = file_builder_new (byteswap);
  file_builder_add_hash (fb, table, &root);
  str = file_builder_serialise (fb, root);
  bytes = xstring_free_to_bytes (str);
  file_builder_free (fb);

  file = xfile_new_for_path (filename);
  data = write_contents_data_new (bytes, file);

  task = xtask_new (NULL, cancellable, callback, user_data);
  xtask_set_task_data (task, data, (xdestroy_notify_t)write_contents_data_free);
  xtask_set_source_tag (task, gvdb_table_write_contents_async);

  xfile_replace_contents_async (file,
                                 xbytes_get_data (bytes, NULL),
                                 xbytes_get_size (bytes),
                                 NULL, FALSE,
                                 XFILE_CREATE_PRIVATE,
                                 cancellable, replace_contents_cb, g_steal_pointer (&task));

  xbytes_unref (bytes);
  xobject_unref (file);
}

xboolean_t
gvdb_table_write_contents_finish (xhashtable_t    *table,
                                  xasync_result_t  *result,
                                  xerror_t       **error)
{
  xreturn_val_if_fail (table != NULL, FALSE);
  xreturn_val_if_fail (xtask_is_valid (result, NULL), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}
