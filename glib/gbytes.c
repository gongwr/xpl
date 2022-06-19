/*
 * Copyright © 2009, 2010 Codethink Limited
 * Copyright © 2011 Collabora Ltd.
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
 *         Stef Walter <stefw@collabora.co.uk>
 */

#include "config.h"

#include "gbytes.h"

#include <glib/garray.h>
#include <glib/gstrfuncs.h>
#include <glib/gatomic.h>
#include <glib/gslice.h>
#include <glib/gtestutils.h>
#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <glib/grefcount.h>

#include <string.h>

/**
 * xbytes_t:
 *
 * A simple refcounted data type representing an immutable sequence of zero or
 * more bytes from an unspecified origin.
 *
 * The purpose of a #xbytes_t is to keep the memory region that it holds
 * alive for as long as anyone holds a reference to the bytes.  When
 * the last reference count is dropped, the memory is released. Multiple
 * unrelated callers can use byte data in the #xbytes_t without coordinating
 * their activities, resting assured that the byte data will not change or
 * move while they hold a reference.
 *
 * A #xbytes_t can come from many different origins that may have
 * different procedures for freeing the memory region.  Examples are
 * memory from g_malloc(), from memory slices, from a #xmapped_file_t or
 * memory from other allocators.
 *
 * #xbytes_t work well as keys in #xhashtable_t. Use xbytes_equal() and
 * xbytes_hash() as parameters to xhash_table_new() or xhash_table_new_full().
 * #xbytes_t can also be used as keys in a #xtree_t by passing the xbytes_compare()
 * function to xtree_new().
 *
 * The data pointed to by this bytes must not be modified. For a mutable
 * array of bytes see #xbyte_array_t. Use xbytes_unref_to_array() to create a
 * mutable array for a #xbytes_t sequence. To create an immutable #xbytes_t from
 * a mutable #xbyte_array_t, use the xbyte_array_free_to_bytes() function.
 *
 * Since: 2.32
 **/

/* Keep in sync with glib/tests/bytes.c */
struct _GBytes
{
  xconstpointer data;  /* may be NULL iff (size == 0) */
  xsize_t size;  /* may be 0 */
  gatomicrefcount ref_count;
  xdestroy_notify_t free_func;
  xpointer_t user_data;
};

/**
 * xbytes_new:
 * @data: (transfer none) (array length=size) (element-type xuint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 *
 * Creates a new #xbytes_t from @data.
 *
 * @data is copied. If @size is 0, @data may be %NULL.
 *
 * Returns: (transfer full): a new #xbytes_t
 *
 * Since: 2.32
 */
xbytes_t *
xbytes_new (xconstpointer data,
             xsize_t         size)
{
  xreturn_val_if_fail (data != NULL || size == 0, NULL);

  return xbytes_new_take (g_memdup2 (data, size), size);
}

/**
 * xbytes_new_take:
 * @data: (transfer full) (array length=size) (element-type xuint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 *
 * Creates a new #xbytes_t from @data.
 *
 * After this call, @data belongs to the bytes and may no longer be
 * modified by the caller.  g_free() will be called on @data when the
 * bytes is no longer in use. Because of this @data must have been created by
 * a call to g_malloc(), g_malloc0() or g_realloc() or by one of the many
 * functions that wrap these calls (such as g_new(), xstrdup(), etc).
 *
 * For creating #xbytes_t with memory from other allocators, see
 * xbytes_new_with_free_func().
 *
 * @data may be %NULL if @size is 0.
 *
 * Returns: (transfer full): a new #xbytes_t
 *
 * Since: 2.32
 */
xbytes_t *
xbytes_new_take (xpointer_t data,
                  xsize_t    size)
{
  return xbytes_new_with_free_func (data, size, g_free, data);
}


/**
 * xbytes_new_static: (skip)
 * @data: (transfer full) (array length=size) (element-type xuint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 *
 * Creates a new #xbytes_t from static data.
 *
 * @data must be static (ie: never modified or freed). It may be %NULL if @size
 * is 0.
 *
 * Returns: (transfer full): a new #xbytes_t
 *
 * Since: 2.32
 */
xbytes_t *
xbytes_new_static (xconstpointer data,
                    xsize_t         size)
{
  return xbytes_new_with_free_func (data, size, NULL, NULL);
}

/**
 * xbytes_new_with_free_func: (skip)
 * @data: (array length=size) (element-type xuint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 * @free_func: the function to call to release the data
 * @user_data: data to pass to @free_func
 *
 * Creates a #xbytes_t from @data.
 *
 * When the last reference is dropped, @free_func will be called with the
 * @user_data argument.
 *
 * @data must not be modified after this call is made until @free_func has
 * been called to indicate that the bytes is no longer in use.
 *
 * @data may be %NULL if @size is 0.
 *
 * Returns: (transfer full): a new #xbytes_t
 *
 * Since: 2.32
 */
xbytes_t *
xbytes_new_with_free_func (xconstpointer  data,
                            xsize_t          size,
                            xdestroy_notify_t free_func,
                            xpointer_t       user_data)
{
  xbytes_t *bytes;

  xreturn_val_if_fail (data != NULL || size == 0, NULL);

  bytes = g_slice_new (xbytes_t);
  bytes->data = data;
  bytes->size = size;
  bytes->free_func = free_func;
  bytes->user_data = user_data;
  g_atomic_ref_count_init (&bytes->ref_count);

  return (xbytes_t *)bytes;
}

/**
 * xbytes_new_from_bytes:
 * @bytes: a #xbytes_t
 * @offset: offset which subsection starts at
 * @length: length of subsection
 *
 * Creates a #xbytes_t which is a subsection of another #xbytes_t. The @offset +
 * @length may not be longer than the size of @bytes.
 *
 * A reference to @bytes will be held by the newly created #xbytes_t until
 * the byte data is no longer needed.
 *
 * Since 2.56, if @offset is 0 and @length matches the size of @bytes, then
 * @bytes will be returned with the reference count incremented by 1. If @bytes
 * is a slice of another #xbytes_t, then the resulting #xbytes_t will reference
 * the same #xbytes_t instead of @bytes. This allows consumers to simplify the
 * usage of #xbytes_t when asynchronously writing to streams.
 *
 * Returns: (transfer full): a new #xbytes_t
 *
 * Since: 2.32
 */
xbytes_t *
xbytes_new_from_bytes (xbytes_t  *bytes,
                        xsize_t    offset,
                        xsize_t    length)
{
  xchar_t *base;

  /* Note that length may be 0. */
  xreturn_val_if_fail (bytes != NULL, NULL);
  xreturn_val_if_fail (offset <= bytes->size, NULL);
  xreturn_val_if_fail (offset + length <= bytes->size, NULL);

  /* Avoid an extra xbytes_t if all bytes were requested */
  if (offset == 0 && length == bytes->size)
    return xbytes_ref (bytes);

  base = (xchar_t *)bytes->data + offset;

  /* Avoid referencing intermediate xbytes_t. In practice, this should
   * only loop once.
   */
  while (bytes->free_func == (xpointer_t)xbytes_unref)
    bytes = bytes->user_data;

  xreturn_val_if_fail (bytes != NULL, NULL);
  xreturn_val_if_fail (base >= (xchar_t *)bytes->data, NULL);
  xreturn_val_if_fail (base <= (xchar_t *)bytes->data + bytes->size, NULL);
  xreturn_val_if_fail (base + length <= (xchar_t *)bytes->data + bytes->size, NULL);

  return xbytes_new_with_free_func (base, length,
                                     (xdestroy_notify_t)xbytes_unref, xbytes_ref (bytes));
}

/**
 * xbytes_get_data:
 * @bytes: a #xbytes_t
 * @size: (out) (optional): location to return size of byte data
 *
 * Get the byte data in the #xbytes_t. This data should not be modified.
 *
 * This function will always return the same pointer for a given #xbytes_t.
 *
 * %NULL may be returned if @size is 0. This is not guaranteed, as the #xbytes_t
 * may represent an empty string with @data non-%NULL and @size as 0. %NULL will
 * not be returned if @size is non-zero.
 *
 * Returns: (transfer none) (array length=size) (element-type xuint8_t) (nullable):
 *          a pointer to the byte data, or %NULL
 *
 * Since: 2.32
 */
xconstpointer
xbytes_get_data (xbytes_t *bytes,
                  xsize_t *size)
{
  xreturn_val_if_fail (bytes != NULL, NULL);
  if (size)
    *size = bytes->size;
  return bytes->data;
}

/**
 * xbytes_get_size:
 * @bytes: a #xbytes_t
 *
 * Get the size of the byte data in the #xbytes_t.
 *
 * This function will always return the same value for a given #xbytes_t.
 *
 * Returns: the size
 *
 * Since: 2.32
 */
xsize_t
xbytes_get_size (xbytes_t *bytes)
{
  xreturn_val_if_fail (bytes != NULL, 0);
  return bytes->size;
}


/**
 * xbytes_ref:
 * @bytes: a #xbytes_t
 *
 * Increase the reference count on @bytes.
 *
 * Returns: the #xbytes_t
 *
 * Since: 2.32
 */
xbytes_t *
xbytes_ref (xbytes_t *bytes)
{
  xreturn_val_if_fail (bytes != NULL, NULL);

  g_atomic_ref_count_inc (&bytes->ref_count);

  return bytes;
}

/**
 * xbytes_unref:
 * @bytes: (nullable): a #xbytes_t
 *
 * Releases a reference on @bytes.  This may result in the bytes being
 * freed. If @bytes is %NULL, it will return immediately.
 *
 * Since: 2.32
 */
void
xbytes_unref (xbytes_t *bytes)
{
  if (bytes == NULL)
    return;

  if (g_atomic_ref_count_dec (&bytes->ref_count))
    {
      if (bytes->free_func != NULL)
        bytes->free_func (bytes->user_data);
      g_slice_free (xbytes_t, bytes);
    }
}

/**
 * xbytes_equal:
 * @bytes1: (type GLib.Bytes): a pointer to a #xbytes_t
 * @bytes2: (type GLib.Bytes): a pointer to a #xbytes_t to compare with @bytes1
 *
 * Compares the two #xbytes_t values being pointed to and returns
 * %TRUE if they are equal.
 *
 * This function can be passed to xhash_table_new() as the @key_equal_func
 * parameter, when using non-%NULL #xbytes_t pointers as keys in a #xhashtable_t.
 *
 * Returns: %TRUE if the two keys match.
 *
 * Since: 2.32
 */
xboolean_t
xbytes_equal (xconstpointer bytes1,
               xconstpointer bytes2)
{
  const xbytes_t *b1 = bytes1;
  const xbytes_t *b2 = bytes2;

  xreturn_val_if_fail (bytes1 != NULL, FALSE);
  xreturn_val_if_fail (bytes2 != NULL, FALSE);

  return b1->size == b2->size &&
         (b1->size == 0 || memcmp (b1->data, b2->data, b1->size) == 0);
}

/**
 * xbytes_hash:
 * @bytes: (type GLib.Bytes): a pointer to a #xbytes_t key
 *
 * Creates an integer hash code for the byte data in the #xbytes_t.
 *
 * This function can be passed to xhash_table_new() as the @key_hash_func
 * parameter, when using non-%NULL #xbytes_t pointers as keys in a #xhashtable_t.
 *
 * Returns: a hash value corresponding to the key.
 *
 * Since: 2.32
 */
xuint_t
xbytes_hash (xconstpointer bytes)
{
  const xbytes_t *a = bytes;
  const signed char *p, *e;
  xuint32_t h = 5381;

  xreturn_val_if_fail (bytes != NULL, 0);

  for (p = (signed char *)a->data, e = (signed char *)a->data + a->size; p != e; p++)
    h = (h << 5) + h + *p;

  return h;
}

/**
 * xbytes_compare:
 * @bytes1: (type GLib.Bytes): a pointer to a #xbytes_t
 * @bytes2: (type GLib.Bytes): a pointer to a #xbytes_t to compare with @bytes1
 *
 * Compares the two #xbytes_t values.
 *
 * This function can be used to sort xbytes_t instances in lexicographical order.
 *
 * If @bytes1 and @bytes2 have different length but the shorter one is a
 * prefix of the longer one then the shorter one is considered to be less than
 * the longer one. Otherwise the first byte where both differ is used for
 * comparison. If @bytes1 has a smaller value at that position it is
 * considered less, otherwise greater than @bytes2.
 *
 * Returns: a negative value if @bytes1 is less than @bytes2, a positive value
 *          if @bytes1 is greater than @bytes2, and zero if @bytes1 is equal to
 *          @bytes2
 *
 *
 * Since: 2.32
 */
xint_t
xbytes_compare (xconstpointer bytes1,
                 xconstpointer bytes2)
{
  const xbytes_t *b1 = bytes1;
  const xbytes_t *b2 = bytes2;
  xint_t ret;

  xreturn_val_if_fail (bytes1 != NULL, 0);
  xreturn_val_if_fail (bytes2 != NULL, 0);

  ret = memcmp (b1->data, b2->data, MIN (b1->size, b2->size));
  if (ret == 0 && b1->size != b2->size)
      ret = b1->size < b2->size ? -1 : 1;
  return ret;
}

static xpointer_t
try_steal_and_unref (xbytes_t         *bytes,
                     xdestroy_notify_t  free_func,
                     xsize_t          *size)
{
  xpointer_t result;

  if (bytes->free_func != free_func || bytes->data == NULL ||
      bytes->user_data != bytes->data)
    return NULL;

  /* Are we the only reference? */
  if (g_atomic_ref_count_compare (&bytes->ref_count, 1))
    {
      *size = bytes->size;
      result = (xpointer_t)bytes->data;
      g_slice_free (xbytes_t, bytes);
      return result;
    }

  return NULL;
}


/**
 * xbytes_unref_to_data:
 * @bytes: (transfer full): a #xbytes_t
 * @size: (out): location to place the length of the returned data
 *
 * Unreferences the bytes, and returns a pointer the same byte data
 * contents.
 *
 * As an optimization, the byte data is returned without copying if this was
 * the last reference to bytes and bytes was created with xbytes_new(),
 * xbytes_new_take() or xbyte_array_free_to_bytes(). In all other cases the
 * data is copied.
 *
 * Returns: (transfer full) (array length=size) (element-type xuint8_t)
 *          (not nullable): a pointer to the same byte data, which should be
 *          freed with g_free()
 *
 * Since: 2.32
 */
xpointer_t
xbytes_unref_to_data (xbytes_t *bytes,
                       xsize_t  *size)
{
  xpointer_t result;

  xreturn_val_if_fail (bytes != NULL, NULL);
  xreturn_val_if_fail (size != NULL, NULL);

  /*
   * Optimal path: if this is was the last reference, then we can return
   * the data from this xbytes_t without copying.
   */

  result = try_steal_and_unref (bytes, g_free, size);
  if (result == NULL)
    {
      /*
       * Copy: Non g_malloc (or compatible) allocator, or static memory,
       * so we have to copy, and then unref.
       */
      result = g_memdup2 (bytes->data, bytes->size);
      *size = bytes->size;
      xbytes_unref (bytes);
    }

  return result;
}

/**
 * xbytes_unref_to_array:
 * @bytes: (transfer full): a #xbytes_t
 *
 * Unreferences the bytes, and returns a new mutable #xbyte_array_t containing
 * the same byte data.
 *
 * As an optimization, the byte data is transferred to the array without copying
 * if this was the last reference to bytes and bytes was created with
 * xbytes_new(), xbytes_new_take() or xbyte_array_free_to_bytes(). In all
 * other cases the data is copied.
 *
 * Do not use it if @bytes contains more than %G_MAXUINT
 * bytes. #xbyte_array_t stores the length of its data in #xuint_t, which
 * may be shorter than #xsize_t, that @bytes is using.
 *
 * Returns: (transfer full): a new mutable #xbyte_array_t containing the same byte data
 *
 * Since: 2.32
 */
xbyte_array_t *
xbytes_unref_to_array (xbytes_t *bytes)
{
  xpointer_t data;
  xsize_t size;

  xreturn_val_if_fail (bytes != NULL, NULL);

  data = xbytes_unref_to_data (bytes, &size);
  return xbyte_array_new_take (data, size);
}

/**
 * xbytes_get_region:
 * @bytes: a #xbytes_t
 * @element_size: a non-zero element size
 * @offset: an offset to the start of the region within the @bytes
 * @n_elements: the number of elements in the region
 *
 * Gets a pointer to a region in @bytes.
 *
 * The region starts at @offset many bytes from the start of the data
 * and contains @n_elements many elements of @element_size size.
 *
 * @n_elements may be zero, but @element_size must always be non-zero.
 * Ideally, @element_size is a static constant (eg: sizeof a struct).
 *
 * This function does careful bounds checking (including checking for
 * arithmetic overflows) and returns a non-%NULL pointer if the
 * specified region lies entirely within the @bytes. If the region is
 * in some way out of range, or if an overflow has occurred, then %NULL
 * is returned.
 *
 * Note: it is possible to have a valid zero-size region. In this case,
 * the returned pointer will be equal to the base pointer of the data of
 * @bytes, plus @offset.  This will be non-%NULL except for the case
 * where @bytes itself was a zero-sized region.  Since it is unlikely
 * that you will be using this function to check for a zero-sized region
 * in a zero-sized @bytes, %NULL effectively always means "error".
 *
 * Returns: (nullable): the requested region, or %NULL in case of an error
 *
 * Since: 2.70
 */
xconstpointer
xbytes_get_region (xbytes_t *bytes,
                    xsize_t   element_size,
                    xsize_t   offset,
                    xsize_t   n_elements)
{
  xsize_t total_size;
  xsize_t end_offset;

  xreturn_val_if_fail (element_size > 0, NULL);

  /* No other assertion checks here.  If something is wrong then we will
   * simply crash (via NULL dereference or divide-by-zero).
   */

  if (!g_size_checked_mul (&total_size, element_size, n_elements))
    return NULL;

  if (!g_size_checked_add (&end_offset, offset, total_size))
    return NULL;

  /* We now have:
   *
   *   0 <= offset <= end_offset
   *
   * So we need only check that end_offset is within the range of the
   * size of @bytes and we're good to go.
   */

  if (end_offset > bytes->size)
    return NULL;

  /* We now have:
   *
   *   0 <= offset <= end_offset <= bytes->size
   */

  return ((xuchar_t *) bytes->data) + offset;
}