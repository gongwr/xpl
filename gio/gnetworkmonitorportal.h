/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2016 Red Hat, Inc.
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

#ifndef __G_NETWORK_MONITOR_PORTAL_H__
#define __G_NETWORK_MONITOR_PORTAL_H__

#include "gnetworkmonitorbase.h"

G_BEGIN_DECLS

#define XTYPE_NETWORK_MONITOR_PORTAL         (xnetwork_monitor_portal_get_type ())
#define G_NETWORK_MONITOR_PORTAL(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_NETWORK_MONITOR_PORTAL, GNetworkMonitorPortal))
#define G_NETWORK_MONITOR_PORTAL_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_NETWORK_MONITOR_PORTAL, GNetworkMonitorPortalClass))
#define X_IS_NETWORK_MONITOR_PORTAL(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_NETWORK_MONITOR_PORTAL))
#define X_IS_NETWORK_MONITOR_PORTAL_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_NETWORK_MONITOR_PORTAL))
#define G_NETWORK_MONITOR_PORTAL_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_NETWORK_MONITOR_PORTAL, GNetworkMonitorPortalClass))
#define G_NETWORK_MONITOR_PORTAL_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, xinitable_t))


typedef struct _GNetworkMonitorPortal        GNetworkMonitorPortal;
typedef struct _GNetworkMonitorPortalClass   GNetworkMonitorPortalClass;
typedef struct _GNetworkMonitorPortalPrivate GNetworkMonitorPortalPrivate;

struct _GNetworkMonitorPortal {
  xnetwork_monitor_base_t parent_instance;

  GNetworkMonitorPortalPrivate *priv;
};

struct _GNetworkMonitorPortalClass {
  xnetwork_monitor_base_class_t parent_class;
};

xtype_t xnetwork_monitor_portal_get_type (void);

G_END_DECLS

#endif /* __G_NETWORK_MONITOR_PORTAL_H__ */
