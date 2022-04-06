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

#ifndef __G_DBUS_OBJECT_H__
#define __G_DBUS_OBJECT_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_OBJECT         (g_dbus_object_get_type())
#define G_DBUS_OBJECT(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_OBJECT, xdbus_object))
#define X_IS_DBUS_OBJECT(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_OBJECT))
#define G_DBUS_OBJECT_GET_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE((o), XTYPE_DBUS_OBJECT, xdbus_object_iface_t))

typedef struct _xdbus_object_iface xdbus_object_iface_t;

/**
 * xdbus_object_iface_t:
 * @parent_iface: The parent interface.
 * @get_object_path: Returns the object path. See g_dbus_object_get_object_path().
 * @get_interfaces: Returns all interfaces. See g_dbus_object_get_interfaces().
 * @get_interface: Returns an interface by name. See g_dbus_object_get_interface().
 * @interface_added: Signal handler for the #xdbus_object_t::interface-added signal.
 * @interface_removed: Signal handler for the #xdbus_object_t::interface-removed signal.
 *
 * Base object type for D-Bus objects.
 *
 * Since: 2.30
 */
struct _xdbus_object_iface
{
  xtype_interface_t parent_iface;

  /* Virtual Functions */
  const xchar_t     *(*get_object_path) (xdbus_object_t  *object);
  xlist_t           *(*get_interfaces)  (xdbus_object_t  *object);
  xdbus_interface_t  *(*get_interface)   (xdbus_object_t  *object,
                                       const xchar_t  *interface_name);

  /* Signals */
  void (*interface_added)   (xdbus_object_t     *object,
                             xdbus_interface_t  *interface_);
  void (*interface_removed) (xdbus_object_t     *object,
                             xdbus_interface_t  *interface_);

};

XPL_AVAILABLE_IN_ALL
xtype_t            g_dbus_object_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
const xchar_t     *g_dbus_object_get_object_path (xdbus_object_t  *object);
XPL_AVAILABLE_IN_ALL
xlist_t           *g_dbus_object_get_interfaces  (xdbus_object_t  *object);
XPL_AVAILABLE_IN_ALL
xdbus_interface_t  *g_dbus_object_get_interface   (xdbus_object_t  *object,
                                                const xchar_t  *interface_name);

G_END_DECLS

#endif /* __G_DBUS_OBJECT_H__ */
