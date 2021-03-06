/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XINITABLE_H__
#define __XINITABLE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_INITABLE            (xinitable_get_type ())
#define XINITABLE(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_INITABLE, xinitable_t))
#define X_IS_INITABLE(obj)         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_INITABLE))
#define XINITABLE_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_INITABLE, xinitable_iface_t))
#define XTYPE_IS_INITABLE(type)   (xtype_is_a ((type), XTYPE_INITABLE))

/**
 * xinitable_t:
 *
 * Interface for initializable objects.
 *
 * Since: 2.22
 **/
typedef struct _xinitable_iface xinitable_iface_t;

/**
 * xinitable_iface_t:
 * @x_iface: The parent interface.
 * @init: Initializes the object.
 *
 * Provides an interface for initializing object such that initialization
 * may fail.
 *
 * Since: 2.22
 **/
struct _xinitable_iface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xboolean_t    (* init) (xinitable_t    *initable,
			xcancellable_t *cancellable,
			xerror_t      **error);
};


XPL_AVAILABLE_IN_ALL
xtype_t    xinitable_get_type   (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t xinitable_init       (xinitable_t     *initable,
				xcancellable_t  *cancellable,
				xerror_t       **error);

XPL_AVAILABLE_IN_ALL
xpointer_t xinitable_new        (xtype_t          object_type,
				xcancellable_t  *cancellable,
				xerror_t       **error,
				const xchar_t   *first_property_name,
				...);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

XPL_DEPRECATED_IN_2_54_FOR(xobject_new_with_properties and xinitable_init)
xpointer_t xinitable_newv       (xtype_t          object_type,
				xuint_t          n_parameters,
				GParameter    *parameters,
				xcancellable_t  *cancellable,
				xerror_t       **error);

G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
xobject_t* xinitable_new_valist (xtype_t          object_type,
				const xchar_t   *first_property_name,
				va_list        var_args,
				xcancellable_t  *cancellable,
				xerror_t       **error);

G_END_DECLS


#endif /* __XINITABLE_H__ */
