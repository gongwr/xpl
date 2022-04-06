/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2017 Red Hat, Inc.
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
 */

#include "config.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "gopenuriportal.h"
#include "xdp-dbus.h"
#include "gstdio.h"

#ifdef G_OS_UNIX
#include "gunixfdlist.h"
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#else
#define HAVE_O_CLOEXEC 1
#endif


static GXdpOpenURI *openuri;

static xboolean_t
init_openuri_portal (void)
{
  static xsize_t openuri_inited = 0;

  if (g_once_init_enter (&openuri_inited))
    {
      xerror_t *error = NULL;
      xdbus_connection_t *connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

      if (connection != NULL)
        {
          openuri = gxdp_open_uri_proxy_new_sync (connection, 0,
                                                  "org.freedesktop.portal.Desktop",
                                                  "/org/freedesktop/portal/desktop",
                                                  NULL, &error);
          if (openuri == NULL)
            {
              g_warning ("Cannot create document portal proxy: %s", error->message);
              xerror_free (error);
            }

          xobject_unref (connection);
        }
      else
        {
          g_warning ("Cannot connect to session bus when initializing document portal: %s",
                     error->message);
          xerror_free (error);
        }

      g_once_init_leave (&openuri_inited, 1);
    }

  return openuri != NULL;
}

xboolean_t
g_openuri_portal_open_uri (const char  *uri,
                           const char  *parent_window,
                           xerror_t     **error)
{
  xfile_t *file = NULL;
  xvariant_builder_t opt_builder;
  xboolean_t res;

  if (!init_openuri_portal ())
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "OpenURI portal is not available");
      return FALSE;
    }

  xvariant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);

  file = xfile_new_for_uri (uri);
  if (xfile_is_native (file))
    {
      char *path = NULL;
      xunix_fd_list_t *fd_list = NULL;
      int fd, fd_id, errsv;

      path = xfile_get_path (file);

      fd = g_open (path, O_RDONLY | O_CLOEXEC);
      errsv = errno;
      if (fd == -1)
        {
	  g_free (path);
	  xvariant_builder_clear (&opt_builder);
          g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                       "Failed to open '%s'", path);
          return FALSE;
        }

#ifndef HAVE_O_CLOEXEC
      fcntl (fd, F_SETFD, FD_CLOEXEC);
#endif
      fd_list = g_unix_fd_list_new_from_array (&fd, 1);
      fd = -1;
      fd_id = 0;

      res = gxdp_open_uri_call_open_file_sync (openuri,
                                               parent_window ? parent_window : "",
                                               xvariant_new ("h", fd_id),
                                               xvariant_builder_end (&opt_builder),
                                               fd_list,
                                               NULL,
                                               NULL,
                                               NULL,
                                               error);
      g_free (path);
      xobject_unref (fd_list);
    }
  else
    {
      res = gxdp_open_uri_call_open_uri_sync (openuri,
                                              parent_window ? parent_window : "",
                                              uri,
                                              xvariant_builder_end (&opt_builder),
                                              NULL,
                                              NULL,
                                              error);
    }

  xobject_unref (file);

  return res;
}

enum {
  XDG_DESKTOP_PORTAL_SUCCESS   = 0,
  XDG_DESKTOP_PORTAL_CANCELLED = 1,
  XDG_DESKTOP_PORTAL_FAILED    = 2
};

static void
response_received (xdbus_connection_t *connection,
                   const char      *sender_name,
                   const char      *object_path,
                   const char      *interface_name,
                   const char      *signal_name,
                   xvariant_t        *parameters,
                   xpointer_t         user_data)
{
  xtask_t *task = user_data;
  xuint32_t response;
  xuint_t signal_id;

  signal_id = GPOINTER_TO_UINT (xobject_get_data (G_OBJECT (task), "signal-id"));
  g_dbus_connection_signal_unsubscribe (connection, signal_id);

  xvariant_get (parameters, "(u@a{sv})", &response, NULL);

  switch (response)
    {
    case XDG_DESKTOP_PORTAL_SUCCESS:
      xtask_return_boolean (task, TRUE);
      break;
    case XDG_DESKTOP_PORTAL_CANCELLED:
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Launch cancelled");
      break;
    case XDG_DESKTOP_PORTAL_FAILED:
    default:
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Launch failed");
      break;
    }

  xobject_unref (task);
}

static void
open_call_done (xobject_t      *source,
                xasync_result_t *result,
                xpointer_t      user_data)
{
  GXdpOpenURI *openuri = GXDP_OPEN_URI (source);
  xdbus_connection_t *connection;
  xtask_t *task = user_data;
  xerror_t *error = NULL;
  xboolean_t open_file;
  xboolean_t res;
  char *path = NULL;
  const char *handle;
  xuint_t signal_id;

  connection = g_dbus_proxy_get_connection (G_DBUS_PROXY (openuri));
  open_file = GPOINTER_TO_INT (xobject_get_data (G_OBJECT (task), "open-file"));

  if (open_file)
    res = gxdp_open_uri_call_open_file_finish (openuri, &path, NULL, result, &error);
  else
    res = gxdp_open_uri_call_open_uri_finish (openuri, &path, result, &error);

  if (!res)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      g_free (path);
      return;
    }

  handle = (const char *)xobject_get_data (G_OBJECT (task), "handle");
  if (xstrcmp0 (handle, path) != 0)
    {
      signal_id = GPOINTER_TO_UINT (xobject_get_data (G_OBJECT (task), "signal-id"));
      g_dbus_connection_signal_unsubscribe (connection, signal_id);

      signal_id = g_dbus_connection_signal_subscribe (connection,
                                                      "org.freedesktop.portal.Desktop",
                                                      "org.freedesktop.portal.Request",
                                                      "Response",
                                                      path,
                                                      NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                                      response_received,
                                                      task,
                                                      NULL);
      xobject_set_data (G_OBJECT (task), "signal-id", GINT_TO_POINTER (signal_id));
    }
}

void
g_openuri_portal_open_uri_async (const char          *uri,
                                 const char          *parent_window,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  xdbus_connection_t *connection;
  xtask_t *task;
  xfile_t *file;
  xvariant_t *opts = NULL;
  int i;
  xuint_t signal_id;

  if (!init_openuri_portal ())
    {
      xtask_report_new_error (NULL, callback, user_data, NULL,
                               G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                               "OpenURI portal is not available");
      return;
    }

  connection = g_dbus_proxy_get_connection (G_DBUS_PROXY (openuri));

  if (callback)
    {
      xvariant_builder_t opt_builder;
      char *token;
      char *sender;
      char *handle;

      task = xtask_new (NULL, cancellable, callback, user_data);

      token = xstrdup_printf ("gio%d", g_random_int_range (0, G_MAXINT));
      sender = xstrdup (g_dbus_connection_get_unique_name (connection) + 1);
      for (i = 0; sender[i]; i++)
        if (sender[i] == '.')
          sender[i] = '_';

      handle = xstrdup_printf ("/org/freedesktop/portal/desktop/request/%s/%s", sender, token);
      xobject_set_data_full (G_OBJECT (task), "handle", handle, g_free);
      g_free (sender);

      signal_id = g_dbus_connection_signal_subscribe (connection,
                                                      "org.freedesktop.portal.Desktop",
                                                      "org.freedesktop.portal.Request",
                                                      "Response",
                                                      handle,
                                                      NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                                      response_received,
                                                      task,
                                                      NULL);
      xobject_set_data (G_OBJECT (task), "signal-id", GINT_TO_POINTER (signal_id));

      xvariant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);
      xvariant_builder_add (&opt_builder, "{sv}", "handle_token", xvariant_new_string (token));
      g_free (token);

      opts = xvariant_builder_end (&opt_builder);
    }
  else
    task = NULL;

  file = xfile_new_for_uri (uri);
  if (xfile_is_native (file))
    {
      char *path = NULL;
      xunix_fd_list_t *fd_list = NULL;
      int fd, fd_id, errsv;

      if (task)
        xobject_set_data (G_OBJECT (task), "open-file", GINT_TO_POINTER (TRUE));

      path = xfile_get_path (file);
      fd = g_open (path, O_RDONLY | O_CLOEXEC);
      errsv = errno;
      if (fd == -1)
        {
          xtask_report_new_error (NULL, callback, user_data, NULL,
                                   G_IO_ERROR, g_io_error_from_errno (errsv),
                                   "OpenURI portal is not available");
          return;
        }

#ifndef HAVE_O_CLOEXEC
      fcntl (fd, F_SETFD, FD_CLOEXEC);
#endif
      fd_list = g_unix_fd_list_new_from_array (&fd, 1);
      fd = -1;
      fd_id = 0;

      gxdp_open_uri_call_open_file (openuri,
                                    parent_window ? parent_window : "",
                                    xvariant_new ("h", fd_id),
                                    opts,
                                    fd_list,
                                    cancellable,
                                    task ? open_call_done : NULL,
                                    task);
      xobject_unref (fd_list);
      g_free (path);
    }
  else
    {
      gxdp_open_uri_call_open_uri (openuri,
                                   parent_window ? parent_window : "",
                                   uri,
                                   opts,
                                   cancellable,
                                   task ? open_call_done : NULL,
                                   task);
    }

  xobject_unref (file);
}

xboolean_t
g_openuri_portal_open_uri_finish (xasync_result_t  *result,
                                  xerror_t       **error)
{
  return xtask_propagate_boolean (XTASK (result), error);
}
