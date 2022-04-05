/* GIO - GLib Input, Output and Streaming Library
 * Copyright © 2008 Christian Kellner, Samuel Cormier-Iijima
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *          Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XSOCKET_CONNECTION_H__
#define __XSOCKET_CONNECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gsocket.h>
#include <gio/giostream.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_CONNECTION                            (xsocket_connection_get_type ())
#define XSOCKET_CONNECTION(inst)                           (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_CONNECTION, xsocket_connection_t))
#define XSOCKET_CONNECTION_CLASS(class)                    (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_CONNECTION, xsocket_connection_class_t))
#define X_IS_SOCKET_CONNECTION(inst)                        (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_CONNECTION))
#define X_IS_SOCKET_CONNECTION_CLASS(class)                 (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_CONNECTION))
#define XSOCKET_CONNECTION_GET_CLASS(inst)                 (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_CONNECTION, xsocket_connection_class_t))

typedef struct _xsocket_connection_private_t                    xsocket_connection_private_t;
typedef struct _xsocket_connection_class                      xsocket_connection_class_t;

struct _xsocket_connection_class
{
  xio_stream_class_t parent_class;

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
};

struct _xsocket_connection
{
  xio_stream_t parent_instance;
  xsocket_connection_private_t *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t              xsocket_connection_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xboolean_t           xsocket_connection_is_connected              (xsocket_connection_t  *connection);
XPL_AVAILABLE_IN_2_32
xboolean_t           xsocket_connection_connect                   (xsocket_connection_t  *connection,
								  xsocket_address_t     *address,
								  xcancellable_t       *cancellable,
								  xerror_t            **error);
XPL_AVAILABLE_IN_2_32
void               xsocket_connection_connect_async             (xsocket_connection_t  *connection,
								  xsocket_address_t     *address,
								  xcancellable_t       *cancellable,
								  xasync_ready_callback_t callback,
								  xpointer_t            user_data);
XPL_AVAILABLE_IN_2_32
xboolean_t           xsocket_connection_connect_finish            (xsocket_connection_t  *connection,
								  xasync_result_t       *result,
								  xerror_t            **error);

XPL_AVAILABLE_IN_ALL
xsocket_t           *xsocket_connection_get_socket                (xsocket_connection_t  *connection);
XPL_AVAILABLE_IN_ALL
xsocket_address_t    *xsocket_connection_get_local_address         (xsocket_connection_t  *connection,
								  xerror_t            **error);
XPL_AVAILABLE_IN_ALL
xsocket_address_t    *xsocket_connection_get_remote_address        (xsocket_connection_t  *connection,
								  xerror_t            **error);

XPL_AVAILABLE_IN_ALL
void               xsocket_connection_factory_register_type     (xtype_t               g_type,
								  xsocket_family_t       family,
								  xsocket_type_t         type,
								  xint_t                protocol);
XPL_AVAILABLE_IN_ALL
xtype_t              xsocket_connection_factory_lookup_type       (xsocket_family_t       family,
								  xsocket_type_t         type,
								  xint_t                protocol_id);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *xsocket_connection_factory_create_connection (xsocket_t            *socket);

G_END_DECLS

#endif /* __XSOCKET_CONNECTION_H__ */
