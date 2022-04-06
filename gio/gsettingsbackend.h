/*
 * Copyright © 2009, 2010 Codethink Limited
 * Copyright © 2010 Red Hat, Inc.
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Matthias Clasen <mclasen@redhat.com>
 */

#ifndef __G_SETTINGS_BACKEND_H__
#define __G_SETTINGS_BACKEND_H__

#if !defined (G_SETTINGS_ENABLE_BACKEND) && !defined (GIO_COMPILATION)
#error "You must define G_SETTINGS_ENABLE_BACKEND before including <gio/gsettingsbackend.h>."
#endif

#define __GIO_GIO_H_INSIDE__
#include <gio/giotypes.h>
#undef __GIO_GIO_H_INSIDE__

G_BEGIN_DECLS

#define XTYPE_SETTINGS_BACKEND                             (g_settings_backend_get_type ())
#define G_SETTINGS_BACKEND(inst)                            (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SETTINGS_BACKEND, xsettings_backend))
#define G_SETTINGS_BACKEND_CLASS(class)                     (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_SETTINGS_BACKEND, GSettingsBackendClass))
#define X_IS_SETTINGS_BACKEND(inst)                         (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SETTINGS_BACKEND))
#define X_IS_SETTINGS_BACKEND_CLASS(class)                  (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_SETTINGS_BACKEND))
#define G_SETTINGS_BACKEND_GET_CLASS(inst)                  (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_SETTINGS_BACKEND, GSettingsBackendClass))

/**
 * G_SETTINGS_BACKEND_EXTENSION_POINT_NAME:
 *
 * Extension point for #xsettings_backend_t functionality.
 **/
#define G_SETTINGS_BACKEND_EXTENSION_POINT_NAME "gsettings-backend"

/**
 * xsettings_backend_t:
 *
 * An implementation of a settings storage repository.
 **/
typedef struct _GSettingsBackendPrivate                     GSettingsBackendPrivate;
typedef struct _GSettingsBackendClass                       GSettingsBackendClass;

/**
 * GSettingsBackendClass:
 * @read: virtual method to read a key's value
 * @get_writable: virtual method to get if a key is writable
 * @write: virtual method to change key's value
 * @write_tree: virtual method to change a tree of keys
 * @reset: virtual method to reset state
 * @subscribe: virtual method to subscribe to key changes
 * @unsubscribe: virtual method to unsubscribe to key changes
 * @sync: virtual method to sync state
 * @get_permission: virtual method to get permission of a key
 * @read_user_value: virtual method to read user's key value
 *
 * Class structure for #xsettings_backend_t.
 */
struct _GSettingsBackendClass
{
  xobject_class_t parent_class;

  xvariant_t *    (*read)             (xsettings_backend_t    *backend,
                                     const xchar_t         *key,
                                     const xvariant_type_t  *expected_type,
                                     xboolean_t             default_value);

  xboolean_t      (*get_writable)     (xsettings_backend_t    *backend,
                                     const xchar_t         *key);

  xboolean_t      (*write)            (xsettings_backend_t    *backend,
                                     const xchar_t         *key,
                                     xvariant_t            *value,
                                     xpointer_t             origin_tag);
  xboolean_t      (*write_tree)       (xsettings_backend_t    *backend,
                                     xtree_t               *tree,
                                     xpointer_t             origin_tag);
  void          (*reset)            (xsettings_backend_t    *backend,
                                     const xchar_t         *key,
                                     xpointer_t             origin_tag);

  void          (*subscribe)        (xsettings_backend_t    *backend,
                                     const xchar_t         *name);
  void          (*unsubscribe)      (xsettings_backend_t    *backend,
                                     const xchar_t         *name);
  void          (*sync)             (xsettings_backend_t    *backend);

  xpermission_t * (*get_permission)   (xsettings_backend_t    *backend,
                                     const xchar_t         *path);

  xvariant_t *    (*read_user_value)  (xsettings_backend_t    *backend,
                                     const xchar_t         *key,
                                     const xvariant_type_t  *expected_type);

  /*< private >*/
  xpointer_t padding[23];
};

struct _GSettingsBackend
{
  xobject_t parent_instance;

  /*< private >*/
  GSettingsBackendPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                   g_settings_backend_get_type                     (void);

XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_changed                      (xsettings_backend_t    *backend,
                                                                         const xchar_t         *key,
                                                                         xpointer_t             origin_tag);
XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_path_changed                 (xsettings_backend_t    *backend,
                                                                         const xchar_t         *path,
                                                                         xpointer_t             origin_tag);
XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_flatten_tree                 (xtree_t               *tree,
                                                                         xchar_t              **path,
                                                                         const xchar_t       ***keys,
                                                                         xvariant_t          ***values);
XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_keys_changed                 (xsettings_backend_t    *backend,
                                                                         const xchar_t         *path,
                                                                         xchar_t const * const *items,
                                                                         xpointer_t             origin_tag);

XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_path_writable_changed        (xsettings_backend_t    *backend,
                                                                         const xchar_t         *path);
XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_writable_changed             (xsettings_backend_t    *backend,
                                                                         const xchar_t         *key);
XPL_AVAILABLE_IN_ALL
void                    g_settings_backend_changed_tree                 (xsettings_backend_t    *backend,
                                                                         xtree_t               *tree,
                                                                         xpointer_t             origin_tag);

XPL_AVAILABLE_IN_ALL
xsettings_backend_t *      g_settings_backend_get_default                  (void);

XPL_AVAILABLE_IN_ALL
xsettings_backend_t *      g_keyfile_settings_backend_new                  (const xchar_t         *filename,
                                                                         const xchar_t         *root_path,
                                                                         const xchar_t         *root_group);

XPL_AVAILABLE_IN_ALL
xsettings_backend_t *      g_null_settings_backend_new                     (void);

XPL_AVAILABLE_IN_ALL
xsettings_backend_t *      g_memory_settings_backend_new                   (void);

G_END_DECLS

#endif /* __G_SETTINGS_BACKEND_H__ */
