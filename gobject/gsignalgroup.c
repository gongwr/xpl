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

#include "gparamspecs.h"
#include "gsignalgroup.h"
#include "gvaluetypes.h"

/**
 * SECTION:gsignalgroup
 * @Title: xsignal_group_t
 * @Short_description: Manage a collection of signals on a xobject_t
 *
 * #xsignal_group_t manages to simplify the process of connecting
 * many signals to a #xobject_t as a group. As such there is no API
 * to disconnect a signal from the group.
 *
 * In particular, this allows you to:
 *
 *  - Change the target instance, which automatically causes disconnection
 *    of the signals from the old instance and connecting to the new instance.
 *  - Block and unblock signals as a group
 *  - Ensuring that blocked state transfers across target instances.
 *
 * One place you might want to use such a structure is with #GtkTextView and
 * #GtkTextBuffer. Often times, you'll need to connect to many signals on
 * #GtkTextBuffer from a #GtkTextView subclass. This allows you to create a
 * signal group during instance construction, simply bind the
 * #GtkTextView:buffer property to #xsignal_group_t:target and connect
 * all the signals you need. When the #GtkTextView:buffer property changes
 * all of the signals will be transitioned correctly.
 *
 * Since: 2.72
 */

struct _GSignalGroup
{
  xobject_t     parent_instance;

  GWeakRef    target_ref;
  GRecMutex   mutex;
  xptr_array_t  *handlers;
  xtype_t       target_type;
  xssize_t      block_count;

  xuint_t       has_bound_at_least_once : 1;
};

typedef struct _xsignal_group_class
{
  xobject_class_t parent_class;

  void (*bind) (xsignal_group_t *self,
                xobject_t      *target);
} xsignal_group_class_t;

typedef struct
{
  xsignal_group_t *group;
  gulong             handler_id;
  xclosure_t          *closure;
  xuint_t              signal_id;
  xquark             signal_detail;
  xuint_t              connect_after : 1;
} SignalHandler;

G_DEFINE_TYPE (xsignal_group, xsignal_group, XTYPE_OBJECT)

typedef enum
{
  PROP_TARGET = 1,
  PROP_TARGET_TYPE,
  LAST_PROP
} xsignal_group_property_t;

enum
{
  BIND,
  UNBIND,
  LAST_SIGNAL
};

static xparam_spec_t *properties[LAST_PROP];
static xuint_t signals[LAST_SIGNAL];

static void
xsignal_group_set_target_type (xsignal_group_t *self,
                                xtype_t         target_type)
{
  g_assert (X_IS_SIGNAL_GROUP (self));
  g_assert (xtype_is_a (target_type, XTYPE_OBJECT));

  self->target_type = target_type;

  /* The class must be created at least once for the signals
   * to be registered, otherwise g_signal_parse_name() will fail
   */
  if (XTYPE_IS_INTERFACE (target_type))
    {
      if (xtype_default_interface_peek (target_type) == NULL)
        xtype_default_interface_unref (xtype_default_interface_ref (target_type));
    }
  else
    {
      if (xtype_class_peek (target_type) == NULL)
        xtype_class_unref (xtype_class_ref (target_type));
    }
}

static void
xsignal_group_gc_handlers (xsignal_group_t *self)
{
  xuint_t i;

  g_assert (X_IS_SIGNAL_GROUP (self));

  /*
   * Remove any handlers for which the closures have become invalid. We do
   * this cleanup lazily to avoid situations where we could have disposal
   * active on both the signal group and the peer object.
   */

  for (i = self->handlers->len; i > 0; i--)
    {
      const SignalHandler *handler = xptr_array_index (self->handlers, i - 1);

      g_assert (handler != NULL);
      g_assert (handler->closure != NULL);

      if (handler->closure->is_invalid)
        xptr_array_remove_index (self->handlers, i - 1);
    }
}

static void
xsignal_group__target_weak_notify (xpointer_t  data,
                                    xobject_t  *where_object_was)
{
  xsignal_group_t *self = data;
  xuint_t i;

  g_assert (X_IS_SIGNAL_GROUP (self));
  g_assert (where_object_was != NULL);

  g_rec_mutex_lock (&self->mutex);

  g_weak_ref_set (&self->target_ref, NULL);

  for (i = 0; i < self->handlers->len; i++)
    {
      SignalHandler *handler = xptr_array_index (self->handlers, i);

      handler->handler_id = 0;
    }

  g_signal_emit (self, signals[UNBIND], 0);
  xobject_notify_by_pspec (G_OBJECT (self), properties[PROP_TARGET]);

  g_rec_mutex_unlock (&self->mutex);
}

static void
xsignal_group_bind_handler (xsignal_group_t  *self,
                             SignalHandler *handler,
                             xobject_t       *target)
{
  xssize_t i;

  g_assert (self != NULL);
  g_assert (X_IS_OBJECT (target));
  g_assert (handler != NULL);
  g_assert (handler->signal_id != 0);
  g_assert (handler->closure != NULL);
  g_assert (handler->closure->is_invalid == 0);
  g_assert (handler->handler_id == 0);

  handler->handler_id = g_signal_connect_closure_by_id (target,
                                                        handler->signal_id,
                                                        handler->signal_detail,
                                                        handler->closure,
                                                        handler->connect_after);

  g_assert (handler->handler_id != 0);

  for (i = 0; i < self->block_count; i++)
    g_signal_handler_block (target, handler->handler_id);
}

static void
xsignal_group_bind (xsignal_group_t *self,
                     xobject_t      *target)
{
  xobject_t *hold;
  xuint_t i;

  g_assert (X_IS_SIGNAL_GROUP (self));
  g_assert (!target || X_IS_OBJECT (target));

  if (target == NULL)
    return;

  self->has_bound_at_least_once = TRUE;

  hold = xobject_ref (target);

  g_weak_ref_set (&self->target_ref, hold);
  xobject_weak_ref (hold, xsignal_group__target_weak_notify, self);

  xsignal_group_gc_handlers (self);

  for (i = 0; i < self->handlers->len; i++)
    {
      SignalHandler *handler = xptr_array_index (self->handlers, i);

      xsignal_group_bind_handler (self, handler, hold);
    }

  g_signal_emit (self, signals [BIND], 0, hold);

  xobject_unref (hold);
}

static void
xsignal_group_unbind (xsignal_group_t *self)
{
  xobject_t *target;
  xuint_t i;

  g_return_if_fail (X_IS_SIGNAL_GROUP (self));

  target = g_weak_ref_get (&self->target_ref);

  /*
   * Target may be NULL by this point, as we got notified of its destruction.
   * However, if we're early enough, we may get a full reference back and can
   * cleanly disconnect our connections.
   */

  if (target != NULL)
    {
      g_weak_ref_set (&self->target_ref, NULL);

      /*
       * Let go of our weak reference now that we have a full reference
       * for the life of this function.
       */
      xobject_weak_unref (target,
                           xsignal_group__target_weak_notify,
                           self);
    }

  xsignal_group_gc_handlers (self);

  for (i = 0; i < self->handlers->len; i++)
    {
      SignalHandler *handler;
      gulong handler_id;

      handler = xptr_array_index (self->handlers, i);

      g_assert (handler != NULL);
      g_assert (handler->signal_id != 0);
      g_assert (handler->closure != NULL);

      handler_id = handler->handler_id;
      handler->handler_id = 0;

      /*
       * If @target is NULL, we lost a race to cleanup the weak
       * instance and the signal connections have already been
       * finalized and therefore nothing to do.
       */

      if (target != NULL && handler_id != 0)
        g_signal_handler_disconnect (target, handler_id);
    }

  g_signal_emit (self, signals [UNBIND], 0);

  g_clear_object (&target);
}

static xboolean_t
xsignal_group_check_target_type (xsignal_group_t *self,
                                  xpointer_t      target)
{
  if ((target != NULL) &&
      !xtype_is_a (G_OBJECT_TYPE (target), self->target_type))
    {
      g_critical ("Failed to set xsignal_group_t of target type %s "
                  "using target %p of type %s",
                  xtype_name (self->target_type),
                  target, G_OBJECT_TYPE_NAME (target));
      return FALSE;
    }

  return TRUE;
}

/**
 * xsignal_group_block:
 * @self: the #xsignal_group_t
 *
 * Blocks all signal handlers managed by @self so they will not
 * be called during any signal emissions. Must be unblocked exactly
 * the same number of times it has been blocked to become active again.
 *
 * This blocked state will be kept across changes of the target instance.
 *
 * Since: 2.72
 */
void
xsignal_group_block (xsignal_group_t *self)
{
  xobject_t *target;
  xuint_t i;

  g_return_if_fail (X_IS_SIGNAL_GROUP (self));
  g_return_if_fail (self->block_count >= 0);

  g_rec_mutex_lock (&self->mutex);

  self->block_count++;

  target = g_weak_ref_get (&self->target_ref);

  if (target == NULL)
    goto unlock;

  for (i = 0; i < self->handlers->len; i++)
    {
      const SignalHandler *handler = xptr_array_index (self->handlers, i);

      g_assert (handler != NULL);
      g_assert (handler->signal_id != 0);
      g_assert (handler->closure != NULL);
      g_assert (handler->handler_id != 0);

      g_signal_handler_block (target, handler->handler_id);
    }

  xobject_unref (target);

unlock:
  g_rec_mutex_unlock (&self->mutex);
}

/**
 * xsignal_group_unblock:
 * @self: the #xsignal_group_t
 *
 * Unblocks all signal handlers managed by @self so they will be
 * called again during any signal emissions unless it is blocked
 * again. Must be unblocked exactly the same number of times it
 * has been blocked to become active again.
 *
 * Since: 2.72
 */
void
xsignal_group_unblock (xsignal_group_t *self)
{
  xobject_t *target;
  xuint_t i;

  g_return_if_fail (X_IS_SIGNAL_GROUP (self));
  g_return_if_fail (self->block_count > 0);

  g_rec_mutex_lock (&self->mutex);

  self->block_count--;

  target = g_weak_ref_get (&self->target_ref);
  if (target == NULL)
    goto unlock;

  for (i = 0; i < self->handlers->len; i++)
    {
      const SignalHandler *handler = xptr_array_index (self->handlers, i);

      g_assert (handler != NULL);
      g_assert (handler->signal_id != 0);
      g_assert (handler->closure != NULL);
      g_assert (handler->handler_id != 0);

      g_signal_handler_unblock (target, handler->handler_id);
    }

  xobject_unref (target);

unlock:
  g_rec_mutex_unlock (&self->mutex);
}

/**
 * xsignal_group_dup_target:
 * @self: the #xsignal_group_t
 *
 * Gets the target instance used when connecting signals.
 *
 * Returns: (nullable) (transfer full) (type xobject_t): The target instance
 *
 * Since: 2.72
 */
xpointer_t
xsignal_group_dup_target (xsignal_group_t *self)
{
  xobject_t *target;

  g_return_val_if_fail (X_IS_SIGNAL_GROUP (self), NULL);

  g_rec_mutex_lock (&self->mutex);
  target = g_weak_ref_get (&self->target_ref);
  g_rec_mutex_unlock (&self->mutex);

  return target;
}

/**
 * xsignal_group_set_target:
 * @self: the #xsignal_group_t.
 * @target: (nullable) (type xobject_t) (transfer none): The target instance used
 *     when connecting signals.
 *
 * Sets the target instance used when connecting signals. Any signal
 * that has been registered with xsignal_group_connect_object() or
 * similar functions will be connected to this object.
 *
 * If the target instance was previously set, signals will be
 * disconnected from that object prior to connecting to @target.
 *
 * Since: 2.72
 */
void
xsignal_group_set_target (xsignal_group_t *self,
                           xpointer_t      target)
{
  xobject_t *object;

  g_return_if_fail (X_IS_SIGNAL_GROUP (self));

  g_rec_mutex_lock (&self->mutex);

  object = g_weak_ref_get (&self->target_ref);

  if (object == (xobject_t *)target)
    goto cleanup;

  if (!xsignal_group_check_target_type (self, target))
    goto cleanup;

  /* Only emit unbind if we've ever called bind */
  if (self->has_bound_at_least_once)
    xsignal_group_unbind (self);

  xsignal_group_bind (self, target);

  xobject_notify_by_pspec (G_OBJECT (self), properties[PROP_TARGET]);

cleanup:
  g_clear_object (&object);
  g_rec_mutex_unlock (&self->mutex);
}

static void
signal_handler_free (xpointer_t data)
{
  SignalHandler *handler = data;

  if (handler->closure != NULL)
    xclosure_invalidate (handler->closure);

  handler->handler_id = 0;
  handler->signal_id = 0;
  handler->signal_detail = 0;
  g_clear_pointer (&handler->closure, xclosure_unref);
  g_slice_free (SignalHandler, handler);
}

static void
xsignal_group_constructed (xobject_t *object)
{
  xsignal_group_t *self = (xsignal_group_t *)object;
  xobject_t *target;

  g_rec_mutex_lock (&self->mutex);

  target = g_weak_ref_get (&self->target_ref);
  if (!xsignal_group_check_target_type (self, target))
    xsignal_group_set_target (self, NULL);

  G_OBJECT_CLASS (xsignal_group_parent_class)->constructed (object);

  g_clear_object (&target);

  g_rec_mutex_unlock (&self->mutex);
}

static void
xsignal_group_dispose (xobject_t *object)
{
  xsignal_group_t *self = (xsignal_group_t *)object;

  g_rec_mutex_lock (&self->mutex);

  xsignal_group_gc_handlers (self);

  if (self->has_bound_at_least_once)
    xsignal_group_unbind (self);

  g_clear_pointer (&self->handlers, xptr_array_unref);

  g_rec_mutex_unlock (&self->mutex);

  G_OBJECT_CLASS (xsignal_group_parent_class)->dispose (object);
}

static void
xsignal_group_finalize (xobject_t *object)
{
  xsignal_group_t *self = (xsignal_group_t *)object;

  g_weak_ref_clear (&self->target_ref);
  g_rec_mutex_clear (&self->mutex);

  G_OBJECT_CLASS (xsignal_group_parent_class)->finalize (object);
}

static void
xsignal_group_get_property (xobject_t    *object,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  xsignal_group_t *self = G_SIGNAL_GROUP (object);

  switch ((xsignal_group_property_t) prop_id)
    {
    case PROP_TARGET:
      xvalue_take_object (value, xsignal_group_dup_target (self));
      break;

    case PROP_TARGET_TYPE:
      xvalue_set_gtype (value, self->target_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsignal_group_set_property (xobject_t      *object,
                             xuint_t         prop_id,
                             const xvalue_t *value,
                             xparam_spec_t   *pspec)
{
  xsignal_group_t *self = G_SIGNAL_GROUP (object);

  switch ((xsignal_group_property_t) prop_id)
    {
    case PROP_TARGET:
      xsignal_group_set_target (self, xvalue_get_object (value));
      break;

    case PROP_TARGET_TYPE:
      xsignal_group_set_target_type (self, xvalue_get_gtype (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsignal_group_class_init (xsignal_group_class_t *klass)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = xsignal_group_constructed;
  object_class->dispose = xsignal_group_dispose;
  object_class->finalize = xsignal_group_finalize;
  object_class->get_property = xsignal_group_get_property;
  object_class->set_property = xsignal_group_set_property;

  /**
   * xsignal_group_t:target
   *
   * The target instance used when connecting signals.
   *
   * Since: 2.72
   */
  properties[PROP_TARGET] =
      g_param_spec_object ("target",
                           "Target",
                           "The target instance used when connecting signals.",
                           XTYPE_OBJECT,
                           (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  /**
   * xsignal_group_t:target-type
   *
   * The #xtype_t of the target property.
   *
   * Since: 2.72
   */
  properties[PROP_TARGET_TYPE] =
      g_param_spec_gtype ("target-type",
                          "Target Type",
                          "The xtype_t of the target property.",
                          XTYPE_OBJECT,
                          (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  xobject_class_install_properties (object_class, LAST_PROP, properties);

  /**
   * xsignal_group_t::bind:
   * @self: the #xsignal_group_t
   * @instance: a #xobject_t containing the new value for #xsignal_group_t:target
   *
   * This signal is emitted when #xsignal_group_t:target is set to a new value
   * other than %NULL. It is similar to #xobject_t::notify on `target` except it
   * will not emit when #xsignal_group_t:target is %NULL and also allows for
   * receiving the #xobject_t without a data-race.
   *
   * Since: 2.72
   */
  signals[BIND] =
      g_signal_new ("bind",
                    XTYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL, NULL,
                    XTYPE_NONE,
                    1,
                    XTYPE_OBJECT);

  /**
   * xsignal_group_t::unbind:
   * @self: a #xsignal_group_t
   *
   * This signal is emitted when the target instance of @self is set to a
   * new #xobject_t.
   *
   * This signal will only be emitted if the previous target of @self is
   * non-%NULL.
   *
   * Since: 2.72
   */
  signals[UNBIND] =
      g_signal_new ("unbind",
                    XTYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL, NULL,
                    XTYPE_NONE,
                    0);
}

static void
xsignal_group_init (xsignal_group_t *self)
{
  g_rec_mutex_init (&self->mutex);
  self->handlers = xptr_array_new_with_free_func (signal_handler_free);
  self->target_type = XTYPE_OBJECT;
}

/**
 * xsignal_group_new:
 * @target_type: the #xtype_t of the target instance.
 *
 * Creates a new #xsignal_group_t for target instances of @target_type.
 *
 * Returns: (transfer full): a new #xsignal_group_t
 *
 * Since: 2.72
 */
xsignal_group_t *
xsignal_group_new (xtype_t target_type)
{
  g_return_val_if_fail (xtype_is_a (target_type, XTYPE_OBJECT), NULL);

  return xobject_new (XTYPE_SIGNAL_GROUP,
                       "target-type", target_type,
                       NULL);
}

static void
xsignal_group_connect_full (xsignal_group_t   *self,
                             const xchar_t    *detailed_signal,
                             xcallback_t       c_handler,
                             xpointer_t        data,
                             xclosure_notify_t  notify,
                             GConnectFlags   flags,
                             xboolean_t        is_object)
{
  xobject_t *target;
  SignalHandler *handler;
  xclosure_t *closure;
  xuint_t signal_id;
  xquark signal_detail;

  g_return_if_fail (X_IS_SIGNAL_GROUP (self));
  g_return_if_fail (detailed_signal != NULL);
  g_return_if_fail (g_signal_parse_name (detailed_signal, self->target_type,
                                         &signal_id, &signal_detail, TRUE) != 0);
  g_return_if_fail (c_handler != NULL);
  g_return_if_fail (!is_object || X_IS_OBJECT (data));

  g_rec_mutex_lock (&self->mutex);

  if (self->has_bound_at_least_once)
    {
      g_critical ("Cannot add signals after setting target");
      g_rec_mutex_unlock (&self->mutex);
      return;
    }

  if ((flags & G_CONNECT_SWAPPED) != 0)
    closure = g_cclosure_new_swap (c_handler, data, notify);
  else
    closure = g_cclosure_new (c_handler, data, notify);

  handler = g_slice_new0 (SignalHandler);
  handler->group = self;
  handler->signal_id = signal_id;
  handler->signal_detail = signal_detail;
  handler->closure = xclosure_ref (closure);
  handler->connect_after = ((flags & G_CONNECT_AFTER) != 0);

  xclosure_sink (closure);

  if (is_object)
    {
      /* Set closure->is_invalid when data is disposed. We only track this to avoid
       * reconnecting in the future. However, we do a round of cleanup when ever we
       * connect a new object or the target changes to GC the old handlers.
       */
      xobject_watch_closure (data, closure);
    }

  xptr_array_add (self->handlers, handler);

  target = g_weak_ref_get (&self->target_ref);

  if (target != NULL)
    {
      xsignal_group_bind_handler (self, handler, target);
      xobject_unref (target);
    }

  /* Lazily remove any old handlers on connect */
  xsignal_group_gc_handlers (self);

  g_rec_mutex_unlock (&self->mutex);
}

/**
 * xsignal_group_connect_object: (skip)
 * @self: a #xsignal_group_t
 * @detailed_signal: a string of the form `signal-name` with optional `::signal-detail`
 * @c_handler: (scope notified): the #xcallback_t to connect
 * @object: (not nullable) (transfer none): the #xobject_t to pass as data to @c_handler calls
 * @flags: #GConnectFlags for the signal connection
 *
 * Connects @c_handler to the signal @detailed_signal on #xsignal_group_t:target.
 *
 * Ensures that the @object stays alive during the call to @c_handler
 * by temporarily adding a reference count. When the @object is destroyed
 * the signal handler will automatically be removed.
 *
 * You cannot connect a signal handler after #xsignal_group_t:target has been set.
 *
 * Since: 2.72
 */
void
xsignal_group_connect_object (xsignal_group_t  *self,
                               const xchar_t   *detailed_signal,
                               xcallback_t      c_handler,
                               xpointer_t       object,
                               GConnectFlags  flags)
{
  g_return_if_fail (X_IS_OBJECT (object));

  xsignal_group_connect_full (self, detailed_signal, c_handler, object, NULL,
                               flags, TRUE);
}

/**
 * xsignal_group_connect_data:
 * @self: a #xsignal_group_t
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: (scope notified) (closure data) (destroy notify): the #xcallback_t to connect
 * @data: the data to pass to @c_handler calls
 * @notify: function to be called when disposing of @self
 * @flags: the flags used to create the signal connection
 *
 * Connects @c_handler to the signal @detailed_signal
 * on the target instance of @self.
 *
 * You cannot connect a signal handler after #xsignal_group_t:target has been set.
 *
 * Since: 2.72
 */
void
xsignal_group_connect_data (xsignal_group_t   *self,
                             const xchar_t    *detailed_signal,
                             xcallback_t       c_handler,
                             xpointer_t        data,
                             xclosure_notify_t  notify,
                             GConnectFlags   flags)
{
  xsignal_group_connect_full (self, detailed_signal, c_handler, data, notify,
                               flags, FALSE);
}

/**
 * xsignal_group_connect: (skip)
 * @self: a #xsignal_group_t
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: (scope notified): the #xcallback_t to connect
 * @data: the data to pass to @c_handler calls
 *
 * Connects @c_handler to the signal @detailed_signal
 * on the target instance of @self.
 *
 * You cannot connect a signal handler after #xsignal_group_t:target has been set.
 *
 * Since: 2.72
 */
void
xsignal_group_connect (xsignal_group_t *self,
                        const xchar_t  *detailed_signal,
                        xcallback_t     c_handler,
                        xpointer_t      data)
{
  xsignal_group_connect_full (self, detailed_signal, c_handler, data, NULL,
                               0, FALSE);
}

/**
 * xsignal_group_connect_after: (skip)
 * @self: a #xsignal_group_t
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: (scope notified): the #xcallback_t to connect
 * @data: the data to pass to @c_handler calls
 *
 * Connects @c_handler to the signal @detailed_signal
 * on the target instance of @self.
 *
 * The @c_handler will be called after the default handler of the signal.
 *
 * You cannot connect a signal handler after #xsignal_group_t:target has been set.
 *
 * Since: 2.72
 */
void
xsignal_group_connect_after (xsignal_group_t *self,
                              const xchar_t  *detailed_signal,
                              xcallback_t     c_handler,
                              xpointer_t      data)
{
  xsignal_group_connect_full (self, detailed_signal, c_handler,
                               data, NULL, G_CONNECT_AFTER, FALSE);
}

/**
 * xsignal_group_connect_swapped:
 * @self: a #xsignal_group_t
 * @detailed_signal: a string of the form "signal-name::detail"
 * @c_handler: (scope async): the #xcallback_t to connect
 * @data: the data to pass to @c_handler calls
 *
 * Connects @c_handler to the signal @detailed_signal
 * on the target instance of @self.
 *
 * The instance on which the signal is emitted and @data
 * will be swapped when calling @c_handler.
 *
 * You cannot connect a signal handler after #xsignal_group_t:target has been set.
 *
 * Since: 2.72
 */
void
xsignal_group_connect_swapped (xsignal_group_t *self,
                                const xchar_t  *detailed_signal,
                                xcallback_t     c_handler,
                                xpointer_t      data)
{
  xsignal_group_connect_full (self, detailed_signal, c_handler, data, NULL,
                               G_CONNECT_SWAPPED, FALSE);
}
