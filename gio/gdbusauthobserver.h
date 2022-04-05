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

#ifndef __G_DBUS_AUTH_OBSERVER_H__
#define __G_DBUS_AUTH_OBSERVER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_AUTH_OBSERVER         (g_dbus_auth_observer_get_type ())
#define G_DBUS_AUTH_OBSERVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_AUTH_OBSERVER, GDBusAuthObserver))
#define X_IS_DBUS_AUTH_OBSERVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_AUTH_OBSERVER))

XPL_AVAILABLE_IN_ALL
xtype_t              g_dbus_auth_observer_get_type                     (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
GDBusAuthObserver *g_dbus_auth_observer_new                          (void);
XPL_AVAILABLE_IN_ALL
xboolean_t           g_dbus_auth_observer_authorize_authenticated_peer (GDBusAuthObserver  *observer,
                                                                      xio_stream_t          *stream,
                                                                      GCredentials       *credentials);

XPL_AVAILABLE_IN_2_34
xboolean_t           g_dbus_auth_observer_allow_mechanism (GDBusAuthObserver  *observer,
                                                         const xchar_t        *mechanism);

G_END_DECLS

#endif /* _G_DBUS_AUTH_OBSERVER_H__ */
