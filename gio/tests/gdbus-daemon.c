#include "config.h"

#include "gdbusdaemon.h"
#include <glib/gi18n.h>

int
main (int argc, char *argv[])
{
  xdbus_daemon_t *daemon;
  xmain_loop_t *loop;
  const char *address = NULL;
  const char *config_file = NULL;
  xerror_t *error = NULL;
  xboolean_t print_address = FALSE;
  xboolean_t print_env = FALSE;
  xoption_context_t *context;
  GOptionEntry entries[] = {
    { "address", 0, 0, G_OPTION_ARG_STRING, &address, N_("Address to listen on"), NULL },
    { "config-file", 0, 0, G_OPTION_ARG_STRING, &config_file, N_("Ignored, for compat with GTestDbus"), NULL },
    { "print-address", 0, 0, G_OPTION_ARG_NONE, &print_address, N_("Print address"), NULL },
    { "print-env", 0, 0, G_OPTION_ARG_NONE, &print_env, N_("Print address in shell mode"), NULL },
    G_OPTION_ENTRY_NULL
  };

  context = g_option_context_new ("");
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_set_summary (context,
    N_("Run a dbus service"));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  error = NULL;
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      return 1;
    }

  g_option_context_free (context);

  if (argc != 1)
    {
      g_printerr (_("Wrong args\n"));
      return 1;
    }


  loop = xmain_loop_new (NULL, FALSE);

  if (argc >= 2)
    address = argv[1];

  daemon = _xdbus_daemon_new (address, NULL, &error);
  if (daemon == NULL)
    {
      g_printerr ("Can't init bus: %s\n", error->message);
      return 1;
    }

  if (print_env)
    g_print ("export DBUS_SESSION_BUS_ADDRESS=\"%s\"\n", _xdbus_daemon_get_address (daemon));

  if (print_address)
    g_print ("%s\n", _xdbus_daemon_get_address (daemon));

  xmain_loop_run (loop);

  xmain_loop_unref (loop);

  return 0;
}
