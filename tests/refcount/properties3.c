#include <glib.h>
#include <glib-object.h>

#define XTYPE_TEST                (my_test_get_type ())
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
  xint_t setcount;
};

struct _GTestClass
{
  xobject_class_t parent_class;
};

static xtype_t my_test_get_type (void);
G_DEFINE_TYPE (GTest, my_test, XTYPE_OBJECT)

static xint_t stopping;  /* (atomic) */

static void my_test_get_property (xobject_t    *object,
				  xuint_t       prop_id,
				  GValue     *value,
				  GParamSpec *pspec);
static void my_test_set_property (xobject_t      *object,
				  xuint_t         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec);

static void
my_test_class_init (GTestClass * klass)
{
  xobject_class_t *gobject_class;

  gobject_class = (xobject_class_t *) klass;

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
      g_value_set_int (value, g_atomic_int_get (&test->dummy));
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
      g_atomic_int_set (&test->dummy, g_value_get_int (value));
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

  g_atomic_int_inc (&test->count);
}

static void
my_test_do_property (GTest * test)
{
  xint_t dummy;

  g_atomic_int_inc (&test->setcount);

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
  xint_t i;
  GTest *test;
  GArray *test_threads;
  const xint_t n_threads = 5;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test = g_object_new (XTYPE_TEST, NULL);

  g_assert (test->count == test->dummy);
  g_signal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (GThread *));

  g_atomic_int_set (&stopping, 0);

  for (i = 0; i < n_threads; i++) {
    GThread *thread;

    thread = g_thread_create ((GThreadFunc) run_thread, test, TRUE, NULL);
    g_array_append_val (test_threads, thread);
  }
  g_usleep (30000000);

  g_atomic_int_set (&stopping, 1);
  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < n_threads; i++) {
    GThread *thread;

    thread = g_array_index (test_threads, GThread *, i);
    g_thread_join (thread);
  }

  g_print ("stopped\n");

  g_print ("%d %d\n", test->setcount, test->count);

  g_array_free (test_threads, TRUE);
  g_object_unref (test);

  return 0;
}
