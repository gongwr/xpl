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

#ifndef __G_FILTER_INPUT_STREAM_H__
#define __G_FILTER_INPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/ginputstream.h>

G_BEGIN_DECLS

#define XTYPE_FILTER_INPUT_STREAM         (g_filter_input_stream_get_type ())
#define G_FILTER_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FILTER_INPUT_STREAM, xfilter_input_stream))
#define G_FILTER_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_FILTER_INPUT_STREAM, GFilterInputStreamClass))
#define X_IS_FILTER_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_FILTER_INPUT_STREAM))
#define X_IS_FILTER_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_FILTER_INPUT_STREAM))
#define G_FILTER_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_FILTER_INPUT_STREAM, GFilterInputStreamClass))

/**
 * xfilter_input_stream_t:
 *
 * A base class for all input streams that work on an underlying stream.
 **/
typedef struct _GFilterInputStreamClass    GFilterInputStreamClass;

struct _GFilterInputStream
{
  xinput_stream_t parent_instance;

  /*<protected >*/
  xinput_stream_t *base_stream;
};

struct _GFilterInputStreamClass
{
  GInputStreamClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
};


XPL_AVAILABLE_IN_ALL
xtype_t          g_filter_input_stream_get_type              (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xinput_stream_t * g_filter_input_stream_get_base_stream       (xfilter_input_stream_t *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_filter_input_stream_get_close_base_stream (xfilter_input_stream_t *stream);
XPL_AVAILABLE_IN_ALL
void           g_filter_input_stream_set_close_base_stream (xfilter_input_stream_t *stream,
                                                            xboolean_t            close_base);

G_END_DECLS

#endif /* __G_FILTER_INPUT_STREAM_H__ */
