/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc
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
#include "glib.h"
#include "glibintl.h"

#include "gnetworkmonitor.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "ginitable.h"
#include "gioenumtypes.h"
#include "giomodule-priv.h"
#include "gtask.h"

/**
 * SECTION:gnetworkmonitor
 * @title: xnetwork_monitor_t
 * @short_description: Network status monitor
 * @include: gio/gio.h
 *
 * #xnetwork_monitor_t provides an easy-to-use cross-platform API
 * for monitoring network connectivity. On Linux, the available
 * implementations are based on the kernel's netlink interface and
 * on NetworkManager.
 *
 * There is also an implementation for use inside Flatpak sandboxes.
 */

/**
 * xnetwork_monitor_t:
 *
 * #xnetwork_monitor_t monitors the status of network connections and
 * indicates when a possibly-user-visible change has occurred.
 *
 * Since: 2.32
 */

/**
 * GNetworkMonitorInterface:
 * @x_iface: The parent interface.
 * @network_changed: the virtual function pointer for the
 *  xnetwork_monitor_t::network-changed signal.
 * @can_reach: the virtual function pointer for g_network_monitor_can_reach()
 * @can_reach_async: the virtual function pointer for
 *  g_network_monitor_can_reach_async()
 * @can_reach_finish: the virtual function pointer for
 *  g_network_monitor_can_reach_finish()
 *
 * The virtual function table for #xnetwork_monitor_t.
 *
 * Since: 2.32
 */

G_DEFINE_INTERFACE_WITH_CODE (xnetwork_monitor_t, g_network_monitor, XTYPE_OBJECT,
                              xtype_interface_add_prerequisite (g_define_type_id, XTYPE_INITABLE))


enum {
  NETWORK_CHANGED,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };
static xnetwork_monitor_t *network_monitor_default_singleton = NULL;  /* (owned) (atomic) */

/**
 * g_network_monitor_get_default:
 *
 * Gets the default #xnetwork_monitor_t for the system.
 *
 * Returns: (not nullable) (transfer none): a #xnetwork_monitor_t, which will be
 *     a dummy object if no network monitor is available
 *
 * Since: 2.32
 */
xnetwork_monitor_t *
g_network_monitor_get_default (void)
{
  if (g_once_init_enter (&network_monitor_default_singleton))
    {
      xnetwork_monitor_t *singleton;

      singleton = _xio_module_get_default (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                            "GIO_USE_NETWORK_MONITOR",
                                            NULL);

      g_once_init_leave (&network_monitor_default_singleton, singleton);
    }

  return network_monitor_default_singleton;
}

/**
 * g_network_monitor_get_network_available:
 * @monitor: the #xnetwork_monitor_t
 *
 * Checks if the network is available. "Available" here means that the
 * system has a default route available for at least one of IPv4 or
 * IPv6. It does not necessarily imply that the public Internet is
 * reachable. See #xnetwork_monitor_t:network-available for more details.
 *
 * Returns: whether the network is available
 *
 * Since: 2.32
 */
xboolean_t
g_network_monitor_get_network_available (xnetwork_monitor_t *monitor)
{
  xboolean_t available = FALSE;

  xobject_get (G_OBJECT (monitor), "network-available", &available, NULL);
  return available;
}

/**
 * g_network_monitor_get_network_metered:
 * @monitor: the #xnetwork_monitor_t
 *
 * Checks if the network is metered.
 * See #xnetwork_monitor_t:network-metered for more details.
 *
 * Returns: whether the connection is metered
 *
 * Since: 2.46
 */
xboolean_t
g_network_monitor_get_network_metered (xnetwork_monitor_t *monitor)
{
  xboolean_t metered = FALSE;

  xobject_get (G_OBJECT (monitor), "network-metered", &metered, NULL);
  return metered;
}

/**
 * g_network_monitor_get_connectivity:
 * @monitor: the #xnetwork_monitor_t
 *
 * Gets a more detailed networking state than
 * g_network_monitor_get_network_available().
 *
 * If #xnetwork_monitor_t:network-available is %FALSE, then the
 * connectivity state will be %G_NETWORK_CONNECTIVITY_LOCAL.
 *
 * If #xnetwork_monitor_t:network-available is %TRUE, then the
 * connectivity state will be %G_NETWORK_CONNECTIVITY_FULL (if there
 * is full Internet connectivity), %G_NETWORK_CONNECTIVITY_LIMITED (if
 * the host has a default route, but appears to be unable to actually
 * reach the full Internet), or %G_NETWORK_CONNECTIVITY_PORTAL (if the
 * host is trapped behind a "captive portal" that requires some sort
 * of login or acknowledgement before allowing full Internet access).
 *
 * Note that in the case of %G_NETWORK_CONNECTIVITY_LIMITED and
 * %G_NETWORK_CONNECTIVITY_PORTAL, it is possible that some sites are
 * reachable but others are not. In this case, applications can
 * attempt to connect to remote servers, but should gracefully fall
 * back to their "offline" behavior if the connection attempt fails.
 *
 * Return value: the network connectivity state
 *
 * Since: 2.44
 */
GNetworkConnectivity
g_network_monitor_get_connectivity (xnetwork_monitor_t *monitor)
{
  GNetworkConnectivity connectivity;

  xobject_get (G_OBJECT (monitor), "connectivity", &connectivity, NULL);

  return connectivity;
}

/**
 * g_network_monitor_can_reach:
 * @monitor: a #xnetwork_monitor_t
 * @connectable: a #xsocket_connectable_t
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Attempts to determine whether or not the host pointed to by
 * @connectable can be reached, without actually trying to connect to
 * it.
 *
 * This may return %TRUE even when #xnetwork_monitor_t:network-available
 * is %FALSE, if, for example, @monitor can determine that
 * @connectable refers to a host on a local network.
 *
 * If @monitor believes that an attempt to connect to @connectable
 * will succeed, it will return %TRUE. Otherwise, it will return
 * %FALSE and set @error to an appropriate error (such as
 * %G_IO_ERROR_HOST_UNREACHABLE).
 *
 * Note that although this does not attempt to connect to
 * @connectable, it may still block for a brief period of time (eg,
 * trying to do multicast DNS on the local network), so if you do not
 * want to block, you should use g_network_monitor_can_reach_async().
 *
 * Returns: %TRUE if @connectable is reachable, %FALSE if not.
 *
 * Since: 2.32
 */
xboolean_t
g_network_monitor_can_reach (xnetwork_monitor_t     *monitor,
                             xsocket_connectable_t  *connectable,
                             xcancellable_t        *cancellable,
                             xerror_t             **error)
{
  GNetworkMonitorInterface *iface;

  iface = G_NETWORK_MONITOR_GET_INTERFACE (monitor);
  return iface->can_reach (monitor, connectable, cancellable, error);
}

static void
g_network_monitor_real_can_reach_async (xnetwork_monitor_t     *monitor,
                                        xsocket_connectable_t  *connectable,
                                        xcancellable_t        *cancellable,
                                        xasync_ready_callback_t  callback,
                                        xpointer_t             user_data)
{
  xtask_t *task;
  xerror_t *error = NULL;

  task = xtask_new (monitor, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_network_monitor_real_can_reach_async);

  if (g_network_monitor_can_reach (monitor, connectable, cancellable, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * g_network_monitor_can_reach_async:
 * @monitor: a #xnetwork_monitor_t
 * @connectable: a #xsocket_connectable_t
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t to call when the
 *     request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously attempts to determine whether or not the host
 * pointed to by @connectable can be reached, without actually
 * trying to connect to it.
 *
 * For more details, see g_network_monitor_can_reach().
 *
 * When the operation is finished, @callback will be called.
 * You can then call g_network_monitor_can_reach_finish()
 * to get the result of the operation.
 */
void
g_network_monitor_can_reach_async (xnetwork_monitor_t     *monitor,
                                   xsocket_connectable_t  *connectable,
                                   xcancellable_t        *cancellable,
                                   xasync_ready_callback_t  callback,
                                   xpointer_t             user_data)
{
  GNetworkMonitorInterface *iface;

  iface = G_NETWORK_MONITOR_GET_INTERFACE (monitor);
  iface->can_reach_async (monitor, connectable, cancellable, callback, user_data);
}

static xboolean_t
g_network_monitor_real_can_reach_finish (xnetwork_monitor_t  *monitor,
                                         xasync_result_t     *result,
                                         xerror_t          **error)
{
  g_return_val_if_fail (xtask_is_valid (result, monitor), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

/**
 * g_network_monitor_can_reach_finish:
 * @monitor: a #xnetwork_monitor_t
 * @result: a #xasync_result_t
 * @error: return location for errors, or %NULL
 *
 * Finishes an async network connectivity test.
 * See g_network_monitor_can_reach_async().
 *
 * Returns: %TRUE if network is reachable, %FALSE if not.
 */
xboolean_t
g_network_monitor_can_reach_finish (xnetwork_monitor_t     *monitor,
                                    xasync_result_t        *result,
                                    xerror_t             **error)
{
  GNetworkMonitorInterface *iface;

  iface = G_NETWORK_MONITOR_GET_INTERFACE (monitor);
  return iface->can_reach_finish (monitor, result, error);
}

static void
g_network_monitor_default_init (GNetworkMonitorInterface *iface)
{
  iface->can_reach_async  = g_network_monitor_real_can_reach_async;
  iface->can_reach_finish = g_network_monitor_real_can_reach_finish;

  /**
   * xnetwork_monitor_t::network-changed:
   * @monitor: a #xnetwork_monitor_t
   * @network_available: the current value of #xnetwork_monitor_t:network-available
   *
   * Emitted when the network configuration changes.
   *
   * Since: 2.32
   */
  signals[NETWORK_CHANGED] =
    g_signal_new (I_("network-changed"),
                  XTYPE_NETWORK_MONITOR,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GNetworkMonitorInterface, network_changed),
                  NULL, NULL,
                  NULL,
                  XTYPE_NONE, 1,
                  XTYPE_BOOLEAN);

  /**
   * xnetwork_monitor_t:network-available:
   *
   * Whether the network is considered available. That is, whether the
   * system has a default route for at least one of IPv4 or IPv6.
   *
   * Real-world networks are of course much more complicated than
   * this; the machine may be connected to a wifi hotspot that
   * requires payment before allowing traffic through, or may be
   * connected to a functioning router that has lost its own upstream
   * connectivity. Some hosts might only be accessible when a VPN is
   * active. Other hosts might only be accessible when the VPN is
   * not active. Thus, it is best to use g_network_monitor_can_reach()
   * or g_network_monitor_can_reach_async() to test for reachability
   * on a host-by-host basis. (On the other hand, when the property is
   * %FALSE, the application can reasonably expect that no remote
   * hosts at all are reachable, and should indicate this to the user
   * in its UI.)
   *
   * See also #xnetwork_monitor_t::network-changed.
   *
   * Since: 2.32
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_boolean ("network-available",
                                                             P_("Network available"),
                                                             P_("Whether the network is available"),
                                                             FALSE,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_STATIC_STRINGS));

  /**
   * xnetwork_monitor_t:network-metered:
   *
   * Whether the network is considered metered. That is, whether the
   * system has traffic flowing through the default connection that is
   * subject to limitations set by service providers. For example, traffic
   * might be billed by the amount of data transmitted, or there might be a
   * quota on the amount of traffic per month. This is typical with tethered
   * connections (3G and 4G) and in such situations, bandwidth intensive
   * applications may wish to avoid network activity where possible if it will
   * cost the user money or use up their limited quota.
   *
   * If more information is required about specific devices then the
   * system network management API should be used instead (for example,
   * NetworkManager or ConnMan).
   *
   * If this information is not available then no networks will be
   * marked as metered.
   *
   * See also #xnetwork_monitor_t:network-available.
   *
   * Since: 2.46
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_boolean ("network-metered",
                                                             P_("Network metered"),
                                                             P_("Whether the network is metered"),
                                                             FALSE,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_STATIC_STRINGS));

  /**
   * xnetwork_monitor_t:connectivity:
   *
   * More detailed information about the host's network connectivity.
   * See g_network_monitor_get_connectivity() and
   * #GNetworkConnectivity for more details.
   *
   * Since: 2.44
   */
  xobject_interface_install_property (iface,
                                       g_param_spec_enum ("connectivity",
                                                          P_("Network connectivity"),
                                                          P_("Level of network connectivity"),
                                                          XTYPE_NETWORK_CONNECTIVITY,
                                                          G_NETWORK_CONNECTIVITY_FULL,
                                                          G_PARAM_READABLE |
                                                          G_PARAM_STATIC_STRINGS));
}
