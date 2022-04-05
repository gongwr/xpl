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

#ifndef __G_FILE_INPUT_STREAM_H__
#define __G_FILE_INPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/ginputstream.h>

G_BEGIN_DECLS

#define XTYPE_FILE_INPUT_STREAM         (g_file_input_stream_get_type ())
#define G_FILE_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FILE_INPUT_STREAM, GFileInputStream))
#define G_FILE_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_FILE_INPUT_STREAM, GFileInputStreamClass))
#define X_IS_FILE_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_FILE_INPUT_STREAM))
#define X_IS_FILE_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_FILE_INPUT_STREAM))
#define G_FILE_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_FILE_INPUT_STREAM, GFileInputStreamClass))

/**
 * GFileInputStream:
 *
 * A subclass of xinput_stream_t for opened files. This adds
 * a few file-specific operations and seeking.
 *
 * #GFileInputStream implements #GSeekable.
 **/
typedef struct _GFileInputStreamClass    GFileInputStreamClass;
typedef struct _GFileInputStreamPrivate  GFileInputStreamPrivate;

struct _GFileInputStream
{
  xinput_stream_t parent_instance;

  /*< private >*/
  GFileInputStreamPrivate *priv;
};

struct _GFileInputStreamClass
{
  GInputStreamClass parent_class;

  goffset     (* tell)              (GFileInputStream     *stream);
  xboolean_t    (* can_seek)          (GFileInputStream     *stream);
  xboolean_t    (* seek)	            (GFileInputStream     *stream,
                                     goffset               offset,
                                     GSeekType             type,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  GFileInfo * (* query_info)        (GFileInputStream     *stream,
                                     const char           *attributes,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  void        (* query_info_async)  (GFileInputStream     *stream,
                                     const char           *attributes,
                                     int                   io_priority,
                                     xcancellable_t         *cancellable,
                                     xasync_ready_callback_t   callback,
                                     xpointer_t              user_data);
  GFileInfo * (* query_info_finish) (GFileInputStream     *stream,
                                     xasync_result_t         *result,
                                     xerror_t              **error);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t      g_file_input_stream_get_type          (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GFileInfo *g_file_input_stream_query_info        (GFileInputStream     *stream,
						  const char           *attributes,
						  xcancellable_t         *cancellable,
						  xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void       g_file_input_stream_query_info_async  (GFileInputStream     *stream,
						  const char           *attributes,
						  int                   io_priority,
						  xcancellable_t         *cancellable,
						  xasync_ready_callback_t   callback,
						  xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
GFileInfo *g_file_input_stream_query_info_finish (GFileInputStream     *stream,
						  xasync_result_t         *result,
						  xerror_t              **error);

G_END_DECLS

#endif /* __G_FILE_FILE_INPUT_STREAM_H__ */
