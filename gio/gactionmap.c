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
#include "gactionmap.h"
#include "gaction.h"

/**
 * SECTION:gactionmap
 * @title: xaction_map_t
 * @include: gio/gio.h
 * @short_description: Interface for action containers
 *
 * The xaction_map_t interface is implemented by #xaction_group_t
 * implementations that operate by containing a number of
 * named #xaction_t instances, such as #xsimple_action_group_t.
 *
 * One useful application of this interface is to map the
 * names of actions from various action groups to unique,
 * prefixed names (e.g. by prepending "app." or "win.").
 * This is the motivation for the 'Map' part of the interface
 * name.
 *
 * Since: 2.32
 **/

/**
 * xaction_map_t:
 *
 * #xaction_map_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

/**
 * xaction_map_interface_t:
 * @lookup_action: the virtual function pointer for xaction_map_lookup_action()
 * @add_action: the virtual function pointer for xaction_map_add_action()
 * @remove_action: the virtual function pointer for xaction_map_remove_action()
 *
 * The virtual function table for #xaction_map_t.
 *
 * Since: 2.32
 **/

G_DEFINE_INTERFACE (xaction_map, xaction_map, XTYPE_OBJECT)

static void
xaction_map_default_init (xaction_map_interface_t *iface)
{
}

/**
 * xaction_map_lookup_action:
 * @action_map: a #xaction_map_t
 * @action_name: the name of an action
 *
 * Looks up the action with the name @action_name in @action_map.
 *
 * If no such action exists, returns %NULL.
 *
 * Returns: (nullable) (transfer none): a #xaction_t, or %NULL
 *
 * Since: 2.32
 */
xaction_t *
xaction_map_lookup_action (xaction_map_t  *action_map,
                            const xchar_t *action_name)
{
  return G_ACTION_MAP_GET_IFACE (action_map)
    ->lookup_action (action_map, action_name);
}

/**
 * xaction_map_add_action:
 * @action_map: a #xaction_map_t
 * @action: a #xaction_t
 *
 * Adds an action to the @action_map.
 *
 * If the action map already contains an action with the same name
 * as @action then the old action is dropped from the action map.
 *
 * The action map takes its own reference on @action.
 *
 * Since: 2.32
 */
void
xaction_map_add_action (xaction_map_t *action_map,
                         xaction_t    *action)
{
  G_ACTION_MAP_GET_IFACE (action_map)->add_action (action_map, action);
}

/**
 * xaction_map_remove_action:
 * @action_map: a #xaction_map_t
 * @action_name: the name of the action
 *
 * Removes the named action from the action map.
 *
 * If no action of this name is in the map then nothing happens.
 *
 * Since: 2.32
 */
void
xaction_map_remove_action (xaction_map_t  *action_map,
                            const xchar_t *action_name)
{
  G_ACTION_MAP_GET_IFACE (action_map)->remove_action (action_map, action_name);
}

/**
 * xaction_entry_t:
 * @name: the name of the action
 * @activate: the callback to connect to the "activate" signal of the
 *            action.  Since GLib 2.40, this can be %NULL for stateful
 *            actions, in which case the default handler is used.  For
 *            boolean-stated actions with no parameter, this is a
 *            toggle.  For other state types (and parameter type equal
 *            to the state type) this will be a function that
 *            just calls @change_state (which you should provide).
 * @parameter_type: the type of the parameter that must be passed to the
 *                  activate function for this action, given as a single
 *                  xvariant_t type string (or %NULL for no parameter)
 * @state: the initial state for this action, given in
 *         [xvariant_t text format][gvariant-text].  The state is parsed
 *         with no extra type information, so type tags must be added to
 *         the string if they are necessary.  Stateless actions should
 *         give %NULL here.
 * @change_state: the callback to connect to the "change-state" signal
 *                of the action.  All stateful actions should provide a
 *                handler here; stateless actions should not.
 *
 * This struct defines a single action.  It is for use with
 * xaction_map_add_action_entries().
 *
 * The order of the items in the structure are intended to reflect
 * frequency of use.  It is permissible to use an incomplete initialiser
 * in order to leave some of the later values as %NULL.  All values
 * after @name are optional.  Additional optional fields may be added in
 * the future.
 *
 * See xaction_map_add_action_entries() for an example.
 **/

/**
 * xaction_map_add_action_entries:
 * @action_map: a #xaction_map_t
 * @entries: (array length=n_entries) (element-type xaction_entry_t): a pointer to
 *           the first item in an array of #xaction_entry_t structs
 * @n_entries: the length of @entries, or -1 if @entries is %NULL-terminated
 * @user_data: the user data for signal connections
 *
 * A convenience function for creating multiple #xsimple_action_t instances
 * and adding them to a #xaction_map_t.
 *
 * Each action is constructed as per one #xaction_entry_t.
 *
 * |[<!-- language="C" -->
 * static void
 * activate_quit (xsimple_action_t *simple,
 *                xvariant_t      *parameter,
 *                xpointer_t       user_data)
 * {
 *   exit (0);
 * }
 *
 * static void
 * activate_print_string (xsimple_action_t *simple,
 *                        xvariant_t      *parameter,
 *                        xpointer_t       user_data)
 * {
 *   g_print ("%s\n", xvariant_get_string (parameter, NULL));
 * }
 *
 * static xaction_group_t *
 * create_action_group (void)
 * {
 *   const xaction_entry_t entries[] = {
 *     { "quit",         activate_quit              },
 *     { "print-string", activate_print_string, "s" }
 *   };
 *   xsimple_action_group_t *group;
 *
 *   group = g_simple_action_group_new ();
 *   xaction_map_add_action_entries (G_ACTION_MAP (group), entries, G_N_ELEMENTS (entries), NULL);
 *
 *   return XACTION_GROUP (group);
 * }
 * ]|
 *
 * Since: 2.32
 */
void
xaction_map_add_action_entries (xaction_map_t         *action_map,
                                 const xaction_entry_t *entries,
                                 xint_t                n_entries,
                                 xpointer_t            user_data)
{
  xint_t i;

  g_return_if_fail (X_IS_ACTION_MAP (action_map));
  g_return_if_fail (entries != NULL || n_entries == 0);

  for (i = 0; n_entries == -1 ? entries[i].name != NULL : i < n_entries; i++)
    {
      const xaction_entry_t *entry = &entries[i];
      const xvariant_type_t *parameter_type;
      xsimple_action_t *action;

      if (entry->parameter_type)
        {
          if (!xvariant_type_string_is_valid (entry->parameter_type))
            {
              g_critical ("xaction_map_add_entries: the type "
                          "string '%s' given as the parameter type for "
                          "action '%s' is not a valid xvariant_t type "
                          "string.  This action will not be added.",
                          entry->parameter_type, entry->name);
              return;
            }

          parameter_type = G_VARIANT_TYPE (entry->parameter_type);
        }
      else
        parameter_type = NULL;

      if (entry->state)
        {
          xerror_t *error = NULL;
          xvariant_t *state;

          state = xvariant_parse (NULL, entry->state, NULL, NULL, &error);
          if (state == NULL)
            {
              g_critical ("xaction_map_add_entries: xvariant_t could "
                          "not parse the state value given for action '%s' "
                          "('%s'): %s.  This action will not be added.",
                          entry->name, entry->state, error->message);
              xerror_free (error);
              continue;
            }

          action = g_simple_action_new_stateful (entry->name,
                                                 parameter_type,
                                                 state);

          xvariant_unref (state);
        }
      else
        {
          action = g_simple_action_new (entry->name,
                                        parameter_type);
        }

      if (entry->activate != NULL)
        g_signal_connect (action, "activate",
                          G_CALLBACK (entry->activate), user_data);

      if (entry->change_state != NULL)
        g_signal_connect (action, "change-state",
                          G_CALLBACK (entry->change_state), user_data);

      xaction_map_add_action (action_map, G_ACTION (action));
      xobject_unref (action);
    }
}
