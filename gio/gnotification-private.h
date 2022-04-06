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

const xchar_t *           xnotification_get_id                           (xnotification_t *notification);

const xchar_t *           xnotification_get_title                        (xnotification_t *notification);

const xchar_t *           xnotification_get_body                         (xnotification_t *notification);

const xchar_t *           xnotification_get_category                     (xnotification_t *notification);

xicon_t *                 xnotification_get_icon                         (xnotification_t *notification);

GNotificationPriority   xnotification_get_priority                     (xnotification_t *notification);

xuint_t                   xnotification_get_n_buttons                    (xnotification_t *notification);

void                    xnotification_get_button                       (xnotification_t  *notification,
                                                                         xint_t            index,
                                                                         xchar_t         **label,
                                                                         xchar_t         **action,
                                                                         xvariant_t      **target);

xint_t                    xnotification_get_button_with_action           (xnotification_t *notification,
                                                                         const xchar_t   *action);

xboolean_t                xnotification_get_default_action               (xnotification_t  *notification,
                                                                         xchar_t         **action,
                                                                         xvariant_t      **target);

xvariant_t *              xnotification_serialize                        (xnotification_t *notification);

#endif
