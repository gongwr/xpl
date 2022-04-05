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

#ifndef __G_LOCAL_FILE_OUTPUT_STREAM_H__
#define __G_LOCAL_FILE_OUTPUT_STREAM_H__

#include <gio/gfileoutputstream.h>

G_BEGIN_DECLS

#define XTYPE_LOCAL_FILE_OUTPUT_STREAM         (_g_local_file_output_stream_get_type ())
#define G_LOCAL_FILE_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_LOCAL_FILE_OUTPUT_STREAM, GLocalFileOutputStream))
#define G_LOCAL_FILE_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_LOCAL_FILE_OUTPUT_STREAM, GLocalFileOutputStreamClass))
#define X_IS_LOCAL_FILE_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_LOCAL_FILE_OUTPUT_STREAM))
#define X_IS_LOCAL_FILE_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_LOCAL_FILE_OUTPUT_STREAM))
#define G_LOCAL_FILE_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_LOCAL_FILE_OUTPUT_STREAM, GLocalFileOutputStreamClass))

typedef struct _GLocalFileOutputStream         GLocalFileOutputStream;
typedef struct _GLocalFileOutputStreamClass    GLocalFileOutputStreamClass;
typedef struct _GLocalFileOutputStreamPrivate  GLocalFileOutputStreamPrivate;

struct _GLocalFileOutputStream
{
  GFileOutputStream parent_instance;

  /*< private >*/
  GLocalFileOutputStreamPrivate *priv;
};

struct _GLocalFileOutputStreamClass
{
  GFileOutputStreamClass parent_class;
};

xtype_t               _g_local_file_output_stream_get_type (void) G_GNUC_CONST;

void     _g_local_file_output_stream_set_do_close (GLocalFileOutputStream *out,
						   xboolean_t do_close);
xboolean_t _g_local_file_output_stream_really_close (GLocalFileOutputStream *out,
						   xcancellable_t   *cancellable,
						   xerror_t        **error);

GFileOutputStream * _g_local_file_output_stream_new      (int               fd);
GFileOutputStream * _g_local_file_output_stream_open     (const char       *filename,
							  xboolean_t          readable,
                                                          xcancellable_t     *cancellable,
                                                          xerror_t          **error);
GFileOutputStream * _g_local_file_output_stream_create   (const char       *filename,
							  xboolean_t          readable,
                                                          GFileCreateFlags  flags,
                                                          GFileInfo        *reference_info,
                                                          xcancellable_t     *cancellable,
                                                          xerror_t          **error);
GFileOutputStream * _g_local_file_output_stream_append   (const char       *filename,
                                                          GFileCreateFlags  flags,
                                                          xcancellable_t     *cancellable,
                                                          xerror_t          **error);
GFileOutputStream * _g_local_file_output_stream_replace  (const char       *filename,
							  xboolean_t          readable,
                                                          const char       *etag,
                                                          xboolean_t          create_backup,
                                                          GFileCreateFlags  flags,
                                                          GFileInfo        *reference_info,
                                                          xcancellable_t     *cancellable,
                                                          xerror_t          **error);

/* Hack to get the fd since GFileDescriptorBased (which is how you
 * _should_ get the fd) is only available on UNIX but things like
 * win32 needs this as well
 */
xint_t _g_local_file_output_stream_get_fd (GLocalFileOutputStream *output_stream);

G_END_DECLS

#endif /* __G_LOCAL_FILE_OUTPUT_STREAM_H__ */
