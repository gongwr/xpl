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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * gvaluetypes.h: GLib default values
 */
#ifndef __G_VALUETYPES_H__
#define __G_VALUETYPES_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gvalue.h>

G_BEGIN_DECLS

/* --- type macros --- */
/**
 * G_VALUE_HOLDS_CHAR:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_CHAR.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_CHAR(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_CHAR))
/**
 * G_VALUE_HOLDS_UCHAR:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_UCHAR.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_UCHAR(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_UCHAR))
/**
 * G_VALUE_HOLDS_BOOLEAN:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_BOOLEAN.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_BOOLEAN(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_BOOLEAN))
/**
 * G_VALUE_HOLDS_INT:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_INT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_INT(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_INT))
/**
 * G_VALUE_HOLDS_UINT:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_UINT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_UINT(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_UINT))
/**
 * G_VALUE_HOLDS_LONG:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_LONG.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_LONG(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_LONG))
/**
 * G_VALUE_HOLDS_ULONG:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_ULONG.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_ULONG(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_ULONG))
/**
 * G_VALUE_HOLDS_INT64:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_INT64.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_INT64(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_INT64))
/**
 * G_VALUE_HOLDS_UINT64:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_UINT64.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_UINT64(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_UINT64))
/**
 * G_VALUE_HOLDS_FLOAT:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_FLOAT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_FLOAT(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_FLOAT))
/**
 * G_VALUE_HOLDS_DOUBLE:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_DOUBLE.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_DOUBLE(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_DOUBLE))
/**
 * G_VALUE_HOLDS_STRING:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_STRING.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_STRING(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_STRING))
/**
 * G_VALUE_IS_INTERNED_STRING:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether @value contains a string which is canonical.
 *
 * Returns: %TRUE if the value contains a string in its canonical
 * representation, as returned by g_intern_string(). See also
 * xvalue_set_interned_string().
 *
 * Since: 2.66
 */
#define G_VALUE_IS_INTERNED_STRING(value) (G_VALUE_HOLDS_STRING (value) && ((value)->data[1].v_uint & G_VALUE_INTERNED_STRING)) XPL_AVAILABLE_MACRO_IN_2_66
/**
 * G_VALUE_HOLDS_POINTER:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_POINTER.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_POINTER(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_POINTER))
/**
 * XTYPE_GTYPE:
 *
 * The type for #xtype_t.
 */
#define	XTYPE_GTYPE			 (g_gtype_get_type())
/**
 * G_VALUE_HOLDS_GTYPE:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_GTYPE.
 *
 * Since: 2.12
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_GTYPE(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_GTYPE))
/**
 * G_VALUE_HOLDS_VARIANT:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values of type %XTYPE_VARIANT.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.26
 */
#define G_VALUE_HOLDS_VARIANT(value)     (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_VARIANT))


/* --- prototypes --- */
XPL_DEPRECATED_IN_2_32_FOR(xvalue_set_schar)
void                  xvalue_set_char          (xvalue_t       *value,
                                                 xchar_t         v_char);
XPL_DEPRECATED_IN_2_32_FOR(xvalue_get_schar)
xchar_t                 xvalue_get_char          (const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_schar		(xvalue_t	      *value,
						 gint8	       v_char);
XPL_AVAILABLE_IN_ALL
gint8		      xvalue_get_schar		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_uchar		(xvalue_t	      *value,
						 guchar	       v_uchar);
XPL_AVAILABLE_IN_ALL
guchar		      xvalue_get_uchar		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_boolean	(xvalue_t	      *value,
						 xboolean_t      v_boolean);
XPL_AVAILABLE_IN_ALL
xboolean_t	      xvalue_get_boolean	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_int		(xvalue_t	      *value,
						 xint_t	       v_int);
XPL_AVAILABLE_IN_ALL
xint_t		      xvalue_get_int		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_uint		(xvalue_t	      *value,
						 xuint_t	       v_uint);
XPL_AVAILABLE_IN_ALL
xuint_t		      xvalue_get_uint		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_long		(xvalue_t	      *value,
						 xlong_t	       v_long);
XPL_AVAILABLE_IN_ALL
xlong_t		      xvalue_get_long		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_ulong		(xvalue_t	      *value,
						 gulong	       v_ulong);
XPL_AVAILABLE_IN_ALL
gulong		      xvalue_get_ulong		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_int64		(xvalue_t	      *value,
						 gint64	       v_int64);
XPL_AVAILABLE_IN_ALL
gint64		      xvalue_get_int64		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_uint64	(xvalue_t	      *value,
						 xuint64_t      v_uint64);
XPL_AVAILABLE_IN_ALL
xuint64_t		      xvalue_get_uint64	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_float		(xvalue_t	      *value,
						 gfloat	       v_float);
XPL_AVAILABLE_IN_ALL
gfloat		      xvalue_get_float		(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_double	(xvalue_t	      *value,
						 xdouble_t       v_double);
XPL_AVAILABLE_IN_ALL
xdouble_t		      xvalue_get_double	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_string	(xvalue_t	      *value,
						 const xchar_t  *v_string);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_static_string (xvalue_t	      *value,
						 const xchar_t  *v_string);
XPL_AVAILABLE_IN_2_66
void		      xvalue_set_interned_string (xvalue_t      *value,
						   const xchar_t  *v_string);
XPL_AVAILABLE_IN_ALL
const xchar_t *         xvalue_get_string	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
xchar_t*		      xvalue_dup_string	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_pointer	(xvalue_t	      *value,
						 xpointer_t      v_pointer);
XPL_AVAILABLE_IN_ALL
xpointer_t	      xvalue_get_pointer	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
xtype_t		      g_gtype_get_type		(void);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_gtype	        (xvalue_t	      *value,
						 xtype_t         v_gtype);
XPL_AVAILABLE_IN_ALL
xtype_t	              xvalue_get_gtype	        (const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
void		      xvalue_set_variant	(xvalue_t	      *value,
						 xvariant_t     *variant);
XPL_AVAILABLE_IN_ALL
void		      xvalue_take_variant	(xvalue_t	      *value,
						 xvariant_t     *variant);
XPL_AVAILABLE_IN_ALL
xvariant_t*	      xvalue_get_variant	(const xvalue_t *value);
XPL_AVAILABLE_IN_ALL
xvariant_t*	      xvalue_dup_variant	(const xvalue_t *value);


/* Convenience for registering new pointer types */
XPL_AVAILABLE_IN_ALL
xtype_t                 g_pointer_type_register_static (const xchar_t *name);

/* debugging aid, describe value contents as string */
XPL_AVAILABLE_IN_ALL
xchar_t*                xstrdup_value_contents   (const xvalue_t *value);


XPL_AVAILABLE_IN_ALL
void xvalue_take_string		        (xvalue_t		   *value,
						 xchar_t		   *v_string);
XPL_DEPRECATED_FOR(xvalue_take_string)
void xvalue_set_string_take_ownership          (xvalue_t            *value,
                                                 xchar_t             *v_string);


/* humpf, need a C representable type name for XTYPE_STRING */
/**
 * gchararray:
 *
 * A C representable type name for %XTYPE_STRING.
 */
typedef xchar_t* gchararray;


G_END_DECLS

#endif /* __G_VALUETYPES_H__ */
