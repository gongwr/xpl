/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2009 Codethink Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __XSOCKET_CONTROL_MESSAGE_H__
#define __XSOCKET_CONTROL_MESSAGE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_CONTROL_MESSAGE                       (xsocket_control_message_get_type ())
#define XSOCKET_CONTROL_MESSAGE(inst)                      (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_CONTROL_MESSAGE,                          \
                                                             xsocket_control_message))
#define XSOCKET_CONTROL_MESSAGE_CLASS(class)               (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_CONTROL_MESSAGE,                          \
                                                             xsocket_control_message_class_t))
#define X_IS_SOCKET_CONTROL_MESSAGE(inst)                   (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_CONTROL_MESSAGE))
#define X_IS_SOCKET_CONTROL_MESSAGE_CLASS(class)            (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_CONTROL_MESSAGE))
#define XSOCKET_CONTROL_MESSAGE_GET_CLASS(inst)            (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_CONTROL_MESSAGE,                          \
                                                             xsocket_control_message_class_t))

typedef struct _xsocket_control_message_private                xsocket_control_message_private_t;
typedef struct _xsocket_control_message_class                  xsocket_control_message_class_t;

/**
 * xsocket_control_message_class_t:
 * @get_size: gets the size of the message.
 * @get_level: gets the protocol of the message.
 * @get_type: gets the protocol specific type of the message.
 * @serialize: Writes out the message data.
 * @deserialize: Tries to deserialize a message.
 *
 * Class structure for #xsocket_control_message_t.
 **/

struct _xsocket_control_message_class
{
  xobject_class_t parent_class;

  xsize_t                  (* get_size)  (xsocket_control_message_t  *message);
  int                    (* get_level) (xsocket_control_message_t  *message);
  int                    (* get_type)  (xsocket_control_message_t  *message);
  void                   (* serialize) (xsocket_control_message_t  *message,
					xpointer_t                data);
  xsocket_control_message_t *(* deserialize) (int                   level,
					  int                   type,
					  xsize_t                 size,
					  xpointer_t              data);

  /*< private >*/

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

struct _GSocketControlMessage
{
  xobject_t parent_instance;
  xsocket_control_message_private_t *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                  xsocket_control_message_get_type     (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xsize_t                  xsocket_control_message_get_size     (xsocket_control_message_t *message);
XPL_AVAILABLE_IN_ALL
int                    xsocket_control_message_get_level    (xsocket_control_message_t *message);
XPL_AVAILABLE_IN_ALL
int                    xsocket_control_message_get_msg_type (xsocket_control_message_t *message);
XPL_AVAILABLE_IN_ALL
void                   xsocket_control_message_serialize    (xsocket_control_message_t *message,
							      xpointer_t               data);
XPL_AVAILABLE_IN_ALL
xsocket_control_message_t *xsocket_control_message_deserialize  (int                    level,
							      int                    type,
							      xsize_t                  size,
							      xpointer_t               data);


G_END_DECLS

#endif /* __XSOCKET_CONTROL_MESSAGE_H__ */
