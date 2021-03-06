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

#ifndef __G_DBUS_OBJECT_SKELETON_H__
#define __G_DBUS_OBJECT_SKELETON_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DBUS_OBJECT_SKELETON         (xdbus_object_skeleton_get_type ())
#define G_DBUS_OBJECT_SKELETON(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_OBJECT_SKELETON, xdbus_object_skeleton))
#define G_DBUS_OBJECT_SKELETON_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DBUS_OBJECT_SKELETON, GDBusObjectSkeletonClass))
#define G_DBUS_OBJECT_SKELETON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_OBJECT_SKELETON, GDBusObjectSkeletonClass))
#define X_IS_DBUS_OBJECT_SKELETON(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_OBJECT_SKELETON))
#define X_IS_DBUS_OBJECT_SKELETON_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_OBJECT_SKELETON))

typedef struct _GDBusObjectSkeletonClass   GDBusObjectSkeletonClass;
typedef struct _xdbus_object_skeleton_private xdbus_object_skeleton_private_t;

/**
 * xdbus_object_skeleton_t:
 *
 * The #xdbus_object_skeleton_t structure contains private data and should only be
 * accessed using the provided API.
 *
 * Since: 2.30
 */
struct _GDBusObjectSkeleton
{
  /*< private >*/
  xobject_t parent_instance;
  xdbus_object_skeleton_private_t *priv;
};

/**
 * GDBusObjectSkeletonClass:
 * @parent_class: The parent class.
 * @authorize_method: Signal class handler for the #xdbus_object_skeleton_t::authorize-method signal.
 *
 * Class structure for #xdbus_object_skeleton_t.
 *
 * Since: 2.30
 */
struct _GDBusObjectSkeletonClass
{
  xobject_class_t parent_class;

  /* Signals */
  xboolean_t (*authorize_method) (xdbus_object_skeleton_t       *object,
                                xdbus_interface_skeleton_t    *interface_,
                                xdbus_method_invocation_t *invocation);

  /*< private >*/
  xpointer_t padding[8];
};

XPL_AVAILABLE_IN_ALL
xtype_t                xdbus_object_skeleton_get_type                  (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xdbus_object_skeleton_t *xdbus_object_skeleton_new                       (const xchar_t            *object_path);
XPL_AVAILABLE_IN_ALL
void                 xdbus_object_skeleton_flush                     (xdbus_object_skeleton_t    *object);
XPL_AVAILABLE_IN_ALL
void                 xdbus_object_skeleton_add_interface             (xdbus_object_skeleton_t    *object,
                                                                       xdbus_interface_skeleton_t *interface_);
XPL_AVAILABLE_IN_ALL
void                 xdbus_object_skeleton_remove_interface          (xdbus_object_skeleton_t    *object,
                                                                       xdbus_interface_skeleton_t *interface_);
XPL_AVAILABLE_IN_ALL
void                 xdbus_object_skeleton_remove_interface_by_name  (xdbus_object_skeleton_t    *object,
                                                                       const xchar_t            *interface_name);
XPL_AVAILABLE_IN_ALL
void                 xdbus_object_skeleton_set_object_path           (xdbus_object_skeleton_t    *object,
                                                                       const xchar_t            *object_path);

G_END_DECLS

#endif /* __G_DBUS_OBJECT_SKELETON_H */
