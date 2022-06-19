/*
 * Copyright © 2013 Lars Uebernickel
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
 * Authors: Lars Uebernickel <lars@uebernic.de>
 */

#include "gnotification-server.h"

#include <gio/gio.h>

typedef xobject_class_t GNotificationServerClass;

struct _GNotificationServer
{
  xobject_t parent;

  xdbus_connection_t *connection;
  xuint_t name_owner_id;
  xuint_t object_id;

  xuint_t is_running;

  /* app_ids -> hashtables of notification ids -> a{sv} */
  xhashtable_t *applications;
};

XDEFINE_TYPE (GNotificationServer, xnotification_server, XTYPE_OBJECT)

enum
{
  PROP_0,
  PROP_IS_RUNNING
};

static xdbus_interface_info_t *
org_gtk_Notifications_get_interface (void)
{
  static xdbus_interface_info_t *iface_info;

  if (iface_info == NULL)
    {
      xdbus_node_info_t *info;
      xerror_t *error = NULL;

      info = g_dbus_node_info_new_for_xml (
        "<node>"
        "  <interface name='org.gtk.Notifications'>"
        "    <method name='AddNotification'>"
        "      <arg type='s' direction='in' />"
        "      <arg type='s' direction='in' />"
        "      <arg type='a{sv}' direction='in' />"
        "    </method>"
        "    <method name='RemoveNotification'>"
        "      <arg type='s' direction='in' />"
        "      <arg type='s' direction='in' />"
        "    </method>"
        "  </interface>"
        "</node>", &error);

      if (info == NULL)
        xerror ("%s", error->message);

      iface_info = g_dbus_node_info_lookup_interface (info, "org.gtk.Notifications");
      xassert (iface_info);

      g_dbus_interface_info_ref (iface_info);
      g_dbus_node_info_unref (info);
    }

  return iface_info;
}

static void
xnotification_server_notification_added (GNotificationServer *server,
                                          const xchar_t         *app_id,
                                          const xchar_t         *notification_id,
                                          xvariant_t            *notification)
{
  xhashtable_t *notifications;

  notifications = xhash_table_lookup (server->applications, app_id);
  if (notifications == NULL)
    {
      notifications = xhash_table_new_full (xstr_hash, xstr_equal,
                                             g_free, (xdestroy_notify_t) xvariant_unref);
      xhash_table_insert (server->applications, xstrdup (app_id), notifications);
    }

  xhash_table_replace (notifications, xstrdup (notification_id), xvariant_ref (notification));

  xsignal_emit_by_name (server, "notification-received", app_id, notification_id, notification);
}

static void
xnotification_server_notification_removed (GNotificationServer *server,
                                            const xchar_t         *app_id,
                                            const xchar_t         *notification_id)
{
  xhashtable_t *notifications;

  notifications = xhash_table_lookup (server->applications, app_id);
  if (notifications)
    {
      xhash_table_remove (notifications, notification_id);
      if (xhash_table_size (notifications) == 0)
        xhash_table_remove (server->applications, app_id);
    }

  xsignal_emit_by_name (server, "notification-removed", app_id, notification_id);
}

static void
org_gtk_Notifications_method_call (xdbus_connection_t       *connection,
                                   const xchar_t           *sender,
                                   const xchar_t           *object_path,
                                   const xchar_t           *interface_name,
                                   const xchar_t           *method_name,
                                   xvariant_t              *parameters,
                                   xdbus_method_invocation_t *invocation,
                                   xpointer_t               user_data)
{
  GNotificationServer *server = user_data;

  if (xstr_equal (method_name, "AddNotification"))
    {
      const xchar_t *app_id;
      const xchar_t *notification_id;
      xvariant_t *notification;

      xvariant_get (parameters, "(&s&s@a{sv})", &app_id, &notification_id, &notification);
      xnotification_server_notification_added (server, app_id, notification_id, notification);
      xdbus_method_invocation_return_value (invocation, NULL);

      xvariant_unref (notification);
    }
  else if (xstr_equal (method_name, "RemoveNotification"))
    {
      const xchar_t *app_id;
      const xchar_t *notification_id;

      xvariant_get (parameters, "(&s&s)", &app_id, &notification_id);
      xnotification_server_notification_removed (server, app_id, notification_id);
      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else
    {
      xdbus_method_invocation_return_dbus_error (invocation, "UnknownMethod", "No such method");
    }
}

static void
xnotification_server_dispose (xobject_t *object)
{
  GNotificationServer *server = G_NOTIFICATION_SERVER (object);

  xnotification_server_stop (server);

  g_clear_pointer (&server->applications, xhash_table_unref);
  g_clear_object (&server->connection);

  XOBJECT_CLASS (xnotification_server_parent_class)->dispose (object);
}

static void
xnotification_server_get_property (xobject_t    *object,
                                    xuint_t       property_id,
                                    xvalue_t     *value,
                                    xparam_spec_t *pspec)
{
  GNotificationServer *server = G_NOTIFICATION_SERVER (object);

  switch (property_id)
    {
    case PROP_IS_RUNNING:
      xvalue_set_boolean (value, server->is_running);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
xnotification_server_class_init (GNotificationServerClass *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->get_property = xnotification_server_get_property;
  object_class->dispose = xnotification_server_dispose;

  xobject_class_install_property (object_class, PROP_IS_RUNNING,
                                   xparam_spec_boolean ("is-running", "", "", FALSE,
                                                         XPARAM_READABLE | XPARAM_STATIC_STRINGS));

  xsignal_new ("notification-received", XTYPE_NOTIFICATION_SERVER, G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, g_cclosure_marshal_generic, XTYPE_NONE, 3,
                XTYPE_STRING, XTYPE_STRING, XTYPE_VARIANT);

  xsignal_new ("notification-removed", XTYPE_NOTIFICATION_SERVER, G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, g_cclosure_marshal_generic, XTYPE_NONE, 2,
                XTYPE_STRING, XTYPE_STRING);
}

static void
xnotification_server_bus_acquired (xdbus_connection_t *connection,
                                    const xchar_t     *name,
                                    xpointer_t         user_data)
{
  const xdbus_interface_vtable_t vtable = {
    org_gtk_Notifications_method_call, NULL, NULL, { 0 }
  };
  GNotificationServer *server = user_data;

  server->object_id = xdbus_connection_register_object (connection, "/org/gtk/Notifications",
                                                         org_gtk_Notifications_get_interface (),
                                                         &vtable, server, NULL, NULL);

  /* register_object only fails if the same object is exported more than once */
  xassert (server->object_id > 0);

  server->connection = xobject_ref (connection);
}

static void
xnotification_server_name_acquired (xdbus_connection_t *connection,
                                     const xchar_t     *name,
                                     xpointer_t         user_data)
{
  GNotificationServer *server = user_data;

  server->is_running = TRUE;
  xobject_notify (G_OBJECT (server), "is-running");
}

static void
xnotification_server_name_lost (xdbus_connection_t *connection,
                                 const xchar_t     *name,
                                 xpointer_t         user_data)
{
  GNotificationServer *server = user_data;

  xnotification_server_stop (server);

  if (connection == NULL && server->connection)
    g_clear_object (&server->connection);
}

static void
xnotification_server_init (GNotificationServer *server)
{
  server->applications = xhash_table_new_full (xstr_hash, xstr_equal,
                                                g_free, (xdestroy_notify_t) xhash_table_unref);

  server->name_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                          "org.gtk.Notifications",
                                          G_BUS_NAME_OWNER_FLAGS_NONE,
                                          xnotification_server_bus_acquired,
                                          xnotification_server_name_acquired,
                                          xnotification_server_name_lost,
                                          server, NULL);
}

GNotificationServer *
xnotification_server_new (void)
{
  return xobject_new (XTYPE_NOTIFICATION_SERVER, NULL);
}

void
xnotification_server_stop (GNotificationServer *server)
{
  g_return_if_fail (X_IS_NOTIFICATION_SERVER (server));

  if (server->name_owner_id)
    {
      g_bus_unown_name (server->name_owner_id);
      server->name_owner_id = 0;
    }

  if (server->object_id && server->connection)
    {
      xdbus_connection_unregister_object (server->connection, server->object_id);
      server->object_id = 0;
    }

  if (server->is_running)
    {
      server->is_running = FALSE;
      xobject_notify (G_OBJECT (server), "is-running");
    }
}

xboolean_t
xnotification_server_get_is_running (GNotificationServer *server)
{
  xreturn_val_if_fail (X_IS_NOTIFICATION_SERVER (server), FALSE);

  return server->is_running;
}

xchar_t **
xnotification_server_list_applications (GNotificationServer *server)
{
  xreturn_val_if_fail (X_IS_NOTIFICATION_SERVER (server), NULL);

  return (xchar_t **) xhash_table_get_keys_as_array (server->applications, NULL);
}

xchar_t **
xnotification_server_list_notifications (GNotificationServer *server,
                                          const xchar_t         *app_id)
{
  xhashtable_t *notifications;

  xreturn_val_if_fail (X_IS_NOTIFICATION_SERVER (server), NULL);
  xreturn_val_if_fail (app_id != NULL, NULL);

  notifications = xhash_table_lookup (server->applications, app_id);

  if (notifications == NULL)
    return NULL;

  return (xchar_t **) xhash_table_get_keys_as_array (notifications, NULL);
}
