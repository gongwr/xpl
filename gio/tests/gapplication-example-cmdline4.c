#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>


static xint_t
handle_local_options (xapplication_t      *application,
                      xvariant_dict_t      *options,
                      xpointer_t           user_data)
{
  xuint32_t count;

  /* Deal (locally) with version option */
  if (xvariant_dict_lookup (options, "version", "b", &count))
    {
      g_print ("This is example-cmdline4, version 1.2.3\n");
      return EXIT_SUCCESS;
    }

  return -1;

}

static xint_t
command_line (xapplication_t                *application,
              xapplication_command_line_t     *cmdline,
              xpointer_t                     user_data)
{
  xuint32_t count;

  xvariant_dict_t *options = xapplication_command_line_get_options_dict (cmdline);

  /* Deal with arg option */
  if (xvariant_dict_lookup (options, "flag", "b", &count))
    {
      xapplication_command_line_print (cmdline, "flag is set\n");
    }

  return EXIT_SUCCESS;
}


int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  GOptionEntry entries[] = {
    /* A version flag option, to be handled locally */
    { "version", 'v', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL, "Show the application version", NULL },

    /* A dummy flag option, to be handled in primary */
    { "flag", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, NULL, "A flag argument", NULL },

    G_OPTION_ENTRY_NULL
  };

  app = xapplication_new ("org.gtk.TestApplication",
                           G_APPLICATION_HANDLES_COMMAND_LINE);

  xapplication_add_main_option_entries (app, entries);

  xapplication_set_option_context_parameter_string (app, "- a simple command line example");
  xapplication_set_option_context_summary (app,
                                            "Summary:\n"
                                            "This is a simple command line --help example.");
  xapplication_set_option_context_description (app,
                                                "Description:\n"
                                                "This example illustrates the use of "
                                                "xapplication command line --help functionalities "
                                                "(parameter string, summary, description). "
                                                "It does nothing at all except displaying information "
                                                "when invoked with --help argument...\n");

  xsignal_connect (app, "handle-local-options", G_CALLBACK (handle_local_options), NULL);
  xsignal_connect (app, "command-line", G_CALLBACK (command_line), NULL);

  /* This application does absolutely nothing, except if a command line is given */
  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
