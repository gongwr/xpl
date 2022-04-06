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

#ifndef __G_DBUS_NAME_WATCHING_H__
#define __G_DBUS_NAME_WATCHING_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * GBusNameAppearedCallback:
 * @connection: The #xdbus_connection_t the name is being watched on.
 * @name: The name being watched.
 * @name_owner: Unique name of the owner of the name being watched.
 * @user_data: User data passed to g_bus_watch_name().
 *
 * Invoked when the name being watched is known to have to have an owner.
 *
 * Since: 2.26
 */
typedef void (*GBusNameAppearedCallback) (xdbus_connection_t *connection,
                                          const xchar_t     *name,
                                          const xchar_t     *name_owner,
                                          xpointer_t         user_data);

/**
 * GBusNameVanishedCallback:
 * @connection: The #xdbus_connection_t the name is being watched on, or
 *     %NULL.
 * @name: The name being watched.
 * @user_data: User data passed to g_bus_watch_name().
 *
 * Invoked when the name being watched is known not to have to have an owner.
 *
 * This is also invoked when the #xdbus_connection_t on which the watch was
 * established has been closed.  In that case, @connection will be
 * %NULL.
 *
 * Since: 2.26
 */
typedef void (*GBusNameVanishedCallback) (xdbus_connection_t *connection,
                                          const xchar_t     *name,
                                          xpointer_t         user_data);


XPL_AVAILABLE_IN_ALL
xuint_t g_bus_watch_name               (GBusType                  bus_type,
                                      const xchar_t              *name,
                                      GBusNameWatcherFlags      flags,
                                      GBusNameAppearedCallback  name_appeared_handler,
                                      GBusNameVanishedCallback  name_vanished_handler,
                                      xpointer_t                  user_data,
                                      xdestroy_notify_t            user_data_free_func);
XPL_AVAILABLE_IN_ALL
xuint_t g_bus_watch_name_on_connection (xdbus_connection_t          *connection,
                                      const xchar_t              *name,
                                      GBusNameWatcherFlags      flags,
                                      GBusNameAppearedCallback  name_appeared_handler,
                                      GBusNameVanishedCallback  name_vanished_handler,
                                      xpointer_t                  user_data,
                                      xdestroy_notify_t            user_data_free_func);
XPL_AVAILABLE_IN_ALL
xuint_t g_bus_watch_name_with_closures (GBusType                  bus_type,
                                      const xchar_t              *name,
                                      GBusNameWatcherFlags      flags,
                                      xclosure_t                 *name_appeared_closure,
                                      xclosure_t                 *name_vanished_closure);
XPL_AVAILABLE_IN_ALL
xuint_t g_bus_watch_name_on_connection_with_closures (
                                      xdbus_connection_t          *connection,
                                      const xchar_t              *name,
                                      GBusNameWatcherFlags      flags,
                                      xclosure_t                 *name_appeared_closure,
                                      xclosure_t                 *name_vanished_closure);
XPL_AVAILABLE_IN_ALL
void  g_bus_unwatch_name             (xuint_t                     watcher_id);

G_END_DECLS

#endif /* __G_DBUS_NAME_WATCHING_H__ */
