/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
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

#ifndef __XINET_ADDRESS_MASK_H__
#define __XINET_ADDRESS_MASK_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_INET_ADDRESS_MASK         (xinet_address_mask_get_type ())
#define XINET_ADDRESS_MASK(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_INET_ADDRESS_MASK, xinet_address_mask))
#define XINET_ADDRESS_MASK_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_INET_ADDRESS_MASK, xinet_address_mask_class))
#define X_IS_INET_ADDRESS_MASK(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_INET_ADDRESS_MASK))
#define X_IS_INET_ADDRESS_MASK_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_INET_ADDRESS_MASK))
#define XINET_ADDRESS_MASK_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_INET_ADDRESS_MASK, xinet_address_mask_class))

typedef struct _xinet_address_mask_class   xinet_address_mask_class_t;
typedef struct _xinet_address_mask_private xinet_address_mask_private_t;

struct _GInetAddressMask
{
  xobject_t parent_instance;

  /*< private >*/
  xinet_address_mask_private_t *priv;
};

struct _xinet_address_mask_class
{
  xobject_class_t parent_class;

};

XPL_AVAILABLE_IN_2_32
xtype_t xinet_address_mask_get_type (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xinet_address_mask_t *xinet_address_mask_new             (xinet_address_t      *addr,
						       xuint_t              length,
						       xerror_t           **error);

XPL_AVAILABLE_IN_2_32
xinet_address_mask_t *xinet_address_mask_new_from_string (const xchar_t       *mask_string,
						       xerror_t           **error);
XPL_AVAILABLE_IN_2_32
xchar_t            *xinet_address_mask_to_string       (xinet_address_mask_t  *mask);

XPL_AVAILABLE_IN_2_32
xsocket_family_t     xinet_address_mask_get_family      (xinet_address_mask_t  *mask);
XPL_AVAILABLE_IN_2_32
xinet_address_t     *xinet_address_mask_get_address     (xinet_address_mask_t  *mask);
XPL_AVAILABLE_IN_2_32
xuint_t             xinet_address_mask_get_length      (xinet_address_mask_t  *mask);

XPL_AVAILABLE_IN_2_32
xboolean_t          xinet_address_mask_matches         (xinet_address_mask_t  *mask,
						       xinet_address_t      *address);
XPL_AVAILABLE_IN_2_32
xboolean_t          xinet_address_mask_equal           (xinet_address_mask_t  *mask,
						       xinet_address_mask_t  *mask2);

G_END_DECLS

#endif /* __XINET_ADDRESS_MASK_H__ */
