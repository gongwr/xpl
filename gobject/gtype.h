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
#ifndef __XTYPE_H__
#define __XTYPE_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include        <glib.h>

G_BEGIN_DECLS

/* Basic Type Macros
 */
/**
 * XTYPE_FUNDAMENTAL:
 * @type: A #xtype_t value.
 *
 * The fundamental type which is the ancestor of @type.
 *
 * Fundamental types are types that serve as ultimate bases for the derived types,
 * thus they are the roots of distinct inheritance hierarchies.
 */
#define XTYPE_FUNDAMENTAL(type)	(xtype_fundamental (type))
/**
 * XTYPE_FUNDAMENTAL_MAX:
 *
 * An integer constant that represents the number of identifiers reserved
 * for types that are assigned at compile-time.
 */
#define	XTYPE_FUNDAMENTAL_MAX		(255 << XTYPE_FUNDAMENTAL_SHIFT)

/* Constant fundamental types,
 */
/**
 * XTYPE_INVALID:
 *
 * An invalid #xtype_t used as error return value in some functions which return
 * a #xtype_t.
 */
#define XTYPE_INVALID			XTYPE_MAKE_FUNDAMENTAL (0)
/**
 * XTYPE_NONE:
 *
 * A fundamental type which is used as a replacement for the C
 * void return type.
 */
#define XTYPE_NONE			XTYPE_MAKE_FUNDAMENTAL (1)
/**
 * XTYPE_INTERFACE:
 *
 * The fundamental type from which all interfaces are derived.
 */
#define XTYPE_INTERFACE		XTYPE_MAKE_FUNDAMENTAL (2)
/**
 * XTYPE_CHAR:
 *
 * The fundamental type corresponding to #xchar_t.
 *
 * The type designated by %XTYPE_CHAR is unconditionally an 8-bit signed integer.
 * This may or may not be the same type a the C type "xchar_t".
 */
#define XTYPE_CHAR			XTYPE_MAKE_FUNDAMENTAL (3)
/**
 * XTYPE_UCHAR:
 *
 * The fundamental type corresponding to #xuchar_t.
 */
#define XTYPE_UCHAR			XTYPE_MAKE_FUNDAMENTAL (4)
/**
 * XTYPE_BOOLEAN:
 *
 * The fundamental type corresponding to #xboolean_t.
 */
#define XTYPE_BOOLEAN			XTYPE_MAKE_FUNDAMENTAL (5)
/**
 * XTYPE_INT:
 *
 * The fundamental type corresponding to #xint_t.
 */
#define XTYPE_INT			XTYPE_MAKE_FUNDAMENTAL (6)
/**
 * XTYPE_UINT:
 *
 * The fundamental type corresponding to #xuint_t.
 */
#define XTYPE_UINT			XTYPE_MAKE_FUNDAMENTAL (7)
/**
 * XTYPE_LONG:
 *
 * The fundamental type corresponding to #xlong_t.
 */
#define XTYPE_LONG			XTYPE_MAKE_FUNDAMENTAL (8)
/**
 * XTYPE_ULONG:
 *
 * The fundamental type corresponding to #xulong_t.
 */
#define XTYPE_ULONG			XTYPE_MAKE_FUNDAMENTAL (9)
/**
 * XTYPE_INT64:
 *
 * The fundamental type corresponding to #sint64_t.
 */
#define XTYPE_INT64			XTYPE_MAKE_FUNDAMENTAL (10)
/**
 * XTYPE_UINT64:
 *
 * The fundamental type corresponding to #xuint64_t.
 */
#define XTYPE_UINT64			XTYPE_MAKE_FUNDAMENTAL (11)
/**
 * XTYPE_ENUM:
 *
 * The fundamental type from which all enumeration types are derived.
 */
#define XTYPE_ENUM			XTYPE_MAKE_FUNDAMENTAL (12)
/**
 * XTYPE_FLAGS:
 *
 * The fundamental type from which all flags types are derived.
 */
#define XTYPE_FLAGS			XTYPE_MAKE_FUNDAMENTAL (13)
/**
 * XTYPE_FLOAT:
 *
 * The fundamental type corresponding to #gfloat.
 */
#define XTYPE_FLOAT			XTYPE_MAKE_FUNDAMENTAL (14)
/**
 * XTYPE_DOUBLE:
 *
 * The fundamental type corresponding to #xdouble_t.
 */
#define XTYPE_DOUBLE			XTYPE_MAKE_FUNDAMENTAL (15)
/**
 * XTYPE_STRING:
 *
 * The fundamental type corresponding to nul-terminated C strings.
 */
#define XTYPE_STRING			XTYPE_MAKE_FUNDAMENTAL (16)
/**
 * XTYPE_POINTER:
 *
 * The fundamental type corresponding to #xpointer_t.
 */
#define XTYPE_POINTER			XTYPE_MAKE_FUNDAMENTAL (17)
/**
 * XTYPE_BOXED:
 *
 * The fundamental type from which all boxed types are derived.
 */
#define XTYPE_BOXED			XTYPE_MAKE_FUNDAMENTAL (18)
/**
 * XTYPE_PARAM:
 *
 * The fundamental type from which all #xparam_spec_t types are derived.
 */
#define XTYPE_PARAM			XTYPE_MAKE_FUNDAMENTAL (19)
/**
 * XTYPE_OBJECT:
 *
 * The fundamental type for #xobject_t.
 */
#define XTYPE_OBJECT			XTYPE_MAKE_FUNDAMENTAL (20)
/**
 * XTYPE_VARIANT:
 *
 * The fundamental type corresponding to #xvariant_t.
 *
 * All floating #xvariant_t instances passed through the #xtype_t system are
 * consumed.
 *
 * Note that callbacks in closures, and signal handlers
 * for signals of return type %XTYPE_VARIANT, must never return floating
 * variants.
 *
 * Note: GLib 2.24 did include a boxed type with this name. It was replaced
 * with this fundamental type in 2.26.
 *
 * Since: 2.26
 */
#define	XTYPE_VARIANT                  XTYPE_MAKE_FUNDAMENTAL (21)


/* Reserved fundamental type numbers to create new fundamental
 * type IDs with XTYPE_MAKE_FUNDAMENTAL().
 *
 * Open an issue on https://gitlab.gnome.org/GNOME/glib/issues/new for
 * reservations.
 */
/**
 * XTYPE_FUNDAMENTAL_SHIFT:
 *
 * Shift value used in converting numbers to type IDs.
 */
#define	XTYPE_FUNDAMENTAL_SHIFT	(2)
/**
 * XTYPE_MAKE_FUNDAMENTAL:
 * @x: the fundamental type number.
 *
 * Get the type ID for the fundamental type number @x.
 *
 * Use xtype_fundamental_next() instead of this macro to create new fundamental
 * types.
 *
 * Returns: the xtype_t
 */
#define	XTYPE_MAKE_FUNDAMENTAL(x)	((xtype_t) ((x) << XTYPE_FUNDAMENTAL_SHIFT))
/**
 * XTYPE_RESERVED_XPL_FIRST:
 *
 * First fundamental type number to create a new fundamental type id with
 * XTYPE_MAKE_FUNDAMENTAL() reserved for GLib.
 */
#define XTYPE_RESERVED_XPL_FIRST	(22)
/**
 * XTYPE_RESERVED_XPL_LAST:
 *
 * Last fundamental type number reserved for GLib.
 */
#define XTYPE_RESERVED_XPL_LAST	(31)
/**
 * XTYPE_RESERVED_BSE_FIRST:
 *
 * First fundamental type number to create a new fundamental type id with
 * XTYPE_MAKE_FUNDAMENTAL() reserved for BSE.
 */
#define XTYPE_RESERVED_BSE_FIRST	(32)
/**
 * XTYPE_RESERVED_BSE_LAST:
 *
 * Last fundamental type number reserved for BSE.
 */
#define XTYPE_RESERVED_BSE_LAST	(48)
/**
 * XTYPE_RESERVED_USER_FIRST:
 *
 * First available fundamental type number to create new fundamental
 * type id with XTYPE_MAKE_FUNDAMENTAL().
 */
#define XTYPE_RESERVED_USER_FIRST	(49)


/* Type Checking Macros
 */
/**
 * XTYPE_IS_FUNDAMENTAL:
 * @type: A #xtype_t value
 *
 * Checks if @type is a fundamental type.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_FUNDAMENTAL(type)             ((type) <= XTYPE_FUNDAMENTAL_MAX)
/**
 * XTYPE_IS_DERIVED:
 * @type: A #xtype_t value
 *
 * Checks if @type is derived (or in object-oriented terminology:
 * inherited) from another type (this holds true for all non-fundamental
 * types).
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_DERIVED(type)                 ((type) > XTYPE_FUNDAMENTAL_MAX)
/**
 * XTYPE_IS_INTERFACE:
 * @type: A #xtype_t value
 *
 * Checks if @type is an interface type.
 *
 * An interface type provides a pure API, the implementation
 * of which is provided by another type (which is then said to conform
 * to the interface).  GLib interfaces are somewhat analogous to Java
 * interfaces and C++ classes containing only pure virtual functions,
 * with the difference that xtype_t interfaces are not derivable (but see
 * xtype_interface_add_prerequisite() for an alternative).
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_INTERFACE(type)               (XTYPE_FUNDAMENTAL (type) == XTYPE_INTERFACE)
/**
 * XTYPE_IS_CLASSED:
 * @type: A #xtype_t value
 *
 * Checks if @type is a classed type.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_CLASSED(type)                 (xtype_test_flags ((type), XTYPE_FLAG_CLASSED))
/**
 * XTYPE_IS_INSTANTIATABLE:
 * @type: A #xtype_t value
 *
 * Checks if @type can be instantiated.  Instantiation is the
 * process of creating an instance (object) of this type.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_INSTANTIATABLE(type)          (xtype_test_flags ((type), XTYPE_FLAG_INSTANTIATABLE))
/**
 * XTYPE_IS_DERIVABLE:
 * @type: A #xtype_t value
 *
 * Checks if @type is a derivable type.  A derivable type can
 * be used as the base class of a flat (single-level) class hierarchy.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_DERIVABLE(type)               (xtype_test_flags ((type), XTYPE_FLAG_DERIVABLE))
/**
 * XTYPE_IS_DEEP_DERIVABLE:
 * @type: A #xtype_t value
 *
 * Checks if @type is a deep derivable type.  A deep derivable type
 * can be used as the base class of a deep (multi-level) class hierarchy.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_DEEP_DERIVABLE(type)          (xtype_test_flags ((type), XTYPE_FLAG_DEEP_DERIVABLE))
/**
 * XTYPE_IS_ABSTRACT:
 * @type: A #xtype_t value
 *
 * Checks if @type is an abstract type.  An abstract type cannot be
 * instantiated and is normally used as an abstract base class for
 * derived classes.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_ABSTRACT(type)                (xtype_test_flags ((type), XTYPE_FLAG_ABSTRACT))
/**
 * XTYPE_IS_VALUE_ABSTRACT:
 * @type: A #xtype_t value
 *
 * Checks if @type is an abstract value type.  An abstract value type introduces
 * a value table, but can't be used for xvalue_init() and is normally used as
 * an abstract base type for derived value types.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_VALUE_ABSTRACT(type)          (xtype_test_flags ((type), XTYPE_FLAG_VALUE_ABSTRACT))
/**
 * XTYPE_IS_VALUE_TYPE:
 * @type: A #xtype_t value
 *
 * Checks if @type is a value type and can be used with xvalue_init().
 *
 * Returns: %TRUE on success
 */
#define XTYPE_IS_VALUE_TYPE(type)              (xtype_check_is_value_type (type))
/**
 * XTYPE_HAS_VALUE_TABLE:
 * @type: A #xtype_t value
 *
 * Checks if @type has a #xtype_value_table_t.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_HAS_VALUE_TABLE(type)            (xtype_value_table_peek (type) != NULL)
/**
 * XTYPE_IS_FINAL:
 * @type: a #xtype_t value
 *
 * Checks if @type is a final type. A final type cannot be derived any
 * further.
 *
 * Returns: %TRUE on success
 *
 * Since: 2.70
 */
#define XTYPE_IS_FINAL(type)                   (xtype_test_flags ((type), XTYPE_FLAG_FINAL)) XPL_AVAILABLE_MACRO_IN_2_70


/* Typedefs
 */
/**
 * xtype_t:
 *
 * A numerical value which represents the unique identifier of a registered
 * type.
 */
#if     XPL_SIZEOF_SIZE_T != XPL_SIZEOF_LONG || !defined __cplusplus
typedef xsize_t                           xtype_t;
#else   /* for historic reasons, C++ links against xulong_t GTypes */
typedef xulong_t                          xtype_t;
#endif
typedef struct _GValue                  xvalue_t;
typedef union  _xtype_c_value             xtype_c_value_t;
typedef struct _GTypePlugin             GTypePlugin;
typedef struct _GTypeClass              xtype_class_t;
typedef struct _GTypeInterface          xtype_interface_t;
typedef struct _GTypeInstance           GTypeInstance;
typedef struct _GTypeInfo               xtype_info_t;
typedef struct _GTypeFundamentalInfo    GTypeFundamentalInfo;
typedef struct _GInterfaceInfo          xinterface_info_t;
typedef struct _xtype_value_table_t         xtype_value_table_t;
typedef struct _GTypeQuery		GTypeQuery;


/* Basic Type Structures
 */
/**
 * xtype_class_t:
 *
 * An opaque structure used as the base of all classes.
 */
struct _GTypeClass
{
  /*< private >*/
  xtype_t g_type;
};
/**
 * GTypeInstance:
 *
 * An opaque structure used as the base of all type instances.
 */
struct _GTypeInstance
{
  /*< private >*/
  xtype_class_t *g_class;
};
/**
 * xtype_interface_t:
 *
 * An opaque structure used as the base of all interface types.
 */
struct _GTypeInterface
{
  /*< private >*/
  xtype_t g_type;         /* iface type */
  xtype_t g_instance_type;
};
/**
 * GTypeQuery:
 * @type: the #xtype_t value of the type
 * @type_name: the name of the type
 * @class_size: the size of the class structure
 * @instance_size: the size of the instance structure
 *
 * A structure holding information for a specific type.
 *
 * See also: xtype_query()
 */
struct _GTypeQuery
{
  xtype_t		type;
  const xchar_t  *type_name;
  xuint_t		class_size;
  xuint_t		instance_size;
};


/* Casts, checks and accessors for structured types
 * usage of these macros is reserved to type implementations only
 */
/*< protected >*/
/**
 * XTYPE_CHECK_INSTANCE:
 * @instance: Location of a #GTypeInstance structure
 *
 * Checks if @instance is a valid #GTypeInstance structure,
 * otherwise issues a warning and returns %FALSE. %NULL is not a valid
 * #GTypeInstance.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_CHECK_INSTANCE(instance)				(_XTYPE_CHI ((GTypeInstance*) (instance)))
/**
 * XTYPE_CHECK_INSTANCE_CAST:
 * @instance: (nullable): Location of a #GTypeInstance structure
 * @g_type: The type to be returned
 * @c_type_t: The corresponding C type of @g_type
 *
 * Checks that @instance is an instance of the type identified by @g_type
 * and issues a warning if this is not the case. Returns @instance casted
 * to a pointer to @c_type.
 *
 * No warning will be issued if @instance is %NULL, and %NULL will be returned.
 *
 * This macro should only be used in type implementations.
 */
#define XTYPE_CHECK_INSTANCE_CAST(instance, g_type, c_type_t)    (_XTYPE_CIC ((instance), (g_type), c_type_t))
/**
 * XTYPE_CHECK_INSTANCE_TYPE:
 * @instance: (nullable): Location of a #GTypeInstance structure.
 * @g_type: The type to be checked
 *
 * Checks if @instance is an instance of the type identified by @g_type. If
 * @instance is %NULL, %FALSE will be returned.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_CHECK_INSTANCE_TYPE(instance, g_type)            (_XTYPE_CIT ((instance), (g_type)))
/**
 * XTYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE:
 * @instance: (nullable): Location of a #GTypeInstance structure.
 * @g_type: The fundamental type to be checked
 *
 * Checks if @instance is an instance of the fundamental type identified by @g_type.
 * If @instance is %NULL, %FALSE will be returned.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE(instance, g_type)            (_XTYPE_CIFT ((instance), (g_type)))
/**
 * XTYPE_INSTANCE_GET_CLASS:
 * @instance: Location of the #GTypeInstance structure
 * @g_type: The #xtype_t of the class to be returned
 * @c_type: The C type of the class structure
 *
 * Get the class structure of a given @instance, casted
 * to a specified ancestor type @g_type of the instance.
 *
 * Note that while calling a xinstance_init_func_t(), the class pointer
 * gets modified, so it might not always return the expected pointer.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: a pointer to the class structure
 */
#define XTYPE_INSTANCE_GET_CLASS(instance, g_type, c_type)     (_XTYPE_IGC ((instance), (g_type), c_type))
/**
 * XTYPE_INSTANCE_GET_INTERFACE:
 * @instance: Location of the #GTypeInstance structure
 * @g_type: The #xtype_t of the interface to be returned
 * @c_type: The C type of the interface structure
 *
 * Get the interface structure for interface @g_type of a given @instance.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: a pointer to the interface structure
 */
#define XTYPE_INSTANCE_GET_INTERFACE(instance, g_type, c_type) (_XTYPE_IGI ((instance), (g_type), c_type))
/**
 * XTYPE_CHECK_CLASS_CAST:
 * @g_class: Location of a #xtype_class_t structure
 * @g_type: The type to be returned
 * @c_type: The corresponding C type of class structure of @g_type
 *
 * Checks that @g_class is a class structure of the type identified by @g_type
 * and issues a warning if this is not the case. Returns @g_class casted
 * to a pointer to @c_type. %NULL is not a valid class structure.
 *
 * This macro should only be used in type implementations.
 */
#define XTYPE_CHECK_CLASS_CAST(g_class, g_type, c_type)        (_XTYPE_CCC ((g_class), (g_type), c_type))
/**
 * XTYPE_CHECK_CLASS_TYPE:
 * @g_class: (nullable): Location of a #xtype_class_t structure
 * @g_type: The type to be checked
 *
 * Checks if @g_class is a class structure of the type identified by
 * @g_type. If @g_class is %NULL, %FALSE will be returned.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_CHECK_CLASS_TYPE(g_class, g_type)                (_XTYPE_CCT ((g_class), (g_type)))
/**
 * XTYPE_CHECK_VALUE:
 * @value: a #xvalue_t
 *
 * Checks if @value has been initialized to hold values
 * of a value type.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_CHECK_VALUE(value)				(_XTYPE_CHV ((value)))
/**
 * XTYPE_CHECK_VALUE_TYPE:
 * @value: a #xvalue_t
 * @g_type: The type to be checked
 *
 * Checks if @value has been initialized to hold values
 * of type @g_type.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: %TRUE on success
 */
#define XTYPE_CHECK_VALUE_TYPE(value, g_type)			(_XTYPE_CVH ((value), (g_type)))
/**
 * XTYPE_FROM_INSTANCE:
 * @instance: Location of a valid #GTypeInstance structure
 *
 * Get the type identifier from a given @instance structure.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: the #xtype_t
 */
#define XTYPE_FROM_INSTANCE(instance)                          (XTYPE_FROM_CLASS (((GTypeInstance*) (instance))->g_class))
/**
 * XTYPE_FROM_CLASS:
 * @g_class: Location of a valid #xtype_class_t structure
 *
 * Get the type identifier from a given @class structure.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: the #xtype_t
 */
#define XTYPE_FROM_CLASS(g_class)                              (((xtype_class_t*) (g_class))->g_type)
/**
 * XTYPE_FROM_INTERFACE:
 * @x_iface: Location of a valid #xtype_interface_t structure
 *
 * Get the type identifier from a given @interface structure.
 *
 * This macro should only be used in type implementations.
 *
 * Returns: the #xtype_t
 */
#define XTYPE_FROM_INTERFACE(x_iface)                          (((xtype_interface_t*) (x_iface))->g_type)

/**
 * XTYPE_INSTANCE_GET_PRIVATE:
 * @instance: the instance of a type deriving from @private_type
 * @g_type: the type identifying which private data to retrieve
 * @c_type: The C type for the private structure
 *
 * Gets the private structure for a particular type.
 *
 * The private structure must have been registered in the
 * class_init function with xtype_class_add_private().
 *
 * This macro should only be used in type implementations.
 *
 * Since: 2.4
 * Deprecated: 2.58: Use G_ADD_PRIVATE() and the generated
 *   `your_type_get_instance_private()` function instead
 * Returns: (not nullable): a pointer to the private data structure
 */
#define XTYPE_INSTANCE_GET_PRIVATE(instance, g_type, c_type)   ((c_type*) xtype_instance_get_private ((GTypeInstance*) (instance), (g_type))) XPL_DEPRECATED_MACRO_IN_2_58_FOR(G_ADD_PRIVATE)

/**
 * XTYPE_CLASS_GET_PRIVATE:
 * @klass: the class of a type deriving from @private_type
 * @g_type: the type identifying which private data to retrieve
 * @c_type: The C type for the private structure
 *
 * Gets the private class structure for a particular type.
 *
 * The private structure must have been registered in the
 * get_type() function with xtype_add_class_private().
 *
 * This macro should only be used in type implementations.
 *
 * Since: 2.24
 * Returns: (not nullable): a pointer to the private data structure
 */
#define XTYPE_CLASS_GET_PRIVATE(klass, g_type, c_type)   ((c_type*) xtype_class_get_private ((xtype_class_t*) (klass), (g_type)))

/**
 * GTypeDebugFlags:
 * @XTYPE_DEBUG_NONE: Print no messages
 * @XTYPE_DEBUG_OBJECTS: Print messages about object bookkeeping
 * @XTYPE_DEBUG_SIGNALS: Print messages about signal emissions
 * @XTYPE_DEBUG_MASK: Mask covering all debug flags
 * @XTYPE_DEBUG_INSTANCE_COUNT: Keep a count of instances of each type
 *
 * These flags used to be passed to xtype_init_with_debug_flags() which
 * is now deprecated.
 *
 * If you need to enable debugging features, use the GOBJECT_DEBUG
 * environment variable.
 *
 * Deprecated: 2.36: xtype_init() is now done automatically
 */
typedef enum	/*< skip >*/
{
  XTYPE_DEBUG_NONE	= 0,
  XTYPE_DEBUG_OBJECTS	= 1 << 0,
  XTYPE_DEBUG_SIGNALS	= 1 << 1,
  XTYPE_DEBUG_INSTANCE_COUNT = 1 << 2,
  XTYPE_DEBUG_MASK	= 0x07
} GTypeDebugFlags XPL_DEPRECATED_TYPE_IN_2_36;


/* --- prototypes --- */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_36
void                  xtype_init                    (void);
XPL_DEPRECATED_IN_2_36
void                  xtype_init_with_debug_flags   (GTypeDebugFlags  debug_flags);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
const xchar_t *         xtype_name                    (xtype_t            type);
XPL_AVAILABLE_IN_ALL
xquark                xtype_qname                   (xtype_t            type);
XPL_AVAILABLE_IN_ALL
xtype_t                 xtype_from_name               (const xchar_t     *name);
XPL_AVAILABLE_IN_ALL
xtype_t                 xtype_parent                  (xtype_t            type);
XPL_AVAILABLE_IN_ALL
xuint_t                 xtype_depth                   (xtype_t            type);
XPL_AVAILABLE_IN_ALL
xtype_t                 xtype_next_base               (xtype_t            leaf_type,
						      xtype_t            root_type);
XPL_AVAILABLE_IN_ALL
xboolean_t              xtype_is_a                    (xtype_t            type,
						      xtype_t            is_a_type);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_class_ref               (xtype_t            type);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_class_peek              (xtype_t            type);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_class_peek_static       (xtype_t            type);
XPL_AVAILABLE_IN_ALL
void                  xtype_class_unref             (xpointer_t         g_class);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_class_peek_parent       (xpointer_t         g_class);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_interface_peek          (xpointer_t         instance_class,
						      xtype_t            iface_type);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_interface_peek_parent   (xpointer_t         x_iface);

XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_default_interface_ref   (xtype_t            g_type);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_default_interface_peek  (xtype_t            g_type);
XPL_AVAILABLE_IN_ALL
void                  xtype_default_interface_unref (xpointer_t         x_iface);

/* g_free() the returned arrays */
XPL_AVAILABLE_IN_ALL
xtype_t*                xtype_children                (xtype_t            type,
						      xuint_t           *n_children);
XPL_AVAILABLE_IN_ALL
xtype_t*                xtype_interfaces              (xtype_t            type,
						      xuint_t           *n_interfaces);

/* per-type _static_ data */
XPL_AVAILABLE_IN_ALL
void                  xtype_set_qdata               (xtype_t            type,
						      xquark           quark,
						      xpointer_t         data);
XPL_AVAILABLE_IN_ALL
xpointer_t              xtype_get_qdata               (xtype_t            type,
						      xquark           quark);
XPL_AVAILABLE_IN_ALL
void		      xtype_query		     (xtype_t	       type,
						      GTypeQuery      *query);

XPL_AVAILABLE_IN_2_44
int                   xtype_get_instance_count      (xtype_t            type);

/* --- type registration --- */
/**
 * xbase_init_func_t:
 * @g_class: (type xobject_t.TypeClass): The #xtype_class_t structure to initialize
 *
 * A callback function used by the type system to do base initialization
 * of the class structures of derived types.
 *
 * This function is called as part of the initialization process of all derived
 * classes and should reallocate or reset all dynamic class members copied over
 * from the parent class.
 *
 * For example, class members (such as strings) that are not sufficiently
 * handled by a plain memory copy of the parent class into the derived class
 * have to be altered. See xclass_init_func_t() for a discussion of the class
 * initialization process.
 */
typedef void   (*xbase_init_func_t)              (xpointer_t         g_class);
/**
 * xbase_finalize_func_t:
 * @g_class: (type xobject_t.TypeClass): The #xtype_class_t structure to finalize
 *
 * A callback function used by the type system to finalize those portions
 * of a derived types class structure that were setup from the corresponding
 * xbase_init_func_t() function.
 *
 * Class finalization basically works the inverse way in which class
 * initialization is performed.
 *
 * See xclass_init_func_t() for a discussion of the class initialization process.
 */
typedef void   (*xbase_finalize_func_t)          (xpointer_t         g_class);
/**
 * xclass_init_func_t:
 * @g_class: (type xobject_t.TypeClass): The #xtype_class_t structure to initialize.
 * @class_data: The @class_data member supplied via the #xtype_info_t structure.
 *
 * A callback function used by the type system to initialize the class
 * of a specific type.
 *
 * This function should initialize all static class members.
 *
 * The initialization process of a class involves:
 *
 * - Copying common members from the parent class over to the
 *   derived class structure.
 * - Zero initialization of the remaining members not copied
 *   over from the parent class.
 * - Invocation of the xbase_init_func_t() initializers of all parent
 *   types and the class' type.
 * - Invocation of the class' xclass_init_func_t() initializer.
 *
 * Since derived classes are partially initialized through a memory copy
 * of the parent class, the general rule is that xbase_init_func_t() and
 * xbase_finalize_func_t() should take care of necessary reinitialization
 * and release of those class members that were introduced by the type
 * that specified these xbase_init_func_t()/xbase_finalize_func_t().
 * xclass_init_func_t() should only care about initializing static
 * class members, while dynamic class members (such as allocated strings
 * or reference counted resources) are better handled by a xbase_init_func_t()
 * for this type, so proper initialization of the dynamic class members
 * is performed for class initialization of derived types as well.
 *
 * An example may help to correspond the intend of the different class
 * initializers:
 *
 * |[<!-- language="C" -->
 * typedef struct {
 *   xobject_class_t parent_class;
 *   xint_t         static_integer;
 *   xchar_t       *dynamic_string;
 * } TypeAClass;
 * static void
 * type_a_base_class_init (TypeAClass *class)
 * {
 *   class->dynamic_string = xstrdup ("some string");
 * }
 * static void
 * type_a_base_class_finalize (TypeAClass *class)
 * {
 *   g_free (class->dynamic_string);
 * }
 * static void
 * type_a_class_init (TypeAClass *class)
 * {
 *   class->static_integer = 42;
 * }
 *
 * typedef struct {
 *   TypeAClass   parent_class;
 *   gfloat       static_float;
 *   xstring_t     *dynamic_xstring;
 * } TypeBClass;
 * static void
 * type_b_base_class_init (TypeBClass *class)
 * {
 *   class->dynamic_xstring = xstring_new ("some other string");
 * }
 * static void
 * type_b_base_class_finalize (TypeBClass *class)
 * {
 *   xstring_free (class->dynamic_xstring);
 * }
 * static void
 * type_b_class_init (TypeBClass *class)
 * {
 *   class->static_float = 3.14159265358979323846;
 * }
 * ]|
 *
 * Initialization of TypeBClass will first cause initialization of
 * TypeAClass (derived classes reference their parent classes, see
 * xtype_class_ref() on this).
 *
 * Initialization of TypeAClass roughly involves zero-initializing its fields,
 * then calling its xbase_init_func_t() type_a_base_class_init() to allocate
 * its dynamic members (dynamic_string), and finally calling its xclass_init_func_t()
 * type_a_class_init() to initialize its static members (static_integer).
 * The first step in the initialization process of TypeBClass is then
 * a plain memory copy of the contents of TypeAClass into TypeBClass and
 * zero-initialization of the remaining fields in TypeBClass.
 * The dynamic members of TypeAClass within TypeBClass now need
 * reinitialization which is performed by calling type_a_base_class_init()
 * with an argument of TypeBClass.
 *
 * After that, the xbase_init_func_t() of TypeBClass, type_b_base_class_init()
 * is called to allocate the dynamic members of TypeBClass (dynamic_xstring),
 * and finally the xclass_init_func_t() of TypeBClass, type_b_class_init(),
 * is called to complete the initialization process with the static members
 * (static_float).
 *
 * Corresponding finalization counter parts to the xbase_init_func_t() functions
 * have to be provided to release allocated resources at class finalization
 * time.
 */
typedef void   (*xclass_init_func_t)             (xpointer_t         g_class,
					      xpointer_t         class_data);
/**
 * xclass_finalize_func_t:
 * @g_class: (type xobject_t.TypeClass): The #xtype_class_t structure to finalize
 * @class_data: The @class_data member supplied via the #xtype_info_t structure
 *
 * A callback function used by the type system to finalize a class.
 *
 * This function is rarely needed, as dynamically allocated class resources
 * should be handled by xbase_init_func_t() and xbase_finalize_func_t().
 *
 * Also, specification of a xclass_finalize_func_t() in the #xtype_info_t
 * structure of a static type is invalid, because classes of static types
 * will never be finalized (they are artificially kept alive when their
 * reference count drops to zero).
 */
typedef void   (*xclass_finalize_func_t)         (xpointer_t         g_class,
					      xpointer_t         class_data);
/**
 * xinstance_init_func_t:
 * @instance: The instance to initialize
 * @g_class: (type xobject_t.TypeClass): The class of the type the instance is
 *    created for
 *
 * A callback function used by the type system to initialize a new
 * instance of a type.
 *
 * This function initializes all instance members and allocates any resources
 * required by it.
 *
 * Initialization of a derived instance involves calling all its parent
 * types instance initializers, so the class member of the instance
 * is altered during its initialization to always point to the class that
 * belongs to the type the current initializer was introduced for.
 *
 * The extended members of @instance are guaranteed to have been filled with
 * zeros before this function is called.
 */
typedef void   (*xinstance_init_func_t)          (GTypeInstance   *instance,
					      xpointer_t         g_class);
/**
 * GInterfaceInitFunc:
 * @x_iface: (type xobject_t.TypeInterface): The interface structure to initialize
 * @iface_data: The @interface_data supplied via the #xinterface_info_t structure
 *
 * A callback function used by the type system to initialize a new
 * interface.
 *
 * This function should initialize all internal data and* allocate any
 * resources required by the interface.
 *
 * The members of @iface_data are guaranteed to have been filled with
 * zeros before this function is called.
 */
typedef void   (*GInterfaceInitFunc)         (xpointer_t         x_iface,
					      xpointer_t         iface_data);
/**
 * GInterfaceFinalizeFunc:
 * @x_iface: (type xobject_t.TypeInterface): The interface structure to finalize
 * @iface_data: The @interface_data supplied via the #xinterface_info_t structure
 *
 * A callback function used by the type system to finalize an interface.
 *
 * This function should destroy any internal data and release any resources
 * allocated by the corresponding GInterfaceInitFunc() function.
 */
typedef void   (*GInterfaceFinalizeFunc)     (xpointer_t         x_iface,
					      xpointer_t         iface_data);
/**
 * GTypeClassCacheFunc:
 * @cache_data: data that was given to the xtype_add_class_cache_func() call
 * @g_class: (type xobject_t.TypeClass): The #xtype_class_t structure which is
 *    unreferenced
 *
 * A callback function which is called when the reference count of a class
 * drops to zero.
 *
 * It may use xtype_class_ref() to prevent the class from being freed. You
 * should not call xtype_class_unref() from a #GTypeClassCacheFunc function
 * to prevent infinite recursion, use xtype_class_unref_uncached() instead.
 *
 * The functions have to check the class id passed in to figure
 * whether they actually want to cache the class of this type, since all
 * classes are routed through the same #GTypeClassCacheFunc chain.
 *
 * Returns: %TRUE to stop further #GTypeClassCacheFuncs from being
 *  called, %FALSE to continue
 */
typedef xboolean_t (*GTypeClassCacheFunc)	     (xpointer_t	       cache_data,
					      xtype_class_t      *g_class);
/**
 * GTypeInterfaceCheckFunc:
 * @check_data: data passed to xtype_add_interface_check()
 * @x_iface: (type xobject_t.TypeInterface): the interface that has been
 *    initialized
 *
 * A callback called after an interface vtable is initialized.
 *
 * See xtype_add_interface_check().
 *
 * Since: 2.4
 */
typedef void     (*GTypeInterfaceCheckFunc)  (xpointer_t	       check_data,
					      xpointer_t         x_iface);
/**
 * GTypeFundamentalFlags:
 * @XTYPE_FLAG_CLASSED: Indicates a classed type
 * @XTYPE_FLAG_INSTANTIATABLE: Indicates an instantiatable type (implies classed)
 * @XTYPE_FLAG_DERIVABLE: Indicates a flat derivable type
 * @XTYPE_FLAG_DEEP_DERIVABLE: Indicates a deep derivable type (implies derivable)
 *
 * Bit masks used to check or determine specific characteristics of a
 * fundamental type.
 */
typedef enum    /*< skip >*/
{
  XTYPE_FLAG_CLASSED           = (1 << 0),
  XTYPE_FLAG_INSTANTIATABLE    = (1 << 1),
  XTYPE_FLAG_DERIVABLE         = (1 << 2),
  XTYPE_FLAG_DEEP_DERIVABLE    = (1 << 3)
} GTypeFundamentalFlags;
/**
 * xtype_flags_t:
 * @XTYPE_FLAG_ABSTRACT: Indicates an abstract type. No instances can be
 *  created for an abstract type
 * @XTYPE_FLAG_VALUE_ABSTRACT: Indicates an abstract value type, i.e. a type
 *  that introduces a value table, but can't be used for
 *  xvalue_init()
 * @XTYPE_FLAG_FINAL: Indicates a final type. A final type is a non-derivable
 *  leaf node in a deep derivable type hierarchy tree. Since: 2.70
 *
 * Bit masks used to check or determine characteristics of a type.
 */
typedef enum    /*< skip >*/
{
  XTYPE_FLAG_ABSTRACT = (1 << 4),
  XTYPE_FLAG_VALUE_ABSTRACT = (1 << 5),
  XTYPE_FLAG_FINAL XPL_AVAILABLE_ENUMERATOR_IN_2_70 = (1 << 6)
} xtype_flags_t;
/**
 * xtype_info_t:
 * @class_size: Size of the class structure (required for interface, classed and instantiatable types)
 * @base_init: Location of the base initialization function (optional)
 * @base_finalize: Location of the base finalization function (optional)
 * @class_init: Location of the class initialization function for
 *  classed and instantiatable types. Location of the default vtable
 *  inititalization function for interface types. (optional) This function
 *  is used both to fill in virtual functions in the class or default vtable,
 *  and to do type-specific setup such as registering signals and object
 *  properties.
 * @class_finalize: Location of the class finalization function for
 *  classed and instantiatable types. Location of the default vtable
 *  finalization function for interface types. (optional)
 * @class_data: User-supplied data passed to the class init/finalize functions
 * @instance_size: Size of the instance (object) structure (required for instantiatable types only)
 * @n_preallocs: Prior to GLib 2.10, it specified the number of pre-allocated (cached) instances to reserve memory for (0 indicates no caching). Since GLib 2.10, it is ignored, since instances are allocated with the [slice allocator][glib-Memory-Slices] now.
 * @instance_init: Location of the instance initialization function (optional, for instantiatable types only)
 * @value_table: A #xtype_value_table_t function table for generic handling of GValues
 *  of this type (usually only useful for fundamental types)
 *
 * This structure is used to provide the type system with the information
 * required to initialize and destruct (finalize) a type's class and
 * its instances.
 *
 * The initialized structure is passed to the xtype_register_static() function
 * (or is copied into the provided #xtype_info_t structure in the
 * xtype_plugin_complete_type_info()). The type system will perform a deep
 * copy of this structure, so its memory does not need to be persistent
 * across invocation of xtype_register_static().
 */
struct _GTypeInfo
{
  /* interface types, classed types, instantiated types */
  xuint16_t                class_size;

  xbase_init_func_t          base_init;
  xbase_finalize_func_t      base_finalize;

  /* interface types, classed types, instantiated types */
  xclass_init_func_t         class_init;
  xclass_finalize_func_t     class_finalize;
  xconstpointer          class_data;

  /* instantiated types */
  xuint16_t                instance_size;
  xuint16_t                n_preallocs;
  xinstance_init_func_t      instance_init;

  /* value handling */
  const xtype_value_table_t	*value_table;
};
/**
 * GTypeFundamentalInfo:
 * @type_flags: #GTypeFundamentalFlags describing the characteristics of the fundamental type
 *
 * A structure that provides information to the type system which is
 * used specifically for managing fundamental types.
 */
struct _GTypeFundamentalInfo
{
  GTypeFundamentalFlags  type_flags;
};
/**
 * xinterface_info_t:
 * @interface_init: location of the interface initialization function
 * @interface_finalize: location of the interface finalization function
 * @interface_data: user-supplied data passed to the interface init/finalize functions
 *
 * A structure that provides information to the type system which is
 * used specifically for managing interface types.
 */
struct _GInterfaceInfo
{
  GInterfaceInitFunc     interface_init;
  GInterfaceFinalizeFunc interface_finalize;
  xpointer_t               interface_data;
};
/**
 * xtype_value_table_t:
 * @value_init: Default initialize @values contents by poking values
 *  directly into the value->data array. The data array of
 *  the #xvalue_t passed into this function was zero-filled
 *  with `memset()`, so no care has to be taken to free any
 *  old contents. E.g. for the implementation of a string
 *  value that may never be %NULL, the implementation might
 *  look like:
 *  |[<!-- language="C" -->
 *  value->data[0].v_pointer = xstrdup ("");
 *  ]|
 * @value_free: Free any old contents that might be left in the
 *  data array of the passed in @value. No resources may
 *  remain allocated through the #xvalue_t contents after
 *  this function returns. E.g. for our above string type:
 *  |[<!-- language="C" -->
 *  // only free strings without a specific flag for static storage
 *  if (!(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
 *    g_free (value->data[0].v_pointer);
 *  ]|
 * @value_copy: @dest_value is a #xvalue_t with zero-filled data section
 *  and @src_value is a properly setup #xvalue_t of same or
 *  derived type.
 *  The purpose of this function is to copy the contents of
 *  @src_value into @dest_value in a way, that even after
 *  @src_value has been freed, the contents of @dest_value
 *  remain valid. String type example:
 *  |[<!-- language="C" -->
 *  dest_value->data[0].v_pointer = xstrdup (src_value->data[0].v_pointer);
 *  ]|
 * @value_peek_pointer: If the value contents fit into a pointer, such as objects
 *  or strings, return this pointer, so the caller can peek at
 *  the current contents. To extend on our above string example:
 *  |[<!-- language="C" -->
 *  return value->data[0].v_pointer;
 *  ]|
 * @collect_format: A string format describing how to collect the contents of
 *  this value bit-by-bit. Each character in the format represents
 *  an argument to be collected, and the characters themselves indicate
 *  the type of the argument. Currently supported arguments are:
 *  - 'i' - Integers. passed as collect_values[].v_int.
 *  - 'l' - Longs. passed as collect_values[].v_long.
 *  - 'd' - Doubles. passed as collect_values[].v_double.
 *  - 'p' - Pointers. passed as collect_values[].v_pointer.
 *  It should be noted that for variable argument list construction,
 *  ANSI C promotes every type smaller than an integer to an int, and
 *  floats to doubles. So for collection of short int or char, 'i'
 *  needs to be used, and for collection of floats 'd'.
 * @collect_value: The collect_value() function is responsible for converting the
 *  values collected from a variable argument list into contents
 *  suitable for storage in a xvalue_t. This function should setup
 *  @value similar to value_init(); e.g. for a string value that
 *  does not allow %NULL pointers, it needs to either spew an error,
 *  or do an implicit conversion by storing an empty string.
 *  The @value passed in to this function has a zero-filled data
 *  array, so just like for value_init() it is guaranteed to not
 *  contain any old contents that might need freeing.
 *  @n_collect_values is exactly the string length of @collect_format,
 *  and @collect_values is an array of unions #xtype_c_value_t with
 *  length @n_collect_values, containing the collected values
 *  according to @collect_format.
 *  @collect_flags is an argument provided as a hint by the caller.
 *  It may contain the flag %G_VALUE_NOCOPY_CONTENTS indicating,
 *  that the collected value contents may be considered "static"
 *  for the duration of the @value lifetime.
 *  Thus an extra copy of the contents stored in @collect_values is
 *  not required for assignment to @value.
 *  For our above string example, we continue with:
 *  |[<!-- language="C" -->
 *  if (!collect_values[0].v_pointer)
 *    value->data[0].v_pointer = xstrdup ("");
 *  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
 *  {
 *    value->data[0].v_pointer = collect_values[0].v_pointer;
 *    // keep a flag for the value_free() implementation to not free this string
 *    value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
 *  }
 *  else
 *    value->data[0].v_pointer = xstrdup (collect_values[0].v_pointer);
 *  return NULL;
 *  ]|
 *  It should be noted, that it is generally a bad idea to follow the
 *  %G_VALUE_NOCOPY_CONTENTS hint for reference counted types. Due to
 *  reentrancy requirements and reference count assertions performed
 *  by the signal emission code, reference counts should always be
 *  incremented for reference counted contents stored in the value->data
 *  array.  To deviate from our string example for a moment, and taking
 *  a look at an exemplary implementation for collect_value() of
 *  #xobject_t:
 *  |[<!-- language="C" -->
 *    xobject_t *object = G_OBJECT (collect_values[0].v_pointer);
 *    g_return_val_if_fail (object != NULL,
 *       xstrdup_printf ("Object passed as invalid NULL pointer"));
 *    // never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types
 *    value->data[0].v_pointer = xobject_ref (object);
 *    return NULL;
 *  ]|
 *  The reference count for valid objects is always incremented,
 *  regardless of @collect_flags. For invalid objects, the example
 *  returns a newly allocated string without altering @value.
 *  Upon success, collect_value() needs to return %NULL. If, however,
 *  an error condition occurred, collect_value() may spew an
 *  error by returning a newly allocated non-%NULL string, giving
 *  a suitable description of the error condition.
 *  The calling code makes no assumptions about the @value
 *  contents being valid upon error returns, @value
 *  is simply thrown away without further freeing. As such, it is
 *  a good idea to not allocate #xvalue_t contents, prior to returning
 *  an error, however, collect_values() is not obliged to return
 *  a correctly setup @value for error returns, simply because
 *  any non-%NULL return is considered a fatal condition so further
 *  program behaviour is undefined.
 * @lcopy_format: Format description of the arguments to collect for @lcopy_value,
 *  analogous to @collect_format. Usually, @lcopy_format string consists
 *  only of 'p's to provide lcopy_value() with pointers to storage locations.
 * @lcopy_value: This function is responsible for storing the @value contents into
 *  arguments passed through a variable argument list which got
 *  collected into @collect_values according to @lcopy_format.
 *  @n_collect_values equals the string length of @lcopy_format,
 *  and @collect_flags may contain %G_VALUE_NOCOPY_CONTENTS.
 *  In contrast to collect_value(), lcopy_value() is obliged to
 *  always properly support %G_VALUE_NOCOPY_CONTENTS.
 *  Similar to collect_value() the function may prematurely abort
 *  by returning a newly allocated string describing an error condition.
 *  To complete the string example:
 *  |[<!-- language="C" -->
 *  xchar_t **string_p = collect_values[0].v_pointer;
 *  g_return_val_if_fail (string_p != NULL,
 *      xstrdup_printf ("string location passed as NULL"));
 *  if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
 *    *string_p = value->data[0].v_pointer;
 *  else
 *    *string_p = xstrdup (value->data[0].v_pointer);
 *  ]|
 *  And an illustrative version of lcopy_value() for
 *  reference-counted types:
 *  |[<!-- language="C" -->
 *  xobject_t **object_p = collect_values[0].v_pointer;
 *  g_return_val_if_fail (object_p != NULL,
 *    xstrdup_printf ("object location passed as NULL"));
 *  if (!value->data[0].v_pointer)
 *    *object_p = NULL;
 *  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS) // always honour
 *    *object_p = value->data[0].v_pointer;
 *  else
 *    *object_p = xobject_ref (value->data[0].v_pointer);
 *  return NULL;
 *  ]|
 *
 * The #xtype_value_table_t provides the functions required by the #xvalue_t
 * implementation, to serve as a container for values of a type.
 */

struct _xtype_value_table_t
{
  void     (*value_init)         (xvalue_t       *value);
  void     (*value_free)         (xvalue_t       *value);
  void     (*value_copy)         (const xvalue_t *src_value,
				  xvalue_t       *dest_value);
  /* varargs functionality (optional) */
  xpointer_t (*value_peek_pointer) (const xvalue_t *value);
  const xchar_t *collect_format;
  xchar_t*   (*collect_value)      (xvalue_t       *value,
				  xuint_t         n_collect_values,
				  xtype_c_value_t  *collect_values,
				  xuint_t		collect_flags);
  const xchar_t *lcopy_format;
  xchar_t*   (*lcopy_value)        (const xvalue_t *value,
				  xuint_t         n_collect_values,
				  xtype_c_value_t  *collect_values,
				  xuint_t		collect_flags);
};
XPL_AVAILABLE_IN_ALL
xtype_t xtype_register_static		(xtype_t			     parent_type,
					 const xchar_t		    *type_name,
					 const xtype_info_t	    *info,
					 xtype_flags_t		     flags);
XPL_AVAILABLE_IN_ALL
xtype_t xtype_register_static_simple     (xtype_t                       parent_type,
					 const xchar_t                *type_name,
					 xuint_t                       class_size,
					 xclass_init_func_t              class_init,
					 xuint_t                       instance_size,
					 xinstance_init_func_t           instance_init,
					 xtype_flags_t	             flags);

XPL_AVAILABLE_IN_ALL
xtype_t xtype_register_dynamic		(xtype_t			     parent_type,
					 const xchar_t		    *type_name,
					 GTypePlugin		    *plugin,
					 xtype_flags_t		     flags);
XPL_AVAILABLE_IN_ALL
xtype_t xtype_register_fundamental	(xtype_t			     type_id,
					 const xchar_t		    *type_name,
					 const xtype_info_t	    *info,
					 const GTypeFundamentalInfo *finfo,
					 xtype_flags_t		     flags);
XPL_AVAILABLE_IN_ALL
void  xtype_add_interface_static	(xtype_t			     instance_type,
					 xtype_t			     interface_type,
					 const xinterface_info_t	    *info);
XPL_AVAILABLE_IN_ALL
void  xtype_add_interface_dynamic	(xtype_t			     instance_type,
					 xtype_t			     interface_type,
					 GTypePlugin		    *plugin);
XPL_AVAILABLE_IN_ALL
void  xtype_interface_add_prerequisite (xtype_t			     interface_type,
					 xtype_t			     prerequisite_type);
XPL_AVAILABLE_IN_ALL
xtype_t*xtype_interface_prerequisites    (xtype_t                       interface_type,
					 xuint_t                      *n_prerequisites);
XPL_AVAILABLE_IN_2_68
xtype_t xtype_interface_instantiatable_prerequisite
                                        (xtype_t                       interface_type);
XPL_DEPRECATED_IN_2_58
void     xtype_class_add_private       (xpointer_t                    g_class,
                                         xsize_t                       private_size);
XPL_AVAILABLE_IN_2_38
xint_t     xtype_add_instance_private    (xtype_t                       class_type,
                                         xsize_t                       private_size);
XPL_AVAILABLE_IN_ALL
xpointer_t xtype_instance_get_private    (GTypeInstance              *instance,
                                         xtype_t                       private_type);
XPL_AVAILABLE_IN_2_38
void     xtype_class_adjust_private_offset (xpointer_t                g_class,
                                             xint_t                   *private_size_or_offset);

XPL_AVAILABLE_IN_ALL
void      xtype_add_class_private      (xtype_t    		     class_type,
					 xsize_t    		     private_size);
XPL_AVAILABLE_IN_ALL
xpointer_t  xtype_class_get_private      (xtype_class_t 		    *klass,
					 xtype_t			     private_type);
XPL_AVAILABLE_IN_2_38
xint_t      xtype_class_get_instance_private_offset (xpointer_t         g_class);

XPL_AVAILABLE_IN_2_34
void      xtype_ensure                 (xtype_t                       type);
XPL_AVAILABLE_IN_2_36
xuint_t     xtype_get_type_registration_serial (void);


/* --- xtype_t boilerplate --- */
/**
 * G_DECLARE_FINAL_TYPE:
 * @ModuleObjNameNoT: The name of the new type, in camel case (like `GtkWidget`)
 * @module_obj_name: The name of the new type in lowercase, with words
 *  separated by `_` (like `gtk_widget`)
 * @MODULE: The name of the module, in all caps (like `GTK`)
 * @OBJ_NAME: The bare name of the type, in all caps (like `WIDGET`)
 * @ParentNameNoT: the name of the parent type, in camel case (like `GtkWidget`)
 *
 * A convenience macro for emitting the usual declarations in the header file
 * for a type which is not (at the present time) intended to be subclassed.
 *
 * You might use it in a header as follows:
 *
 * |[<!-- language="C" -->
 * #ifndef _myapp_window_h_
 * #define _myapp_window_h_
 *
 * #include <gtk/gtk.h>
 *
 * #define MY_APP_TYPE_WINDOW my_app_window_get_type ()
 * G_DECLARE_FINAL_TYPE (MyAppWindow, my_app_window, MY_APP, WINDOW, GtkWindow)
 *
 * MyAppWindow *    my_app_window_new    (void);
 *
 * ...
 *
 * #endif
 * ]|
 *
 * And use it as follow in your C file:
 *
 * |[<!-- language="C" -->
 * struct _MyAppWindow
 * {
 *  GtkWindow parent;
 *  ...
 * };
 * G_DEFINE_TYPE (MyAppWindow, my_app_window, GTK_TYPE_WINDOW)
 * ]|
 *
 * This results in the following things happening:
 *
 * - the usual `my_app_window_get_type()` function is declared with a return type of #xtype_t
 *
 * - the `MyAppWindow` type is defined as a `typedef` of `struct _MyAppWindow`.  The struct itself is not
 *   defined and should be defined from the .c file before G_DEFINE_TYPE() is used.
 *
 * - the `MY_APP_WINDOW()` cast is emitted as `static inline` function along with the `MY_APP_IS_WINDOW()` type
 *   checking function
 *
 * - the `MyAppWindowClass` type is defined as a struct containing `GtkWindowClass`.  This is done for the
 *   convenience of the person defining the type and should not be considered to be part of the ABI.  In
 *   particular, without a firm declaration of the instance structure, it is not possible to subclass the type
 *   and therefore the fact that the size of the class structure is exposed is not a concern and it can be
 *   freely changed at any point in the future.
 *
 * - x_autoptr() support being added for your type, based on the type of your parent class
 *
 * You can only use this function if your parent type also supports x_autoptr().
 *
 * Because the type macro (`MY_APP_TYPE_WINDOW` in the above example) is not a callable, you must continue to
 * manually define this as a macro for yourself.
 *
 * The declaration of the `_get_type()` function is the first thing emitted by the macro.  This allows this macro
 * to be used in the usual way with export control and API versioning macros.
 *
 * If you want to declare your own class structure, use G_DECLARE_DERIVABLE_TYPE().
 *
 * If you are writing a library, it is important to note that it is possible to convert a type from using
 * G_DECLARE_FINAL_TYPE() to G_DECLARE_DERIVABLE_TYPE() without breaking API or ABI.  As a precaution, you
 * should therefore use G_DECLARE_FINAL_TYPE() until you are sure that it makes sense for your class to be
 * subclassed.  Once a class structure has been exposed it is not possible to change its size or remove or
 * reorder items without breaking the API and/or ABI.
 *
 * Since: 2.44
 **/
#define G_DECLARE_FINAL_TYPE(ModuleObjNameNoT, module_obj_name, MODULE, OBJ_NAME, ParentNameNoT) \
  xtype_t module_obj_name##_get_type (void);                                                               \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                       \
  typedef struct _##ModuleObjNameNoT ModuleObjNameNoT##_t;                                                         \
  typedef struct { ParentNameNoT##_class_t parent_class; } ModuleObjNameNoT##_class_t;                               \
                                                                                                         \
  _XPL_DEFINE_AUTOPTR_CHAINUP (ModuleObjNameNoT, ParentNameNoT)                                               \
  G_DEFINE_AUTOPTR_CLEANUP_FUNC (ModuleObjNameNoT##_class, xtype_class_unref)                               \
                                                                                                         \
  G_GNUC_UNUSED static inline ModuleObjNameNoT##_t * MODULE##_##OBJ_NAME (xpointer_t ptr) {                       \
    return XTYPE_CHECK_INSTANCE_CAST (ptr, module_obj_name##_get_type (), ModuleObjNameNoT##_t); }             \
  G_GNUC_UNUSED static inline xboolean_t MODULE##_IS_##OBJ_NAME (xpointer_t ptr) {                           \
    return XTYPE_CHECK_INSTANCE_TYPE (ptr, module_obj_name##_get_type ()); }                            \
  G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * G_DECLARE_DERIVABLE_TYPE:
 * @ModuleObjNameNoT: The name of the new type, in camel case (like `GtkWidget`)
 * @module_obj_name: The name of the new type in lowercase, with words
 *  separated by `_` (like `gtk_widget`)
 * @MODULE: The name of the module, in all caps (like `GTK`)
 * @OBJ_NAME: The bare name of the type, in all caps (like `WIDGET`)
 * @ParentNameNoT: the name of the parent type, in camel case (like `GtkWidget`)
 *
 * A convenience macro for emitting the usual declarations in the
 * header file for a type which is intended to be subclassed.
 *
 * You might use it in a header as follows:
 *
 * |[<!-- language="C" -->
 * #ifndef _gtk_frobber_h_
 * #define _gtk_frobber_h_
 *
 * #define GTK_TYPE_FROBBER gtk_frobber_get_type ()
 * GDK_AVAILABLE_IN_3_12
 * G_DECLARE_DERIVABLE_TYPE (GtkFrobber, gtk_frobber, GTK, FROBBER, GtkWidget)
 *
 * struct _GtkFrobberClass
 * {
 *   GtkWidgetClass parent_class;
 *
 *   void (* handle_frob)  (GtkFrobber *frobber,
 *                          xuint_t       n_frobs);
 *
 *   xpointer_t padding[12];
 * };
 *
 * GtkWidget *    gtk_frobber_new   (void);
 *
 * ...
 *
 * #endif
 * ]|
 *
 * Since the instance structure is public it is often needed to declare a
 * private struct as follow in your C file:
 *
 * |[<!-- language="C" -->
 * typedef struct _GtkFrobberPrivate GtkFrobberPrivate;
 * struct _GtkFrobberPrivate
 * {
 *   ...
 * };
 * G_DEFINE_TYPE_WITH_PRIVATE (GtkFrobber, gtk_frobber, GTK_TYPE_WIDGET)
 * ]|
 *
 * This results in the following things happening:
 *
 * - the usual `gtk_frobber_get_type()` function is declared with a return type of #xtype_t
 *
 * - the `GtkFrobber` struct is created with `GtkWidget` as the first and only item.  You are expected to use
 *   a private structure from your .c file to store your instance variables.
 *
 * - the `GtkFrobberClass` type is defined as a typedef to `struct _GtkFrobberClass`, which is left undefined.
 *   You should do this from the header file directly after you use the macro.
 *
 * - the `GTK_FROBBER()` and `GTK_FROBBER_CLASS()` casts are emitted as `static inline` functions along with
 *   the `GTK_IS_FROBBER()` and `GTK_IS_FROBBER_CLASS()` type checking functions and `GTK_FROBBER_GET_CLASS()`
 *   function.
 *
 * - x_autoptr() support being added for your type, based on the type of your parent class
 *
 * You can only use this function if your parent type also supports x_autoptr().
 *
 * Because the type macro (`GTK_TYPE_FROBBER` in the above example) is not a callable, you must continue to
 * manually define this as a macro for yourself.
 *
 * The declaration of the `_get_type()` function is the first thing emitted by the macro.  This allows this macro
 * to be used in the usual way with export control and API versioning macros.
 *
 * If you are writing a library, it is important to note that it is possible to convert a type from using
 * G_DECLARE_FINAL_TYPE() to G_DECLARE_DERIVABLE_TYPE() without breaking API or ABI.  As a precaution, you
 * should therefore use G_DECLARE_FINAL_TYPE() until you are sure that it makes sense for your class to be
 * subclassed.  Once a class structure has been exposed it is not possible to change its size or remove or
 * reorder items without breaking the API and/or ABI.  If you want to declare your own class structure, use
 * G_DECLARE_DERIVABLE_TYPE().  If you want to declare a class without exposing the class or instance
 * structures, use G_DECLARE_FINAL_TYPE().
 *
 * If you must use G_DECLARE_DERIVABLE_TYPE() you should be sure to include some padding at the bottom of your
 * class structure to leave space for the addition of future virtual functions.
 *
 * Since: 2.44
 **/
#define G_DECLARE_DERIVABLE_TYPE(ModuleObjNameNoT, module_obj_name, MODULE, OBJ_NAME, ParentNameNoT) \
  xtype_t module_obj_name##_get_type (void);                                                               \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                       \
  typedef struct _##ModuleObjNameNoT ModuleObjNameNoT##_t;                                                         \
  typedef struct _##ModuleObjNameNoT##_class ModuleObjNameNoT##_class_t;                                           \
  struct _##ModuleObjNameNoT { ParentNameNoT##_t parent_instance; };                                               \
                                                                                                         \
  _XPL_DEFINE_AUTOPTR_CHAINUP (ModuleObjNameNoT, ParentNameNoT)                                               \
  G_DEFINE_AUTOPTR_CLEANUP_FUNC (ModuleObjNameNoT##_class, xtype_class_unref)                               \
                                                                                                         \
  G_GNUC_UNUSED static inline ModuleObjNameNoT##_t * MODULE##_##OBJ_NAME (xpointer_t ptr) {                       \
    return XTYPE_CHECK_INSTANCE_CAST (ptr, module_obj_name##_get_type (), ModuleObjNameNoT##_t); }             \
  G_GNUC_UNUSED static inline ModuleObjNameNoT##_class_t * MODULE##_##OBJ_NAME##_CLASS (xpointer_t ptr) {        \
    return XTYPE_CHECK_CLASS_CAST (ptr, module_obj_name##_get_type (), ModuleObjNameNoT##_class_t); }         \
  G_GNUC_UNUSED static inline xboolean_t MODULE##_IS_##OBJ_NAME (xpointer_t ptr) {                           \
    return XTYPE_CHECK_INSTANCE_TYPE (ptr, module_obj_name##_get_type ()); }                            \
  G_GNUC_UNUSED static inline xboolean_t MODULE##_IS_##OBJ_NAME##_CLASS (xpointer_t ptr) {                   \
    return XTYPE_CHECK_CLASS_TYPE (ptr, module_obj_name##_get_type ()); }                               \
  G_GNUC_UNUSED static inline ModuleObjNameNoT##_class_t * MODULE##_##OBJ_NAME##_GET_CLASS (xpointer_t ptr) {    \
    return XTYPE_INSTANCE_GET_CLASS (ptr, module_obj_name##_get_type (), ModuleObjNameNoT##_class_t); }       \
  G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * G_DECLARE_INTERFACE:
 * @ModuleObjNameNoT: The name of the new type, in camel case (like `GtkWidget`)
 * @module_obj_name: The name of the new type in lowercase, with words
 *  separated by `_` (like `gtk_widget`)
 * @MODULE: The name of the module, in all caps (like `GTK`)
 * @OBJ_NAME: The bare name of the type, in all caps (like `WIDGET`)
 * @PrerequisiteName: the name of the prerequisite type, in camel case (like `GtkWidget`)
 *
 * A convenience macro for emitting the usual declarations in the header file for a #GInterface type.
 *
 * You might use it in a header as follows:
 *
 * |[<!-- language="C" -->
 * #ifndef _my_model_h_
 * #define _my_model_h_
 *
 * #define MY_TYPE_MODEL my_model_get_type ()
 * GDK_AVAILABLE_IN_3_12
 * G_DECLARE_INTERFACE (MyModel, my_model, MY, MODEL, xobject_t)
 *
 * struct _MyModelInterface
 * {
 *   xtype_interface_t x_iface;
 *
 *   xpointer_t (* get_item)  (MyModel *model);
 * };
 *
 * xpointer_t my_model_get_item (MyModel *model);
 *
 * ...
 *
 * #endif
 * ]|
 *
 * And use it as follow in your C file:
 *
 * |[<!-- language="C" -->
 * G_DEFINE_INTERFACE (MyModel, my_model, XTYPE_OBJECT);
 *
 * static void
 * my_model_default_init (MyModelInterface *iface)
 * {
 *   ...
 * }
 * ]|
 *
 * This results in the following things happening:
 *
 * - the usual `my_model_get_type()` function is declared with a return type of #xtype_t
 *
 * - the `MyModelInterface` type is defined as a typedef to `struct _MyModelInterface`,
 *   which is left undefined. You should do this from the header file directly after
 *   you use the macro.
 *
 * - the `MY_MODEL()` cast is emitted as `static inline` functions along with
 *   the `MY_IS_MODEL()` type checking function and `MY_MODEL_GET_IFACE()` function.
 *
 * - x_autoptr() support being added for your type, based on your prerequisite type.
 *
 * You can only use this function if your prerequisite type also supports x_autoptr().
 *
 * Because the type macro (`MY_TYPE_MODEL` in the above example) is not a callable, you must continue to
 * manually define this as a macro for yourself.
 *
 * The declaration of the `_get_type()` function is the first thing emitted by the macro.  This allows this macro
 * to be used in the usual way with export control and API versioning macros.
 *
 * Since: 2.44
 **/
#define G_DECLARE_INTERFACE(ModuleObjNameNoT, module_obj_name, MODULE, OBJ_NAME, PrerequisiteNameNoT) \
  xtype_t module_obj_name##_get_type (void);                                                                 \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                         \
  typedef struct _##ModuleObjNameNoT ModuleObjNameNoT##_t;                                                           \
  typedef struct _##ModuleObjNameNoT##_interface ModuleObjNameNoT##_interface_t;                                     \
                                                                                                           \
  _XPL_DEFINE_AUTOPTR_CHAINUP (ModuleObjNameNoT, PrerequisiteNameNoT)                                           \
                                                                                                           \
  G_GNUC_UNUSED static inline ModuleObjNameNoT * MODULE##_##OBJ_NAME (xpointer_t ptr) {                         \
    return XTYPE_CHECK_INSTANCE_CAST (ptr, module_obj_name##_get_type (), ModuleObjNameNoT##_t); }               \
  G_GNUC_UNUSED static inline xboolean_t MODULE##_IS_##OBJ_NAME (xpointer_t ptr) {                             \
    return XTYPE_CHECK_INSTANCE_TYPE (ptr, module_obj_name##_get_type ()); }                              \
  G_GNUC_UNUSED static inline ModuleObjNameNoT##_interface_t * MODULE##_##OBJ_NAME##_GET_IFACE (xpointer_t ptr) {  \
    return XTYPE_INSTANCE_GET_INTERFACE (ptr, module_obj_name##_get_type (), ModuleObjNameNoT##_interface_t); } \
  G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * G_DEFINE_TYPE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *  separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 *
 * A convenience macro for type implementations, which declares a class
 * initialization function, an instance initialization function (see #xtype_info_t
 * for information about these) and a static variable named `t_n_parent_class`
 * pointing to the parent class. Furthermore, it defines a `*_get_type()` function.
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.4
 */
#define G_DEFINE_TYPE(TN, t_n, T_P)			    G_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, 0, {})
/**
 * G_DEFINE_TYPE_WITH_CODE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type in lowercase, with words separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 * @_C_: Custom code that gets inserted in the `*_get_type()` function.
 *
 * A convenience macro for type implementations.
 *
 * Similar to G_DEFINE_TYPE(), but allows you to insert custom code into the
 * `*_get_type()` function, e.g. interface implementations via G_IMPLEMENT_INTERFACE().
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.4
 */
#define G_DEFINE_TYPE_WITH_CODE(TNxT, t_n, T_P, _C_)	    _G_DEFINE_TYPE_EXTENDED_BEGIN (TNxT, t_n, T_P, 0) {_C_;} _G_DEFINE_TYPE_EXTENDED_END()
/**
 * G_DEFINE_TYPE_WITH_PRIVATE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *  separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 *
 * A convenience macro for type implementations, which declares a class
 * initialization function, an instance initialization function (see #xtype_info_t
 * for information about these), a static variable named `t_n_parent_class`
 * pointing to the parent class, and adds private instance data to the type.
 *
 * Furthermore, it defines a `*_get_type()` function. See G_DEFINE_TYPE_EXTENDED()
 * for an example.
 *
 * Note that private structs added with this macros must have a struct
 * name of the form `TN ## Private`.
 *
 * The private instance data can be retrieved using the automatically generated
 * getter function `t_n_get_instance_private()`.
 *
 * See also: G_ADD_PRIVATE()
 *
 * Since: 2.38
 */
#define G_DEFINE_TYPE_WITH_PRIVATE(TN, t_n, T_P)            G_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, 0, G_ADD_PRIVATE (TN))
/**
 * G_DEFINE_ABSTRACT_TYPE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *  separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 *
 * A convenience macro for type implementations.
 *
 * Similar to G_DEFINE_TYPE(), but defines an abstract type.
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.4
 */
#define G_DEFINE_ABSTRACT_TYPE(TN, t_n, T_P)		    G_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, XTYPE_FLAG_ABSTRACT, {})
/**
 * G_DEFINE_ABSTRACT_TYPE_WITH_CODE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *  separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 * @_C_: Custom code that gets inserted in the `type_name_get_type()` function.
 *
 * A convenience macro for type implementations.
 *
 * Similar to G_DEFINE_TYPE_WITH_CODE(), but defines an abstract type and
 * allows you to insert custom code into the `*_get_type()` function, e.g.
 * interface implementations via G_IMPLEMENT_INTERFACE().
 *
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.4
 */
#define G_DEFINE_ABSTRACT_TYPE_WITH_CODE(TNxT, t_n, T_P, _C_) _G_DEFINE_TYPE_EXTENDED_BEGIN (TNxT, t_n, T_P, XTYPE_FLAG_ABSTRACT) {_C_;} _G_DEFINE_TYPE_EXTENDED_END()
/**
 * G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *  separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 *
 * Similar to G_DEFINE_TYPE_WITH_PRIVATE(), but defines an abstract type.
 *
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.38
 */
#define G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(TN, t_n, T_P)   G_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, XTYPE_FLAG_ABSTRACT, G_ADD_PRIVATE (TN))
/**
 * G_DEFINE_FINAL_TYPE:
 * @TN: the name of the new type, in Camel case
 * @t_n: the name of the new type, in lower case, with words
 *   separated by `_` (snake case)
 * @T_P: the #xtype_t of the parent type
 *
 * A convenience macro for type implementations.
 *
 * Similar to G_DEFINE_TYPE(), but defines a final type.
 *
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.70
 */
#define G_DEFINE_FINAL_TYPE(TN, t_n, T_P)                      G_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, XTYPE_FLAG_FINAL, {}) XPL_AVAILABLE_MACRO_IN_2_70
/**
 * G_DEFINE_FINAL_TYPE_WITH_CODE:
 * @TN: the name of the new type, in Camel case
 * @t_n: the name of the new type, in lower case, with words
 *   separated by `_` (snake case)
 * @T_P: the #xtype_t of the parent type
 * @_C_: Custom code that gets inserted in the `type_name_get_type()` function.
 *
 * A convenience macro for type implementations.
 *
 * Similar to G_DEFINE_TYPE_WITH_CODE(), but defines a final type and
 * allows you to insert custom code into the `*_get_type()` function, e.g.
 * interface implementations via G_IMPLEMENT_INTERFACE().
 *
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.70
 */
#define G_DEFINE_FINAL_TYPE_WITH_CODE(TN, t_n, T_P, _C_)       _G_DEFINE_TYPE_EXTENDED_BEGIN (TN, t_n, T_P, XTYPE_FLAG_FINAL) {_C_;} _G_DEFINE_TYPE_EXTENDED_END() XPL_AVAILABLE_MACRO_IN_2_70
/**
 * G_DEFINE_FINAL_TYPE_WITH_PRIVATE:
 * @TN: the name of the new type, in Camel case
 * @t_n: the name of the new type, in lower case, with words
 *   separated by `_` (snake case)
 * @T_P: the #xtype_t of the parent type
 *
 * A convenience macro for type implementations.
 *
 * Similar to G_DEFINE_TYPE_WITH_PRIVATE(), but defines a final type.
 *
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Since: 2.70
 */
#define G_DEFINE_FINAL_TYPE_WITH_PRIVATE(TN, t_n, T_P)         G_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, XTYPE_FLAG_FINAL, G_ADD_PRIVATE (TN)) XPL_AVAILABLE_MACRO_IN_2_70
/**
 * G_DEFINE_TYPE_EXTENDED:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words
 *    separated by `_`.
 * @T_P: The #xtype_t of the parent type.
 * @_f_: #xtype_flags_t to pass to xtype_register_static()
 * @_C_: Custom code that gets inserted in the `*_get_type()` function.
 *
 * The most general convenience macro for type implementations, on which
 * G_DEFINE_TYPE(), etc are based.
 *
 * |[<!-- language="C" -->
 * G_DEFINE_TYPE_EXTENDED (GtkGadget,
 *                         gtk_gadget,
 *                         GTK_TYPE_WIDGET,
 *                         0,
 *                         G_ADD_PRIVATE (GtkGadget)
 *                         G_IMPLEMENT_INTERFACE (TYPE_GIZMO,
 *                                                gtk_gadget_gizmo_init));
 * ]|
 *
 * expands to
 *
 * |[<!-- language="C" -->
 * static void     gtk_gadget_init       (GtkGadget      *self);
 * static void     gtk_gadget_class_init (GtkGadgetClass *klass);
 * static xpointer_t gtk_gadget_parent_class = NULL;
 * static xint_t     GtkGadget_private_offset;
 * static void     gtk_gadget_class_intern_init (xpointer_t klass)
 * {
 *   gtk_gadget_parent_class = xtype_class_peek_parent (klass);
 *   if (GtkGadget_private_offset != 0)
 *     xtype_class_adjust_private_offset (klass, &GtkGadget_private_offset);
 *   gtk_gadget_class_init ((GtkGadgetClass*) klass);
 * }
 * static inline xpointer_t gtk_gadget_get_instance_private (GtkGadget *self)
 * {
 *   return (G_STRUCT_MEMBER_P (self, GtkGadget_private_offset));
 * }
 *
 * xtype_t
 * gtk_gadget_get_type (void)
 * {
 *   static xsize_t static_g_define_type_id = 0;
 *   if (g_once_init_enter (&static_g_define_type_id))
 *     {
 *       xtype_t g_define_type_id =
 *         xtype_register_static_simple (GTK_TYPE_WIDGET,
 *                                        g_intern_static_string ("GtkGadget"),
 *                                        sizeof (GtkGadgetClass),
 *                                        (xclass_init_func_t) gtk_gadget_class_intern_init,
 *                                        sizeof (GtkGadget),
 *                                        (xinstance_init_func_t) gtk_gadget_init,
 *                                        0);
 *       {
 *         GtkGadget_private_offset =
 *           xtype_add_instance_private (g_define_type_id, sizeof (GtkGadgetPrivate));
 *       }
 *       {
 *         const xinterface_info_t g_implement_interface_info = {
 *           (GInterfaceInitFunc) gtk_gadget_gizmo_init
 *         };
 *         xtype_add_interface_static (g_define_type_id, TYPE_GIZMO, &g_implement_interface_info);
 *       }
 *       g_once_init_leave (&static_g_define_type_id, g_define_type_id);
 *     }
 *   return static_g_define_type_id;
 * }
 * ]|
 *
 * The only pieces which have to be manually provided are the definitions of
 * the instance and class structure and the definitions of the instance and
 * class init functions.
 *
 * Since: 2.4
 */
#define G_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, _f_, _C_)	    _G_DEFINE_TYPE_EXTENDED_BEGIN (TN, t_n, T_P, _f_) {_C_;} _G_DEFINE_TYPE_EXTENDED_END()

/**
 * G_DEFINE_INTERFACE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words separated by `_`.
 * @T_P: The #xtype_t of the prerequisite type for the interface, or %XTYPE_INVALID
 * for no prerequisite type.
 *
 * A convenience macro for #xtype_interface_t definitions, which declares
 * a default vtable initialization function and defines a `*_get_type()`
 * function.
 *
 * The macro expects the interface initialization function to have the
 * name `t_n ## _default_init`, and the interface structure to have the
 * name `TN ## Interface`.
 *
 * The initialization function has signature
 * `static void t_n ## _default_init (TypeName##_interface_t *klass);`, rather than
 * the full #GInterfaceInitFunc signature, for brevity and convenience. If you
 * need to use an initialization function with an `iface_data` argument, you
 * must write the #xtype_interface_t definitions manually.
 *
 * Since: 2.24
 */
#define G_DEFINE_INTERFACE(TNxT, t_n, T_P)		    G_DEFINE_INTERFACE_WITH_CODE(TNxT, t_n, T_P, ;)

/**
 * G_DEFINE_INTERFACE_WITH_CODE:
 * @TN: The name of the new type, in Camel case.
 * @t_n: The name of the new type, in lowercase, with words separated by `_`.
 * @T_P: The #xtype_t of the prerequisite type for the interface, or %XTYPE_INVALID
 * for no prerequisite type.
 * @_C_: Custom code that gets inserted in the `*_get_type()` function.
 *
 * A convenience macro for #xtype_interface_t definitions.
 *
 * Similar to G_DEFINE_INTERFACE(), but allows you to insert custom code
 * into the `*_get_type()` function, e.g. additional interface implementations
 * via G_IMPLEMENT_INTERFACE(), or additional prerequisite types.
 *
 * See G_DEFINE_TYPE_EXTENDED() for a similar example using
 * G_DEFINE_TYPE_WITH_CODE().
 *
 * Since: 2.24
 */
#define G_DEFINE_INTERFACE_WITH_CODE(TNxT, t_n, T_P, _C_)     _G_DEFINE_INTERFACE_EXTENDED_BEGIN(TNxT, t_n, T_P) {_C_;} _G_DEFINE_INTERFACE_EXTENDED_END()

/**
 * G_IMPLEMENT_INTERFACE:
 * @TYPE_IFACE: The #xtype_t of the interface to add
 * @iface_init: (type GInterfaceInitFunc): The interface init function, of type #GInterfaceInitFunc
 *
 * A convenience macro to ease interface addition in the `_C_` section
 * of G_DEFINE_TYPE_WITH_CODE() or G_DEFINE_ABSTRACT_TYPE_WITH_CODE().
 * See G_DEFINE_TYPE_EXTENDED() for an example.
 *
 * Note that this macro can only be used together with the `G_DEFINE_TYPE_*`
 * macros, since it depends on variable names from those macros.
 *
 * Since: 2.4
 */
#define G_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init)       { \
  const xinterface_info_t g_implement_interface_info = { \
    (GInterfaceInitFunc)(void (*)(void)) iface_init, NULL, NULL \
  }; \
  xtype_add_interface_static (g_define_type_id, TYPE_IFACE, &g_implement_interface_info); \
}

/**
 * G_ADD_PRIVATE:
 * @TypeName: the name of the type in CamelCase
 *
 * A convenience macro to ease adding private data to instances of a new type
 * in the @_C_ section of G_DEFINE_TYPE_WITH_CODE() or
 * G_DEFINE_ABSTRACT_TYPE_WITH_CODE().
 *
 * For instance:
 *
 * |[<!-- language="C" -->
 *   typedef struct _xobject xobject_t;
 *   typedef struct _xobject_class xobject_class_t;
 *
 *   typedef struct {
 *     xint_t foo;
 *     xint_t bar;
 *   } xobject_private_t;
 *
 *   G_DEFINE_TYPE_WITH_CODE (xobject_t, my_object, XTYPE_OBJECT,
 *                            G_ADD_PRIVATE (xobject_t))
 * ]|
 *
 * Will add `xobject_private_t` as the private data to any instance of the
 * `xobject_t` type.
 *
 * `G_DEFINE_TYPE_*` macros will automatically create a private function
 * based on the arguments to this macro, which can be used to safely
 * retrieve the private data from an instance of the type; for instance:
 *
 * |[<!-- language="C" -->
 *   xint_t
 *   my_object_get_foo (xobject_t *obj)
 *   {
 *     xobject_private_t *priv = my_object_get_instance_private (obj);
 *
 *     g_return_val_if_fail (MY_IS_OBJECT (obj), 0);
 *
 *     return priv->foo;
 *   }
 *
 *   void
 *   my_object_set_bar (xobject_t *obj,
 *                      xint_t      bar)
 *   {
 *     xobject_private_t *priv = my_object_get_instance_private (obj);
 *
 *     g_return_if_fail (MY_IS_OBJECT (obj));
 *
 *     if (priv->bar != bar)
 *       priv->bar = bar;
 *   }
 * ]|
 *
 * Since GLib 2.72, the returned `xobject_private_t` pointer is guaranteed to be
 * aligned to at least the alignment of the largest basic GLib type (typically
 * this is #xuint64_t or #xdouble_t). If you need larger alignment for an element in
 * the struct, you should allocate it on the heap (aligned), or arrange for your
 * `xobject_private_t` struct to be appropriately padded.
 *
 * Note that this macro can only be used together with the `G_DEFINE_TYPE_*`
 * macros, since it depends on variable names from those macros.
 *
 * Also note that private structs added with these macros must have a struct
 * name of the form `TypeNamePrivate`.
 *
 * It is safe to call the `_get_instance_private` function on %NULL or invalid
 * objects since it's only adding an offset to the instance pointer. In that
 * case the returned pointer must not be dereferenced.
 *
 * Since: 2.38
 */
#define G_ADD_PRIVATE(TypeName) { \
  TypeName##_private_offset = \
    xtype_add_instance_private (g_define_type_id, sizeof (TypeName##_private_t)); \
}

/**
 * G_PRIVATE_OFFSET:
 * @TypeName: the name of the type in CamelCase
 * @field: the name of the field in the private data structure
 *
 * Evaluates to the offset of the @field inside the instance private data
 * structure for @TypeName.
 *
 * Note that this macro can only be used together with the `G_DEFINE_TYPE_*`
 * and G_ADD_PRIVATE() macros, since it depends on variable names from
 * those macros.
 *
 * Since: 2.38
 */
#define G_PRIVATE_OFFSET(TypeName, field) \
  (TypeName##_private_offset + (G_STRUCT_OFFSET (TypeName##Private, field)))

/**
 * G_PRIVATE_FIELD_P:
 * @TypeName: the name of the type in CamelCase
 * @inst: the instance of @TypeName you wish to access
 * @field_name: the name of the field in the private data structure
 *
 * Evaluates to a pointer to the @field_name inside the @inst private data
 * structure for @TypeName.
 *
 * Note that this macro can only be used together with the `G_DEFINE_TYPE_*`
 * and G_ADD_PRIVATE() macros, since it depends on variable names from
 * those macros.
 *
 * Since: 2.38
 */
#define G_PRIVATE_FIELD_P(TypeName, inst, field_name) \
  G_STRUCT_MEMBER_P (inst, G_PRIVATE_OFFSET (TypeName, field_name))

/**
 * G_PRIVATE_FIELD:
 * @TypeName: the name of the type in CamelCase
 * @inst: the instance of @TypeName you wish to access
 * @field_type: the type of the field in the private data structure
 * @field_name: the name of the field in the private data structure
 *
 * Evaluates to the @field_name inside the @inst private data
 * structure for @TypeName.
 *
 * Note that this macro can only be used together with the `G_DEFINE_TYPE_*`
 * and G_ADD_PRIVATE() macros, since it depends on variable names from
 * those macros.
 *
 * Since: 2.38
 */
#define G_PRIVATE_FIELD(TypeName, inst, field_type, field_name) \
  G_STRUCT_MEMBER (field_type, inst, G_PRIVATE_OFFSET (TypeName, field_name))

/* we need to have this macro under conditional expansion, as it references
 * a function that has been added in 2.38. see bug:
 * https://bugzilla.gnome.org/show_bug.cgi?id=703191
 */
#if XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_38
#define _G_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name) \
static void     type_name##_class_intern_init (xpointer_t klass) \
{ \
  type_name##_parent_class = xtype_class_peek_parent (klass); \
  if (TypeName##_private_offset != 0) \
    xtype_class_adjust_private_offset (klass, &TypeName##_private_offset); \
  type_name##_class_init ((TypeName##_class_t*) klass); \
}

#else
#define _G_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name) \
static void     type_name##_class_intern_init (xpointer_t klass) \
{ \
  type_name##_parent_class = xtype_class_peek_parent (klass); \
  type_name##_class_init ((TypeName##_class_t*) klass); \
}
#endif /* XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_38 */

/* Added for _G_DEFINE_TYPE_EXTENDED_WITH_PRELUDE */
#define _G_DEFINE_TYPE_EXTENDED_BEGIN_PRE(TypeName, type_name, TYPE_PARENT) \
\
static void     type_name##_init              (TypeName##_t        *self); \
static void     type_name##_class_init        (TypeName##_class_t *klass); \
static xtype_t    type_name##_get_type_once     (void); \
static xpointer_t type_name##_parent_class = NULL; \
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
  static xsize_t static_g_define_type_id = 0;
  /* Prelude goes here */

/* Added for _G_DEFINE_TYPE_EXTENDED_WITH_PRELUDE */
#define _G_DEFINE_TYPE_EXTENDED_BEGIN_REGISTER(TypeName, type_name, TYPE_PARENT, flags) \
  if (g_once_init_enter (&static_g_define_type_id)) \
    { \
      xtype_t g_define_type_id = type_name##_get_type_once (); \
      g_once_init_leave (&static_g_define_type_id, g_define_type_id); \
    }					\
  return static_g_define_type_id; \
} /* closes type_name##_get_type() */ \
\
G_GNUC_NO_INLINE \
static xtype_t \
type_name##_get_type_once (void) \
{ \
  xtype_t g_define_type_id = \
        xtype_register_static_simple (TYPE_PARENT, \
                                       g_intern_static_string (#TypeName), \
                                       sizeof (TypeName##_class_t), \
                                       (xclass_init_func_t)(void (*)(void)) type_name##_class_intern_init, \
                                       sizeof (TypeName##_t), \
                                       (xinstance_init_func_t)(void (*)(void)) type_name##_init, \
                                       (xtype_flags_t) flags); \
    { /* custom code follows */
#define _G_DEFINE_TYPE_EXTENDED_END()	\
      /* following custom code */	\
    }					\
  return g_define_type_id; \
} /* closes type_name##_get_type_once() */

/* This was defined before we had G_DEFINE_TYPE_WITH_CODE_AND_PRELUDE, it's simplest
 * to keep it.
 */
#define _G_DEFINE_TYPE_EXTENDED_BEGIN(TypeNameNoT, type_name, TYPE_PARENT, flags) \
  _G_DEFINE_TYPE_EXTENDED_BEGIN_PRE(TypeNameNoT, type_name, TYPE_PARENT) \
  _G_DEFINE_TYPE_EXTENDED_BEGIN_REGISTER(TypeNameNoT, type_name, TYPE_PARENT, flags) \

#define _G_DEFINE_INTERFACE_EXTENDED_BEGIN(TypeNameNoT, type_name, TYPE_PREREQ) \
\
static void     type_name##_default_init        (TypeNameNoT##_interface_t *klass); \
\
xtype_t \
type_name##_get_type (void) \
{ \
  static xsize_t static_g_define_type_id = 0; \
  if (g_once_init_enter (&static_g_define_type_id)) \
    { \
      xtype_t g_define_type_id = \
        xtype_register_static_simple (XTYPE_INTERFACE, \
                                       g_intern_static_string (#TypeNameNoT), \
                                       sizeof (TypeNameNoT##_interface_t), \
                                       (xclass_init_func_t)(void (*)(void)) type_name##_default_init, \
                                       0, \
                                       (xinstance_init_func_t)NULL, \
                                       (xtype_flags_t) 0); \
      if (TYPE_PREREQ != XTYPE_INVALID) \
        xtype_interface_add_prerequisite (g_define_type_id, TYPE_PREREQ); \
      { /* custom code follows */
#define _G_DEFINE_INTERFACE_EXTENDED_END()	\
        /* following custom code */		\
      }						\
      g_once_init_leave (&static_g_define_type_id, g_define_type_id); \
    }						\
  return static_g_define_type_id; \
} /* closes type_name##_get_type() */

/**
 * G_DEFINE_BOXED_TYPE:
 * @TypeName: The name of the new type, in Camel case
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by `_`
 * @copy_func: the #GBoxedCopyFunc for the new type
 * @free_func: the #GBoxedFreeFunc for the new type
 *
 * A convenience macro for defining a new custom boxed type.
 *
 * Using this macro is the recommended way of defining new custom boxed
 * types, over calling xboxed_type_register_static() directly. It defines
 * a `type_name_get_type()` function which will return the newly defined
 * #xtype_t, enabling lazy instantiation.
 *
 * |[<!-- language="C" -->
 * G_DEFINE_BOXED_TYPE (MyStruct, my_struct, my_struct_copy, my_struct_free)
 *
 * void
 * foo ()
 * {
 *   xtype_t type = my_struct_get_type ();
 *   // ... your code ...
 * }
 * ]|
 *
 * Since: 2.26
 */
#define G_DEFINE_BOXED_TYPE(TypeName, type_name, copy_func, free_func) G_DEFINE_BOXED_TYPE_WITH_CODE (TypeName, type_name, copy_func, free_func, {})
/**
 * G_DEFINE_BOXED_TYPE_WITH_CODE:
 * @TypeName: The name of the new type, in Camel case
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by `_`
 * @copy_func: the #GBoxedCopyFunc for the new type
 * @free_func: the #GBoxedFreeFunc for the new type
 * @_C_: Custom code that gets inserted in the `*_get_type()` function
 *
 * A convenience macro for boxed type implementations.
 *
 * Similar to G_DEFINE_BOXED_TYPE(), but allows to insert custom code into the
 * `type_name_get_type()` function, e.g. to register value transformations with
 * xvalue_register_transform_func(), for instance:
 *
 * |[<!-- language="C" -->
 * G_DEFINE_BOXED_TYPE_WITH_CODE (GdkRectangle, gdk_rectangle,
 *                                gdk_rectangle_copy,
 *                                gdk_rectangle_free,
 *                                register_rectangle_transform_funcs (g_define_type_id))
 * ]|
 *
 * Similarly to the `G_DEFINE_TYPE_*` family of macros, the #xtype_t of the newly
 * defined boxed type is exposed in the `g_define_type_id` variable.
 *
 * Since: 2.26
 */
#define G_DEFINE_BOXED_TYPE_WITH_CODE(TypeName, type_name, copy_func, free_func, _C_) _G_DEFINE_BOXED_TYPE_BEGIN (TypeName, type_name, copy_func, free_func) {_C_;} _G_DEFINE_TYPE_EXTENDED_END()

/* Only use this in non-C++ on GCC >= 2.7, except for Darwin/ppc64.
 * See https://bugzilla.gnome.org/show_bug.cgi?id=647145
 */
#if !defined (__cplusplus) && (G_GNUC_CHECK_VERSION(2, 7)) && !(defined (__APPLE__) && defined (__ppc64__))
#define _G_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func) \
static xtype_t type_name##_get_type_once (void); \
\
xtype_t \
type_name##_get_type (void) \
{ \
  static xsize_t static_g_define_type_id = 0; \
  if (g_once_init_enter (&static_g_define_type_id)) \
    { \
      xtype_t g_define_type_id = type_name##_get_type_once (); \
      g_once_init_leave (&static_g_define_type_id, g_define_type_id); \
    } \
  return static_g_define_type_id; \
} \
\
G_GNUC_NO_INLINE \
static xtype_t \
type_name##_get_type_once (void) \
{ \
  xtype_t (* _g_register_boxed) \
    (const xchar_t *, \
     union \
       { \
         TypeName * (*do_copy_type) (TypeName *); \
         TypeName * (*do_const_copy_type) (const TypeName *); \
         GBoxedCopyFunc do_copy_boxed; \
       } __attribute__((__transparent_union__)), \
     union \
       { \
         void (* do_free_type) (TypeName *); \
         GBoxedFreeFunc do_free_boxed; \
       } __attribute__((__transparent_union__)) \
    ) = xboxed_type_register_static; \
  xtype_t g_define_type_id = \
    _g_register_boxed (g_intern_static_string (#TypeName), copy_func, free_func); \
  { /* custom code follows */
#else
#define _G_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func) \
static xtype_t type_name##_get_type_once (void); \
\
xtype_t \
type_name##_get_type (void) \
{ \
  static xsize_t static_g_define_type_id = 0; \
  if (g_once_init_enter (&static_g_define_type_id)) \
    { \
      xtype_t g_define_type_id = type_name##_get_type_once (); \
      g_once_init_leave (&static_g_define_type_id, g_define_type_id); \
    } \
  return static_g_define_type_id; \
} \
\
G_GNUC_NO_INLINE \
static xtype_t \
type_name##_get_type_once (void) \
{ \
  xtype_t g_define_type_id = \
    xboxed_type_register_static (g_intern_static_string (#TypeName), \
                                  (GBoxedCopyFunc) copy_func, \
                                  (GBoxedFreeFunc) free_func); \
  { /* custom code follows */
#endif /* __GNUC__ */

/**
 * G_DEFINE_POINTER_TYPE:
 * @TypeName: The name of the new type, in Camel case
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by `_`
 *
 * A convenience macro for pointer type implementations, which defines a
 * `type_name_get_type()` function registering the pointer type.
 *
 * Since: 2.26
 */
#define G_DEFINE_POINTER_TYPE(TypeName, type_name) G_DEFINE_POINTER_TYPE_WITH_CODE (TypeName, type_name, {})
/**
 * G_DEFINE_POINTER_TYPE_WITH_CODE:
 * @TypeName: The name of the new type, in Camel case
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by `_`
 * @_C_: Custom code that gets inserted in the `*_get_type()` function
 *
 * A convenience macro for pointer type implementations.
 * Similar to G_DEFINE_POINTER_TYPE(), but allows to insert
 * custom code into the `type_name_get_type()` function.
 *
 * Since: 2.26
 */
#define G_DEFINE_POINTER_TYPE_WITH_CODE(TypeName, type_name, _C_) _G_DEFINE_POINTER_TYPE_BEGIN (TypeName, type_name) {_C_;} _G_DEFINE_TYPE_EXTENDED_END()

#define _G_DEFINE_POINTER_TYPE_BEGIN(TypeName, type_name) \
static xtype_t type_name##_get_type_once (void); \
\
xtype_t \
type_name##_get_type (void) \
{ \
  static xsize_t static_g_define_type_id = 0; \
  if (g_once_init_enter (&static_g_define_type_id)) \
    { \
      xtype_t g_define_type_id = type_name##_get_type_once (); \
      g_once_init_leave (&static_g_define_type_id, g_define_type_id); \
    } \
  return static_g_define_type_id; \
} \
\
G_GNUC_NO_INLINE \
static xtype_t \
type_name##_get_type_once (void) \
{ \
  xtype_t g_define_type_id = \
    g_pointer_type_register_static (g_intern_static_string (#TypeName)); \
  { /* custom code follows */

/* --- protected (for fundamental type implementations) --- */
XPL_AVAILABLE_IN_ALL
GTypePlugin*	 xtype_get_plugin		(xtype_t		     type);
XPL_AVAILABLE_IN_ALL
GTypePlugin*	 xtype_interface_get_plugin	(xtype_t		     instance_type,
						 xtype_t               interface_type);
XPL_AVAILABLE_IN_ALL
xtype_t		 xtype_fundamental_next	(void);
XPL_AVAILABLE_IN_ALL
xtype_t		 xtype_fundamental		(xtype_t		     type_id);
XPL_AVAILABLE_IN_ALL
GTypeInstance*   xtype_create_instance         (xtype_t               type);
XPL_AVAILABLE_IN_ALL
void             xtype_free_instance           (GTypeInstance      *instance);

XPL_AVAILABLE_IN_ALL
void		 xtype_add_class_cache_func    (xpointer_t	     cache_data,
						 GTypeClassCacheFunc cache_func);
XPL_AVAILABLE_IN_ALL
void		 xtype_remove_class_cache_func (xpointer_t	     cache_data,
						 GTypeClassCacheFunc cache_func);
XPL_AVAILABLE_IN_ALL
void             xtype_class_unref_uncached    (xpointer_t            g_class);

XPL_AVAILABLE_IN_ALL
void             xtype_add_interface_check     (xpointer_t	         check_data,
						 GTypeInterfaceCheckFunc check_func);
XPL_AVAILABLE_IN_ALL
void             xtype_remove_interface_check  (xpointer_t	         check_data,
						 GTypeInterfaceCheckFunc check_func);

XPL_AVAILABLE_IN_ALL
xtype_value_table_t* xtype_value_table_peek        (xtype_t		     type);


/*< private >*/
XPL_AVAILABLE_IN_ALL
xboolean_t	 xtype_check_instance          (GTypeInstance      *instance) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
GTypeInstance*   xtype_check_instance_cast     (GTypeInstance      *instance,
						 xtype_t               iface_type);
XPL_AVAILABLE_IN_ALL
xboolean_t         xtype_check_instance_is_a	(GTypeInstance      *instance,
						 xtype_t               iface_type) G_GNUC_PURE;
XPL_AVAILABLE_IN_2_42
xboolean_t         xtype_check_instance_is_fundamentally_a (GTypeInstance *instance,
                                                           xtype_t          fundamental_type) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xtype_class_t*      xtype_check_class_cast        (xtype_class_t         *g_class,
						 xtype_t               is_a_type);
XPL_AVAILABLE_IN_ALL
xboolean_t         xtype_check_class_is_a        (xtype_class_t         *g_class,
						 xtype_t               is_a_type) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xboolean_t	 xtype_check_is_value_type     (xtype_t		     type) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t	 xtype_check_value             (const xvalue_t       *value) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xboolean_t	 xtype_check_value_holds	(const xvalue_t	    *value,
						 xtype_t		     type) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xboolean_t         xtype_test_flags              (xtype_t               type,
						 xuint_t               flags) G_GNUC_CONST;


/* --- debugging functions --- */
XPL_AVAILABLE_IN_ALL
const xchar_t *    xtype_name_from_instance      (GTypeInstance	*instance);
XPL_AVAILABLE_IN_ALL
const xchar_t *    xtype_name_from_class         (xtype_class_t	*g_class);


/* --- implementation bits --- */
#ifndef G_DISABLE_CAST_CHECKS
#  define _XTYPE_CIC(ip, gt, ct) \
    ((ct*) (void *) xtype_check_instance_cast ((GTypeInstance*) ip, gt))
#  define _XTYPE_CCC(cp, gt, ct) \
    ((ct*) (void *) xtype_check_class_cast ((xtype_class_t*) cp, gt))
#else /* G_DISABLE_CAST_CHECKS */
#  define _XTYPE_CIC(ip, gt, ct)       ((ct*) ip)
#  define _XTYPE_CCC(cp, gt, ct)       ((ct*) cp)
#endif /* G_DISABLE_CAST_CHECKS */
#define _XTYPE_CHI(ip)			(xtype_check_instance ((GTypeInstance*) ip))
#define _XTYPE_CHV(vl)			(xtype_check_value ((xvalue_t*) vl))
#define _XTYPE_IGC(ip, gt, ct)         ((ct*) (((GTypeInstance*) ip)->g_class))
#define _XTYPE_IGI(ip, gt, ct)         ((ct*) xtype_interface_peek (((GTypeInstance*) ip)->g_class, gt))
#define _XTYPE_CIFT(ip, ft)            (xtype_check_instance_is_fundamentally_a ((GTypeInstance*) ip, ft))
#ifdef	__GNUC__
#  define _XTYPE_CIT(ip, gt)             (G_GNUC_EXTENSION ({ \
  GTypeInstance *__inst = (GTypeInstance*) ip; xtype_t __t = gt; xboolean_t __r; \
  if (!__inst) \
    __r = FALSE; \
  else if (__inst->g_class && __inst->g_class->g_type == __t) \
    __r = TRUE; \
  else \
    __r = xtype_check_instance_is_a (__inst, __t); \
  __r; \
}))
#  define _XTYPE_CCT(cp, gt)             (G_GNUC_EXTENSION ({ \
  xtype_class_t *__class = (xtype_class_t*) cp; xtype_t __t = gt; xboolean_t __r; \
  if (!__class) \
    __r = FALSE; \
  else if (__class->g_type == __t) \
    __r = TRUE; \
  else \
    __r = xtype_check_class_is_a (__class, __t); \
  __r; \
}))
#  define _XTYPE_CVH(vl, gt)             (G_GNUC_EXTENSION ({ \
  const xvalue_t *__val = (const xvalue_t*) vl; xtype_t __t = gt; xboolean_t __r; \
  if (!__val) \
    __r = FALSE; \
  else if (__val->g_type == __t)		\
    __r = TRUE; \
  else \
    __r = xtype_check_value_holds (__val, __t); \
  __r; \
}))
#else  /* !__GNUC__ */
#  define _XTYPE_CIT(ip, gt)             (xtype_check_instance_is_a ((GTypeInstance*) ip, gt))
#  define _XTYPE_CCT(cp, gt)             (xtype_check_class_is_a ((xtype_class_t*) cp, gt))
#  define _XTYPE_CVH(vl, gt)             (xtype_check_value_holds ((const xvalue_t*) vl, gt))
#endif /* !__GNUC__ */
/**
 * XTYPE_FLAG_RESERVED_ID_BIT:
 *
 * A bit in the type number that's supposed to be left untouched.
 */
#define	XTYPE_FLAG_RESERVED_ID_BIT	((xtype_t) (1 << 0))

G_END_DECLS

#endif /* __XTYPE_H__ */
