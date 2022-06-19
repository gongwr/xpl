/*
 * Copyright © 2011 Canonical Ltd.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gmenuexporter.h"

#include "gdbusmethodinvocation.h"
#include "gdbusintrospection.h"
#include "gdbusnamewatching.h"
#include "gdbuserror.h"

/**
 * SECTION:gmenuexporter
 * @title: xmenu_model_t exporter
 * @short_description: Export GMenuModels on D-Bus
 * @include: gio/gio.h
 * @see_also: #xmenu_model_t, #xdbus_menu_model_t
 *
 * These functions support exporting a #xmenu_model_t on D-Bus.
 * The D-Bus interface that is used is a private implementation
 * detail.
 *
 * To access an exported #xmenu_model_t remotely, use
 * g_dbus_menu_model_get() to obtain a #xdbus_menu_model_t.
 */

/* {{{1 D-Bus Interface description */

/* For documentation of this interface, see
 * https://wiki.gnome.org/Projects/GLib/xapplication_t/DBusAPI
 */

static xdbus_interface_info_t *
org_gtk_Menus_get_interface (void)
{
  static xdbus_interface_info_t *interface_info;

  if (interface_info == NULL)
    {
      xerror_t *error = NULL;
      xdbus_node_info_t *info;

      info = g_dbus_node_info_new_for_xml ("<node>"
                                           "  <interface name='org.gtk.Menus'>"
                                           "    <method name='Start'>"
                                           "      <arg type='au' name='groups' direction='in'/>"
                                           "      <arg type='a(uuaa{sv})' name='content' direction='out'/>"
                                           "    </method>"
                                           "    <method name='End'>"
                                           "      <arg type='au' name='groups' direction='in'/>"
                                           "    </method>"
                                           "    <signal name='Changed'>"
                                           "      arg type='a(uuuuaa{sv})' name='changes'/>"
                                           "    </signal>"
                                           "  </interface>"
                                           "</node>", &error);
      if (info == NULL)
        xerror ("%s", error->message);
      interface_info = g_dbus_node_info_lookup_interface (info, "org.gtk.Menus");
      xassert (interface_info != NULL);
      g_dbus_interface_info_ref (interface_info);
      g_dbus_node_info_unref (info);
    }

  return interface_info;
}

/* {{{1 Forward declarations */
typedef struct _GMenuExporterMenu                           GMenuExporterMenu;
typedef struct _GMenuExporterLink                           GMenuExporterLink;
typedef struct _GMenuExporterGroup                          GMenuExporterGroup;
typedef struct _GMenuExporterRemote                         GMenuExporterRemote;
typedef struct _GMenuExporterWatch                          GMenuExporterWatch;
typedef struct _GMenuExporter                               GMenuExporter;

static xboolean_t                 xmenu_exporter_group_is_subscribed    (GMenuExporterGroup *group);
static xuint_t                    xmenu_exporter_group_get_id           (GMenuExporterGroup *group);
static GMenuExporter *          xmenu_exporter_group_get_exporter     (GMenuExporterGroup *group);
static GMenuExporterMenu *      xmenu_exporter_group_add_menu         (GMenuExporterGroup *group,
                                                                        xmenu_model_t         *model);
static void                     xmenu_exporter_group_remove_menu      (GMenuExporterGroup *group,
                                                                        xuint_t               id);

static GMenuExporterGroup *     xmenu_exporter_create_group           (GMenuExporter      *exporter);
static GMenuExporterGroup *     xmenu_exporter_lookup_group           (GMenuExporter      *exporter,
                                                                        xuint_t               group_id);
static void                     xmenu_exporter_report                 (GMenuExporter      *exporter,
                                                                        xvariant_t           *report);
static void                     xmenu_exporter_remove_group           (GMenuExporter      *exporter,
                                                                        xuint_t               id);

/* {{{1 GMenuExporterLink, GMenuExporterMenu */

struct _GMenuExporterMenu
{
  GMenuExporterGroup *group;
  xuint_t               id;

  xmenu_model_t *model;
  xulong_t      handler_id;
  xsequence_t  *item_links;
};

struct _GMenuExporterLink
{
  xchar_t             *name;
  GMenuExporterMenu *menu;
  GMenuExporterLink *next;
};

static void
xmenu_exporter_menu_free (GMenuExporterMenu *menu)
{
  xmenu_exporter_group_remove_menu (menu->group, menu->id);

  if (menu->handler_id != 0)
    xsignal_handler_disconnect (menu->model, menu->handler_id);

  if (menu->item_links != NULL)
    g_sequence_free (menu->item_links);

  xobject_unref (menu->model);

  g_slice_free (GMenuExporterMenu, menu);
}

static void
xmenu_exporter_link_free (xpointer_t data)
{
  GMenuExporterLink *link = data;

  while (link != NULL)
    {
      GMenuExporterLink *tmp = link;
      link = tmp->next;

      xmenu_exporter_menu_free (tmp->menu);
      g_free (tmp->name);

      g_slice_free (GMenuExporterLink, tmp);
    }
}

static GMenuExporterLink *
xmenu_exporter_menu_create_links (GMenuExporterMenu *menu,
                                   xint_t               position)
{
  GMenuExporterLink *list = NULL;
  xmenu_link_iter_t *iter;
  const char *name;
  xmenu_model_t *model;

  iter = xmenu_model_iterate_item_links (menu->model, position);

  while (xmenu_link_iter_get_next (iter, &name, &model))
    {
      GMenuExporterGroup *group;
      GMenuExporterLink *tmp;

      /* keep sections in the same group, but create new groups
       * otherwise
       */
      if (!xstr_equal (name, "section"))
        group = xmenu_exporter_create_group (xmenu_exporter_group_get_exporter (menu->group));
      else
        group = menu->group;

      tmp = g_slice_new (GMenuExporterLink);
      tmp->name = xstrconcat (":", name, NULL);
      tmp->menu = xmenu_exporter_group_add_menu (group, model);
      tmp->next = list;
      list = tmp;

      xobject_unref (model);
    }

  xobject_unref (iter);

  return list;
}

static xvariant_t *
xmenu_exporter_menu_describe_item (GMenuExporterMenu *menu,
                                    xint_t               position)
{
  xmenu_attribute_iter_t *attr_iter;
  xvariant_builder_t builder;
  GSequenceIter *iter;
  GMenuExporterLink *link;
  const char *name;
  xvariant_t *value;

  xvariant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);

  attr_iter = xmenu_model_iterate_item_attributes (menu->model, position);
  while (xmenu_attribute_iter_get_next (attr_iter, &name, &value))
    {
      xvariant_builder_add (&builder, "{sv}", name, value);
      xvariant_unref (value);
    }
  xobject_unref (attr_iter);

  iter = g_sequence_get_iter_at_pos (menu->item_links, position);
  for (link = g_sequence_get (iter); link; link = link->next)
    xvariant_builder_add (&builder, "{sv}", link->name,
                           xvariant_new ("(uu)", xmenu_exporter_group_get_id (link->menu->group), link->menu->id));

  return xvariant_builder_end (&builder);
}

static xvariant_t *
xmenu_exporter_menu_list (GMenuExporterMenu *menu)
{
  xvariant_builder_t builder;
  xint_t i, n;

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

  n = g_sequence_get_length (menu->item_links);
  for (i = 0; i < n; i++)
    xvariant_builder_add_value (&builder, xmenu_exporter_menu_describe_item (menu, i));

  return xvariant_builder_end (&builder);
}

static void
xmenu_exporter_menu_items_changed (xmenu_model_t *model,
                                    xint_t        position,
                                    xint_t        removed,
                                    xint_t        added,
                                    xpointer_t    user_data)
{
  GMenuExporterMenu *menu = user_data;
  GSequenceIter *point;
  xint_t i;

  xassert (menu->model == model);
  xassert (menu->item_links != NULL);
  xassert (position + removed <= g_sequence_get_length (menu->item_links));

  point = g_sequence_get_iter_at_pos (menu->item_links, position + removed);
  g_sequence_remove_range (g_sequence_get_iter_at_pos (menu->item_links, position), point);

  for (i = position; i < position + added; i++)
    g_sequence_insert_before (point, xmenu_exporter_menu_create_links (menu, i));

  if (xmenu_exporter_group_is_subscribed (menu->group))
    {
      xvariant_builder_t builder;

      xvariant_builder_init (&builder, G_VARIANT_TYPE ("(uuuuaa{sv})"));
      xvariant_builder_add (&builder, "u", xmenu_exporter_group_get_id (menu->group));
      xvariant_builder_add (&builder, "u", menu->id);
      xvariant_builder_add (&builder, "u", position);
      xvariant_builder_add (&builder, "u", removed);

      xvariant_builder_open (&builder, G_VARIANT_TYPE ("aa{sv}"));
      for (i = position; i < position + added; i++)
        xvariant_builder_add_value (&builder, xmenu_exporter_menu_describe_item (menu, i));
      xvariant_builder_close (&builder);

      xmenu_exporter_report (xmenu_exporter_group_get_exporter (menu->group), xvariant_builder_end (&builder));
    }
}

static void
xmenu_exporter_menu_prepare (GMenuExporterMenu *menu)
{
  xint_t n_items;

  xassert (menu->item_links == NULL);

  if (xmenu_model_is_mutable (menu->model))
    menu->handler_id = xsignal_connect (menu->model, "items-changed",
                                         G_CALLBACK (xmenu_exporter_menu_items_changed), menu);

  menu->item_links = g_sequence_new (xmenu_exporter_link_free);

  n_items = xmenu_model_get_n_items (menu->model);
  if (n_items)
    xmenu_exporter_menu_items_changed (menu->model, 0, 0, n_items, menu);
}

static GMenuExporterMenu *
xmenu_exporter_menu_new (GMenuExporterGroup *group,
                          xuint_t               id,
                          xmenu_model_t         *model)
{
  GMenuExporterMenu *menu;

  menu = g_slice_new0 (GMenuExporterMenu);
  menu->group = group;
  menu->id = id;
  menu->model = xobject_ref (model);

  return menu;
}

/* {{{1 GMenuExporterGroup */

struct _GMenuExporterGroup
{
  GMenuExporter *exporter;
  xuint_t          id;

  xhashtable_t *menus;
  xuint_t       next_menu_id;
  xboolean_t    prepared;

  xint_t subscribed;
};

static void
xmenu_exporter_group_check_if_useless (GMenuExporterGroup *group)
{
  if (xhash_table_size (group->menus) == 0 && group->subscribed == 0)
    {
      xmenu_exporter_remove_group (group->exporter, group->id);

      xhash_table_unref (group->menus);

      g_slice_free (GMenuExporterGroup, group);
    }
}

static void
xmenu_exporter_group_subscribe (GMenuExporterGroup *group,
                                 xvariant_builder_t    *builder)
{
  xhash_table_iter_t iter;
  xpointer_t key, val;

  if (!group->prepared)
    {
      GMenuExporterMenu *menu;

      /* set this first, so that any menus created during the
       * preparation of the first menu also end up in the prepared
       * state.
       * */
      group->prepared = TRUE;

      menu = xhash_table_lookup (group->menus, 0);

      /* If the group was created by a subscription and does not yet
       * exist, it won't have a root menu...
       *
       * That menu will be prepared if it is ever added (due to
       * group->prepared == TRUE).
       */
      if (menu)
        xmenu_exporter_menu_prepare (menu);
    }

  group->subscribed++;

  xhash_table_iter_init (&iter, group->menus);
  while (xhash_table_iter_next (&iter, &key, &val))
    {
      xuint_t id = GPOINTER_TO_INT (key);
      GMenuExporterMenu *menu = val;

      if (!g_sequence_is_empty (menu->item_links))
        {
          xvariant_builder_open (builder, G_VARIANT_TYPE ("(uuaa{sv})"));
          xvariant_builder_add (builder, "u", group->id);
          xvariant_builder_add (builder, "u", id);
          xvariant_builder_add_value (builder, xmenu_exporter_menu_list (menu));
          xvariant_builder_close (builder);
        }
    }
}

static void
xmenu_exporter_group_unsubscribe (GMenuExporterGroup *group,
                                   xint_t                count)
{
  xassert (group->subscribed >= count);

  group->subscribed -= count;

  xmenu_exporter_group_check_if_useless (group);
}

static GMenuExporter *
xmenu_exporter_group_get_exporter (GMenuExporterGroup *group)
{
  return group->exporter;
}

static xboolean_t
xmenu_exporter_group_is_subscribed (GMenuExporterGroup *group)
{
  return group->subscribed > 0;
}

static xuint_t
xmenu_exporter_group_get_id (GMenuExporterGroup *group)
{
  return group->id;
}

static void
xmenu_exporter_group_remove_menu (GMenuExporterGroup *group,
                                   xuint_t               id)
{
  xhash_table_remove (group->menus, GINT_TO_POINTER (id));

  xmenu_exporter_group_check_if_useless (group);
}

static GMenuExporterMenu *
xmenu_exporter_group_add_menu (GMenuExporterGroup *group,
                                xmenu_model_t         *model)
{
  GMenuExporterMenu *menu;
  xuint_t id;

  id = group->next_menu_id++;
  menu = xmenu_exporter_menu_new (group, id, model);
  xhash_table_insert (group->menus, GINT_TO_POINTER (id), menu);

  if (group->prepared)
    xmenu_exporter_menu_prepare (menu);

  return menu;
}

static GMenuExporterGroup *
xmenu_exporter_group_new (GMenuExporter *exporter,
                           xuint_t          id)
{
  GMenuExporterGroup *group;

  group = g_slice_new0 (GMenuExporterGroup);
  group->menus = xhash_table_new (NULL, NULL);
  group->exporter = exporter;
  group->id = id;

  return group;
}

/* {{{1 GMenuExporterRemote */

struct _GMenuExporterRemote
{
  GMenuExporter *exporter;
  xhashtable_t    *watches;
  xuint_t          watch_id;
};

static void
xmenu_exporter_remote_subscribe (GMenuExporterRemote *remote,
                                  xuint_t                group_id,
                                  xvariant_builder_t     *builder)
{
  GMenuExporterGroup *group;
  xuint_t count;

  count = (xsize_t) xhash_table_lookup (remote->watches, GINT_TO_POINTER (group_id));
  xhash_table_insert (remote->watches, GINT_TO_POINTER (group_id), GINT_TO_POINTER (count + 1));

  /* Group will be created (as empty/unsubscribed if it does not exist) */
  group = xmenu_exporter_lookup_group (remote->exporter, group_id);
  xmenu_exporter_group_subscribe (group, builder);
}

static void
xmenu_exporter_remote_unsubscribe (GMenuExporterRemote *remote,
                                    xuint_t                group_id)
{
  GMenuExporterGroup *group;
  xuint_t count;

  count = (xsize_t) xhash_table_lookup (remote->watches, GINT_TO_POINTER (group_id));

  if (count == 0)
    return;

  if (count != 1)
    xhash_table_insert (remote->watches, GINT_TO_POINTER (group_id), GINT_TO_POINTER (count - 1));
  else
    xhash_table_remove (remote->watches, GINT_TO_POINTER (group_id));

  group = xmenu_exporter_lookup_group (remote->exporter, group_id);
  xmenu_exporter_group_unsubscribe (group, 1);
}

static xboolean_t
xmenu_exporter_remote_has_subscriptions (GMenuExporterRemote *remote)
{
  return xhash_table_size (remote->watches) != 0;
}

static void
xmenu_exporter_remote_free (xpointer_t data)
{
  GMenuExporterRemote *remote = data;
  xhash_table_iter_t iter;
  xpointer_t key, val;

  xhash_table_iter_init (&iter, remote->watches);
  while (xhash_table_iter_next (&iter, &key, &val))
    {
      GMenuExporterGroup *group;

      group = xmenu_exporter_lookup_group (remote->exporter, GPOINTER_TO_INT (key));
      xmenu_exporter_group_unsubscribe (group, GPOINTER_TO_INT (val));
    }

  if (remote->watch_id > 0)
    g_bus_unwatch_name (remote->watch_id);

  xhash_table_unref (remote->watches);

  g_slice_free (GMenuExporterRemote, remote);
}

static GMenuExporterRemote *
xmenu_exporter_remote_new (GMenuExporter *exporter,
                            xuint_t          watch_id)
{
  GMenuExporterRemote *remote;

  remote = g_slice_new0 (GMenuExporterRemote);
  remote->exporter = exporter;
  remote->watches = xhash_table_new (NULL, NULL);
  remote->watch_id = watch_id;

  return remote;
}

/* {{{1 GMenuExporter */

struct _GMenuExporter
{
  xdbus_connection_t *connection;
  xchar_t *object_path;
  xuint_t registration_id;
  xhashtable_t *groups;
  xuint_t next_group_id;

  GMenuExporterMenu *root;
  GMenuExporterRemote *peer_remote;
  xhashtable_t *remotes;
};

static void
xmenu_exporter_name_vanished (xdbus_connection_t *connection,
                               const xchar_t     *name,
                               xpointer_t         user_data)
{
  GMenuExporter *exporter = user_data;

  /* connection == NULL when we get called because the connection closed */
  xassert (exporter->connection == connection || connection == NULL);

  xhash_table_remove (exporter->remotes, name);
}

static xvariant_t *
xmenu_exporter_subscribe (GMenuExporter *exporter,
                           const xchar_t   *sender,
                           xvariant_t      *group_ids)
{
  GMenuExporterRemote *remote;
  xvariant_builder_t builder;
  xvariant_iter_t iter;
  xuint32_t id;

  if (sender != NULL)
    remote = xhash_table_lookup (exporter->remotes, sender);
  else
    remote = exporter->peer_remote;

  if (remote == NULL)
    {
      if (sender != NULL)
        {
          xuint_t watch_id;

          watch_id = g_bus_watch_name_on_connection (exporter->connection, sender, G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                     NULL, xmenu_exporter_name_vanished, exporter, NULL);
          remote = xmenu_exporter_remote_new (exporter, watch_id);
          xhash_table_insert (exporter->remotes, xstrdup (sender), remote);
        }
      else
        remote = exporter->peer_remote =
          xmenu_exporter_remote_new (exporter, 0);
    }

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("(a(uuaa{sv}))"));

  xvariant_builder_open (&builder, G_VARIANT_TYPE ("a(uuaa{sv})"));

  xvariant_iter_init (&iter, group_ids);
  while (xvariant_iter_next (&iter, "u", &id))
    xmenu_exporter_remote_subscribe (remote, id, &builder);

  xvariant_builder_close (&builder);

  return xvariant_builder_end (&builder);
}

static void
xmenu_exporter_unsubscribe (GMenuExporter *exporter,
                             const xchar_t   *sender,
                             xvariant_t      *group_ids)
{
  GMenuExporterRemote *remote;
  xvariant_iter_t iter;
  xuint32_t id;

  if (sender != NULL)
    remote = xhash_table_lookup (exporter->remotes, sender);
  else
    remote = exporter->peer_remote;

  if (remote == NULL)
    return;

  xvariant_iter_init (&iter, group_ids);
  while (xvariant_iter_next (&iter, "u", &id))
    xmenu_exporter_remote_unsubscribe (remote, id);

  if (!xmenu_exporter_remote_has_subscriptions (remote))
    {
      if (sender != NULL)
        xhash_table_remove (exporter->remotes, sender);
      else
        g_clear_pointer (&exporter->peer_remote, xmenu_exporter_remote_free);
    }
}

static void
xmenu_exporter_report (GMenuExporter *exporter,
                        xvariant_t      *report)
{
  xvariant_builder_t builder;

  xvariant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);
  xvariant_builder_open (&builder, G_VARIANT_TYPE_ARRAY);
  xvariant_builder_add_value (&builder, report);
  xvariant_builder_close (&builder);

  xdbus_connection_emit_signal (exporter->connection,
                                 NULL,
                                 exporter->object_path,
                                 "org.gtk.Menus", "Changed",
                                 xvariant_builder_end (&builder),
                                 NULL);
}

static void
xmenu_exporter_remove_group (GMenuExporter *exporter,
                              xuint_t          id)
{
  xhash_table_remove (exporter->groups, GINT_TO_POINTER (id));
}

static GMenuExporterGroup *
xmenu_exporter_lookup_group (GMenuExporter *exporter,
                              xuint_t          group_id)
{
  GMenuExporterGroup *group;

  group = xhash_table_lookup (exporter->groups, GINT_TO_POINTER (group_id));

  if (group == NULL)
    {
      group = xmenu_exporter_group_new (exporter, group_id);
      xhash_table_insert (exporter->groups, GINT_TO_POINTER (group_id), group);
    }

  return group;
}

static GMenuExporterGroup *
xmenu_exporter_create_group (GMenuExporter *exporter)
{
  GMenuExporterGroup *group;
  xuint_t id;

  id = exporter->next_group_id++;
  group = xmenu_exporter_group_new (exporter, id);
  xhash_table_insert (exporter->groups, GINT_TO_POINTER (id), group);

  return group;
}

static void
xmenu_exporter_free (xpointer_t user_data)
{
  GMenuExporter *exporter = user_data;

  xmenu_exporter_menu_free (exporter->root);
  g_clear_pointer (&exporter->peer_remote, xmenu_exporter_remote_free);
  xhash_table_unref (exporter->remotes);
  xhash_table_unref (exporter->groups);
  xobject_unref (exporter->connection);
  g_free (exporter->object_path);

  g_slice_free (GMenuExporter, exporter);
}

static void
xmenu_exporter_method_call (xdbus_connection_t       *connection,
                             const xchar_t           *sender,
                             const xchar_t           *object_path,
                             const xchar_t           *interface_name,
                             const xchar_t           *method_name,
                             xvariant_t              *parameters,
                             xdbus_method_invocation_t *invocation,
                             xpointer_t               user_data)
{
  GMenuExporter *exporter = user_data;
  xvariant_t *group_ids;

  group_ids = xvariant_get_child_value (parameters, 0);

  if (xstr_equal (method_name, "Start"))
    xdbus_method_invocation_return_value (invocation, xmenu_exporter_subscribe (exporter, sender, group_ids));

  else if (xstr_equal (method_name, "End"))
    {
      xmenu_exporter_unsubscribe (exporter, sender, group_ids);
      xdbus_method_invocation_return_value (invocation, NULL);
    }

  else
    g_assert_not_reached ();

  xvariant_unref (group_ids);
}

/* {{{1 Public API */

/**
 * xdbus_connection_export_menu_model:
 * @connection: a #xdbus_connection_t
 * @object_path: a D-Bus object path
 * @menu: a #xmenu_model_t
 * @error: return location for an error, or %NULL
 *
 * Exports @menu on @connection at @object_path.
 *
 * The implemented D-Bus API should be considered private.
 * It is subject to change in the future.
 *
 * An object path can only have one menu model exported on it. If this
 * constraint is violated, the export will fail and 0 will be
 * returned (with @error set accordingly).
 *
 * You can unexport the menu model using
 * xdbus_connection_unexport_menu_model() with the return value of
 * this function.
 *
 * Returns: the ID of the export (never zero), or 0 in case of failure
 *
 * Since: 2.32
 */
xuint_t
xdbus_connection_export_menu_model (xdbus_connection_t  *connection,
                                     const xchar_t      *object_path,
                                     xmenu_model_t       *menu,
                                     xerror_t          **error)
{
  const xdbus_interface_vtable_t vtable = {
    xmenu_exporter_method_call, NULL, NULL, { 0 }
  };
  GMenuExporter *exporter;
  xuint_t id;

  exporter = g_slice_new0 (GMenuExporter);

  id = xdbus_connection_register_object (connection, object_path, org_gtk_Menus_get_interface (),
                                          &vtable, exporter, xmenu_exporter_free, error);

  if (id == 0)
    {
      g_slice_free (GMenuExporter, exporter);
      return 0;
    }

  exporter->connection = xobject_ref (connection);
  exporter->object_path = xstrdup (object_path);
  exporter->groups = xhash_table_new (NULL, NULL);
  exporter->remotes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, xmenu_exporter_remote_free);
  exporter->root = xmenu_exporter_group_add_menu (xmenu_exporter_create_group (exporter), menu);

  return id;
}

/**
 * xdbus_connection_unexport_menu_model:
 * @connection: a #xdbus_connection_t
 * @export_id: the ID from xdbus_connection_export_menu_model()
 *
 * Reverses the effect of a previous call to
 * xdbus_connection_export_menu_model().
 *
 * It is an error to call this function with an ID that wasn't returned
 * from xdbus_connection_export_menu_model() or to call it with the
 * same ID more than once.
 *
 * Since: 2.32
 */
void
xdbus_connection_unexport_menu_model (xdbus_connection_t *connection,
                                       xuint_t            export_id)
{
  xdbus_connection_unregister_object (connection, export_id);
}

/* {{{1 Epilogue */
/* vim:set foldmethod=marker: */
