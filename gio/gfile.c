/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#include "config.h"

#ifdef __linux__
#include <sys/ioctl.h>
#include <errno.h>
/* See linux.git/fs/btrfs/ioctl.h */
#define BTRFS_IOCTL_MAGIC 0x94
#define BTRFS_IOC_CLONE _IOW(BTRFS_IOCTL_MAGIC, 9, int)
#endif

#ifdef HAVE_SPLICE
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/*
 * We duplicate the following Linux kernel header defines here so we can still
 * run at full speed on modern kernels in cases where an old toolchain was used
 * to build GLib. This is often done deliberately to allow shipping binaries
 * that need to run on a wide range of systems.
 */
#ifndef F_SETPIPE_SZ
#define F_SETPIPE_SZ 1031
#endif
#ifndef F_GETPIPE_SZ
#define F_GETPIPE_SZ 1032
#endif

#endif

#include <string.h>
#include <sys/types.h>

#include "gfile.h"
#include "glib/gstdio.h"
#ifdef G_OS_UNIX
#include "glib-unix.h"
#endif
#include "gvfs.h"
#include "gtask.h"
#include "gfileattribute-priv.h"
#include "gfiledescriptorbased.h"
#include "gpollfilemonitor.h"
#include "gappinfo.h"
#include "gfileinputstream.h"
#include "gfileoutputstream.h"
#include "glocalfileoutputstream.h"
#include "glocalfileiostream.h"
#include "glocalfile.h"
#include "gcancellable.h"
#include "gasyncresult.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gfile
 * @short_description: File and Directory Handling
 * @include: gio/gio.h
 * @see_also: #xfile_info_t, #xfile_enumerator_t
 *
 * #xfile_t is a high level abstraction for manipulating files on a
 * virtual file system. #GFiles are lightweight, immutable objects
 * that do no I/O upon creation. It is necessary to understand that
 * #xfile_t objects do not represent files, merely an identifier for a
 * file. All file content I/O is implemented as streaming operations
 * (see #xinput_stream_t and #xoutput_stream_t).
 *
 * To construct a #xfile_t, you can use:
 * - xfile_new_for_path() if you have a path.
 * - xfile_new_for_uri() if you have a URI.
 * - xfile_new_for_commandline_arg() for a command line argument.
 * - xfile_new_tmp() to create a temporary file from a template.
 * - xfile_parse_name() from a UTF-8 string gotten from xfile_get_parse_name().
 * - xfile_new_build_filename() to create a file from path elements.
 *
 * One way to think of a #xfile_t is as an abstraction of a pathname. For
 * normal files the system pathname is what is stored internally, but as
 * #GFiles are extensible it could also be something else that corresponds
 * to a pathname in a userspace implementation of a filesystem.
 *
 * #GFiles make up hierarchies of directories and files that correspond to
 * the files on a filesystem. You can move through the file system with
 * #xfile_t using xfile_get_parent() to get an identifier for the parent
 * directory, xfile_get_child() to get a child within a directory,
 * xfile_resolve_relative_path() to resolve a relative path between two
 * #GFiles. There can be multiple hierarchies, so you may not end up at
 * the same root if you repeatedly call xfile_get_parent() on two different
 * files.
 *
 * All #GFiles have a basename (get with xfile_get_basename()). These names
 * are byte strings that are used to identify the file on the filesystem
 * (relative to its parent directory) and there is no guarantees that they
 * have any particular charset encoding or even make any sense at all. If
 * you want to use filenames in a user interface you should use the display
 * name that you can get by requesting the
 * %XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME attribute with xfile_query_info().
 * This is guaranteed to be in UTF-8 and can be used in a user interface.
 * But always store the real basename or the #xfile_t to use to actually
 * access the file, because there is no way to go from a display name to
 * the actual name.
 *
 * Using #xfile_t as an identifier has the same weaknesses as using a path
 * in that there may be multiple aliases for the same file. For instance,
 * hard or soft links may cause two different #GFiles to refer to the same
 * file. Other possible causes for aliases are: case insensitive filesystems,
 * short and long names on FAT/NTFS, or bind mounts in Linux. If you want to
 * check if two #GFiles point to the same file you can query for the
 * %XFILE_ATTRIBUTE_ID_FILE attribute. Note that #xfile_t does some trivial
 * canonicalization of pathnames passed in, so that trivial differences in
 * the path string used at creation (duplicated slashes, slash at end of
 * path, "." or ".." path segments, etc) does not create different #GFiles.
 *
 * Many #xfile_t operations have both synchronous and asynchronous versions
 * to suit your application. Asynchronous versions of synchronous functions
 * simply have _async() appended to their function names. The asynchronous
 * I/O functions call a #xasync_ready_callback_t which is then used to finalize
 * the operation, producing a xasync_result_t which is then passed to the
 * function's matching _finish() operation.
 *
 * It is highly recommended to use asynchronous calls when running within a
 * shared main loop, such as in the main thread of an application. This avoids
 * I/O operations blocking other sources on the main loop from being dispatched.
 * Synchronous I/O operations should be performed from worker threads. See the
 * [introduction to asynchronous programming section][async-programming] for
 * more.
 *
 * Some #xfile_t operations almost always take a noticeable amount of time, and
 * so do not have synchronous analogs. Notable cases include:
 * - xfile_mount_mountable() to mount a mountable file.
 * - xfile_unmount_mountable_with_operation() to unmount a mountable file.
 * - xfile_eject_mountable_with_operation() to eject a mountable file.
 *
 * ## Entity Tags # {#gfile-etag}
 *
 * One notable feature of #GFiles are entity tags, or "etags" for
 * short. Entity tags are somewhat like a more abstract version of the
 * traditional mtime, and can be used to quickly determine if the file
 * has been modified from the version on the file system. See the
 * HTTP 1.1
 * [specification](http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html)
 * for HTTP Etag headers, which are a very similar concept.
 */

static void               xfile_real_query_info_async            (xfile_t                  *file,
                                                                   const char             *attributes,
                                                                   xfile_query_info_flags_t     flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_info_t *        xfile_real_query_info_finish           (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_query_filesystem_info_async (xfile_t                  *file,
                                                                   const char             *attributes,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_info_t *        xfile_real_query_filesystem_info_finish (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_enumerate_children_async    (xfile_t                  *file,
                                                                   const char             *attributes,
                                                                   xfile_query_info_flags_t     flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_enumerator_t *  xfile_real_enumerate_children_finish   (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_read_async                  (xfile_t                  *file,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_input_stream_t * xfile_real_read_finish                 (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_append_to_async             (xfile_t                  *file,
                                                                   xfile_create_flags_t        flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_output_stream_t *xfile_real_append_to_finish            (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_create_async                (xfile_t                  *file,
                                                                   xfile_create_flags_t        flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_output_stream_t *xfile_real_create_finish               (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_replace_async               (xfile_t                  *file,
                                                                   const char             *etag,
                                                                   xboolean_t                make_backup,
                                                                   xfile_create_flags_t        flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_output_stream_t *xfile_real_replace_finish              (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_delete_async                (xfile_t                  *file,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xboolean_t           xfile_real_delete_finish               (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_trash_async                 (xfile_t                  *file,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xboolean_t           xfile_real_trash_finish                (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_move_async                  (xfile_t                  *source,
                                                                   xfile_t                  *destination,
                                                                   xfile_copy_flags_t          flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xfile_progress_callback_t   progress_callback,
                                                                   xpointer_t                progress_callback_data,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xboolean_t           xfile_real_move_finish                 (xfile_t                  *file,
                                                                   xasync_result_t           *result,
                                                                   xerror_t                **error);
static void               xfile_real_make_directory_async        (xfile_t                  *file,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xboolean_t           xfile_real_make_directory_finish       (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_open_readwrite_async        (xfile_t                  *file,
                                                                   int                  io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_io_stream_t *    xfile_real_open_readwrite_finish       (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_create_readwrite_async      (xfile_t                  *file,
                                                                   xfile_create_flags_t        flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_io_stream_t *    xfile_real_create_readwrite_finish     (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_replace_readwrite_async     (xfile_t                  *file,
                                                                   const char             *etag,
                                                                   xboolean_t                make_backup,
                                                                   xfile_create_flags_t        flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_io_stream_t *    xfile_real_replace_readwrite_finish    (xfile_t                  *file,
                                                                  xasync_result_t            *res,
                                                                  xerror_t                 **error);
static xboolean_t           xfile_real_set_attributes_from_info    (xfile_t                  *file,
                                                                   xfile_info_t              *info,
                                                                   xfile_query_info_flags_t     flags,
                                                                   xcancellable_t           *cancellable,
                                                                   xerror_t                **error);
static void               xfile_real_set_display_name_async      (xfile_t                  *file,
                                                                   const char             *display_name,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xfile_t *            xfile_real_set_display_name_finish     (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_set_attributes_async        (xfile_t                  *file,
                                                                   xfile_info_t              *info,
                                                                   xfile_query_info_flags_t     flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xboolean_t           xfile_real_set_attributes_finish       (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xfile_info_t             **info,
                                                                   xerror_t                **error);
static void               xfile_real_find_enclosing_mount_async  (xfile_t                  *file,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xmount_t *           xfile_real_find_enclosing_mount_finish (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);
static void               xfile_real_copy_async                  (xfile_t                  *source,
                                                                   xfile_t                  *destination,
                                                                   xfile_copy_flags_t          flags,
                                                                   int                     io_priority,
                                                                   xcancellable_t           *cancellable,
                                                                   xfile_progress_callback_t   progress_callback,
                                                                   xpointer_t                progress_callback_data,
                                                                   xasync_ready_callback_t     callback,
                                                                   xpointer_t                user_data);
static xboolean_t           xfile_real_copy_finish                 (xfile_t                  *file,
                                                                   xasync_result_t           *res,
                                                                   xerror_t                **error);

static xboolean_t           xfile_real_measure_disk_usage          (xfile_t                         *file,
                                                                   xfile_measure_flags_t              flags,
                                                                   xcancellable_t                  *cancellable,
                                                                   xfile_measure_progress_callback_t   progress_callback,
                                                                   xpointer_t                       progress_data,
                                                                   xuint64_t                       *disk_usage,
                                                                   xuint64_t                       *num_dirs,
                                                                   xuint64_t                       *num_files,
                                                                   xerror_t                       **error);
static void               xfile_real_measure_disk_usage_async    (xfile_t                         *file,
                                                                   xfile_measure_flags_t              flags,
                                                                   xint_t                           io_priority,
                                                                   xcancellable_t                  *cancellable,
                                                                   xfile_measure_progress_callback_t   progress_callback,
                                                                   xpointer_t                       progress_data,
                                                                   xasync_ready_callback_t            callback,
                                                                   xpointer_t                       user_data);
static xboolean_t           xfile_real_measure_disk_usage_finish   (xfile_t                         *file,
                                                                   xasync_result_t                  *result,
                                                                   xuint64_t                       *disk_usage,
                                                                   xuint64_t                       *num_dirs,
                                                                   xuint64_t                       *num_files,
                                                                   xerror_t                       **error);

typedef xfile_iface_t GFileInterface;
G_DEFINE_INTERFACE (xfile, xfile, XTYPE_OBJECT)

static void
xfile_default_init (xfile_iface_t *iface)
{
  iface->enumerate_children_async = xfile_real_enumerate_children_async;
  iface->enumerate_children_finish = xfile_real_enumerate_children_finish;
  iface->set_display_name_async = xfile_real_set_display_name_async;
  iface->set_display_name_finish = xfile_real_set_display_name_finish;
  iface->query_info_async = xfile_real_query_info_async;
  iface->query_info_finish = xfile_real_query_info_finish;
  iface->query_filesystem_info_async = xfile_real_query_filesystem_info_async;
  iface->query_filesystem_info_finish = xfile_real_query_filesystem_info_finish;
  iface->set_attributes_async = xfile_real_set_attributes_async;
  iface->set_attributes_finish = xfile_real_set_attributes_finish;
  iface->read_async = xfile_real_read_async;
  iface->read_finish = xfile_real_read_finish;
  iface->append_to_async = xfile_real_append_to_async;
  iface->append_to_finish = xfile_real_append_to_finish;
  iface->create_async = xfile_real_create_async;
  iface->create_finish = xfile_real_create_finish;
  iface->replace_async = xfile_real_replace_async;
  iface->replace_finish = xfile_real_replace_finish;
  iface->delete_file_async = xfile_real_delete_async;
  iface->delete_file_finish = xfile_real_delete_finish;
  iface->trash_async = xfile_real_trash_async;
  iface->trash_finish = xfile_real_trash_finish;
  iface->move_async = xfile_real_move_async;
  iface->move_finish = xfile_real_move_finish;
  iface->make_directory_async = xfile_real_make_directory_async;
  iface->make_directory_finish = xfile_real_make_directory_finish;
  iface->open_readwrite_async = xfile_real_open_readwrite_async;
  iface->open_readwrite_finish = xfile_real_open_readwrite_finish;
  iface->create_readwrite_async = xfile_real_create_readwrite_async;
  iface->create_readwrite_finish = xfile_real_create_readwrite_finish;
  iface->replace_readwrite_async = xfile_real_replace_readwrite_async;
  iface->replace_readwrite_finish = xfile_real_replace_readwrite_finish;
  iface->find_enclosing_mount_async = xfile_real_find_enclosing_mount_async;
  iface->find_enclosing_mount_finish = xfile_real_find_enclosing_mount_finish;
  iface->set_attributes_from_info = xfile_real_set_attributes_from_info;
  iface->copy_async = xfile_real_copy_async;
  iface->copy_finish = xfile_real_copy_finish;
  iface->measure_disk_usage = xfile_real_measure_disk_usage;
  iface->measure_disk_usage_async = xfile_real_measure_disk_usage_async;
  iface->measure_disk_usage_finish = xfile_real_measure_disk_usage_finish;
}


/**
 * xfile_is_native:
 * @file: input #xfile_t
 *
 * Checks to see if a file is native to the platform.
 *
 * A native file is one expressed in the platform-native filename format,
 * e.g. "C:\Windows" or "/usr/bin/". This does not mean the file is local,
 * as it might be on a locally mounted remote filesystem.
 *
 * On some systems non-native files may be available using the native
 * filesystem via a userspace filesystem (FUSE), in these cases this call
 * will return %FALSE, but xfile_get_path() will still return a native path.
 *
 * This call does no blocking I/O.
 *
 * Returns: %TRUE if @file is native
 */
xboolean_t
xfile_is_native (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);

  iface = XFILE_GET_IFACE (file);

  return (* iface->is_native) (file);
}


/**
 * xfile_has_uri_scheme:
 * @file: input #xfile_t
 * @uri_scheme: a string containing a URI scheme
 *
 * Checks to see if a #xfile_t has a given URI scheme.
 *
 * This call does no blocking I/O.
 *
 * Returns: %TRUE if #xfile_t's backend supports the
 *   given URI scheme, %FALSE if URI scheme is %NULL,
 *   not supported, or #xfile_t is invalid.
 */
xboolean_t
xfile_has_uri_scheme (xfile_t      *file,
                       const char *uri_scheme)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (uri_scheme != NULL, FALSE);

  iface = XFILE_GET_IFACE (file);

  return (* iface->has_uri_scheme) (file, uri_scheme);
}


/**
 * xfile_get_uri_scheme:
 * @file: input #xfile_t
 *
 * Gets the URI scheme for a #xfile_t.
 * RFC 3986 decodes the scheme as:
 * |[
 * URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
 * ]|
 * Common schemes include "file", "http", "ftp", etc.
 *
 * The scheme can be different from the one used to construct the #xfile_t,
 * in that it might be replaced with one that is logically equivalent to the #xfile_t.
 *
 * This call does no blocking I/O.
 *
 * Returns: (nullable): a string containing the URI scheme for the given
 *   #xfile_t or %NULL if the #xfile_t was constructed with an invalid URI. The
 *   returned string should be freed with g_free() when no longer needed.
 */
char *
xfile_get_uri_scheme (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_uri_scheme) (file);
}


/**
 * xfile_get_basename: (virtual get_basename)
 * @file: input #xfile_t
 *
 * Gets the base name (the last component of the path) for a given #xfile_t.
 *
 * If called for the top level of a system (such as the filesystem root
 * or a uri like sftp://host/) it will return a single directory separator
 * (and on Windows, possibly a drive letter).
 *
 * The base name is a byte string (not UTF-8). It has no defined encoding
 * or rules other than it may not contain zero bytes.  If you want to use
 * filenames in a user interface you should use the display name that you
 * can get by requesting the %XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
 * attribute with xfile_query_info().
 *
 * This call does no blocking I/O.
 *
 * Returns: (type filename) (nullable): string containing the #xfile_t's
 *   base name, or %NULL if given #xfile_t is invalid. The returned string
 *   should be freed with g_free() when no longer needed.
 */
char *
xfile_get_basename (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_basename) (file);
}

/**
 * xfile_get_path: (virtual get_path)
 * @file: input #xfile_t
 *
 * Gets the local pathname for #xfile_t, if one exists. If non-%NULL, this is
 * guaranteed to be an absolute, canonical path. It might contain symlinks.
 *
 * This call does no blocking I/O.
 *
 * Returns: (type filename) (nullable): string containing the #xfile_t's path,
 *   or %NULL if no such path exists. The returned string should be freed
 *   with g_free() when no longer needed.
 */
char *
xfile_get_path (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_path) (file);
}

static const char *
file_peek_path_generic (xfile_t *file)
{
  const char *path;
  static xquark _file_path_quark = 0;

  if (G_UNLIKELY (_file_path_quark) == 0)
    _file_path_quark = g_quark_from_static_string ("gio-file-path");

  /* We need to be careful about threading, as two threads calling
   * xfile_peek_path() on the same file could race: both would see
   * (xobject_get_qdata(…) == NULL) to begin with, both would generate and add
   * the path, but the second thread to add it would end up freeing the path
   * set by the first thread. The first thread would still return the pointer
   * to that freed path, though, resulting an a read-after-free. Handle that
   * with a compare-and-swap loop. The xobject_*_qdata() functions are atomic. */

  while (TRUE)
    {
      xchar_t *new_path = NULL;

      path = xobject_get_qdata ((xobject_t*)file, _file_path_quark);

      if (path != NULL)
        break;

      new_path = xfile_get_path (file);
      if (new_path == NULL)
        return NULL;

      /* By passing NULL here, we ensure we never replace existing data: */
      if (xobject_replace_qdata ((xobject_t *) file, _file_path_quark,
                                  NULL, (xpointer_t) new_path,
                                  (xdestroy_notify_t) g_free, NULL))
        {
          path = new_path;
          break;
        }
      else
        g_free (new_path);
    }

  return path;
}

/**
 * xfile_peek_path:
 * @file: input #xfile_t
 *
 * Exactly like xfile_get_path(), but caches the result via
 * xobject_set_qdata_full().  This is useful for example in C
 * applications which mix `xfile_*` APIs with native ones.  It
 * also avoids an extra duplicated string when possible, so will be
 * generally more efficient.
 *
 * This call does no blocking I/O.
 *
 * Returns: (type filename) (nullable): string containing the #xfile_t's path,
 *   or %NULL if no such path exists. The returned string is owned by @file.
 * Since: 2.56
 */
const char *
xfile_peek_path (xfile_t *file)
{
  if (X_IS_LOCAL_FILE (file))
    return _g_local_file_get_filename ((GLocalFile *) file);
  return file_peek_path_generic (file);
}

/**
 * xfile_get_uri:
 * @file: input #xfile_t
 *
 * Gets the URI for the @file.
 *
 * This call does no blocking I/O.
 *
 * Returns: a string containing the #xfile_t's URI. If the #xfile_t was constructed
 *   with an invalid URI, an invalid URI is returned.
 *   The returned string should be freed with g_free()
 *   when no longer needed.
 */
char *
xfile_get_uri (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_uri) (file);
}

/**
 * xfile_get_parse_name:
 * @file: input #xfile_t
 *
 * Gets the parse name of the @file.
 * A parse name is a UTF-8 string that describes the
 * file such that one can get the #xfile_t back using
 * xfile_parse_name().
 *
 * This is generally used to show the #xfile_t as a nice
 * full-pathname kind of string in a user interface,
 * like in a location entry.
 *
 * For local files with names that can safely be converted
 * to UTF-8 the pathname is used, otherwise the IRI is used
 * (a form of URI that allows UTF-8 characters unescaped).
 *
 * This call does no blocking I/O.
 *
 * Returns: a string containing the #xfile_t's parse name.
 *   The returned string should be freed with g_free()
 *   when no longer needed.
 */
char *
xfile_get_parse_name (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_parse_name) (file);
}

/**
 * xfile_dup:
 * @file: input #xfile_t
 *
 * Duplicates a #xfile_t handle. This operation does not duplicate
 * the actual file or directory represented by the #xfile_t; see
 * xfile_copy() if attempting to copy a file.
 *
 * xfile_dup() is useful when a second handle is needed to the same underlying
 * file, for use in a separate thread (#xfile_t is not thread-safe). For use
 * within the same thread, use xobject_ref() to increment the existing object’s
 * reference count.
 *
 * This call does no blocking I/O.
 *
 * Returns: (transfer full): a new #xfile_t that is a duplicate
 *   of the given #xfile_t.
 */
xfile_t *
xfile_dup (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->dup) (file);
}

/**
 * xfile_hash:
 * @file: (type xfile_t): #xconstpointer to a #xfile_t
 *
 * Creates a hash value for a #xfile_t.
 *
 * This call does no blocking I/O.
 *
 * Virtual: hash
 * Returns: 0 if @file is not a valid #xfile_t, otherwise an
 *   integer that can be used as hash value for the #xfile_t.
 *   This function is intended for easily hashing a #xfile_t to
 *   add to a #xhashtable_t or similar data structure.
 */
xuint_t
xfile_hash (xconstpointer file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), 0);

  iface = XFILE_GET_IFACE (file);

  return (* iface->hash) ((xfile_t *)file);
}

/**
 * xfile_equal:
 * @file1: the first #xfile_t
 * @file2: the second #xfile_t
 *
 * Checks if the two given #GFiles refer to the same file.
 *
 * Note that two #GFiles that differ can still refer to the same
 * file on the filesystem due to various forms of filename
 * aliasing.
 *
 * This call does no blocking I/O.
 *
 * Returns: %TRUE if @file1 and @file2 are equal.
 */
xboolean_t
xfile_equal (xfile_t *file1,
              xfile_t *file2)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file1), FALSE);
  g_return_val_if_fail (X_IS_FILE (file2), FALSE);

  if (file1 == file2)
    return TRUE;

  if (XTYPE_FROM_INSTANCE (file1) != XTYPE_FROM_INSTANCE (file2))
    return FALSE;

  iface = XFILE_GET_IFACE (file1);

  return (* iface->equal) (file1, file2);
}


/**
 * xfile_get_parent:
 * @file: input #xfile_t
 *
 * Gets the parent directory for the @file.
 * If the @file represents the root directory of the
 * file system, then %NULL will be returned.
 *
 * This call does no blocking I/O.
 *
 * Returns: (nullable) (transfer full): a #xfile_t structure to the
 *   parent of the given #xfile_t or %NULL if there is no parent. Free
 *   the returned object with xobject_unref().
 */
xfile_t *
xfile_get_parent (xfile_t *file)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_parent) (file);
}

/**
 * xfile_has_parent:
 * @file: input #xfile_t
 * @parent: (nullable): the parent to check for, or %NULL
 *
 * Checks if @file has a parent, and optionally, if it is @parent.
 *
 * If @parent is %NULL then this function returns %TRUE if @file has any
 * parent at all.  If @parent is non-%NULL then %TRUE is only returned
 * if @file is an immediate child of @parent.
 *
 * Returns: %TRUE if @file is an immediate child of @parent (or any parent in
 *   the case that @parent is %NULL).
 *
 * Since: 2.24
 */
xboolean_t
xfile_has_parent (xfile_t *file,
                   xfile_t *parent)
{
  xfile_t *actual_parent;
  xboolean_t result;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (parent == NULL || X_IS_FILE (parent), FALSE);

  actual_parent = xfile_get_parent (file);

  if (actual_parent != NULL)
    {
      if (parent != NULL)
        result = xfile_equal (parent, actual_parent);
      else
        result = TRUE;

      xobject_unref (actual_parent);
    }
  else
    result = FALSE;

  return result;
}

/**
 * xfile_get_child:
 * @file: input #xfile_t
 * @name: (type filename): string containing the child's basename
 *
 * Gets a child of @file with basename equal to @name.
 *
 * Note that the file with that specific name might not exist, but
 * you can still have a #xfile_t that points to it. You can use this
 * for instance to create that file.
 *
 * This call does no blocking I/O.
 *
 * Returns: (transfer full): a #xfile_t to a child specified by @name.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_get_child (xfile_t      *file,
                  const char *name)
{
  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (!g_path_is_absolute (name), NULL);

  return xfile_resolve_relative_path (file, name);
}

/**
 * xfile_get_child_for_display_name:
 * @file: input #xfile_t
 * @display_name: string to a possible child
 * @error: return location for an error
 *
 * Gets the child of @file for a given @display_name (i.e. a UTF-8
 * version of the name). If this function fails, it returns %NULL
 * and @error will be set. This is very useful when constructing a
 * #xfile_t for a new file and the user entered the filename in the
 * user interface, for instance when you select a directory and
 * type a filename in the file selector.
 *
 * This call does no blocking I/O.
 *
 * Returns: (transfer full): a #xfile_t to the specified child, or
 *   %NULL if the display name couldn't be converted.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_get_child_for_display_name (xfile_t      *file,
                                   const char *display_name,
                                   xerror_t **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (display_name != NULL, NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->get_child_for_display_name) (file, display_name, error);
}

/**
 * xfile_has_prefix:
 * @file: input #xfile_t
 * @prefix: input #xfile_t
 *
 * Checks whether @file has the prefix specified by @prefix.
 *
 * In other words, if the names of initial elements of @file's
 * pathname match @prefix. Only full pathname elements are matched,
 * so a path like /foo is not considered a prefix of /foobar, only
 * of /foo/bar.
 *
 * A #xfile_t is not a prefix of itself. If you want to check for
 * equality, use xfile_equal().
 *
 * This call does no I/O, as it works purely on names. As such it can
 * sometimes return %FALSE even if @file is inside a @prefix (from a
 * filesystem point of view), because the prefix of @file is an alias
 * of @prefix.
 *
 * Virtual: prefix_matches
 * Returns:  %TRUE if the @file's parent, grandparent, etc is @prefix,
 *   %FALSE otherwise.
 */
xboolean_t
xfile_has_prefix (xfile_t *file,
                   xfile_t *prefix)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_FILE (prefix), FALSE);

  if (XTYPE_FROM_INSTANCE (file) != XTYPE_FROM_INSTANCE (prefix))
    return FALSE;

  iface = XFILE_GET_IFACE (file);

  /* The vtable function differs in arg order since
   * we're using the old contains_file call
   */
  return (* iface->prefix_matches) (prefix, file);
}

/**
 * xfile_get_relative_path: (virtual get_relative_path)
 * @parent: input #xfile_t
 * @descendant: input #xfile_t
 *
 * Gets the path for @descendant relative to @parent.
 *
 * This call does no blocking I/O.
 *
 * Returns: (type filename) (nullable): string with the relative path from
 *   @descendant to @parent, or %NULL if @descendant doesn't have @parent as
 *   prefix. The returned string should be freed with g_free() when
 *   no longer needed.
 */
char *
xfile_get_relative_path (xfile_t *parent,
                          xfile_t *descendant)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (parent), NULL);
  g_return_val_if_fail (X_IS_FILE (descendant), NULL);

  if (XTYPE_FROM_INSTANCE (parent) != XTYPE_FROM_INSTANCE (descendant))
    return NULL;

  iface = XFILE_GET_IFACE (parent);

  return (* iface->get_relative_path) (parent, descendant);
}

/**
 * xfile_resolve_relative_path:
 * @file: input #xfile_t
 * @relative_path: (type filename): a given relative path string
 *
 * Resolves a relative path for @file to an absolute path.
 *
 * This call does no blocking I/O.
 *
 * If the @relative_path is an absolute path name, the resolution
 * is done absolutely (without taking @file path as base).
 *
 * Returns: (transfer full): a #xfile_t for the resolved path.
 */
xfile_t *
xfile_resolve_relative_path (xfile_t      *file,
                              const char *relative_path)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (relative_path != NULL, NULL);

  iface = XFILE_GET_IFACE (file);

  return (* iface->resolve_relative_path) (file, relative_path);
}

/**
 * xfile_enumerate_children:
 * @file: input #xfile_t
 * @attributes: an attribute query string
 * @flags: a set of #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: #xerror_t for error reporting
 *
 * Gets the requested information about the files in a directory.
 * The result is a #xfile_enumerator_t object that will give out
 * #xfile_info_t objects for all the files in the directory.
 *
 * The @attributes value is a string that specifies the file
 * attributes that should be gathered. It is not an error if
 * it's not possible to read a particular requested attribute
 * from a file - it just won't be set. @attributes should
 * be a comma-separated list of attributes or attribute wildcards.
 * The wildcard "*" means all attributes, and a wildcard like
 * "standard::*" means all attributes in the standard namespace.
 * An example attribute query be "standard::*,owner::user".
 * The standard attributes are available as defines, like
 * %XFILE_ATTRIBUTE_STANDARD_NAME. %XFILE_ATTRIBUTE_STANDARD_NAME should
 * always be specified if you plan to call xfile_enumerator_get_child() or
 * xfile_enumerator_iterate() on the returned enumerator.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * If the file does not exist, the %G_IO_ERROR_NOT_FOUND error will
 * be returned. If the file is not a directory, the %G_IO_ERROR_NOT_DIRECTORY
 * error will be returned. Other errors are possible too.
 *
 * Returns: (transfer full): A #xfile_enumerator_t if successful,
 *   %NULL on error. Free the returned object with xobject_unref().
 */
xfile_enumerator_t *
xfile_enumerate_children (xfile_t                *file,
                           const char           *attributes,
                           xfile_query_info_flags_t   flags,
                           xcancellable_t         *cancellable,
                           xerror_t              **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->enumerate_children == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->enumerate_children) (file, attributes, flags,
                                        cancellable, error);
}

/**
 * xfile_enumerate_children_async:
 * @file: input #xfile_t
 * @attributes: an attribute query string
 * @flags: a set of #xfile_query_info_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call when the
 *   request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously gets the requested information about the files
 * in a directory. The result is a #xfile_enumerator_t object that will
 * give out #xfile_info_t objects for all the files in the directory.
 *
 * For more details, see xfile_enumerate_children() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called. You can
 * then call xfile_enumerate_children_finish() to get the result of
 * the operation.
 */
void
xfile_enumerate_children_async (xfile_t               *file,
                                 const char          *attributes,
                                 xfile_query_info_flags_t  flags,
                                 int                  io_priority,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->enumerate_children_async) (file,
                                       attributes,
                                       flags,
                                       io_priority,
                                       cancellable,
                                       callback,
                                       user_data);
}

/**
 * xfile_enumerate_children_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t
 *
 * Finishes an async enumerate children operation.
 * See xfile_enumerate_children_async().
 *
 * Returns: (transfer full): a #xfile_enumerator_t or %NULL
 *   if an error occurred.
 *   Free the returned object with xobject_unref().
 */
xfile_enumerator_t *
xfile_enumerate_children_finish (xfile_t         *file,
                                  xasync_result_t  *res,
                                  xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->enumerate_children_finish) (file, res, error);
}

/**
 * xfile_query_exists:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 *
 * Utility function to check if a particular file exists. This is
 * implemented using xfile_query_info() and as such does blocking I/O.
 *
 * Note that in many cases it is [racy to first check for file existence](https://en.wikipedia.org/wiki/Time_of_check_to_time_of_use)
 * and then execute something based on the outcome of that, because the
 * file might have been created or removed in between the operations. The
 * general approach to handling that is to not check, but just do the
 * operation and handle the errors as they come.
 *
 * As an example of race-free checking, take the case of reading a file,
 * and if it doesn't exist, creating it. There are two racy versions: read
 * it, and on error create it; and: check if it exists, if not create it.
 * These can both result in two processes creating the file (with perhaps
 * a partially written file as the result). The correct approach is to
 * always try to create the file with xfile_create() which will either
 * atomically create the file or fail with a %G_IO_ERROR_EXISTS error.
 *
 * However, in many cases an existence check is useful in a user interface,
 * for instance to make a menu item sensitive/insensitive, so that you don't
 * have to fool users that something is possible and then just show an error
 * dialog. If you do this, you should make sure to also handle the errors
 * that can happen due to races when you execute the operation.
 *
 * Returns: %TRUE if the file exists (and can be detected without error),
 *   %FALSE otherwise (or if cancelled).
 */
xboolean_t
xfile_query_exists (xfile_t        *file,
                     xcancellable_t *cancellable)
{
  xfile_info_t *info;

  g_return_val_if_fail (X_IS_FILE(file), FALSE);

  info = xfile_query_info (file, XFILE_ATTRIBUTE_STANDARD_TYPE,
                            XFILE_QUERY_INFO_NONE, cancellable, NULL);
  if (info != NULL)
    {
      xobject_unref (info);
      return TRUE;
    }

  return FALSE;
}

/**
 * xfile_query_file_type:
 * @file: input #xfile_t
 * @flags: a set of #xfile_query_info_flags_t passed to xfile_query_info()
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 *
 * Utility function to inspect the #xfile_type_t of a file. This is
 * implemented using xfile_query_info() and as such does blocking I/O.
 *
 * The primary use case of this method is to check if a file is
 * a regular file, directory, or symlink.
 *
 * Returns: The #xfile_type_t of the file and %XFILE_TYPE_UNKNOWN
 *   if the file does not exist
 *
 * Since: 2.18
 */
xfile_type_t
xfile_query_file_type (xfile_t               *file,
                        xfile_query_info_flags_t  flags,
                        xcancellable_t        *cancellable)
{
  xfile_info_t *info;
  xfile_type_t file_type;

  g_return_val_if_fail (X_IS_FILE(file), XFILE_TYPE_UNKNOWN);
  info = xfile_query_info (file, XFILE_ATTRIBUTE_STANDARD_TYPE, flags,
                            cancellable, NULL);
  if (info != NULL)
    {
      file_type = xfile_info_get_file_type (info);
      xobject_unref (info);
    }
  else
    file_type = XFILE_TYPE_UNKNOWN;

  return file_type;
}

/**
 * xfile_query_info:
 * @file: input #xfile_t
 * @attributes: an attribute query string
 * @flags: a set of #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t
 *
 * Gets the requested information about specified @file.
 * The result is a #xfile_info_t object that contains key-value
 * attributes (such as the type or size of the file).
 *
 * The @attributes value is a string that specifies the file
 * attributes that should be gathered. It is not an error if
 * it's not possible to read a particular requested attribute
 * from a file - it just won't be set. @attributes should be a
 * comma-separated list of attributes or attribute wildcards.
 * The wildcard "*" means all attributes, and a wildcard like
 * "standard::*" means all attributes in the standard namespace.
 * An example attribute query be "standard::*,owner::user".
 * The standard attributes are available as defines, like
 * %XFILE_ATTRIBUTE_STANDARD_NAME.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * For symlinks, normally the information about the target of the
 * symlink is returned, rather than information about the symlink
 * itself. However if you pass %XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS
 * in @flags the information about the symlink itself will be returned.
 * Also, for symlinks that point to non-existing files the information
 * about the symlink itself will be returned.
 *
 * If the file does not exist, the %G_IO_ERROR_NOT_FOUND error will be
 * returned. Other errors are possible too, and depend on what kind of
 * filesystem the file is on.
 *
 * Returns: (transfer full): a #xfile_info_t for the given @file, or %NULL
 *   on error. Free the returned object with xobject_unref().
 */
xfile_info_t *
xfile_query_info (xfile_t                *file,
                   const char           *attributes,
                   xfile_query_info_flags_t   flags,
                   xcancellable_t         *cancellable,
                   xerror_t              **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->query_info == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->query_info) (file, attributes, flags, cancellable, error);
}

/**
 * xfile_query_info_async:
 * @file: input #xfile_t
 * @attributes: an attribute query string
 * @flags: a set of #xfile_query_info_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call when the
 *   request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously gets the requested information about specified @file.
 * The result is a #xfile_info_t object that contains key-value attributes
 * (such as type or size for the file).
 *
 * For more details, see xfile_query_info() which is the synchronous
 * version of this call.
 *
 * When the operation is finished, @callback will be called. You can
 * then call xfile_query_info_finish() to get the result of the operation.
 */
void
xfile_query_info_async (xfile_t               *file,
                         const char          *attributes,
                         xfile_query_info_flags_t  flags,
                         int                  io_priority,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->query_info_async) (file,
                               attributes,
                               flags,
                               io_priority,
                               cancellable,
                               callback,
                               user_data);
}

/**
 * xfile_query_info_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t
 *
 * Finishes an asynchronous file info query.
 * See xfile_query_info_async().
 *
 * Returns: (transfer full): #xfile_info_t for given @file
 *   or %NULL on error. Free the returned object with
 *   xobject_unref().
 */
xfile_info_t *
xfile_query_info_finish (xfile_t         *file,
                          xasync_result_t  *res,
                          xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->query_info_finish) (file, res, error);
}

/**
 * xfile_query_filesystem_info:
 * @file: input #xfile_t
 * @attributes:  an attribute query string
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t
 *
 * Similar to xfile_query_info(), but obtains information
 * about the filesystem the @file is on, rather than the file itself.
 * For instance the amount of space available and the type of
 * the filesystem.
 *
 * The @attributes value is a string that specifies the attributes
 * that should be gathered. It is not an error if it's not possible
 * to read a particular requested attribute from a file - it just
 * won't be set. @attributes should be a comma-separated list of
 * attributes or attribute wildcards. The wildcard "*" means all
 * attributes, and a wildcard like "filesystem::*" means all attributes
 * in the filesystem namespace. The standard namespace for filesystem
 * attributes is "filesystem". Common attributes of interest are
 * %XFILE_ATTRIBUTE_FILESYSTEM_SIZE (the total size of the filesystem
 * in bytes), %XFILE_ATTRIBUTE_FILESYSTEM_FREE (number of bytes available),
 * and %XFILE_ATTRIBUTE_FILESYSTEM_TYPE (type of the filesystem).
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * If the file does not exist, the %G_IO_ERROR_NOT_FOUND error will
 * be returned. Other errors are possible too, and depend on what
 * kind of filesystem the file is on.
 *
 * Returns: (transfer full): a #xfile_info_t or %NULL if there was an error.
 *   Free the returned object with xobject_unref().
 */
xfile_info_t *
xfile_query_filesystem_info (xfile_t         *file,
                              const char    *attributes,
                              xcancellable_t  *cancellable,
                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->query_filesystem_info == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->query_filesystem_info) (file, attributes, cancellable, error);
}

/**
 * xfile_query_filesystem_info_async:
 * @file: input #xfile_t
 * @attributes: an attribute query string
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously gets the requested information about the filesystem
 * that the specified @file is on. The result is a #xfile_info_t object
 * that contains key-value attributes (such as type or size for the
 * file).
 *
 * For more details, see xfile_query_filesystem_info() which is the
 * synchronous version of this call.
 *
 * When the operation is finished, @callback will be called. You can
 * then call xfile_query_info_finish() to get the result of the
 * operation.
 */
void
xfile_query_filesystem_info_async (xfile_t               *file,
                                    const char          *attributes,
                                    int                  io_priority,
                                    xcancellable_t        *cancellable,
                                    xasync_ready_callback_t  callback,
                                    xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->query_filesystem_info_async) (file,
                                          attributes,
                                          io_priority,
                                          cancellable,
                                          callback,
                                          user_data);
}

/**
 * xfile_query_filesystem_info_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t
 *
 * Finishes an asynchronous filesystem info query.
 * See xfile_query_filesystem_info_async().
 *
 * Returns: (transfer full): #xfile_info_t for given @file
 *   or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_info_t *
xfile_query_filesystem_info_finish (xfile_t         *file,
                                     xasync_result_t  *res,
                                     xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->query_filesystem_info_finish) (file, res, error);
}

/**
 * xfile_find_enclosing_mount:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t
 *
 * Gets a #xmount_t for the #xfile_t.
 *
 * #xmount_t is returned only for user interesting locations, see
 * #xvolume_monitor_t. If the #xfile_iface_t for @file does not have a #mount,
 * @error will be set to %G_IO_ERROR_NOT_FOUND and %NULL #will be returned.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (transfer full): a #xmount_t where the @file is located
 *   or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xmount_t *
xfile_find_enclosing_mount (xfile_t         *file,
                             xcancellable_t  *cancellable,
                             xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  if (iface->find_enclosing_mount == NULL)
    {

      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                           /* Translators: This is an error message when
                            * trying to find the enclosing (user visible)
                            * mount of a file, but none exists.
                            */
                           _("Containing mount does not exist"));
      return NULL;
    }

  return (* iface->find_enclosing_mount) (file, cancellable, error);
}

/**
 * xfile_find_enclosing_mount_async:
 * @file: a #xfile_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously gets the mount for the file.
 *
 * For more details, see xfile_find_enclosing_mount() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_find_enclosing_mount_finish() to
 * get the result of the operation.
 */
void
xfile_find_enclosing_mount_async (xfile_t              *file,
                                   int                   io_priority,
                                   xcancellable_t         *cancellable,
                                   xasync_ready_callback_t   callback,
                                   xpointer_t              user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->find_enclosing_mount_async) (file,
                                         io_priority,
                                         cancellable,
                                         callback,
                                         user_data);
}

/**
 * xfile_find_enclosing_mount_finish:
 * @file: a #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t
 *
 * Finishes an asynchronous find mount request.
 * See xfile_find_enclosing_mount_async().
 *
 * Returns: (transfer full): #xmount_t for given @file or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xmount_t *
xfile_find_enclosing_mount_finish (xfile_t         *file,
                                    xasync_result_t  *res,
                                    xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->find_enclosing_mount_finish) (file, res, error);
}


/**
 * xfile_read:
 * @file: #xfile_t to read
 * @cancellable: (nullable): a #xcancellable_t
 * @error: a #xerror_t, or %NULL
 *
 * Opens a file for reading. The result is a #xfile_input_stream_t that
 * can be used to read the contents of the file.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If the file does not exist, the %G_IO_ERROR_NOT_FOUND error will be
 * returned. If the file is a directory, the %G_IO_ERROR_IS_DIRECTORY
 * error will be returned. Other errors are possible too, and depend
 * on what kind of filesystem the file is on.
 *
 * Virtual: read_fn
 * Returns: (transfer full): #xfile_input_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_input_stream_t *
xfile_read (xfile_t         *file,
             xcancellable_t  *cancellable,
             xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->read_fn == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->read_fn) (file, cancellable, error);
}

/**
 * xfile_append_to:
 * @file: input #xfile_t
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Gets an output stream for appending data to the file.
 * If the file doesn't already exist it is created.
 *
 * By default files created are generally readable by everyone,
 * but if you pass %XFILE_CREATE_PRIVATE in @flags the file
 * will be made readable only to the current user, to the level that
 * is supported on the target filesystem.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * Some file systems don't allow all file names, and may return an
 * %G_IO_ERROR_INVALID_FILENAME error. If the file is a directory the
 * %G_IO_ERROR_IS_DIRECTORY error will be returned. Other errors are
 * possible too, and depend on what kind of filesystem the file is on.
 *
 * Returns: (transfer full): a #xfile_output_stream_t, or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_output_stream_t *
xfile_append_to (xfile_t             *file,
                  xfile_create_flags_t   flags,
                  xcancellable_t      *cancellable,
                  xerror_t           **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->append_to == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->append_to) (file, flags, cancellable, error);
}

/**
 * xfile_create:
 * @file: input #xfile_t
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Creates a new file and returns an output stream for writing to it.
 * The file must not already exist.
 *
 * By default files created are generally readable by everyone,
 * but if you pass %XFILE_CREATE_PRIVATE in @flags the file
 * will be made readable only to the current user, to the level
 * that is supported on the target filesystem.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * If a file or directory with this name already exists the
 * %G_IO_ERROR_EXISTS error will be returned. Some file systems don't
 * allow all file names, and may return an %G_IO_ERROR_INVALID_FILENAME
 * error, and if the name is to long %G_IO_ERROR_FILENAME_TOO_LONG will
 * be returned. Other errors are possible too, and depend on what kind
 * of filesystem the file is on.
 *
 * Returns: (transfer full): a #xfile_output_stream_t for the newly created
 *   file, or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_output_stream_t *
xfile_create (xfile_t             *file,
               xfile_create_flags_t   flags,
               xcancellable_t      *cancellable,
               xerror_t           **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->create == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->create) (file, flags, cancellable, error);
}

/**
 * xfile_replace:
 * @file: input #xfile_t
 * @etag: (nullable): an optional [entity tag][gfile-etag]
 *   for the current #xfile_t, or #NULL to ignore
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Returns an output stream for overwriting the file, possibly
 * creating a backup copy of the file first. If the file doesn't exist,
 * it will be created.
 *
 * This will try to replace the file in the safest way possible so
 * that any errors during the writing will not affect an already
 * existing copy of the file. For instance, for local files it
 * may write to a temporary file and then atomically rename over
 * the destination when the stream is closed.
 *
 * By default files created are generally readable by everyone,
 * but if you pass %XFILE_CREATE_PRIVATE in @flags the file
 * will be made readable only to the current user, to the level that
 * is supported on the target filesystem.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * If you pass in a non-%NULL @etag value and @file already exists, then
 * this value is compared to the current entity tag of the file, and if
 * they differ an %G_IO_ERROR_WRONG_ETAG error is returned. This
 * generally means that the file has been changed since you last read
 * it. You can get the new etag from xfile_output_stream_get_etag()
 * after you've finished writing and closed the #xfile_output_stream_t. When
 * you load a new file you can use xfile_input_stream_query_info() to
 * get the etag of the file.
 *
 * If @make_backup is %TRUE, this function will attempt to make a
 * backup of the current file before overwriting it. If this fails
 * a %G_IO_ERROR_CANT_CREATE_BACKUP error will be returned. If you
 * want to replace anyway, try again with @make_backup set to %FALSE.
 *
 * If the file is a directory the %G_IO_ERROR_IS_DIRECTORY error will
 * be returned, and if the file is some other form of non-regular file
 * then a %G_IO_ERROR_NOT_REGULAR_FILE error will be returned. Some
 * file systems don't allow all file names, and may return an
 * %G_IO_ERROR_INVALID_FILENAME error, and if the name is to long
 * %G_IO_ERROR_FILENAME_TOO_LONG will be returned. Other errors are
 * possible too, and depend on what kind of filesystem the file is on.
 *
 * Returns: (transfer full): a #xfile_output_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_output_stream_t *
xfile_replace (xfile_t             *file,
                const char        *etag,
                xboolean_t           make_backup,
                xfile_create_flags_t   flags,
                xcancellable_t      *cancellable,
                xerror_t           **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->replace == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  /* Handle empty tag string as NULL in consistent way. */
  if (etag && *etag == 0)
    etag = NULL;

  return (* iface->replace) (file, etag, make_backup, flags, cancellable, error);
}

/**
 * xfile_open_readwrite:
 * @file: #xfile_t to open
 * @cancellable: (nullable): a #xcancellable_t
 * @error: a #xerror_t, or %NULL
 *
 * Opens an existing file for reading and writing. The result is
 * a #xfile_io_stream_t that can be used to read and write the contents
 * of the file.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * If the file does not exist, the %G_IO_ERROR_NOT_FOUND error will
 * be returned. If the file is a directory, the %G_IO_ERROR_IS_DIRECTORY
 * error will be returned. Other errors are possible too, and depend on
 * what kind of filesystem the file is on. Note that in many non-local
 * file cases read and write streams are not supported, so make sure you
 * really need to do read and write streaming, rather than just opening
 * for reading or writing.
 *
 * Returns: (transfer full): #xfile_io_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xfile_io_stream_t *
xfile_open_readwrite (xfile_t         *file,
                       xcancellable_t  *cancellable,
                       xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->open_readwrite == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->open_readwrite) (file, cancellable, error);
}

/**
 * xfile_create_readwrite:
 * @file: a #xfile_t
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: return location for a #xerror_t, or %NULL
 *
 * Creates a new file and returns a stream for reading and
 * writing to it. The file must not already exist.
 *
 * By default files created are generally readable by everyone,
 * but if you pass %XFILE_CREATE_PRIVATE in @flags the file
 * will be made readable only to the current user, to the level
 * that is supported on the target filesystem.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * If a file or directory with this name already exists, the
 * %G_IO_ERROR_EXISTS error will be returned. Some file systems don't
 * allow all file names, and may return an %G_IO_ERROR_INVALID_FILENAME
 * error, and if the name is too long, %G_IO_ERROR_FILENAME_TOO_LONG
 * will be returned. Other errors are possible too, and depend on what
 * kind of filesystem the file is on.
 *
 * Note that in many non-local file cases read and write streams are
 * not supported, so make sure you really need to do read and write
 * streaming, rather than just opening for reading or writing.
 *
 * Returns: (transfer full): a #xfile_io_stream_t for the newly created
 *   file, or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xfile_io_stream_t *
xfile_create_readwrite (xfile_t             *file,
                         xfile_create_flags_t   flags,
                         xcancellable_t      *cancellable,
                         xerror_t           **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->create_readwrite == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->create_readwrite) (file, flags, cancellable, error);
}

/**
 * xfile_replace_readwrite:
 * @file: a #xfile_t
 * @etag: (nullable): an optional [entity tag][gfile-etag]
 *   for the current #xfile_t, or #NULL to ignore
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: return location for a #xerror_t, or %NULL
 *
 * Returns an output stream for overwriting the file in readwrite mode,
 * possibly creating a backup copy of the file first. If the file doesn't
 * exist, it will be created.
 *
 * For details about the behaviour, see xfile_replace() which does the
 * same thing but returns an output stream only.
 *
 * Note that in many non-local file cases read and write streams are not
 * supported, so make sure you really need to do read and write streaming,
 * rather than just opening for reading or writing.
 *
 * Returns: (transfer full): a #xfile_io_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xfile_io_stream_t *
xfile_replace_readwrite (xfile_t             *file,
                          const char        *etag,
                          xboolean_t           make_backup,
                          xfile_create_flags_t   flags,
                          xcancellable_t      *cancellable,
                          xerror_t           **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->replace_readwrite == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->replace_readwrite) (file, etag, make_backup, flags, cancellable, error);
}

/**
 * xfile_read_async:
 * @file: input #xfile_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously opens @file for reading.
 *
 * For more details, see xfile_read() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_read_finish() to get the result
 * of the operation.
 */
void
xfile_read_async (xfile_t               *file,
                   int                  io_priority,
                   xcancellable_t        *cancellable,
                   xasync_ready_callback_t  callback,
                   xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->read_async) (file,
                         io_priority,
                         cancellable,
                         callback,
                         user_data);
}

/**
 * xfile_read_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file read operation started with
 * xfile_read_async().
 *
 * Returns: (transfer full): a #xfile_input_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_input_stream_t *
xfile_read_finish (xfile_t         *file,
                    xasync_result_t  *res,
                    xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->read_finish) (file, res, error);
}

/**
 * xfile_append_to_async:
 * @file: input #xfile_t
 * @flags: a set of #xfile_create_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously opens @file for appending.
 *
 * For more details, see xfile_append_to() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_append_to_finish() to get the result
 * of the operation.
 */
void
xfile_append_to_async (xfile_t               *file,
                        xfile_create_flags_t     flags,
                        int                  io_priority,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->append_to_async) (file,
                              flags,
                              io_priority,
                              cancellable,
                              callback,
                              user_data);
}

/**
 * xfile_append_to_finish:
 * @file: input #xfile_t
 * @res: #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file append operation started with
 * xfile_append_to_async().
 *
 * Returns: (transfer full): a valid #xfile_output_stream_t
 *   or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_output_stream_t *
xfile_append_to_finish (xfile_t         *file,
                         xasync_result_t  *res,
                         xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->append_to_finish) (file, res, error);
}

/**
 * xfile_create_async:
 * @file: input #xfile_t
 * @flags: a set of #xfile_create_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously creates a new file and returns an output stream
 * for writing to it. The file must not already exist.
 *
 * For more details, see xfile_create() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_create_finish() to get the result
 * of the operation.
 */
void
xfile_create_async (xfile_t               *file,
                     xfile_create_flags_t     flags,
                     int                  io_priority,
                     xcancellable_t        *cancellable,
                     xasync_ready_callback_t  callback,
                     xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->create_async) (file,
                           flags,
                           io_priority,
                           cancellable,
                           callback,
                           user_data);
}

/**
 * xfile_create_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file create operation started with
 * xfile_create_async().
 *
 * Returns: (transfer full): a #xfile_output_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_output_stream_t *
xfile_create_finish (xfile_t         *file,
                      xasync_result_t  *res,
                      xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->create_finish) (file, res, error);
}

/**
 * xfile_replace_async:
 * @file: input #xfile_t
 * @etag: (nullable): an [entity tag][gfile-etag] for the current #xfile_t,
 *   or %NULL to ignore
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously overwrites the file, replacing the contents,
 * possibly creating a backup copy of the file first.
 *
 * For more details, see xfile_replace() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_replace_finish() to get the result
 * of the operation.
 */
void
xfile_replace_async (xfile_t               *file,
                      const char          *etag,
                      xboolean_t             make_backup,
                      xfile_create_flags_t     flags,
                      int                  io_priority,
                      xcancellable_t        *cancellable,
                      xasync_ready_callback_t  callback,
                      xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->replace_async) (file,
                            etag,
                            make_backup,
                            flags,
                            io_priority,
                            cancellable,
                            callback,
                            user_data);
}

/**
 * xfile_replace_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file replace operation started with
 * xfile_replace_async().
 *
 * Returns: (transfer full): a #xfile_output_stream_t, or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_output_stream_t *
xfile_replace_finish (xfile_t         *file,
                       xasync_result_t  *res,
                       xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->replace_finish) (file, res, error);
}

/**
 * xfile_open_readwrite_async
 * @file: input #xfile_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously opens @file for reading and writing.
 *
 * For more details, see xfile_open_readwrite() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_open_readwrite_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_open_readwrite_async (xfile_t               *file,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->open_readwrite_async) (file,
                                   io_priority,
                                   cancellable,
                                   callback,
                                   user_data);
}

/**
 * xfile_open_readwrite_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file read operation started with
 * xfile_open_readwrite_async().
 *
 * Returns: (transfer full): a #xfile_io_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xfile_io_stream_t *
xfile_open_readwrite_finish (xfile_t         *file,
                              xasync_result_t  *res,
                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->open_readwrite_finish) (file, res, error);
}

/**
 * xfile_create_readwrite_async:
 * @file: input #xfile_t
 * @flags: a set of #xfile_create_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously creates a new file and returns a stream
 * for reading and writing to it. The file must not already exist.
 *
 * For more details, see xfile_create_readwrite() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_create_readwrite_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_create_readwrite_async (xfile_t               *file,
                               xfile_create_flags_t     flags,
                               int                  io_priority,
                               xcancellable_t        *cancellable,
                               xasync_ready_callback_t  callback,
                               xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->create_readwrite_async) (file,
                                     flags,
                                     io_priority,
                                     cancellable,
                                     callback,
                                     user_data);
}

/**
 * xfile_create_readwrite_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file create operation started with
 * xfile_create_readwrite_async().
 *
 * Returns: (transfer full): a #xfile_io_stream_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xfile_io_stream_t *
xfile_create_readwrite_finish (xfile_t         *file,
                                xasync_result_t  *res,
                                xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->create_readwrite_finish) (file, res, error);
}

/**
 * xfile_replace_readwrite_async:
 * @file: input #xfile_t
 * @etag: (nullable): an [entity tag][gfile-etag] for the current #xfile_t,
 *   or %NULL to ignore
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously overwrites the file in read-write mode,
 * replacing the contents, possibly creating a backup copy
 * of the file first.
 *
 * For more details, see xfile_replace_readwrite() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_replace_readwrite_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_replace_readwrite_async (xfile_t               *file,
                                const char          *etag,
                                xboolean_t             make_backup,
                                xfile_create_flags_t     flags,
                                int                  io_priority,
                                xcancellable_t        *cancellable,
                                xasync_ready_callback_t  callback,
                                xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->replace_readwrite_async) (file,
                                      etag,
                                      make_backup,
                                      flags,
                                      io_priority,
                                      cancellable,
                                      callback,
                                      user_data);
}

/**
 * xfile_replace_readwrite_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file replace operation started with
 * xfile_replace_readwrite_async().
 *
 * Returns: (transfer full): a #xfile_io_stream_t, or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xfile_io_stream_t *
xfile_replace_readwrite_finish (xfile_t         *file,
                                 xasync_result_t  *res,
                                 xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->replace_readwrite_finish) (file, res, error);
}

static xboolean_t
copy_symlink (xfile_t           *destination,
              xfile_copy_flags_t   flags,
              xcancellable_t    *cancellable,
              const char      *target,
              xerror_t         **error)
{
  xerror_t *my_error;
  xboolean_t tried_delete;
  xfile_info_t *info;
  xfile_type_t file_type;

  tried_delete = FALSE;

 retry:
  my_error = NULL;
  if (!xfile_make_symbolic_link (destination, target, cancellable, &my_error))
    {
      /* Maybe it already existed, and we want to overwrite? */
      if (!tried_delete && (flags & XFILE_COPY_OVERWRITE) &&
          my_error->domain == G_IO_ERROR && my_error->code == G_IO_ERROR_EXISTS)
        {
          g_clear_error (&my_error);

          /* Don't overwrite if the destination is a directory */
          info = xfile_query_info (destination, XFILE_ATTRIBUTE_STANDARD_TYPE,
                                    XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                    cancellable, &my_error);
          if (info != NULL)
            {
              file_type = xfile_info_get_file_type (info);
              xobject_unref (info);

              if (file_type == XFILE_TYPE_DIRECTORY)
                {
                  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
                                       _("Can’t copy over directory"));
                  return FALSE;
                }
            }

          if (!xfile_delete (destination, cancellable, error))
            return FALSE;

          tried_delete = TRUE;
          goto retry;
        }
            /* Nah, fail */
      g_propagate_error (error, my_error);
      return FALSE;
    }

  return TRUE;
}

static xfile_input_stream_t *
open_source_for_copy (xfile_t           *source,
                      xfile_t           *destination,
                      xfile_copy_flags_t   flags,
                      xcancellable_t    *cancellable,
                      xerror_t         **error)
{
  xerror_t *my_error;
  xfile_input_stream_t *ret;
  xfile_info_t *info;
  xfile_type_t file_type;

  my_error = NULL;
  ret = xfile_read (source, cancellable, &my_error);
  if (ret != NULL)
    return ret;

  /* There was an error opening the source, try to set a good error for it: */
  if (my_error->domain == G_IO_ERROR && my_error->code == G_IO_ERROR_IS_DIRECTORY)
    {
      /* The source is a directory, don't fail with WOULD_RECURSE immediately,
       * as that is less useful to the app. Better check for errors on the
       * target instead.
       */
      xerror_free (my_error);
      my_error = NULL;

      info = xfile_query_info (destination, XFILE_ATTRIBUTE_STANDARD_TYPE,
                                XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                cancellable, &my_error);
      if (info != NULL &&
          xfile_info_has_attribute (info, XFILE_ATTRIBUTE_STANDARD_TYPE))
        {
          file_type = xfile_info_get_file_type (info);
          xobject_unref (info);

          if (flags & XFILE_COPY_OVERWRITE)
            {
              if (file_type == XFILE_TYPE_DIRECTORY)
                {
                  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_WOULD_MERGE,
                                       _("Can’t copy directory over directory"));
                  return NULL;
                }
              /* continue to would_recurse error */
            }
          else
            {
              g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                                   _("Target file exists"));
              return NULL;
            }
        }
      else
        {
          /* Error getting info from target, return that error
           * (except for NOT_FOUND, which is no error here)
           */
          g_clear_object (&info);
          if (my_error != NULL && !xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
            {
              g_propagate_error (error, my_error);
              return NULL;
            }
          g_clear_error (&my_error);
        }

      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_WOULD_RECURSE,
                           _("Can’t recursively copy directory"));
      return NULL;
    }

  g_propagate_error (error, my_error);
  return NULL;
}

static xboolean_t
should_copy (xfile_attribute_info_t *info,
             xboolean_t            copy_all_attributes,
             xboolean_t            skip_perms)
{
  if (skip_perms && strcmp(info->name, "unix::mode") == 0)
        return FALSE;

  if (copy_all_attributes)
    return info->flags & XFILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED;
  return info->flags & XFILE_ATTRIBUTE_INFO_COPY_WITH_FILE;
}

/**
 * xfile_build_attribute_list_for_copy:
 * @file: a #xfile_t to copy attributes to
 * @flags: a set of #xfile_copy_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, %NULL to ignore
 *
 * Prepares the file attribute query string for copying to @file.
 *
 * This function prepares an attribute query string to be
 * passed to xfile_query_info() to get a list of attributes
 * normally copied with the file (see xfile_copy_attributes()
 * for the detailed description). This function is used by the
 * implementation of xfile_copy_attributes() and is useful
 * when one needs to query and set the attributes in two
 * stages (e.g., for recursive move of a directory).
 *
 * Returns: an attribute query string for xfile_query_info(),
 *   or %NULL if an error occurs.
 *
 * Since: 2.68
 */
char *
xfile_build_attribute_list_for_copy (xfile_t                  *file,
                                      xfile_copy_flags_t          flags,
                                      xcancellable_t           *cancellable,
                                      xerror_t                **error)
{
  char *ret = NULL;
  xfile_attribute_info_list_t *attributes = NULL, *namespaces = NULL;
  xstring_t *s = NULL;
  xboolean_t first;
  int i;
  xboolean_t copy_all_attributes;
  xboolean_t skip_perms;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  copy_all_attributes = flags & XFILE_COPY_ALL_METADATA;
  skip_perms = (flags & XFILE_COPY_TARGET_DEFAULT_PERMS) != 0;

  /* Ignore errors here, if the target supports no attributes there is
   * nothing to copy.  We still honor the cancellable though.
   */
  attributes = xfile_query_settable_attributes (file, cancellable, NULL);
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    goto out;

  namespaces = xfile_query_writable_namespaces (file, cancellable, NULL);
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    goto out;

  if (attributes == NULL && namespaces == NULL)
    goto out;

  first = TRUE;
  s = xstring_new ("");

  if (attributes)
    {
      for (i = 0; i < attributes->n_infos; i++)
        {
          if (should_copy (&attributes->infos[i], copy_all_attributes, skip_perms))
            {
              if (first)
                first = FALSE;
              else
                xstring_append_c (s, ',');

              xstring_append (s, attributes->infos[i].name);
            }
        }
    }

  if (namespaces)
    {
      for (i = 0; i < namespaces->n_infos; i++)
        {
          if (should_copy (&namespaces->infos[i], copy_all_attributes, FALSE))
            {
              if (first)
                first = FALSE;
              else
                xstring_append_c (s, ',');

              xstring_append (s, namespaces->infos[i].name);
              xstring_append (s, "::*");
            }
        }
    }

  ret = xstring_free (s, FALSE);
  s = NULL;
 out:
  if (s)
    xstring_free (s, TRUE);
  if (attributes)
    xfile_attribute_info_list_unref (attributes);
  if (namespaces)
    xfile_attribute_info_list_unref (namespaces);

  return ret;
}

/**
 * xfile_copy_attributes:
 * @source: a #xfile_t with attributes
 * @destination: a #xfile_t to copy attributes to
 * @flags: a set of #xfile_copy_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, %NULL to ignore
 *
 * Copies the file attributes from @source to @destination.
 *
 * Normally only a subset of the file attributes are copied,
 * those that are copies in a normal file copy operation
 * (which for instance does not include e.g. owner). However
 * if %XFILE_COPY_ALL_METADATA is specified in @flags, then
 * all the metadata that is possible to copy is copied. This
 * is useful when implementing move by copy + delete source.
 *
 * Returns: %TRUE if the attributes were copied successfully,
 *   %FALSE otherwise.
 */
xboolean_t
xfile_copy_attributes (xfile_t           *source,
                        xfile_t           *destination,
                        xfile_copy_flags_t   flags,
                        xcancellable_t    *cancellable,
                        xerror_t         **error)
{
  char *attrs_to_read;
  xboolean_t res;
  xfile_info_t *info;
  xboolean_t source_nofollow_symlinks;

  attrs_to_read = xfile_build_attribute_list_for_copy (destination, flags,
                                                        cancellable, error);
  if (!attrs_to_read)
    return FALSE;

  source_nofollow_symlinks = flags & XFILE_COPY_NOFOLLOW_SYMLINKS;

  /* Ignore errors here, if we can't read some info (e.g. if it doesn't exist)
   * we just don't copy it.
   */
  info = xfile_query_info (source, attrs_to_read,
                            source_nofollow_symlinks ? XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS:0,
                            cancellable,
                            NULL);

  g_free (attrs_to_read);

  res = TRUE;
  if  (info)
    {
      res = xfile_set_attributes_from_info (destination,
                                             info,
                                             XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                             cancellable,
                                             error);
      xobject_unref (info);
    }

  return res;
}

/* 256k minus malloc overhead */
#define STREAM_BUFFER_SIZE (1024*256 - 2 *sizeof(xpointer_t))

static xboolean_t
copy_stream_with_progress (xinput_stream_t           *in,
                           xoutput_stream_t          *out,
                           xfile_t                  *source,
                           xcancellable_t           *cancellable,
                           xfile_progress_callback_t   progress_callback,
                           xpointer_t                progress_callback_data,
                           xerror_t                **error)
{
  xssize_t n_read;
  xsize_t n_written;
  xoffset_t current_size;
  char *buffer;
  xboolean_t res;
  xoffset_t total_size;
  xfile_info_t *info;

  total_size = -1;
  /* avoid performance impact of querying total size when it's not needed */
  if (progress_callback)
    {
      info = xfile_input_stream_query_info (XFILE_INPUT_STREAM (in),
                                             XFILE_ATTRIBUTE_STANDARD_SIZE,
                                             cancellable, NULL);
      if (info)
        {
          if (xfile_info_has_attribute (info, XFILE_ATTRIBUTE_STANDARD_SIZE))
            total_size = xfile_info_get_size (info);
          xobject_unref (info);
        }

      if (total_size == -1)
        {
          info = xfile_query_info (source,
                                    XFILE_ATTRIBUTE_STANDARD_SIZE,
                                    XFILE_QUERY_INFO_NONE,
                                    cancellable, NULL);
          if (info)
            {
              if (xfile_info_has_attribute (info, XFILE_ATTRIBUTE_STANDARD_SIZE))
                total_size = xfile_info_get_size (info);
              xobject_unref (info);
            }
        }
    }

  if (total_size == -1)
    total_size = 0;

  buffer = g_malloc0 (STREAM_BUFFER_SIZE);
  current_size = 0;
  res = TRUE;
  while (TRUE)
    {
      n_read = xinput_stream_read (in, buffer, STREAM_BUFFER_SIZE, cancellable, error);
      if (n_read == -1)
        {
          res = FALSE;
          break;
        }

      if (n_read == 0)
        break;

      current_size += n_read;

      res = xoutput_stream_write_all (out, buffer, n_read, &n_written, cancellable, error);
      if (!res)
        break;

      if (progress_callback)
        progress_callback (current_size, total_size, progress_callback_data);
    }
  g_free (buffer);

  /* Make sure we send full copied size */
  if (progress_callback)
    progress_callback (current_size, total_size, progress_callback_data);

  return res;
}

#ifdef HAVE_SPLICE

static xboolean_t
do_splice (int     fd_in,
           loff_t *off_in,
           int     fd_out,
           loff_t *off_out,
           size_t  len,
           long   *bytes_transferd,
           xerror_t **error)
{
  long result;

retry:
  result = splice (fd_in, off_in, fd_out, off_out, len, SPLICE_F_MORE);

  if (result == -1)
    {
      int errsv = errno;

      if (errsv == EINTR)
        goto retry;
      else if (errsv == ENOSYS || errsv == EINVAL || errsv == EOPNOTSUPP)
        g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                             _("Splice not supported"));
      else
        g_set_error (error, G_IO_ERROR,
                     g_io_error_from_errno (errsv),
                     _("Error splicing file: %s"),
                     xstrerror (errsv));

      return FALSE;
    }

  *bytes_transferd = result;
  return TRUE;
}

static xboolean_t
splice_stream_with_progress (xinput_stream_t           *in,
                             xoutput_stream_t          *out,
                             xcancellable_t           *cancellable,
                             xfile_progress_callback_t   progress_callback,
                             xpointer_t                progress_callback_data,
                             xerror_t                **error)
{
  int buffer[2] = { -1, -1 };
  int buffer_size;
  xboolean_t res;
  xoffset_t total_size;
  loff_t offset_in;
  loff_t offset_out;
  int fd_in, fd_out;

  fd_in = xfile_descriptor_based_get_fd (XFILE_DESCRIPTOR_BASED (in));
  fd_out = xfile_descriptor_based_get_fd (XFILE_DESCRIPTOR_BASED (out));

  if (!g_unix_open_pipe (buffer, FD_CLOEXEC, error))
    return FALSE;

  /* Try a 1MiB buffer for improved throughput. If that fails, use the default
   * pipe size. See: https://bugzilla.gnome.org/791457 */
  buffer_size = fcntl (buffer[1], F_SETPIPE_SZ, 1024 * 1024);
  if (buffer_size <= 0)
    {
      buffer_size = fcntl (buffer[1], F_GETPIPE_SZ);
      if (buffer_size <= 0)
        {
          /* If #F_GETPIPE_SZ isn’t available, assume we’re on Linux < 2.6.35,
           * but ≥ 2.6.11, meaning the pipe capacity is 64KiB. Ignore the
           * possibility of running on Linux < 2.6.11 (where the capacity was
           * the system page size, typically 4KiB) because it’s ancient.
           * See pipe(7). */
          buffer_size = 1024 * 64;
        }
    }

  g_assert (buffer_size > 0);

  total_size = -1;
  /* avoid performance impact of querying total size when it's not needed */
  if (progress_callback)
    {
      struct stat sbuf;

      if (fstat (fd_in, &sbuf) == 0)
        total_size = sbuf.st_size;
    }

  if (total_size == -1)
    total_size = 0;

  offset_in = offset_out = 0;
  res = FALSE;
  while (TRUE)
    {
      long n_read;
      long n_written;

      if (g_cancellable_set_error_if_cancelled (cancellable, error))
        break;

      if (!do_splice (fd_in, &offset_in, buffer[1], NULL, buffer_size, &n_read, error))
        break;

      if (n_read == 0)
        {
          res = TRUE;
          break;
        }

      while (n_read > 0)
        {
          if (g_cancellable_set_error_if_cancelled (cancellable, error))
            goto out;

          if (!do_splice (buffer[0], NULL, fd_out, &offset_out, n_read, &n_written, error))
            goto out;

          n_read -= n_written;
        }

      if (progress_callback)
        progress_callback (offset_in, total_size, progress_callback_data);
    }

  /* Make sure we send full copied size */
  if (progress_callback)
    progress_callback (offset_in, total_size, progress_callback_data);

  if (!g_close (buffer[0], error))
    goto out;
  buffer[0] = -1;
  if (!g_close (buffer[1], error))
    goto out;
  buffer[1] = -1;
 out:
  if (buffer[0] != -1)
    (void) g_close (buffer[0], NULL);
  if (buffer[1] != -1)
    (void) g_close (buffer[1], NULL);

  return res;
}
#endif

#ifdef __linux__
static xboolean_t
btrfs_reflink_with_progress (xinput_stream_t           *in,
                             xoutput_stream_t          *out,
                             xfile_info_t              *info,
                             xcancellable_t           *cancellable,
                             xfile_progress_callback_t   progress_callback,
                             xpointer_t                progress_callback_data,
                             xerror_t                **error)
{
  xoffset_t source_size;
  int fd_in, fd_out;
  int ret, errsv;

  fd_in = xfile_descriptor_based_get_fd (XFILE_DESCRIPTOR_BASED (in));
  fd_out = xfile_descriptor_based_get_fd (XFILE_DESCRIPTOR_BASED (out));

  if (progress_callback)
    source_size = xfile_info_get_size (info);

  /* Btrfs clone ioctl properties:
   *  - Works at the inode level
   *  - Doesn't work with directories
   *  - Always follows symlinks (source and destination)
   *
   * By the time we get here, *in and *out are both regular files */
  ret = ioctl (fd_out, BTRFS_IOC_CLONE, fd_in);
  errsv = errno;

  if (ret < 0)
    {
      if (errsv == EXDEV)
	g_set_error_literal (error, G_IO_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     _("Copy (reflink/clone) between mounts is not supported"));
      else if (errsv == EINVAL)
	g_set_error_literal (error, G_IO_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     _("Copy (reflink/clone) is not supported or invalid"));
      else
	/* Most probably something odd happened; retry with fallback */
	g_set_error_literal (error, G_IO_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     _("Copy (reflink/clone) is not supported or didn’t work"));
      /* We retry with fallback for all error cases because Btrfs is currently
       * unstable, and so we can't trust it to do clone properly.
       * In addition, any hard errors here would cause the same failure in the
       * fallback manual copy as well. */
      return FALSE;
    }

  /* Make sure we send full copied size */
  if (progress_callback)
    progress_callback (source_size, source_size, progress_callback_data);

  return TRUE;
}
#endif

static xboolean_t
file_copy_fallback (xfile_t                  *source,
                    xfile_t                  *destination,
                    xfile_copy_flags_t          flags,
                    xcancellable_t           *cancellable,
                    xfile_progress_callback_t   progress_callback,
                    xpointer_t                progress_callback_data,
                    xerror_t                **error)
{
  xboolean_t ret = FALSE;
  xfile_input_stream_t *file_in = NULL;
  xinput_stream_t *in = NULL;
  xoutput_stream_t *out = NULL;
  xfile_info_t *info = NULL;
  const char *target;
  char *attrs_to_read;
  xboolean_t do_set_attributes = FALSE;
  xfile_create_flags_t create_flags;
  xerror_t *tmp_error = NULL;

  /* need to know the file type */
  info = xfile_query_info (source,
                            XFILE_ATTRIBUTE_STANDARD_TYPE "," XFILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                            XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            cancellable,
                            error);
  if (!info)
    goto out;

  /* Maybe copy the symlink? */
  if ((flags & XFILE_COPY_NOFOLLOW_SYMLINKS) &&
      xfile_info_get_file_type (info) == XFILE_TYPE_SYMBOLIC_LINK)
    {
      target = xfile_info_get_symlink_target (info);
      if (target)
        {
          if (!copy_symlink (destination, flags, cancellable, target, error))
            goto out;

          ret = TRUE;
          goto out;
        }
        /* ... else fall back on a regular file copy */
    }
  /* Handle "special" files (pipes, device nodes, ...)? */
  else if (xfile_info_get_file_type (info) == XFILE_TYPE_SPECIAL)
    {
      /* FIXME: could try to recreate device nodes and others? */
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Can’t copy special file"));
      goto out;
    }

  /* Everything else should just fall back on a regular copy. */

  file_in = open_source_for_copy (source, destination, flags, cancellable, error);
  if (!file_in)
    goto out;
  in = G_INPUT_STREAM (file_in);

  attrs_to_read = xfile_build_attribute_list_for_copy (destination, flags,
                                                        cancellable, error);
  if (!attrs_to_read)
    goto out;

  /* Ok, ditch the previous lightweight info (on Unix we just
   * called lstat()); at this point we gather all the information
   * we need about the source from the opened file descriptor.
   */
  xobject_unref (info);

  info = xfile_input_stream_query_info (file_in, attrs_to_read,
                                         cancellable, &tmp_error);
  if (!info)
    {
      /* Not all gvfs backends implement query_info_on_read(), we
       * can just fall back to the pathname again.
       * https://bugzilla.gnome.org/706254
       */
      if (xerror_matches (tmp_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
        {
          g_clear_error (&tmp_error);
          info = xfile_query_info (source, attrs_to_read, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                    cancellable, error);
        }
      else
        {
          g_free (attrs_to_read);
          g_propagate_error (error, tmp_error);
          goto out;
        }
    }
  g_free (attrs_to_read);
  if (!info)
    goto out;

  do_set_attributes = TRUE;

  /* In the local file path, we pass down the source info which
   * includes things like unix::mode, to ensure that the target file
   * is not created with different permissions from the source file.
   *
   * If a future API like xfile_replace_with_info() is added, switch
   * this code to use that.
   *
   * Use %XFILE_CREATE_PRIVATE unless
   *  - we were told to create the file with default permissions (i.e. the
   *    process’ umask),
   *  - or if the source file is on a file system which doesn’t support
   *    `unix::mode` (in which case it probably also makes sense to create the
   *    destination with default permissions because the source cannot be
   *    private),
   *  - or if the destination file is a `GLocalFile`, in which case we can
   *    directly open() it with the permissions from the source file.
   */
  create_flags = XFILE_CREATE_NONE;
  if (!(flags & XFILE_COPY_TARGET_DEFAULT_PERMS) &&
      xfile_info_has_attribute (info, XFILE_ATTRIBUTE_UNIX_MODE) &&
      !X_IS_LOCAL_FILE (destination))
    create_flags |= XFILE_CREATE_PRIVATE;
  if (flags & XFILE_COPY_OVERWRITE)
    create_flags |= XFILE_CREATE_REPLACE_DESTINATION;

  if (X_IS_LOCAL_FILE (destination))
    {
      if (flags & XFILE_COPY_OVERWRITE)
        out = (xoutput_stream_t*)_g_local_file_output_stream_replace (_g_local_file_get_filename (G_LOCAL_FILE (destination)),
                                                                   FALSE, NULL,
                                                                   flags & XFILE_COPY_BACKUP,
                                                                   create_flags,
                                                                   (flags & XFILE_COPY_TARGET_DEFAULT_PERMS) ? NULL : info,
                                                                   cancellable, error);
      else
        out = (xoutput_stream_t*)_g_local_file_output_stream_create (_g_local_file_get_filename (G_LOCAL_FILE (destination)),
                                                                  FALSE, create_flags,
                                                                  (flags & XFILE_COPY_TARGET_DEFAULT_PERMS) ? NULL : info,
                                                                  cancellable, error);
    }
  else if (flags & XFILE_COPY_OVERWRITE)
    {
      out = (xoutput_stream_t *)xfile_replace (destination,
                                             NULL,
                                             flags & XFILE_COPY_BACKUP,
                                             create_flags,
                                             cancellable, error);
    }
  else
    {
      out = (xoutput_stream_t *)xfile_create (destination, create_flags, cancellable, error);
    }

  if (!out)
    goto out;

#ifdef __linux__
  if (X_IS_FILE_DESCRIPTOR_BASED (in) && X_IS_FILE_DESCRIPTOR_BASED (out))
    {
      xerror_t *reflink_err = NULL;

      if (!btrfs_reflink_with_progress (in, out, info, cancellable,
                                        progress_callback, progress_callback_data,
                                        &reflink_err))
        {
          if (xerror_matches (reflink_err, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
            {
              g_clear_error (&reflink_err);
            }
          else
            {
              g_propagate_error (error, reflink_err);
              goto out;
            }
        }
      else
        {
          ret = TRUE;
          goto out;
        }
    }
#endif

#ifdef HAVE_SPLICE
  if (X_IS_FILE_DESCRIPTOR_BASED (in) && X_IS_FILE_DESCRIPTOR_BASED (out))
    {
      xerror_t *splice_err = NULL;

      if (!splice_stream_with_progress (in, out, cancellable,
                                        progress_callback, progress_callback_data,
                                        &splice_err))
        {
          if (xerror_matches (splice_err, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
            {
              g_clear_error (&splice_err);
            }
          else
            {
              g_propagate_error (error, splice_err);
              goto out;
            }
        }
      else
        {
          ret = TRUE;
          goto out;
        }
    }

#endif

  /* A plain read/write loop */
  if (!copy_stream_with_progress (in, out, source, cancellable,
                                  progress_callback, progress_callback_data,
                                  error))
    goto out;

  ret = TRUE;
 out:
  if (in)
    {
      /* Don't care about errors in source here */
      (void) xinput_stream_close (in, cancellable, NULL);
      xobject_unref (in);
    }

  if (out)
    {
      /* But write errors on close are bad! */
      if (!xoutput_stream_close (out, cancellable, ret ? error : NULL))
        ret = FALSE;
      xobject_unref (out);
    }

  /* Ignore errors here. Failure to copy metadata is not a hard error */
  /* TODO: set these attributes /before/ we do the rename() on Unix */
  if (ret && do_set_attributes)
    {
      xfile_set_attributes_from_info (destination,
                                       info,
                                       XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                       cancellable,
                                       NULL);
    }

  g_clear_object (&info);

  return ret;
}

/**
 * xfile_copy:
 * @source: input #xfile_t
 * @destination: destination #xfile_t
 * @flags: set of #xfile_copy_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @progress_callback: (nullable) (scope call): function to callback with
 *   progress information, or %NULL if progress information is not needed
 * @progress_callback_data: (closure): user data to pass to @progress_callback
 * @error: #xerror_t to set on error, or %NULL
 *
 * Copies the file @source to the location specified by @destination.
 * Can not handle recursive copies of directories.
 *
 * If the flag %XFILE_COPY_OVERWRITE is specified an already
 * existing @destination file is overwritten.
 *
 * If the flag %XFILE_COPY_NOFOLLOW_SYMLINKS is specified then symlinks
 * will be copied as symlinks, otherwise the target of the
 * @source symlink will be copied.
 *
 * If the flag %XFILE_COPY_ALL_METADATA is specified then all the metadata
 * that is possible to copy is copied, not just the default subset (which,
 * for instance, does not include the owner, see #xfile_info_t).
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @progress_callback is not %NULL, then the operation can be monitored
 * by setting this to a #xfile_progress_callback_t function.
 * @progress_callback_data will be passed to this function. It is guaranteed
 * that this callback will be called after all data has been transferred with
 * the total number of bytes copied during the operation.
 *
 * If the @source file does not exist, then the %G_IO_ERROR_NOT_FOUND error
 * is returned, independent on the status of the @destination.
 *
 * If %XFILE_COPY_OVERWRITE is not specified and the target exists, then
 * the error %G_IO_ERROR_EXISTS is returned.
 *
 * If trying to overwrite a file over a directory, the %G_IO_ERROR_IS_DIRECTORY
 * error is returned. If trying to overwrite a directory with a directory the
 * %G_IO_ERROR_WOULD_MERGE error is returned.
 *
 * If the source is a directory and the target does not exist, or
 * %XFILE_COPY_OVERWRITE is specified and the target is a file, then the
 * %G_IO_ERROR_WOULD_RECURSE error is returned.
 *
 * If you are interested in copying the #xfile_t object itself (not the on-disk
 * file), see xfile_dup().
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
xboolean_t
xfile_copy (xfile_t                  *source,
             xfile_t                  *destination,
             xfile_copy_flags_t          flags,
             xcancellable_t           *cancellable,
             xfile_progress_callback_t   progress_callback,
             xpointer_t                progress_callback_data,
             xerror_t                **error)
{
  xfile_iface_t *iface;
  xerror_t *my_error;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE (source), FALSE);
  g_return_val_if_fail (X_IS_FILE (destination), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  iface = XFILE_GET_IFACE (destination);
  if (iface->copy)
    {
      my_error = NULL;
      res = (* iface->copy) (source, destination,
                             flags, cancellable,
                             progress_callback, progress_callback_data,
                             &my_error);

      if (res)
        return TRUE;

      if (my_error->domain != G_IO_ERROR || my_error->code != G_IO_ERROR_NOT_SUPPORTED)
        {
          g_propagate_error (error, my_error);
              return FALSE;
        }
      else
        g_clear_error (&my_error);
    }

  /* If the types are different, and the destination method failed
   * also try the source method
   */
  if (G_OBJECT_TYPE (source) != G_OBJECT_TYPE (destination))
    {
      iface = XFILE_GET_IFACE (source);

      if (iface->copy)
        {
          my_error = NULL;
          res = (* iface->copy) (source, destination,
                                 flags, cancellable,
                                 progress_callback, progress_callback_data,
                                 &my_error);

          if (res)
            return TRUE;

          if (my_error->domain != G_IO_ERROR || my_error->code != G_IO_ERROR_NOT_SUPPORTED)
            {
              g_propagate_error (error, my_error);
              return FALSE;
            }
          else
            g_clear_error (&my_error);
        }
    }

  return file_copy_fallback (source, destination, flags, cancellable,
                             progress_callback, progress_callback_data,
                             error);
}

/**
 * xfile_copy_async:
 * @source: input #xfile_t
 * @destination: destination #xfile_t
 * @flags: set of #xfile_copy_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @progress_callback: (nullable) (scope notified): function to callback with progress
 *   information, or %NULL if progress information is not needed
 * @progress_callback_data: (closure progress_callback) (nullable): user data to pass to @progress_callback
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: (closure callback): the data to pass to callback function
 *
 * Copies the file @source to the location specified by @destination
 * asynchronously. For details of the behaviour, see xfile_copy().
 *
 * If @progress_callback is not %NULL, then that function that will be called
 * just like in xfile_copy(). The callback will run in the default main context
 * of the thread calling xfile_copy_async() — the same context as @callback is
 * run in.
 *
 * When the operation is finished, @callback will be called. You can then call
 * xfile_copy_finish() to get the result of the operation.
 */
void
xfile_copy_async (xfile_t                  *source,
                   xfile_t                  *destination,
                   xfile_copy_flags_t          flags,
                   int                     io_priority,
                   xcancellable_t           *cancellable,
                   xfile_progress_callback_t   progress_callback,
                   xpointer_t                progress_callback_data,
                   xasync_ready_callback_t     callback,
                   xpointer_t                user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (source));
  g_return_if_fail (X_IS_FILE (destination));

  iface = XFILE_GET_IFACE (source);
  (* iface->copy_async) (source,
                         destination,
                         flags,
                         io_priority,
                         cancellable,
                         progress_callback,
                         progress_callback_data,
                         callback,
                         user_data);
}

/**
 * xfile_copy_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes copying the file started with xfile_copy_async().
 *
 * Returns: a %TRUE on success, %FALSE on error.
 */
xboolean_t
xfile_copy_finish (xfile_t         *file,
                    xasync_result_t  *res,
                    xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), FALSE);

  if (xasync_result_legacy_propagate_error (res, error))
    return FALSE;

  iface = XFILE_GET_IFACE (file);
  return (* iface->copy_finish) (file, res, error);
}

/**
 * xfile_move:
 * @source: #xfile_t pointing to the source location
 * @destination: #xfile_t pointing to the destination location
 * @flags: set of #xfile_copy_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @progress_callback: (nullable) (scope call): #xfile_progress_callback_t
 *   function for updates
 * @progress_callback_data: (closure): xpointer_t to user data for
 *   the callback function
 * @error: #xerror_t for returning error conditions, or %NULL
 *
 * Tries to move the file or directory @source to the location specified
 * by @destination. If native move operations are supported then this is
 * used, otherwise a copy + delete fallback is used. The native
 * implementation may support moving directories (for instance on moves
 * inside the same filesystem), but the fallback code does not.
 *
 * If the flag %XFILE_COPY_OVERWRITE is specified an already
 * existing @destination file is overwritten.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @progress_callback is not %NULL, then the operation can be monitored
 * by setting this to a #xfile_progress_callback_t function.
 * @progress_callback_data will be passed to this function. It is
 * guaranteed that this callback will be called after all data has been
 * transferred with the total number of bytes copied during the operation.
 *
 * If the @source file does not exist, then the %G_IO_ERROR_NOT_FOUND
 * error is returned, independent on the status of the @destination.
 *
 * If %XFILE_COPY_OVERWRITE is not specified and the target exists,
 * then the error %G_IO_ERROR_EXISTS is returned.
 *
 * If trying to overwrite a file over a directory, the %G_IO_ERROR_IS_DIRECTORY
 * error is returned. If trying to overwrite a directory with a directory the
 * %G_IO_ERROR_WOULD_MERGE error is returned.
 *
 * If the source is a directory and the target does not exist, or
 * %XFILE_COPY_OVERWRITE is specified and the target is a file, then
 * the %G_IO_ERROR_WOULD_RECURSE error may be returned (if the native
 * move operation isn't available).
 *
 * Returns: %TRUE on successful move, %FALSE otherwise.
 */
xboolean_t
xfile_move (xfile_t                  *source,
             xfile_t                  *destination,
             xfile_copy_flags_t          flags,
             xcancellable_t           *cancellable,
             xfile_progress_callback_t   progress_callback,
             xpointer_t                progress_callback_data,
             xerror_t                **error)
{
  xfile_iface_t *iface;
  xerror_t *my_error;
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE (source), FALSE);
  g_return_val_if_fail (X_IS_FILE (destination), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  iface = XFILE_GET_IFACE (destination);
  if (iface->move)
    {
      my_error = NULL;
      res = (* iface->move) (source, destination,
                             flags, cancellable,
                             progress_callback, progress_callback_data,
                             &my_error);

      if (res)
        return TRUE;

      if (my_error->domain != G_IO_ERROR || my_error->code != G_IO_ERROR_NOT_SUPPORTED)
        {
          g_propagate_error (error, my_error);
          return FALSE;
        }
      else
        g_clear_error (&my_error);
    }

  /* If the types are different, and the destination method failed
   * also try the source method
   */
  if (G_OBJECT_TYPE (source) != G_OBJECT_TYPE (destination))
    {
      iface = XFILE_GET_IFACE (source);

      if (iface->move)
        {
          my_error = NULL;
          res = (* iface->move) (source, destination,
                                 flags, cancellable,
                                 progress_callback, progress_callback_data,
                                 &my_error);

          if (res)
            return TRUE;

          if (my_error->domain != G_IO_ERROR || my_error->code != G_IO_ERROR_NOT_SUPPORTED)
            {
              g_propagate_error (error, my_error);
              return FALSE;
            }
          else
            g_clear_error (&my_error);
        }
    }

  if (flags & XFILE_COPY_NO_FALLBACK_FOR_MOVE)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return FALSE;
    }

  flags |= XFILE_COPY_ALL_METADATA | XFILE_COPY_NOFOLLOW_SYMLINKS;
  if (!xfile_copy (source, destination, flags, cancellable,
                    progress_callback, progress_callback_data,
                    error))
    return FALSE;

  return xfile_delete (source, cancellable, error);
}

/**
 * xfile_move_async:
 * @source: #xfile_t pointing to the source location
 * @destination: #xfile_t pointing to the destination location
 * @flags: set of #xfile_copy_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @progress_callback: (nullable) (scope call): #xfile_progress_callback_t
 *   function for updates
 * @progress_callback_data: (closure): xpointer_t to user data for
 *   the callback function
 * @callback: a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously moves a file @source to the location of @destination. For details of the behaviour, see xfile_move().
 *
 * If @progress_callback is not %NULL, then that function that will be called
 * just like in xfile_move(). The callback will run in the default main context
 * of the thread calling xfile_move_async() — the same context as @callback is
 * run in.
 *
 * When the operation is finished, @callback will be called. You can then call
 * xfile_move_finish() to get the result of the operation.
 *
 * Since: 2.72
 */
void
xfile_move_async (xfile_t                *source,
                   xfile_t                *destination,
                   xfile_copy_flags_t        flags,
                   int                   io_priority,
                   xcancellable_t         *cancellable,
                   xfile_progress_callback_t progress_callback,
                   xpointer_t              progress_callback_data,
                   xasync_ready_callback_t   callback,
                   xpointer_t              user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (source));
  g_return_if_fail (X_IS_FILE (destination));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  iface = XFILE_GET_IFACE (source);
  (* iface->move_async) (source,
                         destination,
                         flags,
                         io_priority,
                         cancellable,
                         progress_callback,
                         progress_callback_data,
                         callback,
                         user_data);
}

/**
 * xfile_move_finish:
 * @file: input source #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file movement, started with
 * xfile_move_async().
 *
 * Returns: %TRUE on successful file move, %FALSE otherwise.
 *
 * Since: 2.72
 */
xboolean_t
xfile_move_finish (xfile_t         *file,
                    xasync_result_t  *result,
                    xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  iface = XFILE_GET_IFACE (file);
  return (* iface->move_finish) (file, result, error);
}

/**
 * xfile_make_directory:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Creates a directory. Note that this will only create a child directory
 * of the immediate parent directory of the path or URI given by the #xfile_t.
 * To recursively create directories, see xfile_make_directory_with_parents().
 * This function will fail if the parent directory does not exist, setting
 * @error to %G_IO_ERROR_NOT_FOUND. If the file system doesn't support
 * creating directories, this function will fail, setting @error to
 * %G_IO_ERROR_NOT_SUPPORTED.
 *
 * For a local #xfile_t the newly created directory will have the default
 * (current) ownership and permissions of the current process.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE on successful creation, %FALSE otherwise.
 */
xboolean_t
xfile_make_directory (xfile_t         *file,
                       xcancellable_t  *cancellable,
                       xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  iface = XFILE_GET_IFACE (file);

  if (iface->make_directory == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return FALSE;
    }

  return (* iface->make_directory) (file, cancellable, error);
}

/**
 * xfile_make_directory_async:
 * @file: input #xfile_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously creates a directory.
 *
 * Virtual: make_directory_async
 * Since: 2.38
 */
void
xfile_make_directory_async (xfile_t               *file,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->make_directory_async) (file,
                                   io_priority,
                                   cancellable,
                                   callback,
                                   user_data);
}

/**
 * xfile_make_directory_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous directory creation, started with
 * xfile_make_directory_async().
 *
 * Virtual: make_directory_finish
 * Returns: %TRUE on successful directory creation, %FALSE otherwise.
 * Since: 2.38
 */
xboolean_t
xfile_make_directory_finish (xfile_t         *file,
                              xasync_result_t  *result,
                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  iface = XFILE_GET_IFACE (file);
  return (* iface->make_directory_finish) (file, result, error);
}

/**
 * xfile_make_directory_with_parents:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Creates a directory and any parent directories that may not
 * exist similar to 'mkdir -p'. If the file system does not support
 * creating directories, this function will fail, setting @error to
 * %G_IO_ERROR_NOT_SUPPORTED. If the directory itself already exists,
 * this function will fail setting @error to %G_IO_ERROR_EXISTS, unlike
 * the similar g_mkdir_with_parents().
 *
 * For a local #xfile_t the newly created directories will have the default
 * (current) ownership and permissions of the current process.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if all directories have been successfully created, %FALSE
 * otherwise.
 *
 * Since: 2.18
 */
xboolean_t
xfile_make_directory_with_parents (xfile_t         *file,
                                    xcancellable_t  *cancellable,
                                    xerror_t       **error)
{
  xfile_t *work_file = NULL;
  xlist_t *list = NULL, *l;
  xerror_t *my_error = NULL;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  /* Try for the simple case of not having to create any parent
   * directories.  If any parent directory needs to be created, this
   * call will fail with NOT_FOUND. If that happens, then that value of
   * my_error persists into the while loop below.
   */
  xfile_make_directory (file, cancellable, &my_error);
  if (!xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
    {
      if (my_error)
        g_propagate_error (error, my_error);
      return my_error == NULL;
    }

  work_file = xobject_ref (file);

  /* Creates the parent directories as needed. In case any particular
   * creation operation fails for lack of other parent directories
   * (NOT_FOUND), the directory is added to a list of directories to
   * create later, and the value of my_error is retained until the next
   * iteration of the loop.  After the loop my_error should either be
   * empty or contain a real failure condition.
   */
  while (xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
    {
      xfile_t *parent_file;

      parent_file = xfile_get_parent (work_file);
      if (parent_file == NULL)
        break;

      g_clear_error (&my_error);
      xfile_make_directory (parent_file, cancellable, &my_error);
      /* Another process may have created the directory in between the
       * G_IO_ERROR_NOT_FOUND and now
       */
      if (xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
        g_clear_error (&my_error);

      xobject_unref (work_file);
      work_file = xobject_ref (parent_file);

      if (xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        list = xlist_prepend (list, parent_file);  /* Transfer ownership of ref */
      else
        xobject_unref (parent_file);
    }

  /* All directories should be able to be created now, so an error at
   * this point means the whole operation must fail -- except an EXISTS
   * error, which means that another process already created the
   * directory in between the previous failure and now.
   */
  for (l = list; my_error == NULL && l; l = l->next)
    {
      xfile_make_directory ((xfile_t *) l->data, cancellable, &my_error);
      if (xerror_matches (my_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
        g_clear_error (&my_error);
    }

  if (work_file)
    xobject_unref (work_file);

  /* Clean up */
  while (list != NULL)
    {
      xobject_unref ((xfile_t *) list->data);
      list = xlist_remove (list, list->data);
    }

  /* At this point an error in my_error means a that something
   * unexpected failed in either of the loops above, so the whole
   * operation must fail.
   */
  if (my_error != NULL)
    {
      g_propagate_error (error, my_error);
      return FALSE;
    }

  return xfile_make_directory (file, cancellable, error);
}

/**
 * xfile_make_symbolic_link:
 * @file: a #xfile_t with the name of the symlink to create
 * @symlink_value: (type filename): a string with the path for the target
 *   of the new symlink
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t
 *
 * Creates a symbolic link named @file which contains the string
 * @symlink_value.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE on the creation of a new symlink, %FALSE otherwise.
 */
xboolean_t
xfile_make_symbolic_link (xfile_t         *file,
                           const char    *symlink_value,
                           xcancellable_t  *cancellable,
                           xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (symlink_value != NULL, FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  if (*symlink_value == '\0')
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Invalid symlink value given"));
      return FALSE;
    }

  iface = XFILE_GET_IFACE (file);

  if (iface->make_symbolic_link == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Symbolic links not supported"));
      return FALSE;
    }

  return (* iface->make_symbolic_link) (file, symlink_value, cancellable, error);
}

/**
 * xfile_delete:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Deletes a file. If the @file is a directory, it will only be
 * deleted if it is empty. This has the same semantics as g_unlink().
 *
 * If @file doesn’t exist, %G_IO_ERROR_NOT_FOUND will be returned. This allows
 * for deletion to be implemented avoiding
 * [time-of-check to time-of-use races](https://en.wikipedia.org/wiki/Time-of-check_to_time-of-use):
 * |[
 * x_autoptr(xerror) local_error = NULL;
 * if (!xfile_delete (my_file, my_cancellable, &local_error) &&
 *     !xerror_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
 *   {
 *     // deletion failed for some reason other than the file not existing:
 *     // so report the error
 *     g_warning ("Failed to delete %s: %s",
 *                xfile_peek_path (my_file), local_error->message);
 *   }
 * ]|
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Virtual: delete_file
 * Returns: %TRUE if the file was deleted. %FALSE otherwise.
 */
xboolean_t
xfile_delete (xfile_t         *file,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  iface = XFILE_GET_IFACE (file);

  if (iface->delete_file == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return FALSE;
    }

  return (* iface->delete_file) (file, cancellable, error);
}

/**
 * xfile_delete_async:
 * @file: input #xfile_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously delete a file. If the @file is a directory, it will
 * only be deleted if it is empty.  This has the same semantics as
 * g_unlink().
 *
 * Virtual: delete_file_async
 * Since: 2.34
 */
void
xfile_delete_async (xfile_t               *file,
                     int                  io_priority,
                     xcancellable_t        *cancellable,
                     xasync_ready_callback_t  callback,
                     xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->delete_file_async) (file,
                                io_priority,
                                cancellable,
                                callback,
                                user_data);
}

/**
 * xfile_delete_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes deleting a file started with xfile_delete_async().
 *
 * Virtual: delete_file_finish
 * Returns: %TRUE if the file was deleted. %FALSE otherwise.
 * Since: 2.34
 **/
xboolean_t
xfile_delete_finish (xfile_t         *file,
                      xasync_result_t  *result,
                      xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;

  iface = XFILE_GET_IFACE (file);
  return (* iface->delete_file_finish) (file, result, error);
}

/**
 * xfile_trash:
 * @file: #xfile_t to send to trash
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sends @file to the "Trashcan", if possible. This is similar to
 * deleting it, but the user can recover it before emptying the trashcan.
 * Not all file systems support trashing, so this call can return the
 * %G_IO_ERROR_NOT_SUPPORTED error. Since GLib 2.66, the `x-gvfs-notrash` unix
 * mount option can be used to disable xfile_trash() support for certain
 * mounts, the %G_IO_ERROR_NOT_SUPPORTED error will be returned in that case.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Virtual: trash
 * Returns: %TRUE on successful trash, %FALSE otherwise.
 */
xboolean_t
xfile_trash (xfile_t         *file,
              xcancellable_t  *cancellable,
              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  iface = XFILE_GET_IFACE (file);

  if (iface->trash == NULL)
    {
      g_set_error_literal (error,
                           G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Trash not supported"));
      return FALSE;
    }

  return (* iface->trash) (file, cancellable, error);
}

/**
 * xfile_trash_async:
 * @file: input #xfile_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously sends @file to the Trash location, if possible.
 *
 * Virtual: trash_async
 * Since: 2.38
 */
void
xfile_trash_async (xfile_t               *file,
                    int                  io_priority,
                    xcancellable_t        *cancellable,
                    xasync_ready_callback_t  callback,
                    xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);
  (* iface->trash_async) (file,
                          io_priority,
                          cancellable,
                          callback,
                          user_data);
}

/**
 * xfile_trash_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous file trashing operation, started with
 * xfile_trash_async().
 *
 * Virtual: trash_finish
 * Returns: %TRUE on successful trash, %FALSE otherwise.
 * Since: 2.38
 */
xboolean_t
xfile_trash_finish (xfile_t         *file,
                     xasync_result_t  *result,
                     xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  iface = XFILE_GET_IFACE (file);
  return (* iface->trash_finish) (file, result, error);
}

/**
 * xfile_set_display_name:
 * @file: input #xfile_t
 * @display_name: a string
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Renames @file to the specified display name.
 *
 * The display name is converted from UTF-8 to the correct encoding
 * for the target filesystem if possible and the @file is renamed to this.
 *
 * If you want to implement a rename operation in the user interface the
 * edit name (%XFILE_ATTRIBUTE_STANDARD_EDIT_NAME) should be used as the
 * initial value in the rename widget, and then the result after editing
 * should be passed to xfile_set_display_name().
 *
 * On success the resulting converted filename is returned.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (transfer full): a #xfile_t specifying what @file was renamed to,
 *   or %NULL if there was an error.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_set_display_name (xfile_t         *file,
                         const xchar_t   *display_name,
                         xcancellable_t  *cancellable,
                         xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (display_name != NULL, NULL);

  if (strchr (display_name, G_DIR_SEPARATOR) != NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("File names cannot contain “%c”"), G_DIR_SEPARATOR);
      return NULL;
    }

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  return (* iface->set_display_name) (file, display_name, cancellable, error);
}

/**
 * xfile_set_display_name_async:
 * @file: input #xfile_t
 * @display_name: a string
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously sets the display name for a given #xfile_t.
 *
 * For more details, see xfile_set_display_name() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_set_display_name_finish() to get
 * the result of the operation.
 */
void
xfile_set_display_name_async (xfile_t               *file,
                               const xchar_t         *display_name,
                               xint_t                 io_priority,
                               xcancellable_t        *cancellable,
                               xasync_ready_callback_t  callback,
                               xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));
  g_return_if_fail (display_name != NULL);

  iface = XFILE_GET_IFACE (file);
  (* iface->set_display_name_async) (file,
                                     display_name,
                                     io_priority,
                                     cancellable,
                                     callback,
                                     user_data);
}

/**
 * xfile_set_display_name_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes setting a display name started with
 * xfile_set_display_name_async().
 *
 * Returns: (transfer full): a #xfile_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_set_display_name_finish (xfile_t         *file,
                                xasync_result_t  *res,
                                xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);
  return (* iface->set_display_name_finish) (file, res, error);
}

/**
 * xfile_query_settable_attributes:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Obtain the list of settable attributes for the file.
 *
 * Returns the type and full attribute name of all the attributes
 * that can be set on this file. This doesn't mean setting it will
 * always succeed though, you might get an access failure, or some
 * specific file may not support a specific attribute.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (transfer full): a #xfile_attribute_info_list_t describing the settable attributes.
 *   When you are done with it, release it with
 *   xfile_attribute_info_list_unref()
 */
xfile_attribute_info_list_t *
xfile_query_settable_attributes (xfile_t         *file,
                                  xcancellable_t  *cancellable,
                                  xerror_t       **error)
{
  xfile_iface_t *iface;
  xerror_t *my_error;
  xfile_attribute_info_list_t *list;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->query_settable_attributes == NULL)
    return xfile_attribute_info_list_new ();

  my_error = NULL;
  list = (* iface->query_settable_attributes) (file, cancellable, &my_error);

  if (list == NULL)
    {
      if (my_error->domain == G_IO_ERROR && my_error->code == G_IO_ERROR_NOT_SUPPORTED)
        {
          list = xfile_attribute_info_list_new ();
          xerror_free (my_error);
        }
      else
        g_propagate_error (error, my_error);
    }

  return list;
}

/**
 * xfile_query_writable_namespaces:
 * @file: input #xfile_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Obtain the list of attribute namespaces where new attributes
 * can be created by a user. An example of this is extended
 * attributes (in the "xattr" namespace).
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (transfer full): a #xfile_attribute_info_list_t describing the writable namespaces.
 *   When you are done with it, release it with
 *   xfile_attribute_info_list_unref()
 */
xfile_attribute_info_list_t *
xfile_query_writable_namespaces (xfile_t         *file,
                                  xcancellable_t  *cancellable,
                                  xerror_t       **error)
{
  xfile_iface_t *iface;
  xerror_t *my_error;
  xfile_attribute_info_list_t *list;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->query_writable_namespaces == NULL)
    return xfile_attribute_info_list_new ();

  my_error = NULL;
  list = (* iface->query_writable_namespaces) (file, cancellable, &my_error);

  if (list == NULL)
    {
      g_warn_if_reached();
      list = xfile_attribute_info_list_new ();
    }

  if (my_error != NULL)
    {
      if (my_error->domain == G_IO_ERROR && my_error->code == G_IO_ERROR_NOT_SUPPORTED)
        {
          xerror_free (my_error);
        }
      else
        g_propagate_error (error, my_error);
    }

  return list;
}

/**
 * xfile_set_attribute:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @type: The type of the attribute
 * @value_p: (nullable): a pointer to the value (or the pointer
 *   itself if the type is a pointer type)
 * @flags: a set of #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets an attribute in the file with attribute name @attribute to @value_p.
 *
 * Some attributes can be unset by setting @type to
 * %XFILE_ATTRIBUTE_TYPE_INVALID and @value_p to %NULL.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the attribute was set, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute (xfile_t                *file,
                      const xchar_t          *attribute,
                      xfile_attribute_type_t    type,
                      xpointer_t              value_p,
                      xfile_query_info_flags_t   flags,
                      xcancellable_t         *cancellable,
                      xerror_t              **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  iface = XFILE_GET_IFACE (file);

  if (iface->set_attribute == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return FALSE;
    }

  return (* iface->set_attribute) (file, attribute, type, value_p, flags, cancellable, error);
}

/**
 * xfile_set_attributes_from_info:
 * @file: input #xfile_t
 * @info: a #xfile_info_t
 * @flags: #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Tries to set all attributes in the #xfile_info_t on the target
 * values, not stopping on the first error.
 *
 * If there is any error during this operation then @error will
 * be set to the first error. Error on particular fields are flagged
 * by setting the "status" field in the attribute value to
 * %XFILE_ATTRIBUTE_STATUS_ERROR_SETTING, which means you can
 * also detect further errors.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %FALSE if there was any error, %TRUE otherwise.
 */
xboolean_t
xfile_set_attributes_from_info (xfile_t                *file,
                                 xfile_info_t            *info,
                                 xfile_query_info_flags_t   flags,
                                 xcancellable_t         *cancellable,
                                 xerror_t              **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_FILE_INFO (info), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  xfile_info_clear_status (info);

  iface = XFILE_GET_IFACE (file);

  return (* iface->set_attributes_from_info) (file,
                                              info,
                                              flags,
                                              cancellable,
                                              error);
}

static xboolean_t
xfile_real_set_attributes_from_info (xfile_t                *file,
                                      xfile_info_t            *info,
                                      xfile_query_info_flags_t   flags,
                                      xcancellable_t         *cancellable,
                                      xerror_t              **error)
{
  char **attributes;
  int i;
  xboolean_t res;
  GFileAttributeValue *value;

  res = TRUE;

  attributes = xfile_info_list_attributes (info, NULL);

  for (i = 0; attributes[i] != NULL; i++)
    {
      value = _xfile_info_get_attribute_value (info, attributes[i]);

      if (value->status != XFILE_ATTRIBUTE_STATUS_UNSET)
        continue;

      if (!xfile_set_attribute (file, attributes[i],
                                 value->type, _xfile_attribute_value_peek_as_pointer (value),
                                 flags, cancellable, error))
        {
          value->status = XFILE_ATTRIBUTE_STATUS_ERROR_SETTING;
          res = FALSE;
          /* Don't set error multiple times */
          error = NULL;
        }
      else
        value->status = XFILE_ATTRIBUTE_STATUS_SET;
    }

  xstrfreev (attributes);

  return res;
}

/**
 * xfile_set_attributes_async:
 * @file: input #xfile_t
 * @info: a #xfile_info_t
 * @flags: a #xfile_query_info_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): a #xpointer_t
 *
 * Asynchronously sets the attributes of @file with @info.
 *
 * For more details, see xfile_set_attributes_from_info(),
 * which is the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_set_attributes_finish() to get
 * the result of the operation.
 */
void
xfile_set_attributes_async (xfile_t               *file,
                             xfile_info_t           *info,
                             xfile_query_info_flags_t  flags,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));
  g_return_if_fail (X_IS_FILE_INFO (info));

  iface = XFILE_GET_IFACE (file);
  (* iface->set_attributes_async) (file,
                                   info,
                                   flags,
                                   io_priority,
                                   cancellable,
                                   callback,
                                   user_data);
}

/**
 * xfile_set_attributes_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @info: (out) (transfer full): a #xfile_info_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes setting an attribute started in xfile_set_attributes_async().
 *
 * Returns: %TRUE if the attributes were set correctly, %FALSE otherwise.
 */
xboolean_t
xfile_set_attributes_finish (xfile_t         *file,
                              xasync_result_t  *result,
                              xfile_info_t    **info,
                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  /* No standard handling of errors here, as we must set info even
   * on errors
   */
  iface = XFILE_GET_IFACE (file);
  return (* iface->set_attributes_finish) (file, result, info, error);
}

/**
 * xfile_set_attribute_string:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @value: a string containing the attribute's value
 * @flags: #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets @attribute of type %XFILE_ATTRIBUTE_TYPE_STRING to @value.
 * If @attribute is of a different type, this operation will fail.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @attribute was successfully set, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute_string (xfile_t                *file,
                             const char           *attribute,
                             const char           *value,
                             xfile_query_info_flags_t   flags,
                             xcancellable_t         *cancellable,
                             xerror_t              **error)
{
  return xfile_set_attribute (file, attribute,
                               XFILE_ATTRIBUTE_TYPE_STRING, (xpointer_t)value,
                               flags, cancellable, error);
}

/**
 * xfile_set_attribute_byte_string:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @value: a string containing the attribute's new value
 * @flags: a #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets @attribute of type %XFILE_ATTRIBUTE_TYPE_BYTE_STRING to @value.
 * If @attribute is of a different type, this operation will fail,
 * returning %FALSE.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @attribute was successfully set to @value
 *   in the @file, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute_byte_string  (xfile_t                *file,
                                   const xchar_t          *attribute,
                                   const xchar_t          *value,
                                   xfile_query_info_flags_t   flags,
                                   xcancellable_t         *cancellable,
                                   xerror_t              **error)
{
  return xfile_set_attribute (file, attribute,
                               XFILE_ATTRIBUTE_TYPE_BYTE_STRING, (xpointer_t)value,
                               flags, cancellable, error);
}

/**
 * xfile_set_attribute_uint32:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @value: a #xuint32_t containing the attribute's new value
 * @flags: a #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets @attribute of type %XFILE_ATTRIBUTE_TYPE_UINT32 to @value.
 * If @attribute is of a different type, this operation will fail.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @attribute was successfully set to @value
 *   in the @file, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute_uint32 (xfile_t                *file,
                             const xchar_t          *attribute,
                             xuint32_t               value,
                             xfile_query_info_flags_t   flags,
                             xcancellable_t         *cancellable,
                             xerror_t              **error)
{
  return xfile_set_attribute (file, attribute,
                               XFILE_ATTRIBUTE_TYPE_UINT32, &value,
                               flags, cancellable, error);
}

/**
 * xfile_set_attribute_int32:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @value: a #gint32 containing the attribute's new value
 * @flags: a #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets @attribute of type %XFILE_ATTRIBUTE_TYPE_INT32 to @value.
 * If @attribute is of a different type, this operation will fail.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @attribute was successfully set to @value
 *   in the @file, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute_int32 (xfile_t                *file,
                            const xchar_t          *attribute,
                            gint32                value,
                            xfile_query_info_flags_t   flags,
                            xcancellable_t         *cancellable,
                            xerror_t              **error)
{
  return xfile_set_attribute (file, attribute,
                               XFILE_ATTRIBUTE_TYPE_INT32, &value,
                               flags, cancellable, error);
}

/**
 * xfile_set_attribute_uint64:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @value: a #xuint64_t containing the attribute's new value
 * @flags: a #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets @attribute of type %XFILE_ATTRIBUTE_TYPE_UINT64 to @value.
 * If @attribute is of a different type, this operation will fail.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @attribute was successfully set to @value
 *   in the @file, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute_uint64 (xfile_t                *file,
                             const xchar_t          *attribute,
                             xuint64_t               value,
                             xfile_query_info_flags_t   flags,
                             xcancellable_t         *cancellable,
                             xerror_t              **error)
 {
  return xfile_set_attribute (file, attribute,
                               XFILE_ATTRIBUTE_TYPE_UINT64, &value,
                               flags, cancellable, error);
}

/**
 * xfile_set_attribute_int64:
 * @file: input #xfile_t
 * @attribute: a string containing the attribute's name
 * @value: a #xuint64_t containing the attribute's new value
 * @flags: a #xfile_query_info_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Sets @attribute of type %XFILE_ATTRIBUTE_TYPE_INT64 to @value.
 * If @attribute is of a different type, this operation will fail.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @attribute was successfully set, %FALSE otherwise.
 */
xboolean_t
xfile_set_attribute_int64 (xfile_t                *file,
                            const xchar_t          *attribute,
                            gint64                value,
                            xfile_query_info_flags_t   flags,
                            xcancellable_t         *cancellable,
                            xerror_t              **error)
{
  return xfile_set_attribute (file, attribute,
                               XFILE_ATTRIBUTE_TYPE_INT64, &value,
                               flags, cancellable, error);
}

/**
 * xfile_mount_mountable:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t,
 *   or %NULL to avoid user interaction
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async) (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: (closure): the data to pass to callback function
 *
 * Mounts a file of type XFILE_TYPE_MOUNTABLE.
 * Using @mount_operation, you can request callbacks when, for instance,
 * passwords are needed during authentication.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_mount_mountable_finish() to get
 * the result of the operation.
 */
void
xfile_mount_mountable (xfile_t               *file,
                        GMountMountFlags     flags,
                        xmount_operation_t     *mount_operation,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->mount_mountable == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_mount_mountable,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  (* iface->mount_mountable) (file,
                              flags,
                              mount_operation,
                              cancellable,
                              callback,
                              user_data);
}

/**
 * xfile_mount_mountable_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes a mount operation. See xfile_mount_mountable() for details.
 *
 * Finish an asynchronous mount operation that was started
 * with xfile_mount_mountable().
 *
 * Returns: (transfer full): a #xfile_t or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_mount_mountable_finish (xfile_t         *file,
                               xasync_result_t  *result,
                               xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, xfile_mount_mountable))
    return xtask_propagate_pointer (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  return (* iface->mount_mountable_finish) (file, result, error);
}

/**
 * xfile_unmount_mountable:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async) (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: (closure): the data to pass to callback function
 *
 * Unmounts a file of type XFILE_TYPE_MOUNTABLE.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_unmount_mountable_finish() to get
 * the result of the operation.
 *
 * Deprecated: 2.22: Use xfile_unmount_mountable_with_operation() instead.
 */
void
xfile_unmount_mountable (xfile_t               *file,
                          xmount_unmount_flags_t   flags,
                          xcancellable_t        *cancellable,
                          xasync_ready_callback_t  callback,
                          xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->unmount_mountable == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_unmount_mountable_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  (* iface->unmount_mountable) (file,
                                flags,
                                cancellable,
                                callback,
                                user_data);
}

/**
 * xfile_unmount_mountable_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an unmount operation, see xfile_unmount_mountable() for details.
 *
 * Finish an asynchronous unmount operation that was started
 * with xfile_unmount_mountable().
 *
 * Returns: %TRUE if the operation finished successfully.
 *   %FALSE otherwise.
 *
 * Deprecated: 2.22: Use xfile_unmount_mountable_with_operation_finish()
 *   instead.
 */
xboolean_t
xfile_unmount_mountable_finish (xfile_t         *file,
                                 xasync_result_t  *result,
                                 xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_unmount_mountable_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  return (* iface->unmount_mountable_finish) (file, result, error);
}

/**
 * xfile_unmount_mountable_with_operation:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t,
 *   or %NULL to avoid user interaction
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async) (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: (closure): the data to pass to callback function
 *
 * Unmounts a file of type %XFILE_TYPE_MOUNTABLE.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_unmount_mountable_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_unmount_mountable_with_operation (xfile_t               *file,
                                         xmount_unmount_flags_t   flags,
                                         xmount_operation_t     *mount_operation,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->unmount_mountable == NULL && iface->unmount_mountable_with_operation == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_unmount_mountable_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  if (iface->unmount_mountable_with_operation != NULL)
    (* iface->unmount_mountable_with_operation) (file,
                                                 flags,
                                                 mount_operation,
                                                 cancellable,
                                                 callback,
                                                 user_data);
  else
    (* iface->unmount_mountable) (file,
                                  flags,
                                  cancellable,
                                  callback,
                                  user_data);
}

/**
 * xfile_unmount_mountable_with_operation_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an unmount operation,
 * see xfile_unmount_mountable_with_operation() for details.
 *
 * Finish an asynchronous unmount operation that was started
 * with xfile_unmount_mountable_with_operation().
 *
 * Returns: %TRUE if the operation finished successfully.
 *   %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xfile_unmount_mountable_with_operation_finish (xfile_t         *file,
                                                xasync_result_t  *result,
                                                xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_unmount_mountable_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  if (iface->unmount_mountable_with_operation_finish != NULL)
    return (* iface->unmount_mountable_with_operation_finish) (file, result, error);
  else
    return (* iface->unmount_mountable_finish) (file, result, error);
}

/**
 * xfile_eject_mountable:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async) (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: (closure): the data to pass to callback function
 *
 * Starts an asynchronous eject on a mountable.
 * When this operation has completed, @callback will be called with
 * @user_user data, and the operation can be finalized with
 * xfile_eject_mountable_finish().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Deprecated: 2.22: Use xfile_eject_mountable_with_operation() instead.
 */
void
xfile_eject_mountable (xfile_t               *file,
                        xmount_unmount_flags_t   flags,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->eject_mountable == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_eject_mountable_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  (* iface->eject_mountable) (file,
                              flags,
                              cancellable,
                              callback,
                              user_data);
}

/**
 * xfile_eject_mountable_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous eject operation started by
 * xfile_eject_mountable().
 *
 * Returns: %TRUE if the @file was ejected successfully.
 *   %FALSE otherwise.
 *
 * Deprecated: 2.22: Use xfile_eject_mountable_with_operation_finish()
 *   instead.
 */
xboolean_t
xfile_eject_mountable_finish (xfile_t         *file,
                               xasync_result_t  *result,
                               xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_eject_mountable_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  return (* iface->eject_mountable_finish) (file, result, error);
}

/**
 * xfile_eject_mountable_with_operation:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t,
 *   or %NULL to avoid user interaction
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (scope async) (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: (closure): the data to pass to callback function
 *
 * Starts an asynchronous eject on a mountable.
 * When this operation has completed, @callback will be called with
 * @user_user data, and the operation can be finalized with
 * xfile_eject_mountable_with_operation_finish().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Since: 2.22
 */
void
xfile_eject_mountable_with_operation (xfile_t               *file,
                                       xmount_unmount_flags_t   flags,
                                       xmount_operation_t     *mount_operation,
                                       xcancellable_t        *cancellable,
                                       xasync_ready_callback_t  callback,
                                       xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->eject_mountable == NULL && iface->eject_mountable_with_operation == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_eject_mountable_with_operation,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  if (iface->eject_mountable_with_operation != NULL)
    (* iface->eject_mountable_with_operation) (file,
                                               flags,
                                               mount_operation,
                                               cancellable,
                                               callback,
                                               user_data);
  else
    (* iface->eject_mountable) (file,
                                flags,
                                cancellable,
                                callback,
                                user_data);
}

/**
 * xfile_eject_mountable_with_operation_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous eject operation started by
 * xfile_eject_mountable_with_operation().
 *
 * Returns: %TRUE if the @file was ejected successfully.
 *   %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xfile_eject_mountable_with_operation_finish (xfile_t         *file,
                                              xasync_result_t  *result,
                                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_eject_mountable_with_operation))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  if (iface->eject_mountable_with_operation_finish != NULL)
    return (* iface->eject_mountable_with_operation_finish) (file, result, error);
  else
    return (* iface->eject_mountable_finish) (file, result, error);
}

/**
 * xfile_monitor_directory:
 * @file: input #xfile_t
 * @flags: a set of #xfile_monitor_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Obtains a directory monitor for the given file.
 * This may fail if directory monitoring is not supported.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * It does not make sense for @flags to contain
 * %XFILE_MONITOR_WATCH_HARD_LINKS, since hard links can not be made to
 * directories.  It is not possible to monitor all the files in a
 * directory for changes made via hard links; if you want to do this then
 * you must register individual watches with xfile_monitor().
 *
 * Virtual: monitor_dir
 * Returns: (transfer full): a #xfile_monitor_t for the given @file,
 *   or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_monitor_t *
xfile_monitor_directory (xfile_t              *file,
                          xfile_monitor_flags_t   flags,
                          xcancellable_t       *cancellable,
                          xerror_t            **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (~flags & XFILE_MONITOR_WATCH_HARD_LINKS, NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  if (iface->monitor_dir == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           _("Operation not supported"));
      return NULL;
    }

  return (* iface->monitor_dir) (file, flags, cancellable, error);
}

/**
 * xfile_monitor_file:
 * @file: input #xfile_t
 * @flags: a set of #xfile_monitor_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Obtains a file monitor for the given file. If no file notification
 * mechanism exists, then regular polling of the file is used.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @flags contains %XFILE_MONITOR_WATCH_HARD_LINKS then the monitor
 * will also attempt to report changes made to the file via another
 * filename (ie, a hard link). Without this flag, you can only rely on
 * changes made through the filename contained in @file to be
 * reported. Using this flag may result in an increase in resource
 * usage, and may not have any effect depending on the #xfile_monitor_t
 * backend and/or filesystem type.
 *
 * Returns: (transfer full): a #xfile_monitor_t for the given @file,
 *   or %NULL on error.
 *   Free the returned object with xobject_unref().
 */
xfile_monitor_t *
xfile_monitor_file (xfile_t              *file,
                     xfile_monitor_flags_t   flags,
                     xcancellable_t       *cancellable,
                     xerror_t            **error)
{
  xfile_iface_t *iface;
  xfile_monitor_t *monitor;

  g_return_val_if_fail (X_IS_FILE (file), NULL);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  iface = XFILE_GET_IFACE (file);

  monitor = NULL;

  if (iface->monitor_file)
    monitor = (* iface->monitor_file) (file, flags, cancellable, NULL);

  /* Fallback to polling */
  if (monitor == NULL)
    monitor = _g_poll_file_monitor_new (file);

  return monitor;
}

/**
 * xfile_monitor:
 * @file: input #xfile_t
 * @flags: a set of #xfile_monitor_flags_t
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Obtains a file or directory monitor for the given file,
 * depending on the type of the file.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (transfer full): a #xfile_monitor_t for the given @file,
 *   or %NULL on error.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.18
 */
xfile_monitor_t *
xfile_monitor (xfile_t              *file,
                xfile_monitor_flags_t   flags,
                xcancellable_t       *cancellable,
                xerror_t            **error)
{
  if (xfile_query_file_type (file, 0, cancellable) == XFILE_TYPE_DIRECTORY)
    return xfile_monitor_directory (file,
                                     flags & ~XFILE_MONITOR_WATCH_HARD_LINKS,
                                     cancellable, error);
  else
    return xfile_monitor_file (file, flags, cancellable, error);
}

/********************************************
 *   Default implementation of async ops    *
 ********************************************/

typedef struct {
  char *attributes;
  xfile_query_info_flags_t flags;
} QueryInfoAsyncData;

static void
query_info_data_free (QueryInfoAsyncData *data)
{
  g_free (data->attributes);
  g_free (data);
}

static void
query_info_async_thread (xtask_t         *task,
                         xpointer_t       object,
                         xpointer_t       task_data,
                         xcancellable_t  *cancellable)
{
  QueryInfoAsyncData *data = task_data;
  xfile_info_t *info;
  xerror_t *error = NULL;

  info = xfile_query_info (XFILE (object), data->attributes, data->flags, cancellable, &error);
  if (info)
    xtask_return_pointer (task, info, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_query_info_async (xfile_t               *file,
                              const char          *attributes,
                              xfile_query_info_flags_t  flags,
                              int                  io_priority,
                              xcancellable_t        *cancellable,
                              xasync_ready_callback_t  callback,
                              xpointer_t             user_data)
{
  xtask_t *task;
  QueryInfoAsyncData *data;

  data = g_new0 (QueryInfoAsyncData, 1);
  data->attributes = xstrdup (attributes);
  data->flags = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_query_info_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t)query_info_data_free);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, query_info_async_thread);
  xobject_unref (task);
}

static xfile_info_t *
xfile_real_query_info_finish (xfile_t         *file,
                               xasync_result_t  *res,
                               xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
query_filesystem_info_async_thread (xtask_t         *task,
                                    xpointer_t       object,
                                    xpointer_t       task_data,
                                    xcancellable_t  *cancellable)
{
  const char *attributes = task_data;
  xfile_info_t *info;
  xerror_t *error = NULL;

  info = xfile_query_filesystem_info (XFILE (object), attributes, cancellable, &error);
  if (info)
    xtask_return_pointer (task, info, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_query_filesystem_info_async (xfile_t               *file,
                                         const char          *attributes,
                                         int                  io_priority,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_query_filesystem_info_async);
  xtask_set_task_data (task, xstrdup (attributes), g_free);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, query_filesystem_info_async_thread);
  xobject_unref (task);
}

static xfile_info_t *
xfile_real_query_filesystem_info_finish (xfile_t         *file,
                                          xasync_result_t  *res,
                                          xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
enumerate_children_async_thread (xtask_t         *task,
                                 xpointer_t       object,
                                 xpointer_t       task_data,
                                 xcancellable_t  *cancellable)
{
  QueryInfoAsyncData *data = task_data;
  xfile_enumerator_t *enumerator;
  xerror_t *error = NULL;

  enumerator = xfile_enumerate_children (XFILE (object), data->attributes, data->flags, cancellable, &error);
  if (error)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, enumerator, xobject_unref);
}

static void
xfile_real_enumerate_children_async (xfile_t               *file,
                                      const char          *attributes,
                                      xfile_query_info_flags_t  flags,
                                      int                  io_priority,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  xtask_t *task;
  QueryInfoAsyncData *data;

  data = g_new0 (QueryInfoAsyncData, 1);
  data->attributes = xstrdup (attributes);
  data->flags = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_enumerate_children_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t)query_info_data_free);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, enumerate_children_async_thread);
  xobject_unref (task);
}

static xfile_enumerator_t *
xfile_real_enumerate_children_finish (xfile_t         *file,
                                       xasync_result_t  *res,
                                       xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
open_read_async_thread (xtask_t         *task,
                        xpointer_t       object,
                        xpointer_t       task_data,
                        xcancellable_t  *cancellable)
{
  xfile_input_stream_t *stream;
  xerror_t *error = NULL;

  stream = xfile_read (XFILE (object), cancellable, &error);
  if (stream)
    xtask_return_pointer (task, stream, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_read_async (xfile_t               *file,
                        int                  io_priority,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_read_async);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, open_read_async_thread);
  xobject_unref (task);
}

static xfile_input_stream_t *
xfile_real_read_finish (xfile_t         *file,
                         xasync_result_t  *res,
                         xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
append_to_async_thread (xtask_t         *task,
                        xpointer_t       source_object,
                        xpointer_t       task_data,
                        xcancellable_t  *cancellable)
{
  xfile_create_flags_t *data = task_data;
  xfile_output_stream_t *stream;
  xerror_t *error = NULL;

  stream = xfile_append_to (XFILE (source_object), *data, cancellable, &error);
  if (stream)
    xtask_return_pointer (task, stream, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_append_to_async (xfile_t               *file,
                             xfile_create_flags_t     flags,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data)
{
  xfile_create_flags_t *data;
  xtask_t *task;

  data = g_new0 (xfile_create_flags_t, 1);
  *data = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_append_to_async);
  xtask_set_task_data (task, data, g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, append_to_async_thread);
  xobject_unref (task);
}

static xfile_output_stream_t *
xfile_real_append_to_finish (xfile_t         *file,
                              xasync_result_t  *res,
                              xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
create_async_thread (xtask_t         *task,
                     xpointer_t       source_object,
                     xpointer_t       task_data,
                     xcancellable_t  *cancellable)
{
  xfile_create_flags_t *data = task_data;
  xfile_output_stream_t *stream;
  xerror_t *error = NULL;

  stream = xfile_create (XFILE (source_object), *data, cancellable, &error);
  if (stream)
    xtask_return_pointer (task, stream, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_create_async (xfile_t               *file,
                          xfile_create_flags_t     flags,
                          int                  io_priority,
                          xcancellable_t        *cancellable,
                          xasync_ready_callback_t  callback,
                          xpointer_t             user_data)
{
  xfile_create_flags_t *data;
  xtask_t *task;

  data = g_new0 (xfile_create_flags_t, 1);
  *data = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_create_async);
  xtask_set_task_data (task, data, g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, create_async_thread);
  xobject_unref (task);
}

static xfile_output_stream_t *
xfile_real_create_finish (xfile_t         *file,
                           xasync_result_t  *res,
                           xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

typedef struct {
  xfile_output_stream_t *stream;
  char *etag;
  xboolean_t make_backup;
  xfile_create_flags_t flags;
} ReplaceAsyncData;

static void
replace_async_data_free (ReplaceAsyncData *data)
{
  if (data->stream)
    xobject_unref (data->stream);
  g_free (data->etag);
  g_free (data);
}

static void
replace_async_thread (xtask_t         *task,
                      xpointer_t       source_object,
                      xpointer_t       task_data,
                      xcancellable_t  *cancellable)
{
  xfile_output_stream_t *stream;
  ReplaceAsyncData *data = task_data;
  xerror_t *error = NULL;

  stream = xfile_replace (XFILE (source_object),
                           data->etag,
                           data->make_backup,
                           data->flags,
                           cancellable,
                           &error);

  if (stream)
    xtask_return_pointer (task, stream, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_replace_async (xfile_t               *file,
                           const char          *etag,
                           xboolean_t             make_backup,
                           xfile_create_flags_t     flags,
                           int                  io_priority,
                           xcancellable_t        *cancellable,
                           xasync_ready_callback_t  callback,
                           xpointer_t             user_data)
{
  xtask_t *task;
  ReplaceAsyncData *data;

  data = g_new0 (ReplaceAsyncData, 1);
  data->etag = xstrdup (etag);
  data->make_backup = make_backup;
  data->flags = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_replace_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t)replace_async_data_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, replace_async_thread);
  xobject_unref (task);
}

static xfile_output_stream_t *
xfile_real_replace_finish (xfile_t         *file,
                            xasync_result_t  *res,
                            xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
delete_async_thread (xtask_t        *task,
                     xpointer_t      object,
                     xpointer_t      task_data,
                     xcancellable_t *cancellable)
{
  xerror_t *error = NULL;

  if (xfile_delete (XFILE (object), cancellable, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_delete_async (xfile_t               *file,
                          int                  io_priority,
                          xcancellable_t        *cancellable,
                          xasync_ready_callback_t  callback,
                          xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_delete_async);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, delete_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_delete_finish (xfile_t         *file,
                           xasync_result_t  *res,
                           xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}

static void
trash_async_thread (xtask_t        *task,
                    xpointer_t      object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xerror_t *error = NULL;

  if (xfile_trash (XFILE (object), cancellable, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_trash_async (xfile_t               *file,
                         int                  io_priority,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_trash_async);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, trash_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_trash_finish (xfile_t         *file,
                          xasync_result_t  *res,
                          xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}


typedef struct {
  xfile_t *source;  /* (owned) */
  xfile_t *destination;  /* (owned) */
  xfile_copy_flags_t flags;
  xfile_progress_callback_t progress_cb;
  xpointer_t progress_cb_data;
} MoveAsyncData;

static void
move_async_data_free (MoveAsyncData *data)
{
  xobject_unref (data->source);
  xobject_unref (data->destination);
  g_slice_free (MoveAsyncData, data);
}

typedef struct {
  MoveAsyncData *data;  /* (unowned) */
  xoffset_t current_num_bytes;
  xoffset_t total_num_bytes;
} MoveProgressData;

static xboolean_t
move_async_progress_in_main (xpointer_t user_data)
{
  MoveProgressData *progress = user_data;
  MoveAsyncData *data = progress->data;

  data->progress_cb (progress->current_num_bytes,
                     progress->total_num_bytes,
                     data->progress_cb_data);

  return G_SOURCE_REMOVE;
}

static void
move_async_progress_callback (xoffset_t  current_num_bytes,
                              xoffset_t  total_num_bytes,
                              xpointer_t user_data)
{
  xtask_t *task = user_data;
  MoveAsyncData *data = xtask_get_task_data (task);
  MoveProgressData *progress;

  progress = g_new0 (MoveProgressData, 1);
  progress->data = data;
  progress->current_num_bytes = current_num_bytes;
  progress->total_num_bytes = total_num_bytes;

  xmain_context_invoke_full (xtask_get_context (task),
                              xtask_get_priority (task),
                              move_async_progress_in_main,
                              g_steal_pointer (&progress),
                              g_free);
}

static void
move_async_thread (xtask_t        *task,
                   xpointer_t      source,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  MoveAsyncData *data = task_data;
  xboolean_t result;
  xerror_t *error = NULL;

  result = xfile_move (data->source,
                        data->destination,
                        data->flags,
                        cancellable,
                        (data->progress_cb != NULL) ? move_async_progress_callback : NULL,
                        task,
                        &error);
  if (result)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, g_steal_pointer (&error));
}

static void
xfile_real_move_async (xfile_t                  *source,
                        xfile_t                  *destination,
                        xfile_copy_flags_t          flags,
                        int                     io_priority,
                        xcancellable_t           *cancellable,
                        xfile_progress_callback_t   progress_callback,
                        xpointer_t                progress_callback_data,
                        xasync_ready_callback_t     callback,
                        xpointer_t                user_data)
{
  xtask_t *task;
  MoveAsyncData *data;

  data = g_slice_new0 (MoveAsyncData);
  data->source = xobject_ref (source);
  data->destination = xobject_ref (destination);
  data->flags = flags;
  data->progress_cb = progress_callback;
  data->progress_cb_data = progress_callback_data;

  task = xtask_new (source, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_move_async);
  xtask_set_task_data (task, g_steal_pointer (&data), (xdestroy_notify_t) move_async_data_free);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, move_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_move_finish (xfile_t        *file,
                         xasync_result_t *result,
                         xerror_t      **error)
{
  g_return_val_if_fail (xtask_is_valid (result, file), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static void
make_directory_async_thread (xtask_t        *task,
                             xpointer_t      object,
                             xpointer_t      task_data,
                             xcancellable_t *cancellable)
{
  xerror_t *error = NULL;

  if (xfile_make_directory (XFILE (object), cancellable, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_make_directory_async (xfile_t               *file,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_make_directory_async);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, make_directory_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_make_directory_finish (xfile_t         *file,
                                   xasync_result_t  *res,
                                   xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}

static void
open_readwrite_async_thread (xtask_t        *task,
                             xpointer_t      object,
                             xpointer_t      task_data,
                             xcancellable_t *cancellable)
{
  xfile_io_stream_t *stream;
  xerror_t *error = NULL;

  stream = xfile_open_readwrite (XFILE (object), cancellable, &error);

  if (stream == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, stream, xobject_unref);
}

static void
xfile_real_open_readwrite_async (xfile_t               *file,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_open_readwrite_async);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, open_readwrite_async_thread);
  xobject_unref (task);
}

static xfile_io_stream_t *
xfile_real_open_readwrite_finish (xfile_t         *file,
                                   xasync_result_t  *res,
                                   xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
create_readwrite_async_thread (xtask_t        *task,
                               xpointer_t      object,
                               xpointer_t      task_data,
                               xcancellable_t *cancellable)
{
  xfile_create_flags_t *data = task_data;
  xfile_io_stream_t *stream;
  xerror_t *error = NULL;

  stream = xfile_create_readwrite (XFILE (object), *data, cancellable, &error);

  if (stream == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, stream, xobject_unref);
}

static void
xfile_real_create_readwrite_async (xfile_t               *file,
                                    xfile_create_flags_t     flags,
                                    int                  io_priority,
                                    xcancellable_t        *cancellable,
                                    xasync_ready_callback_t  callback,
                                    xpointer_t             user_data)
{
  xfile_create_flags_t *data;
  xtask_t *task;

  data = g_new0 (xfile_create_flags_t, 1);
  *data = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_create_readwrite_async);
  xtask_set_task_data (task, data, g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, create_readwrite_async_thread);
  xobject_unref (task);
}

static xfile_io_stream_t *
xfile_real_create_readwrite_finish (xfile_t         *file,
                                     xasync_result_t  *res,
                                     xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

typedef struct {
  char *etag;
  xboolean_t make_backup;
  xfile_create_flags_t flags;
} ReplaceRWAsyncData;

static void
replace_rw_async_data_free (ReplaceRWAsyncData *data)
{
  g_free (data->etag);
  g_free (data);
}

static void
replace_readwrite_async_thread (xtask_t        *task,
                                xpointer_t      object,
                                xpointer_t      task_data,
                                xcancellable_t *cancellable)
{
  xfile_io_stream_t *stream;
  xerror_t *error = NULL;
  ReplaceRWAsyncData *data = task_data;

  stream = xfile_replace_readwrite (XFILE (object),
                                     data->etag,
                                     data->make_backup,
                                     data->flags,
                                     cancellable,
                                     &error);

  if (stream == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, stream, xobject_unref);
}

static void
xfile_real_replace_readwrite_async (xfile_t               *file,
                                     const char          *etag,
                                     xboolean_t             make_backup,
                                     xfile_create_flags_t     flags,
                                     int                  io_priority,
                                     xcancellable_t        *cancellable,
                                     xasync_ready_callback_t  callback,
                                     xpointer_t             user_data)
{
  xtask_t *task;
  ReplaceRWAsyncData *data;

  data = g_new0 (ReplaceRWAsyncData, 1);
  data->etag = xstrdup (etag);
  data->make_backup = make_backup;
  data->flags = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_replace_readwrite_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t)replace_rw_async_data_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, replace_readwrite_async_thread);
  xobject_unref (task);
}

static xfile_io_stream_t *
xfile_real_replace_readwrite_finish (xfile_t         *file,
                                      xasync_result_t  *res,
                                      xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
set_display_name_async_thread (xtask_t        *task,
                               xpointer_t      object,
                               xpointer_t      task_data,
                               xcancellable_t *cancellable)
{
  xerror_t *error = NULL;
  char *name = task_data;
  xfile_t *file;

  file = xfile_set_display_name (XFILE (object), name, cancellable, &error);

  if (file == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, file, xobject_unref);
}

static void
xfile_real_set_display_name_async (xfile_t               *file,
                                    const char          *display_name,
                                    int                  io_priority,
                                    xcancellable_t        *cancellable,
                                    xasync_ready_callback_t  callback,
                                    xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_set_display_name_async);
  xtask_set_task_data (task, xstrdup (display_name), g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, set_display_name_async_thread);
  xobject_unref (task);
}

static xfile_t *
xfile_real_set_display_name_finish (xfile_t         *file,
                                     xasync_result_t  *res,
                                     xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}

typedef struct {
  xfile_query_info_flags_t flags;
  xfile_info_t *info;
  xboolean_t res;
  xerror_t *error;
} SetInfoAsyncData;

static void
set_info_data_free (SetInfoAsyncData *data)
{
  if (data->info)
    xobject_unref (data->info);
  if (data->error)
    xerror_free (data->error);
  g_free (data);
}

static void
set_info_async_thread (xtask_t        *task,
                       xpointer_t      object,
                       xpointer_t      task_data,
                       xcancellable_t *cancellable)
{
  SetInfoAsyncData *data = task_data;

  data->error = NULL;
  data->res = xfile_set_attributes_from_info (XFILE (object),
                                               data->info,
                                               data->flags,
                                               cancellable,
                                               &data->error);
}

static void
xfile_real_set_attributes_async (xfile_t               *file,
                                  xfile_info_t           *info,
                                  xfile_query_info_flags_t  flags,
                                  int                  io_priority,
                                  xcancellable_t        *cancellable,
                                  xasync_ready_callback_t  callback,
                                  xpointer_t             user_data)
{
  xtask_t *task;
  SetInfoAsyncData *data;

  data = g_new0 (SetInfoAsyncData, 1);
  data->info = xfile_info_dup (info);
  data->flags = flags;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_set_attributes_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t)set_info_data_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, set_info_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_set_attributes_finish (xfile_t         *file,
                                   xasync_result_t  *res,
                                   xfile_info_t    **info,
                                   xerror_t       **error)
{
  SetInfoAsyncData *data;

  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);

  data = xtask_get_task_data (XTASK (res));

  if (info)
    *info = xobject_ref (data->info);

  if (error != NULL && data->error)
    *error = xerror_copy (data->error);

  return data->res;
}

static void
find_enclosing_mount_async_thread (xtask_t        *task,
                                   xpointer_t      object,
                                   xpointer_t      task_data,
                                   xcancellable_t *cancellable)
{
  xerror_t *error = NULL;
  xmount_t *mount;

  mount = xfile_find_enclosing_mount (XFILE (object), cancellable, &error);

  if (mount == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, mount, xobject_unref);
}

static void
xfile_real_find_enclosing_mount_async (xfile_t               *file,
                                        int                  io_priority,
                                        xcancellable_t        *cancellable,
                                        xasync_ready_callback_t  callback,
                                        xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_find_enclosing_mount_async);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, find_enclosing_mount_async_thread);
  xobject_unref (task);
}

static xmount_t *
xfile_real_find_enclosing_mount_finish (xfile_t         *file,
                                         xasync_result_t  *res,
                                         xerror_t       **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), NULL);

  return xtask_propagate_pointer (XTASK (res), error);
}


typedef struct {
  xfile_t *source;
  xfile_t *destination;
  xfile_copy_flags_t flags;
  xfile_progress_callback_t progress_cb;
  xpointer_t progress_cb_data;
} CopyAsyncData;

static void
copy_async_data_free (CopyAsyncData *data)
{
  xobject_unref (data->source);
  xobject_unref (data->destination);
  g_slice_free (CopyAsyncData, data);
}

typedef struct {
  CopyAsyncData *data;
  xoffset_t current_num_bytes;
  xoffset_t total_num_bytes;
} CopyProgressData;

static xboolean_t
copy_async_progress_in_main (xpointer_t user_data)
{
  CopyProgressData *progress = user_data;
  CopyAsyncData *data = progress->data;

  data->progress_cb (progress->current_num_bytes,
                     progress->total_num_bytes,
                     data->progress_cb_data);

  return FALSE;
}

static void
copy_async_progress_callback (xoffset_t  current_num_bytes,
                              xoffset_t  total_num_bytes,
                              xpointer_t user_data)
{
  xtask_t *task = user_data;
  CopyAsyncData *data = xtask_get_task_data (task);
  CopyProgressData *progress;

  progress = g_new (CopyProgressData, 1);
  progress->data = data;
  progress->current_num_bytes = current_num_bytes;
  progress->total_num_bytes = total_num_bytes;

  xmain_context_invoke_full (xtask_get_context (task),
                              xtask_get_priority (task),
                              copy_async_progress_in_main,
                              progress,
                              g_free);
}

static void
copy_async_thread (xtask_t        *task,
                   xpointer_t      source,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  CopyAsyncData *data = task_data;
  xboolean_t result;
  xerror_t *error = NULL;

  result = xfile_copy (data->source,
                        data->destination,
                        data->flags,
                        cancellable,
                        (data->progress_cb != NULL) ? copy_async_progress_callback : NULL,
                        task,
                        &error);
  if (result)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_copy_async (xfile_t                  *source,
                        xfile_t                  *destination,
                        xfile_copy_flags_t          flags,
                        int                     io_priority,
                        xcancellable_t           *cancellable,
                        xfile_progress_callback_t   progress_callback,
                        xpointer_t                progress_callback_data,
                        xasync_ready_callback_t     callback,
                        xpointer_t                user_data)
{
  xtask_t *task;
  CopyAsyncData *data;

  data = g_slice_new (CopyAsyncData);
  data->source = xobject_ref (source);
  data->destination = xobject_ref (destination);
  data->flags = flags;
  data->progress_cb = progress_callback;
  data->progress_cb_data = progress_callback_data;

  task = xtask_new (source, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_copy_async);
  xtask_set_task_data (task, data, (xdestroy_notify_t)copy_async_data_free);
  xtask_set_priority (task, io_priority);
  xtask_run_in_thread (task, copy_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_copy_finish (xfile_t        *file,
                         xasync_result_t *res,
                         xerror_t      **error)
{
  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);

  return xtask_propagate_boolean (XTASK (res), error);
}


/********************************************
 *   Default VFS operations                 *
 ********************************************/

/**
 * xfile_new_for_path:
 * @path: (type filename): a string containing a relative or absolute path.
 *   The string must be encoded in the glib filename encoding.
 *
 * Constructs a #xfile_t for a given path. This operation never
 * fails, but the returned object might not support any I/O
 * operation if @path is malformed.
 *
 * Returns: (transfer full): a new #xfile_t for the given @path.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_new_for_path (const char *path)
{
  g_return_val_if_fail (path != NULL, NULL);

  return xvfs_get_file_for_path (xvfs_get_default (), path);
}

/**
 * xfile_new_for_uri:
 * @uri: a UTF-8 string containing a URI
 *
 * Constructs a #xfile_t for a given URI. This operation never
 * fails, but the returned object might not support any I/O
 * operation if @uri is malformed or if the uri type is
 * not supported.
 *
 * Returns: (transfer full): a new #xfile_t for the given @uri.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_new_for_uri (const char *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return xvfs_get_file_for_uri (xvfs_get_default (), uri);
}

/**
 * xfile_new_tmp:
 * @tmpl: (type filename) (nullable): Template for the file
 *   name, as in xfile_open_tmp(), or %NULL for a default template
 * @iostream: (out): on return, a #xfile_io_stream_t for the created file
 * @error: a #xerror_t, or %NULL
 *
 * Opens a file in the preferred directory for temporary files (as
 * returned by g_get_tmp_dir()) and returns a #xfile_t and
 * #xfile_io_stream_t pointing to it.
 *
 * @tmpl should be a string in the GLib file name encoding
 * containing a sequence of six 'X' characters, and containing no
 * directory components. If it is %NULL, a default template is used.
 *
 * Unlike the other #xfile_t constructors, this will return %NULL if
 * a temporary file could not be created.
 *
 * Returns: (transfer full): a new #xfile_t.
 *   Free the returned object with xobject_unref().
 *
 * Since: 2.32
 */
xfile_t *
xfile_new_tmp (const char     *tmpl,
                xfile_io_stream_t **iostream,
                xerror_t        **error)
{
  xint_t fd;
  xchar_t *path;
  xfile_t *file;
  xfile_output_stream_t *output;

  g_return_val_if_fail (iostream != NULL, NULL);

  fd = xfile_open_tmp (tmpl, &path, error);
  if (fd == -1)
    return NULL;

  file = xfile_new_for_path (path);

  output = _g_local_file_output_stream_new (fd);
  *iostream = _xlocal_file_io_stream_new (G_LOCAL_FILE_OUTPUT_STREAM (output));

  xobject_unref (output);
  g_free (path);

  return file;
}

/**
 * xfile_parse_name:
 * @parse_name: a file name or path to be parsed
 *
 * Constructs a #xfile_t with the given @parse_name (i.e. something
 * given by xfile_get_parse_name()). This operation never fails,
 * but the returned object might not support any I/O operation if
 * the @parse_name cannot be parsed.
 *
 * Returns: (transfer full): a new #xfile_t.
 */
xfile_t *
xfile_parse_name (const char *parse_name)
{
  g_return_val_if_fail (parse_name != NULL, NULL);

  return xvfs_parse_name (xvfs_get_default (), parse_name);
}

/**
 * xfile_new_build_filename:
 * @first_element: (type filename): the first element in the path
 * @...: remaining elements in path, terminated by %NULL
 *
 * Constructs a #xfile_t from a series of elements using the correct
 * separator for filenames.
 *
 * Using this function is equivalent to calling g_build_filename(),
 * followed by xfile_new_for_path() on the result.
 *
 * Returns: (transfer full): a new #xfile_t
 *
 * Since: 2.56
 */
xfile_t *
xfile_new_build_filename (const xchar_t *first_element,
                           ...)
{
  xchar_t *str;
  xfile_t *file;
  va_list args;

  g_return_val_if_fail (first_element != NULL, NULL);

  va_start (args, first_element);
  str = g_build_filename_valist (first_element, &args);
  va_end (args);

  file = xfile_new_for_path (str);
  g_free (str);

  return file;
}

static xboolean_t
is_valid_scheme_character (char c)
{
  return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

/* Following RFC 2396, valid schemes are built like:
 *       scheme        = alpha *( alpha | digit | "+" | "-" | "." )
 */
static xboolean_t
has_valid_scheme (const char *uri)
{
  const char *p;

  p = uri;

  if (!g_ascii_isalpha (*p))
    return FALSE;

  do {
    p++;
  } while (is_valid_scheme_character (*p));

  return *p == ':';
}

static xfile_t *
new_for_cmdline_arg (const xchar_t *arg,
                     const xchar_t *cwd)
{
  xfile_t *file;
  char *filename;

  if (g_path_is_absolute (arg))
    return xfile_new_for_path (arg);

  if (has_valid_scheme (arg))
    return xfile_new_for_uri (arg);

  if (cwd == NULL)
    {
      char *current_dir;

      current_dir = g_get_current_dir ();
      filename = g_build_filename (current_dir, arg, NULL);
      g_free (current_dir);
    }
  else
    filename = g_build_filename (cwd, arg, NULL);

  file = xfile_new_for_path (filename);
  g_free (filename);

  return file;
}

/**
 * xfile_new_for_commandline_arg:
 * @arg: (type filename): a command line string
 *
 * Creates a #xfile_t with the given argument from the command line.
 * The value of @arg can be either a URI, an absolute path or a
 * relative path resolved relative to the current working directory.
 * This operation never fails, but the returned object might not
 * support any I/O operation if @arg points to a malformed path.
 *
 * Note that on Windows, this function expects its argument to be in
 * UTF-8 -- not the system code page.  This means that you
 * should not use this function with string from argv as it is passed
 * to main().  g_win32_get_command_line() will return a UTF-8 version of
 * the commandline.  #xapplication_t also uses UTF-8 but
 * xapplication_command_line_create_file_for_arg() may be more useful
 * for you there.  It is also always possible to use this function with
 * #xoption_context_t arguments of type %G_OPTION_ARXFILENAME.
 *
 * Returns: (transfer full): a new #xfile_t.
 *   Free the returned object with xobject_unref().
 */
xfile_t *
xfile_new_for_commandline_arg (const char *arg)
{
  g_return_val_if_fail (arg != NULL, NULL);

  return new_for_cmdline_arg (arg, NULL);
}

/**
 * xfile_new_for_commandline_arg_and_cwd:
 * @arg: (type filename): a command line string
 * @cwd: (type filename): the current working directory of the commandline
 *
 * Creates a #xfile_t with the given argument from the command line.
 *
 * This function is similar to xfile_new_for_commandline_arg() except
 * that it allows for passing the current working directory as an
 * argument instead of using the current working directory of the
 * process.
 *
 * This is useful if the commandline argument was given in a context
 * other than the invocation of the current process.
 *
 * See also xapplication_command_line_create_file_for_arg().
 *
 * Returns: (transfer full): a new #xfile_t
 *
 * Since: 2.36
 **/
xfile_t *
xfile_new_for_commandline_arg_and_cwd (const xchar_t *arg,
                                        const xchar_t *cwd)
{
  g_return_val_if_fail (arg != NULL, NULL);
  g_return_val_if_fail (cwd != NULL, NULL);

  return new_for_cmdline_arg (arg, cwd);
}

/**
 * xfile_mount_enclosing_volume:
 * @location: input #xfile_t
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t
 *   or %NULL to avoid user interaction
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: the data to pass to callback function
 *
 * Starts a @mount_operation, mounting the volume that contains
 * the file @location.
 *
 * When this operation has completed, @callback will be called with
 * @user_user data, and the operation can be finalized with
 * xfile_mount_enclosing_volume_finish().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 */
void
xfile_mount_enclosing_volume (xfile_t               *location,
                               GMountMountFlags     flags,
                               xmount_operation_t     *mount_operation,
                               xcancellable_t        *cancellable,
                               xasync_ready_callback_t  callback,
                               xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (location));

  iface = XFILE_GET_IFACE (location);

  if (iface->mount_enclosing_volume == NULL)
    {
      xtask_report_new_error (location, callback, user_data,
                               xfile_mount_enclosing_volume,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("volume doesn’t implement mount"));
      return;
    }

  (* iface->mount_enclosing_volume) (location, flags, mount_operation, cancellable, callback, user_data);

}

/**
 * xfile_mount_enclosing_volume_finish:
 * @location: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes a mount operation started by xfile_mount_enclosing_volume().
 *
 * Returns: %TRUE if successful. If an error has occurred,
 *   this function will return %FALSE and set @error
 *   appropriately if present.
 */
xboolean_t
xfile_mount_enclosing_volume_finish (xfile_t         *location,
                                      xasync_result_t  *result,
                                      xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (location), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_mount_enclosing_volume))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (location);

  return (* iface->mount_enclosing_volume_finish) (location, result, error);
}

/********************************************
 *   Utility functions                      *
 ********************************************/

/**
 * xfile_query_default_handler:
 * @file: a #xfile_t to open
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Returns the #xapp_info_t that is registered as the default
 * application to handle the file specified by @file.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (transfer full): a #xapp_info_t if the handle was found,
 *   %NULL if there were errors.
 *   When you are done with it, release it with xobject_unref()
 */
xapp_info_t *
xfile_query_default_handler (xfile_t         *file,
                              xcancellable_t  *cancellable,
                              xerror_t       **error)
{
  char *uri_scheme;
  const char *content_type;
  xapp_info_t *appinfo;
  xfile_info_t *info;
  char *path;

  uri_scheme = xfile_get_uri_scheme (file);
  if (uri_scheme && uri_scheme[0] != '\0')
    {
      appinfo = xapp_info_get_default_for_uri_scheme (uri_scheme);
      g_free (uri_scheme);

      if (appinfo != NULL)
        return appinfo;
    }
  else
    g_free (uri_scheme);

  info = xfile_query_info (file,
                            XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                            XFILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
                            0,
                            cancellable,
                            error);
  if (info == NULL)
    return NULL;

  appinfo = NULL;

  content_type = xfile_info_get_content_type (info);
  if (content_type == NULL)
    content_type = xfile_info_get_attribute_string (info, XFILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
  if (content_type)
    {
      /* Don't use is_native(), as we want to support fuse paths if available */
      path = xfile_get_path (file);
      appinfo = xapp_info_get_default_for_type (content_type,
                                                 path == NULL);
      g_free (path);
    }

  xobject_unref (info);

  if (appinfo != NULL)
    return appinfo;

  g_set_error_literal (error, G_IO_ERROR,
                       G_IO_ERROR_NOT_SUPPORTED,
                       _("No application is registered as handling this file"));
  return NULL;
}

static void
query_default_handler_query_info_cb (xobject_t      *object,
                                     xasync_result_t *result,
                                     xpointer_t      user_data)
{
  xfile_t *file = XFILE (object);
  xtask_t *task = XTASK (user_data);
  xerror_t *error = NULL;
  xfile_info_t *info;
  const char *content_type;
  xapp_info_t *appinfo = NULL;

  info = xfile_query_info_finish (file, result, &error);
  if (info == NULL)
    {
      xtask_return_error (task, g_steal_pointer (&error));
      xobject_unref (task);
      return;
    }

  content_type = xfile_info_get_content_type (info);
  if (content_type == NULL)
    content_type = xfile_info_get_attribute_string (info, XFILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
  if (content_type)
    {
      char *path;

      /* Don't use is_native(), as we want to support fuse paths if available */
      path = xfile_get_path (file);

      /* FIXME: The following still uses blocking calls. */
      appinfo = xapp_info_get_default_for_type (content_type,
                                                 path == NULL);
      g_free (path);
    }

  xobject_unref (info);

  if (appinfo != NULL)
    xtask_return_pointer (task, g_steal_pointer (&appinfo), xobject_unref);
  else
    xtask_return_new_error (task,
                             G_IO_ERROR,
                             G_IO_ERROR_NOT_SUPPORTED,
                             _("No application is registered as handling this file"));
  xobject_unref (task);
}

/**
 * xfile_query_default_handler_async:
 * @file: a #xfile_t to open
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Async version of xfile_query_default_handler().
 *
 * Since: 2.60
 */
void
xfile_query_default_handler_async (xfile_t              *file,
                                    int                 io_priority,
                                    xcancellable_t       *cancellable,
                                    xasync_ready_callback_t callback,
                                    xpointer_t            user_data)
{
  xtask_t *task;
  char *uri_scheme;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_query_default_handler_async);

  uri_scheme = xfile_get_uri_scheme (file);
  if (uri_scheme && uri_scheme[0] != '\0')
    {
      xapp_info_t *appinfo;

      /* FIXME: The following still uses blocking calls. */
      appinfo = xapp_info_get_default_for_uri_scheme (uri_scheme);
      g_free (uri_scheme);

      if (appinfo != NULL)
        {
          xtask_return_pointer (task, g_steal_pointer (&appinfo), xobject_unref);
          xobject_unref (task);
          return;
        }
    }
  else
    g_free (uri_scheme);

  xfile_query_info_async (file,
                           XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                           XFILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
                           0,
                           io_priority,
                           cancellable,
                           query_default_handler_query_info_cb,
                           g_steal_pointer (&task));
}

/**
 * xfile_query_default_handler_finish:
 * @file: a #xfile_t to open
 * @result: a #xasync_result_t
 * @error: (nullable): a #xerror_t
 *
 * Finishes a xfile_query_default_handler_async() operation.
 *
 * Returns: (transfer full): a #xapp_info_t if the handle was found,
 *   %NULL if there were errors.
 *   When you are done with it, release it with xobject_unref()
 *
 * Since: 2.60
 */
xapp_info_t *
xfile_query_default_handler_finish (xfile_t        *file,
                                     xasync_result_t *result,
                                     xerror_t      **error)
{
  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (xtask_is_valid (result, file), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

#define GET_CONTENT_BLOCK_SIZE 8192

/**
 * xfile_load_contents:
 * @file: input #xfile_t
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @contents: (out) (transfer full) (element-type xuint8_t) (array length=length): a location to place the contents of the file
 * @length: (out) (optional): a location to place the length of the contents of the file,
 *   or %NULL if the length is not needed
 * @etag_out: (out) (optional) (nullable): a location to place the current entity tag for the file,
 *   or %NULL if the entity tag is not needed
 * @error: a #xerror_t, or %NULL
 *
 * Loads the content of the file into memory. The data is always
 * zero-terminated, but this is not included in the resultant @length.
 * The returned @contents should be freed with g_free() when no longer
 * needed.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: %TRUE if the @file's contents were successfully loaded.
 *   %FALSE if there were errors.
 */
xboolean_t
xfile_load_contents (xfile_t         *file,
                      xcancellable_t  *cancellable,
                      char         **contents,
                      xsize_t         *length,
                      char         **etag_out,
                      xerror_t       **error)
{
  xfile_input_stream_t *in;
  xbyte_array_t *content;
  xsize_t pos;
  xssize_t res;
  xfile_info_t *info;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  in = xfile_read (file, cancellable, error);
  if (in == NULL)
    return FALSE;

  content = xbyte_array_new ();
  pos = 0;

  xbyte_array_set_size (content, pos + GET_CONTENT_BLOCK_SIZE + 1);
  while ((res = xinput_stream_read (G_INPUT_STREAM (in),
                                     content->data + pos,
                                     GET_CONTENT_BLOCK_SIZE,
                                     cancellable, error)) > 0)
    {
      pos += res;
      xbyte_array_set_size (content, pos + GET_CONTENT_BLOCK_SIZE + 1);
    }

  if (etag_out)
    {
      *etag_out = NULL;

      info = xfile_input_stream_query_info (in,
                                             XFILE_ATTRIBUTE_ETAG_VALUE,
                                             cancellable,
                                             NULL);
      if (info)
        {
          *etag_out = xstrdup (xfile_info_get_etag (info));
          xobject_unref (info);
        }
    }

  /* Ignore errors on close */
  xinput_stream_close (G_INPUT_STREAM (in), cancellable, NULL);
  xobject_unref (in);

  if (res < 0)
    {
      /* error is set already */
      xbyte_array_free (content, TRUE);
      return FALSE;
    }

  if (length)
    *length = pos;

  /* Zero terminate (we got an extra byte allocated for this */
  content->data[pos] = 0;

  *contents = (char *)xbyte_array_free (content, FALSE);

  return TRUE;
}

typedef struct {
  xtask_t *task;
  xfile_read_more_callback_t read_more_callback;
  xbyte_array_t *content;
  xsize_t pos;
  char *etag;
} LoadContentsData;


static void
load_contents_data_free (LoadContentsData *data)
{
  if (data->content)
    xbyte_array_free (data->content, TRUE);
  g_free (data->etag);
  g_free (data);
}

static void
load_contents_close_callback (xobject_t      *obj,
                              xasync_result_t *close_res,
                              xpointer_t      user_data)
{
  xinput_stream_t *stream = G_INPUT_STREAM (obj);
  LoadContentsData *data = user_data;

  /* Ignore errors here, we're only reading anyway */
  xinput_stream_close_finish (stream, close_res, NULL);
  xobject_unref (stream);

  xtask_return_boolean (data->task, TRUE);
  xobject_unref (data->task);
}

static void
load_contents_fstat_callback (xobject_t      *obj,
                              xasync_result_t *stat_res,
                              xpointer_t      user_data)
{
  xinput_stream_t *stream = G_INPUT_STREAM (obj);
  LoadContentsData *data = user_data;
  xfile_info_t *info;

  info = xfile_input_stream_query_info_finish (XFILE_INPUT_STREAM (stream),
                                                stat_res, NULL);
  if (info)
    {
      data->etag = xstrdup (xfile_info_get_etag (info));
      xobject_unref (info);
    }

  xinput_stream_close_async (stream, 0,
                              xtask_get_cancellable (data->task),
                              load_contents_close_callback, data);
}

static void
load_contents_read_callback (xobject_t      *obj,
                             xasync_result_t *read_res,
                             xpointer_t      user_data)
{
  xinput_stream_t *stream = G_INPUT_STREAM (obj);
  LoadContentsData *data = user_data;
  xerror_t *error = NULL;
  xssize_t read_size;

  read_size = xinput_stream_read_finish (stream, read_res, &error);

  if (read_size < 0)
    {
      xtask_return_error (data->task, error);
      xobject_unref (data->task);

      /* Close the file ignoring any error */
      xinput_stream_close_async (stream, 0, NULL, NULL, NULL);
      xobject_unref (stream);
    }
  else if (read_size == 0)
    {
      xfile_input_stream_query_info_async (XFILE_INPUT_STREAM (stream),
                                            XFILE_ATTRIBUTE_ETAG_VALUE,
                                            0,
                                            xtask_get_cancellable (data->task),
                                            load_contents_fstat_callback,
                                            data);
    }
  else if (read_size > 0)
    {
      data->pos += read_size;

      xbyte_array_set_size (data->content,
                             data->pos + GET_CONTENT_BLOCK_SIZE);


      if (data->read_more_callback &&
          !data->read_more_callback ((char *)data->content->data, data->pos,
                                     xasync_result_get_user_data (G_ASYNC_RESULT (data->task))))
        xfile_input_stream_query_info_async (XFILE_INPUT_STREAM (stream),
                                              XFILE_ATTRIBUTE_ETAG_VALUE,
                                              0,
                                              xtask_get_cancellable (data->task),
                                              load_contents_fstat_callback,
                                              data);
      else
        xinput_stream_read_async (stream,
                                   data->content->data + data->pos,
                                   GET_CONTENT_BLOCK_SIZE,
                                   0,
                                   xtask_get_cancellable (data->task),
                                   load_contents_read_callback,
                                   data);
    }
}

static void
load_contents_open_callback (xobject_t      *obj,
                             xasync_result_t *open_res,
                             xpointer_t      user_data)
{
  xfile_t *file = XFILE (obj);
  xfile_input_stream_t *stream;
  LoadContentsData *data = user_data;
  xerror_t *error = NULL;

  stream = xfile_read_finish (file, open_res, &error);

  if (stream)
    {
      xbyte_array_set_size (data->content,
                             data->pos + GET_CONTENT_BLOCK_SIZE);
      xinput_stream_read_async (G_INPUT_STREAM (stream),
                                 data->content->data + data->pos,
                                 GET_CONTENT_BLOCK_SIZE,
                                 0,
                                 xtask_get_cancellable (data->task),
                                 load_contents_read_callback,
                                 data);
    }
  else
    {
      xtask_return_error (data->task, error);
      xobject_unref (data->task);
    }
}

/**
 * xfile_load_partial_contents_async: (skip)
 * @file: input #xfile_t
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @read_more_callback: (scope call) (closure user_data): a
 *   #xfile_read_more_callback_t to receive partial data
 *   and to specify whether further data should be read
 * @callback: (scope async) (closure user_data): a #xasync_ready_callback_t to call
 *   when the request is satisfied
 * @user_data: the data to pass to the callback functions
 *
 * Reads the partial contents of a file. A #xfile_read_more_callback_t should
 * be used to stop reading from the file when appropriate, else this
 * function will behave exactly as xfile_load_contents_async(). This
 * operation can be finished by xfile_load_partial_contents_finish().
 *
 * Users of this function should be aware that @user_data is passed to
 * both the @read_more_callback and the @callback.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 */
void
xfile_load_partial_contents_async (xfile_t                 *file,
                                    xcancellable_t          *cancellable,
                                    xfile_read_more_callback_t  read_more_callback,
                                    xasync_ready_callback_t    callback,
                                    xpointer_t               user_data)
{
  LoadContentsData *data;

  g_return_if_fail (X_IS_FILE (file));

  data = g_new0 (LoadContentsData, 1);
  data->read_more_callback = read_more_callback;
  data->content = xbyte_array_new ();

  data->task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (data->task, xfile_load_partial_contents_async);
  xtask_set_task_data (data->task, data, (xdestroy_notify_t)load_contents_data_free);

  xfile_read_async (file,
                     0,
                     xtask_get_cancellable (data->task),
                     load_contents_open_callback,
                     data);
}

/**
 * xfile_load_partial_contents_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @contents: (out) (transfer full) (element-type xuint8_t) (array length=length): a location to place the contents of the file
 * @length: (out) (optional): a location to place the length of the contents of the file,
 *   or %NULL if the length is not needed
 * @etag_out: (out) (optional) (nullable): a location to place the current entity tag for the file,
 *   or %NULL if the entity tag is not needed
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous partial load operation that was started
 * with xfile_load_partial_contents_async(). The data is always
 * zero-terminated, but this is not included in the resultant @length.
 * The returned @contents should be freed with g_free() when no longer
 * needed.
 *
 * Returns: %TRUE if the load was successful. If %FALSE and @error is
 *   present, it will be set appropriately.
 */
xboolean_t
xfile_load_partial_contents_finish (xfile_t         *file,
                                     xasync_result_t  *res,
                                     char         **contents,
                                     xsize_t         *length,
                                     char         **etag_out,
                                     xerror_t       **error)
{
  xtask_t *task;
  LoadContentsData *data;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  task = XTASK (res);

  if (!xtask_propagate_boolean (task, error))
    {
      if (length)
        *length = 0;
      return FALSE;
    }

  data = xtask_get_task_data (task);

  if (length)
    *length = data->pos;

  if (etag_out)
    {
      *etag_out = data->etag;
      data->etag = NULL;
    }

  /* Zero terminate */
  xbyte_array_set_size (data->content, data->pos + 1);
  data->content->data[data->pos] = 0;

  *contents = (char *)xbyte_array_free (data->content, FALSE);
  data->content = NULL;

  return TRUE;
}

/**
 * xfile_load_contents_async:
 * @file: input #xfile_t
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Starts an asynchronous load of the @file's contents.
 *
 * For more details, see xfile_load_contents() which is
 * the synchronous version of this call.
 *
 * When the load operation has completed, @callback will be called
 * with @user data. To finish the operation, call
 * xfile_load_contents_finish() with the #xasync_result_t returned by
 * the @callback.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 */
void
xfile_load_contents_async (xfile_t               *file,
                           xcancellable_t        *cancellable,
                           xasync_ready_callback_t  callback,
                           xpointer_t             user_data)
{
  xfile_load_partial_contents_async (file,
                                      cancellable,
                                      NULL,
                                      callback, user_data);
}

/**
 * xfile_load_contents_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @contents: (out) (transfer full) (element-type xuint8_t) (array length=length): a location to place the contents of the file
 * @length: (out) (optional): a location to place the length of the contents of the file,
 *   or %NULL if the length is not needed
 * @etag_out: (out) (optional) (nullable): a location to place the current entity tag for the file,
 *   or %NULL if the entity tag is not needed
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous load of the @file's contents.
 * The contents are placed in @contents, and @length is set to the
 * size of the @contents string. The @contents should be freed with
 * g_free() when no longer needed. If @etag_out is present, it will be
 * set to the new entity tag for the @file.
 *
 * Returns: %TRUE if the load was successful. If %FALSE and @error is
 *   present, it will be set appropriately.
 */
xboolean_t
xfile_load_contents_finish (xfile_t         *file,
                             xasync_result_t  *res,
                             char         **contents,
                             xsize_t         *length,
                             char         **etag_out,
                             xerror_t       **error)
{
  return xfile_load_partial_contents_finish (file,
                                              res,
                                              contents,
                                              length,
                                              etag_out,
                                              error);
}

/**
 * xfile_replace_contents:
 * @file: input #xfile_t
 * @contents: (element-type xuint8_t) (array length=length): a string containing the new contents for @file
 * @length: the length of @contents in bytes
 * @etag: (nullable): the old [entity-tag][gfile-etag] for the document,
 *   or %NULL
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @new_etag: (out) (optional) (nullable): a location to a new [entity tag][gfile-etag]
 *   for the document. This should be freed with g_free() when no longer
 *   needed, or %NULL
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @error: a #xerror_t, or %NULL
 *
 * Replaces the contents of @file with @contents of @length bytes.
 *
 * If @etag is specified (not %NULL), any existing file must have that etag,
 * or the error %G_IO_ERROR_WRONG_ETAG will be returned.
 *
 * If @make_backup is %TRUE, this function will attempt to make a backup
 * of @file. Internally, it uses xfile_replace(), so will try to replace the
 * file contents in the safest way possible. For example, atomic renames are
 * used when replacing local files’ contents.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * The returned @new_etag can be used to verify that the file hasn't
 * changed the next time it is saved over.
 *
 * Returns: %TRUE if successful. If an error has occurred, this function
 *   will return %FALSE and set @error appropriately if present.
 */
xboolean_t
xfile_replace_contents (xfile_t             *file,
                         const char        *contents,
                         xsize_t              length,
                         const char        *etag,
                         xboolean_t           make_backup,
                         xfile_create_flags_t   flags,
                         char             **new_etag,
                         xcancellable_t      *cancellable,
                         xerror_t           **error)
{
  xfile_output_stream_t *out;
  xsize_t pos, remainder;
  xssize_t res;
  xboolean_t ret;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  out = xfile_replace (file, etag, make_backup, flags, cancellable, error);
  if (out == NULL)
    return FALSE;

  pos = 0;
  remainder = length;
  while (remainder > 0 &&
         (res = xoutput_stream_write (G_OUTPUT_STREAM (out),
                                       contents + pos,
                                       MIN (remainder, GET_CONTENT_BLOCK_SIZE),
                                       cancellable,
                                       error)) > 0)
    {
      pos += res;
      remainder -= res;
    }

  if (remainder > 0 && res < 0)
    {
      /* Ignore errors on close */
      xoutput_stream_close (G_OUTPUT_STREAM (out), cancellable, NULL);
      xobject_unref (out);

      /* error is set already */
      return FALSE;
    }

  ret = xoutput_stream_close (G_OUTPUT_STREAM (out), cancellable, error);

  if (new_etag)
    *new_etag = xfile_output_stream_get_etag (out);

  xobject_unref (out);

  return ret;
}

typedef struct {
  xtask_t *task;
  xbytes_t *content;
  xsize_t pos;
  char *etag;
  xboolean_t failed;
} ReplaceContentsData;

static void
replace_contents_data_free (ReplaceContentsData *data)
{
  xbytes_unref (data->content);
  g_free (data->etag);
  g_free (data);
}

static void
replace_contents_close_callback (xobject_t      *obj,
                                 xasync_result_t *close_res,
                                 xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (obj);
  ReplaceContentsData *data = user_data;

  /* Ignore errors here, we're only reading anyway */
  xoutput_stream_close_finish (stream, close_res, NULL);

  if (!data->failed)
    {
      data->etag = xfile_output_stream_get_etag (XFILE_OUTPUT_STREAM (stream));
      xtask_return_boolean (data->task, TRUE);
    }
  xobject_unref (data->task);
}

static void
replace_contents_write_callback (xobject_t      *obj,
                                 xasync_result_t *read_res,
                                 xpointer_t      user_data)
{
  xoutput_stream_t *stream = G_OUTPUT_STREAM (obj);
  ReplaceContentsData *data = user_data;
  xerror_t *error = NULL;
  xssize_t write_size;

  write_size = xoutput_stream_write_finish (stream, read_res, &error);

  if (write_size <= 0)
    {
      /* Error or EOF, close the file */
      if (write_size < 0)
        {
          data->failed = TRUE;
          xtask_return_error (data->task, error);
        }
      xoutput_stream_close_async (stream, 0,
                                   xtask_get_cancellable (data->task),
                                   replace_contents_close_callback, data);
    }
  else if (write_size > 0)
    {
      const xchar_t *content;
      xsize_t length;

      content = xbytes_get_data (data->content, &length);
      data->pos += write_size;

      if (data->pos >= length)
        xoutput_stream_close_async (stream, 0,
                                     xtask_get_cancellable (data->task),
                                     replace_contents_close_callback, data);
      else
        xoutput_stream_write_async (stream,
                                     content + data->pos,
                                     length - data->pos,
                                     0,
                                     xtask_get_cancellable (data->task),
                                     replace_contents_write_callback,
                                     data);
    }
}

static void
replace_contents_open_callback (xobject_t      *obj,
                                xasync_result_t *open_res,
                                xpointer_t      user_data)
{
  xfile_t *file = XFILE (obj);
  xfile_output_stream_t *stream;
  ReplaceContentsData *data = user_data;
  xerror_t *error = NULL;

  stream = xfile_replace_finish (file, open_res, &error);

  if (stream)
    {
      const xchar_t *content;
      xsize_t length;

      content = xbytes_get_data (data->content, &length);
      xoutput_stream_write_async (G_OUTPUT_STREAM (stream),
                                   content + data->pos,
                                   length - data->pos,
                                   0,
                                   xtask_get_cancellable (data->task),
                                   replace_contents_write_callback,
                                   data);
      xobject_unref (stream);  /* ownership is transferred to the write_async() call above */
    }
  else
    {
      xtask_return_error (data->task, error);
      xobject_unref (data->task);
    }
}

/**
 * xfile_replace_contents_async:
 * @file: input #xfile_t
 * @contents: (element-type xuint8_t) (array length=length): string of contents to replace the file with
 * @length: the length of @contents in bytes
 * @etag: (nullable): a new [entity tag][gfile-etag] for the @file, or %NULL
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Starts an asynchronous replacement of @file with the given
 * @contents of @length bytes. @etag will replace the document's
 * current entity tag.
 *
 * When this operation has completed, @callback will be called with
 * @user_user data, and the operation can be finalized with
 * xfile_replace_contents_finish().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @make_backup is %TRUE, this function will attempt to
 * make a backup of @file.
 *
 * Note that no copy of @contents will be made, so it must stay valid
 * until @callback is called. See xfile_replace_contents_bytes_async()
 * for a #xbytes_t version that will automatically hold a reference to the
 * contents (without copying) for the duration of the call.
 */
void
xfile_replace_contents_async  (xfile_t               *file,
                                const char          *contents,
                                xsize_t                length,
                                const char          *etag,
                                xboolean_t             make_backup,
                                xfile_create_flags_t     flags,
                                xcancellable_t        *cancellable,
                                xasync_ready_callback_t  callback,
                                xpointer_t             user_data)
{
  xbytes_t *bytes;

  bytes = xbytes_new_static (contents, length);
  xfile_replace_contents_bytes_async (file, bytes, etag, make_backup, flags,
      cancellable, callback, user_data);
  xbytes_unref (bytes);
}

/**
 * xfile_replace_contents_bytes_async:
 * @file: input #xfile_t
 * @contents: a #xbytes_t
 * @etag: (nullable): a new [entity tag][gfile-etag] for the @file, or %NULL
 * @make_backup: %TRUE if a backup should be created
 * @flags: a set of #xfile_create_flags_t
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @callback: a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Same as xfile_replace_contents_async() but takes a #xbytes_t input instead.
 * This function will keep a ref on @contents until the operation is done.
 * Unlike xfile_replace_contents_async() this allows forgetting about the
 * content without waiting for the callback.
 *
 * When this operation has completed, @callback will be called with
 * @user_user data, and the operation can be finalized with
 * xfile_replace_contents_finish().
 *
 * Since: 2.40
 */
void
xfile_replace_contents_bytes_async  (xfile_t               *file,
                                      xbytes_t              *contents,
                                      const char          *etag,
                                      xboolean_t             make_backup,
                                      xfile_create_flags_t     flags,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  ReplaceContentsData *data;

  g_return_if_fail (X_IS_FILE (file));
  g_return_if_fail (contents != NULL);

  data = g_new0 (ReplaceContentsData, 1);

  data->content = xbytes_ref (contents);

  data->task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (data->task, xfile_replace_contents_bytes_async);
  xtask_set_task_data (data->task, data, (xdestroy_notify_t)replace_contents_data_free);

  xfile_replace_async (file,
                        etag,
                        make_backup,
                        flags,
                        0,
                        xtask_get_cancellable (data->task),
                        replace_contents_open_callback,
                        data);
}

/**
 * xfile_replace_contents_finish:
 * @file: input #xfile_t
 * @res: a #xasync_result_t
 * @new_etag: (out) (optional) (nullable): a location of a new [entity tag][gfile-etag]
 *   for the document. This should be freed with g_free() when it is no
 *   longer needed, or %NULL
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous replace of the given @file. See
 * xfile_replace_contents_async(). Sets @new_etag to the new entity
 * tag for the document, if present.
 *
 * Returns: %TRUE on success, %FALSE on failure.
 */
xboolean_t
xfile_replace_contents_finish (xfile_t         *file,
                                xasync_result_t  *res,
                                char         **new_etag,
                                xerror_t       **error)
{
  xtask_t *task;
  ReplaceContentsData *data;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (xtask_is_valid (res, file), FALSE);

  task = XTASK (res);

  if (!xtask_propagate_boolean (task, error))
    return FALSE;

  data = xtask_get_task_data (task);

  if (new_etag)
    {
      *new_etag = data->etag;
      data->etag = NULL; /* Take ownership */
    }

  return TRUE;
}

xboolean_t
xfile_real_measure_disk_usage (xfile_t                         *file,
                                xfile_measure_flags_t              flags,
                                xcancellable_t                  *cancellable,
                                xfile_measure_progress_callback_t   progress_callback,
                                xpointer_t                       progress_data,
                                xuint64_t                       *disk_usage,
                                xuint64_t                       *num_dirs,
                                xuint64_t                       *num_files,
                                xerror_t                       **error)
{
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "Operation not supported for the current backend.");
  return FALSE;
}

typedef struct
{
  xfile_measure_flags_t             flags;
  xfile_measure_progress_callback_t  progress_callback;
  xpointer_t                      progress_data;
} MeasureTaskData;

typedef struct
{
  xuint64_t disk_usage;
  xuint64_t num_dirs;
  xuint64_t num_files;
} MeasureResult;

typedef struct
{
  xfile_measure_progress_callback_t callback;
  xpointer_t                     user_data;
  xboolean_t                     reporting;
  xuint64_t                      current_size;
  xuint64_t                      num_dirs;
  xuint64_t                      num_files;
} MeasureProgress;

static xboolean_t
measure_disk_usage_invoke_progress (xpointer_t user_data)
{
  MeasureProgress *progress = user_data;

  (* progress->callback) (progress->reporting,
                          progress->current_size, progress->num_dirs, progress->num_files,
                          progress->user_data);

  return FALSE;
}

static void
measure_disk_usage_progress (xboolean_t reporting,
                             xuint64_t  current_size,
                             xuint64_t  num_dirs,
                             xuint64_t  num_files,
                             xpointer_t user_data)
{
  MeasureProgress progress;
  xtask_t *task = user_data;
  MeasureTaskData *data;

  data = xtask_get_task_data (task);

  progress.callback = data->progress_callback;
  progress.user_data = data->progress_data;
  progress.reporting = reporting;
  progress.current_size = current_size;
  progress.num_dirs = num_dirs;
  progress.num_files = num_files;

  xmain_context_invoke_full (xtask_get_context (task),
                              xtask_get_priority (task),
                              measure_disk_usage_invoke_progress,
                              g_memdup2 (&progress, sizeof progress),
                              g_free);
}

static void
measure_disk_usage_thread (xtask_t        *task,
                           xpointer_t      source_object,
                           xpointer_t      task_data,
                           xcancellable_t *cancellable)
{
  MeasureTaskData *data = task_data;
  xerror_t *error = NULL;
  MeasureResult result = { 0, };

  if (xfile_measure_disk_usage (source_object, data->flags, cancellable,
                                 data->progress_callback ? measure_disk_usage_progress : NULL, task,
                                 &result.disk_usage, &result.num_dirs, &result.num_files,
                                 &error))
    xtask_return_pointer (task, g_memdup2 (&result, sizeof result), g_free);
  else
    xtask_return_error (task, error);
}

static void
xfile_real_measure_disk_usage_async (xfile_t                        *file,
                                      xfile_measure_flags_t             flags,
                                      xint_t                          io_priority,
                                      xcancellable_t                 *cancellable,
                                      xfile_measure_progress_callback_t  progress_callback,
                                      xpointer_t                      progress_data,
                                      xasync_ready_callback_t           callback,
                                      xpointer_t                      user_data)
{
  MeasureTaskData data;
  xtask_t *task;

  data.flags = flags;
  data.progress_callback = progress_callback;
  data.progress_data = progress_data;

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_real_measure_disk_usage_async);
  xtask_set_task_data (task, g_memdup2 (&data, sizeof data), g_free);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, measure_disk_usage_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_real_measure_disk_usage_finish (xfile_t         *file,
                                       xasync_result_t  *result,
                                       xuint64_t       *disk_usage,
                                       xuint64_t       *num_dirs,
                                       xuint64_t       *num_files,
                                       xerror_t       **error)
{
  MeasureResult *measure_result;

  g_return_val_if_fail (xtask_is_valid (result, file), FALSE);

  measure_result = xtask_propagate_pointer (XTASK (result), error);

  if (measure_result == NULL)
    return FALSE;

  if (disk_usage)
    *disk_usage = measure_result->disk_usage;

  if (num_dirs)
    *num_dirs = measure_result->num_dirs;

  if (num_files)
    *num_files = measure_result->num_files;

  g_free (measure_result);

  return TRUE;
}

/**
 * xfile_measure_disk_usage:
 * @file: a #xfile_t
 * @flags: #xfile_measure_flags_t
 * @cancellable: (nullable): optional #xcancellable_t
 * @progress_callback: (nullable): a #xfile_measure_progress_callback_t
 * @progress_data: user_data for @progress_callback
 * @disk_usage: (out) (optional): the number of bytes of disk space used
 * @num_dirs: (out) (optional): the number of directories encountered
 * @num_files: (out) (optional): the number of non-directories encountered
 * @error: (nullable): %NULL, or a pointer to a %NULL #xerror_t pointer
 *
 * Recursively measures the disk usage of @file.
 *
 * This is essentially an analog of the 'du' command, but it also
 * reports the number of directories and non-directory files encountered
 * (including things like symbolic links).
 *
 * By default, errors are only reported against the toplevel file
 * itself.  Errors found while recursing are silently ignored, unless
 * %XFILE_MEASURE_REPORT_ANY_ERROR is given in @flags.
 *
 * The returned size, @disk_usage, is in bytes and should be formatted
 * with g_format_size() in order to get something reasonable for showing
 * in a user interface.
 *
 * @progress_callback and @progress_data can be given to request
 * periodic progress updates while scanning.  See the documentation for
 * #xfile_measure_progress_callback_t for information about when and how the
 * callback will be invoked.
 *
 * Returns: %TRUE if successful, with the out parameters set.
 *   %FALSE otherwise, with @error set.
 *
 * Since: 2.38
 **/
xboolean_t
xfile_measure_disk_usage (xfile_t                         *file,
                           xfile_measure_flags_t              flags,
                           xcancellable_t                  *cancellable,
                           xfile_measure_progress_callback_t   progress_callback,
                           xpointer_t                       progress_data,
                           xuint64_t                       *disk_usage,
                           xuint64_t                       *num_dirs,
                           xuint64_t                       *num_files,
                           xerror_t                       **error)
{
  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return XFILE_GET_IFACE (file)->measure_disk_usage (file, flags, cancellable,
                                                      progress_callback, progress_data,
                                                      disk_usage, num_dirs, num_files,
                                                      error);
}

/**
 * xfile_measure_disk_usage_async:
 * @file: a #xfile_t
 * @flags: #xfile_measure_flags_t
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t
 * @progress_callback: (nullable): a #xfile_measure_progress_callback_t
 * @progress_data: user_data for @progress_callback
 * @callback: (nullable): a #xasync_ready_callback_t to call when complete
 * @user_data: the data to pass to callback function
 *
 * Recursively measures the disk usage of @file.
 *
 * This is the asynchronous version of xfile_measure_disk_usage().  See
 * there for more information.
 *
 * Since: 2.38
 **/
void
xfile_measure_disk_usage_async (xfile_t                        *file,
                                 xfile_measure_flags_t             flags,
                                 xint_t                          io_priority,
                                 xcancellable_t                 *cancellable,
                                 xfile_measure_progress_callback_t  progress_callback,
                                 xpointer_t                      progress_data,
                                 xasync_ready_callback_t           callback,
                                 xpointer_t                      user_data)
{
  g_return_if_fail (X_IS_FILE (file));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  XFILE_GET_IFACE (file)->measure_disk_usage_async (file, flags, io_priority, cancellable,
                                                     progress_callback, progress_data,
                                                     callback, user_data);
}

/**
 * xfile_measure_disk_usage_finish:
 * @file: a #xfile_t
 * @result: the #xasync_result_t passed to your #xasync_ready_callback_t
 * @disk_usage: (out) (optional): the number of bytes of disk space used
 * @num_dirs: (out) (optional): the number of directories encountered
 * @num_files: (out) (optional): the number of non-directories encountered
 * @error: (nullable): %NULL, or a pointer to a %NULL #xerror_t pointer
 *
 * Collects the results from an earlier call to
 * xfile_measure_disk_usage_async().  See xfile_measure_disk_usage() for
 * more information.
 *
 * Returns: %TRUE if successful, with the out parameters set.
 *   %FALSE otherwise, with @error set.
 *
 * Since: 2.38
 **/
xboolean_t
xfile_measure_disk_usage_finish (xfile_t         *file,
                                  xasync_result_t  *result,
                                  xuint64_t       *disk_usage,
                                  xuint64_t       *num_dirs,
                                  xuint64_t       *num_files,
                                  xerror_t       **error)
{
  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return XFILE_GET_IFACE (file)->measure_disk_usage_finish (file, result, disk_usage, num_dirs, num_files, error);
}

/**
 * xfile_start_mountable:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @start_operation: (nullable): a #xmount_operation_t, or %NULL to avoid user interaction
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request is satisfied, or %NULL
 * @user_data: the data to pass to callback function
 *
 * Starts a file of type %XFILE_TYPE_MOUNTABLE.
 * Using @start_operation, you can request callbacks when, for instance,
 * passwords are needed during authentication.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_mount_mountable_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_start_mountable (xfile_t               *file,
                        GDriveStartFlags     flags,
                        xmount_operation_t     *start_operation,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->start_mountable == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_start_mountable,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  (* iface->start_mountable) (file,
                              flags,
                              start_operation,
                              cancellable,
                              callback,
                              user_data);
}

/**
 * xfile_start_mountable_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes a start operation. See xfile_start_mountable() for details.
 *
 * Finish an asynchronous start operation that was started
 * with xfile_start_mountable().
 *
 * Returns: %TRUE if the operation finished successfully. %FALSE
 * otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xfile_start_mountable_finish (xfile_t         *file,
                               xasync_result_t  *result,
                               xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_start_mountable))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  return (* iface->start_mountable_finish) (file, result, error);
}

/**
 * xfile_stop_mountable:
 * @file: input #xfile_t
 * @flags: flags affecting the operation
 * @mount_operation: (nullable): a #xmount_operation_t,
 *   or %NULL to avoid user interaction.
 * @cancellable: (nullable): optional #xcancellable_t object,
 *   %NULL to ignore
 * @callback: (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: the data to pass to callback function
 *
 * Stops a file of type %XFILE_TYPE_MOUNTABLE.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_stop_mountable_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_stop_mountable (xfile_t               *file,
                       xmount_unmount_flags_t   flags,
                       xmount_operation_t     *mount_operation,
                       xcancellable_t        *cancellable,
                       xasync_ready_callback_t  callback,
                       xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->stop_mountable == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_stop_mountable,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  (* iface->stop_mountable) (file,
                             flags,
                             mount_operation,
                             cancellable,
                             callback,
                             user_data);
}

/**
 * xfile_stop_mountable_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes a stop operation, see xfile_stop_mountable() for details.
 *
 * Finish an asynchronous stop operation that was started
 * with xfile_stop_mountable().
 *
 * Returns: %TRUE if the operation finished successfully.
 *   %FALSE otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xfile_stop_mountable_finish (xfile_t         *file,
                              xasync_result_t  *result,
                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_stop_mountable))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  return (* iface->stop_mountable_finish) (file, result, error);
}

/**
 * xfile_poll_mountable:
 * @file: input #xfile_t
 * @cancellable: optional #xcancellable_t object, %NULL to ignore
 * @callback: (nullable): a #xasync_ready_callback_t to call
 *   when the request is satisfied, or %NULL
 * @user_data: the data to pass to callback function
 *
 * Polls a file of type %XFILE_TYPE_MOUNTABLE.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When the operation is finished, @callback will be called.
 * You can then call xfile_mount_mountable_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xfile_poll_mountable (xfile_t               *file,
                       xcancellable_t        *cancellable,
                       xasync_ready_callback_t  callback,
                       xpointer_t             user_data)
{
  xfile_iface_t *iface;

  g_return_if_fail (X_IS_FILE (file));

  iface = XFILE_GET_IFACE (file);

  if (iface->poll_mountable == NULL)
    {
      xtask_report_new_error (file, callback, user_data,
                               xfile_poll_mountable,
                               G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               _("Operation not supported"));
      return;
    }

  (* iface->poll_mountable) (file,
                             cancellable,
                             callback,
                             user_data);
}

/**
 * xfile_poll_mountable_finish:
 * @file: input #xfile_t
 * @result: a #xasync_result_t
 * @error: a #xerror_t, or %NULL
 *
 * Finishes a poll operation. See xfile_poll_mountable() for details.
 *
 * Finish an asynchronous poll operation that was polled
 * with xfile_poll_mountable().
 *
 * Returns: %TRUE if the operation finished successfully. %FALSE
 * otherwise.
 *
 * Since: 2.22
 */
xboolean_t
xfile_poll_mountable_finish (xfile_t         *file,
                              xasync_result_t  *result,
                              xerror_t       **error)
{
  xfile_iface_t *iface;

  g_return_val_if_fail (X_IS_FILE (file), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_poll_mountable))
    return xtask_propagate_boolean (XTASK (result), error);

  iface = XFILE_GET_IFACE (file);
  return (* iface->poll_mountable_finish) (file, result, error);
}

/**
 * xfile_supports_thread_contexts:
 * @file: a #xfile_t
 *
 * Checks if @file supports
 * [thread-default contexts][g-main-context-push-thread-default-context].
 * If this returns %FALSE, you cannot perform asynchronous operations on
 * @file in a thread that has a thread-default context.
 *
 * Returns: Whether or not @file supports thread-default contexts.
 *
 * Since: 2.22
 */
xboolean_t
xfile_supports_thread_contexts (xfile_t *file)
{
 xfile_iface_t *iface;

 g_return_val_if_fail (X_IS_FILE (file), FALSE);

 iface = XFILE_GET_IFACE (file);
 return iface->supports_thread_contexts;
}

/**
 * xfile_load_bytes:
 * @file: a #xfile_t
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @etag_out: (out) (nullable) (optional): a location to place the current
 *   entity tag for the file, or %NULL if the entity tag is not needed
 * @error: a location for a #xerror_t or %NULL
 *
 * Loads the contents of @file and returns it as #xbytes_t.
 *
 * If @file is a resource:// based URI, the resulting bytes will reference the
 * embedded resource instead of a copy. Otherwise, this is equivalent to calling
 * xfile_load_contents() and xbytes_new_take().
 *
 * For resources, @etag_out will be set to %NULL.
 *
 * The data contained in the resulting #xbytes_t is always zero-terminated, but
 * this is not included in the #xbytes_t length. The resulting #xbytes_t should be
 * freed with xbytes_unref() when no longer in use.
 *
 * Returns: (transfer full): a #xbytes_t or %NULL and @error is set
 *
 * Since: 2.56
 */
xbytes_t *
xfile_load_bytes (xfile_t         *file,
                   xcancellable_t  *cancellable,
                   xchar_t        **etag_out,
                   xerror_t       **error)
{
  xchar_t *contents;
  xsize_t len;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (etag_out != NULL)
    *etag_out = NULL;

  if (xfile_has_uri_scheme (file, "resource"))
    {
      xbytes_t *bytes;
      xchar_t *uri, *unescaped;

      uri = xfile_get_uri (file);
      unescaped = xuri_unescape_string (uri + strlen ("resource://"), NULL);
      g_free (uri);

      bytes = g_resources_lookup_data (unescaped, G_RESOURCE_LOOKUP_FLAGS_NONE, error);
      g_free (unescaped);

      return bytes;
    }

  /* contents is guaranteed to be \0 terminated */
  if (xfile_load_contents (file, cancellable, &contents, &len, etag_out, error))
    return xbytes_new_take (g_steal_pointer (&contents), len);

  return NULL;
}

static void
xfile_load_bytes_cb (xobject_t      *object,
                      xasync_result_t *result,
                      xpointer_t      user_data)
{
  xfile_t *file = XFILE (object);
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xchar_t *etag = NULL;
  xchar_t *contents = NULL;
  xsize_t len = 0;

  xfile_load_contents_finish (file, result, &contents, &len, &etag, &error);
  xtask_set_task_data (task, g_steal_pointer (&etag), g_free);

  if (error != NULL)
    xtask_return_error (task, g_steal_pointer (&error));
  else
    xtask_return_pointer (task,
                           xbytes_new_take (g_steal_pointer (&contents), len),
                           (xdestroy_notify_t)xbytes_unref);

  xobject_unref (task);
}

/**
 * xfile_load_bytes_async:
 * @file: a #xfile_t
 * @cancellable: (nullable): a #xcancellable_t or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t to call when the
 *   request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously loads the contents of @file as #xbytes_t.
 *
 * If @file is a resource:// based URI, the resulting bytes will reference the
 * embedded resource instead of a copy. Otherwise, this is equivalent to calling
 * xfile_load_contents_async() and xbytes_new_take().
 *
 * @callback should call xfile_load_bytes_finish() to get the result of this
 * asynchronous operation.
 *
 * See xfile_load_bytes() for more information.
 *
 * Since: 2.56
 */
void
xfile_load_bytes_async (xfile_t               *file,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xerror_t *error = NULL;
  xbytes_t *bytes;
  xtask_t *task;

  g_return_if_fail (X_IS_FILE (file));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  task = xtask_new (file, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_load_bytes_async);

  if (!xfile_has_uri_scheme (file, "resource"))
    {
      xfile_load_contents_async (file,
                                  cancellable,
                                  xfile_load_bytes_cb,
                                  g_steal_pointer (&task));
      return;
    }

  bytes = xfile_load_bytes (file, cancellable, NULL, &error);

  if (bytes == NULL)
    xtask_return_error (task, g_steal_pointer (&error));
  else
    xtask_return_pointer (task,
                           g_steal_pointer (&bytes),
                           (xdestroy_notify_t)xbytes_unref);

  xobject_unref (task);
}

/**
 * xfile_load_bytes_finish:
 * @file: a #xfile_t
 * @result: a #xasync_result_t provided to the callback
 * @etag_out: (out) (nullable) (optional): a location to place the current
 *   entity tag for the file, or %NULL if the entity tag is not needed
 * @error: a location for a #xerror_t, or %NULL
 *
 * Completes an asynchronous request to xfile_load_bytes_async().
 *
 * For resources, @etag_out will be set to %NULL.
 *
 * The data contained in the resulting #xbytes_t is always zero-terminated, but
 * this is not included in the #xbytes_t length. The resulting #xbytes_t should be
 * freed with xbytes_unref() when no longer in use.
 *
 * See xfile_load_bytes() for more information.
 *
 * Returns: (transfer full): a #xbytes_t or %NULL and @error is set
 *
 * Since: 2.56
 */
xbytes_t *
xfile_load_bytes_finish (xfile_t         *file,
                          xasync_result_t  *result,
                          xchar_t        **etag_out,
                          xerror_t       **error)
{
  xbytes_t *bytes;

  g_return_val_if_fail (X_IS_FILE (file), NULL);
  g_return_val_if_fail (X_IS_TASK (result), NULL);
  g_return_val_if_fail (xtask_is_valid (XTASK (result), file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  bytes = xtask_propagate_pointer (XTASK (result), error);

  if (etag_out != NULL)
    *etag_out = xstrdup (xtask_get_task_data (XTASK (result)));

  return bytes;
}
