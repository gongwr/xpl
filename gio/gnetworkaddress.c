/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2018 Igalia S.L.
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
#include <glib.h>
#include "glibintl.h"

#include <stdlib.h>
#include "gnetworkaddress.h"
#include "gasyncresult.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gnetworkingprivate.h"
#include "gproxyaddressenumerator.h"
#include "gresolver.h"
#include "gtask.h"
#include "gsocketaddressenumerator.h"
#include "gioerror.h"
#include "gsocketconnectable.h"

#include <string.h>

/* As recommended by RFC 8305 this is the time it waits for a following
   DNS response to come in (ipv4 waiting on ipv6 generally)
 */
#define HAPPY_EYEBALLS_RESOLUTION_DELAY_MS 50

/**
 * SECTION:gnetworkaddress
 * @short_description: A GSocketConnectable for resolving hostnames
 * @include: gio/gio.h
 *
 * #GNetworkAddress provides an easy way to resolve a hostname and
 * then attempt to connect to that host, handling the possibility of
 * multiple IP addresses and multiple address families.
 *
 * The enumeration results of resolved addresses *may* be cached as long
 * as this object is kept alive which may have unexpected results if
 * alive for too long.
 *
 * See #GSocketConnectable for an example of using the connectable
 * interface.
 */

/**
 * GNetworkAddress:
 *
 * A #GSocketConnectable for resolving a hostname and connecting to
 * that host.
 */

struct _GNetworkAddressPrivate {
  xchar_t *hostname;
  guint16 port;
  xlist_t *cached_sockaddrs;
  xchar_t *scheme;

  gint64 resolver_serial;
};

enum {
  PROP_0,
  PROP_HOSTNAME,
  PROP_PORT,
  PROP_SCHEME,
};

static void g_network_address_set_property (xobject_t      *object,
                                            xuint_t         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void g_network_address_get_property (xobject_t      *object,
                                            xuint_t         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

static void                      g_network_address_connectable_iface_init       (GSocketConnectableIface *iface);
static GSocketAddressEnumerator *g_network_address_connectable_enumerate        (GSocketConnectable      *connectable);
static GSocketAddressEnumerator	*g_network_address_connectable_proxy_enumerate  (GSocketConnectable      *connectable);
static xchar_t                    *g_network_address_connectable_to_string        (GSocketConnectable      *connectable);

G_DEFINE_TYPE_WITH_CODE (GNetworkAddress, g_network_address, XTYPE_OBJECT,
                         G_ADD_PRIVATE (GNetworkAddress)
                         G_IMPLEMENT_INTERFACE (XTYPE_SOCKET_CONNECTABLE,
                                                g_network_address_connectable_iface_init))

static void
g_network_address_finalize (xobject_t *object)
{
  GNetworkAddress *addr = G_NETWORK_ADDRESS (object);

  g_free (addr->priv->hostname);
  g_free (addr->priv->scheme);
  g_list_free_full (addr->priv->cached_sockaddrs, g_object_unref);

  G_OBJECT_CLASS (g_network_address_parent_class)->finalize (object);
}

static void
g_network_address_class_init (GNetworkAddressClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = g_network_address_set_property;
  gobject_class->get_property = g_network_address_get_property;
  gobject_class->finalize = g_network_address_finalize;

  g_object_class_install_property (gobject_class, PROP_HOSTNAME,
                                   g_param_spec_string ("hostname",
                                                        P_("Hostname"),
                                                        P_("Hostname to resolve"),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
                                   g_param_spec_uint ("port",
                                                      P_("Port"),
                                                      P_("Network port"),
                                                      0, 65535, 0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SCHEME,
                                   g_param_spec_string ("scheme",
                                                        P_("Scheme"),
                                                        P_("URI Scheme"),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
g_network_address_connectable_iface_init (GSocketConnectableIface *connectable_iface)
{
  connectable_iface->enumerate  = g_network_address_connectable_enumerate;
  connectable_iface->proxy_enumerate = g_network_address_connectable_proxy_enumerate;
  connectable_iface->to_string = g_network_address_connectable_to_string;
}

static void
g_network_address_init (GNetworkAddress *addr)
{
  addr->priv = g_network_address_get_instance_private (addr);
}

static void
g_network_address_set_property (xobject_t      *object,
                                xuint_t         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GNetworkAddress *addr = G_NETWORK_ADDRESS (object);

  switch (prop_id)
    {
    case PROP_HOSTNAME:
      g_free (addr->priv->hostname);
      addr->priv->hostname = g_value_dup_string (value);
      break;

    case PROP_PORT:
      addr->priv->port = g_value_get_uint (value);
      break;

    case PROP_SCHEME:
      g_free (addr->priv->scheme);
      addr->priv->scheme = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_network_address_get_property (xobject_t    *object,
                                xuint_t       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GNetworkAddress *addr = G_NETWORK_ADDRESS (object);

  switch (prop_id)
    {
    case PROP_HOSTNAME:
      g_value_set_string (value, addr->priv->hostname);
      break;

    case PROP_PORT:
      g_value_set_uint (value, addr->priv->port);
      break;

    case PROP_SCHEME:
      g_value_set_string (value, addr->priv->scheme);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

/*
 * inet_addresses_to_inet_socket_addresses:
 * @addresses: (transfer full): #xlist_t of #xinet_address_t
 *
 * Returns: (transfer full): #xlist_t of #GInetSocketAddress
 */
static xlist_t *
inet_addresses_to_inet_socket_addresses (GNetworkAddress *addr,
                                         xlist_t           *addresses)
{
  xlist_t *a, *socket_addresses = NULL;

  for (a = addresses; a; a = a->next)
    {
      xsocket_address_t *sockaddr = g_inet_socket_address_new (a->data, addr->priv->port);
      socket_addresses = g_list_append (socket_addresses, g_steal_pointer (&sockaddr));
      g_object_unref (a->data);
    }

  g_list_free (addresses);
  return socket_addresses;
}

/*
 * g_network_address_set_cached_addresses:
 * @addr: A #GNetworkAddress
 * @addresses: (transfer full): List of #xinet_address_t or #GInetSocketAddress
 * @resolver_serial: Serial of #GResolver used
 *
 * Consumes @addresses and uses them to replace the current internal list.
 */
static void
g_network_address_set_cached_addresses (GNetworkAddress *addr,
                                        xlist_t           *addresses,
                                        guint64          resolver_serial)
{
  g_assert (addresses != NULL);

  if (addr->priv->cached_sockaddrs)
    g_list_free_full (addr->priv->cached_sockaddrs, g_object_unref);

  if (X_IS_INET_SOCKET_ADDRESS (addresses->data))
    addr->priv->cached_sockaddrs = g_steal_pointer (&addresses);
  else
    addr->priv->cached_sockaddrs = inet_addresses_to_inet_socket_addresses (addr, g_steal_pointer (&addresses));
  addr->priv->resolver_serial = resolver_serial;
}

static xboolean_t
g_network_address_parse_sockaddr (GNetworkAddress *addr)
{
  xsocket_address_t *sockaddr;

  g_assert (addr->priv->cached_sockaddrs == NULL);

  sockaddr = g_inet_socket_address_new_from_string (addr->priv->hostname,
                                                    addr->priv->port);
  if (sockaddr)
    {
      addr->priv->cached_sockaddrs = g_list_append (addr->priv->cached_sockaddrs, sockaddr);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * g_network_address_new:
 * @hostname: the hostname
 * @port: the port
 *
 * Creates a new #GSocketConnectable for connecting to the given
 * @hostname and @port.
 *
 * Note that depending on the configuration of the machine, a
 * @hostname of `localhost` may refer to the IPv4 loopback address
 * only, or to both IPv4 and IPv6; use
 * g_network_address_new_loopback() to create a #GNetworkAddress that
 * is guaranteed to resolve to both addresses.
 *
 * Returns: (transfer full) (type GNetworkAddress): the new #GNetworkAddress
 *
 * Since: 2.22
 */
GSocketConnectable *
g_network_address_new (const xchar_t *hostname,
                       guint16      port)
{
  return g_object_new (XTYPE_NETWORK_ADDRESS,
                       "hostname", hostname,
                       "port", port,
                       NULL);
}

/**
 * g_network_address_new_loopback:
 * @port: the port
 *
 * Creates a new #GSocketConnectable for connecting to the local host
 * over a loopback connection to the given @port. This is intended for
 * use in connecting to local services which may be running on IPv4 or
 * IPv6.
 *
 * The connectable will return IPv4 and IPv6 loopback addresses,
 * regardless of how the host resolves `localhost`. By contrast,
 * g_network_address_new() will often only return an IPv4 address when
 * resolving `localhost`, and an IPv6 address for `localhost6`.
 *
 * g_network_address_get_hostname() will always return `localhost` for
 * a #GNetworkAddress created with this constructor.
 *
 * Returns: (transfer full) (type GNetworkAddress): the new #GNetworkAddress
 *
 * Since: 2.44
 */
GSocketConnectable *
g_network_address_new_loopback (guint16 port)
{
  GNetworkAddress *addr;
  xlist_t *addrs = NULL;

  addr = g_object_new (XTYPE_NETWORK_ADDRESS,
                       "hostname", "localhost",
                       "port", port,
                       NULL);

  addrs = g_list_append (addrs, xinet_address_new_loopback (AF_INET6));
  addrs = g_list_append (addrs, xinet_address_new_loopback (AF_INET));
  g_network_address_set_cached_addresses (addr, g_steal_pointer (&addrs), 0);

  return XSOCKET_CONNECTABLE (addr);
}

/**
 * g_network_address_parse:
 * @host_and_port: the hostname and optionally a port
 * @default_port: the default port if not in @host_and_port
 * @error: a pointer to a #xerror_t, or %NULL
 *
 * Creates a new #GSocketConnectable for connecting to the given
 * @hostname and @port. May fail and return %NULL in case
 * parsing @host_and_port fails.
 *
 * @host_and_port may be in any of a number of recognised formats; an IPv6
 * address, an IPv4 address, or a domain name (in which case a DNS
 * lookup is performed). Quoting with [] is supported for all address
 * types. A port override may be specified in the usual way with a
 * colon.
 *
 * If no port is specified in @host_and_port then @default_port will be
 * used as the port number to connect to.
 *
 * In general, @host_and_port is expected to be provided by the user
 * (allowing them to give the hostname, and a port override if necessary)
 * and @default_port is expected to be provided by the application.
 *
 * (The port component of @host_and_port can also be specified as a
 * service name rather than as a numeric port, but this functionality
 * is deprecated, because it depends on the contents of /etc/services,
 * which is generally quite sparse on platforms other than Linux.)
 *
 * Returns: (transfer full) (type GNetworkAddress): the new
 *   #GNetworkAddress, or %NULL on error
 *
 * Since: 2.22
 */
GSocketConnectable *
g_network_address_parse (const xchar_t  *host_and_port,
                         guint16       default_port,
                         xerror_t      **error)
{
  GSocketConnectable *connectable;
  const xchar_t *port;
  guint16 portnum;
  xchar_t *name;

  g_return_val_if_fail (host_and_port != NULL, NULL);

  port = NULL;
  if (host_and_port[0] == '[')
    /* escaped host part (to allow, eg. "[2001:db8::1]:888") */
    {
      const xchar_t *end;

      end = strchr (host_and_port, ']');
      if (end == NULL)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                       _("Hostname “%s” contains “[” but not “]”"), host_and_port);
          return NULL;
        }

      if (end[1] == '\0')
        port = NULL;
      else if (end[1] == ':')
        port = &end[2];
      else
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                       "The ']' character (in hostname '%s') must come at the"
                       " end or be immediately followed by ':' and a port",
                       host_and_port);
          return NULL;
        }

      name = g_strndup (host_and_port + 1, end - host_and_port - 1);
    }

  else if ((port = strchr (host_and_port, ':')))
    /* string has a ':' in it */
    {
      /* skip ':' */
      port++;

      if (strchr (port, ':'))
        /* more than one ':' in string */
        {
          /* this is actually an unescaped IPv6 address */
          name = g_strdup (host_and_port);
          port = NULL;
        }
      else
        name = g_strndup (host_and_port, port - host_and_port - 1);
    }

  else
    /* plain hostname, no port */
    name = g_strdup (host_and_port);

  if (port != NULL)
    {
      if (port[0] == '\0')
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                       "If a ':' character is given, it must be followed by a "
                       "port (in hostname '%s').", host_and_port);
          g_free (name);
          return NULL;
        }

      else if ('0' <= port[0] && port[0] <= '9')
        {
          char *end;
          long value;

          value = strtol (port, &end, 10);
          if (*end != '\0' || value < 0 || value > G_MAXUINT16)
            {
              g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                           "Invalid numeric port '%s' specified in hostname '%s'",
                           port, host_and_port);
              g_free (name);
              return NULL;
            }

          portnum = value;
        }

      else
        {
          struct servent *entry;

          entry = getservbyname (port, "tcp");
          if (entry == NULL)
            {
              g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                           "Unknown service '%s' specified in hostname '%s'",
                           port, host_and_port);
#ifdef HAVE_ENDSERVENT
              endservent ();
#endif
              g_free (name);
              return NULL;
            }

          portnum = g_ntohs (entry->s_port);

#ifdef HAVE_ENDSERVENT
          endservent ();
#endif
        }
    }
  else
    {
      /* No port in host_and_port */
      portnum = default_port;
    }

  connectable = g_network_address_new (name, portnum);
  g_free (name);

  return connectable;
}

/**
 * g_network_address_parse_uri:
 * @uri: the hostname and optionally a port
 * @default_port: The default port if none is found in the URI
 * @error: a pointer to a #xerror_t, or %NULL
 *
 * Creates a new #GSocketConnectable for connecting to the given
 * @uri. May fail and return %NULL in case parsing @uri fails.
 *
 * Using this rather than g_network_address_new() or
 * g_network_address_parse() allows #GSocketClient to determine
 * when to use application-specific proxy protocols.
 *
 * Returns: (transfer full) (type GNetworkAddress): the new
 *   #GNetworkAddress, or %NULL on error
 *
 * Since: 2.26
 */
GSocketConnectable *
g_network_address_parse_uri (const xchar_t  *uri,
    			     guint16       default_port,
			     xerror_t      **error)
{
  GSocketConnectable *conn = NULL;
  xchar_t *scheme = NULL;
  xchar_t *hostname = NULL;
  xint_t port;

  if (!g_uri_split_network (uri, G_URI_FLAGS_NONE,
                            &scheme, &hostname, &port, NULL))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                   "Invalid URI ‘%s’", uri);
      return NULL;
    }

  if (port <= 0)
    port = default_port;

  conn = g_object_new (XTYPE_NETWORK_ADDRESS,
                       "hostname", hostname,
                       "port", (xuint_t) port,
                       "scheme", scheme,
                       NULL);
  g_free (scheme);
  g_free (hostname);

  return conn;
}

/**
 * g_network_address_get_hostname:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's hostname. This might be either UTF-8 or ASCII-encoded,
 * depending on what @addr was created with.
 *
 * Returns: @addr's hostname
 *
 * Since: 2.22
 */
const xchar_t *
g_network_address_get_hostname (GNetworkAddress *addr)
{
  g_return_val_if_fail (X_IS_NETWORK_ADDRESS (addr), NULL);

  return addr->priv->hostname;
}

/**
 * g_network_address_get_port:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's port number
 *
 * Returns: @addr's port (which may be 0)
 *
 * Since: 2.22
 */
guint16
g_network_address_get_port (GNetworkAddress *addr)
{
  g_return_val_if_fail (X_IS_NETWORK_ADDRESS (addr), 0);

  return addr->priv->port;
}

/**
 * g_network_address_get_scheme:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's scheme
 *
 * Returns: (nullable): @addr's scheme (%NULL if not built from URI)
 *
 * Since: 2.26
 */
const xchar_t *
g_network_address_get_scheme (GNetworkAddress *addr)
{
  g_return_val_if_fail (X_IS_NETWORK_ADDRESS (addr), NULL);

  return addr->priv->scheme;
}

#define XTYPE_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (_g_network_address_address_enumerator_get_type ())
#define G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR(obj) (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_NETWORK_ADDRESS_ADDRESS_ENUMERATOR, GNetworkAddressAddressEnumerator))

typedef enum {
  RESOLVE_STATE_NONE = 0,
  RESOLVE_STATE_WAITING_ON_IPV4 = 1 << 0,
  RESOLVE_STATE_WAITING_ON_IPV6 = 1 << 1,
} ResolveState;

typedef struct {
  GSocketAddressEnumerator parent_instance;

  GNetworkAddress *addr; /* (owned) */
  xlist_t *addresses; /* (owned) (nullable) */
  xlist_t *current_item; /* (unowned) (nullable) */
  GTask *queued_task; /* (owned) (nullable) */
  GTask *waiting_task; /* (owned) (nullable) */
  xerror_t *last_error; /* (owned) (nullable) */
  GSource *wait_source; /* (owned) (nullable) */
  GMainContext *context; /* (owned) (nullable) */
  ResolveState state;
} GNetworkAddressAddressEnumerator;

typedef struct {
  GSocketAddressEnumeratorClass parent_class;

} GNetworkAddressAddressEnumeratorClass;

static xtype_t _g_network_address_address_enumerator_get_type (void);
G_DEFINE_TYPE (GNetworkAddressAddressEnumerator, _g_network_address_address_enumerator, XTYPE_SOCKET_ADDRESS_ENUMERATOR)

static void
g_network_address_address_enumerator_finalize (xobject_t *object)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (object);

  if (addr_enum->wait_source)
    {
      g_source_destroy (addr_enum->wait_source);
      g_clear_pointer (&addr_enum->wait_source, g_source_unref);
    }
  g_clear_object (&addr_enum->queued_task);
  g_clear_object (&addr_enum->waiting_task);
  g_clear_error (&addr_enum->last_error);
  g_object_unref (addr_enum->addr);
  g_clear_pointer (&addr_enum->context, g_main_context_unref);
  g_list_free_full (addr_enum->addresses, g_object_unref);

  G_OBJECT_CLASS (_g_network_address_address_enumerator_parent_class)->finalize (object);
}

static inline xsocket_family_t
get_address_family (GInetSocketAddress *address)
{
  return xinet_address_get_family (g_inet_socket_address_get_address (address));
}

static void
list_split_families (xlist_t  *list,
                     xlist_t **out_ipv4,
                     xlist_t **out_ipv6)
{
  g_assert (out_ipv4);
  g_assert (out_ipv6);

  while (list)
    {
      xsocket_family_t family = get_address_family (list->data);
      switch (family)
        {
          case XSOCKET_FAMILY_IPV4:
            *out_ipv4 = g_list_prepend (*out_ipv4, list->data);
            break;
          case XSOCKET_FAMILY_IPV6:
            *out_ipv6 = g_list_prepend (*out_ipv6, list->data);
            break;
          case XSOCKET_FAMILY_INVALID:
          case XSOCKET_FAMILY_UNIX:
            g_assert_not_reached ();
        }

      list = g_list_next (list);
    }

  *out_ipv4 = g_list_reverse (*out_ipv4);
  *out_ipv6 = g_list_reverse (*out_ipv6);
}

static xlist_t *
list_interleave_families (xlist_t *list1,
                          xlist_t *list2)
{
  xlist_t *interleaved = NULL;

  while (list1 || list2)
    {
      if (list1)
        {
          interleaved = g_list_append (interleaved, list1->data);
          list1 = g_list_delete_link (list1, list1);
        }
      if (list2)
        {
          interleaved = g_list_append (interleaved, list2->data);
          list2 = g_list_delete_link (list2, list2);
        }
    }

  return interleaved;
}

/* list_copy_interleaved:
 * @list: (transfer container): List to copy
 *
 * Does a shallow copy of a list with address families interleaved.
 *
 * For example:
 *   Input: [ipv6, ipv6, ipv4, ipv4]
 *   Output: [ipv6, ipv4, ipv6, ipv4]
 *
 * Returns: (transfer container): A new list
 */
static xlist_t *
list_copy_interleaved (xlist_t *list)
{
  xlist_t *ipv4 = NULL, *ipv6 = NULL;

  list_split_families (list, &ipv4, &ipv6);
  return list_interleave_families (ipv6, ipv4);
}

/* list_concat_interleaved:
 * @parent_list: (transfer container): Already existing list
 * @current_item: (transfer container): Item after which to resort
 * @new_list: (transfer container): New list to be interleaved and concatenated
 *
 * This differs from g_list_concat() + list_copy_interleaved() in that it sorts
 * items in the previous list starting from @current_item and concats the results
 * to @parent_list.
 *
 * Returns: (transfer container): New start of list
 */
static xlist_t *
list_concat_interleaved (xlist_t *parent_list,
                         xlist_t *current_item,
                         xlist_t *new_list)
{
  xlist_t *ipv4 = NULL, *ipv6 = NULL, *interleaved, *trailing = NULL;
  xsocket_family_t last_family = XSOCKET_FAMILY_IPV4; /* Default to starting with ipv6 */

  if (current_item)
    {
      last_family = get_address_family (current_item->data);

      /* Unused addresses will get removed, resorted, then readded */
      trailing = g_list_next (current_item);
      current_item->next = NULL;
    }

  list_split_families (trailing, &ipv4, &ipv6);
  list_split_families (new_list, &ipv4, &ipv6);
  g_list_free (new_list);

  if (trailing)
    g_list_free (trailing);

  if (last_family == XSOCKET_FAMILY_IPV4)
    interleaved = list_interleave_families (ipv6, ipv4);
  else
    interleaved = list_interleave_families (ipv4, ipv6);

  return g_list_concat (parent_list, interleaved);
}

static void
maybe_update_address_cache (GNetworkAddressAddressEnumerator *addr_enum,
                            GResolver                        *resolver)
{
  xlist_t *addresses, *p;

  /* Only cache complete results */
  if (addr_enum->state & RESOLVE_STATE_WAITING_ON_IPV4 || addr_enum->state & RESOLVE_STATE_WAITING_ON_IPV6)
    return;

  /* The enumerators list will not necessarily be fully sorted */
  addresses = list_copy_interleaved (addr_enum->addresses);
  for (p = addresses; p; p = p->next)
    g_object_ref (p->data);

  g_network_address_set_cached_addresses (addr_enum->addr, g_steal_pointer (&addresses), g_resolver_get_serial (resolver));
}

static void
g_network_address_address_enumerator_add_addresses (GNetworkAddressAddressEnumerator *addr_enum,
                                                    xlist_t                            *addresses,
                                                    GResolver                        *resolver)
{
  xlist_t *new_addresses = inet_addresses_to_inet_socket_addresses (addr_enum->addr, addresses);

  if (addr_enum->addresses == NULL)
    addr_enum->addresses = g_steal_pointer (&new_addresses);
  else
    addr_enum->addresses = list_concat_interleaved (addr_enum->addresses, addr_enum->current_item, g_steal_pointer (&new_addresses));

  maybe_update_address_cache (addr_enum, resolver);
}

static xpointer_t
copy_object (gconstpointer src,
             xpointer_t      user_data)
{
  return g_object_ref (G_OBJECT (src));
}

static xsocket_address_t *
init_and_query_next_address (GNetworkAddressAddressEnumerator *addr_enum)
{
  xlist_t *next_item;

  if (addr_enum->addresses == NULL)
    addr_enum->addresses = g_list_copy_deep (addr_enum->addr->priv->cached_sockaddrs,
                                             copy_object, NULL);

  /* We always want to look at the next item at call time to get the latest results.
     That means that sometimes ->next is NULL this call but is valid next call.
   */
  if (addr_enum->current_item == NULL)
    next_item = addr_enum->current_item = addr_enum->addresses;
  else
    next_item = g_list_next (addr_enum->current_item);

  if (next_item)
    {
      addr_enum->current_item = next_item;
      return g_object_ref (addr_enum->current_item->data);
    }
  else
    return NULL;
}

static xsocket_address_t *
g_network_address_address_enumerator_next (GSocketAddressEnumerator  *enumerator,
                                           xcancellable_t              *cancellable,
                                           xerror_t                   **error)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (enumerator);

  if (addr_enum->addresses == NULL)
    {
      GNetworkAddress *addr = addr_enum->addr;
      GResolver *resolver = g_resolver_get_default ();
      gint64 serial = g_resolver_get_serial (resolver);

      if (addr->priv->resolver_serial != 0 &&
          addr->priv->resolver_serial != serial)
        {
          /* Resolver has reloaded, discard cached addresses */
          g_list_free_full (addr->priv->cached_sockaddrs, g_object_unref);
          addr->priv->cached_sockaddrs = NULL;
        }

      if (!addr->priv->cached_sockaddrs)
        g_network_address_parse_sockaddr (addr);
      if (!addr->priv->cached_sockaddrs)
        {
          xlist_t *addresses;

          addresses = g_resolver_lookup_by_name (resolver,
                                                 addr->priv->hostname,
                                                 cancellable, error);
          if (!addresses)
            {
              g_object_unref (resolver);
              return NULL;
            }

          g_network_address_set_cached_addresses (addr, g_steal_pointer (&addresses), serial);
        }

      g_object_unref (resolver);
    }

  return init_and_query_next_address (addr_enum);
}

static void
complete_queued_task (GNetworkAddressAddressEnumerator *addr_enum,
                      GTask                            *task,
                      xerror_t                           *error)
{
  if (error)
    g_task_return_error (task, error);
  else
    {
      xsocket_address_t *sockaddr = init_and_query_next_address (addr_enum);
      g_task_return_pointer (task, g_steal_pointer (&sockaddr), g_object_unref);
    }
  g_object_unref (task);
}

static int
on_address_timeout (xpointer_t user_data)
{
  GNetworkAddressAddressEnumerator *addr_enum = user_data;

  /* Upon completion it may get unref'd by the owner */
  g_object_ref (addr_enum);

  if (addr_enum->queued_task != NULL)
      complete_queued_task (addr_enum, g_steal_pointer (&addr_enum->queued_task),
                            g_steal_pointer (&addr_enum->last_error));
  else if (addr_enum->waiting_task != NULL)
      complete_queued_task (addr_enum, g_steal_pointer (&addr_enum->waiting_task),
                            NULL);

  g_clear_pointer (&addr_enum->wait_source, g_source_unref);
  g_object_unref (addr_enum);

  return G_SOURCE_REMOVE;
}

static void
got_ipv6_addresses (xobject_t      *source_object,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  GNetworkAddressAddressEnumerator *addr_enum = user_data;
  GResolver *resolver = G_RESOLVER (source_object);
  xlist_t *addresses;
  xerror_t *error = NULL;

  addr_enum->state ^= RESOLVE_STATE_WAITING_ON_IPV6;

  addresses = g_resolver_lookup_by_name_with_flags_finish (resolver, result, &error);
  if (!error)
    g_network_address_address_enumerator_add_addresses (addr_enum, g_steal_pointer (&addresses), resolver);
  else
    g_debug ("IPv6 DNS error: %s", error->message);

  /* If ipv4 was first and waiting on us it can stop waiting */
  if (addr_enum->wait_source)
    {
      g_source_destroy (addr_enum->wait_source);
      g_clear_pointer (&addr_enum->wait_source, g_source_unref);
    }

  /* If we got an error before ipv4 then let its response handle it.
   * If we get ipv6 response first or error second then
   * immediately complete the task.
   */
  if (error != NULL && !addr_enum->last_error && (addr_enum->state & RESOLVE_STATE_WAITING_ON_IPV4))
    {
      /* ipv6 lookup failed, but ipv4 is still outstanding.  wait. */
      addr_enum->last_error = g_steal_pointer (&error);
    }
  else if (addr_enum->waiting_task != NULL)
    {
      complete_queued_task (addr_enum, g_steal_pointer (&addr_enum->waiting_task), NULL);
    }
  else if (addr_enum->queued_task != NULL)
    {
      xerror_t *task_error = NULL;

      /* If both errored just use the ipv6 one,
         but if ipv6 errored and ipv4 didn't we don't error */
      if (error != NULL && addr_enum->last_error)
          task_error = g_steal_pointer (&error);

      g_clear_error (&addr_enum->last_error);
      complete_queued_task (addr_enum, g_steal_pointer (&addr_enum->queued_task),
                            g_steal_pointer (&task_error));
    }

  g_clear_error (&error);
  g_object_unref (addr_enum);
}

static void
got_ipv4_addresses (xobject_t      *source_object,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  GNetworkAddressAddressEnumerator *addr_enum = user_data;
  GResolver *resolver = G_RESOLVER (source_object);
  xlist_t *addresses;
  xerror_t *error = NULL;

  addr_enum->state ^= RESOLVE_STATE_WAITING_ON_IPV4;

  addresses = g_resolver_lookup_by_name_with_flags_finish (resolver, result, &error);
  if (!error)
    g_network_address_address_enumerator_add_addresses (addr_enum, g_steal_pointer (&addresses), resolver);
  else
    g_debug ("IPv4 DNS error: %s", error->message);

  if (addr_enum->wait_source)
    {
      g_source_destroy (addr_enum->wait_source);
      g_clear_pointer (&addr_enum->wait_source, g_source_unref);
    }

  /* If ipv6 already came in and errored then we return.
   * If ipv6 returned successfully then we don't need to do anything unless
   * another enumeration was waiting on us.
   * If ipv6 hasn't come we should wait a short while for it as RFC 8305 suggests.
   */
  if (addr_enum->last_error)
    {
      g_assert (addr_enum->queued_task);
      g_clear_error (&addr_enum->last_error);
      complete_queued_task (addr_enum, g_steal_pointer (&addr_enum->queued_task),
                            g_steal_pointer (&error));
    }
  else if (addr_enum->waiting_task != NULL)
    {
      complete_queued_task (addr_enum, g_steal_pointer (&addr_enum->waiting_task), NULL);
    }
  else if (addr_enum->queued_task != NULL)
    {
      addr_enum->last_error = g_steal_pointer (&error);
      addr_enum->wait_source = g_timeout_source_new (HAPPY_EYEBALLS_RESOLUTION_DELAY_MS);
      g_source_set_callback (addr_enum->wait_source,
                             on_address_timeout,
                             addr_enum, NULL);
      g_source_attach (addr_enum->wait_source, addr_enum->context);
    }

  g_clear_error (&error);
  g_object_unref (addr_enum);
}

static void
g_network_address_address_enumerator_next_async (GSocketAddressEnumerator  *enumerator,
                                                 xcancellable_t              *cancellable,
                                                 xasync_ready_callback_t        callback,
                                                 xpointer_t                   user_data)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (enumerator);
  xsocket_address_t *sockaddr;
  GTask *task;

  task = g_task_new (addr_enum, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_network_address_address_enumerator_next_async);

  if (addr_enum->addresses == NULL && addr_enum->state == RESOLVE_STATE_NONE)
    {
      GNetworkAddress *addr = addr_enum->addr;
      GResolver *resolver = g_resolver_get_default ();
      gint64 serial = g_resolver_get_serial (resolver);

      if (addr->priv->resolver_serial != 0 &&
          addr->priv->resolver_serial != serial)
        {
          /* Resolver has reloaded, discard cached addresses */
          g_list_free_full (addr->priv->cached_sockaddrs, g_object_unref);
          addr->priv->cached_sockaddrs = NULL;
        }

      if (addr->priv->cached_sockaddrs == NULL)
        {
          if (g_network_address_parse_sockaddr (addr))
            complete_queued_task (addr_enum, task, NULL);
          else
            {
              /* It does not make sense for this to be called multiple
               * times before the initial callback has been called */
              g_assert (addr_enum->queued_task == NULL);

              addr_enum->state = RESOLVE_STATE_WAITING_ON_IPV4 | RESOLVE_STATE_WAITING_ON_IPV6;
              addr_enum->queued_task = g_steal_pointer (&task);
              /* Look up in parallel as per RFC 8305 */
              g_resolver_lookup_by_name_with_flags_async (resolver,
                                                          addr->priv->hostname,
                                                          G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY,
                                                          cancellable,
                                                          got_ipv6_addresses, g_object_ref (addr_enum));
              g_resolver_lookup_by_name_with_flags_async (resolver,
                                                          addr->priv->hostname,
                                                          G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY,
                                                          cancellable,
                                                          got_ipv4_addresses, g_object_ref (addr_enum));
            }
          g_object_unref (resolver);
          return;
        }

      g_object_unref (resolver);
    }

  sockaddr = init_and_query_next_address (addr_enum);
  if (sockaddr == NULL && (addr_enum->state & RESOLVE_STATE_WAITING_ON_IPV4 ||
                           addr_enum->state & RESOLVE_STATE_WAITING_ON_IPV6))
    {
      addr_enum->waiting_task = task;
    }
  else
    {
      g_task_return_pointer (task, sockaddr, g_object_unref);
      g_object_unref (task);
    }
}

static xsocket_address_t *
g_network_address_address_enumerator_next_finish (GSocketAddressEnumerator  *enumerator,
                                                  xasync_result_t              *result,
                                                  xerror_t                   **error)
{
  g_return_val_if_fail (g_task_is_valid (result, enumerator), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
_g_network_address_address_enumerator_init (GNetworkAddressAddressEnumerator *enumerator)
{
  enumerator->context = g_main_context_ref_thread_default ();
}

static void
_g_network_address_address_enumerator_class_init (GNetworkAddressAddressEnumeratorClass *addrenum_class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (addrenum_class);
  GSocketAddressEnumeratorClass *enumerator_class =
    XSOCKET_ADDRESS_ENUMERATOR_CLASS (addrenum_class);

  enumerator_class->next = g_network_address_address_enumerator_next;
  enumerator_class->next_async = g_network_address_address_enumerator_next_async;
  enumerator_class->next_finish = g_network_address_address_enumerator_next_finish;
  object_class->finalize = g_network_address_address_enumerator_finalize;
}

static GSocketAddressEnumerator *
g_network_address_connectable_enumerate (GSocketConnectable *connectable)
{
  GNetworkAddressAddressEnumerator *addr_enum;

  addr_enum = g_object_new (XTYPE_NETWORK_ADDRESS_ADDRESS_ENUMERATOR, NULL);
  addr_enum->addr = g_object_ref (G_NETWORK_ADDRESS (connectable));

  return (GSocketAddressEnumerator *)addr_enum;
}

static GSocketAddressEnumerator *
g_network_address_connectable_proxy_enumerate (GSocketConnectable *connectable)
{
  GNetworkAddress *self = G_NETWORK_ADDRESS (connectable);
  GSocketAddressEnumerator *proxy_enum;
  xchar_t *uri;

  uri = g_uri_join (G_URI_FLAGS_NONE,
                    self->priv->scheme ? self->priv->scheme : "none",
                    NULL,
                    self->priv->hostname,
                    self->priv->port,
                    "",
                    NULL,
                    NULL);

  proxy_enum = g_object_new (XTYPE_PROXY_ADDRESS_ENUMERATOR,
                             "connectable", connectable,
      	       	       	     "uri", uri,
      	       	       	     NULL);

  g_free (uri);

  return proxy_enum;
}

static xchar_t *
g_network_address_connectable_to_string (GSocketConnectable *connectable)
{
  GNetworkAddress *addr;
  const xchar_t *scheme;
  guint16 port;
  GString *out;  /* owned */

  addr = G_NETWORK_ADDRESS (connectable);
  out = g_string_new ("");

  scheme = g_network_address_get_scheme (addr);
  if (scheme != NULL)
    g_string_append_printf (out, "%s:", scheme);

  g_string_append (out, g_network_address_get_hostname (addr));

  port = g_network_address_get_port (addr);
  if (port != 0)
    g_string_append_printf (out, ":%u", port);

  return g_string_free (out, FALSE);
}
