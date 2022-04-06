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

#define XTYPE_VFS         (xvfs_get_type ())
#define XVFS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_VFS, xvfs))
#define XVFS_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_VFS, xvfs_class_t))
#define XVFS_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_VFS, xvfs_class_t))
#define X_IS_VFS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_VFS))
#define X_IS_VFS_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_VFS))

/**
 * xvfs_file_lookup_func_t:
 * @vfs: a #xvfs_t
 * @identifier: the identifier to look up a #xfile_t for. This can either
 *     be an URI or a parse name as returned by xfile_get_parse_name()
 * @user_data: user data passed to the function
 *
 * This function type is used by xvfs_register_uri_scheme() to make it
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
typedef xfile_t * (* xvfs_file_lookup_func_t) (xvfs_t       *vfs,
                                        const char *identifier,
                                        xpointer_t    user_data);

/**
 * XVFS_EXTENSION_POINT_NAME:
 *
 * Extension point for #xvfs_t functionality.
 * See [Extending GIO][extending-gio].
 */
#define XVFS_EXTENSION_POINT_NAME "gio-vfs"

/**
 * xvfs_t:
 *
 * Virtual File System object.
 **/
typedef struct _GVfsClass    xvfs_class_t;

struct _GVfs
{
  xobject_t parent_instance;
};

struct _GVfsClass
{
  xobject_class_t parent_class;

  /* Virtual Table */

  xboolean_t              (* is_active)                 (xvfs_t       *vfs);
  xfile_t               * (* get_file_for_path)         (xvfs_t       *vfs,
                                                       const char *path);
  xfile_t               * (* get_file_for_uri)          (xvfs_t       *vfs,
                                                       const char *uri);
  const xchar_t * const * (* get_supported_uri_schemes) (xvfs_t       *vfs);
  xfile_t               * (* parse_name)                (xvfs_t       *vfs,
                                                       const char *parse_name);

  /*< private >*/
  void                  (* local_file_add_info)       (xvfs_t       *vfs,
						       const char *filename,
						       xuint64_t     device,
						       xfile_attribute_matcher_t *attribute_matcher,
						       xfile_info_t  *info,
						       xcancellable_t *cancellable,
						       xpointer_t   *extra_data,
						       xdestroy_notify_t *free_extra_data);
  void                  (* add_writable_namespaces)   (xvfs_t       *vfs,
						       xfile_attribute_info_list_t *list);
  xboolean_t              (* local_file_set_attributes) (xvfs_t       *vfs,
						       const char *filename,
						       xfile_info_t  *info,
                                                       xfile_query_info_flags_t flags,
                                                       xcancellable_t *cancellable,
						       xerror_t    **error);
  void                  (* local_file_removed)        (xvfs_t       *vfs,
						       const char *filename);
  void                  (* local_file_moved)          (xvfs_t       *vfs,
						       const char *source,
						       const char *dest);
  xicon_t *               (* deserialize_icon)          (xvfs_t       *vfs,
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
xtype_t                 xvfs_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t              xvfs_is_active                 (xvfs_t       *vfs);
XPL_AVAILABLE_IN_ALL
xfile_t *               xvfs_get_file_for_path         (xvfs_t       *vfs,
                                                       const char *path);
XPL_AVAILABLE_IN_ALL
xfile_t *               xvfs_get_file_for_uri          (xvfs_t       *vfs,
                                                       const char *uri);
XPL_AVAILABLE_IN_ALL
const xchar_t* const * xvfs_get_supported_uri_schemes  (xvfs_t       *vfs);

XPL_AVAILABLE_IN_ALL
xfile_t *               xvfs_parse_name                (xvfs_t       *vfs,
                                                       const char *parse_name);

XPL_AVAILABLE_IN_ALL
xvfs_t *                xvfs_get_default               (void);
XPL_AVAILABLE_IN_ALL
xvfs_t *                xvfs_get_local                 (void);

XPL_AVAILABLE_IN_2_50
xboolean_t              xvfs_register_uri_scheme       (xvfs_t               *vfs,
                                                       const char         *scheme,
                                                       xvfs_file_lookup_func_t  uri_func,
                                                       xpointer_t            uri_data,
                                                       xdestroy_notify_t      uri_destroy,
                                                       xvfs_file_lookup_func_t  parse_name_func,
                                                       xpointer_t            parse_name_data,
                                                       xdestroy_notify_t      parse_name_destroy);
XPL_AVAILABLE_IN_2_50
xboolean_t              xvfs_unregister_uri_scheme     (xvfs_t               *vfs,
                                                       const char         *scheme);


G_END_DECLS

#endif /* __G_VFS_H__ */
