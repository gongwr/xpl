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

#ifndef __G_UNIX_INPUT_STREAM_H__
#define __G_UNIX_INPUT_STREAM_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_UNIX_INPUT_STREAM         (g_unix_input_stream_get_type ())
#define G_UNIX_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_UNIX_INPUT_STREAM, GUnixInputStream))
#define G_UNIX_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_UNIX_INPUT_STREAM, GUnixInputStreamClass))
#define X_IS_UNIX_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_UNIX_INPUT_STREAM))
#define X_IS_UNIX_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_UNIX_INPUT_STREAM))
#define G_UNIX_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_UNIX_INPUT_STREAM, GUnixInputStreamClass))

/**
 * GUnixInputStream:
 *
 * Implements #xinput_stream_t for reading from selectable unix file descriptors
 **/
typedef struct _GUnixInputStream         GUnixInputStream;
typedef struct _GUnixInputStreamClass    GUnixInputStreamClass;
typedef struct _GUnixInputStreamPrivate  GUnixInputStreamPrivate;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixInputStream, xobject_unref)

struct _GUnixInputStream
{
  xinput_stream_t parent_instance;

  /*< private >*/
  GUnixInputStreamPrivate *priv;
};

struct _GUnixInputStreamClass
{
  xinput_stream_class_t parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t          g_unix_input_stream_get_type     (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xinput_stream_t * g_unix_input_stream_new          (xint_t              fd,
                                                 xboolean_t          close_fd);
XPL_AVAILABLE_IN_ALL
void           g_unix_input_stream_set_close_fd (GUnixInputStream *stream,
                                                 xboolean_t          close_fd);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_unix_input_stream_get_close_fd (GUnixInputStream *stream);
XPL_AVAILABLE_IN_ALL
xint_t           g_unix_input_stream_get_fd       (GUnixInputStream *stream);

G_END_DECLS

#endif /* __G_UNIX_INPUT_STREAM_H__ */
