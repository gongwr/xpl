/* XPL - Library of useful routines for C programming
 * Copyright (C) 2010 Ryan Lortie
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

static void
test_listenv (void)
{
  xhashtable_t *table;
  xchar_t **list;
  xint_t i;

  table = xhash_table_new_full (xstr_hash, xstr_equal,
                                 g_free, g_free);

  list = g_get_environ ();
  for (i = 0; list[i]; i++)
    {
      xchar_t **parts;

      parts = xstrsplit (list[i], "=", 2);
      g_assert_null (xhash_table_lookup (table, parts[0]));
      if (xstrcmp0 (parts[0], ""))
        xhash_table_insert (table, parts[0], parts[1]);
      g_free (parts);
    }
  xstrfreev (list);

  g_assert_cmpint (xhash_table_size (table), >, 0);

  list = g_listenv ();
  for (i = 0; list[i]; i++)
    {
      const xchar_t *expected;
      const xchar_t *value;

      expected = xhash_table_lookup (table, list[i]);
      value = g_getenv (list[i]);
      g_assert_cmpstr (value, ==, expected);
      xhash_table_remove (table, list[i]);
    }
  g_assert_cmpint (xhash_table_size (table), ==, 0);
  xhash_table_unref (table);
  xstrfreev (list);
}

static void
test_getenv (void)
{
  const xchar_t *data;
  const xchar_t *variable = "TEST_G_SETENV";
  const xchar_t *value1 = "works";
  const xchar_t *value2 = "again";

  /* Check that TEST_G_SETENV is not already set */
  g_assert_null (g_getenv (variable));

  /* Check if g_setenv() failed */
  g_assert_cmpint (g_setenv (variable, value1, TRUE), !=, 0);

  data = g_getenv (variable);
  g_assert_nonnull (data);
  g_assert_cmpstr (data, ==, value1);

  g_assert_cmpint (g_setenv (variable, value2, FALSE), !=, 0);

  data = g_getenv (variable);
  g_assert_nonnull (data);
  g_assert_cmpstr (data, !=, value2);
  g_assert_cmpstr (data, ==, value1);

  g_assert_cmpint (g_setenv (variable, value2, TRUE), !=, 0);

  data = g_getenv (variable);
  g_assert_nonnull (data);
  g_assert_cmpstr (data, !=, value1);
  g_assert_cmpstr (data, ==, value2);

  g_unsetenv (variable);
  g_assert_null (g_getenv (variable));

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      g_assert_false (g_setenv (NULL, "baz", TRUE));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      g_assert_false (g_setenv ("foo", NULL, TRUE));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* == NULL*");
      g_assert_false (g_setenv ("foo=bar", "baz", TRUE));
      g_test_assert_expected_messages ();
    }

  g_assert_true (g_setenv ("foo", "bar=baz", TRUE));

  /* Different OSs return different values; some return NULL because the key
   * is invalid, but some are happy to return what we set above. */
  data = g_getenv ("foo=bar");
  if (data != NULL)
    g_assert_cmpstr (data, ==, "baz");

  data = g_getenv ("foo");
  g_assert_cmpstr (data, ==, "bar=baz");

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      g_unsetenv (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* == NULL*");
      g_unsetenv ("foo=bar");
      g_test_assert_expected_messages ();
    }

  g_unsetenv ("foo");
  g_assert_null (g_getenv ("foo"));
}

static void
test_setenv (void)
{
  const xchar_t *var, *value;

  var = "NOSUCHENVVAR";
  value = "value1";

  g_assert_null (g_getenv (var));
  g_setenv (var, value, FALSE);
  g_assert_cmpstr (g_getenv (var), ==, value);
  g_assert_true (g_setenv (var, "value2", FALSE));
  g_assert_cmpstr (g_getenv (var), ==, value);
  g_assert_true (g_setenv (var, "value2", TRUE));
  g_assert_cmpstr (g_getenv (var), ==, "value2");
  g_unsetenv (var);
  g_assert_null (g_getenv (var));
}

static void
test_environ_array (void)
{
  xchar_t **env;
  const xchar_t *value;

  env = g_new (xchar_t *, 1);
  env[0] = NULL;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      g_environ_getenv (env, NULL);
      g_test_assert_expected_messages ();
    }

  value = g_environ_getenv (env, "foo");
  g_assert_null (value);

  if (g_test_undefined ())
    {
      xchar_t **undefined_env;

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      undefined_env = g_environ_setenv (env, NULL, "bar", TRUE);
      g_test_assert_expected_messages ();
      xstrfreev (undefined_env);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* == NULL*");
      undefined_env = g_environ_setenv (env, "foo=fuz", "bar", TRUE);
      g_test_assert_expected_messages ();
      xstrfreev (undefined_env);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      undefined_env = g_environ_setenv (env, "foo", NULL, TRUE);
      g_test_assert_expected_messages ();
      xstrfreev (undefined_env);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* != NULL*");
      undefined_env = g_environ_unsetenv (env, NULL);
      g_test_assert_expected_messages ();
      xstrfreev (undefined_env);
    }

  env = g_environ_setenv (env, "foo", "bar", TRUE);
  value = g_environ_getenv (env, "foo");
  g_assert_cmpstr (value, ==, "bar");

  env = g_environ_setenv (env, "foo2", "bar2", FALSE);
  value = g_environ_getenv (env, "foo");
  g_assert_cmpstr (value, ==, "bar");
  value = g_environ_getenv (env, "foo2");
  g_assert_cmpstr (value, ==, "bar2");

  env = g_environ_setenv (env, "foo", "x", FALSE);
  value = g_environ_getenv (env, "foo");
  g_assert_cmpstr (value, ==, "bar");

  env = g_environ_setenv (env, "foo", "x", TRUE);
  value = g_environ_getenv (env, "foo");
  g_assert_cmpstr (value, ==, "x");

  env = g_environ_unsetenv (env, "foo2");
  value = g_environ_getenv (env, "foo2");
  g_assert_null (value);

  xstrfreev (env);
}

static void
test_environ_null (void)
{
  xchar_t **env;
  const xchar_t *value;

  env = NULL;

  value = g_environ_getenv (env, "foo");
  g_assert_null (value);

  env = g_environ_setenv (NULL, "foo", "bar", TRUE);
  g_assert_nonnull (env);
  xstrfreev (env);

  env = g_environ_unsetenv (NULL, "foo");
  g_assert_null (env);
}

static void
test_environ_case (void)
{
  xchar_t **env;
  const xchar_t *value;

  env = NULL;

  env = g_environ_setenv (env, "foo", "bar", TRUE);
  value = g_environ_getenv (env, "foo");
  g_assert_cmpstr (value, ==, "bar");

  value = g_environ_getenv (env, "foo_t");
#ifdef G_OS_WIN32
  g_assert_cmpstr (value, ==, "bar");
#else
  g_assert_null (value);
#endif

  env = g_environ_setenv (env, "FOO", "x", TRUE);
  value = g_environ_getenv (env, "foo");
#ifdef G_OS_WIN32
  g_assert_cmpstr (value, ==, "x");
#else
  g_assert_cmpstr (value, ==, "bar");
#endif

  env = g_environ_unsetenv (env, "foo_t");
  value = g_environ_getenv (env, "foo");
#ifdef G_OS_WIN32
  g_assert_null (value);
#else
  g_assert_cmpstr (value, ==, "bar");
#endif

  xstrfreev (env);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/environ/listenv", test_listenv);
  g_test_add_func ("/environ/getenv", test_getenv);
  g_test_add_func ("/environ/setenv", test_setenv);
  g_test_add_func ("/environ/array", test_environ_array);
  g_test_add_func ("/environ/null", test_environ_null);
  g_test_add_func ("/environ/case", test_environ_case);

  return g_test_run ();
}
