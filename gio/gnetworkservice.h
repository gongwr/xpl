/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#ifndef __G_NETWORK_SERVICE_H__
#define __G_NETWORK_SERVICE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_NETWORK_SERVICE         (g_network_service_get_type ())
#define G_NETWORK_SERVICE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_NETWORK_SERVICE, xnetwork_service))
#define G_NETWORK_SERVICE_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_NETWORK_SERVICE, GNetworkServiceClass))
#define X_IS_NETWORK_SERVICE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_NETWORK_SERVICE))
#define X_IS_NETWORK_SERVICE_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_NETWORK_SERVICE))
#define G_NETWORK_SERVICE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_NETWORK_SERVICE, GNetworkServiceClass))

typedef struct _GNetworkServiceClass   GNetworkServiceClass;
typedef struct _GNetworkServicePrivate GNetworkServicePrivate;

struct _GNetworkService
{
  xobject_t parent_instance;

  /*< private >*/
  GNetworkServicePrivate *priv;
};

struct _GNetworkServiceClass
{
  xobject_class_t parent_class;

};

XPL_AVAILABLE_IN_ALL
xtype_t                g_network_service_get_type      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_connectable_t  *g_network_service_new           (const xchar_t     *service,
						      const xchar_t     *protocol,
						      const xchar_t     *domain);

XPL_AVAILABLE_IN_ALL
const xchar_t         *g_network_service_get_service   (xnetwork_service_t *srv);
XPL_AVAILABLE_IN_ALL
const xchar_t         *g_network_service_get_protocol  (xnetwork_service_t *srv);
XPL_AVAILABLE_IN_ALL
const xchar_t         *g_network_service_get_domain    (xnetwork_service_t *srv);
XPL_AVAILABLE_IN_ALL
const xchar_t         *g_network_service_get_scheme    (xnetwork_service_t *srv);
XPL_AVAILABLE_IN_ALL
void                 g_network_service_set_scheme    (xnetwork_service_t *srv, const xchar_t *scheme);

G_END_DECLS

#endif /* __G_NETWORK_SERVICE_H__ */
