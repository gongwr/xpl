#include <glib-object.h>

static void
test_registration_serial (void)
{
  xint_t serial1, serial2, serial3;

  serial1 = g_type_get_type_registration_serial ();
  g_pointer_type_register_static ("my+pointer");
  serial2 = g_type_get_type_registration_serial ();
  g_assert (serial1 != serial2);
  serial3 = g_type_get_type_registration_serial ();
  g_assert (serial2 == serial3);
}

typedef struct {
  xtype_interface_t x_iface;
} BarInterface;

xtype_t bar_get_type (void);

G_DEFINE_INTERFACE (Bar, bar, XTYPE_OBJECT)

static void
bar_default_init (BarInterface *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} FooInterface;

xtype_t foo_get_type (void);

G_DEFINE_INTERFACE_WITH_CODE (Foo, foo, XTYPE_OBJECT,
                              g_type_interface_add_prerequisite (g_define_type_id, bar_get_type ()))

static void
foo_default_init (FooInterface *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} BaaInterface;

xtype_t baa_get_type (void);

G_DEFINE_INTERFACE (Baa, baa, XTYPE_INVALID)

static void
baa_default_init (BaaInterface *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} BooInterface;

xtype_t boo_get_type (void);

G_DEFINE_INTERFACE_WITH_CODE (Boo, boo, XTYPE_INVALID,
                              g_type_interface_add_prerequisite (g_define_type_id, baa_get_type ()))

static void
boo_default_init (BooInterface *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} BibiInterface;

xtype_t bibi_get_type (void);

G_DEFINE_INTERFACE (Bibi, bibi, XTYPE_INITIALLY_UNOWNED)

static void
bibi_default_init (BibiInterface *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} BozoInterface;

xtype_t bozo_get_type (void);

G_DEFINE_INTERFACE_WITH_CODE (Bozo, bozo, XTYPE_INVALID,
                              g_type_interface_add_prerequisite (g_define_type_id, foo_get_type ());
                              g_type_interface_add_prerequisite (g_define_type_id, bibi_get_type ()))

static void
bozo_default_init (BozoInterface *iface)
{
}



static void
test_interface_prerequisite (void)
{
  xtype_t *prereqs;
  xuint_t n_prereqs;
  xpointer_t iface;
  xpointer_t parent;

  prereqs = g_type_interface_prerequisites (foo_get_type (), &n_prereqs);
  g_assert_cmpint (n_prereqs, ==, 2);
  g_assert (prereqs[0] == bar_get_type ());
  g_assert (prereqs[1] == XTYPE_OBJECT);
  g_assert (g_type_interface_instantiatable_prerequisite (foo_get_type ()) == XTYPE_OBJECT);

  iface = g_type_default_interface_ref (foo_get_type ());
  parent = g_type_interface_peek_parent (iface);
  g_assert (parent == NULL);
  g_type_default_interface_unref (iface);

  g_free (prereqs);

  g_assert_cmpint (g_type_interface_instantiatable_prerequisite (baa_get_type ()), ==, XTYPE_INVALID);
  g_assert_cmpint (g_type_interface_instantiatable_prerequisite (boo_get_type ()), ==, XTYPE_INVALID);

  g_assert_cmpint (g_type_interface_instantiatable_prerequisite (bozo_get_type ()), ==, XTYPE_INITIALLY_UNOWNED);
}

typedef struct {
  xtype_interface_t x_iface;
} BazInterface;

xtype_t baz_get_type (void);

G_DEFINE_INTERFACE (Baz, baz, XTYPE_OBJECT)

static void
baz_default_init (BazInterface *iface)
{
}

typedef struct {
  xobject_t parent;
} Bazo;

typedef struct {
  xobject_class_t parent_class;
} BazoClass;

xtype_t bazo_get_type (void);
static void bazo_iface_init (BazInterface *i);

G_DEFINE_TYPE_WITH_CODE (Bazo, bazo, XTYPE_INITIALLY_UNOWNED,
                         G_IMPLEMENT_INTERFACE (baz_get_type (),
                                                bazo_iface_init);)

static void
bazo_init (Bazo *b)
{
}

static void
bazo_class_init (BazoClass *c)
{
}

static void
bazo_iface_init (BazInterface *i)
{
}

static xint_t check_called;

static void
check_func (xpointer_t check_data,
            xpointer_t x_iface)
{
  g_assert (check_data == &check_called);

  check_called++;
}

static void
test_interface_check (void)
{
  xobject_t *o;

  check_called = 0;
  g_type_add_interface_check (&check_called, check_func);
  o = g_object_new (bazo_get_type (), NULL);
  g_object_unref (o);
  g_assert_cmpint (check_called, ==, 1);
  g_type_remove_interface_check (&check_called, check_func);
}

static void
test_next_base (void)
{
  xtype_t type;

  type = g_type_next_base (bazo_get_type (), XTYPE_OBJECT);

  g_assert (type == XTYPE_INITIALLY_UNOWNED);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/type/registration-serial", test_registration_serial);
  g_test_add_func ("/type/interface-prerequisite", test_interface_prerequisite);
  g_test_add_func ("/type/interface-check", test_interface_check);
  g_test_add_func ("/type/next-base", test_next_base);

  return g_test_run ();
}
