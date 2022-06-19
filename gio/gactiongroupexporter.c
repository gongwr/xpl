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
 * @see_also: #xaction_group_t, #xdbus_action_group_t
 *
 * These functions support exporting a #xaction_group_t on D-Bus.
 * The D-Bus interface that is used is a private implementation
 * detail.
 *
 * To access an exported #xaction_group_t remotely, use
 * xdbus_action_group_get() to obtain a #xdbus_action_group_t.
 */

static xvariant_t *
xaction_group_describe_action (xaction_group_t *action_group,
                                const xchar_t  *name)
{
  const xvariant_type_t *type;
  xvariant_builder_t builder;
  xboolean_t enabled;
  xvariant_t *state;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("(bgav)"));

  enabled = xaction_group_get_action_enabled (action_group, name);
  xvariant_builder_add (&builder, "b", enabled);

  if ((type = xaction_group_get_action_parameter_type (action_group, name)))
    {
      xchar_t *str = xvariant_type_dup_string (type);
      xvariant_builder_add (&builder, "g", str);
      g_free (str);
    }
  else
    xvariant_builder_add (&builder, "g", "");

  xvariant_builder_open (&builder, G_VARIANT_TYPE ("av"));
  if ((state = xaction_group_get_action_state (action_group, name)))
    {
      xvariant_builder_add (&builder, "v", state);
      xvariant_unref (state);
    }
  xvariant_builder_close (&builder);

  return xvariant_builder_end (&builder);
}

/* Using XML saves us dozens of relocations vs. using the introspection
 * structure types.  We only need to burn cycles and memory if we
 * actually use the exporter -- not in every single app using GIO.
 *
 * It's also a lot easier to read. :)
 *
 * For documentation of this interface, see
 * https://wiki.gnome.org/Projects/GLib/xapplication_t/DBusAPI
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

static xdbus_interface_info_t *org_gtk_Actions;

typedef struct
{
  xaction_group_t    *action_group;
  xdbus_connection_t *connection;
  xmain_context_t    *context;
  xchar_t           *object_path;
  xhashtable_t      *pending_changes;
  xsource_t         *pendinxsource;
} GActionGroupExporter;

#define ACTION_ADDED_EVENT             (1u<<0)
#define ACTION_REMOVED_EVENT           (1u<<1)
#define ACTION_STATE_CHANGED_EVENT     (1u<<2)
#define ACTION_ENABLED_CHANGED_EVENT   (1u<<3)

static xboolean_t
xaction_group_exporter_dispatch_events (xpointer_t user_data)
{
  GActionGroupExporter *exporter = user_data;
  xvariant_builder_t removes;
  xvariant_builder_t enabled_changes;
  xvariant_builder_t state_changes;
  xvariant_builder_t adds;
  xhash_table_iter_t iter;
  xpointer_t value;
  xpointer_t key;

  xvariant_builder_init (&removes, G_VARIANT_TYPE_STRING_ARRAY);
  xvariant_builder_init (&enabled_changes, G_VARIANT_TYPE ("a{sb}"));
  xvariant_builder_init (&state_changes, G_VARIANT_TYPE ("a{sv}"));
  xvariant_builder_init (&adds, G_VARIANT_TYPE ("a{s(bgav)}"));

  xhash_table_iter_init (&iter, exporter->pending_changes);
  while (xhash_table_iter_next (&iter, &key, &value))
    {
      xuint_t events = GPOINTER_TO_INT (value);
      const xchar_t *name = key;

      /* Adds and removes are incompatible with enabled or state
       * changes, but we must report at least one event.
       */
      xassert (((events & (ACTION_ENABLED_CHANGED_EVENT | ACTION_STATE_CHANGED_EVENT)) == 0) !=
                ((events & (ACTION_REMOVED_EVENT | ACTION_ADDED_EVENT)) == 0));

      if (events & ACTION_REMOVED_EVENT)
        xvariant_builder_add (&removes, "s", name);

      if (events & ACTION_ENABLED_CHANGED_EVENT)
        {
          xboolean_t enabled;

          enabled = xaction_group_get_action_enabled (exporter->action_group, name);
          xvariant_builder_add (&enabled_changes, "{sb}", name, enabled);
        }

      if (events & ACTION_STATE_CHANGED_EVENT)
        {
          xvariant_t *state;

          state = xaction_group_get_action_state (exporter->action_group, name);
          xvariant_builder_add (&state_changes, "{sv}", name, state);
          xvariant_unref (state);
        }

      if (events & ACTION_ADDED_EVENT)
        {
          xvariant_t *description;

          description = xaction_group_describe_action (exporter->action_group, name);
          xvariant_builder_add (&adds, "{s@(bgav)}", name, description);
        }
    }

  xhash_table_remove_all (exporter->pending_changes);

  xdbus_connection_emit_signal (exporter->connection, NULL, exporter->object_path,
                                 "org.gtk.Actions", "Changed",
                                 xvariant_new ("(asa{sb}a{sv}a{s(bgav)})",
                                                &removes, &enabled_changes,
                                                &state_changes, &adds),
                                 NULL);

  exporter->pendinxsource = NULL;

  return FALSE;
}

static void
xaction_group_exporter_flush_queue (GActionGroupExporter *exporter)
{
  if (exporter->pendinxsource)
    {
      xsource_destroy (exporter->pendinxsource);
      xaction_group_exporter_dispatch_events (exporter);
      xassert (exporter->pendinxsource == NULL);
    }
}

static xuint_t
xaction_group_exporter_get_events (GActionGroupExporter *exporter,
                                    const xchar_t          *name)
{
  return (xsize_t) xhash_table_lookup (exporter->pending_changes, name);
}

static void
xaction_group_exporter_set_events (GActionGroupExporter *exporter,
                                    const xchar_t          *name,
                                    xuint_t                 events)
{
  xboolean_t have_events;
  xboolean_t is_queued;

  if (events != 0)
    xhash_table_insert (exporter->pending_changes, xstrdup (name), GINT_TO_POINTER (events));
  else
    xhash_table_remove (exporter->pending_changes, name);

  have_events = xhash_table_size (exporter->pending_changes) > 0;
  is_queued = exporter->pendinxsource != NULL;

  if (have_events && !is_queued)
    {
      xsource_t *source;

      source = g_idle_source_new ();
      exporter->pendinxsource = source;
      xsource_set_callback (source, xaction_group_exporter_dispatch_events, exporter, NULL);
      xsource_set_static_name (source, "[gio] xaction_group_exporter_dispatch_events");
      xsource_attach (source, exporter->context);
      xsource_unref (source);
    }

  if (!have_events && is_queued)
    {
      xsource_destroy (exporter->pendinxsource);
      exporter->pendinxsource = NULL;
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
  xassert (~event_mask & (ACTION_STATE_CHANGED_EVENT |
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
      xassert (~event_mask & ~(ACTION_STATE_CHANGED_EVENT | ACTION_ENABLED_CHANGED_EVENT));
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
  xassert (~event_mask & ACTION_REMOVED_EVENT ||
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
  xassert (~event_mask & ACTION_REMOVED_EVENT ||
            event_mask & ACTION_ADDED_EVENT);

  if (~event_mask & ACTION_ADDED_EVENT)
    event_mask |= ACTION_ENABLED_CHANGED_EVENT;

  xaction_group_exporter_set_events (exporter, action_name, event_mask);
}

static void
org_gtk_Actions_method_call (xdbus_connection_t       *connection,
                             const xchar_t           *sender,
                             const xchar_t           *object_path,
                             const xchar_t           *interface_name,
                             const xchar_t           *method_name,
                             xvariant_t              *parameters,
                             xdbus_method_invocation_t *invocation,
                             xpointer_t               user_data)
{
  GActionGroupExporter *exporter = user_data;
  xvariant_t *result = NULL;

  xaction_group_exporter_flush_queue (exporter);

  if (xstr_equal (method_name, "List"))
    {
      xchar_t **list;

      list = xaction_group_list_actions (exporter->action_group);
      result = xvariant_new ("(^as)", list);
      xstrfreev (list);
    }

  else if (xstr_equal (method_name, "Describe"))
    {
      const xchar_t *name;
      xvariant_t *desc;

      xvariant_get (parameters, "(&s)", &name);

      if (!xaction_group_has_action (exporter->action_group, name))
        {
          xdbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                                 "The named action ('%s') does not exist.", name);
          return;
        }

      desc = xaction_group_describe_action (exporter->action_group, name);
      result = xvariant_new ("(@(bgav))", desc);
    }

  else if (xstr_equal (method_name, "DescribeAll"))
    {
      xvariant_builder_t builder;
      xchar_t **list;
      xint_t i;

      list = xaction_group_list_actions (exporter->action_group);
      xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{s(bgav)}"));
      for (i = 0; list[i]; i++)
        {
          const xchar_t *name = list[i];
          xvariant_t *description;

          description = xaction_group_describe_action (exporter->action_group, name);
          xvariant_builder_add (&builder, "{s@(bgav)}", name, description);
        }
      result = xvariant_new ("(a{s(bgav)})", &builder);
      xstrfreev (list);
    }

  else if (xstr_equal (method_name, "Activate"))
    {
      xvariant_t *parameter = NULL;
      xvariant_t *platform_data;
      xvariant_iter_t *iter;
      const xchar_t *name;

      xvariant_get (parameters, "(&sav@a{sv})", &name, &iter, &platform_data);
      xvariant_iter_next (iter, "v", &parameter);
      xvariant_iter_free (iter);

      if (X_IS_REMOTE_ACTION_GROUP (exporter->action_group))
        xremote_action_group_activate_action_full (G_REMOTE_ACTION_GROUP (exporter->action_group),
                                                    name, parameter, platform_data);
      else
        xaction_group_activate_action (exporter->action_group, name, parameter);

      if (parameter)
        xvariant_unref (parameter);

      xvariant_unref (platform_data);
    }

  else if (xstr_equal (method_name, "SetState"))
    {
      xvariant_t *platform_data;
      const xchar_t *name;
      xvariant_t *state;

      xvariant_get (parameters, "(&sv@a{sv})", &name, &state, &platform_data);

      if (X_IS_REMOTE_ACTION_GROUP (exporter->action_group))
        xremote_action_group_change_action_state_full (G_REMOTE_ACTION_GROUP (exporter->action_group),
                                                        name, state, platform_data);
      else
        xaction_group_change_action_state (exporter->action_group, name, state);

      xvariant_unref (platform_data);
      xvariant_unref (state);
    }

  else
    g_assert_not_reached ();

  xdbus_method_invocation_return_value (invocation, result);
}

static void
xaction_group_exporter_free (xpointer_t user_data)
{
  GActionGroupExporter *exporter = user_data;

  xsignal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_added, exporter);
  xsignal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_enabled_changed, exporter);
  xsignal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_state_changed, exporter);
  xsignal_handlers_disconnect_by_func (exporter->action_group,
                                        xaction_group_exporter_action_removed, exporter);

  xhash_table_unref (exporter->pending_changes);
  if (exporter->pendinxsource)
    xsource_destroy (exporter->pendinxsource);

  xmain_context_unref (exporter->context);
  xobject_unref (exporter->connection);
  xobject_unref (exporter->action_group);
  g_free (exporter->object_path);

  g_slice_free (GActionGroupExporter, exporter);
}

/**
 * xdbus_connection_export_action_group:
 * @connection: a #xdbus_connection_t
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
 * xdbus_connection_unexport_action_group() with the return value of
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
xdbus_connection_export_action_group (xdbus_connection_t  *connection,
                                       const xchar_t      *object_path,
                                       xaction_group_t     *action_group,
                                       xerror_t          **error)
{
  const xdbus_interface_vtable_t vtable = {
    org_gtk_Actions_method_call, NULL, NULL, { 0 }
  };
  GActionGroupExporter *exporter;
  xuint_t id;

  if G_UNLIKELY (org_gtk_Actions == NULL)
    {
      xerror_t *error = NULL;
      xdbus_node_info_t *info;

      info = g_dbus_node_info_new_for_xml (org_gtk_Actions_xml, &error);
      if G_UNLIKELY (info == NULL)
        xerror ("%s", error->message);
      org_gtk_Actions = g_dbus_node_info_lookup_interface (info, "org.gtk.Actions");
      xassert (org_gtk_Actions != NULL);
      g_dbus_interface_info_ref (org_gtk_Actions);
      g_dbus_node_info_unref (info);
    }

  exporter = g_slice_new (GActionGroupExporter);
  id = xdbus_connection_register_object (connection, object_path, org_gtk_Actions, &vtable,
                                          exporter, xaction_group_exporter_free, error);

  if (id == 0)
    {
      g_slice_free (GActionGroupExporter, exporter);
      return 0;
    }

  exporter->context = xmain_context_ref_thread_default ();
  exporter->pending_changes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);
  exporter->pendinxsource = NULL;
  exporter->action_group = xobject_ref (action_group);
  exporter->connection = xobject_ref (connection);
  exporter->object_path = xstrdup (object_path);

  xsignal_connect (action_group, "action-added",
                    G_CALLBACK (xaction_group_exporter_action_added), exporter);
  xsignal_connect (action_group, "action-removed",
                    G_CALLBACK (xaction_group_exporter_action_removed), exporter);
  xsignal_connect (action_group, "action-state-changed",
                    G_CALLBACK (xaction_group_exporter_action_state_changed), exporter);
  xsignal_connect (action_group, "action-enabled-changed",
                    G_CALLBACK (xaction_group_exporter_action_enabled_changed), exporter);

  return id;
}

/**
 * xdbus_connection_unexport_action_group:
 * @connection: a #xdbus_connection_t
 * @export_id: the ID from xdbus_connection_export_action_group()
 *
 * Reverses the effect of a previous call to
 * xdbus_connection_export_action_group().
 *
 * It is an error to call this function with an ID that wasn't returned
 * from xdbus_connection_export_action_group() or to call it with the
 * same ID more than once.
 *
 * Since: 2.32
 **/
void
xdbus_connection_unexport_action_group (xdbus_connection_t *connection,
                                         xuint_t            export_id)
{
  xdbus_connection_unregister_object (connection, export_id);
}
