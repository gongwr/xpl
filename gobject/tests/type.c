#include <glib-object.h>

static void
test_registration_serial (void)
{
  xint_t serial1, serial2, serial3;

  serial1 = xtype_get_type_registration_serial ();
  g_pointer_type_register_static ("my+pointer");
  serial2 = xtype_get_type_registration_serial ();
  xassert (serial1 != serial2);
  serial3 = xtype_get_type_registration_serial ();
  xassert (serial2 == serial3);
}

typedef struct {
  xtype_interface_t x_iface;
} bar_interface_t;

xtype_t bar_get_type (void);

G_DEFINE_INTERFACE (bar, bar, XTYPE_OBJECT)

static void
bar_default_init (bar_interface_t *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} foo_interface_t;

xtype_t foo_get_type (void);

G_DEFINE_INTERFACE_WITH_CODE (foo, foo, XTYPE_OBJECT,
                              xtype_interface_add_prerequisite (g_define_type_id, bar_get_type ()))

static void
foo_default_init (foo_interface_t *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} baa_interface_t;

xtype_t baa_get_type (void);

G_DEFINE_INTERFACE (baa, baa, XTYPE_INVALID)

static void
baa_default_init (baa_interface_t *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} boo_interface_t;

xtype_t boo_get_type (void);

G_DEFINE_INTERFACE_WITH_CODE (boo, boo, XTYPE_INVALID,
                              xtype_interface_add_prerequisite (g_define_type_id, baa_get_type ()))

static void
boo_default_init (boo_interface_t *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} bibi_interface_t;

xtype_t bibi_get_type (void);

G_DEFINE_INTERFACE (bibi, bibi, XTYPE_INITIALLY_UNOWNED)

static void
bibi_default_init (bibi_interface_t *iface)
{
}

typedef struct {
  xtype_interface_t x_iface;
} bozo_interface_t;

xtype_t bozo_get_type (void);

G_DEFINE_INTERFACE_WITH_CODE (bozo, bozo, XTYPE_INVALID,
                              xtype_interface_add_prerequisite (g_define_type_id, foo_get_type ());
                              xtype_interface_add_prerequisite (g_define_type_id, bibi_get_type ()))

static void
bozo_default_init (bozo_interface_t *iface)
{
}



static void
test_interface_prerequisite (void)
{
  xtype_t *prereqs;
  xuint_t n_prereqs;
  xpointer_t iface;
  xpointer_t parent;

  prereqs = xtype_interface_prerequisites (foo_get_type (), &n_prereqs);
  g_assert_cmpint (n_prereqs, ==, 2);
  xassert (prereqs[0] == bar_get_type ());
  xassert (prereqs[1] == XTYPE_OBJECT);
  xassert (xtype_interface_instantiatable_prerequisite (foo_get_type ()) == XTYPE_OBJECT);

  iface = xtype_default_interface_ref (foo_get_type ());
  parent = xtype_interface_peek_parent (iface);
  xassert (parent == NULL);
  xtype_default_interface_unref (iface);

  g_free (prereqs);

  g_assert_cmpint (xtype_interface_instantiatable_prerequisite (baa_get_type ()), ==, XTYPE_INVALID);
  g_assert_cmpint (xtype_interface_instantiatable_prerequisite (boo_get_type ()), ==, XTYPE_INVALID);

  g_assert_cmpint (xtype_interface_instantiatable_prerequisite (bozo_get_type ()), ==, XTYPE_INITIALLY_UNOWNED);
}

typedef struct {
  xtype_interface_t x_iface;
} baz_interface_t;

xtype_t baz_get_type (void);

G_DEFINE_INTERFACE (baz, baz, XTYPE_OBJECT)

static void
baz_default_init (baz_interface_t *iface)
{
}

typedef struct {
  xobject_t parent;
} bazo_t;

typedef struct {
  xobject_class_t parent_class;
} bazo_class_t;

xtype_t bazo_get_type (void);
static void bazo_iface_init (baz_interface_t *i);

G_DEFINE_TYPE_WITH_CODE (bazo, bazo, XTYPE_INITIALLY_UNOWNED,
                         G_IMPLEMENT_INTERFACE (baz_get_type (),
                                                bazo_iface_init);)

static void
bazo_init (bazo_t *b)
{
}

static void
bazo_class_init (bazo_class_t *c)
{
}

static void
bazo_iface_init (baz_interface_t *i)
{
}

static xint_t check_called;

static void
check_func (xpointer_t check_data,
            xpointer_t x_iface)
{
  xassert (check_data == &check_called);

  check_called++;
}

static void
test_interface_check (void)
{
  xobject_t *o;

  check_called = 0;
  xtype_add_interface_check (&check_called, check_func);
  o = xobject_new (bazo_get_type (), NULL);
  xobject_unref (o);
  g_assert_cmpint (check_called, ==, 1);
  xtype_remove_interface_check (&check_called, check_func);
}

static void
test_next_base (void)
{
  xtype_t type;

  type = xtype_next_base (bazo_get_type (), XTYPE_OBJECT);

  xassert (type == XTYPE_INITIALLY_UNOWNED);
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
