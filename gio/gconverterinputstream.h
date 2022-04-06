/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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

#ifndef __G_CONVERTER_INPUT_STREAM_H__
#define __G_CONVERTER_INPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gfilterinputstream.h>
#include <gio/gconverter.h>

G_BEGIN_DECLS

#define XTYPE_CONVERTER_INPUT_STREAM         (xconverter_input_stream_get_type ())
#define G_CONVERTER_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_CONVERTER_INPUT_STREAM, xconverter_input_stream))
#define G_CONVERTER_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_CONVERTER_INPUT_STREAM, xconverter_input_stream_class_t))
#define X_IS_CONVERTER_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_CONVERTER_INPUT_STREAM))
#define X_IS_CONVERTER_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_CONVERTER_INPUT_STREAM))
#define G_CONVERTER_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_CONVERTER_INPUT_STREAM, xconverter_input_stream_class_t))

/**
 * xconverter_input_stream_t:
 *
 * An implementation of #xfilter_input_stream_t that allows data
 * conversion.
 **/
typedef struct _xconverter_input_stream_class    xconverter_input_stream_class_t;
typedef struct _GConverterInputStreamPrivate  GConverterInputStreamPrivate;

struct _GConverterInputStream
{
  xfilter_input_stream_t parent_instance;

  /*< private >*/
  GConverterInputStreamPrivate *priv;
};

struct _xconverter_input_stream_class
{
  GFilterInputStreamClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t                  xconverter_input_stream_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xinput_stream_t          *xconverter_input_stream_new           (xinput_stream_t          *base_stream,
                                                               xconverter_t            *converter);
XPL_AVAILABLE_IN_ALL
xconverter_t            *xconverter_input_stream_get_converter (xconverter_input_stream_t *converter_stream);

G_END_DECLS

#endif /* __G_CONVERTER_INPUT_STREAM_H__ */
