/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2009 Red Hat, Inc.
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

#ifndef __G_LOCAL_FILE_IO_STREAM_H__
#define __G_LOCAL_FILE_IO_STREAM_H__

#include <gio/gfileiostream.h>
#include "glocalfileoutputstream.h"

G_BEGIN_DECLS

#define XTYPE_LOCAL_FILE_IO_STREAM         (_g_local_file_io_stream_get_type ())
#define G_LOCAL_FILE_IO_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_LOCAL_FILE_IO_STREAM, GLocalFileIOStream))
#define G_LOCAL_FILE_IO_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_LOCAL_FILE_IO_STREAM, GLocalFileIOStreamClass))
#define X_IS_LOCAL_FILE_IO_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_LOCAL_FILE_IO_STREAM))
#define X_IS_LOCAL_FILE_IO_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_LOCAL_FILE_IO_STREAM))
#define G_LOCAL_FILE_IO_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_LOCAL_FILE_IO_STREAM, GLocalFileIOStreamClass))

typedef struct _GLocalFileIOStream         GLocalFileIOStream;
typedef struct _GLocalFileIOStreamClass    GLocalFileIOStreamClass;
typedef struct _GLocalFileIOStreamPrivate  GLocalFileIOStreamPrivate;

struct _GLocalFileIOStream
{
  GFileIOStream parent_instance;

  xinput_stream_t *input_stream;
  xoutput_stream_t *output_stream;
};

struct _GLocalFileIOStreamClass
{
  GFileIOStreamClass parent_class;
};

xtype_t           _g_local_file_io_stream_get_type (void) G_GNUC_CONST;
GFileIOStream * _g_local_file_io_stream_new (GLocalFileOutputStream *output_stream);

G_END_DECLS

#endif /* __G_LOCAL_FILE_IO_STREAM_H__ */
