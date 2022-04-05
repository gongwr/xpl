/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#ifndef __G_SRV_TARGET_H__
#define __G_SRV_TARGET_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xtype_t g_srv_target_get_type (void) G_GNUC_CONST;
#define XTYPE_SRV_TARGET (g_srv_target_get_type ())

XPL_AVAILABLE_IN_ALL
GSrvTarget  *g_srv_target_new          (const xchar_t *hostname,
				        guint16      port,
				        guint16      priority,
				        guint16      weight);
XPL_AVAILABLE_IN_ALL
GSrvTarget  *g_srv_target_copy         (GSrvTarget  *target);
XPL_AVAILABLE_IN_ALL
void         g_srv_target_free         (GSrvTarget  *target);

XPL_AVAILABLE_IN_ALL
const xchar_t *g_srv_target_get_hostname (GSrvTarget  *target);
XPL_AVAILABLE_IN_ALL
guint16      g_srv_target_get_port     (GSrvTarget  *target);
XPL_AVAILABLE_IN_ALL
guint16      g_srv_target_get_priority (GSrvTarget  *target);
XPL_AVAILABLE_IN_ALL
guint16      g_srv_target_get_weight   (GSrvTarget  *target);

XPL_AVAILABLE_IN_ALL
xlist_t       *g_srv_target_list_sort    (xlist_t       *targets);

G_END_DECLS

#endif /* __G_SRV_TARGET_H__ */
