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

#ifndef __G_ASYNC_RESULT_H__
#define __G_ASYNC_RESULT_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_ASYNC_RESULT            (xasync_result_get_type ())
#define G_ASYNC_RESULT(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_ASYNC_RESULT, xasync_result_t))
#define X_IS_ASYNC_RESULT(obj)	       (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_ASYNC_RESULT))
#define G_ASYNC_RESULT_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_ASYNC_RESULT, xasync_result_iface_t))

/**
 * xasync_result_t:
 *
 * Holds results information for an asynchronous operation,
 * usually passed directly to an asynchronous _finish() operation.
 **/
typedef struct _GAsyncResultIface    xasync_result_iface_t;


/**
 * xasync_result_iface_t:
 * @x_iface: The parent interface.
 * @get_user_data: Gets the user data passed to the callback.
 * @get_source_object: Gets the source object that issued the asynchronous operation.
 * @is_tagged: Checks if a result is tagged with a particular source.
 *
 * Interface definition for #xasync_result_t.
 **/
struct _GAsyncResultIface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xpointer_t  (* get_user_data)     (xasync_result_t *res);
  xobject_t * (* get_source_object) (xasync_result_t *res);

  xboolean_t  (* is_tagged)         (xasync_result_t *res,
				   xpointer_t      source_tag);
};

XPL_AVAILABLE_IN_ALL
xtype_t    xasync_result_get_type          (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xpointer_t xasync_result_get_user_data     (xasync_result_t *res);
XPL_AVAILABLE_IN_ALL
xobject_t *xasync_result_get_source_object (xasync_result_t *res);

XPL_AVAILABLE_IN_2_34
xboolean_t xasync_result_legacy_propagate_error (xasync_result_t  *res,
						xerror_t       **error);
XPL_AVAILABLE_IN_2_34
xboolean_t xasync_result_is_tagged              (xasync_result_t  *res,
						xpointer_t       source_tag);

G_END_DECLS

#endif /* __G_ASYNC_RESULT_H__ */
