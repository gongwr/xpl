/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2011 Canonical Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_DBUS_ACTION_GROUP_H__
#define __G_DBUS_ACTION_GROUP_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include "giotypes.h"

G_BEGIN_DECLS

#define XTYPE_DBUS_ACTION_GROUP                            (xdbus_action_group_get_type ())
#define G_DBUS_ACTION_GROUP(inst)                           (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_DBUS_ACTION_GROUP, xdbus_action_group_t))
#define G_DBUS_ACTION_GROUP_CLASS(class)                    (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_DBUS_ACTION_GROUP, xdbus_action_group_class_t))
#define X_IS_DBUS_ACTION_GROUP(inst)                        (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_DBUS_ACTION_GROUP))
#define X_IS_DBUS_ACTION_GROUP_CLASS(class)                 (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_DBUS_ACTION_GROUP))
#define G_DBUS_ACTION_GROUP_GET_CLASS(inst)                 (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_DBUS_ACTION_GROUP, xdbus_action_group_class_t))

XPL_AVAILABLE_IN_ALL
xtype_t                   xdbus_action_group_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xdbus_action_group_t *      xdbus_action_group_get                       (xdbus_connection_t        *connection,
                                                                       const xchar_t            *bus_name,
                                                                       const xchar_t            *object_path);

G_END_DECLS

#endif /* __G_DBUS_ACTION_GROUP_H__ */
