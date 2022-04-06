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

#include "config.h"
#include "glib.h"
#include "glibintl.h"

#include "gdebugcontroller.h"
#include "ginitable.h"
#include "giomodule-priv.h"

/**
 * SECTION:gdebugcontroller
 * @title: xdebug_controller_t
 * @short_description: Debugging controller
 * @include: gio/gio.h
 *
 * #xdebug_controller_t is an interface to expose control of debugging features and
 * debug output.
 *
 * It is implemented on Linux using #xdebug_controller_dbus_t, which exposes a D-Bus
 * interface to allow authenticated peers to control debug features in this
 * process.
 *
 * Whether debug output is enabled is exposed as
 * #xdebug_controller_t:debug-enabled. This controls g_log_set_debug_enabled() by
 * default. Application code may connect to the #xobject_t::notify signal for it
 * to control other parts of its debug infrastructure as necessary.
 *
 * If your application or service is using the default GLib log writer function,
 * creating one of the built-in implementations of #xdebug_controller_t should be
 * all that’s needed to dynamically enable or disable debug output.
 *
 * Since: 2.72
 */

G_DEFINE_INTERFACE_WITH_CODE (xdebug_controller, xdebug_controller, XTYPE_OBJECT,
                              xtype_interface_add_prerequisite (g_define_type_id, XTYPE_INITABLE))

static void
xdebug_controller_default_init (xdebug_controller_tInterface *iface)
{
  /**
   * xdebug_controller_t:debug-enabled:
   *
   * %TRUE if debug output should be exposed (for example by forwarding it to
   * the journal), %FALSE otherwise.
   *
   * Since: 2.72
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_boolean ("debug-enabled",
                                                             "Debug Enabled",
                                                             "Whether to expose debug output",
                                                             FALSE,
                                                             G_PARAM_READWRITE |
                                                             G_PARAM_STATIC_STRINGS |
                                                             G_PARAM_EXPLICIT_NOTIFY));
}

/**
 * xdebug_controller_get_debug_enabled:
 * @self: a #xdebug_controller_t
 *
 * Get the value of #xdebug_controller_t:debug-enabled.
 *
 * Returns: %TRUE if debug output should be exposed, %FALSE otherwise
 * Since: 2.72
 */
xboolean_t
xdebug_controller_get_debug_enabled (xdebug_controller_t *self)
{
  xboolean_t enabled;

  g_return_val_if_fail (X_IS_DEBUG_CONTROLLER (self), FALSE);

  xobject_get (G_OBJECT (self),
                "debug-enabled", &enabled,
                NULL);

  return enabled;
}

/**
 * xdebug_controller_set_debug_enabled:
 * @self: a #xdebug_controller_t
 * @debug_enabled: %TRUE if debug output should be exposed, %FALSE otherwise
 *
 * Set the value of #xdebug_controller_t:debug-enabled.
 *
 * Since: 2.72
 */
void
xdebug_controller_set_debug_enabled (xdebug_controller_t *self,
                                      xboolean_t          debug_enabled)
{
  g_return_if_fail (X_IS_DEBUG_CONTROLLER (self));

  xobject_set (G_OBJECT (self),
                "debug-enabled", debug_enabled,
                NULL);
}
