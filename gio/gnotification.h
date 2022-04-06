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

#ifndef __G_NOTIFICATION_H__
#define __G_NOTIFICATION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>
#include <gio/gioenums.h>

G_BEGIN_DECLS

#define XTYPE_NOTIFICATION         (xnotification_get_type ())
#define G_NOTIFICATION(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_NOTIFICATION, xnotification))
#define X_IS_NOTIFICATION(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_NOTIFICATION))

XPL_AVAILABLE_IN_2_40
xtype_t                   xnotification_get_type                         (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_40
xnotification_t *         xnotification_new                              (const xchar_t *title);

XPL_AVAILABLE_IN_2_40
void                    xnotification_set_title                        (xnotification_t *notification,
                                                                         const xchar_t   *title);

XPL_AVAILABLE_IN_2_40
void                    xnotification_set_body                         (xnotification_t *notification,
                                                                         const xchar_t   *body);

XPL_AVAILABLE_IN_2_40
void                    xnotification_set_icon                         (xnotification_t *notification,
                                                                         xicon_t         *icon);

XPL_DEPRECATED_IN_2_42_FOR(xnotification_set_priority)
void                    xnotification_set_urgent                       (xnotification_t *notification,
                                                                         xboolean_t       urgent);

XPL_AVAILABLE_IN_2_42
void                    xnotification_set_priority                     (xnotification_t         *notification,
                                                                         GNotificationPriority  priority);

XPL_AVAILABLE_IN_2_70
void                    xnotification_set_category                     (xnotification_t *notification,
                                                                         const xchar_t   *category);

XPL_AVAILABLE_IN_2_40
void                    xnotification_add_button                       (xnotification_t *notification,
                                                                         const xchar_t   *label,
                                                                         const xchar_t   *detailed_action);

XPL_AVAILABLE_IN_2_40
void                    xnotification_add_button_with_target           (xnotification_t *notification,
                                                                         const xchar_t   *label,
                                                                         const xchar_t   *action,
                                                                         const xchar_t   *target_format,
                                                                         ...);

XPL_AVAILABLE_IN_2_40
void                    xnotification_add_button_with_target_value     (xnotification_t *notification,
                                                                         const xchar_t   *label,
                                                                         const xchar_t   *action,
                                                                         xvariant_t      *target);

XPL_AVAILABLE_IN_2_40
void                    xnotification_set_default_action               (xnotification_t *notification,
                                                                         const xchar_t   *detailed_action);

XPL_AVAILABLE_IN_2_40
void                    xnotification_set_default_action_and_target    (xnotification_t *notification,
                                                                         const xchar_t   *action,
                                                                         const xchar_t   *target_format,
                                                                         ...);

XPL_AVAILABLE_IN_2_40
void                 xnotification_set_default_action_and_target_value (xnotification_t *notification,
                                                                         const xchar_t   *action,
                                                                         xvariant_t      *target);

G_END_DECLS

#endif
