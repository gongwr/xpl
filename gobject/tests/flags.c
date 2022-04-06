/* flags.c
 * Copyright (C) 2018 Arthur Demchenkov
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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <glib-object.h>

/* Check that validation of flags works on architectures where
 * #xint_t and #xlong_t are different sizes, as the flags are cast
 * between types a few times.
 *
 * See: https://gitlab.gnome.org/GNOME/glib/issues/1572
 */

enum {
  PROP_FLAGS = 1
};

typedef struct _xtest xtest_t;
typedef struct _xtest_class xtest_class_t;

typedef enum {
  NO_FLAG      = 0,
  LOWEST_FLAG  = 1,
  HIGHEST_FLAG = 1 << 31
} MyFlagsEnum;

struct _xtest {
  xobject_t object;
  MyFlagsEnum flags;
};

struct _xtest_class {
  xobject_class_t parent_class;
};

static xtype_t xtest_get_type (void);
static xtype_t xtest_flags_get_type (void);

#define XTYPE_TEST (xtest_get_type())
#define XTEST(test) (XTYPE_CHECK_INSTANCE_CAST ((test), XTYPE_TEST, xtest_t))
G_DEFINE_TYPE (xtest, xtest, XTYPE_OBJECT)

static void xtest_class_init (xtest_class_t * klass);
static void xtest_init (xtest_t * test);
static void xtest_get_property (xobject_t    *object,
				  xuint_t       prop_id,
				  xvalue_t     *value,
				  xparam_spec_t *pspec);
static void xtest_set_property (xobject_t      *object,
				  xuint_t         prop_id,
				  const xvalue_t *value,
				  xparam_spec_t   *pspec);

static xtype_t
xtest_flags_get_type (void)
{
  static xtype_t flags_type = 0;

  if (G_UNLIKELY(flags_type == 0))
    {
      static const xflags_value_t values[] = {
	{ LOWEST_FLAG,  "LOWEST_FLAG",  "lowest" },
	{ HIGHEST_FLAG, "HIGHEST_FLAG", "highest" },
	{ 0, NULL, NULL }
      };

      flags_type = xflags_register_static (g_intern_static_string ("GTestFlags"), values);
    }
  return flags_type;
}

static void
xtest_class_init (xtest_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xtest_get_property;
  gobject_class->set_property = xtest_set_property;

  xobject_class_install_property (gobject_class, 1,
				   g_param_spec_flags ("flags",
						       "Flags",
						       "Flags test property",
						       xtest_flags_get_type(), 0,
						       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void xtest_init (xtest_t *test)
{
}

static void
xtest_get_property (xobject_t    *object,
		      xuint_t       prop_id,
		      xvalue_t     *value,
		      xparam_spec_t *pspec)
{
  xtest_t *test = XTEST (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      xvalue_set_flags (value, test->flags);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xtest_set_property (xobject_t      *object,
		      xuint_t         prop_id,
		      const xvalue_t *value,
		      xparam_spec_t   *pspec)
{
  xtest_t *test = XTEST (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      test->flags = xvalue_get_flags (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
check_flags_validation (void)
{
  xuint_t test_flags[] = {
    NO_FLAG,
    LOWEST_FLAG,
    HIGHEST_FLAG,
    LOWEST_FLAG | HIGHEST_FLAG
  };
  xuint_t flag_read;
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (test_flags); i++)
    {
      xuint_t flag_set = test_flags[i];
      xobject_t *test = xobject_new (XTYPE_TEST,
				    "flags", flag_set,
				    NULL);

      xobject_get (test, "flags", &flag_read, NULL);

      /* This check will fail in case of xint_t -> xlong_t conversion
       * in value_flags_enum_collect_value() */
      g_assert_cmpint (flag_read, ==, flag_set);

      xobject_unref (test);
    }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gobject/flags/validate", check_flags_validation);
  return g_test_run ();
}
