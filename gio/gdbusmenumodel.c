/*
 * Copyright © 2011 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gdbusmenumodel.h"

#include "gmenumodel.h"

/* Prelude {{{1 */

/**
 * SECTION:gdbusmenumodel
 * @title: xdbus_menu_model_t
 * @short_description: A D-Bus xmenu_model_t implementation
 * @include: gio/gio.h
 * @see_also: [xmenu_model_t Exporter][gio-xmenu_model_t-exporter]
 *
 * #xdbus_menu_model_t is an implementation of #xmenu_model_t that can be used
 * as a proxy for a menu model that is exported over D-Bus with
 * g_dbus_connection_export_menu_model().
 */

/**
 * xdbus_menu_model_t:
 *
 * #xdbus_menu_model_t is an opaque data structure and can only be accessed
 * using the following functions.
 */

/*
 * There are 3 main (quasi-)classes involved here:
 *
 *   - GDBusMenuPath
 *   - GDBusMenuGroup
 *   - xdbus_menu_model_t
 *
 * Each of these classes exists as a parameterised singleton keyed to a
 * particular thing:
 *
 *   - GDBusMenuPath represents a D-Bus object path on a particular
 *     unique bus name on a particular xdbus_connection_t and in a
 *     particular xmain_context_t.
 *
 *   - GDBusMenuGroup represents a particular group on a particular
 *     GDBusMenuPath.
 *
 *   - xdbus_menu_model_t represents a particular menu within a particular
 *     GDBusMenuGroup.
 *
 * There are also two (and a half) utility structs:
 *
 *  - PathIdentifier and ConstPathIdentifier
 *  - GDBusMenuModelItem
 *
 * PathIdentifier is the 4-tuple of (xmain_context_t, xdbus_connection_t,
 * unique name, object path) that uniquely identifies a particular
 * GDBusMenuPath.  It holds ownership on each of these things, so we
 * have a ConstPathIdentifier variant that does not.
 *
 * We have a 3-level hierarchy of hashtables:
 *
 *   - a global hashtable (g_dbus_menu_paths) maps from PathIdentifier
 *     to GDBusMenuPath
 *
 *   - each GDBusMenuPath has a hashtable mapping from xuint_t (group
 *     number) to GDBusMenuGroup
 *
 *   - each GDBusMenuGroup has a hashtable mapping from xuint_t (menu
 *     number) to xdbus_menu_model_t.
 *
 * In this way, each quintuplet of (connection, bus name, object path,
 * group id, menu id) maps to a single xdbus_menu_model_t instance that can be
 * located via 3 hashtable lookups.
 *
 * All of the 3 classes are refcounted (GDBusMenuPath and
 * GDBusMenuGroup manually, and xdbus_menu_model_t by virtue of being a
 * xobject_t).  The hashtables do not hold references -- rather, when the
 * last reference is dropped, the object is removed from the hashtable.
 *
 * The hard references go in the other direction: xdbus_menu_model_t is created
 * as the user requests it and only exists as long as the user holds a
 * reference on it.  xdbus_menu_model_t holds a reference on the GDBusMenuGroup
 * from which it came. GDBusMenuGroup holds a reference on
 * GDBusMenuPath.
 *
 * In addition to refcounts, each object has an 'active' variable (ints
 * for GDBusMenuPath and GDBusMenuGroup, boolean for xdbus_menu_model_t).
 *
 *   - xdbus_menu_model_t is inactive when created and becomes active only when
 *     first queried for information.  This prevents extra work from
 *     happening just by someone acquiring a xdbus_menu_model_t (and not
 *     actually trying to display it yet).
 *
 *   - The active count on GDBusMenuGroup is equal to the number of
 *     xdbus_menu_model_t instances in that group that are active.  When the
 *     active count transitions from 0 to 1, the group calls the 'Start'
 *     method on the service to begin monitoring that group.  When it
 *     drops from 1 to 0, the group calls the 'End' method to stop
 *     monitoring.
 *
 *   - The active count on GDBusMenuPath is equal to the number of
 *     GDBusMenuGroup instances on that path with a non-zero active
 *     count.  When the active count transitions from 0 to 1, the path
 *     sets up a signal subscription to monitor any changes.  The signal
 *     subscription is taken down when the active count transitions from
 *     1 to 0.
 *
 * When active, GDBusMenuPath gets incoming signals when changes occur.
 * If the change signal mentions a group for which we currently have an
 * active GDBusMenuGroup, the change signal is passed along to that
 * group.  If the group is inactive, the change signal is ignored.
 *
 * Most of the "work" occurs in GDBusMenuGroup.  In addition to the
 * hashtable of xdbus_menu_model_t instances, it keeps a hashtable of the actual
 * menu contents, each encoded as xsequence_t of GDBusMenuModelItem.  It
 * initially populates this table with the results of the "Start" method
 * call and then updates it according to incoming change signals.  If
 * the change signal mentions a menu for which we current have an active
 * xdbus_menu_model_t, the change signal is passed along to that model.  If the
 * model is inactive, the change signal is ignored.
 *
 * GDBusMenuModelItem is just a pair of hashtables, one for the attributes
 * and one for the links of the item.  Both map strings to xvariant_t
 * instances.  In the case of links, the xvariant_t has type '(uu)' and is
 * turned into a xdbus_menu_model_t at the point that the user pulls it through
 * the API.
 *
 * Following the "empty is the same as non-existent" rule, the hashtable
 * of xsequence_t of GDBusMenuModelItem holds NULL for empty menus.
 *
 * xdbus_menu_model_t contains very little functionality of its own.  It holds a
 * (weak) reference to the xsequence_t of GDBusMenuModelItem contained in the
 * GDBusMenuGroup.  It uses this xsequence_t to implement the xmenu_model_t
 * interface.  It also emits the "items-changed" signal if it is active
 * and it was told that the contents of the xsequence_t changed.
 */

typedef struct _GDBusMenuGroup GDBusMenuGroup;
typedef struct _GDBusMenuPath GDBusMenuPath;

static void                     g_dbus_menu_group_changed               (GDBusMenuGroup  *group,
                                                                         xuint_t            menu_id,
                                                                         xint_t             position,
                                                                         xint_t             removed,
                                                                         xvariant_t        *added);
static void                     g_dbus_menu_model_changed               (xdbus_menu_model_t  *proxy,
                                                                         xsequence_t       *items,
                                                                         xint_t             position,
                                                                         xint_t             removed,
                                                                         xint_t             added);
static GDBusMenuGroup *         g_dbus_menu_group_get_from_path         (GDBusMenuPath   *path,
                                                                         xuint_t            group_id);
static xdbus_menu_model_t *         g_dbus_menu_model_get_from_group        (GDBusMenuGroup  *group,
                                                                         xuint_t            menu_id);

/* PathIdentifier {{{1 */
typedef struct
{
  xmain_context_t *context;
  xdbus_connection_t *connection;
  xchar_t *bus_name;
  xchar_t *object_path;
} PathIdentifier;

typedef const struct
{
  xmain_context_t *context;
  xdbus_connection_t *connection;
  const xchar_t *bus_name;
  const xchar_t *object_path;
} ConstPathIdentifier;

static xuint_t
path_identifier_hash (xconstpointer data)
{
  ConstPathIdentifier *id = data;

  return xstr_hash (id->object_path);
}

static xboolean_t
path_identifier_equal (xconstpointer a,
                       xconstpointer b)
{
  ConstPathIdentifier *id_a = a;
  ConstPathIdentifier *id_b = b;

  return id_a->connection == id_b->connection &&
         xstrcmp0 (id_a->bus_name, id_b->bus_name) == 0 &&
         xstr_equal (id_a->object_path, id_b->object_path);
}

static void
path_identifier_free (PathIdentifier *id)
{
  xmain_context_unref (id->context);
  xobject_unref (id->connection);
  g_free (id->bus_name);
  g_free (id->object_path);

  g_slice_free (PathIdentifier, id);
}

static PathIdentifier *
path_identifier_new (ConstPathIdentifier *cid)
{
  PathIdentifier *id;

  id = g_slice_new (PathIdentifier);
  id->context = xmain_context_ref (cid->context);
  id->connection = xobject_ref (cid->connection);
  id->bus_name = xstrdup (cid->bus_name);
  id->object_path = xstrdup (cid->object_path);

  return id;
}

/* GDBusMenuPath {{{1 */

struct _GDBusMenuPath
{
  PathIdentifier *id;
  xint_t ref_count;

  xhashtable_t *groups;
  xint_t active;
  xuint_t watch_id;
};

static xhashtable_t *g_dbus_menu_paths;

static GDBusMenuPath *
g_dbus_menu_path_ref (GDBusMenuPath *path)
{
  path->ref_count++;

  return path;
}

static void
g_dbus_menu_path_unref (GDBusMenuPath *path)
{
  if (--path->ref_count == 0)
    {
      xhash_table_remove (g_dbus_menu_paths, path->id);
      xhash_table_unref (path->groups);
      path_identifier_free (path->id);

      g_slice_free (GDBusMenuPath, path);
    }
}

static void
g_dbus_menu_path_signal (xdbus_connection_t *connection,
                         const xchar_t     *sender_name,
                         const xchar_t     *object_path,
                         const xchar_t     *interface_name,
                         const xchar_t     *signal_name,
                         xvariant_t        *parameters,
                         xpointer_t         user_data)
{
  GDBusMenuPath *path = user_data;
  xvariant_iter_t *iter;
  xuint_t group_id;
  xuint_t menu_id;
  xuint_t position;
  xuint_t removes;
  xvariant_t *adds;

  if (!xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(a(uuuuaa{sv}))")))
    return;

  xvariant_get (parameters, "(a(uuuuaa{sv}))", &iter);
  while (xvariant_iter_loop (iter, "(uuuu@aa{sv})", &group_id, &menu_id, &position, &removes, &adds))
    {
      GDBusMenuGroup *group;

      group = xhash_table_lookup (path->groups, GINT_TO_POINTER (group_id));

      if (group != NULL)
        g_dbus_menu_group_changed (group, menu_id, position, removes, adds);
    }
  xvariant_iter_free (iter);
}

static void
g_dbus_menu_path_activate (GDBusMenuPath *path)
{
  if (path->active++ == 0)
    path->watch_id = g_dbus_connection_signal_subscribe (path->id->connection, path->id->bus_name,
                                                         "org.gtk.Menus", "Changed", path->id->object_path,
                                                         NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                                                         g_dbus_menu_path_signal, path, NULL);
}

static void
g_dbus_menu_path_deactivate (GDBusMenuPath *path)
{
  if (--path->active == 0)
    g_dbus_connection_signal_unsubscribe (path->id->connection, path->watch_id);
}

static GDBusMenuPath *
g_dbus_menu_path_get (xmain_context_t    *context,
                      xdbus_connection_t *connection,
                      const xchar_t     *bus_name,
                      const xchar_t     *object_path)
{
  ConstPathIdentifier cid = { context, connection, bus_name, object_path };
  GDBusMenuPath *path;

  if (g_dbus_menu_paths == NULL)
    g_dbus_menu_paths = xhash_table_new (path_identifier_hash, path_identifier_equal);

  path = xhash_table_lookup (g_dbus_menu_paths, &cid);

  if (path == NULL)
    {
      path = g_slice_new (GDBusMenuPath);
      path->id = path_identifier_new (&cid);
      path->groups = xhash_table_new (NULL, NULL);
      path->ref_count = 0;
      path->active = 0;

      xhash_table_insert (g_dbus_menu_paths, path->id, path);
    }

  return g_dbus_menu_path_ref (path);
}

/* GDBusMenuGroup, GDBusMenuModelItem {{{1 */
typedef enum
{
  GROUP_OFFLINE,
  GROUP_PENDING,
  GROUP_ONLINE
} GroupStatus;

struct _GDBusMenuGroup
{
  GDBusMenuPath *path;
  xuint_t id;

  xhashtable_t *proxies; /* uint -> unowned xdbus_menu_model_t */
  xhashtable_t *menus;   /* uint -> owned xsequence_t */
  xint_t ref_count;
  GroupStatus state;
  xint_t active;
};

typedef struct
{
  xhashtable_t *attributes;
  xhashtable_t *links;
} GDBusMenuModelItem;

static GDBusMenuGroup *
g_dbus_menu_group_ref (GDBusMenuGroup *group)
{
  group->ref_count++;

  return group;
}

static void
g_dbus_menu_group_unref (GDBusMenuGroup *group)
{
  if (--group->ref_count == 0)
    {
      g_assert (group->state == GROUP_OFFLINE);
      g_assert (group->active == 0);

      xhash_table_remove (group->path->groups, GINT_TO_POINTER (group->id));
      xhash_table_unref (group->proxies);
      xhash_table_unref (group->menus);

      g_dbus_menu_path_unref (group->path);

      g_slice_free (GDBusMenuGroup, group);
    }
}

static void
g_dbus_menu_model_item_free (xpointer_t data)
{
  GDBusMenuModelItem *item = data;

  xhash_table_unref (item->attributes);
  xhash_table_unref (item->links);

  g_slice_free (GDBusMenuModelItem, item);
}

static GDBusMenuModelItem *
g_dbus_menu_group_create_item (xvariant_t *description)
{
  GDBusMenuModelItem *item;
  xvariant_iter_t iter;
  const xchar_t *key;
  xvariant_t *value;

  item = g_slice_new (GDBusMenuModelItem);
  item->attributes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);
  item->links = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);

  xvariant_iter_init (&iter, description);
  while (xvariant_iter_loop (&iter, "{&sv}", &key, &value))
    if (key[0] == ':')
      /* key + 1 to skip the ':' */
      xhash_table_insert (item->links, xstrdup (key + 1), xvariant_ref (value));
    else
      xhash_table_insert (item->attributes, xstrdup (key), xvariant_ref (value));

  return item;
}

/*
 * GDBusMenuGroup can be in three states:
 *
 * OFFLINE: not subscribed to this group
 * PENDING: we made the call to subscribe to this group, but the result
 *          has not come back yet
 * ONLINE: we are fully subscribed
 *
 * We can get into some nasty situations where we make a call due to an
 * activation request but receive a deactivation request before the call
 * returns.  If another activation request occurs then we could risk
 * sending a Start request even though one is already in progress.  For
 * this reason, we have to carefully consider what to do in each of the
 * three states for each of the following situations:
 *
 *  - activation requested
 *  - deactivation requested
 *  - Start call finishes
 *
 * To simplify things a bit, we do not have a callback for the Stop
 * call.  We just send it and assume that it takes effect immediately.
 *
 * Activation requested:
 *   OFFLINE: make the Start call and transition to PENDING
 *   PENDING: do nothing -- call is already in progress.
 *   ONLINE: this should not be possible
 *
 * Deactivation requested:
 *   OFFLINE: this should not be possible
 *   PENDING: do nothing -- handle it when the Start call finishes
 *   ONLINE: send the Stop call and move to OFFLINE immediately
 *
 * Start call finishes:
 *   OFFLINE: this should not be possible
 *   PENDING:
 *     If we should be active (ie: active count > 0): move to ONLINE
 *     If not: send Stop call and move to OFFLINE immediately
 *   ONLINE: this should not be possible
 *
 * We have to take care with regards to signal subscriptions (ie:
 * activation of the GDBusMenuPath).  The signal subscription is always
 * established when transitioning from OFFLINE to PENDING and taken down
 * when transitioning to OFFLINE (from either PENDING or ONLINE).
 *
 * Since there are two places where we transition to OFFLINE, we split
 * that code out into a separate function.
 */
static void
g_dbus_menu_group_go_offline (GDBusMenuGroup *group)
{
  g_dbus_menu_path_deactivate (group->path);
  g_dbus_connection_call (group->path->id->connection,
                          group->path->id->bus_name,
                          group->path->id->object_path,
                          "org.gtk.Menus", "End",
                          xvariant_new_parsed ("([ %u ],)", group->id),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                          NULL, NULL, NULL);
  group->state = GROUP_OFFLINE;
}


static void
g_dbus_menu_group_start_ready (xobject_t      *source_object,
                               xasync_result_t *result,
                               xpointer_t      user_data)
{
  xdbus_connection_t *connection = G_DBUS_CONNECTION (source_object);
  GDBusMenuGroup *group = user_data;
  xvariant_t *reply;

  g_assert (group->state == GROUP_PENDING);

  reply = g_dbus_connection_call_finish (connection, result, NULL);

  if (group->active)
    {
      group->state = GROUP_ONLINE;

      /* If we receive no reply, just act like we got an empty reply. */
      if (reply)
        {
          xvariant_iter_t *iter;
          xvariant_t *items;
          xuint_t group_id;
          xuint_t menu_id;

          xvariant_get (reply, "(a(uuaa{sv}))", &iter);
          while (xvariant_iter_loop (iter, "(uu@aa{sv})", &group_id, &menu_id, &items))
            if (group_id == group->id)
              g_dbus_menu_group_changed (group, menu_id, 0, 0, items);
          xvariant_iter_free (iter);
        }
    }
  else
    g_dbus_menu_group_go_offline (group);

  if (reply)
    xvariant_unref (reply);

  g_dbus_menu_group_unref (group);
}

static void
g_dbus_menu_group_activate (GDBusMenuGroup *group)
{
  if (group->active++ == 0)
    {
      g_assert (group->state != GROUP_ONLINE);

      if (group->state == GROUP_OFFLINE)
        {
          g_dbus_menu_path_activate (group->path);

          g_dbus_connection_call (group->path->id->connection,
                                  group->path->id->bus_name,
                                  group->path->id->object_path,
                                  "org.gtk.Menus", "Start",
                                  xvariant_new_parsed ("([ %u ],)", group->id),
                                  G_VARIANT_TYPE ("(a(uuaa{sv}))"),
                                  G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                  g_dbus_menu_group_start_ready,
                                  g_dbus_menu_group_ref (group));
          group->state = GROUP_PENDING;
        }
    }
}

static void
g_dbus_menu_group_deactivate (GDBusMenuGroup *group)
{
  if (--group->active == 0)
    {
      g_assert (group->state != GROUP_OFFLINE);

      if (group->state == GROUP_ONLINE)
        {
          /* We are here because nobody is watching, so just free
           * everything and don't bother with the notifications.
           */
          xhash_table_remove_all (group->menus);

          g_dbus_menu_group_go_offline (group);
        }
    }
}

static void
g_dbus_menu_group_changed (GDBusMenuGroup *group,
                           xuint_t           menu_id,
                           xint_t            position,
                           xint_t            removed,
                           xvariant_t       *added)
{
  GSequenceIter *point;
  xvariant_iter_t iter;
  xdbus_menu_model_t *proxy;
  xsequence_t *items;
  xvariant_t *item;
  xint_t n_added;

  /* We could have signals coming to us when we're not active (due to
   * some other process having subscribed to this group) or when we're
   * pending.  In both of those cases, we want to ignore the signal
   * since we'll get our own information when we call "Start" for
   * ourselves.
   */
  if (group->state != GROUP_ONLINE)
    return;

  items = xhash_table_lookup (group->menus, GINT_TO_POINTER (menu_id));

  if (items == NULL)
    {
      items = g_sequence_new (g_dbus_menu_model_item_free);
      xhash_table_insert (group->menus, GINT_TO_POINTER (menu_id), items);
    }

  point = g_sequence_get_iter_at_pos (items, position + removed);

  g_return_if_fail (point != NULL);

  if (removed)
    {
      GSequenceIter *start;

      start = g_sequence_get_iter_at_pos (items, position);
      g_sequence_remove_range (start, point);
    }

  n_added = xvariant_iter_init (&iter, added);
  while (xvariant_iter_loop (&iter, "@a{sv}", &item))
    g_sequence_insert_before (point, g_dbus_menu_group_create_item (item));

  if (g_sequence_is_empty (items))
    {
      xhash_table_remove (group->menus, GINT_TO_POINTER (menu_id));
      items = NULL;
    }

  if ((proxy = xhash_table_lookup (group->proxies, GINT_TO_POINTER (menu_id))))
    g_dbus_menu_model_changed (proxy, items, position, removed, n_added);
}

static GDBusMenuGroup *
g_dbus_menu_group_get_from_path (GDBusMenuPath *path,
                                 xuint_t          group_id)
{
  GDBusMenuGroup *group;

  group = xhash_table_lookup (path->groups, GINT_TO_POINTER (group_id));

  if (group == NULL)
    {
      group = g_slice_new (GDBusMenuGroup);
      group->path = g_dbus_menu_path_ref (path);
      group->id = group_id;
      group->proxies = xhash_table_new (NULL, NULL);
      group->menus = xhash_table_new_full (NULL, NULL, NULL, (xdestroy_notify_t) g_sequence_free);
      group->state = GROUP_OFFLINE;
      group->active = 0;
      group->ref_count = 0;

      xhash_table_insert (path->groups, GINT_TO_POINTER (group->id), group);
    }

  return g_dbus_menu_group_ref (group);
}

static GDBusMenuGroup *
g_dbus_menu_group_get (xmain_context_t    *context,
                       xdbus_connection_t *connection,
                       const xchar_t     *bus_name,
                       const xchar_t     *object_path,
                       xuint_t            group_id)
{
  GDBusMenuGroup *group;
  GDBusMenuPath *path;

  path = g_dbus_menu_path_get (context, connection, bus_name, object_path);
  group = g_dbus_menu_group_get_from_path (path, group_id);
  g_dbus_menu_path_unref (path);

  return group;
}

/* xdbus_menu_model_t {{{1 */

typedef xmenu_model_class_t xdbus_menu_model_class_t;
struct _GDBusMenuModel
{
  xmenu_model_t parent;

  GDBusMenuGroup *group;
  xuint_t id;

  xsequence_t *items; /* unowned */
  xboolean_t active;
};

G_DEFINE_TYPE (xdbus_menu_model, xdbus_menu_model, XTYPE_MENU_MODEL)

static xboolean_t
g_dbus_menu_model_is_mutable (xmenu_model_t *model)
{
  return TRUE;
}

static xint_t
g_dbus_menu_model_get_n_items (xmenu_model_t *model)
{
  xdbus_menu_model_t *proxy = G_DBUS_MENU_MODEL (model);

  if (!proxy->active)
    {
      g_dbus_menu_group_activate (proxy->group);
      proxy->active = TRUE;
    }

  return proxy->items ? g_sequence_get_length (proxy->items) : 0;
}

static void
g_dbus_menu_model_get_item_attributes (xmenu_model_t  *model,
                                       xint_t         item_index,
                                       xhashtable_t **table)
{
  xdbus_menu_model_t *proxy = G_DBUS_MENU_MODEL (model);
  GDBusMenuModelItem *item;
  GSequenceIter *iter;

  g_return_if_fail (proxy->active);
  g_return_if_fail (proxy->items);

  iter = g_sequence_get_iter_at_pos (proxy->items, item_index);
  g_return_if_fail (iter);

  item = g_sequence_get (iter);
  g_return_if_fail (item);

  *table = xhash_table_ref (item->attributes);
}

static void
g_dbus_menu_model_get_item_links (xmenu_model_t  *model,
                                  xint_t         item_index,
                                  xhashtable_t **table)
{
  xdbus_menu_model_t *proxy = G_DBUS_MENU_MODEL (model);
  GDBusMenuModelItem *item;
  GSequenceIter *iter;

  g_return_if_fail (proxy->active);
  g_return_if_fail (proxy->items);

  iter = g_sequence_get_iter_at_pos (proxy->items, item_index);
  g_return_if_fail (iter);

  item = g_sequence_get (iter);
  g_return_if_fail (item);

  *table = xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);

  {
    xhash_table_iter_t tmp;
    xpointer_t key;
    xpointer_t value;

    xhash_table_iter_init (&tmp, item->links);
    while (xhash_table_iter_next (&tmp, &key, &value))
      {
        if (xvariant_is_of_type (value, G_VARIANT_TYPE ("(uu)")))
          {
            xuint_t group_id, menu_id;
            GDBusMenuGroup *group;
            xdbus_menu_model_t *link;

            xvariant_get (value, "(uu)", &group_id, &menu_id);

            /* save the hash lookup in a relatively common case */
            if (proxy->group->id != group_id)
              group = g_dbus_menu_group_get_from_path (proxy->group->path, group_id);
            else
              group = g_dbus_menu_group_ref (proxy->group);

            link = g_dbus_menu_model_get_from_group (group, menu_id);

            xhash_table_insert (*table, xstrdup (key), link);

            g_dbus_menu_group_unref (group);
          }
      }
  }
}

static void
g_dbus_menu_model_finalize (xobject_t *object)
{
  xdbus_menu_model_t *proxy = G_DBUS_MENU_MODEL (object);

  if (proxy->active)
    g_dbus_menu_group_deactivate (proxy->group);

  xhash_table_remove (proxy->group->proxies, GINT_TO_POINTER (proxy->id));
  g_dbus_menu_group_unref (proxy->group);

  G_OBJECT_CLASS (g_dbus_menu_model_parent_class)
    ->finalize (object);
}

static void
g_dbus_menu_model_init (xdbus_menu_model_t *proxy)
{
}

static void
g_dbus_menu_model_class_init (xdbus_menu_model_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  class->is_mutable = g_dbus_menu_model_is_mutable;
  class->get_n_items = g_dbus_menu_model_get_n_items;
  class->get_item_attributes = g_dbus_menu_model_get_item_attributes;
  class->get_item_links = g_dbus_menu_model_get_item_links;

  object_class->finalize = g_dbus_menu_model_finalize;
}

static void
g_dbus_menu_model_changed (xdbus_menu_model_t *proxy,
                           xsequence_t      *items,
                           xint_t            position,
                           xint_t            removed,
                           xint_t            added)
{
  proxy->items = items;

  if (proxy->active && (removed || added))
    xmenu_model_items_changed (XMENU_MODEL (proxy), position, removed, added);
}

static xdbus_menu_model_t *
g_dbus_menu_model_get_from_group (GDBusMenuGroup *group,
                                  xuint_t           menu_id)
{
  xdbus_menu_model_t *proxy;

  proxy = xhash_table_lookup (group->proxies, GINT_TO_POINTER (menu_id));
  if (proxy)
    xobject_ref (proxy);

  if (proxy == NULL)
    {
      proxy = xobject_new (XTYPE_DBUS_MENU_MODEL, NULL);
      proxy->items = xhash_table_lookup (group->menus, GINT_TO_POINTER (menu_id));
      xhash_table_insert (group->proxies, GINT_TO_POINTER (menu_id), proxy);
      proxy->group = g_dbus_menu_group_ref (group);
      proxy->id = menu_id;
    }

  return proxy;
}

/**
 * g_dbus_menu_model_get:
 * @connection: a #xdbus_connection_t
 * @bus_name: (nullable): the bus name which exports the menu model
 *     or %NULL if @connection is not a message bus connection
 * @object_path: the object path at which the menu model is exported
 *
 * Obtains a #xdbus_menu_model_t for the menu model which is exported
 * at the given @bus_name and @object_path.
 *
 * The thread default main context is taken at the time of this call.
 * All signals on the menu model (and any linked models) are reported
 * with respect to this context.  All calls on the returned menu model
 * (and linked models) must also originate from this same context, with
 * the thread default main context unchanged.
 *
 * Returns: (transfer full): a #xdbus_menu_model_t object. Free with
 *     xobject_unref().
 *
 * Since: 2.32
 */
xdbus_menu_model_t *
g_dbus_menu_model_get (xdbus_connection_t *connection,
                       const xchar_t     *bus_name,
                       const xchar_t     *object_path)
{
  GDBusMenuGroup *group;
  xdbus_menu_model_t *proxy;
  xmain_context_t *context;

  g_return_val_if_fail (bus_name != NULL || g_dbus_connection_get_unique_name (connection) == NULL, NULL);

  context = xmain_context_get_thread_default ();
  if (context == NULL)
    context = xmain_context_default ();

  group = g_dbus_menu_group_get (context, connection, bus_name, object_path, 0);
  proxy = g_dbus_menu_model_get_from_group (group, 0);
  g_dbus_menu_group_unref (group);

  return proxy;
}

#if 0
static void
dump_proxy (xpointer_t key, xpointer_t value, xpointer_t data)
{
  xdbus_menu_model_t *proxy = value;

  g_print ("    menu %d refcount %d active %d\n",
           proxy->id, G_OBJECT (proxy)->ref_count, proxy->active);
}

static void
dump_group (xpointer_t key, xpointer_t value, xpointer_t data)
{
  GDBusMenuGroup *group = value;

  g_print ("  group %d refcount %d state %d active %d\n",
           group->id, group->ref_count, group->state, group->active);

  xhash_table_foreach (group->proxies, dump_proxy, NULL);
}

static void
dump_path (xpointer_t key, xpointer_t value, xpointer_t data)
{
  PathIdentifier *pid = key;
  GDBusMenuPath *path = value;

  g_print ("%s active %d\n", pid->object_path, path->active);
  xhash_table_foreach (path->groups, dump_group, NULL);
}

void
g_dbus_menu_model_dump (void)
{
  xhash_table_foreach (g_dbus_menu_paths, dump_path, NULL);
}

#endif

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
