/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2018 Red Hat, Inc.
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
#include <stdio.h>

#include "glib/glib-private.h"

#include "gdbus-tests.h"

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
#include "gdbus-test-codegen-generated-min-required-2-64.h"
#else
#include "gdbus-test-codegen-generated.h"
#endif

#include "gdbus-test-codegen-generated-interface-info.h"

#if XPL_VERSION_MIN_REQUIRED < XPL_VERSION_2_68
# undef G_DBUS_METHOD_INVOCATION_HANDLED
# define G_DBUS_METHOD_INVOCATION_HANDLED TRUE
#endif

/* ---------------------------------------------------------------------------------------------------- */

static xuint_t
count_annotations (xdbus_annotation_info_t **annotations)
{
  xuint_t ret;
  ret = 0;
  while (annotations != NULL && annotations[ret] != NULL)
    ret++;
  return ret;
}

/* checks that
 *
 *  - non-internal annotations are written out correctly; and
 *  - injection via --annotation --key --value works
 */
static void
test_annotations (void)
{
  xdbus_interface_info_t *iface;
  xdbus_method_info_t *method;
  xdbus_signalInfo_t *signal;
  xdbus_property_info_t *property;

  iface = foo_igen_bar_interface_info ();
  xassert (iface != NULL);

  /* see meson.build for where these annotations are injected */
  g_assert_cmpint (count_annotations (iface->annotations), ==, 1);
  g_assert_cmpstr (g_dbus_annotation_info_lookup (iface->annotations, "Key1"), ==, "Value1");

  method = g_dbus_interface_info_lookup_method (iface, "HelloWorld");
  xassert (method != NULL);
  g_assert_cmpint (count_annotations (method->annotations), ==, 2);
  g_assert_cmpstr (g_dbus_annotation_info_lookup (method->annotations, "ExistingAnnotation"), ==, "blah");
  g_assert_cmpstr (g_dbus_annotation_info_lookup (method->annotations, "Key3"), ==, "Value3");

  signal = g_dbus_interface_info_lookup_signal (iface, "TestSignal");
  xassert (signal != NULL);
  g_assert_cmpint (count_annotations (signal->annotations), ==, 1);
  g_assert_cmpstr (g_dbus_annotation_info_lookup (signal->annotations, "Key4"), ==, "Value4");
  g_assert_cmpstr (g_dbus_annotation_info_lookup (signal->args[1]->annotations, "Key8"), ==, "Value8");

  property = g_dbus_interface_info_lookup_property (iface, "ay");
  xassert (property != NULL);
  g_assert_cmpint (count_annotations (property->annotations), ==, 1);
  g_assert_cmpstr (g_dbus_annotation_info_lookup (property->annotations, "Key5"), ==, "Value5");

  method = g_dbus_interface_info_lookup_method (iface, "TestPrimitiveTypes");
  xassert (method != NULL);
  g_assert_cmpstr (g_dbus_annotation_info_lookup (method->in_args[4]->annotations, "Key6"), ==, "Value6");
  g_assert_cmpstr (g_dbus_annotation_info_lookup (method->out_args[5]->annotations, "Key7"), ==, "Value7");
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
on_handle_hello_world (FooiGenBar             *object,
                       xdbus_method_invocation_t  *invocation,
                       const xchar_t            *greeting,
                       xpointer_t                user_data)
{
  xchar_t *response;
  response = xstrdup_printf ("Word! You said '%s'. I'm Skeleton, btw!", greeting);
  foo_igen_bar_complete_hello_world (object, invocation, response);
  g_free (response);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_test_primitive_types (FooiGenBar            *object,
                                xdbus_method_invocation_t *invocation,
                                xuchar_t                 val_byte,
                                xboolean_t               val_boolean,
                                gint16                 val_int16,
                                xuint16_t                val_uint16,
                                xint_t                   val_int32,
                                xuint_t                  val_uint32,
                                sint64_t                 val_int64,
                                xuint64_t                val_uint64,
                                xdouble_t                val_double,
                                const xchar_t           *val_string,
                                const xchar_t           *val_objpath,
                                const xchar_t           *val_signature,
                                const xchar_t           *val_bytestring,
                                xpointer_t               user_data)
{
  xchar_t *s1;
  xchar_t *s2;
  xchar_t *s3;
  s1 = xstrdup_printf ("Word! You said '%s'. Rock'n'roll!", val_string);
  s2 = xstrdup_printf ("/modified%s", val_objpath);
  s3 = xstrdup_printf ("assgit%s", val_signature);
  foo_igen_bar_complete_test_primitive_types (object,
                                              invocation,
                                              10 + val_byte,
                                              !val_boolean,
                                              100 + val_int16,
                                              1000 + val_uint16,
                                              10000 + val_int32,
                                              100000 + val_uint32,
                                              1000000 + val_int64,
                                              10000000 + val_uint64,
                                              val_double / G_PI,
                                              s1,
                                              s2,
                                              s3,
                                              "bytestring!\xff");
  g_free (s1);
  g_free (s2);
  g_free (s3);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_test_non_primitive_types (FooiGenBar            *object,
                                    xdbus_method_invocation_t *invocation,
                                    xvariant_t              *dict_s_to_s,
                                    xvariant_t              *dict_s_to_pairs,
                                    xvariant_t              *a_struct,
                                    const xchar_t* const    *array_of_strings,
                                    const xchar_t* const    *array_of_objpaths,
                                    xvariant_t              *array_of_signatures,
                                    const xchar_t* const    *array_of_bytestrings,
                                    xpointer_t               user_data)
{
  xchar_t *s;
  xstring_t *str;
  str = xstring_new (NULL);
  s = xvariant_print (dict_s_to_s, TRUE); xstring_append (str, s); g_free (s);
  s = xvariant_print (dict_s_to_pairs, TRUE); xstring_append (str, s); g_free (s);
  s = xvariant_print (a_struct, TRUE); xstring_append (str, s); g_free (s);
  s = xstrjoinv (", ", (xchar_t **) array_of_strings);
  xstring_append_printf (str, "array_of_strings: [%s] ", s);
  g_free (s);
  s = xstrjoinv (", ", (xchar_t **) array_of_objpaths);
  xstring_append_printf (str, "array_of_objpaths: [%s] ", s);
  g_free (s);
  s = xvariant_print (array_of_signatures, TRUE);
  xstring_append_printf (str, "array_of_signatures: %s ", s);
  g_free (s);
  s = xstrjoinv (", ", (xchar_t **) array_of_bytestrings);
  xstring_append_printf (str, "array_of_bytestrings: [%s] ", s);
  g_free (s);
  foo_igen_bar_complete_test_non_primitive_types (object, invocation,
                                                  array_of_strings,
                                                  array_of_objpaths,
                                                  array_of_signatures,
                                                  array_of_bytestrings,
                                                  str->str);
  xstring_free (str, TRUE);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_request_signal_emission (FooiGenBar             *object,
                                   xdbus_method_invocation_t  *invocation,
                                   xint_t                    which_one,
                                   xpointer_t                user_data)
{
  if (which_one == 0)
    {
      const xchar_t *a_strv[] = {"foo", "bar", NULL};
      const xchar_t *a_bytestring_array[] = {"foo\xff", "bar\xff", NULL};
      xvariant_t *a_variant = xvariant_new_parsed ("{'first': (42, 42), 'second': (43, 43)}");
      foo_igen_bar_emit_test_signal (object, 43, a_strv, a_bytestring_array, a_variant); /* consumes a_variant */
      foo_igen_bar_complete_request_signal_emission (object, invocation);
    }
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_request_multi_property_mods (FooiGenBar             *object,
                                       xdbus_method_invocation_t  *invocation,
                                       xpointer_t                user_data)
{
  foo_igen_bar_set_y (object, foo_igen_bar_get_y (object) + 1);
  foo_igen_bar_set_i (object, foo_igen_bar_get_i (object) + 1);
  foo_igen_bar_set_y (object, foo_igen_bar_get_y (object) + 1);
  foo_igen_bar_set_i (object, foo_igen_bar_get_i (object) + 1);
  g_dbus_interface_skeleton_flush (G_DBUS_INTERFACE_SKELETON (object));
  foo_igen_bar_set_y (object, foo_igen_bar_get_y (object) + 1);
  foo_igen_bar_set_i (object, foo_igen_bar_get_i (object) + 1);
  foo_igen_bar_complete_request_multi_property_mods (object, invocation);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_property_cancellation (FooiGenBar             *object,
                                 xdbus_method_invocation_t  *invocation,
                                 xpointer_t                user_data)
{
  xuint_t n;
  n = foo_igen_bar_get_n (object);
  /* This queues up a PropertiesChange event */
  foo_igen_bar_set_n (object, n + 1);
  /* this modifies the queued up event */
  foo_igen_bar_set_n (object, n);
  /* this flushes all PropertiesChanges event (sends the D-Bus message right
   * away, if any - there should not be any)
   */
  g_dbus_interface_skeleton_flush (G_DBUS_INTERFACE_SKELETON (object));
  /* this makes us return the reply D-Bus method */
  foo_igen_bar_complete_property_cancellation (object, invocation);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
on_handle_force_method (FooiGenBat             *object,
                        xdbus_method_invocation_t  *invocation,
                        xvariant_t               *force_in_i,
                        xvariant_t               *force_in_s,
                        xvariant_t               *force_in_ay,
                        xvariant_t               *force_in_struct,
                        xpointer_t                user_data)
{
  xvariant_t *ret_i;
  xvariant_t *ret_s;
  xvariant_t *ret_ay;
  xvariant_t *ret_struct;
  gint32 val;
  xchar_t *s;

  ret_i = xvariant_new_int32 (xvariant_get_int32 (force_in_i) + 10);
  s = xstrdup_printf ("%s_foo", xvariant_get_string (force_in_s, NULL));
  ret_s = xvariant_new_string (s);
  g_free (s);
  s = xstrdup_printf ("%s_foo\xff", xvariant_get_bytestring (force_in_ay));
  ret_ay = xvariant_new_bytestring (s);
  g_free (s);

  xvariant_get (force_in_struct, "(i)", &val);
  ret_struct = xvariant_new ("(i)", val + 10);

  xvariant_ref_sink (ret_i);
  xvariant_ref_sink (ret_s);
  xvariant_ref_sink (ret_ay);
  xvariant_ref_sink (ret_struct);

  foo_igen_bat_emit_force_signal (object,
                                  ret_i,
                                  ret_s,
                                  ret_ay,
                                  ret_struct);

  foo_igen_bat_complete_force_method (object,
                                      invocation,
                                      ret_i,
                                      ret_s,
                                      ret_ay,
                                      ret_struct);

  xvariant_unref (ret_i);
  xvariant_unref (ret_s);
  xvariant_unref (ret_ay);
  xvariant_unref (ret_struct);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}


/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
my_g_authorize_method_handler (xdbus_interface_skeleton_t *interface,
                               xdbus_method_invocation_t  *invocation,
                               xpointer_t                user_data)
{
  const xchar_t *method_name;
  xboolean_t authorized;

  authorized = FALSE;

  method_name = xdbus_method_invocation_get_method_name (invocation);
  if (xstrcmp0 (method_name, "CheckNotAuthorized") == 0)
    {
      authorized = FALSE;
    }
  else if (xstrcmp0 (method_name, "CheckAuthorized") == 0)
    {
      authorized = TRUE;
    }
  else if (xstrcmp0 (method_name, "CheckNotAuthorizedFromObject") == 0)
    {
      authorized = TRUE;
    }
  else
    {
      g_assert_not_reached ();
    }

  if (!authorized)
    {
      xdbus_method_invocation_return_error (invocation,
                                             G_IO_ERROR,
                                             G_IO_ERROR_PERMISSION_DENIED,
                                             "not authorized...");
    }
  return authorized;
}

static xboolean_t
my_object_authorize_method_handler (xdbus_object_skeleton_t     *object,
                                    xdbus_interface_skeleton_t  *interface,
                                    xdbus_method_invocation_t   *invocation,
                                    xpointer_t                 user_data)
{
  const xchar_t *method_name;
  xboolean_t authorized;

  authorized = FALSE;

  method_name = xdbus_method_invocation_get_method_name (invocation);
  if (xstrcmp0 (method_name, "CheckNotAuthorized") == 0)
    {
      authorized = TRUE;
    }
  else if (xstrcmp0 (method_name, "CheckAuthorized") == 0)
    {
      authorized = TRUE;
    }
  else if (xstrcmp0 (method_name, "CheckNotAuthorizedFromObject") == 0)
    {
      authorized = FALSE;
    }
  else
    {
      g_assert_not_reached ();
    }

  if (!authorized)
    {
      xdbus_method_invocation_return_error (invocation,
                                             G_IO_ERROR,
                                             G_IO_ERROR_PENDING,
                                             "not authorized (from object)...");
    }
  return authorized;
}

static xboolean_t
on_handle_check_not_authorized (FooiGenAuthorize       *object,
                                xdbus_method_invocation_t  *invocation,
                                xpointer_t                user_data)
{
  foo_igen_authorize_complete_check_not_authorized (object, invocation);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_check_authorized (FooiGenAuthorize       *object,
                            xdbus_method_invocation_t  *invocation,
                            xpointer_t                user_data)
{
  foo_igen_authorize_complete_check_authorized (object, invocation);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
on_handle_check_not_authorized_from_object (FooiGenAuthorize       *object,
                                            xdbus_method_invocation_t  *invocation,
                                            xpointer_t                user_data)
{
  foo_igen_authorize_complete_check_not_authorized_from_object (object, invocation);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
on_handle_get_self (FooiGenMethodThreads   *object,
                    xdbus_method_invocation_t  *invocation,
                    xpointer_t                user_data)
{
  xchar_t *s;
  s = xstrdup_printf ("%p", (void *)xthread_self ());
  foo_igen_method_threads_complete_get_self (object, invocation, s);
  g_free (s);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

/* ---------------------------------------------------------------------------------------------------- */

static xthread_t *method_handler_thread = NULL;

static FooiGenBar *exported_bar_object = NULL;
static FooiGenBat *exported_bat_object = NULL;
static FooiGenAuthorize *exported_authorize_object = NULL;
static xdbus_object_skeleton_t *authorize_enclosing_object = NULL;
static FooiGenMethodThreads *exported_thread_object_1 = NULL;
static FooiGenMethodThreads *exported_thread_object_2 = NULL;

static void
unexport_objects (void)
{
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (exported_bar_object));
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (exported_bat_object));
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (exported_authorize_object));
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (exported_thread_object_1));
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (exported_thread_object_2));
}

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xerror_t *error;

  /* test_t that we can export an object using the generated
   * FooiGenBarSkeleton subclass. Notes:
   *
   * 1. We handle methods by simply connecting to the appropriate
   * xobject_t signal.
   *
   * 2. Property storage is taken care of by the class; we can
   *    use xobject_get()/xobject_set() (and the generated
   *    C bindings at will)
   */
  error = NULL;
  exported_bar_object = foo_igen_bar_skeleton_new ();
  foo_igen_bar_set_ay (exported_bar_object, "ABCabc");
  foo_igen_bar_set_y (exported_bar_object, 42);
  foo_igen_bar_set_d (exported_bar_object, 43.0);
  foo_igen_bar_set_finally_normal_name (exported_bar_object, "There aint no place like home");
  foo_igen_bar_set_writeonly_property (exported_bar_object, "Mr. Burns");

  /* The following works because it's on the Skeleton object - it will
   * fail (at run-time) on a Proxy (see on_proxy_appeared() below)
   */
  foo_igen_bar_set_readonly_property (exported_bar_object, "blah");
  g_assert_cmpstr (foo_igen_bar_get_writeonly_property (exported_bar_object), ==, "Mr. Burns");

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (exported_bar_object),
                                    connection,
                                    "/bar",
                                    &error);
  g_assert_no_error (error);
  xsignal_connect (exported_bar_object,
                    "handle-hello-world",
                    G_CALLBACK (on_handle_hello_world),
                    NULL);
  xsignal_connect (exported_bar_object,
                    "handle-test-primitive-types",
                    G_CALLBACK (on_handle_test_primitive_types),
                    NULL);
  xsignal_connect (exported_bar_object,
                    "handle-test-non-primitive-types",
                    G_CALLBACK (on_handle_test_non_primitive_types),
                    NULL);
  xsignal_connect (exported_bar_object,
                    "handle-request-signal-emission",
                    G_CALLBACK (on_handle_request_signal_emission),
                    NULL);
  xsignal_connect (exported_bar_object,
                    "handle-request-multi-property-mods",
                    G_CALLBACK (on_handle_request_multi_property_mods),
                    NULL);
  xsignal_connect (exported_bar_object,
                    "handle-property-cancellation",
                    G_CALLBACK (on_handle_property_cancellation),
                    NULL);

  exported_bat_object = foo_igen_bat_skeleton_new ();
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (exported_bat_object),
                                    connection,
                                    "/bat",
                                    &error);
  g_assert_no_error (error);
  xsignal_connect (exported_bat_object,
                    "handle-force-method",
                    G_CALLBACK (on_handle_force_method),
                    NULL);
  xobject_set (exported_bat_object,
                "force-i", xvariant_new_int32 (43),
                "force-s", xvariant_new_string ("prop string"),
                "force-ay", xvariant_new_bytestring ("prop bytestring\xff"),
                "force-struct", xvariant_new ("(i)", 4300),
                NULL);

  authorize_enclosing_object = xdbus_object_skeleton_new ("/authorize");
  xsignal_connect (authorize_enclosing_object,
                    "authorize-method",
                    G_CALLBACK (my_object_authorize_method_handler),
                    NULL);
  exported_authorize_object = foo_igen_authorize_skeleton_new ();
  xdbus_object_skeleton_add_interface (authorize_enclosing_object,
                                        G_DBUS_INTERFACE_SKELETON (exported_authorize_object));
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (exported_authorize_object),
                                    connection,
                                    "/authorize",
                                    &error);
  g_assert_no_error (error);
  xsignal_connect (exported_authorize_object,
                    "g-authorize-method",
                    G_CALLBACK (my_g_authorize_method_handler),
                    NULL);
  xsignal_connect (exported_authorize_object,
                    "handle-check-not-authorized",
                    G_CALLBACK (on_handle_check_not_authorized),
                    NULL);
  xsignal_connect (exported_authorize_object,
                    "handle-check-authorized",
                    G_CALLBACK (on_handle_check_authorized),
                    NULL);
  xsignal_connect (exported_authorize_object,
                    "handle-check-not-authorized-from-object",
                    G_CALLBACK (on_handle_check_not_authorized_from_object),
                    NULL);


  /* only object 1 has the G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD flag set */
  exported_thread_object_1 = foo_igen_method_threads_skeleton_new ();
  g_dbus_interface_skeleton_set_flags (G_DBUS_INTERFACE_SKELETON (exported_thread_object_1),
                                       G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);

  xassert (!g_dbus_interface_skeleton_has_connection (G_DBUS_INTERFACE_SKELETON (exported_thread_object_1), connection));
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (exported_thread_object_1),
                                    connection,
                                    "/method_threads_1",
                                    &error);
  g_assert_no_error (error);
  xsignal_connect (exported_thread_object_1,
                    "handle-get-self",
                    G_CALLBACK (on_handle_get_self),
                    NULL);
  g_assert_cmpint (g_dbus_interface_skeleton_get_flags (G_DBUS_INTERFACE_SKELETON (exported_thread_object_1)), ==, G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);

  exported_thread_object_2 = foo_igen_method_threads_skeleton_new ();
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (exported_thread_object_2),
                                    connection,
                                    "/method_threads_2",
                                    &error);
  g_assert_no_error (error);
  xsignal_connect (exported_thread_object_2,
                    "handle-get-self",
                    G_CALLBACK (on_handle_get_self),
                    NULL);

  g_assert_cmpint (g_dbus_interface_skeleton_get_flags (G_DBUS_INTERFACE_SKELETON (exported_thread_object_2)), ==, G_DBUS_INTERFACE_SKELETON_FLAGS_NONE);

  method_handler_thread = xthread_self ();
}

static xpointer_t check_proxies_in_thread (xpointer_t user_data);

static void
on_name_acquired (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  xpointer_t         user_data)
{
  xmain_loop_t *loop = user_data;
  xthread_t *thread = xthread_new ("check-proxies", check_proxies_in_thread, loop);
  xthread_unref (thread);
}

static void
on_name_lost (xdbus_connection_t *connection,
              const xchar_t     *name,
              xpointer_t         user_data)
{
  g_assert_not_reached ();
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xmain_loop_t *thread_loop;
  xint_t initial_y;
  xint_t initial_i;
  xuint_t num_g_properties_changed;
  xboolean_t received_test_signal;
  xuint_t num_notify_u;
  xuint_t num_notify_n;
} ClientData;

static void
on_notify_u (xobject_t    *object,
           xparam_spec_t *pspec,
           xpointer_t    user_data)
{
  ClientData *data = user_data;
  g_assert_cmpstr (pspec->name, ==, "u");
  data->num_notify_u += 1;
}

static void
on_notify_n (xobject_t    *object,
             xparam_spec_t *pspec,
             xpointer_t    user_data)
{
  ClientData *data = user_data;
  g_assert_cmpstr (pspec->name, ==, "n");
  data->num_notify_n += 1;
}

static void
on_g_properties_changed (xdbus_proxy_t          *_proxy,
                         xvariant_t            *changed_properties,
                         const xchar_t* const  *invalidated_properties,
                         xpointer_t             user_data)
{
  ClientData *data = user_data;
  FooiGenBar *proxy = FOO_IGEN_BAR (_proxy);

  g_assert_cmpint (xvariant_n_children (changed_properties), ==, 2);

  if (data->num_g_properties_changed == 0)
    {
      g_assert_cmpint (data->initial_y, ==, foo_igen_bar_get_y (proxy) - 2);
      g_assert_cmpint (data->initial_i, ==, foo_igen_bar_get_i (proxy) - 2);
    }
  else if (data->num_g_properties_changed == 1)
    {
      g_assert_cmpint (data->initial_y, ==, foo_igen_bar_get_y (proxy) - 3);
      g_assert_cmpint (data->initial_i, ==, foo_igen_bar_get_i (proxy) - 3);
    }
  else
    g_assert_not_reached ();

  data->num_g_properties_changed++;

  if (data->num_g_properties_changed == 2)
    xmain_loop_quit (data->thread_loop);
}

static void
on_test_signal (FooiGenBar          *proxy,
                xint_t                 val_int32,
                const xchar_t* const  *array_of_strings,
                const xchar_t* const  *array_of_bytestrings,
                xvariant_t            *dict_s_to_pairs,
                xpointer_t             user_data)
{
  ClientData *data = user_data;

  g_assert_cmpint (val_int32, ==, 43);
  g_assert_cmpstr (array_of_strings[0], ==, "foo");
  g_assert_cmpstr (array_of_strings[1], ==, "bar");
  xassert (array_of_strings[2] == NULL);
  g_assert_cmpstr (array_of_bytestrings[0], ==, "foo\xff");
  g_assert_cmpstr (array_of_bytestrings[1], ==, "bar\xff");
  xassert (array_of_bytestrings[2] == NULL);

  data->received_test_signal = TRUE;
  xmain_loop_quit (data->thread_loop);
}

static void
on_property_cancellation_cb (FooiGenBar    *proxy,
                             xasync_result_t  *res,
                             xpointer_t       user_data)
{
  ClientData *data = user_data;
  xboolean_t ret;
  xerror_t *error = NULL;

  error = NULL;
  ret = foo_igen_bar_call_property_cancellation_finish (proxy, res, &error);
  g_assert_no_error (error);
  xassert (ret);

  xmain_loop_quit (data->thread_loop);
}

static void
check_bar_proxy (FooiGenBar *proxy,
                 xmain_loop_t  *thread_loop)
{
  const xchar_t *array_of_strings[3] = {"one", "two", NULL};
  const xchar_t *array_of_strings_2[3] = {"one2", "two2", NULL};
  const xchar_t *array_of_objpaths[3] = {"/one", "/one/two", NULL};
  xvariant_t *array_of_signatures = NULL;
  const xchar_t *array_of_bytestrings[3] = {"one\xff", "two\xff", NULL};
  xchar_t **ret_array_of_strings = NULL;
  xchar_t **ret_array_of_objpaths = NULL;
  xvariant_t *ret_array_of_signatures = NULL;
  xchar_t **ret_array_of_bytestrings = NULL;
  xuchar_t ret_val_byte;
  xboolean_t ret_val_boolean;
  gint16 ret_val_int16;
  xuint16_t ret_val_uint16;
  xint_t ret_val_int32;
  xuint_t ret_val_uint32;
  sint64_t ret_val_int64;
  xuint64_t ret_val_uint64;
  xdouble_t ret_val_double;
  xchar_t *ret_val_string;
  xchar_t *ret_val_objpath;
  xchar_t *ret_val_signature;
  xchar_t *ret_val_bytestring;
  xboolean_t ret;
  xerror_t *error;
  ClientData *data;
  xuchar_t val_y;
  xboolean_t val_b;
  xint_t val_n;
  xuint_t val_q;
  xint_t val_i;
  xuint_t val_u;
  sint64_t val_x;
  xuint64_t val_t;
  xdouble_t val_d;
  xchar_t *val_s;
  xchar_t *val_o;
  xchar_t *val_g;
  xchar_t *val_ay;
  xchar_t **val_as;
  xchar_t **val_ao;
  xvariant_t *val_ag;
  gint32 val_unset_i;
  xdouble_t val_unset_d;
  xchar_t *val_unset_s;
  xchar_t *val_unset_o;
  xchar_t *val_unset_g;
  xchar_t *val_unset_ay;
  xchar_t **val_unset_as;
  xchar_t **val_unset_ao;
  xvariant_t *val_unset_ag;
  xvariant_t *val_unset_struct;
  xchar_t *val_finally_normal_name;
  xvariant_t *v;
  xchar_t *s;
  const xchar_t *const *read_as;
  const xchar_t *const *read_as2;
  const xchar_t *const *read_as3;

  data = g_new0 (ClientData, 1);
  data->thread_loop = thread_loop;

  v = xdbus_proxy_get_cached_property (G_DBUS_PROXY (proxy), "y");
  xassert (v != NULL);
  xvariant_unref (v);

  /* set empty values to non-empty */
  val_unset_i = 42;
  val_unset_d = 42.0;
  val_unset_s = "42";
  val_unset_o = "42";
  val_unset_g = "42";
  val_unset_ay = NULL;
  val_unset_as = NULL;
  val_unset_ao = NULL;
  val_unset_ag = NULL;
  val_unset_struct = NULL;
  /* check properties */
  xobject_get (proxy,
                "y", &val_y,
                "b", &val_b,
                "n", &val_n,
                "q", &val_q,
                "i", &val_i,
                "u", &val_u,
                "x", &val_x,
                "t", &val_t,
                "d", &val_d,
                "s", &val_s,
                "o", &val_o,
                "g", &val_g,
                "ay", &val_ay,
                "as", &val_as,
                "ao", &val_ao,
                "ag", &val_ag,
                "unset_i", &val_unset_i,
                "unset_d", &val_unset_d,
                "unset_s", &val_unset_s,
                "unset_o", &val_unset_o,
                "unset_g", &val_unset_g,
                "unset_ay", &val_unset_ay,
                "unset_as", &val_unset_as,
                "unset_ao", &val_unset_ao,
                "unset_ag", &val_unset_ag,
                "unset_struct", &val_unset_struct,
                "finally-normal-name", &val_finally_normal_name,
                NULL);
  g_assert_cmpint (val_y, ==, 42);
  g_assert_cmpstr (val_finally_normal_name, ==, "There aint no place like home");
  g_free (val_s);
  g_free (val_o);
  g_free (val_g);
  g_assert_cmpstr (val_ay, ==, "ABCabc");
  g_free (val_ay);
  xstrfreev (val_as);
  xstrfreev (val_ao);
  xvariant_unref (val_ag);
  g_free (val_finally_normal_name);
  /* check empty values */
  g_assert_cmpint (val_unset_i, ==, 0);
  g_assert_cmpfloat (val_unset_d, ==, 0.0);
  g_assert_cmpstr (val_unset_s, ==, "");
  g_assert_cmpstr (val_unset_o, ==, "/");
  g_assert_cmpstr (val_unset_g, ==, "");
  g_free (val_unset_s);
  g_free (val_unset_o);
  g_free (val_unset_g);
  g_assert_cmpstr (val_unset_ay, ==, "");
  xassert (val_unset_as[0] == NULL);
  xassert (val_unset_ao[0] == NULL);
  xassert (xvariant_is_of_type (val_unset_ag, G_VARIANT_TYPE ("ag")));
  xassert (xvariant_is_of_type (val_unset_struct, G_VARIANT_TYPE ("(idsogayasaoag)")));
  s = xvariant_print (val_unset_struct, TRUE);
  g_assert_cmpstr (s, ==, "(0, 0.0, '', objectpath '/', signature '', @ay [], @as [], @ao [], @ag [])");
  g_free (s);
  g_free (val_unset_ay);
  xstrfreev (val_unset_as);
  xstrfreev (val_unset_ao);
  xvariant_unref (val_unset_ag);
  xvariant_unref (val_unset_struct);

  /* Try setting a property. This causes the generated glue to invoke
   * the org.fd.DBus.Properties.Set() method asynchronously. So we
   * have to wait for properties-changed...
   */
  foo_igen_bar_set_finally_normal_name (proxy, "foo!");
  _g_assert_property_notify (proxy, "finally-normal-name");
  g_assert_cmpstr (foo_igen_bar_get_finally_normal_name (proxy), ==, "foo!");

  /* Try setting properties that requires memory management. This
   * is to exercise the paths that frees the references.
   */

  xobject_set (proxy,
                "s", "a string",
                "o", "/a/path",
                "g", "asig",
                "ay", xvariant_new_parsed ("[byte 0x65, 0x67]"),
                "as", array_of_strings,
                "ao", array_of_objpaths,
                "ag", xvariant_new_parsed ("[@g 'ass', 'git']"),
                NULL);

  error = NULL;
  ret = foo_igen_bar_call_test_primitive_types_sync (proxy,
                                                     10,
                                                     TRUE,
                                                     11,
                                                     12,
                                                     13,
                                                     14,
                                                     15,
                                                     16,
                                                     17,
                                                     "a string",
                                                     "/a/path",
                                                     "asig",
                                                     "bytestring\xff",
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
                                                     G_DBUS_CALL_FLAGS_NONE,
                                                     -1,
#endif
                                                     &ret_val_byte,
                                                     &ret_val_boolean,
                                                     &ret_val_int16,
                                                     &ret_val_uint16,
                                                     &ret_val_int32,
                                                     &ret_val_uint32,
                                                     &ret_val_int64,
                                                     &ret_val_uint64,
                                                     &ret_val_double,
                                                     &ret_val_string,
                                                     &ret_val_objpath,
                                                     &ret_val_signature,
                                                     &ret_val_bytestring,
                                                     NULL, /* xcancellable_t */
                                                     &error);
  g_assert_no_error (error);
  xassert (ret);

  g_clear_pointer (&ret_val_string, g_free);
  g_clear_pointer (&ret_val_objpath, g_free);
  g_clear_pointer (&ret_val_signature, g_free);
  g_clear_pointer (&ret_val_bytestring, g_free);

  error = NULL;
  array_of_signatures = xvariant_ref_sink (xvariant_new_parsed ("[@g 'ass', 'git']"));
  ret = foo_igen_bar_call_test_non_primitive_types_sync (proxy,
                                                         xvariant_new_parsed ("{'one': 'red',"
                                                                               " 'two': 'blue'}"),
                                                         xvariant_new_parsed ("{'first': (42, 42), "
                                                                               "'second': (43, 43)}"),
                                                         xvariant_new_parsed ("(42, 'foo', 'bar')"),
                                                         array_of_strings,
                                                         array_of_objpaths,
                                                         array_of_signatures,
                                                         array_of_bytestrings,
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
                                                         G_DBUS_CALL_FLAGS_NONE,
                                                         -1,
#endif
                                                         &ret_array_of_strings,
                                                         &ret_array_of_objpaths,
                                                         &ret_array_of_signatures,
                                                         &ret_array_of_bytestrings,
                                                         &s,
                                                         NULL, /* xcancellable_t */
                                                         &error);

  g_assert_no_error (error);
  xassert (ret);

  g_assert_nonnull (ret_array_of_strings);
  g_assert_cmpuint (xstrv_length ((xchar_t **) ret_array_of_strings), ==,
                    xstrv_length ((xchar_t **) array_of_strings));
  g_assert_nonnull (ret_array_of_objpaths);
  g_assert_cmpuint (xstrv_length ((xchar_t **) ret_array_of_objpaths), ==,
                    xstrv_length ((xchar_t **) array_of_objpaths));
  g_assert_nonnull (ret_array_of_signatures);
  g_assert_cmpvariant (ret_array_of_signatures, array_of_signatures);
  g_assert_nonnull (ret_array_of_bytestrings);
  g_assert_cmpuint (xstrv_length ((xchar_t **) ret_array_of_bytestrings), ==,
                    xstrv_length ((xchar_t **) array_of_bytestrings));

  g_clear_pointer (&ret_array_of_strings, xstrfreev);
  g_clear_pointer (&ret_array_of_objpaths, xstrfreev);
  g_clear_pointer (&ret_array_of_signatures, xvariant_unref);
  g_clear_pointer (&ret_array_of_bytestrings, xstrfreev);
  g_clear_pointer (&s, g_free);

  /* Check that org.freedesktop.DBus.Error.UnknownMethod is returned on
   * unimplemented methods.
   */
  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_bar_call_unimplemented_method_sync (proxy, G_DBUS_CALL_FLAGS_NONE, -1, NULL /* xcancellable_t */, &error);
#else
  ret = foo_igen_bar_call_unimplemented_method_sync (proxy, NULL /* xcancellable_t */, &error);
#endif
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
  xerror_free (error);
  error = NULL;
  xassert (!ret);

  xsignal_connect (proxy,
                    "test-signal",
                    G_CALLBACK (on_test_signal),
                    data);
  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_bar_call_request_signal_emission_sync (proxy, 0, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
#else
  ret = foo_igen_bar_call_request_signal_emission_sync (proxy, 0, NULL, &error);
#endif
  g_assert_no_error (error);
  xassert (ret);

  xassert (!data->received_test_signal);
  xmain_loop_run (thread_loop);
  xassert (data->received_test_signal);

  /* Try setting a property. This causes the generated glue to invoke
   * the org.fd.DBus.Properties.Set() method asynchronously. So we
   * have to wait for properties-changed...
   */
  foo_igen_bar_set_finally_normal_name (proxy, "hey back!");
  _g_assert_property_notify (proxy, "finally-normal-name");
  g_assert_cmpstr (foo_igen_bar_get_finally_normal_name (proxy), ==, "hey back!");

  /* Check that multiple calls to a strv getter works... and that
   * updates on them works as well (See comment for "property vfuncs"
   * in gio/gdbus-codegen/codegen.py for details)
   */
  read_as = foo_igen_bar_get_as (proxy);
  read_as2 = foo_igen_bar_get_as (proxy);
  g_assert_cmpint (xstrv_length ((xchar_t **) read_as), ==, 2);
  g_assert_cmpstr (read_as[0], ==, "one");
  g_assert_cmpstr (read_as[1], ==, "two");
  xassert (read_as == read_as2); /* this is more testing an implementation detail */
  xobject_set (proxy,
                "as", array_of_strings_2,
                NULL);
  _g_assert_property_notify (proxy, "as");
  read_as3 = foo_igen_bar_get_as (proxy);
  g_assert_cmpint (xstrv_length ((xchar_t **) read_as3), ==, 2);
  g_assert_cmpstr (read_as3[0], ==, "one2");
  g_assert_cmpstr (read_as3[1], ==, "two2");

  /* Check that grouping changes in idle works.
   *
   * See on_handle_request_multi_property_mods(). The server should
   * emit exactly two PropertiesChanged signals each containing two
   * properties.
   *
   * On the first reception, y and i should both be increased by
   * two. On the second reception, only by one. The signal handler
   * checks this.
   *
   * This also checks that _drain_notify() works.
   */
  data->initial_y = foo_igen_bar_get_y (proxy);
  data->initial_i = foo_igen_bar_get_i (proxy);
  xsignal_connect (proxy,
                    "g-properties-changed",
                    G_CALLBACK (on_g_properties_changed),
                    data);
  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_bar_call_request_multi_property_mods_sync (proxy, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
#else
  ret = foo_igen_bar_call_request_multi_property_mods_sync (proxy, NULL, &error);
#endif
  g_assert_no_error (error);
  xassert (ret);
  xmain_loop_run (thread_loop);
  g_assert_cmpint (data->num_g_properties_changed, ==, 2);
  xsignal_handlers_disconnect_by_func (proxy,
                                        G_CALLBACK (on_g_properties_changed),
                                        data);

  /* Check that we don't emit PropertiesChanged() if the property
   * didn't change... we actually get two notifies.. one for the
   * local set (without a value change) and one when receiving
   * the PropertiesChanged() signal generated from the remote end.
   */
  g_assert_cmpint (data->num_notify_u, ==, 0);
  xsignal_connect (proxy,
                    "notify::u",
                    G_CALLBACK (on_notify_u),
                    data);
  foo_igen_bar_set_u (proxy, 1042);
  g_assert_cmpint (data->num_notify_u, ==, 1);
  g_assert_cmpint (foo_igen_bar_get_u (proxy), ==, 0);
  _g_assert_property_notify (proxy, "u");
  g_assert_cmpint (foo_igen_bar_get_u (proxy), ==, 1042);
  g_assert_cmpint (data->num_notify_u, ==, 2);

  /* Now change u again to the same value.. this will cause a
   * local notify:: notify and the usual Properties.Set() call
   *
   * (Btw, why also the Set() call if the value in the cache is
   * the same? Because someone else might have changed it
   * in the mean time and we're just waiting to receive the
   * PropertiesChanged() signal...)
   *
   * More tricky - how do we check for the *absence* of the
   * notification that u changed? Simple: we change another
   * property and wait for that PropertiesChanged() message
   * to arrive.
   */
  foo_igen_bar_set_u (proxy, 1042);
  g_assert_cmpint (data->num_notify_u, ==, 3);

  g_assert_cmpint (data->num_notify_n, ==, 0);
  xsignal_connect (proxy,
                    "notify::n",
                    G_CALLBACK (on_notify_n),
                    data);
  foo_igen_bar_set_n (proxy, 10042);
  g_assert_cmpint (data->num_notify_n, ==, 1);
  g_assert_cmpint (foo_igen_bar_get_n (proxy), ==, 0);
  _g_assert_property_notify (proxy, "n");
  g_assert_cmpint (foo_igen_bar_get_n (proxy), ==, 10042);
  g_assert_cmpint (data->num_notify_n, ==, 2);
  /* Checks that u didn't change at all */
  g_assert_cmpint (data->num_notify_u, ==, 3);

  /* Now we check that if the service does
   *
   *   xuint_t n = foo_igen_bar_get_n (foo);
   *   foo_igen_bar_set_n (foo, n + 1);
   *   foo_igen_bar_set_n (foo, n);
   *
   *  then no PropertiesChanged() signal is emitted!
   */
  error = NULL;
  foo_igen_bar_call_property_cancellation (proxy,
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
#endif
                                           NULL, /* xcancellable_t */
                                           (xasync_ready_callback_t) on_property_cancellation_cb,
                                           data);
  xmain_loop_run (thread_loop);
  /* Checks that n didn't change at all */
  g_assert_cmpint (data->num_notify_n, ==, 2);

  /* cleanup */
  g_free (data);
  xvariant_unref (array_of_signatures);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_force_signal (FooiGenBat *proxy,
                 xvariant_t   *force_i,
                 xvariant_t   *force_s,
                 xvariant_t   *force_ay,
                 xvariant_t   *force_struct,
                 xpointer_t    user_data)
{
  xboolean_t *signal_received = user_data;
  xint_t val;

  xassert (!(*signal_received));

  g_assert_cmpint (xvariant_get_int32 (force_i), ==, 42 + 10);
  g_assert_cmpstr (xvariant_get_string (force_s, NULL), ==, "a string_foo");
  g_assert_cmpstr (xvariant_get_bytestring (force_ay), ==, "a bytestring\xff_foo\xff");
  xvariant_get (force_struct, "(i)", &val);
  g_assert_cmpint (val, ==, 4200 + 10);

  *signal_received = TRUE;
}

static void
check_bat_proxy (FooiGenBat *proxy,
                 xmain_loop_t  *thread_loop)
{
  xerror_t *error;
  xvariant_t *ret_i;
  xvariant_t *ret_s;
  xvariant_t *ret_ay;
  xvariant_t *ret_struct;
  xint_t val;
  xboolean_t force_signal_received;

  /* --------------------------------------------------- */
  /* Check type-mapping where we force use of a xvariant_t */
  /* --------------------------------------------------- */

  /* check properties */
  xobject_get (proxy,
                "force-i", &ret_i,
                "force-s", &ret_s,
                "force-ay", &ret_ay,
                "force-struct", &ret_struct,
                NULL);
  g_assert_cmpint (xvariant_get_int32 (ret_i), ==, 43);
  g_assert_cmpstr (xvariant_get_string (ret_s, NULL), ==, "prop string");
  g_assert_cmpstr (xvariant_get_bytestring (ret_ay), ==, "prop bytestring\xff");
  xvariant_get (ret_struct, "(i)", &val);
  g_assert_cmpint (val, ==, 4300);
  xvariant_unref (ret_i);
  xvariant_unref (ret_s);
  xvariant_unref (ret_ay);
  xvariant_unref (ret_struct);

  /* check method and signal */
  force_signal_received = FALSE;
  xsignal_connect (proxy,
                    "force-signal",
                    G_CALLBACK (on_force_signal),
                    &force_signal_received);

  error = NULL;
  foo_igen_bat_call_force_method_sync (proxy,
                                       xvariant_new_int32 (42),
                                       xvariant_new_string ("a string"),
                                       xvariant_new_bytestring ("a bytestring\xff"),
                                       xvariant_new ("(i)", 4200),
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
#endif
                                       &ret_i,
                                       &ret_s,
                                       &ret_ay,
                                       &ret_struct,
                                       NULL, /* xcancellable_t* */
                                       &error);
  g_assert_no_error (error);
  g_assert_cmpint (xvariant_get_int32 (ret_i), ==, 42 + 10);
  g_assert_cmpstr (xvariant_get_string (ret_s, NULL), ==, "a string_foo");
  g_assert_cmpstr (xvariant_get_bytestring (ret_ay), ==, "a bytestring\xff_foo\xff");
  xvariant_get (ret_struct, "(i)", &val);
  g_assert_cmpint (val, ==, 4200 + 10);
  xvariant_unref (ret_i);
  xvariant_unref (ret_s);
  xvariant_unref (ret_ay);
  xvariant_unref (ret_struct);
  _g_assert_signal_received (proxy, "force-signal");
  xassert (force_signal_received);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
check_authorize_proxy (FooiGenAuthorize *proxy,
                       xmain_loop_t        *thread_loop)
{
  xerror_t *error;
  xboolean_t ret;

  /* Check that g-authorize-method works as intended */

  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_authorize_call_check_not_authorized_sync (proxy, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
#else
  ret = foo_igen_authorize_call_check_not_authorized_sync (proxy, NULL, &error);
#endif
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED);
  xerror_free (error);
  xassert (!ret);

  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_authorize_call_check_authorized_sync (proxy, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
#else
  ret = foo_igen_authorize_call_check_authorized_sync (proxy, NULL, &error);
#endif
  g_assert_no_error (error);
  xassert (ret);

  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_authorize_call_check_not_authorized_from_object_sync (proxy, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
#else
  ret = foo_igen_authorize_call_check_not_authorized_from_object_sync (proxy, NULL, &error);
#endif
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  xerror_free (error);
  xassert (!ret);
}

/* ---------------------------------------------------------------------------------------------------- */

static xthread_t *
get_self_via_proxy (FooiGenMethodThreads *proxy_1)
{
  xerror_t *error;
  xchar_t *self_str;
  xboolean_t ret;
  xpointer_t self;

  error = NULL;
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
  ret = foo_igen_method_threads_call_get_self_sync (proxy_1, G_DBUS_CALL_FLAGS_NONE, -1, &self_str, NULL, &error);
#else
  ret = foo_igen_method_threads_call_get_self_sync (proxy_1, &self_str, NULL, &error);
#endif
  g_assert_no_error (error);
  xassert (ret);

  g_assert_cmpint (sscanf (self_str, "%p", &self), ==, 1);

  g_free (self_str);

  return self;
}

static void
check_thread_proxies (FooiGenMethodThreads *proxy_1,
                      FooiGenMethodThreads *proxy_2,
                      xmain_loop_t            *thread_loop)
{
  /* proxy_1 is indeed using threads so should never get the handler thread */
  xassert (get_self_via_proxy (proxy_1) != method_handler_thread);

  /* proxy_2 is not using threads so should get the handler thread */
  xassert (get_self_via_proxy (proxy_2) == method_handler_thread);
}

/* ---------------------------------------------------------------------------------------------------- */

static xpointer_t
check_proxies_in_thread (xpointer_t user_data)
{
  xmain_loop_t *loop = user_data;
#ifdef _XPL_ADDRESS_SANITIZER

  /* Silence "Not available before 2.38" when using old API */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_test_incomplete ("FIXME: Leaks a GWeakRef, see glib#2312");
  G_GNUC_END_IGNORE_DEPRECATIONS

  (void) check_thread_proxies;
  (void) check_authorize_proxy;
  (void) check_bat_proxy;
  (void) check_bar_proxy;
#else
  xmain_context_t *thread_context;
  xmain_loop_t *thread_loop;
  xerror_t *error;
  FooiGenBar *bar_proxy;
  FooiGenBat *bat_proxy;
  FooiGenAuthorize *authorize_proxy;
  FooiGenMethodThreads *thread_proxy_1;
  FooiGenMethodThreads *thread_proxy_2;

  thread_context = xmain_context_new ();
  thread_loop = xmain_loop_new (thread_context, FALSE);
  xmain_context_push_thread_default (thread_context);

  /* Check the object */
  error = NULL;
  bar_proxy = foo_igen_bar_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   "org.gtk.GDBus.BindingsTool.test_t",
                                                   "/bar",
                                                   NULL, /* xcancellable_t* */
                                                   &error);
  check_bar_proxy (bar_proxy, thread_loop);
  g_assert_no_error (error);
  xobject_unref (bar_proxy);

  error = NULL;
  bat_proxy = foo_igen_bat_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   "org.gtk.GDBus.BindingsTool.test_t",
                                                   "/bat",
                                                   NULL, /* xcancellable_t* */
                                                   &error);
  check_bat_proxy (bat_proxy, thread_loop);
  g_assert_no_error (error);
  xobject_unref (bat_proxy);

  error = NULL;
  authorize_proxy = foo_igen_authorize_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                               G_DBUS_PROXY_FLAGS_NONE,
                                                               "org.gtk.GDBus.BindingsTool.test_t",
                                                               "/authorize",
                                                               NULL, /* xcancellable_t* */
                                                               &error);
  check_authorize_proxy (authorize_proxy, thread_loop);
  g_assert_no_error (error);
  xobject_unref (authorize_proxy);

  error = NULL;
  thread_proxy_1 = foo_igen_method_threads_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                                   "org.gtk.GDBus.BindingsTool.test_t",
                                                                   "/method_threads_1",
                                                                   NULL, /* xcancellable_t* */
                                                                   &error);
  g_assert_no_error (error);
  thread_proxy_2 = foo_igen_method_threads_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                                   "org.gtk.GDBus.BindingsTool.test_t",
                                                                   "/method_threads_2",
                                                                   NULL, /* xcancellable_t* */
                                                                   &error);
  g_assert_no_error (error);
  check_thread_proxies (thread_proxy_1, thread_proxy_2, thread_loop);
  xobject_unref (thread_proxy_1);
  xobject_unref (thread_proxy_2);

  xmain_loop_unref (thread_loop);
  xmain_context_unref (thread_context);
#endif

  /* this breaks out of the loop in main() (below) */
  xmain_loop_quit (loop);
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xchar_t *xml;
  xmain_loop_t *loop;
} IntrospectData;

static void
introspect_cb (xdbus_connection_t   *connection,
               xasync_result_t      *res,
               xpointer_t           user_data)
{
  IntrospectData *data = user_data;
  xvariant_t *result;
  xerror_t *error;

  error = NULL;
  result = xdbus_connection_call_finish (connection,
                                          res,
                                          &error);
  g_assert_no_error (error);
  xassert (result != NULL);
  xvariant_get (result, "(s)", &data->xml);
  xvariant_unref (result);

  xmain_loop_quit (data->loop);
}

static xdbus_node_info_t *
introspect (xdbus_connection_t  *connection,
            const xchar_t      *name,
            const xchar_t      *object_path,
            xmain_loop_t        *loop)
{
  xerror_t *error;
  xdbus_node_info_t *node_info;
  IntrospectData *data;

  data = g_new0 (IntrospectData, 1);
  data->xml = NULL;
  data->loop = loop;

  /* do this async to avoid deadlocks */
  xdbus_connection_call (connection,
                          name,
                          object_path,
                          "org.freedesktop.DBus.Introspectable",
                          "Introspect",
                          NULL, /* params */
                          G_VARIANT_TYPE ("(s)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) introspect_cb,
                          data);
  xmain_loop_run (loop);
  xassert (data->xml != NULL);

  error = NULL;
  node_info = g_dbus_node_info_new_for_xml (data->xml, &error);
  g_assert_no_error (error);
  xassert (node_info != NULL);
  g_free (data->xml);
  g_free (data);

  return node_info;
}

static xuint_t
count_interfaces (xdbus_node_info_t *info)
{
  xuint_t n;
  for (n = 0; info->interfaces != NULL && info->interfaces[n] != NULL; n++)
    ;
  return n;
}

static xuint_t
count_nodes (xdbus_node_info_t *info)
{
  xuint_t n;
  for (n = 0; info->nodes != NULL && info->nodes[n] != NULL; n++)
    ;
  return n;
}

static xuint_t
has_interface (xdbus_node_info_t *info,
               const xchar_t   *name)
{
  xuint_t n;
  for (n = 0; info->interfaces != NULL && info->interfaces[n] != NULL; n++)
    {
      if (xstrcmp0 (info->interfaces[n]->name, name) == 0)
        return TRUE;
    }
  return FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
  xmain_loop_t *loop;
  xvariant_t *result;
} OMGetManagedObjectsData;

static void
om_get_all_cb (xdbus_connection_t *connection,
               xasync_result_t    *res,
               xpointer_t         user_data)
{
  OMGetManagedObjectsData *data = user_data;
  xerror_t *error;

  error = NULL;
  data->result = xdbus_connection_call_finish (connection,
                                                res,
                                                &error);
  g_assert_no_error (error);
  xassert (data->result != NULL);
  xmain_loop_quit (data->loop);
}

static void
om_check_get_all (xdbus_connection_t *c,
                  xmain_loop_t       *loop,
                  const xchar_t     *str)
{
  OMGetManagedObjectsData data;
  xchar_t *s;

  data.loop = loop;
  data.result = NULL;

  /* do this async to avoid deadlocks */
  xdbus_connection_call (c,
                          xdbus_connection_get_unique_name (c),
                          "/managed",
                          "org.freedesktop.DBus.ObjectManager",
                          "GetManagedObjects",
                          NULL, /* params */
                          G_VARIANT_TYPE ("(a{oa{sa{sv}}})"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) om_get_all_cb,
                          &data);
  xmain_loop_run (loop);
  xassert (data.result != NULL);
  s = xvariant_print (data.result, TRUE);
  g_assert_cmpstr (s, ==, str);
  g_free (s);
  xvariant_unref (data.result);
}

typedef struct
{
  xmain_loop_t *loop;
  xuint_t state;

  xuint_t num_object_proxy_added_signals;
  xuint_t num_object_proxy_removed_signals;
  xuint_t num_interface_added_signals;
  xuint_t num_interface_removed_signals;
} OMData;

static xint_t
my_pstrcmp (const xchar_t **a, const xchar_t **b)
{
  return xstrcmp0 (*a, *b);
}

static void
om_check_interfaces_added (const xchar_t *signal_name,
                           xvariant_t *parameters,
                           const xchar_t *object_path,
                           const xchar_t *first_interface_name,
                           ...)
{
  const xchar_t *path;
  xvariant_t *array;
  xuint_t n;
  xptr_array_t *interfaces;
  xptr_array_t *interfaces_in_message;
  va_list var_args;
  const xchar_t *str;

  interfaces = xptr_array_new ();
  xptr_array_add (interfaces, (xpointer_t) first_interface_name);
  va_start (var_args, first_interface_name);
  do
    {
      str = va_arg (var_args, const xchar_t *);
      if (str == NULL)
        break;
      xptr_array_add (interfaces, (xpointer_t) str);
    }
  while (TRUE);
  va_end (var_args);

  xvariant_get (parameters, "(&o*)", &path, &array);
  g_assert_cmpstr (signal_name, ==, "InterfacesAdded");
  g_assert_cmpstr (path, ==, object_path);
  g_assert_cmpint (xvariant_n_children (array), ==, interfaces->len);
  interfaces_in_message = xptr_array_new ();
  for (n = 0; n < interfaces->len; n++)
    {
      const xchar_t *iface_name;
      xvariant_get_child (array, n, "{&sa{sv}}", &iface_name, NULL);
      xptr_array_add (interfaces_in_message, (xpointer_t) iface_name);
    }
  g_assert_cmpint (interfaces_in_message->len, ==, interfaces->len);
  xptr_array_sort (interfaces, (GCompareFunc) my_pstrcmp);
  xptr_array_sort (interfaces_in_message, (GCompareFunc) my_pstrcmp);
  for (n = 0; n < interfaces->len; n++)
    g_assert_cmpstr (interfaces->pdata[n], ==, interfaces_in_message->pdata[n]);
  xptr_array_unref (interfaces_in_message);
  xptr_array_unref (interfaces);
  xvariant_unref (array);
}

static void
om_check_interfaces_removed (const xchar_t *signal_name,
                             xvariant_t *parameters,
                             const xchar_t *object_path,
                             const xchar_t *first_interface_name,
                             ...)
{
  const xchar_t *path;
  xvariant_t *array;
  xuint_t n;
  xptr_array_t *interfaces;
  xptr_array_t *interfaces_in_message;
  va_list var_args;
  const xchar_t *str;

  interfaces = xptr_array_new ();
  xptr_array_add (interfaces, (xpointer_t) first_interface_name);
  va_start (var_args, first_interface_name);
  do
    {
      str = va_arg (var_args, const xchar_t *);
      if (str == NULL)
        break;
      xptr_array_add (interfaces, (xpointer_t) str);
    }
  while (TRUE);
  va_end (var_args);

  xvariant_get (parameters, "(&o*)", &path, &array);
  g_assert_cmpstr (signal_name, ==, "InterfacesRemoved");
  g_assert_cmpstr (path, ==, object_path);
  g_assert_cmpint (xvariant_n_children (array), ==, interfaces->len);
  interfaces_in_message = xptr_array_new ();
  for (n = 0; n < interfaces->len; n++)
    {
      const xchar_t *iface_name;
      xvariant_get_child (array, n, "&s", &iface_name, NULL);
      xptr_array_add (interfaces_in_message, (xpointer_t) iface_name);
    }
  g_assert_cmpint (interfaces_in_message->len, ==, interfaces->len);
  xptr_array_sort (interfaces, (GCompareFunc) my_pstrcmp);
  xptr_array_sort (interfaces_in_message, (GCompareFunc) my_pstrcmp);
  for (n = 0; n < interfaces->len; n++)
    g_assert_cmpstr (interfaces->pdata[n], ==, interfaces_in_message->pdata[n]);
  xptr_array_unref (interfaces_in_message);
  xptr_array_unref (interfaces);
  xvariant_unref (array);
}

static void
om_on_signal (xdbus_connection_t *connection,
              const xchar_t     *sender_name,
              const xchar_t     *object_path,
              const xchar_t     *interface_name,
              const xchar_t     *signal_name,
              xvariant_t        *parameters,
              xpointer_t         user_data)
{
  OMData *om_data = user_data;

  //g_debug ("foo: %s", xvariant_print (parameters, TRUE));

  switch (om_data->state)
    {
    default:
    case 0:
      g_printerr ("failing and om_data->state=%d on signal %s, params=%s\n",
               om_data->state,
               signal_name,
               xvariant_print (parameters, TRUE));
      g_assert_not_reached ();
      break;

    case 1:
      om_check_interfaces_added (signal_name, parameters, "/managed/first",
                                 "org.project.Bar", NULL);
      om_data->state = 2;
      xmain_loop_quit (om_data->loop);
      break;

    case 3:
      om_check_interfaces_removed (signal_name, parameters, "/managed/first",
                                   "org.project.Bar", NULL);
      om_data->state = 5;
      /* keep running the loop */
      break;

    case 5:
      om_check_interfaces_added (signal_name, parameters, "/managed/first",
                                 "org.project.Bar", NULL);
      om_data->state = 6;
      xmain_loop_quit (om_data->loop);
      break;

    case 7:
      om_check_interfaces_removed (signal_name, parameters, "/managed/first",
                                   "org.project.Bar", NULL);
      om_data->state = 9;
      /* keep running the loop */
      break;

    case 9:
      om_check_interfaces_added (signal_name, parameters, "/managed/first",
                                 "org.project.Bar", NULL);
      om_data->state = 10;
      xmain_loop_quit (om_data->loop);
      break;

    case 11:
      om_check_interfaces_added (signal_name, parameters, "/managed/first",
                                 "org.project.Bat", NULL);
      om_data->state = 12;
      xmain_loop_quit (om_data->loop);
      break;

    case 13:
      om_check_interfaces_removed (signal_name, parameters, "/managed/first",
                                   "org.project.Bar", NULL);
      om_data->state = 14;
      xmain_loop_quit (om_data->loop);
      break;

    case 15:
      om_check_interfaces_removed (signal_name, parameters, "/managed/first",
                                   "org.project.Bat", NULL);
      om_data->state = 16;
      xmain_loop_quit (om_data->loop);
      break;

    case 17:
      om_check_interfaces_added (signal_name, parameters, "/managed/first",
                                 "com.acme.Coyote", NULL);
      om_data->state = 18;
      xmain_loop_quit (om_data->loop);
      break;

    case 101:
      om_check_interfaces_added (signal_name, parameters, "/managed/second",
                                 "org.project.Bat", "org.project.Bar", NULL);
      om_data->state = 102;
      xmain_loop_quit (om_data->loop);
      break;

    case 103:
      om_check_interfaces_removed (signal_name, parameters, "/managed/second",
                                   "org.project.Bat", "org.project.Bar", NULL);
      om_data->state = 104;
      xmain_loop_quit (om_data->loop);
      break;

    case 200:
      om_check_interfaces_added (signal_name, parameters, "/managed/first_1",
                                 "com.acme.Coyote", NULL);
      om_data->state = 201;
      xmain_loop_quit (om_data->loop);
      break;
    }
}

static xasync_result_t *om_res = NULL;

static void
om_pm_start_cb (FooiGenObjectManagerClient *manager,
                xasync_result_t               *res,
                xpointer_t                    user_data)
{
  xmain_loop_t *loop = user_data;
  om_res = xobject_ref (res);
  xmain_loop_quit (loop);
}

static void
on_interface_added (xdbus_object_t    *object,
                    xdbus_interface_t *interface,
                    xpointer_t        user_data)
{
  OMData *om_data = user_data;
  om_data->num_interface_added_signals += 1;
}

static void
on_interface_removed (xdbus_object_t    *object,
                      xdbus_interface_t *interface,
                      xpointer_t        user_data)
{
  OMData *om_data = user_data;
  om_data->num_interface_removed_signals += 1;
}

static void
on_object_proxy_added (xdbus_object_manager_client_t  *manager,
                       xdbus_object_proxy_t   *object_proxy,
                       xpointer_t            user_data)
{
  OMData *om_data = user_data;
  om_data->num_object_proxy_added_signals += 1;
  xsignal_connect (object_proxy,
                    "interface-added",
                    G_CALLBACK (on_interface_added),
                    om_data);
  xsignal_connect (object_proxy,
                    "interface-removed",
                    G_CALLBACK (on_interface_removed),
                    om_data);
}

static void
on_object_proxy_removed (xdbus_object_manager_client_t  *manager,
                         xdbus_object_proxy_t   *object_proxy,
                         xpointer_t            user_data)
{
  OMData *om_data = user_data;
  om_data->num_object_proxy_removed_signals += 1;
  g_assert_cmpint (xsignal_handlers_disconnect_by_func (object_proxy,
                                                         G_CALLBACK (on_interface_added),
                                                         om_data), ==, 1);
  g_assert_cmpint (xsignal_handlers_disconnect_by_func (object_proxy,
                                                         G_CALLBACK (on_interface_removed),
                                                         om_data), ==, 1);
}

static void
property_changed (xobject_t    *object,
		  xparam_spec_t *pspec,
		  xpointer_t    user_data)
{
  xboolean_t *changed = user_data;

  *changed = TRUE;
}

static void
om_check_property_and_signal_emission (xmain_loop_t  *loop,
                                       FooiGenBar *skeleton,
                                       FooiGenBar *proxy)
{
  xboolean_t d_changed = FALSE;
  xboolean_t quiet_changed = FALSE;
  xboolean_t quiet_too_changed = FALSE;
  xuint_t handler;

  /* First PropertiesChanged */
  g_assert_cmpint (foo_igen_bar_get_i (skeleton), ==, 0);
  g_assert_cmpint (foo_igen_bar_get_i (proxy), ==, 0);
  foo_igen_bar_set_i (skeleton, 1);
  _g_assert_property_notify (proxy, "i");
  g_assert_cmpint (foo_igen_bar_get_i (skeleton), ==, 1);
  g_assert_cmpint (foo_igen_bar_get_i (proxy), ==, 1);

  /* Double-check the xdouble_t case */
  g_assert_cmpfloat (foo_igen_bar_get_d (skeleton), ==, 0.0);
  g_assert_cmpfloat (foo_igen_bar_get_d (proxy), ==, 0.0);
  foo_igen_bar_set_d (skeleton, 1.0);
  _g_assert_property_notify (proxy, "d");

  /* Verify that re-setting it to the same value doesn't cause a
   * notify on the proxy, by taking advantage of the fact that
   * notifications are serialized.
   */
  handler = xsignal_connect (proxy, "notify::d",
			      G_CALLBACK (property_changed), &d_changed);
  foo_igen_bar_set_d (skeleton, 1.0);
  foo_igen_bar_set_i (skeleton, 2);
  _g_assert_property_notify (proxy, "i");
  xassert (d_changed == FALSE);
  xsignal_handler_disconnect (proxy, handler);

  /* Verify that re-setting a property with the "EmitsChangedSignal"
   * set to false doesn't emit a signal. */
  handler = xsignal_connect (proxy, "notify::quiet",
			      G_CALLBACK (property_changed), &quiet_changed);
  foo_igen_bar_set_quiet (skeleton, "hush!");
  foo_igen_bar_set_i (skeleton, 3);
  _g_assert_property_notify (proxy, "i");
  xassert (quiet_changed == FALSE);
  g_assert_cmpstr (foo_igen_bar_get_quiet (skeleton), ==, "hush!");
  xsignal_handler_disconnect (proxy, handler);

  /* Also verify that re-setting a property with the "EmitsChangedSignal"
   * set to 'const' doesn't emit a signal. */
  handler = xsignal_connect (proxy, "notify::quiet-too",
			      G_CALLBACK (property_changed), &quiet_changed);
  foo_igen_bar_set_quiet_too (skeleton, "hush too!");
  foo_igen_bar_set_i (skeleton, 4);
  _g_assert_property_notify (proxy, "i");
  xassert (quiet_too_changed == FALSE);
  g_assert_cmpstr (foo_igen_bar_get_quiet_too (skeleton), ==, "hush too!");
  xsignal_handler_disconnect (proxy, handler);

  /* Then just a regular signal */
  foo_igen_bar_emit_another_signal (skeleton, "word");
  _g_assert_signal_received (proxy, "another-signal");
}

static void
check_object_manager (void)
{
  FooiGenObjectSkeleton *o = NULL;
  FooiGenObjectSkeleton *o2 = NULL;
  FooiGenObjectSkeleton *o3 = NULL;
  xdbus_interface_skeleton_t *i;
  xdbus_connection_t *c;
  xdbus_object_manager_server_t *manager = NULL;
  xdbus_node_info_t *info;
  xerror_t *error;
  xmain_loop_t *loop;
  OMData *om_data = NULL;
  xuint_t om_signal_id = 0;
  xdbus_object_manager_t *pm = NULL;
  xlist_t *object_proxies;
  xlist_t *proxies;
  xdbus_object_t *op;
  xdbus_proxy_t *p;
  FooiGenBar *bar_skeleton;
  xdbus_interface_t *iface;
  xchar_t *path, *name, *name_owner;
  xdbus_connection_t *c2;
  GDBusObjectManagerClientFlags flags;

  loop = xmain_loop_new (NULL, FALSE);

  om_data = g_new0 (OMData, 1);
  om_data->loop = loop;
  om_data->state = 0;

  error = NULL;
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  g_assert_no_error (error);
  xassert (c != NULL);

  om_signal_id = xdbus_connection_signal_subscribe (c,
                                                     NULL, /* sender */
                                                     "org.freedesktop.DBus.ObjectManager",
                                                     NULL, /* member */
                                                     NULL, /* object_path */
                                                     NULL, /* arg0 */
                                                     G_DBUS_SIGNAL_FLAGS_NONE,
                                                     om_on_signal,
                                                     om_data,
                                                     NULL); /* user_data_free_func */

  /* Our xdbus_object_manager_client_t tests are simple - we basically just count the
   * number of times the various signals have been emitted (we don't check
   * that the right objects/interfaces are passed though - that's checked
   * in the lower-level tests in om_on_signal()...)
   *
   * Note that these tests rely on the D-Bus signal handlers used by
   * xdbus_object_manager_client_t firing before om_on_signal().
   */
  error = NULL;
  pm = foo_igen_object_manager_client_new_sync (c,
                                                G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                xdbus_connection_get_unique_name (c),
                                                "/managed",
                                                NULL, /* xcancellable_t */
                                                &error);
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD);
  xerror_free (error);
  xassert (pm == NULL);

  manager = xdbus_object_manager_server_new ("/managed");

  xassert (xdbus_object_manager_server_get_connection (manager) == NULL);

  xdbus_object_manager_server_set_connection (manager, c);

  g_assert_cmpstr (g_dbus_object_manager_get_object_path (G_DBUS_OBJECT_MANAGER (manager)), ==, "/managed");
  xobject_get (manager, "object-path", &path, "connection", &c2, NULL);
  g_assert_cmpstr (path, ==, "/managed");
  xassert (c2 == c);
  g_free (path);
  g_clear_object (&c2);

  /* Check that the manager object is visible */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* ObjectManager + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.freedesktop.DBus.ObjectManager"));
  g_assert_cmpint (count_nodes (info), ==, 0);
  g_dbus_node_info_unref (info);

  /* Check GetManagedObjects() - should be empty since we have no objects */
  om_check_get_all (c, loop,
                    "(@a{oa{sa{sv}}} {},)");

  /* Now try to create the proxy manager again - this time it should work */
  error = NULL;
  foo_igen_object_manager_client_new (c,
                                      G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                      xdbus_connection_get_unique_name (c),
                                      "/managed",
                                      NULL, /* xcancellable_t */
                                      (xasync_ready_callback_t) om_pm_start_cb,
                                      loop);
  xmain_loop_run (loop);
  error = NULL;
  pm = foo_igen_object_manager_client_new_finish (om_res, &error);
  g_clear_object (&om_res);
  g_assert_no_error (error);
  xassert (pm != NULL);
  xsignal_connect (pm,
                    "object-added",
                    G_CALLBACK (on_object_proxy_added),
                    om_data);
  xsignal_connect (pm,
                    "object-removed",
                    G_CALLBACK (on_object_proxy_removed),
                    om_data);

  g_assert_cmpstr (g_dbus_object_manager_get_object_path (G_DBUS_OBJECT_MANAGER (pm)), ==, "/managed");
  xobject_get (pm,
                "object-path", &path,
                "connection", &c2,
                "name", &name,
                "name-owner", &name_owner,
                "flags", &flags,
                NULL);
  g_assert_cmpstr (path, ==, "/managed");
  g_assert_cmpstr (name, ==, xdbus_connection_get_unique_name (c));
  g_assert_cmpstr (name_owner, ==, xdbus_connection_get_unique_name (c));
  g_assert_cmpint (flags, ==, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE);
  xassert (c2 == c);
  g_free (path);
  g_clear_object (&c2);
  g_free (name);
  g_free (name_owner);

  /* ... check there are no object proxies yet */
  object_proxies = g_dbus_object_manager_get_objects (pm);
  xassert (object_proxies == NULL);

  /* First, export an object with a single interface (also check that
   * g_dbus_interface_get_object() works and that the object isn't reffed)
   */
  o = foo_igen_object_skeleton_new ("/managed/first");
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_bar_skeleton_new ());
  xassert (g_dbus_interface_get_object (G_DBUS_INTERFACE (i)) == NULL);
  g_assert_cmpint (G_OBJECT (o)->ref_count, ==, 1);
  foo_igen_object_skeleton_set_bar (o, FOO_IGEN_BAR (i));
  g_assert_cmpint (G_OBJECT (o)->ref_count, ==, 1);
  xassert (g_dbus_interface_get_object (G_DBUS_INTERFACE (i)) == G_DBUS_OBJECT (o));
  g_assert_cmpint (G_OBJECT (o)->ref_count, ==, 1);
  foo_igen_object_skeleton_set_bar (o, NULL);
  xassert (g_dbus_interface_get_object (G_DBUS_INTERFACE (i)) == NULL);
  g_assert_cmpint (G_OBJECT (o)->ref_count, ==, 1);
  foo_igen_object_skeleton_set_bar (o, FOO_IGEN_BAR (i));
  xassert (g_dbus_interface_get_object (G_DBUS_INTERFACE (i)) == G_DBUS_OBJECT (o));
  g_assert_cmpint (G_OBJECT (o)->ref_count, ==, 1);

  o2 = FOO_IGEN_OBJECT_SKELETON (g_dbus_interface_dup_object (G_DBUS_INTERFACE (i)));
  xassert (G_DBUS_OBJECT (o2) == G_DBUS_OBJECT (o));
  g_assert_cmpint (G_OBJECT (o2)->ref_count, ==, 2);
  g_clear_object (&o2);

  xdbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (o));

  /* ... check we get the InterfacesAdded signal */
  om_data->state = 1;

  xmain_loop_run (om_data->loop);

  g_assert_cmpint (om_data->state, ==, 2);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 0);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 0);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 0);
  /* ... check there's one non-standard interfaces */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* Bar + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.project.Bar"));
  g_dbus_node_info_unref (info);

  /* Also check g_dbus_object_manager_get_interface */
  iface = g_dbus_object_manager_get_interface (G_DBUS_OBJECT_MANAGER (manager), "/managed/first", "org.project.Bar");
  xassert (iface != NULL);
  g_clear_object (&iface);
  iface = g_dbus_object_manager_get_interface (G_DBUS_OBJECT_MANAGER (manager), "/managed/first", "org.project.Bat");
  xassert (iface == NULL);
  iface = g_dbus_object_manager_get_interface (G_DBUS_OBJECT_MANAGER (pm), "/managed/first", "org.project.Bar");
  xassert (iface != NULL);
  g_clear_object (&iface);
  iface = g_dbus_object_manager_get_interface (G_DBUS_OBJECT_MANAGER (pm), "/managed/first", "org.project.Bat");
  xassert (iface == NULL);

  /* Now, check adding the same interface replaces the existing one */
  foo_igen_object_skeleton_set_bar (o, FOO_IGEN_BAR (i));
  /* ... check we get the InterfacesRemoved */
  om_data->state = 3;
  xmain_loop_run (om_data->loop);
  /* ... and then check we get the InterfacesAdded */
  g_assert_cmpint (om_data->state, ==, 6);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 2);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 0);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 0);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* Bar + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.project.Bar"));
  g_dbus_node_info_unref (info);
  g_clear_object (&i);

  /* check adding an interface of same type (but not same object) replaces the existing one */
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_bar_skeleton_new ());
  foo_igen_object_skeleton_set_bar (o, FOO_IGEN_BAR (i));
  /* ... check we get the InterfacesRemoved and then InterfacesAdded */
  om_data->state = 7;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 10);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 3);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 2);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 0);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 0);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* Bar + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.project.Bar"));
  g_dbus_node_info_unref (info);
  g_clear_object (&i);

  /* check adding an interface of another type doesn't replace the existing one */
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_bat_skeleton_new ());
  foo_igen_object_skeleton_set_bat (o, FOO_IGEN_BAT (i));
  g_clear_object (&i);
  /* ... check we get the InterfacesAdded */
  om_data->state = 11;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 12);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 3);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 2);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 0);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 5); /* Bar,Bat + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.project.Bar"));
  xassert (has_interface (info, "org.project.Bat"));
  g_dbus_node_info_unref (info);

  /* check we can remove an interface */
  foo_igen_object_skeleton_set_bar (o, NULL);
  /* ... check we get the InterfacesRemoved */
  om_data->state = 13;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 14);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 3);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 2);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 1);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* Bat + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.project.Bat"));
  g_dbus_node_info_unref (info);
  /* also and that the call only has effect if the interface actually exists
   *
   * (Note: if a signal was emitted we'd assert in the signal handler
   * because we're in state 14)
   */
  foo_igen_object_skeleton_set_bar (o, NULL);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* Bat + Properties,Introspectable,Peer */
  xassert (has_interface (info, "org.project.Bat"));
  g_dbus_node_info_unref (info);

  /* remove the last interface */
  foo_igen_object_skeleton_set_bat (o, NULL);
  /* ... check we get the InterfacesRemoved */
  om_data->state = 15;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 16);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 3);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 3);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 1);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 0); /* nothing */
  g_dbus_node_info_unref (info);

  /* and add an interface again */
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_com_acme_coyote_skeleton_new ());
  foo_igen_object_skeleton_set_com_acme_coyote (o, FOO_IGEN_COM_ACME_COYOTE (i));
  g_clear_object (&i);
  /* ... check we get the InterfacesAdded */
  om_data->state = 17;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 18);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 4);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 3);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 1);
  /* ... check introspection data */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/first", loop);
  g_assert_cmpint (count_interfaces (info), ==, 4); /* com.acme.Coyote + Properties,Introspectable,Peer */
  xassert (has_interface (info, "com.acme.Coyote"));
  g_dbus_node_info_unref (info);

  /* Check GetManagedObjects() - should be just the Coyote */
  om_check_get_all (c, loop,
                    "({objectpath '/managed/first': {'com.acme.Coyote': {'Mood': <''>}}},)");

  /* -------------------------------------------------- */

  /* create a new object with two interfaces */
  o2 = foo_igen_object_skeleton_new ("/managed/second");
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_bar_skeleton_new ());
  bar_skeleton = FOO_IGEN_BAR (i); /* save for later test */
  foo_igen_object_skeleton_set_bar (o2, FOO_IGEN_BAR (i));
  g_clear_object (&i);
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_bat_skeleton_new ());
  foo_igen_object_skeleton_set_bat (o2, FOO_IGEN_BAT (i));
  g_clear_object (&i);
  /* ... add it */
  xdbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (o2));
  /* ... check we get the InterfacesAdded with _two_ interfaces */
  om_data->state = 101;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 102);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 5);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 3);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 1);

  /* -------------------------------------------------- */

  /* Now that we have a couple of objects with interfaces, check
   * that ObjectManager.GetManagedObjects() works
   */
  om_check_get_all (c, loop,
                    "({objectpath '/managed/first': {'com.acme.Coyote': {'Mood': <''>}}, '/managed/second': {'org.project.Bar': {'y': <byte 0x00>, 'b': <false>, 'n': <int16 0>, 'q': <uint16 0>, 'i': <0>, 'u': <uint32 0>, 'x': <int64 0>, 't': <uint64 0>, 'd': <0.0>, 's': <''>, 'o': <objectpath '/'>, 'g': <signature ''>, 'ay': <b''>, 'as': <@as []>, 'aay': <@aay []>, 'ao': <@ao []>, 'ag': <@ag []>, 'FinallyNormalName': <''>, 'ReadonlyProperty': <''>, 'quiet': <''>, 'quiet_too': <''>, 'unset_i': <0>, 'unset_d': <0.0>, 'unset_s': <''>, 'unset_o': <objectpath '/'>, 'unset_g': <signature ''>, 'unset_ay': <b''>, 'unset_as': <@as []>, 'unset_ao': <@ao []>, 'unset_ag': <@ag []>, 'unset_struct': <(0, 0.0, '', objectpath '/', signature '', @ay [], @as [], @ao [], @ag [])>}, 'org.project.Bat': {'force_i': <0>, 'force_s': <''>, 'force_ay': <@ay []>, 'force_struct': <(0,)>}}},)");

  /* Set connection to NULL, causing everything to be unexported.. verify this.. and
   * then set the connection back.. and then check things still work
   */
  xdbus_object_manager_server_set_connection (manager, NULL);
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed", loop);
  g_assert_cmpint (count_interfaces (info), ==, 0); /* nothing */
  g_dbus_node_info_unref (info);

  xdbus_object_manager_server_set_connection (manager, c);
  om_check_get_all (c, loop,
                    "({objectpath '/managed/first': {'com.acme.Coyote': {'Mood': <''>}}, '/managed/second': {'org.project.Bar': {'y': <byte 0x00>, 'b': <false>, 'n': <int16 0>, 'q': <uint16 0>, 'i': <0>, 'u': <uint32 0>, 'x': <int64 0>, 't': <uint64 0>, 'd': <0.0>, 's': <''>, 'o': <objectpath '/'>, 'g': <signature ''>, 'ay': <b''>, 'as': <@as []>, 'aay': <@aay []>, 'ao': <@ao []>, 'ag': <@ag []>, 'FinallyNormalName': <''>, 'ReadonlyProperty': <''>, 'quiet': <''>, 'quiet_too': <''>, 'unset_i': <0>, 'unset_d': <0.0>, 'unset_s': <''>, 'unset_o': <objectpath '/'>, 'unset_g': <signature ''>, 'unset_ay': <b''>, 'unset_as': <@as []>, 'unset_ao': <@ao []>, 'unset_ag': <@ag []>, 'unset_struct': <(0, 0.0, '', objectpath '/', signature '', @ay [], @as [], @ao [], @ag [])>}, 'org.project.Bat': {'force_i': <0>, 'force_s': <''>, 'force_ay': <@ay []>, 'force_struct': <(0,)>}}},)");

  /* Also check that the ObjectManagerClient returns these objects - and
   * that they are of the right xtype_t cf. what was requested via
   * the generated ::get-proxy-type signal handler
   */
  object_proxies = g_dbus_object_manager_get_objects (pm);
  xassert (xlist_length (object_proxies) == 2);
  xlist_free_full (object_proxies, xobject_unref);
  op = g_dbus_object_manager_get_object (pm, "/managed/first");
  xassert (op != NULL);
  xassert (FOO_IGEN_IS_OBJECT_PROXY (op));
  g_assert_cmpstr (g_dbus_object_get_object_path (op), ==, "/managed/first");
  proxies = g_dbus_object_get_interfaces (op);
  xassert (xlist_length (proxies) == 1);
  xlist_free_full (proxies, xobject_unref);
  p = G_DBUS_PROXY (foo_igen_object_get_com_acme_coyote (FOO_IGEN_OBJECT (op)));
  xassert (p != NULL);
  g_assert_cmpint (XTYPE_FROM_INSTANCE (p), ==, FOO_IGEN_TYPE_COM_ACME_COYOTE_PROXY);
  xassert (xtype_is_a (XTYPE_FROM_INSTANCE (p), FOO_IGEN_TYPE_COM_ACME_COYOTE));
  g_clear_object (&p);
  p = (xdbus_proxy_t *) g_dbus_object_get_interface (op, "org.project.NonExisting");
  xassert (p == NULL);
  g_clear_object (&op);

  /* -- */
  op = g_dbus_object_manager_get_object (pm, "/managed/second");
  xassert (op != NULL);
  xassert (FOO_IGEN_IS_OBJECT_PROXY (op));
  g_assert_cmpstr (g_dbus_object_get_object_path (op), ==, "/managed/second");
  proxies = g_dbus_object_get_interfaces (op);
  xassert (xlist_length (proxies) == 2);
  xlist_free_full (proxies, xobject_unref);
  p = G_DBUS_PROXY (foo_igen_object_get_bat (FOO_IGEN_OBJECT (op)));
  xassert (p != NULL);
  g_assert_cmpint (XTYPE_FROM_INSTANCE (p), ==, FOO_IGEN_TYPE_BAT_PROXY);
  xassert (xtype_is_a (XTYPE_FROM_INSTANCE (p), FOO_IGEN_TYPE_BAT));
  g_clear_object (&p);
  p = G_DBUS_PROXY (foo_igen_object_get_bar (FOO_IGEN_OBJECT (op)));
  xassert (p != NULL);
  g_assert_cmpint (XTYPE_FROM_INSTANCE (p), ==, FOO_IGEN_TYPE_BAR_PROXY);
  xassert (xtype_is_a (XTYPE_FROM_INSTANCE (p), FOO_IGEN_TYPE_BAR));
  /* ... now that we have a Bar instance around, also check that we get signals
   *     and property changes...
   */
  om_check_property_and_signal_emission (loop, bar_skeleton, FOO_IGEN_BAR (p));
  g_clear_object (&p);
  p = (xdbus_proxy_t *) g_dbus_object_get_interface (op, "org.project.NonExisting");
  xassert (p == NULL);
  g_clear_object (&op);

  /* -------------------------------------------------- */

  /* Now remove the second object added above */
  xdbus_object_manager_server_unexport (manager, "/managed/second");
  /* ... check we get InterfacesRemoved with both interfaces */
  om_data->state = 103;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 104);
  g_assert_cmpint (om_data->num_object_proxy_added_signals, ==, 5);
  g_assert_cmpint (om_data->num_object_proxy_removed_signals, ==, 4);
  g_assert_cmpint (om_data->num_interface_added_signals, ==, 1);
  g_assert_cmpint (om_data->num_interface_removed_signals, ==, 1);
  /* ... check introspection data (there should be nothing) */
  info = introspect (c, xdbus_connection_get_unique_name (c), "/managed/second", loop);
  g_assert_cmpint (count_nodes (info), ==, 0);
  g_assert_cmpint (count_interfaces (info), ==, 0);
  g_dbus_node_info_unref (info);

  /* Check GetManagedObjects() again */
  om_check_get_all (c, loop,
                    "({objectpath '/managed/first': {'com.acme.Coyote': {'Mood': <''>}}},)");
  /* -------------------------------------------------- */

  /* Check that export_uniquely() works */

  o3 = foo_igen_object_skeleton_new ("/managed/first");
  i = G_DBUS_INTERFACE_SKELETON (foo_igen_com_acme_coyote_skeleton_new ());
  foo_igen_com_acme_coyote_set_mood (FOO_IGEN_COM_ACME_COYOTE (i), "indifferent");
  foo_igen_object_skeleton_set_com_acme_coyote (o3, FOO_IGEN_COM_ACME_COYOTE (i));
  g_clear_object (&i);
  xdbus_object_manager_server_export_uniquely (manager, G_DBUS_OBJECT_SKELETON (o3));
  /* ... check we get the InterfacesAdded signal */
  om_data->state = 200;
  xmain_loop_run (om_data->loop);
  g_assert_cmpint (om_data->state, ==, 201);

  om_check_get_all (c, loop,
                    "({objectpath '/managed/first': {'com.acme.Coyote': {'Mood': <''>}}, '/managed/first_1': {'com.acme.Coyote': {'Mood': <'indifferent'>}}},)");

  //xmain_loop_run (loop); /* TODO: tmp */

  /* Clean up objects */
  xassert (xdbus_object_manager_server_unexport (manager, "/managed/first_1"));
  //xassert (xdbus_object_manager_server_unexport (manager, "/managed/second"));
  xassert (xdbus_object_manager_server_unexport (manager, "/managed/first"));
  g_assert_cmpint (xlist_length (g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (manager))), ==, 0);

  if (loop != NULL)
    xmain_loop_unref (loop);

  if (om_signal_id != 0)
    xdbus_connection_signal_unsubscribe (c, om_signal_id);
  g_clear_object (&o3);
  g_clear_object (&o2);
  g_clear_object (&o);
  g_clear_object (&manager);
  if (pm != NULL)
    {
      g_assert_cmpint (xsignal_handlers_disconnect_by_func (pm,
                                                             G_CALLBACK (on_object_proxy_added),
                                                             om_data), ==, 1);
      g_assert_cmpint (xsignal_handlers_disconnect_by_func (pm,
                                                             G_CALLBACK (on_object_proxy_removed),
                                                             om_data), ==, 1);
      g_clear_object (&pm);
    }
  g_clear_object (&c);

  g_free (om_data);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_object_manager (void)
{
  xmain_loop_t *loop;
  xuint_t id;

  loop = xmain_loop_new (NULL, FALSE);

  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       "org.gtk.GDBus.BindingsTool.test_t",
                       G_BUS_NAME_OWNER_FLAGS_NONE,
                       on_bus_acquired,
                       on_name_acquired,
                       on_name_lost,
                       loop,
                       NULL);

  xmain_loop_run (loop);

  check_object_manager ();

  /* uncomment to keep the service around (to e.g. introspect it) */
  /* xmain_loop_run (loop); */

  unexport_objects ();

  g_bus_unown_name (id);
  xmain_loop_unref (loop);
}

/* ---------------------------------------------------------------------------------------------------- */
/* This checks that forcing names via org.gtk.GDBus.Name works (see test-codegen.xml) */

extern xpointer_t name_forcing_1;
extern xpointer_t name_forcing_2;
extern xpointer_t name_forcing_3;
extern xpointer_t name_forcing_4;
extern xpointer_t name_forcing_5;
extern xpointer_t name_forcing_6;
extern xpointer_t name_forcing_7;
xpointer_t name_forcing_1 = foo_igen_rocket123_get_type;
xpointer_t name_forcing_2 = foo_igen_rocket123_call_ignite_xyz;
xpointer_t name_forcing_3 = foo_igen_rocket123_emit_exploded_xyz;
xpointer_t name_forcing_4 = foo_igen_rocket123_get_speed_xyz;
xpointer_t name_forcing_5 = foo_igen_test_ugly_case_interface_call_get_iscsi_servers;
xpointer_t name_forcing_6 = foo_igen_test_ugly_case_interface_emit_servers_updated_now;
xpointer_t name_forcing_7 = foo_igen_test_ugly_case_interface_get_ugly_name;

/* ---------------------------------------------------------------------------------------------------- */

/* See https://bugzilla.gnome.org/show_bug.cgi?id=647577#c5 for details */

#define CHECK_FIELD(name, v1, v2) g_assert_cmpint (G_STRUCT_OFFSET (FooiGenChangingInterface##v1##Iface, name), ==, G_STRUCT_OFFSET (FooiGenChangingInterface##v2##Iface, name));

static void
test_interface_stability (void)
{
  CHECK_FIELD(handle_foo_method, V1, V2);
  CHECK_FIELD(handle_bar_method, V1, V2);
  CHECK_FIELD(handle_baz_method, V1, V2);
  CHECK_FIELD(foo_signal, V1, V2);
  CHECK_FIELD(bar_signal, V1, V2);
  CHECK_FIELD(baz_signal, V1, V2);
  CHECK_FIELD(handle_new_method_in2, V2, V10);
  CHECK_FIELD(new_signal_in2, V2, V10);
}

#undef CHECK_FIELD

/* ---------------------------------------------------------------------------------------------------- */

/* property naming
 *
 * - check that a property with name "Type" is mapped into g-name "type"
 *   with C accessors get_type_ (to avoid clashing with the xtype_t accessor)
 *   and set_type_ (for symmetry)
 *   (see https://bugzilla.gnome.org/show_bug.cgi?id=679473 for details)
 *
 * - (could add more tests here)
 */

static void
test_property_naming (void)
{
  xpointer_t c_getter_name = foo_igen_naming_get_type_;
  xpointer_t c_setter_name = foo_igen_naming_set_type_;
  FooiGenNaming *skel;

  (void) c_getter_name;
  (void) c_setter_name;

  skel = foo_igen_naming_skeleton_new ();
  xassert (xobject_class_find_property (G_OBJECT_GET_CLASS (skel), "type") != NULL);
  xobject_unref (skel);
}

/* ---------------------------------------------------------------------------------------------------- */

/* autocleanups
 *
 * - check that x_autoptr() works for all generated types, if supported by the
 *   current compiler
 */

static void
test_autocleanups (void)
{
#ifdef x_autoptr
  x_autoptr(FooiGenBar) bar = NULL;
  x_autoptr(FooiGenBarProxy) bar_proxy = NULL;
  x_autoptr(FooiGenBarSkeleton) bar_skeleton = NULL;
  x_autoptr(FooiGenObject) object = NULL;
  x_autoptr(FooiGenObjectProxy) object_proxy = NULL;
  x_autoptr(FooiGenObjectSkeleton) object_skeleton = NULL;
  x_autoptr(FooiGenObjectManagerClient) object_manager_client = NULL;

  (void) bar;
  (void) bar_proxy;
  (void) bar_skeleton;
  (void) object;
  (void) object_proxy;
  (void) object_skeleton;
  (void) object_manager_client;
#elif XPL_CHECK_VERSION(2, 38, 0)
  /* This file is compiled twice, once without GLib version guards and once
   * with
   *
   *   -DXPL_VERSION_MIN_REQUIRED=XPL_VERSION_2_36
   *   -DXPL_VERSION_MAX_ALLOWED=XPL_VERSION_2_36
   *
   * g_test_skip() was added in 2.38.
   */
  g_test_skip ("x_autoptr() not supported on this compiler");
#else
  /* Let's just say it passed. */
#endif
}

/* ---------------------------------------------------------------------------------------------------- */

/* deprecations
 */

static void
test_deprecations (void)
{
  {
    FooiGenOldieInterface *iskel;
    xparam_spec_t *pspec;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    iskel = foo_igen_oldie_interface_skeleton_new ();
    G_GNUC_END_IGNORE_DEPRECATIONS;

    pspec = xobject_class_find_property (G_OBJECT_GET_CLASS (iskel), "bat");
    g_assert_nonnull (pspec);
    g_assert_cmpint (pspec->flags & XPARAM_DEPRECATED, ==, XPARAM_DEPRECATED);

    xobject_unref (iskel);
  }

  {
    FooiGenObjectSkeleton *oskel;
    xparam_spec_t *pspec;

    oskel = foo_igen_object_skeleton_new ("/objects/first");
    pspec = xobject_class_find_property (G_OBJECT_GET_CLASS (oskel), "oldie-interface");
    g_assert_nonnull (pspec);
    g_assert_cmpint (pspec->flags & XPARAM_DEPRECATED, ==, XPARAM_DEPRECATED);

    xobject_unref (oskel);
  }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
assert_arg_infos_equal (xdbus_arg_info_t **a,
                        xdbus_arg_info_t **b)
{
  if (a == NULL)
    {
      g_assert_null (b);
      return;
    }

  g_assert_nonnull (b);

  for (; *a != NULL && *b != NULL; a++, b++)
    {
      g_assert_cmpstr ((*a)->name, ==, (*b)->name);
      g_assert_cmpstr ((*a)->signature, ==, (*b)->signature);
    }

  g_assert_null (*a);
  g_assert_null (*b);
}

static void
assert_annotations_equal (xdbus_annotation_info_t **a,
                          xdbus_annotation_info_t **b)
{
  xuint_t a_len = count_annotations (a);
  xuint_t b_len = count_annotations (b);

  g_assert_cmpuint (a_len, ==, b_len);

  if (a == NULL || b == NULL)
    return;

  for (; *a != NULL && *b != NULL; a++, b++)
    {
      g_assert_cmpstr ((*a)->key, ==, (*b)->key);
      g_assert_cmpstr ((*a)->value, ==, (*b)->value);
      assert_annotations_equal ((*a)->annotations, (*b)->annotations);
    }

  g_assert_null (*a);
  g_assert_null (*b);
}

/* test_t that the xdbus_interface_info_t structure generated by gdbus-codegen
 * --interface-info-body matches that generated by the other mode.
 */
static void
test_standalone_interface_info (void)
{
  xdbus_interface_skeleton_t *skel = G_DBUS_INTERFACE_SKELETON (foo_igen_bar_skeleton_new ());
  xdbus_interface_info_t *skel_info = g_dbus_interface_skeleton_get_info (skel);
  const xdbus_interface_info_t *slim_info = &org_project_bar_interface;
  xsize_t i;

  g_assert_cmpstr (skel_info->name, ==, slim_info->name);

  for (i = 0; skel_info->methods[i] != NULL; i++)
    {
      xdbus_method_info_t *skel_method = skel_info->methods[i];
      xdbus_method_info_t *slim_method = slim_info->methods[i];

      g_assert_nonnull (slim_method);
      g_assert_cmpstr (skel_method->name, ==, slim_method->name);
      assert_arg_infos_equal (skel_method->in_args, slim_method->in_args);
      assert_arg_infos_equal (skel_method->out_args, slim_method->out_args);
      assert_annotations_equal (skel_method->annotations, slim_method->annotations);
    }
  g_assert_null (slim_info->methods[i]);

  for (i = 0; skel_info->signals[i] != NULL; i++)
    {
      xdbus_signalInfo_t *skel_signal = skel_info->signals[i];
      xdbus_signalInfo_t *slim_signal = slim_info->signals[i];

      g_assert_nonnull (slim_signal);
      g_assert_cmpstr (skel_signal->name, ==, slim_signal->name);
      assert_arg_infos_equal (skel_signal->args, slim_signal->args);
      assert_annotations_equal (skel_signal->annotations, slim_signal->annotations);
    }
  g_assert_null (slim_info->signals[i]);

  for (i = 0; skel_info->properties[i] != NULL; i++)
    {
      xdbus_property_info_t *skel_prop = skel_info->properties[i];
      xdbus_property_info_t *slim_prop = slim_info->properties[i];

      g_assert_nonnull (slim_prop);

      g_assert_cmpstr (skel_prop->name, ==, slim_prop->name);
      g_assert_cmpstr (skel_prop->signature, ==, slim_prop->signature);
      g_assert_cmpuint (skel_prop->flags, ==, slim_prop->flags);
      assert_annotations_equal (skel_prop->annotations, slim_prop->annotations);
    }
  g_assert_null (slim_info->properties[i]);

  assert_annotations_equal (skel_info->annotations, slim_info->annotations);

  g_clear_object (&skel);
}

/* ---------------------------------------------------------------------------------------------------- */
static xboolean_t
handle_hello_fd (FooiGenFDPassing *object,
                 xdbus_method_invocation_t *invocation,
                 xunix_fd_list_t *fd_list,
                 const xchar_t *arg_greeting)
{
  foo_igen_fdpassing_complete_hello_fd (object, invocation, fd_list, arg_greeting);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
static xboolean_t
handle_no_annotation (FooiGenFDPassing *object,
                      xdbus_method_invocation_t *invocation,
                      xunix_fd_list_t *fd_list,
                      xvariant_t *arg_greeting,
                      const xchar_t *arg_greeting_locale)
{
  foo_igen_fdpassing_complete_no_annotation (object, invocation, fd_list, arg_greeting, arg_greeting_locale);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
handle_no_annotation_nested (FooiGenFDPassing *object,
                             xdbus_method_invocation_t *invocation,
                             xunix_fd_list_t *fd_list,
                             xvariant_t *arg_files)
{
  foo_igen_fdpassing_complete_no_annotation_nested (object, invocation, fd_list);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}
#else
static xboolean_t
handle_no_annotation (FooiGenFDPassing *object,
                      xdbus_method_invocation_t *invocation,
                      xvariant_t *arg_greeting,
                      const xchar_t *arg_greeting_locale)
{
  foo_igen_fdpassing_complete_no_annotation (object, invocation, arg_greeting, arg_greeting_locale);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}

static xboolean_t
handle_no_annotation_nested (FooiGenFDPassing *object,
                             xdbus_method_invocation_t *invocation,
                             xvariant_t *arg_files)
{
  foo_igen_fdpassing_complete_no_annotation_nested (object, invocation);
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}
#endif

/* test_t that generated code for methods includes xunix_fd_list_t arguments
 * unconditionally if the method is explicitly annotated as C.UnixFD, and only
 * emits xunix_fd_list_t arguments when there's merely an 'h' parameter if
 * --glib-min-required=2.64 or greater.
 */
static void
test_unix_fd_list (void)
{
  FooiGenFDPassingIface iface;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1726");

  /* This method is explicitly annotated. */
  iface.handle_hello_fd = handle_hello_fd;

  /* This one is not annotated; even though it's got an in and out 'h'
   * parameter, for backwards compatibility we cannot emit xunix_fd_list_t
   * arguments unless --glib-min-required >= 2.64 was used.
   */
  iface.handle_no_annotation = handle_no_annotation;

  /* This method has an 'h' inside a complex type. */
  iface.handle_no_annotation_nested = handle_no_annotation_nested;

  (void) iface;
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gdbus/codegen/annotations", test_annotations);
  g_test_add_func ("/gdbus/codegen/interface_stability", test_interface_stability);
  g_test_add_func ("/gdbus/codegen/object-manager", test_object_manager);
  g_test_add_func ("/gdbus/codegen/property-naming", test_property_naming);
  g_test_add_func ("/gdbus/codegen/autocleanups", test_autocleanups);
  g_test_add_func ("/gdbus/codegen/deprecations", test_deprecations);
  g_test_add_func ("/gdbus/codegen/standalone-interface-info", test_standalone_interface_info);
  g_test_add_func ("/gdbus/codegen/unix-fd-list", test_unix_fd_list);

  return session_bus_run ();
}
