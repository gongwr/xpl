
#include "gdbus-object-manager-example/objectmanager-gen.h"

/* ---------------------------------------------------------------------------------------------------- */

static void
print_objects (xdbus_object_manager_t *manager)
{
  xlist_t *objects;
  xlist_t *l;

  g_print ("Object manager at %s\n", g_dbus_object_manager_get_object_path (manager));
  objects = g_dbus_object_manager_get_objects (manager);
  for (l = objects; l != NULL; l = l->next)
    {
      ExampleObject *object = EXAMPLE_OBJECT (l->data);
      xlist_t *interfaces;
      xlist_t *ll;
      g_print (" - Object at %s\n", g_dbus_object_get_object_path (G_DBUS_OBJECT (object)));

      interfaces = g_dbus_object_get_interfaces (G_DBUS_OBJECT (object));
      for (ll = interfaces; ll != NULL; ll = ll->next)
        {
          xdbus_interface_t *interface = G_DBUS_INTERFACE (ll->data);
          g_print ("   - Interface %s\n", g_dbus_interface_get_info (interface)->name);

          /* Note that @interface is really a xdbus_proxy_t instance - and additionally also
           * an ExampleAnimal or ExampleCat instance - either of these can be used to
           * invoke methods on the remote object. For example, the generated function
           *
           *  void example_animal_call_poke_sync (ExampleAnimal  *proxy,
           *                                      xboolean_t        make_sad,
           *                                      xboolean_t        make_happy,
           *                                      xcancellable_t   *cancellable,
           *                                      xerror_t        **error);
           *
           * can be used to call the Poke() D-Bus method on the .Animal interface.
           * Additionally, the generated function
           *
           *  const xchar_t *example_animal_get_mood (ExampleAnimal *object);
           *
           * can be used to get the value of the :Mood property.
           */
        }
      xlist_free_full (interfaces, xobject_unref);
    }
  xlist_free_full (objects, xobject_unref);
}

static void
on_object_added (xdbus_object_manager_t *manager,
                 xdbus_object_t        *object,
                 xpointer_t            user_data)
{
  xchar_t *owner;
  owner = xdbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("Added object at %s (owner %s)\n", g_dbus_object_get_object_path (object), owner);
  g_free (owner);
}

static void
on_object_removed (xdbus_object_manager_t *manager,
                   xdbus_object_t        *object,
                   xpointer_t            user_data)
{
  xchar_t *owner;
  owner = xdbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("Removed object at %s (owner %s)\n", g_dbus_object_get_object_path (object), owner);
  g_free (owner);
}

static void
on_notify_name_owner (xobject_t    *object,
                      xparam_spec_t *pspec,
                      xpointer_t    user_data)
{
  xdbus_object_manager_client_t *manager = G_DBUS_OBJECT_MANAGER_CLIENT (object);
  xchar_t *name_owner;

  name_owner = xdbus_object_manager_client_get_name_owner (manager);
  g_print ("name-owner: %s\n", name_owner);
  g_free (name_owner);
}

static void
on_interface_proxy_properties_changed (xdbus_object_manager_client_t *manager,
                                       xdbus_object_proxy_t         *object_proxy,
                                       xdbus_proxy_t               *interface_proxy,
                                       xvariant_t                 *changed_properties,
                                       const xchar_t *const       *invalidated_properties,
                                       xpointer_t                  user_data)
{
  xvariant_iter_t iter;
  const xchar_t *key;
  xvariant_t *value;
  xchar_t *s;

  g_print ("Properties Changed on %s:\n", g_dbus_object_get_object_path (G_DBUS_OBJECT (object_proxy)));
  xvariant_iter_init (&iter, changed_properties);
  while (xvariant_iter_next (&iter, "{&sv}", &key, &value))
    {
      s = xvariant_print (value, TRUE);
      g_print ("  %s -> %s\n", key, s);
      xvariant_unref (value);
      g_free (s);
    }
}

xint_t
main (xint_t argc, xchar_t *argv[])
{
  xdbus_object_manager_t *manager;
  xmain_loop_t *loop;
  xerror_t *error;
  xchar_t *name_owner;

  manager = NULL;
  loop = NULL;

  loop = xmain_loop_new (NULL, FALSE);

  error = NULL;
  manager = example_object_manager_client_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                            "org.gtk.GDBus.Examples.ObjectManager",
                                                            "/example/Animals",
                                                            NULL, /* xcancellable_t */
                                                            &error);
  if (manager == NULL)
    {
      g_printerr ("Error getting object manager client: %s", error->message);
      xerror_free (error);
      goto out;
    }

  name_owner = xdbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("name-owner: %s\n", name_owner);
  g_free (name_owner);

  print_objects (manager);

  xsignal_connect (manager,
                    "notify::name-owner",
                    G_CALLBACK (on_notify_name_owner),
                    NULL);
  xsignal_connect (manager,
                    "object-added",
                    G_CALLBACK (on_object_added),
                    NULL);
  xsignal_connect (manager,
                    "object-removed",
                    G_CALLBACK (on_object_removed),
                    NULL);
  xsignal_connect (manager,
                    "interface-proxy-properties-changed",
                    G_CALLBACK (on_interface_proxy_properties_changed),
                    NULL);

  xmain_loop_run (loop);

 out:
  if (manager != NULL)
    xobject_unref (manager);
  if (loop != NULL)
    xmain_loop_unref (loop);

  return 0;
}
