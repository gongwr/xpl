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

#ifndef __XDEBUG_CONTROLLER_DBUS_H__
#define __XDEBUG_CONTROLLER_DBUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * xdebug_controller_dbus_t:
 *
 * #xdebug_controller_dbus_t is an implementation of #xdebug_controller_t over D-Bus.
 *
 * Since: 2.72
 */
#define XTYPE_DEBUG_CONTROLLER_DBUS (xdebug_controller_dbus_get_type ())
XPL_AVAILABLE_IN_2_72
G_DECLARE_DERIVABLE_TYPE (xdebug_controller_dbus_t, xdebug_controller_dbus, G, DEBUG_CONTROLLER_DBUS, xobject_t)

/**
 * xdebug_controller_tDBusClass:
 * @parent_class: The parent class.
 * @authorize: Default handler for the #xdebug_controller_dbus_t::authorize signal.
 *
 * The virtual function table for #xdebug_controller_dbus_t.
 *
 * Since: 2.72
 */
struct _xdebug_controller_tDBusClass
{
  xobject_class_t parent_class;

  xboolean_t (*authorize)  (xdebug_controller_dbus_t  *controller,
                          xdbus_method_invocation_t *invocation);

  xpointer_t padding[12];
};

XPL_AVAILABLE_IN_2_72
xdebug_controller_dbus_t *xdebug_controller_dbus_new (xdbus_connection_t  *connection,
                                                   xcancellable_t     *cancellable,
                                                   xerror_t          **error);

XPL_AVAILABLE_IN_2_72
void xdebug_controller_dbus_stop (xdebug_controller_dbus_t *self);

G_END_DECLS

#endif /* __XDEBUG_CONTROLLER_DBUS_H__ */
