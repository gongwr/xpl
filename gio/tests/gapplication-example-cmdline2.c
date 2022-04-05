#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static int
command_line (GApplication            *application,
              GApplicationCommandLine *cmdline)
{
  xchar_t **argv;
  xint_t argc;
  xint_t i;

  argv = g_application_command_line_get_arguments (cmdline, &argc);

  for (i = 0; i < argc; i++)
    g_print ("handling argument %s remotely\n", argv[i]);

  g_strfreev (argv);

  return 0;
}

static xboolean_t
test_local_cmdline (GApplication   *application,
                    xchar_t        ***arguments,
                    xint_t           *exit_status)
{
  xint_t i, j;
  xchar_t **argv;

  argv = *arguments;

  i = 1;
  while (argv[i])
    {
      if (g_str_has_prefix (argv[i], "--local-"))
        {
          g_print ("handling argument %s locally\n", argv[i]);
          g_free (argv[i]);
          for (j = i; argv[j]; j++)
            argv[j] = argv[j + 1];
        }
      else
        {
          g_print ("not handling argument %s locally\n", argv[i]);
          i++;
        }
    }

  *exit_status = 0;

  return FALSE;
}

typedef GApplication TestApplication;
typedef GApplicationClass TestApplicationClass;

static xtype_t test_application_get_type (void);
G_DEFINE_TYPE (TestApplication, test_application, XTYPE_APPLICATION)

static void
test_application_finalize (xobject_t *object)
{
  G_OBJECT_CLASS (test_application_parent_class)->finalize (object);
}

static void
test_application_init (TestApplication *app)
{
}

static void
test_application_class_init (TestApplicationClass *class)
{
  G_OBJECT_CLASS (class)->finalize = test_application_finalize;
  G_APPLICATION_CLASS (class)->local_command_line = test_local_cmdline;
}

static GApplication *
test_application_new (const xchar_t       *application_id,
                      GApplicationFlags  flags)
{
  g_return_val_if_fail (g_application_id_is_valid (application_id), NULL);

  return g_object_new (test_application_get_type (),
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

int
main (int argc, char **argv)
{
  GApplication *app;
  int status;

  app = test_application_new ("org.gtk.TestApplication", 0);
  g_application_set_inactivity_timeout (app, 10000);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);

  status = g_application_run (app, argc, argv);

  g_object_unref (app);

  return status;
}
