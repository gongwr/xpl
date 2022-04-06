/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2012 Red Hat, Inc.
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

#ifndef __G_POLLABLE_UTILS_H__
#define __G_POLLABLE_UTILS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gio.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xsource_t *g_pollable_source_new       (xobject_t        *pollable_stream);

XPL_AVAILABLE_IN_2_34
xsource_t *g_pollable_source_new_full  (xpointer_t        pollable_stream,
				      xsource_t        *child_source,
				      xcancellable_t   *cancellable);

XPL_AVAILABLE_IN_2_34
xssize_t   g_pollable_stream_read      (xinput_stream_t   *stream,
				      void           *buffer,
				      xsize_t           count,
				      xboolean_t        blocking,
				      xcancellable_t   *cancellable,
				      xerror_t        **error);

XPL_AVAILABLE_IN_2_34
xssize_t   g_pollable_stream_write     (xoutput_stream_t  *stream,
				      const void     *buffer,
				      xsize_t           count,
				      xboolean_t        blocking,
				      xcancellable_t   *cancellable,
				      xerror_t        **error);
XPL_AVAILABLE_IN_2_34
xboolean_t g_pollable_stream_write_all (xoutput_stream_t  *stream,
				      const void     *buffer,
				      xsize_t           count,
				      xboolean_t        blocking,
				      xsize_t          *bytes_written,
				      xcancellable_t   *cancellable,
				      xerror_t        **error);

G_END_DECLS

#endif /* _G_POLLABLE_UTILS_H_ */
