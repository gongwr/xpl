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
#include <glib/gstdio.h>
#include "gioerror.h"
#include "gwin32outputstream.h"
#include "giowin32-priv.h"
#include "gcancellable.h"
#include "gasynchelper.h"
#include "glibintl.h"

/**
 * SECTION:gwin32outputstream
 * @short_description: Streaming output operations for Windows file handles
 * @include: gio/gwin32outputstream.h
 * @see_also: #xoutput_stream_t
 *
 * #GWin32OutputStream implements #xoutput_stream_t for writing to a
 * Windows file handle.
 *
 * Note that `<gio/gwin32outputstream.h>` belongs to the Windows-specific GIO
 * interfaces, thus you have to use the `gio-windows-2.0.pc` pkg-config file
 * when using it.
 */

struct _GWin32OutputStreamPrivate {
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

G_DEFINE_TYPE_WITH_PRIVATE (GWin32OutputStream, g_win32_output_stream, XTYPE_OUTPUT_STREAM)

static void
g_win32_output_stream_set_property (xobject_t         *object,
				    xuint_t            prop_id,
				    const xvalue_t    *value,
				    xparam_spec_t      *pspec)
{
  GWin32OutputStream *win32_stream;

  win32_stream = G_WIN32_OUTPUT_STREAM (object);

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
g_win32_output_stream_get_property (xobject_t    *object,
				    xuint_t       prop_id,
				    xvalue_t     *value,
				    xparam_spec_t *pspec)
{
  GWin32OutputStream *win32_stream;

  win32_stream = G_WIN32_OUTPUT_STREAM (object);

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
g_win32_output_stream_write (xoutput_stream_t  *stream,
			    const void     *buffer,
			    xsize_t           count,
			    xcancellable_t   *cancellable,
			    xerror_t        **error)
{
  GWin32OutputStream *win32_stream;
  BOOL res;
  DWORD nbytes, nwritten;
  OVERLAPPED overlap = { 0, };
  xssize_t retval = -1;

  win32_stream = G_WIN32_OUTPUT_STREAM (stream);

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return -1;

  if (count > G_MAXINT)
    nbytes = G_MAXINT;
  else
    nbytes = count;

  overlap.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
  xreturn_val_if_fail (overlap.hEvent != NULL, -1);

  res = WriteFile (win32_stream->priv->handle, buffer, nbytes, &nwritten, &overlap);
  if (res)
    retval = nwritten;
  else
    {
      int errsv = GetLastError ();

      if (errsv == ERROR_IO_PENDING &&
          _g_win32_overlap_wait_result (win32_stream->priv->handle,
                                        &overlap, &nwritten, cancellable))
        {
          retval = nwritten;
          goto end;
        }

      if (xcancellable_set_error_if_cancelled (cancellable, error))
        goto end;

      errsv = GetLastError ();
      if (errsv == ERROR_HANDLE_EOF ||
          errsv == ERROR_BROKEN_PIPE)
        {
          retval = 0;
        }
      else
        {
          xchar_t *emsg;

          emsg = g_win32_error_message (errsv);
          g_set_error (error, G_IO_ERROR,
                       g_io_error_from_win32_error (errsv),
                       _("Error writing to handle: %s"),
                       emsg);
          g_free (emsg);
        }
    }

end:
  CloseHandle (overlap.hEvent);
  return retval;
}

static xboolean_t
g_win32_output_stream_close (xoutput_stream_t  *stream,
			     xcancellable_t   *cancellable,
			     xerror_t        **error)
{
  GWin32OutputStream *win32_stream;
  BOOL res;

  win32_stream = G_WIN32_OUTPUT_STREAM (stream);

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
g_win32_output_stream_class_init (GWin32OutputStreamClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
  xoutput_stream_class_t *stream_class = G_OUTPUT_STREAM_CLASS (klass);

  xobject_class->get_property = g_win32_output_stream_get_property;
  xobject_class->set_property = g_win32_output_stream_set_property;

  stream_class->write_fn = g_win32_output_stream_write;
  stream_class->close_fn = g_win32_output_stream_close;

   /**
   * GWin32OutputStream:handle:
   *
   * The file handle that the stream writes to.
   *
   * Since: 2.26
   */
  props[PROP_HANDLE] =
    xparam_spec_pointer ("handle",
                          P_("File handle"),
                          P_("The file handle to write to"),
                          XPARAM_READABLE |
                          XPARAM_WRITABLE |
                          XPARAM_CONSTRUCT_ONLY |
                          XPARAM_STATIC_STRINGS);

  /**
   * GWin32OutputStream:close-handle:
   *
   * Whether to close the file handle when the stream is closed.
   *
   * Since: 2.26
   */
  props[PROP_CLOSE_HANDLE] =
    xparam_spec_boolean ("close-handle",
                          P_("Close file handle"),
                          P_("Whether to close the file handle when the stream is closed"),
                          TRUE,
                          XPARAM_READABLE |
                          XPARAM_WRITABLE |
                          XPARAM_STATIC_STRINGS);

  xobject_class_install_properties (xobject_class, LAST_PROP, props);
}

static void
g_win32_output_stream_init (GWin32OutputStream *win32_stream)
{
  win32_stream->priv = g_win32_output_stream_get_instance_private (win32_stream);
  win32_stream->priv->handle = NULL;
  win32_stream->priv->close_handle = TRUE;
  win32_stream->priv->fd = -1;
}

/**
 * g_win32_output_stream_new:
 * @handle: a Win32 file handle
 * @close_handle: %TRUE to close the handle when done
 *
 * Creates a new #GWin32OutputStream for the given @handle.
 *
 * If @close_handle, is %TRUE, the handle will be closed when the
 * output stream is destroyed.
 *
 * Returns: a new #xoutput_stream_t
 *
 * Since: 2.26
**/
xoutput_stream_t *
g_win32_output_stream_new (void    *handle,
			   xboolean_t close_handle)
{
  GWin32OutputStream *stream;

  xreturn_val_if_fail (handle != NULL, NULL);

  stream = xobject_new (XTYPE_WIN32_OUTPUT_STREAM,
			 "handle", handle,
			 "close-handle", close_handle,
			 NULL);

  return G_OUTPUT_STREAM (stream);
}

/**
 * g_win32_output_stream_set_close_handle:
 * @stream: a #GWin32OutputStream
 * @close_handle: %TRUE to close the handle when done
 *
 * Sets whether the handle of @stream shall be closed when the stream
 * is closed.
 *
 * Since: 2.26
 */
void
g_win32_output_stream_set_close_handle (GWin32OutputStream *stream,
					xboolean_t           close_handle)
{
  g_return_if_fail (X_IS_WIN32_OUTPUT_STREAM (stream));

  close_handle = close_handle != FALSE;
  if (stream->priv->close_handle != close_handle)
    {
      stream->priv->close_handle = close_handle;
      xobject_notify (G_OBJECT (stream), "close-handle");
    }
}

/**
 * g_win32_output_stream_get_close_handle:
 * @stream: a #GWin32OutputStream
 *
 * Returns whether the handle of @stream will be closed when the
 * stream is closed.
 *
 * Returns: %TRUE if the handle is closed when done
 *
 * Since: 2.26
 */
xboolean_t
g_win32_output_stream_get_close_handle (GWin32OutputStream *stream)
{
  xreturn_val_if_fail (X_IS_WIN32_OUTPUT_STREAM (stream), FALSE);

  return stream->priv->close_handle;
}

/**
 * g_win32_output_stream_get_handle:
 * @stream: a #GWin32OutputStream
 *
 * Return the Windows handle that the stream writes to.
 *
 * Returns: The handle descriptor of @stream
 *
 * Since: 2.26
 */
void *
g_win32_output_stream_get_handle (GWin32OutputStream *stream)
{
  xreturn_val_if_fail (X_IS_WIN32_OUTPUT_STREAM (stream), NULL);

  return stream->priv->handle;
}

xoutput_stream_t *
g_win32_output_stream_new_from_fd (xint_t      fd,
                                   xboolean_t  close_fd)
{
  GWin32OutputStream *win32_stream;

  win32_stream = G_WIN32_OUTPUT_STREAM (g_win32_output_stream_new ((HANDLE) _get_osfhandle (fd), close_fd));
  win32_stream->priv->fd = fd;

  return (xoutput_stream_t*)win32_stream;
}
