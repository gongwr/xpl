#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static int
command_line (xapplication_t            *application,
              xapplication_command_line_t *cmdline)
{
  xchar_t **argv;
  xint_t argc;
  xint_t i;

  argv = xapplication_command_line_get_arguments (cmdline, &argc);

  xapplication_command_line_print (cmdline,
                                    "This text is written back\n"
                                    "to stdout of the caller\n");

  for (i = 0; i < argc; i++)
    g_print ("argument %d: %s\n", i, argv[i]);

  xstrfreev (argv);

  return 0;
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = xapplication_new ("org.gtk.TestApplication",
                           G_APPLICATION_HANDLES_COMMAND_LINE);
  xsignal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
  xapplication_set_inactivity_timeout (app, 10000);

  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
