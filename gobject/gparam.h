/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
 *
 * gparam.h: xparam_spec_t base class implementation
 */
#ifndef __XPARAM_H__
#define __XPARAM_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gvalue.h>

G_BEGIN_DECLS

/* --- standard type macros --- */
/**
 * XTYPE_IS_PARAM:
 * @type: a #xtype_t ID
 *
 * Checks whether @type "is a" %XTYPE_PARAM.
 */
#define XTYPE_IS_PARAM(type)		(XTYPE_FUNDAMENTAL (type) == XTYPE_PARAM)
/**
 * XPARAM_SPEC:
 * @pspec: a valid #xparam_spec_t
 *
 * Casts a derived #xparam_spec_t object (e.g. of type #GParamSpecInt) into
 * a #xparam_spec_t object.
 */
#define XPARAM_SPEC(pspec)		(XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM, xparam_spec_t))
/**
 * X_IS_PARAM_SPEC:
 * @pspec: a #xparam_spec_t
 *
 * Checks whether @pspec "is a" valid #xparam_spec_t structure of type %XTYPE_PARAM
 * or derived.
 */
#if XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_42
#define X_IS_PARAM_SPEC(pspec)		(XTYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE ((pspec), XTYPE_PARAM))
#else
#define X_IS_PARAM_SPEC(pspec)		(XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM))
#endif
/**
 * XPARAM_SPEC_CLASS:
 * @pclass: a valid #GParamSpecClass
 *
 * Casts a derived #GParamSpecClass structure into a #GParamSpecClass structure.
 */
#define XPARAM_SPEC_CLASS(pclass)      (XTYPE_CHECK_CLASS_CAST ((pclass), XTYPE_PARAM, GParamSpecClass))
/**
 * X_IS_PARAM_SPEC_CLASS:
 * @pclass: a #GParamSpecClass
 *
 * Checks whether @pclass "is a" valid #GParamSpecClass structure of type
 * %XTYPE_PARAM or derived.
 */
#define X_IS_PARAM_SPEC_CLASS(pclass)   (XTYPE_CHECK_CLASS_TYPE ((pclass), XTYPE_PARAM))
/**
 * XPARAM_SPEC_GET_CLASS:
 * @pspec: a valid #xparam_spec_t
 *
 * Retrieves the #GParamSpecClass of a #xparam_spec_t.
 */
#define XPARAM_SPEC_GET_CLASS(pspec)	(XTYPE_INSTANCE_GET_CLASS ((pspec), XTYPE_PARAM, GParamSpecClass))


/* --- convenience macros --- */
/**
 * XPARAM_SPEC_TYPE:
 * @pspec: a valid #xparam_spec_t
 *
 * Retrieves the #xtype_t of this @pspec.
 */
#define XPARAM_SPEC_TYPE(pspec)	(XTYPE_FROM_INSTANCE (pspec))
/**
 * XPARAM_SPEC_TYPE_NAME:
 * @pspec: a valid #xparam_spec_t
 *
 * Retrieves the #xtype_t name of this @pspec.
 */
#define XPARAM_SPEC_TYPE_NAME(pspec)	(xtype_name (XPARAM_SPEC_TYPE (pspec)))
/**
 * XPARAM_SPEC_VALUE_TYPE:
 * @pspec: a valid #xparam_spec_t
 *
 * Retrieves the #xtype_t to initialize a #xvalue_t for this parameter.
 */
#define	XPARAM_SPEC_VALUE_TYPE(pspec)	(XPARAM_SPEC (pspec)->value_type)
/**
 * G_VALUE_HOLDS_PARAM:
 * @value: a valid #xvalue_t structure
 *
 * Checks whether the given #xvalue_t can hold values derived from type %XTYPE_PARAM.
 *
 * Returns: %TRUE on success.
 */
#define G_VALUE_HOLDS_PARAM(value)	(XTYPE_CHECK_VALUE_TYPE ((value), XTYPE_PARAM))


/* --- flags --- */
/**
 * GParamFlags:
 * @XPARAM_READABLE: the parameter is readable
 * @XPARAM_WRITABLE: the parameter is writable
 * @XPARAM_READWRITE: alias for %XPARAM_READABLE | %XPARAM_WRITABLE
 * @XPARAM_CONSTRUCT: the parameter will be set upon object construction
 * @XPARAM_CONSTRUCT_ONLY: the parameter can only be set upon object construction
 * @XPARAM_LAX_VALIDATION: upon parameter conversion (see g_param_value_convert())
 *  strict validation is not required
 * @XPARAM_STATIC_NAME: the string used as name when constructing the
 *  parameter is guaranteed to remain valid and
 *  unmodified for the lifetime of the parameter.
 *  Since 2.8
 * @XPARAM_STATIC_NICK: the string used as nick when constructing the
 *  parameter is guaranteed to remain valid and
 *  unmmodified for the lifetime of the parameter.
 *  Since 2.8
 * @XPARAM_STATIC_BLURB: the string used as blurb when constructing the
 *  parameter is guaranteed to remain valid and
 *  unmodified for the lifetime of the parameter.
 *  Since 2.8
 * @XPARAM_EXPLICIT_NOTIFY: calls to xobject_set_property() for this
 *   property will not automatically result in a "notify" signal being
 *   emitted: the implementation must call xobject_notify() themselves
 *   in case the property actually changes.  Since: 2.42.
 * @XPARAM_PRIVATE: internal
 * @XPARAM_DEPRECATED: the parameter is deprecated and will be removed
 *  in a future version. A warning will be generated if it is used
 *  while running with G_ENABLE_DIAGNOSTIC=1.
 *  Since 2.26
 *
 * Through the #GParamFlags flag values, certain aspects of parameters
 * can be configured.
 *
 * See also: %XPARAM_STATIC_STRINGS
 */
typedef enum
{
  XPARAM_READABLE            = 1 << 0,
  XPARAM_WRITABLE            = 1 << 1,
  XPARAM_READWRITE           = (XPARAM_READABLE | XPARAM_WRITABLE),
  XPARAM_CONSTRUCT	      = 1 << 2,
  XPARAM_CONSTRUCT_ONLY      = 1 << 3,
  XPARAM_LAX_VALIDATION      = 1 << 4,
  XPARAM_STATIC_NAME	      = 1 << 5,
  XPARAM_PRIVATE XPL_DEPRECATED_ENUMERATOR_IN_2_26 = XPARAM_STATIC_NAME,
  XPARAM_STATIC_NICK	      = 1 << 6,
  XPARAM_STATIC_BLURB	      = 1 << 7,
  /* User defined flags go here */
  XPARAM_EXPLICIT_NOTIFY     = 1 << 30,
  /* Avoid warning with -Wpedantic for gcc6 */
  XPARAM_DEPRECATED          = (xint_t)(1u << 31)
} GParamFlags;

/**
 * XPARAM_STATIC_STRINGS:
 *
 * #GParamFlags value alias for %XPARAM_STATIC_NAME | %XPARAM_STATIC_NICK | %XPARAM_STATIC_BLURB.
 *
 * Since 2.13.0
 */
#define	XPARAM_STATIC_STRINGS (XPARAM_STATIC_NAME | XPARAM_STATIC_NICK | XPARAM_STATIC_BLURB)
/* bits in the range 0xffffff00 are reserved for 3rd party usage */
/**
 * XPARAM_MASK:
 *
 * Mask containing the bits of #xparam_spec_t.flags which are reserved for GLib.
 */
#define	XPARAM_MASK		(0x000000ff)
/**
 * XPARAM_USER_SHIFT:
 *
 * Minimum shift count to be used for user defined flags, to be stored in
 * #xparam_spec_t.flags. The maximum allowed is 10.
 */
#define	XPARAM_USER_SHIFT	(8)

/* --- typedefs & structures --- */
typedef struct _GParamSpec      xparam_spec_t;
typedef struct _GParamSpecClass GParamSpecClass;
typedef struct _GParameter	GParameter XPL_DEPRECATED_TYPE_IN_2_54;
typedef struct _GParamSpecPool  GParamSpecPool;
/**
 * xparam_spec_t: (ref-func xparam_spec_ref_sink) (unref-func xparam_spec_unref) (set-value-func xvalue_set_param) (get-value-func xvalue_get_param)
 * @xtype_instance: private #GTypeInstance portion
 * @name: name of this parameter: always an interned string
 * @flags: #GParamFlags flags for this parameter
 * @value_type: the #xvalue_t type for this parameter
 * @owner_type: #xtype_t type that uses (introduces) this parameter
 *
 * All other fields of the xparam_spec_t struct are private and
 * should not be used directly.
 */
struct _GParamSpec
{
  GTypeInstance  xtype_instance;

  const xchar_t   *name;          /* interned string */
  GParamFlags    flags;
  xtype_t		 value_type;
  xtype_t		 owner_type;	/* class or interface using this property */

  /*< private >*/
  xchar_t         *_nick;
  xchar_t         *_blurb;
  GData		*qdata;
  xuint_t          ref_count;
  xuint_t		 param_id;	/* sort-criteria */
};
/**
 * GParamSpecClass:
 * @xtype_class: the parent class
 * @value_type: the #xvalue_t type for this parameter
 * @finalize: The instance finalization function (optional), should chain
 *  up to the finalize method of the parent class.
 * @value_set_default: Resets a @value to the default value for this type
 *  (recommended, the default is xvalue_reset()), see
 *  g_param_value_set_default().
 * @value_validate: Ensures that the contents of @value comply with the
 *  specifications set out by this type (optional), see
 *  g_param_value_validate().
 * @values_cmp: Compares @value1 with @value2 according to this type
 *  (recommended, the default is memcmp()), see g_param_values_cmp().
 *
 * The class structure for the xparam_spec_t type.
 * Normally, xparam_spec_t classes are filled by
 * g_param_type_register_static().
 */
struct _GParamSpecClass
{
  xtype_class_t      xtype_class;

  xtype_t		  value_type;

  void	        (*finalize)		(xparam_spec_t   *pspec);

  /* GParam methods */
  void          (*value_set_default)    (xparam_spec_t   *pspec,
					 xvalue_t       *value);
  xboolean_t      (*value_validate)       (xparam_spec_t   *pspec,
					 xvalue_t       *value);
  xint_t          (*values_cmp)           (xparam_spec_t   *pspec,
					 const xvalue_t *value1,
					 const xvalue_t *value2);
  /*< private >*/
  xpointer_t	  dummy[4];
};
/**
 * GParameter:
 * @name: the parameter name
 * @value: the parameter value
 *
 * The GParameter struct is an auxiliary structure used
 * to hand parameter name/value pairs to xobject_newv().
 *
 * Deprecated: 2.54: This type is not introspectable.
 */
struct _GParameter /* auxiliary structure for _setv() variants */
{
  const xchar_t *name;
  xvalue_t       value;
} XPL_DEPRECATED_TYPE_IN_2_54;


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	xparam_spec_ref		(xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
void		xparam_spec_unref		(xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
void		xparam_spec_sink		(xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	xparam_spec_ref_sink   	(xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
xpointer_t        xparam_spec_get_qdata		(xparam_spec_t    *pspec,
						 xquark         quark);
XPL_AVAILABLE_IN_ALL
void            xparam_spec_set_qdata		(xparam_spec_t    *pspec,
						 xquark         quark,
						 xpointer_t       data);
XPL_AVAILABLE_IN_ALL
void            xparam_spec_set_qdata_full	(xparam_spec_t    *pspec,
						 xquark         quark,
						 xpointer_t       data,
						 xdestroy_notify_t destroy);
XPL_AVAILABLE_IN_ALL
xpointer_t        xparam_spec_steal_qdata	(xparam_spec_t    *pspec,
						 xquark         quark);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*     xparam_spec_get_redirect_target (xparam_spec_t   *pspec);

XPL_AVAILABLE_IN_ALL
void		g_param_value_set_default	(xparam_spec_t    *pspec,
						 xvalue_t	       *value);
XPL_AVAILABLE_IN_ALL
xboolean_t	g_param_value_defaults		(xparam_spec_t    *pspec,
						 const xvalue_t  *value);
XPL_AVAILABLE_IN_ALL
xboolean_t	g_param_value_validate		(xparam_spec_t    *pspec,
						 xvalue_t	       *value);
XPL_AVAILABLE_IN_ALL
xboolean_t	g_param_value_convert		(xparam_spec_t    *pspec,
						 const xvalue_t  *src_value,
						 xvalue_t	       *dest_value,
						 xboolean_t	strict_validation);
XPL_AVAILABLE_IN_ALL
xint_t		g_param_values_cmp		(xparam_spec_t    *pspec,
						 const xvalue_t  *value1,
						 const xvalue_t  *value2);
XPL_AVAILABLE_IN_ALL
const xchar_t *   xparam_spec_get_name           (xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
const xchar_t *   xparam_spec_get_nick           (xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
const xchar_t *   xparam_spec_get_blurb          (xparam_spec_t    *pspec);
XPL_AVAILABLE_IN_ALL
void            xvalue_set_param               (xvalue_t	       *value,
						 xparam_spec_t    *param);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*     xvalue_get_param               (const xvalue_t  *value);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*     xvalue_dup_param               (const xvalue_t  *value);


XPL_AVAILABLE_IN_ALL
void           xvalue_take_param               (xvalue_t        *value,
					         xparam_spec_t    *param);
XPL_DEPRECATED_FOR(xvalue_take_param)
void           xvalue_set_param_take_ownership (xvalue_t        *value,
                                                 xparam_spec_t    *param);
XPL_AVAILABLE_IN_2_36
const xvalue_t *  xparam_spec_get_default_value  (xparam_spec_t    *pspec);

XPL_AVAILABLE_IN_2_46
xquark          xparam_spec_get_name_quark     (xparam_spec_t    *pspec);

/* --- convenience functions --- */
typedef struct _GParamSpecTypeInfo GParamSpecTypeInfo;
/**
 * GParamSpecTypeInfo:
 * @instance_size: Size of the instance (object) structure.
 * @n_preallocs: Prior to GLib 2.10, it specified the number of pre-allocated (cached) instances to reserve memory for (0 indicates no caching). Since GLib 2.10, it is ignored, since instances are allocated with the [slice allocator][glib-Memory-Slices] now.
 * @instance_init: Location of the instance initialization function (optional).
 * @value_type: The #xtype_t of values conforming to this #xparam_spec_t
 * @finalize: The instance finalization function (optional).
 * @value_set_default: Resets a @value to the default value for @pspec
 *  (recommended, the default is xvalue_reset()), see
 *  g_param_value_set_default().
 * @value_validate: Ensures that the contents of @value comply with the
 *  specifications set out by @pspec (optional), see
 *  g_param_value_validate().
 * @values_cmp: Compares @value1 with @value2 according to @pspec
 *  (recommended, the default is memcmp()), see g_param_values_cmp().
 *
 * This structure is used to provide the type system with the information
 * required to initialize and destruct (finalize) a parameter's class and
 * instances thereof.
 *
 * The initialized structure is passed to the g_param_type_register_static()
 * The type system will perform a deep copy of this structure, so its memory
 * does not need to be persistent across invocation of
 * g_param_type_register_static().
 */
struct _GParamSpecTypeInfo
{
  /* type system portion */
  xuint16_t         instance_size;                               /* obligatory */
  xuint16_t         n_preallocs;                                 /* optional */
  void		(*instance_init)	(xparam_spec_t   *pspec); /* optional */

  /* class portion */
  xtype_t           value_type;				       /* obligatory */
  void          (*finalize)             (xparam_spec_t   *pspec); /* optional */
  void          (*value_set_default)    (xparam_spec_t   *pspec,  /* recommended */
					 xvalue_t       *value);
  xboolean_t      (*value_validate)       (xparam_spec_t   *pspec,  /* optional */
					 xvalue_t       *value);
  xint_t          (*values_cmp)           (xparam_spec_t   *pspec,  /* recommended */
					 const xvalue_t *value1,
					 const xvalue_t *value2);
};
XPL_AVAILABLE_IN_ALL
xtype_t	g_param_type_register_static	(const xchar_t		  *name,
					 const GParamSpecTypeInfo *pspec_info);

XPL_AVAILABLE_IN_2_66
xboolean_t xparam_spec_is_valid_name    (const xchar_t              *name);

/* For registering builting types */
xtype_t  _g_param_type_register_static_constant (const xchar_t              *name,
					       const GParamSpecTypeInfo *pspec_info,
					       xtype_t                     opt_type);


/* --- protected --- */
XPL_AVAILABLE_IN_ALL
xpointer_t	xparam_spec_internal		(xtype_t	        param_type,
						 const xchar_t   *name,
						 const xchar_t   *nick,
						 const xchar_t   *blurb,
						 GParamFlags    flags);
XPL_AVAILABLE_IN_ALL
GParamSpecPool* xparam_spec_pool_new		(xboolean_t	type_prefixing);
XPL_AVAILABLE_IN_ALL
void		xparam_spec_pool_insert	(GParamSpecPool	*pool,
						 xparam_spec_t	*pspec,
						 xtype_t		 owner_type);
XPL_AVAILABLE_IN_ALL
void		xparam_spec_pool_remove	(GParamSpecPool	*pool,
						 xparam_spec_t	*pspec);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	xparam_spec_pool_lookup	(GParamSpecPool	*pool,
						 const xchar_t	*param_name,
						 xtype_t		 owner_type,
						 xboolean_t	 walk_ancestors);
XPL_AVAILABLE_IN_ALL
xlist_t*		xparam_spec_pool_list_owned	(GParamSpecPool	*pool,
						 xtype_t		 owner_type);
XPL_AVAILABLE_IN_ALL
xparam_spec_t**	xparam_spec_pool_list		(GParamSpecPool	*pool,
						 xtype_t		 owner_type,
						 xuint_t		*n_pspecs_p);


/* contracts:
 *
 * xboolean_t value_validate (xparam_spec_t *pspec,
 *                          xvalue_t     *value):
 *	modify value contents in the least destructive way, so
 *	that it complies with pspec's requirements (i.e.
 *	according to minimum/maximum ranges etc...). return
 *	whether modification was necessary.
 *
 * xint_t values_cmp (xparam_spec_t   *pspec,
 *                  const xvalue_t *value1,
 *                  const xvalue_t *value2):
 *	return value1 - value2, i.e. (-1) if value1 < value2,
 *	(+1) if value1 > value2, and (0) otherwise (equality)
 */

G_END_DECLS

#endif /* __XPARAM_H__ */
