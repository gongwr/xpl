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
#include "gicon.h"
#include "gloadableicon.h"
#include "gasyncresult.h"
#include "gtask.h"
#include "glibintl.h"


/**
 * SECTION:gloadableicon
 * @short_description: Loadable Icons
 * @include: gio/gio.h
 * @see_also: #xicon_t, #xthemed_icon_t
 *
 * Extends the #xicon_t interface and adds the ability to
 * load icons from streams.
 **/

static void          g_loadable_icon_real_load_async  (xloadable_icon_t        *icon,
						       int                   size,
						       xcancellable_t         *cancellable,
						       xasync_ready_callback_t   callback,
						       xpointer_t              user_data);
static xinput_stream_t *g_loadable_icon_real_load_finish (xloadable_icon_t        *icon,
						       xasync_result_t         *res,
						       char                **type,
						       xerror_t              **error);

typedef xloadable_icon_iface_t GLoadableIconInterface;
G_DEFINE_INTERFACE(xloadable_icon_t, g_loadable_icon, XTYPE_ICON)

static void
g_loadable_icon_default_init (xloadable_icon_iface_t *iface)
{
  iface->load_async = g_loadable_icon_real_load_async;
  iface->load_finish = g_loadable_icon_real_load_finish;
}

/**
 * g_loadable_icon_load:
 * @icon: a #xloadable_icon_t.
 * @size: an integer.
 * @type: (out) (optional): a location to store the type of the loaded
 * icon, %NULL to ignore.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to
 * ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL
 * to ignore.
 *
 * Loads a loadable icon. For the asynchronous version of this function,
 * see g_loadable_icon_load_async().
 *
 * Returns: (transfer full): a #xinput_stream_t to read the icon from.
 **/
xinput_stream_t *
g_loadable_icon_load (xloadable_icon_t  *icon,
		      int             size,
		      char          **type,
		      xcancellable_t   *cancellable,
		      xerror_t        **error)
{
  xloadable_icon_iface_t *iface;

  g_return_val_if_fail (X_IS_LOADABLE_ICON (icon), NULL);

  iface = G_LOADABLE_ICON_GET_IFACE (icon);

  return (* iface->load) (icon, size, type, cancellable, error);
}

/**
 * g_loadable_icon_load_async:
 * @icon: a #xloadable_icon_t.
 * @size: an integer.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the
 *            request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Loads an icon asynchronously. To finish this function, see
 * g_loadable_icon_load_finish(). For the synchronous, blocking
 * version of this function, see g_loadable_icon_load().
 **/
void
g_loadable_icon_load_async (xloadable_icon_t       *icon,
                            int                  size,
                            xcancellable_t        *cancellable,
                            xasync_ready_callback_t  callback,
                            xpointer_t             user_data)
{
  xloadable_icon_iface_t *iface;

  g_return_if_fail (X_IS_LOADABLE_ICON (icon));

  iface = G_LOADABLE_ICON_GET_IFACE (icon);

  (* iface->load_async) (icon, size, cancellable, callback, user_data);
}

/**
 * g_loadable_icon_load_finish:
 * @icon: a #xloadable_icon_t.
 * @res: a #xasync_result_t.
 * @type: (out) (optional): a location to store the type of the loaded
 *        icon, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an asynchronous icon load started in g_loadable_icon_load_async().
 *
 * Returns: (transfer full): a #xinput_stream_t to read the icon from.
 **/
xinput_stream_t *
g_loadable_icon_load_finish (xloadable_icon_t  *icon,
			     xasync_result_t   *res,
			     char          **type,
			     xerror_t        **error)
{
  xloadable_icon_iface_t *iface;

  g_return_val_if_fail (X_IS_LOADABLE_ICON (icon), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (res), NULL);

  if (xasync_result_legacy_propagate_error (res, error))
    return NULL;

  iface = G_LOADABLE_ICON_GET_IFACE (icon);

  return (* iface->load_finish) (icon, res, type, error);
}

/********************************************
 *   Default implementation of async load   *
 ********************************************/

typedef struct {
  int size;
  char *type;
} LoadData;

static void
load_data_free (LoadData *data)
{
  g_free (data->type);
  g_free (data);
}

static void
load_async_thread (xtask_t        *task,
                   xpointer_t      source_object,
                   xpointer_t      task_data,
                   xcancellable_t *cancellable)
{
  xloadable_icon_t *icon = source_object;
  LoadData *data = task_data;
  xloadable_icon_iface_t *iface;
  xinput_stream_t *stream;
  xerror_t *error = NULL;

  iface = G_LOADABLE_ICON_GET_IFACE (icon);
  stream = iface->load (icon, data->size, &data->type,
                        cancellable, &error);

  if (stream)
    xtask_return_pointer (task, stream, xobject_unref);
  else
    xtask_return_error (task, error);
}



static void
g_loadable_icon_real_load_async (xloadable_icon_t       *icon,
				 int                  size,
				 xcancellable_t        *cancellable,
				 xasync_ready_callback_t  callback,
				 xpointer_t             user_data)
{
  xtask_t *task;
  LoadData *data;

  task = xtask_new (icon, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_loadable_icon_real_load_async);
  data = g_new0 (LoadData, 1);
  xtask_set_task_data (task, data, (xdestroy_notify_t) load_data_free);
  xtask_run_in_thread (task, load_async_thread);
  xobject_unref (task);
}

static xinput_stream_t *
g_loadable_icon_real_load_finish (xloadable_icon_t        *icon,
				  xasync_result_t         *res,
				  char                **type,
				  xerror_t              **error)
{
  xtask_t *task;
  LoadData *data;
  xinput_stream_t *stream;

  g_return_val_if_fail (xtask_is_valid (res, icon), NULL);

  task = XTASK (res);
  data = xtask_get_task_data (task);

  stream = xtask_propagate_pointer (task, error);
  if (stream && type)
    {
      *type = data->type;
      data->type = NULL;
    }

  return stream;
}
