/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include <gio/gio.h>
#include <unistd.h>
#include <string.h>

#include "gdbus-tests.h"

/* all tests rely on a shared mainloop */
static xmain_loop_t *loop = NULL;

/* ---------------------------------------------------------------------------------------------------- */
/* test_t introspection parser */
/* ---------------------------------------------------------------------------------------------------- */

static void
test_introspection (xdbus_proxy_t *proxy)
{
  xerror_t *error;
  const xchar_t *xml_data;
  xdbus_node_info_t *node_info;
  xdbus_interface_info_t *interface_info;
  xdbus_method_info_t *method_info;
  xdbus_signalInfo_t *signal_info;
  xvariant_t *result;

  error = NULL;

  /*
   * Invoke Introspect(), then parse the output.
   */
  result = xdbus_proxy_call_sync (proxy,
                                   "org.freedesktop.DBus.Introspectable.Introspect",
                                   NULL,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  xvariant_get (result, "(&s)", &xml_data);

  node_info = g_dbus_node_info_new_for_xml (xml_data, &error);
  g_assert_no_error (error);
  g_assert (node_info != NULL);

  /* for now we only check a couple of things. TODO: check more things */

  interface_info = g_dbus_node_info_lookup_interface (node_info, "com.example.NonExistantInterface");
  g_assert (interface_info == NULL);

  interface_info = g_dbus_node_info_lookup_interface (node_info, "org.freedesktop.DBus.Introspectable");
  g_assert (interface_info != NULL);
  method_info = g_dbus_interface_info_lookup_method (interface_info, "NonExistantMethod");
  g_assert (method_info == NULL);
  method_info = g_dbus_interface_info_lookup_method (interface_info, "Introspect");
  g_assert (method_info != NULL);
  g_assert (method_info->in_args != NULL);
  g_assert (method_info->in_args[0] == NULL);
  g_assert (method_info->out_args != NULL);
  g_assert (method_info->out_args[0] != NULL);
  g_assert (method_info->out_args[1] == NULL);
  g_assert_cmpstr (method_info->out_args[0]->signature, ==, "s");

  interface_info = g_dbus_node_info_lookup_interface (node_info, "com.example.Frob");
  g_assert (interface_info != NULL);
  signal_info = g_dbus_interface_info_lookup_signal (interface_info, "TestSignal");
  g_assert (signal_info != NULL);
  g_assert (signal_info->args != NULL);
  g_assert (signal_info->args[0] != NULL);
  g_assert_cmpstr (signal_info->args[0]->signature, ==, "s");
  g_assert (signal_info->args[1] != NULL);
  g_assert_cmpstr (signal_info->args[1]->signature, ==, "o");
  g_assert (signal_info->args[2] != NULL);
  g_assert_cmpstr (signal_info->args[2]->signature, ==, "v");
  g_assert (signal_info->args[3] == NULL);

  g_dbus_node_info_unref (node_info);
  xvariant_unref (result);

  xmain_loop_quit (loop);
}

static void
test_introspection_parser (void)
{
  xdbus_proxy_t *proxy;
  xdbus_connection_t *connection;
  xerror_t *error;

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                               NULL,
                               &error);
  g_assert_no_error (error);
  error = NULL;
  proxy = xdbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,                      /* xdbus_interface_info_t */
                                 "com.example.TestService", /* name */
                                 "/com/example/test_object_t", /* object path */
                                 "com.example.Frob",        /* interface */
                                 NULL, /* xcancellable_t */
                                 &error);
  g_assert_no_error (error);

  /* this is safe; testserver will exit once the bus goes away */
  g_assert (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  _g_assert_property_notify (proxy, "g-name-owner");

  test_introspection (proxy);

  xobject_unref (proxy);
  xobject_unref (connection);
}

/* check that a parse-generate roundtrip produces identical results
 */
static void
test_generate (void)
{
  xdbus_node_info_t *info;
  xdbus_node_info_t *info2;
  xdbus_interface_info_t *iinfo;
  xdbus_method_info_t *minfo;
  xdbus_signalInfo_t *sinfo;
  xdbus_arg_info_t *arginfo;
  xdbus_property_info_t *pinfo;
  xdbus_annotation_info_t *aninfo;
  const xchar_t *data =
  "  <node>"
  "    <interface name='com.example.Frob'>"
  "      <annotation name='foo' value='bar'/>"
  "      <method name='PairReturn'>"
  "        <annotation name='org.freedesktop.DBus.GLib.Async' value=''/>"
  "        <arg type='u' name='somenumber' direction='in'/>"
  "        <arg type='s' name='somestring' direction='out'/>"
  "      </method>"
  "      <signal name='HelloWorld'>"
  "        <arg type='s' name='greeting' direction='out'/>"
  "      </signal>"
  "      <method name='Sleep'>"
  "        <arg type='i' name='timeout' direction='in'/>"
  "      </method>"
  "      <property name='y' type='y' access='readwrite'>"
  "        <annotation name='needs-escaping' value='bar&lt;&gt;&apos;&quot;'/>"
  "      </property>"
  "    </interface>"
  "  </node>";

  xstring_t *string;
  xstring_t *string2;
  xerror_t *error;

  error = NULL;
  info = g_dbus_node_info_new_for_xml (data, &error);
  g_assert_no_error (error);

  iinfo = g_dbus_node_info_lookup_interface (info, "com.example.Frob");
  aninfo = iinfo->annotations[0];
  g_assert_cmpstr (aninfo->key, ==, "foo");
  g_assert_cmpstr (aninfo->value, ==, "bar");
  g_assert (iinfo->annotations[1] == NULL);
  minfo = g_dbus_interface_info_lookup_method (iinfo, "PairReturn");
  g_assert_cmpstr (g_dbus_annotation_info_lookup (minfo->annotations, "org.freedesktop.DBus.GLib.Async"), ==, "");
  arginfo = minfo->in_args[0];
  g_assert_cmpstr (arginfo->name, ==, "somenumber");
  g_assert_cmpstr (arginfo->signature, ==, "u");
  g_assert (minfo->in_args[1] == NULL);
  arginfo = minfo->out_args[0];
  g_assert_cmpstr (arginfo->name, ==, "somestring");
  g_assert_cmpstr (arginfo->signature, ==, "s");
  g_assert (minfo->out_args[1] == NULL);
  sinfo = g_dbus_interface_info_lookup_signal (iinfo, "HelloWorld");
  arginfo = sinfo->args[0];
  g_assert_cmpstr (arginfo->name, ==, "greeting");
  g_assert_cmpstr (arginfo->signature, ==, "s");
  g_assert (sinfo->args[1] == NULL);
  pinfo = g_dbus_interface_info_lookup_property (iinfo, "y");
  g_assert_cmpstr (pinfo->signature, ==, "y");
  g_assert_cmpint (pinfo->flags, ==, G_DBUS_PROPERTY_INFO_FLAGS_READABLE |
                                     G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE);

  string = xstring_new ("");
  g_dbus_node_info_generate_xml (info, 2, string);

  info2 = g_dbus_node_info_new_for_xml (string->str, &error);
  string2 = xstring_new ("");
  g_dbus_node_info_generate_xml (info2, 2, string2);

  g_assert_cmpstr (string->str, ==, string2->str);
  xstring_free (string, TRUE);
  xstring_free (string2, TRUE);

  g_dbus_node_info_unref (info);
  g_dbus_node_info_unref (info2);
}

/* test that omitted direction attributes default to 'out' for signals,
 * and 'in' for methods.
 */
static void
test_default_direction (void)
{
  xdbus_node_info_t *info;
  xdbus_interface_info_t *iinfo;
  xdbus_method_info_t *minfo;
  xdbus_signalInfo_t *sinfo;
  xdbus_arg_info_t *arginfo;
  const xchar_t *data =
  "  <node>"
  "    <interface name='com.example.Frob'>"
  "      <signal name='HelloWorld'>"
  "        <arg type='s' name='greeting'/>"
  "      </signal>"
  "      <method name='Sleep'>"
  "        <arg type='i' name='timeout'/>"
  "      </method>"
  "    </interface>"
  "  </node>";

  xerror_t *error;

  error = NULL;
  info = g_dbus_node_info_new_for_xml (data, &error);
  g_assert_no_error (error);

  iinfo = g_dbus_node_info_lookup_interface (info, "com.example.Frob");
  sinfo = g_dbus_interface_info_lookup_signal (iinfo, "HelloWorld");
  g_assert (sinfo->args != NULL);
  arginfo = sinfo->args[0];
  g_assert_cmpstr (arginfo->name, ==, "greeting");
  g_assert (sinfo->args[1] == NULL);
  minfo = g_dbus_interface_info_lookup_method (iinfo, "Sleep");
  g_assert (minfo->in_args != NULL);
  arginfo = minfo->in_args[0];
  g_assert_cmpstr (arginfo->name, ==, "timeout");
  g_assert (minfo->in_args[1] == NULL);

  g_dbus_node_info_unref (info);
}

static void
test_extra_data (void)
{
  xdbus_node_info_t *info;
  const xchar_t *data =
  "  <node>"
  "    <interface name='com.example.Frob' version='1.0'>"
  "      <doc:doc><doc:description><doc:para>Blah blah</doc:para></doc:description></doc:doc>"
  "      <method name='DownloadPackages'>"
  "        <arg type='u' name='somenumber' direction='in'>"
  "          <doc:doc><doc:summary><doc:para>"
  "            See <doc:ulink url='http:///example.com'>example</doc:ulink>"
  "          </doc:para></doc:summary></doc:doc>"
  "        </arg>"
  "        <arg type='s' name='somestring' direction='out'>"
  "          <doc:doc><doc:summary><doc:para>"
  "            More docs"
  "          </doc:para></doc:summary></doc:doc>"
  "        </arg>"
  "      </method>"
  "      <signal name='HelloWorld'>"
  "        <arg type='s' name='somestring'/>"
  "      </signal>"
  "      <method name='Sleep'>"
  "        <arg type='i' name='timeout' direction='in'/>"
  "      </method>"
  "      <property name='y' type='y' access='readwrite'/>"
  "    </interface>"
  "  </node>";
  xerror_t *error;

  error = NULL;
  info = g_dbus_node_info_new_for_xml (data, &error);
  g_assert_no_error (error);

  g_dbus_node_info_unref (info);
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  xint_t ret;

  g_test_init (&argc, &argv, NULL);

  /* all the tests rely on a shared main loop */
  loop = xmain_loop_new (NULL, FALSE);

  g_test_add_func ("/gdbus/introspection-parser", test_introspection_parser);
  g_test_add_func ("/gdbus/introspection-generate", test_generate);
  g_test_add_func ("/gdbus/introspection-default-direction", test_default_direction);
  g_test_add_func ("/gdbus/introspection-extra-data", test_extra_data);

  ret = session_bus_run ();

  while (xmain_context_iteration (NULL, FALSE));
  xmain_loop_unref (loop);

  return ret;
}
