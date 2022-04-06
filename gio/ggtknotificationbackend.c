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

#include "config.h"
#include "gnotificationbackend.h"

#include "giomodule-priv.h"
#include "gdbusconnection.h"
#include "gapplication.h"
#include "gnotification-private.h"

#define XTYPE_GTK_NOTIFICATION_BACKEND  (g_gtk_notification_backend_get_type ())
#define G_GTK_NOTIFICATION_BACKEND(o)    (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_GTK_NOTIFICATION_BACKEND, GGtkNotificationBackend))

typedef struct _GGtkNotificationBackend GGtkNotificationBackend;
typedef xnotification_backend_class_t       GGtkNotificationBackendClass;

struct _GGtkNotificationBackend
{
  xnotification_backend_t parent;
};

xtype_t g_gtk_notification_backend_get_type (void);

G_DEFINE_TYPE_WITH_CODE (GGtkNotificationBackend, g_gtk_notification_backend, XTYPE_NOTIFICATION_BACKEND,
  _xio_modules_ensure_extension_points_registered ();
  g_io_extension_point_implement (G_NOTIFICATION_BACKEND_EXTENSION_POINT_NAME,
                                 g_define_type_id, "gtk", 100))

static xboolean_t
g_gtk_notification_backend_is_supported (void)
{
  xdbus_connection_t *session_bus;
  xvariant_t *reply;

  /* Find out if the notification server is running. This is a
   * synchronous call because gio extension points don't support asnyc
   * backend verification. This is only run once and only contacts the
   * dbus daemon. */

  session_bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  if (session_bus == NULL)
    return FALSE;

  reply = g_dbus_connection_call_sync (session_bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus",
                                       "GetNameOwner", xvariant_new ("(s)", "org.gtk.Notifications"),
                                       G_VARIANT_TYPE ("(s)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);

  xobject_unref (session_bus);

  if (reply)
    {
      xvariant_unref (reply);
      return TRUE;
    }
  else
    return FALSE;
}

static void
g_gtk_notification_backend_send_notification (xnotification_backend_t *backend,
                                              const xchar_t          *id,
                                              xnotification_t        *notification)
{
  xvariant_t *params;

  params = xvariant_new ("(ss@a{sv})", xapplication_get_application_id (backend->application),
                                        id,
                                        xnotification_serialize (notification));

  g_dbus_connection_call (backend->dbus_connection,
                          "org.gtk.Notifications", "/org/gtk/Notifications",
                          "org.gtk.Notifications", "AddNotification", params,
                          G_VARIANT_TYPE_UNIT,
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void
g_gtk_notification_backend_withdraw_notification (xnotification_backend_t *backend,
                                                  const xchar_t          *id)
{
  xvariant_t *params;

  params = xvariant_new ("(ss)", xapplication_get_application_id (backend->application), id);

  g_dbus_connection_call (backend->dbus_connection, "org.gtk.Notifications",
                          "/org/gtk/Notifications", "org.gtk.Notifications",
                          "RemoveNotification", params, G_VARIANT_TYPE_UNIT,
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void
g_gtk_notification_backend_init (GGtkNotificationBackend *backend)
{
}

static void
g_gtk_notification_backend_class_init (GGtkNotificationBackendClass *class)
{
  xnotification_backend_class_t *backend_class = G_NOTIFICATION_BACKEND_CLASS (class);

  backend_class->is_supported = g_gtk_notification_backend_is_supported;
  backend_class->send_notification = g_gtk_notification_backend_send_notification;
  backend_class->withdraw_notification = g_gtk_notification_backend_withdraw_notification;
}
