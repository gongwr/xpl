/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora, Ltd.
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
 * Authors: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include <config.h>
#include <glib.h>
#include <string.h>

#include <gio/gsocketaddress.h>

#include "gproxyaddress.h"
#include "glibintl.h"

/**
 * SECTION:gproxyaddress
 * @short_description: An internet address with proxy information
 * @include: gio/gio.h
 *
 * Support for proxied #xinet_socket_address_t.
 */

/**
 * xproxy_address_t:
 *
 * A #xinet_socket_address_t representing a connection via a proxy server
 *
 * Since: 2.26
 **/

/**
 * GProxyAddressClass:
 *
 * Class structure for #xproxy_address_t.
 *
 * Since: 2.26
 **/

enum
{
  PROP_0,
  PROP_PROTOCOL,
  PROP_DESTINATION_PROTOCOL,
  PROP_DESTINATION_HOSTNAME,
  PROP_DESTINATION_PORT,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_URI
};

struct _GProxyAddressPrivate
{
  xchar_t 	 *uri;
  xchar_t 	 *protocol;
  xchar_t		 *username;
  xchar_t		 *password;
  xchar_t 	 *dest_protocol;
  xchar_t 	 *dest_hostname;
  xuint16_t 	  dest_port;
};

G_DEFINE_TYPE_WITH_PRIVATE (xproxy_address_t, xproxy_address, XTYPE_INET_SOCKET_ADDRESS)

static void
xproxy_address_finalize (xobject_t *object)
{
  xproxy_address_t *proxy = G_PROXY_ADDRESS (object);

  g_free (proxy->priv->uri);
  g_free (proxy->priv->protocol);
  g_free (proxy->priv->username);
  g_free (proxy->priv->password);
  g_free (proxy->priv->dest_hostname);
  g_free (proxy->priv->dest_protocol);

  XOBJECT_CLASS (xproxy_address_parent_class)->finalize (object);
}

static void
xproxy_address_set_property (xobject_t      *object,
			      xuint_t         prop_id,
			      const xvalue_t *value,
			      xparam_spec_t   *pspec)
{
  xproxy_address_t *proxy = G_PROXY_ADDRESS (object);

  switch (prop_id)
    {
    case PROP_PROTOCOL:
      g_free (proxy->priv->protocol);
      proxy->priv->protocol = xvalue_dup_string (value);
      break;

    case PROP_DESTINATION_PROTOCOL:
      g_free (proxy->priv->dest_protocol);
      proxy->priv->dest_protocol = xvalue_dup_string (value);
      break;

    case PROP_DESTINATION_HOSTNAME:
      g_free (proxy->priv->dest_hostname);
      proxy->priv->dest_hostname = xvalue_dup_string (value);
      break;

    case PROP_DESTINATION_PORT:
      proxy->priv->dest_port = xvalue_get_uint (value);
      break;

    case PROP_USERNAME:
      g_free (proxy->priv->username);
      proxy->priv->username = xvalue_dup_string (value);
      break;

    case PROP_PASSWORD:
      g_free (proxy->priv->password);
      proxy->priv->password = xvalue_dup_string (value);
      break;

    case PROP_URI:
      g_free (proxy->priv->uri);
      proxy->priv->uri = xvalue_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xproxy_address_get_property (xobject_t    *object,
			      xuint_t       prop_id,
			      xvalue_t     *value,
			      xparam_spec_t *pspec)
{
  xproxy_address_t *proxy = G_PROXY_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_PROTOCOL:
	xvalue_set_string (value, proxy->priv->protocol);
	break;

      case PROP_DESTINATION_PROTOCOL:
	xvalue_set_string (value, proxy->priv->dest_protocol);
	break;

      case PROP_DESTINATION_HOSTNAME:
	xvalue_set_string (value, proxy->priv->dest_hostname);
	break;

      case PROP_DESTINATION_PORT:
	xvalue_set_uint (value, proxy->priv->dest_port);
	break;

      case PROP_USERNAME:
	xvalue_set_string (value, proxy->priv->username);
	break;

      case PROP_PASSWORD:
	xvalue_set_string (value, proxy->priv->password);
	break;

      case PROP_URI:
	xvalue_set_string (value, proxy->priv->uri);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xproxy_address_class_init (GProxyAddressClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = xproxy_address_finalize;
  xobject_class->set_property = xproxy_address_set_property;
  xobject_class->get_property = xproxy_address_get_property;

  xobject_class_install_property (xobject_class,
				   PROP_PROTOCOL,
				   xparam_spec_string ("protocol",
						       P_("Protocol"),
						       P_("The proxy protocol"),
						       NULL,
						       XPARAM_READWRITE |
						       XPARAM_CONSTRUCT_ONLY |
						       XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class,
				   PROP_USERNAME,
				   xparam_spec_string ("username",
						       P_("Username"),
						       P_("The proxy username"),
						       NULL,
						       XPARAM_READWRITE |
						       XPARAM_CONSTRUCT_ONLY |
						       XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class,
				   PROP_PASSWORD,
				   xparam_spec_string ("password",
						       P_("Password"),
						       P_("The proxy password"),
						       NULL,
						       XPARAM_READWRITE |
						       XPARAM_CONSTRUCT_ONLY |
						       XPARAM_STATIC_STRINGS));

  /**
   * xproxy_address_t:destination-protocol:
   *
   * The protocol being spoke to the destination host, or %NULL if
   * the #xproxy_address_t doesn't know.
   *
   * Since: 2.34
   */
  xobject_class_install_property (xobject_class,
				   PROP_DESTINATION_PROTOCOL,
				   xparam_spec_string ("destination-protocol",
						       P_("Destination Protocol"),
						       P_("The proxy destination protocol"),
						       NULL,
						       XPARAM_READWRITE |
						       XPARAM_CONSTRUCT_ONLY |
						       XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class,
				   PROP_DESTINATION_HOSTNAME,
				   xparam_spec_string ("destination-hostname",
						       P_("Destination Hostname"),
						       P_("The proxy destination hostname"),
						       NULL,
						       XPARAM_READWRITE |
						       XPARAM_CONSTRUCT_ONLY |
						       XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class,
				   PROP_DESTINATION_PORT,
				   xparam_spec_uint ("destination-port",
						      P_("Destination Port"),
						      P_("The proxy destination port"),
						      0, 65535, 0,
						      XPARAM_READWRITE |
						      XPARAM_CONSTRUCT_ONLY |
						      XPARAM_STATIC_STRINGS));

  /**
   * xproxy_address_t:uri:
   *
   * The URI string that the proxy was constructed from (or %NULL
   * if the creator didn't specify this).
   *
   * Since: 2.34
   */
  xobject_class_install_property (xobject_class,
				   PROP_URI,
				   xparam_spec_string ("uri",
							P_("URI"),
							P_("The proxy’s URI"),
							NULL,
							XPARAM_READWRITE |
							XPARAM_CONSTRUCT_ONLY |
							XPARAM_STATIC_STRINGS));
}

static void
xproxy_address_init (xproxy_address_t *proxy)
{
  proxy->priv = xproxy_address_get_instance_private (proxy);
  proxy->priv->protocol = NULL;
  proxy->priv->username = NULL;
  proxy->priv->password = NULL;
  proxy->priv->dest_hostname = NULL;
  proxy->priv->dest_port = 0;
}

/**
 * xproxy_address_new:
 * @inetaddr: The proxy server #xinet_address_t.
 * @port: The proxy server port.
 * @protocol: The proxy protocol to support, in lower case (e.g. socks, http).
 * @dest_hostname: The destination hostname the proxy should tunnel to.
 * @dest_port: The destination port to tunnel to.
 * @username: (nullable): The username to authenticate to the proxy server
 *     (or %NULL).
 * @password: (nullable): The password to authenticate to the proxy server
 *     (or %NULL).
 *
 * Creates a new #xproxy_address_t for @inetaddr with @protocol that should
 * tunnel through @dest_hostname and @dest_port.
 *
 * (Note that this method doesn't set the #xproxy_address_t:uri or
 * #xproxy_address_t:destination-protocol fields; use xobject_new()
 * directly if you want to set those.)
 *
 * Returns: a new #xproxy_address_t
 *
 * Since: 2.26
 */
xsocket_address_t *
xproxy_address_new (xinet_address_t  *inetaddr,
		     xuint16_t        port,
		     const xchar_t   *protocol,
		     const xchar_t   *dest_hostname,
		     xuint16_t        dest_port,
		     const xchar_t   *username,
		     const xchar_t   *password)
{
  return xobject_new (XTYPE_PROXY_ADDRESS,
		       "address", inetaddr,
		       "port", port,
		       "protocol", protocol,
		       "destination-hostname", dest_hostname,
		       "destination-port", dest_port,
		       "username", username,
		       "password", password,
		       NULL);
}


/**
 * xproxy_address_get_protocol:
 * @proxy: a #xproxy_address_t
 *
 * Gets @proxy's protocol. eg, "socks" or "http"
 *
 * Returns: the @proxy's protocol
 *
 * Since: 2.26
 */
const xchar_t *
xproxy_address_get_protocol (xproxy_address_t *proxy)
{
  return proxy->priv->protocol;
}

/**
 * xproxy_address_get_destination_protocol:
 * @proxy: a #xproxy_address_t
 *
 * Gets the protocol that is being spoken to the destination
 * server; eg, "http" or "ftp".
 *
 * Returns: the @proxy's destination protocol
 *
 * Since: 2.34
 */
const xchar_t *
xproxy_address_get_destination_protocol (xproxy_address_t *proxy)
{
  return proxy->priv->dest_protocol;
}

/**
 * xproxy_address_get_destination_hostname:
 * @proxy: a #xproxy_address_t
 *
 * Gets @proxy's destination hostname; that is, the name of the host
 * that will be connected to via the proxy, not the name of the proxy
 * itself.
 *
 * Returns: the @proxy's destination hostname
 *
 * Since: 2.26
 */
const xchar_t *
xproxy_address_get_destination_hostname (xproxy_address_t *proxy)
{
  return proxy->priv->dest_hostname;
}

/**
 * xproxy_address_get_destination_port:
 * @proxy: a #xproxy_address_t
 *
 * Gets @proxy's destination port; that is, the port on the
 * destination host that will be connected to via the proxy, not the
 * port number of the proxy itself.
 *
 * Returns: the @proxy's destination port
 *
 * Since: 2.26
 */
xuint16_t
xproxy_address_get_destination_port (xproxy_address_t *proxy)
{
  return proxy->priv->dest_port;
}

/**
 * xproxy_address_get_username:
 * @proxy: a #xproxy_address_t
 *
 * Gets @proxy's username.
 *
 * Returns: (nullable): the @proxy's username
 *
 * Since: 2.26
 */
const xchar_t *
xproxy_address_get_username (xproxy_address_t *proxy)
{
  return proxy->priv->username;
}

/**
 * xproxy_address_get_password:
 * @proxy: a #xproxy_address_t
 *
 * Gets @proxy's password.
 *
 * Returns: (nullable): the @proxy's password
 *
 * Since: 2.26
 */
const xchar_t *
xproxy_address_get_password (xproxy_address_t *proxy)
{
  return proxy->priv->password;
}


/**
 * xproxy_address_get_uri:
 * @proxy: a #xproxy_address_t
 *
 * Gets the proxy URI that @proxy was constructed from.
 *
 * Returns: (nullable): the @proxy's URI, or %NULL if unknown
 *
 * Since: 2.34
 */
const xchar_t *
xproxy_address_get_uri (xproxy_address_t *proxy)
{
  return proxy->priv->uri;
}
