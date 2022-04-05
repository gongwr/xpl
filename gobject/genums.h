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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __G_ENUMS_H__
#define __G_ENUMS_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include <gobject/gtype.h>

G_BEGIN_DECLS

/* --- type macros --- */
/**
 * XTYPE_IS_ENUM:
 * @type: a #xtype_t ID.
 *
 * Checks whether @type "is a" %XTYPE_ENUM.
 *
 * Returns: %TRUE if @type "is a" %XTYPE_ENUM.
 */
#define XTYPE_IS_ENUM(type)	       (XTYPE_FUNDAMENTAL (type) == XTYPE_ENUM)
/**
 * G_ENUM_CLASS:
 * @class: a valid #GEnumClass
 *
 * Casts a derived #GEnumClass structure into a #GEnumClass structure.
 */
#define G_ENUM_CLASS(class)	       (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_ENUM, GEnumClass))
/**
 * X_IS_ENUM_CLASS:
 * @class: a #GEnumClass
 *
 * Checks whether @class "is a" valid #GEnumClass structure of type %XTYPE_ENUM
 * or derived.
 */
#define X_IS_ENUM_CLASS(class)	       (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_ENUM))
/**
 * G_ENUM_CLASS_TYPE:
 * @class: a #GEnumClass
 *
 * Get the type identifier from a given #GEnumClass structure.
 *
 * Returns: the #xtype_t
 */
#define G_ENUM_CLASS_TYPE(class)       (XTYPE_FROM_CLASS (class))
/**
 * G_ENUM_CLASS_TYPE_NAME:
 * @class: a #GEnumClass
 *
 * Get the static type name from a given #GEnumClass structure.
 *
 * Returns: the type name.
 */
#define G_ENUM_CLASS_TYPE_NAME(class)  (g_type_name (G_ENUM_CLASS_TYPE (class)))


/**
 * XTYPE_IS_FLAGS:
 * @type: a #xtype_t ID.
 *
 * Checks whether @type "is a" %XTYPE_FLAGS.
 *
 * Returns: %TRUE if @type "is a" %XTYPE_FLAGS.
 */
#define XTYPE_IS_FLAGS(type)	       (XTYPE_FUNDAMENTAL (type) == XTYPE_FLAGS)
/**
 * G_FLAGS_CLASS:
 * @class: a valid #GFlagsClass
 *
 * Casts a derived #GFlagsClass structure into a #GFlagsClass structure.
 */
#define G_FLAGS_CLASS(class)	       (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_FLAGS, GFlagsClass))
/**
 * X_IS_FLAGS_CLASS:
 * @class: a #GFlagsClass
 *
 * Checks whether @class "is a" valid #GFlagsClass structure of type %XTYPE_FLAGS
 * or derived.
 */
#define X_IS_FLAGS_CLASS(class)        (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_FLAGS))
/**
 * G_FLAGS_CLASS_TYPE:
 * @class: a #GFlagsClass
 *
 * Get the type identifier from a given #GFlagsClass structure.
 *
 * Returns: the #xtype_t
 */
#define G_FLAGS_CLASS_TYPE(class)      (XTYPE_FROM_CLASS (class))
/**
 * G_FLAGS_CLASS_TYPE_NAME:
 * @class: a #GFlagsClass
 *
 * Get the static type name from a given #GFlagsClass structure.
 *
 * Returns: the type name.
 */
#define G_FLAGS_CLASS_TYPE_NAME(class) (g_type_name (G_FLAGS_CLASS_TYPE (class)))


/**
 * G_VALUE_HOLDS_ENUM:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values derived from type %XTYPE_ENUM.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_ENUM(value)      (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_ENUM))
/**
 * G_VALUE_HOLDS_FLAGS:
 * @value: a valid #GValue structure
 *
 * Checks whether the given #GValue can hold values derived from type %XTYPE_FLAGS.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_FLAGS(value)     (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_FLAGS))


/* --- enum/flag values & classes --- */
typedef struct _GEnumClass  GEnumClass;
typedef struct _GFlagsClass GFlagsClass;
typedef struct _GEnumValue  GEnumValue;
typedef struct _GFlagsValue GFlagsValue;

/**
 * GEnumClass:
 * @g_type_class: the parent class
 * @minimum: the smallest possible value.
 * @maximum: the largest possible value.
 * @n_values: the number of possible values.
 * @values: an array of #GEnumValue structs describing the
 *  individual values.
 *
 * The class of an enumeration type holds information about its
 * possible values.
 */
struct	_GEnumClass
{
  GTypeClass  g_type_class;

  /*< public >*/
  xint_t	      minimum;
  xint_t	      maximum;
  xuint_t	      n_values;
  GEnumValue *values;
};
/**
 * GFlagsClass:
 * @g_type_class: the parent class
 * @mask: a mask covering all possible values.
 * @n_values: the number of possible values.
 * @values: an array of #GFlagsValue structs describing the
 *  individual values.
 *
 * The class of a flags type holds information about its
 * possible values.
 */
struct	_GFlagsClass
{
  GTypeClass   g_type_class;

  /*< public >*/
  xuint_t	       mask;
  xuint_t	       n_values;
  GFlagsValue *values;
};
/**
 * GEnumValue:
 * @value: the enum value
 * @value_name: the name of the value
 * @value_nick: the nickname of the value
 *
 * A structure which contains a single enum value, its name, and its
 * nickname.
 */
struct _GEnumValue
{
  xint_t	 value;
  const xchar_t *value_name;
  const xchar_t *value_nick;
};
/**
 * GFlagsValue:
 * @value: the flags value
 * @value_name: the name of the value
 * @value_nick: the nickname of the value
 *
 * A structure which contains a single flags value, its name, and its
 * nickname.
 */
struct _GFlagsValue
{
  xuint_t	 value;
  const xchar_t *value_name;
  const xchar_t *value_nick;
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
GEnumValue*	g_enum_get_value		(GEnumClass	*enum_class,
						 xint_t		 value);
XPL_AVAILABLE_IN_ALL
GEnumValue*	g_enum_get_value_by_name	(GEnumClass	*enum_class,
						 const xchar_t	*name);
XPL_AVAILABLE_IN_ALL
GEnumValue*	g_enum_get_value_by_nick	(GEnumClass	*enum_class,
						 const xchar_t	*nick);
XPL_AVAILABLE_IN_ALL
GFlagsValue*	g_flags_get_first_value		(GFlagsClass	*flags_class,
						 xuint_t		 value);
XPL_AVAILABLE_IN_ALL
GFlagsValue*	g_flags_get_value_by_name	(GFlagsClass	*flags_class,
						 const xchar_t	*name);
XPL_AVAILABLE_IN_ALL
GFlagsValue*	g_flags_get_value_by_nick	(GFlagsClass	*flags_class,
						 const xchar_t	*nick);
XPL_AVAILABLE_IN_2_54
xchar_t          *g_enum_to_string                (xtype_t           g_enum_type,
                                                 xint_t            value);
XPL_AVAILABLE_IN_2_54
xchar_t          *g_flags_to_string               (xtype_t           flags_type,
                                                 xuint_t           value);
XPL_AVAILABLE_IN_ALL
void            g_value_set_enum        	(GValue         *value,
						 xint_t            v_enum);
XPL_AVAILABLE_IN_ALL
xint_t            g_value_get_enum        	(const GValue   *value);
XPL_AVAILABLE_IN_ALL
void            g_value_set_flags       	(GValue         *value,
						 xuint_t           v_flags);
XPL_AVAILABLE_IN_ALL
xuint_t           g_value_get_flags       	(const GValue   *value);



/* --- registration functions --- */
/* const_static_values is a NULL terminated array of enum/flags
 * values that is taken over!
 */
XPL_AVAILABLE_IN_ALL
xtype_t	g_enum_register_static	   (const xchar_t	      *name,
				    const GEnumValue  *const_static_values);
XPL_AVAILABLE_IN_ALL
xtype_t	g_flags_register_static	   (const xchar_t	      *name,
				    const GFlagsValue *const_static_values);
/* functions to complete the type information
 * for enums/flags implemented by plugins
 */
XPL_AVAILABLE_IN_ALL
void	g_enum_complete_type_info  (xtype_t	       g_enum_type,
				    GTypeInfo	      *info,
				    const GEnumValue  *const_values);
XPL_AVAILABLE_IN_ALL
void	g_flags_complete_type_info (xtype_t	       g_flags_type,
				    GTypeInfo	      *info,
				    const GFlagsValue *const_values);

G_END_DECLS

#endif /* __G_ENUMS_H__ */
