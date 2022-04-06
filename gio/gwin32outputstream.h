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

#ifndef __G_WIN32_OUTPUT_STREAM_H__
#define __G_WIN32_OUTPUT_STREAM_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_WIN32_OUTPUT_STREAM         (g_win32_output_stream_get_type ())
#define G_WIN32_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_WIN32_OUTPUT_STREAM, GWin32OutputStream))
#define G_WIN32_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_WIN32_OUTPUT_STREAM, GWin32OutputStreamClass))
#define X_IS_WIN32_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_WIN32_OUTPUT_STREAM))
#define X_IS_WIN32_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_WIN32_OUTPUT_STREAM))
#define G_WIN32_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_WIN32_OUTPUT_STREAM, GWin32OutputStreamClass))

/**
 * GWin32OutputStream:
 *
 * Implements #xoutput_stream_t for outputting to Windows file handles
 **/
typedef struct _GWin32OutputStream         GWin32OutputStream;
typedef struct _GWin32OutputStreamClass    GWin32OutputStreamClass;
typedef struct _GWin32OutputStreamPrivate  GWin32OutputStreamPrivate;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GWin32OutputStream, xobject_unref)

struct _GWin32OutputStream
{
  xoutput_stream_t parent_instance;

  /*< private >*/
  GWin32OutputStreamPrivate *priv;
};

struct _GWin32OutputStreamClass
{
  GOutputStreamClass parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t           g_win32_output_stream_get_type         (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xoutput_stream_t * g_win32_output_stream_new              (void               *handle,
							xboolean_t            close_handle);
XPL_AVAILABLE_IN_ALL
void            g_win32_output_stream_set_close_handle (GWin32OutputStream *stream,
							xboolean_t           close_handle);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_win32_output_stream_get_close_handle (GWin32OutputStream *stream);
XPL_AVAILABLE_IN_ALL
void           *g_win32_output_stream_get_handle       (GWin32OutputStream *stream);
G_END_DECLS

#endif /* __G_WIN32_OUTPUT_STREAM_H__ */
