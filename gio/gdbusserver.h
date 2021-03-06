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

#ifndef __G_DBUS_SERVER_H__
#define __G_DBUS_SERVER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_SERVER         (xdbus_server_get_type ())
#define G_DBUS_SERVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_SERVER, xdbus_server))
#define X_IS_DBUS_SERVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_SERVER))

XPL_AVAILABLE_IN_ALL
xtype_t             xdbus_server_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xdbus_server_t      *xdbus_server_new_sync           (const xchar_t       *address,
                                                    xdbus_server_flags_t   flags,
                                                    const xchar_t       *guid,
                                                    xdbus_auth_observer_t *observer,
                                                    xcancellable_t      *cancellable,
                                                    xerror_t           **error);
XPL_AVAILABLE_IN_ALL
const xchar_t      *xdbus_server_get_client_address (xdbus_server_t       *server);
XPL_AVAILABLE_IN_ALL
const xchar_t      *xdbus_server_get_guid           (xdbus_server_t       *server);
XPL_AVAILABLE_IN_ALL
xdbus_server_flags_t  xdbus_server_get_flags          (xdbus_server_t       *server);
XPL_AVAILABLE_IN_ALL
void              xdbus_server_start              (xdbus_server_t       *server);
XPL_AVAILABLE_IN_ALL
void              xdbus_server_stop               (xdbus_server_t       *server);
XPL_AVAILABLE_IN_ALL
xboolean_t          xdbus_server_is_active          (xdbus_server_t       *server);

G_END_DECLS

#endif /* __G_DBUS_SERVER_H__ */
