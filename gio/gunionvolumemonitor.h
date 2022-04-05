/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 *         David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_UNION_VOLUME_MONITOR_H__
#define __G_UNION_VOLUME_MONITOR_H__

#include <gio/gvolumemonitor.h>

G_BEGIN_DECLS

#define XTYPE_UNION_VOLUME_MONITOR        (_g_union_volume_monitor_get_type ())
#define G_UNION_VOLUME_MONITOR(o)          (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_UNION_VOLUME_MONITOR, GUnionVolumeMonitor))
#define G_UNION_VOLUME_MONITOR_CLASS(k)    (XTYPE_CHECK_CLASS_CAST((k), XTYPE_UNION_VOLUME_MONITOR, GUnionVolumeMonitorClass))
#define X_IS_UNION_VOLUME_MONITOR(o)       (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_UNION_VOLUME_MONITOR))
#define X_IS_UNION_VOLUME_MONITOR_CLASS(k) (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_UNION_VOLUME_MONITOR))

typedef struct _GUnionVolumeMonitor      GUnionVolumeMonitor;
typedef struct _GUnionVolumeMonitorClass GUnionVolumeMonitorClass;

struct _GUnionVolumeMonitorClass
{
  GVolumeMonitorClass parent_class;
};

xtype_t _g_union_volume_monitor_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __G_UNION_VOLUME_MONITOR_H__ */
