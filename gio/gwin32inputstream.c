/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2010 Red Hat, Inc.
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
 * Author: Tor Lillqvist <tml@iki.fi>
 */

#include "config.h"

#include <windows.h>

#include <io.h>

#include <glib.h>
#include "gioerror.h"
#include "gwin32inputstream.h"
#include "giowin32-priv.h"
#include "gcancellable.h"
#include "gasynchelper.h"
#include "glibintl.h"

/**
 * SECTION:gwin32inputstream
 * @short_description: Streaming input operations for Windows file handles
 * @include: gio/gwin32inputstream.h
 * @see_also: #xinput_stream_t
 *
 * #GWin32InputStream implements #xinput_stream_t for reading from a
 * Windows file handle.
 *
 * Note that `<gio/gwin32inputstream.h>` belongs to the Windows-specific GIO
 * interfaces, thus you have to use the `gio-windows-2.0.pc` pkg-config file
 * when using it.
 */

struct _GWin32InputStreamPrivate {
  HANDLE handle;
  xboolean_t close_handle;
  xint_t fd;
};

enum {
  PROP_0,
  PROP_HANDLE,
  PROP_CLOSE_HANDLE,
  LAST_PROP
};

static xparam_spec_t *props[LAST_PROP];

G_DEFINE_TYPE_WITH_PRIVATE (GWin32InputStream, g_win32_input_stream, XTYPE_INPUT_STREAM)

static void
g_win32_input_stream_set_property (xobject_t         *object,
				   xuint_t            prop_id,
				   const xvalue_t    *value,
				   xparam_spec_t      *pspec)
{
  GWin32InputStream *win32_stream;

  win32_stream = G_WIN32_INPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_HANDLE:
      win32_stream->priv->handle = xvalue_get_pointer (value);
      break;
    case PROP_CLOSE_HANDLE:
      win32_stream->priv->close_handle = xvalue_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_win32_input_stream_get_property (xobject_t    *object,
				   xuint_t       prop_id,
				   xvalue_t     *value,
				   xparam_spec_t *pspec)
{
  GWin32InputStream *win32_stream;

  win32_stream = G_WIN32_INPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_HANDLE:
      xvalue_set_pointer (value, win32_stream->priv->handle);
      break;
    case PROP_CLOSE_HANDLE:
      xvalue_set_boolean (value, win32_stream->priv->close_handle);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static xssize_t
g_win32_input_stream_read (xinput_stream_t  *stream,
			   void          *buffer,
			   xsize_t          count,
			   xcancellable_t  *cancellable,
			   xerror_t       **error)
{
  GWin32InputStream *win32_stream;
  BOOL res;
  DWORD nbytes, nread;
  OVERLAPPED overlap = { 0, };
  xssize_t retval = -1;

  win32_stream = G_WIN32_INPUT_STREAM (stream);

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return -1;

  if (count > G_MAXINT)
    nbytes = G_MAXINT;
  else
    nbytes = count;

  overlap.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
  g_return_val_if_fail (overlap.hEvent != NULL, -1);

  res = ReadFile (win32_stream->priv->handle, buffer, nbytes, &nread, &overlap);
  if (res)
    retval = nread;
  else
    {
      int errsv = GetLastError ();

      if (errsv == ERROR_IO_PENDING &&
          _g_win32_overlap_wait_result (win32_stream->priv->handle,
                                        &overlap, &nread, cancellable))
        {
          retval = nread;
          goto end;
        }

      if (xcancellable_set_error_if_cancelled (cancellable, error))
        goto end;

      errsv = GetLastError ();
      if (errsv == ERROR_MORE_DATA)
        {
          /* If a named pipe is being read in message mode and the
           * next message is longer than the nNumberOfBytesToRead
           * parameter specifies, ReadFile returns FALSE and
           * GetLastError returns ERROR_MORE_DATA */
          retval = nread;
          goto end;
        }
      else if (errsv == ERROR_HANDLE_EOF ||
               errsv == ERROR_BROKEN_PIPE)
        {
          /* TODO: the other end of a pipe may call the WriteFile
           * function with nNumberOfBytesToWrite set to zero. In this
           * case, it's not possible for the caller to know if it's
           * broken pipe or a read of 0. Perhaps we should add a
           * is_broken flag for this win32 case.. */
          retval = 0;
        }
      else
        {
          xchar_t *emsg;

          emsg = g_win32_error_message (errsv);
          g_set_error (error, G_IO_ERROR,
                       g_io_error_from_win32_error (errsv),
                       _("Error reading from handle: %s"),
                       emsg);
          g_free (emsg);
        }
    }

end:
  CloseHandle (overlap.hEvent);
  return retval;
}

static xboolean_t
g_win32_input_stream_close (xinput_stream_t  *stream,
			   xcancellable_t  *cancellable,
			   xerror_t       **error)
{
  GWin32InputStream *win32_stream;
  BOOL res;

  win32_stream = G_WIN32_INPUT_STREAM (stream);

  if (!win32_stream->priv->close_handle)
    return TRUE;

  if (win32_stream->priv->fd != -1)
    {
      if (close (win32_stream->priv->fd) < 0)
	{
	  int errsv = errno;

	  g_set_error (error, G_IO_ERROR,
	               g_io_error_from_errno (errsv),
	               _("Error closing file descriptor: %s"),
	               xstrerror (errsv));
	  return FALSE;
	}
    }
  else
    {
      res = CloseHandle (win32_stream->priv->handle);
      if (!res)
	{
	  int errsv = GetLastError ();
	  xchar_t *emsg = g_win32_error_message (errsv);

	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_win32_error (errsv),
		       _("Error closing handle: %s"),
		       emsg);
	  g_free (emsg);
	  return FALSE;
	}
    }

  return TRUE;
}

static void
g_win32_input_stream_class_init (GWin32InputStreamClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  xinput_stream_class_t *stream_class = G_INPUT_STREAM_CLASS (klass);

  gobject_class->get_property = g_win32_input_stream_get_property;
  gobject_class->set_property = g_win32_input_stream_set_property;

  stream_class->read_fn = g_win32_input_stream_read;
  stream_class->close_fn = g_win32_input_stream_close;

  /**
   * GWin32InputStream:handle:
   *
   * The handle that the stream reads from.
   *
   * Since: 2.26
   */
  props[PROP_HANDLE] =
    g_param_spec_pointer ("handle",
                          P_("File handle"),
                          P_("The file handle to read from"),
                          G_PARAM_READABLE |
                          G_PARAM_WRITABLE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS);

  /**
   * GWin32InputStream:close-handle:
   *
   * Whether to close the file handle when the stream is closed.
   *
   * Since: 2.26
   */
  props[PROP_CLOSE_HANDLE] =
    g_param_spec_boolean ("close-handle",
                          P_("Close file handle"),
                          P_("Whether to close the file handle when the stream is closed"),
                          TRUE,
                          G_PARAM_READABLE |
                          G_PARAM_WRITABLE |
                          G_PARAM_STATIC_STRINGS);

  xobject_class_install_properties (gobject_class, LAST_PROP, props);
}

static void
g_win32_input_stream_init (GWin32InputStream *win32_stream)
{
  win32_stream->priv = g_win32_input_stream_get_instance_private (win32_stream);
  win32_stream->priv->handle = NULL;
  win32_stream->priv->close_handle = TRUE;
  win32_stream->priv->fd = -1;
}

/**
 * g_win32_input_stream_new:
 * @handle: a Win32 file handle
 * @close_handle: %TRUE to close the handle when done
 *
 * Creates a new #GWin32InputStream for the given @handle.
 *
 * If @close_handle is %TRUE, the handle will be closed
 * when the stream is closed.
 *
 * Note that "handle" here means a Win32 HANDLE, not a "file descriptor"
 * as used in the Windows C libraries.
 *
 * Returns: a new #GWin32InputStream
 **/
xinput_stream_t *
g_win32_input_stream_new (void     *handle,
			  xboolean_t close_handle)
{
  GWin32InputStream *stream;

  g_return_val_if_fail (handle != NULL, NULL);

  stream = xobject_new (XTYPE_WIN32_INPUT_STREAM,
			 "handle", handle,
			 "close-handle", close_handle,
			 NULL);

  return G_INPUT_STREAM (stream);
}

/**
 * g_win32_input_stream_set_close_handle:
 * @stream: a #GWin32InputStream
 * @close_handle: %TRUE to close the handle when done
 *
 * Sets whether the handle of @stream shall be closed
 * when the stream is closed.
 *
 * Since: 2.26
 */
void
g_win32_input_stream_set_close_handle (GWin32InputStream *stream,
				       xboolean_t          close_handle)
{
  g_return_if_fail (X_IS_WIN32_INPUT_STREAM (stream));

  close_handle = close_handle != FALSE;
  if (stream->priv->close_handle != close_handle)
    {
      stream->priv->close_handle = close_handle;
      xobject_notify (G_OBJECT (stream), "close-handle");
    }
}

/**
 * g_win32_input_stream_get_close_handle:
 * @stream: a #GWin32InputStream
 *
 * Returns whether the handle of @stream will be
 * closed when the stream is closed.
 *
 * Returns: %TRUE if the handle is closed when done
 *
 * Since: 2.26
 */
xboolean_t
g_win32_input_stream_get_close_handle (GWin32InputStream *stream)
{
  g_return_val_if_fail (X_IS_WIN32_INPUT_STREAM (stream), FALSE);

  return stream->priv->close_handle;
}

/**
 * g_win32_input_stream_get_handle:
 * @stream: a #GWin32InputStream
 *
 * Return the Windows file handle that the stream reads from.
 *
 * Returns: The file handle of @stream
 *
 * Since: 2.26
 */
void *
g_win32_input_stream_get_handle (GWin32InputStream *stream)
{
  g_return_val_if_fail (X_IS_WIN32_INPUT_STREAM (stream), NULL);

  return stream->priv->handle;
}

xinput_stream_t *
g_win32_input_stream_new_from_fd (xint_t      fd,
				  xboolean_t  close_fd)
{
  GWin32InputStream *win32_stream;

  win32_stream = G_WIN32_INPUT_STREAM (g_win32_input_stream_new ((HANDLE) _get_osfhandle (fd), close_fd));
  win32_stream->priv->fd = fd;

  return (xinput_stream_t*)win32_stream;
}
