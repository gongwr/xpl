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

#ifndef __G_DBUS_NAME_OWNING_H__
#define __G_DBUS_NAME_OWNING_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * GBusAcquiredCallback:
 * @connection: The #xdbus_connection_t to a message bus.
 * @name: The name that is requested to be owned.
 * @user_data: User data passed to g_bus_own_name().
 *
 * Invoked when a connection to a message bus has been obtained.
 *
 * Since: 2.26
 */
typedef void (*GBusAcquiredCallback) (xdbus_connection_t *connection,
                                      const xchar_t     *name,
                                      xpointer_t         user_data);

/**
 * GBusNameAcquiredCallback:
 * @connection: The #xdbus_connection_t on which to acquired the name.
 * @name: The name being owned.
 * @user_data: User data passed to g_bus_own_name() or g_bus_own_name_on_connection().
 *
 * Invoked when the name is acquired.
 *
 * Since: 2.26
 */
typedef void (*GBusNameAcquiredCallback) (xdbus_connection_t *connection,
                                          const xchar_t     *name,
                                          xpointer_t         user_data);

/**
 * GBusNameLostCallback:
 * @connection: The #xdbus_connection_t on which to acquire the name or %NULL if
 * the connection was disconnected.
 * @name: The name being owned.
 * @user_data: User data passed to g_bus_own_name() or g_bus_own_name_on_connection().
 *
 * Invoked when the name is lost or @connection has been closed.
 *
 * Since: 2.26
 */
typedef void (*GBusNameLostCallback) (xdbus_connection_t *connection,
                                      const xchar_t     *name,
                                      xpointer_t         user_data);

XPL_AVAILABLE_IN_ALL
xuint_t g_bus_own_name                 (xbus_type_t                  bus_type,
                                      const xchar_t              *name,
                                      GBusNameOwnerFlags        flags,
                                      GBusAcquiredCallback      bus_acquired_handler,
                                      GBusNameAcquiredCallback  name_acquired_handler,
                                      GBusNameLostCallback      name_lost_handler,
                                      xpointer_t                  user_data,
                                      xdestroy_notify_t            user_data_free_func);

XPL_AVAILABLE_IN_ALL
xuint_t g_bus_own_name_on_connection   (xdbus_connection_t          *connection,
                                      const xchar_t              *name,
                                      GBusNameOwnerFlags        flags,
                                      GBusNameAcquiredCallback  name_acquired_handler,
                                      GBusNameLostCallback      name_lost_handler,
                                      xpointer_t                  user_data,
                                      xdestroy_notify_t            user_data_free_func);

XPL_AVAILABLE_IN_ALL
xuint_t g_bus_own_name_with_closures   (xbus_type_t                  bus_type,
                                      const xchar_t              *name,
                                      GBusNameOwnerFlags        flags,
                                      xclosure_t                 *bus_acquired_closure,
                                      xclosure_t                 *name_acquired_closure,
                                      xclosure_t                 *name_lost_closure);

XPL_AVAILABLE_IN_ALL
xuint_t g_bus_own_name_on_connection_with_closures (
                                      xdbus_connection_t          *connection,
                                      const xchar_t              *name,
                                      GBusNameOwnerFlags        flags,
                                      xclosure_t                 *name_acquired_closure,
                                      xclosure_t                 *name_lost_closure);

XPL_AVAILABLE_IN_ALL
void  g_bus_unown_name               (xuint_t                     owner_id);

G_END_DECLS

#endif /* __G_DBUS_NAME_OWNING_H__ */
