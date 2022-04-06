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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gapplicationimpl.h"

#include "gactiongroup.h"
#include "gactiongroupexporter.h"
#include "gremoteactiongroup.h"
#include "gdbusactiongroup-private.h"
#include "gapplication.h"
#include "gfile.h"
#include "gdbusconnection.h"
#include "gdbusintrospection.h"
#include "gdbuserror.h"
#include "glib/gstdio.h"

#include <string.h>
#include <stdio.h>

#include "gapplicationcommandline.h"
#include "gdbusmethodinvocation.h"

#ifdef G_OS_UNIX
#include "gunixinputstream.h"
#include "gunixfdlist.h"
#endif

/* D-Bus Interface definition {{{1 */

/* For documentation of these interfaces, see
 * https://wiki.gnome.org/Projects/GLib/xapplication_t/DBusAPI
 */
static const xchar_t org_gtk_Application_xml[] =
  "<node>"
    "<interface name='org.gtk.Application'>"
      "<method name='Activate'>"
        "<arg type='a{sv}' name='platform-data' direction='in'/>"
      "</method>"
      "<method name='Open'>"
        "<arg type='as' name='uris' direction='in'/>"
        "<arg type='s' name='hint' direction='in'/>"
        "<arg type='a{sv}' name='platform-data' direction='in'/>"
      "</method>"
      "<method name='CommandLine'>"
        "<arg type='o' name='path' direction='in'/>"
        "<arg type='aay' name='arguments' direction='in'/>"
        "<arg type='a{sv}' name='platform-data' direction='in'/>"
        "<arg type='i' name='exit-status' direction='out'/>"
      "</method>"
    "<property name='Busy' type='b' access='read'/>"
    "</interface>"
  "</node>";

static xdbus_interface_info_t *org_gtk_Application;

static const xchar_t org_freedesktop_Application_xml[] =
  "<node>"
    "<interface name='org.freedesktop.Application'>"
      "<method name='Activate'>"
        "<arg type='a{sv}' name='platform-data' direction='in'/>"
      "</method>"
      "<method name='Open'>"
        "<arg type='as' name='uris' direction='in'/>"
        "<arg type='a{sv}' name='platform-data' direction='in'/>"
      "</method>"
      "<method name='ActivateAction'>"
        "<arg type='s' name='action-name' direction='in'/>"
        "<arg type='av' name='parameter' direction='in'/>"
        "<arg type='a{sv}' name='platform-data' direction='in'/>"
      "</method>"
    "</interface>"
  "</node>";

static xdbus_interface_info_t *org_freedesktop_Application;

static const xchar_t org_gtk_private_CommandLine_xml[] =
  "<node>"
    "<interface name='org.gtk.private.CommandLine'>"
      "<method name='Print'>"
        "<arg type='s' name='message' direction='in'/>"
      "</method>"
      "<method name='PrintError'>"
        "<arg type='s' name='message' direction='in'/>"
      "</method>"
    "</interface>"
  "</node>";

static xdbus_interface_info_t *org_gtk_private_CommandLine;

/* xapplication_t implementation {{{1 */
struct _GApplicationImpl
{
  xdbus_connection_t *session_bus;
  xaction_group_t    *exported_actions;
  const xchar_t     *bus_name;
  xuint_t            name_lost_signal;

  xchar_t           *object_path;
  xuint_t            object_id;
  xuint_t            fdo_object_id;
  xuint_t            actions_id;

  xboolean_t         properties_live;
  xboolean_t         primary;
  xboolean_t         busy;
  xboolean_t         registered;
  xapplication_t    *app;
};


static xapplication_command_line_t *
xdbus_command_line_new (xdbus_method_invocation_t *invocation);

static xvariant_t *
xapplication_impl_get_property (xdbus_connection_t *connection,
                                 const xchar_t  *sender,
                                 const xchar_t  *object_path,
                                 const xchar_t  *interface_name,
                                 const xchar_t  *property_name,
                                 xerror_t      **error,
                                 xpointer_t      user_data)
{
  GApplicationImpl *impl = user_data;

  if (strcmp (property_name, "Busy") == 0)
    return xvariant_new_boolean (impl->busy);

  g_assert_not_reached ();

  return NULL;
}

static void
send_property_change (GApplicationImpl *impl)
{
  xvariant_builder_t builder;

  xvariant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_add (&builder,
                         "{sv}",
                         "Busy", xvariant_new_boolean (impl->busy));

  xdbus_connection_emit_signal (impl->session_bus,
                                 NULL,
                                 impl->object_path,
                                 "org.freedesktop.DBus.Properties",
                                 "PropertiesChanged",
                                 xvariant_new ("(sa{sv}as)",
                                                "org.gtk.Application",
                                                &builder,
                                                NULL),
                                 NULL);
}

static void
xapplication_impl_method_call (xdbus_connection_t       *connection,
                                const xchar_t           *sender,
                                const xchar_t           *object_path,
                                const xchar_t           *interface_name,
                                const xchar_t           *method_name,
                                xvariant_t              *parameters,
                                xdbus_method_invocation_t *invocation,
                                xpointer_t               user_data)
{
  GApplicationImpl *impl = user_data;
  xapplication_class_t *class;

  class = G_APPLICATION_GET_CLASS (impl->app);

  if (strcmp (method_name, "Activate") == 0)
    {
      xvariant_t *platform_data;

      /* Completely the same for both freedesktop and gtk interfaces */

      xvariant_get (parameters, "(@a{sv})", &platform_data);

      class->before_emit (impl->app, platform_data);
      xsignal_emit_by_name (impl->app, "activate");
      class->after_emit (impl->app, platform_data);
      xvariant_unref (platform_data);

      xdbus_method_invocation_return_value (invocation, NULL);
    }

  else if (strcmp (method_name, "Open") == 0)
    {
      GApplicationFlags flags;
      xvariant_t *platform_data;
      const xchar_t *hint;
      xvariant_t *array;
      xfile_t **files;
      xint_t n, i;

      flags = xapplication_get_flags (impl->app);
      if ((flags & G_APPLICATION_HANDLES_OPEN) == 0)
        {
          xdbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "Application does not open files");
          return;
        }

      /* freedesktop interface has no hint parameter */
      if (xstr_equal (interface_name, "org.freedesktop.Application"))
        {
          xvariant_get (parameters, "(@as@a{sv})", &array, &platform_data);
          hint = "";
        }
      else
        xvariant_get (parameters, "(@as&s@a{sv})", &array, &hint, &platform_data);

      n = xvariant_n_children (array);
      files = g_new (xfile_t *, n + 1);

      for (i = 0; i < n; i++)
        {
          const xchar_t *uri;

          xvariant_get_child (array, i, "&s", &uri);
          files[i] = xfile_new_for_uri (uri);
        }
      xvariant_unref (array);
      files[n] = NULL;

      class->before_emit (impl->app, platform_data);
      xsignal_emit_by_name (impl->app, "open", files, n, hint);
      class->after_emit (impl->app, platform_data);

      xvariant_unref (platform_data);

      for (i = 0; i < n; i++)
        xobject_unref (files[i]);
      g_free (files);

      xdbus_method_invocation_return_value (invocation, NULL);
    }

  else if (strcmp (method_name, "CommandLine") == 0)
    {
      GApplicationFlags flags;
      xapplication_command_line_t *cmdline;
      xvariant_t *platform_data;
      int status;

      flags = xapplication_get_flags (impl->app);
      if ((flags & G_APPLICATION_HANDLES_COMMAND_LINE) == 0)
        {
          xdbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
                                                 "Application does not handle command line arguments");
          return;
        }

      /* Only on the GtkApplication interface */

      cmdline = xdbus_command_line_new (invocation);
      platform_data = xvariant_get_child_value (parameters, 2);
      class->before_emit (impl->app, platform_data);
      xsignal_emit_by_name (impl->app, "command-line", cmdline, &status);
      xapplication_command_line_set_exit_status (cmdline, status);
      class->after_emit (impl->app, platform_data);
      xvariant_unref (platform_data);
      xobject_unref (cmdline);
    }
  else if (xstr_equal (method_name, "ActivateAction"))
    {
      xvariant_t *parameter = NULL;
      xvariant_t *platform_data;
      xvariant_iter_t *iter;
      const xchar_t *name;

      /* Only on the freedesktop interface */

      xvariant_get (parameters, "(&sav@a{sv})", &name, &iter, &platform_data);
      xvariant_iter_next (iter, "v", &parameter);
      xvariant_iter_free (iter);

      class->before_emit (impl->app, platform_data);
      xaction_group_activate_action (impl->exported_actions, name, parameter);
      class->after_emit (impl->app, platform_data);

      if (parameter)
        xvariant_unref (parameter);

      xvariant_unref (platform_data);

      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else
    g_assert_not_reached ();
}

static xchar_t *
application_path_from_appid (const xchar_t *appid)
{
  xchar_t *appid_path, *iter;

  if (appid == NULL)
    /* this is a private implementation detail */
    return xstrdup ("/org/gtk/Application/anonymous");

  appid_path = xstrconcat ("/", appid, NULL);
  for (iter = appid_path; *iter; iter++)
    {
      if (*iter == '.')
        *iter = '/';

      if (*iter == '-')
        *iter = '_';
    }

  return appid_path;
}

static void xapplication_impl_stop_primary (GApplicationImpl *impl);

static void
name_lost (xdbus_connection_t *bus,
           const char      *sender_name,
           const char      *object_path,
           const char      *interface_name,
           const char      *signal_name,
           xvariant_t        *parameters,
           xpointer_t         user_data)
{
  GApplicationImpl *impl = user_data;
  xboolean_t handled;

  impl->primary = FALSE;
  xapplication_impl_stop_primary (impl);
  xsignal_emit_by_name (impl->app, "name-lost", &handled);
}

/* Attempt to become the primary instance.
 *
 * Returns %TRUE if everything went OK, regardless of if we became the
 * primary instance or not.  %FALSE is reserved for when something went
 * seriously wrong (and @error will be set too, in that case).
 *
 * After a %TRUE return, impl->primary will be TRUE if we were
 * successful.
 */
static xboolean_t
xapplication_impl_attempt_primary (GApplicationImpl  *impl,
                                    xcancellable_t      *cancellable,
                                    xerror_t           **error)
{
  static const xdbus_interface_vtable_t vtable = {
    xapplication_impl_method_call,
    xapplication_impl_get_property,
    NULL, /* set_property */
    { 0 }
  };
  xapplication_class_t *app_class = G_APPLICATION_GET_CLASS (impl->app);
  GBusNameOwnerFlags name_owner_flags;
  GApplicationFlags app_flags;
  xvariant_t *reply;
  xuint32_t rval;
  xerror_t *local_error = NULL;

  if (org_gtk_Application == NULL)
    {
      xerror_t *error = NULL;
      xdbus_node_info_t *info;

      info = g_dbus_node_info_new_for_xml (org_gtk_Application_xml, &error);
      if G_UNLIKELY (info == NULL)
        xerror ("%s", error->message);
      org_gtk_Application = g_dbus_node_info_lookup_interface (info, "org.gtk.Application");
      g_assert (org_gtk_Application != NULL);
      g_dbus_interface_info_ref (org_gtk_Application);
      g_dbus_node_info_unref (info);

      info = g_dbus_node_info_new_for_xml (org_freedesktop_Application_xml, &error);
      if G_UNLIKELY (info == NULL)
        xerror ("%s", error->message);
      org_freedesktop_Application = g_dbus_node_info_lookup_interface (info, "org.freedesktop.Application");
      g_assert (org_freedesktop_Application != NULL);
      g_dbus_interface_info_ref (org_freedesktop_Application);
      g_dbus_node_info_unref (info);
    }

  /* We could possibly have been D-Bus activated as a result of incoming
   * requests on either the application or actiongroup interfaces.
   * Because of how GDBus dispatches messages, we need to ensure that
   * both of those things are registered before we attempt to request
   * our name.
   *
   * The action group need not be populated yet, as long as it happens
   * before we return to the mainloop.  The reason for that is because
   * GDBus does the check to make sure the object exists from the worker
   * thread but doesn't actually dispatch the action invocation until we
   * hit the mainloop in this thread.  There is also no danger of
   * receiving 'activate' or 'open' signals until after 'startup' runs,
   * for the same reason.
   */
  impl->object_id = xdbus_connection_register_object (impl->session_bus, impl->object_path,
                                                       org_gtk_Application, &vtable, impl, NULL, error);

  if (impl->object_id == 0)
    return FALSE;

  impl->fdo_object_id = xdbus_connection_register_object (impl->session_bus, impl->object_path,
                                                           org_freedesktop_Application, &vtable, impl, NULL, error);

  if (impl->fdo_object_id == 0)
    return FALSE;

  impl->actions_id = xdbus_connection_export_action_group (impl->session_bus, impl->object_path,
                                                            impl->exported_actions, error);

  if (impl->actions_id == 0)
    return FALSE;

  impl->registered = TRUE;
  if (!app_class->dbus_register (impl->app,
                                 impl->session_bus,
                                 impl->object_path,
                                 &local_error))
    {
      g_return_val_if_fail (local_error != NULL, FALSE);
      g_propagate_error (error, g_steal_pointer (&local_error));
      return FALSE;
    }

  g_return_val_if_fail (local_error == NULL, FALSE);

  if (impl->bus_name == NULL)
    {
      /* If this is a non-unique application then it is sufficient to
       * have our object paths registered. We can return now.
       *
       * Note: non-unique applications always act as primary-instance.
       */
      impl->primary = TRUE;
      return TRUE;
    }

  /* If this is a unique application then we need to attempt to own
   * the well-known name and fall back to remote mode (!is_primary)
   * in the case that we can't do that.
   */
  name_owner_flags = G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE;
  app_flags = xapplication_get_flags (impl->app);

  if (app_flags & G_APPLICATION_ALLOW_REPLACEMENT)
    {
      impl->name_lost_signal = xdbus_connection_signal_subscribe (impl->session_bus,
                                                                   "org.freedesktop.DBus",
                                                                   "org.freedesktop.DBus",
                                                                   "NameLost",
                                                                   "/org/freedesktop/DBus",
                                                                   impl->bus_name,
                                                                   G_DBUS_SIGNAL_FLAGS_NONE,
                                                                   name_lost,
                                                                   impl,
                                                                   NULL);

      name_owner_flags |= G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
    }
  if (app_flags & G_APPLICATION_REPLACE)
    name_owner_flags |= G_BUS_NAME_OWNER_FLAGS_REPLACE;

  reply = xdbus_connection_call_sync (impl->session_bus,
                                       "org.freedesktop.DBus",
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus",
                                       "RequestName",
                                       xvariant_new ("(su)", impl->bus_name, name_owner_flags),
                                       G_VARIANT_TYPE ("(u)"),
                                       0, -1, cancellable, error);

  if (reply == NULL)
    return FALSE;

  xvariant_get (reply, "(u)", &rval);
  xvariant_unref (reply);

  /* DBUS_REQUEST_NAME_REPLY_EXISTS: 3 */
  impl->primary = (rval != 3);

  if (!impl->primary && impl->name_lost_signal)
    {
      xdbus_connection_signal_unsubscribe (impl->session_bus, impl->name_lost_signal);
      impl->name_lost_signal = 0;
    }

  return TRUE;
}

/* Stop doing the things that the primary instance does.
 *
 * This should be called if attempting to become the primary instance
 * failed (in order to clean up any partial success) and should also
 * be called when freeing the xapplication_t.
 *
 * It is safe to call this multiple times.
 */
static void
xapplication_impl_stop_primary (GApplicationImpl *impl)
{
  xapplication_class_t *app_class = G_APPLICATION_GET_CLASS (impl->app);

  if (impl->registered)
    {
      app_class->dbus_unregister (impl->app,
                                  impl->session_bus,
                                  impl->object_path);
      impl->registered = FALSE;
    }

  if (impl->object_id)
    {
      xdbus_connection_unregister_object (impl->session_bus, impl->object_id);
      impl->object_id = 0;
    }

  if (impl->fdo_object_id)
    {
      xdbus_connection_unregister_object (impl->session_bus, impl->fdo_object_id);
      impl->fdo_object_id = 0;
    }

  if (impl->actions_id)
    {
      xdbus_connection_unexport_action_group (impl->session_bus, impl->actions_id);
      impl->actions_id = 0;
    }

  if (impl->name_lost_signal)
    {
      xdbus_connection_signal_unsubscribe (impl->session_bus, impl->name_lost_signal);
      impl->name_lost_signal = 0;
    }

  if (impl->primary && impl->bus_name)
    {
      xdbus_connection_call (impl->session_bus, "org.freedesktop.DBus",
                              "/org/freedesktop/DBus", "org.freedesktop.DBus",
                              "ReleaseName", xvariant_new ("(s)", impl->bus_name),
                              NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
      impl->primary = FALSE;
    }
}

void
xapplication_impl_set_busy_state (GApplicationImpl *impl,
                                   xboolean_t          busy)
{
  if (impl->busy != busy)
    {
      impl->busy = busy;
      send_property_change (impl);
    }
}

void
xapplication_impl_destroy (GApplicationImpl *impl)
{
  xapplication_impl_stop_primary (impl);

  if (impl->session_bus)
    xobject_unref (impl->session_bus);

  g_free (impl->object_path);

  g_slice_free (GApplicationImpl, impl);
}

GApplicationImpl *
xapplication_impl_register (xapplication_t        *application,
                             const xchar_t         *appid,
                             GApplicationFlags    flags,
                             xaction_group_t        *exported_actions,
                             xremote_action_group_t **remote_actions,
                             xcancellable_t        *cancellable,
                             xerror_t             **error)
{
  xdbus_action_group_t *actions;
  GApplicationImpl *impl;

  g_assert ((flags & G_APPLICATION_NON_UNIQUE) || appid != NULL);

  impl = g_slice_new0 (GApplicationImpl);

  impl->app = application;
  impl->exported_actions = exported_actions;

  /* non-unique applications do not attempt to acquire a bus name */
  if (~flags & G_APPLICATION_NON_UNIQUE)
    impl->bus_name = appid;

  impl->session_bus = g_bus_get_sync (G_BUS_TYPE_SESSION, cancellable, NULL);

  if (impl->session_bus == NULL)
    {
      /* If we can't connect to the session bus, proceed as a normal
       * non-unique application.
       */
      *remote_actions = NULL;
      return impl;
    }

  impl->object_path = application_path_from_appid (appid);

  /* Only try to be the primary instance if
   * G_APPLICATION_IS_LAUNCHER was not specified.
   */
  if (~flags & G_APPLICATION_IS_LAUNCHER)
    {
      if (!xapplication_impl_attempt_primary (impl, cancellable, error))
        {
          xapplication_impl_destroy (impl);
          return NULL;
        }

      if (impl->primary)
        return impl;

      /* We didn't make it.  Drop our service-side stuff. */
      xapplication_impl_stop_primary (impl);

      if (flags & G_APPLICATION_IS_SERVICE)
        {
          g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                       "Unable to acquire bus name '%s'", appid);
          xapplication_impl_destroy (impl);

          return NULL;
        }
    }

  /* We are non-primary.  Try to get the primary's list of actions.
   * This also serves as a mechanism to ensure that the primary exists
   * (ie: D-Bus service files installed correctly, etc).
   */
  actions = xdbus_action_group_get (impl->session_bus, impl->bus_name, impl->object_path);
  if (!xdbus_action_group_sync (actions, cancellable, error))
    {
      /* The primary appears not to exist.  Fail the registration. */
      xapplication_impl_destroy (impl);
      xobject_unref (actions);

      return NULL;
    }

  *remote_actions = G_REMOTE_ACTION_GROUP (actions);

  return impl;
}

void
xapplication_impl_activate (GApplicationImpl *impl,
                             xvariant_t         *platform_data)
{
  xdbus_connection_call (impl->session_bus,
                          impl->bus_name,
                          impl->object_path,
                          "org.gtk.Application",
                          "Activate",
                          xvariant_new ("(@a{sv})", platform_data),
                          NULL, 0, -1, NULL, NULL, NULL);
}

void
xapplication_impl_open (GApplicationImpl  *impl,
                         xfile_t            **files,
                         xint_t               n_files,
                         const xchar_t       *hint,
                         xvariant_t          *platform_data)
{
  xvariant_builder_t builder;
  xint_t i;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("(assa{sv})"));
  xvariant_builder_open (&builder, G_VARIANT_TYPE_STRING_ARRAY);
  for (i = 0; i < n_files; i++)
    {
      xchar_t *uri = xfile_get_uri (files[i]);
      xvariant_builder_add (&builder, "s", uri);
      g_free (uri);
    }
  xvariant_builder_close (&builder);
  xvariant_builder_add (&builder, "s", hint);
  xvariant_builder_add_value (&builder, platform_data);

  xdbus_connection_call (impl->session_bus,
                          impl->bus_name,
                          impl->object_path,
                          "org.gtk.Application",
                          "Open",
                          xvariant_builder_end (&builder),
                          NULL, 0, -1, NULL, NULL, NULL);
}

static void
xapplication_impl_cmdline_method_call (xdbus_connection_t       *connection,
                                        const xchar_t           *sender,
                                        const xchar_t           *object_path,
                                        const xchar_t           *interface_name,
                                        const xchar_t           *method_name,
                                        xvariant_t              *parameters,
                                        xdbus_method_invocation_t *invocation,
                                        xpointer_t               user_data)
{
  const xchar_t *message;

  xvariant_get_child (parameters, 0, "&s", &message);

  if (strcmp (method_name, "Print") == 0)
    g_print ("%s", message);
  else if (strcmp (method_name, "PrintError") == 0)
    g_printerr ("%s", message);
  else
    g_assert_not_reached ();

  xdbus_method_invocation_return_value (invocation, NULL);
}

typedef struct
{
  xmain_loop_t *loop;
  int status;
} CommandLineData;

static void
xapplication_impl_cmdline_done (xobject_t      *source,
                                 xasync_result_t *result,
                                 xpointer_t      user_data)
{
  CommandLineData *data = user_data;
  xerror_t *error = NULL;
  xvariant_t *reply;

#ifdef G_OS_UNIX
  reply = xdbus_connection_call_with_unix_fd_list_finish (G_DBUS_CONNECTION (source), NULL, result, &error);
#else
  reply = xdbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);
#endif


  if (reply != NULL)
    {
      xvariant_get (reply, "(i)", &data->status);
      xvariant_unref (reply);
    }

  else
    {
      g_printerr ("%s\n", error->message);
      xerror_free (error);
      data->status = 1;
    }

  xmain_loop_quit (data->loop);
}

int
xapplication_impl_command_line (GApplicationImpl    *impl,
                                 const xchar_t * const *arguments,
                                 xvariant_t            *platform_data)
{
  static const xdbus_interface_vtable_t vtable = {
    xapplication_impl_cmdline_method_call, NULL, NULL, { 0 }
  };
  const xchar_t *object_path = "/org/gtk/Application/CommandLine";
  xmain_context_t *context;
  CommandLineData data;
  xuint_t object_id G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  context = xmain_context_new ();
  data.loop = xmain_loop_new (context, FALSE);
  xmain_context_push_thread_default (context);

  if (org_gtk_private_CommandLine == NULL)
    {
      xerror_t *error = NULL;
      xdbus_node_info_t *info;

      info = g_dbus_node_info_new_for_xml (org_gtk_private_CommandLine_xml, &error);
      if G_UNLIKELY (info == NULL)
        xerror ("%s", error->message);
      org_gtk_private_CommandLine = g_dbus_node_info_lookup_interface (info, "org.gtk.private.CommandLine");
      g_assert (org_gtk_private_CommandLine != NULL);
      g_dbus_interface_info_ref (org_gtk_private_CommandLine);
      g_dbus_node_info_unref (info);
    }

  object_id = xdbus_connection_register_object (impl->session_bus, object_path,
                                                 org_gtk_private_CommandLine,
                                                 &vtable, &data, NULL, NULL);
  /* In theory we should try other paths... */
  g_assert (object_id != 0);

#ifdef G_OS_UNIX
  {
    xerror_t *error = NULL;
    xunix_fd_list_t *fd_list;

    /* send along the stdin in case
     * xapplication_command_line_get_stdin_data() is called
     */
    fd_list = g_unix_fd_list_new ();
    g_unix_fd_list_append (fd_list, 0, &error);
    g_assert_no_error (error);

    xdbus_connection_call_with_unix_fd_list (impl->session_bus, impl->bus_name, impl->object_path,
                                              "org.gtk.Application", "CommandLine",
                                              xvariant_new ("(o^aay@a{sv})", object_path, arguments, platform_data),
                                              G_VARIANT_TYPE ("(i)"), 0, G_MAXINT, fd_list, NULL,
                                              xapplication_impl_cmdline_done, &data);
    xobject_unref (fd_list);
  }
#else
  xdbus_connection_call (impl->session_bus, impl->bus_name, impl->object_path,
                          "org.gtk.Application", "CommandLine",
                          xvariant_new ("(o^aay@a{sv})", object_path, arguments, platform_data),
                          G_VARIANT_TYPE ("(i)"), 0, G_MAXINT, NULL,
                          xapplication_impl_cmdline_done, &data);
#endif

  xmain_loop_run (data.loop);

  xmain_context_pop_thread_default (context);
  xmain_context_unref (context);
  xmain_loop_unref (data.loop);

  return data.status;
}

void
xapplication_impl_flush (GApplicationImpl *impl)
{
  if (impl->session_bus)
    xdbus_connection_flush_sync (impl->session_bus, NULL, NULL);
}

xdbus_connection_t *
xapplication_impl_get_dbus_connection (GApplicationImpl *impl)
{
  return impl->session_bus;
}

const xchar_t *
xapplication_impl_get_dbus_object_path (GApplicationImpl *impl)
{
  return impl->object_path;
}

/* xdbus_command_line_t implementation {{{1 */

typedef xapplication_command_line_class_t GDBusCommandLineClass;
static xtype_t xdbus_command_line_get_type (void);
typedef struct
{
  xapplication_command_line_t  parent_instance;
  xdbus_method_invocation_t   *invocation;

  xdbus_connection_t *connection;
  const xchar_t     *bus_name;
  const xchar_t     *object_path;
} xdbus_command_line_t;


G_DEFINE_TYPE (xdbus_command_line,
               xdbus_command_line,
               XTYPE_APPLICATION_COMMAND_LINE)

static void
xdbus_command_line_print_literal (xapplication_command_line_t *cmdline,
                                   const xchar_t             *message)
{
  xdbus_command_line_t *gdbcl = (xdbus_command_line_t *) cmdline;

  xdbus_connection_call (gdbcl->connection,
                          gdbcl->bus_name,
                          gdbcl->object_path,
                          "org.gtk.private.CommandLine", "Print",
                          xvariant_new ("(s)", message),
                          NULL, 0, -1, NULL, NULL, NULL);
}

static void
xdbus_command_line_printerr_literal (xapplication_command_line_t *cmdline,
                                      const xchar_t             *message)
{
  xdbus_command_line_t *gdbcl = (xdbus_command_line_t *) cmdline;

  xdbus_connection_call (gdbcl->connection,
                          gdbcl->bus_name,
                          gdbcl->object_path,
                          "org.gtk.private.CommandLine", "PrintError",
                          xvariant_new ("(s)", message),
                          NULL, 0, -1, NULL, NULL, NULL);
}

static xinput_stream_t *
xdbus_command_line_get_stdin (xapplication_command_line_t *cmdline)
{
#ifdef G_OS_UNIX
  xdbus_command_line_t *gdbcl = (xdbus_command_line_t *) cmdline;
  xinput_stream_t *result = NULL;
  xdbus_message_t *message;
  xunix_fd_list_t *fd_list;

  message = xdbus_method_invocation_get_message (gdbcl->invocation);
  fd_list = xdbus_message_get_unix_fd_list (message);

  if (fd_list && g_unix_fd_list_get_length (fd_list))
    {
      xint_t *fds, n_fds, i;

      fds = g_unix_fd_list_steal_fds (fd_list, &n_fds);
      result = g_unix_input_stream_new (fds[0], TRUE);
      for (i = 1; i < n_fds; i++)
        (void) g_close (fds[i], NULL);
      g_free (fds);
    }

  return result;
#else
  return NULL;
#endif
}

static void
xdbus_command_line_finalize (xobject_t *object)
{
  xapplication_command_line_t *cmdline = G_APPLICATION_COMMAND_LINE (object);
  xdbus_command_line_t *gdbcl = (xdbus_command_line_t *) object;
  xint_t status;

  status = xapplication_command_line_get_exit_status (cmdline);

  xdbus_method_invocation_return_value (gdbcl->invocation,
                                         xvariant_new ("(i)", status));
  xobject_unref (gdbcl->invocation);

  G_OBJECT_CLASS (xdbus_command_line_parent_class)
    ->finalize (object);
}

static void
xdbus_command_line_init (xdbus_command_line_t *gdbcl)
{
}

static void
xdbus_command_line_class_init (xapplication_command_line_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = xdbus_command_line_finalize;
  class->printerr_literal = xdbus_command_line_printerr_literal;
  class->print_literal = xdbus_command_line_print_literal;
  class->get_stdin = xdbus_command_line_get_stdin;
}

static xapplication_command_line_t *
xdbus_command_line_new (xdbus_method_invocation_t *invocation)
{
  xdbus_command_line_t *gdbcl;
  xvariant_t *args;
  xvariant_t *arguments, *platform_data;

  args = xdbus_method_invocation_get_parameters (invocation);

  arguments = xvariant_get_child_value (args, 1);
  platform_data = xvariant_get_child_value (args, 2);
  gdbcl = xobject_new (xdbus_command_line_get_type (),
                        "arguments", arguments,
                        "platform-data", platform_data,
                        NULL);
  xvariant_unref (arguments);
  xvariant_unref (platform_data);

  gdbcl->connection = xdbus_method_invocation_get_connection (invocation);
  gdbcl->bus_name = xdbus_method_invocation_get_sender (invocation);
  xvariant_get_child (args, 0, "&o", &gdbcl->object_path);
  gdbcl->invocation = xobject_ref (invocation);

  return G_APPLICATION_COMMAND_LINE (gdbcl);
}

/* Epilogue {{{1 */

/* vim:set foldmethod=marker: */
