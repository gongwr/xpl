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

/*
 * MT safe
 */

#include "config.h"

#include <string.h>

#include "gstringchunk.h"

#include "ghash.h"
#include "gslist.h"
#include "gmessages.h"

#include "gutils.h"
#include "gutilsprivate.h"

/**
 * SECTION:string_chunks
 * @title: String Chunks
 * @short_description: efficient storage of groups of strings
 *
 * String chunks are used to store groups of strings. Memory is
 * allocated in blocks, and as strings are added to the #xstring_chunk_t
 * they are copied into the next free position in a block. When a block
 * is full a new block is allocated.
 *
 * When storing a large number of strings, string chunks are more
 * efficient than using xstrdup() since fewer calls to malloc() are
 * needed, and less memory is wasted in memory allocation overheads.
 *
 * By adding strings with xstring_chunk_insert_const() it is also
 * possible to remove duplicates.
 *
 * To create a new #xstring_chunk_t use xstring_chunk_new().
 *
 * To add strings to a #xstring_chunk_t use xstring_chunk_insert().
 *
 * To add strings to a #xstring_chunk_t, but without duplicating strings
 * which are already in the #xstring_chunk_t, use
 * xstring_chunk_insert_const().
 *
 * To free the entire #xstring_chunk_t use xstring_chunk_free(). It is
 * not possible to free individual strings.
 */

/**
 * xstring_chunk_t:
 *
 * An opaque data structure representing String Chunks.
 * It should only be accessed by using the following functions.
 */
struct _GStringChunk
{
  xhashtable_t *const_table;
  xslist_t     *storage_list;
  xsize_t       storage_next;
  xsize_t       this_size;
  xsize_t       default_size;
};

/**
 * xstring_chunk_new:
 * @size: the default size of the blocks of memory which are
 *     allocated to store the strings. If a particular string
 *     is larger than this default size, a larger block of
 *     memory will be allocated for it.
 *
 * Creates a new #xstring_chunk_t.
 *
 * Returns: a new #xstring_chunk_t
 */
xstring_chunk_t *
xstring_chunk_new (xsize_t size)
{
  xstring_chunk_t *new_chunk = g_new (xstring_chunk_t, 1);
  xsize_t actual_size = 1;

  actual_size = g_nearest_pow (MAX (1, size));

  new_chunk->const_table  = NULL;
  new_chunk->storage_list = NULL;
  new_chunk->storage_next = actual_size;
  new_chunk->default_size = actual_size;
  new_chunk->this_size    = actual_size;

  return new_chunk;
}

/**
 * xstring_chunk_free:
 * @chunk: a #xstring_chunk_t
 *
 * Frees all memory allocated by the #xstring_chunk_t.
 * After calling xstring_chunk_free() it is not safe to
 * access any of the strings which were contained within it.
 */
void
xstring_chunk_free (xstring_chunk_t *chunk)
{
  g_return_if_fail (chunk != NULL);

  if (chunk->storage_list)
    xslist_free_full (chunk->storage_list, g_free);

  if (chunk->const_table)
    xhash_table_destroy (chunk->const_table);

  g_free (chunk);
}

/**
 * xstring_chunk_clear:
 * @chunk: a #xstring_chunk_t
 *
 * Frees all strings contained within the #xstring_chunk_t.
 * After calling xstring_chunk_clear() it is not safe to
 * access any of the strings which were contained within it.
 *
 * Since: 2.14
 */
void
xstring_chunk_clear (xstring_chunk_t *chunk)
{
  g_return_if_fail (chunk != NULL);

  if (chunk->storage_list)
    {
      xslist_free_full (chunk->storage_list, g_free);

      chunk->storage_list = NULL;
      chunk->storage_next = chunk->default_size;
      chunk->this_size    = chunk->default_size;
    }

  if (chunk->const_table)
      xhash_table_remove_all (chunk->const_table);
}

/**
 * xstring_chunk_insert:
 * @chunk: a #xstring_chunk_t
 * @string: the string to add
 *
 * Adds a copy of @string to the #xstring_chunk_t.
 * It returns a pointer to the new copy of the string
 * in the #xstring_chunk_t. The characters in the string
 * can be changed, if necessary, though you should not
 * change anything after the end of the string.
 *
 * Unlike xstring_chunk_insert_const(), this function
 * does not check for duplicates. Also strings added
 * with xstring_chunk_insert() will not be searched
 * by xstring_chunk_insert_const() when looking for
 * duplicates.
 *
 * Returns: a pointer to the copy of @string within
 *     the #xstring_chunk_t
 */
xchar_t*
xstring_chunk_insert (xstring_chunk_t *chunk,
                       const xchar_t  *string)
{
  xreturn_val_if_fail (chunk != NULL, NULL);

  return xstring_chunk_insert_len (chunk, string, -1);
}

/**
 * xstring_chunk_insert_const:
 * @chunk: a #xstring_chunk_t
 * @string: the string to add
 *
 * Adds a copy of @string to the #xstring_chunk_t, unless the same
 * string has already been added to the #xstring_chunk_t with
 * xstring_chunk_insert_const().
 *
 * This function is useful if you need to copy a large number
 * of strings but do not want to waste space storing duplicates.
 * But you must remember that there may be several pointers to
 * the same string, and so any changes made to the strings
 * should be done very carefully.
 *
 * Note that xstring_chunk_insert_const() will not return a
 * pointer to a string added with xstring_chunk_insert(), even
 * if they do match.
 *
 * Returns: a pointer to the new or existing copy of @string
 *     within the #xstring_chunk_t
 */
xchar_t*
xstring_chunk_insert_const (xstring_chunk_t *chunk,
                             const xchar_t  *string)
{
  char* lookup;

  xreturn_val_if_fail (chunk != NULL, NULL);

  if (!chunk->const_table)
    chunk->const_table = xhash_table_new (xstr_hash, xstr_equal);

  lookup = (char*) xhash_table_lookup (chunk->const_table, (xchar_t *)string);

  if (!lookup)
    {
      lookup = xstring_chunk_insert (chunk, string);
      xhash_table_add (chunk->const_table, lookup);
    }

  return lookup;
}

/**
 * xstring_chunk_insert_len:
 * @chunk: a #xstring_chunk_t
 * @string: bytes to insert
 * @len: number of bytes of @string to insert, or -1 to insert a
 *     nul-terminated string
 *
 * Adds a copy of the first @len bytes of @string to the #xstring_chunk_t.
 * The copy is nul-terminated.
 *
 * Since this function does not stop at nul bytes, it is the caller's
 * responsibility to ensure that @string has at least @len addressable
 * bytes.
 *
 * The characters in the returned string can be changed, if necessary,
 * though you should not change anything after the end of the string.
 *
 * Returns: a pointer to the copy of @string within the #xstring_chunk_t
 *
 * Since: 2.4
 */
xchar_t*
xstring_chunk_insert_len (xstring_chunk_t *chunk,
                           const xchar_t  *string,
                           xssize_t        len)
{
  xsize_t size;
  xchar_t* pos;

  xreturn_val_if_fail (chunk != NULL, NULL);

  if (len < 0)
    size = strlen (string);
  else
    size = (xsize_t) len;

  if ((G_MAXSIZE - chunk->storage_next < size + 1) || (chunk->storage_next + size + 1) > chunk->this_size)
    {
      xsize_t new_size = g_nearest_pow (MAX (chunk->default_size, size + 1));

      /* If size is bigger than G_MAXSIZE / 2 then store it in its own
       * allocation instead of failing here */
      if (new_size == 0)
        new_size = size + 1;

      chunk->storage_list = xslist_prepend (chunk->storage_list,
                                             g_new (xchar_t, new_size));

      chunk->this_size = new_size;
      chunk->storage_next = 0;
    }

  pos = ((xchar_t *) chunk->storage_list->data) + chunk->storage_next;

  *(pos + size) = '\0';

  memcpy (pos, string, size);

  chunk->storage_next += size + 1;

  return pos;
}
