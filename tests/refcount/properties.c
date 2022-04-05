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

enum {
  PROP_0,
  PROP_DUMMY
};

typedef struct _GTest GTest;
typedef struct _GTestClass GTestClass;

struct _GTest
{
  xobject_t object;
  xint_t id;
  xint_t dummy;

  xint_t count;
};

struct _GTestClass
{
  xobject_class_t parent_class;
};

static xtype_t my_test_get_type (void);
static xboolean_t stopping;

static void my_test_class_init (GTestClass * klass);
static void my_test_init (GTest * test);
static void my_test_dispose (xobject_t * object);
static void my_test_get_property (xobject_t    *object,
                                  xuint_t       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec);
static void my_test_set_property (xobject_t      *object,
                                  xuint_t         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec);

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

    test_type = g_type_register_static (XTYPE_OBJECT, "GTest", &test_info, 0);
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
  gobject_class->get_property = my_test_get_property;
  gobject_class->set_property = my_test_set_property;

  g_object_class_install_property (gobject_class,
				   PROP_DUMMY,
				   g_param_spec_int ("dummy",
						     NULL,
						     NULL,
						     0, G_MAXINT, 0,
						     G_PARAM_READWRITE));
}

static void
my_test_init (GTest * test)
{
  static xuint_t static_id = 1;
  test->id = static_id++;
}

static void
my_test_dispose (xobject_t * object)
{
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
my_test_get_property (xobject_t    *object,
                      xuint_t       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      g_value_set_int (value, test->dummy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_test_set_property (xobject_t      *object,
                      xuint_t         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      test->dummy = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
dummy_notify (xobject_t    *object,
              GParamSpec *pspec)
{
  GTest *test;

  test = MY_TEST (object);

  test->count++;
}

static void
my_test_do_property (GTest * test)
{
  xint_t dummy;

  g_object_get (test, "dummy", &dummy, NULL);
  g_object_set (test, "dummy", dummy + 1, NULL);
}

static xpointer_t
run_thread (GTest * test)
{
  xint_t i = 1;

  while (!g_atomic_int_get (&stopping)) {
    my_test_do_property (test);
    if ((i++ % 10000) == 0)
      {
        g_print (".%c", 'a' + test->id);
        g_thread_yield(); /* force context switch */
      }
  }

  return NULL;
}

int
main (int argc, char **argv)
{
#define N_THREADS 5
  GThread *test_threads[N_THREADS];
  GTest *test_objects[N_THREADS];
  xint_t i;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  for (i = 0; i < N_THREADS; i++) {
    GTest *test;

    test = g_object_new (XTYPE_TEST, NULL);
    test_objects[i] = test;

    g_assert (test->count == test->dummy);
    g_signal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL);
  }

  g_atomic_int_set (&stopping, FALSE);

  for (i = 0; i < N_THREADS; i++)
    test_threads[i] = g_thread_create ((GThreadFunc) run_thread, test_objects[i], TRUE, NULL);

  g_usleep (3000000);

  g_atomic_int_set (&stopping, TRUE);
  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < N_THREADS; i++)
    g_thread_join (test_threads[i]);

  g_print ("stopped\n");

  for (i = 0; i < N_THREADS; i++) {
    GTest *test = test_objects[i];

    g_assert (test->count == test->dummy);
    g_object_unref (test);
  }

  return 0;
}
