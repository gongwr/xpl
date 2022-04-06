/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2001, 2003 Red Hat, Inc.
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
#define	G_LOG_DOMAIN "TestAccumulator"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#include <string.h>

#include	<glib-object.h>

#include "testmarshal.h"
#include "testcommon.h"

/* What this test tests is the behavior of signal accumulators
 * Two accumulators are tested:
 *
 * 1: A custom accumulator that appends the returned strings
 * 2: The standard g_signal_accumulator_true_handled that stops
 *    emission on TRUE returns.
 */

/*
 * test_object_t, a parent class for test_object_t
 */
#define TEST_TYPE_OBJECT          (test_object_get_type ())
typedef struct _test_object        test_object_t;
typedef struct _test_object_class   test_object_class_t;

struct _test_object
{
  xobject_t parent_instance;
};
struct _test_object_class
{
  xobject_class_t parent_class;

  xchar_t*   (*test_signal1) (test_object_t *tobject,
			    xint_t        param);
  xboolean_t (*test_signal2) (test_object_t *tobject,
			    xint_t        param);
  xvariant_t* (*test_signal3) (test_object_t *tobject,
                             xboolean_t *weak_ptr);
};

static xtype_t test_object_get_type (void);

static xboolean_t
test_signal1_accumulator (xsignal_invocation_hint_t *ihint,
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

  xvalue_set_string_take_ownership (return_accu, result_string);

  return TRUE;
}

static xchar_t *
test_object_signal1_callback_before (test_object_t *tobject,
				     xint_t        param,
				     xpointer_t    data)
{
  return xstrdup ("<before>");
}

static xchar_t *
test_object_real_signal1 (test_object_t *tobject,
			  xint_t        param)
{
  return xstrdup ("<default>");
}

static xchar_t *
test_object_signal1_callback_after (test_object_t *tobject,
				    xint_t        param,
				    xpointer_t    data)
{
  return xstrdup ("<after>");
}

static xboolean_t
test_object_signal2_callback_before (test_object_t *tobject,
				     xint_t        param)
{
  switch (param)
    {
    case 1: return TRUE;
    case 2: return FALSE;
    case 3: return FALSE;
    case 4: return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

static xboolean_t
test_object_real_signal2 (test_object_t *tobject,
			  xint_t        param)
{
  switch (param)
    {
    case 1: g_assert_not_reached (); return FALSE;
    case 2: return TRUE;
    case 3: return FALSE;
    case 4: return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

static xboolean_t
test_object_signal2_callback_after (test_object_t *tobject,
				     xint_t        param)
{
  switch (param)
    {
    case 1: g_assert_not_reached (); return FALSE;
    case 2: g_assert_not_reached (); return FALSE;
    case 3: return TRUE;
    case 4: return FALSE;
    }

  g_assert_not_reached ();
  return FALSE;
}

static xboolean_t
test_signal3_accumulator (xsignal_invocation_hint_t *ihint,
			  xvalue_t                *return_accu,
			  const xvalue_t          *handler_return,
			  xpointer_t               data)
{
  xvariant_t *variant;

  variant = xvalue_get_variant (handler_return);
  g_assert (!xvariant_is_floating (variant));

  xvalue_set_variant (return_accu, variant);

  return variant == NULL;
}

/* To be notified when the variant is finalised, we construct
 * it from data with a custom xdestroy_notify_t.
 */

typedef struct {
  char *mem;
  xsize_t n;
  xboolean_t *weak_ptr;
} VariantData;

static void
free_data (VariantData *data)
{
  *(data->weak_ptr) = TRUE;
  g_free (data->mem);
  g_slice_free (VariantData, data);
}

static xvariant_t *
test_object_real_signal3 (test_object_t *tobject,
                          xboolean_t *weak_ptr)
{
  xvariant_t *variant;
  VariantData *data;

  variant = xvariant_ref_sink (xvariant_new_uint32 (42));
  data = g_slice_new (VariantData);
  data->weak_ptr = weak_ptr;
  data->n = xvariant_get_size (variant);
  data->mem = g_malloc (data->n);
  xvariant_store (variant, data->mem);
  xvariant_unref (variant);

  variant = xvariant_new_from_data (G_VARIANT_TYPE ("u"),
                                     data->mem,
                                     data->n,
                                     TRUE,
                                     (xdestroy_notify_t) free_data,
                                     data);
  return xvariant_ref_sink (variant);
}

static void
test_object_class_init (test_object_class_t *class)
{
  class->test_signal1 = test_object_real_signal1;
  class->test_signal2 = test_object_real_signal2;
  class->test_signal3 = test_object_real_signal3;

  g_signal_new ("test-signal1",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (test_object_class_t, test_signal1),
		test_signal1_accumulator, NULL,
		test_marshal_STRING__INT,
		XTYPE_STRING, 1, XTYPE_INT);
  g_signal_new ("test-signal2",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (test_object_class_t, test_signal2),
		g_signal_accumulator_true_handled, NULL,
		test_marshal_BOOLEAN__INT,
		XTYPE_BOOLEAN, 1, XTYPE_INT);
  g_signal_new ("test-signal3",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (test_object_class_t, test_signal3),
		test_signal3_accumulator, NULL,
		test_marshal_VARIANT__POINTER,
		XTYPE_VARIANT, 1, XTYPE_POINTER);
}

static DEFINE_TYPE(test_object, test_object,
		   test_object_class_init, NULL, NULL,
		   XTYPE_OBJECT)

int
main (int   argc,
      char *argv[])
{
  test_object_t *object;
  xchar_t *string_result;
  xboolean_t bool_result;
  xboolean_t variant_finalised;
  xvariant_t *variant_result;

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);

  object = xobject_new (TEST_TYPE_OBJECT, NULL);

  g_signal_connect (object, "test-signal1",
		    G_CALLBACK (test_object_signal1_callback_before), NULL);
  g_signal_connect_after (object, "test-signal1",
			  G_CALLBACK (test_object_signal1_callback_after), NULL);

  g_signal_emit_by_name (object, "test-signal1", 0, &string_result);
  g_assert (strcmp (string_result, "<before><default><after>") == 0);
  g_free (string_result);

  g_signal_connect (object, "test-signal2",
		    G_CALLBACK (test_object_signal2_callback_before), NULL);
  g_signal_connect_after (object, "test-signal2",
			  G_CALLBACK (test_object_signal2_callback_after), NULL);

  bool_result = FALSE;
  g_signal_emit_by_name (object, "test-signal2", 1, &bool_result);
  g_assert (bool_result == TRUE);
  bool_result = FALSE;
  g_signal_emit_by_name (object, "test-signal2", 2, &bool_result);
  g_assert (bool_result == TRUE);
  bool_result = FALSE;
  g_signal_emit_by_name (object, "test-signal2", 3, &bool_result);
  g_assert (bool_result == TRUE);
  bool_result = TRUE;
  g_signal_emit_by_name (object, "test-signal2", 4, &bool_result);
  g_assert (bool_result == FALSE);

  variant_finalised = FALSE;
  variant_result = NULL;
  g_signal_emit_by_name (object, "test-signal3", &variant_finalised, &variant_result);
  g_assert (variant_result != NULL);
  g_assert (!xvariant_is_floating (variant_result));

  /* Test that variant_result had refcount 1 */
  g_assert (!variant_finalised);
  xvariant_unref (variant_result);
  g_assert (variant_finalised);

  xobject_unref (object);

  return 0;
}
