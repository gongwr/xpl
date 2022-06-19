/* Unit tests for xcond_t
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

static xcond_t cond;
static xmutex_t mutex;
static xint_t next;  /* locked by @mutex */

static void
push_value (xint_t value)
{
  g_mutex_lock (&mutex);
  while (next != 0)
    g_cond_wait (&cond, &mutex);
  next = value;
  if (g_test_verbose ())
    g_printerr ("Thread %p producing next value: %d\n", xthread_self (), value);
  if (value % 10 == 0)
    g_cond_broadcast (&cond);
  else
    g_cond_signal (&cond);
  g_mutex_unlock (&mutex);
}

static xint_t
pop_value (void)
{
  xint_t value;

  g_mutex_lock (&mutex);
  while (next == 0)
    {
      if (g_test_verbose ())
        g_printerr ("Thread %p waiting for cond\n", xthread_self ());
      g_cond_wait (&cond, &mutex);
    }
  value = next;
  next = 0;
  g_cond_broadcast (&cond);
  if (g_test_verbose ())
    g_printerr ("Thread %p consuming value %d\n", xthread_self (), value);
  g_mutex_unlock (&mutex);

  return value;
}

static xpointer_t
produce_values (xpointer_t data)
{
  xint_t total;
  xint_t i;

  total = 0;

  for (i = 1; i < 100; i++)
    {
      total += i;
      push_value (i);
    }

  push_value (-1);
  push_value (-1);

  if (g_test_verbose ())
    g_printerr ("Thread %p produced %d altogether\n", xthread_self (), total);

  return GINT_TO_POINTER (total);
}

static xpointer_t
consume_values (xpointer_t data)
{
  xint_t accum = 0;
  xint_t value;

  while (TRUE)
    {
      value = pop_value ();
      if (value == -1)
        break;

      accum += value;
    }

  if (g_test_verbose ())
    g_printerr ("Thread %p accumulated %d\n", xthread_self (), accum);

  return GINT_TO_POINTER (accum);
}

static xthread_t *producer, *consumer1, *consumer2;

static void
test_cond1 (void)
{
  xint_t total, acc1, acc2;

  producer = xthread_create (produce_values, NULL, TRUE, NULL);
  consumer1 = xthread_create (consume_values, NULL, TRUE, NULL);
  consumer2 = xthread_create (consume_values, NULL, TRUE, NULL);

  total = GPOINTER_TO_INT (xthread_join (producer));
  acc1 = GPOINTER_TO_INT (xthread_join (consumer1));
  acc2 = GPOINTER_TO_INT (xthread_join (consumer2));

  g_assert_cmpint (total, ==, acc1 + acc2);
}

typedef struct
{
  xmutex_t mutex;
  xcond_t  cond;
  xint_t   limit;
  xint_t   count;
} Barrier;

static void
barrier_init (Barrier *barrier,
              xint_t     limit)
{
  g_mutex_init (&barrier->mutex);
  g_cond_init (&barrier->cond);
  barrier->limit = limit;
  barrier->count = limit;
}

static xint_t
barrier_wait (Barrier *barrier)
{
  xint_t ret;

  g_mutex_lock (&barrier->mutex);
  barrier->count--;
  if (barrier->count == 0)
    {
      ret = -1;
      barrier->count = barrier->limit;
      g_cond_broadcast (&barrier->cond);
    }
  else
    {
      ret = 0;
      while (barrier->count != barrier->limit)
        g_cond_wait (&barrier->cond, &barrier->mutex);
    }
  g_mutex_unlock (&barrier->mutex);

  return ret;
}

static void
barrier_clear (Barrier *barrier)
{
  g_mutex_clear (&barrier->mutex);
  g_cond_clear (&barrier->cond);
}

static Barrier b;
static xint_t check;

static xpointer_t
cond2_func (xpointer_t data)
{
  xint_t value = GPOINTER_TO_INT (data);
  xint_t ret;

  g_atomic_int_inc (&check);

  if (g_test_verbose ())
    g_printerr ("thread %d starting, check %d\n", value, g_atomic_int_get (&check));

  g_usleep (10000 * value);

  g_atomic_int_inc (&check);

  if (g_test_verbose ())
    g_printerr ("thread %d reaching barrier, check %d\n", value, g_atomic_int_get (&check));

  ret = barrier_wait (&b);

  g_assert_cmpint (g_atomic_int_get (&check), ==, 10);

  if (g_test_verbose ())
    g_printerr ("thread %d leaving barrier (%d), check %d\n", value, ret, g_atomic_int_get (&check));

  return NULL;
}

/* this test demonstrates how to use a condition variable
 * to implement a barrier
 */
static void
test_cond2 (void)
{
  xint_t i;
  xthread_t *threads[5];

  g_atomic_int_set (&check, 0);

  barrier_init (&b, 5);
  for (i = 0; i < 5; i++)
    threads[i] = xthread_create (cond2_func, GINT_TO_POINTER (i), TRUE, NULL);

  for (i = 0; i < 5; i++)
    xthread_join (threads[i]);

  g_assert_cmpint (g_atomic_int_get (&check), ==, 10);

  barrier_clear (&b);
}

static void
test_wait_until (void)
{
  sint64_t until;
  xmutex_t lock;
  xcond_t cond;

  /* This test will make sure we don't wait too much or too little.
   *
   * We check the 'too long' with a timeout of 60 seconds.
   *
   * We check the 'too short' by verifying a guarantee of the API: we
   * should not wake up until the specified time has passed.
   */
  g_mutex_init (&lock);
  g_cond_init (&cond);

  until = g_get_monotonic_time () + G_TIME_SPAN_SECOND;

  /* Could still have spurious wakeups, so we must loop... */
  g_mutex_lock (&lock);
  while (g_cond_wait_until (&cond, &lock, until))
    ;
  g_mutex_unlock (&lock);

  /* Make sure it's after the until time */
  g_assert_cmpint (until, <=, g_get_monotonic_time ());

  /* Make sure it returns FALSE on timeout */
  until = g_get_monotonic_time () + G_TIME_SPAN_SECOND / 50;
  g_mutex_lock (&lock);
  xassert (g_cond_wait_until (&cond, &lock, until) == FALSE);
  g_mutex_unlock (&lock);

  g_mutex_clear (&lock);
  g_cond_clear (&cond);
}

#ifdef __linux__

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

static pthread_t main_thread;

static void *
mutex_holder (void *data)
{
  xmutex_t *lock = data;

  g_mutex_lock (lock);

  /* Let the lock become contended */
  g_usleep (G_TIME_SPAN_SECOND);

  /* Interrupt the wait on the other thread */
  pthread_kill (main_thread, SIGHUP);

  /* If we don't sleep here, then the g_mutex_unlock() below will clear
   * the mutex, causing the interrupted futex call in the other thread
   * to return success (which is not what we want).
   *
   * The other thread needs to have time to wake up and see that the
   * lock is still contended.
   */
  g_usleep (G_TIME_SPAN_SECOND / 10);

  g_mutex_unlock (lock);

  return NULL;
}

static void
signal_handler (int sig)
{
}

static void
test_wait_until_errno (void)
{
  xboolean_t result;
  xmutex_t lock;
  xcond_t cond;
  struct sigaction act = { };

  /* important: no SA_RESTART (we want EINTR) */
  act.sa_handler = signal_handler;

  g_test_summary ("Check proper handling of errno in g_cond_wait_until with a contended mutex");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/merge_requests/957");

  g_mutex_init (&lock);
  g_cond_init (&cond);

  main_thread = pthread_self ();
  sigaction (SIGHUP, &act, NULL);

  g_mutex_lock (&lock);

  /* We create an annoying worker thread that will do two things:
   *
   *   1) hold the lock that we want to reacquire after returning from
   *      the condition variable wait
   *
   *   2) send us a signal to cause our wait on the contended lock to
   *      return EINTR, clobbering the errno return from the condition
   *      variable
   */
  xthread_unref (xthread_new ("mutex-holder", mutex_holder, &lock));

  result = g_cond_wait_until (&cond, &lock,
                              g_get_monotonic_time () + G_TIME_SPAN_SECOND / 50);

  /* Even after all that disruption, we should still successfully return
   * 'timed out'.
   */
  g_assert_false (result);

  g_mutex_unlock (&lock);

  g_cond_clear (&cond);
  g_mutex_clear (&lock);
}

#else
static void
test_wait_until_errno (void)
{
  g_test_skip ("We only test this on Linux");
}
#endif

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/thread/cond1", test_cond1);
  g_test_add_func ("/thread/cond2", test_cond2);
  g_test_add_func ("/thread/cond/wait-until", test_wait_until);
  g_test_add_func ("/thread/cond/wait-until/contended-and-interrupted", test_wait_until_errno);

  return g_test_run ();
}
