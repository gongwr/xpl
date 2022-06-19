/*
 * Copyright © 2015 Canonical Limited
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC(xclosure, xclosure_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xenum_class, xtype_class_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xflags_class, xtype_class_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xobject, xobject_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xinitially_unowned, xobject_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xparam_spec, xparam_spec_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xtype_class, xtype_class_unref)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(xvalue, xvalue_unset)
