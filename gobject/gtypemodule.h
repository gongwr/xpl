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
#ifndef __XTYPE_MODULE_H__
#define __XTYPE_MODULE_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include <gobject/gobject.h>
#include <gobject/genums.h>

G_BEGIN_DECLS

typedef struct _xtype_module_t      xtype_module_t;
typedef struct _xtype_module_class xtype_module_class_t;

#define XTYPE_TYPE_MODULE              (xtype_module_get_type ())
#define XTYPE_MODULE(module)           (XTYPE_CHECK_INSTANCE_CAST ((module), XTYPE_TYPE_MODULE, xtype_module_t))
#define XTYPE_MODULE_CLASS(class)      (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_TYPE_MODULE, xtype_module_class_t))
#define X_IS_TYPE_MODULE(module)        (XTYPE_CHECK_INSTANCE_TYPE ((module), XTYPE_TYPE_MODULE))
#define X_IS_TYPE_MODULE_CLASS(class)   (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_TYPE_MODULE))
#define XTYPE_MODULE_GET_CLASS(module) (XTYPE_INSTANCE_GET_CLASS ((module), XTYPE_TYPE_MODULE, xtype_module_class_t))

G_DEFINE_AUTOPTR_CLEANUP_FUNC(xtype_module, xobject_unref)

/**
 * xtype_module_t:
 * @name: the name of the module
 *
 * The members of the xtype_module_t structure should not
 * be accessed directly, except for the @name field.
 */
struct _xtype_module_t
{
  xobject_t parent_instance;

  xuint_t use_count;
  xslist_t *type_infos;
  xslist_t *interface_infos;

  /*< public >*/
  xchar_t *name;
};

/**
 * xtype_module_class_t:
 * @parent_class: the parent class
 * @load: loads the module and registers one or more types using
 *  xtype_module_register_type().
 * @unload: unloads the module
 *
 * In order to implement dynamic loading of types based on #xtype_module_t,
 * the @load and @unload functions in #xtype_module_class_t must be implemented.
 */
struct _xtype_module_class
{
  xobject_class_t parent_class;

  /*< public >*/
  xboolean_t (* load)   (xtype_module_t *module);
  void     (* unload) (xtype_module_t *module);

  /*< private >*/
  /* Padding for future expansion */
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
};

/**
 * G_DEFINE_DYNAMIC_TYPE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *  separated by '_'.
 * @T_P: The #xtype_t of the parent type.
 *
 * A convenience macro for dynamic type implementations, which declares a
 * class initialization function, an instance initialization function (see
 * #xtype_info_t for information about these) and a static variable named
 * `t_n`_parent_class pointing to the parent class.
 *
 * Furthermore, it defines a `*_get_type()` and a static `*_register_type()`
 * functions for use in your `module_init()`.
 *
 * See G_DEFINE_DYNAMIC_TYPE_EXTENDED() for an example.
 *
 * Since: 2.14
 */
#define G_DEFINE_DYNAMIC_TYPE(TN, t_n, T_P)          G_DEFINE_DYNAMIC_TYPE_EXTENDED (TN, t_n, T_P, 0, {})
/**
 * G_DEFINE_DYNAMIC_TYPE_EXTENDED:
 * @TypeName: The name of the new type, in Camel case.
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by '_'.
 * @TYPE_PARENT: The #xtype_t of the parent type.
 * @flags: #xtype_flags_t to pass to xtype_module_register_type()
 * @CODE: Custom code that gets inserted in the *_get_type() function.
 *
 * A more general version of G_DEFINE_DYNAMIC_TYPE() which
 * allows to specify #xtype_flags_t and custom code.
 *
 * |[<!-- language="C" -->
 * G_DEFINE_DYNAMIC_TYPE_EXTENDED (GtkGadget,
 *                                 gtk_gadget,
 *                                 GTK_TYPE_THING,
 *                                 0,
 *                                 G_IMPLEMENT_INTERFACE_DYNAMIC (TYPE_GIZMO,
 *                                                                gtk_gadget_gizmo_init));
 * ]|
 *
 * expands to
 *
 * |[<!-- language="C" -->
 * static void     gtk_gadget_init              (GtkGadget      *self);
 * static void     gtk_gadget_class_init        (GtkGadgetClass *klass);
 * static void     gtk_gadget_class_finalize    (GtkGadgetClass *klass);
 *
 * static xpointer_t gtk_gadget_parent_class = NULL;
 * static xtype_t    gtk_gadget_type_id = 0;
 *
 * static void     gtk_gadget_class_intern_init (xpointer_t klass)
 * {
 *   gtk_gadget_parent_class = xtype_class_peek_parent (klass);
 *   gtk_gadget_class_init ((GtkGadgetClass*) klass);
 * }
 *
 * xtype_t
 * gtk_gadget_get_type (void)
 * {
 *   return gtk_gadget_type_id;
 * }
 *
 * static void
 * gtk_gadget_register_type (xtype_module_t *type_module)
 * {
 *   const xtype_info_t g_define_type_info = {
 *     sizeof (GtkGadgetClass),
 *     (xbase_init_func_t) NULL,
 *     (xbase_finalize_func_t) NULL,
 *     (xclass_init_func_t) gtk_gadget_class_intern_init,
 *     (xclass_finalize_func_t) gtk_gadget_class_finalize,
 *     NULL,   // class_data
 *     sizeof (GtkGadget),
 *     0,      // n_preallocs
 *     (xinstance_init_func_t) gtk_gadget_init,
 *     NULL    // value_table
 *   };
 *   gtk_gadget_type_id = xtype_module_register_type (type_module,
 *                                                     GTK_TYPE_THING,
 *                                                     "GtkGadget",
 *                                                     &g_define_type_info,
 *                                                     (xtype_flags_t) flags);
 *   {
 *     const xinterface_info_t g_implement_interface_info = {
 *       (GInterfaceInitFunc) gtk_gadget_gizmo_init
 *     };
 *     xtype_module_add_interface (type_module, g_define_type_id, TYPE_GIZMO, &g_implement_interface_info);
 *   }
 * }
 * ]|
 *
 * Since: 2.14
 */
#define G_DEFINE_DYNAMIC_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE) \
static void     type_name##_init              (TypeName##_t *self); \
static void     type_name##_class_init        (TypeName##_class_t *klass); \
static void     type_name##_class_finalize    (TypeName##_class_t *klass); \
static xpointer_t type_name##_parent_class = NULL; \
static xtype_t    type_name##_type_id = 0; \
static xint_t     TypeName##_private_offset; \
\
_G_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name) \
\
G_GNUC_UNUSED \
static inline xpointer_t \
type_name##_get_instance_private (TypeName##_t *self) \
{ \
  return (G_STRUCT_MEMBER_P (self, TypeName##_private_offset)); \
} \
\
xtype_t \
type_name##_get_type (void) \
{ \
  return type_name##_type_id; \
} \
static void \
type_name##_register_type (xtype_module_t *type_module) \
{ \
  xtype_t g_define_type_id G_GNUC_UNUSED; \
  const xtype_info_t g_define_type_info = { \
    sizeof (TypeName##_class_t), \
    (xbase_init_func_t) NULL, \
    (xbase_finalize_func_t) NULL, \
    (xclass_init_func_t)(void (*)(void)) type_name##_class_intern_init, \
    (xclass_finalize_func_t)(void (*)(void)) type_name##_class_finalize, \
    NULL,   /* class_data */ \
    sizeof (TypeName##_t), \
    0,      /* n_preallocs */ \
    (xinstance_init_func_t)(void (*)(void)) type_name##_init, \
    NULL    /* value_table */ \
  }; \
  type_name##_type_id = xtype_module_register_type (type_module, \
						     TYPE_PARENT, \
						     #TypeName, \
						     &g_define_type_info, \
						     (xtype_flags_t) flags); \
  g_define_type_id = type_name##_type_id; \
  { CODE ; } \
}

/**
 * G_IMPLEMENT_INTERFACE_DYNAMIC:
 * @TYPE_IFACE: The #xtype_t of the interface to add
 * @iface_init: The interface init function
 *
 * A convenience macro to ease interface addition in the @_C_ section
 * of G_DEFINE_DYNAMIC_TYPE_EXTENDED().
 *
 * See G_DEFINE_DYNAMIC_TYPE_EXTENDED() for an example.
 *
 * Note that this macro can only be used together with the
 * G_DEFINE_DYNAMIC_TYPE_EXTENDED macros, since it depends on variable
 * names from that macro.
 *
 * Since: 2.24
 */
#define G_IMPLEMENT_INTERFACE_DYNAMIC(TYPE_IFACE, iface_init)       { \
  const xinterface_info_t g_implement_interface_info = { \
    (GInterfaceInitFunc)(void (*)(void)) iface_init, NULL, NULL      \
  }; \
  xtype_module_add_interface (type_module, g_define_type_id, TYPE_IFACE, &g_implement_interface_info); \
}

/**
 * G_ADD_PRIVATE_DYNAMIC:
 * @TypeName: the name of the type in CamelCase
 *
 * A convenience macro to ease adding private data to instances of a new dynamic
 * type in the @_C_ section of G_DEFINE_DYNAMIC_TYPE_EXTENDED().
 *
 * See G_ADD_PRIVATE() for details, it is similar but for static types.
 *
 * Note that this macro can only be used together with the
 * G_DEFINE_DYNAMIC_TYPE_EXTENDED macros, since it depends on variable
 * names from that macro.
 *
 * Since: 2.38
 */
#define G_ADD_PRIVATE_DYNAMIC(TypeName)         { \
  TypeName##_private_offset = sizeof (TypeName##Private); \
}

XPL_AVAILABLE_IN_ALL
xtype_t    xtype_module_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xtype_module_use            (xtype_module_t          *module);
XPL_AVAILABLE_IN_ALL
void     xtype_module_unuse          (xtype_module_t          *module);
XPL_AVAILABLE_IN_ALL
void     xtype_module_set_name       (xtype_module_t          *module,
                                       const xchar_t          *name);
XPL_AVAILABLE_IN_ALL
xtype_t    xtype_module_register_type  (xtype_module_t          *module,
                                       xtype_t                 parent_type,
                                       const xchar_t          *type_name,
                                       const xtype_info_t      *type_info,
                                       xtype_flags_t            flags);
XPL_AVAILABLE_IN_ALL
void     xtype_module_add_interface  (xtype_module_t          *module,
                                       xtype_t                 instance_type,
                                       xtype_t                 interface_type,
                                       const xinterface_info_t *interface_info);
XPL_AVAILABLE_IN_ALL
xtype_t    xtype_module_register_enum  (xtype_module_t          *module,
                                       const xchar_t          *name,
                                       const xenum_value_t     *const_static_values);
XPL_AVAILABLE_IN_ALL
xtype_t    xtype_module_register_flags (xtype_module_t          *module,
                                       const xchar_t          *name,
                                       const xflags_value_t    *const_static_values);

G_END_DECLS

#endif /* __XTYPE_MODULE_H__ */
