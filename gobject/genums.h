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
 * XENUM_CLASS:
 * @class: a valid #xenum_class_t
 *
 * Casts a derived #xenum_class_t structure into a #xenum_class_t structure.
 */
#define XENUM_CLASS(class)	       (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_ENUM, xenum_class_t))
/**
 * X_IS_ENUM_CLASS:
 * @class: a #xenum_class_t
 *
 * Checks whether @class "is a" valid #xenum_class_t structure of type %XTYPE_ENUM
 * or derived.
 */
#define X_IS_ENUM_CLASS(class)	       (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_ENUM))
/**
 * XENUM_CLASS_TYPE:
 * @class: a #xenum_class_t
 *
 * Get the type identifier from a given #xenum_class_t structure.
 *
 * Returns: the #xtype_t
 */
#define XENUM_CLASS_TYPE(class)       (XTYPE_FROM_CLASS (class))
/**
 * XENUM_CLASS_TYPE_NAME:
 * @class: a #xenum_class_t
 *
 * Get the static type name from a given #xenum_class_t structure.
 *
 * Returns: the type name.
 */
#define XENUM_CLASS_TYPE_NAME(class)  (xtype_name (XENUM_CLASS_TYPE (class)))

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
 * XFLAGS_CLASS:
 * @class: a valid #xflags_class_t
 *
 * Casts a derived #xflags_class_t structure into a #xflags_class_t structure.
 */
#define XFLAGS_CLASS(class)	       (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_FLAGS, xflags_class_t))
/**
 * X_IS_FLAGS_CLASS:
 * @class: a #xflags_class_t
 *
 * Checks whether @class "is a" valid #xflags_class_t structure of type %XTYPE_FLAGS
 * or derived.
 */
#define X_IS_FLAGS_CLASS(class)        (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_FLAGS))
/**
 * XFLAGS_CLASS_TYPE:
 * @class: a #xflags_class_t
 *
 * Get the type identifier from a given #xflags_class_t structure.
 *
 * Returns: the #xtype_t
 */
#define XFLAGS_CLASS_TYPE(class)      (XTYPE_FROM_CLASS (class))
/**
 * XFLAGS_CLASS_TYPE_NAME:
 * @class: a #xflags_class_t
 *
 * Get the static type name from a given #xflags_class_t structure.
 *
 * Returns: the type name.
 */
#define XFLAGS_CLASS_TYPE_NAME(class) (xtype_name (XFLAGS_CLASS_TYPE (class)))


/**
 * G_VALUE_HOLDS_ENUM:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values derived from type %XTYPE_ENUM.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_ENUM(value)      (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_ENUM))
/**
 * G_VALUE_HOLDS_FLAGS:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values derived from type %XTYPE_FLAGS.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_FLAGS(value)     (XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_FLAGS))


/* --- enum/flag values & classes --- */
typedef struct _xenum_class  xenum_class_t;
typedef struct _xflags_class xflags_class_t;
typedef struct _xenum_value  xenum_value_t;
typedef struct _xflags_value xflags_value_t;

/**
 * xenum_class_t:
 * @xtype_class: the parent class
 * @minimum: the smallest possible value.
 * @maximum: the largest possible value.
 * @n_values: the number of possible values.
 * @values: an array of #xenum_value_t structs describing the
 *  individual values.
 *
 * The class of an enumeration type holds information about its
 * possible values.
 */
struct	_xenum_class
{
  xtype_class_t  xtype_class;

  /*< public >*/
  xint_t	      minimum;
  xint_t	      maximum;
  xuint_t	      n_values;
  xenum_value_t *values;
};
/**
 * xflags_class_t:
 * @xtype_class: the parent class
 * @mask: a mask covering all possible values.
 * @n_values: the number of possible values.
 * @values: an array of #xflags_value_t structs describing the
 *  individual values.
 *
 * The class of a flags type holds information about its
 * possible values.
 */
struct	_xflags_class
{
  xtype_class_t   xtype_class;

  /*< public >*/
  xuint_t	       mask;
  xuint_t	       n_values;
  xflags_value_t *values;
};
/**
 * xenum_value_t:
 * @value: the enum value
 * @value_name: the name of the value
 * @value_nick: the nickname of the value
 *
 * A structure which contains a single enum value, its name, and its
 * nickname.
 */
struct _xenum_value
{
  xint_t	 value;
  const xchar_t *value_name;
  const xchar_t *value_nick;
};
/**
 * xflags_value_t:
 * @value: the flags value
 * @value_name: the name of the value
 * @value_nick: the nickname of the value
 *
 * A structure which contains a single flags value, its name, and its
 * nickname.
 */
struct _xflags_value
{
  xuint_t	 value;
  const xchar_t *value_name;
  const xchar_t *value_nick;
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xenum_value_t*	xenum_get_value		(xenum_class_t	*enum_class,
						 xint_t		 value);
XPL_AVAILABLE_IN_ALL
xenum_value_t*	xenum_get_value_by_name	(xenum_class_t	*enum_class,
						 const xchar_t	*name);
XPL_AVAILABLE_IN_ALL
xenum_value_t*	xenum_get_value_by_nick	(xenum_class_t	*enum_class,
						 const xchar_t	*nick);
XPL_AVAILABLE_IN_ALL
xflags_value_t*	xflags_get_first_value		(xflags_class_t	*flags_class,
						 xuint_t		 value);
XPL_AVAILABLE_IN_ALL
xflags_value_t*	xflags_get_value_by_name	(xflags_class_t	*flags_class,
						 const xchar_t	*name);
XPL_AVAILABLE_IN_ALL
xflags_value_t*	xflags_get_value_by_nick	(xflags_class_t	*flags_class,
						 const xchar_t	*nick);
XPL_AVAILABLE_IN_2_54
xchar_t          *xenum_to_string                (xtype_t           xenum_type,
                                                 xint_t            value);
XPL_AVAILABLE_IN_2_54
xchar_t          *xflags_to_string               (xtype_t           flags_type,
                                                 xuint_t           value);
XPL_AVAILABLE_IN_ALL
void            xvalue_set_enum        	(xvalue_t         *value,
						 xint_t            v_enum);
XPL_AVAILABLE_IN_ALL
xint_t            xvalue_get_enum        	(const xvalue_t   *value);
XPL_AVAILABLE_IN_ALL
void            xvalue_set_flags       	(xvalue_t         *value,
						 xuint_t           v_flags);
XPL_AVAILABLE_IN_ALL
xuint_t           xvalue_get_flags       	(const xvalue_t   *value);



/* --- registration functions --- */
/* const_static_values is a NULL terminated array of enum/flags
 * values that is taken over!
 */
XPL_AVAILABLE_IN_ALL
xtype_t	xenum_register_static	   (const xchar_t	      *name,
				    const xenum_value_t  *const_static_values);
XPL_AVAILABLE_IN_ALL
xtype_t	xflags_register_static	   (const xchar_t	      *name,
				    const xflags_value_t *const_static_values);
/* functions to complete the type information
 * for enums/flags implemented by plugins
 */
XPL_AVAILABLE_IN_ALL
void	xenum_complete_type_info  (xtype_t	       xenum_type,
				    xtype_info_t	      *info,
				    const xenum_value_t  *const_values);
XPL_AVAILABLE_IN_ALL
void	xflags_complete_type_info (xtype_t	       xflags_type,
				    xtype_info_t	      *info,
				    const xflags_value_t *const_values);

G_END_DECLS

#endif /* __G_ENUMS_H__ */
