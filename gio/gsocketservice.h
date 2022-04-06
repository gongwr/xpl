/* GIO - GLib Input, Output and Streaming Library
 *
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XSOCKET_SERVICE_H__
#define __XSOCKET_SERVICE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gsocketlistener.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_SERVICE                               (xsocket_service_get_type ())
#define XSOCKET_SERVICE(inst)                              (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SOCKET_SERVICE, xsocket_service))
#define XSOCKET_SERVICE_CLASS(class)                       (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SOCKET_SERVICE, GSocketServiceClass))
#define X_IS_SOCKET_SERVICE(inst)                           (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SOCKET_SERVICE))
#define X_IS_SOCKET_SERVICE_CLASS(class)                    (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SOCKET_SERVICE))
#define XSOCKET_SERVICE_GET_CLASS(inst)                    (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SOCKET_SERVICE, GSocketServiceClass))

typedef struct _GSocketServicePrivate                       GSocketServicePrivate;
typedef struct _GSocketServiceClass                         GSocketServiceClass;

/**
 * GSocketServiceClass:
 * @incoming: signal emitted when new connections are accepted
 *
 * Class structure for #xsocket_service_t.
 */
struct _GSocketServiceClass
{
  GSocketListenerClass parent_class;

  xboolean_t (* incoming) (xsocket_service_t    *service,
                         xsocket_connection_t *connection,
			 xobject_t           *source_object);

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
};

struct _GSocketService
{
  xsocket_listener_t parent_instance;
  GSocketServicePrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t           xsocket_service_get_type  (void);

XPL_AVAILABLE_IN_ALL
xsocket_service_t *xsocket_service_new       (void);
XPL_AVAILABLE_IN_ALL
void            xsocket_service_start     (xsocket_service_t *service);
XPL_AVAILABLE_IN_ALL
void            xsocket_service_stop      (xsocket_service_t *service);
XPL_AVAILABLE_IN_ALL
xboolean_t        xsocket_service_is_active (xsocket_service_t *service);


G_END_DECLS

#endif /* __XSOCKET_SERVICE_H__ */
