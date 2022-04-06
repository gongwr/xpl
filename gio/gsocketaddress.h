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

#ifndef __XSOCKET_ADDRESS_H__
#define __XSOCKET_ADDRESS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_ADDRESS         (xsocket_address_get_type ())
#define XSOCKET_ADDRESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SOCKET_ADDRESS, xsocket_address))
#define XSOCKET_ADDRESS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_SOCKET_ADDRESS, GSocketAddressClass))
#define X_IS_SOCKET_ADDRESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SOCKET_ADDRESS))
#define X_IS_SOCKET_ADDRESS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_SOCKET_ADDRESS))
#define XSOCKET_ADDRESS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_SOCKET_ADDRESS, GSocketAddressClass))

typedef struct _GSocketAddressClass   GSocketAddressClass;

struct _GSocketAddress
{
  xobject_t parent_instance;
};

struct _GSocketAddressClass
{
  xobject_class_t parent_class;

  xsocket_family_t  (*get_family)      (xsocket_address_t *address);

  xssize_t         (*get_native_size) (xsocket_address_t *address);

  xboolean_t       (*to_native)       (xsocket_address_t *address,
                                     xpointer_t        dest,
                                     xsize_t           destlen,
				     xerror_t        **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t                 xsocket_address_get_type        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_family_t         xsocket_address_get_family      (xsocket_address_t *address);

XPL_AVAILABLE_IN_ALL
xsocket_address_t *      xsocket_address_new_from_native (xpointer_t        native,
                                                        xsize_t           len);

XPL_AVAILABLE_IN_ALL
xboolean_t              xsocket_address_to_native       (xsocket_address_t *address,
                                                        xpointer_t        dest,
                                                        xsize_t           destlen,
							xerror_t        **error);

XPL_AVAILABLE_IN_ALL
xssize_t                xsocket_address_get_native_size (xsocket_address_t *address);

G_END_DECLS

#endif /* __XSOCKET_ADDRESS_H__ */
