#include <glib.h>
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define XTYPE_TEST                (xtest_get_type ())
#define XTEST(test)              (XTYPE_CHECK_INSTANCE_CAST ((test), XTYPE_TEST, xtest_t))
#define MY_IS_TEST(test)           (XTYPE_CHECK_INSTANCE_TYPE ((test), XTYPE_TEST))
#define XTEST_CLASS(tclass)      (XTYPE_CHECK_CLASS_CAST ((tclass), XTYPE_TEST, xtest_class_t))
#define MY_IS_TEST_CLASS(tclass)   (XTYPE_CHECK_CLASS_TYPE ((tclass), XTYPE_TEST))
#define XTEST_GET_CLASS(test)    (XTYPE_INSTANCE_GET_CLASS ((test), XTYPE_TEST, xtest_class_t))

typedef struct _xtest xtest_t;
typedef struct _xtest_class xtest_class_t;

struct _xtest
{
  xobject_t object;

  xint_t value;
};

struct _xtest_class
{
  xobject_class_t parent_class;

  void (*test_signal1) (xtest_t * test, xint_t an_int);
  void (*test_signal2) (xtest_t * test, xint_t an_int);
  xchar_t * (*test_signal3) (xtest_t * test, xint_t an_int);
};

static xtype_t xtest_get_type (void);
static xboolean_t stopping;

/* Element signals and args */
enum
{
  TEST_SIGNAL1,
  TEST_SIGNAL2,
  TEST_SIGNAL3,
  /* add more above */
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_TEST_PROP
};

static void xtest_class_init (xtest_class_t * klass);
static void xtest_init (xtest_t * test);
static void xtest_dispose (xobject_t * object);

static void signal2_handler (xtest_t * test, xint_t anint);
static xchar_t * signal3_handler (xtest_t * test, xint_t anint);

static void xtest_set_property (xobject_t * object, xuint_t prop_id,
    const xvalue_t * value, xparam_spec_t * pspec);
static void xtest_get_property (xobject_t * object, xuint_t prop_id,
    xvalue_t * value, xparam_spec_t * pspec);

static xobject_class_t *parent_class = NULL;

static xuint_t xtest_signals[LAST_SIGNAL] = { 0 };

static xtype_t
xtest_get_type (void)
{
  static xtype_t test_type = 0;

  if (!test_type) {
    const xtype_info_t test_info = {
      sizeof (xtest_class_t),
      NULL,
      NULL,
      (xclass_init_func_t) xtest_class_init,
      NULL,
      NULL,
      sizeof (xtest_t),
      0,
      (xinstance_init_func_t) xtest_init,
      NULL
    };

    test_type = xtype_register_static (XTYPE_OBJECT, "xtest_t",
        &test_info, 0);
  }
  return test_type;
}

static void
xtest_class_init (xtest_class_t * klass)
{
  xobject_class_t *gobject_class;

  gobject_class = (xobject_class_t *) klass;

  parent_class = xtype_class_ref (XTYPE_OBJECT);

  gobject_class->dispose = xtest_dispose;
  gobject_class->set_property = xtest_set_property;
  gobject_class->get_property = xtest_get_property;

  xtest_signals[TEST_SIGNAL1] =
      xsignal_new ("test-signal1", XTYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (xtest_class_t, test_signal1), NULL,
      NULL, g_cclosure_marshal_VOID__INT, XTYPE_NONE, 1, XTYPE_INT);
  xtest_signals[TEST_SIGNAL2] =
      xsignal_new ("test-signal2", XTYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (xtest_class_t, test_signal2), NULL,
      NULL, g_cclosure_marshal_VOID__INT, XTYPE_NONE, 1, XTYPE_INT);
  xtest_signals[TEST_SIGNAL3] =
      xsignal_new ("test-signal3", XTYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (xtest_class_t, test_signal3), NULL,
      NULL, g_cclosure_marshal_generic, XTYPE_STRING, 1, XTYPE_INT);

  xobject_class_install_property (G_OBJECT_CLASS (klass), ARG_TEST_PROP,
      g_param_spec_int ("test-prop", "test_t Prop", "test_t property",
          0, 1, 0, G_PARAM_READWRITE));

  klass->test_signal2 = signal2_handler;
  klass->test_signal3 = signal3_handler;
}

static void
xtest_init (xtest_t * test)
{
  g_print ("init %p\n", test);

  test->value = 0;
}

static void
xtest_dispose (xobject_t * object)
{
  xtest_t *test;

  test = XTEST (object);

  g_print ("dispose %p!\n", test);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
xtest_set_property (xobject_t * object, xuint_t prop_id,
                      const xvalue_t * value, xparam_spec_t * pspec)
{
  xtest_t *test;

  test = XTEST (object);

  switch (prop_id) {
    case ARG_TEST_PROP:
      test->value = xvalue_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
xtest_get_property (xobject_t * object, xuint_t prop_id,
                      xvalue_t * value, xparam_spec_t * pspec)
{
  xtest_t *test;

  test = XTEST (object);

  switch (prop_id) {
    case ARG_TEST_PROP:
      xvalue_set_int (value, test->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
xtest_do_signal1 (xtest_t * test)
{
  xsignal_emit (G_OBJECT (test), xtest_signals[TEST_SIGNAL1], 0, 0);
}

static void
signal2_handler (xtest_t * test, xint_t anint)
{
}

static void
xtest_do_signal2 (xtest_t * test)
{
  xsignal_emit (G_OBJECT (test), xtest_signals[TEST_SIGNAL2], 0, 0);
}

static xchar_t *
signal3_handler (xtest_t * test, xint_t anint)
{
  return xstrdup ("test");
}

static void
xtest_do_signal3 (xtest_t * test)
{
  xchar_t *res;

  xsignal_emit (G_OBJECT (test), xtest_signals[TEST_SIGNAL3], 0, 0, &res);
  g_assert (res);
  g_free (res);
}

static void
xtest_do_prop (xtest_t * test)
{
  test->value = g_random_int ();
  xobject_notify (G_OBJECT (test), "test-prop");
}

static xpointer_t
run_thread (xtest_t * test)
{
  xint_t i = 1;

  while (!g_atomic_int_get (&stopping)) {
    if (TESTNUM == 1)
      xtest_do_signal1 (test);
    if (TESTNUM == 2)
      xtest_do_signal2 (test);
    if (TESTNUM == 3)
      xtest_do_prop (test);
    if (TESTNUM == 4)
      xtest_do_signal3 (test);
    if ((i++ % 10000) == 0) {
      g_print (".");
      xthread_yield(); /* force context switch */
    }
  }

  return NULL;
}

static void
notify (xobject_t *object, xparam_spec_t *spec, xpointer_t user_data)
{
  xint_t value;

  xobject_get (object, "test-prop", &value, NULL);
  /*g_print ("+ %d", value);*/
}

int
main (int argc, char **argv)
{
  xint_t i;
  xtest_t *test1, *test2;
  xarray_t *test_threads;
  const xint_t n_threads = 1;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test1 = xobject_new (XTYPE_TEST, NULL);
  test2 = xobject_new (XTYPE_TEST, NULL);

  xsignal_connect (test1, "notify::test-prop", G_CALLBACK (notify), NULL);
  xsignal_connect (test1, "test-signal1", G_CALLBACK (notify), NULL);
  xsignal_connect (test1, "test-signal2", G_CALLBACK (notify), NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (xthread_t *));

  stopping = FALSE;

  for (i = 0; i < n_threads; i++) {
    xthread_t *thread;

    thread = xthread_create ((GThreadFunc) run_thread, test1, TRUE, NULL);
    g_array_append_val (test_threads, thread);

    thread = xthread_create ((GThreadFunc) run_thread, test2, TRUE, NULL);
    g_array_append_val (test_threads, thread);
  }
  g_usleep (5000000);

  g_atomic_int_set (&stopping, TRUE);

  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < 2 * n_threads; i++) {
    xthread_t *thread;

    thread = g_array_index (test_threads, xthread_t *, i);
    xthread_join (thread);
  }

  g_print ("stopped\n");

  g_array_free (test_threads, TRUE);
  xobject_unref (test1);
  xobject_unref (test2);

  return 0;
}
