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

#ifndef __XDEBUG_CONTROLLER_H__
#define __XDEBUG_CONTROLLER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * XDEBUG_CONTROLLER_EXTENSION_POINT_NAME:
 *
 * Extension point for debug control functionality.
 * See [Extending GIO][extending-gio].
 *
 * Since: 2.72
 */
#define XDEBUG_CONTROLLER_EXTENSION_POINT_NAME "gio-debug-controller"

/**
 * xdebug_controller_t:
 *
 * #xdebug_controller_t is an interface to expose control of debugging features and
 * debug output.
 *
 * Since: 2.72
 */
#define XTYPE_DEBUG_CONTROLLER             (xdebug_controller_get_type ())
XPL_AVAILABLE_IN_2_72
G_DECLARE_INTERFACE(xdebug_controller, xdebug_controller, g, debug_controller, xobject)

#define XDEBUG_CONTROLLER(o)               (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DEBUG_CONTROLLER, xdebug_controller_t))
#define X_IS_DEBUG_CONTROLLER(o)            (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DEBUG_CONTROLLER))
#define XDEBUG_CONTROLLER_GET_INTERFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_DEBUG_CONTROLLER, xdebug_controller_tInterface))

/**
 * xdebug_controller_tInterface:
 * @x_iface: The parent interface.
 *
 * The virtual function table for #xdebug_controller_t.
 *
 * Since: 2.72
 */
struct _xdebug_controller_tInterface {
  /*< private >*/
  xtype_interface_t x_iface;
};

XPL_AVAILABLE_IN_2_72
xboolean_t               xdebug_controller_get_debug_enabled     (xdebug_controller_t *self);
XPL_AVAILABLE_IN_2_72
void                   xdebug_controller_set_debug_enabled     (xdebug_controller_t *self,
                                                                 xboolean_t          debug_enabled);

G_END_DECLS

#endif /* __XDEBUG_CONTROLLER_H__ */
