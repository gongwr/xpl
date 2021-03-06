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

#ifndef __G_UNIX_FD_MESSAGE_H__
#define __G_UNIX_FD_MESSAGE_H__

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

G_BEGIN_DECLS

#define XTYPE_UNIX_FD_MESSAGE                              (g_unix_fd_message_get_type ())
#define G_UNIX_FD_MESSAGE(inst)                             (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_UNIX_FD_MESSAGE, GUnixFDMessage))
#define G_UNIX_FD_MESSAGE_CLASS(class)                      (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_UNIX_FD_MESSAGE, GUnixFDMessageClass))
#define X_IS_UNIX_FD_MESSAGE(inst)                          (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_UNIX_FD_MESSAGE))
#define X_IS_UNIX_FD_MESSAGE_CLASS(class)                   (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_UNIX_FD_MESSAGE))
#define G_UNIX_FD_MESSAGE_GET_CLASS(inst)                   (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_UNIX_FD_MESSAGE, GUnixFDMessageClass))

typedef struct _GUnixFDMessagePrivate                       GUnixFDMessagePrivate;
typedef struct _GUnixFDMessageClass                         GUnixFDMessageClass;
typedef struct _GUnixFDMessage                              GUnixFDMessage;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixFDMessage, xobject_unref)

struct _GUnixFDMessageClass
{
  xsocket_control_message_class_t parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
};

struct _GUnixFDMessage
{
  xsocket_control_message_t parent_instance;
  GUnixFDMessagePrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                   g_unix_fd_message_get_type                      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xsocket_control_message_t * g_unix_fd_message_new_with_fd_list              (xunix_fd_list_t     *fd_list);
XPL_AVAILABLE_IN_ALL
xsocket_control_message_t * g_unix_fd_message_new                           (void);

XPL_AVAILABLE_IN_ALL
xunix_fd_list_t *           g_unix_fd_message_get_fd_list                   (GUnixFDMessage  *message);

XPL_AVAILABLE_IN_ALL
xint_t *                  g_unix_fd_message_steal_fds                     (GUnixFDMessage  *message,
                                                                         xint_t            *length);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_unix_fd_message_append_fd                     (GUnixFDMessage  *message,
                                                                         xint_t             fd,
                                                                         xerror_t         **error);

G_END_DECLS

#endif /* __G_UNIX_FD_MESSAGE_H__ */
