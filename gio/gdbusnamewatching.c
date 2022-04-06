/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gdbusutils.h"
#include "gdbusnamewatching.h"
#include "gdbuserror.h"
#include "gdbusprivate.h"
#include "gdbusconnection.h"

#include "glibintl.h"

/**
 * SECTION:gdbusnamewatching
 * @title: Watching Bus Names
 * @short_description: Simple API for watching bus names
 * @include: gio/gio.h
 *
 * Convenience API for watching bus names.
 *
 * A simple example for watching a name can be found in
 * [gdbus-example-watch-name.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/gio/tests/gdbus-example-watch-name.c)
 */

G_LOCK_DEFINE_STATIC (lock);

/* ---------------------------------------------------------------------------------------------------- */

typedef enum
{
  PREVIOUS_CALL_NONE = 0,
  PREVIOUS_CALL_APPEARED,
  PREVIOUS_CALL_VANISHED,
} PreviousCall;

typedef struct
{
  xint_t                      ref_count;  /* (atomic) */
  xuint_t                     id;
  xchar_t                    *name;
  GBusNameWatcherFlags      flags;
  xchar_t                    *name_owner;
  GBusNameAppearedCallback  name_appeared_handler;
  GBusNameVanishedCallback  name_vanished_handler;
  xpointer_t                  user_data;
  xdestroy_notify_t            user_data_free_func;
  xmain_context_t             *main_context;

  xdbus_connection_t          *connection;
  gulong                    disconnected_signal_handler_id;
  xuint_t                     name_owner_changed_subscription_id;

  PreviousCall              previous_call;

  xboolean_t                  cancelled;
  xboolean_t                  initialized;
} Client;

/* Must be accessed atomically. */
static xuint_t next_global_id = 1;  /* (atomic) */

/* Must be accessed with @lock held. */
static xhashtable_t *map_id_to_client = NULL;

static Client *
client_ref (Client *client)
{
  g_atomic_int_inc (&client->ref_count);
  return client;
}

static xboolean_t
free_user_data_cb (xpointer_t user_data)
{
  /* The user data is actually freed by the xdestroy_notify_t for the idle source */
  return G_SOURCE_REMOVE;
}

static void
client_unref (Client *client)
{
  if (g_atomic_int_dec_and_test (&client->ref_count))
    {
      if (client->connection != NULL)
        {
          if (client->name_owner_changed_subscription_id > 0)
            g_dbus_connection_signal_unsubscribe (client->connection, client->name_owner_changed_subscription_id);
          if (client->disconnected_signal_handler_id > 0)
            g_signal_handler_disconnect (client->connection, client->disconnected_signal_handler_id);
          xobject_unref (client->connection);
        }
      g_free (client->name);
      g_free (client->name_owner);

      if (client->user_data_free_func != NULL)
        {
          /* Ensure client->user_data_free_func() is called from the right thread */
          if (client->main_context != xmain_context_get_thread_default ())
            {
              xsource_t *idle_source = g_idle_source_new ();
              xsource_set_callback (idle_source, free_user_data_cb,
                                     client->user_data,
                                     client->user_data_free_func);
              xsource_set_name (idle_source, "[gio, gdbusnamewatching.c] free_user_data_cb");
              xsource_attach (idle_source, client->main_context);
              xsource_unref (idle_source);
            }
          else
            client->user_data_free_func (client->user_data);
        }

      xmain_context_unref (client->main_context);

      g_free (client);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

typedef enum
{
  CALL_TYPE_NAME_APPEARED,
  CALL_TYPE_NAME_VANISHED
} CallType;

typedef struct
{
  Client *client;

  /* keep this separate because client->connection may
   * be set to NULL after scheduling the call
   */
  xdbus_connection_t *connection;

  /* ditto */
  xchar_t *name_owner;

  CallType call_type;
} CallHandlerData;

static void
call_handler_data_free (CallHandlerData *data)
{
  if (data->connection != NULL)
    xobject_unref (data->connection);
  g_free (data->name_owner);
  client_unref (data->client);
  g_free (data);
}

static void
actually_do_call (Client *client, xdbus_connection_t *connection, const xchar_t *name_owner, CallType call_type)
{
  /* The client might have been cancelled (g_bus_unwatch_name()) while we were
   * sitting in the #xmain_context_t dispatch queue. */
  if (client->cancelled)
    return;

  switch (call_type)
    {
    case CALL_TYPE_NAME_APPEARED:
      if (client->name_appeared_handler != NULL)
        {
          client->name_appeared_handler (connection,
                                         client->name,
                                         name_owner,
                                         client->user_data);
        }
      break;

    case CALL_TYPE_NAME_VANISHED:
      if (client->name_vanished_handler != NULL)
        {
          client->name_vanished_handler (connection,
                                         client->name,
                                         client->user_data);
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static xboolean_t
call_in_idle_cb (xpointer_t _data)
{
  CallHandlerData *data = _data;
  actually_do_call (data->client, data->connection, data->name_owner, data->call_type);
  return FALSE;
}

static void
schedule_call_in_idle (Client *client, CallType call_type)
{
  CallHandlerData *data;
  xsource_t *idle_source;

  data = g_new0 (CallHandlerData, 1);
  data->client = client_ref (client);
  data->connection = client->connection != NULL ? xobject_ref (client->connection) : NULL;
  data->name_owner = xstrdup (client->name_owner);
  data->call_type = call_type;

  idle_source = g_idle_source_new ();
  xsource_set_priority (idle_source, G_PRIORITY_HIGH);
  xsource_set_callback (idle_source,
                         call_in_idle_cb,
                         data,
                         (xdestroy_notify_t) call_handler_data_free);
  xsource_set_static_name (idle_source, "[gio, gdbusnamewatching.c] call_in_idle_cb");
  xsource_attach (idle_source, client->main_context);
  xsource_unref (idle_source);
}

static void
do_call (Client *client, CallType call_type)
{
  xmain_context_t *current_context;

  /* only schedule in idle if we're not in the right thread */
  current_context = xmain_context_ref_thread_default ();
  if (current_context != client->main_context)
    schedule_call_in_idle (client, call_type);
  else
    actually_do_call (client, client->connection, client->name_owner, call_type);
  xmain_context_unref (current_context);
}

static void
call_appeared_handler (Client *client)
{
  if (client->previous_call != PREVIOUS_CALL_APPEARED)
    {
      client->previous_call = PREVIOUS_CALL_APPEARED;
      if (!client->cancelled && client->name_appeared_handler != NULL)
        {
          do_call (client, CALL_TYPE_NAME_APPEARED);
        }
    }
}

static void
call_vanished_handler (Client *client)
{
  if (client->previous_call != PREVIOUS_CALL_VANISHED)
    {
      client->previous_call = PREVIOUS_CALL_VANISHED;
      if (!client->cancelled && client->name_vanished_handler != NULL)
        {
          do_call (client, CALL_TYPE_NAME_VANISHED);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------- */

/* Return a reference to the #Client for @watcher_id, or %NULL if it’s been
 * unwatched. This is safe to call from any thread. */
static Client *
dup_client (xuint_t watcher_id)
{
  Client *client;

  G_LOCK (lock);

  g_assert (watcher_id != 0);
  g_assert (map_id_to_client != NULL);

  client = xhash_table_lookup (map_id_to_client, GUINT_TO_POINTER (watcher_id));

  if (client != NULL)
    client_ref (client);

  G_UNLOCK (lock);

  return client;
}

/* Could be called from any thread, so it could be called after client_unref()
 * has started finalising the #Client. Avoid that by looking up the #Client
 * atomically. */
static void
on_connection_disconnected (xdbus_connection_t *connection,
                            xboolean_t         remote_peer_vanished,
                            xerror_t          *error,
                            xpointer_t         user_data)
{
  xuint_t watcher_id = GPOINTER_TO_UINT (user_data);
  Client *client = NULL;

  client = dup_client (watcher_id);
  if (client == NULL)
    return;

  if (client->name_owner_changed_subscription_id > 0)
    g_dbus_connection_signal_unsubscribe (client->connection, client->name_owner_changed_subscription_id);
  if (client->disconnected_signal_handler_id > 0)
    g_signal_handler_disconnect (client->connection, client->disconnected_signal_handler_id);
  xobject_unref (client->connection);
  client->disconnected_signal_handler_id = 0;
  client->name_owner_changed_subscription_id = 0;
  client->connection = NULL;

  call_vanished_handler (client);

  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

/* Will always be called from the thread which acquired client->main_context. */
static void
on_name_owner_changed (xdbus_connection_t *connection,
                       const xchar_t      *sender_name,
                       const xchar_t      *object_path,
                       const xchar_t      *interface_name,
                       const xchar_t      *signal_name,
                       xvariant_t         *parameters,
                       xpointer_t          user_data)
{
  xuint_t watcher_id = GPOINTER_TO_UINT (user_data);
  Client *client = NULL;
  const xchar_t *name;
  const xchar_t *old_owner;
  const xchar_t *new_owner;

  client = dup_client (watcher_id);
  if (client == NULL)
    return;

  if (!client->initialized)
    goto out;

  if (xstrcmp0 (object_path, "/org/freedesktop/DBus") != 0 ||
      xstrcmp0 (interface_name, "org.freedesktop.DBus") != 0 ||
      xstrcmp0 (sender_name, "org.freedesktop.DBus") != 0)
    goto out;

  xvariant_get (parameters,
                 "(&s&s&s)",
                 &name,
                 &old_owner,
                 &new_owner);

  /* we only care about a specific name */
  if (xstrcmp0 (name, client->name) != 0)
    goto out;

  if ((old_owner != NULL && strlen (old_owner) > 0) && client->name_owner != NULL)
    {
      g_free (client->name_owner);
      client->name_owner = NULL;
      call_vanished_handler (client);
    }

  if (new_owner != NULL && strlen (new_owner) > 0)
    {
      g_warn_if_fail (client->name_owner == NULL);
      g_free (client->name_owner);
      client->name_owner = xstrdup (new_owner);
      call_appeared_handler (client);
    }

 out:
  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
get_name_owner_cb (xobject_t      *source_object,
                   xasync_result_t *res,
                   xpointer_t      user_data)
{
  Client *client = user_data;
  xvariant_t *result;
  const char *name_owner;

  name_owner = NULL;
  result = NULL;

  result = g_dbus_connection_call_finish (client->connection,
                                          res,
                                          NULL);
  if (result != NULL)
    {
      xvariant_get (result, "(&s)", &name_owner);
    }

  if (name_owner != NULL)
    {
      g_warn_if_fail (client->name_owner == NULL);
      client->name_owner = xstrdup (name_owner);
      call_appeared_handler (client);
    }
  else
    {
      call_vanished_handler (client);
    }

  client->initialized = TRUE;

  if (result != NULL)
    xvariant_unref (result);
  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
invoke_get_name_owner (Client *client)
{
  g_dbus_connection_call (client->connection,
                          "org.freedesktop.DBus",  /* bus name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "GetNameOwner",          /* method name */
                          xvariant_new ("(s)", client->name),
                          G_VARIANT_TYPE ("(s)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (xasync_ready_callback_t) get_name_owner_cb,
                          client_ref (client));
}

/* ---------------------------------------------------------------------------------------------------- */

static void
start_service_by_name_cb (xobject_t      *source_object,
                          xasync_result_t *res,
                          xpointer_t      user_data)
{
  Client *client = user_data;
  xvariant_t *result;

  result = NULL;

  result = g_dbus_connection_call_finish (client->connection,
                                          res,
                                          NULL);
  if (result != NULL)
    {
      xuint32_t start_service_result;
      xvariant_get (result, "(u)", &start_service_result);

      if (start_service_result == 1) /* DBUS_START_REPLY_SUCCESS */
        {
          invoke_get_name_owner (client);
        }
      else if (start_service_result == 2) /* DBUS_START_REPLY_ALREADY_RUNNING */
        {
          invoke_get_name_owner (client);
        }
      else
        {
          g_warning ("Unexpected reply %d from StartServiceByName() method", start_service_result);
          call_vanished_handler (client);
          client->initialized = TRUE;
        }
    }
  else
    {
      /* Errors are not unexpected; the bus will reply e.g.
       *
       *   org.freedesktop.DBus.Error.ServiceUnknown: The name org.gnome.Epiphany2
       *   was not provided by any .service files
       *
       * This doesn't mean that the name doesn't have an owner, just
       * that it's not provided by a .service file. So proceed to
       * invoke GetNameOwner().
       */
      invoke_get_name_owner (client);
    }

  if (result != NULL)
    xvariant_unref (result);
  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
has_connection (Client *client)
{
  /* listen for disconnection */
  client->disconnected_signal_handler_id = g_signal_connect (client->connection,
                                                             "closed",
                                                             G_CALLBACK (on_connection_disconnected),
                                                             GUINT_TO_POINTER (client->id));

  /* start listening to NameOwnerChanged messages immediately */
  client->name_owner_changed_subscription_id = g_dbus_connection_signal_subscribe (client->connection,
                                                                                   "org.freedesktop.DBus",  /* name */
                                                                                   "org.freedesktop.DBus",  /* if */
                                                                                   "NameOwnerChanged",      /* signal */
                                                                                   "/org/freedesktop/DBus", /* path */
                                                                                   client->name,
                                                                                   G_DBUS_SIGNAL_FLAGS_NONE,
                                                                                   on_name_owner_changed,
                                                                                   GUINT_TO_POINTER (client->id),
                                                                                   NULL);

  if (client->flags & G_BUS_NAME_WATCHER_FLAGS_AUTO_START)
    {
      g_dbus_connection_call (client->connection,
                              "org.freedesktop.DBus",  /* bus name */
                              "/org/freedesktop/DBus", /* object path */
                              "org.freedesktop.DBus",  /* interface name */
                              "StartServiceByName",    /* method name */
                              xvariant_new ("(su)", client->name, 0),
                              G_VARIANT_TYPE ("(u)"),
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,
                              NULL,
                              (xasync_ready_callback_t) start_service_by_name_cb,
                              client_ref (client));
    }
  else
    {
      /* check owner */
      invoke_get_name_owner (client);
    }
}


static void
connection_get_cb (xobject_t      *source_object,
                   xasync_result_t *res,
                   xpointer_t      user_data)
{
  Client *client = user_data;

  client->connection = g_bus_get_finish (res, NULL);
  if (client->connection == NULL)
    {
      call_vanished_handler (client);
      goto out;
    }

  has_connection (client);

 out:
  client_unref (client);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_bus_watch_name:
 * @bus_type: The type of bus to watch a name on.
 * @name: The name (well-known or unique) to watch.
 * @flags: Flags from the #GBusNameWatcherFlags enumeration.
 * @name_appeared_handler: (nullable): Handler to invoke when @name is known to exist or %NULL.
 * @name_vanished_handler: (nullable): Handler to invoke when @name is known to not exist or %NULL.
 * @user_data: User data to pass to handlers.
 * @user_data_free_func: (nullable): Function for freeing @user_data or %NULL.
 *
 * Starts watching @name on the bus specified by @bus_type and calls
 * @name_appeared_handler and @name_vanished_handler when the name is
 * known to have an owner respectively known to lose its
 * owner. Callbacks will be invoked in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread you are calling this function from.
 *
 * You are guaranteed that one of the handlers will be invoked after
 * calling this function. When you are done watching the name, just
 * call g_bus_unwatch_name() with the watcher id this function
 * returns.
 *
 * If the name vanishes or appears (for example the application owning
 * the name could restart), the handlers are also invoked. If the
 * #xdbus_connection_t that is used for watching the name disconnects, then
 * @name_vanished_handler is invoked since it is no longer
 * possible to access the name.
 *
 * Another guarantee is that invocations of @name_appeared_handler
 * and @name_vanished_handler are guaranteed to alternate; that
 * is, if @name_appeared_handler is invoked then you are
 * guaranteed that the next time one of the handlers is invoked, it
 * will be @name_vanished_handler. The reverse is also true.
 *
 * This behavior makes it very simple to write applications that want
 * to take action when a certain [name exists][gdbus-watching-names].
 * Basically, the application should create object proxies in
 * @name_appeared_handler and destroy them again (if any) in
 * @name_vanished_handler.
 *
 * Returns: An identifier (never 0) that can be used with
 * g_bus_unwatch_name() to stop watching the name.
 *
 * Since: 2.26
 */
xuint_t
g_bus_watch_name (GBusType                  bus_type,
                  const xchar_t              *name,
                  GBusNameWatcherFlags      flags,
                  GBusNameAppearedCallback  name_appeared_handler,
                  GBusNameVanishedCallback  name_vanished_handler,
                  xpointer_t                  user_data,
                  xdestroy_notify_t            user_data_free_func)
{
  Client *client;

  g_return_val_if_fail (g_dbus_is_name (name), 0);

  G_LOCK (lock);

  client = g_new0 (Client, 1);
  client->ref_count = 1;
  client->id = (xuint_t) g_atomic_int_add (&next_global_id, 1); /* TODO: uh oh, handle overflow */
  client->name = xstrdup (name);
  client->flags = flags;
  client->name_appeared_handler = name_appeared_handler;
  client->name_vanished_handler = name_vanished_handler;
  client->user_data = user_data;
  client->user_data_free_func = user_data_free_func;
  client->main_context = xmain_context_ref_thread_default ();

  if (map_id_to_client == NULL)
    {
      map_id_to_client = xhash_table_new (g_direct_hash, g_direct_equal);
    }
  xhash_table_insert (map_id_to_client,
                       GUINT_TO_POINTER (client->id),
                       client);

  g_bus_get (bus_type,
             NULL,
             connection_get_cb,
             client_ref (client));

  G_UNLOCK (lock);

  return client->id;
}

/**
 * g_bus_watch_name_on_connection:
 * @connection: A #xdbus_connection_t.
 * @name: The name (well-known or unique) to watch.
 * @flags: Flags from the #GBusNameWatcherFlags enumeration.
 * @name_appeared_handler: (nullable): Handler to invoke when @name is known to exist or %NULL.
 * @name_vanished_handler: (nullable): Handler to invoke when @name is known to not exist or %NULL.
 * @user_data: User data to pass to handlers.
 * @user_data_free_func: (nullable): Function for freeing @user_data or %NULL.
 *
 * Like g_bus_watch_name() but takes a #xdbus_connection_t instead of a
 * #GBusType.
 *
 * Returns: An identifier (never 0) that can be used with
 * g_bus_unwatch_name() to stop watching the name.
 *
 * Since: 2.26
 */
xuint_t g_bus_watch_name_on_connection (xdbus_connection_t          *connection,
                                      const xchar_t              *name,
                                      GBusNameWatcherFlags      flags,
                                      GBusNameAppearedCallback  name_appeared_handler,
                                      GBusNameVanishedCallback  name_vanished_handler,
                                      xpointer_t                  user_data,
                                      xdestroy_notify_t            user_data_free_func)
{
  Client *client;

  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), 0);
  g_return_val_if_fail (g_dbus_is_name (name), 0);

  G_LOCK (lock);

  client = g_new0 (Client, 1);
  client->ref_count = 1;
  client->id = (xuint_t) g_atomic_int_add (&next_global_id, 1); /* TODO: uh oh, handle overflow */
  client->name = xstrdup (name);
  client->flags = flags;
  client->name_appeared_handler = name_appeared_handler;
  client->name_vanished_handler = name_vanished_handler;
  client->user_data = user_data;
  client->user_data_free_func = user_data_free_func;
  client->main_context = xmain_context_ref_thread_default ();

  if (map_id_to_client == NULL)
    map_id_to_client = xhash_table_new (g_direct_hash, g_direct_equal);

  xhash_table_insert (map_id_to_client,
                       GUINT_TO_POINTER (client->id),
                       client);

  client->connection = xobject_ref (connection);
  G_UNLOCK (lock);

  has_connection (client);

  return client->id;
}

typedef struct {
  xclosure_t *name_appeared_closure;
  xclosure_t *name_vanished_closure;
} WatchNameData;

static WatchNameData *
watch_name_data_new (xclosure_t *name_appeared_closure,
                     xclosure_t *name_vanished_closure)
{
  WatchNameData *data;

  data = g_new0 (WatchNameData, 1);

  if (name_appeared_closure != NULL)
    {
      data->name_appeared_closure = xclosure_ref (name_appeared_closure);
      xclosure_sink (name_appeared_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (name_appeared_closure))
        xclosure_set_marshal (name_appeared_closure, g_cclosure_marshal_generic);
    }

  if (name_vanished_closure != NULL)
    {
      data->name_vanished_closure = xclosure_ref (name_vanished_closure);
      xclosure_sink (name_vanished_closure);
      if (G_CLOSURE_NEEDS_MARSHAL (name_vanished_closure))
        xclosure_set_marshal (name_vanished_closure, g_cclosure_marshal_generic);
    }

  return data;
}

static void
watch_with_closures_on_name_appeared (xdbus_connection_t *connection,
                                      const xchar_t     *name,
                                      const xchar_t     *name_owner,
                                      xpointer_t         user_data)
{
  WatchNameData *data = user_data;
  xvalue_t params[3] = { G_VALUE_INIT, G_VALUE_INIT, G_VALUE_INIT };

  xvalue_init (&params[0], XTYPE_DBUS_CONNECTION);
  xvalue_set_object (&params[0], connection);

  xvalue_init (&params[1], XTYPE_STRING);
  xvalue_set_string (&params[1], name);

  xvalue_init (&params[2], XTYPE_STRING);
  xvalue_set_string (&params[2], name_owner);

  xclosure_invoke (data->name_appeared_closure, NULL, 3, params, NULL);

  xvalue_unset (params + 0);
  xvalue_unset (params + 1);
  xvalue_unset (params + 2);
}

static void
watch_with_closures_on_name_vanished (xdbus_connection_t *connection,
                                      const xchar_t     *name,
                                      xpointer_t         user_data)
{
  WatchNameData *data = user_data;
  xvalue_t params[2] = { G_VALUE_INIT, G_VALUE_INIT };

  xvalue_init (&params[0], XTYPE_DBUS_CONNECTION);
  xvalue_set_object (&params[0], connection);

  xvalue_init (&params[1], XTYPE_STRING);
  xvalue_set_string (&params[1], name);

  xclosure_invoke (data->name_vanished_closure, NULL, 2, params, NULL);

  xvalue_unset (params + 0);
  xvalue_unset (params + 1);
}

static void
bus_watch_name_free_func (xpointer_t user_data)
{
  WatchNameData *data = user_data;

  if (data->name_appeared_closure != NULL)
    xclosure_unref (data->name_appeared_closure);

  if (data->name_vanished_closure != NULL)
    xclosure_unref (data->name_vanished_closure);

  g_free (data);
}

/**
 * g_bus_watch_name_with_closures: (rename-to g_bus_watch_name)
 * @bus_type: The type of bus to watch a name on.
 * @name: The name (well-known or unique) to watch.
 * @flags: Flags from the #GBusNameWatcherFlags enumeration.
 * @name_appeared_closure: (nullable): #xclosure_t to invoke when @name is known
 * to exist or %NULL.
 * @name_vanished_closure: (nullable): #xclosure_t to invoke when @name is known
 * to not exist or %NULL.
 *
 * Version of g_bus_watch_name() using closures instead of callbacks for
 * easier binding in other languages.
 *
 * Returns: An identifier (never 0) that can be used with
 * g_bus_unwatch_name() to stop watching the name.
 *
 * Since: 2.26
 */
xuint_t
g_bus_watch_name_with_closures (GBusType                 bus_type,
                                const xchar_t             *name,
                                GBusNameWatcherFlags     flags,
                                xclosure_t                *name_appeared_closure,
                                xclosure_t                *name_vanished_closure)
{
  return g_bus_watch_name (bus_type,
          name,
          flags,
          name_appeared_closure != NULL ? watch_with_closures_on_name_appeared : NULL,
          name_vanished_closure != NULL ? watch_with_closures_on_name_vanished : NULL,
          watch_name_data_new (name_appeared_closure, name_vanished_closure),
          bus_watch_name_free_func);
}

/**
 * g_bus_watch_name_on_connection_with_closures: (rename-to g_bus_watch_name_on_connection)
 * @connection: A #xdbus_connection_t.
 * @name: The name (well-known or unique) to watch.
 * @flags: Flags from the #GBusNameWatcherFlags enumeration.
 * @name_appeared_closure: (nullable): #xclosure_t to invoke when @name is known
 * to exist or %NULL.
 * @name_vanished_closure: (nullable): #xclosure_t to invoke when @name is known
 * to not exist or %NULL.
 *
 * Version of g_bus_watch_name_on_connection() using closures instead of callbacks for
 * easier binding in other languages.
 *
 * Returns: An identifier (never 0) that can be used with
 * g_bus_unwatch_name() to stop watching the name.
 *
 * Since: 2.26
 */
xuint_t g_bus_watch_name_on_connection_with_closures (
                                      xdbus_connection_t          *connection,
                                      const xchar_t              *name,
                                      GBusNameWatcherFlags      flags,
                                      xclosure_t                 *name_appeared_closure,
                                      xclosure_t                 *name_vanished_closure)
{
  return g_bus_watch_name_on_connection (connection,
          name,
          flags,
          name_appeared_closure != NULL ? watch_with_closures_on_name_appeared : NULL,
          name_vanished_closure != NULL ? watch_with_closures_on_name_vanished : NULL,
          watch_name_data_new (name_appeared_closure, name_vanished_closure),
          bus_watch_name_free_func);
}

/**
 * g_bus_unwatch_name:
 * @watcher_id: An identifier obtained from g_bus_watch_name()
 *
 * Stops watching a name.
 *
 * Note that there may still be D-Bus traffic to process (relating to watching
 * and unwatching the name) in the current thread-default #xmain_context_t after
 * this function has returned. You should continue to iterate the #xmain_context_t
 * until the #xdestroy_notify_t function passed to g_bus_watch_name() is called, in
 * order to avoid memory leaks through callbacks queued on the #xmain_context_t
 * after it’s stopped being iterated.
 *
 * Since: 2.26
 */
void
g_bus_unwatch_name (xuint_t watcher_id)
{
  Client *client;

  g_return_if_fail (watcher_id > 0);

  client = NULL;

  G_LOCK (lock);
  if (watcher_id == 0 ||
      map_id_to_client == NULL ||
      (client = xhash_table_lookup (map_id_to_client, GUINT_TO_POINTER (watcher_id))) == NULL)
    {
      g_warning ("Invalid id %d passed to g_bus_unwatch_name()", watcher_id);
      goto out;
    }

  client->cancelled = TRUE;
  g_warn_if_fail (xhash_table_remove (map_id_to_client, GUINT_TO_POINTER (watcher_id)));

 out:
  G_UNLOCK (lock);

  /* do callback without holding lock */
  if (client != NULL)
    {
      client_unref (client);
    }
}
