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

#ifndef __XFILE_OUTPUT_STREAM_H__
#define __XFILE_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/goutputstream.h>

G_BEGIN_DECLS

#define XTYPE_FILE_OUTPUT_STREAM         (xfile_output_stream_get_type ())
#define XFILE_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FILE_OUTPUT_STREAM, xfile_output_stream))
#define XFILE_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_FILE_OUTPUT_STREAM, GFileOutputStreamClass))
#define X_IS_FILE_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_FILE_OUTPUT_STREAM))
#define X_IS_FILE_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_FILE_OUTPUT_STREAM))
#define XFILE_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_FILE_OUTPUT_STREAM, GFileOutputStreamClass))

/**
 * xfile_output_stream_t:
 *
 * A subclass of xoutput_stream_t for opened files. This adds
 * a few file-specific operations and seeking and truncating.
 *
 * #xfile_output_stream_t implements xseekable__t.
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
  xoutput_stream_class_t parent_class;

  xoffset_t     (* tell)              (xfile_output_stream_t    *stream);
  xboolean_t    (* can_seek)          (xfile_output_stream_t    *stream);
  xboolean_t    (* seek)	            (xfile_output_stream_t    *stream,
                                     xoffset_t               offset,
                                     xseek_type_t             type,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  xboolean_t    (* can_truncate)      (xfile_output_stream_t    *stream);
  xboolean_t    (* truncate_fn)       (xfile_output_stream_t    *stream,
                                     xoffset_t               size,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  xfile_info_t * (* query_info)        (xfile_output_stream_t    *stream,
                                     const char           *attributes,
                                     xcancellable_t         *cancellable,
                                     xerror_t              **error);
  void        (* query_info_async)  (xfile_output_stream_t     *stream,
                                     const char            *attributes,
                                     int                   io_priority,
                                     xcancellable_t         *cancellable,
                                     xasync_ready_callback_t   callback,
                                     xpointer_t              user_data);
  xfile_info_t * (* query_info_finish) (xfile_output_stream_t     *stream,
                                     xasync_result_t         *result,
                                     xerror_t              **error);
  char      * (* get_etag)          (xfile_output_stream_t    *stream);

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t      xfile_output_stream_get_type          (void) G_GNUC_CONST;


XPL_AVAILABLE_IN_ALL
xfile_info_t *xfile_output_stream_query_info        (xfile_output_stream_t    *stream,
                                                   const char           *attributes,
                                                   xcancellable_t         *cancellable,
                                                   xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void       xfile_output_stream_query_info_async  (xfile_output_stream_t    *stream,
						   const char           *attributes,
						   int                   io_priority,
						   xcancellable_t         *cancellable,
						   xasync_ready_callback_t   callback,
						   xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xfile_info_t *xfile_output_stream_query_info_finish (xfile_output_stream_t    *stream,
						   xasync_result_t         *result,
						   xerror_t              **error);
XPL_AVAILABLE_IN_ALL
char *     xfile_output_stream_get_etag          (xfile_output_stream_t    *stream);

G_END_DECLS

#endif /* __XFILE_FILE_OUTPUT_STREAM_H__ */
