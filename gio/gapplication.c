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

/* Prologue {{{1 */
#include "config.h"

#include "gapplication.h"

#include "gapplicationcommandline.h"
#include "gsimpleactiongroup.h"
#include "gremoteactiongroup.h"
#include "gapplicationimpl.h"
#include "gactiongroup.h"
#include "gactionmap.h"
#include "gsettings.h"
#include "gnotification-private.h"
#include "gnotificationbackend.h"
#include "gdbusutils.h"

#include "gioenumtypes.h"
#include "gioenums.h"
#include "gfile.h"

#include "glibintl.h"
#include "gmarshal-internal.h"

#include <string.h>

/**
 * SECTION:gapplication
 * @title: xapplication_t
 * @short_description: Core application class
 * @include: gio/gio.h
 *
 * A #xapplication_t is the foundation of an application.  It wraps some
 * low-level platform-specific services and is intended to act as the
 * foundation for higher-level application classes such as
 * #GtkApplication or #MxApplication.  In general, you should not use
 * this class outside of a higher level framework.
 *
 * xapplication_t provides convenient life cycle management by maintaining
 * a "use count" for the primary application instance. The use count can
 * be changed using xapplication_hold() and xapplication_release(). If
 * it drops to zero, the application exits. Higher-level classes such as
 * #GtkApplication employ the use count to ensure that the application
 * stays alive as long as it has any opened windows.
 *
 * Another feature that xapplication_t (optionally) provides is process
 * uniqueness. Applications can make use of this functionality by
 * providing a unique application ID. If given, only one application
 * with this ID can be running at a time per session. The session
 * concept is platform-dependent, but corresponds roughly to a graphical
 * desktop login. When your application is launched again, its
 * arguments are passed through platform communication to the already
 * running program. The already running instance of the program is
 * called the "primary instance"; for non-unique applications this is
 * always the current instance. On Linux, the D-Bus session bus
 * is used for communication.
 *
 * The use of #xapplication_t differs from some other commonly-used
 * uniqueness libraries (such as libunique) in important ways. The
 * application is not expected to manually register itself and check
 * if it is the primary instance. Instead, the main() function of a
 * #xapplication_t should do very little more than instantiating the
 * application instance, possibly connecting signal handlers, then
 * calling xapplication_run(). All checks for uniqueness are done
 * internally. If the application is the primary instance then the
 * startup signal is emitted and the mainloop runs. If the application
 * is not the primary instance then a signal is sent to the primary
 * instance and xapplication_run() promptly returns. See the code
 * examples below.
 *
 * If used, the expected form of an application identifier is the same as
 * that of of a
 * [D-Bus well-known bus name](https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names-bus).
 * Examples include: `com.example.MyApp`, `org.example.internal_apps.Calculator`,
 * `org._7_zip.Archiver`.
 * For details on valid application identifiers, see xapplication_id_is_valid().
 *
 * On Linux, the application identifier is claimed as a well-known bus name
 * on the user's session bus.  This means that the uniqueness of your
 * application is scoped to the current session.  It also means that your
 * application may provide additional services (through registration of other
 * object paths) at that bus name.  The registration of these object paths
 * should be done with the shared GDBus session bus.  Note that due to the
 * internal architecture of GDBus, method calls can be dispatched at any time
 * (even if a main loop is not running).  For this reason, you must ensure that
 * any object paths that you wish to register are registered before #xapplication_t
 * attempts to acquire the bus name of your application (which happens in
 * xapplication_register()).  Unfortunately, this means that you cannot use
 * xapplication_get_is_remote() to decide if you want to register object paths.
 *
 * xapplication_t also implements the #xaction_group_t and #xaction_map_t
 * interfaces and lets you easily export actions by adding them with
 * xaction_map_add_action(). When invoking an action by calling
 * xaction_group_activate_action() on the application, it is always
 * invoked in the primary instance. The actions are also exported on
 * the session bus, and GIO provides the #xdbus_action_group_t wrapper to
 * conveniently access them remotely. GIO provides a #xdbus_menu_model_t wrapper
 * for remote access to exported #GMenuModels.
 *
 * There is a number of different entry points into a xapplication_t:
 *
 * - via 'Activate' (i.e. just starting the application)
 *
 * - via 'Open' (i.e. opening some files)
 *
 * - by handling a command-line
 *
 * - via activating an action
 *
 * The #xapplication_t::startup signal lets you handle the application
 * initialization for all of these in a single place.
 *
 * Regardless of which of these entry points is used to start the
 * application, xapplication_t passes some ‘platform data’ from the
 * launching instance to the primary instance, in the form of a
 * #xvariant_t dictionary mapping strings to variants. To use platform
 * data, override the @before_emit or @after_emit virtual functions
 * in your #xapplication_t subclass. When dealing with
 * #xapplication_command_line_t objects, the platform data is
 * directly available via xapplication_command_line_get_cwd(),
 * xapplication_command_line_get_environ() and
 * xapplication_command_line_get_platform_data().
 *
 * As the name indicates, the platform data may vary depending on the
 * operating system, but it always includes the current directory (key
 * "cwd"), and optionally the environment (ie the set of environment
 * variables and their values) of the calling process (key "environ").
 * The environment is only added to the platform data if the
 * %G_APPLICATION_SEND_ENVIRONMENT flag is set. #xapplication_t subclasses
 * can add their own platform data by overriding the @add_platform_data
 * virtual function. For instance, #GtkApplication adds startup notification
 * data in this way.
 *
 * To parse commandline arguments you may handle the
 * #xapplication_t::command-line signal or override the local_command_line()
 * vfunc, to parse them in either the primary instance or the local instance,
 * respectively.
 *
 * For an example of opening files with a xapplication_t, see
 * [gapplication-example-open.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gapplication-example-open.c).
 *
 * For an example of using actions with xapplication_t, see
 * [gapplication-example-actions.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gapplication-example-actions.c).
 *
 * For an example of using extra D-Bus hooks with xapplication_t, see
 * [gapplication-example-dbushooks.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gapplication-example-dbushooks.c).
 */

/**
 * xapplication_t:
 *
 * #xapplication_t is an opaque data structure and can only be accessed
 * using the following functions.
 * Since: 2.28
 */

/**
 * xapplication_class_t:
 * @startup: invoked on the primary instance immediately after registration
 * @shutdown: invoked only on the registered primary instance immediately
 *      after the main loop terminates
 * @activate: invoked on the primary instance when an activation occurs
 * @open: invoked on the primary instance when there are files to open
 * @command_line: invoked on the primary instance when a command-line is
 *   not handled locally
 * @local_command_line: invoked (locally). The virtual function has the chance
 *     to inspect (and possibly replace) command line arguments. See
 *     xapplication_run() for more information. Also see the
 *     #xapplication_t::handle-local-options signal, which is a simpler
 *     alternative to handling some commandline options locally
 * @before_emit: invoked on the primary instance before 'activate', 'open',
 *     'command-line' or any action invocation, gets the 'platform data' from
 *     the calling instance
 * @after_emit: invoked on the primary instance after 'activate', 'open',
 *     'command-line' or any action invocation, gets the 'platform data' from
 *     the calling instance
 * @add_platform_data: invoked (locally) to add 'platform data' to be sent to
 *     the primary instance when activating, opening or invoking actions
 * @quit_mainloop: Used to be invoked on the primary instance when the use
 *     count of the application drops to zero (and after any inactivity
 *     timeout, if requested). Not used anymore since 2.32
 * @run_mainloop: Used to be invoked on the primary instance from
 *     xapplication_run() if the use-count is non-zero. Since 2.32,
 *     xapplication_t is iterating the main context directly and is not
 *     using @run_mainloop anymore
 * @dbus_register: invoked locally during registration, if the application is
 *     using its D-Bus backend. You can use this to export extra objects on the
 *     bus, that need to exist before the application tries to own the bus name.
 *     The function is passed the #xdbus_connection_t to to session bus, and the
 *     object path that #xapplication_t will use to export is D-Bus API.
 *     If this function returns %TRUE, registration will proceed; otherwise
 *     registration will abort. Since: 2.34
 * @dbus_unregister: invoked locally during unregistration, if the application
 *     is using its D-Bus backend. Use this to undo anything done by
 *     the @dbus_register vfunc. Since: 2.34
 * @handle_local_options: invoked locally after the parsing of the commandline
 *  options has occurred. Since: 2.40
 * @name_lost: invoked when another instance is taking over the name. Since: 2.60
 *
 * Virtual function table for #xapplication_t.
 *
 * Since: 2.28
 */

struct _xapplication_private_t
{
  GApplicationFlags  flags;
  xchar_t             *id;
  xchar_t             *resource_path;

  xaction_group_t      *actions;

  xuint_t              inactivity_timeout_id;
  xuint_t              inactivity_timeout;
  xuint_t              use_count;
  xuint_t              busy_count;

  xuint_t              is_registered : 1;
  xuint_t              is_remote : 1;
  xuint_t              did_startup : 1;
  xuint_t              did_shutdown : 1;
  xuint_t              must_quit_now : 1;

  xremote_action_group_t *remote_actions;
  GApplicationImpl   *impl;

  xnotification_backend_t *notifications;

  /* xoption_context_t support */
  xoption_group_t       *main_options;
  xslist_t             *option_groups;
  xhashtable_t         *packed_options;
  xboolean_t            options_parsed;
  xchar_t              *parameter_string;
  xchar_t              *summary;
  xchar_t              *description;

  /* Allocated option strings, from xapplication_add_main_option() */
  xslist_t             *option_strings;
};

enum
{
  PROP_NONE,
  PROP_APPLICATION_ID,
  PROP_FLAGS,
  PROP_RESOURCE_BASE_PATH,
  PROP_IS_REGISTERED,
  PROP_IS_REMOTE,
  PROP_INACTIVITY_TIMEOUT,
  PROP_ACTION_GROUP,
  PROP_IS_BUSY
};

enum
{
  SIGNAL_STARTUP,
  SIGNAL_SHUTDOWN,
  SIGNAL_ACTIVATE,
  SIGNAL_OPEN,
  SIGNAL_ACTION,
  SIGNAL_COMMAND_LINE,
  SIGNAL_HANDLE_LOCAL_OPTIONS,
  SIGNAL_NAME_LOST,
  NR_SIGNALS
};

static xuint_t xapplication_signals[NR_SIGNALS];

static void xapplication_action_group_iface_init (xaction_group_interface_t *);
static void xapplication_action_map_iface_init (xaction_map_interface_t *);
G_DEFINE_TYPE_WITH_CODE (xapplication, xapplication, XTYPE_OBJECT,
 G_ADD_PRIVATE (xapplication_t)
 G_IMPLEMENT_INTERFACE (XTYPE_ACTION_GROUP, xapplication_action_group_iface_init)
 G_IMPLEMENT_INTERFACE (XTYPE_ACTION_MAP, xapplication_action_map_iface_init))

/* GApplicationExportedActions {{{1 */

/* We create a subclass of xsimple_action_group_t that implements
 * xremote_action_group_t and deals with the platform data using
 * xapplication_t's before/after_emit vfuncs.  This is the action group we
 * will be exporting.
 *
 * We could implement xremote_action_group_t on xapplication_t directly, but
 * this would be potentially extremely confusing to have exposed as part
 * of the public API of xapplication_t.  We certainly don't want anyone in
 * the same process to be calling these APIs...
 */
typedef GSimpleActionGroupClass GApplicationExportedActionsClass;
typedef struct
{
  xsimple_action_group_t parent_instance;
  xapplication_t *application;
} GApplicationExportedActions;

static xtype_t xapplication_exported_actions_get_type   (void);
static void  xapplication_exported_actions_iface_init (xremote_action_group_interface_t *iface);
G_DEFINE_TYPE_WITH_CODE (GApplicationExportedActions, xapplication_exported_actions, XTYPE_SIMPLE_ACTION_GROUP,
                         G_IMPLEMENT_INTERFACE (XTYPE_REMOTE_ACTION_GROUP, xapplication_exported_actions_iface_init))

static void
xapplication_exported_actions_activate_action_full (xremote_action_group_t *remote,
                                                     const xchar_t        *action_name,
                                                     xvariant_t           *parameter,
                                                     xvariant_t           *platform_data)
{
  GApplicationExportedActions *exported = (GApplicationExportedActions *) remote;

  G_APPLICATION_GET_CLASS (exported->application)
    ->before_emit (exported->application, platform_data);

  xaction_group_activate_action (XACTION_GROUP (exported), action_name, parameter);

  G_APPLICATION_GET_CLASS (exported->application)
    ->after_emit (exported->application, platform_data);
}

static void
xapplication_exported_actions_change_action_state_full (xremote_action_group_t *remote,
                                                         const xchar_t        *action_name,
                                                         xvariant_t           *value,
                                                         xvariant_t           *platform_data)
{
  GApplicationExportedActions *exported = (GApplicationExportedActions *) remote;

  G_APPLICATION_GET_CLASS (exported->application)
    ->before_emit (exported->application, platform_data);

  xaction_group_change_action_state (XACTION_GROUP (exported), action_name, value);

  G_APPLICATION_GET_CLASS (exported->application)
    ->after_emit (exported->application, platform_data);
}

static void
xapplication_exported_actions_init (GApplicationExportedActions *actions)
{
}

static void
xapplication_exported_actions_iface_init (xremote_action_group_interface_t *iface)
{
  iface->activate_action_full = xapplication_exported_actions_activate_action_full;
  iface->change_action_state_full = xapplication_exported_actions_change_action_state_full;
}

static void
xapplication_exported_actions_class_init (GApplicationExportedActionsClass *class)
{
}

static xaction_group_t *
xapplication_exported_actions_new (xapplication_t *application)
{
  GApplicationExportedActions *actions;

  actions = xobject_new (xapplication_exported_actions_get_type (), NULL);
  actions->application = application;

  return XACTION_GROUP (actions);
}

/* Command line option handling {{{1 */

static void
free_option_entry (xpointer_t data)
{
  GOptionEntry *entry = data;

  switch (entry->arg)
    {
    case G_OPTION_ARG_STRING:
    case G_OPTION_ARXFILENAME:
      g_free (*(xchar_t **) entry->arg_data);
      break;

    case G_OPTION_ARG_STRING_ARRAY:
    case G_OPTION_ARXFILENAME_ARRAY:
      xstrfreev (*(xchar_t ***) entry->arg_data);
      break;

    default:
      /* most things require no free... */
      break;
    }

  /* ...except for the space that we allocated for it ourselves */
  g_free (entry->arg_data);

  g_slice_free (GOptionEntry, entry);
}

static void
xapplication_pack_option_entries (xapplication_t *application,
                                   xvariant_dict_t *dict)
{
  xhash_table_iter_t iter;
  xpointer_t item;

  xhash_table_iter_init (&iter, application->priv->packed_options);
  while (xhash_table_iter_next (&iter, NULL, &item))
    {
      GOptionEntry *entry = item;
      xvariant_t *value = NULL;

      switch (entry->arg)
        {
        case G_OPTION_ARG_NONE:
          if (*(xboolean_t *) entry->arg_data != 2)
            value = xvariant_new_boolean (*(xboolean_t *) entry->arg_data);
          break;

        case G_OPTION_ARG_STRING:
          if (*(xchar_t **) entry->arg_data)
            value = xvariant_new_string (*(xchar_t **) entry->arg_data);
          break;

        case G_OPTION_ARG_INT:
          if (*(gint32 *) entry->arg_data)
            value = xvariant_new_int32 (*(gint32 *) entry->arg_data);
          break;

        case G_OPTION_ARXFILENAME:
          if (*(xchar_t **) entry->arg_data)
            value = xvariant_new_bytestring (*(xchar_t **) entry->arg_data);
          break;

        case G_OPTION_ARG_STRING_ARRAY:
          if (*(xchar_t ***) entry->arg_data)
            value = xvariant_new_strv (*(const xchar_t ***) entry->arg_data, -1);
          break;

        case G_OPTION_ARXFILENAME_ARRAY:
          if (*(xchar_t ***) entry->arg_data)
            value = xvariant_new_bytestring_array (*(const xchar_t ***) entry->arg_data, -1);
          break;

        case G_OPTION_ARG_DOUBLE:
          if (*(xdouble_t *) entry->arg_data)
            value = xvariant_new_double (*(xdouble_t *) entry->arg_data);
          break;

        case G_OPTION_ARG_INT64:
          if (*(gint64 *) entry->arg_data)
            value = xvariant_new_int64 (*(gint64 *) entry->arg_data);
          break;

        default:
          g_assert_not_reached ();
        }

      if (value)
        xvariant_dict_insert_value (dict, entry->long_name, value);
    }
}

static xvariant_dict_t *
xapplication_parse_command_line (xapplication_t   *application,
                                  xchar_t        ***arguments,
                                  xerror_t        **error)
{
  xboolean_t become_service = FALSE;
  xchar_t *app_id = NULL;
  xboolean_t replace = FALSE;
  xvariant_dict_t *dict = NULL;
  xoption_context_t *context;
  xoption_group_t *gapplication_group;

  /* Due to the memory management of xoption_group_t we can only parse
   * options once.  That's because once you add a group to the
   * xoption_context_t there is no way to get it back again.  This is fine:
   * local_command_line() should never get invoked more than once
   * anyway.  Add a sanity check just to be sure.
   */
  g_return_val_if_fail (!application->priv->options_parsed, NULL);

  context = g_option_context_new (application->priv->parameter_string);
  g_option_context_set_summary (context, application->priv->summary);
  g_option_context_set_description (context, application->priv->description);

  gapplication_group = xoption_group_new ("gapplication",
                                           _("xapplication_t options"), _("Show xapplication_t options"),
                                           NULL, NULL);
  xoption_group_set_translation_domain (gapplication_group, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gapplication_group);

  /* If the application has not registered local options and it has
   * G_APPLICATION_HANDLES_COMMAND_LINE then we have to assume that
   * their primary instance commandline handler may want to deal with
   * the arguments.  We must therefore ignore them.
   *
   * We must also ignore --help in this case since some applications
   * will try to handle this from the remote side.  See #737869.
   */
  if (application->priv->main_options == NULL && (application->priv->flags & G_APPLICATION_HANDLES_COMMAND_LINE))
    {
      g_option_context_set_ignore_unknown_options (context, TRUE);
      g_option_context_set_help_enabled (context, FALSE);
    }

  /* Add the main option group, if it exists */
  if (application->priv->main_options)
    {
      /* This consumes the main_options */
      g_option_context_set_main_group (context, application->priv->main_options);
      application->priv->main_options = NULL;
    }

  /* Add any other option groups if they exist.  Adding them to the
   * context will consume them, so we free the list as we go...
   */
  while (application->priv->option_groups)
    {
      g_option_context_add_group (context, application->priv->option_groups->data);
      application->priv->option_groups = xslist_delete_link (application->priv->option_groups,
                                                              application->priv->option_groups);
    }

  /* In the case that we are not explicitly marked as a service or a
   * launcher then we want to add the "--gapplication-service" option to
   * allow the process to be made into a service.
   */
  if ((application->priv->flags & (G_APPLICATION_IS_SERVICE | G_APPLICATION_IS_LAUNCHER)) == 0)
    {
      GOptionEntry entries[] = {
        { "gapplication-service", '\0', 0, G_OPTION_ARG_NONE, &become_service,
          N_("Enter xapplication_t service mode (use from D-Bus service files)"), NULL },
        G_OPTION_ENTRY_NULL
      };

      xoption_group_add_entries (gapplication_group, entries);
    }

  /* Allow overriding the ID if the application allows it */
  if (application->priv->flags & G_APPLICATION_CAN_OVERRIDE_APP_ID)
    {
      GOptionEntry entries[] = {
        { "gapplication-app-id", '\0', 0, G_OPTION_ARG_STRING, &app_id,
          N_("Override the application’s ID"), NULL },
        G_OPTION_ENTRY_NULL
      };

      xoption_group_add_entries (gapplication_group, entries);
    }

  /* Allow replacing if the application allows it */
  if (application->priv->flags & G_APPLICATION_ALLOW_REPLACEMENT)
    {
      GOptionEntry entries[] = {
        { "gapplication-replace", '\0', 0, G_OPTION_ARG_NONE, &replace,
          N_("Replace the running instance"), NULL },
        G_OPTION_ENTRY_NULL
      };

      xoption_group_add_entries (gapplication_group, entries);
    }

  /* Now we parse... */
  if (!g_option_context_parse_strv (context, arguments, error))
    goto out;

  /* Check for --gapplication-service */
  if (become_service)
    application->priv->flags |= G_APPLICATION_IS_SERVICE;

  /* Check for --gapplication-app-id */
  if (app_id)
    xapplication_set_application_id (application, app_id);

  /* Check for --gapplication-replace */
  if (replace)
    application->priv->flags |= G_APPLICATION_REPLACE;

  dict = xvariant_dict_new (NULL);
  if (application->priv->packed_options)
    {
      xapplication_pack_option_entries (application, dict);
      xhash_table_unref (application->priv->packed_options);
      application->priv->packed_options = NULL;
    }

out:
  /* Make sure we don't run again */
  application->priv->options_parsed = TRUE;

  g_option_context_free (context);
  g_free (app_id);

  return dict;
}

static void
add_packed_option (xapplication_t *application,
                   GOptionEntry *entry)
{
  switch (entry->arg)
    {
    case G_OPTION_ARG_NONE:
      entry->arg_data = g_new (xboolean_t, 1);
      *(xboolean_t *) entry->arg_data = 2;
      break;

    case G_OPTION_ARG_INT:
      entry->arg_data = g_new0 (xint_t, 1);
      break;

    case G_OPTION_ARG_STRING:
    case G_OPTION_ARXFILENAME:
    case G_OPTION_ARG_STRING_ARRAY:
    case G_OPTION_ARXFILENAME_ARRAY:
      entry->arg_data = g_new0 (xpointer_t, 1);
      break;

    case G_OPTION_ARG_INT64:
      entry->arg_data = g_new0 (gint64, 1);
      break;

    case G_OPTION_ARG_DOUBLE:
      entry->arg_data = g_new0 (xdouble_t, 1);
      break;

    default:
      g_return_if_reached ();
    }

  if (!application->priv->packed_options)
    application->priv->packed_options = xhash_table_new_full (xstr_hash, xstr_equal, g_free, free_option_entry);

  xhash_table_insert (application->priv->packed_options,
                       xstrdup (entry->long_name),
                       g_slice_dup (GOptionEntry, entry));
}

/**
 * xapplication_add_main_option_entries:
 * @application: a #xapplication_t
 * @entries: (array zero-terminated=1) (element-type GOptionEntry) a
 *           %NULL-terminated list of #GOptionEntrys
 *
 * Adds main option entries to be handled by @application.
 *
 * This function is comparable to g_option_context_add_main_entries().
 *
 * After the commandline arguments are parsed, the
 * #xapplication_t::handle-local-options signal will be emitted.  At this
 * point, the application can inspect the values pointed to by @arg_data
 * in the given #GOptionEntrys.
 *
 * Unlike #xoption_context_t, #xapplication_t supports giving a %NULL
 * @arg_data for a non-callback #GOptionEntry.  This results in the
 * argument in question being packed into a #xvariant_dict_t which is also
 * passed to #xapplication_t::handle-local-options, where it can be
 * inspected and modified.  If %G_APPLICATION_HANDLES_COMMAND_LINE is
 * set, then the resulting dictionary is sent to the primary instance,
 * where xapplication_command_line_get_options_dict() will return it.
 * This "packing" is done according to the type of the argument --
 * booleans for normal flags, strings for strings, bytestrings for
 * filenames, etc.  The packing only occurs if the flag is given (ie: we
 * do not pack a "false" #xvariant_t in the case that a flag is missing).
 *
 * In general, it is recommended that all commandline arguments are
 * parsed locally.  The options dictionary should then be used to
 * transmit the result of the parsing to the primary instance, where
 * xvariant_dict_lookup() can be used.  For local options, it is
 * possible to either use @arg_data in the usual way, or to consult (and
 * potentially remove) the option from the options dictionary.
 *
 * This function is new in GLib 2.40.  Before then, the only real choice
 * was to send all of the commandline arguments (options and all) to the
 * primary instance for handling.  #xapplication_t ignored them completely
 * on the local side.  Calling this function "opts in" to the new
 * behaviour, and in particular, means that unrecognised options will be
 * treated as errors.  Unrecognised options have never been ignored when
 * %G_APPLICATION_HANDLES_COMMAND_LINE is unset.
 *
 * If #xapplication_t::handle-local-options needs to see the list of
 * filenames, then the use of %G_OPTION_REMAINING is recommended.  If
 * @arg_data is %NULL then %G_OPTION_REMAINING can be used as a key into
 * the options dictionary.  If you do use %G_OPTION_REMAINING then you
 * need to handle these arguments for yourself because once they are
 * consumed, they will no longer be visible to the default handling
 * (which treats them as filenames to be opened).
 *
 * It is important to use the proper xvariant_t format when retrieving
 * the options with xvariant_dict_lookup():
 * - for %G_OPTION_ARG_NONE, use `b`
 * - for %G_OPTION_ARG_STRING, use `&s`
 * - for %G_OPTION_ARG_INT, use `i`
 * - for %G_OPTION_ARG_INT64, use `x`
 * - for %G_OPTION_ARG_DOUBLE, use `d`
 * - for %G_OPTION_ARXFILENAME, use `^&ay`
 * - for %G_OPTION_ARG_STRING_ARRAY, use `^a&s`
 * - for %G_OPTION_ARXFILENAME_ARRAY, use `^a&ay`
 *
 * Since: 2.40
 */
void
xapplication_add_main_option_entries (xapplication_t       *application,
                                       const GOptionEntry *entries)
{
  xint_t i;

  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (entries != NULL);

  if (!application->priv->main_options)
    {
      application->priv->main_options = xoption_group_new (NULL, NULL, NULL, NULL, NULL);
      xoption_group_set_translation_domain (application->priv->main_options, NULL);
    }

  for (i = 0; entries[i].long_name; i++)
    {
      GOptionEntry my_entries[2] =
        {
          G_OPTION_ENTRY_NULL,
          G_OPTION_ENTRY_NULL
        };
      my_entries[0] = entries[i];

      if (!my_entries[0].arg_data)
        add_packed_option (application, &my_entries[0]);

      xoption_group_add_entries (application->priv->main_options, my_entries);
    }
}

/**
 * xapplication_add_main_option:
 * @application: the #xapplication_t
 * @long_name: the long name of an option used to specify it in a commandline
 * @short_name: the short name of an option
 * @flags: flags from #GOptionFlags
 * @arg: the type of the option, as a #GOptionArg
 * @description: the description for the option in `--help` output
 * @arg_description: (nullable): the placeholder to use for the extra argument
 *    parsed by the option in `--help` output
 *
 * Add an option to be handled by @application.
 *
 * Calling this function is the equivalent of calling
 * xapplication_add_main_option_entries() with a single #GOptionEntry
 * that has its arg_data member set to %NULL.
 *
 * The parsed arguments will be packed into a #xvariant_dict_t which
 * is passed to #xapplication_t::handle-local-options. If
 * %G_APPLICATION_HANDLES_COMMAND_LINE is set, then it will also
 * be sent to the primary instance. See
 * xapplication_add_main_option_entries() for more details.
 *
 * See #GOptionEntry for more documentation of the arguments.
 *
 * Since: 2.42
 **/
void
xapplication_add_main_option (xapplication_t *application,
                               const char   *long_name,
                               char          short_name,
                               GOptionFlags  flags,
                               GOptionArg    arg,
                               const char   *description,
                               const char   *arg_description)
{
  xchar_t *dup_string;
  GOptionEntry my_entry[2] = {
    { NULL, short_name, flags, arg, NULL, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };

  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (long_name != NULL);
  g_return_if_fail (description != NULL);

  my_entry[0].long_name = dup_string = xstrdup (long_name);
  application->priv->option_strings = xslist_prepend (application->priv->option_strings, dup_string);

  my_entry[0].description = dup_string = xstrdup (description);
  application->priv->option_strings = xslist_prepend (application->priv->option_strings, dup_string);

  my_entry[0].arg_description = dup_string = xstrdup (arg_description);
  application->priv->option_strings = xslist_prepend (application->priv->option_strings, dup_string);

  xapplication_add_main_option_entries (application, my_entry);
}

/**
 * xapplication_add_option_group:
 * @application: the #xapplication_t
 * @group: (transfer full): a #xoption_group_t
 *
 * Adds a #xoption_group_t to the commandline handling of @application.
 *
 * This function is comparable to g_option_context_add_group().
 *
 * Unlike xapplication_add_main_option_entries(), this function does
 * not deal with %NULL @arg_data and never transmits options to the
 * primary instance.
 *
 * The reason for that is because, by the time the options arrive at the
 * primary instance, it is typically too late to do anything with them.
 * Taking the GTK option group as an example: GTK will already have been
 * initialised by the time the #xapplication_t::command-line handler runs.
 * In the case that this is not the first-running instance of the
 * application, the existing instance may already have been running for
 * a very long time.
 *
 * This means that the options from #xoption_group_t are only really usable
 * in the case that the instance of the application being run is the
 * first instance.  Passing options like `--display=` or `--gdk-debug=`
 * on future runs will have no effect on the existing primary instance.
 *
 * Calling this function will cause the options in the supplied option
 * group to be parsed, but it does not cause you to be "opted in" to the
 * new functionality whereby unrecognised options are rejected even if
 * %G_APPLICATION_HANDLES_COMMAND_LINE was given.
 *
 * Since: 2.40
 **/
void
xapplication_add_option_group (xapplication_t *application,
                                xoption_group_t *group)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (group != NULL);

  application->priv->option_groups = xslist_prepend (application->priv->option_groups, group);
}

/**
 * xapplication_set_option_context_parameter_string:
 * @application: the #xapplication_t
 * @parameter_string: (nullable): a string which is displayed
 *   in the first line of `--help` output, after the usage summary `programname [OPTION...]`.
 *
 * Sets the parameter string to be used by the commandline handling of @application.
 *
 * This function registers the argument to be passed to g_option_context_new()
 * when the internal #xoption_context_t of @application is created.
 *
 * See g_option_context_new() for more information about @parameter_string.
 *
 * Since: 2.56
 */
void
xapplication_set_option_context_parameter_string (xapplication_t *application,
                                                   const xchar_t  *parameter_string)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  g_free (application->priv->parameter_string);
  application->priv->parameter_string = xstrdup (parameter_string);
}

/**
 * xapplication_set_option_context_summary:
 * @application: the #xapplication_t
 * @summary: (nullable): a string to be shown in `--help` output
 *  before the list of options, or %NULL
 *
 * Adds a summary to the @application option context.
 *
 * See g_option_context_set_summary() for more information.
 *
 * Since: 2.56
 */
void
xapplication_set_option_context_summary (xapplication_t *application,
                                          const xchar_t  *summary)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  g_free (application->priv->summary);
  application->priv->summary = xstrdup (summary);
}

/**
 * xapplication_set_option_context_description:
 * @application: the #xapplication_t
 * @description: (nullable): a string to be shown in `--help` output
 *  after the list of options, or %NULL
 *
 * Adds a description to the @application option context.
 *
 * See g_option_context_set_description() for more information.
 *
 * Since: 2.56
 */
void
xapplication_set_option_context_description (xapplication_t *application,
                                              const xchar_t  *description)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  g_free (application->priv->description);
  application->priv->description = xstrdup (description);

}


/* vfunc defaults {{{1 */
static void
xapplication_real_before_emit (xapplication_t *application,
                                xvariant_t     *platform_data)
{
}

static void
xapplication_real_after_emit (xapplication_t *application,
                               xvariant_t     *platform_data)
{
}

static void
xapplication_real_startup (xapplication_t *application)
{
  application->priv->did_startup = TRUE;
}

static void
xapplication_real_shutdown (xapplication_t *application)
{
  application->priv->did_shutdown = TRUE;
}

static void
xapplication_real_activate (xapplication_t *application)
{
  if (!g_signal_has_handler_pending (application,
                                     xapplication_signals[SIGNAL_ACTIVATE],
                                     0, TRUE) &&
      G_APPLICATION_GET_CLASS (application)->activate == xapplication_real_activate)
    {
      static xboolean_t warned;

      if (warned)
        return;

      g_warning ("Your application does not implement "
                 "xapplication_activate() and has no handlers connected "
                 "to the 'activate' signal.  It should do one of these.");
      warned = TRUE;
    }
}

static void
xapplication_real_open (xapplication_t  *application,
                         xfile_t        **files,
                         xint_t           n_files,
                         const xchar_t   *hint)
{
  if (!g_signal_has_handler_pending (application,
                                     xapplication_signals[SIGNAL_OPEN],
                                     0, TRUE) &&
      G_APPLICATION_GET_CLASS (application)->open == xapplication_real_open)
    {
      static xboolean_t warned;

      if (warned)
        return;

      g_warning ("Your application claims to support opening files "
                 "but does not implement xapplication_open() and has no "
                 "handlers connected to the 'open' signal.");
      warned = TRUE;
    }
}

static int
xapplication_real_command_line (xapplication_t            *application,
                                 xapplication_command_line_t *cmdline)
{
  if (!g_signal_has_handler_pending (application,
                                     xapplication_signals[SIGNAL_COMMAND_LINE],
                                     0, TRUE) &&
      G_APPLICATION_GET_CLASS (application)->command_line == xapplication_real_command_line)
    {
      static xboolean_t warned;

      if (warned)
        return 1;

      g_warning ("Your application claims to support custom command line "
                 "handling but does not implement xapplication_command_line() "
                 "and has no handlers connected to the 'command-line' signal.");

      warned = TRUE;
    }

    return 1;
}

static xint_t
xapplication_real_handle_local_options (xapplication_t *application,
                                         xvariant_dict_t *options)
{
  return -1;
}

static xvariant_t *
get_platform_data (xapplication_t *application,
                   xvariant_t     *options)
{
  xvariant_builder_t *builder;
  xvariant_t *result;

  builder = xvariant_builder_new (G_VARIANT_TYPE ("a{sv}"));

  {
    xchar_t *cwd = g_get_current_dir ();
    xvariant_builder_add (builder, "{sv}", "cwd",
                           xvariant_new_bytestring (cwd));
    g_free (cwd);
  }

  if (application->priv->flags & G_APPLICATION_SEND_ENVIRONMENT)
    {
      xvariant_t *array;
      xchar_t **envp;

      envp = g_get_environ ();
      array = xvariant_new_bytestring_array ((const xchar_t **) envp, -1);
      xstrfreev (envp);

      xvariant_builder_add (builder, "{sv}", "environ", array);
    }

  if (options)
    xvariant_builder_add (builder, "{sv}", "options", options);

  G_APPLICATION_GET_CLASS (application)->
    add_platform_data (application, builder);

  result = xvariant_builder_end (builder);
  xvariant_builder_unref (builder);

  return result;
}

static void
xapplication_call_command_line (xapplication_t        *application,
                                 const xchar_t * const *arguments,
                                 xvariant_t            *options,
                                 xint_t                *exit_status)
{
  if (application->priv->is_remote)
    {
      xvariant_t *platform_data;

      platform_data = get_platform_data (application, options);
      *exit_status = xapplication_impl_command_line (application->priv->impl, arguments, platform_data);
    }
  else
    {
      xapplication_command_line_t *cmdline;
      xvariant_t *v;

      v = xvariant_new_bytestring_array ((const xchar_t **) arguments, -1);
      cmdline = xobject_new (XTYPE_APPLICATION_COMMAND_LINE,
                              "arguments", v,
                              "options", options,
                              NULL);
      g_signal_emit (application, xapplication_signals[SIGNAL_COMMAND_LINE], 0, cmdline, exit_status);
      xobject_unref (cmdline);
    }
}

static xboolean_t
xapplication_real_local_command_line (xapplication_t   *application,
                                       xchar_t        ***arguments,
                                       int            *exit_status)
{
  xerror_t *error = NULL;
  xvariant_dict_t *options;
  xint_t n_args;

  options = xapplication_parse_command_line (application, arguments, &error);
  if (!options)
    {
      g_printerr ("%s\n", error->message);
      xerror_free (error);
      *exit_status = 1;
      return TRUE;
    }

  g_signal_emit (application, xapplication_signals[SIGNAL_HANDLE_LOCAL_OPTIONS], 0, options, exit_status);

  if (*exit_status >= 0)
    {
      xvariant_dict_unref (options);
      return TRUE;
    }

  if (!xapplication_register (application, NULL, &error))
    {
      g_printerr ("Failed to register: %s\n", error->message);
      xvariant_dict_unref (options);
      xerror_free (error);
      *exit_status = 1;
      return TRUE;
    }

  n_args = xstrv_length (*arguments);

  if (application->priv->flags & G_APPLICATION_IS_SERVICE)
    {
      if ((*exit_status = n_args > 1))
        {
          g_printerr ("xapplication_t service mode takes no arguments.\n");
          application->priv->flags &= ~G_APPLICATION_IS_SERVICE;
          *exit_status = 1;
        }
      else
        *exit_status = 0;
    }
  else if (application->priv->flags & G_APPLICATION_HANDLES_COMMAND_LINE)
    {
      xapplication_call_command_line (application,
                                       (const xchar_t **) *arguments,
                                       xvariant_dict_end (options),
                                       exit_status);
    }
  else
    {
      if (n_args <= 1)
        {
          xapplication_activate (application);
          *exit_status = 0;
        }

      else
        {
          if (~application->priv->flags & G_APPLICATION_HANDLES_OPEN)
            {
              g_critical ("This application can not open files.");
              *exit_status = 1;
            }
          else
            {
              xfile_t **files;
              xint_t n_files;
              xint_t i;

              n_files = n_args - 1;
              files = g_new (xfile_t *, n_files);

              for (i = 0; i < n_files; i++)
                files[i] = xfile_new_for_commandline_arg ((*arguments)[i + 1]);

              xapplication_open (application, files, n_files, "");

              for (i = 0; i < n_files; i++)
                xobject_unref (files[i]);
              g_free (files);

              *exit_status = 0;
            }
        }
    }

  xvariant_dict_unref (options);

  return TRUE;
}

static void
xapplication_real_add_platform_data (xapplication_t    *application,
                                      xvariant_builder_t *builder)
{
}

static xboolean_t
xapplication_real_dbus_register (xapplication_t    *application,
                                  xdbus_connection_t *connection,
                                  const xchar_t     *object_path,
                                  xerror_t         **error)
{
  return TRUE;
}

static void
xapplication_real_dbus_unregister (xapplication_t    *application,
                                    xdbus_connection_t *connection,
                                    const xchar_t     *object_path)
{
}

static xboolean_t
xapplication_real_name_lost (xapplication_t *application)
{
  xapplication_quit (application);
  return TRUE;
}

/* xobject_t implementation stuff {{{1 */
static void
xapplication_set_property (xobject_t      *object,
                            xuint_t         prop_id,
                            const xvalue_t *value,
                            xparam_spec_t   *pspec)
{
  xapplication_t *application = G_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_APPLICATION_ID:
      xapplication_set_application_id (application,
                                        xvalue_get_string (value));
      break;

    case PROP_FLAGS:
      xapplication_set_flags (application, xvalue_get_flags (value));
      break;

    case PROP_RESOURCE_BASE_PATH:
      xapplication_set_resource_base_path (application, xvalue_get_string (value));
      break;

    case PROP_INACTIVITY_TIMEOUT:
      xapplication_set_inactivity_timeout (application,
                                            xvalue_get_uint (value));
      break;

    case PROP_ACTION_GROUP:
      g_clear_object (&application->priv->actions);
      application->priv->actions = xvalue_dup_object (value);
      break;

    default:
      g_assert_not_reached ();
    }
}

/**
 * xapplication_set_action_group:
 * @application: a #xapplication_t
 * @action_group: (nullable): a #xaction_group_t, or %NULL
 *
 * This used to be how actions were associated with a #xapplication_t.
 * Now there is #xaction_map_t for that.
 *
 * Since: 2.28
 *
 * Deprecated:2.32:Use the #xaction_map_t interface instead.  Never ever
 * mix use of this API with use of #xaction_map_t on the same @application
 * or things will go very badly wrong.  This function is known to
 * introduce buggy behaviour (ie: signals not emitted on changes to the
 * action group), so you should really use #xaction_map_t instead.
 **/
void
xapplication_set_action_group (xapplication_t *application,
                                xaction_group_t *action_group)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (!application->priv->is_registered);

  if (application->priv->actions != NULL)
    xobject_unref (application->priv->actions);

  application->priv->actions = action_group;

  if (application->priv->actions != NULL)
    xobject_ref (application->priv->actions);
}

static void
xapplication_get_property (xobject_t    *object,
                            xuint_t       prop_id,
                            xvalue_t     *value,
                            xparam_spec_t *pspec)
{
  xapplication_t *application = G_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_APPLICATION_ID:
      xvalue_set_string (value,
                          xapplication_get_application_id (application));
      break;

    case PROP_FLAGS:
      xvalue_set_flags (value,
                         xapplication_get_flags (application));
      break;

    case PROP_RESOURCE_BASE_PATH:
      xvalue_set_string (value, xapplication_get_resource_base_path (application));
      break;

    case PROP_IS_REGISTERED:
      xvalue_set_boolean (value,
                           xapplication_get_is_registered (application));
      break;

    case PROP_IS_REMOTE:
      xvalue_set_boolean (value,
                           xapplication_get_is_remote (application));
      break;

    case PROP_INACTIVITY_TIMEOUT:
      xvalue_set_uint (value,
                        xapplication_get_inactivity_timeout (application));
      break;

    case PROP_IS_BUSY:
      xvalue_set_boolean (value, xapplication_get_is_busy (application));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
xapplication_constructed (xobject_t *object)
{
  xapplication_t *application = G_APPLICATION (object);

  if (xapplication_get_default () == NULL)
    xapplication_set_default (application);

  /* People should not set properties from _init... */
  g_assert (application->priv->resource_path == NULL);

  if (application->priv->id != NULL)
    {
      xint_t i;

      application->priv->resource_path = xstrconcat ("/", application->priv->id, NULL);

      for (i = 1; application->priv->resource_path[i]; i++)
        if (application->priv->resource_path[i] == '.')
          application->priv->resource_path[i] = '/';
    }
}

static void
xapplication_dispose (xobject_t *object)
{
  xapplication_t *application = G_APPLICATION (object);

  if (application->priv->impl != NULL &&
      G_APPLICATION_GET_CLASS (application)->dbus_unregister != xapplication_real_dbus_unregister)
    {
      static xboolean_t warned;

      if (!warned)
        {
          g_warning ("Your application did not unregister from D-Bus before destruction. "
                     "Consider using xapplication_run().");
        }

      warned = TRUE;
    }

  G_OBJECT_CLASS (xapplication_parent_class)->dispose (object);
}

static void
xapplication_finalize (xobject_t *object)
{
  xapplication_t *application = G_APPLICATION (object);

  if (application->priv->inactivity_timeout_id)
    xsource_remove (application->priv->inactivity_timeout_id);

  xslist_free_full (application->priv->option_groups, (xdestroy_notify_t) xoption_group_unref);
  if (application->priv->main_options)
    xoption_group_unref (application->priv->main_options);
  if (application->priv->packed_options)
    xhash_table_unref (application->priv->packed_options);

  g_free (application->priv->parameter_string);
  g_free (application->priv->summary);
  g_free (application->priv->description);

  xslist_free_full (application->priv->option_strings, g_free);

  if (application->priv->impl)
    xapplication_impl_destroy (application->priv->impl);
  g_free (application->priv->id);

  if (xapplication_get_default () == application)
    xapplication_set_default (NULL);

  if (application->priv->actions)
    xobject_unref (application->priv->actions);

  g_clear_object (&application->priv->remote_actions);

  if (application->priv->notifications)
    xobject_unref (application->priv->notifications);

  g_free (application->priv->resource_path);

  G_OBJECT_CLASS (xapplication_parent_class)
    ->finalize (object);
}

static void
xapplication_init (xapplication_t *application)
{
  application->priv = xapplication_get_instance_private (application);

  application->priv->actions = xapplication_exported_actions_new (application);

  /* application->priv->actions is the one and only ref on the group, so when
   * we dispose, the action group will die, disconnecting all signals.
   */
  g_signal_connect_swapped (application->priv->actions, "action-added",
                            G_CALLBACK (xaction_group_action_added), application);
  g_signal_connect_swapped (application->priv->actions, "action-enabled-changed",
                            G_CALLBACK (xaction_group_action_enabled_changed), application);
  g_signal_connect_swapped (application->priv->actions, "action-state-changed",
                            G_CALLBACK (xaction_group_action_state_changed), application);
  g_signal_connect_swapped (application->priv->actions, "action-removed",
                            G_CALLBACK (xaction_group_action_removed), application);
}

static xboolean_t
xapplication_handle_local_options_accumulator (xsignal_invocation_hint_t *ihint,
                                                xvalue_t                *return_accu,
                                                const xvalue_t          *handler_return,
                                                xpointer_t               dummy)
{
  xint_t value;

  value = xvalue_get_int (handler_return);
  xvalue_set_int (return_accu, value);

  return value < 0;
}

static void
xapplication_class_init (xapplication_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->constructed = xapplication_constructed;
  object_class->dispose = xapplication_dispose;
  object_class->finalize = xapplication_finalize;
  object_class->get_property = xapplication_get_property;
  object_class->set_property = xapplication_set_property;

  class->before_emit = xapplication_real_before_emit;
  class->after_emit = xapplication_real_after_emit;
  class->startup = xapplication_real_startup;
  class->shutdown = xapplication_real_shutdown;
  class->activate = xapplication_real_activate;
  class->open = xapplication_real_open;
  class->command_line = xapplication_real_command_line;
  class->local_command_line = xapplication_real_local_command_line;
  class->handle_local_options = xapplication_real_handle_local_options;
  class->add_platform_data = xapplication_real_add_platform_data;
  class->dbus_register = xapplication_real_dbus_register;
  class->dbus_unregister = xapplication_real_dbus_unregister;
  class->name_lost = xapplication_real_name_lost;

  xobject_class_install_property (object_class, PROP_APPLICATION_ID,
    g_param_spec_string ("application-id",
                         P_("Application identifier"),
                         P_("The unique identifier for the application"),
                         NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_FLAGS,
    g_param_spec_flags ("flags",
                        P_("Application flags"),
                        P_("Flags specifying the behaviour of the application"),
                        XTYPE_APPLICATION_FLAGS, G_APPLICATION_FLAGS_NONE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_RESOURCE_BASE_PATH,
    g_param_spec_string ("resource-base-path",
                         P_("Resource base path"),
                         P_("The base resource path for the application"),
                         NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_IS_REGISTERED,
    g_param_spec_boolean ("is-registered",
                          P_("Is registered"),
                          P_("If xapplication_register() has been called"),
                          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_IS_REMOTE,
    g_param_spec_boolean ("is-remote",
                          P_("Is remote"),
                          P_("If this application instance is remote"),
                          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_INACTIVITY_TIMEOUT,
    g_param_spec_uint ("inactivity-timeout",
                       P_("Inactivity timeout"),
                       P_("Time (ms) to stay alive after becoming idle"),
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_ACTION_GROUP,
    g_param_spec_object ("action-group",
                         P_("Action group"),
                         P_("The group of actions that the application exports"),
                         XTYPE_ACTION_GROUP,
                         G_PARAM_DEPRECATED | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  /**
   * xapplication_t:is-busy:
   *
   * Whether the application is currently marked as busy through
   * xapplication_mark_busy() or xapplication_bind_busy_property().
   *
   * Since: 2.44
   */
  xobject_class_install_property (object_class, PROP_IS_BUSY,
    g_param_spec_boolean ("is-busy",
                          P_("Is busy"),
                          P_("If this application is currently marked busy"),
                          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * xapplication_t::startup:
   * @application: the application
   *
   * The ::startup signal is emitted on the primary instance immediately
   * after registration. See xapplication_register().
   */
  xapplication_signals[SIGNAL_STARTUP] =
    g_signal_new (I_("startup"), XTYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (xapplication_class_t, startup),
                  NULL, NULL, NULL, XTYPE_NONE, 0);

  /**
   * xapplication_t::shutdown:
   * @application: the application
   *
   * The ::shutdown signal is emitted only on the registered primary instance
   * immediately after the main loop terminates.
   */
  xapplication_signals[SIGNAL_SHUTDOWN] =
    g_signal_new (I_("shutdown"), XTYPE_APPLICATION, G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xapplication_class_t, shutdown),
                  NULL, NULL, NULL, XTYPE_NONE, 0);

  /**
   * xapplication_t::activate:
   * @application: the application
   *
   * The ::activate signal is emitted on the primary instance when an
   * activation occurs. See xapplication_activate().
   */
  xapplication_signals[SIGNAL_ACTIVATE] =
    g_signal_new (I_("activate"), XTYPE_APPLICATION, G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xapplication_class_t, activate),
                  NULL, NULL, NULL, XTYPE_NONE, 0);


  /**
   * xapplication_t::open:
   * @application: the application
   * @files: (array length=n_files) (element-type xfile_t): an array of #GFiles
   * @n_files: the length of @files
   * @hint: a hint provided by the calling instance
   *
   * The ::open signal is emitted on the primary instance when there are
   * files to open. See xapplication_open() for more information.
   */
  xapplication_signals[SIGNAL_OPEN] =
    g_signal_new (I_("open"), XTYPE_APPLICATION, G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xapplication_class_t, open),
                  NULL, NULL,
                  _g_cclosure_marshal_VOID__POINTER_INT_STRING,
                  XTYPE_NONE, 3, XTYPE_POINTER, XTYPE_INT, XTYPE_STRING);
  g_signal_set_va_marshaller (xapplication_signals[SIGNAL_OPEN],
                              XTYPE_FROM_CLASS (class),
                              _g_cclosure_marshal_VOID__POINTER_INT_STRINGv);

  /**
   * xapplication_t::command-line:
   * @application: the application
   * @command_line: a #xapplication_command_line_t representing the
   *     passed commandline
   *
   * The ::command-line signal is emitted on the primary instance when
   * a commandline is not handled locally. See xapplication_run() and
   * the #xapplication_command_line_t documentation for more information.
   *
   * Returns: An integer that is set as the exit status for the calling
   *   process. See xapplication_command_line_set_exit_status().
   */
  xapplication_signals[SIGNAL_COMMAND_LINE] =
    g_signal_new (I_("command-line"), XTYPE_APPLICATION, G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xapplication_class_t, command_line),
                  g_signal_accumulator_first_wins, NULL,
                  _g_cclosure_marshal_INT__OBJECT,
                  XTYPE_INT, 1, XTYPE_APPLICATION_COMMAND_LINE);
  g_signal_set_va_marshaller (xapplication_signals[SIGNAL_COMMAND_LINE],
                              XTYPE_FROM_CLASS (class),
                              _g_cclosure_marshal_INT__OBJECTv);

  /**
   * xapplication_t::handle-local-options:
   * @application: the application
   * @options: the options dictionary
   *
   * The ::handle-local-options signal is emitted on the local instance
   * after the parsing of the commandline options has occurred.
   *
   * You can add options to be recognised during commandline option
   * parsing using xapplication_add_main_option_entries() and
   * xapplication_add_option_group().
   *
   * Signal handlers can inspect @options (along with values pointed to
   * from the @arg_data of an installed #GOptionEntrys) in order to
   * decide to perform certain actions, including direct local handling
   * (which may be useful for options like --version).
   *
   * In the event that the application is marked
   * %G_APPLICATION_HANDLES_COMMAND_LINE the "normal processing" will
   * send the @options dictionary to the primary instance where it can be
   * read with xapplication_command_line_get_options_dict().  The signal
   * handler can modify the dictionary before returning, and the
   * modified dictionary will be sent.
   *
   * In the event that %G_APPLICATION_HANDLES_COMMAND_LINE is not set,
   * "normal processing" will treat the remaining uncollected command
   * line arguments as filenames or URIs.  If there are no arguments,
   * the application is activated by xapplication_activate().  One or
   * more arguments results in a call to xapplication_open().
   *
   * If you want to handle the local commandline arguments for yourself
   * by converting them to calls to xapplication_open() or
   * xaction_group_activate_action() then you must be sure to register
   * the application first.  You should probably not call
   * xapplication_activate() for yourself, however: just return -1 and
   * allow the default handler to do it for you.  This will ensure that
   * the `--gapplication-service` switch works properly (i.e. no activation
   * in that case).
   *
   * Note that this signal is emitted from the default implementation of
   * local_command_line().  If you override that function and don't
   * chain up then this signal will never be emitted.
   *
   * You can override local_command_line() if you need more powerful
   * capabilities than what is provided here, but this should not
   * normally be required.
   *
   * Returns: an exit code. If you have handled your options and want
   * to exit the process, return a non-negative option, 0 for success,
   * and a positive value for failure. To continue, return -1 to let
   * the default option processing continue.
   *
   * Since: 2.40
   **/
  xapplication_signals[SIGNAL_HANDLE_LOCAL_OPTIONS] =
    g_signal_new (I_("handle-local-options"), XTYPE_APPLICATION, G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xapplication_class_t, handle_local_options),
                  xapplication_handle_local_options_accumulator, NULL,
                  _g_cclosure_marshal_INT__BOXED,
                  XTYPE_INT, 1, XTYPE_VARIANT_DICT);
  g_signal_set_va_marshaller (xapplication_signals[SIGNAL_HANDLE_LOCAL_OPTIONS],
                              XTYPE_FROM_CLASS (class),
                              _g_cclosure_marshal_INT__BOXEDv);

  /**
   * xapplication_t::name-lost:
   * @application: the application
   *
   * The ::name-lost signal is emitted only on the registered primary instance
   * when a new instance has taken over. This can only happen if the application
   * is using the %G_APPLICATION_ALLOW_REPLACEMENT flag.
   *
   * The default handler for this signal calls xapplication_quit().
   *
   * Returns: %TRUE if the signal has been handled
   *
   * Since: 2.60
   */
  xapplication_signals[SIGNAL_NAME_LOST] =
    g_signal_new (I_("name-lost"), XTYPE_APPLICATION, G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (xapplication_class_t, name_lost),
                  g_signal_accumulator_true_handled, NULL,
                  _g_cclosure_marshal_BOOLEAN__VOID,
                  XTYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (xapplication_signals[SIGNAL_NAME_LOST],
                              XTYPE_FROM_CLASS (class),
                              _g_cclosure_marshal_BOOLEAN__VOIDv);
}

/* Application ID validity {{{1 */

/**
 * xapplication_id_is_valid:
 * @application_id: a potential application identifier
 *
 * Checks if @application_id is a valid application identifier.
 *
 * A valid ID is required for calls to xapplication_new() and
 * xapplication_set_application_id().
 *
 * Application identifiers follow the same format as
 * [D-Bus well-known bus names](https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names-bus).
 * For convenience, the restrictions on application identifiers are
 * reproduced here:
 *
 * - Application identifiers are composed of 1 or more elements separated by a
 *   period (`.`) character. All elements must contain at least one character.
 *
 * - Each element must only contain the ASCII characters `[A-Z][a-z][0-9]_-`,
 *   with `-` discouraged in new application identifiers. Each element must not
 *   begin with a digit.
 *
 * - Application identifiers must contain at least one `.` (period) character
 *   (and thus at least two elements).
 *
 * - Application identifiers must not begin with a `.` (period) character.
 *
 * - Application identifiers must not exceed 255 characters.
 *
 * Note that the hyphen (`-`) character is allowed in application identifiers,
 * but is problematic or not allowed in various specifications and APIs that
 * refer to D-Bus, such as
 * [Flatpak application IDs](http://docs.flatpak.org/en/latest/introduction.html#identifiers),
 * the
 * [`DBusActivatable` interface in the Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#dbus),
 * and the convention that an application's "main" interface and object path
 * resemble its application identifier and bus name. To avoid situations that
 * require special-case handling, it is recommended that new application
 * identifiers consistently replace hyphens with underscores.
 *
 * Like D-Bus interface names, application identifiers should start with the
 * reversed DNS domain name of the author of the interface (in lower-case), and
 * it is conventional for the rest of the application identifier to consist of
 * words run together, with initial capital letters.
 *
 * As with D-Bus interface names, if the author's DNS domain name contains
 * hyphen/minus characters they should be replaced by underscores, and if it
 * contains leading digits they should be escaped by prepending an underscore.
 * For example, if the owner of 7-zip.org used an application identifier for an
 * archiving application, it might be named `org._7_zip.Archiver`.
 *
 * Returns: %TRUE if @application_id is valid
 */
xboolean_t
xapplication_id_is_valid (const xchar_t *application_id)
{
  return g_dbus_is_name (application_id) &&
         !g_dbus_is_unique_name (application_id);
}

/* Public Constructor {{{1 */
/**
 * xapplication_new:
 * @application_id: (nullable): the application id
 * @flags: the application flags
 *
 * Creates a new #xapplication_t instance.
 *
 * If non-%NULL, the application id must be valid.  See
 * xapplication_id_is_valid().
 *
 * If no application ID is given then some features of #xapplication_t
 * (most notably application uniqueness) will be disabled.
 *
 * Returns: a new #xapplication_t instance
 **/
xapplication_t *
xapplication_new (const xchar_t       *application_id,
                   GApplicationFlags  flags)
{
  g_return_val_if_fail (application_id == NULL || xapplication_id_is_valid (application_id), NULL);

  return xobject_new (XTYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

/* Simple get/set: application id, flags, inactivity timeout {{{1 */
/**
 * xapplication_get_application_id:
 * @application: a #xapplication_t
 *
 * Gets the unique identifier for @application.
 *
 * Returns: (nullable): the identifier for @application, owned by @application
 *
 * Since: 2.28
 **/
const xchar_t *
xapplication_get_application_id (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), NULL);

  return application->priv->id;
}

/**
 * xapplication_set_application_id:
 * @application: a #xapplication_t
 * @application_id: (nullable): the identifier for @application
 *
 * Sets the unique identifier for @application.
 *
 * The application id can only be modified if @application has not yet
 * been registered.
 *
 * If non-%NULL, the application id must be valid.  See
 * xapplication_id_is_valid().
 *
 * Since: 2.28
 **/
void
xapplication_set_application_id (xapplication_t *application,
                                  const xchar_t  *application_id)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  if (xstrcmp0 (application->priv->id, application_id) != 0)
    {
      g_return_if_fail (application_id == NULL || xapplication_id_is_valid (application_id));
      g_return_if_fail (!application->priv->is_registered);

      g_free (application->priv->id);
      application->priv->id = xstrdup (application_id);

      xobject_notify (G_OBJECT (application), "application-id");
    }
}

/**
 * xapplication_get_flags:
 * @application: a #xapplication_t
 *
 * Gets the flags for @application.
 *
 * See #GApplicationFlags.
 *
 * Returns: the flags for @application
 *
 * Since: 2.28
 **/
GApplicationFlags
xapplication_get_flags (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), 0);

  return application->priv->flags;
}

/**
 * xapplication_set_flags:
 * @application: a #xapplication_t
 * @flags: the flags for @application
 *
 * Sets the flags for @application.
 *
 * The flags can only be modified if @application has not yet been
 * registered.
 *
 * See #GApplicationFlags.
 *
 * Since: 2.28
 **/
void
xapplication_set_flags (xapplication_t      *application,
                         GApplicationFlags  flags)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  if (application->priv->flags != flags)
    {
      g_return_if_fail (!application->priv->is_registered);

      application->priv->flags = flags;

      xobject_notify (G_OBJECT (application), "flags");
    }
}

/**
 * xapplication_get_resource_base_path:
 * @application: a #xapplication_t
 *
 * Gets the resource base path of @application.
 *
 * See xapplication_set_resource_base_path() for more information.
 *
 * Returns: (nullable): the base resource path, if one is set
 *
 * Since: 2.42
 */
const xchar_t *
xapplication_get_resource_base_path (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), NULL);

  return application->priv->resource_path;
}

/**
 * xapplication_set_resource_base_path:
 * @application: a #xapplication_t
 * @resource_path: (nullable): the resource path to use
 *
 * Sets (or unsets) the base resource path of @application.
 *
 * The path is used to automatically load various [application
 * resources][gresource] such as menu layouts and action descriptions.
 * The various types of resources will be found at fixed names relative
 * to the given base path.
 *
 * By default, the resource base path is determined from the application
 * ID by prefixing '/' and replacing each '.' with '/'.  This is done at
 * the time that the #xapplication_t object is constructed.  Changes to
 * the application ID after that point will not have an impact on the
 * resource base path.
 *
 * As an example, if the application has an ID of "org.example.app" then
 * the default resource base path will be "/org/example/app".  If this
 * is a #GtkApplication (and you have not manually changed the path)
 * then Gtk will then search for the menus of the application at
 * "/org/example/app/gtk/menus.ui".
 *
 * See #xresource_t for more information about adding resources to your
 * application.
 *
 * You can disable automatic resource loading functionality by setting
 * the path to %NULL.
 *
 * Changing the resource base path once the application is running is
 * not recommended.  The point at which the resource path is consulted
 * for forming paths for various purposes is unspecified.  When writing
 * a sub-class of #xapplication_t you should either set the
 * #xapplication_t:resource-base-path property at construction time, or call
 * this function during the instance initialization. Alternatively, you
 * can call this function in the #xapplication_class_t.startup virtual function,
 * before chaining up to the parent implementation.
 *
 * Since: 2.42
 */
void
xapplication_set_resource_base_path (xapplication_t *application,
                                      const xchar_t  *resource_path)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (resource_path == NULL || xstr_has_prefix (resource_path, "/"));

  if (xstrcmp0 (application->priv->resource_path, resource_path) != 0)
    {
      g_free (application->priv->resource_path);

      application->priv->resource_path = xstrdup (resource_path);

      xobject_notify (G_OBJECT (application), "resource-base-path");
    }
}

/**
 * xapplication_get_inactivity_timeout:
 * @application: a #xapplication_t
 *
 * Gets the current inactivity timeout for the application.
 *
 * This is the amount of time (in milliseconds) after the last call to
 * xapplication_release() before the application stops running.
 *
 * Returns: the timeout, in milliseconds
 *
 * Since: 2.28
 **/
xuint_t
xapplication_get_inactivity_timeout (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), 0);

  return application->priv->inactivity_timeout;
}

/**
 * xapplication_set_inactivity_timeout:
 * @application: a #xapplication_t
 * @inactivity_timeout: the timeout, in milliseconds
 *
 * Sets the current inactivity timeout for the application.
 *
 * This is the amount of time (in milliseconds) after the last call to
 * xapplication_release() before the application stops running.
 *
 * This call has no side effects of its own.  The value set here is only
 * used for next time xapplication_release() drops the use count to
 * zero.  Any timeouts currently in progress are not impacted.
 *
 * Since: 2.28
 **/
void
xapplication_set_inactivity_timeout (xapplication_t *application,
                                      xuint_t         inactivity_timeout)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  if (application->priv->inactivity_timeout != inactivity_timeout)
    {
      application->priv->inactivity_timeout = inactivity_timeout;

      xobject_notify (G_OBJECT (application), "inactivity-timeout");
    }
}
/* Read-only property getters (is registered, is remote, dbus stuff) {{{1 */
/**
 * xapplication_get_is_registered:
 * @application: a #xapplication_t
 *
 * Checks if @application is registered.
 *
 * An application is registered if xapplication_register() has been
 * successfully called.
 *
 * Returns: %TRUE if @application is registered
 *
 * Since: 2.28
 **/
xboolean_t
xapplication_get_is_registered (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), FALSE);

  return application->priv->is_registered;
}

/**
 * xapplication_get_is_remote:
 * @application: a #xapplication_t
 *
 * Checks if @application is remote.
 *
 * If @application is remote then it means that another instance of
 * application already exists (the 'primary' instance).  Calls to
 * perform actions on @application will result in the actions being
 * performed by the primary instance.
 *
 * The value of this property cannot be accessed before
 * xapplication_register() has been called.  See
 * xapplication_get_is_registered().
 *
 * Returns: %TRUE if @application is remote
 *
 * Since: 2.28
 **/
xboolean_t
xapplication_get_is_remote (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (application->priv->is_registered, FALSE);

  return application->priv->is_remote;
}

/**
 * xapplication_get_dbus_connection:
 * @application: a #xapplication_t
 *
 * Gets the #xdbus_connection_t being used by the application, or %NULL.
 *
 * If #xapplication_t is using its D-Bus backend then this function will
 * return the #xdbus_connection_t being used for uniqueness and
 * communication with the desktop environment and other instances of the
 * application.
 *
 * If #xapplication_t is not using D-Bus then this function will return
 * %NULL.  This includes the situation where the D-Bus backend would
 * normally be in use but we were unable to connect to the bus.
 *
 * This function must not be called before the application has been
 * registered.  See xapplication_get_is_registered().
 *
 * Returns: (nullable) (transfer none): a #xdbus_connection_t, or %NULL
 *
 * Since: 2.34
 **/
xdbus_connection_t *
xapplication_get_dbus_connection (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (application->priv->is_registered, FALSE);

  return xapplication_impl_get_dbus_connection (application->priv->impl);
}

/**
 * xapplication_get_dbus_object_path:
 * @application: a #xapplication_t
 *
 * Gets the D-Bus object path being used by the application, or %NULL.
 *
 * If #xapplication_t is using its D-Bus backend then this function will
 * return the D-Bus object path that #xapplication_t is using.  If the
 * application is the primary instance then there is an object published
 * at this path.  If the application is not the primary instance then
 * the result of this function is undefined.
 *
 * If #xapplication_t is not using D-Bus then this function will return
 * %NULL.  This includes the situation where the D-Bus backend would
 * normally be in use but we were unable to connect to the bus.
 *
 * This function must not be called before the application has been
 * registered.  See xapplication_get_is_registered().
 *
 * Returns: (nullable): the object path, or %NULL
 *
 * Since: 2.34
 **/
const xchar_t *
xapplication_get_dbus_object_path (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (application->priv->is_registered, FALSE);

  return xapplication_impl_get_dbus_object_path (application->priv->impl);
}


/* Register {{{1 */
/**
 * xapplication_register:
 * @application: a #xapplication_t
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a pointer to a NULL #xerror_t, or %NULL
 *
 * Attempts registration of the application.
 *
 * This is the point at which the application discovers if it is the
 * primary instance or merely acting as a remote for an already-existing
 * primary instance.  This is implemented by attempting to acquire the
 * application identifier as a unique bus name on the session bus using
 * GDBus.
 *
 * If there is no application ID or if %G_APPLICATION_NON_UNIQUE was
 * given, then this process will always become the primary instance.
 *
 * Due to the internal architecture of GDBus, method calls can be
 * dispatched at any time (even if a main loop is not running).  For
 * this reason, you must ensure that any object paths that you wish to
 * register are registered before calling this function.
 *
 * If the application has already been registered then %TRUE is
 * returned with no work performed.
 *
 * The #xapplication_t::startup signal is emitted if registration succeeds
 * and @application is the primary instance (including the non-unique
 * case).
 *
 * In the event of an error (such as @cancellable being cancelled, or a
 * failure to connect to the session bus), %FALSE is returned and @error
 * is set appropriately.
 *
 * Note: the return value of this function is not an indicator that this
 * instance is or is not the primary instance of the application.  See
 * xapplication_get_is_remote() for that.
 *
 * Returns: %TRUE if registration succeeded
 *
 * Since: 2.28
 **/
xboolean_t
xapplication_register (xapplication_t  *application,
                        xcancellable_t  *cancellable,
                        xerror_t       **error)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), FALSE);

  if (!application->priv->is_registered)
    {
      if (application->priv->id == NULL)
        application->priv->flags |= G_APPLICATION_NON_UNIQUE;

      application->priv->impl =
        xapplication_impl_register (application, application->priv->id,
                                     application->priv->flags,
                                     application->priv->actions,
                                     &application->priv->remote_actions,
                                     cancellable, error);

      if (application->priv->impl == NULL)
        return FALSE;

      application->priv->is_remote = application->priv->remote_actions != NULL;
      application->priv->is_registered = TRUE;

      xobject_notify (G_OBJECT (application), "is-registered");

      if (!application->priv->is_remote)
        {
          g_signal_emit (application, xapplication_signals[SIGNAL_STARTUP], 0);

          if (!application->priv->did_startup)
            g_critical ("xapplication_t subclass '%s' failed to chain up on"
                        " ::startup (from start of override function)",
                        G_OBJECT_TYPE_NAME (application));
        }
    }

  return TRUE;
}

/* Hold/release {{{1 */
/**
 * xapplication_hold:
 * @application: a #xapplication_t
 *
 * Increases the use count of @application.
 *
 * Use this function to indicate that the application has a reason to
 * continue to run.  For example, xapplication_hold() is called by GTK+
 * when a toplevel window is on the screen.
 *
 * To cancel the hold, call xapplication_release().
 **/
void
xapplication_hold (xapplication_t *application)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  if (application->priv->inactivity_timeout_id)
    {
      xsource_remove (application->priv->inactivity_timeout_id);
      application->priv->inactivity_timeout_id = 0;
    }

  application->priv->use_count++;
}

static xboolean_t
inactivity_timeout_expired (xpointer_t data)
{
  xapplication_t *application = G_APPLICATION (data);

  application->priv->inactivity_timeout_id = 0;

  return G_SOURCE_REMOVE;
}


/**
 * xapplication_release:
 * @application: a #xapplication_t
 *
 * Decrease the use count of @application.
 *
 * When the use count reaches zero, the application will stop running.
 *
 * Never call this function except to cancel the effect of a previous
 * call to xapplication_hold().
 **/
void
xapplication_release (xapplication_t *application)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (application->priv->use_count > 0);

  application->priv->use_count--;

  if (application->priv->use_count == 0 && application->priv->inactivity_timeout)
    application->priv->inactivity_timeout_id = g_timeout_add (application->priv->inactivity_timeout,
                                                              inactivity_timeout_expired, application);
}

/* Activate, Open {{{1 */
/**
 * xapplication_activate:
 * @application: a #xapplication_t
 *
 * Activates the application.
 *
 * In essence, this results in the #xapplication_t::activate signal being
 * emitted in the primary instance.
 *
 * The application must be registered before calling this function.
 *
 * Since: 2.28
 **/
void
xapplication_activate (xapplication_t *application)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (application->priv->is_registered);

  if (application->priv->is_remote)
    xapplication_impl_activate (application->priv->impl,
                                 get_platform_data (application, NULL));

  else
    g_signal_emit (application, xapplication_signals[SIGNAL_ACTIVATE], 0);
}

/**
 * xapplication_open:
 * @application: a #xapplication_t
 * @files: (array length=n_files): an array of #GFiles to open
 * @n_files: the length of the @files array
 * @hint: a hint (or ""), but never %NULL
 *
 * Opens the given files.
 *
 * In essence, this results in the #xapplication_t::open signal being emitted
 * in the primary instance.
 *
 * @n_files must be greater than zero.
 *
 * @hint is simply passed through to the ::open signal.  It is
 * intended to be used by applications that have multiple modes for
 * opening files (eg: "view" vs "edit", etc).  Unless you have a need
 * for this functionality, you should use "".
 *
 * The application must be registered before calling this function
 * and it must have the %G_APPLICATION_HANDLES_OPEN flag set.
 *
 * Since: 2.28
 **/
void
xapplication_open (xapplication_t  *application,
                    xfile_t        **files,
                    xint_t           n_files,
                    const xchar_t   *hint)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (application->priv->flags &
                    G_APPLICATION_HANDLES_OPEN);
  g_return_if_fail (application->priv->is_registered);

  if (application->priv->is_remote)
    xapplication_impl_open (application->priv->impl,
                             files, n_files, hint,
                             get_platform_data (application, NULL));

  else
    g_signal_emit (application, xapplication_signals[SIGNAL_OPEN],
                   0, files, n_files, hint);
}

/* Run {{{1 */
/**
 * xapplication_run:
 * @application: a #xapplication_t
 * @argc: the argc from main() (or 0 if @argv is %NULL)
 * @argv: (array length=argc) (element-type filename) (nullable):
 *     the argv from main(), or %NULL
 *
 * Runs the application.
 *
 * This function is intended to be run from main() and its return value
 * is intended to be returned by main(). Although you are expected to pass
 * the @argc, @argv parameters from main() to this function, it is possible
 * to pass %NULL if @argv is not available or commandline handling is not
 * required.  Note that on Windows, @argc and @argv are ignored, and
 * g_win32_get_command_line() is called internally (for proper support
 * of Unicode commandline arguments).
 *
 * #xapplication_t will attempt to parse the commandline arguments.  You
 * can add commandline flags to the list of recognised options by way of
 * xapplication_add_main_option_entries().  After this, the
 * #xapplication_t::handle-local-options signal is emitted, from which the
 * application can inspect the values of its #GOptionEntrys.
 *
 * #xapplication_t::handle-local-options is a good place to handle options
 * such as `--version`, where an immediate reply from the local process is
 * desired (instead of communicating with an already-running instance).
 * A #xapplication_t::handle-local-options handler can stop further processing
 * by returning a non-negative value, which then becomes the exit status of
 * the process.
 *
 * What happens next depends on the flags: if
 * %G_APPLICATION_HANDLES_COMMAND_LINE was specified then the remaining
 * commandline arguments are sent to the primary instance, where a
 * #xapplication_t::command-line signal is emitted.  Otherwise, the
 * remaining commandline arguments are assumed to be a list of files.
 * If there are no files listed, the application is activated via the
 * #xapplication_t::activate signal.  If there are one or more files, and
 * %G_APPLICATION_HANDLES_OPEN was specified then the files are opened
 * via the #xapplication_t::open signal.
 *
 * If you are interested in doing more complicated local handling of the
 * commandline then you should implement your own #xapplication_t subclass
 * and override local_command_line(). In this case, you most likely want
 * to return %TRUE from your local_command_line() implementation to
 * suppress the default handling. See
 * [gapplication-example-cmdline2.c][https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gapplication-example-cmdline2.c]
 * for an example.
 *
 * If, after the above is done, the use count of the application is zero
 * then the exit status is returned immediately.  If the use count is
 * non-zero then the default main context is iterated until the use count
 * falls to zero, at which point 0 is returned.
 *
 * If the %G_APPLICATION_IS_SERVICE flag is set, then the service will
 * run for as much as 10 seconds with a use count of zero while waiting
 * for the message that caused the activation to arrive.  After that,
 * if the use count falls to zero the application will exit immediately,
 * except in the case that xapplication_set_inactivity_timeout() is in
 * use.
 *
 * This function sets the prgname (g_set_prgname()), if not already set,
 * to the basename of argv[0].
 *
 * Much like xmain_loop_run(), this function will acquire the main context
 * for the duration that the application is running.
 *
 * Since 2.40, applications that are not explicitly flagged as services
 * or launchers (ie: neither %G_APPLICATION_IS_SERVICE or
 * %G_APPLICATION_IS_LAUNCHER are given as flags) will check (from the
 * default handler for local_command_line) if "--gapplication-service"
 * was given in the command line.  If this flag is present then normal
 * commandline processing is interrupted and the
 * %G_APPLICATION_IS_SERVICE flag is set.  This provides a "compromise"
 * solution whereby running an application directly from the commandline
 * will invoke it in the normal way (which can be useful for debugging)
 * while still allowing applications to be D-Bus activated in service
 * mode.  The D-Bus service file should invoke the executable with
 * "--gapplication-service" as the sole commandline argument.  This
 * approach is suitable for use by most graphical applications but
 * should not be used from applications like editors that need precise
 * control over when processes invoked via the commandline will exit and
 * what their exit status will be.
 *
 * Returns: the exit status
 *
 * Since: 2.28
 **/
int
xapplication_run (xapplication_t  *application,
                   int            argc,
                   char         **argv)
{
  xchar_t **arguments;
  int status;
  xmain_context_t *context;
  xboolean_t acquired_context;

  g_return_val_if_fail (X_IS_APPLICATION (application), 1);
  g_return_val_if_fail (argc == 0 || argv != NULL, 1);
  g_return_val_if_fail (!application->priv->must_quit_now, 1);

#ifdef G_OS_WIN32
  {
    xint_t new_argc = 0;

    arguments = g_win32_get_command_line ();

    /*
     * CommandLineToArgvW(), which is called by g_win32_get_command_line(),
     * pulls in the whole command line that is used to call the program.  This is
     * fine in cases where the program is a .exe program, but in the cases where the
     * program is a called via a script, such as PyGObject's gtk-demo.py, which is normally
     * called using 'python gtk-demo.py' on Windows, the program name (argv[0])
     * returned by g_win32_get_command_line() will not be the argv[0] that ->local_command_line()
     * would expect, causing the program to fail with "This application can not open files."
     */
    new_argc = xstrv_length (arguments);

    if (new_argc > argc)
      {
        xint_t i;

        for (i = 0; i < new_argc - argc; i++)
          g_free (arguments[i]);

        memmove (&arguments[0],
                 &arguments[new_argc - argc],
                 sizeof (arguments[0]) * (argc + 1));
      }
  }
#elif defined(__APPLE__)
  {
    xint_t i, j;

    /*
     * OSX adds an unexpected parameter on the format -psn_X_XXXXXX
     * when opening the application using Launch Services. In order
     * to avoid that GOption fails to parse this parameter we just
     * skip it if it was provided.
     * See: https://gitlab.gnome.org/GNOME/glib/issues/1784
     */
    arguments = g_new (xchar_t *, argc + 1);
    for (i = 0, j = 0; i < argc; i++)
      {
        if (!xstr_has_prefix (argv[i], "-psn_"))
          {
            arguments[j] = xstrdup (argv[i]);
            j++;
          }
      }
    arguments[j] = NULL;
  }
#else
  {
    xint_t i;

    arguments = g_new (xchar_t *, argc + 1);
    for (i = 0; i < argc; i++)
      arguments[i] = xstrdup (argv[i]);
    arguments[i] = NULL;
  }
#endif

  if (g_get_prgname () == NULL && argc > 0)
    {
      xchar_t *prgname;

      prgname = g_path_get_basename (argv[0]);
      g_set_prgname (prgname);
      g_free (prgname);
    }

  context = xmain_context_default ();
  acquired_context = xmain_context_acquire (context);
  if (!acquired_context)
    {
      g_critical ("xapplication_run() cannot acquire the default main context because it is already acquired by another thread!");
      xstrfreev (arguments);
      return 1;
    }

  if (!G_APPLICATION_GET_CLASS (application)
        ->local_command_line (application, &arguments, &status))
    {
      xerror_t *error = NULL;

      if (!xapplication_register (application, NULL, &error))
        {
          g_printerr ("Failed to register: %s\n", error->message);
          xerror_free (error);
          return 1;
        }

      xapplication_call_command_line (application, (const xchar_t **) arguments, NULL, &status);
    }

  xstrfreev (arguments);

  if (application->priv->flags & G_APPLICATION_IS_SERVICE &&
      application->priv->is_registered &&
      !application->priv->use_count &&
      !application->priv->inactivity_timeout_id)
    {
      application->priv->inactivity_timeout_id =
        g_timeout_add (10000, inactivity_timeout_expired, application);
    }

  while (application->priv->use_count || application->priv->inactivity_timeout_id)
    {
      if (application->priv->must_quit_now)
        break;

      xmain_context_iteration (context, TRUE);
      status = 0;
    }

  if (application->priv->is_registered && !application->priv->is_remote)
    {
      g_signal_emit (application, xapplication_signals[SIGNAL_SHUTDOWN], 0);

      if (!application->priv->did_shutdown)
        g_critical ("xapplication_t subclass '%s' failed to chain up on"
                    " ::shutdown (from end of override function)",
                    G_OBJECT_TYPE_NAME (application));
    }

  if (application->priv->impl)
    {
      if (application->priv->is_registered)
        {
          application->priv->is_registered = FALSE;

          xobject_notify (G_OBJECT (application), "is-registered");
        }

      xapplication_impl_flush (application->priv->impl);
      xapplication_impl_destroy (application->priv->impl);
      application->priv->impl = NULL;
    }

  g_settings_sync ();

  if (!application->priv->must_quit_now)
    while (xmain_context_iteration (context, FALSE))
      ;

  xmain_context_release (context);

  return status;
}

static xchar_t **
xapplication_list_actions (xaction_group_t *action_group)
{
  xapplication_t *application = G_APPLICATION (action_group);

  g_return_val_if_fail (application->priv->is_registered, NULL);

  if (application->priv->remote_actions != NULL)
    return xaction_group_list_actions (XACTION_GROUP (application->priv->remote_actions));

  else if (application->priv->actions != NULL)
    return xaction_group_list_actions (application->priv->actions);

  else
    /* empty string array */
    return g_new0 (xchar_t *, 1);
}

static xboolean_t
xapplication_query_action (xaction_group_t        *group,
                            const xchar_t         *action_name,
                            xboolean_t            *enabled,
                            const xvariant_type_t **parameter_type,
                            const xvariant_type_t **state_type,
                            xvariant_t           **state_hint,
                            xvariant_t           **state)
{
  xapplication_t *application = G_APPLICATION (group);

  g_return_val_if_fail (application->priv->is_registered, FALSE);

  if (application->priv->remote_actions != NULL)
    return xaction_group_query_action (XACTION_GROUP (application->priv->remote_actions),
                                        action_name,
                                        enabled,
                                        parameter_type,
                                        state_type,
                                        state_hint,
                                        state);

  if (application->priv->actions != NULL)
    return xaction_group_query_action (application->priv->actions,
                                        action_name,
                                        enabled,
                                        parameter_type,
                                        state_type,
                                        state_hint,
                                        state);

  return FALSE;
}

static void
xapplication_change_action_state (xaction_group_t *action_group,
                                   const xchar_t  *action_name,
                                   xvariant_t     *value)
{
  xapplication_t *application = G_APPLICATION (action_group);

  g_return_if_fail (application->priv->is_remote ||
                    application->priv->actions != NULL);
  g_return_if_fail (application->priv->is_registered);

  if (application->priv->remote_actions)
    xremote_action_group_change_action_state_full (application->priv->remote_actions,
                                                    action_name, value, get_platform_data (application, NULL));

  else
    xaction_group_change_action_state (application->priv->actions, action_name, value);
}

static void
xapplication_activate_action (xaction_group_t *action_group,
                               const xchar_t  *action_name,
                               xvariant_t     *parameter)
{
  xapplication_t *application = G_APPLICATION (action_group);

  g_return_if_fail (application->priv->is_remote ||
                    application->priv->actions != NULL);
  g_return_if_fail (application->priv->is_registered);

  if (application->priv->remote_actions)
    xremote_action_group_activate_action_full (application->priv->remote_actions,
                                                action_name, parameter, get_platform_data (application, NULL));

  else
    xaction_group_activate_action (application->priv->actions, action_name, parameter);
}

static xaction_t *
xapplication_lookup_action (xaction_map_t  *action_map,
                             const xchar_t *action_name)
{
  xapplication_t *application = G_APPLICATION (action_map);

  g_return_val_if_fail (X_IS_ACTION_MAP (application->priv->actions), NULL);

  return xaction_map_lookup_action (G_ACTION_MAP (application->priv->actions), action_name);
}

static void
xapplication_add_action (xaction_map_t *action_map,
                          xaction_t    *action)
{
  xapplication_t *application = G_APPLICATION (action_map);

  g_return_if_fail (X_IS_ACTION_MAP (application->priv->actions));

  xaction_map_add_action (G_ACTION_MAP (application->priv->actions), action);
}

static void
xapplication_remove_action (xaction_map_t  *action_map,
                             const xchar_t *action_name)
{
  xapplication_t *application = G_APPLICATION (action_map);

  g_return_if_fail (X_IS_ACTION_MAP (application->priv->actions));

  xaction_map_remove_action (G_ACTION_MAP (application->priv->actions), action_name);
}

static void
xapplication_action_group_iface_init (xaction_group_interface_t *iface)
{
  iface->list_actions = xapplication_list_actions;
  iface->query_action = xapplication_query_action;
  iface->change_action_state = xapplication_change_action_state;
  iface->activate_action = xapplication_activate_action;
}

static void
xapplication_action_map_iface_init (xaction_map_interface_t *iface)
{
  iface->lookup_action = xapplication_lookup_action;
  iface->add_action = xapplication_add_action;
  iface->remove_action = xapplication_remove_action;
}

/* Default Application {{{1 */

static xapplication_t *default_app;

/**
 * xapplication_get_default:
 *
 * Returns the default #xapplication_t instance for this process.
 *
 * Normally there is only one #xapplication_t per process and it becomes
 * the default when it is created.  You can exercise more control over
 * this by using xapplication_set_default().
 *
 * If there is no default application then %NULL is returned.
 *
 * Returns: (nullable) (transfer none): the default application for this process, or %NULL
 *
 * Since: 2.32
 **/
xapplication_t *
xapplication_get_default (void)
{
  return default_app;
}

/**
 * xapplication_set_default:
 * @application: (nullable): the application to set as default, or %NULL
 *
 * Sets or unsets the default application for the process, as returned
 * by xapplication_get_default().
 *
 * This function does not take its own reference on @application.  If
 * @application is destroyed then the default application will revert
 * back to %NULL.
 *
 * Since: 2.32
 **/
void
xapplication_set_default (xapplication_t *application)
{
  default_app = application;
}

/**
 * xapplication_quit:
 * @application: a #xapplication_t
 *
 * Immediately quits the application.
 *
 * Upon return to the mainloop, xapplication_run() will return,
 * calling only the 'shutdown' function before doing so.
 *
 * The hold count is ignored.
 * Take care if your code has called xapplication_hold() on the application and
 * is therefore still expecting it to exist.
 * (Note that you may have called xapplication_hold() indirectly, for example
 * through gtk_application_add_window().)
 *
 * The result of calling xapplication_run() again after it returns is
 * unspecified.
 *
 * Since: 2.32
 **/
void
xapplication_quit (xapplication_t *application)
{
  g_return_if_fail (X_IS_APPLICATION (application));

  application->priv->must_quit_now = TRUE;
}

/**
 * xapplication_mark_busy:
 * @application: a #xapplication_t
 *
 * Increases the busy count of @application.
 *
 * Use this function to indicate that the application is busy, for instance
 * while a long running operation is pending.
 *
 * The busy state will be exposed to other processes, so a session shell will
 * use that information to indicate the state to the user (e.g. with a
 * spinner).
 *
 * To cancel the busy indication, use xapplication_unmark_busy().
 *
 * The application must be registered before calling this function.
 *
 * Since: 2.38
 **/
void
xapplication_mark_busy (xapplication_t *application)
{
  xboolean_t was_busy;

  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (application->priv->is_registered);

  was_busy = (application->priv->busy_count > 0);
  application->priv->busy_count++;

  if (!was_busy)
    {
      xapplication_impl_set_busy_state (application->priv->impl, TRUE);
      xobject_notify (G_OBJECT (application), "is-busy");
    }
}

/**
 * xapplication_unmark_busy:
 * @application: a #xapplication_t
 *
 * Decreases the busy count of @application.
 *
 * When the busy count reaches zero, the new state will be propagated
 * to other processes.
 *
 * This function must only be called to cancel the effect of a previous
 * call to xapplication_mark_busy().
 *
 * Since: 2.38
 **/
void
xapplication_unmark_busy (xapplication_t *application)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (application->priv->busy_count > 0);

  application->priv->busy_count--;

  if (application->priv->busy_count == 0)
    {
      xapplication_impl_set_busy_state (application->priv->impl, FALSE);
      xobject_notify (G_OBJECT (application), "is-busy");
    }
}

/**
 * xapplication_get_is_busy:
 * @application: a #xapplication_t
 *
 * Gets the application's current busy state, as set through
 * xapplication_mark_busy() or xapplication_bind_busy_property().
 *
 * Returns: %TRUE if @application is currently marked as busy
 *
 * Since: 2.44
 */
xboolean_t
xapplication_get_is_busy (xapplication_t *application)
{
  g_return_val_if_fail (X_IS_APPLICATION (application), FALSE);

  return application->priv->busy_count > 0;
}

/* Notifications {{{1 */

/**
 * xapplication_send_notification:
 * @application: a #xapplication_t
 * @id: (nullable): id of the notification, or %NULL
 * @notification: the #xnotification_t to send
 *
 * Sends a notification on behalf of @application to the desktop shell.
 * There is no guarantee that the notification is displayed immediately,
 * or even at all.
 *
 * Notifications may persist after the application exits. It will be
 * D-Bus-activated when the notification or one of its actions is
 * activated.
 *
 * Modifying @notification after this call has no effect. However, the
 * object can be reused for a later call to this function.
 *
 * @id may be any string that uniquely identifies the event for the
 * application. It does not need to be in any special format. For
 * example, "new-message" might be appropriate for a notification about
 * new messages.
 *
 * If a previous notification was sent with the same @id, it will be
 * replaced with @notification and shown again as if it was a new
 * notification. This works even for notifications sent from a previous
 * execution of the application, as long as @id is the same string.
 *
 * @id may be %NULL, but it is impossible to replace or withdraw
 * notifications without an id.
 *
 * If @notification is no longer relevant, it can be withdrawn with
 * xapplication_withdraw_notification().
 *
 * Since: 2.40
 */
void
xapplication_send_notification (xapplication_t  *application,
                                 const xchar_t   *id,
                                 xnotification_t *notification)
{
  xchar_t *generated_id = NULL;

  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (X_IS_NOTIFICATION (notification));
  g_return_if_fail (xapplication_get_is_registered (application));
  g_return_if_fail (!xapplication_get_is_remote (application));

  if (application->priv->notifications == NULL)
    application->priv->notifications = xnotification_backend_new_default (application);

  if (id == NULL)
    {
      generated_id = g_dbus_generate_guid ();
      id = generated_id;
    }

  xnotification_backend_send_notification (application->priv->notifications, id, notification);

  g_free (generated_id);
}

/**
 * xapplication_withdraw_notification:
 * @application: a #xapplication_t
 * @id: id of a previously sent notification
 *
 * Withdraws a notification that was sent with
 * xapplication_send_notification().
 *
 * This call does nothing if a notification with @id doesn't exist or
 * the notification was never sent.
 *
 * This function works even for notifications sent in previous
 * executions of this application, as long @id is the same as it was for
 * the sent notification.
 *
 * Note that notifications are dismissed when the user clicks on one
 * of the buttons in a notification or triggers its default action, so
 * there is no need to explicitly withdraw the notification in that case.
 *
 * Since: 2.40
 */
void
xapplication_withdraw_notification (xapplication_t *application,
                                     const xchar_t  *id)
{
  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (id != NULL);

  if (application->priv->notifications == NULL)
    application->priv->notifications = xnotification_backend_new_default (application);

  xnotification_backend_withdraw_notification (application->priv->notifications, id);
}

/* Busy binding {{{1 */

typedef struct
{
  xapplication_t *app;
  xboolean_t is_busy;
} GApplicationBusyBinding;

static void
xapplication_busy_binding_destroy (xpointer_t  data,
                                    xclosure_t *closure)
{
  GApplicationBusyBinding *binding = data;

  if (binding->is_busy)
    xapplication_unmark_busy (binding->app);

  xobject_unref (binding->app);
  g_slice_free (GApplicationBusyBinding, binding);
}

static void
xapplication_notify_busy_binding (xobject_t    *object,
                                   xparam_spec_t *pspec,
                                   xpointer_t    user_data)
{
  GApplicationBusyBinding *binding = user_data;
  xboolean_t is_busy;

  xobject_get (object, pspec->name, &is_busy, NULL);

  if (is_busy && !binding->is_busy)
    xapplication_mark_busy (binding->app);
  else if (!is_busy && binding->is_busy)
    xapplication_unmark_busy (binding->app);

  binding->is_busy = is_busy;
}

/**
 * xapplication_bind_busy_property:
 * @application: a #xapplication_t
 * @object: (type xobject_t.Object): a #xobject_t
 * @property: the name of a boolean property of @object
 *
 * Marks @application as busy (see xapplication_mark_busy()) while
 * @property on @object is %TRUE.
 *
 * The binding holds a reference to @application while it is active, but
 * not to @object. Instead, the binding is destroyed when @object is
 * finalized.
 *
 * Since: 2.44
 */
void
xapplication_bind_busy_property (xapplication_t *application,
                                  xpointer_t      object,
                                  const xchar_t  *property)
{
  xuint_t notify_id;
  xquark property_quark;
  xparam_spec_t *pspec;
  GApplicationBusyBinding *binding;
  xclosure_t *closure;

  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (property != NULL);

  notify_id = g_signal_lookup ("notify", XTYPE_OBJECT);
  property_quark = g_quark_from_string (property);
  pspec = xobject_class_find_property (G_OBJECT_GET_CLASS (object), property);

  g_return_if_fail (pspec != NULL && pspec->value_type == XTYPE_BOOLEAN);

  if (g_signal_handler_find (object, G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_FUNC,
                             notify_id, property_quark, NULL, xapplication_notify_busy_binding, NULL) > 0)
    {
      g_critical ("%s: '%s' is already bound to the busy state of the application", G_STRFUNC, property);
      return;
    }

  binding = g_slice_new (GApplicationBusyBinding);
  binding->app = xobject_ref (application);
  binding->is_busy = FALSE;

  closure = g_cclosure_new (G_CALLBACK (xapplication_notify_busy_binding), binding,
                            xapplication_busy_binding_destroy);
  g_signal_connect_closure_by_id (object, notify_id, property_quark, closure, FALSE);

  /* fetch the initial value */
  xapplication_notify_busy_binding (object, pspec, binding);
}

/**
 * xapplication_unbind_busy_property:
 * @application: a #xapplication_t
 * @object: (type xobject_t.Object): a #xobject_t
 * @property: the name of a boolean property of @object
 *
 * Destroys a binding between @property and the busy state of
 * @application that was previously created with
 * xapplication_bind_busy_property().
 *
 * Since: 2.44
 */
void
xapplication_unbind_busy_property (xapplication_t *application,
                                    xpointer_t      object,
                                    const xchar_t  *property)
{
  xuint_t notify_id;
  xquark property_quark;
  gulong handler_id;

  g_return_if_fail (X_IS_APPLICATION (application));
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (property != NULL);

  notify_id = g_signal_lookup ("notify", XTYPE_OBJECT);
  property_quark = g_quark_from_string (property);

  handler_id = g_signal_handler_find (object, G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_FUNC,
                                      notify_id, property_quark, NULL, xapplication_notify_busy_binding, NULL);
  if (handler_id == 0)
    {
      g_critical ("%s: '%s' is not bound to the busy state of the application", G_STRFUNC, property);
      return;
    }

  g_signal_handler_disconnect (object, handler_id);
}

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
