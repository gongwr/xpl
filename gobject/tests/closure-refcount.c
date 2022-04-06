/* Copyright (C) 2005 Imendio AB
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define TEST_POINTER1   ((xpointer_t) 47)
#define TEST_POINTER2   ((xpointer_t) 49)
#define TEST_INT1       (-77)
#define TEST_INT2       (78)

/* --- xtest_t class --- */
typedef struct {
  xobject_t object;
  xint_t value;
  xpointer_t test_pointer1;
  xpointer_t test_pointer2;
} xtest_t;
typedef struct {
  xobject_class_t parent_class;
  void (*test_signal1) (xtest_t * test, xint_t an_int);
  void (*test_signal2) (xtest_t * test, xint_t an_int);
} xtest_class_t;

#define XTYPE_TEST                (xtest_get_type ())
#define XTEST(test)              (XTYPE_CHECK_INSTANCE_CAST ((test), XTYPE_TEST, xtest_t))
#define MY_IS_TEST(test)           (XTYPE_CHECK_INSTANCE_TYPE ((test), XTYPE_TEST))
#define XTEST_CLASS(tclass)      (XTYPE_CHECK_CLASS_CAST ((tclass), XTYPE_TEST, xtest_class_t))
#define MY_IS_TEST_CLASS(tclass)   (XTYPE_CHECK_CLASS_TYPE ((tclass), XTYPE_TEST))
#define XTEST_GET_CLASS(test)    (XTYPE_INSTANCE_GET_CLASS ((test), XTYPE_TEST, xtest_class_t))

static xtype_t xtest_get_type (void);
G_DEFINE_TYPE (xtest, xtest, XTYPE_OBJECT)

/* Test state */
typedef struct
{
  xclosure_t *closure;  /* (unowned) */
  xboolean_t stopping;
  xboolean_t seen_signal_handler;
  xboolean_t seen_cleanup;
  xboolean_t seen_test_int1;
  xboolean_t seen_test_int2;
  xboolean_t seen_thread1;
  xboolean_t seen_thread2;
} TestClosureRefcountData;

/* --- functions --- */
static void
xtest_init (xtest_t * test)
{
  g_test_message ("Init %p", test);

  test->value = 0;
  test->test_pointer1 = TEST_POINTER1;
  test->test_pointer2 = TEST_POINTER2;
}

typedef enum
{
  PROP_TEST_PROP = 1,
} MyTestProperty;

typedef enum
{
  SIGNAL_TEST_SIGNAL1,
  SIGNAL_TEST_SIGNAL2,
} MyTestSignal;

static xuint_t signals[SIGNAL_TEST_SIGNAL2 + 1] = { 0, };

static void
xtest_set_property (xobject_t      *object,
                     xuint_t         prop_id,
                     const xvalue_t *value,
                     xparam_spec_t   *pspec)
{
  xtest_t *test = XTEST (object);
  switch (prop_id)
    {
    case PROP_TEST_PROP:
      test->value = xvalue_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xtest_get_property (xobject_t    *object,
                     xuint_t       prop_id,
                     xvalue_t     *value,
                     xparam_spec_t *pspec)
{
  xtest_t *test = XTEST (object);
  switch (prop_id)
    {
    case PROP_TEST_PROP:
      xvalue_set_int (value, test->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xtest_test_signal2 (xtest_t *test,
                      xint_t   an_int)
{
}

static void
xtest_emit_test_signal1 (xtest_t *test,
                           xint_t   vint)
{
  g_signal_emit (G_OBJECT (test), signals[SIGNAL_TEST_SIGNAL1], 0, vint);
}

static void
xtest_emit_test_signal2 (xtest_t *test,
                           xint_t   vint)
{
  g_signal_emit (G_OBJECT (test), signals[SIGNAL_TEST_SIGNAL2], 0, vint);
}

static void
xtest_class_init (xtest_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = xtest_set_property;
  gobject_class->get_property = xtest_get_property;

  signals[SIGNAL_TEST_SIGNAL1] =
      g_signal_new ("test-signal1", XTYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (xtest_class_t, test_signal1), NULL, NULL,
                    g_cclosure_marshal_VOID__INT, XTYPE_NONE, 1, XTYPE_INT);
  signals[SIGNAL_TEST_SIGNAL2] =
      g_signal_new ("test-signal2", XTYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (xtest_class_t, test_signal2), NULL, NULL,
                    g_cclosure_marshal_VOID__INT, XTYPE_NONE, 1, XTYPE_INT);

  xobject_class_install_property (G_OBJECT_CLASS (klass), PROP_TEST_PROP,
                                   g_param_spec_int ("test-prop", "Test Prop", "Test property",
                                                     0, 1, 0, G_PARAM_READWRITE));
  klass->test_signal2 = xtest_test_signal2;
}

static void
test_closure (xclosure_t *closure)
{
  /* try to produce high contention in closure->ref_count */
  xuint_t i = 0, n = g_random_int () % 199;
  for (i = 0; i < n; i++)
    xclosure_ref (closure);
  xclosure_sink (closure); /* NOP */
  for (i = 0; i < n; i++)
    xclosure_unref (closure);
}

static xpointer_t
thread1_main (xpointer_t user_data)
{
  TestClosureRefcountData *data = user_data;
  xuint_t i = 0;

  for (i = 1; !g_atomic_int_get (&data->stopping); i++)
    {
      test_closure (data->closure);
      if (i % 10000 == 0)
        {
          g_test_message ("Yielding from thread1");
          xthread_yield (); /* force context switch */
          g_atomic_int_set (&data->seen_thread1, TRUE);
        }
    }
  return NULL;
}

static xpointer_t
thread2_main (xpointer_t user_data)
{
  TestClosureRefcountData *data = user_data;
  xuint_t i = 0;

  for (i = 1; !g_atomic_int_get (&data->stopping); i++)
    {
      test_closure (data->closure);
      if (i % 10000 == 0)
        {
          g_test_message ("Yielding from thread2");
          xthread_yield (); /* force context switch */
          g_atomic_int_set (&data->seen_thread2, TRUE);
        }
    }
  return NULL;
}

static void
test_signal_handler (xtest_t   *test,
                     xint_t     vint,
                     xpointer_t user_data)
{
  TestClosureRefcountData *data = user_data;

  g_assert_true (test->test_pointer1 == TEST_POINTER1);

  data->seen_signal_handler = TRUE;
  data->seen_test_int1 |= vint == TEST_INT1;
  data->seen_test_int2 |= vint == TEST_INT2;
}

static void
destroy_data (xpointer_t  user_data,
              xclosure_t *closure)
{
  TestClosureRefcountData *data = user_data;

  data->seen_cleanup = TRUE;
  g_assert_true (data->closure == closure);
  g_assert_cmpint (closure->ref_count, ==, 0);
}

static void
test_emissions (xtest_t *test)
{
  xtest_emit_test_signal1 (test, TEST_INT1);
  xtest_emit_test_signal2 (test, TEST_INT2);
}

/* Test that closure refcounting works even when high contested between three
 * threads (the main thread, thread1 and thread2). Both child threads are
 * contesting refs/unrefs, while the main thread periodically emits signals
 * which also do refs/unrefs on closures. */
static void
test_closure_refcount (void)
{
  xthread_t *thread1, *thread2;
  TestClosureRefcountData test_data = { 0, };
  xclosure_t *closure;
  xtest_t *object;
  xuint_t i, n_iterations;

  object = xobject_new (XTYPE_TEST, NULL);
  closure = g_cclosure_new (G_CALLBACK (test_signal_handler), &test_data, destroy_data);

  g_signal_connect_closure (object, "test-signal1", closure, FALSE);
  g_signal_connect_closure (object, "test-signal2", closure, FALSE);

  test_data.stopping = FALSE;
  test_data.closure = closure;

  thread1 = xthread_new ("thread1", thread1_main, &test_data);
  thread2 = xthread_new ("thread2", thread2_main, &test_data);

  /* The 16-bit compare-and-swap operations currently used for closure
   * refcounts are really slow on some ARM CPUs, notably Cortex-A57.
   * Reduce the number of iterations so that the test completes in a
   * finite time, but don't reduce it so much that the main thread
   * starves the other threads and causes a test failure.
   *
   * https://gitlab.gnome.org/GNOME/glib/issues/1316
   * aka https://bugs.debian.org/880883 */
#if defined(__aarch64__) || defined(__arm__)
  n_iterations = 100000;
#else
  n_iterations = 1000000;
#endif

  /* Run the test for a reasonably high number of iterations, and ensure we
   * don’t terminate until at least 10000 iterations have completed in both
   * thread1 and thread2. Even though @n_iterations is high, we can’t guarantee
   * that the scheduler allocates time fairly (or at all!) to thread1 or
   * thread2. */
  for (i = 1;
       i < n_iterations ||
       !g_atomic_int_get (&test_data.seen_thread1) ||
       !g_atomic_int_get (&test_data.seen_thread2);
       i++)
    {
      test_emissions (object);
      if (i % 10000 == 0)
        {
          g_test_message ("Yielding from main thread");
          xthread_yield (); /* force context switch */
        }
    }

  g_atomic_int_set (&test_data.stopping, TRUE);
  g_test_message ("Stopping");

  /* wait for thread shutdown */
  xthread_join (thread1);
  xthread_join (thread2);

  /* finalize object, destroy signals, run cleanup code */
  xobject_unref (object);

  g_test_message ("Stopped");

  g_assert_true (g_atomic_int_get (&test_data.seen_thread1));
  g_assert_true (g_atomic_int_get (&test_data.seen_thread2));
  g_assert_true (test_data.seen_test_int1);
  g_assert_true (test_data.seen_test_int2);
  g_assert_true (test_data.seen_signal_handler);
  g_assert_true (test_data.seen_cleanup);
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/closure/refcount", test_closure_refcount);

  return g_test_run ();
}
