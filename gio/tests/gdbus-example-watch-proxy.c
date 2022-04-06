#include <gio/gio.h>

static xchar_t *opt_name         = NULL;
static xchar_t *opt_object_path  = NULL;
static xchar_t *opt_interface    = NULL;
static xboolean_t opt_system_bus = FALSE;
static xboolean_t opt_no_auto_start = FALSE;
static xboolean_t opt_no_properties = FALSE;

static GOptionEntry opt_entries[] =
{
  { "name", 'n', 0, G_OPTION_ARG_STRING, &opt_name, "Name of the remote object to watch", NULL },
  { "object-path", 'o', 0, G_OPTION_ARG_STRING, &opt_object_path, "Object path of the remote object", NULL },
  { "interface", 'i', 0, G_OPTION_ARG_STRING, &opt_interface, "D-Bus interface of remote object", NULL },
  { "system-bus", 's', 0, G_OPTION_ARG_NONE, &opt_system_bus, "Use the system-bus instead of the session-bus", NULL },
  { "no-auto-start", 'a', 0, G_OPTION_ARG_NONE, &opt_no_auto_start, "Don't instruct the bus to launch an owner for the name", NULL},
  { "no-properties", 'p', 0, G_OPTION_ARG_NONE, &opt_no_properties, "Do not load properties", NULL},
  G_OPTION_ENTRY_NULL
};

static xmain_loop_t *loop = NULL;

static void
print_properties (xdbus_proxy_t *proxy)
{
  xchar_t **property_names;
  xuint_t n;

  g_print ("    properties:\n");

  property_names = g_dbus_proxy_get_cached_property_names (proxy);
  for (n = 0; property_names != NULL && property_names[n] != NULL; n++)
    {
      const xchar_t *key = property_names[n];
      xvariant_t *value;
      xchar_t *value_str;
      value = g_dbus_proxy_get_cached_property (proxy, key);
      value_str = xvariant_print (value, TRUE);
      g_print ("      %s -> %s\n", key, value_str);
      xvariant_unref (value);
      g_free (value_str);
    }
  xstrfreev (property_names);
}

static void
on_properties_changed (xdbus_proxy_t          *proxy,
                       xvariant_t            *changed_properties,
                       const xchar_t* const  *invalidated_properties,
                       xpointer_t             user_data)
{
  /* Note that we are guaranteed that changed_properties and
   * invalidated_properties are never NULL
   */

  if (xvariant_n_children (changed_properties) > 0)
    {
      xvariant_iter_t *iter;
      const xchar_t *key;
      xvariant_t *value;

      g_print (" *** Properties Changed:\n");
      xvariant_get (changed_properties,
                     "a{sv}",
                     &iter);
      while (xvariant_iter_loop (iter, "{&sv}", &key, &value))
        {
          xchar_t *value_str;
          value_str = xvariant_print (value, TRUE);
          g_print ("      %s -> %s\n", key, value_str);
          g_free (value_str);
        }
      xvariant_iter_free (iter);
    }

  if (xstrv_length ((xstrv_t) invalidated_properties) > 0)
    {
      xuint_t n;
      g_print (" *** Properties Invalidated:\n");
      for (n = 0; invalidated_properties[n] != NULL; n++)
        {
          const xchar_t *key = invalidated_properties[n];
          g_print ("      %s\n", key);
        }
    }
}

static void
on_signal (xdbus_proxy_t *proxy,
           xchar_t      *sender_name,
           xchar_t      *signal_name,
           xvariant_t   *parameters,
           xpointer_t    user_data)
{
  xchar_t *parameters_str;

  parameters_str = xvariant_print (parameters, TRUE);
  g_print (" *** Received Signal: %s: %s\n",
           signal_name,
           parameters_str);
  g_free (parameters_str);
}

static void
print_proxy (xdbus_proxy_t *proxy)
{
  xchar_t *name_owner;

  name_owner = g_dbus_proxy_get_name_owner (proxy);
  if (name_owner != NULL)
    {
      g_print ("+++ Proxy object points to remote object owned by %s\n"
               "    bus:          %s\n"
               "    name:         %s\n"
               "    object path:  %s\n"
               "    interface:    %s\n",
               name_owner,
               opt_system_bus ? "System Bus" : "Session Bus",
               opt_name,
               opt_object_path,
               opt_interface);
      print_properties (proxy);
    }
  else
    {
      g_print ("--- Proxy object is inert - there is no name owner for the name\n"
               "    bus:          %s\n"
               "    name:         %s\n"
               "    object path:  %s\n"
               "    interface:    %s\n",
               opt_system_bus ? "System Bus" : "Session Bus",
               opt_name,
               opt_object_path,
               opt_interface);
    }
  g_free (name_owner);
}

static void
on_name_owner_notify (xobject_t    *object,
                      xparam_spec_t *pspec,
                      xpointer_t    user_data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (object);
  print_proxy (proxy);
}

int
main (int argc, char *argv[])
{
  xoption_context_t *opt_context;
  xerror_t *error;
  GDBusProxyFlags flags;
  xdbus_proxy_t *proxy;

  loop = NULL;
  proxy = NULL;

  opt_context = g_option_context_new ("g_bus_watch_proxy() example");
  g_option_context_set_summary (opt_context,
                                "Example: to watch the object of gdbus-example-server, use:\n"
                                "\n"
                                "  ./gdbus-example-watch-proxy -n org.gtk.GDBus.TestServer  \\\n"
                                "                              -o /org/gtk/GDBus/test_object_t \\\n"
                                "                              -i org.gtk.GDBus.test_interface_t");
  g_option_context_add_main_entries (opt_context, opt_entries, NULL);
  error = NULL;
  if (!g_option_context_parse (opt_context, &argc, &argv, &error))
    {
      g_printerr ("Error parsing options: %s\n", error->message);
      goto out;
    }
  if (opt_name == NULL || opt_object_path == NULL || opt_interface == NULL)
    {
      g_printerr ("Incorrect usage, try --help.\n");
      goto out;
    }

  flags = G_DBUS_PROXY_FLAGS_NONE;
  if (opt_no_properties)
    flags |= G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
  if (opt_no_auto_start)
    flags |= G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;

  loop = xmain_loop_new (NULL, FALSE);

  error = NULL;
  proxy = g_dbus_proxy_new_for_bus_sync (opt_system_bus ? G_BUS_TYPE_SYSTEM : G_BUS_TYPE_SESSION,
                                         flags,
                                         NULL, /* xdbus_interface_info_t */
                                         opt_name,
                                         opt_object_path,
                                         opt_interface,
                                         NULL, /* xcancellable_t */
                                         &error);
  if (proxy == NULL)
    {
      g_printerr ("Error creating proxy: %s\n", error->message);
      xerror_free (error);
      goto out;
    }

  g_signal_connect (proxy,
                    "g-properties-changed",
                    G_CALLBACK (on_properties_changed),
                    NULL);
  g_signal_connect (proxy,
                    "g-signal",
                    G_CALLBACK (on_signal),
                    NULL);
  g_signal_connect (proxy,
                    "notify::g-name-owner",
                    G_CALLBACK (on_name_owner_notify),
                    NULL);
  print_proxy (proxy);

  xmain_loop_run (loop);

 out:
  if (proxy != NULL)
    xobject_unref (proxy);
  if (loop != NULL)
    xmain_loop_unref (loop);
  g_option_context_free (opt_context);
  g_free (opt_name);
  g_free (opt_object_path);
  g_free (opt_interface);

  return 0;
}
