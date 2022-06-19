#include <glib.h>
#ifdef G_OS_UNIX
#include <unistd.h>
#endif

static xmain_loop_t *loop;

static xboolean_t
stop_waiting (xpointer_t data)
{
  xmain_loop_quit (loop);

  return XSOURCE_REMOVE;
}

static xboolean_t
unreachable_callback (xpointer_t data)
{
  g_assert_not_reached ();

  return XSOURCE_REMOVE;
}

static void
test_seconds (void)
{
  xuint_t id;

  /* Bug 642052 mentions that g_timeout_add_seconds(21475) schedules a
   * job that runs once per second.
   *
   * test_t that that isn't true anymore by scheduling two jobs:
   *   - one, as above
   *   - another that runs in 2100ms
   *
   * If everything is working properly, the 2100ms one should run first
   * (and exit the mainloop).  If we ever see the 21475 second job run
   * then we have trouble (since it ran in less than 2 seconds).
   *
   * We need a timeout of at least 2 seconds because
   * g_timeout_add_seconds() can add as much as an additional second of
   * latency.
   */
  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=642052");
  loop = xmain_loop_new (NULL, FALSE);

  g_timeout_add (2100, stop_waiting, NULL);
  id = g_timeout_add_seconds (21475, unreachable_callback, NULL);

  xmain_loop_run (loop);
  xmain_loop_unref (loop);

  xsource_remove (id);
}

static void
test_weeks_overflow (void)
{
  xuint_t id;
  xuint_t interval_seconds;

  /* Internally, the xuint_t interval (in seconds) was converted to milliseconds
   * then stored in a xuint_t variable. This meant that any interval larger than
   * G_MAXUINT / 1000 would overflow.
   *
   * On a system with 32-bit xuint_t, the interval (G_MAXUINT / 1000) + 1 seconds
   * (49.7 days) would end wrapping to 704 milliseconds.
   *
   * test_t that that isn't true anymore by scheduling two jobs:
   *   - one, as above
   *   - another that runs in 2100ms
   *
   * If everything is working properly, the 2100ms one should run first
   * (and exit the mainloop).  If we ever see the other job run
   * then we have trouble (since it ran in less than 2 seconds).
   *
   * We need a timeout of at least 2 seconds because
   * g_timeout_add_seconds() can add as much as an additional second of
   * latency.
   */
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1600");
  loop = xmain_loop_new (NULL, FALSE);

  g_timeout_add (2100, stop_waiting, NULL);
  interval_seconds = 1 + G_MAXUINT / 1000;
  id = g_timeout_add_seconds (interval_seconds, unreachable_callback, NULL);

  xmain_loop_run (loop);
  xmain_loop_unref (loop);

  xsource_remove (id);
}

/* The ready_time for a xsource_t is stored as a sint64_t, as an absolute monotonic
 * time in microseconds. To call poll(), this must be converted to a relative
 * timeout, in milliseconds, as a xint_t. If the ready_time is sufficiently far
 * in the future, the timeout will not fit. Previously, it would be narrowed in
 * an implementation-defined way; if this gave a negative result, poll() would
 * block forever.
 *
 * This test creates a xsource_t with the largest possible ready_time (a little
 * over 292 millennia, assuming g_get_monotonic_time() starts from near 0 when
 * the system boots), adds it to a xmain_context_t, queries it for the parameters
 * to pass to poll() -- essentially the first half of
 * xmain_context_iteration() -- and checks that the timeout is a large
 * positive number.
 */
static void
test_far_future_ready_time (void)
{
  xsource_funcs_t source_funcs = { 0 };
  xmain_context_t *context = xmain_context_new ();
  xsource_t *source = xsource_new (&source_funcs, sizeof (xsource_t));
  xboolean_t acquired, ready;
  xint_t priority, timeout_, n_fds;

  xsource_set_ready_time (source, G_MAXINT64);
  xsource_attach (source, context);

  acquired = xmain_context_acquire (context);
  g_assert_true (acquired);

  ready = xmain_context_prepare (context, &priority);
  g_assert_false (ready);

  n_fds = 0;
  n_fds = xmain_context_query (context, priority, &timeout_, NULL, n_fds);

  g_assert_cmpint (n_fds, >=, 0);

  /* The true timeout in milliseconds doesn't fit into a xint_t. We definitely
   * don't want poll() to block forever:
   */
  g_assert_cmpint (timeout_, >=, 0);
  /* Instead, we want it to block for as long as possible: */
  g_assert_cmpint (timeout_, ==, G_MAXINT);

  xmain_context_release (context);
  xmain_context_unref (context);
  xsource_unref (source);
}

static sint64_t last_time;
static xint_t count;

static xboolean_t
test_func (xpointer_t data)
{
  sint64_t current_time;

  current_time = g_get_monotonic_time ();

  /* We accept 2 on the first iteration because _add_seconds() can
   * have an initial latency of 1 second, see its documentation.
   *
   * Allow up to 500ms leeway for rounding and scheduling.
   */
  if (count == 0)
    g_assert_cmpint (current_time / 1000 - last_time / 1000, <=, 2500);
  else
    g_assert_cmpint (current_time / 1000 - last_time / 1000, <=, 1500);

  last_time = current_time;
  count++;

  /* Make the timeout take up to 0.1 seconds.
   * We should still get scheduled for the next second.
   */
  g_usleep (count * 10000);

  if (count < 10)
    return TRUE;

  xmain_loop_quit (loop);

  return FALSE;
}

static void
test_rounding (void)
{
  loop = xmain_loop_new (NULL, FALSE);

  last_time = g_get_monotonic_time ();
  g_timeout_add_seconds (1, test_func, NULL);

  xmain_loop_run (loop);
  xmain_loop_unref (loop);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/timeout/seconds", test_seconds);
  g_test_add_func ("/timeout/weeks-overflow", test_weeks_overflow);
  g_test_add_func ("/timeout/far-future-ready-time", test_far_future_ready_time);
  g_test_add_func ("/timeout/rounding", test_rounding);

  return g_test_run ();
}
