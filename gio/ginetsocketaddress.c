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
#include <glib.h>
#include <string.h>

#include "ginetsocketaddress.h"
#include "ginetaddress.h"
#include "gnetworkingprivate.h"
#include "gsocketconnectable.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:ginetsocketaddress
 * @short_description: Internet xsocket_address_t
 * @include: gio/gio.h
 *
 * An IPv4 or IPv6 socket address; that is, the combination of a
 * #xinet_address_t and a port number.
 */

/**
 * xinet_socket_address_t:
 *
 * An IPv4 or IPv6 socket address, corresponding to a struct
 * sockaddr_in or struct sockaddr_in6.
 */

struct _GInetSocketAddressPrivate
{
  xinet_address_t *address;
  xuint16_t       port;
  xuint32_t       flowinfo;
  xuint32_t       scope_id;
};

static void   g_inet_socket_address_connectable_iface_init (xsocket_connectable_iface_t *iface);
static xchar_t *g_inet_socket_address_connectable_to_string  (xsocket_connectable_t      *connectable);

G_DEFINE_TYPE_WITH_CODE (xinet_socket_address, g_inet_socket_address, XTYPE_SOCKET_ADDRESS,
                         G_ADD_PRIVATE (xinet_socket_address_t)
                         G_IMPLEMENT_INTERFACE (XTYPE_SOCKET_CONNECTABLE,
                                                g_inet_socket_address_connectable_iface_init))

enum {
  PROP_0,
  PROP_ADDRESS,
  PROP_PORT,
  PROP_FLOWINFO,
  PROP_SCOPE_ID
};

static void
g_inet_socket_address_dispose (xobject_t *object)
{
  xinet_socket_address_t *address = G_INET_SOCKET_ADDRESS (object);

  g_clear_object (&(address->priv->address));

  G_OBJECT_CLASS (g_inet_socket_address_parent_class)->dispose (object);
}

static void
g_inet_socket_address_get_property (xobject_t    *object,
                                    xuint_t       prop_id,
                                    xvalue_t     *value,
                                    xparam_spec_t *pspec)
{
  xinet_socket_address_t *address = G_INET_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_ADDRESS:
        xvalue_set_object (value, address->priv->address);
        break;

      case PROP_PORT:
        xvalue_set_uint (value, address->priv->port);
        break;

      case PROP_FLOWINFO:
	g_return_if_fail (xinet_address_get_family (address->priv->address) == XSOCKET_FAMILY_IPV6);
        xvalue_set_uint (value, address->priv->flowinfo);
        break;

      case PROP_SCOPE_ID:
	g_return_if_fail (xinet_address_get_family (address->priv->address) == XSOCKET_FAMILY_IPV6);
        xvalue_set_uint (value, address->priv->scope_id);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_inet_socket_address_set_property (xobject_t      *object,
                                    xuint_t         prop_id,
                                    const xvalue_t *value,
                                    xparam_spec_t   *pspec)
{
  xinet_socket_address_t *address = G_INET_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_ADDRESS:
        address->priv->address = xobject_ref (xvalue_get_object (value));
        break;

      case PROP_PORT:
        address->priv->port = (xuint16_t) xvalue_get_uint (value);
        break;

      case PROP_FLOWINFO:
	/* We can't test that address->priv->address is IPv6 here,
	 * since this property might get set before PROP_ADDRESS.
	 */
        address->priv->flowinfo = xvalue_get_uint (value);
        break;

      case PROP_SCOPE_ID:
        address->priv->scope_id = xvalue_get_uint (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static xsocket_family_t
g_inet_socket_address_get_family (xsocket_address_t *address)
{
  xinet_socket_address_t *addr;

  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), 0);

  addr = G_INET_SOCKET_ADDRESS (address);

  return xinet_address_get_family (addr->priv->address);
}

static xssize_t
g_inet_socket_address_get_native_size (xsocket_address_t *address)
{
  xinet_socket_address_t *addr;
  xsocket_family_t family;

  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), 0);

  addr = G_INET_SOCKET_ADDRESS (address);
  family = xinet_address_get_family (addr->priv->address);

  if (family == AF_INET)
    return sizeof (struct sockaddr_in);
  else if (family == AF_INET6)
    return sizeof (struct sockaddr_in6);
  else
    return -1;
}

static xboolean_t
g_inet_socket_address_to_native (xsocket_address_t  *address,
                                 xpointer_t         dest,
				 xsize_t            destlen,
				 xerror_t         **error)
{
  xinet_socket_address_t *addr;
  xsocket_family_t family;

  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), FALSE);

  addr = G_INET_SOCKET_ADDRESS (address);
  family = xinet_address_get_family (addr->priv->address);

  if (family == AF_INET)
    {
      struct sockaddr_in *sock = (struct sockaddr_in *) dest;

      if (destlen < sizeof (*sock))
	{
	  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
			       _("Not enough space for socket address"));
	  return FALSE;
	}

      sock->sin_family = AF_INET;
      sock->sin_port = g_htons (addr->priv->port);
      memcpy (&(sock->sin_addr.s_addr), xinet_address_to_bytes (addr->priv->address), sizeof (sock->sin_addr));
      memset (sock->sin_zero, 0, sizeof (sock->sin_zero));
      return TRUE;
    }
  else if (family == AF_INET6)
    {
      struct sockaddr_in6 *sock = (struct sockaddr_in6 *) dest;

      if (destlen < sizeof (*sock))
	{
	  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
			       _("Not enough space for socket address"));
	  return FALSE;
	}

      memset (sock, 0, sizeof (*sock));
      sock->sin6_family = AF_INET6;
      sock->sin6_port = g_htons (addr->priv->port);
      sock->sin6_flowinfo = addr->priv->flowinfo;
      sock->sin6_scope_id = addr->priv->scope_id;
      memcpy (&(sock->sin6_addr.s6_addr), xinet_address_to_bytes (addr->priv->address), sizeof (sock->sin6_addr));
      return TRUE;
    }
  else
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			   _("Unsupported socket address"));
      return FALSE;
    }
}

static void
g_inet_socket_address_class_init (GInetSocketAddressClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  GSocketAddressClass *gsocketaddress_class = XSOCKET_ADDRESS_CLASS (klass);

  gobject_class->dispose = g_inet_socket_address_dispose;
  gobject_class->set_property = g_inet_socket_address_set_property;
  gobject_class->get_property = g_inet_socket_address_get_property;

  gsocketaddress_class->get_family = g_inet_socket_address_get_family;
  gsocketaddress_class->to_native = g_inet_socket_address_to_native;
  gsocketaddress_class->get_native_size = g_inet_socket_address_get_native_size;

  xobject_class_install_property (gobject_class, PROP_ADDRESS,
                                   g_param_spec_object ("address",
                                                        P_("Address"),
                                                        P_("The address"),
                                                        XTYPE_INET_ADDRESS,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class, PROP_PORT,
                                   g_param_spec_uint ("port",
                                                      P_("Port"),
                                                      P_("The port"),
                                                      0,
                                                      65535,
                                                      0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));

  /**
   * xinet_socket_address_t:flowinfo:
   *
   * The `sin6_flowinfo` field, for IPv6 addresses.
   *
   * Since: 2.32
   */
  xobject_class_install_property (gobject_class, PROP_FLOWINFO,
                                   g_param_spec_uint ("flowinfo",
                                                      P_("Flow info"),
                                                      P_("IPv6 flow info"),
                                                      0,
                                                      G_MAXUINT32,
                                                      0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));

  /**
   * xinet_socket_address_t:scope_id:
   *
   * The `sin6_scope_id` field, for IPv6 addresses.
   *
   * Since: 2.32
   */
  xobject_class_install_property (gobject_class, PROP_SCOPE_ID,
                                   g_param_spec_uint ("scope-id",
                                                      P_("Scope ID"),
                                                      P_("IPv6 scope ID"),
                                                      0,
                                                      G_MAXUINT32,
                                                      0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));
}

static void
g_inet_socket_address_connectable_iface_init (xsocket_connectable_iface_t *iface)
{
  xsocket_connectable_iface_t *parent_iface = xtype_interface_peek_parent (iface);

  iface->enumerate = parent_iface->enumerate;
  iface->proxy_enumerate = parent_iface->proxy_enumerate;
  iface->to_string = g_inet_socket_address_connectable_to_string;
}

static xchar_t *
g_inet_socket_address_connectable_to_string (xsocket_connectable_t *connectable)
{
  xinet_socket_address_t *sa;
  xinet_address_t *a;
  xchar_t *a_string;
  xstring_t *out;
  xuint16_t port;

  sa = G_INET_SOCKET_ADDRESS (connectable);
  a = g_inet_socket_address_get_address (sa);
  out = xstring_new ("");

  /* Address. */
  a_string = xinet_address_to_string (a);
  xstring_append (out, a_string);
  g_free (a_string);

  /* Scope ID (IPv6 only). */
  if (xinet_address_get_family (a) == XSOCKET_FAMILY_IPV6 &&
      g_inet_socket_address_get_scope_id (sa) != 0)
    {
      xstring_append_printf (out, "%%%u",
                              g_inet_socket_address_get_scope_id (sa));
    }

  /* Port. */
  port = g_inet_socket_address_get_port (sa);
  if (port != 0)
    {
      /* Disambiguate ports from IPv6 addresses using square brackets. */
      if (xinet_address_get_family (a) == XSOCKET_FAMILY_IPV6)
        {
          xstring_prepend (out, "[");
          xstring_append (out, "]");
        }

      xstring_append_printf (out, ":%u", port);
    }

  return xstring_free (out, FALSE);
}

static void
g_inet_socket_address_init (xinet_socket_address_t *address)
{
  address->priv = g_inet_socket_address_get_instance_private (address);
}

/**
 * g_inet_socket_address_new:
 * @address: a #xinet_address_t
 * @port: a port number
 *
 * Creates a new #xinet_socket_address_t for @address and @port.
 *
 * Returns: a new #xinet_socket_address_t
 *
 * Since: 2.22
 */
xsocket_address_t *
g_inet_socket_address_new (xinet_address_t *address,
                           xuint16_t       port)
{
  return xobject_new (XTYPE_INET_SOCKET_ADDRESS,
		       "address", address,
		       "port", port,
		       NULL);
}

/**
 * g_inet_socket_address_new_from_string:
 * @address: the string form of an IP address
 * @port: a port number
 *
 * Creates a new #xinet_socket_address_t for @address and @port.
 *
 * If @address is an IPv6 address, it can also contain a scope ID
 * (separated from the address by a `%`).
 *
 * Returns: (nullable) (transfer full): a new #xinet_socket_address_t,
 * or %NULL if @address cannot be parsed.
 *
 * Since: 2.40
 */
xsocket_address_t *
g_inet_socket_address_new_from_string (const char *address,
                                       xuint_t       port)
{
  static struct addrinfo *hints, hints_struct;
  xsocket_address_t *saddr;
  xinet_address_t *iaddr;
  struct addrinfo *res;
  xint_t status;

  if (strchr (address, ':'))
    {
      /* IPv6 address (or it's invalid). We use getaddrinfo() because
       * it will handle parsing a scope_id as well.
       */

      if (G_UNLIKELY (g_once_init_enter (&hints)))
        {
          hints_struct.ai_family = AF_UNSPEC;
          hints_struct.ai_socktype = SOCK_STREAM;
          hints_struct.ai_protocol = 0;
          hints_struct.ai_flags = AI_NUMERICHOST;
          g_once_init_leave (&hints, &hints_struct);
        }

      status = getaddrinfo (address, NULL, hints, &res);
      if (status != 0)
        return NULL;

      if (res->ai_family == AF_INET6 &&
          res->ai_addrlen == sizeof (struct sockaddr_in6))
        {
          ((struct sockaddr_in6 *)res->ai_addr)->sin6_port = g_htons (port);
          saddr = xsocket_address_new_from_native (res->ai_addr, res->ai_addrlen);
        }
      else
        saddr = NULL;

      freeaddrinfo (res);
    }
  else
    {
      /* IPv4 (or invalid). We don't want to use getaddrinfo() here,
       * because it accepts the stupid "IPv4 numbers-and-dots
       * notation" addresses that are never used for anything except
       * phishing. Since we don't have to worry about scope IDs for
       * IPv4, we can just use xinet_address_new_from_string().
       */
      iaddr = xinet_address_new_from_string (address);
      if (!iaddr)
        return NULL;

      g_warn_if_fail (xinet_address_get_family (iaddr) == XSOCKET_FAMILY_IPV4);

      saddr = g_inet_socket_address_new (iaddr, port);
      xobject_unref (iaddr);
    }

  return saddr;
}

/**
 * g_inet_socket_address_get_address:
 * @address: a #xinet_socket_address_t
 *
 * Gets @address's #xinet_address_t.
 *
 * Returns: (transfer none): the #xinet_address_t for @address, which must be
 * xobject_ref()'d if it will be stored
 *
 * Since: 2.22
 */
xinet_address_t *
g_inet_socket_address_get_address (xinet_socket_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), NULL);

  return address->priv->address;
}

/**
 * g_inet_socket_address_get_port:
 * @address: a #xinet_socket_address_t
 *
 * Gets @address's port.
 *
 * Returns: the port for @address
 *
 * Since: 2.22
 */
xuint16_t
g_inet_socket_address_get_port (xinet_socket_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), 0);

  return address->priv->port;
}


/**
 * g_inet_socket_address_get_flowinfo:
 * @address: a %XSOCKET_FAMILY_IPV6 #xinet_socket_address_t
 *
 * Gets the `sin6_flowinfo` field from @address,
 * which must be an IPv6 address.
 *
 * Returns: the flowinfo field
 *
 * Since: 2.32
 */
xuint32_t
g_inet_socket_address_get_flowinfo (xinet_socket_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), 0);
  g_return_val_if_fail (xinet_address_get_family (address->priv->address) == XSOCKET_FAMILY_IPV6, 0);

  return address->priv->flowinfo;
}

/**
 * g_inet_socket_address_get_scope_id:
 * @address: a %XSOCKET_FAMILY_IPV6 #xinet_address_t
 *
 * Gets the `sin6_scope_id` field from @address,
 * which must be an IPv6 address.
 *
 * Returns: the scope id field
 *
 * Since: 2.32
 */
xuint32_t
g_inet_socket_address_get_scope_id (xinet_socket_address_t *address)
{
  g_return_val_if_fail (X_IS_INET_SOCKET_ADDRESS (address), 0);
  g_return_val_if_fail (xinet_address_get_family (address->priv->address) == XSOCKET_FAMILY_IPV6, 0);

  return address->priv->scope_id;
}
