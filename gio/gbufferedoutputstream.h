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
 * Author: Christian Kellner <gicmo@gnome.org>
 */

#ifndef __G_BUFFERED_OUTPUT_STREAM_H__
#define __G_BUFFERED_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gfilteroutputstream.h>

G_BEGIN_DECLS

#define XTYPE_BUFFERED_OUTPUT_STREAM         (xbuffered_output_stream_get_type ())
#define G_BUFFERED_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_BUFFERED_OUTPUT_STREAM, xbuffered_output_stream_t))
#define G_BUFFERED_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_BUFFERED_OUTPUT_STREAM, GBufferedOutputStreamClass))
#define X_IS_BUFFERED_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_BUFFERED_OUTPUT_STREAM))
#define X_IS_BUFFERED_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_BUFFERED_OUTPUT_STREAM))
#define G_BUFFERED_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_BUFFERED_OUTPUT_STREAM, GBufferedOutputStreamClass))

/**
 * xbuffered_output_stream_t:
 *
 * An implementation of #xfilter_output_stream_t with a sized buffer.
 **/
typedef struct _GBufferedOutputStreamClass    GBufferedOutputStreamClass;
typedef struct _GBufferedOutputStreamPrivate  GBufferedOutputStreamPrivate;

struct _GBufferedOutputStream
{
  xfilter_output_stream_t parent_instance;

  /*< protected >*/
  GBufferedOutputStreamPrivate *priv;
};

struct _GBufferedOutputStreamClass
{
  GFilterOutputStreamClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
};


XPL_AVAILABLE_IN_ALL
xtype_t          xbuffered_output_stream_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xoutput_stream_t* xbuffered_output_stream_new             (xoutput_stream_t         *base_stream);
XPL_AVAILABLE_IN_ALL
xoutput_stream_t* xbuffered_output_stream_new_sized       (xoutput_stream_t         *base_stream,
							 xsize_t                  size);
XPL_AVAILABLE_IN_ALL
xsize_t          xbuffered_output_stream_get_buffer_size (xbuffered_output_stream_t *stream);
XPL_AVAILABLE_IN_ALL
void           xbuffered_output_stream_set_buffer_size (xbuffered_output_stream_t *stream,
							 xsize_t                  size);
XPL_AVAILABLE_IN_ALL
xboolean_t       xbuffered_output_stream_get_auto_grow   (xbuffered_output_stream_t *stream);
XPL_AVAILABLE_IN_ALL
void           xbuffered_output_stream_set_auto_grow   (xbuffered_output_stream_t *stream,
							 xboolean_t               auto_grow);

G_END_DECLS

#endif /* __G_BUFFERED_OUTPUT_STREAM_H__ */
