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
};

struct _xtest_class
{
  xobject_class_t parent_class;
};

static xtype_t xtest_get_type (void);

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
  xobject_class_t *gobject_class;

  gobject_class = (xobject_class_t *) klass;

  parent_class = xtype_class_ref (XTYPE_OBJECT);

  gobject_class->dispose = xtest_dispose;
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

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
xtest_do_refcount (xtest_t * test)
{
  static xuint_t i = 1;
  if (i++ % 100000 == 0)
    g_print (".");
  xobject_ref (test);
  xobject_unref (test);
}

int
main (int argc, char **argv)
{
  xint_t i;
  xtest_t *test;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test = xobject_new (XTYPE_TEST, NULL);

  for (i=0; i<100000000; i++) {
    xtest_do_refcount (test);
  }

  xobject_unref (test);

  g_print ("\n");

  return 0;
}
