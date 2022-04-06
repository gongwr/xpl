/* GLib testing framework examples and tests
 *
 * Copyright (C) 2012 Red Hat Inc.
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
 * Author: Stef Walter <stefw@gnome.org>
 */

#include "config.h"

#include <gio/gio.h>

#include <sys/socket.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  xdbus_interface_skeleton_t parent;
  xint_t number;
} MockInterface;

typedef struct {
  GDBusInterfaceSkeletonClass parent_class;
} MockInterfaceClass;

static xtype_t mock_interface_get_type (void);
G_DEFINE_TYPE (MockInterface, mock_interface, XTYPE_DBUS_INTERFACE_SKELETON)

static void
mock_interface_init (MockInterface *self)
{

}

static xdbus_interface_info_t *
mock_interface_get_info (xdbus_interface_skeleton_t *skeleton)
{
  static xdbus_property_info_t path_info = {
    -1,
    "Path",
    "o",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
    NULL,
  };

  static xdbus_property_info_t number_info = {
    -1,
    "Number",
    "i",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
    NULL,
  };

  static xdbus_property_info_t *property_info[] = {
    &path_info,
    &number_info,
    NULL
  };

  static xdbus_interface_info_t interface_info = {
    -1,
    (xchar_t *) "org.mock.Interface",
    NULL,
    NULL,
    (xdbus_property_info_t **) &property_info,
    NULL
  };

  return &interface_info;
}

static xvariant_t *
mock_interface_get_property (xdbus_connection_t *connection,
                             const xchar_t *sender,
                             const xchar_t *object_path,
                             const xchar_t *interface_name,
                             const xchar_t *property_name,
                             xerror_t **error,
                             xpointer_t user_data)
{
  MockInterface *self = user_data;
  if (xstr_equal (property_name, "Path"))
    return xvariant_new_object_path (object_path);
  else if (xstr_equal (property_name, "Number"))
    return xvariant_new_int32 (self->number);
  else
    return NULL;
}

static xdbus_interface_vtable_t *
mock_interface_get_vtable (xdbus_interface_skeleton_t *interface)
{
  static xdbus_interface_vtable_t vtable = {
    NULL,
    mock_interface_get_property,
    NULL,
    { 0 }
  };

  return &vtable;
}

static xvariant_t *
mock_interface_get_properties (xdbus_interface_skeleton_t *interface)
{
  xvariant_builder_t builder;
  xdbus_interface_info_t *info;
  xdbus_interface_vtable_t *vtable;
  xuint_t n;

  /* Groan, this is completely generic code and should be in gdbus */

  info = g_dbus_interface_skeleton_get_info (interface);
  vtable = g_dbus_interface_skeleton_get_vtable (interface);

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
  for (n = 0; info->properties[n] != NULL; n++)
    {
      if (info->properties[n]->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE)
        {
          xvariant_t *value;
          g_return_val_if_fail (vtable->get_property != NULL, NULL);
          value = (vtable->get_property) (g_dbus_interface_skeleton_get_connection (interface), NULL,
                                          g_dbus_interface_skeleton_get_object_path (interface),
                                          info->name, info->properties[n]->name,
                                          NULL, interface);
          if (value != NULL)
            {
              xvariant_take_ref (value);
              xvariant_builder_add (&builder, "{sv}", info->properties[n]->name, value);
              xvariant_unref (value);
            }
        }
    }

  return xvariant_builder_end (&builder);
}

static void
mock_interface_flush (xdbus_interface_skeleton_t *skeleton)
{

}

static void
mock_interface_class_init (MockInterfaceClass *klass)
{
  GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (klass);
  skeleton_class->get_info = mock_interface_get_info;
  skeleton_class->get_properties = mock_interface_get_properties;
  skeleton_class->flush = mock_interface_flush;
  skeleton_class->get_vtable = mock_interface_get_vtable;
}
typedef struct {
  xdbus_connection_t *server;
  xdbus_connection_t *client;
  xmain_loop_t *loop;
  xasync_result_t *result;
} test_t;

static void
on_server_connection (xobject_t *source,
                      xasync_result_t *result,
                      xpointer_t user_data)
{
  test_t *test = user_data;
  xerror_t *error = NULL;

  g_assert (test->server == NULL);
  test->server = xdbus_connection_new_finish (result, &error);
  g_assert_no_error (error);
  g_assert (test->server != NULL);

  if (test->server && test->client)
    xmain_loop_quit (test->loop);
}

static void
on_client_connection (xobject_t *source,
                      xasync_result_t *result,
                      xpointer_t user_data)
{
  test_t *test = user_data;
  xerror_t *error = NULL;

  g_assert (test->client == NULL);
  test->client = xdbus_connection_new_finish (result, &error);
  g_assert_no_error (error);
  g_assert (test->client != NULL);

  if (test->server && test->client)
    xmain_loop_quit (test->loop);
}

static void
setup (test_t *test,
       xconstpointer unused)
{
  xerror_t *error = NULL;
  xsocket_t *socket;
  xsocket_connection_t *stream;
  xchar_t *guid;
  int pair[2];

  test->loop = xmain_loop_new (NULL, FALSE);

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, pair) < 0)
    {
      int errsv = errno;
      g_set_error (&error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   "%s", xstrerror (errsv));
      g_assert_no_error (error);
    }

  /* Build up the server stuff */
  socket = xsocket_new_from_fd (pair[1], &error);
  g_assert_no_error (error);

  stream = xsocket_connection_factory_create_connection (socket);
  g_assert (stream != NULL);
  xobject_unref (socket);

  guid = g_dbus_generate_guid ();
  xdbus_connection_new (XIO_STREAM (stream), guid,
                         G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_SERVER |
                         G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS,
                         NULL, NULL, on_server_connection, test);
  xobject_unref (stream);
  g_free (guid);

  /* Build up the client stuff */
  socket = xsocket_new_from_fd (pair[0], &error);
  g_assert_no_error (error);

  stream = xsocket_connection_factory_create_connection (socket);
  g_assert (stream != NULL);
  xobject_unref (socket);

  xdbus_connection_new (XIO_STREAM (stream), NULL,
                         G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                         G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS,
                         NULL, NULL, on_client_connection, test);

  xmain_loop_run (test->loop);

  g_assert (test->server);
  g_assert (test->client);

  xobject_unref (stream);
}

static void
teardown (test_t *test,
          xconstpointer unused)
{
  g_clear_object (&test->client);
  g_clear_object (&test->server);
  xmain_loop_unref (test->loop);
}

static void
on_result (xobject_t *source,
           xasync_result_t *result,
           xpointer_t user_data)
{
  test_t *test = user_data;
  g_assert (test->result == NULL);
  test->result = xobject_ref (result);
  xmain_loop_quit (test->loop);

}

static void
test_object_manager (test_t *test,
                     xconstpointer test_data)
{
  xdbus_object_manager_t *client;
  xdbus_object_manager_server_t *server;
  MockInterface *mock;
  xdbus_object_skeleton_t *skeleton;
  const xchar_t *dbus_name;
  xerror_t *error = NULL;
  xdbus_interface_t *proxy;
  xvariant_t *prop;
  const xchar_t *object_path = test_data;
  xchar_t *number1_path = NULL, *number2_path = NULL;

  if (xstrcmp0 (object_path, "/") == 0)
    {
      number1_path = xstrdup ("/number_1");
      number2_path = xstrdup ("/number_2");
    }
  else
    {
      number1_path = xstrdup_printf ("%s/number_1", object_path);
      number2_path = xstrdup_printf ("%s/number_2", object_path);
    }

  server = xdbus_object_manager_server_new (object_path);

  mock = xobject_new (mock_interface_get_type (), NULL);
  mock->number = 1;
  skeleton = xdbus_object_skeleton_new (number1_path);
  xdbus_object_skeleton_add_interface (skeleton, G_DBUS_INTERFACE_SKELETON (mock));
  xdbus_object_manager_server_export (server, skeleton);

  mock = xobject_new (mock_interface_get_type (), NULL);
  mock->number = 2;
  skeleton = xdbus_object_skeleton_new (number2_path);
  xdbus_object_skeleton_add_interface (skeleton, G_DBUS_INTERFACE_SKELETON (mock));
  xdbus_object_manager_server_export (server, skeleton);

  xdbus_object_manager_server_set_connection (server, test->server);

  dbus_name = NULL;

  xdbus_object_manager_client_new (test->client, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START,
                                    dbus_name, object_path, NULL, NULL, NULL, NULL, on_result, test);

  xmain_loop_run (test->loop);
  client = xdbus_object_manager_client_new_finish (test->result, &error);
  g_assert_no_error (error);
  g_clear_object (&test->result);

  proxy = g_dbus_object_manager_get_interface (client, number1_path, "org.mock.Interface");
  g_assert (proxy != NULL);
  prop = xdbus_proxy_get_cached_property (G_DBUS_PROXY (proxy), "Path");
  g_assert (prop != NULL);
  g_assert_cmpstr ((xchar_t *)xvariant_get_type (prop), ==, (xchar_t *)G_VARIANT_TYPE_OBJECT_PATH);
  g_assert_cmpstr (xvariant_get_string (prop, NULL), ==, number1_path);
  xvariant_unref (prop);
  prop = xdbus_proxy_get_cached_property (G_DBUS_PROXY (proxy), "Number");
  g_assert (prop != NULL);
  g_assert_cmpstr ((xchar_t *)xvariant_get_type (prop), ==, (xchar_t *)G_VARIANT_TYPE_INT32);
  g_assert_cmpint (xvariant_get_int32 (prop), ==, 1);
  xvariant_unref (prop);
  xobject_unref (proxy);

  proxy = g_dbus_object_manager_get_interface (client, number2_path, "org.mock.Interface");
  g_assert (proxy != NULL);
  prop = xdbus_proxy_get_cached_property (G_DBUS_PROXY (proxy), "Path");
  g_assert (prop != NULL);
  g_assert_cmpstr ((xchar_t *)xvariant_get_type (prop), ==, (xchar_t *)G_VARIANT_TYPE_OBJECT_PATH);
  g_assert_cmpstr (xvariant_get_string (prop, NULL), ==, number2_path);
  xvariant_unref (prop);
  prop = xdbus_proxy_get_cached_property (G_DBUS_PROXY (proxy), "Number");
  g_assert (prop != NULL);
  g_assert_cmpstr ((xchar_t *)xvariant_get_type (prop), ==, (xchar_t *)G_VARIANT_TYPE_INT32);
  g_assert_cmpint (xvariant_get_int32 (prop), ==, 2);
  xvariant_unref (prop);
  xobject_unref (proxy);

  xobject_unref (server);
  xobject_unref (client);

  g_free (number2_path);
  g_free (number1_path);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/gdbus/peer-object-manager/normal", test_t, "/objects",
              setup, test_object_manager, teardown);
  g_test_add ("/gdbus/peer-object-manager/root", test_t, "/",
              setup, test_object_manager, teardown);

  return g_test_run();
}
