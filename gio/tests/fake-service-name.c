/*
 * Copyright (C) 2021 Frederic Martinsons
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Frederic Martinsons <frederic.martinsons@gmail.com>
 */

/* A dummy service which just own a dbus name and implement a method to quit*/

#include <gio/gio.h>
#include <glib.h>

static xdbus_node_info_t *introspection_data = NULL;
static xmain_loop_t *loop = NULL;
static const xchar_t introspection_xml[] =
    "<node>"
    "    <interface name='org.gtk.GDBus.FakeService'>"
    "        <method name='Quit'/>"
    "    </interface>"
    "</node>";

static void
incoming_method_call (xdbus_connection_t       *connection,
                      const xchar_t           *sender,
                      const xchar_t           *object_path,
                      const xchar_t           *interface_name,
                      const xchar_t           *method_name,
                      xvariant_t              *parameters,
                      xdbus_method_invocation_t *invocation,
                      xpointer_t               user_data)
{
  if (xstrcmp0 (method_name, "Quit") == 0)
    {
      xdbus_method_invocation_return_value (invocation, NULL);
      xmain_loop_quit (loop);
    }
}

static const xdbus_interface_vtable_t interface_vtable = {
  incoming_method_call,
  NULL,
  NULL,
  { 0 }
};

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xuint_t registration_id;
  xerror_t *error = NULL;
  g_test_message ("Acquired a message bus connection");

  registration_id = xdbus_connection_register_object (connection,
                                                       "/org/gtk/GDBus/FakeService",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL, /* user_data */
                                                       NULL, /* user_data_free_func */
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
}

static void
on_name_acquired (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  xpointer_t         user_data)
{
  g_test_message ("Acquired the name %s", name);
}

static void
on_name_lost (xdbus_connection_t *connection,
              const xchar_t     *name,
              xpointer_t         user_data)
{
  g_test_message ("Lost the name %s", name);
}

xint_t
main (xint_t argc, xchar_t *argv[])
{
  xuint_t id;

  loop = xmain_loop_new (NULL, FALSE);
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       "org.gtk.GDBus.FakeService",
                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                           G_BUS_NAME_OWNER_FLAGS_REPLACE,
                       on_bus_acquired,
                       on_name_acquired,
                       on_name_lost,
                       loop,
                       NULL);

  xmain_loop_run (loop);

  g_bus_unown_name (id);
  xmain_loop_unref (loop);
  g_dbus_node_info_unref (introspection_data);

  return 0;
}
