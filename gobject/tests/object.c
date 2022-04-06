#include <glib-object.h>

/* --------------------------------- */
/* test_object_constructor_singleton */

typedef xobject_t my_singleton_object_t;
typedef xobject_class_t my_singleton_object_class_t;

xtype_t my_singleton_object_get_type (void);

G_DEFINE_TYPE (my_singleton_object, my_singleton_object, XTYPE_OBJECT)

static my_singleton_object_t *singleton;

static void
my_singleton_object_init (my_singleton_object_t *obj)
{
}

static xobject_t *
my_singleton_object_constructor (xtype_t                  type,
                                 xuint_t                  n_construct_properties,
                                 GObjectConstructParam *construct_params)
{
  xobject_t *object;

  if (singleton)
    return xobject_ref (singleton);

  object = G_OBJECT_CLASS (my_singleton_object_parent_class)->
    constructor (type, n_construct_properties, construct_params);
  singleton = (my_singleton_object_t *)object;

  return object;
}

static void
my_singleton_object_finalize (my_singleton_object_t *obj)
{
  singleton = NULL;

  G_OBJECT_CLASS (my_singleton_object_parent_class)->finalize (obj);
}

static void
my_singleton_object_class_init (my_singleton_object_class_t *klass)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = my_singleton_object_constructor;
  object_class->finalize = my_singleton_object_finalize;
}

static void
test_object_constructor_singleton (void)
{
  my_singleton_object_t *one, *two, *three;

  one = xobject_new (my_singleton_object_get_type (), NULL);
  g_assert_cmpint (G_OBJECT (one)->ref_count, ==, 1);

  two = xobject_new (my_singleton_object_get_type (), NULL);
  g_assert (one == two);
  g_assert_cmpint (G_OBJECT (two)->ref_count, ==, 2);

  three = xobject_new (my_singleton_object_get_type (), NULL);
  g_assert (one == three);
  g_assert_cmpint (G_OBJECT (three)->ref_count, ==, 3);

  xobject_add_weak_pointer (G_OBJECT (one), (xpointer_t *)&one);

  xobject_unref (one);
  g_assert (one != NULL);

  xobject_unref (three);
  xobject_unref (two);

  g_assert (one == NULL);
}

/* ----------------------------------- */
/* test_object_constructor_infanticide */

typedef xobject_t my_infanticide_object_t;
typedef xobject_class_t my_infanticide_object_class_t;

xtype_t my_infanticide_object_get_type (void);

G_DEFINE_TYPE (my_infanticide_object, my_infanticide_object, XTYPE_OBJECT)

static void
my_infanticide_object_init (my_infanticide_object_t *obj)
{
}

static xobject_t *
my_infanticide_object_constructor (xtype_t                  type,
                                   xuint_t                  n_construct_properties,
                                   GObjectConstructParam *construct_params)
{
  xobject_t *object;

  object = G_OBJECT_CLASS (my_infanticide_object_parent_class)->
    constructor (type, n_construct_properties, construct_params);

  xobject_unref (object);

  return NULL;
}

static void
my_infanticide_object_class_init (my_infanticide_object_class_t *klass)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = my_infanticide_object_constructor;
}

static void
test_object_constructor_infanticide (void)
{
  xobject_t *obj;
  int i;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=661576");

  for (i = 0; i < 1000; i++)
    {
      g_test_expect_message ("GLib-xobject_t", G_LOG_LEVEL_CRITICAL,
                             "*finalized while still in-construction*");
      g_test_expect_message ("GLib-xobject_t", G_LOG_LEVEL_CRITICAL,
                             "*Custom constructor*returned NULL*");
      obj = xobject_new (my_infanticide_object_get_type (), NULL);
      g_assert_null (obj);
      g_test_assert_expected_messages ();
    }
}

/* --------------------------------- */

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/object/constructor/singleton", test_object_constructor_singleton);
  g_test_add_func ("/object/constructor/infanticide", test_object_constructor_infanticide);

  return g_test_run ();
}
