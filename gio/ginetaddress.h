/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
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
 */

#ifndef __XINET_ADDRESS_H__
#define __XINET_ADDRESS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_INET_ADDRESS         (xinet_address_get_type ())
#define G_INET_ADDRESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_INET_ADDRESS, xinet_address))
#define XINET_ADDRESS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_INET_ADDRESS, GInetAddressClass))
#define X_IS_INET_ADDRESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_INET_ADDRESS))
#define X_IS_INET_ADDRESS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_INET_ADDRESS))
#define XINET_ADDRESS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_INET_ADDRESS, GInetAddressClass))

typedef struct _GInetAddressClass   GInetAddressClass;
typedef struct _GInetAddressPrivate GInetAddressPrivate;

struct _GInetAddress
{
  xobject_t parent_instance;

  /*< private >*/
  GInetAddressPrivate *priv;
};

struct _GInetAddressClass
{
  xobject_class_t parent_class;

  xchar_t *        (*to_string) (xinet_address_t *address);
  const xuint8_t * (*to_bytes)  (xinet_address_t *address);
};

XPL_AVAILABLE_IN_ALL
xtype_t                 xinet_address_get_type             (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xinet_address_t *        xinet_address_new_from_string      (const xchar_t          *string);

XPL_AVAILABLE_IN_ALL
xinet_address_t *        xinet_address_new_from_bytes       (const xuint8_t         *bytes,
							   xsocket_family_t         family);

XPL_AVAILABLE_IN_ALL
xinet_address_t *        xinet_address_new_loopback         (xsocket_family_t         family);

XPL_AVAILABLE_IN_ALL
xinet_address_t *        xinet_address_new_any              (xsocket_family_t         family);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_equal                (xinet_address_t         *address,
                                                           xinet_address_t         *other_address);

XPL_AVAILABLE_IN_ALL
xchar_t *               xinet_address_to_string            (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
const xuint8_t *        xinet_address_to_bytes             (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xsize_t                 xinet_address_get_native_size      (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xsocket_family_t         xinet_address_get_family           (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_any           (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_loopback      (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_link_local    (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_site_local    (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_multicast     (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_mc_global     (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_mc_link_local (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_mc_node_local (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_mc_org_local  (xinet_address_t         *address);

XPL_AVAILABLE_IN_ALL
xboolean_t              xinet_address_get_is_mc_site_local (xinet_address_t         *address);

G_END_DECLS

#endif /* __XINET_ADDRESS_H__ */
