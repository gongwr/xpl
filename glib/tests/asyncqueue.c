/* Unit tests for xasync_queue_t
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

static xint_t
compare_func (xconstpointer d1, xconstpointer d2, xpointer_t data)
{
  xint_t i1, i2;

  i1 = GPOINTER_TO_INT (d1);
  i2 = GPOINTER_TO_INT (d2);

  return i1 - i2;
}

static
void test_async_queue_sort (void)
{
  xasync_queue_t *q;

  q = g_async_queue_new ();

  g_async_queue_push (q, GINT_TO_POINTER (10));
  g_async_queue_push (q, GINT_TO_POINTER (2));
  g_async_queue_push (q, GINT_TO_POINTER (7));

  g_async_queue_sort (q, compare_func, NULL);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_sorted (NULL, GINT_TO_POINTER (1),
                                 compare_func, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_sorted_unlocked (NULL, GINT_TO_POINTER (1),
                                          compare_func, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_sort (NULL, compare_func, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_sort (q, NULL, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_sort_unlocked (NULL, compare_func, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_sort_unlocked (q, NULL, NULL);
      g_test_assert_expected_messages ();
    }

  g_async_queue_push_sorted (q, GINT_TO_POINTER (1), compare_func, NULL);
  g_async_queue_push_sorted (q, GINT_TO_POINTER (8), compare_func, NULL);

  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 1);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 2);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 7);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 8);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 10);

  g_assert_null (g_async_queue_try_pop (q));

  g_async_queue_unref (q);
}

static xint_t destroy_count;

static void
destroy_notify (xpointer_t item)
{
  destroy_count++;
}

static void
test_async_queue_destroy (void)
{
  xasync_queue_t *q;

  destroy_count = 0;

  q = g_async_queue_new_full (destroy_notify);

  g_assert_cmpint (destroy_count, ==, 0);

  g_async_queue_push (q, GINT_TO_POINTER (1));
  g_async_queue_push (q, GINT_TO_POINTER (1));
  g_async_queue_push (q, GINT_TO_POINTER (1));
  g_async_queue_push (q, GINT_TO_POINTER (1));

  g_assert_cmpint (g_async_queue_length (q), ==, 4);

  g_async_queue_unref (q);

  g_assert_cmpint (destroy_count, ==, 4);
}

static xasync_queue_t *q;

static xthread_t *threads[10];
static xint_t counts[10];
static xint_t sums[10];
static xint_t total;

static xpointer_t
thread_func (xpointer_t data)
{
  xint_t pos = GPOINTER_TO_INT (data);
  xint_t value;

  while (1)
    {
      value = GPOINTER_TO_INT (g_async_queue_pop (q));

      if (value == -1)
        break;

      counts[pos]++;
      sums[pos] += value;

      g_usleep (1000);
    }

  return NULL;
}

static void
test_async_queue_threads (void)
{
  xint_t i, j;
  xint_t s, c;
  xint_t value;

  q = g_async_queue_new ();

  for (i = 0; i < 10; i++)
    threads[i] = xthread_new ("test", thread_func, GINT_TO_POINTER (i));

  for (i = 0; i < 100; i++)
    {
      g_async_queue_lock (q);
      for (j = 0; j < 10; j++)
        {
          value = g_random_int_range (1, 100);
          total += value;
          g_async_queue_push_unlocked (q, GINT_TO_POINTER (value));
        }
      g_async_queue_unlock (q);

      g_usleep (1000);
    }

  for (i = 0; i < 10; i++)
    g_async_queue_push (q, GINT_TO_POINTER(-1));

  for (i = 0; i < 10; i++)
    xthread_join (threads[i]);

  g_assert_cmpint (g_async_queue_length (q), ==, 0);

  s = c = 0;

  for (i = 0; i < 10; i++)
    {
      g_assert_cmpint (sums[i], >, 0);
      g_assert_cmpint (counts[i], >, 0);
      s += sums[i];
      c += counts[i];
    }

  g_assert_cmpint (s, ==, total);
  g_assert_cmpint (c, ==, 1000);

  g_async_queue_unref (q);
}

static void
test_async_queue_timed (void)
{
  xasync_queue_t *q;
  GTimeVal tv;
  sint64_t start, end, diff;
  xpointer_t val;

  g_get_current_time (&tv);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_timed_pop (NULL, &tv);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_timed_pop_unlocked (NULL, &tv);
      g_test_assert_expected_messages ();
    }

  q = g_async_queue_new ();

  start = g_get_monotonic_time ();
  g_assert_null (g_async_queue_timeout_pop (q, G_USEC_PER_SEC / 10));

  end = g_get_monotonic_time ();
  diff = end - start;
  g_assert_cmpint (diff, >=, G_USEC_PER_SEC / 10);
  /* diff should be only a little bit more than G_USEC_PER_SEC/10, but
   * we have to leave some wiggle room for heavily-loaded machines...
   */
  g_assert_cmpint (diff, <, 2 * G_USEC_PER_SEC);

  g_async_queue_push (q, GINT_TO_POINTER (10));
  val = g_async_queue_timed_pop (q, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (val), ==, 10);
  g_assert_null (g_async_queue_try_pop (q));

  start = end;
  g_get_current_time (&tv);
  g_time_val_add (&tv, G_USEC_PER_SEC / 10);
  g_assert_null (g_async_queue_timed_pop (q, &tv));

  end = g_get_monotonic_time ();
  diff = end - start;
  g_assert_cmpint (diff, >=, G_USEC_PER_SEC / 10);
  g_assert_cmpint (diff, <, 2 * G_USEC_PER_SEC);

  g_async_queue_push (q, GINT_TO_POINTER (10));
  val = g_async_queue_timed_pop_unlocked (q, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (val), ==, 10);
  g_assert_null (g_async_queue_try_pop (q));

  start = end;
  g_get_current_time (&tv);
  g_time_val_add (&tv, G_USEC_PER_SEC / 10);
  g_async_queue_lock (q);
  g_assert_null (g_async_queue_timed_pop_unlocked (q, &tv));
  g_async_queue_unlock (q);

  end = g_get_monotonic_time ();
  diff = end - start;
  g_assert_cmpint (diff, >=, G_USEC_PER_SEC / 10);
  g_assert_cmpint (diff, <, 2 * G_USEC_PER_SEC);

  g_async_queue_unref (q);
}

static void
test_async_queue_remove (void)
{
  xasync_queue_t *q;

  q = g_async_queue_new ();

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_remove (NULL, GINT_TO_POINTER (1));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_remove (q, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_remove_unlocked (NULL, GINT_TO_POINTER (1));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_remove_unlocked (q, NULL);
      g_test_assert_expected_messages ();
    }

  g_async_queue_push (q, GINT_TO_POINTER (10));
  g_async_queue_push (q, GINT_TO_POINTER (2));
  g_async_queue_push (q, GINT_TO_POINTER (7));
  g_async_queue_push (q, GINT_TO_POINTER (1));

  g_async_queue_remove (q, GINT_TO_POINTER (7));

  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 10);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 2);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 1);

  g_assert_null (g_async_queue_try_pop (q));

  g_async_queue_unref (q);
}

static void
test_async_queue_push_front (void)
{
  xasync_queue_t *q;

  q = g_async_queue_new ();

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_front (NULL, GINT_TO_POINTER (1));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_front (q, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_front_unlocked (NULL, GINT_TO_POINTER (1));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_front_unlocked (q, NULL);
      g_test_assert_expected_messages ();
    }

  g_async_queue_push (q, GINT_TO_POINTER (10));
  g_async_queue_push (q, GINT_TO_POINTER (2));
  g_async_queue_push (q, GINT_TO_POINTER (7));

  g_async_queue_push_front (q, GINT_TO_POINTER (1));

  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 1);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 10);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 2);
  g_assert_cmpint (GPOINTER_TO_INT (g_async_queue_pop (q)), ==, 7);

  g_assert_null (g_async_queue_try_pop (q));

  g_async_queue_unref (q);
}

static void
test_basics (void)
{
  xasync_queue_t *q;
  xpointer_t item;

  destroy_count = 0;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_length (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_length_unlocked (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_ref (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_ref_unlocked (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_unref (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_unref_and_unlock (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_lock (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_unlock (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_pop (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_pop_unlocked (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_try_pop (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_try_pop_unlocked (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_timeout_pop (NULL, 1);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_timeout_pop_unlocked (NULL, 1);
      g_test_assert_expected_messages ();
    }

  q = g_async_queue_new_full (destroy_notify);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push (NULL, GINT_TO_POINTER (1));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push (q, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_unlocked (NULL, GINT_TO_POINTER (1));
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion* failed*");
      g_async_queue_push_unlocked (q, NULL);
      g_test_assert_expected_messages ();
    }

  g_async_queue_lock (q);
  g_async_queue_ref (q);
  g_async_queue_unlock (q);
  g_async_queue_lock (q);
  g_async_queue_ref_unlocked (q);
  g_async_queue_unref_and_unlock (q);

  item = g_async_queue_try_pop (q);
  g_assert_null (item);

  g_async_queue_lock (q);
  item = g_async_queue_try_pop_unlocked (q);
  g_async_queue_unlock (q);
  g_assert_null (item);

  g_async_queue_push (q, GINT_TO_POINTER (1));
  g_async_queue_push (q, GINT_TO_POINTER (2));
  g_async_queue_push (q, GINT_TO_POINTER (3));
  g_assert_cmpint (destroy_count, ==, 0);

  g_async_queue_unref (q);
  g_assert_cmpint (destroy_count, ==, 0);

  item = g_async_queue_pop (q);
  g_assert_cmpint (GPOINTER_TO_INT (item), ==, 1);
  g_assert_cmpint (destroy_count, ==, 0);

  g_async_queue_unref (q);
  g_assert_cmpint (destroy_count, ==, 2);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/asyncqueue/basics", test_basics);
  g_test_add_func ("/asyncqueue/sort", test_async_queue_sort);
  g_test_add_func ("/asyncqueue/destroy", test_async_queue_destroy);
  g_test_add_func ("/asyncqueue/threads", test_async_queue_threads);
  g_test_add_func ("/asyncqueue/timed", test_async_queue_timed);
  g_test_add_func ("/asyncqueue/remove", test_async_queue_remove);
  g_test_add_func ("/asyncqueue/push_front", test_async_queue_push_front);

  return g_test_run ();
}
