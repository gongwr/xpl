#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_node_info_t *introspection_data = NULL;
static xdbus_interface_info_t *manager_interface_info = NULL;
static xdbus_interface_info_t *block_interface_info = NULL;
static xdbus_interface_info_t *partition_interface_info = NULL;

/* Introspection data for the service we are exporting */
static const xchar_t introspection_xml[] =
  "<node>"
  "  <interface name='org.gtk.GDBus.Example.Manager'>"
  "    <method name='Hello'>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "  </interface>"
  "  <interface name='org.gtk.GDBus.Example.Block'>"
  "    <method name='Hello'>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <property type='i' name='Major' access='read'/>"
  "    <property type='i' name='Minor' access='read'/>"
  "    <property type='s' name='Notes' access='readwrite'/>"
  "  </interface>"
  "  <interface name='org.gtk.GDBus.Example.Partition'>"
  "    <method name='Hello'>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <property type='i' name='PartitionNumber' access='read'/>"
  "    <property type='s' name='Notes' access='readwrite'/>"
  "  </interface>"
  "</node>";

/* ---------------------------------------------------------------------------------------------------- */

static void
manager_method_call (xdbus_connection_t       *connection,
                     const xchar_t           *sender,
                     const xchar_t           *object_path,
                     const xchar_t           *interface_name,
                     const xchar_t           *method_name,
                     xvariant_t              *parameters,
                     xdbus_method_invocation_t *invocation,
                     xpointer_t               user_data)
{
  const xchar_t *greeting;
  xchar_t *response;

  g_assert_cmpstr (interface_name, ==, "org.gtk.GDBus.Example.Manager");
  g_assert_cmpstr (method_name, ==, "Hello");

  xvariant_get (parameters, "(&s)", &greeting);

  response = xstrdup_printf ("Method %s.%s with user_data '%s' on object path %s called with arg '%s'",
                              interface_name,
                              method_name,
                              (const xchar_t *) user_data,
                              object_path,
                              greeting);
  xdbus_method_invocation_return_value (invocation,
                                         xvariant_new ("(s)", response));
  g_free (response);
}

const xdbus_interface_vtable_t manager_vtable =
{
  manager_method_call,
  NULL,                 /* get_property */
  NULL,                 /* set_property */
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static void
block_method_call (xdbus_connection_t       *connection,
                   const xchar_t           *sender,
                   const xchar_t           *object_path,
                   const xchar_t           *interface_name,
                   const xchar_t           *method_name,
                   xvariant_t              *parameters,
                   xdbus_method_invocation_t *invocation,
                   xpointer_t               user_data)
{
  g_assert_cmpstr (interface_name, ==, "org.gtk.GDBus.Example.Block");

  if (xstrcmp0 (method_name, "Hello") == 0)
    {
      const xchar_t *greeting;
      xchar_t *response;

      xvariant_get (parameters, "(&s)", &greeting);

      response = xstrdup_printf ("Method %s.%s with user_data '%s' on object path %s called with arg '%s'",
                                  interface_name,
                                  method_name,
                                  (const xchar_t *) user_data,
                                  object_path,
                                  greeting);
      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new ("(s)", response));
      g_free (response);
    }
  else if (xstrcmp0 (method_name, "DoStuff") == 0)
    {
      xdbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.TestSubtree.Error.Failed",
                                                  "This method intentionally always fails");
    }
  else
    {
      g_assert_not_reached ();
    }
}

static xvariant_t *
block_get_property (xdbus_connection_t  *connection,
                    const xchar_t      *sender,
                    const xchar_t      *object_path,
                    const xchar_t      *interface_name,
                    const xchar_t      *property_name,
                    xerror_t          **error,
                    xpointer_t          user_data)
{
  xvariant_t *ret;
  const xchar_t *node;
  xint_t major;
  xint_t minor;

  node = strrchr (object_path, '/') + 1;
  if (xstr_has_prefix (node, "sda"))
    major = 8;
  else
    major = 9;
  if (strlen (node) == 4)
    minor = node[3] - '0';
  else
    minor = 0;

  ret = NULL;
  if (xstrcmp0 (property_name, "Major") == 0)
    {
      ret = xvariant_new_int32 (major);
    }
  else if (xstrcmp0 (property_name, "Minor") == 0)
    {
      ret = xvariant_new_int32 (minor);
    }
  else if (xstrcmp0 (property_name, "Notes") == 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Hello %s. I thought I said reading this property "
                   "always results in an error. kthxbye",
                   sender);
    }
  else
    {
      g_assert_not_reached ();
    }

  return ret;
}

static xboolean_t
block_set_property (xdbus_connection_t  *connection,
                    const xchar_t      *sender,
                    const xchar_t      *object_path,
                    const xchar_t      *interface_name,
                    const xchar_t      *property_name,
                    xvariant_t         *value,
                    xerror_t          **error,
                    xpointer_t          user_data)
{
  /* TODO */
  g_assert_not_reached ();
}

const xdbus_interface_vtable_t block_vtable =
{
  block_method_call,
  block_get_property,
  block_set_property,
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static void
partition_method_call (xdbus_connection_t       *connection,
                       const xchar_t           *sender,
                       const xchar_t           *object_path,
                       const xchar_t           *interface_name,
                       const xchar_t           *method_name,
                       xvariant_t              *parameters,
                       xdbus_method_invocation_t *invocation,
                       xpointer_t               user_data)
{
  const xchar_t *greeting;
  xchar_t *response;

  g_assert_cmpstr (interface_name, ==, "org.gtk.GDBus.Example.Partition");
  g_assert_cmpstr (method_name, ==, "Hello");

  xvariant_get (parameters, "(&s)", &greeting);

  response = xstrdup_printf ("Method %s.%s with user_data '%s' on object path %s called with arg '%s'",
                              interface_name,
                              method_name,
                              (const xchar_t *) user_data,
                              object_path,
                              greeting);
  xdbus_method_invocation_return_value (invocation,
                                         xvariant_new ("(s)", response));
  g_free (response);
}

const xdbus_interface_vtable_t partition_vtable =
{
  partition_method_call,
  NULL,
  NULL,
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t **
subtree_enumerate (xdbus_connection_t       *connection,
                   const xchar_t           *sender,
                   const xchar_t           *object_path,
                   xpointer_t               user_data)
{
  xchar_t **nodes;
  xptr_array_t *p;

  p = xptr_array_new ();
  xptr_array_add (p, xstrdup ("sda"));
  xptr_array_add (p, xstrdup ("sda1"));
  xptr_array_add (p, xstrdup ("sda2"));
  xptr_array_add (p, xstrdup ("sda3"));
  xptr_array_add (p, xstrdup ("sdb"));
  xptr_array_add (p, xstrdup ("sdb1"));
  xptr_array_add (p, xstrdup ("sdc"));
  xptr_array_add (p, xstrdup ("sdc1"));
  xptr_array_add (p, NULL);
  nodes = (xchar_t **) xptr_array_free (p, FALSE);

  return nodes;
}

static xdbus_interface_info_t **
subtree_introspect (xdbus_connection_t       *connection,
                    const xchar_t           *sender,
                    const xchar_t           *object_path,
                    const xchar_t           *node,
                    xpointer_t               user_data)
{
  xptr_array_t *p;

  p = xptr_array_new ();
  if (node == NULL)
    {
      xptr_array_add (p, g_dbus_interface_info_ref (manager_interface_info));
    }
  else
    {
      xptr_array_add (p, g_dbus_interface_info_ref (block_interface_info));
      if (strlen (node) == 4)
        xptr_array_add (p,
                         g_dbus_interface_info_ref (partition_interface_info));
    }

  xptr_array_add (p, NULL);

  return (xdbus_interface_info_t **) xptr_array_free (p, FALSE);
}

static const xdbus_interface_vtable_t *
subtree_dispatch (xdbus_connection_t             *connection,
                  const xchar_t                 *sender,
                  const xchar_t                 *object_path,
                  const xchar_t                 *interface_name,
                  const xchar_t                 *node,
                  xpointer_t                    *out_user_data,
                  xpointer_t                     user_data)
{
  const xdbus_interface_vtable_t *vtable_to_return;
  xpointer_t user_data_to_return;

  if (xstrcmp0 (interface_name, "org.gtk.GDBus.Example.Manager") == 0)
    {
      user_data_to_return = "The Root";
      vtable_to_return = &manager_vtable;
    }
  else
    {
      if (strlen (node) == 4)
        user_data_to_return = "A partition";
      else
        user_data_to_return = "A block device";

      if (xstrcmp0 (interface_name, "org.gtk.GDBus.Example.Block") == 0)
        vtable_to_return = &block_vtable;
      else if (xstrcmp0 (interface_name, "org.gtk.GDBus.Example.Partition") == 0)
        vtable_to_return = &partition_vtable;
      else
        g_assert_not_reached ();
    }

  *out_user_data = user_data_to_return;

  return vtable_to_return;
}

const xdbus_subtree_vtable_t subtree_vtable =
{
  subtree_enumerate,
  subtree_introspect,
  subtree_dispatch,
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xuint_t registration_id;

  registration_id = g_dbus_connection_register_subtree (connection,
                                                        "/org/gtk/GDBus/TestSubtree/Devices",
                                                        &subtree_vtable,
                                                        G_DBUS_SUBTREE_FLAGS_NONE,
                                                        NULL,  /* user_data */
                                                        NULL,  /* user_data_free_func */
                                                        NULL); /* xerror_t** */
  g_assert (registration_id > 0);
}

static void
on_name_acquired (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  xpointer_t         user_data)
{
}

static void
on_name_lost (xdbus_connection_t *connection,
              const xchar_t     *name,
              xpointer_t         user_data)
{
  exit (1);
}

int
main (int argc, char *argv[])
{
  xuint_t owner_id;
  xmain_loop_t *loop;

  /* We are lazy here - we don't want to manually provide
   * the introspection data structures - so we just build
   * them from XML.
   */
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  manager_interface_info = g_dbus_node_info_lookup_interface (introspection_data, "org.gtk.GDBus.Example.Manager");
  block_interface_info = g_dbus_node_info_lookup_interface (introspection_data, "org.gtk.GDBus.Example.Block");
  partition_interface_info = g_dbus_node_info_lookup_interface (introspection_data, "org.gtk.GDBus.Example.Partition");
  g_assert (manager_interface_info != NULL);
  g_assert (block_interface_info != NULL);
  g_assert (partition_interface_info != NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.gtk.GDBus.TestSubtree",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);

  g_bus_unown_name (owner_id);

  g_dbus_node_info_unref (introspection_data);

  return 0;
}
