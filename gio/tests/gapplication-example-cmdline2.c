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

  for (i = 0; i < argc; i++)
    g_print ("handling argument %s remotely\n", argv[i]);

  xstrfreev (argv);

  return 0;
}

static xboolean_t
test_local_cmdline (xapplication_t   *application,
                    xchar_t        ***arguments,
                    xint_t           *exit_status)
{
  xint_t i, j;
  xchar_t **argv;

  argv = *arguments;

  i = 1;
  while (argv[i])
    {
      if (xstr_has_prefix (argv[i], "--local-"))
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

typedef xapplication_t test_application_t;
typedef xapplication_class_t test_application_class_t;

static xtype_t test_application_get_type (void);
XDEFINE_TYPE (test_application_t, test_application, XTYPE_APPLICATION)

static void
test_application_finalize (xobject_t *object)
{
  XOBJECT_CLASS (test_application_parent_class)->finalize (object);
}

static void
test_application_init (test_application_t *app)
{
}

static void
test_application_class_init (test_application_class_t *class)
{
  XOBJECT_CLASS (class)->finalize = test_application_finalize;
  G_APPLICATION_CLASS (class)->local_command_line = test_local_cmdline;
}

static xapplication_t *
test_application_new (const xchar_t       *application_id,
                      GApplicationFlags  flags)
{
  xreturn_val_if_fail (xapplication_id_is_valid (application_id), NULL);

  return xobject_new (test_application_get_type (),
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = test_application_new ("org.gtk.test_application_t", 0);
  xapplication_set_inactivity_timeout (app, 10000);
  xsignal_connect (app, "command-line", G_CALLBACK (command_line), NULL);

  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
