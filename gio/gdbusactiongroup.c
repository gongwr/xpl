/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2011 Canonical Limited
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

#include "gdbusactiongroup-private.h"

#include "gremoteactiongroup.h"
#include "gdbusconnection.h"
#include "gactiongroup.h"

/**
 * SECTION:gdbusactiongroup
 * @title: xdbus_action_group_t
 * @short_description: A D-Bus xaction_group_t implementation
 * @include: gio/gio.h
 * @see_also: [xaction_group_t exporter][gio-xaction_group_t-exporter]
 *
 * #xdbus_action_group_t is an implementation of the #xaction_group_t
 * interface that can be used as a proxy for an action group
 * that is exported over D-Bus with xdbus_connection_export_action_group().
 */

/**
 * xdbus_action_group_t:
 *
 * #xdbus_action_group_t is an opaque data structure and can only be accessed
 * using the following functions.
 */

struct _GDBusActionGroup
{
  xobject_t parent_instance;

  xdbus_connection_t *connection;
  xchar_t           *bus_name;
  xchar_t           *object_path;
  xuint_t            subscription_id;
  xhashtable_t      *actions;

  /* The 'strict' flag indicates that the non-existence of at least one
   * action has potentially been observed through the API.  This means
   * that we should always emit 'action-added' signals for all new
   * actions.
   *
   * The user can observe the non-existence of an action by listing the
   * actions or by performing a query (such as parameter type) on a
   * non-existent action.
   *
   * If the user has no way of knowing that a given action didn't
   * already exist then we can skip emitting 'action-added' signals
   * since they have no way of knowing that it wasn't there from the
   * start.
   */
  xboolean_t         strict;
};

typedef xobject_class_t xdbus_action_group_class_t;

typedef struct
{
  xchar_t        *name;
  xvariant_type_t *parameter_type;
  xboolean_t      enabled;
  xvariant_t     *state;
} action_info_t;

static void
action_info_free (xpointer_t user_data)
{
  action_info_t *info = user_data;

  g_free (info->name);

  if (info->state)
    xvariant_unref (info->state);

  if (info->parameter_type)
    xvariant_type_free (info->parameter_type);

  g_slice_free (action_info_t, info);
}

static action_info_t *
action_info_new_from_iter (xvariant_iter_t *iter)
{
  const xchar_t *param_str;
  action_info_t *info;
  xboolean_t enabled;
  xvariant_t *state;
  xchar_t *name;

  if (!xvariant_iter_next (iter, "{s(b&g@av)}", &name,
                            &enabled, &param_str, &state))
    return NULL;

  info = g_slice_new (action_info_t);
  info->name = name;
  info->enabled = enabled;

  if (xvariant_n_children (state))
    xvariant_get_child (state, 0, "v", &info->state);
  else
    info->state = NULL;
  xvariant_unref (state);

  if (param_str[0])
    info->parameter_type = xvariant_type_copy ((xvariant_type_t *) param_str);
  else
    info->parameter_type = NULL;

  return info;
}

static void xdbus_action_group_remote_iface_init (xremote_action_group_interface_t *iface);
static void xdbus_action_group_iface_init        (xaction_group_interface_t       *iface);
G_DEFINE_TYPE_WITH_CODE (xdbus_action_group, xdbus_action_group, XTYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (XTYPE_ACTION_GROUP, xdbus_action_group_iface_init)
  G_IMPLEMENT_INTERFACE (XTYPE_REMOTE_ACTION_GROUP, xdbus_action_group_remote_iface_init))

static void
xdbus_action_group_changed (xdbus_connection_t *connection,
                             const xchar_t     *sender,
                             const xchar_t     *object_path,
                             const xchar_t     *interface_name,
                             const xchar_t     *signal_name,
                             xvariant_t        *parameters,
                             xpointer_t         user_data)
{
  xdbus_action_group_t *group = user_data;
  xaction_group_t *g_group = user_data;

  /* make sure that we've been fully initialised */
  if (group->actions == NULL)
    return;

  if (xstr_equal (signal_name, "Changed") &&
      xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(asa{sb}a{sv}a{s(bgav)})")))
    {
      /* Removes */
      {
        xvariant_iter_t *iter;
        const xchar_t *name;

        xvariant_get_child (parameters, 0, "as", &iter);
        while (xvariant_iter_next (iter, "&s", &name))
          {
            if (xhash_table_lookup (group->actions, name))
              {
                xhash_table_remove (group->actions, name);
                xaction_group_action_removed (g_group, name);
              }
          }
        xvariant_iter_free (iter);
      }

      /* Enable changes */
      {
        xvariant_iter_t *iter;
        const xchar_t *name;
        xboolean_t enabled;

        xvariant_get_child (parameters, 1, "a{sb}", &iter);
        while (xvariant_iter_next (iter, "{&sb}", &name, &enabled))
          {
            action_info_t *info;

            info = xhash_table_lookup (group->actions, name);

            if (info && info->enabled != enabled)
              {
                info->enabled = enabled;
                xaction_group_action_enabled_changed (g_group, name, enabled);
              }
          }
        xvariant_iter_free (iter);
      }

      /* State changes */
      {
        xvariant_iter_t *iter;
        const xchar_t *name;
        xvariant_t *state;

        xvariant_get_child (parameters, 2, "a{sv}", &iter);
        while (xvariant_iter_next (iter, "{&sv}", &name, &state))
          {
            action_info_t *info;

            info = xhash_table_lookup (group->actions, name);

            if (info && info->state && !xvariant_equal (state, info->state) &&
                xvariant_is_of_type (state, xvariant_get_type (info->state)))
              {
                xvariant_unref (info->state);
                info->state = xvariant_ref (state);

                xaction_group_action_state_changed (g_group, name, state);
              }

            xvariant_unref (state);
          }
        xvariant_iter_free (iter);
      }

      /* Additions */
      {
        xvariant_iter_t *iter;
        action_info_t *info;

        xvariant_get_child (parameters, 3, "a{s(bgav)}", &iter);
        while ((info = action_info_new_from_iter (iter)))
          {
            if (!xhash_table_lookup (group->actions, info->name))
              {
                xhash_table_insert (group->actions, info->name, info);

                if (group->strict)
                  xaction_group_action_added (g_group, info->name);
              }
            else
              action_info_free (info);
          }
        xvariant_iter_free (iter);
      }
    }
}


static void
xdbus_action_group_describe_all_done (xobject_t      *source,
                                       xasync_result_t *result,
                                       xpointer_t      user_data)
{
  xdbus_action_group_t *group= user_data;
  xvariant_t *reply;

  g_assert (group->actions == NULL);
  group->actions = xhash_table_new_full (xstr_hash, xstr_equal, NULL, action_info_free);

  g_assert (group->connection == (xpointer_t) source);
  reply = xdbus_connection_call_finish (group->connection, result, NULL);

  if (reply != NULL)
    {
      xvariant_iter_t *iter;
      action_info_t *action;

      xvariant_get (reply, "(a{s(bgav)})", &iter);
      while ((action = action_info_new_from_iter (iter)))
        {
          xhash_table_insert (group->actions, action->name, action);

          if (group->strict)
            xaction_group_action_added (XACTION_GROUP (group), action->name);
        }
      xvariant_iter_free (iter);
      xvariant_unref (reply);
    }

  xobject_unref (group);
}


static void
xdbus_action_group_async_init (xdbus_action_group_t *group)
{
  if (group->subscription_id != 0)
    return;

  group->subscription_id =
    xdbus_connection_signal_subscribe (group->connection, group->bus_name, "org.gtk.Actions", "Changed", group->object_path,
                                        NULL, G_DBUS_SIGNAL_FLAGS_NONE, xdbus_action_group_changed, group, NULL);

  xdbus_connection_call (group->connection, group->bus_name, group->object_path, "org.gtk.Actions", "DescribeAll", NULL,
                          G_VARIANT_TYPE ("(a{s(bgav)})"), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                          xdbus_action_group_describe_all_done, xobject_ref (group));
}

static xchar_t **
xdbus_action_group_list_actions (xaction_group_t *g_group)
{
  xdbus_action_group_t *group = G_DBUS_ACTION_GROUP (g_group);
  xchar_t **keys;

  if (group->actions != NULL)
    {
      xhash_table_iter_t iter;
      xint_t n, i = 0;
      xpointer_t key;

      n = xhash_table_size (group->actions);
      keys = g_new (xchar_t *, n + 1);

      xhash_table_iter_init (&iter, group->actions);
      while (xhash_table_iter_next (&iter, &key, NULL))
        keys[i++] = xstrdup (key);
      g_assert_cmpint (i, ==, n);
      keys[n] = NULL;
    }
  else
    {
      xdbus_action_group_async_init (group);
      keys = g_new0 (xchar_t *, 1);
    }

  group->strict = TRUE;

  return keys;
}

static xboolean_t
xdbus_action_group_query_action (xaction_group_t        *g_group,
                                  const xchar_t         *action_name,
                                  xboolean_t            *enabled,
                                  const xvariant_type_t **parameter_type,
                                  const xvariant_type_t **state_type,
                                  xvariant_t           **state_hint,
                                  xvariant_t           **state)
{
  xdbus_action_group_t *group = G_DBUS_ACTION_GROUP (g_group);
  action_info_t *info;

  if (group->actions != NULL)
    {
      info = xhash_table_lookup (group->actions, action_name);

      if (info == NULL)
        {
          group->strict = TRUE;
          return FALSE;
        }

      if (enabled)
        *enabled = info->enabled;

      if (parameter_type)
        *parameter_type = info->parameter_type;

      if (state_type)
        *state_type = info->state ? xvariant_get_type (info->state) : NULL;

      if (state_hint)
        *state_hint = NULL;

      if (state)
        *state = info->state ? xvariant_ref (info->state) : NULL;

      return TRUE;
    }
  else
    {
      xdbus_action_group_async_init (group);
      group->strict = TRUE;

      return FALSE;
    }
}

static void
xdbus_action_group_activate_action_full (xremote_action_group_t *remote,
                                          const xchar_t        *action_name,
                                          xvariant_t           *parameter,
                                          xvariant_t           *platform_data)
{
  xdbus_action_group_t *group = G_DBUS_ACTION_GROUP (remote);
  xvariant_builder_t builder;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("av"));

  if (parameter)
    xvariant_builder_add (&builder, "v", parameter);

  xdbus_connection_call (group->connection, group->bus_name, group->object_path, "org.gtk.Actions", "Activate",
                          xvariant_new ("(sav@a{sv})", action_name, &builder, platform_data),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void
xdbus_action_group_change_action_state_full (xremote_action_group_t *remote,
                                              const xchar_t        *action_name,
                                              xvariant_t           *value,
                                              xvariant_t           *platform_data)
{
  xdbus_action_group_t *group = G_DBUS_ACTION_GROUP (remote);

  xdbus_connection_call (group->connection, group->bus_name, group->object_path, "org.gtk.Actions", "SetState",
                          xvariant_new ("(sv@a{sv})", action_name, value, platform_data),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void
xdbus_action_group_change_state (xaction_group_t *group,
                                  const xchar_t  *action_name,
                                  xvariant_t     *value)
{
  xdbus_action_group_change_action_state_full (G_REMOTE_ACTION_GROUP (group),
                                                action_name, value, xvariant_new ("a{sv}", NULL));
}

static void
xdbus_action_group_activate (xaction_group_t *group,
                              const xchar_t  *action_name,
                              xvariant_t     *parameter)
{
  xdbus_action_group_activate_action_full (G_REMOTE_ACTION_GROUP (group),
                                            action_name, parameter, xvariant_new ("a{sv}", NULL));
}

static void
xdbus_action_group_finalize (xobject_t *object)
{
  xdbus_action_group_t *group = G_DBUS_ACTION_GROUP (object);

  if (group->subscription_id)
    xdbus_connection_signal_unsubscribe (group->connection, group->subscription_id);

  if (group->actions)
    xhash_table_unref (group->actions);

  xobject_unref (group->connection);
  g_free (group->object_path);
  g_free (group->bus_name);

  G_OBJECT_CLASS (xdbus_action_group_parent_class)
    ->finalize (object);
}

static void
xdbus_action_group_init (xdbus_action_group_t *group)
{
}

static void
xdbus_action_group_class_init (xdbus_action_group_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = xdbus_action_group_finalize;
}

static void
xdbus_action_group_remote_iface_init (xremote_action_group_interface_t *iface)
{
  iface->activate_action_full = xdbus_action_group_activate_action_full;
  iface->change_action_state_full = xdbus_action_group_change_action_state_full;
}

static void
xdbus_action_group_iface_init (xaction_group_interface_t *iface)
{
  iface->list_actions = xdbus_action_group_list_actions;
  iface->query_action = xdbus_action_group_query_action;
  iface->change_action_state = xdbus_action_group_change_state;
  iface->activate_action = xdbus_action_group_activate;
}

/**
 * xdbus_action_group_get:
 * @connection: A #xdbus_connection_t
 * @bus_name: (nullable): the bus name which exports the action
 *     group or %NULL if @connection is not a message bus connection
 * @object_path: the object path at which the action group is exported
 *
 * Obtains a #xdbus_action_group_t for the action group which is exported at
 * the given @bus_name and @object_path.
 *
 * The thread default main context is taken at the time of this call.
 * All signals on the menu model (and any linked models) are reported
 * with respect to this context.  All calls on the returned menu model
 * (and linked models) must also originate from this same context, with
 * the thread default main context unchanged.
 *
 * This call is non-blocking.  The returned action group may or may not
 * already be filled in.  The correct thing to do is connect the signals
 * for the action group to monitor for changes and then to call
 * xaction_group_list_actions() to get the initial list.
 *
 * Returns: (transfer full): a #xdbus_action_group_t
 *
 * Since: 2.32
 */
xdbus_action_group_t *
xdbus_action_group_get (xdbus_connection_t *connection,
                         const xchar_t     *bus_name,
                         const xchar_t     *object_path)
{
  xdbus_action_group_t *group;

  g_return_val_if_fail (bus_name != NULL || xdbus_connection_get_unique_name (connection) == NULL, NULL);

  group = xobject_new (XTYPE_DBUS_ACTION_GROUP, NULL);
  group->connection = xobject_ref (connection);
  group->bus_name = xstrdup (bus_name);
  group->object_path = xstrdup (object_path);

  return group;
}

xboolean_t
xdbus_action_group_sync (xdbus_action_group_t  *group,
                          xcancellable_t      *cancellable,
                          xerror_t           **error)
{
  xvariant_t *reply;

  g_assert (group->subscription_id == 0);

  group->subscription_id =
    xdbus_connection_signal_subscribe (group->connection, group->bus_name, "org.gtk.Actions", "Changed", group->object_path,
                                        NULL, G_DBUS_SIGNAL_FLAGS_NONE, xdbus_action_group_changed, group, NULL);

  reply = xdbus_connection_call_sync (group->connection, group->bus_name, group->object_path, "org.gtk.Actions",
                                       "DescribeAll", NULL, G_VARIANT_TYPE ("(a{s(bgav)})"),
                                       G_DBUS_CALL_FLAGS_NONE, -1, cancellable, error);

  if (reply != NULL)
    {
      xvariant_iter_t *iter;
      action_info_t *action;

      g_assert (group->actions == NULL);
      group->actions = xhash_table_new_full (xstr_hash, xstr_equal, NULL, action_info_free);

      xvariant_get (reply, "(a{s(bgav)})", &iter);
      while ((action = action_info_new_from_iter (iter)))
        xhash_table_insert (group->actions, action->name, action);
      xvariant_iter_free (iter);
      xvariant_unref (reply);
    }

  return reply != NULL;
}
