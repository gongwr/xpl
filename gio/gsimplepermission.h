/*
 * Copyright © 2010 Codethink Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_SIMPLE_PERMISSION_H__
#define __G_SIMPLE_PERMISSION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SIMPLE_PERMISSION      (g_simple_permission_get_type ())
#define G_SIMPLE_PERMISSION(inst)     (XTYPE_CHECK_INSTANCE_CAST ((inst),   \
                                       XTYPE_SIMPLE_PERMISSION,             \
                                       xsimple_permission))
#define X_IS_SIMPLE_PERMISSION(inst)  (XTYPE_CHECK_INSTANCE_TYPE ((inst),   \
                                       XTYPE_SIMPLE_PERMISSION))

XPL_AVAILABLE_IN_ALL
xtype_t                   g_simple_permission_get_type            (void);
XPL_AVAILABLE_IN_ALL
xpermission_t *           g_simple_permission_new                 (xboolean_t allowed);

G_END_DECLS

#endif /* __G_SIMPLE_PERMISSION_H__ */
