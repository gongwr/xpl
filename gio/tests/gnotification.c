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

#include <glib.h>

#include "gnotification-server.h"
#include "gdbus-sessionbus.h"

static void
activate_app (xapplication_t *application,
              xpointer_t      user_data)
{
  xnotification_t *notification;
  xicon_t *icon;

  notification = xnotification_new ("test_t");

  xapplication_send_notification (application, "test1", notification);
  xapplication_send_notification (application, "test2", notification);
  xapplication_withdraw_notification (application, "test1");
  xapplication_send_notification (application, "test3", notification);

  icon = g_themed_icon_new ("i-c-o-n");
  xnotification_set_icon (notification, icon);
  xobject_unref (icon);

  xnotification_set_body (notification, "body");
  xnotification_set_priority (notification, G_NOTIFICATION_PRIORITY_URGENT);
  xnotification_set_default_action_and_target (notification, "app.action", "i", 42);
  xnotification_add_button_with_target (notification, "label", "app.action2", "s", "bla");

  xapplication_send_notification (application, "test4", notification);
  xapplication_send_notification (application, NULL, notification);

  xdbus_connection_flush_sync (xapplication_get_dbus_connection (application), NULL, NULL);

  xobject_unref (notification);
}

static void
notification_received (GNotificationServer *server,
                       const xchar_t         *app_id,
                       const xchar_t         *notification_id,
                       xvariant_t            *notification,
                       xpointer_t             user_data)
{
  xint_t *count = user_data;
  const xchar_t *title;

  g_assert_cmpstr (app_id, ==, "org.gtk.TestApplication");

  switch (*count)
    {
    case 0:
      g_assert_cmpstr (notification_id, ==, "test1");
      g_assert (xvariant_lookup (notification, "title", "&s", &title));
      g_assert_cmpstr (title, ==, "test_t");
      break;

    case 1:
      g_assert_cmpstr (notification_id, ==, "test2");
      break;

    case 2:
      g_assert_cmpstr (notification_id, ==, "test3");
      break;

    case 3:
      g_assert_cmpstr (notification_id, ==, "test4");
      break;

    case 4:
      g_assert (g_dbus_is_guid (notification_id));

      xnotification_server_stop (server);
      break;
    }

  (*count)++;
}

static void
notification_removed (GNotificationServer *server,
                      const xchar_t         *app_id,
                      const xchar_t         *notification_id,
                      xpointer_t             user_data)
{
  xint_t *count = user_data;

  g_assert_cmpstr (app_id, ==, "org.gtk.TestApplication");
  g_assert_cmpstr (notification_id, ==, "test1");

  (*count)++;
}

static void
server_notify_is_running (xobject_t    *object,
                          xparam_spec_t *pspec,
                          xpointer_t    user_data)
{
  xmain_loop_t *loop = user_data;
  GNotificationServer *server = G_NOTIFICATION_SERVER (object);

  if (xnotification_server_get_is_running (server))
    {
      xapplication_t *app;

      app = xapplication_new ("org.gtk.TestApplication", G_APPLICATION_FLAGS_NONE);
      xsignal_connect (app, "activate", G_CALLBACK (activate_app), NULL);

      xapplication_run (app, 0, NULL);

      xobject_unref (app);
    }
  else
    {
      xmain_loop_quit (loop);
    }
}

static xboolean_t
timeout (xpointer_t user_data)
{
  GNotificationServer *server = user_data;

  xnotification_server_stop (server);

  return G_SOURCE_REMOVE;
}

static void
basic (void)
{
  GNotificationServer *server;
  xmain_loop_t *loop;
  xint_t received_count = 0;
  xint_t removed_count = 0;

  session_bus_up ();

  loop = xmain_loop_new (NULL, FALSE);

  server = xnotification_server_new ();
  xsignal_connect (server, "notification-received", G_CALLBACK (notification_received), &received_count);
  xsignal_connect (server, "notification-removed", G_CALLBACK (notification_removed), &removed_count);
  xsignal_connect (server, "notify::is-running", G_CALLBACK (server_notify_is_running), loop);
  g_timeout_add_seconds (1, timeout, server);

  xmain_loop_run (loop);

  g_assert_cmpint (received_count, ==, 5);
  g_assert_cmpint (removed_count, ==, 1);

  xobject_unref (server);
  xmain_loop_unref (loop);
  session_bus_stop ();
}

struct _GNotification
{
  xobject_t parent;

  xchar_t *title;
  xchar_t *body;
  xicon_t *icon;
  GNotificationPriority priority;
  xchar_t *category;
  xptr_array_t *buttons;
  xchar_t *default_action;
  xvariant_t *default_action_target;
};

typedef struct
{
  xchar_t *label;
  xchar_t *action_name;
  xvariant_t *target;
} Button;

static void
test_properties (void)
{
  xnotification_t *n;
  struct _GNotification *rn;
  xicon_t *icon;
  const xchar_t * const *names;
  Button *b;

  n = xnotification_new ("test_t");

  xnotification_set_title (n, "title");
  xnotification_set_body (n, "body");
  xnotification_set_category (n, "cate.gory");
  icon = g_themed_icon_new ("i-c-o-n");
  xnotification_set_icon (n, icon);
  xobject_unref (icon);
  xnotification_set_priority (n, G_NOTIFICATION_PRIORITY_HIGH);
  xnotification_set_category (n, "cate.gory");
  xnotification_add_button (n, "label1", "app.action1::target1");
  xnotification_set_default_action (n, "app.action2::target2");

  rn = (struct _GNotification *)n;

  g_assert_cmpstr (rn->title, ==, "title");
  g_assert_cmpstr (rn->body, ==, "body");
  g_assert (X_IS_THEMED_ICON (rn->icon));
  names = g_themed_icon_get_names (G_THEMED_ICON (rn->icon));
  g_assert_cmpstr (names[0], ==, "i-c-o-n");
  g_assert_cmpstr (names[1], ==, "i-c-o-n-symbolic");
  g_assert_null (names[2]);
  g_assert (rn->priority == G_NOTIFICATION_PRIORITY_HIGH);
  g_assert_cmpstr (rn->category, ==, "cate.gory");

  g_assert_cmpint (rn->buttons->len, ==, 1);
  b = (Button*)rn->buttons->pdata[0];
  g_assert_cmpstr (b->label, ==, "label1");
  g_assert_cmpstr (b->action_name, ==, "app.action1");
  g_assert_cmpstr (xvariant_get_string (b->target, NULL), ==, "target1");

  g_assert_cmpstr (rn->default_action, ==, "app.action2");
  g_assert_cmpstr (xvariant_get_string (rn->default_action_target, NULL), ==, "target2");

  xobject_unref (n);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gnotification/basic", basic);
  g_test_add_func ("/gnotification/properties", test_properties);

  return g_test_run ();
}
