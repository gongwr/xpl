/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * testmodule.c: Dummy dynamic type module
 * Copyright (C) 2003 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "testmodule.h"
#include "testcommon.h"

static xboolean_t test_module_load   (xtype_module_t *module);
static void     test_module_unload (xtype_module_t *module);

static void
test_module_class_init (test_module_class_t *class)
{
  xtype_module_class_t *module_class = XTYPE_MODULE_CLASS (class);

  module_class->load = test_module_load;
  module_class->unload = test_module_unload;
}

DEFINE_TYPE (test_module, test_module,
	     test_module_class_init, NULL, NULL,
	     XTYPE_TYPE_MODULE)

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

xtype_module_t *
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
