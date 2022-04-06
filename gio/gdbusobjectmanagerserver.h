/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_DBUS_OBJECT_MANAGER_SERVER_H__
#define __G_DBUS_OBJECT_MANAGER_SERVER_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_OBJECT_MANAGER_SERVER         (g_dbus_object_manager_server_get_type ())
#define G_DBUS_OBJECT_MANAGER_SERVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_OBJECT_MANAGER_SERVER, xdbus_object_manager_server))
#define G_DBUS_OBJECT_MANAGER_SERVER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_OBJECT_MANAGER_SERVER, GDBusObjectManagerServerClass))
#define G_DBUS_OBJECT_MANAGER_SERVER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_OBJECT_MANAGER_SERVER, GDBusObjectManagerServerClass))
#define X_IS_DBUS_OBJECT_MANAGER_SERVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_OBJECT_MANAGER_SERVER))
#define X_IS_DBUS_OBJECT_MANAGER_SERVER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_OBJECT_MANAGER_SERVER))

typedef struct _GDBusObjectManagerServerClass   GDBusObjectManagerServerClass;
typedef struct _GDBusObjectManagerServerPrivate GDBusObjectManagerServerPrivate;

/**
 * xdbus_object_manager_server_t:
 *
 * The #xdbus_object_manager_server_t structure contains private data and should
 * only be accessed using the provided API.
 *
 * Since: 2.30
 */
struct _GDBusObjectManagerServer
{
  /*< private >*/
  xobject_t parent_instance;
  GDBusObjectManagerServerPrivate *priv;
};

/**
 * GDBusObjectManagerServerClass:
 * @parent_class: The parent class.
 *
 * Class structure for #xdbus_object_manager_server_t.
 *
 * Since: 2.30
 */
struct _GDBusObjectManagerServerClass
{
  xobject_class_t parent_class;

  /*< private >*/
  xpointer_t padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t                     g_dbus_object_manager_server_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xdbus_object_manager_server_t *g_dbus_object_manager_server_new                 (const xchar_t               *object_path);
XPL_AVAILABLE_IN_ALL
xdbus_connection_t          *g_dbus_object_manager_server_get_connection      (xdbus_object_manager_server_t  *manager);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_object_manager_server_set_connection      (xdbus_object_manager_server_t  *manager,
                                                                            xdbus_connection_t           *connection);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_object_manager_server_export              (xdbus_object_manager_server_t  *manager,
                                                                            xdbus_object_skeleton_t       *object);
XPL_AVAILABLE_IN_ALL
void                      g_dbus_object_manager_server_export_uniquely     (xdbus_object_manager_server_t  *manager,
                                                                            xdbus_object_skeleton_t       *object);
XPL_AVAILABLE_IN_ALL
xboolean_t                  g_dbus_object_manager_server_is_exported         (xdbus_object_manager_server_t  *manager,
                                                                            xdbus_object_skeleton_t       *object);
XPL_AVAILABLE_IN_ALL
xboolean_t                  g_dbus_object_manager_server_unexport            (xdbus_object_manager_server_t  *manager,
                                                                            const xchar_t               *object_path);

G_END_DECLS

#endif /* __G_DBUS_OBJECT_MANAGER_SERVER_H */
