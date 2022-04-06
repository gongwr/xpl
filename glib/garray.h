/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_ARRAY_H__
#define __G_ARRAY_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef struct _GBytes          xbytes_t;
typedef struct _GArray		xarray_t;
typedef struct _GByteArray	xbyte_array_t;
typedef struct _GPtrArray	xptr_array_t;

struct _GArray
{
  xchar_t *data;
  xuint_t len;
};

struct _GByteArray
{
  xuint8_t *data;
  xuint_t	  len;
};

struct _GPtrArray
{
  xpointer_t *pdata;
  xuint_t	    len;
};

/* Resizable arrays. remove fills any cleared spot and shortens the
 * array, while preserving the order. remove_fast will distort the
 * order by moving the last element to the position of the removed.
 */

#define g_array_append_val(a,v)	  g_array_append_vals (a, &(v), 1)
#define g_array_prepend_val(a,v)  g_array_prepend_vals (a, &(v), 1)
#define g_array_insert_val(a,i,v) g_array_insert_vals (a, i, &(v), 1)
#define g_array_index(a,t,i)      (((t*) (void *) (a)->data) [(i)])

XPL_AVAILABLE_IN_ALL
xarray_t* g_array_new               (xboolean_t          zero_terminated,
				   xboolean_t          clear_,
				   xuint_t             element_size);
XPL_AVAILABLE_IN_2_64
xpointer_t g_array_steal            (xarray_t           *array,
                                   xsize_t            *len);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_sized_new         (xboolean_t          zero_terminated,
				   xboolean_t          clear_,
				   xuint_t             element_size,
				   xuint_t             reserved_size);
XPL_AVAILABLE_IN_2_62
xarray_t* g_array_copy              (xarray_t           *array);
XPL_AVAILABLE_IN_ALL
xchar_t*  g_array_free              (xarray_t           *array,
				   xboolean_t          free_segment);
XPL_AVAILABLE_IN_ALL
xarray_t *g_array_ref               (xarray_t           *array);
XPL_AVAILABLE_IN_ALL
void    g_array_unref             (xarray_t           *array);
XPL_AVAILABLE_IN_ALL
xuint_t   g_array_get_element_size  (xarray_t           *array);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_append_vals       (xarray_t           *array,
				   xconstpointer     data,
				   xuint_t             len);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_prepend_vals      (xarray_t           *array,
				   xconstpointer     data,
				   xuint_t             len);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_insert_vals       (xarray_t           *array,
				   xuint_t             index_,
				   xconstpointer     data,
				   xuint_t             len);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_set_size          (xarray_t           *array,
				   xuint_t             length);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_remove_index      (xarray_t           *array,
				   xuint_t             index_);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_remove_index_fast (xarray_t           *array,
				   xuint_t             index_);
XPL_AVAILABLE_IN_ALL
xarray_t* g_array_remove_range      (xarray_t           *array,
				   xuint_t             index_,
				   xuint_t             length);
XPL_AVAILABLE_IN_ALL
void    g_array_sort              (xarray_t           *array,
				   GCompareFunc      compare_func);
XPL_AVAILABLE_IN_ALL
void    g_array_sort_with_data    (xarray_t           *array,
				   GCompareDataFunc  compare_func,
				   xpointer_t          user_data);
XPL_AVAILABLE_IN_2_62
xboolean_t g_array_binary_search    (xarray_t           *array,
                                   xconstpointer     target,
                                   GCompareFunc      compare_func,
                                   xuint_t            *out_match_index);
XPL_AVAILABLE_IN_ALL
void    g_array_set_clear_func    (xarray_t           *array,
                                   xdestroy_notify_t    clear_func);

/* Resizable pointer array.  This interface is much less complicated
 * than the above.  Add appends a pointer.  Remove fills any cleared
 * spot and shortens the array. remove_fast will again distort order.
 */
#define    xptr_array_index(array,index_) ((array)->pdata)[index_]
XPL_AVAILABLE_IN_ALL
xptr_array_t* xptr_array_new                (void);
XPL_AVAILABLE_IN_ALL
xptr_array_t* xptr_array_new_with_free_func (xdestroy_notify_t    element_free_func);
XPL_AVAILABLE_IN_2_64
xpointer_t*   xptr_array_steal              (xptr_array_t        *array,
                                            xsize_t            *len);
XPL_AVAILABLE_IN_2_62
xptr_array_t *xptr_array_copy               (xptr_array_t        *array,
                                           GCopyFunc         func,
                                           xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
xptr_array_t* xptr_array_sized_new          (xuint_t             reserved_size);
XPL_AVAILABLE_IN_ALL
xptr_array_t* xptr_array_new_full           (xuint_t             reserved_size,
					   xdestroy_notify_t    element_free_func);
XPL_AVAILABLE_IN_ALL
xpointer_t*  xptr_array_free               (xptr_array_t        *array,
					   xboolean_t          free_seg);
XPL_AVAILABLE_IN_ALL
xptr_array_t* xptr_array_ref                (xptr_array_t        *array);
XPL_AVAILABLE_IN_ALL
void       xptr_array_unref              (xptr_array_t        *array);
XPL_AVAILABLE_IN_ALL
void       xptr_array_set_free_func      (xptr_array_t        *array,
                                           xdestroy_notify_t    element_free_func);
XPL_AVAILABLE_IN_ALL
void       xptr_array_set_size           (xptr_array_t        *array,
					   xint_t              length);
XPL_AVAILABLE_IN_ALL
xpointer_t   xptr_array_remove_index       (xptr_array_t        *array,
					   xuint_t             index_);
XPL_AVAILABLE_IN_ALL
xpointer_t   xptr_array_remove_index_fast  (xptr_array_t        *array,
					   xuint_t             index_);
XPL_AVAILABLE_IN_2_58
xpointer_t   xptr_array_steal_index        (xptr_array_t        *array,
                                           xuint_t             index_);
XPL_AVAILABLE_IN_2_58
xpointer_t   xptr_array_steal_index_fast   (xptr_array_t        *array,
                                           xuint_t             index_);
XPL_AVAILABLE_IN_ALL
xboolean_t   xptr_array_remove             (xptr_array_t        *array,
					   xpointer_t          data);
XPL_AVAILABLE_IN_ALL
xboolean_t   xptr_array_remove_fast        (xptr_array_t        *array,
					   xpointer_t          data);
XPL_AVAILABLE_IN_ALL
xptr_array_t *xptr_array_remove_range       (xptr_array_t        *array,
					   xuint_t             index_,
					   xuint_t             length);
XPL_AVAILABLE_IN_ALL
void       xptr_array_add                (xptr_array_t        *array,
					   xpointer_t          data);
XPL_AVAILABLE_IN_2_62
void xptr_array_extend                   (xptr_array_t        *array_to_extend,
                                           xptr_array_t        *array,
                                           GCopyFunc         func,
                                           xpointer_t          user_data);
XPL_AVAILABLE_IN_2_62
void xptr_array_extend_and_steal         (xptr_array_t        *array_to_extend,
                                           xptr_array_t        *array);
XPL_AVAILABLE_IN_2_40
void       xptr_array_insert             (xptr_array_t        *array,
                                           xint_t              index_,
                                           xpointer_t          data);
XPL_AVAILABLE_IN_ALL
void       xptr_array_sort               (xptr_array_t        *array,
					   GCompareFunc      compare_func);
XPL_AVAILABLE_IN_ALL
void       xptr_array_sort_with_data     (xptr_array_t        *array,
					   GCompareDataFunc  compare_func,
					   xpointer_t          user_data);
XPL_AVAILABLE_IN_ALL
void       xptr_array_foreach            (xptr_array_t        *array,
					   GFunc             func,
					   xpointer_t          user_data);
XPL_AVAILABLE_IN_2_54
xboolean_t   xptr_array_find               (xptr_array_t        *haystack,
                                           xconstpointer     needle,
                                           xuint_t            *index_);
XPL_AVAILABLE_IN_2_54
xboolean_t   xptr_array_find_with_equal_func (xptr_array_t     *haystack,
                                             xconstpointer  needle,
                                             GEqualFunc     equal_func,
                                             xuint_t         *index_);


/* Byte arrays, an array of xuint8_t.  Implemented as a xarray_t,
 * but type-safe.
 */

XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_new               (void);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_new_take          (xuint8_t           *data,
                                            xsize_t             len);
XPL_AVAILABLE_IN_2_64
xuint8_t*     xbyte_array_steal             (xbyte_array_t       *array,
                                            xsize_t            *len);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_sized_new         (xuint_t             reserved_size);
XPL_AVAILABLE_IN_ALL
xuint8_t*     xbyte_array_free              (xbyte_array_t       *array,
					    xboolean_t          free_segment);
XPL_AVAILABLE_IN_ALL
xbytes_t*     xbyte_array_free_to_bytes     (xbyte_array_t       *array);
XPL_AVAILABLE_IN_ALL
xbyte_array_t *xbyte_array_ref               (xbyte_array_t       *array);
XPL_AVAILABLE_IN_ALL
void        xbyte_array_unref             (xbyte_array_t       *array);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_append            (xbyte_array_t       *array,
					    const xuint8_t     *data,
					    xuint_t             len);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_prepend           (xbyte_array_t       *array,
					    const xuint8_t     *data,
					    xuint_t             len);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_set_size          (xbyte_array_t       *array,
					    xuint_t             length);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_remove_index      (xbyte_array_t       *array,
					    xuint_t             index_);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_remove_index_fast (xbyte_array_t       *array,
					    xuint_t             index_);
XPL_AVAILABLE_IN_ALL
xbyte_array_t* xbyte_array_remove_range      (xbyte_array_t       *array,
					    xuint_t             index_,
					    xuint_t             length);
XPL_AVAILABLE_IN_ALL
void        xbyte_array_sort              (xbyte_array_t       *array,
					    GCompareFunc      compare_func);
XPL_AVAILABLE_IN_ALL
void        xbyte_array_sort_with_data    (xbyte_array_t       *array,
					    GCompareDataFunc  compare_func,
					    xpointer_t          user_data);

G_END_DECLS

#endif /* __G_ARRAY_H__ */
