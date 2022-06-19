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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "testcommon.h"

/* This test tests interface properties, implementing interface
 * properties and #GParamSpecOverride.
 *
 * Four properties are tested:
 *
 * prop1: Defined in test_iface_t, Implemented in base_object_t with a GParamSpecOverride
 * prop2: Defined in test_iface_t, Implemented in base_object_t with a new property
 * prop3: Defined in test_iface_t, Implemented in base_object_t, Overridden in derived_object_t
 * prop4: Defined in base_object_t, Overridden in derived_object_t
 */

static xtype_t base_object_get_type (void);
static xtype_t derived_object_get_type (void);

enum {
  BASE_PROP_0,
  BASE_PROP1,
  BASE_PROP2,
  BASE_PROP3,
  BASE_PROP4
};

enum {
  DERIVED_PROP_0,
  DERIVED_PROP3,
  DERIVED_PROP4
};

/*
 * base_object_t, a parent class for derived_object_t
 */
#define BASE_TYPE_OBJECT          (base_object_get_type ())
#define BASE_OBJECT(obj)          (XTYPE_CHECK_INSTANCE_CAST ((obj), BASE_TYPE_OBJECT, base_object_t))
typedef struct _BaseObject        base_object_t;
typedef struct _BaseObjectClass   base_object_class_t;

struct _BaseObject
{
  xobject_t parent_instance;

  xint_t val1;
  xint_t val2;
  xint_t val3;
  xint_t val4;
};
struct _BaseObjectClass
{
  xobject_class_t parent_class;
};

xobject_class_t *base_parent_class;

/*
 * derived_object_t, the child class of derived_object_t
 */
#define DERIVED_TYPE_OBJECT          (derived_object_get_type ())
typedef struct _derived_object        derived_object_t;
typedef struct _DerivedObjectClass   derived_object_class_t;

struct _derived_object
{
  base_object_t parent_instance;
};
struct _DerivedObjectClass
{
  base_object_class_t parent_class;
};

/*
 * The interface
 */
typedef struct _test_iface_class test_iface_class_t;

struct _test_iface_class
{
  xtype_interface_t base_iface;
};

#define TEST_TYPE_IFACE (test_iface_get_type ())

/* The paramspecs installed on our interface
 */
static xparam_spec_t *iface_spec1, *iface_spec2, *iface_spec3;

/* The paramspecs inherited by our derived object
 */
static xparam_spec_t *inherited_spec1, *inherited_spec2, *inherited_spec3, *inherited_spec4;

static void
test_iface_default_init (test_iface_class_t *iface_vtable)
{
  inherited_spec1 = iface_spec1 = xparam_spec_int ("prop1",
                                                    "Prop1",
                                                    "Property 1",
                                                    G_MININT, /* min */
                                                    0xFFFF,  /* max */
                                                    42,       /* default */
                                                    XPARAM_READWRITE | XPARAM_CONSTRUCT);
  xobject_interface_install_property (iface_vtable, iface_spec1);

  iface_spec2 = xparam_spec_int ("prop2",
                                  "Prop2",
                                  "Property 2",
                                  G_MININT, /* min */
                                  G_MAXINT, /* max */
                                  0,           /* default */
                                  XPARAM_WRITABLE);
  xobject_interface_install_property (iface_vtable, iface_spec2);

  inherited_spec3 = iface_spec3 = xparam_spec_int ("prop3",
                                                    "Prop3",
                                                    "Property 3",
                                                    G_MININT, /* min */
                                                    G_MAXINT, /* max */
                                                    0,         /* default */
                                                    XPARAM_READWRITE);
  xobject_interface_install_property (iface_vtable, iface_spec3);
}

static DEFINE_IFACE (test_iface, test_iface, NULL, test_iface_default_init)


static xobject_t*
base_object_constructor (xtype_t                  type,
                         xuint_t                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
  /* The constructor is the one place where a GParamSpecOverride is visible
   * to the outside world, so we do a bunch of checks here
   */
  xvalue_t value1 = G_VALUE_INIT;
  xvalue_t value2 = G_VALUE_INIT;
  xparam_spec_t *pspec;

  xassert (n_construct_properties == 1);

  pspec = construct_properties->pspec;

  /* Check we got the param spec we expected
   */
  xassert (X_IS_PARAM_SPEC_OVERRIDE (pspec));
  xassert (pspec->param_id == BASE_PROP1);
  xassert (strcmp (xparam_spec_get_name (pspec), "prop1") == 0);
  xassert (xparam_spec_get_redirect_target (pspec) == iface_spec1);

  /* test_t redirection of the nick and blurb to the redirect target
   */
  xassert (strcmp (xparam_spec_get_nick (pspec), "Prop1") == 0);
  xassert (strcmp (xparam_spec_get_blurb (pspec), "Property 1") == 0);

  /* test_t forwarding of the various xparam_spec_t methods to the redirect target
   */
  xvalue_init (&value1, XTYPE_INT);
  xvalue_init (&value2, XTYPE_INT);

  g_param_value_set_default (pspec, &value1);
  xassert (xvalue_get_int (&value1) == 42);

  xvalue_reset (&value1);
  xvalue_set_int (&value1, 0x10000);
  xassert (g_param_value_validate (pspec, &value1));
  xassert (xvalue_get_int (&value1) == 0xFFFF);
  xassert (!g_param_value_validate (pspec, &value1));

  xvalue_reset (&value1);
  xvalue_set_int (&value1, 1);
  xvalue_set_int (&value2, 2);
  xassert (g_param_values_cmp (pspec, &value1, &value2) < 0);
  xassert (g_param_values_cmp (pspec, &value2, &value1) > 0);

  xvalue_unset (&value1);
  xvalue_unset (&value2);

  return base_parent_class->constructor (type,
                                         n_construct_properties,
                                         construct_properties);
}

static void
base_object_set_property (xobject_t      *object,
                          xuint_t         prop_id,
                          const xvalue_t *value,
                          xparam_spec_t   *pspec)
{
  base_object_t *base_object = BASE_OBJECT (object);

  switch (prop_id)
    {
    case BASE_PROP1:
      xassert (pspec == inherited_spec1);
      base_object->val1 = xvalue_get_int (value);
      break;
    case BASE_PROP2:
      xassert (pspec == inherited_spec2);
      base_object->val2 = xvalue_get_int (value);
      break;
    case BASE_PROP3:
      g_assert_not_reached ();
      break;
    case BASE_PROP4:
      g_assert_not_reached ();
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
base_object_get_property (xobject_t    *object,
                          xuint_t       prop_id,
                          xvalue_t     *value,
                          xparam_spec_t *pspec)
{
  base_object_t *base_object = BASE_OBJECT (object);

  switch (prop_id)
    {
    case BASE_PROP1:
      xassert (pspec == inherited_spec1);
      xvalue_set_int (value, base_object->val1);
      break;
    case BASE_PROP2:
      xassert (pspec == inherited_spec2);
      xvalue_set_int (value, base_object->val2);
      break;
    case BASE_PROP3:
      g_assert_not_reached ();
      break;
    case BASE_PROP4:
      g_assert_not_reached ();
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
base_object_notify (xobject_t    *object,
                    xparam_spec_t *pspec)
{
  /* The property passed to notify is the redirect target, not the
   * GParamSpecOverride
   */
  xassert (pspec == inherited_spec1 ||
            pspec == inherited_spec2 ||
            pspec == inherited_spec3 ||
            pspec == inherited_spec4);
}

static void
base_object_class_init (base_object_class_t *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  base_parent_class= xtype_class_peek_parent (class);

  object_class->constructor = base_object_constructor;
  object_class->set_property = base_object_set_property;
  object_class->get_property = base_object_get_property;
  object_class->notify = base_object_notify;

  xobject_class_override_property (object_class, BASE_PROP1, "prop1");

  /* We override this one using a real property, not GParamSpecOverride
   * We change the flags from READONLY to READWRITE to show that we
   * can make the flags less restrictive
   */
  inherited_spec2 = xparam_spec_int ("prop2",
                                      "Prop2",
                                      "Property 2",
                                      G_MININT, /* min */
                                      G_MAXINT, /* max */
                                      0,        /* default */
                                      XPARAM_READWRITE);
  xobject_class_install_property (object_class, BASE_PROP2, inherited_spec2);

  xobject_class_override_property (object_class, BASE_PROP3, "prop3");

  inherited_spec4 = xparam_spec_int ("prop4",
                                      "Prop4",
                                      "Property 4",
                                      G_MININT, /* min */
                                      G_MAXINT, /* max */
                                      0,        /* default */
                                      XPARAM_READWRITE);
  xobject_class_install_property (object_class, BASE_PROP4, inherited_spec4);
}

static void
base_object_init (base_object_t *base_object)
{
  base_object->val1 = 42;
}

static DEFINE_TYPE_FULL (base_object, base_object,
                         base_object_class_init, NULL, base_object_init,
                         XTYPE_OBJECT,
                         INTERFACE (NULL, TEST_TYPE_IFACE))

static void
derived_object_set_property (xobject_t      *object,
                             xuint_t         prop_id,
                             const xvalue_t *value,
                             xparam_spec_t   *pspec)
{
  base_object_t *base_object = BASE_OBJECT (object);

  switch (prop_id)
    {
    case DERIVED_PROP3:
      xassert (pspec == inherited_spec3);
      base_object->val3 = xvalue_get_int (value);
      break;
    case DERIVED_PROP4:
      xassert (pspec == inherited_spec4);
      base_object->val4 = xvalue_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
derived_object_get_property (xobject_t    *object,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  base_object_t *base_object = BASE_OBJECT (object);

  switch (prop_id)
    {
    case DERIVED_PROP3:
      xassert (pspec == inherited_spec3);
      xvalue_set_int (value, base_object->val3);
      break;
    case DERIVED_PROP4:
      xassert (pspec == inherited_spec4);
      xvalue_set_int (value, base_object->val4);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
derived_object_class_init (derived_object_class_t *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->set_property = derived_object_set_property;
  object_class->get_property = derived_object_get_property;

  /* Overriding a property that is itself overriding an interface property */
  xobject_class_override_property (object_class, DERIVED_PROP3, "prop3");

  /* Overriding a property not from an interface */
  xobject_class_override_property (object_class, DERIVED_PROP4, "prop4");
}

static DEFINE_TYPE (derived_object, derived_object,
                    derived_object_class_init, NULL, NULL,
                    BASE_TYPE_OBJECT)

/* Helper function for testing ...list_properties() */
static void
assert_in_properties (xparam_spec_t  *param_spec,
                      xparam_spec_t **properties,
                      xint_t         n_properties)
{
  xint_t i;
  xboolean_t found = FALSE;

  for (i = 0; i < n_properties; i++)
    {
      if (properties[i] == param_spec)
        found = TRUE;
    }

  xassert (found);
}

/* test_t setting and getting the properties */
static void
test_set (void)
{
  base_object_t *object;
  xint_t val1, val2, val3, val4;

  object = xobject_new (DERIVED_TYPE_OBJECT, NULL);

  xobject_set (object,
                "prop1", 0x0101,
                "prop2", 0x0202,
                "prop3", 0x0303,
                "prop4", 0x0404,
                NULL);
  xobject_get (object,
                "prop1", &val1,
                "prop2", &val2,
                "prop3", &val3,
                "prop4", &val4,
                NULL);

  xassert (val1 == 0x0101);
  xassert (val2 == 0x0202);
  xassert (val3 == 0x0303);
  xassert (val4 == 0x0404);

  xobject_unref (object);
}

/* test_t that the right spec is passed on explicit notifications */
static void
test_notify (void)
{
  base_object_t *object;

  object = xobject_new (DERIVED_TYPE_OBJECT, NULL);

  xobject_freeze_notify (G_OBJECT (object));
  xobject_notify (G_OBJECT (object), "prop1");
  xobject_notify (G_OBJECT (object), "prop2");
  xobject_notify (G_OBJECT (object), "prop3");
  xobject_notify (G_OBJECT (object), "prop4");
  xobject_thaw_notify (G_OBJECT (object));

  xobject_unref (object);
}

/* test_t xobject_class_find_property() for overridden properties */
static void
test_find_overridden (void)
{
  xobject_class_t *object_class;

  object_class = xtype_class_peek (DERIVED_TYPE_OBJECT);

  xassert (xobject_class_find_property (object_class, "prop1") == inherited_spec1);
  xassert (xobject_class_find_property (object_class, "prop2") == inherited_spec2);
  xassert (xobject_class_find_property (object_class, "prop3") == inherited_spec3);
  xassert (xobject_class_find_property (object_class, "prop4") == inherited_spec4);
}

/* test_t xobject_class_list_properties() for overridden properties */
static void
test_list_overridden (void)
{
  xobject_class_t *object_class;
  xparam_spec_t **properties;
  xuint_t n_properties;

  object_class = xtype_class_peek (DERIVED_TYPE_OBJECT);

  properties = xobject_class_list_properties (object_class, &n_properties);
  xassert (n_properties == 4);
  assert_in_properties (inherited_spec1, properties, n_properties);
  assert_in_properties (inherited_spec2, properties, n_properties);
  assert_in_properties (inherited_spec3, properties, n_properties);
  assert_in_properties (inherited_spec4, properties, n_properties);
  g_free (properties);
}

/* test_t xobject_interface_find_property() */
static void
test_find_interface (void)
{
  test_iface_class_t *iface;

  iface = xtype_default_interface_peek (TEST_TYPE_IFACE);

  xassert (xobject_interface_find_property (iface, "prop1") == iface_spec1);
  xassert (xobject_interface_find_property (iface, "prop2") == iface_spec2);
  xassert (xobject_interface_find_property (iface, "prop3") == iface_spec3);
}

/* test_t xobject_interface_list_properties() */
static void
test_list_interface (void)
{
  test_iface_class_t *iface;
  xparam_spec_t **properties;
  xuint_t n_properties;

  iface = xtype_default_interface_peek (TEST_TYPE_IFACE);

  properties = xobject_interface_list_properties (iface, &n_properties);
  xassert (n_properties == 3);
  assert_in_properties (iface_spec1, properties, n_properties);
  assert_in_properties (iface_spec2, properties, n_properties);
  assert_in_properties (iface_spec3, properties, n_properties);
  g_free (properties);
}

/* base2_object_t, which implements the interface but fails
 * to override some of its properties
 */
#define BASE2_TYPE_OBJECT          (base2_object_get_type ())
#define BASE2_OBJECT(obj)          (XTYPE_CHECK_INSTANCE_CAST ((obj), BASE2_TYPE_OBJECT, base2_object_t))

typedef struct _base2_object        base2_object_t;
typedef struct _base2_object_class   base2_object_class_t;

static void
base2_object_test_iface_init (test_iface_class_t *iface)
{
}

enum {
  BASE2_PROP_0,
  BASE2_PROP1,
  BASE2_PROP2
};

struct _base2_object
{
  xobject_t parent_instance;
};

struct _base2_object_class
{
  xobject_class_t parent_class;
};

static xtype_t base2_object_get_type (void);
G_DEFINE_TYPE_WITH_CODE (base2_object, base2_object, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (TEST_TYPE_IFACE,
                                                base2_object_test_iface_init))

static void
base2_object_get_property (xobject_t    *object,
                           xuint_t       prop_id,
                           xvalue_t     *value,
                           xparam_spec_t *pspec)
{
  switch (prop_id)
    {
    case BASE2_PROP1:
      xvalue_set_int (value, 0);
      break;
    case BASE2_PROP2:
      xvalue_set_int (value, 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
base2_object_set_property (xobject_t      *object,
                           xuint_t         prop_id,
                           const xvalue_t *value,
                           xparam_spec_t   *pspec)
{
  switch (prop_id)
    {
    case BASE2_PROP1:
      break;
    case BASE2_PROP2:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
base2_object_class_init (base2_object_class_t *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->set_property = base2_object_set_property;
  object_class->get_property = base2_object_get_property;

  xobject_class_override_property (object_class, BASE2_PROP1, "prop1");
  xobject_class_override_property (object_class, BASE2_PROP2, "prop2");
}

static void
base2_object_init (base2_object_t *object)
{
}

static void
test_not_overridden (void)
{
  base2_object_t *object;

  if (!g_test_undefined ())
    return;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=637738");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*base2_object_t doesn't implement property 'prop3' from interface 'test_iface_t'*");
  object = xobject_new (BASE2_TYPE_OBJECT, NULL);
  g_test_assert_expected_messages ();

  xobject_unref (object);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/interface/properties/set", test_set);
  g_test_add_func ("/interface/properties/notify", test_notify);
  g_test_add_func ("/interface/properties/find-overridden", test_find_overridden);
  g_test_add_func ("/interface/properties/list-overridden", test_list_overridden);
  g_test_add_func ("/interface/properties/find-interface", test_find_interface);
  g_test_add_func ("/interface/properties/list-interface", test_list_interface);
  g_test_add_func ("/interface/properties/not-overridden", test_not_overridden);

  return g_test_run ();
}
