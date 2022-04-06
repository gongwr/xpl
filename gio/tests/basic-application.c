#include <gio/gio.h>
#include <string.h>

static void
new_activated (xsimple_action_t *action,
               xvariant_t      *parameter,
               xpointer_t       user_data)
{
  xapplication_t *app = user_data;

  xapplication_activate (app);
}

static void
quit_activated (xsimple_action_t *action,
                xvariant_t      *parameter,
                xpointer_t       user_data)
{
  xapplication_t *app = user_data;

  xapplication_quit (app);
}

static void
action1_activated (xsimple_action_t *action,
                   xvariant_t      *parameter,
                   xpointer_t       user_data)
{
  g_print ("activate action1\n");
}

static void
action2_activated (xsimple_action_t *action,
                   xvariant_t      *parameter,
                   xpointer_t       user_data)
{
  xvariant_t *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), xvariant_new_boolean (!xvariant_get_boolean (state)));
  g_print ("activate action2 %d\n", !xvariant_get_boolean (state));
  xvariant_unref (state);
}

static void
change_action2 (xsimple_action_t *action,
                xvariant_t      *state,
                xpointer_t       user_data)
{
  g_print ("change action2 %d\n", xvariant_get_boolean (state));
}

static void
startup (xapplication_t *app)
{
  static xaction_entry_t actions[] = {
    { "new", new_activated, NULL, NULL, NULL, { 0 } },
    { "quit", quit_activated, NULL, NULL, NULL, { 0 } },
    { "action1", action1_activated, NULL, NULL, NULL, { 0 } },
    { "action2", action2_activated, "b", "false", change_action2, { 0 } }
  };

  xaction_map_add_action_entries (G_ACTION_MAP (app),
                                   actions, G_N_ELEMENTS (actions),
                                   app);
}

static void
activate (xapplication_t *application)
{
  xapplication_hold (application);
  g_print ("activated\n");
  xapplication_release (application);
}

static void
open (xapplication_t  *application,
      xfile_t        **files,
      xint_t           n_files,
      const xchar_t   *hint)
{
  xint_t i;

  xapplication_hold (application);

  g_print ("open");
  for (i = 0; i < n_files; i++)
    {
      xchar_t *uri = xfile_get_uri (files[i]);
      g_print (" %s", uri);
      g_free (uri);
    }
  g_print ("\n");

  xapplication_release (application);
}

static int
command_line (xapplication_t            *application,
              xapplication_command_line_t *cmdline)
{
  xchar_t **argv;
  xint_t argc;
  xint_t i;

  xapplication_hold (application);
  argv = xapplication_command_line_get_arguments (cmdline, &argc);

  if (argc > 1)
    {
      if (xstrcmp0 (argv[1], "echo") == 0)
        {
          g_print ("cmdline");
          for (i = 0; i < argc; i++)
            g_print (" %s", argv[i]);
          g_print ("\n");
        }
      else if (xstrcmp0 (argv[1], "env") == 0)
        {
          const xchar_t * const *env;

          env = xapplication_command_line_get_environ (cmdline);
          g_print ("environment");
          for (i = 0; env[i]; i++)
            if (xstr_has_prefix (env[i], "TEST="))
              g_print (" %s", env[i]);
          g_print ("\n");
        }
      else if (xstrcmp0 (argv[1], "getenv") == 0)
        {
          g_print ("getenv TEST=%s\n", xapplication_command_line_getenv (cmdline, "TEST"));
        }
      else if (xstrcmp0 (argv[1], "print") == 0)
        {
          xapplication_command_line_print (cmdline, "print %s\n", argv[2]);
        }
      else if (xstrcmp0 (argv[1], "printerr") == 0)
        {
          xapplication_command_line_printerr (cmdline, "printerr %s\n", argv[2]);
        }
      else if (xstrcmp0 (argv[1], "file") == 0)
        {
          xfile_t *file;

          file = xapplication_command_line_create_file_for_arg (cmdline, argv[2]);
          g_print ("file %s\n", xfile_get_path (file));
          xobject_unref (file);
        }
      else if (xstrcmp0 (argv[1], "properties") == 0)
        {
          xboolean_t remote;
          xvariant_t *data;

          xobject_get (cmdline,
                        "is-remote", &remote,
                        NULL);

          data = xapplication_command_line_get_platform_data (cmdline);
          g_assert (remote);
          g_assert (xvariant_is_of_type (data, G_VARIANT_TYPE ("a{sv}")));
          xvariant_unref (data);
          g_print ("properties ok\n");
        }
      else if (xstrcmp0 (argv[1], "cwd") == 0)
        {
          g_print ("cwd %s\n", xapplication_command_line_get_cwd (cmdline));
        }
      else if (xstrcmp0 (argv[1], "busy") == 0)
        {
          xapplication_mark_busy (xapplication_get_default ());
          g_print ("busy\n");
        }
      else if (xstrcmp0 (argv[1], "idle") == 0)
        {
          xapplication_unmark_busy (xapplication_get_default ());
          g_print ("idle\n");
        }
      else if (xstrcmp0 (argv[1], "stdin") == 0)
        {
          xinput_stream_t *stream;

          stream = xapplication_command_line_get_stdin (cmdline);

          g_assert (stream == NULL || X_IS_INPUT_STREAM (stream));
          xobject_unref (stream);

          g_print ("stdin ok\n");
        }
      else
        g_print ("unexpected command: %s\n", argv[1]);
    }
  else
    g_print ("got ./cmd %d\n", xapplication_command_line_get_is_remote (cmdline));

  xstrfreev (argv);
  xapplication_release (application);

  return 0;
}

static xboolean_t
action_cb (xpointer_t data)
{
  xchar_t **argv = data;
  xapplication_t *app;
  xchar_t **actions;
  xint_t i;

  if (xstrcmp0 (argv[1], "./actions") == 0)
    {
      app = xapplication_get_default ();

      if (xstrcmp0 (argv[2], "list") == 0)
        {
          g_print ("actions");
          actions = xaction_group_list_actions (XACTION_GROUP (app));
          for (i = 0; actions[i]; i++)
            g_print (" %s", actions[i]);
          g_print ("\n");
          xstrfreev (actions);
        }
      else if (xstrcmp0 (argv[2], "activate") == 0)
        {
          xaction_group_activate_action (XACTION_GROUP (app),
                                          "action1", NULL);
        }
      else if (xstrcmp0 (argv[2], "set-state") == 0)
        {
          xaction_group_change_action_state (XACTION_GROUP (app),
                                              "action2",
                                              xvariant_new_boolean (TRUE));
        }
      xapplication_release (app);
    }

  return G_SOURCE_REMOVE;
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = xapplication_new ("org.gtk.TestApplication",
                           G_APPLICATION_SEND_ENVIRONMENT |
                           (xstrcmp0 (argv[1], "./cmd") == 0
                             ? G_APPLICATION_HANDLES_COMMAND_LINE
                             : G_APPLICATION_HANDLES_OPEN));
  xsignal_connect (app, "startup", G_CALLBACK (startup), NULL);
  xsignal_connect (app, "activate", G_CALLBACK (activate), NULL);
  xsignal_connect (app, "open", G_CALLBACK (open), NULL);
  xsignal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
#ifdef STANDALONE
  xapplication_set_inactivity_timeout (app, 10000);
#else
  xapplication_set_inactivity_timeout (app, 1000);
#endif

  if (xstrcmp0 (argv[1], "./actions") == 0)
    {
      xapplication_set_inactivity_timeout (app, 0);
      xapplication_hold (app);
      g_idle_add (action_cb, argv);
    }

  status = xapplication_run (app, argc - 1, argv + 1);

  xobject_unref (app);

  g_print ("exit status: %d\n", status);

  return 0;
}
