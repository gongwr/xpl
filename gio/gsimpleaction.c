/*
 * Copyright © 2010 Codethink Limited
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gsimpleaction.h"

#include "gaction.h"
#include "glibintl.h"

/**
 * SECTION:gsimpleaction
 * @title: xsimple_action_t
 * @short_description: A simple xaction_t implementation
 * @include: gio/gio.h
 *
 * A #xsimple_action_t is the obvious simple implementation of the #xaction_t
 * interface. This is the easiest way to create an action for purposes of
 * adding it to a #xsimple_action_group_t.
 *
 * See also #GtkAction.
 */

/**
 * xsimple_action_t:
 *
 * #xsimple_action_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

struct _GSimpleAction
{
  xobject_t       parent_instance;

  xchar_t        *name;
  xvariant_type_t *parameter_type;
  xboolean_t      enabled;
  xvariant_t     *state;
  xvariant_t     *state_hint;
  xboolean_t      state_set_already;
};

typedef xobject_class_t GSimpleActionClass;

static void g_simple_action_iface_init (xaction_interface_t *iface);
G_DEFINE_TYPE_WITH_CODE (xsimple_action_t, g_simple_action, XTYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (XTYPE_ACTION, g_simple_action_iface_init))

enum
{
  PROP_NONE,
  PROP_NAME,
  PROP_PARAMETER_TYPE,
  PROP_ENABLED,
  PROP_STATE_TYPE,
  PROP_STATE
};

enum
{
  SIGNAL_CHANGE_STATE,
  SIGNAL_ACTIVATE,
  NR_SIGNALS
};

static xuint_t g_simple_action_signals[NR_SIGNALS];

static const xchar_t *
g_simple_action_get_name (xaction_t *action)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  return simple->name;
}

static const xvariant_type_t *
g_simple_action_get_parameter_type (xaction_t *action)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  return simple->parameter_type;
}

static const xvariant_type_t *
g_simple_action_get_state_type (xaction_t *action)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  if (simple->state != NULL)
    return xvariant_get_type (simple->state);
  else
    return NULL;
}

static xvariant_t *
g_simple_action_get_state_hint (xaction_t *action)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  if (simple->state_hint != NULL)
    return xvariant_ref (simple->state_hint);
  else
    return NULL;
}

static xboolean_t
g_simple_action_get_enabled (xaction_t *action)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  return simple->enabled;
}

static void
g_simple_action_change_state (xaction_t  *action,
                              xvariant_t *value)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  /* If the user connected a signal handler then they are responsible
   * for handling state changes.
   */
  if (g_signal_has_handler_pending (action, g_simple_action_signals[SIGNAL_CHANGE_STATE], 0, TRUE))
    g_signal_emit (action, g_simple_action_signals[SIGNAL_CHANGE_STATE], 0, value);

  /* If not, then the default behaviour is to just set the state. */
  else
    g_simple_action_set_state (simple, value);
}

/**
 * g_simple_action_set_state:
 * @simple: a #xsimple_action_t
 * @value: the new #xvariant_t for the state
 *
 * Sets the state of the action.
 *
 * This directly updates the 'state' property to the given value.
 *
 * This should only be called by the implementor of the action.  Users
 * of the action should not attempt to directly modify the 'state'
 * property.  Instead, they should call g_action_change_state() to
 * request the change.
 *
 * If the @value xvariant_t is floating, it is consumed.
 *
 * Since: 2.30
 **/
void
g_simple_action_set_state (xsimple_action_t *simple,
                           xvariant_t      *value)
{
  g_return_if_fail (X_IS_SIMPLE_ACTION (simple));
  g_return_if_fail (value != NULL);

  {
    const xvariant_type_t *state_type;

    state_type = simple->state ?
                   xvariant_get_type (simple->state) : NULL;
    g_return_if_fail (state_type != NULL);
    g_return_if_fail (xvariant_is_of_type (value, state_type));
  }

  xvariant_ref_sink (value);

  if (!simple->state || !xvariant_equal (simple->state, value))
    {
      if (simple->state)
        xvariant_unref (simple->state);

      simple->state = xvariant_ref (value);

      xobject_notify (G_OBJECT (simple), "state");
    }

  xvariant_unref (value);
}

static xvariant_t *
g_simple_action_get_state (xaction_t *action)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  return simple->state ? xvariant_ref (simple->state) : NULL;
}

static void
g_simple_action_activate (xaction_t  *action,
                          xvariant_t *parameter)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (action);

  g_return_if_fail (simple->parameter_type == NULL ?
                      parameter == NULL :
                    (parameter != NULL &&
                     xvariant_is_of_type (parameter,
                                           simple->parameter_type)));

  if (parameter != NULL)
    xvariant_ref_sink (parameter);

  if (simple->enabled)
    {
      /* If the user connected a signal handler then they are responsible
       * for handling activation.
       */
      if (g_signal_has_handler_pending (action, g_simple_action_signals[SIGNAL_ACTIVATE], 0, TRUE))
        g_signal_emit (action, g_simple_action_signals[SIGNAL_ACTIVATE], 0, parameter);

      /* If not, do some reasonable defaults for stateful actions. */
      else if (simple->state)
        {
          /* If we have no parameter and this is a boolean action, toggle. */
          if (parameter == NULL && xvariant_is_of_type (simple->state, G_VARIANT_TYPE_BOOLEAN))
            {
              xboolean_t was_enabled = xvariant_get_boolean (simple->state);
              g_simple_action_change_state (action, xvariant_new_boolean (!was_enabled));
            }

          /* else, if the parameter and state type are the same, do a change-state */
          else if (xvariant_is_of_type (simple->state, xvariant_get_type (parameter)))
            g_simple_action_change_state (action, parameter);
        }
    }

  if (parameter != NULL)
    xvariant_unref (parameter);
}

static void
g_simple_action_set_property (xobject_t    *object,
                              xuint_t       prop_id,
                              const xvalue_t     *value,
                              xparam_spec_t *pspec)
{
  xsimple_action_t *action = G_SIMPLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      action->name = xstrdup (xvalue_get_string (value));
      break;

    case PROP_PARAMETER_TYPE:
      action->parameter_type = xvalue_dup_boxed (value);
      break;

    case PROP_ENABLED:
      action->enabled = xvalue_get_boolean (value);
      break;

    case PROP_STATE:
      /* The first time we see this (during construct) we should just
       * take the state as it was handed to us.
       *
       * After that, we should make sure we go through the same checks
       * as the C API.
       */
      if (!action->state_set_already)
        {
          action->state = xvalue_dup_variant (value);
          action->state_set_already = TRUE;
        }
      else
        g_simple_action_set_state (action, xvalue_get_variant (value));

      break;

    default:
      g_assert_not_reached ();
    }
}

static void
g_simple_action_get_property (xobject_t    *object,
                              xuint_t       prop_id,
                              xvalue_t     *value,
                              xparam_spec_t *pspec)
{
  xaction_t *action = G_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      xvalue_set_string (value, g_simple_action_get_name (action));
      break;

    case PROP_PARAMETER_TYPE:
      xvalue_set_boxed (value, g_simple_action_get_parameter_type (action));
      break;

    case PROP_ENABLED:
      xvalue_set_boolean (value, g_simple_action_get_enabled (action));
      break;

    case PROP_STATE_TYPE:
      xvalue_set_boxed (value, g_simple_action_get_state_type (action));
      break;

    case PROP_STATE:
      xvalue_take_variant (value, g_simple_action_get_state (action));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
g_simple_action_finalize (xobject_t *object)
{
  xsimple_action_t *simple = G_SIMPLE_ACTION (object);

  g_free (simple->name);
  if (simple->parameter_type)
    xvariant_type_free (simple->parameter_type);
  if (simple->state)
    xvariant_unref (simple->state);
  if (simple->state_hint)
    xvariant_unref (simple->state_hint);

  G_OBJECT_CLASS (g_simple_action_parent_class)
    ->finalize (object);
}

void
g_simple_action_init (xsimple_action_t *simple)
{
  simple->enabled = TRUE;
}

void
g_simple_action_iface_init (xaction_interface_t *iface)
{
  iface->get_name = g_simple_action_get_name;
  iface->get_parameter_type = g_simple_action_get_parameter_type;
  iface->get_state_type = g_simple_action_get_state_type;
  iface->get_state_hint = g_simple_action_get_state_hint;
  iface->get_enabled = g_simple_action_get_enabled;
  iface->get_state = g_simple_action_get_state;
  iface->change_state = g_simple_action_change_state;
  iface->activate = g_simple_action_activate;
}

void
g_simple_action_class_init (GSimpleActionClass *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = g_simple_action_set_property;
  object_class->get_property = g_simple_action_get_property;
  object_class->finalize = g_simple_action_finalize;

  /**
   * xsimple_action_t::activate:
   * @simple: the #xsimple_action_t
   * @parameter: (nullable): the parameter to the activation, or %NULL if it has
   *   no parameter
   *
   * Indicates that the action was just activated.
   *
   * @parameter will always be of the expected type, i.e. the parameter type
   * specified when the action was created. If an incorrect type is given when
   * activating the action, this signal is not emitted.
   *
   * Since GLib 2.40, if no handler is connected to this signal then the
   * default behaviour for boolean-stated actions with a %NULL parameter
   * type is to toggle them via the #xsimple_action_t::change-state signal.
   * For stateful actions where the state type is equal to the parameter
   * type, the default is to forward them directly to
   * #xsimple_action_t::change-state.  This should allow almost all users
   * of #xsimple_action_t to connect only one handler or the other.
   *
   * Since: 2.28
   */
  g_simple_action_signals[SIGNAL_ACTIVATE] =
    g_signal_new (I_("activate"),
                  XTYPE_SIMPLE_ACTION,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                  0, NULL, NULL,
                  NULL,
                  XTYPE_NONE, 1,
                  XTYPE_VARIANT);

  /**
   * xsimple_action_t::change-state:
   * @simple: the #xsimple_action_t
   * @value: (nullable): the requested value for the state
   *
   * Indicates that the action just received a request to change its
   * state.
   *
   * @value will always be of the correct state type, i.e. the type of the
   * initial state passed to g_simple_action_new_stateful(). If an incorrect
   * type is given when requesting to change the state, this signal is not
   * emitted.
   *
   * If no handler is connected to this signal then the default
   * behaviour is to call g_simple_action_set_state() to set the state
   * to the requested value. If you connect a signal handler then no
   * default action is taken. If the state should change then you must
   * call g_simple_action_set_state() from the handler.
   *
   * An example of a 'change-state' handler:
   * |[<!-- language="C" -->
   * static void
   * change_volume_state (xsimple_action_t *action,
   *                      xvariant_t      *value,
   *                      xpointer_t       user_data)
   * {
   *   xint_t requested;
   *
   *   requested = xvariant_get_int32 (value);
   *
   *   // Volume only goes from 0 to 10
   *   if (0 <= requested && requested <= 10)
   *     g_simple_action_set_state (action, value);
   * }
   * ]|
   *
   * The handler need not set the state to the requested value.
   * It could set it to any value at all, or take some other action.
   *
   * Since: 2.30
   */
  g_simple_action_signals[SIGNAL_CHANGE_STATE] =
    g_signal_new (I_("change-state"),
                  XTYPE_SIMPLE_ACTION,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                  0, NULL, NULL,
                  NULL,
                  XTYPE_NONE, 1,
                  XTYPE_VARIANT);

  /**
   * xsimple_action_t:name:
   *
   * The name of the action. This is mostly meaningful for identifying
   * the action once it has been added to a #xsimple_action_group_t.
   *
   * Since: 2.28
   **/
  xobject_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        P_("Action Name"),
                                                        P_("The name used to invoke the action"),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * xsimple_action_t:parameter-type:
   *
   * The type of the parameter that must be given when activating the
   * action.
   *
   * Since: 2.28
   **/
  xobject_class_install_property (object_class, PROP_PARAMETER_TYPE,
                                   g_param_spec_boxed ("parameter-type",
                                                       P_("Parameter Type"),
                                                       P_("The type of xvariant_t passed to activate()"),
                                                       XTYPE_VARIANT_TYPE,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * xsimple_action_t:enabled:
   *
   * If @action is currently enabled.
   *
   * If the action is disabled then calls to g_action_activate() and
   * g_action_change_state() have no effect.
   *
   * Since: 2.28
   **/
  xobject_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         P_("Enabled"),
                                                         P_("If the action can be activated"),
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xsimple_action_t:state-type:
   *
   * The #xvariant_type_t of the state that the action has, or %NULL if the
   * action is stateless.
   *
   * Since: 2.28
   **/
  xobject_class_install_property (object_class, PROP_STATE_TYPE,
                                   g_param_spec_boxed ("state-type",
                                                       P_("State Type"),
                                                       P_("The type of the state kept by the action"),
                                                       XTYPE_VARIANT_TYPE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * xsimple_action_t:state:
   *
   * The state of the action, or %NULL if the action is stateless.
   *
   * Since: 2.28
   **/
  xobject_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_variant ("state",
                                                         P_("State"),
                                                         P_("The state the action is in"),
                                                         G_VARIANT_TYPE_ANY,
                                                         NULL,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                                                         G_PARAM_STATIC_STRINGS));
}

/**
 * g_simple_action_set_enabled:
 * @simple: a #xsimple_action_t
 * @enabled: whether the action is enabled
 *
 * Sets the action as enabled or not.
 *
 * An action must be enabled in order to be activated or in order to
 * have its state changed from outside callers.
 *
 * This should only be called by the implementor of the action.  Users
 * of the action should not attempt to modify its enabled flag.
 *
 * Since: 2.28
 **/
void
g_simple_action_set_enabled (xsimple_action_t *simple,
                             xboolean_t       enabled)
{
  g_return_if_fail (X_IS_SIMPLE_ACTION (simple));

  enabled = !!enabled;

  if (simple->enabled != enabled)
    {
      simple->enabled = enabled;
      xobject_notify (G_OBJECT (simple), "enabled");
    }
}

/**
 * g_simple_action_set_state_hint:
 * @simple: a #xsimple_action_t
 * @state_hint: (nullable): a #xvariant_t representing the state hint
 *
 * Sets the state hint for the action.
 *
 * See g_action_get_state_hint() for more information about
 * action state hints.
 *
 * Since: 2.44
 **/
void
g_simple_action_set_state_hint (xsimple_action_t *simple,
                                xvariant_t      *state_hint)
{
  g_return_if_fail (X_IS_SIMPLE_ACTION (simple));

  if (simple->state_hint != NULL)
    {
      xvariant_unref (simple->state_hint);
      simple->state_hint = NULL;
    }

  if (state_hint != NULL)
    simple->state_hint = xvariant_ref (state_hint);
}

/**
 * g_simple_action_new:
 * @name: the name of the action
 * @parameter_type: (nullable): the type of parameter that will be passed to
 *   handlers for the #xsimple_action_t::activate signal, or %NULL for no parameter
 *
 * Creates a new action.
 *
 * The created action is stateless. See g_simple_action_new_stateful() to create
 * an action that has state.
 *
 * Returns: a new #xsimple_action_t
 *
 * Since: 2.28
 **/
xsimple_action_t *
g_simple_action_new (const xchar_t        *name,
                     const xvariant_type_t *parameter_type)
{
  g_return_val_if_fail (name != NULL, NULL);

  return xobject_new (XTYPE_SIMPLE_ACTION,
                       "name", name,
                       "parameter-type", parameter_type,
                       NULL);
}

/**
 * g_simple_action_new_stateful:
 * @name: the name of the action
 * @parameter_type: (nullable): the type of the parameter that will be passed to
 *   handlers for the #xsimple_action_t::activate signal, or %NULL for no parameter
 * @state: the initial state of the action
 *
 * Creates a new stateful action.
 *
 * All future state values must have the same #xvariant_type_t as the initial
 * @state.
 *
 * If the @state #xvariant_t is floating, it is consumed.
 *
 * Returns: a new #xsimple_action_t
 *
 * Since: 2.28
 **/
xsimple_action_t *
g_simple_action_new_stateful (const xchar_t        *name,
                              const xvariant_type_t *parameter_type,
                              xvariant_t           *state)
{
  return xobject_new (XTYPE_SIMPLE_ACTION,
                       "name", name,
                       "parameter-type", parameter_type,
                       "state", state,
                       NULL);
}
