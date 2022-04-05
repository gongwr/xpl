/*
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
 */

#ifndef __XSOCKET_H__
#define __XSOCKET_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET                                       (xsocket_get_type ())
#define G_SOCKET(inst)                                      (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET, xsocket_t))
#define XSOCKET_CLASS(class)                               (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET, GSocketClass))
#define X_IS_SOCKET(inst)                                   (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET))
#define X_IS_SOCKET_CLASS(class)                            (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET))
#define XSOCKET_GET_CLASS(inst)                            (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET, GSocketClass))

typedef struct _GSocketPrivate                              GSocketPrivate;
typedef struct _GSocketClass                                GSocketClass;

struct _GSocketClass
{
  xobject_class_t parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
  void (*_g_reserved7) (void);
  void (*_g_reserved8) (void);
  void (*_g_reserved9) (void);
  void (*_g_reserved10) (void);
};

struct _GSocket
{
  xobject_t parent_instance;
  GSocketPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                  xsocket_get_type                (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xsocket_t *              xsocket_new                     (xsocket_family_t            family,
							 xsocket_type_t              type,
							 GSocketProtocol          protocol,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xsocket_t *              xsocket_new_from_fd             (xint_t                     fd,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
int                    xsocket_get_fd                  (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
xsocket_family_t          xsocket_get_family              (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
xsocket_type_t            xsocket_get_socket_type         (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
GSocketProtocol        xsocket_get_protocol            (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
xsocket_address_t *       xsocket_get_local_address       (xsocket_t                 *socket,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xsocket_address_t *       xsocket_get_remote_address      (xsocket_t                 *socket,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
void                   xsocket_set_blocking            (xsocket_t                 *socket,
							 xboolean_t                 blocking);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_get_blocking            (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
void                   xsocket_set_keepalive           (xsocket_t                 *socket,
							 xboolean_t                 keepalive);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_get_keepalive           (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
xint_t                   xsocket_get_listen_backlog      (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
void                   xsocket_set_listen_backlog      (xsocket_t                 *socket,
							 xint_t                     backlog);
XPL_AVAILABLE_IN_ALL
xuint_t                  xsocket_get_timeout             (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
void                   xsocket_set_timeout             (xsocket_t                 *socket,
							 xuint_t                    timeout);

XPL_AVAILABLE_IN_2_32
xuint_t                  xsocket_get_ttl                 (xsocket_t                 *socket);
XPL_AVAILABLE_IN_2_32
void                   xsocket_set_ttl                 (xsocket_t                 *socket,
                                                         xuint_t                    ttl);

XPL_AVAILABLE_IN_2_32
xboolean_t               xsocket_get_broadcast           (xsocket_t                 *socket);
XPL_AVAILABLE_IN_2_32
void                   xsocket_set_broadcast           (xsocket_t                 *socket,
                                                         xboolean_t		  broadcast);

XPL_AVAILABLE_IN_2_32
xboolean_t               xsocket_get_multicast_loopback  (xsocket_t                 *socket);
XPL_AVAILABLE_IN_2_32
void                   xsocket_set_multicast_loopback  (xsocket_t                 *socket,
                                                         xboolean_t		  loopback);
XPL_AVAILABLE_IN_2_32
xuint_t                  xsocket_get_multicast_ttl       (xsocket_t                 *socket);
XPL_AVAILABLE_IN_2_32
void                   xsocket_set_multicast_ttl       (xsocket_t                 *socket,
                                                         xuint_t                    ttl);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_is_connected            (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_bind                    (xsocket_t                 *socket,
							 xsocket_address_t          *address,
							 xboolean_t                 allow_reuse,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_2_32
xboolean_t               xsocket_join_multicast_group    (xsocket_t                 *socket,
                                                         xinet_address_t            *group,
                                                         xboolean_t                 source_specific,
                                                         const xchar_t             *iface,
                                                         xerror_t                 **error);
XPL_AVAILABLE_IN_2_32
xboolean_t               xsocket_leave_multicast_group   (xsocket_t                 *socket,
                                                         xinet_address_t            *group,
                                                         xboolean_t                 source_specific,
                                                         const xchar_t             *iface,
                                                         xerror_t                 **error);
XPL_AVAILABLE_IN_2_56
xboolean_t               xsocket_join_multicast_group_ssm    (xsocket_t                 *socket,
                                                             xinet_address_t            *group,
                                                             xinet_address_t            *source_specific,
                                                             const xchar_t             *iface,
                                                             xerror_t                 **error);
XPL_AVAILABLE_IN_2_56
xboolean_t               xsocket_leave_multicast_group_ssm   (xsocket_t                 *socket,
                                                             xinet_address_t            *group,
                                                             xinet_address_t            *source_specific,
                                                             const xchar_t             *iface,
                                                             xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_connect                 (xsocket_t                 *socket,
							 xsocket_address_t          *address,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_check_connect_result    (xsocket_t                 *socket,
							 xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
gssize                 xsocket_get_available_bytes     (xsocket_t                 *socket);

XPL_AVAILABLE_IN_ALL
GIOCondition           xsocket_condition_check         (xsocket_t                 *socket,
							 GIOCondition             condition);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_condition_wait          (xsocket_t                 *socket,
							 GIOCondition             condition,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_2_32
xboolean_t               xsocket_condition_timed_wait    (xsocket_t                 *socket,
							 GIOCondition             condition,
							 gint64                   timeout_us,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xsocket_t *              xsocket_accept                  (xsocket_t                 *socket,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_listen                  (xsocket_t                 *socket,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_receive                 (xsocket_t                 *socket,
							 xchar_t                   *buffer,
							 xsize_t                    size,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_receive_from            (xsocket_t                 *socket,
							 xsocket_address_t         **address,
							 xchar_t                   *buffer,
							 xsize_t                    size,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_send                    (xsocket_t                 *socket,
							 const xchar_t             *buffer,
							 xsize_t                    size,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_send_to                 (xsocket_t                 *socket,
							 xsocket_address_t          *address,
							 const xchar_t             *buffer,
							 xsize_t                    size,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_receive_message         (xsocket_t                 *socket,
							 xsocket_address_t         **address,
							 GInputVector            *vectors,
							 xint_t                     num_vectors,
							 GSocketControlMessage ***messages,
							 xint_t                    *num_messages,
							 xint_t                    *flags,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_send_message            (xsocket_t                 *socket,
							 xsocket_address_t          *address,
							 GOutputVector           *vectors,
							 xint_t                     num_vectors,
							 GSocketControlMessage  **messages,
							 xint_t                     num_messages,
							 xint_t                     flags,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);

XPL_AVAILABLE_IN_2_48
xint_t                   xsocket_receive_messages        (xsocket_t                 *socket,
                                                         GInputMessage           *messages,
                                                         xuint_t                    num_messages,
                                                         xint_t                     flags,
                                                         xcancellable_t            *cancellable,
                                                         xerror_t                 **error);
XPL_AVAILABLE_IN_2_44
xint_t                   xsocket_send_messages           (xsocket_t                 *socket,
							 GOutputMessage          *messages,
							 xuint_t                    num_messages,
							 xint_t                     flags,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_close                   (xsocket_t                 *socket,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_shutdown                (xsocket_t                 *socket,
							 xboolean_t                 shutdown_read,
							 xboolean_t                 shutdown_write,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_is_closed               (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
GSource *              xsocket_create_source           (xsocket_t                 *socket,
							 GIOCondition             condition,
							 xcancellable_t            *cancellable);
XPL_AVAILABLE_IN_ALL
xboolean_t               xsocket_speaks_ipv4             (xsocket_t                 *socket);
XPL_AVAILABLE_IN_ALL
GCredentials          *xsocket_get_credentials         (xsocket_t                 *socket,
                                                         xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
gssize                 xsocket_receive_with_blocking   (xsocket_t                 *socket,
							 xchar_t                   *buffer,
							 xsize_t                    size,
							 xboolean_t                 blocking,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_ALL
gssize                 xsocket_send_with_blocking      (xsocket_t                 *socket,
							 const xchar_t             *buffer,
							 xsize_t                    size,
							 xboolean_t                 blocking,
							 xcancellable_t            *cancellable,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_2_60
GPollableReturn        xsocket_send_message_with_timeout (xsocket_t                *socket,
							   xsocket_address_t         *address,
							   const GOutputVector    *vectors,
							   xint_t                    num_vectors,
							   GSocketControlMessage **messages,
							   xint_t                    num_messages,
							   xint_t                    flags,
							   gint64                  timeout_us,
							   xsize_t                  *bytes_written,
							   xcancellable_t           *cancellable,
							   xerror_t                **error);
XPL_AVAILABLE_IN_2_36
xboolean_t               xsocket_get_option              (xsocket_t                 *socket,
							 xint_t                     level,
							 xint_t                     optname,
							 xint_t                    *value,
							 xerror_t                 **error);
XPL_AVAILABLE_IN_2_36
xboolean_t               xsocket_set_option              (xsocket_t                 *socket,
							 xint_t                     level,
							 xint_t                     optname,
							 xint_t                     value,
							 xerror_t                 **error);

G_END_DECLS

#endif /* __XSOCKET_H__ */
