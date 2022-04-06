/* GLib testing framework examples and tests
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

#include <gio/gio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "gdbus-tests.h"

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xmain_loop_t *loop;
  xboolean_t   timed_out;
} PropertyNotifyData;

static void
on_property_notify (xobject_t    *object,
                    xparam_spec_t *pspec,
                    xpointer_t    user_data)
{
  PropertyNotifyData *data = user_data;
  xmain_loop_quit (data->loop);
}

static xboolean_t
on_property_notify_timeout (xpointer_t user_data)
{
  PropertyNotifyData *data = user_data;
  data->timed_out = TRUE;
  xmain_loop_quit (data->loop);
  return G_SOURCE_CONTINUE;
}

xboolean_t
_g_assert_property_notify_run (xpointer_t     object,
                               const xchar_t *property_name)
{
  xchar_t *s;
  gulong handler_id;
  xuint_t timeout_id;
  PropertyNotifyData data;

  data.loop = xmain_loop_new (xmain_context_get_thread_default (), FALSE);
  data.timed_out = FALSE;
  s = xstrdup_printf ("notify::%s", property_name);
  handler_id = g_signal_connect (object,
                                 s,
                                 G_CALLBACK (on_property_notify),
                                 &data);
  g_free (s);
  timeout_id = g_timeout_add_seconds (30,
                                      on_property_notify_timeout,
                                      &data);
  xmain_loop_run (data.loop);
  g_signal_handler_disconnect (object, handler_id);
  xsource_remove (timeout_id);
  xmain_loop_unref (data.loop);

  return data.timed_out;
}

static xboolean_t
_give_up (xpointer_t data)
{
  xerror ("%s", (const xchar_t *) data);
  g_return_val_if_reached (G_SOURCE_CONTINUE);
}

typedef struct
{
  xmain_context_t *context;
  xboolean_t name_appeared;
  xboolean_t unwatch_complete;
} WatchData;

static void
name_appeared_cb (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  const xchar_t     *name_owner,
                  xpointer_t         user_data)
{
  WatchData *data = user_data;

  g_assert (name_owner != NULL);
  data->name_appeared = TRUE;
  xmain_context_wakeup (data->context);
}

static void
watch_free_cb (xpointer_t user_data)
{
  WatchData *data = user_data;

  data->unwatch_complete = TRUE;
  xmain_context_wakeup (data->context);
}

void
ensure_gdbus_testserver_up (xdbus_connection_t *connection,
                            xmain_context_t    *context)
{
  xsource_t *timeout_source = NULL;
  xuint_t watch_id;
  WatchData data = { context, FALSE, FALSE };

  xmain_context_push_thread_default (context);

  watch_id = g_bus_watch_name_on_connection (connection,
                                             "com.example.TestService",
                                             G_BUS_NAME_WATCHER_FLAGS_NONE,
                                             name_appeared_cb,
                                             NULL,
                                             &data,
                                             watch_free_cb);

  timeout_source = g_timeout_source_new_seconds (60);
  xsource_set_callback (timeout_source, _give_up,
                         "waited more than ~ 60s for gdbus-testserver to take its bus name",
                         NULL);
  xsource_attach (timeout_source, context);

  while (!data.name_appeared)
    xmain_context_iteration (context, TRUE);

  g_bus_unwatch_name (watch_id);
  watch_id = 0;

  while (!data.unwatch_complete)
    xmain_context_iteration (context, TRUE);

  xsource_destroy (timeout_source);
  xsource_unref (timeout_source);

  xmain_context_pop_thread_default (context);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xmain_loop_t *loop;
  xboolean_t   timed_out;
} SignalReceivedData;

static void
on_signal_received (xpointer_t user_data)
{
  SignalReceivedData *data = user_data;
  xmain_loop_quit (data->loop);
}

static xboolean_t
on_signal_received_timeout (xpointer_t user_data)
{
  SignalReceivedData *data = user_data;
  data->timed_out = TRUE;
  xmain_loop_quit (data->loop);
  return G_SOURCE_CONTINUE;
}

xboolean_t
_g_assert_signal_received_run (xpointer_t     object,
                               const xchar_t *signal_name)
{
  gulong handler_id;
  xuint_t timeout_id;
  SignalReceivedData data;

  data.loop = xmain_loop_new (xmain_context_get_thread_default (), FALSE);
  data.timed_out = FALSE;
  handler_id = g_signal_connect_swapped (object,
                                         signal_name,
                                         G_CALLBACK (on_signal_received),
                                         &data);
  timeout_id = g_timeout_add_seconds (30,
                                      on_signal_received_timeout,
                                      &data);
  xmain_loop_run (data.loop);
  g_signal_handler_disconnect (object, handler_id);
  xsource_remove (timeout_id);
  xmain_loop_unref (data.loop);

  return data.timed_out;
}

/* ---------------------------------------------------------------------------------------------------- */

xdbus_connection_t *
_g_bus_get_priv (GBusType            bus_type,
                 xcancellable_t       *cancellable,
                 xerror_t            **error)
{
  xchar_t *address;
  xdbus_connection_t *ret;

  ret = NULL;

  address = g_dbus_address_get_for_bus_sync (bus_type, cancellable, error);
  if (address == NULL)
    goto out;

  ret = g_dbus_connection_new_for_address_sync (address,
                                                G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                NULL, /* xdbus_auth_observer_t */
                                                cancellable,
                                                error);
  g_free (address);

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */
