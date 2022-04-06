/* gbinding.h: Binding for object properties
 *
 * Copyright (C) 2010  Intel Corp.
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
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
 */

#ifndef __XBINDING_H__
#define __XBINDING_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include <glib.h>
#include <gobject/gobject.h>

G_BEGIN_DECLS

#define XTYPE_BINDING_FLAGS    (xbinding_flags_get_type ())

#define XTYPE_BINDING          (xbinding_get_type ())
#define G_BINDING(obj)          (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_BINDING, xbinding_t))
#define X_IS_BINDING(obj)       (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_BINDING))

/**
 * xbinding_t:
 *
 * xbinding_t is an opaque structure whose members
 * cannot be accessed directly.
 *
 * Since: 2.26
 */
typedef struct _xbinding        xbinding_t;

/**
 * xbinding_transform_func:
 * @binding: a #xbinding_t
 * @from_value: the #xvalue_t containing the value to transform
 * @to_value: the #xvalue_t in which to store the transformed value
 * @user_data: data passed to the transform function
 *
 * A function to be called to transform @from_value to @to_value.
 *
 * If this is the @transform_to function of a binding, then @from_value
 * is the @source_property on the @source object, and @to_value is the
 * @target_property on the @target object. If this is the
 * @transform_from function of a %XBINDING_BIDIRECTIONAL binding,
 * then those roles are reversed.
 *
 * Returns: %TRUE if the transformation was successful, and %FALSE
 *   otherwise
 *
 * Since: 2.26
 */
typedef xboolean_t (* xbinding_transform_func) (xbinding_t     *binding,
                                            const xvalue_t *from_value,
                                            xvalue_t       *to_value,
                                            xpointer_t      user_data);

/**
 * xbinding_flags_t:
 * @XBINDING_DEFAULT: The default binding; if the source property
 *   changes, the target property is updated with its value.
 * @XBINDING_BIDIRECTIONAL: Bidirectional binding; if either the
 *   property of the source or the property of the target changes,
 *   the other is updated.
 * @XBINDING_SYNC_CREATE: Synchronize the values of the source and
 *   target properties when creating the binding; the direction of
 *   the synchronization is always from the source to the target.
 * @XBINDING_INVERT_BOOLEAN: If the two properties being bound are
 *   booleans, setting one to %TRUE will result in the other being
 *   set to %FALSE and vice versa. This flag will only work for
 *   boolean properties, and cannot be used when passing custom
 *   transformation functions to xobject_bind_property_full().
 *
 * Flags to be passed to xobject_bind_property() or
 * xobject_bind_property_full().
 *
 * This enumeration can be extended at later date.
 *
 * Since: 2.26
 */
typedef enum { /*< prefix=G_BINDING >*/
  XBINDING_DEFAULT        = 0,

  XBINDING_BIDIRECTIONAL  = 1 << 0,
  XBINDING_SYNC_CREATE    = 1 << 1,
  XBINDING_INVERT_BOOLEAN = 1 << 2
} xbinding_flags_t;

XPL_AVAILABLE_IN_ALL
xtype_t                 xbinding_flags_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t                 xbinding_get_type            (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xbinding_flags_t         xbinding_get_flags           (xbinding_t *binding);
XPL_DEPRECATED_IN_2_68_FOR(xbinding_dup_source)
xobject_t *             xbinding_get_source          (xbinding_t *binding);
XPL_AVAILABLE_IN_2_68
xobject_t *             xbinding_dup_source          (xbinding_t *binding);
XPL_DEPRECATED_IN_2_68_FOR(xbinding_dup_target)
xobject_t *             xbinding_get_target          (xbinding_t *binding);
XPL_AVAILABLE_IN_2_68
xobject_t *             xbinding_dup_target          (xbinding_t *binding);
XPL_AVAILABLE_IN_ALL
const xchar_t *         xbinding_get_source_property (xbinding_t *binding);
XPL_AVAILABLE_IN_ALL
const xchar_t *         xbinding_get_target_property (xbinding_t *binding);
XPL_AVAILABLE_IN_2_38
void                  xbinding_unbind              (xbinding_t *binding);

XPL_AVAILABLE_IN_ALL
xbinding_t *xobject_bind_property               (xpointer_t               source,
                                                const xchar_t           *source_property,
                                                xpointer_t               target,
                                                const xchar_t           *target_property,
                                                xbinding_flags_t          flags);
XPL_AVAILABLE_IN_ALL
xbinding_t *xobject_bind_property_full          (xpointer_t               source,
                                                const xchar_t           *source_property,
                                                xpointer_t               target,
                                                const xchar_t           *target_property,
                                                xbinding_flags_t          flags,
                                                xbinding_transform_func  transform_to,
                                                xbinding_transform_func  transform_from,
                                                xpointer_t               user_data,
                                                xdestroy_notify_t         notify);
XPL_AVAILABLE_IN_ALL
xbinding_t *xobject_bind_property_with_closures (xpointer_t               source,
                                                const xchar_t           *source_property,
                                                xpointer_t               target,
                                                const xchar_t           *target_property,
                                                xbinding_flags_t          flags,
                                                xclosure_t              *transform_to,
                                                xclosure_t              *transform_from);

G_END_DECLS

#endif /* __XBINDING_H__ */
