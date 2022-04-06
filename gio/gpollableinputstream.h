/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __G_POLLABLE_INPUT_STREAM_H__
#define __G_POLLABLE_INPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_POLLABLE_INPUT_STREAM               (g_pollable_input_stream_get_type ())
#define G_POLLABLE_INPUT_STREAM(obj)               (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_POLLABLE_INPUT_STREAM, xpollable_input_stream))
#define X_IS_POLLABLE_INPUT_STREAM(obj)            (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_POLLABLE_INPUT_STREAM))
#define G_POLLABLE_INPUT_STREAM_GET_INTERFACE(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_POLLABLE_INPUT_STREAM, GPollableInputStreamInterface))

/**
 * xpollable_input_stream_t:
 *
 * An interface for a #xinput_stream_t that can be polled for readability.
 *
 * Since: 2.28
 */
typedef struct _GPollableInputStreamInterface GPollableInputStreamInterface;

/**
 * GPollableInputStreamInterface:
 * @x_iface: The parent interface.
 * @can_poll: Checks if the #xpollable_input_stream_t instance is actually pollable
 * @is_readable: Checks if the stream is readable
 * @create_source: Creates a #xsource_t to poll the stream
 * @read_nonblocking: Does a non-blocking read or returns
 *   %G_IO_ERROR_WOULD_BLOCK
 *
 * The interface for pollable input streams.
 *
 * The default implementation of @can_poll always returns %TRUE.
 *
 * The default implementation of @read_nonblocking calls
 * g_pollable_input_stream_is_readable(), and then calls
 * xinput_stream_read() if it returns %TRUE. This means you only need
 * to override it if it is possible that your @is_readable
 * implementation may return %TRUE when the stream is not actually
 * readable.
 *
 * Since: 2.28
 */
struct _GPollableInputStreamInterface
{
  xtype_interface_t x_iface;

  /* Virtual Table */
  xboolean_t     (*can_poll)         (xpollable_input_stream_t  *stream);

  xboolean_t     (*is_readable)      (xpollable_input_stream_t  *stream);
  xsource_t *    (*create_source)    (xpollable_input_stream_t  *stream,
				    xcancellable_t          *cancellable);
  xssize_t       (*read_nonblocking) (xpollable_input_stream_t  *stream,
				    void                  *buffer,
				    xsize_t                  count,
				    xerror_t               **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t    g_pollable_input_stream_get_type         (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t g_pollable_input_stream_can_poll         (xpollable_input_stream_t  *stream);

XPL_AVAILABLE_IN_ALL
xboolean_t g_pollable_input_stream_is_readable      (xpollable_input_stream_t  *stream);
XPL_AVAILABLE_IN_ALL
xsource_t *g_pollable_input_stream_create_source    (xpollable_input_stream_t  *stream,
						   xcancellable_t          *cancellable);

XPL_AVAILABLE_IN_ALL
xssize_t   g_pollable_input_stream_read_nonblocking (xpollable_input_stream_t  *stream,
						   void                  *buffer,
						   xsize_t                  count,
						   xcancellable_t          *cancellable,
						   xerror_t               **error);

G_END_DECLS


#endif /* __G_POLLABLE_INPUT_STREAM_H__ */
