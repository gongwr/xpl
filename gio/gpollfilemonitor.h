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

#ifndef __XPOLL_FILE_MONITOR_H__
#define __XPOLL_FILE_MONITOR_H__

#include <gio/gfilemonitor.h>

G_BEGIN_DECLS

#define XTYPE_POLL_FILE_MONITOR		(_xpoll_file_monitor_get_type ())
#define XPOLL_FILE_MONITOR(o)			(XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_POLL_FILE_MONITOR, xpoll_file_monitor_t))
#define XPOLL_FILE_MONITOR_CLASS(k)		(XTYPE_CHECK_CLASS_CAST ((k), XTYPE_POLL_FILE_MONITOR, xpoll_file_monitor_tClass))
#define X_IS_POLL_FILE_MONITOR(o)		(XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_POLL_FILE_MONITOR))
#define X_IS_POLL_FILE_MONITOR_CLASS(k)	(XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_POLL_FILE_MONITOR))

typedef struct _xpoll_file_monitor_t      xpoll_file_monitor_t;
typedef struct _xpoll_file_monitor_tClass xpoll_file_monitor_tClass;

struct _xpoll_file_monitor_tClass
{
  xfile_monitor_class_t parent_class;
};

xtype_t          _xpoll_file_monitor_get_type (void) G_GNUC_CONST;

xfile_monitor_t * _xpoll_file_monitor_new      (xfile_t *file);

G_END_DECLS

#endif /* __XPOLL_FILE_MONITOR_H__ */
