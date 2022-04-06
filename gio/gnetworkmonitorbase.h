/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
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

#ifndef __G_NETWORK_MONITOR_BASE_H__
#define __G_NETWORK_MONITOR_BASE_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_NETWORK_MONITOR_BASE         (g_network_monitor_base_get_type ())
#define G_NETWORK_MONITOR_BASE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_NETWORK_MONITOR_BASE, xnetwork_monitor_base))
#define G_NETWORK_MONITOR_BASE_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_NETWORK_MONITOR_BASE, xnetwork_monitor_base_class))
#define X_IS_NETWORK_MONITOR_BASE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_NETWORK_MONITOR_BASE))
#define X_IS_NETWORK_MONITOR_BASE_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_NETWORK_MONITOR_BASE))
#define G_NETWORK_MONITOR_BASE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_NETWORK_MONITOR_BASE, xnetwork_monitor_base_class))

typedef struct _GNetworkMonitorBase        xnetwork_monitor_base_t;
typedef struct _Xnetwork_monitor_base_class_t   xnetwork_monitor_base_class_t;
typedef struct _GNetworkMonitorBasePrivate GNetworkMonitorBasePrivate;

struct _GNetworkMonitorBase {
  xobject_t parent_instance;

  GNetworkMonitorBasePrivate *priv;
};

struct _Xnetwork_monitor_base_class_t {
  xobject_class_t parent_class;

  /*< private >*/
  /* Padding for future expansion */
  xpointer_t padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t g_network_monitor_base_get_type (void);

/*< protected >*/
XPL_AVAILABLE_IN_2_32
void g_network_monitor_base_add_network    (xnetwork_monitor_base_t  *monitor,
					    xinet_address_mask_t     *network);
XPL_AVAILABLE_IN_2_32
void g_network_monitor_base_remove_network (xnetwork_monitor_base_t  *monitor,
					    xinet_address_mask_t     *network);
XPL_AVAILABLE_IN_ALL
void g_network_monitor_base_set_networks   (xnetwork_monitor_base_t  *monitor,
					    xinet_address_mask_t    **networks,
					    xint_t                  length);

G_END_DECLS

#endif /* __G_NETWORK_MONITOR_BASE_H__ */
