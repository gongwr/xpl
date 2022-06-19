/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2001 Red Hat, Inc.
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

#include <string.h>

#undef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN "test_object_t"
#include	<glib-object.h>

/* --- test_iface_t --- */
#define TEST_TYPE_IFACE           (test_iface_get_type ())
#define TEST_IFACE(obj)		  (XTYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_IFACE, test_iface_t))
#define TEST_IS_IFACE(obj)	  (XTYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_IFACE))
#define TEST_IFACE_GET_CLASS(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE, test_iface_class_t))
typedef struct _test_iface      test_iface_t;
typedef struct _test_iface_class test_iface_class_t;
struct _test_iface_class
{
  xtype_interface_t base_iface;
  void	(*print_string)	(test_iface_t	*tiobj,
			 const xchar_t	*string);
};
static void	iface_base_init		(test_iface_class_t	*iface);
static void	iface_base_finalize	(test_iface_class_t	*iface);
static void	print_foo		(test_iface_t	*tiobj,
					 const xchar_t	*string);
static xtype_t
test_iface_get_type (void)
{
  static xtype_t test_iface_type = 0;

  if (!test_iface_type)
    {
      const xtype_info_t test_iface_info =
      {
	sizeof (test_iface_class_t),
	(xbase_init_func_t)	iface_base_init,		/* base_init */
	(xbase_finalize_func_t) iface_base_finalize,	/* base_finalize */
        NULL,
        NULL,
        NULL,
        0,
        0,
        NULL,
        NULL
      };

      test_iface_type = xtype_register_static (XTYPE_INTERFACE, "test_iface_t", &test_iface_info, 0);
      xtype_interface_add_prerequisite (test_iface_type, XTYPE_OBJECT);
    }

  return test_iface_type;
}
static xuint_t iface_base_init_count = 0;
static void
iface_base_init (test_iface_class_t *iface)
{
  iface_base_init_count++;
  if (iface_base_init_count == 1)
    {
      /* add signals here */
    }
}
static void
iface_base_finalize (test_iface_class_t *iface)
{
  iface_base_init_count--;
  if (iface_base_init_count == 0)
    {
      /* destroy signals here */
    }
}
static void
print_foo (test_iface_t   *tiobj,
	   const xchar_t *string)
{
  if (!string)
    string = "<NULL>";
  g_print ("Iface-FOO: \"%s\" from %p\n", string, tiobj);
}
static void
test_object_test_iface_init (xpointer_t giface,
			     xpointer_t iface_data)
{
  test_iface_class_t *iface = giface;

  xassert (iface_data == GUINT_TO_POINTER (42));

  xassert (XTYPE_FROM_INTERFACE (iface) == TEST_TYPE_IFACE);

  /* assert iface_base_init() was already called */
  xassert (iface_base_init_count > 0);

  /* initialize stuff */
  iface->print_string = print_foo;
}
static void
iface_print_string (test_iface_t   *tiobj,
		    const xchar_t *string)
{
  test_iface_class_t *iface;

  g_return_if_fail (TEST_IS_IFACE (tiobj));
  g_return_if_fail (X_IS_OBJECT (tiobj)); /* ensured through prerequisite */

  iface = TEST_IFACE_GET_CLASS (tiobj);
  xobject_ref (tiobj);
  iface->print_string (tiobj, string);
  xobject_unref (tiobj);
}


/* --- test_object_t --- */
#define TEST_TYPE_OBJECT            (test_object_get_type ())
#define TEST_OBJECT(object)         (XTYPE_CHECK_INSTANCE_CAST ((object), TEST_TYPE_OBJECT, test_object_t))
#define TEST_OBJECT_CLASS(klass)    (XTYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_OBJECT, test_object_class_t))
#define TEST_IS_OBJECT(object)      (XTYPE_CHECK_INSTANCE_TYPE ((object), TEST_TYPE_OBJECT))
#define TEST_IS_OBJECT_CLASS(klass) (XTYPE_CHECK_CLASS_TYPE ((klass), TEST_TYPE_OBJECT))
#define TEST_OBJECT_GET_CLASS(obj)  (XTYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_OBJECT, test_object_class_t))
#define TEST_OBJECT_GET_PRIVATE(o)  (XTYPE_INSTANCE_GET_PRIVATE ((o), TEST_TYPE_OBJECT, test_object_private_t))
typedef struct _test_object        test_object_t;
typedef struct _test_object_class   test_object_class_t;
typedef struct _test_object_private test_object_private_t;
struct _test_object
{
  xobject_t parent_instance;
};
struct _test_object_class
{
  xobject_class_t parent_class;

  xchar_t* (*test_signal) (test_object_t *tobject,
			 test_iface_t  *iface_object,
			 xpointer_t    tdata);
};
struct _test_object_private
{
  int     dummy1;
  xdouble_t dummy2;
};
static void	test_object_class_init	(test_object_class_t	*class);
static void	test_object_init	(test_object_t		*tobject);
static xboolean_t	test_signal_accumulator	(xsignal_invocation_hint_t	*ihint,
					 xvalue_t            	*return_accu,
					 const xvalue_t       	*handler_return,
					 xpointer_t                data);
static xchar_t*	test_object_test_signal	(test_object_t		*tobject,
					 test_iface_t		*iface_object,
					 xpointer_t		 tdata);
static xint_t test_object_private_offset;
static inline xpointer_t
test_object_get_instance_private (test_object_t *self)
{
  return (G_STRUCT_MEMBER_P (self, test_object_private_offset));
}

static xtype_t
test_object_get_type (void)
{
  static xtype_t test_object_type = 0;

  if (!test_object_type)
    {
      const xtype_info_t test_object_info =
      {
	sizeof (test_object_class_t),
	NULL,           /* base_init */
	NULL,           /* base_finalize */
	(xclass_init_func_t) test_object_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (test_object_t),
	5,              /* n_preallocs */
        (xinstance_init_func_t) test_object_init,
        NULL
      };
      xinterface_info_t iface_info = { test_object_test_iface_init, NULL, GUINT_TO_POINTER (42) };

      test_object_type = xtype_register_static (XTYPE_OBJECT, "test_object_t", &test_object_info, 0);
      xtype_add_interface_static (test_object_type, TEST_TYPE_IFACE, &iface_info);

      test_object_private_offset =
        xtype_add_instance_private (test_object_type, sizeof (test_object_private_t));
    }

  return test_object_type;
}
static void
test_object_class_init (test_object_class_t *class)
{
  /*  xobject_class_t *xobject_class = XOBJECT_CLASS (class); */
  xtype_class_adjust_private_offset (class, &test_object_private_offset);

  class->test_signal = test_object_test_signal;

  xsignal_new ("test-signal",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP,
		G_STRUCT_OFFSET (test_object_class_t, test_signal),
		test_signal_accumulator, NULL,
		g_cclosure_marshal_STRING__OBJECT_POINTER,
		XTYPE_STRING, 2, TEST_TYPE_IFACE, XTYPE_POINTER);
}
static void
test_object_init (test_object_t *tobject)
{
  test_object_private_t *priv = test_object_get_instance_private (tobject);

  xassert (priv);

  priv->dummy1 = 54321;
}
/* Check to see if private data initialization in the
 * instance init function works.
 */
static void
test_object_check_private_init (test_object_t *tobject)
{
  test_object_private_t *priv = test_object_get_instance_private (tobject);

  g_print ("private data during initialization: %u == %u\n", priv->dummy1, 54321);
  xassert (priv->dummy1 == 54321);
}
static xboolean_t
test_signal_accumulator (xsignal_invocation_hint_t *ihint,
			 xvalue_t                *return_accu,
			 const xvalue_t          *handler_return,
			 xpointer_t               data)
{
  const xchar_t *accu_string = xvalue_get_string (return_accu);
  const xchar_t *new_string = xvalue_get_string (handler_return);
  xchar_t *result_string;

  if (accu_string)
    result_string = xstrconcat (accu_string, new_string, NULL);
  else if (new_string)
    result_string = xstrdup (new_string);
  else
    result_string = NULL;

  xvalue_take_string (return_accu, result_string);

  return TRUE;
}
static xchar_t*
test_object_test_signal (test_object_t *tobject,
			 test_iface_t  *iface_object,
			 xpointer_t    tdata)
{
  g_message ("::test_signal default_handler called");

  xreturn_val_if_fail (TEST_IS_IFACE (iface_object), NULL);

  return xstrdup ("<default_handler>");
}


/* --- test_iface_t for derived_object_t --- */
static void
print_bar (test_iface_t   *tiobj,
	   const xchar_t *string)
{
  test_iface_class_t *parent_iface;

  g_return_if_fail (TEST_IS_IFACE (tiobj));

  if (!string)
    string = "<NULL>";
  g_print ("Iface-BAR: \"%s\" from %p\n", string, tiobj);

  g_print ("chaining: ");
  parent_iface = xtype_interface_peek_parent (TEST_IFACE_GET_CLASS (tiobj));
  parent_iface->print_string (tiobj, string);

  xassert (xtype_interface_peek_parent (parent_iface) == NULL);
}

static void
derived_object_test_iface_init (xpointer_t giface,
				xpointer_t iface_data)
{
  test_iface_class_t *iface = giface;

  xassert (iface_data == GUINT_TO_POINTER (87));

  xassert (XTYPE_FROM_INTERFACE (iface) == TEST_TYPE_IFACE);

  /* assert test_object_test_iface_init() was already called */
  xassert (iface->print_string == print_foo);

  /* override stuff */
  iface->print_string = print_bar;
}


/* --- derived_object_t --- */
#define DERIVED_TYPE_OBJECT            (derived_object_get_type ())
#define DERIVED_OBJECT(object)         (XTYPE_CHECK_INSTANCE_CAST ((object), DERIVED_TYPE_OBJECT, derived_object_t))
#define DERIVED_OBJECT_CLASS(klass)    (XTYPE_CHECK_CLASS_CAST ((klass), DERIVED_TYPE_OBJECT, derived_object_class_t))
#define DERIVED_IS_OBJECT(object)      (XTYPE_CHECK_INSTANCE_TYPE ((object), DERIVED_TYPE_OBJECT))
#define DERIVED_IS_OBJECT_CLASS(klass) (XTYPE_CHECK_CLASS_TYPE ((klass), DERIVED_TYPE_OBJECT))
#define DERIVED_OBJECT_GET_CLASS(obj)  (XTYPE_INSTANCE_GET_CLASS ((obj), DERIVED_TYPE_OBJECT, derived_object_class_t))
#define DERIVED_OBJECT_GET_PRIVATE(o)  (XTYPE_INSTANCE_GET_PRIVATE ((o), DERIVED_TYPE_OBJECT, derived_object_private_t))
typedef struct _derived_object        derived_object_t;
typedef struct _test_object_class      derived_object_class_t;
typedef struct _derived_object_private derived_object_private_t;
struct _derived_object
{
  test_object_t parent_instance;
  int  dummy1;
  int  dummy2;
};
struct _derived_object_private
{
  char dummy;
};
static void derived_object_class_init (derived_object_class_t *class);
static void derived_object_init       (derived_object_t      *dobject);
static xint_t DerivedObject_private_offset;
static inline xpointer_t
derived_object_get_instance_private (derived_object_t *self)
{
  return (G_STRUCT_MEMBER_P (self, DerivedObject_private_offset));
}
static xtype_t
derived_object_get_type (void)
{
  static xtype_t derived_object_type = 0;

  if (!derived_object_type)
    {
      const xtype_info_t derived_object_info =
      {
	sizeof (derived_object_class_t),
	NULL,           /* base_init */
	NULL,           /* base_finalize */
	(xclass_init_func_t) derived_object_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (derived_object_t),
	5,              /* n_preallocs */
        (xinstance_init_func_t) derived_object_init,
        NULL
      };
      xinterface_info_t iface_info = { derived_object_test_iface_init, NULL, GUINT_TO_POINTER (87) };

      derived_object_type = xtype_register_static (TEST_TYPE_OBJECT, "derived_object_t", &derived_object_info, 0);
      xtype_add_interface_static (derived_object_type, TEST_TYPE_IFACE, &iface_info);
      DerivedObject_private_offset =
        xtype_add_instance_private (derived_object_type, sizeof (derived_object_private_t));
    }

  return derived_object_type;
}
static void
derived_object_class_init (derived_object_class_t *class)
{
  xtype_class_adjust_private_offset (class, &DerivedObject_private_offset);
}
static void
derived_object_init (derived_object_t *dobject)
{
  test_object_private_t *test_priv;
  derived_object_private_t *derived_priv;

  derived_priv = derived_object_get_instance_private (dobject);

  xassert (derived_priv);

  test_priv = test_object_get_instance_private (TEST_OBJECT (dobject));

  xassert (test_priv);
}

/* --- main --- */
int
main (int   argc,
      char *argv[])
{
  xtype_info_t info = { 0, };
  GTypeFundamentalInfo finfo = { 0, };
  xtype_t type;
  test_object_t *sigarg;
  derived_object_t *dobject;
  test_object_private_t *priv;
  xchar_t *string = NULL;

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);

  /* test new fundamentals */
  xassert (XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST) == xtype_fundamental_next ());
  type = xtype_register_fundamental (xtype_fundamental_next (), "FooShadow1", &info, &finfo, 0);
  xassert (type == XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST));
  xassert (XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST + 1) == xtype_fundamental_next ());
  type = xtype_register_fundamental (xtype_fundamental_next (), "FooShadow2", &info, &finfo, 0);
  xassert (type == XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST + 1));
  xassert (XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST + 2) == xtype_fundamental_next ());
  xassert (xtype_from_name ("FooShadow1") == XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST));
  xassert (xtype_from_name ("FooShadow2") == XTYPE_MAKE_FUNDAMENTAL (XTYPE_RESERVED_USER_FIRST + 1));

  /* to test past class initialization interface setups, create the class here */
  xtype_class_ref (TEST_TYPE_OBJECT);

  dobject = xobject_new (DERIVED_TYPE_OBJECT, NULL);
  test_object_check_private_init (TEST_OBJECT (dobject));

  sigarg = xobject_new (TEST_TYPE_OBJECT, NULL);

  g_print ("MAIN: emit test-signal:\n");
  xsignal_emit_by_name (dobject, "test-signal", sigarg, NULL, &string);
  g_message ("signal return: \"%s\"", string);
  g_assert_cmpstr (string, ==, "<default_handler><default_handler><default_handler>");
  g_free (string);

  g_print ("MAIN: call iface print-string on test and derived object:\n");
  iface_print_string (TEST_IFACE (sigarg), "iface-string-from-test-type");
  iface_print_string (TEST_IFACE (dobject), "iface-string-from-derived-type");

  priv = test_object_get_instance_private (TEST_OBJECT (dobject));
  g_print ("private data after initialization: %u == %u\n", priv->dummy1, 54321);
  xassert (priv->dummy1 == 54321);

  xobject_unref (sigarg);
  xobject_unref (dobject);

  g_message ("%s done", argv[0]);

  return 0;
}
