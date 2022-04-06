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

#include "gnotification-private.h"
#include "gdbusutils.h"
#include "gicon.h"
#include "gaction.h"
#include "gioenumtypes.h"

/**
 * SECTION:gnotification
 * @short_description: User Notifications (pop up messages)
 * @include: gio/gio.h
 *
 * #xnotification_t is a mechanism for creating a notification to be shown
 * to the user -- typically as a pop-up notification presented by the
 * desktop environment shell.
 *
 * The key difference between #xnotification_t and other similar APIs is
 * that, if supported by the desktop environment, notifications sent
 * with #xnotification_t will persist after the application has exited,
 * and even across system reboots.
 *
 * Since the user may click on a notification while the application is
 * not running, applications using #xnotification_t should be able to be
 * started as a D-Bus service, using #xapplication_t.
 *
 * In order for #xnotification_t to work, the application must have installed
 * a `.desktop` file. For example:
 * |[
 *  [Desktop Entry]
 *   Name=test_t Application
 *   Comment=Description of what test_t Application does
 *   Exec=gnome-test-application
 *   Icon=org.gnome.TestApplication
 *   Terminal=false
 *   Type=Application
 *   Categories=GNOME;GTK;TestApplication Category;
 *   StartupNotify=true
 *   DBusActivatable=true
 *   X-GNOME-UsesNotifications=true
 * ]|
 *
 * The `X-GNOME-UsesNotifications` key indicates to GNOME Control Center
 * that this application uses notifications, so it can be listed in the
 * Control Center’s ‘Notifications’ panel.
 *
 * The `.desktop` file must be named as `org.gnome.TestApplication.desktop`,
 * where `org.gnome.TestApplication` is the ID passed to xapplication_new().
 *
 * User interaction with a notification (either the default action, or
 * buttons) must be associated with actions on the application (ie:
 * "app." actions).  It is not possible to route user interaction
 * through the notification itself, because the object will not exist if
 * the application is autostarted as a result of a notification being
 * clicked.
 *
 * A notification can be sent with xapplication_send_notification().
 *
 * Since: 2.40
 **/

/**
 * xnotification_t:
 *
 * This structure type is private and should only be accessed using the
 * public APIs.
 *
 * Since: 2.40
 **/

typedef xobject_class_t GNotificationClass;

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

G_DEFINE_TYPE (xnotification, xnotification, XTYPE_OBJECT)

static void
button_free (xpointer_t data)
{
  Button *button = data;

  g_free (button->label);
  g_free (button->action_name);
  if (button->target)
    xvariant_unref (button->target);

  g_slice_free (Button, button);
}

static void
xnotification_dispose (xobject_t *object)
{
  xnotification_t *notification = G_NOTIFICATION (object);

  g_clear_object (&notification->icon);

  G_OBJECT_CLASS (xnotification_parent_class)->dispose (object);
}

static void
xnotification_finalize (xobject_t *object)
{
  xnotification_t *notification = G_NOTIFICATION (object);

  g_free (notification->title);
  g_free (notification->body);
  g_free (notification->category);
  g_free (notification->default_action);
  if (notification->default_action_target)
    xvariant_unref (notification->default_action_target);
  xptr_array_free (notification->buttons, TRUE);

  G_OBJECT_CLASS (xnotification_parent_class)->finalize (object);
}

static void
xnotification_class_init (GNotificationClass *klass)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = xnotification_dispose;
  object_class->finalize = xnotification_finalize;
}

static void
xnotification_init (xnotification_t *notification)
{
  notification->buttons = xptr_array_new_full (2, button_free);
}

/**
 * xnotification_new:
 * @title: the title of the notification
 *
 * Creates a new #xnotification_t with @title as its title.
 *
 * After populating @notification with more details, it can be sent to
 * the desktop shell with xapplication_send_notification(). Changing
 * any properties after this call will not have any effect until
 * resending @notification.
 *
 * Returns: a new #xnotification_t instance
 *
 * Since: 2.40
 */
xnotification_t *
xnotification_new (const xchar_t *title)
{
  xnotification_t *notification;

  g_return_val_if_fail (title != NULL, NULL);

  notification = xobject_new (XTYPE_NOTIFICATION, NULL);
  notification->title = xstrdup (title);

  return notification;
}

/*< private >
 * xnotification_get_title:
 * @notification: a #xnotification_t
 *
 * Gets the title of @notification.
 *
 * Returns: the title of @notification
 *
 * Since: 2.40
 */
const xchar_t *
xnotification_get_title (xnotification_t *notification)
{
  g_return_val_if_fail (X_IS_NOTIFICATION (notification), NULL);

  return notification->title;
}

/**
 * xnotification_set_title:
 * @notification: a #xnotification_t
 * @title: the new title for @notification
 *
 * Sets the title of @notification to @title.
 *
 * Since: 2.40
 */
void
xnotification_set_title (xnotification_t *notification,
                          const xchar_t   *title)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));
  g_return_if_fail (title != NULL);

  g_free (notification->title);

  notification->title = xstrdup (title);
}

/*< private >
 * xnotification_get_body:
 * @notification: a #xnotification_t
 *
 * Gets the current body of @notification.
 *
 * Returns: (nullable): the body of @notification
 *
 * Since: 2.40
 */
const xchar_t *
xnotification_get_body (xnotification_t *notification)
{
  g_return_val_if_fail (X_IS_NOTIFICATION (notification), NULL);

  return notification->body;
}

/**
 * xnotification_set_body:
 * @notification: a #xnotification_t
 * @body: (nullable): the new body for @notification, or %NULL
 *
 * Sets the body of @notification to @body.
 *
 * Since: 2.40
 */
void
xnotification_set_body (xnotification_t *notification,
                         const xchar_t   *body)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));
  g_return_if_fail (body != NULL);

  g_free (notification->body);

  notification->body = xstrdup (body);
}

/*< private >
 * xnotification_get_icon:
 * @notification: a #xnotification_t
 *
 * Gets the icon currently set on @notification.
 *
 * Returns: (transfer none): the icon associated with @notification
 *
 * Since: 2.40
 */
xicon_t *
xnotification_get_icon (xnotification_t *notification)
{
  g_return_val_if_fail (X_IS_NOTIFICATION (notification), NULL);

  return notification->icon;
}

/**
 * xnotification_set_icon:
 * @notification: a #xnotification_t
 * @icon: the icon to be shown in @notification, as a #xicon_t
 *
 * Sets the icon of @notification to @icon.
 *
 * Since: 2.40
 */
void
xnotification_set_icon (xnotification_t *notification,
                         xicon_t         *icon)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));

  if (notification->icon)
    xobject_unref (notification->icon);

  notification->icon = xobject_ref (icon);
}

/*< private >
 * xnotification_get_priority:
 * @notification: a #xnotification_t
 *
 * Returns the priority of @notification
 *
 * Since: 2.42
 */
GNotificationPriority
xnotification_get_priority (xnotification_t *notification)
{
  g_return_val_if_fail (X_IS_NOTIFICATION (notification), G_NOTIFICATION_PRIORITY_NORMAL);

  return notification->priority;
}

/**
 * xnotification_set_urgent:
 * @notification: a #xnotification_t
 * @urgent: %TRUE if @notification is urgent
 *
 * Deprecated in favor of xnotification_set_priority().
 *
 * Since: 2.40
 * Deprecated: 2.42: Since 2.42, this has been deprecated in favour of
 *    xnotification_set_priority().
 */
void
xnotification_set_urgent (xnotification_t *notification,
                           xboolean_t       urgent)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));

  notification->priority = urgent ?
      G_NOTIFICATION_PRIORITY_URGENT :
      G_NOTIFICATION_PRIORITY_NORMAL;
}

/*< private >
 * xnotification_get_category:
 * @notification: a #xnotification_t
 *
 * Gets the cateogry of @notification.
 *
 * This will be %NULL if no category is set.
 *
 * Returns: (nullable): the cateogry of @notification
 *
 * Since: 2.70
 */
const xchar_t *
xnotification_get_category (xnotification_t *notification)
{
  g_return_val_if_fail (X_IS_NOTIFICATION (notification), NULL);

  return notification->category;
}

/**
 * xnotification_set_category:
 * @notification: a #xnotification_t
 * @category: (nullable): the category for @notification, or %NULL for no category
 *
 * Sets the type of @notification to @category. Categories have a main
 * type like `email`, `im` or `device` and can have a detail separated
 * by a `.`, e.g. `im.received` or `email.arrived`. Setting the category
 * helps the notification server to select proper feedback to the user.
 *
 * Standard categories are [listed in the specification](https://specifications.freedesktop.org/notification-spec/latest/ar01s06.html).
 *
 * Since: 2.70
 */
void
xnotification_set_category (xnotification_t *notification,
                             const xchar_t   *category)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));
  g_return_if_fail (category == NULL || *category != '\0');

  g_free (notification->category);

  notification->category = xstrdup (category);
}

/**
 * xnotification_set_priority:
 * @notification: a #xnotification_t
 * @priority: a #GNotificationPriority
 *
 * Sets the priority of @notification to @priority. See
 * #GNotificationPriority for possible values.
 */
void
xnotification_set_priority (xnotification_t         *notification,
                             GNotificationPriority  priority)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));

  notification->priority = priority;
}

/**
 * xnotification_add_button:
 * @notification: a #xnotification_t
 * @label: label of the button
 * @detailed_action: a detailed action name
 *
 * Adds a button to @notification that activates the action in
 * @detailed_action when clicked. That action must be an
 * application-wide action (starting with "app."). If @detailed_action
 * contains a target, the action will be activated with that target as
 * its parameter.
 *
 * See g_action_parse_detailed_name() for a description of the format
 * for @detailed_action.
 *
 * Since: 2.40
 */
void
xnotification_add_button (xnotification_t *notification,
                           const xchar_t   *label,
                           const xchar_t   *detailed_action)
{
  xchar_t *action;
  xvariant_t *target;
  xerror_t *error = NULL;

  g_return_if_fail (detailed_action != NULL);

  if (!g_action_parse_detailed_name (detailed_action, &action, &target, &error))
    {
      g_warning ("%s: %s", G_STRFUNC, error->message);
      xerror_free (error);
      return;
    }

  xnotification_add_button_with_target_value (notification, label, action, target);

  g_free (action);
  if (target)
    xvariant_unref (target);
}

/**
 * xnotification_add_button_with_target: (skip)
 * @notification: a #xnotification_t
 * @label: label of the button
 * @action: an action name
 * @target_format: (nullable): a #xvariant_t format string, or %NULL
 * @...: positional parameters, as determined by @target_format
 *
 * Adds a button to @notification that activates @action when clicked.
 * @action must be an application-wide action (it must start with "app.").
 *
 * If @target_format is given, it is used to collect remaining
 * positional parameters into a #xvariant_t instance, similar to
 * xvariant_new(). @action will be activated with that #xvariant_t as its
 * parameter.
 *
 * Since: 2.40
 */
void
xnotification_add_button_with_target (xnotification_t *notification,
                                       const xchar_t   *label,
                                       const xchar_t   *action,
                                       const xchar_t   *target_format,
                                       ...)
{
  va_list args;
  xvariant_t *target = NULL;

  if (target_format)
    {
      va_start (args, target_format);
      target = xvariant_new_va (target_format, NULL, &args);
      va_end (args);
    }

  xnotification_add_button_with_target_value (notification, label, action, target);
}

/**
 * xnotification_add_button_with_target_value: (rename-to xnotification_add_button_with_target)
 * @notification: a #xnotification_t
 * @label: label of the button
 * @action: an action name
 * @target: (nullable): a #xvariant_t to use as @action's parameter, or %NULL
 *
 * Adds a button to @notification that activates @action when clicked.
 * @action must be an application-wide action (it must start with "app.").
 *
 * If @target is non-%NULL, @action will be activated with @target as
 * its parameter.
 *
 * Since: 2.40
 */
void
xnotification_add_button_with_target_value (xnotification_t *notification,
                                             const xchar_t   *label,
                                             const xchar_t   *action,
                                             xvariant_t      *target)
{
  Button *button;

  g_return_if_fail (X_IS_NOTIFICATION (notification));
  g_return_if_fail (label != NULL);
  g_return_if_fail (action != NULL && g_action_name_is_valid (action));

  if (!xstr_has_prefix (action, "app."))
    {
      g_warning ("%s: action '%s' does not start with 'app.'."
                 "This is unlikely to work properly.", G_STRFUNC, action);
    }

  button =  g_slice_new0 (Button);
  button->label = xstrdup (label);
  button->action_name = xstrdup (action);

  if (target)
    button->target = xvariant_ref_sink (target);

  xptr_array_add (notification->buttons, button);
}

/*< private >
 * xnotification_get_n_buttons:
 * @notification: a #xnotification_t
 *
 * Returns: the amount of buttons added to @notification.
 */
xuint_t
xnotification_get_n_buttons (xnotification_t *notification)
{
  return notification->buttons->len;
}

/*< private >
 * xnotification_get_button:
 * @notification: a #xnotification_t
 * @index: index of the button
 * @label: (): return location for the button's label
 * @action: (): return location for the button's associated action
 * @target: (): return location for the target @action should be
 * activated with
 *
 * Returns a description of a button that was added to @notification
 * with xnotification_add_button().
 *
 * @index must be smaller than the value returned by
 * xnotification_get_n_buttons().
 */
void
xnotification_get_button (xnotification_t  *notification,
                           xint_t            index,
                           xchar_t         **label,
                           xchar_t         **action,
                           xvariant_t      **target)
{
  Button *button;

  button = xptr_array_index (notification->buttons, index);

  if (label)
    *label = xstrdup (button->label);

  if (action)
    *action = xstrdup (button->action_name);

  if (target)
    *target = button->target ? xvariant_ref (button->target) : NULL;
}

/*< private >
 * xnotification_get_button_with_action:
 * @notification: a #xnotification_t
 * @action: an action name
 *
 * Returns the index of the button in @notification that is associated
 * with @action, or -1 if no such button exists.
 */
xint_t
xnotification_get_button_with_action (xnotification_t *notification,
                                       const xchar_t   *action)
{
  xuint_t i;

  for (i = 0; i < notification->buttons->len; i++)
    {
      Button *button;

      button = xptr_array_index (notification->buttons, i);
      if (xstr_equal (action, button->action_name))
        return i;
    }

  return -1;
}


/*< private >
 * xnotification_get_default_action:
 * @notification: a #xnotification_t
 * @action: (nullable): return location for the default action
 * @target: (nullable): return location for the target of the default action
 *
 * Gets the action and target for the default action of @notification.
 *
 * Returns: %TRUE if @notification has a default action
 */
xboolean_t
xnotification_get_default_action (xnotification_t  *notification,
                                   xchar_t         **action,
                                   xvariant_t      **target)
{
  if (notification->default_action == NULL)
    return FALSE;

  if (action)
    *action = xstrdup (notification->default_action);

  if (target)
    {
      if (notification->default_action_target)
        *target = xvariant_ref (notification->default_action_target);
      else
        *target = NULL;
    }

  return TRUE;
}

/**
 * xnotification_set_default_action:
 * @notification: a #xnotification_t
 * @detailed_action: a detailed action name
 *
 * Sets the default action of @notification to @detailed_action. This
 * action is activated when the notification is clicked on.
 *
 * The action in @detailed_action must be an application-wide action (it
 * must start with "app."). If @detailed_action contains a target, the
 * given action will be activated with that target as its parameter.
 * See g_action_parse_detailed_name() for a description of the format
 * for @detailed_action.
 *
 * When no default action is set, the application that the notification
 * was sent on is activated.
 *
 * Since: 2.40
 */
void
xnotification_set_default_action (xnotification_t *notification,
                                   const xchar_t   *detailed_action)
{
  xchar_t *action;
  xvariant_t *target;
  xerror_t *error = NULL;

  if (!g_action_parse_detailed_name (detailed_action, &action, &target, &error))
    {
      g_warning ("%s: %s", G_STRFUNC, error->message);
      xerror_free (error);
      return;
    }

  xnotification_set_default_action_and_target_value (notification, action, target);

  g_free (action);
  if (target)
    xvariant_unref (target);
}

/**
 * xnotification_set_default_action_and_target: (skip)
 * @notification: a #xnotification_t
 * @action: an action name
 * @target_format: (nullable): a #xvariant_t format string, or %NULL
 * @...: positional parameters, as determined by @target_format
 *
 * Sets the default action of @notification to @action. This action is
 * activated when the notification is clicked on. It must be an
 * application-wide action (it must start with "app.").
 *
 * If @target_format is given, it is used to collect remaining
 * positional parameters into a #xvariant_t instance, similar to
 * xvariant_new(). @action will be activated with that #xvariant_t as its
 * parameter.
 *
 * When no default action is set, the application that the notification
 * was sent on is activated.
 *
 * Since: 2.40
 */
void
xnotification_set_default_action_and_target (xnotification_t *notification,
                                              const xchar_t   *action,
                                              const xchar_t   *target_format,
                                              ...)
{
  va_list args;
  xvariant_t *target = NULL;

  if (target_format)
    {
      va_start (args, target_format);
      target = xvariant_new_va (target_format, NULL, &args);
      va_end (args);
    }

  xnotification_set_default_action_and_target_value (notification, action, target);
}

/**
 * xnotification_set_default_action_and_target_value: (rename-to xnotification_set_default_action_and_target)
 * @notification: a #xnotification_t
 * @action: an action name
 * @target: (nullable): a #xvariant_t to use as @action's parameter, or %NULL
 *
 * Sets the default action of @notification to @action. This action is
 * activated when the notification is clicked on. It must be an
 * application-wide action (start with "app.").
 *
 * If @target is non-%NULL, @action will be activated with @target as
 * its parameter.
 *
 * When no default action is set, the application that the notification
 * was sent on is activated.
 *
 * Since: 2.40
 */
void
xnotification_set_default_action_and_target_value (xnotification_t *notification,
                                                    const xchar_t   *action,
                                                    xvariant_t      *target)
{
  g_return_if_fail (X_IS_NOTIFICATION (notification));
  g_return_if_fail (action != NULL && g_action_name_is_valid (action));

  if (!xstr_has_prefix (action, "app."))
    {
      g_warning ("%s: action '%s' does not start with 'app.'."
                 "This is unlikely to work properly.", G_STRFUNC, action);
    }

  g_free (notification->default_action);
  g_clear_pointer (&notification->default_action_target, xvariant_unref);

  notification->default_action = xstrdup (action);

  if (target)
    notification->default_action_target = xvariant_ref_sink (target);
}

static xvariant_t *
xnotification_serialize_button (Button *button)
{
  xvariant_builder_t builder;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));

  xvariant_builder_add (&builder, "{sv}", "label", xvariant_new_string (button->label));
  xvariant_builder_add (&builder, "{sv}", "action", xvariant_new_string (button->action_name));

  if (button->target)
    xvariant_builder_add (&builder, "{sv}", "target", button->target);

  return xvariant_builder_end (&builder);
}

static xvariant_t *
xnotification_get_priority_nick (xnotification_t *notification)
{
  xenum_class_t *enum_class;
  xenum_value_t *value;
  xvariant_t *nick;

  enum_class = xtype_class_ref (XTYPE_NOTIFICATION_PRIORITY);
  value = xenum_get_value (enum_class, xnotification_get_priority (notification));
  g_assert (value != NULL);
  nick = xvariant_new_string (value->value_nick);
  xtype_class_unref (enum_class);

  return nick;
}

/*< private >
 * xnotification_serialize:
 *
 * Serializes @notification into a floating variant of type a{sv}.
 *
 * Returns: the serialized @notification as a floating variant.
 */
xvariant_t *
xnotification_serialize (xnotification_t *notification)
{
  xvariant_builder_t builder;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));

  if (notification->title)
    xvariant_builder_add (&builder, "{sv}", "title", xvariant_new_string (notification->title));

  if (notification->body)
    xvariant_builder_add (&builder, "{sv}", "body", xvariant_new_string (notification->body));

  if (notification->icon)
    {
      xvariant_t *serialized_icon;

      if ((serialized_icon = xicon_serialize (notification->icon)))
        {
          xvariant_builder_add (&builder, "{sv}", "icon", serialized_icon);
          xvariant_unref (serialized_icon);
        }
    }

  xvariant_builder_add (&builder, "{sv}", "priority", xnotification_get_priority_nick (notification));

  if (notification->default_action)
    {
      xvariant_builder_add (&builder, "{sv}", "default-action",
                                               xvariant_new_string (notification->default_action));

      if (notification->default_action_target)
        xvariant_builder_add (&builder, "{sv}", "default-action-target",
                                                  notification->default_action_target);
    }

  if (notification->buttons->len > 0)
    {
      xvariant_builder_t actions_builder;
      xuint_t i;

      xvariant_builder_init (&actions_builder, G_VARIANT_TYPE ("aa{sv}"));

      for (i = 0; i < notification->buttons->len; i++)
        {
          Button *button = xptr_array_index (notification->buttons, i);
          xvariant_builder_add (&actions_builder, "@a{sv}", xnotification_serialize_button (button));
        }

      xvariant_builder_add (&builder, "{sv}", "buttons", xvariant_builder_end (&actions_builder));
    }

  return xvariant_builder_end (&builder);
}
