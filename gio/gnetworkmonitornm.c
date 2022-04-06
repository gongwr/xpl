/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2014 Red Hat, Inc.
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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "gnetworkmonitornm.h"
#include "gioerror.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gnetworkingprivate.h"
#include "gnetworkmonitor.h"
#include "gdbusproxy.h"

static void g_network_monitor_nm_iface_init (GNetworkMonitorInterface *iface);
static void g_network_monitor_nm_initable_iface_init (xinitable_iface_t *iface);

enum
{
  PROP_0,

  PROP_NETWORK_AVAILABLE,
  PROP_NETWORK_METERED,
  PROP_CONNECTIVITY
};

typedef enum {
  NM_CONNECTIVITY_UNKNOWN,
  NM_CONNECTIVITY_NONE,
  NM_CONNECTIVITY_PORTAL,
  NM_CONNECTIVITY_LIMITED,
  NM_CONNECTIVITY_FULL
} NMConnectivityState;

/* Copied from https://developer.gnome.org/libnm-util/stable/libnm-util-NetworkManager.html#NMState;
 * used inline to avoid a NetworkManager dependency from GLib. */
typedef enum {
  NM_STATE_UNKNOWN          = 0,
  NM_STATE_ASLEEP           = 10,
  NM_STATE_DISCONNECTED     = 20,
  NM_STATE_DISCONNECTING    = 30,
  NM_STATE_CONNECTING       = 40,
  NM_STATE_CONNECTED_LOCAL  = 50,
  NM_STATE_CONNECTED_SITE   = 60,
  NM_STATE_CONNECTED_GLOBAL = 70,
} NMState;

struct _GNetworkMonitorNMPrivate
{
  xdbus_proxy_t *proxy;
  xuint_t signal_id;

  GNetworkConnectivity connectivity;
  xboolean_t network_available;
  xboolean_t network_metered;
};

#define g_network_monitor_nm_get_type _g_network_monitor_nm_get_type
G_DEFINE_TYPE_WITH_CODE (GNetworkMonitorNM, g_network_monitor_nm, XTYPE_NETWORK_MONITOR_NETLINK,
                         G_ADD_PRIVATE (GNetworkMonitorNM)
                         G_IMPLEMENT_INTERFACE (XTYPE_NETWORK_MONITOR,
                                                g_network_monitor_nm_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                g_network_monitor_nm_initable_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "networkmanager",
                                                         30))

static void
g_network_monitor_nm_init (GNetworkMonitorNM *nm)
{
  nm->priv = g_network_monitor_nm_get_instance_private (nm);
}

static void
g_network_monitor_nm_get_property (xobject_t    *object,
                                   xuint_t       prop_id,
                                   xvalue_t     *value,
                                   xparam_spec_t *pspec)
{
  GNetworkMonitorNM *nm = G_NETWORK_MONITOR_NM (object);

  switch (prop_id)
    {
    case PROP_NETWORK_AVAILABLE:
      xvalue_set_boolean (value, nm->priv->network_available);
      break;

    case PROP_NETWORK_METERED:
      xvalue_set_boolean (value, nm->priv->network_metered);
      break;

    case PROP_CONNECTIVITY:
      xvalue_set_enum (value, nm->priv->connectivity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static GNetworkConnectivity
nm_conn_to_g_conn (int nm_state)
{
  switch (nm_state)
    {
      case NM_CONNECTIVITY_UNKNOWN:
        return G_NETWORK_CONNECTIVITY_LOCAL;
      case NM_CONNECTIVITY_NONE:
        return G_NETWORK_CONNECTIVITY_LOCAL;
      case NM_CONNECTIVITY_PORTAL:
        return G_NETWORK_CONNECTIVITY_PORTAL;
      case NM_CONNECTIVITY_LIMITED:
        return G_NETWORK_CONNECTIVITY_LIMITED;
      case NM_CONNECTIVITY_FULL:
        return G_NETWORK_CONNECTIVITY_FULL;
      default:
        g_warning ("Unknown NM connectivity state %d", nm_state);
        return G_NETWORK_CONNECTIVITY_LOCAL;
    }
}

static xboolean_t
nm_metered_to_bool (xuint_t nm_metered)
{
  switch (nm_metered)
    {
      case 1: /* yes */
      case 3: /* guess-yes */
        return TRUE;
      case 0: /* unknown */
        /* We default to FALSE in the unknown-because-you're-not-running-NM
         * case, so we should return FALSE in the
         * unknown-when-you-are-running-NM case too. */
      case 2: /* no */
      case 4: /* guess-no */
        return FALSE;
      default:
        g_warning ("Unknown NM metered state %d", nm_metered);
        return FALSE;
    }
}

static void
sync_properties (GNetworkMonitorNM *nm,
                 xboolean_t           emit_signals)
{
  xvariant_t *v;
  NMState nm_state;
  NMConnectivityState nm_connectivity;
  xboolean_t new_network_available;
  xboolean_t new_network_metered;
  GNetworkConnectivity new_connectivity;

  v = g_dbus_proxy_get_cached_property (nm->priv->proxy, "State");
  if (!v)
    return;

  nm_state = xvariant_get_uint32 (v);
  xvariant_unref (v);

  v = g_dbus_proxy_get_cached_property (nm->priv->proxy, "Connectivity");
  if (!v)
    return;

  nm_connectivity = xvariant_get_uint32 (v);
  xvariant_unref (v);

  if (nm_state <= NM_STATE_CONNECTED_LOCAL)
    {
      new_network_available = FALSE;
      new_network_metered = FALSE;
      new_connectivity = G_NETWORK_CONNECTIVITY_LOCAL;
    }
  else if (nm_state <= NM_STATE_CONNECTED_SITE)
    {
      new_network_available = TRUE;
      new_network_metered = FALSE;
      if (nm_connectivity == NM_CONNECTIVITY_PORTAL)
        {
          new_connectivity = G_NETWORK_CONNECTIVITY_PORTAL;
        }
      else
        {
          new_connectivity = G_NETWORK_CONNECTIVITY_LIMITED;
        }
    }
  else /* nm_state == NM_STATE_CONNECTED_FULL */
    {

      /* this is only available post NM 1.0 */
      v = g_dbus_proxy_get_cached_property (nm->priv->proxy, "Metered");
      if (v == NULL)
        {
          new_network_metered = FALSE;
        }
      else
        {
          new_network_metered = nm_metered_to_bool (xvariant_get_uint32 (v));
          xvariant_unref (v);
        }

      new_network_available = TRUE;
      new_connectivity = nm_conn_to_g_conn (nm_connectivity);
    }

  if (!emit_signals)
    {
      nm->priv->network_metered = new_network_metered;
      nm->priv->network_available = new_network_available;
      nm->priv->connectivity = new_connectivity;
      return;
    }

  if (new_network_available != nm->priv->network_available)
    {
      nm->priv->network_available = new_network_available;
      xobject_notify (G_OBJECT (nm), "network-available");
    }
  if (new_network_metered != nm->priv->network_metered)
    {
      nm->priv->network_metered = new_network_metered;
      xobject_notify (G_OBJECT (nm), "network-metered");
    }
  if (new_connectivity != nm->priv->connectivity)
    {
      nm->priv->connectivity = new_connectivity;
      xobject_notify (G_OBJECT (nm), "connectivity");
    }
}

static void
proxy_properties_changed_cb (xdbus_proxy_t        *proxy,
                             xvariant_t          *changed_properties,
                             xstrv_t              invalidated_properties,
                             GNetworkMonitorNM *nm)
{
  sync_properties (nm, TRUE);
}

static xboolean_t
has_property (xdbus_proxy_t *proxy,
              const char *property_name)
{
  char **props;
  xboolean_t prop_found = FALSE;

  props = g_dbus_proxy_get_cached_property_names (proxy);

  if (!props)
    return FALSE;

  prop_found = xstrv_contains ((const xchar_t * const *) props, property_name);
  xstrfreev (props);
  return prop_found;
}

static xboolean_t
g_network_monitor_nm_initable_init (xinitable_t     *initable,
                                    xcancellable_t  *cancellable,
                                    xerror_t       **error)
{
  GNetworkMonitorNM *nm = G_NETWORK_MONITOR_NM (initable);
  xdbus_proxy_t *proxy;
  xinitable_iface_t *parent_iface;
  xchar_t *name_owner = NULL;

  parent_iface = xtype_interface_peek_parent (G_NETWORK_MONITOR_NM_GET_INITABLE_IFACE (initable));
  if (!parent_iface->init (initable, cancellable, error))
    return FALSE;

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START | G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                                         NULL,
                                         "org.freedesktop.NetworkManager",
                                         "/org/freedesktop/NetworkManager",
                                         "org.freedesktop.NetworkManager",
                                         cancellable,
                                         error);
  if (!proxy)
    return FALSE;

  name_owner = g_dbus_proxy_get_name_owner (proxy);

  if (!name_owner)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   _("NetworkManager not running"));
      xobject_unref (proxy);
      return FALSE;
    }

  g_free (name_owner);

  /* Verify it has the PrimaryConnection and Connectivity properties */
  if (!has_property (proxy, "Connectivity"))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   _("NetworkManager version too old"));
      xobject_unref (proxy);
      return FALSE;
    }

  nm->priv->signal_id = g_signal_connect (G_OBJECT (proxy), "g-properties-changed",
                                          G_CALLBACK (proxy_properties_changed_cb), nm);
  nm->priv->proxy = proxy;
  sync_properties (nm, FALSE);

  return TRUE;
}

static void
g_network_monitor_nm_finalize (xobject_t *object)
{
  GNetworkMonitorNM *nm = G_NETWORK_MONITOR_NM (object);

  if (nm->priv->proxy != NULL &&
      nm->priv->signal_id != 0)
    {
      g_signal_handler_disconnect (nm->priv->proxy,
                                   nm->priv->signal_id);
      nm->priv->signal_id = 0;
    }
  g_clear_object (&nm->priv->proxy);

  G_OBJECT_CLASS (g_network_monitor_nm_parent_class)->finalize (object);
}

static void
g_network_monitor_nm_class_init (GNetworkMonitorNMClass *nl_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (nl_class);

  gobject_class->finalize = g_network_monitor_nm_finalize;
  gobject_class->get_property = g_network_monitor_nm_get_property;

  xobject_class_override_property (gobject_class, PROP_NETWORK_AVAILABLE, "network-available");
  xobject_class_override_property (gobject_class, PROP_NETWORK_METERED, "network-metered");
  xobject_class_override_property (gobject_class, PROP_CONNECTIVITY, "connectivity");
}

static void
g_network_monitor_nm_iface_init (GNetworkMonitorInterface *monitor_iface)
{
}

static void
g_network_monitor_nm_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = g_network_monitor_nm_initable_init;
}
