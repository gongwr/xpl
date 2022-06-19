#include <glib-object.h>

static const xenum_value_t my_enum_values[] =
{
  { 1, "the first value", "one" },
  { 2, "the second value", "two" },
  { 3, "the third value", "three" },
  { 0, NULL, NULL }
};

static void
test_enum_basic (void)
{
  xtype_t type;
  xenum_class_t *class;
  xenum_value_t *val;
  xvalue_t value = G_VALUE_INIT;
  xchar_t *to_string;

  type = xenum_register_static ("my_enum_t", my_enum_values);

  xvalue_init (&value, type);
  xassert (G_VALUE_HOLDS_ENUM (&value));

  xvalue_set_enum (&value, 2);
  g_assert_cmpint (xvalue_get_enum (&value), ==, 2);
  xvalue_unset (&value);

  class = xtype_class_ref (type);

  g_assert_cmpint (class->minimum, ==, 1);
  g_assert_cmpint (class->maximum, ==, 3);
  g_assert_cmpint (class->n_values, ==, 3);

  val = xenum_get_value (class, 2);
  xassert (val != NULL);
  g_assert_cmpstr (val->value_name, ==, "the second value");
  val = xenum_get_value (class, 15);
  xassert (val == NULL);

  val = xenum_get_value_by_name (class, "the third value");
  xassert (val != NULL);
  g_assert_cmpint (val->value, ==, 3);
  val = xenum_get_value_by_name (class, "the color purple");
  xassert (val == NULL);

  val = xenum_get_value_by_nick (class, "one");
  xassert (val != NULL);
  g_assert_cmpint (val->value, ==, 1);
  val = xenum_get_value_by_nick (class, "purple");
  xassert (val == NULL);

  to_string = xenum_to_string (type, 2);
  g_assert_cmpstr (to_string, ==, "the second value");
  g_free (to_string);

  to_string = xenum_to_string (type, 15);
  g_assert_cmpstr (to_string, ==, "15");
  g_free (to_string);

  xtype_class_unref (class);
}

static const xflags_value_t my_flaxvalues[] =
{
  { 0, "no flags", "none" },
  { 1, "the first flag", "one" },
  { 2, "the second flag", "two" },
  { 8, "the third flag", "three" },
  { 0, NULL, NULL }
};

static const xflags_value_t no_default_flaxvalues[] =
{
  { 1, "the first flag", "one" },
  { 0, NULL, NULL }
};

static void
test_flags_transform_to_string (const xvalue_t *value)
{
  xvalue_t tmp = G_VALUE_INIT;

  xvalue_init (&tmp, XTYPE_STRING);
  xvalue_transform (value, &tmp);
  xvalue_unset (&tmp);
}

static void
test_flags_basic (void)
{
  xtype_t type, no_default_type;
  xflags_class_t *class;
  xflags_value_t *val;
  xvalue_t value = G_VALUE_INIT;
  xchar_t *to_string;

  type = xflags_register_static ("my_flags_t", my_flaxvalues);
  no_default_type = xflags_register_static ("NoDefaultFlags",
                                             no_default_flaxvalues);

  xvalue_init (&value, type);
  xassert (G_VALUE_HOLDS_FLAGS (&value));

  xvalue_set_flags (&value, 2|8);
  g_assert_cmpint (xvalue_get_flags (&value), ==, 2|8);

  class = xtype_class_ref (type);

  g_assert_cmpint (class->mask, ==, 1|2|8);
  g_assert_cmpint (class->n_values, ==, 4);

  val = xflags_get_first_value (class, 2|8);
  xassert (val != NULL);
  g_assert_cmpstr (val->value_name, ==, "the second flag");
  val = xflags_get_first_value (class, 16);
  xassert (val == NULL);

  val = xflags_get_value_by_name (class, "the third flag");
  xassert (val != NULL);
  g_assert_cmpint (val->value, ==, 8);
  val = xflags_get_value_by_name (class, "the color purple");
  xassert (val == NULL);

  val = xflags_get_value_by_nick (class, "one");
  xassert (val != NULL);
  g_assert_cmpint (val->value, ==, 1);
  val = xflags_get_value_by_nick (class, "purple");
  xassert (val == NULL);

  test_flags_transform_to_string (&value);
  xvalue_unset (&value);

  to_string = xflags_to_string (type, 1|8);
  g_assert_cmpstr (to_string, ==, "the first flag | the third flag");
  g_free (to_string);

  to_string = xflags_to_string (type, 0);
  g_assert_cmpstr (to_string, ==, "no flags");
  g_free (to_string);

  to_string = xflags_to_string (type, 16);
  g_assert_cmpstr (to_string, ==, "0x10");
  g_free (to_string);

  to_string = xflags_to_string (type, 1|16);
  g_assert_cmpstr (to_string, ==, "the first flag | 0x10");
  g_free (to_string);

  to_string = xflags_to_string (no_default_type, 0);
  g_assert_cmpstr (to_string, ==, "0x0");
  g_free (to_string);

  to_string = xflags_to_string (no_default_type, 16);
  g_assert_cmpstr (to_string, ==, "0x10");
  g_free (to_string);

  xtype_class_unref (class);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/enum/basic", test_enum_basic);
  g_test_add_func ("/flags/basic", test_flags_basic);

  return g_test_run ();
}
