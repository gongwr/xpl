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
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_CHAR.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_CHAR(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_CHAR))
/**
 * G_VALUE_HOLDS_UCHAR:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_UCHAR.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_UCHAR(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_UCHAR))
/**
 * G_VALUE_HOLDS_BOOLEAN:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_BOOLEAN.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_BOOLEAN(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_BOOLEAN))
/**
 * G_VALUE_HOLDS_INT:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_INT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_INT(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_INT))
/**
 * G_VALUE_HOLDS_UINT:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_UINT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_UINT(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_UINT))
/**
 * G_VALUE_HOLDS_LONG:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_LONG.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_LONG(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_LONG))
/**
 * G_VALUE_HOLDS_ULONG:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_ULONG.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_ULONG(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_ULONG))
/**
 * G_VALUE_HOLDS_INT64:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_INT64.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_INT64(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_INT64))
/**
 * G_VALUE_HOLDS_UINT64:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_UINT64.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_UINT64(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_UINT64))
/**
 * G_VALUE_HOLDS_FLOAT:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_FLOAT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_FLOAT(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_FLOAT))
/**
 * G_VALUE_HOLDS_DOUBLE:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_DOUBLE.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_DOUBLE(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_DOUBLE))
/**
 * G_VALUE_HOLDS_STRING:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_STRING.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_STRING(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_STRING))
/**
 * G_VALUE_IS_INTERNED_STRING:
 * @value: a valid #GValue structure
 *
 * Checks whether @value contains a string which is canonical.
 *
 * Returns: %TRUE if the value contains a string in its canonical
 * representation, as returned by g_intern_string(). See also
 * g_value_set_interned_string().
 *
 * Since: 2.66
 */
#define G_VALUE_IS_INTERNED_STRING(value) (G_VALUE_HOLDS_STRING (value) && ((value)->data[1].v_uint & G_VALUE_INTERNED_STRING)) XPL_AVAILABLE_MACRO_IN_2_66
/**
 * G_VALUE_HOLDS_POINTER:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_POINTER.
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
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_GTYPE.
 *
 * Since: 2.12
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_GTYPE(value)	 (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_GTYPE))
/**
 * G_VALUE_HOLDS_VARIANT:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values of type %XTYPE_VARIANT.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.26
 */
#define G_VALUE_HOLDS_VARIANT(value)     (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_VARIANT))


/* --- prototypes --- */
XPL_DEPRECATED_IN_2_32_FOR(g_value_set_schar)
void                  g_value_set_char          (GValue       *value,
                                                 xchar_t         v_char);
XPL_DEPRECATED_IN_2_32_FOR(g_value_get_schar)
xchar_t                 g_value_get_char          (const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_schar		(GValue	      *value,
						 gint8	       v_char);
XPL_AVAILABLE_IN_ALL
gint8		      g_value_get_schar		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_uchar		(GValue	      *value,
						 guchar	       v_uchar);
XPL_AVAILABLE_IN_ALL
guchar		      g_value_get_uchar		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_boolean	(GValue	      *value,
						 xboolean_t      v_boolean);
XPL_AVAILABLE_IN_ALL
xboolean_t	      g_value_get_boolean	(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_int		(GValue	      *value,
						 xint_t	       v_int);
XPL_AVAILABLE_IN_ALL
xint_t		      g_value_get_int		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_uint		(GValue	      *value,
						 xuint_t	       v_uint);
XPL_AVAILABLE_IN_ALL
xuint_t		      g_value_get_uint		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_long		(GValue	      *value,
						 glong	       v_long);
XPL_AVAILABLE_IN_ALL
glong		      g_value_get_long		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_ulong		(GValue	      *value,
						 gulong	       v_ulong);
XPL_AVAILABLE_IN_ALL
gulong		      g_value_get_ulong		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_int64		(GValue	      *value,
						 gint64	       v_int64);
XPL_AVAILABLE_IN_ALL
gint64		      g_value_get_int64		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_uint64	(GValue	      *value,
						 guint64      v_uint64);
XPL_AVAILABLE_IN_ALL
guint64		      g_value_get_uint64	(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_float		(GValue	      *value,
						 gfloat	       v_float);
XPL_AVAILABLE_IN_ALL
gfloat		      g_value_get_float		(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_double	(GValue	      *value,
						 xdouble_t       v_double);
XPL_AVAILABLE_IN_ALL
xdouble_t		      g_value_get_double	(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_string	(GValue	      *value,
						 const xchar_t  *v_string);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_static_string (GValue	      *value,
						 const xchar_t  *v_string);
XPL_AVAILABLE_IN_2_66
void		      g_value_set_interned_string (GValue      *value,
						   const xchar_t  *v_string);
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_value_get_string	(const GValue *value);
XPL_AVAILABLE_IN_ALL
xchar_t*		      g_value_dup_string	(const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_pointer	(GValue	      *value,
						 xpointer_t      v_pointer);
XPL_AVAILABLE_IN_ALL
xpointer_t	      g_value_get_pointer	(const GValue *value);
XPL_AVAILABLE_IN_ALL
xtype_t		      g_gtype_get_type		(void);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_gtype	        (GValue	      *value,
						 xtype_t         v_gtype);
XPL_AVAILABLE_IN_ALL
xtype_t	              g_value_get_gtype	        (const GValue *value);
XPL_AVAILABLE_IN_ALL
void		      g_value_set_variant	(GValue	      *value,
						 xvariant_t     *variant);
XPL_AVAILABLE_IN_ALL
void		      g_value_take_variant	(GValue	      *value,
						 xvariant_t     *variant);
XPL_AVAILABLE_IN_ALL
xvariant_t*	      g_value_get_variant	(const GValue *value);
XPL_AVAILABLE_IN_ALL
xvariant_t*	      g_value_dup_variant	(const GValue *value);


/* Convenience for registering new pointer types */
XPL_AVAILABLE_IN_ALL
xtype_t                 g_pointer_type_register_static (const xchar_t *name);

/* debugging aid, describe value contents as string */
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strdup_value_contents   (const GValue *value);


XPL_AVAILABLE_IN_ALL
void g_value_take_string		        (GValue		   *value,
						 xchar_t		   *v_string);
XPL_DEPRECATED_FOR(g_value_take_string)
void g_value_set_string_take_ownership          (GValue            *value,
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
