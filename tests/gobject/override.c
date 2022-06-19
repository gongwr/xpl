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

typedef struct _TestI test_i_t;
typedef struct _TestIClass TestIClass;

struct _TestIClass
{
  xtype_interface_t base_iface;
};

static void
test_i_foo (test_i_t *self)
{
  record ("test_i_t::foo");
}

static void
test_i_default_init (xpointer_t g_class)
{
  foo_signal_id = xsignal_newv ("foo",
				 TEST_TYPE_I,
				 G_SIGNAL_RUN_LAST,
				 g_cclosure_new(G_CALLBACK(test_i_foo),
						NULL, NULL),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__VOID,
				 XTYPE_NONE, 0, NULL);
}

static DEFINE_IFACE (test_i, test_i, NULL, test_i_default_init)

#define TEST_TYPE_A (test_a_get_type())

     typedef struct _TestA test_a_t;
     typedef struct _TestAClass TestAClass;

struct _TestA {
  xobject_t parent;
};
struct _TestAClass {
  xobject_class_t parent_class;

  void (* bar) (test_a_t *self);
};

static void
test_a_foo (test_i_t *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("test_a_t::foo");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  xassert (xsignal_get_invocation_hint (self)->signal_id == foo_signal_id);
  xsignal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static void
test_a_bar (test_a_t *self)
{
  record ("test_a_t::bar");
}

static xchar_t *
test_a_baz (test_a_t    *self,
            xobject_t  *object,
            xpointer_t  pointer)
{
  record ("test_a_t::baz");

  xassert (object == G_OBJECT (self));
  xassert (GPOINTER_TO_INT (pointer) == 23);

  return xstrdup ("test_a_t::baz");
}

static void
test_a_class_init (TestAClass *class)
{
  class->bar = test_a_bar;

  bar_signal_id = xsignal_new ("bar",
				TEST_TYPE_A,
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TestAClass, bar),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				XTYPE_NONE, 0, NULL);

  baz_signal_id = xsignal_new_class_handler ("baz",
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
  xsignal_override_class_closure (foo_signal_id,
				   TEST_TYPE_A,
				   g_cclosure_new (G_CALLBACK (test_a_foo),
						   NULL, NULL));
}

static DEFINE_TYPE_FULL (test_a, test_a,
			 test_a_class_init, NULL, NULL,
			 XTYPE_OBJECT,
			 INTERFACE (test_a_interface_init, TEST_TYPE_I))

#define TEST_TYPE_B (test_b_get_type())

typedef struct _TestB test_b_t;
typedef struct _TestBClass TestBClass;

struct _TestB {
  test_a_t parent;
};
struct _TestBClass {
  TestAClass parent_class;
};

static void
test_b_foo (test_i_t *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("test_b_t::foo");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  xassert (xsignal_get_invocation_hint (self)->signal_id == foo_signal_id);
  xsignal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static void
test_b_bar (test_a_t *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("test_b_t::bar");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  xassert (xsignal_get_invocation_hint (self)->signal_id == bar_signal_id);
  xsignal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static xchar_t *
test_b_baz (test_a_t    *self,
            xobject_t  *object,
            xpointer_t  pointer)
{
  xchar_t *retval = NULL;

  record ("test_b_t::baz");

  xassert (object == G_OBJECT (self));
  xassert (GPOINTER_TO_INT (pointer) == 23);

  xsignal_chain_from_overridden_handler (self, object, pointer, &retval);

  if (retval)
    {
      xchar_t *tmp = xstrconcat (retval , ",test_b_t::baz", NULL);
      g_free (retval);
      retval = tmp;
    }

  return retval;
}

static void
test_b_class_init (TestBClass *class)
{
  xsignal_override_class_closure (foo_signal_id,
				   TEST_TYPE_B,
				   g_cclosure_new (G_CALLBACK (test_b_foo),
						   NULL, NULL));
  xsignal_override_class_closure (bar_signal_id,
				   TEST_TYPE_B,
				   g_cclosure_new (G_CALLBACK (test_b_bar),
						   NULL, NULL));
  xsignal_override_class_handler ("baz",
				   TEST_TYPE_B,
				   G_CALLBACK (test_b_baz));
}

static DEFINE_TYPE (test_b, test_b,
		    test_b_class_init, NULL, NULL,
		    TEST_TYPE_A)

#define TEST_TYPE_C (test_c_get_type())

typedef struct _TestC test_c_t;
typedef struct _TestCClass TestCClass;

struct _TestC {
  test_b_t parent;
};
struct _TestCClass {
  TestBClass parent_class;
};

static void
test_c_foo (test_i_t *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("test_c_t::foo");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  xassert (xsignal_get_invocation_hint (self)->signal_id == foo_signal_id);
  xsignal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static void
test_c_bar (test_a_t *self)
{
  xvalue_t args[1] = { G_VALUE_INIT };

  record ("test_c_t::bar");

  xvalue_init (&args[0], TEST_TYPE_A);
  xvalue_set_object (&args[0], self);

  xassert (xsignal_get_invocation_hint (self)->signal_id == bar_signal_id);
  xsignal_chain_from_overridden (args, NULL);

  xvalue_unset (&args[0]);
}

static xchar_t *
test_c_baz (test_a_t    *self,
            xobject_t  *object,
            xpointer_t  pointer)
{
  xchar_t *retval = NULL;

  record ("test_c_t::baz");

  xassert (object == G_OBJECT (self));
  xassert (GPOINTER_TO_INT (pointer) == 23);

  xsignal_chain_from_overridden_handler (self, object, pointer, &retval);

  if (retval)
    {
      xchar_t *tmp = xstrconcat (retval , ",test_c_t::baz", NULL);
      g_free (retval);
      retval = tmp;
    }

  return retval;
}

static void
test_c_class_init (TestBClass *class)
{
  xsignal_override_class_closure (foo_signal_id,
				   TEST_TYPE_C,
				   g_cclosure_new (G_CALLBACK (test_c_foo),
						   NULL, NULL));
  xsignal_override_class_closure (bar_signal_id,
				   TEST_TYPE_C,
				   g_cclosure_new (G_CALLBACK (test_c_bar),
						   NULL, NULL));
  xsignal_override_class_handler ("baz",
				   TEST_TYPE_C,
				   G_CALLBACK (test_c_baz));
}


static DEFINE_TYPE (test_c, test_c,
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
      xsignal_emit_by_name (self, signal);
    }
  else
    {
      xchar_t *ret;

      xsignal_emit_by_name (self, signal, self, GINT_TO_POINTER (23), &ret);

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

  test (TEST_TYPE_A, "foo", "test_a_t::foo,test_i_t::foo", NULL);
  test (TEST_TYPE_A, "bar", "test_a_t::bar", NULL);
  test (TEST_TYPE_A, "baz", "test_a_t::baz", "test_a_t::baz");

  test (TEST_TYPE_B, "foo", "test_b_t::foo,test_a_t::foo,test_i_t::foo", NULL);
  test (TEST_TYPE_B, "bar", "test_b_t::bar,test_a_t::bar", NULL);
  test (TEST_TYPE_B, "baz", "test_b_t::baz,test_a_t::baz", "test_a_t::baz,test_b_t::baz");

  test (TEST_TYPE_C, "foo", "test_c_t::foo,test_b_t::foo,test_a_t::foo,test_i_t::foo", NULL);
  test (TEST_TYPE_C, "bar", "test_c_t::bar,test_b_t::bar,test_a_t::bar", NULL);
  test (TEST_TYPE_C, "baz", "test_c_t::baz,test_b_t::baz,test_a_t::baz", "test_a_t::baz,test_b_t::baz,test_c_t::baz");

  return failed ? 1 : 0;
}
