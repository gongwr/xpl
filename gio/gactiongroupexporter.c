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

#include "gactiongroupexporter.h"

#include "gdbusmethodinvocation.h"
#include "gremoteactiongroup.h"
#include "gdbusintrospection.h"
#include "gdbusconnection.h"
#include "gactiongroup.h"
#include "gdbuserror.h"

/**
 * SECTION:gactiongroupexporter
 * @title: xaction_group_t exporter
 * @include: gio/gio.h
 * @short_description: Export GActionGroups on D-Bus
 * @see_also: #xaction_group_t, #GDBusActionGroup
 *
 * These functions support exporting a #xaction_group_t on D-Bus.
 * The D-Bus interface that is used is a private implementation
 * detail.
 *
 * To access an exported #xaction_group_t remotely, use
 * g_dbus_action_group_get() to obtain a #GDBusActionGroup.
 */

static xvariant_t *
xaction_group_describe_action (xaction_group_t *action_group,
                                const xchar_t  *name)
{
  const xvariant_type_t *type;
  GVariantBuilder builder;
  xboolean_t enabled;
  xvariant_t *state;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("(bgav)"));

  enabled = xaction_group_get_action_enabled (action_group, name);
  g_variant_builder_add (&builder, "b", enabled);

  if ((type = xaction_group_get_action_parameter_type (action_group, name)))
    {
      xchar_t *str = g_variant_type_dup_string (type);
      g_variant_builder_add (&builder, "g", str);
      g_free (str);
    }
  else
    g_variant_builder_add (&builder, "g", "");

  g_variant_builder_open (&builder, G_VARIANT_TYPE ("av"));
  if ((state = xaction_group_get_action_state (action_group, name)))
    {
      g_variant_builder_add (&builder, "v", state);
      g_variant_unref (state);
    }
  g_variant_builder_close (&builder);

  return g_variant_builder_end (&builder);
}

/* Using XML saves us dozens of relocations vs. using the introspection
 * structure types.  We only need to burn cycles and memory if we
 * actually use the exporter -- not in every single app using GIO.
 *
 * It's also a lot easier to read. :)
 *
 * For documentation of this interface, see
 * https://wiki.gnome.org/Projects/GLib/GApplication/DBusAPI
 */
const char org_gtk_Actions_xml[] =
  "<node>"
  "  <interface name='org.gtk.Actions'>"
  "    <method name='List'>"
  "      <arg type='as' name='list' direction='out'/>"
  "    </method>"
  "    <method name='Describe'>"
  "      <arg type='s' name='action_name' direction='in'/>"
  "      <arg type='(bgav)' name='description' direction='out'/>"
  "    </method>"
  "    <method name='DescribeAll'>"
  "      <arg type='a{s(bgav)}' name='descriptions' direction='out'/>"
  "    </method>"
  "    <method name='Activate'>"
  "      <arg type='s' name='action_name' direction='in'/>"
  "      <arg type='av' name='parameter' direction='in'/>"
  "      <arg type='a{sv}' name='platform_data' direction='in'/>"
  "    </method>"
  "    <method name='SetState'>"
  "      <arg type='s' name='action_name' direction='in'/>"
  "      <arg type='v' name='value' direction='in'/>"
  "      <arg type='a{sv}' name='platform_data' direction='in'/>"
  "    </method>"
  "    <signal name='Changed'>"
  "      <arg type='as' name='removals'/>"
  "      <arg type='a{sb}' name='enable_changes'/>"
  "      <arg type='a{sv}' name='state_changes'/>"
  "      <arg type='a{s(bgav)}' name='additions'/>"
  "    </signal>"
  "  </interface>"
  "</node>";

static GDBusInterfaceInfo *org_gtk_Actions;

typedef struct
{
  xaction_group_t    *action_group;
  GDBusConnection *connection;
  GMainContext    *context;
  xchar_t           *object_path;
  GHashTable      *pending_changes;
  GSource         *pending_source;
} GActionGroupExporter;

#define ACTION_ADDED_EVENT             (1u<<0)
#define ACTION_REMOVED_EVENT           (1u<<1)
#define ACTION_STATE_CHANGED_EVENT     (1u<<2)
#define ACTION_ENABLED_CHANGED_EVENT   (1u<<3)

static xboolean_t
xaction_group_exporter_dispatch_events (xpointer_t user_data)
{
  GActionGroupExporter *exporter = user_data;
  GVariantBuilder removes;
  GVariantBuilder enabled_changes;
  GVariantBuilder state_changes;
  GVariantBuilder adds;
  GHashTableIter iter;
  xpointer_t value;
  xpointer_t key;

  g_variant_builder_init (&removes, G_VARIANT_TYPE_STRING_ARRAY);
  g_variant_builder_init (&enabled_changes, G_VARIANT_TYPE ("a{sb}"));
  g_variant_builder_init (&state_changes, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_init (&adds, G_VARIANT_TYPE ("a{s(bgav)}"));

  g_hash_table_iter_init (&iter, exporter->pending_changes);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      xuint_t events = GPOINTER_TO_INT (value);
      const xchar_t *name = key;

      /* Adds and removes are incompatible with enabled or state
       * changes, but we must report at least one event.
       */
      g_assert (((events & (ACTION_ENABLED_CHANGED_EVENT | ACTION_STATE_CHANGED_EVENT)) == 0) !=
                ((events & (ACTION_REMOVED_EVENT | ACTION_ADDED_EVENT)) == 0));

      if (events & ACTION_REMOVED_EVENT)
        g_variant_builder_add (&removes, "s", name);

      if (events & ACTION_ENABLED_CHANGED_EVENT)
        {
          xboolean_t enabled;

          enabled = xaction_group_get_action_enabled (exporter->action_group, name);
          g_variant_builder_add (&enabled_changes, "{sb}", name, enabled);
        }

      if (events & ACTION_STATE_CHANGED_EVENT)
        {
          xvariant_t *state;

          state = xaction_group_get_action_state (exporter->action_group, name);
          g_variant_builder_add (&state_changes, "{sv}", name, state);
          g_variant_unref (state);
        }

      if (events & ACTION_ADDED_EVENT)
        {
          xvariant_t *description;

          description = xaction_group_describe_action (exporter->action_group, name);
          g_variant_builder_add (&adds, "{s@(bgav)}", name, description);
        }
    }

  g_hash_table_remove_all (exporter->pending_changes);

  g_dbus_connection_emit_signal (exporter->connection, NULL, exporter->object_path,
                                 "org.gtk.Actions", "Changed",
                                 g_variant_new ("(asa{sb}a{sv}a{s(bgav)})",
                                                &removes, &enabled_changes,
                                                &state_changes, &adds),
                                 NULL);

  exporter->pending_source = NULL;

  return FALSE;
}

static void
xaction_group_exporter_flush_queue (GActionGroupExporter *exporter)
{
  if (exporter->pending_source)
    {
      g_source_destroy (exporter->pending_source);
      xaction_group_exporter_dispatch_events (exporter);
      g_assert (exporter->pending_source == NULL);
    }
}

static xuint_t
xaction_group_exporter_get_events (GActionGroupExporter *exporter,
                                    const xchar_t          *name)
{
  return (xsize_t) g_hash_table_lookup (exporter->pending_changes, name);
}

static void
xaction_group_exporter_set_events (GActionGroupExporter *exporter,
                                    const xchar_t          *name,
                                    xuint_t                 events)
{
  xboolean_t have_events;
  xboolean_t is_queued;

  if (events != 0)
    g_hash_table_insert (exporter->pending_changes, g_strdup (name), GINT_TO_POINTER (events));
  else
    g_hash_table_remove (exporter->pending_changes, name);

  have_events = g_hash_table_size (exporter->pending_changes) > 0;
  is_queued = exporter->pending_source != NULL;

  if (have_events && !is_queued)
    {
      GSource *source;

      source = g_idle_source_new ();
      exporter->pending_source = source;
      g_source_set_callback (source, xaction_group_exporter_dispatch_events, exporter, NULL);
      g_source_set_static_name (source, "[gio] xaction_group_exporter_dispatch_events");
      g_source_attach (source, exporter->context);
      g_source_unref (source);
    }

  if (!have_events && is_queued)
    {
      g_source_destroy (exporter->pending_source);
      exporter->pending_source = NULL;
    }
}

static void
xaction_group_exporter_action_added (xaction_group_t *action_group,
                                      const xchar_t  *action_name,
                                      xpointer_t      user_data)
{
  GActionGroupExporter *exporter = user_data;
  xuint_t event_mask;

  event_mask = xaction_group_exporter_get_events (exporter, action_name);

  /* The action is new, so we should not have any pending
   * enabled-changed or state-changed signals for it.
   */
  g_assert (~event_mask & (ACTION_STATE_CHANGED_EVENT |
                           ACTION_ENABLED_CHANGED_EVENT));

  event_mask |= ACTION_ADDED_EVENT;

  xaction_group_exporter_set_events (exporter, action_name, event_mask);
}

static void
xaction_group_exporter_action_removed (xaction_group_t *action_group,
                                        const xchar_t  *action_name,
                                        xpointer_t      user_data)
{
  GActionGroupExporter *exporter = user_data;
  xuint_t event_mask;

  event_mask = xaction_group_exporter_get_events (exporter, action_name);

  /* If the add event for this is still queued then just cancel it since
   * it's gone now.
   *
   * If the event was freshly added, there should not have been any
   * enabled or state changed events.
   */
  if (event_mask & ACTION_ADDED_EVENT)
    {
      g_assert (~event_mask & ~(ACTION_STATE_CHANGED_EVENT | ACTION_ENABLED_CHANGED_EVENT));
      event_mask &= ~ACTION_ADDED_EVENT;
    }

  /* Otherwise, queue a remove event.  Drop any state or enabled changes
   * that were queued before the remove. */
  else
    {
      event_mask |= ACTION_REMOVED_EVENT;
      event_mask &= ~(ACTION_STATE_CHANGED_EVENT | ACTION_ENABLED_CHANGED_EVENT);
    }

  xaction_group_exporter_set_events (exporter, action_name, event_mask);
}

static void
xaction_group_exporter_action_state_changed (xaction_group_t *action_group,
                                              const xchar_t  *action_name,
                                              xvariant_t     *value,
                                              xpointer_t      user_data)
{
  GActionGroupExporter *exporter = user_data;
  xuint_t event_mask;

  event_mask = xaction_group_exporter_get_events (exporter, action_name);

  /* If it was removed, it must have been added back.  Otherwise, why
   * are we hearing about changes?
   */
  g_assert (~event_mask & ACTION_REMOVED_EVENT ||
            event_mask & ACTION_ADDED_EVENT);

  /* If it is freshly added, don't also bother with the state change
   * signal since the updated state will be sent as part of the pending
   * add message.
   */
  if (~event_mask & ACTION_ADDED_EVENT)
    event_mask |= ACTION_STATE_CHANGED_EVENT;

  xaction_group_exporter_set_events (exporter, action_name, event_mask);
}

static void
xaction_group_exporter_action_enabled_changed (xaction_group_t *action_group,
                                                const xchar_t  *action_name,
                                                xboolean_t      enabled,
                                                xpointer_t      user_data)
{
  GActionGroupExporter *exporter = user_data;
  xuint_t event_mask;

  event_mask = xaction_group_exporter_get_events (exporter, action_name);

  /* Reasoning as above. */
  g_assert (~event_mask & ACTION_REMOVED_EVENT ||
            event_mask & ACTION_ADDED_EVENT);

  if (~event_mask & ACTION_ADDED_EVENT)
    event_mask |= ACTION_ENABLED_CHANGED_EVENT;

  xaction_group_exporter_set_events (exporter, action_name, event_mask);
}

static void
org_gtk_Actions_method_call (GDBusConnection       *connection,
                             const xchar_t           *sender,
                             const xchar_t           *object_path,
                             const xchar_t           *interface_name,
                             const xchar_t           *method_name,
                             xvariant_t              *parameters,
                             GDBusMethodInvocation *invocation,
                             xpointer_t               user_data)
{
  GActionGroupExporter *exporter = user_data;
  xvariant_t *result = NULL;

  xaction_group_exporter_flush_queue (exporter);

  if (g_str_equal (method_name, "List"))
    {
      xchar_t **list;

      list = xaction_group_list_actions (exporter->action_group);
      result = g_variant_new ("(^as)", list);
      g_strfreev (list);
    }

  else if (g_str_equal (method_name, "Describe"))
    {
      const xchar_t *name;
      xvariant_t *desc;

      g_variant_get (parameters, "(&s)", &name);

      if (!xaction_group_has_action (exporter->action_group, name))
        {
          g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                 "The named action ('%s') does not exist.", name);
          return;
        }

      desc = xaction_group_describe_action (exporter->action_group, name);
      result = g_variant_new ("(@(bgav))", desc);
    }

  else if (g_str_equal (method_name, "DescribeAll"))
    {
      GVariantBuilder builder;
      xchar_t **list;
      xint_t i;

      list = xaction_group_list_actions (exporter->action_group);
      g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{s(bgav)}"));
      for (i = 0; list[i]; i++)
        {
          const xchar_t *name = list[i];
          xvariant_t *description;

          description = xaction_group_describe_action (exporter->action_group, name);
          g_variant_builder_add (&builder, "{s@(bgav)}", name, description);
        }
      result = g_variant_new ("(a{s(bgav)})", &builder);
      g_strfreev (list);
    }

  else if (g_str_equal (method_name, "Activate"))
    {
      xvariant_t *parameter = NULL;
      xvariant_t *platform_data;
      GVariantIter *iter;
      const xchar_t *name;

      g_variant_get (parameters, "(&sav@a{sv})", &name, &iter, &platform_data);
      g_variant_iter_next (iter, "v", &parameter);
      g_variant_iter_free (iter);

      if (X_IS_REMOTE_ACTION_GROUP (exporter->action_group))
        g_remote_action_group_activate_action_full (G_REMOTE_ACTION_GROUP (exporter->action_group),
                                                    name, parameter, platform_data);
      else
        xaction_group_activate_action (exporter->action_group, name, parameter);

      if (parameter)
        g_variant_unref (parameter);

      g_variant_unref (platform_data);
    }

  else if (g_str_equal (method_name, "SetState"))
    {
      xvariant_t *platform_data;
      const xchar_t *name;
      xvariant_t *state;

      g_variant_get (parameters, "(&sv@a{sv})", &name, &state, &platform_data);

      if (X_IS_REMOTE_ACTION_GROUP (exporter->action_group))
        g_remote_action_group_change_action_state_full (G_REMOTE_ACTION_GROUP (exporter->action_group),
                                                        name, state, platform_data);
      else
        xaction_group_change_action_state (exporter->action_group, name, state);

      g_variant_unref (platform_data);
      g_variant_unref (state);
    }

  else
    g_assert_not_reached ();

  g_dbus_method_invocation_return_value (invocation, result);
}

static void
xaction_group_exporter_free (xpointer_t user_data)
{
  GActionGroupExporter *exporter = user_data;

  g_signal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_added, exporter);
  g_signal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_enabled_changed, exporter);
  g_signal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_state_changed, exporter);
  g_signal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_removed, exporter);

  g_hash_table_unref (exporter->pending_changes);
  if (exporter->pending_source)
    g_source_destroy (exporter->pending_source);

  g_main_context_unref (exporter->context);
  g_object_unref (exporter->connection);
  g_object_unref (exporter->action_group);
  g_free (exporter->object_path);

  g_slice_free (GActionGroupExporter, exporter);
}

/**
 * g_dbus_connection_export_action_group:
 * @connection: a #GDBusConnection
 * @object_path: a D-Bus object path
 * @action_group: a #xaction_group_t
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Exports @action_group on @connection at @object_path.
 *
 * The implemented D-Bus API should be considered private.  It is
 * subject to change in the future.
 *
 * A given object path can only have one action group exported on it.
 * If this constraint is violated, the export will fail and 0 will be
 * returned (with @error set accordingly).
 *
 * You can unexport the action group using
 * g_dbus_connection_unexport_action_group() with the return value of
 * this function.
 *
 * The thread default main context is taken at the time of this call.
 * All incoming action activations and state change requests are
 * reported from this context.  Any changes on the action group that
 * cause it to emit signals must also come from this same context.
 * Since incoming action activations and state change requests are
 * rather likely to cause changes on the action group, this effectively
 * limits a given action group to being exported from only one main
 * context.
 *
 * Returns: the ID of the export (never zero), or 0 in case of failure
 *
 * Since: 2.32
 **/
xuint_t
g_dbus_connection_export_action_group (GDBusConnection  *connection,
                                       const xchar_t      *object_path,
                                       xaction_group_t     *action_group,
                                       xerror_t          **error)
{
  const GDBusInterfaceVTable vtable = {
    org_gtk_Actions_method_call, NULL, NULL, { 0 }
  };
  GActionGroupExporter *exporter;
  xuint_t id;

  if G_UNLIKELY (org_gtk_Actions == NULL)
    {
      xerror_t *error = NULL;
      GDBusNodeInfo *info;

      info = g_dbus_node_info_new_for_xml (org_gtk_Actions_xml, &error);
      if G_UNLIKELY (info == NULL)
        g_error ("%s", error->message);
      org_gtk_Actions = g_dbus_node_info_lookup_interface (info, "org.gtk.Actions");
      g_assert (org_gtk_Actions != NULL);
      g_dbus_interface_info_ref (org_gtk_Actions);
      g_dbus_node_info_unref (info);
    }

  exporter = g_slice_new (GActionGroupExporter);
  id = g_dbus_connection_register_object (connection, object_path, org_gtk_Actions, &vtable,
                                          exporter, xaction_group_exporter_free, error);

  if (id == 0)
    {
      g_slice_free (GActionGroupExporter, exporter);
      return 0;
    }

  exporter->context = g_main_context_ref_thread_default ();
  exporter->pending_changes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  exporter->pending_source = NULL;
  exporter->action_group = g_object_ref (action_group);
  exporter->connection = g_object_ref (connection);
  exporter->object_path = g_strdup (object_path);

  g_signal_connect (action_group, "action-added",
                    G_CALLBACK (xaction_group_exporter_action_added), exporter);
  g_signal_connect (action_group, "action-removed",
                    G_CALLBACK (xaction_group_exporter_action_removed), exporter);
  g_signal_connect (action_group, "action-state-changed",
                    G_CALLBACK (xaction_group_exporter_action_state_changed), exporter);
  g_signal_connect (action_group, "action-enabled-changed",
                    G_CALLBACK (xaction_group_exporter_action_enabled_changed), exporter);

  return id;
}

/**
 * g_dbus_connection_unexport_action_group:
 * @connection: a #GDBusConnection
 * @export_id: the ID from g_dbus_connection_export_action_group()
 *
 * Reverses the effect of a previous call to
 * g_dbus_connection_export_action_group().
 *
 * It is an error to call this function with an ID that wasn't returned
 * from g_dbus_connection_export_action_group() or to call it with the
 * same ID more than once.
 *
 * Since: 2.32
 **/
void
g_dbus_connection_unexport_action_group (GDBusConnection *connection,
                                         xuint_t            export_id)
{
  g_dbus_connection_unregister_object (connection, export_id);
}
