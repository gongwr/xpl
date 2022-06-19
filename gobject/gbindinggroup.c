/* xobject_t - GLib Type, Object, Parameter and Signal Library
 *
 * Copyright (C) 2015-2022 Christian Hergert <christian@hergert.me>
 * Copyright (C) 2015 Garrett Regier <garrettregier@gmail.com>
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include "glib.h"
#include "glibintl.h"

#include "gbindinggroup.h"
#include "gparamspecs.h"

/**
 * SECTION:gbindinggroup
 * @Title: xbinding_group_t
 * @Short_description: Binding multiple properties as a group
 * @include: glib-object.h
 *
 * The #xbinding_group_t can be used to bind multiple properties
 * from an object collectively.
 *
 * Use the various methods to bind properties from a single source
 * object to multiple destination objects. Properties can be bound
 * bidirectionally and are connected when the source object is set
 * with xbinding_group_set_source().
 *
 * Since: 2.72
 */

#if 0
# define DEBUG_BINDINGS
#endif

struct _xbinding_group
{
  xobject_t    parent_instance;
  xmutex_t     mutex;
  xobject_t   *source;         /* (owned weak) */
  xptr_array_t *lazy_bindings;  /* (owned) (element-type LazyBinding) */
};

typedef struct _xbinding_group_class
{
  xobject_class_t parent_class;
} xbinding_group_class_t;

typedef struct
{
  xbinding_group_t      *group;  /* (unowned) */
  const char         *source_property;  /* (interned) */
  const char         *target_property;  /* (interned) */
  xobject_t            *target;  /* (owned weak) */
  xbinding_t           *binding;  /* (unowned) */
  xpointer_t            user_data;
  xdestroy_notify_t      user_data_destroy;
  xpointer_t            transform_to;  /* (nullable) (owned) */
  xpointer_t            transform_from;  /* (nullable) (owned) */
  xbinding_flags_t       binding_flags;
  xuint_t               usinxclosures : 1;
} LazyBinding;

XDEFINE_TYPE (xbinding_group, xbinding_group, XTYPE_OBJECT)

typedef enum
{
  PROP_SOURCE = 1,
  N_PROPS
} xbinding_group_property_t;

static void lazy_binding_free (xpointer_t data);

static xparam_spec_t *properties[N_PROPS];

static void
xbinding_group_connect (xbinding_group_t *self,
                         LazyBinding   *lazy_binding)
{
  xbinding_t *binding;

  xassert (X_IS_BINDING_GROUP (self));
  xassert (self->source != NULL);
  xassert (lazy_binding != NULL);
  xassert (lazy_binding->binding == NULL);
  xassert (lazy_binding->target != NULL);
  xassert (lazy_binding->target_property != NULL);
  xassert (lazy_binding->source_property != NULL);

#ifdef DEBUG_BINDINGS
  {
    xflags_class_t *flags_class;
    g_autofree xchar_t *flags_str = NULL;

    flags_class = xtype_class_ref (XTYPE_BINDING_FLAGS);
    flags_str = xflags_to_string (flags_class, lazy_binding->binding_flags);

    g_print ("Binding %s(%p):%s to %s(%p):%s (flags=%s)\n",
             G_OBJECT_TYPE_NAME (self->source),
             self->source,
             lazy_binding->source_property,
             G_OBJECT_TYPE_NAME (lazy_binding->target),
             lazy_binding->target,
             lazy_binding->target_property,
             flags_str);

    xtype_class_unref (flags_class);
  }
#endif

  if (!lazy_binding->usinxclosures)
    binding = xobject_bind_property_full (self->source,
                                           lazy_binding->source_property,
                                           lazy_binding->target,
                                           lazy_binding->target_property,
                                           lazy_binding->binding_flags,
                                           lazy_binding->transform_to,
                                           lazy_binding->transform_from,
                                           lazy_binding->user_data,
                                           NULL);
  else
    binding = xobject_bind_property_with_closures (self->source,
                                                    lazy_binding->source_property,
                                                    lazy_binding->target,
                                                    lazy_binding->target_property,
                                                    lazy_binding->binding_flags,
                                                    lazy_binding->transform_to,
                                                    lazy_binding->transform_from);

  lazy_binding->binding = binding;
}

static void
xbinding_group_disconnect (LazyBinding *lazy_binding)
{
  xassert (lazy_binding != NULL);

  if (lazy_binding->binding != NULL)
    {
      xbinding_unbind (lazy_binding->binding);
      lazy_binding->binding = NULL;
    }
}

static void
xbinding_group__source_weak_notify (xpointer_t  data,
                                     xobject_t  *where_object_was)
{
  xbinding_group_t *self = data;
  xuint_t i;

  xassert (X_IS_BINDING_GROUP (self));

  g_mutex_lock (&self->mutex);

  self->source = NULL;

  for (i = 0; i < self->lazy_bindings->len; i++)
    {
      LazyBinding *lazy_binding = xptr_array_index (self->lazy_bindings, i);

      lazy_binding->binding = NULL;
    }

  g_mutex_unlock (&self->mutex);
}

static void
xbinding_group__target_weak_notify (xpointer_t  data,
                                     xobject_t  *where_object_was)
{
  xbinding_group_t *self = data;
  LazyBinding *to_free = NULL;
  xuint_t i;

  xassert (X_IS_BINDING_GROUP (self));

  g_mutex_lock (&self->mutex);

  for (i = 0; i < self->lazy_bindings->len; i++)
    {
      LazyBinding *lazy_binding = xptr_array_index (self->lazy_bindings, i);

      if (lazy_binding->target == where_object_was)
        {
          lazy_binding->target = NULL;
          lazy_binding->binding = NULL;

          to_free = xptr_array_steal_index_fast (self->lazy_bindings, i);
          break;
        }
    }

  g_mutex_unlock (&self->mutex);

  if (to_free != NULL)
    lazy_binding_free (to_free);
}

static void
lazy_binding_free (xpointer_t data)
{
  LazyBinding *lazy_binding = data;

  if (lazy_binding->target != NULL)
    {
      xobject_weak_unref (lazy_binding->target,
                           xbinding_group__target_weak_notify,
                           lazy_binding->group);
      lazy_binding->target = NULL;
    }

  xbinding_group_disconnect (lazy_binding);

  lazy_binding->group = NULL;
  lazy_binding->source_property = NULL;
  lazy_binding->target_property = NULL;

  if (lazy_binding->user_data_destroy)
    lazy_binding->user_data_destroy (lazy_binding->user_data);

  if (lazy_binding->usinxclosures)
    {
      g_clear_pointer (&lazy_binding->transform_to, xclosure_unref);
      g_clear_pointer (&lazy_binding->transform_from, xclosure_unref);
    }

  g_slice_free (LazyBinding, lazy_binding);
}

static void
xbinding_group_dispose (xobject_t *object)
{
  xbinding_group_t *self = (xbinding_group_t *)object;
  LazyBinding **lazy_bindings = NULL;
  xsize_t len = 0;
  xsize_t i;

  xassert (X_IS_BINDING_GROUP (self));

  g_mutex_lock (&self->mutex);

  if (self->source != NULL)
    {
      xobject_weak_unref (self->source,
                           xbinding_group__source_weak_notify,
                           self);
      self->source = NULL;
    }

  if (self->lazy_bindings->len > 0)
    lazy_bindings = (LazyBinding **)xptr_array_steal (self->lazy_bindings, &len);

  g_mutex_unlock (&self->mutex);

  /* Free bindings without holding self->mutex to avoid re-entrancy
   * from collateral damage through release of binding closure data,
   * GDataList, etc.
   */
  for (i = 0; i < len; i++)
    lazy_binding_free (lazy_bindings[i]);
  g_free (lazy_bindings);

  XOBJECT_CLASS (xbinding_group_parent_class)->dispose (object);
}

static void
xbinding_group_finalize (xobject_t *object)
{
  xbinding_group_t *self = (xbinding_group_t *)object;

  xassert (self->lazy_bindings != NULL);
  xassert (self->lazy_bindings->len == 0);

  g_clear_pointer (&self->lazy_bindings, xptr_array_unref);
  g_mutex_clear (&self->mutex);

  XOBJECT_CLASS (xbinding_group_parent_class)->finalize (object);
}

static void
xbinding_group_get_property (xobject_t    *object,
                              xuint_t       prop_id,
                              xvalue_t     *value,
                              xparam_spec_t *pspec)
{
  xbinding_group_t *self = XBINDING_GROUP (object);

  switch ((xbinding_group_property_t) prop_id)
    {
    case PROP_SOURCE:
      xvalue_take_object (value, xbinding_group_dup_source (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xbinding_group_set_property (xobject_t      *object,
                              xuint_t         prop_id,
                              const xvalue_t *value,
                              xparam_spec_t   *pspec)
{
  xbinding_group_t *self = XBINDING_GROUP (object);

  switch ((xbinding_group_property_t) prop_id)
    {
    case PROP_SOURCE:
      xbinding_group_set_source (self, xvalue_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xbinding_group_class_init (xbinding_group_class_t *klass)
{
  xobject_class_t *object_class = XOBJECT_CLASS (klass);

  object_class->dispose = xbinding_group_dispose;
  object_class->finalize = xbinding_group_finalize;
  object_class->get_property = xbinding_group_get_property;
  object_class->set_property = xbinding_group_set_property;

  /**
   * xbinding_group_t:source: (nullable)
   *
   * The source object used for binding properties.
   *
   * Since: 2.72
   */
  properties[PROP_SOURCE] =
      xparam_spec_object ("source",
                           "Source",
                           "The source xobject_t used for binding properties.",
                           XTYPE_OBJECT,
                           (XPARAM_READWRITE | XPARAM_EXPLICIT_NOTIFY | XPARAM_STATIC_STRINGS));

  xobject_class_install_properties (object_class, N_PROPS, properties);
}

static void
xbinding_group_init (xbinding_group_t *self)
{
  g_mutex_init (&self->mutex);
  self->lazy_bindings = xptr_array_new_with_free_func (lazy_binding_free);
}

/**
 * xbinding_group_new:
 *
 * Creates a new #xbinding_group_t.
 *
 * Returns: (transfer full): a new #xbinding_group_t
 *
 * Since: 2.72
 */
xbinding_group_t *
xbinding_group_new (void)
{
  return xobject_new (XTYPE_BINDING_GROUP, NULL);
}

/**
 * xbinding_group_dup_source:
 * @self: the #xbinding_group_t
 *
 * Gets the source object used for binding properties.
 *
 * Returns: (transfer none) (nullable) (type xobject_t): a #xobject_t or %NULL.
 *
 * Since: 2.72
 */
xpointer_t
xbinding_group_dup_source (xbinding_group_t *self)
{
  xobject_t *source;

  xreturn_val_if_fail (X_IS_BINDING_GROUP (self), NULL);

  g_mutex_lock (&self->mutex);
  source = self->source ? xobject_ref (self->source) : NULL;
  g_mutex_unlock (&self->mutex);

  return source;
}

static xboolean_t
xbinding_group_check_source (xbinding_group_t *self,
                              xpointer_t       source)
{
  xuint_t i;

  xassert (X_IS_BINDING_GROUP (self));
  xassert (!source || X_IS_OBJECT (source));

  for (i = 0; i < self->lazy_bindings->len; i++)
    {
      LazyBinding *lazy_binding = xptr_array_index (self->lazy_bindings, i);

      xreturn_val_if_fail (xobject_class_find_property (G_OBJECT_GET_CLASS (source),
                                                          lazy_binding->source_property) != NULL,
                            FALSE);
    }

  return TRUE;
}

/**
 * xbinding_group_set_source:
 * @self: the #xbinding_group_t
 * @source: (type xobject_t) (nullable) (transfer none): the source #xobject_t,
 *   or %NULL to clear it
 *
 * Sets @source as the source object used for creating property
 * bindings. If there is already a source object all bindings from it
 * will be removed.
 *
 * Note that all properties that have been bound must exist on @source.
 *
 * Since: 2.72
 */
void
xbinding_group_set_source (xbinding_group_t *self,
                            xpointer_t       source)
{
  xboolean_t notify = FALSE;

  g_return_if_fail (X_IS_BINDING_GROUP (self));
  g_return_if_fail (!source || X_IS_OBJECT (source));
  g_return_if_fail (source != (xpointer_t) self);

  g_mutex_lock (&self->mutex);

  if (source == (xpointer_t) self->source)
    goto unlock;

  if (self->source != NULL)
    {
      xuint_t i;

      xobject_weak_unref (self->source,
                           xbinding_group__source_weak_notify,
                           self);
      self->source = NULL;

      for (i = 0; i < self->lazy_bindings->len; i++)
        {
          LazyBinding *lazy_binding = xptr_array_index (self->lazy_bindings, i);

          xbinding_group_disconnect (lazy_binding);
        }
    }

  if (source != NULL && xbinding_group_check_source (self, source))
    {
      xuint_t i;

      self->source = source;
      xobject_weak_ref (self->source,
                         xbinding_group__source_weak_notify,
                         self);

      for (i = 0; i < self->lazy_bindings->len; i++)
        {
          LazyBinding *lazy_binding;

          lazy_binding = xptr_array_index (self->lazy_bindings, i);
          xbinding_group_connect (self, lazy_binding);
        }
    }

  notify = TRUE;

unlock:
  g_mutex_unlock (&self->mutex);

  if (notify)
    xobject_notify_by_pspec (G_OBJECT (self), properties[PROP_SOURCE]);
}

static void
xbinding_group_bind_helper (xbinding_group_t  *self,
                             const xchar_t    *source_property,
                             xpointer_t        target,
                             const xchar_t    *target_property,
                             xbinding_flags_t   flags,
                             xpointer_t        transform_to,
                             xpointer_t        transform_from,
                             xpointer_t        user_data,
                             xdestroy_notify_t  user_data_destroy,
                             xboolean_t        usinxclosures)
{
  LazyBinding *lazy_binding;

  g_return_if_fail (X_IS_BINDING_GROUP (self));
  g_return_if_fail (source_property != NULL);
  g_return_if_fail (self->source == NULL ||
                    xobject_class_find_property (G_OBJECT_GET_CLASS (self->source),
                                                  source_property) != NULL);
  g_return_if_fail (X_IS_OBJECT (target));
  g_return_if_fail (target_property != NULL);
  g_return_if_fail (xobject_class_find_property (G_OBJECT_GET_CLASS (target),
                                                  target_property) != NULL);
  g_return_if_fail (target != (xpointer_t) self ||
                    strcmp (source_property, target_property) != 0);

  g_mutex_lock (&self->mutex);

  lazy_binding = g_slice_new0 (LazyBinding);
  lazy_binding->group = self;
  lazy_binding->source_property = g_intern_string (source_property);
  lazy_binding->target_property = g_intern_string (target_property);
  lazy_binding->target = target;
  lazy_binding->binding_flags = flags | XBINDING_SYNC_CREATE;
  lazy_binding->user_data = user_data;
  lazy_binding->user_data_destroy = user_data_destroy;
  lazy_binding->transform_to = transform_to;
  lazy_binding->transform_from = transform_from;

  if (usinxclosures)
    {
      lazy_binding->usinxclosures = TRUE;

      if (transform_to != NULL)
        xclosure_sink (xclosure_ref (transform_to));

      if (transform_from != NULL)
        xclosure_sink (xclosure_ref (transform_from));
    }

  xobject_weak_ref (target,
                     xbinding_group__target_weak_notify,
                     self);

  xptr_array_add (self->lazy_bindings, lazy_binding);

  if (self->source != NULL)
    xbinding_group_connect (self, lazy_binding);

  g_mutex_unlock (&self->mutex);
}

/**
 * xbinding_group_bind:
 * @self: the #xbinding_group_t
 * @source_property: the property on the source to bind
 * @target: (type xobject_t) (transfer none) (not nullable): the target #xobject_t
 * @target_property: the property on @target to bind
 * @flags: the flags used to create the #xbinding_t
 *
 * Creates a binding between @source_property on the source object
 * and @target_property on @target. Whenever the @source_property
 * is changed the @target_property is updated using the same value.
 * The binding flag %XBINDING_SYNC_CREATE is automatically specified.
 *
 * See xobject_bind_property() for more information.
 *
 * Since: 2.72
 */
void
xbinding_group_bind (xbinding_group_t *self,
                      const xchar_t   *source_property,
                      xpointer_t       target,
                      const xchar_t   *target_property,
                      xbinding_flags_t  flags)
{
  xbinding_group_bind_full (self, source_property,
                             target, target_property,
                             flags,
                             NULL, NULL,
                             NULL, NULL);
}

/**
 * xbinding_group_bind_full:
 * @self: the #xbinding_group_t
 * @source_property: the property on the source to bind
 * @target: (type xobject_t) (transfer none) (not nullable): the target #xobject_t
 * @target_property: the property on @target to bind
 * @flags: the flags used to create the #xbinding_t
 * @transform_to: (scope notified) (nullable): the transformation function
 *     from the source object to the @target, or %NULL to use the default
 * @transform_from: (scope notified) (nullable): the transformation function
 *     from the @target to the source object, or %NULL to use the default
 * @user_data: custom data to be passed to the transformation
 *             functions, or %NULL
 * @user_data_destroy: function to be called when disposing the binding,
 *     to free the resources used by the transformation functions
 *
 * Creates a binding between @source_property on the source object and
 * @target_property on @target, allowing you to set the transformation
 * functions to be used by the binding. The binding flag
 * %XBINDING_SYNC_CREATE is automatically specified.
 *
 * See xobject_bind_property_full() for more information.
 *
 * Since: 2.72
 */
void
xbinding_group_bind_full (xbinding_group_t         *self,
                           const xchar_t           *source_property,
                           xpointer_t               target,
                           const xchar_t           *target_property,
                           xbinding_flags_t          flags,
                           xbinding_transform_func  transform_to,
                           xbinding_transform_func  transform_from,
                           xpointer_t               user_data,
                           xdestroy_notify_t         user_data_destroy)
{
  xbinding_group_bind_helper (self, source_property,
                               target, target_property,
                               flags,
                               transform_to, transform_from,
                               user_data, user_data_destroy,
                               FALSE);
}

/**
 * xbinding_group_bind_with_closures: (rename-to xbinding_group_bind_full)
 * @self: the #xbinding_group_t
 * @source_property: the property on the source to bind
 * @target: (type xobject_t) (transfer none) (not nullable): the target #xobject_t
 * @target_property: the property on @target to bind
 * @flags: the flags used to create the #xbinding_t
 * @transform_to: (nullable) (transfer none): a #xclosure_t wrapping the
 *     transformation function from the source object to the @target,
 *     or %NULL to use the default
 * @transform_from: (nullable) (transfer none): a #xclosure_t wrapping the
 *     transformation function from the @target to the source object,
 *     or %NULL to use the default
 *
 * Creates a binding between @source_property on the source object and
 * @target_property on @target, allowing you to set the transformation
 * functions to be used by the binding. The binding flag
 * %XBINDING_SYNC_CREATE is automatically specified.
 *
 * This function is the language bindings friendly version of
 * xbinding_group_bind_property_full(), using #GClosures
 * instead of function pointers.
 *
 * See xobject_bind_property_with_closures() for more information.
 *
 * Since: 2.72
 */
void
xbinding_group_bind_with_closures (xbinding_group_t *self,
                                    const xchar_t   *source_property,
                                    xpointer_t       target,
                                    const xchar_t   *target_property,
                                    xbinding_flags_t  flags,
                                    xclosure_t      *transform_to,
                                    xclosure_t      *transform_from)
{
  xbinding_group_bind_helper (self, source_property,
                               target, target_property,
                               flags,
                               transform_to, transform_from,
                               NULL, NULL,
                               TRUE);
}
