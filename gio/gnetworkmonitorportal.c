/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2016 Red Hat, Inc.
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

#include "gnetworkmonitorportal.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "xdp-dbus.h"
#include "gportalsupport.h"

static xinitable_iface_t *initable_parent_iface;
static void xnetwork_monitor_portal_iface_init (GNetworkMonitorInterface *iface);
static void xnetwork_monitor_portal_initable_iface_init (xinitable_iface_t *iface);

enum
{
  PROP_0,
  PROP_NETWORK_AVAILABLE,
  PROP_NETWORK_METERED,
  PROP_CONNECTIVITY
};

struct _GNetworkMonitorPortalPrivate
{
  xdbus_proxy_t *proxy;
  xboolean_t has_network;

  xboolean_t available;
  xboolean_t metered;
  GNetworkConnectivity connectivity;
};

G_DEFINE_TYPE_WITH_CODE (GNetworkMonitorPortal, xnetwork_monitor_portal, XTYPE_NETWORK_MONITOR_BASE,
                         G_ADD_PRIVATE (GNetworkMonitorPortal)
                         G_IMPLEMENT_INTERFACE (XTYPE_NETWORK_MONITOR,
                                                xnetwork_monitor_portal_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xnetwork_monitor_portal_initable_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "portal",
                                                         40))

static void
xnetwork_monitor_portal_init (GNetworkMonitorPortal *nm)
{
  nm->priv = xnetwork_monitor_portal_get_instance_private (nm);
}

static void
xnetwork_monitor_portal_get_property (xobject_t    *object,
                                       xuint_t       prop_id,
                                       xvalue_t     *value,
                                       xparam_spec_t *pspec)
{
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (object);

  switch (prop_id)
    {
    case PROP_NETWORK_AVAILABLE:
      xvalue_set_boolean (value, nm->priv->available);
      break;

    case PROP_NETWORK_METERED:
      xvalue_set_boolean (value, nm->priv->metered);
      break;

    case PROP_CONNECTIVITY:
      xvalue_set_enum (value, nm->priv->connectivity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static xboolean_t
is_valid_connectivity (xuint32_t value)
{
  xenum_value_t *enum_value;
  xenum_class_t *enum_klass;

  enum_klass = xtype_class_ref (XTYPE_NETWORK_CONNECTIVITY);
  enum_value = xenum_get_value (enum_klass, value);

  xtype_class_unref (enum_klass);

  return enum_value != NULL;
}

static void
got_available (xobject_t *source,
               xasync_result_t *res,
               xpointer_t data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (source);
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (data);
  xerror_t *error = NULL;
  xvariant_t *ret;
  xboolean_t available;

  ret = xdbus_proxy_call_finish (proxy, res, &error);
  if (ret == NULL)
    {
      if (!xerror_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD))
        {
          g_warning ("%s", error->message);
          g_clear_error (&error);
          return;
        }

      g_clear_error (&error);

      /* Fall back to version 1 */
      ret = xdbus_proxy_get_cached_property (nm->priv->proxy, "available");
      if (ret == NULL)
        {
          g_warning ("Failed to get the '%s' property", "available");
          return;
        }

      available = xvariant_get_boolean (ret);
      xvariant_unref (ret);
    }
  else
    {
      xvariant_get (ret, "(b)", &available);
      xvariant_unref (ret);
    }

  if (nm->priv->available != available)
    {
      nm->priv->available = available;
      xobject_notify (G_OBJECT (nm), "network-available");
      xsignal_emit_by_name (nm, "network-changed", available);
    }
}

static void
got_metered (xobject_t *source,
             xasync_result_t *res,
             xpointer_t data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (source);
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (data);
  xerror_t *error = NULL;
  xvariant_t *ret;
  xboolean_t metered;

  ret = xdbus_proxy_call_finish (proxy, res, &error);
  if (ret == NULL)
    {
      if (!xerror_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD))
        {
          g_warning ("%s", error->message);
          g_clear_error (&error);
          return;
        }

      g_clear_error (&error);

      /* Fall back to version 1 */
      ret = xdbus_proxy_get_cached_property (nm->priv->proxy, "metered");
      if (ret == NULL)
        {
          g_warning ("Failed to get the '%s' property", "metered");
          return;
        }

      metered = xvariant_get_boolean (ret);
      xvariant_unref (ret);
    }
  else
    {
      xvariant_get (ret, "(b)", &metered);
      xvariant_unref (ret);
    }

  if (nm->priv->metered != metered)
    {
      nm->priv->metered = metered;
      xobject_notify (G_OBJECT (nm), "network-metered");
      xsignal_emit_by_name (nm, "network-changed", nm->priv->available);
    }
}

static void
got_connectivity (xobject_t *source,
                  xasync_result_t *res,
                  xpointer_t data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (source);
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (data);
  xerror_t *error = NULL;
  xvariant_t *ret;
  GNetworkConnectivity connectivity;

  ret = xdbus_proxy_call_finish (proxy, res, &error);
  if (ret == NULL)
    {
      if (!xerror_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD))
        {
          g_warning ("%s", error->message);
          g_clear_error (&error);
          return;
        }

      g_clear_error (&error);

      /* Fall back to version 1 */
      ret = xdbus_proxy_get_cached_property (nm->priv->proxy, "connectivity");
      if (ret == NULL)
        {
          g_warning ("Failed to get the '%s' property", "connectivity");
          return;
        }

      connectivity = xvariant_get_uint32 (ret);
      xvariant_unref (ret);
    }
  else
    {
      xvariant_get (ret, "(u)", &connectivity);
      xvariant_unref (ret);
    }

  if (nm->priv->connectivity != connectivity &&
      is_valid_connectivity (connectivity))
    {
      nm->priv->connectivity = connectivity;
      xobject_notify (G_OBJECT (nm), "connectivity");
      xsignal_emit_by_name (nm, "network-changed", nm->priv->available);
    }
}

static void
got_status (xobject_t *source,
            xasync_result_t *res,
            xpointer_t data)
{
  xdbus_proxy_t *proxy = G_DBUS_PROXY (source);
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (data);
  xerror_t *error = NULL;
  xvariant_t *ret;
  xvariant_t *status;
  xboolean_t available;
  xboolean_t metered;
  GNetworkConnectivity connectivity;

  ret = xdbus_proxy_call_finish (proxy, res, &error);
  if (ret == NULL)
    {
      if (xerror_matches (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD))
        {
          /* Fall back to version 2 */
          xdbus_proxy_call (proxy, "GetConnectivity", NULL, 0, -1, NULL, got_connectivity, nm);
          xdbus_proxy_call (proxy, "GetMetered", NULL, 0, -1, NULL, got_metered, nm);
          xdbus_proxy_call (proxy, "GetAvailable", NULL, 0, -1, NULL, got_available, nm);
        }
      else
        g_warning ("%s", error->message);

      g_clear_error (&error);
      return;
    }

  xvariant_get (ret, "(@a{sv})", &status);
  xvariant_unref (ret);

  xvariant_lookup (status, "available", "b", &available);
  xvariant_lookup (status, "metered", "b", &metered);
  xvariant_lookup (status, "connectivity", "u", &connectivity);
  xvariant_unref (status);

  xobject_freeze_notify (G_OBJECT (nm));

  if (nm->priv->available != available)
    {
      nm->priv->available = available;
      xobject_notify (G_OBJECT (nm), "network-available");
    }

  if (nm->priv->metered != metered)
    {
      nm->priv->metered = metered;
      xobject_notify (G_OBJECT (nm), "network-metered");
    }

  if (nm->priv->connectivity != connectivity &&
      is_valid_connectivity (connectivity))
    {
      nm->priv->connectivity = connectivity;
      xobject_notify (G_OBJECT (nm), "connectivity");
    }

  xobject_thaw_notify (G_OBJECT (nm));

  xsignal_emit_by_name (nm, "network-changed", available);
}

static void
update_properties (xdbus_proxy_t *proxy,
                   GNetworkMonitorPortal *nm)
{
  /* Try version 3 first */
  xdbus_proxy_call (proxy, "GetStatus", NULL, 0, -1, NULL, got_status, nm);
}

static void
proxy_signal (xdbus_proxy_t            *proxy,
              const char            *sender,
              const char            *signal,
              xvariant_t              *parameters,
              GNetworkMonitorPortal *nm)
{
  if (!nm->priv->has_network)
    return;

  if (strcmp (signal, "changed") != 0)
    return;

  /* Version 1 updates "available" with the "changed" signal */
  if (xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(b)")))
    {
      xboolean_t available;

      xvariant_get (parameters, "(b)", &available);
      if (nm->priv->available != available)
        {
          nm->priv->available = available;
          xobject_notify (G_OBJECT (nm), "available");
        }
      xsignal_emit_by_name (nm, "network-changed", available);
    }
  else
    {
      update_properties (proxy, nm);
    }
}

static void
proxy_properties_changed (xdbus_proxy_t            *proxy,
                          xvariant_t              *changed,
                          xvariant_t              *invalidated,
                          GNetworkMonitorPortal *nm)
{
  xboolean_t should_emit_changed = FALSE;
  xvariant_t *ret;

  if (!nm->priv->has_network)
    return;

  ret = xdbus_proxy_get_cached_property (proxy, "connectivity");
  if (ret)
    {
      GNetworkConnectivity connectivity = xvariant_get_uint32 (ret);
      if (nm->priv->connectivity != connectivity &&
          is_valid_connectivity (connectivity))
        {
          nm->priv->connectivity = connectivity;
          xobject_notify (G_OBJECT (nm), "connectivity");
          should_emit_changed = TRUE;
        }
      xvariant_unref (ret);
    }

  ret = xdbus_proxy_get_cached_property (proxy, "metered");
  if (ret)
    {
      xboolean_t metered = xvariant_get_boolean (ret);
      if (nm->priv->metered != metered)
        {
          nm->priv->metered = metered;
          xobject_notify (G_OBJECT (nm), "network-metered");
          should_emit_changed = TRUE;
        }
      xvariant_unref (ret);
    }

  ret = xdbus_proxy_get_cached_property (proxy, "available");
  if (ret)
    {
      xboolean_t available = xvariant_get_boolean (ret);
      if (nm->priv->available != available)
        {
          nm->priv->available = available;
          xobject_notify (G_OBJECT (nm), "network-available");
          should_emit_changed = TRUE;
        }
      xvariant_unref (ret);
    }

  if (should_emit_changed)
    xsignal_emit_by_name (nm, "network-changed", nm->priv->available);
}

static xboolean_t
xnetwork_monitor_portal_initable_init (xinitable_t     *initable,
                                        xcancellable_t  *cancellable,
                                        xerror_t       **error)
{
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (initable);
  xdbus_proxy_t *proxy;
  xchar_t *name_owner = NULL;

  nm->priv->available = FALSE;
  nm->priv->metered = FALSE;
  nm->priv->connectivity = G_NETWORK_CONNECTIVITY_LOCAL;

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
                                         "org.freedesktop.portal.NetworkMonitor",
                                         cancellable,
                                         error);
  if (!proxy)
    return FALSE;

  name_owner = xdbus_proxy_get_name_owner (proxy);

  if (!name_owner)
    {
      xobject_unref (proxy);
      g_set_error (error,
                   G_DBUS_ERROR,
                   G_DBUS_ERROR_NAME_HAS_NO_OWNER,
                   "Desktop portal not found");
      return FALSE;
    }

  g_free (name_owner);

  xsignal_connect (proxy, "g-signal", G_CALLBACK (proxy_signal), nm);
  xsignal_connect (proxy, "g-properties-changed", G_CALLBACK (proxy_properties_changed), nm);

  nm->priv->proxy = proxy;
  nm->priv->has_network = glib_network_available_in_sandbox ();

  if (!initable_parent_iface->init (initable, cancellable, error))
    return FALSE;

  if (nm->priv->has_network)
    update_properties (proxy, nm);

  return TRUE;
}

static void
xnetwork_monitor_portal_finalize (xobject_t *object)
{
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (object);

  g_clear_object (&nm->priv->proxy);

  XOBJECT_CLASS (xnetwork_monitor_portal_parent_class)->finalize (object);
}

static void
xnetwork_monitor_portal_class_init (GNetworkMonitorPortalClass *class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (class);

  xobject_class->finalize  = xnetwork_monitor_portal_finalize;
  xobject_class->get_property = xnetwork_monitor_portal_get_property;

  xobject_class_override_property (xobject_class, PROP_NETWORK_AVAILABLE, "network-available");
  xobject_class_override_property (xobject_class, PROP_NETWORK_METERED, "network-metered");
  xobject_class_override_property (xobject_class, PROP_CONNECTIVITY, "connectivity");
}

static xboolean_t
xnetwork_monitor_portal_can_reach (xnetwork_monitor_t     *monitor,
                                    xsocket_connectable_t  *connectable,
                                    xcancellable_t        *cancellable,
                                    xerror_t             **error)
{
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (monitor);
  xvariant_t *ret;
  xnetwork_address_t *address;
  xboolean_t reachable = FALSE;

  if (!X_IS_NETWORK_ADDRESS (connectable))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                   "Can't handle this kind of xsocket_connectable_t (%s)",
                   G_OBJECT_TYPE_NAME (connectable));
      return FALSE;
    }

  address = G_NETWORK_ADDRESS (connectable);

  ret = xdbus_proxy_call_sync (nm->priv->proxy,
                                "CanReach",
                                xvariant_new ("(su)",
                                               g_network_address_get_hostname (address),
                                               g_network_address_get_port (address)),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                cancellable,
                                error);

  if (ret)
    {
      xvariant_get (ret, "(b)", &reachable);
      xvariant_unref (ret);
    }

  return reachable;
}

static void
can_reach_done (xobject_t      *source,
                xasync_result_t *result,
                xpointer_t      data)
{
  xtask_t *task = data;
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (xtask_get_source_object (task));
  xerror_t *error = NULL;
  xvariant_t *ret;
  xboolean_t reachable;

  ret = xdbus_proxy_call_finish (nm->priv->proxy, result, &error);
  if (ret == NULL)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  xvariant_get (ret, "(b)", &reachable);
  xvariant_unref (ret);

  if (reachable)
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_new_error (task,
                             G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE,
                             "Can't reach host");

  xobject_unref (task);
}

static void
xnetwork_monitor_portal_can_reach_async (xnetwork_monitor_t     *monitor,
                                          xsocket_connectable_t  *connectable,
                                          xcancellable_t        *cancellable,
                                          xasync_ready_callback_t  callback,
                                          xpointer_t             data)
{
  GNetworkMonitorPortal *nm = G_NETWORK_MONITOR_PORTAL (monitor);
  xtask_t *task;
  xnetwork_address_t *address;

  task = xtask_new (monitor, cancellable, callback, data);

  if (!X_IS_NETWORK_ADDRESS (connectable))
    {
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               "Can't handle this kind of xsocket_connectable_t (%s)",
                               G_OBJECT_TYPE_NAME (connectable));
      xobject_unref (task);
      return;
    }

  address = G_NETWORK_ADDRESS (connectable);

  xdbus_proxy_call (nm->priv->proxy,
                     "CanReach",
                     xvariant_new ("(su)",
                                    g_network_address_get_hostname (address),
                                    g_network_address_get_port (address)),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     cancellable,
                     can_reach_done,
                     task);
}

static xboolean_t
xnetwork_monitor_portal_can_reach_finish (xnetwork_monitor_t  *monitor,
                                           xasync_result_t     *result,
                                           xerror_t          **error)
{
  return xtask_propagate_boolean (XTASK (result), error);
}

static void
xnetwork_monitor_portal_iface_init (GNetworkMonitorInterface *monitor_iface)
{
  monitor_iface->can_reach = xnetwork_monitor_portal_can_reach;
  monitor_iface->can_reach_async = xnetwork_monitor_portal_can_reach_async;
  monitor_iface->can_reach_finish = xnetwork_monitor_portal_can_reach_finish;
}

static void
xnetwork_monitor_portal_initable_iface_init (xinitable_iface_t *iface)
{
  initable_parent_iface = xtype_interface_peek_parent (iface);

  iface->init = xnetwork_monitor_portal_initable_init;
}
