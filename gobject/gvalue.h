/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
 * gvalue.h: generic xvalue_t functions
 */
#ifndef __G_VALUE_H__
#define __G_VALUE_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gtype.h>

G_BEGIN_DECLS

/* --- type macros --- */
/**
 * XTYPE_IS_VALUE:
 * @type: A #xtype_t value.
 *
 * Checks whether the passed in type ID can be used for xvalue_init().
 *
 * That is, this macro checks whether this type provides an implementation
 * of the #xtype_value_table_t functions required for a type to create a #xvalue_t of.
 *
 * Returns: Whether @type is suitable as a #xvalue_t type.
 */
#define	XTYPE_IS_VALUE(type)		(xtype_check_is_value_type (type))
/**
 * X_IS_VALUE:
 * @value: A #xvalue_t structure.
 *
 * Checks if @value is a valid and initialized #xvalue_t structure.
 *
 * Returns: %TRUE on success.
 */
#define	X_IS_VALUE(value)		(XTYPE_CHECK_VALUE (value))
/**
 * G_VALUE_TYPE:
 * @value: A #xvalue_t structure.
 *
 * Get the type identifier of @value.
 *
 * Returns: the #xtype_t.
 */
#define	G_VALUE_TYPE(value)		(((xvalue_t*) (value))->g_type)
/**
 * G_VALUE_TYPE_NAME:
 * @value: A #xvalue_t structure.
 *
 * Gets the type name of @value.
 *
 * Returns: the type name.
 */
#define	G_VALUE_TYPE_NAME(value)	(xtype_name (G_VALUE_TYPE (value)))
/**
 * G_VALUE_HOLDS:
 * @value: A #xvalue_t structure.
 * @type: A #xtype_t value.
 *
 * Checks if @value holds (or contains) a value of @type.
 * This macro will also check for @value != %NULL and issue a
 * warning if the check fails.
 *
 * Returns: %TRUE if @value holds the @type.
 */
#define G_VALUE_HOLDS(value,type)	(XTYPE_CHECK_VALUE_TYPE ((value), (type)))


/* --- typedefs & structures --- */
/**
 * GValueTransform:
 * @src_value: Source value.
 * @dest_value: Target value.
 *
 * The type of value transformation functions which can be registered with
 * xvalue_register_transform_func().
 *
 * @dest_value will be initialized to the correct destination type.
 */
typedef void (*GValueTransform) (const xvalue_t *src_value,
				 xvalue_t       *dest_value);
/**
 * xvalue_t:
 *
 * An opaque structure used to hold different types of values.
 *
 * The data within the structure has protected scope: it is accessible only
 * to functions within a #xtype_value_table_t structure, or implementations of
 * the xvalue_*() API. That is, code portions which implement new fundamental
 * types.
 *
 * #xvalue_t users cannot make any assumptions about how data is stored
 * within the 2 element @data union, and the @g_type member should
 * only be accessed through the G_VALUE_TYPE() macro.
 */
struct _GValue
{
  /*< private >*/
  xtype_t		g_type;

  /* public for xtype_value_table_t methods */
  union {
    xint_t	v_int;
    xuint_t	v_uint;
    xlong_t	v_long;
    gulong	v_ulong;
    gint64      v_int64;
    xuint64_t     v_uint64;
    gfloat	v_float;
    xdouble_t	v_double;
    xpointer_t	v_pointer;
  } data[2];
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xvalue_t*         xvalue_init	   	(xvalue_t       *value,
					 xtype_t         g_type);
XPL_AVAILABLE_IN_ALL
void            xvalue_copy    	(const xvalue_t *src_value,
					 xvalue_t       *dest_value);
XPL_AVAILABLE_IN_ALL
xvalue_t*         xvalue_reset   	(xvalue_t       *value);
XPL_AVAILABLE_IN_ALL
void            xvalue_unset   	(xvalue_t       *value);
XPL_AVAILABLE_IN_ALL
void		xvalue_set_instance	(xvalue_t	      *value,
					 xpointer_t      instance);
XPL_AVAILABLE_IN_2_42
void            xvalue_init_from_instance   (xvalue_t       *value,
                                              xpointer_t      instance);


/* --- private --- */
XPL_AVAILABLE_IN_ALL
xboolean_t	xvalue_fits_pointer	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
xpointer_t	xvalue_peek_pointer	(const xvalue_t *value);


/* --- implementation details --- */
XPL_AVAILABLE_IN_ALL
xboolean_t xvalue_type_compatible	(xtype_t		 src_type,
					 xtype_t		 dest_type);
XPL_AVAILABLE_IN_ALL
xboolean_t xvalue_type_transformable	(xtype_t           src_type,
					 xtype_t           dest_type);
XPL_AVAILABLE_IN_ALL
xboolean_t xvalue_transform		(const xvalue_t   *src_value,
					 xvalue_t         *dest_value);
XPL_AVAILABLE_IN_ALL
void	xvalue_register_transform_func	(xtype_t		 src_type,
					 xtype_t		 dest_type,
					 GValueTransform transform_func);

/**
 * G_VALUE_NOCOPY_CONTENTS:
 *
 * If passed to G_VALUE_COLLECT(), allocated data won't be copied
 * but used verbatim. This does not affect ref-counted types like
 * objects. This does not affect usage of xvalue_copy(), the data will
 * be copied if it is not ref-counted.
 */
#define G_VALUE_NOCOPY_CONTENTS (1 << 27)

/**
 * G_VALUE_INTERNED_STRING:
 *
 * For string values, indicates that the string contained is canonical and will
 * exist for the duration of the process. See xvalue_set_interned_string().
 *
 * Since: 2.66
 */
#define G_VALUE_INTERNED_STRING (1 << 28) XPL_AVAILABLE_MACRO_IN_2_66

/**
 * G_VALUE_INIT:
 *
 * A #xvalue_t must be initialized before it can be used. This macro can
 * be used as initializer instead of an explicit `{ 0 }` when declaring
 * a variable, but it cannot be assigned to a variable.
 *
 * |[<!-- language="C" -->
 *   xvalue_t value = G_VALUE_INIT;
 * ]|
 *
 * Since: 2.30
 */
#define G_VALUE_INIT  { 0, { { 0 } } }


G_END_DECLS

#endif /* __G_VALUE_H__ */
