/*
 * Copyright 2011 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#undef G_DISABLE_ASSERT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glib.h"

/* Keep in sync with glib/gbytes.c */
struct _GBytes
{
  xconstpointer data;
  xsize_t size;
  xint_t ref_count;
  xdestroy_notify_t free_func;
  xpointer_t user_data;
};

static const xchar_t *NYAN = "nyannyan";
static const xsize_t N_NYAN = 8;

static void
test_new (void)
{
  const xchar_t *data;
  xbytes_t *bytes;
  xsize_t size;

  data = "test";
  bytes = xbytes_new (data, 4);
  g_assert_nonnull (bytes);
  g_assert_true (xbytes_get_data (bytes, &size) != data);
  g_assert_cmpuint (size, ==, 4);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, 4);
  g_assert_cmpmem (data, 4, xbytes_get_data (bytes, NULL), xbytes_get_size (bytes));

  xbytes_unref (bytes);
}

static void
test_new_take (void)
{
  xchar_t *data;
  xbytes_t *bytes;
  xsize_t size;

  data = xstrdup ("test");
  bytes = xbytes_new_take (data, 4);
  g_assert_nonnull (bytes);
  g_assert_true (xbytes_get_data (bytes, &size) == data);
  g_assert_cmpuint (size, ==, 4);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, 4);

  xbytes_unref (bytes);
}

static void
test_new_static (void)
{
  const xchar_t *data;
  xbytes_t *bytes;
  xsize_t size;

  data = "test";
  bytes = xbytes_new_static (data, 4);
  g_assert_nonnull (bytes);
  g_assert_true (xbytes_get_data (bytes, &size) == data);
  g_assert_cmpuint (size, ==, 4);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, 4);

  xbytes_unref (bytes);
}

static void
test_new_from_bytes (void)
{
  const xchar_t *data = "smile and wave";
  xbytes_t *bytes;
  xbytes_t *sub;

  bytes = xbytes_new (data, 14);
  sub = xbytes_new_from_bytes (bytes, 10, 4);

  g_assert_nonnull (sub);
  g_assert_true (xbytes_get_data (sub, NULL) == ((xchar_t *)xbytes_get_data (bytes, NULL)) + 10);
  xbytes_unref (bytes);

  g_assert_cmpmem (xbytes_get_data (sub, NULL), xbytes_get_size (sub), "wave", 4);
  xbytes_unref (sub);
}

/* Verify that creating slices of xbytes_t reference the top-most bytes
 * at the correct offset. Ensure that intermediate xbytes_t are not referenced.
 */
static void
test_new_from_bytes_slice (void)
{
  xbytes_t *bytes = xbytes_new_static ("Some stupid data", strlen ("Some stupid data") + 1);
  xbytes_t *bytes1 = xbytes_new_from_bytes (bytes, 4, 13);
  xbytes_t *bytes2 = xbytes_new_from_bytes (bytes1, 1, 12);
  xbytes_t *bytes3 = xbytes_new_from_bytes (bytes2, 0, 6);

  g_assert_cmpint (bytes->ref_count, ==, 4);
  g_assert_cmpint (bytes1->ref_count, ==, 1);
  g_assert_cmpint (bytes2->ref_count, ==, 1);
  g_assert_cmpint (bytes3->ref_count, ==, 1);

  g_assert_null (bytes->user_data);
  g_assert_true (bytes1->user_data == bytes);
  g_assert_true (bytes2->user_data == bytes);
  g_assert_true (bytes3->user_data == bytes);

  g_assert_cmpint (17, ==, xbytes_get_size (bytes));
  g_assert_cmpint (13, ==, xbytes_get_size (bytes1));
  g_assert_cmpint (12, ==, xbytes_get_size (bytes2));
  g_assert_cmpint (6, ==, xbytes_get_size (bytes3));

  g_assert_cmpint (0, ==, strncmp ("Some stupid data", (xchar_t *)bytes->data, 17));
  g_assert_cmpint (0, ==, strncmp (" stupid data", (xchar_t *)bytes1->data, 13));
  g_assert_cmpint (0, ==, strncmp ("stupid data", (xchar_t *)bytes2->data, 12));
  g_assert_cmpint (0, ==, strncmp ("stupid", (xchar_t *)bytes3->data, 6));

  xbytes_unref (bytes);
  xbytes_unref (bytes1);
  xbytes_unref (bytes2);
  xbytes_unref (bytes3);
}

/* Ensure that referencing an entire xbytes_t just returns the same bytes
 * instance (with incremented reference count) instead of a new instance.
 */
static void
test_new_from_bytes_shared_ref (void)
{
  xbytes_t *bytes = xbytes_new_static ("Some data", strlen ("Some data") + 1);
  xbytes_t *other = xbytes_new_from_bytes (bytes, 0, xbytes_get_size (bytes));

  g_assert_true (bytes == other);
  g_assert_cmpint (bytes->ref_count, ==, 2);

  xbytes_unref (bytes);
  xbytes_unref (other);
}

static void
on_destroy_increment (xpointer_t data)
{
  xint_t *count = data;
  g_assert_nonnull (count);
  (*count)++;
}

static void
test_new_with_free_func (void)
{
  xbytes_t *bytes;
  xchar_t *data;
  xint_t count = 0;
  xsize_t size;

  data = "test";
  bytes = xbytes_new_with_free_func (data, 4, on_destroy_increment, &count);
  g_assert_nonnull (bytes);
  g_assert_cmpint (count, ==, 0);
  g_assert_true (xbytes_get_data (bytes, &size) == data);
  g_assert_cmpuint (size, ==, 4);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, 4);

  xbytes_unref (bytes);
  g_assert_cmpuint (count, ==, 1);
}

static void
test_hash (void)
{
  xbytes_t *bytes1;
  xbytes_t *bytes2;
  xuint_t hash1;
  xuint_t hash2;

  bytes1 = xbytes_new ("blah", 4);
  bytes2 = xbytes_new ("blah", 4);

  hash1 = xbytes_hash (bytes1);
  hash2 = xbytes_hash (bytes2);
  g_assert_cmpuint (hash1,  ==, hash2);

  xbytes_unref (bytes1);
  xbytes_unref (bytes2);
}

static void
test_equal (void)
{
  xbytes_t *bytes;
  xbytes_t *bytes2;

  bytes = xbytes_new ("blah", 4);

  bytes2 = xbytes_new ("blah", 4);
  g_assert_true (xbytes_equal (bytes, bytes2));
  g_assert_true (xbytes_equal (bytes2, bytes));
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("bla", 3);
  g_assert_false (xbytes_equal (bytes, bytes2));
  g_assert_false (xbytes_equal (bytes2, bytes));
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("true", 4);
  g_assert_false (xbytes_equal (bytes, bytes2));
  g_assert_false (xbytes_equal (bytes2, bytes));
  xbytes_unref (bytes2);

  xbytes_unref (bytes);
}

static void
test_compare (void)
{
  xbytes_t *bytes;
  xbytes_t *bytes2;

  bytes = xbytes_new ("blah", 4);

  bytes2 = xbytes_new ("blah", 4);
  g_assert_cmpint (xbytes_compare (bytes, bytes2), ==, 0);
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("bla", 3);
  g_assert_cmpint (xbytes_compare (bytes, bytes2), >, 0);
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("abcd", 4);
  g_assert_cmpint (xbytes_compare (bytes, bytes2), >, 0);
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("blahblah", 8);
  g_assert_cmpint (xbytes_compare (bytes, bytes2), <, 0);
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("zyx", 3);
  g_assert_cmpint (xbytes_compare (bytes, bytes2), <, 0);
  xbytes_unref (bytes2);

  bytes2 = xbytes_new ("zyxw", 4);
  g_assert_cmpint (xbytes_compare (bytes, bytes2), <, 0);
  xbytes_unref (bytes2);

  xbytes_unref (bytes);
}

static void
test_to_data_transferred (void)
{
  xconstpointer memory;
  xpointer_t data;
  xsize_t size;
  xbytes_t *bytes;

  /* Memory transferred: one reference, and allocated with g_malloc */
  bytes = xbytes_new (NYAN, N_NYAN);
  memory = xbytes_get_data (bytes, NULL);
  data = xbytes_unref_to_data (bytes, &size);
  g_assert_true (data == memory);
  g_assert_cmpmem (data, size, NYAN, N_NYAN);
  g_free (data);
}

static void
test_to_data_two_refs (void)
{
  xconstpointer memory;
  xpointer_t data;
  xsize_t size;
  xbytes_t *bytes;

  /* Memory copied: two references */
  bytes = xbytes_new (NYAN, N_NYAN);
  bytes = xbytes_ref (bytes);
  memory = xbytes_get_data (bytes, NULL);
  data = xbytes_unref_to_data (bytes, &size);
  g_assert_true (data != memory);
  g_assert_cmpmem (data, size, NYAN, N_NYAN);
  g_free (data);
  g_assert_true (xbytes_get_data (bytes, &size) == memory);
  g_assert_cmpuint (size, ==, N_NYAN);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, N_NYAN);
  xbytes_unref (bytes);
}

static void
test_to_data_non_malloc (void)
{
  xpointer_t data;
  xsize_t size;
  xbytes_t *bytes;

  /* Memory copied: non malloc memory */
  bytes = xbytes_new_static (NYAN, N_NYAN);
  g_assert_true (xbytes_get_data (bytes, NULL) == NYAN);
  data = xbytes_unref_to_data (bytes, &size);
  g_assert_true (data != (xpointer_t)NYAN);
  g_assert_cmpmem (data, size, NYAN, N_NYAN);
  g_free (data);
}

static void
test_to_data_different_free_func (void)
{
  xpointer_t data;
  xsize_t size;
  xbytes_t *bytes;
  xchar_t *sentinel = xstrdup ("hello");

  /* Memory copied: free func and user_data don’t point to the bytes data */
  bytes = xbytes_new_with_free_func (NYAN, N_NYAN, g_free, sentinel);
  g_assert_true (xbytes_get_data (bytes, NULL) == NYAN);

  data = xbytes_unref_to_data (bytes, &size);
  g_assert_true (data != (xpointer_t)NYAN);
  g_assert_cmpmem (data, size, NYAN, N_NYAN);
  g_free (data);

  /* @sentinel should not be leaked; testing that requires this test to be run
   * under valgrind. We can’t use a custom free func to check it isn’t leaked,
   * as the point of this test is to hit a condition in `try_steal_and_unref()`
   * which is short-circuited if the free func isn’t g_free().
   * See discussion in https://gitlab.gnome.org/GNOME/glib/-/merge_requests/2152 */
}

static void
test_to_array_transferred (void)
{
  xconstpointer memory;
  xbyte_array_t *array;
  xbytes_t *bytes;

  /* Memory transferred: one reference, and allocated with g_malloc */
  bytes = xbytes_new (NYAN, N_NYAN);
  memory = xbytes_get_data (bytes, NULL);
  array = xbytes_unref_to_array (bytes);
  g_assert_nonnull (array);
  g_assert_true (array->data == memory);
  g_assert_cmpmem (array->data, array->len, NYAN, N_NYAN);
  xbyte_array_unref (array);
}

static void
test_to_array_transferred_oversize (void)
{
  g_test_message ("xbytes_unref_to_array() can only take xbytes_t up to "
                  "G_MAXUINT in length; test that longer ones are rejected");

  if (sizeof (xuint_t) >= sizeof (xsize_t))
    {
      g_test_skip ("Skipping test as xuint_t is not smaller than xsize_t");
    }
  else if (g_test_undefined ())
    {
      xbyte_array_t *array = NULL;
      xbytes_t *bytes = NULL;
      xpointer_t data = g_memdup2 (NYAN, N_NYAN);
      xsize_t len = ((xsize_t) G_MAXUINT) + 1;

      bytes = xbytes_new_take (data, len);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "xbyte_array_new_take: assertion 'len <= G_MAXUINT' failed");
      array = xbytes_unref_to_array (g_steal_pointer (&bytes));
      g_test_assert_expected_messages ();
      g_assert_null (array);

      g_free (data);
    }
  else
    {
      g_test_skip ("Skipping test as testing undefined behaviour is disabled");
    }
}

static void
test_to_array_two_refs (void)
{
  xconstpointer memory;
  xbyte_array_t *array;
  xbytes_t *bytes;
  xsize_t size;

  /* Memory copied: two references */
  bytes = xbytes_new (NYAN, N_NYAN);
  bytes = xbytes_ref (bytes);
  memory = xbytes_get_data (bytes, NULL);
  array = xbytes_unref_to_array (bytes);
  g_assert_nonnull (array);
  g_assert_true (array->data != memory);
  g_assert_cmpmem (array->data, array->len, NYAN, N_NYAN);
  xbyte_array_unref (array);
  g_assert_true (xbytes_get_data (bytes, &size) == memory);
  g_assert_cmpuint (size, ==, N_NYAN);
  g_assert_cmpuint (xbytes_get_size (bytes), ==, N_NYAN);
  xbytes_unref (bytes);
}

static void
test_to_array_non_malloc (void)
{
  xbyte_array_t *array;
  xbytes_t *bytes;

  /* Memory copied: non malloc memory */
  bytes = xbytes_new_static (NYAN, N_NYAN);
  g_assert_true (xbytes_get_data (bytes, NULL) == NYAN);
  array = xbytes_unref_to_array (bytes);
  g_assert_nonnull (array);
  g_assert_true (array->data != (xpointer_t)NYAN);
  g_assert_cmpmem (array->data, array->len, NYAN, N_NYAN);
  xbyte_array_unref (array);
}

static void
test_null (void)
{
  xbytes_t *bytes;
  xpointer_t data;
  xsize_t size;

  bytes = xbytes_new (NULL, 0);

  data = xbytes_unref_to_data (bytes, &size);

  g_assert_null (data);
  g_assert_cmpuint (size, ==, 0);
}

static void
test_get_region (void)
{
  xbytes_t *bytes;

  bytes = xbytes_new_static (NYAN, N_NYAN);

  /* simple valid gets at the start */
  g_assert_true (xbytes_get_region (bytes, 1, 0, 1) == NYAN);
  g_assert_true (xbytes_get_region (bytes, 1, 0, N_NYAN) == NYAN);

  /* an invalid get because the range is too wide */
  g_assert_true (xbytes_get_region (bytes, 1, 0, N_NYAN + 1) == NULL);

  /* an valid get, but of a zero-byte range at the end */
  g_assert_true (xbytes_get_region (bytes, 1, N_NYAN, 0) == NYAN + N_NYAN);

  /* not a valid get because it overlap ones byte */
  g_assert_true (xbytes_get_region (bytes, 1, N_NYAN, 1) == NULL);

  /* let's try some multiplication overflow now */
  g_assert_true (xbytes_get_region (bytes, 32, 0, G_MAXSIZE / 32 + 1) == NULL);
  g_assert_true (xbytes_get_region (bytes, G_MAXSIZE / 32 + 1, 0, 32) == NULL);

  /* and some addition overflow */
  g_assert_true (xbytes_get_region (bytes, 1, G_MAXSIZE, -G_MAXSIZE) == NULL);
  g_assert_true (xbytes_get_region (bytes, 1, G_MAXSSIZE, ((xsize_t) G_MAXSSIZE) + 1) == NULL);
  g_assert_true (xbytes_get_region (bytes, 1, G_MAXSIZE, 1) == NULL);

  xbytes_unref (bytes);
}

static void
test_unref_null (void)
{
  g_test_summary ("Test that calling xbytes_unref() on NULL is a no-op");
  xbytes_unref (NULL);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/bytes/new", test_new);
  g_test_add_func ("/bytes/new-take", test_new_take);
  g_test_add_func ("/bytes/new-static", test_new_static);
  g_test_add_func ("/bytes/new-with-free-func", test_new_with_free_func);
  g_test_add_func ("/bytes/new-from-bytes", test_new_from_bytes);
  g_test_add_func ("/bytes/new-from-bytes-slice", test_new_from_bytes_slice);
  g_test_add_func ("/bytes/new-from-bytes-shared-ref", test_new_from_bytes_shared_ref);
  g_test_add_func ("/bytes/hash", test_hash);
  g_test_add_func ("/bytes/equal", test_equal);
  g_test_add_func ("/bytes/compare", test_compare);
  g_test_add_func ("/bytes/to-data/transferred", test_to_data_transferred);
  g_test_add_func ("/bytes/to-data/two-refs", test_to_data_two_refs);
  g_test_add_func ("/bytes/to-data/non-malloc", test_to_data_non_malloc);
  g_test_add_func ("/bytes/to-data/different-free-func", test_to_data_different_free_func);
  g_test_add_func ("/bytes/to-array/transferred", test_to_array_transferred);
  g_test_add_func ("/bytes/to-array/transferred/oversize", test_to_array_transferred_oversize);
  g_test_add_func ("/bytes/to-array/two-refs", test_to_array_two_refs);
  g_test_add_func ("/bytes/to-array/non-malloc", test_to_array_non_malloc);
  g_test_add_func ("/bytes/null", test_null);
  g_test_add_func ("/bytes/get-region", test_get_region);
  g_test_add_func ("/bytes/unref-null", test_unref_null);

  return g_test_run ();
}
