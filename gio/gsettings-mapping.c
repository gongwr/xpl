/*
 * Copyright © 2010 Novell, Inc.
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
 * Author: Vincent Untz <vuntz@gnome.org>
 */

#include "config.h"

#include "gsettings-mapping.h"

static xvariant_t *
g_settings_set_mapping_int (const xvalue_t       *value,
                            const xvariant_type_t *expected_type)
{
  xvariant_t *variant = NULL;
  sint64_t l;

  if (G_VALUE_HOLDS_INT (value))
    l = xvalue_get_int (value);
  else if (G_VALUE_HOLDS_INT64 (value))
    l = xvalue_get_int64 (value);
  else
    return NULL;

  if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT16))
    {
      if (G_MININT16 <= l && l <= G_MAXINT16)
        variant = xvariant_new_int16 ((gint16) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT16))
    {
      if (0 <= l && l <= G_MAXUINT16)
        variant = xvariant_new_uint16 ((xuint16_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT32))
    {
      if (G_MININT32 <= l && l <= G_MAXINT32)
        variant = xvariant_new_int32 ((xint_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT32))
    {
      if (0 <= l && l <= G_MAXUINT32)
        variant = xvariant_new_uint32 ((xuint_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT64))
    {
      if (G_MININT64 <= l && l <= G_MAXINT64)
        variant = xvariant_new_int64 ((sint64_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT64))
    {
      if (0 <= l && (xuint64_t) l <= G_MAXUINT64)
        variant = xvariant_new_uint64 ((xuint64_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_HANDLE))
    {
      if (0 <= l && l <= G_MAXUINT32)
        variant = xvariant_new_handle ((xuint_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_DOUBLE))
    variant = xvariant_new_double ((xdouble_t) l);

  return variant;
}

static xvariant_t *
g_settings_set_mapping_float (const xvalue_t       *value,
                              const xvariant_type_t *expected_type)
{
  xvariant_t *variant = NULL;
  xdouble_t d;
  sint64_t l;

  if (G_VALUE_HOLDS_DOUBLE (value))
    d = xvalue_get_double (value);
  else
    return NULL;

  l = (sint64_t) d;
  if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT16))
    {
      if (G_MININT16 <= l && l <= G_MAXINT16)
        variant = xvariant_new_int16 ((gint16) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT16))
    {
      if (0 <= l && l <= G_MAXUINT16)
        variant = xvariant_new_uint16 ((xuint16_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT32))
    {
      if (G_MININT32 <= l && l <= G_MAXINT32)
        variant = xvariant_new_int32 ((xint_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT32))
    {
      if (0 <= l && l <= G_MAXUINT32)
        variant = xvariant_new_uint32 ((xuint_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT64))
    {
      if (G_MININT64 <= l && l <= G_MAXINT64)
        variant = xvariant_new_int64 ((sint64_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT64))
    {
      if (0 <= l && (xuint64_t) l <= G_MAXUINT64)
        variant = xvariant_new_uint64 ((xuint64_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_HANDLE))
    {
      if (0 <= l && l <= G_MAXUINT32)
        variant = xvariant_new_handle ((xuint_t) l);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_DOUBLE))
    variant = xvariant_new_double ((xdouble_t) d);

  return variant;
}
static xvariant_t *
g_settings_set_mapping_unsigned_int (const xvalue_t       *value,
                                     const xvariant_type_t *expected_type)
{
  xvariant_t *variant = NULL;
  xuint64_t u;

  if (G_VALUE_HOLDS_UINT (value))
    u = xvalue_get_uint (value);
  else if (G_VALUE_HOLDS_UINT64 (value))
    u = xvalue_get_uint64 (value);
  else
    return NULL;

  if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT16))
    {
      if (u <= G_MAXINT16)
        variant = xvariant_new_int16 ((gint16) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT16))
    {
      if (u <= G_MAXUINT16)
        variant = xvariant_new_uint16 ((xuint16_t) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT32))
    {
      if (u <= G_MAXINT32)
        variant = xvariant_new_int32 ((xint_t) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT32))
    {
      if (u <= G_MAXUINT32)
        variant = xvariant_new_uint32 ((xuint_t) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_INT64))
    {
      if (u <= G_MAXINT64)
        variant = xvariant_new_int64 ((sint64_t) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_UINT64))
    {
      if (u <= G_MAXUINT64)
        variant = xvariant_new_uint64 ((xuint64_t) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_HANDLE))
    {
      if (u <= G_MAXUINT32)
        variant = xvariant_new_handle ((xuint_t) u);
    }
  else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_DOUBLE))
    variant = xvariant_new_double ((xdouble_t) u);

  return variant;
}

static xboolean_t
g_settings_get_mapping_int (xvalue_t   *value,
                            xvariant_t *variant)
{
  const xvariant_type_t *type;
  sint64_t l;

  type = xvariant_get_type (variant);

  if (xvariant_type_equal (type, G_VARIANT_TYPE_INT16))
    l = xvariant_get_int16 (variant);
  else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT32))
    l = xvariant_get_int32 (variant);
  else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT64))
    l = xvariant_get_int64 (variant);
  else if (xvariant_type_equal (type, G_VARIANT_TYPE_HANDLE))
    l = xvariant_get_handle (variant);
  else
    return FALSE;

  if (G_VALUE_HOLDS_INT (value))
    {
      xvalue_set_int (value, l);
      return (G_MININT32 <= l && l <= G_MAXINT32);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      xvalue_set_uint (value, l);
      return (0 <= l && l <= G_MAXUINT32);
    }
  else if (G_VALUE_HOLDS_INT64 (value))
    {
      xvalue_set_int64 (value, l);
      return (G_MININT64 <= l && l <= G_MAXINT64);
    }
  else if (G_VALUE_HOLDS_UINT64 (value))
    {
      xvalue_set_uint64 (value, l);
      return (0 <= l && (xuint64_t) l <= G_MAXUINT64);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      xvalue_set_double (value, l);
      return TRUE;
    }

  return FALSE;
}

static xboolean_t
g_settings_get_mapping_float (xvalue_t   *value,
                              xvariant_t *variant)
{
  const xvariant_type_t *type;
  xdouble_t d;
  sint64_t l;

  type = xvariant_get_type (variant);

  if (xvariant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
    d = xvariant_get_double (variant);
  else
    return FALSE;

  l = (sint64_t)d;
  if (G_VALUE_HOLDS_INT (value))
    {
      xvalue_set_int (value, l);
      return (G_MININT32 <= l && l <= G_MAXINT32);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      xvalue_set_uint (value, l);
      return (0 <= l && l <= G_MAXUINT32);
    }
  else if (G_VALUE_HOLDS_INT64 (value))
    {
      xvalue_set_int64 (value, l);
      return (G_MININT64 <= l && l <= G_MAXINT64);
    }
  else if (G_VALUE_HOLDS_UINT64 (value))
    {
      xvalue_set_uint64 (value, l);
      return (0 <= l && (xuint64_t) l <= G_MAXUINT64);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      xvalue_set_double (value, d);
      return TRUE;
    }

  return FALSE;
}
static xboolean_t
g_settings_get_mapping_unsigned_int (xvalue_t   *value,
                                     xvariant_t *variant)
{
  const xvariant_type_t *type;
  xuint64_t u;

  type = xvariant_get_type (variant);

  if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT16))
    u = xvariant_get_uint16 (variant);
  else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT32))
    u = xvariant_get_uint32 (variant);
  else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT64))
    u = xvariant_get_uint64 (variant);
  else
    return FALSE;

  if (G_VALUE_HOLDS_INT (value))
    {
      xvalue_set_int (value, u);
      return (u <= G_MAXINT32);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      xvalue_set_uint (value, u);
      return (u <= G_MAXUINT32);
    }
  else if (G_VALUE_HOLDS_INT64 (value))
    {
      xvalue_set_int64 (value, u);
      return (u <= G_MAXINT64);
    }
  else if (G_VALUE_HOLDS_UINT64 (value))
    {
      xvalue_set_uint64 (value, u);
      return (u <= G_MAXUINT64);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      xvalue_set_double (value, u);
      return TRUE;
    }

  return FALSE;
}

xvariant_t *
g_settings_set_mapping (const xvalue_t       *value,
                        const xvariant_type_t *expected_type,
                        xpointer_t            user_data)
{
  xchar_t *type_string;

  if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_BOOLEAN))
        return xvariant_new_boolean (xvalue_get_boolean (value));
    }

  else if (G_VALUE_HOLDS_CHAR (value)  ||
           G_VALUE_HOLDS_UCHAR (value))
    {
      if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_BYTE))
        {
          if (G_VALUE_HOLDS_CHAR (value))
            return xvariant_new_byte (xvalue_get_schar (value));
          else
            return xvariant_new_byte (xvalue_get_uchar (value));
        }
    }

  else if (G_VALUE_HOLDS_INT (value)   ||
           G_VALUE_HOLDS_INT64 (value))
    return g_settings_set_mapping_int (value, expected_type);

  else if (G_VALUE_HOLDS_DOUBLE (value))
    return g_settings_set_mapping_float (value, expected_type);

  else if (G_VALUE_HOLDS_UINT (value)  ||
           G_VALUE_HOLDS_UINT64 (value))
    return g_settings_set_mapping_unsigned_int (value, expected_type);

  else if (G_VALUE_HOLDS_STRING (value))
    {
      if (xvalue_get_string (value) == NULL)
        return NULL;
      else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_STRING))
        return xvariant_new_string (xvalue_get_string (value));
      else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_BYTESTRING))
        return xvariant_new_bytestring (xvalue_get_string (value));
      else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_OBJECT_PATH))
        return xvariant_new_object_path (xvalue_get_string (value));
      else if (xvariant_type_equal (expected_type, G_VARIANT_TYPE_SIGNATURE))
        return xvariant_new_signature (xvalue_get_string (value));
    }

  else if (G_VALUE_HOLDS (value, XTYPE_STRV))
    {
      if (xvalue_get_boxed (value) == NULL)
        return NULL;
      return xvariant_new_strv ((const xchar_t **) xvalue_get_boxed (value),
                                 -1);
    }

  else if (G_VALUE_HOLDS_ENUM (value))
    {
      xenum_value_t *enumval;
      xenum_class_t *eclass;

      /* GParamSpecEnum holds a ref on the class so we just peek... */
      eclass = xtype_class_peek (G_VALUE_TYPE (value));
      enumval = xenum_get_value (eclass, xvalue_get_enum (value));

      if (enumval)
        return xvariant_new_string (enumval->value_nick);
      else
        return NULL;
    }

  else if (G_VALUE_HOLDS_FLAGS (value))
    {
      xvariant_builder_t builder;
      xflags_value_t *flagsval;
      xflags_class_t *fclass;
      xuint_t flags;

      fclass = xtype_class_peek (G_VALUE_TYPE (value));
      flags = xvalue_get_flags (value);

      xvariant_builder_init (&builder, G_VARIANT_TYPE ("as"));
      while (flags)
        {
          flagsval = xflags_get_first_value (fclass, flags);

          if (flagsval == NULL)
            {
              xvariant_builder_clear (&builder);
              return NULL;
            }

          xvariant_builder_add (&builder, "s", flagsval->value_nick);
          flags &= ~flagsval->value;
        }

      return xvariant_builder_end (&builder);
    }

  type_string = xvariant_type_dup_string (expected_type);
  g_critical ("No xsettings_t bind handler for type \"%s\".", type_string);
  g_free (type_string);

  return NULL;
}

xboolean_t
g_settings_get_mapping (xvalue_t   *value,
                        xvariant_t *variant,
                        xpointer_t  user_data)
{
  if (xvariant_is_of_type (variant, G_VARIANT_TYPE_BOOLEAN))
    {
      if (!G_VALUE_HOLDS_BOOLEAN (value))
        return FALSE;
      xvalue_set_boolean (value, xvariant_get_boolean (variant));
      return TRUE;
    }

  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_BYTE))
    {
      if (G_VALUE_HOLDS_UCHAR (value))
        xvalue_set_uchar (value, xvariant_get_byte (variant));
      else if (G_VALUE_HOLDS_CHAR (value))
        xvalue_set_schar (value, (gint8)xvariant_get_byte (variant));
      else
        return FALSE;
      return TRUE;
    }

  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_INT16)  ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_INT32)  ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_INT64)  ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_HANDLE))
    return g_settings_get_mapping_int (value, variant);

  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_DOUBLE))
    return g_settings_get_mapping_float (value, variant);

  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_UINT16) ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_UINT32) ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_UINT64))
    return g_settings_get_mapping_unsigned_int (value, variant);

  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_STRING)      ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_OBJECT_PATH) ||
           xvariant_is_of_type (variant, G_VARIANT_TYPE_SIGNATURE))
    {
      if (G_VALUE_HOLDS_STRING (value))
        {
          xvalue_set_string (value, xvariant_get_string (variant, NULL));
          return TRUE;
        }

      else if (G_VALUE_HOLDS_ENUM (value))
        {
          xenum_class_t *eclass;
          xenum_value_t *evalue;
          const xchar_t *nick;

          /* GParamSpecEnum holds a ref on the class so we just peek... */
          eclass = xtype_class_peek (G_VALUE_TYPE (value));
          nick = xvariant_get_string (variant, NULL);
          evalue = xenum_get_value_by_nick (eclass, nick);

          if (evalue)
            {
             xvalue_set_enum (value, evalue->value);
             return TRUE;
            }

          g_warning ("Unable to look up enum nick ‘%s’ via xtype_t", nick);
          return FALSE;
        }
    }
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE ("as")))
    {
      if (G_VALUE_HOLDS (value, XTYPE_STRV))
        {
          xvalue_take_boxed (value, xvariant_dup_strv (variant, NULL));
          return TRUE;
        }

      else if (G_VALUE_HOLDS_FLAGS (value))
        {
          xflags_class_t *fclass;
          xflags_value_t *fvalue;
          const xchar_t *nick;
          xvariant_iter_t iter;
          xuint_t flags = 0;

          fclass = xtype_class_peek (G_VALUE_TYPE (value));

          xvariant_iter_init (&iter, variant);
          while (xvariant_iter_next (&iter, "&s", &nick))
            {
              fvalue = xflags_get_value_by_nick (fclass, nick);

              if (fvalue)
                flags |= fvalue->value;

              else
                {
                  g_warning ("Unable to lookup flags nick '%s' via xtype_t",
                             nick);
                  return FALSE;
                }
            }

          xvalue_set_flags (value, flags);
          return TRUE;
        }
    }
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_BYTESTRING))
    {
      xvalue_set_string (value, xvariant_get_bytestring (variant));
      return TRUE;
    }

  g_critical ("No xsettings_t bind handler for type \"%s\".",
              xvariant_get_type_string (variant));

  return FALSE;
}

xboolean_t
g_settings_mapping_is_compatible (xtype_t               gvalue_type,
                                  const xvariant_type_t *variant_type)
{
  xboolean_t ok = FALSE;

  if (gvalue_type == XTYPE_BOOLEAN)
    ok = xvariant_type_equal (variant_type, G_VARIANT_TYPE_BOOLEAN);
  else if (gvalue_type == XTYPE_CHAR  ||
           gvalue_type == XTYPE_UCHAR)
    ok = xvariant_type_equal (variant_type, G_VARIANT_TYPE_BYTE);
  else if (gvalue_type == XTYPE_INT    ||
           gvalue_type == XTYPE_UINT   ||
           gvalue_type == XTYPE_INT64  ||
           gvalue_type == XTYPE_UINT64 ||
           gvalue_type == XTYPE_DOUBLE)
    ok = (xvariant_type_equal (variant_type, G_VARIANT_TYPE_INT16)  ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_UINT16) ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_INT32)  ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_UINT32) ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_INT64)  ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_UINT64) ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_HANDLE) ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_DOUBLE));
  else if (gvalue_type == XTYPE_STRING)
    ok = (xvariant_type_equal (variant_type, G_VARIANT_TYPE_STRING)      ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE ("ay")) ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_OBJECT_PATH) ||
          xvariant_type_equal (variant_type, G_VARIANT_TYPE_SIGNATURE));
  else if (gvalue_type == XTYPE_STRV)
    ok = xvariant_type_equal (variant_type, G_VARIANT_TYPE ("as"));
  else if (XTYPE_IS_ENUM (gvalue_type))
    ok = xvariant_type_equal (variant_type, G_VARIANT_TYPE_STRING);
  else if (XTYPE_IS_FLAGS (gvalue_type))
    ok = xvariant_type_equal (variant_type, G_VARIANT_TYPE ("as"));

  return ok;
}
