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

#ifndef __G_LOADABLE_ICON_H__
#define __G_LOADABLE_ICON_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_LOADABLE_ICON            (g_loadable_icon_get_type ())
#define G_LOADABLE_ICON(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_LOADABLE_ICON, GLoadableIcon))
#define X_IS_LOADABLE_ICON(obj)	        (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_LOADABLE_ICON))
#define G_LOADABLE_ICON_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_LOADABLE_ICON, GLoadableIconIface))

/**
 * GLoadableIcon:
 *
 * Generic type for all kinds of icons that can be loaded
 * as a stream.
 **/
typedef struct _GLoadableIconIface    		GLoadableIconIface;

/**
 * GLoadableIconIface:
 * @x_iface: The parent interface.
 * @load: Loads an icon.
 * @load_async: Loads an icon asynchronously.
 * @load_finish: Finishes an asynchronous icon load.
 *
 * Interface for icons that can be loaded as a stream.
 **/
struct _GLoadableIconIface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xinput_stream_t * (* load)        (GLoadableIcon       *icon,
                                  int                  size,
                                  char               **type,
                                  xcancellable_t        *cancellable,
                                  xerror_t             **error);
  void           (* load_async)  (GLoadableIcon       *icon,
                                  int                  size,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data);
  xinput_stream_t * (* load_finish) (GLoadableIcon       *icon,
                                  xasync_result_t        *res,
                                  char               **type,
                                  xerror_t             **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t         g_loadable_icon_get_type    (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xinput_stream_t *g_loadable_icon_load        (GLoadableIcon        *icon,
					   int                   size,
					   char                **type,
					   xcancellable_t         *cancellable,
					   xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void          g_loadable_icon_load_async  (GLoadableIcon        *icon,
					   int                   size,
					   xcancellable_t         *cancellable,
					   xasync_ready_callback_t   callback,
					   xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xinput_stream_t *g_loadable_icon_load_finish (GLoadableIcon        *icon,
					   xasync_result_t         *res,
					   char                **type,
					   xerror_t              **error);

G_END_DECLS

#endif /* __G_LOADABLE_ICON_H__ */
