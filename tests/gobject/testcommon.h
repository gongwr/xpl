/* xobject_t - GLib Type, Object, Parameter and Signal Library
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

G_BEGIN_DECLS

#define DEFINE_TYPE_FULL(nameNoT, prefix,				\
		         class_init, base_init, instance_init,	\
		         parent_type, interface_decl)		\
xtype_t								\
prefix ## _get_type (void)					\
{								\
  static xtype_t object_type = 0;					\
								\
  if (!object_type)						\
    {								\
      const xtype_info_t object_info =			\
	{							\
	  sizeof (nameNoT ## _class_t),				\
	  (xbase_init_func_t) base_init,				\
	  (xbase_finalize_func_t) NULL,				\
	  (xclass_init_func_t) class_init,				\
	  (xclass_finalize_func_t) NULL,				\
	  NULL,           /* class_data */			\
	  sizeof (nameNoT ## _t),					\
	  0,             /* n_prelocs */			\
	  (xinstance_init_func_t) instance_init,                    \
          (const xtype_value_table_t *) NULL,			\
	};							\
								\
      object_type = xtype_register_static (parent_type,	\
					    # nameNoT,		\
					    &object_info, 0);	\
      interface_decl						\
    }								\
								\
  return object_type;						\
}

#define DEFINE_TYPE(nameNoT, prefix,				\
		    class_init, base_init, instance_init,	\
		    parent_type)				\
  DEFINE_TYPE_FULL(nameNoT, prefix, class_init, base_init,		\
		   instance_init, parent_type, {})

#define DEFINE_IFACE(nameNoT, prefix, base_init, dflt_init)	\
xtype_t								\
prefix ## _get_type (void)					\
{								\
  static xtype_t iface_type = 0;					\
								\
  if (!iface_type)						\
    {								\
      const xtype_info_t iface_info =			\
      {								\
	sizeof (nameNoT ## _class_t),					\
	(xbase_init_func_t)	base_init,				\
	(xbase_finalize_func_t) NULL,				\
	(xclass_init_func_t) dflt_init,				\
        (xclass_finalize_func_t) NULL,                              \
        (xconstpointer) NULL,                                   \
        (xuint16_t) 0,                                            \
        (xuint16_t) 0,                                            \
        (xinstance_init_func_t) NULL,                               \
        (const xtype_value_table_t*) NULL,                          \
      };							\
								\
      iface_type = xtype_register_static (XTYPE_INTERFACE,	\
					    # nameNoT,		\
					    &iface_info, 0);	\
    }								\
  return iface_type;						\
}

#define INTERFACE_FULL(type, init_func, iface_type)		\
{								\
  xinterface_info_t const iface =				\
    {								\
      (GInterfaceInitFunc) init_func, NULL, NULL		\
    };								\
								\
  xtype_add_interface_static (type, iface_type, &iface);	\
}
#define INTERFACE(init_func, iface_type)	\
  INTERFACE_FULL(object_type, init_func, iface_type)

G_END_DECLS

#endif /* __TEST_COMMON_H__ */
