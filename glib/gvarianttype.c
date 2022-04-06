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

#include "config.h"

#include "gvarianttype.h"

#include <glib/gtestutils.h>
#include <glib/gstrfuncs.h>
#include <glib/gvariant-internal.h>

#include <string.h>


/**
 * SECTION:gvarianttype
 * @title: xvariant_type_t
 * @short_description: introduction to the xvariant_t type system
 * @see_also: #xvariant_type_t, #xvariant_t
 *
 * This section introduces the xvariant_t type system. It is based, in
 * large part, on the D-Bus type system, with two major changes and
 * some minor lifting of restrictions. The
 * [D-Bus specification](http://dbus.freedesktop.org/doc/dbus-specification.html),
 * therefore, provides a significant amount of
 * information that is useful when working with xvariant_t.
 *
 * The first major change with respect to the D-Bus type system is the
 * introduction of maybe (or "nullable") types.  Any type in xvariant_t can be
 * converted to a maybe type, in which case, "nothing" (or "null") becomes a
 * valid value.  Maybe types have been added by introducing the
 * character "m" to type strings.
 *
 * The second major change is that the xvariant_t type system supports the
 * concept of "indefinite types" -- types that are less specific than
 * the normal types found in D-Bus.  For example, it is possible to speak
 * of "an array of any type" in xvariant_t, where the D-Bus type system
 * would require you to speak of "an array of integers" or "an array of
 * strings".  Indefinite types have been added by introducing the
 * characters "*", "?" and "r" to type strings.
 *
 * Finally, all arbitrary restrictions relating to the complexity of
 * types are lifted along with the restriction that dictionary entries
 * may only appear nested inside of arrays.
 *
 * Just as in D-Bus, xvariant_t types are described with strings ("type
 * strings").  Subject to the differences mentioned above, these strings
 * are of the same form as those found in D-Bus.  Note, however: D-Bus
 * always works in terms of messages and therefore individual type
 * strings appear nowhere in its interface.  Instead, "signatures"
 * are a concatenation of the strings of the type of each argument in a
 * message.  xvariant_t deals with single values directly so xvariant_t type
 * strings always describe the type of exactly one value.  This means
 * that a D-Bus signature string is generally not a valid xvariant_t type
 * string -- except in the case that it is the signature of a message
 * containing exactly one argument.
 *
 * An indefinite type is similar in spirit to what may be called an
 * abstract type in other type systems.  No value can exist that has an
 * indefinite type as its type, but values can exist that have types
 * that are subtypes of indefinite types.  That is to say,
 * xvariant_get_type() will never return an indefinite type, but
 * calling xvariant_is_of_type() with an indefinite type may return
 * %TRUE.  For example, you cannot have a value that represents "an
 * array of no particular type", but you can have an "array of integers"
 * which certainly matches the type of "an array of no particular type",
 * since "array of integers" is a subtype of "array of no particular
 * type".
 *
 * This is similar to how instances of abstract classes may not
 * directly exist in other type systems, but instances of their
 * non-abstract subtypes may.  For example, in GTK, no object that has
 * the type of #GtkBin can exist (since #GtkBin is an abstract class),
 * but a #GtkWindow can certainly be instantiated, and you would say
 * that the #GtkWindow is a #GtkBin (since #GtkWindow is a subclass of
 * #GtkBin).
 *
 * ## xvariant_t Type Strings
 *
 * A xvariant_t type string can be any of the following:
 *
 * - any basic type string (listed below)
 *
 * - "v", "r" or "*"
 *
 * - one of the characters 'a' or 'm', followed by another type string
 *
 * - the character '(', followed by a concatenation of zero or more other
 *   type strings, followed by the character ')'
 *
 * - the character '{', followed by a basic type string (see below),
 *   followed by another type string, followed by the character '}'
 *
 * A basic type string describes a basic type (as per
 * xvariant_type_is_basic()) and is always a single character in length.
 * The valid basic type strings are "b", "y", "n", "q", "i", "u", "x", "t",
 * "h", "d", "s", "o", "g" and "?".
 *
 * The above definition is recursive to arbitrary depth. "aaaaai" and
 * "(ui(nq((y)))s)" are both valid type strings, as is
 * "a(aa(ui)(qna{ya(yd)}))". In order to not hit memory limits, #xvariant_t
 * imposes a limit on recursion depth of 65 nested containers. This is the
 * limit in the D-Bus specification (64) plus one to allow a #xdbus_message_t to
 * be nested in a top-level tuple.
 *
 * The meaning of each of the characters is as follows:
 * - `b`: the type string of %G_VARIANT_TYPE_BOOLEAN; a boolean value.
 * - `y`: the type string of %G_VARIANT_TYPE_BYTE; a byte.
 * - `n`: the type string of %G_VARIANT_TYPE_INT16; a signed 16 bit integer.
 * - `q`: the type string of %G_VARIANT_TYPE_UINT16; an unsigned 16 bit integer.
 * - `i`: the type string of %G_VARIANT_TYPE_INT32; a signed 32 bit integer.
 * - `u`: the type string of %G_VARIANT_TYPE_UINT32; an unsigned 32 bit integer.
 * - `x`: the type string of %G_VARIANT_TYPE_INT64; a signed 64 bit integer.
 * - `t`: the type string of %G_VARIANT_TYPE_UINT64; an unsigned 64 bit integer.
 * - `h`: the type string of %G_VARIANT_TYPE_HANDLE; a signed 32 bit value
 *   that, by convention, is used as an index into an array of file
 *   descriptors that are sent alongside a D-Bus message.
 * - `d`: the type string of %G_VARIANT_TYPE_DOUBLE; a double precision
 *   floating point value.
 * - `s`: the type string of %G_VARIANT_TYPE_STRING; a string.
 * - `o`: the type string of %G_VARIANT_TYPE_OBJECT_PATH; a string in the form
 *   of a D-Bus object path.
 * - `g`: the type string of %G_VARIANT_TYPE_SIGNATURE; a string in the form of
 *   a D-Bus type signature.
 * - `?`: the type string of %G_VARIANT_TYPE_BASIC; an indefinite type that
 *   is a supertype of any of the basic types.
 * - `v`: the type string of %G_VARIANT_TYPE_VARIANT; a container type that
 *   contain any other type of value.
 * - `a`: used as a prefix on another type string to mean an array of that
 *   type; the type string "ai", for example, is the type of an array of
 *   signed 32-bit integers.
 * - `m`: used as a prefix on another type string to mean a "maybe", or
 *   "nullable", version of that type; the type string "ms", for example,
 *   is the type of a value that maybe contains a string, or maybe contains
 *   nothing.
 * - `()`: used to enclose zero or more other concatenated type strings to
 *   create a tuple type; the type string "(is)", for example, is the type of
 *   a pair of an integer and a string.
 * - `r`: the type string of %G_VARIANT_TYPE_TUPLE; an indefinite type that is
 *   a supertype of any tuple type, regardless of the number of items.
 * - `{}`: used to enclose a basic type string concatenated with another type
 *   string to create a dictionary entry type, which usually appears inside of
 *   an array to form a dictionary; the type string "a{sd}", for example, is
 *   the type of a dictionary that maps strings to double precision floating
 *   point values.
 *
 *   The first type (the basic type) is the key type and the second type is
 *   the value type. The reason that the first type is restricted to being a
 *   basic type is so that it can easily be hashed.
 * - `*`: the type string of %G_VARIANT_TYPE_ANY; the indefinite type that is
 *   a supertype of all types.  Note that, as with all type strings, this
 *   character represents exactly one type. It cannot be used inside of tuples
 *   to mean "any number of items".
 *
 * Any type string of a container that contains an indefinite type is,
 * itself, an indefinite type. For example, the type string "a*"
 * (corresponding to %G_VARIANT_TYPE_ARRAY) is an indefinite type
 * that is a supertype of every array type. "(*s)" is a supertype
 * of all tuples that contain exactly two items where the second
 * item is a string.
 *
 * "a{?*}" is an indefinite type that is a supertype of all arrays
 * containing dictionary entries where the key is any basic type and
 * the value is any type at all.  This is, by definition, a dictionary,
 * so this type string corresponds to %G_VARIANT_TYPE_DICTIONARY. Note
 * that, due to the restriction that the key of a dictionary entry must
 * be a basic type, "{**}" is not a valid type string.
 */


static xboolean_t
xvariant_type_check (const xvariant_type_t *type)
{
  if (type == NULL)
    return FALSE;

#if 0
  return xvariant_type_string_scan ((const xchar_t *) type, NULL, NULL);
#else
  return TRUE;
#endif
}

static xboolean_t
variant_type_string_scan_internal (const xchar_t  *string,
                                   const xchar_t  *limit,
                                   const xchar_t **endptr,
                                   xsize_t        *depth,
                                   xsize_t         depth_limit)
{
  xsize_t max_depth = 0, child_depth;

  g_return_val_if_fail (string != NULL, FALSE);

  if (string == limit || *string == '\0')
    return FALSE;

  switch (*string++)
    {
    case '(':
      while (string == limit || *string != ')')
        {
          if (depth_limit == 0 ||
              !variant_type_string_scan_internal (string, limit, &string,
                                                  &child_depth,
                                                  depth_limit - 1))
            return FALSE;

          max_depth = MAX (max_depth, child_depth + 1);
        }

      string++;
      break;

    case '{':
      if (depth_limit == 0 ||
          string == limit || *string == '\0' ||                                  /* { */
          !strchr ("bynqihuxtdsog?", *string++) ||                               /* key */
          !variant_type_string_scan_internal (string, limit, &string,
                                              &child_depth, depth_limit - 1) ||  /* value */
          string == limit || *string++ != '}')                                   /* } */
        return FALSE;

      max_depth = MAX (max_depth, child_depth + 1);
      break;

    case 'm': case 'a':
      if (depth_limit == 0 ||
          !variant_type_string_scan_internal (string, limit, &string,
                                              &child_depth, depth_limit - 1))
        return FALSE;

      max_depth = MAX (max_depth, child_depth + 1);
      break;

    case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
    case 'x': case 't': case 'd': case 's': case 'o': case 'g':
    case 'v': case 'r': case '*': case '?': case 'h':
      max_depth = MAX (max_depth, 1);
      break;

    default:
      return FALSE;
    }

  if (endptr != NULL)
    *endptr = string;
  if (depth != NULL)
    *depth = max_depth;

  return TRUE;
}

/**
 * xvariant_type_string_scan:
 * @string: a pointer to any string
 * @limit: (nullable): the end of @string, or %NULL
 * @endptr: (out) (optional): location to store the end pointer, or %NULL
 *
 * Scan for a single complete and valid xvariant_t type string in @string.
 * The memory pointed to by @limit (or bytes beyond it) is never
 * accessed.
 *
 * If a valid type string is found, @endptr is updated to point to the
 * first character past the end of the string that was found and %TRUE
 * is returned.
 *
 * If there is no valid type string starting at @string, or if the type
 * string does not end before @limit then %FALSE is returned.
 *
 * For the simple case of checking if a string is a valid type string,
 * see xvariant_type_string_is_valid().
 *
 * Returns: %TRUE if a valid type string was found
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_type_string_scan (const xchar_t  *string,
                            const xchar_t  *limit,
                            const xchar_t **endptr)
{
  return variant_type_string_scan_internal (string, limit, endptr, NULL,
                                            G_VARIANT_MAX_RECURSION_DEPTH);
}

/* < private >
 * xvariant_type_string_get_depth_:
 * @type_string: a pointer to any string
 *
 * Get the maximum depth of the nested types in @type_string. A basic type will
 * return depth 1, and a container type will return a greater value. The depth
 * of a tuple is 1 plus the depth of its deepest child type.
 *
 * If @type_string is not a valid #xvariant_t type string, 0 will be returned.
 *
 * Returns: depth of @type_string, or 0 on error
 * Since: 2.60
 */
xsize_t
xvariant_type_string_get_depth_ (const xchar_t *type_string)
{
  const xchar_t *endptr;
  xsize_t depth = 0;

  g_return_val_if_fail (type_string != NULL, 0);

  if (!variant_type_string_scan_internal (type_string, NULL, &endptr, &depth,
                                          G_VARIANT_MAX_RECURSION_DEPTH) ||
      *endptr != '\0')
    return 0;

  return depth;
}

/**
 * xvariant_type_string_is_valid:
 * @type_string: a pointer to any string
 *
 * Checks if @type_string is a valid xvariant_t type string.  This call is
 * equivalent to calling xvariant_type_string_scan() and confirming
 * that the following character is a nul terminator.
 *
 * Returns: %TRUE if @type_string is exactly one valid type string
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_string_is_valid (const xchar_t *type_string)
{
  const xchar_t *endptr;

  g_return_val_if_fail (type_string != NULL, FALSE);

  if (!xvariant_type_string_scan (type_string, NULL, &endptr))
    return FALSE;

  return *endptr == '\0';
}

/**
 * xvariant_type_free:
 * @type: (nullable): a #xvariant_type_t, or %NULL
 *
 * Frees a #xvariant_type_t that was allocated with
 * xvariant_type_copy(), xvariant_type_new() or one of the container
 * type constructor functions.
 *
 * In the case that @type is %NULL, this function does nothing.
 *
 * Since 2.24
 **/
void
xvariant_type_free (xvariant_type_t *type)
{
  g_return_if_fail (type == NULL || xvariant_type_check (type));

  g_free (type);
}

/**
 * xvariant_type_copy:
 * @type: a #xvariant_type_t
 *
 * Makes a copy of a #xvariant_type_t.  It is appropriate to call
 * xvariant_type_free() on the return value.  @type may not be %NULL.
 *
 * Returns: (transfer full): a new #xvariant_type_t
 *
 * Since 2.24
 **/
xvariant_type_t *
xvariant_type_copy (const xvariant_type_t *type)
{
  xsize_t length;
  xchar_t *new;

  g_return_val_if_fail (xvariant_type_check (type), NULL);

  length = xvariant_type_get_string_length (type);
  new = g_malloc (length + 1);

  memcpy (new, type, length);
  new[length] = '\0';

  return (xvariant_type_t *) new;
}

/**
 * xvariant_type_new:
 * @type_string: a valid xvariant_t type string
 *
 * Creates a new #xvariant_type_t corresponding to the type string given
 * by @type_string.  It is appropriate to call xvariant_type_free() on
 * the return value.
 *
 * It is a programmer error to call this function with an invalid type
 * string.  Use xvariant_type_string_is_valid() if you are unsure.
 *
 * Returns: (transfer full): a new #xvariant_type_t
 *
 * Since: 2.24
 */
xvariant_type_t *
xvariant_type_new (const xchar_t *type_string)
{
  g_return_val_if_fail (type_string != NULL, NULL);

  return xvariant_type_copy (G_VARIANT_TYPE (type_string));
}

/**
 * xvariant_type_get_string_length:
 * @type: a #xvariant_type_t
 *
 * Returns the length of the type string corresponding to the given
 * @type.  This function must be used to determine the valid extent of
 * the memory region returned by xvariant_type_peek_string().
 *
 * Returns: the length of the corresponding type string
 *
 * Since 2.24
 **/
xsize_t
xvariant_type_get_string_length (const xvariant_type_t *type)
{
  const xchar_t *type_string = (const xchar_t *) type;
  xint_t brackets = 0;
  xsize_t index = 0;

  g_return_val_if_fail (xvariant_type_check (type), 0);

  do
    {
      while (type_string[index] == 'a' || type_string[index] == 'm')
        index++;

      if (type_string[index] == '(' || type_string[index] == '{')
        brackets++;

      else if (type_string[index] == ')' || type_string[index] == '}')
        brackets--;

      index++;
    }
  while (brackets);

  return index;
}

/*
  This function is not introspectable, it returns something that
  is not an array and neither a string
*/
/**
 * xvariant_type_peek_string: (skip)
 * @type: a #xvariant_type_t
 *
 * Returns the type string corresponding to the given @type.  The
 * result is not nul-terminated; in order to determine its length you
 * must call xvariant_type_get_string_length().
 *
 * To get a nul-terminated string, see xvariant_type_dup_string().
 *
 * Returns: the corresponding type string (not nul-terminated)
 *
 * Since 2.24
 **/
const xchar_t *
xvariant_type_peek_string (const xvariant_type_t *type)
{
  g_return_val_if_fail (xvariant_type_check (type), NULL);

  return (const xchar_t *) type;
}

/**
 * xvariant_type_dup_string:
 * @type: a #xvariant_type_t
 *
 * Returns a newly-allocated copy of the type string corresponding to
 * @type.  The returned string is nul-terminated.  It is appropriate to
 * call g_free() on the return value.
 *
 * Returns: (transfer full): the corresponding type string
 *
 * Since 2.24
 **/
xchar_t *
xvariant_type_dup_string (const xvariant_type_t *type)
{
  g_return_val_if_fail (xvariant_type_check (type), NULL);

  return xstrndup (xvariant_type_peek_string (type),
                    xvariant_type_get_string_length (type));
}

/**
 * xvariant_type_is_definite:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is definite (ie: not indefinite).
 *
 * A type is definite if its type string does not contain any indefinite
 * type characters ('*', '?', or 'r').
 *
 * A #xvariant_t instance may not have an indefinite type, so calling
 * this function on the result of xvariant_get_type() will always
 * result in %TRUE being returned.  Calling this function on an
 * indefinite type like %G_VARIANT_TYPE_ARRAY, however, will result in
 * %FALSE being returned.
 *
 * Returns: %TRUE if @type is definite
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_definite (const xvariant_type_t *type)
{
  const xchar_t *type_string;
  xsize_t type_length;
  xsize_t i;

  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  type_length = xvariant_type_get_string_length (type);
  type_string = xvariant_type_peek_string (type);

  for (i = 0; i < type_length; i++)
    if (type_string[i] == '*' ||
        type_string[i] == '?' ||
        type_string[i] == 'r')
      return FALSE;

  return TRUE;
}

/**
 * xvariant_type_is_container:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is a container type.
 *
 * Container types are any array, maybe, tuple, or dictionary
 * entry types plus the variant type.
 *
 * This function returns %TRUE for any indefinite type for which every
 * definite subtype is a container -- %G_VARIANT_TYPE_ARRAY, for
 * example.
 *
 * Returns: %TRUE if @type is a container type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_container (const xvariant_type_t *type)
{
  xchar_t first_char;

  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  first_char = xvariant_type_peek_string (type)[0];
  switch (first_char)
  {
    case 'a':
    case 'm':
    case 'r':
    case '(':
    case '{':
    case 'v':
      return TRUE;

    default:
      return FALSE;
  }
}

/**
 * xvariant_type_is_basic:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is a basic type.
 *
 * Basic types are booleans, bytes, integers, doubles, strings, object
 * paths and signatures.
 *
 * Only a basic type may be used as the key of a dictionary entry.
 *
 * This function returns %FALSE for all indefinite types except
 * %G_VARIANT_TYPE_BASIC.
 *
 * Returns: %TRUE if @type is a basic type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_basic (const xvariant_type_t *type)
{
  xchar_t first_char;

  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  first_char = xvariant_type_peek_string (type)[0];
  switch (first_char)
  {
    case 'b':
    case 'y':
    case 'n':
    case 'q':
    case 'i':
    case 'h':
    case 'u':
    case 't':
    case 'x':
    case 'd':
    case 's':
    case 'o':
    case 'g':
    case '?':
      return TRUE;

    default:
      return FALSE;
  }
}

/**
 * xvariant_type_is_maybe:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is a maybe type.  This is true if the
 * type string for @type starts with an 'm'.
 *
 * This function returns %TRUE for any indefinite type for which every
 * definite subtype is a maybe type -- %G_VARIANT_TYPE_MAYBE, for
 * example.
 *
 * Returns: %TRUE if @type is a maybe type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_maybe (const xvariant_type_t *type)
{
  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  return xvariant_type_peek_string (type)[0] == 'm';
}

/**
 * xvariant_type_is_array:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is an array type.  This is true if the
 * type string for @type starts with an 'a'.
 *
 * This function returns %TRUE for any indefinite type for which every
 * definite subtype is an array type -- %G_VARIANT_TYPE_ARRAY, for
 * example.
 *
 * Returns: %TRUE if @type is an array type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_array (const xvariant_type_t *type)
{
  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  return xvariant_type_peek_string (type)[0] == 'a';
}

/**
 * xvariant_type_is_tuple:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is a tuple type.  This is true if the
 * type string for @type starts with a '(' or if @type is
 * %G_VARIANT_TYPE_TUPLE.
 *
 * This function returns %TRUE for any indefinite type for which every
 * definite subtype is a tuple type -- %G_VARIANT_TYPE_TUPLE, for
 * example.
 *
 * Returns: %TRUE if @type is a tuple type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_tuple (const xvariant_type_t *type)
{
  xchar_t type_char;

  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  type_char = xvariant_type_peek_string (type)[0];
  return type_char == 'r' || type_char == '(';
}

/**
 * xvariant_type_is_dict_entry:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is a dictionary entry type.  This is
 * true if the type string for @type starts with a '{'.
 *
 * This function returns %TRUE for any indefinite type for which every
 * definite subtype is a dictionary entry type --
 * %G_VARIANT_TYPE_DICT_ENTRY, for example.
 *
 * Returns: %TRUE if @type is a dictionary entry type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_dict_entry (const xvariant_type_t *type)
{
  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  return xvariant_type_peek_string (type)[0] == '{';
}

/**
 * xvariant_type_is_variant:
 * @type: a #xvariant_type_t
 *
 * Determines if the given @type is the variant type.
 *
 * Returns: %TRUE if @type is the variant type
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_variant (const xvariant_type_t *type)
{
  g_return_val_if_fail (xvariant_type_check (type), FALSE);

  return xvariant_type_peek_string (type)[0] == 'v';
}

/**
 * xvariant_type_hash:
 * @type: (type xvariant_type_t): a #xvariant_type_t
 *
 * Hashes @type.
 *
 * The argument type of @type is only #xconstpointer to allow use with
 * #xhashtable_t without function pointer casting.  A valid
 * #xvariant_type_t must be provided.
 *
 * Returns: the hash value
 *
 * Since 2.24
 **/
xuint_t
xvariant_type_hash (xconstpointer type)
{
  const xchar_t *type_string;
  xuint_t value = 0;
  xsize_t length;
  xsize_t i;

  g_return_val_if_fail (xvariant_type_check (type), 0);

  type_string = xvariant_type_peek_string (type);
  length = xvariant_type_get_string_length (type);

  for (i = 0; i < length; i++)
    value = (value << 5) - value + type_string[i];

  return value;
}

/**
 * xvariant_type_equal:
 * @type1: (type xvariant_type_t): a #xvariant_type_t
 * @type2: (type xvariant_type_t): a #xvariant_type_t
 *
 * Compares @type1 and @type2 for equality.
 *
 * Only returns %TRUE if the types are exactly equal.  Even if one type
 * is an indefinite type and the other is a subtype of it, %FALSE will
 * be returned if they are not exactly equal.  If you want to check for
 * subtypes, use xvariant_type_is_subtype_of().
 *
 * The argument types of @type1 and @type2 are only #xconstpointer to
 * allow use with #xhashtable_t without function pointer casting.  For
 * both arguments, a valid #xvariant_type_t must be provided.
 *
 * Returns: %TRUE if @type1 and @type2 are exactly equal
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_equal (xconstpointer type1,
                      xconstpointer type2)
{
  const xchar_t *string1, *string2;
  xsize_t size1, size2;

  g_return_val_if_fail (xvariant_type_check (type1), FALSE);
  g_return_val_if_fail (xvariant_type_check (type2), FALSE);

  if (type1 == type2)
    return TRUE;

  size1 = xvariant_type_get_string_length (type1);
  size2 = xvariant_type_get_string_length (type2);

  if (size1 != size2)
    return FALSE;

  string1 = xvariant_type_peek_string (type1);
  string2 = xvariant_type_peek_string (type2);

  return memcmp (string1, string2, size1) == 0;
}

/**
 * xvariant_type_is_subtype_of:
 * @type: a #xvariant_type_t
 * @supertype: a #xvariant_type_t
 *
 * Checks if @type is a subtype of @supertype.
 *
 * This function returns %TRUE if @type is a subtype of @supertype.  All
 * types are considered to be subtypes of themselves.  Aside from that,
 * only indefinite types can have subtypes.
 *
 * Returns: %TRUE if @type is a subtype of @supertype
 *
 * Since 2.24
 **/
xboolean_t
xvariant_type_is_subtype_of (const xvariant_type_t *type,
                              const xvariant_type_t *supertype)
{
  const xchar_t *supertype_string;
  const xchar_t *supertype_end;
  const xchar_t *type_string;

  g_return_val_if_fail (xvariant_type_check (type), FALSE);
  g_return_val_if_fail (xvariant_type_check (supertype), FALSE);

  supertype_string = xvariant_type_peek_string (supertype);
  type_string = xvariant_type_peek_string (type);

  supertype_end = supertype_string +
                  xvariant_type_get_string_length (supertype);

  /* we know that type and supertype are both well-formed, so it's
   * safe to treat this merely as a text processing problem.
   */
  while (supertype_string < supertype_end)
    {
      char supertype_char = *supertype_string++;

      if (supertype_char == *type_string)
        type_string++;

      else if (*type_string == ')')
        return FALSE;

      else
        {
          const xvariant_type_t *target_type = (xvariant_type_t *) type_string;

          switch (supertype_char)
            {
            case 'r':
              if (!xvariant_type_is_tuple (target_type))
                return FALSE;
              break;

            case '*':
              break;

            case '?':
              if (!xvariant_type_is_basic (target_type))
                return FALSE;
              break;

            default:
              return FALSE;
            }

          type_string += xvariant_type_get_string_length (target_type);
        }
    }

  return TRUE;
}

/**
 * xvariant_type_element:
 * @type: an array or maybe #xvariant_type_t
 *
 * Determines the element type of an array or maybe type.
 *
 * This function may only be used with array or maybe types.
 *
 * Returns: (transfer none): the element type of @type
 *
 * Since 2.24
 **/
const xvariant_type_t *
xvariant_type_element (const xvariant_type_t *type)
{
  const xchar_t *type_string;

  g_return_val_if_fail (xvariant_type_check (type), NULL);

  type_string = xvariant_type_peek_string (type);

  g_assert (type_string[0] == 'a' || type_string[0] == 'm');

  return (const xvariant_type_t *) &type_string[1];
}

/**
 * xvariant_type_first:
 * @type: a tuple or dictionary entry #xvariant_type_t
 *
 * Determines the first item type of a tuple or dictionary entry
 * type.
 *
 * This function may only be used with tuple or dictionary entry types,
 * but must not be used with the generic tuple type
 * %G_VARIANT_TYPE_TUPLE.
 *
 * In the case of a dictionary entry type, this returns the type of
 * the key.
 *
 * %NULL is returned in case of @type being %G_VARIANT_TYPE_UNIT.
 *
 * This call, together with xvariant_type_next() provides an iterator
 * interface over tuple and dictionary entry types.
 *
 * Returns: (transfer none): the first item type of @type, or %NULL
 *
 * Since 2.24
 **/
const xvariant_type_t *
xvariant_type_first (const xvariant_type_t *type)
{
  const xchar_t *type_string;

  g_return_val_if_fail (xvariant_type_check (type), NULL);

  type_string = xvariant_type_peek_string (type);
  g_assert (type_string[0] == '(' || type_string[0] == '{');

  if (type_string[1] == ')')
    return NULL;

  return (const xvariant_type_t *) &type_string[1];
}

/**
 * xvariant_type_next:
 * @type: a #xvariant_type_t from a previous call
 *
 * Determines the next item type of a tuple or dictionary entry
 * type.
 *
 * @type must be the result of a previous call to
 * xvariant_type_first() or xvariant_type_next().
 *
 * If called on the key type of a dictionary entry then this call
 * returns the value type.  If called on the value type of a dictionary
 * entry then this call returns %NULL.
 *
 * For tuples, %NULL is returned when @type is the last item in a tuple.
 *
 * Returns: (transfer none): the next #xvariant_type_t after @type, or %NULL
 *
 * Since 2.24
 **/
const xvariant_type_t *
xvariant_type_next (const xvariant_type_t *type)
{
  const xchar_t *type_string;

  g_return_val_if_fail (xvariant_type_check (type), NULL);

  type_string = xvariant_type_peek_string (type);
  type_string += xvariant_type_get_string_length (type);

  if (*type_string == ')' || *type_string == '}')
    return NULL;

  return (const xvariant_type_t *) type_string;
}

/**
 * xvariant_type_n_items:
 * @type: a tuple or dictionary entry #xvariant_type_t
 *
 * Determines the number of items contained in a tuple or
 * dictionary entry type.
 *
 * This function may only be used with tuple or dictionary entry types,
 * but must not be used with the generic tuple type
 * %G_VARIANT_TYPE_TUPLE.
 *
 * In the case of a dictionary entry type, this function will always
 * return 2.
 *
 * Returns: the number of items in @type
 *
 * Since 2.24
 **/
xsize_t
xvariant_type_n_items (const xvariant_type_t *type)
{
  xsize_t count = 0;

  g_return_val_if_fail (xvariant_type_check (type), 0);

  for (type = xvariant_type_first (type);
       type;
       type = xvariant_type_next (type))
    count++;

  return count;
}

/**
 * xvariant_type_key:
 * @type: a dictionary entry #xvariant_type_t
 *
 * Determines the key type of a dictionary entry type.
 *
 * This function may only be used with a dictionary entry type.  Other
 * than the additional restriction, this call is equivalent to
 * xvariant_type_first().
 *
 * Returns: (transfer none): the key type of the dictionary entry
 *
 * Since 2.24
 **/
const xvariant_type_t *
xvariant_type_key (const xvariant_type_t *type)
{
  const xchar_t *type_string;

  g_return_val_if_fail (xvariant_type_check (type), NULL);

  type_string = xvariant_type_peek_string (type);
  g_assert (type_string[0] == '{');

  return (const xvariant_type_t *) &type_string[1];
}

/**
 * xvariant_type_value:
 * @type: a dictionary entry #xvariant_type_t
 *
 * Determines the value type of a dictionary entry type.
 *
 * This function may only be used with a dictionary entry type.
 *
 * Returns: (transfer none): the value type of the dictionary entry
 *
 * Since 2.24
 **/
const xvariant_type_t *
xvariant_type_value (const xvariant_type_t *type)
{
#ifndef G_DISABLE_ASSERT
  const xchar_t *type_string;
#endif

  g_return_val_if_fail (xvariant_type_check (type), NULL);

#ifndef G_DISABLE_ASSERT
  type_string = xvariant_type_peek_string (type);
  g_assert (type_string[0] == '{');
#endif

  return xvariant_type_next (xvariant_type_key (type));
}

/**
 * xvariant_type_new_tuple:
 * @items: (array length=length): an array of #GVariantTypes, one for each item
 * @length: the length of @items, or -1
 *
 * Constructs a new tuple type, from @items.
 *
 * @length is the number of items in @items, or -1 to indicate that
 * @items is %NULL-terminated.
 *
 * It is appropriate to call xvariant_type_free() on the return value.
 *
 * Returns: (transfer full): a new tuple #xvariant_type_t
 *
 * Since 2.24
 **/
static xvariant_type_t *
xvariant_type_new_tuple_slow (const xvariant_type_t * const *items,
                               xint_t                        length)
{
  /* the "slow" version is needed in case the static buffer of 1024
   * bytes is exceeded when running the normal version.  this will
   * happen only with very unusually large types, so it can be slow.
   */
  xstring_t *string;
  xint_t i;

  string = xstring_new ("(");
  for (i = 0; i < length; i++)
    {
      const xvariant_type_t *type;
      xsize_t size;

      g_return_val_if_fail (xvariant_type_check (items[i]), NULL);

      type = items[i];
      size = xvariant_type_get_string_length (type);
      xstring_append_len (string, (const xchar_t *) type, size);
    }
  xstring_append_c (string, ')');

  return (xvariant_type_t *) xstring_free (string, FALSE);
}

xvariant_type_t *
xvariant_type_new_tuple (const xvariant_type_t * const *items,
                          xint_t                        length)
{
  char buffer[1024];
  xsize_t offset;
  xsize_t i;
  xsize_t length_unsigned;

  g_return_val_if_fail (length == 0 || items != NULL, NULL);

  if (length < 0)
    for (length_unsigned = 0; items[length_unsigned] != NULL; length_unsigned++);
  else
    length_unsigned = (xsize_t) length;

  offset = 0;
  buffer[offset++] = '(';

  for (i = 0; i < length_unsigned; i++)
    {
      const xvariant_type_t *type;
      xsize_t size;

      g_return_val_if_fail (xvariant_type_check (items[i]), NULL);

      type = items[i];
      size = xvariant_type_get_string_length (type);

      if (offset + size >= sizeof buffer) /* leave room for ')' */
        return xvariant_type_new_tuple_slow (items, length_unsigned);

      memcpy (&buffer[offset], type, size);
      offset += size;
    }

  g_assert (offset < sizeof buffer);
  buffer[offset++] = ')';

  return (xvariant_type_t *) g_memdup2 (buffer, offset);
}

/**
 * xvariant_type_new_array: (constructor)
 * @element: a #xvariant_type_t
 *
 * Constructs the type corresponding to an array of elements of the
 * type @type.
 *
 * It is appropriate to call xvariant_type_free() on the return value.
 *
 * Returns: (transfer full): a new array #xvariant_type_t
 *
 * Since 2.24
 **/
xvariant_type_t *
xvariant_type_new_array (const xvariant_type_t *element)
{
  xsize_t size;
  xchar_t *new;

  g_return_val_if_fail (xvariant_type_check (element), NULL);

  size = xvariant_type_get_string_length (element);
  new = g_malloc (size + 1);

  new[0] = 'a';
  memcpy (new + 1, element, size);

  return (xvariant_type_t *) new;
}

/**
 * xvariant_type_new_maybe: (constructor)
 * @element: a #xvariant_type_t
 *
 * Constructs the type corresponding to a maybe instance containing
 * type @type or Nothing.
 *
 * It is appropriate to call xvariant_type_free() on the return value.
 *
 * Returns: (transfer full): a new maybe #xvariant_type_t
 *
 * Since 2.24
 **/
xvariant_type_t *
xvariant_type_new_maybe (const xvariant_type_t *element)
{
  xsize_t size;
  xchar_t *new;

  g_return_val_if_fail (xvariant_type_check (element), NULL);

  size = xvariant_type_get_string_length (element);
  new = g_malloc (size + 1);

  new[0] = 'm';
  memcpy (new + 1, element, size);

  return (xvariant_type_t *) new;
}

/**
 * xvariant_type_new_dict_entry: (constructor)
 * @key: a basic #xvariant_type_t
 * @value: a #xvariant_type_t
 *
 * Constructs the type corresponding to a dictionary entry with a key
 * of type @key and a value of type @value.
 *
 * It is appropriate to call xvariant_type_free() on the return value.
 *
 * Returns: (transfer full): a new dictionary entry #xvariant_type_t
 *
 * Since 2.24
 **/
xvariant_type_t *
xvariant_type_new_dict_entry (const xvariant_type_t *key,
                               const xvariant_type_t *value)
{
  xsize_t keysize, valsize;
  xchar_t *new;

  g_return_val_if_fail (xvariant_type_check (key), NULL);
  g_return_val_if_fail (xvariant_type_check (value), NULL);

  keysize = xvariant_type_get_string_length (key);
  valsize = xvariant_type_get_string_length (value);

  new = g_malloc (1 + keysize + valsize + 1);

  new[0] = '{';
  memcpy (new + 1, key, keysize);
  memcpy (new + 1 + keysize, value, valsize);
  new[1 + keysize + valsize] = '}';

  return (xvariant_type_t *) new;
}

/* private */
const xvariant_type_t *
xvariant_type_checked_ (const xchar_t *type_string)
{
  g_return_val_if_fail (xvariant_type_string_is_valid (type_string), NULL);
  return (const xvariant_type_t *) type_string;
}
