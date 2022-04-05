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

#ifndef __G_SEEKABLE_H__
#define __G_SEEKABLE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SEEKABLE            (g_seekable_get_type ())
#define G_SEEKABLE(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_SEEKABLE, GSeekable))
#define X_IS_SEEKABLE(obj)         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_SEEKABLE))
#define G_SEEKABLE_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_SEEKABLE, GSeekableIface))

/**
 * GSeekable:
 *
 * Seek object for streaming operations.
 **/
typedef struct _GSeekableIface   GSeekableIface;

/**
 * GSeekableIface:
 * @x_iface: The parent interface.
 * @tell: Tells the current location within a stream.
 * @can_seek: Checks if seeking is supported by the stream.
 * @seek: Seeks to a location within a stream.
 * @can_truncate: Checks if truncation is supported by the stream.
 * @truncate_fn: Truncates a stream.
 *
 * Provides an interface for implementing seekable functionality on I/O Streams.
 **/
struct _GSeekableIface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  goffset     (* tell)	         (GSeekable    *seekable);

  xboolean_t    (* can_seek)       (GSeekable    *seekable);
  xboolean_t    (* seek)	         (GSeekable    *seekable,
				  goffset       offset,
				  GSeekType     type,
				  xcancellable_t *cancellable,
				  xerror_t      **error);

  xboolean_t    (* can_truncate)   (GSeekable    *seekable);
  xboolean_t    (* truncate_fn)    (GSeekable    *seekable,
				  goffset       offset,
				  xcancellable_t *cancellable,
				  xerror_t       **error);

  /* TODO: Async seek/truncate */
};

XPL_AVAILABLE_IN_ALL
xtype_t    g_seekable_get_type     (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
goffset  g_seekable_tell         (GSeekable     *seekable);
XPL_AVAILABLE_IN_ALL
xboolean_t g_seekable_can_seek     (GSeekable     *seekable);
XPL_AVAILABLE_IN_ALL
xboolean_t g_seekable_seek         (GSeekable     *seekable,
				  goffset        offset,
				  GSeekType      type,
				  xcancellable_t  *cancellable,
				  xerror_t       **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_seekable_can_truncate (GSeekable     *seekable);
XPL_AVAILABLE_IN_ALL
xboolean_t g_seekable_truncate     (GSeekable     *seekable,
				  goffset        offset,
				  xcancellable_t  *cancellable,
				  xerror_t       **error);

G_END_DECLS


#endif /* __G_SEEKABLE_H__ */
