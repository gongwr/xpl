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
#include "gactiongroup.h"
#include "gaction.h"
#include "glibintl.h"
#include "gmarshal-internal.h"

/**
 * SECTION:gactiongroup
 * @title: xaction_group_t
 * @short_description: A group of actions
 * @include: gio/gio.h
 * @see_also: #GAction
 *
 * #xaction_group_t represents a group of actions. Actions can be used to
 * expose functionality in a structured way, either from one part of a
 * program to another, or to the outside world. Action groups are often
 * used together with a #GMenuModel that provides additional
 * representation data for displaying the actions to the user, e.g. in
 * a menu.
 *
 * The main way to interact with the actions in a xaction_group_t is to
 * activate them with xaction_group_activate_action(). Activating an
 * action may require a #xvariant_t parameter. The required type of the
 * parameter can be inquired with xaction_group_get_action_parameter_type().
 * Actions may be disabled, see xaction_group_get_action_enabled().
 * Activating a disabled action has no effect.
 *
 * Actions may optionally have a state in the form of a #xvariant_t. The
 * current state of an action can be inquired with
 * xaction_group_get_action_state(). Activating a stateful action may
 * change its state, but it is also possible to set the state by calling
 * xaction_group_change_action_state().
 *
 * As typical example, consider a text editing application which has an
 * option to change the current font to 'bold'. A good way to represent
 * this would be a stateful action, with a boolean state. Activating the
 * action would toggle the state.
 *
 * Each action in the group has a unique name (which is a string).  All
 * method calls, except xaction_group_list_actions() take the name of
 * an action as an argument.
 *
 * The #xaction_group_t API is meant to be the 'public' API to the action
 * group.  The calls here are exactly the interaction that 'external
 * forces' (eg: UI, incoming D-Bus messages, etc.) are supposed to have
 * with actions.  'Internal' APIs (ie: ones meant only to be accessed by
 * the action group implementation) are found on subclasses.  This is
 * why you will find - for example - xaction_group_get_action_enabled()
 * but not an equivalent set() call.
 *
 * Signals are emitted on the action group in response to state changes
 * on individual actions.
 *
 * Implementations of #xaction_group_t should provide implementations for
 * the virtual functions xaction_group_list_actions() and
 * xaction_group_query_action().  The other virtual functions should
 * not be implemented - their "wrappers" are actually implemented with
 * calls to xaction_group_query_action().
 */

/**
 * xaction_group_t:
 *
 * #xaction_group_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

/**
 * xaction_group_interface_t:
 * @has_action: the virtual function pointer for xaction_group_has_action()
 * @list_actions: the virtual function pointer for xaction_group_list_actions()
 * @get_action_parameter_type: the virtual function pointer for xaction_group_get_action_parameter_type()
 * @get_action_state_type: the virtual function pointer for xaction_group_get_action_state_type()
 * @get_action_state_hint: the virtual function pointer for xaction_group_get_action_state_hint()
 * @get_action_enabled: the virtual function pointer for xaction_group_get_action_enabled()
 * @get_action_state: the virtual function pointer for xaction_group_get_action_state()
 * @change_action_state: the virtual function pointer for xaction_group_change_action_state()
 * @query_action: the virtual function pointer for xaction_group_query_action()
 * @activate_action: the virtual function pointer for xaction_group_activate_action()
 * @change_action_state: the virtual function pointer for xaction_group_change_action_state()
 * @action_added: the class closure for the #xaction_group_t::action-added signal
 * @action_removed: the class closure for the #xaction_group_t::action-removed signal
 * @action_enabled_changed: the class closure for the #xaction_group_t::action-enabled-changed signal
 * @action_state_changed: the class closure for the #xaction_group_t::action-enabled-changed signal
 *
 * The virtual function table for #xaction_group_t.
 *
 * Since: 2.28
 **/

G_DEFINE_INTERFACE (xaction_group_t, xaction_group, XTYPE_OBJECT)

enum
{
  SIGNAL_ACTION_ADDED,
  SIGNAL_ACTION_REMOVED,
  SIGNAL_ACTION_ENABLED_CHANGED,
  SIGNAL_ACTION_STATE_CHANGED,
  NR_SIGNALS
};

static xuint_t xaction_group_signals[NR_SIGNALS];

static xboolean_t
xaction_group_real_has_action (xaction_group_t *action_group,
                                const xchar_t  *action_name)
{
  return xaction_group_query_action (action_group, action_name, NULL, NULL, NULL, NULL, NULL);
}

static xboolean_t
xaction_group_real_get_action_enabled (xaction_group_t *action_group,
                                        const xchar_t  *action_name)
{
  xboolean_t enabled = FALSE;

  xaction_group_query_action (action_group, action_name, &enabled, NULL, NULL, NULL, NULL);

  return enabled;
}

static const xvariant_type_t *
xaction_group_real_get_action_parameter_type (xaction_group_t *action_group,
                                               const xchar_t  *action_name)
{
  const xvariant_type_t *type = NULL;

  xaction_group_query_action (action_group, action_name, NULL, &type, NULL, NULL, NULL);

  return type;
}

static const xvariant_type_t *
xaction_group_real_get_action_state_type (xaction_group_t *action_group,
                                           const xchar_t  *action_name)
{
  const xvariant_type_t *type = NULL;

  xaction_group_query_action (action_group, action_name, NULL, NULL, &type, NULL, NULL);

  return type;
}

static xvariant_t *
xaction_group_real_get_action_state_hint (xaction_group_t *action_group,
                                           const xchar_t  *action_name)
{
  xvariant_t *hint = NULL;

  xaction_group_query_action (action_group, action_name, NULL, NULL, NULL, &hint, NULL);

  return hint;
}

static xvariant_t *
xaction_group_real_get_action_state (xaction_group_t *action_group,
                                      const xchar_t  *action_name)
{
  xvariant_t *state = NULL;

  xaction_group_query_action (action_group, action_name, NULL, NULL, NULL, NULL, &state);

  return state;
}

static xboolean_t
xaction_group_real_query_action (xaction_group_t        *action_group,
                                  const xchar_t         *action_name,
                                  xboolean_t            *enabled,
                                  const xvariant_type_t **parameter_type,
                                  const xvariant_type_t **state_type,
                                  xvariant_t           **state_hint,
                                  xvariant_t           **state)
{
  xaction_group_interface_t *iface = XACTION_GROUP_GET_IFACE (action_group);

  /* we expect implementations to override this method, but we also
   * allow for implementations that existed before this method was
   * introduced to override the individual accessors instead.
   *
   * detect the case that neither has happened and report it.
   */
  if G_UNLIKELY (iface->has_action == xaction_group_real_has_action ||
                 iface->get_action_enabled == xaction_group_real_get_action_enabled ||
                 iface->get_action_parameter_type == xaction_group_real_get_action_parameter_type ||
                 iface->get_action_state_type == xaction_group_real_get_action_state_type ||
                 iface->get_action_state_hint == xaction_group_real_get_action_state_hint ||
                 iface->get_action_state == xaction_group_real_get_action_state)
    {
      g_critical ("Class '%s' implements xaction_group_t interface without overriding "
                  "query_action() method -- bailing out to avoid infinite recursion.",
                  G_OBJECT_TYPE_NAME (action_group));
      return FALSE;
    }

  if (!(* iface->has_action) (action_group, action_name))
    return FALSE;

  if (enabled != NULL)
    *enabled = (* iface->get_action_enabled) (action_group, action_name);

  if (parameter_type != NULL)
    *parameter_type = (* iface->get_action_parameter_type) (action_group, action_name);

  if (state_type != NULL)
    *state_type = (* iface->get_action_state_type) (action_group, action_name);

  if (state_hint != NULL)
    *state_hint = (* iface->get_action_state_hint) (action_group, action_name);

  if (state != NULL)
    *state = (* iface->get_action_state) (action_group, action_name);

  return TRUE;
}

static void
xaction_group_default_init (xaction_group_interface_t *iface)
{
  iface->has_action = xaction_group_real_has_action;
  iface->get_action_enabled = xaction_group_real_get_action_enabled;
  iface->get_action_parameter_type = xaction_group_real_get_action_parameter_type;
  iface->get_action_state_type = xaction_group_real_get_action_state_type;
  iface->get_action_state_hint = xaction_group_real_get_action_state_hint;
  iface->get_action_state = xaction_group_real_get_action_state;
  iface->query_action = xaction_group_real_query_action;

  /**
   * xaction_group_t::action-added:
   * @action_group: the #xaction_group_t that changed
   * @action_name: the name of the action in @action_group
   *
   * Signals that a new action was just added to the group.
   * This signal is emitted after the action has been added
   * and is now visible.
   *
   * Since: 2.28
   **/
  xaction_group_signals[SIGNAL_ACTION_ADDED] =
    g_signal_new (I_("action-added"),
                  XTYPE_ACTION_GROUP,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (xaction_group_interface_t, action_added),
                  NULL, NULL,
                  NULL,
                  XTYPE_NONE, 1,
                  XTYPE_STRING);

  /**
   * xaction_group_t::action-removed:
   * @action_group: the #xaction_group_t that changed
   * @action_name: the name of the action in @action_group
   *
   * Signals that an action is just about to be removed from the group.
   * This signal is emitted before the action is removed, so the action
   * is still visible and can be queried from the signal handler.
   *
   * Since: 2.28
   **/
  xaction_group_signals[SIGNAL_ACTION_REMOVED] =
    g_signal_new (I_("action-removed"),
                  XTYPE_ACTION_GROUP,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (xaction_group_interface_t, action_removed),
                  NULL, NULL,
                  NULL,
                  XTYPE_NONE, 1,
                  XTYPE_STRING);


  /**
   * xaction_group_t::action-enabled-changed:
   * @action_group: the #xaction_group_t that changed
   * @action_name: the name of the action in @action_group
   * @enabled: whether the action is enabled or not
   *
   * Signals that the enabled status of the named action has changed.
   *
   * Since: 2.28
   **/
  xaction_group_signals[SIGNAL_ACTION_ENABLED_CHANGED] =
    g_signal_new (I_("action-enabled-changed"),
                  XTYPE_ACTION_GROUP,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (xaction_group_interface_t,
                                   action_enabled_changed),
                  NULL, NULL,
                  _g_cclosure_marshal_VOID__STRING_BOOLEAN,
                  XTYPE_NONE, 2,
                  XTYPE_STRING,
                  XTYPE_BOOLEAN);
  g_signal_set_va_marshaller (xaction_group_signals[SIGNAL_ACTION_ENABLED_CHANGED],
                              XTYPE_FROM_INTERFACE (iface),
                              _g_cclosure_marshal_VOID__STRING_BOOLEANv);

  /**
   * xaction_group_t::action-state-changed:
   * @action_group: the #xaction_group_t that changed
   * @action_name: the name of the action in @action_group
   * @value: the new value of the state
   *
   * Signals that the state of the named action has changed.
   *
   * Since: 2.28
   **/
  xaction_group_signals[SIGNAL_ACTION_STATE_CHANGED] =
    g_signal_new (I_("action-state-changed"),
                  XTYPE_ACTION_GROUP,
                  G_SIGNAL_RUN_LAST |
                  G_SIGNAL_DETAILED |
                  G_SIGNAL_MUST_COLLECT,
                  G_STRUCT_OFFSET (xaction_group_interface_t,
                                   action_state_changed),
                  NULL, NULL,
                  _g_cclosure_marshal_VOID__STRING_VARIANT,
                  XTYPE_NONE, 2,
                  XTYPE_STRING,
                  XTYPE_VARIANT);
  g_signal_set_va_marshaller (xaction_group_signals[SIGNAL_ACTION_STATE_CHANGED],
                              XTYPE_FROM_INTERFACE (iface),
                              _g_cclosure_marshal_VOID__STRING_VARIANTv);
}

/**
 * xaction_group_list_actions:
 * @action_group: a #xaction_group_t
 *
 * Lists the actions contained within @action_group.
 *
 * The caller is responsible for freeing the list with g_strfreev() when
 * it is no longer required.
 *
 * Returns: (transfer full): a %NULL-terminated array of the names of the
 * actions in the group
 *
 * Since: 2.28
 **/
xchar_t **
xaction_group_list_actions (xaction_group_t *action_group)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), NULL);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->list_actions (action_group);
}

/**
 * xaction_group_has_action:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to check for
 *
 * Checks if the named action exists within @action_group.
 *
 * Returns: whether the named action exists
 *
 * Since: 2.28
 **/
xboolean_t
xaction_group_has_action (xaction_group_t *action_group,
                           const xchar_t  *action_name)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), FALSE);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->has_action (action_group, action_name);
}

/**
 * xaction_group_get_action_parameter_type:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to query
 *
 * Queries the type of the parameter that must be given when activating
 * the named action within @action_group.
 *
 * When activating the action using xaction_group_activate_action(),
 * the #xvariant_t given to that function must be of the type returned
 * by this function.
 *
 * In the case that this function returns %NULL, you must not give any
 * #xvariant_t, but %NULL instead.
 *
 * The parameter type of a particular action will never change but it is
 * possible for an action to be removed and for a new action to be added
 * with the same name but a different parameter type.
 *
 * Returns: (nullable): the parameter type
 *
 * Since: 2.28
 **/
const xvariant_type_t *
xaction_group_get_action_parameter_type (xaction_group_t *action_group,
                                          const xchar_t  *action_name)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), NULL);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->get_action_parameter_type (action_group, action_name);
}

/**
 * xaction_group_get_action_state_type:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to query
 *
 * Queries the type of the state of the named action within
 * @action_group.
 *
 * If the action is stateful then this function returns the
 * #xvariant_type_t of the state.  All calls to
 * xaction_group_change_action_state() must give a #xvariant_t of this
 * type and xaction_group_get_action_state() will return a #xvariant_t
 * of the same type.
 *
 * If the action is not stateful then this function will return %NULL.
 * In that case, xaction_group_get_action_state() will return %NULL
 * and you must not call xaction_group_change_action_state().
 *
 * The state type of a particular action will never change but it is
 * possible for an action to be removed and for a new action to be added
 * with the same name but a different state type.
 *
 * Returns: (nullable): the state type, if the action is stateful
 *
 * Since: 2.28
 **/
const xvariant_type_t *
xaction_group_get_action_state_type (xaction_group_t *action_group,
                                      const xchar_t  *action_name)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), NULL);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->get_action_state_type (action_group, action_name);
}

/**
 * xaction_group_get_action_state_hint:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to query
 *
 * Requests a hint about the valid range of values for the state of the
 * named action within @action_group.
 *
 * If %NULL is returned it either means that the action is not stateful
 * or that there is no hint about the valid range of values for the
 * state of the action.
 *
 * If a #xvariant_t array is returned then each item in the array is a
 * possible value for the state.  If a #xvariant_t pair (ie: two-tuple) is
 * returned then the tuple specifies the inclusive lower and upper bound
 * of valid values for the state.
 *
 * In any case, the information is merely a hint.  It may be possible to
 * have a state value outside of the hinted range and setting a value
 * within the range may fail.
 *
 * The return value (if non-%NULL) should be freed with
 * g_variant_unref() when it is no longer required.
 *
 * Returns: (nullable) (transfer full): the state range hint
 *
 * Since: 2.28
 **/
xvariant_t *
xaction_group_get_action_state_hint (xaction_group_t *action_group,
                                      const xchar_t  *action_name)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), NULL);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->get_action_state_hint (action_group, action_name);
}

/**
 * xaction_group_get_action_enabled:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to query
 *
 * Checks if the named action within @action_group is currently enabled.
 *
 * An action must be enabled in order to be activated or in order to
 * have its state changed from outside callers.
 *
 * Returns: whether or not the action is currently enabled
 *
 * Since: 2.28
 **/
xboolean_t
xaction_group_get_action_enabled (xaction_group_t *action_group,
                                   const xchar_t  *action_name)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), FALSE);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->get_action_enabled (action_group, action_name);
}

/**
 * xaction_group_get_action_state:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to query
 *
 * Queries the current state of the named action within @action_group.
 *
 * If the action is not stateful then %NULL will be returned.  If the
 * action is stateful then the type of the return value is the type
 * given by xaction_group_get_action_state_type().
 *
 * The return value (if non-%NULL) should be freed with
 * g_variant_unref() when it is no longer required.
 *
 * Returns: (nullable) (transfer full): the current state of the action
 *
 * Since: 2.28
 **/
xvariant_t *
xaction_group_get_action_state (xaction_group_t *action_group,
                                 const xchar_t  *action_name)
{
  g_return_val_if_fail (X_IS_ACTION_GROUP (action_group), NULL);

  return XACTION_GROUP_GET_IFACE (action_group)
    ->get_action_state (action_group, action_name);
}

/**
 * xaction_group_change_action_state:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to request the change on
 * @value: the new state
 *
 * Request for the state of the named action within @action_group to be
 * changed to @value.
 *
 * The action must be stateful and @value must be of the correct type.
 * See xaction_group_get_action_state_type().
 *
 * This call merely requests a change.  The action may refuse to change
 * its state or may change its state to something other than @value.
 * See xaction_group_get_action_state_hint().
 *
 * If the @value xvariant_t is floating, it is consumed.
 *
 * Since: 2.28
 **/
void
xaction_group_change_action_state (xaction_group_t *action_group,
                                    const xchar_t  *action_name,
                                    xvariant_t     *value)
{
  g_return_if_fail (X_IS_ACTION_GROUP (action_group));
  g_return_if_fail (action_name != NULL);
  g_return_if_fail (value != NULL);

  XACTION_GROUP_GET_IFACE (action_group)
    ->change_action_state (action_group, action_name, value);
}

/**
 * xaction_group_activate_action:
 * @action_group: a #xaction_group_t
 * @action_name: the name of the action to activate
 * @parameter: (nullable): parameters to the activation
 *
 * Activate the named action within @action_group.
 *
 * If the action is expecting a parameter, then the correct type of
 * parameter must be given as @parameter.  If the action is expecting no
 * parameters then @parameter must be %NULL.  See
 * xaction_group_get_action_parameter_type().
 *
 * If the #xaction_group_t implementation supports asynchronous remote
 * activation over D-Bus, this call may return before the relevant
 * D-Bus traffic has been sent, or any replies have been received. In
 * order to block on such asynchronous activation calls,
 * g_dbus_connection_flush() should be called prior to the code, which
 * depends on the result of the action activation. Without flushing
 * the D-Bus connection, there is no guarantee that the action would
 * have been activated.
 *
 * The following code which runs in a remote app instance, shows an
 * example of a "quit" action being activated on the primary app
 * instance over D-Bus. Here g_dbus_connection_flush() is called
 * before `exit()`. Without g_dbus_connection_flush(), the "quit" action
 * may fail to be activated on the primary instance.
 *
 * |[<!-- language="C" -->
 * // call "quit" action on primary instance
 * xaction_group_activate_action (XACTION_GROUP (app), "quit", NULL);
 *
 * // make sure the action is activated now
 * g_dbus_connection_flush (...);
 *
 * g_debug ("application has been terminated. exiting.");
 *
 * exit (0);
 * ]|
 *
 * Since: 2.28
 **/
void
xaction_group_activate_action (xaction_group_t *action_group,
                                const xchar_t  *action_name,
                                xvariant_t     *parameter)
{
  g_return_if_fail (X_IS_ACTION_GROUP (action_group));
  g_return_if_fail (action_name != NULL);

  XACTION_GROUP_GET_IFACE (action_group)
    ->activate_action (action_group, action_name, parameter);
}

/**
 * xaction_group_action_added:
 * @action_group: a #xaction_group_t
 * @action_name: the name of an action in the group
 *
 * Emits the #xaction_group_t::action-added signal on @action_group.
 *
 * This function should only be called by #xaction_group_t implementations.
 *
 * Since: 2.28
 **/
void
xaction_group_action_added (xaction_group_t *action_group,
                             const xchar_t  *action_name)
{
  g_return_if_fail (X_IS_ACTION_GROUP (action_group));
  g_return_if_fail (action_name != NULL);

  g_signal_emit (action_group,
                 xaction_group_signals[SIGNAL_ACTION_ADDED],
                 g_quark_try_string (action_name),
                 action_name);
}

/**
 * xaction_group_action_removed:
 * @action_group: a #xaction_group_t
 * @action_name: the name of an action in the group
 *
 * Emits the #xaction_group_t::action-removed signal on @action_group.
 *
 * This function should only be called by #xaction_group_t implementations.
 *
 * Since: 2.28
 **/
void
xaction_group_action_removed (xaction_group_t *action_group,
                               const xchar_t  *action_name)
{
  g_return_if_fail (X_IS_ACTION_GROUP (action_group));
  g_return_if_fail (action_name != NULL);

  g_signal_emit (action_group,
                 xaction_group_signals[SIGNAL_ACTION_REMOVED],
                 g_quark_try_string (action_name),
                 action_name);
}

/**
 * xaction_group_action_enabled_changed:
 * @action_group: a #xaction_group_t
 * @action_name: the name of an action in the group
 * @enabled: whether or not the action is now enabled
 *
 * Emits the #xaction_group_t::action-enabled-changed signal on @action_group.
 *
 * This function should only be called by #xaction_group_t implementations.
 *
 * Since: 2.28
 **/
void
xaction_group_action_enabled_changed (xaction_group_t *action_group,
                                       const xchar_t  *action_name,
                                       xboolean_t      enabled)
{
  g_return_if_fail (X_IS_ACTION_GROUP (action_group));
  g_return_if_fail (action_name != NULL);

  enabled = !!enabled;

  g_signal_emit (action_group,
                 xaction_group_signals[SIGNAL_ACTION_ENABLED_CHANGED],
                 g_quark_try_string (action_name),
                 action_name,
                 enabled);
}

/**
 * xaction_group_action_state_changed:
 * @action_group: a #xaction_group_t
 * @action_name: the name of an action in the group
 * @state: the new state of the named action
 *
 * Emits the #xaction_group_t::action-state-changed signal on @action_group.
 *
 * This function should only be called by #xaction_group_t implementations.
 *
 * Since: 2.28
 **/
void
xaction_group_action_state_changed (xaction_group_t *action_group,
                                     const xchar_t  *action_name,
                                     xvariant_t     *state)
{
  g_return_if_fail (X_IS_ACTION_GROUP (action_group));
  g_return_if_fail (action_name != NULL);

  g_signal_emit (action_group,
                 xaction_group_signals[SIGNAL_ACTION_STATE_CHANGED],
                 g_quark_try_string (action_name),
                 action_name,
                 state);
}

/**
 * xaction_group_query_action:
 * @action_group: a #xaction_group_t
 * @action_name: the name of an action in the group
 * @enabled: (out): if the action is presently enabled
 * @parameter_type: (out) (optional): the parameter type, or %NULL if none needed
 * @state_type: (out) (optional): the state type, or %NULL if stateless
 * @state_hint: (out) (optional): the state hint, or %NULL if none
 * @state: (out) (optional): the current state, or %NULL if stateless
 *
 * Queries all aspects of the named action within an @action_group.
 *
 * This function acquires the information available from
 * xaction_group_has_action(), xaction_group_get_action_enabled(),
 * xaction_group_get_action_parameter_type(),
 * xaction_group_get_action_state_type(),
 * xaction_group_get_action_state_hint() and
 * xaction_group_get_action_state() with a single function call.
 *
 * This provides two main benefits.
 *
 * The first is the improvement in efficiency that comes with not having
 * to perform repeated lookups of the action in order to discover
 * different things about it.  The second is that implementing
 * #xaction_group_t can now be done by only overriding this one virtual
 * function.
 *
 * The interface provides a default implementation of this function that
 * calls the individual functions, as required, to fetch the
 * information.  The interface also provides default implementations of
 * those functions that call this function.  All implementations,
 * therefore, must override either this function or all of the others.
 *
 * If the action exists, %TRUE is returned and any of the requested
 * fields (as indicated by having a non-%NULL reference passed in) are
 * filled.  If the action doesn't exist, %FALSE is returned and the
 * fields may or may not have been modified.
 *
 * Returns: %TRUE if the action exists, else %FALSE
 *
 * Since: 2.32
 **/
xboolean_t
xaction_group_query_action (xaction_group_t        *action_group,
                             const xchar_t         *action_name,
                             xboolean_t            *enabled,
                             const xvariant_type_t **parameter_type,
                             const xvariant_type_t **state_type,
                             xvariant_t           **state_hint,
                             xvariant_t           **state)
{
  return XACTION_GROUP_GET_IFACE (action_group)
    ->query_action (action_group, action_name, enabled, parameter_type, state_type, state_hint, state);
}