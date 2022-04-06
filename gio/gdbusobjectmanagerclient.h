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

#ifndef __G_DBUS_OBJECT_MANAGER_CLIENT_H__
#define __G_DBUS_OBJECT_MANAGER_CLIENT_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_OBJECT_MANAGER_CLIENT         (g_dbus_object_manager_client_get_type ())
#define G_DBUS_OBJECT_MANAGER_CLIENT(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_OBJECT_MANAGER_CLIENT, xdbus_object_manager_client))
#define G_DBUS_OBJECT_MANAGER_CLIENT_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_OBJECT_MANAGER_CLIENT, GDBusObjectManagerClientClass))
#define G_DBUS_OBJECT_MANAGER_CLIENT_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_OBJECT_MANAGER_CLIENT, GDBusObjectManagerClientClass))
#define X_IS_DBUS_OBJECT_MANAGER_CLIENT(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_OBJECT_MANAGER_CLIENT))
#define X_IS_DBUS_OBJECT_MANAGER_CLIENT_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_OBJECT_MANAGER_CLIENT))

typedef struct _GDBusObjectManagerClientClass   GDBusObjectManagerClientClass;
typedef struct _GDBusObjectManagerClientPrivate GDBusObjectManagerClientPrivate;

/**
 * xdbus_object_manager_client_t:
  *
 * The #xdbus_object_manager_client_t structure contains private data and should
 * only be accessed using the provided API.
 *
 * Since: 2.30
 */
struct _GDBusObjectManagerClient
{
  /*< private >*/
  xobject_t parent_instance;
  GDBusObjectManagerClientPrivate *priv;
};

/**
 * GDBusObjectManagerClientClass:
 * @parent_class: The parent class.
 * @interface_proxy_signal: Signal class handler for the #xdbus_object_manager_client_t::interface-proxy-signal signal.
 * @interface_proxy_properties_changed: Signal class handler for the #xdbus_object_manager_client_t::interface-proxy-properties-changed signal.
 *
 * Class structure for #xdbus_object_manager_client_t.
 *
 * Since: 2.30
 */
struct _GDBusObjectManagerClientClass
{
  xobject_class_t parent_class;

  /* signals */
  void    (*interface_proxy_signal)             (xdbus_object_manager_client_t *manager,
                                                 xdbus_object_proxy_t         *object_proxy,
                                                 xdbus_proxy_t               *interface_proxy,
                                                 const xchar_t              *sender_name,
                                                 const xchar_t              *signal_name,
                                                 xvariant_t                 *parameters);

  void    (*interface_proxy_properties_changed) (xdbus_object_manager_client_t   *manager,
                                                 xdbus_object_proxy_t           *object_proxy,
                                                 xdbus_proxy_t                 *interface_proxy,
                                                 xvariant_t                   *changed_properties,
                                                 const xchar_t* const         *invalidated_properties);

  /*< private >*/
  xpointer_t padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t                         g_dbus_object_manager_client_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
void                          g_dbus_object_manager_client_new                (xdbus_connection_t               *connection,
                                                                               GDBusObjectManagerClientFlags  flags,
                                                                               const xchar_t                   *name,
                                                                               const xchar_t                   *object_path,
                                                                               xdbus_proxy_type_func_t             get_proxy_type_func,
                                                                               xpointer_t                       get_proxy_type_user_data,
                                                                               xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                                                               xcancellable_t                  *cancellable,
                                                                               xasync_ready_callback_t            callback,
                                                                               xpointer_t                       user_data);
XPL_AVAILABLE_IN_ALL
xdbus_object_manager_t           *g_dbus_object_manager_client_new_finish         (xasync_result_t                  *res,
                                                                               xerror_t                       **error);
XPL_AVAILABLE_IN_ALL
xdbus_object_manager_t           *g_dbus_object_manager_client_new_sync           (xdbus_connection_t               *connection,
                                                                               GDBusObjectManagerClientFlags  flags,
                                                                               const xchar_t                   *name,
                                                                               const xchar_t                   *object_path,
                                                                               xdbus_proxy_type_func_t             get_proxy_type_func,
                                                                               xpointer_t                       get_proxy_type_user_data,
                                                                               xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                                                               xcancellable_t                  *cancellable,
                                                                               xerror_t                       **error);
XPL_AVAILABLE_IN_ALL
void                          g_dbus_object_manager_client_new_for_bus        (GBusType                       bus_type,
                                                                               GDBusObjectManagerClientFlags  flags,
                                                                               const xchar_t                   *name,
                                                                               const xchar_t                   *object_path,
                                                                               xdbus_proxy_type_func_t             get_proxy_type_func,
                                                                               xpointer_t                       get_proxy_type_user_data,
                                                                               xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                                                               xcancellable_t                  *cancellable,
                                                                               xasync_ready_callback_t            callback,
                                                                               xpointer_t                       user_data);
XPL_AVAILABLE_IN_ALL
xdbus_object_manager_t           *g_dbus_object_manager_client_new_for_bus_finish (xasync_result_t                  *res,
                                                                               xerror_t                       **error);
XPL_AVAILABLE_IN_ALL
xdbus_object_manager_t           *g_dbus_object_manager_client_new_for_bus_sync   (GBusType                       bus_type,
                                                                               GDBusObjectManagerClientFlags  flags,
                                                                               const xchar_t                   *name,
                                                                               const xchar_t                   *object_path,
                                                                               xdbus_proxy_type_func_t             get_proxy_type_func,
                                                                               xpointer_t                       get_proxy_type_user_data,
                                                                               xdestroy_notify_t                 get_proxy_type_destroy_notify,
                                                                               xcancellable_t                  *cancellable,
                                                                               xerror_t                       **error);
XPL_AVAILABLE_IN_ALL
xdbus_connection_t              *g_dbus_object_manager_client_get_connection     (xdbus_object_manager_client_t      *manager);
XPL_AVAILABLE_IN_ALL
GDBusObjectManagerClientFlags g_dbus_object_manager_client_get_flags          (xdbus_object_manager_client_t      *manager);
XPL_AVAILABLE_IN_ALL
const xchar_t                  *g_dbus_object_manager_client_get_name           (xdbus_object_manager_client_t      *manager);
XPL_AVAILABLE_IN_ALL
xchar_t                        *g_dbus_object_manager_client_get_name_owner     (xdbus_object_manager_client_t      *manager);

G_END_DECLS

#endif /* __G_DBUS_OBJECT_MANAGER_CLIENT_H */
