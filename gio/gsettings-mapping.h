/*
 * Copyright © 2010 Novell, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Vincent Untz <vuntz@gnome.org>
 */

#ifndef __G_SETTINGS_MAPPING_H__
#define __G_SETTINGS_MAPPING_H__

#include <glib-object.h>

xvariant_t *              g_settings_set_mapping                          (const GValue       *value,
                                                                         const xvariant_type_t *expected_type,
                                                                         xpointer_t            user_data);
xboolean_t                g_settings_get_mapping                          (GValue             *value,
                                                                         xvariant_t           *variant,
                                                                         xpointer_t            user_data);
xboolean_t                g_settings_mapping_is_compatible                (xtype_t               gvalue_type,
                                                                         const xvariant_type_t *variant_type);

#endif /* __G_SETTINGS_MAPPING_H__ */
