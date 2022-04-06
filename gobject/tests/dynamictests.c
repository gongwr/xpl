/* GLib testing framework examples and tests
 * Copyright (C) 2008 Imendio AB
 * Authors: Tim Janik
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include <glib.h>
#include <glib-object.h>

/* This test tests the macros for defining dynamic types.
 */

static xmutex_t sync_mutex;
static xboolean_t loaded = FALSE;

/* MODULE */
typedef struct _test_module      test_module_t;
typedef struct _test_module_class test_module_class_t;

#define TEST_TYPE_MODULE              (test_module_get_type ())
#define TEST_MODULE(module)           (XTYPE_CHECK_INSTANCE_CAST ((module), TEST_TYPE_MODULE, test_module_t))
#define TEST_MODULE_CLASS(class)      (XTYPE_CHECK_CLASS_CAST ((class), TEST_TYPE_MODULE, test_module_class_t))
#define TEST_IS_MODULE(module)        (XTYPE_CHECK_INSTANCE_TYPE ((module), TEST_TYPE_MODULE))
#define TEST_IS_MODULE_CLASS(class)   (XTYPE_CHECK_CLASS_TYPE ((class), TEST_TYPE_MODULE))
#define TEST_MODULE_GET_CLASS(module) (XTYPE_INSTANCE_GET_CLASS ((module), TEST_TYPE_MODULE, test_module_class_t))
typedef void (*test_module_register_func_t) (xtype_module_t *module);

struct _test_module
{
  xtype_module_t parent_instance;

  test_module_register_func_t register_func;
};

struct _test_module_class
{
  xtype_module_class_t parent_class;
};

static xtype_t test_module_get_type (void);

static xboolean_t
test_module_load (xtype_module_t *module)
{
  test_module_t *test_module = TEST_MODULE (module);

  test_module->register_func (module);

  return TRUE;
}

static void
test_module_unload (xtype_module_t *module)
{
}

static void
test_module_class_init (test_module_class_t *class)
{
  xtype_module_class_t *module_class = XTYPE_MODULE_CLASS (class);

  module_class->load = test_module_load;
  module_class->unload = test_module_unload;
}

static xtype_t test_module_get_type (void)
{
  static xtype_t object_type = 0;

  if (!object_type) {
    static const xtype_info_t object_info =
      {
	sizeof (test_module_class_t),
	(xbase_init_func_t) NULL,
	(xbase_finalize_func_t) NULL,
	(xclass_init_func_t) test_module_class_init,
	(xclass_finalize_func_t) NULL,
	NULL,
	sizeof (test_module_t),
	0,
        (xinstance_init_func_t)NULL,
        NULL,
      };
    object_type = xtype_register_static (XTYPE_TYPE_MODULE, "test_module_t", &object_info, 0);
  }
  return object_type;
}


static xtype_module_t *
test_module_new (test_module_register_func_t register_func)
{
  test_module_t *test_module = xobject_new (TEST_TYPE_MODULE, NULL);
  xtype_module_t *module = XTYPE_MODULE (test_module);

  test_module->register_func = register_func;

  /* Register the types initially */
  xtype_module_use (module);
  xtype_module_unuse (module);

  return XTYPE_MODULE (module);
}



#define DYNAMIC_OBJECT_TYPE (dynamic_object_get_type ())

typedef xobject_t dynamic_object_t;
typedef struct _dynamic_object_class dynamic_object_class_t;

struct _dynamic_object_class
{
  xobject_class_t parent_class;
  xuint_t val;
};

static xtype_t dynamic_object_get_type (void);
G_DEFINE_DYNAMIC_TYPE(dynamic_object, dynamic_object, XTYPE_OBJECT)

static void
dynamic_object_class_init (dynamic_object_class_t *class)
{
  class->val = 42;
  g_assert (loaded == FALSE);
  loaded = TRUE;
}

static void
dynamic_object_class_finalize (dynamic_object_class_t *class)
{
  g_assert (loaded == TRUE);
  loaded = FALSE;
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

#define N_THREADS 100
#define N_REFS 10000

static xpointer_t
ref_unref_thread (xpointer_t data)
{
  xint_t i;
  /* first, synchronize with other threads,
   */
  if (g_test_verbose())
    g_printerr ("WAITING!\n");
  g_mutex_lock (&sync_mutex);
  g_mutex_unlock (&sync_mutex);
  if (g_test_verbose ())
    g_printerr ("STARTING\n");

  /* ref/unref the klass 10000000 times */
  for (i = N_REFS; i; i--) {
    if (g_test_verbose ())
      if (i % 10)
	g_printerr ("%d\n", i);
    xtype_class_unref (xtype_class_ref ((xtype_t) data));
  }

  if (g_test_verbose())
    g_printerr ("DONE !\n");

  return NULL;
}

static void
test_multithreaded_dynamic_type_init (void)
{
  xtype_module_t *module;
  dynamic_object_class_t *class;
  /* Create N_THREADS threads that are going to just ref/unref a class */
  xthread_t *threads[N_THREADS];
  xuint_t i;

  module = test_module_new (module_register);
  g_assert (module != NULL);

  /* Not loaded until we call ref for the first time */
  class = xtype_class_peek (DYNAMIC_OBJECT_TYPE);
  g_assert (class == NULL);
  g_assert (!loaded);

  /* pause newly created threads */
  g_mutex_lock (&sync_mutex);

  /* create threads */
  for (i = 0; i < N_THREADS; i++) {
    threads[i] = xthread_new ("test", ref_unref_thread, (xpointer_t) DYNAMIC_OBJECT_TYPE);
  }

  /* execute threads */
  g_mutex_unlock (&sync_mutex);

  for (i = 0; i < N_THREADS; i++) {
    xthread_join (threads[i]);
  }
}

enum
{
  PROP_0,
  PROP_FOO
};

typedef struct _dyn_obj dyn_obj_t;
typedef struct _dyn_obj_class dyn_obj_class_t;
typedef struct _DynIfaceInterface dyn_iface_interface_t;

struct _dyn_obj
{
  xobject_t obj;

  xint_t foo;
};

struct _dyn_obj_class
{
  xobject_class_t class;
};

struct _DynIfaceInterface
{
  xtype_interface_t iface;
};

static void dyn_obj_iface_init (dyn_iface_interface_t *iface);

static xtype_t dyn_iface_get_type (void);
G_DEFINE_INTERFACE (dyn_iface, dyn_iface, XTYPE_OBJECT)

static xtype_t dyn_obj_get_type (void);
G_DEFINE_DYNAMIC_TYPE_EXTENDED(dyn_obj, dyn_obj, XTYPE_OBJECT, 0,
                      G_IMPLEMENT_INTERFACE_DYNAMIC(dyn_iface_get_type (), dyn_obj_iface_init))


static void
dyn_iface_default_init (dyn_iface_interface_t *iface)
{
  xobject_interface_install_property (iface,
    g_param_spec_int ("foo", NULL, NULL, 0, 100, 0, G_PARAM_READWRITE));
}

static void
dyn_obj_iface_init (dyn_iface_interface_t *iface)
{
}

static void
dyn_obj_init (dyn_obj_t *obj)
{
  obj->foo = 0;
}

static void
set_prop (xobject_t      *object,
          xuint_t         prop_id,
          const xvalue_t *value,
          xparam_spec_t   *pspec)
{
  dyn_obj_t *obj = (dyn_obj_t *)object;

  switch (prop_id)
    {
    case PROP_FOO:
      obj->foo = xvalue_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
get_prop (xobject_t    *object,
          xuint_t       prop_id,
          xvalue_t     *value,
          xparam_spec_t *pspec)
{
  dyn_obj_t *obj = (dyn_obj_t *)object;

  switch (prop_id)
    {
    case PROP_FOO:
      xvalue_set_int (value, obj->foo);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
dyn_obj_class_init (dyn_obj_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = set_prop;
  object_class->get_property = get_prop;

  xobject_class_override_property (object_class, PROP_FOO, "foo");
}

static void
dyn_obj_class_finalize (dyn_obj_class_t *class)
{
}

static void
mod_register (xtype_module_t *module)
{
  dyn_obj_register_type (module);
}

static void
test_dynamic_interface_properties (void)
{
  xtype_module_t *module;
  dyn_obj_t *obj;
  xint_t val;

  module = test_module_new (mod_register);
  g_assert (module != NULL);

  obj = xobject_new (dyn_obj_get_type (), "foo", 1, NULL);
  xobject_get (obj, "foo", &val, NULL);
  g_assert_cmpint (val, ==, 1);

  xobject_unref (obj);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/xobject_t/threaded-dynamic-ref-unref-init", test_multithreaded_dynamic_type_init);
  g_test_add_func ("/xobject_t/dynamic-interface-properties", test_dynamic_interface_properties);

  return g_test_run();
}
