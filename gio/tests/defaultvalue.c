/* GIO default value tests
 * Copyright (C) 2013 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <gio/gio.h>

static void
check_property (const char *output,
	        xparam_spec_t *pspec,
		xvalue_t *value)
{
  xvalue_t default_value = G_VALUE_INIT;
  char *v, *dv, *msg;

  if (g_param_value_defaults (pspec, value))
      return;

  g_param_value_set_default (pspec, &default_value);

  v = xstrdup_value_contents (value);
  dv = xstrdup_value_contents (&default_value);

  msg = xstrdup_printf ("%s %s.%s: %s != %s\n",
			 output,
			 xtype_name (pspec->owner_type),
			 pspec->name,
			 dv, v);
  g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, msg);
  g_free (msg);

  g_free (v);
  g_free (dv);
  xvalue_unset (&default_value);
}

static void
test_type (xconstpointer data)
{
  xobject_class_t *klass;
  xobject_t *instance;
  xparam_spec_t **pspecs;
  xuint_t n_pspecs, i;
  xtype_t type;

  type = * (xtype_t *) data;

  if (xtype_is_a (type, XTYPE_APP_INFO_MONITOR))
    {
      g_test_skip ("singleton");
      return;
    }

  if (xtype_is_a (type, XTYPE_BINDING) ||
      xtype_is_a (type, XTYPE_BUFFERED_INPUT_STREAM) ||
      xtype_is_a (type, XTYPE_BUFFERED_OUTPUT_STREAM) ||
      xtype_is_a (type, XTYPE_CHARSET_CONVERTER) ||
      xtype_is_a (type, XTYPE_DBUS_ACTION_GROUP) ||
      xtype_is_a (type, XTYPE_DBUS_CONNECTION) ||
      xtype_is_a (type, XTYPE_DBUS_OBJECT_MANAGER_CLIENT) ||
      xtype_is_a (type, XTYPE_DBUS_OBJECT_MANAGER_SERVER) ||
      xtype_is_a (type, XTYPE_DBUS_PROXY) ||
      xtype_is_a (type, XTYPE_DBUS_SERVER) ||
      xtype_is_a (type, XTYPE_FILTER_OUTPUT_STREAM) ||
      xtype_is_a (type, XTYPE_FILTER_INPUT_STREAM) ||
      xtype_is_a (type, XTYPE_INET_ADDRESS) ||
      xtype_is_a (type, XTYPE_INET_SOCKET_ADDRESS) ||
      xtype_is_a (type, XTYPE_PROPERTY_ACTION) ||
      xtype_is_a (type, XTYPE_SETTINGS) ||
      xtype_is_a (type, XTYPE_SOCKET_CONNECTION) ||
      xtype_is_a (type, XTYPE_SIMPLE_IO_STREAM) ||
      xtype_is_a (type, XTYPE_THEMED_ICON) ||
      FALSE)
    {
      g_test_skip ("mandatory construct params");
      return;
    }

  if (xtype_is_a (type, XTYPE_DBUS_MENU_MODEL) ||
      xtype_is_a (type, XTYPE_DBUS_METHOD_INVOCATION))
    {
      g_test_skip ("crash in finalize");
      return;
    }

  if (xtype_is_a (type, XTYPE_FILE_ENUMERATOR) ||
      xtype_is_a (type, XTYPE_FILE_IO_STREAM))
    {
      g_test_skip ("should be abstract");
      return;
    }

  klass = xtype_class_ref (type);
  instance = xobject_new (type, NULL);

  if (X_IS_INITABLE (instance) &&
      !xinitable_init (XINITABLE (instance), NULL, NULL))
    {
      g_test_skip ("initialization failed");
      xobject_unref (instance);
      xtype_class_unref (klass);
      return;
    }

  if (xtype_is_a (type, XTYPE_INITIALLY_UNOWNED))
    xobject_ref_sink (instance);

  pspecs = xobject_class_list_properties (klass, &n_pspecs);
  for (i = 0; i < n_pspecs; ++i)
    {
      xparam_spec_t *pspec = pspecs[i];
      xvalue_t value = G_VALUE_INIT;

      if (pspec->owner_type != type)
	continue;

      if ((pspec->flags & XPARAM_READABLE) == 0)
	continue;

      if (xtype_is_a (type, XTYPE_APPLICATION) &&
          (strcmp (pspec->name, "is-remote") == 0))
        {
          g_test_message ("skipping xapplication_t:is-remote");
          continue;
        }

      if (xtype_is_a (type, XTYPE_PROXY_ADDRESS_ENUMERATOR) &&
          (strcmp (pspec->name, "proxy-resolver") == 0))
        {
          g_test_message ("skipping xproxy_address_enumerator_t:proxy-resolver");
          continue;
        }

      if (xtype_is_a (type, XTYPE_SOCKET_CLIENT) &&
          (strcmp (pspec->name, "proxy-resolver") == 0))
        {
          g_test_message ("skipping xsocket_client_t:proxy-resolver");
          continue;
        }

      if (g_test_verbose ())
        g_printerr ("Property %s.%s\n", xtype_name (pspec->owner_type), pspec->name);
      xvalue_init (&value, XPARAM_SPEC_VALUE_TYPE (pspec));
      xobject_get_property (instance, pspec->name, &value);
      check_property ("Property", pspec, &value);
      xvalue_unset (&value);
    }
  g_free (pspecs);
  xobject_unref (instance);
  xtype_class_unref (klass);
}

static xtype_t *all_registered_types;

static const xtype_t *
list_all_types (void)
{
  if (!all_registered_types)
    {
      xtype_t *tp;
      all_registered_types = g_new0 (xtype_t, 1000);
      tp = all_registered_types;
#include "giotypefuncs.inc"
      *tp = 0;
    }

  return all_registered_types;
}

int
main (int argc, char **argv)
{
  const xtype_t *otypes;
  xuint_t i;
  xtest_dbus_t *bus;
  xint_t result;

  g_setenv ("GIO_USE_VFS", "local", TRUE);
  g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);

  g_test_init (&argc, &argv, NULL);

  /* Create one test bus for all tests, as we have a lot of very small
   * and quick tests.
   */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  otypes = list_all_types ();
  for (i = 0; otypes[i]; i++)
    {
      xchar_t *testname;

      if (!XTYPE_IS_CLASSED (otypes[i]))
        continue;

      if (XTYPE_IS_ABSTRACT (otypes[i]))
        continue;

      if (!xtype_is_a (otypes[i], XTYPE_OBJECT))
        continue;

      testname = xstrdup_printf ("/Default Values/%s",
				  xtype_name (otypes[i]));
      g_test_add_data_func (testname, &otypes[i], test_type);
      g_free (testname);
    }

  result = g_test_run ();

  g_test_dbus_down (bus);
  xobject_unref (bus);

  return result;
}
