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
#include "gdbusobjectmanager.h"
#include "gdbusinterface.h"
#include "gdbusutils.h"

#include "glibintl.h"
#include "gmarshal-internal.h"

/**
 * SECTION:gdbusobjectmanager
 * @short_description: Base type for D-Bus object managers
 * @include: gio/gio.h
 *
 * The #xdbus_object_manager_t type is the base type for service- and
 * client-side implementations of the standardized
 * [org.freedesktop.DBus.ObjectManager](http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)
 * interface.
 *
 * See #xdbus_object_manager_client_t for the client-side implementation
 * and #xdbus_object_manager_server_t for the service-side implementation.
 */

/**
 * xdbus_object_manager_t:
 *
 * #xdbus_object_manager_t is an opaque data structure and can only be accessed
 * using the following functions.
 */

typedef xdbus_object_manager_iface_t GDBusObjectManagerInterface;
G_DEFINE_INTERFACE (xdbus_object_manager, g_dbus_object_manager, XTYPE_OBJECT)

enum {
  OBJECT_ADDED,
  OBJECT_REMOVED,
  INTERFACE_ADDED,
  INTERFACE_REMOVED,
  N_SIGNALS
};

static xuint_t signals[N_SIGNALS];

static void
g_dbus_object_manager_default_init (xdbus_object_manager_iface_t *iface)
{
  /**
   * xdbus_object_manager_t::object-added:
   * @manager: The #xdbus_object_manager_t emitting the signal.
   * @object: The #xdbus_object_t that was added.
   *
   * Emitted when @object is added to @manager.
   *
   * Since: 2.30
   */
  signals[OBJECT_ADDED] =
    xsignal_new (I_("object-added"),
                  XTYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xdbus_object_manager_iface_t, object_added),
                  NULL,
                  NULL,
                  NULL,
                  XTYPE_NONE,
                  1,
                  XTYPE_DBUS_OBJECT);

  /**
   * xdbus_object_manager_t::object-removed:
   * @manager: The #xdbus_object_manager_t emitting the signal.
   * @object: The #xdbus_object_t that was removed.
   *
   * Emitted when @object is removed from @manager.
   *
   * Since: 2.30
   */
  signals[OBJECT_REMOVED] =
    xsignal_new (I_("object-removed"),
                  XTYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xdbus_object_manager_iface_t, object_removed),
                  NULL,
                  NULL,
                  NULL,
                  XTYPE_NONE,
                  1,
                  XTYPE_DBUS_OBJECT);

  /**
   * xdbus_object_manager_t::interface-added:
   * @manager: The #xdbus_object_manager_t emitting the signal.
   * @object: The #xdbus_object_t on which an interface was added.
   * @interface: The #xdbus_interface_t that was added.
   *
   * Emitted when @interface is added to @object.
   *
   * This signal exists purely as a convenience to avoid having to
   * connect signals to all objects managed by @manager.
   *
   * Since: 2.30
   */
  signals[INTERFACE_ADDED] =
    xsignal_new (I_("interface-added"),
                  XTYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xdbus_object_manager_iface_t, interface_added),
                  NULL,
                  NULL,
                  _g_cclosure_marshal_VOID__OBJECT_OBJECT,
                  XTYPE_NONE,
                  2,
                  XTYPE_DBUS_OBJECT,
                  XTYPE_DBUS_INTERFACE);
  xsignal_set_va_marshaller (signals[INTERFACE_ADDED],
                              XTYPE_FROM_INTERFACE (iface),
                              _g_cclosure_marshal_VOID__OBJECT_OBJECTv);

  /**
   * xdbus_object_manager_t::interface-removed:
   * @manager: The #xdbus_object_manager_t emitting the signal.
   * @object: The #xdbus_object_t on which an interface was removed.
   * @interface: The #xdbus_interface_t that was removed.
   *
   * Emitted when @interface has been removed from @object.
   *
   * This signal exists purely as a convenience to avoid having to
   * connect signals to all objects managed by @manager.
   *
   * Since: 2.30
   */
  signals[INTERFACE_REMOVED] =
    xsignal_new (I_("interface-removed"),
                  XTYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xdbus_object_manager_iface_t, interface_removed),
                  NULL,
                  NULL,
                  _g_cclosure_marshal_VOID__OBJECT_OBJECT,
                  XTYPE_NONE,
                  2,
                  XTYPE_DBUS_OBJECT,
                  XTYPE_DBUS_INTERFACE);
  xsignal_set_va_marshaller (signals[INTERFACE_REMOVED],
                              XTYPE_FROM_INTERFACE (iface),
                              _g_cclosure_marshal_VOID__OBJECT_OBJECTv);

}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_object_manager_get_object_path:
 * @manager: A #xdbus_object_manager_t.
 *
 * Gets the object path that @manager is for.
 *
 * Returns: A string owned by @manager. Do not free.
 *
 * Since: 2.30
 */
const xchar_t *
g_dbus_object_manager_get_object_path (xdbus_object_manager_t *manager)
{
  xdbus_object_manager_iface_t *iface = G_DBUS_OBJECT_MANAGER_GET_IFACE (manager);
  return iface->get_object_path (manager);
}

/**
 * g_dbus_object_manager_get_objects:
 * @manager: A #xdbus_object_manager_t.
 *
 * Gets all #xdbus_object_t objects known to @manager.
 *
 * Returns: (transfer full) (element-type xdbus_object_t): A list of
 *   #xdbus_object_t objects. The returned list should be freed with
 *   xlist_free() after each element has been freed with
 *   xobject_unref().
 *
 * Since: 2.30
 */
xlist_t *
g_dbus_object_manager_get_objects (xdbus_object_manager_t *manager)
{
  xdbus_object_manager_iface_t *iface = G_DBUS_OBJECT_MANAGER_GET_IFACE (manager);
  return iface->get_objects (manager);
}

/**
 * g_dbus_object_manager_get_object:
 * @manager: A #xdbus_object_manager_t.
 * @object_path: Object path to look up.
 *
 * Gets the #xdbus_object_t at @object_path, if any.
 *
 * Returns: (transfer full) (nullable): A #xdbus_object_t or %NULL. Free with
 *   xobject_unref().
 *
 * Since: 2.30
 */
xdbus_object_t *
g_dbus_object_manager_get_object (xdbus_object_manager_t *manager,
                                  const xchar_t        *object_path)
{
  xdbus_object_manager_iface_t *iface = G_DBUS_OBJECT_MANAGER_GET_IFACE (manager);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), NULL);
  return iface->get_object (manager, object_path);
}

/**
 * g_dbus_object_manager_get_interface:
 * @manager: A #xdbus_object_manager_t.
 * @object_path: Object path to look up.
 * @interface_name: D-Bus interface name to look up.
 *
 * Gets the interface proxy for @interface_name at @object_path, if
 * any.
 *
 * Returns: (transfer full) (nullable): A #xdbus_interface_t instance or %NULL. Free
 *   with xobject_unref().
 *
 * Since: 2.30
 */
xdbus_interface_t *
g_dbus_object_manager_get_interface (xdbus_object_manager_t *manager,
                                     const xchar_t        *object_path,
                                     const xchar_t        *interface_name)
{
  xdbus_object_manager_iface_t *iface = G_DBUS_OBJECT_MANAGER_GET_IFACE (manager);
  xreturn_val_if_fail (xvariant_is_object_path (object_path), NULL);
  xreturn_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);
  return iface->get_interface (manager, object_path, interface_name);
}
