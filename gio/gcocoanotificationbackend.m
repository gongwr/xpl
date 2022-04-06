/*
 * Copyright © 2015 Patrick Griffis
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
 * Authors: Patrick Griffis
 */

#include "config.h"

#import <Cocoa/Cocoa.h>
#include "gnotificationbackend.h"
#include "gapplication.h"
#include "gaction.h"
#include "gactiongroup.h"
#include "giomodule-priv.h"
#include "gnotification-private.h"
#include "gthemedicon.h"
#include "gfileicon.h"
#include "gfile.h"

#define XTYPE_COCOA_NOTIFICATION_BACKEND  (g_cocoa_notification_backend_get_type ())
#define G_COCOA_NOTIFICATION_BACKEND(o)    (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_COCOA_NOTIFICATION_BACKEND, GCocoaNotificationBackend))

typedef struct _GCocoaNotificationBackend GCocoaNotificationBackend;
typedef xnotification_backend_class_t            GCocoaNotificationBackendClass;
struct _GCocoaNotificationBackend
{
  xnotification_backend_t parent;
};

xtype_t g_cocoa_notification_backend_get_type (void);

G_DEFINE_TYPE_WITH_CODE (GCocoaNotificationBackend, g_cocoa_notification_backend, XTYPE_NOTIFICATION_BACKEND,
  _xio_modules_ensure_extension_points_registered ();
  g_io_extension_point_implement (G_NOTIFICATION_BACKEND_EXTENSION_POINT_NAME, g_define_type_id, "cocoa", 200));

static NSString *
nsstring_from_cstr (const char *cstr)
{
  if (!cstr)
    return nil;

  return [[NSString alloc] initWithUTF8String:cstr];
}

static NSImage*
nsimage_from_gicon (xicon_t *icon)
{
  if (X_IS_FILE_ICON (icon))
    {
      NSImage *image = nil;
      xfile_t *file;
      char *path;

      file = xfile_icon_get_file (XFILE_ICON (icon));
      path = xfile_get_path (file);
      if (path)
        {
          NSString *str_path = nsstring_from_cstr (path);
          image = [[NSImage alloc] initByReferencingFile:str_path];

          [str_path release];
          g_free (path);
        }
      return image;
    }
  else
    {
      g_warning ("This icon type is not handled by this NotificationBackend");
      return nil;
    }
}

static void
activate_detailed_action (const char * action)
{
  char *name;
  xvariant_t *target;

  if (!xstr_has_prefix (action, "app."))
    {
      g_warning ("Notification action does not have \"app.\" prefix");
      return;
    }

  if (g_action_parse_detailed_name (action, &name, &target, NULL))
    {
      xaction_group_activate_action (XACTION_GROUP (xapplication_get_default()), name + 4, target);
      g_free (name);
      if (target)
        xvariant_unref (target);
    }
}

@interface GNotificationCenterDelegate : NSObject<NSUserNotificationCenterDelegate> @end
@implementation GNotificationCenterDelegate

-(void) userNotificationCenter:(NSUserNotificationCenter*) center
       didActivateNotification:(NSUserNotification*)       notification
{
  if ([notification activationType] == NSUserNotificationActivationTypeContentsClicked)
    {
      const char *action = [[notification userInfo][@"default"] UTF8String];
      if (action)
        activate_detailed_action (action);
      /* OSX Always activates the front window */
    }
  else if ([notification activationType] == NSUserNotificationActivationTypeActionButtonClicked)
    {
      const char *action = [[notification userInfo][@"button0"] UTF8String];
      if (action)
        activate_detailed_action (action);
    }

  [center removeDeliveredNotification:notification];
}

@end

static GNotificationCenterDelegate *cocoa_notification_delegate;

static xboolean_t
g_cocoa_notification_backend_is_supported (void)
{
  NSBundle *bundle = [NSBundle mainBundle];

  /* This is always actually supported, but without a bundle it does nothing */
  if (![bundle bundleIdentifier])
    return FALSE;

  return TRUE;
}

static void
add_actions_to_notification (NSUserNotification   *userNotification,
                             xnotification_t        *notification)
{
  xuint_t n_buttons = xnotification_get_n_buttons (notification);
  char *action = NULL, *label = NULL;
  xvariant_t *target = NULL;
  NSMutableDictionary *user_info = nil;

  if (xnotification_get_default_action (notification, &action, &target))
    {
      char *detailed_name = g_action_print_detailed_name (action, target);
      NSString *action_name = nsstring_from_cstr (detailed_name);
      user_info = [[NSMutableDictionary alloc] init];

      user_info[@"default"] = action_name;

      [action_name release];
      g_free (detailed_name);
      g_clear_pointer (&action, g_free);
      g_clear_pointer (&target, xvariant_unref);
    }

  if (n_buttons)
    {
      xnotification_get_button (notification, 0, &label, &action, &target);
      if (label)
        {
          NSString *str_label = nsstring_from_cstr (label);
          char *detailed_name = g_action_print_detailed_name (action, target);
          NSString *action_name = nsstring_from_cstr (detailed_name);

          if (!user_info)
            user_info = [[NSMutableDictionary alloc] init];

          user_info[@"button0"] = action_name;
          userNotification.actionButtonTitle = str_label;

          [str_label release];
          [action_name release];
          g_free (label);
          g_free (action);
          g_free (detailed_name);
          g_clear_pointer (&target, xvariant_unref);
        }

      if (n_buttons > 1)
        g_warning ("Only a single button is currently supported by this NotificationBackend");
    }

    userNotification.userInfo = user_info;
    [user_info release];
}

static void
g_cocoa_notification_backend_send_notification (xnotification_backend_t *backend,
                                                const xchar_t          *cstr_id,
                                                xnotification_t        *notification)
{
  NSString *str_title = nil, *str_text = nil, *str_id = nil;
  NSImage *content = nil;
  const char *cstr;
  xicon_t *icon;
  NSUserNotification *userNotification;
  NSUserNotificationCenter *center;

  if ((cstr = xnotification_get_title (notification)))
    str_title = nsstring_from_cstr (cstr);
  if ((cstr = xnotification_get_body (notification)))
    str_text = nsstring_from_cstr (cstr);
  if (cstr_id != NULL)
    str_id = nsstring_from_cstr (cstr_id);
  if ((icon = xnotification_get_icon (notification)))
    content = nsimage_from_gicon (icon);
  /* NOTE: There is no priority */

  userNotification = [NSUserNotification new];
  userNotification.title = str_title;
  userNotification.informativeText = str_text;
  userNotification.identifier = str_id;
  userNotification.contentImage = content;
  /* NOTE: Buttons only show up if your bundle has NSUserNotificationAlertStyle set to "alerts" */
  add_actions_to_notification (userNotification, notification);

  if (!cocoa_notification_delegate)
    cocoa_notification_delegate = [[GNotificationCenterDelegate alloc] init];

  center = [NSUserNotificationCenter defaultUserNotificationCenter];
  center.delegate = cocoa_notification_delegate;
  [center deliverNotification:userNotification];

  [str_title release];
  [str_text release];
  [str_id release];
  [content release];
  [userNotification release];
}

static void
g_cocoa_notification_backend_withdraw_notification (xnotification_backend_t *backend,
                                                    const xchar_t          *cstr_id)
{
  NSUserNotificationCenter *center = [NSUserNotificationCenter defaultUserNotificationCenter];
  NSArray *notifications = [center deliveredNotifications];
  NSString *str_id = nsstring_from_cstr (cstr_id);

  for (NSUserNotification *notification in notifications)
    {
      if ([notification.identifier compare:str_id] == NSOrderedSame)
        {
          [center removeDeliveredNotification:notification];
          break;
        }
    }

  [str_id release];
}

static void
g_cocoa_notification_backend_init (GCocoaNotificationBackend *backend)
{
}

static void
g_cocoa_notification_backend_class_init (GCocoaNotificationBackendClass *klass)
{
  xnotification_backend_class_t *backend_class = G_NOTIFICATION_BACKEND_CLASS (klass);

  backend_class->is_supported = g_cocoa_notification_backend_is_supported;
  backend_class->send_notification = g_cocoa_notification_backend_send_notification;
  backend_class->withdraw_notification = g_cocoa_notification_backend_withdraw_notification;
}
