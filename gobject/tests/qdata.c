/*
 * Copyright 2012 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#include <glib-object.h>

xboolean_t fail;

#define THREADS 10
#define ROUNDS 10000

xobject_t *object;
xint_t bucket[THREADS];  /* accessed from multiple threads, but should never be contested due to the sequence of thread operations */

static xpointer_t
thread_func (xpointer_t data)
{
  xint_t idx = GPOINTER_TO_INT (data);
  xint_t i;
  xint_t d;
  xint_t value;
  xint_t new_value;

  for (i = 0; i < ROUNDS; i++)
    {
      d = g_random_int_range (-10, 100);
      bucket[idx] += d;
retry:
      value = GPOINTER_TO_INT (xobject_get_data (object, "test"));
      new_value = value + d;
      if (fail)
        xobject_set_data (object, "test", GINT_TO_POINTER (new_value));
      else
        {
          if (!xobject_replace_data (object, "test",
                                      GINT_TO_POINTER (value),
                                      GINT_TO_POINTER (new_value),
                                      NULL, NULL))
            goto retry;
        }
      xthread_yield ();
    }

  return NULL;
}

static void
test_qdata_threaded (void)
{
  xint_t sum;
  xint_t i;
  xthread_t *threads[THREADS];
  xint_t result;

  object = xobject_new (XTYPE_OBJECT, NULL);
  xobject_set_data (object, "test", GINT_TO_POINTER (0));

  for (i = 0; i < THREADS; i++)
    bucket[i] = 0;

  for (i = 0; i < THREADS; i++)
    threads[i] = xthread_new ("qdata", thread_func, GINT_TO_POINTER (i));

  for (i = 0; i < THREADS; i++)
    xthread_join (threads[i]);

  sum = 0;
  for (i = 0; i < THREADS; i++)
    sum += bucket[i];

  result = GPOINTER_TO_INT (xobject_get_data (object, "test"));

  g_assert_cmpint (sum, ==, result);

  xobject_unref (object);
}

static void
test_qdata_dup (void)
{
  xchar_t *s, *s2;
  xquark quark;
  xboolean_t b;

  quark = g_quark_from_static_string ("test");
  object = xobject_new (XTYPE_OBJECT, NULL);
  s = xstrdup ("s");
  xobject_set_qdata_full (object, quark, s, g_free);

  s2 = xobject_dup_qdata (object, quark, (GDuplicateFunc)xstrdup, NULL);

  g_assert_cmpstr (s, ==, s2);
  g_assert (s != s2);

  g_free (s2);

  b = xobject_replace_qdata (object, quark, s, "s2", NULL, NULL);
  g_assert (b);

  g_free (s);

  xobject_unref (object);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  fail = !!g_getenv ("FAIL");

  g_test_add_func ("/qdata/threaded", test_qdata_threaded);
  g_test_add_func ("/qdata/dup", test_qdata_dup);

  return g_test_run ();
}
