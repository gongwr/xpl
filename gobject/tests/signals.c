#include <glib-object.h>
#include "marshalers.h"

#define g_assert_cmpflags(type,n1, cmp, n2) G_STMT_START { \
                                               type __n1 = (n1), __n2 = (n2); \
                                               if (__n1 cmp __n2) ; else \
                                                 g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                                                             #n1 " " #cmp " " #n2, __n1, #cmp, __n2, 'i'); \
                                            } G_STMT_END
#define g_assert_cmpenum(type,n1, cmp, n2) G_STMT_START { \
                                               type __n1 = (n1), __n2 = (n2); \
                                               if (__n1 cmp __n2) ; else \
                                                 g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                                                             #n1 " " #cmp " " #n2, __n1, #cmp, __n2, 'i'); \
                                            } G_STMT_END

typedef enum {
  TEST_ENUM_NEGATIVE = -30,
  TEST_ENUM_NONE = 0,
  TEST_ENUM_FOO = 1,
  TEST_ENUM_BAR = 2
} TestEnum;

typedef enum {
  TEST_UNSIGNED_ENUM_FOO = 1,
  TEST_UNSIGNED_ENUM_BAR = 42
  /* Don't test 0x80000000 for now- nothing appears to do this in
   * practice, and it triggers xvalue_t/xenum_t bugs on ppc64.
   */
} TestUnsignedEnum;

static void
custom_marshal_VOID__INVOCATIONHINT (xclosure_t     *closure,
                                     xvalue_t       *return_value G_GNUC_UNUSED,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INVOCATIONHINT) (xpointer_t     data1,
                                                     xpointer_t     invocation_hint,
                                                     xpointer_t     data2);
  GMarshalFunc_VOID__INVOCATIONHINT callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INVOCATIONHINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            invocation_hint,
            data2);
}

static xtype_t
test_enum_get_type (void)
{
  static xsize_t static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      static const xenum_value_t values[] = {
        { TEST_ENUM_NEGATIVE, "TEST_ENUM_NEGATIVE", "negative" },
        { TEST_ENUM_NONE, "TEST_ENUM_NONE", "none" },
        { TEST_ENUM_FOO, "TEST_ENUM_FOO", "foo" },
        { TEST_ENUM_BAR, "TEST_ENUM_BAR", "bar" },
        { 0, NULL, NULL }
      };
      xtype_t g_define_type_id =
        xenum_register_static (g_intern_static_string ("TestEnum"), values);
      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

static xtype_t
test_unsigned_enum_get_type (void)
{
  static xsize_t static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      static const xenum_value_t values[] = {
        { TEST_UNSIGNED_ENUM_FOO, "TEST_UNSIGNED_ENUM_FOO", "foo" },
        { TEST_UNSIGNED_ENUM_BAR, "TEST_UNSIGNED_ENUM_BAR", "bar" },
        { 0, NULL, NULL }
      };
      xtype_t g_define_type_id =
        xenum_register_static (g_intern_static_string ("TestUnsignedEnum"), values);
      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

typedef enum {
  MY_ENUM_VALUE = 1,
} MyEnum;

static const xenum_value_t my_enum_values[] =
{
  { MY_ENUM_VALUE, "the first value", "one" },
  { 0, NULL, NULL }
};

typedef enum {
  MY_FLAGS_FIRST_BIT = (1 << 0),
  MY_FLAGS_THIRD_BIT = (1 << 2),
  MY_FLAGS_LAST_BIT = (1 << 31)
} MyFlags;

static const xflags_value_t my_flaxvalues[] =
{
  { MY_FLAGS_FIRST_BIT, "the first bit", "first-bit" },
  { MY_FLAGS_THIRD_BIT, "the third bit", "third-bit" },
  { MY_FLAGS_LAST_BIT, "the last bit", "last-bit" },
  { 0, NULL, NULL }
};

static xtype_t enum_type;
static xtype_t flags_type;

static xuint_t simple_id;
static xuint_t simple2_id;

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

G_DEFINE_TYPE_WITH_CODE (baa_t, baa, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (foo_get_type (), baa_init_foo))

static void
baa_init (baa_t *baa)
{
}

static void
baa_class_init (baa_class_t *class)
{
}

typedef struct _Test Test;
typedef struct _TestClass TestClass;

struct _Test
{
  xobject_t parent_instance;
};

static void all_types_handler (Test *test, int i, xboolean_t b, char c, guchar uc, xuint_t ui, xlong_t l, gulong ul, MyEnum e, MyFlags f, float fl, double db, char *str, xparam_spec_t *param, xbytes_t *bytes, xpointer_t ptr, Test *obj, xvariant_t *var, gint64 i64, xuint64_t ui64);
static xboolean_t accumulator_sum (xsignal_invocation_hint_t *ihint, xvalue_t *return_accu, const xvalue_t *handler_return, xpointer_t data);
static xboolean_t accumulator_concat_string (xsignal_invocation_hint_t *ihint, xvalue_t *return_accu, const xvalue_t *handler_return, xpointer_t data);
static xchar_t * accumulator_class (Test *test);

struct _TestClass
{
  xobject_class_t parent_class;

  void (* variant_changed) (Test *, xvariant_t *);
  void (* all_types) (Test *test, int i, xboolean_t b, char c, guchar uc, xuint_t ui, xlong_t l, gulong ul, MyEnum e, MyFlags f, float fl, double db, char *str, xparam_spec_t *param, xbytes_t *bytes, xpointer_t ptr, Test *obj, xvariant_t *var, gint64 i64, xuint64_t ui64);
  void (* all_types_null) (Test *test, int i, xboolean_t b, char c, guchar uc, xuint_t ui, xlong_t l, gulong ul, MyEnum e, MyFlags f, float fl, double db, char *str, xparam_spec_t *param, xbytes_t *bytes, xpointer_t ptr, Test *obj, xvariant_t *var, gint64 i64, xuint64_t ui64);
  xchar_t * (*accumulator_class) (Test *test);
};

static xtype_t test_get_type (void);
G_DEFINE_TYPE (Test, test, XTYPE_OBJECT)

static void
test_init (Test *test)
{
}

static void
test_class_init (TestClass *klass)
{
  xuint_t s;

  enum_type = xenum_register_static ("MyEnum", my_enum_values);
  flags_type = xflags_register_static ("MyFlag", my_flaxvalues);

  klass->all_types = all_types_handler;
  klass->accumulator_class = accumulator_class;

  simple_id = g_signal_new ("simple",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                XTYPE_NONE,
                0);
  g_signal_new ("simple-detailed",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                NULL,
                XTYPE_NONE,
                0);
  /* Deliberately install this one in non-canonical form to check that’s handled correctly: */
  simple2_id = g_signal_new ("simple_2",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                0,
                NULL, NULL,
                NULL,
                XTYPE_NONE,
                0);
  g_signal_new ("simple-accumulator",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                accumulator_sum, NULL,
                NULL,
                XTYPE_INT,
                0);
  g_signal_new ("accumulator-class-first",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (TestClass, accumulator_class),
                accumulator_concat_string, NULL,
                NULL,
                XTYPE_STRING,
                0);
  g_signal_new ("accumulator-class-last",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestClass, accumulator_class),
                accumulator_concat_string, NULL,
                NULL,
                XTYPE_STRING,
                0);
  g_signal_new ("accumulator-class-cleanup",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_CLEANUP,
                G_STRUCT_OFFSET (TestClass, accumulator_class),
                accumulator_concat_string, NULL,
                NULL,
                XTYPE_STRING,
                0);
  g_signal_new ("accumulator-class-first-last",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestClass, accumulator_class),
                accumulator_concat_string, NULL,
                NULL,
                XTYPE_STRING,
                0);
  g_signal_new ("accumulator-class-first-last-cleanup",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP,
                G_STRUCT_OFFSET (TestClass, accumulator_class),
                accumulator_concat_string, NULL,
                NULL,
                XTYPE_STRING,
                0);
  g_signal_new ("accumulator-class-last-cleanup",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP,
                G_STRUCT_OFFSET (TestClass, accumulator_class),
                accumulator_concat_string, NULL,
                NULL,
                XTYPE_STRING,
                0);
  g_signal_new ("generic-marshaller-1",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                XTYPE_NONE,
                7,
                XTYPE_CHAR, XTYPE_UCHAR, XTYPE_INT, XTYPE_LONG, XTYPE_POINTER, XTYPE_DOUBLE, XTYPE_FLOAT);
  g_signal_new ("generic-marshaller-2",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                XTYPE_NONE,
                5,
                XTYPE_INT, test_enum_get_type(), XTYPE_INT, test_unsigned_enum_get_type (), XTYPE_INT);
  g_signal_new ("generic-marshaller-enum-return-signed",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                test_enum_get_type(),
                0);
  g_signal_new ("generic-marshaller-enum-return-unsigned",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                test_unsigned_enum_get_type(),
                0);
  g_signal_new ("generic-marshaller-int-return",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                XTYPE_INT,
                0);
  s = g_signal_new ("va-marshaller-int-return",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                test_INT__VOID,
                XTYPE_INT,
                0);
  g_signal_set_va_marshaller (s, XTYPE_FROM_CLASS (klass),
			      test_INT__VOIDv);
  g_signal_new ("generic-marshaller-uint-return",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                XTYPE_UINT,
                0);
  g_signal_new ("generic-marshaller-interface-return",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                NULL,
                foo_get_type (),
                0);
  s = g_signal_new ("va-marshaller-uint-return",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                test_INT__VOID,
                XTYPE_UINT,
                0);
  g_signal_set_va_marshaller (s, XTYPE_FROM_CLASS (klass),
			      test_UINT__VOIDv);
  g_signal_new ("custom-marshaller",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                custom_marshal_VOID__INVOCATIONHINT,
                XTYPE_NONE,
                1,
                XTYPE_POINTER);
  g_signal_new ("variant-changed-no-slot",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VARIANT,
                XTYPE_NONE,
                1,
                XTYPE_VARIANT);
  g_signal_new ("variant-changed",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                G_STRUCT_OFFSET (TestClass, variant_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__VARIANT,
                XTYPE_NONE,
                1,
                XTYPE_VARIANT);
  g_signal_new ("all-types",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestClass, all_types),
                NULL, NULL,
                test_VOID__INT_BOOLEAN_CHAR_UCHAR_UINT_LONG_ULONXENUM_FLAGS_FLOAT_DOUBLE_STRING_PARAM_BOXED_POINTER_OBJECT_VARIANT_INT64_UINT64,
                XTYPE_NONE,
                19,
		XTYPE_INT,
		XTYPE_BOOLEAN,
		XTYPE_CHAR,
		XTYPE_UCHAR,
		XTYPE_UINT,
		XTYPE_LONG,
		XTYPE_ULONG,
		enum_type,
		flags_type,
		XTYPE_FLOAT,
		XTYPE_DOUBLE,
		XTYPE_STRING,
		XTYPE_PARAM_LONG,
		XTYPE_BYTES,
		XTYPE_POINTER,
		test_get_type (),
                XTYPE_VARIANT,
		XTYPE_INT64,
		XTYPE_UINT64);
  s = g_signal_new ("all-types-va",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestClass, all_types),
                NULL, NULL,
                test_VOID__INT_BOOLEAN_CHAR_UCHAR_UINT_LONG_ULONXENUM_FLAGS_FLOAT_DOUBLE_STRING_PARAM_BOXED_POINTER_OBJECT_VARIANT_INT64_UINT64,
                XTYPE_NONE,
                19,
		XTYPE_INT,
		XTYPE_BOOLEAN,
		XTYPE_CHAR,
		XTYPE_UCHAR,
		XTYPE_UINT,
		XTYPE_LONG,
		XTYPE_ULONG,
		enum_type,
		flags_type,
		XTYPE_FLOAT,
		XTYPE_DOUBLE,
		XTYPE_STRING,
		XTYPE_PARAM_LONG,
		XTYPE_BYTES,
		XTYPE_POINTER,
		test_get_type (),
                XTYPE_VARIANT,
		XTYPE_INT64,
		XTYPE_UINT64);
  g_signal_set_va_marshaller (s, XTYPE_FROM_CLASS (klass),
			      test_VOID__INT_BOOLEAN_CHAR_UCHAR_UINT_LONG_ULONXENUM_FLAGS_FLOAT_DOUBLE_STRING_PARAM_BOXED_POINTER_OBJECT_VARIANT_INT64_UINT64v);

  g_signal_new ("all-types-generic",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestClass, all_types),
                NULL, NULL,
                NULL,
                XTYPE_NONE,
                19,
		XTYPE_INT,
		XTYPE_BOOLEAN,
		XTYPE_CHAR,
		XTYPE_UCHAR,
		XTYPE_UINT,
		XTYPE_LONG,
		XTYPE_ULONG,
		enum_type,
		flags_type,
		XTYPE_FLOAT,
		XTYPE_DOUBLE,
		XTYPE_STRING,
		XTYPE_PARAM_LONG,
		XTYPE_BYTES,
		XTYPE_POINTER,
		test_get_type (),
                XTYPE_VARIANT,
		XTYPE_INT64,
		XTYPE_UINT64);
  g_signal_new ("all-types-null",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (TestClass, all_types_null),
                NULL, NULL,
                test_VOID__INT_BOOLEAN_CHAR_UCHAR_UINT_LONG_ULONXENUM_FLAGS_FLOAT_DOUBLE_STRING_PARAM_BOXED_POINTER_OBJECT_VARIANT_INT64_UINT64,
                XTYPE_NONE,
                19,
		XTYPE_INT,
		XTYPE_BOOLEAN,
		XTYPE_CHAR,
		XTYPE_UCHAR,
		XTYPE_UINT,
		XTYPE_LONG,
		XTYPE_ULONG,
		enum_type,
		flags_type,
		XTYPE_FLOAT,
		XTYPE_DOUBLE,
		XTYPE_STRING,
		XTYPE_PARAM_LONG,
		XTYPE_BYTES,
		XTYPE_POINTER,
		test_get_type (),
                XTYPE_VARIANT,
		XTYPE_INT64,
		XTYPE_UINT64);
  g_signal_new ("all-types-empty",
                XTYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL,
                test_VOID__INT_BOOLEAN_CHAR_UCHAR_UINT_LONG_ULONXENUM_FLAGS_FLOAT_DOUBLE_STRING_PARAM_BOXED_POINTER_OBJECT_VARIANT_INT64_UINT64,
                XTYPE_NONE,
                19,
		XTYPE_INT,
		XTYPE_BOOLEAN,
		XTYPE_CHAR,
		XTYPE_UCHAR,
		XTYPE_UINT,
		XTYPE_LONG,
		XTYPE_ULONG,
		enum_type,
		flags_type,
		XTYPE_FLOAT,
		XTYPE_DOUBLE,
		XTYPE_STRING,
		XTYPE_PARAM_LONG,
		XTYPE_BYTES,
		XTYPE_POINTER,
		test_get_type (),
                XTYPE_VARIANT,
		XTYPE_INT64,
		XTYPE_UINT64);
}

typedef struct _Test Test2;
typedef struct _TestClass Test2Class;

static xtype_t test2_get_type (void);
G_DEFINE_TYPE (Test2, test2, XTYPE_OBJECT)

static void
test2_init (Test2 *test)
{
}

static void
test2_class_init (Test2Class *klass)
{
}

static void
test_variant_signal (void)
{
  Test *test;
  xvariant_t *v;

  /* Tests that the signal emission consumes the variant,
   * even if there are no handlers connected.
   */

  test = xobject_new (test_get_type (), NULL);

  v = xvariant_new_boolean (TRUE);
  xvariant_ref (v);
  g_assert_true (xvariant_is_floating (v));
  g_signal_emit_by_name (test, "variant-changed-no-slot", v);
  g_assert_false (xvariant_is_floating (v));
  xvariant_unref (v);

  v = xvariant_new_boolean (TRUE);
  xvariant_ref (v);
  g_assert_true (xvariant_is_floating (v));
  g_signal_emit_by_name (test, "variant-changed", v);
  g_assert_false (xvariant_is_floating (v));
  xvariant_unref (v);

  xobject_unref (test);
}

static void
on_generic_marshaller_1 (Test *obj,
			 gint8 v_schar,
			 xuint8_t v_uchar,
			 xint_t v_int,
			 xlong_t v_long,
			 xpointer_t v_pointer,
			 xdouble_t v_double,
			 gfloat v_float,
			 xpointer_t user_data)
{
  g_assert_cmpint (v_schar, ==, 42);
  g_assert_cmpint (v_uchar, ==, 43);
  g_assert_cmpint (v_int, ==, 4096);
  g_assert_cmpint (v_long, ==, 8192);
  g_assert_null (v_pointer);
  g_assert_cmpfloat (v_double, >, 0.0);
  g_assert_cmpfloat (v_double, <, 1.0);
  g_assert_cmpfloat (v_float, >, 5.0);
  g_assert_cmpfloat (v_float, <, 6.0);
}

static void
test_generic_marshaller_signal_1 (void)
{
  Test *test;
  test = xobject_new (test_get_type (), NULL);

  g_signal_connect (test, "generic-marshaller-1", G_CALLBACK (on_generic_marshaller_1), NULL);

  g_signal_emit_by_name (test, "generic-marshaller-1", 42, 43, 4096, 8192, NULL, 0.5, 5.5);

  xobject_unref (test);
}

static void
on_generic_marshaller_2 (Test *obj,
			 xint_t        v_int1,
			 TestEnum    v_enum,
			 xint_t        v_int2,
			 TestUnsignedEnum v_uenum,
			 xint_t        v_int3)
{
  g_assert_cmpint (v_int1, ==, 42);
  g_assert_cmpint (v_enum, ==, TEST_ENUM_BAR);
  g_assert_cmpint (v_int2, ==, 43);
  g_assert_cmpint (v_uenum, ==, TEST_UNSIGNED_ENUM_BAR);
  g_assert_cmpint (v_int3, ==, 44);
}

static void
test_generic_marshaller_signal_2 (void)
{
  Test *test;
  test = xobject_new (test_get_type (), NULL);

  g_signal_connect (test, "generic-marshaller-2", G_CALLBACK (on_generic_marshaller_2), NULL);

  g_signal_emit_by_name (test, "generic-marshaller-2", 42, TEST_ENUM_BAR, 43, TEST_UNSIGNED_ENUM_BAR, 44);

  xobject_unref (test);
}

static TestEnum
on_generic_marshaller_enum_return_signed_1 (Test *obj)
{
  return TEST_ENUM_NEGATIVE;
}

static TestEnum
on_generic_marshaller_enum_return_signed_2 (Test *obj)
{
  return TEST_ENUM_BAR;
}

static void
test_generic_marshaller_signal_enum_return_signed (void)
{
  Test *test;
  xuint_t id;
  TestEnum retval = 0;

  test = xobject_new (test_get_type (), NULL);

  /* Test return value NEGATIVE */
  id = g_signal_connect (test,
                         "generic-marshaller-enum-return-signed",
                         G_CALLBACK (on_generic_marshaller_enum_return_signed_1),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-enum-return-signed", &retval);
  g_assert_cmpint (retval, ==, TEST_ENUM_NEGATIVE);
  g_signal_handler_disconnect (test, id);

  /* Test return value BAR */
  retval = 0;
  id = g_signal_connect (test,
                         "generic-marshaller-enum-return-signed",
                         G_CALLBACK (on_generic_marshaller_enum_return_signed_2),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-enum-return-signed", &retval);
  g_assert_cmpint (retval, ==, TEST_ENUM_BAR);
  g_signal_handler_disconnect (test, id);

  xobject_unref (test);
}

static TestUnsignedEnum
on_generic_marshaller_enum_return_unsigned_1 (Test *obj)
{
  return TEST_UNSIGNED_ENUM_FOO;
}

static TestUnsignedEnum
on_generic_marshaller_enum_return_unsigned_2 (Test *obj)
{
  return TEST_UNSIGNED_ENUM_BAR;
}

static void
test_generic_marshaller_signal_enum_return_unsigned (void)
{
  Test *test;
  xuint_t id;
  TestUnsignedEnum retval = 0;

  test = xobject_new (test_get_type (), NULL);

  /* Test return value FOO */
  id = g_signal_connect (test,
                         "generic-marshaller-enum-return-unsigned",
                         G_CALLBACK (on_generic_marshaller_enum_return_unsigned_1),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-enum-return-unsigned", &retval);
  g_assert_cmpint (retval, ==, TEST_UNSIGNED_ENUM_FOO);
  g_signal_handler_disconnect (test, id);

  /* Test return value BAR */
  retval = 0;
  id = g_signal_connect (test,
                         "generic-marshaller-enum-return-unsigned",
                         G_CALLBACK (on_generic_marshaller_enum_return_unsigned_2),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-enum-return-unsigned", &retval);
  g_assert_cmpint (retval, ==, TEST_UNSIGNED_ENUM_BAR);
  g_signal_handler_disconnect (test, id);

  xobject_unref (test);
}

/**********************/

static xint_t
on_generic_marshaller_int_return_signed_1 (Test *obj)
{
  return -30;
}

static xint_t
on_generic_marshaller_int_return_signed_2 (Test *obj)
{
  return 2;
}

static void
test_generic_marshaller_signal_int_return (void)
{
  Test *test;
  xuint_t id;
  xint_t retval = 0;

  test = xobject_new (test_get_type (), NULL);

  /* Test return value -30 */
  id = g_signal_connect (test,
                         "generic-marshaller-int-return",
                         G_CALLBACK (on_generic_marshaller_int_return_signed_1),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-int-return", &retval);
  g_assert_cmpint (retval, ==, -30);
  g_signal_handler_disconnect (test, id);

  /* Test return value positive */
  retval = 0;
  id = g_signal_connect (test,
                         "generic-marshaller-int-return",
                         G_CALLBACK (on_generic_marshaller_int_return_signed_2),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-int-return", &retval);
  g_assert_cmpint (retval, ==, 2);
  g_signal_handler_disconnect (test, id);

  /* Same test for va marshaller */

  /* Test return value -30 */
  id = g_signal_connect (test,
                         "va-marshaller-int-return",
                         G_CALLBACK (on_generic_marshaller_int_return_signed_1),
                         NULL);
  g_signal_emit_by_name (test, "va-marshaller-int-return", &retval);
  g_assert_cmpint (retval, ==, -30);
  g_signal_handler_disconnect (test, id);

  /* Test return value positive */
  retval = 0;
  id = g_signal_connect (test,
                         "va-marshaller-int-return",
                         G_CALLBACK (on_generic_marshaller_int_return_signed_2),
                         NULL);
  g_signal_emit_by_name (test, "va-marshaller-int-return", &retval);
  g_assert_cmpint (retval, ==, 2);
  g_signal_handler_disconnect (test, id);

  xobject_unref (test);
}

static xuint_t
on_generic_marshaller_uint_return_1 (Test *obj)
{
  return 1;
}

static xuint_t
on_generic_marshaller_uint_return_2 (Test *obj)
{
  return G_MAXUINT;
}

static void
test_generic_marshaller_signal_uint_return (void)
{
  Test *test;
  xuint_t id;
  xuint_t retval = 0;

  test = xobject_new (test_get_type (), NULL);

  id = g_signal_connect (test,
                         "generic-marshaller-uint-return",
                         G_CALLBACK (on_generic_marshaller_uint_return_1),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-uint-return", &retval);
  g_assert_cmpint (retval, ==, 1);
  g_signal_handler_disconnect (test, id);

  retval = 0;
  id = g_signal_connect (test,
                         "generic-marshaller-uint-return",
                         G_CALLBACK (on_generic_marshaller_uint_return_2),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-uint-return", &retval);
  g_assert_cmpint (retval, ==, G_MAXUINT);
  g_signal_handler_disconnect (test, id);

  /* Same test for va marshaller */

  id = g_signal_connect (test,
                         "va-marshaller-uint-return",
                         G_CALLBACK (on_generic_marshaller_uint_return_1),
                         NULL);
  g_signal_emit_by_name (test, "va-marshaller-uint-return", &retval);
  g_assert_cmpint (retval, ==, 1);
  g_signal_handler_disconnect (test, id);

  retval = 0;
  id = g_signal_connect (test,
                         "va-marshaller-uint-return",
                         G_CALLBACK (on_generic_marshaller_uint_return_2),
                         NULL);
  g_signal_emit_by_name (test, "va-marshaller-uint-return", &retval);
  g_assert_cmpint (retval, ==, G_MAXUINT);
  g_signal_handler_disconnect (test, id);

  xobject_unref (test);
}

static xpointer_t
on_generic_marshaller_interface_return (Test *test)
{
  return xobject_new (baa_get_type (), NULL);
}

static void
test_generic_marshaller_signal_interface_return (void)
{
  Test *test;
  xuint_t id;
  xpointer_t retval;

  test = xobject_new (test_get_type (), NULL);

  /* Test return value -30 */
  id = g_signal_connect (test,
                         "generic-marshaller-interface-return",
                         G_CALLBACK (on_generic_marshaller_interface_return),
                         NULL);
  g_signal_emit_by_name (test, "generic-marshaller-interface-return", &retval);
  g_assert_true (xtype_check_instance_is_a ((GTypeInstance*)retval, foo_get_type ()));
  xobject_unref (retval);

  g_signal_handler_disconnect (test, id);

  xobject_unref (test);
}

static const xsignal_invocation_hint_t dont_use_this = { 0, };

static void
custom_marshaller_callback (Test                  *test,
                            xsignal_invocation_hint_t *hint,
                            xpointer_t               unused)
{
  xsignal_invocation_hint_t *ihint;

  g_assert_true (hint != &dont_use_this);

  ihint = g_signal_get_invocation_hint (test);

  g_assert_cmpuint (hint->signal_id, ==, ihint->signal_id);
  g_assert_cmpuint (hint->detail , ==, ihint->detail);
  g_assert_cmpflags (GSignalFlags, hint->run_type, ==, ihint->run_type);
}

static void
test_custom_marshaller (void)
{
  Test *test;

  test = xobject_new (test_get_type (), NULL);

  g_signal_connect (test,
                    "custom-marshaller",
                    G_CALLBACK (custom_marshaller_callback),
                    NULL);

  g_signal_emit_by_name (test, "custom-marshaller", &dont_use_this);

  xobject_unref (test);
}

static int all_type_handlers_count = 0;

static void
all_types_handler (Test *test, int i, xboolean_t b, char c, guchar uc, xuint_t ui, xlong_t l, gulong ul, MyEnum e, MyFlags f, float fl, double db, char *str, xparam_spec_t *param, xbytes_t *bytes, xpointer_t ptr, Test *obj, xvariant_t *var, gint64 i64, xuint64_t ui64)
{
  all_type_handlers_count++;

  g_assert_cmpint (i, ==, 42);
  g_assert_cmpint (b, ==, TRUE);
  g_assert_cmpint (c, ==, 17);
  g_assert_cmpuint (uc, ==, 140);
  g_assert_cmpuint (ui, ==, G_MAXUINT - 42);
  g_assert_cmpint (l, ==, -1117);
  g_assert_cmpuint (ul, ==, G_MAXULONG - 999);
  g_assert_cmpenum (MyEnum, e, ==, MY_ENUM_VALUE);
  g_assert_cmpflags (MyFlags, f, ==, MY_FLAGS_FIRST_BIT | MY_FLAGS_THIRD_BIT | MY_FLAGS_LAST_BIT);
  g_assert_cmpfloat (fl, ==, 0.25);
  g_assert_cmpfloat (db, ==, 1.5);
  g_assert_cmpstr (str, ==, "Test");
  g_assert_cmpstr (g_param_spec_get_nick (param), ==, "nick");
  g_assert_cmpstr (xbytes_get_data (bytes, NULL), ==, "Blah");
  g_assert_true (ptr == &enum_type);
  g_assert_cmpuint (xvariant_get_uint16 (var), == , 99);
  g_assert_cmpint (i64, ==, G_MAXINT64 - 1234);
  g_assert_cmpuint (ui64, ==, G_MAXUINT64 - 123456);
}

static void
all_types_handler_cb (Test *test, int i, xboolean_t b, char c, guchar uc, xuint_t ui, xlong_t l, gulong ul, MyEnum e, xuint_t f, float fl, double db, char *str, xparam_spec_t *param, xbytes_t *bytes, xpointer_t ptr, Test *obj, xvariant_t *var, gint64 i64, xuint64_t ui64, xpointer_t user_data)
{
  g_assert_true (user_data == &flags_type);
  all_types_handler (test, i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, obj, var, i64, ui64);
}

static void
test_all_types (void)
{
  Test *test;

  int i = 42;
  xboolean_t b = TRUE;
  char c = 17;
  guchar uc = 140;
  xuint_t ui = G_MAXUINT - 42;
  xlong_t l =  -1117;
  gulong ul = G_MAXULONG - 999;
  MyEnum e = MY_ENUM_VALUE;
  MyFlags f = MY_FLAGS_FIRST_BIT | MY_FLAGS_THIRD_BIT | MY_FLAGS_LAST_BIT;
  float fl = 0.25;
  double db = 1.5;
  char *str = "Test";
  xparam_spec_t *param = g_param_spec_long	 ("param", "nick", "blurb", 0, 10, 4, 0);
  xbytes_t *bytes = xbytes_new_static ("Blah", 5);
  xpointer_t ptr = &enum_type;
  xvariant_t *var = xvariant_new_uint16 (99);
  gint64 i64;
  xuint64_t ui64;
  xvariant_ref_sink (var);
  i64 = G_MAXINT64 - 1234;
  ui64 = G_MAXUINT64 - 123456;

  test = xobject_new (test_get_type (), NULL);

  all_type_handlers_count = 0;

  g_signal_emit_by_name (test, "all-types",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-va",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-generic",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-empty",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-null",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);

  g_assert_cmpint (all_type_handlers_count, ==, 3);

  all_type_handlers_count = 0;

  g_signal_connect (test, "all-types", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-va", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-generic", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-empty", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-null", G_CALLBACK (all_types_handler_cb), &flags_type);

  g_signal_emit_by_name (test, "all-types",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-va",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-generic",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-empty",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-null",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);

  g_assert_cmpint (all_type_handlers_count, ==, 3 + 5);

  all_type_handlers_count = 0;

  g_signal_connect (test, "all-types", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-va", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-generic", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-empty", G_CALLBACK (all_types_handler_cb), &flags_type);
  g_signal_connect (test, "all-types-null", G_CALLBACK (all_types_handler_cb), &flags_type);

  g_signal_emit_by_name (test, "all-types",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-va",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-generic",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-empty",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);
  g_signal_emit_by_name (test, "all-types-null",
			 i, b, c, uc, ui, l, ul, e, f, fl, db, str, param, bytes, ptr, test, var, i64, ui64);

  g_assert_cmpint (all_type_handlers_count, ==, 3 + 5 + 5);

  xobject_unref (test);
  g_param_spec_unref (param);
  xbytes_unref (bytes);
  xvariant_unref (var);
}

static void
test_connect (void)
{
  xobject_t *test;
  xint_t retval;

  test = xobject_new (test_get_type (), NULL);

  xobject_connect (test,
                    "signal::generic-marshaller-int-return",
                    G_CALLBACK (on_generic_marshaller_int_return_signed_1),
                    NULL,
                    "object-signal::va-marshaller-int-return",
                    G_CALLBACK (on_generic_marshaller_int_return_signed_2),
                    NULL,
                    NULL);
  g_signal_emit_by_name (test, "generic-marshaller-int-return", &retval);
  g_assert_cmpint (retval, ==, -30);
  g_signal_emit_by_name (test, "va-marshaller-int-return", &retval);
  g_assert_cmpint (retval, ==, 2);

  xobject_disconnect (test,
                       "any-signal",
                       G_CALLBACK (on_generic_marshaller_int_return_signed_1),
                       NULL,
                       "any-signal::va-marshaller-int-return",
                       G_CALLBACK (on_generic_marshaller_int_return_signed_2),
                       NULL,
                       NULL);

  xobject_unref (test);
}

static void
simple_handler1 (xobject_t *sender,
                 xobject_t *target)
{
  xobject_unref (target);
}

static void
simple_handler2 (xobject_t *sender,
                 xobject_t *target)
{
  xobject_unref (target);
}

static void
test_destroy_target_object (void)
{
  Test *sender, *target1, *target2;

  sender = xobject_new (test_get_type (), NULL);
  target1 = xobject_new (test_get_type (), NULL);
  target2 = xobject_new (test_get_type (), NULL);
  g_signal_connect_object (sender, "simple", G_CALLBACK (simple_handler1), target1, 0);
  g_signal_connect_object (sender, "simple", G_CALLBACK (simple_handler2), target2, 0);
  g_signal_emit_by_name (sender, "simple");
  xobject_unref (sender);
}

static xboolean_t
hook_func (xsignal_invocation_hint_t *ihint,
           xuint_t                  n_params,
           const xvalue_t          *params,
           xpointer_t               data)
{
  xint_t *count = data;

  (*count)++;

  return TRUE;
}

static void
test_emission_hook (void)
{
  xobject_t *test1, *test2;
  xint_t count = 0;
  gulong hook;

  test1 = xobject_new (test_get_type (), NULL);
  test2 = xobject_new (test_get_type (), NULL);

  hook = g_signal_add_emission_hook (simple_id, 0, hook_func, &count, NULL);
  g_assert_cmpint (count, ==, 0);
  g_signal_emit_by_name (test1, "simple");
  g_assert_cmpint (count, ==, 1);
  g_signal_emit_by_name (test2, "simple");
  g_assert_cmpint (count, ==, 2);
  g_signal_remove_emission_hook (simple_id, hook);
  g_signal_emit_by_name (test1, "simple");
  g_assert_cmpint (count, ==, 2);

  xobject_unref (test1);
  xobject_unref (test2);
}

static void
simple_cb (xpointer_t instance, xpointer_t data)
{
  xsignal_invocation_hint_t *ihint;

  ihint = g_signal_get_invocation_hint (instance);

  g_assert_cmpstr (g_signal_name (ihint->signal_id), ==, "simple");

  g_signal_emit_by_name (instance, "simple-2");
}

static void
simple2_cb (xpointer_t instance, xpointer_t data)
{
  xsignal_invocation_hint_t *ihint;

  ihint = g_signal_get_invocation_hint (instance);

  g_assert_cmpstr (g_signal_name (ihint->signal_id), ==, "simple-2");
}

static void
test_invocation_hint (void)
{
  xobject_t *test;

  test = xobject_new (test_get_type (), NULL);

  g_signal_connect (test, "simple", G_CALLBACK (simple_cb), NULL);
  g_signal_connect (test, "simple-2", G_CALLBACK (simple2_cb), NULL);
  g_signal_emit_by_name (test, "simple");

  xobject_unref (test);
}

static xboolean_t
accumulator_sum (xsignal_invocation_hint_t *ihint,
                 xvalue_t                *return_accu,
                 const xvalue_t          *handler_return,
                 xpointer_t               data)
{
  xint_t acc = xvalue_get_int (return_accu);
  xint_t ret = xvalue_get_int (handler_return);

  g_assert_cmpint (ret, >, 0);

  if (ihint->run_type & G_SIGNAL_ACCUMULATOR_FIRST_RUN)
    {
      g_assert_cmpint (acc, ==, 0);
      g_assert_cmpint (ret, ==, 1);
      g_assert_true (ihint->run_type & G_SIGNAL_RUN_FIRST);
      g_assert_false (ihint->run_type & G_SIGNAL_RUN_LAST);
    }
  else if (ihint->run_type & G_SIGNAL_RUN_FIRST)
    {
      /* Only the first signal handler was called so far */
      g_assert_cmpint (acc, ==, 1);
      g_assert_cmpint (ret, ==, 2);
      g_assert_false (ihint->run_type & G_SIGNAL_RUN_LAST);
    }
  else if (ihint->run_type & G_SIGNAL_RUN_LAST)
    {
      /* Only the first two signal handler were called so far */
      g_assert_cmpint (acc, ==, 3);
      g_assert_cmpint (ret, ==, 3);
      g_assert_false (ihint->run_type & G_SIGNAL_RUN_FIRST);
    }
  else
    {
      g_assert_not_reached ();
    }

  xvalue_set_int (return_accu, acc + ret);

  /* Continue with the other signal handlers as long as the sum is < 6,
   * i.e. don't run simple_accumulator_4_cb() */
  return acc + ret < 6;
}

static xint_t
simple_accumulator_1_cb (xpointer_t instance, xpointer_t data)
{
  return 1;
}

static xint_t
simple_accumulator_2_cb (xpointer_t instance, xpointer_t data)
{
  return 2;
}

static xint_t
simple_accumulator_3_cb (xpointer_t instance, xpointer_t data)
{
  return 3;
}

static xint_t
simple_accumulator_4_cb (xpointer_t instance, xpointer_t data)
{
  return 4;
}

static void
test_accumulator (void)
{
  xobject_t *test;
  xint_t ret = -1;

  test = xobject_new (test_get_type (), NULL);

  /* Connect in reverse order to make sure that LAST signal handlers are
   * called after FIRST signal handlers but signal handlers in each "group"
   * are called in the order they were registered */
  g_signal_connect_after (test, "simple-accumulator", G_CALLBACK (simple_accumulator_3_cb), NULL);
  g_signal_connect_after (test, "simple-accumulator", G_CALLBACK (simple_accumulator_4_cb), NULL);
  g_signal_connect (test, "simple-accumulator", G_CALLBACK (simple_accumulator_1_cb), NULL);
  g_signal_connect (test, "simple-accumulator", G_CALLBACK (simple_accumulator_2_cb), NULL);
  g_signal_emit_by_name (test, "simple-accumulator", &ret);

  /* simple_accumulator_4_cb() is not run because accumulator is 6 */
  g_assert_cmpint (ret, ==, 6);

  xobject_unref (test);
}

static xboolean_t
accumulator_concat_string (xsignal_invocation_hint_t *ihint, xvalue_t *return_accu, const xvalue_t *handler_return, xpointer_t data)
{
  const xchar_t *acc = xvalue_get_string (return_accu);
  const xchar_t *ret = xvalue_get_string (handler_return);

  g_assert_nonnull (ret);

  if (acc == NULL)
    xvalue_set_string (return_accu, ret);
  else
    xvalue_take_string (return_accu, xstrconcat (acc, ret, NULL));

  return TRUE;
}

static xchar_t *
accumulator_class_before_cb (xpointer_t instance, xpointer_t data)
{
  return xstrdup ("before");
}

static xchar_t *
accumulator_class_after_cb (xpointer_t instance, xpointer_t data)
{
  return xstrdup ("after");
}

static xchar_t *
accumulator_class (Test *test)
{
  return xstrdup ("class");
}

static void
test_accumulator_class (void)
{
  const struct {
    const xchar_t *signal_name;
    const xchar_t *return_string;
  } tests[] = {
    {"accumulator-class-first", "classbeforeafter"},
    {"accumulator-class-last", "beforeclassafter"},
    {"accumulator-class-cleanup", "beforeafterclass"},
    {"accumulator-class-first-last", "classbeforeclassafter"},
    {"accumulator-class-first-last-cleanup", "classbeforeclassafterclass"},
    {"accumulator-class-last-cleanup", "beforeclassafterclass"},
  };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xobject_t *test;
      xchar_t *ret = NULL;

      g_test_message ("Signal: %s", tests[i].signal_name);

      test = xobject_new (test_get_type (), NULL);

      g_signal_connect (test, tests[i].signal_name, G_CALLBACK (accumulator_class_before_cb), NULL);
      g_signal_connect_after (test, tests[i].signal_name, G_CALLBACK (accumulator_class_after_cb), NULL);
      g_signal_emit_by_name (test, tests[i].signal_name, &ret);

      g_assert_cmpstr (ret, ==, tests[i].return_string);
      g_free (ret);

      xobject_unref (test);
    }
}

static xboolean_t
in_set (const xchar_t *s,
        const xchar_t *set[])
{
  xint_t i;

  for (i = 0; set[i]; i++)
    {
      if (xstrcmp0 (s, set[i]) == 0)
        return TRUE;
    }

  return FALSE;
}

static void
test_introspection (void)
{
  xuint_t *ids;
  xuint_t n_ids;
  const xchar_t *name;
  xuint_t i;
  const xchar_t *names[] = {
    "simple",
    "simple-detailed",
    "simple-2",
    "simple-accumulator",
    "accumulator-class-first",
    "accumulator-class-last",
    "accumulator-class-cleanup",
    "accumulator-class-first-last",
    "accumulator-class-first-last-cleanup",
    "accumulator-class-last-cleanup",
    "generic-marshaller-1",
    "generic-marshaller-2",
    "generic-marshaller-enum-return-signed",
    "generic-marshaller-enum-return-unsigned",
    "generic-marshaller-int-return",
    "va-marshaller-int-return",
    "generic-marshaller-uint-return",
    "generic-marshaller-interface-return",
    "va-marshaller-uint-return",
    "variant-changed-no-slot",
    "variant-changed",
    "all-types",
    "all-types-va",
    "all-types-generic",
    "all-types-null",
    "all-types-empty",
    "custom-marshaller",
    NULL
  };
  GSignalQuery query;

  ids = g_signal_list_ids (test_get_type (), &n_ids);
  g_assert_cmpuint (n_ids, ==, xstrv_length ((xchar_t**)names));

  for (i = 0; i < n_ids; i++)
    {
      name = g_signal_name (ids[i]);
      g_assert_true (in_set (name, names));
    }

  g_signal_query (simple_id, &query);
  g_assert_cmpuint (query.signal_id, ==, simple_id);
  g_assert_cmpstr (query.signal_name, ==, "simple");
  g_assert_true (query.itype == test_get_type ());
  g_assert_cmpint (query.signal_flags, ==, G_SIGNAL_RUN_LAST);
  g_assert_cmpint (query.return_type, ==, XTYPE_NONE);
  g_assert_cmpuint (query.n_params, ==, 0);

  g_free (ids);
}

static void
test_handler (xpointer_t instance, xpointer_t data)
{
  xint_t *count = data;

  (*count)++;
}

static void
test_block_handler (void)
{
  xobject_t *test1, *test2;
  xint_t count1 = 0;
  xint_t count2 = 0;
  gulong handler1, handler;

  test1 = xobject_new (test_get_type (), NULL);
  test2 = xobject_new (test_get_type (), NULL);

  handler1 = g_signal_connect (test1, "simple", G_CALLBACK (test_handler), &count1);
  g_signal_connect (test2, "simple", G_CALLBACK (test_handler), &count2);

  handler = g_signal_handler_find (test1, G_SIGNAL_MATCH_ID, simple_id, 0, NULL, NULL, NULL);

  g_assert_true (handler == handler1);

  g_assert_cmpint (count1, ==, 0);
  g_assert_cmpint (count2, ==, 0);

  g_signal_emit_by_name (test1, "simple");
  g_signal_emit_by_name (test2, "simple");

  g_assert_cmpint (count1, ==, 1);
  g_assert_cmpint (count2, ==, 1);

  g_signal_handler_block (test1, handler1);

  g_signal_emit_by_name (test1, "simple");
  g_signal_emit_by_name (test2, "simple");

  g_assert_cmpint (count1, ==, 1);
  g_assert_cmpint (count2, ==, 2);

  g_signal_handler_unblock (test1, handler1);

  g_signal_emit_by_name (test1, "simple");
  g_signal_emit_by_name (test2, "simple");

  g_assert_cmpint (count1, ==, 2);
  g_assert_cmpint (count2, ==, 3);

  g_assert_cmpuint (g_signal_handlers_block_matched (test1, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, test_block_handler, NULL), ==, 0);
  g_assert_cmpuint (g_signal_handlers_block_matched (test2, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, test_handler, NULL), ==, 1);

  g_signal_emit_by_name (test1, "simple");
  g_signal_emit_by_name (test2, "simple");

  g_assert_cmpint (count1, ==, 3);
  g_assert_cmpint (count2, ==, 3);

  g_signal_handlers_unblock_matched (test2, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, test_handler, NULL);

  xobject_unref (test1);
  xobject_unref (test2);
}

static void
stop_emission (xpointer_t instance, xpointer_t data)
{
  g_signal_stop_emission (instance, simple_id, 0);
}

static void
stop_emission_by_name (xpointer_t instance, xpointer_t data)
{
  g_signal_stop_emission_by_name (instance, "simple");
}

static void
dont_reach (xpointer_t instance, xpointer_t data)
{
  g_assert_not_reached ();
}

static void
test_stop_emission (void)
{
  xobject_t *test1;
  gulong handler;

  test1 = xobject_new (test_get_type (), NULL);
  handler = g_signal_connect (test1, "simple", G_CALLBACK (stop_emission), NULL);
  g_signal_connect_after (test1, "simple", G_CALLBACK (dont_reach), NULL);

  g_signal_emit_by_name (test1, "simple");

  g_signal_handler_disconnect (test1, handler);
  g_signal_connect (test1, "simple", G_CALLBACK (stop_emission_by_name), NULL);

  g_signal_emit_by_name (test1, "simple");

  xobject_unref (test1);
}

static void
test_signal_disconnect_wrong_object (void)
{
  Test *object, *object2;
  Test2 *object3;
  xuint_t signal_id;

  object = xobject_new (test_get_type (), NULL);
  object2 = xobject_new (test_get_type (), NULL);
  object3 = xobject_new (test2_get_type (), NULL);

  signal_id = g_signal_connect (object,
                                "simple",
                                G_CALLBACK (simple_handler1),
                                NULL);

  /* disconnect from the wrong object (same type), should warn */
  g_test_expect_message ("GLib-xobject_t", G_LOG_LEVEL_WARNING,
                         "*: instance '*' has no handler with id '*'");
  g_signal_handler_disconnect (object2, signal_id);
  g_test_assert_expected_messages ();

  /* and from an object of the wrong type */
  g_test_expect_message ("GLib-xobject_t", G_LOG_LEVEL_WARNING,
                         "*: instance '*' has no handler with id '*'");
  g_signal_handler_disconnect (object3, signal_id);
  g_test_assert_expected_messages ();

  /* it's still connected */
  g_assert_true (g_signal_handler_is_connected (object, signal_id));

  xobject_unref (object);
  xobject_unref (object2);
  xobject_unref (object3);
}

static void
test_clear_signal_handler (void)
{
  xobject_t *test_obj;
  gulong handler;

  test_obj = xobject_new (test_get_type (), NULL);

  handler = g_signal_connect (test_obj, "simple", G_CALLBACK (dont_reach), NULL);
  g_assert_cmpuint (handler, >, 0);

  g_clear_signal_handler (&handler, test_obj);
  g_assert_cmpuint (handler, ==, 0);

  g_signal_emit_by_name (test_obj, "simple");

  g_clear_signal_handler (&handler, test_obj);

  if (g_test_undefined ())
    {
      handler = g_random_int_range (0x01, 0xFF);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                             "*instance '* has no handler with id *'");
      g_clear_signal_handler (&handler, test_obj);
      g_assert_cmpuint (handler, ==, 0);
      g_test_assert_expected_messages ();
    }

  xobject_unref (test_obj);
}

static void
test_lookup (void)
{
  xtype_class_t *test_class;
  xuint_t signal_id, saved_signal_id;

  g_test_summary ("Test that g_signal_lookup() works with a variety of inputs.");

  test_class = xtype_class_ref (test_get_type ());

  signal_id = g_signal_lookup ("all-types", test_get_type ());
  g_assert_cmpint (signal_id, !=, 0);

  saved_signal_id = signal_id;

  /* Try with a non-canonical name. */
  signal_id = g_signal_lookup ("all_types", test_get_type ());
  g_assert_cmpint (signal_id, ==, saved_signal_id);

  /* Looking up a non-existent signal should return nothing. */
  g_assert_cmpint (g_signal_lookup ("nope", test_get_type ()), ==, 0);

  xtype_class_unref (test_class);
}

static void
test_lookup_invalid (void)
{
  g_test_summary ("Test that g_signal_lookup() emits a warning if looking up an invalid signal name.");

  if (g_test_subprocess ())
    {
      xtype_class_t *test_class;
      xuint_t signal_id;

      test_class = xtype_class_ref (test_get_type ());

      signal_id = g_signal_lookup ("", test_get_type ());
      g_assert_cmpint (signal_id, ==, 0);

      xtype_class_unref (test_class);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*WARNING*unable to look up invalid signal name*");
}

static void
test_parse_name (void)
{
  xtype_class_t *test_class;
  xuint_t signal_id, saved_signal_id;
  xboolean_t retval;
  xquark detail, saved_detail;

  g_test_summary ("Test that g_signal_parse_name() works with a variety of inputs.");

  test_class = xtype_class_ref (test_get_type ());

  /* Simple test. */
  retval = g_signal_parse_name ("simple-detailed", test_get_type (), &signal_id, &detail, TRUE);
  g_assert_true (retval);
  g_assert_cmpint (signal_id, !=, 0);
  g_assert_cmpint (detail, ==, 0);

  saved_signal_id = signal_id;

  /* Simple test with detail. */
  retval = g_signal_parse_name ("simple-detailed::a-detail", test_get_type (), &signal_id, &detail, TRUE);
  g_assert_true (retval);
  g_assert_cmpint (signal_id, ==, saved_signal_id);
  g_assert_cmpint (detail, !=, 0);

  saved_detail = detail;

  /* Simple test with the same detail again. */
  retval = g_signal_parse_name ("simple-detailed::a-detail", test_get_type (), &signal_id, &detail, FALSE);
  g_assert_true (retval);
  g_assert_cmpint (signal_id, ==, saved_signal_id);
  g_assert_cmpint (detail, ==, saved_detail);

  /* Simple test with a new detail. */
  retval = g_signal_parse_name ("simple-detailed::another-detail", test_get_type (), &signal_id, &detail, FALSE);
  g_assert_true (retval);
  g_assert_cmpint (signal_id, ==, saved_signal_id);
  g_assert_cmpint (detail, ==, 0);  /* we didn’t force the quark */

  /* Canonicalisation shouldn’t affect the results. */
  retval = g_signal_parse_name ("simple_detailed::a-detail", test_get_type (), &signal_id, &detail, FALSE);
  g_assert_true (retval);
  g_assert_cmpint (signal_id, ==, saved_signal_id);
  g_assert_cmpint (detail, ==, saved_detail);

  /* Details don’t have to look like property names. */
  retval = g_signal_parse_name ("simple-detailed::hello::world", test_get_type (), &signal_id, &detail, TRUE);
  g_assert_true (retval);
  g_assert_cmpint (signal_id, ==, saved_signal_id);
  g_assert_cmpint (detail, !=, 0);

  /* Trying to parse a detail for a signal which isn’t %G_SIGNAL_DETAILED should fail. */
  retval = g_signal_parse_name ("all-types::a-detail", test_get_type (), &signal_id, &detail, FALSE);
  g_assert_false (retval);

  xtype_class_unref (test_class);
}

static void
test_parse_name_invalid (void)
{
  xtype_class_t *test_class;
  xsize_t i;
  xuint_t signal_id;
  xquark detail;
  const xchar_t *vectors[] =
    {
      "",
      "7zip",
      "invalid:signal",
      "simple-detailed::",
      "simple-detailed:",
      ":",
      "::",
      ":valid-detail",
      "::valid-detail",
    };

  g_test_summary ("Test that g_signal_parse_name() ignores a variety of invalid inputs.");

  test_class = xtype_class_ref (test_get_type ());

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      g_test_message ("Parser input: %s", vectors[i]);
      g_assert_false (g_signal_parse_name (vectors[i], test_get_type (), &signal_id, &detail, TRUE));
    }

  xtype_class_unref (test_class);
}

static void
test_signals_invalid_name (xconstpointer test_data)
{
  const xchar_t *signal_name = test_data;

  g_test_summary ("Check that g_signal_new() rejects invalid signal names.");

  if (g_test_subprocess ())
    {
      g_signal_new (signal_name,
                    test_get_type (),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                    0,
                    NULL, NULL,
                    NULL,
                    XTYPE_NONE,
                    0);
      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*g_signal_is_valid_name (signal_name)*");
}

static void
test_signal_is_valid_name (void)
{
  const xchar_t *valid_names[] =
    {
      "signal",
      "i",
      "multiple-segments",
      "segment0-SEGMENT1",
      "using_underscores",
    };
  const xchar_t *invalid_names[] =
    {
      "",
      "7zip",
      "my_int:hello",
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (valid_names); i++)
    g_assert_true (g_signal_is_valid_name (valid_names[i]));

  for (i = 0; i < G_N_ELEMENTS (invalid_names); i++)
    g_assert_false (g_signal_is_valid_name (invalid_names[i]));
}

/* --- */

int
main (int argc,
     char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/signals/all-types", test_all_types);
  g_test_add_func ("/gobject/signals/variant", test_variant_signal);
  g_test_add_func ("/gobject/signals/destroy-target-object", test_destroy_target_object);
  g_test_add_func ("/gobject/signals/generic-marshaller-1", test_generic_marshaller_signal_1);
  g_test_add_func ("/gobject/signals/generic-marshaller-2", test_generic_marshaller_signal_2);
  g_test_add_func ("/gobject/signals/generic-marshaller-enum-return-signed", test_generic_marshaller_signal_enum_return_signed);
  g_test_add_func ("/gobject/signals/generic-marshaller-enum-return-unsigned", test_generic_marshaller_signal_enum_return_unsigned);
  g_test_add_func ("/gobject/signals/generic-marshaller-int-return", test_generic_marshaller_signal_int_return);
  g_test_add_func ("/gobject/signals/generic-marshaller-uint-return", test_generic_marshaller_signal_uint_return);
  g_test_add_func ("/gobject/signals/generic-marshaller-interface-return", test_generic_marshaller_signal_interface_return);
  g_test_add_func ("/gobject/signals/custom-marshaller", test_custom_marshaller);
  g_test_add_func ("/gobject/signals/connect", test_connect);
  g_test_add_func ("/gobject/signals/emission-hook", test_emission_hook);
  g_test_add_func ("/gobject/signals/accumulator", test_accumulator);
  g_test_add_func ("/gobject/signals/accumulator-class", test_accumulator_class);
  g_test_add_func ("/gobject/signals/introspection", test_introspection);
  g_test_add_func ("/gobject/signals/block-handler", test_block_handler);
  g_test_add_func ("/gobject/signals/stop-emission", test_stop_emission);
  g_test_add_func ("/gobject/signals/invocation-hint", test_invocation_hint);
  g_test_add_func ("/gobject/signals/test-disconnection-wrong-object", test_signal_disconnect_wrong_object);
  g_test_add_func ("/gobject/signals/clear-signal-handler", test_clear_signal_handler);
  g_test_add_func ("/gobject/signals/lookup", test_lookup);
  g_test_add_func ("/gobject/signals/lookup/invalid", test_lookup_invalid);
  g_test_add_func ("/gobject/signals/parse-name", test_parse_name);
  g_test_add_func ("/gobject/signals/parse-name/invalid", test_parse_name_invalid);
  g_test_add_data_func ("/gobject/signals/invalid-name/colon", "my_int:hello", test_signals_invalid_name);
  g_test_add_data_func ("/gobject/signals/invalid-name/first-char", "7zip", test_signals_invalid_name);
  g_test_add_data_func ("/gobject/signals/invalid-name/empty", "", test_signals_invalid_name);
  g_test_add_func ("/gobject/signals/is-valid-name", test_signal_is_valid_name);

  return g_test_run ();
}
