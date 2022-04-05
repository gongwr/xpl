#include <glib.h>
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define XTYPE_TEST               (my_test_get_type ())
#define MY_TEST(test)              (XTYPE_CHECK_INSTANCE_CAST ((test), XTYPE_TEST, GTest))
#define MY_IS_TEST(test)           (XTYPE_CHECK_INSTANCE_TYPE ((test), XTYPE_TEST))
#define MY_TEST_CLASS(tclass)      (XTYPE_CHECK_CLASS_CAST ((tclass), XTYPE_TEST, GTestClass))
#define MY_IS_TEST_CLASS(tclass)   (XTYPE_CHECK_CLASS_TYPE ((tclass), XTYPE_TEST))
#define MY_TEST_GET_CLASS(test)    (XTYPE_INSTANCE_GET_CLASS ((test), XTYPE_TEST, GTestClass))

typedef struct _GTest GTest;
typedef struct _GTestClass GTestClass;

struct _GTest
{
  xobject_t object;
};

struct _GTestClass
{
  xobject_class_t parent_class;
};

static xtype_t my_test_get_type (void);
static xint_t stopping;  /* (atomic) */

static void my_test_class_init (GTestClass * klass);
static void my_test_init (GTest * test);
static void my_test_dispose (xobject_t * object);

static xobject_class_t *parent_class = NULL;

static xtype_t
my_test_get_type (void)
{
  static xtype_t test_type = 0;

  if (!test_type) {
    const GTypeInfo test_info = {
      sizeof (GTestClass),
      NULL,
      NULL,
      (GClassInitFunc) my_test_class_init,
      NULL,
      NULL,
      sizeof (GTest),
      0,
      (GInstanceInitFunc) my_test_init,
      NULL
    };

    test_type = g_type_register_static (XTYPE_OBJECT, "GTest",
        &test_info, 0);
  }
  return test_type;
}

static void
my_test_class_init (GTestClass * klass)
{
  xobject_class_t *gobject_class;

  gobject_class = (xobject_class_t *) klass;

  parent_class = g_type_class_ref (XTYPE_OBJECT);

  gobject_class->dispose = my_test_dispose;
}

static void
my_test_init (GTest * test)
{
  g_print ("init %p\n", test);
}

static void
my_test_dispose (xobject_t * object)
{
  GTest *test;

  test = MY_TEST (object);

  g_print ("dispose %p!\n", test);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
my_test_do_refcount (GTest * test)
{
  g_object_ref (test);
  g_object_unref (test);
}

static xpointer_t
run_thread (GTest * test)
{
  xint_t i = 1;

  while (!g_atomic_int_get (&stopping)) {
    my_test_do_refcount (test);
    if ((i++ % 10000) == 0) {
      g_print (".");
      g_thread_yield(); /* force context switch */
    }
  }

  return NULL;
}

int
main (int argc, char **argv)
{
  xuint_t i;
  GTest *test1, *test2;
  GArray *test_threads;
  const xuint_t n_threads = 5;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test1 = g_object_new (XTYPE_TEST, NULL);
  test2 = g_object_new (XTYPE_TEST, NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (GThread *));

  g_atomic_int_set (&stopping, 0);

  for (i = 0; i < n_threads; i++) {
    GThread *thread;

    thread = g_thread_create ((GThreadFunc) run_thread, test1, TRUE, NULL);
    g_array_append_val (test_threads, thread);

    thread = g_thread_create ((GThreadFunc) run_thread, test2, TRUE, NULL);
    g_array_append_val (test_threads, thread);
  }
  g_usleep (5000000);

  g_atomic_int_set (&stopping, 1);

  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < 2 * n_threads; i++) {
    GThread *thread;

    thread = g_array_index (test_threads, GThread *, i);
    g_thread_join (thread);
  }

  g_object_unref (test1);
  g_object_unref (test2);
  g_array_unref (test_threads);

  g_print ("stopped\n");

  return 0;
}
