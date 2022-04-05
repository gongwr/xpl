/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2019 Red Hat, Inc.
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
 */

#include "config.h"

#include "gmemorymonitor.h"
#include "gmemorymonitordbus.h"
#include "gioerror.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gcancellable.h"
#include "gdbusproxy.h"
#include "gdbusnamewatching.h"

#define G_MEMORY_MONITOR_DBUS_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, GInitable))

static void g_memory_monitor_dbus_iface_init (GMemoryMonitorInterface *iface);
static void g_memory_monitor_dbus_initable_iface_init (GInitableIface *iface);

struct _GMemoryMonitorDBus
{
  xobject_t parent_instance;

  xuint_t watch_id;
  xcancellable_t *cancellable;
  GDBusProxy *proxy;
  gulong signal_id;
};

G_DEFINE_TYPE_WITH_CODE (GMemoryMonitorDBus, g_memory_monitor_dbus, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                g_memory_monitor_dbus_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_MEMORY_MONITOR,
                                                g_memory_monitor_dbus_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_MEMORY_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "dbus",
                                                         30))

static void
g_memory_monitor_dbus_init (GMemoryMonitorDBus *dbus)
{
}

static void
proxy_signal_cb (GDBusProxy         *proxy,
                 const xchar_t        *sender_name,
                 const xchar_t        *signal_name,
                 xvariant_t           *parameters,
                 GMemoryMonitorDBus *dbus)
{
  guint8 level;

  if (g_strcmp0 (signal_name, "LowMemoryWarning") != 0)
    return;
  if (parameters == NULL)
    return;

  g_variant_get (parameters, "(y)", &level);
  g_signal_emit_by_name (dbus, "low-memory-warning", level);
}

static void
lmm_proxy_cb (xobject_t      *source_object,
              xasync_result_t *res,
              xpointer_t      user_data)
{
  GMemoryMonitorDBus *dbus = user_data;
  GDBusProxy *proxy;
  xerror_t *error = NULL;

  proxy = g_dbus_proxy_new_finish (res, &error);
  if (!proxy)
    {
      g_debug ("Failed to create LowMemoryMonitor D-Bus proxy: %s",
               error->message);
      g_error_free (error);
      return;
    }

  dbus->signal_id = g_signal_connect (G_OBJECT (proxy), "g-signal",
                                      G_CALLBACK (proxy_signal_cb), dbus);
  dbus->proxy = proxy;

}

static void
lmm_appeared_cb (GDBusConnection *connection,
                 const xchar_t     *name,
                 const xchar_t     *name_owner,
                 xpointer_t         user_data)
{
  GMemoryMonitorDBus *dbus = user_data;

  g_dbus_proxy_new (connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                    NULL,
                    "org.freedesktop.LowMemoryMonitor",
                    "/org/freedesktop/LowMemoryMonitor",
                    "org.freedesktop.LowMemoryMonitor",
                    dbus->cancellable,
                    lmm_proxy_cb,
                    dbus);
}

static void
lmm_vanished_cb (GDBusConnection *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  GMemoryMonitorDBus *dbus = user_data;

  g_clear_signal_handler (&dbus->signal_id, dbus->proxy);
  g_clear_object (&dbus->proxy);
}

static xboolean_t
g_memory_monitor_dbus_initable_init (GInitable     *initable,
                                     xcancellable_t  *cancellable,
                                     xerror_t       **error)
{
  GMemoryMonitorDBus *dbus = G_MEMORY_MONITOR_DBUS (initable);

  dbus->cancellable = g_cancellable_new ();
  dbus->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                     "org.freedesktop.LowMemoryMonitor",
                                     G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                     lmm_appeared_cb,
                                     lmm_vanished_cb,
                                     dbus,
                                     NULL);

  return TRUE;
}

static void
g_memory_monitor_dbus_finalize (xobject_t *object)
{
  GMemoryMonitorDBus *dbus = G_MEMORY_MONITOR_DBUS (object);

  g_cancellable_cancel (dbus->cancellable);
  g_clear_object (&dbus->cancellable);
  g_clear_signal_handler (&dbus->signal_id, dbus->proxy);
  g_clear_object (&dbus->proxy);
  g_clear_handle_id (&dbus->watch_id, g_bus_unwatch_name);

  G_OBJECT_CLASS (g_memory_monitor_dbus_parent_class)->finalize (object);
}

static void
g_memory_monitor_dbus_class_init (GMemoryMonitorDBusClass *nl_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (nl_class);

  gobject_class->finalize = g_memory_monitor_dbus_finalize;
}

static void
g_memory_monitor_dbus_iface_init (GMemoryMonitorInterface *monitor_iface)
{
}

static void
g_memory_monitor_dbus_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_memory_monitor_dbus_initable_init;
}
