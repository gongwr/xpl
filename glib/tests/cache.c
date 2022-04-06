/* Copyright (C) 2011 Red Hat, Inc.
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

/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

static xint_t value_create_count = 0;
static xint_t value_destroy_count = 0;

static xpointer_t
value_create (xpointer_t key)
{
  xint_t *value;

  value_create_count++;

  value = g_new (xint_t, 1);
  *value = *(xint_t*)key * 2;

  return value;
}

static void
value_destroy (xpointer_t value)
{
  value_destroy_count++;
  g_free (value);
}

static xpointer_t
key_dup (xpointer_t key)
{
  xint_t *newkey;

  newkey = g_new (xint_t, 1);
  *newkey = *(xint_t*)key;

  return newkey;
}

static void
key_destroy (xpointer_t key)
{
  g_free (key);
}

static xuint_t
key_hash (xconstpointer key)
{
  return *(xuint_t*)key;
}

static xuint_t
value_hash (xconstpointer value)
{
  return *(xuint_t*)value;
}

static xboolean_t
key_equal (xconstpointer key1, xconstpointer key2)
{
  return *(xint_t*)key1 == *(xint_t*)key2;
}

static void
key_foreach (xpointer_t valuep, xpointer_t keyp, xpointer_t data)
{
  xint_t *count = data;
  xint_t *key = keyp;

  (*count)++;

  g_assert_cmpint (*key, ==, 2);
}

static void
value_foreach (xpointer_t keyp, xpointer_t nodep, xpointer_t data)
{
  xint_t *count = data;
  xint_t *key = keyp;

  (*count)++;

  g_assert_cmpint (*key, ==, 2);
}

static void
test_cache_basic (void)
{
  GCache *c;
  xint_t *key;
  xint_t *value;
  xint_t count;

  value_create_count = 0;
  value_destroy_count = 0;

  c = g_cache_new (value_create, value_destroy,
                   key_dup, key_destroy,
                   key_hash, value_hash, key_equal);

  key = g_new (xint_t, 1);
  *key = 2;

  value = g_cache_insert (c, key);
  g_assert_cmpint (*value, ==, 4);
  g_assert_cmpint (value_create_count, ==, 1);
  g_assert_cmpint (value_destroy_count, ==, 0);

  count = 0;
  g_cache_key_foreach (c, key_foreach, &count);
  g_assert_cmpint (count, ==, 1);

  count = 0;
  g_cache_value_foreach (c, value_foreach, &count);
  g_assert_cmpint (count, ==, 1);

  value = g_cache_insert (c, key);
  g_assert_cmpint (*value, ==, 4);
  g_assert_cmpint (value_create_count, ==, 1);
  g_assert_cmpint (value_destroy_count, ==, 0);

  g_cache_remove (c, value);
  g_assert_cmpint (value_create_count, ==, 1);
  g_assert_cmpint (value_destroy_count, ==, 0);

  g_cache_remove (c, value);
  g_assert_cmpint (value_create_count, ==, 1);
  g_assert_cmpint (value_destroy_count, ==, 1);

  value = g_cache_insert (c, key);
  g_assert_cmpint (*value, ==, 4);
  g_assert_cmpint (value_create_count, ==, 2);
  g_assert_cmpint (value_destroy_count, ==, 1);

  g_cache_remove (c, value);
  g_cache_destroy (c);
  g_free (key);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/cache/basic", test_cache_basic);

  return g_test_run ();

}
