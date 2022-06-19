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

#ifndef __XSOCKET_ADDRESS_ENUMERATOR_H__
#define __XSOCKET_ADDRESS_ENUMERATOR_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SOCKET_ADDRESS_ENUMERATOR         (xsocket_address_enumerator_get_type ())
#define XSOCKET_ADDRESS_ENUMERATOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SOCKET_ADDRESS_ENUMERATOR, xsocket_address_enumerator))
#define XSOCKET_ADDRESS_ENUMERATOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_SOCKET_ADDRESS_ENUMERATOR, xsocket_address_enumerator_class_t))
#define X_IS_SOCKET_ADDRESS_ENUMERATOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SOCKET_ADDRESS_ENUMERATOR))
#define X_IS_SOCKET_ADDRESS_ENUMERATOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_SOCKET_ADDRESS_ENUMERATOR))
#define XSOCKET_ADDRESS_ENUMERATOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_SOCKET_ADDRESS_ENUMERATOR, xsocket_address_enumerator_class_t))

/**
 * xsocket_address_enumerator_t:
 *
 * Enumerator type for objects that contain or generate
 * #xsocket_address_t instances.
 */
typedef struct _GSocketAddressEnumeratorClass xsocket_address_enumerator_class_t;

struct _GSocketAddressEnumerator
{
  /*< private >*/
  xobject_t parent_instance;
};

/**
 * xsocket_address_enumerator_class_t:
 * @next: Virtual method for xsocket_address_enumerator_next().
 * @next_async: Virtual method for xsocket_address_enumerator_next_async().
 * @next_finish: Virtual method for xsocket_address_enumerator_next_finish().
 *
 * Class structure for #xsocket_address_enumerator_t.
 */
struct _GSocketAddressEnumeratorClass
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/
  /* Virtual Table */

  xsocket_address_t * (* next)        (xsocket_address_enumerator_t  *enumerator,
				    xcancellable_t              *cancellable,
				    xerror_t                   **error);

  void             (* next_async)  (xsocket_address_enumerator_t  *enumerator,
				    xcancellable_t              *cancellable,
				    xasync_ready_callback_t        callback,
				    xpointer_t                   user_data);
  xsocket_address_t * (* next_finish) (xsocket_address_enumerator_t  *enumerator,
				    xasync_result_t              *result,
				    xerror_t                   **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t           xsocket_address_enumerator_get_type        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsocket_address_t *xsocket_address_enumerator_next        (xsocket_address_enumerator_t  *enumerator,
							 xcancellable_t              *cancellable,
							 xerror_t                   **error);

XPL_AVAILABLE_IN_ALL
void            xsocket_address_enumerator_next_async  (xsocket_address_enumerator_t  *enumerator,
							 xcancellable_t              *cancellable,
							 xasync_ready_callback_t        callback,
							 xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
xsocket_address_t *xsocket_address_enumerator_next_finish (xsocket_address_enumerator_t  *enumerator,
							 xasync_result_t              *result,
							 xerror_t                   **error);

G_END_DECLS


#endif /* __XSOCKET_ADDRESS_ENUMERATOR_H__ */
