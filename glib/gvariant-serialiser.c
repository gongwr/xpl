/*
 * Copyright © 2007, 2008 Ryan Lortie
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

/* Prologue {{{1 */
#include "config.h"

#include "gvariant-serialiser.h"

#include <glib/gvariant-internal.h>
#include <glib/gtestutils.h>
#include <glib/gstrfuncs.h>
#include <glib/gtypes.h>

#include <string.h>


/* GVariantSerialiser
 *
 * After this prologue section, this file has roughly 2 parts.
 *
 * The first part is split up into sections according to various
 * container types.  Maybe, Array, Tuple, Variant.  The Maybe and Array
 * sections are subdivided for element types being fixed or
 * variable-sized types.
 *
 * Each section documents the format of that particular type of
 * container and implements 5 functions for dealing with it:
 *
 *  n_children:
 *    - determines (according to serialized data) how many child values
 *      are inside a particular container value.
 *
 *  get_child:
 *    - gets the type of and the serialized data corresponding to a
 *      given child value within the container value.
 *
 *  needed_size:
 *    - determines how much space would be required to serialize a
 *      container of this type, containing the given children so that
 *      buffers can be preallocated before serializing.
 *
 *  serialise:
 *    - write the serialized data for a container of this type,
 *      containing the given children, to a buffer.
 *
 *  is_normal:
 *    - check the given data to ensure that it is in normal form.  For a
 *      given set of child values, there is exactly one normal form for
 *      the serialized data of a container.  Other forms are possible
 *      while maintaining the same children (for example, by inserting
 *      something other than zero bytes as padding) but only one form is
 *      the normal form.
 *
 * The second part contains the main entry point for each of the above 5
 * functions and logic to dispatch it to the handler for the appropriate
 * container type code.
 *
 * The second part also contains a routine to byteswap serialized
 * values.  This code makes use of the n_children() and get_child()
 * functions above to do its work so no extra support is needed on a
 * per-container-type basis.
 *
 * There is also additional code for checking for normal form.  All
 * numeric types are always in normal form since the full range of
 * values is permitted (eg: 0 to 255 is a valid byte).  Special checks
 * need to be performed for booleans (only 0 or 1 allowed), strings
 * (properly nul-terminated) and object paths and signature strings
 * (meeting the D-Bus specification requirements).  Depth checks need to be
 * performed for nested types (arrays, tuples, and variants), to avoid massive
 * recursion which could exhaust our stack when handling untrusted input.
 */

/* < private >
 * GVariantSerialised:
 * @type_info: the #GVariantTypeInfo of this value
 * @data: (nullable): the serialized data of this value, or %NULL
 * @size: the size of this value
 *
 * A structure representing a xvariant_t in serialized form.  This
 * structure is used with #GVariantSerialisedFiller functions and as the
 * primary interface to the serializer.  See #GVariantSerialisedFiller
 * for a description of its use there.
 *
 * When used with the serializer API functions, the following invariants
 * apply to all #GVariantTypeSerialised structures passed to and
 * returned from the serializer.
 *
 * @type_info must be non-%NULL.
 *
 * @data must be properly aligned for the type described by @type_info.
 *
 * If @type_info describes a fixed-sized type then @size must always be
 * equal to the fixed size of that type.
 *
 * For fixed-sized types (and only fixed-sized types), @data may be
 * %NULL even if @size is non-zero.  This happens when a framing error
 * occurs while attempting to extract a fixed-sized value out of a
 * variable-sized container.  There is no data to return for the
 * fixed-sized type, yet @size must be non-zero.  The effect of this
 * combination should be as if @data were a pointer to an
 * appropriately-sized zero-filled region.
 *
 * @depth has no restrictions; the depth of a top-level serialized #xvariant_t is
 * zero, and it increases for each level of nested child.
 */

/* < private >
 * xvariant_serialised_check:
 * @serialised: a #GVariantSerialised struct
 *
 * Checks @serialised for validity according to the invariants described
 * above.
 *
 * Returns: %TRUE if @serialised is valid; %FALSE otherwise
 */
xboolean_t
xvariant_serialised_check (GVariantSerialised serialised)
{
  xsize_t fixed_size;
  xuint_t alignment;

  if (serialised.type_info == NULL)
    return FALSE;
  xvariant_type_info_query (serialised.type_info, &alignment, &fixed_size);

  if (fixed_size != 0 && serialised.size != fixed_size)
    return FALSE;
  else if (fixed_size == 0 &&
           !(serialised.size == 0 || serialised.data != NULL))
    return FALSE;

  /* Depending on the native alignment requirements of the machine, the
   * compiler will insert either 3 or 7 padding bytes after the char.
   * This will result in the sizeof() the struct being 12 or 16.
   * Subtract 9 to get 3 or 7 which is a nice bitmask to apply to get
   * the alignment bits that we "care about" being zero: in the
   * 4-aligned case, we care about 2 bits, and in the 8-aligned case, we
   * care about 3 bits.
   */
  alignment &= sizeof (struct {
                         char a;
                         union {
                           xuint64_t x;
                           void *y;
                           xdouble_t z;
                         } b;
                       }
                      ) - 9;

  /* Some OSes (FreeBSD is a known example) have a malloc() that returns
   * unaligned memory if you request small sizes.  'malloc (1);', for
   * example, has been seen to return pointers aligned to 6 mod 16.
   *
   * Check if this is a small allocation and return without enforcing
   * the alignment assertion if this is the case.
   */
  return (serialised.size <= alignment ||
          (alignment & (xsize_t) serialised.data) == 0);
}

/* < private >
 * GVariantSerialisedFiller:
 * @serialised: a #GVariantSerialised instance to fill
 * @data: data from the children array
 *
 * This function is called back from xvariant_serialiser_needed_size()
 * and xvariant_serialiser_serialise().  It fills in missing details
 * from a partially-complete #GVariantSerialised.
 *
 * The @data parameter passed back to the function is one of the items
 * that was passed to the serializer in the @children array.  It
 * represents a single child item of the container that is being
 * serialized.  The information filled in to @serialised is the
 * information for this child.
 *
 * If the @type_info field of @serialised is %NULL then the callback
 * function must set it to the type information corresponding to the
 * type of the child.  No reference should be added.  If it is non-%NULL
 * then the callback should assert that it is equal to the actual type
 * of the child.
 *
 * If the @size field is zero then the callback must fill it in with the
 * required amount of space to store the serialized form of the child.
 * If it is non-zero then the callback should assert that it is equal to
 * the needed size of the child.
 *
 * If @data is non-%NULL then it points to a space that is properly
 * aligned for and large enough to store the serialized data of the
 * child.  The callback must store the serialized form of the child at
 * @data.
 *
 * If the child value is another container then the callback will likely
 * recurse back into the serializer by calling
 * xvariant_serialiser_needed_size() to determine @size and
 * xvariant_serialiser_serialise() to write to @data.
 */

/* PART 1: Container types {{{1
 *
 * This section contains the serializer implementation functions for
 * each container type.
 */

/* Maybe {{{2
 *
 * Maybe types are handled depending on if the element type of the maybe
 * type is a fixed-sized or variable-sized type.  Although all maybe
 * types themselves are variable-sized types, herein, a maybe value with
 * a fixed-sized element type is called a "fixed-sized maybe" for
 * convenience and a maybe value with a variable-sized element type is
 * called a "variable-sized maybe".
 */

/* Fixed-sized Maybe {{{3
 *
 * The size of a maybe value with a fixed-sized element type is either 0
 * or equal to the fixed size of its element type.  The case where the
 * size of the maybe value is zero corresponds to the "Nothing" case and
 * the case where the size of the maybe value is equal to the fixed size
 * of the element type corresponds to the "Just" case; in that case, the
 * serialized data of the child value forms the entire serialized data
 * of the maybe value.
 *
 * In the event that a fixed-sized maybe value is presented with a size
 * that is not equal to the fixed size of the element type then the
 * value must be taken to be "Nothing".
 */

static xsize_t
gvs_fixed_sized_maybe_n_children (GVariantSerialised value)
{
  xsize_t element_fixed_size;

  xvariant_type_info_query_element (value.type_info, NULL,
                                     &element_fixed_size);

  return (element_fixed_size == value.size) ? 1 : 0;
}

static GVariantSerialised
gvs_fixed_sized_maybe_get_child (GVariantSerialised value,
                                 xsize_t              index_)
{
  /* the child has the same bounds as the
   * container, so just update the type.
   */
  value.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_ref (value.type_info);
  value.depth++;

  return value;
}

static xsize_t
gvs_fixed_sized_maybe_needed_size (GVariantTypeInfo         *type_info,
                                   GVariantSerialisedFiller  gvs_filler,
                                   const xpointer_t           *children,
                                   xsize_t                     n_children)
{
  if (n_children)
    {
      xsize_t element_fixed_size;

      xvariant_type_info_query_element (type_info, NULL,
                                         &element_fixed_size);

      return element_fixed_size;
    }
  else
    return 0;
}

static void
gvs_fixed_sized_maybe_serialise (GVariantSerialised        value,
                                 GVariantSerialisedFiller  gvs_filler,
                                 const xpointer_t           *children,
                                 xsize_t                     n_children)
{
  if (n_children)
    {
      GVariantSerialised child = { NULL, value.data, value.size, value.depth + 1 };

      gvs_filler (&child, children[0]);
    }
}

static xboolean_t
gvs_fixed_sized_maybe_is_normal (GVariantSerialised value)
{
  if (value.size > 0)
    {
      xsize_t element_fixed_size;

      xvariant_type_info_query_element (value.type_info,
                                         NULL, &element_fixed_size);

      if (value.size != element_fixed_size)
        return FALSE;

      /* proper element size: "Just".  recurse to the child. */
      value.type_info = xvariant_type_info_element (value.type_info);
      value.depth++;

      return xvariant_serialised_is_normal (value);
    }

  /* size of 0: "Nothing" */
  return TRUE;
}

/* Variable-sized Maybe
 *
 * The size of a maybe value with a variable-sized element type is
 * either 0 or strictly greater than 0.  The case where the size of the
 * maybe value is zero corresponds to the "Nothing" case and the case
 * where the size of the maybe value is greater than zero corresponds to
 * the "Just" case; in that case, the serialized data of the child value
 * forms the first part of the serialized data of the maybe value and is
 * followed by a single zero byte.  This zero byte is always appended,
 * regardless of any zero bytes that may already be at the end of the
 * serialized ata of the child value.
 */

static xsize_t
gvs_variable_sized_maybe_n_children (GVariantSerialised value)
{
  return (value.size > 0) ? 1 : 0;
}

static GVariantSerialised
gvs_variable_sized_maybe_get_child (GVariantSerialised value,
                                    xsize_t              index_)
{
  /* remove the padding byte and update the type. */
  value.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_ref (value.type_info);
  value.size--;

  /* if it's zero-sized then it may as well be NULL */
  if (value.size == 0)
    value.data = NULL;

  value.depth++;

  return value;
}

static xsize_t
gvs_variable_sized_maybe_needed_size (GVariantTypeInfo         *type_info,
                                      GVariantSerialisedFiller  gvs_filler,
                                      const xpointer_t           *children,
                                      xsize_t                     n_children)
{
  if (n_children)
    {
      GVariantSerialised child = { 0, };

      gvs_filler (&child, children[0]);

      return child.size + 1;
    }
  else
    return 0;
}

static void
gvs_variable_sized_maybe_serialise (GVariantSerialised        value,
                                    GVariantSerialisedFiller  gvs_filler,
                                    const xpointer_t           *children,
                                    xsize_t                     n_children)
{
  if (n_children)
    {
      GVariantSerialised child = { NULL, value.data, value.size - 1, value.depth + 1 };

      /* write the data for the child.  */
      gvs_filler (&child, children[0]);
      value.data[child.size] = '\0';
    }
}

static xboolean_t
gvs_variable_sized_maybe_is_normal (GVariantSerialised value)
{
  if (value.size == 0)
    return TRUE;

  if (value.data[value.size - 1] != '\0')
    return FALSE;

  value.type_info = xvariant_type_info_element (value.type_info);
  value.size--;
  value.depth++;

  return xvariant_serialised_is_normal (value);
}

/* Arrays {{{2
 *
 * Just as with maybe types, array types are handled depending on if the
 * element type of the array type is a fixed-sized or variable-sized
 * type.  Similar to maybe types, for convenience, an array value with a
 * fixed-sized element type is called a "fixed-sized array" and an array
 * value with a variable-sized element type is called a "variable sized
 * array".
 */

/* Fixed-sized Array {{{3
 *
 * For fixed sized arrays, the serialized data is simply a concatenation
 * of the serialized data of each element, in order.  Since fixed-sized
 * values always have a fixed size that is a multiple of their alignment
 * requirement no extra padding is required.
 *
 * In the event that a fixed-sized array is presented with a size that
 * is not an integer multiple of the element size then the value of the
 * array must be taken as being empty.
 */

static xsize_t
gvs_fixed_sized_array_n_children (GVariantSerialised value)
{
  xsize_t element_fixed_size;

  xvariant_type_info_query_element (value.type_info, NULL,
                                     &element_fixed_size);

  if (value.size % element_fixed_size == 0)
    return value.size / element_fixed_size;

  return 0;
}

static GVariantSerialised
gvs_fixed_sized_array_get_child (GVariantSerialised value,
                                 xsize_t              index_)
{
  GVariantSerialised child = { 0, };

  child.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_query (child.type_info, NULL, &child.size);
  child.data = value.data + (child.size * index_);
  xvariant_type_info_ref (child.type_info);
  child.depth = value.depth + 1;

  return child;
}

static xsize_t
gvs_fixed_sized_array_needed_size (GVariantTypeInfo         *type_info,
                                   GVariantSerialisedFiller  gvs_filler,
                                   const xpointer_t           *children,
                                   xsize_t                     n_children)
{
  xsize_t element_fixed_size;

  xvariant_type_info_query_element (type_info, NULL, &element_fixed_size);

  return element_fixed_size * n_children;
}

static void
gvs_fixed_sized_array_serialise (GVariantSerialised        value,
                                 GVariantSerialisedFiller  gvs_filler,
                                 const xpointer_t           *children,
                                 xsize_t                     n_children)
{
  GVariantSerialised child = { 0, };
  xsize_t i;

  child.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_query (child.type_info, NULL, &child.size);
  child.data = value.data;
  child.depth = value.depth + 1;

  for (i = 0; i < n_children; i++)
    {
      gvs_filler (&child, children[i]);
      child.data += child.size;
    }
}

static xboolean_t
gvs_fixed_sized_array_is_normal (GVariantSerialised value)
{
  GVariantSerialised child = { 0, };

  child.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_query (child.type_info, NULL, &child.size);
  child.depth = value.depth + 1;

  if (value.size % child.size != 0)
    return FALSE;

  for (child.data = value.data;
       child.data < value.data + value.size;
       child.data += child.size)
    {
      if (!xvariant_serialised_is_normal (child))
        return FALSE;
    }

  return TRUE;
}

/* Variable-sized Array {{{3
 *
 * Variable sized arrays, containing variable-sized elements, must be
 * able to determine the boundaries between the elements.  The items
 * cannot simply be concatenated.  Additionally, we are faced with the
 * fact that non-fixed-sized values do not necessarily have a size that
 * is a multiple of their alignment requirement, so we may need to
 * insert zero-filled padding.
 *
 * While it is possible to find the start of an item by starting from
 * the end of the item before it and padding for alignment, it is not
 * generally possible to do the reverse operation.  For this reason, we
 * record the end point of each element in the array.
 *
 * xvariant_t works in terms of "offsets".  An offset is a pointer to a
 * boundary between two bytes.  In 4 bytes of serialized data, there
 * would be 5 possible offsets: one at the start ('0'), one between each
 * pair of adjacent bytes ('1', '2', '3') and one at the end ('4').
 *
 * The numeric value of an offset is an unsigned integer given relative
 * to the start of the serialized data of the array.  Offsets are always
 * stored in little endian byte order and are always only as big as they
 * need to be.  For example, in 255 bytes of serialized data, there are
 * 256 offsets.  All possibilities can be stored in an 8 bit unsigned
 * integer.  In 256 bytes of serialized data, however, there are 257
 * possible offsets so 16 bit integers must be used.  The size of an
 * offset is always a power of 2.
 *
 * The offsets are stored at the end of the serialized data of the
 * array.  They are simply concatenated on without any particular
 * alignment.  The size of the offsets is included in the size of the
 * serialized data for purposes of determining the size of the offsets.
 * This presents a possibly ambiguity; in certain cases, a particular
 * value of array could have two different serialized forms.
 *
 * Imagine an array containing a single string of 253 bytes in length
 * (so, 254 bytes including the nul terminator).  Now the offset must be
 * written.  If an 8 bit offset is written, it will bring the size of
 * the array's serialized data to 255 -- which means that the use of an
 * 8 bit offset was valid.  If a 16 bit offset is used then the total
 * size of the array will be 256 -- which means that the use of a 16 bit
 * offset was valid.  Although both of these will be accepted by the
 * deserializer, only the smaller of the two is considered to be in
 * normal form and that is the one that the serializer must produce.
 */

/* bytes may be NULL if (size == 0). */
static inline xsize_t
gvs_read_unaligned_le (xuchar_t *bytes,
                       xuint_t   size)
{
  union
  {
    xuchar_t bytes[XPL_SIZEOF_SIZE_T];
    xsize_t integer;
  } tmpvalue;

  tmpvalue.integer = 0;
  if (bytes != NULL)
    memcpy (&tmpvalue.bytes, bytes, size);

  return GSIZE_FROM_LE (tmpvalue.integer);
}

static inline void
gvs_write_unaligned_le (xuchar_t *bytes,
                        xsize_t   value,
                        xuint_t   size)
{
  union
  {
    xuchar_t bytes[XPL_SIZEOF_SIZE_T];
    xsize_t integer;
  } tmpvalue;

  tmpvalue.integer = GSIZE_TO_LE (value);
  memcpy (bytes, &tmpvalue.bytes, size);
}

static xuint_t
gvs_get_offset_size (xsize_t size)
{
  if (size > G_MAXUINT32)
    return 8;

  else if (size > G_MAXUINT16)
    return 4;

  else if (size > G_MAXUINT8)
    return 2;

  else if (size > 0)
    return 1;

  return 0;
}

static xsize_t
gvs_calculate_total_size (xsize_t body_size,
                          xsize_t offsets)
{
  if (body_size + 1 * offsets <= G_MAXUINT8)
    return body_size + 1 * offsets;

  if (body_size + 2 * offsets <= G_MAXUINT16)
    return body_size + 2 * offsets;

  if (body_size + 4 * offsets <= G_MAXUINT32)
    return body_size + 4 * offsets;

  return body_size + 8 * offsets;
}

static xsize_t
gvs_variable_sized_array_n_children (GVariantSerialised value)
{
  xsize_t offsets_array_size;
  xsize_t offset_size;
  xsize_t last_end;

  if (value.size == 0)
    return 0;

  offset_size = gvs_get_offset_size (value.size);

  last_end = gvs_read_unaligned_le (value.data + value.size -
                                    offset_size, offset_size);

  if (last_end > value.size)
    return 0;

  offsets_array_size = value.size - last_end;

  if (offsets_array_size % offset_size)
    return 0;

  return offsets_array_size / offset_size;
}

static GVariantSerialised
gvs_variable_sized_array_get_child (GVariantSerialised value,
                                    xsize_t              index_)
{
  GVariantSerialised child = { 0, };
  xsize_t offset_size;
  xsize_t last_end;
  xsize_t start;
  xsize_t end;

  child.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_ref (child.type_info);
  child.depth = value.depth + 1;

  offset_size = gvs_get_offset_size (value.size);

  last_end = gvs_read_unaligned_le (value.data + value.size -
                                    offset_size, offset_size);

  if (index_ > 0)
    {
      xuint_t alignment;

      start = gvs_read_unaligned_le (value.data + last_end +
                                     (offset_size * (index_ - 1)),
                                     offset_size);

      xvariant_type_info_query (child.type_info, &alignment, NULL);
      start += (-start) & alignment;
    }
  else
    start = 0;

  end = gvs_read_unaligned_le (value.data + last_end +
                               (offset_size * index_),
                               offset_size);

  if (start < end && end <= value.size && end <= last_end)
    {
      child.data = value.data + start;
      child.size = end - start;
    }

  return child;
}

static xsize_t
gvs_variable_sized_array_needed_size (GVariantTypeInfo         *type_info,
                                      GVariantSerialisedFiller  gvs_filler,
                                      const xpointer_t           *children,
                                      xsize_t                     n_children)
{
  xuint_t alignment;
  xsize_t offset;
  xsize_t i;

  xvariant_type_info_query (type_info, &alignment, NULL);
  offset = 0;

  for (i = 0; i < n_children; i++)
    {
      GVariantSerialised child = { 0, };

      offset += (-offset) & alignment;
      gvs_filler (&child, children[i]);
      offset += child.size;
    }

  return gvs_calculate_total_size (offset, n_children);
}

static void
gvs_variable_sized_array_serialise (GVariantSerialised        value,
                                    GVariantSerialisedFiller  gvs_filler,
                                    const xpointer_t           *children,
                                    xsize_t                     n_children)
{
  xuchar_t *offset_ptr;
  xsize_t offset_size;
  xuint_t alignment;
  xsize_t offset;
  xsize_t i;

  xvariant_type_info_query (value.type_info, &alignment, NULL);
  offset_size = gvs_get_offset_size (value.size);
  offset = 0;

  offset_ptr = value.data + value.size - offset_size * n_children;

  for (i = 0; i < n_children; i++)
    {
      GVariantSerialised child = { 0, };

      while (offset & alignment)
        value.data[offset++] = '\0';

      child.data = value.data + offset;
      gvs_filler (&child, children[i]);
      offset += child.size;

      gvs_write_unaligned_le (offset_ptr, offset, offset_size);
      offset_ptr += offset_size;
    }
}

static xboolean_t
gvs_variable_sized_array_is_normal (GVariantSerialised value)
{
  GVariantSerialised child = { 0, };
  xsize_t offsets_array_size;
  xuchar_t *offsets_array;
  xuint_t offset_size;
  xuint_t alignment;
  xsize_t last_end;
  xsize_t length;
  xsize_t offset;
  xsize_t i;

  if (value.size == 0)
    return TRUE;

  offset_size = gvs_get_offset_size (value.size);
  last_end = gvs_read_unaligned_le (value.data + value.size -
                                    offset_size, offset_size);

  if (last_end > value.size)
    return FALSE;

  offsets_array_size = value.size - last_end;

  if (offsets_array_size % offset_size)
    return FALSE;

  offsets_array = value.data + value.size - offsets_array_size;
  length = offsets_array_size / offset_size;

  if (length == 0)
    return FALSE;

  child.type_info = xvariant_type_info_element (value.type_info);
  xvariant_type_info_query (child.type_info, &alignment, NULL);
  child.depth = value.depth + 1;
  offset = 0;

  for (i = 0; i < length; i++)
    {
      xsize_t this_end;

      this_end = gvs_read_unaligned_le (offsets_array + offset_size * i,
                                        offset_size);

      if (this_end < offset || this_end > last_end)
        return FALSE;

      while (offset & alignment)
        {
          if (!(offset < this_end && value.data[offset] == '\0'))
            return FALSE;
          offset++;
        }

      child.data = value.data + offset;
      child.size = this_end - offset;

      if (child.size == 0)
        child.data = NULL;

      if (!xvariant_serialised_is_normal (child))
        return FALSE;

      offset = this_end;
    }

  xassert (offset == last_end);

  return TRUE;
}

/* Tuples {{{2
 *
 * Since tuples can contain a mix of variable- and fixed-sized items,
 * they are, in terms of serialization, a hybrid of variable-sized and
 * fixed-sized arrays.
 *
 * Offsets are only stored for variable-sized items.  Also, since the
 * number of items in a tuple is known from its type, we are able to
 * know exactly how many offsets to expect in the serialized data (and
 * therefore how much space is taken up by the offset array).  This
 * means that we know where the end of the serialized data for the last
 * item is -- we can just subtract the size of the offset array from the
 * total size of the tuple.  For this reason, the last item in the tuple
 * doesn't need an offset stored.
 *
 * Tuple offsets are stored in reverse.  This design choice allows
 * iterator-based deserializers to be more efficient.
 *
 * Most of the "heavy lifting" here is handled by the GVariantTypeInfo
 * for the tuple.  See the notes in gvarianttypeinfo.h.
 */

static xsize_t
gvs_tuple_n_children (GVariantSerialised value)
{
  return xvariant_type_info_n_members (value.type_info);
}

static GVariantSerialised
gvs_tuple_get_child (GVariantSerialised value,
                     xsize_t              index_)
{
  const GVariantMemberInfo *member_info;
  GVariantSerialised child = { 0, };
  xsize_t offset_size;
  xsize_t start, end, last_end;

  member_info = xvariant_type_info_member_info (value.type_info, index_);
  child.type_info = xvariant_type_info_ref (member_info->type_info);
  child.depth = value.depth + 1;
  offset_size = gvs_get_offset_size (value.size);

  /* tuples are the only (potentially) fixed-sized containers, so the
   * only ones that have to deal with the possibility of having %NULL
   * data with a non-zero %size if errors occurred elsewhere.
   */
  if G_UNLIKELY (value.data == NULL && value.size != 0)
    {
      xvariant_type_info_query (child.type_info, NULL, &child.size);

      /* this can only happen in fixed-sized tuples,
       * so the child must also be fixed sized.
       */
      xassert (child.size != 0);
      child.data = NULL;

      return child;
    }

  if (member_info->ending_type == G_VARIANT_MEMBER_ENDING_OFFSET)
    {
      if (offset_size * (member_info->i + 2) > value.size)
        return child;
    }
  else
    {
      if (offset_size * (member_info->i + 1) > value.size)
        {
          /* if the child is fixed size, return its size.
           * if child is not fixed-sized, return size = 0.
           */
          xvariant_type_info_query (child.type_info, NULL, &child.size);

          return child;
        }
    }

  if (member_info->i + 1)
    start = gvs_read_unaligned_le (value.data + value.size -
                                   offset_size * (member_info->i + 1),
                                   offset_size);
  else
    start = 0;

  start += member_info->a;
  start &= member_info->b;
  start |= member_info->c;

  if (member_info->ending_type == G_VARIANT_MEMBER_ENDING_LAST)
    end = value.size - offset_size * (member_info->i + 1);

  else if (member_info->ending_type == G_VARIANT_MEMBER_ENDING_FIXED)
    {
      xsize_t fixed_size;

      xvariant_type_info_query (child.type_info, NULL, &fixed_size);
      end = start + fixed_size;
      child.size = fixed_size;
    }

  else /* G_VARIANT_MEMBER_ENDING_OFFSET */
    end = gvs_read_unaligned_le (value.data + value.size -
                                 offset_size * (member_info->i + 2),
                                 offset_size);

  /* The child should not extend into the offset table. */
  if (index_ != xvariant_type_info_n_members (value.type_info) - 1)
    {
      GVariantSerialised last_child;
      last_child = gvs_tuple_get_child (value,
                                        xvariant_type_info_n_members (value.type_info) - 1);
      last_end = last_child.data + last_child.size - value.data;
      xvariant_type_info_unref (last_child.type_info);
    }
  else
    last_end = end;

  if (start < end && end <= value.size && end <= last_end)
    {
      child.data = value.data + start;
      child.size = end - start;
    }

  return child;
}

static xsize_t
gvs_tuple_needed_size (GVariantTypeInfo         *type_info,
                       GVariantSerialisedFiller  gvs_filler,
                       const xpointer_t           *children,
                       xsize_t                     n_children)
{
  const GVariantMemberInfo *member_info = NULL;
  xsize_t fixed_size;
  xsize_t offset;
  xsize_t i;

  xvariant_type_info_query (type_info, NULL, &fixed_size);

  if (fixed_size)
    return fixed_size;

  offset = 0;

  for (i = 0; i < n_children; i++)
    {
      xuint_t alignment;

      member_info = xvariant_type_info_member_info (type_info, i);
      xvariant_type_info_query (member_info->type_info,
                                 &alignment, &fixed_size);
      offset += (-offset) & alignment;

      if (fixed_size)
        offset += fixed_size;
      else
        {
          GVariantSerialised child = { 0, };

          gvs_filler (&child, children[i]);
          offset += child.size;
        }
    }

  return gvs_calculate_total_size (offset, member_info->i + 1);
}

static void
gvs_tuple_serialise (GVariantSerialised        value,
                     GVariantSerialisedFiller  gvs_filler,
                     const xpointer_t           *children,
                     xsize_t                     n_children)
{
  xsize_t offset_size;
  xsize_t offset;
  xsize_t i;

  offset_size = gvs_get_offset_size (value.size);
  offset = 0;

  for (i = 0; i < n_children; i++)
    {
      const GVariantMemberInfo *member_info;
      GVariantSerialised child = { 0, };
      xuint_t alignment;

      member_info = xvariant_type_info_member_info (value.type_info, i);
      xvariant_type_info_query (member_info->type_info, &alignment, NULL);

      while (offset & alignment)
        value.data[offset++] = '\0';

      child.data = value.data + offset;
      gvs_filler (&child, children[i]);
      offset += child.size;

      if (member_info->ending_type == G_VARIANT_MEMBER_ENDING_OFFSET)
        {
          value.size -= offset_size;
          gvs_write_unaligned_le (value.data + value.size,
                                  offset, offset_size);
        }
    }

  while (offset < value.size)
    value.data[offset++] = '\0';
}

static xboolean_t
gvs_tuple_is_normal (GVariantSerialised value)
{
  xuint_t offset_size;
  xsize_t offset_ptr;
  xsize_t length;
  xsize_t offset;
  xsize_t i;

  /* as per the comment in gvs_tuple_get_child() */
  if G_UNLIKELY (value.data == NULL && value.size != 0)
    return FALSE;

  offset_size = gvs_get_offset_size (value.size);
  length = xvariant_type_info_n_members (value.type_info);
  offset_ptr = value.size;
  offset = 0;

  for (i = 0; i < length; i++)
    {
      const GVariantMemberInfo *member_info;
      GVariantSerialised child;
      xsize_t fixed_size;
      xuint_t alignment;
      xsize_t end;

      member_info = xvariant_type_info_member_info (value.type_info, i);
      child.type_info = member_info->type_info;
      child.depth = value.depth + 1;

      xvariant_type_info_query (child.type_info, &alignment, &fixed_size);

      while (offset & alignment)
        {
          if (offset > value.size || value.data[offset] != '\0')
            return FALSE;
          offset++;
        }

      child.data = value.data + offset;

      switch (member_info->ending_type)
        {
        case G_VARIANT_MEMBER_ENDING_FIXED:
          end = offset + fixed_size;
          break;

        case G_VARIANT_MEMBER_ENDING_LAST:
          end = offset_ptr;
          break;

        case G_VARIANT_MEMBER_ENDING_OFFSET:
          if (offset_ptr < offset_size)
            return FALSE;

          offset_ptr -= offset_size;

          if (offset_ptr < offset)
            return FALSE;

          end = gvs_read_unaligned_le (value.data + offset_ptr, offset_size);
          break;

        default:
          g_assert_not_reached ();
        }

      if (end < offset || end > offset_ptr)
        return FALSE;

      child.size = end - offset;

      if (child.size == 0)
        child.data = NULL;

      if (!xvariant_serialised_is_normal (child))
        return FALSE;

      offset = end;
    }

  {
    xsize_t fixed_size;
    xuint_t alignment;

    xvariant_type_info_query (value.type_info, &alignment, &fixed_size);

    if (fixed_size)
      {
        xassert (fixed_size == value.size);
        xassert (offset_ptr == value.size);

        if (i == 0)
          {
            if (value.data[offset++] != '\0')
              return FALSE;
          }
        else
          {
            while (offset & alignment)
              if (value.data[offset++] != '\0')
                return FALSE;
          }

        xassert (offset == value.size);
      }
  }

  return offset_ptr == offset;
}

/* Variants {{{2
 *
 * Variants are stored by storing the serialized data of the child,
 * followed by a '\0' character, followed by the type string of the
 * child.
 *
 * In the case that a value is presented that contains no '\0'
 * character, or doesn't have a single well-formed definite type string
 * following that character, the variant must be taken as containing the
 * unit tuple: ().
 */

static inline xsize_t
gvs_variant_n_children (GVariantSerialised value)
{
  return 1;
}

static inline GVariantSerialised
gvs_variant_get_child (GVariantSerialised value,
                       xsize_t              index_)
{
  GVariantSerialised child = { 0, };

  /* NOTE: not O(1) and impossible for it to be... */
  if (value.size)
    {
      /* find '\0' character */
      for (child.size = value.size - 1; child.size; child.size--)
        if (value.data[child.size] == '\0')
          break;

      /* ensure we didn't just hit the start of the string */
      if (value.data[child.size] == '\0')
        {
          const xchar_t *type_string = (xchar_t *) &value.data[child.size + 1];
          const xchar_t *limit = (xchar_t *) &value.data[value.size];
          const xchar_t *end;

          if (xvariant_type_string_scan (type_string, limit, &end) &&
              end == limit)
            {
              const xvariant_type_t *type = (xvariant_type_t *) type_string;

              if (xvariant_type_is_definite (type))
                {
                  xsize_t fixed_size;
                  xsize_t child_type_depth;

                  child.type_info = xvariant_type_info_get (type);
                  child.depth = value.depth + 1;

                  if (child.size != 0)
                    /* only set to non-%NULL if size > 0 */
                    child.data = value.data;

                  xvariant_type_info_query (child.type_info,
                                             NULL, &fixed_size);
                  child_type_depth = xvariant_type_info_query_depth (child.type_info);

                  if ((!fixed_size || fixed_size == child.size) &&
                      value.depth < G_VARIANT_MAX_RECURSION_DEPTH - child_type_depth)
                    return child;

                  xvariant_type_info_unref (child.type_info);
                }
            }
        }
    }

  child.type_info = xvariant_type_info_get (G_VARIANT_TYPE_UNIT);
  child.data = NULL;
  child.size = 1;
  child.depth = value.depth + 1;

  return child;
}

static inline xsize_t
gvs_variant_needed_size (GVariantTypeInfo         *type_info,
                         GVariantSerialisedFiller  gvs_filler,
                         const xpointer_t           *children,
                         xsize_t                     n_children)
{
  GVariantSerialised child = { 0, };
  const xchar_t *type_string;

  gvs_filler (&child, children[0]);
  type_string = xvariant_type_info_get_type_string (child.type_info);

  return child.size + 1 + strlen (type_string);
}

static inline void
gvs_variant_serialise (GVariantSerialised        value,
                       GVariantSerialisedFiller  gvs_filler,
                       const xpointer_t           *children,
                       xsize_t                     n_children)
{
  GVariantSerialised child = { 0, };
  const xchar_t *type_string;

  child.data = value.data;

  gvs_filler (&child, children[0]);
  type_string = xvariant_type_info_get_type_string (child.type_info);
  value.data[child.size] = '\0';
  memcpy (value.data + child.size + 1, type_string, strlen (type_string));
}

static inline xboolean_t
gvs_variant_is_normal (GVariantSerialised value)
{
  GVariantSerialised child;
  xboolean_t normal;
  xsize_t child_type_depth;

  child = gvs_variant_get_child (value, 0);
  child_type_depth = xvariant_type_info_query_depth (child.type_info);

  normal = (value.depth < G_VARIANT_MAX_RECURSION_DEPTH - child_type_depth) &&
           (child.data != NULL || child.size == 0) &&
           xvariant_serialised_is_normal (child);

  xvariant_type_info_unref (child.type_info);

  return normal;
}



/* PART 2: Serializer API {{{1
 *
 * This is the implementation of the API of the serializer as advertised
 * in gvariant-serialiser.h.
 */

/* Dispatch Utilities {{{2
 *
 * These macros allow a given function (for example,
 * xvariant_serialiser_serialise) to be dispatched to the appropriate
 * type-specific function above (fixed/variable-sized maybe,
 * fixed/variable-sized array, tuple or variant).
 */
#define DISPATCH_FIXED(type_info, before, after) \
  {                                                     \
    xsize_t fixed_size;                                   \
                                                        \
    xvariant_type_info_query_element (type_info, NULL, \
                                       &fixed_size);    \
                                                        \
    if (fixed_size)                                     \
      {                                                 \
        before ## fixed_sized ## after                  \
      }                                                 \
    else                                                \
      {                                                 \
        before ## variable_sized ## after               \
      }                                                 \
  }

#define DISPATCH_CASES(type_info, before, after) \
  switch (xvariant_type_info_get_type_char (type_info))        \
    {                                                           \
      case G_VARIANT_TYPE_INFO_CHAR_MAYBE:                      \
        DISPATCH_FIXED (type_info, before, _maybe ## after)     \
                                                                \
      case G_VARIANT_TYPE_INFO_CHAR_ARRAY:                      \
        DISPATCH_FIXED (type_info, before, _array ## after)     \
                                                                \
      case G_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY:                 \
      case G_VARIANT_TYPE_INFO_CHAR_TUPLE:                      \
        {                                                       \
          before ## tuple ## after                              \
        }                                                       \
                                                                \
      case G_VARIANT_TYPE_INFO_CHAR_VARIANT:                    \
        {                                                       \
          before ## variant ## after                            \
        }                                                       \
    }

/* Serializer entry points {{{2
 *
 * These are the functions that are called in order for the serializer
 * to do its thing.
 */

/* < private >
 * xvariant_serialised_n_children:
 * @serialised: a #GVariantSerialised
 *
 * For serialized data that represents a container value (maybes,
 * tuples, arrays, variants), determine how many child items are inside
 * that container.
 *
 * Returns: the number of children
 */
xsize_t
xvariant_serialised_n_children (GVariantSerialised serialised)
{
  xassert (xvariant_serialised_check (serialised));

  DISPATCH_CASES (serialised.type_info,

                  return gvs_/**/,/**/_n_children (serialised);

                 )
  g_assert_not_reached ();
}

/* < private >
 * xvariant_serialised_get_child:
 * @serialised: a #GVariantSerialised
 * @index_: the index of the child to fetch
 *
 * Extracts a child from a serialized data representing a container
 * value.
 *
 * It is an error to call this function with an index out of bounds.
 *
 * If the result .data == %NULL and .size > 0 then there has been an
 * error extracting the requested fixed-sized value.  This number of
 * zero bytes needs to be allocated instead.
 *
 * In the case that .data == %NULL and .size == 0 then a zero-sized
 * item of a variable-sized type is being returned.
 *
 * .data is never non-%NULL if size is 0.
 *
 * Returns: a #GVariantSerialised for the child
 */
GVariantSerialised
xvariant_serialised_get_child (GVariantSerialised serialised,
                                xsize_t              index_)
{
  GVariantSerialised child;

  xassert (xvariant_serialised_check (serialised));

  if G_LIKELY (index_ < xvariant_serialised_n_children (serialised))
    {
      DISPATCH_CASES (serialised.type_info,

                      child = gvs_/**/,/**/_get_child (serialised, index_);
                      xassert (child.size || child.data == NULL);
                      xassert (xvariant_serialised_check (child));
                      return child;

                     )
      g_assert_not_reached ();
    }

  xerror ("Attempt to access item %"G_GSIZE_FORMAT
           " in a container with only %"G_GSIZE_FORMAT" items",
           index_, xvariant_serialised_n_children (serialised));
}

/* < private >
 * xvariant_serialiser_serialise:
 * @serialised: a #GVariantSerialised, properly set up
 * @gvs_filler: the filler function
 * @children: an array of child items
 * @n_children: the size of @children
 *
 * Writes data in serialized form.
 *
 * The type_info field of @serialised must be filled in to type info for
 * the type that we are serializing.
 *
 * The size field of @serialised must be filled in with the value
 * returned by a previous call to xvariant_serialiser_needed_size().
 *
 * The data field of @serialised must be a pointer to a properly-aligned
 * memory region large enough to serialize into (ie: at least as big as
 * the size field).
 *
 * This function is only resonsible for serializing the top-level
 * container.  @gvs_filler is called on each child of the container in
 * order for all of the data of that child to be filled in.
 */
void
xvariant_serialiser_serialise (GVariantSerialised        serialised,
                                GVariantSerialisedFiller  gvs_filler,
                                const xpointer_t           *children,
                                xsize_t                     n_children)
{
  xassert (xvariant_serialised_check (serialised));

  DISPATCH_CASES (serialised.type_info,

                  gvs_/**/,/**/_serialise (serialised, gvs_filler,
                                           children, n_children);
                  return;

                 )
  g_assert_not_reached ();
}

/* < private >
 * xvariant_serialiser_needed_size:
 * @type_info: the type to serialize for
 * @gvs_filler: the filler function
 * @children: an array of child items
 * @n_children: the size of @children
 *
 * Determines how much memory would be needed to serialize this value.
 *
 * This function is only resonsible for performing calculations for the
 * top-level container.  @gvs_filler is called on each child of the
 * container in order to determine its size.
 */
xsize_t
xvariant_serialiser_needed_size (GVariantTypeInfo         *type_info,
                                  GVariantSerialisedFiller  gvs_filler,
                                  const xpointer_t           *children,
                                  xsize_t                     n_children)
{
  DISPATCH_CASES (type_info,

                  return gvs_/**/,/**/_needed_size (type_info, gvs_filler,
                                                    children, n_children);

                 )
  g_assert_not_reached ();
}

/* Byteswapping {{{2 */

/* < private >
 * xvariant_serialised_byteswap:
 * @value: a #GVariantSerialised
 *
 * Byte-swap serialized data.  The result of this function is only
 * well-defined if the data is in normal form.
 */
void
xvariant_serialised_byteswap (GVariantSerialised serialised)
{
  xsize_t fixed_size;
  xuint_t alignment;

  xassert (xvariant_serialised_check (serialised));

  if (!serialised.data)
    return;

  /* the types we potentially need to byteswap are
   * exactly those with alignment requirements.
   */
  xvariant_type_info_query (serialised.type_info, &alignment, &fixed_size);
  if (!alignment)
    return;

  /* if fixed size and alignment are equal then we are down
   * to the base integer type and we should swap it.  the
   * only exception to this is if we have a tuple with a
   * single item, and then swapping it will be OK anyway.
   */
  if (alignment + 1 == fixed_size)
    {
      switch (fixed_size)
      {
        case 2:
          {
            xuint16_t *ptr = (xuint16_t *) serialised.data;

            g_assert_cmpint (serialised.size, ==, 2);
            *ptr = GUINT16_SWAP_LE_BE (*ptr);
          }
          return;

        case 4:
          {
            xuint32_t *ptr = (xuint32_t *) serialised.data;

            g_assert_cmpint (serialised.size, ==, 4);
            *ptr = GUINT32_SWAP_LE_BE (*ptr);
          }
          return;

        case 8:
          {
            xuint64_t *ptr = (xuint64_t *) serialised.data;

            g_assert_cmpint (serialised.size, ==, 8);
            *ptr = GUINT64_SWAP_LE_BE (*ptr);
          }
          return;

        default:
          g_assert_not_reached ();
      }
    }

  /* else, we have a container that potentially contains
   * some children that need to be byteswapped.
   */
  else
    {
      xsize_t children, i;

      children = xvariant_serialised_n_children (serialised);
      for (i = 0; i < children; i++)
        {
          GVariantSerialised child;

          child = xvariant_serialised_get_child (serialised, i);
          xvariant_serialised_byteswap (child);
          xvariant_type_info_unref (child.type_info);
        }
    }
}

/* Normal form checking {{{2 */

/* < private >
 * xvariant_serialised_is_normal:
 * @serialised: a #GVariantSerialised
 *
 * Determines, recursively if @serialised is in normal form.  There is
 * precisely one normal form of serialized data for each possible value.
 *
 * It is possible that multiple byte sequences form the serialized data
 * for a given value if, for example, the padding bytes are filled in
 * with something other than zeros, but only one form is the normal
 * form.
 */
xboolean_t
xvariant_serialised_is_normal (GVariantSerialised serialised)
{
  if (serialised.depth >= G_VARIANT_MAX_RECURSION_DEPTH)
    return FALSE;

  DISPATCH_CASES (serialised.type_info,

                  return gvs_/**/,/**/_is_normal (serialised);

                 )

  if (serialised.data == NULL)
    return FALSE;

  /* some hard-coded terminal cases */
  switch (xvariant_type_info_get_type_char (serialised.type_info))
    {
    case 'b': /* boolean */
      return serialised.data[0] < 2;

    case 's': /* string */
      return xvariant_serialiser_is_string (serialised.data,
                                             serialised.size);

    case 'o':
      return xvariant_serialiser_is_object_path (serialised.data,
                                                  serialised.size);

    case 'g':
      return xvariant_serialiser_is_signature (serialised.data,
                                                serialised.size);

    default:
      /* all of the other types are fixed-sized numerical types for
       * which all possible values are valid (including various NaN
       * representations for floating point values).
       */
      return TRUE;
    }
}

/* Validity-checking functions {{{2
 *
 * Checks if strings, object paths and signature strings are valid.
 */

/* < private >
 * xvariant_serialiser_is_string:
 * @data: a possible string
 * @size: the size of @data
 *
 * Ensures that @data is a valid string with a nul terminator at the end
 * and no nul bytes embedded.
 */
xboolean_t
xvariant_serialiser_is_string (xconstpointer data,
                                xsize_t         size)
{
  const xchar_t *expected_end;
  const xchar_t *end;

  /* Strings must end with a nul terminator. */
  if (size == 0)
    return FALSE;

  expected_end = ((xchar_t *) data) + size - 1;

  if (*expected_end != '\0')
    return FALSE;

  xutf8_validate_len (data, size, &end);

  return end == expected_end;
}

/* < private >
 * xvariant_serialiser_is_object_path:
 * @data: a possible D-Bus object path
 * @size: the size of @data
 *
 * Performs the checks for being a valid string.
 *
 * Also, ensures that @data is a valid D-Bus object path, as per the D-Bus
 * specification.
 */
xboolean_t
xvariant_serialiser_is_object_path (xconstpointer data,
                                     xsize_t         size)
{
  const xchar_t *string = data;
  xsize_t i;

  if (!xvariant_serialiser_is_string (data, size))
    return FALSE;

  /* The path must begin with an ASCII '/' (integer 47) character */
  if (string[0] != '/')
    return FALSE;

  for (i = 1; string[i]; i++)
    /* Each element must only contain the ASCII characters
     * "[A-Z][a-z][0-9]_"
     */
    if (g_ascii_isalnum (string[i]) || string[i] == '_')
      ;

    /* must consist of elements separated by slash characters. */
    else if (string[i] == '/')
      {
        /* No element may be the empty string. */
        /* Multiple '/' characters cannot occur in sequence. */
        if (string[i - 1] == '/')
          return FALSE;
      }

    else
      return FALSE;

  /* A trailing '/' character is not allowed unless the path is the
   * root path (a single '/' character).
   */
  if (i > 1 && string[i - 1] == '/')
    return FALSE;

  return TRUE;
}

/* < private >
 * xvariant_serialiser_is_signature:
 * @data: a possible D-Bus signature
 * @size: the size of @data
 *
 * Performs the checks for being a valid string.
 *
 * Also, ensures that @data is a valid D-Bus type signature, as per the
 * D-Bus specification. Note that this means the empty string is valid, as the
 * D-Bus specification defines a signature as “zero or more single complete
 * types”.
 */
xboolean_t
xvariant_serialiser_is_signature (xconstpointer data,
                                   xsize_t         size)
{
  const xchar_t *string = data;
  xsize_t first_invalid;

  if (!xvariant_serialiser_is_string (data, size))
    return FALSE;

  /* make sure no non-definite characters appear */
  first_invalid = strspn (string, "ybnqiuxthdvasog(){}");
  if (string[first_invalid])
    return FALSE;

  /* make sure each type string is well-formed */
  while (*string)
    if (!xvariant_type_string_scan (string, NULL, &string))
      return FALSE;

  return TRUE;
}

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
