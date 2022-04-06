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

#ifndef __XFILE_MONITOR_H__
#define __XFILE_MONITOR_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_FILE_MONITOR         (xfile_monitor_get_type ())
#define XFILE_MONITOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FILE_MONITOR, xfile_monitor))
#define XFILE_MONITOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_FILE_MONITOR, xfile_monitor_class_t))
#define X_IS_FILE_MONITOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_FILE_MONITOR))
#define X_IS_FILE_MONITOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_FILE_MONITOR))
#define XFILE_MONITOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_FILE_MONITOR, xfile_monitor_class_t))

typedef struct _GFileMonitorClass       xfile_monitor_class_t;
typedef struct _GFileMonitorPrivate	xfile_monitor_private_t;

/**
 * xfile_monitor_t:
 *
 * Watches for changes to a file.
 **/
struct _GFileMonitor
{
  xobject_t parent_instance;

  /*< private >*/
  xfile_monitor_private_t *priv;
};

struct _GFileMonitorClass
{
  xobject_class_t parent_class;

  /* Signals */
  void     (* changed) (xfile_monitor_t      *monitor,
                        xfile_t             *file,
                        xfile_t             *other_file,
                        xfile_monitor_event_t  event_type);

  /* Virtual Table */
  xboolean_t (* cancel)  (xfile_monitor_t      *monitor);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t    xfile_monitor_get_type       (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t xfile_monitor_cancel         (xfile_monitor_t      *monitor);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_monitor_is_cancelled   (xfile_monitor_t      *monitor);
XPL_AVAILABLE_IN_ALL
void     xfile_monitor_set_rate_limit (xfile_monitor_t      *monitor,
                                        xint_t               limit_msecs);


/* For implementations */
XPL_AVAILABLE_IN_ALL
void     xfile_monitor_emit_event     (xfile_monitor_t      *monitor,
                                        xfile_t             *child,
                                        xfile_t             *other_file,
                                        xfile_monitor_event_t  event_type);

G_END_DECLS

#endif /* __XFILE_MONITOR_H__ */
