/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"

#include <stdlib.h>

#include "gtypeplugin.h"
#include "gtypemodule.h"


/**
 * SECTION:gtypemodule
 * @short_description: Type loading modules
 * @see_also: #GTypePlugin, #GModule
 * @title: xtype_module_t
 *
 * #xtype_module_t provides a simple implementation of the #GTypePlugin
 * interface.
 *
 * The model of #xtype_module_t is a dynamically loaded module which
 * implements some number of types and interface implementations.
 *
 * When the module is loaded, it registers its types and interfaces
 * using xtype_module_register_type() and xtype_module_add_interface().
 * As long as any instances of these types and interface implementations
 * are in use, the module is kept loaded. When the types and interfaces
 * are gone, the module may be unloaded. If the types and interfaces
 * become used again, the module will be reloaded. Note that the last
 * reference cannot be released from within the module code, since that
 * would lead to the caller's code being unloaded before xobject_unref()
 * returns to it.
 *
 * Keeping track of whether the module should be loaded or not is done by
 * using a use count - it starts at zero, and whenever it is greater than
 * zero, the module is loaded. The use count is maintained internally by
 * the type system, but also can be explicitly controlled by
 * xtype_module_use() and xtype_module_unuse(). Typically, when loading
 * a module for the first type, xtype_module_use() will be used to load
 * it so that it can initialize its types. At some later point, when the
 * module no longer needs to be loaded except for the type
 * implementations it contains, xtype_module_unuse() is called.
 *
 * #xtype_module_t does not actually provide any implementation of module
 * loading and unloading. To create a particular module type you must
 * derive from #xtype_module_t and implement the load and unload functions
 * in #xtype_module_class_t.
 */

typedef struct _ModuleTypeInfo ModuleTypeInfo;
typedef struct _ModuleInterfaceInfo ModuleInterfaceInfo;

struct _ModuleTypeInfo
{
  xboolean_t  loaded;
  xtype_t     type;
  xtype_t     parent_type;
  xtype_info_t info;
};

struct _ModuleInterfaceInfo
{
  xboolean_t       loaded;
  xtype_t          instance_type;
  xtype_t          interface_type;
  xinterface_info_t info;
};

static void xtype_module_use_plugin              (GTypePlugin     *plugin);
static void xtype_module_complete_type_info      (GTypePlugin     *plugin,
						   xtype_t            g_type,
						   xtype_info_t       *info,
						   xtype_value_table_t *value_table);
static void xtype_module_complete_interface_info (GTypePlugin     *plugin,
						   xtype_t            instance_type,
						   xtype_t            interface_type,
						   xinterface_info_t  *info);

static xpointer_t parent_class = NULL;

static void
xtype_module_dispose (xobject_t *object)
{
  xtype_module_t *module = XTYPE_MODULE (object);

  if (module->type_infos || module->interface_infos)
    {
      g_warning (G_STRLOC ": unsolicitated invocation of xobject_run_dispose() on xtype_module_t");

      xobject_ref (object);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
xtype_module_finalize (xobject_t *object)
{
  xtype_module_t *module = XTYPE_MODULE (object);

  g_free (module->name);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xtype_module_class_init (xtype_module_class_t *class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (class);

  parent_class = G_OBJECT_CLASS (xtype_class_peek_parent (class));

  gobject_class->dispose = xtype_module_dispose;
  gobject_class->finalize = xtype_module_finalize;
}

static void
xtype_module_iface_init (GTypePluginClass *iface)
{
  iface->use_plugin = xtype_module_use_plugin;
  iface->unuse_plugin = (void (*) (GTypePlugin *))xtype_module_unuse;
  iface->complete_type_info = xtype_module_complete_type_info;
  iface->complete_interface_info = xtype_module_complete_interface_info;
}

xtype_t
xtype_module_get_type (void)
{
  static xtype_t type_module_type = 0;

  if (!type_module_type)
    {
      const xtype_info_t type_module_info = {
        sizeof (xtype_module_class_t),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (xclass_init_func_t) xtype_module_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (xtype_module_t),
        0,              /* n_preallocs */
        NULL,           /* instance_init */
        NULL,           /* value_table */
      };
      const xinterface_info_t iface_info = {
        (GInterfaceInitFunc) xtype_module_iface_init,
        NULL,               /* interface_finalize */
        NULL,               /* interface_data */
      };

      type_module_type = xtype_register_static (XTYPE_OBJECT, g_intern_static_string ("xtype_module_t"), &type_module_info, XTYPE_FLAG_ABSTRACT);

      xtype_add_interface_static (type_module_type, XTYPE_TYPE_PLUGIN, &iface_info);
    }

  return type_module_type;
}

/**
 * xtype_module_set_name:
 * @module: a #xtype_module_t.
 * @name: a human-readable name to use in error messages.
 *
 * Sets the name for a #xtype_module_t
 */
void
xtype_module_set_name (xtype_module_t  *module,
			const xchar_t  *name)
{
  g_return_if_fail (X_IS_TYPE_MODULE (module));

  g_free (module->name);
  module->name = xstrdup (name);
}

static ModuleTypeInfo *
xtype_module_find_type_info (xtype_module_t *module,
			      xtype_t        type)
{
  xslist_t *tmp_list = module->type_infos;
  while (tmp_list)
    {
      ModuleTypeInfo *type_info = tmp_list->data;
      if (type_info->type == type)
	return type_info;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

static ModuleInterfaceInfo *
xtype_module_find_interface_info (xtype_module_t *module,
				   xtype_t        instance_type,
				   xtype_t        interface_type)
{
  xslist_t *tmp_list = module->interface_infos;
  while (tmp_list)
    {
      ModuleInterfaceInfo *interface_info = tmp_list->data;
      if (interface_info->instance_type == instance_type &&
	  interface_info->interface_type == interface_type)
	return interface_info;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * xtype_module_use:
 * @module: a #xtype_module_t
 *
 * Increases the use count of a #xtype_module_t by one. If the
 * use count was zero before, the plugin will be loaded.
 * If loading the plugin fails, the use count is reset to
 * its prior value.
 *
 * Returns: %FALSE if the plugin needed to be loaded and
 *  loading the plugin failed.
 */
xboolean_t
xtype_module_use (xtype_module_t *module)
{
  g_return_val_if_fail (X_IS_TYPE_MODULE (module), FALSE);

  module->use_count++;
  if (module->use_count == 1)
    {
      xslist_t *tmp_list;

      if (!XTYPE_MODULE_GET_CLASS (module)->load (module))
	{
	  module->use_count--;
	  return FALSE;
	}

      tmp_list = module->type_infos;
      while (tmp_list)
	{
	  ModuleTypeInfo *type_info = tmp_list->data;
	  if (!type_info->loaded)
	    {
	      g_warning ("plugin '%s' failed to register type '%s'",
			 module->name ? module->name : "(unknown)",
			 xtype_name (type_info->type));
	      module->use_count--;
	      return FALSE;
	    }

	  tmp_list = tmp_list->next;
	}
    }

  return TRUE;
}

/**
 * xtype_module_unuse:
 * @module: a #xtype_module_t
 *
 * Decreases the use count of a #xtype_module_t by one. If the
 * result is zero, the module will be unloaded. (However, the
 * #xtype_module_t will not be freed, and types associated with the
 * #xtype_module_t are not unregistered. Once a #xtype_module_t is
 * initialized, it must exist forever.)
 */
void
xtype_module_unuse (xtype_module_t *module)
{
  g_return_if_fail (X_IS_TYPE_MODULE (module));
  g_return_if_fail (module->use_count > 0);

  module->use_count--;

  if (module->use_count == 0)
    {
      xslist_t *tmp_list;

      XTYPE_MODULE_GET_CLASS (module)->unload (module);

      tmp_list = module->type_infos;
      while (tmp_list)
	{
	  ModuleTypeInfo *type_info = tmp_list->data;
	  type_info->loaded = FALSE;

	  tmp_list = tmp_list->next;
	}
    }
}

static void
xtype_module_use_plugin (GTypePlugin *plugin)
{
  xtype_module_t *module = XTYPE_MODULE (plugin);

  if (!xtype_module_use (module))
    {
      g_warning ("Fatal error - Could not reload previously loaded plugin '%s'",
		 module->name ? module->name : "(unknown)");
      exit (1);
    }
}

static void
xtype_module_complete_type_info (GTypePlugin     *plugin,
				  xtype_t            g_type,
				  xtype_info_t       *info,
				  xtype_value_table_t *value_table)
{
  xtype_module_t *module = XTYPE_MODULE (plugin);
  ModuleTypeInfo *module_type_info = xtype_module_find_type_info (module, g_type);

  *info = module_type_info->info;

  if (module_type_info->info.value_table)
    *value_table = *module_type_info->info.value_table;
}

static void
xtype_module_complete_interface_info (GTypePlugin    *plugin,
				       xtype_t           instance_type,
				       xtype_t           interface_type,
				       xinterface_info_t *info)
{
  xtype_module_t *module = XTYPE_MODULE (plugin);
  ModuleInterfaceInfo *module_interface_info = xtype_module_find_interface_info (module, instance_type, interface_type);

  *info = module_interface_info->info;
}

/**
 * xtype_module_register_type:
 * @module: (nullable): a #xtype_module_t
 * @parent_type: the type for the parent class
 * @type_name: name for the type
 * @type_info: type information structure
 * @flags: flags field providing details about the type
 *
 * Looks up or registers a type that is implemented with a particular
 * type plugin. If a type with name @type_name was previously registered,
 * the #xtype_t identifier for the type is returned, otherwise the type
 * is newly registered, and the resulting #xtype_t identifier returned.
 *
 * When reregistering a type (typically because a module is unloaded
 * then reloaded, and reinitialized), @module and @parent_type must
 * be the same as they were previously.
 *
 * As long as any instances of the type exist, the type plugin will
 * not be unloaded.
 *
 * Since 2.56 if @module is %NULL this will call xtype_register_static()
 * instead. This can be used when making a static build of the module.
 *
 * Returns: the new or existing type ID
 */
xtype_t
xtype_module_register_type (xtype_module_t     *module,
			     xtype_t            parent_type,
			     const xchar_t     *type_name,
			     const xtype_info_t *type_info,
			     xtype_flags_t       flags)
{
  ModuleTypeInfo *module_type_info = NULL;
  xtype_t type;

  g_return_val_if_fail (type_name != NULL, 0);
  g_return_val_if_fail (type_info != NULL, 0);

  if (module == NULL)
    {
      /* Cannot pass type_info directly to xtype_register_static() here because
       * it has class_finalize != NULL and that's forbidden for static types */
      return xtype_register_static_simple (parent_type,
                                            type_name,
                                            type_info->class_size,
                                            type_info->class_init,
                                            type_info->instance_size,
                                            type_info->instance_init,
                                            flags);
    }

  type = xtype_from_name (type_name);
  if (type)
    {
      GTypePlugin *old_plugin = xtype_get_plugin (type);

      if (old_plugin != XTYPE_PLUGIN (module))
	{
	  g_warning ("Two different plugins tried to register '%s'.", type_name);
	  return 0;
	}
    }

  if (type)
    {
      module_type_info = xtype_module_find_type_info (module, type);

      if (module_type_info->parent_type != parent_type)
	{
	  const xchar_t *parent_type_name = xtype_name (parent_type);

	  g_warning ("Type '%s' recreated with different parent type."
		     "(was '%s', now '%s')", type_name,
		     xtype_name (module_type_info->parent_type),
		     parent_type_name ? parent_type_name : "(unknown)");
	  return 0;
	}

      if (module_type_info->info.value_table)
	g_free ((xtype_value_table_t *) module_type_info->info.value_table);
    }
  else
    {
      module_type_info = g_new (ModuleTypeInfo, 1);

      module_type_info->parent_type = parent_type;
      module_type_info->type = xtype_register_dynamic (parent_type, type_name, XTYPE_PLUGIN (module), flags);

      module->type_infos = xslist_prepend (module->type_infos, module_type_info);
    }

  module_type_info->loaded = TRUE;
  module_type_info->info = *type_info;
  if (type_info->value_table)
    module_type_info->info.value_table = g_memdup2 (type_info->value_table,
						   sizeof (xtype_value_table_t));

  return module_type_info->type;
}

/**
 * xtype_module_add_interface:
 * @module: (nullable): a #xtype_module_t
 * @instance_type: type to which to add the interface.
 * @interface_type: interface type to add
 * @interface_info: type information structure
 *
 * Registers an additional interface for a type, whose interface lives
 * in the given type plugin. If the interface was already registered
 * for the type in this plugin, nothing will be done.
 *
 * As long as any instances of the type exist, the type plugin will
 * not be unloaded.
 *
 * Since 2.56 if @module is %NULL this will call xtype_add_interface_static()
 * instead. This can be used when making a static build of the module.
 */
void
xtype_module_add_interface (xtype_module_t          *module,
			     xtype_t                 instance_type,
			     xtype_t                 interface_type,
			     const xinterface_info_t *interface_info)
{
  ModuleInterfaceInfo *module_interface_info = NULL;

  g_return_if_fail (interface_info != NULL);

  if (module == NULL)
    {
      xtype_add_interface_static (instance_type, interface_type, interface_info);
      return;
    }

  if (xtype_is_a (instance_type, interface_type))
    {
      GTypePlugin *old_plugin = xtype_interface_get_plugin (instance_type,
							     interface_type);

      if (!old_plugin)
	{
	  g_warning ("Interface '%s' for '%s' was previously registered statically or for a parent type.",
		     xtype_name (interface_type), xtype_name (instance_type));
	  return;
	}
      else if (old_plugin != XTYPE_PLUGIN (module))
	{
	  g_warning ("Two different plugins tried to register interface '%s' for '%s'.",
		     xtype_name (interface_type), xtype_name (instance_type));
	  return;
	}

      module_interface_info = xtype_module_find_interface_info (module, instance_type, interface_type);

      g_assert (module_interface_info);
    }
  else
    {
      module_interface_info = g_new (ModuleInterfaceInfo, 1);

      module_interface_info->instance_type = instance_type;
      module_interface_info->interface_type = interface_type;

      xtype_add_interface_dynamic (instance_type, interface_type, XTYPE_PLUGIN (module));

      module->interface_infos = xslist_prepend (module->interface_infos, module_interface_info);
    }

  module_interface_info->loaded = TRUE;
  module_interface_info->info = *interface_info;
}

/**
 * xtype_module_register_enum:
 * @module: (nullable): a #xtype_module_t
 * @name: name for the type
 * @const_static_values: an array of #xenum_value_t structs for the
 *                       possible enumeration values. The array is
 *                       terminated by a struct with all members being
 *                       0.
 *
 * Looks up or registers an enumeration that is implemented with a particular
 * type plugin. If a type with name @type_name was previously registered,
 * the #xtype_t identifier for the type is returned, otherwise the type
 * is newly registered, and the resulting #xtype_t identifier returned.
 *
 * As long as any instances of the type exist, the type plugin will
 * not be unloaded.
 *
 * Since 2.56 if @module is %NULL this will call xtype_register_static()
 * instead. This can be used when making a static build of the module.
 *
 * Since: 2.6
 *
 * Returns: the new or existing type ID
 */
xtype_t
xtype_module_register_enum (xtype_module_t      *module,
                             const xchar_t      *name,
                             const xenum_value_t *const_static_values)
{
  xtype_info_t enum_type_info = { 0, };

  g_return_val_if_fail (module == NULL || X_IS_TYPE_MODULE (module), 0);
  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (const_static_values != NULL, 0);

  xenum_complete_type_info (XTYPE_ENUM,
                             &enum_type_info, const_static_values);

  return xtype_module_register_type (XTYPE_MODULE (module),
                                      XTYPE_ENUM, name, &enum_type_info, 0);
}

/**
 * xtype_module_register_flags:
 * @module: (nullable): a #xtype_module_t
 * @name: name for the type
 * @const_static_values: an array of #xflags_value_t structs for the
 *                       possible flags values. The array is
 *                       terminated by a struct with all members being
 *                       0.
 *
 * Looks up or registers a flags type that is implemented with a particular
 * type plugin. If a type with name @type_name was previously registered,
 * the #xtype_t identifier for the type is returned, otherwise the type
 * is newly registered, and the resulting #xtype_t identifier returned.
 *
 * As long as any instances of the type exist, the type plugin will
 * not be unloaded.
 *
 * Since 2.56 if @module is %NULL this will call xtype_register_static()
 * instead. This can be used when making a static build of the module.
 *
 * Since: 2.6
 *
 * Returns: the new or existing type ID
 */
xtype_t
xtype_module_register_flags (xtype_module_t      *module,
                             const xchar_t       *name,
                             const xflags_value_t *const_static_values)
{
  xtype_info_t flags_type_info = { 0, };

  g_return_val_if_fail (module == NULL || X_IS_TYPE_MODULE (module), 0);
  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (const_static_values != NULL, 0);

  xflags_complete_type_info (XTYPE_FLAGS,
                             &flags_type_info, const_static_values);

  return xtype_module_register_type (XTYPE_MODULE (module),
                                      XTYPE_FLAGS, name, &flags_type_info, 0);
}
