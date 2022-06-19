#include <glib.h>
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define XTYPE_TEST               (xtest_get_type ())
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
};

struct _xtest_class
{
  xobject_class_t parent_class;
};

static xtype_t xtest_get_type (void);
static xint_t stopping;  /* (atomic) */

static void xtest_class_init (xtest_class_t * klass);
static void xtest_init (xtest_t * test);
static void xtest_dispose (xobject_t * object);

static xobject_class_t *parent_class = NULL;

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
  xobject_class_t *xobject_class;

  xobject_class = (xobject_class_t *) klass;

  parent_class = xtype_class_ref (XTYPE_OBJECT);

  xobject_class->dispose = xtest_dispose;
}

static void
xtest_init (xtest_t * test)
{
  g_print ("init %p\n", test);
}

static void
xtest_dispose (xobject_t * object)
{
  xtest_t *test;

  test = XTEST (object);

  g_print ("dispose %p!\n", test);

  XOBJECT_CLASS (parent_class)->dispose (object);
}

static void
xtest_do_refcount (xtest_t * test)
{
  xobject_ref (test);
  xobject_unref (test);
}

static xpointer_t
run_thread (xtest_t * test)
{
  xint_t i = 1;

  while (!g_atomic_int_get (&stopping)) {
    xtest_do_refcount (test);
    if ((i++ % 10000) == 0) {
      g_print (".");
      xthread_yield(); /* force context switch */
    }
  }

  return NULL;
}

int
main (int argc, char **argv)
{
  xuint_t i;
  xtest_t *test1, *test2;
  xarray_t *test_threads;
  const xuint_t n_threads = 5;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test1 = xobject_new (XTYPE_TEST, NULL);
  test2 = xobject_new (XTYPE_TEST, NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (xthread_t *));

  g_atomic_int_set (&stopping, 0);

  for (i = 0; i < n_threads; i++) {
    xthread_t *thread;

    thread = xthread_create ((GThreadFunc) run_thread, test1, TRUE, NULL);
    g_array_append_val (test_threads, thread);

    thread = xthread_create ((GThreadFunc) run_thread, test2, TRUE, NULL);
    g_array_append_val (test_threads, thread);
  }
  g_usleep (5000000);

  g_atomic_int_set (&stopping, 1);

  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < 2 * n_threads; i++) {
    xthread_t *thread;

    thread = g_array_index (test_threads, xthread_t *, i);
    xthread_join (thread);
  }

  xobject_unref (test1);
  xobject_unref (test2);
  g_array_unref (test_threads);

  g_print ("stopped\n");

  return 0;
}
