#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static void
activate (xapplication_t *application)
{
  xapplication_hold (application);
  g_print ("activated\n");
  xapplication_release (application);
}

static void
activate_action (xaction_t  *action,
                 xvariant_t *parameter,
                 xpointer_t  data)
{
  xapplication_t *application = G_APPLICATION (data);

  xapplication_hold (application);
  g_print ("action %s activated\n", g_action_get_name (action));
  xapplication_release (application);
}

static void
activate_toggle_action (xsimple_action_t *action,
                        xvariant_t      *parameter,
                        xpointer_t       data)
{
  xapplication_t *application = G_APPLICATION (data);
  xvariant_t *state;
  xboolean_t b;

  g_print ("action %s activated\n", g_action_get_name (G_ACTION (action)));

  xapplication_hold (application);
  state = g_action_get_state (G_ACTION (action));
  b = xvariant_get_boolean (state);
  xvariant_unref (state);
  g_simple_action_set_state (action, xvariant_new_boolean (!b));
  g_print ("state change %d -> %d\n", b, !b);
  xapplication_release (application);
}

static void
add_actions (xapplication_t *app)
{
  xsimple_action_t *action;

  action = g_simple_action_new ("simple-action", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (activate_action), app);
  xaction_map_add_action (G_ACTION_MAP (app), G_ACTION (action));
  xobject_unref (action);

  action = g_simple_action_new_stateful ("toggle-action", NULL,
                                         xvariant_new_boolean (FALSE));
  g_signal_connect (action, "activate", G_CALLBACK (activate_toggle_action), app);
  xaction_map_add_action (G_ACTION_MAP (app), G_ACTION (action));
  xobject_unref (action);
}

static void
describe_and_activate_action (xaction_group_t *group,
                              const xchar_t  *name)
{
  const xvariant_type_t *param_type;
  xvariant_t *state;
  xboolean_t enabled;
  xchar_t *tmp;

  param_type = xaction_group_get_action_parameter_type (group, name);
  state = xaction_group_get_action_state (group, name);
  enabled = xaction_group_get_action_enabled (group, name);

  g_print ("action name:      %s\n", name);
  tmp = param_type ? xvariant_type_dup_string (param_type) : NULL;
  g_print ("parameter type:   %s\n", tmp ? tmp : "<none>");
  g_free (tmp);
  g_print ("state type:       %s\n",
           state ? xvariant_get_type_string (state) : "<none>");
  tmp = state ? xvariant_print (state, FALSE) : NULL;
  g_print ("state:            %s\n", tmp ? tmp : "<none>");
  g_free (tmp);
  g_print ("enabled:          %s\n", enabled ? "true" : "false");

  if (state != NULL)
    xvariant_unref (state);

  xaction_group_activate_action (group, name, NULL);
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = xapplication_new ("org.gtk.TestApplication", 0);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  xapplication_set_inactivity_timeout (app, 10000);

  add_actions (app);

  if (argc > 1 && strcmp (argv[1], "--simple-action") == 0)
    {
      xapplication_register (app, NULL, NULL);
      describe_and_activate_action (XACTION_GROUP (app), "simple-action");
      exit (0);
    }
  else if (argc > 1 && strcmp (argv[1], "--toggle-action") == 0)
    {
      xapplication_register (app, NULL, NULL);
      describe_and_activate_action (XACTION_GROUP (app), "toggle-action");
      exit (0);
    }

  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
