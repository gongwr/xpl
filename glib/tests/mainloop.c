/* Unit tests for xmain_loop_t
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

#include <glib.h>
#include <glib/gstdio.h>
#include "glib-private.h"
#include <stdio.h>
#include <string.h>

static xboolean_t
cb (xpointer_t data)
{
  return FALSE;
}

static xboolean_t
prepare (xsource_t *source, xint_t *time)
{
  return FALSE;
}
static xboolean_t
check (xsource_t *source)
{
  return FALSE;
}
static xboolean_t
dispatch (xsource_t *source, xsource_func_t cb, xpointer_t date)
{
  return FALSE;
}

static xsource_funcs_t global_funcs = {
  prepare,
  check,
  dispatch,
  NULL,
  NULL,
  NULL
};

static void
test_maincontext_basic (void)
{
  xmain_context_t *ctx;
  xsource_t *source;
  xuint_t id;
  xpointer_t data = &global_funcs;

  ctx = xmain_context_new ();

  g_assert_false (xmain_context_pending (ctx));
  g_assert_false (xmain_context_iteration (ctx, FALSE));

  source = xsource_new (&global_funcs, sizeof (xsource_t));
  g_assert_cmpint (xsource_get_priority (source), ==, G_PRIORITY_DEFAULT);
  g_assert_false (xsource_is_destroyed (source));

  g_assert_false (xsource_get_can_recurse (source));
  g_assert_null (xsource_get_name (source));

  xsource_set_can_recurse (source, TRUE);
  xsource_set_static_name (source, "d");

  g_assert_true (xsource_get_can_recurse (source));
  g_assert_cmpstr (xsource_get_name (source), ==, "d");

  xsource_set_static_name (source, "still d");
  g_assert_cmpstr (xsource_get_name (source), ==, "still d");

  g_assert_null (xmain_context_find_source_by_user_data (ctx, NULL));
  g_assert_null (xmain_context_find_source_by_funcs_user_data (ctx, &global_funcs, NULL));

  id = xsource_attach (source, ctx);
  g_assert_cmpint (xsource_get_id (source), ==, id);
  g_assert_true (xmain_context_find_source_by_id (ctx, id) == source);

  xsource_set_priority (source, G_PRIORITY_HIGH);
  g_assert_cmpint (xsource_get_priority (source), ==, G_PRIORITY_HIGH);

  xsource_destroy (source);
  g_assert_true (xsource_get_context (source) == ctx);
  g_assert_null (xmain_context_find_source_by_id (ctx, id));

  xmain_context_unref (ctx);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*source->context != NULL*failed*");
      g_assert_null (xsource_get_context (source));
      g_test_assert_expected_messages ();
    }

  xsource_unref (source);

  ctx = xmain_context_default ();
  source = xsource_new (&global_funcs, sizeof (xsource_t));
  xsource_set_funcs (source, &global_funcs);
  xsource_set_callback (source, cb, data, NULL);
  id = xsource_attach (source, ctx);
  xsource_unref (source);
  xsource_set_name_by_id (id, "e");
  g_assert_cmpstr (xsource_get_name (source), ==, "e");
  g_assert_true (xsource_get_context (source) == ctx);
  g_assert_true (xsource_remove_by_funcs_user_data (&global_funcs, data));

  source = xsource_new (&global_funcs, sizeof (xsource_t));
  xsource_set_funcs (source, &global_funcs);
  xsource_set_callback (source, cb, data, NULL);
  id = xsource_attach (source, ctx);
  g_assert_cmpint (id, >, 0);
  xsource_unref (source);
  g_assert_true (xsource_remove_by_user_data (data));
  g_assert_false (xsource_remove_by_user_data ((xpointer_t)0x1234));

  g_idle_add (cb, data);
  g_assert_true (g_idle_remove_by_data (data));
}

static void
test_mainloop_basic (void)
{
  xmain_loop_t *loop;
  xmain_context_t *ctx;

  loop = xmain_loop_new (NULL, FALSE);

  g_assert_false (xmain_loop_is_running (loop));

  xmain_loop_ref (loop);

  ctx = xmain_loop_get_context (loop);
  g_assert_true (ctx == xmain_context_default ());

  xmain_loop_unref (loop);

  g_assert_cmpint (g_main_depth (), ==, 0);

  xmain_loop_unref (loop);
}

static void
test_ownerless_polling (xconstpointer test_data)
{
  xboolean_t attach_first = GPOINTER_TO_INT (test_data);
  xmain_context_t *ctx = xmain_context_new_with_flags (
    XMAIN_CONTEXT_FLAGS_OWNERLESS_POLLING);

  xpollfd_t fds[20];
  xint_t fds_size;
  xint_t max_priority;
  xsource_t *source = NULL;

  g_assert_true (ctx != xmain_context_default ());

  xmain_context_push_thread_default (ctx);

  /* Drain events */
  for (;;)
    {
      xboolean_t ready_to_dispatch = xmain_context_prepare (ctx, &max_priority);
      xint_t timeout, nready;
      fds_size = xmain_context_query (ctx, max_priority, &timeout, fds, G_N_ELEMENTS (fds));
      nready = g_poll (fds, fds_size, /*timeout=*/0);
      if (!ready_to_dispatch && nready == 0)
        {
          if (timeout == -1)
            break;
          else
            g_usleep (timeout * 1000);
        }
      ready_to_dispatch = xmain_context_check (ctx, max_priority, fds, fds_size);
      if (ready_to_dispatch)
        xmain_context_dispatch (ctx);
    }

  if (!attach_first)
    xmain_context_pop_thread_default (ctx);

  source = g_idle_source_new ();
  xsource_attach (source, ctx);
  xsource_unref (source);

  if (attach_first)
    xmain_context_pop_thread_default (ctx);

  g_assert_cmpint (g_poll (fds, fds_size, 0), >, 0);

  xmain_context_unref (ctx);
}

static xint_t global_a;
static xint_t global_b;
static xint_t global_c;

static xboolean_t
count_calls (xpointer_t data)
{
  xint_t *i = data;

  (*i)++;

  return TRUE;
}

static void
test_timeouts (void)
{
  xmain_context_t *ctx;
  xmain_loop_t *loop;
  xsource_t *source;

  if (!g_test_thorough ())
    {
      g_test_skip ("Not running timing heavy test");
      return;
    }

  global_a = global_b = global_c = 0;

  ctx = xmain_context_new ();
  loop = xmain_loop_new (ctx, FALSE);

  source = g_timeout_source_new (100);
  xsource_set_callback (source, count_calls, &global_a, NULL);
  xsource_attach (source, ctx);
  xsource_unref (source);

  source = g_timeout_source_new (250);
  xsource_set_callback (source, count_calls, &global_b, NULL);
  xsource_attach (source, ctx);
  xsource_unref (source);

  source = g_timeout_source_new (330);
  xsource_set_callback (source, count_calls, &global_c, NULL);
  xsource_attach (source, ctx);
  xsource_unref (source);

  source = g_timeout_source_new (1050);
  xsource_set_callback (source, (xsource_func_t)xmain_loop_quit, loop, NULL);
  xsource_attach (source, ctx);
  xsource_unref (source);

  xmain_loop_run (loop);

  /* We may be delayed for an arbitrary amount of time - for example,
   * it's possible for all timeouts to fire exactly once.
   */
  g_assert_cmpint (global_a, >, 0);
  g_assert_cmpint (global_a, >=, global_b);
  g_assert_cmpint (global_b, >=, global_c);

  g_assert_cmpint (global_a, <=, 10);
  g_assert_cmpint (global_b, <=, 4);
  g_assert_cmpint (global_c, <=, 3);

  xmain_loop_unref (loop);
  xmain_context_unref (ctx);
}

static void
test_priorities (void)
{
  xmain_context_t *ctx;
  xsource_t *sourcea;
  xsource_t *sourceb;

  global_a = global_b = global_c = 0;

  ctx = xmain_context_new ();

  sourcea = g_idle_source_new ();
  xsource_set_callback (sourcea, count_calls, &global_a, NULL);
  xsource_set_priority (sourcea, 1);
  xsource_attach (sourcea, ctx);
  xsource_unref (sourcea);

  sourceb = g_idle_source_new ();
  xsource_set_callback (sourceb, count_calls, &global_b, NULL);
  xsource_set_priority (sourceb, 0);
  xsource_attach (sourceb, ctx);
  xsource_unref (sourceb);

  g_assert_true (xmain_context_pending (ctx));
  g_assert_true (xmain_context_iteration (ctx, FALSE));
  g_assert_cmpint (global_a, ==, 0);
  g_assert_cmpint (global_b, ==, 1);

  g_assert_true (xmain_context_iteration (ctx, FALSE));
  g_assert_cmpint (global_a, ==, 0);
  g_assert_cmpint (global_b, ==, 2);

  xsource_destroy (sourceb);

  g_assert_true (xmain_context_iteration (ctx, FALSE));
  g_assert_cmpint (global_a, ==, 1);
  g_assert_cmpint (global_b, ==, 2);

  g_assert_true (xmain_context_pending (ctx));
  xsource_destroy (sourcea);
  g_assert_false (xmain_context_pending (ctx));

  xmain_context_unref (ctx);
}

static xboolean_t
quit_loop (xpointer_t data)
{
  xmain_loop_t *loop = data;

  xmain_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static xint_t count;

static xboolean_t
func (xpointer_t data)
{
  if (data != NULL)
    g_assert_true (data == xthread_self ());

  count++;

  return FALSE;
}

static xboolean_t
call_func (xpointer_t data)
{
  func (xthread_self ());

  return G_SOURCE_REMOVE;
}

static xmutex_t mutex;
static xcond_t cond;
static xboolean_t thread_ready;

static xpointer_t
thread_func (xpointer_t data)
{
  xmain_context_t *ctx = data;
  xmain_loop_t *loop;
  xsource_t *source;

  xmain_context_push_thread_default (ctx);
  loop = xmain_loop_new (ctx, FALSE);

  g_mutex_lock (&mutex);
  thread_ready = TRUE;
  g_cond_signal (&cond);
  g_mutex_unlock (&mutex);

  source = g_timeout_source_new (500);
  xsource_set_callback (source, quit_loop, loop, NULL);
  xsource_attach (source, ctx);
  xsource_unref (source);

  xmain_loop_run (loop);

  xmain_context_pop_thread_default (ctx);
  xmain_loop_unref (loop);

  return NULL;
}

static void
test_invoke (void)
{
  xmain_context_t *ctx;
  xthread_t *thread;

  count = 0;

  /* this one gets invoked directly */
  xmain_context_invoke (NULL, func, xthread_self ());
  g_assert_cmpint (count, ==, 1);

  /* invoking out of an idle works too */
  g_idle_add (call_func, NULL);
  xmain_context_iteration (xmain_context_default (), FALSE);
  g_assert_cmpint (count, ==, 2);

  /* test thread-default forcing the invocation to go
   * to another thread
   */
  ctx = xmain_context_new ();
  thread = xthread_new ("worker", thread_func, ctx);

  g_mutex_lock (&mutex);
  while (!thread_ready)
    g_cond_wait (&cond, &mutex);
  g_mutex_unlock (&mutex);

  xmain_context_invoke (ctx, func, thread);

  xthread_join (thread);
  g_assert_cmpint (count, ==, 3);

  xmain_context_unref (ctx);
}

/* We can't use timeout sources here because on slow or heavily-loaded
 * machines, the test program might not get enough cycles to hit the
 * timeouts at the expected times. So instead we define a source that
 * is based on the number of xmain_context_t iterations.
 */

static xint_t counter;
static gint64 last_counter_update;

typedef struct {
  xsource_t source;
  xint_t    interval;
  xint_t    timeout;
} CounterSource;

static xboolean_t
counter_source_prepare (xsource_t *source,
                        xint_t    *timeout)
{
  CounterSource *csource = (CounterSource *)source;
  gint64 now;

  now = xsource_get_time (source);
  if (now != last_counter_update)
    {
      last_counter_update = now;
      counter++;
    }

  *timeout = 1;
  return counter >= csource->timeout;
}

static xboolean_t
counter_source_dispatch (xsource_t    *source,
                         xsource_func_t callback,
                         xpointer_t    user_data)
{
  CounterSource *csource = (CounterSource *) source;
  xboolean_t again;

  again = callback (user_data);

  if (again)
    csource->timeout = counter + csource->interval;

  return again;
}

static xsource_funcs_t counter_source_funcs = {
  counter_source_prepare,
  NULL,
  counter_source_dispatch,
  NULL,
  NULL,
  NULL
};

static xsource_t *
counter_source_new (xint_t interval)
{
  xsource_t *source = xsource_new (&counter_source_funcs, sizeof (CounterSource));
  CounterSource *csource = (CounterSource *) source;

  csource->interval = interval;
  csource->timeout = counter + interval;

  return source;
}


static xboolean_t
run_inner_loop (xpointer_t user_data)
{
  xmain_context_t *ctx = user_data;
  xmain_loop_t *inner;
  xsource_t *timeout;

  global_a++;

  inner = xmain_loop_new (ctx, FALSE);
  timeout = counter_source_new (100);
  xsource_set_callback (timeout, quit_loop, inner, NULL);
  xsource_attach (timeout, ctx);
  xsource_unref (timeout);

  xmain_loop_run (inner);
  xmain_loop_unref (inner);

  return G_SOURCE_CONTINUE;
}

static void
test_child_sources (void)
{
  xmain_context_t *ctx;
  xmain_loop_t *loop;
  xsource_t *parent, *child_b, *child_c, *end;

  ctx = xmain_context_new ();
  loop = xmain_loop_new (ctx, FALSE);

  global_a = global_b = global_c = 0;

  parent = counter_source_new (2000);
  xsource_set_callback (parent, run_inner_loop, ctx, NULL);
  xsource_set_priority (parent, G_PRIORITY_LOW);
  xsource_attach (parent, ctx);

  child_b = counter_source_new (250);
  xsource_set_callback (child_b, count_calls, &global_b, NULL);
  xsource_add_child_source (parent, child_b);

  child_c = counter_source_new (330);
  xsource_set_callback (child_c, count_calls, &global_c, NULL);
  xsource_set_priority (child_c, G_PRIORITY_HIGH);
  xsource_add_child_source (parent, child_c);

  /* Child sources always have the priority of the parent */
  g_assert_cmpint (xsource_get_priority (parent), ==, G_PRIORITY_LOW);
  g_assert_cmpint (xsource_get_priority (child_b), ==, G_PRIORITY_LOW);
  g_assert_cmpint (xsource_get_priority (child_c), ==, G_PRIORITY_LOW);
  xsource_set_priority (parent, G_PRIORITY_DEFAULT);
  g_assert_cmpint (xsource_get_priority (parent), ==, G_PRIORITY_DEFAULT);
  g_assert_cmpint (xsource_get_priority (child_b), ==, G_PRIORITY_DEFAULT);
  g_assert_cmpint (xsource_get_priority (child_c), ==, G_PRIORITY_DEFAULT);

  end = counter_source_new (1050);
  xsource_set_callback (end, quit_loop, loop, NULL);
  xsource_attach (end, ctx);
  xsource_unref (end);

  xmain_loop_run (loop);

  /* The parent source's own timeout will never trigger, so "a" will
   * only get incremented when "b" or "c" does. And when timeouts get
   * blocked, they still wait the full interval next time rather than
   * "catching up". So the timing is:
   *
   *  250 - b++ -> a++, run_inner_loop
   *  330 - (c is blocked)
   *  350 - inner_loop ends
   *  350 - c++ belatedly -> a++, run_inner_loop
   *  450 - inner loop ends
   *  500 - b++ -> a++, run_inner_loop
   *  600 - inner_loop ends
   *  680 - c++ -> a++, run_inner_loop
   *  750 - (b is blocked)
   *  780 - inner loop ends
   *  780 - b++ belatedly -> a++, run_inner_loop
   *  880 - inner loop ends
   * 1010 - c++ -> a++, run_inner_loop
   * 1030 - (b is blocked)
   * 1050 - end runs, quits outer loop, which has no effect yet
   * 1110 - inner loop ends, a returns, outer loop exits
   */

  g_assert_cmpint (global_a, ==, 6);
  g_assert_cmpint (global_b, ==, 3);
  g_assert_cmpint (global_c, ==, 3);

  xsource_destroy (parent);
  xsource_unref (parent);
  xsource_unref (child_b);
  xsource_unref (child_c);

  xmain_loop_unref (loop);
  xmain_context_unref (ctx);
}

static void
test_recursive_child_sources (void)
{
  xmain_context_t *ctx;
  xmain_loop_t *loop;
  xsource_t *parent, *child_b, *child_c, *end;

  ctx = xmain_context_new ();
  loop = xmain_loop_new (ctx, FALSE);

  global_a = global_b = global_c = 0;

  parent = counter_source_new (500);
  xsource_set_callback (parent, count_calls, &global_a, NULL);

  child_b = counter_source_new (220);
  xsource_set_callback (child_b, count_calls, &global_b, NULL);
  xsource_add_child_source (parent, child_b);

  child_c = counter_source_new (430);
  xsource_set_callback (child_c, count_calls, &global_c, NULL);
  xsource_add_child_source (child_b, child_c);

  xsource_attach (parent, ctx);

  end = counter_source_new (2010);
  xsource_set_callback (end, (xsource_func_t)xmain_loop_quit, loop, NULL);
  xsource_attach (end, ctx);
  xsource_unref (end);

  xmain_loop_run (loop);

  /* Sequence of events:
   *  220 b (b -> 440, a -> 720)
   *  430 c (c -> 860, b -> 650, a -> 930)
   *  650 b (b -> 870, a -> 1150)
   *  860 c (c -> 1290, b -> 1080, a -> 1360)
   * 1080 b (b -> 1300, a -> 1580)
   * 1290 c (c -> 1720, b -> 1510, a -> 1790)
   * 1510 b (b -> 1730, a -> 2010)
   * 1720 c (c -> 2150, b -> 1940, a -> 2220)
   * 1940 b (b -> 2160, a -> 2440)
   */

  g_assert_cmpint (global_a, ==, 9);
  g_assert_cmpint (global_b, ==, 9);
  g_assert_cmpint (global_c, ==, 4);

  xsource_destroy (parent);
  xsource_unref (parent);
  xsource_unref (child_b);
  xsource_unref (child_c);

  xmain_loop_unref (loop);
  xmain_context_unref (ctx);
}

typedef struct {
  xsource_t *parent, *old_child, *new_child;
  xmain_loop_t *loop;
} SwappingTestData;

static xboolean_t
swap_sources (xpointer_t user_data)
{
  SwappingTestData *data = user_data;

  if (data->old_child)
    {
      xsource_remove_child_source (data->parent, data->old_child);
      g_clear_pointer (&data->old_child, xsource_unref);
    }

  if (!data->new_child)
    {
      data->new_child = g_timeout_source_new (0);
      xsource_set_callback (data->new_child, quit_loop, data->loop, NULL);
      xsource_add_child_source (data->parent, data->new_child);
    }

  return G_SOURCE_CONTINUE;
}

static xboolean_t
assert_not_reached_callback (xpointer_t user_data)
{
  g_assert_not_reached ();

  return G_SOURCE_REMOVE;
}

static void
test_swapping_child_sources (void)
{
  xmain_context_t *ctx;
  xmain_loop_t *loop;
  SwappingTestData data;

  ctx = xmain_context_new ();
  loop = xmain_loop_new (ctx, FALSE);

  data.parent = counter_source_new (50);
  data.loop = loop;
  xsource_set_callback (data.parent, swap_sources, &data, NULL);
  xsource_attach (data.parent, ctx);

  data.old_child = counter_source_new (100);
  xsource_add_child_source (data.parent, data.old_child);
  xsource_set_callback (data.old_child, assert_not_reached_callback, NULL, NULL);

  data.new_child = NULL;
  xmain_loop_run (loop);

  xsource_destroy (data.parent);
  xsource_unref (data.parent);
  xsource_unref (data.new_child);

  xmain_loop_unref (loop);
  xmain_context_unref (ctx);
}

static xboolean_t
add_source_callback (xpointer_t user_data)
{
  xmain_loop_t *loop = user_data;
  xsource_t *self = g_main_current_source (), *child;
  xio_channel_t *io;

  /* It doesn't matter whether this is a valid fd or not; it never
   * actually gets polled; the test is just checking that
   * xsource_add_child_source() doesn't crash.
   */
  io = g_io_channel_unix_new (0);
  child = g_io_create_watch (io, G_IO_IN);
  xsource_add_child_source (self, child);
  xsource_unref (child);
  g_io_channel_unref (io);

  xmain_loop_quit (loop);
  return FALSE;
}

static void
test_blocked_child_sources (void)
{
  xmain_context_t *ctx;
  xmain_loop_t *loop;
  xsource_t *source;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=701283");

  ctx = xmain_context_new ();
  loop = xmain_loop_new (ctx, FALSE);

  source = g_idle_source_new ();
  xsource_set_callback (source, add_source_callback, loop, NULL);
  xsource_attach (source, ctx);

  xmain_loop_run (loop);

  xsource_destroy (source);
  xsource_unref (source);

  xmain_loop_unref (loop);
  xmain_context_unref (ctx);
}

typedef struct {
  xmain_context_t *ctx;
  xmain_loop_t *loop;

  xsource_t *timeout1, *timeout2;
  gint64 time1;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GTimeVal tv;  /* needed for xsource_get_current_time() */
G_GNUC_END_IGNORE_DEPRECATIONS
} TimeTestData;

static xboolean_t
timeout1_callback (xpointer_t user_data)
{
  TimeTestData *data = user_data;
  xsource_t *source;
  gint64 mtime1, mtime2, time2;

  source = g_main_current_source ();
  g_assert_true (source == data->timeout1);

  if (data->time1 == -1)
    {
      /* First iteration */
      g_assert_false (xsource_is_destroyed (data->timeout2));

      mtime1 = g_get_monotonic_time ();
      data->time1 = xsource_get_time (source);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      xsource_get_current_time (source, &data->tv);
G_GNUC_END_IGNORE_DEPRECATIONS

      /* xsource_get_time() does not change during a single callback */
      g_usleep (1000000);
      mtime2 = g_get_monotonic_time ();
      time2 = xsource_get_time (source);

      g_assert_cmpint (mtime1, <, mtime2);
      g_assert_cmpint (data->time1, ==, time2);
    }
  else
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      GTimeVal tv;
G_GNUC_END_IGNORE_DEPRECATIONS

      /* Second iteration */
      g_assert_true (xsource_is_destroyed (data->timeout2));

      /* xsource_get_time() MAY change between iterations; in this
       * case we know for sure that it did because of the g_usleep()
       * last time.
       */
      time2 = xsource_get_time (source);
      g_assert_cmpint (data->time1, <, time2);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      xsource_get_current_time (source, &tv);
G_GNUC_END_IGNORE_DEPRECATIONS

      g_assert_true (tv.tv_sec > data->tv.tv_sec ||
                     (tv.tv_sec == data->tv.tv_sec &&
                      tv.tv_usec > data->tv.tv_usec));

      xmain_loop_quit (data->loop);
    }

  return TRUE;
}

static xboolean_t
timeout2_callback (xpointer_t user_data)
{
  TimeTestData *data = user_data;
  xsource_t *source;
  gint64 time2, time3;

  source = g_main_current_source ();
  g_assert_true (source == data->timeout2);

  g_assert_false (xsource_is_destroyed (data->timeout1));

  /* xsource_get_time() does not change between different sources in
   * a single iteration of the mainloop.
   */
  time2 = xsource_get_time (source);
  g_assert_cmpint (data->time1, ==, time2);

  /* The source should still have a valid time even after being
   * destroyed, since it's currently running.
   */
  xsource_destroy (source);
  time3 = xsource_get_time (source);
  g_assert_cmpint (time2, ==, time3);

  return FALSE;
}

static void
test_source_time (void)
{
  TimeTestData data;

  data.ctx = xmain_context_new ();
  data.loop = xmain_loop_new (data.ctx, FALSE);

  data.timeout1 = g_timeout_source_new (0);
  xsource_set_callback (data.timeout1, timeout1_callback, &data, NULL);
  xsource_attach (data.timeout1, data.ctx);

  data.timeout2 = g_timeout_source_new (0);
  xsource_set_callback (data.timeout2, timeout2_callback, &data, NULL);
  xsource_attach (data.timeout2, data.ctx);

  data.time1 = -1;

  xmain_loop_run (data.loop);

  g_assert_false (xsource_is_destroyed (data.timeout1));
  g_assert_true (xsource_is_destroyed (data.timeout2));

  xsource_destroy (data.timeout1);
  xsource_unref (data.timeout1);
  xsource_unref (data.timeout2);

  xmain_loop_unref (data.loop);
  xmain_context_unref (data.ctx);
}

typedef struct {
  xuint_t outstanding_ops;
  xmain_loop_t *loop;
} TestOverflowData;

static xboolean_t
on_source_fired_cb (xpointer_t user_data)
{
  TestOverflowData *data = user_data;
  xsource_t *current_source;
  xmain_context_t *current_context;
  xuint_t source_id;

  data->outstanding_ops--;

  current_source = g_main_current_source ();
  current_context = xsource_get_context (current_source);
  source_id = xsource_get_id (current_source);
  g_assert_nonnull (xmain_context_find_source_by_id (current_context, source_id));
  xsource_destroy (current_source);
  g_assert_null (xmain_context_find_source_by_id (current_context, source_id));

  if (data->outstanding_ops == 0)
    xmain_loop_quit (data->loop);
  return FALSE;
}

static xsource_t *
add_idle_source (xmain_context_t *ctx,
                 TestOverflowData *data)
{
  xsource_t *source;

  source = g_idle_source_new ();
  xsource_set_callback (source, on_source_fired_cb, data, NULL);
  xsource_attach (source, ctx);
  xsource_unref (source);
  data->outstanding_ops++;

  return source;
}

static void
test_mainloop_overflow (void)
{
  xmain_context_t *ctx;
  xmain_loop_t *loop;
  xsource_t *source;
  TestOverflowData data;
  xuint_t i;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=687098");

  memset (&data, 0, sizeof (data));

  ctx = XPL_PRIVATE_CALL (xmain_context_new_with_next_id) (G_MAXUINT-1);

  loop = xmain_loop_new (ctx, TRUE);
  data.outstanding_ops = 0;
  data.loop = loop;

  source = add_idle_source (ctx, &data);
  g_assert_cmpint (source->source_id, ==, G_MAXUINT-1);

  source = add_idle_source (ctx, &data);
  g_assert_cmpint (source->source_id, ==, G_MAXUINT);

  source = add_idle_source (ctx, &data);
  g_assert_cmpint (source->source_id, !=, 0);

  /* Now, a lot more sources */
  for (i = 0; i < 50; i++)
    {
      source = add_idle_source (ctx, &data);
      g_assert_cmpint (source->source_id, !=, 0);
    }

  xmain_loop_run (loop);
  g_assert_cmpint (data.outstanding_ops, ==, 0);

  xmain_loop_unref (loop);
  xmain_context_unref (ctx);
}

static xint_t ready_time_dispatched;  /* (atomic) */

static xboolean_t
ready_time_dispatch (xsource_t     *source,
                     xsource_func_t  callback,
                     xpointer_t     user_data)
{
  g_atomic_int_set (&ready_time_dispatched, TRUE);

  xsource_set_ready_time (source, -1);

  return TRUE;
}

static xpointer_t
run_context (xpointer_t user_data)
{
  xmain_loop_run (user_data);

  return NULL;
}

static void
test_ready_time (void)
{
  xthread_t *thread;
  xsource_t *source;
  xsource_funcs_t source_funcs = {
    NULL, NULL, ready_time_dispatch, NULL, NULL, NULL
  };
  xmain_loop_t *loop;

  source = xsource_new (&source_funcs, sizeof (xsource_t));
  xsource_attach (source, NULL);
  xsource_unref (source);

  /* Unfortunately we can't do too many things with respect to timing
   * without getting into trouble on slow systems or heavily loaded
   * builders.
   *
   * We can test that the basics are working, though.
   */

  /* A source with no ready time set should not fire */
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_false (g_atomic_int_get (&ready_time_dispatched));

  /* The ready time should not have been changed */
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);

  /* Of course this shouldn't change anything either */
  xsource_set_ready_time (source, -1);
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);

  /* A source with a ready time set to tomorrow should not fire on any
   * builder, no matter how badly loaded...
   */
  xsource_set_ready_time (source, g_get_monotonic_time () + G_TIME_SPAN_DAY);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_false (g_atomic_int_get (&ready_time_dispatched));
  /* Make sure it didn't get reset */
  g_assert_cmpint (xsource_get_ready_time (source), !=, -1);

  /* Ready time of -1 -> don't fire */
  xsource_set_ready_time (source, -1);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_false (g_atomic_int_get (&ready_time_dispatched));
  /* Not reset, but should still be -1 from above */
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);

  /* A ready time of the current time should fire immediately */
  xsource_set_ready_time (source, g_get_monotonic_time ());
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  /* Should have gotten reset by the handler function */
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);

  /* As well as one in the recent past... */
  xsource_set_ready_time (source, g_get_monotonic_time () - G_TIME_SPAN_SECOND);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);

  /* Zero is the 'official' way to get a source to fire immediately */
  xsource_set_ready_time (source, 0);
  while (xmain_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_assert_cmpint (xsource_get_ready_time (source), ==, -1);

  /* Now do some tests of cross-thread wakeups.
   *
   * Make sure it wakes up right away from the start.
   */
  xsource_set_ready_time (source, 0);
  loop = xmain_loop_new (NULL, FALSE);
  thread = xthread_new ("context thread", run_context, loop);
  while (!g_atomic_int_get (&ready_time_dispatched));

  /* Now let's see if it can wake up from sleeping. */
  g_usleep (G_TIME_SPAN_SECOND / 2);
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  xsource_set_ready_time (source, 0);
  while (!g_atomic_int_get (&ready_time_dispatched));

  /* kill the thread */
  xmain_loop_quit (loop);
  xthread_join (thread);
  xmain_loop_unref (loop);

  xsource_destroy (source);
}

static void
test_wakeup(void)
{
  xmain_context_t *ctx;
  int i;

  ctx = xmain_context_new ();

  /* run a random large enough number of times because
   * main contexts tend to wake up a few times after creation.
   */
  for (i = 0; i < 100; i++)
    {
      /* This is the invariant we care about:
       * xmain_context_wakeup(ctx,) ensures that the next call to
       * xmain_context_iteration (ctx, TRUE) returns and doesn't
       * block.
       * This is important in threaded apps where we might not know
       * if the thread calls xmain_context_wakeup() before or after
       * we enter xmain_context_iteration().
       */
      xmain_context_wakeup (ctx);
      xmain_context_iteration (ctx, TRUE);
    }

  xmain_context_unref (ctx);
}

static void
test_remove_invalid (void)
{
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "Source ID 3000000000 was not found*");
  xsource_remove (3000000000u);
  g_test_assert_expected_messages ();
}

static xboolean_t
trivial_prepare (xsource_t *source,
                 xint_t    *timeout)
{
  *timeout = 0;
  return TRUE;
}

static xint_t n_finalized;

static void
trivial_finalize (xsource_t *source)
{
  n_finalized++;
}

static void
test_unref_while_pending (void)
{
  static xsource_funcs_t funcs = {
    trivial_prepare, NULL, NULL, trivial_finalize, NULL, NULL
  };
  xmain_context_t *context;
  xsource_t *source;

  context = xmain_context_new ();

  source = xsource_new (&funcs, sizeof (xsource_t));
  xsource_attach (source, context);
  xsource_unref (source);

  /* Do incomplete main iteration -- get a pending source but don't dispatch it. */
  xmain_context_prepare (context, NULL);
  xmain_context_query (context, 0, NULL, NULL, 0);
  xmain_context_check (context, 1000, NULL, 0);

  /* Destroy the context */
  xmain_context_unref (context);

  /* Make sure we didn't leak the source */
  g_assert_cmpint (n_finalized, ==, 1);
}

#ifdef G_OS_UNIX

#include <glib-unix.h>
#include <unistd.h>

static xchar_t zeros[1024];

static xsize_t
fill_a_pipe (xint_t fd)
{
  xsize_t written = 0;
  xpollfd_t pfd;

  pfd.fd = fd;
  pfd.events = G_IO_OUT;
  while (g_poll (&pfd, 1, 0) == 1)
    /* we should never see -1 here */
    written += write (fd, zeros, sizeof zeros);

  return written;
}

static xboolean_t
write_bytes (xint_t         fd,
             xio_condition_t condition,
             xpointer_t     user_data)
{
  xssize_t *to_write = user_data;
  xint_t limit;

  if (*to_write == 0)
    return FALSE;

  /* Detect if we run before we should */
  g_assert_cmpint (*to_write, >=, 0);

  limit = MIN ((xsize_t) *to_write, sizeof zeros);
  *to_write -= write (fd, zeros, limit);

  return TRUE;
}

static xboolean_t
read_bytes (xint_t         fd,
            xio_condition_t condition,
            xpointer_t     user_data)
{
  static xchar_t buffer[1024];
  xssize_t *to_read = user_data;

  *to_read -= read (fd, buffer, sizeof buffer);

  /* The loop will exit when there is nothing else to read, then we will
   * use xsource_remove() to destroy this source.
   */
  return TRUE;
}

#ifdef G_OS_UNIX
static void
test_unix_fd (void)
{
  xssize_t to_write = -1;
  xssize_t to_read;
  xint_t fds[2];
  xint_t a, b;
  xint_t s;
  xsource_t *source_a;
  xsource_t *source_b;

  s = pipe (fds);
  g_assert_cmpint (s, ==, 0);

  to_read = fill_a_pipe (fds[1]);
  /* write at higher priority to keep the pipe full... */
  a = g_unix_fd_add_full (G_PRIORITY_HIGH, fds[1], G_IO_OUT, write_bytes, &to_write, NULL);
  source_a = xsource_ref (xmain_context_find_source_by_id (NULL, a));
  /* make sure no 'writes' get dispatched yet */
  while (xmain_context_iteration (NULL, FALSE));

  to_read += 128 * 1024 * 1024;
  to_write = 128 * 1024 * 1024;
  b = g_unix_fd_add (fds[0], G_IO_IN, read_bytes, &to_read);
  source_b = xsource_ref (xmain_context_find_source_by_id (NULL, b));

  /* Assuming the kernel isn't internally 'laggy' then there will always
   * be either data to read or room in which to write.  That will keep
   * the loop running until all data has been read and written.
   */
  while (TRUE)
    {
      xssize_t to_write_was = to_write;
      xssize_t to_read_was = to_read;

      if (!xmain_context_iteration (NULL, FALSE))
        break;

      /* Since the sources are at different priority, only one of them
       * should possibly have run.
       */
      g_assert_true (to_write == to_write_was || to_read == to_read_was);
    }

  g_assert_cmpint (to_write, ==, 0);
  g_assert_cmpint (to_read, ==, 0);

  /* 'a' is already removed by itself */
  g_assert_true (xsource_is_destroyed (source_a));
  xsource_unref (source_a);
  xsource_remove (b);
  g_assert_true (xsource_is_destroyed (source_b));
  xsource_unref (source_b);
  close (fds[1]);
  close (fds[0]);
}
#endif

static void
assert_main_context_state (xint_t n_to_poll,
                           ...)
{
  xmain_context_t *context;
  xboolean_t consumed[10] = { };
  xpollfd_t poll_fds[10];
  xboolean_t acquired;
  xboolean_t immediate;
  xint_t max_priority;
  xint_t timeout;
  xint_t n;
  xint_t i, j;
  va_list ap;

  context = xmain_context_default ();

  acquired = xmain_context_acquire (context);
  g_assert_true (acquired);

  immediate = xmain_context_prepare (context, &max_priority);
  g_assert_false (immediate);
  n = xmain_context_query (context, max_priority, &timeout, poll_fds, 10);
  g_assert_cmpint (n, ==, n_to_poll + 1); /* one will be the gwakeup */

  va_start (ap, n_to_poll);
  for (i = 0; i < n_to_poll; i++)
    {
      xint_t expected_fd = va_arg (ap, xint_t);
      xio_condition_t expected_events = va_arg (ap, xio_condition_t);
      xio_condition_t report_events = va_arg (ap, xio_condition_t);

      for (j = 0; j < n; j++)
        if (!consumed[j] && poll_fds[j].fd == expected_fd && poll_fds[j].events == expected_events)
          {
            poll_fds[j].revents = report_events;
            consumed[j] = TRUE;
            break;
          }

      if (j == n)
        xerror ("Unable to find fd %d (index %d) with events 0x%x", expected_fd, i, (xuint_t) expected_events);
    }
  va_end (ap);

  /* find the gwakeup, flag as non-ready */
  for (i = 0; i < n; i++)
    if (!consumed[i])
      poll_fds[i].revents = 0;

  if (xmain_context_check (context, max_priority, poll_fds, n))
    xmain_context_dispatch (context);

  xmain_context_release (context);
}

static xboolean_t
flag_bool (xint_t         fd,
           xio_condition_t condition,
           xpointer_t     user_data)
{
  xboolean_t *flag = user_data;

  *flag = TRUE;

  return TRUE;
}

static void
test_unix_fd_source (void)
{
  xsource_t *out_source;
  xsource_t *in_source;
  xsource_t *source;
  xboolean_t out, in;
  xint_t fds[2];
  xint_t s;

  assert_main_context_state (0);

  s = pipe (fds);
  g_assert_cmpint (s, ==, 0);

  source = g_unix_fd_source_new (fds[1], G_IO_OUT);
  xsource_attach (source, NULL);

  /* Check that a source with no callback gets successfully detached
   * with a warning printed.
   */
  g_test_expect_message ("GLib", G_LOG_LEVEL_WARNING, "*GUnixFDSource dispatched without callback*");
  while (xmain_context_iteration (NULL, FALSE));
  g_test_assert_expected_messages ();
  g_assert_true (xsource_is_destroyed (source));
  xsource_unref (source);

  out = in = FALSE;
  out_source = g_unix_fd_source_new (fds[1], G_IO_OUT);
  /* -Wcast-function-type complains about casting 'flag_bool' to xsource_func_t.
   * GCC has no way of knowing that it will be cast back to GUnixFDSourceFunc
   * before being called. Although GLib itself is not compiled with
   * -Wcast-function-type, applications that use GLib may well be (since
   * -Wextra includes it), so we provide a G_SOURCE_FUNC() macro to suppress
   * the warning. We check that it works here.
   */
#if G_GNUC_CHECK_VERSION(8, 0)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wcast-function-type"
#endif
  xsource_set_callback (out_source, G_SOURCE_FUNC (flag_bool), &out, NULL);
#if G_GNUC_CHECK_VERSION(8, 0)
#pragma GCC diagnostic pop
#endif
  xsource_attach (out_source, NULL);
  assert_main_context_state (1,
                             fds[1], G_IO_OUT, 0);
  g_assert_true (!in && !out);

  in_source = g_unix_fd_source_new (fds[0], G_IO_IN);
  xsource_set_callback (in_source, (xsource_func_t) flag_bool, &in, NULL);
  xsource_set_priority (in_source, G_PRIORITY_DEFAULT_IDLE);
  xsource_attach (in_source, NULL);
  assert_main_context_state (2,
                             fds[0], G_IO_IN, G_IO_IN,
                             fds[1], G_IO_OUT, G_IO_OUT);
  /* out is higher priority so only it should fire */
  g_assert_true (!in && out);

  /* raise the priority of the in source to higher than out*/
  in = out = FALSE;
  xsource_set_priority (in_source, G_PRIORITY_HIGH);
  assert_main_context_state (2,
                             fds[0], G_IO_IN, G_IO_IN,
                             fds[1], G_IO_OUT, G_IO_OUT);
  g_assert_true (in && !out);

  /* now, let them be equal */
  in = out = FALSE;
  xsource_set_priority (in_source, G_PRIORITY_DEFAULT);
  assert_main_context_state (2,
                             fds[0], G_IO_IN, G_IO_IN,
                             fds[1], G_IO_OUT, G_IO_OUT);
  g_assert_true (in && out);

  xsource_destroy (out_source);
  xsource_unref (out_source);
  xsource_destroy (in_source);
  xsource_unref (in_source);
  close (fds[1]);
  close (fds[0]);
}

typedef struct
{
  xsource_t parent;
  xboolean_t flagged;
} FlagSource;

static xboolean_t
return_true (xsource_t *source, xsource_func_t callback, xpointer_t user_data)
{
  FlagSource *flaxsource = (FlagSource *) source;

  flaxsource->flagged = TRUE;

  return TRUE;
}

#define assert_flagged(s) g_assert_true (((FlagSource *) (s))->flagged);
#define assert_not_flagged(s) g_assert_true (!((FlagSource *) (s))->flagged);
#define clear_flag(s) ((FlagSource *) (s))->flagged = 0

static void
test_source_unix_fd_api (void)
{
  xsource_funcs_t no_funcs = {
    NULL, NULL, return_true, NULL, NULL, NULL
  };
  xsource_t *source_a;
  xsource_t *source_b;
  xpointer_t tag1, tag2;
  xint_t fds_a[2];
  xint_t fds_b[2];

  pipe (fds_a);
  pipe (fds_b);

  source_a = xsource_new (&no_funcs, sizeof (FlagSource));
  source_b = xsource_new (&no_funcs, sizeof (FlagSource));

  /* attach a source with more than one fd */
  xsource_add_unix_fd (source_a, fds_a[0], G_IO_IN);
  xsource_add_unix_fd (source_a, fds_a[1], G_IO_OUT);
  xsource_attach (source_a, NULL);
  assert_main_context_state (2,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, 0);
  assert_not_flagged (source_a);

  /* attach a higher priority source with no fds */
  xsource_set_priority (source_b, G_PRIORITY_HIGH);
  xsource_attach (source_b, NULL);
  assert_main_context_state (2,
                             fds_a[0], G_IO_IN, G_IO_IN,
                             fds_a[1], G_IO_OUT, 0);
  assert_flagged (source_a);
  assert_not_flagged (source_b);
  clear_flag (source_a);

  /* add some fds to the second source, while attached */
  tag1 = xsource_add_unix_fd (source_b, fds_b[0], G_IO_IN);
  tag2 = xsource_add_unix_fd (source_b, fds_b[1], G_IO_OUT);
  assert_main_context_state (4,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, G_IO_OUT,
                             fds_b[0], G_IO_IN, 0,
                             fds_b[1], G_IO_OUT, G_IO_OUT);
  /* only 'b' (higher priority) should have dispatched */
  assert_not_flagged (source_a);
  assert_flagged (source_b);
  clear_flag (source_b);

  /* change our events on b to the same as they were before */
  xsource_modify_unix_fd (source_b, tag1, G_IO_IN);
  xsource_modify_unix_fd (source_b, tag2, G_IO_OUT);
  assert_main_context_state (4,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, G_IO_OUT,
                             fds_b[0], G_IO_IN, 0,
                             fds_b[1], G_IO_OUT, G_IO_OUT);
  assert_not_flagged (source_a);
  assert_flagged (source_b);
  clear_flag (source_b);

  /* now reverse them */
  xsource_modify_unix_fd (source_b, tag1, G_IO_OUT);
  xsource_modify_unix_fd (source_b, tag2, G_IO_IN);
  assert_main_context_state (4,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, G_IO_OUT,
                             fds_b[0], G_IO_OUT, 0,
                             fds_b[1], G_IO_IN, 0);
  /* 'b' had no events, so 'a' can go this time */
  assert_flagged (source_a);
  assert_not_flagged (source_b);
  clear_flag (source_a);

  /* remove one of the fds from 'b' */
  xsource_remove_unix_fd (source_b, tag1);
  assert_main_context_state (3,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, 0,
                             fds_b[1], G_IO_IN, 0);
  assert_not_flagged (source_a);
  assert_not_flagged (source_b);

  /* remove the other */
  xsource_remove_unix_fd (source_b, tag2);
  assert_main_context_state (2,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, 0);
  assert_not_flagged (source_a);
  assert_not_flagged (source_b);

  /* destroy the sources */
  xsource_destroy (source_a);
  xsource_destroy (source_b);
  assert_main_context_state (0);

  xsource_unref (source_a);
  xsource_unref (source_b);
  close (fds_a[0]);
  close (fds_a[1]);
  close (fds_b[0]);
  close (fds_b[1]);
}

static xboolean_t
unixfd_quit_loop (xint_t         fd,
                  xio_condition_t condition,
                  xpointer_t     user_data)
{
  xmain_loop_t *loop = user_data;

  xmain_loop_quit (loop);

  return FALSE;
}

static void
test_unix_file_poll (void)
{
  xint_t fd;
  xsource_t *source;
  xmain_loop_t *loop;

  fd = open ("/dev/null", O_RDONLY);
  g_assert_cmpint (fd, >=, 0);

  loop = xmain_loop_new (NULL, FALSE);

  source = g_unix_fd_source_new (fd, G_IO_IN);
  xsource_set_callback (source, (xsource_func_t) unixfd_quit_loop, loop, NULL);
  xsource_attach (source, NULL);

  /* Should not block */
  xmain_loop_run (loop);

  xsource_destroy (source);

  assert_main_context_state (0);

  xsource_unref (source);

  xmain_loop_unref (loop);

  close (fd);
}

static void
test_unix_fd_priority (void)
{
  xint_t fd1, fd2;
  xmain_loop_t *loop;
  xsource_t *source;

  xint_t s1 = 0;
  xboolean_t s2 = FALSE, s3 = FALSE;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/1592");

  loop = xmain_loop_new (NULL, FALSE);

  source = g_idle_source_new ();
  xsource_set_callback (source, count_calls, &s1, NULL);
  xsource_set_priority (source, 0);
  xsource_attach (source, NULL);
  xsource_unref (source);

  fd1 = open ("/dev/random", O_RDONLY);
  g_assert_cmpint (fd1, >=, 0);
  source = g_unix_fd_source_new (fd1, G_IO_IN);
  xsource_set_callback (source, G_SOURCE_FUNC (flag_bool), &s2, NULL);
  xsource_set_priority (source, 10);
  xsource_attach (source, NULL);
  xsource_unref (source);

  fd2 = open ("/dev/random", O_RDONLY);
  g_assert_cmpint (fd2, >=, 0);
  source = g_unix_fd_source_new (fd2, G_IO_IN);
  xsource_set_callback (source, G_SOURCE_FUNC (flag_bool), &s3, NULL);
  xsource_set_priority (source, 0);
  xsource_attach (source, NULL);
  xsource_unref (source);

  /* This tests a bug that depends on the source with the lowest FD
     identifier to have the lowest priority. Make sure that this is
     the case. */
  g_assert_cmpint (fd1, <, fd2);

  g_assert_true (xmain_context_iteration (NULL, FALSE));

  /* Idle source should have been dispatched. */
  g_assert_cmpint (s1, ==, 1);
  /* Low priority FD source shouldn't have been dispatched. */
  g_assert_false (s2);
  /* Default priority FD source should have been dispatched. */
  g_assert_true (s3);

  xmain_loop_unref (loop);

  close (fd1);
  close (fd2);
}

#endif

#ifdef G_OS_UNIX
static xboolean_t
timeout_cb (xpointer_t data)
{
  xmain_loop_t *loop = data;
  xmain_context_t *context;

  context = xmain_loop_get_context (loop);
  g_assert_true (xmain_loop_is_running (loop));
  g_assert_true (xmain_context_is_owner (context));

  xmain_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static xpointer_t
threadf (xpointer_t data)
{
  xmain_context_t *context = data;
  xmain_loop_t *loop;
  xsource_t *source;

  loop = xmain_loop_new (context, FALSE);
  source = g_timeout_source_new (250);
  xsource_set_callback (source, timeout_cb, loop, NULL);
  xsource_attach (source, context);

  xmain_loop_run (loop);

  xsource_destroy (source);
  xsource_unref (source);
  xmain_loop_unref (loop);

  return NULL;
}

static void
test_mainloop_wait (void)
{
#ifdef _XPL_ADDRESS_SANITIZER
  (void) threadf;
  g_test_incomplete ("FIXME: Leaks a xmain_loop_t, see glib#2307");
#else
  xmain_context_t *context;
  xthread_t *t1, *t2;

  context = xmain_context_new ();

  t1 = xthread_new ("t1", threadf, context);
  t2 = xthread_new ("t2", threadf, context);

  xthread_join (t1);
  xthread_join (t2);

  xmain_context_unref (context);
#endif
}
#endif

static xboolean_t
nfds_in_cb (xio_channel_t   *io,
            xio_condition_t  condition,
            xpointer_t      user_data)
{
  xboolean_t *in_cb_ran = user_data;

  *in_cb_ran = TRUE;
  g_assert_cmpint (condition, ==, G_IO_IN);
  return FALSE;
}

static xboolean_t
nfds_out_cb (xio_channel_t   *io,
             xio_condition_t  condition,
             xpointer_t      user_data)
{
  xboolean_t *out_cb_ran = user_data;

  *out_cb_ran = TRUE;
  g_assert_cmpint (condition, ==, G_IO_OUT);
  return FALSE;
}

static xboolean_t
nfds_out_low_cb (xio_channel_t   *io,
                 xio_condition_t  condition,
                 xpointer_t      user_data)
{
  g_assert_not_reached ();
  return FALSE;
}

static void
test_nfds (void)
{
  xmain_context_t *ctx;
  xpollfd_t out_fds[3];
  xint_t fd, nfds;
  xio_channel_t *io;
  xsource_t *source1, *source2, *source3;
  xboolean_t source1_ran = FALSE, source3_ran = FALSE;
  xchar_t *tmpfile;
  xerror_t *error = NULL;

  ctx = xmain_context_new ();
  nfds = xmain_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  /* An "empty" xmain_context_t will have a single xpollfd_t, for its
   * internal GWakeup.
   */
  g_assert_cmpint (nfds, ==, 1);

  fd = xfile_open_tmp (NULL, &tmpfile, &error);
  g_assert_no_error (error);

  io = g_io_channel_unix_new (fd);
#ifdef G_OS_WIN32
  /* The fd in the pollfds won't be the same fd we passed in */
  g_io_channel_win32_make_pollfd (io, G_IO_IN, out_fds);
  fd = out_fds[0].fd;
#endif

  /* Add our first pollfd */
  source1 = g_io_create_watch (io, G_IO_IN);
  xsource_set_priority (source1, G_PRIORITY_DEFAULT);
  xsource_set_callback (source1, (xsource_func_t) nfds_in_cb,
                         &source1_ran, NULL);
  xsource_attach (source1, ctx);

  nfds = xmain_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN);
  else
    g_assert_not_reached ();

  /* Add a second pollfd with the same fd but different event, and
   * lower priority.
   */
  source2 = g_io_create_watch (io, G_IO_OUT);
  xsource_set_priority (source2, G_PRIORITY_LOW);
  xsource_set_callback (source2, (xsource_func_t) nfds_out_low_cb,
                         NULL, NULL);
  xsource_attach (source2, ctx);

  /* xmain_context_query() should still return only 2 pollfds,
   * one of which has our fd, and a combined events field.
   */
  nfds = xmain_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN | G_IO_OUT);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN | G_IO_OUT);
  else
    g_assert_not_reached ();

  /* But if we query with a max priority, we won't see the
   * lower-priority one.
   */
  nfds = xmain_context_query (ctx, G_PRIORITY_DEFAULT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN);
  else
    g_assert_not_reached ();

  /* Third pollfd */
  source3 = g_io_create_watch (io, G_IO_OUT);
  xsource_set_priority (source3, G_PRIORITY_DEFAULT);
  xsource_set_callback (source3, (xsource_func_t) nfds_out_cb,
                         &source3_ran, NULL);
  xsource_attach (source3, ctx);

  nfds = xmain_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN | G_IO_OUT);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN | G_IO_OUT);
  else
    g_assert_not_reached ();

  /* Now actually iterate the loop; the fd should be readable and
   * writable, so source1 and source3 should be triggered, but *not*
   * source2, since it's lower priority than them. (Though on
   * G_OS_WIN32, source3 doesn't get triggered, probably because of
   * giowin32 weirdness...)
   */
  xmain_context_iteration (ctx, FALSE);

  g_assert_true (source1_ran);
#ifndef G_OS_WIN32
  g_assert_true (source3_ran);
#endif

  xsource_destroy (source1);
  xsource_unref (source1);
  xsource_destroy (source2);
  xsource_unref (source2);
  xsource_destroy (source3);
  xsource_unref (source3);

  g_io_channel_unref (io);
  remove (tmpfile);
  g_free (tmpfile);

  xmain_context_unref (ctx);
}

static xboolean_t
nsources_cb (xpointer_t user_data)
{
  g_assert_not_reached ();
  return FALSE;
}

static void
shuffle_nsources (xsource_t **sources, int num)
{
  int i, a, b;
  xsource_t *tmp;

  for (i = 0; i < num * 10; i++)
    {
      a = g_random_int_range (0, num);
      b = g_random_int_range (0, num);
      tmp = sources[a];
      sources[a] = sources[b];
      sources[b] = tmp;
    }
}

static void
test_nsources_same_priority (void)
{
  xmain_context_t *context;
  xsource_t **sources;
  gint64 start, end;
  xsize_t n_sources = 50000, i;

  context = xmain_context_default ();
  sources = g_new0 (xsource_t *, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      sources[i] = g_idle_source_new ();
      xsource_set_callback (sources[i], nsources_cb, NULL, NULL);
      xsource_attach (sources[i], context);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Add same-priority sources: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    g_assert_true (sources[i] == xmain_context_find_source_by_id (context, xsource_get_id (sources[i])));
  end = g_get_monotonic_time ();
  g_test_message ("Find each source: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  shuffle_nsources (sources, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      xsource_destroy (sources[i]);
      xsource_unref (sources[i]);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Remove in random order: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  /* Make sure they really did get removed */
  xmain_context_iteration (context, FALSE);

  g_free (sources);
}

static void
test_nsources_different_priority (void)
{
  xmain_context_t *context;
  xsource_t **sources;
  gint64 start, end;
  xsize_t n_sources = 50000, i;

  context = xmain_context_default ();
  sources = g_new0 (xsource_t *, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      sources[i] = g_idle_source_new ();
      xsource_set_callback (sources[i], nsources_cb, NULL, NULL);
      xsource_set_priority (sources[i], i % 100);
      xsource_attach (sources[i], context);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Add different-priority sources: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    g_assert_true (sources[i] == xmain_context_find_source_by_id (context, xsource_get_id (sources[i])));
  end = g_get_monotonic_time ();
  g_test_message ("Find each source: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  shuffle_nsources (sources, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      xsource_destroy (sources[i]);
      xsource_unref (sources[i]);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Remove in random order: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  /* Make sure they really did get removed */
  xmain_context_iteration (context, FALSE);

  g_free (sources);
}

static void
thread_pool_attach_func (xpointer_t data,
                         xpointer_t user_data)
{
  xmain_context_t *context = user_data;
  xsource_t *source = data;

  xsource_attach (source, context);
  xsource_unref (source);
}

static void
thread_pool_destroy_func (xpointer_t data,
                          xpointer_t user_data)
{
  xsource_t *source = data;

  xsource_destroy (source);
}

static void
test_nsources_threadpool (void)
{
  xmain_context_t *context;
  xsource_t **sources;
  GThreadPool *pool;
  xerror_t *error = NULL;
  gint64 start, end;
  xsize_t n_sources = 50000, i;

  context = xmain_context_default ();
  sources = g_new0 (xsource_t *, n_sources);

  pool = xthread_pool_new (thread_pool_attach_func, context,
                            20, TRUE, NULL);
  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      sources[i] = g_idle_source_new ();
      xsource_set_callback (sources[i], nsources_cb, NULL, NULL);
      xthread_pool_push (pool, sources[i], &error);
      g_assert_no_error (error);
    }
  xthread_pool_free (pool, FALSE, TRUE);
  end = g_get_monotonic_time ();
  g_test_message ("Add sources from threads: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  pool = xthread_pool_new (thread_pool_destroy_func, context,
                            20, TRUE, NULL);
  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      xthread_pool_push (pool, sources[i], &error);
      g_assert_no_error (error);
    }
  xthread_pool_free (pool, FALSE, TRUE);
  end = g_get_monotonic_time ();
  g_test_message ("Remove sources from threads: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  /* Make sure they really did get removed */
  xmain_context_iteration (context, FALSE);

  g_free (sources);
}

static xboolean_t source_finalize_called = FALSE;
static xuint_t source_dispose_called = 0;
static xboolean_t source_dispose_recycle = FALSE;

static void
finalize (xsource_t *source)
{
  g_assert_false (source_finalize_called);
  source_finalize_called = TRUE;
}

static void
dispose (xsource_t *source)
{
  /* Dispose must always be called before finalize */
  g_assert_false (source_finalize_called);

  if (source_dispose_recycle)
    xsource_ref (source);
  source_dispose_called++;
}

static xsource_funcs_t source_funcs = {
  prepare,
  check,
  dispatch,
  finalize,
  NULL,
  NULL
};

static void
test_maincontext_source_finalization (void)
{
  xsource_t *source;

  /* Check if xsource_t destruction without dispose function works and calls the
   * finalize function as expected */
  source_finalize_called = FALSE;
  source_dispose_called = 0;
  source_dispose_recycle = FALSE;
  source = xsource_new (&source_funcs, sizeof (xsource_t));
  xsource_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 0);
  g_assert_true (source_finalize_called);

  /* Check if xsource_t destruction with dispose function works and calls the
   * dispose and finalize function as expected */
  source_finalize_called = FALSE;
  source_dispose_called = 0;
  source_dispose_recycle = FALSE;
  source = xsource_new (&source_funcs, sizeof (xsource_t));
  xsource_set_dispose_function (source, dispose);
  xsource_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 1);
  g_assert_true (source_finalize_called);

  /* Check if xsource_t destruction with dispose function works and recycling
   * the source from dispose works without calling the finalize function */
  source_finalize_called = FALSE;
  source_dispose_called = 0;
  source_dispose_recycle = TRUE;
  source = xsource_new (&source_funcs, sizeof (xsource_t));
  xsource_set_dispose_function (source, dispose);
  xsource_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 1);
  g_assert_false (source_finalize_called);

  /* Check if the source is properly recycled */
  g_assert_cmpint (source->ref_count, ==, 1);

  /* And then get rid of it properly */
  source_dispose_recycle = FALSE;
  xsource_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 2);
  g_assert_true (source_finalize_called);
}

/* xsource_t implementation which optionally keeps a strong reference to another
 * xsource_t until finalization, when it destroys and unrefs the other source.
 */
typedef struct {
  xsource_t source;

  xsource_t *other_source;
} SourceWithSource;

static void
finalize_source_with_source (xsource_t *source)
{
  SourceWithSource *s = (SourceWithSource *) source;

  if (s->other_source)
    {
      xsource_destroy (s->other_source);
      xsource_unref (s->other_source);
      s->other_source = NULL;
    }
}

static xsource_funcs_t source_with_source_funcs = {
  NULL,
  NULL,
  NULL,
  finalize_source_with_source,
  NULL,
  NULL
};

static void
test_maincontext_source_finalization_from_source (xconstpointer user_data)
{
  xmain_context_t *c = xmain_context_new ();
  xsource_t *s1, *s2;

  g_test_summary ("Tests if freeing a xsource_t as part of another xsource_t "
                  "during main context destruction works.");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/merge_requests/1353");

  s1 = xsource_new (&source_with_source_funcs, sizeof (SourceWithSource));
  s2 = xsource_new (&source_with_source_funcs, sizeof (SourceWithSource));
  ((SourceWithSource *)s1)->other_source = xsource_ref (s2);

  /* Attach sources in a different order so they end up being destroyed
   * in a different order by the main context */
  if (GPOINTER_TO_INT (user_data) < 5)
    {
      xsource_attach (s1, c);
      xsource_attach (s2, c);
    }
  else
    {
      xsource_attach (s2, c);
      xsource_attach (s1, c);
    }

  /* Test a few different permutations here */
  if (GPOINTER_TO_INT (user_data) % 5 == 0)
    {
      /* Get rid of our references so that destroying the context
       * will unref them immediately */
      xsource_unref (s1);
      xsource_unref (s2);
      xmain_context_unref (c);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 1)
    {
      /* Destroy and free the sources before the context */
      xsource_destroy (s1);
      xsource_unref (s1);
      xsource_destroy (s2);
      xsource_unref (s2);
      xmain_context_unref (c);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 2)
    {
      /* Destroy and free the sources before the context */
      xsource_destroy (s2);
      xsource_unref (s2);
      xsource_destroy (s1);
      xsource_unref (s1);
      xmain_context_unref (c);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 3)
    {
      /* Destroy and free the context before the sources */
      xmain_context_unref (c);
      xsource_unref (s2);
      xsource_unref (s1);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 4)
    {
      /* Destroy and free the context before the sources */
      xmain_context_unref (c);
      xsource_unref (s1);
      xsource_unref (s2);
    }
}

static xboolean_t
dispatch_source_with_source (xsource_t *source, xsource_func_t callback, xpointer_t user_data)
{
  return G_SOURCE_REMOVE;
}

static xsource_funcs_t source_with_source_funcs_dispatch = {
  NULL,
  NULL,
  dispatch_source_with_source,
  finalize_source_with_source,
  NULL,
  NULL
};

static void
test_maincontext_source_finalization_from_dispatch (xconstpointer user_data)
{
  xmain_context_t *c = xmain_context_new ();
  xsource_t *s1, *s2;

  g_test_summary ("Tests if freeing a xsource_t as part of another xsource_t "
                  "during main context iteration works.");

  s1 = xsource_new (&source_with_source_funcs_dispatch, sizeof (SourceWithSource));
  s2 = xsource_new (&source_with_source_funcs_dispatch, sizeof (SourceWithSource));
  ((SourceWithSource *)s1)->other_source = xsource_ref (s2);

  xsource_attach (s1, c);
  xsource_attach (s2, c);

  if (GPOINTER_TO_INT (user_data) == 0)
    {
      /* This finalizes s1 as part of the iteration, which then destroys and
       * frees s2 too */
      xsource_set_ready_time (s1, 0);
    }
  else if (GPOINTER_TO_INT (user_data) == 1)
    {
      /* This destroys s2 as part of the iteration but does not free it as
       * it's still referenced by s1 */
      xsource_set_ready_time (s2, 0);
    }
  else if (GPOINTER_TO_INT (user_data) == 2)
    {
      /* This destroys both s1 and s2 as part of the iteration */
      xsource_set_ready_time (s1, 0);
      xsource_set_ready_time (s2, 0);
    }

  /* Get rid of our references so only the main context has one now */
  xsource_unref (s1);
  xsource_unref (s2);

  /* Iterate as long as there are sources to dispatch */
  while (xmain_context_iteration (c, FALSE))
    {
      /* Do nothing here */
    }

  xmain_context_unref (c);
}

static void
test_steal_fd (void)
{
  xerror_t *error = NULL;
  xchar_t *tmpfile = NULL;
  int fd = -42;
  int borrowed;
  int stolen;

  g_assert_cmpint (g_steal_fd (&fd), ==, -42);
  g_assert_cmpint (fd, ==, -1);
  g_assert_cmpint (g_steal_fd (&fd), ==, -1);
  g_assert_cmpint (fd, ==, -1);

  fd = xfile_open_tmp (NULL, &tmpfile, &error);
  g_assert_cmpint (fd, >=, 0);
  g_assert_no_error (error);
  borrowed = fd;
  stolen = g_steal_fd (&fd);
  g_assert_cmpint (fd, ==, -1);
  g_assert_cmpint (borrowed, ==, stolen);

  g_close (g_steal_fd (&stolen), &error);
  g_assert_no_error (error);
  g_assert_cmpint (stolen, ==, -1);

  g_assert_no_errno (remove (tmpfile));
  g_free (tmpfile);
}

int
main (int argc, char *argv[])
{
  xint_t i;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/maincontext/basic", test_maincontext_basic);
  g_test_add_func ("/maincontext/nsources_same_priority", test_nsources_same_priority);
  g_test_add_func ("/maincontext/nsources_different_priority", test_nsources_different_priority);
  g_test_add_func ("/maincontext/nsources_threadpool", test_nsources_threadpool);
  g_test_add_func ("/maincontext/source_finalization", test_maincontext_source_finalization);
  for (i = 0; i < 10; i++)
    {
      xchar_t *name = xstrdup_printf ("/maincontext/source_finalization_from_source/%d", i);
      g_test_add_data_func (name, GINT_TO_POINTER (i), test_maincontext_source_finalization_from_source);
      g_free (name);
    }
  for (i = 0; i < 3; i++)
    {
      xchar_t *name = xstrdup_printf ("/maincontext/source_finalization_from_dispatch/%d", i);
      g_test_add_data_func (name, GINT_TO_POINTER (i), test_maincontext_source_finalization_from_dispatch);
      g_free (name);
    }
  g_test_add_func ("/mainloop/basic", test_mainloop_basic);
  g_test_add_func ("/mainloop/timeouts", test_timeouts);
  g_test_add_func ("/mainloop/priorities", test_priorities);
  g_test_add_func ("/mainloop/invoke", test_invoke);
  g_test_add_func ("/mainloop/child_sources", test_child_sources);
  g_test_add_func ("/mainloop/recursive_child_sources", test_recursive_child_sources);
  g_test_add_func ("/mainloop/swapping_child_sources", test_swapping_child_sources);
  g_test_add_func ("/mainloop/blocked_child_sources", test_blocked_child_sources);
  g_test_add_func ("/mainloop/source_time", test_source_time);
  g_test_add_func ("/mainloop/overflow", test_mainloop_overflow);
  g_test_add_func ("/mainloop/ready-time", test_ready_time);
  g_test_add_func ("/mainloop/wakeup", test_wakeup);
  g_test_add_func ("/mainloop/remove-invalid", test_remove_invalid);
  g_test_add_func ("/mainloop/unref-while-pending", test_unref_while_pending);
#ifdef G_OS_UNIX
  g_test_add_func ("/mainloop/unix-fd", test_unix_fd);
  g_test_add_func ("/mainloop/unix-fd-source", test_unix_fd_source);
  g_test_add_func ("/mainloop/source-unix-fd-api", test_source_unix_fd_api);
  g_test_add_func ("/mainloop/wait", test_mainloop_wait);
  g_test_add_func ("/mainloop/unix-file-poll", test_unix_file_poll);
  g_test_add_func ("/mainloop/unix-fd-priority", test_unix_fd_priority);
#endif
  g_test_add_func ("/mainloop/nfds", test_nfds);
  g_test_add_func ("/mainloop/steal-fd", test_steal_fd);
  g_test_add_data_func ("/mainloop/ownerless-polling/attach-first", GINT_TO_POINTER (TRUE), test_ownerless_polling);
  g_test_add_data_func ("/mainloop/ownerless-polling/pop-first", GINT_TO_POINTER (FALSE), test_ownerless_polling);

  return g_test_run ();
}
