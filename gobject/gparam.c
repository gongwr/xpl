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
 */

/*
 * MT safe
 */

#include "config.h"

#include <string.h>

#include "gparam.h"
#include "gparamspecs.h"
#include "gvaluecollector.h"
#include "gtype-private.h"

/**
 * SECTION:gparamspec
 * @short_description: Metadata for parameter specifications
 * @see_also: xobject_class_install_property(), xobject_set(),
 *     xobject_get(), xobject_set_property(), xobject_get_property(),
 *     xvalue_register_transform_func()
 * @title: xparam_spec_t
 *
 * #xparam_spec_t is an object structure that encapsulates the metadata
 * required to specify parameters, such as e.g. #xobject_t properties.
 *
 * ## Parameter names # {#canonical-parameter-names}
 *
 * A property name consists of one or more segments consisting of ASCII letters
 * and digits, separated by either the `-` or `_` character. The first
 * character of a property name must be a letter. These are the same rules as
 * for signal naming (see xsignal_new()).
 *
 * When creating and looking up a #xparam_spec_t, either separator can be
 * used, but they cannot be mixed. Using `-` is considerably more
 * efficient, and is the ‘canonical form’. Using `_` is discouraged.
 */


/* --- defines --- */
#define PARAM_FLOATING_FLAG                     0x2
#define	G_PARAM_USER_MASK			(~0U << G_PARAM_USER_SHIFT)
#define PSPEC_APPLIES_TO_VALUE(pspec, value)	(XTYPE_CHECK_VALUE_TYPE ((value), G_PARAM_SPEC_VALUE_TYPE (pspec)))

/* --- prototypes --- */
static void	g_param_spec_class_base_init	 (GParamSpecClass	*class);
static void	g_param_spec_class_base_finalize (GParamSpecClass	*class);
static void	g_param_spec_class_init		 (GParamSpecClass	*class,
						  xpointer_t               class_data);
static void	g_param_spec_init		 (xparam_spec_t		*pspec,
						  GParamSpecClass	*class);
static void	g_param_spec_finalize		 (xparam_spec_t		*pspec);
static void	value_param_init		(xvalue_t		*value);
static void	value_param_free_value		(xvalue_t		*value);
static void	value_param_copy_value		(const xvalue_t	*src_value,
						 xvalue_t		*dest_value);
static void	value_param_transform_value	(const xvalue_t	*src_value,
						 xvalue_t		*dest_value);
static xpointer_t	value_param_peek_pointer	(const xvalue_t	*value);
static xchar_t*	value_param_collect_value	(xvalue_t		*value,
						 xuint_t           n_collect_values,
						 xtype_c_value_t    *collect_values,
						 xuint_t           collect_flags);
static xchar_t*	value_param_lcopy_value		(const xvalue_t	*value,
						 xuint_t           n_collect_values,
						 xtype_c_value_t    *collect_values,
						 xuint_t           collect_flags);

typedef struct
{
  xvalue_t default_value;
  xquark name_quark;
} GParamSpecPrivate;

static xint_t g_param_private_offset;

/* --- functions --- */
static inline GParamSpecPrivate *
g_param_spec_get_private (xparam_spec_t *pspec)
{
  return &G_STRUCT_MEMBER (GParamSpecPrivate, pspec, g_param_private_offset);
}

void
_g_param_type_init (void)
{
  static const GTypeFundamentalInfo finfo = {
    (XTYPE_FLAG_CLASSED |
     XTYPE_FLAG_INSTANTIATABLE |
     XTYPE_FLAG_DERIVABLE |
     XTYPE_FLAG_DEEP_DERIVABLE),
  };
  static const xtype_value_table_t param_value_table = {
    value_param_init,           /* value_init */
    value_param_free_value,     /* value_free */
    value_param_copy_value,     /* value_copy */
    value_param_peek_pointer,   /* value_peek_pointer */
    "p",			/* collect_format */
    value_param_collect_value,  /* collect_value */
    "p",			/* lcopy_format */
    value_param_lcopy_value,    /* lcopy_value */
  };
  const xtype_info_t param_spec_info = {
    sizeof (GParamSpecClass),

    (xbase_init_func_t) g_param_spec_class_base_init,
    (xbase_finalize_func_t) g_param_spec_class_base_finalize,
    (xclass_init_func_t) g_param_spec_class_init,
    (xclass_finalize_func_t) NULL,
    NULL,	/* class_data */

    sizeof (xparam_spec_t),
    0,		/* n_preallocs */
    (xinstance_init_func_t) g_param_spec_init,

    &param_value_table,
  };
  xtype_t type;

  /* This should be registered as xparam_spec_t instead of GParam, for
   * consistency sake, so that type name can be mapped to struct name,
   * However, some language bindings, most noticeable the python ones
   * depends on the "GParam" identifier, see #548689
   */
  type = xtype_register_fundamental (XTYPE_PARAM, g_intern_static_string ("GParam"), &param_spec_info, &finfo, XTYPE_FLAG_ABSTRACT);
  g_assert (type == XTYPE_PARAM);
  g_param_private_offset = xtype_add_instance_private (type, sizeof (GParamSpecPrivate));
  xvalue_register_transform_func (XTYPE_PARAM, XTYPE_PARAM, value_param_transform_value);
}

static void
g_param_spec_class_base_init (GParamSpecClass *class)
{
}

static void
g_param_spec_class_base_finalize (GParamSpecClass *class)
{
}

static void
g_param_spec_class_init (GParamSpecClass *class,
			 xpointer_t         class_data)
{
  class->value_type = XTYPE_NONE;
  class->finalize = g_param_spec_finalize;
  class->value_set_default = NULL;
  class->value_validate = NULL;
  class->values_cmp = NULL;

  xtype_class_adjust_private_offset (class, &g_param_private_offset);
}

static void
g_param_spec_init (xparam_spec_t      *pspec,
		   GParamSpecClass *class)
{
  pspec->name = NULL;
  pspec->_nick = NULL;
  pspec->_blurb = NULL;
  pspec->flags = 0;
  pspec->value_type = class->value_type;
  pspec->owner_type = 0;
  pspec->qdata = NULL;
  g_datalist_set_flags (&pspec->qdata, PARAM_FLOATING_FLAG);
  pspec->ref_count = 1;
  pspec->param_id = 0;
}

static void
g_param_spec_finalize (xparam_spec_t *pspec)
{
  GParamSpecPrivate *priv = g_param_spec_get_private (pspec);

  if (priv->default_value.g_type)
    xvalue_reset (&priv->default_value);

  g_datalist_clear (&pspec->qdata);

  if (!(pspec->flags & G_PARAM_STATIC_NICK))
    g_free (pspec->_nick);

  if (!(pspec->flags & G_PARAM_STATIC_BLURB))
    g_free (pspec->_blurb);

  xtype_free_instance ((GTypeInstance*) pspec);
}

/**
 * g_param_spec_ref: (skip)
 * @pspec: (transfer none) (not nullable): a valid #xparam_spec_t
 *
 * Increments the reference count of @pspec.
 *
 * Returns: (transfer full) (not nullable): the #xparam_spec_t that was passed into this function
 */
xparam_spec_t*
g_param_spec_ref (xparam_spec_t *pspec)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);

  g_atomic_int_inc ((int *)&pspec->ref_count);

  return pspec;
}

/**
 * g_param_spec_unref: (skip)
 * @pspec: a valid #xparam_spec_t
 *
 * Decrements the reference count of a @pspec.
 */
void
g_param_spec_unref (xparam_spec_t *pspec)
{
  xboolean_t is_zero;

  g_return_if_fail (X_IS_PARAM_SPEC (pspec));

  is_zero = g_atomic_int_dec_and_test ((int *)&pspec->ref_count);

  if (G_UNLIKELY (is_zero))
    {
      G_PARAM_SPEC_GET_CLASS (pspec)->finalize (pspec);
    }
}

/**
 * g_param_spec_sink:
 * @pspec: a valid #xparam_spec_t
 *
 * The initial reference count of a newly created #xparam_spec_t is 1,
 * even though no one has explicitly called g_param_spec_ref() on it
 * yet. So the initial reference count is flagged as "floating", until
 * someone calls `g_param_spec_ref (pspec); g_param_spec_sink
 * (pspec);` in sequence on it, taking over the initial
 * reference count (thus ending up with a @pspec that has a reference
 * count of 1 still, but is not flagged "floating" anymore).
 */
void
g_param_spec_sink (xparam_spec_t *pspec)
{
  xsize_t oldvalue;
  g_return_if_fail (X_IS_PARAM_SPEC (pspec));

  oldvalue = g_atomic_pointer_and (&pspec->qdata, ~(xsize_t)PARAM_FLOATING_FLAG);
  if (oldvalue & PARAM_FLOATING_FLAG)
    g_param_spec_unref (pspec);
}

/**
 * g_param_spec_ref_sink: (skip)
 * @pspec: a valid #xparam_spec_t
 *
 * Convenience function to ref and sink a #xparam_spec_t.
 *
 * Since: 2.10
 * Returns: (transfer full) (not nullable): the #xparam_spec_t that was passed into this function
 */
xparam_spec_t*
g_param_spec_ref_sink (xparam_spec_t *pspec)
{
  xsize_t oldvalue;
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);

  oldvalue = g_atomic_pointer_and (&pspec->qdata, ~(xsize_t)PARAM_FLOATING_FLAG);
  if (!(oldvalue & PARAM_FLOATING_FLAG))
    g_param_spec_ref (pspec);

  return pspec;
}

/**
 * g_param_spec_get_name:
 * @pspec: a valid #xparam_spec_t
 *
 * Get the name of a #xparam_spec_t.
 *
 * The name is always an "interned" string (as per g_intern_string()).
 * This allows for pointer-value comparisons.
 *
 * Returns: the name of @pspec.
 */
const xchar_t *
g_param_spec_get_name (xparam_spec_t *pspec)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);

  return pspec->name;
}

/**
 * g_param_spec_get_nick:
 * @pspec: a valid #xparam_spec_t
 *
 * Get the nickname of a #xparam_spec_t.
 *
 * Returns: the nickname of @pspec.
 */
const xchar_t *
g_param_spec_get_nick (xparam_spec_t *pspec)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);

  if (pspec->_nick)
    return pspec->_nick;
  else
    {
      xparam_spec_t *redirect_target;

      redirect_target = g_param_spec_get_redirect_target (pspec);
      if (redirect_target && redirect_target->_nick)
	return redirect_target->_nick;
    }

  return pspec->name;
}

/**
 * g_param_spec_get_blurb:
 * @pspec: a valid #xparam_spec_t
 *
 * Get the short description of a #xparam_spec_t.
 *
 * Returns: (nullable): the short description of @pspec.
 */
const xchar_t *
g_param_spec_get_blurb (xparam_spec_t *pspec)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);

  if (pspec->_blurb)
    return pspec->_blurb;
  else
    {
      xparam_spec_t *redirect_target;

      redirect_target = g_param_spec_get_redirect_target (pspec);
      if (redirect_target && redirect_target->_blurb)
	return redirect_target->_blurb;
    }

  return NULL;
}

/* @key must have already been validated with is_valid()
 * Modifies @key in place. */
static void
canonicalize_key (xchar_t *key)
{
  xchar_t *p;

  for (p = key; *p != 0; p++)
    {
      xchar_t c = *p;

      if (c == '_')
        *p = '-';
    }
}

/* @key must have already been validated with is_valid() */
static xboolean_t
is_canonical (const xchar_t *key)
{
  return (strchr (key, '_') == NULL);
}

/**
 * g_param_spec_is_valid_name:
 * @name: the canonical name of the property
 *
 * Validate a property name for a #xparam_spec_t. This can be useful for
 * dynamically-generated properties which need to be validated at run-time
 * before actually trying to create them.
 *
 * See [canonical parameter names][canonical-parameter-names] for details of
 * the rules for valid names.
 *
 * Returns: %TRUE if @name is a valid property name, %FALSE otherwise.
 * Since: 2.66
 */
xboolean_t
g_param_spec_is_valid_name (const xchar_t *name)
{
  const xchar_t *p;

  /* First character must be a letter. */
  if ((name[0] < 'A' || name[0] > 'Z') &&
      (name[0] < 'a' || name[0] > 'z'))
    return FALSE;

  for (p = name; *p != 0; p++)
    {
      const xchar_t c = *p;

      if (c != '-' && c != '_' &&
          (c < '0' || c > '9') &&
          (c < 'A' || c > 'Z') &&
          (c < 'a' || c > 'z'))
        return FALSE;
    }

  return TRUE;
}

/**
 * g_param_spec_internal: (skip)
 * @param_type: the #xtype_t for the property; must be derived from %XTYPE_PARAM
 * @name: the canonical name of the property
 * @nick: the nickname of the property
 * @blurb: a short description of the property
 * @flags: a combination of #GParamFlags
 *
 * Creates a new #xparam_spec_t instance.
 *
 * See [canonical parameter names][canonical-parameter-names] for details of
 * the rules for @name. Names which violate these rules lead to undefined
 * behaviour.
 *
 * Beyond the name, #GParamSpecs have two more descriptive
 * strings associated with them, the @nick, which should be suitable
 * for use as a label for the property in a property editor, and the
 * @blurb, which should be a somewhat longer description, suitable for
 * e.g. a tooltip. The @nick and @blurb should ideally be localized.
 *
 * Returns: (type xobject_t.ParamSpec): (transfer floating): a newly allocated
 *     #xparam_spec_t instance, which is initially floating
 */
xpointer_t
g_param_spec_internal (xtype_t        param_type,
		       const xchar_t *name,
		       const xchar_t *nick,
		       const xchar_t *blurb,
		       GParamFlags  flags)
{
  xparam_spec_t *pspec;
  GParamSpecPrivate *priv;

  g_return_val_if_fail (XTYPE_IS_PARAM (param_type) && param_type != XTYPE_PARAM, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (g_param_spec_is_valid_name (name), NULL);
  g_return_val_if_fail (!(flags & G_PARAM_STATIC_NAME) || is_canonical (name), NULL);

  pspec = (xpointer_t) xtype_create_instance (param_type);

  if (flags & G_PARAM_STATIC_NAME)
    {
      /* pspec->name is not freed if (flags & G_PARAM_STATIC_NAME) */
      pspec->name = (xchar_t *) g_intern_static_string (name);
      if (!is_canonical (pspec->name))
        g_warning ("G_PARAM_STATIC_NAME used with non-canonical pspec name: %s", pspec->name);
    }
  else
    {
      if (is_canonical (name))
        pspec->name = (xchar_t *) g_intern_string (name);
      else
        {
          xchar_t *tmp = xstrdup (name);
          canonicalize_key (tmp);
          pspec->name = (xchar_t *) g_intern_string (tmp);
          g_free (tmp);
        }
    }

  priv = g_param_spec_get_private (pspec);
  priv->name_quark = g_quark_from_string (pspec->name);

  if (flags & G_PARAM_STATIC_NICK)
    pspec->_nick = (xchar_t*) nick;
  else
    pspec->_nick = xstrdup (nick);

  if (flags & G_PARAM_STATIC_BLURB)
    pspec->_blurb = (xchar_t*) blurb;
  else
    pspec->_blurb = xstrdup (blurb);

  pspec->flags = (flags & G_PARAM_USER_MASK) | (flags & G_PARAM_MASK);

  return pspec;
}

/**
 * g_param_spec_get_qdata:
 * @pspec: a valid #xparam_spec_t
 * @quark: a #xquark, naming the user data pointer
 *
 * Gets back user data pointers stored via g_param_spec_set_qdata().
 *
 * Returns: (transfer none) (nullable): the user data pointer set, or %NULL
 */
xpointer_t
g_param_spec_get_qdata (xparam_spec_t *pspec,
			xquark      quark)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);

  return quark ? g_datalist_id_get_data (&pspec->qdata, quark) : NULL;
}

/**
 * g_param_spec_set_qdata:
 * @pspec: the #xparam_spec_t to set store a user data pointer
 * @quark: a #xquark, naming the user data pointer
 * @data: (nullable): an opaque user data pointer
 *
 * Sets an opaque, named pointer on a #xparam_spec_t. The name is
 * specified through a #xquark (retrieved e.g. via
 * g_quark_from_static_string()), and the pointer can be gotten back
 * from the @pspec with g_param_spec_get_qdata().  Setting a
 * previously set user data pointer, overrides (frees) the old pointer
 * set, using %NULL as pointer essentially removes the data stored.
 */
void
g_param_spec_set_qdata (xparam_spec_t *pspec,
			xquark      quark,
			xpointer_t    data)
{
  g_return_if_fail (X_IS_PARAM_SPEC (pspec));
  g_return_if_fail (quark > 0);

  g_datalist_id_set_data (&pspec->qdata, quark, data);
}

/**
 * g_param_spec_set_qdata_full: (skip)
 * @pspec: the #xparam_spec_t to set store a user data pointer
 * @quark: a #xquark, naming the user data pointer
 * @data: (nullable): an opaque user data pointer
 * @destroy: (nullable): function to invoke with @data as argument, when @data needs to
 *  be freed
 *
 * This function works like g_param_spec_set_qdata(), but in addition,
 * a `void (*destroy) (xpointer_t)` function may be
 * specified which is called with @data as argument when the @pspec is
 * finalized, or the data is being overwritten by a call to
 * g_param_spec_set_qdata() with the same @quark.
 */
void
g_param_spec_set_qdata_full (xparam_spec_t    *pspec,
			     xquark         quark,
			     xpointer_t       data,
			     xdestroy_notify_t destroy)
{
  g_return_if_fail (X_IS_PARAM_SPEC (pspec));
  g_return_if_fail (quark > 0);

  g_datalist_id_set_data_full (&pspec->qdata, quark, data, data ? destroy : (xdestroy_notify_t) NULL);
}

/**
 * g_param_spec_steal_qdata:
 * @pspec: the #xparam_spec_t to get a stored user data pointer from
 * @quark: a #xquark, naming the user data pointer
 *
 * Gets back user data pointers stored via g_param_spec_set_qdata()
 * and removes the @data from @pspec without invoking its destroy()
 * function (if any was set).  Usually, calling this function is only
 * required to update user data pointers with a destroy notifier.
 *
 * Returns: (transfer none) (nullable): the user data pointer set, or %NULL
 */
xpointer_t
g_param_spec_steal_qdata (xparam_spec_t *pspec,
			  xquark      quark)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (quark > 0, NULL);

  return g_datalist_id_remove_no_notify (&pspec->qdata, quark);
}

/**
 * g_param_spec_get_redirect_target:
 * @pspec: a #xparam_spec_t
 *
 * If the paramspec redirects operations to another paramspec,
 * returns that paramspec. Redirect is used typically for
 * providing a new implementation of a property in a derived
 * type while preserving all the properties from the parent
 * type. Redirection is established by creating a property
 * of type #GParamSpecOverride. See xobject_class_override_property()
 * for an example of the use of this capability.
 *
 * Since: 2.4
 *
 * Returns: (transfer none) (nullable): paramspec to which requests on this
 *          paramspec should be redirected, or %NULL if none.
 */
xparam_spec_t*
g_param_spec_get_redirect_target (xparam_spec_t *pspec)
{
  GTypeInstance *inst = (GTypeInstance *)pspec;

  if (inst && inst->g_class && inst->g_class->g_type == XTYPE_PARAM_OVERRIDE)
    return ((GParamSpecOverride*)pspec)->overridden;
  else
    return NULL;
}

/**
 * g_param_value_set_default:
 * @pspec: a valid #xparam_spec_t
 * @value: a #xvalue_t of correct type for @pspec; since 2.64, you
 *   can also pass an empty #xvalue_t, initialized with %G_VALUE_INIT
 *
 * Sets @value to its default value as specified in @pspec.
 */
void
g_param_value_set_default (xparam_spec_t *pspec,
			   xvalue_t     *value)
{
  g_return_if_fail (X_IS_PARAM_SPEC (pspec));

  if (G_VALUE_TYPE (value) == XTYPE_INVALID)
    {
      xvalue_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
    }
  else
    {
      g_return_if_fail (X_IS_VALUE (value));
      g_return_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value));
      xvalue_reset (value);
    }

  G_PARAM_SPEC_GET_CLASS (pspec)->value_set_default (pspec, value);
}

/**
 * g_param_value_defaults:
 * @pspec: a valid #xparam_spec_t
 * @value: a #xvalue_t of correct type for @pspec
 *
 * Checks whether @value contains the default value as specified in @pspec.
 *
 * Returns: whether @value contains the canonical default for this @pspec
 */
xboolean_t
g_param_value_defaults (xparam_spec_t   *pspec,
			const xvalue_t *value)
{
  xvalue_t dflt_value = G_VALUE_INIT;
  xboolean_t defaults;

  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (X_IS_VALUE (value), FALSE);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value), FALSE);

  xvalue_init (&dflt_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  G_PARAM_SPEC_GET_CLASS (pspec)->value_set_default (pspec, &dflt_value);
  defaults = G_PARAM_SPEC_GET_CLASS (pspec)->values_cmp (pspec, value, &dflt_value) == 0;
  xvalue_unset (&dflt_value);

  return defaults;
}

/**
 * g_param_value_validate:
 * @pspec: a valid #xparam_spec_t
 * @value: a #xvalue_t of correct type for @pspec
 *
 * Ensures that the contents of @value comply with the specifications
 * set out by @pspec. For example, a #GParamSpecInt might require
 * that integers stored in @value may not be smaller than -42 and not be
 * greater than +42. If @value contains an integer outside of this range,
 * it is modified accordingly, so the resulting value will fit into the
 * range -42 .. +42.
 *
 * Returns: whether modifying @value was necessary to ensure validity
 */
xboolean_t
g_param_value_validate (xparam_spec_t *pspec,
			xvalue_t     *value)
{
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (X_IS_VALUE (value), FALSE);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value), FALSE);

  if (G_PARAM_SPEC_GET_CLASS (pspec)->value_validate)
    {
      xvalue_t oval = *value;

      if (G_PARAM_SPEC_GET_CLASS (pspec)->value_validate (pspec, value) ||
	  memcmp (&oval.data, &value->data, sizeof (oval.data)))
	return TRUE;
    }

  return FALSE;
}

/**
 * g_param_value_convert:
 * @pspec: a valid #xparam_spec_t
 * @src_value: source #xvalue_t
 * @dest_value: destination #xvalue_t of correct type for @pspec
 * @strict_validation: %TRUE requires @dest_value to conform to @pspec
 * without modifications
 *
 * Transforms @src_value into @dest_value if possible, and then
 * validates @dest_value, in order for it to conform to @pspec.  If
 * @strict_validation is %TRUE this function will only succeed if the
 * transformed @dest_value complied to @pspec without modifications.
 *
 * See also xvalue_type_transformable(), xvalue_transform() and
 * g_param_value_validate().
 *
 * Returns: %TRUE if transformation and validation were successful,
 *  %FALSE otherwise and @dest_value is left untouched.
 */
xboolean_t
g_param_value_convert (xparam_spec_t   *pspec,
		       const xvalue_t *src_value,
		       xvalue_t       *dest_value,
		       xboolean_t	     strict_validation)
{
  xvalue_t tmp_value = G_VALUE_INIT;

  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (X_IS_VALUE (src_value), FALSE);
  g_return_val_if_fail (X_IS_VALUE (dest_value), FALSE);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, dest_value), FALSE);

  /* better leave dest_value untouched when returning FALSE */

  xvalue_init (&tmp_value, G_VALUE_TYPE (dest_value));
  if (xvalue_transform (src_value, &tmp_value) &&
      (!g_param_value_validate (pspec, &tmp_value) || !strict_validation))
    {
      xvalue_unset (dest_value);

      /* values are relocatable */
      memcpy (dest_value, &tmp_value, sizeof (tmp_value));

      return TRUE;
    }
  else
    {
      xvalue_unset (&tmp_value);

      return FALSE;
    }
}

/**
 * g_param_values_cmp:
 * @pspec: a valid #xparam_spec_t
 * @value1: a #xvalue_t of correct type for @pspec
 * @value2: a #xvalue_t of correct type for @pspec
 *
 * Compares @value1 with @value2 according to @pspec, and return -1, 0 or +1,
 * if @value1 is found to be less than, equal to or greater than @value2,
 * respectively.
 *
 * Returns: -1, 0 or +1, for a less than, equal to or greater than result
 */
xint_t
g_param_values_cmp (xparam_spec_t   *pspec,
		    const xvalue_t *value1,
		    const xvalue_t *value2)
{
  xint_t cmp;

  /* param_values_cmp() effectively does: value1 - value2
   * so the return values are:
   * -1)  value1 < value2
   *  0)  value1 == value2
   *  1)  value1 > value2
   */
  g_return_val_if_fail (X_IS_PARAM_SPEC (pspec), 0);
  g_return_val_if_fail (X_IS_VALUE (value1), 0);
  g_return_val_if_fail (X_IS_VALUE (value2), 0);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value1), 0);
  g_return_val_if_fail (PSPEC_APPLIES_TO_VALUE (pspec, value2), 0);

  cmp = G_PARAM_SPEC_GET_CLASS (pspec)->values_cmp (pspec, value1, value2);

  return CLAMP (cmp, -1, 1);
}

static void
value_param_init (xvalue_t *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_param_free_value (xvalue_t *value)
{
  if (value->data[0].v_pointer)
    g_param_spec_unref (value->data[0].v_pointer);
}

static void
value_param_copy_value (const xvalue_t *src_value,
			xvalue_t       *dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_param_spec_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static void
value_param_transform_value (const xvalue_t *src_value,
			     xvalue_t       *dest_value)
{
  if (src_value->data[0].v_pointer &&
      xtype_is_a (G_PARAM_SPEC_TYPE (dest_value->data[0].v_pointer), G_VALUE_TYPE (dest_value)))
    dest_value->data[0].v_pointer = g_param_spec_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static xpointer_t
value_param_peek_pointer (const xvalue_t *value)
{
  return value->data[0].v_pointer;
}

static xchar_t*
value_param_collect_value (xvalue_t      *value,
			   xuint_t        n_collect_values,
			   xtype_c_value_t *collect_values,
			   xuint_t        collect_flags)
{
  if (collect_values[0].v_pointer)
    {
      xparam_spec_t *param = collect_values[0].v_pointer;

      if (param->xtype_instance.g_class == NULL)
	return xstrconcat ("invalid unclassed param spec pointer for value type '",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      else if (!xvalue_type_compatible (G_PARAM_SPEC_TYPE (param), G_VALUE_TYPE (value)))
	return xstrconcat ("invalid param spec type '",
			    G_PARAM_SPEC_TYPE_NAME (param),
			    "' for value type '",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      value->data[0].v_pointer = g_param_spec_ref (param);
    }
  else
    value->data[0].v_pointer = NULL;

  return NULL;
}

static xchar_t*
value_param_lcopy_value (const xvalue_t *value,
			 xuint_t         n_collect_values,
			 xtype_c_value_t  *collect_values,
			 xuint_t         collect_flags)
{
  xparam_spec_t **param_p = collect_values[0].v_pointer;

  g_return_val_if_fail (param_p != NULL, xstrdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  if (!value->data[0].v_pointer)
    *param_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *param_p = value->data[0].v_pointer;
  else
    *param_p = g_param_spec_ref (value->data[0].v_pointer);

  return NULL;
}


/* --- param spec pool --- */
/**
 * GParamSpecPool:
 *
 * A #GParamSpecPool maintains a collection of #GParamSpecs which can be
 * quickly accessed by owner and name.
 *
 * The implementation of the #xobject_t property system uses such a pool to
 * store the #GParamSpecs of the properties all object types.
 */
struct _GParamSpecPool
{
  xmutex_t       mutex;
  xboolean_t     type_prefixing;
  xhashtable_t  *hash_table;
};

static xuint_t
param_spec_pool_hash (xconstpointer key_spec)
{
  const xparam_spec_t *key = key_spec;
  const xchar_t *p;
  xuint_t h = key->owner_type;

  for (p = key->name; *p; p++)
    h = (h << 5) - h + *p;

  return h;
}

static xboolean_t
param_spec_pool_equals (xconstpointer key_spec_1,
			xconstpointer key_spec_2)
{
  const xparam_spec_t *key1 = key_spec_1;
  const xparam_spec_t *key2 = key_spec_2;

  return (key1->owner_type == key2->owner_type &&
	  strcmp (key1->name, key2->name) == 0);
}

/**
 * g_param_spec_pool_new:
 * @type_prefixing: Whether the pool will support type-prefixed property names.
 *
 * Creates a new #GParamSpecPool.
 *
 * If @type_prefixing is %TRUE, lookups in the newly created pool will
 * allow to specify the owner as a colon-separated prefix of the
 * property name, like "GtkContainer:border-width". This feature is
 * deprecated, so you should always set @type_prefixing to %FALSE.
 *
 * Returns: (transfer full): a newly allocated #GParamSpecPool.
 */
GParamSpecPool*
g_param_spec_pool_new (xboolean_t type_prefixing)
{
  static xmutex_t init_mutex;
  GParamSpecPool *pool = g_new (GParamSpecPool, 1);

  memcpy (&pool->mutex, &init_mutex, sizeof (init_mutex));
  pool->type_prefixing = type_prefixing != FALSE;
  pool->hash_table = xhash_table_new (param_spec_pool_hash, param_spec_pool_equals);

  return pool;
}

/**
 * g_param_spec_pool_insert:
 * @pool: a #GParamSpecPool.
 * @pspec: (transfer none) (not nullable): the #xparam_spec_t to insert
 * @owner_type: a #xtype_t identifying the owner of @pspec
 *
 * Inserts a #xparam_spec_t in the pool.
 */
void
g_param_spec_pool_insert (GParamSpecPool *pool,
			  xparam_spec_t     *pspec,
			  xtype_t           owner_type)
{
  const xchar_t *p;

  if (pool && pspec && owner_type > 0 && pspec->owner_type == 0)
    {
      for (p = pspec->name; *p; p++)
	{
	  if (!strchr (G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", *p))
	    {
	      g_warning (G_STRLOC ": pspec name \"%s\" contains invalid characters", pspec->name);
	      return;
	    }
	}
      g_mutex_lock (&pool->mutex);
      pspec->owner_type = owner_type;
      g_param_spec_ref (pspec);
      xhash_table_add (pool->hash_table, pspec);
      g_mutex_unlock (&pool->mutex);
    }
  else
    {
      g_return_if_fail (pool != NULL);
      g_return_if_fail (pspec);
      g_return_if_fail (owner_type > 0);
      g_return_if_fail (pspec->owner_type == 0);
    }
}

/**
 * g_param_spec_pool_remove:
 * @pool: a #GParamSpecPool
 * @pspec: (transfer none) (not nullable): the #xparam_spec_t to remove
 *
 * Removes a #xparam_spec_t from the pool.
 */
void
g_param_spec_pool_remove (GParamSpecPool *pool,
			  xparam_spec_t     *pspec)
{
  if (pool && pspec)
    {
      g_mutex_lock (&pool->mutex);
      if (xhash_table_remove (pool->hash_table, pspec))
	g_param_spec_unref (pspec);
      else
	g_warning (G_STRLOC ": attempt to remove unknown pspec '%s' from pool", pspec->name);
      g_mutex_unlock (&pool->mutex);
    }
  else
    {
      g_return_if_fail (pool != NULL);
      g_return_if_fail (pspec);
    }
}

static inline xparam_spec_t*
param_spec_ht_lookup (xhashtable_t  *hash_table,
		      const xchar_t *param_name,
		      xtype_t        owner_type,
		      xboolean_t     walk_ancestors)
{
  xparam_spec_t key, *pspec;

  key.owner_type = owner_type;
  key.name = (xchar_t*) param_name;
  if (walk_ancestors)
    do
      {
	pspec = xhash_table_lookup (hash_table, &key);
	if (pspec)
	  return pspec;
	key.owner_type = xtype_parent (key.owner_type);
      }
    while (key.owner_type);
  else
    pspec = xhash_table_lookup (hash_table, &key);

  if (!pspec && !is_canonical (param_name))
    {
      xchar_t *canonical;

      canonical = xstrdup (key.name);
      canonicalize_key (canonical);

      /* try canonicalized form */
      key.name = canonical;
      key.owner_type = owner_type;

      if (walk_ancestors)
        do
          {
            pspec = xhash_table_lookup (hash_table, &key);
            if (pspec)
              {
                g_free (canonical);
                return pspec;
              }
            key.owner_type = xtype_parent (key.owner_type);
          }
        while (key.owner_type);
      else
        pspec = xhash_table_lookup (hash_table, &key);

      g_free (canonical);
    }

  return pspec;
}

/**
 * g_param_spec_pool_lookup:
 * @pool: a #GParamSpecPool
 * @param_name: the name to look for
 * @owner_type: the owner to look for
 * @walk_ancestors: If %TRUE, also try to find a #xparam_spec_t with @param_name
 *  owned by an ancestor of @owner_type.
 *
 * Looks up a #xparam_spec_t in the pool.
 *
 * Returns: (transfer none) (nullable): The found #xparam_spec_t, or %NULL if no
 * matching #xparam_spec_t was found.
 */
xparam_spec_t*
g_param_spec_pool_lookup (GParamSpecPool *pool,
			  const xchar_t    *param_name,
			  xtype_t           owner_type,
			  xboolean_t        walk_ancestors)
{
  xparam_spec_t *pspec;
  xchar_t *delim;

  g_return_val_if_fail (pool != NULL, NULL);
  g_return_val_if_fail (param_name != NULL, NULL);

  g_mutex_lock (&pool->mutex);

  delim = pool->type_prefixing ? strchr (param_name, ':') : NULL;

  /* try quick and away, i.e. without prefix */
  if (!delim)
    {
      pspec = param_spec_ht_lookup (pool->hash_table, param_name, owner_type, walk_ancestors);
      g_mutex_unlock (&pool->mutex);

      return pspec;
    }

  /* strip type prefix */
  if (pool->type_prefixing && delim[1] == ':')
    {
      xuint_t l = delim - param_name;
      xchar_t stack_buffer[32], *buffer = l < 32 ? stack_buffer : g_new (xchar_t, l + 1);
      xtype_t type;

      strncpy (buffer, param_name, delim - param_name);
      buffer[l] = 0;
      type = xtype_from_name (buffer);
      if (l >= 32)
	g_free (buffer);
      if (type)		/* type==0 isn't a valid type pefix */
	{
	  /* sanity check, these cases don't make a whole lot of sense */
	  if ((!walk_ancestors && type != owner_type) || !xtype_is_a (owner_type, type))
	    {
	      g_mutex_unlock (&pool->mutex);

	      return NULL;
	    }
	  owner_type = type;
	  param_name += l + 2;
	  pspec = param_spec_ht_lookup (pool->hash_table, param_name, owner_type, walk_ancestors);
	  g_mutex_unlock (&pool->mutex);

	  return pspec;
	}
    }
  /* malformed param_name */

  g_mutex_unlock (&pool->mutex);

  return NULL;
}

static void
pool_list (xpointer_t key,
	   xpointer_t value,
	   xpointer_t user_data)
{
  xparam_spec_t *pspec = value;
  xpointer_t *data = user_data;
  xtype_t owner_type = (xtype_t) data[1];

  if (owner_type == pspec->owner_type)
    data[0] = xlist_prepend (data[0], pspec);
}

/**
 * g_param_spec_pool_list_owned:
 * @pool: a #GParamSpecPool
 * @owner_type: the owner to look for
 *
 * Gets an #xlist_t of all #GParamSpecs owned by @owner_type in
 * the pool.
 *
 * Returns: (transfer container) (element-type xobject_t.ParamSpec): a
 *          #xlist_t of all #GParamSpecs owned by @owner_type in
 *          the pool#GParamSpecs.
 */
xlist_t*
g_param_spec_pool_list_owned (GParamSpecPool *pool,
			      xtype_t           owner_type)
{
  xpointer_t data[2];

  g_return_val_if_fail (pool != NULL, NULL);
  g_return_val_if_fail (owner_type > 0, NULL);

  g_mutex_lock (&pool->mutex);
  data[0] = NULL;
  data[1] = (xpointer_t) owner_type;
  xhash_table_foreach (pool->hash_table, pool_list, &data);
  g_mutex_unlock (&pool->mutex);

  return data[0];
}

static xint_t
pspec_compare_id (xconstpointer a,
		  xconstpointer b)
{
  const xparam_spec_t *pspec1 = a, *pspec2 = b;

  if (pspec1->param_id < pspec2->param_id)
    return -1;

  if (pspec1->param_id > pspec2->param_id)
    return 1;

  return strcmp (pspec1->name, pspec2->name);
}

static inline xboolean_t
should_list_pspec (xparam_spec_t *pspec,
                   xtype_t      owner_type,
                   xhashtable_t *ht)
{
  xparam_spec_t *found;

  /* Remove paramspecs that are redirected, and also paramspecs
   * that have are overridden by non-redirected properties.
   * The idea is to get the single paramspec for each name that
   * best corresponds to what the application sees.
   */
  if (g_param_spec_get_redirect_target (pspec))
    return FALSE;

  found = param_spec_ht_lookup (ht, pspec->name, owner_type, TRUE);
  if (found != pspec)
    {
      xparam_spec_t *redirect = g_param_spec_get_redirect_target (found);
      if (redirect != pspec)
        return FALSE;
    }

  return TRUE;
}

static void
pool_depth_list (xpointer_t key,
		 xpointer_t value,
		 xpointer_t user_data)
{
  xparam_spec_t *pspec = value;
  xpointer_t *data = user_data;
  xslist_t **slists = data[0];
  xtype_t owner_type = (xtype_t) data[1];
  xhashtable_t *ht = data[2];
  int *count = data[3];

  if (xtype_is_a (owner_type, pspec->owner_type) &&
      should_list_pspec (pspec, owner_type, ht))
    {
      if (XTYPE_IS_INTERFACE (pspec->owner_type))
	{
	  slists[0] = xslist_prepend (slists[0], pspec);
          *count = *count + 1;
	}
      else
	{
          xuint_t d = xtype_depth (pspec->owner_type);

          slists[d - 1] = xslist_prepend (slists[d - 1], pspec);
          *count = *count + 1;
	}
    }
}

/* We handle interfaces specially since we don't want to
 * count interface prerequisites like normal inheritance;
 * the property comes from the direct inheritance from
 * the prerequisite class, not from the interface that
 * prerequires it.
 *
 * also 'depth' isn't a meaningful concept for interface
 * prerequites.
 */
static void
pool_depth_list_for_interface (xpointer_t key,
			       xpointer_t value,
			       xpointer_t user_data)
{
  xparam_spec_t *pspec = value;
  xpointer_t *data = user_data;
  xslist_t **slists = data[0];
  xtype_t owner_type = (xtype_t) data[1];
  xhashtable_t *ht = data[2];
  int *count = data[3];

  if (pspec->owner_type == owner_type &&
      should_list_pspec (pspec, owner_type, ht))
    {
      slists[0] = xslist_prepend (slists[0], pspec);
      *count = *count + 1;
    }
}

/**
 * g_param_spec_pool_list:
 * @pool: a #GParamSpecPool
 * @owner_type: the owner to look for
 * @n_pspecs_p: (out): return location for the length of the returned array
 *
 * Gets an array of all #GParamSpecs owned by @owner_type in
 * the pool.
 *
 * Returns: (array length=n_pspecs_p) (transfer container): a newly
 *          allocated array containing pointers to all #GParamSpecs
 *          owned by @owner_type in the pool
 */
xparam_spec_t**
g_param_spec_pool_list (GParamSpecPool *pool,
			xtype_t           owner_type,
			xuint_t          *n_pspecs_p)
{
  xparam_spec_t **pspecs, **p;
  xslist_t **slists, *node;
  xpointer_t data[4];
  xuint_t d, i;
  int n_pspecs = 0;

  g_return_val_if_fail (pool != NULL, NULL);
  g_return_val_if_fail (owner_type > 0, NULL);
  g_return_val_if_fail (n_pspecs_p != NULL, NULL);

  g_mutex_lock (&pool->mutex);
  d = xtype_depth (owner_type);
  slists = g_new0 (xslist_t*, d);
  data[0] = slists;
  data[1] = (xpointer_t) owner_type;
  data[2] = pool->hash_table;
  data[3] = &n_pspecs;

  xhash_table_foreach (pool->hash_table,
                        XTYPE_IS_INTERFACE (owner_type) ?
                          pool_depth_list_for_interface :
                          pool_depth_list,
                        &data);

  pspecs = g_new (xparam_spec_t*, n_pspecs + 1);
  p = pspecs;
  for (i = 0; i < d; i++)
    {
      slists[i] = xslist_sort (slists[i], pspec_compare_id);
      for (node = slists[i]; node; node = node->next)
	*p++ = node->data;
      xslist_free (slists[i]);
    }
  *p++ = NULL;
  g_free (slists);
  g_mutex_unlock (&pool->mutex);

  *n_pspecs_p = n_pspecs;

  return pspecs;
}

/* --- auxiliary functions --- */
typedef struct
{
  /* class portion */
  xtype_t           value_type;
  void          (*finalize)             (xparam_spec_t   *pspec);
  void          (*value_set_default)    (xparam_spec_t   *pspec,
					 xvalue_t       *value);
  xboolean_t      (*value_validate)       (xparam_spec_t   *pspec,
					 xvalue_t       *value);
  xint_t          (*values_cmp)           (xparam_spec_t   *pspec,
					 const xvalue_t *value1,
					 const xvalue_t *value2);
} ParamSpecClassInfo;

static void
param_spec_generic_class_init (xpointer_t g_class,
			       xpointer_t class_data)
{
  GParamSpecClass *class = g_class;
  ParamSpecClassInfo *info = class_data;

  class->value_type = info->value_type;
  if (info->finalize)
    class->finalize = info->finalize;			/* optional */
  class->value_set_default = info->value_set_default;
  if (info->value_validate)
    class->value_validate = info->value_validate;	/* optional */
  class->values_cmp = info->values_cmp;
  g_free (class_data);
}

static void
default_value_set_default (xparam_spec_t *pspec,
			   xvalue_t     *value)
{
  /* value is already zero initialized */
}

static xint_t
default_values_cmp (xparam_spec_t   *pspec,
		    const xvalue_t *value1,
		    const xvalue_t *value2)
{
  return memcmp (&value1->data, &value2->data, sizeof (value1->data));
}

/**
 * g_param_type_register_static:
 * @name: 0-terminated string used as the name of the new #xparam_spec_t type.
 * @pspec_info: The #GParamSpecTypeInfo for this #xparam_spec_t type.
 *
 * Registers @name as the name of a new static type derived
 * from %XTYPE_PARAM.
 *
 * The type system uses the information contained in the #GParamSpecTypeInfo
 * structure pointed to by @info to manage the #xparam_spec_t type and its
 * instances.
 *
 * Returns: The new type identifier.
 */
xtype_t
g_param_type_register_static (const xchar_t              *name,
			      const GParamSpecTypeInfo *pspec_info)
{
  xtype_info_t info = {
    sizeof (GParamSpecClass),      /* class_size */
    NULL,                          /* base_init */
    NULL,                          /* base_destroy */
    param_spec_generic_class_init, /* class_init */
    NULL,                          /* class_destroy */
    NULL,                          /* class_data */
    0,                             /* instance_size */
    16,                            /* n_preallocs */
    NULL,                          /* instance_init */
    NULL,                          /* value_table */
  };
  ParamSpecClassInfo *cinfo;

  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (pspec_info != NULL, 0);
  g_return_val_if_fail (xtype_from_name (name) == 0, 0);
  g_return_val_if_fail (pspec_info->instance_size >= sizeof (xparam_spec_t), 0);
  g_return_val_if_fail (xtype_name (pspec_info->value_type) != NULL, 0);
  /* default: g_return_val_if_fail (pspec_info->value_set_default != NULL, 0); */
  /* optional: g_return_val_if_fail (pspec_info->value_validate != NULL, 0); */
  /* default: g_return_val_if_fail (pspec_info->values_cmp != NULL, 0); */

  info.instance_size = pspec_info->instance_size;
  info.n_preallocs = pspec_info->n_preallocs;
  info.instance_init = (xinstance_init_func_t) pspec_info->instance_init;
  cinfo = g_new (ParamSpecClassInfo, 1);
  cinfo->value_type = pspec_info->value_type;
  cinfo->finalize = pspec_info->finalize;
  cinfo->value_set_default = pspec_info->value_set_default ? pspec_info->value_set_default : default_value_set_default;
  cinfo->value_validate = pspec_info->value_validate;
  cinfo->values_cmp = pspec_info->values_cmp ? pspec_info->values_cmp : default_values_cmp;
  info.class_data = cinfo;

  return xtype_register_static (XTYPE_PARAM, name, &info, 0);
}

/**
 * xvalue_set_param:
 * @value: a valid #xvalue_t of type %XTYPE_PARAM
 * @param: (nullable): the #xparam_spec_t to be set
 *
 * Set the contents of a %XTYPE_PARAM #xvalue_t to @param.
 */
void
xvalue_set_param (xvalue_t     *value,
		   xparam_spec_t *param)
{
  g_return_if_fail (G_VALUE_HOLDS_PARAM (value));
  if (param)
    g_return_if_fail (X_IS_PARAM_SPEC (param));

  if (value->data[0].v_pointer)
    g_param_spec_unref (value->data[0].v_pointer);
  value->data[0].v_pointer = param;
  if (value->data[0].v_pointer)
    g_param_spec_ref (value->data[0].v_pointer);
}

/**
 * xvalue_set_param_take_ownership: (skip)
 * @value: a valid #xvalue_t of type %XTYPE_PARAM
 * @param: (nullable): the #xparam_spec_t to be set
 *
 * This is an internal function introduced mainly for C marshallers.
 *
 * Deprecated: 2.4: Use xvalue_take_param() instead.
 */
void
xvalue_set_param_take_ownership (xvalue_t     *value,
				  xparam_spec_t *param)
{
  xvalue_take_param (value, param);
}

/**
 * xvalue_take_param: (skip)
 * @value: a valid #xvalue_t of type %XTYPE_PARAM
 * @param: (nullable): the #xparam_spec_t to be set
 *
 * Sets the contents of a %XTYPE_PARAM #xvalue_t to @param and takes
 * over the ownership of the caller’s reference to @param; the caller
 * doesn’t have to unref it any more.
 *
 * Since: 2.4
 */
void
xvalue_take_param (xvalue_t     *value,
		    xparam_spec_t *param)
{
  g_return_if_fail (G_VALUE_HOLDS_PARAM (value));
  if (param)
    g_return_if_fail (X_IS_PARAM_SPEC (param));

  if (value->data[0].v_pointer)
    g_param_spec_unref (value->data[0].v_pointer);
  value->data[0].v_pointer = param; /* we take over the reference count */
}

/**
 * xvalue_get_param:
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_PARAM
 *
 * Get the contents of a %XTYPE_PARAM #xvalue_t.
 *
 * Returns: (transfer none): #xparam_spec_t content of @value
 */
xparam_spec_t*
xvalue_get_param (const xvalue_t *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_PARAM (value), NULL);

  return value->data[0].v_pointer;
}

/**
 * xvalue_dup_param: (skip)
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_PARAM
 *
 * Get the contents of a %XTYPE_PARAM #xvalue_t, increasing its
 * reference count.
 *
 * Returns: (transfer full): #xparam_spec_t content of @value, should be
 *     unreferenced when no longer needed.
 */
xparam_spec_t*
xvalue_dup_param (const xvalue_t *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_PARAM (value), NULL);

  return value->data[0].v_pointer ? g_param_spec_ref (value->data[0].v_pointer) : NULL;
}

/**
 * g_param_spec_get_default_value:
 * @pspec: a #xparam_spec_t
 *
 * Gets the default value of @pspec as a pointer to a #xvalue_t.
 *
 * The #xvalue_t will remain valid for the life of @pspec.
 *
 * Returns: a pointer to a #xvalue_t which must not be modified
 *
 * Since: 2.38
 **/
const xvalue_t *
g_param_spec_get_default_value (xparam_spec_t *pspec)
{
  GParamSpecPrivate *priv = g_param_spec_get_private (pspec);

  /* We use the type field of the xvalue_t as the key for the once because
   * it will be zero before it is initialised and non-zero after.  We
   * have to take care that we don't write a non-zero value to the type
   * field before we are completely done, however, because then another
   * thread could come along and find the value partially-initialised.
   *
   * In order to accomplish this we store the default value in a
   * stack-allocated xvalue_t.  We then set the type field in that value
   * to zero and copy the contents into place.  We then end by storing
   * the type as the last step in order to ensure that we're completely
   * done before a g_once_init_enter() could take the fast path in
   * another thread.
   */
  if (g_once_init_enter (&priv->default_value.g_type))
    {
      xvalue_t default_value = G_VALUE_INIT;

      xvalue_init (&default_value, pspec->value_type);
      g_param_value_set_default (pspec, &default_value);

      /* store all but the type */
      memcpy (priv->default_value.data, default_value.data, sizeof (default_value.data));

      g_once_init_leave (&priv->default_value.g_type, pspec->value_type);
    }

  return &priv->default_value;
}

/**
 * g_param_spec_get_name_quark:
 * @pspec: a #xparam_spec_t
 *
 * Gets the xquark for the name.
 *
 * Returns: the xquark for @pspec->name.
 *
 * Since: 2.46
 */
xquark
g_param_spec_get_name_quark (xparam_spec_t *pspec)
{
  GParamSpecPrivate *priv = g_param_spec_get_private (pspec);

  /* Return the quark that we've stashed away at creation time.
   * This lets us avoid a lock and a hash table lookup when
   * dispatching property change notification.
   */

  return priv->name_quark;
}
