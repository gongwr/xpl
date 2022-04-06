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

static xdbus_connection_t *c = NULL;

/* ---------------------------------------------------------------------------------------------------- */
/* Test that we can export objects, the hierarchy is correct and the right handlers are invoked */
/* ---------------------------------------------------------------------------------------------------- */

static const xdbus_arg_info_t foo_method1_in_args =
{
  -1,
  "an_input_string",
  "s",
  NULL
};
static const xdbus_arg_info_t * const foo_method1_in_arg_pointers[] = {&foo_method1_in_args, NULL};

static const xdbus_arg_info_t foo_method1_out_args =
{
  -1,
  "an_output_string",
  "s",
  NULL
};
static const xdbus_arg_info_t * const foo_method1_out_arg_pointers[] = {&foo_method1_out_args, NULL};

static const xdbus_method_info_t foo_method_info_method1 =
{
  -1,
  "Method1",
  (xdbus_arg_info_t **) &foo_method1_in_arg_pointers,
  (xdbus_arg_info_t **) &foo_method1_out_arg_pointers,
  NULL
};
static const xdbus_method_info_t foo_method_info_method2 =
{
  -1,
  "Method2",
  NULL,
  NULL,
  NULL
};
static const xdbus_method_info_t * const foo_method_info_pointers[] = {&foo_method_info_method1, &foo_method_info_method2, NULL};

static const xdbus_signalInfo_t foo_signal_info =
{
  -1,
  "SignalAlpha",
  NULL,
  NULL
};
static const xdbus_signalInfo_t * const foo_signal_info_pointers[] = {&foo_signal_info, NULL};

static const xdbus_property_info_t foo_property_info[] =
{
  {
    -1,
    "PropertyUno",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
    NULL
  },
  {
    -1,
    "NotWritable",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
    NULL
  },
  {
    -1,
    "NotReadable",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
    NULL
  }
};
static const xdbus_property_info_t * const foo_property_info_pointers[] =
{
  &foo_property_info[0],
  &foo_property_info[1],
  &foo_property_info[2],
  NULL
};

static const xdbus_interface_info_t foo_interface_info =
{
  -1,
  "org.example.foo_t",
  (xdbus_method_info_t **) &foo_method_info_pointers,
  (xdbus_signalInfo_t **) &foo_signal_info_pointers,
  (xdbus_property_info_t **)&foo_property_info_pointers,
  NULL,
};

/* Foo2 is just foo_t without the properties */
static const xdbus_interface_info_t foo2_interface_info =
{
  -1,
  "org.example.Foo2",
  (xdbus_method_info_t **) &foo_method_info_pointers,
  (xdbus_signalInfo_t **) &foo_signal_info_pointers,
  NULL,
  NULL
};

static void
foo_method_call (xdbus_connection_t       *connection,
                 const xchar_t           *sender,
                 const xchar_t           *object_path,
                 const xchar_t           *interface_name,
                 const xchar_t           *method_name,
                 xvariant_t              *parameters,
                 xdbus_method_invocation_t *invocation,
                 xpointer_t               user_data)
{
  if (xstrcmp0 (method_name, "Method1") == 0)
    {
      const xchar_t *input;
      xchar_t *output;
      xvariant_get (parameters, "(&s)", &input);
      output = xstrdup_printf ("You passed the string '%s'. Jolly good!", input);
      xdbus_method_invocation_return_value (invocation, xvariant_new ("(s)", output));
      g_free (output);
    }
  else
    {
      xdbus_method_invocation_return_dbus_error (invocation,
                                                  "org.example.SomeError",
                                                  "How do you like them apples, buddy!");
    }
}

static xvariant_t *
foo_get_property (xdbus_connection_t       *connection,
                  const xchar_t           *sender,
                  const xchar_t           *object_path,
                  const xchar_t           *interface_name,
                  const xchar_t           *property_name,
                  xerror_t               **error,
                  xpointer_t               user_data)
{
  xvariant_t *ret;
  xchar_t *s;
  s = xstrdup_printf ("Property '%s' Is What It Is!", property_name);
  ret = xvariant_new_string (s);
  g_free (s);
  return ret;
}

static xboolean_t
foo_set_property (xdbus_connection_t       *connection,
                  const xchar_t           *sender,
                  const xchar_t           *object_path,
                  const xchar_t           *interface_name,
                  const xchar_t           *property_name,
                  xvariant_t              *value,
                  xerror_t               **error,
                  xpointer_t               user_data)
{
  xchar_t *s;
  s = xvariant_print (value, TRUE);
  g_set_error (error,
               G_DBUS_ERROR,
               G_DBUS_ERROR_SPAWN_FILE_INVALID,
               "Returning some error instead of writing the value '%s' to the property '%s'",
               s, property_name);
  g_free (s);
  return FALSE;
}

static const xdbus_interface_vtable_t foo_vtable =
{
  foo_method_call,
  foo_get_property,
  foo_set_property,
  { 0 },
};

/* -------------------- */

static const xdbus_method_info_t bar_method_info[] =
{
  {
    -1,
    "MethodA",
    NULL,
    NULL,
    NULL
  },
  {
    -1,
    "MethodB",
    NULL,
    NULL,
    NULL
  }
};
static const xdbus_method_info_t * const bar_method_info_pointers[] = {&bar_method_info[0], &bar_method_info[1], NULL};

static const xdbus_signalInfo_t bar_signal_info[] =
{
  {
    -1,
    "SignalMars",
    NULL,
    NULL
  }
};
static const xdbus_signalInfo_t * const bar_signal_info_pointers[] = {&bar_signal_info[0], NULL};

static const xdbus_property_info_t bar_property_info[] =
{
  {
    -1,
    "PropertyDuo",
    "s",
    G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
    NULL
  }
};
static const xdbus_property_info_t * const bar_property_info_pointers[] = {&bar_property_info[0], NULL};

static const xdbus_interface_info_t bar_interface_info =
{
  -1,
  "org.example.Bar",
  (xdbus_method_info_t **) bar_method_info_pointers,
  (xdbus_signalInfo_t **) bar_signal_info_pointers,
  (xdbus_property_info_t **) bar_property_info_pointers,
  NULL,
};

/* -------------------- */

static const xdbus_method_info_t dyna_method_info[] =
{
  {
    -1,
    "DynaCyber",
    NULL,
    NULL,
    NULL
  }
};
static const xdbus_method_info_t * const dyna_method_info_pointers[] = {&dyna_method_info[0], NULL};

static const xdbus_interface_info_t dyna_interface_info =
{
  -1,
  "org.example.Dyna",
  (xdbus_method_info_t **) dyna_method_info_pointers,
  NULL, /* no signals */
  NULL, /* no properties */
  NULL,
};

static void
dyna_cyber (xdbus_connection_t *connection,
            const xchar_t *sender,
            const xchar_t *object_path,
            const xchar_t *interface_name,
            const xchar_t *method_name,
            xvariant_t *parameters,
            xdbus_method_invocation_t *invocation,
            xpointer_t user_data)
{
  xptr_array_t *data = user_data;
  xchar_t *node_name;
  xuint_t n;

  node_name = strrchr (object_path, '/') + 1;

  /* Add new node if it is not already known */
  for (n = 0; n < data->len ; n++)
    {
      if (xstrcmp0 (xptr_array_index (data, n), node_name) == 0)
        goto out;
    }
  xptr_array_add (data, xstrdup(node_name));

  out:
    xdbus_method_invocation_return_value (invocation, NULL);
}

static const xdbus_interface_vtable_t dyna_interface_vtable =
{
  dyna_cyber,
  NULL,
  NULL,
  { 0 }
};

/* ---------------------------------------------------------------------------------------------------- */

static void
introspect_callback (xdbus_proxy_t   *proxy,
                     xasync_result_t *res,
                     xpointer_t      user_data)
{
  xchar_t **xml_data = user_data;
  xvariant_t *result;
  xerror_t *error;

  error = NULL;
  result = g_dbus_proxy_call_finish (proxy,
                                              res,
                                              &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  xvariant_get (result, "(s)", xml_data);
  xvariant_unref (result);

  xmain_loop_quit (loop);
}

static xint_t
compare_strings (xconstpointer a,
                 xconstpointer b)
{
  const xchar_t *sa = *(const xchar_t **) a;
  const xchar_t *sb = *(const xchar_t **) b;

  /* Array terminator must sort last */
  if (sa == NULL)
    return 1;
  if (sb == NULL)
    return -1;

  return strcmp (sa, sb);
}

static xchar_t **
get_nodes_at (xdbus_connection_t  *c,
              const xchar_t      *object_path)
{
  xerror_t *error;
  xdbus_proxy_t *proxy;
  xchar_t *xml_data;
  xptr_array_t *p;
  xdbus_node_info_t *node_info;
  xuint_t n;

  error = NULL;
  proxy = g_dbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                 G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                 NULL,
                                 g_dbus_connection_get_unique_name (c),
                                 object_path,
                                 "org.freedesktop.DBus.Introspectable",
                                 NULL,
                                 &error);
  g_assert_no_error (error);
  g_assert (proxy != NULL);

  /* do this async to avoid libdbus-1 deadlocks */
  xml_data = NULL;
  g_dbus_proxy_call (proxy,
                     "Introspect",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (xasync_ready_callback_t) introspect_callback,
                     &xml_data);
  xmain_loop_run (loop);
  g_assert (xml_data != NULL);

  node_info = g_dbus_node_info_new_for_xml (xml_data, &error);
  g_assert_no_error (error);
  g_assert (node_info != NULL);

  p = xptr_array_new ();
  for (n = 0; node_info->nodes != NULL && node_info->nodes[n] != NULL; n++)
    {
      const xdbus_node_info_t *sub_node_info = node_info->nodes[n];
      xptr_array_add (p, xstrdup (sub_node_info->path));
    }
  xptr_array_add (p, NULL);

  xobject_unref (proxy);
  g_free (xml_data);
  g_dbus_node_info_unref (node_info);

  /* Nodes are semantically unordered; sort array so tests can rely on order */
  xptr_array_sort (p, compare_strings);

  return (xchar_t **) xptr_array_free (p, FALSE);
}

static xboolean_t
has_interface (xdbus_connection_t *c,
               const xchar_t     *object_path,
               const xchar_t     *interface_name)
{
  xerror_t *error;
  xdbus_proxy_t *proxy;
  xchar_t *xml_data;
  xdbus_node_info_t *node_info;
  xboolean_t ret;

  error = NULL;
  proxy = g_dbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                 G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                 NULL,
                                 g_dbus_connection_get_unique_name (c),
                                 object_path,
                                 "org.freedesktop.DBus.Introspectable",
                                 NULL,
                                 &error);
  g_assert_no_error (error);
  g_assert (proxy != NULL);

  /* do this async to avoid libdbus-1 deadlocks */
  xml_data = NULL;
  g_dbus_proxy_call (proxy,
                     "Introspect",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (xasync_ready_callback_t) introspect_callback,
                     &xml_data);
  xmain_loop_run (loop);
  g_assert (xml_data != NULL);

  node_info = g_dbus_node_info_new_for_xml (xml_data, &error);
  g_assert_no_error (error);
  g_assert (node_info != NULL);

  ret = (g_dbus_node_info_lookup_interface (node_info, interface_name) != NULL);

  xobject_unref (proxy);
  g_free (xml_data);
  g_dbus_node_info_unref (node_info);

  return ret;
}

static xuint_t
count_interfaces (xdbus_connection_t *c,
                  const xchar_t     *object_path)
{
  xerror_t *error;
  xdbus_proxy_t *proxy;
  xchar_t *xml_data;
  xdbus_node_info_t *node_info;
  xuint_t ret;

  error = NULL;
  proxy = g_dbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                 G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                 NULL,
                                 g_dbus_connection_get_unique_name (c),
                                 object_path,
                                 "org.freedesktop.DBus.Introspectable",
                                 NULL,
                                 &error);
  g_assert_no_error (error);
  g_assert (proxy != NULL);

  /* do this async to avoid libdbus-1 deadlocks */
  xml_data = NULL;
  g_dbus_proxy_call (proxy,
                     "Introspect",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (xasync_ready_callback_t) introspect_callback,
                     &xml_data);
  xmain_loop_run (loop);
  g_assert (xml_data != NULL);

  node_info = g_dbus_node_info_new_for_xml (xml_data, &error);
  g_assert_no_error (error);
  g_assert (node_info != NULL);

  ret = 0;
  while (node_info->interfaces != NULL && node_info->interfaces[ret] != NULL)
    ret++;

  xobject_unref (proxy);
  g_free (xml_data);
  g_dbus_node_info_unref (node_info);

  return ret;
}

static void
dyna_create_callback (xdbus_proxy_t   *proxy,
                      xasync_result_t  *res,
                      xpointer_t      user_data)
{
  xvariant_t *result;
  xerror_t *error;

  error = NULL;
  result = g_dbus_proxy_call_finish (proxy,
                                     res,
                                     &error);
  g_assert_no_error (error);
  g_assert (result != NULL);
  xvariant_unref (result);

  xmain_loop_quit (loop);
}

/* Dynamically create @object_name under /foo/dyna */
static void
dyna_create (xdbus_connection_t *c,
             const xchar_t     *object_name)
{
  xerror_t *error;
  xdbus_proxy_t *proxy;
  xchar_t *object_path;

  object_path = xstrconcat ("/foo/dyna/", object_name, NULL);

  error = NULL;
  proxy = g_dbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                 G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                 NULL,
                                 g_dbus_connection_get_unique_name (c),
                                 object_path,
                                 "org.example.Dyna",
                                 NULL,
                                 &error);
  g_assert_no_error (error);
  g_assert (proxy != NULL);

  /* do this async to avoid libdbus-1 deadlocks */
  g_dbus_proxy_call (proxy,
                     "DynaCyber",
                     xvariant_new ("()"),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (xasync_ready_callback_t) dyna_create_callback,
                     NULL);
  xmain_loop_run (loop);

  g_assert_no_error (error);

  xobject_unref (proxy);
  g_free (object_path);

  return;
}

typedef struct
{
  xuint_t num_unregistered_calls;
  xuint_t num_unregistered_subtree_calls;
  xuint_t num_subtree_nodes;
} ObjectRegistrationData;

static void
on_object_unregistered (xpointer_t user_data)
{
  ObjectRegistrationData *data = user_data;

  data->num_unregistered_calls++;
}

static void
on_subtree_unregistered (xpointer_t user_data)
{
  ObjectRegistrationData *data = user_data;

  data->num_unregistered_subtree_calls++;
}

static xboolean_t
_xstrv_has_string (const xchar_t* const * haystack,
                    const xchar_t *needle)
{
  xuint_t n;

  for (n = 0; haystack != NULL && haystack[n] != NULL; n++)
    {
      if (xstrcmp0 (haystack[n], needle) == 0)
        return TRUE;
    }
  return FALSE;
}

/* -------------------- */

static xchar_t **
subtree_enumerate (xdbus_connection_t       *connection,
                   const xchar_t           *sender,
                   const xchar_t           *object_path,
                   xpointer_t               user_data)
{
  ObjectRegistrationData *data = user_data;
  xptr_array_t *p;
  xchar_t **nodes;
  xuint_t n;

  p = xptr_array_new ();

  for (n = 0; n < data->num_subtree_nodes; n++)
    {
      xptr_array_add (p, xstrdup_printf ("vp%d", n));
      xptr_array_add (p, xstrdup_printf ("evp%d", n));
    }
  xptr_array_add (p, NULL);

  nodes = (xchar_t **) xptr_array_free (p, FALSE);

  return nodes;
}

/* Only allows certain objects, and aborts on unknowns */
static xdbus_interface_info_t **
subtree_introspect (xdbus_connection_t       *connection,
                    const xchar_t           *sender,
                    const xchar_t           *object_path,
                    const xchar_t           *node,
                    xpointer_t               user_data)
{
  const xdbus_interface_info_t *interfaces[2] = {
     NULL /* filled in below */, NULL
  };

  /* VPs implement the foo_t interface, EVPs implement the Bar interface. The root
   * does not implement any interfaces
   */
  if (node == NULL)
    {
      return NULL;
    }
  else if (xstr_has_prefix (node, "vp"))
    {
      interfaces[0] = &foo_interface_info;
    }
  else if (xstr_has_prefix (node, "evp"))
    {
      interfaces[0] = &bar_interface_info;
    }
  else
    {
      g_assert_not_reached ();
    }

  return g_memdup2 (interfaces, 2 * sizeof (void *));
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
  if (xstrcmp0 (interface_name, "org.example.foo_t") == 0)
    return &foo_vtable;
  else
    return NULL;
}

static const xdbus_subtree_vtable_t subtree_vtable =
{
  subtree_enumerate,
  subtree_introspect,
  subtree_dispatch,
  { 0 }
};

/* -------------------- */

static xchar_t **
dynamic_subtree_enumerate (xdbus_connection_t       *connection,
                           const xchar_t           *sender,
                           const xchar_t           *object_path,
                           xpointer_t               user_data)
{
  xptr_array_t *data = user_data;
  xchar_t **nodes = g_new (xchar_t*, data->len + 1);
  xuint_t n;

  for (n = 0; n < data->len; n++)
    {
      nodes[n] = xstrdup (xptr_array_index (data, n));
    }
  nodes[data->len] = NULL;

  return nodes;
}

/* Allow all objects to be introspected */
static xdbus_interface_info_t **
dynamic_subtree_introspect (xdbus_connection_t       *connection,
                            const xchar_t           *sender,
                            const xchar_t           *object_path,
                            const xchar_t           *node,
                            xpointer_t               user_data)
{
  const xdbus_interface_info_t *interfaces[2] = { &dyna_interface_info, NULL };

  return g_memdup2 (interfaces, 2 * sizeof (void *));
}

static const xdbus_interface_vtable_t *
dynamic_subtree_dispatch (xdbus_connection_t             *connection,
                          const xchar_t                 *sender,
                          const xchar_t                 *object_path,
                          const xchar_t                 *interface_name,
                          const xchar_t                 *node,
                          xpointer_t                    *out_user_data,
                          xpointer_t                     user_data)
{
  *out_user_data = user_data;
  return &dyna_interface_vtable;
}

static const xdbus_subtree_vtable_t dynamic_subtree_vtable =
{
  dynamic_subtree_enumerate,
  dynamic_subtree_introspect,
  dynamic_subtree_dispatch,
  { 0 }
};

/* -------------------- */

typedef struct
{
  const xchar_t *object_path;
  xboolean_t check_remote_errors;
} TestDispatchThreadFuncArgs;

static xpointer_t
test_dispatch_thread_func (xpointer_t user_data)
{
  TestDispatchThreadFuncArgs *args = user_data;
  const xchar_t *object_path = args->object_path;
  xdbus_proxy_t *foo_proxy;
  xvariant_t *value;
  xvariant_t *inner;
  xerror_t *error;
  xchar_t *s;
  const xchar_t *value_str;

  foo_proxy = g_dbus_proxy_new_sync (c,
                                     G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
                                     G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                     NULL,
                                     g_dbus_connection_get_unique_name (c),
                                     object_path,
                                     "org.example.foo_t",
                                     NULL,
                                     &error);
  g_assert (foo_proxy != NULL);

  /* generic interfaces */
  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Peer.Ping",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_no_error (error);
  g_assert (value != NULL);
  xvariant_unref (value);

  /* user methods */
  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "Method1",
                                  xvariant_new ("(s)", "winwinwin"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_no_error (error);
  g_assert (value != NULL);
  g_assert (xvariant_is_of_type (value, G_VARIANT_TYPE ("(s)")));
  xvariant_get (value, "(&s)", &value_str);
  g_assert_cmpstr (value_str, ==, "You passed the string 'winwinwin'. Jolly good!");
  xvariant_unref (value);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "Method2",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_DBUS_ERROR);
  g_assert_cmpstr (error->message, ==, "GDBus.Error:org.example.SomeError: How do you like them apples, buddy!");
  xerror_free (error);
  g_assert (value == NULL);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "Method2",
                                  xvariant_new ("(s)", "failfailfail"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS);
  g_assert_cmpstr (error->message, ==, "GDBus.Error:org.freedesktop.DBus.Error.InvalidArgs: Type of message, “(s)”, does not match expected type “()”");
  xerror_free (error);
  g_assert (value == NULL);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "NonExistantMethod",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
  g_assert_cmpstr (error->message, ==, "GDBus.Error:org.freedesktop.DBus.Error.UnknownMethod: No such method “NonExistantMethod”");
  xerror_free (error);
  g_assert (value == NULL);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.example.FooXYZ.NonExistant",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
  xerror_free (error);
  g_assert (value == NULL);

  /* user properties */
  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Properties.Get",
                                  xvariant_new ("(ss)",
                                                 "org.example.foo_t",
                                                 "PropertyUno"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_no_error (error);
  g_assert (value != NULL);
  g_assert (xvariant_is_of_type (value, G_VARIANT_TYPE ("(v)")));
  xvariant_get (value, "(v)", &inner);
  g_assert (xvariant_is_of_type (inner, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr (xvariant_get_string (inner, NULL), ==, "Property 'PropertyUno' Is What It Is!");
  xvariant_unref (value);
  xvariant_unref (inner);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Properties.Get",
                                  xvariant_new ("(ss)",
                                                 "org.example.foo_t",
                                                 "ThisDoesntExist"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert (value == NULL);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS);
  g_assert_cmpstr (error->message, ==, "GDBus.Error:org.freedesktop.DBus.Error.InvalidArgs: No such property “ThisDoesntExist”");
  xerror_free (error);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Properties.Get",
                                  xvariant_new ("(ss)",
                                                 "org.example.foo_t",
                                                 "NotReadable"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert (value == NULL);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS);
  g_assert_cmpstr (error->message, ==, "GDBus.Error:org.freedesktop.DBus.Error.InvalidArgs: Property “NotReadable” is not readable");
  xerror_free (error);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Properties.Set",
                                  xvariant_new ("(ssv)",
                                                 "org.example.foo_t",
                                                 "NotReadable",
                                                 xvariant_new_string ("But Writable you are!")),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert (value == NULL);
  if (args->check_remote_errors)
    {
      /* _with_closures variant doesn't support customizing error data. */
      g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_SPAWN_FILE_INVALID);
      g_assert_cmpstr (error->message, ==, "GDBus.Error:org.freedesktop.DBus.Error.Spawn.FileInvalid: Returning some error instead of writing the value ''But Writable you are!'' to the property 'NotReadable'");
    }
  g_assert (error != NULL && error->domain == G_DBUS_ERROR);
  xerror_free (error);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Properties.Set",
                                  xvariant_new ("(ssv)",
                                                 "org.example.foo_t",
                                                 "NotWritable",
                                                 xvariant_new_uint32 (42)),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert (value == NULL);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS);
  g_assert_cmpstr (error->message, ==, "GDBus.Error:org.freedesktop.DBus.Error.InvalidArgs: Property “NotWritable” is not writable");
  xerror_free (error);

  error = NULL;
  value = g_dbus_proxy_call_sync (foo_proxy,
                                  "org.freedesktop.DBus.Properties.GetAll",
                                  xvariant_new ("(s)",
                                                 "org.example.foo_t"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);
  g_assert_no_error (error);
  g_assert (value != NULL);
  g_assert (xvariant_is_of_type (value, G_VARIANT_TYPE ("(a{sv})")));
  s = xvariant_print (value, TRUE);
  g_assert_cmpstr (s, ==, "({'PropertyUno': <\"Property 'PropertyUno' Is What It Is!\">, 'NotWritable': <\"Property 'NotWritable' Is What It Is!\">},)");
  g_free (s);
  xvariant_unref (value);

  xobject_unref (foo_proxy);

  xmain_loop_quit (loop);
  return NULL;
}

static void
test_dispatch (const xchar_t *object_path,
               xboolean_t     check_remote_errors)
{
  xthread_t *thread;

  TestDispatchThreadFuncArgs args = {object_path, check_remote_errors};

  /* run this in a thread to avoid deadlocks */
  thread = xthread_new ("test_dispatch",
                         test_dispatch_thread_func,
                         (xpointer_t) &args);
  xmain_loop_run (loop);
  xthread_join (thread);
}

static void
test_object_registration (void)
{
  xerror_t *error;
  ObjectRegistrationData data;
  xptr_array_t *dyna_data;
  xchar_t **nodes;
  xuint_t boss_foo_reg_id;
  xuint_t boss_bar_reg_id;
  xuint_t worker1_foo_reg_id;
  xuint_t worker1p1_foo_reg_id;
  xuint_t worker2_bar_reg_id;
  xuint_t intern1_foo_reg_id;
  xuint_t intern2_bar_reg_id;
  xuint_t intern2_foo_reg_id;
  xuint_t intern3_bar_reg_id;
  xuint_t registration_id;
  xuint_t subtree_registration_id;
  xuint_t non_subtree_object_path_foo_reg_id;
  xuint_t non_subtree_object_path_bar_reg_id;
  xuint_t dyna_subtree_registration_id;
  xuint_t num_successful_registrations;

  data.num_unregistered_calls = 0;
  data.num_unregistered_subtree_calls = 0;
  data.num_subtree_nodes = 0;

  num_successful_registrations = 0;

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (c != NULL);

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       &foo_vtable,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  boss_foo_reg_id = registration_id;
  num_successful_registrations++;

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss",
                                                       (xdbus_interface_info_t *) &bar_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  boss_bar_reg_id = registration_id;
  num_successful_registrations++;

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/worker1",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  worker1_foo_reg_id = registration_id;
  num_successful_registrations++;

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/worker1p1",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  worker1p1_foo_reg_id = registration_id;
  num_successful_registrations++;

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/worker2",
                                                       (xdbus_interface_info_t *) &bar_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  worker2_bar_reg_id = registration_id;
  num_successful_registrations++;

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/interns/intern1",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  intern1_foo_reg_id = registration_id;
  num_successful_registrations++;

  /* ... and try again at another path */
  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/interns/intern2",
                                                       (xdbus_interface_info_t *) &bar_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  intern2_bar_reg_id = registration_id;
  num_successful_registrations++;

  /* register at the same path/interface - this should fail */
  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/interns/intern2",
                                                       (xdbus_interface_info_t *) &bar_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
  g_assert (!g_dbus_error_is_remote_error (error));
  xerror_free (error);
  error = NULL;
  g_assert (registration_id == 0);

  /* register at different interface - shouldn't fail */
  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/interns/intern2",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  intern2_foo_reg_id = registration_id;
  num_successful_registrations++;

  /* unregister it via the id */
  g_assert (g_dbus_connection_unregister_object (c, registration_id));
  xmain_context_iteration (NULL, FALSE);
  g_assert_cmpint (data.num_unregistered_calls, ==, 1);
  intern2_foo_reg_id = 0;

  /* register it back */
  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/interns/intern2",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  intern2_foo_reg_id = registration_id;
  num_successful_registrations++;

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/interns/intern3",
                                                       (xdbus_interface_info_t *) &bar_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  intern3_bar_reg_id = registration_id;
  num_successful_registrations++;

  /* now register a whole subtree at /foo/boss/executives */
  subtree_registration_id = g_dbus_connection_register_subtree (c,
                                                                "/foo/boss/executives",
                                                                &subtree_vtable,
                                                                G_DBUS_SUBTREE_FLAGS_NONE,
                                                                &data,
                                                                on_subtree_unregistered,
                                                                &error);
  g_assert_no_error (error);
  g_assert (subtree_registration_id > 0);
  /* try registering it again.. this should fail */
  registration_id = g_dbus_connection_register_subtree (c,
                                                        "/foo/boss/executives",
                                                        &subtree_vtable,
                                                        G_DBUS_SUBTREE_FLAGS_NONE,
                                                        &data,
                                                        on_subtree_unregistered,
                                                        &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
  g_assert (!g_dbus_error_is_remote_error (error));
  xerror_free (error);
  error = NULL;
  g_assert (registration_id == 0);

  /* unregister it, then register it again */
  g_assert_cmpint (data.num_unregistered_subtree_calls, ==, 0);
  g_assert (g_dbus_connection_unregister_subtree (c, subtree_registration_id));
  xmain_context_iteration (NULL, FALSE);
  g_assert_cmpint (data.num_unregistered_subtree_calls, ==, 1);
  subtree_registration_id = g_dbus_connection_register_subtree (c,
                                                                "/foo/boss/executives",
                                                                &subtree_vtable,
                                                                G_DBUS_SUBTREE_FLAGS_NONE,
                                                                &data,
                                                                on_subtree_unregistered,
                                                                &error);
  g_assert_no_error (error);
  g_assert (subtree_registration_id > 0);

  /* try to register something under /foo/boss/executives - this should work
   * because registered subtrees and registered objects can coexist.
   *
   * Make the exported object implement *two* interfaces so we can check
   * that the right introspection handler is invoked.
   */
  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/executives/non_subtree_object",
                                                       (xdbus_interface_info_t *) &bar_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  non_subtree_object_path_bar_reg_id = registration_id;
  num_successful_registrations++;
  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo/boss/executives/non_subtree_object",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       NULL,
                                                       &data,
                                                       on_object_unregistered,
                                                       &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);
  non_subtree_object_path_foo_reg_id = registration_id;
  num_successful_registrations++;

  /* now register a dynamic subtree, spawning objects as they are called */
  dyna_data = xptr_array_new_with_free_func (g_free);
  dyna_subtree_registration_id = g_dbus_connection_register_subtree (c,
                                                                     "/foo/dyna",
                                                                     &dynamic_subtree_vtable,
                                                                     G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES,
                                                                     dyna_data,
                                                                     (xdestroy_notify_t)xptr_array_unref,
                                                                     &error);
  g_assert_no_error (error);
  g_assert (dyna_subtree_registration_id > 0);

  /* First assert that we have no nodes in the dynamic subtree */
  nodes = get_nodes_at (c, "/foo/dyna");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 0);
  xstrfreev (nodes);
  g_assert_cmpint (count_interfaces (c, "/foo/dyna"), ==, 4);

  /* Install three nodes in the dynamic subtree via the dyna_data backdoor and
   * assert that they show up correctly in the introspection data */
  xptr_array_add (dyna_data, xstrdup ("lol"));
  xptr_array_add (dyna_data, xstrdup ("cat"));
  xptr_array_add (dyna_data, xstrdup ("cheezburger"));
  nodes = get_nodes_at (c, "/foo/dyna");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 3);
  g_assert_cmpstr (nodes[0], ==, "cat");
  g_assert_cmpstr (nodes[1], ==, "cheezburger");
  g_assert_cmpstr (nodes[2], ==, "lol");
  xstrfreev (nodes);
  g_assert_cmpint (count_interfaces (c, "/foo/dyna/lol"), ==, 4);
  g_assert_cmpint (count_interfaces (c, "/foo/dyna/cat"), ==, 4);
  g_assert_cmpint (count_interfaces (c, "/foo/dyna/cheezburger"), ==, 4);

  /* Call a non-existing object path and assert that it has been created */
  dyna_create (c, "dynamicallycreated");
  nodes = get_nodes_at (c, "/foo/dyna");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 4);
  g_assert_cmpstr (nodes[0], ==, "cat");
  g_assert_cmpstr (nodes[1], ==, "cheezburger");
  g_assert_cmpstr (nodes[2], ==, "dynamicallycreated");
  g_assert_cmpstr (nodes[3], ==, "lol");
  xstrfreev (nodes);
  g_assert_cmpint (count_interfaces (c, "/foo/dyna/dynamicallycreated"), ==, 4);

  /* now check that the object hierarchy is properly generated... yes, it's a bit
   * perverse that we round-trip to the bus to introspect ourselves ;-)
   */
  nodes = get_nodes_at (c, "/");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 1);
  g_assert_cmpstr (nodes[0], ==, "foo");
  xstrfreev (nodes);
  g_assert_cmpint (count_interfaces (c, "/"), ==, 0);

  nodes = get_nodes_at (c, "/foo");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 2);
  g_assert_cmpstr (nodes[0], ==, "boss");
  g_assert_cmpstr (nodes[1], ==, "dyna");
  xstrfreev (nodes);
  g_assert_cmpint (count_interfaces (c, "/foo"), ==, 0);

  nodes = get_nodes_at (c, "/foo/boss");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 5);
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "worker1"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "worker1p1"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "worker2"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "interns"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "executives"));
  xstrfreev (nodes);
  /* any registered object always implement org.freedesktop.DBus.[Peer,Introspectable,Properties] */
  g_assert_cmpint (count_interfaces (c, "/foo/boss"), ==, 5);
  g_assert (has_interface (c, "/foo/boss", foo_interface_info.name));
  g_assert (has_interface (c, "/foo/boss", bar_interface_info.name));

  /* check subtree nodes - we should have only non_subtree_object in /foo/boss/executives
   * because data.num_subtree_nodes is 0
   */
  nodes = get_nodes_at (c, "/foo/boss/executives");
  g_assert (nodes != NULL);
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "non_subtree_object"));
  g_assert_cmpint (xstrv_length (nodes), ==, 1);
  xstrfreev (nodes);
  g_assert_cmpint (count_interfaces (c, "/foo/boss/executives"), ==, 0);

  /* now change data.num_subtree_nodes and check */
  data.num_subtree_nodes = 2;
  nodes = get_nodes_at (c, "/foo/boss/executives");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 5);
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "non_subtree_object"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "vp0"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "vp1"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "evp0"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "evp1"));
  /* check that /foo/boss/executives/non_subtree_object is not handled by the
   * subtree handlers - we can do this because objects from subtree handlers
   * has exactly one interface and non_subtree_object has two
   */
  g_assert_cmpint (count_interfaces (c, "/foo/boss/executives/non_subtree_object"), ==, 5);
  g_assert (has_interface (c, "/foo/boss/executives/non_subtree_object", foo_interface_info.name));
  g_assert (has_interface (c, "/foo/boss/executives/non_subtree_object", bar_interface_info.name));
  /* check that the vp and evp objects are handled by the subtree handlers */
  g_assert_cmpint (count_interfaces (c, "/foo/boss/executives/vp0"), ==, 4);
  g_assert_cmpint (count_interfaces (c, "/foo/boss/executives/vp1"), ==, 4);
  g_assert_cmpint (count_interfaces (c, "/foo/boss/executives/evp0"), ==, 4);
  g_assert_cmpint (count_interfaces (c, "/foo/boss/executives/evp1"), ==, 4);
  g_assert (has_interface (c, "/foo/boss/executives/vp0", foo_interface_info.name));
  g_assert (has_interface (c, "/foo/boss/executives/vp1", foo_interface_info.name));
  g_assert (has_interface (c, "/foo/boss/executives/evp0", bar_interface_info.name));
  g_assert (has_interface (c, "/foo/boss/executives/evp1", bar_interface_info.name));
  xstrfreev (nodes);
  data.num_subtree_nodes = 3;
  nodes = get_nodes_at (c, "/foo/boss/executives");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 7);
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "non_subtree_object"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "vp0"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "vp1"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "vp2"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "evp0"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "evp1"));
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "evp2"));
  xstrfreev (nodes);

  /* This is to check that a bug (rather, class of bugs) in gdbusconnection.c's
   *
   *  g_dbus_connection_list_registered_unlocked()
   *
   * where /foo/boss/worker1 reported a child '1', is now fixed.
   */
  nodes = get_nodes_at (c, "/foo/boss/worker1");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 0);
  xstrfreev (nodes);

  /* check that calls are properly dispatched to the functions in foo_vtable for objects
   * implementing the org.example.foo_t interface
   *
   * We do this for both a regular registered object (/foo/boss) and also for an object
   * registered through the subtree mechanism.
   */
  test_dispatch ("/foo/boss", TRUE);
  test_dispatch ("/foo/boss/executives/vp0", TRUE);

  /* To prevent from exiting and attaching a D-Bus tool like D-Feet; uncomment: */
#if 0
  g_debug ("Point D-feet or other tool at: %s", g_test_dbus_get_temporary_address());
  xmain_loop_run (loop);
#endif

  /* check that unregistering the subtree handler works */
  g_assert_cmpint (data.num_unregistered_subtree_calls, ==, 1);
  g_assert (g_dbus_connection_unregister_subtree (c, subtree_registration_id));
  xmain_context_iteration (NULL, FALSE);
  g_assert_cmpint (data.num_unregistered_subtree_calls, ==, 2);
  nodes = get_nodes_at (c, "/foo/boss/executives");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 1);
  g_assert (_xstrv_has_string ((const xchar_t* const *) nodes, "non_subtree_object"));
  xstrfreev (nodes);

  g_assert (g_dbus_connection_unregister_object (c, boss_foo_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, boss_bar_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, worker1_foo_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, worker1p1_foo_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, worker2_bar_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, intern1_foo_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, intern2_bar_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, intern2_foo_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, intern3_bar_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, non_subtree_object_path_bar_reg_id));
  g_assert (g_dbus_connection_unregister_object (c, non_subtree_object_path_foo_reg_id));

  xmain_context_iteration (NULL, FALSE);
  g_assert_cmpint (data.num_unregistered_calls, ==, num_successful_registrations);

  /* check that we no longer export any objects - TODO: it looks like there's a bug in
   * libdbus-1 here: libdbus still reports the '/foo' object; so disable the test for now
   */
#if 0
  nodes = get_nodes_at (c, "/");
  g_assert (nodes != NULL);
  g_assert_cmpint (xstrv_length (nodes), ==, 0);
  xstrfreev (nodes);
#endif

  xobject_unref (c);
}

static void
test_object_registration_with_closures (void)
{
  xerror_t *error;
  xuint_t registration_id;

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (c != NULL);

  registration_id = g_dbus_connection_register_object_with_closures (c,
                                                                     "/foo/boss",
                                                                     (xdbus_interface_info_t *) &foo_interface_info,
                                                                     g_cclosure_new (G_CALLBACK (foo_method_call), NULL, NULL),
                                                                     g_cclosure_new (G_CALLBACK (foo_get_property), NULL, NULL),
                                                                     g_cclosure_new (G_CALLBACK (foo_set_property), NULL, NULL),
                                                                     &error);
  g_assert_no_error (error);
  g_assert (registration_id > 0);

  test_dispatch ("/foo/boss", FALSE);

  g_assert (g_dbus_connection_unregister_object (c, registration_id));

  xobject_unref (c);
}

static const xdbus_interface_info_t test_interface_info1 =
{
  -1,
  "org.example.foo_t",
  (xdbus_method_info_t **) NULL,
  (xdbus_signalInfo_t **) NULL,
  (xdbus_property_info_t **) NULL,
  NULL,
};

static const xdbus_interface_info_t test_interface_info2 =
{
  -1,
  "org.freedesktop.DBus.Properties",
  (xdbus_method_info_t **) NULL,
  (xdbus_signalInfo_t **) NULL,
  (xdbus_property_info_t **) NULL,
  NULL,
};

static void
check_interfaces (xdbus_connection_t  *c,
                  const xchar_t      *object_path,
                  const xchar_t     **interfaces)
{
  xerror_t *error;
  xdbus_proxy_t *proxy;
  xchar_t *xml_data;
  xdbus_node_info_t *node_info;
  xint_t i, j;

  error = NULL;
  proxy = g_dbus_proxy_new_sync (c,
                                 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                 G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                 NULL,
                                 g_dbus_connection_get_unique_name (c),
                                 object_path,
                                 "org.freedesktop.DBus.Introspectable",
                                 NULL,
                                 &error);
  g_assert_no_error (error);
  g_assert (proxy != NULL);

  /* do this async to avoid libdbus-1 deadlocks */
  xml_data = NULL;
  g_dbus_proxy_call (proxy,
                     "Introspect",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (xasync_ready_callback_t) introspect_callback,
                     &xml_data);
  xmain_loop_run (loop);
  g_assert (xml_data != NULL);

  node_info = g_dbus_node_info_new_for_xml (xml_data, &error);
  g_assert_no_error (error);
  g_assert (node_info != NULL);

  g_assert (node_info->interfaces != NULL);
  for (i = 0; node_info->interfaces[i]; i++) ;
#if 0
  if (xstrv_length ((xchar_t**)interfaces) != i - 1)
    {
      g_printerr ("expected ");
      for (i = 0; interfaces[i]; i++)
        g_printerr ("%s ", interfaces[i]);
      g_printerr ("\ngot ");
      for (i = 0; node_info->interfaces[i]; i++)
        g_printerr ("%s ", node_info->interfaces[i]->name);
      g_printerr ("\n");
    }
#endif
  g_assert_cmpint (xstrv_length ((xchar_t**)interfaces), ==, i - 1);

  for (i = 0; interfaces[i]; i++)
    {
      for (j = 0; node_info->interfaces[j]; j++)
        {
          if (strcmp (interfaces[i], node_info->interfaces[j]->name) == 0)
            goto found;
        }

      g_assert_not_reached ();

 found: ;
    }

  xobject_unref (proxy);
  g_free (xml_data);
  g_dbus_node_info_unref (node_info);
}

static void
test_registered_interfaces (void)
{
  xerror_t *error;
  xuint_t id1, id2;
  const xchar_t *interfaces[] = {
    "org.example.foo_t",
    "org.freedesktop.DBus.Properties",
    "org.freedesktop.DBus.Introspectable",
    NULL,
  };

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (c != NULL);

  id1 = g_dbus_connection_register_object (c,
                                           "/test",
                                           (xdbus_interface_info_t *) &test_interface_info1,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &error);
  g_assert_no_error (error);
  g_assert (id1 > 0);
  id2 = g_dbus_connection_register_object (c,
                                           "/test",
                                           (xdbus_interface_info_t *) &test_interface_info2,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &error);
  g_assert_no_error (error);
  g_assert (id2 > 0);

  check_interfaces (c, "/test", interfaces);

  g_assert (g_dbus_connection_unregister_object (c, id1));
  g_assert (g_dbus_connection_unregister_object (c, id2));
  xobject_unref (c);
}


/* ---------------------------------------------------------------------------------------------------- */

static void
test_async_method_call (xdbus_connection_t       *connection,
                        const xchar_t           *sender,
                        const xchar_t           *object_path,
                        const xchar_t           *interface_name,
                        const xchar_t           *method_name,
                        xvariant_t              *parameters,
                        xdbus_method_invocation_t *invocation,
                        xpointer_t               user_data)
{
  const xdbus_property_info_t *property;

  /* Strictly speaking, this function should also expect to receive
   * method calls not on the org.freedesktop.DBus.Properties interface,
   * but we don't do any during this testcase, so assert that.
   */
  g_assert_cmpstr (interface_name, ==, "org.freedesktop.DBus.Properties");
  g_assert (xdbus_method_invocation_get_method_info (invocation) == NULL);

  property = xdbus_method_invocation_get_property_info (invocation);

  /* We should never be seeing any property calls on the com.example.Bar
   * interface because it doesn't export any properties.
   *
   * In each case below make sure the interface is org.example.foo_t.
   */

  /* Do a whole lot of asserts to make sure that invalid calls are still
   * getting properly rejected by xdbus_connection_t and that our
   * environment is as we expect it to be.
   */
  if (xstr_equal (method_name, "Get"))
    {
      const xchar_t *iface_name, *prop_name;

      xvariant_get (parameters, "(&s&s)", &iface_name, &prop_name);
      g_assert_cmpstr (iface_name, ==, "org.example.foo_t");
      g_assert (property != NULL);
      g_assert_cmpstr (prop_name, ==, property->name);
      g_assert (property->flags & G_DBUS_PROPERTY_INFO_FLAGS_READABLE);
      xdbus_method_invocation_return_value (invocation, xvariant_new ("(v)", xvariant_new_string (prop_name)));
    }

  else if (xstr_equal (method_name, "Set"))
    {
      const xchar_t *iface_name, *prop_name;
      xvariant_t *value;

      xvariant_get (parameters, "(&s&sv)", &iface_name, &prop_name, &value);
      g_assert_cmpstr (iface_name, ==, "org.example.foo_t");
      g_assert (property != NULL);
      g_assert_cmpstr (prop_name, ==, property->name);
      g_assert (property->flags & G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE);
      g_assert (xvariant_is_of_type (value, G_VARIANT_TYPE (property->signature)));
      xdbus_method_invocation_return_value (invocation, xvariant_new ("()"));
      xvariant_unref (value);
    }

  else if (xstr_equal (method_name, "GetAll"))
    {
      const xchar_t *iface_name;

      xvariant_get (parameters, "(&s)", &iface_name);
      g_assert_cmpstr (iface_name, ==, "org.example.foo_t");
      g_assert (property == NULL);
      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new_parsed ("({ 'PropertyUno': < 'uno' >,"
                                                                   "   'NotWritable': < 'notwrite' > },)"));
    }

  else
    g_assert_not_reached ();
}

static xint_t outstanding_cases;

static void
ensure_result_cb (xobject_t      *source,
                  xasync_result_t *result,
                  xpointer_t      user_data)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (source);
  xvariant_t *reply;

  reply = g_dbus_connection_call_finish (connection, result, NULL);

  if (user_data == NULL)
    {
      /* Expected an error */
      g_assert (reply == NULL);
    }
  else
    {
      /* Expected a reply of a particular format. */
      xchar_t *str;

      g_assert (reply != NULL);
      str = xvariant_print (reply, TRUE);
      g_assert_cmpstr (str, ==, (const xchar_t *) user_data);
      g_free (str);

      xvariant_unref (reply);
    }

  g_assert_cmpint (outstanding_cases, >, 0);
  outstanding_cases--;
}

static void
test_async_case (xdbus_connection_t *connection,
                 const xchar_t     *expected_reply,
                 const xchar_t     *method,
                 const xchar_t     *format_string,
                 ...)
{
  va_list ap;

  va_start (ap, format_string);

  g_dbus_connection_call (connection, g_dbus_connection_get_unique_name (connection), "/foo",
                          "org.freedesktop.DBus.Properties", method, xvariant_new_va (format_string, NULL, &ap),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, ensure_result_cb, (xpointer_t) expected_reply);

  va_end (ap);

  outstanding_cases++;
}

static void
test_async_properties (void)
{
  xerror_t *error = NULL;
  xuint_t registration_id, registration_id2;
  static const xdbus_interface_vtable_t vtable = {
    test_async_method_call, NULL, NULL, { 0 }
  };

  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  g_assert (c != NULL);

  registration_id = g_dbus_connection_register_object (c,
                                                       "/foo",
                                                       (xdbus_interface_info_t *) &foo_interface_info,
                                                       &vtable, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert (registration_id);
  registration_id2 = g_dbus_connection_register_object (c,
                                                        "/foo",
                                                        (xdbus_interface_info_t *) &foo2_interface_info,
                                                        &vtable, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert (registration_id);

  test_async_case (c, NULL, "random", "()");

  /* Test a variety of error cases */
  test_async_case (c, NULL, "Get", "(si)", "wrong signature", 5);
  test_async_case (c, NULL, "Get", "(ss)", "org.example.WrongInterface", "zzz");
  test_async_case (c, NULL, "Get", "(ss)", "org.example.foo_t", "NoSuchProperty");
  test_async_case (c, NULL, "Get", "(ss)", "org.example.foo_t", "NotReadable");

  test_async_case (c, NULL, "Set", "(si)", "wrong signature", 5);
  test_async_case (c, NULL, "Set", "(ssv)", "org.example.WrongInterface", "zzz", xvariant_new_string (""));
  test_async_case (c, NULL, "Set", "(ssv)", "org.example.foo_t", "NoSuchProperty", xvariant_new_string (""));
  test_async_case (c, NULL, "Set", "(ssv)", "org.example.foo_t", "NotWritable", xvariant_new_string (""));
  test_async_case (c, NULL, "Set", "(ssv)", "org.example.foo_t", "PropertyUno", xvariant_new_object_path ("/wrong"));

  test_async_case (c, NULL, "GetAll", "(si)", "wrong signature", 5);
  test_async_case (c, NULL, "GetAll", "(s)", "org.example.WrongInterface");

  /* Make sure that we get no unexpected async property calls for com.example.Foo2 */
  test_async_case (c, NULL, "Get", "(ss)", "org.example.Foo2", "zzz");
  test_async_case (c, NULL, "Set", "(ssv)", "org.example.Foo2", "zzz", xvariant_new_string (""));
  test_async_case (c, "(@a{sv} {},)", "GetAll", "(s)", "org.example.Foo2");

  /* Now do the proper things */
  test_async_case (c, "(<'PropertyUno'>,)", "Get", "(ss)", "org.example.foo_t", "PropertyUno");
  test_async_case (c, "(<'NotWritable'>,)", "Get", "(ss)", "org.example.foo_t", "NotWritable");
  test_async_case (c, "()", "Set", "(ssv)", "org.example.foo_t", "PropertyUno", xvariant_new_string (""));
  test_async_case (c, "()", "Set", "(ssv)", "org.example.foo_t", "NotReadable", xvariant_new_string (""));
  test_async_case (c, "({'PropertyUno': <'uno'>, 'NotWritable': <'notwrite'>},)", "GetAll", "(s)", "org.example.foo_t");

  while (outstanding_cases)
    xmain_context_iteration (NULL, TRUE);

  g_dbus_connection_unregister_object (c, registration_id);
  g_dbus_connection_unregister_object (c, registration_id2);
  xobject_unref (c);
}

typedef struct
{
  xdbus_connection_t *connection;  /* (owned) */
  xuint_t registration_id;
  xuint_t subtree_registration_id;
} ThreadedUnregistrationData;

static xpointer_t
unregister_thread_cb (xpointer_t user_data)
{
  ThreadedUnregistrationData *data = user_data;

  /* Sleeping here makes the race more likely to be hit, as it balances the
   * time taken to set up the thread and unregister, with the time taken to
   * make and handle the D-Bus call. This will likely change with future kernel
   * versions, but there isn’t a more deterministic synchronisation point that
   * I can think of to use instead. */
  usleep (330);

  if (data->registration_id > 0)
    g_assert_true (g_dbus_connection_unregister_object (data->connection, data->registration_id));

  if (data->subtree_registration_id > 0)
    g_assert_true (g_dbus_connection_unregister_subtree (data->connection, data->subtree_registration_id));

  return NULL;
}

static void
async_result_cb (xobject_t      *source_object,
                 xasync_result_t *result,
                 xpointer_t      user_data)
{
  xasync_result_t **result_out = user_data;

  *result_out = xobject_ref (result);
  xmain_context_wakeup (NULL);
}

/* Returns %TRUE if this iteration resolved the race with the unregistration
 * first, %FALSE if the call handler was invoked first. */
static xboolean_t
test_threaded_unregistration_iteration (xboolean_t subtree)
{
  ThreadedUnregistrationData data = { NULL, 0, 0 };
  ObjectRegistrationData object_registration_data = { 0, 0, 2 };
  xerror_t *local_error = NULL;
  xthread_t *unregister_thread = NULL;
  const xchar_t *object_path;
  xvariant_t *value = NULL;
  const xchar_t *value_str;
  xasync_result_t *call_result = NULL;
  xboolean_t unregistration_was_first;

  data.connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (data.connection);

  /* Register an object or a subtree */
  if (!subtree)
    {
      data.registration_id = g_dbus_connection_register_object (data.connection,
                                                                "/foo/boss",
                                                                (xdbus_interface_info_t *) &foo_interface_info,
                                                                &foo_vtable,
                                                                &object_registration_data,
                                                                on_object_unregistered,
                                                                &local_error);
      g_assert_no_error (local_error);
      g_assert_cmpint (data.registration_id, >, 0);

      object_path = "/foo/boss";
    }
  else
    {
      data.subtree_registration_id = g_dbus_connection_register_subtree (data.connection,
                                                                         "/foo/boss/executives",
                                                                         &subtree_vtable,
                                                                         G_DBUS_SUBTREE_FLAGS_NONE,
                                                                         &object_registration_data,
                                                                         on_subtree_unregistered,
                                                                         &local_error);
      g_assert_no_error (local_error);
      g_assert_cmpint (data.subtree_registration_id, >, 0);

      object_path = "/foo/boss/executives/vp0";
    }

  /* Allow the registrations to go through. */
  xmain_context_iteration (NULL, FALSE);

  /* Spawn a thread to unregister the object/subtree. This will race with
   * the call we subsequently make. */
  unregister_thread = xthread_new ("unregister-object",
                                    unregister_thread_cb, &data);

  /* Call a method on the object (or an object in the subtree). The callback
   * will be invoked in this main context. */
  g_dbus_connection_call (data.connection,
                          g_dbus_connection_get_unique_name (data.connection),
                          object_path,
                          "org.example.foo_t",
                          "Method1",
                          xvariant_new ("(s)", "winwinwin"),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          async_result_cb,
                          &call_result);

  while (call_result == NULL)
    xmain_context_iteration (NULL, TRUE);

  value = g_dbus_connection_call_finish (data.connection, call_result, &local_error);

  /* The result of the method could either be an error (that the object doesn’t
   * exist) or a valid result, depending on how the thread was scheduled
   * relative to the call. */
  unregistration_was_first = (value == NULL);
  if (value != NULL)
    {
      g_assert_no_error (local_error);
      g_assert_true (xvariant_is_of_type (value, G_VARIANT_TYPE ("(s)")));
      xvariant_get (value, "(&s)", &value_str);
      g_assert_cmpstr (value_str, ==, "You passed the string 'winwinwin'. Jolly good!");
    }
  else
    {
      g_assert_null (value);
      g_assert_error (local_error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
    }

  g_clear_pointer (&value, xvariant_unref);
  g_clear_error (&local_error);

  /* Tidy up. */
  xthread_join (g_steal_pointer (&unregister_thread));

  g_clear_object (&call_result);
  g_clear_object (&data.connection);

  return unregistration_was_first;
}

static void
test_threaded_unregistration (xconstpointer test_data)
{
  xboolean_t subtree = GPOINTER_TO_INT (test_data);
  xuint_t i;
  xuint_t n_iterations_unregistration_first = 0;
  xuint_t n_iterations_call_first = 0;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2400");
  g_test_summary ("Test that object/subtree unregistration from one thread "
                  "doesn’t cause problems when racing with method callbacks "
                  "in another thread for that object or subtree");

  /* Run iterations of the test until it’s likely we’ve hit the race. Limit the
   * number of iterations so the test doesn’t run forever if not. The choice of
   * 100 is arbitrary. */
  for (i = 0; i < 1000 && (n_iterations_unregistration_first < 100 || n_iterations_call_first < 100); i++)
    {
      if (test_threaded_unregistration_iteration (subtree))
        n_iterations_unregistration_first++;
      else
        n_iterations_call_first++;
    }

  /* If the condition below is met, we probably failed to reproduce the race.
   * Don’t fail the test, though, as we can’t always control whether we hit the
   * race, and spurious test failures are annoying. */
  if (n_iterations_unregistration_first < 100 ||
      n_iterations_call_first < 100)
    g_test_skip_printf ("Failed to reproduce race (%u iterations with unregistration first, %u with call first); skipping test",
                        n_iterations_unregistration_first, n_iterations_call_first);
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

  g_test_add_func ("/gdbus/object-registration", test_object_registration);
  g_test_add_func ("/gdbus/object-registration-with-closures", test_object_registration_with_closures);
  g_test_add_func ("/gdbus/registered-interfaces", test_registered_interfaces);
  g_test_add_func ("/gdbus/async-properties", test_async_properties);
  g_test_add_data_func ("/gdbus/threaded-unregistration/object", GINT_TO_POINTER (FALSE), test_threaded_unregistration);
  g_test_add_data_func ("/gdbus/threaded-unregistration/subtree", GINT_TO_POINTER (TRUE), test_threaded_unregistration);

  /* TODO: check that we spit out correct introspection data */
  /* TODO: check that registering a whole subtree works */

  ret = session_bus_run ();

  xmain_loop_unref (loop);

  return ret;
}
