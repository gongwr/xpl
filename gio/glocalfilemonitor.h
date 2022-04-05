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
 */

#ifndef __G_LOCAL_FILE_MONITOR_H__
#define __G_LOCAL_FILE_MONITOR_H__

#include <gio/gfilemonitor.h>
#include "gunixmounts.h"

G_BEGIN_DECLS

#define XTYPE_LOCAL_FILE_MONITOR		(g_local_file_monitor_get_type ())
#define G_LOCAL_FILE_MONITOR(o)			(XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_LOCAL_FILE_MONITOR, GLocalFileMonitor))
#define G_LOCAL_FILE_MONITOR_CLASS(k)		(XTYPE_CHECK_CLASS_CAST ((k), XTYPE_LOCAL_FILE_MONITOR, GLocalFileMonitorClass))
#define X_IS_LOCAL_FILE_MONITOR(o)		(XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_LOCAL_FILE_MONITOR))
#define X_IS_LOCAL_FILE_MONITOR_CLASS(k)	(XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_LOCAL_FILE_MONITOR))
#define G_LOCAL_FILE_MONITOR_GET_CLASS(o)       (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_LOCAL_FILE_MONITOR, GLocalFileMonitorClass))

#define G_LOCAL_FILE_MONITOR_EXTENSION_POINT_NAME "gio-local-file-monitor"
#define G_NFS_FILE_MONITOR_EXTENSION_POINT_NAME   "gio-nfs-file-monitor"

typedef struct _GLocalFileMonitor      GLocalFileMonitor;
typedef struct _GLocalFileMonitorClass GLocalFileMonitorClass;
typedef struct _GFileMonitorSource     GFileMonitorSource;

struct _GLocalFileMonitor
{
  GFileMonitor parent_instance;

  GFileMonitorSource *source;
  GUnixMountMonitor  *mount_monitor;
  xboolean_t            was_mounted;
};

struct _GLocalFileMonitorClass
{
  GFileMonitorClass parent_class;

  xboolean_t (* is_supported) (void);
  void     (* start)        (GLocalFileMonitor *local_monitor,
                             const xchar_t *dirname,
                             const xchar_t *basename,
                             const xchar_t *filename,
                             GFileMonitorSource *source);

  xboolean_t mount_notify;
};

#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_ALL
#endif
xtype_t           g_local_file_monitor_get_type (void) G_GNUC_CONST;

/* for glocalfile.c */
GFileMonitor *
g_local_file_monitor_new_for_path (const xchar_t        *pathname,
                                   xboolean_t            is_directory,
                                   GFileMonitorFlags   flags,
                                   xerror_t            **error);

/* for various users in glib */
typedef void (* GFileMonitorCallback) (GFileMonitor      *monitor,
                                       xfile_t             *child,
                                       xfile_t             *other,
                                       GFileMonitorEvent  event,
                                       xpointer_t           user_data);
GFileMonitor *
g_local_file_monitor_new_in_worker (const xchar_t           *pathname,
                                    xboolean_t               is_directory,
                                    GFileMonitorFlags      flags,
                                    GFileMonitorCallback   callback,
                                    xpointer_t               user_data,
                                    GClosureNotify         destroy_user_data,
                                    xerror_t               **error);

/* for implementations of GLocalFileMonitor */
XPL_AVAILABLE_IN_2_44
xboolean_t
g_file_monitor_source_handle_event (GFileMonitorSource *fms,
                                    GFileMonitorEvent   event_type,
                                    const xchar_t        *child,
                                    const xchar_t        *rename_to,
                                    xfile_t              *other,
                                    gint64              event_time);


G_END_DECLS

#endif /* __G_LOCAL_FILE_MONITOR_H__ */
