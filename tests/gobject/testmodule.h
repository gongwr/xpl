/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * testmodule.h: Dummy dynamic type module
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

#ifndef __TEST_MODULE_H__
#define __TEST_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS

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

xtype_t        test_module_get_type      (void);
xtype_module_t *test_module_new           (test_module_register_func_t register_func);

G_END_DECLS

#endif /* __TEST_MODULE_H__ */
