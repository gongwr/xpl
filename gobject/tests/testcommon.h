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

#define DEFINE_TYPE_FULL(name, prefix,				\
		         class_init, base_init, instance_init,	\
		         parent_type, interface_decl)		\
xtype_t								\
prefix ## _get_type (void)					\
{								\
  static xtype_t object_type = 0;					\
								\
  if (!object_type)						\
    {								\
      static const GTypeInfo object_info =			\
	{							\
	  sizeof (name ## Class),				\
	  (GBaseInitFunc) base_init,				\
	  (GBaseFinalizeFunc) NULL,				\
	  (GClassInitFunc) class_init,				\
	  (GClassFinalizeFunc) NULL,				\
	  NULL,           /* class_data */			\
	  sizeof (name),					\
	  0,             /* n_prelocs */			\
	  (GInstanceInitFunc) instance_init,			\
	  (const GTypeValueTable *) NULL,			\
	};							\
								\
      object_type = g_type_register_static (parent_type,	\
					    # name,		\
					    &object_info, 0);	\
      interface_decl						\
    }								\
								\
  return object_type;						\
}

#define DEFINE_TYPE(name, prefix,				\
		    class_init, base_init, instance_init,	\
		    parent_type)				\
  DEFINE_TYPE_FULL(name, prefix, class_init, base_init,		\
		   instance_init, parent_type, {})

#define DEFINE_IFACE(name, prefix, base_init, dflt_init)	\
xtype_t								\
prefix ## _get_type (void)					\
{								\
  static xtype_t iface_type = 0;					\
								\
  if (!iface_type)						\
    {								\
      static const GTypeInfo iface_info =			\
      {								\
	sizeof (name ## Class),					\
	(GBaseInitFunc)	base_init,				\
	(GBaseFinalizeFunc) NULL,				\
	(GClassInitFunc) dflt_init,				\
	(GClassFinalizeFunc) NULL,				\
	(gconstpointer) NULL,					\
	(guint16) 0,						\
	(guint16) 0,						\
	(GInstanceInitFunc) NULL,				\
	(const GTypeValueTable*) NULL,				\
      };							\
								\
      iface_type = g_type_register_static (XTYPE_INTERFACE,	\
					    # name,		\
					    &iface_info, 0);	\
    }								\
  return iface_type;						\
}

#define INTERFACE_FULL(type, init_func, iface_type)		\
{								\
  static GInterfaceInfo const iface =				\
    {								\
      (GInterfaceInitFunc) init_func, NULL, NULL		\
    };								\
								\
  g_type_add_interface_static (type, iface_type, &iface);	\
}
#define INTERFACE(init_func, iface_type)	\
  INTERFACE_FULL(object_type, init_func, iface_type)

G_END_DECLS

#endif /* __TEST_COMMON_H__ */
