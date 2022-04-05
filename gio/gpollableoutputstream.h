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

#ifndef __G_POLLABLE_OUTPUT_STREAM_H__
#define __G_POLLABLE_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_POLLABLE_OUTPUT_STREAM               (g_pollable_output_stream_get_type ())
#define G_POLLABLE_OUTPUT_STREAM(obj)               (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_POLLABLE_OUTPUT_STREAM, GPollableOutputStream))
#define X_IS_POLLABLE_OUTPUT_STREAM(obj)            (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_POLLABLE_OUTPUT_STREAM))
#define G_POLLABLE_OUTPUT_STREAM_GET_INTERFACE(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_POLLABLE_OUTPUT_STREAM, GPollableOutputStreamInterface))

/**
 * GPollableOutputStream:
 *
 * An interface for a #xoutput_stream_t that can be polled for writeability.
 *
 * Since: 2.28
 */
typedef struct _GPollableOutputStreamInterface GPollableOutputStreamInterface;

/**
 * GPollableOutputStreamInterface:
 * @x_iface: The parent interface.
 * @can_poll: Checks if the #GPollableOutputStream instance is actually pollable
 * @is_writable: Checks if the stream is writable
 * @create_source: Creates a #GSource to poll the stream
 * @write_nonblocking: Does a non-blocking write or returns
 *   %G_IO_ERROR_WOULD_BLOCK
 * @writev_nonblocking: Does a vectored non-blocking write, or returns
 *   %G_POLLABLE_RETURN_WOULD_BLOCK
 *
 * The interface for pollable output streams.
 *
 * The default implementation of @can_poll always returns %TRUE.
 *
 * The default implementation of @write_nonblocking calls
 * g_pollable_output_stream_is_writable(), and then calls
 * g_output_stream_write() if it returns %TRUE. This means you only
 * need to override it if it is possible that your @is_writable
 * implementation may return %TRUE when the stream is not actually
 * writable.
 *
 * The default implementation of @writev_nonblocking calls
 * g_pollable_output_stream_write_nonblocking() for each vector, and converts
 * its return value and error (if set) to a #GPollableReturn. You should
 * override this where possible to avoid having to allocate a #xerror_t to return
 * %G_IO_ERROR_WOULD_BLOCK.
 *
 * Since: 2.28
 */
struct _GPollableOutputStreamInterface
{
  xtype_interface_t x_iface;

  /* Virtual Table */
  xboolean_t     (*can_poll)          (GPollableOutputStream  *stream);

  xboolean_t     (*is_writable)       (GPollableOutputStream  *stream);
  GSource *    (*create_source)     (GPollableOutputStream  *stream,
				     xcancellable_t           *cancellable);
  gssize       (*write_nonblocking) (GPollableOutputStream  *stream,
				     const void             *buffer,
				     xsize_t                   count,
				     xerror_t                **error);
  GPollableReturn (*writev_nonblocking) (GPollableOutputStream  *stream,
					 const GOutputVector    *vectors,
					 xsize_t                   n_vectors,
					 xsize_t                  *bytes_written,
					 xerror_t                **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t    g_pollable_output_stream_get_type          (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t g_pollable_output_stream_can_poll          (GPollableOutputStream  *stream);

XPL_AVAILABLE_IN_ALL
xboolean_t g_pollable_output_stream_is_writable       (GPollableOutputStream  *stream);
XPL_AVAILABLE_IN_ALL
GSource *g_pollable_output_stream_create_source     (GPollableOutputStream  *stream,
						     xcancellable_t           *cancellable);

XPL_AVAILABLE_IN_ALL
gssize   g_pollable_output_stream_write_nonblocking (GPollableOutputStream  *stream,
						     const void             *buffer,
						     xsize_t                   count,
						     xcancellable_t           *cancellable,
						     xerror_t                **error);

XPL_AVAILABLE_IN_2_60
GPollableReturn g_pollable_output_stream_writev_nonblocking (GPollableOutputStream  *stream,
							     const GOutputVector    *vectors,
							     xsize_t                   n_vectors,
							     xsize_t                  *bytes_written,
							     xcancellable_t           *cancellable,
							     xerror_t                **error);

G_END_DECLS


#endif /* __G_POLLABLE_OUTPUT_STREAM_H__ */
