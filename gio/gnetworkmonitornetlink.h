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

#ifndef __XNETWORK_MONITOR_NETLINK_H__
#define __XNETWORK_MONITOR_NETLINK_H__

#include "gnetworkmonitorbase.h"

G_BEGIN_DECLS

#define XTYPE_NETWORK_MONITOR_NETLINK         (_xnetwork_monitor_netlink_get_type ())
#define XNETWORK_MONITOR_NETLINK(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_NETWORK_MONITOR_NETLINK, xnetwork_monitor_netlink_t))
#define XNETWORK_MONITOR_NETLINK_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_NETWORK_MONITOR_NETLINK, xnetwork_monitor_netlink_class_t))
#define X_IS_NETWORK_MONITOR_NETLINK(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_NETWORK_MONITOR_NETLINK))
#define X_IS_NETWORK_MONITOR_NETLINK_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_NETWORK_MONITOR_NETLINK))
#define XNETWORK_MONITOR_NETLINK_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_NETWORK_MONITOR_NETLINK, xnetwork_monitor_netlink_class_t))

typedef struct _xnetwork_monitor_netlink        xnetwork_monitor_netlink_t;
typedef struct _xnetwork_monitor_netlink_class   xnetwork_monitor_netlink_class_t;
typedef struct _xnetwork_monitor_netlink_private xnetwork_monitor_netlink_private_t;

struct _xnetwork_monitor_netlink {
  xnetwork_monitor_base_t parent_instance;

  xnetwork_monitor_netlink_private_t *priv;
};

struct _xnetwork_monitor_netlink_class {
  xnetwork_monitor_base_class_t parent_class;

  /*< private >*/
  /* Padding for future expansion */
  xpointer_t padding[8];
};

xtype_t _xnetwork_monitor_netlink_get_type (void);

G_END_DECLS

#endif /* __XNETWORK_MONITOR_NETLINK_H__ */
