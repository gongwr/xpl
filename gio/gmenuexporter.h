/*
 * Copyright © 2011 Canonical Ltd.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __XMENU_EXPORTER_H__
#define __XMENU_EXPORTER_H__

#include <gio/gdbusconnection.h>
#include <gio/gmenumodel.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_2_32
xuint_t                   g_dbus_connection_export_menu_model             (xdbus_connection_t  *connection,
                                                                         const xchar_t      *object_path,
                                                                         xmenu_model_t       *menu,
                                                                         xerror_t          **error);

XPL_AVAILABLE_IN_2_32
void                    g_dbus_connection_unexport_menu_model           (xdbus_connection_t  *connection,
                                                                         xuint_t             export_id);

G_END_DECLS

#endif /* __XMENU_EXPORTER_H__ */
