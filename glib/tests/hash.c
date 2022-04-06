/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1999 The Free Software Foundation
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

static int global_array[10000];

static void
fill_hash_table_and_array (xhashtable_t *hash_table)
{
  int i;

  for (i = 0; i < 10000; i++)
    {
      global_array[i] = i;
      xhash_table_insert (hash_table, &global_array[i], &global_array[i]);
    }
}

static void
init_result_array (int result_array[10000])
{
  int i;

  for (i = 0; i < 10000; i++)
    result_array[i] = -1;
}

static void
verify_result_array (int array[10000])
{
  int i;

  for (i = 0; i < 10000; i++)
    g_assert (array[i] == i);
}

static void
handle_pair (xpointer_t key, xpointer_t value, int result_array[10000])
{
  int n;

  g_assert (key == value);

  n = *((int *) value);

  g_assert (n >= 0 && n < 10000);
  g_assert (result_array[n] == -1);

  result_array[n] = n;
}

static xboolean_t
my_hash_callback_remove (xpointer_t key,
                         xpointer_t value,
                         xpointer_t user_data)
{
  int *d = value;

  if ((*d) % 2)
    return TRUE;

  return FALSE;
}

static void
my_hash_callback_remove_test (xpointer_t key,
                              xpointer_t value,
                              xpointer_t user_data)
{
  int *d = value;

  if ((*d) % 2)
    g_assert_not_reached ();
}

static void
my_hash_callback (xpointer_t key,
                  xpointer_t value,
                  xpointer_t user_data)
{
  handle_pair (key, value, user_data);
}

static xuint_t
my_hash (xconstpointer key)
{
  return (xuint_t) *((const xint_t*) key);
}

static xboolean_t
my_hash_equal (xconstpointer a,
               xconstpointer b)
{
  return *((const xint_t*) a) == *((const xint_t*) b);
}



/*
 * This is a simplified version of the pathalias hashing function.
 * Thanks to Steve Belovin and Peter Honeyman
 *
 * hash a string into a long int.  31 bit crc (from andrew appel).
 * the crc table is computed at run time by crcinit() -- we could
 * precompute, but it takes 1 clock tick on a 750.
 *
 * This fast table calculation works only if POLY is a prime polynomial
 * in the field of integers modulo 2.  Since the coefficients of a
 * 32-bit polynomial won't fit in a 32-bit word, the high-order bit is
 * implicit.  IT MUST ALSO BE THE CASE that the coefficients of orders
 * 31 down to 25 are zero.  Happily, we have candidates, from
 * E. J.  Watson, "Primitive Polynomials (Mod 2)", Math. Comp. 16 (1962):
 *      x^32 + x^7 + x^5 + x^3 + x^2 + x^1 + x^0
 *      x^31 + x^3 + x^0
 *
 * We reverse the bits to get:
 *      111101010000000000000000000000001 but drop the last 1
 *         f   5   0   0   0   0   0   0
 *      010010000000000000000000000000001 ditto, for 31-bit crc
 *         4   8   0   0   0   0   0   0
 */

#define POLY 0x48000000L        /* 31-bit polynomial (avoids sign problems) */

static xuint_t CrcTable[128];

/*
 - crcinit - initialize tables for hash function
 */
static void crcinit(void)
{
  int i, j;
  xuint_t sum;

  for (i = 0; i < 128; ++i)
    {
      sum = 0L;
      for (j = 7 - 1; j >= 0; --j)
        if (i & (1 << j))
          sum ^= POLY >> j;
      CrcTable[i] = sum;
    }
}

/*
 - hash - Honeyman's nice hashing function
 */
static xuint_t
honeyman_hash (xconstpointer key)
{
  const xchar_t *name = (const xchar_t *) key;
  xsize_t size;
  xuint_t sum = 0;

  g_assert (name != NULL);
  g_assert (*name != 0);

  size = strlen (name);

  while (size--)
    sum = (sum >> 7) ^ CrcTable[(sum ^ (*name++)) & 0x7f];

  return sum;
}


static xboolean_t
second_hash_cmp (xconstpointer a, xconstpointer b)
{
  return strcmp (a, b) == 0;
}



static xuint_t
one_hash (xconstpointer key)
{
  return 1;
}


static void
not_even_foreach (xpointer_t key,
                  xpointer_t value,
                  xpointer_t user_data)
{
  const char *_key = (const char *) key;
  const char *_value = (const char *) value;
  int i;
  char val [20];

  g_assert (_key != NULL);
  g_assert (*_key != 0);
  g_assert (_value != NULL);
  g_assert (*_value != 0);

  i = atoi (_key);

  sprintf (val, "%d value", i);
  g_assert (strcmp (_value, val) == 0);

  g_assert ((i % 2) != 0);
  g_assert (i != 3);
}

static xboolean_t
remove_even_foreach (xpointer_t key,
                     xpointer_t value,
                     xpointer_t user_data)
{
  const char *_key = (const char *) key;
  const char *_value = (const char *) value;
  int i;
  char val [20];

  g_assert (_key != NULL);
  g_assert (*_key != 0);
  g_assert (_value != NULL);
  g_assert (*_value != 0);

  i = atoi (_key);

  sprintf (val, "%d value", i);
  g_assert (strcmp (_value, val) == 0);

  return ((i % 2) == 0) ? TRUE : FALSE;
}




static void
second_hash_test (xconstpointer d)
{
  xboolean_t simple_hash = GPOINTER_TO_INT (d);

  int       i;
  char      key[20] = "", val[20]="", *v, *orig_key, *orig_val;
  xhashtable_t     *h;
  xboolean_t found;

  crcinit ();

  h = xhash_table_new_full (simple_hash ? one_hash : honeyman_hash,
                             second_hash_cmp,
                             g_free, g_free);
  g_assert (h != NULL);
  for (i = 0; i < 20; i++)
    {
      sprintf (key, "%d", i);
      g_assert (atoi (key) == i);

      sprintf (val, "%d value", i);
      g_assert (atoi (val) == i);

      xhash_table_insert (h, xstrdup (key), xstrdup (val));
    }

  g_assert (xhash_table_size (h) == 20);

  for (i = 0; i < 20; i++)
    {
      sprintf (key, "%d", i);
      g_assert (atoi(key) == i);

      v = (char *) xhash_table_lookup (h, key);

      g_assert (v != NULL);
      g_assert (*v != 0);
      g_assert (atoi (v) == i);
    }

  sprintf (key, "%d", 3);
  xhash_table_remove (h, key);
  g_assert (xhash_table_size (h) == 19);
  xhash_table_foreach_remove (h, remove_even_foreach, NULL);
  g_assert (xhash_table_size (h) == 9);
  xhash_table_foreach (h, not_even_foreach, NULL);

  for (i = 0; i < 20; i++)
    {
      sprintf (key, "%d", i);
      g_assert (atoi(key) == i);

      sprintf (val, "%d value", i);
      g_assert (atoi (val) == i);

      orig_key = orig_val = NULL;
      found = xhash_table_lookup_extended (h, key,
                                            (xpointer_t)&orig_key,
                                            (xpointer_t)&orig_val);
      if ((i % 2) == 0 || i == 3)
        {
          g_assert (!found);
          continue;
        }

      g_assert (found);

      g_assert (orig_key != NULL);
      g_assert (strcmp (key, orig_key) == 0);

      g_assert (orig_val != NULL);
      g_assert (strcmp (val, orig_val) == 0);
    }

  xhash_table_destroy (h);
}

static xboolean_t
find_first (xpointer_t key,
            xpointer_t value,
            xpointer_t user_data)
{
  xint_t *v = value;
  xint_t *test = user_data;
  return (*v == *test);
}

static void
direct_hash_test (void)
{
  xint_t       i, rc;
  xhashtable_t     *h;

  h = xhash_table_new (NULL, NULL);
  g_assert (h != NULL);
  for (i = 1; i <= 20; i++)
    xhash_table_insert (h, GINT_TO_POINTER (i),
                         GINT_TO_POINTER (i + 42));

  g_assert (xhash_table_size (h) == 20);

  for (i = 1; i <= 20; i++)
    {
      rc = GPOINTER_TO_INT (xhash_table_lookup (h, GINT_TO_POINTER (i)));

      g_assert (rc != 0);
      g_assert ((rc - 42) == i);
    }

  xhash_table_destroy (h);
}

static void
direct_hash_test2 (void)
{
  xint_t       i, rc;
  xhashtable_t     *h;

  h = xhash_table_new (g_direct_hash, g_direct_equal);
  g_assert (h != NULL);
  for (i = 1; i <= 20; i++)
    xhash_table_insert (h, GINT_TO_POINTER (i),
                         GINT_TO_POINTER (i + 42));

  g_assert (xhash_table_size (h) == 20);

  for (i = 1; i <= 20; i++)
    {
      rc = GPOINTER_TO_INT (xhash_table_lookup (h, GINT_TO_POINTER (i)));

      g_assert (rc != 0);
      g_assert ((rc - 42) == i);
    }

  xhash_table_destroy (h);
}

static void
int_hash_test (void)
{
  xint_t       i, rc;
  xhashtable_t     *h;
  xint_t     values[20];
  xint_t key;

  h = xhash_table_new (g_int_hash, g_int_equal);
  g_assert (h != NULL);
  for (i = 0; i < 20; i++)
    {
      values[i] = i + 42;
      xhash_table_insert (h, &values[i], GINT_TO_POINTER (i + 42));
    }

  g_assert (xhash_table_size (h) == 20);

  for (i = 0; i < 20; i++)
    {
      key = i + 42;
      rc = GPOINTER_TO_INT (xhash_table_lookup (h, &key));

      g_assert_cmpint (rc, ==, i + 42);
    }

  xhash_table_destroy (h);
}

static void
int64_hash_test (void)
{
  xint_t       i, rc;
  xhashtable_t     *h;
  gint64     values[20];
  gint64 key;

  h = xhash_table_new (g_int64_hash, g_int64_equal);
  g_assert (h != NULL);
  for (i = 0; i < 20; i++)
    {
      values[i] = i + 42;
      xhash_table_insert (h, &values[i], GINT_TO_POINTER (i + 42));
    }

  g_assert (xhash_table_size (h) == 20);

  for (i = 0; i < 20; i++)
    {
      key = i + 42;
      rc = GPOINTER_TO_INT (xhash_table_lookup (h, &key));

      g_assert_cmpint (rc, ==, i + 42);
    }

  xhash_table_destroy (h);
}

static void
double_hash_test (void)
{
  xint_t       i, rc;
  xhashtable_t     *h;
  xdouble_t values[20];
  xdouble_t key;

  h = xhash_table_new (g_double_hash, g_double_equal);
  g_assert (h != NULL);
  for (i = 0; i < 20; i++)
    {
      values[i] = i + 42.5;
      xhash_table_insert (h, &values[i], GINT_TO_POINTER (i + 42));
    }

  g_assert (xhash_table_size (h) == 20);

  for (i = 0; i < 20; i++)
    {
      key = i + 42.5;
      rc = GPOINTER_TO_INT (xhash_table_lookup (h, &key));

      g_assert_cmpint (rc, ==, i + 42);
    }

  xhash_table_destroy (h);
}

static void
string_free (xpointer_t data)
{
  xstring_t *s = data;

  xstring_free (s, TRUE);
}

static void
string_hash_test (void)
{
  xint_t       i, rc;
  xhashtable_t     *h;
  xstring_t *s;

  h = xhash_table_new_full ((GHashFunc)xstring_hash, (GEqualFunc)xstring_equal, string_free, NULL);
  g_assert (h != NULL);
  for (i = 0; i < 20; i++)
    {
      s = xstring_new ("");
      xstring_append_printf (s, "%d", i + 42);
      xstring_append_c (s, '.');
      xstring_prepend_unichar (s, 0x2301);
      xhash_table_insert (h, s, GINT_TO_POINTER (i + 42));
    }

  g_assert (xhash_table_size (h) == 20);

  s = xstring_new ("");
  for (i = 0; i < 20; i++)
    {
      xstring_assign (s, "");
      xstring_append_printf (s, "%d", i + 42);
      xstring_append_c (s, '.');
      xstring_prepend_unichar (s, 0x2301);
      rc = GPOINTER_TO_INT (xhash_table_lookup (h, s));

      g_assert_cmpint (rc, ==, i + 42);
    }

  xstring_free (s, TRUE);
  xhash_table_destroy (h);
}

static void
set_check (xpointer_t key,
           xpointer_t value,
           xpointer_t user_data)
{
  int *pi = user_data;
  if (key != value)
    g_assert_not_reached ();

  g_assert_cmpint (atoi (key) % 7, ==, 2);

  (*pi)++;
}

static void
set_hash_test (void)
{
  xhashtable_t *hash_table =
    xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);
  int i;

  for (i = 2; i < 5000; i += 7)
    {
      char *s = xstrdup_printf ("%d", i);
      g_assert (xhash_table_add (hash_table, s));
    }

  g_assert (!xhash_table_add (hash_table, xstrdup_printf ("%d", 2)));

  i = 0;
  xhash_table_foreach (hash_table, set_check, &i);
  g_assert_cmpint (i, ==, xhash_table_size (hash_table));

  g_assert (xhash_table_contains (hash_table, "2"));
  g_assert (xhash_table_contains (hash_table, "9"));
  g_assert (!xhash_table_contains (hash_table, "a"));

  /* this will cause the hash table to loose set nature */
  g_assert (xhash_table_insert (hash_table, xstrdup ("a"), "b"));
  g_assert (!xhash_table_insert (hash_table, xstrdup ("a"), "b"));

  g_assert (xhash_table_replace (hash_table, xstrdup ("c"), "d"));
  g_assert (!xhash_table_replace (hash_table, xstrdup ("c"), "d"));

  g_assert_cmpstr (xhash_table_lookup (hash_table, "2"), ==, "2");
  g_assert_cmpstr (xhash_table_lookup (hash_table, "a"), ==, "b");

  xhash_table_destroy (hash_table);
}


static void
test_hash_misc (void)
{
  xhashtable_t *hash_table;
  xint_t i;
  xint_t value = 120;
  xint_t *pvalue;
  xlist_t *keys, *values;
  xsize_t keys_len, values_len;
  xhash_table_iter_t iter;
  xpointer_t ikey, ivalue;
  int result_array[10000];
  int n_array[1];

  hash_table = xhash_table_new (my_hash, my_hash_equal);
  fill_hash_table_and_array (hash_table);
  pvalue = xhash_table_find (hash_table, find_first, &value);
  if (!pvalue || *pvalue != value)
    g_assert_not_reached();

  keys = xhash_table_get_keys (hash_table);
  if (!keys)
    g_assert_not_reached ();

  values = xhash_table_get_values (hash_table);
  if (!values)
    g_assert_not_reached ();

  keys_len = xlist_length (keys);
  values_len = xlist_length (values);
  if (values_len != keys_len &&  keys_len != xhash_table_size (hash_table))
    g_assert_not_reached ();

  xlist_free (keys);
  xlist_free (values);

  init_result_array (result_array);
  xhash_table_iter_init (&iter, hash_table);
  for (i = 0; i < 10000; i++)
    {
      g_assert (xhash_table_iter_next (&iter, &ikey, &ivalue));

      handle_pair (ikey, ivalue, result_array);

      if (i % 2)
        xhash_table_iter_remove (&iter);
    }
  g_assert (! xhash_table_iter_next (&iter, &ikey, &ivalue));
  g_assert (xhash_table_size (hash_table) == 5000);
  verify_result_array (result_array);

  fill_hash_table_and_array (hash_table);

  init_result_array (result_array);
  xhash_table_foreach (hash_table, my_hash_callback, result_array);
  verify_result_array (result_array);

  for (i = 0; i < 10000; i++)
    xhash_table_remove (hash_table, &global_array[i]);

  fill_hash_table_and_array (hash_table);

  if (xhash_table_foreach_remove (hash_table, my_hash_callback_remove, NULL) != 5000 ||
      xhash_table_size (hash_table) != 5000)
    g_assert_not_reached();

  xhash_table_foreach (hash_table, my_hash_callback_remove_test, NULL);
  xhash_table_destroy (hash_table);

  hash_table = xhash_table_new (my_hash, my_hash_equal);
  fill_hash_table_and_array (hash_table);

  n_array[0] = 1;

  xhash_table_iter_init (&iter, hash_table);
  for (i = 0; i < 10000; i++)
    {
      g_assert (xhash_table_iter_next (&iter, &ikey, &ivalue));
      xhash_table_iter_replace (&iter, &n_array[0]);
    }

  xhash_table_iter_init (&iter, hash_table);
  for (i = 0; i < 10000; i++)
    {
      g_assert (xhash_table_iter_next (&iter, &ikey, &ivalue));

      g_assert (ivalue == &n_array[0]);
    }

  xhash_table_destroy (hash_table);
}

static xint_t destroy_counter;

static void
value_destroy (xpointer_t value)
{
  destroy_counter++;
}

static void
test_hash_ref (void)
{
  xhashtable_t *h;
  xhash_table_iter_t iter;
  xchar_t *key, *value;
  xboolean_t abc_seen = FALSE;
  xboolean_t cde_seen = FALSE;
  xboolean_t xyz_seen = FALSE;

  h = xhash_table_new_full (xstr_hash, xstr_equal, NULL, value_destroy);
  xhash_table_insert (h, "abc", "ABC");
  xhash_table_insert (h, "cde", "CDE");
  xhash_table_insert (h, "xyz", "XYZ");

  g_assert_cmpint (xhash_table_size (h), == , 3);

  xhash_table_iter_init (&iter, h);

  while (xhash_table_iter_next (&iter, (xpointer_t*)&key, (xpointer_t*)&value))
    {
      if (strcmp (key, "abc") == 0)
        {
          g_assert_cmpstr (value, ==, "ABC");
          abc_seen = TRUE;
          xhash_table_iter_steal (&iter);
        }
      else if (strcmp (key, "cde") == 0)
        {
          g_assert_cmpstr (value, ==, "CDE");
          cde_seen = TRUE;
        }
      else if (strcmp (key, "xyz") == 0)
        {
          g_assert_cmpstr (value, ==, "XYZ");
          xyz_seen = TRUE;
        }
    }
  g_assert_cmpint (destroy_counter, ==, 0);

  g_assert (xhash_table_iter_get_hash_table (&iter) == h);
  g_assert (abc_seen && cde_seen && xyz_seen);
  g_assert_cmpint (xhash_table_size (h), == , 2);

  xhash_table_ref (h);
  xhash_table_destroy (h);
  g_assert_cmpint (xhash_table_size (h), == , 0);
  g_assert_cmpint (destroy_counter, ==, 2);
  xhash_table_insert (h, "uvw", "UVW");
  xhash_table_unref (h);
  g_assert_cmpint (destroy_counter, ==, 3);
}

static xuint_t
null_safe_str_hash (xconstpointer key)
{
  if (key == NULL)
    return 0;
  else
    return xstr_hash (key);
}

static xboolean_t
null_safe_str_equal (xconstpointer a, xconstpointer b)
{
  return xstrcmp0 (a, b) == 0;
}

static void
test_lookup_null_key (void)
{
  xhashtable_t *h;
  xboolean_t res;
  xpointer_t key;
  xpointer_t value;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=642944");

  h = xhash_table_new (null_safe_str_hash, null_safe_str_equal);
  xhash_table_insert (h, "abc", "ABC");

  res = xhash_table_lookup_extended (h, NULL, &key, &value);
  g_assert (!res);

  xhash_table_insert (h, NULL, "NULL");

  res = xhash_table_lookup_extended (h, NULL, &key, &value);
  g_assert (res);
  g_assert_cmpstr (value, ==, "NULL");

  xhash_table_unref (h);
}

static xint_t destroy_key_counter;

static void
key_destroy (xpointer_t key)
{
  destroy_key_counter++;
}

static void
test_remove_all (void)
{
  xhashtable_t *h;
  xboolean_t res;

  h = xhash_table_new_full (xstr_hash, xstr_equal, key_destroy, value_destroy);

  xhash_table_insert (h, "abc", "cde");
  xhash_table_insert (h, "cde", "xyz");
  xhash_table_insert (h, "xyz", "abc");

  destroy_counter = 0;
  destroy_key_counter = 0;

  xhash_table_steal_all (h);
  g_assert_cmpint (destroy_counter, ==, 0);
  g_assert_cmpint (destroy_key_counter, ==, 0);

  /* Test stealing on an empty hash table. */
  xhash_table_steal_all (h);
  g_assert_cmpint (destroy_counter, ==, 0);
  g_assert_cmpint (destroy_key_counter, ==, 0);

  xhash_table_insert (h, "abc", "ABC");
  xhash_table_insert (h, "cde", "CDE");
  xhash_table_insert (h, "xyz", "XYZ");

  res = xhash_table_steal (h, "nosuchkey");
  g_assert (!res);
  g_assert_cmpint (destroy_counter, ==, 0);
  g_assert_cmpint (destroy_key_counter, ==, 0);

  res = xhash_table_steal (h, "xyz");
  g_assert (res);
  g_assert_cmpint (destroy_counter, ==, 0);
  g_assert_cmpint (destroy_key_counter, ==, 0);

  xhash_table_remove_all (h);
  g_assert_cmpint (destroy_counter, ==, 2);
  g_assert_cmpint (destroy_key_counter, ==, 2);

  xhash_table_remove_all (h);
  g_assert_cmpint (destroy_counter, ==, 2);
  g_assert_cmpint (destroy_key_counter, ==, 2);

  xhash_table_unref (h);
}

xhashtable_t *recursive_destruction_table = NULL;
static void
recursive_value_destroy (xpointer_t value)
{
  destroy_counter++;

  if (recursive_destruction_table)
    xhash_table_remove (recursive_destruction_table, value);
}

static void
test_recursive_remove_all_subprocess (void)
{
  xhashtable_t *h;

  h = xhash_table_new_full (xstr_hash, xstr_equal, key_destroy, recursive_value_destroy);
  recursive_destruction_table = h;

  /* Add more items compared to test_remove_all, as it would not fail otherwise. */
  xhash_table_insert (h, "abc", "cde");
  xhash_table_insert (h, "cde", "fgh");
  xhash_table_insert (h, "fgh", "ijk");
  xhash_table_insert (h, "ijk", "lmn");
  xhash_table_insert (h, "lmn", "opq");
  xhash_table_insert (h, "opq", "rst");
  xhash_table_insert (h, "rst", "uvw");
  xhash_table_insert (h, "uvw", "xyz");
  xhash_table_insert (h, "xyz", "abc");

  destroy_counter = 0;
  destroy_key_counter = 0;

  xhash_table_remove_all (h);
  g_assert_cmpint (destroy_counter, ==, 9);
  g_assert_cmpint (destroy_key_counter, ==, 9);

  xhash_table_unref (h);
}

static void
test_recursive_remove_all (void)
{
  g_test_trap_subprocess ("/hash/recursive-remove-all/subprocess", 1000000, 0);
  g_test_trap_assert_passed ();
}

typedef struct {
  xint_t ref_count;
  const xchar_t *key;
} RefCountedKey;

static xuint_t
hash_func (xconstpointer key)
{
  const RefCountedKey *rkey = key;

  return xstr_hash (rkey->key);
}

static xboolean_t
eq_func (xconstpointer a, xconstpointer b)
{
  const RefCountedKey *aa = a;
  const RefCountedKey *bb = b;

  return xstrcmp0 (aa->key, bb->key) == 0;
}

static void
key_unref (xpointer_t data)
{
  RefCountedKey *key = data;

  g_assert (key->ref_count > 0);

  key->ref_count -= 1;

  if (key->ref_count == 0)
    g_free (key);
}

static RefCountedKey *
key_ref (RefCountedKey *key)
{
  key->ref_count += 1;

  return key;
}

static RefCountedKey *
key_new (const xchar_t *key)
{
  RefCountedKey *rkey;

  rkey = g_new (RefCountedKey, 1);

  rkey->ref_count = 1;
  rkey->key = key;

  return rkey;
}

static void
set_ref_hash_test (void)
{
  xhashtable_t *h;
  RefCountedKey *key1;
  RefCountedKey *key2;

  h = xhash_table_new_full (hash_func, eq_func, key_unref, key_unref);

  key1 = key_new ("a");
  key2 = key_new ("a");

  g_assert_cmpint (key1->ref_count, ==, 1);
  g_assert_cmpint (key2->ref_count, ==, 1);

  xhash_table_insert (h, key_ref (key1), key_ref (key1));

  g_assert_cmpint (key1->ref_count, ==, 3);
  g_assert_cmpint (key2->ref_count, ==, 1);

  xhash_table_replace (h, key_ref (key2), key_ref (key2));

  g_assert_cmpint (key1->ref_count, ==, 1);
  g_assert_cmpint (key2->ref_count, ==, 3);

  xhash_table_remove (h, key1);

  g_assert_cmpint (key1->ref_count, ==, 1);
  g_assert_cmpint (key2->ref_count, ==, 1);

  xhash_table_unref (h);

  key_unref (key1);
  key_unref (key2);
}

static xhashtable_t *global_hashtable;

typedef struct {
    xchar_t *string;
    xboolean_t freed;
} FakeFreeData;

static xptr_array_t *fake_free_data;

static void
fake_free (xpointer_t dead)
{
  xuint_t i;

  for (i = 0; i < fake_free_data->len; i++)
    {
      FakeFreeData *ffd = xptr_array_index (fake_free_data, i);

      if (ffd->string == (xchar_t *) dead)
        {
          g_assert (!ffd->freed);
          ffd->freed = TRUE;
          return;
        }
    }

  g_assert_not_reached ();
}

static void
value_destroy_insert (xpointer_t value)
{
  xhash_table_remove_all (global_hashtable);
}

static void
test_destroy_modify (void)
{
  FakeFreeData *ffd;
  xuint_t i;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=650459");

  fake_free_data = xptr_array_new ();

  global_hashtable = xhash_table_new_full (xstr_hash, xstr_equal, fake_free, value_destroy_insert);

  ffd = g_new0 (FakeFreeData, 1);
  ffd->string = xstrdup ("a");
  xptr_array_add (fake_free_data, ffd);
  xhash_table_insert (global_hashtable, ffd->string, "b");

  ffd = g_new0 (FakeFreeData, 1);
  ffd->string = xstrdup ("c");
  xptr_array_add (fake_free_data, ffd);
  xhash_table_insert (global_hashtable, ffd->string, "d");

  ffd = g_new0 (FakeFreeData, 1);
  ffd->string = xstrdup ("e");
  xptr_array_add (fake_free_data, ffd);
  xhash_table_insert (global_hashtable, ffd->string, "f");

  ffd = g_new0 (FakeFreeData, 1);
  ffd->string = xstrdup ("g");
  xptr_array_add (fake_free_data, ffd);
  xhash_table_insert (global_hashtable, ffd->string, "h");

  ffd = g_new0 (FakeFreeData, 1);
  ffd->string = xstrdup ("h");
  xptr_array_add (fake_free_data, ffd);
  xhash_table_insert (global_hashtable, ffd->string, "k");

  ffd = g_new0 (FakeFreeData, 1);
  ffd->string = xstrdup ("a");
  xptr_array_add (fake_free_data, ffd);
  xhash_table_insert (global_hashtable, ffd->string, "c");

  xhash_table_remove (global_hashtable, "c");

  /* that removed everything... */
  for (i = 0; i < fake_free_data->len; i++)
    {
      ffd = xptr_array_index (fake_free_data, i);

      g_assert (ffd->freed);
      g_free (ffd->string);
      g_free (ffd);
    }

  xptr_array_unref (fake_free_data);

  /* ... so this is a no-op */
  xhash_table_remove (global_hashtable, "e");

  xhash_table_unref (global_hashtable);
}

static xboolean_t
find_str (xpointer_t key, xpointer_t value, xpointer_t data)
{
  return xstr_equal (key, data);
}

static void
test_find (void)
{
  xhashtable_t *hash;
  const xchar_t *value;

  hash = xhash_table_new (xstr_hash, xstr_equal);

  xhash_table_insert (hash, "a", "A");
  xhash_table_insert (hash, "b", "B");
  xhash_table_insert (hash, "c", "C");
  xhash_table_insert (hash, "d", "D");
  xhash_table_insert (hash, "e", "E");
  xhash_table_insert (hash, "f", "F");

  value = xhash_table_find (hash, find_str, "a");
  g_assert_cmpstr (value, ==, "A");

  value = xhash_table_find (hash, find_str, "b");
  g_assert_cmpstr (value, ==, "B");

  value = xhash_table_find (hash, find_str, "c");
  g_assert_cmpstr (value, ==, "C");

  value = xhash_table_find (hash, find_str, "d");
  g_assert_cmpstr (value, ==, "D");

  value = xhash_table_find (hash, find_str, "e");
  g_assert_cmpstr (value, ==, "E");

  value = xhash_table_find (hash, find_str, "f");
  g_assert_cmpstr (value, ==, "F");

  value = xhash_table_find (hash, find_str, "0");
  g_assert (value == NULL);

  xhash_table_unref (hash);
}

xboolean_t seen_key[6];

static void
foreach_func (xpointer_t key, xpointer_t value, xpointer_t data)
{
  seen_key[((char*)key)[0] - 'a'] = TRUE;
}

static void
test_foreach (void)
{
  xhashtable_t *hash;
  xint_t i;

  hash = xhash_table_new (xstr_hash, xstr_equal);

  xhash_table_insert (hash, "a", "A");
  xhash_table_insert (hash, "b", "B");
  xhash_table_insert (hash, "c", "C");
  xhash_table_insert (hash, "d", "D");
  xhash_table_insert (hash, "e", "E");
  xhash_table_insert (hash, "f", "F");

  for (i = 0; i < 6; i++)
    seen_key[i] = FALSE;

  xhash_table_foreach (hash, foreach_func, NULL);

  for (i = 0; i < 6; i++)
    g_assert (seen_key[i]);

  xhash_table_unref (hash);
}

static xboolean_t
foreach_steal_func (xpointer_t key, xpointer_t value, xpointer_t data)
{
  xhashtable_t *hash2 = data;

  if (strstr ("ace", (xchar_t*)key))
    {
      xhash_table_insert (hash2, key, value);
      return TRUE;
    }

  return FALSE;
}


static void
test_foreach_steal (void)
{
  xhashtable_t *hash;
  xhashtable_t *hash2;

  hash = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);
  hash2 = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);

  xhash_table_insert (hash, xstrdup ("a"), xstrdup ("A"));
  xhash_table_insert (hash, xstrdup ("b"), xstrdup ("B"));
  xhash_table_insert (hash, xstrdup ("c"), xstrdup ("C"));
  xhash_table_insert (hash, xstrdup ("d"), xstrdup ("D"));
  xhash_table_insert (hash, xstrdup ("e"), xstrdup ("E"));
  xhash_table_insert (hash, xstrdup ("f"), xstrdup ("F"));

  xhash_table_foreach_steal (hash, foreach_steal_func, hash2);

  g_assert_cmpint (xhash_table_size (hash), ==, 3);
  g_assert_cmpint (xhash_table_size (hash2), ==, 3);

  g_assert_cmpstr (xhash_table_lookup (hash2, "a"), ==, "A");
  g_assert_cmpstr (xhash_table_lookup (hash, "b"), ==, "B");
  g_assert_cmpstr (xhash_table_lookup (hash2, "c"), ==, "C");
  g_assert_cmpstr (xhash_table_lookup (hash, "d"), ==, "D");
  g_assert_cmpstr (xhash_table_lookup (hash2, "e"), ==, "E");
  g_assert_cmpstr (xhash_table_lookup (hash, "f"), ==, "F");

  xhash_table_unref (hash);
  xhash_table_unref (hash2);
}

/* Test xhash_table_steal_extended() works properly with existing and
 * non-existing keys. */
static void
test_steal_extended (void)
{
  xhashtable_t *hash;
  xchar_t *stolen_key = NULL, *stolen_value = NULL;

  hash = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);

  xhash_table_insert (hash, xstrdup ("a"), xstrdup ("A"));
  xhash_table_insert (hash, xstrdup ("b"), xstrdup ("B"));
  xhash_table_insert (hash, xstrdup ("c"), xstrdup ("C"));
  xhash_table_insert (hash, xstrdup ("d"), xstrdup ("D"));
  xhash_table_insert (hash, xstrdup ("e"), xstrdup ("E"));
  xhash_table_insert (hash, xstrdup ("f"), xstrdup ("F"));

  g_assert_true (xhash_table_steal_extended (hash, "a",
                                              (xpointer_t *) &stolen_key,
                                              (xpointer_t *) &stolen_value));
  g_assert_cmpstr (stolen_key, ==, "a");
  g_assert_cmpstr (stolen_value, ==, "A");
  g_clear_pointer (&stolen_key, g_free);
  g_clear_pointer (&stolen_value, g_free);

  g_assert_cmpuint (xhash_table_size (hash), ==, 5);

  g_assert_false (xhash_table_steal_extended (hash, "a",
                                               (xpointer_t *) &stolen_key,
                                               (xpointer_t *) &stolen_value));
  g_assert_null (stolen_key);
  g_assert_null (stolen_value);

  g_assert_false (xhash_table_steal_extended (hash, "never a key",
                                               (xpointer_t *) &stolen_key,
                                               (xpointer_t *) &stolen_value));
  g_assert_null (stolen_key);
  g_assert_null (stolen_value);

  g_assert_cmpuint (xhash_table_size (hash), ==, 5);

  xhash_table_unref (hash);
}

/* Test that passing %NULL to the optional xhash_table_steal_extended()
 * arguments works. */
static void
test_steal_extended_optional (void)
{
  xhashtable_t *hash;
  const xchar_t *stolen_key = NULL, *stolen_value = NULL;

  hash = xhash_table_new_full (xstr_hash, xstr_equal, NULL, NULL);

  xhash_table_insert (hash, "b", "B");
  xhash_table_insert (hash, "c", "C");
  xhash_table_insert (hash, "d", "D");
  xhash_table_insert (hash, "e", "E");
  xhash_table_insert (hash, "f", "F");

  g_assert_true (xhash_table_steal_extended (hash, "b",
                                              (xpointer_t *) &stolen_key,
                                              NULL));
  g_assert_cmpstr (stolen_key, ==, "b");

  g_assert_cmpuint (xhash_table_size (hash), ==, 4);

  g_assert_false (xhash_table_steal_extended (hash, "b",
                                               (xpointer_t *) &stolen_key,
                                               NULL));
  g_assert_null (stolen_key);

  g_assert_true (xhash_table_steal_extended (hash, "c",
                                              NULL,
                                              (xpointer_t *) &stolen_value));
  g_assert_cmpstr (stolen_value, ==, "C");

  g_assert_cmpuint (xhash_table_size (hash), ==, 3);

  g_assert_false (xhash_table_steal_extended (hash, "c",
                                               NULL,
                                               (xpointer_t *) &stolen_value));
  g_assert_null (stolen_value);

  g_assert_true (xhash_table_steal_extended (hash, "d", NULL, NULL));

  g_assert_cmpuint (xhash_table_size (hash), ==, 2);

  g_assert_false (xhash_table_steal_extended (hash, "d", NULL, NULL));

  g_assert_cmpuint (xhash_table_size (hash), ==, 2);

  xhash_table_unref (hash);
}

/* Test xhash_table_lookup_extended() works with its optional parameters
 * sometimes set to %NULL. */
static void
test_lookup_extended (void)
{
  xhashtable_t *hash;
  const xchar_t *original_key = NULL, *value = NULL;

  hash = xhash_table_new_full (xstr_hash, xstr_equal, g_free, g_free);

  xhash_table_insert (hash, xstrdup ("a"), xstrdup ("A"));
  xhash_table_insert (hash, xstrdup ("b"), xstrdup ("B"));
  xhash_table_insert (hash, xstrdup ("c"), xstrdup ("C"));
  xhash_table_insert (hash, xstrdup ("d"), xstrdup ("D"));
  xhash_table_insert (hash, xstrdup ("e"), xstrdup ("E"));
  xhash_table_insert (hash, xstrdup ("f"), xstrdup ("F"));

  g_assert_true (xhash_table_lookup_extended (hash, "a",
                                               (xpointer_t *) &original_key,
                                               (xpointer_t *) &value));
  g_assert_cmpstr (original_key, ==, "a");
  g_assert_cmpstr (value, ==, "A");

  g_assert_true (xhash_table_lookup_extended (hash, "b",
                                               NULL,
                                               (xpointer_t *) &value));
  g_assert_cmpstr (value, ==, "B");

  g_assert_true (xhash_table_lookup_extended (hash, "c",
                                               (xpointer_t *) &original_key,
                                               NULL));
  g_assert_cmpstr (original_key, ==, "c");

  g_assert_true (xhash_table_lookup_extended (hash, "d", NULL, NULL));

  g_assert_false (xhash_table_lookup_extended (hash, "not a key",
                                                (xpointer_t *) &original_key,
                                                (xpointer_t *) &value));
  g_assert_null (original_key);
  g_assert_null (value);

  g_assert_false (xhash_table_lookup_extended (hash, "not a key",
                                                NULL,
                                                (xpointer_t *) &value));
  g_assert_null (value);

  g_assert_false (xhash_table_lookup_extended (hash, "not a key",
                                                (xpointer_t *) &original_key,
                                                NULL));
  g_assert_null (original_key);

  g_assert_false (xhash_table_lookup_extended (hash, "not a key", NULL, NULL));

  xhash_table_unref (hash);
}

static void
inc_state (xpointer_t user_data)
{
  int *state = user_data;
  g_assert_cmpint (*state, ==, 0);
  *state = 1;
}

static void
test_new_similar (void)
{
  xhashtable_t *hash1;
  xhashtable_t *hash2;
  int state1;
  int state2;

  hash1 = xhash_table_new_full (xstr_hash, xstr_equal,
                                 g_free, inc_state);
  state1 = 0;
  xhash_table_insert (hash1,
                       xstrdup ("test"),
                       &state1);
  g_assert_true (xhash_table_lookup (hash1, "test") == &state1);

  hash2 = xhash_table_new_similar (hash1);

  g_assert_true (xhash_table_lookup (hash1, "test") == &state1);
  g_assert_null (xhash_table_lookup (hash2, "test"));

  state2 = 0;
  xhash_table_insert (hash2, xstrdup ("test"), &state2);
  g_assert_true (xhash_table_lookup (hash2, "test") == &state2);
  xhash_table_remove (hash2, "test");
  g_assert_cmpint (state2, ==, 1);

  g_assert_cmpint (state1, ==, 0);
  xhash_table_remove (hash1, "test");
  g_assert_cmpint (state1, ==, 1);

  xhash_table_unref (hash1);
  xhash_table_unref (hash2);
}

struct _GHashTable
{
  xsize_t            size;
  xint_t             mod;
  xuint_t            mask;
  xint_t             nnodes;
  xint_t             noccupied;  /* nnodes + tombstones */

  xuint_t            have_big_keys : 1;
  xuint_t            have_bixvalues : 1;

  xpointer_t        *keys;
  xuint_t           *hashes;
  xpointer_t        *values;

  GHashFunc        hash_func;
  GEqualFunc       key_equal_func;
  xint_t             ref_count;  /* (atomic) */

#ifndef G_DISABLE_ASSERT
  int              version;
#endif
  xdestroy_notify_t   key_destroy_func;
  xdestroy_notify_t   value_destroy_func;
};

static void
count_keys (xhashtable_t *h, xint_t *unused, xint_t *occupied, xint_t *tombstones)
{
  xsize_t i;

  *unused = 0;
  *occupied = 0;
  *tombstones = 0;
  for (i = 0; i < h->size; i++)
    {
      if (h->hashes[i] == 0)
        (*unused)++;
      else if (h->hashes[i] == 1)
        (*tombstones)++;
      else
        (*occupied)++;
    }
}

#define BIG_ENTRY_SIZE (SIZEOF_VOID_P)
#define SMALL_ENTRY_SIZE (SIZEOF_INT)

#if SMALL_ENTRY_SIZE < BIG_ENTRY_SIZE
# define USE_SMALL_ARRAYS
#endif

static xpointer_t
fetch_key_or_value (xpointer_t a, xuint_t index, xboolean_t is_big)
{
#ifdef USE_SMALL_ARRAYS
  return is_big ? *(((xpointer_t *) a) + index) : GUINT_TO_POINTER (*(((xuint_t *) a) + index));
#else
  return *(((xpointer_t *) a) + index);
#endif
}

static void
check_data (xhashtable_t *h)
{
  xsize_t i;

  for (i = 0; i < h->size; i++)
    {
      if (h->hashes[i] >= 2)
        {
          g_assert_cmpint (h->hashes[i], ==, h->hash_func (fetch_key_or_value (h->keys, i, h->have_big_keys)));
        }
    }
}

static void
check_consistency (xhashtable_t *h)
{
  xint_t unused;
  xint_t occupied;
  xint_t tombstones;

  count_keys (h, &unused, &occupied, &tombstones);

  g_assert_cmpint (occupied, ==, h->nnodes);
  g_assert_cmpint (occupied + tombstones, ==, h->noccupied);
  g_assert_cmpint (occupied + tombstones + unused, ==, h->size);

  check_data (h);
}

static void
check_counts (xhashtable_t *h, xint_t occupied, xint_t tombstones)
{
  g_assert_cmpint (occupied, ==, h->nnodes);
  g_assert_cmpint (occupied + tombstones, ==, h->noccupied);
}

static void
trivial_key_destroy (xpointer_t key)
{
}

static void
test_internal_consistency (void)
{
  xhashtable_t *h;

  h = xhash_table_new_full (xstr_hash, xstr_equal, trivial_key_destroy, NULL);

  check_counts (h, 0, 0);
  check_consistency (h);

  xhash_table_insert (h, "a", "A");
  xhash_table_insert (h, "b", "B");
  xhash_table_insert (h, "c", "C");
  xhash_table_insert (h, "d", "D");
  xhash_table_insert (h, "e", "E");
  xhash_table_insert (h, "f", "F");

  check_counts (h, 6, 0);
  check_consistency (h);

  xhash_table_remove (h, "a");
  check_counts (h, 5, 1);
  check_consistency (h);

  xhash_table_remove (h, "b");
  check_counts (h, 4, 2);
  check_consistency (h);

  xhash_table_insert (h, "c", "c");
  check_counts (h, 4, 2);
  check_consistency (h);

  xhash_table_insert (h, "a", "A");
  check_counts (h, 5, 1);
  check_consistency (h);

  xhash_table_remove_all (h);
  check_counts (h, 0, 0);
  check_consistency (h);

  xhash_table_unref (h);
}

static void
my_key_free (xpointer_t v)
{
  xchar_t *s = v;
  g_assert (s[0] != 'x');
  s[0] = 'x';
  g_free (v);
}

static void
my_value_free (xpointer_t v)
{
  xchar_t *s = v;
  g_assert (s[0] != 'y');
  s[0] = 'y';
  g_free (v);
}

static void
test_iter_replace (void)
{
  xhashtable_t *h;
  xhash_table_iter_t iter;
  xpointer_t k, v;
  xchar_t *s;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=662544");

  h = xhash_table_new_full (xstr_hash, xstr_equal, my_key_free, my_value_free);

  xhash_table_insert (h, xstrdup ("A"), xstrdup ("a"));
  xhash_table_insert (h, xstrdup ("B"), xstrdup ("b"));
  xhash_table_insert (h, xstrdup ("C"), xstrdup ("c"));

  xhash_table_iter_init (&iter, h);

  while (xhash_table_iter_next (&iter, &k, &v))
    {
       s = (xchar_t*)v;
       g_assert (g_ascii_islower (s[0]));
       xhash_table_iter_replace (&iter, xstrdup (k));
    }

  xhash_table_unref (h);
}

static void
replace_first_character (xchar_t *string)
{
  string[0] = 'b';
}

static void
test_set_insert_corruption (void)
{
  xhashtable_t *hash_table =
    xhash_table_new_full (xstr_hash, xstr_equal,
        (xdestroy_notify_t) replace_first_character, NULL);
  xhash_table_iter_t iter;
  xchar_t a[] = "foo";
  xchar_t b[] = "foo";
  xpointer_t key, value;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=692815");

  xhash_table_insert (hash_table, a, a);
  g_assert (xhash_table_contains (hash_table, "foo"));

  xhash_table_insert (hash_table, b, b);

  g_assert_cmpuint (xhash_table_size (hash_table), ==, 1);
  xhash_table_iter_init (&iter, hash_table);
  if (!xhash_table_iter_next (&iter, &key, &value))
    g_assert_not_reached();

  /* per the documentation to xhash_table_insert(), 'b' has now been freed,
   * and the sole key in 'hash_table' should be 'a'.
   */
  g_assert (key != b);
  g_assert (key == a);

  g_assert_cmpstr (b, ==, "boo");

  /* xhash_table_insert() also says that the value should now be 'b',
   * which is probably not what the caller intended but is precisely what they
   * asked for.
   */
  g_assert (value == b);

  /* even though the hash has now been de-set-ified: */
  g_assert (xhash_table_contains (hash_table, "foo"));

  xhash_table_unref (hash_table);
}

static void
test_set_to_strv (void)
{
  xhashtable_t *set;
  xchar_t **strv;
  xuint_t n;

  set = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);
  xhash_table_add (set, xstrdup ("xyz"));
  xhash_table_add (set, xstrdup ("xyz"));
  xhash_table_add (set, xstrdup ("abc"));
  strv = (xchar_t **) xhash_table_get_keys_as_array (set, &n);
  xhash_table_steal_all (set);
  xhash_table_unref (set);
  g_assert_cmpint (n, ==, 2);
  n = xstrv_length (strv);
  g_assert_cmpint (n, ==, 2);
  if (xstr_equal (strv[0], "abc"))
    g_assert_cmpstr (strv[1], ==, "xyz");
  else
    {
    g_assert_cmpstr (strv[0], ==, "xyz");
    g_assert_cmpstr (strv[1], ==, "abc");
    }
  xstrfreev (strv);
}

static xboolean_t
is_prime (xuint_t p)
{
  xuint_t i;

  if (p % 2 == 0)
    return FALSE;

  i = 3;
  while (TRUE)
    {
      if (i * i > p)
        return TRUE;

      if (p % i == 0)
        return FALSE;

      i += 2;
    }
}

static void
test_primes (void)
{
  xuint_t p, q;
  xdouble_t r, min, max;

  max = 1.0;
  min = 10.0;
  q = 1;
  while (1) {
    p = q;
    q = g_spaced_primes_closest (p);
    g_assert (is_prime (q));
    if (p == 1) continue;
    if (q == p) break;
    r = q / (xdouble_t) p;
    min = MIN (min, r);
    max = MAX (max, r);
  };

  g_assert_cmpfloat (1.3, <, min);
  g_assert_cmpfloat (max, <, 2.0);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/hash/misc", test_hash_misc);
  g_test_add_data_func ("/hash/one", GINT_TO_POINTER (TRUE), second_hash_test);
  g_test_add_data_func ("/hash/honeyman", GINT_TO_POINTER (FALSE), second_hash_test);
  g_test_add_func ("/hash/direct", direct_hash_test);
  g_test_add_func ("/hash/direct2", direct_hash_test2);
  g_test_add_func ("/hash/int", int_hash_test);
  g_test_add_func ("/hash/int64", int64_hash_test);
  g_test_add_func ("/hash/double", double_hash_test);
  g_test_add_func ("/hash/string", string_hash_test);
  g_test_add_func ("/hash/set", set_hash_test);
  g_test_add_func ("/hash/set-ref", set_ref_hash_test);
  g_test_add_func ("/hash/ref", test_hash_ref);
  g_test_add_func ("/hash/remove-all", test_remove_all);
  g_test_add_func ("/hash/recursive-remove-all", test_recursive_remove_all);
  g_test_add_func ("/hash/recursive-remove-all/subprocess", test_recursive_remove_all_subprocess);
  g_test_add_func ("/hash/find", test_find);
  g_test_add_func ("/hash/foreach", test_foreach);
  g_test_add_func ("/hash/foreach-steal", test_foreach_steal);
  g_test_add_func ("/hash/steal-extended", test_steal_extended);
  g_test_add_func ("/hash/steal-extended/optional", test_steal_extended_optional);
  g_test_add_func ("/hash/lookup-extended", test_lookup_extended);
  g_test_add_func ("/hash/new-similar", test_new_similar);

  /* tests for individual bugs */
  g_test_add_func ("/hash/lookup-null-key", test_lookup_null_key);
  g_test_add_func ("/hash/destroy-modify", test_destroy_modify);
  g_test_add_func ("/hash/consistency", test_internal_consistency);
  g_test_add_func ("/hash/iter-replace", test_iter_replace);
  g_test_add_func ("/hash/set-insert-corruption", test_set_insert_corruption);
  g_test_add_func ("/hash/set-to-strv", test_set_to_strv);
  g_test_add_func ("/hash/primes", test_primes);

  return g_test_run ();

}
