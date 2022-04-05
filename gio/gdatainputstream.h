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

#ifndef __G_DATA_INPUT_STREAM_H__
#define __G_DATA_INPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gbufferedinputstream.h>

G_BEGIN_DECLS

#define XTYPE_DATA_INPUT_STREAM         (g_data_input_stream_get_type ())
#define G_DATA_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DATA_INPUT_STREAM, GDataInputStream))
#define G_DATA_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DATA_INPUT_STREAM, GDataInputStreamClass))
#define X_IS_DATA_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DATA_INPUT_STREAM))
#define X_IS_DATA_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DATA_INPUT_STREAM))
#define G_DATA_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DATA_INPUT_STREAM, GDataInputStreamClass))

/**
 * GDataInputStream:
 *
 * An implementation of #GBufferedInputStream that allows for high-level
 * data manipulation of arbitrary data (including binary operations).
 **/
typedef struct _GDataInputStreamClass    GDataInputStreamClass;
typedef struct _GDataInputStreamPrivate  GDataInputStreamPrivate;

struct _GDataInputStream
{
  GBufferedInputStream parent_instance;

  /*< private >*/
  GDataInputStreamPrivate *priv;
};

struct _GDataInputStreamClass
{
  GBufferedInputStreamClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t                  g_data_input_stream_get_type             (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
GDataInputStream *     g_data_input_stream_new                  (xinput_stream_t            *base_stream);

XPL_AVAILABLE_IN_ALL
void                   g_data_input_stream_set_byte_order       (GDataInputStream        *stream,
                                                                 GDataStreamByteOrder     order);
XPL_AVAILABLE_IN_ALL
GDataStreamByteOrder   g_data_input_stream_get_byte_order       (GDataInputStream        *stream);
XPL_AVAILABLE_IN_ALL
void                   g_data_input_stream_set_newline_type     (GDataInputStream        *stream,
                                                                 GDataStreamNewlineType   type);
XPL_AVAILABLE_IN_ALL
GDataStreamNewlineType g_data_input_stream_get_newline_type     (GDataInputStream        *stream);
XPL_AVAILABLE_IN_ALL
guchar                 g_data_input_stream_read_byte            (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gint16                 g_data_input_stream_read_int16           (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
guint16                g_data_input_stream_read_uint16          (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gint32                 g_data_input_stream_read_int32           (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
guint32                g_data_input_stream_read_uint32          (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gint64                 g_data_input_stream_read_int64           (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
guint64                g_data_input_stream_read_uint64          (GDataInputStream        *stream,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
char *                 g_data_input_stream_read_line            (GDataInputStream        *stream,
                                                                 xsize_t                   *length,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_2_30
char *                 g_data_input_stream_read_line_utf8       (GDataInputStream        *stream,
								 xsize_t                   *length,
								 xcancellable_t            *cancellable,
								 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
void                   g_data_input_stream_read_line_async      (GDataInputStream        *stream,
                                                                 xint_t                     io_priority,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);
XPL_AVAILABLE_IN_ALL
char *                 g_data_input_stream_read_line_finish     (GDataInputStream        *stream,
                                                                 xasync_result_t            *result,
                                                                 xsize_t                   *length,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_2_30
char *                 g_data_input_stream_read_line_finish_utf8(GDataInputStream        *stream,
                                                                 xasync_result_t            *result,
                                                                 xsize_t                   *length,
                                                                 xerror_t                 **error);
XPL_DEPRECATED_IN_2_56_FOR (g_data_input_stream_read_upto)
char *                 g_data_input_stream_read_until           (GDataInputStream        *stream,
                                                                 const xchar_t             *stop_chars,
                                                                 xsize_t                   *length,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_DEPRECATED_IN_2_56_FOR (g_data_input_stream_read_upto_async)
void                   g_data_input_stream_read_until_async     (GDataInputStream        *stream,
                                                                 const xchar_t             *stop_chars,
                                                                 xint_t                     io_priority,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);
XPL_DEPRECATED_IN_2_56_FOR (g_data_input_stream_read_upto_finish)
char *                 g_data_input_stream_read_until_finish    (GDataInputStream        *stream,
                                                                 xasync_result_t            *result,
                                                                 xsize_t                   *length,
                                                                 xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
char *                 g_data_input_stream_read_upto            (GDataInputStream        *stream,
                                                                 const xchar_t             *stop_chars,
                                                                 gssize                   stop_chars_len,
                                                                 xsize_t                   *length,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
void                   g_data_input_stream_read_upto_async      (GDataInputStream        *stream,
                                                                 const xchar_t             *stop_chars,
                                                                 gssize                   stop_chars_len,
                                                                 xint_t                     io_priority,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);
XPL_AVAILABLE_IN_ALL
char *                 g_data_input_stream_read_upto_finish     (GDataInputStream        *stream,
                                                                 xasync_result_t            *result,
                                                                 xsize_t                   *length,
                                                                 xerror_t                 **error);

G_END_DECLS

#endif /* __G_DATA_INPUT_STREAM_H__ */
