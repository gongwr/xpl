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
 * gvalue.h: generic GValue functions
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
 * Checks whether the passed in type ID can be used for g_value_init().
 *
 * That is, this macro checks whether this type provides an implementation
 * of the #GTypeValueTable functions required for a type to create a #GValue of.
 *
 * Returns: Whether @type is suitable as a #GValue type.
 */
#define	XTYPE_IS_VALUE(type)		(g_type_check_is_value_type (type))
/**
 * X_IS_VALUE:
 * @value: A #GValue structure.
 *
 * Checks if @value is a valid and initialized #GValue structure.
 *
 * Returns: %TRUE on success.
 */
#define	X_IS_VALUE(value)		(XTYPE_CHECK_VALUE (value))
/**
 * G_VALUE_TYPE:
 * @value: A #GValue structure.
 *
 * Get the type identifier of @value.
 *
 * Returns: the #xtype_t.
 */
#define	G_VALUE_TYPE(value)		(((GValue*) (value))->g_type)
/**
 * G_VALUE_TYPE_NAME:
 * @value: A #GValue structure.
 *
 * Gets the type name of @value.
 *
 * Returns: the type name.
 */
#define	G_VALUE_TYPE_NAME(value)	(g_type_name (G_VALUE_TYPE (value)))
/**
 * G_VALUE_HOLDS:
 * @value: A #GValue structure.
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
 * g_value_register_transform_func().
 *
 * @dest_value will be initialized to the correct destination type.
 */
typedef void (*GValueTransform) (const GValue *src_value,
				 GValue       *dest_value);
/**
 * GValue:
 *
 * An opaque structure used to hold different types of values.
 *
 * The data within the structure has protected scope: it is accessible only
 * to functions within a #GTypeValueTable structure, or implementations of
 * the g_value_*() API. That is, code portions which implement new fundamental
 * types.
 *
 * #GValue users cannot make any assumptions about how data is stored
 * within the 2 element @data union, and the @g_type member should
 * only be accessed through the G_VALUE_TYPE() macro.
 */
struct _GValue
{
  /*< private >*/
  xtype_t		g_type;

  /* public for GTypeValueTable methods */
  union {
    xint_t	v_int;
    xuint_t	v_uint;
    glong	v_long;
    gulong	v_ulong;
    gint64      v_int64;
    guint64     v_uint64;
    gfloat	v_float;
    xdouble_t	v_double;
    xpointer_t	v_pointer;
  } data[2];
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
GValue*         g_value_init	   	(GValue       *value,
					 xtype_t         g_type);
XPL_AVAILABLE_IN_ALL
void            g_value_copy    	(const GValue *src_value,
					 GValue       *dest_value);
XPL_AVAILABLE_IN_ALL
GValue*         g_value_reset   	(GValue       *value);
XPL_AVAILABLE_IN_ALL
void            g_value_unset   	(GValue       *value);
XPL_AVAILABLE_IN_ALL
void		g_value_set_instance	(GValue	      *value,
					 xpointer_t      instance);
XPL_AVAILABLE_IN_2_42
void            g_value_init_from_instance   (GValue       *value,
                                              xpointer_t      instance);


/* --- private --- */
XPL_AVAILABLE_IN_ALL
xboolean_t	g_value_fits_pointer	(const GValue *value);
XPL_AVAILABLE_IN_ALL
xpointer_t	g_value_peek_pointer	(const GValue *value);


/* --- implementation details --- */
XPL_AVAILABLE_IN_ALL
xboolean_t g_value_type_compatible	(xtype_t		 src_type,
					 xtype_t		 dest_type);
XPL_AVAILABLE_IN_ALL
xboolean_t g_value_type_transformable	(xtype_t           src_type,
					 xtype_t           dest_type);
XPL_AVAILABLE_IN_ALL
xboolean_t g_value_transform		(const GValue   *src_value,
					 GValue         *dest_value);
XPL_AVAILABLE_IN_ALL
void	g_value_register_transform_func	(xtype_t		 src_type,
					 xtype_t		 dest_type,
					 GValueTransform transform_func);

/**
 * G_VALUE_NOCOPY_CONTENTS:
 *
 * If passed to G_VALUE_COLLECT(), allocated data won't be copied
 * but used verbatim. This does not affect ref-counted types like
 * objects. This does not affect usage of g_value_copy(), the data will
 * be copied if it is not ref-counted.
 */
#define G_VALUE_NOCOPY_CONTENTS (1 << 27)

/**
 * G_VALUE_INTERNED_STRING:
 *
 * For string values, indicates that the string contained is canonical and will
 * exist for the duration of the process. See g_value_set_interned_string().
 *
 * Since: 2.66
 */
#define G_VALUE_INTERNED_STRING (1 << 28) XPL_AVAILABLE_MACRO_IN_2_66

/**
 * G_VALUE_INIT:
 *
 * A #GValue must be initialized before it can be used. This macro can
 * be used as initializer instead of an explicit `{ 0 }` when declaring
 * a variable, but it cannot be assigned to a variable.
 *
 * |[<!-- language="C" -->
 *   GValue value = G_VALUE_INIT;
 * ]|
 *
 * Since: 2.30
 */
#define G_VALUE_INIT  { 0, { { 0 } } }


G_END_DECLS

#endif /* __G_VALUE_H__ */
