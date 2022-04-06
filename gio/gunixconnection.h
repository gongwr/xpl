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

#ifndef __G_UNIX_CONNECTION_H__
#define __G_UNIX_CONNECTION_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_UNIX_CONNECTION                              (g_unix_connection_get_type ())
#define G_UNIX_CONNECTION(inst)                             (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_UNIX_CONNECTION, GUnixConnection))
#define G_UNIX_CONNECTION_CLASS(class)                      (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_UNIX_CONNECTION, GUnixConnectionClass))
#define X_IS_UNIX_CONNECTION(inst)                          (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_UNIX_CONNECTION))
#define X_IS_UNIX_CONNECTION_CLASS(class)                   (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_UNIX_CONNECTION))
#define G_UNIX_CONNECTION_GET_CLASS(inst)                   (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_UNIX_CONNECTION, GUnixConnectionClass))

typedef struct _GUnixConnection                             GUnixConnection;
typedef struct _GUnixConnectionPrivate                      GUnixConnectionPrivate;
typedef struct _GUnixConnectionClass                        GUnixConnectionClass;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GUnixConnection, xobject_unref)

struct _GUnixConnectionClass
{
  xsocket_connection_class_t parent_class;
};

struct _GUnixConnection
{
  xsocket_connection_t parent_instance;
  GUnixConnectionPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                   g_unix_connection_get_type                      (void);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_unix_connection_send_fd                       (GUnixConnection      *connection,
                                                                         xint_t                  fd,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xint_t                    g_unix_connection_receive_fd                    (GUnixConnection      *connection,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_unix_connection_send_credentials              (GUnixConnection      *connection,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_32
void                    g_unix_connection_send_credentials_async        (GUnixConnection      *connection,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_2_32
xboolean_t                g_unix_connection_send_credentials_finish       (GUnixConnection      *connection,
                                                                         xasync_result_t         *result,
                                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_32
xcredentials_t           *g_unix_connection_receive_credentials           (GUnixConnection      *connection,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_32
void                    g_unix_connection_receive_credentials_async     (GUnixConnection      *connection,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xcredentials_t           *g_unix_connection_receive_credentials_finish    (GUnixConnection      *connection,
                                                                         xasync_result_t         *result,
                                                                         xerror_t              **error);

G_END_DECLS

#endif /* __G_UNIX_CONNECTION_H__ */
