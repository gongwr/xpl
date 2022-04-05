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
#define G_DBUS_INTERFACE_SKELETON(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_INTERFACE_SKELETON, GDBusInterfaceSkeleton))
#define G_DBUS_INTERFACE_SKELETON_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_INTERFACE_SKELETON, GDBusInterfaceSkeletonClass))
#define G_DBUS_INTERFACE_SKELETON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_INTERFACE_SKELETON, GDBusInterfaceSkeletonClass))
#define X_IS_DBUS_INTERFACE_SKELETON(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_INTERFACE_SKELETON))
#define X_IS_DBUS_INTERFACE_SKELETON_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_INTERFACE_SKELETON))

typedef struct _GDBusInterfaceSkeletonClass   GDBusInterfaceSkeletonClass;
typedef struct _GDBusInterfaceSkeletonPrivate GDBusInterfaceSkeletonPrivate;

/**
 * GDBusInterfaceSkeleton:
 *
 * The #GDBusInterfaceSkeleton structure contains private data and should
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
 * @get_info: Returns a #GDBusInterfaceInfo. See g_dbus_interface_skeleton_get_info() for details.
 * @get_vtable: Returns a #GDBusInterfaceVTable. See g_dbus_interface_skeleton_get_vtable() for details.
 * @get_properties: Returns a #xvariant_t with all properties. See g_dbus_interface_skeleton_get_properties().
 * @flush: Emits outstanding changes, if any. See g_dbus_interface_skeleton_flush().
 * @g_authorize_method: Signal class handler for the #GDBusInterfaceSkeleton::g-authorize-method signal.
 *
 * Class structure for #GDBusInterfaceSkeleton.
 *
 * Since: 2.30
 */
struct _GDBusInterfaceSkeletonClass
{
  xobject_class_t parent_class;

  /* Virtual Functions */
  GDBusInterfaceInfo   *(*get_info)       (GDBusInterfaceSkeleton  *interface_);
  GDBusInterfaceVTable *(*get_vtable)     (GDBusInterfaceSkeleton  *interface_);
  xvariant_t             *(*get_properties) (GDBusInterfaceSkeleton  *interface_);
  void                  (*flush)          (GDBusInterfaceSkeleton  *interface_);

  /*< private >*/
  xpointer_t vfunc_padding[8];
  /*< public >*/

  /* Signals */
  xboolean_t (*g_authorize_method) (GDBusInterfaceSkeleton  *interface_,
                                  GDBusMethodInvocation   *invocation);

  /*< private >*/
  xpointer_t signal_padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t                        g_dbus_interface_skeleton_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
GDBusInterfaceSkeletonFlags  g_dbus_interface_skeleton_get_flags       (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
void                         g_dbus_interface_skeleton_set_flags       (GDBusInterfaceSkeleton      *interface_,
                                                                        GDBusInterfaceSkeletonFlags  flags);
XPL_AVAILABLE_IN_ALL
GDBusInterfaceInfo          *g_dbus_interface_skeleton_get_info        (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
GDBusInterfaceVTable        *g_dbus_interface_skeleton_get_vtable      (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
xvariant_t                    *g_dbus_interface_skeleton_get_properties  (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
void                         g_dbus_interface_skeleton_flush           (GDBusInterfaceSkeleton      *interface_);

XPL_AVAILABLE_IN_ALL
xboolean_t                     g_dbus_interface_skeleton_export          (GDBusInterfaceSkeleton      *interface_,
                                                                        GDBusConnection             *connection,
                                                                        const xchar_t                 *object_path,
                                                                        xerror_t                     **error);
XPL_AVAILABLE_IN_ALL
void                         g_dbus_interface_skeleton_unexport        (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
void                g_dbus_interface_skeleton_unexport_from_connection (GDBusInterfaceSkeleton      *interface_,
                                                                        GDBusConnection             *connection);

XPL_AVAILABLE_IN_ALL
GDBusConnection             *g_dbus_interface_skeleton_get_connection  (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
xlist_t                       *g_dbus_interface_skeleton_get_connections (GDBusInterfaceSkeleton      *interface_);
XPL_AVAILABLE_IN_ALL
xboolean_t                     g_dbus_interface_skeleton_has_connection  (GDBusInterfaceSkeleton      *interface_,
                                                                        GDBusConnection             *connection);
XPL_AVAILABLE_IN_ALL
const xchar_t                 *g_dbus_interface_skeleton_get_object_path (GDBusInterfaceSkeleton      *interface_);

G_END_DECLS

#endif /* __G_DBUS_INTERFACE_SKELETON_H */
