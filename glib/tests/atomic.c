/*
 * Copyright 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#include <glib.h>

/* We want the g_atomic_pointer_get() macros to work when compiling third party
 * projects with -Wbad-function-cast.
 * See https://gitlab.gnome.org/GNOME/glib/issues/1041. */
#pragma GCC diagnostic error "-Wbad-function-cast"

static void
test_types (void)
{
  const xint_t *csp;
  const xint_t * const *cspp;
  xuint_t u, u2;
  xint_t s, s2;
  xpointer_t vp, vp2;
  const char *vp_str;
  const char *volatile vp_str_vol;
  const char *str = "Hello";
  int *ip, *ip2;
  xsize_t gs, gs2;
  xboolean_t res;

  csp = &s;
  cspp = &csp;

  g_atomic_int_set (&u, 5);
  u2 = (xuint_t) g_atomic_int_get (&u);
  g_assert_cmpint (u2, ==, 5);
  res = g_atomic_int_compare_and_exchange (&u, 6, 7);
  g_assert_false (res);
  g_assert_cmpint (u, ==, 5);
  g_atomic_int_add (&u, 1);
  g_assert_cmpint (u, ==, 6);
  g_atomic_int_inc (&u);
  g_assert_cmpint (u, ==, 7);
  res = g_atomic_int_dec_and_test (&u);
  g_assert_false (res);
  g_assert_cmpint (u, ==, 6);
  u2 = g_atomic_int_and (&u, 5);
  g_assert_cmpint (u2, ==, 6);
  g_assert_cmpint (u, ==, 4);
  u2 = g_atomic_int_or (&u, 8);
  g_assert_cmpint (u2, ==, 4);
  g_assert_cmpint (u, ==, 12);
  u2 = g_atomic_int_xor (&u, 4);
  g_assert_cmpint (u2, ==, 12);
  g_assert_cmpint (u, ==, 8);

  g_atomic_int_set (&s, 5);
  s2 = g_atomic_int_get (&s);
  g_assert_cmpint (s2, ==, 5);
  res = g_atomic_int_compare_and_exchange (&s, 6, 7);
  g_assert_false (res);
  g_assert_cmpint (s, ==, 5);
  g_atomic_int_add (&s, 1);
  g_assert_cmpint (s, ==, 6);
  g_atomic_int_inc (&s);
  g_assert_cmpint (s, ==, 7);
  res = g_atomic_int_dec_and_test (&s);
  g_assert_false (res);
  g_assert_cmpint (s, ==, 6);
  s2 = (xint_t) g_atomic_int_and (&s, 5);
  g_assert_cmpint (s2, ==, 6);
  g_assert_cmpint (s, ==, 4);
  s2 = (xint_t) g_atomic_int_or (&s, 8);
  g_assert_cmpint (s2, ==, 4);
  g_assert_cmpint (s, ==, 12);
  s2 = (xint_t) g_atomic_int_xor (&s, 4);
  g_assert_cmpint (s2, ==, 12);
  g_assert_cmpint (s, ==, 8);

  g_atomic_pointer_set (&vp, 0);
  vp2 = g_atomic_pointer_get (&vp);
  g_assert_true (vp2 == 0);
  res = g_atomic_pointer_compare_and_exchange (&vp, &s, &s);
  g_assert_false (res);
  g_assert_true (vp == 0);
  res = g_atomic_pointer_compare_and_exchange (&vp, NULL, NULL);
  g_assert_true (res);
  g_assert_true (vp == 0);

  g_atomic_pointer_set (&vp_str, NULL);
  res = g_atomic_pointer_compare_and_exchange (&vp_str, NULL, str);
  g_assert_true (res);

  /* Note that atomic variables should almost certainly not be marked as
   * `volatile` — see http://isvolatileusefulwiththreads.in/c/. This test exists
   * to make sure that we don’t warn when built against older third party code. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
  g_atomic_pointer_set (&vp_str_vol, NULL);
  res = g_atomic_pointer_compare_and_exchange (&vp_str_vol, NULL, str);
  g_assert_true (res);
#pragma GCC diagnostic pop

  g_atomic_pointer_set (&ip, 0);
  ip2 = g_atomic_pointer_get (&ip);
  g_assert_true (ip2 == 0);
  res = g_atomic_pointer_compare_and_exchange (&ip, NULL, NULL);
  g_assert_true (res);
  g_assert_true (ip == 0);

  g_atomic_pointer_set (&gs, 0);
  vp2 = (xpointer_t) g_atomic_pointer_get (&gs);
  gs2 = (xsize_t) vp2;
  g_assert_cmpuint (gs2, ==, 0);
  res = g_atomic_pointer_compare_and_exchange (&gs, NULL, (xsize_t) NULL);
  g_assert_true (res);
  g_assert_cmpuint (gs, ==, 0);
  gs2 = (xsize_t) g_atomic_pointer_add (&gs, 5);
  g_assert_cmpuint (gs2, ==, 0);
  g_assert_cmpuint (gs, ==, 5);
  gs2 = g_atomic_pointer_and (&gs, 6);
  g_assert_cmpuint (gs2, ==, 5);
  g_assert_cmpuint (gs, ==, 4);
  gs2 = g_atomic_pointer_or (&gs, 8);
  g_assert_cmpuint (gs2, ==, 4);
  g_assert_cmpuint (gs, ==, 12);
  gs2 = g_atomic_pointer_xor (&gs, 4);
  g_assert_cmpuint (gs2, ==, 12);
  g_assert_cmpuint (gs, ==, 8);

  g_assert_cmpint (g_atomic_int_get (csp), ==, s);
  g_assert_true (g_atomic_pointer_get ((const xint_t **) cspp) == csp);

  /* repeat, without the macros */
#undef g_atomic_int_set
#undef g_atomic_int_get
#undef g_atomic_int_compare_and_exchange
#undef g_atomic_int_add
#undef g_atomic_int_inc
#undef g_atomic_int_and
#undef g_atomic_int_or
#undef g_atomic_int_xor
#undef g_atomic_int_dec_and_test
#undef g_atomic_pointer_set
#undef g_atomic_pointer_get
#undef g_atomic_pointer_compare_and_exchange
#undef g_atomic_pointer_add
#undef g_atomic_pointer_and
#undef g_atomic_pointer_or
#undef g_atomic_pointer_xor

  g_atomic_int_set ((xint_t*)&u, 5);
  u2 = (xuint_t) g_atomic_int_get ((xint_t*)&u);
  g_assert_cmpint (u2, ==, 5);
  res = g_atomic_int_compare_and_exchange ((xint_t*)&u, 6, 7);
  g_assert_false (res);
  g_assert_cmpint (u, ==, 5);
  g_atomic_int_add ((xint_t*)&u, 1);
  g_assert_cmpint (u, ==, 6);
  g_atomic_int_inc ((xint_t*)&u);
  g_assert_cmpint (u, ==, 7);
  res = g_atomic_int_dec_and_test ((xint_t*)&u);
  g_assert_false (res);
  g_assert_cmpint (u, ==, 6);
  u2 = g_atomic_int_and (&u, 5);
  g_assert_cmpint (u2, ==, 6);
  g_assert_cmpint (u, ==, 4);
  u2 = g_atomic_int_or (&u, 8);
  g_assert_cmpint (u2, ==, 4);
  g_assert_cmpint (u, ==, 12);
  u2 = g_atomic_int_xor (&u, 4);
  g_assert_cmpint (u2, ==, 12);

  g_atomic_int_set (&s, 5);
  s2 = g_atomic_int_get (&s);
  g_assert_cmpint (s2, ==, 5);
  res = g_atomic_int_compare_and_exchange (&s, 6, 7);
  g_assert_false (res);
  g_assert_cmpint (s, ==, 5);
  g_atomic_int_add (&s, 1);
  g_assert_cmpint (s, ==, 6);
  g_atomic_int_inc (&s);
  g_assert_cmpint (s, ==, 7);
  res = g_atomic_int_dec_and_test (&s);
  g_assert_false (res);
  g_assert_cmpint (s, ==, 6);
  s2 = (xint_t) g_atomic_int_and ((xuint_t*)&s, 5);
  g_assert_cmpint (s2, ==, 6);
  g_assert_cmpint (s, ==, 4);
  s2 = (xint_t) g_atomic_int_or ((xuint_t*)&s, 8);
  g_assert_cmpint (s2, ==, 4);
  g_assert_cmpint (s, ==, 12);
  s2 = (xint_t) g_atomic_int_xor ((xuint_t*)&s, 4);
  g_assert_cmpint (s2, ==, 12);
  g_assert_cmpint (s, ==, 8);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  s2 = g_atomic_int_exchange_and_add ((xint_t*)&s, 1);
G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint (s2, ==, 8);
  g_assert_cmpint (s, ==, 9);

  g_atomic_pointer_set (&vp, 0);
  vp2 = g_atomic_pointer_get (&vp);
  g_assert_true (vp2 == 0);
  res = g_atomic_pointer_compare_and_exchange (&vp, &s, &s);
  g_assert_false (res);
  g_assert_true (vp == 0);
  res = g_atomic_pointer_compare_and_exchange (&vp, NULL, NULL);
  g_assert_true (res);
  g_assert_true (vp == 0);

  g_atomic_pointer_set (&vp_str, NULL);
  res = g_atomic_pointer_compare_and_exchange (&vp_str, NULL, (char *) str);
  g_assert_true (res);

  /* Note that atomic variables should almost certainly not be marked as
   * `volatile` — see http://isvolatileusefulwiththreads.in/c/. This test exists
   * to make sure that we don’t warn when built against older third party code. */
  g_atomic_pointer_set (&vp_str_vol, NULL);
  res = g_atomic_pointer_compare_and_exchange (&vp_str_vol, NULL, (char *) str);
  g_assert_true (res);

  g_atomic_pointer_set (&ip, 0);
  ip2 = g_atomic_pointer_get (&ip);
  g_assert_true (ip2 == 0);
  res = g_atomic_pointer_compare_and_exchange (&ip, NULL, NULL);
  g_assert_true (res);
  g_assert_true (ip == 0);

  g_atomic_pointer_set (&gs, 0);
  vp = g_atomic_pointer_get (&gs);
  gs2 = (xsize_t) vp;
  g_assert_cmpuint (gs2, ==, 0);
  res = g_atomic_pointer_compare_and_exchange (&gs, NULL, NULL);
  g_assert_true (res);
  g_assert_cmpuint (gs, ==, 0);
  gs2 = (xsize_t) g_atomic_pointer_add (&gs, 5);
  g_assert_cmpuint (gs2, ==, 0);
  g_assert_cmpuint (gs, ==, 5);
  gs2 = g_atomic_pointer_and (&gs, 6);
  g_assert_cmpuint (gs2, ==, 5);
  g_assert_cmpuint (gs, ==, 4);
  gs2 = g_atomic_pointer_or (&gs, 8);
  g_assert_cmpuint (gs2, ==, 4);
  g_assert_cmpuint (gs, ==, 12);
  gs2 = g_atomic_pointer_xor (&gs, 4);
  g_assert_cmpuint (gs2, ==, 12);
  g_assert_cmpuint (gs, ==, 8);

  g_assert_cmpint (g_atomic_int_get (csp), ==, s);
  g_assert_true (g_atomic_pointer_get (cspp) == csp);
}

#define THREADS 10
#define ROUNDS 10000

xint_t bucket[THREADS];  /* never contested by threads, not accessed atomically */
xint_t atomic;  /* (atomic) */

static xpointer_t
thread_func (xpointer_t data)
{
  xint_t idx = GPOINTER_TO_INT (data);
  xint_t i;
  xint_t d;

  for (i = 0; i < ROUNDS; i++)
    {
      d = g_random_int_range (-10, 100);
      bucket[idx] += d;
      g_atomic_int_add (&atomic, d);
      xthread_yield ();
    }

  return NULL;
}

static void
test_threaded (void)
{
  xint_t sum;
  xint_t i;
  xthread_t *threads[THREADS];

  atomic = 0;
  for (i = 0; i < THREADS; i++)
    bucket[i] = 0;

  for (i = 0; i < THREADS; i++)
    threads[i] = xthread_new ("atomic", thread_func, GINT_TO_POINTER (i));

  for (i = 0; i < THREADS; i++)
    xthread_join (threads[i]);

  sum = 0;
  for (i = 0; i < THREADS; i++)
    sum += bucket[i];

  g_assert_cmpint (sum, ==, atomic);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/atomic/types", test_types);
  g_test_add_func ("/atomic/threaded", test_threaded);

  return g_test_run ();
}
