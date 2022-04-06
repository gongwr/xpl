/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2008 Novell, Inc.
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
 * Author: Tor Lillqvist <tml@novell.com>
 */

#include "config.h"

#include <glib.h>

#include "gio/gcancellable.h"
#include "gio/gioerror.h"
#include "gwinhttpfileinputstream.h"
#include "glibintl.h"

struct _GWinHttpFileInputStream
{
  xfile_input_stream_t parent_instance;

  GWinHttpFile *file;
  xboolean_t request_sent;
  HINTERNET connection;
  HINTERNET request;
};

struct _GWinHttpFileInputStreamClass
{
  GFileInputStreamClass parent_class;
};

#define g_winhttp_file_input_stream_get_type _g_winhttp_file_input_stream_get_type
G_DEFINE_TYPE (GWinHttpFileInputStream, g_winhttp_file_input_stream, XTYPE_FILE_INPUT_STREAM)

static xssize_t g_winhttp_file_input_stream_read (xinput_stream_t    *stream,
                                                void            *buffer,
                                                xsize_t            count,
                                                xcancellable_t    *cancellable,
                                                xerror_t         **error);

static xboolean_t g_winhttp_file_input_stream_close (xinput_stream_t  *stream,
						   xcancellable_t  *cancellable,
						   xerror_t       **error);

static void
g_winhttp_file_input_stream_finalize (xobject_t *object)
{
  GWinHttpFileInputStream *winhttp_stream;

  winhttp_stream = G_WINHTTP_FILE_INPUT_STREAM (object);

  if (winhttp_stream->request != NULL)
    G_WINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (winhttp_stream->request);
  if (winhttp_stream->connection != NULL)
    G_WINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (winhttp_stream->connection);

  xobject_unref (winhttp_stream->file);
  winhttp_stream->file = NULL;

  G_OBJECT_CLASS (g_winhttp_file_input_stream_parent_class)->finalize (object);
}

static void
g_winhttp_file_input_stream_class_init (GWinHttpFileInputStreamClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *stream_class = G_INPUT_STREAM_CLASS (klass);

  gobject_class->finalize = g_winhttp_file_input_stream_finalize;

  stream_class->read_fn = g_winhttp_file_input_stream_read;
  stream_class->close_fn = g_winhttp_file_input_stream_close;
}

static void
g_winhttp_file_input_stream_init (GWinHttpFileInputStream *info)
{
}

/*
 * g_winhttp_file_input_stream_new:
 * @file: the GWinHttpFile being read
 * @connection: handle to the HTTP connection, as from WinHttpConnect()
 * @request: handle to the HTTP request, as from WinHttpOpenRequest
 *
 * Returns: #xfile_input_stream_t for the given request
 */
xfile_input_stream_t *
_g_winhttp_file_input_stream_new (GWinHttpFile *file,
                                  HINTERNET     connection,
                                  HINTERNET     request)
{
  GWinHttpFileInputStream *stream;

  stream = xobject_new (XTYPE_WINHTTP_FILE_INPUT_STREAM, NULL);

  stream->file = xobject_ref (file);
  stream->request_sent = FALSE;
  stream->connection = connection;
  stream->request = request;

  return XFILE_INPUT_STREAM (stream);
}

static xssize_t
g_winhttp_file_input_stream_read (xinput_stream_t  *stream,
                                  void          *buffer,
                                  xsize_t          count,
                                  xcancellable_t  *cancellable,
                                  xerror_t       **error)
{
  GWinHttpFileInputStream *winhttp_stream = G_WINHTTP_FILE_INPUT_STREAM (stream);
  DWORD bytes_read;

  if (!winhttp_stream->request_sent)
    {
      if (!G_WINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpSendRequest
          (winhttp_stream->request,
           NULL, 0,
           NULL, 0,
           0,
           0))
        {
          _g_winhttp_set_error (error, GetLastError (), "GET request");

          return -1;
        }

      if (!_g_winhttp_response (winhttp_stream->file->vfs,
                                winhttp_stream->request,
                                error,
                                "GET request"))
        return -1;

      winhttp_stream->request_sent = TRUE;
    }

  if (!G_WINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpReadData
      (winhttp_stream->request, buffer, count, &bytes_read))
    {
      _g_winhttp_set_error (error, GetLastError (), "GET request");

      return -1;
    }

  return bytes_read;
}

static xboolean_t
g_winhttp_file_input_stream_close (xinput_stream_t         *stream,
				   xcancellable_t         *cancellable,
				   xerror_t              **error)
{
  GWinHttpFileInputStream *winhttp_stream = G_WINHTTP_FILE_INPUT_STREAM (stream);

  if (winhttp_stream->connection != NULL)
    G_WINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (winhttp_stream->connection);
  winhttp_stream->connection = NULL;
  return TRUE;
}
