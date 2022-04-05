#include <glib.h>
#include <glib-object.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#define XTYPE_TEST                (my_test_get_type ())
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
  static xuint_t i = 1;
  if (i++ % 100000 == 0)
    g_print (".");
  g_object_ref (test);
  g_object_unref (test);
}

int
main (int argc, char **argv)
{
  xint_t i;
  GTest *test;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  test = g_object_new (XTYPE_TEST, NULL);

  for (i=0; i<100000000; i++) {
    my_test_do_refcount (test);
  }

  g_object_unref (test);

  g_print ("\n");

  return 0;
}
