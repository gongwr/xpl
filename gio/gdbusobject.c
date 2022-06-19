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

#include "config.h"

#include "gdbusobject.h"
#include "gdbusinterface.h"
#include "gdbusutils.h"

#include "glibintl.h"

/**
 * SECTION:gdbusobject
 * @short_description: Base type for D-Bus objects
 * @include: gio/gio.h
 *
 * The #xdbus_object_t type is the base type for D-Bus objects on both
 * the service side (see #xdbus_object_skeleton_t) and the client side
 * (see #xdbus_object_proxy_t). It is essentially just a container of
 * interfaces.
 */

/**
 * xdbus_object_t:
 *
 * #xdbus_object_t is an opaque data structure and can only be accessed
 * using the following functions.
 */

typedef xdbus_object_iface_t GDBusObjectInterface;
G_DEFINE_INTERFACE (xdbus_object, g_dbus_object, XTYPE_OBJECT)

static void
g_dbus_object_default_init (xdbus_object_iface_t *iface)
{
  /**
   * xdbus_object_t::interface-added:
   * @object: The #xdbus_object_t emitting the signal.
   * @interface: The #xdbus_interface_t that was added.
   *
   * Emitted when @interface is added to @object.
   *
   * Since: 2.30
   */
  xsignal_new (I_("interface-added"),
                XTYPE_FROM_INTERFACE (iface),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (xdbus_object_iface_t, interface_added),
                NULL,
                NULL,
                NULL,
                XTYPE_NONE,
                1,
                XTYPE_DBUS_INTERFACE);

  /**
   * xdbus_object_t::interface-removed:
   * @object: The #xdbus_object_t emitting the signal.
   * @interface: The #xdbus_interface_t that was removed.
   *
   * Emitted when @interface is removed from @object.
   *
   * Since: 2.30
   */
  xsignal_new (I_("interface-removed"),
                XTYPE_FROM_INTERFACE (iface),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (xdbus_object_iface_t, interface_removed),
                NULL,
                NULL,
                NULL,
                XTYPE_NONE,
                1,
                XTYPE_DBUS_INTERFACE);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_object_get_object_path:
 * @object: A #xdbus_object_t.
 *
 * Gets the object path for @object.
 *
 * Returns: A string owned by @object. Do not free.
 *
 * Since: 2.30
 */
const xchar_t *
g_dbus_object_get_object_path (xdbus_object_t *object)
{
  xdbus_object_iface_t *iface = G_DBUS_OBJECT_GET_IFACE (object);
  return iface->get_object_path (object);
}

/**
 * g_dbus_object_get_interfaces:
 * @object: A #xdbus_object_t.
 *
 * Gets the D-Bus interfaces associated with @object.
 *
 * Returns: (element-type xdbus_interface_t) (transfer full): A list of #xdbus_interface_t instances.
 *   The returned list must be freed by xlist_free() after each element has been freed
 *   with xobject_unref().
 *
 * Since: 2.30
 */
xlist_t *
g_dbus_object_get_interfaces (xdbus_object_t *object)
{
  xdbus_object_iface_t *iface = G_DBUS_OBJECT_GET_IFACE (object);
  return iface->get_interfaces (object);
}

/**
 * g_dbus_object_get_interface:
 * @object: A #xdbus_object_t.
 * @interface_name: A D-Bus interface name.
 *
 * Gets the D-Bus interface with name @interface_name associated with
 * @object, if any.
 *
 * Returns: (nullable) (transfer full): %NULL if not found, otherwise a
 *   #xdbus_interface_t that must be freed with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_interface_t *
g_dbus_object_get_interface (xdbus_object_t *object,
                             const xchar_t *interface_name)
{
  xdbus_object_iface_t *iface = G_DBUS_OBJECT_GET_IFACE (object);
  xreturn_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);
  return iface->get_interface (object, interface_name);
}
