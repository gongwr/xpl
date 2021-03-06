/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2009 Red Hat, Inc.
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

#include <math.h>
#include <string.h>
#include <glib-object.h>
#include "testcommon.h"

#define WARM_UP_N_RUNS 50
#define ESTIMATE_ROUND_TIME_N_RUNS 5
#define DEFAULT_TEST_TIME 15 /* seconds */
 /* The time we want each round to take, in seconds, this should
  * be large enough compared to the timer resolution, but small
  * enough that the risk of any random slowness will miss the
  * running window */
#define TARGET_ROUND_TIME 0.008

static xboolean_t verbose = FALSE;
static int test_length = DEFAULT_TEST_TIME;

static GOptionEntry cmd_entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Print extra information", NULL},
  {"seconds", 's', 0, G_OPTION_ARG_INT, &test_length,
   "Time to run each test in seconds", NULL},
  G_OPTION_ENTRY_NULL
};

typedef struct _PerformanceTest PerformanceTest;
struct _PerformanceTest {
  const char *name;
  xpointer_t extra_data;

  xpointer_t (*setup) (PerformanceTest *test);
  void (*init) (PerformanceTest *test,
		xpointer_t data,
		double factor);
  void (*run) (PerformanceTest *test,
	       xpointer_t data);
  void (*finish) (PerformanceTest *test,
		  xpointer_t data);
  void (*teardown) (PerformanceTest *test,
		    xpointer_t data);
  void (*print_result) (PerformanceTest *test,
			xpointer_t data,
			double time);
};

static void
run_test (PerformanceTest *test)
{
  xpointer_t data = NULL;
  xuint64_t i, num_rounds;
  double elapsed, min_elapsed, max_elapsed, avg_elapsed, factor;
  xtimer_t *timer;

  g_print ("Running test %s\n", test->name);

  /* Set up test */
  timer = g_timer_new ();
  data = test->setup (test);

  if (verbose)
    g_print ("Warming up\n");

  g_timer_start (timer);

  /* Warm up the test by doing a few runs */
  for (i = 0; i < WARM_UP_N_RUNS; i++)
    {
      test->init (test, data, 1.0);
      test->run (test, data);
      test->finish (test, data);
    }

  g_timer_stop (timer);
  elapsed = g_timer_elapsed (timer, NULL);

  if (verbose)
    {
      g_print ("Warm up time: %.2f secs\n", elapsed);
      g_print ("Estimating round time\n");
    }

  /* Estimate time for one run by doing a few test rounds */
  min_elapsed = 0;
  for (i = 0; i < ESTIMATE_ROUND_TIME_N_RUNS; i++)
    {
      test->init (test, data, 1.0);
      g_timer_start (timer);
      test->run (test, data);
      g_timer_stop (timer);
      test->finish (test, data);

      elapsed = g_timer_elapsed (timer, NULL);
      if (i == 0)
	min_elapsed = elapsed;
      else
	min_elapsed = MIN (min_elapsed, elapsed);
    }

  factor = TARGET_ROUND_TIME / min_elapsed;

  if (verbose)
    g_print ("Uncorrected round time: %.4f msecs, correction factor %.2f\n", 1000*min_elapsed, factor);

  /* Calculate number of rounds needed */
  num_rounds = (test_length / TARGET_ROUND_TIME) + 1;

  if (verbose)
    g_print ("Running %"G_GINT64_MODIFIER"d rounds\n", num_rounds);

  /* Run the test */
  avg_elapsed = 0.0;
  min_elapsed = 0.0;
  max_elapsed = 0.0;
  for (i = 0; i < num_rounds; i++)
    {
      test->init (test, data, factor);
      g_timer_start (timer);
      test->run (test, data);
      g_timer_stop (timer);
      test->finish (test, data);
      elapsed = g_timer_elapsed (timer, NULL);

      if (i == 0)
	max_elapsed = min_elapsed = avg_elapsed = elapsed;
      else
        {
          min_elapsed = MIN (min_elapsed, elapsed);
          max_elapsed = MAX (max_elapsed, elapsed);
          avg_elapsed += elapsed;
        }
    }

  if (num_rounds > 1)
    avg_elapsed = avg_elapsed / num_rounds;

  if (verbose)
    {
      g_print ("Minimum corrected round time: %.2f msecs\n", min_elapsed * 1000);
      g_print ("Maximum corrected round time: %.2f msecs\n", max_elapsed * 1000);
      g_print ("Average corrected round time: %.2f msecs\n", avg_elapsed * 1000);
    }

  /* Print the results */
  test->print_result (test, data, min_elapsed);

  /* Tear down */
  test->teardown (test, data);
  g_timer_destroy (timer);
}

/*************************************************************
 * Simple object is a very simple small xobject_t subclass
 * with no properties, no signals, implementing no interfaces
 *************************************************************/

static xtype_t simple_object_get_type (void);
#define SIMPLE_TYPE_OBJECT        (simple_object_get_type ())
typedef struct _simple_object      simple_object_t;
typedef struct _simple_object_class   simple_object_class_t;

struct _simple_object
{
  xobject_t parent_instance;
  int val;
};

struct _simple_object_class
{
  xobject_class_t parent_class;
};

XDEFINE_TYPE (simple_object, simple_object, XTYPE_OBJECT)

static void
simple_object_finalize (xobject_t *object)
{
  XOBJECT_CLASS (simple_object_parent_class)->finalize (object);
}

static void
simple_object_class_init (simple_object_class_t *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->finalize = simple_object_finalize;
}

static void
simple_object_init (simple_object_t *simple_object)
{
  simple_object->val = 42;
}

typedef struct _test_iface_class test_iface_class_t;
typedef struct _test_iface_class TestIface1Class;
typedef struct _test_iface_class TestIface2Class;
typedef struct _test_iface_class TestIface3Class;
typedef struct _test_iface_class TestIface4Class;
typedef struct _test_iface_class TestIface5Class;
typedef struct _test_iface test_iface_t;

struct _test_iface_class
{
  xtype_interface_t base_iface;
  void (*method) (test_iface_t *obj);
};

static xtype_t test_iface1_get_type (void);
static xtype_t test_iface2_get_type (void);
static xtype_t test_iface3_get_type (void);
static xtype_t test_iface4_get_type (void);
static xtype_t test_iface5_get_type (void);

#define TEST_TYPE_IFACE1 (test_iface1_get_type ())
#define TEST_TYPE_IFACE2 (test_iface2_get_type ())
#define TEST_TYPE_IFACE3 (test_iface3_get_type ())
#define TEST_TYPE_IFACE4 (test_iface4_get_type ())
#define TEST_TYPE_IFACE5 (test_iface5_get_type ())

static DEFINE_IFACE (test_iface1, test_iface1, NULL, NULL)
static DEFINE_IFACE (test_iface2, test_iface2, NULL, NULL)
static DEFINE_IFACE (test_iface3, test_iface3, NULL, NULL)
static DEFINE_IFACE (test_iface4, test_iface4, NULL, NULL)
static DEFINE_IFACE (test_iface5, test_iface5, NULL, NULL)

/*************************************************************
 * Complex object is a xobject_t subclass with a properties,
 * construct properties, signals and implementing an interface.
 *************************************************************/

static xtype_t complex_object_get_type (void);
#define COMPLEX_TYPE_OBJECT        (complex_object_get_type ())
typedef struct _complex_object      complex_object_t;
typedef struct _complex_object_class complex_object_class_t;

struct _complex_object
{
  xobject_t parent_instance;
  int val1;
  int val2;
};

struct _complex_object_class
{
  xobject_class_t parent_class;

  void (*signal) (complex_object_t *obj);
  void (*signal_empty) (complex_object_t *obj);
};

static void complex_test_iface_init (xpointer_t         x_iface,
				     xpointer_t         iface_data);

G_DEFINE_TYPE_EXTENDED (complex_object_t, complex_object,
			XTYPE_OBJECT, 0,
			G_IMPLEMENT_INTERFACE (TEST_TYPE_IFACE1, complex_test_iface_init)
			G_IMPLEMENT_INTERFACE (TEST_TYPE_IFACE2, complex_test_iface_init)
			G_IMPLEMENT_INTERFACE (TEST_TYPE_IFACE3, complex_test_iface_init)
			G_IMPLEMENT_INTERFACE (TEST_TYPE_IFACE4, complex_test_iface_init)
			G_IMPLEMENT_INTERFACE (TEST_TYPE_IFACE5, complex_test_iface_init))

#define COMPLEX_OBJECT(object) (XTYPE_CHECK_INSTANCE_CAST ((object), COMPLEX_TYPE_OBJECT, complex_object_t))

enum {
  PROP_0,
  PROP_VAL1,
  PROP_VAL2
};

enum {
  COMPLEX_SIGNAL,
  COMPLEX_SIGNAL_EMPTY,
  COMPLEX_SIGNAL_GENERIC,
  COMPLEX_SIGNAL_GENERIC_EMPTY,
  COMPLEX_SIGNAL_ARGS,
  COMPLEX_LAST_SIGNAL
};

static xuint_t complex_signals[COMPLEX_LAST_SIGNAL] = { 0 };

static void
complex_object_finalize (xobject_t *object)
{
  XOBJECT_CLASS (complex_object_parent_class)->finalize (object);
}

static void
complex_object_set_property (xobject_t         *object,
			     xuint_t            prop_id,
			     const xvalue_t    *value,
			     xparam_spec_t      *pspec)
{
  complex_object_t *complex = COMPLEX_OBJECT (object);

  switch (prop_id)
    {
    case PROP_VAL1:
      complex->val1 = xvalue_get_int (value);
      break;
    case PROP_VAL2:
      complex->val2 = xvalue_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
complex_object_get_property (xobject_t         *object,
			     xuint_t            prop_id,
			     xvalue_t          *value,
			     xparam_spec_t      *pspec)
{
  complex_object_t *complex = COMPLEX_OBJECT (object);

  switch (prop_id)
    {
    case PROP_VAL1:
      xvalue_set_int (value, complex->val1);
      break;
    case PROP_VAL2:
      xvalue_set_int (value, complex->val2);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
complex_object_real_signal (complex_object_t *obj)
{
}

static void
complex_object_class_init (complex_object_class_t *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->finalize = complex_object_finalize;
  object_class->set_property = complex_object_set_property;
  object_class->get_property = complex_object_get_property;

  class->signal = complex_object_real_signal;

  complex_signals[COMPLEX_SIGNAL] =
    xsignal_new ("signal",
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (complex_object_class_t, signal),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  XTYPE_NONE, 0);

  complex_signals[COMPLEX_SIGNAL_EMPTY] =
    xsignal_new ("signal-empty",
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (complex_object_class_t, signal_empty),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  XTYPE_NONE, 0);

  complex_signals[COMPLEX_SIGNAL_GENERIC] =
    xsignal_new ("signal-generic",
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (complex_object_class_t, signal),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 0);
  complex_signals[COMPLEX_SIGNAL_GENERIC_EMPTY] =
    xsignal_new ("signal-generic-empty",
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (complex_object_class_t, signal_empty),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 0);

  complex_signals[COMPLEX_SIGNAL_ARGS] =
    xsignal_new ("signal-args",
                  XTYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (complex_object_class_t, signal),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT_POINTER,
                  XTYPE_NONE, 2, XTYPE_UINT, XTYPE_POINTER);

  xobject_class_install_property (object_class,
				   PROP_VAL1,
				   xparam_spec_int ("val1",
 						     "val1",
 						     "val1",
 						     0,
 						     G_MAXINT,
 						     42,
 						     XPARAM_CONSTRUCT | XPARAM_READWRITE));
  xobject_class_install_property (object_class,
				   PROP_VAL2,
				   xparam_spec_int ("val2",
 						     "val2",
 						     "val2",
 						     0,
 						     G_MAXINT,
 						     43,
 						     XPARAM_READWRITE));


}

static void
complex_object_iface_method (test_iface_t *obj)
{
  complex_object_t *complex = COMPLEX_OBJECT (obj);
  complex->val1++;
}

static void
complex_test_iface_init (xpointer_t         x_iface,
			 xpointer_t         iface_data)
{
  test_iface_class_t *iface = x_iface;
  iface->method = complex_object_iface_method;
}

static void
complex_object_init (complex_object_t *complex_object)
{
  complex_object->val2 = 43;
}

/*************************************************************
 * test_t object construction performance
 *************************************************************/

#define NUM_OBJECT_TO_CONSTRUCT 10000

struct ConstructionTest {
  xobject_t **objects;
  int n_objects;
  xtype_t type;
};

static xpointer_t
test_construction_setup (PerformanceTest *test)
{
  struct ConstructionTest *data;

  data = g_new0 (struct ConstructionTest, 1);
  data->type = ((xtype_t (*)(void))test->extra_data)();

  return data;
}

static void
test_construction_init (PerformanceTest *test,
			xpointer_t _data,
			double count_factor)
{
  struct ConstructionTest *data = _data;
  int n;

  n = NUM_OBJECT_TO_CONSTRUCT * count_factor;
  if (data->n_objects != n)
    {
      data->n_objects = n;
      data->objects = g_new (xobject_t *, n);
    }
}

static void
test_construction_run (PerformanceTest *test,
		       xpointer_t _data)
{
  struct ConstructionTest *data = _data;
  xobject_t **objects = data->objects;
  xtype_t type = data->type;
  int i, n_objects;

  n_objects = data->n_objects;
  for (i = 0; i < n_objects; i++)
    objects[i] = xobject_new (type, NULL);
}

static void
test_construction_finish (PerformanceTest *test,
			  xpointer_t _data)
{
  struct ConstructionTest *data = _data;
  int i;

  for (i = 0; i < data->n_objects; i++)
    xobject_unref (data->objects[i]);
}

static void
test_construction_teardown (PerformanceTest *test,
			    xpointer_t _data)
{
  struct ConstructionTest *data = _data;
  g_free (data->objects);
  g_free (data);
}

static void
test_construction_print_result (PerformanceTest *test,
				xpointer_t _data,
				double time)
{
  struct ConstructionTest *data = _data;

  g_print ("Millions of constructed objects per second: %.3f\n",
	   data->n_objects / (time * 1000000));
}

/*************************************************************
 * test_t runtime type check performance
 *************************************************************/

#define NUM_KILO_CHECKS_PER_ROUND 50

struct TypeCheckTest {
  xobject_t *object;
  int n_checks;
};

static xpointer_t
test_type_check_setup (PerformanceTest *test)
{
  struct TypeCheckTest *data;

  data = g_new0 (struct TypeCheckTest, 1);
  data->object = xobject_new (COMPLEX_TYPE_OBJECT, NULL);

  return data;
}

static void
test_type_check_init (PerformanceTest *test,
		      xpointer_t _data,
		      double factor)
{
  struct TypeCheckTest *data = _data;

  data->n_checks = factor * NUM_KILO_CHECKS_PER_ROUND;
}


/* Work around xtype_check_instance_is_a being marked "pure",
   and thus only called once for the loop. */
xboolean_t (*my_type_check_instance_is_a) (GTypeInstance *type_instance,
					 xtype_t          iface_type) = &xtype_check_instance_is_a;

static void
test_type_check_run (PerformanceTest *test,
		     xpointer_t _data)
{
  struct TypeCheckTest *data = _data;
  xobject_t *object = data->object;
  xtype_t type, types[5];
  int i, j;

  types[0] = test_iface1_get_type ();
  types[1] = test_iface2_get_type ();
  types[2] = test_iface3_get_type ();
  types[3] = test_iface4_get_type ();
  types[4] = test_iface5_get_type ();

  for (i = 0; i < data->n_checks; i++)
    {
      type = types[i%5];
      for (j = 0; j < 1000; j++)
	{
	  my_type_check_instance_is_a ((GTypeInstance *)object,
				       type);
	}
    }
}

static void
test_type_check_finish (PerformanceTest *test,
			xpointer_t data)
{
}

static void
test_type_check_print_result (PerformanceTest *test,
			      xpointer_t _data,
			      double time)
{
  struct TypeCheckTest *data = _data;
  g_print ("Million type checks per second: %.2f\n",
	   data->n_checks / (1000*time));
}

static void
test_type_check_teardown (PerformanceTest *test,
			  xpointer_t _data)
{
  struct TypeCheckTest *data = _data;

  xobject_unref (data->object);
  g_free (data);
}

/*************************************************************
 * test_t signal emissions performance (common code)
 *************************************************************/

#define NUM_EMISSIONS_PER_ROUND 10000

struct EmissionTest {
  xobject_t *object;
  int n_checks;
  int signal_id;
};

static void
test_emission_run (PerformanceTest *test,
                             xpointer_t _data)
{
  struct EmissionTest *data = _data;
  xobject_t *object = data->object;
  int i;

  for (i = 0; i < data->n_checks; i++)
    xsignal_emit (object, data->signal_id, 0);
}

static void
test_emission_run_args (PerformanceTest *test,
                        xpointer_t _data)
{
  struct EmissionTest *data = _data;
  xobject_t *object = data->object;
  int i;

  for (i = 0; i < data->n_checks; i++)
    xsignal_emit (object, data->signal_id, 0, 0, NULL);
}

/*************************************************************
 * test_t signal unhandled emissions performance
 *************************************************************/

static xpointer_t
test_emission_unhandled_setup (PerformanceTest *test)
{
  struct EmissionTest *data;

  data = g_new0 (struct EmissionTest, 1);
  data->object = xobject_new (COMPLEX_TYPE_OBJECT, NULL);
  data->signal_id = complex_signals[GPOINTER_TO_INT (test->extra_data)];
  return data;
}

static void
test_emission_unhandled_init (PerformanceTest *test,
                              xpointer_t _data,
                              double factor)
{
  struct EmissionTest *data = _data;

  data->n_checks = factor * NUM_EMISSIONS_PER_ROUND;
}

static void
test_emission_unhandled_finish (PerformanceTest *test,
                                xpointer_t data)
{
}

static void
test_emission_unhandled_print_result (PerformanceTest *test,
                                      xpointer_t _data,
                                      double time)
{
  struct EmissionTest *data = _data;

  g_print ("Emissions per second: %.0f\n",
	   data->n_checks / time);
}

static void
test_emission_unhandled_teardown (PerformanceTest *test,
                                  xpointer_t _data)
{
  struct EmissionTest *data = _data;

  xobject_unref (data->object);
  g_free (data);
}

/*************************************************************
 * test_t signal handled emissions performance
 *************************************************************/

static void
test_emission_handled_handler (complex_object_t *obj, xpointer_t data)
{
}

static xpointer_t
test_emission_handled_setup (PerformanceTest *test)
{
  struct EmissionTest *data;

  data = g_new0 (struct EmissionTest, 1);
  data->object = xobject_new (COMPLEX_TYPE_OBJECT, NULL);
  data->signal_id = complex_signals[GPOINTER_TO_INT (test->extra_data)];
  xsignal_connect (data->object, "signal",
                    G_CALLBACK (test_emission_handled_handler),
                    NULL);
  xsignal_connect (data->object, "signal-empty",
                    G_CALLBACK (test_emission_handled_handler),
                    NULL);
  xsignal_connect (data->object, "signal-generic",
                    G_CALLBACK (test_emission_handled_handler),
                    NULL);
  xsignal_connect (data->object, "signal-generic-empty",
                    G_CALLBACK (test_emission_handled_handler),
                    NULL);
  xsignal_connect (data->object, "signal-args",
                    G_CALLBACK (test_emission_handled_handler),
                    NULL);

  return data;
}

static void
test_emission_handled_init (PerformanceTest *test,
                            xpointer_t _data,
                            double factor)
{
  struct EmissionTest *data = _data;

  data->n_checks = factor * NUM_EMISSIONS_PER_ROUND;
}

static void
test_emission_handled_finish (PerformanceTest *test,
                              xpointer_t data)
{
}

static void
test_emission_handled_print_result (PerformanceTest *test,
                                    xpointer_t _data,
                                    double time)
{
  struct EmissionTest *data = _data;

  g_print ("Emissions per second: %.0f\n",
	   data->n_checks / time);
}

static void
test_emission_handled_teardown (PerformanceTest *test,
                                xpointer_t _data)
{
  struct EmissionTest *data = _data;

  xobject_unref (data->object);
  g_free (data);
}

/*************************************************************
 * test_t object refcount performance
 *************************************************************/

#define NUM_KILO_REFS_PER_ROUND 100000

struct RefcountTest {
  xobject_t *object;
  int n_checks;
};

static xpointer_t
test_refcount_setup (PerformanceTest *test)
{
  struct RefcountTest *data;

  data = g_new0 (struct RefcountTest, 1);
  data->object = xobject_new (COMPLEX_TYPE_OBJECT, NULL);

  return data;
}

static void
test_refcount_init (PerformanceTest *test,
                    xpointer_t _data,
                    double factor)
{
  struct RefcountTest *data = _data;

  data->n_checks = factor * NUM_KILO_REFS_PER_ROUND;
}

static void
test_refcount_run (PerformanceTest *test,
                   xpointer_t _data)
{
  struct RefcountTest *data = _data;
  xobject_t *object = data->object;
  int i;

  for (i = 0; i < data->n_checks; i++)
    {
      xobject_ref (object);
      xobject_ref (object);
      xobject_ref (object);
      xobject_unref (object);
      xobject_unref (object);

      xobject_ref (object);
      xobject_ref (object);
      xobject_unref (object);
      xobject_unref (object);
      xobject_unref (object);
    }
}

static void
test_refcount_finish (PerformanceTest *test,
                      xpointer_t _data)
{
}

static void
test_refcount_print_result (PerformanceTest *test,
			      xpointer_t _data,
			      double time)
{
  struct RefcountTest *data = _data;
  g_print ("Million refs+unref per second: %.2f\n",
	   data->n_checks * 5 / (time * 1000000 ));
}

static void
test_refcount_teardown (PerformanceTest *test,
			  xpointer_t _data)
{
  struct RefcountTest *data = _data;

  xobject_unref (data->object);
  g_free (data);
}

/*************************************************************
 * Main test code
 *************************************************************/

static PerformanceTest tests[] = {
  {
    "simple-construction",
    simple_object_get_type,
    test_construction_setup,
    test_construction_init,
    test_construction_run,
    test_construction_finish,
    test_construction_teardown,
    test_construction_print_result
  },
  {
    "complex-construction",
    complex_object_get_type,
    test_construction_setup,
    test_construction_init,
    test_construction_run,
    test_construction_finish,
    test_construction_teardown,
    test_construction_print_result
  },
  {
    "type-check",
    NULL,
    test_type_check_setup,
    test_type_check_init,
    test_type_check_run,
    test_type_check_finish,
    test_type_check_teardown,
    test_type_check_print_result
  },
  {
    "emit-unhandled",
    GINT_TO_POINTER (COMPLEX_SIGNAL),
    test_emission_unhandled_setup,
    test_emission_unhandled_init,
    test_emission_run,
    test_emission_unhandled_finish,
    test_emission_unhandled_teardown,
    test_emission_unhandled_print_result
  },
  {
    "emit-unhandled-empty",
    GINT_TO_POINTER (COMPLEX_SIGNAL_EMPTY),
    test_emission_unhandled_setup,
    test_emission_unhandled_init,
    test_emission_run,
    test_emission_unhandled_finish,
    test_emission_unhandled_teardown,
    test_emission_unhandled_print_result
  },
  {
    "emit-unhandled-generic",
    GINT_TO_POINTER (COMPLEX_SIGNAL_GENERIC),
    test_emission_unhandled_setup,
    test_emission_unhandled_init,
    test_emission_run,
    test_emission_unhandled_finish,
    test_emission_unhandled_teardown,
    test_emission_unhandled_print_result
  },
  {
    "emit-unhandled-generic-empty",
    GINT_TO_POINTER (COMPLEX_SIGNAL_GENERIC_EMPTY),
    test_emission_unhandled_setup,
    test_emission_unhandled_init,
    test_emission_run,
    test_emission_unhandled_finish,
    test_emission_unhandled_teardown,
    test_emission_unhandled_print_result
  },
  {
    "emit-unhandled-args",
    GINT_TO_POINTER (COMPLEX_SIGNAL_ARGS),
    test_emission_unhandled_setup,
    test_emission_unhandled_init,
    test_emission_run_args,
    test_emission_unhandled_finish,
    test_emission_unhandled_teardown,
    test_emission_unhandled_print_result
  },
  {
    "emit-handled",
    GINT_TO_POINTER (COMPLEX_SIGNAL),
    test_emission_handled_setup,
    test_emission_handled_init,
    test_emission_run,
    test_emission_handled_finish,
    test_emission_handled_teardown,
    test_emission_handled_print_result
  },
  {
    "emit-handled-empty",
    GINT_TO_POINTER (COMPLEX_SIGNAL_EMPTY),
    test_emission_handled_setup,
    test_emission_handled_init,
    test_emission_run,
    test_emission_handled_finish,
    test_emission_handled_teardown,
    test_emission_handled_print_result
  },
  {
    "emit-handled-generic",
    GINT_TO_POINTER (COMPLEX_SIGNAL_GENERIC),
    test_emission_handled_setup,
    test_emission_handled_init,
    test_emission_run,
    test_emission_handled_finish,
    test_emission_handled_teardown,
    test_emission_handled_print_result
  },
  {
    "emit-handled-generic-empty",
    GINT_TO_POINTER (COMPLEX_SIGNAL_GENERIC_EMPTY),
    test_emission_handled_setup,
    test_emission_handled_init,
    test_emission_run,
    test_emission_handled_finish,
    test_emission_handled_teardown,
    test_emission_handled_print_result
  },
  {
    "emit-handled-args",
    GINT_TO_POINTER (COMPLEX_SIGNAL_ARGS),
    test_emission_handled_setup,
    test_emission_handled_init,
    test_emission_run_args,
    test_emission_handled_finish,
    test_emission_handled_teardown,
    test_emission_handled_print_result
  },
  {
    "refcount",
    NULL,
    test_refcount_setup,
    test_refcount_init,
    test_refcount_run,
    test_refcount_finish,
    test_refcount_teardown,
    test_refcount_print_result
  }
};

static PerformanceTest *
find_test (const char *name)
{
  xsize_t i;
  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      if (strcmp (tests[i].name, name) == 0)
	return &tests[i];
    }
  return NULL;
}
int
main (int   argc,
      char *argv[])
{
  PerformanceTest *test;
  xoption_context_t *context;
  xerror_t *error = NULL;
  int i;

  context = g_option_context_new ("xobject_t performance tests");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (argc > 1)
    {
      for (i = 1; i < argc; i++)
	{
	  test = find_test (argv[i]);
	  if (test)
	    run_test (test);
	}
    }
  else
    {
      xsize_t k;
      for (k = 0; k < G_N_ELEMENTS (tests); k++)
        run_test (&tests[k]);
    }

  return 0;
}
