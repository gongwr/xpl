/* GIO - GLib Input, IO and Streaming Library
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

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include "glibintl.h"
#include "gioerror.h"
#include "gcancellable.h"
#include "glocalfileiostream.h"
#include "glocalfileinputstream.h"
#include "glocalfileinfo.h"

#ifdef G_OS_UNIX
#include "gfiledescriptorbased.h"
#endif


#define xlocal_file_io_stream_get_type _xlocal_file_io_stream_get_type
G_DEFINE_TYPE (xlocal_file_io_stream, xlocal_file_io_stream, XTYPE_FILE_IO_STREAM)

static void
xlocal_file_io_stream_finalize (xobject_t *object)
{
  xlocal_file_io_stream_t *file;

  file = G_LOCAL_FILE_IO_STREAM (object);

  xobject_unref (file->input_stream);
  xobject_unref (file->output_stream);

  G_OBJECT_CLASS (xlocal_file_io_stream_parent_class)->finalize (object);
}

xfile_io_stream_t *
_xlocal_file_io_stream_new (GLocalFileOutputStream *output_stream)
{
  xlocal_file_io_stream_t *stream;
  int fd;

  stream = xobject_new (XTYPE_LOCAL_FILE_IO_STREAM, NULL);
  stream->output_stream = xobject_ref (G_OUTPUT_STREAM (output_stream));
  _g_local_file_output_stream_set_do_close (output_stream, FALSE);
  fd = _g_local_file_output_stream_get_fd (output_stream);
  stream->input_stream = (xinput_stream_t *)_g_local_file_input_stream_new (fd);

  _g_local_file_input_stream_set_do_close (G_LOCAL_FILE_INPUT_STREAM (stream->input_stream),
					   FALSE);

  return XFILE_IO_STREAM (stream);
}

static xinput_stream_t *
xlocal_file_io_stream_get_input_stream (xio_stream_t *stream)
{
  return G_LOCAL_FILE_IO_STREAM (stream)->input_stream;
}

static xoutput_stream_t *
xlocal_file_io_stream_get_output_stream (xio_stream_t *stream)
{
  return G_LOCAL_FILE_IO_STREAM (stream)->output_stream;
}


static xboolean_t
xlocal_file_io_stream_close (xio_stream_t  *stream,
			      xcancellable_t   *cancellable,
			      xerror_t        **error)
{
  xlocal_file_io_stream_t *file = G_LOCAL_FILE_IO_STREAM (stream);

  /* There are shortcutted and can't fail */
  xoutput_stream_close (file->output_stream, cancellable, NULL);
  xinput_stream_close (file->input_stream, cancellable, NULL);

  return
    _g_local_file_output_stream_really_close (G_LOCAL_FILE_OUTPUT_STREAM (file->output_stream),
					      cancellable, error);
}

static void
xlocal_file_io_stream_class_init (xlocal_file_io_stream_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  xio_stream_class_t *stream_class = XIO_STREAM_CLASS (klass);

  gobject_class->finalize = xlocal_file_io_stream_finalize;

  stream_class->get_input_stream = xlocal_file_io_stream_get_input_stream;
  stream_class->get_output_stream = xlocal_file_io_stream_get_output_stream;
  stream_class->close_fn = xlocal_file_io_stream_close;
}

static void
xlocal_file_io_stream_init (xlocal_file_io_stream_t *stream)
{
}
