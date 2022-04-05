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

#ifndef __G_FILE_OUTPUT_STREAM_H__
#define __G_FILE_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/goutputstream.h>

G_BEGIN_DECLS

#define XTYPE_FILE_OUTPUT_STREAM         (g_file_output_stream_get_type ())
#define G_FILE_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FILE_OUTPUT_STREAM, GFileOutputStream))
#define G_FILE_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_FILE_OUTPUT_STREAM, GFileOutputStreamClass))
#define X_IS_FILE_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_FILE_OUTPUT_STREAM))
#define X_IS_FILE_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_FILE_OUTPUT_STREAM))
#define G_FILE_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_FILE_OUTPUT_STREAM, GFileOutputStreamClass))

/**
 * GFileOutputStream:
 *
 * A subclass of xoutput_stream_t for opened files. This adds
 * a few file-specific operations and seeking and truncating.
 *
 * #GFileOutputStream implements GSeekable.
 **/
typedef struct _GFileOutputStreamClass    GFileOutputStreamClass;
typedef struct _GFileOutputStreamPrivate  GFileOutputStreamPrivate;

struct _GFileOutputStream
{
  xoutput_stream_t parent_instance;

  /*< private >*/
  GFileOutputStreamPrivate *priv;
};

struct _GFileOutputStreamClass
{
  GOutputStreamClass parent_class;

  goffset     (* tell)              (GFileOutputStream    *stream);
  xboolean_t    (* can_seek)          (GFileOutputStream    *stream);
  xboolean_t    (* seek)	            (GFileOutputStream    *stream,
                                     goffset               offset,
                                     GSeekType             type,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  xboolean_t    (* can_truncate)      (GFileOutputStream    *stream);
  xboolean_t    (* truncate_fn)       (GFileOutputStream    *stream,
                                     goffset               size,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  GFileInfo * (* query_info)        (GFileOutputStream    *stream,
                                     const char           *attributes,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  void        (* query_info_async)  (GFileOutputStream     *stream,
                                     const char            *attributes,
                                     int                   io_priority,
                                     xcancellable_t         *cancellable,
                                     xasync_ready_callback_t   callback,
                                     xpointer_t              user_data);
  GFileInfo * (* query_info_finish) (GFileOutputStream     *stream,
                                     xasync_result_t         *result,
                                     xerror_t              **error);
  char      * (* get_etag)          (GFileOutputStream    *stream);

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t      g_file_output_stream_get_type          (void) G_GNUC_CONST;


XPL_AVAILABLE_IN_ALL
GFileInfo *g_file_output_stream_query_info        (GFileOutputStream    *stream,
                                                   const char           *attributes,
                                                   xcancellable_t         *cancellable,
                                                   xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void       g_file_output_stream_query_info_async  (GFileOutputStream    *stream,
						   const char           *attributes,
						   int                   io_priority,
						   xcancellable_t         *cancellable,
						   xasync_ready_callback_t   callback,
						   xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
GFileInfo *g_file_output_stream_query_info_finish (GFileOutputStream    *stream,
						   xasync_result_t         *result,
						   xerror_t              **error);
XPL_AVAILABLE_IN_ALL
char *     g_file_output_stream_get_etag          (GFileOutputStream    *stream);

G_END_DECLS

#endif /* __G_FILE_FILE_OUTPUT_STREAM_H__ */
