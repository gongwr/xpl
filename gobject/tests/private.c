/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib-object.h>

typedef struct {
  xobject_t parent_instance;
} test_object_t;

typedef struct {
  int dummy_0;
  float dummy_1;
} test_object_private_t;

typedef struct {
  xobject_class_t parent_class;
} test_object_class_t;

xtype_t test_object_get_type (void);

G_DEFINE_TYPE_WITH_CODE (test_object, test_object, XTYPE_OBJECT,
                         G_ADD_PRIVATE (test_object))

static void
test_object_class_init (test_object_class_t *klass)
{
}

static void
test_object_init (test_object_t *self)
{
  test_object_private_t *priv = test_object_get_instance_private (self);

  if (g_test_verbose ())
    g_printerr ("Offset of %sPrivate for type '%s': %d\n",
             G_OBJECT_TYPE_NAME (self),
             G_OBJECT_TYPE_NAME (self),
             test_object_private_offset);

  priv->dummy_0 = 42;
  priv->dummy_1 = 3.14159f;
}

static int
test_object_get_dummy_0 (test_object_t *self)
{
  test_object_private_t *priv = test_object_get_instance_private (self);

  return priv->dummy_0;
}

static float
test_object_get_dummy_1 (test_object_t *self)
{
  test_object_private_t *priv = test_object_get_instance_private (self);

  return priv->dummy_1;
}

typedef struct {
  test_object_t parent_instance;
} test_derived_t;

typedef struct {
  char *dummy_2;
} test_derived_private_t;

typedef struct {
  test_object_class_t parent_class;
} test_derived_class_t;

xtype_t test_derived_get_type (void);

G_DEFINE_TYPE_WITH_CODE (test_derived, test_derived, test_object_get_type (),
                         G_ADD_PRIVATE (test_derived))

static void
test_derived_finalize (xobject_t *obj)
{
  test_derived_private_t *priv = test_derived_get_instance_private ((test_derived_t *) obj);

  g_free (priv->dummy_2);

  XOBJECT_CLASS (test_derived_parent_class)->finalize (obj);
}

static void
test_derived_class_init (test_derived_class_t *klass)
{
  XOBJECT_CLASS (klass)->finalize = test_derived_finalize;
}

static void
test_derived_init (test_derived_t *self)
{
  test_derived_private_t *priv = test_derived_get_instance_private (self);

  if (g_test_verbose ())
    g_printerr ("Offset of %sPrivate for type '%s': %d\n",
             G_OBJECT_TYPE_NAME (self),
             G_OBJECT_TYPE_NAME (self),
             test_derived_private_offset);

  priv->dummy_2 = xstrdup ("Hello");
}

static const char *
test_derived_get_dummy_2 (test_derived_t *self)
{
  test_derived_private_t *priv = test_derived_get_instance_private (self);

  return priv->dummy_2;
}

typedef struct {
  test_object_t parent_instance;
} test_mixed_t;

typedef struct {
  xint_t dummy_3;
} test_mixed_private_t;

typedef struct {
  test_object_class_t parent_class;
} test_mixed_class_t;

xtype_t test_mixed_get_type (void);

XDEFINE_TYPE (test_mixed, test_mixed, test_object_get_type ())

static void
test_mixed_class_init (test_mixed_class_t *klass)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  xtype_class_add_private (klass, sizeof (test_mixed_private_t));
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
test_mixed_init (test_mixed_t *self)
{
  test_mixed_private_t *priv = XTYPE_INSTANCE_GET_PRIVATE (self, test_mixed_get_type (), test_mixed_private_t);

  if (g_test_verbose ())
    g_printerr ("Offset of %sPrivate for type '%s': %d\n",
             G_OBJECT_TYPE_NAME (self),
             G_OBJECT_TYPE_NAME (self),
             test_mixed_private_offset);

  priv->dummy_3 = 47;
}

static xint_t
test_mixed_get_dummy_3 (test_mixed_t *self)
{
  test_mixed_private_t *priv = XTYPE_INSTANCE_GET_PRIVATE (self, test_mixed_get_type (), test_mixed_private_t);

  return priv->dummy_3;
}

typedef struct {
  test_mixed_t parent_instance;
} test_mixed_derived_t;

typedef struct {
  sint64_t dummy_4;
} test_mixed_derived_private_t;

typedef struct {
  test_mixed_class_t parent_class;
} test_mixed_derived_class_t;

xtype_t test_mixed_derived_get_type (void);

G_DEFINE_TYPE_WITH_CODE (test_mixed_derived, test_mixed_derived, test_mixed_get_type (),
                         G_ADD_PRIVATE (test_mixed_derived))

static void
test_mixed_derived_class_init (test_mixed_derived_class_t *klass)
{
}

static void
test_mixed_derived_init (test_mixed_derived_t *self)
{
  test_mixed_derived_private_t *priv = test_mixed_derived_get_instance_private (self);

  if (g_test_verbose ())
    g_printerr ("Offset of %sPrivate for type '%s': %d\n",
             G_OBJECT_TYPE_NAME (self),
             G_OBJECT_TYPE_NAME (self),
             test_mixed_derived_private_offset);

  priv->dummy_4 = g_get_monotonic_time ();
}

static sint64_t
test_mixed_derived_get_dummy_4 (test_mixed_derived_t *self)
{
  test_mixed_derived_private_t *priv = test_mixed_derived_get_instance_private (self);

  return priv->dummy_4;
}

static void
private_instance (void)
{
  test_object_t *obj = xobject_new (test_object_get_type (), NULL);
  xpointer_t class;
  xint_t offset;

  g_assert_cmpint (test_object_get_dummy_0 (obj), ==, 42);
  g_assert_cmpfloat (test_object_get_dummy_1 (obj), ==, 3.14159f);

  class = xtype_class_ref (test_object_get_type ());
  offset = xtype_class_get_instance_private_offset (class);
  xtype_class_unref (class);

  xassert (offset == test_object_private_offset);

  xobject_unref (obj);
}

static void
private_derived_instance (void)
{
  test_derived_t *obj = xobject_new (test_derived_get_type (), NULL);

  g_assert_cmpstr (test_derived_get_dummy_2 (obj), ==, "Hello");
  g_assert_cmpint (test_object_get_dummy_0 ((test_object_t *) obj), ==, 42);

  xobject_unref (obj);
}

static void
private_mixed_derived_instance (void)
{
  test_mixed_derived_t *derived = xobject_new (test_mixed_derived_get_type (), NULL);
  test_mixed_t *mixed = xobject_new (test_mixed_get_type (), NULL);

  g_assert_cmpint (test_mixed_get_dummy_3 (mixed), ==, 47);
  xassert (test_mixed_derived_get_dummy_4 (derived) <= g_get_monotonic_time ());

  xobject_unref (derived);
  xobject_unref (mixed);
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/private/instance", private_instance);
  g_test_add_func ("/private/derived-instance", private_derived_instance);
  g_test_add_func ("/private/mixed-derived-instance", private_mixed_derived_instance);

  return g_test_run ();
}
