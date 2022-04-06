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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * gvaluearray.h: GLib array type holding GValues
 */
#ifndef __G_VALUE_ARRAY_H__
#define __G_VALUE_ARRAY_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gvalue.h>

G_BEGIN_DECLS

/**
 * XTYPE_VALUE_ARRAY:
 *
 * The type ID of the "xvalue_array_t" type which is a boxed type,
 * used to pass around pointers to xvalue_array_ts.
 *
 * Deprecated: 2.32: Use #xarray_t instead of #xvalue_array_t
 */
#define XTYPE_VALUE_ARRAY (xvalue_array_get_type ()) XPL_DEPRECATED_MACRO_IN_2_32_FOR(XTYPE_ARRAY)

/* --- typedefs & structs --- */
typedef struct _xvalue_array_t xvalue_array_t;
/**
 * xvalue_array_t:
 * @n_values: number of values contained in the array
 * @values: array of values
 *
 * A #xvalue_array_t contains an array of #xvalue_t elements.
 */
struct _xvalue_array_t
{
  xuint_t   n_values;
  xvalue_t *values;

  /*< private >*/
  xuint_t   n_prealloced;
};

/* --- prototypes --- */
XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xtype_t           xvalue_array_get_type       (void) G_GNUC_CONST;

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_t*		xvalue_array_get_nth	     (xvalue_array_t	*value_array,
					      xuint_t		 index_);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_new	     (xuint_t		 n_prealloced);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
void		xvalue_array_free	     (xvalue_array_t	*value_array);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_copy	     (const xvalue_array_t *value_array);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_prepend	     (xvalue_array_t	*value_array,
					      const xvalue_t	*value);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_append	     (xvalue_array_t	*value_array,
					      const xvalue_t	*value);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_insert	     (xvalue_array_t	*value_array,
					      xuint_t		 index_,
					      const xvalue_t	*value);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_remove	     (xvalue_array_t	*value_array,
					      xuint_t		 index_);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_sort	     (xvalue_array_t	*value_array,
					      GCompareFunc	 compare_func);

XPL_DEPRECATED_IN_2_32_FOR(xarray_t)
xvalue_array_t*	xvalue_array_sort_with_data (xvalue_array_t	*value_array,
					      GCompareDataFunc	 compare_func,
					      xpointer_t		 user_data);


G_END_DECLS

#endif /* __G_VALUE_ARRAY_H__ */
