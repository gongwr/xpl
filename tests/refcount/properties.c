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
};

struct _xtest_class
{
  xobject_class_t parent_class;
};

static xtype_t xtest_get_type (void);
static xboolean_t stopping;

static void xtest_class_init (xtest_class_t * klass);
static void xtest_init (xtest_t * test);
static void xtest_dispose (xobject_t * object);
static void xtest_get_property (xobject_t    *object,
                                  xuint_t       prop_id,
                                  xvalue_t     *value,
                                  xparam_spec_t *pspec);
static void xtest_set_property (xobject_t      *object,
                                  xuint_t         prop_id,
                                  const xvalue_t *value,
                                  xparam_spec_t   *pspec);

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

    test_type = xtype_register_static (XTYPE_OBJECT, "xtest_t", &test_info, 0);
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
  gobject_class->get_property = xtest_get_property;
  gobject_class->set_property = xtest_set_property;

  xobject_class_install_property (gobject_class,
				   PROP_DUMMY,
				   g_param_spec_int ("dummy",
						     NULL,
						     NULL,
						     0, G_MAXINT, 0,
						     G_PARAM_READWRITE));
}

static void
xtest_init (xtest_t * test)
{
  static xuint_t static_id = 1;
  test->id = static_id++;
}

static void
xtest_dispose (xobject_t * object)
{
  G_OBJECT_CLASS (parent_class)->dispose (object);
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
      xvalue_set_int (value, test->dummy);
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
      test->dummy = xvalue_get_int (value);
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

  test->count++;
}

static void
xtest_do_property (xtest_t * test)
{
  xint_t dummy;

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
#define N_THREADS 5
  xthread_t *test_threads[N_THREADS];
  xtest_t *test_objects[N_THREADS];
  xint_t i;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  for (i = 0; i < N_THREADS; i++) {
    xtest_t *test;

    test = xobject_new (XTYPE_TEST, NULL);
    test_objects[i] = test;

    g_assert (test->count == test->dummy);
    xsignal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL);
  }

  g_atomic_int_set (&stopping, FALSE);

  for (i = 0; i < N_THREADS; i++)
    test_threads[i] = xthread_create ((GThreadFunc) run_thread, test_objects[i], TRUE, NULL);

  g_usleep (3000000);

  g_atomic_int_set (&stopping, TRUE);
  g_print ("\nstopping\n");

  /* join all threads */
  for (i = 0; i < N_THREADS; i++)
    xthread_join (test_threads[i]);

  g_print ("stopped\n");

  for (i = 0; i < N_THREADS; i++) {
    xtest_t *test = test_objects[i];

    g_assert (test->count == test->dummy);
    xobject_unref (test);
  }

  return 0;
}
