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

#ifndef __XSOCKET_OUTPUT_STREAM_H__
#define __XSOCKET_OUTPUT_STREAM_H__

#include <gio/goutputstream.h>
#include <gio/gsocket.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_OUTPUT_STREAM                         (_xsocket_output_stream_get_type ())
#define XSOCKET_OUTPUT_STREAM(inst)                        (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_OUTPUT_STREAM, GSocketOutputStream))
#define XSOCKET_OUTPUT_STREAM_CLASS(class)                 (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_OUTPUT_STREAM, GSocketOutputStreamClass))
#define X_IS_SOCKET_OUTPUT_STREAM(inst)                     (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_OUTPUT_STREAM))
#define X_IS_SOCKET_OUTPUT_STREAM_CLASS(class)              (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_OUTPUT_STREAM))
#define XSOCKET_OUTPUT_STREAM_GET_CLASS(inst)              (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_OUTPUT_STREAM, GSocketOutputStreamClass))

typedef struct _GSocketOutputStreamPrivate                  GSocketOutputStreamPrivate;
typedef struct _GSocketOutputStreamClass                    GSocketOutputStreamClass;
typedef struct _GSocketOutputStream                         GSocketOutputStream;

struct _GSocketOutputStreamClass
{
  xoutput_stream_class_t parent_class;
};

struct _GSocketOutputStream
{
  xoutput_stream_t parent_instance;
  GSocketOutputStreamPrivate *priv;
};

xtype_t                   _xsocket_output_stream_get_type                 (void) G_GNUC_CONST;
GSocketOutputStream *   _xsocket_output_stream_new                     (xsocket_t *socket);

G_END_DECLS

#endif /* __XSOCKET_OUTPUT_STREAM_H__ */
