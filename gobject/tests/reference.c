#include <glib-object.h>

static void
test_fundamentals (void)
{
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_NONE));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_INTERFACE));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_CHAR));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_UCHAR));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_BOOLEAN));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_INT));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_UINT));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_LONG));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_ULONG));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_INT64));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_UINT64));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_ENUM));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_FLAGS));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_FLOAT));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_DOUBLE));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_STRING));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_POINTER));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_BOXED));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_PARAM));
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_OBJECT));
  xassert (XTYPE_OBJECT == xobject_get_type ());
  xassert (XTYPE_IS_FUNDAMENTAL (XTYPE_VARIANT));
  xassert (XTYPE_IS_DERIVED (XTYPE_INITIALLY_UNOWNED));

  xassert (xtype_fundamental_next () == XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST));
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

typedef struct _my_object my_object_t;
typedef struct _my_object_class my_object_class_t;
typedef struct _my_object_class_private my_object_class_private_t;

struct _my_object
{
  my_object_t parent_instance;

  xint_t count;
};

struct _my_object_class
{
  my_object_class_t parent_class;
};

struct _my_object_class_private
{
  xint_t secret_class_count;
};

static xtype_t my_object_get_type (void);
G_DEFINE_TYPE_WITH_CODE (my_object, my_object, XTYPE_OBJECT,
                         xtype_add_class_private (g_define_type_id, sizeof (my_object_class_private_t)) );

static void
my_object_init (my_object_t *obj)
{
  obj->count = 42;
}

static void
my_object_class_init (my_object_class_t *klass)
{
}

static void
test_class_private (void)
{
  my_object_t *obj;
  my_object_class_t *class;
  my_object_class_private_t *priv;

  obj = my_object_new (my_object_get_type (), NULL);

  class = xtype_class_ref (my_object_get_type ());
  priv = XTYPE_CLASS_GET_PRIVATE (class, my_object_get_type (), my_object_class_private_t);
  priv->secret_class_count = 13;
  xtype_class_unref (class);

  my_object_unref (obj);

  g_assert_cmpint (xtype_qname (my_object_get_type ()), ==, g_quark_from_string ("my_object_t"));
}

static void
test_clear (void)
{
  my_object_t *o = NULL;
  my_object_t *tmp;

  g_clear_object (&o);
  xassert (o == NULL);

  tmp = my_object_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  o = my_object_ref (tmp);
  xassert (o != NULL);

  g_assert_cmpint (tmp->ref_count, ==, 2);
  g_clear_object (&o);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  xassert (o == NULL);

  my_object_unref (tmp);
}

static void
test_clear_function (void)
{
  my_object_t *o = NULL;
  my_object_t *tmp;

  (g_clear_object) (&o);
  xassert (o == NULL);

  tmp = my_object_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  o = my_object_ref (tmp);
  xassert (o != NULL);

  g_assert_cmpint (tmp->ref_count, ==, 2);
  (g_clear_object) (&o);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  xassert (o == NULL);

  my_object_unref (tmp);
}

static void
test_set (void)
{
  my_object_t *o = NULL;
  my_object_t *tmp;
  xpointer_t tmp_weak = NULL;

  xassert (!g_set_object (&o, NULL));
  xassert (o == NULL);

  tmp = my_object_new (XTYPE_OBJECT, NULL);
  tmp_weak = tmp;
  my_object_add_weak_pointer (tmp, &tmp_weak);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  xassert (g_set_object (&o, tmp));
  xassert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 2);

  my_object_unref (tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  /* Setting it again shouldn’t cause finalisation. */
  xassert (!g_set_object (&o, tmp));
  xassert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  g_assert_nonnull (tmp_weak);

  xassert (g_set_object (&o, NULL));
  xassert (o == NULL);
  g_assert_null (tmp_weak);
}

static void
test_set_function (void)
{
  my_object_t *o = NULL;
  my_object_t *tmp;
  xpointer_t tmp_weak = NULL;

  xassert (!(g_set_object) (&o, NULL));
  xassert (o == NULL);

  tmp = my_object_new (XTYPE_OBJECT, NULL);
  tmp_weak = tmp;
  my_object_add_weak_pointer (tmp, &tmp_weak);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  xassert ((g_set_object) (&o, tmp));
  xassert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 2);

  my_object_unref (tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);

  /* Setting it again shouldn’t cause finalisation. */
  xassert (!(g_set_object) (&o, tmp));
  xassert (o == tmp);
  g_assert_cmpint (tmp->ref_count, ==, 1);
  g_assert_nonnull (tmp_weak);

  xassert ((g_set_object) (&o, NULL));
  xassert (o == NULL);
  g_assert_null (tmp_weak);
}

static void
test_set_derived_type (void)
{
  xbinding_t *obj = NULL;
  my_object_t *o = NULL;
  xbinding_t *b = NULL;

  g_test_summary ("Check that g_set_object() doesn’t give strict aliasing "
                  "warnings when used on types derived from my_object_t");

  g_assert_false (g_set_object (&o, NULL));
  g_assert_null (o);

  g_assert_false (g_set_object (&b, NULL));
  g_assert_null (b);

  obj = my_object_new (my_object_get_type (), NULL);

  g_assert_true (g_set_object (&o, G_OBJECT (obj)));
  g_assert_true (o == G_OBJECT (obj));

  g_assert_true (g_set_object (&b, obj));
  g_assert_true (b == obj);

  my_object_unref (obj);
  g_clear_object (&b);
  g_clear_object (&o);
}

static void
toggle_cb (xpointer_t data, my_object_t *obj, xboolean_t is_last)
{
  xboolean_t *b = data;

  *b = TRUE;
}

static void
test_object_value (void)
{
  my_object_t *v;
  my_object_t *v2;
  xvalue_t value = G_VALUE_INIT;
  xboolean_t toggled = FALSE;

  xvalue_init (&value, XTYPE_OBJECT);

  v = my_object_new (XTYPE_OBJECT, NULL);
  my_object_add_toggle_ref (v, toggle_cb, &toggled);

  xvalue_take_object (&value, v);

  v2 = xvalue_get_object (&value);
  xassert (v2 == v);

  v2 = xvalue_dup_object (&value);
  xassert (v2 == v);  /* objects use ref/unref for copy/free */
  my_object_unref (v2);

  xassert (!toggled);
  xvalue_unset (&value);
  xassert (toggled);

  /* test the deprecated variant too */
  xvalue_init (&value, XTYPE_OBJECT);
  /* get a new reference */
  my_object_ref (v);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  xvalue_set_object_take_ownership (&value, v);
G_GNUC_END_IGNORE_DEPRECATIONS

  toggled = FALSE;
  xvalue_unset (&value);
  xassert (toggled);

  my_object_remove_toggle_ref (v, toggle_cb, &toggled);
}

static void
test_initially_unowned (void)
{
  my_object_t *obj;

  obj = my_object_new (XTYPE_INITIALLY_UNOWNED, NULL);
  xassert (my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_ref_sink (obj);
  xassert (!my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_ref_sink (obj);
  xassert (!my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 2);

  my_object_unref (obj);
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_force_floating (obj);
  xassert (my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_ref_sink (obj);
  my_object_unref (obj);

  obj = my_object_new (XTYPE_INITIALLY_UNOWNED, NULL);
  g_assert_true (my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_take_ref (obj);
  g_assert_false (my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_take_ref (obj);
  g_assert_false (my_object_is_floating (obj));
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_unref (obj);
}

static void
test_weak_pointer (void)
{
  my_object_t *obj;
  xpointer_t weak;
  xpointer_t weak2;

  weak = weak2 = obj = my_object_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_add_weak_pointer (obj, &weak);
  my_object_add_weak_pointer (obj, &weak2);
  g_assert_cmpint (obj->ref_count, ==, 1);
  xassert (weak == obj);
  xassert (weak2 == obj);

  my_object_remove_weak_pointer (obj, &weak2);
  g_assert_cmpint (obj->ref_count, ==, 1);
  xassert (weak == obj);
  xassert (weak2 == obj);

  my_object_unref (obj);
  xassert (weak == NULL);
  xassert (weak2 == obj);
}

static void
test_weak_pointer_clear (void)
{
  my_object_t *obj;
  xpointer_t weak = NULL;

  g_clear_weak_pointer (&weak);
  g_assert_null (weak);

  weak = obj = my_object_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_add_weak_pointer (obj, &weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  g_clear_weak_pointer (&weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_null (weak);

  my_object_unref (obj);
}

static void
test_weak_pointer_clear_function (void)
{
  my_object_t *obj;
  xpointer_t weak = NULL;

  (g_clear_weak_pointer) (&weak);
  g_assert_null (weak);

  weak = obj = my_object_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  my_object_add_weak_pointer (obj, &weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_true (weak == obj);

  (g_clear_weak_pointer) (&weak);
  g_assert_cmpint (obj->ref_count, ==, 1);
  g_assert_null (weak);

  my_object_unref (obj);
}

static void
test_weak_pointer_set (void)
{
  my_object_t *obj;
  xpointer_t weak = NULL;

  g_assert_false (g_set_weak_pointer (&weak, NULL));
  g_assert_null (weak);

  obj = my_object_new (XTYPE_OBJECT, NULL);
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

  my_object_unref (obj);
  g_assert_null (weak);
}

static void
test_weak_pointer_set_function (void)
{
  my_object_t *obj;
  xpointer_t weak = NULL;

  g_assert_false ((g_set_weak_pointer) (&weak, NULL));
  g_assert_null (weak);

  obj = my_object_new (XTYPE_OBJECT, NULL);
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

  my_object_unref (obj);
  g_assert_null (weak);
}

/* See gobject/tests/threadtests.c for the threaded version */
static void
test_weak_ref (void)
{
  my_object_t *obj;
  my_object_t *obj2;
  my_object_t *tmp;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  GWeakRef weak2 = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  GWeakRef weak3 = { { GUINT_TO_POINTER (0xDEADBEEFU) } };
  GWeakRef *dynamic_weak = g_new (GWeakRef, 1);

  /* you can initialize to empty like this... */
  g_weak_ref_init (&weak2, NULL);
  xassert (g_weak_ref_get (&weak2) == NULL);

  /* ... or via an initializer */
  g_weak_ref_init (&weak3, NULL);
  xassert (g_weak_ref_get (&weak3) == NULL);

  obj = my_object_new (XTYPE_OBJECT, NULL);
  g_assert_cmpint (obj->ref_count, ==, 1);

  obj2 = my_object_new (XTYPE_OBJECT, NULL);
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
  xassert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  my_object_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  tmp = g_weak_ref_get (&weak2);
  xassert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  my_object_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  tmp = g_weak_ref_get (&weak3);
  xassert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  my_object_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  tmp = g_weak_ref_get (dynamic_weak);
  xassert (tmp == obj);
  g_assert_cmpint (obj->ref_count, ==, 2);
  my_object_unref (tmp);
  g_assert_cmpint (obj->ref_count, ==, 1);

  /* clearing a weak ref stops tracking */
  g_weak_ref_clear (&weak);

  /* setting a weak ref to NULL stops tracking too */
  g_weak_ref_set (&weak2, NULL);
  xassert (g_weak_ref_get (&weak2) == NULL);
  g_weak_ref_clear (&weak2);

  /* setting a weak ref to a new object stops tracking the old one */
  g_weak_ref_set (dynamic_weak, obj2);
  tmp = g_weak_ref_get (dynamic_weak);
  xassert (tmp == obj2);
  g_assert_cmpint (obj2->ref_count, ==, 2);
  my_object_unref (tmp);
  g_assert_cmpint (obj2->ref_count, ==, 1);

  g_assert_cmpint (obj->ref_count, ==, 1);

  /* free the object: weak3 is the only one left pointing there */
  my_object_unref (obj);
  xassert (g_weak_ref_get (&weak3) == NULL);

  /* setting a weak ref to a new object stops tracking the old one */
  g_weak_ref_set (dynamic_weak, obj2);
  tmp = g_weak_ref_get (dynamic_weak);
  xassert (tmp == obj2);
  g_assert_cmpint (obj2->ref_count, ==, 2);
  my_object_unref (tmp);
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
  my_object_unref (tmp);
  g_assert_cmpint (obj2->ref_count, ==, 1);

  /* clear and free dynamic_weak... */
  g_weak_ref_clear (dynamic_weak);

  /* ... to prove that doing so stops this from being a use-after-free */
  my_object_unref (obj2);
  g_free (dynamic_weak);
}

G_DECLARE_FINAL_TYPE (WeakReffedObject, weak_reffed_object,
                      WEAK, REFFED_OBJECT, my_object)

struct _WeakReffedObject
{
  my_object_t parent;

  GWeakRef *weak_ref;
};

XDEFINE_TYPE (WeakReffedObject, weak_reffed_object, XTYPE_OBJECT)

static void
weak_reffed_object_dispose (my_object_t *object)
{
  WeakReffedObject *weak_reffed = WEAK_REFFED_OBJECT (object);

  g_assert_cmpint (object->ref_count, ==, 1);

  g_weak_ref_set (weak_reffed->weak_ref, object);

  XOBJECT_CLASS (weak_reffed_object_parent_class)->dispose (object);

  g_assert_null (g_weak_ref_get (weak_reffed->weak_ref));
}

static void
weak_reffed_object_init (WeakReffedObject *connector)
{
}

static void
weak_reffed_object_class_init (WeakReffedObjectClass *klass)
{
  my_object_class_t *object_class = XOBJECT_CLASS (klass);

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

  obj = my_object_new (weak_reffed_object_get_type (), NULL);
  obj->weak_ref = &weak;

  g_assert_cmpint (G_OBJECT (obj)->ref_count, ==, 1);
  g_clear_object (&obj);

  g_assert_null (g_weak_ref_get (&weak));
}

static void
test_weak_ref_on_run_dispose (void)
{
  my_object_t *obj;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/865");
  g_test_summary ("test_t that a weak ref is cleared on my_object_run_dispose()");

  obj = my_object_new (XTYPE_OBJECT, NULL);
  g_weak_ref_init (&weak, obj);

  g_assert_true (obj == g_weak_ref_get (&weak));
  my_object_unref (obj);

  my_object_run_dispose (obj);
  g_assert_null (g_weak_ref_get (&weak));

  g_clear_object (&obj);
  g_assert_null (g_weak_ref_get (&weak));
}

static void
on_weak_ref_toggle_notify (xpointer_t data,
                           my_object_t *object,
                           xboolean_t is_last_ref)
{
  GWeakRef *weak = data;

  if (is_last_ref)
    g_weak_ref_set (weak, object);
}

static void
on_weak_ref_toggle_notify_disposed (xpointer_t data,
                                    my_object_t *object)
{
  g_assert_cmpint (object->ref_count, ==, 1);

  my_object_ref (object);
  my_object_unref (object);
}

static void
test_weak_ref_on_toggle_notify (void)
{
  my_object_t *obj;
  GWeakRef weak = { { GUINT_TO_POINTER (0xDEADBEEFU) } };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2390");
  g_test_summary ("test_t that a weak ref set on toggle notify is cleared");

  g_weak_ref_init (&weak, NULL);

  obj = my_object_new (XTYPE_OBJECT, NULL);
  my_object_add_toggle_ref (obj, on_weak_ref_toggle_notify, &weak);
  my_object_weak_ref (obj, on_weak_ref_toggle_notify_disposed, NULL);
  my_object_unref (obj);

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
               my_object_t  *obj,
               xboolean_t  is_last)
{
  Count *c = data;

  xassert (is_last == c->should_be_last);

  c->count++;
}

static void
test_toggle_ref (void)
{
  my_object_t *obj;
  Count c, c2;

  obj = my_object_new (XTYPE_OBJECT, NULL);

  my_object_add_toggle_ref (obj, toggle_notify, &c);
  my_object_add_toggle_ref (obj, toggle_notify, &c2);

  c.should_be_last = c2.should_be_last = TRUE;
  c.count = c2.count = 0;

  my_object_unref (obj);

  g_assert_cmpint (c.count, ==, 0);
  g_assert_cmpint (c2.count, ==, 0);

  my_object_ref (obj);

  g_assert_cmpint (c.count, ==, 0);
  g_assert_cmpint (c2.count, ==, 0);

  my_object_remove_toggle_ref (obj, toggle_notify, &c2);

  my_object_unref (obj);

  g_assert_cmpint (c.count, ==, 1);

  c.should_be_last = FALSE;

  my_object_ref (obj);

  g_assert_cmpint (c.count, ==, 2);

  c.should_be_last = TRUE;

  my_object_unref (obj);

  g_assert_cmpint (c.count, ==, 3);

  my_object_remove_toggle_ref (obj, toggle_notify, &c);
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
  my_object_t *obj;
  xpointer_t v;
  xquark quark;

  obj = my_object_new (XTYPE_OBJECT, NULL);

  global_value = 1;
  global_destroyed = FALSE;
  my_object_set_data_full (obj, "test", GINT_TO_POINTER (1), data_destroy);
  v = my_object_get_data (obj, "test");
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 1);
  my_object_set_data_full (obj, "test", GINT_TO_POINTER (2), data_destroy);
  xassert (global_destroyed);
  global_value = 2;
  global_destroyed = FALSE;
  v = my_object_steal_data (obj, "test");
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 2);
  xassert (!global_destroyed);

  global_value = 1;
  global_destroyed = FALSE;
  quark = g_quark_from_string ("test");
  my_object_set_qdata_full (obj, quark, GINT_TO_POINTER (1), data_destroy);
  v = my_object_get_qdata (obj, quark);
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 1);
  my_object_set_qdata_full (obj, quark, GINT_TO_POINTER (2), data_destroy);
  xassert (global_destroyed);
  global_value = 2;
  global_destroyed = FALSE;
  v = my_object_steal_qdata (obj, quark);
  g_assert_cmpint (GPOINTER_TO_INT (v), ==, 2);
  xassert (!global_destroyed);

  my_object_set_qdata_full (obj, quark, GINT_TO_POINTER (3), data_destroy);
  global_value = 3;
  global_destroyed = FALSE;
  my_object_unref (obj);

  xassert (global_destroyed);
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
  my_object_t *obj;
  Value *v, *v1, *v2, *v3, *old_val;
  xdestroy_notify_t old_destroy;
  xboolean_t res;

  obj = my_object_new (XTYPE_OBJECT, NULL);

  v1 = new_value ("bla");

  my_object_set_data_full (obj, "test", v1, unref_value);

  v = my_object_get_data (obj, "test");
  g_assert_cmpstr (v->value, ==, "bla");
  g_assert_cmpint (v->refcount, ==, 1);

  v = my_object_dup_data (obj, "test", ref_value, &old_val);
  xassert (old_val == v1);
  g_assert_cmpstr (v->value, ==, "bla");
  g_assert_cmpint (v->refcount, ==, 2);
  unref_value (v);

  v = my_object_dup_data (obj, "nono", ref_value, &old_val);
  xassert (old_val == NULL);
  xassert (v == NULL);

  v2 = new_value ("not");

  res = my_object_replace_data (obj, "test", v1, v2, unref_value, &old_destroy);
  xassert (res == TRUE);
  xassert (old_destroy == unref_value);
  g_assert_cmpstr (v1->value, ==, "bla");
  g_assert_cmpint (v1->refcount, ==, 1);

  v = my_object_get_data (obj, "test");
  g_assert_cmpstr (v->value, ==, "not");
  g_assert_cmpint (v->refcount, ==, 1);

  v3 = new_value ("xyz");
  res = my_object_replace_data (obj, "test", v1, v3, unref_value, &old_destroy);
  xassert (res == FALSE);
  g_assert_cmpstr (v2->value, ==, "not");
  g_assert_cmpint (v2->refcount, ==, 1);

  unref_value (v1);

  res = my_object_replace_data (obj, "test", NULL, v3, unref_value, &old_destroy);
  xassert (res == FALSE);
  g_assert_cmpstr (v2->value, ==, "not");
  g_assert_cmpint (v2->refcount, ==, 1);

  res = my_object_replace_data (obj, "test", v2, NULL, unref_value, &old_destroy);
  xassert (res == TRUE);
  xassert (old_destroy == unref_value);
  g_assert_cmpstr (v2->value, ==, "not");
  g_assert_cmpint (v2->refcount, ==, 1);

  unref_value (v2);

  v = my_object_get_data (obj, "test");
  xassert (v == NULL);

  res = my_object_replace_data (obj, "test", NULL, v3, unref_value, &old_destroy);
  xassert (res == TRUE);

  v = my_object_get_data (obj, "test");
  xassert (v == v3);

  ref_value (v3, NULL);
  g_assert_cmpint (v3->refcount, ==, 2);
  my_object_unref (obj);
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
