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

#ifndef __G_NOTIFICATION_PRIVATE_H__
#define __G_NOTIFICATION_PRIVATE_H__

#include "gnotification.h"

const xchar_t *           g_notification_get_id                           (GNotification *notification);

const xchar_t *           g_notification_get_title                        (GNotification *notification);

const xchar_t *           g_notification_get_body                         (GNotification *notification);

const xchar_t *           g_notification_get_category                     (GNotification *notification);

xicon_t *                 g_notification_get_icon                         (GNotification *notification);

GNotificationPriority   g_notification_get_priority                     (GNotification *notification);

xuint_t                   g_notification_get_n_buttons                    (GNotification *notification);

void                    g_notification_get_button                       (GNotification  *notification,
                                                                         xint_t            index,
                                                                         xchar_t         **label,
                                                                         xchar_t         **action,
                                                                         xvariant_t      **target);

xint_t                    g_notification_get_button_with_action           (GNotification *notification,
                                                                         const xchar_t   *action);

xboolean_t                g_notification_get_default_action               (GNotification  *notification,
                                                                         xchar_t         **action,
                                                                         xvariant_t      **target);

xvariant_t *              g_notification_serialize                        (GNotification *notification);

#endif
