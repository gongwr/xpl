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

#include <glib/gvariant-serialiser.h>
#include "gvariant-internal.h"
#include <glib/gvariant-core.h>
#include <glib/gtestutils.h>
#include <glib/gstrfuncs.h>
#include <glib/gslice.h>
#include <glib/ghash.h>
#include <glib/gmem.h>

#include <string.h>


/**
 * SECTION:gvariant
 * @title: xvariant_t
 * @short_description: strongly typed value datatype
 * @see_also: xvariant_type_t
 *
 * #xvariant_t is a variant datatype; it can contain one or more values
 * along with information about the type of the values.
 *
 * A #xvariant_t may contain simple types, like an integer, or a boolean value;
 * or complex types, like an array of two strings, or a dictionary of key
 * value pairs. A #xvariant_t is also immutable: once it's been created neither
 * its type nor its content can be modified further.
 *
 * xvariant_t is useful whenever data needs to be serialized, for example when
 * sending method parameters in D-Bus, or when saving settings using xsettings_t.
 *
 * When creating a new #xvariant_t, you pass the data you want to store in it
 * along with a string representing the type of data you wish to pass to it.
 *
 * For instance, if you want to create a #xvariant_t holding an integer value you
 * can use:
 *
 * |[<!-- language="C" -->
 *   xvariant_t *v = xvariant_new ("u", 40);
 * ]|
 *
 * The string "u" in the first argument tells #xvariant_t that the data passed to
 * the constructor (40) is going to be an unsigned integer.
 *
 * More advanced examples of #xvariant_t in use can be found in documentation for
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * The range of possible values is determined by the type.
 *
 * The type system used by #xvariant_t is #xvariant_type_t.
 *
 * #xvariant_t instances always have a type and a value (which are given
 * at construction time).  The type and value of a #xvariant_t instance
 * can never change other than by the #xvariant_t itself being
 * destroyed.  A #xvariant_t cannot contain a pointer.
 *
 * #xvariant_t is reference counted using xvariant_ref() and
 * xvariant_unref().  #xvariant_t also has floating reference counts --
 * see xvariant_ref_sink().
 *
 * #xvariant_t is completely threadsafe.  A #xvariant_t instance can be
 * concurrently accessed in any way from any number of threads without
 * problems.
 *
 * #xvariant_t is heavily optimised for dealing with data in serialized
 * form.  It works particularly well with data located in memory-mapped
 * files.  It can perform nearly all deserialization operations in a
 * small constant time, usually touching only a single memory page.
 * Serialized #xvariant_t data can also be sent over the network.
 *
 * #xvariant_t is largely compatible with D-Bus.  Almost all types of
 * #xvariant_t instances can be sent over D-Bus.  See #xvariant_type_t for
 * exceptions.  (However, #xvariant_t's serialization format is not the same
 * as the serialization format of a D-Bus message body: use #xdbus_message_t,
 * in the gio library, for those.)
 *
 * For space-efficiency, the #xvariant_t serialization format does not
 * automatically include the variant's length, type or endianness,
 * which must either be implied from context (such as knowledge that a
 * particular file format always contains a little-endian
 * %G_VARIANT_TYPE_VARIANT which occupies the whole length of the file)
 * or supplied out-of-band (for instance, a length, type and/or endianness
 * indicator could be placed at the beginning of a file, network message
 * or network stream).
 *
 * A #xvariant_t's size is limited mainly by any lower level operating
 * system constraints, such as the number of bits in #xsize_t.  For
 * example, it is reasonable to have a 2GB file mapped into memory
 * with #xmapped_file_t, and call xvariant_new_from_data() on it.
 *
 * For convenience to C programmers, #xvariant_t features powerful
 * varargs-based value construction and destruction.  This feature is
 * designed to be embedded in other libraries.
 *
 * There is a Python-inspired text language for describing #xvariant_t
 * values.  #xvariant_t includes a printer for this language and a parser
 * with type inferencing.
 *
 * ## Memory Use
 *
 * #xvariant_t tries to be quite efficient with respect to memory use.
 * This section gives a rough idea of how much memory is used by the
 * current implementation.  The information here is subject to change
 * in the future.
 *
 * The memory allocated by #xvariant_t can be grouped into 4 broad
 * purposes: memory for serialized data, memory for the type
 * information cache, buffer management memory and memory for the
 * #xvariant_t structure itself.
 *
 * ## Serialized Data Memory
 *
 * This is the memory that is used for storing xvariant_t data in
 * serialized form.  This is what would be sent over the network or
 * what would end up on disk, not counting any indicator of the
 * endianness, or of the length or type of the top-level variant.
 *
 * The amount of memory required to store a boolean is 1 byte. 16,
 * 32 and 64 bit integers and double precision floating point numbers
 * use their "natural" size.  Strings (including object path and
 * signature strings) are stored with a nul terminator, and as such
 * use the length of the string plus 1 byte.
 *
 * Maybe types use no space at all to represent the null value and
 * use the same amount of space (sometimes plus one byte) as the
 * equivalent non-maybe-typed value to represent the non-null case.
 *
 * Arrays use the amount of space required to store each of their
 * members, concatenated.  Additionally, if the items stored in an
 * array are not of a fixed-size (ie: strings, other arrays, etc)
 * then an additional framing offset is stored for each item.  The
 * size of this offset is either 1, 2 or 4 bytes depending on the
 * overall size of the container.  Additionally, extra padding bytes
 * are added as required for alignment of child values.
 *
 * Tuples (including dictionary entries) use the amount of space
 * required to store each of their members, concatenated, plus one
 * framing offset (as per arrays) for each non-fixed-sized item in
 * the tuple, except for the last one.  Additionally, extra padding
 * bytes are added as required for alignment of child values.
 *
 * Variants use the same amount of space as the item inside of the
 * variant, plus 1 byte, plus the length of the type string for the
 * item inside the variant.
 *
 * As an example, consider a dictionary mapping strings to variants.
 * In the case that the dictionary is empty, 0 bytes are required for
 * the serialization.
 *
 * If we add an item "width" that maps to the int32 value of 500 then
 * we will use 4 byte to store the int32 (so 6 for the variant
 * containing it) and 6 bytes for the string.  The variant must be
 * aligned to 8 after the 6 bytes of the string, so that's 2 extra
 * bytes.  6 (string) + 2 (padding) + 6 (variant) is 14 bytes used
 * for the dictionary entry.  An additional 1 byte is added to the
 * array as a framing offset making a total of 15 bytes.
 *
 * If we add another entry, "title" that maps to a nullable string
 * that happens to have a value of null, then we use 0 bytes for the
 * null value (and 3 bytes for the variant to contain it along with
 * its type string) plus 6 bytes for the string.  Again, we need 2
 * padding bytes.  That makes a total of 6 + 2 + 3 = 11 bytes.
 *
 * We now require extra padding between the two items in the array.
 * After the 14 bytes of the first item, that's 2 bytes required.
 * We now require 2 framing offsets for an extra two
 * bytes. 14 + 2 + 11 + 2 = 29 bytes to encode the entire two-item
 * dictionary.
 *
 * ## Type Information Cache
 *
 * For each xvariant_t type that currently exists in the program a type
 * information structure is kept in the type information cache.  The
 * type information structure is required for rapid deserialization.
 *
 * Continuing with the above example, if a #xvariant_t exists with the
 * type "a{sv}" then a type information struct will exist for
 * "a{sv}", "{sv}", "s", and "v".  Multiple uses of the same type
 * will share the same type information.  Additionally, all
 * single-digit types are stored in read-only static memory and do
 * not contribute to the writable memory footprint of a program using
 * #xvariant_t.
 *
 * Aside from the type information structures stored in read-only
 * memory, there are two forms of type information.  One is used for
 * container types where there is a single element type: arrays and
 * maybe types.  The other is used for container types where there
 * are multiple element types: tuples and dictionary entries.
 *
 * Array type info structures are 6 * sizeof (void *), plus the
 * memory required to store the type string itself.  This means that
 * on 32-bit systems, the cache entry for "a{sv}" would require 30
 * bytes of memory (plus malloc overhead).
 *
 * Tuple type info structures are 6 * sizeof (void *), plus 4 *
 * sizeof (void *) for each item in the tuple, plus the memory
 * required to store the type string itself.  A 2-item tuple, for
 * example, would have a type information structure that consumed
 * writable memory in the size of 14 * sizeof (void *) (plus type
 * string)  This means that on 32-bit systems, the cache entry for
 * "{sv}" would require 61 bytes of memory (plus malloc overhead).
 *
 * This means that in total, for our "a{sv}" example, 91 bytes of
 * type information would be allocated.
 *
 * The type information cache, additionally, uses a #xhashtable_t to
 * store and look up the cached items and stores a pointer to this
 * hash table in static storage.  The hash table is freed when there
 * are zero items in the type cache.
 *
 * Although these sizes may seem large it is important to remember
 * that a program will probably only have a very small number of
 * different types of values in it and that only one type information
 * structure is required for many different values of the same type.
 *
 * ## buffer_t Management Memory
 *
 * #xvariant_t uses an internal buffer management structure to deal
 * with the various different possible sources of serialized data
 * that it uses.  The buffer is responsible for ensuring that the
 * correct call is made when the data is no longer in use by
 * #xvariant_t.  This may involve a g_free() or a g_slice_free() or
 * even xmapped_file_unref().
 *
 * One buffer management structure is used for each chunk of
 * serialized data.  The size of the buffer management structure
 * is 4 * (void *).  On 32-bit systems, that's 16 bytes.
 *
 * ## xvariant_t structure
 *
 * The size of a #xvariant_t structure is 6 * (void *).  On 32-bit
 * systems, that's 24 bytes.
 *
 * #xvariant_t structures only exist if they are explicitly created
 * with API calls.  For example, if a #xvariant_t is constructed out of
 * serialized data for the example given above (with the dictionary)
 * then although there are 9 individual values that comprise the
 * entire dictionary (two keys, two values, two variants containing
 * the values, two dictionary entries, plus the dictionary itself),
 * only 1 #xvariant_t instance exists -- the one referring to the
 * dictionary.
 *
 * If calls are made to start accessing the other values then
 * #xvariant_t instances will exist for those values only for as long
 * as they are in use (ie: until you call xvariant_unref()).  The
 * type information is shared.  The serialized data and the buffer
 * management structure for that serialized data is shared by the
 * child.
 *
 * ## Summary
 *
 * To put the entire example together, for our dictionary mapping
 * strings to variants (with two entries, as given above), we are
 * using 91 bytes of memory for type information, 29 bytes of memory
 * for the serialized data, 16 bytes for buffer management and 24
 * bytes for the #xvariant_t instance, or a total of 160 bytes, plus
 * malloc overhead.  If we were to use xvariant_get_child_value() to
 * access the two dictionary entries, we would use an additional 48
 * bytes.  If we were to have other dictionaries of the same type, we
 * would use more memory for the serialized data and buffer
 * management for those dictionaries, but the type information would
 * be shared.
 */

/* definition of xvariant_t structure is in gvariant-core.c */

/* this is a g_return_val_if_fail() for making
 * sure a (xvariant_t *) has the required type.
 */
#define TYPE_CHECK(value, TYPE, val) \
  if G_UNLIKELY (!xvariant_is_of_type (value, TYPE)) {           \
    g_return_if_fail_warning (G_LOG_DOMAIN, G_STRFUNC,            \
                              "xvariant_is_of_type (" #value     \
                              ", " #TYPE ")");                    \
    return val;                                                   \
  }

/* Numeric Type Constructor/Getters {{{1 */
/* < private >
 * xvariant_new_from_trusted:
 * @type: the #xvariant_type_t
 * @data: the data to use
 * @size: the size of @data
 *
 * Constructs a new trusted #xvariant_t instance from the provided data.
 * This is used to implement xvariant_new_* for all the basic types.
 *
 * Note: @data must be backed by memory that is aligned appropriately for the
 * @type being loaded. Otherwise this function will internally create a copy of
 * the memory (since GLib 2.60) or (in older versions) fail and exit the
 * process.
 *
 * Returns: a new floating #xvariant_t
 */
static xvariant_t *
xvariant_new_from_trusted (const xvariant_type_t *type,
                            xconstpointer       data,
                            xsize_t               size)
{
  xvariant_t *value;
  xbytes_t *bytes;

  bytes = xbytes_new (data, size);
  value = xvariant_new_from_bytes (type, bytes, TRUE);
  xbytes_unref (bytes);

  return value;
}

/**
 * xvariant_new_boolean:
 * @value: a #xboolean_t value
 *
 * Creates a new boolean #xvariant_t instance -- either %TRUE or %FALSE.
 *
 * Returns: (transfer none): a floating reference to a new boolean #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_boolean (xboolean_t value)
{
  xuchar_t v = value;

  return xvariant_new_from_trusted (G_VARIANT_TYPE_BOOLEAN, &v, 1);
}

/**
 * xvariant_get_boolean:
 * @value: a boolean #xvariant_t instance
 *
 * Returns the boolean value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_BOOLEAN.
 *
 * Returns: %TRUE or %FALSE
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_get_boolean (xvariant_t *value)
{
  const xuchar_t *data;

  TYPE_CHECK (value, G_VARIANT_TYPE_BOOLEAN, FALSE);

  data = xvariant_get_data (value);

  return data != NULL ? *data != 0 : FALSE;
}

/* the constructors and accessors for byte, int{16,32,64}, handles and
 * doubles all look pretty much exactly the same, so we reduce
 * copy/pasting here.
 */
#define NUMERIC_TYPE(TYPE, type, ctype) \
  xvariant_t *xvariant_new_##type (ctype value) {                \
    return xvariant_new_from_trusted (G_VARIANT_TYPE_##TYPE,   \
                                       &value, sizeof value);   \
  }                                                             \
  ctype xvariant_get_##type (xvariant_t *value) {                \
    const ctype *data;                                          \
    TYPE_CHECK (value, G_VARIANT_TYPE_ ## TYPE, 0);             \
    data = xvariant_get_data (value);                          \
    return data != NULL ? *data : 0;                            \
  }


/**
 * xvariant_new_byte:
 * @value: a #xuint8_t value
 *
 * Creates a new byte #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new byte #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_byte:
 * @value: a byte #xvariant_t instance
 *
 * Returns the byte value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_BYTE.
 *
 * Returns: a #xuint8_t
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (BYTE, byte, xuint8_t)

/**
 * xvariant_new_int16:
 * @value: a #gint16 value
 *
 * Creates a new int16 #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new int16 #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_int16:
 * @value: an int16 #xvariant_t instance
 *
 * Returns the 16-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_INT16.
 *
 * Returns: a #gint16
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (INT16, int16, gint16)

/**
 * xvariant_new_uint16:
 * @value: a #xuint16_t value
 *
 * Creates a new uint16 #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new uint16 #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_uint16:
 * @value: a uint16 #xvariant_t instance
 *
 * Returns the 16-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_UINT16.
 *
 * Returns: a #xuint16_t
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (UINT16, uint16, xuint16_t)

/**
 * xvariant_new_int32:
 * @value: a #gint32 value
 *
 * Creates a new int32 #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new int32 #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_int32:
 * @value: an int32 #xvariant_t instance
 *
 * Returns the 32-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_INT32.
 *
 * Returns: a #gint32
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (INT32, int32, gint32)

/**
 * xvariant_new_uint32:
 * @value: a #xuint32_t value
 *
 * Creates a new uint32 #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new uint32 #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_uint32:
 * @value: a uint32 #xvariant_t instance
 *
 * Returns the 32-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_UINT32.
 *
 * Returns: a #xuint32_t
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (UINT32, uint32, xuint32_t)

/**
 * xvariant_new_int64:
 * @value: a #sint64_t value
 *
 * Creates a new int64 #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new int64 #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_int64:
 * @value: an int64 #xvariant_t instance
 *
 * Returns the 64-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_INT64.
 *
 * Returns: a #sint64_t
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (INT64, int64, sint64_t)

/**
 * xvariant_new_uint64:
 * @value: a #xuint64_t value
 *
 * Creates a new uint64 #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new uint64 #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_uint64:
 * @value: a uint64 #xvariant_t instance
 *
 * Returns the 64-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_UINT64.
 *
 * Returns: a #xuint64_t
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (UINT64, uint64, xuint64_t)

/**
 * xvariant_new_handle:
 * @value: a #gint32 value
 *
 * Creates a new handle #xvariant_t instance.
 *
 * By convention, handles are indexes into an array of file descriptors
 * that are sent alongside a D-Bus message.  If you're not interacting
 * with D-Bus, you probably don't need them.
 *
 * Returns: (transfer none): a floating reference to a new handle #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_handle:
 * @value: a handle #xvariant_t instance
 *
 * Returns the 32-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type other
 * than %G_VARIANT_TYPE_HANDLE.
 *
 * By convention, handles are indexes into an array of file descriptors
 * that are sent alongside a D-Bus message.  If you're not interacting
 * with D-Bus, you probably don't need them.
 *
 * Returns: a #gint32
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (HANDLE, handle, gint32)

/**
 * xvariant_new_double:
 * @value: a #xdouble_t floating point value
 *
 * Creates a new double #xvariant_t instance.
 *
 * Returns: (transfer none): a floating reference to a new double #xvariant_t instance
 *
 * Since: 2.24
 **/
/**
 * xvariant_get_double:
 * @value: a double #xvariant_t instance
 *
 * Returns the double precision floating point value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_DOUBLE.
 *
 * Returns: a #xdouble_t
 *
 * Since: 2.24
 **/
NUMERIC_TYPE (DOUBLE, double, xdouble_t)

/* Container type Constructor / Deconstructors {{{1 */
/**
 * xvariant_new_maybe:
 * @child_type: (nullable): the #xvariant_type_t of the child, or %NULL
 * @child: (nullable): the child value, or %NULL
 *
 * Depending on if @child is %NULL, either wraps @child inside of a
 * maybe container or creates a Nothing instance for the given @type.
 *
 * At least one of @child_type and @child must be non-%NULL.
 * If @child_type is non-%NULL then it must be a definite type.
 * If they are both non-%NULL then @child_type must be the type
 * of @child.
 *
 * If @child is a floating reference (see xvariant_ref_sink()), the new
 * instance takes ownership of @child.
 *
 * Returns: (transfer none): a floating reference to a new #xvariant_t maybe instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_maybe (const xvariant_type_t *child_type,
                     xvariant_t           *child)
{
  xvariant_type_t *maybe_type;
  xvariant_t *value;

  g_return_val_if_fail (child_type == NULL || xvariant_type_is_definite
                        (child_type), 0);
  g_return_val_if_fail (child_type != NULL || child != NULL, NULL);
  g_return_val_if_fail (child_type == NULL || child == NULL ||
                        xvariant_is_of_type (child, child_type),
                        NULL);

  if (child_type == NULL)
    child_type = xvariant_get_type (child);

  maybe_type = xvariant_type_new_maybe (child_type);

  if (child != NULL)
    {
      xvariant_t **children;
      xboolean_t trusted;

      children = g_new (xvariant_t *, 1);
      children[0] = xvariant_ref_sink (child);
      trusted = xvariant_is_trusted (children[0]);

      value = xvariant_new_from_children (maybe_type, children, 1, trusted);
    }
  else
    value = xvariant_new_from_children (maybe_type, NULL, 0, TRUE);

  xvariant_type_free (maybe_type);

  return value;
}

/**
 * xvariant_get_maybe:
 * @value: a maybe-typed value
 *
 * Given a maybe-typed #xvariant_t instance, extract its value.  If the
 * value is Nothing, then this function returns %NULL.
 *
 * Returns: (nullable) (transfer full): the contents of @value, or %NULL
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_get_maybe (xvariant_t *value)
{
  TYPE_CHECK (value, G_VARIANT_TYPE_MAYBE, NULL);

  if (xvariant_n_children (value))
    return xvariant_get_child_value (value, 0);

  return NULL;
}

/**
 * xvariant_new_variant: (constructor)
 * @value: a #xvariant_t instance
 *
 * Boxes @value.  The result is a #xvariant_t instance representing a
 * variant containing the original value.
 *
 * If @child is a floating reference (see xvariant_ref_sink()), the new
 * instance takes ownership of @child.
 *
 * Returns: (transfer none): a floating reference to a new variant #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_variant (xvariant_t *value)
{
  g_return_val_if_fail (value != NULL, NULL);

  xvariant_ref_sink (value);

  return xvariant_new_from_children (G_VARIANT_TYPE_VARIANT,
                                      g_memdup2 (&value, sizeof value),
                                      1, xvariant_is_trusted (value));
}

/**
 * xvariant_get_variant:
 * @value: a variant #xvariant_t instance
 *
 * Unboxes @value.  The result is the #xvariant_t instance that was
 * contained in @value.
 *
 * Returns: (transfer full): the item contained in the variant
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_get_variant (xvariant_t *value)
{
  TYPE_CHECK (value, G_VARIANT_TYPE_VARIANT, NULL);

  return xvariant_get_child_value (value, 0);
}

/**
 * xvariant_new_array:
 * @child_type: (nullable): the element type of the new array
 * @children: (nullable) (array length=n_children): an array of
 *            #xvariant_t pointers, the children
 * @n_children: the length of @children
 *
 * Creates a new #xvariant_t array from @children.
 *
 * @child_type must be non-%NULL if @n_children is zero.  Otherwise, the
 * child type is determined by inspecting the first element of the
 * @children array.  If @child_type is non-%NULL then it must be a
 * definite type.
 *
 * The items of the array are taken from the @children array.  No entry
 * in the @children array may be %NULL.
 *
 * All items in the array must have the same type, which must be the
 * same as @child_type, if given.
 *
 * If the @children are floating references (see xvariant_ref_sink()), the
 * new instance takes ownership of them as if via xvariant_ref_sink().
 *
 * Returns: (transfer none): a floating reference to a new #xvariant_t array
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_array (const xvariant_type_t *child_type,
                     xvariant_t * const   *children,
                     xsize_t               n_children)
{
  xvariant_type_t *array_type;
  xvariant_t **my_children;
  xboolean_t trusted;
  xvariant_t *value;
  xsize_t i;

  g_return_val_if_fail (n_children > 0 || child_type != NULL, NULL);
  g_return_val_if_fail (n_children == 0 || children != NULL, NULL);
  g_return_val_if_fail (child_type == NULL ||
                        xvariant_type_is_definite (child_type), NULL);

  my_children = g_new (xvariant_t *, n_children);
  trusted = TRUE;

  if (child_type == NULL)
    child_type = xvariant_get_type (children[0]);
  array_type = xvariant_type_new_array (child_type);

  for (i = 0; i < n_children; i++)
    {
      if G_UNLIKELY (!xvariant_is_of_type (children[i], child_type))
        {
          while (i != 0)
            xvariant_unref (my_children[--i]);
          g_free (my_children);
	  g_return_val_if_fail (xvariant_is_of_type (children[i], child_type), NULL);
        }
      my_children[i] = xvariant_ref_sink (children[i]);
      trusted &= xvariant_is_trusted (children[i]);
    }

  value = xvariant_new_from_children (array_type, my_children,
                                       n_children, trusted);
  xvariant_type_free (array_type);

  return value;
}

/*< private >
 * xvariant_make_tuple_type:
 * @children: (array length=n_children): an array of xvariant_t *
 * @n_children: the length of @children
 *
 * Return the type of a tuple containing @children as its items.
 **/
static xvariant_type_t *
xvariant_make_tuple_type (xvariant_t * const *children,
                           xsize_t             n_children)
{
  const xvariant_type_t **types;
  xvariant_type_t *type;
  xsize_t i;

  types = g_new (const xvariant_type_t *, n_children);

  for (i = 0; i < n_children; i++)
    types[i] = xvariant_get_type (children[i]);

  type = xvariant_type_new_tuple (types, n_children);
  g_free (types);

  return type;
}

/**
 * xvariant_new_tuple:
 * @children: (array length=n_children): the items to make the tuple out of
 * @n_children: the length of @children
 *
 * Creates a new tuple #xvariant_t out of the items in @children.  The
 * type is determined from the types of @children.  No entry in the
 * @children array may be %NULL.
 *
 * If @n_children is 0 then the unit tuple is constructed.
 *
 * If the @children are floating references (see xvariant_ref_sink()), the
 * new instance takes ownership of them as if via xvariant_ref_sink().
 *
 * Returns: (transfer none): a floating reference to a new #xvariant_t tuple
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_tuple (xvariant_t * const *children,
                     xsize_t             n_children)
{
  xvariant_type_t *tuple_type;
  xvariant_t **my_children;
  xboolean_t trusted;
  xvariant_t *value;
  xsize_t i;

  g_return_val_if_fail (n_children == 0 || children != NULL, NULL);

  my_children = g_new (xvariant_t *, n_children);
  trusted = TRUE;

  for (i = 0; i < n_children; i++)
    {
      my_children[i] = xvariant_ref_sink (children[i]);
      trusted &= xvariant_is_trusted (children[i]);
    }

  tuple_type = xvariant_make_tuple_type (children, n_children);
  value = xvariant_new_from_children (tuple_type, my_children,
                                       n_children, trusted);
  xvariant_type_free (tuple_type);

  return value;
}

/*< private >
 * xvariant_make_dict_entry_type:
 * @key: a #xvariant_t, the key
 * @val: a #xvariant_t, the value
 *
 * Return the type of a dictionary entry containing @key and @val as its
 * children.
 **/
static xvariant_type_t *
xvariant_make_dict_entry_type (xvariant_t *key,
                                xvariant_t *val)
{
  return xvariant_type_new_dict_entry (xvariant_get_type (key),
                                        xvariant_get_type (val));
}

/**
 * xvariant_new_dict_entry: (constructor)
 * @key: a basic #xvariant_t, the key
 * @value: a #xvariant_t, the value
 *
 * Creates a new dictionary entry #xvariant_t. @key and @value must be
 * non-%NULL. @key must be a value of a basic type (ie: not a container).
 *
 * If the @key or @value are floating references (see xvariant_ref_sink()),
 * the new instance takes ownership of them as if via xvariant_ref_sink().
 *
 * Returns: (transfer none): a floating reference to a new dictionary entry #xvariant_t
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_dict_entry (xvariant_t *key,
                          xvariant_t *value)
{
  xvariant_type_t *dict_type;
  xvariant_t **children;
  xboolean_t trusted;

  g_return_val_if_fail (key != NULL && value != NULL, NULL);
  g_return_val_if_fail (!xvariant_is_container (key), NULL);

  children = g_new (xvariant_t *, 2);
  children[0] = xvariant_ref_sink (key);
  children[1] = xvariant_ref_sink (value);
  trusted = xvariant_is_trusted (key) && xvariant_is_trusted (value);

  dict_type = xvariant_make_dict_entry_type (key, value);
  value = xvariant_new_from_children (dict_type, children, 2, trusted);
  xvariant_type_free (dict_type);

  return value;
}

/**
 * xvariant_lookup: (skip)
 * @dictionary: a dictionary #xvariant_t
 * @key: the key to look up in the dictionary
 * @format_string: a xvariant_t format string
 * @...: the arguments to unpack the value into
 *
 * Looks up a value in a dictionary #xvariant_t.
 *
 * This function is a wrapper around xvariant_lookup_value() and
 * xvariant_get().  In the case that %NULL would have been returned,
 * this function returns %FALSE.  Otherwise, it unpacks the returned
 * value and returns %TRUE.
 *
 * @format_string determines the C types that are used for unpacking
 * the values and also determines if the values are copied or borrowed,
 * see the section on
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * This function is currently implemented with a linear scan.  If you
 * plan to do many lookups then #xvariant_dict_t may be more efficient.
 *
 * Returns: %TRUE if a value was unpacked
 *
 * Since: 2.28
 */
xboolean_t
xvariant_lookup (xvariant_t    *dictionary,
                  const xchar_t *key,
                  const xchar_t *format_string,
                  ...)
{
  xvariant_type_t *type;
  xvariant_t *value;

  /* flatten */
  xvariant_get_data (dictionary);

  type = xvariant_format_string_scan_type (format_string, NULL, NULL);
  value = xvariant_lookup_value (dictionary, key, type);
  xvariant_type_free (type);

  if (value)
    {
      va_list ap;

      va_start (ap, format_string);
      xvariant_get_va (value, format_string, NULL, &ap);
      xvariant_unref (value);
      va_end (ap);

      return TRUE;
    }

  else
    return FALSE;
}

/**
 * xvariant_lookup_value:
 * @dictionary: a dictionary #xvariant_t
 * @key: the key to look up in the dictionary
 * @expected_type: (nullable): a #xvariant_type_t, or %NULL
 *
 * Looks up a value in a dictionary #xvariant_t.
 *
 * This function works with dictionaries of the type a{s*} (and equally
 * well with type a{o*}, but we only further discuss the string case
 * for sake of clarity).
 *
 * In the event that @dictionary has the type a{sv}, the @expected_type
 * string specifies what type of value is expected to be inside of the
 * variant. If the value inside the variant has a different type then
 * %NULL is returned. In the event that @dictionary has a value type other
 * than v then @expected_type must directly match the value type and it is
 * used to unpack the value directly or an error occurs.
 *
 * In either case, if @key is not found in @dictionary, %NULL is returned.
 *
 * If the key is found and the value has the correct type, it is
 * returned.  If @expected_type was specified then any non-%NULL return
 * value will have this type.
 *
 * This function is currently implemented with a linear scan.  If you
 * plan to do many lookups then #xvariant_dict_t may be more efficient.
 *
 * Returns: (transfer full): the value of the dictionary key, or %NULL
 *
 * Since: 2.28
 */
xvariant_t *
xvariant_lookup_value (xvariant_t           *dictionary,
                        const xchar_t        *key,
                        const xvariant_type_t *expected_type)
{
  xvariant_iter_t iter;
  xvariant_t *entry;
  xvariant_t *value;

  g_return_val_if_fail (xvariant_is_of_type (dictionary,
                                              G_VARIANT_TYPE ("a{s*}")) ||
                        xvariant_is_of_type (dictionary,
                                              G_VARIANT_TYPE ("a{o*}")),
                        NULL);

  xvariant_iter_init (&iter, dictionary);

  while ((entry = xvariant_iter_next_value (&iter)))
    {
      xvariant_t *entry_key;
      xboolean_t matches;

      entry_key = xvariant_get_child_value (entry, 0);
      matches = strcmp (xvariant_get_string (entry_key, NULL), key) == 0;
      xvariant_unref (entry_key);

      if (matches)
        break;

      xvariant_unref (entry);
    }

  if (entry == NULL)
    return NULL;

  value = xvariant_get_child_value (entry, 1);
  xvariant_unref (entry);

  if (xvariant_is_of_type (value, G_VARIANT_TYPE_VARIANT))
    {
      xvariant_t *tmp;

      tmp = xvariant_get_variant (value);
      xvariant_unref (value);

      if (expected_type && !xvariant_is_of_type (tmp, expected_type))
        {
          xvariant_unref (tmp);
          tmp = NULL;
        }

      value = tmp;
    }

  g_return_val_if_fail (expected_type == NULL || value == NULL ||
                        xvariant_is_of_type (value, expected_type), NULL);

  return value;
}

/**
 * xvariant_get_fixed_array:
 * @value: a #xvariant_t array with fixed-sized elements
 * @n_elements: (out): a pointer to the location to store the number of items
 * @element_size: the size of each element
 *
 * Provides access to the serialized data for an array of fixed-sized
 * items.
 *
 * @value must be an array with fixed-sized elements.  Numeric types are
 * fixed-size, as are tuples containing only other fixed-sized types.
 *
 * @element_size must be the size of a single element in the array,
 * as given by the section on
 * [serialized data memory][gvariant-serialized-data-memory].
 *
 * In particular, arrays of these fixed-sized types can be interpreted
 * as an array of the given C type, with @element_size set to the size
 * the appropriate type:
 * - %G_VARIANT_TYPE_INT16 (etc.): #gint16 (etc.)
 * - %G_VARIANT_TYPE_BOOLEAN: #xuchar_t (not #xboolean_t!)
 * - %G_VARIANT_TYPE_BYTE: #xuint8_t
 * - %G_VARIANT_TYPE_HANDLE: #xuint32_t
 * - %G_VARIANT_TYPE_DOUBLE: #xdouble_t
 *
 * For example, if calling this function for an array of 32-bit integers,
 * you might say `sizeof(gint32)`. This value isn't used except for the purpose
 * of a double-check that the form of the serialized data matches the caller's
 * expectation.
 *
 * @n_elements, which must be non-%NULL, is set equal to the number of
 * items in the array.
 *
 * Returns: (array length=n_elements) (transfer none): a pointer to
 *     the fixed array
 *
 * Since: 2.24
 **/
xconstpointer
xvariant_get_fixed_array (xvariant_t *value,
                           xsize_t    *n_elements,
                           xsize_t     element_size)
{
  GVariantTypeInfo *array_info;
  xsize_t array_element_size;
  xconstpointer data;
  xsize_t size;

  TYPE_CHECK (value, G_VARIANT_TYPE_ARRAY, NULL);

  g_return_val_if_fail (n_elements != NULL, NULL);
  g_return_val_if_fail (element_size > 0, NULL);

  array_info = xvariant_get_type_info (value);
  xvariant_type_info_query_element (array_info, NULL, &array_element_size);

  g_return_val_if_fail (array_element_size, NULL);

  if G_UNLIKELY (array_element_size != element_size)
    {
      if (array_element_size)
        g_critical ("xvariant_get_fixed_array: assertion "
                    "'xvariant_array_has_fixed_size (value, element_size)' "
                    "failed: array size %"G_GSIZE_FORMAT" does not match "
                    "given element_size %"G_GSIZE_FORMAT".",
                    array_element_size, element_size);
      else
        g_critical ("xvariant_get_fixed_array: assertion "
                    "'xvariant_array_has_fixed_size (value, element_size)' "
                    "failed: array does not have fixed size.");
    }

  data = xvariant_get_data (value);
  size = xvariant_get_size (value);

  if (size % element_size)
    *n_elements = 0;
  else
    *n_elements = size / element_size;

  if (*n_elements)
    return data;

  return NULL;
}

/**
 * xvariant_new_fixed_array:
 * @element_type: the #xvariant_type_t of each element
 * @elements: a pointer to the fixed array of contiguous elements
 * @n_elements: the number of elements
 * @element_size: the size of each element
 *
 * Constructs a new array #xvariant_t instance, where the elements are
 * of @element_type type.
 *
 * @elements must be an array with fixed-sized elements.  Numeric types are
 * fixed-size as are tuples containing only other fixed-sized types.
 *
 * @element_size must be the size of a single element in the array.
 * For example, if calling this function for an array of 32-bit integers,
 * you might say sizeof(gint32). This value isn't used except for the purpose
 * of a double-check that the form of the serialized data matches the caller's
 * expectation.
 *
 * @n_elements must be the length of the @elements array.
 *
 * Returns: (transfer none): a floating reference to a new array #xvariant_t instance
 *
 * Since: 2.32
 **/
xvariant_t *
xvariant_new_fixed_array (const xvariant_type_t  *element_type,
                           xconstpointer        elements,
                           xsize_t                n_elements,
                           xsize_t                element_size)
{
  xvariant_type_t *array_type;
  xsize_t array_element_size;
  GVariantTypeInfo *array_info;
  xvariant_t *value;
  xpointer_t data;

  g_return_val_if_fail (xvariant_type_is_definite (element_type), NULL);
  g_return_val_if_fail (element_size > 0, NULL);

  array_type = xvariant_type_new_array (element_type);
  array_info = xvariant_type_info_get (array_type);
  xvariant_type_info_query_element (array_info, NULL, &array_element_size);
  if G_UNLIKELY (array_element_size != element_size)
    {
      if (array_element_size)
        g_critical ("xvariant_new_fixed_array: array size %" G_GSIZE_FORMAT
                    " does not match given element_size %" G_GSIZE_FORMAT ".",
                    array_element_size, element_size);
      else
        g_critical ("xvariant_get_fixed_array: array does not have fixed size.");
      return NULL;
    }

  data = g_memdup2 (elements, n_elements * element_size);
  value = xvariant_new_from_data (array_type, data,
                                   n_elements * element_size,
                                   FALSE, g_free, data);

  xvariant_type_free (array_type);
  xvariant_type_info_unref (array_info);

  return value;
}

/* String type constructor/getters/validation {{{1 */
/**
 * xvariant_new_string:
 * @string: a normal UTF-8 nul-terminated string
 *
 * Creates a string #xvariant_t with the contents of @string.
 *
 * @string must be valid UTF-8, and must not be %NULL. To encode
 * potentially-%NULL strings, use xvariant_new() with `ms` as the
 * [format string][gvariant-format-strings-maybe-types].
 *
 * Returns: (transfer none): a floating reference to a new string #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_string (const xchar_t *string)
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (xutf8_validate (string, -1, NULL), NULL);

  return xvariant_new_from_trusted (G_VARIANT_TYPE_STRING,
                                     string, strlen (string) + 1);
}

/**
 * xvariant_new_take_string: (skip)
 * @string: a normal UTF-8 nul-terminated string
 *
 * Creates a string #xvariant_t with the contents of @string.
 *
 * @string must be valid UTF-8, and must not be %NULL. To encode
 * potentially-%NULL strings, use this with xvariant_new_maybe().
 *
 * This function consumes @string.  g_free() will be called on @string
 * when it is no longer required.
 *
 * You must not modify or access @string in any other way after passing
 * it to this function.  It is even possible that @string is immediately
 * freed.
 *
 * Returns: (transfer none): a floating reference to a new string
 *   #xvariant_t instance
 *
 * Since: 2.38
 **/
xvariant_t *
xvariant_new_take_string (xchar_t *string)
{
  xvariant_t *value;
  xbytes_t *bytes;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (xutf8_validate (string, -1, NULL), NULL);

  bytes = xbytes_new_take (string, strlen (string) + 1);
  value = xvariant_new_from_bytes (G_VARIANT_TYPE_STRING, bytes, TRUE);
  xbytes_unref (bytes);

  return value;
}

/**
 * xvariant_new_printf: (skip)
 * @format_string: a printf-style format string
 * @...: arguments for @format_string
 *
 * Creates a string-type xvariant_t using printf formatting.
 *
 * This is similar to calling xstrdup_printf() and then
 * xvariant_new_string() but it saves a temporary variable and an
 * unnecessary copy.
 *
 * Returns: (transfer none): a floating reference to a new string
 *   #xvariant_t instance
 *
 * Since: 2.38
 **/
xvariant_t *
xvariant_new_printf (const xchar_t *format_string,
                      ...)
{
  xvariant_t *value;
  xbytes_t *bytes;
  xchar_t *string;
  va_list ap;

  g_return_val_if_fail (format_string != NULL, NULL);

  va_start (ap, format_string);
  string = xstrdup_vprintf (format_string, ap);
  va_end (ap);

  bytes = xbytes_new_take (string, strlen (string) + 1);
  value = xvariant_new_from_bytes (G_VARIANT_TYPE_STRING, bytes, TRUE);
  xbytes_unref (bytes);

  return value;
}

/**
 * xvariant_new_object_path:
 * @object_path: a normal C nul-terminated string
 *
 * Creates a D-Bus object path #xvariant_t with the contents of @string.
 * @string must be a valid D-Bus object path.  Use
 * xvariant_is_object_path() if you're not sure.
 *
 * Returns: (transfer none): a floating reference to a new object path #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_object_path (const xchar_t *object_path)
{
  g_return_val_if_fail (xvariant_is_object_path (object_path), NULL);

  return xvariant_new_from_trusted (G_VARIANT_TYPE_OBJECT_PATH,
                                     object_path, strlen (object_path) + 1);
}

/**
 * xvariant_is_object_path:
 * @string: a normal C nul-terminated string
 *
 * Determines if a given string is a valid D-Bus object path.  You
 * should ensure that a string is a valid D-Bus object path before
 * passing it to xvariant_new_object_path().
 *
 * A valid object path starts with `/` followed by zero or more
 * sequences of characters separated by `/` characters.  Each sequence
 * must contain only the characters `[A-Z][a-z][0-9]_`.  No sequence
 * (including the one following the final `/` character) may be empty.
 *
 * Returns: %TRUE if @string is a D-Bus object path
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_is_object_path (const xchar_t *string)
{
  g_return_val_if_fail (string != NULL, FALSE);

  return xvariant_serialiser_is_object_path (string, strlen (string) + 1);
}

/**
 * xvariant_new_signature:
 * @signature: a normal C nul-terminated string
 *
 * Creates a D-Bus type signature #xvariant_t with the contents of
 * @string.  @string must be a valid D-Bus type signature.  Use
 * xvariant_is_signature() if you're not sure.
 *
 * Returns: (transfer none): a floating reference to a new signature #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_signature (const xchar_t *signature)
{
  g_return_val_if_fail (xvariant_is_signature (signature), NULL);

  return xvariant_new_from_trusted (G_VARIANT_TYPE_SIGNATURE,
                                     signature, strlen (signature) + 1);
}

/**
 * xvariant_is_signature:
 * @string: a normal C nul-terminated string
 *
 * Determines if a given string is a valid D-Bus type signature.  You
 * should ensure that a string is a valid D-Bus type signature before
 * passing it to xvariant_new_signature().
 *
 * D-Bus type signatures consist of zero or more definite #xvariant_type_t
 * strings in sequence.
 *
 * Returns: %TRUE if @string is a D-Bus type signature
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_is_signature (const xchar_t *string)
{
  g_return_val_if_fail (string != NULL, FALSE);

  return xvariant_serialiser_is_signature (string, strlen (string) + 1);
}

/**
 * xvariant_get_string:
 * @value: a string #xvariant_t instance
 * @length: (optional) (default 0) (out): a pointer to a #xsize_t,
 *          to store the length
 *
 * Returns the string value of a #xvariant_t instance with a string
 * type.  This includes the types %G_VARIANT_TYPE_STRING,
 * %G_VARIANT_TYPE_OBJECT_PATH and %G_VARIANT_TYPE_SIGNATURE.
 *
 * The string will always be UTF-8 encoded, will never be %NULL, and will never
 * contain nul bytes.
 *
 * If @length is non-%NULL then the length of the string (in bytes) is
 * returned there.  For trusted values, this information is already
 * known.  Untrusted values will be validated and, if valid, a strlen() will be
 * performed. If invalid, a default value will be returned — for
 * %G_VARIANT_TYPE_OBJECT_PATH, this is `"/"`, and for other types it is the
 * empty string.
 *
 * It is an error to call this function with a @value of any type
 * other than those three.
 *
 * The return value remains valid as long as @value exists.
 *
 * Returns: (transfer none): the constant string, UTF-8 encoded
 *
 * Since: 2.24
 **/
const xchar_t *
xvariant_get_string (xvariant_t *value,
                      xsize_t    *length)
{
  xconstpointer data;
  xsize_t size;

  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (
    xvariant_is_of_type (value, G_VARIANT_TYPE_STRING) ||
    xvariant_is_of_type (value, G_VARIANT_TYPE_OBJECT_PATH) ||
    xvariant_is_of_type (value, G_VARIANT_TYPE_SIGNATURE), NULL);

  data = xvariant_get_data (value);
  size = xvariant_get_size (value);

  if (!xvariant_is_trusted (value))
    {
      switch (xvariant_classify (value))
        {
        case XVARIANT_CLASS_STRING:
          if (xvariant_serialiser_is_string (data, size))
            break;

          data = "";
          size = 1;
          break;

        case XVARIANT_CLASS_OBJECT_PATH:
          if (xvariant_serialiser_is_object_path (data, size))
            break;

          data = "/";
          size = 2;
          break;

        case XVARIANT_CLASS_SIGNATURE:
          if (xvariant_serialiser_is_signature (data, size))
            break;

          data = "";
          size = 1;
          break;

        default:
          g_assert_not_reached ();
        }
    }

  if (length)
    *length = size - 1;

  return data;
}

/**
 * xvariant_dup_string:
 * @value: a string #xvariant_t instance
 * @length: (out): a pointer to a #xsize_t, to store the length
 *
 * Similar to xvariant_get_string() except that instead of returning
 * a constant string, the string is duplicated.
 *
 * The string will always be UTF-8 encoded.
 *
 * The return value must be freed using g_free().
 *
 * Returns: (transfer full): a newly allocated string, UTF-8 encoded
 *
 * Since: 2.24
 **/
xchar_t *
xvariant_dup_string (xvariant_t *value,
                      xsize_t    *length)
{
  return xstrdup (xvariant_get_string (value, length));
}

/**
 * xvariant_new_strv:
 * @strv: (array length=length) (element-type utf8): an array of strings
 * @length: the length of @strv, or -1
 *
 * Constructs an array of strings #xvariant_t from the given array of
 * strings.
 *
 * If @length is -1 then @strv is %NULL-terminated.
 *
 * Returns: (transfer none): a new floating #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_strv (const xchar_t * const *strv,
                    xssize_t               length)
{
  xvariant_t **strings;
  xsize_t i, length_unsigned;

  g_return_val_if_fail (length == 0 || strv != NULL, NULL);

  if (length < 0)
    length = xstrv_length ((xchar_t **) strv);
  length_unsigned = length;

  strings = g_new (xvariant_t *, length_unsigned);
  for (i = 0; i < length_unsigned; i++)
    strings[i] = xvariant_ref_sink (xvariant_new_string (strv[i]));

  return xvariant_new_from_children (G_VARIANT_TYPE_STRING_ARRAY,
                                      strings, length_unsigned, TRUE);
}

/**
 * xvariant_get_strv:
 * @value: an array of strings #xvariant_t
 * @length: (out) (optional): the length of the result, or %NULL
 *
 * Gets the contents of an array of strings #xvariant_t.  This call
 * makes a shallow copy; the return result should be released with
 * g_free(), but the individual strings must not be modified.
 *
 * If @length is non-%NULL then the number of elements in the result
 * is stored there.  In any case, the resulting array will be
 * %NULL-terminated.
 *
 * For an empty array, @length will be set to 0 and a pointer to a
 * %NULL pointer will be returned.
 *
 * Returns: (array length=length zero-terminated=1) (transfer container): an array of constant strings
 *
 * Since: 2.24
 **/
const xchar_t **
xvariant_get_strv (xvariant_t *value,
                    xsize_t    *length)
{
  const xchar_t **strv;
  xsize_t n;
  xsize_t i;

  TYPE_CHECK (value, G_VARIANT_TYPE_STRING_ARRAY, NULL);

  xvariant_get_data (value);
  n = xvariant_n_children (value);
  strv = g_new (const xchar_t *, n + 1);

  for (i = 0; i < n; i++)
    {
      xvariant_t *string;

      string = xvariant_get_child_value (value, i);
      strv[i] = xvariant_get_string (string, NULL);
      xvariant_unref (string);
    }
  strv[i] = NULL;

  if (length)
    *length = n;

  return strv;
}

/**
 * xvariant_dup_strv:
 * @value: an array of strings #xvariant_t
 * @length: (out) (optional): the length of the result, or %NULL
 *
 * Gets the contents of an array of strings #xvariant_t.  This call
 * makes a deep copy; the return result should be released with
 * xstrfreev().
 *
 * If @length is non-%NULL then the number of elements in the result
 * is stored there.  In any case, the resulting array will be
 * %NULL-terminated.
 *
 * For an empty array, @length will be set to 0 and a pointer to a
 * %NULL pointer will be returned.
 *
 * Returns: (array length=length zero-terminated=1) (transfer full): an array of strings
 *
 * Since: 2.24
 **/
xchar_t **
xvariant_dup_strv (xvariant_t *value,
                    xsize_t    *length)
{
  xchar_t **strv;
  xsize_t n;
  xsize_t i;

  TYPE_CHECK (value, G_VARIANT_TYPE_STRING_ARRAY, NULL);

  n = xvariant_n_children (value);
  strv = g_new (xchar_t *, n + 1);

  for (i = 0; i < n; i++)
    {
      xvariant_t *string;

      string = xvariant_get_child_value (value, i);
      strv[i] = xvariant_dup_string (string, NULL);
      xvariant_unref (string);
    }
  strv[i] = NULL;

  if (length)
    *length = n;

  return strv;
}

/**
 * xvariant_new_objv:
 * @strv: (array length=length) (element-type utf8): an array of strings
 * @length: the length of @strv, or -1
 *
 * Constructs an array of object paths #xvariant_t from the given array of
 * strings.
 *
 * Each string must be a valid #xvariant_t object path; see
 * xvariant_is_object_path().
 *
 * If @length is -1 then @strv is %NULL-terminated.
 *
 * Returns: (transfer none): a new floating #xvariant_t instance
 *
 * Since: 2.30
 **/
xvariant_t *
xvariant_new_objv (const xchar_t * const *strv,
                    xssize_t               length)
{
  xvariant_t **strings;
  xsize_t i, length_unsigned;

  g_return_val_if_fail (length == 0 || strv != NULL, NULL);

  if (length < 0)
    length = xstrv_length ((xchar_t **) strv);
  length_unsigned = length;

  strings = g_new (xvariant_t *, length_unsigned);
  for (i = 0; i < length_unsigned; i++)
    strings[i] = xvariant_ref_sink (xvariant_new_object_path (strv[i]));

  return xvariant_new_from_children (G_VARIANT_TYPE_OBJECT_PATH_ARRAY,
                                      strings, length_unsigned, TRUE);
}

/**
 * xvariant_get_objv:
 * @value: an array of object paths #xvariant_t
 * @length: (out) (optional): the length of the result, or %NULL
 *
 * Gets the contents of an array of object paths #xvariant_t.  This call
 * makes a shallow copy; the return result should be released with
 * g_free(), but the individual strings must not be modified.
 *
 * If @length is non-%NULL then the number of elements in the result
 * is stored there.  In any case, the resulting array will be
 * %NULL-terminated.
 *
 * For an empty array, @length will be set to 0 and a pointer to a
 * %NULL pointer will be returned.
 *
 * Returns: (array length=length zero-terminated=1) (transfer container): an array of constant strings
 *
 * Since: 2.30
 **/
const xchar_t **
xvariant_get_objv (xvariant_t *value,
                    xsize_t    *length)
{
  const xchar_t **strv;
  xsize_t n;
  xsize_t i;

  TYPE_CHECK (value, G_VARIANT_TYPE_OBJECT_PATH_ARRAY, NULL);

  xvariant_get_data (value);
  n = xvariant_n_children (value);
  strv = g_new (const xchar_t *, n + 1);

  for (i = 0; i < n; i++)
    {
      xvariant_t *string;

      string = xvariant_get_child_value (value, i);
      strv[i] = xvariant_get_string (string, NULL);
      xvariant_unref (string);
    }
  strv[i] = NULL;

  if (length)
    *length = n;

  return strv;
}

/**
 * xvariant_dup_objv:
 * @value: an array of object paths #xvariant_t
 * @length: (out) (optional): the length of the result, or %NULL
 *
 * Gets the contents of an array of object paths #xvariant_t.  This call
 * makes a deep copy; the return result should be released with
 * xstrfreev().
 *
 * If @length is non-%NULL then the number of elements in the result
 * is stored there.  In any case, the resulting array will be
 * %NULL-terminated.
 *
 * For an empty array, @length will be set to 0 and a pointer to a
 * %NULL pointer will be returned.
 *
 * Returns: (array length=length zero-terminated=1) (transfer full): an array of strings
 *
 * Since: 2.30
 **/
xchar_t **
xvariant_dup_objv (xvariant_t *value,
                    xsize_t    *length)
{
  xchar_t **strv;
  xsize_t n;
  xsize_t i;

  TYPE_CHECK (value, G_VARIANT_TYPE_OBJECT_PATH_ARRAY, NULL);

  n = xvariant_n_children (value);
  strv = g_new (xchar_t *, n + 1);

  for (i = 0; i < n; i++)
    {
      xvariant_t *string;

      string = xvariant_get_child_value (value, i);
      strv[i] = xvariant_dup_string (string, NULL);
      xvariant_unref (string);
    }
  strv[i] = NULL;

  if (length)
    *length = n;

  return strv;
}


/**
 * xvariant_new_bytestring:
 * @string: (array zero-terminated=1) (element-type xuint8_t): a normal
 *          nul-terminated string in no particular encoding
 *
 * Creates an array-of-bytes #xvariant_t with the contents of @string.
 * This function is just like xvariant_new_string() except that the
 * string need not be valid UTF-8.
 *
 * The nul terminator character at the end of the string is stored in
 * the array.
 *
 * Returns: (transfer none): a floating reference to a new bytestring #xvariant_t instance
 *
 * Since: 2.26
 **/
xvariant_t *
xvariant_new_bytestring (const xchar_t *string)
{
  g_return_val_if_fail (string != NULL, NULL);

  return xvariant_new_from_trusted (G_VARIANT_TYPE_BYTESTRING,
                                     string, strlen (string) + 1);
}

/**
 * xvariant_get_bytestring:
 * @value: an array-of-bytes #xvariant_t instance
 *
 * Returns the string value of a #xvariant_t instance with an
 * array-of-bytes type.  The string has no particular encoding.
 *
 * If the array does not end with a nul terminator character, the empty
 * string is returned.  For this reason, you can always trust that a
 * non-%NULL nul-terminated string will be returned by this function.
 *
 * If the array contains a nul terminator character somewhere other than
 * the last byte then the returned string is the string, up to the first
 * such nul character.
 *
 * xvariant_get_fixed_array() should be used instead if the array contains
 * arbitrary data that could not be nul-terminated or could contain nul bytes.
 *
 * It is an error to call this function with a @value that is not an
 * array of bytes.
 *
 * The return value remains valid as long as @value exists.
 *
 * Returns: (transfer none) (array zero-terminated=1) (element-type xuint8_t):
 *          the constant string
 *
 * Since: 2.26
 **/
const xchar_t *
xvariant_get_bytestring (xvariant_t *value)
{
  const xchar_t *string;
  xsize_t size;

  TYPE_CHECK (value, G_VARIANT_TYPE_BYTESTRING, NULL);

  /* Won't be NULL since this is an array type */
  string = xvariant_get_data (value);
  size = xvariant_get_size (value);

  if (size && string[size - 1] == '\0')
    return string;
  else
    return "";
}

/**
 * xvariant_dup_bytestring:
 * @value: an array-of-bytes #xvariant_t instance
 * @length: (out) (optional) (default NULL): a pointer to a #xsize_t, to store
 *          the length (not including the nul terminator)
 *
 * Similar to xvariant_get_bytestring() except that instead of
 * returning a constant string, the string is duplicated.
 *
 * The return value must be freed using g_free().
 *
 * Returns: (transfer full) (array zero-terminated=1 length=length) (element-type xuint8_t):
 *          a newly allocated string
 *
 * Since: 2.26
 **/
xchar_t *
xvariant_dup_bytestring (xvariant_t *value,
                          xsize_t    *length)
{
  const xchar_t *original = xvariant_get_bytestring (value);
  xsize_t size;

  /* don't crash in case get_bytestring() had an assert failure */
  if (original == NULL)
    return NULL;

  size = strlen (original);

  if (length)
    *length = size;

  return g_memdup2 (original, size + 1);
}

/**
 * xvariant_new_bytestring_array:
 * @strv: (array length=length): an array of strings
 * @length: the length of @strv, or -1
 *
 * Constructs an array of bytestring #xvariant_t from the given array of
 * strings.
 *
 * If @length is -1 then @strv is %NULL-terminated.
 *
 * Returns: (transfer none): a new floating #xvariant_t instance
 *
 * Since: 2.26
 **/
xvariant_t *
xvariant_new_bytestring_array (const xchar_t * const *strv,
                                xssize_t               length)
{
  xvariant_t **strings;
  xsize_t i, length_unsigned;

  g_return_val_if_fail (length == 0 || strv != NULL, NULL);

  if (length < 0)
    length = xstrv_length ((xchar_t **) strv);
  length_unsigned = length;

  strings = g_new (xvariant_t *, length_unsigned);
  for (i = 0; i < length_unsigned; i++)
    strings[i] = xvariant_ref_sink (xvariant_new_bytestring (strv[i]));

  return xvariant_new_from_children (G_VARIANT_TYPE_BYTESTRING_ARRAY,
                                      strings, length_unsigned, TRUE);
}

/**
 * xvariant_get_bytestring_array:
 * @value: an array of array of bytes #xvariant_t ('aay')
 * @length: (out) (optional): the length of the result, or %NULL
 *
 * Gets the contents of an array of array of bytes #xvariant_t.  This call
 * makes a shallow copy; the return result should be released with
 * g_free(), but the individual strings must not be modified.
 *
 * If @length is non-%NULL then the number of elements in the result is
 * stored there.  In any case, the resulting array will be
 * %NULL-terminated.
 *
 * For an empty array, @length will be set to 0 and a pointer to a
 * %NULL pointer will be returned.
 *
 * Returns: (array length=length) (transfer container): an array of constant strings
 *
 * Since: 2.26
 **/
const xchar_t **
xvariant_get_bytestring_array (xvariant_t *value,
                                xsize_t    *length)
{
  const xchar_t **strv;
  xsize_t n;
  xsize_t i;

  TYPE_CHECK (value, G_VARIANT_TYPE_BYTESTRING_ARRAY, NULL);

  xvariant_get_data (value);
  n = xvariant_n_children (value);
  strv = g_new (const xchar_t *, n + 1);

  for (i = 0; i < n; i++)
    {
      xvariant_t *string;

      string = xvariant_get_child_value (value, i);
      strv[i] = xvariant_get_bytestring (string);
      xvariant_unref (string);
    }
  strv[i] = NULL;

  if (length)
    *length = n;

  return strv;
}

/**
 * xvariant_dup_bytestring_array:
 * @value: an array of array of bytes #xvariant_t ('aay')
 * @length: (out) (optional): the length of the result, or %NULL
 *
 * Gets the contents of an array of array of bytes #xvariant_t.  This call
 * makes a deep copy; the return result should be released with
 * xstrfreev().
 *
 * If @length is non-%NULL then the number of elements in the result is
 * stored there.  In any case, the resulting array will be
 * %NULL-terminated.
 *
 * For an empty array, @length will be set to 0 and a pointer to a
 * %NULL pointer will be returned.
 *
 * Returns: (array length=length) (transfer full): an array of strings
 *
 * Since: 2.26
 **/
xchar_t **
xvariant_dup_bytestring_array (xvariant_t *value,
                                xsize_t    *length)
{
  xchar_t **strv;
  xsize_t n;
  xsize_t i;

  TYPE_CHECK (value, G_VARIANT_TYPE_BYTESTRING_ARRAY, NULL);

  xvariant_get_data (value);
  n = xvariant_n_children (value);
  strv = g_new (xchar_t *, n + 1);

  for (i = 0; i < n; i++)
    {
      xvariant_t *string;

      string = xvariant_get_child_value (value, i);
      strv[i] = xvariant_dup_bytestring (string, NULL);
      xvariant_unref (string);
    }
  strv[i] = NULL;

  if (length)
    *length = n;

  return strv;
}

/* Type checking and querying {{{1 */
/**
 * xvariant_get_type:
 * @value: a #xvariant_t
 *
 * Determines the type of @value.
 *
 * The return value is valid for the lifetime of @value and must not
 * be freed.
 *
 * Returns: a #xvariant_type_t
 *
 * Since: 2.24
 **/
const xvariant_type_t *
xvariant_get_type (xvariant_t *value)
{
  GVariantTypeInfo *type_info;

  g_return_val_if_fail (value != NULL, NULL);

  type_info = xvariant_get_type_info (value);

  return (xvariant_type_t *) xvariant_type_info_get_type_string (type_info);
}

/**
 * xvariant_get_type_string:
 * @value: a #xvariant_t
 *
 * Returns the type string of @value.  Unlike the result of calling
 * xvariant_type_peek_string(), this string is nul-terminated.  This
 * string belongs to #xvariant_t and must not be freed.
 *
 * Returns: the type string for the type of @value
 *
 * Since: 2.24
 **/
const xchar_t *
xvariant_get_type_string (xvariant_t *value)
{
  GVariantTypeInfo *type_info;

  g_return_val_if_fail (value != NULL, NULL);

  type_info = xvariant_get_type_info (value);

  return xvariant_type_info_get_type_string (type_info);
}

/**
 * xvariant_is_of_type:
 * @value: a #xvariant_t instance
 * @type: a #xvariant_type_t
 *
 * Checks if a value has a type matching the provided type.
 *
 * Returns: %TRUE if the type of @value matches @type
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_is_of_type (xvariant_t           *value,
                      const xvariant_type_t *type)
{
  return xvariant_type_is_subtype_of (xvariant_get_type (value), type);
}

/**
 * xvariant_is_container:
 * @value: a #xvariant_t instance
 *
 * Checks if @value is a container.
 *
 * Returns: %TRUE if @value is a container
 *
 * Since: 2.24
 */
xboolean_t
xvariant_is_container (xvariant_t *value)
{
  return xvariant_type_is_container (xvariant_get_type (value));
}


/**
 * xvariant_classify:
 * @value: a #xvariant_t
 *
 * Classifies @value according to its top-level type.
 *
 * Returns: the #xvariant_class_t of @value
 *
 * Since: 2.24
 **/
/**
 * xvariant_class_t:
 * @XVARIANT_CLASS_BOOLEAN: The #xvariant_t is a boolean.
 * @XVARIANT_CLASS_BYTE: The #xvariant_t is a byte.
 * @XVARIANT_CLASS_INT16: The #xvariant_t is a signed 16 bit integer.
 * @XVARIANT_CLASS_UINT16: The #xvariant_t is an unsigned 16 bit integer.
 * @XVARIANT_CLASS_INT32: The #xvariant_t is a signed 32 bit integer.
 * @XVARIANT_CLASS_UINT32: The #xvariant_t is an unsigned 32 bit integer.
 * @XVARIANT_CLASS_INT64: The #xvariant_t is a signed 64 bit integer.
 * @XVARIANT_CLASS_UINT64: The #xvariant_t is an unsigned 64 bit integer.
 * @XVARIANT_CLASS_HANDLE: The #xvariant_t is a file handle index.
 * @XVARIANT_CLASS_DOUBLE: The #xvariant_t is a double precision floating
 *                          point value.
 * @XVARIANT_CLASS_STRING: The #xvariant_t is a normal string.
 * @XVARIANT_CLASS_OBJECT_PATH: The #xvariant_t is a D-Bus object path
 *                               string.
 * @XVARIANT_CLASS_SIGNATURE: The #xvariant_t is a D-Bus signature string.
 * @XVARIANT_CLASS_VARIANT: The #xvariant_t is a variant.
 * @XVARIANT_CLASS_MAYBE: The #xvariant_t is a maybe-typed value.
 * @XVARIANT_CLASS_ARRAY: The #xvariant_t is an array.
 * @XVARIANT_CLASS_TUPLE: The #xvariant_t is a tuple.
 * @XVARIANT_CLASS_DICT_ENTRY: The #xvariant_t is a dictionary entry.
 *
 * The range of possible top-level types of #xvariant_t instances.
 *
 * Since: 2.24
 **/
xvariant_class_t
xvariant_classify (xvariant_t *value)
{
  g_return_val_if_fail (value != NULL, 0);

  return *xvariant_get_type_string (value);
}

/* Pretty printer {{{1 */
/* This function is not introspectable because if @string is NULL,
   @returns is (transfer full), otherwise it is (transfer none), which
   is not supported by GObjectIntrospection */
/**
 * xvariant_print_string: (skip)
 * @value: a #xvariant_t
 * @string: (nullable) (default NULL): a #xstring_t, or %NULL
 * @type_annotate: %TRUE if type information should be included in
 *                 the output
 *
 * Behaves as xvariant_print(), but operates on a #xstring_t.
 *
 * If @string is non-%NULL then it is appended to and returned.  Else,
 * a new empty #xstring_t is allocated and it is returned.
 *
 * Returns: a #xstring_t containing the string
 *
 * Since: 2.24
 **/
xstring_t *
xvariant_print_string (xvariant_t *value,
                        xstring_t  *string,
                        xboolean_t  type_annotate)
{
  if G_UNLIKELY (string == NULL)
    string = xstring_new (NULL);

  switch (xvariant_classify (value))
    {
    case XVARIANT_CLASS_MAYBE:
      if (type_annotate)
        xstring_append_printf (string, "@%s ",
                                xvariant_get_type_string (value));

      if (xvariant_n_children (value))
        {
          xchar_t *printed_child;
          xvariant_t *element;

          /* Nested maybes:
           *
           * Consider the case of the type "mmi".  In this case we could
           * write "just just 4", but "4" alone is totally unambiguous,
           * so we try to drop "just" where possible.
           *
           * We have to be careful not to always drop "just", though,
           * since "nothing" needs to be distinguishable from "just
           * nothing".  The case where we need to ensure we keep the
           * "just" is actually exactly the case where we have a nested
           * Nothing.
           *
           * Instead of searching for that nested Nothing, we just print
           * the contained value into a separate string and see if we
           * end up with "nothing" at the end of it.  If so, we need to
           * add "just" at our level.
           */
          element = xvariant_get_child_value (value, 0);
          printed_child = xvariant_print (element, FALSE);
          xvariant_unref (element);

          if (xstr_has_suffix (printed_child, "nothing"))
            xstring_append (string, "just ");
          xstring_append (string, printed_child);
          g_free (printed_child);
        }
      else
        xstring_append (string, "nothing");

      break;

    case XVARIANT_CLASS_ARRAY:
      /* it's an array so the first character of the type string is 'a'
       *
       * if the first two characters are 'ay' then it's a bytestring.
       * under certain conditions we print those as strings.
       */
      if (xvariant_get_type_string (value)[1] == 'y')
        {
          const xchar_t *str;
          xsize_t size;
          xsize_t i;

          /* first determine if it is a byte string.
           * that's when there's a single nul character: at the end.
           */
          str = xvariant_get_data (value);
          size = xvariant_get_size (value);

          for (i = 0; i < size; i++)
            if (str[i] == '\0')
              break;

          /* first nul byte is the last byte -> it's a byte string. */
          if (i == size - 1)
            {
              xchar_t *escaped = xstrescape (str, NULL);

              /* use double quotes only if a ' is in the string */
              if (strchr (str, '\''))
                xstring_append_printf (string, "b\"%s\"", escaped);
              else
                xstring_append_printf (string, "b'%s'", escaped);

              g_free (escaped);
              break;
            }

          else
            {
              /* fall through and handle normally... */
            }
        }

      /*
       * if the first two characters are 'a{' then it's an array of
       * dictionary entries (ie: a dictionary) so we print that
       * differently.
       */
      if (xvariant_get_type_string (value)[1] == '{')
        /* dictionary */
        {
          const xchar_t *comma = "";
          xsize_t n, i;

          if ((n = xvariant_n_children (value)) == 0)
            {
              if (type_annotate)
                xstring_append_printf (string, "@%s ",
                                        xvariant_get_type_string (value));
              xstring_append (string, "{}");
              break;
            }

          xstring_append_c (string, '{');
          for (i = 0; i < n; i++)
            {
              xvariant_t *entry, *key, *val;

              xstring_append (string, comma);
              comma = ", ";

              entry = xvariant_get_child_value (value, i);
              key = xvariant_get_child_value (entry, 0);
              val = xvariant_get_child_value (entry, 1);
              xvariant_unref (entry);

              xvariant_print_string (key, string, type_annotate);
              xvariant_unref (key);
              xstring_append (string, ": ");
              xvariant_print_string (val, string, type_annotate);
              xvariant_unref (val);
              type_annotate = FALSE;
            }
          xstring_append_c (string, '}');
        }
      else
        /* normal (non-dictionary) array */
        {
          const xchar_t *comma = "";
          xsize_t n, i;

          if ((n = xvariant_n_children (value)) == 0)
            {
              if (type_annotate)
                xstring_append_printf (string, "@%s ",
                                        xvariant_get_type_string (value));
              xstring_append (string, "[]");
              break;
            }

          xstring_append_c (string, '[');
          for (i = 0; i < n; i++)
            {
              xvariant_t *element;

              xstring_append (string, comma);
              comma = ", ";

              element = xvariant_get_child_value (value, i);

              xvariant_print_string (element, string, type_annotate);
              xvariant_unref (element);
              type_annotate = FALSE;
            }
          xstring_append_c (string, ']');
        }

      break;

    case XVARIANT_CLASS_TUPLE:
      {
        xsize_t n, i;

        n = xvariant_n_children (value);

        xstring_append_c (string, '(');
        for (i = 0; i < n; i++)
          {
            xvariant_t *element;

            element = xvariant_get_child_value (value, i);
            xvariant_print_string (element, string, type_annotate);
            xstring_append (string, ", ");
            xvariant_unref (element);
          }

        /* for >1 item:  remove final ", "
         * for 1 item:   remove final " ", but leave the ","
         * for 0 items:  there is only "(", so remove nothing
         */
        xstring_truncate (string, string->len - (n > 0) - (n > 1));
        xstring_append_c (string, ')');
      }
      break;

    case XVARIANT_CLASS_DICT_ENTRY:
      {
        xvariant_t *element;

        xstring_append_c (string, '{');

        element = xvariant_get_child_value (value, 0);
        xvariant_print_string (element, string, type_annotate);
        xvariant_unref (element);

        xstring_append (string, ", ");

        element = xvariant_get_child_value (value, 1);
        xvariant_print_string (element, string, type_annotate);
        xvariant_unref (element);

        xstring_append_c (string, '}');
      }
      break;

    case XVARIANT_CLASS_VARIANT:
      {
        xvariant_t *child = xvariant_get_variant (value);

        /* Always annotate types in nested variants, because they are
         * (by nature) of variable type.
         */
        xstring_append_c (string, '<');
        xvariant_print_string (child, string, TRUE);
        xstring_append_c (string, '>');

        xvariant_unref (child);
      }
      break;

    case XVARIANT_CLASS_BOOLEAN:
      if (xvariant_get_boolean (value))
        xstring_append (string, "true");
      else
        xstring_append (string, "false");
      break;

    case XVARIANT_CLASS_STRING:
      {
        const xchar_t *str = xvariant_get_string (value, NULL);
        xunichar_t quote = strchr (str, '\'') ? '"' : '\'';

        xstring_append_c (string, quote);

        while (*str)
          {
            xunichar_t c = xutf8_get_char (str);

            if (c == quote || c == '\\')
              xstring_append_c (string, '\\');

            if (xunichar_isprint (c))
              xstring_append_unichar (string, c);

            else
              {
                xstring_append_c (string, '\\');
                if (c < 0x10000)
                  switch (c)
                    {
                    case '\a':
                      xstring_append_c (string, 'a');
                      break;

                    case '\b':
                      xstring_append_c (string, 'b');
                      break;

                    case '\f':
                      xstring_append_c (string, 'f');
                      break;

                    case '\n':
                      xstring_append_c (string, 'n');
                      break;

                    case '\r':
                      xstring_append_c (string, 'r');
                      break;

                    case '\t':
                      xstring_append_c (string, 't');
                      break;

                    case '\v':
                      xstring_append_c (string, 'v');
                      break;

                    default:
                      xstring_append_printf (string, "u%04x", c);
                      break;
                    }
                 else
                   xstring_append_printf (string, "U%08x", c);
              }

            str = xutf8_next_char (str);
          }

        xstring_append_c (string, quote);
      }
      break;

    case XVARIANT_CLASS_BYTE:
      if (type_annotate)
        xstring_append (string, "byte ");
      xstring_append_printf (string, "0x%02x",
                              xvariant_get_byte (value));
      break;

    case XVARIANT_CLASS_INT16:
      if (type_annotate)
        xstring_append (string, "int16 ");
      xstring_append_printf (string, "%"G_GINT16_FORMAT,
                              xvariant_get_int16 (value));
      break;

    case XVARIANT_CLASS_UINT16:
      if (type_annotate)
        xstring_append (string, "uint16 ");
      xstring_append_printf (string, "%"G_GUINT16_FORMAT,
                              xvariant_get_uint16 (value));
      break;

    case XVARIANT_CLASS_INT32:
      /* Never annotate this type because it is the default for numbers
       * (and this is a *pretty* printer)
       */
      xstring_append_printf (string, "%"G_GINT32_FORMAT,
                              xvariant_get_int32 (value));
      break;

    case XVARIANT_CLASS_HANDLE:
      if (type_annotate)
        xstring_append (string, "handle ");
      xstring_append_printf (string, "%"G_GINT32_FORMAT,
                              xvariant_get_handle (value));
      break;

    case XVARIANT_CLASS_UINT32:
      if (type_annotate)
        xstring_append (string, "uint32 ");
      xstring_append_printf (string, "%"G_GUINT32_FORMAT,
                              xvariant_get_uint32 (value));
      break;

    case XVARIANT_CLASS_INT64:
      if (type_annotate)
        xstring_append (string, "int64 ");
      xstring_append_printf (string, "%"G_GINT64_FORMAT,
                              xvariant_get_int64 (value));
      break;

    case XVARIANT_CLASS_UINT64:
      if (type_annotate)
        xstring_append (string, "uint64 ");
      xstring_append_printf (string, "%"G_GUINT64_FORMAT,
                              xvariant_get_uint64 (value));
      break;

    case XVARIANT_CLASS_DOUBLE:
      {
        xchar_t buffer[100];
        xint_t i;

        g_ascii_dtostr (buffer, sizeof buffer, xvariant_get_double (value));

        for (i = 0; buffer[i]; i++)
          if (buffer[i] == '.' || buffer[i] == 'e' ||
              buffer[i] == 'n' || buffer[i] == 'N')
            break;

        /* if there is no '.' or 'e' in the float then add one */
        if (buffer[i] == '\0')
          {
            buffer[i++] = '.';
            buffer[i++] = '0';
            buffer[i++] = '\0';
          }

        xstring_append (string, buffer);
      }
      break;

    case XVARIANT_CLASS_OBJECT_PATH:
      if (type_annotate)
        xstring_append (string, "objectpath ");
      xstring_append_printf (string, "\'%s\'",
                              xvariant_get_string (value, NULL));
      break;

    case XVARIANT_CLASS_SIGNATURE:
      if (type_annotate)
        xstring_append (string, "signature ");
      xstring_append_printf (string, "\'%s\'",
                              xvariant_get_string (value, NULL));
      break;

    default:
      g_assert_not_reached ();
  }

  return string;
}

/**
 * xvariant_print:
 * @value: a #xvariant_t
 * @type_annotate: %TRUE if type information should be included in
 *                 the output
 *
 * Pretty-prints @value in the format understood by xvariant_parse().
 *
 * The format is described [here][gvariant-text].
 *
 * If @type_annotate is %TRUE, then type information is included in
 * the output.
 *
 * Returns: (transfer full): a newly-allocated string holding the result.
 *
 * Since: 2.24
 */
xchar_t *
xvariant_print (xvariant_t *value,
                 xboolean_t  type_annotate)
{
  return xstring_free (xvariant_print_string (value, NULL, type_annotate),
                        FALSE);
}

/* Hash, Equal, Compare {{{1 */
/**
 * xvariant_hash:
 * @value: (type xvariant_t): a basic #xvariant_t value as a #xconstpointer
 *
 * Generates a hash value for a #xvariant_t instance.
 *
 * The output of this function is guaranteed to be the same for a given
 * value only per-process.  It may change between different processor
 * architectures or even different versions of GLib.  Do not use this
 * function as a basis for building protocols or file formats.
 *
 * The type of @value is #xconstpointer only to allow use of this
 * function with #xhashtable_t.  @value must be a #xvariant_t.
 *
 * Returns: a hash value corresponding to @value
 *
 * Since: 2.24
 **/
xuint_t
xvariant_hash (xconstpointer value_)
{
  xvariant_t *value = (xvariant_t *) value_;

  switch (xvariant_classify (value))
    {
    case XVARIANT_CLASS_STRING:
    case XVARIANT_CLASS_OBJECT_PATH:
    case XVARIANT_CLASS_SIGNATURE:
      return xstr_hash (xvariant_get_string (value, NULL));

    case XVARIANT_CLASS_BOOLEAN:
      /* this is a very odd thing to hash... */
      return xvariant_get_boolean (value);

    case XVARIANT_CLASS_BYTE:
      return xvariant_get_byte (value);

    case XVARIANT_CLASS_INT16:
    case XVARIANT_CLASS_UINT16:
      {
        const xuint16_t *ptr;

        ptr = xvariant_get_data (value);

        if (ptr)
          return *ptr;
        else
          return 0;
      }

    case XVARIANT_CLASS_INT32:
    case XVARIANT_CLASS_UINT32:
    case XVARIANT_CLASS_HANDLE:
      {
        const xuint_t *ptr;

        ptr = xvariant_get_data (value);

        if (ptr)
          return *ptr;
        else
          return 0;
      }

    case XVARIANT_CLASS_INT64:
    case XVARIANT_CLASS_UINT64:
    case XVARIANT_CLASS_DOUBLE:
      /* need a separate case for these guys because otherwise
       * performance could be quite bad on big endian systems
       */
      {
        const xuint_t *ptr;

        ptr = xvariant_get_data (value);

        if (ptr)
          return ptr[0] + ptr[1];
        else
          return 0;
      }

    default:
      g_return_val_if_fail (!xvariant_is_container (value), 0);
      g_assert_not_reached ();
    }
}

/**
 * xvariant_equal:
 * @one: (type xvariant_t): a #xvariant_t instance
 * @two: (type xvariant_t): a #xvariant_t instance
 *
 * Checks if @one and @two have the same type and value.
 *
 * The types of @one and @two are #xconstpointer only to allow use of
 * this function with #xhashtable_t.  They must each be a #xvariant_t.
 *
 * Returns: %TRUE if @one and @two are equal
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_equal (xconstpointer one,
                 xconstpointer two)
{
  xboolean_t equal;

  g_return_val_if_fail (one != NULL && two != NULL, FALSE);

  if (xvariant_get_type_info ((xvariant_t *) one) !=
      xvariant_get_type_info ((xvariant_t *) two))
    return FALSE;

  /* if both values are trusted to be in their canonical serialized form
   * then a simple memcmp() of their serialized data will answer the
   * question.
   *
   * if not, then this might generate a false negative (since it is
   * possible for two different byte sequences to represent the same
   * value).  for now we solve this by pretty-printing both values and
   * comparing the result.
   */
  if (xvariant_is_trusted ((xvariant_t *) one) &&
      xvariant_is_trusted ((xvariant_t *) two))
    {
      xconstpointer data_one, data_two;
      xsize_t size_one, size_two;

      size_one = xvariant_get_size ((xvariant_t *) one);
      size_two = xvariant_get_size ((xvariant_t *) two);

      if (size_one != size_two)
        return FALSE;

      data_one = xvariant_get_data ((xvariant_t *) one);
      data_two = xvariant_get_data ((xvariant_t *) two);

      if (size_one)
        equal = memcmp (data_one, data_two, size_one) == 0;
      else
        equal = TRUE;
    }
  else
    {
      xchar_t *strone, *strtwo;

      strone = xvariant_print ((xvariant_t *) one, FALSE);
      strtwo = xvariant_print ((xvariant_t *) two, FALSE);
      equal = strcmp (strone, strtwo) == 0;
      g_free (strone);
      g_free (strtwo);
    }

  return equal;
}

/**
 * xvariant_compare:
 * @one: (type xvariant_t): a basic-typed #xvariant_t instance
 * @two: (type xvariant_t): a #xvariant_t instance of the same type
 *
 * Compares @one and @two.
 *
 * The types of @one and @two are #xconstpointer only to allow use of
 * this function with #xtree_t, #xptr_array_t, etc.  They must each be a
 * #xvariant_t.
 *
 * Comparison is only defined for basic types (ie: booleans, numbers,
 * strings).  For booleans, %FALSE is less than %TRUE.  Numbers are
 * ordered in the usual way.  Strings are in ASCII lexographical order.
 *
 * It is a programmer error to attempt to compare container values or
 * two values that have types that are not exactly equal.  For example,
 * you cannot compare a 32-bit signed integer with a 32-bit unsigned
 * integer.  Also note that this function is not particularly
 * well-behaved when it comes to comparison of doubles; in particular,
 * the handling of incomparable values (ie: NaN) is undefined.
 *
 * If you only require an equality comparison, xvariant_equal() is more
 * general.
 *
 * Returns: negative value if a < b;
 *          zero if a = b;
 *          positive value if a > b.
 *
 * Since: 2.26
 **/
xint_t
xvariant_compare (xconstpointer one,
                   xconstpointer two)
{
  xvariant_t *a = (xvariant_t *) one;
  xvariant_t *b = (xvariant_t *) two;

  g_return_val_if_fail (xvariant_classify (a) == xvariant_classify (b), 0);

  switch (xvariant_classify (a))
    {
    case XVARIANT_CLASS_BOOLEAN:
      return xvariant_get_boolean (a) -
             xvariant_get_boolean (b);

    case XVARIANT_CLASS_BYTE:
      return ((xint_t) xvariant_get_byte (a)) -
             ((xint_t) xvariant_get_byte (b));

    case XVARIANT_CLASS_INT16:
      return ((xint_t) xvariant_get_int16 (a)) -
             ((xint_t) xvariant_get_int16 (b));

    case XVARIANT_CLASS_UINT16:
      return ((xint_t) xvariant_get_uint16 (a)) -
             ((xint_t) xvariant_get_uint16 (b));

    case XVARIANT_CLASS_INT32:
      {
        gint32 a_val = xvariant_get_int32 (a);
        gint32 b_val = xvariant_get_int32 (b);

        return (a_val == b_val) ? 0 : (a_val > b_val) ? 1 : -1;
      }

    case XVARIANT_CLASS_UINT32:
      {
        xuint32_t a_val = xvariant_get_uint32 (a);
        xuint32_t b_val = xvariant_get_uint32 (b);

        return (a_val == b_val) ? 0 : (a_val > b_val) ? 1 : -1;
      }

    case XVARIANT_CLASS_INT64:
      {
        sint64_t a_val = xvariant_get_int64 (a);
        sint64_t b_val = xvariant_get_int64 (b);

        return (a_val == b_val) ? 0 : (a_val > b_val) ? 1 : -1;
      }

    case XVARIANT_CLASS_UINT64:
      {
        xuint64_t a_val = xvariant_get_uint64 (a);
        xuint64_t b_val = xvariant_get_uint64 (b);

        return (a_val == b_val) ? 0 : (a_val > b_val) ? 1 : -1;
      }

    case XVARIANT_CLASS_DOUBLE:
      {
        xdouble_t a_val = xvariant_get_double (a);
        xdouble_t b_val = xvariant_get_double (b);

        return (a_val == b_val) ? 0 : (a_val > b_val) ? 1 : -1;
      }

    case XVARIANT_CLASS_STRING:
    case XVARIANT_CLASS_OBJECT_PATH:
    case XVARIANT_CLASS_SIGNATURE:
      return strcmp (xvariant_get_string (a, NULL),
                     xvariant_get_string (b, NULL));

    default:
      g_return_val_if_fail (!xvariant_is_container (a), 0);
      g_assert_not_reached ();
    }
}

/* xvariant_iter_t {{{1 */
/**
 * xvariant_iter_t: (skip)
 *
 * #xvariant_iter_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/
struct stack_iter
{
  xvariant_t *value;
  xssize_t n, i;

  const xchar_t *loop_format;

  xsize_t padding[3];
  xsize_t magic;
};

G_STATIC_ASSERT (sizeof (struct stack_iter) <= sizeof (xvariant_iter_t));

struct heap_iter
{
  struct stack_iter iter;

  xvariant_t *value_ref;
  xsize_t magic;
};

#define GVSI(i)                 ((struct stack_iter *) (i))
#define GVHI(i)                 ((struct heap_iter *) (i))
#define GVSI_MAGIC              ((xsize_t) 3579507750u)
#define GVHI_MAGIC              ((xsize_t) 1450270775u)
#define is_valid_iter(i)        (i != NULL && \
                                 GVSI(i)->magic == GVSI_MAGIC)
#define is_valid_heap_iter(i)   (is_valid_iter(i) && \
                                 GVHI(i)->magic == GVHI_MAGIC)

/**
 * xvariant_iter_new:
 * @value: a container #xvariant_t
 *
 * Creates a heap-allocated #xvariant_iter_t for iterating over the items
 * in @value.
 *
 * Use xvariant_iter_free() to free the return value when you no longer
 * need it.
 *
 * A reference is taken to @value and will be released only when
 * xvariant_iter_free() is called.
 *
 * Returns: (transfer full): a new heap-allocated #xvariant_iter_t
 *
 * Since: 2.24
 **/
xvariant_iter_t *
xvariant_iter_new (xvariant_t *value)
{
  xvariant_iter_t *iter;

  iter = (xvariant_iter_t *) g_slice_new (struct heap_iter);
  GVHI(iter)->value_ref = xvariant_ref (value);
  GVHI(iter)->magic = GVHI_MAGIC;

  xvariant_iter_init (iter, value);

  return iter;
}

/**
 * xvariant_iter_init: (skip)
 * @iter: a pointer to a #xvariant_iter_t
 * @value: a container #xvariant_t
 *
 * Initialises (without allocating) a #xvariant_iter_t.  @iter may be
 * completely uninitialised prior to this call; its old value is
 * ignored.
 *
 * The iterator remains valid for as long as @value exists, and need not
 * be freed in any way.
 *
 * Returns: the number of items in @value
 *
 * Since: 2.24
 **/
xsize_t
xvariant_iter_init (xvariant_iter_t *iter,
                     xvariant_t     *value)
{
  GVSI(iter)->magic = GVSI_MAGIC;
  GVSI(iter)->value = value;
  GVSI(iter)->n = xvariant_n_children (value);
  GVSI(iter)->i = -1;
  GVSI(iter)->loop_format = NULL;

  return GVSI(iter)->n;
}

/**
 * xvariant_iter_copy:
 * @iter: a #xvariant_iter_t
 *
 * Creates a new heap-allocated #xvariant_iter_t to iterate over the
 * container that was being iterated over by @iter.  Iteration begins on
 * the new iterator from the current position of the old iterator but
 * the two copies are independent past that point.
 *
 * Use xvariant_iter_free() to free the return value when you no longer
 * need it.
 *
 * A reference is taken to the container that @iter is iterating over
 * and will be related only when xvariant_iter_free() is called.
 *
 * Returns: (transfer full): a new heap-allocated #xvariant_iter_t
 *
 * Since: 2.24
 **/
xvariant_iter_t *
xvariant_iter_copy (xvariant_iter_t *iter)
{
  xvariant_iter_t *copy;

  g_return_val_if_fail (is_valid_iter (iter), 0);

  copy = xvariant_iter_new (GVSI(iter)->value);
  GVSI(copy)->i = GVSI(iter)->i;

  return copy;
}

/**
 * xvariant_iter_n_children:
 * @iter: a #xvariant_iter_t
 *
 * Queries the number of child items in the container that we are
 * iterating over.  This is the total number of items -- not the number
 * of items remaining.
 *
 * This function might be useful for preallocation of arrays.
 *
 * Returns: the number of children in the container
 *
 * Since: 2.24
 **/
xsize_t
xvariant_iter_n_children (xvariant_iter_t *iter)
{
  g_return_val_if_fail (is_valid_iter (iter), 0);

  return GVSI(iter)->n;
}

/**
 * xvariant_iter_free:
 * @iter: (transfer full): a heap-allocated #xvariant_iter_t
 *
 * Frees a heap-allocated #xvariant_iter_t.  Only call this function on
 * iterators that were returned by xvariant_iter_new() or
 * xvariant_iter_copy().
 *
 * Since: 2.24
 **/
void
xvariant_iter_free (xvariant_iter_t *iter)
{
  g_return_if_fail (is_valid_heap_iter (iter));

  xvariant_unref (GVHI(iter)->value_ref);
  GVHI(iter)->magic = 0;

  g_slice_free (struct heap_iter, GVHI(iter));
}

/**
 * xvariant_iter_next_value:
 * @iter: a #xvariant_iter_t
 *
 * Gets the next item in the container.  If no more items remain then
 * %NULL is returned.
 *
 * Use xvariant_unref() to drop your reference on the return value when
 * you no longer need it.
 *
 * Here is an example for iterating with xvariant_iter_next_value():
 * |[<!-- language="C" -->
 *   // recursively iterate a container
 *   void
 *   iterate_container_recursive (xvariant_t *container)
 *   {
 *     xvariant_iter_t iter;
 *     xvariant_t *child;
 *
 *     xvariant_iter_init (&iter, container);
 *     while ((child = xvariant_iter_next_value (&iter)))
 *       {
 *         g_print ("type '%s'\n", xvariant_get_type_string (child));
 *
 *         if (xvariant_is_container (child))
 *           iterate_container_recursive (child);
 *
 *         xvariant_unref (child);
 *       }
 *   }
 * ]|
 *
 * Returns: (nullable) (transfer full): a #xvariant_t, or %NULL
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_iter_next_value (xvariant_iter_t *iter)
{
  g_return_val_if_fail (is_valid_iter (iter), FALSE);

  if G_UNLIKELY (GVSI(iter)->i >= GVSI(iter)->n)
    {
      g_critical ("xvariant_iter_next_value: must not be called again "
                  "after NULL has already been returned.");
      return NULL;
    }

  GVSI(iter)->i++;

  if (GVSI(iter)->i < GVSI(iter)->n)
    return xvariant_get_child_value (GVSI(iter)->value, GVSI(iter)->i);

  return NULL;
}

/* xvariant_builder_t {{{1 */
/**
 * xvariant_builder_t:
 *
 * A utility type for constructing container-type #xvariant_t instances.
 *
 * This is an opaque structure and may only be accessed using the
 * following functions.
 *
 * #xvariant_builder_t is not threadsafe in any way.  Do not attempt to
 * access it from more than one thread.
 **/

struct stack_builder
{
  xvariant_builder_t *parent;
  xvariant_type_t *type;

  /* type constraint explicitly specified by 'type'.
   * for tuple types, this moves along as we add more items.
   */
  const xvariant_type_t *expected_type;

  /* type constraint implied by previous array item.
   */
  const xvariant_type_t *prev_item_type;

  /* constraints on the number of children.  max = -1 for unlimited. */
  xsize_t min_items;
  xsize_t max_items;

  /* dynamically-growing pointer array */
  xvariant_t **children;
  xsize_t allocated_children;
  xsize_t offset;

  /* set to '1' if all items in the container will have the same type
   * (ie: maybe, array, variant) '0' if not (ie: tuple, dict entry)
   */
  xuint_t uniform_item_types : 1;

  /* set to '1' initially and changed to '0' if an untrusted value is
   * added
   */
  xuint_t trusted : 1;

  xsize_t magic;
};

G_STATIC_ASSERT (sizeof (struct stack_builder) <= sizeof (xvariant_builder_t));

struct heap_builder
{
  xvariant_builder_t builder;
  xsize_t magic;

  xint_t ref_count;
};

#define GVSB(b)                  ((struct stack_builder *) (b))
#define GVHB(b)                  ((struct heap_builder *) (b))
#define GVSB_MAGIC               ((xsize_t) 1033660112u)
#define GVSB_MAGIC_PARTIAL       ((xsize_t) 2942751021u)
#define GVHB_MAGIC               ((xsize_t) 3087242682u)
#define is_valid_builder(b)      (GVSB(b)->magic == GVSB_MAGIC)
#define is_valid_heap_builder(b) (GVHB(b)->magic == GVHB_MAGIC)

/* Just to make sure that by adding a union to xvariant_builder_t, we
 * didn't accidentally change ABI. */
G_STATIC_ASSERT (sizeof (xvariant_builder_t) == sizeof (xsize_t[16]));

static xboolean_t
ensure_valid_builder (xvariant_builder_t *builder)
{
  if (builder == NULL)
    return FALSE;
  else if (is_valid_builder (builder))
    return TRUE;
  if (builder->u.s.partial_magic == GVSB_MAGIC_PARTIAL)
    {
      static xvariant_builder_t cleared_builder;

      /* Make sure that only first two fields were set and the rest is
       * zeroed to avoid messing up the builder that had parent
       * address equal to GVSB_MAGIC_PARTIAL. */
      if (memcmp (cleared_builder.u.s.y, builder->u.s.y, sizeof cleared_builder.u.s.y))
        return FALSE;

      xvariant_builder_init (builder, builder->u.s.type);
    }
  return is_valid_builder (builder);
}

/* return_if_invalid_builder (b) is like
 * g_return_if_fail (ensure_valid_builder (b)), except that
 * the side effects of ensure_valid_builder are evaluated
 * regardless of whether G_DISABLE_CHECKS is defined or not. */
#define return_if_invalid_builder(b) G_STMT_START {                \
  xboolean_t valid_builder G_GNUC_UNUSED = ensure_valid_builder (b); \
  g_return_if_fail (valid_builder);                                \
} G_STMT_END

/* return_val_if_invalid_builder (b, val) is like
 * g_return_val_if_fail (ensure_valid_builder (b), val), except that
 * the side effects of ensure_valid_builder are evaluated
 * regardless of whether G_DISABLE_CHECKS is defined or not. */
#define return_val_if_invalid_builder(b, val) G_STMT_START {       \
  xboolean_t valid_builder G_GNUC_UNUSED = ensure_valid_builder (b); \
  g_return_val_if_fail (valid_builder, val);                       \
} G_STMT_END

/**
 * xvariant_builder_new:
 * @type: a container type
 *
 * Allocates and initialises a new #xvariant_builder_t.
 *
 * You should call xvariant_builder_unref() on the return value when it
 * is no longer needed.  The memory will not be automatically freed by
 * any other call.
 *
 * In most cases it is easier to place a #xvariant_builder_t directly on
 * the stack of the calling function and initialise it with
 * xvariant_builder_init().
 *
 * Returns: (transfer full): a #xvariant_builder_t
 *
 * Since: 2.24
 **/
xvariant_builder_t *
xvariant_builder_new (const xvariant_type_t *type)
{
  xvariant_builder_t *builder;

  builder = (xvariant_builder_t *) g_slice_new (struct heap_builder);
  xvariant_builder_init (builder, type);
  GVHB(builder)->magic = GVHB_MAGIC;
  GVHB(builder)->ref_count = 1;

  return builder;
}

/**
 * xvariant_builder_unref:
 * @builder: (transfer full): a #xvariant_builder_t allocated by xvariant_builder_new()
 *
 * Decreases the reference count on @builder.
 *
 * In the event that there are no more references, releases all memory
 * associated with the #xvariant_builder_t.
 *
 * Don't call this on stack-allocated #xvariant_builder_t instances or bad
 * things will happen.
 *
 * Since: 2.24
 **/
void
xvariant_builder_unref (xvariant_builder_t *builder)
{
  g_return_if_fail (is_valid_heap_builder (builder));

  if (--GVHB(builder)->ref_count)
    return;

  xvariant_builder_clear (builder);
  GVHB(builder)->magic = 0;

  g_slice_free (struct heap_builder, GVHB(builder));
}

/**
 * xvariant_builder_ref:
 * @builder: a #xvariant_builder_t allocated by xvariant_builder_new()
 *
 * Increases the reference count on @builder.
 *
 * Don't call this on stack-allocated #xvariant_builder_t instances or bad
 * things will happen.
 *
 * Returns: (transfer full): a new reference to @builder
 *
 * Since: 2.24
 **/
xvariant_builder_t *
xvariant_builder_ref (xvariant_builder_t *builder)
{
  g_return_val_if_fail (is_valid_heap_builder (builder), NULL);

  GVHB(builder)->ref_count++;

  return builder;
}

/**
 * xvariant_builder_clear: (skip)
 * @builder: a #xvariant_builder_t
 *
 * Releases all memory associated with a #xvariant_builder_t without
 * freeing the #xvariant_builder_t structure itself.
 *
 * It typically only makes sense to do this on a stack-allocated
 * #xvariant_builder_t if you want to abort building the value part-way
 * through.  This function need not be called if you call
 * xvariant_builder_end() and it also doesn't need to be called on
 * builders allocated with xvariant_builder_new() (see
 * xvariant_builder_unref() for that).
 *
 * This function leaves the #xvariant_builder_t structure set to all-zeros.
 * It is valid to call this function on either an initialised
 * #xvariant_builder_t or one that is set to all-zeros but it is not valid
 * to call this function on uninitialised memory.
 *
 * Since: 2.24
 **/
void
xvariant_builder_clear (xvariant_builder_t *builder)
{
  xsize_t i;

  if (GVSB(builder)->magic == 0)
    /* all-zeros or partial case */
    return;

  return_if_invalid_builder (builder);

  xvariant_type_free (GVSB(builder)->type);

  for (i = 0; i < GVSB(builder)->offset; i++)
    xvariant_unref (GVSB(builder)->children[i]);

  g_free (GVSB(builder)->children);

  if (GVSB(builder)->parent)
    {
      xvariant_builder_clear (GVSB(builder)->parent);
      g_slice_free (xvariant_builder_t, GVSB(builder)->parent);
    }

  memset (builder, 0, sizeof (xvariant_builder_t));
}

/**
 * xvariant_builder_init: (skip)
 * @builder: a #xvariant_builder_t
 * @type: a container type
 *
 * Initialises a #xvariant_builder_t structure.
 *
 * @type must be non-%NULL.  It specifies the type of container to
 * construct.  It can be an indefinite type such as
 * %G_VARIANT_TYPE_ARRAY or a definite type such as "as" or "(ii)".
 * Maybe, array, tuple, dictionary entry and variant-typed values may be
 * constructed.
 *
 * After the builder is initialised, values are added using
 * xvariant_builder_add_value() or xvariant_builder_add().
 *
 * After all the child values are added, xvariant_builder_end() frees
 * the memory associated with the builder and returns the #xvariant_t that
 * was created.
 *
 * This function completely ignores the previous contents of @builder.
 * On one hand this means that it is valid to pass in completely
 * uninitialised memory.  On the other hand, this means that if you are
 * initialising over top of an existing #xvariant_builder_t you need to
 * first call xvariant_builder_clear() in order to avoid leaking
 * memory.
 *
 * You must not call xvariant_builder_ref() or
 * xvariant_builder_unref() on a #xvariant_builder_t that was initialised
 * with this function.  If you ever pass a reference to a
 * #xvariant_builder_t outside of the control of your own code then you
 * should assume that the person receiving that reference may try to use
 * reference counting; you should use xvariant_builder_new() instead of
 * this function.
 *
 * Since: 2.24
 **/
void
xvariant_builder_init (xvariant_builder_t    *builder,
                        const xvariant_type_t *type)
{
  g_return_if_fail (type != NULL);
  g_return_if_fail (xvariant_type_is_container (type));

  memset (builder, 0, sizeof (xvariant_builder_t));

  GVSB(builder)->type = xvariant_type_copy (type);
  GVSB(builder)->magic = GVSB_MAGIC;
  GVSB(builder)->trusted = TRUE;

  switch (*(const xchar_t *) type)
    {
    case XVARIANT_CLASS_VARIANT:
      GVSB(builder)->uniform_item_types = TRUE;
      GVSB(builder)->allocated_children = 1;
      GVSB(builder)->expected_type = NULL;
      GVSB(builder)->min_items = 1;
      GVSB(builder)->max_items = 1;
      break;

    case XVARIANT_CLASS_ARRAY:
      GVSB(builder)->uniform_item_types = TRUE;
      GVSB(builder)->allocated_children = 8;
      GVSB(builder)->expected_type =
        xvariant_type_element (GVSB(builder)->type);
      GVSB(builder)->min_items = 0;
      GVSB(builder)->max_items = -1;
      break;

    case XVARIANT_CLASS_MAYBE:
      GVSB(builder)->uniform_item_types = TRUE;
      GVSB(builder)->allocated_children = 1;
      GVSB(builder)->expected_type =
        xvariant_type_element (GVSB(builder)->type);
      GVSB(builder)->min_items = 0;
      GVSB(builder)->max_items = 1;
      break;

    case XVARIANT_CLASS_DICT_ENTRY:
      GVSB(builder)->uniform_item_types = FALSE;
      GVSB(builder)->allocated_children = 2;
      GVSB(builder)->expected_type =
        xvariant_type_key (GVSB(builder)->type);
      GVSB(builder)->min_items = 2;
      GVSB(builder)->max_items = 2;
      break;

    case 'r': /* G_VARIANT_TYPE_TUPLE was given */
      GVSB(builder)->uniform_item_types = FALSE;
      GVSB(builder)->allocated_children = 8;
      GVSB(builder)->expected_type = NULL;
      GVSB(builder)->min_items = 0;
      GVSB(builder)->max_items = -1;
      break;

    case XVARIANT_CLASS_TUPLE: /* a definite tuple type was given */
      GVSB(builder)->allocated_children = xvariant_type_n_items (type);
      GVSB(builder)->expected_type =
        xvariant_type_first (GVSB(builder)->type);
      GVSB(builder)->min_items = GVSB(builder)->allocated_children;
      GVSB(builder)->max_items = GVSB(builder)->allocated_children;
      GVSB(builder)->uniform_item_types = FALSE;
      break;

    default:
      g_assert_not_reached ();
   }

  GVSB(builder)->children = g_new (xvariant_t *,
                                   GVSB(builder)->allocated_children);
}

static void
xvariant_builder_make_room (struct stack_builder *builder)
{
  if (builder->offset == builder->allocated_children)
    {
      builder->allocated_children *= 2;
      builder->children = g_renew (xvariant_t *, builder->children,
                                   builder->allocated_children);
    }
}

/**
 * xvariant_builder_add_value:
 * @builder: a #xvariant_builder_t
 * @value: a #xvariant_t
 *
 * Adds @value to @builder.
 *
 * It is an error to call this function in any way that would create an
 * inconsistent value to be constructed.  Some examples of this are
 * putting different types of items into an array, putting the wrong
 * types or number of items in a tuple, putting more than one value into
 * a variant, etc.
 *
 * If @value is a floating reference (see xvariant_ref_sink()),
 * the @builder instance takes ownership of @value.
 *
 * Since: 2.24
 **/
void
xvariant_builder_add_value (xvariant_builder_t *builder,
                             xvariant_t        *value)
{
  return_if_invalid_builder (builder);
  g_return_if_fail (GVSB(builder)->offset < GVSB(builder)->max_items);
  g_return_if_fail (!GVSB(builder)->expected_type ||
                    xvariant_is_of_type (value,
                                          GVSB(builder)->expected_type));
  g_return_if_fail (!GVSB(builder)->prev_item_type ||
                    xvariant_is_of_type (value,
                                          GVSB(builder)->prev_item_type));

  GVSB(builder)->trusted &= xvariant_is_trusted (value);

  if (!GVSB(builder)->uniform_item_types)
    {
      /* advance our expected type pointers */
      if (GVSB(builder)->expected_type)
        GVSB(builder)->expected_type =
          xvariant_type_next (GVSB(builder)->expected_type);

      if (GVSB(builder)->prev_item_type)
        GVSB(builder)->prev_item_type =
          xvariant_type_next (GVSB(builder)->prev_item_type);
    }
  else
    GVSB(builder)->prev_item_type = xvariant_get_type (value);

  xvariant_builder_make_room (GVSB(builder));

  GVSB(builder)->children[GVSB(builder)->offset++] =
    xvariant_ref_sink (value);
}

/**
 * xvariant_builder_open:
 * @builder: a #xvariant_builder_t
 * @type: the #xvariant_type_t of the container
 *
 * Opens a subcontainer inside the given @builder.  When done adding
 * items to the subcontainer, xvariant_builder_close() must be called. @type
 * is the type of the container: so to build a tuple of several values, @type
 * must include the tuple itself.
 *
 * It is an error to call this function in any way that would cause an
 * inconsistent value to be constructed (ie: adding too many values or
 * a value of an incorrect type).
 *
 * Example of building a nested variant:
 * |[<!-- language="C" -->
 * xvariant_builder_t builder;
 * xuint32_t some_number = get_number ();
 * x_autoptr (xhashtable_t) some_dict = get_dict ();
 * xhash_table_iter_t iter;
 * const xchar_t *key;
 * const xvariant_t *value;
 * x_autoptr (xvariant_t) output = NULL;
 *
 * xvariant_builder_init (&builder, G_VARIANT_TYPE ("(ua{sv})"));
 * xvariant_builder_add (&builder, "u", some_number);
 * xvariant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));
 *
 * xhash_table_iter_init (&iter, some_dict);
 * while (xhash_table_iter_next (&iter, (xpointer_t *) &key, (xpointer_t *) &value))
 *   {
 *     xvariant_builder_open (&builder, G_VARIANT_TYPE ("{sv}"));
 *     xvariant_builder_add (&builder, "s", key);
 *     xvariant_builder_add (&builder, "v", value);
 *     xvariant_builder_close (&builder);
 *   }
 *
 * xvariant_builder_close (&builder);
 *
 * output = xvariant_builder_end (&builder);
 * ]|
 *
 * Since: 2.24
 **/
void
xvariant_builder_open (xvariant_builder_t    *builder,
                        const xvariant_type_t *type)
{
  xvariant_builder_t *parent;

  return_if_invalid_builder (builder);
  g_return_if_fail (GVSB(builder)->offset < GVSB(builder)->max_items);
  g_return_if_fail (!GVSB(builder)->expected_type ||
                    xvariant_type_is_subtype_of (type,
                                                  GVSB(builder)->expected_type));
  g_return_if_fail (!GVSB(builder)->prev_item_type ||
                    xvariant_type_is_subtype_of (GVSB(builder)->prev_item_type,
                                                  type));

  parent = g_slice_dup (xvariant_builder_t, builder);
  xvariant_builder_init (builder, type);
  GVSB(builder)->parent = parent;

  /* push the prev_item_type down into the subcontainer */
  if (GVSB(parent)->prev_item_type)
    {
      if (!GVSB(builder)->uniform_item_types)
        /* tuples and dict entries */
        GVSB(builder)->prev_item_type =
          xvariant_type_first (GVSB(parent)->prev_item_type);

      else if (!xvariant_type_is_variant (GVSB(builder)->type))
        /* maybes and arrays */
        GVSB(builder)->prev_item_type =
          xvariant_type_element (GVSB(parent)->prev_item_type);
    }
}

/**
 * xvariant_builder_close:
 * @builder: a #xvariant_builder_t
 *
 * Closes the subcontainer inside the given @builder that was opened by
 * the most recent call to xvariant_builder_open().
 *
 * It is an error to call this function in any way that would create an
 * inconsistent value to be constructed (ie: too few values added to the
 * subcontainer).
 *
 * Since: 2.24
 **/
void
xvariant_builder_close (xvariant_builder_t *builder)
{
  xvariant_builder_t *parent;

  return_if_invalid_builder (builder);
  g_return_if_fail (GVSB(builder)->parent != NULL);

  parent = GVSB(builder)->parent;
  GVSB(builder)->parent = NULL;

  xvariant_builder_add_value (parent, xvariant_builder_end (builder));
  *builder = *parent;

  g_slice_free (xvariant_builder_t, parent);
}

/*< private >
 * xvariant_make_maybe_type:
 * @element: a #xvariant_t
 *
 * Return the type of a maybe containing @element.
 */
static xvariant_type_t *
xvariant_make_maybe_type (xvariant_t *element)
{
  return xvariant_type_new_maybe (xvariant_get_type (element));
}

/*< private >
 * xvariant_make_array_type:
 * @element: a #xvariant_t
 *
 * Return the type of an array containing @element.
 */
static xvariant_type_t *
xvariant_make_array_type (xvariant_t *element)
{
  return xvariant_type_new_array (xvariant_get_type (element));
}

/**
 * xvariant_builder_end:
 * @builder: a #xvariant_builder_t
 *
 * Ends the builder process and returns the constructed value.
 *
 * It is not permissible to use @builder in any way after this call
 * except for reference counting operations (in the case of a
 * heap-allocated #xvariant_builder_t) or by reinitialising it with
 * xvariant_builder_init() (in the case of stack-allocated). This
 * means that for the stack-allocated builders there is no need to
 * call xvariant_builder_clear() after the call to
 * xvariant_builder_end().
 *
 * It is an error to call this function in any way that would create an
 * inconsistent value to be constructed (ie: insufficient number of
 * items added to a container with a specific number of children
 * required).  It is also an error to call this function if the builder
 * was created with an indefinite array or maybe type and no children
 * have been added; in this case it is impossible to infer the type of
 * the empty array.
 *
 * Returns: (transfer none): a new, floating, #xvariant_t
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_builder_end (xvariant_builder_t *builder)
{
  xvariant_type_t *my_type;
  xvariant_t *value;

  return_val_if_invalid_builder (builder, NULL);
  g_return_val_if_fail (GVSB(builder)->offset >= GVSB(builder)->min_items,
                        NULL);
  g_return_val_if_fail (!GVSB(builder)->uniform_item_types ||
                        GVSB(builder)->prev_item_type != NULL ||
                        xvariant_type_is_definite (GVSB(builder)->type),
                        NULL);

  if (xvariant_type_is_definite (GVSB(builder)->type))
    my_type = xvariant_type_copy (GVSB(builder)->type);

  else if (xvariant_type_is_maybe (GVSB(builder)->type))
    my_type = xvariant_make_maybe_type (GVSB(builder)->children[0]);

  else if (xvariant_type_is_array (GVSB(builder)->type))
    my_type = xvariant_make_array_type (GVSB(builder)->children[0]);

  else if (xvariant_type_is_tuple (GVSB(builder)->type))
    my_type = xvariant_make_tuple_type (GVSB(builder)->children,
                                         GVSB(builder)->offset);

  else if (xvariant_type_is_dict_entry (GVSB(builder)->type))
    my_type = xvariant_make_dict_entry_type (GVSB(builder)->children[0],
                                              GVSB(builder)->children[1]);
  else
    g_assert_not_reached ();

  value = xvariant_new_from_children (my_type,
                                       g_renew (xvariant_t *,
                                                GVSB(builder)->children,
                                                GVSB(builder)->offset),
                                       GVSB(builder)->offset,
                                       GVSB(builder)->trusted);
  GVSB(builder)->children = NULL;
  GVSB(builder)->offset = 0;

  xvariant_builder_clear (builder);
  xvariant_type_free (my_type);

  return value;
}

/* xvariant_dict_t {{{1 */

/**
 * xvariant_dict_t:
 *
 * #xvariant_dict_t is a mutable interface to #xvariant_t dictionaries.
 *
 * It can be used for doing a sequence of dictionary lookups in an
 * efficient way on an existing #xvariant_t dictionary or it can be used
 * to construct new dictionaries with a hashtable-like interface.  It
 * can also be used for taking existing dictionaries and modifying them
 * in order to create new ones.
 *
 * #xvariant_dict_t can only be used with %G_VARIANT_TYPE_VARDICT
 * dictionaries.
 *
 * It is possible to use #xvariant_dict_t allocated on the stack or on the
 * heap.  When using a stack-allocated #xvariant_dict_t, you begin with a
 * call to xvariant_dict_init() and free the resources with a call to
 * xvariant_dict_clear().
 *
 * Heap-allocated #xvariant_dict_t follows normal refcounting rules: you
 * allocate it with xvariant_dict_new() and use xvariant_dict_ref()
 * and xvariant_dict_unref().
 *
 * xvariant_dict_end() is used to convert the #xvariant_dict_t back into a
 * dictionary-type #xvariant_t.  When used with stack-allocated instances,
 * this also implicitly frees all associated memory, but for
 * heap-allocated instances, you must still call xvariant_dict_unref()
 * afterwards.
 *
 * You will typically want to use a heap-allocated #xvariant_dict_t when
 * you expose it as part of an API.  For most other uses, the
 * stack-allocated form will be more convenient.
 *
 * Consider the following two examples that do the same thing in each
 * style: take an existing dictionary and look up the "count" uint32
 * key, adding 1 to it if it is found, or returning an error if the
 * key is not found.  Each returns the new dictionary as a floating
 * #xvariant_t.
 *
 * ## Using a stack-allocated xvariant_dict_t
 *
 * |[<!-- language="C" -->
 *   xvariant_t *
 *   add_to_count (xvariant_t  *orig,
 *                 xerror_t   **error)
 *   {
 *     xvariant_dict_t dict;
 *     xuint32_t count;
 *
 *     xvariant_dict_init (&dict, orig);
 *     if (!xvariant_dict_lookup (&dict, "count", "u", &count))
 *       {
 *         g_set_error (...);
 *         xvariant_dict_clear (&dict);
 *         return NULL;
 *       }
 *
 *     xvariant_dict_insert (&dict, "count", "u", count + 1);
 *
 *     return xvariant_dict_end (&dict);
 *   }
 * ]|
 *
 * ## Using heap-allocated xvariant_dict_t
 *
 * |[<!-- language="C" -->
 *   xvariant_t *
 *   add_to_count (xvariant_t  *orig,
 *                 xerror_t   **error)
 *   {
 *     xvariant_dict_t *dict;
 *     xvariant_t *result;
 *     xuint32_t count;
 *
 *     dict = xvariant_dict_new (orig);
 *
 *     if (xvariant_dict_lookup (dict, "count", "u", &count))
 *       {
 *         xvariant_dict_insert (dict, "count", "u", count + 1);
 *         result = xvariant_dict_end (dict);
 *       }
 *     else
 *       {
 *         g_set_error (...);
 *         result = NULL;
 *       }
 *
 *     xvariant_dict_unref (dict);
 *
 *     return result;
 *   }
 * ]|
 *
 * Since: 2.40
 **/
struct stack_dict
{
  xhashtable_t *values;
  xsize_t magic;
};

G_STATIC_ASSERT (sizeof (struct stack_dict) <= sizeof (xvariant_dict_t));

struct heap_dict
{
  struct stack_dict dict;
  xint_t ref_count;
  xsize_t magic;
};

#define GVSD(d)                 ((struct stack_dict *) (d))
#define GVHD(d)                 ((struct heap_dict *) (d))
#define GVSD_MAGIC              ((xsize_t) 2579507750u)
#define GVSD_MAGIC_PARTIAL      ((xsize_t) 3488698669u)
#define GVHD_MAGIC              ((xsize_t) 2450270775u)
#define is_valid_dict(d)        (GVSD(d)->magic == GVSD_MAGIC)
#define is_valid_heap_dict(d)   (GVHD(d)->magic == GVHD_MAGIC)

/* Just to make sure that by adding a union to xvariant_dict_t, we didn't
 * accidentally change ABI. */
G_STATIC_ASSERT (sizeof (xvariant_dict_t) == sizeof (xsize_t[16]));

static xboolean_t
ensure_valid_dict (xvariant_dict_t *dict)
{
  if (dict == NULL)
    return FALSE;
  else if (is_valid_dict (dict))
    return TRUE;
  if (dict->u.s.partial_magic == GVSD_MAGIC_PARTIAL)
    {
      static xvariant_dict_t cleared_dict;

      /* Make sure that only first two fields were set and the rest is
       * zeroed to avoid messing up the builder that had parent
       * address equal to GVSB_MAGIC_PARTIAL. */
      if (memcmp (cleared_dict.u.s.y, dict->u.s.y, sizeof cleared_dict.u.s.y))
        return FALSE;

      xvariant_dict_init (dict, dict->u.s.asv);
    }
  return is_valid_dict (dict);
}

/* return_if_invalid_dict (d) is like
 * g_return_if_fail (ensure_valid_dict (d)), except that
 * the side effects of ensure_valid_dict are evaluated
 * regardless of whether G_DISABLE_CHECKS is defined or not. */
#define return_if_invalid_dict(d) G_STMT_START {                \
  xboolean_t valid_dict G_GNUC_UNUSED = ensure_valid_dict (d);    \
  g_return_if_fail (valid_dict);                                \
} G_STMT_END

/* return_val_if_invalid_dict (d, val) is like
 * g_return_val_if_fail (ensure_valid_dict (d), val), except that
 * the side effects of ensure_valid_dict are evaluated
 * regardless of whether G_DISABLE_CHECKS is defined or not. */
#define return_val_if_invalid_dict(d, val) G_STMT_START {       \
  xboolean_t valid_dict G_GNUC_UNUSED = ensure_valid_dict (d);    \
  g_return_val_if_fail (valid_dict, val);                       \
} G_STMT_END

/**
 * xvariant_dict_new:
 * @from_asv: (nullable): the #xvariant_t with which to initialise the
 *   dictionary
 *
 * Allocates and initialises a new #xvariant_dict_t.
 *
 * You should call xvariant_dict_unref() on the return value when it
 * is no longer needed.  The memory will not be automatically freed by
 * any other call.
 *
 * In some cases it may be easier to place a #xvariant_dict_t directly on
 * the stack of the calling function and initialise it with
 * xvariant_dict_init().  This is particularly useful when you are
 * using #xvariant_dict_t to construct a #xvariant_t.
 *
 * Returns: (transfer full): a #xvariant_dict_t
 *
 * Since: 2.40
 **/
xvariant_dict_t *
xvariant_dict_new (xvariant_t *from_asv)
{
  xvariant_dict_t *dict;

  dict = g_slice_alloc (sizeof (struct heap_dict));
  xvariant_dict_init (dict, from_asv);
  GVHD(dict)->magic = GVHD_MAGIC;
  GVHD(dict)->ref_count = 1;

  return dict;
}

/**
 * xvariant_dict_init: (skip)
 * @dict: a #xvariant_dict_t
 * @from_asv: (nullable): the initial value for @dict
 *
 * Initialises a #xvariant_dict_t structure.
 *
 * If @from_asv is given, it is used to initialise the dictionary.
 *
 * This function completely ignores the previous contents of @dict.  On
 * one hand this means that it is valid to pass in completely
 * uninitialised memory.  On the other hand, this means that if you are
 * initialising over top of an existing #xvariant_dict_t you need to first
 * call xvariant_dict_clear() in order to avoid leaking memory.
 *
 * You must not call xvariant_dict_ref() or xvariant_dict_unref() on a
 * #xvariant_dict_t that was initialised with this function.  If you ever
 * pass a reference to a #xvariant_dict_t outside of the control of your
 * own code then you should assume that the person receiving that
 * reference may try to use reference counting; you should use
 * xvariant_dict_new() instead of this function.
 *
 * Since: 2.40
 **/
void
xvariant_dict_init (xvariant_dict_t *dict,
                     xvariant_t     *from_asv)
{
  xvariant_iter_t iter;
  xchar_t *key;
  xvariant_t *value;

  GVSD(dict)->values = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);
  GVSD(dict)->magic = GVSD_MAGIC;

  if (from_asv)
    {
      xvariant_iter_init (&iter, from_asv);
      while (xvariant_iter_next (&iter, "{sv}", &key, &value))
        xhash_table_insert (GVSD(dict)->values, key, value);
    }
}

/**
 * xvariant_dict_lookup:
 * @dict: a #xvariant_dict_t
 * @key: the key to look up in the dictionary
 * @format_string: a xvariant_t format string
 * @...: the arguments to unpack the value into
 *
 * Looks up a value in a #xvariant_dict_t.
 *
 * This function is a wrapper around xvariant_dict_lookup_value() and
 * xvariant_get().  In the case that %NULL would have been returned,
 * this function returns %FALSE.  Otherwise, it unpacks the returned
 * value and returns %TRUE.
 *
 * @format_string determines the C types that are used for unpacking the
 * values and also determines if the values are copied or borrowed, see the
 * section on [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * Returns: %TRUE if a value was unpacked
 *
 * Since: 2.40
 **/
xboolean_t
xvariant_dict_lookup (xvariant_dict_t *dict,
                       const xchar_t  *key,
                       const xchar_t  *format_string,
                       ...)
{
  xvariant_t *value;
  va_list ap;

  return_val_if_invalid_dict (dict, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (format_string != NULL, FALSE);

  value = xhash_table_lookup (GVSD(dict)->values, key);

  if (value == NULL || !xvariant_check_format_string (value, format_string, FALSE))
    return FALSE;

  va_start (ap, format_string);
  xvariant_get_va (value, format_string, NULL, &ap);
  va_end (ap);

  return TRUE;
}

/**
 * xvariant_dict_lookup_value:
 * @dict: a #xvariant_dict_t
 * @key: the key to look up in the dictionary
 * @expected_type: (nullable): a #xvariant_type_t, or %NULL
 *
 * Looks up a value in a #xvariant_dict_t.
 *
 * If @key is not found in @dictionary, %NULL is returned.
 *
 * The @expected_type string specifies what type of value is expected.
 * If the value associated with @key has a different type then %NULL is
 * returned.
 *
 * If the key is found and the value has the correct type, it is
 * returned.  If @expected_type was specified then any non-%NULL return
 * value will have this type.
 *
 * Returns: (transfer full): the value of the dictionary key, or %NULL
 *
 * Since: 2.40
 **/
xvariant_t *
xvariant_dict_lookup_value (xvariant_dict_t       *dict,
                             const xchar_t        *key,
                             const xvariant_type_t *expected_type)
{
  xvariant_t *result;

  return_val_if_invalid_dict (dict, NULL);
  g_return_val_if_fail (key != NULL, NULL);

  result = xhash_table_lookup (GVSD(dict)->values, key);

  if (result && (!expected_type || xvariant_is_of_type (result, expected_type)))
    return xvariant_ref (result);

  return NULL;
}

/**
 * xvariant_dict_contains:
 * @dict: a #xvariant_dict_t
 * @key: the key to look up in the dictionary
 *
 * Checks if @key exists in @dict.
 *
 * Returns: %TRUE if @key is in @dict
 *
 * Since: 2.40
 **/
xboolean_t
xvariant_dict_contains (xvariant_dict_t *dict,
                         const xchar_t  *key)
{
  return_val_if_invalid_dict (dict, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  return xhash_table_contains (GVSD(dict)->values, key);
}

/**
 * xvariant_dict_insert:
 * @dict: a #xvariant_dict_t
 * @key: the key to insert a value for
 * @format_string: a #xvariant_t varargs format string
 * @...: arguments, as per @format_string
 *
 * Inserts a value into a #xvariant_dict_t.
 *
 * This call is a convenience wrapper that is exactly equivalent to
 * calling xvariant_new() followed by xvariant_dict_insert_value().
 *
 * Since: 2.40
 **/
void
xvariant_dict_insert (xvariant_dict_t *dict,
                       const xchar_t  *key,
                       const xchar_t  *format_string,
                       ...)
{
  va_list ap;

  return_if_invalid_dict (dict);
  g_return_if_fail (key != NULL);
  g_return_if_fail (format_string != NULL);

  va_start (ap, format_string);
  xvariant_dict_insert_value (dict, key, xvariant_new_va (format_string, NULL, &ap));
  va_end (ap);
}

/**
 * xvariant_dict_insert_value:
 * @dict: a #xvariant_dict_t
 * @key: the key to insert a value for
 * @value: the value to insert
 *
 * Inserts (or replaces) a key in a #xvariant_dict_t.
 *
 * @value is consumed if it is floating.
 *
 * Since: 2.40
 **/
void
xvariant_dict_insert_value (xvariant_dict_t *dict,
                             const xchar_t  *key,
                             xvariant_t     *value)
{
  return_if_invalid_dict (dict);
  g_return_if_fail (key != NULL);
  g_return_if_fail (value != NULL);

  xhash_table_insert (GVSD(dict)->values, xstrdup (key), xvariant_ref_sink (value));
}

/**
 * xvariant_dict_remove:
 * @dict: a #xvariant_dict_t
 * @key: the key to remove
 *
 * Removes a key and its associated value from a #xvariant_dict_t.
 *
 * Returns: %TRUE if the key was found and removed
 *
 * Since: 2.40
 **/
xboolean_t
xvariant_dict_remove (xvariant_dict_t *dict,
                       const xchar_t  *key)
{
  return_val_if_invalid_dict (dict, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  return xhash_table_remove (GVSD(dict)->values, key);
}

/**
 * xvariant_dict_clear:
 * @dict: a #xvariant_dict_t
 *
 * Releases all memory associated with a #xvariant_dict_t without freeing
 * the #xvariant_dict_t structure itself.
 *
 * It typically only makes sense to do this on a stack-allocated
 * #xvariant_dict_t if you want to abort building the value part-way
 * through.  This function need not be called if you call
 * xvariant_dict_end() and it also doesn't need to be called on dicts
 * allocated with xvariant_dict_new (see xvariant_dict_unref() for
 * that).
 *
 * It is valid to call this function on either an initialised
 * #xvariant_dict_t or one that was previously cleared by an earlier call
 * to xvariant_dict_clear() but it is not valid to call this function
 * on uninitialised memory.
 *
 * Since: 2.40
 **/
void
xvariant_dict_clear (xvariant_dict_t *dict)
{
  if (GVSD(dict)->magic == 0)
    /* all-zeros case */
    return;

  return_if_invalid_dict (dict);

  xhash_table_unref (GVSD(dict)->values);
  GVSD(dict)->values = NULL;

  GVSD(dict)->magic = 0;
}

/**
 * xvariant_dict_end:
 * @dict: a #xvariant_dict_t
 *
 * Returns the current value of @dict as a #xvariant_t of type
 * %G_VARIANT_TYPE_VARDICT, clearing it in the process.
 *
 * It is not permissible to use @dict in any way after this call except
 * for reference counting operations (in the case of a heap-allocated
 * #xvariant_dict_t) or by reinitialising it with xvariant_dict_init() (in
 * the case of stack-allocated).
 *
 * Returns: (transfer none): a new, floating, #xvariant_t
 *
 * Since: 2.40
 **/
xvariant_t *
xvariant_dict_end (xvariant_dict_t *dict)
{
  xvariant_builder_t builder;
  xhash_table_iter_t iter;
  xpointer_t key, value;

  return_val_if_invalid_dict (dict, NULL);

  xvariant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);

  xhash_table_iter_init (&iter, GVSD(dict)->values);
  while (xhash_table_iter_next (&iter, &key, &value))
    xvariant_builder_add (&builder, "{sv}", (const xchar_t *) key, (xvariant_t *) value);

  xvariant_dict_clear (dict);

  return xvariant_builder_end (&builder);
}

/**
 * xvariant_dict_ref:
 * @dict: a heap-allocated #xvariant_dict_t
 *
 * Increases the reference count on @dict.
 *
 * Don't call this on stack-allocated #xvariant_dict_t instances or bad
 * things will happen.
 *
 * Returns: (transfer full): a new reference to @dict
 *
 * Since: 2.40
 **/
xvariant_dict_t *
xvariant_dict_ref (xvariant_dict_t *dict)
{
  g_return_val_if_fail (is_valid_heap_dict (dict), NULL);

  GVHD(dict)->ref_count++;

  return dict;
}

/**
 * xvariant_dict_unref:
 * @dict: (transfer full): a heap-allocated #xvariant_dict_t
 *
 * Decreases the reference count on @dict.
 *
 * In the event that there are no more references, releases all memory
 * associated with the #xvariant_dict_t.
 *
 * Don't call this on stack-allocated #xvariant_dict_t instances or bad
 * things will happen.
 *
 * Since: 2.40
 **/
void
xvariant_dict_unref (xvariant_dict_t *dict)
{
  g_return_if_fail (is_valid_heap_dict (dict));

  if (--GVHD(dict)->ref_count == 0)
    {
      xvariant_dict_clear (dict);
      g_slice_free (struct heap_dict, (struct heap_dict *) dict);
    }
}


/* Format strings {{{1 */
/*< private >
 * xvariant_format_string_scan:
 * @string: a string that may be prefixed with a format string
 * @limit: (nullable) (default NULL): a pointer to the end of @string,
 *         or %NULL
 * @endptr: (nullable) (default NULL): location to store the end pointer,
 *          or %NULL
 *
 * Checks the string pointed to by @string for starting with a properly
 * formed #xvariant_t varargs format string.  If no valid format string is
 * found then %FALSE is returned.
 *
 * If @string does start with a valid format string then %TRUE is
 * returned.  If @endptr is non-%NULL then it is updated to point to the
 * first character after the format string.
 *
 * If @limit is non-%NULL then @limit (and any character after it) will
 * not be accessed and the effect is otherwise equivalent to if the
 * character at @limit were nul.
 *
 * See the section on [xvariant_t format strings][gvariant-format-strings].
 *
 * Returns: %TRUE if there was a valid format string
 *
 * Since: 2.24
 */
xboolean_t
xvariant_format_string_scan (const xchar_t  *string,
                              const xchar_t  *limit,
                              const xchar_t **endptr)
{
#define next_char() (string == limit ? '\0' : *(string++))
#define peek_char() (string == limit ? '\0' : *string)
  char c;

  switch (next_char())
    {
    case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
    case 'x': case 't': case 'h': case 'd': case 's': case 'o':
    case 'g': case 'v': case '*': case '?': case 'r':
      break;

    case 'm':
      return xvariant_format_string_scan (string, limit, endptr);

    case 'a':
    case '@':
      return xvariant_type_string_scan (string, limit, endptr);

    case '(':
      while (peek_char() != ')')
        if (!xvariant_format_string_scan (string, limit, &string))
          return FALSE;

      next_char(); /* consume ')' */
      break;

    case '{':
      c = next_char();

      if (c == '&')
        {
          c = next_char ();

          if (c != 's' && c != 'o' && c != 'g')
            return FALSE;
        }
      else
        {
          if (c == '@')
            c = next_char ();

          /* ISO/IEC 9899:1999 (C99) §7.21.5.2:
           *    The terminating null character is considered to be
           *    part of the string.
           */
          if (c != '\0' && strchr ("bynqiuxthdsog?", c) == NULL)
            return FALSE;
        }

      if (!xvariant_format_string_scan (string, limit, &string))
        return FALSE;

      if (next_char() != '}')
        return FALSE;

      break;

    case '^':
      if ((c = next_char()) == 'a')
        {
          if ((c = next_char()) == '&')
            {
              if ((c = next_char()) == 'a')
                {
                  if ((c = next_char()) == 'y')
                    break;      /* '^a&ay' */
                }

              else if (c == 's' || c == 'o')
                break;          /* '^a&s', '^a&o' */
            }

          else if (c == 'a')
            {
              if ((c = next_char()) == 'y')
                break;          /* '^aay' */
            }

          else if (c == 's' || c == 'o')
            break;              /* '^as', '^ao' */

          else if (c == 'y')
            break;              /* '^ay' */
        }
      else if (c == '&')
        {
          if ((c = next_char()) == 'a')
            {
              if ((c = next_char()) == 'y')
                break;          /* '^&ay' */
            }
        }

      return FALSE;

    case '&':
      c = next_char();

      if (c != 's' && c != 'o' && c != 'g')
        return FALSE;

      break;

    default:
      return FALSE;
    }

  if (endptr != NULL)
    *endptr = string;

#undef next_char
#undef peek_char

  return TRUE;
}

/**
 * xvariant_check_format_string:
 * @value: a #xvariant_t
 * @format_string: a valid #xvariant_t format string
 * @copy_only: %TRUE to ensure the format string makes deep copies
 *
 * Checks if calling xvariant_get() with @format_string on @value would
 * be valid from a type-compatibility standpoint.  @format_string is
 * assumed to be a valid format string (from a syntactic standpoint).
 *
 * If @copy_only is %TRUE then this function additionally checks that it
 * would be safe to call xvariant_unref() on @value immediately after
 * the call to xvariant_get() without invalidating the result.  This is
 * only possible if deep copies are made (ie: there are no pointers to
 * the data inside of the soon-to-be-freed #xvariant_t instance).  If this
 * check fails then a g_critical() is printed and %FALSE is returned.
 *
 * This function is meant to be used by functions that wish to provide
 * varargs accessors to #xvariant_t values of uncertain values (eg:
 * xvariant_lookup() or xmenu_model_get_item_attribute()).
 *
 * Returns: %TRUE if @format_string is safe to use
 *
 * Since: 2.34
 */
xboolean_t
xvariant_check_format_string (xvariant_t    *value,
                               const xchar_t *format_string,
                               xboolean_t     copy_only)
{
  const xchar_t *original_format = format_string;
  const xchar_t *type_string;

  /* Interesting factoid: assuming a format string is valid, it can be
   * converted to a type string by removing all '@' '&' and '^'
   * characters.
   *
   * Instead of doing that, we can just skip those characters when
   * comparing it to the type string of @value.
   *
   * For the copy-only case we can just drop the '&' from the list of
   * characters to skip over.  A '&' will never appear in a type string
   * so we know that it won't be possible to return %TRUE if it is in a
   * format string.
   */
  type_string = xvariant_get_type_string (value);

  while (*type_string || *format_string)
    {
      xchar_t format = *format_string++;

      switch (format)
        {
        case '&':
          if G_UNLIKELY (copy_only)
            {
              /* for the love of all that is good, please don't mark this string for translation... */
              g_critical ("xvariant_check_format_string() is being called by a function with a xvariant_t varargs "
                          "interface to validate the passed format string for type safety.  The passed format "
                          "(%s) contains a '&' character which would result in a pointer being returned to the "
                          "data inside of a xvariant_t instance that may no longer exist by the time the function "
                          "returns.  Modify your code to use a format string without '&'.", original_format);
              return FALSE;
            }

          G_GNUC_FALLTHROUGH;
        case '^':
        case '@':
          /* ignore these 2 (or 3) */
          continue;

        case '?':
          /* attempt to consume one of 'bynqiuxthdsog' */
          {
            char s = *type_string++;

            if (s == '\0' || strchr ("bynqiuxthdsog", s) == NULL)
              return FALSE;
          }
          continue;

        case 'r':
          /* ensure it's a tuple */
          if (*type_string != '(')
            return FALSE;

          G_GNUC_FALLTHROUGH;
        case '*':
          /* consume a full type string for the '*' or 'r' */
          if (!xvariant_type_string_scan (type_string, NULL, &type_string))
            return FALSE;

          continue;

        default:
          /* attempt to consume exactly one character equal to the format */
          if (format != *type_string++)
            return FALSE;
        }
    }

  return TRUE;
}

/*< private >
 * xvariant_format_string_scan_type:
 * @string: a string that may be prefixed with a format string
 * @limit: (nullable) (default NULL): a pointer to the end of @string,
 *         or %NULL
 * @endptr: (nullable) (default NULL): location to store the end pointer,
 *          or %NULL
 *
 * If @string starts with a valid format string then this function will
 * return the type that the format string corresponds to.  Otherwise
 * this function returns %NULL.
 *
 * Use xvariant_type_free() to free the return value when you no longer
 * need it.
 *
 * This function is otherwise exactly like
 * xvariant_format_string_scan().
 *
 * Returns: (nullable): a #xvariant_type_t if there was a valid format string
 *
 * Since: 2.24
 */
xvariant_type_t *
xvariant_format_string_scan_type (const xchar_t  *string,
                                   const xchar_t  *limit,
                                   const xchar_t **endptr)
{
  const xchar_t *my_end;
  xchar_t *dest;
  xchar_t *new;

  if (endptr == NULL)
    endptr = &my_end;

  if (!xvariant_format_string_scan (string, limit, endptr))
    return NULL;

  dest = new = g_malloc (*endptr - string + 1);
  while (string != *endptr)
    {
      if (*string != '@' && *string != '&' && *string != '^')
        *dest++ = *string;
      string++;
    }
  *dest = '\0';

  return (xvariant_type_t *) G_VARIANT_TYPE (new);
}

static xboolean_t
valid_format_string (const xchar_t *format_string,
                     xboolean_t     single,
                     xvariant_t    *value)
{
  const xchar_t *endptr;
  xvariant_type_t *type;

  type = xvariant_format_string_scan_type (format_string, NULL, &endptr);

  if G_UNLIKELY (type == NULL || (single && *endptr != '\0'))
    {
      if (single)
        g_critical ("'%s' is not a valid xvariant_t format string",
                    format_string);
      else
        g_critical ("'%s' does not have a valid xvariant_t format "
                    "string as a prefix", format_string);

      if (type != NULL)
        xvariant_type_free (type);

      return FALSE;
    }

  if G_UNLIKELY (value && !xvariant_is_of_type (value, type))
    {
      xchar_t *fragment;
      xchar_t *typestr;

      fragment = xstrndup (format_string, endptr - format_string);
      typestr = xvariant_type_dup_string (type);

      g_critical ("the xvariant_t format string '%s' has a type of "
                  "'%s' but the given value has a type of '%s'",
                  fragment, typestr, xvariant_get_type_string (value));

      xvariant_type_free (type);
      g_free (fragment);
      g_free (typestr);

      return FALSE;
    }

  xvariant_type_free (type);

  return TRUE;
}

/* Variable Arguments {{{1 */
/* We consider 2 main classes of format strings:
 *
 *   - recursive format strings
 *      these are ones that result in recursion and the collection of
 *      possibly more than one argument.  Maybe types, tuples,
 *      dictionary entries.
 *
 *   - leaf format string
 *      these result in the collection of a single argument.
 *
 * Leaf format strings are further subdivided into two categories:
 *
 *   - single non-null pointer ("nnp")
 *      these either collect or return a single non-null pointer.
 *
 *   - other
 *      these collect or return something else (bool, number, etc).
 *
 * Based on the above, the varargs handling code is split into 4 main parts:
 *
 *   - nnp handling code
 *   - leaf handling code (which may invoke nnp code)
 *   - generic handling code (may be recursive, may invoke leaf code)
 *   - user-facing API (which invokes the generic code)
 *
 * Each section implements some of the following functions:
 *
 *   - skip:
 *      collect the arguments for the format string as if
 *      xvariant_new() had been called, but do nothing with them.  used
 *      for skipping over arguments when constructing a Nothing maybe
 *      type.
 *
 *   - new:
 *      create a xvariant_t *
 *
 *   - get:
 *      unpack a xvariant_t *
 *
 *   - free (nnp only):
 *      free a previously allocated item
 */

static xboolean_t
xvariant_format_string_is_leaf (const xchar_t *str)
{
  return str[0] != 'm' && str[0] != '(' && str[0] != '{';
}

static xboolean_t
xvariant_format_string_is_nnp (const xchar_t *str)
{
  return str[0] == 'a' || str[0] == 's' || str[0] == 'o' || str[0] == 'g' ||
         str[0] == '^' || str[0] == '@' || str[0] == '*' || str[0] == '?' ||
         str[0] == 'r' || str[0] == 'v' || str[0] == '&';
}

/* Single non-null pointer ("nnp") {{{2 */
static void
xvariant_valist_free_nnp (const xchar_t *str,
                           xpointer_t     ptr)
{
  switch (*str)
    {
    case 'a':
      xvariant_iter_free (ptr);
      break;

    case '^':
      if (xstr_has_suffix (str, "y"))
        {
          if (str[2] != 'a') /* '^a&ay', '^ay' */
            g_free (ptr);
          else if (str[1] == 'a') /* '^aay' */
            xstrfreev (ptr);
          break; /* '^&ay' */
        }
      else if (str[2] != '&') /* '^as', '^ao' */
        xstrfreev (ptr);
      else                      /* '^a&s', '^a&o' */
        g_free (ptr);
      break;

    case 's':
    case 'o':
    case 'g':
      g_free (ptr);
      break;

    case '@':
    case '*':
    case '?':
    case 'v':
      xvariant_unref (ptr);
      break;

    case '&':
      break;

    default:
      g_assert_not_reached ();
    }
}

static xchar_t
xvariant_scan_convenience (const xchar_t **str,
                            xboolean_t     *constant,
                            xuint_t        *arrays)
{
  *constant = FALSE;
  *arrays = 0;

  for (;;)
    {
      char c = *(*str)++;

      if (c == '&')
        *constant = TRUE;

      else if (c == 'a')
        (*arrays)++;

      else
        return c;
    }
}

static xvariant_t *
xvariant_valist_new_nnp (const xchar_t **str,
                          xpointer_t      ptr)
{
  if (**str == '&')
    (*str)++;

  switch (*(*str)++)
    {
    case 'a':
      if (ptr != NULL)
        {
          const xvariant_type_t *type;
          xvariant_t *value;

          value = xvariant_builder_end (ptr);
          type = xvariant_get_type (value);

          if G_UNLIKELY (!xvariant_type_is_array (type))
            xerror ("xvariant_new: expected array xvariant_builder_t but "
                     "the built value has type '%s'",
                     xvariant_get_type_string (value));

          type = xvariant_type_element (type);

          if G_UNLIKELY (!xvariant_type_is_subtype_of (type, (xvariant_type_t *) *str))
            {
              xchar_t *type_string = xvariant_type_dup_string ((xvariant_type_t *) *str);
              xerror ("xvariant_new: expected xvariant_builder_t array element "
                       "type '%s' but the built value has element type '%s'",
                       type_string, xvariant_get_type_string (value) + 1);
              g_free (type_string);
            }

          xvariant_type_string_scan (*str, NULL, str);

          return value;
        }
      else

        /* special case: NULL pointer for empty array */
        {
          const xvariant_type_t *type = (xvariant_type_t *) *str;

          xvariant_type_string_scan (*str, NULL, str);

          if G_UNLIKELY (!xvariant_type_is_definite (type))
            xerror ("xvariant_new: NULL pointer given with indefinite "
                     "array type; unable to determine which type of empty "
                     "array to construct.");

          return xvariant_new_array (type, NULL, 0);
        }

    case 's':
      {
        xvariant_t *value;

        value = xvariant_new_string (ptr);

        if (value == NULL)
          value = xvariant_new_string ("[Invalid UTF-8]");

        return value;
      }

    case 'o':
      return xvariant_new_object_path (ptr);

    case 'g':
      return xvariant_new_signature (ptr);

    case '^':
      {
        xboolean_t constant;
        xuint_t arrays;
        xchar_t type;

        type = xvariant_scan_convenience (str, &constant, &arrays);

        if (type == 's')
          return xvariant_new_strv (ptr, -1);

        if (type == 'o')
          return xvariant_new_objv (ptr, -1);

        if (arrays > 1)
          return xvariant_new_bytestring_array (ptr, -1);

        return xvariant_new_bytestring (ptr);
      }

    case '@':
      if G_UNLIKELY (!xvariant_is_of_type (ptr, (xvariant_type_t *) *str))
        {
          xchar_t *type_string = xvariant_type_dup_string ((xvariant_type_t *) *str);
          xerror ("xvariant_new: expected xvariant_t of type '%s' but "
                   "received value has type '%s'",
                   type_string, xvariant_get_type_string (ptr));
          g_free (type_string);
        }

      xvariant_type_string_scan (*str, NULL, str);

      return ptr;

    case '*':
      return ptr;

    case '?':
      if G_UNLIKELY (!xvariant_type_is_basic (xvariant_get_type (ptr)))
        xerror ("xvariant_new: format string '?' expects basic-typed "
                 "xvariant_t, but received value has type '%s'",
                 xvariant_get_type_string (ptr));

      return ptr;

    case 'r':
      if G_UNLIKELY (!xvariant_type_is_tuple (xvariant_get_type (ptr)))
        xerror ("xvariant_new: format string 'r' expects tuple-typed "
                 "xvariant_t, but received value has type '%s'",
                 xvariant_get_type_string (ptr));

      return ptr;

    case 'v':
      return xvariant_new_variant (ptr);

    default:
      g_assert_not_reached ();
    }
}

static xpointer_t
xvariant_valist_get_nnp (const xchar_t **str,
                          xvariant_t     *value)
{
  switch (*(*str)++)
    {
    case 'a':
      xvariant_type_string_scan (*str, NULL, str);
      return xvariant_iter_new (value);

    case '&':
      (*str)++;
      return (xchar_t *) xvariant_get_string (value, NULL);

    case 's':
    case 'o':
    case 'g':
      return xvariant_dup_string (value, NULL);

    case '^':
      {
        xboolean_t constant;
        xuint_t arrays;
        xchar_t type;

        type = xvariant_scan_convenience (str, &constant, &arrays);

        if (type == 's')
          {
            if (constant)
              return xvariant_get_strv (value, NULL);
            else
              return xvariant_dup_strv (value, NULL);
          }

        else if (type == 'o')
          {
            if (constant)
              return xvariant_get_objv (value, NULL);
            else
              return xvariant_dup_objv (value, NULL);
          }

        else if (arrays > 1)
          {
            if (constant)
              return xvariant_get_bytestring_array (value, NULL);
            else
              return xvariant_dup_bytestring_array (value, NULL);
          }

        else
          {
            if (constant)
              return (xchar_t *) xvariant_get_bytestring (value);
            else
              return xvariant_dup_bytestring (value, NULL);
          }
      }

    case '@':
      xvariant_type_string_scan (*str, NULL, str);
      G_GNUC_FALLTHROUGH;

    case '*':
    case '?':
    case 'r':
      return xvariant_ref (value);

    case 'v':
      return xvariant_get_variant (value);

    default:
      g_assert_not_reached ();
    }
}

/* Leaves {{{2 */
static void
xvariant_valist_skip_leaf (const xchar_t **str,
                            va_list      *app)
{
  if (xvariant_format_string_is_nnp (*str))
    {
      xvariant_format_string_scan (*str, NULL, str);
      va_arg (*app, xpointer_t);
      return;
    }

  switch (*(*str)++)
    {
    case 'b':
    case 'y':
    case 'n':
    case 'q':
    case 'i':
    case 'u':
    case 'h':
      va_arg (*app, int);
      return;

    case 'x':
    case 't':
      va_arg (*app, xuint64_t);
      return;

    case 'd':
      va_arg (*app, xdouble_t);
      return;

    default:
      g_assert_not_reached ();
    }
}

static xvariant_t *
xvariant_valist_new_leaf (const xchar_t **str,
                           va_list      *app)
{
  if (xvariant_format_string_is_nnp (*str))
    return xvariant_valist_new_nnp (str, va_arg (*app, xpointer_t));

  switch (*(*str)++)
    {
    case 'b':
      return xvariant_new_boolean (va_arg (*app, xboolean_t));

    case 'y':
      return xvariant_new_byte (va_arg (*app, xuint_t));

    case 'n':
      return xvariant_new_int16 (va_arg (*app, xint_t));

    case 'q':
      return xvariant_new_uint16 (va_arg (*app, xuint_t));

    case 'i':
      return xvariant_new_int32 (va_arg (*app, xint_t));

    case 'u':
      return xvariant_new_uint32 (va_arg (*app, xuint_t));

    case 'x':
      return xvariant_new_int64 (va_arg (*app, sint64_t));

    case 't':
      return xvariant_new_uint64 (va_arg (*app, xuint64_t));

    case 'h':
      return xvariant_new_handle (va_arg (*app, xint_t));

    case 'd':
      return xvariant_new_double (va_arg (*app, xdouble_t));

    default:
      g_assert_not_reached ();
    }
}

/* The code below assumes this */
G_STATIC_ASSERT (sizeof (xboolean_t) == sizeof (xuint32_t));
G_STATIC_ASSERT (sizeof (xdouble_t) == sizeof (xuint64_t));

static void
xvariant_valist_get_leaf (const xchar_t **str,
                           xvariant_t     *value,
                           xboolean_t      free,
                           va_list      *app)
{
  xpointer_t ptr = va_arg (*app, xpointer_t);

  if (ptr == NULL)
    {
      xvariant_format_string_scan (*str, NULL, str);
      return;
    }

  if (xvariant_format_string_is_nnp (*str))
    {
      xpointer_t *nnp = (xpointer_t *) ptr;

      if (free && *nnp != NULL)
        xvariant_valist_free_nnp (*str, *nnp);

      *nnp = NULL;

      if (value != NULL)
        *nnp = xvariant_valist_get_nnp (str, value);
      else
        xvariant_format_string_scan (*str, NULL, str);

      return;
    }

  if (value != NULL)
    {
      switch (*(*str)++)
        {
        case 'b':
          *(xboolean_t *) ptr = xvariant_get_boolean (value);
          return;

        case 'y':
          *(xuint8_t *) ptr = xvariant_get_byte (value);
          return;

        case 'n':
          *(gint16 *) ptr = xvariant_get_int16 (value);
          return;

        case 'q':
          *(xuint16_t *) ptr = xvariant_get_uint16 (value);
          return;

        case 'i':
          *(gint32 *) ptr = xvariant_get_int32 (value);
          return;

        case 'u':
          *(xuint32_t *) ptr = xvariant_get_uint32 (value);
          return;

        case 'x':
          *(sint64_t *) ptr = xvariant_get_int64 (value);
          return;

        case 't':
          *(xuint64_t *) ptr = xvariant_get_uint64 (value);
          return;

        case 'h':
          *(gint32 *) ptr = xvariant_get_handle (value);
          return;

        case 'd':
          *(xdouble_t *) ptr = xvariant_get_double (value);
          return;
        }
    }
  else
    {
      switch (*(*str)++)
        {
        case 'y':
          *(xuint8_t *) ptr = 0;
          return;

        case 'n':
        case 'q':
          *(xuint16_t *) ptr = 0;
          return;

        case 'i':
        case 'u':
        case 'h':
        case 'b':
          *(xuint32_t *) ptr = 0;
          return;

        case 'x':
        case 't':
        case 'd':
          *(xuint64_t *) ptr = 0;
          return;
        }
    }

  g_assert_not_reached ();
}

/* Generic (recursive) {{{2 */
static void
xvariant_valist_skip (const xchar_t **str,
                       va_list      *app)
{
  if (xvariant_format_string_is_leaf (*str))
    xvariant_valist_skip_leaf (str, app);

  else if (**str == 'm') /* maybe */
    {
      (*str)++;

      if (!xvariant_format_string_is_nnp (*str))
        va_arg (*app, xboolean_t);

      xvariant_valist_skip (str, app);
    }
  else /* tuple, dictionary entry */
    {
      g_assert (**str == '(' || **str == '{');
      (*str)++;
      while (**str != ')' && **str != '}')
        xvariant_valist_skip (str, app);
      (*str)++;
    }
}

static xvariant_t *
xvariant_valist_new (const xchar_t **str,
                      va_list      *app)
{
  if (xvariant_format_string_is_leaf (*str))
    return xvariant_valist_new_leaf (str, app);

  if (**str == 'm') /* maybe */
    {
      xvariant_type_t *type = NULL;
      xvariant_t *value = NULL;

      (*str)++;

      if (xvariant_format_string_is_nnp (*str))
        {
          xpointer_t nnp = va_arg (*app, xpointer_t);

          if (nnp != NULL)
            value = xvariant_valist_new_nnp (str, nnp);
          else
            type = xvariant_format_string_scan_type (*str, NULL, str);
        }
      else
        {
          xboolean_t just = va_arg (*app, xboolean_t);

          if (just)
            value = xvariant_valist_new (str, app);
          else
            {
              type = xvariant_format_string_scan_type (*str, NULL, NULL);
              xvariant_valist_skip (str, app);
            }
        }

      value = xvariant_new_maybe (type, value);

      if (type != NULL)
        xvariant_type_free (type);

      return value;
    }
  else /* tuple, dictionary entry */
    {
      xvariant_builder_t b;

      if (**str == '(')
        xvariant_builder_init (&b, G_VARIANT_TYPE_TUPLE);
      else
        {
          g_assert (**str == '{');
          xvariant_builder_init (&b, G_VARIANT_TYPE_DICT_ENTRY);
        }

      (*str)++; /* '(' */
      while (**str != ')' && **str != '}')
        xvariant_builder_add_value (&b, xvariant_valist_new (str, app));
      (*str)++; /* ')' */

      return xvariant_builder_end (&b);
    }
}

static void
xvariant_valist_get (const xchar_t **str,
                      xvariant_t     *value,
                      xboolean_t      free,
                      va_list      *app)
{
  if (xvariant_format_string_is_leaf (*str))
    xvariant_valist_get_leaf (str, value, free, app);

  else if (**str == 'm')
    {
      (*str)++;

      if (value != NULL)
        value = xvariant_get_maybe (value);

      if (!xvariant_format_string_is_nnp (*str))
        {
          xboolean_t *ptr = va_arg (*app, xboolean_t *);

          if (ptr != NULL)
            *ptr = value != NULL;
        }

      xvariant_valist_get (str, value, free, app);

      if (value != NULL)
        xvariant_unref (value);
    }

  else /* tuple, dictionary entry */
    {
      xint_t index = 0;

      g_assert (**str == '(' || **str == '{');

      (*str)++;
      while (**str != ')' && **str != '}')
        {
          if (value != NULL)
            {
              xvariant_t *child = xvariant_get_child_value (value, index++);
              xvariant_valist_get (str, child, free, app);
              xvariant_unref (child);
            }
          else
            xvariant_valist_get (str, NULL, free, app);
        }
      (*str)++;
    }
}

/* User-facing API {{{2 */
/**
 * xvariant_new: (skip)
 * @format_string: a #xvariant_t format string
 * @...: arguments, as per @format_string
 *
 * Creates a new #xvariant_t instance.
 *
 * Think of this function as an analogue to xstrdup_printf().
 *
 * The type of the created instance and the arguments that are expected
 * by this function are determined by @format_string. See the section on
 * [xvariant_t format strings][gvariant-format-strings]. Please note that
 * the syntax of the format string is very likely to be extended in the
 * future.
 *
 * The first character of the format string must not be '*' '?' '@' or
 * 'r'; in essence, a new #xvariant_t must always be constructed by this
 * function (and not merely passed through it unmodified).
 *
 * Note that the arguments must be of the correct width for their types
 * specified in @format_string. This can be achieved by casting them. See
 * the [xvariant_t varargs documentation][gvariant-varargs].
 *
 * |[<!-- language="C" -->
 * my_flags_t some_flags = FLAG_ONE | FLAG_TWO;
 * const xchar_t *some_strings[] = { "a", "b", "c", NULL };
 * xvariant_t *new_variant;
 *
 * new_variant = xvariant_new ("(t^as)",
 *                              // This cast is required.
 *                              (xuint64_t) some_flags,
 *                              some_strings);
 * ]|
 *
 * Returns: a new floating #xvariant_t instance
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new (const xchar_t *format_string,
               ...)
{
  xvariant_t *value;
  va_list ap;

  g_return_val_if_fail (valid_format_string (format_string, TRUE, NULL) &&
                        format_string[0] != '?' && format_string[0] != '@' &&
                        format_string[0] != '*' && format_string[0] != 'r',
                        NULL);

  va_start (ap, format_string);
  value = xvariant_new_va (format_string, NULL, &ap);
  va_end (ap);

  return value;
}

/**
 * xvariant_new_va: (skip)
 * @format_string: a string that is prefixed with a format string
 * @endptr: (nullable) (default NULL): location to store the end pointer,
 *          or %NULL
 * @app: a pointer to a #va_list
 *
 * This function is intended to be used by libraries based on
 * #xvariant_t that want to provide xvariant_new()-like functionality
 * to their users.
 *
 * The API is more general than xvariant_new() to allow a wider range
 * of possible uses.
 *
 * @format_string must still point to a valid format string, but it only
 * needs to be nul-terminated if @endptr is %NULL.  If @endptr is
 * non-%NULL then it is updated to point to the first character past the
 * end of the format string.
 *
 * @app is a pointer to a #va_list.  The arguments, according to
 * @format_string, are collected from this #va_list and the list is left
 * pointing to the argument following the last.
 *
 * Note that the arguments in @app must be of the correct width for their
 * types specified in @format_string when collected into the #va_list.
 * See the [xvariant_t varargs documentation][gvariant-varargs].
 *
 * These two generalisations allow mixing of multiple calls to
 * xvariant_new_va() and xvariant_get_va() within a single actual
 * varargs call by the user.
 *
 * The return value will be floating if it was a newly created xvariant_t
 * instance (for example, if the format string was "(ii)").  In the case
 * that the format_string was '*', '?', 'r', or a format starting with
 * '@' then the collected #xvariant_t pointer will be returned unmodified,
 * without adding any additional references.
 *
 * In order to behave correctly in all cases it is necessary for the
 * calling function to xvariant_ref_sink() the return result before
 * returning control to the user that originally provided the pointer.
 * At this point, the caller will have their own full reference to the
 * result.  This can also be done by adding the result to a container,
 * or by passing it to another xvariant_new() call.
 *
 * Returns: a new, usually floating, #xvariant_t
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_va (const xchar_t  *format_string,
                  const xchar_t **endptr,
                  va_list      *app)
{
  xvariant_t *value;

  g_return_val_if_fail (valid_format_string (format_string, !endptr, NULL),
                        NULL);
  g_return_val_if_fail (app != NULL, NULL);

  value = xvariant_valist_new (&format_string, app);

  if (endptr != NULL)
    *endptr = format_string;

  return value;
}

/**
 * xvariant_get: (skip)
 * @value: a #xvariant_t instance
 * @format_string: a #xvariant_t format string
 * @...: arguments, as per @format_string
 *
 * Deconstructs a #xvariant_t instance.
 *
 * Think of this function as an analogue to scanf().
 *
 * The arguments that are expected by this function are entirely
 * determined by @format_string.  @format_string also restricts the
 * permissible types of @value.  It is an error to give a value with
 * an incompatible type.  See the section on
 * [xvariant_t format strings][gvariant-format-strings].
 * Please note that the syntax of the format string is very likely to be
 * extended in the future.
 *
 * @format_string determines the C types that are used for unpacking
 * the values and also determines if the values are copied or borrowed,
 * see the section on
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * Since: 2.24
 **/
void
xvariant_get (xvariant_t    *value,
               const xchar_t *format_string,
               ...)
{
  va_list ap;

  g_return_if_fail (value != NULL);
  g_return_if_fail (valid_format_string (format_string, TRUE, value));

  /* if any direct-pointer-access formats are in use, flatten first */
  if (strchr (format_string, '&'))
    xvariant_get_data (value);

  va_start (ap, format_string);
  xvariant_get_va (value, format_string, NULL, &ap);
  va_end (ap);
}

/**
 * xvariant_get_va: (skip)
 * @value: a #xvariant_t
 * @format_string: a string that is prefixed with a format string
 * @endptr: (nullable) (default NULL): location to store the end pointer,
 *          or %NULL
 * @app: a pointer to a #va_list
 *
 * This function is intended to be used by libraries based on #xvariant_t
 * that want to provide xvariant_get()-like functionality to their
 * users.
 *
 * The API is more general than xvariant_get() to allow a wider range
 * of possible uses.
 *
 * @format_string must still point to a valid format string, but it only
 * need to be nul-terminated if @endptr is %NULL.  If @endptr is
 * non-%NULL then it is updated to point to the first character past the
 * end of the format string.
 *
 * @app is a pointer to a #va_list.  The arguments, according to
 * @format_string, are collected from this #va_list and the list is left
 * pointing to the argument following the last.
 *
 * These two generalisations allow mixing of multiple calls to
 * xvariant_new_va() and xvariant_get_va() within a single actual
 * varargs call by the user.
 *
 * @format_string determines the C types that are used for unpacking
 * the values and also determines if the values are copied or borrowed,
 * see the section on
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * Since: 2.24
 **/
void
xvariant_get_va (xvariant_t     *value,
                  const xchar_t  *format_string,
                  const xchar_t **endptr,
                  va_list      *app)
{
  g_return_if_fail (valid_format_string (format_string, !endptr, value));
  g_return_if_fail (value != NULL);
  g_return_if_fail (app != NULL);

  /* if any direct-pointer-access formats are in use, flatten first */
  if (strchr (format_string, '&'))
    xvariant_get_data (value);

  xvariant_valist_get (&format_string, value, FALSE, app);

  if (endptr != NULL)
    *endptr = format_string;
}

/* Varargs-enabled Utility Functions {{{1 */

/**
 * xvariant_builder_add: (skip)
 * @builder: a #xvariant_builder_t
 * @format_string: a #xvariant_t varargs format string
 * @...: arguments, as per @format_string
 *
 * Adds to a #xvariant_builder_t.
 *
 * This call is a convenience wrapper that is exactly equivalent to
 * calling xvariant_new() followed by xvariant_builder_add_value().
 *
 * Note that the arguments must be of the correct width for their types
 * specified in @format_string. This can be achieved by casting them. See
 * the [xvariant_t varargs documentation][gvariant-varargs].
 *
 * This function might be used as follows:
 *
 * |[<!-- language="C" -->
 * xvariant_t *
 * make_pointless_dictionary (void)
 * {
 *   xvariant_builder_t builder;
 *   int i;
 *
 *   xvariant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
 *   for (i = 0; i < 16; i++)
 *     {
 *       xchar_t buf[3];
 *
 *       sprintf (buf, "%d", i);
 *       xvariant_builder_add (&builder, "{is}", i, buf);
 *     }
 *
 *   return xvariant_builder_end (&builder);
 * }
 * ]|
 *
 * Since: 2.24
 */
void
xvariant_builder_add (xvariant_builder_t *builder,
                       const xchar_t     *format_string,
                       ...)
{
  xvariant_t *variant;
  va_list ap;

  va_start (ap, format_string);
  variant = xvariant_new_va (format_string, NULL, &ap);
  va_end (ap);

  xvariant_builder_add_value (builder, variant);
}

/**
 * xvariant_get_child: (skip)
 * @value: a container #xvariant_t
 * @index_: the index of the child to deconstruct
 * @format_string: a #xvariant_t format string
 * @...: arguments, as per @format_string
 *
 * Reads a child item out of a container #xvariant_t instance and
 * deconstructs it according to @format_string.  This call is
 * essentially a combination of xvariant_get_child_value() and
 * xvariant_get().
 *
 * @format_string determines the C types that are used for unpacking
 * the values and also determines if the values are copied or borrowed,
 * see the section on
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * Since: 2.24
 **/
void
xvariant_get_child (xvariant_t    *value,
                     xsize_t        index_,
                     const xchar_t *format_string,
                     ...)
{
  xvariant_t *child;
  va_list ap;

  /* if any direct-pointer-access formats are in use, flatten first */
  if (strchr (format_string, '&'))
    xvariant_get_data (value);

  child = xvariant_get_child_value (value, index_);
  g_return_if_fail (valid_format_string (format_string, TRUE, child));

  va_start (ap, format_string);
  xvariant_get_va (child, format_string, NULL, &ap);
  va_end (ap);

  xvariant_unref (child);
}

/**
 * xvariant_iter_next: (skip)
 * @iter: a #xvariant_iter_t
 * @format_string: a xvariant_t format string
 * @...: the arguments to unpack the value into
 *
 * Gets the next item in the container and unpacks it into the variable
 * argument list according to @format_string, returning %TRUE.
 *
 * If no more items remain then %FALSE is returned.
 *
 * All of the pointers given on the variable arguments list of this
 * function are assumed to point at uninitialised memory.  It is the
 * responsibility of the caller to free all of the values returned by
 * the unpacking process.
 *
 * Here is an example for memory management with xvariant_iter_next():
 * |[<!-- language="C" -->
 *   // Iterates a dictionary of type 'a{sv}'
 *   void
 *   iterate_dictionary (xvariant_t *dictionary)
 *   {
 *     xvariant_iter_t iter;
 *     xvariant_t *value;
 *     xchar_t *key;
 *
 *     xvariant_iter_init (&iter, dictionary);
 *     while (xvariant_iter_next (&iter, "{sv}", &key, &value))
 *       {
 *         g_print ("Item '%s' has type '%s'\n", key,
 *                  xvariant_get_type_string (value));
 *
 *         // must free data for ourselves
 *         xvariant_unref (value);
 *         g_free (key);
 *       }
 *   }
 * ]|
 *
 * For a solution that is likely to be more convenient to C programmers
 * when dealing with loops, see xvariant_iter_loop().
 *
 * @format_string determines the C types that are used for unpacking
 * the values and also determines if the values are copied or borrowed.
 *
 * See the section on
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * Returns: %TRUE if a value was unpacked, or %FALSE if there as no value
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_iter_next (xvariant_iter_t *iter,
                     const xchar_t  *format_string,
                     ...)
{
  xvariant_t *value;

  value = xvariant_iter_next_value (iter);

  g_return_val_if_fail (valid_format_string (format_string, TRUE, value),
                        FALSE);

  if (value != NULL)
    {
      va_list ap;

      va_start (ap, format_string);
      xvariant_valist_get (&format_string, value, FALSE, &ap);
      va_end (ap);

      xvariant_unref (value);
    }

  return value != NULL;
}

/**
 * xvariant_iter_loop: (skip)
 * @iter: a #xvariant_iter_t
 * @format_string: a xvariant_t format string
 * @...: the arguments to unpack the value into
 *
 * Gets the next item in the container and unpacks it into the variable
 * argument list according to @format_string, returning %TRUE.
 *
 * If no more items remain then %FALSE is returned.
 *
 * On the first call to this function, the pointers appearing on the
 * variable argument list are assumed to point at uninitialised memory.
 * On the second and later calls, it is assumed that the same pointers
 * will be given and that they will point to the memory as set by the
 * previous call to this function.  This allows the previous values to
 * be freed, as appropriate.
 *
 * This function is intended to be used with a while loop as
 * demonstrated in the following example.  This function can only be
 * used when iterating over an array.  It is only valid to call this
 * function with a string constant for the format string and the same
 * string constant must be used each time.  Mixing calls to this
 * function and xvariant_iter_next() or xvariant_iter_next_value() on
 * the same iterator causes undefined behavior.
 *
 * If you break out of a such a while loop using xvariant_iter_loop() then
 * you must free or unreference all the unpacked values as you would with
 * xvariant_get(). Failure to do so will cause a memory leak.
 *
 * Here is an example for memory management with xvariant_iter_loop():
 * |[<!-- language="C" -->
 *   // Iterates a dictionary of type 'a{sv}'
 *   void
 *   iterate_dictionary (xvariant_t *dictionary)
 *   {
 *     xvariant_iter_t iter;
 *     xvariant_t *value;
 *     xchar_t *key;
 *
 *     xvariant_iter_init (&iter, dictionary);
 *     while (xvariant_iter_loop (&iter, "{sv}", &key, &value))
 *       {
 *         g_print ("Item '%s' has type '%s'\n", key,
 *                  xvariant_get_type_string (value));
 *
 *         // no need to free 'key' and 'value' here
 *         // unless breaking out of this loop
 *       }
 *   }
 * ]|
 *
 * For most cases you should use xvariant_iter_next().
 *
 * This function is really only useful when unpacking into #xvariant_t or
 * #xvariant_iter_t in order to allow you to skip the call to
 * xvariant_unref() or xvariant_iter_free().
 *
 * For example, if you are only looping over simple integer and string
 * types, xvariant_iter_next() is definitely preferred.  For string
 * types, use the '&' prefix to avoid allocating any memory at all (and
 * thereby avoiding the need to free anything as well).
 *
 * @format_string determines the C types that are used for unpacking
 * the values and also determines if the values are copied or borrowed.
 *
 * See the section on
 * [xvariant_t format strings][gvariant-format-strings-pointers].
 *
 * Returns: %TRUE if a value was unpacked, or %FALSE if there was no
 *          value
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_iter_loop (xvariant_iter_t *iter,
                     const xchar_t  *format_string,
                     ...)
{
  xboolean_t first_time = GVSI(iter)->loop_format == NULL;
  xvariant_t *value;
  va_list ap;

  g_return_val_if_fail (first_time ||
                        format_string == GVSI(iter)->loop_format,
                        FALSE);

  if (first_time)
    {
      TYPE_CHECK (GVSI(iter)->value, G_VARIANT_TYPE_ARRAY, FALSE);
      GVSI(iter)->loop_format = format_string;

      if (strchr (format_string, '&'))
        xvariant_get_data (GVSI(iter)->value);
    }

  value = xvariant_iter_next_value (iter);

  g_return_val_if_fail (!first_time ||
                        valid_format_string (format_string, TRUE, value),
                        FALSE);

  va_start (ap, format_string);
  xvariant_valist_get (&format_string, value, !first_time, &ap);
  va_end (ap);

  if (value != NULL)
    xvariant_unref (value);

  return value != NULL;
}

/* Serialized data {{{1 */
static xvariant_t *
xvariant_deep_copy (xvariant_t *value)
{
  switch (xvariant_classify (value))
    {
    case XVARIANT_CLASS_MAYBE:
    case XVARIANT_CLASS_ARRAY:
    case XVARIANT_CLASS_TUPLE:
    case XVARIANT_CLASS_DICT_ENTRY:
    case XVARIANT_CLASS_VARIANT:
      {
        xvariant_builder_t builder;
        xvariant_iter_t iter;
        xvariant_t *child;

        xvariant_builder_init (&builder, xvariant_get_type (value));
        xvariant_iter_init (&iter, value);

        while ((child = xvariant_iter_next_value (&iter)))
          {
            xvariant_builder_add_value (&builder, xvariant_deep_copy (child));
            xvariant_unref (child);
          }

        return xvariant_builder_end (&builder);
      }

    case XVARIANT_CLASS_BOOLEAN:
      return xvariant_new_boolean (xvariant_get_boolean (value));

    case XVARIANT_CLASS_BYTE:
      return xvariant_new_byte (xvariant_get_byte (value));

    case XVARIANT_CLASS_INT16:
      return xvariant_new_int16 (xvariant_get_int16 (value));

    case XVARIANT_CLASS_UINT16:
      return xvariant_new_uint16 (xvariant_get_uint16 (value));

    case XVARIANT_CLASS_INT32:
      return xvariant_new_int32 (xvariant_get_int32 (value));

    case XVARIANT_CLASS_UINT32:
      return xvariant_new_uint32 (xvariant_get_uint32 (value));

    case XVARIANT_CLASS_INT64:
      return xvariant_new_int64 (xvariant_get_int64 (value));

    case XVARIANT_CLASS_UINT64:
      return xvariant_new_uint64 (xvariant_get_uint64 (value));

    case XVARIANT_CLASS_HANDLE:
      return xvariant_new_handle (xvariant_get_handle (value));

    case XVARIANT_CLASS_DOUBLE:
      return xvariant_new_double (xvariant_get_double (value));

    case XVARIANT_CLASS_STRING:
      return xvariant_new_string (xvariant_get_string (value, NULL));

    case XVARIANT_CLASS_OBJECT_PATH:
      return xvariant_new_object_path (xvariant_get_string (value, NULL));

    case XVARIANT_CLASS_SIGNATURE:
      return xvariant_new_signature (xvariant_get_string (value, NULL));
    }

  g_assert_not_reached ();
}

/**
 * xvariant_get_normal_form:
 * @value: a #xvariant_t
 *
 * Gets a #xvariant_t instance that has the same value as @value and is
 * trusted to be in normal form.
 *
 * If @value is already trusted to be in normal form then a new
 * reference to @value is returned.
 *
 * If @value is not already trusted, then it is scanned to check if it
 * is in normal form.  If it is found to be in normal form then it is
 * marked as trusted and a new reference to it is returned.
 *
 * If @value is found not to be in normal form then a new trusted
 * #xvariant_t is created with the same value as @value.
 *
 * It makes sense to call this function if you've received #xvariant_t
 * data from untrusted sources and you want to ensure your serialized
 * output is definitely in normal form.
 *
 * If @value is already in normal form, a new reference will be returned
 * (which will be floating if @value is floating). If it is not in normal form,
 * the newly created #xvariant_t will be returned with a single non-floating
 * reference. Typically, xvariant_take_ref() should be called on the return
 * value from this function to guarantee ownership of a single non-floating
 * reference to it.
 *
 * Returns: (transfer full): a trusted #xvariant_t
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_get_normal_form (xvariant_t *value)
{
  xvariant_t *trusted;

  if (xvariant_is_normal_form (value))
    return xvariant_ref (value);

  trusted = xvariant_deep_copy (value);
  g_assert (xvariant_is_trusted (trusted));

  return xvariant_ref_sink (trusted);
}

/**
 * xvariant_byteswap:
 * @value: a #xvariant_t
 *
 * Performs a byteswapping operation on the contents of @value.  The
 * result is that all multi-byte numeric data contained in @value is
 * byteswapped.  That includes 16, 32, and 64bit signed and unsigned
 * integers as well as file handles and double precision floating point
 * values.
 *
 * This function is an identity mapping on any value that does not
 * contain multi-byte numeric data.  That include strings, booleans,
 * bytes and containers containing only these things (recursively).
 *
 * The returned value is always in normal form and is marked as trusted.
 *
 * Returns: (transfer full): the byteswapped form of @value
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_byteswap (xvariant_t *value)
{
  GVariantTypeInfo *type_info;
  xuint_t alignment;
  xvariant_t *new;

  type_info = xvariant_get_type_info (value);

  xvariant_type_info_query (type_info, &alignment, NULL);

  if (alignment)
    /* (potentially) contains multi-byte numeric data */
    {
      GVariantSerialised serialised;
      xvariant_t *trusted;
      xbytes_t *bytes;

      trusted = xvariant_get_normal_form (value);
      serialised.type_info = xvariant_get_type_info (trusted);
      serialised.size = xvariant_get_size (trusted);
      serialised.data = g_malloc (serialised.size);
      serialised.depth = xvariant_get_depth (trusted);
      xvariant_store (trusted, serialised.data);
      xvariant_unref (trusted);

      xvariant_serialised_byteswap (serialised);

      bytes = xbytes_new_take (serialised.data, serialised.size);
      new = xvariant_new_from_bytes (xvariant_get_type (value), bytes, TRUE);
      xbytes_unref (bytes);
    }
  else
    /* contains no multi-byte data */
    new = value;

  return xvariant_ref_sink (new);
}

/**
 * xvariant_new_from_data:
 * @type: a definite #xvariant_type_t
 * @data: (array length=size) (element-type xuint8_t): the serialized data
 * @size: the size of @data
 * @trusted: %TRUE if @data is definitely in normal form
 * @notify: (scope async): function to call when @data is no longer needed
 * @user_data: data for @notify
 *
 * Creates a new #xvariant_t instance from serialized data.
 *
 * @type is the type of #xvariant_t instance that will be constructed.
 * The interpretation of @data depends on knowing the type.
 *
 * @data is not modified by this function and must remain valid with an
 * unchanging value until such a time as @notify is called with
 * @user_data.  If the contents of @data change before that time then
 * the result is undefined.
 *
 * If @data is trusted to be serialized data in normal form then
 * @trusted should be %TRUE.  This applies to serialized data created
 * within this process or read from a trusted location on the disk (such
 * as a file installed in /usr/lib alongside your application).  You
 * should set trusted to %FALSE if @data is read from the network, a
 * file in the user's home directory, etc.
 *
 * If @data was not stored in this machine's native endianness, any multi-byte
 * numeric values in the returned variant will also be in non-native
 * endianness. xvariant_byteswap() can be used to recover the original values.
 *
 * @notify will be called with @user_data when @data is no longer
 * needed.  The exact time of this call is unspecified and might even be
 * before this function returns.
 *
 * Note: @data must be backed by memory that is aligned appropriately for the
 * @type being loaded. Otherwise this function will internally create a copy of
 * the memory (since GLib 2.60) or (in older versions) fail and exit the
 * process.
 *
 * Returns: (transfer none): a new floating #xvariant_t of type @type
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_new_from_data (const xvariant_type_t *type,
                         xconstpointer       data,
                         xsize_t               size,
                         xboolean_t            trusted,
                         xdestroy_notify_t      notify,
                         xpointer_t            user_data)
{
  xvariant_t *value;
  xbytes_t *bytes;

  g_return_val_if_fail (xvariant_type_is_definite (type), NULL);
  g_return_val_if_fail (data != NULL || size == 0, NULL);

  if (notify)
    bytes = xbytes_new_with_free_func (data, size, notify, user_data);
  else
    bytes = xbytes_new_static (data, size);

  value = xvariant_new_from_bytes (type, bytes, trusted);
  xbytes_unref (bytes);

  return value;
}

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
