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
#include "gfileenumerator.h"
#include "gfile.h"
#include "gioscheduler.h"
#include "gasyncresult.h"
#include "gasynchelper.h"
#include "gioerror.h"
#include "glibintl.h"

struct _xfile_enumerator_private_t {
  /* TODO: Should be public for subclasses? */
  xfile_t *container;
  xuint_t closed : 1;
  xuint_t pending : 1;
  xasync_ready_callback_t outstanding_callback;
  xerror_t *outstandinxerror;
};

/**
 * SECTION:gfileenumerator
 * @short_description: Enumerated Files Routines
 * @include: gio/gio.h
 *
 * #xfile_enumerator_t allows you to operate on a set of #GFiles,
 * returning a #xfile_info_t structure for each file enumerated (e.g.
 * xfile_enumerate_children() will return a #xfile_enumerator_t for each
 * of the children within a directory).
 *
 * To get the next file's information from a #xfile_enumerator_t, use
 * xfile_enumerator_next_file() or its asynchronous version,
 * xfile_enumerator_next_files_async(). Note that the asynchronous
 * version will return a list of #GFileInfos, whereas the
 * synchronous will only return the next file in the enumerator.
 *
 * The ordering of returned files is unspecified for non-Unix
 * platforms; for more information, see g_dir_read_name().  On Unix,
 * when operating on local files, returned files will be sorted by
 * inode number.  Effectively you can assume that the ordering of
 * returned files will be stable between successive calls (and
 * applications) assuming the directory is unchanged.
 *
 * If your application needs a specific ordering, such as by name or
 * modification time, you will have to implement that in your
 * application code.
 *
 * To close a #xfile_enumerator_t, use xfile_enumerator_close(), or
 * its asynchronous version, xfile_enumerator_close_async(). Once
 * a #xfile_enumerator_t is closed, no further actions may be performed
 * on it, and it should be freed with xobject_unref().
 *
 **/

G_DEFINE_TYPE_WITH_PRIVATE (xfile_enumerator_t, xfile_enumerator, XTYPE_OBJECT)

enum {
  PROP_0,
  PROP_CONTAINER
};

static void     xfile_enumerator_real_next_files_async  (xfile_enumerator_t      *enumerator,
							  int                   num_files,
							  int                   io_priority,
							  xcancellable_t         *cancellable,
							  xasync_ready_callback_t   callback,
							  xpointer_t              user_data);
static xlist_t *  xfile_enumerator_real_next_files_finish (xfile_enumerator_t      *enumerator,
							  xasync_result_t         *res,
							  xerror_t              **error);
static void     xfile_enumerator_real_close_async       (xfile_enumerator_t      *enumerator,
							  int                   io_priority,
							  xcancellable_t         *cancellable,
							  xasync_ready_callback_t   callback,
							  xpointer_t              user_data);
static xboolean_t xfile_enumerator_real_close_finish      (xfile_enumerator_t      *enumerator,
							  xasync_result_t         *res,
							  xerror_t              **error);

static void
xfile_enumerator_set_property (xobject_t      *object,
                                xuint_t         property_id,
                                const xvalue_t *value,
                                xparam_spec_t   *pspec)
{
  xfile_enumerator_t *enumerator;

  enumerator = XFILE_ENUMERATOR (object);

  switch (property_id) {
  case PROP_CONTAINER:
    enumerator->priv->container = xvalue_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
xfile_enumerator_dispose (xobject_t *object)
{
  xfile_enumerator_t *enumerator;

  enumerator = XFILE_ENUMERATOR (object);

  if (enumerator->priv->container) {
    xobject_unref (enumerator->priv->container);
    enumerator->priv->container = NULL;
  }

  G_OBJECT_CLASS (xfile_enumerator_parent_class)->dispose (object);
}

static void
xfile_enumerator_finalize (xobject_t *object)
{
  xfile_enumerator_t *enumerator;

  enumerator = XFILE_ENUMERATOR (object);

  if (!enumerator->priv->closed)
    xfile_enumerator_close (enumerator, NULL, NULL);

  G_OBJECT_CLASS (xfile_enumerator_parent_class)->finalize (object);
}

static void
xfile_enumerator_class_init (xfile_enumerator_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = xfile_enumerator_set_property;
  gobject_class->dispose = xfile_enumerator_dispose;
  gobject_class->finalize = xfile_enumerator_finalize;

  klass->next_files_async = xfile_enumerator_real_next_files_async;
  klass->next_files_finish = xfile_enumerator_real_next_files_finish;
  klass->close_async = xfile_enumerator_real_close_async;
  klass->close_finish = xfile_enumerator_real_close_finish;

  xobject_class_install_property
    (gobject_class, PROP_CONTAINER,
     g_param_spec_object ("container", P_("Container"),
                          P_("The container that is being enumerated"),
                          XTYPE_FILE,
                          G_PARAM_WRITABLE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));
}

static void
xfile_enumerator_init (xfile_enumerator_t *enumerator)
{
  enumerator->priv = xfile_enumerator_get_instance_private (enumerator);
}

/**
 * xfile_enumerator_next_file:
 * @enumerator: a #xfile_enumerator_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Returns information for the next file in the enumerated object.
 * Will block until the information is available. The #xfile_info_t
 * returned from this function will contain attributes that match the
 * attribute string that was passed when the #xfile_enumerator_t was created.
 *
 * See the documentation of #xfile_enumerator_t for information about the
 * order of returned files.
 *
 * On error, returns %NULL and sets @error to the error. If the
 * enumerator is at the end, %NULL will be returned and @error will
 * be unset.
 *
 * Returns: (nullable) (transfer full): A #xfile_info_t or %NULL on error
 *    or end of enumerator.  Free the returned object with
 *    xobject_unref() when no longer needed.
 **/
xfile_info_t *
xfile_enumerator_next_file (xfile_enumerator_t *enumerator,
			     xcancellable_t *cancellable,
			     xerror_t **error)
{
  xfile_enumerator_class_t *class;
  xfile_info_t *info;

  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), NULL);
  g_return_val_if_fail (enumerator != NULL, NULL);

  if (enumerator->priv->closed)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                           _("Enumerator is closed"));
      return NULL;
    }

  if (enumerator->priv->pending)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
                           _("File enumerator has outstanding operation"));
      return NULL;
    }

  if (enumerator->priv->outstandinxerror)
    {
      g_propagate_error (error, enumerator->priv->outstandinxerror);
      enumerator->priv->outstandinxerror = NULL;
      return NULL;
    }

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);

  if (cancellable)
    xcancellable_push_current (cancellable);

  enumerator->priv->pending = TRUE;
  info = (* class->next_file) (enumerator, cancellable, error);
  enumerator->priv->pending = FALSE;

  if (cancellable)
    xcancellable_pop_current (cancellable);

  return info;
}

/**
 * xfile_enumerator_close:
 * @enumerator: a #xfile_enumerator_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Releases all resources used by this enumerator, making the
 * enumerator return %G_IO_ERROR_CLOSED on all calls.
 *
 * This will be automatically called when the last reference
 * is dropped, but you might want to call this function to make
 * sure resources are released as early as possible.
 *
 * Returns: #TRUE on success or #FALSE on error.
 **/
xboolean_t
xfile_enumerator_close (xfile_enumerator_t  *enumerator,
			 xcancellable_t     *cancellable,
			 xerror_t          **error)
{
  xfile_enumerator_class_t *class;

  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), FALSE);
  g_return_val_if_fail (enumerator != NULL, FALSE);

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);

  if (enumerator->priv->closed)
    return TRUE;

  if (enumerator->priv->pending)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
                           _("File enumerator has outstanding operation"));
      return FALSE;
    }

  if (cancellable)
    xcancellable_push_current (cancellable);

  enumerator->priv->pending = TRUE;
  (* class->close_fn) (enumerator, cancellable, error);
  enumerator->priv->pending = FALSE;
  enumerator->priv->closed = TRUE;

  if (cancellable)
    xcancellable_pop_current (cancellable);

  return TRUE;
}

static void
next_async_callback_wrapper (xobject_t      *source_object,
			     xasync_result_t *res,
			     xpointer_t      user_data)
{
  xfile_enumerator_t *enumerator = XFILE_ENUMERATOR (source_object);

  enumerator->priv->pending = FALSE;
  if (enumerator->priv->outstanding_callback)
    (*enumerator->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (enumerator);
}

/**
 * xfile_enumerator_next_files_async:
 * @enumerator: a #xfile_enumerator_t.
 * @num_files: the number of file info objects to request
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Request information for a number of files from the enumerator asynchronously.
 * When all i/o for the operation is finished the @callback will be called with
 * the requested information.
 *
 * See the documentation of #xfile_enumerator_t for information about the
 * order of returned files.
 *
 * The callback can be called with less than @num_files files in case of error
 * or at the end of the enumerator. In case of a partial error the callback will
 * be called with any succeeding items and no error, and on the next request the
 * error will be reported. If a request is cancelled the callback will be called
 * with %G_IO_ERROR_CANCELLED.
 *
 * During an async request no other sync and async calls are allowed, and will
 * result in %G_IO_ERROR_PENDING errors.
 *
 * Any outstanding i/o request with higher priority (lower numerical value) will
 * be executed before an outstanding request with lower priority. Default
 * priority is %G_PRIORITY_DEFAULT.
 **/
void
xfile_enumerator_next_files_async (xfile_enumerator_t     *enumerator,
				    int                  num_files,
				    int                  io_priority,
				    xcancellable_t        *cancellable,
				    xasync_ready_callback_t  callback,
				    xpointer_t             user_data)
{
  xfile_enumerator_class_t *class;

  g_return_if_fail (X_IS_FILE_ENUMERATOR (enumerator));
  g_return_if_fail (enumerator != NULL);
  g_return_if_fail (num_files >= 0);

  if (num_files == 0)
    {
      xtask_t *task;

      task = xtask_new (enumerator, cancellable, callback, user_data);
      xtask_set_source_tag (task, xfile_enumerator_next_files_async);
      xtask_return_pointer (task, NULL, NULL);
      xobject_unref (task);
      return;
    }

  if (enumerator->priv->closed)
    {
      xtask_report_new_error (enumerator, callback, user_data,
                               xfile_enumerator_next_files_async,
                               G_IO_ERROR, G_IO_ERROR_CLOSED,
                               _("File enumerator is already closed"));
      return;
    }

  if (enumerator->priv->pending)
    {
      xtask_report_new_error (enumerator, callback, user_data,
                               xfile_enumerator_next_files_async,
                               G_IO_ERROR, G_IO_ERROR_PENDING,
                               _("File enumerator has outstanding operation"));
      return;
    }

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);

  enumerator->priv->pending = TRUE;
  enumerator->priv->outstanding_callback = callback;
  xobject_ref (enumerator);
  (* class->next_files_async) (enumerator, num_files, io_priority, cancellable,
			       next_async_callback_wrapper, user_data);
}

/**
 * xfile_enumerator_next_files_finish:
 * @enumerator: a #xfile_enumerator_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes the asynchronous operation started with xfile_enumerator_next_files_async().
 *
 * Returns: (transfer full) (element-type Gio.FileInfo): a #xlist_t of #GFileInfos. You must free the list with
 *     xlist_free() and unref the infos with xobject_unref() when you're
 *     done with them.
 **/
xlist_t *
xfile_enumerator_next_files_finish (xfile_enumerator_t  *enumerator,
				     xasync_result_t     *result,
				     xerror_t          **error)
{
  xfile_enumerator_class_t *class;

  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, xfile_enumerator_next_files_async))
    return xtask_propagate_pointer (XTASK (result), error);

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);
  return class->next_files_finish (enumerator, result, error);
}

static void
close_async_callback_wrapper (xobject_t      *source_object,
			      xasync_result_t *res,
			      xpointer_t      user_data)
{
  xfile_enumerator_t *enumerator = XFILE_ENUMERATOR (source_object);

  enumerator->priv->pending = FALSE;
  enumerator->priv->closed = TRUE;
  if (enumerator->priv->outstanding_callback)
    (*enumerator->priv->outstanding_callback) (source_object, res, user_data);
  xobject_unref (enumerator);
}

/**
 * xfile_enumerator_close_async:
 * @enumerator: a #xfile_enumerator_t.
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously closes the file enumerator.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned in
 * xfile_enumerator_close_finish().
 **/
void
xfile_enumerator_close_async (xfile_enumerator_t     *enumerator,
			       int                  io_priority,
			       xcancellable_t        *cancellable,
			       xasync_ready_callback_t  callback,
			       xpointer_t             user_data)
{
  xfile_enumerator_class_t *class;

  g_return_if_fail (X_IS_FILE_ENUMERATOR (enumerator));

  if (enumerator->priv->closed)
    {
      xtask_report_new_error (enumerator, callback, user_data,
                               xfile_enumerator_close_async,
                               G_IO_ERROR, G_IO_ERROR_CLOSED,
                               _("File enumerator is already closed"));
      return;
    }

  if (enumerator->priv->pending)
    {
      xtask_report_new_error (enumerator, callback, user_data,
                               xfile_enumerator_close_async,
                               G_IO_ERROR, G_IO_ERROR_PENDING,
                               _("File enumerator has outstanding operation"));
      return;
    }

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);

  enumerator->priv->pending = TRUE;
  enumerator->priv->outstanding_callback = callback;
  xobject_ref (enumerator);
  (* class->close_async) (enumerator, io_priority, cancellable,
			  close_async_callback_wrapper, user_data);
}

/**
 * xfile_enumerator_close_finish:
 * @enumerator: a #xfile_enumerator_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes closing a file enumerator, started from xfile_enumerator_close_async().
 *
 * If the file enumerator was already closed when xfile_enumerator_close_async()
 * was called, then this function will report %G_IO_ERROR_CLOSED in @error, and
 * return %FALSE. If the file enumerator had pending operation when the close
 * operation was started, then this function will report %G_IO_ERROR_PENDING, and
 * return %FALSE.  If @cancellable was not %NULL, then the operation may have been
 * cancelled by triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be set, and %FALSE will be
 * returned.
 *
 * Returns: %TRUE if the close operation has finished successfully.
 **/
xboolean_t
xfile_enumerator_close_finish (xfile_enumerator_t  *enumerator,
				xasync_result_t     *result,
				xerror_t          **error)
{
  xfile_enumerator_class_t *class;

  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), FALSE);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), FALSE);

  if (xasync_result_legacy_propagate_error (result, error))
    return FALSE;
  else if (xasync_result_is_tagged (result, xfile_enumerator_close_async))
    return xtask_propagate_boolean (XTASK (result), error);

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);
  return class->close_finish (enumerator, result, error);
}

/**
 * xfile_enumerator_is_closed:
 * @enumerator: a #xfile_enumerator_t.
 *
 * Checks if the file enumerator has been closed.
 *
 * Returns: %TRUE if the @enumerator is closed.
 **/
xboolean_t
xfile_enumerator_is_closed (xfile_enumerator_t *enumerator)
{
  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), TRUE);

  return enumerator->priv->closed;
}

/**
 * xfile_enumerator_has_pending:
 * @enumerator: a #xfile_enumerator_t.
 *
 * Checks if the file enumerator has pending operations.
 *
 * Returns: %TRUE if the @enumerator has pending operations.
 **/
xboolean_t
xfile_enumerator_has_pending (xfile_enumerator_t *enumerator)
{
  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), TRUE);

  return enumerator->priv->pending;
}

/**
 * xfile_enumerator_set_pending:
 * @enumerator: a #xfile_enumerator_t.
 * @pending: a boolean value.
 *
 * Sets the file enumerator as having pending operations.
 **/
void
xfile_enumerator_set_pending (xfile_enumerator_t *enumerator,
			       xboolean_t         pending)
{
  g_return_if_fail (X_IS_FILE_ENUMERATOR (enumerator));

  enumerator->priv->pending = pending;
}

/**
 * xfile_enumerator_iterate:
 * @direnum: an open #xfile_enumerator_t
 * @out_info: (out) (transfer none) (optional): Output location for the next #xfile_info_t, or %NULL
 * @out_child: (out) (transfer none) (optional): Output location for the next #xfile_t, or %NULL
 * @cancellable: a #xcancellable_t
 * @error: a #xerror_t
 *
 * This is a version of xfile_enumerator_next_file() that's easier to
 * use correctly from C programs.  With xfile_enumerator_next_file(),
 * the xboolean_t return value signifies "end of iteration or error", which
 * requires allocation of a temporary #xerror_t.
 *
 * In contrast, with this function, a %FALSE return from
 * xfile_enumerator_iterate() *always* means
 * "error".  End of iteration is signaled by @out_info or @out_child being %NULL.
 *
 * Another crucial difference is that the references for @out_info and
 * @out_child are owned by @direnum (they are cached as hidden
 * properties).  You must not unref them in your own code.  This makes
 * memory management significantly easier for C code in combination
 * with loops.
 *
 * Finally, this function optionally allows retrieving a #xfile_t as
 * well.
 *
 * You must specify at least one of @out_info or @out_child.
 *
 * The code pattern for correctly using xfile_enumerator_iterate() from C
 * is:
 *
 * |[
 * direnum = xfile_enumerate_children (file, ...);
 * while (TRUE)
 *   {
 *     xfile_info_t *info;
 *     if (!xfile_enumerator_iterate (direnum, &info, NULL, cancellable, error))
 *       goto out;
 *     if (!info)
 *       break;
 *     ... do stuff with "info"; do not unref it! ...
 *   }
 *
 * out:
 *   xobject_unref (direnum); // Note: frees the last @info
 * ]|
 *
 *
 * Since: 2.44
 */
xboolean_t
xfile_enumerator_iterate (xfile_enumerator_t  *direnum,
                           xfile_info_t       **out_info,
                           xfile_t           **out_child,
                           xcancellable_t     *cancellable,
                           xerror_t          **error)
{
  xboolean_t ret = FALSE;
  xerror_t *temp_error = NULL;
  xfile_info_t *ret_info = NULL;

  static xquark cached_info_quark;
  static xquark cached_child_quark;
  static xsize_t quarks_initialized;

  g_return_val_if_fail (direnum != NULL, FALSE);
  g_return_val_if_fail (out_info != NULL || out_child != NULL, FALSE);

  if (g_once_init_enter (&quarks_initialized))
    {
      cached_info_quark = g_quark_from_static_string ("g-cached-info");
      cached_child_quark = g_quark_from_static_string ("g-cached-child");
      g_once_init_leave (&quarks_initialized, 1);
    }

  ret_info = xfile_enumerator_next_file (direnum, cancellable, &temp_error);
  if (temp_error != NULL)
    {
      g_propagate_error (error, temp_error);
      goto out;
    }

  if (ret_info)
    {
      if (out_child != NULL)
        {
          const char *name = xfile_info_get_name (ret_info);

          if (G_UNLIKELY (name == NULL))
            {
              g_critical ("xfile_enumerator_iterate() created without standard::name");
              g_return_val_if_reached (FALSE);
            }
          else
            {
              *out_child = xfile_get_child (xfile_enumerator_get_container (direnum), name);
              xobject_set_qdata_full ((xobject_t*)direnum, cached_child_quark, *out_child, (xdestroy_notify_t)xobject_unref);
            }
        }
      if (out_info != NULL)
        {
          xobject_set_qdata_full ((xobject_t*)direnum, cached_info_quark, ret_info, (xdestroy_notify_t)xobject_unref);
          *out_info = ret_info;
        }
      else
        xobject_unref (ret_info);
    }
  else
    {
      if (out_info)
        *out_info = NULL;
      if (out_child)
        *out_child = NULL;
    }

  ret = TRUE;
 out:
  return ret;
}

/**
 * xfile_enumerator_get_container:
 * @enumerator: a #xfile_enumerator_t
 *
 * Get the #xfile_t container which is being enumerated.
 *
 * Returns: (transfer none): the #xfile_t which is being enumerated.
 *
 * Since: 2.18
 */
xfile_t *
xfile_enumerator_get_container (xfile_enumerator_t *enumerator)
{
  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), NULL);

  return enumerator->priv->container;
}

/**
 * xfile_enumerator_get_child:
 * @enumerator: a #xfile_enumerator_t
 * @info: a #xfile_info_t gotten from xfile_enumerator_next_file()
 *   or the async equivalents.
 *
 * Return a new #xfile_t which refers to the file named by @info in the source
 * directory of @enumerator.  This function is primarily intended to be used
 * inside loops with xfile_enumerator_next_file().
 *
 * To use this, %XFILE_ATTRIBUTE_STANDARD_NAME must have been listed in the
 * attributes list used when creating the #xfile_enumerator_t.
 *
 * This is a convenience method that's equivalent to:
 * |[<!-- language="C" -->
 *   xchar_t *name = xfile_info_get_name (info);
 *   xfile_t *child = xfile_get_child (xfile_enumerator_get_container (enumr),
 *                                    name);
 * ]|
 *
 * Returns: (transfer full): a #xfile_t for the #xfile_info_t passed it.
 *
 * Since: 2.36
 */
xfile_t *
xfile_enumerator_get_child (xfile_enumerator_t *enumerator,
                             xfile_info_t       *info)
{
  const xchar_t *name;

  g_return_val_if_fail (X_IS_FILE_ENUMERATOR (enumerator), NULL);
  g_return_val_if_fail (X_IS_FILE_INFO (info), NULL);

  name = xfile_info_get_name (info);

  if (G_UNLIKELY (name == NULL))
    {
      g_critical ("xfile_enumerator_t created without standard::name");
      g_return_val_if_reached (NULL);
    }

  return xfile_get_child (enumerator->priv->container, name);
}

static void
next_async_op_free (xlist_t *files)
{
  xlist_free_full (files, xobject_unref);
}

static void
next_files_thread (xtask_t        *task,
                   xpointer_t      source_object,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  xfile_enumerator_t *enumerator = source_object;
  int num_files = GPOINTER_TO_INT (task_data);
  xfile_enumerator_class_t *class;
  xlist_t *files = NULL;
  xerror_t *error = NULL;
  xfile_info_t *info;
  int i;

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);

  for (i = 0; i < num_files; i++)
    {
      if (xcancellable_set_error_if_cancelled (cancellable, &error))
	info = NULL;
      else
	info = class->next_file (enumerator, cancellable, &error);

      if (info == NULL)
	{
	  /* If we get an error after first file, return that on next operation */
	  if (error != NULL && i > 0)
	    {
	      if (xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
		xerror_free (error); /* Never propagate cancel errors to other call */
	      else
		enumerator->priv->outstandinxerror = error;
	      error = NULL;
	    }

	  break;
	}
      else
	files = xlist_prepend (files, info);
    }

  if (error)
    {
      xlist_free_full (files, xobject_unref);
      xtask_return_error (task, error);
    }
  else
    xtask_return_pointer (task, files, (xdestroy_notify_t)next_async_op_free);
}

static void
xfile_enumerator_real_next_files_async (xfile_enumerator_t     *enumerator,
					 int                  num_files,
					 int                  io_priority,
					 xcancellable_t        *cancellable,
					 xasync_ready_callback_t  callback,
					 xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (enumerator, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_enumerator_real_next_files_async);
  xtask_set_task_data (task, GINT_TO_POINTER (num_files), NULL);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, next_files_thread);
  xobject_unref (task);
}

static xlist_t *
xfile_enumerator_real_next_files_finish (xfile_enumerator_t                *enumerator,
					  xasync_result_t                   *result,
					  xerror_t                        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, enumerator), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static void
close_async_thread (xtask_t        *task,
                    xpointer_t      source_object,
                    xpointer_t      task_data,
                    xcancellable_t *cancellable)
{
  xfile_enumerator_t *enumerator = source_object;
  xfile_enumerator_class_t *class;
  xerror_t *error = NULL;
  xboolean_t result;

  class = XFILE_ENUMERATOR_GET_CLASS (enumerator);
  result = class->close_fn (enumerator, cancellable, &error);
  if (result)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
}

static void
xfile_enumerator_real_close_async (xfile_enumerator_t     *enumerator,
				    int                  io_priority,
				    xcancellable_t        *cancellable,
				    xasync_ready_callback_t  callback,
				    xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (enumerator, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_enumerator_real_close_async);
  xtask_set_priority (task, io_priority);

  xtask_run_in_thread (task, close_async_thread);
  xobject_unref (task);
}

static xboolean_t
xfile_enumerator_real_close_finish (xfile_enumerator_t  *enumerator,
                                     xasync_result_t     *result,
                                     xerror_t          **error)
{
  g_return_val_if_fail (xtask_is_valid (result, enumerator), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}
