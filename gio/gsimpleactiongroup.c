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

#include "gsimpleactiongroup.h"

#include "gsimpleaction.h"
#include "gactionmap.h"
#include "gaction.h"

/**
 * SECTION:gsimpleactiongroup
 * @title: xsimple_action_group_t
 * @short_description: A simple xaction_group_t implementation
 * @include: gio/gio.h
 *
 * #xsimple_action_group_t is a hash table filled with #xaction_t objects,
 * implementing the #xaction_group_t and #xaction_map_t interfaces.
 **/

struct _GSimpleActionGroupPrivate
{
  xhashtable_t *table;  /* string -> xaction_t */
};

static void g_simple_action_group_iface_init (xaction_group_interface_t *);
static void g_simple_action_group_map_iface_init (xaction_map_interface_t *);
G_DEFINE_TYPE_WITH_CODE (xsimple_action_group_t,
  g_simple_action_group, XTYPE_OBJECT,
  G_ADD_PRIVATE (xsimple_action_group_t)
  G_IMPLEMENT_INTERFACE (XTYPE_ACTION_GROUP,
                         g_simple_action_group_iface_init);
  G_IMPLEMENT_INTERFACE (XTYPE_ACTION_MAP,
                         g_simple_action_group_map_iface_init))

static xchar_t **
g_simple_action_group_list_actions (xaction_group_t *group)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (group);
  xhash_table_iter_t iter;
  xint_t n, i = 0;
  xchar_t **keys;
  xpointer_t key;

  n = xhash_table_size (simple->priv->table);
  keys = g_new (xchar_t *, n + 1);

  xhash_table_iter_init (&iter, simple->priv->table);
  while (xhash_table_iter_next (&iter, &key, NULL))
    keys[i++] = xstrdup (key);
  g_assert_cmpint (i, ==, n);
  keys[n] = NULL;

  return keys;
}

static xboolean_t
g_simple_action_group_query_action (xaction_group_t        *group,
                                    const xchar_t         *action_name,
                                    xboolean_t            *enabled,
                                    const xvariant_type_t **parameter_type,
                                    const xvariant_type_t **state_type,
                                    xvariant_t           **state_hint,
                                    xvariant_t           **state)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (group);
  xaction_t *action;

  action = xhash_table_lookup (simple->priv->table, action_name);

  if (action == NULL)
    return FALSE;

  if (enabled)
    *enabled = g_action_get_enabled (action);

  if (parameter_type)
    *parameter_type = g_action_get_parameter_type (action);

  if (state_type)
    *state_type = g_action_get_state_type (action);

  if (state_hint)
    *state_hint = g_action_get_state_hint (action);

  if (state)
    *state = g_action_get_state (action);

  return TRUE;
}

static void
g_simple_action_group_change_state (xaction_group_t *group,
                                    const xchar_t  *action_name,
                                    xvariant_t     *value)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (group);
  xaction_t *action;

  action = xhash_table_lookup (simple->priv->table, action_name);

  if (action == NULL)
    return;

  g_action_change_state (action, value);
}

static void
g_simple_action_group_activate (xaction_group_t *group,
                                const xchar_t  *action_name,
                                xvariant_t     *parameter)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (group);
  xaction_t *action;

  action = xhash_table_lookup (simple->priv->table, action_name);

  if (action == NULL)
    return;

  g_action_activate (action, parameter);
}

static void
action_enabled_notify (xaction_t     *action,
                       xparam_spec_t  *pspec,
                       xpointer_t     user_data)
{
  xaction_group_action_enabled_changed (user_data,
                                         g_action_get_name (action),
                                         g_action_get_enabled (action));
}

static void
action_state_notify (xaction_t    *action,
                     xparam_spec_t *pspec,
                     xpointer_t    user_data)
{
  xvariant_t *value;

  value = g_action_get_state (action);
  xaction_group_action_state_changed (user_data,
                                       g_action_get_name (action),
                                       value);
  xvariant_unref (value);
}

static void
g_simple_action_group_disconnect (xpointer_t key,
                                  xpointer_t value,
                                  xpointer_t user_data)
{
  g_signal_handlers_disconnect_by_func (value, action_enabled_notify,
                                        user_data);
  g_signal_handlers_disconnect_by_func (value, action_state_notify,
                                        user_data);
}

static xaction_t *
g_simple_action_group_lookup_action (xaction_map_t *action_map,
                                     const xchar_t        *action_name)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (action_map);

  return xhash_table_lookup (simple->priv->table, action_name);
}

static void
g_simple_action_group_add_action (xaction_map_t *action_map,
                                  xaction_t    *action)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (action_map);
  const xchar_t *action_name;
  xaction_t *old_action;

  action_name = g_action_get_name (action);
  if (action_name == NULL)
    {
      g_critical ("The supplied action has no name. You must set the "
                  "xaction_t:name property when creating an action.");
      return;
    }

  old_action = xhash_table_lookup (simple->priv->table, action_name);

  if (old_action != action)
    {
      if (old_action != NULL)
        {
          xaction_group_action_removed (XACTION_GROUP (simple),
                                         action_name);
          g_simple_action_group_disconnect (NULL, old_action, simple);
        }

      g_signal_connect (action, "notify::enabled",
                        G_CALLBACK (action_enabled_notify), simple);

      if (g_action_get_state_type (action) != NULL)
        g_signal_connect (action, "notify::state",
                          G_CALLBACK (action_state_notify), simple);

      xhash_table_insert (simple->priv->table,
                           xstrdup (action_name),
                           xobject_ref (action));

      xaction_group_action_added (XACTION_GROUP (simple), action_name);
    }
}

static void
g_simple_action_group_remove_action (xaction_map_t  *action_map,
                                     const xchar_t *action_name)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (action_map);
  xaction_t *action;

  action = xhash_table_lookup (simple->priv->table, action_name);

  if (action != NULL)
    {
      xaction_group_action_removed (XACTION_GROUP (simple), action_name);
      g_simple_action_group_disconnect (NULL, action, simple);
      xhash_table_remove (simple->priv->table, action_name);
    }
}

static void
g_simple_action_group_finalize (xobject_t *object)
{
  xsimple_action_group_t *simple = G_SIMPLE_ACTION_GROUP (object);

  xhash_table_foreach (simple->priv->table,
                        g_simple_action_group_disconnect,
                        simple);
  xhash_table_unref (simple->priv->table);

  G_OBJECT_CLASS (g_simple_action_group_parent_class)
    ->finalize (object);
}

static void
g_simple_action_group_init (xsimple_action_group_t *simple)
{
  simple->priv = g_simple_action_group_get_instance_private (simple);
  simple->priv->table = xhash_table_new_full (xstr_hash, xstr_equal,
                                               g_free, xobject_unref);
}

static void
g_simple_action_group_class_init (GSimpleActionGroupClass *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = g_simple_action_group_finalize;
}

static void
g_simple_action_group_iface_init (xaction_group_interface_t *iface)
{
  iface->list_actions = g_simple_action_group_list_actions;
  iface->query_action = g_simple_action_group_query_action;
  iface->change_action_state = g_simple_action_group_change_state;
  iface->activate_action = g_simple_action_group_activate;
}

static void
g_simple_action_group_map_iface_init (xaction_map_interface_t *iface)
{
  iface->add_action = g_simple_action_group_add_action;
  iface->remove_action = g_simple_action_group_remove_action;
  iface->lookup_action = g_simple_action_group_lookup_action;
}

/**
 * g_simple_action_group_new:
 *
 * Creates a new, empty, #xsimple_action_group_t.
 *
 * Returns: a new #xsimple_action_group_t
 *
 * Since: 2.28
 **/
xsimple_action_group_t *
g_simple_action_group_new (void)
{
  return xobject_new (XTYPE_SIMPLE_ACTION_GROUP, NULL);
}

/**
 * g_simple_action_group_lookup:
 * @simple: a #xsimple_action_group_t
 * @action_name: the name of an action
 *
 * Looks up the action with the name @action_name in the group.
 *
 * If no such action exists, returns %NULL.
 *
 * Returns: (transfer none): a #xaction_t, or %NULL
 *
 * Since: 2.28
 *
 * Deprecated: 2.38: Use xaction_map_lookup_action()
 */
xaction_t *
g_simple_action_group_lookup (xsimple_action_group_t *simple,
                              const xchar_t        *action_name)
{
  g_return_val_if_fail (X_IS_SIMPLE_ACTION_GROUP (simple), NULL);

  return xaction_map_lookup_action (G_ACTION_MAP (simple), action_name);
}

/**
 * g_simple_action_group_insert:
 * @simple: a #xsimple_action_group_t
 * @action: a #xaction_t
 *
 * Adds an action to the action group.
 *
 * If the action group already contains an action with the same name as
 * @action then the old action is dropped from the group.
 *
 * The action group takes its own reference on @action.
 *
 * Since: 2.28
 *
 * Deprecated: 2.38: Use xaction_map_add_action()
 **/
void
g_simple_action_group_insert (xsimple_action_group_t *simple,
                              xaction_t            *action)
{
  g_return_if_fail (X_IS_SIMPLE_ACTION_GROUP (simple));

  xaction_map_add_action (G_ACTION_MAP (simple), action);
}

/**
 * g_simple_action_group_remove:
 * @simple: a #xsimple_action_group_t
 * @action_name: the name of the action
 *
 * Removes the named action from the action group.
 *
 * If no action of this name is in the group then nothing happens.
 *
 * Since: 2.28
 *
 * Deprecated: 2.38: Use xaction_map_remove_action()
 **/
void
g_simple_action_group_remove (xsimple_action_group_t *simple,
                              const xchar_t        *action_name)
{
  g_return_if_fail (X_IS_SIMPLE_ACTION_GROUP (simple));

  xaction_map_remove_action (G_ACTION_MAP (simple), action_name);
}


/**
 * g_simple_action_group_add_entries:
 * @simple: a #xsimple_action_group_t
 * @entries: (array length=n_entries): a pointer to the first item in
 *           an array of #xaction_entry_t structs
 * @n_entries: the length of @entries, or -1
 * @user_data: the user data for signal connections
 *
 * A convenience function for creating multiple #xsimple_action_t instances
 * and adding them to the action group.
 *
 * Since: 2.30
 *
 * Deprecated: 2.38: Use xaction_map_add_action_entries()
 **/
void
g_simple_action_group_add_entries (xsimple_action_group_t *simple,
                                   const xaction_entry_t *entries,
                                   xint_t                n_entries,
                                   xpointer_t            user_data)
{
  xaction_map_add_action_entries (G_ACTION_MAP (simple), entries, n_entries, user_data);
}
