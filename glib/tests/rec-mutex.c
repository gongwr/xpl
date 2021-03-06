/* Unit tests for GRecMutex
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
test_rec_mutex1 (void)
{
  GRecMutex mutex;

  g_rec_mutex_init (&mutex);
  g_rec_mutex_lock (&mutex);
  g_rec_mutex_unlock (&mutex);
  g_rec_mutex_lock (&mutex);
  g_rec_mutex_unlock (&mutex);
  g_rec_mutex_clear (&mutex);
}

static void
test_rec_mutex2 (void)
{
  static GRecMutex mutex;

  g_rec_mutex_lock (&mutex);
  g_rec_mutex_unlock (&mutex);
  g_rec_mutex_lock (&mutex);
  g_rec_mutex_unlock (&mutex);
}

static void
test_rec_mutex3 (void)
{
  static GRecMutex mutex;
  xboolean_t ret;

  ret = g_rec_mutex_trylock (&mutex);
  xassert (ret);

  ret = g_rec_mutex_trylock (&mutex);
  xassert (ret);

  g_rec_mutex_unlock (&mutex);
  g_rec_mutex_unlock (&mutex);
}

#define LOCKS      48
#define ITERATIONS 10000
#define THREADS    100


xthread_t   *owners[LOCKS];
GRecMutex  locks[LOCKS];

static void
acquire (xint_t nr)
{
  xthread_t *self;

  self = xthread_self ();

  if (!g_rec_mutex_trylock (&locks[nr]))
    {
      if (g_test_verbose ())
        g_printerr ("thread %p going to block on lock %d\n", self, nr);

      g_rec_mutex_lock (&locks[nr]);
    }

  xassert (owners[nr] == NULL);   /* hopefully nobody else is here */
  owners[nr] = self;

  /* let some other threads try to ruin our day */
  xthread_yield ();
  xthread_yield ();

  xassert (owners[nr] == self);   /* hopefully this is still us... */

  if (g_test_verbose ())
    g_printerr ("thread %p recursively taking lock %d\n", self, nr);

  g_rec_mutex_lock (&locks[nr]);  /* we're recursive, after all */

  xassert (owners[nr] == self);   /* hopefully this is still us... */

  g_rec_mutex_unlock (&locks[nr]);

  xthread_yield ();
  xthread_yield ();

  xassert (owners[nr] == self);   /* hopefully this is still us... */
  owners[nr] = NULL;               /* make way for the next guy */

  g_rec_mutex_unlock (&locks[nr]);
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
test_rec_mutex4 (void)
{
  xint_t i;
  xthread_t *threads[THREADS];

  for (i = 0; i < LOCKS; i++)
    g_rec_mutex_init (&locks[i]);

  for (i = 0; i < THREADS; i++)
    threads[i] = xthread_new ("test", thread_func, NULL);

  for (i = 0; i < THREADS; i++)
    xthread_join (threads[i]);

  for (i = 0; i < LOCKS; i++)
    g_rec_mutex_clear (&locks[i]);

  for (i = 0; i < LOCKS; i++)
    xassert (owners[i] == NULL);
}

#define COUNT_TO 100000000

static xint_t depth;

static xboolean_t
do_addition (xint_t *value)
{
  static GRecMutex lock;
  xboolean_t more;
  xint_t i;

  /* test performance of "good" cases (ie: short critical sections) */
  for (i = 0; i < depth; i++)
    g_rec_mutex_lock (&lock);

  if ((more = *value != COUNT_TO))
    if (*value != -1)
      (*value)++;

  for (i = 0; i < depth; i++)
    g_rec_mutex_unlock (&lock);

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
  xint_t c = GPOINTER_TO_INT (data);
  xthread_t *threads[THREADS];
  sint64_t start_time;
  xint_t n_threads;
  xdouble_t rate;
  xint_t x = -1;
  xint_t i;

  n_threads = c / 256;
  depth = c % 256;

  for (i = 0; i < n_threads - 1; i++)
    threads[i] = xthread_new ("test", addition_thread, &x);

  /* avoid measuring thread setup/teardown time */
  start_time = g_get_monotonic_time ();
  g_atomic_int_set (&x, 0);
  addition_thread (&x);
  g_assert_cmpint (g_atomic_int_get (&x), ==, COUNT_TO);
  rate = g_get_monotonic_time () - start_time;
  rate = x / rate;

  for (i = 0; i < n_threads - 1; i++)
    xthread_join (threads[i]);

  g_test_maximized_result (rate, "%f mips", rate);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/thread/rec-mutex1", test_rec_mutex1);
  g_test_add_func ("/thread/rec-mutex2", test_rec_mutex2);
  g_test_add_func ("/thread/rec-mutex3", test_rec_mutex3);
  g_test_add_func ("/thread/rec-mutex4", test_rec_mutex4);

  if (g_test_perf ())
    {
      xint_t i, j;

      for (i = 0; i < 5; i++)
        for (j = 1; j <= 5; j++)
          {
            xchar_t name[80];
            xuint_t c;

            c = i * 256 + j;

            if (i)
              sprintf (name, "/thread/rec-mutex/perf/contended%d/depth%d", i, j);
            else
              sprintf (name, "/thread/rec-mutex/perf/uncontended/depth%d", j);

            g_test_add_data_func (name, GINT_TO_POINTER (c), test_mutex_perf);
          }
    }

  return g_test_run ();
}
