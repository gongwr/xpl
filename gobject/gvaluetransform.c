/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2001 Red Hat, Inc.
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
 */

#include "config.h"

#include <string.h>

#include "gvalue.h"
#include "gtype-private.h"
#include "genums.h"


/* same type transforms
 */
static void
value_transform_memcpy_data0 (const xvalue_t *src_value,
                              xvalue_t       *dest_value)
{
  memcpy (&dest_value->data[0], &src_value->data[0], sizeof (src_value->data[0]));
}
#define value_transform_int_int         value_transform_memcpy_data0
#define value_transform_uint_uint       value_transform_memcpy_data0
#define value_transform_long_long       value_transform_memcpy_data0
#define value_transform_ulong_ulong     value_transform_memcpy_data0
#define value_transform_int64_int64     value_transform_memcpy_data0
#define value_transform_uint64_uint64   value_transform_memcpy_data0
#define value_transform_float_float     value_transform_memcpy_data0
#define value_transform_double_double   value_transform_memcpy_data0


/* numeric casts
 */
#define DEFINE_CAST(func_name, from_member, ctype, to_member)               \
static void                                                                 \
value_transform_##func_name (const xvalue_t *src_value,                       \
                             xvalue_t       *dest_value)                      \
{                                                                           \
  ctype c_value = src_value->data[0].from_member;                           \
  dest_value->data[0].to_member = c_value;                                  \
} extern void glib_dummy_decl (void)
DEFINE_CAST (int_s8,            v_int,    gint8,   v_int);
DEFINE_CAST (int_u8,            v_int,    xuint8_t,  v_uint);
DEFINE_CAST (int_uint,          v_int,    xuint_t,   v_uint);
DEFINE_CAST (int_long,          v_int,    xlong_t,   v_long);
DEFINE_CAST (int_ulong,         v_int,    gulong,  v_ulong);
DEFINE_CAST (int_int64,         v_int,    gint64,  v_int64);
DEFINE_CAST (int_uint64,        v_int,    xuint64_t, v_uint64);
DEFINE_CAST (int_float,         v_int,    gfloat,  v_float);
DEFINE_CAST (int_double,        v_int,    xdouble_t, v_double);
DEFINE_CAST (uint_s8,           v_uint,   gint8,   v_int);
DEFINE_CAST (uint_u8,           v_uint,   xuint8_t,  v_uint);
DEFINE_CAST (uint_int,          v_uint,   xint_t,    v_int);
DEFINE_CAST (uint_long,         v_uint,   xlong_t,   v_long);
DEFINE_CAST (uint_ulong,        v_uint,   gulong,  v_ulong);
DEFINE_CAST (uint_int64,        v_uint,   gint64,  v_int64);
DEFINE_CAST (uint_uint64,       v_uint,   xuint64_t, v_uint64);
DEFINE_CAST (uint_float,        v_uint,   gfloat,  v_float);
DEFINE_CAST (uint_double,       v_uint,   xdouble_t, v_double);
DEFINE_CAST (long_s8,           v_long,   gint8,   v_int);
DEFINE_CAST (long_u8,           v_long,   xuint8_t,  v_uint);
DEFINE_CAST (long_int,          v_long,   xint_t,    v_int);
DEFINE_CAST (long_uint,         v_long,   xuint_t,   v_uint);
DEFINE_CAST (long_ulong,        v_long,   gulong,  v_ulong);
DEFINE_CAST (long_int64,        v_long,   gint64,  v_int64);
DEFINE_CAST (long_uint64,       v_long,   xuint64_t, v_uint64);
DEFINE_CAST (long_float,        v_long,   gfloat,  v_float);
DEFINE_CAST (long_double,       v_long,   xdouble_t, v_double);
DEFINE_CAST (ulong_s8,          v_ulong,  gint8,   v_int);
DEFINE_CAST (ulong_u8,          v_ulong,  xuint8_t,  v_uint);
DEFINE_CAST (ulong_int,         v_ulong,  xint_t,    v_int);
DEFINE_CAST (ulong_uint,        v_ulong,  xuint_t,   v_uint);
DEFINE_CAST (ulong_int64,       v_ulong,  gint64,  v_int64);
DEFINE_CAST (ulong_uint64,      v_ulong,  xuint64_t, v_uint64);
DEFINE_CAST (ulong_long,        v_ulong,  xlong_t,   v_long);
DEFINE_CAST (ulong_float,       v_ulong,  gfloat,  v_float);
DEFINE_CAST (ulong_double,      v_ulong,  xdouble_t, v_double);
DEFINE_CAST (int64_s8,          v_int64,  gint8,   v_int);
DEFINE_CAST (int64_u8,          v_int64,  xuint8_t,  v_uint);
DEFINE_CAST (int64_int,         v_int64,  xint_t,    v_int);
DEFINE_CAST (int64_uint,        v_int64,  xuint_t,   v_uint);
DEFINE_CAST (int64_long,        v_int64,  xlong_t,   v_long);
DEFINE_CAST (int64_uint64,      v_int64,  xuint64_t, v_uint64);
DEFINE_CAST (int64_ulong,       v_int64,  gulong,  v_ulong);
DEFINE_CAST (int64_float,       v_int64,  gfloat,  v_float);
DEFINE_CAST (int64_double,      v_int64,  xdouble_t, v_double);
DEFINE_CAST (uint64_s8,         v_uint64, gint8,   v_int);
DEFINE_CAST (uint64_u8,         v_uint64, xuint8_t,  v_uint);
DEFINE_CAST (uint64_int,        v_uint64, xint_t,    v_int);
DEFINE_CAST (uint64_uint,       v_uint64, xuint_t,   v_uint);
DEFINE_CAST (uint64_long,       v_uint64, xlong_t,   v_long);
DEFINE_CAST (uint64_ulong,      v_uint64, gulong,  v_ulong);
DEFINE_CAST (uint64_int64,      v_uint64, gint64,  v_int64);
DEFINE_CAST (uint64_float,      v_uint64, gfloat,  v_float);
DEFINE_CAST (uint64_double,     v_uint64, xdouble_t, v_double);
DEFINE_CAST (float_s8,          v_float,  gint8,   v_int);
DEFINE_CAST (float_u8,          v_float,  xuint8_t,  v_uint);
DEFINE_CAST (float_int,         v_float,  xint_t,    v_int);
DEFINE_CAST (float_uint,        v_float,  xuint_t,   v_uint);
DEFINE_CAST (float_long,        v_float,  xlong_t,   v_long);
DEFINE_CAST (float_ulong,       v_float,  gulong,  v_ulong);
DEFINE_CAST (float_int64,       v_float,  gint64,  v_int64);
DEFINE_CAST (float_uint64,      v_float,  xuint64_t, v_uint64);
DEFINE_CAST (float_double,      v_float,  xdouble_t, v_double);
DEFINE_CAST (double_s8,         v_double, gint8,   v_int);
DEFINE_CAST (double_u8,         v_double, xuint8_t,  v_uint);
DEFINE_CAST (double_int,        v_double, xint_t,    v_int);
DEFINE_CAST (double_uint,       v_double, xuint_t,   v_uint);
DEFINE_CAST (double_long,       v_double, xlong_t,   v_long);
DEFINE_CAST (double_ulong,      v_double, gulong,  v_ulong);
DEFINE_CAST (double_int64,      v_double, gint64,  v_int64);
DEFINE_CAST (double_uint64,     v_double, xuint64_t, v_uint64);
DEFINE_CAST (double_float,      v_double, gfloat,  v_float);


/* boolean assignments
 */
#define DEFINE_BOOL_CHECK(func_name, from_member)                           \
static void                                                                 \
value_transform_##func_name (const xvalue_t *src_value,                       \
                             xvalue_t       *dest_value)                      \
{                                                                           \
  dest_value->data[0].v_int = src_value->data[0].from_member != 0;  \
} extern void glib_dummy_decl (void)
DEFINE_BOOL_CHECK (int_bool,    v_int);
DEFINE_BOOL_CHECK (uint_bool,   v_uint);
DEFINE_BOOL_CHECK (long_bool,   v_long);
DEFINE_BOOL_CHECK (ulong_bool,  v_ulong);
DEFINE_BOOL_CHECK (int64_bool,  v_int64);
DEFINE_BOOL_CHECK (uint64_bool, v_uint64);


/* string printouts
 */
#define DEFINE_SPRINTF(func_name, from_member, format)                      \
static void                                                                 \
value_transform_##func_name (const xvalue_t *src_value,                       \
                             xvalue_t       *dest_value)                      \
{                                                                           \
  dest_value->data[0].v_pointer = xstrdup_printf ((format),                \
						   src_value->data[0].from_member);             \
} extern void glib_dummy_decl (void)
DEFINE_SPRINTF (int_string,     v_int,    "%d");
DEFINE_SPRINTF (uint_string,    v_uint,   "%u");
DEFINE_SPRINTF (lonxstring,    v_long,   "%ld");
DEFINE_SPRINTF (ulonxstring,   v_ulong,  "%lu");
DEFINE_SPRINTF (int64_string,   v_int64,  "%" G_GINT64_FORMAT);
DEFINE_SPRINTF (uint64_string,  v_uint64, "%" G_GUINT64_FORMAT);
DEFINE_SPRINTF (float_string,   v_float,  "%f");
DEFINE_SPRINTF (double_string,  v_double, "%f");


/* special cases
 */
static void
value_transform_bool_string (const xvalue_t *src_value,
                             xvalue_t       *dest_value)
{
  dest_value->data[0].v_pointer = xstrdup_printf ("%s",
                                                   src_value->data[0].v_int ?
                                                   "TRUE" : "FALSE");
}
static void
value_transform_strinxstring (const xvalue_t *src_value,
                               xvalue_t       *dest_value)
{
  dest_value->data[0].v_pointer = xstrdup (src_value->data[0].v_pointer);
}
static void
value_transform_enum_string (const xvalue_t *src_value,
                             xvalue_t       *dest_value)
{
  xint_t v_enum = src_value->data[0].v_long;
  xchar_t *str = xenum_to_string (G_VALUE_TYPE (src_value), v_enum);

  dest_value->data[0].v_pointer = str;
}
static void
value_transform_flags_string (const xvalue_t *src_value,
                              xvalue_t       *dest_value)
{
  xflags_class_t *class = xtype_class_ref (G_VALUE_TYPE (src_value));
  xflags_value_t *flags_value = xflags_get_first_value (class, src_value->data[0].v_ulong);

  /* Note: this does not use xflags_to_string()
   * to keep backwards compatibility.
   */
  if (flags_value)
    {
      xstring_t *xstring = xstring_new (NULL);
      xuint_t v_flags = src_value->data[0].v_ulong;

      do
        {
          v_flags &= ~flags_value->value;

          if (xstring->str[0])
            xstring_append (xstring, " | ");
          xstring_append (xstring, flags_value->value_name);
          flags_value = xflags_get_first_value (class, v_flags);
        }
      while (flags_value && v_flags);

      if (v_flags)
        dest_value->data[0].v_pointer = xstrdup_printf ("%s | %u",
                                                         xstring->str,
                                                         v_flags);
      else
        dest_value->data[0].v_pointer = xstrdup (xstring->str);
      xstring_free (xstring, TRUE);
    }
  else
    dest_value->data[0].v_pointer = xstrdup_printf ("%lu", src_value->data[0].v_ulong);

  xtype_class_unref (class);
}


/* registration
 */
void
_xvalue_transforms_init (void)
{
  /* some transformations are a bit questionable,
   * we currently skip those
   */
#define SKIP____register_transform_func(type1,type2,transform_func)     /* skip questionable transforms */ \
  (void)0

  /* numeric types (plus to string) */
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_CHAR,            value_transform_int_int);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_UCHAR,           value_transform_int_u8);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_BOOLEAN,         value_transform_int_bool);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_INT,             value_transform_int_int);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_UINT,            value_transform_int_uint);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_LONG,            value_transform_int_long);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_ULONG,           value_transform_int_ulong);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_INT64,           value_transform_int_int64);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_UINT64,          value_transform_int_uint64);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_ENUM,            value_transform_int_long);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_FLAGS,           value_transform_int_ulong);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_FLOAT,           value_transform_int_float);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_DOUBLE,          value_transform_int_double);
  xvalue_register_transform_func (XTYPE_CHAR,         XTYPE_STRING,          value_transform_int_string);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_CHAR,            value_transform_uint_s8);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_UCHAR,           value_transform_uint_uint);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_BOOLEAN,         value_transform_uint_bool);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_INT,             value_transform_uint_int);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_UINT,            value_transform_uint_uint);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_LONG,            value_transform_uint_long);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_ULONG,           value_transform_uint_ulong);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_INT64,           value_transform_uint_int64);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_UINT64,          value_transform_uint_uint64);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_ENUM,            value_transform_uint_long);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_FLAGS,           value_transform_uint_ulong);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_FLOAT,           value_transform_uint_float);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_DOUBLE,          value_transform_uint_double);
  xvalue_register_transform_func (XTYPE_UCHAR,        XTYPE_STRING,          value_transform_uint_string);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_CHAR,            value_transform_int_s8);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_UCHAR,           value_transform_int_u8);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_BOOLEAN,         value_transform_int_int);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_INT,             value_transform_int_int);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_UINT,            value_transform_int_uint);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_LONG,            value_transform_int_long);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_ULONG,           value_transform_int_ulong);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_INT64,           value_transform_int_int64);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_UINT64,          value_transform_int_uint64);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_ENUM,            value_transform_int_long);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_FLAGS,           value_transform_int_ulong);
  SKIP____register_transform_func (XTYPE_BOOLEAN,      XTYPE_FLOAT,           value_transform_int_float);
  SKIP____register_transform_func (XTYPE_BOOLEAN,      XTYPE_DOUBLE,          value_transform_int_double);
  xvalue_register_transform_func (XTYPE_BOOLEAN,      XTYPE_STRING,          value_transform_bool_string);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_CHAR,            value_transform_int_s8);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_UCHAR,           value_transform_int_u8);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_BOOLEAN,         value_transform_int_bool);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_INT,             value_transform_int_int);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_UINT,            value_transform_int_uint);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_LONG,            value_transform_int_long);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_ULONG,           value_transform_int_ulong);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_INT64,           value_transform_int_int64);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_UINT64,          value_transform_int_uint64);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_ENUM,            value_transform_int_long);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_FLAGS,           value_transform_int_ulong);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_FLOAT,           value_transform_int_float);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_DOUBLE,          value_transform_int_double);
  xvalue_register_transform_func (XTYPE_INT,          XTYPE_STRING,          value_transform_int_string);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_CHAR,            value_transform_uint_s8);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_UCHAR,           value_transform_uint_u8);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_BOOLEAN,         value_transform_uint_bool);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_INT,             value_transform_uint_int);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_UINT,            value_transform_uint_uint);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_LONG,            value_transform_uint_long);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_ULONG,           value_transform_uint_ulong);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_INT64,           value_transform_uint_int64);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_UINT64,          value_transform_uint_uint64);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_ENUM,            value_transform_uint_long);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_FLAGS,           value_transform_uint_ulong);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_FLOAT,           value_transform_uint_float);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_DOUBLE,          value_transform_uint_double);
  xvalue_register_transform_func (XTYPE_UINT,         XTYPE_STRING,          value_transform_uint_string);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_CHAR,            value_transform_long_s8);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_UCHAR,           value_transform_long_u8);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_BOOLEAN,         value_transform_long_bool);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_INT,             value_transform_long_int);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_UINT,            value_transform_long_uint);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_LONG,            value_transform_long_long);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_ULONG,           value_transform_long_ulong);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_INT64,           value_transform_long_int64);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_UINT64,          value_transform_long_uint64);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_ENUM,            value_transform_long_long);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_FLAGS,           value_transform_long_ulong);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_FLOAT,           value_transform_long_float);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_DOUBLE,          value_transform_long_double);
  xvalue_register_transform_func (XTYPE_LONG,         XTYPE_STRING,          value_transform_lonxstring);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_CHAR,            value_transform_ulong_s8);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_UCHAR,           value_transform_ulong_u8);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_BOOLEAN,         value_transform_ulong_bool);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_INT,             value_transform_ulong_int);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_UINT,            value_transform_ulong_uint);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_LONG,            value_transform_ulong_long);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_ULONG,           value_transform_ulong_ulong);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_INT64,           value_transform_ulong_int64);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_UINT64,          value_transform_ulong_uint64);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_ENUM,            value_transform_ulong_long);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_FLAGS,           value_transform_ulong_ulong);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_FLOAT,           value_transform_ulong_float);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_DOUBLE,          value_transform_ulong_double);
  xvalue_register_transform_func (XTYPE_ULONG,        XTYPE_STRING,          value_transform_ulonxstring);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_CHAR,            value_transform_int64_s8);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_UCHAR,           value_transform_int64_u8);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_BOOLEAN,         value_transform_int64_bool);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_INT,             value_transform_int64_int);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_UINT,            value_transform_int64_uint);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_LONG,            value_transform_int64_long);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_ULONG,           value_transform_int64_ulong);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_INT64,           value_transform_int64_int64);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_UINT64,          value_transform_int64_uint64);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_ENUM,            value_transform_int64_long);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_FLAGS,           value_transform_int64_ulong);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_FLOAT,           value_transform_int64_float);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_DOUBLE,          value_transform_int64_double);
  xvalue_register_transform_func (XTYPE_INT64,        XTYPE_STRING,          value_transform_int64_string);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_CHAR,            value_transform_uint64_s8);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_UCHAR,           value_transform_uint64_u8);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_BOOLEAN,         value_transform_uint64_bool);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_INT,             value_transform_uint64_int);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_UINT,            value_transform_uint64_uint);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_LONG,            value_transform_uint64_long);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_ULONG,           value_transform_uint64_ulong);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_INT64,           value_transform_uint64_int64);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_UINT64,          value_transform_uint64_uint64);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_ENUM,            value_transform_uint64_long);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_FLAGS,           value_transform_uint64_ulong);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_FLOAT,           value_transform_uint64_float);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_DOUBLE,          value_transform_uint64_double);
  xvalue_register_transform_func (XTYPE_UINT64,       XTYPE_STRING,          value_transform_uint64_string);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_CHAR,            value_transform_long_s8);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_UCHAR,           value_transform_long_u8);
  SKIP____register_transform_func (XTYPE_ENUM,         XTYPE_BOOLEAN,         value_transform_long_bool);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_INT,             value_transform_long_int);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_UINT,            value_transform_long_uint);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_LONG,            value_transform_long_long);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_ULONG,           value_transform_long_ulong);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_INT64,           value_transform_long_int64);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_UINT64,          value_transform_long_uint64);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_ENUM,            value_transform_long_long);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_FLAGS,           value_transform_long_ulong);
  SKIP____register_transform_func (XTYPE_ENUM,         XTYPE_FLOAT,           value_transform_long_float);
  SKIP____register_transform_func (XTYPE_ENUM,         XTYPE_DOUBLE,          value_transform_long_double);
  xvalue_register_transform_func (XTYPE_ENUM,         XTYPE_STRING,          value_transform_enum_string);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_CHAR,            value_transform_ulong_s8);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_UCHAR,           value_transform_ulong_u8);
  SKIP____register_transform_func (XTYPE_FLAGS,        XTYPE_BOOLEAN,         value_transform_ulong_bool);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_INT,             value_transform_ulong_int);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_UINT,            value_transform_ulong_uint);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_LONG,            value_transform_ulong_long);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_ULONG,           value_transform_ulong_ulong);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_INT64,           value_transform_ulong_int64);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_UINT64,          value_transform_ulong_uint64);
  SKIP____register_transform_func (XTYPE_FLAGS,        XTYPE_ENUM,            value_transform_ulong_long);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_FLAGS,           value_transform_ulong_ulong);
  SKIP____register_transform_func (XTYPE_FLAGS,        XTYPE_FLOAT,           value_transform_ulong_float);
  SKIP____register_transform_func (XTYPE_FLAGS,        XTYPE_DOUBLE,          value_transform_ulong_double);
  xvalue_register_transform_func (XTYPE_FLAGS,        XTYPE_STRING,          value_transform_flags_string);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_CHAR,            value_transform_float_s8);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_UCHAR,           value_transform_float_u8);
  SKIP____register_transform_func (XTYPE_FLOAT,        XTYPE_BOOLEAN,         value_transform_float_bool);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_INT,             value_transform_float_int);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_UINT,            value_transform_float_uint);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_LONG,            value_transform_float_long);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_ULONG,           value_transform_float_ulong);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_INT64,           value_transform_float_int64);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_UINT64,          value_transform_float_uint64);
  SKIP____register_transform_func (XTYPE_FLOAT,        XTYPE_ENUM,            value_transform_float_long);
  SKIP____register_transform_func (XTYPE_FLOAT,        XTYPE_FLAGS,           value_transform_float_ulong);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_FLOAT,           value_transform_float_float);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_DOUBLE,          value_transform_float_double);
  xvalue_register_transform_func (XTYPE_FLOAT,        XTYPE_STRING,          value_transform_float_string);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_CHAR,            value_transform_double_s8);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_UCHAR,           value_transform_double_u8);
  SKIP____register_transform_func (XTYPE_DOUBLE,       XTYPE_BOOLEAN,         value_transform_double_bool);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_INT,             value_transform_double_int);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_UINT,            value_transform_double_uint);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_LONG,            value_transform_double_long);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_ULONG,           value_transform_double_ulong);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_INT64,           value_transform_double_int64);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_UINT64,          value_transform_double_uint64);
  SKIP____register_transform_func (XTYPE_DOUBLE,       XTYPE_ENUM,            value_transform_double_long);
  SKIP____register_transform_func (XTYPE_DOUBLE,       XTYPE_FLAGS,           value_transform_double_ulong);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_FLOAT,           value_transform_double_float);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_DOUBLE,          value_transform_double_double);
  xvalue_register_transform_func (XTYPE_DOUBLE,       XTYPE_STRING,          value_transform_double_string);
  /* string types */
  xvalue_register_transform_func (XTYPE_STRING,       XTYPE_STRING,          value_transform_strinxstring);
}
