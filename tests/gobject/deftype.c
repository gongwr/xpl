/* deftype.c
 * Copyright (C) 2006 Behdad Esfahbod
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
#include <glib-object.h>

/* see http://bugzilla.gnome.org/show_bug.cgi?id=337128 for the purpose of this test */

#define MY_G_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init)       { \
  const xinterface_info_t g_implement_interface_info = { \
      (GInterfaceInitFunc) iface_init, \
      NULL, \
      NULL \
    }; \
  xtype_add_interface_static (g_define_type_id, TYPE_IFACE, &g_implement_interface_info); \
}

#define MY_DEFINE_TYPE(TN, t_n, T_P) \
	G_DEFINE_TYPE_WITH_CODE (TN, t_n, T_P, \
				 MY_G_IMPLEMENT_INTERFACE (XTYPE_INTERFACE, NULL))

typedef struct _type_name {
  xobject_t parent_instance;
  const char *name;
} type_name_t;

typedef struct _type_name_class {
  xobject_class_t parent_parent;
} type_name_class_t;

xtype_t           type_name_get_type          (void);

MY_DEFINE_TYPE (type_name, type_name, XTYPE_OBJECT)

static void     type_name_init              (type_name_t      *self)
{
}

static void     type_name_class_init        (type_name_class_t *klass)
{
}

int
main (void)
{
  return 0;
}
