/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gdbusutils.h"

#include "glibintl.h"

/**
 * SECTION:gdbusutils
 * @title: D-Bus Utilities
 * @short_description: Various utilities related to D-Bus
 * @include: gio/gio.h
 *
 * Various utility routines related to D-Bus.
 */

static xboolean_t
is_valid_bus_name_character (xint_t c,
                             xboolean_t allow_hyphen)
{
  return
    (c >= '0' && c <= '9') ||
    (c >= 'A' && c <= 'Z') ||
    (c >= 'a' && c <= 'z') ||
    (c == '_') ||
    (allow_hyphen && c == '-');
}

static xboolean_t
is_valid_initial_bus_name_character (xint_t c,
                                     xboolean_t allow_initial_digit,
                                     xboolean_t allow_hyphen)
{
  if (allow_initial_digit)
    return is_valid_bus_name_character (c, allow_hyphen);
  else
    return
      (c >= 'A' && c <= 'Z') ||
      (c >= 'a' && c <= 'z') ||
      (c == '_') ||
      (allow_hyphen && c == '-');
}

static xboolean_t
is_valid_name (const xchar_t *start,
               xuint_t len,
               xboolean_t allow_initial_digit,
               xboolean_t allow_hyphen)
{
  xboolean_t ret;
  const xchar_t *s;
  const xchar_t *end;
  xboolean_t has_dot;

  ret = FALSE;

  if (len == 0)
    goto out;

  s = start;
  end = s + len;
  has_dot = FALSE;
  while (s != end)
    {
      if (*s == '.')
        {
          s += 1;
          if (G_UNLIKELY (!is_valid_initial_bus_name_character (*s, allow_initial_digit, allow_hyphen)))
            goto out;
          has_dot = TRUE;
        }
      else if (G_UNLIKELY (!is_valid_bus_name_character (*s, allow_hyphen)))
        {
          goto out;
        }
      s += 1;
    }

  if (G_UNLIKELY (!has_dot))
    goto out;

  ret = TRUE;

 out:
  return ret;
}

/**
 * g_dbus_is_name:
 * @string: The string to check.
 *
 * Checks if @string is a valid D-Bus bus name (either unique or well-known).
 *
 * Returns: %TRUE if valid, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_name (const xchar_t *string)
{
  xuint_t len;
  xboolean_t ret;
  const xchar_t *s;

  g_return_val_if_fail (string != NULL, FALSE);

  ret = FALSE;

  len = strlen (string);
  if (G_UNLIKELY (len == 0 || len > 255))
    goto out;

  s = string;
  if (*s == ':')
    {
      /* handle unique name */
      if (!is_valid_name (s + 1, len - 1, TRUE, TRUE))
        goto out;
      ret = TRUE;
      goto out;
    }
  else if (G_UNLIKELY (*s == '.'))
    {
      /* can't start with a . */
      goto out;
    }
  else if (G_UNLIKELY (!is_valid_initial_bus_name_character (*s, FALSE, TRUE)))
    goto out;

  ret = is_valid_name (s + 1, len - 1, FALSE, TRUE);

 out:
  return ret;
}

/**
 * g_dbus_is_unique_name:
 * @string: The string to check.
 *
 * Checks if @string is a valid D-Bus unique bus name.
 *
 * Returns: %TRUE if valid, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_unique_name (const xchar_t *string)
{
  xboolean_t ret;
  xuint_t len;

  g_return_val_if_fail (string != NULL, FALSE);

  ret = FALSE;

  len = strlen (string);
  if (G_UNLIKELY (len == 0 || len > 255))
    goto out;

  if (G_UNLIKELY (*string != ':'))
    goto out;

  if (G_UNLIKELY (!is_valid_name (string + 1, len - 1, TRUE, TRUE)))
    goto out;

  ret = TRUE;

 out:
  return ret;
}

/**
 * g_dbus_is_member_name:
 * @string: The string to check.
 *
 * Checks if @string is a valid D-Bus member (e.g. signal or method) name.
 *
 * Returns: %TRUE if valid, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_member_name (const xchar_t *string)
{
  xboolean_t ret;
  xuint_t n;

  ret = FALSE;
  if (G_UNLIKELY (string == NULL))
    goto out;

  if (G_UNLIKELY (!is_valid_initial_bus_name_character (string[0], FALSE, FALSE)))
    goto out;

  for (n = 1; string[n] != '\0'; n++)
    {
      if (G_UNLIKELY (!is_valid_bus_name_character (string[n], FALSE)))
        {
          goto out;
        }
    }

  ret = TRUE;

 out:
  return ret;
}

/**
 * g_dbus_is_interface_name:
 * @string: The string to check.
 *
 * Checks if @string is a valid D-Bus interface name.
 *
 * Returns: %TRUE if valid, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_interface_name (const xchar_t *string)
{
  xuint_t len;
  xboolean_t ret;
  const xchar_t *s;

  g_return_val_if_fail (string != NULL, FALSE);

  ret = FALSE;

  len = strlen (string);
  if (G_UNLIKELY (len == 0 || len > 255))
    goto out;

  s = string;
  if (G_UNLIKELY (*s == '.'))
    {
      /* can't start with a . */
      goto out;
    }
  else if (G_UNLIKELY (!is_valid_initial_bus_name_character (*s, FALSE, FALSE)))
    goto out;

  ret = is_valid_name (s + 1, len - 1, FALSE, FALSE);

 out:
  return ret;
}

/**
 * g_dbus_is_error_name:
 * @string: The string to check.
 *
 * Check whether @string is a valid D-Bus error name.
 *
 * This function returns the same result as g_dbus_is_interface_name(),
 * because D-Bus error names are defined to have exactly the
 * same syntax as interface names.
 *
 * Returns: %TRUE if valid, %FALSE otherwise.
 *
 * Since: 2.70
 */
xboolean_t
g_dbus_is_error_name (const xchar_t *string)
{
  /* Error names are the same syntax as interface names.
   * See https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names-error */
  return g_dbus_is_interface_name (string);
}

/* ---------------------------------------------------------------------------------------------------- */

/* TODO: maybe move to glib? if so, it should conform to http://en.wikipedia.org/wiki/Guid and/or
 *       http://tools.ietf.org/html/rfc4122 - specifically it should have hyphens then.
 */

/**
 * g_dbus_generate_guid:
 *
 * Generate a D-Bus GUID that can be used with
 * e.g. xdbus_connection_new().
 *
 * See the
 * [D-Bus specification](https://dbus.freedesktop.org/doc/dbus-specification.html#uuids)
 * regarding what strings are valid D-Bus GUIDs. The specification refers to
 * these as ‘UUIDs’ whereas GLib (for historical reasons) refers to them as
 * ‘GUIDs’. The terms are interchangeable.
 *
 * Note that D-Bus GUIDs do not follow
 * [RFC 4122](https://datatracker.ietf.org/doc/html/rfc4122).
 *
 * Returns: A valid D-Bus GUID. Free with g_free().
 *
 * Since: 2.26
 */
xchar_t *
g_dbus_generate_guid (void)
{
  xstring_t *s;
  xuint32_t r1;
  xuint32_t r2;
  xuint32_t r3;
  sint64_t now_us;

  s = xstring_new (NULL);

  r1 = g_random_int ();
  r2 = g_random_int ();
  r3 = g_random_int ();
  now_us = g_get_real_time ();

  xstring_append_printf (s, "%08x", r1);
  xstring_append_printf (s, "%08x", r2);
  xstring_append_printf (s, "%08x", r3);
  xstring_append_printf (s, "%08x", (xuint32_t) (now_us / G_USEC_PER_SEC));

  return xstring_free (s, FALSE);
}

/**
 * g_dbus_is_guid:
 * @string: The string to check.
 *
 * Checks if @string is a D-Bus GUID.
 *
 * See the documentation for g_dbus_generate_guid() for more information about
 * the format of a GUID.
 *
 * Returns: %TRUE if @string is a GUID, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_is_guid (const xchar_t *string)
{
  xboolean_t ret;
  xuint_t n;

  g_return_val_if_fail (string != NULL, FALSE);

  ret = FALSE;

  for (n = 0; n < 32; n++)
    {
      if (!g_ascii_isxdigit (string[n]))
        goto out;
    }
  if (string[32] != '\0')
    goto out;

  ret = TRUE;

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_gvariant_to_gvalue:
 * @value: A #xvariant_t.
 * @out_gvalue: (out): Return location pointing to a zero-filled (uninitialized) #xvalue_t.
 *
 * Converts a #xvariant_t to a #xvalue_t. If @value is floating, it is consumed.
 *
 * The rules specified in the g_dbus_gvalue_to_gvariant() function are
 * used - this function is essentially its reverse form. So, a #xvariant_t
 * containing any basic or string array type will be converted to a #xvalue_t
 * containing a basic value or string array. Any other #xvariant_t (handle,
 * variant, tuple, dict entry) will be converted to a #xvalue_t containing that
 * #xvariant_t.
 *
 * The conversion never fails - a valid #xvalue_t is always returned in
 * @out_gvalue.
 *
 * Since: 2.30
 */
void
g_dbus_gvariant_to_gvalue (xvariant_t  *value,
                           xvalue_t    *out_gvalue)
{
  const xvariant_type_t *type;
  xchar_t **array;

  g_return_if_fail (value != NULL);
  g_return_if_fail (out_gvalue != NULL);

  memset (out_gvalue, '\0', sizeof (xvalue_t));

  switch (xvariant_classify (value))
    {
    case XVARIANT_CLASS_BOOLEAN:
      xvalue_init (out_gvalue, XTYPE_BOOLEAN);
      xvalue_set_boolean (out_gvalue, xvariant_get_boolean (value));
      break;

    case XVARIANT_CLASS_BYTE:
      xvalue_init (out_gvalue, XTYPE_UCHAR);
      xvalue_set_uchar (out_gvalue, xvariant_get_byte (value));
      break;

    case XVARIANT_CLASS_INT16:
      xvalue_init (out_gvalue, XTYPE_INT);
      xvalue_set_int (out_gvalue, xvariant_get_int16 (value));
      break;

    case XVARIANT_CLASS_UINT16:
      xvalue_init (out_gvalue, XTYPE_UINT);
      xvalue_set_uint (out_gvalue, xvariant_get_uint16 (value));
      break;

    case XVARIANT_CLASS_INT32:
      xvalue_init (out_gvalue, XTYPE_INT);
      xvalue_set_int (out_gvalue, xvariant_get_int32 (value));
      break;

    case XVARIANT_CLASS_UINT32:
      xvalue_init (out_gvalue, XTYPE_UINT);
      xvalue_set_uint (out_gvalue, xvariant_get_uint32 (value));
      break;

    case XVARIANT_CLASS_INT64:
      xvalue_init (out_gvalue, XTYPE_INT64);
      xvalue_set_int64 (out_gvalue, xvariant_get_int64 (value));
      break;

    case XVARIANT_CLASS_UINT64:
      xvalue_init (out_gvalue, XTYPE_UINT64);
      xvalue_set_uint64 (out_gvalue, xvariant_get_uint64 (value));
      break;

    case XVARIANT_CLASS_DOUBLE:
      xvalue_init (out_gvalue, XTYPE_DOUBLE);
      xvalue_set_double (out_gvalue, xvariant_get_double (value));
      break;

    case XVARIANT_CLASS_STRING:
      xvalue_init (out_gvalue, XTYPE_STRING);
      xvalue_set_string (out_gvalue, xvariant_get_string (value, NULL));
      break;

    case XVARIANT_CLASS_OBJECT_PATH:
      xvalue_init (out_gvalue, XTYPE_STRING);
      xvalue_set_string (out_gvalue, xvariant_get_string (value, NULL));
      break;

    case XVARIANT_CLASS_SIGNATURE:
      xvalue_init (out_gvalue, XTYPE_STRING);
      xvalue_set_string (out_gvalue, xvariant_get_string (value, NULL));
      break;

    case XVARIANT_CLASS_ARRAY:
      type = xvariant_get_type (value);
      switch (xvariant_type_peek_string (type)[1])
        {
        case XVARIANT_CLASS_BYTE:
          xvalue_init (out_gvalue, XTYPE_STRING);
          xvalue_set_string (out_gvalue, xvariant_get_bytestring (value));
          break;

        case XVARIANT_CLASS_STRING:
          xvalue_init (out_gvalue, XTYPE_STRV);
          array = xvariant_dup_strv (value, NULL);
          xvalue_take_boxed (out_gvalue, array);
          break;

        case XVARIANT_CLASS_OBJECT_PATH:
          xvalue_init (out_gvalue, XTYPE_STRV);
          array = xvariant_dup_objv (value, NULL);
          xvalue_take_boxed (out_gvalue, array);
          break;

        case XVARIANT_CLASS_ARRAY:
          switch (xvariant_type_peek_string (type)[2])
            {
            case XVARIANT_CLASS_BYTE:
              xvalue_init (out_gvalue, XTYPE_STRV);
              array = xvariant_dup_bytestring_array (value, NULL);
              xvalue_take_boxed (out_gvalue, array);
              break;

            default:
              xvalue_init (out_gvalue, XTYPE_VARIANT);
              xvalue_set_variant (out_gvalue, value);
              break;
            }
          break;

        default:
          xvalue_init (out_gvalue, XTYPE_VARIANT);
          xvalue_set_variant (out_gvalue, value);
          break;
        }
      break;

    case XVARIANT_CLASS_HANDLE:
    case XVARIANT_CLASS_VARIANT:
    case XVARIANT_CLASS_MAYBE:
    case XVARIANT_CLASS_TUPLE:
    case XVARIANT_CLASS_DICT_ENTRY:
      xvalue_init (out_gvalue, XTYPE_VARIANT);
      xvalue_set_variant (out_gvalue, value);
      break;
    }
}


/**
 * g_dbus_gvalue_to_gvariant:
 * @gvalue: A #xvalue_t to convert to a #xvariant_t
 * @type: A #xvariant_type_t
 *
 * Converts a #xvalue_t to a #xvariant_t of the type indicated by the @type
 * parameter.
 *
 * The conversion is using the following rules:
 *
 * - `XTYPE_STRING`: 's', 'o', 'g' or 'ay'
 * - `XTYPE_STRV`: 'as', 'ao' or 'aay'
 * - `XTYPE_BOOLEAN`: 'b'
 * - `XTYPE_UCHAR`: 'y'
 * - `XTYPE_INT`: 'i', 'n'
 * - `XTYPE_UINT`: 'u', 'q'
 * - `XTYPE_INT64`: 'x'
 * - `XTYPE_UINT64`: 't'
 * - `XTYPE_DOUBLE`: 'd'
 * - `XTYPE_VARIANT`: Any #xvariant_type_t
 *
 * This can fail if e.g. @gvalue is of type %XTYPE_STRING and @type
 * is 'i', i.e. %G_VARIANT_TYPE_INT32. It will also fail for any #xtype_t
 * (including e.g. %XTYPE_OBJECT and %XTYPE_BOXED derived-types) not
 * in the table above.
 *
 * Note that if @gvalue is of type %XTYPE_VARIANT and its value is
 * %NULL, the empty #xvariant_t instance (never %NULL) for @type is
 * returned (e.g. 0 for scalar types, the empty string for string types,
 * '/' for object path types, the empty array for any array type and so on).
 *
 * See the g_dbus_gvariant_to_gvalue() function for how to convert a
 * #xvariant_t to a #xvalue_t.
 *
 * Returns: (transfer full): A #xvariant_t (never floating) of
 *     #xvariant_type_t @type holding the data from @gvalue or an empty #xvariant_t
 *     in case of failure. Free with xvariant_unref().
 *
 * Since: 2.30
 */
xvariant_t *
g_dbus_gvalue_to_gvariant (const xvalue_t       *gvalue,
                           const xvariant_type_t *type)
{
  xvariant_t *ret;
  const xchar_t *s;
  const xchar_t * const *as;
  const xchar_t *empty_strv[1] = {NULL};

  g_return_val_if_fail (gvalue != NULL, NULL);
  g_return_val_if_fail (type != NULL, NULL);

  ret = NULL;

  /* @type can easily be e.g. "s" with the xvalue_t holding a xvariant_t - for example this
   * can happen when using the org.gtk.GDBus.C.ForceGVariant annotation with the
   * gdbus-codegen(1) tool.
   */
  if (G_VALUE_TYPE (gvalue) == XTYPE_VARIANT)
    {
      ret = xvalue_dup_variant (gvalue);
    }
  else
    {
      switch (xvariant_type_peek_string (type)[0])
        {
        case XVARIANT_CLASS_BOOLEAN:
          ret = xvariant_ref_sink (xvariant_new_boolean (xvalue_get_boolean (gvalue)));
          break;

        case XVARIANT_CLASS_BYTE:
          ret = xvariant_ref_sink (xvariant_new_byte (xvalue_get_uchar (gvalue)));
          break;

        case XVARIANT_CLASS_INT16:
          ret = xvariant_ref_sink (xvariant_new_int16 (xvalue_get_int (gvalue)));
          break;

        case XVARIANT_CLASS_UINT16:
          ret = xvariant_ref_sink (xvariant_new_uint16 (xvalue_get_uint (gvalue)));
          break;

        case XVARIANT_CLASS_INT32:
          ret = xvariant_ref_sink (xvariant_new_int32 (xvalue_get_int (gvalue)));
          break;

        case XVARIANT_CLASS_UINT32:
          ret = xvariant_ref_sink (xvariant_new_uint32 (xvalue_get_uint (gvalue)));
          break;

        case XVARIANT_CLASS_INT64:
          ret = xvariant_ref_sink (xvariant_new_int64 (xvalue_get_int64 (gvalue)));
          break;

        case XVARIANT_CLASS_UINT64:
          ret = xvariant_ref_sink (xvariant_new_uint64 (xvalue_get_uint64 (gvalue)));
          break;

        case XVARIANT_CLASS_DOUBLE:
          ret = xvariant_ref_sink (xvariant_new_double (xvalue_get_double (gvalue)));
          break;

        case XVARIANT_CLASS_STRING:
          s = xvalue_get_string (gvalue);
          if (s == NULL)
            s = "";
          ret = xvariant_ref_sink (xvariant_new_string (s));
          break;

        case XVARIANT_CLASS_OBJECT_PATH:
          s = xvalue_get_string (gvalue);
          if (s == NULL)
            s = "/";
          ret = xvariant_ref_sink (xvariant_new_object_path (s));
          break;

        case XVARIANT_CLASS_SIGNATURE:
          s = xvalue_get_string (gvalue);
          if (s == NULL)
            s = "";
          ret = xvariant_ref_sink (xvariant_new_signature (s));
          break;

        case XVARIANT_CLASS_ARRAY:
          switch (xvariant_type_peek_string (type)[1])
            {
            case XVARIANT_CLASS_BYTE:
              s = xvalue_get_string (gvalue);
              if (s == NULL)
                s = "";
              ret = xvariant_ref_sink (xvariant_new_bytestring (s));
              break;

            case XVARIANT_CLASS_STRING:
              as = xvalue_get_boxed (gvalue);
              if (as == NULL)
                as = empty_strv;
              ret = xvariant_ref_sink (xvariant_new_strv (as, -1));
              break;

            case XVARIANT_CLASS_OBJECT_PATH:
              as = xvalue_get_boxed (gvalue);
              if (as == NULL)
                as = empty_strv;
              ret = xvariant_ref_sink (xvariant_new_objv (as, -1));
              break;

            case XVARIANT_CLASS_ARRAY:
              switch (xvariant_type_peek_string (type)[2])
                {
                case XVARIANT_CLASS_BYTE:
                  as = xvalue_get_boxed (gvalue);
                  if (as == NULL)
                    as = empty_strv;
                  ret = xvariant_ref_sink (xvariant_new_bytestring_array (as, -1));
                  break;

                default:
                  ret = xvalue_dup_variant (gvalue);
                  break;
                }
              break;

            default:
              ret = xvalue_dup_variant (gvalue);
              break;
            }
          break;

        case XVARIANT_CLASS_HANDLE:
        case XVARIANT_CLASS_VARIANT:
        case XVARIANT_CLASS_MAYBE:
        case XVARIANT_CLASS_TUPLE:
        case XVARIANT_CLASS_DICT_ENTRY:
          ret = xvalue_dup_variant (gvalue);
          break;
        }
    }

  /* Could be that the xvalue_t is holding a NULL xvariant_t - in that case,
   * we return an "empty" xvariant_t instead of a NULL xvariant_t
   */
  if (ret == NULL)
    {
      xvariant_t *untrusted_empty;
      untrusted_empty = xvariant_new_from_data (type, NULL, 0, FALSE, NULL, NULL);
      ret = xvariant_take_ref (xvariant_get_normal_form (untrusted_empty));
      xvariant_unref (untrusted_empty);
    }

  g_assert (!xvariant_is_floating (ret));

  return ret;
}

/**
 * g_dbus_escape_object_path_bytestring:
 * @bytes: (array zero-terminated=1) (element-type xuint8_t): the string of bytes to escape
 *
 * Escapes @bytes for use in a D-Bus object path component.
 * @bytes is an array of zero or more nonzero bytes in an
 * unspecified encoding, followed by a single zero byte.
 *
 * The escaping method consists of replacing all non-alphanumeric
 * characters (see g_ascii_isalnum()) with their hexadecimal value
 * preceded by an underscore (`_`). For example:
 * `foo.bar.baz` will become `foo_2ebar_2ebaz`.
 *
 * This method is appropriate to use when the input is nearly
 * a valid object path component but is not when your input
 * is far from being a valid object path component.
 * Other escaping algorithms are also valid to use with
 * D-Bus object paths.
 *
 * This can be reversed with g_dbus_unescape_object_path().
 *
 * Returns: an escaped version of @bytes. Free with g_free().
 *
 * Since: 2.68
 *
 */
xchar_t *
g_dbus_escape_object_path_bytestring (const xuint8_t *bytes)
{
  xstring_t *escaped;
  const xuint8_t *p;

  g_return_val_if_fail (bytes != NULL, NULL);

  if (*bytes == '\0')
    return xstrdup ("_");

  escaped = xstring_new (NULL);
  for (p = bytes; *p; p++)
    {
      if (g_ascii_isalnum (*p))
        xstring_append_c (escaped, *p);
      else
        xstring_append_printf (escaped, "_%02x", *p);
    }

  return xstring_free (escaped, FALSE);
}

/**
 * g_dbus_escape_object_path:
 * @s: the string to escape
 *
 * This is a language binding friendly version of g_dbus_escape_object_path_bytestring().
 *
 * Returns: an escaped version of @s. Free with g_free().
 *
 * Since: 2.68
 */
xchar_t *
g_dbus_escape_object_path (const xchar_t *s)
{
  return (xchar_t *) g_dbus_escape_object_path_bytestring ((const xuint8_t *) s);
}

/**
 * g_dbus_unescape_object_path:
 * @s: the string to unescape
 *
 * Unescapes an string that was previously escaped with
 * g_dbus_escape_object_path(). If the string is in a format that could
 * not have been returned by g_dbus_escape_object_path(), this function
 * returns %NULL.
 *
 * Encoding alphanumeric characters which do not need to be
 * encoded is not allowed (e.g `_63` is not valid, the string
 * should contain `c` instead).
 *
 * Returns: (array zero-terminated=1) (element-type xuint8_t) (nullable): an
 *   unescaped version of @s, or %NULL if @s is not a string returned
 *   from g_dbus_escape_object_path(). Free with g_free().
 *
 * Since: 2.68
 */
xuint8_t *
g_dbus_unescape_object_path (const xchar_t *s)
{
  xstring_t *unescaped;
  const xchar_t *p;

  g_return_val_if_fail (s != NULL, NULL);

  if (xstr_equal (s, "_"))
    return (xuint8_t *) xstrdup ("");

  unescaped = xstring_new (NULL);
  for (p = s; *p; p++)
    {
      xint_t hi, lo;

      if (g_ascii_isalnum (*p))
        {
          xstring_append_c (unescaped, *p);
        }
      else if (*p == '_' &&
               ((hi = g_ascii_xdigit_value (p[1])) >= 0) &&
               ((lo = g_ascii_xdigit_value (p[2])) >= 0) &&
               (hi || lo) &&                      /* \0 is not allowed */
               !g_ascii_isalnum ((hi << 4) | lo)) /* alnums must not be encoded */
        {
          xstring_append_c (unescaped, (hi << 4) | lo);
          p += 2;
        }
      else
        {
          /* the string was not encoded correctly */
          xstring_free (unescaped, TRUE);
          return NULL;
        }
    }

  return (xuint8_t *) xstring_free (unescaped, FALSE);
}
