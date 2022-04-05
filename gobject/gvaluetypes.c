/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
 * Copyright © 2010 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MT safe
 */

#include "config.h"

#include <string.h>
#include <stdlib.h> /* qsort() */

#include "gvaluetypes.h"
#include "gtype-private.h"
#include "gvaluecollector.h"
#include "gobject.h"
#include "gparam.h"
#include "gboxed.h"
#include "genums.h"


/* --- value functions --- */
static void
value_init_long0 (GValue *value)
{
  value->data[0].v_long = 0;
}

static void
value_copy_long0 (const GValue *src_value,
		  GValue       *dest_value)
{
  dest_value->data[0].v_long = src_value->data[0].v_long;
}

static xchar_t*
value_lcopy_char (const GValue *value,
		  xuint_t         n_collect_values,
		  GTypeCValue  *collect_values,
		  xuint_t         collect_flags)
{
  gint8 *int8_p = collect_values[0].v_pointer;

  g_return_val_if_fail (int8_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *int8_p = value->data[0].v_int;

  return NULL;
}

static xchar_t*
value_lcopy_boolean (const GValue *value,
		     xuint_t         n_collect_values,
		     GTypeCValue  *collect_values,
		     xuint_t         collect_flags)
{
  xboolean_t *bool_p = collect_values[0].v_pointer;

  g_return_val_if_fail (bool_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *bool_p = value->data[0].v_int;

  return NULL;
}

static xchar_t*
value_collect_int (GValue      *value,
		   xuint_t        n_collect_values,
		   GTypeCValue *collect_values,
		   xuint_t        collect_flags)
{
  value->data[0].v_int = collect_values[0].v_int;

  return NULL;
}

static xchar_t*
value_lcopy_int (const GValue *value,
		 xuint_t         n_collect_values,
		 GTypeCValue  *collect_values,
		 xuint_t         collect_flags)
{
  xint_t *int_p = collect_values[0].v_pointer;

  g_return_val_if_fail (int_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *int_p = value->data[0].v_int;

  return NULL;
}

static xchar_t*
value_collect_long (GValue      *value,
		    xuint_t        n_collect_values,
		    GTypeCValue *collect_values,
		    xuint_t        collect_flags)
{
  value->data[0].v_long = collect_values[0].v_long;

  return NULL;
}

static xchar_t*
value_lcopy_long (const GValue *value,
		  xuint_t         n_collect_values,
		  GTypeCValue  *collect_values,
		  xuint_t         collect_flags)
{
  glong *long_p = collect_values[0].v_pointer;

  g_return_val_if_fail (long_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *long_p = value->data[0].v_long;

  return NULL;
}

static void
value_init_int64 (GValue *value)
{
  value->data[0].v_int64 = 0;
}

static void
value_copy_int64 (const GValue *src_value,
		  GValue       *dest_value)
{
  dest_value->data[0].v_int64 = src_value->data[0].v_int64;
}

static xchar_t*
value_collect_int64 (GValue      *value,
		     xuint_t        n_collect_values,
		     GTypeCValue *collect_values,
		     xuint_t        collect_flags)
{
  value->data[0].v_int64 = collect_values[0].v_int64;

  return NULL;
}

static xchar_t*
value_lcopy_int64 (const GValue *value,
		   xuint_t         n_collect_values,
		   GTypeCValue  *collect_values,
		   xuint_t         collect_flags)
{
  gint64 *int64_p = collect_values[0].v_pointer;

  g_return_val_if_fail (int64_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *int64_p = value->data[0].v_int64;

  return NULL;
}

static void
value_init_float (GValue *value)
{
  value->data[0].v_float = 0.0;
}

static void
value_copy_float (const GValue *src_value,
		  GValue       *dest_value)
{
  dest_value->data[0].v_float = src_value->data[0].v_float;
}

static xchar_t*
value_collect_float (GValue      *value,
		     xuint_t        n_collect_values,
		     GTypeCValue *collect_values,
		     xuint_t        collect_flags)
{
  value->data[0].v_float = collect_values[0].v_double;

  return NULL;
}

static xchar_t*
value_lcopy_float (const GValue *value,
		   xuint_t         n_collect_values,
		   GTypeCValue  *collect_values,
		   xuint_t         collect_flags)
{
  gfloat *float_p = collect_values[0].v_pointer;

  g_return_val_if_fail (float_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *float_p = value->data[0].v_float;

  return NULL;
}

static void
value_init_double (GValue *value)
{
  value->data[0].v_double = 0.0;
}

static void
value_copy_double (const GValue *src_value,
		   GValue	*dest_value)
{
  dest_value->data[0].v_double = src_value->data[0].v_double;
}

static xchar_t*
value_collect_double (GValue	  *value,
		      xuint_t        n_collect_values,
		      GTypeCValue *collect_values,
		      xuint_t        collect_flags)
{
  value->data[0].v_double = collect_values[0].v_double;

  return NULL;
}

static xchar_t*
value_lcopy_double (const GValue *value,
		    xuint_t         n_collect_values,
		    GTypeCValue  *collect_values,
		    xuint_t         collect_flags)
{
  xdouble_t *double_p = collect_values[0].v_pointer;

  g_return_val_if_fail (double_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *double_p = value->data[0].v_double;

  return NULL;
}

static void
value_init_string (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_free_string (GValue *value)
{
  if (!(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
    g_free (value->data[0].v_pointer);
}

static void
value_copy_string (const GValue *src_value,
		   GValue	*dest_value)
{
  if (src_value->data[1].v_uint & G_VALUE_INTERNED_STRING)
    {
      dest_value->data[0].v_pointer = src_value->data[0].v_pointer;
      dest_value->data[1].v_uint = src_value->data[1].v_uint;
    }
  else
    {
      dest_value->data[0].v_pointer = g_strdup (src_value->data[0].v_pointer);
      /* Don't copy over *any* flags, we're restarting from scratch */
    }
}

static xchar_t*
value_collect_string (GValue	  *value,
		      xuint_t        n_collect_values,
		      GTypeCValue *collect_values,
		      xuint_t        collect_flags)
{
  if (!collect_values[0].v_pointer)
    value->data[0].v_pointer = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    {
      value->data[0].v_pointer = collect_values[0].v_pointer;
      value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
    }
  else
    value->data[0].v_pointer = g_strdup (collect_values[0].v_pointer);

  return NULL;
}

static xchar_t*
value_lcopy_string (const GValue *value,
		    xuint_t         n_collect_values,
		    GTypeCValue  *collect_values,
		    xuint_t         collect_flags)
{
  xchar_t **string_p = collect_values[0].v_pointer;

  g_return_val_if_fail (string_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  if (!value->data[0].v_pointer)
    *string_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *string_p = value->data[0].v_pointer;
  else
    *string_p = g_strdup (value->data[0].v_pointer);

  return NULL;
}

static void
value_init_pointer (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_copy_pointer (const GValue *src_value,
		    GValue       *dest_value)
{
  dest_value->data[0].v_pointer = src_value->data[0].v_pointer;
}

static xpointer_t
value_peek_pointer0 (const GValue *value)
{
  return value->data[0].v_pointer;
}

static xchar_t*
value_collect_pointer (GValue      *value,
		       xuint_t        n_collect_values,
		       GTypeCValue *collect_values,
		       xuint_t        collect_flags)
{
  value->data[0].v_pointer = collect_values[0].v_pointer;

  return NULL;
}

static xchar_t*
value_lcopy_pointer (const GValue *value,
		     xuint_t         n_collect_values,
		     GTypeCValue  *collect_values,
		     xuint_t         collect_flags)
{
  xpointer_t *pointer_p = collect_values[0].v_pointer;

  g_return_val_if_fail (pointer_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *pointer_p = value->data[0].v_pointer;

  return NULL;
}

static void
value_free_variant (GValue *value)
{
  if (!(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS) &&
      value->data[0].v_pointer)
    g_variant_unref (value->data[0].v_pointer);
}

static void
value_copy_variant (const GValue *src_value,
		   GValue	*dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_variant_ref_sink (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static xchar_t*
value_collect_variant (GValue	  *value,
		      xuint_t        n_collect_values,
		      GTypeCValue *collect_values,
		      xuint_t        collect_flags)
{
  if (!collect_values[0].v_pointer)
    value->data[0].v_pointer = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    {
      value->data[0].v_pointer = collect_values[0].v_pointer;
      value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
    }
  else
    value->data[0].v_pointer = g_variant_ref_sink (collect_values[0].v_pointer);

  return NULL;
}

static xchar_t*
value_lcopy_variant (const GValue *value,
		    xuint_t         n_collect_values,
		    GTypeCValue  *collect_values,
		    xuint_t         collect_flags)
{
  xvariant_t **variant_p = collect_values[0].v_pointer;

  g_return_val_if_fail (variant_p != NULL, g_strdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  if (!value->data[0].v_pointer)
    *variant_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *variant_p = value->data[0].v_pointer;
  else
    *variant_p = g_variant_ref_sink (value->data[0].v_pointer);

  return NULL;
}

/* --- type initialization --- */
void
_g_value_types_init (void)
{
  GTypeInfo info = {
    0,				/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    NULL,			/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    0,				/* n_preallocs */
    NULL,			/* instance_init */
    NULL,			/* value_table */
  };
  const GTypeFundamentalInfo finfo = { XTYPE_FLAG_DERIVABLE, };
  xtype_t type G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  /* XTYPE_CHAR / XTYPE_UCHAR
   */
  {
    static const GTypeValueTable value_table = {
      value_init_long0,		/* value_init */
      NULL,			/* value_free */
      value_copy_long0,		/* value_copy */
      NULL,			/* value_peek_pointer */
      "i",			/* collect_format */
      value_collect_int,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_char,		/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_CHAR, g_intern_static_string ("xchar_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_CHAR);
    type = g_type_register_fundamental (XTYPE_UCHAR, g_intern_static_string ("guchar"), &info, &finfo, 0);
    g_assert (type == XTYPE_UCHAR);
  }

  /* XTYPE_BOOLEAN
   */
  {
    static const GTypeValueTable value_table = {
      value_init_long0,		 /* value_init */
      NULL,			 /* value_free */
      value_copy_long0,		 /* value_copy */
      NULL,                      /* value_peek_pointer */
      "i",			 /* collect_format */
      value_collect_int,	 /* collect_value */
      "p",			 /* lcopy_format */
      value_lcopy_boolean,	 /* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_BOOLEAN, g_intern_static_string ("xboolean_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_BOOLEAN);
  }

  /* XTYPE_INT / XTYPE_UINT
   */
  {
    static const GTypeValueTable value_table = {
      value_init_long0,		/* value_init */
      NULL,			/* value_free */
      value_copy_long0,		/* value_copy */
      NULL,                     /* value_peek_pointer */
      "i",			/* collect_format */
      value_collect_int,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_int,		/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_INT, g_intern_static_string ("xint_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_INT);
    type = g_type_register_fundamental (XTYPE_UINT, g_intern_static_string ("xuint_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_UINT);
  }

  /* XTYPE_LONG / XTYPE_ULONG
   */
  {
    static const GTypeValueTable value_table = {
      value_init_long0,		/* value_init */
      NULL,			/* value_free */
      value_copy_long0,		/* value_copy */
      NULL,                     /* value_peek_pointer */
      "l",			/* collect_format */
      value_collect_long,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_long,		/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_LONG, g_intern_static_string ("glong"), &info, &finfo, 0);
    g_assert (type == XTYPE_LONG);
    type = g_type_register_fundamental (XTYPE_ULONG, g_intern_static_string ("gulong"), &info, &finfo, 0);
    g_assert (type == XTYPE_ULONG);
  }

  /* XTYPE_INT64 / XTYPE_UINT64
   */
  {
    static const GTypeValueTable value_table = {
      value_init_int64,		/* value_init */
      NULL,			/* value_free */
      value_copy_int64,		/* value_copy */
      NULL,                     /* value_peek_pointer */
      "q",			/* collect_format */
      value_collect_int64,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_int64,	/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_INT64, g_intern_static_string ("gint64"), &info, &finfo, 0);
    g_assert (type == XTYPE_INT64);
    type = g_type_register_fundamental (XTYPE_UINT64, g_intern_static_string ("guint64"), &info, &finfo, 0);
    g_assert (type == XTYPE_UINT64);
  }

  /* XTYPE_FLOAT
   */
  {
    static const GTypeValueTable value_table = {
      value_init_float,		 /* value_init */
      NULL,			 /* value_free */
      value_copy_float,		 /* value_copy */
      NULL,                      /* value_peek_pointer */
      "d",			 /* collect_format */
      value_collect_float,	 /* collect_value */
      "p",			 /* lcopy_format */
      value_lcopy_float,	 /* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_FLOAT, g_intern_static_string ("gfloat"), &info, &finfo, 0);
    g_assert (type == XTYPE_FLOAT);
  }

  /* XTYPE_DOUBLE
   */
  {
    static const GTypeValueTable value_table = {
      value_init_double,	/* value_init */
      NULL,			/* value_free */
      value_copy_double,	/* value_copy */
      NULL,                     /* value_peek_pointer */
      "d",			/* collect_format */
      value_collect_double,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_double,	/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_DOUBLE, g_intern_static_string ("xdouble_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_DOUBLE);
  }

  /* XTYPE_STRING
   */
  {
    static const GTypeValueTable value_table = {
      value_init_string,	/* value_init */
      value_free_string,	/* value_free */
      value_copy_string,	/* value_copy */
      value_peek_pointer0,	/* value_peek_pointer */
      "p",			/* collect_format */
      value_collect_string,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_string,	/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_STRING, g_intern_static_string ("gchararray"), &info, &finfo, 0);
    g_assert (type == XTYPE_STRING);
  }

  /* XTYPE_POINTER
   */
  {
    static const GTypeValueTable value_table = {
      value_init_pointer,	/* value_init */
      NULL,			/* value_free */
      value_copy_pointer,	/* value_copy */
      value_peek_pointer0,	/* value_peek_pointer */
      "p",			/* collect_format */
      value_collect_pointer,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_pointer,	/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_POINTER, g_intern_static_string ("xpointer_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_POINTER);
  }

  /* XTYPE_VARIANT
   */
  {
    static const GTypeValueTable value_table = {
      value_init_pointer,	/* value_init */
      value_free_variant,	/* value_free */
      value_copy_variant,	/* value_copy */
      value_peek_pointer0,	/* value_peek_pointer */
      "p",			/* collect_format */
      value_collect_variant,	/* collect_value */
      "p",			/* lcopy_format */
      value_lcopy_variant,	/* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (XTYPE_VARIANT, g_intern_static_string ("xvariant_t"), &info, &finfo, 0);
    g_assert (type == XTYPE_VARIANT);
  }
}


/* --- GValue functions --- */
/**
 * g_value_set_char:
 * @value: a valid #GValue of type %XTYPE_CHAR
 * @v_char: character value to be set
 *
 * Set the contents of a %XTYPE_CHAR #GValue to @v_char.
 * Deprecated: 2.32: This function's input type is broken, see g_value_set_schar()
 */
void
g_value_set_char (GValue *value,
		  xchar_t	  v_char)
{
  g_return_if_fail (G_VALUE_HOLDS_CHAR (value));

  value->data[0].v_int = v_char;
}

/**
 * g_value_get_char:
 * @value: a valid #GValue of type %XTYPE_CHAR
 *
 * Do not use this function; it is broken on platforms where the %char
 * type is unsigned, such as ARM and PowerPC.  See g_value_get_schar().
 *
 * Get the contents of a %XTYPE_CHAR #GValue.
 *
 * Returns: character contents of @value
 * Deprecated: 2.32: This function's return type is broken, see g_value_get_schar()
 */
xchar_t
g_value_get_char (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_CHAR (value), 0);

  return value->data[0].v_int;
}

/**
 * g_value_set_schar:
 * @value: a valid #GValue of type %XTYPE_CHAR
 * @v_char: signed 8 bit integer to be set
 *
 * Set the contents of a %XTYPE_CHAR #GValue to @v_char.
 *
 * Since: 2.32
 */
void
g_value_set_schar (GValue *value,
		   gint8   v_char)
{
  g_return_if_fail (G_VALUE_HOLDS_CHAR (value));

  value->data[0].v_int = v_char;
}

/**
 * g_value_get_schar:
 * @value: a valid #GValue of type %XTYPE_CHAR
 *
 * Get the contents of a %XTYPE_CHAR #GValue.
 *
 * Returns: signed 8 bit integer contents of @value
 * Since: 2.32
 */
gint8
g_value_get_schar (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_CHAR (value), 0);

  return value->data[0].v_int;
}

/**
 * g_value_set_uchar:
 * @value: a valid #GValue of type %XTYPE_UCHAR
 * @v_uchar: unsigned character value to be set
 *
 * Set the contents of a %XTYPE_UCHAR #GValue to @v_uchar.
 */
void
g_value_set_uchar (GValue *value,
		   guchar  v_uchar)
{
  g_return_if_fail (G_VALUE_HOLDS_UCHAR (value));

  value->data[0].v_uint = v_uchar;
}

/**
 * g_value_get_uchar:
 * @value: a valid #GValue of type %XTYPE_UCHAR
 *
 * Get the contents of a %XTYPE_UCHAR #GValue.
 *
 * Returns: unsigned character contents of @value
 */
guchar
g_value_get_uchar (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_UCHAR (value), 0);

  return value->data[0].v_uint;
}

/**
 * g_value_set_boolean:
 * @value: a valid #GValue of type %XTYPE_BOOLEAN
 * @v_boolean: boolean value to be set
 *
 * Set the contents of a %XTYPE_BOOLEAN #GValue to @v_boolean.
 */
void
g_value_set_boolean (GValue  *value,
		     xboolean_t v_boolean)
{
  g_return_if_fail (G_VALUE_HOLDS_BOOLEAN (value));

  value->data[0].v_int = v_boolean != FALSE;
}

/**
 * g_value_get_boolean:
 * @value: a valid #GValue of type %XTYPE_BOOLEAN
 *
 * Get the contents of a %XTYPE_BOOLEAN #GValue.
 *
 * Returns: boolean contents of @value
 */
xboolean_t
g_value_get_boolean (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_BOOLEAN (value), 0);

  return value->data[0].v_int;
}

/**
 * g_value_set_int:
 * @value: a valid #GValue of type %XTYPE_INT
 * @v_int: integer value to be set
 *
 * Set the contents of a %XTYPE_INT #GValue to @v_int.
 */
void
g_value_set_int (GValue *value,
		 xint_t	 v_int)
{
  g_return_if_fail (G_VALUE_HOLDS_INT (value));

  value->data[0].v_int = v_int;
}

/**
 * g_value_get_int:
 * @value: a valid #GValue of type %XTYPE_INT
 *
 * Get the contents of a %XTYPE_INT #GValue.
 *
 * Returns: integer contents of @value
 */
xint_t
g_value_get_int (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_INT (value), 0);

  return value->data[0].v_int;
}

/**
 * g_value_set_uint:
 * @value: a valid #GValue of type %XTYPE_UINT
 * @v_uint: unsigned integer value to be set
 *
 * Set the contents of a %XTYPE_UINT #GValue to @v_uint.
 */
void
g_value_set_uint (GValue *value,
		  xuint_t	  v_uint)
{
  g_return_if_fail (G_VALUE_HOLDS_UINT (value));

  value->data[0].v_uint = v_uint;
}

/**
 * g_value_get_uint:
 * @value: a valid #GValue of type %XTYPE_UINT
 *
 * Get the contents of a %XTYPE_UINT #GValue.
 *
 * Returns: unsigned integer contents of @value
 */
xuint_t
g_value_get_uint (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_UINT (value), 0);

  return value->data[0].v_uint;
}

/**
 * g_value_set_long:
 * @value: a valid #GValue of type %XTYPE_LONG
 * @v_long: long integer value to be set
 *
 * Set the contents of a %XTYPE_LONG #GValue to @v_long.
 */
void
g_value_set_long (GValue *value,
		  glong	  v_long)
{
  g_return_if_fail (G_VALUE_HOLDS_LONG (value));

  value->data[0].v_long = v_long;
}

/**
 * g_value_get_long:
 * @value: a valid #GValue of type %XTYPE_LONG
 *
 * Get the contents of a %XTYPE_LONG #GValue.
 *
 * Returns: long integer contents of @value
 */
glong
g_value_get_long (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_LONG (value), 0);

  return value->data[0].v_long;
}

/**
 * g_value_set_ulong:
 * @value: a valid #GValue of type %XTYPE_ULONG
 * @v_ulong: unsigned long integer value to be set
 *
 * Set the contents of a %XTYPE_ULONG #GValue to @v_ulong.
 */
void
g_value_set_ulong (GValue *value,
		   gulong  v_ulong)
{
  g_return_if_fail (G_VALUE_HOLDS_ULONG (value));

  value->data[0].v_ulong = v_ulong;
}

/**
 * g_value_get_ulong:
 * @value: a valid #GValue of type %XTYPE_ULONG
 *
 * Get the contents of a %XTYPE_ULONG #GValue.
 *
 * Returns: unsigned long integer contents of @value
 */
gulong
g_value_get_ulong (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_ULONG (value), 0);

  return value->data[0].v_ulong;
}

/**
 * g_value_get_int64:
 * @value: a valid #GValue of type %XTYPE_INT64
 *
 * Get the contents of a %XTYPE_INT64 #GValue.
 *
 * Returns: 64bit integer contents of @value
 */
void
g_value_set_int64 (GValue *value,
		   gint64  v_int64)
{
  g_return_if_fail (G_VALUE_HOLDS_INT64 (value));

  value->data[0].v_int64 = v_int64;
}

/**
 * g_value_set_int64:
 * @value: a valid #GValue of type %XTYPE_INT64
 * @v_int64: 64bit integer value to be set
 *
 * Set the contents of a %XTYPE_INT64 #GValue to @v_int64.
 */
gint64
g_value_get_int64 (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_INT64 (value), 0);

  return value->data[0].v_int64;
}

/**
 * g_value_set_uint64:
 * @value: a valid #GValue of type %XTYPE_UINT64
 * @v_uint64: unsigned 64bit integer value to be set
 *
 * Set the contents of a %XTYPE_UINT64 #GValue to @v_uint64.
 */
void
g_value_set_uint64 (GValue *value,
		    guint64 v_uint64)
{
  g_return_if_fail (G_VALUE_HOLDS_UINT64 (value));

  value->data[0].v_uint64 = v_uint64;
}

/**
 * g_value_get_uint64:
 * @value: a valid #GValue of type %XTYPE_UINT64
 *
 * Get the contents of a %XTYPE_UINT64 #GValue.
 *
 * Returns: unsigned 64bit integer contents of @value
 */
guint64
g_value_get_uint64 (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_UINT64 (value), 0);

  return value->data[0].v_uint64;
}

/**
 * g_value_set_float:
 * @value: a valid #GValue of type %XTYPE_FLOAT
 * @v_float: float value to be set
 *
 * Set the contents of a %XTYPE_FLOAT #GValue to @v_float.
 */
void
g_value_set_float (GValue *value,
		   gfloat  v_float)
{
  g_return_if_fail (G_VALUE_HOLDS_FLOAT (value));

  value->data[0].v_float = v_float;
}

/**
 * g_value_get_float:
 * @value: a valid #GValue of type %XTYPE_FLOAT
 *
 * Get the contents of a %XTYPE_FLOAT #GValue.
 *
 * Returns: float contents of @value
 */
gfloat
g_value_get_float (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_FLOAT (value), 0);

  return value->data[0].v_float;
}

/**
 * g_value_set_double:
 * @value: a valid #GValue of type %XTYPE_DOUBLE
 * @v_double: double value to be set
 *
 * Set the contents of a %XTYPE_DOUBLE #GValue to @v_double.
 */
void
g_value_set_double (GValue *value,
		    xdouble_t v_double)
{
  g_return_if_fail (G_VALUE_HOLDS_DOUBLE (value));

  value->data[0].v_double = v_double;
}

/**
 * g_value_get_double:
 * @value: a valid #GValue of type %XTYPE_DOUBLE
 *
 * Get the contents of a %XTYPE_DOUBLE #GValue.
 *
 * Returns: double contents of @value
 */
xdouble_t
g_value_get_double (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_DOUBLE (value), 0);

  return value->data[0].v_double;
}

/**
 * g_value_set_string:
 * @value: a valid #GValue of type %XTYPE_STRING
 * @v_string: (nullable): caller-owned string to be duplicated for the #GValue
 *
 * Set the contents of a %XTYPE_STRING #GValue to a copy of @v_string.
 */
void
g_value_set_string (GValue	*value,
		    const xchar_t *v_string)
{
  xchar_t *new_val;

  g_return_if_fail (G_VALUE_HOLDS_STRING (value));

  new_val = g_strdup (v_string);

  if (value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS)
    value->data[1].v_uint = 0;
  else
    g_free (value->data[0].v_pointer);

  value->data[0].v_pointer = new_val;
}

/**
 * g_value_set_static_string:
 * @value: a valid #GValue of type %XTYPE_STRING
 * @v_string: (nullable): static string to be set
 *
 * Set the contents of a %XTYPE_STRING #GValue to @v_string.
 * The string is assumed to be static, and is thus not duplicated
 * when setting the #GValue.
 *
 * If the the string is a canonical string, using g_value_set_interned_string()
 * is more appropriate.
 */
void
g_value_set_static_string (GValue      *value,
			   const xchar_t *v_string)
{
  g_return_if_fail (G_VALUE_HOLDS_STRING (value));

  if (!(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
    g_free (value->data[0].v_pointer);
  value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
  value->data[0].v_pointer = (xchar_t*) v_string;
}

/**
 * g_value_set_interned_string:
 * @value: a valid #GValue of type %XTYPE_STRING
 * @v_string: (nullable): static string to be set
 *
 * Set the contents of a %XTYPE_STRING #GValue to @v_string.  The string is
 * assumed to be static and interned (canonical, for example from
 * g_intern_string()), and is thus not duplicated when setting the #GValue.
 *
 * Since: 2.66
 */
void
g_value_set_interned_string (GValue *value,
                             const xchar_t *v_string)
{
  g_return_if_fail (G_VALUE_HOLDS_STRING (value));

  if (!(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
    g_free (value->data[0].v_pointer);
  value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS | G_VALUE_INTERNED_STRING;
  value->data[0].v_pointer = (xchar_t *) v_string;
}

/**
 * g_value_set_string_take_ownership:
 * @value: a valid #GValue of type %XTYPE_STRING
 * @v_string: (nullable): duplicated unowned string to be set
 *
 * This is an internal function introduced mainly for C marshallers.
 *
 * Deprecated: 2.4: Use g_value_take_string() instead.
 */
void
g_value_set_string_take_ownership (GValue *value,
				   xchar_t  *v_string)
{
  g_value_take_string (value, v_string);
}

/**
 * g_value_take_string:
 * @value: a valid #GValue of type %XTYPE_STRING
 * @v_string: (nullable): string to take ownership of
 *
 * Sets the contents of a %XTYPE_STRING #GValue to @v_string.
 *
 * Since: 2.4
 */
void
g_value_take_string (GValue *value,
		     xchar_t  *v_string)
{
  g_return_if_fail (G_VALUE_HOLDS_STRING (value));

  if (value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS)
    value->data[1].v_uint = 0;
  else
    g_free (value->data[0].v_pointer);
  value->data[0].v_pointer = v_string;
}

/**
 * g_value_get_string:
 * @value: a valid #GValue of type %XTYPE_STRING
 *
 * Get the contents of a %XTYPE_STRING #GValue.
 *
 * Returns: string content of @value
 */
const xchar_t*
g_value_get_string (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_STRING (value), NULL);

  return value->data[0].v_pointer;
}

/**
 * g_value_dup_string:
 * @value: a valid #GValue of type %XTYPE_STRING
 *
 * Get a copy the contents of a %XTYPE_STRING #GValue.
 *
 * Returns: a newly allocated copy of the string content of @value
 */
xchar_t*
g_value_dup_string (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_STRING (value), NULL);

  return g_strdup (value->data[0].v_pointer);
}

/**
 * g_value_set_pointer:
 * @value: a valid #GValue of %XTYPE_POINTER
 * @v_pointer: pointer value to be set
 *
 * Set the contents of a pointer #GValue to @v_pointer.
 */
void
g_value_set_pointer (GValue  *value,
		     xpointer_t v_pointer)
{
  g_return_if_fail (G_VALUE_HOLDS_POINTER (value));

  value->data[0].v_pointer = v_pointer;
}

/**
 * g_value_get_pointer:
 * @value: a valid #GValue of %XTYPE_POINTER
 *
 * Get the contents of a pointer #GValue.
 *
 * Returns: (transfer none): pointer contents of @value
 */
xpointer_t
g_value_get_pointer (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_POINTER (value), NULL);

  return value->data[0].v_pointer;
}

G_DEFINE_POINTER_TYPE (xtype_t, g_gtype)

/**
 * g_value_set_gtype:
 * @value: a valid #GValue of type %XTYPE_GTYPE
 * @v_gtype: #xtype_t to be set
 *
 * Set the contents of a %XTYPE_GTYPE #GValue to @v_gtype.
 *
 * Since: 2.12
 */
void
g_value_set_gtype (GValue *value,
		   xtype_t   v_gtype)
{
  g_return_if_fail (G_VALUE_HOLDS_GTYPE (value));

  value->data[0].v_pointer = GSIZE_TO_POINTER (v_gtype);

}

/**
 * g_value_get_gtype:
 * @value: a valid #GValue of type %XTYPE_GTYPE
 *
 * Get the contents of a %XTYPE_GTYPE #GValue.
 *
 * Since: 2.12
 *
 * Returns: the #xtype_t stored in @value
 */
xtype_t
g_value_get_gtype (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_GTYPE (value), 0);

  return GPOINTER_TO_SIZE (value->data[0].v_pointer);
}

/**
 * g_value_set_variant:
 * @value: a valid #GValue of type %XTYPE_VARIANT
 * @variant: (nullable): a #xvariant_t, or %NULL
 *
 * Set the contents of a variant #GValue to @variant.
 * If the variant is floating, it is consumed.
 *
 * Since: 2.26
 */
void
g_value_set_variant (GValue   *value,
                     xvariant_t *variant)
{
  xvariant_t *old_variant;

  g_return_if_fail (G_VALUE_HOLDS_VARIANT (value));

  old_variant = value->data[0].v_pointer;

  if (variant)
    value->data[0].v_pointer = g_variant_ref_sink (variant);
  else
    value->data[0].v_pointer = NULL;

  if (old_variant)
    g_variant_unref (old_variant);
}

/**
 * g_value_take_variant:
 * @value: a valid #GValue of type %XTYPE_VARIANT
 * @variant: (nullable) (transfer full): a #xvariant_t, or %NULL
 *
 * Set the contents of a variant #GValue to @variant, and takes over
 * the ownership of the caller's reference to @variant;
 * the caller doesn't have to unref it any more (i.e. the reference
 * count of the variant is not increased).
 *
 * If @variant was floating then its floating reference is converted to
 * a hard reference.
 *
 * If you want the #GValue to hold its own reference to @variant, use
 * g_value_set_variant() instead.
 *
 * This is an internal function introduced mainly for C marshallers.
 *
 * Since: 2.26
 */
void
g_value_take_variant (GValue   *value,
                      xvariant_t *variant)
{
  xvariant_t *old_variant;

  g_return_if_fail (G_VALUE_HOLDS_VARIANT (value));

  old_variant = value->data[0].v_pointer;

  if (variant)
    value->data[0].v_pointer = g_variant_take_ref (variant);
  else
    value->data[0].v_pointer = NULL;

  if (old_variant)
    g_variant_unref (old_variant);
}

/**
 * g_value_get_variant:
 * @value: a valid #GValue of type %XTYPE_VARIANT
 *
 * Get the contents of a variant #GValue.
 *
 * Returns: (transfer none) (nullable): variant contents of @value (may be %NULL)
 *
 * Since: 2.26
 */
xvariant_t*
g_value_get_variant (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_VARIANT (value), NULL);

  return value->data[0].v_pointer;
}

/**
 * g_value_dup_variant:
 * @value: a valid #GValue of type %XTYPE_VARIANT
 *
 * Get the contents of a variant #GValue, increasing its refcount. The returned
 * #xvariant_t is never floating.
 *
 * Returns: (transfer full) (nullable): variant contents of @value (may be %NULL);
 *    should be unreffed using g_variant_unref() when no longer needed
 *
 * Since: 2.26
 */
xvariant_t*
g_value_dup_variant (const GValue *value)
{
  xvariant_t *variant;

  g_return_val_if_fail (G_VALUE_HOLDS_VARIANT (value), NULL);

  variant = value->data[0].v_pointer;
  if (variant)
    g_variant_ref_sink (variant);

  return variant;
}

/**
 * g_strdup_value_contents:
 * @value: #GValue which contents are to be described.
 *
 * Return a newly allocated string, which describes the contents of a
 * #GValue.  The main purpose of this function is to describe #GValue
 * contents for debugging output, the way in which the contents are
 * described may change between different GLib versions.
 *
 * Returns: Newly allocated string.
 */
xchar_t*
g_strdup_value_contents (const GValue *value)
{
  const xchar_t *src;
  xchar_t *contents;

  g_return_val_if_fail (X_IS_VALUE (value), NULL);

  if (G_VALUE_HOLDS_STRING (value))
    {
      src = g_value_get_string (value);

      if (!src)
	contents = g_strdup ("NULL");
      else
	{
	  xchar_t *s = g_strescape (src, NULL);

	  contents = g_strdup_printf ("\"%s\"", s);
	  g_free (s);
	}
    }
  else if (g_value_type_transformable (G_VALUE_TYPE (value), XTYPE_STRING))
    {
      GValue tmp_value = G_VALUE_INIT;
      xchar_t *s;

      g_value_init (&tmp_value, XTYPE_STRING);
      g_value_transform (value, &tmp_value);
      s = g_strescape (g_value_get_string (&tmp_value), NULL);
      g_value_unset (&tmp_value);
      if (G_VALUE_HOLDS_ENUM (value) || G_VALUE_HOLDS_FLAGS (value))
	contents = g_strdup_printf ("((%s) %s)",
				    g_type_name (G_VALUE_TYPE (value)),
				    s);
      else
	contents = g_strdup (s ? s : "NULL");
      g_free (s);
    }
  else if (g_value_fits_pointer (value))
    {
      xpointer_t p = g_value_peek_pointer (value);

      if (!p)
	contents = g_strdup ("NULL");
      else if (G_VALUE_HOLDS_OBJECT (value))
	contents = g_strdup_printf ("((%s*) %p)", G_OBJECT_TYPE_NAME (p), p);
      else if (G_VALUE_HOLDS_PARAM (value))
	contents = g_strdup_printf ("((%s*) %p)", G_PARAM_SPEC_TYPE_NAME (p), p);
      else if (G_VALUE_HOLDS (value, XTYPE_STRV))
        {
          GStrv strv = g_value_get_boxed (value);
          GString *tmp = g_string_new ("[");

          while (*strv != NULL)
            {
              xchar_t *escaped = g_strescape (*strv, NULL);

              g_string_append_printf (tmp, "\"%s\"", escaped);
              g_free (escaped);

              if (*++strv != NULL)
                g_string_append (tmp, ", ");
            }

          g_string_append (tmp, "]");
          contents = g_string_free (tmp, FALSE);
        }
      else if (G_VALUE_HOLDS_BOXED (value))
	contents = g_strdup_printf ("((%s*) %p)", g_type_name (G_VALUE_TYPE (value)), p);
      else if (G_VALUE_HOLDS_POINTER (value))
	contents = g_strdup_printf ("((xpointer_t) %p)", p);
      else
	contents = g_strdup ("???");
    }
  else
    contents = g_strdup ("???");

  return contents;
}

/**
 * g_pointer_type_register_static:
 * @name: the name of the new pointer type.
 *
 * Creates a new %XTYPE_POINTER derived type id for a new
 * pointer type with name @name.
 *
 * Returns: a new %XTYPE_POINTER derived type id for @name.
 */
xtype_t
g_pointer_type_register_static (const xchar_t *name)
{
  const GTypeInfo type_info = {
    0,			/* class_size */
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    NULL,		/* class_init */
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    0,			/* instance_size */
    0,			/* n_preallocs */
    NULL,		/* instance_init */
    NULL		/* value_table */
  };
  xtype_t type;

  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (g_type_from_name (name) == 0, 0);

  type = g_type_register_static (XTYPE_POINTER, name, &type_info, 0);

  return type;
}
