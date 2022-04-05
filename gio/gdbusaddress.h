/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_DBUS_ADDRESS_H__
#define __G_DBUS_ADDRESS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_2_36
xchar_t *g_dbus_address_escape_value (const xchar_t *string);

XPL_AVAILABLE_IN_ALL
xboolean_t g_dbus_is_address (const xchar_t *string);
XPL_AVAILABLE_IN_ALL
xboolean_t g_dbus_is_supported_address (const xchar_t  *string,
                                      xerror_t      **error);

XPL_AVAILABLE_IN_ALL
void                 g_dbus_address_get_stream        (const xchar_t          *address,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);

XPL_AVAILABLE_IN_ALL
xio_stream_t           *g_dbus_address_get_stream_finish (xasync_result_t         *res,
                                                       xchar_t               **out_guid,
                                                       xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xio_stream_t           *g_dbus_address_get_stream_sync   (const xchar_t          *address,
                                                       xchar_t               **out_guid,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xchar_t               *g_dbus_address_get_for_bus_sync  (GBusType              bus_type,
                                                       xcancellable_t  *cancellable,
                                                       xerror_t              **error);

G_END_DECLS

#endif /* __G_DBUS_ADDRESS_H__ */
