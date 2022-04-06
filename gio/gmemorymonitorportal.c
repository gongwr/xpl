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
#include "gmemorymonitorportal.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "xdp-dbus.h"
#include "gportalsupport.h"

#define G_MEMORY_MONITOR_PORTAL_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, xinitable_t))

static void xmemory_monitor_portal_iface_init (GMemoryMonitorInterface *iface);
static void xmemory_monitor_portal_initable_iface_init (xinitable_iface_t *iface);

struct _GMemoryMonitorPortal
{
  xobject_t parent_instance;

  xdbus_proxy_t *proxy;
  xulong_t signal_id;
};

G_DEFINE_TYPE_WITH_CODE (GMemoryMonitorPortal, xmemory_monitor_portal, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xmemory_monitor_portal_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_MEMORY_MONITOR,
                                                xmemory_monitor_portal_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_MEMORY_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "portal",
                                                         40))

static void
xmemory_monitor_portal_init (GMemoryMonitorPortal *portal)
{
}

static void
proxy_signal (xdbus_proxy_t            *proxy,
              const char            *sender,
              const char            *signal,
              xvariant_t              *parameters,
              GMemoryMonitorPortal *portal)
{
  xuint8_t level;

  if (strcmp (signal, "LowMemoryWarning") != 0)
    return;
  if (!parameters)
    return;

  xvariant_get (parameters, "(y)", &level);
  xsignal_emit_by_name (portal, "low-memory-warning", level);
}

static xboolean_t
xmemory_monitor_portal_initable_init (xinitable_t     *initable,
                                        xcancellable_t  *cancellable,
                                        xerror_t       **error)
{
  GMemoryMonitorPortal *portal = G_MEMORY_MONITOR_PORTAL (initable);
  xdbus_proxy_t *proxy;
  xchar_t *name_owner = NULL;

  if (!glib_should_use_portal ())
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Not using portals");
      return FALSE;
    }

  proxy = xdbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                         NULL,
                                         "org.freedesktop.portal.Desktop",
                                         "/org/freedesktop/portal/desktop",
                                         "org.freedesktop.portal.MemoryMonitor",
                                         cancellable,
                                         error);
  if (!proxy)
    return FALSE;

  name_owner = xdbus_proxy_get_name_owner (proxy);

  if (name_owner == NULL)
    {
      xobject_unref (proxy);
      g_set_error (error,
                   G_DBUS_ERROR,
                   G_DBUS_ERROR_NAME_HAS_NO_OWNER,
                   "Desktop portal not found");
      return FALSE;
    }

  g_free (name_owner);

  portal->signal_id = xsignal_connect (proxy, "g-signal",
                                        G_CALLBACK (proxy_signal), portal);

  portal->proxy = proxy;

  return TRUE;
}

static void
xmemory_monitor_portal_finalize (xobject_t *object)
{
  GMemoryMonitorPortal *portal = G_MEMORY_MONITOR_PORTAL (object);

  if (portal->proxy != NULL)
    g_clear_signal_handler (&portal->signal_id, portal->proxy);
  g_clear_object (&portal->proxy);

  G_OBJECT_CLASS (xmemory_monitor_portal_parent_class)->finalize (object);
}

static void
xmemory_monitor_portal_class_init (GMemoryMonitorPortalClass *nl_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (nl_class);

  gobject_class->finalize  = xmemory_monitor_portal_finalize;
}

static void
xmemory_monitor_portal_iface_init (GMemoryMonitorInterface *monitor_iface)
{
}

static void
xmemory_monitor_portal_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = xmemory_monitor_portal_initable_init;
}
