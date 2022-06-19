/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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

#include "genums.h"
#include "gtype-private.h"
#include "gvalue.h"
#include "gvaluecollector.h"


/**
 * SECTION:enumerations_flags
 * @short_description: Enumeration and flags types
 * @title: Enumeration and Flag Types
 * @see_also:#GParamSpecEnum, #GParamSpecFlags, xparam_spec_enum(),
 * xparam_spec_flags()
 *
 * The GLib type system provides fundamental types for enumeration and
 * flags types. (Flags types are like enumerations, but allow their
 * values to be combined by bitwise or). A registered enumeration or
 * flags type associates a name and a nickname with each allowed
 * value, and the methods xenum_get_value_by_name(),
 * xenum_get_value_by_nick(), xflags_get_value_by_name() and
 * xflags_get_value_by_nick() can look up values by their name or
 * nickname.  When an enumeration or flags type is registered with the
 * GLib type system, it can be used as value type for object
 * properties, using xparam_spec_enum() or xparam_spec_flags().
 *
 * xobject_t ships with a utility called [glib-mkenums][glib-mkenums],
 * that can construct suitable type registration functions from C enumeration
 * definitions.
 *
 * Example of how to get a string representation of an enum value:
 * |[<!-- language="C" -->
 * xenum_class_t *enum_class;
 * xenum_value_t *enum_value;
 *
 * enum_class = xtype_class_ref (MAMAN_TYPE_MY_ENUM);
 * enum_value = xenum_get_value (enum_class, MAMAN_MY_ENUM_FOO);
 *
 * g_print ("Name: %s\n", enum_value->value_name);
 *
 * xtype_class_unref (enum_class);
 * ]|
 */


/* --- prototypes --- */
static void	xenum_class_init		(xenum_class_t	*class,
						 xpointer_t	 class_data);
static void	xflags_class_init		(xflags_class_t	*class,
						 xpointer_t	 class_data);
static void	value_flags_enum_init		(xvalue_t		*value);
static void	value_flags_enum_copy_value	(const xvalue_t	*src_value,
						 xvalue_t		*dest_value);
static xchar_t*	value_flags_enum_collect_value  (xvalue_t		*value,
						 xuint_t           n_collect_values,
						 xtype_c_value_t    *collect_values,
						 xuint_t           collect_flags);
static xchar_t*	value_flags_enum_lcopy_value	(const xvalue_t	*value,
						 xuint_t           n_collect_values,
						 xtype_c_value_t    *collect_values,
						 xuint_t           collect_flags);

/* --- functions --- */
void
_xenum_types_init (void)
{
  static xboolean_t initialized = FALSE;
  static const xtype_value_table_t flags_enum_value_table = {
    value_flags_enum_init,	    /* value_init */
    NULL,			    /* value_free */
    value_flags_enum_copy_value,    /* value_copy */
    NULL,			    /* value_peek_pointer */
    "i",			    /* collect_format */
    value_flags_enum_collect_value, /* collect_value */
    "p",			    /* lcopy_format */
    value_flags_enum_lcopy_value,   /* lcopy_value */
  };
  xtype_info_t info = {
    0,                          /* class_size */
    NULL,                       /* base_init */
    NULL,                       /* base_destroy */
    NULL,                       /* class_init */
    NULL,                       /* class_destroy */
    NULL,                       /* class_data */
    0,                          /* instance_size */
    0,                          /* n_preallocs */
    NULL,                       /* instance_init */
    &flags_enum_value_table,    /* value_table */
  };
  static const GTypeFundamentalInfo finfo = {
    XTYPE_FLAG_CLASSED | XTYPE_FLAG_DERIVABLE,
  };
  xtype_t type G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  g_return_if_fail (initialized == FALSE);
  initialized = TRUE;

  /* XTYPE_ENUM
   */
  info.class_size = sizeof (xenum_class_t);
  type = xtype_register_fundamental (XTYPE_ENUM, g_intern_static_string ("xenum_t"), &info, &finfo,
				      XTYPE_FLAG_ABSTRACT | XTYPE_FLAG_VALUE_ABSTRACT);
  xassert (type == XTYPE_ENUM);

  /* XTYPE_FLAGS
   */
  info.class_size = sizeof (xflags_class_t);
  type = xtype_register_fundamental (XTYPE_FLAGS, g_intern_static_string ("GFlags"), &info, &finfo,
				      XTYPE_FLAG_ABSTRACT | XTYPE_FLAG_VALUE_ABSTRACT);
  xassert (type == XTYPE_FLAGS);
}

static void
value_flags_enum_init (xvalue_t *value)
{
  value->data[0].v_long = 0;
}

static void
value_flags_enum_copy_value (const xvalue_t *src_value,
			     xvalue_t	  *dest_value)
{
  dest_value->data[0].v_long = src_value->data[0].v_long;
}

static xchar_t*
value_flags_enum_collect_value (xvalue_t      *value,
				xuint_t        n_collect_values,
				xtype_c_value_t *collect_values,
				xuint_t        collect_flags)
{
  if (G_VALUE_HOLDS_ENUM (value))
    value->data[0].v_long = collect_values[0].v_int;
  else
    value->data[0].v_ulong = (xuint_t) collect_values[0].v_int;

  return NULL;
}

static xchar_t*
value_flags_enum_lcopy_value (const xvalue_t *value,
			      xuint_t         n_collect_values,
			      xtype_c_value_t  *collect_values,
			      xuint_t         collect_flags)
{
  xint_t *int_p = collect_values[0].v_pointer;

  xreturn_val_if_fail (int_p != NULL, xstrdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  *int_p = value->data[0].v_long;

  return NULL;
}

/**
 * xenum_register_static:
 * @name: A nul-terminated string used as the name of the new type.
 * @const_static_values: An array of #xenum_value_t structs for the possible
 *  enumeration values. The array is terminated by a struct with all
 *  members being 0. xobject_t keeps a reference to the data, so it cannot
 *  be stack-allocated.
 *
 * Registers a new static enumeration type with the name @name.
 *
 * It is normally more convenient to let [glib-mkenums][glib-mkenums],
 * generate a my_enum_get_type() function from a usual C enumeration
 * definition  than to write one yourself using xenum_register_static().
 *
 * Returns: The new type identifier.
 */
xtype_t
xenum_register_static (const xchar_t	 *name,
			const xenum_value_t *const_static_values)
{
  xtype_info_t enum_type_info = {
    sizeof (xenum_class_t), /* class_size */
    NULL,                /* base_init */
    NULL,                /* base_finalize */
    (xclass_init_func_t) xenum_class_init,
    NULL,                /* class_finalize */
    NULL,                /* class_data */
    0,                   /* instance_size */
    0,                   /* n_preallocs */
    NULL,                /* instance_init */
    NULL,		 /* value_table */
  };
  xtype_t type;

  xreturn_val_if_fail (name != NULL, 0);
  xreturn_val_if_fail (const_static_values != NULL, 0);

  enum_type_info.class_data = const_static_values;

  type = xtype_register_static (XTYPE_ENUM, name, &enum_type_info, 0);

  return type;
}

/**
 * xflags_register_static:
 * @name: A nul-terminated string used as the name of the new type.
 * @const_static_values: An array of #xflags_value_t structs for the possible
 *  flags values. The array is terminated by a struct with all members being 0.
 *  xobject_t keeps a reference to the data, so it cannot be stack-allocated.
 *
 * Registers a new static flags type with the name @name.
 *
 * It is normally more convenient to let [glib-mkenums][glib-mkenums]
 * generate a my_flags_get_type() function from a usual C enumeration
 * definition than to write one yourself using xflags_register_static().
 *
 * Returns: The new type identifier.
 */
xtype_t
xflags_register_static (const xchar_t	   *name,
			 const xflags_value_t *const_static_values)
{
  xtype_info_t flags_type_info = {
    sizeof (xflags_class_t), /* class_size */
    NULL,                 /* base_init */
    NULL,                 /* base_finalize */
    (xclass_init_func_t) xflags_class_init,
    NULL,                 /* class_finalize */
    NULL,                 /* class_data */
    0,                    /* instance_size */
    0,                    /* n_preallocs */
    NULL,                 /* instance_init */
    NULL,		  /* value_table */
  };
  xtype_t type;

  xreturn_val_if_fail (name != NULL, 0);
  xreturn_val_if_fail (const_static_values != NULL, 0);

  flags_type_info.class_data = const_static_values;

  type = xtype_register_static (XTYPE_FLAGS, name, &flags_type_info, 0);

  return type;
}

/**
 * xenum_complete_type_info:
 * @xenum_type: the type identifier of the type being completed
 * @info: (out callee-allocates): the #xtype_info_t struct to be filled in
 * @const_values: An array of #xenum_value_t structs for the possible
 *  enumeration values. The array is terminated by a struct with all
 *  members being 0.
 *
 * This function is meant to be called from the `complete_type_info`
 * function of a #GTypePlugin implementation, as in the following
 * example:
 *
 * |[<!-- language="C" -->
 * static void
 * my_enum_complete_type_info (GTypePlugin     *plugin,
 *                             xtype_t            g_type,
 *                             xtype_info_t       *info,
 *                             xtype_value_table_t *value_table)
 * {
 *   static const xenum_value_t values[] = {
 *     { MY_ENUM_FOO, "MY_ENUM_FOO", "foo" },
 *     { MY_ENUM_BAR, "MY_ENUM_BAR", "bar" },
 *     { 0, NULL, NULL }
 *   };
 *
 *   xenum_complete_type_info (type, info, values);
 * }
 * ]|
 */
void
xenum_complete_type_info (xtype_t	     xenum_type,
			   xtype_info_t	    *info,
			   const xenum_value_t *const_values)
{
  g_return_if_fail (XTYPE_IS_ENUM (xenum_type));
  g_return_if_fail (info != NULL);
  g_return_if_fail (const_values != NULL);

  info->class_size = sizeof (xenum_class_t);
  info->base_init = NULL;
  info->base_finalize = NULL;
  info->class_init = (xclass_init_func_t) xenum_class_init;
  info->class_finalize = NULL;
  info->class_data = const_values;
}

/**
 * xflags_complete_type_info:
 * @xflags_type: the type identifier of the type being completed
 * @info: (out callee-allocates): the #xtype_info_t struct to be filled in
 * @const_values: An array of #xflags_value_t structs for the possible
 *  enumeration values. The array is terminated by a struct with all
 *  members being 0.
 *
 * This function is meant to be called from the complete_type_info()
 * function of a #GTypePlugin implementation, see the example for
 * xenum_complete_type_info() above.
 */
void
xflags_complete_type_info (xtype_t	       xflags_type,
			    xtype_info_t	      *info,
			    const xflags_value_t *const_values)
{
  g_return_if_fail (XTYPE_IS_FLAGS (xflags_type));
  g_return_if_fail (info != NULL);
  g_return_if_fail (const_values != NULL);

  info->class_size = sizeof (xflags_class_t);
  info->base_init = NULL;
  info->base_finalize = NULL;
  info->class_init = (xclass_init_func_t) xflags_class_init;
  info->class_finalize = NULL;
  info->class_data = const_values;
}

static void
xenum_class_init (xenum_class_t *class,
		   xpointer_t    class_data)
{
  g_return_if_fail (X_IS_ENUM_CLASS (class));

  class->minimum = 0;
  class->maximum = 0;
  class->n_values = 0;
  class->values = class_data;

  if (class->values)
    {
      xenum_value_t *values;

      class->minimum = class->values->value;
      class->maximum = class->values->value;
      for (values = class->values; values->value_name; values++)
	{
	  class->minimum = MIN (class->minimum, values->value);
	  class->maximum = MAX (class->maximum, values->value);
	  class->n_values++;
	}
    }
}

static void
xflags_class_init (xflags_class_t *class,
		    xpointer_t	 class_data)
{
  g_return_if_fail (X_IS_FLAGS_CLASS (class));

  class->mask = 0;
  class->n_values = 0;
  class->values = class_data;

  if (class->values)
    {
      xflags_value_t *values;

      for (values = class->values; values->value_name; values++)
	{
	  class->mask |= values->value;
	  class->n_values++;
	}
    }
}

/**
 * xenum_get_value_by_name:
 * @enum_class: a #xenum_class_t
 * @name: the name to look up
 *
 * Looks up a #xenum_value_t by name.
 *
 * Returns: (transfer none) (nullable): the #xenum_value_t with name @name,
 *          or %NULL if the enumeration doesn't have a member
 *          with that name
 */
xenum_value_t*
xenum_get_value_by_name (xenum_class_t  *enum_class,
			  const xchar_t *name)
{
  xreturn_val_if_fail (X_IS_ENUM_CLASS (enum_class), NULL);
  xreturn_val_if_fail (name != NULL, NULL);

  if (enum_class->n_values)
    {
      xenum_value_t *enum_value;

      for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
	if (strcmp (name, enum_value->value_name) == 0)
	  return enum_value;
    }

  return NULL;
}

/**
 * xflags_get_value_by_name:
 * @flags_class: a #xflags_class_t
 * @name: the name to look up
 *
 * Looks up a #xflags_value_t by name.
 *
 * Returns: (transfer none) (nullable): the #xflags_value_t with name @name,
 *          or %NULL if there is no flag with that name
 */
xflags_value_t*
xflags_get_value_by_name (xflags_class_t *flags_class,
			   const xchar_t *name)
{
  xreturn_val_if_fail (X_IS_FLAGS_CLASS (flags_class), NULL);
  xreturn_val_if_fail (name != NULL, NULL);

  if (flags_class->n_values)
    {
      xflags_value_t *flags_value;

      for (flags_value = flags_class->values; flags_value->value_name; flags_value++)
	if (strcmp (name, flags_value->value_name) == 0)
	  return flags_value;
    }

  return NULL;
}

/**
 * xenum_get_value_by_nick:
 * @enum_class: a #xenum_class_t
 * @nick: the nickname to look up
 *
 * Looks up a #xenum_value_t by nickname.
 *
 * Returns: (transfer none) (nullable): the #xenum_value_t with nickname @nick,
 *          or %NULL if the enumeration doesn't have a member
 *          with that nickname
 */
xenum_value_t*
xenum_get_value_by_nick (xenum_class_t  *enum_class,
			  const xchar_t *nick)
{
  xreturn_val_if_fail (X_IS_ENUM_CLASS (enum_class), NULL);
  xreturn_val_if_fail (nick != NULL, NULL);

  if (enum_class->n_values)
    {
      xenum_value_t *enum_value;

      for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
	if (enum_value->value_nick && strcmp (nick, enum_value->value_nick) == 0)
	  return enum_value;
    }

  return NULL;
}

/**
 * xflags_get_value_by_nick:
 * @flags_class: a #xflags_class_t
 * @nick: the nickname to look up
 *
 * Looks up a #xflags_value_t by nickname.
 *
 * Returns: (transfer none) (nullable): the #xflags_value_t with nickname @nick,
 *          or %NULL if there is no flag with that nickname
 */
xflags_value_t*
xflags_get_value_by_nick (xflags_class_t *flags_class,
			   const xchar_t *nick)
{
  xreturn_val_if_fail (X_IS_FLAGS_CLASS (flags_class), NULL);
  xreturn_val_if_fail (nick != NULL, NULL);

  if (flags_class->n_values)
    {
      xflags_value_t *flags_value;

      for (flags_value = flags_class->values; flags_value->value_nick; flags_value++)
	if (flags_value->value_nick && strcmp (nick, flags_value->value_nick) == 0)
	  return flags_value;
    }

  return NULL;
}

/**
 * xenum_get_value:
 * @enum_class: a #xenum_class_t
 * @value: the value to look up
 *
 * Returns the #xenum_value_t for a value.
 *
 * Returns: (transfer none) (nullable): the #xenum_value_t for @value, or %NULL
 *          if @value is not a member of the enumeration
 */
xenum_value_t*
xenum_get_value (xenum_class_t *enum_class,
		  xint_t	      value)
{
  xreturn_val_if_fail (X_IS_ENUM_CLASS (enum_class), NULL);

  if (enum_class->n_values)
    {
      xenum_value_t *enum_value;

      for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
	if (enum_value->value == value)
	  return enum_value;
    }

  return NULL;
}

/**
 * xflags_get_first_value:
 * @flags_class: a #xflags_class_t
 * @value: the value
 *
 * Returns the first #xflags_value_t which is set in @value.
 *
 * Returns: (transfer none) (nullable): the first #xflags_value_t which is set in
 *          @value, or %NULL if none is set
 */
xflags_value_t*
xflags_get_first_value (xflags_class_t *flags_class,
			 xuint_t	      value)
{
  xreturn_val_if_fail (X_IS_FLAGS_CLASS (flags_class), NULL);

  if (flags_class->n_values)
    {
      xflags_value_t *flags_value;

      if (value == 0)
        {
          for (flags_value = flags_class->values; flags_value->value_name; flags_value++)
            if (flags_value->value == 0)
              return flags_value;
        }
      else
        {
          for (flags_value = flags_class->values; flags_value->value_name; flags_value++)
            if (flags_value->value != 0 && (flags_value->value & value) == flags_value->value)
              return flags_value;
        }
    }

  return NULL;
}

/**
 * xenum_to_string:
 * @xenum_type: the type identifier of a #xenum_class_t type
 * @value: the value
 *
 * Pretty-prints @value in the form of the enum’s name.
 *
 * This is intended to be used for debugging purposes. The format of the output
 * may change in the future.
 *
 * Returns: (transfer full): a newly-allocated text string
 *
 * Since: 2.54
 */
xchar_t *
xenum_to_string (xtype_t xenum_type,
                  xint_t  value)
{
  xchar_t *result;
  xenum_class_t *enum_class;
  xenum_value_t *enum_value;

  xreturn_val_if_fail (XTYPE_IS_ENUM (xenum_type), NULL);

  enum_class = xtype_class_ref (xenum_type);

  /* Already warned */
  if (enum_class == NULL)
    return xstrdup_printf ("%d", value);

  enum_value = xenum_get_value (enum_class, value);

  if (enum_value == NULL)
    result = xstrdup_printf ("%d", value);
  else
    result = xstrdup (enum_value->value_name);

  xtype_class_unref (enum_class);
  return result;
}

/*
 * xflags_get_value_string:
 * @flags_class: a #xflags_class_t
 * @value: the value
 *
 * Pretty-prints @value in the form of the flag names separated by ` | ` and
 * sorted. Any extra bits will be shown at the end as a hexadecimal number.
 *
 * This is intended to be used for debugging purposes. The format of the output
 * may change in the future.
 *
 * Returns: (transfer full): a newly-allocated text string
 *
 * Since: 2.54
 */
static xchar_t *
xflags_get_value_string (xflags_class_t *flags_class,
                          xuint_t        value)
{
  xstring_t *str;
  xflags_value_t *flags_value;

  xreturn_val_if_fail (X_IS_FLAGS_CLASS (flags_class), NULL);

  str = xstring_new (NULL);

  while ((str->len == 0 || value != 0) &&
         (flags_value = xflags_get_first_value (flags_class, value)) != NULL)
    {
      if (str->len > 0)
        xstring_append (str, " | ");

      xstring_append (str, flags_value->value_name);

      value &= ~flags_value->value;
    }

  /* Show the extra bits */
  if (value != 0 || str->len == 0)
    {
      if (str->len > 0)
        xstring_append (str, " | ");

      xstring_append_printf (str, "0x%x", value);
    }

  return xstring_free (str, FALSE);
}

/**
 * xflags_to_string:
 * @flags_type: the type identifier of a #xflags_class_t type
 * @value: the value
 *
 * Pretty-prints @value in the form of the flag names separated by ` | ` and
 * sorted. Any extra bits will be shown at the end as a hexadecimal number.
 *
 * This is intended to be used for debugging purposes. The format of the output
 * may change in the future.
 *
 * Returns: (transfer full): a newly-allocated text string
 *
 * Since: 2.54
 */
xchar_t *
xflags_to_string (xtype_t flags_type,
                   xuint_t value)
{
  xchar_t *result;
  xflags_class_t *flags_class;

  xreturn_val_if_fail (XTYPE_IS_FLAGS (flags_type), NULL);

  flags_class = xtype_class_ref (flags_type);

  /* Already warned */
  if (flags_class == NULL)
    return NULL;

  result = xflags_get_value_string (flags_class, value);

  xtype_class_unref (flags_class);
  return result;
}


/**
 * xvalue_set_enum:
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_ENUM
 * @v_enum: enum value to be set
 *
 * Set the contents of a %XTYPE_ENUM #xvalue_t to @v_enum.
 */
void
xvalue_set_enum (xvalue_t *value,
		  xint_t    v_enum)
{
  g_return_if_fail (G_VALUE_HOLDS_ENUM (value));

  value->data[0].v_long = v_enum;
}

/**
 * xvalue_get_enum:
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_ENUM
 *
 * Get the contents of a %XTYPE_ENUM #xvalue_t.
 *
 * Returns: enum contents of @value
 */
xint_t
xvalue_get_enum (const xvalue_t *value)
{
  xreturn_val_if_fail (G_VALUE_HOLDS_ENUM (value), 0);

  return value->data[0].v_long;
}

/**
 * xvalue_set_flags:
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_FLAGS
 * @v_flags: flags value to be set
 *
 * Set the contents of a %XTYPE_FLAGS #xvalue_t to @v_flags.
 */
void
xvalue_set_flags (xvalue_t *value,
		   xuint_t   v_flags)
{
  g_return_if_fail (G_VALUE_HOLDS_FLAGS (value));

  value->data[0].v_ulong = v_flags;
}

/**
 * xvalue_get_flags:
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_FLAGS
 *
 * Get the contents of a %XTYPE_FLAGS #xvalue_t.
 *
 * Returns: flags contents of @value
 */
xuint_t
xvalue_get_flags (const xvalue_t *value)
{
  xreturn_val_if_fail (G_VALUE_HOLDS_FLAGS (value), 0);

  return value->data[0].v_ulong;
}
