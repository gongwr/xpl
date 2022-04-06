#include <gio/gio.h>
#include <stdlib.h>

#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
/* For STDOUT_FILENO */
#include <unistd.h>
#endif

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_node_info_t *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const xchar_t introspection_xml[] =
  "<node>"
  "  <interface name='org.gtk.GDBus.test_interface_t'>"
  "    <annotation name='org.gtk.GDBus.Annotation' value='OnInterface'/>"
  "    <annotation name='org.gtk.GDBus.Annotation' value='AlsoOnInterface'/>"
  "    <method name='HelloWorld'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <method name='EmitSignal'>"
  "      <arg type='d' name='speed_in_mph' direction='in'>"
  "        <annotation name='org.gtk.GDBus.Annotation' value='OnArg'/>"
  "      </arg>"
  "    </method>"
  "    <method name='GimmeStdout'/>"
  "    <signal name='VelocityChanged'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='Onsignal'/>"
  "      <arg type='d' name='speed_in_mph'/>"
  "      <arg type='s' name='speed_as_string'>"
  "        <annotation name='org.gtk.GDBus.Annotation' value='OnArg_NonFirst'/>"
  "      </arg>"
  "    </signal>"
  "    <property type='s' name='FluxCapicitorName' access='read'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='OnProperty'>"
  "        <annotation name='org.gtk.GDBus.Annotation' value='OnAnnotation_YesThisIsCrazy'/>"
  "      </annotation>"
  "    </property>"
  "    <property type='s' name='Title' access='readwrite'/>"
  "    <property type='s' name='ReadingAlwaysThrowsError' access='read'/>"
  "    <property type='s' name='WritingAlwaysThrowsError' access='readwrite'/>"
  "    <property type='s' name='OnlyWritable' access='write'/>"
  "    <property type='s' name='foo_t' access='read'/>"
  "    <property type='s' name='Bar' access='read'/>"
  "  </interface>"
  "</node>";

/* ---------------------------------------------------------------------------------------------------- */

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
  if (xstrcmp0 (method_name, "HelloWorld") == 0)
    {
      const xchar_t *greeting;

      xvariant_get (parameters, "(&s)", &greeting);

      if (xstrcmp0 (greeting, "Return Unregistered") == 0)
        {
          xdbus_method_invocation_return_error (invocation,
                                                 G_IO_ERROR,
                                                 G_IO_ERROR_FAILED_HANDLED,
                                                 "As requested, here's a xerror_t not registered (G_IO_ERROR_FAILED_HANDLED)");
        }
      else if (xstrcmp0 (greeting, "Return Registered") == 0)
        {
          xdbus_method_invocation_return_error (invocation,
                                                 G_DBUS_ERROR,
                                                 G_DBUS_ERROR_MATCH_RULE_NOT_FOUND,
                                                 "As requested, here's a xerror_t that is registered (G_DBUS_ERROR_MATCH_RULE_NOT_FOUND)");
        }
      else if (xstrcmp0 (greeting, "Return Raw") == 0)
        {
          xdbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.SomeErrorName",
                                                      "As requested, here's a raw D-Bus error");
        }
      else
        {
          xchar_t *response;
          response = xstrdup_printf ("You greeted me with '%s'. Thanks!", greeting);
          xdbus_method_invocation_return_value (invocation,
                                                 xvariant_new ("(s)", response));
          g_free (response);
        }
    }
  else if (xstrcmp0 (method_name, "EmitSignal") == 0)
    {
      xerror_t *local_error;
      xdouble_t speed_in_mph;
      xchar_t *speed_as_string;

      xvariant_get (parameters, "(d)", &speed_in_mph);
      speed_as_string = xstrdup_printf ("%g mph!", speed_in_mph);

      local_error = NULL;
      xdbus_connection_emit_signal (connection,
                                     NULL,
                                     object_path,
                                     interface_name,
                                     "VelocityChanged",
                                     xvariant_new ("(ds)",
                                                    speed_in_mph,
                                                    speed_as_string),
                                     &local_error);
      g_assert_no_error (local_error);
      g_free (speed_as_string);

      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "GimmeStdout") == 0)
    {
#ifdef G_OS_UNIX
      if (xdbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING)
        {
          xdbus_message_t *reply;
          xunix_fd_list_t *fd_list;
          xerror_t *error;

          fd_list = g_unix_fd_list_new ();
          error = NULL;
          g_unix_fd_list_append (fd_list, STDOUT_FILENO, &error);
          g_assert_no_error (error);

          reply = xdbus_message_new_method_reply (xdbus_method_invocation_get_message (invocation));
          xdbus_message_set_unix_fd_list (reply, fd_list);

          error = NULL;
          xdbus_connection_send_message (connection,
                                          reply,
                                          G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                          NULL, /* out_serial */
                                          &error);
          g_assert_no_error (error);

          xobject_unref (invocation);
          xobject_unref (fd_list);
          xobject_unref (reply);
        }
      else
        {
          xdbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Failed",
                                                      "Your message bus daemon does not support file descriptor passing (need D-Bus >= 1.3.0)");
        }
#else
      xdbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.NotOnUnix",
                                                  "Your OS does not support file descriptor passing");
#endif
    }
}

static xchar_t *_global_title = NULL;

static xboolean_t swap_a_and_b = FALSE;

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

  ret = NULL;
  if (xstrcmp0 (property_name, "FluxCapicitorName") == 0)
    {
      ret = xvariant_new_string ("DeLorean");
    }
  else if (xstrcmp0 (property_name, "Title") == 0)
    {
      if (_global_title == NULL)
        _global_title = xstrdup ("Back To C!");
      ret = xvariant_new_string (_global_title);
    }
  else if (xstrcmp0 (property_name, "ReadingAlwaysThrowsError") == 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Hello %s. I thought I said reading this property "
                   "always results in an error. kthxbye",
                   sender);
    }
  else if (xstrcmp0 (property_name, "WritingAlwaysThrowsError") == 0)
    {
      ret = xvariant_new_string ("There's no home like home");
    }
  else if (xstrcmp0 (property_name, "foo_t") == 0)
    {
      ret = xvariant_new_string (swap_a_and_b ? "Tock" : "Tick");
    }
  else if (xstrcmp0 (property_name, "Bar") == 0)
    {
      ret = xvariant_new_string (swap_a_and_b ? "Tick" : "Tock");
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
  if (xstrcmp0 (property_name, "Title") == 0)
    {
      if (xstrcmp0 (_global_title, xvariant_get_string (value, NULL)) != 0)
        {
          xvariant_builder_t *builder;
          xerror_t *local_error;

          g_free (_global_title);
          _global_title = xvariant_dup_string (value, NULL);

          local_error = NULL;
          builder = xvariant_builder_new (G_VARIANT_TYPE_ARRAY);
          xvariant_builder_add (builder,
                                 "{sv}",
                                 "Title",
                                 xvariant_new_string (_global_title));
          xdbus_connection_emit_signal (connection,
                                         NULL,
                                         object_path,
                                         "org.freedesktop.DBus.Properties",
                                         "PropertiesChanged",
                                         xvariant_new ("(sa{sv}as)",
                                                        interface_name,
                                                        builder,
                                                        NULL),
                                         &local_error);
          g_assert_no_error (local_error);
        }
    }
  else if (xstrcmp0 (property_name, "ReadingAlwaysThrowsError") == 0)
    {
      /* do nothing - they can't read it after all! */
    }
  else if (xstrcmp0 (property_name, "WritingAlwaysThrowsError") == 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Hello AGAIN %s. I thought I said writing this property "
                   "always results in an error. kthxbye",
                   sender);
    }

  return *error == NULL;
}


/* for now */
static const xdbus_interface_vtable_t interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property,
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
on_timeout_cb (xpointer_t user_data)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (user_data);
  xvariant_builder_t *builder;
  xvariant_builder_t *invalidated_builder;
  xerror_t *error;

  swap_a_and_b = !swap_a_and_b;

  error = NULL;
  builder = xvariant_builder_new (G_VARIANT_TYPE_ARRAY);
  invalidated_builder = xvariant_builder_new (G_VARIANT_TYPE ("as"));
  xvariant_builder_add (builder,
                         "{sv}",
                         "foo_t",
                         xvariant_new_string (swap_a_and_b ? "Tock" : "Tick"));
  xvariant_builder_add (builder,
                         "{sv}",
                         "Bar",
                         xvariant_new_string (swap_a_and_b ? "Tick" : "Tock"));
  xdbus_connection_emit_signal (connection,
                                 NULL,
                                 "/org/gtk/GDBus/test_object_t",
                                 "org.freedesktop.DBus.Properties",
                                 "PropertiesChanged",
                                 xvariant_new ("(sa{sv}as)",
                                                "org.gtk.GDBus.test_interface_t",
                                                builder,
                                                invalidated_builder),
                                 &error);
  g_assert_no_error (error);


  return G_SOURCE_CONTINUE;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xuint_t registration_id;

  registration_id = xdbus_connection_register_object (connection,
                                                       "/org/gtk/GDBus/test_object_t",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL,  /* user_data */
                                                       NULL,  /* user_data_free_func */
                                                       NULL); /* xerror_t** */
  g_assert (registration_id > 0);

  /* swap value of properties foo_t and Bar every two seconds */
  g_timeout_add_seconds (2,
                         on_timeout_cb,
                         connection);
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

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.gtk.GDBus.TestServer",
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
