#include <glib.h>
#include <glib-object.h>

#define XTYPE_TEST                (xtest_get_type ())
#define XTEST(test)              (XTYPE_CHECK_INSTANCE_CAST ((test), XTYPE_TEST, xtest_t))
#define MY_IS_TEST(test)           (XTYPE_CHECK_INSTANCE_TYPE ((test), XTYPE_TEST))
#define XTEST_CLASS(tclass)      (XTYPE_CHECK_CLASS_CAST ((tclass), XTYPE_TEST, xtest_class_t))
#define MY_IS_TEST_CLASS(tclass)   (XTYPE_CHECK_CLASS_TYPE ((tclass), XTYPE_TEST))
#define XTEST_GET_CLASS(test)    (XTYPE_INSTANCE_GET_CLASS ((test), XTYPE_TEST, xtest_class_t))

enum {
  PROP_0,
  PROP_DUMMY
};

typedef struct _xtest xtest_t;
typedef struct _xtest_class xtest_class_t;

struct _xtest
{
  xobject_t object;
  xint_t id;
  xint_t dummy;

  xint_t count;
  xint_t setcount;
};

struct _xtest_class
{
  xobject_class_t parent_class;
};

static xtype_t xtest_get_type (void);
XDEFINE_TYPE (xtest, xtest, XTYPE_OBJECT)

static xint_t stopping;  /* (atomic) */

static void xtest_get_property (xobject_t    *object,
				  xuint_t       prop_id,
				  xvalue_t     *value,
				  xparam_spec_t *pspec);
static void xtest_set_property (xobject_t      *object,
				  xuint_t         prop_id,
				  const xvalue_t *value,
				  xparam_spec_t   *pspec);

static void
xtest_class_init (xtest_class_t * klass)
{
  xobject_class_t *xobject_class;

  xobject_class = (xobject_class_t *) klass;

  xobject_class->get_property = xtest_get_property;
  xobject_class->set_property = xtest_set_property;

  xobject_class_install_property (xobject_class,
				   PROP_DUMMY,
				   xparam_spec_int ("dummy",
						     NULL,
						     NULL,
						     0, G_MAXINT, 0,
						     XPARAM_READWRITE));
}

static void
xtest_init (xtest_t * test)
{
  static xuint_t static_id = 1;
  test->id = static_id++;
}

static void
xtest_get_property (xobject_t    *object,
		      xuint_t       prop_id,
		      xvalue_t     *value,
		      xparam_spec_t *pspec)
{
  xtest_t *test;

  test = XTEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      xvalue_set_int (value, g_atomic_int_get (&test->dummy));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xtest_set_property (xobject_t      *object,
		      xuint_t         prop_id,
		      const xvalue_t *value,
		      xparam_spec_t   *pspec)
{
  xtest_t *test;

  test = XTEST (object);

  switch (prop_id)
    {
    case PROP_DUMMY:
      g_atomic_int_set (&test->dummy, xvalue_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
dummy_notify (xobject_t    *object,
	      xparam_spec_t *pspec)
{
  xtest_t *test;

  test = XTEST (object);

  g_atomic_int_inc (&test->count);
}

static void
xtest_do_property (xtest_t * test)
{
  xint_t dummy;

  g_atomic_int_inc (&test->setcount);

  xobject_get (test, "dummy", &dummy, NULL);
  xobject_set (test, "dummy", dummy + 1, NULL);
}

static xpointer_t
run_thread (xtest_t * test)
{
  xint_t i = 1;

  while (!g_atomic_int_get (&stopping)) {
    xtest_do_property (test);
    if ((i++ % 10000) == 0)
      {
	g_print (".%c", 'a' + test->id);
	xthread_yield(); /* force context switch */
      }
  }

  return NULL;
}

int
main (int argc, char **argv)
{
  xint_t i;
  xtest_t *test;
  xarray_t *test_threads;
  const xint_t n_threads = 5;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test = xobject_new (XTYPE_TEST, NULL);

  xassert (test->count == test->dummy);
  xsignal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL);

  test_threads = g_array_new (FALSE, FALSE, sizeof (xthread_t *));

  g_atomic_int_set (&stopping, 0);

  for (i = 0; i < n_threads; i++) {
    xthread_t *thread;

    thread = xthread_create ((GThreadFunc) run_thread, test, TRUE, NULL);
    g_array_append_val (test_threads, thread);
  }
  g_usleep (30000000);

  g_atomic_int_set (&stopping, 1);
  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < n_threads; i++) {
    xthread_t *thread;

    thread = g_array_index (test_threads, xthread_t *, i);
    xthread_join (thread);
  }

  g_print ("stopped\n");

  g_print ("%d %d\n", test->setcount, test->count);

  g_array_free (test_threads, TRUE);
  xobject_unref (test);

  return 0;
}
