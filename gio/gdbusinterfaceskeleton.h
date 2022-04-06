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

#ifndef __G_DBUS_INTERFACE_SKELETON_H__
#define __G_DBUS_INTERFACE_SKELETON_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_INTERFACE_SKELETON         (g_dbus_interface_skeleton_get_type ())
#define G_DBUS_INTERFACE_SKELETON(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_INTERFACE_SKELETON, xdbus_interface_skeleton))
#define G_DBUS_INTERFACE_SKELETON_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_INTERFACE_SKELETON, GDBusInterfaceSkeletonClass))
#define G_DBUS_INTERFACE_SKELETON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_INTERFACE_SKELETON, GDBusInterfaceSkeletonClass))
#define X_IS_DBUS_INTERFACE_SKELETON(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_INTERFACE_SKELETON))
#define X_IS_DBUS_INTERFACE_SKELETON_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_INTERFACE_SKELETON))

typedef struct _GDBusInterfaceSkeletonClass   GDBusInterfaceSkeletonClass;
typedef struct _GDBusInterfaceSkeletonPrivate GDBusInterfaceSkeletonPrivate;

/**
 * xdbus_interface_skeleton_t:
 *
 * The #xdbus_interface_skeleton_t structure contains private data and should
 * only be accessed using the provided API.
 *
 * Since: 2.30
 */
struct _GDBusInterfaceSkeleton
{
  /*< private >*/
  xobject_t parent_instance;
  GDBusInterfaceSkeletonPrivate *priv;
};

/**
 * GDBusInterfaceSkeletonClass:
 * @parent_class: The parent class.
 * @get_info: Returns a #xdbus_interface_info_t. See g_dbus_interface_skeleton_get_info() for details.
 * @get_vtable: Returns a #xdbus_interface_vtable_t. See g_dbus_interface_skeleton_get_vtable() for details.
 * @get_properties: Returns a #xvariant_t with all properties. See g_dbus_interface_skeleton_get_properties().
 * @flush: Emits outstanding changes, if any. See g_dbus_interface_skeleton_flush().
 * @g_authorize_method: Signal class handler for the #xdbus_interface_skeleton_t::g-authorize-method signal.
 *
 * Class structure for #xdbus_interface_skeleton_t.
 *
 * Since: 2.30
 */
struct _GDBusInterfaceSkeletonClass
{
  xobject_class_t parent_class;

  /* Virtual Functions */
  xdbus_interface_info_t   *(*get_info)       (xdbus_interface_skeleton_t  *interface_);
  xdbus_interface_vtable_t *(*get_vtable)     (xdbus_interface_skeleton_t  *interface_);
  xvariant_t             *(*get_properties) (xdbus_interface_skeleton_t  *interface_);
  void                  (*flush)          (xdbus_interface_skeleton_t  *interface_);

  /*< private >*/
  xpointer_t vfunc_padding[8];
  /*< public >*/

  /* Signals */
  xboolean_t (*g_authorize_method) (xdbus_interface_skeleton_t  *interface_,
                                  xdbus_method_invocation_t   *invocation);

  /*< private >*/
  xpointer_t signal_padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t                        g_dbus_interface_skeleton_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
GDBusInterfaceSkeletonFlags  g_dbus_interface_skeleton_get_flags       (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
void                         g_dbus_interface_skeleton_set_flags       (xdbus_interface_skeleton_t      *interface_,
                                                                        GDBusInterfaceSkeletonFlags  flags);
XPL_AVAILABLE_IN_ALL
xdbus_interface_info_t          *g_dbus_interface_skeleton_get_info        (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
xdbus_interface_vtable_t        *g_dbus_interface_skeleton_get_vtable      (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
xvariant_t                    *g_dbus_interface_skeleton_get_properties  (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
void                         g_dbus_interface_skeleton_flush           (xdbus_interface_skeleton_t      *interface_);

XPL_AVAILABLE_IN_ALL
xboolean_t                     g_dbus_interface_skeleton_export          (xdbus_interface_skeleton_t      *interface_,
                                                                        xdbus_connection_t             *connection,
                                                                        const xchar_t                 *object_path,
                                                                        xerror_t                     **error);
XPL_AVAILABLE_IN_ALL
void                         g_dbus_interface_skeleton_unexport        (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
void                g_dbus_interface_skeleton_unexport_from_connection (xdbus_interface_skeleton_t      *interface_,
                                                                        xdbus_connection_t             *connection);

XPL_AVAILABLE_IN_ALL
xdbus_connection_t             *g_dbus_interface_skeleton_get_connection  (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
xlist_t                       *g_dbus_interface_skeleton_get_connections (xdbus_interface_skeleton_t      *interface_);
XPL_AVAILABLE_IN_ALL
xboolean_t                     g_dbus_interface_skeleton_has_connection  (xdbus_interface_skeleton_t      *interface_,
                                                                        xdbus_connection_t             *connection);
XPL_AVAILABLE_IN_ALL
const xchar_t                 *g_dbus_interface_skeleton_get_object_path (xdbus_interface_skeleton_t      *interface_);

G_END_DECLS

#endif /* __G_DBUS_INTERFACE_SKELETON_H */
