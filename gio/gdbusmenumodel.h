/*
 * Copyright © 2011 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_DBUS_MENU_MODEL_H__
#define __G_DBUS_MENU_MODEL_H__

#include <gio/gdbusconnection.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_MENU_MODEL          (g_dbus_menu_model_get_type ())
#define G_DBUS_MENU_MODEL(inst)         (XTYPE_CHECK_INSTANCE_CAST ((inst),   \
                                         XTYPE_DBUS_MENU_MODEL, xdbus_menu_model_t)
#define X_IS_DBUS_MENU_MODEL(inst)      (XTYPE_CHECK_INSTANCE_TYPE ((inst),   \
                                         XTYPE_DBUS_MENU_MODEL))

typedef struct _GDBusMenuModel xdbus_menu_model_t;

XPL_AVAILABLE_IN_ALL
xtype_t                   g_dbus_menu_model_get_type     (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xdbus_menu_model_t *        g_dbus_menu_model_get          (xdbus_connection_t *connection,
                                                        const xchar_t     *bus_name,
                                                        const xchar_t     *object_path);

G_END_DECLS

#endif /* __G_DBUS_MENU_MODEL_H__ */
