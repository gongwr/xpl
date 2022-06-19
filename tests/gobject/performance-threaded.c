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

#define DEFAULT_TEST_TIME 2 /* seconds */

static xtype_t
simple_register_class (const char *name, xtype_t parent, ...)
{
  xinterface_info_t interface_info = { NULL, NULL, NULL };
  va_list args;
  xtype_t type, interface;

  va_start (args, parent);
  type = xtype_register_static_simple (parent, name, sizeof (xobject_class_t),
      NULL, parent == XTYPE_INTERFACE ? 0 : sizeof (xobject_t), NULL, 0);
  for (;;)
    {
      interface = va_arg (args, xtype_t);
      if (interface == 0)
        break;
      xtype_add_interface_static (type, interface, &interface_info);
    }
  va_end (args);

  return type;
}

/* test emulating liststore behavior for interface lookups */

static xtype_t liststore;
static xtype_t liststore_interfaces[6];

static xpointer_t
register_types (void)
{
  static xsize_t inited = 0;
  if (g_once_init_enter (&inited))
    {
      liststore_interfaces[0] = simple_register_class ("GtkBuildable", XTYPE_INTERFACE, 0);
      liststore_interfaces[1] = simple_register_class ("GtkTreeDragDest", XTYPE_INTERFACE, 0);
      liststore_interfaces[2] = simple_register_class ("GtkTreeModel", XTYPE_INTERFACE, 0);
      liststore_interfaces[3] = simple_register_class ("GtkTreeDragSource", XTYPE_INTERFACE, 0);
      liststore_interfaces[4] = simple_register_class ("GtkTreeSortable", XTYPE_INTERFACE, 0);
      liststore_interfaces[5] = simple_register_class ("UnrelatedInterface", XTYPE_INTERFACE, 0);

      liststore = simple_register_class ("GtkListStore", XTYPE_OBJECT,
          liststore_interfaces[0], liststore_interfaces[1], liststore_interfaces[2],
          liststore_interfaces[3], liststore_interfaces[4], (xtype_t) 0);

      g_once_init_leave (&inited, 1);
    }
  return NULL;
}

static void
liststore_is_a_run (xpointer_t data)
{
  xuint_t i;

  for (i = 0; i < 1000; i++)
    {
      xassert (xtype_is_a (liststore, liststore_interfaces[0]));
      xassert (xtype_is_a (liststore, liststore_interfaces[1]));
      xassert (xtype_is_a (liststore, liststore_interfaces[2]));
      xassert (xtype_is_a (liststore, liststore_interfaces[3]));
      xassert (xtype_is_a (liststore, liststore_interfaces[4]));
      xassert (!xtype_is_a (liststore, liststore_interfaces[5]));
    }
}

static xpointer_t
liststore_get_class (void)
{
  register_types ();
  return xtype_class_ref (liststore);
}

static void
liststore_interface_peek_run (xpointer_t klass)
{
  xuint_t i;
  xpointer_t iface;

  for (i = 0; i < 1000; i++)
    {
      iface = xtype_interface_peek (klass, liststore_interfaces[0]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[1]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[2]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[3]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[4]);
      xassert (iface);
    }
}

static void
liststore_interface_peek_same_run (xpointer_t klass)
{
  xuint_t i;
  xpointer_t iface;

  for (i = 0; i < 1000; i++)
    {
      iface = xtype_interface_peek (klass, liststore_interfaces[0]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[0]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[0]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[0]);
      xassert (iface);
      iface = xtype_interface_peek (klass, liststore_interfaces[0]);
      xassert (iface);
    }
}

#if 0
/* DUMB test doing nothing */

static xpointer_t
no_setup (void)
{
  return NULL;
}

static void
no_run (xpointer_t data)
{
}
#endif

static void
no_reset (xpointer_t data)
{
}

static void
no_teardown (xpointer_t data)
{
}

typedef struct _PerformanceTest PerformanceTest;
struct _PerformanceTest {
  const char *name;

  xpointer_t (*setup) (void);
  void (*run) (xpointer_t data);
  void (*reset) (xpointer_t data);
  void (*teardown) (xpointer_t data);
};

static const PerformanceTest tests[] = {
  { "liststore-is-a",
    register_types,
    liststore_is_a_run,
    no_reset,
    no_teardown },
  { "liststore-interface-peek",
    liststore_get_class,
    liststore_interface_peek_run,
    no_reset,
    xtype_class_unref },
  { "liststore-interface-peek-same",
    liststore_get_class,
    liststore_interface_peek_same_run,
    no_reset,
    xtype_class_unref },
#if 0
  { "nothing",
    no_setup,
    no_run,
    no_reset,
    no_teardown }
#endif
};

static xboolean_t verbose = FALSE;
static xuint_t n_threads = 0;
static xboolean_t list = FALSE;
static int test_length = DEFAULT_TEST_TIME;

static GOptionEntry cmd_entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Print extra information", NULL},
  {"threads", 't', 0, G_OPTION_ARG_INT, &n_threads,
   "number of threads to run in parallel", NULL},
  {"seconds", 's', 0, G_OPTION_ARG_INT, &test_length,
   "Time to run each test in seconds", NULL},
  {"list", 'l', 0, G_OPTION_ARG_NONE, &list,
   "List all available tests and exit", NULL},
  G_OPTION_ENTRY_NULL
};

static xpointer_t
run_test_thread (xpointer_t user_data)
{
  const PerformanceTest *test = user_data;
  xpointer_t data;
  double elapsed;
  xtimer_t *timer, *total;
  xarray_t *results;

  total = g_timer_new ();
  g_timer_start (total);

  /* Set up test */
  timer = g_timer_new ();
  data = test->setup ();
  results = g_array_new (FALSE, FALSE, sizeof (double));

  /* Run the test */
  while (g_timer_elapsed (total, NULL) < test_length)
    {
      g_timer_reset (timer);
      g_timer_start (timer);
      test->run (data);
      g_timer_stop (timer);
      elapsed = g_timer_elapsed (timer, NULL);
      g_array_append_val (results, elapsed);
      test->reset (data);
    }

  /* Tear down */
  test->teardown (data);
  g_timer_destroy (timer);
  g_timer_destroy (total);

  return results;
}

static int
compare_doubles (xconstpointer a, xconstpointer b)
{
  double d = *(double *) a - *(double *) b;

  if (d < 0)
    return -1;
  if (d > 0)
    return 1;
  return 0;
}

static void
print_results (xarray_t *array)
{
  double min, max, avg;
  xuint_t i;

  g_array_sort (array, compare_doubles);

  /* FIXME: discard outliers */

  min = g_array_index (array, double, 0) * 1000;
  max = g_array_index (array, double, array->len - 1) * 1000;
  avg = 0;
  for (i = 0; i < array->len; i++)
    {
      avg += g_array_index (array, double, i);
    }
  avg = avg / array->len * 1000;

  g_print ("  %u runs, min/avg/max = %.3f/%.3f/%.3f ms\n", array->len, min, avg, max);
}

static void
run_test (const PerformanceTest *test)
{
  xarray_t *results;

  g_print ("Running test \"%s\"\n", test->name);

  if (n_threads == 0) {
    results = run_test_thread ((xpointer_t) test);
  } else {
    xuint_t i;
    xthread_t **threads;
    xarray_t *thread_results;

    threads = g_new (xthread_t *, n_threads);
    for (i = 0; i < n_threads; i++) {
      threads[i] = xthread_create (run_test_thread, (xpointer_t) test, TRUE, NULL);
      xassert (threads[i] != NULL);
    }

    results = g_array_new (FALSE, FALSE, sizeof (double));
    for (i = 0; i < n_threads; i++) {
      thread_results = xthread_join (threads[i]);
      g_array_append_vals (results, thread_results->data, thread_results->len);
      g_array_free (thread_results, TRUE);
    }
    g_free (threads);
  }

  print_results (results);
  g_array_free (results, TRUE);
}

static const PerformanceTest *
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
  const PerformanceTest *test;
  xoption_context_t *context;
  xerror_t *error = NULL;
  xsize_t i;

  context = g_option_context_new ("xobject_t performance tests");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (list)
    {
      for (i = 0; i < G_N_ELEMENTS (tests); i++)
        {
          g_print ("%s\n", tests[i].name);
        }
      return 0;
    }

  if (argc > 1)
    {
      int k;
      for (k = 1; k < argc; k++)
	{
	  test = find_test (argv[k]);
	  if (test)
	    run_test (test);
	}
    }
  else
    {
      for (i = 0; i < G_N_ELEMENTS (tests); i++)
	run_test (&tests[i]);
    }

  return 0;
}
