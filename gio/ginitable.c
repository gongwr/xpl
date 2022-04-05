/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include "ginitable.h"
#include "glibintl.h"


/**
 * SECTION:ginitable
 * @short_description: Failable object initialization interface
 * @include: gio/gio.h
 * @see_also: #GAsyncInitable
 *
 * #GInitable is implemented by objects that can fail during
 * initialization. If an object implements this interface then
 * it must be initialized as the first thing after construction,
 * either via g_initable_init() or g_async_initable_init_async()
 * (the latter is only available if it also implements #GAsyncInitable).
 *
 * If the object is not initialized, or initialization returns with an
 * error, then all operations on the object except g_object_ref() and
 * g_object_unref() are considered to be invalid, and have undefined
 * behaviour. They will often fail with g_critical() or g_warning(), but
 * this must not be relied on.
 *
 * Users of objects implementing this are not intended to use
 * the interface method directly, instead it will be used automatically
 * in various ways. For C applications you generally just call
 * g_initable_new() directly, or indirectly via a foo_thing_new() wrapper.
 * This will call g_initable_init() under the cover, returning %NULL and
 * setting a #xerror_t on failure (at which point the instance is
 * unreferenced).
 *
 * For bindings in languages where the native constructor supports
 * exceptions the binding could check for objects implementing %GInitable
 * during normal construction and automatically initialize them, throwing
 * an exception on failure.
 */

typedef GInitableIface GInitableInterface;
G_DEFINE_INTERFACE (GInitable, g_initable, XTYPE_OBJECT)

static void
g_initable_default_init (GInitableInterface *iface)
{
}

/**
 * g_initable_init:
 * @initable: a #GInitable.
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Initializes the object implementing the interface.
 *
 * This method is intended for language bindings. If writing in C,
 * g_initable_new() should typically be used instead.
 *
 * The object must be initialized before any real use after initial
 * construction, either with this function or g_async_initable_init_async().
 *
 * Implementations may also support cancellation. If @cancellable is not %NULL,
 * then initialization can be cancelled by triggering the cancellable object
 * from another thread. If the operation was cancelled, the error
 * %G_IO_ERROR_CANCELLED will be returned. If @cancellable is not %NULL and
 * the object doesn't support cancellable initialization the error
 * %G_IO_ERROR_NOT_SUPPORTED will be returned.
 *
 * If the object is not initialized, or initialization returns with an
 * error, then all operations on the object except g_object_ref() and
 * g_object_unref() are considered to be invalid, and have undefined
 * behaviour. See the [introduction][ginitable] for more details.
 *
 * Callers should not assume that a class which implements #GInitable can be
 * initialized multiple times, unless the class explicitly documents itself as
 * supporting this. Generally, a class’ implementation of init() can assume
 * (and assert) that it will only be called once. Previously, this documentation
 * recommended all #GInitable implementations should be idempotent; that
 * recommendation was relaxed in GLib 2.54.
 *
 * If a class explicitly supports being initialized multiple times, it is
 * recommended that the method is idempotent: multiple calls with the same
 * arguments should return the same results. Only the first call initializes
 * the object; further calls return the result of the first call.
 *
 * One reason why a class might need to support idempotent initialization is if
 * it is designed to be used via the singleton pattern, with a
 * #xobject_class_t.constructor that sometimes returns an existing instance.
 * In this pattern, a caller would expect to be able to call g_initable_init()
 * on the result of g_object_new(), regardless of whether it is in fact a new
 * instance.
 *
 * Returns: %TRUE if successful. If an error has occurred, this function will
 *     return %FALSE and set @error appropriately if present.
 *
 * Since: 2.22
 */
xboolean_t
g_initable_init (GInitable     *initable,
		 xcancellable_t  *cancellable,
		 xerror_t       **error)
{
  GInitableIface *iface;

  g_return_val_if_fail (X_IS_INITABLE (initable), FALSE);

  iface = G_INITABLE_GET_IFACE (initable);

  return (* iface->init) (initable, cancellable, error);
}

/**
 * g_initable_new:
 * @object_type: a #xtype_t supporting #GInitable.
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *    ignore.
 * @first_property_name: (nullable): the name of the first property, or %NULL if no
 *     properties
 * @...:  the value if the first property, followed by and other property
 *    value pairs, and ended by %NULL.
 *
 * Helper function for constructing #GInitable object. This is
 * similar to g_object_new() but also initializes the object
 * and returns %NULL, setting an error on failure.
 *
 * Returns: (type xobject_t.Object) (transfer full): a newly allocated
 *      #xobject_t, or %NULL on error
 *
 * Since: 2.22
 */
xpointer_t
g_initable_new (xtype_t          object_type,
		xcancellable_t  *cancellable,
		xerror_t       **error,
		const xchar_t   *first_property_name,
		...)
{
  xobject_t *object;
  va_list var_args;

  va_start (var_args, first_property_name);
  object = g_initable_new_valist (object_type,
				  first_property_name, var_args,
				  cancellable, error);
  va_end (var_args);

  return object;
}

/**
 * g_initable_newv:
 * @object_type: a #xtype_t supporting #GInitable.
 * @n_parameters: the number of parameters in @parameters
 * @parameters: (array length=n_parameters): the parameters to use to construct the object
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Helper function for constructing #GInitable object. This is
 * similar to g_object_newv() but also initializes the object
 * and returns %NULL, setting an error on failure.
 *
 * Returns: (type xobject_t.Object) (transfer full): a newly allocated
 *      #xobject_t, or %NULL on error
 *
 * Since: 2.22
 * Deprecated: 2.54: Use g_object_new_with_properties() and
 * g_initable_init() instead. See #GParameter for more information.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
xpointer_t
g_initable_newv (xtype_t          object_type,
		 xuint_t          n_parameters,
		 GParameter    *parameters,
		 xcancellable_t  *cancellable,
		 xerror_t       **error)
{
  xobject_t *obj;

  g_return_val_if_fail (XTYPE_IS_INITABLE (object_type), NULL);

  obj = g_object_newv (object_type, n_parameters, parameters);

  if (!g_initable_init (G_INITABLE (obj), cancellable, error))
    {
      g_object_unref (obj);
      return NULL;
    }

  return (xpointer_t)obj;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * g_initable_new_valist:
 * @object_type: a #xtype_t supporting #GInitable.
 * @first_property_name: the name of the first property, followed by
 * the value, and other property value pairs, and ended by %NULL.
 * @var_args: The var args list generated from @first_property_name.
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 *     ignore.
 *
 * Helper function for constructing #GInitable object. This is
 * similar to g_object_new_valist() but also initializes the object
 * and returns %NULL, setting an error on failure.
 *
 * Returns: (type xobject_t.Object) (transfer full): a newly allocated
 *      #xobject_t, or %NULL on error
 *
 * Since: 2.22
 */
xobject_t*
g_initable_new_valist (xtype_t          object_type,
		       const xchar_t   *first_property_name,
		       va_list        var_args,
		       xcancellable_t  *cancellable,
		       xerror_t       **error)
{
  xobject_t *obj;

  g_return_val_if_fail (XTYPE_IS_INITABLE (object_type), NULL);

  obj = g_object_new_valist (object_type,
			     first_property_name,
			     var_args);

  if (!g_initable_init (G_INITABLE (obj), cancellable, error))
    {
      g_object_unref (obj);
      return NULL;
    }

  return obj;
}
