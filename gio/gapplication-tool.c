/*
 * Copyright © 2013 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include <gio/gdesktopappinfo.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <string.h>
#include <locale.h>

struct help_topic
{
  const xchar_t *command;
  const xchar_t *summary;
  const xchar_t *description;
  const xchar_t *synopsis;
};

struct help_substvar
{
  const xchar_t *var;
  const xchar_t *description;
};

static const struct help_topic topics[] = {
  { "help",         N_("Print help"),
                    N_("Print help"),
                    N_("[COMMAND]")
  },
  { "version",      N_("Print version"),
                    N_("Print version information and exit"),
                    NULL
  },
  { "list-apps",    N_("List applications"),
                    N_("List the installed D-Bus activatable applications (by .desktop files)"),
                    NULL
  },
  { "launch",       N_("Launch an application"),
                    N_("Launch the application (with optional files to open)"),
                    N_("APPID [FILE…]")
  },
  { "action",       N_("Activate an action"),
                    N_("Invoke an action on the application"),
                    N_("APPID ACTION [PARAMETER]")
  },
  { "list-actions", N_("List available actions"),
                    N_("List static actions for an application (from .desktop file)"),
                    N_("APPID")
  }
};

static const struct help_substvar substvars[] = {
  { N_("COMMAND"),   N_("The command to print detailed help for")                             },
  { N_("APPID"),     N_("Application identifier in D-Bus format (eg: org.example.viewer)")    },
  { N_("FILE"),      N_("Optional relative or absolute filenames, or URIs to open")           },
  { N_("ACTION"),    N_("The action name to invoke")                                          },
  { N_("PARAMETER"), N_("Optional parameter to the action invocation, in xvariant_t format")    }
};

static int
app_help (xboolean_t     requested,
          const xchar_t *command)
{
  const struct help_topic *topic = NULL;
  xstring_t *string;

  string = xstring_new (NULL);

  if (command)
    {
      xsize_t i;

      for (i = 0; i < G_N_ELEMENTS (topics); i++)
        if (xstr_equal (topics[i].command, command))
          topic = &topics[i];

      if (!topic)
        {
          xstring_printf (string, _("Unknown command %s\n\n"), command);
          requested = FALSE;
        }
    }

  xstring_append (string, _("Usage:\n"));

  if (topic)
    {
      xuint_t maxwidth;
      xsize_t i;

      xstring_append_printf (string, "\n  %s %s %s\n\n", "gapplication",
                              topic->command, topic->synopsis ? _(topic->synopsis) : "");
      xstring_append_printf (string, "%s\n\n", _(topic->description));

      if (topic->synopsis)
        {
          xstring_append (string, _("Arguments:\n"));

          maxwidth = 0;
          for (i = 0; i < G_N_ELEMENTS (substvars); i++)
            if (strstr (topic->synopsis, substvars[i].var))
              maxwidth = MAX(maxwidth, strlen (_(substvars[i].var)));

          for (i = 0; i < G_N_ELEMENTS (substvars); i++)
            if (strstr (topic->synopsis, substvars[i].var))
              xstring_append_printf (string, "  %-*.*s   %s\n", maxwidth, maxwidth,
                                      _(substvars[i].var), _(substvars[i].description));
          xstring_append (string, "\n");
        }
    }
  else
    {
      xuint_t maxwidth;
      xsize_t i;

      xstring_append_printf (string, "\n  %s %s %s\n\n", "gapplication", _("COMMAND"), _("[ARGS…]"));
      xstring_append_printf (string, _("Commands:\n"));

      maxwidth = 0;
      for (i = 0; i < G_N_ELEMENTS (topics); i++)
        maxwidth = MAX(maxwidth, strlen (topics[i].command));

      for (i = 0; i < G_N_ELEMENTS (topics); i++)
        xstring_append_printf (string, "  %-*.*s   %s\n", maxwidth, maxwidth,
                                topics[i].command, _(topics[i].summary));

      xstring_append (string, "\n");
      /* Translators: do not translate 'help', but please translate 'COMMAND'. */
      xstring_append_printf (string, _("Use “%s help COMMAND” to get detailed help.\n\n"), "gapplication");
    }

  if (requested)
    g_print ("%s", string->str);
  else
    g_printerr ("%s\n", string->str);

  xstring_free (string, TRUE);

  return requested ? 0 : 1;
}

static xboolean_t
app_check_name (xchar_t       **args,
                const xchar_t  *command)
{
  if (args[0] == NULL)
    {
      g_printerr (_("%s command requires an application id to directly follow\n\n"), command);
      return FALSE;
    }

  if (!g_dbus_is_name (args[0]))
    {
      g_printerr (_("invalid application id: “%s”\n"), args[0]);
      return FALSE;
    }

  return TRUE;
}

static int
app_no_args (const xchar_t *command)
{
  /* Translators: %s is replaced with a command name like 'list-actions' */
  g_printerr (_("“%s” takes no arguments\n\n"), command);
  return app_help (FALSE, command);
}

static int
app_version (xchar_t **args)
{
  if (xstrv_length (args))
    return app_no_args ("version");

  g_print (PACKAGE_VERSION "\n");
  return 0;
}

static int
app_list (xchar_t **args)
{
  xlist_t *apps;

  if (xstrv_length (args))
    return app_no_args ("list");

  apps = xapp_info_get_all ();

  while (apps)
    {
      GDesktopAppInfo *info = apps->data;

      if (X_IS_DESKTOP_APP_INFO (info))
        if (g_desktop_app_info_get_boolean (info, "DBusActivatable"))
          {
            const xchar_t *filename;

            filename = xapp_info_get_id (G_APP_INFO (info));
            if (xstr_has_suffix (filename, ".desktop"))
              {
                xchar_t *id;

                id = xstrndup (filename, strlen (filename) - 8);
                g_print ("%s\n", id);
                g_free (id);
              }
          }

      apps = xlist_delete_link (apps, apps);
      xobject_unref (info);
    }

  return 0;
}

static xchar_t *
app_path_for_id (const xchar_t *app_id)
{
  xchar_t *path;
  xint_t i;

  path = xstrconcat ("/", app_id, NULL);
  for (i = 0; path[i]; i++)
    {
      if (path[i] == '.')
        path[i] = '/';
      if (path[i] == '-')
        path[i] = '_';
    }

  return path;
}

static int
app_call (const xchar_t *app_id,
          const xchar_t *method_name,
          xvariant_t    *parameters)
{
  xdbus_connection_t *session;
  xerror_t *error = NULL;
  xchar_t *object_path;
  xvariant_t *result;


  session = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (!session)
    {
      xvariant_unref (xvariant_ref_sink (parameters));
      g_printerr (_("unable to connect to D-Bus: %s\n"), error->message);
      xerror_free (error);
      return 1;
    }

  object_path = app_path_for_id (app_id);

  result = g_dbus_connection_call_sync (session, app_id, object_path, "org.freedesktop.Application",
                                        method_name, parameters, G_VARIANT_TYPE_UNIT,
                                        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  g_free (object_path);

  if (result)
    {
      xvariant_unref (result);
      return 0;
    }
  else
    {
      g_printerr (_("error sending %s message to application: %s\n"), method_name, error->message);
      xerror_free (error);
      return 1;
    }
}

static xvariant_t *
app_get_platform_data (void)
{
  xvariant_builder_t builder;
  const xchar_t *startup_id;

  xvariant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);

  if ((startup_id = g_getenv ("DESKTOP_STARTUP_ID")))
    xvariant_builder_add (&builder, "{sv}", "desktop-startup-id", xvariant_new_string (startup_id));

  return xvariant_builder_end (&builder);
}

static int
app_action (xchar_t **args)
{
  xvariant_builder_t params;
  const xchar_t *name;

  if (!app_check_name (args, "action"))
    return 1;

  if (args[1] == NULL)
    {
      g_printerr (_("action name must be given after application id\n"));
      return 1;
    }

  name = args[1];

  if (!g_action_name_is_valid (name))
    {
      g_printerr (_("invalid action name: “%s”\n"
                    "action names must consist of only alphanumerics, “-” and “.”\n"), name);
      return 1;
    }

  xvariant_builder_init (&params, G_VARIANT_TYPE ("av"));

  if (args[2])
    {
      xerror_t *error = NULL;
      xvariant_t *parameter;

      parameter = xvariant_parse (NULL, args[2], NULL, NULL, &error);

      if (!parameter)
        {
          xchar_t *context;

          context = xvariant_parse_error_print_context (error, args[2]);
          g_printerr (_("error parsing action parameter: %s\n"), context);
          xvariant_builder_clear (&params);
          xerror_free (error);
          g_free (context);
          return 1;
        }

      xvariant_builder_add (&params, "v", parameter);
      xvariant_unref (parameter);

      if (args[3])
        {
          g_printerr (_("actions accept a maximum of one parameter\n"));
          xvariant_builder_clear (&params);
          return 1;
        }
    }

  return app_call (args[0], "ActivateAction", xvariant_new ("(sav@a{sv})", name, &params, app_get_platform_data ()));
}

static int
app_activate (const xchar_t *app_id)
{
  return app_call (app_id, "Activate", xvariant_new ("(@a{sv})", app_get_platform_data ()));
}

static int
app_launch (xchar_t **args)
{
  xvariant_builder_t files;
  xint_t i;

  if (!app_check_name (args, "launch"))
    return 1;

  if (args[1] == NULL)
    return app_activate (args[0]);

  xvariant_builder_init (&files, G_VARIANT_TYPE_STRING_ARRAY);

  for (i = 1; args[i]; i++)
    {
      xfile_t *file;

      /* "This operation never fails" */
      file = xfile_new_for_commandline_arg (args[i]);
      xvariant_builder_add_value (&files, xvariant_new_take_string (xfile_get_uri (file)));
      xobject_unref (file);
    }

  return app_call (args[0], "Open", xvariant_new ("(as@a{sv})", &files, app_get_platform_data ()));
}

static int
app_list_actions (xchar_t **args)
{
  const xchar_t * const *actions;
  GDesktopAppInfo *app_info;
  xchar_t *filename;
  xint_t i;

  if (!app_check_name (args, "list-actions"))
    return 1;

  if (args[1])
    {
      g_printerr (_("list-actions command takes only the application id"));
      app_help (FALSE, "list-actions");
    }

  filename = xstrconcat (args[0], ".desktop", NULL);
  app_info = g_desktop_app_info_new (filename);
  g_free (filename);

  if (app_info == NULL)
    {
      g_printerr (_("unable to find desktop file for application %s\n"), args[0]);
      return 1;
    }

  actions = g_desktop_app_info_list_actions (app_info);

  for (i = 0; actions[i]; i++)
    g_print ("%s\n", actions[i]);

  xobject_unref (app_info);

  return 0;
}

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "");
  textdomain (GETTEXT_PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, XPL_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  if (argc < 2)
    return app_help (TRUE, NULL);

  if (xstr_equal (argv[1], "help"))
    return app_help (TRUE, argv[2]);

  if (xstr_equal (argv[1], "version"))
    return app_version (argv + 2);

  if (xstr_equal (argv[1], "list-apps"))
    return app_list (argv + 2);

  if (xstr_equal (argv[1], "launch"))
    return app_launch (argv + 2);

  if (xstr_equal (argv[1], "action"))
    return app_action (argv + 2);

  if (xstr_equal (argv[1], "list-actions"))
    return app_list_actions (argv + 2);

  g_printerr (_("unrecognised command: %s\n\n"), argv[1]);

  return app_help (FALSE, NULL);
}
