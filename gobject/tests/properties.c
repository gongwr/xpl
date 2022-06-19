#include <stdlib.h>
#include <gstdio.h>
#include <glib-object.h>

typedef struct _test_object {
  xobject_t parent_instance;
  xint_t foo;
  xboolean_t bar;
  xchar_t *baz;
  xchar_t *quux;
} test_object_t;

typedef struct _test_object_class {
  xobject_class_t parent_class;
} test_object_class_t;

enum { PROP_0, PROP_FOO, PROP_BAR, PROP_BAZ, PROP_QUUX, N_PROPERTIES };

static xparam_spec_t *properties[N_PROPERTIES] = { NULL, };

static xtype_t test_object_get_type (void);
XDEFINE_TYPE (test_object, test_object, XTYPE_OBJECT)

static void
test_object_set_foo (test_object_t *obj,
                     xint_t        foo)
{
  if (obj->foo != foo)
    {
      obj->foo = foo;

      xassert (properties[PROP_FOO] != NULL);
      xobject_notify_by_pspec (G_OBJECT (obj), properties[PROP_FOO]);
    }
}

static void
test_object_set_bar (test_object_t *obj,
                     xboolean_t    bar)
{
  bar = !!bar;

  if (obj->bar != bar)
    {
      obj->bar = bar;

      xassert (properties[PROP_BAR] != NULL);
      xobject_notify_by_pspec (G_OBJECT (obj), properties[PROP_BAR]);
    }
}

static void
test_object_set_baz (test_object_t  *obj,
                     const xchar_t *baz)
{
  if (xstrcmp0 (obj->baz, baz) != 0)
    {
      g_free (obj->baz);
      obj->baz = xstrdup (baz);

      xassert (properties[PROP_BAZ] != NULL);
      xobject_notify_by_pspec (G_OBJECT (obj), properties[PROP_BAZ]);
    }
}

static void
test_object_set_quux (test_object_t  *obj,
                      const xchar_t *quux)
{
  if (xstrcmp0 (obj->quux, quux) != 0)
    {
      g_free (obj->quux);
      obj->quux = xstrdup (quux);

      xassert (properties[PROP_QUUX] != NULL);
      xobject_notify_by_pspec (G_OBJECT (obj), properties[PROP_QUUX]);
    }
}

static void
test_object_finalize (xobject_t *gobject)
{
  test_object_t *self = (test_object_t *) gobject;

  g_free (self->baz);
  g_free (self->quux);

  /* When the ref_count of an object is zero it is still
   * possible to notify the property, but it should do
   * nothing and silently quit (bug #705570)
   */
  xobject_notify (gobject, "foo");
  xobject_notify_by_pspec (gobject, properties[PROP_BAR]);

  XOBJECT_CLASS (test_object_parent_class)->finalize (gobject);
}

static void
test_object_set_property (xobject_t *gobject,
                          xuint_t prop_id,
                          const xvalue_t *value,
                          xparam_spec_t *pspec)
{
  test_object_t *tobj = (test_object_t *) gobject;

  g_assert_cmpint (prop_id, !=, 0);
  g_assert_cmpint (prop_id, !=, N_PROPERTIES);
  xassert (pspec == properties[prop_id]);

  switch (prop_id)
    {
    case PROP_FOO:
      test_object_set_foo (tobj, xvalue_get_int (value));
      break;

    case PROP_BAR:
      test_object_set_bar (tobj, xvalue_get_boolean (value));
      break;

    case PROP_BAZ:
      test_object_set_baz (tobj, xvalue_get_string (value));
      break;

    case PROP_QUUX:
      test_object_set_quux (tobj, xvalue_get_string (value));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
test_object_get_property (xobject_t *gobject,
                          xuint_t prop_id,
                          xvalue_t *value,
                          xparam_spec_t *pspec)
{
  test_object_t *tobj = (test_object_t *) gobject;

  g_assert_cmpint (prop_id, !=, 0);
  g_assert_cmpint (prop_id, !=, N_PROPERTIES);
  xassert (pspec == properties[prop_id]);

  switch (prop_id)
    {
    case PROP_FOO:
      xvalue_set_int (value, tobj->foo);
      break;

    case PROP_BAR:
      xvalue_set_boolean (value, tobj->bar);
      break;

    case PROP_BAZ:
      xvalue_set_string (value, tobj->baz);
      break;

    case PROP_QUUX:
      xvalue_set_string (value, tobj->quux);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
test_object_class_init (test_object_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  properties[PROP_FOO] = xparam_spec_int ("foo", "foo_t", "foo_t",
                                           -1, G_MAXINT,
                                           0,
                                           XPARAM_READWRITE);
  properties[PROP_BAR] = xparam_spec_boolean ("bar", "Bar", "Bar",
                                               FALSE,
                                               XPARAM_READWRITE);
  properties[PROP_BAZ] = xparam_spec_string ("baz", "baz_t", "baz_t",
                                              NULL,
                                              XPARAM_READWRITE);
  properties[PROP_QUUX] = xparam_spec_string ("quux", "quux", "quux",
                                               NULL,
                                               XPARAM_READWRITE | XPARAM_EXPLICIT_NOTIFY);

  xobject_class->set_property = test_object_set_property;
  xobject_class->get_property = test_object_get_property;
  xobject_class->finalize = test_object_finalize;

  xobject_class_install_properties (xobject_class, N_PROPERTIES, properties);
}

static void
test_object_init (test_object_t *self)
{
  self->foo = 42;
  self->bar = TRUE;
  self->baz = xstrdup ("Hello");
  self->quux = NULL;
}

static void
properties_install (void)
{
  test_object_t *obj = xobject_new (test_object_get_type (), NULL);
  xparam_spec_t *pspec;

  xassert (properties[PROP_FOO] != NULL);

  pspec = xobject_class_find_property (G_OBJECT_GET_CLASS (obj), "foo");
  xassert (properties[PROP_FOO] == pspec);

  xobject_unref (obj);
}

typedef struct {
  const xchar_t *name;
  xparam_spec_t *pspec;
  xboolean_t    fired;
} TestNotifyClosure;

static void
on_notify (xobject_t           *gobject,
           xparam_spec_t        *pspec,
           TestNotifyClosure *closure)
{
  xassert (closure->pspec == pspec);
  g_assert_cmpstr (closure->name, ==, pspec->name);
  closure->fired = TRUE;
}

static void
properties_notify (void)
{
  test_object_t *obj = xobject_new (test_object_get_type (), NULL);
  TestNotifyClosure closure;

  xassert (properties[PROP_FOO] != NULL);
  xassert (properties[PROP_QUUX] != NULL);
  xsignal_connect (obj, "notify", G_CALLBACK (on_notify), &closure);

  closure.name = "foo";
  closure.pspec = properties[PROP_FOO];

  closure.fired = FALSE;
  xobject_set (obj, "foo", 47, NULL);
  xassert (closure.fired);

  closure.name = "baz";
  closure.pspec = properties[PROP_BAZ];

  closure.fired = FALSE;
  xobject_set (obj, "baz", "something new", NULL);
  xassert (closure.fired);

  /* baz lacks explicit notify, so we will see this twice */
  closure.fired = FALSE;
  xobject_set (obj, "baz", "something new", NULL);
  xassert (closure.fired);

  /* quux on the other hand, ... */
  closure.name = "quux";
  closure.pspec = properties[PROP_QUUX];

  closure.fired = FALSE;
  xobject_set (obj, "quux", "something new", NULL);
  xassert (closure.fired);

  /* no change; no notify */
  closure.fired = FALSE;
  xobject_set (obj, "quux", "something new", NULL);
  xassert (!closure.fired);


  xobject_unref (obj);
}

typedef struct {
  xparam_spec_t *pspec[3];
  xint_t pos;
} Notifys;

static void
on_notify2 (xobject_t    *gobject,
            xparam_spec_t *pspec,
            Notifys    *n)
{
  xassert (n->pspec[n->pos] == pspec);
  n->pos++;
}

static void
properties_notify_queue (void)
{
  test_object_t *obj = xobject_new (test_object_get_type (), NULL);
  Notifys n;

  xassert (properties[PROP_FOO] != NULL);

  n.pspec[0] = properties[PROP_BAZ];
  n.pspec[1] = properties[PROP_BAR];
  n.pspec[2] = properties[PROP_FOO];
  n.pos = 0;

  xsignal_connect (obj, "notify", G_CALLBACK (on_notify2), &n);

  xobject_freeze_notify (G_OBJECT (obj));
  xobject_set (obj, "foo", 47, NULL);
  xobject_set (obj, "bar", TRUE, "foo", 42, "baz", "abc", NULL);
  xobject_thaw_notify (G_OBJECT (obj));
  xassert (n.pos == 3);

  xobject_unref (obj);
}

static void
properties_construct (void)
{
  test_object_t *obj;
  xint_t val;
  xboolean_t b;
  xchar_t *s;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=630357");

  /* more than 16 args triggers a realloc in xobject_new_valist() */
  obj = xobject_new (test_object_get_type (),
                      "foo", 1,
                      "foo", 2,
                      "foo", 3,
                      "foo", 4,
                      "foo", 5,
                      "bar", FALSE,
                      "foo", 6,
                      "foo", 7,
                      "foo", 8,
                      "foo", 9,
                      "foo", 10,
                      "baz", "boo",
                      "foo", 11,
                      "foo", 12,
                      "foo", 13,
                      "foo", 14,
                      "foo", 15,
                      "foo", 16,
                      "foo", 17,
                      "foo", 18,
                      NULL);

  xobject_get (obj, "foo", &val, NULL);
  xassert (val == 18);
  xobject_get (obj, "bar", &b, NULL);
  xassert (!b);
  xobject_get (obj, "baz", &s, NULL);
  g_assert_cmpstr (s, ==, "boo");
  g_free (s);

  xobject_unref (obj);
}

static void
properties_testv_with_no_properties (void)
{
  test_object_t *test_obj;
  const char *prop_names[4] = { "foo", "bar", "baz", "quux" };
  xvalue_t values_out[4] = { G_VALUE_INIT };
  xuint_t i;

  /* test_t newv_with_properties && getv */
  test_obj = (test_object_t *) xobject_new_with_properties (
      test_object_get_type (), 0, NULL, NULL);
  xobject_getv (G_OBJECT (test_obj), 4, prop_names, values_out);

  /* It should have init values */
  g_assert_cmpint (xvalue_get_int (&values_out[0]), ==, 42);
  g_assert_true (xvalue_get_boolean (&values_out[1]));
  g_assert_cmpstr (xvalue_get_string (&values_out[2]), ==, "Hello");
  g_assert_cmpstr (xvalue_get_string (&values_out[3]), ==, NULL);

  for (i = 0; i < 4; i++)
    xvalue_unset (&values_out[i]);
  xobject_unref (test_obj);
}

static void
properties_testv_with_valid_properties (void)
{
  test_object_t *test_obj;
  const char *prop_names[4] = { "foo", "bar", "baz", "quux" };

  xvalue_t values_in[4] = { G_VALUE_INIT };
  xvalue_t values_out[4] = { G_VALUE_INIT };
  xuint_t i;

  xvalue_init (&(values_in[0]), XTYPE_INT);
  xvalue_set_int (&(values_in[0]), 100);

  xvalue_init (&(values_in[1]), XTYPE_BOOLEAN);
  xvalue_set_boolean (&(values_in[1]), TRUE);

  xvalue_init (&(values_in[2]), XTYPE_STRING);
  xvalue_set_string (&(values_in[2]), "pigs");

  xvalue_init (&(values_in[3]), XTYPE_STRING);
  xvalue_set_string (&(values_in[3]), "fly");

  /* test_t newv_with_properties && getv */
  test_obj = (test_object_t *) xobject_new_with_properties (
      test_object_get_type (), 4, prop_names, values_in);
  xobject_getv (G_OBJECT (test_obj), 4, prop_names, values_out);

  g_assert_cmpint (xvalue_get_int (&values_out[0]), ==, 100);
  g_assert_true (xvalue_get_boolean (&values_out[1]));
  g_assert_cmpstr (xvalue_get_string (&values_out[2]), ==, "pigs");
  g_assert_cmpstr (xvalue_get_string (&values_out[3]), ==, "fly");

  for (i = 0; i < G_N_ELEMENTS (values_out); i++)
    xvalue_unset (&values_out[i]);

  /* test_t newv2 && getv */
  xvalue_set_string (&(values_in[2]), "Elmo knows");
  xvalue_set_string (&(values_in[3]), "where you live");
  xobject_setv (G_OBJECT (test_obj), 4, prop_names, values_in);

  xobject_getv (G_OBJECT (test_obj), 4, prop_names, values_out);

  g_assert_cmpint (xvalue_get_int (&values_out[0]), ==, 100);
  g_assert_true (xvalue_get_boolean (&values_out[1]));

  g_assert_cmpstr (xvalue_get_string (&values_out[2]), ==, "Elmo knows");
  g_assert_cmpstr (xvalue_get_string (&values_out[3]), ==, "where you live");

  for (i = 0; i < G_N_ELEMENTS (values_in); i++)
    xvalue_unset (&values_in[i]);
  for (i = 0; i < G_N_ELEMENTS (values_out); i++)
    xvalue_unset (&values_out[i]);

  xobject_unref (test_obj);
}

static void
properties_testv_with_invalid_property_type (void)
{
  if (g_test_subprocess ())
    {
      test_object_t *test_obj;
      const char *invalid_prop_names[1] = { "foo" };
      xvalue_t values_in[1] = { G_VALUE_INIT };

      xvalue_init (&(values_in[0]), XTYPE_STRING);
      xvalue_set_string (&(values_in[0]), "fly");

      test_obj = (test_object_t *) xobject_new_with_properties (
          test_object_get_type (), 1, invalid_prop_names, values_in);
      /* should give a warning */

      xobject_unref (test_obj);
    }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*foo*xint_t*gchararray*");
}


static void
properties_testv_with_invalid_property_names (void)
{
  if (g_test_subprocess ())
    {
      test_object_t *test_obj;
      const char *invalid_prop_names[4] = { "foo", "boo", "moo", "poo" };
      xvalue_t values_in[4] = { G_VALUE_INIT };

      xvalue_init (&(values_in[0]), XTYPE_INT);
      xvalue_set_int (&(values_in[0]), 100);

      xvalue_init (&(values_in[1]), XTYPE_BOOLEAN);
      xvalue_set_boolean (&(values_in[1]), TRUE);

      xvalue_init (&(values_in[2]), XTYPE_STRING);
      xvalue_set_string (&(values_in[2]), "pigs");

      xvalue_init (&(values_in[3]), XTYPE_STRING);
      xvalue_set_string (&(values_in[3]), "fly");

      test_obj = (test_object_t *) xobject_new_with_properties (
          test_object_get_type (), 4, invalid_prop_names, values_in);
      /* This call should give 3 Critical warnings. Actually, a critical warning
       * shouldn't make xobject_new_with_properties to fail when a bad named
       * property is given, because, it will just ignore that property. However,
       * for test purposes, it is considered that the test doesn't pass.
       */

      xobject_unref (test_obj);
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*xobject_new_is_valid_property*boo*");
}

static void
properties_testv_getv (void)
{
  test_object_t *test_obj;
  const char *prop_names[4] = { "foo", "bar", "baz", "quux" };
  xvalue_t values_out_initialized[4] = { G_VALUE_INIT };
  xvalue_t values_out_uninitialized[4] = { G_VALUE_INIT };
  xuint_t i;

  xvalue_init (&(values_out_initialized[0]), XTYPE_INT);
  xvalue_init (&(values_out_initialized[1]), XTYPE_BOOLEAN);
  xvalue_init (&(values_out_initialized[2]), XTYPE_STRING);
  xvalue_init (&(values_out_initialized[3]), XTYPE_STRING);

  test_obj = (test_object_t *) xobject_new_with_properties (
      test_object_get_type (), 0, NULL, NULL);

  /* test_t xobject_getv for an initialized values array */
  xobject_getv (G_OBJECT (test_obj), 4, prop_names, values_out_initialized);
  /* It should have init values */
  g_assert_cmpint (xvalue_get_int (&values_out_initialized[0]), ==, 42);
  g_assert_true (xvalue_get_boolean (&values_out_initialized[1]));
  g_assert_cmpstr (xvalue_get_string (&values_out_initialized[2]), ==, "Hello");
  g_assert_cmpstr (xvalue_get_string (&values_out_initialized[3]), ==, NULL);

  /* test_t xobject_getv for an uninitialized values array */
  xobject_getv (G_OBJECT (test_obj), 4, prop_names, values_out_uninitialized);
  /* It should have init values */
  g_assert_cmpint (xvalue_get_int (&values_out_uninitialized[0]), ==, 42);
  g_assert_true (xvalue_get_boolean (&values_out_uninitialized[1]));
  g_assert_cmpstr (xvalue_get_string (&values_out_uninitialized[2]), ==, "Hello");
  g_assert_cmpstr (xvalue_get_string (&values_out_uninitialized[3]), ==, NULL);

  for (i = 0; i < 4; i++)
    {
      xvalue_unset (&values_out_initialized[i]);
      xvalue_unset (&values_out_uninitialized[i]);
    }
  xobject_unref (test_obj);
}

static void
properties_get_property (void)
{
  test_object_t *test_obj;
  struct {
    const char *name;
    xtype_t gtype;
    xvalue_t value;
  } test_props[] = {
    { "foo", XTYPE_INT, G_VALUE_INIT },
    { "bar", XTYPE_INVALID, G_VALUE_INIT },
    { "bar", XTYPE_STRING, G_VALUE_INIT },
  };
  xsize_t i;

  g_test_summary ("xobject_get_property() accepts uninitialized, "
                  "initialized, and transformable values");

  for (i = 0; i < G_N_ELEMENTS (test_props); i++)
    {
      if (test_props[i].gtype != XTYPE_INVALID)
        xvalue_init (&(test_props[i].value), test_props[i].gtype);
    }

  test_obj = (test_object_t *) xobject_new_with_properties (test_object_get_type (), 0, NULL, NULL);

  g_test_message ("test_t xobject_get_property with an initialized value");
  xobject_get_property (G_OBJECT (test_obj), test_props[0].name, &(test_props[0].value));
  g_assert_cmpint (xvalue_get_int (&(test_props[0].value)), ==, 42);

  g_test_message ("test_t xobject_get_property with an uninitialized value");
  xobject_get_property (G_OBJECT (test_obj), test_props[1].name, &(test_props[1].value));
  g_assert_true (xvalue_get_boolean (&(test_props[1].value)));

  g_test_message ("test_t xobject_get_property with a transformable value");
  xobject_get_property (G_OBJECT (test_obj), test_props[2].name, &(test_props[2].value));
  g_assert_true (G_VALUE_HOLDS_STRING (&(test_props[2].value)));
  g_assert_cmpstr (xvalue_get_string (&(test_props[2].value)), ==, "TRUE");

  for (i = 0; i < G_N_ELEMENTS (test_props); i++)
    xvalue_unset (&(test_props[i].value));

  xobject_unref (test_obj);
}

static void
properties_testv_notify_queue (void)
{
  test_object_t *test_obj;
  const char *prop_names[3] = { "foo", "bar", "baz" };
  xvalue_t values_in[3] = { G_VALUE_INIT };
  Notifys n;
  xuint_t i;

  xvalue_init (&(values_in[0]), XTYPE_INT);
  xvalue_set_int (&(values_in[0]), 100);

  xvalue_init (&(values_in[1]), XTYPE_BOOLEAN);
  xvalue_set_boolean (&(values_in[1]), TRUE);

  xvalue_init (&(values_in[2]), XTYPE_STRING);
  xvalue_set_string (&(values_in[2]), "");

  /* test_t newv_with_properties && getv */
  test_obj = (test_object_t *) xobject_new_with_properties (
      test_object_get_type (), 0, NULL, NULL);

  g_assert_nonnull (properties[PROP_FOO]);

  n.pspec[0] = properties[PROP_BAZ];
  n.pspec[1] = properties[PROP_BAR];
  n.pspec[2] = properties[PROP_FOO];
  n.pos = 0;

  xsignal_connect (test_obj, "notify", G_CALLBACK (on_notify2), &n);

  xobject_freeze_notify (G_OBJECT (test_obj));
  {
    xobject_setv (G_OBJECT (test_obj), 3, prop_names, values_in);

    /* Set "foo" to 70 */
    xvalue_set_int (&(values_in[0]), 100);
    xobject_setv (G_OBJECT (test_obj), 1, prop_names, values_in);
  }
  xobject_thaw_notify (G_OBJECT (test_obj));
  g_assert_cmpint (n.pos, ==, 3);

  for (i = 0; i < 3; i++)
    xvalue_unset (&values_in[i]);
  xobject_unref (test_obj);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/properties/install", properties_install);
  g_test_add_func ("/properties/notify", properties_notify);
  g_test_add_func ("/properties/notify-queue", properties_notify_queue);
  g_test_add_func ("/properties/construct", properties_construct);
  g_test_add_func ("/properties/get-property", properties_get_property);

  g_test_add_func ("/properties/testv_with_no_properties",
      properties_testv_with_no_properties);
  g_test_add_func ("/properties/testv_with_valid_properties",
      properties_testv_with_valid_properties);
  g_test_add_func ("/properties/testv_with_invalid_property_type",
      properties_testv_with_invalid_property_type);
  g_test_add_func ("/properties/testv_with_invalid_property_names",
      properties_testv_with_invalid_property_names);
  g_test_add_func ("/properties/testv_getv", properties_testv_getv);
  g_test_add_func ("/properties/testv_notify_queue",
      properties_testv_notify_queue);

  return g_test_run ();
}
