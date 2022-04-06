/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
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
#ifndef __G_BOXED_H__
#define __G_BOXED_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include        <gobject/gtype.h>

#ifndef __GI_SCANNER__
#include        <gobject/glib-types.h>
#endif

G_BEGIN_DECLS

/* --- type macros --- */
#define XTYPE_IS_BOXED(type)      (XTYPE_FUNDAMENTAL (type) == XTYPE_BOXED)
/**
 * G_VALUE_HOLDS_BOXED:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values derived
 * from type %XTYPE_BOXED.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_BOXED(value) (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_BOXED))


/* --- typedefs --- */
/**
 * GBoxedCopyFunc:
 * @boxed: (not nullable): The boxed structure to be copied.
 *
 * This function is provided by the user and should produce a copy
 * of the passed in boxed structure.
 *
 * Returns: (not nullable): The newly created copy of the boxed structure.
 */
typedef xpointer_t (*GBoxedCopyFunc) (xpointer_t boxed);

/**
 * GBoxedFreeFunc:
 * @boxed: (not nullable): The boxed structure to be freed.
 *
 * This function is provided by the user and should free the boxed
 * structure passed.
 */
typedef void (*GBoxedFreeFunc) (xpointer_t boxed);


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xpointer_t xboxed_copy                     (xtype_t boxed_type,
                                           xconstpointer  src_boxed);
XPL_AVAILABLE_IN_ALL
void     xboxed_free                     (xtype_t          boxed_type,
                                           xpointer_t       boxed);
XPL_AVAILABLE_IN_ALL
void     xvalue_set_boxed                (xvalue_t        *value,
                                           xconstpointer  v_boxed);
XPL_AVAILABLE_IN_ALL
void     xvalue_set_static_boxed         (xvalue_t        *value,
                                           xconstpointer  v_boxed);
XPL_AVAILABLE_IN_ALL
void     xvalue_take_boxed               (xvalue_t        *value,
                                           xconstpointer  v_boxed);
XPL_DEPRECATED_FOR(xvalue_take_boxed)
void     xvalue_set_boxed_take_ownership (xvalue_t        *value,
                                           xconstpointer  v_boxed);
XPL_AVAILABLE_IN_ALL
xpointer_t xvalue_get_boxed                (const xvalue_t  *value);
XPL_AVAILABLE_IN_ALL
xpointer_t xvalue_dup_boxed                (const xvalue_t  *value);


/* --- convenience --- */
XPL_AVAILABLE_IN_ALL
xtype_t    xboxed_type_register_static     (const xchar_t   *name,
                                           GBoxedCopyFunc boxed_copy,
                                           GBoxedFreeFunc boxed_free);

/* --- xobject_t boxed types --- */
/**
 * XTYPE_CLOSURE:
 *
 * The #xtype_t for #xclosure_t.
 */
#define XTYPE_CLOSURE (xclosure_get_type ())

/**
 * XTYPE_VALUE:
 *
 * The type ID of the "xvalue_t" type which is a boxed type,
 * used to pass around pointers to GValues.
 */
#define XTYPE_VALUE (xvalue_get_type ())

XPL_AVAILABLE_IN_ALL
xtype_t   xclosure_get_type         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xvalue_get_type           (void) G_GNUC_CONST;

G_END_DECLS

#endif  /* __G_BOXED_H__ */
