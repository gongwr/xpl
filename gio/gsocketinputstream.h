/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 Christian Kellner, Samuel Cormier-Iijima
 * Copyright © 2009 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *          Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __XSOCKET_INPUT_STREAM_H__
#define __XSOCKET_INPUT_STREAM_H__

#include <gio/ginputstream.h>
#include <gio/gsocket.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_INPUT_STREAM                          (_xsocket_input_stream_get_type ())
#define XSOCKET_INPUT_STREAM(inst)                         (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_INPUT_STREAM, GSocketInputStream))
#define XSOCKET_INPUT_STREAM_CLASS(class)                  (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_INPUT_STREAM, GSocketInputStreamClass))
#define X_IS_SOCKET_INPUT_STREAM(inst)                      (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_INPUT_STREAM))
#define X_IS_SOCKET_INPUT_STREAM_CLASS(class)               (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_INPUT_STREAM))
#define XSOCKET_INPUT_STREAM_GET_CLASS(inst)               (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_INPUT_STREAM, GSocketInputStreamClass))

typedef struct _GSocketInputStreamPrivate                   GSocketInputStreamPrivate;
typedef struct _GSocketInputStreamClass                     GSocketInputStreamClass;
typedef struct _GSocketInputStream                          GSocketInputStream;

struct _GSocketInputStreamClass
{
  GInputStreamClass parent_class;
};

struct _GSocketInputStream
{
  xinput_stream_t parent_instance;
  GSocketInputStreamPrivate *priv;
};

xtype_t                   _xsocket_input_stream_get_type                  (void) G_GNUC_CONST;
GSocketInputStream *    _xsocket_input_stream_new                      (xsocket_t *socket);

G_END_DECLS

#endif /* __XSOCKET_INPUT_STREAM_H___ */
