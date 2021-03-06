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

#ifndef __G_DBUS_INTERFACE_H__
#define __G_DBUS_INTERFACE_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_INTERFACE         (g_dbus_interface_get_type())
#define G_DBUS_INTERFACE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_INTERFACE, xdbus_interface))
#define X_IS_DBUS_INTERFACE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_INTERFACE))
#define G_DBUS_INTERFACE_GET_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE((o), XTYPE_DBUS_INTERFACE, xdbus_interface_iface_t))

/**
 * xdbus_interface_t:
 *
 * Base type for D-Bus interfaces.
 *
 * Since: 2.30
 */

typedef struct _GDBusInterfaceIface xdbus_interface_iface_t;

/**
 * xdbus_interface_iface_t:
 * @parent_iface: The parent interface.
 * @get_info: Returns a #xdbus_interface_info_t. See g_dbus_interface_get_info().
 * @get_object: Gets the enclosing #xdbus_object_t. See g_dbus_interface_get_object().
 * @set_object: Sets the enclosing #xdbus_object_t. See g_dbus_interface_set_object().
 * @dup_object: Gets a reference to the enclosing #xdbus_object_t. See g_dbus_interface_dup_object(). Added in 2.32.
 *
 * Base type for D-Bus interfaces.
 *
 * Since: 2.30
 */
struct _GDBusInterfaceIface
{
  xtype_interface_t parent_iface;

  /* Virtual Functions */
  xdbus_interface_info_t   *(*get_info)   (xdbus_interface_t      *interface_);
  xdbus_object_t          *(*get_object) (xdbus_interface_t      *interface_);
  void                  (*set_object) (xdbus_interface_t      *interface_,
                                       xdbus_object_t         *object);
  xdbus_object_t          *(*dup_object) (xdbus_interface_t      *interface_);
};

XPL_AVAILABLE_IN_ALL
xtype_t                 g_dbus_interface_get_type         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xdbus_interface_info_t   *g_dbus_interface_get_info         (xdbus_interface_t      *interface_);
XPL_AVAILABLE_IN_ALL
xdbus_object_t          *g_dbus_interface_get_object       (xdbus_interface_t      *interface_);
XPL_AVAILABLE_IN_ALL
void                  g_dbus_interface_set_object       (xdbus_interface_t      *interface_,
                                                         xdbus_object_t         *object);
XPL_AVAILABLE_IN_2_32
xdbus_object_t          *g_dbus_interface_dup_object       (xdbus_interface_t      *interface_);

G_END_DECLS

#endif /* __G_DBUS_INTERFACE_H__ */
