/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2021 Endless OS Foundation, LLC
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

#ifndef __G_DEBUG_CONTROLLER_DBUS_H__
#define __G_DEBUG_CONTROLLER_DBUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GDebugControllerDBus:
 *
 * #GDebugControllerDBus is an implementation of #GDebugController over D-Bus.
 *
 * Since: 2.72
 */
#define XTYPE_DEBUG_CONTROLLER_DBUS (g_debug_controller_dbus_get_type ())
XPL_AVAILABLE_IN_2_72
G_DECLARE_DERIVABLE_TYPE (GDebugControllerDBus, g_debug_controller_dbus, G, DEBUG_CONTROLLER_DBUS, xobject_t)

/**
 * GDebugControllerDBusClass:
 * @parent_class: The parent class.
 * @authorize: Default handler for the #GDebugControllerDBus::authorize signal.
 *
 * The virtual function table for #GDebugControllerDBus.
 *
 * Since: 2.72
 */
struct _GDebugControllerDBusClass
{
  xobject_class_t parent_class;

  xboolean_t (*authorize)  (GDebugControllerDBus  *controller,
                          xdbus_method_invocation_t *invocation);

  xpointer_t padding[12];
};

XPL_AVAILABLE_IN_2_72
GDebugControllerDBus *g_debug_controller_dbus_new (xdbus_connection_t  *connection,
                                                   xcancellable_t     *cancellable,
                                                   xerror_t          **error);

XPL_AVAILABLE_IN_2_72
void g_debug_controller_dbus_stop (GDebugControllerDBus *self);

G_END_DECLS

#endif /* __G_DEBUG_CONTROLLER_DBUS_H__ */
