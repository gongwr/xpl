#include <gio/gio.h>

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  /* This is where we'd export some objects on the bus */
}

static void
on_name_acquired (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  xpointer_t         user_data)
{
  g_print ("Acquired the name %s on the session bus\n", name);
}

static void
on_name_lost (xdbus_connection_t *connection,
              const xchar_t     *name,
              xpointer_t         user_data)
{
  g_print ("Lost the name %s on the session bus\n", name);
}

int
main (int argc, char *argv[])
{
  xuint_t owner_id;
  xmain_loop_t *loop;
  GBusNameOwnerFlags flags;
  xboolean_t opt_replace;
  xboolean_t opt_allow_replacement;
  xchar_t *opt_name;
  xoption_context_t *opt_context;
  xerror_t *error;
  GOptionEntry opt_entries[] =
    {
      { "replace", 'r', 0, G_OPTION_ARG_NONE, &opt_replace, "Replace existing name if possible", NULL },
      { "allow-replacement", 'a', 0, G_OPTION_ARG_NONE, &opt_allow_replacement, "Allow replacement", NULL },
      { "name", 'n', 0, G_OPTION_ARG_STRING, &opt_name, "Name to acquire", NULL },
      G_OPTION_ENTRY_NULL
    };

  error = NULL;
  opt_name = NULL;
  opt_replace = FALSE;
  opt_allow_replacement = FALSE;
  opt_context = g_option_context_new ("g_bus_own_name() example");
  g_option_context_add_main_entries (opt_context, opt_entries, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &error))
    {
      g_printerr ("Error parsing options: %s", error->message);
      return 1;
    }
  if (opt_name == NULL)
    {
      g_printerr ("Incorrect usage, try --help.\n");
      return 1;
    }

  flags = G_BUS_NAME_OWNER_FLAGS_NONE;
  if (opt_replace)
    flags |= G_BUS_NAME_OWNER_FLAGS_REPLACE;
  if (opt_allow_replacement)
    flags |= G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             opt_name,
                             flags,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);

  g_bus_unown_name (owner_id);

  return 0;
}
