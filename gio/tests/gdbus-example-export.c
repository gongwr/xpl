#include <gio/gio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------------------------------- */

/* The object we want to export */
typedef struct _xobject_class xobject_class_t;
typedef struct _xobject xobject_t;

struct _xobject_class
{
  xobject_class_t parent_class;
};

struct _xobject
{
  xobject_t parent_instance;

  xint_t count;
  xchar_t *name;
};

enum
{
  PROP_0,
  PROP_COUNT,
  PROP_NAME
};

static xtype_t my_object_get_type (void);
G_DEFINE_TYPE (xobject_t, my_object, XTYPE_OBJECT)

static void
my_object_finalize (xobject_t *object)
{
  xobject_t *myobj = (xobject_t*)object;

  g_free (myobj->name);

  G_OBJECT_CLASS (my_object_parent_class)->finalize (object);
}

static void
my_object_init (xobject_t *object)
{
  object->count = 0;
  object->name = NULL;
}

static void
my_object_get_property (xobject_t    *object,
                        xuint_t       prop_id,
                        xvalue_t     *value,
                        xparam_spec_t *pspec)
{
  xobject_t *myobj = (xobject_t*)object;

  switch (prop_id)
    {
    case PROP_COUNT:
      xvalue_set_int (value, myobj->count);
      break;

    case PROP_NAME:
      xvalue_set_string (value, myobj->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
my_object_set_property (xobject_t      *object,
                        xuint_t         prop_id,
                        const xvalue_t *value,
                        xparam_spec_t   *pspec)
{
  xobject_t *myobj = (xobject_t*)object;

  switch (prop_id)
    {
    case PROP_COUNT:
      myobj->count = xvalue_get_int (value);
      break;

    case PROP_NAME:
      g_free (myobj->name);
      myobj->name = xvalue_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
my_object_class_init (xobject_class_t *class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = my_object_finalize;
  gobject_class->set_property = my_object_set_property;
  gobject_class->get_property = my_object_get_property;

  xobject_class_install_property (gobject_class,
                                   PROP_COUNT,
                                   g_param_spec_int ("count",
                                                     "Count",
                                                     "Count",
                                                     0, 99999, 0,
                                                     G_PARAM_READWRITE));

  xobject_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Name",
                                                        NULL,
                                                        G_PARAM_READWRITE));
}

/* A method that we want to export */
static void
my_object_change_count (xobject_t *myobj,
                        xint_t      change)
{
  myobj->count = 2 * myobj->count + change;

  xobject_notify (G_OBJECT (myobj), "count");
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_node_info_t *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const xchar_t introspection_xml[] =
  "<node>"
  "  <interface name='org.myorg.xobject_t'>"
  "    <method name='ChangeCount'>"
  "      <arg type='i' name='change' direction='in'/>"
  "    </method>"
  "    <property type='i' name='Count' access='read'/>"
  "    <property type='s' name='Name' access='readwrite'/>"
  "  </interface>"
  "</node>";


static void
handle_method_call (xdbus_connection_t       *connection,
                    const xchar_t           *sender,
                    const xchar_t           *object_path,
                    const xchar_t           *interface_name,
                    const xchar_t           *method_name,
                    xvariant_t              *parameters,
                    xdbus_method_invocation_t *invocation,
                    xpointer_t               user_data)
{
  xobject_t *myobj = user_data;

  if (xstrcmp0 (method_name, "ChangeCount") == 0)
    {
      xint_t change;
      xvariant_get (parameters, "(i)", &change);

      my_object_change_count (myobj, change);

      xdbus_method_invocation_return_value (invocation, NULL);
    }
}

static xvariant_t *
handle_get_property (xdbus_connection_t  *connection,
                     const xchar_t      *sender,
                     const xchar_t      *object_path,
                     const xchar_t      *interface_name,
                     const xchar_t      *property_name,
                     xerror_t          **error,
                     xpointer_t          user_data)
{
  xvariant_t *ret;
  xobject_t *myobj = user_data;

  ret = NULL;
  if (xstrcmp0 (property_name, "Count") == 0)
    {
      ret = xvariant_new_int32 (myobj->count);
    }
  else if (xstrcmp0 (property_name, "Name") == 0)
    {
      ret = xvariant_new_string (myobj->name ? myobj->name : "");
    }

  return ret;
}

static xboolean_t
handle_set_property (xdbus_connection_t  *connection,
                     const xchar_t      *sender,
                     const xchar_t      *object_path,
                     const xchar_t      *interface_name,
                     const xchar_t      *property_name,
                     xvariant_t         *value,
                     xerror_t          **error,
                     xpointer_t          user_data)
{
  xobject_t *myobj = user_data;

  if (xstrcmp0 (property_name, "Count") == 0)
    {
      xobject_set (myobj, "count", xvariant_get_int32 (value), NULL);
    }
  else if (xstrcmp0 (property_name, "Name") == 0)
    {
      xobject_set (myobj, "name", xvariant_get_string (value, NULL), NULL);
    }

  return TRUE;
}


/* for now */
static const xdbus_interface_vtable_t interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property,
  { 0 }
};

static void
send_property_change (xobject_t         *obj,
                      xparam_spec_t      *pspec,
                      xdbus_connection_t *connection)
{
  xvariant_builder_t *builder;
  xvariant_builder_t *invalidated_builder;
  xobject_t *myobj = (xobject_t *)obj;

  builder = xvariant_builder_new (G_VARIANT_TYPE_ARRAY);
  invalidated_builder = xvariant_builder_new (G_VARIANT_TYPE ("as"));

  if (xstrcmp0 (pspec->name, "count") == 0)
    xvariant_builder_add (builder,
                           "{sv}",
                           "Count", xvariant_new_int32 (myobj->count));
  else if (xstrcmp0 (pspec->name, "name") == 0)
    xvariant_builder_add (builder,
                           "{sv}",
                           "Name", xvariant_new_string (myobj->name ? myobj->name : ""));

  xdbus_connection_emit_signal (connection,
                                 NULL,
                                 "/org/myorg/xobject_t",
                                 "org.freedesktop.DBus.Properties",
                                 "PropertiesChanged",
                                 xvariant_new ("(sa{sv}as)",
                                                "org.myorg.xobject_t",
                                                builder,
                                                invalidated_builder),
                                 NULL);
}

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xobject_t *myobj = user_data;
  xuint_t registration_id;

  xsignal_connect (myobj, "notify",
                    G_CALLBACK (send_property_change), connection);
  registration_id = xdbus_connection_register_object (connection,
                                                       "/org/myorg/xobject_t",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       myobj,
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
  xobject_t *myobj;

  /* We are lazy here - we don't want to manually provide
   * the introspection data structures - so we just build
   * them from XML.
   */
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  myobj = xobject_new (my_object_get_type (), NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.myorg.xobject_t",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             myobj,
                             NULL);

  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);

  g_bus_unown_name (owner_id);

  g_dbus_node_info_unref (introspection_data);

  xobject_unref (myobj);

  return 0;
}
