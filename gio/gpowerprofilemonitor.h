/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2019 Red Hat, Inc.
 * Copyright 2021 Igalia S.L.
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

#ifndef __G_POWER_PROFILE_MONITOR_H__
#define __G_POWER_PROFILE_MONITOR_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * G_POWER_PROFILE_MONITOR_EXTENSION_POINT_NAME:
 *
 * Extension point for power profile usage monitoring functionality.
 * See [Extending GIO][extending-gio].
 *
 * Since: 2.70
 */
#define G_POWER_PROFILE_MONITOR_EXTENSION_POINT_NAME "gio-power-profile-monitor"

#define XTYPE_POWER_PROFILE_MONITOR             (g_power_profile_monitor_get_type ())
XPL_AVAILABLE_IN_2_70
G_DECLARE_INTERFACE (xpower_profile_monitor_t, g_power_profile_monitor, g, power_profile_monitor, xobject_t)

#define G_POWER_PROFILE_MONITOR(o)               (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_POWER_PROFILE_MONITOR, xpower_profile_monitor))
#define X_IS_POWER_PROFILE_MONITOR(o)            (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_POWER_PROFILE_MONITOR))
#define G_POWER_PROFILE_MONITOR_GET_INTERFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_POWER_PROFILE_MONITOR, xpower_profile_monitor_interface))

struct _xpower_profile_monitor_tInterface
{
  /*< private >*/
  xtype_interface_t x_iface;
};

XPL_AVAILABLE_IN_2_70
xpower_profile_monitor_t      *g_power_profile_monitor_dup_default              (void);

XPL_AVAILABLE_IN_2_70
xboolean_t                   g_power_profile_monitor_get_power_saver_enabled  (xpower_profile_monitor_t *monitor);

G_END_DECLS

#endif /* __G_POWER_PROFILE_MONITOR_H__ */
