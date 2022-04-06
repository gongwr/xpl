/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * override.c: Closure override test program
 * Copyright (C) 2001, James Henstridge
 * Copyright (C) 2003, Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#undef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN "TestOverride"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#undef VERBOSE

#include <string.h>

#include <glib.h>
#include <glib-object.h>

#include "testcommon.h"

static xuint_t foo_signal_id = 0;
static xuint_t bar_signal_id = 0;
static xuint_t baz_signal_id = 0;

static xtype_t test_i_get_type (void);
static xtype_t test_a_get_type (void);
static xtype_t test_b_get_type (void);
static xtype_t test_c_get_type (void);

static void  record (const xchar_t *str);

#define TEST_TYPE_I (test_i_get_type ())

typedef struct _TestI TestI;
typedef struct _TestIClass TestIClass;

struct _TestIClass
{
  xtype_interface_t base_iface;
};

static void
test_i_foo (TestI *self)
{
  record ("TestI::foo");
}

static void
test_i_default_init (xpointer_t g_class)
{
  foo_signal_id = g_signal_newv ("foo",
				 TEST_TYPE_I,
				 G_SIGNAL_RUN_LAST,
				 g_cclosure_new(G_CALLBACK(test_i_foo),
						NULL, NULL),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__VOID,
				 XTYPE_NONE, 0, NULL);
}

static DEFINE_IFACE (TestI, test_i, NULL, test_i_default_init)

#define TEST_TYPE_A (test_a_get_type())

     typedef struct _TestA TestA;
     typedef struct _TestAClass TestAClass;

struct _TestA {
  xobject_t parent;
};
struct _TestAClass {
  xobject_class_t parent_class;

  void (* bar) (TestA *self);
};

static void
test_a_foo (TestI *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("TestA::foo");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  g_assert (g_signal_get_invocation_hint (self)->signal_id == foo_signal_id);
  g_signal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static void
test_a_bar (TestA *self)
{
  record ("TestA::bar");
}

static xchar_t *
test_a_baz (TestA    *self,
            xobject_t  *object,
            xpointer_t  pointer)
{
  record ("TestA::baz");

  g_assert (object == G_OBJECT (self));
  g_assert (GPOINTER_TO_INT (pointer) == 23);

  return xstrdup ("TestA::baz");
}

static void
test_a_class_init (TestAClass *class)
{
  class->bar = test_a_bar;

  bar_signal_id = g_signal_new ("bar",
				TEST_TYPE_A,
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TestAClass, bar),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				XTYPE_NONE, 0, NULL);

  baz_signal_id = g_signal_new_class_handler ("baz",
                                              TEST_TYPE_A,
                                              G_SIGNAL_RUN_LAST,
                                              G_CALLBACK (test_a_baz),
                                              NULL, NULL,
                                              g_cclosure_marshal_STRING__OBJECT_POINTER,
                                              XTYPE_STRING, 2,
                                              XTYPE_OBJECT,
                                              XTYPE_POINTER);
}

static void
test_a_interface_init (TestIClass *iface)
{
  g_signal_override_class_closure (foo_signal_id,
				   TEST_TYPE_A,
				   g_cclosure_new (G_CALLBACK (test_a_foo),
						   NULL, NULL));
}

static DEFINE_TYPE_FULL (TestA, test_a,
			 test_a_class_init, NULL, NULL,
			 XTYPE_OBJECT,
			 INTERFACE (test_a_interface_init, TEST_TYPE_I))

#define TEST_TYPE_B (test_b_get_type())

typedef struct _TestB TestB;
typedef struct _TestBClass TestBClass;

struct _TestB {
  TestA parent;
};
struct _TestBClass {
  TestAClass parent_class;
};

static void
test_b_foo (TestI *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("TestB::foo");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  g_assert (g_signal_get_invocation_hint (self)->signal_id == foo_signal_id);
  g_signal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static void
test_b_bar (TestA *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("TestB::bar");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  g_assert (g_signal_get_invocation_hint (self)->signal_id == bar_signal_id);
  g_signal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static xchar_t *
test_b_baz (TestA    *self,
            xobject_t  *object,
            xpointer_t  pointer)
{
  xchar_t *retval = NULL;

  record ("TestB::baz");

  g_assert (object == G_OBJECT (self));
  g_assert (GPOINTER_TO_INT (pointer) == 23);

  g_signal_chain_from_overridden_handler (self, object, pointer, &retval);

  if (retval)
    {
      xchar_t *tmp = xstrconcat (retval , ",TestB::baz", NULL);
      g_free (retval);
      retval = tmp;
    }

  return retval;
}

static void
test_b_class_init (TestBClass *class)
{
  g_signal_override_class_closure (foo_signal_id,
				   TEST_TYPE_B,
				   g_cclosure_new (G_CALLBACK (test_b_foo),
						   NULL, NULL));
  g_signal_override_class_closure (bar_signal_id,
				   TEST_TYPE_B,
				   g_cclosure_new (G_CALLBACK (test_b_bar),
						   NULL, NULL));
  g_signal_override_class_handler ("baz",
				   TEST_TYPE_B,
				   G_CALLBACK (test_b_baz));
}

static DEFINE_TYPE (TestB, test_b,
		    test_b_class_init, NULL, NULL,
		    TEST_TYPE_A)

#define TEST_TYPE_C (test_c_get_type())

typedef struct _TestC TestC;
typedef struct _TestCClass TestCClass;

struct _TestC {
  TestB parent;
};
struct _TestCClass {
  TestBClass parent_class;
};

static void
test_c_foo (TestI *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("TestC::foo");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  g_assert (g_signal_get_invocation_hint (self)->signal_id == foo_signal_id);
  g_signal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static void
test_c_bar (TestA *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("TestC::bar");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  g_assert (g_signal_get_invocation_hint (self)->signal_id == bar_signal_id);
  g_signal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static xchar_t *
test_c_baz (TestA    *self,
            xobject_t  *object,
            xpointer_t  pointer)
{
  xchar_t *retval = NULL;

  record ("TestC::baz");

  g_assert (object == G_OBJECT (self));
  g_assert (GPOINTER_TO_INT (pointer) == 23);

  g_signal_chain_from_overridden_handler (self, object, pointer, &retval);

  if (retval)
    {
      xchar_t *tmp = xstrconcat (retval , ",TestC::baz", NULL);
      g_free (retval);
      retval = tmp;
    }

  return retval;
}

static void
test_c_class_init (TestBClass *class)
{
  g_signal_override_class_closure (foo_signal_id,
				   TEST_TYPE_C,
				   g_cclosure_new (G_CALLBACK (test_c_foo),
						   NULL, NULL));
  g_signal_override_class_closure (bar_signal_id,
				   TEST_TYPE_C,
				   g_cclosure_new (G_CALLBACK (test_c_bar),
						   NULL, NULL));
  g_signal_override_class_handler ("baz",
				   TEST_TYPE_C,
				   G_CALLBACK (test_c_baz));
}


static DEFINE_TYPE (TestC, test_c,
		    test_c_class_init, NULL, NULL,
		    TEST_TYPE_B)

static xstring_t *test_string = NULL;
xboolean_t failed = FALSE;

static void
record (const xchar_t *str)
{
  if (test_string->len)
    xstring_append_c (test_string, ',');
  xstring_append (test_string, str);
}

static void
test (xtype_t        type,
      const xchar_t *signal,
      const xchar_t *expected,
      const xchar_t *expected_retval)
{
  xobject_t *self = xobject_new (type, NULL);

  test_string = xstring_new (NULL);

  if (strcmp (signal, "baz"))
    {
      g_signal_emit_by_name (self, signal);
    }
  else
    {
      xchar_t *ret;

      g_signal_emit_by_name (self, signal, self, GINT_TO_POINTER (23), &ret);

      if (strcmp (ret, expected_retval) != 0)
        failed = TRUE;

      g_free (ret);
    }

#ifndef VERBOSE
  if (strcmp (test_string->str, expected) != 0)
#endif
    {
      g_printerr ("*** emitting %s on a %s instance\n"
		  "    Expecting: %s\n"
		  "    Got: %s\n",
		  signal, xtype_name (type),
		  expected,
		  test_string->str);

      if (strcmp (test_string->str, expected) != 0)
	failed = TRUE;
    }

  xstring_free (test_string, TRUE);
  xobject_unref (self);
}

int
main (int argc, char **argv)
{
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);

  test (TEST_TYPE_A, "foo", "TestA::foo,TestI::foo", NULL);
  test (TEST_TYPE_A, "bar", "TestA::bar", NULL);
  test (TEST_TYPE_A, "baz", "TestA::baz", "TestA::baz");

  test (TEST_TYPE_B, "foo", "TestB::foo,TestA::foo,TestI::foo", NULL);
  test (TEST_TYPE_B, "bar", "TestB::bar,TestA::bar", NULL);
  test (TEST_TYPE_B, "baz", "TestB::baz,TestA::baz", "TestA::baz,TestB::baz");

  test (TEST_TYPE_C, "foo", "TestC::foo,TestB::foo,TestA::foo,TestI::foo", NULL);
  test (TEST_TYPE_C, "bar", "TestC::bar,TestB::bar,TestA::bar", NULL);
  test (TEST_TYPE_C, "baz", "TestC::baz,TestB::baz,TestA::baz", "TestA::baz,TestB::baz,TestC::baz");

  return failed ? 1 : 0;
}
