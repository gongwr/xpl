/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 */

#include <config.h>

#include <string.h>

#include <glib.h>

#include "ginetaddress.h"
#include "gioenums.h"
#include "gioenumtypes.h"
#include "glibintl.h"
#include "gnetworkingprivate.h"

struct _GInetAddressPrivate
{
  xsocket_family_t family;
  union {
    struct in_addr ipv4;
#ifdef HAVE_IPV6
    struct in6_addr ipv6;
#endif
  } addr;
};

/**
 * SECTION:ginetaddress
 * @short_description: An IPv4/IPv6 address
 * @include: gio/gio.h
 *
 * #xinet_address_t represents an IPv4 or IPv6 internet address. Use
 * g_resolver_lookup_by_name() or g_resolver_lookup_by_name_async() to
 * look up the #xinet_address_t for a hostname. Use
 * g_resolver_lookup_by_address() or
 * g_resolver_lookup_by_address_async() to look up the hostname for a
 * #xinet_address_t.
 *
 * To actually connect to a remote host, you will need a
 * #xinet_socket_address_t (which includes a #xinet_address_t as well as a
 * port number).
 */

/**
 * xinet_address_t:
 *
 * An IPv4 or IPv6 internet address.
 */

G_DEFINE_TYPE_WITH_CODE (xinet_address, g_inet_address, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xinet_address_t)
			 g_networking_init ();)

enum
{
  PROP_0,
  PROP_FAMILY,
  PROP_BYTES,
  PROP_IS_ANY,
  PROP_IS_LOOPBACK,
  PROP_IS_LINK_LOCAL,
  PROP_IS_SITE_LOCAL,
  PROP_IS_MULTICAST,
  PROP_IS_MC_GLOBAL,
  PROP_IS_MC_LINK_LOCAL,
  PROP_IS_MC_NODE_LOCAL,
  PROP_IS_MC_ORG_LOCAL,
  PROP_IS_MC_SITE_LOCAL,
};

static void
xinet_address_set_property (xobject_t      *object,
			     xuint_t         prop_id,
			     const xvalue_t *value,
			     xparam_spec_t   *pspec)
{
  xinet_address_t *address = G_INET_ADDRESS (object);

  switch (prop_id)
    {
    case PROP_FAMILY:
      address->priv->family = xvalue_get_enum (value);
      break;

    case PROP_BYTES:
#ifdef HAVE_IPV6
      memcpy (&address->priv->addr, xvalue_get_pointer (value),
	      address->priv->family == AF_INET ?
	      sizeof (address->priv->addr.ipv4) :
	      sizeof (address->priv->addr.ipv6));
#else
      g_assert (address->priv->family == AF_INET);
      memcpy (&address->priv->addr, xvalue_get_pointer (value),
              sizeof (address->priv->addr.ipv4));
#endif
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
xinet_address_get_property (xobject_t    *object,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  xinet_address_t *address = G_INET_ADDRESS (object);

  switch (prop_id)
    {
    case PROP_FAMILY:
      xvalue_set_enum (value, address->priv->family);
      break;

    case PROP_BYTES:
      xvalue_set_pointer (value, &address->priv->addr);
      break;

    case PROP_IS_ANY:
      xvalue_set_boolean (value, xinet_address_get_is_any (address));
      break;

    case PROP_IS_LOOPBACK:
      xvalue_set_boolean (value, xinet_address_get_is_loopback (address));
      break;

    case PROP_IS_LINK_LOCAL:
      xvalue_set_boolean (value, xinet_address_get_is_link_local (address));
      break;

    case PROP_IS_SITE_LOCAL:
      xvalue_set_boolean (value, xinet_address_get_is_site_local (address));
      break;

    case PROP_IS_MULTICAST:
      xvalue_set_boolean (value, xinet_address_get_is_multicast (address));
      break;

    case PROP_IS_MC_GLOBAL:
      xvalue_set_boolean (value, xinet_address_get_is_mc_global (address));
      break;

    case PROP_IS_MC_LINK_LOCAL:
      xvalue_set_boolean (value, xinet_address_get_is_mc_link_local (address));
      break;

    case PROP_IS_MC_NODE_LOCAL:
      xvalue_set_boolean (value, xinet_address_get_is_mc_node_local (address));
      break;

    case PROP_IS_MC_ORG_LOCAL:
      xvalue_set_boolean (value, xinet_address_get_is_mc_org_local (address));
      break;

    case PROP_IS_MC_SITE_LOCAL:
      xvalue_set_boolean (value, xinet_address_get_is_mc_site_local (address));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xinet_address_class_init (GInetAddressClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = xinet_address_set_property;
  gobject_class->get_property = xinet_address_get_property;

  xobject_class_install_property (gobject_class, PROP_FAMILY,
                                   g_param_spec_enum ("family",
						      P_("Address family"),
						      P_("The address family (IPv4 or IPv6)"),
						      XTYPE_SOCKET_FAMILY,
						      XSOCKET_FAMILY_INVALID,
						      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class, PROP_BYTES,
                                   g_param_spec_pointer ("bytes",
							 P_("Bytes"),
							 P_("The raw address data"),
							 G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-any:
   *
   * Whether this is the "any" address for its family.
   * See xinet_address_get_is_any().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_ANY,
                                   g_param_spec_boolean ("is-any",
                                                         P_("Is any"),
                                                         P_("Whether this is the \"any\" address for its family"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-link-local:
   *
   * Whether this is a link-local address.
   * See xinet_address_get_is_link_local().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_LINK_LOCAL,
                                   g_param_spec_boolean ("is-link-local",
                                                         P_("Is link-local"),
                                                         P_("Whether this is a link-local address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-loopback:
   *
   * Whether this is the loopback address for its family.
   * See xinet_address_get_is_loopback().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_LOOPBACK,
                                   g_param_spec_boolean ("is-loopback",
                                                         P_("Is loopback"),
                                                         P_("Whether this is the loopback address for its family"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-site-local:
   *
   * Whether this is a site-local address.
   * See xinet_address_get_is_loopback().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_SITE_LOCAL,
                                   g_param_spec_boolean ("is-site-local",
                                                         P_("Is site-local"),
                                                         P_("Whether this is a site-local address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-multicast:
   *
   * Whether this is a multicast address.
   * See xinet_address_get_is_multicast().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_MULTICAST,
                                   g_param_spec_boolean ("is-multicast",
                                                         P_("Is multicast"),
                                                         P_("Whether this is a multicast address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-mc-global:
   *
   * Whether this is a global multicast address.
   * See xinet_address_get_is_mc_global().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_MC_GLOBAL,
                                   g_param_spec_boolean ("is-mc-global",
                                                         P_("Is multicast global"),
                                                         P_("Whether this is a global multicast address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));


  /**
   * xinet_address_t:is-mc-link-local:
   *
   * Whether this is a link-local multicast address.
   * See xinet_address_get_is_mc_link_local().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_MC_LINK_LOCAL,
                                   g_param_spec_boolean ("is-mc-link-local",
                                                         P_("Is multicast link-local"),
                                                         P_("Whether this is a link-local multicast address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-mc-node-local:
   *
   * Whether this is a node-local multicast address.
   * See xinet_address_get_is_mc_node_local().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_MC_NODE_LOCAL,
                                   g_param_spec_boolean ("is-mc-node-local",
                                                         P_("Is multicast node-local"),
                                                         P_("Whether this is a node-local multicast address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-mc-org-local:
   *
   * Whether this is an organization-local multicast address.
   * See xinet_address_get_is_mc_org_local().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_MC_ORG_LOCAL,
                                   g_param_spec_boolean ("is-mc-org-local",
                                                         P_("Is multicast org-local"),
                                                         P_("Whether this is an organization-local multicast address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xinet_address_t:is-mc-site-local:
   *
   * Whether this is a site-local multicast address.
   * See xinet_address_get_is_mc_site_local().
   *
   * Since: 2.22
   */
  xobject_class_install_property (gobject_class, PROP_IS_MC_SITE_LOCAL,
                                   g_param_spec_boolean ("is-mc-site-local",
                                                         P_("Is multicast site-local"),
                                                         P_("Whether this is a site-local multicast address"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));
}

static void
xinet_address_init (xinet_address_t *address)
{
  address->priv = xinet_address_get_instance_private (address);
}

/**
 * xinet_address_new_from_string:
 * @string: a string representation of an IP address
 *
 * Parses @string as an IP address and creates a new #xinet_address_t.
 *
 * Returns: (nullable) (transfer full): a new #xinet_address_t corresponding
 * to @string, or %NULL if @string could not be parsed.
 *     Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xinet_address_t *
xinet_address_new_from_string (const xchar_t *string)
{
  struct in_addr in_addr;
#ifdef HAVE_IPV6
  struct in6_addr in6_addr;
#endif

  g_return_val_if_fail (string != NULL, NULL);

  /* If this xinet_address_t is the first networking-related object to be
   * created, then we won't have called g_networking_init() yet at
   * this point.
   */
  g_networking_init ();

  if (inet_pton (AF_INET, string, &in_addr) > 0)
    return xinet_address_new_from_bytes ((xuint8_t *)&in_addr, AF_INET);
#ifdef HAVE_IPV6
  else if (inet_pton (AF_INET6, string, &in6_addr) > 0)
    return xinet_address_new_from_bytes ((xuint8_t *)&in6_addr, AF_INET6);
#endif

  return NULL;
}

#define XINET_ADDRESS_FAMILY_IS_VALID(family) ((family) == AF_INET || (family) == AF_INET6)

/**
 * xinet_address_new_from_bytes:
 * @bytes: (array) (element-type xuint8_t): raw address data
 * @family: the address family of @bytes
 *
 * Creates a new #xinet_address_t from the given @family and @bytes.
 * @bytes should be 4 bytes for %XSOCKET_FAMILY_IPV4 and 16 bytes for
 * %XSOCKET_FAMILY_IPV6.
 *
 * Returns: a new #xinet_address_t corresponding to @family and @bytes.
 *     Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xinet_address_t *
xinet_address_new_from_bytes (const xuint8_t         *bytes,
			       xsocket_family_t  family)
{
  g_return_val_if_fail (XINET_ADDRESS_FAMILY_IS_VALID (family), NULL);

  return xobject_new (XTYPE_INET_ADDRESS,
		       "family", family,
		       "bytes", bytes,
		       NULL);
}

/**
 * xinet_address_new_loopback:
 * @family: the address family
 *
 * Creates a #xinet_address_t for the loopback address for @family.
 *
 * Returns: a new #xinet_address_t corresponding to the loopback address
 * for @family.
 *     Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xinet_address_t *
xinet_address_new_loopback (xsocket_family_t family)
{
  g_return_val_if_fail (XINET_ADDRESS_FAMILY_IS_VALID (family), NULL);

  if (family == AF_INET)
    {
      xuint8_t addr[4] = {127, 0, 0, 1};

      return xinet_address_new_from_bytes (addr, family);
    }
  else
#ifdef HAVE_IPV6
    return xinet_address_new_from_bytes (in6addr_loopback.s6_addr, family);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_new_any:
 * @family: the address family
 *
 * Creates a #xinet_address_t for the "any" address (unassigned/"don't
 * care") for @family.
 *
 * Returns: a new #xinet_address_t corresponding to the "any" address
 * for @family.
 *     Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xinet_address_t *
xinet_address_new_any (xsocket_family_t family)
{
  g_return_val_if_fail (XINET_ADDRESS_FAMILY_IS_VALID (family), NULL);

  if (family == AF_INET)
    {
      xuint8_t addr[4] = {0, 0, 0, 0};

      return xinet_address_new_from_bytes (addr, family);
    }
  else
#ifdef HAVE_IPV6
    return xinet_address_new_from_bytes (in6addr_any.s6_addr, family);
#else
    g_assert_not_reached ();
#endif
}


/**
 * xinet_address_to_string:
 * @address: a #xinet_address_t
 *
 * Converts @address to string form.
 *
 * Returns: a representation of @address as a string, which should be
 * freed after use.
 *
 * Since: 2.22
 */
xchar_t *
xinet_address_to_string (xinet_address_t *address)
{
  xchar_t buffer[INET6_ADDRSTRLEN];

  g_return_val_if_fail (X_IS_INET_ADDRESS (address), NULL);

  if (address->priv->family == AF_INET)
    {
      inet_ntop (AF_INET, &address->priv->addr.ipv4, buffer, sizeof (buffer));
      return xstrdup (buffer);
    }
  else
    {
#ifdef HAVE_IPV6
      inet_ntop (AF_INET6, &address->priv->addr.ipv6, buffer, sizeof (buffer));
      return xstrdup (buffer);
#else
      g_assert_not_reached ();
#endif
    }
}

/**
 * xinet_address_to_bytes: (skip)
 * @address: a #xinet_address_t
 *
 * Gets the raw binary address data from @address.
 *
 * Returns: a pointer to an internal array of the bytes in @address,
 * which should not be modified, stored, or freed. The size of this
 * array can be gotten with xinet_address_get_native_size().
 *
 * Since: 2.22
 */
const xuint8_t *
xinet_address_to_bytes (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), NULL);

  return (xuint8_t *)&address->priv->addr;
}

/**
 * xinet_address_get_native_size:
 * @address: a #xinet_address_t
 *
 * Gets the size of the native raw binary address for @address. This
 * is the size of the data that you get from xinet_address_to_bytes().
 *
 * Returns: the number of bytes used for the native version of @address.
 *
 * Since: 2.22
 */
xsize_t
xinet_address_get_native_size (xinet_address_t *address)
{
  if (address->priv->family == AF_INET)
    return sizeof (address->priv->addr.ipv4);
#ifdef HAVE_IPV6
  return sizeof (address->priv->addr.ipv6);
#else
  g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_family:
 * @address: a #xinet_address_t
 *
 * Gets @address's family
 *
 * Returns: @address's family
 *
 * Since: 2.22
 */
xsocket_family_t
xinet_address_get_family (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  return address->priv->family;
}

/**
 * xinet_address_get_is_any:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is the "any" address for its family.
 *
 * Returns: %TRUE if @address is the "any" address for its family.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_any (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      xuint32_t addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      return addr4 == INADDR_ANY;
    }
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_UNSPECIFIED (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_loopback:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is the loopback address for its family.
 *
 * Returns: %TRUE if @address is the loopback address for its family.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_loopback (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      xuint32_t addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      /* 127.0.0.0/8 */
      return ((addr4 & 0xff000000) == 0x7f000000);
    }
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_LOOPBACK (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_link_local:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a link-local address (that is, if it
 * identifies a host on a local network that is not connected to the
 * Internet).
 *
 * Returns: %TRUE if @address is a link-local address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_link_local (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      xuint32_t addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      /* 169.254.0.0/16 */
      return ((addr4 & 0xffff0000) == 0xa9fe0000);
    }
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_LINKLOCAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_site_local:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a site-local address such as 10.0.0.1
 * (that is, the address identifies a host on a local network that can
 * not be reached directly from the Internet, but which may have
 * outgoing Internet connectivity via a NAT or firewall).
 *
 * Returns: %TRUE if @address is a site-local address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_site_local (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      xuint32_t addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      /* 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16 */
      return ((addr4 & 0xff000000) == 0x0a000000 ||
	      (addr4 & 0xfff00000) == 0xac100000 ||
	      (addr4 & 0xffff0000) == 0xc0a80000);
    }
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_SITELOCAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_multicast:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a multicast address.
 *
 * Returns: %TRUE if @address is a multicast address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_multicast (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      xuint32_t addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      return IN_MULTICAST (addr4);
    }
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_MULTICAST (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_mc_global:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a global multicast address.
 *
 * Returns: %TRUE if @address is a global multicast address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_mc_global (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_MC_GLOBAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_mc_link_local:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a link-local multicast address.
 *
 * Returns: %TRUE if @address is a link-local multicast address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_mc_link_local (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_MC_LINKLOCAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_mc_node_local:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a node-local multicast address.
 *
 * Returns: %TRUE if @address is a node-local multicast address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_mc_node_local (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_MC_NODELOCAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_mc_org_local:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is an organization-local multicast address.
 *
 * Returns: %TRUE if @address is an organization-local multicast address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_mc_org_local  (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_MC_ORGLOCAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_get_is_mc_site_local:
 * @address: a #xinet_address_t
 *
 * Tests whether @address is a site-local multicast address.
 *
 * Returns: %TRUE if @address is a site-local multicast address.
 *
 * Since: 2.22
 */
xboolean_t
xinet_address_get_is_mc_site_local (xinet_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
#ifdef HAVE_IPV6
    return IN6_IS_ADDR_MC_SITELOCAL (&address->priv->addr.ipv6);
#else
    g_assert_not_reached ();
#endif
}

/**
 * xinet_address_equal:
 * @address: A #xinet_address_t.
 * @other_address: Another #xinet_address_t.
 *
 * Checks if two #xinet_address_t instances are equal, e.g. the same address.
 *
 * Returns: %TRUE if @address and @other_address are equal, %FALSE otherwise.
 *
 * Since: 2.30
 */
xboolean_t
xinet_address_equal (xinet_address_t *address,
                      xinet_address_t *other_address)
{
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), FALSE);
  g_return_val_if_fail (X_IS_INET_ADDRESS (other_address), FALSE);

  if (xinet_address_get_family (address) != xinet_address_get_family (other_address))
    return FALSE;

  if (memcmp (xinet_address_to_bytes (address),
              xinet_address_to_bytes (other_address),
              xinet_address_get_native_size (address)) != 0)
    return FALSE;

  return TRUE;
}
