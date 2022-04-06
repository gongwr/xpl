/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
#ifndef __G_OBJECT_H__
#define __G_OBJECT_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include        <gobject/gtype.h>
#include        <gobject/gvalue.h>
#include        <gobject/gparam.h>
#include        <gobject/gclosure.h>
#include        <gobject/gsignal.h>
#include        <gobject/gboxed.h>

G_BEGIN_DECLS

/* --- type macros --- */
/**
 * XTYPE_IS_OBJECT:
 * @type: Type id to check
 *
 * Check if the passed in type id is a %XTYPE_OBJECT or derived from it.
 *
 * Returns: %FALSE or %TRUE, indicating whether @type is a %XTYPE_OBJECT.
 */
#define XTYPE_IS_OBJECT(type)      (XTYPE_FUNDAMENTAL (type) == XTYPE_OBJECT)
/**
 * G_OBJECT:
 * @object: Object which is subject to casting.
 *
 * Casts a #xobject_t or derived pointer into a (xobject_t*) pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 */
#define G_OBJECT(object)            (XTYPE_CHECK_INSTANCE_CAST ((object), XTYPE_OBJECT, xobject_t))
/**
 * G_OBJECT_CLASS:
 * @class: a valid #xobject_class_t
 *
 * Casts a derived #xobject_class_t structure into a #xobject_class_t structure.
 */
#define G_OBJECT_CLASS(class)       (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_OBJECT, xobject_class_t))
/**
 * X_IS_OBJECT:
 * @object: Instance to check for being a %XTYPE_OBJECT.
 *
 * Checks whether a valid #GTypeInstance pointer is of type %XTYPE_OBJECT.
 */
#if XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_42
#define X_IS_OBJECT(object)         (XTYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE ((object), XTYPE_OBJECT))
#else
#define X_IS_OBJECT(object)         (XTYPE_CHECK_INSTANCE_TYPE ((object), XTYPE_OBJECT))
#endif
/**
 * X_IS_OBJECT_CLASS:
 * @class: a #xobject_class_t
 *
 * Checks whether @class "is a" valid #xobject_class_t structure of type
 * %XTYPE_OBJECT or derived.
 */
#define X_IS_OBJECT_CLASS(class)    (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_OBJECT))
/**
 * G_OBJECT_GET_CLASS:
 * @object: a #xobject_t instance.
 *
 * Get the class structure associated to a #xobject_t instance.
 *
 * Returns: pointer to object class structure.
 */
#define G_OBJECT_GET_CLASS(object)  (XTYPE_INSTANCE_GET_CLASS ((object), XTYPE_OBJECT, xobject_class_t))
/**
 * G_OBJECT_TYPE:
 * @object: Object to return the type id for.
 *
 * Get the type id of an object.
 *
 * Returns: Type id of @object.
 */
#define G_OBJECT_TYPE(object)       (XTYPE_FROM_INSTANCE (object))
/**
 * G_OBJECT_TYPE_NAME:
 * @object: Object to return the type name for.
 *
 * Get the name of an object's type.
 *
 * Returns: Type name of @object. The string is owned by the type system and
 *  should not be freed.
 */
#define G_OBJECT_TYPE_NAME(object)  (xtype_name (G_OBJECT_TYPE (object)))
/**
 * G_OBJECT_CLASS_TYPE:
 * @class: a valid #xobject_class_t
 *
 * Get the type id of a class structure.
 *
 * Returns: Type id of @class.
 */
#define G_OBJECT_CLASS_TYPE(class)  (XTYPE_FROM_CLASS (class))
/**
 * G_OBJECT_CLASS_NAME:
 * @class: a valid #xobject_class_t
 *
 * Return the name of a class structure's type.
 *
 * Returns: Type name of @class. The string is owned by the type system and
 *  should not be freed.
 */
#define G_OBJECT_CLASS_NAME(class)  (xtype_name (G_OBJECT_CLASS_TYPE (class)))
/**
 * G_VALUE_HOLDS_OBJECT:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values derived from type %XTYPE_OBJECT.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_OBJECT(value) (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_OBJECT))

/* --- type macros --- */
/**
 * XTYPE_INITIALLY_UNOWNED:
 *
 * The type for #xinitially_unowned_t.
 */
#define XTYPE_INITIALLY_UNOWNED	      (xinitially_unowned_get_type())
/**
 * G_INITIALLY_UNOWNED:
 * @object: Object which is subject to casting.
 *
 * Casts a #xinitially_unowned_t or derived pointer into a (xinitially_unowned_t*)
 * pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 */
#define G_INITIALLY_UNOWNED(object)           (XTYPE_CHECK_INSTANCE_CAST ((object), XTYPE_INITIALLY_UNOWNED, xinitially_unowned_t))
/**
 * G_INITIALLY_UNOWNED_CLASS:
 * @class: a valid #xinitially_unowned_class_t
 *
 * Casts a derived #xinitially_unowned_class_t structure into a
 * #xinitially_unowned_class_t structure.
 */
#define G_INITIALLY_UNOWNED_CLASS(class)      (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_INITIALLY_UNOWNED, xinitially_unowned_class_t))
/**
 * X_IS_INITIALLY_UNOWNED:
 * @object: Instance to check for being a %XTYPE_INITIALLY_UNOWNED.
 *
 * Checks whether a valid #GTypeInstance pointer is of type %XTYPE_INITIALLY_UNOWNED.
 */
#define X_IS_INITIALLY_UNOWNED(object)        (XTYPE_CHECK_INSTANCE_TYPE ((object), XTYPE_INITIALLY_UNOWNED))
/**
 * X_IS_INITIALLY_UNOWNED_CLASS:
 * @class: a #xinitially_unowned_class_t
 *
 * Checks whether @class "is a" valid #xinitially_unowned_class_t structure of type
 * %XTYPE_INITIALLY_UNOWNED or derived.
 */
#define X_IS_INITIALLY_UNOWNED_CLASS(class)   (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_INITIALLY_UNOWNED))
/**
 * G_INITIALLY_UNOWNED_GET_CLASS:
 * @object: a #xinitially_unowned_t instance.
 *
 * Get the class structure associated to a #xinitially_unowned_t instance.
 *
 * Returns: pointer to object class structure.
 */
#define G_INITIALLY_UNOWNED_GET_CLASS(object) (XTYPE_INSTANCE_GET_CLASS ((object), XTYPE_INITIALLY_UNOWNED, xinitially_unowned_class_t))
/* xinitially_unowned_t ia a xobject_t with initially floating reference count */


/* --- typedefs & structures --- */
typedef struct _GObject                  xobject_t;
typedef struct _GObjectClass             xobject_class_t;
typedef struct _GObject                  xinitially_unowned_t;
typedef struct _GObjectClass             xinitially_unowned_class_t;
typedef struct _GObjectConstructParam    GObjectConstructParam;
/**
 * GObjectGetPropertyFunc:
 * @object: a #xobject_t
 * @property_id: the numeric id under which the property was registered with
 *  xobject_class_install_property().
 * @value: a #xvalue_t to return the property value in
 * @pspec: the #xparam_spec_t describing the property
 *
 * The type of the @get_property function of #xobject_class_t.
 */
typedef void (*GObjectGetPropertyFunc)  (xobject_t      *object,
                                         xuint_t         property_id,
                                         xvalue_t       *value,
                                         xparam_spec_t   *pspec);
/**
 * GObjectSetPropertyFunc:
 * @object: a #xobject_t
 * @property_id: the numeric id under which the property was registered with
 *  xobject_class_install_property().
 * @value: the new value for the property
 * @pspec: the #xparam_spec_t describing the property
 *
 * The type of the @set_property function of #xobject_class_t.
 */
typedef void (*GObjectSetPropertyFunc)  (xobject_t      *object,
                                         xuint_t         property_id,
                                         const xvalue_t *value,
                                         xparam_spec_t   *pspec);
/**
 * GObjectFinalizeFunc:
 * @object: the #xobject_t being finalized
 *
 * The type of the @finalize function of #xobject_class_t.
 */
typedef void (*GObjectFinalizeFunc)     (xobject_t      *object);
/**
 * GWeakNotify:
 * @data: data that was provided when the weak reference was established
 * @where_the_object_was: the object being disposed
 *
 * A #GWeakNotify function can be added to an object as a callback that gets
 * triggered when the object is finalized.
 *
 * Since the object is already being disposed when the #GWeakNotify is called,
 * there's not much you could do with the object, apart from e.g. using its
 * address as hash-index or the like.
 *
 * In particular, this means it’s invalid to call xobject_ref(),
 * g_weak_ref_init(), g_weak_ref_set(), xobject_add_toggle_ref(),
 * xobject_weak_ref(), xobject_add_weak_pointer() or any function which calls
 * them on the object from this callback.
 */
typedef void (*GWeakNotify)		(xpointer_t      data,
					 xobject_t      *where_the_object_was);
/**
 * xobject_t:
 *
 * The base object type.
 *
 * All the fields in the `xobject_t` structure are private to the implementation
 * and should never be accessed directly.
 *
 * Since GLib 2.72, all #GObjects are guaranteed to be aligned to at least the
 * alignment of the largest basic GLib type (typically this is #xuint64_t or
 * #xdouble_t). If you need larger alignment for an element in a #xobject_t, you
 * should allocate it on the heap (aligned), or arrange for your #xobject_t to be
 * appropriately padded. This guarantee applies to the #xobject_t (or derived)
 * struct, the #xobject_class_t (or derived) struct, and any private data allocated
 * by G_ADD_PRIVATE().
 */
struct  _GObject
{
  GTypeInstance  xtype_instance;

  /*< private >*/
  xuint_t          ref_count;  /* (atomic) */
  GData         *qdata;
};
/**
 * xobject_class_t:
 * @xtype_class: the parent class
 * @constructor: the @constructor function is called by xobject_new () to
 *  complete the object initialization after all the construction properties are
 *  set. The first thing a @constructor implementation must do is chain up to the
 *  @constructor of the parent class. Overriding @constructor should be rarely
 *  needed, e.g. to handle construct properties, or to implement singletons.
 * @set_property: the generic setter for all properties of this type. Should be
 *  overridden for every type with properties. If implementations of
 *  @set_property don't emit property change notification explicitly, this will
 *  be done implicitly by the type system. However, if the notify signal is
 *  emitted explicitly, the type system will not emit it a second time.
 * @get_property: the generic getter for all properties of this type. Should be
 *  overridden for every type with properties.
 * @dispose: the @dispose function is supposed to drop all references to other
 *  objects, but keep the instance otherwise intact, so that client method
 *  invocations still work. It may be run multiple times (due to reference
 *  loops). Before returning, @dispose should chain up to the @dispose method
 *  of the parent class.
 * @finalize: instance finalization function, should finish the finalization of
 *  the instance begun in @dispose and chain up to the @finalize method of the
 *  parent class.
 * @dispatch_properties_changed: emits property change notification for a bunch
 *  of properties. Overriding @dispatch_properties_changed should be rarely
 *  needed.
 * @notify: the class closure for the notify signal
 * @constructed: the @constructed function is called by xobject_new() as the
 *  final step of the object creation process.  At the point of the call, all
 *  construction properties have been set on the object.  The purpose of this
 *  call is to allow for object initialisation steps that can only be performed
 *  after construction properties have been set.  @constructed implementors
 *  should chain up to the @constructed call of their parent class to allow it
 *  to complete its initialisation.
 *
 * The class structure for the xobject_t type.
 *
 * |[<!-- language="C" -->
 * // Example of implementing a singleton using a constructor.
 * static MySingleton *the_singleton = NULL;
 *
 * static xobject_t*
 * my_singleton_constructor (xtype_t                  type,
 *                           xuint_t                  n_construct_params,
 *                           GObjectConstructParam *construct_params)
 * {
 *   xobject_t *object;
 *
 *   if (!the_singleton)
 *     {
 *       object = G_OBJECT_CLASS (parent_class)->constructor (type,
 *                                                            n_construct_params,
 *                                                            construct_params);
 *       the_singleton = MY_SINGLETON (object);
 *     }
 *   else
 *     object = xobject_ref (G_OBJECT (the_singleton));
 *
 *   return object;
 * }
 * ]|
 */
struct  _GObjectClass
{
  xtype_class_t   xtype_class;

  /*< private >*/
  xslist_t      *construct_properties;

  /*< public >*/
  /* seldom overridden */
  xobject_t*   (*constructor)     (xtype_t                  type,
                                 xuint_t                  n_construct_properties,
                                 GObjectConstructParam *construct_properties);
  /* overridable methods */
  void       (*set_property)		(xobject_t        *object,
                                         xuint_t           property_id,
                                         const xvalue_t   *value,
                                         xparam_spec_t     *pspec);
  void       (*get_property)		(xobject_t        *object,
                                         xuint_t           property_id,
                                         xvalue_t         *value,
                                         xparam_spec_t     *pspec);
  void       (*dispose)			(xobject_t        *object);
  void       (*finalize)		(xobject_t        *object);
  /* seldom overridden */
  void       (*dispatch_properties_changed) (xobject_t      *object,
					     xuint_t	   n_pspecs,
					     xparam_spec_t  **pspecs);
  /* signals */
  void	     (*notify)			(xobject_t	*object,
					 xparam_spec_t	*pspec);

  /* called when done constructing */
  void	     (*constructed)		(xobject_t	*object);

  /*< private >*/
  xsize_t		flags;

  /* padding */
  xpointer_t	pdummy[6];
};

/**
 * GObjectConstructParam:
 * @pspec: the #xparam_spec_t of the construct parameter
 * @value: the value to set the parameter to
 *
 * The GObjectConstructParam struct is an auxiliary structure used to hand
 * #xparam_spec_t/#xvalue_t pairs to the @constructor of a #xobject_class_t.
 */
struct _GObjectConstructParam
{
  xparam_spec_t *pspec;
  xvalue_t     *value;
};

/**
 * xinitially_unowned_t:
 *
 * A type for objects that have an initially floating reference.
 *
 * All the fields in the `xinitially_unowned_t` structure are private to the
 * implementation and should never be accessed directly.
 */
/**
 * xinitially_unowned_class_t:
 *
 * The class structure for the xinitially_unowned_t type.
 */


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xtype_t       xinitially_unowned_get_type      (void);
XPL_AVAILABLE_IN_ALL
void        xobject_class_install_property   (xobject_class_t   *oclass,
					       xuint_t           property_id,
					       xparam_spec_t     *pspec);
XPL_AVAILABLE_IN_ALL
xparam_spec_t* xobject_class_find_property      (xobject_class_t   *oclass,
					       const xchar_t    *property_name);
XPL_AVAILABLE_IN_ALL
xparam_spec_t**xobject_class_list_properties    (xobject_class_t   *oclass,
					       xuint_t	      *n_properties);
XPL_AVAILABLE_IN_ALL
void        xobject_class_override_property  (xobject_class_t   *oclass,
					       xuint_t           property_id,
					       const xchar_t    *name);
XPL_AVAILABLE_IN_ALL
void        xobject_class_install_properties (xobject_class_t   *oclass,
                                               xuint_t           n_pspecs,
                                               xparam_spec_t    **pspecs);

XPL_AVAILABLE_IN_ALL
void        xobject_interface_install_property (xpointer_t     x_iface,
						 xparam_spec_t  *pspec);
XPL_AVAILABLE_IN_ALL
xparam_spec_t* xobject_interface_find_property    (xpointer_t     x_iface,
						 const xchar_t *property_name);
XPL_AVAILABLE_IN_ALL
xparam_spec_t**xobject_interface_list_properties  (xpointer_t     x_iface,
						 xuint_t       *n_properties_p);

XPL_AVAILABLE_IN_ALL
xtype_t       xobject_get_type                 (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_new                      (xtype_t           object_type,
					       const xchar_t    *first_property_name,
					       ...);
XPL_AVAILABLE_IN_2_54
xobject_t*    xobject_new_with_properties      (xtype_t           object_type,
                                               xuint_t           n_properties,
                                               const char     *names[],
                                               const xvalue_t    values[]);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

XPL_DEPRECATED_IN_2_54_FOR(xobject_new_with_properties)
xpointer_t    xobject_newv		      (xtype_t           object_type,
					       xuint_t	       n_parameters,
					       GParameter     *parameters);

G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
xobject_t*    xobject_new_valist               (xtype_t           object_type,
					       const xchar_t    *first_property_name,
					       va_list         var_args);
XPL_AVAILABLE_IN_ALL
void	    xobject_set                      (xpointer_t	       object,
					       const xchar_t    *first_property_name,
					       ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
void        xobject_get                      (xpointer_t        object,
					       const xchar_t    *first_property_name,
					       ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_connect                  (xpointer_t	       object,
					       const xchar_t    *signal_spec,
					       ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
void	    xobject_disconnect               (xpointer_t	       object,
					       const xchar_t    *signal_spec,
					       ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_2_54
void        xobject_setv                     (xobject_t        *object,
                                               xuint_t           n_properties,
                                               const xchar_t    *names[],
                                               const xvalue_t    values[]);
XPL_AVAILABLE_IN_ALL
void        xobject_set_valist               (xobject_t        *object,
					       const xchar_t    *first_property_name,
					       va_list         var_args);
XPL_AVAILABLE_IN_2_54
void        xobject_getv                     (xobject_t        *object,
                                               xuint_t           n_properties,
                                               const xchar_t    *names[],
                                               xvalue_t          values[]);
XPL_AVAILABLE_IN_ALL
void        xobject_get_valist               (xobject_t        *object,
					       const xchar_t    *first_property_name,
					       va_list         var_args);
XPL_AVAILABLE_IN_ALL
void        xobject_set_property             (xobject_t        *object,
					       const xchar_t    *property_name,
					       const xvalue_t   *value);
XPL_AVAILABLE_IN_ALL
void        xobject_get_property             (xobject_t        *object,
					       const xchar_t    *property_name,
					       xvalue_t         *value);
XPL_AVAILABLE_IN_ALL
void        xobject_freeze_notify            (xobject_t        *object);
XPL_AVAILABLE_IN_ALL
void        xobject_notify                   (xobject_t        *object,
					       const xchar_t    *property_name);
XPL_AVAILABLE_IN_ALL
void        xobject_notify_by_pspec          (xobject_t        *object,
					       xparam_spec_t     *pspec);
XPL_AVAILABLE_IN_ALL
void        xobject_thaw_notify              (xobject_t        *object);
XPL_AVAILABLE_IN_ALL
xboolean_t    xobject_is_floating    	      (xpointer_t        object);
XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_ref_sink       	      (xpointer_t	       object);
XPL_AVAILABLE_IN_2_70
xpointer_t    xobject_take_ref                 (xpointer_t        object);
XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_ref                      (xpointer_t        object);
XPL_AVAILABLE_IN_ALL
void        xobject_unref                    (xpointer_t        object);
XPL_AVAILABLE_IN_ALL
void	    xobject_weak_ref		      (xobject_t	      *object,
					       GWeakNotify     notify,
					       xpointer_t	       data);
XPL_AVAILABLE_IN_ALL
void	    xobject_weak_unref		      (xobject_t	      *object,
					       GWeakNotify     notify,
					       xpointer_t	       data);
XPL_AVAILABLE_IN_ALL
void        xobject_add_weak_pointer         (xobject_t        *object,
                                               xpointer_t       *weak_pointer_location);
XPL_AVAILABLE_IN_ALL
void        xobject_remove_weak_pointer      (xobject_t        *object,
                                               xpointer_t       *weak_pointer_location);

#if defined(glib_typeof) && XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_56
/* Make reference APIs type safe with macros */
#define xobject_ref(Obj) ((glib_typeof (Obj)) (xobject_ref) (Obj))
#define xobject_ref_sink(Obj) ((glib_typeof (Obj)) (xobject_ref_sink) (Obj))
#endif

/**
 * GToggleNotify:
 * @data: Callback data passed to xobject_add_toggle_ref()
 * @object: The object on which xobject_add_toggle_ref() was called.
 * @is_last_ref: %TRUE if the toggle reference is now the
 *  last reference to the object. %FALSE if the toggle
 *  reference was the last reference and there are now other
 *  references.
 *
 * A callback function used for notification when the state
 * of a toggle reference changes.
 *
 * See also: xobject_add_toggle_ref()
 */
typedef void (*GToggleNotify) (xpointer_t      data,
			       xobject_t      *object,
			       xboolean_t      is_last_ref);

XPL_AVAILABLE_IN_ALL
void xobject_add_toggle_ref    (xobject_t       *object,
				 GToggleNotify  notify,
				 xpointer_t       data);
XPL_AVAILABLE_IN_ALL
void xobject_remove_toggle_ref (xobject_t       *object,
				 GToggleNotify  notify,
				 xpointer_t       data);

XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_get_qdata                (xobject_t        *object,
					       xquark          quark);
XPL_AVAILABLE_IN_ALL
void        xobject_set_qdata                (xobject_t        *object,
					       xquark          quark,
					       xpointer_t        data);
XPL_AVAILABLE_IN_ALL
void        xobject_set_qdata_full           (xobject_t        *object,
					       xquark          quark,
					       xpointer_t        data,
					       xdestroy_notify_t  destroy);
XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_steal_qdata              (xobject_t        *object,
					       xquark          quark);

XPL_AVAILABLE_IN_2_34
xpointer_t    xobject_dup_qdata                (xobject_t        *object,
                                               xquark          quark,
                                               GDuplicateFunc  dup_func,
					       xpointer_t         user_data);
XPL_AVAILABLE_IN_2_34
xboolean_t    xobject_replace_qdata            (xobject_t        *object,
                                               xquark          quark,
                                               xpointer_t        oldval,
                                               xpointer_t        newval,
                                               xdestroy_notify_t  destroy,
					       xdestroy_notify_t *old_destroy);

XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_get_data                 (xobject_t        *object,
					       const xchar_t    *key);
XPL_AVAILABLE_IN_ALL
void        xobject_set_data                 (xobject_t        *object,
					       const xchar_t    *key,
					       xpointer_t        data);
XPL_AVAILABLE_IN_ALL
void        xobject_set_data_full            (xobject_t        *object,
					       const xchar_t    *key,
					       xpointer_t        data,
					       xdestroy_notify_t  destroy);
XPL_AVAILABLE_IN_ALL
xpointer_t    xobject_steal_data               (xobject_t        *object,
					       const xchar_t    *key);

XPL_AVAILABLE_IN_2_34
xpointer_t    xobject_dup_data                 (xobject_t        *object,
                                               const xchar_t    *key,
                                               GDuplicateFunc  dup_func,
					       xpointer_t         user_data);
XPL_AVAILABLE_IN_2_34
xboolean_t    xobject_replace_data             (xobject_t        *object,
                                               const xchar_t    *key,
                                               xpointer_t        oldval,
                                               xpointer_t        newval,
                                               xdestroy_notify_t  destroy,
					       xdestroy_notify_t *old_destroy);


XPL_AVAILABLE_IN_ALL
void        xobject_watch_closure            (xobject_t        *object,
					       xclosure_t       *closure);
XPL_AVAILABLE_IN_ALL
xclosure_t*   g_cclosure_new_object             (xcallback_t       callback_func,
					       xobject_t	      *object);
XPL_AVAILABLE_IN_ALL
xclosure_t*   g_cclosure_new_object_swap        (xcallback_t       callback_func,
					       xobject_t	      *object);
XPL_AVAILABLE_IN_ALL
xclosure_t*   xclosure_new_object              (xuint_t           sizeof_closure,
					       xobject_t        *object);
XPL_AVAILABLE_IN_ALL
void        xvalue_set_object                (xvalue_t         *value,
					       xpointer_t        v_object);
XPL_AVAILABLE_IN_ALL
xpointer_t    xvalue_get_object                (const xvalue_t   *value);
XPL_AVAILABLE_IN_ALL
xpointer_t    xvalue_dup_object                (const xvalue_t   *value);
XPL_AVAILABLE_IN_ALL
gulong	    g_signal_connect_object           (xpointer_t	       instance,
					       const xchar_t    *detailed_signal,
					       xcallback_t       c_handler,
					       xpointer_t	       gobject,
					       GConnectFlags   connect_flags);

/*< protected >*/
XPL_AVAILABLE_IN_ALL
void        xobject_force_floating           (xobject_t        *object);
XPL_AVAILABLE_IN_ALL
void        xobject_run_dispose	      (xobject_t	      *object);


XPL_AVAILABLE_IN_ALL
void        xvalue_take_object               (xvalue_t         *value,
					       xpointer_t        v_object);
XPL_DEPRECATED_FOR(xvalue_take_object)
void        xvalue_set_object_take_ownership (xvalue_t         *value,
                                               xpointer_t        v_object);

XPL_DEPRECATED
xsize_t	    xobject_compat_control	      (xsize_t	       what,
					       xpointer_t	       data);

/* --- implementation macros --- */
#define G_OBJECT_WARN_INVALID_PSPEC(object, pname, property_id, pspec) \
G_STMT_START { \
  xobject_t *_glib__object = (xobject_t*) (object); \
  xparam_spec_t *_glib__pspec = (xparam_spec_t*) (pspec); \
  xuint_t _glib__property_id = (property_id); \
  g_warning ("%s:%d: invalid %s id %u for \"%s\" of type '%s' in '%s'", \
             __FILE__, __LINE__, \
             (pname), \
             _glib__property_id, \
             _glib__pspec->name, \
             xtype_name (G_PARAM_SPEC_TYPE (_glib__pspec)), \
             G_OBJECT_TYPE_NAME (_glib__object)); \
} G_STMT_END
/**
 * G_OBJECT_WARN_INVALID_PROPERTY_ID:
 * @object: the #xobject_t on which set_property() or get_property() was called
 * @property_id: the numeric id of the property
 * @pspec: the #xparam_spec_t of the property
 *
 * This macro should be used to emit a standard warning about unexpected
 * properties in set_property() and get_property() implementations.
 */
#define G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec) \
    G_OBJECT_WARN_INVALID_PSPEC ((object), "property", (property_id), (pspec))

XPL_AVAILABLE_IN_ALL
void    g_clear_object (xobject_t **object_ptr);
#define g_clear_object(object_ptr) g_clear_pointer ((object_ptr), xobject_unref)

/**
 * g_set_object: (skip)
 * @object_ptr: (inout) (not optional) (nullable): a pointer to a #xobject_t reference
 * @new_object: (nullable) (transfer none): a pointer to the new #xobject_t to
 *   assign to @object_ptr, or %NULL to clear the pointer
 *
 * Updates a #xobject_t pointer to refer to @new_object.
 *
 * It increments the reference count of @new_object (if non-%NULL), decrements
 * the reference count of the current value of @object_ptr (if non-%NULL), and
 * assigns @new_object to @object_ptr. The assignment is not atomic.
 *
 * @object_ptr must not be %NULL, but can point to a %NULL value.
 *
 * A macro is also included that allows this function to be used without
 * pointer casts. The function itself is static inline, so its address may vary
 * between compilation units.
 *
 * One convenient usage of this function is in implementing property setters:
 * |[
 *   void
 *   foo_set_bar (foo_t *foo,
 *                Bar *new_bar)
 *   {
 *     g_return_if_fail (IS_FOO (foo));
 *     g_return_if_fail (new_bar == NULL || IS_BAR (new_bar));
 *
 *     if (g_set_object (&foo->bar, new_bar))
 *       xobject_notify (foo, "bar");
 *   }
 * ]|
 *
 * Returns: %TRUE if the value of @object_ptr changed, %FALSE otherwise
 *
 * Since: 2.44
 */
static inline xboolean_t
(g_set_object) (xobject_t **object_ptr,
                xobject_t  *new_object)
{
  xobject_t *old_object = *object_ptr;

  /* rely on xobject_[un]ref() to check the pointers are actually GObjects;
   * elide a (object_ptr != NULL) check because most of the time we will be
   * operating on struct members with a constant offset, so a NULL check would
   * not catch bugs
   */

  if (old_object == new_object)
    return FALSE;

  if (new_object != NULL)
    xobject_ref (new_object);

  *object_ptr = new_object;

  if (old_object != NULL)
    xobject_unref (old_object);

  return TRUE;
}

/* We need GCC for __extension__, which we need to sort out strict aliasing of @object_ptr */
#if defined(__GNUC__)

#define g_set_object(object_ptr, new_object) \
  (G_GNUC_EXTENSION ({ \
    G_STATIC_ASSERT (sizeof *(object_ptr) == sizeof (new_object)); \
    /* Only one access, please; work around type aliasing */ \
    union { char *in; xobject_t **out; } _object_ptr; \
    _object_ptr.in = (char *) (object_ptr); \
    /* Check types match */ \
    (void) (0 ? *(object_ptr) = (new_object), FALSE : FALSE); \
    (g_set_object) (_object_ptr.out, (xobject_t *) new_object); \
  })) \
  XPL_AVAILABLE_MACRO_IN_2_44

#else  /* if !defined(__GNUC__) */

#define g_set_object(object_ptr, new_object) \
 (/* Check types match. */ \
  0 ? *(object_ptr) = (new_object), FALSE : \
  (g_set_object) ((xobject_t **) (object_ptr), (xobject_t *) (new_object)) \
 )

#endif  /* !defined(__GNUC__) */

/**
 * g_assert_finalize_object: (skip)
 * @object: (transfer full) (type xobject_t.Object): an object
 *
 * Assert that @object is non-%NULL, then release one reference to it with
 * xobject_unref() and assert that it has been finalized (i.e. that there
 * are no more references).
 *
 * If assertions are disabled via `G_DISABLE_ASSERT`,
 * this macro just calls xobject_unref() without any further checks.
 *
 * This macro should only be used in regression tests.
 *
 * Since: 2.62
 */
static inline void
(g_assert_finalize_object) (xobject_t *object)
{
  xpointer_t weak_pointer = object;

  g_assert_true (X_IS_OBJECT (weak_pointer));
  xobject_add_weak_pointer (object, &weak_pointer);
  xobject_unref (weak_pointer);
  g_assert_null (weak_pointer);
}

#ifdef G_DISABLE_ASSERT
#define g_assert_finalize_object(object) xobject_unref (object)
#else
#define g_assert_finalize_object(object) (g_assert_finalize_object ((xobject_t *) object))
#endif

/**
 * g_clear_weak_pointer: (skip)
 * @weak_pointer_location: The memory address of a pointer
 *
 * Clears a weak reference to a #xobject_t.
 *
 * @weak_pointer_location must not be %NULL.
 *
 * If the weak reference is %NULL then this function does nothing.
 * Otherwise, the weak reference to the object is removed for that location
 * and the pointer is set to %NULL.
 *
 * A macro is also included that allows this function to be used without
 * pointer casts. The function itself is static inline, so its address may vary
 * between compilation units.
 *
 * Since: 2.56
 */
static inline void
(g_clear_weak_pointer) (xpointer_t *weak_pointer_location)
{
  xobject_t *object = (xobject_t *) *weak_pointer_location;

  if (object != NULL)
    {
      xobject_remove_weak_pointer (object, weak_pointer_location);
      *weak_pointer_location = NULL;
    }
}

#define g_clear_weak_pointer(weak_pointer_location) \
 (/* Check types match. */ \
  (g_clear_weak_pointer) ((xpointer_t *) (weak_pointer_location)) \
 )

/**
 * g_set_weak_pointer: (skip)
 * @weak_pointer_location: the memory address of a pointer
 * @new_object: (nullable) (transfer none): a pointer to the new #xobject_t to
 *   assign to it, or %NULL to clear the pointer
 *
 * Updates a pointer to weakly refer to @new_object.
 *
 * It assigns @new_object to @weak_pointer_location and ensures
 * that @weak_pointer_location will automatically be set to %NULL
 * if @new_object gets destroyed. The assignment is not atomic.
 * The weak reference is not thread-safe, see xobject_add_weak_pointer()
 * for details.
 *
 * The @weak_pointer_location argument must not be %NULL.
 *
 * A macro is also included that allows this function to be used without
 * pointer casts. The function itself is static inline, so its address may vary
 * between compilation units.
 *
 * One convenient usage of this function is in implementing property setters:
 * |[
 *   void
 *   foo_set_bar (foo_t *foo,
 *                Bar *new_bar)
 *   {
 *     g_return_if_fail (IS_FOO (foo));
 *     g_return_if_fail (new_bar == NULL || IS_BAR (new_bar));
 *
 *     if (g_set_weak_pointer (&foo->bar, new_bar))
 *       xobject_notify (foo, "bar");
 *   }
 * ]|
 *
 * Returns: %TRUE if the value of @weak_pointer_location changed, %FALSE otherwise
 *
 * Since: 2.56
 */
static inline xboolean_t
(g_set_weak_pointer) (xpointer_t *weak_pointer_location,
                      xobject_t  *new_object)
{
  xobject_t *old_object = (xobject_t *) *weak_pointer_location;

  /* elide a (weak_pointer_location != NULL) check because most of the time we
   * will be operating on struct members with a constant offset, so a NULL
   * check would not catch bugs
   */

  if (old_object == new_object)
    return FALSE;

  if (old_object != NULL)
    xobject_remove_weak_pointer (old_object, weak_pointer_location);

  *weak_pointer_location = new_object;

  if (new_object != NULL)
    xobject_add_weak_pointer (new_object, weak_pointer_location);

  return TRUE;
}

#define g_set_weak_pointer(weak_pointer_location, new_object) \
 (/* Check types match. */ \
  0 ? *(weak_pointer_location) = (new_object), FALSE : \
  (g_set_weak_pointer) ((xpointer_t *) (weak_pointer_location), (xobject_t *) (new_object)) \
 )

typedef struct {
    /*<private>*/
    union { xpointer_t p; } priv;
} GWeakRef;

XPL_AVAILABLE_IN_ALL
void     g_weak_ref_init       (GWeakRef *weak_ref,
                                xpointer_t  object);
XPL_AVAILABLE_IN_ALL
void     g_weak_ref_clear      (GWeakRef *weak_ref);
XPL_AVAILABLE_IN_ALL
xpointer_t g_weak_ref_get        (GWeakRef *weak_ref);
XPL_AVAILABLE_IN_ALL
void     g_weak_ref_set        (GWeakRef *weak_ref,
                                xpointer_t  object);

G_END_DECLS

#endif /* __G_OBJECT_H__ */
