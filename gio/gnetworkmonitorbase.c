/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
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

#include "gnetworkmonitorbase.h"
#include "ginetaddress.h"
#include "ginetaddressmask.h"
#include "ginetsocketaddress.h"
#include "ginitable.h"
#include "gioerror.h"
#include "giomodule-priv.h"
#include "gnetworkmonitor.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"
#include "gtask.h"
#include "glibintl.h"

static void xnetwork_monitor_base_iface_init (GNetworkMonitorInterface *iface);
static void xnetwork_monitor_base_initable_iface_init (xinitable_iface_t *iface);

enum
{
  PROP_0,

  PROP_NETWORK_AVAILABLE,
  PROP_NETWORK_METERED,
  PROP_CONNECTIVITY
};

struct _GNetworkMonitorBasePrivate
{
  xhashtable_t   *networks  /* (element-type xinet_address_mask_t) (owned) */;
  xboolean_t      have_ipv4_default_route;
  xboolean_t      have_ipv6_default_route;
  xboolean_t      is_available;

  xmain_context_t *context;
  xsource_t      *network_changed_source;
  xboolean_t      initializing;
};

static xuint_t network_changed_signal = 0;

static void queue_network_changed (xnetwork_monitor_base_t *monitor);
static xuint_t inet_address_mask_hash (xconstpointer key);
static xboolean_t inet_address_mask_equal (xconstpointer a,
                                         xconstpointer b);

G_DEFINE_TYPE_WITH_CODE (xnetwork_monitor_base, xnetwork_monitor_base, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xnetwork_monitor_base)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xnetwork_monitor_base_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_NETWORK_MONITOR,
                                                xnetwork_monitor_base_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "base",
                                                         0))

static void
xnetwork_monitor_base_init (xnetwork_monitor_base_t *monitor)
{
  monitor->priv = xnetwork_monitor_base_get_instance_private (monitor);
  monitor->priv->networks = xhash_table_new_full (inet_address_mask_hash,
                                                   inet_address_mask_equal,
                                                   xobject_unref, NULL);
  monitor->priv->context = xmain_context_get_thread_default ();
  if (monitor->priv->context)
    xmain_context_ref (monitor->priv->context);

  monitor->priv->initializing = TRUE;
}

static void
xnetwork_monitor_base_constructed (xobject_t *object)
{
  xnetwork_monitor_base_t *monitor = G_NETWORK_MONITOR_BASE (object);

  if (G_OBJECT_TYPE (monitor) == XTYPE_NETWORK_MONITOR_BASE)
    {
      xinet_address_mask_t *mask;

      /* We're the dumb base class, not a smarter subclass. So just
       * assume that the network is available.
       */
      mask = xinet_address_mask_new_from_string ("0.0.0.0/0", NULL);
      xnetwork_monitor_base_add_network (monitor, mask);
      xobject_unref (mask);

      mask = xinet_address_mask_new_from_string ("::/0", NULL);
      if (mask)
        {
          /* On some environments (for example Windows without IPv6 support
           * enabled) the string "::/0" can't be processed and causes
           * xinet_address_mask_new_from_string to return NULL */
          xnetwork_monitor_base_add_network (monitor, mask);
          xobject_unref (mask);
        }
    }
}

static void
xnetwork_monitor_base_get_property (xobject_t    *object,
                                     xuint_t       prop_id,
                                     xvalue_t     *value,
                                     xparam_spec_t *pspec)
{
  xnetwork_monitor_base_t *monitor = G_NETWORK_MONITOR_BASE (object);

  switch (prop_id)
    {
    case PROP_NETWORK_AVAILABLE:
      xvalue_set_boolean (value, monitor->priv->is_available);
      break;

    case PROP_NETWORK_METERED:
      /* Default to FALSE in the unknown case. */
      xvalue_set_boolean (value, FALSE);
      break;

    case PROP_CONNECTIVITY:
      xvalue_set_enum (value,
                        monitor->priv->is_available ?
                        G_NETWORK_CONNECTIVITY_FULL :
                        G_NETWORK_CONNECTIVITY_LOCAL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
xnetwork_monitor_base_finalize (xobject_t *object)
{
  xnetwork_monitor_base_t *monitor = G_NETWORK_MONITOR_BASE (object);

  xhash_table_unref (monitor->priv->networks);
  if (monitor->priv->network_changed_source)
    {
      xsource_destroy (monitor->priv->network_changed_source);
      xsource_unref (monitor->priv->network_changed_source);
    }
  if (monitor->priv->context)
    xmain_context_unref (monitor->priv->context);

  XOBJECT_CLASS (xnetwork_monitor_base_parent_class)->finalize (object);
}

static void
xnetwork_monitor_base_class_init (xnetwork_monitor_base_class_t *monitor_class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (monitor_class);

  xobject_class->constructed  = xnetwork_monitor_base_constructed;
  xobject_class->get_property = xnetwork_monitor_base_get_property;
  xobject_class->finalize     = xnetwork_monitor_base_finalize;

  xobject_class_override_property (xobject_class, PROP_NETWORK_AVAILABLE, "network-available");
  xobject_class_override_property (xobject_class, PROP_NETWORK_METERED, "network-metered");
  xobject_class_override_property (xobject_class, PROP_CONNECTIVITY, "connectivity");
}

static xboolean_t
xnetwork_monitor_base_can_reach_sockaddr (xnetwork_monitor_base_t *base,
                                           xsocket_address_t *sockaddr)
{
  xinet_address_t *iaddr;
  xhash_table_iter_t iter;
  xpointer_t key;

  if (!X_IS_INET_SOCKET_ADDRESS (sockaddr))
    return FALSE;

  iaddr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (sockaddr));
  xhash_table_iter_init (&iter, base->priv->networks);
  while (xhash_table_iter_next (&iter, &key, NULL))
    {
      xinet_address_mask_t *mask = key;
      if (xinet_address_mask_matches (mask, iaddr))
        return TRUE;
    }

  return FALSE;
}

static xboolean_t
xnetwork_monitor_base_can_reach (xnetwork_monitor_t      *monitor,
                                  xsocket_connectable_t   *connectable,
                                  xcancellable_t         *cancellable,
                                  xerror_t              **error)
{
  xnetwork_monitor_base_t *base = G_NETWORK_MONITOR_BASE (monitor);
  xsocket_address_enumerator_t *enumerator;
  xsocket_address_t *addr;

  if (xhash_table_size (base->priv->networks) == 0)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NETWORK_UNREACHABLE,
                           _("Network unreachable"));
      return FALSE;
    }

  enumerator = xsocket_connectable_proxy_enumerate (connectable);
  addr = xsocket_address_enumerator_next (enumerator, cancellable, error);
  if (!addr)
    {
      /* Either the user cancelled, or DNS resolution failed */
      xobject_unref (enumerator);
      return FALSE;
    }

  if (base->priv->have_ipv4_default_route &&
      base->priv->have_ipv6_default_route)
    {
      xobject_unref (enumerator);
      xobject_unref (addr);
      return TRUE;
    }

  while (addr)
    {
      if (xnetwork_monitor_base_can_reach_sockaddr (base, addr))
        {
          xobject_unref (addr);
          xobject_unref (enumerator);
          return TRUE;
        }

      xobject_unref (addr);
      addr = xsocket_address_enumerator_next (enumerator, cancellable, error);
    }
  xobject_unref (enumerator);

  if (error && !*error)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE,
                           _("Host unreachable"));
    }
  return FALSE;
}

static void
can_reach_async_got_address (xobject_t      *object,
                             xasync_result_t *result,
                             xpointer_t      user_data)
{
  xsocket_address_enumerator_t *enumerator = XSOCKET_ADDRESS_ENUMERATOR (object);
  xtask_t *task = user_data;
  xnetwork_monitor_base_t *base = xtask_get_source_object (task);
  xsocket_address_t *addr;
  xerror_t *error = NULL;

  addr = xsocket_address_enumerator_next_finish (enumerator, result, &error);
  if (!addr)
    {
      if (error)
        {
          /* Either the user cancelled, or DNS resolution failed */
          xtask_return_error (task, error);
          xobject_unref (task);
          return;
        }
      else
        {
          /* Resolved all addresses, none matched */
          xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE,
                                   _("Host unreachable"));
          xobject_unref (task);
          return;
        }
    }

  if (xnetwork_monitor_base_can_reach_sockaddr (base, addr))
    {
      xobject_unref (addr);
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
      return;
    }
  xobject_unref (addr);

  xsocket_address_enumerator_next_async (enumerator,
                                          xtask_get_cancellable (task),
                                          can_reach_async_got_address, task);
}

static void
xnetwork_monitor_base_can_reach_async (xnetwork_monitor_t     *monitor,
                                        xsocket_connectable_t  *connectable,
                                        xcancellable_t        *cancellable,
                                        xasync_ready_callback_t  callback,
                                        xpointer_t             user_data)
{
  xtask_t *task;
  xsocket_address_enumerator_t *enumerator;

  task = xtask_new (monitor, cancellable, callback, user_data);
  xtask_set_source_tag (task, xnetwork_monitor_base_can_reach_async);

  if (xhash_table_size (G_NETWORK_MONITOR_BASE (monitor)->priv->networks) == 0)
    {
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NETWORK_UNREACHABLE,
                               _("Network unreachable"));
      xobject_unref (task);
      return;
    }

  enumerator = xsocket_connectable_proxy_enumerate (connectable);
  xsocket_address_enumerator_next_async (enumerator, cancellable,
                                          can_reach_async_got_address, task);
  xobject_unref (enumerator);
}

static xboolean_t
xnetwork_monitor_base_can_reach_finish (xnetwork_monitor_t  *monitor,
                                         xasync_result_t     *result,
                                         xerror_t          **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, monitor), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static void
xnetwork_monitor_base_iface_init (GNetworkMonitorInterface *monitor_iface)
{
  monitor_iface->can_reach = xnetwork_monitor_base_can_reach;
  monitor_iface->can_reach_async = xnetwork_monitor_base_can_reach_async;
  monitor_iface->can_reach_finish = xnetwork_monitor_base_can_reach_finish;

  network_changed_signal = xsignal_lookup ("network-changed", XTYPE_NETWORK_MONITOR);
}

static xboolean_t
xnetwork_monitor_base_initable_init (xinitable_t     *initable,
                                      xcancellable_t  *cancellable,
                                      xerror_t       **error)
{
  xnetwork_monitor_base_t *base = G_NETWORK_MONITOR_BASE (initable);

  base->priv->initializing = FALSE;

  return TRUE;
}

static void
xnetwork_monitor_base_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = xnetwork_monitor_base_initable_init;
}

static xuint_t
inet_address_mask_hash (xconstpointer key)
{
  xinet_address_mask_t *mask = XINET_ADDRESS_MASK (key);
  xuint_t addr_hash;
  xuint_t mask_length = xinet_address_mask_get_length (mask);
  xinet_address_t *addr = xinet_address_mask_get_address (mask);
  const xuint8_t *bytes = xinet_address_to_bytes (addr);
  xsize_t bytes_length = xinet_address_get_native_size (addr);

  union
    {
      const xuint8_t *bytes;
      xuint32_t *hash32;
      xuint64_t *hash64;
    } integerifier;

  /* If we can fit the entire address into the hash key, do it. Don’t worry
   * about endianness; the address should always be in network endianness. */
  if (bytes_length == sizeof (xuint32_t))
    {
      integerifier.bytes = bytes;
      addr_hash = *integerifier.hash32;
    }
  else if (bytes_length == sizeof (xuint64_t))
    {
      integerifier.bytes = bytes;
      addr_hash = *integerifier.hash64;
    }
  else
    {
      xsize_t i;

      /* Otherwise, fall back to adding the bytes together. We do this, rather
       * than XORing them, as routes often have repeated tuples which would
       * cancel out under XOR. */
      addr_hash = 0;
      for (i = 0; i < bytes_length; i++)
        addr_hash += bytes[i];
    }

  return addr_hash + mask_length;;
}

static xboolean_t
inet_address_mask_equal (xconstpointer a,
                         xconstpointer b)
{
  xinet_address_mask_t *mask_a = XINET_ADDRESS_MASK (a);
  xinet_address_mask_t *mask_b = XINET_ADDRESS_MASK (b);

  return xinet_address_mask_equal (mask_a, mask_b);
}

static xboolean_t
emit_network_changed (xpointer_t user_data)
{
  xnetwork_monitor_base_t *monitor = user_data;
  xboolean_t is_available;

  if (xsource_is_destroyed (g_main_current_source ()))
    return FALSE;

  xobject_ref (monitor);

  is_available = (monitor->priv->have_ipv4_default_route ||
                  monitor->priv->have_ipv6_default_route);
  if (monitor->priv->is_available != is_available)
    {
      monitor->priv->is_available = is_available;
      xobject_notify (G_OBJECT (monitor), "network-available");
    }

  xsignal_emit (monitor, network_changed_signal, 0, is_available);

  xsource_unref (monitor->priv->network_changed_source);
  monitor->priv->network_changed_source = NULL;

  xobject_unref (monitor);
  return FALSE;
}

static void
queue_network_changed (xnetwork_monitor_base_t *monitor)
{
  if (!monitor->priv->network_changed_source &&
      !monitor->priv->initializing)
    {
      xsource_t *source;

      source = g_idle_source_new ();
      /* Use G_PRIORITY_HIGH_IDLE priority so that multiple
       * network-change-related notifications coming in at
       * G_PRIORITY_DEFAULT will get coalesced into one signal
       * emission.
       */
      xsource_set_priority (source, G_PRIORITY_HIGH_IDLE);
      xsource_set_callback (source, emit_network_changed, monitor, NULL);
      xsource_set_static_name (source, "[gio] emit_network_changed");
      xsource_attach (source, monitor->priv->context);
      monitor->priv->network_changed_source = source;
    }

  /* Normally we wait to update is_available until we emit the signal,
   * to keep things consistent. But when we're first creating the
   * object, we want it to be correct right away.
   */
  if (monitor->priv->initializing)
    {
      monitor->priv->is_available = (monitor->priv->have_ipv4_default_route ||
                                     monitor->priv->have_ipv6_default_route);
    }
}

/**
 * xnetwork_monitor_base_add_network:
 * @monitor: the #xnetwork_monitor_base_t
 * @network: (transfer none): a #xinet_address_mask_t
 *
 * Adds @network to @monitor's list of available networks.
 *
 * Since: 2.32
 */
void
xnetwork_monitor_base_add_network (xnetwork_monitor_base_t *monitor,
                                    xinet_address_mask_t    *network)
{
  if (!xhash_table_add (monitor->priv->networks, xobject_ref (network)))
    return;

  if (xinet_address_mask_get_length (network) == 0)
    {
      switch (xinet_address_mask_get_family (network))
        {
        case XSOCKET_FAMILY_IPV4:
          monitor->priv->have_ipv4_default_route = TRUE;
          break;
        case XSOCKET_FAMILY_IPV6:
          monitor->priv->have_ipv6_default_route = TRUE;
          break;
        default:
          break;
        }
    }

  /* Don't emit network-changed when multicast-link-local routing
   * changes. This rather arbitrary decision is mostly because it
   * seems to change quite often...
   */
  if (xinet_address_get_is_mc_link_local (xinet_address_mask_get_address (network)))
    return;

  queue_network_changed (monitor);
}

/**
 * xnetwork_monitor_base_remove_network:
 * @monitor: the #xnetwork_monitor_base_t
 * @network: a #xinet_address_mask_t
 *
 * Removes @network from @monitor's list of available networks.
 *
 * Since: 2.32
 */
void
xnetwork_monitor_base_remove_network (xnetwork_monitor_base_t *monitor,
                                       xinet_address_mask_t    *network)
{
  if (!xhash_table_remove (monitor->priv->networks, network))
    return;

  if (xinet_address_mask_get_length (network) == 0)
    {
      switch (xinet_address_mask_get_family (network))
        {
        case XSOCKET_FAMILY_IPV4:
          monitor->priv->have_ipv4_default_route = FALSE;
          break;
        case XSOCKET_FAMILY_IPV6:
          monitor->priv->have_ipv6_default_route = FALSE;
          break;
        default:
          break;
        }
    }

  queue_network_changed (monitor);
}

/**
 * xnetwork_monitor_base_set_networks:
 * @monitor: the #xnetwork_monitor_base_t
 * @networks: (array length=length): an array of #xinet_address_mask_t
 * @length: length of @networks
 *
 * Drops @monitor's current list of available networks and replaces
 * it with @networks.
 */
void
xnetwork_monitor_base_set_networks (xnetwork_monitor_base_t  *monitor,
                                     xinet_address_mask_t    **networks,
                                     xint_t                  length)
{
  int i;

  xhash_table_remove_all (monitor->priv->networks);
  monitor->priv->have_ipv4_default_route = FALSE;
  monitor->priv->have_ipv6_default_route = FALSE;

  for (i = 0; i < length; i++)
    xnetwork_monitor_base_add_network (monitor, networks[i]);
}
