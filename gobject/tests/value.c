/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#define XPL_VERSION_MIN_REQUIRED       XPL_VERSION_2_30

#include <glib.h>
#include <glib-object.h>
#include "gobject/gvaluecollector.h"

static void
test_enum_transformation (void)
{
  xtype_t type;
  GValue orig = G_VALUE_INIT;
  GValue xform = G_VALUE_INIT;
  GEnumValue values[] = { {0,"0","0"}, {1,"1","1"}};

 type = g_enum_register_static ("TestEnum", values);

 g_value_init (&orig, type);
 g_value_set_enum (&orig, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_CHAR);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_char (&xform), ==, 1);
 g_assert_cmpint (g_value_get_schar (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_UCHAR);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_uchar (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_INT);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_int (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_UINT);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_uint (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_LONG);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_long (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_ULONG);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_ulong (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_INT64);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_int64 (&xform), ==, 1);

 memset (&xform, 0, sizeof (GValue));
 g_value_init (&xform, XTYPE_UINT64);
 g_value_transform (&orig, &xform);
 g_assert_cmpint (g_value_get_uint64 (&xform), ==, 1);
}


static void
test_gtype_value (void)
{
  xtype_t type;
  GValue value = G_VALUE_INIT;
  GValue copy = G_VALUE_INIT;

  g_value_init (&value, XTYPE_GTYPE);

  g_value_set_gtype (&value, XTYPE_BOXED);
  type = g_value_get_gtype (&value);
  g_assert_true (type == XTYPE_BOXED);

  g_value_init (&copy, XTYPE_GTYPE);
  g_value_copy (&value, &copy);
  type = g_value_get_gtype (&copy);
  g_assert_true (type == XTYPE_BOXED);
}

static xchar_t *
collect (GValue *value, ...)
{
  xchar_t *error;
  va_list var_args;

  error = NULL;

  va_start (var_args, value);
  G_VALUE_COLLECT (value, var_args, 0, &error);
  va_end (var_args);

  return error;
}

static xchar_t *
lcopy (GValue *value, ...)
{
  xchar_t *error;
  va_list var_args;

  error = NULL;

  va_start (var_args, value);
  G_VALUE_LCOPY (value, var_args, 0, &error);
  va_end (var_args);

  return error;
}

static void
test_collection (void)
{
  GValue value = G_VALUE_INIT;
  xchar_t *error;

  g_value_init (&value, XTYPE_CHAR);
  error = collect (&value, 'c');
  g_assert_null (error);
  g_assert_cmpint (g_value_get_char (&value), ==, 'c');

  g_value_unset (&value);
  g_value_init (&value, XTYPE_UCHAR);
  error = collect (&value, 129);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_uchar (&value), ==, 129);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_BOOLEAN);
  error = collect (&value, TRUE);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_boolean (&value), ==, TRUE);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_INT);
  error = collect (&value, G_MAXINT);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_int (&value), ==, G_MAXINT);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_UINT);
  error = collect (&value, G_MAXUINT);
  g_assert_null (error);
  g_assert_cmpuint (g_value_get_uint (&value), ==, G_MAXUINT);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_LONG);
  error = collect (&value, G_MAXLONG);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_long (&value), ==, G_MAXLONG);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_ULONG);
  error = collect (&value, G_MAXULONG);
  g_assert_null (error);
  g_assert_cmpuint (g_value_get_ulong (&value), ==, G_MAXULONG);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_INT64);
  error = collect (&value, G_MAXINT64);
  g_assert_null (error);
  g_assert_cmpint (g_value_get_int64 (&value), ==, G_MAXINT64);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_UINT64);
  error = collect (&value, G_MAXUINT64);
  g_assert_null (error);
  g_assert_cmpuint (g_value_get_uint64 (&value), ==, G_MAXUINT64);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_FLOAT);
  error = collect (&value, G_MAXFLOAT);
  g_assert_null (error);
  g_assert_cmpfloat (g_value_get_float (&value), ==, G_MAXFLOAT);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_DOUBLE);
  error = collect (&value, G_MAXDOUBLE);
  g_assert_null (error);
  g_assert_cmpfloat (g_value_get_double (&value), ==, G_MAXDOUBLE);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_STRING);
  error = collect (&value, "string ?");
  g_assert_null (error);
  g_assert_cmpstr (g_value_get_string (&value), ==, "string ?");

  g_value_unset (&value);
  g_value_init (&value, XTYPE_GTYPE);
  error = collect (&value, XTYPE_BOXED);
  g_assert_null (error);
  g_assert_true (g_value_get_gtype (&value) == XTYPE_BOXED);

  g_value_unset (&value);
  g_value_init (&value, XTYPE_VARIANT);
  error = collect (&value, g_variant_new_uint32 (42));
  g_assert_null (error);
  g_assert_true (g_variant_is_of_type (g_value_get_variant (&value),
                                       G_VARIANT_TYPE ("u")));
  g_assert_cmpuint (g_variant_get_uint32 (g_value_get_variant (&value)), ==, 42);

  g_value_unset (&value);
}

static void
test_copying (void)
{
  GValue value = G_VALUE_INIT;
  xchar_t *error;

  {
    xchar_t c = 0;

    g_value_init (&value, XTYPE_CHAR);
    g_value_set_char (&value, 'c');
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpint (c, ==, 'c');
  }

  {
    guchar c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_UCHAR);
    g_value_set_uchar (&value, 129);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpint (c, ==, 129);
  }

  {
    xint_t c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_INT);
    g_value_set_int (&value, G_MAXINT);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpint (c, ==, G_MAXINT);
  }

  {
    xuint_t c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_UINT);
    g_value_set_uint (&value, G_MAXUINT);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpuint (c, ==, G_MAXUINT);
  }

  {
    glong c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_LONG);
    g_value_set_long (&value, G_MAXLONG);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXLONG);
  }

  {
    gulong c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_ULONG);
    g_value_set_ulong (&value, G_MAXULONG);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXULONG);
  }

  {
    gint64 c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_INT64);
    g_value_set_int64 (&value, G_MAXINT64);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXINT64);
  }

  {
    guint64 c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_UINT64);
    g_value_set_uint64 (&value, G_MAXUINT64);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXUINT64);
  }

  {
    gfloat c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_FLOAT);
    g_value_set_float (&value, G_MAXFLOAT);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXFLOAT);
  }

  {
    xdouble_t c = 0;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_DOUBLE);
    g_value_set_double (&value, G_MAXDOUBLE);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert (c == G_MAXDOUBLE);
  }

  {
    xchar_t *c = NULL;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_STRING);
    g_value_set_string (&value, "string ?");
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_cmpstr (c, ==, "string ?");
    g_free (c);
  }

  {
    xtype_t c = XTYPE_NONE;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_GTYPE);
    g_value_set_gtype (&value, XTYPE_BOXED);
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_true (c == XTYPE_BOXED);
  }

  {
    xvariant_t *c = NULL;

    g_value_unset (&value);
    g_value_init (&value, XTYPE_VARIANT);
    g_value_set_variant (&value, g_variant_new_uint32 (42));
    error = lcopy (&value, &c);
    g_assert_null (error);
    g_assert_nonnull (c);
    g_assert (g_variant_is_of_type (c, G_VARIANT_TYPE ("u")));
    g_assert_cmpuint (g_variant_get_uint32 (c), ==, 42);
    g_variant_unref (c);
    g_value_unset (&value);
  }
}

static void
test_value_basic (void)
{
  GValue value = G_VALUE_INIT;

  g_assert_false (X_IS_VALUE (&value));
  g_assert_false (G_VALUE_HOLDS_INT (&value));
  g_value_unset (&value);
  g_assert_false (X_IS_VALUE (&value));
  g_assert_false (G_VALUE_HOLDS_INT (&value));

  g_value_init (&value, XTYPE_INT);
  g_assert_true (X_IS_VALUE (&value));
  g_assert_true (G_VALUE_HOLDS_INT (&value));
  g_assert_false (G_VALUE_HOLDS_UINT (&value));
  g_assert_cmpint (g_value_get_int (&value), ==, 0);

  g_value_set_int (&value, 10);
  g_assert_cmpint (g_value_get_int (&value), ==, 10);

  g_value_reset (&value);
  g_assert_true (X_IS_VALUE (&value));
  g_assert_true (G_VALUE_HOLDS_INT (&value));
  g_assert_cmpint (g_value_get_int (&value), ==, 0);

  g_value_unset (&value);
  g_assert_false (X_IS_VALUE (&value));
  g_assert_false (G_VALUE_HOLDS_INT (&value));
}

static void
test_value_string (void)
{
  const xchar_t *static1 = "static1";
  const xchar_t *static2 = "static2";
  const xchar_t *storedstr;
  const xchar_t *copystr;
  xchar_t *str1, *str2;
  GValue value = G_VALUE_INIT;
  GValue copy = G_VALUE_INIT;

  g_test_summary ("Test that XTYPE_STRING GValue copy properly");

  /*
   * Regular strings (ownership not passed)
   */

  /* Create a regular string gvalue and make sure it copies the provided string */
  g_value_init (&value, XTYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));

  /* The string contents should be empty at this point */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == NULL);

  g_value_set_string (&value, static1);
  /* The contents should be a copy of the same string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static1);
  g_assert_cmpstr (storedstr, ==, static1);
  /* Check g_value_dup_string() provides a copy */
  str1 = g_value_dup_string (&value);
  g_assert_true (storedstr != str1);
  g_assert_cmpstr (str1, ==, static1);
  g_free (str1);

  /* Copying a regular string gvalue should copy the contents */
  g_value_init (&copy, XTYPE_STRING);
  g_value_copy (&value, &copy);
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr != storedstr);
  g_assert_cmpstr (copystr, ==, static1);
  g_value_unset (&copy);

  /* Setting a new string should change the contents */
  g_value_set_string (&value, static2);
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Setting a static string over that should also change it (test for
   * coverage and valgrind) */
  g_value_set_static_string (&value, static1);
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static1);

  /* Giving a string directly (ownership passed) should replace the content */
  str2 = g_strdup (static2);
  g_value_take_string (&value, str2);
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, str2);

  g_value_unset (&value);

  /*
   * Regular strings (ownership passed)
   */

  g_value_init (&value, XTYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  str1 = g_strdup (static1);
  g_value_take_string (&value, str1);
  /* The contents should be the string we provided */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == str1);
  /* But g_value_dup_string() should provide a copy */
  str2 = g_value_dup_string (&value);
  g_assert_true (storedstr != str2);
  g_assert_cmpstr (str2, ==, static1);
  g_free (str2);

  /* Copying a regular string gvalue (even with ownership passed) should copy
   * the contents */
  g_value_init (&copy, XTYPE_STRING);
  g_value_copy (&value, &copy);
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr != storedstr);
  g_assert_cmpstr (copystr, ==, static1);
  g_value_unset (&copy);

  /* Setting a new regular string should change the contents */
  g_value_set_string (&value, static2);
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static2);

  g_value_unset (&value);

  /*
   * Static strings
   */
  g_value_init (&value, XTYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  g_value_set_static_string (&value, static1);
  /* The contents should be the string we provided */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == static1);
  /* But g_value_dup_string() should provide a copy */
  str2 = g_value_dup_string (&value);
  g_assert_true (storedstr != str2);
  g_assert_cmpstr (str2, ==, static1);
  g_free (str2);

  /* Copying a static string gvalue should *actually* copy the contents */
  g_value_init (&copy, XTYPE_STRING);
  g_value_copy (&value, &copy);
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr != static1);
  g_value_unset (&copy);

  /* Setting a new string should change the contents */
  g_value_set_static_string (&value, static2);
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static1);
  g_assert_cmpstr (storedstr, ==, static2);

  g_value_unset (&value);

  /*
   * Interned/Canonical strings
   */
  static1 = g_intern_static_string (static1);
  g_value_init (&value, XTYPE_STRING);
  g_assert_true (G_VALUE_HOLDS_STRING (&value));
  g_value_set_interned_string (&value, static1);
  g_assert_true (G_VALUE_IS_INTERNED_STRING (&value));
  /* The contents should be the string we provided */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr == static1);
  /* But g_value_dup_string() should provide a copy */
  str2 = g_value_dup_string (&value);
  g_assert_true (storedstr != str2);
  g_assert_cmpstr (str2, ==, static1);
  g_free (str2);

  /* Copying an interned string gvalue should *not* copy the contents
   * and should still be an interned string */
  g_value_init (&copy, XTYPE_STRING);
  g_value_copy (&value, &copy);
  g_assert_true (G_VALUE_IS_INTERNED_STRING (&copy));
  copystr = g_value_get_string (&copy);
  g_assert_true (copystr == static1);
  g_value_unset (&copy);

  /* Setting a new interned string should change the contents */
  static2 = g_intern_static_string (static2);
  g_value_set_interned_string (&value, static2);
  g_assert_true (G_VALUE_IS_INTERNED_STRING (&value));
  /* The contents should be the interned string */
  storedstr = g_value_get_string (&value);
  g_assert_cmpstr (storedstr, ==, static2);

  /* Setting a new regular string should change the contents */
  g_value_set_string (&value, static2);
  g_assert_false (G_VALUE_IS_INTERNED_STRING (&value));
  /* The contents should be a copy of that *new* string */
  storedstr = g_value_get_string (&value);
  g_assert_true (storedstr != static2);
  g_assert_cmpstr (storedstr, ==, static2);

  g_value_unset (&value);
}

static xint_t
cmpint (gconstpointer a, gconstpointer b)
{
  const GValue *aa = a;
  const GValue *bb = b;

  return g_value_get_int (aa) - g_value_get_int (bb);
}

static void
test_valuearray_basic (void)
{
  GValueArray *a;
  GValueArray *a2;
  GValue v = G_VALUE_INIT;
  GValue *p;
  xuint_t i;

  a = g_value_array_new (20);

  g_value_init (&v, XTYPE_INT);
  for (i = 0; i < 100; i++)
    {
      g_value_set_int (&v, i);
      g_value_array_append (a, &v);
    }

  g_assert_cmpint (a->n_values, ==, 100);
  p = g_value_array_get_nth (a, 5);
  g_assert_cmpint (g_value_get_int (p), ==, 5);

  for (i = 20; i < 100; i+= 5)
    g_value_array_remove (a, 100 - i);

  for (i = 100; i < 150; i++)
    {
      g_value_set_int (&v, i);
      g_value_array_prepend (a, &v);
    }

  g_value_array_sort (a, cmpint);
  for (i = 0; i < a->n_values - 1; i++)
    g_assert_cmpint (g_value_get_int (&a->values[i]), <=, g_value_get_int (&a->values[i+1]));

  a2 = g_value_array_copy (a);
  for (i = 0; i < a->n_values; i++)
    g_assert_cmpint (g_value_get_int (&a->values[i]), ==, g_value_get_int (&a2->values[i]));

  g_value_array_free (a);
  g_value_array_free (a2);
}

/* We create some dummy objects with this relationship:
 *
 *               xobject_t           TestInterface
 *              /       \         /  /
 *     TestObjectA     TestObjectB  /
 *      /       \                  /
 * TestObjectA1 TestObjectA2-------
 *
 * ie: TestObjectA1 and TestObjectA2 are subclasses of TestObjectA
 * and TestObjectB is related to neither. TestObjectA2 and TestObjectB
 * implement TestInterface
 */

typedef xtype_interface_t TestInterfaceInterface;
static xtype_t test_interface_get_type (void);
G_DEFINE_INTERFACE (TestInterface, test_interface, XTYPE_OBJECT)
static void test_interface_default_init (TestInterfaceInterface *iface) { }

static xtype_t test_object_a_get_type (void);
typedef xobject_t TestObjectA; typedef xobject_class_t TestObjectAClass;
G_DEFINE_TYPE (TestObjectA, test_object_a, XTYPE_OBJECT)
static void test_object_a_class_init (TestObjectAClass *class) { }
static void test_object_a_init (TestObjectA *a) { }

static xtype_t test_object_b_get_type (void);
typedef xobject_t TestObjectB; typedef xobject_class_t TestObjectBClass;
static void test_object_b_iface_init (TestInterfaceInterface *iface) { }
G_DEFINE_TYPE_WITH_CODE (TestObjectB, test_object_b, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (test_interface_get_type (), test_object_b_iface_init))
static void test_object_b_class_init (TestObjectBClass *class) { }
static void test_object_b_init (TestObjectB *b) { }

static xtype_t test_object_a1_get_type (void);
typedef xobject_t TestObjectA1; typedef xobject_class_t TestObjectA1Class;
G_DEFINE_TYPE (TestObjectA1, test_object_a1, test_object_a_get_type ())
static void test_object_a1_class_init (TestObjectA1Class *class) { }
static void test_object_a1_init (TestObjectA1 *c) { }

static xtype_t test_object_a2_get_type (void);
typedef xobject_t TestObjectA2; typedef xobject_class_t TestObjectA2Class;
static void test_object_a2_iface_init (TestInterfaceInterface *iface) { }
G_DEFINE_TYPE_WITH_CODE (TestObjectA2, test_object_a2, test_object_a_get_type (),
                         G_IMPLEMENT_INTERFACE (test_interface_get_type (), test_object_a2_iface_init))
static void test_object_a2_class_init (TestObjectA2Class *class) { }
static void test_object_a2_init (TestObjectA2 *b) { }

static void
test_value_transform_object (void)
{
  GValue src = G_VALUE_INIT;
  GValue dest = G_VALUE_INIT;
  xobject_t *object;
  xuint_t i, s, d;
  xtype_t types[] = {
    XTYPE_OBJECT,
    test_interface_get_type (),
    test_object_a_get_type (),
    test_object_b_get_type (),
    test_object_a1_get_type (),
    test_object_a2_get_type ()
  };

  for (i = 0; i < G_N_ELEMENTS (types); i++)
    {
      if (!XTYPE_IS_CLASSED (types[i]))
        continue;

      object = g_object_new (types[i], NULL);

      for (s = 0; s < G_N_ELEMENTS (types); s++)
        {
          if (!XTYPE_CHECK_INSTANCE_TYPE (object, types[s]))
            continue;

          g_value_init (&src, types[s]);
          g_value_set_object (&src, object);

          for (d = 0; d < G_N_ELEMENTS (types); d++)
            {
              g_test_message ("Next: %s object in GValue of %s to GValue of %s", g_type_name (types[i]), g_type_name (types[s]), g_type_name (types[d]));
              g_assert_true (g_value_type_transformable (types[s], types[d]));
              g_value_init (&dest, types[d]);
              g_assert_true (g_value_transform (&src, &dest));
              g_assert_cmpint (g_value_get_object (&dest) != NULL, ==, XTYPE_CHECK_INSTANCE_TYPE (object, types[d]));
              g_value_unset (&dest);
            }
          g_value_unset (&src);
        }

      g_object_unref (object);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/value/basic", test_value_basic);
  g_test_add_func ("/value/array/basic", test_valuearray_basic);
  g_test_add_func ("/value/collection", test_collection);
  g_test_add_func ("/value/copying", test_copying);
  g_test_add_func ("/value/enum-transformation", test_enum_transformation);
  g_test_add_func ("/value/gtype", test_gtype_value);
  g_test_add_func ("/value/string", test_value_string);
  g_test_add_func ("/value/transform-object", test_value_transform_object);

  return g_test_run ();
}
