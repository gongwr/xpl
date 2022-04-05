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

#ifndef __G_DATA_OUTPUT_STREAM_H__
#define __G_DATA_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gfilteroutputstream.h>

G_BEGIN_DECLS

#define XTYPE_DATA_OUTPUT_STREAM         (g_data_output_stream_get_type ())
#define G_DATA_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DATA_OUTPUT_STREAM, GDataOutputStream))
#define G_DATA_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DATA_OUTPUT_STREAM, GDataOutputStreamClass))
#define X_IS_DATA_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DATA_OUTPUT_STREAM))
#define X_IS_DATA_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DATA_OUTPUT_STREAM))
#define G_DATA_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DATA_OUTPUT_STREAM, GDataOutputStreamClass))

/**
 * GDataOutputStream:
 *
 * An implementation of #GBufferedOutputStream that allows for high-level
 * data manipulation of arbitrary data (including binary operations).
 **/
typedef struct _GDataOutputStream         GDataOutputStream;
typedef struct _GDataOutputStreamClass    GDataOutputStreamClass;
typedef struct _GDataOutputStreamPrivate  GDataOutputStreamPrivate;

struct _GDataOutputStream
{
  GFilterOutputStream parent_instance;

  /*< private >*/
  GDataOutputStreamPrivate *priv;
};

struct _GDataOutputStreamClass
{
  GFilterOutputStreamClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};


XPL_AVAILABLE_IN_ALL
xtype_t                g_data_output_stream_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
GDataOutputStream *  g_data_output_stream_new            (xoutput_stream_t         *base_stream);

XPL_AVAILABLE_IN_ALL
void                 g_data_output_stream_set_byte_order (GDataOutputStream     *stream,
							  GDataStreamByteOrder   order);
XPL_AVAILABLE_IN_ALL
GDataStreamByteOrder g_data_output_stream_get_byte_order (GDataOutputStream     *stream);

XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_byte       (GDataOutputStream     *stream,
							  guchar                 data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_int16      (GDataOutputStream     *stream,
							  gint16                 data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_uint16     (GDataOutputStream     *stream,
							  guint16                data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_int32      (GDataOutputStream     *stream,
							  gint32                 data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_uint32     (GDataOutputStream     *stream,
							  guint32                data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_int64      (GDataOutputStream     *stream,
							  gint64                 data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_uint64     (GDataOutputStream     *stream,
							  guint64                data,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_data_output_stream_put_string     (GDataOutputStream     *stream,
							  const char            *str,
							  xcancellable_t          *cancellable,
							  xerror_t               **error);

G_END_DECLS

#endif /* __G_DATA_OUTPUT_STREAM_H__ */
