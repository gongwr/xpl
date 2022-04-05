/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 Christian Kellner, Samuel Cormier-Iijima
 * Copyright © 2009 Codethink Limited
 * Copyright © 2009 Red Hat, Inc
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

#ifndef __XSOCKET_LISTENER_H__
#define __XSOCKET_LISTENER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_LISTENER                              (xsocket_listener_get_type ())
#define XSOCKET_LISTENER(inst)                             (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_LISTENER, GSocketListener))
#define XSOCKET_LISTENER_CLASS(class)                      (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_LISTENER, GSocketListenerClass))
#define X_IS_SOCKET_LISTENER(inst)                          (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_LISTENER))
#define X_IS_SOCKET_LISTENER_CLASS(class)                   (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_LISTENER))
#define XSOCKET_LISTENER_GET_CLASS(inst)                   (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_LISTENER, GSocketListenerClass))

typedef struct _GSocketListenerPrivate                      GSocketListenerPrivate;
typedef struct _GSocketListenerClass                        GSocketListenerClass;

/**
 * GSocketListenerClass:
 * @changed: virtual method called when the set of socket listened to changes
 *
 * Class structure for #GSocketListener.
 **/
struct _GSocketListenerClass
{
  xobject_class_t parent_class;

  void (* changed) (GSocketListener *listener);

  void (* event) (GSocketListener      *listener,
                  GSocketListenerEvent  event,
                  xsocket_t              *socket);

  /* Padding for future expansion */
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
};

struct _GSocketListener
{
  xobject_t parent_instance;
  GSocketListenerPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                   xsocket_listener_get_type                      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GSocketListener *       xsocket_listener_new                           (void);

XPL_AVAILABLE_IN_ALL
void                    xsocket_listener_set_backlog                   (GSocketListener     *listener,
									 int                  listen_backlog);

XPL_AVAILABLE_IN_ALL
xboolean_t                xsocket_listener_add_socket                    (GSocketListener     *listener,
                                                                         xsocket_t             *socket,
									 xobject_t             *source_object,
									 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xsocket_listener_add_address                   (GSocketListener     *listener,
                                                                         xsocket_address_t      *address,
									 xsocket_type_t          type,
									 GSocketProtocol      protocol,
									 xobject_t             *source_object,
                                                                         xsocket_address_t     **effective_address,
									 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xsocket_listener_add_inet_port                 (GSocketListener     *listener,
                                                                         guint16              port,
									 xobject_t             *source_object,
									 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
guint16                 xsocket_listener_add_any_inet_port             (GSocketListener     *listener,
									 xobject_t             *source_object,
									 xerror_t             **error);

XPL_AVAILABLE_IN_ALL
xsocket_t *               xsocket_listener_accept_socket                 (GSocketListener      *listener,
									 xobject_t             **source_object,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void                    xsocket_listener_accept_socket_async           (GSocketListener      *listener,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xsocket_t *               xsocket_listener_accept_socket_finish          (GSocketListener      *listener,
                                                                         xasync_result_t         *result,
									 xobject_t             **source_object,
                                                                         xerror_t              **error);


XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_listener_accept                        (GSocketListener      *listener,
									 xobject_t             **source_object,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void                    xsocket_listener_accept_async                  (GSocketListener      *listener,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_listener_accept_finish                 (GSocketListener      *listener,
                                                                         xasync_result_t         *result,
									 xobject_t             **source_object,
                                                                         xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void                    xsocket_listener_close                         (GSocketListener      *listener);

G_END_DECLS

#endif /* __XSOCKET_LISTENER_H__ */
