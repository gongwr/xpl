/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_VFS_H__
#define __G_VFS_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_VFS         (g_vfs_get_type ())
#define G_VFS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_VFS, GVfs))
#define G_VFS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_VFS, GVfsClass))
#define G_VFS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_VFS, GVfsClass))
#define X_IS_VFS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_VFS))
#define X_IS_VFS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_VFS))

/**
 * GVfsFileLookupFunc:
 * @vfs: a #GVfs
 * @identifier: the identifier to look up a #xfile_t for. This can either
 *     be an URI or a parse name as returned by g_file_get_parse_name()
 * @user_data: user data passed to the function
 *
 * This function type is used by g_vfs_register_uri_scheme() to make it
 * possible for a client to associate an URI scheme to a different #xfile_t
 * implementation.
 *
 * The client should return a reference to the new file that has been
 * created for @uri, or %NULL to continue with the default implementation.
 *
 * Returns: (transfer full): a #xfile_t for @identifier.
 *
 * Since: 2.50
 */
typedef xfile_t * (* GVfsFileLookupFunc) (GVfs       *vfs,
                                        const char *identifier,
                                        xpointer_t    user_data);

/**
 * G_VFS_EXTENSION_POINT_NAME:
 *
 * Extension point for #GVfs functionality.
 * See [Extending GIO][extending-gio].
 */
#define G_VFS_EXTENSION_POINT_NAME "gio-vfs"

/**
 * GVfs:
 *
 * Virtual File System object.
 **/
typedef struct _GVfsClass    GVfsClass;

struct _GVfs
{
  xobject_t parent_instance;
};

struct _GVfsClass
{
  xobject_class_t parent_class;

  /* Virtual Table */

  xboolean_t              (* is_active)                 (GVfs       *vfs);
  xfile_t               * (* get_file_for_path)         (GVfs       *vfs,
                                                       const char *path);
  xfile_t               * (* get_file_for_uri)          (GVfs       *vfs,
                                                       const char *uri);
  const xchar_t * const * (* get_supported_uri_schemes) (GVfs       *vfs);
  xfile_t               * (* parse_name)                (GVfs       *vfs,
                                                       const char *parse_name);

  /*< private >*/
  void                  (* local_file_add_info)       (GVfs       *vfs,
						       const char *filename,
						       guint64     device,
						       GFileAttributeMatcher *attribute_matcher,
						       GFileInfo  *info,
						       xcancellable_t *cancellable,
						       xpointer_t   *extra_data,
						       GDestroyNotify *free_extra_data);
  void                  (* add_writable_namespaces)   (GVfs       *vfs,
						       GFileAttributeInfoList *list);
  xboolean_t              (* local_file_set_attributes) (GVfs       *vfs,
						       const char *filename,
						       GFileInfo  *info,
                                                       GFileQueryInfoFlags flags,
                                                       xcancellable_t *cancellable,
						       xerror_t    **error);
  void                  (* local_file_removed)        (GVfs       *vfs,
						       const char *filename);
  void                  (* local_file_moved)          (GVfs       *vfs,
						       const char *source,
						       const char *dest);
  xicon_t *               (* deserialize_icon)          (GVfs       *vfs,
                                                       xvariant_t   *value);
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t                 g_vfs_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t              g_vfs_is_active                 (GVfs       *vfs);
XPL_AVAILABLE_IN_ALL
xfile_t *               g_vfs_get_file_for_path         (GVfs       *vfs,
                                                       const char *path);
XPL_AVAILABLE_IN_ALL
xfile_t *               g_vfs_get_file_for_uri          (GVfs       *vfs,
                                                       const char *uri);
XPL_AVAILABLE_IN_ALL
const xchar_t* const * g_vfs_get_supported_uri_schemes  (GVfs       *vfs);

XPL_AVAILABLE_IN_ALL
xfile_t *               g_vfs_parse_name                (GVfs       *vfs,
                                                       const char *parse_name);

XPL_AVAILABLE_IN_ALL
GVfs *                g_vfs_get_default               (void);
XPL_AVAILABLE_IN_ALL
GVfs *                g_vfs_get_local                 (void);

XPL_AVAILABLE_IN_2_50
xboolean_t              g_vfs_register_uri_scheme       (GVfs               *vfs,
                                                       const char         *scheme,
                                                       GVfsFileLookupFunc  uri_func,
                                                       xpointer_t            uri_data,
                                                       GDestroyNotify      uri_destroy,
                                                       GVfsFileLookupFunc  parse_name_func,
                                                       xpointer_t            parse_name_data,
                                                       GDestroyNotify      parse_name_destroy);
XPL_AVAILABLE_IN_2_50
xboolean_t              g_vfs_unregister_uri_scheme     (GVfs               *vfs,
                                                       const char         *scheme);


G_END_DECLS

#endif /* __G_VFS_H__ */
