/*
 * Copyright © 2007, 2008 Ryan Lortie
 * Copyright © 2009, 2010 Codethink Limited
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

#ifndef __G_VARIANT_TYPE_H__
#define __G_VARIANT_TYPE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

/**
 * xvariant_type_t:
 *
 * A type in the xvariant_t type system.
 *
 * Two types may not be compared by value; use xvariant_type_equal() or
 * xvariant_type_is_subtype_of().  May be copied using
 * xvariant_type_copy() and freed using xvariant_type_free().
 **/
typedef struct _GVariantType xvariant_type_t;

/**
 * G_VARIANT_TYPE_BOOLEAN:
 *
 * The type of a value that can be either %TRUE or %FALSE.
 **/
#define G_VARIANT_TYPE_BOOLEAN              ((const xvariant_type_t *) "b")

/**
 * G_VARIANT_TYPE_BYTE:
 *
 * The type of an integer value that can range from 0 to 255.
 **/
#define G_VARIANT_TYPE_BYTE                 ((const xvariant_type_t *) "y")

/**
 * G_VARIANT_TYPE_INT16:
 *
 * The type of an integer value that can range from -32768 to 32767.
 **/
#define G_VARIANT_TYPE_INT16                ((const xvariant_type_t *) "n")

/**
 * G_VARIANT_TYPE_UINT16:
 *
 * The type of an integer value that can range from 0 to 65535.
 * There were about this many people living in Toronto in the 1870s.
 **/
#define G_VARIANT_TYPE_UINT16               ((const xvariant_type_t *) "q")

/**
 * G_VARIANT_TYPE_INT32:
 *
 * The type of an integer value that can range from -2147483648 to
 * 2147483647.
 **/
#define G_VARIANT_TYPE_INT32                ((const xvariant_type_t *) "i")

/**
 * G_VARIANT_TYPE_UINT32:
 *
 * The type of an integer value that can range from 0 to 4294967295.
 * That's one number for everyone who was around in the late 1970s.
 **/
#define G_VARIANT_TYPE_UINT32               ((const xvariant_type_t *) "u")

/**
 * G_VARIANT_TYPE_INT64:
 *
 * The type of an integer value that can range from
 * -9223372036854775808 to 9223372036854775807.
 **/
#define G_VARIANT_TYPE_INT64                ((const xvariant_type_t *) "x")

/**
 * G_VARIANT_TYPE_UINT64:
 *
 * The type of an integer value that can range from 0
 * to 18446744073709551615 (inclusive).  That's a really big number,
 * but a Rubik's cube can have a bit more than twice as many possible
 * positions.
 **/
#define G_VARIANT_TYPE_UINT64               ((const xvariant_type_t *) "t")

/**
 * G_VARIANT_TYPE_DOUBLE:
 *
 * The type of a double precision IEEE754 floating point number.
 * These guys go up to about 1.80e308 (plus and minus) but miss out on
 * some numbers in between.  In any case, that's far greater than the
 * estimated number of fundamental particles in the observable
 * universe.
 **/
#define G_VARIANT_TYPE_DOUBLE               ((const xvariant_type_t *) "d")

/**
 * G_VARIANT_TYPE_STRING:
 *
 * The type of a string.  "" is a string.  %NULL is not a string.
 **/
#define G_VARIANT_TYPE_STRING               ((const xvariant_type_t *) "s")

/**
 * G_VARIANT_TYPE_OBJECT_PATH:
 *
 * The type of a D-Bus object reference.  These are strings of a
 * specific format used to identify objects at a given destination on
 * the bus.
 *
 * If you are not interacting with D-Bus, then there is no reason to make
 * use of this type.  If you are, then the D-Bus specification contains a
 * precise description of valid object paths.
 **/
#define G_VARIANT_TYPE_OBJECT_PATH          ((const xvariant_type_t *) "o")

/**
 * G_VARIANT_TYPE_SIGNATURE:
 *
 * The type of a D-Bus type signature.  These are strings of a specific
 * format used as type signatures for D-Bus methods and messages.
 *
 * If you are not interacting with D-Bus, then there is no reason to make
 * use of this type.  If you are, then the D-Bus specification contains a
 * precise description of valid signature strings.
 **/
#define G_VARIANT_TYPE_SIGNATURE            ((const xvariant_type_t *) "g")

/**
 * G_VARIANT_TYPE_VARIANT:
 *
 * The type of a box that contains any other value (including another
 * variant).
 **/
#define G_VARIANT_TYPE_VARIANT              ((const xvariant_type_t *) "v")

/**
 * G_VARIANT_TYPE_HANDLE:
 *
 * The type of a 32bit signed integer value, that by convention, is used
 * as an index into an array of file descriptors that are sent alongside
 * a D-Bus message.
 *
 * If you are not interacting with D-Bus, then there is no reason to make
 * use of this type.
 **/
#define G_VARIANT_TYPE_HANDLE               ((const xvariant_type_t *) "h")

/**
 * G_VARIANT_TYPE_UNIT:
 *
 * The empty tuple type.  Has only one instance.  Known also as "triv"
 * or "void".
 **/
#define G_VARIANT_TYPE_UNIT                 ((const xvariant_type_t *) "()")

/**
 * G_VARIANT_TYPE_ANY:
 *
 * An indefinite type that is a supertype of every type (including
 * itself).
 **/
#define G_VARIANT_TYPE_ANY                  ((const xvariant_type_t *) "*")

/**
 * G_VARIANT_TYPE_BASIC:
 *
 * An indefinite type that is a supertype of every basic (ie:
 * non-container) type.
 **/
#define G_VARIANT_TYPE_BASIC                ((const xvariant_type_t *) "?")

/**
 * G_VARIANT_TYPE_MAYBE:
 *
 * An indefinite type that is a supertype of every maybe type.
 **/
#define G_VARIANT_TYPE_MAYBE                ((const xvariant_type_t *) "m*")

/**
 * G_VARIANT_TYPE_ARRAY:
 *
 * An indefinite type that is a supertype of every array type.
 **/
#define G_VARIANT_TYPE_ARRAY                ((const xvariant_type_t *) "a*")

/**
 * G_VARIANT_TYPE_TUPLE:
 *
 * An indefinite type that is a supertype of every tuple type,
 * regardless of the number of items in the tuple.
 **/
#define G_VARIANT_TYPE_TUPLE                ((const xvariant_type_t *) "r")

/**
 * G_VARIANT_TYPE_DICT_ENTRY:
 *
 * An indefinite type that is a supertype of every dictionary entry
 * type.
 **/
#define G_VARIANT_TYPE_DICT_ENTRY           ((const xvariant_type_t *) "{?*}")

/**
 * G_VARIANT_TYPE_DICTIONARY:
 *
 * An indefinite type that is a supertype of every dictionary type --
 * that is, any array type that has an element type equal to any
 * dictionary entry type.
 **/
#define G_VARIANT_TYPE_DICTIONARY           ((const xvariant_type_t *) "a{?*}")

/**
 * G_VARIANT_TYPE_STRING_ARRAY:
 *
 * The type of an array of strings.
 **/
#define G_VARIANT_TYPE_STRING_ARRAY         ((const xvariant_type_t *) "as")

/**
 * G_VARIANT_TYPE_OBJECT_PATH_ARRAY:
 *
 * The type of an array of object paths.
 **/
#define G_VARIANT_TYPE_OBJECT_PATH_ARRAY    ((const xvariant_type_t *) "ao")

/**
 * G_VARIANT_TYPE_BYTESTRING:
 *
 * The type of an array of bytes.  This type is commonly used to pass
 * around strings that may not be valid utf8.  In that case, the
 * convention is that the nul terminator character should be included as
 * the last character in the array.
 **/
#define G_VARIANT_TYPE_BYTESTRING           ((const xvariant_type_t *) "ay")

/**
 * G_VARIANT_TYPE_BYTESTRING_ARRAY:
 *
 * The type of an array of byte strings (an array of arrays of bytes).
 **/
#define G_VARIANT_TYPE_BYTESTRING_ARRAY     ((const xvariant_type_t *) "aay")

/**
 * G_VARIANT_TYPE_VARDICT:
 *
 * The type of a dictionary mapping strings to variants (the ubiquitous
 * "a{sv}" type).
 *
 * Since: 2.30
 **/
#define G_VARIANT_TYPE_VARDICT              ((const xvariant_type_t *) "a{sv}")


/**
 * G_VARIANT_TYPE:
 * @type_string: a well-formed #xvariant_type_t type string
 *
 * Converts a string to a const #xvariant_type_t.  Depending on the
 * current debugging level, this function may perform a runtime check
 * to ensure that @string is a valid xvariant_t type string.
 *
 * It is always a programmer error to use this macro with an invalid
 * type string. If in doubt, use xvariant_type_string_is_valid() to
 * check if the string is valid.
 *
 * Since 2.24
 **/
#ifndef G_DISABLE_CHECKS
# define G_VARIANT_TYPE(type_string)            (xvariant_type_checked_ ((type_string)))
#else
# define G_VARIANT_TYPE(type_string)            ((const xvariant_type_t *) (type_string))
#endif

/* type string checking */
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_string_is_valid          (const xchar_t         *type_string);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_string_scan              (const xchar_t         *string,
                                                                         const xchar_t         *limit,
                                                                         const xchar_t        **endptr);

/* create/destroy */
XPL_AVAILABLE_IN_ALL
void                            xvariant_type_free                     (xvariant_type_t        *type);
XPL_AVAILABLE_IN_ALL
xvariant_type_t *                  xvariant_type_copy                     (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xvariant_type_t *                  xvariant_type_new                      (const xchar_t         *type_string);

/* getters */
XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_type_get_string_length        (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   xvariant_type_peek_string              (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xchar_t *                         xvariant_type_dup_string               (const xvariant_type_t  *type);

/* classification */
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_definite              (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_container             (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_basic                 (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_maybe                 (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_array                 (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_tuple                 (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_dict_entry            (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_variant               (const xvariant_type_t  *type);

/* for hash tables */
XPL_AVAILABLE_IN_ALL
xuint_t                           xvariant_type_hash                     (xconstpointer        type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_equal                    (xconstpointer        type1,
                                                                         xconstpointer        type2);

/* subtypes */
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_type_is_subtype_of            (const xvariant_type_t  *type,
                                                                         const xvariant_type_t  *supertype);

/* type iterator interface */
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_type_element                  (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_type_first                    (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_type_next                     (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_type_n_items                  (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_type_key                      (const xvariant_type_t  *type);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_type_value                    (const xvariant_type_t  *type);

/* constructors */
XPL_AVAILABLE_IN_ALL
xvariant_type_t *                  xvariant_type_new_array                (const xvariant_type_t  *element);
XPL_AVAILABLE_IN_ALL
xvariant_type_t *                  xvariant_type_new_maybe                (const xvariant_type_t  *element);
XPL_AVAILABLE_IN_ALL
xvariant_type_t *                  xvariant_type_new_tuple                (const xvariant_type_t * const *items,
                                                                         xint_t                 length);
XPL_AVAILABLE_IN_ALL
xvariant_type_t *                  xvariant_type_new_dict_entry           (const xvariant_type_t  *key,
                                                                         const xvariant_type_t  *value);

/*< private >*/
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_type_checked_                 (const xchar_t *);
XPL_AVAILABLE_IN_2_60
xsize_t                           xvariant_type_string_get_depth_        (const xchar_t *type_string);

G_END_DECLS

#endif /* __G_VARIANT_TYPE_H__ */
