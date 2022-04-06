#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static xboolean_t
my_cmdline_handler (xpointer_t data)
{
  xapplication_command_line_t *cmdline = data;
  xchar_t **args;
  xchar_t **argv;
  xint_t argc;
  xint_t arg1;
  xboolean_t arg2;
  xboolean_t help;
  xoption_context_t *context;
  GOptionEntry entries[] = {
    { "arg1", 0, 0, G_OPTION_ARG_INT, &arg1, NULL, NULL },
    { "arg2", 0, 0, G_OPTION_ARG_NONE, &arg2, NULL, NULL },
    { "help", '?', 0, G_OPTION_ARG_NONE, &help, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };
  xerror_t *error;
  xint_t i;

  args = xapplication_command_line_get_arguments (cmdline, &argc);

  /* We have to make an extra copy of the array, since g_option_context_parse()
   * assumes that it can remove strings from the array without freeing them.
   */
  argv = g_new (xchar_t*, argc + 1);
  for (i = 0; i <= argc; i++)
    argv[i] = args[i];

  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_add_main_entries (context, entries, NULL);

  arg1 = 0;
  arg2 = FALSE;
  help = FALSE;
  error = NULL;
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      xapplication_command_line_printerr (cmdline, "%s\n", error->message);
      xerror_free (error);
      xapplication_command_line_set_exit_status (cmdline, 1);
    }
  else if (help)
    {
      xchar_t *text;
      text = g_option_context_get_help (context, FALSE, NULL);
      xapplication_command_line_print (cmdline, "%s",  text);
      g_free (text);
    }
  else
    {
      xapplication_command_line_print (cmdline, "arg1 is %d and arg2 is %s\n",
                                        arg1, arg2 ? "TRUE" : "FALSE");
      xapplication_command_line_set_exit_status (cmdline, 0);
    }

  g_free (argv);
  xstrfreev (args);

  g_option_context_free (context);

  /* we are done handling this commandline */
  xobject_unref (cmdline);

  return G_SOURCE_REMOVE;
}

static int
command_line (xapplication_t            *application,
              xapplication_command_line_t *cmdline)
{
  /* keep the application running until we are done with this commandline */
  xapplication_hold (application);

  xobject_set_data_full (G_OBJECT (cmdline),
                          "application", application,
                          (xdestroy_notify_t)xapplication_release);

  xobject_ref (cmdline);
  g_idle_add (my_cmdline_handler, cmdline);

  return 0;
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = xapplication_new ("org.gtk.TestApplication",
                           G_APPLICATION_HANDLES_COMMAND_LINE);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
  xapplication_set_inactivity_timeout (app, 10000);

  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
