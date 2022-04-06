/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2014-2018 Jan-Michael Brummer <jan.brummer@tabos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#include "gwin32networkmonitor.h"
#include "ginetaddress.h"
#include "ginetaddressmask.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gnetworkingprivate.h"
#include "gsocket.h"
#include "gnetworkmonitor.h"
#include "gioerror.h"

static xinitable_iface_t *initable_parent_iface;
static void g_win32_network_monitor_iface_init (GNetworkMonitorInterface *iface);
static void g_win32_network_monitor_initable_iface_init (xinitable_iface_t *iface);

struct _GWin32NetworkMonitorPrivate
{
  xboolean_t initialized;
  xerror_t *init_error;
  xmain_context_t *main_context;
  xsource_t *route_change_source;
  HANDLE handle;
};

#define g_win32_network_monitor_get_type _g_win32_network_monitor_get_type
G_DEFINE_TYPE_WITH_CODE (GWin32NetworkMonitor, g_win32_network_monitor, XTYPE_NETWORK_MONITOR_BASE,
                         G_ADD_PRIVATE (GWin32NetworkMonitor)
                         G_IMPLEMENT_INTERFACE (XTYPE_NETWORK_MONITOR,
                                                g_win32_network_monitor_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                g_win32_network_monitor_initable_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "win32",
                                                         20))

static void
g_win32_network_monitor_init (GWin32NetworkMonitor *win)
{
  win->priv = g_win32_network_monitor_get_instance_private (win);
}

static xboolean_t
win_network_monitor_get_ip_info (IP_ADDRESS_PREFIX  prefix,
                                 xsocket_family_t     *family,
                                 const xuint8_t     **dest,
                                 xsize_t             *len)
{
  switch (prefix.Prefix.si_family)
    {
      case AF_UNSPEC:
        /* Fall-through: AF_UNSPEC deliveres both IPV4 and IPV6 infos, let`s stick with IPV4 here */
      case AF_INET:
        *family = XSOCKET_FAMILY_IPV4;
        *dest = (xuint8_t *) &prefix.Prefix.Ipv4.sin_addr;
        *len = prefix.PrefixLength;
        break;
      case AF_INET6:
        *family = XSOCKET_FAMILY_IPV6;
        *dest = (xuint8_t *) &prefix.Prefix.Ipv6.sin6_addr;
        *len = prefix.PrefixLength;
        break;
      default:
        return FALSE;
    }

  return TRUE;
}

static xinet_address_mask_t *
get_network_mask (xsocket_family_t  family,
                  const xuint8_t  *dest,
                  xsize_t          len)
{
  xinet_address_mask_t *network;
  xinet_address_t *dest_addr;

  if (dest != NULL)
    dest_addr = xinet_address_new_from_bytes (dest, family);
  else
    dest_addr = xinet_address_new_any (family);

  network = xinet_address_mask_new (dest_addr, len, NULL);
  xobject_unref (dest_addr);

  return network;
}

static xboolean_t
win_network_monitor_process_table (GWin32NetworkMonitor  *win,
                                   xerror_t                 **error)
{
  DWORD ret = 0;
  xptr_array_t *networks;
  xsize_t i;
  MIB_IPFORWARD_TABLE2 *routes = NULL;
  MIB_IPFORWARD_ROW2 *route;

  ret = GetIpForwardTable2 (AF_UNSPEC, &routes);
  if (ret != ERROR_SUCCESS)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "GetIpForwardTable2 () failed: %ld", ret);

      return FALSE;
    }

  networks = xptr_array_new_full (routes->NumEntries, xobject_unref);
  for (i = 0; i < routes->NumEntries; i++)
    {
      xinet_address_mask_t *network;
      const xuint8_t *dest;
      xsize_t len;
      xsocket_family_t family;

      route = routes->Table + i;

      if (!win_network_monitor_get_ip_info (route->DestinationPrefix, &family, &dest, &len))
        continue;

      network = get_network_mask (family, dest, len);
      if (network == NULL)
        continue;

      xptr_array_add (networks, network);
    }

  xnetwork_monitor_base_set_networks (G_NETWORK_MONITOR_BASE (win),
                                       (xinet_address_mask_t **) networks->pdata,
                                       networks->len);

  return TRUE;
}

static void
add_network (GWin32NetworkMonitor *win,
             xsocket_family_t         family,
             const xuint8_t         *dest,
             xsize_t                 dest_len)
{
  xinet_address_mask_t *network;

  network = get_network_mask (family, dest, dest_len);
  if (network != NULL)
    {
      xnetwork_monitor_base_add_network (G_NETWORK_MONITOR_BASE (win), network);
      xobject_unref (network);
    }
}

static void
remove_network (GWin32NetworkMonitor *win,
                xsocket_family_t         family,
                const xuint8_t         *dest,
                xsize_t                 dest_len)
{
  xinet_address_mask_t *network;

  network = get_network_mask (family, dest, dest_len);
  if (network != NULL)
    {
      xnetwork_monitor_base_remove_network (G_NETWORK_MONITOR_BASE (win), network);
      xobject_unref (network);
    }
}

typedef struct {
  PMIB_IPFORWARD_ROW2 route;
  MIB_NOTIFICATION_TYPE type;
  GWin32NetworkMonitor *win;
} RouteData;

static xboolean_t
win_network_monitor_invoke_route_changed (xpointer_t user_data)
{
  xsocket_family_t family;
  RouteData *route_data = user_data;
  const xuint8_t *dest;
  xsize_t len;

  switch (route_data->type)
    {
      case MibDeleteInstance:
        if (!win_network_monitor_get_ip_info (route_data->route->DestinationPrefix, &family, &dest, &len))
          break;

        remove_network (route_data->win, family, dest, len);
        break;
      case MibAddInstance:
        if (!win_network_monitor_get_ip_info (route_data->route->DestinationPrefix, &family, &dest, &len))
            break;

        add_network (route_data->win, family, dest, len);
        break;
      case MibInitialNotification:
      default:
        break;
    }

  return G_SOURCE_REMOVE;
}

static VOID WINAPI
win_network_monitor_route_changed_cb (PVOID                 context,
                                      PMIB_IPFORWARD_ROW2   route,
                                      MIB_NOTIFICATION_TYPE type)
{
  GWin32NetworkMonitor *win = context;
  RouteData *route_data;

  route_data = g_new0 (RouteData, 1);
  route_data->route = route;
  route_data->type = type;
  route_data->win = win;

  win->priv->route_change_source = g_idle_source_new ();
  xsource_set_priority (win->priv->route_change_source, G_PRIORITY_DEFAULT);
  xsource_set_callback (win->priv->route_change_source,
                         win_network_monitor_invoke_route_changed,
                         route_data,
                         g_free);

  xsource_attach (win->priv->route_change_source, win->priv->main_context);
}

static xboolean_t
g_win32_network_monitor_initable_init (xinitable_t     *initable,
                                       xcancellable_t  *cancellable,
                                       xerror_t       **error)
{
  GWin32NetworkMonitor *win = G_WIN32_NETWORK_MONITOR (initable);
  NTSTATUS status;
  xboolean_t read;

  if (!win->priv->initialized)
    {
      win->priv->main_context = xmain_context_ref_thread_default ();

      /* Read current IP routing table. */
      read = win_network_monitor_process_table (win, &win->priv->init_error);
      if (read)
        {
          /* Register for IPv4 and IPv6 route updates. */
          status = NotifyRouteChange2 (AF_UNSPEC, (PIPFORWARD_CHANGE_CALLBACK) win_network_monitor_route_changed_cb, win, FALSE, &win->priv->handle);
          if (status != NO_ERROR)
            g_set_error (&win->priv->init_error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "NotifyRouteChange2() error: %ld", status);
        }

      win->priv->initialized = TRUE;
    }

  /* Forward the results. */
  if (win->priv->init_error != NULL)
    {
      g_propagate_error (error, xerror_copy (win->priv->init_error));
      return FALSE;
    }

  return initable_parent_iface->init (initable, cancellable, error);
}

static void
g_win32_network_monitor_finalize (xobject_t *object)
{
  GWin32NetworkMonitor *win = G_WIN32_NETWORK_MONITOR (object);

  /* Cancel notification event */
  if (win->priv->handle)
    CancelMibChangeNotify2 (win->priv->handle);

  g_clear_error (&win->priv->init_error);

  if (win->priv->route_change_source != NULL)
    {
      xsource_destroy (win->priv->route_change_source);
      xsource_unref (win->priv->route_change_source);
    }

  xmain_context_unref (win->priv->main_context);

  G_OBJECT_CLASS (g_win32_network_monitor_parent_class)->finalize (object);
}

static void
g_win32_network_monitor_class_init (GWin32NetworkMonitorClass *win_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (win_class);

  gobject_class->finalize = g_win32_network_monitor_finalize;
}

static void
g_win32_network_monitor_iface_init (GNetworkMonitorInterface *monitor_iface)
{
}

static void
g_win32_network_monitor_initable_iface_init (xinitable_iface_t *iface)
{
  initable_parent_iface = xtype_interface_peek_parent (iface);

  iface->init = g_win32_network_monitor_initable_init;
}
