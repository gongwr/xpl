#include <glib-object.h>

static void
test_fundamentals (void)
{
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_NONE));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_INTERFACE));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_CHAR));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_UCHAR));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_BOOLEAN));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_INT));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_UINT));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_LONG));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_ULONG));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_INT64));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_UINT64));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_ENUM));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_FLAGS));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_FLOAT));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_DOUBLE));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_STRING));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_POINTER));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_BOXED));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_PARAM));
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_OBJECT));
  g_assert (XTYPE_OBJECT == xobject_get_type ());
  g_assert (XTYPE_IS_FUNDAMENTAL (XTYPE_VARIANT));
  g_assert (XTYPE_IS_DERIVED (XTYPE_INITIALLY_UNOWNED));

  g_assert (xtype_fundamental_next () == XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST));
}

static void
test_type_qdata (void)
{
  xchar_t *data;

  xtype_set_qdata (XTYPE_ENUM, g_quark_from_string ("bla"), "bla");
  data = xtype_get_qdata (XTYPE_ENUM, g_quark_from_string ("bla"));
  g_assert_cmpstr (data, ==, "bla");
}

static void
test_type_query (void)
{
  GTypeQuery query;

  xtype_query (XTYPE_ENUM, &query);
  g_assert_cmpint (query.type, ==, XTYPE_ENUM);
  g_assert_cmpstr (query.type_name, ==, "xenum_t");
  g_assert_cmpint (query.class_size, ==, sizeof (xenum_class_t));
  g_assert_cmpint (query.instance_size, ==, 0);
}

typedef struct _xobject xobject_t;
typedef struct _xobject_class xobject_class_t;
typedef struct _MyObjectClassPrivate MyObjectClassPrivate;

struct _xobject
{
  xobject_t parent_instance;

  xint_t count;
};

struct _xobject_class
{
  xobject_class_t parent_class;
};

struct _MyObjectClassPrivate
{
  xint_t secret_class_count;
};

static xtype_t my_object_get_type (void);
G_DEFINE_TYPE_WITH_CODE (xobject_t, my_object, XTYPE_OBJECT,
                         xtype_add_class_private (g_define_type_id, sizeof (MyObjectClassPrivate)) );

static void
my_object_init (xobject_t *obj)
{
  obj->count = 42;
}

static void
my_object_class_init (xobject_class_t *klass)
{
}

static void
test_class_private (void)
{
  xobject_t *obj;
  xobject_class_t *class;
  MyObjectClassPrivate *priv;

  obj = xobject_new (my_object_get_type (), NULL);

  class = xtype_class_ref (my_object_get_type ());
  priv = XTYPE_CLASS_GET_PRIVATE (class, my_object_get_type (), MyObjectClassPrivate);
  priv->secret_class_count = 13;
  xtype_class_unref (class);

  xobject_unref (obj);

  g_assert_cmpint (xtype_qname (my_object_get_type ()), ==, g_quark_from_string ("xobject_t"));
}

static void
test_clear (void)
{
  xobject_t *o = NULL;
  xobject_t *tmp;

  g_clear_object (&o);
  g_assert (o == NULL);

  tmp = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  o = xobject_ref (tmp);
  g_assert (o != NULL);

  g_assert_cmpint (tmp->ref_count, ==, 2);
  g_clear_object (&o);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  g_assert (o == NULL);

  xobject_unref (tmp);
}

static void
test_clear_function (void)
{
  xobject_t *o = NULL;
  xobject_t *tmp;

  (g_clear_object) (&o);
  g_assert (o == NULL);

  tmp = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  o = xobject_ref (tmp);
  g_assert (o != NULL);

  g_assert_cmpint (tmp->ref_count, ==, 2);
  (g_clear_object) (&o);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  g_assert (o == NULL);

  xobject_unref (tmp);
}

static void
test_set (void)
{
  xobject_t *o = NULL;
  xobject_t *tmp;
  xpointer_t tmp_weak = NULL;

  g_assert (!g_set_object (&o, NULL));
  g_assert (o == NULL);

  tmp = xobject_new (XTYPE_OBJECT, NULL);
  tmp_weak = tmp;
  xobject_add_weak_pointer (tmp, &tmp_weak);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  g_assert (g_set_object (&o, tmp));
  g_assert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 2);

  xobject_unref (tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  /* Setting it again shouldn’t cause finalisation. */
  g_assert (!g_set_object (&o, tmp));
  g_assert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  g_assert_nonnull (tmp_weak);

  g_assert (g_set_object (&o, NULL));
  g_assert (o == NULL);
  g_assert_null (tmp_weak);
}

static void
test_set_function (void)
{
  xobject_t *o = NULL;
  xobject_t *tmp;
  xpointer_t tmp_weak = NULL;

  g_assert (!(g_set_object) (&o, NULL));
  g_assert (o == NULL);

  tmp = xobject_new (XTYPE_OBJECT, NULL);
  tmp_weak = tmp;
  xobject_add_weak_pointer (tmp, &tmp_weak);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  g_assert ((g_set_object) (&o, tmp));
  g_assert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 2);

  xobject_unref (tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  /* Setting it again shouldn’t cause finalisation. */
  g_assert (!(g_set_object) (&o, tmp));
  g_assert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  g_assert_nonnull (tmp_weak);

  g_assert ((g_set_object) (&o, NULL));
  g_assert (o == NULL);
  g_assert_null (tmp_weak);
}

static void
test_set_derived_type (void)
{
  xbinding_t *obj = NULL;
  xobject_t *o = NULL;
  xbinding_t *b = NULL;

  g_test_summary ("Check that g_set_object() doesn’t give strict aliasing "
                  "warnings when used on types derived from xobject_t");

  g_assert_false (g_set_object (&o, NULL));
  g_assert_null (o);

  g_assert_false (g_set_object (&b, NULL));
  g_assert_null (b);

  obj = xobject_new (my_object_get_type (), NULL);

  g_assert_true (g_set_object (&o, G_OBJECT (obj)));
  g_assert_true (o == G_OBJECT (obj));

  g_assert_true (g_set_object (&b, obj));
  g_assert_true (b == obj);

  xobject_unref (obj);
  g_clear_object (&b);
  g_clear_object (&o);
}

static void
toggle_cb (xpointer_t data, xobject_t *obj, xboolean_t is_last)
{
  xboolean_t *b = data;

  *b = TRUE;
}

static void
test_object_value (void)
{
  xobject_t *v;
  xobject_t *v2;
  xvalue_t value = G_VALUE_INIT;
  xboolean_t toggled = FALSE;

  xvalue_init (&value, XTYPE_OBJECT);

  v = xobject_new (XTYPE_OBJECT, NULL);
  xobject_add_toggle_ref (v, toggle_cb, &toggled);

  xvalue_take_object (&value, v);

  v2 = xvalue_get_object (&value);
  g_assert (v2 == v);

  v2 = xvalue_dup_object (&value);
  g_assert (v2 == v);  /* objects use ref/unref for copy/free */
  xobject_unref (v2);

  g_assert (!toggled);
  xvalue_unset (&value);
  g_assert (toggled);

  /* test the deprecated variant too */
  xvalue_init (&value, XTYPE_OBJECT);
  /* get a new reference */
  xobject_ref (v);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  xvalue_set_object_take_ownership (&value, v);
G_GNUC_END_IGNORE_DEPRECATIONS

  toggled = FALSE;
  xvalue_unset (&value);
  g_assert (toggled);

  xobject_remove_toggle_ref (v, toggle_cb, &toggled);
}

static void
test_initially_unowned (void)
{
  xobject_t *obj;

  obj = xobject_new (XTYPE_INITIALLY_UNOWNED, NULL);
  g_assert (xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_ref_sink (obj);
  g_assert (!xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_ref_sink (obj);
  g_assert (!xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 2);

  xobject_unref (obj);
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_force_floating (obj);
  g_assert (xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_ref_sink (obj);
  xobject_unref (obj);

  obj = xobject_new (XTYPE_INITIALLY_UNOWNED, NULL);
  g_assert_true (xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_take_ref (obj);
  g_assert_false (xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_take_ref (obj);
  g_assert_false (xobject_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_unref (obj);
}

static void
test_weak_pointer (void)
{
  xobject_t *obj;
  xpointer_t weak;
  xpointer_t weak2;

  weak = weak2 = obj = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_add_weak_pointer (obj, &weak);
  xobject_add_weak_pointer (obj, &weak2);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert (weak == obj);
  g_assert (weak2 == obj);

  xobject_remove_weak_pointer (obj, &weak2);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert (weak == obj);
  g_assert (weak2 == obj);

  xobject_unref (obj);
  g_assert (weak == NULL);
  g_assert (weak2 == obj);
}

static void
test_weak_pointer_clear (void)
{
  xobject_t *obj;
  xpointer_t weak = NULL;

  g_clear_weak_pointer (&weak);
  g_assert_null (weak);

  weak = obj = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_add_weak_pointer (obj, &weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  g_clear_weak_pointer (&weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_null (weak);

  xobject_unref (obj);
}

static void
test_weak_pointer_clear_function (void)
{
  xobject_t *obj;
  xpointer_t weak = NULL;

  (g_clear_weak_pointer) (&weak);
  g_assert_null (weak);

  weak = obj = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  xobject_add_weak_pointer (obj, &weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  (g_clear_weak_pointer) (&weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_null (weak);

  xobject_unref (obj);
}

static void
test_weak_pointer_set (void)
{
  xobject_t *obj;
  xpointer_t weak = NULL;

  g_assert_false (g_set_weak_pointer (&weak, NULL));
  g_assert_null (weak);

  obj = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  g_assert_true (g_set_weak_pointer (&weak, obj));
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  g_assert_true (g_set_weak_pointer (&weak, NULL));
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_null (weak);

  g_assert_true (g_set_weak_pointer (&weak, obj));
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  xobject_unref (obj);
  g_assert_null (weak);
}

static void
test_weak_pointer_set_function (void)
{
  xobject_t *obj;
  xpointer_t weak = NULL;

  g_assert_false ((g_set_weak_pointer) (&weak, NULL));
  g_assert_null (weak);

  obj = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  g_assert_true ((g_set_weak_pointer) (&weak, obj));
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  g_assert_true ((g_set_weak_pointer) (&weak, NULL));
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_null (weak);

  g_assert_true ((g_set_weak_pointer) (&weak, obj));
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  xobject_unref (obj);
  g_assert_null (weak);
}

/* See gobject/tests/threadtests.c for the threaded version */
static void
test_weak_ref (void)
{
  xobject_t *obj;
  xobject_t *obj2;
  xobject_t *tmp;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  GWeakRef weak2 = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  GWeakRef weak3 = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  GWeakRef *dynamic_weak = g_new (GWeakRef, 1);

  /* you can initialize to empty like this... */
  g_weak_ref_init (&weak2, NULL);
  g_assert (g_weak_ref_get (&weak2) == NULL);

  /* ... or via an initializer */
  g_weak_ref_init (&weak3, NULL);
  g_assert (g_weak_ref_get (&weak3) == NULL);

  obj = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  obj2 = xobject_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj2->ref_count, ==, 1);

  /* you can init with an object (even if uninitialized) */
  g_weak_ref_init (&weak, obj);
  g_weak_ref_init (dynamic_weak, obj);
  /* or set to point at an object, if initialized (maybe to 0) */
  g_weak_ref_set (&weak2, obj);
  g_weak_ref_set (&weak3, obj);
  /* none of this affects its refcount */
  g_assert_cmpint (obj->ref_count, ==, 1);

  /* getting the value takes a ref */
  tmp = g_weak_ref_get (&weak);
  g_assert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  tmp = g_weak_ref_get (&weak2);
  g_assert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  tmp = g_weak_ref_get (&weak3);
  g_assert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  tmp = g_weak_ref_get (dynamic_weak);
  g_assert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  /* clearing a weak ref stops tracking */
  g_weak_ref_clear (&weak);

  /* setting a weak ref to NULL stops tracking too */
  g_weak_ref_set (&weak2, NULL);
  g_assert (g_weak_ref_get (&weak2) == NULL);
  g_weak_ref_clear (&weak2);

  /* setting a weak ref to a new object stops tracking the old one */
  g_weak_ref_set (dynamic_weak, obj2);
  tmp = g_weak_ref_get (dynamic_weak);
  g_assert (tmp == obj2);
  g_assert_cmpint (obj2->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj2->ref_count, ==, 1);

  g_assert_cmpint (obj->ref_count, ==, 1);

  /* free the object: weak3 is the only one left pointing there */
  xobject_unref (obj);
  g_assert (g_weak_ref_get (&weak3) == NULL);

  /* setting a weak ref to a new object stops tracking the old one */
  g_weak_ref_set (dynamic_weak, obj2);
  tmp = g_weak_ref_get (dynamic_weak);
  g_assert (tmp == obj2);
  g_assert_cmpint (obj2->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj2->ref_count, ==, 1);

  g_weak_ref_clear (&weak3);

  /* unset dynamic_weak... */
  g_weak_ref_set (dynamic_weak, NULL);
  g_assert_null (g_weak_ref_get (dynamic_weak));

  /* initializing a weak reference to an object that had before works */
  g_weak_ref_set (dynamic_weak, obj2);
  tmp = g_weak_ref_get (dynamic_weak);
  g_assert_true (tmp == obj2);
  g_assert_cmpint (obj2->ref_count, ==, 2);
  xobject_unref (tmp);
  g_assert_cmpint (obj2->ref_count, ==, 1);

  /* clear and free dynamic_weak... */
  g_weak_ref_clear (dynamic_weak);

  /* ... to prove that doing so stops this from being a use-after-free */
  xobject_unref (obj2);
  g_free (dynamic_weak);
}

G_DECLARE_FINAL_TYPE (WeakReffedObject, weak_reffed_object,
                      WEAK, REFFED_OBJECT, xobject)

struct _WeakReffedObject
{
  xobject_t parent;

  GWeakRef *weak_ref;
};

G_DEFINE_TYPE (WeakReffedObject, weak_reffed_object, XTYPE_OBJECT)

static void
weak_reffed_object_dispose (xobject_t *object)
{
  WeakReffedObject *weak_reffed = WEAK_REFFED_OBJECT (object);

  g_assert_cmpint (object->ref_count, ==, 1);

  g_weak_ref_set (weak_reffed->weak_ref, object);

  G_OBJECT_CLASS (weak_reffed_object_parent_class)->dispose (object);

  g_assert_null (g_weak_ref_get (weak_reffed->weak_ref));
}

static void
weak_reffed_object_init (WeakReffedObject *connector)
{
}

static void
weak_reffed_object_class_init (WeakReffedObjectClass *klass)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = weak_reffed_object_dispose;
}

static void
test_weak_ref_on_dispose (void)
{
  WeakReffedObject *obj;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2390");
  g_test_summary ("test_t that a weak ref set during dispose vfunc is cleared");

  g_weak_ref_init (&weak, NULL);

  obj = xobject_new (weak_reffed_object_get_type (), NULL);
  obj->weak_ref = &weak;

  g_assert_cmpint (G_OBJECT (obj)->ref_count, ==, 1);
  g_clear_object (&obj);

  g_assert_null (g_weak_ref_get (&weak));
}

static void
test_weak_ref_on_run_dispose (void)
{
  xobject_t *obj;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/865");
  g_test_summary ("test_t that a weak ref is cleared on xobject_run_dispose()");

  obj = xobject_new (XTYPE_OBJECT, NULL);
  g_weak_ref_init (&weak, obj);

  g_assert_true (obj == g_weak_ref_get (&weak));
  xobject_unref (obj);

  xobject_run_dispose (obj);
  g_assert_null (g_weak_ref_get (&weak));

  g_clear_object (&obj);
  g_assert_null (g_weak_ref_get (&weak));
}

static void
on_weak_ref_toggle_notify (xpointer_t data,
                           xobject_t *object,
                           xboolean_t is_last_ref)
{
  GWeakRef *weak = data;

  if (is_last_ref)
    g_weak_ref_set (weak, object);
}

static void
on_weak_ref_toggle_notify_disposed (xpointer_t data,
                                    xobject_t *object)
{
  g_assert_cmpint (object->ref_count, ==, 1);

  xobject_ref (object);
  xobject_unref (object);
}

static void
test_weak_ref_on_toggle_notify (void)
{
  xobject_t *obj;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2390");
  g_test_summary ("test_t that a weak ref set on toggle notify is cleared");

  g_weak_ref_init (&weak, NULL);

  obj = xobject_new (XTYPE_OBJECT, NULL);
  xobject_add_toggle_ref (obj, on_weak_ref_toggle_notify, &weak);
  xobject_weak_ref (obj, on_weak_ref_toggle_notify_disposed, NULL);
  xobject_unref (obj);

  g_assert_cmpint (obj->ref_count, ==, 1);
  g_clear_object (&obj);

  g_assert_null (g_weak_ref_get (&weak));
}

typedef struct
{
  xboolean_t should_be_last;
  xint_t count;
} Count;

static void
toggle_notify (xpointer_t  data,
               xobject_t  *obj,
               xboolean_t  is_last)
{
  Count *c = data;

  g_assert (is_last == c->should_be_last);

  c->count++;
}

static void
test_toggle_ref (void)
{
  xobject_t *obj;
  Count c, c2;

  obj = xobject_new (XTYPE_OBJECT, NULL);

  xobject_add_toggle_ref (obj, toggle_notify, &c);
  xobject_add_toggle_ref (obj, toggle_notify, &c2);

  c.should_be_last = c2.should_be_last = TRUE;
  c.count = c2.count = 0;

  xobject_unref (obj);

  g_assert_cmpint (c.count, ==, 0);
  g_assert_cmpint (c2.count, ==, 0);

  xobject_ref (obj);

  g_assert_cmpint (c.count, ==, 0);
  g_assert_cmpint (c2.count, ==, 0);

  xobject_remove_toggle_ref (obj, toggle_notify, &c2);

  xobject_unref (obj);

  g_assert_cmpint (c.count, ==, 1);

  c.should_be_last = FALSE;

  xobject_ref (obj);

  g_assert_cmpint (c.count, ==, 2);

  c.should_be_last = TRUE;

  xobject_unref (obj);

  g_assert_cmpint (c.count, ==, 3);

  xobject_remove_toggle_ref (obj, toggle_notify, &c);
}

static xboolean_t global_destroyed;
static xint_t global_value;

static void
data_destroy (xpointer_t data)
{
  g_assert_cmpint (GPOINTER_TO_INT (data), ==, global_value);

  global_destroyed = TRUE;
}

static void
test_object_qdata (void)
{
  xobject_t *obj;
  xpointer_t v;
  xquark quark;

  obj = xobject_new (XTYPE_OBJECT, NULL);

  global_value = 1;
  global_destroyed = FALSE;
  xobject_set_data_full (obj, "test", GINT_TO_POINTER (1), data_destroy);
  v = xobject_get_data (obj, "test");
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 1);
  xobject_set_data_full (obj, "test", GINT_TO_POINTER (2), data_destroy);
  g_assert (global_destroyed);
  global_value = 2;
  global_destroyed = FALSE;
  v = xobject_steal_data (obj, "test");
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 2);
  g_assert (!global_destroyed);

  global_value = 1;
  global_destroyed = FALSE;
  quark = g_quark_from_string ("test");
  xobject_set_qdata_full (obj, quark, GINT_TO_POINTER (1), data_destroy);
  v = xobject_get_qdata (obj, quark);
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 1);
  xobject_set_qdata_full (obj, quark, GINT_TO_POINTER (2), data_destroy);
  g_assert (global_destroyed);
  global_value = 2;
  global_destroyed = FALSE;
  v = xobject_steal_qdata (obj, quark);
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 2);
  g_assert (!global_destroyed);

  xobject_set_qdata_full (obj, quark, GINT_TO_POINTER (3), data_destroy);
  global_value = 3;
  global_destroyed = FALSE;
  xobject_unref (obj);

  g_assert (global_destroyed);
}

typedef struct {
  const xchar_t *value;
  xint_t refcount;
} Value;

static xpointer_t
ref_value (xpointer_t value, xpointer_t user_data)
{
  Value *v = value;
  Value **old_value_p = user_data;

  if (old_value_p)
    *old_value_p = v;

  if (v)
    v->refcount += 1;

  return value;
}

static void
unref_value (xpointer_t value)
{
  Value *v = value;

  v->refcount -= 1;
  if (v->refcount == 0)
    g_free (value);
}

static
Value *
new_value (const xchar_t *s)
{
  Value *v;

  v = g_new (Value, 1);
  v->value = s;
  v->refcount = 1;

  return v;
}

static void
test_object_qdata2 (void)
{
  xobject_t *obj;
  Value *v, *v1, *v2, *v3, *old_val;
  xdestroy_notify_t old_destroy;
  xboolean_t res;

  obj = xobject_new (XTYPE_OBJECT, NULL);

  v1 = new_value ("bla");

  xobject_set_data_full (obj, "test", v1, unref_value);

  v = xobject_get_data (obj, "test");
  g_assert_cmpstr (v->value, ==, "bla");
  g_assert_cmpint (v->refcount, ==, 1);

  v = xobject_dup_data (obj, "test", ref_value, &old_val);
  g_assert (old_val == v1);
  g_assert_cmpstr (v->value, ==, "bla");
  g_assert_cmpint (v->refcount, ==, 2);
  unref_value (v);

  v = xobject_dup_data (obj, "nono", ref_value, &old_val);
  g_assert (old_val == NULL);
  g_assert (v == NULL);

  v2 = new_value ("not");

  res = xobject_replace_data (obj, "test", v1, v2, unref_value, &old_destroy);
  g_assert (res == TRUE);
  g_assert (old_destroy == unref_value);
  g_assert_cmpstr (v1->value, ==, "bla");
  g_assert_cmpint (v1->refcount, ==, 1);

  v = xobject_get_data (obj, "test");
  g_assert_cmpstr (v->value, ==, "not");
  g_assert_cmpint (v->refcount, ==, 1);

  v3 = new_value ("xyz");
  res = xobject_replace_data (obj, "test", v1, v3, unref_value, &old_destroy);
  g_assert (res == FALSE);
  g_assert_cmpstr (v2->value, ==, "not");
  g_assert_cmpint (v2->refcount, ==, 1);

  unref_value (v1);

  res = xobject_replace_data (obj, "test", NULL, v3, unref_value, &old_destroy);
  g_assert (res == FALSE);
  g_assert_cmpstr (v2->value, ==, "not");
  g_assert_cmpint (v2->refcount, ==, 1);

  res = xobject_replace_data (obj, "test", v2, NULL, unref_value, &old_destroy);
  g_assert (res == TRUE);
  g_assert (old_destroy == unref_value);
  g_assert_cmpstr (v2->value, ==, "not");
  g_assert_cmpint (v2->refcount, ==, 1);

  unref_value (v2);

  v = xobject_get_data (obj, "test");
  g_assert (v == NULL);

  res = xobject_replace_data (obj, "test", NULL, v3, unref_value, &old_destroy);
  g_assert (res == TRUE);

  v = xobject_get_data (obj, "test");
  g_assert (v == v3);

  ref_value (v3, NULL);
  g_assert_cmpint (v3->refcount, ==, 2);
  xobject_unref (obj);
  g_assert_cmpint (v3->refcount, ==, 1);
  unref_value (v3);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/type/fundamentals", test_fundamentals);
  g_test_add_func ("/type/qdata", test_type_qdata);
  g_test_add_func ("/type/query", test_type_query);
  g_test_add_func ("/type/class-private", test_class_private);
  g_test_add_func ("/object/clear", test_clear);
  g_test_add_func ("/object/clear-function", test_clear_function);
  g_test_add_func ("/object/set", test_set);
  g_test_add_func ("/object/set-function", test_set_function);
  g_test_add_func ("/object/set/derived-type", test_set_derived_type);
  g_test_add_func ("/object/value", test_object_value);
  g_test_add_func ("/object/initially-unowned", test_initially_unowned);
  g_test_add_func ("/object/weak-pointer", test_weak_pointer);
  g_test_add_func ("/object/weak-pointer/clear", test_weak_pointer_clear);
  g_test_add_func ("/object/weak-pointer/clear-function", test_weak_pointer_clear_function);
  g_test_add_func ("/object/weak-pointer/set", test_weak_pointer_set);
  g_test_add_func ("/object/weak-pointer/set-function", test_weak_pointer_set_function);
  g_test_add_func ("/object/weak-ref", test_weak_ref);
  g_test_add_func ("/object/weak-ref/on-dispose", test_weak_ref_on_dispose);
  g_test_add_func ("/object/weak-ref/on-run-dispose", test_weak_ref_on_run_dispose);
  g_test_add_func ("/object/weak-ref/on-toggle-notify", test_weak_ref_on_toggle_notify);
  g_test_add_func ("/object/toggle-ref", test_toggle_ref);
  g_test_add_func ("/object/qdata", test_object_qdata);
  g_test_add_func ("/object/qdata2", test_object_qdata2);

  return g_test_run ();
}
