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
#define	G_LOG_DOMAIN "TestDynamicType"

#undef G_DISABLE_ASSERT
#undef G_DISABLE_CHECKS
#undef G_DISABLE_CAST_CHECKS

#include <glib-object.h>

#include "testcommon.h"
#include "testmodule.h"

/* This test tests the macros for defining dynamic types.
 */

static xboolean_t loaded = FALSE;

struct _test_iface_class
{
  xtype_interface_t base_iface;
  xuint_t val;
};

static xtype_t test_iface_get_type (void);
#define TEST_TYPE_IFACE           (test_iface_get_type ())
#define TEST_IFACE_GET_CLASS(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), TEST_TYPE_IFACE, test_iface_class_t))
typedef struct _test_iface      test_iface_t;
typedef struct _test_iface_class test_iface_class_t;

static void test_iface_base_init    (test_iface_class_t *iface);
static void test_iface_default_init (test_iface_class_t *iface, xpointer_t class_data);

static DEFINE_IFACE(test_iface_t, test_iface, test_iface_base_init, test_iface_default_init)

static void
test_iface_default_init (test_iface_class_t *iface,
			 xpointer_t        class_data)
{
}

static void
test_iface_base_init (test_iface_class_t *iface)
{
}

xtype_t dynamic_object_get_type (void);
#define DYNAMIC_OBJECT_TYPE (dynamic_object_get_type ())

typedef xobject_t dynamic_object_t;
typedef struct _dynamic_object_class dynamic_object_class_t;

struct _dynamic_object_class
{
  xobject_class_t parent_class;
  xuint_t val;
};

static void dynamic_object_iface_init (test_iface_t *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(dynamic_object_t, dynamic_object, XTYPE_OBJECT, 0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC (TEST_TYPE_IFACE,
							      dynamic_object_iface_init));

static void
dynamic_object_class_init (dynamic_object_class_t *class)
{
  class->val = 42;
  loaded = TRUE;
}

static void
dynamic_object_class_finalize (dynamic_object_class_t *class)
{
  loaded = FALSE;
}

static void
dynamic_object_iface_init (test_iface_t *iface)
{
}

static void
dynamic_object_init (dynamic_object_t *dynamic_object)
{
}

static void
module_register (xtype_module_t *module)
{
  dynamic_object_register_type (module);
}

static void
test_dynamic_type (void)
{
  dynamic_object_class_t *class;

  test_module_new (module_register);

  /* Not loaded until we call ref for the first time */
  class = xtype_class_peek (DYNAMIC_OBJECT_TYPE);
  g_assert (class == NULL);
  g_assert (!loaded);

  /* Make sure interfaces work */
  g_assert (xtype_is_a (DYNAMIC_OBJECT_TYPE,
			 TEST_TYPE_IFACE));

  /* Ref loads */
  class = xtype_class_ref (DYNAMIC_OBJECT_TYPE);
  g_assert (class && class->val == 42);
  g_assert (loaded);

  /* Peek then works */
  class = xtype_class_peek (DYNAMIC_OBJECT_TYPE);
  g_assert (class && class->val == 42);
  g_assert (loaded);

  /* Make sure interfaces still work */
  g_assert (xtype_is_a (DYNAMIC_OBJECT_TYPE,
			 TEST_TYPE_IFACE));

  /* Unref causes finalize */
  xtype_class_unref (class);

  /* Peek returns NULL */
  class = xtype_class_peek (DYNAMIC_OBJECT_TYPE);
#if 0
  g_assert (!class);
  g_assert (!loaded);
#endif

  /* Ref reloads */
  class = xtype_class_ref (DYNAMIC_OBJECT_TYPE);
  g_assert (class && class->val == 42);
  g_assert (loaded);

  /* And Unref causes finalize once more*/
  xtype_class_unref (class);
  class = xtype_class_peek (DYNAMIC_OBJECT_TYPE);
#if 0
  g_assert (!class);
  g_assert (!loaded);
#endif
}

int
main (int   argc,
      char *argv[])
{
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
			  G_LOG_LEVEL_WARNING |
			  G_LOG_LEVEL_CRITICAL);

  test_dynamic_type ();

  return 0;
}
