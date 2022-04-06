/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glib.h"

/* Test data to be passed to any function which calls g_array_new(), providing
 * the parameters for that call. Most #xarray_t tests should be repeated for all
 * possible values of #ArrayTestData. */
typedef struct
{
  xboolean_t zero_terminated;
  xboolean_t clear_;
} ArrayTestData;

/* Assert that @garray contains @n_expected_elements as given in @expected_data.
 * @garray must contain #xint_t elements. */
static void
assert_int_array_equal (xarray_t     *garray,
                        const xint_t *expected_data,
                        xsize_t       n_expected_elements)
{
  xsize_t i;

  g_assert_cmpuint (garray->len, ==, n_expected_elements);
  for (i = 0; i < garray->len; i++)
    g_assert_cmpint (g_array_index (garray, xint_t, i), ==, expected_data[i]);
}

/* Iff config->zero_terminated is %TRUE, assert that the final element of
 * @garray is zero. @garray must contain #xint_t elements. */
static void
assert_int_array_zero_terminated (const ArrayTestData *config,
                                  xarray_t              *garray)
{
  if (config->zero_terminated)
    {
      xint_t *data = (xint_t *) garray->data;
      g_assert_cmpint (data[garray->len], ==, 0);
    }
}

static void
sum_up (xpointer_t data,
	xpointer_t user_data)
{
  xint_t *sum = (xint_t *)user_data;

  *sum += GPOINTER_TO_INT (data);
}

/* Check that expanding an array with g_array_set_size() clears the new elements
 * if @clear_ was specified during construction. */
static void
array_set_size (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xsize_t i;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  g_assert_cmpuint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_set_size (garray, 5);
  g_assert_cmpuint (garray->len, ==, 5);
  assert_int_array_zero_terminated (config, garray);

  if (config->clear_)
    for (i = 0; i < 5; i++)
      g_assert_cmpint (g_array_index (garray, xint_t, i), ==, 0);

  g_array_unref (garray);
}

/* As with array_set_size(), but with a sized array. */
static void
array_set_size_sized (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xsize_t i;

  garray = g_array_sized_new (config->zero_terminated, config->clear_, sizeof (xint_t), 10);
  g_assert_cmpuint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_set_size (garray, 5);
  g_assert_cmpuint (garray->len, ==, 5);
  assert_int_array_zero_terminated (config, garray);

  if (config->clear_)
    for (i = 0; i < 5; i++)
      g_assert_cmpint (g_array_index (garray, xint_t, i), ==, 0);

  g_array_unref (garray);
}

/* Check that a zero-terminated array does actually have a zero terminator. */
static void
array_new_zero_terminated (void)
{
  xarray_t *garray;
  xchar_t *out_str = NULL;

  garray = g_array_new (TRUE, FALSE, sizeof (xchar_t));
  g_assert_cmpuint (garray->len, ==, 0);

  g_array_append_vals (garray, "hello", strlen ("hello"));
  g_assert_cmpuint (garray->len, ==, 5);
  g_assert_cmpstr (garray->data, ==, "hello");

  out_str = g_array_free (garray, FALSE);
  g_assert_cmpstr (out_str, ==, "hello");
  g_free (out_str);
}

/* Check g_array_steal() function */
static void
array_steal (void)
{
  const xuint_t array_size = 10000;
  xarray_t *garray;
  xint_t *adata;
  xuint_t i;
  xsize_t len, past_len;

  garray = g_array_new (FALSE, FALSE, sizeof (xint_t));
  adata = (xint_t *) g_array_steal (garray, NULL);
  g_assert_null (adata);

  adata = (xint_t *) g_array_steal (garray, &len);
  g_assert_null (adata);
  g_assert_cmpint (len, ==, 0);

  for (i = 0; i < array_size; i++)
    g_array_append_val (garray, i);

  for (i = 0; i < array_size; i++)
    g_assert_cmpint (g_array_index (garray, xint_t, i), ==, i);


  past_len = garray->len;
  adata = (xint_t *) g_array_steal (garray, &len);
  for (i = 0; i < array_size; i++)
    g_assert_cmpint (adata[i], ==, i);

  g_assert_cmpint (past_len, ==, len);
  g_assert_cmpint (garray->len, ==, 0);

  g_array_append_val (garray, i);

  g_assert_cmpint (adata[0], ==, 0);
  g_assert_cmpint (g_array_index (garray, xint_t, 0), ==, array_size);
  g_assert_cmpint (garray->len, ==, 1);

  g_array_remove_index (garray, 0);

  for (i = 0; i < array_size; i++)
    g_array_append_val (garray, i);

  g_assert_cmpint (garray->len, ==, array_size);
  g_assert_cmpmem (adata, array_size * sizeof (xint_t),
                   garray->data, array_size * sizeof (xint_t));
  g_free (adata);
  g_array_free (garray, TRUE);
}

/* Check that g_array_append_val() works correctly for various #xarray_t
 * configurations. */
static void
array_append_val (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xint_t i;
  xint_t *segment;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  for (i = 0; i < 10000; i++)
    g_array_append_val (garray, i);
  assert_int_array_zero_terminated (config, garray);

  for (i = 0; i < 10000; i++)
    g_assert_cmpint (g_array_index (garray, xint_t, i), ==, i);

  segment = (xint_t*)g_array_free (garray, FALSE);
  for (i = 0; i < 10000; i++)
    g_assert_cmpint (segment[i], ==, i);
  if (config->zero_terminated)
    g_assert_cmpint (segment[10000], ==, 0);

  g_free (segment);
}

/* Check that g_array_prepend_val() works correctly for various #xarray_t
 * configurations. */
static void
array_prepend_val (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xint_t i;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  for (i = 0; i < 100; i++)
    g_array_prepend_val (garray, i);
  assert_int_array_zero_terminated (config, garray);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (g_array_index (garray, xint_t, i), ==, (100 - i - 1));

  g_array_free (garray, TRUE);
}

/* Test that g_array_prepend_vals() works correctly with various array
 * configurations. */
static void
array_prepend_vals (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray, *garray_out;
  const xint_t vals[] = { 0, 1, 2, 3, 4 };
  const xint_t expected_vals1[] = { 0, 1 };
  const xint_t expected_vals2[] = { 2, 0, 1 };
  const xint_t expected_vals3[] = { 3, 4, 2, 0, 1 };

  /* Set up an array. */
  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend several values to an empty array. */
  garray_out = g_array_prepend_vals (garray, vals, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals1, G_N_ELEMENTS (expected_vals1));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend a single value. */
  garray_out = g_array_prepend_vals (garray, vals + 2, 1);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals2, G_N_ELEMENTS (expected_vals2));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend several values to a non-empty array. */
  garray_out = g_array_prepend_vals (garray, vals + 3, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend no values. */
  garray_out = g_array_prepend_vals (garray, vals, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  /* Prepend no values with %NULL data. */
  garray_out = g_array_prepend_vals (garray, NULL, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  g_array_free (garray, TRUE);
}

/* Test that g_array_insert_vals() works correctly with various array
 * configurations. */
static void
array_insert_vals (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray, *garray_out;
  xsize_t i;
  const xint_t vals[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  const xint_t expected_vals1[] = { 0, 1 };
  const xint_t expected_vals2[] = { 0, 2, 3, 1 };
  const xint_t expected_vals3[] = { 0, 2, 3, 1, 4 };
  const xint_t expected_vals4[] = { 5, 0, 2, 3, 1, 4 };
  const xint_t expected_vals5[] = { 5, 0, 2, 3, 1, 4, 0, 0, 0, 0, 6, 7 };

  /* Set up an array. */
  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  assert_int_array_zero_terminated (config, garray);

  /* Insert several values at the beginning. */
  garray_out = g_array_insert_vals (garray, 0, vals, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals1, G_N_ELEMENTS (expected_vals1));
  assert_int_array_zero_terminated (config, garray);

  /* Insert some more part-way through. */
  garray_out = g_array_insert_vals (garray, 1, vals + 2, 2);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals2, G_N_ELEMENTS (expected_vals2));
  assert_int_array_zero_terminated (config, garray);

  /* And at the end. */
  garray_out = g_array_insert_vals (garray, garray->len, vals + 4, 1);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals3, G_N_ELEMENTS (expected_vals3));
  assert_int_array_zero_terminated (config, garray);

  /* Then back at the beginning again. */
  garray_out = g_array_insert_vals (garray, 0, vals + 5, 1);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals4, G_N_ELEMENTS (expected_vals4));
  assert_int_array_zero_terminated (config, garray);

  /* Insert zero elements. */
  garray_out = g_array_insert_vals (garray, 0, vals, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals4, G_N_ELEMENTS (expected_vals4));
  assert_int_array_zero_terminated (config, garray);

  /* Insert zero elements with a %NULL pointer. */
  garray_out = g_array_insert_vals (garray, 0, NULL, 0);
  g_assert_true (garray == garray_out);
  assert_int_array_equal (garray, expected_vals4, G_N_ELEMENTS (expected_vals4));
  assert_int_array_zero_terminated (config, garray);

  /* Insert some elements off the end of the array. The behaviour here depends
   * on whether the array clears entries. */
  garray_out = g_array_insert_vals (garray, garray->len + 4, vals + 6, 2);
  g_assert_true (garray == garray_out);

  g_assert_cmpuint (garray->len, ==, G_N_ELEMENTS (expected_vals5));
  for (i = 0; i < G_N_ELEMENTS (expected_vals5); i++)
    {
      if (config->clear_ || i < 6 || i > 9)
        g_assert_cmpint (g_array_index (garray, xint_t, i), ==, expected_vals5[i]);
    }

  assert_int_array_zero_terminated (config, garray);

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_index() works correctly for various #xarray_t
 * configurations. */
static void
array_remove_index (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xuint_t i;
  xint_t prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  for (i = 0; i < 100; i++)
    g_array_append_val (garray, i);
  assert_int_array_zero_terminated (config, garray);

  g_assert_cmpint (garray->len, ==, 100);

  g_array_remove_index (garray, 1);
  g_array_remove_index (garray, 3);
  g_array_remove_index (garray, 21);
  g_array_remove_index (garray, 57);

  g_assert_cmpint (garray->len, ==, 96);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, xint_t, i);
      g_assert (cur != 1 &&  cur != 4 && cur != 23 && cur != 60);
      g_assert_cmpint (prev, <, cur);
      prev = cur;
    }

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_index_fast() works correctly for various #xarray_t
 * configurations. */
static void
array_remove_index_fast (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xuint_t i;
  xint_t prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  for (i = 0; i < 100; i++)
    g_array_append_val (garray, i);

  g_assert_cmpint (garray->len, ==, 100);
  assert_int_array_zero_terminated (config, garray);

  g_array_remove_index_fast (garray, 1);
  g_array_remove_index_fast (garray, 3);
  g_array_remove_index_fast (garray, 21);
  g_array_remove_index_fast (garray, 57);

  g_assert_cmpint (garray->len, ==, 96);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, xint_t, i);
      g_assert (cur != 1 &&  cur != 3 && cur != 21 && cur != 57);
      if (cur < 96)
        {
          g_assert_cmpint (prev, <, cur);
          prev = cur;
        }
    }

  g_array_free (garray, TRUE);
}

/* Check that g_array_remove_range() works correctly for various #xarray_t
 * configurations. */
static void
array_remove_range (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xuint_t i;
  xint_t prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));
  for (i = 0; i < 100; i++)
    g_array_append_val (garray, i);

  g_assert_cmpint (garray->len, ==, 100);
  assert_int_array_zero_terminated (config, garray);

  g_array_remove_range (garray, 31, 4);

  g_assert_cmpint (garray->len, ==, 96);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, xint_t, i);
      g_assert (cur < 31 || cur > 34);
      g_assert_cmpint (prev, <, cur);
      prev = cur;
    }

  /* Ensure the entire array can be cleared, even when empty. */
  g_array_remove_range (garray, 0, garray->len);

  g_assert_cmpint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_remove_range (garray, 0, garray->len);

  g_assert_cmpint (garray->len, ==, 0);
  assert_int_array_zero_terminated (config, garray);

  g_array_free (garray, TRUE);
}

static void
array_ref_count (void)
{
  xarray_t *garray;
  xarray_t *garray2;
  xint_t i;

  garray = g_array_new (FALSE, FALSE, sizeof (xint_t));
  g_assert_cmpint (g_array_get_element_size (garray), ==, sizeof (xint_t));
  for (i = 0; i < 100; i++)
    g_array_prepend_val (garray, i);

  /* check we can ref, unref and still access the array */
  garray2 = g_array_ref (garray);
  g_assert (garray == garray2);
  g_array_unref (garray2);
  for (i = 0; i < 100; i++)
    g_assert_cmpint (g_array_index (garray, xint_t, i), ==, (100 - i - 1));

  /* garray2 should be an empty valid xarray_t wrapper */
  garray2 = g_array_ref (garray);
  g_array_free (garray, TRUE);

  g_assert_cmpint (garray2->len, ==, 0);
  g_array_unref (garray2);
}

static int
int_compare (xconstpointer p1, xconstpointer p2)
{
  const xint_t *i1 = p1;
  const xint_t *i2 = p2;

  return *i1 - *i2;
}

static void
array_copy (xconstpointer test_data)
{
  xarray_t *array, *array_copy;
  xsize_t i;
  const ArrayTestData *config = test_data;
  const xsize_t array_size = 100;

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      array = g_array_copy (NULL);
      g_test_assert_expected_messages ();

      g_assert_null (array);
    }

  /* Testing simple copy */
  array = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));

  for (i = 0; i < array_size; i++)
    g_array_append_val (array, i);

  array_copy = g_array_copy (array);

  /* Check internal data */
  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (g_array_index (array, xint_t, i), ==,
                      g_array_index (array_copy, xint_t, i));

  /* Check internal parameters ('zero_terminated' flag) */
  if (config->zero_terminated)
    {
      const xint_t *data = (const xint_t *) array_copy->data;
      g_assert_cmpint (data[array_copy->len], ==, 0);
    }

  /* Check internal parameters ('clear' flag) */
  if (config->clear_)
    {
      xuint_t old_length = array_copy->len;
      g_array_set_size (array_copy, old_length + 5);
      for (i = old_length; i < old_length + 5; i++)
        g_assert_cmpint (g_array_index (array_copy, xint_t, i), ==, 0);
    }

  /* Clean-up */
  g_array_unref (array);
  g_array_unref (array_copy);
}

static int
int_compare_data (xconstpointer p1, xconstpointer p2, xpointer_t data)
{
  const xint_t *i1 = p1;
  const xint_t *i2 = p2;

  return *i1 - *i2;
}

/* Check that g_array_sort() works correctly for various #xarray_t
 * configurations. */
static void
array_sort (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xuint_t i;
  xint_t prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));

  /* Sort empty array */
  g_array_sort (garray, int_compare);

  for (i = 0; i < 10000; i++)
    {
      cur = g_random_int_range (0, 10000);
      g_array_append_val (garray, cur);
    }
  assert_int_array_zero_terminated (config, garray);

  g_array_sort (garray, int_compare);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, xint_t, i);
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_array_free (garray, TRUE);
}

/* Check that g_array_sort_with_data() works correctly for various #xarray_t
 * configurations. */
static void
array_sort_with_data (xconstpointer test_data)
{
  const ArrayTestData *config = test_data;
  xarray_t *garray;
  xuint_t i;
  xint_t prev, cur;

  garray = g_array_new (config->zero_terminated, config->clear_, sizeof (xint_t));

  /* Sort empty array */
  g_array_sort_with_data (garray, int_compare_data, NULL);

  for (i = 0; i < 10000; i++)
    {
      cur = g_random_int_range (0, 10000);
      g_array_append_val (garray, cur);
    }
  assert_int_array_zero_terminated (config, garray);

  g_array_sort_with_data (garray, int_compare_data, NULL);
  assert_int_array_zero_terminated (config, garray);

  prev = -1;
  for (i = 0; i < garray->len; i++)
    {
      cur = g_array_index (garray, xint_t, i);
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  g_array_free (garray, TRUE);
}

static xint_t num_clear_func_invocations = 0;

static void
my_clear_func (xpointer_t data)
{
  num_clear_func_invocations += 1;
}

static void
array_clear_func (void)
{
  xarray_t *garray;
  xint_t i;
  xint_t cur;

  garray = g_array_new (FALSE, FALSE, sizeof (xint_t));
  g_array_set_clear_func (garray, my_clear_func);

  for (i = 0; i < 10; i++)
    {
      cur = g_random_int_range (0, 100);
      g_array_append_val (garray, cur);
    }

  g_array_remove_index (garray, 9);
  g_assert_cmpint (num_clear_func_invocations, ==, 1);

  g_array_remove_range (garray, 5, 3);
  g_assert_cmpint (num_clear_func_invocations, ==, 4);

  g_array_remove_index_fast (garray, 4);
  g_assert_cmpint (num_clear_func_invocations, ==, 5);

  g_array_free (garray, TRUE);
  g_assert_cmpint (num_clear_func_invocations, ==, 10);
}

/* Defining a comparison function for testing g_array_binary_search() */
static xint_t
cmpint (xconstpointer a, xconstpointer b)
{
  const xint_t *_a = a;
  const xint_t *_b = b;

  return *_a - *_b;
}

/* Testing g_array_binary_search() function */
static void
test_array_binary_search (void)
{
  xarray_t *garray;
  xuint_t i, matched_index;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 0);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_array_binary_search (NULL, &i, cmpint, NULL));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      g_assert_false (g_array_binary_search (garray, &i, NULL, NULL));
      g_test_assert_expected_messages ();
      g_array_free (garray, TRUE);
    }

  /* Testing array of size 0 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 0);

  i = 1;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 1 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 1);
  i = 1;
  g_array_append_val (garray, i);

  g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 2;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 2 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 2);
  for (i = 1; i < 3; i++)
    g_array_append_val (garray, i);

  for (i = 1; i < 3; i++)
    g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 4;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 3 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 3);
  for (i = 1; i < 4; i++)
    g_array_append_val (garray, i);

  for (i = 1; i < 4; i++)
    g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 5;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);

  /* Testing array of size 10000 */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 10000);

  for (i = 1; i < 10001; i++)
    g_array_append_val (garray, i);

  for (i = 1; i < 10001; i++)
    g_assert_true (g_array_binary_search (garray, &i, cmpint, NULL));

  for (i = 1; i < 10001; i++)
    {
      g_assert_true (g_array_binary_search (garray, &i, cmpint, &matched_index));
      g_assert_cmpint (i, ==, matched_index + 1);
    }

  /* Testing negative result */
  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));
  g_assert_false (g_array_binary_search (garray, &i, cmpint, &matched_index));

  i = 10002;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));
  g_assert_false (g_array_binary_search (garray, &i, cmpint, &matched_index));

  g_array_free (garray, TRUE);

  /* Test for a not-found element in the middle of the array. */
  garray = g_array_sized_new (FALSE, FALSE, sizeof (xuint_t), 3);
  for (i = 1; i < 10; i += 2)
    g_array_append_val (garray, i);

  i = 0;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 2;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  i = 10;
  g_assert_false (g_array_binary_search (garray, &i, cmpint, NULL));

  g_array_free (garray, TRUE);
}

static void
test_array_copy_sized (void)
{
  xarray_t *array1 = NULL, *array2 = NULL, *array3 = NULL;
  int val = 5;

  g_test_summary ("Test that copying a newly-allocated sized array works.");

  array1 = g_array_sized_new (FALSE, FALSE, sizeof (int), 1);
  array2 = g_array_copy (array1);

  g_assert_cmpuint (array2->len, ==, array1->len);

  g_array_append_val (array1, val);
  array3 = g_array_copy (array1);

  g_assert_cmpuint (array3->len, ==, array1->len);
  g_assert_cmpuint (g_array_index (array3, int, 0), ==, g_array_index (array1, int, 0));
  g_assert_cmpuint (array3->len, ==, 1);
  g_assert_cmpuint (g_array_index (array3, int, 0), ==, val);

  g_array_unref (array3);
  g_array_unref (array2);
  g_array_unref (array1);
}

static void
array_overflow_append_vals (void)
{
  if (!g_test_undefined ())
      return;

  if (g_test_subprocess ())
    {
      xarray_t *array = g_array_new (TRUE, FALSE, 1);
      /* Check for overflow should happen before data is accessed. */
      g_array_append_vals (array, NULL, G_MAXUINT);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*adding 4294967295 to array would overflow*");
    }
}

static void
array_overflow_set_size (void)
{
  if (!g_test_undefined ())
      return;

  if (g_test_subprocess ())
    {
      xarray_t *array = g_array_new (TRUE, FALSE, 1);
      g_array_set_size (array, G_MAXUINT);
    }
  else
    {
      g_test_trap_subprocess (NULL, 0, 0);
      g_test_trap_assert_failed ();
      g_test_trap_assert_stderr ("*adding 4294967295 to array would overflow*");
    }
}

/* Check xptr_array_steal() function */
static void
pointer_array_steal (void)
{
  const xuint_t array_size = 10000;
  xptr_array_t *gparray;
  xpointer_t *pdata;
  xuint_t i;
  xsize_t len, past_len;

  gparray = xptr_array_new ();
  pdata = xptr_array_steal (gparray, NULL);
  g_assert_null (pdata);

  pdata = xptr_array_steal (gparray, &len);
  g_assert_null (pdata);
  g_assert_cmpint (len, ==, 0);

  for (i = 0; i < array_size; i++)
    xptr_array_add (gparray, GINT_TO_POINTER (i));

  past_len = gparray->len;
  pdata = xptr_array_steal (gparray, &len);
  g_assert_cmpint (gparray->len, ==, 0);
  g_assert_cmpint (past_len, ==, len);
  xptr_array_add (gparray, GINT_TO_POINTER (10));

  g_assert_cmpint ((xsize_t) pdata[0], ==, (xsize_t) GINT_TO_POINTER (0));
  g_assert_cmpint ((xsize_t) xptr_array_index (gparray, 0), ==,
                   (xsize_t) GINT_TO_POINTER (10));
  g_assert_cmpint (gparray->len, ==, 1);

  xptr_array_remove_index (gparray, 0);

  for (i = 0; i < array_size; i++)
    xptr_array_add (gparray, GINT_TO_POINTER (i));
  g_assert_cmpmem (pdata, array_size * sizeof (xpointer_t),
                   gparray->pdata, array_size * sizeof (xpointer_t));
  g_free (pdata);

  xptr_array_free (gparray, TRUE);
}

static void
pointer_array_add (void)
{
  xptr_array_t *gparray;
  xint_t i;
  xint_t sum = 0;
  xpointer_t *segment;

  gparray = xptr_array_sized_new (1000);

  for (i = 0; i < 10000; i++)
    xptr_array_add (gparray, GINT_TO_POINTER (i));

  for (i = 0; i < 10000; i++)
    g_assert (xptr_array_index (gparray, i) == GINT_TO_POINTER (i));

  xptr_array_foreach (gparray, sum_up, &sum);
  g_assert (sum == 49995000);

  segment = xptr_array_free (gparray, FALSE);
  for (i = 0; i < 10000; i++)
    g_assert (segment[i] == GINT_TO_POINTER (i));
  g_free (segment);
}

static void
pointer_array_insert (void)
{
  xptr_array_t *gparray;
  xint_t i;
  xint_t sum = 0;
  xint_t index;

  gparray = xptr_array_sized_new (1000);

  for (i = 0; i < 10000; i++)
    {
      index = g_random_int_range (-1, i + 1);
      xptr_array_insert (gparray, index, GINT_TO_POINTER (i));
    }

  xptr_array_foreach (gparray, sum_up, &sum);
  g_assert (sum == 49995000);

  xptr_array_free (gparray, TRUE);
}

static void
pointer_array_ref_count (void)
{
  xptr_array_t *gparray;
  xptr_array_t *gparray2;
  xint_t i;
  xint_t sum = 0;

  gparray = xptr_array_new ();
  for (i = 0; i < 10000; i++)
    xptr_array_add (gparray, GINT_TO_POINTER (i));

  /* check we can ref, unref and still access the array */
  gparray2 = xptr_array_ref (gparray);
  g_assert (gparray == gparray2);
  xptr_array_unref (gparray2);
  for (i = 0; i < 10000; i++)
    g_assert (xptr_array_index (gparray, i) == GINT_TO_POINTER (i));

  xptr_array_foreach (gparray, sum_up, &sum);
  g_assert (sum == 49995000);

  /* gparray2 should be an empty valid xptr_array_t wrapper */
  gparray2 = xptr_array_ref (gparray);
  xptr_array_free (gparray, TRUE);

  g_assert_cmpint (gparray2->len, ==, 0);
  xptr_array_unref (gparray2);
}

static xint_t num_free_func_invocations = 0;

static void
my_free_func (xpointer_t data)
{
  num_free_func_invocations++;
  g_free (data);
}

static void
pointer_array_free_func (void)
{
  xptr_array_t *gparray;
  xptr_array_t *gparray2;
  xchar_t **strv;
  xchar_t *s;

  num_free_func_invocations = 0;
  gparray = xptr_array_new_with_free_func (my_free_func);
  xptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 0);

  gparray = xptr_array_new_with_free_func (my_free_func);
  xptr_array_free (gparray, TRUE);
  g_assert_cmpint (num_free_func_invocations, ==, 0);

  num_free_func_invocations = 0;
  gparray = xptr_array_new_with_free_func (my_free_func);
  xptr_array_add (gparray, xstrdup ("foo"));
  xptr_array_add (gparray, xstrdup ("bar"));
  xptr_array_add (gparray, xstrdup ("baz"));
  xptr_array_remove_index (gparray, 0);
  g_assert_cmpint (num_free_func_invocations, ==, 1);
  xptr_array_remove_index_fast (gparray, 1);
  g_assert_cmpint (num_free_func_invocations, ==, 2);
  s = xstrdup ("frob");
  xptr_array_add (gparray, s);
  g_assert (xptr_array_remove (gparray, s));
  g_assert (!xptr_array_remove (gparray, "nuun"));
  g_assert (!xptr_array_remove_fast (gparray, "mlo"));
  g_assert_cmpint (num_free_func_invocations, ==, 3);
  s = xstrdup ("frob");
  xptr_array_add (gparray, s);
  xptr_array_set_size (gparray, 1);
  g_assert_cmpint (num_free_func_invocations, ==, 4);
  xptr_array_ref (gparray);
  xptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 4);
  xptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 5);

  num_free_func_invocations = 0;
  gparray = xptr_array_new_full (10, my_free_func);
  xptr_array_add (gparray, xstrdup ("foo"));
  xptr_array_add (gparray, xstrdup ("bar"));
  xptr_array_add (gparray, xstrdup ("baz"));
  xptr_array_set_size (gparray, 20);
  xptr_array_add (gparray, NULL);
  gparray2 = xptr_array_ref (gparray);
  strv = (xchar_t **) xptr_array_free (gparray, FALSE);
  g_assert_cmpint (num_free_func_invocations, ==, 0);
  xstrfreev (strv);
  xptr_array_unref (gparray2);
  g_assert_cmpint (num_free_func_invocations, ==, 0);

  num_free_func_invocations = 0;
  gparray = xptr_array_new_with_free_func (my_free_func);
  xptr_array_add (gparray, xstrdup ("foo"));
  xptr_array_add (gparray, xstrdup ("bar"));
  xptr_array_add (gparray, xstrdup ("baz"));
  xptr_array_remove_range (gparray, 1, 1);
  xptr_array_unref (gparray);
  g_assert_cmpint (num_free_func_invocations, ==, 3);

  num_free_func_invocations = 0;
  gparray = xptr_array_new_with_free_func (my_free_func);
  xptr_array_add (gparray, xstrdup ("foo"));
  xptr_array_add (gparray, xstrdup ("bar"));
  xptr_array_add (gparray, xstrdup ("baz"));
  xptr_array_free (gparray, TRUE);
  g_assert_cmpint (num_free_func_invocations, ==, 3);

  num_free_func_invocations = 0;
  gparray = xptr_array_new_with_free_func (my_free_func);
  xptr_array_add (gparray, "foo");
  xptr_array_add (gparray, "bar");
  xptr_array_add (gparray, "baz");
  xptr_array_set_free_func (gparray, NULL);
  xptr_array_free (gparray, TRUE);
  g_assert_cmpint (num_free_func_invocations, ==, 0);
}

static xpointer_t
ptr_array_copy_func (xconstpointer src, xpointer_t userdata)
{
  xsize_t *dst = g_malloc (sizeof (xsize_t));
  *dst = *((xsize_t *) src);
  return dst;
}

/* Test the xptr_array_copy() function */
static void
pointer_array_copy (void)
{
  xptr_array_t *ptr_array, *ptr_array2;
  xsize_t i;
  const xsize_t array_size = 100;
  xsize_t *array_test = g_malloc (array_size * sizeof (xsize_t));

  g_test_summary ("Check all normal behaviour of stealing elements from one "
                  "array to append to another, covering different array sizes "
                  "and element copy functions");

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      ptr_array = xptr_array_copy (NULL, NULL, NULL);
      g_test_assert_expected_messages ();
      g_assert_cmpuint ((xsize_t) ptr_array, ==, (xsize_t) NULL);
    }

  /* Initializing array_test */
  for (i = 0; i < array_size; i++)
    array_test[i] = i;

  /* Test copy an empty array */
  ptr_array = xptr_array_sized_new (0);
  ptr_array2 = xptr_array_copy (ptr_array, NULL, NULL);

  g_assert_cmpuint (ptr_array2->len, ==, ptr_array->len);

  xptr_array_unref (ptr_array);
  xptr_array_unref (ptr_array2);

  /* Test simple copy */
  ptr_array = xptr_array_sized_new (array_size);

  for (i = 0; i < array_size; i++)
    xptr_array_add (ptr_array, &array_test[i]);

  ptr_array2 = xptr_array_copy (ptr_array, NULL, NULL);

  g_assert_cmpuint (ptr_array2->len, ==, ptr_array->len);
  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array2, i)), ==, i);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint ((xsize_t) xptr_array_index (ptr_array, i), ==,
                      (xsize_t) xptr_array_index (ptr_array2, i));

  xptr_array_free (ptr_array2, TRUE);

  /* Test copy through GCopyFunc */
  ptr_array2 = xptr_array_copy (ptr_array, ptr_array_copy_func, NULL);
  xptr_array_set_free_func (ptr_array2, g_free);

  g_assert_cmpuint (ptr_array2->len, ==, ptr_array->len);
  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array2, i)), ==, i);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint ((xsize_t) xptr_array_index (ptr_array, i), !=,
                      (xsize_t) xptr_array_index (ptr_array2, i));

  xptr_array_free (ptr_array2, TRUE);

  /* Final cleanup */
  xptr_array_free (ptr_array, TRUE);
  g_free (array_test);
}

/* Test the xptr_array_extend() function */
static void
pointer_array_extend (void)
{
  xptr_array_t *ptr_array, *ptr_array2;
  xsize_t i;
  const xsize_t array_size = 100;
  xsize_t *array_test = g_malloc (array_size * sizeof (xsize_t));

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      ptr_array = xptr_array_sized_new (0);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      xptr_array_extend (NULL, ptr_array, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      xptr_array_extend (ptr_array, NULL, NULL, NULL);
      g_test_assert_expected_messages ();

      xptr_array_unref (ptr_array);
    }

  /* Initializing array_test */
  for (i = 0; i < array_size; i++)
    array_test[i] = i;

  /* Testing extend with array of size zero */
  ptr_array = xptr_array_sized_new (0);
  ptr_array2 = xptr_array_sized_new (0);

  xptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  g_assert_cmpuint (ptr_array->len, ==, 0);
  g_assert_cmpuint (ptr_array2->len, ==, 0);

  xptr_array_unref (ptr_array);
  xptr_array_unref (ptr_array2);

  /* Testing extend an array of size zero */
  ptr_array = xptr_array_sized_new (array_size);
  ptr_array2 = xptr_array_sized_new (0);

  for (i = 0; i < array_size; i++)
    {
      xptr_array_add (ptr_array, &array_test[i]);
    }

  xptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array, i)), ==, i);

  xptr_array_unref (ptr_array);
  xptr_array_unref (ptr_array2);

  /* Testing extend an array of size zero */
  ptr_array = xptr_array_sized_new (0);
  ptr_array2 = xptr_array_sized_new (array_size);

  for (i = 0; i < array_size; i++)
    {
      xptr_array_add (ptr_array2, &array_test[i]);
    }

  xptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array, i)), ==, i);

  xptr_array_unref (ptr_array);
  xptr_array_unref (ptr_array2);

  /* Testing simple extend */
  ptr_array = xptr_array_sized_new (array_size / 2);
  ptr_array2 = xptr_array_sized_new (array_size / 2);

  for (i = 0; i < array_size / 2; i++)
    {
      xptr_array_add (ptr_array, &array_test[i]);
      xptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  xptr_array_extend (ptr_array, ptr_array2, NULL, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array, i)), ==, i);

  xptr_array_unref (ptr_array);
  xptr_array_unref (ptr_array2);

  /* Testing extend with GCopyFunc */
  ptr_array = xptr_array_sized_new (array_size / 2);
  ptr_array2 = xptr_array_sized_new (array_size / 2);

  for (i = 0; i < array_size / 2; i++)
    {
      xptr_array_add (ptr_array, &array_test[i]);
      xptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  xptr_array_extend (ptr_array, ptr_array2, ptr_array_copy_func, NULL);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array, i)), ==, i);

  /* Clean-up memory */
  for (i = array_size / 2; i < array_size; i++)
    g_free (xptr_array_index (ptr_array, i));

  xptr_array_unref (ptr_array);
  xptr_array_unref (ptr_array2);
  g_free (array_test);
}

/* Test the xptr_array_extend_and_steal() function */
static void
pointer_array_extend_and_steal (void)
{
  xptr_array_t *ptr_array, *ptr_array2, *ptr_array3;
  xsize_t i;
  const xsize_t array_size = 100;
  xsize_t *array_test = g_malloc (array_size * sizeof (xsize_t));

  /* Initializing array_test */
  for (i = 0; i < array_size; i++)
    array_test[i] = i;

  /* Testing simple extend_and_steal() */
  ptr_array = xptr_array_sized_new (array_size / 2);
  ptr_array2 = xptr_array_sized_new (array_size / 2);

  for (i = 0; i < array_size / 2; i++)
    {
      xptr_array_add (ptr_array, &array_test[i]);
      xptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  xptr_array_extend_and_steal (ptr_array, ptr_array2);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array, i)), ==, i);

  xptr_array_free (ptr_array, TRUE);

  /* Testing extend_and_steal() with a pending reference to stolen array */
  ptr_array = xptr_array_sized_new (array_size / 2);
  ptr_array2 = xptr_array_sized_new (array_size / 2);

  for (i = 0; i < array_size / 2; i++)
    {
      xptr_array_add (ptr_array, &array_test[i]);
      xptr_array_add (ptr_array2, &array_test[i + (array_size / 2)]);
    }

  ptr_array3 = xptr_array_ref (ptr_array2);

  xptr_array_extend_and_steal (ptr_array, ptr_array2);

  for (i = 0; i < array_size; i++)
    g_assert_cmpuint (*((xsize_t *) xptr_array_index (ptr_array, i)), ==, i);

  g_assert_cmpuint (ptr_array3->len, ==, 0);
  g_assert_null (ptr_array3->pdata);

  xptr_array_add (ptr_array2, NULL);

  xptr_array_free (ptr_array, TRUE);
  xptr_array_free (ptr_array3, TRUE);

  /* Final memory clean-up */
  g_free (array_test);
}

static xint_t
ptr_compare (xconstpointer p1, xconstpointer p2)
{
  xpointer_t i1 = *(xpointer_t*)p1;
  xpointer_t i2 = *(xpointer_t*)p2;

  return GPOINTER_TO_INT (i1) - GPOINTER_TO_INT (i2);
}

static xint_t
ptr_compare_data (xconstpointer p1, xconstpointer p2, xpointer_t data)
{
  xpointer_t i1 = *(xpointer_t*)p1;
  xpointer_t i2 = *(xpointer_t*)p2;

  return GPOINTER_TO_INT (i1) - GPOINTER_TO_INT (i2);
}

static void
pointer_array_sort (void)
{
  xptr_array_t *gparray;
  xint_t i;
  xint_t val;
  xint_t prev, cur;

  gparray = xptr_array_new ();

  /* Sort empty array */
  xptr_array_sort (gparray, ptr_compare);

  for (i = 0; i < 10000; i++)
    {
      val = g_random_int_range (0, 10000);
      xptr_array_add (gparray, GINT_TO_POINTER (val));
    }

  xptr_array_sort (gparray, ptr_compare);

  prev = -1;
  for (i = 0; i < 10000; i++)
    {
      cur = GPOINTER_TO_INT (xptr_array_index (gparray, i));
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  xptr_array_free (gparray, TRUE);
}

/* Please keep pointer_array_sort_example() in sync with the doc-comment
 * of xptr_array_sort() */

typedef struct
{
  xchar_t *name;
  xint_t size;
} FileListEntry;

static void
file_list_entry_free (xpointer_t p)
{
  FileListEntry *entry = p;

  g_free (entry->name);
  g_free (entry);
}

static xint_t
sort_filelist (xconstpointer a, xconstpointer b)
{
   const FileListEntry *entry1 = *((FileListEntry **) a);
   const FileListEntry *entry2 = *((FileListEntry **) b);

   return g_ascii_strcasecmp (entry1->name, entry2->name);
}

static void
pointer_array_sort_example (void)
{
  xptr_array_t *file_list = NULL;
  FileListEntry *entry;

  g_test_summary ("Check that the doc-comment for xptr_array_sort() is correct");

  file_list = xptr_array_new_with_free_func (file_list_entry_free);

  entry = g_new0 (FileListEntry, 1);
  entry->name = xstrdup ("README");
  entry->size = 42;
  xptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = xstrdup ("empty");
  entry->size = 0;
  xptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = xstrdup ("aardvark");
  entry->size = 23;
  xptr_array_add (file_list, g_steal_pointer (&entry));

  xptr_array_sort (file_list, sort_filelist);

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = xptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = xptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = xptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  xptr_array_unref (file_list);
}

/* Please keep pointer_array_sort_with_data_example() in sync with the
 * doc-comment of xptr_array_sort_with_data() */

typedef enum { SORT_NAME, SORT_SIZE } SortMode;

static xint_t
sort_filelist_how (xconstpointer a, xconstpointer b, xpointer_t user_data)
{
  xint_t order;
  const SortMode sort_mode = GPOINTER_TO_INT (user_data);
  const FileListEntry *entry1 = *((FileListEntry **) a);
  const FileListEntry *entry2 = *((FileListEntry **) b);

  switch (sort_mode)
    {
    case SORT_NAME:
      order = g_ascii_strcasecmp (entry1->name, entry2->name);
      break;
    case SORT_SIZE:
      order = entry1->size - entry2->size;
      break;
    default:
      order = 0;
      break;
    }
  return order;
}

static void
pointer_array_sort_with_data_example (void)
{
  xptr_array_t *file_list = NULL;
  FileListEntry *entry;
  SortMode sort_mode;

  g_test_summary ("Check that the doc-comment for xptr_array_sort_with_data() is correct");

  file_list = xptr_array_new_with_free_func (file_list_entry_free);

  entry = g_new0 (FileListEntry, 1);
  entry->name = xstrdup ("README");
  entry->size = 42;
  xptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = xstrdup ("empty");
  entry->size = 0;
  xptr_array_add (file_list, g_steal_pointer (&entry));

  entry = g_new0 (FileListEntry, 1);
  entry->name = xstrdup ("aardvark");
  entry->size = 23;
  xptr_array_add (file_list, g_steal_pointer (&entry));

  sort_mode = SORT_NAME;
  xptr_array_sort_with_data (file_list, sort_filelist_how, GINT_TO_POINTER (sort_mode));

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = xptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = xptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = xptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  sort_mode = SORT_SIZE;
  xptr_array_sort_with_data (file_list, sort_filelist_how, GINT_TO_POINTER (sort_mode));

  g_assert_cmpuint (file_list->len, ==, 3);
  entry = xptr_array_index (file_list, 0);
  g_assert_cmpstr (entry->name, ==, "empty");
  entry = xptr_array_index (file_list, 1);
  g_assert_cmpstr (entry->name, ==, "aardvark");
  entry = xptr_array_index (file_list, 2);
  g_assert_cmpstr (entry->name, ==, "README");

  xptr_array_unref (file_list);
}

static void
pointer_array_sort_with_data (void)
{
  xptr_array_t *gparray;
  xint_t i;
  xint_t prev, cur;

  gparray = xptr_array_new ();

  /* Sort empty array */
  xptr_array_sort_with_data (gparray, ptr_compare_data, NULL);

  for (i = 0; i < 10000; i++)
    xptr_array_add (gparray, GINT_TO_POINTER (g_random_int_range (0, 10000)));

  xptr_array_sort_with_data (gparray, ptr_compare_data, NULL);

  prev = -1;
  for (i = 0; i < 10000; i++)
    {
      cur = GPOINTER_TO_INT (xptr_array_index (gparray, i));
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  xptr_array_free (gparray, TRUE);
}

static void
pointer_array_find_empty (void)
{
  xptr_array_t *array;
  xuint_t idx;

  array = xptr_array_new ();

  g_assert_false (xptr_array_find (array, "some-value", NULL));  /* NULL index */
  g_assert_false (xptr_array_find (array, "some-value", &idx));  /* non-NULL index */
  g_assert_false (xptr_array_find_with_equal_func (array, "some-value", xstr_equal, NULL));  /* NULL index */
  g_assert_false (xptr_array_find_with_equal_func (array, "some-value", xstr_equal, &idx));  /* non-NULL index */

  xptr_array_free (array, TRUE);
}

static void
pointer_array_find_non_empty (void)
{
  xptr_array_t *array;
  xuint_t idx;
  const xchar_t *str_pointer = "static-string";

  array = xptr_array_new ();

  xptr_array_add (array, "some");
  xptr_array_add (array, "random");
  xptr_array_add (array, "values");
  xptr_array_add (array, "some");
  xptr_array_add (array, "duplicated");
  xptr_array_add (array, (xpointer_t) str_pointer);

  g_assert_true (xptr_array_find_with_equal_func (array, "random", xstr_equal, NULL));  /* NULL index */
  g_assert_true (xptr_array_find_with_equal_func (array, "random", xstr_equal, &idx));  /* non-NULL index */
  g_assert_cmpuint (idx, ==, 1);

  g_assert_true (xptr_array_find_with_equal_func (array, "some", xstr_equal, &idx));  /* duplicate element */
  g_assert_cmpuint (idx, ==, 0);

  g_assert_false (xptr_array_find_with_equal_func (array, "nope", xstr_equal, NULL));

  g_assert_true (xptr_array_find_with_equal_func (array, str_pointer, xstr_equal, &idx));
  g_assert_cmpuint (idx, ==, 5);
  idx = G_MAXUINT;
  g_assert_true (xptr_array_find_with_equal_func (array, str_pointer, NULL, &idx));  /* NULL equal func */
  g_assert_cmpuint (idx, ==, 5);
  idx = G_MAXUINT;
  g_assert_true (xptr_array_find (array, str_pointer, &idx));  /* NULL equal func */
  g_assert_cmpuint (idx, ==, 5);

  xptr_array_free (array, TRUE);
}

static void
steal_destroy_notify (xpointer_t data)
{
  xuint_t *counter = data;
  *counter = *counter + 1;
}

/* Test that xptr_array_steal_index() and xptr_array_steal_index_fast() can
 * remove elements from a pointer array without the #xdestroy_notify_t being called. */
static void
pointer_array_steal_index (void)
{
  xuint_t i1 = 0, i2 = 0, i3 = 0, i4 = 0;
  xpointer_t out1, out2;
  xptr_array_t *array = xptr_array_new_with_free_func (steal_destroy_notify);

  xptr_array_add (array, &i1);
  xptr_array_add (array, &i2);
  xptr_array_add (array, &i3);
  xptr_array_add (array, &i4);

  g_assert_cmpuint (array->len, ==, 4);

  /* Remove a single element. */
  out1 = xptr_array_steal_index (array, 0);
  g_assert_true (out1 == &i1);
  g_assert_cmpuint (i1, ==, 0);  /* should not have been destroyed */

  /* Following elements should have been moved down. */
  g_assert_cmpuint (array->len, ==, 3);
  g_assert_true (xptr_array_index (array, 0) == &i2);
  g_assert_true (xptr_array_index (array, 1) == &i3);
  g_assert_true (xptr_array_index (array, 2) == &i4);

  /* Remove another element, quickly. */
  out2 = xptr_array_steal_index_fast (array, 0);
  g_assert_true (out2 == &i2);
  g_assert_cmpuint (i2, ==, 0);  /* should not have been destroyed */

  /* Last element should have been swapped in place. */
  g_assert_cmpuint (array->len, ==, 2);
  g_assert_true (xptr_array_index (array, 0) == &i4);
  g_assert_true (xptr_array_index (array, 1) == &i3);

  /* Check that destroying the pointer array doesn’t affect the stolen elements. */
  xptr_array_unref (array);

  g_assert_cmpuint (i1, ==, 0);
  g_assert_cmpuint (i2, ==, 0);
  g_assert_cmpuint (i3, ==, 1);
  g_assert_cmpuint (i4, ==, 1);
}

static void
byte_array_new_take_overflow (void)
{
#if SIZE_WIDTH <= UINT_WIDTH
  g_test_skip ("Overflow test requires G_MAXSIZE > G_MAXUINT.");
#else
  xbyte_array_t* arr;

  if (!g_test_undefined ())
      return;

  /* Check for overflow should happen before data is accessed. */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                          "*assertion 'len <= G_MAXUINT' failed");
  arr = xbyte_array_new_take (NULL, (xsize_t)G_MAXUINT + 1);
  g_assert_null (arr);
  g_test_assert_expected_messages ();
#endif
}

static void
byte_array_steal (void)
{
  const xuint_t array_size = 10000;
  xbyte_array_t *gbarray;
  xuint8_t *bdata;
  xuint_t i;
  xsize_t len, past_len;

  gbarray = xbyte_array_new ();
  bdata = xbyte_array_steal (gbarray, NULL);
  g_assert_cmpint ((xsize_t) bdata, ==, (xsize_t) gbarray->data);
  g_free (bdata);

  for (i = 0; i < array_size; i++)
    xbyte_array_append (gbarray, (xuint8_t *) "abcd", 4);

  past_len = gbarray->len;
  bdata = xbyte_array_steal (gbarray, &len);

  g_assert_cmpint (len, ==, past_len);
  g_assert_cmpint (gbarray->len, ==, 0);

  xbyte_array_append (gbarray, (xuint8_t *) "@", 1);

  g_assert_cmpint (bdata[0], ==, 'a');
  g_assert_cmpint (gbarray->data[0], ==, '@');
  g_assert_cmpint (gbarray->len, ==, 1);

  xbyte_array_remove_index (gbarray, 0);

  g_free (bdata);
  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_append (void)
{
  xbyte_array_t *gbarray;
  xint_t i;
  xuint8_t *segment;

  gbarray = xbyte_array_sized_new (1000);
  for (i = 0; i < 10000; i++)
    xbyte_array_append (gbarray, (xuint8_t*) "abcd", 4);

  for (i = 0; i < 10000; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  segment = xbyte_array_free (gbarray, FALSE);

  for (i = 0; i < 10000; i++)
    {
      g_assert (segment[4*i] == 'a');
      g_assert (segment[4*i+1] == 'b');
      g_assert (segment[4*i+2] == 'c');
      g_assert (segment[4*i+3] == 'd');
    }

  g_free (segment);
}

static void
byte_array_prepend (void)
{
  xbyte_array_t *gbarray;
  xint_t i;

  gbarray = xbyte_array_new ();
  xbyte_array_set_size (gbarray, 1000);

  for (i = 0; i < 10000; i++)
    xbyte_array_prepend (gbarray, (xuint8_t*) "abcd", 4);

  for (i = 0; i < 10000; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_ref_count (void)
{
  xbyte_array_t *gbarray;
  xbyte_array_t *gbarray2;
  xint_t i;

  gbarray = xbyte_array_new ();
  for (i = 0; i < 10000; i++)
    xbyte_array_append (gbarray, (xuint8_t*) "abcd", 4);

  gbarray2 = xbyte_array_ref (gbarray);
  g_assert (gbarray2 == gbarray);
  xbyte_array_unref (gbarray2);
  for (i = 0; i < 10000; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  gbarray2 = xbyte_array_ref (gbarray);
  g_assert (gbarray2 == gbarray);
  xbyte_array_free (gbarray, TRUE);
  g_assert_cmpint (gbarray2->len, ==, 0);
  xbyte_array_unref (gbarray2);
}

static void
byte_array_remove (void)
{
  xbyte_array_t *gbarray;
  xint_t i;

  gbarray = xbyte_array_new ();
  for (i = 0; i < 100; i++)
    xbyte_array_append (gbarray, (xuint8_t*) "abcd", 4);

  g_assert_cmpint (gbarray->len, ==, 400);

  xbyte_array_remove_index (gbarray, 4);
  xbyte_array_remove_index (gbarray, 4);
  xbyte_array_remove_index (gbarray, 4);
  xbyte_array_remove_index (gbarray, 4);

  g_assert_cmpint (gbarray->len, ==, 396);

  for (i = 0; i < 99; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_remove_fast (void)
{
  xbyte_array_t *gbarray;
  xint_t i;

  gbarray = xbyte_array_new ();
  for (i = 0; i < 100; i++)
    xbyte_array_append (gbarray, (xuint8_t*) "abcd", 4);

  g_assert_cmpint (gbarray->len, ==, 400);

  xbyte_array_remove_index_fast (gbarray, 4);
  xbyte_array_remove_index_fast (gbarray, 4);
  xbyte_array_remove_index_fast (gbarray, 4);
  xbyte_array_remove_index_fast (gbarray, 4);

  g_assert_cmpint (gbarray->len, ==, 396);

  for (i = 0; i < 99; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_remove_range (void)
{
  xbyte_array_t *gbarray;
  xint_t i;

  gbarray = xbyte_array_new ();
  for (i = 0; i < 100; i++)
    xbyte_array_append (gbarray, (xuint8_t*) "abcd", 4);

  g_assert_cmpint (gbarray->len, ==, 400);

  xbyte_array_remove_range (gbarray, 12, 4);

  g_assert_cmpint (gbarray->len, ==, 396);

  for (i = 0; i < 99; i++)
    {
      g_assert (gbarray->data[4*i] == 'a');
      g_assert (gbarray->data[4*i+1] == 'b');
      g_assert (gbarray->data[4*i+2] == 'c');
      g_assert (gbarray->data[4*i+3] == 'd');
    }

  /* Ensure the entire array can be cleared, even when empty. */
  xbyte_array_remove_range (gbarray, 0, gbarray->len);
  xbyte_array_remove_range (gbarray, 0, gbarray->len);

  xbyte_array_free (gbarray, TRUE);
}

static int
byte_compare (xconstpointer p1, xconstpointer p2)
{
  const xuint8_t *i1 = p1;
  const xuint8_t *i2 = p2;

  return *i1 - *i2;
}

static int
byte_compare_data (xconstpointer p1, xconstpointer p2, xpointer_t data)
{
  const xuint8_t *i1 = p1;
  const xuint8_t *i2 = p2;

  return *i1 - *i2;
}

static void
byte_array_sort (void)
{
  xbyte_array_t *gbarray;
  xuint_t i;
  xuint8_t val;
  xuint8_t prev, cur;

  gbarray = xbyte_array_new ();
  for (i = 0; i < 100; i++)
    {
      val = 'a' + g_random_int_range (0, 26);
      xbyte_array_append (gbarray, (xuint8_t*) &val, 1);
    }

  xbyte_array_sort (gbarray, byte_compare);

  prev = 'a';
  for (i = 0; i < gbarray->len; i++)
    {
      cur = gbarray->data[i];
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_sort_with_data (void)
{
  xbyte_array_t *gbarray;
  xuint_t i;
  xuint8_t val;
  xuint8_t prev, cur;

  gbarray = xbyte_array_new ();
  for (i = 0; i < 100; i++)
    {
      val = 'a' + g_random_int_range (0, 26);
      xbyte_array_append (gbarray, (xuint8_t*) &val, 1);
    }

  xbyte_array_sort_with_data (gbarray, byte_compare_data, NULL);

  prev = 'a';
  for (i = 0; i < gbarray->len; i++)
    {
      cur = gbarray->data[i];
      g_assert_cmpint (prev, <=, cur);
      prev = cur;
    }

  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_new_take (void)
{
  xbyte_array_t *gbarray;
  xuint8_t *data;

  data = g_memdup2 ("woooweeewow", 11);
  gbarray = xbyte_array_new_take (data, 11);
  g_assert (gbarray->data == data);
  g_assert_cmpuint (gbarray->len, ==, 11);
  xbyte_array_free (gbarray, TRUE);
}

static void
byte_array_free_to_bytes (void)
{
  xbyte_array_t *gbarray;
  xpointer_t memory;
  xbytes_t *bytes;
  xsize_t size;

  gbarray = xbyte_array_new ();
  xbyte_array_append (gbarray, (xuint8_t *)"woooweeewow", 11);
  memory = gbarray->data;

  bytes = xbyte_array_free_to_bytes (gbarray);
  g_assert (bytes != NULL);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, 11);
  g_assert (xbytes_get_data (bytes, &size) == memory);
  g_assert_cmpuint (size, ==, 11);

  xbytes_unref (bytes);
}

static void
add_array_test (const xchar_t         *test_path,
                const ArrayTestData *config,
                GTestDataFunc        test_func)
{
  xchar_t *test_name = NULL;

  test_name = xstrdup_printf ("%s/%s-%s",
                               test_path,
                               config->zero_terminated ? "zero-terminated" : "non-zero-terminated",
                               config->clear_ ? "clear" : "no-clear");
  g_test_add_data_func (test_name, config, test_func);
  g_free (test_name);
}

int
main (int argc, char *argv[])
{
  /* Test all possible combinations of g_array_new() parameters. */
  const ArrayTestData array_configurations[] =
    {
      { FALSE, FALSE },
      { FALSE, TRUE },
      { TRUE, FALSE },
      { TRUE, TRUE },
    };
  xsize_t i;

  g_test_init (&argc, &argv, NULL);

  /* array tests */
  g_test_add_func ("/array/new/zero-terminated", array_new_zero_terminated);
  g_test_add_func ("/array/ref-count", array_ref_count);
  g_test_add_func ("/array/steal", array_steal);
  g_test_add_func ("/array/clear-func", array_clear_func);
  g_test_add_func ("/array/binary-search", test_array_binary_search);
  g_test_add_func ("/array/copy-sized", test_array_copy_sized);
  g_test_add_func ("/array/overflow-append-vals", array_overflow_append_vals);
  g_test_add_func ("/array/overflow-set-size", array_overflow_set_size);

  for (i = 0; i < G_N_ELEMENTS (array_configurations); i++)
    {
      add_array_test ("/array/set-size", &array_configurations[i], array_set_size);
      add_array_test ("/array/set-size/sized", &array_configurations[i], array_set_size_sized);
      add_array_test ("/array/append-val", &array_configurations[i], array_append_val);
      add_array_test ("/array/prepend-val", &array_configurations[i], array_prepend_val);
      add_array_test ("/array/prepend-vals", &array_configurations[i], array_prepend_vals);
      add_array_test ("/array/insert-vals", &array_configurations[i], array_insert_vals);
      add_array_test ("/array/remove-index", &array_configurations[i], array_remove_index);
      add_array_test ("/array/remove-index-fast", &array_configurations[i], array_remove_index_fast);
      add_array_test ("/array/remove-range", &array_configurations[i], array_remove_range);
      add_array_test ("/array/copy", &array_configurations[i], array_copy);
      add_array_test ("/array/sort", &array_configurations[i], array_sort);
      add_array_test ("/array/sort-with-data", &array_configurations[i], array_sort_with_data);
    }

  /* pointer arrays */
  g_test_add_func ("/pointerarray/add", pointer_array_add);
  g_test_add_func ("/pointerarray/insert", pointer_array_insert);
  g_test_add_func ("/pointerarray/ref-count", pointer_array_ref_count);
  g_test_add_func ("/pointerarray/free-func", pointer_array_free_func);
  g_test_add_func ("/pointerarray/array_copy", pointer_array_copy);
  g_test_add_func ("/pointerarray/array_extend", pointer_array_extend);
  g_test_add_func ("/pointerarray/array_extend_and_steal", pointer_array_extend_and_steal);
  g_test_add_func ("/pointerarray/sort", pointer_array_sort);
  g_test_add_func ("/pointerarray/sort/example", pointer_array_sort_example);
  g_test_add_func ("/pointerarray/sort-with-data", pointer_array_sort_with_data);
  g_test_add_func ("/pointerarray/sort-with-data/example", pointer_array_sort_with_data_example);
  g_test_add_func ("/pointerarray/find/empty", pointer_array_find_empty);
  g_test_add_func ("/pointerarray/find/non-empty", pointer_array_find_non_empty);
  g_test_add_func ("/pointerarray/steal", pointer_array_steal);
  g_test_add_func ("/pointerarray/steal_index", pointer_array_steal_index);

  /* byte arrays */
  g_test_add_func ("/bytearray/steal", byte_array_steal);
  g_test_add_func ("/bytearray/append", byte_array_append);
  g_test_add_func ("/bytearray/prepend", byte_array_prepend);
  g_test_add_func ("/bytearray/remove", byte_array_remove);
  g_test_add_func ("/bytearray/remove-fast", byte_array_remove_fast);
  g_test_add_func ("/bytearray/remove-range", byte_array_remove_range);
  g_test_add_func ("/bytearray/ref-count", byte_array_ref_count);
  g_test_add_func ("/bytearray/sort", byte_array_sort);
  g_test_add_func ("/bytearray/sort-with-data", byte_array_sort_with_data);
  g_test_add_func ("/bytearray/new-take", byte_array_new_take);
  g_test_add_func ("/bytearray/new-take-overflow", byte_array_new_take_overflow);
  g_test_add_func ("/bytearray/free-to-bytes", byte_array_free_to_bytes);

  return g_test_run ();
}
