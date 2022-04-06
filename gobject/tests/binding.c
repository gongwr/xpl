#include <stdlib.h>
#include <gstdio.h>
#include <glib-object.h>

typedef struct {
  xtype_interface_t x_iface;
} foo_interface_t;

xtype_t foo_get_type (void);

G_DEFINE_INTERFACE (foo, foo, XTYPE_OBJECT)

static void
foo_default_init (foo_interface_t *iface)
{
}

typedef struct {
  xobject_t parent;
} baa_t;

typedef struct {
  xobject_class_t parent_class;
} baa_class_t;

static void
baa_init_foo (foo_interface_t *iface)
{
}

xtype_t baa_get_type (void);

G_DEFINE_TYPE_WITH_CODE (baa, baa, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (foo_get_type (), baa_init_foo))

static void
baa_init (baa_t *baa)
{
}

static void
baa_class_init (baa_class_t *class)
{
}

typedef struct _BindingSource
{
  xobject_t parent_instance;

  xint_t foo;
  xint_t bar;
  xdouble_t double_value;
  xboolean_t toggle;
  xpointer_t item;
} binding_source_t;

typedef struct _BindingSourceClass
{
  xobject_class_t parent_class;
} binding_source_class_t;

enum
{
  PROP_SOURCE_0,

  PROP_SOURCE_FOO,
  PROP_SOURCE_BAR,
  PROP_SOURCE_DOUBLE_VALUE,
  PROP_SOURCE_TOGGLE,
  PROP_SOURCE_OBJECT
};

static xtype_t binding_source_get_type (void);
G_DEFINE_TYPE (binding_source, binding_source, XTYPE_OBJECT)

static void
binding_source_set_property (xobject_t      *gobject,
                             xuint_t         prop_id,
                             const xvalue_t *value,
                             xparam_spec_t   *pspec)
{
  binding_source_t *source = (binding_source_t *) gobject;

  switch (prop_id)
    {
    case PROP_SOURCE_FOO:
      source->foo = xvalue_get_int (value);
      break;

    case PROP_SOURCE_BAR:
      source->bar = xvalue_get_int (value);
      break;

    case PROP_SOURCE_DOUBLE_VALUE:
      source->double_value = xvalue_get_double (value);
      break;

    case PROP_SOURCE_TOGGLE:
      source->toggle = xvalue_get_boolean (value);
      break;

    case PROP_SOURCE_OBJECT:
      source->item = xvalue_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_source_get_property (xobject_t    *gobject,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  binding_source_t *source = (binding_source_t *) gobject;

  switch (prop_id)
    {
    case PROP_SOURCE_FOO:
      xvalue_set_int (value, source->foo);
      break;

    case PROP_SOURCE_BAR:
      xvalue_set_int (value, source->bar);
      break;

    case PROP_SOURCE_DOUBLE_VALUE:
      xvalue_set_double (value, source->double_value);
      break;

    case PROP_SOURCE_TOGGLE:
      xvalue_set_boolean (value, source->toggle);
      break;

    case PROP_SOURCE_OBJECT:
      xvalue_set_object (value, source->item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_source_class_init (binding_source_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = binding_source_set_property;
  gobject_class->get_property = binding_source_get_property;

  xobject_class_install_property (gobject_class, PROP_SOURCE_FOO,
                                   g_param_spec_int ("foo", "foo_t", "foo_t",
                                                     -1, 100,
                                                     0,
                                                     G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_SOURCE_BAR,
                                   g_param_spec_int ("bar", "Bar", "Bar",
                                                     -1, 100,
                                                     0,
                                                     G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_SOURCE_DOUBLE_VALUE,
                                   g_param_spec_double ("double-value", "Value", "Value",
                                                        -100.0, 200.0,
                                                        0.0,
                                                        G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_SOURCE_TOGGLE,
                                   g_param_spec_boolean ("toggle", "Toggle", "Toggle",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_SOURCE_OBJECT,
                                   g_param_spec_object ("object", "Object", "Object",
                                                        XTYPE_OBJECT,
                                                        G_PARAM_READWRITE));
}

static void
binding_source_init (binding_source_t *self)
{
}

typedef struct _binding_target
{
  xobject_t parent_instance;

  xint_t bar;
  xdouble_t double_value;
  xboolean_t toggle;
  xpointer_t foo;
} binding_target_t;

typedef struct _binding_target_class
{
  xobject_class_t parent_class;
} binding_target_class_t;

enum
{
  PROP_TARGET_0,

  PROP_TARGET_BAR,
  PROP_TARGET_DOUBLE_VALUE,
  PROP_TARGET_TOGGLE,
  PROP_TARGET_FOO
};

static xtype_t binding_target_get_type (void);
G_DEFINE_TYPE (binding_target, binding_target, XTYPE_OBJECT)

static void
binding_target_set_property (xobject_t      *gobject,
                             xuint_t         prop_id,
                             const xvalue_t *value,
                             xparam_spec_t   *pspec)
{
  binding_target_t *target = (binding_target_t *) gobject;

  switch (prop_id)
    {
    case PROP_TARGET_BAR:
      target->bar = xvalue_get_int (value);
      break;

    case PROP_TARGET_DOUBLE_VALUE:
      target->double_value = xvalue_get_double (value);
      break;

    case PROP_TARGET_TOGGLE:
      target->toggle = xvalue_get_boolean (value);
      break;

    case PROP_TARGET_FOO:
      target->foo = xvalue_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_target_get_property (xobject_t    *gobject,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  binding_target_t *target = (binding_target_t *) gobject;

  switch (prop_id)
    {
    case PROP_TARGET_BAR:
      xvalue_set_int (value, target->bar);
      break;

    case PROP_TARGET_DOUBLE_VALUE:
      xvalue_set_double (value, target->double_value);
      break;

    case PROP_TARGET_TOGGLE:
      xvalue_set_boolean (value, target->toggle);
      break;

    case PROP_TARGET_FOO:
      xvalue_set_object (value, target->foo);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
binding_target_class_init (binding_target_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = binding_target_set_property;
  gobject_class->get_property = binding_target_get_property;

  xobject_class_install_property (gobject_class, PROP_TARGET_BAR,
                                   g_param_spec_int ("bar", "Bar", "Bar",
                                                     -1, 100,
                                                     0,
                                                     G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_TARGET_DOUBLE_VALUE,
                                   g_param_spec_double ("double-value", "Value", "Value",
                                                        -100.0, 200.0,
                                                        0.0,
                                                        G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_TARGET_TOGGLE,
                                   g_param_spec_boolean ("toggle", "Toggle", "Toggle",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  xobject_class_install_property (gobject_class, PROP_TARGET_FOO,
                                   g_param_spec_object ("foo", "foo_t", "foo_t",
                                                        foo_get_type (),
                                                        G_PARAM_READWRITE));
}

static void
binding_target_init (binding_target_t *self)
{
}

static xboolean_t
celsius_to_fahrenheit (xbinding_t     *binding,
                       const xvalue_t *from_value,
                       xvalue_t       *to_value,
                       xpointer_t      user_data G_GNUC_UNUSED)
{
  xdouble_t celsius, fahrenheit;

  g_assert_true (G_VALUE_HOLDS (from_value, XTYPE_DOUBLE));
  g_assert_true (G_VALUE_HOLDS (to_value, XTYPE_DOUBLE));

  celsius = xvalue_get_double (from_value);
  fahrenheit = (9 * celsius / 5) + 32.0;

  if (g_test_verbose ())
    g_printerr ("Converting %.2fC to %.2fF\n", celsius, fahrenheit);

  xvalue_set_double (to_value, fahrenheit);

  return TRUE;
}

static xboolean_t
fahrenheit_to_celsius (xbinding_t     *binding,
                       const xvalue_t *from_value,
                       xvalue_t       *to_value,
                       xpointer_t      user_data G_GNUC_UNUSED)
{
  xdouble_t celsius, fahrenheit;

  g_assert_true (G_VALUE_HOLDS (from_value, XTYPE_DOUBLE));
  g_assert_true (G_VALUE_HOLDS (to_value, XTYPE_DOUBLE));

  fahrenheit = xvalue_get_double (from_value);
  celsius = 5 * (fahrenheit - 32.0) / 9;

  if (g_test_verbose ())
    g_printerr ("Converting %.2fF to %.2fC\n", fahrenheit, celsius);

  xvalue_set_double (to_value, celsius);

  return TRUE;
}

static void
binding_default (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xobject_t *tmp;
  xbinding_t *binding;

  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_DEFAULT);

  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);
  tmp = xbinding_dup_source (binding);
  g_assert_nonnull (tmp);
  g_assert_true ((binding_source_t *) tmp == source);
  xobject_unref (tmp);
  tmp = xbinding_dup_target (binding);
  g_assert_nonnull (tmp);
  g_assert_true ((binding_target_t *) tmp == target);
  xobject_unref (tmp);
  g_assert_cmpstr (xbinding_get_source_property (binding), ==, "foo");
  g_assert_cmpstr (xbinding_get_target_property (binding), ==, "bar");
  g_assert_cmpint (xbinding_get_flags (binding), ==, XBINDING_DEFAULT);

  xobject_set (source, "foo", 42, NULL);
  g_assert_cmpint (source->foo, ==, target->bar);

  xobject_set (target, "bar", 47, NULL);
  g_assert_cmpint (source->foo, !=, target->bar);

  xobject_unref (binding);

  xobject_set (source, "foo", 0, NULL);
  g_assert_cmpint (source->foo, !=, target->bar);

  xobject_unref (source);
  xobject_unref (target);
  g_assert_null (binding);
}

static void
binding_canonicalisation (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding;
  xobject_t *tmp;

  g_test_summary ("test_t that bindings set up with non-canonical property names work");

  binding = xobject_bind_property (source, "double_value",
                                    target, "double_value",
                                    XBINDING_DEFAULT);

  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);
  tmp = xbinding_dup_source (binding);
  g_assert_nonnull (tmp);
  g_assert_true ((binding_source_t *) tmp == source);
  xobject_unref (tmp);
  tmp = xbinding_dup_target (binding);
  g_assert_nonnull (tmp);
  g_assert_true ((binding_target_t *) tmp == target);
  xobject_unref (tmp);
  g_assert_cmpstr (xbinding_get_source_property (binding), ==, "double-value");
  g_assert_cmpstr (xbinding_get_target_property (binding), ==, "double-value");
  g_assert_cmpint (xbinding_get_flags (binding), ==, XBINDING_DEFAULT);

  xobject_set (source, "double-value", 24.0, NULL);
  g_assert_cmpfloat (target->double_value, ==, source->double_value);

  xobject_set (target, "double-value", 69.0, NULL);
  g_assert_cmpfloat (source->double_value, !=, target->double_value);

  xobject_unref (target);
  xobject_unref (source);
  g_assert_null (binding);
}

static void
binding_bidirectional (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding;

  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_BIDIRECTIONAL);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);

  xobject_set (source, "foo", 42, NULL);
  g_assert_cmpint (source->foo, ==, target->bar);

  xobject_set (target, "bar", 47, NULL);
  g_assert_cmpint (source->foo, ==, target->bar);

  xobject_unref (binding);

  xobject_set (source, "foo", 0, NULL);
  g_assert_cmpint (source->foo, !=, target->bar);

  xobject_unref (source);
  xobject_unref (target);
  g_assert_null (binding);
}

static void
data_free (xpointer_t data)
{
  xboolean_t *b = data;

  *b = TRUE;
}

static void
binding_transform_default (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding;
  xpointer_t src, trg;
  xchar_t *src_prop, *trg_prop;
  xbinding_flags_t flags;

  binding = xobject_bind_property (source, "foo",
                                    target, "double-value",
                                    XBINDING_BIDIRECTIONAL);

  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);

  xobject_get (binding,
                "source", &src,
                "source-property", &src_prop,
                "target", &trg,
                "target-property", &trg_prop,
                "flags", &flags,
                NULL);
  g_assert_true (src == source);
  g_assert_true (trg == target);
  g_assert_cmpstr (src_prop, ==, "foo");
  g_assert_cmpstr (trg_prop, ==, "double-value");
  g_assert_cmpint (flags, ==, XBINDING_BIDIRECTIONAL);
  xobject_unref (src);
  xobject_unref (trg);
  g_free (src_prop);
  g_free (trg_prop);

  xobject_set (source, "foo", 24, NULL);
  g_assert_cmpfloat (target->double_value, ==, 24.0);

  xobject_set (target, "double-value", 69.0, NULL);
  g_assert_cmpint (source->foo, ==, 69);

  xobject_unref (target);
  xobject_unref (source);
  g_assert_null (binding);
}

static void
binding_transform (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding G_GNUC_UNUSED;
  xboolean_t unused_data = FALSE;

  binding = xobject_bind_property_full (source, "double-value",
                                         target, "double-value",
                                         XBINDING_BIDIRECTIONAL,
                                         celsius_to_fahrenheit,
                                         fahrenheit_to_celsius,
                                         &unused_data, data_free);

  xobject_set (source, "double-value", 24.0, NULL);
  g_assert_cmpfloat (target->double_value, ==, ((9 * 24.0 / 5) + 32.0));

  xobject_set (target, "double-value", 69.0, NULL);
  g_assert_cmpfloat (source->double_value, ==, (5 * (69.0 - 32.0) / 9));

  xobject_unref (source);
  xobject_unref (target);

  g_assert_true (unused_data);
}

static void
binding_transform_closure (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding G_GNUC_UNUSED;
  xboolean_t unused_data_1 = FALSE, unused_data_2 = FALSE;
  xclosure_t *c2f_clos, *f2c_clos;

  c2f_clos = g_cclosure_new (G_CALLBACK (celsius_to_fahrenheit), &unused_data_1, (xclosure_notify_t) data_free);

  f2c_clos = g_cclosure_new (G_CALLBACK (fahrenheit_to_celsius), &unused_data_2, (xclosure_notify_t) data_free);

  binding = xobject_bind_property_with_closures (source, "double-value",
                                                  target, "double-value",
                                                  XBINDING_BIDIRECTIONAL,
                                                  c2f_clos,
                                                  f2c_clos);

  xobject_set (source, "double-value", 24.0, NULL);
  g_assert_cmpfloat (target->double_value, ==, ((9 * 24.0 / 5) + 32.0));

  xobject_set (target, "double-value", 69.0, NULL);
  g_assert_cmpfloat (source->double_value, ==, (5 * (69.0 - 32.0) / 9));

  xobject_unref (source);
  xobject_unref (target);

  g_assert_true (unused_data_1);
  g_assert_true (unused_data_2);
}

static void
binding_chain (void)
{
  binding_source_t *a = xobject_new (binding_source_get_type (), NULL);
  binding_source_t *b = xobject_new (binding_source_get_type (), NULL);
  binding_source_t *c = xobject_new (binding_source_get_type (), NULL);
  xbinding_t *binding_1, *binding_2;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=621782");

  /* A -> B, B -> C */
  binding_1 = xobject_bind_property (a, "foo", b, "foo", XBINDING_BIDIRECTIONAL);
  xobject_add_weak_pointer (G_OBJECT (binding_1), (xpointer_t *) &binding_1);

  binding_2 = xobject_bind_property (b, "foo", c, "foo", XBINDING_BIDIRECTIONAL);
  xobject_add_weak_pointer (G_OBJECT (binding_2), (xpointer_t *) &binding_2);

  /* verify the chain */
  xobject_set (a, "foo", 42, NULL);
  g_assert_cmpint (a->foo, ==, b->foo);
  g_assert_cmpint (b->foo, ==, c->foo);

  /* unbind A -> B and B -> C */
  xobject_unref (binding_1);
  g_assert_null (binding_1);
  xobject_unref (binding_2);
  g_assert_null (binding_2);

  /* bind A -> C directly */
  binding_2 = xobject_bind_property (a, "foo", c, "foo", XBINDING_BIDIRECTIONAL);

  /* verify the chain is broken */
  xobject_set (a, "foo", 47, NULL);
  g_assert_cmpint (a->foo, !=, b->foo);
  g_assert_cmpint (a->foo, ==, c->foo);

  xobject_unref (a);
  xobject_unref (b);
  xobject_unref (c);
}

static void
binding_sync_create (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (),
                                        "foo", 42,
                                        NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (),
                                        "bar", 47,
                                        NULL);
  xbinding_t *binding;

  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_DEFAULT | XBINDING_SYNC_CREATE);

  g_assert_cmpint (source->foo, ==, 42);
  g_assert_cmpint (target->bar, ==, 42);

  xobject_set (source, "foo", 47, NULL);
  g_assert_cmpint (source->foo, ==, target->bar);

  xobject_unref (binding);

  xobject_set (target, "bar", 49, NULL);

  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_BIDIRECTIONAL | XBINDING_SYNC_CREATE);
  g_assert_cmpint (source->foo, ==, 47);
  g_assert_cmpint (target->bar, ==, 47);

  xobject_unref (source);
  xobject_unref (target);
}

static void
binding_invert_boolean (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (),
                                        "toggle", TRUE,
                                        NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (),
                                        "toggle", FALSE,
                                        NULL);
  xbinding_t *binding;

  binding = xobject_bind_property (source, "toggle",
                                    target, "toggle",
                                    XBINDING_BIDIRECTIONAL | XBINDING_INVERT_BOOLEAN);

  g_assert_true (source->toggle);
  g_assert_false (target->toggle);

  xobject_set (source, "toggle", FALSE, NULL);
  g_assert_false (source->toggle);
  g_assert_true (target->toggle);

  xobject_set (target, "toggle", FALSE, NULL);
  g_assert_true (source->toggle);
  g_assert_false (target->toggle);

  xobject_unref (binding);
  xobject_unref (source);
  xobject_unref (target);
}

static void
binding_same_object (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (),
                                        "foo", 100,
                                        "bar", 50,
                                        NULL);
  xbinding_t *binding;

  binding = xobject_bind_property (source, "foo",
                                    source, "bar",
                                    XBINDING_BIDIRECTIONAL);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);

  xobject_set (source, "foo", 10, NULL);
  g_assert_cmpint (source->foo, ==, 10);
  g_assert_cmpint (source->bar, ==, 10);
  xobject_set (source, "bar", 30, NULL);
  g_assert_cmpint (source->foo, ==, 30);
  g_assert_cmpint (source->bar, ==, 30);

  xobject_unref (source);
  g_assert_null (binding);
}

static void
binding_unbind (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding;

  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_DEFAULT);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);

  xobject_set (source, "foo", 42, NULL);
  g_assert_cmpint (source->foo, ==, target->bar);

  xobject_set (target, "bar", 47, NULL);
  g_assert_cmpint (source->foo, !=, target->bar);

  xbinding_unbind (binding);
  g_assert_null (binding);

  xobject_set (source, "foo", 0, NULL);
  g_assert_cmpint (source->foo, !=, target->bar);

  xobject_unref (source);
  xobject_unref (target);


  /* xbinding_unbind() has a special case for this */
  source = xobject_new (binding_source_get_type (), NULL);
  binding = xobject_bind_property (source, "foo",
                                    source, "bar",
                                    XBINDING_DEFAULT);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);

  xbinding_unbind (binding);
  g_assert_null (binding);

  xobject_unref (source);
}

/* When source or target die, so does the binding if there is no other ref */
static void
binding_unbind_weak (void)
{
  xbinding_t *binding;
  binding_source_t *source;
  binding_target_t *target;

  /* first source, then target */
  source = xobject_new (binding_source_get_type (), NULL);
  target = xobject_new (binding_target_get_type (), NULL);
  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_DEFAULT);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);
  g_assert_nonnull (binding);
  xobject_unref (source);
  g_assert_null (binding);
  xobject_unref (target);
  g_assert_null (binding);

  /* first target, then source */
  source = xobject_new (binding_source_get_type (), NULL);
  target = xobject_new (binding_target_get_type (), NULL);
  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_DEFAULT);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);
  g_assert_nonnull (binding);
  xobject_unref (target);
  g_assert_null (binding);
  xobject_unref (source);
  g_assert_null (binding);

  /* target and source are the same */
  source = xobject_new (binding_source_get_type (), NULL);
  binding = xobject_bind_property (source, "foo",
                                    source, "bar",
                                    XBINDING_DEFAULT);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);
  g_assert_nonnull (binding);
  xobject_unref (source);
  g_assert_null (binding);
}

/* test_t that every call to unbind() after the first is a noop */
static void
binding_unbind_multiple (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding;
  xuint_t i;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1373");

  binding = xobject_bind_property (source, "foo",
                                    target, "bar",
                                    XBINDING_DEFAULT);
  xobject_ref (binding);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);
  g_assert_nonnull (binding);

  /* this shouldn't crash */
  for (i = 0; i < 50; i++)
    {
      xbinding_unbind (binding);
      g_assert_nonnull (binding);
    }

  xobject_unref (binding);
  g_assert_null (binding);

  xobject_unref (source);
  xobject_unref (target);
}

static void
binding_fail (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xbinding_t *binding;

  /* double -> boolean is not supported */
  binding = xobject_bind_property (source, "double-value",
                                    target, "toggle",
                                    XBINDING_DEFAULT);
  xobject_add_weak_pointer (G_OBJECT (binding), (xpointer_t *) &binding);

  g_test_expect_message ("GLib-xobject_t", G_LOG_LEVEL_WARNING,
                         "*Unable to convert*double*boolean*");
  xobject_set (source, "double-value", 1.0, NULL);
  g_test_assert_expected_messages ();

  xobject_unref (source);
  xobject_unref (target);
  g_assert_null (binding);
}

static xboolean_t
transform_to_func (xbinding_t     *binding,
                   const xvalue_t *value_a,
                   xvalue_t       *value_b,
                   xpointer_t      user_data)
{
  if (xvalue_type_compatible (G_VALUE_TYPE (value_a), G_VALUE_TYPE (value_b)))
    {
      xvalue_copy (value_a, value_b);
      return TRUE;
    }

  if (xvalue_type_transformable (G_VALUE_TYPE (value_a), G_VALUE_TYPE (value_b)))
    {
      if (xvalue_transform (value_a, value_b))
        return TRUE;
    }

  return FALSE;
}

static void
binding_interface (void)
{
  binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
  binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
  xobject_t *baa;
  xbinding_t *binding;
  xclosure_t *transform_to;

  /* binding a generic object property to an interface-valued one */
  binding = xobject_bind_property (source, "object",
                                    target, "foo",
                                    XBINDING_DEFAULT);

  baa = xobject_new (baa_get_type (), NULL);
  xobject_set (source, "object", baa, NULL);
  xobject_unref (baa);

  xbinding_unbind (binding);

  /* the same, with a generic marshaller */
  transform_to = g_cclosure_new (G_CALLBACK (transform_to_func), NULL, NULL);
  xclosure_set_marshal (transform_to, g_cclosure_marshal_generic);
  binding = xobject_bind_property_with_closures (source, "object",
                                                  target, "foo",
                                                  XBINDING_DEFAULT,
                                                  transform_to,
                                                  NULL);

  baa = xobject_new (baa_get_type (), NULL);
  xobject_set (source, "object", baa, NULL);
  xobject_unref (baa);

  xbinding_unbind (binding);

  xobject_unref (source);
  xobject_unref (target);
}

typedef struct {
  xthread_t *thread;
  xbinding_t *binding;
  xmutex_t *lock;
  xcond_t *cond;
  xboolean_t *wait;
  xint_t *count; /* (atomic) */
} ConcurrentUnbindData;

static xpointer_t
concurrent_unbind_func (xpointer_t data)
{
  ConcurrentUnbindData *unbind_data = data;

  g_mutex_lock (unbind_data->lock);
  g_atomic_int_inc (unbind_data->count);
  while (*unbind_data->wait)
    g_cond_wait (unbind_data->cond, unbind_data->lock);
  g_mutex_unlock (unbind_data->lock);
  xbinding_unbind (unbind_data->binding);
  xobject_unref (unbind_data->binding);

  return NULL;
}

static void
binding_concurrent_unbind (void)
{
  xuint_t i, j;

  g_test_summary ("test_t that unbinding from multiple threads concurrently works correctly");

  for (i = 0; i < 50; i++)
    {
      binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
      binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
      xbinding_t *binding;
      xqueue_t threads = G_QUEUE_INIT;
      xmutex_t lock;
      xcond_t cond;
      xboolean_t wait = TRUE;
      xint_t count = 0; /* (atomic) */
      ConcurrentUnbindData *data;

      g_mutex_init (&lock);
      g_cond_init (&cond);

      binding = xobject_bind_property (source, "foo",
                                        target, "bar",
                                        XBINDING_BIDIRECTIONAL);
      xobject_ref (binding);

      for (j = 0; j < 10; j++)
        {
          data = g_new0 (ConcurrentUnbindData, 1);

          data->binding = xobject_ref (binding);
          data->lock = &lock;
          data->cond = &cond;
          data->wait = &wait;
          data->count = &count;

          data->thread = xthread_new ("binding-concurrent", concurrent_unbind_func, data);
          g_queue_push_tail (&threads, data);
        }

      /* wait until all threads are started */
      while (g_atomic_int_get (&count) < 10)
        xthread_yield ();

      g_mutex_lock (&lock);
      wait = FALSE;
      g_cond_broadcast (&cond);
      g_mutex_unlock (&lock);

      while ((data = g_queue_pop_head (&threads)))
        {
          xthread_join (data->thread);
          g_free (data);
        }

      g_mutex_clear (&lock);
      g_cond_clear (&cond);

      xobject_unref (binding);
      xobject_unref (source);
      xobject_unref (target);
    }
}

typedef struct {
  xobject_t *object;
  xmutex_t *lock;
  xcond_t *cond;
  xint_t *count; /* (atomic) */
  xboolean_t *wait;
} ConcurrentFinalizeData;

static xpointer_t
concurrent_finalize_func (xpointer_t data)
{
  ConcurrentFinalizeData *finalize_data = data;

  g_mutex_lock (finalize_data->lock);
  g_atomic_int_inc (finalize_data->count);
  while (*finalize_data->wait)
    g_cond_wait (finalize_data->cond, finalize_data->lock);
  g_mutex_unlock (finalize_data->lock);
  xobject_unref (finalize_data->object);
  g_free (finalize_data);

  return NULL;
}

static void
binding_concurrent_finalizing (void)
{
  xuint_t i;

  g_test_summary ("test_t that finalizing source/target from multiple threads concurrently works correctly");

  for (i = 0; i < 50; i++)
    {
      binding_source_t *source = xobject_new (binding_source_get_type (), NULL);
      binding_target_t *target = xobject_new (binding_target_get_type (), NULL);
      xbinding_t *binding;
      xmutex_t lock;
      xcond_t cond;
      xboolean_t wait = TRUE;
      ConcurrentFinalizeData *data;
      xthread_t *source_thread, *target_thread;
      xint_t count = 0; /* (atomic) */

      g_mutex_init (&lock);
      g_cond_init (&cond);

      binding = xobject_bind_property (source, "foo",
                                        target, "bar",
                                        XBINDING_BIDIRECTIONAL);
      xobject_ref (binding);

      data = g_new0 (ConcurrentFinalizeData, 1);
      data->object = (xobject_t *) source;
      data->wait = &wait;
      data->lock = &lock;
      data->cond = &cond;
      data->count = &count;
      source_thread = xthread_new ("binding-concurrent", concurrent_finalize_func, data);

      data = g_new0 (ConcurrentFinalizeData, 1);
      data->object = (xobject_t *) target;
      data->wait = &wait;
      data->lock = &lock;
      data->cond = &cond;
      data->count = &count;
      target_thread = xthread_new ("binding-concurrent", concurrent_finalize_func, data);

      /* wait until all threads are started */
      while (g_atomic_int_get (&count) < 2)
        xthread_yield ();

      g_mutex_lock (&lock);
      wait = FALSE;
      g_cond_broadcast (&cond);
      g_mutex_unlock (&lock);

      xthread_join (source_thread);
      xthread_join (target_thread);

      g_mutex_clear (&lock);
      g_cond_clear (&cond);

      xobject_unref (binding);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/binding/default", binding_default);
  g_test_add_func ("/binding/canonicalisation", binding_canonicalisation);
  g_test_add_func ("/binding/bidirectional", binding_bidirectional);
  g_test_add_func ("/binding/transform", binding_transform);
  g_test_add_func ("/binding/transform-default", binding_transform_default);
  g_test_add_func ("/binding/transform-closure", binding_transform_closure);
  g_test_add_func ("/binding/chain", binding_chain);
  g_test_add_func ("/binding/sync-create", binding_sync_create);
  g_test_add_func ("/binding/invert-boolean", binding_invert_boolean);
  g_test_add_func ("/binding/same-object", binding_same_object);
  g_test_add_func ("/binding/unbind", binding_unbind);
  g_test_add_func ("/binding/unbind-weak", binding_unbind_weak);
  g_test_add_func ("/binding/unbind-multiple", binding_unbind_multiple);
  g_test_add_func ("/binding/fail", binding_fail);
  g_test_add_func ("/binding/interface", binding_interface);
  g_test_add_func ("/binding/concurrent-unbind", binding_concurrent_unbind);
  g_test_add_func ("/binding/concurrent-finalizing", binding_concurrent_finalizing);

  return g_test_run ();
}
