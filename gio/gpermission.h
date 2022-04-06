/*
 * Copyright © 2010 Codethink Limited
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_PERMISSION_H__
#define __G_PERMISSION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_PERMISSION             (g_permission_get_type ())
#define G_PERMISSION(inst)            (XTYPE_CHECK_INSTANCE_CAST ((inst),   \
                                       XTYPE_PERMISSION, xpermission))
#define G_PERMISSION_CLASS(class)     (XTYPE_CHECK_CLASS_CAST ((class),     \
                                       XTYPE_PERMISSION, GPermissionClass))
#define X_IS_PERMISSION(inst)         (XTYPE_CHECK_INSTANCE_TYPE ((inst),   \
                                       XTYPE_PERMISSION))
#define X_IS_PERMISSION_CLASS(class)  (XTYPE_CHECK_CLASS_TYPE ((class),     \
                                       XTYPE_PERMISSION))
#define G_PERMISSION_GET_CLASS(inst)  (XTYPE_INSTANCE_GET_CLASS ((inst),    \
                                       XTYPE_PERMISSION, GPermissionClass))

typedef struct _GPermissionPrivate    GPermissionPrivate;
typedef struct _GPermissionClass      GPermissionClass;

struct _GPermission
{
  xobject_t parent_instance;

  /*< private >*/
  GPermissionPrivate *priv;
};

struct _GPermissionClass {
  xobject_class_t parent_class;

  xboolean_t (*acquire)        (xpermission_t          *permission,
                              xcancellable_t         *cancellable,
                              xerror_t              **error);
  void     (*acquire_async)  (xpermission_t          *permission,
                              xcancellable_t         *cancellable,
                              xasync_ready_callback_t   callback,
                              xpointer_t              user_data);
  xboolean_t (*acquire_finish) (xpermission_t          *permission,
                              xasync_result_t         *result,
                              xerror_t              **error);

  xboolean_t (*release)        (xpermission_t          *permission,
                              xcancellable_t         *cancellable,
                              xerror_t              **error);
  void     (*release_async)  (xpermission_t          *permission,
                              xcancellable_t         *cancellable,
                              xasync_ready_callback_t   callback,
                              xpointer_t              user_data);
  xboolean_t (*release_finish) (xpermission_t          *permission,
                              xasync_result_t         *result,
                              xerror_t              **error);

  xpointer_t reserved[16];
};

XPL_AVAILABLE_IN_ALL
xtype_t           g_permission_get_type           (void);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_acquire            (xpermission_t          *permission,
                                                 xcancellable_t         *cancellable,
                                                 xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void            g_permission_acquire_async      (xpermission_t          *permission,
                                                 xcancellable_t         *cancellable,
                                                 xasync_ready_callback_t   callback,
                                                 xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_acquire_finish     (xpermission_t          *permission,
                                                 xasync_result_t         *result,
                                                 xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_release            (xpermission_t          *permission,
                                                 xcancellable_t         *cancellable,
                                                 xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void            g_permission_release_async      (xpermission_t          *permission,
                                                 xcancellable_t         *cancellable,
                                                 xasync_ready_callback_t   callback,
                                                 xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_release_finish     (xpermission_t          *permission,
                                                 xasync_result_t         *result,
                                                 xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_get_allowed        (xpermission_t   *permission);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_get_can_acquire    (xpermission_t   *permission);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_permission_get_can_release    (xpermission_t   *permission);

XPL_AVAILABLE_IN_ALL
void            g_permission_impl_update        (xpermission_t  *permission,
                                                 xboolean_t      allowed,
                                                 xboolean_t      can_acquire,
                                                 xboolean_t      can_release);

G_END_DECLS

#endif /* __G_PERMISSION_H__ */
