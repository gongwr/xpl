/* xobject_t - GLib Type, Object, Parameter and Signal Library
 *
 * Copyright (C) 2015-2022 Christian Hergert <christian@hergert.me>
 * Copyright (C) 2015 Garrett Regier <garrettregier@gmail.com>
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef __XBINDING_GROUP_H__
#define __XBINDING_GROUP_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include <glib.h>
#include <gobject/gobject.h>
#include <gobject/gbinding.h>

G_BEGIN_DECLS

#define XBINDING_GROUP(obj)    (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_BINDING_GROUP, xbinding_group_t))
#define X_IS_BINDING_GROUP(obj) (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_BINDING_GROUP))
#define XTYPE_BINDING_GROUP    (xbinding_group_get_type())

/**
 * xbinding_group_t:
 *
 * xbinding_group_t is an opaque structure whose members
 * cannot be accessed directly.
 *
 * Since: 2.72
 */
typedef struct _xbinding_group xbinding_group_t;

XPL_AVAILABLE_IN_2_72
xtype_t          xbinding_group_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_72
xbinding_group_t *xbinding_group_new                (void);
XPL_AVAILABLE_IN_2_72
xpointer_t       xbinding_group_dup_source         (xbinding_group_t         *self);
XPL_AVAILABLE_IN_2_72
void           xbinding_group_set_source         (xbinding_group_t         *self,
                                                   xpointer_t               source);
XPL_AVAILABLE_IN_2_72
void           xbinding_group_bind               (xbinding_group_t         *self,
                                                   const xchar_t           *source_property,
                                                   xpointer_t               target,
                                                   const xchar_t           *target_property,
                                                   xbinding_flags_t          flags);
XPL_AVAILABLE_IN_2_72
void           xbinding_group_bind_full          (xbinding_group_t         *self,
                                                   const xchar_t           *source_property,
                                                   xpointer_t               target,
                                                   const xchar_t           *target_property,
                                                   xbinding_flags_t          flags,
                                                   xbinding_transform_func  transform_to,
                                                   xbinding_transform_func  transform_from,
                                                   xpointer_t               user_data,
                                                   xdestroy_notify_t         user_data_destroy);
XPL_AVAILABLE_IN_2_72
void           xbinding_group_bind_with_closures (xbinding_group_t         *self,
                                                   const xchar_t           *source_property,
                                                   xpointer_t               target,
                                                   const xchar_t           *target_property,
                                                   xbinding_flags_t          flags,
                                                   xclosure_t              *transform_to,
                                                   xclosure_t              *transform_from);

G_END_DECLS

#endif /* __XBINDING_GROUP_H__ */
