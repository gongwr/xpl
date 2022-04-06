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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __XTYPE_PLUGIN_H__
#define __XTYPE_PLUGIN_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gtype.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define XTYPE_TYPE_PLUGIN		(xtype_plugin_get_type ())
#define XTYPE_PLUGIN(inst)		(XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TYPE_PLUGIN, GTypePlugin))
#define XTYPE_PLUGIN_CLASS(vtable)	(XTYPE_CHECK_CLASS_CAST ((vtable), XTYPE_TYPE_PLUGIN, GTypePluginClass))
#define X_IS_TYPE_PLUGIN(inst)		(XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TYPE_PLUGIN))
#define X_IS_TYPE_PLUGIN_CLASS(vtable)	(XTYPE_CHECK_CLASS_TYPE ((vtable), XTYPE_TYPE_PLUGIN))
#define XTYPE_PLUGIN_GET_CLASS(inst)	(XTYPE_INSTANCE_GET_INTERFACE ((inst), XTYPE_TYPE_PLUGIN, GTypePluginClass))


/* --- typedefs & structures --- */
typedef struct _GTypePluginClass		   GTypePluginClass;
/**
 * GTypePluginUse:
 * @plugin: the #GTypePlugin whose use count should be increased
 *
 * The type of the @use_plugin function of #GTypePluginClass, which gets called
 * to increase the use count of @plugin.
 */
typedef void  (*GTypePluginUse)			  (GTypePlugin     *plugin);
/**
 * GTypePluginUnuse:
 * @plugin: the #GTypePlugin whose use count should be decreased
 *
 * The type of the @unuse_plugin function of #GTypePluginClass.
 */
typedef void  (*GTypePluginUnuse)		  (GTypePlugin     *plugin);
/**
 * GTypePluginCompleteTypeInfo:
 * @plugin: the #GTypePlugin
 * @g_type: the #xtype_t whose info is completed
 * @info: the #xtype_info_t struct to fill in
 * @value_table: the #xtype_value_table_t to fill in
 *
 * The type of the @complete_type_info function of #GTypePluginClass.
 */
typedef void  (*GTypePluginCompleteTypeInfo)	  (GTypePlugin     *plugin,
						   xtype_t            g_type,
						   xtype_info_t       *info,
						   xtype_value_table_t *value_table);
/**
 * GTypePluginCompleteInterfaceInfo:
 * @plugin: the #GTypePlugin
 * @instance_type: the #xtype_t of an instantiatable type to which the interface
 *  is added
 * @interface_type: the #xtype_t of the interface whose info is completed
 * @info: the #xinterface_info_t to fill in
 *
 * The type of the @complete_interface_info function of #GTypePluginClass.
 */
typedef void  (*GTypePluginCompleteInterfaceInfo) (GTypePlugin     *plugin,
						   xtype_t            instance_type,
						   xtype_t            interface_type,
						   xinterface_info_t  *info);
/**
 * GTypePlugin:
 *
 * The GTypePlugin typedef is used as a placeholder
 * for objects that implement the GTypePlugin interface.
 */
/**
 * GTypePluginClass:
 * @use_plugin: Increases the use count of the plugin.
 * @unuse_plugin: Decreases the use count of the plugin.
 * @complete_type_info: Fills in the #xtype_info_t and
 *  #xtype_value_table_t structs for the type. The structs are initialized
 *  with `memset(s, 0, sizeof (s))` before calling this function.
 * @complete_interface_info: Fills in missing parts of the #xinterface_info_t
 *  for the interface. The structs is initialized with
 *  `memset(s, 0, sizeof (s))` before calling this function.
 *
 * The #GTypePlugin interface is used by the type system in order to handle
 * the lifecycle of dynamically loaded types.
 */
struct _GTypePluginClass
{
  /*< private >*/
  xtype_interface_t		   base_iface;

  /*< public >*/
  GTypePluginUse		   use_plugin;
  GTypePluginUnuse		   unuse_plugin;
  GTypePluginCompleteTypeInfo	   complete_type_info;
  GTypePluginCompleteInterfaceInfo complete_interface_info;
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xtype_t	xtype_plugin_get_type			(void)	G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
void	xtype_plugin_use			(GTypePlugin	 *plugin);
XPL_AVAILABLE_IN_ALL
void	xtype_plugin_unuse			(GTypePlugin	 *plugin);
XPL_AVAILABLE_IN_ALL
void	xtype_plugin_complete_type_info	(GTypePlugin     *plugin,
						 xtype_t            g_type,
						 xtype_info_t       *info,
						 xtype_value_table_t *value_table);
XPL_AVAILABLE_IN_ALL
void	xtype_plugin_complete_interface_info	(GTypePlugin     *plugin,
						 xtype_t            instance_type,
						 xtype_t            interface_type,
						 xinterface_info_t  *info);

G_END_DECLS

#endif /* __XTYPE_PLUGIN_H__ */
