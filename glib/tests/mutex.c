/* Unit tests for xmutex_t
 * Copyright (C) 2011 Red Hat, Inc
 * Author: Matthias Clasen
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

#include <stdio.h>

static void
test_mutex1 (void)
{
  xmutex_t mutex;

  g_mutex_init (&mutex);
  g_mutex_lock (&mutex);
  g_mutex_unlock (&mutex);
  g_mutex_lock (&mutex);
  g_mutex_unlock (&mutex);
  g_mutex_clear (&mutex);
}

static void
test_mutex2 (void)
{
  static xmutex_t mutex;

  g_mutex_lock (&mutex);
  g_mutex_unlock (&mutex);
  g_mutex_lock (&mutex);
  g_mutex_unlock (&mutex);
}

static void
test_mutex3 (void)
{
  xmutex_t *mutex;

  mutex = g_mutex_new ();
  g_mutex_lock (mutex);
  g_mutex_unlock (mutex);
  g_mutex_lock (mutex);
  g_mutex_unlock (mutex);
  g_mutex_free (mutex);
}

static void
test_mutex4 (void)
{
  static xmutex_t mutex;
  xboolean_t ret;

  ret = g_mutex_trylock (&mutex);
  xassert (ret);

  /* no guarantees that mutex is recursive, so could return 0 or 1 */
  if (g_mutex_trylock (&mutex))
    g_mutex_unlock (&mutex);

  g_mutex_unlock (&mutex);
}

#define LOCKS      48
#define ITERATIONS 10000
#define THREADS    100


xthread_t *owners[LOCKS];
xmutex_t   locks[LOCKS];

static void
acquire (xint_t nr)
{
  xthread_t *self;

  self = xthread_self ();

  if (!g_mutex_trylock (&locks[nr]))
    {
      if (g_test_verbose ())
        g_printerr ("thread %p going to block on lock %d\n", self, nr);

      g_mutex_lock (&locks[nr]);
    }

  xassert (owners[nr] == NULL);   /* hopefully nobody else is here */
  owners[nr] = self;

  /* let some other threads try to ruin our day */
  xthread_yield ();
  xthread_yield ();
  xthread_yield ();

  xassert (owners[nr] == self);   /* hopefully this is still us... */
  owners[nr] = NULL;               /* make way for the next guy */

  g_mutex_unlock (&locks[nr]);
}

static xpointer_t
thread_func (xpointer_t data)
{
  xint_t i;
  xrand_t *rand;

  rand = g_rand_new ();

  for (i = 0; i < ITERATIONS; i++)
    acquire (g_rand_int_range (rand, 0, LOCKS));

  g_rand_free (rand);

  return NULL;
}

static void
test_mutex5 (void)
{
  xint_t i;
  xthread_t *threads[THREADS];

  for (i = 0; i < LOCKS; i++)
    g_mutex_init (&locks[i]);

  for (i = 0; i < THREADS; i++)
    threads[i] = xthread_create (thread_func, NULL, TRUE, NULL);

  for (i = 0; i < THREADS; i++)
    xthread_join (threads[i]);

  for (i = 0; i < LOCKS; i++)
    g_mutex_clear (&locks[i]);

  for (i = 0; i < LOCKS; i++)
    xassert (owners[i] == NULL);
}

#define COUNT_TO 100000000

static xboolean_t
do_addition (xint_t *value)
{
  static xmutex_t lock;
  xboolean_t more;

  /* test performance of "good" cases (ie: short critical sections) */
  g_mutex_lock (&lock);
  if ((more = *value != COUNT_TO))
    if (*value != -1)
      (*value)++;
  g_mutex_unlock (&lock);

  return more;
}

static xpointer_t
addition_thread (xpointer_t value)
{
  while (do_addition (value));

  return NULL;
}

static void
test_mutex_perf (xconstpointer data)
{
  xuint_t n_threads = GPOINTER_TO_UINT (data);
  xthread_t *threads[THREADS];
  sint64_t start_time;
  xdouble_t rate;
  xint_t x = -1;
  xuint_t i;

  xassert (n_threads <= G_N_ELEMENTS (threads));

  for (i = 0; n_threads > 0 && i < n_threads - 1; i++)
    threads[i] = xthread_create (addition_thread, &x, TRUE, NULL);

  /* avoid measuring thread setup/teardown time */
  start_time = g_get_monotonic_time ();
  g_atomic_int_set (&x, 0);
  addition_thread (&x);
  g_assert_cmpint (g_atomic_int_get (&x), ==, COUNT_TO);
  rate = g_get_monotonic_time () - start_time;
  rate = x / rate;

  for (i = 0; n_threads > 0 && i < n_threads - 1; i++)
    xthread_join (threads[i]);

  g_test_maximized_result (rate, "%f mips", rate);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/thread/mutex1", test_mutex1);
  g_test_add_func ("/thread/mutex2", test_mutex2);
  g_test_add_func ("/thread/mutex3", test_mutex3);
  g_test_add_func ("/thread/mutex4", test_mutex4);
  g_test_add_func ("/thread/mutex5", test_mutex5);

  if (g_test_perf ())
    {
      xuint_t i;

      g_test_add_data_func ("/thread/mutex/perf/uncontended", GUINT_TO_POINTER (0), test_mutex_perf);

      for (i = 1; i <= 10; i++)
        {
          xchar_t name[80];
          sprintf (name, "/thread/mutex/perf/contended/%u", i);
          g_test_add_data_func (name, GUINT_TO_POINTER (i), test_mutex_perf);
        }
    }

  return g_test_run ();
}
