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
/* test_t that the method aspects of xdbus_proxy_t works */
/* ---------------------------------------------------------------------------------------------------- */

static void
test_methods (xdbus_proxy_t *proxy)
{
  xvariant_t *result;
  xerror_t *error;
  const xchar_t *str;
  xchar_t *dbus_error_name;

  /* check that we can invoke a method */
  error = NULL;
  result = xdbus_proxy_call_sync (proxy,
                                   "HelloWorld",
                                   xvariant_new ("(s)", "Hey"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "(s)");
  xvariant_get (result, "(&s)", &str);
  g_assert_cmpstr (str, ==, "You greeted me with 'Hey'. Thanks!");
  xvariant_unref (result);

  /* Check that we can completely recover the returned error */
  result = xdbus_proxy_call_sync (proxy,
                                   "HelloWorld",
                                   xvariant_new ("(s)", "Yo"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_DBUS_ERROR);
  g_assert_true (g_dbus_error_is_remote_error (error));
  g_assert_true (g_dbus_error_is_remote_error (error));
  g_assert_null (result);
  dbus_error_name = g_dbus_error_get_remote_error (error);
  g_assert_cmpstr (dbus_error_name, ==, "com.example.TestException");
  g_free (dbus_error_name);
  g_assert_true (g_dbus_error_strip_remote_error (error));
  g_assert_cmpstr (error->message, ==, "Yo is not a proper greeting");
  g_clear_error (&error);

  /* Check that we get a timeout if the method handling is taking longer than
   * timeout. We use such a long sleep because on slow machines, if the
   * sleep isn't much longer than the timeout and we're doing a parallel
   * build, there's no guarantee we'll be scheduled in the window between
   * the timeout being hit and the sleep finishing. */
  error = NULL;
  result = xdbus_proxy_call_sync (proxy,
                                   "Sleep",
                                   xvariant_new ("(i)", 10000 /* msec */),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   100 /* msec */,
                                   NULL,
                                   &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_assert_null (result);
  g_clear_error (&error);

  /* Check that proxy-default timeouts work. */
  g_assert_cmpint (xdbus_proxy_get_default_timeout (proxy), ==, -1);

  /* the default timeout is 25000 msec so this should work */
  result = xdbus_proxy_call_sync (proxy,
                                   "Sleep",
                                   xvariant_new ("(i)", 500 /* msec */),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, /* use proxy default (e.g. -1 -> e.g. 25000 msec) */
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);

  /* Now set the proxy-default timeout to 250 msec and try the 10000 msec
   * call - this should FAIL. Again, we use such a long sleep because on slow
   * machines there's no guarantee we'll be scheduled when we want to be. */
  xdbus_proxy_set_default_timeout (proxy, 250);
  g_assert_cmpint (xdbus_proxy_get_default_timeout (proxy), ==, 250);
  result = xdbus_proxy_call_sync (proxy,
                                   "Sleep",
                                   xvariant_new ("(i)", 10000 /* msec */),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1, /* use proxy default (e.g. 250 msec) */
                                   NULL,
                                   &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_assert_false (g_dbus_error_is_remote_error (error));
  g_assert_null (result);
  g_clear_error (&error);

  /* clean up after ourselves */
  xdbus_proxy_set_default_timeout (proxy, -1);
}

static xboolean_t
strv_equal (xchar_t **strv, ...)
{
  xsize_t count;
  va_list list;
  const xchar_t *str;
  xboolean_t res;

  res = TRUE;
  count = 0;
  va_start (list, strv);
  while (1)
    {
      str = va_arg (list, const xchar_t *);
      if (str == NULL)
        break;
      if (xstrcmp0 (str, strv[count]) != 0)
        {
          res = FALSE;
          break;
        }
      count++;
    }
  va_end (list);

  if (res)
    res = xstrv_length (strv) == count;

  return res;
}

/* ---------------------------------------------------------------------------------------------------- */
/* test_t that the property aspects of xdbus_proxy_t works */
/* ---------------------------------------------------------------------------------------------------- */

static void
test_properties (xdbus_proxy_t *proxy)
{
  xerror_t *error;
  xvariant_t *variant;
  xvariant_t *variant2;
  xvariant_t *result;
  xchar_t **names;
  xchar_t *name_owner;
  xdbus_proxy_t *proxy2;

  error = NULL;

  if (xdbus_proxy_get_flags (proxy) & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES)
    {
       g_assert_null (xdbus_proxy_get_cached_property_names (proxy));
       return;
    }

  /*
   * Check that we can list all cached properties.
   */
  names = xdbus_proxy_get_cached_property_names (proxy);

  g_assert_true (strv_equal (names,
                             "PropertyThatWillBeInvalidated",
                             "ab",
                             "ad",
                             "ai",
                             "an",
                             "ao",
                             "aq",
                             "as",
                             "at",
                             "au",
                             "ax",
                             "ay",
                             "b",
                             "d",
                             "foo",
                             "i",
                             "n",
                             "o",
                             "q",
                             "s",
                             "t",
                             "u",
                             "x",
                             "y",
                             NULL));

  xstrfreev (names);

  /*
   * Check that we can read cached properties.
   *
   * No need to test all properties - xvariant_t has already been tested
   */
  variant = xdbus_proxy_get_cached_property (proxy, "y");
  g_assert_nonnull (variant);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 1);
  xvariant_unref (variant);
  variant = xdbus_proxy_get_cached_property (proxy, "o");
  g_assert_nonnull (variant);
  g_assert_cmpstr (xvariant_get_string (variant, NULL), ==, "/some/path");
  xvariant_unref (variant);

  /*
   * Now ask the service to change a property and check that #xdbus_proxy_t::g-property-changed
   * is received. Also check that the cache is updated.
   */
  variant2 = xvariant_new_byte (42);
  result = xdbus_proxy_call_sync (proxy,
                                   "FrobSetProperty",
                                   xvariant_new ("(sv)",
                                                  "y",
                                                  variant2),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);
  _g_assert_signal_received (proxy, "g-properties-changed");
  variant = xdbus_proxy_get_cached_property (proxy, "y");
  g_assert_nonnull (variant);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 42);
  xvariant_unref (variant);

  xdbus_proxy_set_cached_property (proxy, "y", xvariant_new_byte (142));
  variant = xdbus_proxy_get_cached_property (proxy, "y");
  g_assert_nonnull (variant);
  g_assert_cmpint (xvariant_get_byte (variant), ==, 142);
  xvariant_unref (variant);

  xdbus_proxy_set_cached_property (proxy, "y", NULL);
  variant = xdbus_proxy_get_cached_property (proxy, "y");
  g_assert_null (variant);

  /* Check that the invalidation feature of the PropertiesChanged()
   * signal works... First, check that we have a cached value of the
   * property (from the initial GetAll() call)
   */
  variant = xdbus_proxy_get_cached_property (proxy, "PropertyThatWillBeInvalidated");
  g_assert_nonnull (variant);
  g_assert_cmpstr (xvariant_get_string (variant, NULL), ==, "InitialValue");
  xvariant_unref (variant);
  /* now ask to invalidate the property - this causes a
   *
   *   PropertiesChanaged("com.example.Frob",
   *                      {},
   *                      ["PropertyThatWillBeInvalidated")
   *
   * signal to be emitted. This is received before the method reply
   * for FrobInvalidateProperty *but* since the proxy was created in
   * the same thread as we're doing this synchronous call, we'll get
   * the method reply before...
   */
  result = xdbus_proxy_call_sync (proxy,
                                   "FrobInvalidateProperty",
                                   xvariant_new ("(s)", "OMGInvalidated"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);
  /* ... hence we wait for the g-properties-changed signal to be delivered */
  _g_assert_signal_received (proxy, "g-properties-changed");
  /* ... and now we finally, check that the cached value has been invalidated */
  variant = xdbus_proxy_get_cached_property (proxy, "PropertyThatWillBeInvalidated");
  g_assert_null (variant);

  /* Now test that G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES works - we need a new proxy for that */
  error = NULL;
  proxy2 = xdbus_proxy_new_sync (xdbus_proxy_get_connection (proxy),
                                  G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                                  NULL,                      /* xdbus_interface_info_t */
                                  "com.example.TestService", /* name */
                                  "/com/example/test_object_t", /* object path */
                                  "com.example.Frob",        /* interface */
                                  NULL, /* xcancellable_t */
                                  &error);
  g_assert_no_error (error);

  name_owner = xdbus_proxy_get_name_owner (proxy2);
  g_assert_nonnull (name_owner);
  g_free (name_owner);

  variant = xdbus_proxy_get_cached_property (proxy2, "PropertyThatWillBeInvalidated");
  g_assert_nonnull (variant);
  g_assert_cmpstr (xvariant_get_string (variant, NULL), ==, "OMGInvalidated"); /* from previous test */
  xvariant_unref (variant);

  result = xdbus_proxy_call_sync (proxy2,
                                   "FrobInvalidateProperty",
                                   xvariant_new ("(s)", "OMGInvalidated2"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);

  /* this time we should get the ::g-properties-changed _with_ the value */
  _g_assert_signal_received (proxy2, "g-properties-changed");

  variant = xdbus_proxy_get_cached_property (proxy2, "PropertyThatWillBeInvalidated");
  g_assert_nonnull (variant);
  g_assert_cmpstr (xvariant_get_string (variant, NULL), ==, "OMGInvalidated2");
  xvariant_unref (variant);

  xobject_unref (proxy2);
}

/* ---------------------------------------------------------------------------------------------------- */
/* test_t that the signal aspects of xdbus_proxy_t works */
/* ---------------------------------------------------------------------------------------------------- */

static void
test_proxy_signals_on_signal (xdbus_proxy_t  *proxy,
                              const xchar_t *sender_name,
                              const xchar_t *signal_name,
                              xvariant_t    *parameters,
                              xpointer_t     user_data)
{
  xstring_t *s = user_data;

  g_assert_cmpstr (signal_name, ==, "TestSignal");
  g_assert_cmpstr (xvariant_get_type_string (parameters), ==, "(sov)");

  xvariant_print_string (parameters, s, TRUE);
}

typedef struct
{
  xmain_loop_t *internal_loop;
  xstring_t *s;
} TestSignalData;

static void
test_proxy_signals_on_emit_signal_cb (xdbus_proxy_t   *proxy,
                                      xasync_result_t *res,
                                      xpointer_t      user_data)
{
  TestSignalData *data = user_data;
  xerror_t *error;
  xvariant_t *result;

  error = NULL;
  result = xdbus_proxy_call_finish (proxy,
                                     res,
                                     &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);

  /* check that the signal was received before we got the method result */
  g_assert_cmpuint (strlen (data->s->str), >, 0);

  /* break out of the loop */
  xmain_loop_quit (data->internal_loop);
}

static void
test_signals (xdbus_proxy_t *proxy)
{
  xerror_t *error;
  xstring_t *s;
  xulong_t signal_handler_id;
  TestSignalData data;
  xvariant_t *result;

  error = NULL;

  /*
   * Ask the service to emit a signal and check that we receive it.
   *
   * Note that blocking calls don't block in the mainloop so wait for the signal (which
   * is dispatched before the method reply)
   */
  s = xstring_new (NULL);
  signal_handler_id = xsignal_connect (proxy,
                                        "g-signal",
                                        G_CALLBACK (test_proxy_signals_on_signal),
                                        s);

  result = xdbus_proxy_call_sync (proxy,
                                   "EmitSignal",
                                   xvariant_new ("(so)",
                                                  "Accept the next proposition you hear",
                                                  "/some/path"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);
  /* check that we haven't received the signal just yet */
  g_assert_cmpuint (strlen (s->str), ==, 0);
  /* and now wait for the signal */
  _g_assert_signal_received (proxy, "g-signal");
  g_assert_cmpstr (s->str,
                   ==,
                   "('Accept the next proposition you hear .. in bed!', objectpath '/some/path/in/bed', <'a variant'>)");
  xsignal_handler_disconnect (proxy, signal_handler_id);
  xstring_free (s, TRUE);

  /*
   * Now do this async to check the signal is received before the method returns.
   */
  s = xstring_new (NULL);
  data.internal_loop = xmain_loop_new (NULL, FALSE);
  data.s = s;
  signal_handler_id = xsignal_connect (proxy,
                                        "g-signal",
                                        G_CALLBACK (test_proxy_signals_on_signal),
                                        s);
  xdbus_proxy_call (proxy,
                     "EmitSignal",
                     xvariant_new ("(so)",
                                    "You will make a great programmer",
                                    "/some/other/path"),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (xasync_ready_callback_t) test_proxy_signals_on_emit_signal_cb,
                     &data);
  xmain_loop_run (data.internal_loop);
  xmain_loop_unref (data.internal_loop);
  g_assert_cmpstr (s->str,
                   ==,
                   "('You will make a great programmer .. in bed!', objectpath '/some/other/path/in/bed', <'a variant'>)");
  xsignal_handler_disconnect (proxy, signal_handler_id);
  xstring_free (s, TRUE);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
test_bogus_method_return (xdbus_proxy_t *proxy)
{
  xerror_t *error = NULL;
  xvariant_t *result;

  result = xdbus_proxy_call_sync (proxy,
                                   "PairReturn",
                                   NULL,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  xerror_free (error);
  g_assert_null (result);
}

#if 0 /* Disabled: see https://bugzilla.gnome.org/show_bug.cgi?id=658999 */
static void
test_bogus_signal (xdbus_proxy_t *proxy)
{
  xerror_t *error = NULL;
  xvariant_t *result;
  xdbus_interface_info_t *old_iface_info;

  result = xdbus_proxy_call_sync (proxy,
                                   "EmitSignal2",
                                   NULL,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      /* and now wait for the signal that will never arrive */
      _g_assert_signal_received (proxy, "g-signal");
    }
  g_test_trap_assert_stderr ("*Dropping signal TestSignal2 of type (i) since the type from the expected interface is (u)*");
  g_test_trap_assert_failed();

  /* Our main branch will also do g_warning() when running the mainloop so
   * temporarily remove the expected interface
   */
  old_iface_info = xdbus_proxy_get_interface_info (proxy);
  xdbus_proxy_set_interface_info (proxy, NULL);
  _g_assert_signal_received (proxy, "g-signal");
  xdbus_proxy_set_interface_info (proxy, old_iface_info);
}

static void
test_bogus_property (xdbus_proxy_t *proxy)
{
  xerror_t *error = NULL;
  xvariant_t *result;
  xdbus_interface_info_t *old_iface_info;

  /* Make the service emit a PropertiesChanged signal for property 'i' of type 'i' - since
   * our introspection data has this as type 'u' we should get a warning on stderr.
   */
  result = xdbus_proxy_call_sync (proxy,
                                   "FrobSetProperty",
                                   xvariant_new ("(sv)",
                                                  "i", xvariant_new_int32 (42)),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_assert_cmpstr (xvariant_get_type_string (result), ==, "()");
  xvariant_unref (result);

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      /* and now wait for the signal that will never arrive */
      _g_assert_signal_received (proxy, "g-properties-changed");
    }
  g_test_trap_assert_stderr ("*Received property i with type i does not match expected type u in the expected interface*");
  g_test_trap_assert_failed();

  /* Our main branch will also do g_warning() when running the mainloop so
   * temporarily remove the expected interface
   */
  old_iface_info = xdbus_proxy_get_interface_info (proxy);
  xdbus_proxy_set_interface_info (proxy, NULL);
  _g_assert_signal_received (proxy, "g-properties-changed");
  xdbus_proxy_set_interface_info (proxy, old_iface_info);
}
#endif /* Disabled: see https://bugzilla.gnome.org/show_bug.cgi?id=658999 */

/* ---------------------------------------------------------------------------------------------------- */

static const xchar_t *frob_dbus_interface_xml =
  "<node>"
  "  <interface name='com.example.Frob'>"
  /* PairReturn() is deliberately different from gdbus-testserver's definition */
  "    <method name='PairReturn'>"
  "      <arg type='u' name='somenumber' direction='in'/>"
  "      <arg type='s' name='somestring' direction='out'/>"
  "    </method>"
  "    <method name='HelloWorld'>"
  "      <arg type='s' name='somestring' direction='in'/>"
  "      <arg type='s' name='somestring' direction='out'/>"
  "    </method>"
  "    <method name='Sleep'>"
  "      <arg type='i' name='timeout' direction='in'/>"
  "    </method>"
  /* We deliberately only mention a single property here */
  "    <property name='y' type='y' access='readwrite'/>"
  /* The 'i' property is deliberately different from gdbus-testserver's definition */
  "    <property name='i' type='u' access='readwrite'/>"
  /* ::TestSignal2 is deliberately different from gdbus-testserver's definition */
  "    <signal name='TestSignal2'>"
  "      <arg type='u' name='somenumber'/>"
  "    </signal>"
  "  </interface>"
  "</node>";
static xdbus_interface_info_t *frob_dbus_interface_info;

static void
test_expected_interface (xdbus_proxy_t *proxy)
{
  xvariant_t *value;
  xerror_t *error;

  /* This is obviously wrong but expected interface is not set so we don't fail... */
  xdbus_proxy_set_cached_property (proxy, "y", xvariant_new_string ("error_me_out!"));
  xdbus_proxy_set_cached_property (proxy, "y", xvariant_new_byte (42));
  xdbus_proxy_set_cached_property (proxy, "does-not-exist", xvariant_new_string ("something"));
  xdbus_proxy_set_cached_property (proxy, "does-not-exist", NULL);

  /* Now repeat the method tests, with an expected interface set */
  xdbus_proxy_set_interface_info (proxy, frob_dbus_interface_info);
  test_methods (proxy);
  test_signals (proxy);

  /* And also where we deliberately set the expected interface definition incorrectly */
  test_bogus_method_return (proxy);
  /* Disabled: see https://bugzilla.gnome.org/show_bug.cgi?id=658999
  test_bogus_signal (proxy);
  test_bogus_property (proxy);
  */

  if (g_test_undefined ())
    {
      /* Also check that we complain if setting a cached property of the wrong type */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                             "*Trying to set property y of type s but according to the expected interface the type is y*");
      value = xvariant_ref_sink (xvariant_new_string ("error_me_out!"));
      xdbus_proxy_set_cached_property (proxy, "y", value);
      xvariant_unref (value);
      g_test_assert_expected_messages ();
    }

  /* this should work, however (since the type is correct) */
  xdbus_proxy_set_cached_property (proxy, "y", xvariant_new_byte (42));

  if (g_test_undefined ())
    {
      /* Try to get the value of a property where the type we expect is different from
       * what we have in our cache (e.g. what the service returned)
       */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                             "*Trying to get property i with type i but according to the expected interface the type is u*");
      value = xdbus_proxy_get_cached_property (proxy, "i");
      g_test_assert_expected_messages ();
    }

  /* Even if a property does not exist in expected_interface, looking it
   * up, or setting it, should never fail. Because it could be that the
   * property has been added to the service but the xdbus_interface_info_t*
   * passed to xdbus_proxy_set_interface_info() just haven't been updated.
   *
   * See https://bugzilla.gnome.org/show_bug.cgi?id=660886
   */
  value = xdbus_proxy_get_cached_property (proxy, "d");
  g_assert_nonnull (value);
  g_assert_true (xvariant_is_of_type (value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat (xvariant_get_double (value), ==, 7.5);
  xvariant_unref (value);
  /* update it via the cached property... */
  xdbus_proxy_set_cached_property (proxy, "d", xvariant_new_double (75.0));
  /* ... and finally check that it has changed */
  value = xdbus_proxy_get_cached_property (proxy, "d");
  g_assert_nonnull (value);
  g_assert_true (xvariant_is_of_type (value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat (xvariant_get_double (value), ==, 75.0);
  xvariant_unref (value);
  /* now update it via the D-Bus interface... */
  error = NULL;
  value = xdbus_proxy_call_sync (proxy, "FrobSetProperty",
                                  xvariant_new ("(sv)", "d", xvariant_new_double (85.0)),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (value);
  g_assert_cmpstr (xvariant_get_type_string (value), ==, "()");
  xvariant_unref (value);
  /* ...ensure we receive the ::PropertiesChanged signal... */
  _g_assert_signal_received (proxy, "g-properties-changed");
  /* ... and finally check that it has changed */
  value = xdbus_proxy_get_cached_property (proxy, "d");
  g_assert_nonnull (value);
  g_assert_true (xvariant_is_of_type (value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat (xvariant_get_double (value), ==, 85.0);
  xvariant_unref (value);
}

static void
test_basic (xdbus_proxy_t *proxy)
{
  xdbus_connection_t *connection;
  xdbus_connection_t *conn;
  xdbus_proxy_flags_t flags;
  xdbus_interface_info_t *info;
  xchar_t *name;
  xchar_t *path;
  xchar_t *interface;
  xint_t timeout;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  g_assert_true (xdbus_proxy_get_connection (proxy) == connection);
  g_assert_null (xdbus_proxy_get_interface_info (proxy));
  g_assert_cmpstr (xdbus_proxy_get_name (proxy), ==, "com.example.TestService");
  g_assert_cmpstr (xdbus_proxy_get_object_path (proxy), ==, "/com/example/test_object_t");
  g_assert_cmpstr (xdbus_proxy_get_interface_name (proxy), ==, "com.example.Frob");
  g_assert_cmpint (xdbus_proxy_get_default_timeout (proxy), ==, -1);

  xobject_get (proxy,
                "g-connection", &conn,
                "g-interface-info", &info,
                "g-flags", &flags,
                "g-name", &name,
                "g-object-path", &path,
                "g-interface-name", &interface,
                "g-default-timeout", &timeout,
                NULL);

  g_assert_true (conn == connection);
  g_assert_null (info);
  g_assert_cmpint (flags, ==, xdbus_proxy_get_flags (proxy));
  g_assert_cmpstr (name, ==, "com.example.TestService");
  g_assert_cmpstr (path, ==, "/com/example/test_object_t");
  g_assert_cmpstr (interface, ==, "com.example.Frob");
  g_assert_cmpint (timeout, ==, -1);

  xobject_unref (conn);
  g_free (name);
  g_free (path);
  g_free (interface);

  xobject_unref (connection);
}

static void
name_disappeared_cb (xdbus_connection_t *connection,
                     const xchar_t     *name,
                     xpointer_t         user_data)
{
  xboolean_t *name_disappeared = user_data;
  *name_disappeared = TRUE;
  xmain_context_wakeup (NULL);
}

static void
kill_test_service (xdbus_connection_t *connection)
{
#ifdef G_OS_UNIX
  xuint_t pid;
  xvariant_t *ret;
  xerror_t *error = NULL;
  const xchar_t *name = "com.example.TestService";
  xuint_t watch_id;
  xboolean_t name_disappeared = FALSE;

  ret = xdbus_connection_call_sync (connection,
                                     "org.freedesktop.DBus",
                                     "/org/freedesktop/DBus",
                                     "org.freedesktop.DBus",
                                     "GetConnectionUnixProcessID",
                                     xvariant_new ("(s)", name),
                                     NULL,
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &error);
  xvariant_get (ret, "(u)", &pid);
  xvariant_unref (ret);

  /* Watch the name and wait until it???s disappeared. */
  watch_id = g_bus_watch_name_on_connection (connection, name,
                                             G_BUS_NAME_WATCHER_FLAGS_NONE,
                                             NULL, name_disappeared_cb,
                                             &name_disappeared, NULL);
  kill (pid, SIGTERM);

  while (!name_disappeared)
    xmain_context_iteration (NULL, TRUE);

  g_bus_unwatch_name (watch_id);
#else
  g_warning ("Can't kill com.example.TestService");
#endif
}

static void
test_proxy_with_flags (xdbus_proxy_flags_t flags)
{
  xdbus_proxy_t *proxy;
  xdbus_connection_t *connection;
  xerror_t *error;
  xchar_t *owner;

  error = NULL;
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                               NULL,
                               &error);
  g_assert_no_error (error);
  error = NULL;
  proxy = xdbus_proxy_new_sync (connection,
                                 flags,
                                 NULL,                      /* xdbus_interface_info_t */
                                 "com.example.TestService", /* name */
                                 "/com/example/test_object_t", /* object path */
                                 "com.example.Frob",        /* interface */
                                 NULL, /* xcancellable_t */
                                 &error);
  g_assert_no_error (error);

  /* this is safe; we explicitly kill the service later on */
  g_assert_true (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  _g_assert_property_notify (proxy, "g-name-owner");

  test_basic (proxy);
  test_methods (proxy);
  test_properties (proxy);
  test_signals (proxy);
  test_expected_interface (proxy);

  kill_test_service (connection);

  owner = xdbus_proxy_get_name_owner (proxy);
  g_assert_null (owner);
  g_free (owner);

  xobject_unref (proxy);
  xobject_unref (connection);
}

static void
test_proxy (void)
{
  test_proxy_with_flags (G_DBUS_PROXY_FLAGS_NONE);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
proxy_ready (xobject_t      *source,
             xasync_result_t *result,
             xpointer_t      user_data)
{
  xdbus_proxy_t *proxy;
  xerror_t *error;
  xchar_t *owner;

  error = NULL;
  proxy = xdbus_proxy_new_for_bus_finish (result, &error);
  g_assert_no_error (error);

  owner = xdbus_proxy_get_name_owner (proxy);
  g_assert_null (owner);
  g_free (owner);

  /* this is safe; we explicitly kill the service later on */
  g_assert_true (g_spawn_command_line_async (g_test_get_filename (G_TEST_BUILT, "gdbus-testserver", NULL), NULL));

  _g_assert_property_notify (proxy, "g-name-owner");

  test_basic (proxy);
  test_methods (proxy);
  test_properties (proxy);
  test_signals (proxy);
  test_expected_interface (proxy);

  kill_test_service (xdbus_proxy_get_connection (proxy));
  xobject_unref (proxy);
  xmain_loop_quit (loop);
}

static xboolean_t
fail_test (xpointer_t user_data)
{
  g_assert_not_reached ();
}

static void
test_async (void)
{
  xuint_t id;

  xdbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,                      /* xdbus_interface_info_t */
                            "com.example.TestService", /* name */
                            "/com/example/test_object_t", /* object path */
                            "com.example.Frob",        /* interface */
                            NULL, /* xcancellable_t */
                            proxy_ready,
                            NULL);

  id = g_timeout_add (10000, fail_test, NULL);
  xmain_loop_run (loop);

  xsource_remove (id);
}

static void
test_no_properties (void)
{
  xdbus_proxy_t *proxy;
  xerror_t *error;

  error = NULL;
  proxy = xdbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                         NULL,                      /* xdbus_interface_info_t */
                                         "com.example.TestService", /* name */
                                         "/com/example/test_object_t", /* object path */
                                         "com.example.Frob",        /* interface */
                                         NULL, /* xcancellable_t */
                                         &error);
  g_assert_no_error (error);

  test_properties (proxy);

  xobject_unref (proxy);
}

static void
check_error (xobject_t      *source,
             xasync_result_t *result,
             xpointer_t      user_data)
{
  xerror_t *error = NULL;
  xvariant_t *reply;

  reply = xdbus_proxy_call_finish (G_DBUS_PROXY (source), result, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_assert_null (reply);
  xerror_free (error);

  xmain_loop_quit (loop);
}

static void
test_wellknown_noauto (void)
{
  xerror_t *error = NULL;
  xdbus_proxy_t *proxy;
  xuint_t id;

  proxy = xdbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                         NULL, "some.name.that.does.not.exist",
                                         "/", "some.interface", NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (proxy);

  xdbus_proxy_call (proxy, "method", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, check_error, NULL);
  id = g_timeout_add (10000, fail_test, NULL);
  xmain_loop_run (loop);
  xobject_unref (proxy);
  xsource_remove (id);
}

typedef enum {
  ADD_MATCH,
  REMOVE_MATCH,
} AddOrRemove;

static void
add_or_remove_match_rule (xdbus_connection_t *connection,
                          AddOrRemove      add_or_remove,
                          xvariant_t        *match_rule)
{
  xdbus_message_t *message = NULL;
  xerror_t *error = NULL;

  message = xdbus_message_new_method_call ("org.freedesktop.DBus", /* name */
                                            "/org/freedesktop/DBus", /* path */
                                            "org.freedesktop.DBus", /* interface */
                                            (add_or_remove == ADD_MATCH) ? "AddMatch" : "RemoveMatch");
  xdbus_message_set_body (message, match_rule);
  xdbus_connection_send_message (connection,
                                  message,
                                  G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                  NULL,
                                  &error);
  g_assert_no_error (error);
  g_clear_object (&message);
}

static void
test_proxy_no_match_rule (void)
{
  xdbus_connection_t *connection = NULL;
  xvariant_t *match_rule = NULL;

  g_test_summary ("test_t that G_DBUS_PROXY_FLAGS_NO_MATCH_RULE works");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/1109");

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  /* Add a custom match rule which matches everything. */
  match_rule = xvariant_ref_sink (xvariant_new ("(s)", "type='signal'"));
  add_or_remove_match_rule (connection, ADD_MATCH, match_rule);

  /* Run the tests. */
  test_proxy_with_flags (G_DBUS_PROXY_FLAGS_NO_MATCH_RULE);

  /* Remove the match rule again. */
  add_or_remove_match_rule (connection, REMOVE_MATCH, match_rule);

  g_clear_pointer (&match_rule, xvariant_unref);
  g_clear_object (&connection);
}

int
main (int   argc,
      char *argv[])
{
  xint_t ret;
  xdbus_node_info_t *introspection_data = NULL;

  g_test_init (&argc, &argv, NULL);

  introspection_data = g_dbus_node_info_new_for_xml (frob_dbus_interface_xml, NULL);
  g_assert_nonnull (introspection_data);
  frob_dbus_interface_info = introspection_data->interfaces[0];

  /* all the tests rely on a shared main loop */
  loop = xmain_loop_new (NULL, FALSE);

  g_test_add_func ("/gdbus/proxy", test_proxy);
  g_test_add_func ("/gdbus/proxy/no-properties", test_no_properties);
  g_test_add_func ("/gdbus/proxy/wellknown-noauto", test_wellknown_noauto);
  g_test_add_func ("/gdbus/proxy/async", test_async);
  g_test_add_func ("/gdbus/proxy/no-match-rule", test_proxy_no_match_rule);

  ret = session_bus_run();

  g_dbus_node_info_unref (introspection_data);
  xmain_loop_unref (loop);

  return ret;
}
