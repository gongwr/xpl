/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008, 2009 Codethink Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XSOCKET_CLIENT_H__
#define __XSOCKET_CLIENT_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_CLIENT                                (xsocket_client_get_type ())
#define XSOCKET_CLIENT(inst)                               (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_CLIENT, GSocketClient))
#define XSOCKET_CLIENT_CLASS(class)                        (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_CLIENT, GSocketClientClass))
#define X_IS_SOCKET_CLIENT(inst)                            (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_CLIENT))
#define X_IS_SOCKET_CLIENT_CLASS(class)                     (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_CLIENT))
#define XSOCKET_CLIENT_GET_CLASS(inst)                     (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_CLIENT, GSocketClientClass))

typedef struct _GSocketClientPrivate                        GSocketClientPrivate;
typedef struct _GSocketClientClass                          GSocketClientClass;

struct _GSocketClientClass
{
  xobject_class_t parent_class;

  void (* event) (GSocketClient       *client,
		  GSocketClientEvent  event,
		  GSocketConnectable  *connectable,
		  xio_stream_t           *connection);

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
};

struct _GSocketClient
{
  xobject_t parent_instance;
  GSocketClientPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                   xsocket_client_get_type                        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GSocketClient          *xsocket_client_new                             (void);

XPL_AVAILABLE_IN_ALL
xsocket_family_t           xsocket_client_get_family                      (GSocketClient        *client);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_set_family                      (GSocketClient        *client,
									 xsocket_family_t         family);
XPL_AVAILABLE_IN_ALL
xsocket_type_t             xsocket_client_get_socket_type                 (GSocketClient        *client);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_set_socket_type                 (GSocketClient        *client,
									 xsocket_type_t           type);
XPL_AVAILABLE_IN_ALL
GSocketProtocol         xsocket_client_get_protocol                    (GSocketClient        *client);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_set_protocol                    (GSocketClient        *client,
									 GSocketProtocol       protocol);
XPL_AVAILABLE_IN_ALL
xsocket_address_t         *xsocket_client_get_local_address               (GSocketClient        *client);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_set_local_address               (GSocketClient        *client,
									 xsocket_address_t       *address);
XPL_AVAILABLE_IN_ALL
xuint_t                   xsocket_client_get_timeout                     (GSocketClient        *client);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_set_timeout                     (GSocketClient        *client,
									 xuint_t                 timeout);
XPL_AVAILABLE_IN_ALL
xboolean_t                xsocket_client_get_enable_proxy                (GSocketClient        *client);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_set_enable_proxy                (GSocketClient        *client,
    									 xboolean_t	      enable);

XPL_AVAILABLE_IN_2_28
xboolean_t                xsocket_client_get_tls                         (GSocketClient        *client);
XPL_AVAILABLE_IN_2_28
void                    xsocket_client_set_tls                         (GSocketClient        *client,
									 xboolean_t              tls);
XPL_DEPRECATED_IN_2_72
GTlsCertificateFlags    xsocket_client_get_tls_validation_flags        (GSocketClient        *client);
XPL_DEPRECATED_IN_2_72
void                    xsocket_client_set_tls_validation_flags        (GSocketClient        *client,
									 GTlsCertificateFlags  flags);
XPL_AVAILABLE_IN_2_36
GProxyResolver         *xsocket_client_get_proxy_resolver              (GSocketClient        *client);
XPL_AVAILABLE_IN_2_36
void                    xsocket_client_set_proxy_resolver              (GSocketClient        *client,
                                                                         GProxyResolver       *proxy_resolver);

XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect                         (GSocketClient        *client,
                                                                         GSocketConnectable   *connectable,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect_to_host                 (GSocketClient        *client,
									 const xchar_t          *host_and_port,
									 guint16               default_port,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect_to_service              (GSocketClient        *client,
									 const xchar_t          *domain,
									 const xchar_t          *service,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_26
xsocket_connection_t *     xsocket_client_connect_to_uri                  (GSocketClient        *client,
									 const xchar_t          *uri,
									 guint16               default_port,
                                                                         xcancellable_t         *cancellable,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_connect_async                   (GSocketClient        *client,
                                                                         GSocketConnectable   *connectable,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect_finish                  (GSocketClient        *client,
                                                                         xasync_result_t         *result,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_connect_to_host_async           (GSocketClient        *client,
									 const xchar_t          *host_and_port,
									 guint16               default_port,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect_to_host_finish          (GSocketClient        *client,
                                                                         xasync_result_t         *result,
                                                                         xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void                    xsocket_client_connect_to_service_async        (GSocketClient        *client,
									 const xchar_t          *domain,
									 const xchar_t          *service,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect_to_service_finish       (GSocketClient        *client,
                                                                         xasync_result_t         *result,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void                    xsocket_client_connect_to_uri_async            (GSocketClient        *client,
									 const xchar_t          *uri,
									 guint16               default_port,
                                                                         xcancellable_t         *cancellable,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xsocket_connection_t *     xsocket_client_connect_to_uri_finish           (GSocketClient        *client,
                                                                         xasync_result_t         *result,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void			xsocket_client_add_application_proxy		(GSocketClient        *client,
									 const xchar_t          *protocol);

G_END_DECLS

#endif /* __XSOCKET_CLIENT_H___ */
