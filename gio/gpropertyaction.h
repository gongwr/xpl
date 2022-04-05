/*
 * Copyright © 2013 Canonical Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_PROPERTY_ACTION_H__
#define __G_PROPERTY_ACTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_PROPERTY_ACTION                              (g_property_action_get_type ())
#define G_PROPERTY_ACTION(inst)                             (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_PROPERTY_ACTION, GPropertyAction))
#define X_IS_PROPERTY_ACTION(inst)                          (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_PROPERTY_ACTION))

XPL_AVAILABLE_IN_2_38
xtype_t                   g_property_action_get_type                      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_38
GPropertyAction *       g_property_action_new                           (const xchar_t *name,
                                                                         xpointer_t     object,
                                                                         const xchar_t *property_name);

G_END_DECLS

#endif /* __G_PROPERTY_ACTION_H__ */
