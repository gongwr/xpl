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
#define G_DBUS_OBJECT(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_OBJECT, GDBusObject))
#define X_IS_DBUS_OBJECT(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_OBJECT))
#define G_DBUS_OBJECT_GET_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE((o), XTYPE_DBUS_OBJECT, GDBusObjectIface))

typedef struct _GDBusObjectIface GDBusObjectIface;

/**
 * GDBusObjectIface:
 * @parent_iface: The parent interface.
 * @get_object_path: Returns the object path. See g_dbus_object_get_object_path().
 * @get_interfaces: Returns all interfaces. See g_dbus_object_get_interfaces().
 * @get_interface: Returns an interface by name. See g_dbus_object_get_interface().
 * @interface_added: Signal handler for the #GDBusObject::interface-added signal.
 * @interface_removed: Signal handler for the #GDBusObject::interface-removed signal.
 *
 * Base object type for D-Bus objects.
 *
 * Since: 2.30
 */
struct _GDBusObjectIface
{
  xtype_interface_t parent_iface;

  /* Virtual Functions */
  const xchar_t     *(*get_object_path) (GDBusObject  *object);
  xlist_t           *(*get_interfaces)  (GDBusObject  *object);
  GDBusInterface  *(*get_interface)   (GDBusObject  *object,
                                       const xchar_t  *interface_name);

  /* Signals */
  void (*interface_added)   (GDBusObject     *object,
                             GDBusInterface  *interface_);
  void (*interface_removed) (GDBusObject     *object,
                             GDBusInterface  *interface_);

};

XPL_AVAILABLE_IN_ALL
xtype_t            g_dbus_object_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
const xchar_t     *g_dbus_object_get_object_path (GDBusObject  *object);
XPL_AVAILABLE_IN_ALL
xlist_t           *g_dbus_object_get_interfaces  (GDBusObject  *object);
XPL_AVAILABLE_IN_ALL
GDBusInterface  *g_dbus_object_get_interface   (GDBusObject  *object,
                                                const xchar_t  *interface_name);

G_END_DECLS

#endif /* __G_DBUS_OBJECT_H__ */
