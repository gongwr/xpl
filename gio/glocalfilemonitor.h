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
#define G_LOCAL_FILE_MONITOR(o)			(XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_LOCAL_FILE_MONITOR, xlocal_file_monitor))
#define G_LOCAL_FILE_MONITOR_CLASS(k)		(XTYPE_CHECK_CLASS_CAST ((k), XTYPE_LOCAL_FILE_MONITOR, xlocal_file_monitor_class))
#define X_IS_LOCAL_FILE_MONITOR(o)		(XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_LOCAL_FILE_MONITOR))
#define X_IS_LOCAL_FILE_MONITOR_CLASS(k)	(XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_LOCAL_FILE_MONITOR))
#define G_LOCAL_FILE_MONITOR_GET_CLASS(o)       (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_LOCAL_FILE_MONITOR, xlocal_file_monitor_class))

#define G_LOCAL_FILE_MONITOR_EXTENSION_POINT_NAME "gio-local-file-monitor"
#define G_NFS_FILE_MONITOR_EXTENSION_POINT_NAME   "gio-nfs-file-monitor"

typedef struct _xlocal_file_monitor_t      xlocal_file_monitor_t;
typedef struct _xlocal_file_monitor_class_t xlocal_file_monitor_class_t;
typedef struct _GFileMonitorSource     GFileMonitorSource;

struct _xlocal_file_monitor_t
{
  xfile_monitor_t parent_instance;

  GFileMonitorSource *source;
  xunix_mount_monitor_t  *mount_monitor;
  xboolean_t            was_mounted;
};

struct _xlocal_file_monitor_class_t
{
  xfile_monitor_class_t parent_class;

  xboolean_t (* is_supported) (void);
  void     (* start)        (xlocal_file_monitor_t *local_monitor,
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
xfile_monitor_t *
g_local_file_monitor_new_for_path (const xchar_t        *pathname,
                                   xboolean_t            is_directory,
                                   xfile_monitor_flags_t   flags,
                                   xerror_t            **error);

/* for various users in glib */
typedef void (* GFileMonitorCallback) (xfile_monitor_t      *monitor,
                                       xfile_t             *child,
                                       xfile_t             *other,
                                       xfile_monitor_event_t  event,
                                       xpointer_t           user_data);
xfile_monitor_t *
g_local_file_monitor_new_in_worker (const xchar_t           *pathname,
                                    xboolean_t               is_directory,
                                    xfile_monitor_flags_t      flags,
                                    GFileMonitorCallback   callback,
                                    xpointer_t               user_data,
                                    xclosure_notify_t         destroy_user_data,
                                    xerror_t               **error);

/* for implementations of xlocal_file_monitor_t */
XPL_AVAILABLE_IN_2_44
xboolean_t
xfile_monitor_source_handle_event (GFileMonitorSource *fms,
                                    xfile_monitor_event_t   event_type,
                                    const xchar_t        *child,
                                    const xchar_t        *rename_to,
                                    xfile_t              *other,
                                    sint64_t              event_time);


G_END_DECLS

#endif /* __G_LOCAL_FILE_MONITOR_H__ */
