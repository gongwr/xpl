#include <stdlib.h>
#include <locale.h>
#include <libintl.h>
#include <unistd.h>
#include <sys/types.h>
#include <gio/gio.h>
#include <gstdio.h>
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include "testenum.h"

static const xchar_t *locale_dir = ".";

static xboolean_t backend_set;

/* These tests rely on the schemas in org.gtk.test.gschema.xml
 * to be compiled and installed in the same directory.
 */

typedef struct
{
  xchar_t *tmp_dir;
} Fixture;

static void
setup (Fixture       *fixture,
       xconstpointer  user_data)
{
  xerror_t *error = NULL;

  fixture->tmp_dir = g_dir_make_tmp ("gio-test-gsettings_XXXXXX", &error);
  g_assert_no_error (error);

  g_test_message ("Using temporary directory: %s", fixture->tmp_dir);
}

static void
teardown (Fixture       *fixture,
          xconstpointer  user_data)
{
  g_assert_no_errno (g_rmdir (fixture->tmp_dir));
  g_clear_pointer (&fixture->tmp_dir, g_free);
}

static void
check_and_free (xvariant_t    *value,
                const xchar_t *expected)
{
  xchar_t *printed;

  printed = xvariant_print (value, TRUE);
  g_assert_cmpstr (printed, ==, expected);
  g_free (printed);

  xvariant_unref (value);
}

/* Wrapper around g_assert_cmpstr() which gets a setting from a #xsettings_t
 * using g_settings_get(). */
#define settings_assert_cmpstr(settings, key, op, expected_value) G_STMT_START { \
  xchar_t *__str; \
  g_settings_get ((settings), (key), "s", &__str); \
  g_assert_cmpstr (__str, op, (expected_value)); \
  g_free (__str); \
} G_STMT_END


/* Just to get warmed up: Read and set a string, and
 * verify that can read the changed string back
 */
static void
test_basic (void)
{
  xchar_t *str = NULL;
  xobject_t *b;
  xchar_t *path;
  xboolean_t has_unapplied;
  xboolean_t delay_apply;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test");

  xobject_get (settings,
                "schema-id", &str,
                "backend", &b,
                "path", &path,
                "has-unapplied", &has_unapplied,
                "delay-apply", &delay_apply,
                NULL);
  g_assert_cmpstr (str, ==, "org.gtk.test");
  g_assert_nonnull (b);
  g_assert_cmpstr (path, ==, "/tests/");
  g_assert_false (has_unapplied);
  g_assert_false (delay_apply);
  g_free (str);
  xobject_unref (b);
  g_free (path);

  settings_assert_cmpstr (settings, "greeting", ==, "Hello, earthlings");

  g_settings_set (settings, "greeting", "s", "goodbye world");
  settings_assert_cmpstr (settings, "greeting", ==, "goodbye world");

  if (!backend_set && g_test_undefined ())
    {
      xsettings_t *tmp_settings = g_settings_new ("org.gtk.test");

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*g_settings_set_value*expects type*");
      g_settings_set (tmp_settings, "greeting", "i", 555);
      g_test_assert_expected_messages ();

      xobject_unref (tmp_settings);
    }

  settings_assert_cmpstr (settings, "greeting", ==, "goodbye world");

  g_settings_reset (settings, "greeting");
  str = g_settings_get_string (settings, "greeting");
  g_assert_cmpstr (str, ==, "Hello, earthlings");
  g_free (str);

  g_settings_set (settings, "greeting", "s", "this is the end");
  xobject_unref (settings);
}

/* Check that we get an error when getting a key
 * that is not in the schema
 */
static void
test_unknown_key (void)
{
  if (!g_test_undefined ())
    return;

  if (g_test_subprocess ())
    {
      xsettings_t *settings;
      xvariant_t *value;

      settings = g_settings_new ("org.gtk.test");
      value = g_settings_get_value (settings, "no_such_key");

      g_assert_null (value);

      xobject_unref (settings);
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*does not contain*");
}

/* Check that we get an error when the schema
 * has not been installed
 */
static void
test_no_schema (void)
{
  if (!g_test_undefined ())
    return;

  if (g_test_subprocess ())
    {
      xsettings_t *settings;

      settings = g_settings_new ("no.such.schema");

      g_assert_null (settings);
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*Settings schema 'no.such.schema' is not installed*");
}

/* Check that we get an error when passing a type string
 * that does not match the schema
 */
static void
test_wrong_type (void)
{
  xsettings_t *settings;
  xchar_t *str = NULL;

  if (!g_test_undefined ())
    return;

  settings = g_settings_new ("org.gtk.test");

  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL,
                         "*given value has a type of*");
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL,
                         "*valid_format_string*");
  g_settings_get (settings, "greeting", "o", &str);
  g_test_assert_expected_messages ();

  g_assert_null (str);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*expects type 's'*");
  g_settings_set (settings, "greeting", "o", "/a/path");
  g_test_assert_expected_messages ();

  xobject_unref (settings);
}

/* Check errors with explicit paths */
static void
test_wrong_path (void)
{
  if (!g_test_undefined ())
    return;

  if (g_test_subprocess ())
    {
      xsettings_t *settings G_GNUC_UNUSED;

      settings = g_settings_new_with_path ("org.gtk.test", "/wrong-path/");
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*but path * specified by schema*");
}

static void
test_no_path (void)
{
  if (!g_test_undefined ())
    return;

  if (g_test_subprocess ())
    {
      xsettings_t *settings G_GNUC_UNUSED;

      settings = g_settings_new ("org.gtk.test.no-path");
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*attempting to create schema * without a path**");
}


/* Check that we can successfully read and set the full
 * range of all basic types
 */
static void
test_basic_types (void)
{
  xsettings_t *settings;
  xboolean_t b;
  xuint8_t byte;
  gint16 i16;
  xuint16_t u16;
  gint32 i32;
  xuint32_t u32;
  sint64_t i64;
  xuint64_t u64;
  xdouble_t d;
  xchar_t *str;

  settings = g_settings_new ("org.gtk.test.basic-types");

  g_settings_get (settings, "test-boolean", "b", &b);
  g_assert_cmpint (b, ==, 1);

  g_settings_set (settings, "test-boolean", "b", 0);
  g_settings_get (settings, "test-boolean", "b", &b);
  g_assert_cmpint (b, ==, 0);

  g_settings_get (settings, "test-byte", "y", &byte);
  g_assert_cmpint (byte, ==, 25);

  g_settings_set (settings, "test-byte", "y", G_MAXUINT8);
  g_settings_get (settings, "test-byte", "y", &byte);
  g_assert_cmpint (byte, ==, G_MAXUINT8);

  g_settings_get (settings, "test-int16", "n", &i16);
  g_assert_cmpint (i16, ==, -1234);

  g_settings_set (settings, "test-int16", "n", G_MININT16);
  g_settings_get (settings, "test-int16", "n", &i16);
  g_assert_cmpint (i16, ==, G_MININT16);

  g_settings_set (settings, "test-int16", "n", G_MAXINT16);
  g_settings_get (settings, "test-int16", "n", &i16);
  g_assert_cmpint (i16, ==, G_MAXINT16);

  g_settings_get (settings, "test-uint16", "q", &u16);
  g_assert_cmpuint (u16, ==, 1234);

  g_settings_set (settings, "test-uint16", "q", G_MAXUINT16);
  g_settings_get (settings, "test-uint16", "q", &u16);
  g_assert_cmpuint (u16, ==, G_MAXUINT16);

  g_settings_get (settings, "test-int32", "i", &i32);
  g_assert_cmpint (i32, ==, -123456);

  g_settings_set (settings, "test-int32", "i", G_MININT32);
  g_settings_get (settings, "test-int32", "i", &i32);
  g_assert_cmpint (i32, ==, G_MININT32);

  g_settings_set (settings, "test-int32", "i", G_MAXINT32);
  g_settings_get (settings, "test-int32", "i", &i32);
  g_assert_cmpint (i32, ==, G_MAXINT32);

  g_settings_get (settings, "test-uint32", "u", &u32);
  g_assert_cmpuint (u32, ==, 123456);

  g_settings_set (settings, "test-uint32", "u", G_MAXUINT32);
  g_settings_get (settings, "test-uint32", "u", &u32);
  g_assert_cmpuint (u32, ==, G_MAXUINT32);

  g_settings_get (settings, "test-int64", "x", &i64);
  g_assert_cmpuint (i64, ==, -123456789);

  g_settings_set (settings, "test-int64", "x", G_MININT64);
  g_settings_get (settings, "test-int64", "x", &i64);
  g_assert_cmpuint (i64, ==, G_MININT64);

  g_settings_set (settings, "test-int64", "x", G_MAXINT64);
  g_settings_get (settings, "test-int64", "x", &i64);
  g_assert_cmpuint (i64, ==, G_MAXINT64);

  g_settings_get (settings, "test-uint64", "t", &u64);
  g_assert_cmpuint (u64, ==, 123456789);

  g_settings_set (settings, "test-uint64", "t", G_MAXUINT64);
  g_settings_get (settings, "test-uint64", "t", &u64);
  g_assert_cmpuint (u64, ==, G_MAXUINT64);

  g_settings_get (settings, "test-double", "d", &d);
  g_assert_cmpfloat (d, ==, 123.456);

  g_settings_set (settings, "test-double", "d", G_MINDOUBLE);
  g_settings_get (settings, "test-double", "d", &d);
  g_assert_cmpfloat (d, ==, G_MINDOUBLE);

  g_settings_set (settings, "test-double", "d", G_MAXDOUBLE);
  g_settings_get (settings, "test-double", "d", &d);
  g_assert_cmpfloat (d, ==, G_MAXDOUBLE);

  settings_assert_cmpstr (settings, "test-string", ==, "a string, it seems");

  g_settings_get (settings, "test-objectpath", "o", &str);
  g_assert_cmpstr (str, ==, "/a/object/path");
  xobject_unref (settings);
  g_free (str);
  str = NULL;
}

/* Check that we can read an set complex types like
 * tuples, arrays and dictionaries
 */
static void
test_complex_types (void)
{
  xsettings_t *settings;
  xchar_t *s;
  xint_t i1, i2;
  xvariant_iter_t *iter = NULL;
  xvariant_t *v = NULL;

  settings = g_settings_new ("org.gtk.test.complex-types");

  g_settings_get (settings, "test-tuple", "(s(ii))", &s, &i1, &i2);
  g_assert_cmpstr (s, ==, "one");
  g_assert_cmpint (i1,==, 2);
  g_assert_cmpint (i2,==, 3);
  g_free (s) ;
  s = NULL;

  g_settings_set (settings, "test-tuple", "(s(ii))", "none", 0, 0);
  g_settings_get (settings, "test-tuple", "(s(ii))", &s, &i1, &i2);
  g_assert_cmpstr (s, ==, "none");
  g_assert_cmpint (i1,==, 0);
  g_assert_cmpint (i2,==, 0);
  g_free (s);
  s = NULL;

  g_settings_get (settings, "test-array", "ai", &iter);
  g_assert_cmpint (xvariant_iter_n_children (iter), ==, 6);
  g_assert_true (xvariant_iter_next (iter, "i", &i1));
  g_assert_cmpint (i1, ==, 0);
  g_assert_true (xvariant_iter_next (iter, "i", &i1));
  g_assert_cmpint (i1, ==, 1);
  g_assert_true (xvariant_iter_next (iter, "i", &i1));
  g_assert_cmpint (i1, ==, 2);
  g_assert_true (xvariant_iter_next (iter, "i", &i1));
  g_assert_cmpint (i1, ==, 3);
  g_assert_true (xvariant_iter_next (iter, "i", &i1));
  g_assert_cmpint (i1, ==, 4);
  g_assert_true (xvariant_iter_next (iter, "i", &i1));
  g_assert_cmpint (i1, ==, 5);
  g_assert_false (xvariant_iter_next (iter, "i", &i1));
  xvariant_iter_free (iter);

  g_settings_get (settings, "test-dict", "a{sau}", &iter);
  g_assert_cmpint (xvariant_iter_n_children (iter), ==, 2);
  g_assert_true (xvariant_iter_next (iter, "{&s@au}", &s, &v));
  g_assert_cmpstr (s, ==, "AC");
  g_assert_cmpstr ((char *)xvariant_get_type (v), ==, "au");
  xvariant_unref (v);
  g_assert_true (xvariant_iter_next (iter, "{&s@au}", &s, &v));
  g_assert_cmpstr (s, ==, "IV");
  g_assert_cmpstr ((char *)xvariant_get_type (v), ==, "au");
  xvariant_unref (v);
  xvariant_iter_free (iter);

  v = g_settings_get_value (settings, "test-dict");
  g_assert_cmpstr ((char *)xvariant_get_type (v), ==, "a{sau}");
  xvariant_unref (v);

  xobject_unref (settings);
}

static xboolean_t changed_cb_called;

static void
changed_cb (xsettings_t   *settings,
            const xchar_t *key,
            xpointer_t     data)
{
  changed_cb_called = TRUE;

  g_assert_cmpstr (key, ==, data);
}

/* test_t that basic change notification with the changed signal works.
 */
static void
test_changes (void)
{
  xsettings_t *settings;
  xsettings_t *settings2;

  settings = g_settings_new ("org.gtk.test");

  xsignal_connect (settings, "changed",
                    G_CALLBACK (changed_cb), "greeting");

  changed_cb_called = FALSE;

  g_settings_set (settings, "greeting", "s", "new greeting");
  g_assert_true (changed_cb_called);

  settings2 = g_settings_new ("org.gtk.test");

  changed_cb_called = FALSE;

  g_settings_set (settings2, "greeting", "s", "hi");
  g_assert_true (changed_cb_called);

  xobject_unref (settings2);
  xobject_unref (settings);
}

static xboolean_t changed_cb_called2;

static void
changed_cb2 (xsettings_t   *settings,
             const xchar_t *key,
             xpointer_t     data)
{
  xboolean_t *p = data;

  *p = TRUE;
}

/* test_t that changes done to a delay-mode instance
 * don't appear to the outside world until apply. Also
 * check that we get change notification when they are
 * applied.
 * Also test that the has-unapplied property is properly
 * maintained.
 */
static void
test_delay_apply (void)
{
  xsettings_t *settings;
  xsettings_t *settings2;
  xboolean_t writable;
  xvariant_t *v;
  const xchar_t *s;

  settings = g_settings_new ("org.gtk.test");
  settings2 = g_settings_new ("org.gtk.test");

  g_settings_set (settings2, "greeting", "s", "top o' the morning");

  changed_cb_called = FALSE;
  changed_cb_called2 = FALSE;

  xsignal_connect (settings, "changed",
                    G_CALLBACK (changed_cb2), &changed_cb_called);
  xsignal_connect (settings2, "changed",
                    G_CALLBACK (changed_cb2), &changed_cb_called2);

  g_settings_delay (settings);

  g_settings_set (settings, "greeting", "s", "greetings from test_delay_apply");

  g_assert_true (changed_cb_called);
  g_assert_false (changed_cb_called2);

  /* Try resetting the key and ensure a notification is emitted on the delayed #xsettings_t object. */
  changed_cb_called = FALSE;
  changed_cb_called2 = FALSE;

  g_settings_reset (settings, "greeting");

  g_assert_true (changed_cb_called);
  g_assert_false (changed_cb_called2);

  /* Locally change the greeting again. */
  changed_cb_called = FALSE;
  changed_cb_called2 = FALSE;

  g_settings_set (settings, "greeting", "s", "greetings from test_delay_apply");

  g_assert_true (changed_cb_called);
  g_assert_false (changed_cb_called2);

  writable = g_settings_is_writable (settings, "greeting");
  g_assert_true (writable);

  settings_assert_cmpstr (settings, "greeting", ==, "greetings from test_delay_apply");

  v = g_settings_get_user_value (settings, "greeting");
  s = xvariant_get_string (v, NULL);
  g_assert_cmpstr (s, ==, "greetings from test_delay_apply");
  xvariant_unref (v);

  settings_assert_cmpstr (settings2, "greeting", ==, "top o' the morning");

  g_assert_true (g_settings_get_has_unapplied (settings));
  g_assert_false (g_settings_get_has_unapplied (settings2));

  changed_cb_called = FALSE;
  changed_cb_called2 = FALSE;

  g_settings_apply (settings);

  g_assert_false (changed_cb_called);
  g_assert_true (changed_cb_called2);

  settings_assert_cmpstr (settings, "greeting", ==, "greetings from test_delay_apply");
  settings_assert_cmpstr (settings2, "greeting", ==, "greetings from test_delay_apply");

  g_assert_false (g_settings_get_has_unapplied (settings));
  g_assert_false (g_settings_get_has_unapplied (settings2));

  g_settings_reset (settings, "greeting");
  g_settings_apply (settings);

  settings_assert_cmpstr (settings, "greeting", ==, "Hello, earthlings");

  xobject_unref (settings2);
  xobject_unref (settings);
}

/* test_t that reverting unapplied changes in a delay-apply
 * settings instance works.
 */
static void
test_delay_revert (void)
{
  xsettings_t *settings;
  xsettings_t *settings2;

  settings = g_settings_new ("org.gtk.test");
  settings2 = g_settings_new ("org.gtk.test");

  g_settings_set (settings2, "greeting", "s", "top o' the morning");

  settings_assert_cmpstr (settings, "greeting", ==, "top o' the morning");

  g_settings_delay (settings);

  g_settings_set (settings, "greeting", "s", "greetings from test_delay_revert");

  settings_assert_cmpstr (settings, "greeting", ==, "greetings from test_delay_revert");
  settings_assert_cmpstr (settings2, "greeting", ==, "top o' the morning");

  g_assert_true (g_settings_get_has_unapplied (settings));

  g_settings_revert (settings);

  g_assert_false (g_settings_get_has_unapplied (settings));

  settings_assert_cmpstr (settings, "greeting", ==, "top o' the morning");
  settings_assert_cmpstr (settings2, "greeting", ==, "top o' the morning");

  xobject_unref (settings2);
  xobject_unref (settings);
}

static void
test_delay_child (void)
{
  xsettings_t *base;
  xsettings_t *settings;
  xsettings_t *child;
  xuint8_t byte;
  xboolean_t delay;

  base = g_settings_new ("org.gtk.test.basic-types");
  g_settings_set (base, "test-byte", "y", 36);

  settings = g_settings_new ("org.gtk.test");
  g_settings_delay (settings);
  xobject_get (settings, "delay-apply", &delay, NULL);
  g_assert_true (delay);

  child = g_settings_get_child (settings, "basic-types");
  g_assert_nonnull (child);

  xobject_get (child, "delay-apply", &delay, NULL);
  g_assert_true (delay);

  g_settings_get (child, "test-byte", "y", &byte);
  g_assert_cmpuint (byte, ==, 36);

  g_settings_set (child, "test-byte", "y", 42);

  /* make sure the child was delayed too */
  g_settings_get (base, "test-byte", "y", &byte);
  g_assert_cmpuint (byte, ==, 36);

  /* apply the child and the changes should be saved */
  g_settings_apply (child);
  g_settings_get (base, "test-byte", "y", &byte);
  g_assert_cmpuint (byte, ==, 42);

  xobject_unref (child);
  xobject_unref (settings);
  xobject_unref (base);
}

static void
test_delay_reset_key (void)
{
  xsettings_t *direct_settings = NULL, *delayed_settings = NULL;

  g_test_summary ("test_t that resetting a key on a delayed settings instance works");

  delayed_settings = g_settings_new ("org.gtk.test");
  direct_settings = g_settings_new ("org.gtk.test");

  g_settings_set (direct_settings, "greeting", "s", "ey up");

  settings_assert_cmpstr (delayed_settings, "greeting", ==, "ey up");

  /* Set up a delayed settings backend. */
  g_settings_delay (delayed_settings);

  g_settings_set (delayed_settings, "greeting", "s", "how do");

  settings_assert_cmpstr (delayed_settings, "greeting", ==, "how do");
  settings_assert_cmpstr (direct_settings, "greeting", ==, "ey up");

  g_assert_true (g_settings_get_has_unapplied (delayed_settings));

  g_settings_reset (delayed_settings, "greeting");

  /* There are still unapplied settings, because the reset is resetting to the
   * value from the schema, not the value from @direct_settings. */
  g_assert_true (g_settings_get_has_unapplied (delayed_settings));

  settings_assert_cmpstr (delayed_settings, "greeting", ==, "Hello, earthlings");
  settings_assert_cmpstr (direct_settings, "greeting", ==, "ey up");

  /* Apply the settings changes (i.e. the reset). */
  g_settings_apply (delayed_settings);

  g_assert_false (g_settings_get_has_unapplied (delayed_settings));

  settings_assert_cmpstr (delayed_settings, "greeting", ==, "Hello, earthlings");
  settings_assert_cmpstr (direct_settings, "greeting", ==, "Hello, earthlings");

  xobject_unref (direct_settings);
  xobject_unref (delayed_settings);
}

static void
keys_changed_cb (xsettings_t    *settings,
                 const xquark *keys,
                 xint_t          n_keys)
{
  g_assert_cmpint (n_keys, ==, 2);

  g_assert_true ((keys[0] == g_quark_from_static_string ("greeting") &&
                  keys[1] == g_quark_from_static_string ("farewell")) ||
                 (keys[1] == g_quark_from_static_string ("greeting") &&
                  keys[0] == g_quark_from_static_string ("farewell")));

  settings_assert_cmpstr (settings, "greeting", ==, "greetings from test_atomic");
  settings_assert_cmpstr (settings, "farewell", ==, "atomic bye-bye");
}

/* Check that delay-applied changes appear atomically.
 * More specifically, verify that all changed keys appear
 * with their new value while handling the change-event signal.
 */
static void
test_atomic (void)
{
  xsettings_t *settings;
  xsettings_t *settings2;

  settings = g_settings_new ("org.gtk.test");
  settings2 = g_settings_new ("org.gtk.test");

  g_settings_set (settings2, "greeting", "s", "top o' the morning");

  changed_cb_called = FALSE;
  changed_cb_called2 = FALSE;

  xsignal_connect (settings2, "change-event",
                    G_CALLBACK (keys_changed_cb), NULL);

  g_settings_delay (settings);

  g_settings_set (settings, "greeting", "s", "greetings from test_atomic");
  g_settings_set (settings, "farewell", "s", "atomic bye-bye");

  g_settings_apply (settings);

  settings_assert_cmpstr (settings, "greeting", ==, "greetings from test_atomic");
  settings_assert_cmpstr (settings, "farewell", ==, "atomic bye-bye");
  settings_assert_cmpstr (settings2, "greeting", ==, "greetings from test_atomic");
  settings_assert_cmpstr (settings2, "farewell", ==, "atomic bye-bye");

  xobject_unref (settings2);
  xobject_unref (settings);
}

/* On Windows the interaction between the C library locale and libintl
 * (from GNU gettext) is not like on POSIX, so just skip these tests
 * for now.
 *
 * There are several issues:
 *
 * 1) The C library doesn't use LC_MESSAGES, that is implemented only
 * in libintl (defined in its <libintl.h>).
 *
 * 2) The locale names that setlocale() accepts and returns aren't in
 * the "de_DE" style, but like "German_Germany".
 *
 * 3) libintl looks at the Win32 thread locale and not the C library
 * locale. (And even if libintl would use the C library's locale, as
 * there are several alternative C library DLLs, libintl might be
 * linked to a different one than the application code, so they
 * wouldn't have the same C library locale anyway.)
 */

/* test_t that translations work for schema defaults.
 *
 * This test relies on the de.po file in the same directory
 * to be compiled into ./de/LC_MESSAGES/test.mo
 */
static void
test_l10n (void)
{
  xsettings_t *settings;
  xchar_t *str;
  xchar_t *locale;

  bindtextdomain ("test", locale_dir);
  bind_textdomain_codeset ("test", "UTF-8");

  locale = xstrdup (setlocale (LC_MESSAGES, NULL));

  settings = g_settings_new ("org.gtk.test.localized");

  g_setenv ("LC_MESSAGES", "C", TRUE);
  setlocale (LC_MESSAGES, "C");
  str = g_settings_get_string (settings, "error-message");
  g_setenv ("LC_MESSAGES", locale, TRUE);
  setlocale (LC_MESSAGES, locale);

  g_assert_cmpstr (str, ==, "Unnamed");
  g_free (str);
  str = NULL;

  g_setenv ("LC_MESSAGES", "de_DE.UTF-8", TRUE);
  setlocale (LC_MESSAGES, "de_DE.UTF-8");
  /* Only do the test if translation is actually working... */
  if (xstr_equal (dgettext ("test", "\"Unnamed\""), "\"Unbenannt\""))
    {
      str = g_settings_get_string (settings, "error-message");

      g_assert_cmpstr (str, ==, "Unbenannt");
      g_free (str);
      str = NULL;
    }
  else
    g_printerr ("warning: translation is not working... skipping test. ");

  g_setenv ("LC_MESSAGES", locale, TRUE);
  setlocale (LC_MESSAGES, locale);
  g_free (locale);
  xobject_unref (settings);
}

/* test_t that message context works as expected with translated
 * schema defaults. Also, verify that non-ASCII UTF-8 content
 * works.
 *
 * This test relies on the de.po file in the same directory
 * to be compiled into ./de/LC_MESSAGES/test.mo
 */
static void
test_l10n_context (void)
{
  xsettings_t *settings;
  xchar_t *str;
  xchar_t *locale;

  bindtextdomain ("test", locale_dir);
  bind_textdomain_codeset ("test", "UTF-8");

  locale = xstrdup (setlocale (LC_MESSAGES, NULL));

  settings = g_settings_new ("org.gtk.test.localized");

  g_setenv ("LC_MESSAGES", "C", TRUE);
  setlocale (LC_MESSAGES, "C");
  g_settings_get (settings, "backspace", "s", &str);
  g_setenv ("LC_MESSAGES", locale, TRUE);
  setlocale (LC_MESSAGES, locale);

  g_assert_cmpstr (str, ==, "BackSpace");
  g_free (str);
  str = NULL;

  g_setenv ("LC_MESSAGES", "de_DE.UTF-8", TRUE);
  setlocale (LC_MESSAGES, "de_DE.UTF-8");
  /* Only do the test if translation is actually working... */
  if (xstr_equal (dgettext ("test", "\"Unnamed\""), "\"Unbenannt\""))
    settings_assert_cmpstr (settings, "backspace", ==, "L??schen");
  else
    g_printerr ("warning: translation is not working... skipping test.  ");

  g_setenv ("LC_MESSAGES", locale, TRUE);
  setlocale (LC_MESSAGES, locale);
  g_free (locale);
  xobject_unref (settings);
}

enum
{
  PROP_0,
  PROP_BOOL,
  PROP_ANTI_BOOL,
  PROP_BYTE,
  PROP_INT16,
  PROP_UINT16,
  PROP_INT,
  PROP_UINT,
  PROP_INT64,
  PROP_UINT64,
  PROP_DOUBLE,
  PROP_STRING,
  PROP_NO_READ,
  PROP_NO_WRITE,
  PROP_STRV,
  PROP_ENUM,
  PROP_FLAGS
};

typedef struct
{
  xobject_t parent_instance;

  xboolean_t bool_prop;
  xboolean_t anti_bool_prop;
  gint8 byte_prop;
  xint_t int16_prop;
  xuint16_t uint16_prop;
  xint_t int_prop;
  xuint_t uint_prop;
  sint64_t int64_prop;
  xuint64_t uint64_prop;
  xdouble_t double_prop;
  xchar_t *string_prop;
  xchar_t *no_read_prop;
  xchar_t *no_write_prop;
  xchar_t **strv_prop;
  xuint_t enum_prop;
  xuint_t flags_prop;
} test_object_t;

typedef struct
{
  xobject_class_t parent_class;
} test_object_class_t;

static xtype_t test_object_get_type (void);
XDEFINE_TYPE (test_object, test_object, XTYPE_OBJECT)

static void
test_object_init (test_object_t *object)
{
}

static void
test_object_finalize (xobject_t *object)
{
  test_object_t *testo = (test_object_t*)object;
  xstrfreev (testo->strv_prop);
  g_free (testo->string_prop);
  XOBJECT_CLASS (test_object_parent_class)->finalize (object);
}

static void
test_object_get_property (xobject_t    *object,
                          xuint_t       prop_id,
                          xvalue_t     *value,
                          xparam_spec_t *pspec)
{
  test_object_t *test_object = (test_object_t *)object;

  switch (prop_id)
    {
    case PROP_BOOL:
      xvalue_set_boolean (value, test_object->bool_prop);
      break;
    case PROP_ANTI_BOOL:
      xvalue_set_boolean (value, test_object->anti_bool_prop);
      break;
    case PROP_BYTE:
      xvalue_set_schar (value, test_object->byte_prop);
      break;
    case PROP_UINT16:
      xvalue_set_uint (value, test_object->uint16_prop);
      break;
    case PROP_INT16:
      xvalue_set_int (value, test_object->int16_prop);
      break;
    case PROP_INT:
      xvalue_set_int (value, test_object->int_prop);
      break;
    case PROP_UINT:
      xvalue_set_uint (value, test_object->uint_prop);
      break;
    case PROP_INT64:
      xvalue_set_int64 (value, test_object->int64_prop);
      break;
    case PROP_UINT64:
      xvalue_set_uint64 (value, test_object->uint64_prop);
      break;
    case PROP_DOUBLE:
      xvalue_set_double (value, test_object->double_prop);
      break;
    case PROP_STRING:
      xvalue_set_string (value, test_object->string_prop);
      break;
    case PROP_NO_WRITE:
      xvalue_set_string (value, test_object->no_write_prop);
      break;
    case PROP_STRV:
      xvalue_set_boxed (value, test_object->strv_prop);
      break;
    case PROP_ENUM:
      xvalue_set_enum (value, test_object->enum_prop);
      break;
    case PROP_FLAGS:
      xvalue_set_flags (value, test_object->flags_prop);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
test_object_set_property (xobject_t      *object,
                          xuint_t         prop_id,
                          const xvalue_t *value,
                          xparam_spec_t   *pspec)
{
  test_object_t *test_object = (test_object_t *)object;

  switch (prop_id)
    {
    case PROP_BOOL:
      test_object->bool_prop = xvalue_get_boolean (value);
      break;
    case PROP_ANTI_BOOL:
      test_object->anti_bool_prop = xvalue_get_boolean (value);
      break;
    case PROP_BYTE:
      test_object->byte_prop = xvalue_get_schar (value);
      break;
    case PROP_INT16:
      test_object->int16_prop = xvalue_get_int (value);
      break;
    case PROP_UINT16:
      test_object->uint16_prop = xvalue_get_uint (value);
      break;
    case PROP_INT:
      test_object->int_prop = xvalue_get_int (value);
      break;
    case PROP_UINT:
      test_object->uint_prop = xvalue_get_uint (value);
      break;
    case PROP_INT64:
      test_object->int64_prop = xvalue_get_int64 (value);
      break;
    case PROP_UINT64:
      test_object->uint64_prop = xvalue_get_uint64 (value);
      break;
    case PROP_DOUBLE:
      test_object->double_prop = xvalue_get_double (value);
      break;
    case PROP_STRING:
      g_free (test_object->string_prop);
      test_object->string_prop = xvalue_dup_string (value);
      break;
    case PROP_NO_READ:
      g_free (test_object->no_read_prop);
      test_object->no_read_prop = xvalue_dup_string (value);
      break;
    case PROP_STRV:
      xstrfreev (test_object->strv_prop);
      test_object->strv_prop = xvalue_dup_boxed (value);
      break;
    case PROP_ENUM:
      test_object->enum_prop = xvalue_get_enum (value);
      break;
    case PROP_FLAGS:
      test_object->flags_prop = xvalue_get_flags (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static xtype_t
test_enum_get_type (void)
{
  static xsize_t define_type_id = 0;

  if (g_once_init_enter (&define_type_id))
    {
      static const xenum_value_t values[] = {
        { TEST_ENUM_FOO, "TEST_ENUM_FOO", "foo" },
        { TEST_ENUM_BAR, "TEST_ENUM_BAR", "bar" },
        { TEST_ENUM_BAZ, "TEST_ENUM_BAZ", "baz" },
        { TEST_ENUM_QUUX, "TEST_ENUM_QUUX", "quux" },
        { 0, NULL, NULL }
      };

      xtype_t type_id = xenum_register_static ("TestEnum", values);
      g_once_init_leave (&define_type_id, type_id);
    }

  return define_type_id;
}

static xtype_t
test_flags_get_type (void)
{
  static xsize_t define_type_id = 0;

  if (g_once_init_enter (&define_type_id))
    {
      static const xflags_value_t values[] = {
        { TEST_FLAGS_NONE, "TEST_FLAGS_NONE", "none" },
        { TEST_FLAGS_MOURNING, "TEST_FLAGS_MOURNING", "mourning" },
        { TEST_FLAGS_LAUGHING, "TEST_FLAGS_LAUGHING", "laughing" },
        { TEST_FLAGS_WALKING, "TEST_FLAGS_WALKING", "walking" },
        { 0, NULL, NULL }
      };

      xtype_t type_id = xflags_register_static ("TestFlags", values);
      g_once_init_leave (&define_type_id, type_id);
    }

  return define_type_id;
}

static void
test_object_class_init (test_object_class_t *class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (class);

  xobject_class->get_property = test_object_get_property;
  xobject_class->set_property = test_object_set_property;
  xobject_class->finalize = test_object_finalize;

  xobject_class_install_property (xobject_class, PROP_BOOL,
    xparam_spec_boolean ("bool", "", "", FALSE, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_ANTI_BOOL,
    xparam_spec_boolean ("anti-bool", "", "", FALSE, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_BYTE,
    xparam_spec_char ("byte", "", "", G_MININT8, G_MAXINT8, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_INT16,
    xparam_spec_int ("int16", "", "", -G_MAXINT16, G_MAXINT16, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_UINT16,
    xparam_spec_uint ("uint16", "", "", 0, G_MAXUINT16, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_INT,
    xparam_spec_int ("int", "", "", G_MININT, G_MAXINT, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_UINT,
    xparam_spec_uint ("uint", "", "", 0, G_MAXUINT, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_INT64,
    xparam_spec_int64 ("int64", "", "", G_MININT64, G_MAXINT64, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_UINT64,
    xparam_spec_uint64 ("uint64", "", "", 0, G_MAXUINT64, 0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_DOUBLE,
    xparam_spec_double ("double", "", "", -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_STRING,
    xparam_spec_string ("string", "", "", NULL, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_NO_WRITE,
    xparam_spec_string ("no-write", "", "", NULL, XPARAM_READABLE));
  xobject_class_install_property (xobject_class, PROP_NO_READ,
    xparam_spec_string ("no-read", "", "", NULL, XPARAM_WRITABLE));
  xobject_class_install_property (xobject_class, PROP_STRV,
    xparam_spec_boxed ("strv", "", "", XTYPE_STRV, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_ENUM,
    xparam_spec_enum ("enum", "", "", test_enum_get_type (), TEST_ENUM_FOO, XPARAM_READWRITE));
  xobject_class_install_property (xobject_class, PROP_FLAGS,
    xparam_spec_flags ("flags", "", "", test_flags_get_type (), TEST_FLAGS_NONE, XPARAM_READWRITE));
}

static test_object_t *
test_object_new (void)
{
  return (test_object_t*)xobject_new (test_object_get_type (), NULL);
}

/* test_t basic binding functionality for simple types.
 * Verify that with bidirectional bindings, changes on either side
 * are notified on the other end.
 */
static void
test_simple_binding (void)
{
  test_object_t *obj;
  xsettings_t *settings;
  xboolean_t b;
  xchar_t y;
  xint_t i;
  xuint_t u;
  gint16 n;
  xuint16_t q;
  xint_t n2;
  xuint_t q2;
  sint64_t i64;
  xuint64_t u64;
  xdouble_t d;
  xchar_t *s;
  xvariant_t *value;
  xchar_t **strv;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_bind (settings, "bool", obj, "bool", G_SETTINGS_BIND_DEFAULT);
  xobject_set (obj, "bool", TRUE, NULL);
  g_assert_cmpint (g_settings_get_boolean (settings, "bool"), ==, TRUE);

  g_settings_set_boolean (settings, "bool", FALSE);
  b = TRUE;
  xobject_get (obj, "bool", &b, NULL);
  g_assert_cmpint (b, ==, FALSE);

  g_settings_bind (settings, "anti-bool", obj, "anti-bool",
                   G_SETTINGS_BIND_INVERT_BOOLEAN);
  xobject_set (obj, "anti-bool", FALSE, NULL);
  g_assert_cmpint (g_settings_get_boolean (settings, "anti-bool"), ==, TRUE);

  g_settings_set_boolean (settings, "anti-bool", FALSE);
  b = FALSE;
  xobject_get (obj, "anti-bool", &b, NULL);
  g_assert_cmpint (b, ==, TRUE);

  g_settings_bind (settings, "byte", obj, "byte", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "byte", 123, NULL);
  y = 'c';
  g_settings_get (settings, "byte", "y", &y);
  g_assert_cmpint (y, ==, 123);

  g_settings_set (settings, "byte", "y", 54);
  y = 'c';
  xobject_get (obj, "byte", &y, NULL);
  g_assert_cmpint (y, ==, 54);

  g_settings_bind (settings, "int16", obj, "int16", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "int16", 1234, NULL);
  n = 4321;
  g_settings_get (settings, "int16", "n", &n);
  g_assert_cmpint (n, ==, 1234);

  g_settings_set (settings, "int16", "n", 4321);
  n2 = 1111;
  xobject_get (obj, "int16", &n2, NULL);
  g_assert_cmpint (n2, ==, 4321);

  g_settings_bind (settings, "uint16", obj, "uint16", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "uint16", (xuint16_t) G_MAXUINT16, NULL);
  q = 1111;
  g_settings_get (settings, "uint16", "q", &q);
  g_assert_cmpuint (q, ==, G_MAXUINT16);

  g_settings_set (settings, "uint16", "q", (xuint16_t) G_MAXINT16);
  q2 = 1111;
  xobject_get (obj, "uint16", &q2, NULL);
  g_assert_cmpuint (q2, ==, (xuint16_t) G_MAXINT16);

  g_settings_bind (settings, "int", obj, "int", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "int", 12345, NULL);
  g_assert_cmpint (g_settings_get_int (settings, "int"), ==, 12345);

  g_settings_set_int (settings, "int", 54321);
  i = 1111;
  xobject_get (obj, "int", &i, NULL);
  g_assert_cmpint (i, ==, 54321);

  g_settings_bind (settings, "uint", obj, "uint", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "uint", 12345, NULL);
  g_assert_cmpuint (g_settings_get_uint (settings, "uint"), ==, 12345);

  g_settings_set_uint (settings, "uint", 54321);
  u = 1111;
  xobject_get (obj, "uint", &u, NULL);
  g_assert_cmpuint (u, ==, 54321);

  g_settings_bind (settings, "uint64", obj, "uint64", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "uint64", (xuint64_t) 12345, NULL);
  g_assert_cmpuint (g_settings_get_uint64 (settings, "uint64"), ==, 12345);

  g_settings_set_uint64 (settings, "uint64", 54321);
  u64 = 1111;
  xobject_get (obj, "uint64", &u64, NULL);
  g_assert_cmpuint (u64, ==, 54321);

  g_settings_bind (settings, "int64", obj, "int64", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "int64", (sint64_t) G_MAXINT64, NULL);
  i64 = 1111;
  g_settings_get (settings, "int64", "x", &i64);
  g_assert_cmpint (i64, ==, G_MAXINT64);

  g_settings_set (settings, "int64", "x", (sint64_t) G_MININT64);
  i64 = 1111;
  xobject_get (obj, "int64", &i64, NULL);
  g_assert_cmpint (i64, ==, G_MININT64);

  g_settings_bind (settings, "uint64", obj, "uint64", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "uint64", (xuint64_t) G_MAXUINT64, NULL);
  u64 = 1111;
  g_settings_get (settings, "uint64", "t", &u64);
  g_assert_cmpuint (u64, ==, G_MAXUINT64);

  g_settings_set (settings, "uint64", "t", (xuint64_t) G_MAXINT64);
  u64 = 1111;
  xobject_get (obj, "uint64", &u64, NULL);
  g_assert_cmpuint (u64, ==, (xuint64_t) G_MAXINT64);

  g_settings_bind (settings, "string", obj, "string", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "string", "bu ba", NULL);
  s = g_settings_get_string (settings, "string");
  g_assert_cmpstr (s, ==, "bu ba");
  g_free (s);

  g_settings_set_string (settings, "string", "bla bla");
  xobject_get (obj, "string", &s, NULL);
  g_assert_cmpstr (s, ==, "bla bla");
  g_free (s);

  g_settings_bind (settings, "chararray", obj, "string", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "string", "non-unicode:\315", NULL);
  value = g_settings_get_value (settings, "chararray");
  g_assert_cmpstr (xvariant_get_bytestring (value), ==, "non-unicode:\315");
  xvariant_unref (value);

  g_settings_bind (settings, "double", obj, "double", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "double", G_MAXFLOAT, NULL);
  g_assert_cmpfloat (g_settings_get_double (settings, "double"), ==, G_MAXFLOAT);

  g_settings_set_double (settings, "double", G_MINFLOAT);
  d = 1.0;
  xobject_get (obj, "double", &d, NULL);
  g_assert_cmpfloat (d, ==, G_MINFLOAT);

  xobject_set (obj, "double", G_MAXDOUBLE, NULL);
  g_assert_cmpfloat (g_settings_get_double (settings, "double"), ==, G_MAXDOUBLE);

  g_settings_set_double (settings, "double", -G_MINDOUBLE);
  d = 1.0;
  xobject_get (obj, "double", &d, NULL);
  g_assert_cmpfloat (d, ==, -G_MINDOUBLE);

  strv = xstrsplit ("plastic bag,middle class,polyethylene", ",", 0);
  g_settings_bind (settings, "strv", obj, "strv", G_SETTINGS_BIND_DEFAULT);
  xobject_set (obj, "strv", strv, NULL);
  xstrfreev (strv);
  strv = g_settings_get_strv (settings, "strv");
  s = xstrjoinv (",", strv);
  g_assert_cmpstr (s, ==, "plastic bag,middle class,polyethylene");
  xstrfreev (strv);
  g_free (s);
  strv = xstrsplit ("decaffeinate,unleaded,keep all surfaces clean", ",", 0);
  g_settings_set_strv (settings, "strv", (const xchar_t **) strv);
  xstrfreev (strv);
  xobject_get (obj, "strv", &strv, NULL);
  s = xstrjoinv (",", strv);
  g_assert_cmpstr (s, ==, "decaffeinate,unleaded,keep all surfaces clean");
  xstrfreev (strv);
  g_free (s);
  g_settings_set_strv (settings, "strv", NULL);
  xobject_get (obj, "strv", &strv, NULL);
  g_assert_nonnull (strv);
  g_assert_cmpint (xstrv_length (strv), ==, 0);
  xstrfreev (strv);

  g_settings_bind (settings, "enum", obj, "enum", G_SETTINGS_BIND_DEFAULT);
  xobject_set (obj, "enum", TEST_ENUM_BAZ, NULL);
  s = g_settings_get_string (settings, "enum");
  g_assert_cmpstr (s, ==, "baz");
  g_free (s);
  g_assert_cmpint (g_settings_get_enum (settings, "enum"), ==, TEST_ENUM_BAZ);

  g_settings_set_enum (settings, "enum", TEST_ENUM_QUUX);
  i = 230;
  xobject_get (obj, "enum", &i, NULL);
  g_assert_cmpint (i, ==, TEST_ENUM_QUUX);

  g_settings_set_string (settings, "enum", "baz");
  i = 230;
  xobject_get (obj, "enum", &i, NULL);
  g_assert_cmpint (i, ==, TEST_ENUM_BAZ);

  g_settings_bind (settings, "flags", obj, "flags", G_SETTINGS_BIND_DEFAULT);
  xobject_set (obj, "flags", TEST_FLAGS_MOURNING, NULL);
  strv = g_settings_get_strv (settings, "flags");
  g_assert_cmpint (xstrv_length (strv), ==, 1);
  g_assert_cmpstr (strv[0], ==, "mourning");
  xstrfreev (strv);

  g_assert_cmpint (g_settings_get_flags (settings, "flags"), ==, TEST_FLAGS_MOURNING);

  g_settings_set_flags (settings, "flags", TEST_FLAGS_MOURNING | TEST_FLAGS_WALKING);
  i = 230;
  xobject_get (obj, "flags", &i, NULL);
  g_assert_cmpint (i, ==, TEST_FLAGS_MOURNING | TEST_FLAGS_WALKING);

  g_settings_bind (settings, "uint", obj, "uint", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "uint", 12345, NULL);
  g_assert_cmpuint (g_settings_get_uint (settings, "uint"), ==, 12345);

  g_settings_set_uint (settings, "uint", 54321);
  u = 1111;
  xobject_get (obj, "uint", &u, NULL);
  g_assert_cmpuint (u, ==, 54321);

  g_settings_bind (settings, "range", obj, "uint", G_SETTINGS_BIND_DEFAULT);
  xobject_set (obj, "uint", 22, NULL);
  u = 1111;
  g_assert_cmpuint (g_settings_get_uint (settings, "range"), ==, 22);
  xobject_get (obj, "uint", &u, NULL);
  g_assert_cmpuint (u, ==, 22);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "* is out of schema-specified range for*");
  xobject_set (obj, "uint", 45, NULL);
  g_test_assert_expected_messages ();
  u = 1111;
  xobject_get (obj, "uint", &u, NULL);
  g_assert_cmpuint (g_settings_get_uint (settings, "range"), ==, 22);
  /* The value of the object is currently not reset back to its initial value
  g_assert_cmpuint (u, ==, 22); */

  xobject_unref (obj);
  xobject_unref (settings);
}

static void
test_unbind (void)
{
  test_object_t *obj;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_bind (settings, "int", obj, "int", G_SETTINGS_BIND_DEFAULT);

  xobject_set (obj, "int", 12345, NULL);
  g_assert_cmpint (g_settings_get_int (settings, "int"), ==, 12345);

  g_settings_unbind (obj, "int");

  xobject_set (obj, "int", 54321, NULL);
  g_assert_cmpint (g_settings_get_int (settings, "int"), ==, 12345);

  xobject_unref (obj);
  xobject_unref (settings);
}

static void
test_bind_writable (void)
{
  test_object_t *obj;
  xsettings_t *settings;
  xboolean_t b;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  xobject_set (obj, "bool", FALSE, NULL);

  g_settings_bind_writable (settings, "int", obj, "bool", FALSE);

  xobject_get (obj, "bool", &b, NULL);
  g_assert_true (b);

  g_settings_unbind (obj, "bool");

  g_settings_bind_writable (settings, "int", obj, "bool", TRUE);

  xobject_get (obj, "bool", &b, NULL);
  g_assert_false (b);

  xobject_unref (obj);
  xobject_unref (settings);
}

/* test_t one-way bindings.
 * Verify that changes on one side show up on the other,
 * but not vice versa
 */
static void
test_directional_binding (void)
{
  test_object_t *obj;
  xsettings_t *settings;
  xboolean_t b;
  xint_t i;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  xobject_set (obj, "bool", FALSE, NULL);
  g_settings_set_boolean (settings, "bool", FALSE);

  g_settings_bind (settings, "bool", obj, "bool", G_SETTINGS_BIND_GET);

  g_settings_set_boolean (settings, "bool", TRUE);
  xobject_get (obj, "bool", &b, NULL);
  g_assert_cmpint (b, ==, TRUE);

  xobject_set (obj, "bool", FALSE, NULL);
  g_assert_cmpint (g_settings_get_boolean (settings, "bool"), ==, TRUE);

  xobject_set (obj, "int", 20, NULL);
  g_settings_set_int (settings, "int", 20);

  g_settings_bind (settings, "int", obj, "int", G_SETTINGS_BIND_SET);

  xobject_set (obj, "int", 32, NULL);
  g_assert_cmpint (g_settings_get_int (settings, "int"), ==, 32);

  g_settings_set_int (settings, "int", 20);
  xobject_get (obj, "int", &i, NULL);
  g_assert_cmpint (i, ==, 32);

  xobject_unref (obj);
  xobject_unref (settings);
}

/* test_t that type mismatch is caught when creating a binding */
static void
test_typesafe_binding (void)
{
  if (!g_test_undefined ())
    return;

  if (g_test_subprocess ())
    {
      test_object_t *obj;
      xsettings_t *settings;

      settings = g_settings_new ("org.gtk.test.binding");
      obj = test_object_new ();

      g_settings_bind (settings, "string", obj, "int", G_SETTINGS_BIND_DEFAULT);

      xobject_unref (obj);
      xobject_unref (settings);
      return;
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*not compatible*");
}

static xboolean_t
string_to_bool (xvalue_t   *value,
                xvariant_t *variant,
                xpointer_t  user_data)
{
  const xchar_t *s;

  s = xvariant_get_string (variant, NULL);
  xvalue_set_boolean (value, xstrcmp0 (s, "true") == 0);

  return TRUE;
}

static xvariant_t *
bool_to_string (const xvalue_t       *value,
                const xvariant_type_t *expected_type,
                xpointer_t            user_data)
{
  if (xvalue_get_boolean (value))
    return xvariant_new_string ("true");
  else
    return xvariant_new_string ("false");
}

static xvariant_t *
bool_to_bool (const xvalue_t       *value,
              const xvariant_type_t *expected_type,
              xpointer_t            user_data)
{
  return xvariant_new_boolean (xvalue_get_boolean (value));
}

/* test_t custom bindings.
 * Translate strings to booleans and back
 */
static void
test_custom_binding (void)
{
  test_object_t *obj;
  xsettings_t *settings;
  xchar_t *s;
  xboolean_t b;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_set_string (settings, "string", "true");

  g_settings_bind_with_mapping (settings, "string",
                                obj, "bool",
                                G_SETTINGS_BIND_DEFAULT,
                                string_to_bool,
                                bool_to_string,
                                NULL, NULL);

  g_settings_set_string (settings, "string", "false");
  xobject_get (obj, "bool", &b, NULL);
  g_assert_cmpint (b, ==, FALSE);

  g_settings_set_string (settings, "string", "not true");
  xobject_get (obj, "bool", &b, NULL);
  g_assert_cmpint (b, ==, FALSE);

  xobject_set (obj, "bool", TRUE, NULL);
  s = g_settings_get_string (settings, "string");
  g_assert_cmpstr (s, ==, "true");
  g_free (s);

  g_settings_bind_with_mapping (settings, "string",
                                obj, "bool",
                                G_SETTINGS_BIND_DEFAULT,
                                string_to_bool, bool_to_bool,
                                NULL, NULL);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*binding mapping function for key 'string' returned"
                         " xvariant_t of type 'b' when type 's' was requested*");
  xobject_set (obj, "bool", FALSE, NULL);
  g_test_assert_expected_messages ();

  xobject_unref (obj);
  xobject_unref (settings);
}

/* test_t that with G_SETTINGS_BIND_NO_CHANGES, the
 * initial settings value is transported to the object
 * side, but later settings changes do not affect the
 * object
 */
static void
test_no_change_binding (void)
{
  test_object_t *obj;
  xsettings_t *settings;
  xboolean_t b;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  xobject_set (obj, "bool", TRUE, NULL);
  g_settings_set_boolean (settings, "bool", FALSE);

  g_settings_bind (settings, "bool", obj, "bool", G_SETTINGS_BIND_GET_NO_CHANGES);

  xobject_get (obj, "bool", &b, NULL);
  g_assert_cmpint (b, ==, FALSE);

  g_settings_set_boolean (settings, "bool", TRUE);
  xobject_get (obj, "bool", &b, NULL);
  g_assert_cmpint (b, ==, FALSE);

  g_settings_set_boolean (settings, "bool", FALSE);
  xobject_set (obj, "bool", TRUE, NULL);
  b = g_settings_get_boolean (settings, "bool");
  g_assert_cmpint (b, ==, TRUE);

  xobject_unref (obj);
  xobject_unref (settings);
}

/* test_t that binding a non-readable property only
 * works in 'GET' mode.
 */
static void
test_no_read_binding_fail (void)
{
  test_object_t *obj;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_bind (settings, "string", obj, "no-read", 0);
}

static void
test_no_read_binding_pass (void)
{
  test_object_t *obj;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_bind (settings, "string", obj, "no-read", G_SETTINGS_BIND_GET);

  exit (0);
}

static void
test_no_read_binding (void)
{
  if (g_test_undefined ())
    {
      g_test_trap_subprocess ("/gsettings/no-read-binding/subprocess/fail", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*property*is not readable*");
    }

  g_test_trap_subprocess ("/gsettings/no-read-binding/subprocess/pass", 0, 0);
  g_test_trap_assert_passed ();
}

/* test_t that binding a non-writable property only
 * works in 'SET' mode.
 */
static void
test_no_write_binding_fail (void)
{
  test_object_t *obj;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_bind (settings, "string", obj, "no-write", 0);
}

static void
test_no_write_binding_pass (void)
{
  test_object_t *obj;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.binding");
  obj = test_object_new ();

  g_settings_bind (settings, "string", obj, "no-write", G_SETTINGS_BIND_SET);

  exit (0);
}

static void
test_no_write_binding (void)
{
  if (g_test_undefined ())
    {
      g_test_trap_subprocess ("/gsettings/no-write-binding/subprocess/fail", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*property*is not writable*");
    }

  g_test_trap_subprocess ("/gsettings/no-write-binding/subprocess/pass", 0, 0);
  g_test_trap_assert_passed ();
}

static void
key_changed_cb (xsettings_t *settings, const xchar_t *key, xpointer_t data)
{
  xboolean_t *b = data;
  (*b) = TRUE;
}

typedef struct
{
  const xchar_t *path;
  const xchar_t *root_group;
  const xchar_t *keyfile_group;
  const xchar_t *root_path;
} KeyfileTestData;

/*
 * test_t that using a keyfile works
 */
static void
test_keyfile (Fixture       *fixture,
              xconstpointer  user_data)
{
  xsettings_backend_t *kf_backend;
  xsettings_t *settings;
  xkey_file_t *keyfile;
  xchar_t *str;
  xboolean_t writable;
  xerror_t *error = NULL;
  xchar_t *data;
  xsize_t len;
  xboolean_t called = FALSE;
  xchar_t *keyfile_path = NULL, *store_path = NULL;

  keyfile_path = g_build_filename (fixture->tmp_dir, "keyfile", NULL);
  store_path = g_build_filename (keyfile_path, "gsettings.store", NULL);
  kf_backend = xkeyfile_settings_backend_new (store_path, "/", "root");
  settings = g_settings_new_with_backend ("org.gtk.test", kf_backend);
  xobject_unref (kf_backend);

  g_settings_reset (settings, "greeting");
  str = g_settings_get_string (settings, "greeting");
  g_assert_cmpstr (str, ==, "Hello, earthlings");
  g_free (str);

  writable = g_settings_is_writable (settings, "greeting");
  g_assert_true (writable);
  g_settings_set (settings, "greeting", "s", "see if this works");

  str = g_settings_get_string (settings, "greeting");
  g_assert_cmpstr (str, ==, "see if this works");
  g_free (str);

  g_settings_delay (settings);
  g_settings_set (settings, "farewell", "s", "cheerio");
  g_settings_apply (settings);

  keyfile = xkey_file_new ();
  g_assert_true (xkey_file_load_from_file (keyfile, store_path, 0, NULL));

  str = xkey_file_get_string (keyfile, "tests", "greeting", NULL);
  g_assert_cmpstr (str, ==, "'see if this works'");
  g_free (str);

  str = xkey_file_get_string (keyfile, "tests", "farewell", NULL);
  g_assert_cmpstr (str, ==, "'cheerio'");
  g_free (str);
  xkey_file_free (keyfile);

  g_settings_reset (settings, "greeting");
  g_settings_apply (settings);
  keyfile = xkey_file_new ();
  g_assert_true (xkey_file_load_from_file (keyfile, store_path, 0, NULL));

  str = xkey_file_get_string (keyfile, "tests", "greeting", NULL);
  g_assert_null (str);

  called = FALSE;
  xsignal_connect (settings, "changed::greeting", G_CALLBACK (key_changed_cb), &called);

  xkey_file_set_string (keyfile, "tests", "greeting", "'howdy'");
  data = xkey_file_to_data (keyfile, &len, NULL);
  xfile_set_contents (store_path, data, len, &error);
  g_assert_no_error (error);
  while (!called)
    xmain_context_iteration (NULL, FALSE);
  xsignal_handlers_disconnect_by_func (settings, key_changed_cb, &called);

  str = g_settings_get_string (settings, "greeting");
  g_assert_cmpstr (str, ==, "howdy");
  g_free (str);

  /* Now check setting a string without quotes */
  called = FALSE;
  xsignal_connect (settings, "changed::greeting", G_CALLBACK (key_changed_cb), &called);

  xkey_file_set_string (keyfile, "tests", "greeting", "he\"l????u??");
  g_free (data);
  data = xkey_file_to_data (keyfile, &len, NULL);
  xfile_set_contents (store_path, data, len, &error);
  g_assert_no_error (error);
  while (!called)
    xmain_context_iteration (NULL, FALSE);
  xsignal_handlers_disconnect_by_func (settings, key_changed_cb, &called);

  str = g_settings_get_string (settings, "greeting");
  g_assert_cmpstr (str, ==, "he\"l????u??");
  g_free (str);

  g_settings_set (settings, "farewell", "s", "cheerio");

  /* Check that empty keys/groups are not allowed. */
  g_assert_false (g_settings_is_writable (settings, ""));
  g_assert_false (g_settings_is_writable (settings, "/"));

  /* When executing as root, changing the mode of the keyfile will have
   * no effect on the writability of the settings.
   */
  if (geteuid () != 0)
    {
      called = FALSE;
      xsignal_connect (settings, "writable-changed::greeting",
                        G_CALLBACK (key_changed_cb), &called);

      g_assert_no_errno (g_chmod (keyfile_path, 0500));
      while (!called)
        xmain_context_iteration (NULL, FALSE);
      xsignal_handlers_disconnect_by_func (settings, key_changed_cb, &called);

      writable = g_settings_is_writable (settings, "greeting");
      g_assert_false (writable);
    }

  xkey_file_free (keyfile);
  g_free (data);

  xobject_unref (settings);

  /* Clean up the temporary directory. */
  g_assert_no_errno (g_chmod (keyfile_path, 0777));
  g_assert_no_errno (g_remove (store_path));
  g_assert_no_errno (g_rmdir (keyfile_path));
  g_free (store_path);
  g_free (keyfile_path);
}

/*
 * test_t that using a keyfile works with a schema with no path set.
 */
static void
test_keyfile_no_path (Fixture       *fixture,
                      xconstpointer  user_data)
{
  const KeyfileTestData *test_data = user_data;
  xsettings_backend_t *kf_backend;
  xsettings_t *settings;
  xkey_file_t *keyfile;
  xboolean_t writable;
  xchar_t *key = NULL;
  xerror_t *error = NULL;
  xchar_t *keyfile_path = NULL, *store_path = NULL;

  keyfile_path = g_build_filename (fixture->tmp_dir, "keyfile", NULL);
  store_path = g_build_filename (keyfile_path, "gsettings.store", NULL);
  kf_backend = xkeyfile_settings_backend_new (store_path, test_data->root_path, test_data->root_group);
  settings = g_settings_new_with_backend_and_path ("org.gtk.test.no-path", kf_backend, test_data->path);
  xobject_unref (kf_backend);

  g_settings_reset (settings, "test-boolean");
  g_assert_true (g_settings_get_boolean (settings, "test-boolean"));

  writable = g_settings_is_writable (settings, "test-boolean");
  g_assert_true (writable);
  g_settings_set (settings, "test-boolean", "b", FALSE);

  g_assert_false (g_settings_get_boolean (settings, "test-boolean"));

  g_settings_delay (settings);
  g_settings_set (settings, "test-boolean", "b", TRUE);
  g_settings_apply (settings);

  keyfile = xkey_file_new ();
  g_assert_true (xkey_file_load_from_file (keyfile, store_path, 0, NULL));

  g_assert_true (xkey_file_get_boolean (keyfile, test_data->keyfile_group, "test-boolean", NULL));

  xkey_file_free (keyfile);

  g_settings_reset (settings, "test-boolean");
  g_settings_apply (settings);
  keyfile = xkey_file_new ();
  g_assert_true (xkey_file_load_from_file (keyfile, store_path, 0, NULL));

  g_assert_false (xkey_file_get_string (keyfile, test_data->keyfile_group, "test-boolean", &error));
  g_assert_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND);
  g_clear_error (&error);

  /* Check that empty keys/groups are not allowed. */
  g_assert_false (g_settings_is_writable (settings, ""));
  g_assert_false (g_settings_is_writable (settings, "/"));

  /* Keys which ghost the root group name are not allowed. This can only be
   * tested when the path is `/` as otherwise it acts as a prefix and prevents
   * any ghosting. */
  if (xstr_equal (test_data->path, "/"))
    {
      key = xstrdup_printf ("%s/%s", test_data->root_group, "");
      g_assert_false (g_settings_is_writable (settings, key));
      g_free (key);

      key = xstrdup_printf ("%s/%s", test_data->root_group, "/");
      g_assert_false (g_settings_is_writable (settings, key));
      g_free (key);

      key = xstrdup_printf ("%s/%s", test_data->root_group, "test-boolean");
      g_assert_false (g_settings_is_writable (settings, key));
      g_free (key);
    }

  xkey_file_free (keyfile);
  xobject_unref (settings);

  /* Clean up the temporary directory. */
  g_assert_no_errno (g_chmod (keyfile_path, 0777));
  g_assert_no_errno (g_remove (store_path));
  g_assert_no_errno (g_rmdir (keyfile_path));
  g_free (store_path);
  g_free (keyfile_path);
}

/*
 * test_t that a keyfile rejects writes to keys outside its root path.
 */
static void
test_keyfile_outside_root_path (Fixture       *fixture,
                                xconstpointer  user_data)
{
  xsettings_backend_t *kf_backend;
  xsettings_t *settings;
  xchar_t *keyfile_path = NULL, *store_path = NULL;

  keyfile_path = g_build_filename (fixture->tmp_dir, "keyfile", NULL);
  store_path = g_build_filename (keyfile_path, "gsettings.store", NULL);
  kf_backend = xkeyfile_settings_backend_new (store_path, "/tests/basic-types/", "root");
  settings = g_settings_new_with_backend_and_path ("org.gtk.test.no-path", kf_backend, "/tests/");
  xobject_unref (kf_backend);

  g_assert_false (g_settings_is_writable (settings, "test-boolean"));

  xobject_unref (settings);

  /* Clean up the temporary directory. The keyfile probably doesn???t exist, so
   * don???t error on failure. */
  g_remove (store_path);
  g_assert_no_errno (g_rmdir (keyfile_path));
  g_free (store_path);
  g_free (keyfile_path);
}

/*
 * test_t that a keyfile rejects writes to keys in the root if no root group is set.
 */
static void
test_keyfile_no_root_group (Fixture       *fixture,
                            xconstpointer  user_data)
{
  xsettings_backend_t *kf_backend;
  xsettings_t *settings;
  xchar_t *keyfile_path = NULL, *store_path = NULL;

  keyfile_path = g_build_filename (fixture->tmp_dir, "keyfile", NULL);
  store_path = g_build_filename (keyfile_path, "gsettings.store", NULL);
  kf_backend = xkeyfile_settings_backend_new (store_path, "/", NULL);
  settings = g_settings_new_with_backend_and_path ("org.gtk.test.no-path", kf_backend, "/");
  xobject_unref (kf_backend);

  g_assert_false (g_settings_is_writable (settings, "test-boolean"));
  g_assert_true (g_settings_is_writable (settings, "child/test-boolean"));

  xobject_unref (settings);

  /* Clean up the temporary directory. The keyfile probably doesn???t exist, so
   * don???t error on failure. */
  g_remove (store_path);
  g_assert_no_errno (g_rmdir (keyfile_path));
  g_free (store_path);
  g_free (keyfile_path);
}

/* test_t that getting child schemas works
 */
static void
test_child_schema (void)
{
  xsettings_t *settings;
  xsettings_t *child;
  xuint8_t byte;

  /* first establish some known conditions */
  settings = g_settings_new ("org.gtk.test.basic-types");
  g_settings_set (settings, "test-byte", "y", 36);

  g_settings_get (settings, "test-byte", "y", &byte);
  g_assert_cmpint (byte, ==, 36);

  xobject_unref (settings);

  settings = g_settings_new ("org.gtk.test");
  child = g_settings_get_child (settings, "basic-types");
  g_assert_nonnull (child);

  g_settings_get (child, "test-byte", "y", &byte);
  g_assert_cmpint (byte, ==, 36);

  xobject_unref (child);
  xobject_unref (settings);
}

#include "../strinfo.c"

static void
test_strinfo (void)
{
  /*  "foo" has a value of 1
   *  "bar" has a value of 2
   *  "baz" is an alias for "bar"
   */
  xchar_t array[] =
    "\1\0\0\0"      "\xff""foo"     "\0\0\0\xff"    "\2\0\0\0"
    "\xff" "bar"    "\0\0\0\xff"    "\3\0\0\0"      "\xfe""baz"
    "\0\0\0\xff";
  const xuint32_t *strinfo = (xuint32_t *) array;
  xuint_t length = sizeof array / 4;
  xuint_t result = 0;

  {
    /* build it and compare */
    xstring_t *builder;

    builder = xstring_new (NULL);
    strinfo_builder_append_item (builder, "foo", 1);
    strinfo_builder_append_item (builder, "bar", 2);
    g_assert_true (strinfo_builder_append_alias (builder, "baz", "bar"));
    g_assert_cmpmem (builder->str, builder->len, strinfo, length * 4);
    xstring_free (builder, TRUE);
  }

  g_assert_cmpstr (strinfo_string_from_alias (strinfo, length, "foo"),
                   ==, NULL);
  g_assert_cmpstr (strinfo_string_from_alias (strinfo, length, "bar"),
                   ==, NULL);
  g_assert_cmpstr (strinfo_string_from_alias (strinfo, length, "baz"),
                   ==, "bar");
  g_assert_cmpstr (strinfo_string_from_alias (strinfo, length, "quux"),
                   ==, NULL);

  g_assert_true (strinfo_enum_from_string (strinfo, length, "foo", &result));
  g_assert_cmpint (result, ==, 1);
  g_assert_true (strinfo_enum_from_string (strinfo, length, "bar", &result));
  g_assert_cmpint (result, ==, 2);
  g_assert_false (strinfo_enum_from_string (strinfo, length, "baz", &result));
  g_assert_false (strinfo_enum_from_string (strinfo, length, "quux", &result));

  g_assert_cmpstr (strinfo_string_from_enum (strinfo, length, 0), ==, NULL);
  g_assert_cmpstr (strinfo_string_from_enum (strinfo, length, 1), ==, "foo");
  g_assert_cmpstr (strinfo_string_from_enum (strinfo, length, 2), ==, "bar");
  g_assert_cmpstr (strinfo_string_from_enum (strinfo, length, 3), ==, NULL);

  g_assert_true (strinfo_is_string_valid (strinfo, length, "foo"));
  g_assert_true (strinfo_is_string_valid (strinfo, length, "bar"));
  g_assert_false (strinfo_is_string_valid (strinfo, length, "baz"));
  g_assert_false (strinfo_is_string_valid (strinfo, length, "quux"));
}

static void
test_enums_non_enum_key (void)
{
  xsettings_t *direct;

  direct = g_settings_new ("org.gtk.test.enums.direct");
  g_settings_get_enum (direct, "test");
  g_assert_not_reached ();
}

static void
test_enums_non_enum_value (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.enums");
  g_settings_set_enum (settings, "test", 42);
  g_assert_not_reached ();
}

static void
test_enums_range (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.enums");
  g_settings_set_string (settings, "test", "qux");
  g_assert_not_reached ();
}

static void
test_enums_non_flags (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.enums");
  g_settings_get_flags (settings, "test");
  g_assert_not_reached ();
}

static void
test_enums (void)
{
  xsettings_t *settings, *direct;
  xchar_t *str;

  settings = g_settings_new ("org.gtk.test.enums");
  direct = g_settings_new ("org.gtk.test.enums.direct");

  if (g_test_undefined () && !backend_set)
    {
      g_test_trap_subprocess ("/gsettings/enums/subprocess/non-enum-key", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*not associated with an enum*");

      g_test_trap_subprocess ("/gsettings/enums/subprocess/non-enum-value", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*invalid enum value 42*");

      g_test_trap_subprocess ("/gsettings/enums/subprocess/range", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*g_settings_set_value*valid range*");

      g_test_trap_subprocess ("/gsettings/enums/subprocess/non-flags", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*not associated with a flags*");
    }

  str = g_settings_get_string (settings, "test");
  g_assert_cmpstr (str, ==, "bar");
  g_free (str);

  g_settings_set_enum (settings, "test", TEST_ENUM_FOO);

  str = g_settings_get_string (settings, "test");
  g_assert_cmpstr (str, ==, "foo");
  g_free (str);

  g_assert_cmpint (g_settings_get_enum (settings, "test"), ==, TEST_ENUM_FOO);

  g_settings_set_string (direct, "test", "qux");

  str = g_settings_get_string (direct, "test");
  g_assert_cmpstr (str, ==, "qux");
  g_free (str);

  str = g_settings_get_string (settings, "test");
  g_assert_cmpstr (str, ==, "quux");
  g_free (str);

  g_assert_cmpint (g_settings_get_enum (settings, "test"), ==, TEST_ENUM_QUUX);

  xobject_unref (direct);
  xobject_unref (settings);
}

static void
test_flags_non_flags_key (void)
{
  xsettings_t *direct;

  direct = g_settings_new ("org.gtk.test.enums.direct");
  g_settings_get_flags (direct, "test");
  g_assert_not_reached ();
}

static void
test_flags_non_flags_value (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.enums");
  g_settings_set_flags (settings, "f-test", 0x42);
  g_assert_not_reached ();
}

static void
test_flags_range (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.enums");
  g_settings_set_strv (settings, "f-test",
                       (const xchar_t **) xstrsplit ("rock", ",", 0));
  g_assert_not_reached ();
}

static void
test_flags_non_enum (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.enums");
  g_settings_get_enum (settings, "f-test");
  g_assert_not_reached ();
}

static void
test_flags (void)
{
  xsettings_t *settings, *direct;
  xchar_t **strv;
  xchar_t *str;

  settings = g_settings_new ("org.gtk.test.enums");
  direct = g_settings_new ("org.gtk.test.enums.direct");

  if (g_test_undefined () && !backend_set)
    {
      g_test_trap_subprocess ("/gsettings/flags/subprocess/non-flags-key", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*not associated with a flags*");

      g_test_trap_subprocess ("/gsettings/flags/subprocess/non-flags-value", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*invalid flags value 0x00000042*");

      g_test_trap_subprocess ("/gsettings/flags/subprocess/range", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*g_settings_set_value*valid range*");

      g_test_trap_subprocess ("/gsettings/flags/subprocess/non-enum", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*not associated with an enum*");
    }

  strv = g_settings_get_strv (settings, "f-test");
  str = xstrjoinv (",", strv);
  g_assert_cmpstr (str, ==, "");
  xstrfreev (strv);
  g_free (str);

  g_settings_set_flags (settings, "f-test",
                        TEST_FLAGS_WALKING | TEST_FLAGS_TALKING);

  strv = g_settings_get_strv (settings, "f-test");
  str = xstrjoinv (",", strv);
  g_assert_cmpstr (str, ==, "talking,walking");
  xstrfreev (strv);
  g_free (str);

  g_assert_cmpint (g_settings_get_flags (settings, "f-test"), ==,
                   TEST_FLAGS_WALKING | TEST_FLAGS_TALKING);

  strv = xstrsplit ("speaking,laughing", ",", 0);
  g_settings_set_strv (direct, "f-test", (const xchar_t **) strv);
  xstrfreev (strv);

  strv = g_settings_get_strv (direct, "f-test");
  str = xstrjoinv (",", strv);
  g_assert_cmpstr (str, ==, "speaking,laughing");
  xstrfreev (strv);
  g_free (str);

  strv = g_settings_get_strv (settings, "f-test");
  str = xstrjoinv (",", strv);
  g_assert_cmpstr (str, ==, "talking,laughing");
  xstrfreev (strv);
  g_free (str);

  g_assert_cmpint (g_settings_get_flags (settings, "f-test"), ==,
                   TEST_FLAGS_TALKING | TEST_FLAGS_LAUGHING);

  xobject_unref (direct);
  xobject_unref (settings);
}

static void
test_range_high (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.range");
  g_settings_set_int (settings, "val", 45);
  g_assert_not_reached ();
}

static void
test_range_low (void)
{
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test.range");
  g_settings_set_int (settings, "val", 1);
  g_assert_not_reached ();
}

static void
test_range (void)
{
  xsettings_t *settings, *direct;
  xvariant_t *value;

  settings = g_settings_new ("org.gtk.test.range");
  direct = g_settings_new ("org.gtk.test.range.direct");

  if (g_test_undefined () && !backend_set)
    {
      g_test_trap_subprocess ("/gsettings/range/subprocess/high", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*g_settings_set_value*valid range*");

      g_test_trap_subprocess ("/gsettings/range/subprocess/low", 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*g_settings_set_value*valid range*");
    }

  g_assert_cmpint (g_settings_get_int (settings, "val"), ==, 33);
  g_settings_set_int (direct, "val", 22);
  g_assert_cmpint (g_settings_get_int (direct, "val"), ==, 22);
  g_assert_cmpint (g_settings_get_int (settings, "val"), ==, 22);
  g_settings_set_int (direct, "val", 45);
  g_assert_cmpint (g_settings_get_int (direct, "val"), ==, 45);
  g_assert_cmpint (g_settings_get_int (settings, "val"), ==, 33);
  g_settings_set_int (direct, "val", 1);
  g_assert_cmpint (g_settings_get_int (direct, "val"), ==, 1);
  g_assert_cmpint (g_settings_get_int (settings, "val"), ==, 33);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  value = xvariant_new_int32 (1);
  g_assert_false (g_settings_range_check (settings, "val", value));
  xvariant_unref (value);
  value = xvariant_new_int32 (33);
  g_assert_true (g_settings_range_check (settings, "val", value));
  xvariant_unref (value);
  value = xvariant_new_int32 (45);
  g_assert_false (g_settings_range_check (settings, "val", value));
  xvariant_unref (value);
G_GNUC_END_IGNORE_DEPRECATIONS

  xobject_unref (direct);
  xobject_unref (settings);
}

static xboolean_t
strv_has_string (xchar_t       **haystack,
                 const xchar_t  *needle)
{
  xuint_t n;

  for (n = 0; haystack != NULL && haystack[n] != NULL; n++)
    {
      if (xstrcmp0 (haystack[n], needle) == 0)
        return TRUE;
    }
  return FALSE;
}

static xboolean_t
strv_set_equal (xchar_t **strv, ...)
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
      if (!strv_has_string (strv, str))
        {
          res = FALSE;
          break;
        }
      count++;
    }
  va_end (list);

  if (res)
    res = xstrv_length ((xchar_t**)strv) == count;

  return res;
}

static void
test_list_items (void)
{
  xsettings_schema_t *schema;
  xsettings_t *settings;
  xchar_t **children;
  xchar_t **keys;

  settings = g_settings_new ("org.gtk.test");
  xobject_get (settings, "settings-schema", &schema, NULL);
  children = g_settings_list_children (settings);
  keys = g_settings_schema_list_keys (schema);

  g_assert_true (strv_set_equal (children, "basic-types", "complex-types", "localized", NULL));
  g_assert_true (strv_set_equal (keys, "greeting", "farewell", NULL));

  xstrfreev (children);
  xstrfreev (keys);

  g_settings_schema_unref (schema);
  xobject_unref (settings);
}

static void
test_list_schemas (void)
{
  const xchar_t * const *schemas;
  const xchar_t * const *relocs;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  relocs = g_settings_list_relocatable_schemas ();
  schemas = g_settings_list_schemas ();
G_GNUC_END_IGNORE_DEPRECATIONS

  g_assert_true (strv_set_equal ((xchar_t **)relocs,
                                 "org.gtk.test.no-path",
                                 "org.gtk.test.extends.base",
                                 "org.gtk.test.extends.extended",
                                 NULL));

  g_assert_true (strv_set_equal ((xchar_t **)schemas,
                                 "org.gtk.test",
                                 "org.gtk.test.basic-types",
                                 "org.gtk.test.complex-types",
                                 "org.gtk.test.localized",
                                 "org.gtk.test.binding",
                                 "org.gtk.test.enums",
                                 "org.gtk.test.enums.direct",
                                 "org.gtk.test.range",
                                 "org.gtk.test.range.direct",
                                 "org.gtk.test.mapped",
                                 "org.gtk.test.descriptions",
                                 "org.gtk.test.per-desktop",
                                 NULL));
}

static xboolean_t
map_func (xvariant_t *value,
          xpointer_t *result,
          xpointer_t  user_data)
{
  xint_t *state = user_data;
  xint_t v;

  if (value)
    v = xvariant_get_int32 (value);
  else
    v = -1;

  if (*state == 0)
    {
      g_assert_cmpint (v, ==, 1);
      (*state)++;
      return FALSE;
    }
  else if (*state == 1)
    {
      g_assert_cmpint (v, ==, 0);
      (*state)++;
      return FALSE;
    }
  else
    {
      g_assert_null (value);
      *result = xvariant_new_int32 (5);
      return TRUE;
    }
}

static void
test_get_mapped (void)
{
  xsettings_t *settings;
  xint_t state;
  xpointer_t p;
  xint_t val;

  settings = g_settings_new ("org.gtk.test.mapped");
  g_settings_set_int (settings, "val", 1);

  state = 0;
  p = g_settings_get_mapped (settings, "val", map_func, &state);
  val = xvariant_get_int32 ((xvariant_t*)p);
  g_assert_cmpint (val, ==, 5);

  xvariant_unref (p);
  xobject_unref (settings);
}

static void
test_get_range (void)
{
  xsettings_t *settings;
  xvariant_t *range;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  settings = g_settings_new ("org.gtk.test.range");
  range = g_settings_get_range (settings, "val");
  check_and_free (range, "('range', <(2, 44)>)");
  xobject_unref (settings);

  settings = g_settings_new ("org.gtk.test.enums");
  range = g_settings_get_range (settings, "test");
  check_and_free (range, "('enum', <['foo', 'bar', 'baz', 'quux']>)");
  xobject_unref (settings);

  settings = g_settings_new ("org.gtk.test.enums");
  range = g_settings_get_range (settings, "f-test");
  check_and_free (range, "('flags', "
                  "<['mourning', 'laughing', 'talking', 'walking']>)");
  xobject_unref (settings);

  settings = g_settings_new ("org.gtk.test");
  range = g_settings_get_range (settings, "greeting");
  check_and_free (range, "('type', <@as []>)");
  xobject_unref (settings);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
test_schema_source (void)
{
  xsettings_schema_source_t *parent;
  xsettings_schema_source_t *source;
  xsettings_backend_t *backend;
  xsettings_schema_t *schema;
  xerror_t *error = NULL;
  xsettings_t *settings, *child;
  xboolean_t enabled;

  backend = g_settings_backend_get_default ();

  /* make sure it fails properly */
  parent = g_settings_schema_source_get_default ();
  source = g_settings_schema_source_new_from_directory ("/path/that/does/not/exist", parent,  TRUE, &error);
  g_assert_null (source);
  g_assert_error (error, XFILE_ERROR, XFILE_ERROR_NOENT);
  g_clear_error (&error);

  /* test_t error handling of corrupt compiled files. */
  source = g_settings_schema_source_new_from_directory ("schema-source-corrupt", parent, TRUE, &error);
  g_assert_error (error, XFILE_ERROR, XFILE_ERROR_INVAL);
  g_assert_null (source);
  g_clear_error (&error);

  /* test_t error handling of empty compiled files. */
  source = g_settings_schema_source_new_from_directory ("schema-source-empty", parent, TRUE, &error);
  g_assert_error (error, XFILE_ERROR, XFILE_ERROR_INVAL);
  g_assert_null (source);
  g_clear_error (&error);

  /* create a source with the parent */
  source = g_settings_schema_source_new_from_directory ("schema-source", parent, TRUE, &error);
  g_assert_no_error (error);
  g_assert_nonnull (source);

  /* check recursive lookups are working */
  schema = g_settings_schema_source_lookup (source, "org.gtk.test", TRUE);
  g_assert_nonnull (schema);
  g_settings_schema_unref (schema);

  /* check recursive lookups for non-existent schemas */
  schema = g_settings_schema_source_lookup (source, "org.gtk.doesnotexist", TRUE);
  g_assert_null (schema);

  /* check non-recursive for schema that only exists in lower layers */
  schema = g_settings_schema_source_lookup (source, "org.gtk.test", FALSE);
  g_assert_null (schema);

  /* check non-recursive lookup for non-existent */
  schema = g_settings_schema_source_lookup (source, "org.gtk.doesnotexist", FALSE);
  g_assert_null (schema);

  /* check non-recursive for schema that exists in toplevel */
  schema = g_settings_schema_source_lookup (source, "org.gtk.schemasourcecheck", FALSE);
  g_assert_nonnull (schema);
  g_settings_schema_unref (schema);

  /* check recursive for schema that exists in toplevel */
  schema = g_settings_schema_source_lookup (source, "org.gtk.schemasourcecheck", TRUE);
  g_assert_nonnull (schema);

  /* try to use it for something */
  settings = g_settings_new_full (schema, backend, "/test/");
  g_settings_schema_unref (schema);
  enabled = FALSE;
  g_settings_get (settings, "enabled", "b", &enabled);
  g_assert_true (enabled);

  /* Check that child schemas are resolved from the correct schema source, see glib#1884 */
  child = g_settings_get_child (settings, "child");
  g_settings_get (settings, "enabled", "b", &enabled);

  xobject_unref (child);
  xobject_unref (settings);
  g_settings_schema_source_unref (source);

  /* try again, but with no parent */
  source = g_settings_schema_source_new_from_directory ("schema-source", NULL, FALSE, NULL);
  g_assert_nonnull (source);

  /* should not find it this time, even if recursive... */
  schema = g_settings_schema_source_lookup (source, "org.gtk.test", FALSE);
  g_assert_null (schema);
  schema = g_settings_schema_source_lookup (source, "org.gtk.test", TRUE);
  g_assert_null (schema);

  /* should still find our own... */
  schema = g_settings_schema_source_lookup (source, "org.gtk.schemasourcecheck", TRUE);
  g_assert_nonnull (schema);
  g_settings_schema_unref (schema);
  schema = g_settings_schema_source_lookup (source, "org.gtk.schemasourcecheck", FALSE);
  g_assert_nonnull (schema);
  g_settings_schema_unref (schema);

  g_settings_schema_source_unref (source);
  xobject_unref (backend);
}

static void
test_schema_list_keys (void)
{
  xchar_t                 **keys;
  xsettings_schema_source_t  *src    = g_settings_schema_source_get_default ();
  xsettings_schema_t        *schema = g_settings_schema_source_lookup (src, "org.gtk.test", TRUE);
  g_assert_nonnull (schema);

  keys = g_settings_schema_list_keys (schema);

  g_assert_true (strv_set_equal ((xchar_t **)keys,
                                 "greeting",
                                 "farewell",
                                 NULL));

  xstrfreev (keys);
  g_settings_schema_unref (schema);
}

static void
test_actions (void)
{
  xaction_t *string, *toggle;
  xboolean_t c1, c2, c3;
  xsettings_t *settings;
  xchar_t *name;
  xvariant_type_t *param_type;
  xboolean_t enabled;
  xvariant_type_t *state_type;
  xvariant_t *state;

  settings = g_settings_new ("org.gtk.test.basic-types");
  string = g_settings_create_action (settings, "test-string");
  toggle = g_settings_create_action (settings, "test-boolean");
  xobject_unref (settings); /* should be held by the actions */

  xsignal_connect (settings, "changed", G_CALLBACK (changed_cb2), &c1);
  xsignal_connect (string, "notify::state", G_CALLBACK (changed_cb2), &c2);
  xsignal_connect (toggle, "notify::state", G_CALLBACK (changed_cb2), &c3);

  c1 = c2 = c3 = FALSE;
  g_settings_set_string (settings, "test-string", "hello world");
  check_and_free (g_action_get_state (string), "'hello world'");
  g_assert_true (c1 && c2 && !c3);
  c1 = c2 = c3 = FALSE;

  g_action_activate (string, xvariant_new_string ("hihi"));
  check_and_free (g_settings_get_value (settings, "test-string"), "'hihi'");
  g_assert_true (c1 && c2 && !c3);
  c1 = c2 = c3 = FALSE;

  g_action_change_state (string, xvariant_new_string ("kthxbye"));
  check_and_free (g_settings_get_value (settings, "test-string"), "'kthxbye'");
  g_assert_true (c1 && c2 && !c3);
  c1 = c2 = c3 = FALSE;

  g_action_change_state (toggle, xvariant_new_boolean (TRUE));
  g_assert_true (g_settings_get_boolean (settings, "test-boolean"));
  g_assert_true (c1 && !c2 && c3);
  c1 = c2 = c3 = FALSE;

  g_action_activate (toggle, NULL);
  g_assert_false (g_settings_get_boolean (settings, "test-boolean"));
  g_assert_true (c1 && !c2 && c3);

  xobject_get (string,
                "name", &name,
                "parameter-type", &param_type,
                "enabled", &enabled,
                "state-type", &state_type,
                "state", &state,
                NULL);

  g_assert_cmpstr (name, ==, "test-string");
  g_assert_true (xvariant_type_equal (param_type, G_VARIANT_TYPE_STRING));
  g_assert_true (enabled);
  g_assert_true (xvariant_type_equal (state_type, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr (xvariant_get_string (state, NULL), ==, "kthxbye");

  g_free (name);
  xvariant_type_free (param_type);
  xvariant_type_free (state_type);
  xvariant_unref (state);

  xobject_unref (string);
  xobject_unref (toggle);
}

static void
test_null_backend (void)
{
  xsettings_backend_t *backend;
  xsettings_t *settings;
  xchar_t *str;
  xboolean_t writable;

  backend = g_null_settings_backend_new ();
  settings = g_settings_new_with_backend_and_path ("org.gtk.test", backend, "/tests/");

  xobject_get (settings, "schema-id", &str, NULL);
  g_assert_cmpstr (str, ==, "org.gtk.test");
  g_free (str);

  settings_assert_cmpstr (settings, "greeting", ==, "Hello, earthlings");

  g_settings_set (settings, "greeting", "s", "goodbye world");
  settings_assert_cmpstr (settings, "greeting", ==, "Hello, earthlings");

  writable = g_settings_is_writable (settings, "greeting");
  g_assert_false (writable);

  g_settings_reset (settings, "greeting");

  g_settings_delay (settings);
  g_settings_set (settings, "greeting", "s", "goodbye world");
  g_settings_apply (settings);
  settings_assert_cmpstr (settings, "greeting", ==, "Hello, earthlings");

  xobject_unref (settings);
  xobject_unref (backend);
}

static void
test_memory_backend (void)
{
  xsettings_backend_t *backend;

  backend = xmemory_settings_backend_new ();
  g_assert_true (X_IS_SETTINGS_BACKEND (backend));
  xobject_unref (backend);
}

static void
test_read_descriptions (void)
{
  xsettings_schema_t *schema;
  xsettings_schema_key_t *key;
  xsettings_t *settings;

  settings = g_settings_new ("org.gtk.test");
  xobject_get (settings, "settings-schema", &schema, NULL);
  key = g_settings_schema_get_key (schema, "greeting");

  g_assert_cmpstr (g_settings_schema_key_get_summary (key), ==, "A greeting");
  g_assert_cmpstr (g_settings_schema_key_get_description (key), ==, "Greeting of the invading martians");

  g_settings_schema_key_unref (key);
  g_settings_schema_unref (schema);

  xobject_unref (settings);

  settings = g_settings_new ("org.gtk.test.descriptions");
  xobject_get (settings, "settings-schema", &schema, NULL);
  key = g_settings_schema_get_key (schema, "a");

  g_assert_cmpstr (g_settings_schema_key_get_summary (key), ==,
                   "a paragraph.\n\n"
                   "with some whitespace.\n\n"
                   "because not everyone has a great editor.\n\n"
                   "lots of space is as one.");

  g_settings_schema_key_unref (key);
  g_settings_schema_unref (schema);

  xobject_unref (settings);
}

static void
test_default_value (void)
{
  xsettings_t *settings;
  xsettings_schema_t *schema;
  xsettings_schema_key_t *key;
  xvariant_t *v;
  const xchar_t *str;
  xchar_t *s;

  settings = g_settings_new ("org.gtk.test");
  xobject_get (settings, "settings-schema", &schema, NULL);
  key = g_settings_schema_get_key (schema, "greeting");
  g_settings_schema_unref (schema);
  g_settings_schema_key_ref (key);

  g_assert_true (xvariant_type_equal (g_settings_schema_key_get_value_type (key), G_VARIANT_TYPE_STRING));

  v = g_settings_schema_key_get_default_value (key);
  str = xvariant_get_string (v, NULL);
  g_assert_cmpstr (str, ==, "Hello, earthlings");
  xvariant_unref (v);

  g_settings_schema_key_unref (key);
  g_settings_schema_key_unref (key);

  g_settings_set (settings, "greeting", "s", "goodbye world");

  v = g_settings_get_user_value (settings, "greeting");
  str = xvariant_get_string (v, NULL);
  g_assert_cmpstr (str, ==, "goodbye world");
  xvariant_unref (v);

  v = g_settings_get_default_value (settings, "greeting");
  str = xvariant_get_string (v, NULL);
  g_assert_cmpstr (str, ==, "Hello, earthlings");
  xvariant_unref (v);

  g_settings_reset (settings, "greeting");

  v = g_settings_get_user_value (settings, "greeting");
  g_assert_null (v);

  s = g_settings_get_string (settings, "greeting");
  g_assert_cmpstr (s, ==, "Hello, earthlings");
  g_free (s);

  xobject_unref (settings);
}

static xboolean_t
string_map_func (xvariant_t *value,
                 xpointer_t *result,
                 xpointer_t  user_data)
{
  const xchar_t *str;

  str = xvariant_get_string (value, NULL);
  *result = xvariant_new_string (str);

  return TRUE;
}

/* test_t that per-desktop values from org.gtk.test.gschema.override
 * does not change default value if current desktop is not listed in
 * $XDG_CURRENT_DESKTOP.
 */
static void
test_per_desktop (void)
{
  xsettings_t *settings;
  test_object_t *obj;
  xpointer_t p;
  xchar_t *str;

  settings = g_settings_new ("org.gtk.test.per-desktop");
  obj = test_object_new ();

  if (!g_test_subprocess ())
    {
      g_test_trap_subprocess ("/gsettings/per-desktop/subprocess", 0, 0);
      g_test_trap_assert_passed ();
    }

  str = g_settings_get_string (settings, "desktop");
  g_assert_cmpstr (str, ==, "GNOME");
  g_free (str);

  p = g_settings_get_mapped (settings, "desktop", string_map_func, NULL);

  str = xvariant_dup_string (p, NULL);
  g_assert_cmpstr (str, ==, "GNOME");
  g_free (str);

  xvariant_unref (p);

  g_settings_bind (settings, "desktop", obj, "string", G_SETTINGS_BIND_DEFAULT);

  xobject_get (obj, "string", &str, NULL);
  g_assert_cmpstr (str, ==, "GNOME");
  g_free (str);

  xobject_unref (settings);
  xobject_unref (obj);
}

/* test_t that per-desktop values from org.gtk.test.gschema.override
 * are successfully loaded based on the value of $XDG_CURRENT_DESKTOP.
 */
static void
test_per_desktop_subprocess (void)
{
  xsettings_t *settings;
  test_object_t *obj;
  xpointer_t p;
  xchar_t *str;

  g_setenv ("XDG_CURRENT_DESKTOP", "GNOME-Classic:GNOME", TRUE);

  settings = g_settings_new ("org.gtk.test.per-desktop");
  obj = test_object_new ();

  str = g_settings_get_string (settings, "desktop");
  g_assert_cmpstr (str, ==, "GNOME Classic");
  g_free (str);

  p = g_settings_get_mapped (settings, "desktop", string_map_func, NULL);

  str = xvariant_dup_string (p, NULL);
  g_assert_cmpstr (str, ==, "GNOME Classic");
  g_free (str);

  xvariant_unref (p);

  g_settings_bind (settings, "desktop", obj, "string", G_SETTINGS_BIND_DEFAULT);

  xobject_get (obj, "string", &str, NULL);
  g_assert_cmpstr (str, ==, "GNOME Classic");
  g_free (str);

  xobject_unref (settings);
  xobject_unref (obj);
}

static void
test_extended_schema (void)
{
  xsettings_schema_t *schema;
  xsettings_t *settings;
  xchar_t **keys;

  settings = g_settings_new_with_path ("org.gtk.test.extends.extended", "/test/extendes/");
  xobject_get (settings, "settings-schema", &schema, NULL);
  keys = g_settings_schema_list_keys (schema);
  g_assert_true (strv_set_equal (keys, "int32", "string", "another-int32", NULL));
  xstrfreev (keys);
  xobject_unref (settings);
  g_settings_schema_unref (schema);
}

int
main (int argc, char *argv[])
{
  xchar_t *schema_text;
  xchar_t *override_text;
  xchar_t *enums;
  xint_t result;
  const KeyfileTestData keyfile_test_data_explicit_path = { "/tests/", "root", "tests", "/" };
  const KeyfileTestData keyfile_test_data_empty_path = { "/", "root", "root", "/" };
  const KeyfileTestData keyfile_test_data_long_path = {
    "/tests/path/is/very/long/and/this/makes/some/comparisons/take/a/different/branch/",
    "root",
    "tests/path/is/very/long/and/this/makes/some/comparisons/take/a/different/branch",
    "/"
  };

/* Meson build sets this */
#ifdef TEST_LOCALE_PATH
  if (xstr_has_suffix (TEST_LOCALE_PATH, "LC_MESSAGES"))
    {
      locale_dir = TEST_LOCALE_PATH G_DIR_SEPARATOR_S ".." G_DIR_SEPARATOR_S "..";
    }
#endif

  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  if (!g_test_subprocess ())
    {
      xerror_t *local_error = NULL;
      /* A GVDB header is 6 guint32s, and requires a magic number in the first
       * two guint32s. A set of zero bytes of a greater length is considered
       * corrupt. */
      const xuint8_t gschemas_compiled_corrupt[sizeof (xuint32_t) * 7] = { 0, };

      backend_set = g_getenv ("GSETTINGS_BACKEND") != NULL;

      g_setenv ("XDG_DATA_DIRS", ".", TRUE);
      g_setenv ("XDG_DATA_HOME", ".", TRUE);
      g_setenv ("GSETTINGS_SCHEMA_DIR", ".", TRUE);
      g_setenv ("XDG_CURRENT_DESKTOP", "", TRUE);

      if (!backend_set)
        g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);

      g_remove ("org.gtk.test.enums.xml");
      /* #XPL_MKENUMS is defined in meson.build */
      g_assert_true (g_spawn_command_line_sync (XPL_MKENUMS " "
                                                "--template " SRCDIR "/enums.xml.template "
                                                SRCDIR "/testenum.h",
                                                &enums, NULL, &result, NULL));
      g_assert_cmpint (result, ==, 0);
      g_assert_true (xfile_set_contents ("org.gtk.test.enums.xml", enums, -1, NULL));
      g_free (enums);

      g_assert_true (xfile_get_contents (SRCDIR "/org.gtk.test.gschema.xml.orig", &schema_text, NULL, NULL));
      g_assert_true (xfile_set_contents ("org.gtk.test.gschema.xml", schema_text, -1, NULL));
      g_free (schema_text);

      g_assert_true (xfile_get_contents (SRCDIR "/org.gtk.test.gschema.override.orig", &override_text, NULL, NULL));
      g_assert_true (xfile_set_contents ("org.gtk.test.gschema.override", override_text, -1, NULL));
      g_free (override_text);

      g_remove ("gschemas.compiled");
      /* #XPL_COMPILE_SCHEMAS is defined in meson.build */
      g_assert_true (g_spawn_command_line_sync (XPL_COMPILE_SCHEMAS " --targetdir=. "
                                                "--schema-file=org.gtk.test.enums.xml "
                                                "--schema-file=org.gtk.test.gschema.xml "
                                                "--override-file=org.gtk.test.gschema.override",
                                                NULL, NULL, &result, NULL));
      g_assert_cmpint (result, ==, 0);

      g_remove ("schema-source/gschemas.compiled");
      g_mkdir ("schema-source", 0777);
      g_assert_true (g_spawn_command_line_sync (XPL_COMPILE_SCHEMAS " --targetdir=schema-source "
                                                "--schema-file=" SRCDIR "/org.gtk.schemasourcecheck.gschema.xml",
                                                NULL, NULL, &result, NULL));
      g_assert_cmpint (result, ==, 0);

      g_remove ("schema-source-corrupt/gschemas.compiled");
      g_mkdir ("schema-source-corrupt", 0777);
      xfile_set_contents ("schema-source-corrupt/gschemas.compiled",
                           (const xchar_t *) gschemas_compiled_corrupt,
                           sizeof (gschemas_compiled_corrupt),
                           &local_error);
      g_assert_no_error (local_error);

      g_remove ("schema-source-empty/gschemas.compiled");
      g_mkdir ("schema-source-empty", 0777);
      xfile_set_contents ("schema-source-empty/gschemas.compiled",
                           "", 0,
                           &local_error);
      g_assert_no_error (local_error);
   }

  g_test_add_func ("/gsettings/basic", test_basic);

  if (!backend_set)
    {
      g_test_add_func ("/gsettings/no-schema", test_no_schema);
      g_test_add_func ("/gsettings/unknown-key", test_unknown_key);
      g_test_add_func ("/gsettings/wrong-type", test_wrong_type);
      g_test_add_func ("/gsettings/wrong-path", test_wrong_path);
      g_test_add_func ("/gsettings/no-path", test_no_path);
    }

  g_test_add_func ("/gsettings/basic-types", test_basic_types);
  g_test_add_func ("/gsettings/complex-types", test_complex_types);
  g_test_add_func ("/gsettings/changes", test_changes);

  g_test_add_func ("/gsettings/l10n", test_l10n);
  g_test_add_func ("/gsettings/l10n-context", test_l10n_context);

  g_test_add_func ("/gsettings/delay-apply", test_delay_apply);
  g_test_add_func ("/gsettings/delay-revert", test_delay_revert);
  g_test_add_func ("/gsettings/delay-child", test_delay_child);
  g_test_add_func ("/gsettings/delay-reset-key", test_delay_reset_key);
  g_test_add_func ("/gsettings/atomic", test_atomic);

  g_test_add_func ("/gsettings/simple-binding", test_simple_binding);
  g_test_add_func ("/gsettings/directional-binding", test_directional_binding);
  g_test_add_func ("/gsettings/custom-binding", test_custom_binding);
  g_test_add_func ("/gsettings/no-change-binding", test_no_change_binding);
  g_test_add_func ("/gsettings/unbinding", test_unbind);
  g_test_add_func ("/gsettings/writable-binding", test_bind_writable);

  if (!backend_set)
    {
      g_test_add_func ("/gsettings/typesafe-binding", test_typesafe_binding);
      g_test_add_func ("/gsettings/no-read-binding", test_no_read_binding);
      g_test_add_func ("/gsettings/no-read-binding/subprocess/fail", test_no_read_binding_fail);
      g_test_add_func ("/gsettings/no-read-binding/subprocess/pass", test_no_read_binding_pass);
      g_test_add_func ("/gsettings/no-write-binding", test_no_write_binding);
      g_test_add_func ("/gsettings/no-write-binding/subprocess/fail", test_no_write_binding_fail);
      g_test_add_func ("/gsettings/no-write-binding/subprocess/pass", test_no_write_binding_pass);
    }

  g_test_add ("/gsettings/keyfile", Fixture, NULL, setup, test_keyfile, teardown);
  g_test_add ("/gsettings/keyfile/explicit-path", Fixture, &keyfile_test_data_explicit_path, setup, test_keyfile_no_path, teardown);
  g_test_add ("/gsettings/keyfile/empty-path", Fixture, &keyfile_test_data_empty_path, setup, test_keyfile_no_path, teardown);
  g_test_add ("/gsettings/keyfile/long-path", Fixture, &keyfile_test_data_long_path, setup, test_keyfile_no_path, teardown);
  g_test_add ("/gsettings/keyfile/outside-root-path", Fixture, NULL, setup, test_keyfile_outside_root_path, teardown);
  g_test_add ("/gsettings/keyfile/no-root-group", Fixture, NULL, setup, test_keyfile_no_root_group, teardown);
  g_test_add_func ("/gsettings/child-schema", test_child_schema);
  g_test_add_func ("/gsettings/strinfo", test_strinfo);
  g_test_add_func ("/gsettings/enums", test_enums);
  g_test_add_func ("/gsettings/enums/subprocess/non-enum-key", test_enums_non_enum_key);
  g_test_add_func ("/gsettings/enums/subprocess/non-enum-value", test_enums_non_enum_value);
  g_test_add_func ("/gsettings/enums/subprocess/range", test_enums_range);
  g_test_add_func ("/gsettings/enums/subprocess/non-flags", test_enums_non_flags);
  g_test_add_func ("/gsettings/flags", test_flags);
  g_test_add_func ("/gsettings/flags/subprocess/non-flags-key", test_flags_non_flags_key);
  g_test_add_func ("/gsettings/flags/subprocess/non-flags-value", test_flags_non_flags_value);
  g_test_add_func ("/gsettings/flags/subprocess/range", test_flags_range);
  g_test_add_func ("/gsettings/flags/subprocess/non-enum", test_flags_non_enum);
  g_test_add_func ("/gsettings/range", test_range);
  g_test_add_func ("/gsettings/range/subprocess/high", test_range_high);
  g_test_add_func ("/gsettings/range/subprocess/low", test_range_low);
  g_test_add_func ("/gsettings/list-items", test_list_items);
  g_test_add_func ("/gsettings/list-schemas", test_list_schemas);
  g_test_add_func ("/gsettings/mapped", test_get_mapped);
  g_test_add_func ("/gsettings/get-range", test_get_range);
  g_test_add_func ("/gsettings/schema-source", test_schema_source);
  g_test_add_func ("/gsettings/schema-list-keys", test_schema_list_keys);
  g_test_add_func ("/gsettings/actions", test_actions);
  g_test_add_func ("/gsettings/null-backend", test_null_backend);
  g_test_add_func ("/gsettings/memory-backend", test_memory_backend);
  g_test_add_func ("/gsettings/read-descriptions", test_read_descriptions);
  g_test_add_func ("/gsettings/test-extended-schema", test_extended_schema);
  g_test_add_func ("/gsettings/default-value", test_default_value);
  g_test_add_func ("/gsettings/per-desktop", test_per_desktop);
  g_test_add_func ("/gsettings/per-desktop/subprocess", test_per_desktop_subprocess);

  result = g_test_run ();

  g_settings_sync ();

  /* FIXME: Due to the way #xsettings_t objects can be used without specifying a
   * backend, the default backend is leaked. In order to be able to run this
   * test under valgrind and get meaningful checking for real leaks, use this
   * hack to drop the final reference to the default #xsettings_backend_t.
   *
   * This should not be used in production code. */
    {
      xsettings_backend_t *backend;

      backend = g_settings_backend_get_default ();
      xobject_unref (backend);  /* reference from the *_get_default() call */
      g_assert_finalize_object (backend);  /* singleton reference owned by GLib */
    }

  return result;
}
