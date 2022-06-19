/*
 * Copyright © 2008 Ryan Lortie
 * Copyright © 2010 Codethink Limited
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#include "config.h"

/* LOCKS should be more than the number of contention
 * counters in gthread.c in order to ensure we exercise
 * the case where they overlap.
 */
#define LOCKS      48
#define ITERATIONS 10000
#define THREADS    100

#include <glib.h>

#if TEST_EMULATED_FUTEX

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

  /* this is defined for the 1bit-mutex-emufutex test.
   *
   * we want to test the emulated futex even if futex(2) is available.
   */

  /* side-step some glib build stuff */
  #define XPL_COMPILATION

  /* rebuild gbitlock.c without futex support,
     defining our own version of the g_bit_*lock symbols
   */
  #undef g_pointer_bit_lock
  #undef g_pointer_bit_trylock
  #undef g_pointer_bit_unlock

  #define g_bit_lock            _emufutex_g_bit_lock
  #define g_bit_trylock         _emufutex_g_bit_trylock
  #define g_bit_unlock          _emufutex_g_bit_unlock
  #define g_pointer_bit_lock    _emufutex_g_pointer_bit_lock
  #define g_pointer_bit_trylock _emufutex_g_pointer_bit_trylock
  #define g_pointer_bit_unlock  _emufutex_g_pointer_bit_unlock

  #define G_BIT_LOCK_FORCE_FUTEX_EMULATION

  #include <glib/gbitlock.c>

#pragma GCC diagnostic pop
#endif

volatile xthread_t *owners[LOCKS];
volatile xint_t     locks[LOCKS];
volatile xpointer_t ptrs[LOCKS];
volatile xint_t     bits[LOCKS];

static void
acquire (int      nr,
         xboolean_t use_pointers)
{
  xthread_t *self;

  self = xthread_self ();

  g_assert_cmpint (((xsize_t) ptrs) % sizeof(xint_t), ==, 0);

  if (!(use_pointers ?
          g_pointer_bit_trylock (&ptrs[nr], bits[nr])
        : g_bit_trylock (&locks[nr], bits[nr])))
    {
      if (g_test_verbose ())
        g_printerr ("thread %p going to block on lock %d\n", self, nr);

      if (use_pointers)
        g_pointer_bit_lock (&ptrs[nr], bits[nr]);
      else
        g_bit_lock (&locks[nr], bits[nr]);
    }

  xassert (owners[nr] == NULL);   /* hopefully nobody else is here */
  owners[nr] = self;

  /* let some other threads try to ruin our day */
  xthread_yield ();
  xthread_yield ();
  xthread_yield ();

  xassert (owners[nr] == self);   /* hopefully this is still us... */
  owners[nr] = NULL;               /* make way for the next guy */

  if (use_pointers)
    g_pointer_bit_unlock (&ptrs[nr], bits[nr]);
  else
    g_bit_unlock (&locks[nr], bits[nr]);
}

static xpointer_t
thread_func (xpointer_t data)
{
  xboolean_t use_pointers = GPOINTER_TO_INT (data);
  xint_t i;
  xrand_t *rand;

  rand = g_rand_new ();

  for (i = 0; i < ITERATIONS; i++)
    acquire (g_rand_int_range (rand, 0, LOCKS), use_pointers);

  g_rand_free (rand);

  return NULL;
}

static void
testcase (xconstpointer data)
{
  xboolean_t use_pointers = GPOINTER_TO_INT (data);
  xthread_t *threads[THREADS];
  int i;

#ifdef TEST_EMULATED_FUTEX
  #define SUFFIX "-emufutex"

  /* ensure that we are using the emulated futex by checking
   * (at compile-time) for the existence of 'g_futex_address_list'
   */
  xassert (g_futex_address_list == NULL);
#else
  #define SUFFIX ""
#endif

  for (i = 0; i < LOCKS; i++)
    bits[i] = g_random_int () % 32;

  for (i = 0; i < THREADS; i++)
    threads[i] = xthread_new ("foo", thread_func,
                               GINT_TO_POINTER (use_pointers));

  for (i = 0; i < THREADS; i++)
    xthread_join (threads[i]);

  for (i = 0; i < LOCKS; i++)
    {
      xassert (owners[i] == NULL);
      xassert (locks[i] == 0);
    }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/glib/1bit-mutex" SUFFIX "/int", (xpointer_t) 0, testcase);
  g_test_add_data_func ("/glib/1bit-mutex" SUFFIX "/pointer", (xpointer_t) 1, testcase);

  return g_test_run ();
}
