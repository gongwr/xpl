/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2005 Red Hat, Inc.
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
#define	G_LOG_DOMAIN "TestReferences"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#include	<glib-object.h>

/* This test tests weak and toggle references
 */

static xobject_t *global_object;

static xboolean_t object_destroyed;
static xboolean_t weak_ref1_notified;
static xboolean_t weak_ref2_notified;
static xboolean_t toggle_ref1_weakened;
static xboolean_t toggle_ref1_strengthened;
static xboolean_t toggle_ref2_weakened;
static xboolean_t toggle_ref2_strengthened;
static xboolean_t toggle_ref3_weakened;
static xboolean_t toggle_ref3_strengthened;

/*
 * test_object_t, a parent class for test_object_t
 */
static xtype_t test_object_get_type (void);
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
};

XDEFINE_TYPE (test_object, test_object, XTYPE_OBJECT)

static void
test_object_finalize (xobject_t *object)
{
  object_destroyed = TRUE;

  XOBJECT_CLASS (test_object_parent_class)->finalize (object);
}

static void
test_object_class_init (test_object_class_t *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->finalize = test_object_finalize;
}

static void
test_object_init (test_object_t *test_object)
{
}

static void
clear_flags (void)
{
  object_destroyed = FALSE;
  weak_ref1_notified = FALSE;
  weak_ref2_notified = FALSE;
  toggle_ref1_weakened = FALSE;
  toggle_ref1_strengthened = FALSE;
  toggle_ref2_weakened = FALSE;
  toggle_ref2_strengthened = FALSE;
  toggle_ref3_weakened = FALSE;
  toggle_ref3_strengthened = FALSE;
}

static void
weak_ref1 (xpointer_t data,
	   xobject_t *object)
{
  xassert (object == global_object);
  xassert (data == GUINT_TO_POINTER (42));

  weak_ref1_notified = TRUE;
}

static void
weak_ref2 (xpointer_t data,
	   xobject_t *object)
{
  xassert (object == global_object);
  xassert (data == GUINT_TO_POINTER (24));

  weak_ref2_notified = TRUE;
}

static void
toggle_ref1 (xpointer_t data,
	     xobject_t *object,
	     xboolean_t is_last_ref)
{
  xassert (object == global_object);
  xassert (data == GUINT_TO_POINTER (42));

  if (is_last_ref)
    toggle_ref1_weakened = TRUE;
  else
    toggle_ref1_strengthened = TRUE;
}

static void
toggle_ref2 (xpointer_t data,
	     xobject_t *object,
	     xboolean_t is_last_ref)
{
  xassert (object == global_object);
  xassert (data == GUINT_TO_POINTER (24));

  if (is_last_ref)
    toggle_ref2_weakened = TRUE;
  else
    toggle_ref2_strengthened = TRUE;
}

static void
toggle_ref3 (xpointer_t data,
	     xobject_t *object,
	     xboolean_t is_last_ref)
{
  xassert (object == global_object);
  xassert (data == GUINT_TO_POINTER (34));

  if (is_last_ref)
    {
      toggle_ref3_weakened = TRUE;
      xobject_remove_toggle_ref (object, toggle_ref3, GUINT_TO_POINTER (34));
    }
  else
    toggle_ref3_strengthened = TRUE;
}

int
main (int   argc,
      char *argv[])
{
  xobject_t *object;

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);

  /* test_t basic weak reference operation
   */
  global_object = object = xobject_new (TEST_TYPE_OBJECT, NULL);

  xobject_weak_ref (object, weak_ref1, GUINT_TO_POINTER (42));

  clear_flags ();
  xobject_unref (object);
  xassert (weak_ref1_notified == TRUE);
  xassert (object_destroyed == TRUE);

  /* test_t two weak references at once
   */
  global_object = object = xobject_new (TEST_TYPE_OBJECT, NULL);

  xobject_weak_ref (object, weak_ref1, GUINT_TO_POINTER (42));
  xobject_weak_ref (object, weak_ref2, GUINT_TO_POINTER (24));

  clear_flags ();
  xobject_unref (object);
  xassert (weak_ref1_notified == TRUE);
  xassert (weak_ref2_notified == TRUE);
  xassert (object_destroyed == TRUE);

  /* test_t remove weak references
   */
  global_object = object = xobject_new (TEST_TYPE_OBJECT, NULL);

  xobject_weak_ref (object, weak_ref1, GUINT_TO_POINTER (42));
  xobject_weak_ref (object, weak_ref2, GUINT_TO_POINTER (24));
  xobject_weak_unref (object, weak_ref1, GUINT_TO_POINTER (42));

  clear_flags ();
  xobject_unref (object);
  xassert (weak_ref1_notified == FALSE);
  xassert (weak_ref2_notified == TRUE);
  xassert (object_destroyed == TRUE);

  /* test_t basic toggle reference operation
   */
  global_object = object = xobject_new (TEST_TYPE_OBJECT, NULL);

  xobject_add_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));

  clear_flags ();
  xobject_unref (object);
  xassert (toggle_ref1_weakened == TRUE);
  xassert (toggle_ref1_strengthened == FALSE);
  xassert (object_destroyed == FALSE);

  clear_flags ();
  xobject_ref (object);
  xassert (toggle_ref1_weakened == FALSE);
  xassert (toggle_ref1_strengthened == TRUE);
  xassert (object_destroyed == FALSE);

  xobject_unref (object);

  clear_flags ();
  xobject_remove_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));
  xassert (toggle_ref1_weakened == FALSE);
  xassert (toggle_ref1_strengthened == FALSE);
  xassert (object_destroyed == TRUE);

  global_object = object = xobject_new (TEST_TYPE_OBJECT, NULL);

  /* test_t two toggle references at once
   */
  xobject_add_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));
  xobject_add_toggle_ref (object, toggle_ref2, GUINT_TO_POINTER (24));

  clear_flags ();
  xobject_unref (object);
  xassert (toggle_ref1_weakened == FALSE);
  xassert (toggle_ref1_strengthened == FALSE);
  xassert (toggle_ref2_weakened == FALSE);
  xassert (toggle_ref2_strengthened == FALSE);
  xassert (object_destroyed == FALSE);

  clear_flags ();
  xobject_remove_toggle_ref (object, toggle_ref1, GUINT_TO_POINTER (42));
  xassert (toggle_ref1_weakened == FALSE);
  xassert (toggle_ref1_strengthened == FALSE);
  xassert (toggle_ref2_weakened == TRUE);
  xassert (toggle_ref2_strengthened == FALSE);
  xassert (object_destroyed == FALSE);

  clear_flags ();
  /* Check that removing a toggle ref with %NULL data works fine. */
  xobject_remove_toggle_ref (object, toggle_ref2, NULL);
  xassert (toggle_ref1_weakened == FALSE);
  xassert (toggle_ref1_strengthened == FALSE);
  xassert (toggle_ref2_weakened == FALSE);
  xassert (toggle_ref2_strengthened == FALSE);
  xassert (object_destroyed == TRUE);

  /* test_t a toggle reference that removes itself
   */
  global_object = object = xobject_new (TEST_TYPE_OBJECT, NULL);

  xobject_add_toggle_ref (object, toggle_ref3, GUINT_TO_POINTER (34));

  clear_flags ();
  xobject_unref (object);
  xassert (toggle_ref3_weakened == TRUE);
  xassert (toggle_ref3_strengthened == FALSE);
  xassert (object_destroyed == TRUE);

  return 0;
}
