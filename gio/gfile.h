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

#ifndef __XFILE_H__
#define __XFILE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_FILE            (xfile_get_type ())
#define XFILE(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_FILE, xfile))
#define X_IS_FILE(obj)	       (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_FILE))
#define XFILE_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_FILE, xfile_iface_t))

#if 0
/**
 * xfile_t:
 *
 * A handle to an object implementing the #xfile_iface_t interface.
 * Generally stores a location within the file system. Handles do not
 * necessarily represent files or directories that currently exist.
 **/
typedef struct _GFile         		xfile_t; /* Dummy typedef */
#endif
typedef struct _GFileIface    		xfile_iface_t;


/**
 * xfile_iface_t:
 * @x_iface: The parent interface.
 * @dup: Duplicates a #xfile_t.
 * @hash: Creates a hash of a #xfile_t.
 * @equal: Checks equality of two given #GFiles.
 * @is_native: Checks to see if a file is native to the system.
 * @has_uri_scheme: Checks to see if a #xfile_t has a given URI scheme.
 * @get_uri_scheme: Gets the URI scheme for a #xfile_t.
 * @get_basename: Gets the basename for a given #xfile_t.
 * @get_path: Gets the current path within a #xfile_t.
 * @get_uri: Gets a URI for the path within a #xfile_t.
 * @get_parse_name: Gets the parsed name for the #xfile_t.
 * @get_parent: Gets the parent directory for the #xfile_t.
 * @prefix_matches: Checks whether a #xfile_t contains a specified file.
 * @get_relative_path: Gets the path for a #xfile_t relative to a given path.
 * @resolve_relative_path: Resolves a relative path for a #xfile_t to an absolute path.
 * @get_child_for_display_name: Gets the child #xfile_t for a given display name.
 * @enumerate_children: Gets a #xfile_enumerator_t with the children of a #xfile_t.
 * @enumerate_children_async: Asynchronously gets a #xfile_enumerator_t with the children of a #xfile_t.
 * @enumerate_children_finish: Finishes asynchronously enumerating the children.
 * @query_info: Gets the #xfile_info_t for a #xfile_t.
 * @query_info_async: Asynchronously gets the #xfile_info_t for a #xfile_t.
 * @query_info_finish: Finishes an asynchronous query info operation.
 * @query_filesystem_info: Gets a #xfile_info_t for the file system #xfile_t is on.
 * @query_filesystem_info_async: Asynchronously gets a #xfile_info_t for the file system #xfile_t is on.
 * @query_filesystem_info_finish: Finishes asynchronously getting the file system info.
 * @find_enclosing_mount: Gets a #xmount_t for the #xfile_t.
 * @find_enclosing_mount_async: Asynchronously gets the #xmount_t for a #xfile_t.
 * @find_enclosing_mount_finish: Finishes asynchronously getting the volume.
 * @set_display_name: Sets the display name for a #xfile_t.
 * @set_display_name_async: Asynchronously sets a #xfile_t's display name.
 * @set_display_name_finish: Finishes asynchronously setting a #xfile_t's display name.
 * @query_settable_attributes: Returns a list of #GFileAttributeInfos that can be set.
 * @_query_settable_attributes_async: Asynchronously gets a list of #GFileAttributeInfos that can be set.
 * @_query_settable_attributes_finish: Finishes asynchronously querying settable attributes.
 * @query_writable_namespaces: Returns a list of #xfile_attribute_info_t namespaces that are writable.
 * @_query_writable_namespaces_async: Asynchronously gets a list of #xfile_attribute_info_t namespaces that are writable.
 * @_query_writable_namespaces_finish: Finishes asynchronously querying the writable namespaces.
 * @set_attribute: Sets a #xfile_attribute_info_t.
 * @set_attributes_from_info: Sets a #xfile_attribute_info_t with information from a #xfile_info_t.
 * @set_attributes_async: Asynchronously sets a file's attributes.
 * @set_attributes_finish: Finishes setting a file's attributes asynchronously.
 * @read_fn: Reads a file asynchronously.
 * @read_async: Asynchronously reads a file.
 * @read_finish: Finishes asynchronously reading a file.
 * @append_to: Writes to the end of a file.
 * @append_to_async: Asynchronously writes to the end of a file.
 * @append_to_finish: Finishes an asynchronous file append operation.
 * @create: Creates a new file.
 * @create_async: Asynchronously creates a file.
 * @create_finish: Finishes asynchronously creating a file.
 * @replace: Replaces the contents of a file.
 * @replace_async: Asynchronously replaces the contents of a file.
 * @replace_finish: Finishes asynchronously replacing a file.
 * @delete_file: Deletes a file.
 * @delete_file_async: Asynchronously deletes a file.
 * @delete_file_finish: Finishes an asynchronous delete.
 * @trash: Sends a #xfile_t to the Trash location.
 * @trash_async: Asynchronously sends a #xfile_t to the Trash location.
 * @trash_finish: Finishes an asynchronous file trashing operation.
 * @make_directory: Makes a directory.
 * @make_directory_async: Asynchronously makes a directory.
 * @make_directory_finish: Finishes making a directory asynchronously.
 * @make_symbolic_link: (nullable): Makes a symbolic link. %NULL if symbolic
 *    links are unsupported.
 * @_make_symbolic_link_async: Asynchronously makes a symbolic link
 * @_make_symbolic_link_finish: Finishes making a symbolic link asynchronously.
 * @copy: (nullable): Copies a file. %NULL if copying is unsupported, which will
 *     cause `xfile_t` to use a fallback copy method where it reads from the
 *     source and writes to the destination.
 * @copy_async: Asynchronously copies a file.
 * @copy_finish: Finishes an asynchronous copy operation.
 * @move: Moves a file.
 * @move_async: Asynchronously moves a file. Since: 2.72
 * @move_finish: Finishes an asynchronous move operation. Since: 2.72
 * @mount_mountable: Mounts a mountable object.
 * @mount_mountable_finish: Finishes a mounting operation.
 * @unmount_mountable: Unmounts a mountable object.
 * @unmount_mountable_finish: Finishes an unmount operation.
 * @eject_mountable: Ejects a mountable.
 * @eject_mountable_finish: Finishes an eject operation.
 * @mount_enclosing_volume: Mounts a specified location.
 * @mount_enclosing_volume_finish: Finishes mounting a specified location.
 * @monitor_dir: Creates a #xfile_monitor_t for the location.
 * @monitor_file: Creates a #xfile_monitor_t for the location.
 * @open_readwrite: Open file read/write. Since 2.22.
 * @open_readwrite_async: Asynchronously opens file read/write. Since 2.22.
 * @open_readwrite_finish: Finishes an asynchronous open read/write. Since 2.22.
 * @create_readwrite: Creates file read/write. Since 2.22.
 * @create_readwrite_async: Asynchronously creates file read/write. Since 2.22.
 * @create_readwrite_finish: Finishes an asynchronous creates read/write. Since 2.22.
 * @replace_readwrite: Replaces file read/write. Since 2.22.
 * @replace_readwrite_async: Asynchronously replaces file read/write. Since 2.22.
 * @replace_readwrite_finish: Finishes an asynchronous replace read/write. Since 2.22.
 * @start_mountable: Starts a mountable object. Since 2.22.
 * @start_mountable_finish: Finishes a start operation. Since 2.22.
 * @stop_mountable: Stops a mountable. Since 2.22.
 * @stop_mountable_finish: Finishes a stop operation. Since 2.22.
 * @supports_thread_contexts: a boolean that indicates whether the #xfile_t implementation supports thread-default contexts. Since 2.22.
 * @unmount_mountable_with_operation: Unmounts a mountable object using a #xmount_operation_t. Since 2.22.
 * @unmount_mountable_with_operation_finish: Finishes an unmount operation using a #xmount_operation_t. Since 2.22.
 * @eject_mountable_with_operation: Ejects a mountable object using a #xmount_operation_t. Since 2.22.
 * @eject_mountable_with_operation_finish: Finishes an eject operation using a #xmount_operation_t. Since 2.22.
 * @poll_mountable: Polls a mountable object for media changes. Since 2.22.
 * @poll_mountable_finish: Finishes a poll operation for media changes. Since 2.22.
 * @measure_disk_usage: Recursively measures the disk usage of @file. Since 2.38
 * @measure_disk_usage_async: Asynchronously recursively measures the disk usage of @file. Since 2.38
 * @measure_disk_usage_finish: Finishes an asynchronous recursive measurement of the disk usage of @file. Since 2.38
 *
 * An interface for writing VFS file handles.
 **/
struct _GFileIface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xfile_t *             (* dup)                         (xfile_t         *file);
  xuint_t               (* hash)                        (xfile_t         *file);
  xboolean_t            (* equal)                       (xfile_t         *file1,
                                                       xfile_t         *file2);
  xboolean_t            (* is_native)                   (xfile_t         *file);
  xboolean_t            (* has_uri_scheme)              (xfile_t         *file,
                                                       const char    *uri_scheme);
  char *              (* get_uri_scheme)              (xfile_t         *file);
  char *              (* get_basename)                (xfile_t         *file);
  char *              (* get_path)                    (xfile_t         *file);
  char *              (* get_uri)                     (xfile_t         *file);
  char *              (* get_parse_name)              (xfile_t         *file);
  xfile_t *             (* get_parent)                  (xfile_t         *file);
  xboolean_t            (* prefix_matches)              (xfile_t         *prefix,
                                                       xfile_t         *file);
  char *              (* get_relative_path)           (xfile_t         *parent,
                                                       xfile_t         *descendant);
  xfile_t *             (* resolve_relative_path)       (xfile_t        *file,
                                                       const char   *relative_path);
  xfile_t *             (* get_child_for_display_name)  (xfile_t        *file,
                                                       const char   *display_name,
                                                       xerror_t      **error);

  xfile_enumerator_t *   (* enumerate_children)          (xfile_t                *file,
                                                       const char           *attributes,
                                                       xfile_query_info_flags_t   flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* enumerate_children_async)    (xfile_t                *file,
                                                       const char           *attributes,
                                                       xfile_query_info_flags_t   flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_enumerator_t *   (* enumerate_children_finish)   (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_info_t *         (* query_info)                  (xfile_t                *file,
                                                       const char           *attributes,
                                                       xfile_query_info_flags_t   flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* query_info_async)            (xfile_t                *file,
                                                       const char           *attributes,
                                                       xfile_query_info_flags_t   flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_info_t *         (* query_info_finish)           (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_info_t *         (* query_filesystem_info)       (xfile_t                *file,
                                                       const char           *attributes,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* query_filesystem_info_async) (xfile_t                *file,
                                                       const char           *attributes,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_info_t *         (* query_filesystem_info_finish)(xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xmount_t *            (* find_enclosing_mount)        (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* find_enclosing_mount_async)  (xfile_t                *file,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xmount_t *            (* find_enclosing_mount_finish) (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_t *             (* set_display_name)            (xfile_t                *file,
                                                       const char           *display_name,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* set_display_name_async)      (xfile_t                *file,
                                                       const char           *display_name,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_t *             (* set_display_name_finish)     (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_attribute_info_list_t * (* query_settable_attributes)    (xfile_t          *file,
                                                             xcancellable_t   *cancellable,
                                                             xerror_t        **error);
  void                (* _query_settable_attributes_async)  (void);
  void                (* _query_settable_attributes_finish) (void);

  xfile_attribute_info_list_t * (* query_writable_namespaces)    (xfile_t          *file,
                                                             xcancellable_t   *cancellable,
                                                             xerror_t        **error);
  void                (* _query_writable_namespaces_async)  (void);
  void                (* _query_writable_namespaces_finish) (void);

  xboolean_t            (* set_attribute)               (xfile_t                *file,
                                                       const char           *attribute,
                                                       xfile_attribute_type_t    type,
                                                       xpointer_t              value_p,
                                                       xfile_query_info_flags_t   flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  xboolean_t            (* set_attributes_from_info)    (xfile_t                *file,
                                                       xfile_info_t            *info,
                                                       xfile_query_info_flags_t   flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* set_attributes_async)        (xfile_t                *file,
                                                       xfile_info_t            *info,
                                                       xfile_query_info_flags_t   flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* set_attributes_finish)       (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xfile_info_t           **info,
                                                       xerror_t              **error);

  xfile_input_stream_t *  (* read_fn)                     (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* read_async)                  (xfile_t                *file,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_input_stream_t *  (* read_finish)                 (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_output_stream_t * (* append_to)                   (xfile_t                *file,
                                                       xfile_create_flags_t      flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* append_to_async)             (xfile_t                *file,
                                                       xfile_create_flags_t      flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_output_stream_t * (* append_to_finish)            (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_output_stream_t * (* create)                      (xfile_t                *file,
                                                       xfile_create_flags_t      flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* create_async)                (xfile_t                *file,
                                                       xfile_create_flags_t      flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_output_stream_t * (* create_finish)               (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xfile_output_stream_t * (* replace)                     (xfile_t                *file,
                                                       const char           *etag,
                                                       xboolean_t              make_backup,
                                                       xfile_create_flags_t      flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* replace_async)               (xfile_t                *file,
                                                       const char           *etag,
                                                       xboolean_t              make_backup,
                                                       xfile_create_flags_t      flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_output_stream_t * (* replace_finish)              (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xboolean_t            (* delete_file)                 (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* delete_file_async)           (xfile_t                *file,
						       int                   io_priority,
						       xcancellable_t         *cancellable,
						       xasync_ready_callback_t   callback,
						       xpointer_t              user_data);
  xboolean_t            (* delete_file_finish)          (xfile_t                *file,
						       xasync_result_t         *result,
						       xerror_t              **error);

  xboolean_t            (* trash)                       (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* trash_async)                 (xfile_t                *file,
						       int                   io_priority,
						       xcancellable_t         *cancellable,
						       xasync_ready_callback_t   callback,
						       xpointer_t              user_data);
  xboolean_t            (* trash_finish)                (xfile_t                *file,
						       xasync_result_t         *result,
						       xerror_t              **error);

  xboolean_t            (* make_directory)              (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* make_directory_async)        (xfile_t                *file,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* make_directory_finish)       (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  xboolean_t            (* make_symbolic_link)          (xfile_t                *file,
                                                       const char           *symlink_value,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* _make_symbolic_link_async)   (void);
  void                (* _make_symbolic_link_finish)  (void);

  xboolean_t            (* copy)                        (xfile_t                *source,
                                                       xfile_t                *destination,
                                                       xfile_copy_flags_t        flags,
                                                       xcancellable_t         *cancellable,
                                                       xfile_progress_callback_t progress_callback,
                                                       xpointer_t              progress_callback_data,
                                                       xerror_t              **error);
  void                (* copy_async)                  (xfile_t                *source,
                                                       xfile_t                *destination,
                                                       xfile_copy_flags_t        flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xfile_progress_callback_t progress_callback,
                                                       xpointer_t              progress_callback_data,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* copy_finish)                 (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  xboolean_t            (* move)                        (xfile_t                *source,
                                                       xfile_t                *destination,
                                                       xfile_copy_flags_t        flags,
                                                       xcancellable_t         *cancellable,
                                                       xfile_progress_callback_t progress_callback,
                                                       xpointer_t              progress_callback_data,
                                                       xerror_t              **error);
  void                (* move_async)                  (xfile_t                *source,
                                                       xfile_t                *destination,
                                                       xfile_copy_flags_t        flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xfile_progress_callback_t progress_callback,
                                                       xpointer_t              progress_callback_data,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* move_finish)                 (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* mount_mountable)             (xfile_t                *file,
                                                       GMountMountFlags      flags,
                                                       xmount_operation_t      *mount_operation,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_t *             (* mount_mountable_finish)      (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* unmount_mountable)           (xfile_t                *file,
                                                       xmount_unmount_flags_t    flags,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* unmount_mountable_finish)    (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* eject_mountable)             (xfile_t                *file,
                                                       xmount_unmount_flags_t    flags,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* eject_mountable_finish)      (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* mount_enclosing_volume)      (xfile_t                *location,
                                                       GMountMountFlags      flags,
                                                       xmount_operation_t      *mount_operation,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t         (* mount_enclosing_volume_finish)  (xfile_t                *location,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  xfile_monitor_t *      (* monitor_dir)                 (xfile_t                *file,
                                                       xfile_monitor_flags_t     flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  xfile_monitor_t *      (* monitor_file)                (xfile_t                *file,
                                                       xfile_monitor_flags_t     flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);

  xfile_io_stream_t *     (* open_readwrite)              (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* open_readwrite_async)        (xfile_t                *file,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_io_stream_t *     (* open_readwrite_finish)       (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);
  xfile_io_stream_t *     (* create_readwrite)            (xfile_t                *file,
						       xfile_create_flags_t      flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* create_readwrite_async)      (xfile_t                *file,
						       xfile_create_flags_t      flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_io_stream_t *     (* create_readwrite_finish)      (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);
  xfile_io_stream_t *     (* replace_readwrite)           (xfile_t                *file,
                                                       const char           *etag,
                                                       xboolean_t              make_backup,
                                                       xfile_create_flags_t      flags,
                                                       xcancellable_t         *cancellable,
                                                       xerror_t              **error);
  void                (* replace_readwrite_async)     (xfile_t                *file,
                                                       const char           *etag,
                                                       xboolean_t              make_backup,
                                                       xfile_create_flags_t      flags,
                                                       int                   io_priority,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xfile_io_stream_t *     (* replace_readwrite_finish)    (xfile_t                *file,
                                                       xasync_result_t         *res,
                                                       xerror_t              **error);

  void                (* start_mountable)             (xfile_t                *file,
                                                       GDriveStartFlags      flags,
                                                       xmount_operation_t      *start_operation,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* start_mountable_finish)      (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* stop_mountable)              (xfile_t                *file,
                                                       xmount_unmount_flags_t    flags,
                                                       xmount_operation_t      *mount_operation,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* stop_mountable_finish)       (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  xboolean_t            supports_thread_contexts;

  void                (* unmount_mountable_with_operation) (xfile_t           *file,
                                                       xmount_unmount_flags_t    flags,
                                                       xmount_operation_t      *mount_operation,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* unmount_mountable_with_operation_finish) (xfile_t    *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* eject_mountable_with_operation) (xfile_t             *file,
                                                       xmount_unmount_flags_t    flags,
                                                       xmount_operation_t      *mount_operation,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* eject_mountable_with_operation_finish) (xfile_t      *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  void                (* poll_mountable)              (xfile_t                *file,
                                                       xcancellable_t         *cancellable,
                                                       xasync_ready_callback_t   callback,
                                                       xpointer_t              user_data);
  xboolean_t            (* poll_mountable_finish)       (xfile_t                *file,
                                                       xasync_result_t         *result,
                                                       xerror_t              **error);

  xboolean_t            (* measure_disk_usage)          (xfile_t                         *file,
                                                       xfile_measure_flags_t              flags,
                                                       xcancellable_t                  *cancellable,
                                                       xfile_measure_progress_callback_t   progress_callback,
                                                       xpointer_t                       progress_data,
                                                       xuint64_t                       *disk_usage,
                                                       xuint64_t                       *num_dirs,
                                                       xuint64_t                       *num_files,
                                                       xerror_t                       **error);
  void                (* measure_disk_usage_async)    (xfile_t                         *file,
                                                       xfile_measure_flags_t              flags,
                                                       xint_t                           io_priority,
                                                       xcancellable_t                  *cancellable,
                                                       xfile_measure_progress_callback_t   progress_callback,
                                                       xpointer_t                       progress_data,
                                                       xasync_ready_callback_t            callback,
                                                       xpointer_t                       user_data);
  xboolean_t            (* measure_disk_usage_finish)   (xfile_t                         *file,
                                                       xasync_result_t                  *result,
                                                       xuint64_t                       *disk_usage,
                                                       xuint64_t                       *num_dirs,
                                                       xuint64_t                       *num_files,
                                                       xerror_t                       **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t                   xfile_get_type                   (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_new_for_path               (const char                 *path);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_new_for_uri                (const char                 *uri);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_new_for_commandline_arg    (const char                 *arg);
XPL_AVAILABLE_IN_2_36
xfile_t *                 xfile_new_for_commandline_arg_and_cwd (const xchar_t           *arg,
                                                                const xchar_t           *cwd);
XPL_AVAILABLE_IN_2_32
xfile_t *                 xfile_new_tmp                    (const char                 *tmpl,
                                                           xfile_io_stream_t             **iostream,
                                                           xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_parse_name                 (const char                 *parse_name);
XPL_AVAILABLE_IN_2_56
xfile_t *                 xfile_new_build_filename         (const xchar_t                *first_element,
                                                           ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_dup                        (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
xuint_t                   xfile_hash                       (xconstpointer               file);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_equal                      (xfile_t                      *file1,
							   xfile_t                      *file2);
XPL_AVAILABLE_IN_ALL
char *                  xfile_get_basename               (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
char *                  xfile_get_path                   (xfile_t                      *file);
XPL_AVAILABLE_IN_2_56
const char *            xfile_peek_path                  (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
char *                  xfile_get_uri                    (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
char *                  xfile_get_parse_name             (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_get_parent                 (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_has_parent                 (xfile_t                      *file,
                                                           xfile_t                      *parent);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_get_child                  (xfile_t                      *file,
							   const char                 *name);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_get_child_for_display_name (xfile_t                      *file,
							   const char                 *display_name,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_has_prefix                 (xfile_t                      *file,
							   xfile_t                      *prefix);
XPL_AVAILABLE_IN_ALL
char *                  xfile_get_relative_path          (xfile_t                      *parent,
							   xfile_t                      *descendant);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_resolve_relative_path      (xfile_t                      *file,
							   const char                 *relative_path);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_is_native                  (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_has_uri_scheme             (xfile_t                      *file,
							   const char                 *uri_scheme);
XPL_AVAILABLE_IN_ALL
char *                  xfile_get_uri_scheme             (xfile_t                      *file);
XPL_AVAILABLE_IN_ALL
xfile_input_stream_t *      xfile_read                       (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_read_async                 (xfile_t                      *file,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_input_stream_t *      xfile_read_finish                (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_output_stream_t *     xfile_append_to                  (xfile_t                      *file,
							   xfile_create_flags_t             flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_output_stream_t *     xfile_create                     (xfile_t                      *file,
							   xfile_create_flags_t             flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_output_stream_t *     xfile_replace                    (xfile_t                      *file,
							   const char                 *etag,
							   xboolean_t                    make_backup,
							   xfile_create_flags_t            flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_append_to_async            (xfile_t                      *file,
							   xfile_create_flags_t            flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_output_stream_t *     xfile_append_to_finish           (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_create_async               (xfile_t                      *file,
							   xfile_create_flags_t            flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_output_stream_t *     xfile_create_finish              (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_replace_async              (xfile_t                      *file,
							   const char                 *etag,
							   xboolean_t                    make_backup,
							   xfile_create_flags_t            flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_output_stream_t *     xfile_replace_finish             (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_io_stream_t *         xfile_open_readwrite             (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_open_readwrite_async       (xfile_t                      *file,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_io_stream_t *         xfile_open_readwrite_finish      (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_io_stream_t *         xfile_create_readwrite           (xfile_t                      *file,
							   xfile_create_flags_t            flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_create_readwrite_async     (xfile_t                      *file,
							   xfile_create_flags_t            flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_io_stream_t *         xfile_create_readwrite_finish    (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_io_stream_t *         xfile_replace_readwrite          (xfile_t                      *file,
							   const char                 *etag,
							   xboolean_t                    make_backup,
							   xfile_create_flags_t            flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_replace_readwrite_async    (xfile_t                      *file,
							   const char                 *etag,
							   xboolean_t                    make_backup,
							   xfile_create_flags_t            flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_io_stream_t *         xfile_replace_readwrite_finish   (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_query_exists               (xfile_t                      *file,
							   xcancellable_t               *cancellable);
XPL_AVAILABLE_IN_ALL
xfile_type_t               xfile_query_file_type            (xfile_t                      *file,
                                                           xfile_query_info_flags_t         flags,
                                                           xcancellable_t               *cancellable);
XPL_AVAILABLE_IN_ALL
xfile_info_t *             xfile_query_info                 (xfile_t                      *file,
							   const char                 *attributes,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_query_info_async           (xfile_t                      *file,
							   const char                 *attributes,
							   xfile_query_info_flags_t         flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_info_t *             xfile_query_info_finish          (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_info_t *             xfile_query_filesystem_info      (xfile_t                      *file,
							   const char                 *attributes,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_query_filesystem_info_async (xfile_t                      *file,
							   const char                 *attributes,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_info_t *             xfile_query_filesystem_info_finish (xfile_t                      *file,
                                                           xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xmount_t *                xfile_find_enclosing_mount       (xfile_t                      *file,
                                                           xcancellable_t               *cancellable,
                                                           xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_find_enclosing_mount_async (xfile_t                      *file,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xmount_t *                xfile_find_enclosing_mount_finish (xfile_t                     *file,
							    xasync_result_t              *res,
							    xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
xfile_enumerator_t *       xfile_enumerate_children         (xfile_t                      *file,
							   const char                 *attributes,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_enumerate_children_async   (xfile_t                      *file,
							   const char                 *attributes,
							   xfile_query_info_flags_t         flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_enumerator_t *       xfile_enumerate_children_finish  (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_set_display_name           (xfile_t                      *file,
							   const char                 *display_name,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_set_display_name_async     (xfile_t                      *file,
							   const char                 *display_name,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_set_display_name_finish    (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_delete                     (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_2_34
void                    xfile_delete_async               (xfile_t                      *file,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);

XPL_AVAILABLE_IN_2_34
xboolean_t                xfile_delete_finish              (xfile_t                      *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_trash                      (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_2_38
void                    xfile_trash_async                (xfile_t                      *file,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);

XPL_AVAILABLE_IN_2_38
xboolean_t                xfile_trash_finish               (xfile_t                      *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_copy                       (xfile_t                      *source,
							   xfile_t                      *destination,
							   xfile_copy_flags_t              flags,
							   xcancellable_t               *cancellable,
							   xfile_progress_callback_t       progress_callback,
							   xpointer_t                    progress_callback_data,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_copy_async                 (xfile_t                      *source,
							   xfile_t                      *destination,
							   xfile_copy_flags_t              flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xfile_progress_callback_t       progress_callback,
							   xpointer_t                    progress_callback_data,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_copy_finish                (xfile_t                      *file,
							   xasync_result_t               *res,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_move                       (xfile_t                      *source,
							   xfile_t                      *destination,
							   xfile_copy_flags_t              flags,
							   xcancellable_t               *cancellable,
							   xfile_progress_callback_t       progress_callback,
							   xpointer_t                    progress_callback_data,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_2_72
void                    xfile_move_async                 (xfile_t                      *source,
							                                             xfile_t                      *destination,
							                                             xfile_copy_flags_t              flags,
							                                             int                         io_priority,
							                                             xcancellable_t               *cancellable,
							                                             xfile_progress_callback_t       progress_callback,
							                                             xpointer_t                    progress_callback_data,
							                                             xasync_ready_callback_t         callback,
							                                             xpointer_t                    user_data);
XPL_AVAILABLE_IN_2_72
xboolean_t                xfile_move_finish                (xfile_t                      *file,
							                                             xasync_result_t               *result,
							                                             xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_make_directory             (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_2_38
void                    xfile_make_directory_async       (xfile_t                      *file,
                                                           int                         io_priority,
                                                           xcancellable_t               *cancellable,
                                                           xasync_ready_callback_t         callback,
                                                           xpointer_t                    user_data);
XPL_AVAILABLE_IN_2_38
xboolean_t                xfile_make_directory_finish      (xfile_t                      *file,
                                                           xasync_result_t               *result,
                                                           xerror_t                    **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_make_directory_with_parents (xfile_t                     *file,
		                                           xcancellable_t               *cancellable,
		                                           xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_make_symbolic_link         (xfile_t                      *file,
							   const char                 *symlink_value,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_attribute_info_list_t *xfile_query_settable_attributes  (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xfile_attribute_info_list_t *xfile_query_writable_namespaces  (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute              (xfile_t                      *file,
							   const char                 *attribute,
							   xfile_attribute_type_t          type,
							   xpointer_t                    value_p,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attributes_from_info   (xfile_t                      *file,
							   xfile_info_t                  *info,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_set_attributes_async       (xfile_t                      *file,
							   xfile_info_t                  *info,
							   xfile_query_info_flags_t         flags,
							   int                         io_priority,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attributes_finish      (xfile_t                      *file,
							   xasync_result_t               *result,
							   xfile_info_t                 **info,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute_string       (xfile_t                      *file,
							   const char                 *attribute,
							   const char                 *value,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute_byte_string  (xfile_t                      *file,
							   const char                 *attribute,
							   const char                 *value,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute_uint32       (xfile_t                      *file,
							   const char                 *attribute,
							   xuint32_t                     value,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute_int32        (xfile_t                      *file,
							   const char                 *attribute,
							   gint32                      value,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute_uint64       (xfile_t                      *file,
							   const char                 *attribute,
							   xuint64_t                     value,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_set_attribute_int64        (xfile_t                      *file,
							   const char                 *attribute,
							   sint64_t                      value,
							   xfile_query_info_flags_t         flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_mount_enclosing_volume     (xfile_t                      *location,
							   GMountMountFlags            flags,
							   xmount_operation_t            *mount_operation,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_mount_enclosing_volume_finish (xfile_t                      *location,
							   xasync_result_t               *result,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_mount_mountable            (xfile_t                      *file,
							   GMountMountFlags            flags,
							   xmount_operation_t            *mount_operation,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xfile_t *                 xfile_mount_mountable_finish     (xfile_t                      *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);
XPL_DEPRECATED_FOR(xfile_unmount_mountable_with_operation)
void                    xfile_unmount_mountable          (xfile_t                      *file,
                                                           xmount_unmount_flags_t          flags,
                                                           xcancellable_t               *cancellable,
                                                           xasync_ready_callback_t         callback,
                                                           xpointer_t                    user_data);

XPL_DEPRECATED_FOR(xfile_unmount_mountable_with_operation_finish)
xboolean_t                xfile_unmount_mountable_finish   (xfile_t                      *file,
                                                           xasync_result_t               *result,
                                                           xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_unmount_mountable_with_operation (xfile_t                *file,
							   xmount_unmount_flags_t          flags,
							   xmount_operation_t            *mount_operation,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_unmount_mountable_with_operation_finish (xfile_t         *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);
XPL_DEPRECATED_FOR(xfile_eject_mountable_with_operation)
void                    xfile_eject_mountable            (xfile_t                      *file,
                                                           xmount_unmount_flags_t          flags,
                                                           xcancellable_t               *cancellable,
                                                           xasync_ready_callback_t         callback,
                                                           xpointer_t                    user_data);

XPL_DEPRECATED_FOR(xfile_eject_mountable_with_operation_finish)
xboolean_t                xfile_eject_mountable_finish     (xfile_t                      *file,
                                                           xasync_result_t               *result,
                                                           xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_eject_mountable_with_operation (xfile_t                  *file,
							   xmount_unmount_flags_t          flags,
							   xmount_operation_t            *mount_operation,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_eject_mountable_with_operation_finish (xfile_t           *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_2_68
char *			xfile_build_attribute_list_for_copy (xfile_t                   *file,
							   xfile_copy_flags_t              flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_copy_attributes            (xfile_t                      *source,
							   xfile_t                      *destination,
							   xfile_copy_flags_t              flags,
							   xcancellable_t               *cancellable,
							   xerror_t                    **error);


XPL_AVAILABLE_IN_ALL
xfile_monitor_t*           xfile_monitor_directory          (xfile_t                  *file,
							   xfile_monitor_flags_t       flags,
							   xcancellable_t           *cancellable,
							   xerror_t                **error);
XPL_AVAILABLE_IN_ALL
xfile_monitor_t*           xfile_monitor_file               (xfile_t                  *file,
							   xfile_monitor_flags_t       flags,
							   xcancellable_t           *cancellable,
							   xerror_t                **error);
XPL_AVAILABLE_IN_ALL
xfile_monitor_t*           xfile_monitor                    (xfile_t                  *file,
							   xfile_monitor_flags_t       flags,
							   xcancellable_t           *cancellable,
							   xerror_t                **error);

XPL_AVAILABLE_IN_2_38
xboolean_t                xfile_measure_disk_usage         (xfile_t                         *file,
                                                           xfile_measure_flags_t              flags,
                                                           xcancellable_t                  *cancellable,
                                                           xfile_measure_progress_callback_t   progress_callback,
                                                           xpointer_t                       progress_data,
                                                           xuint64_t                       *disk_usage,
                                                           xuint64_t                       *num_dirs,
                                                           xuint64_t                       *num_files,
                                                           xerror_t                       **error);

XPL_AVAILABLE_IN_2_38
void                    xfile_measure_disk_usage_async   (xfile_t                         *file,
                                                           xfile_measure_flags_t              flags,
                                                           xint_t                           io_priority,
                                                           xcancellable_t                  *cancellable,
                                                           xfile_measure_progress_callback_t   progress_callback,
                                                           xpointer_t                       progress_data,
                                                           xasync_ready_callback_t            callback,
                                                           xpointer_t                       user_data);

XPL_AVAILABLE_IN_2_38
xboolean_t                xfile_measure_disk_usage_finish  (xfile_t                         *file,
                                                           xasync_result_t                  *result,
                                                           xuint64_t                       *disk_usage,
                                                           xuint64_t                       *num_dirs,
                                                           xuint64_t                       *num_files,
                                                           xerror_t                       **error);

XPL_AVAILABLE_IN_ALL
void                    xfile_start_mountable            (xfile_t                      *file,
							   GDriveStartFlags            flags,
							   xmount_operation_t            *start_operation,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_start_mountable_finish     (xfile_t                      *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);
XPL_AVAILABLE_IN_ALL
void                    xfile_stop_mountable             (xfile_t                      *file,
							   xmount_unmount_flags_t          flags,
                                                           xmount_operation_t            *mount_operation,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_stop_mountable_finish      (xfile_t                      *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);

XPL_AVAILABLE_IN_ALL
void                    xfile_poll_mountable             (xfile_t                      *file,
							   xcancellable_t               *cancellable,
							   xasync_ready_callback_t         callback,
							   xpointer_t                    user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t                xfile_poll_mountable_finish      (xfile_t                      *file,
							   xasync_result_t               *result,
							   xerror_t                    **error);

/* Utilities */

XPL_AVAILABLE_IN_ALL
xapp_info_t *xfile_query_default_handler       (xfile_t                  *file,
					      xcancellable_t           *cancellable,
					      xerror_t                **error);
XPL_AVAILABLE_IN_2_60
void      xfile_query_default_handler_async (xfile_t                  *file,
                                              int                     io_priority,
                                              xcancellable_t           *cancellable,
                                              xasync_ready_callback_t     callback,
                                              xpointer_t                user_data);
XPL_AVAILABLE_IN_2_60
xapp_info_t *xfile_query_default_handler_finish (xfile_t                 *file,
                                               xasync_result_t          *result,
                                               xerror_t               **error);

XPL_AVAILABLE_IN_ALL
xboolean_t xfile_load_contents                (xfile_t                  *file,
					      xcancellable_t           *cancellable,
					      char                  **contents,
					      xsize_t                  *length,
					      char                  **etag_out,
					      xerror_t                **error);
XPL_AVAILABLE_IN_ALL
void     xfile_load_contents_async          (xfile_t                  *file,
					      xcancellable_t           *cancellable,
					      xasync_ready_callback_t     callback,
					      xpointer_t                user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_load_contents_finish         (xfile_t                  *file,
					      xasync_result_t           *res,
					      char                  **contents,
					      xsize_t                  *length,
					      char                  **etag_out,
					      xerror_t                **error);
XPL_AVAILABLE_IN_ALL
void     xfile_load_partial_contents_async  (xfile_t                  *file,
					      xcancellable_t           *cancellable,
					      xfile_read_more_callback_t   read_more_callback,
					      xasync_ready_callback_t     callback,
					      xpointer_t                user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_load_partial_contents_finish (xfile_t                  *file,
					      xasync_result_t           *res,
					      char                  **contents,
					      xsize_t                  *length,
					      char                  **etag_out,
					      xerror_t                **error);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_replace_contents             (xfile_t                  *file,
					      const char             *contents,
					      xsize_t                   length,
					      const char             *etag,
					      xboolean_t                make_backup,
					      xfile_create_flags_t        flags,
					      char                  **new_etag,
					      xcancellable_t           *cancellable,
					      xerror_t                **error);
XPL_AVAILABLE_IN_ALL
void     xfile_replace_contents_async       (xfile_t                  *file,
					      const char             *contents,
					      xsize_t                   length,
					      const char             *etag,
					      xboolean_t                make_backup,
					      xfile_create_flags_t        flags,
					      xcancellable_t           *cancellable,
					      xasync_ready_callback_t     callback,
					      xpointer_t                user_data);
XPL_AVAILABLE_IN_2_40
void     xfile_replace_contents_bytes_async (xfile_t                  *file,
					      xbytes_t                 *contents,
					      const char             *etag,
					      xboolean_t                make_backup,
					      xfile_create_flags_t        flags,
					      xcancellable_t           *cancellable,
					      xasync_ready_callback_t     callback,
					      xpointer_t                user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_replace_contents_finish      (xfile_t                  *file,
					      xasync_result_t           *res,
					      char                  **new_etag,
					      xerror_t                **error);

XPL_AVAILABLE_IN_ALL
xboolean_t xfile_supports_thread_contexts     (xfile_t                  *file);

XPL_AVAILABLE_IN_2_56
xbytes_t  *xfile_load_bytes                   (xfile_t                  *file,
                                              xcancellable_t           *cancellable,
                                              xchar_t                 **etag_out,
                                              xerror_t                **error);
XPL_AVAILABLE_IN_2_56
void     xfile_load_bytes_async             (xfile_t                  *file,
                                              xcancellable_t           *cancellable,
                                              xasync_ready_callback_t     callback,
                                              xpointer_t                user_data);
XPL_AVAILABLE_IN_2_56
xbytes_t  *xfile_load_bytes_finish            (xfile_t                  *file,
                                              xasync_result_t           *result,
                                              xchar_t                 **etag_out,
                                              xerror_t                **error);

G_END_DECLS

#endif /* __XFILE_H__ */
