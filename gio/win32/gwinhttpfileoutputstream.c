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
#include "gwinhttpfileoutputstream.h"
#include "glibintl.h"

struct _xwin_http_file_output_stream
{
  xfile_output_stream_t parent_instance;

  xwin_http_file_t *file;
  HINTERNET connection;
  xoffset_t offset;
};

struct _xwin_http_file_output_stream_class
{
  xfile_output_stream_class_t parent_class;
};

#define xwinhttp_file_output_stream_get_type _xwinhttp_file_output_stream_get_type
XDEFINE_TYPE (xwin_http_file_output_stream, xwinhttp_file_output_stream, XTYPE_FILE_OUTPUT_STREAM)

static xssize_t     xwinhttp_file_output_stream_write      (xoutput_stream_t     *stream,
                                                           const void        *buffer,
                                                           xsize_t              count,
                                                           xcancellable_t      *cancellable,
                                                           xerror_t           **error);

static void
xwinhttp_file_output_stream_finalize (xobject_t *object)
{
  xwin_http_file_output_stream_t *winhttp_stream;

  winhttp_stream = XWINHTTP_FILE_OUTPUT_STREAM (object);

  if (winhttp_stream->connection != NULL)
    XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (winhttp_stream->connection);

  XOBJECT_CLASS (xwinhttp_file_output_stream_parent_class)->finalize (object);
}

static void
xwinhttp_file_output_stream_class_init (xwin_http_file_output_stream_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
  xoutput_stream_class_t *stream_class = G_OUTPUT_STREAM_CLASS (klass);

  xobject_class->finalize = xwinhttp_file_output_stream_finalize;

  stream_class->write_fn = xwinhttp_file_output_stream_write;
}

static void
xwinhttp_file_output_stream_init (xwin_http_file_output_stream_t *info)
{
}

/*
 * xwinhttp_file_output_stream_new:
 * @file: the xwin_http_file_t being read
 * @connection: handle to the HTTP connection, as from WinHttpConnect()
 * @request: handle to the HTTP request, as from WinHttpOpenRequest
 *
 * Returns: #xfile_output_stream_t for the given request
 */
xfile_output_stream_t *
_xwinhttp_file_output_stream_new (xwin_http_file_t *file,
                                   HINTERNET     connection)
{
  xwin_http_file_output_stream_t *stream;

  stream = xobject_new (XTYPE_WINHTTP_FILE_OUTPUT_STREAM, NULL);

  stream->file = file;
  stream->connection = connection;
  stream->offset = 0;

  return XFILE_OUTPUT_STREAM (stream);
}

static xssize_t
xwinhttp_file_output_stream_write (xoutput_stream_t  *stream,
                                    const void     *buffer,
                                    xsize_t           count,
                                    xcancellable_t   *cancellable,
                                    xerror_t        **error)
{
  xwin_http_file_output_stream_t *winhttp_stream = XWINHTTP_FILE_OUTPUT_STREAM (stream);
  HINTERNET request;
  char *headers;
  wchar_t *wheaders;
  DWORD bytes_written;

  request = XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpOpenRequest
    (winhttp_stream->connection,
     L"PUT",
     winhttp_stream->file->url.lpszUrlPath,
     NULL,
     WINHTTP_NO_REFERER,
     NULL,
     winhttp_stream->file->url.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);

  if (request == NULL)
    {
      _g_winhttp_set_error (error, GetLastError (), "PUT request");

      return -1;
    }

  headers = xstrdup_printf ("Content-Range: bytes %" G_GINT64_FORMAT "-%" G_GINT64_FORMAT "/*\r\n",
                             winhttp_stream->offset, winhttp_stream->offset + count);
  wheaders = xutf8_to_utf16 (headers, -1, NULL, NULL, NULL);
  g_free (headers);

  if (!XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpSendRequest
      (request,
       wheaders, -1,
       NULL, 0,
       count,
       0))
    {
      _g_winhttp_set_error (error, GetLastError (), "PUT request");

      XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (request);
      g_free (wheaders);

      return -1;
    }

  g_free (wheaders);

  if (!XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpWriteData
      (request, buffer, count, &bytes_written))
    {
      _g_winhttp_set_error (error, GetLastError (), "PUT request");

      XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (request);

      return -1;
    }

  winhttp_stream->offset += bytes_written;

  if (!_g_winhttp_response (winhttp_stream->file->vfs,
                            request,
                            error,
                            "PUT request"))
    {
      XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (request);

      return -1;
    }

  XWINHTTP_VFS_GET_CLASS (winhttp_stream->file->vfs)->funcs->pWinHttpCloseHandle (request);

  return bytes_written;
}
