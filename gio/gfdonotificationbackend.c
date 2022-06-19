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

#include "gapplication.h"
#include "giomodule-priv.h"
#include "gnotification-private.h"
#include "gdbusconnection.h"
#include "gdbusnamewatching.h"
#include "gactiongroup.h"
#include "gaction.h"
#include "gthemedicon.h"
#include "gfileicon.h"
#include "gfile.h"
#include "gdbusutils.h"

#define XTYPE_FDO_NOTIFICATION_BACKEND  (g_fdo_notification_backend_get_type ())
#define G_FDO_NOTIFICATION_BACKEND(o)    (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FDO_NOTIFICATION_BACKEND, GFdoNotificationBackend))

typedef struct _GFdoNotificationBackend GFdoNotificationBackend;
typedef xnotification_backend_class_t       GFdoNotificationBackendClass;

struct _GFdoNotificationBackend
{
  xnotification_backend_t parent;

  xuint_t   bus_name_id;

  xuint_t   notify_subscription;
  xslist_t *notifications;
};

xtype_t g_fdo_notification_backend_get_type (void);

G_DEFINE_TYPE_WITH_CODE (GFdoNotificationBackend, g_fdo_notification_backend, XTYPE_NOTIFICATION_BACKEND,
  _xio_modules_ensure_extension_points_registered ();
  g_io_extension_point_implement (G_NOTIFICATION_BACKEND_EXTENSION_POINT_NAME,
                                 g_define_type_id, "freedesktop", 0))

typedef struct
{
  GFdoNotificationBackend *backend;
  xchar_t *id;
  xuint32_t notify_id;
  xchar_t *default_action;
  xvariant_t *default_action_target;
} FreedesktopNotification;

static void
freedesktop_notification_free (xpointer_t data)
{
  FreedesktopNotification *n = data;

  xobject_unref (n->backend);
  g_free (n->id);
  g_free (n->default_action);
  if (n->default_action_target)
    xvariant_unref (n->default_action_target);

  g_slice_free (FreedesktopNotification, n);
}

static FreedesktopNotification *
freedesktop_notification_new (GFdoNotificationBackend *backend,
                              const xchar_t             *id,
                              xnotification_t           *notification)
{
  FreedesktopNotification *n;

  n = g_slice_new0 (FreedesktopNotification);
  n->backend = xobject_ref (backend);
  n->id = xstrdup (id);
  n->notify_id = 0;
  xnotification_get_default_action (notification,
                                     &n->default_action,
                                     &n->default_action_target);

  return n;
}

static FreedesktopNotification *
g_fdo_notification_backend_find_notification (GFdoNotificationBackend *backend,
                                              const xchar_t             *id)
{
  xslist_t *it;

  for (it = backend->notifications; it != NULL; it = it->next)
    {
      FreedesktopNotification *n = it->data;
      if (xstr_equal (n->id, id))
        return n;
    }

  return NULL;
}

static FreedesktopNotification *
g_fdo_notification_backend_find_notification_by_notify_id (GFdoNotificationBackend *backend,
                                                           xuint32_t                  id)
{
  xslist_t *it;

  for (it = backend->notifications; it != NULL; it = it->next)
    {
      FreedesktopNotification *n = it->data;
      if (n->notify_id == id)
        return n;
    }

  return NULL;
}

static void
activate_action (GFdoNotificationBackend *backend,
                 const xchar_t             *name,
                 xvariant_t                *parameter)
{
  xnotification_backend_t *g_backend = G_NOTIFICATION_BACKEND (backend);

  if (name)
    {
      if (xstr_has_prefix (name, "app."))
        xaction_group_activate_action (XACTION_GROUP (g_backend->application), name + 4, parameter);
    }
  else
    {
      xapplication_activate (g_backend->application);
    }
}

static void
notify_signal (xdbus_connection_t *connection,
               const xchar_t     *sender_name,
               const xchar_t     *object_path,
               const xchar_t     *interface_name,
               const xchar_t     *signal_name,
               xvariant_t        *parameters,
               xpointer_t         user_data)
{
  GFdoNotificationBackend *backend = user_data;
  xuint32_t id = 0;
  const xchar_t *action = NULL;
  FreedesktopNotification *n;

  if (xstr_equal (signal_name, "NotificationClosed") &&
      xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(uu)")))
    {
      xvariant_get (parameters, "(uu)", &id, NULL);
    }
  else if (xstr_equal (signal_name, "ActionInvoked") &&
           xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(us)")))
    {
      xvariant_get (parameters, "(u&s)", &id, &action);
    }
  else
    return;

  n = g_fdo_notification_backend_find_notification_by_notify_id (backend, id);
  if (n == NULL)
    return;

  if (action)
    {
      if (xstr_equal (action, "default"))
        {
          activate_action (backend, n->default_action, n->default_action_target);
        }
      else
        {
          xchar_t *name;
          xvariant_t *target;

          if (xaction_parse_detailed_name (action, &name, &target, NULL))
            {
              activate_action (backend, name, target);
              g_free (name);
              if (target)
                xvariant_unref (target);
            }
        }
    }

  /* Get the notification again in case the action redrew it */
  n = g_fdo_notification_backend_find_notification_by_notify_id (backend, id);
  if (n != NULL)
    {
      backend->notifications = xslist_remove (backend->notifications, n);
      freedesktop_notification_free (n);
    }
}

static void
name_vanished_handler_cb (xdbus_connection_t *connection,
                          const xchar_t     *name,
                          xpointer_t         user_data)
{
  GFdoNotificationBackend *backend = user_data;

  if (backend->notifications)
    {
      xslist_free_full (backend->notifications, freedesktop_notification_free);
      backend->notifications = NULL;
    }
}

/* Converts a GNotificationPriority to an urgency level as defined by
 * the freedesktop spec (0: low, 1: normal, 2: critical).
 */
static xuchar_t
urgency_from_priority (GNotificationPriority priority)
{
  switch (priority)
    {
    case G_NOTIFICATION_PRIORITY_LOW:
      return 0;

    default:
    case G_NOTIFICATION_PRIORITY_NORMAL:
    case G_NOTIFICATION_PRIORITY_HIGH:
      return 1;

    case G_NOTIFICATION_PRIORITY_URGENT:
      return 2;
    }
}

static void
call_notify (xdbus_connection_t     *con,
             xapplication_t        *app,
             xuint32_t              replace_id,
             xnotification_t       *notification,
             xasync_ready_callback_t  callback,
             xpointer_t             user_data)
{
  xvariant_builder_t action_builder;
  xuint_t n_buttons;
  xuint_t i;
  xvariant_builder_t hints_builder;
  xicon_t *icon;
  xvariant_t *parameters;
  const xchar_t *app_name;
  const xchar_t *body;
  xuchar_t urgency;

  xvariant_builder_init (&action_builder, G_VARIANT_TYPE_STRING_ARRAY);
  if (xnotification_get_default_action (notification, NULL, NULL))
    {
      xvariant_builder_add (&action_builder, "s", "default");
      xvariant_builder_add (&action_builder, "s", "");
    }

  n_buttons = xnotification_get_n_buttons (notification);
  for (i = 0; i < n_buttons; i++)
    {
      xchar_t *label;
      xchar_t *action;
      xvariant_t *target;
      xchar_t *detailed_name;

      xnotification_get_button (notification, i, &label, &action, &target);
      detailed_name = g_action_print_detailed_name (action, target);

      /* Actions named 'default' collide with libnotify's naming of the
       * default action. Rewriting them to something unique is enough,
       * because those actions can never be activated (they aren't
       * prefixed with 'app.').
       */
      if (xstr_equal (detailed_name, "default"))
        {
          g_free (detailed_name);
          detailed_name = g_dbus_generate_guid ();
        }

      xvariant_builder_add_value (&action_builder, xvariant_new_take_string (detailed_name));
      xvariant_builder_add_value (&action_builder, xvariant_new_take_string (label));

      g_free (action);
      if (target)
        xvariant_unref (target);
    }

  xvariant_builder_init (&hints_builder, G_VARIANT_TYPE ("a{sv}"));
  xvariant_builder_add (&hints_builder, "{sv}", "desktop-entry",
                         xvariant_new_string (xapplication_get_application_id (app)));
  urgency = urgency_from_priority (xnotification_get_priority (notification));
  xvariant_builder_add (&hints_builder, "{sv}", "urgency", xvariant_new_byte (urgency));
  if (xnotification_get_category (notification))
    {
      xvariant_builder_add (&hints_builder, "{sv}", "category",
                             xvariant_new_string (xnotification_get_category (notification)));
    }

  icon = xnotification_get_icon (notification);
  if (icon != NULL)
    {
      if (X_IS_FILE_ICON (icon))
        {
           xfile_t *file;

           file = xfile_icon_get_file (XFILE_ICON (icon));
           xvariant_builder_add (&hints_builder, "{sv}", "image-path",
                                  xvariant_new_take_string (xfile_get_path (file)));
        }
      else if (X_IS_THEMED_ICON (icon))
        {
           const xchar_t* const* icon_names = g_themed_icon_get_names(G_THEMED_ICON (icon));
           /* Take first name from xthemed_icon_t */
           xvariant_builder_add (&hints_builder, "{sv}", "image-path",
                                  xvariant_new_string (icon_names[0]));
        }
    }

  app_name = g_get_application_name ();
  body = xnotification_get_body (notification);

  parameters = xvariant_new ("(susssasa{sv}i)",
                              app_name ? app_name : "",
                              replace_id,
                              "",           /* app icon */
                              xnotification_get_title (notification),
                              body ? body : "",
                              &action_builder,
                              &hints_builder,
                              -1);          /* expire_timeout */

  xdbus_connection_call (con, "org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                          "org.freedesktop.Notifications", "Notify",
                          parameters, G_VARIANT_TYPE ("(u)"),
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                          callback, user_data);
}

static void
notification_sent (xobject_t      *source_object,
                   xasync_result_t *result,
                   xpointer_t      user_data)
{
  FreedesktopNotification *n = user_data;
  xvariant_t *val;
  xerror_t *error = NULL;
  static xboolean_t warning_printed = FALSE;

  val = xdbus_connection_call_finish (G_DBUS_CONNECTION (source_object), result, &error);
  if (val)
    {
      GFdoNotificationBackend *backend = n->backend;
      FreedesktopNotification *match;

      xvariant_get (val, "(u)", &n->notify_id);
      xvariant_unref (val);

      match = g_fdo_notification_backend_find_notification_by_notify_id (backend, n->notify_id);
      if (match != NULL)
        {
          backend->notifications = xslist_remove (backend->notifications, match);
          freedesktop_notification_free (match);
        }
      backend->notifications = xslist_prepend (backend->notifications, n);
    }
  else
    {
      if (!warning_printed)
        {
          g_warning ("unable to send notifications through org.freedesktop.Notifications: %s",
                     error->message);
          warning_printed = TRUE;
        }

      freedesktop_notification_free (n);
      xerror_free (error);
    }
}

static void
g_fdo_notification_backend_dispose (xobject_t *object)
{
  GFdoNotificationBackend *backend = G_FDO_NOTIFICATION_BACKEND (object);

  if (backend->bus_name_id)
    {
      g_bus_unwatch_name (backend->bus_name_id);
      backend->bus_name_id = 0;
    }

  if (backend->notify_subscription)
    {
      xdbus_connection_t *session_bus;

      session_bus = G_NOTIFICATION_BACKEND (backend)->dbus_connection;
      xdbus_connection_signal_unsubscribe (session_bus, backend->notify_subscription);
      backend->notify_subscription = 0;
    }

  if (backend->notifications)
    {
      xslist_free_full (backend->notifications, freedesktop_notification_free);
      backend->notifications = NULL;
    }

  XOBJECT_CLASS (g_fdo_notification_backend_parent_class)->dispose (object);
}

static xboolean_t
g_fdo_notification_backend_is_supported (void)
{
  /* This is the fallback backend with the lowest priority. To avoid an
   * unnecessary synchronous dbus call to check for
   * org.freedesktop.Notifications, this function always succeeds. A
   * warning will be printed when sending the first notification fails.
   */
  return TRUE;
}

static void
g_fdo_notification_backend_send_notification (xnotification_backend_t *backend,
                                              const xchar_t          *id,
                                              xnotification_t        *notification)
{
  GFdoNotificationBackend *self = G_FDO_NOTIFICATION_BACKEND (backend);
  FreedesktopNotification *n, *tmp;

  if (self->bus_name_id == 0)
    {
      self->bus_name_id = g_bus_watch_name_on_connection (backend->dbus_connection,
                                                          "org.freedesktop.Notifications",
                                                          G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                          NULL,
                                                          name_vanished_handler_cb,
                                                          backend,
                                                          NULL);
    }

  if (self->notify_subscription == 0)
    {
      self->notify_subscription =
        xdbus_connection_signal_subscribe (backend->dbus_connection,
                                            "org.freedesktop.Notifications",
                                            "org.freedesktop.Notifications", NULL,
                                            "/org/freedesktop/Notifications", NULL,
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            notify_signal, backend, NULL);
    }

  n = freedesktop_notification_new (self, id, notification);

  tmp = g_fdo_notification_backend_find_notification (self, id);
  if (tmp)
    n->notify_id = tmp->notify_id;

  call_notify (backend->dbus_connection, backend->application, n->notify_id, notification, notification_sent, n);
}

static void
g_fdo_notification_backend_withdraw_notification (xnotification_backend_t *backend,
                                                  const xchar_t          *id)
{
  GFdoNotificationBackend *self = G_FDO_NOTIFICATION_BACKEND (backend);
  FreedesktopNotification *n;

  n = g_fdo_notification_backend_find_notification (self, id);
  if (n)
    {
      if (n->notify_id > 0)
        {
          xdbus_connection_call (backend->dbus_connection,
                                  "org.freedesktop.Notifications",
                                  "/org/freedesktop/Notifications",
                                  "org.freedesktop.Notifications", "CloseNotification",
                                  xvariant_new ("(u)", n->notify_id), NULL,
                                  G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
        }

      self->notifications = xslist_remove (self->notifications, n);
      freedesktop_notification_free (n);
    }
}

static void
g_fdo_notification_backend_init (GFdoNotificationBackend *backend)
{
}

static void
g_fdo_notification_backend_class_init (GFdoNotificationBackendClass *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);
  xnotification_backend_class_t *backend_class = G_NOTIFICATION_BACKEND_CLASS (class);

  object_class->dispose = g_fdo_notification_backend_dispose;

  backend_class->is_supported = g_fdo_notification_backend_is_supported;
  backend_class->send_notification = g_fdo_notification_backend_send_notification;
  backend_class->withdraw_notification = g_fdo_notification_backend_withdraw_notification;
}
