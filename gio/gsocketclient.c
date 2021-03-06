/*  GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008, 2009 codethink
 * Copyright © 2009 Red Hat, Inc
 * Copyright © 2018 Igalia S.L.
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include "gsocketclient.h"

#ifndef G_OS_WIN32
#include <netinet/in.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <gio/gioenumtypes.h>
#include <gio/gsocketaddressenumerator.h>
#include <gio/gsocketconnectable.h>
#include <gio/gsocketconnection.h>
#include <gio/gioprivate.h>
#include <gio/gproxyaddressenumerator.h>
#include <gio/gproxyaddress.h>
#include <gio/gtask.h>
#include <gio/gcancellable.h>
#include <gio/gioerror.h>
#include <gio/gsocket.h>
#include <gio/gnetworkaddress.h>
#include <gio/gnetworking.h>
#include <gio/gnetworkservice.h>
#include <gio/gproxy.h>
#include <gio/gproxyresolver.h>
#include <gio/gsocketaddress.h>
#include <gio/gtcpconnection.h>
#include <gio/gtcpwrapperconnection.h>
#include <gio/gtlscertificate.h>
#include <gio/gtlsclientconnection.h>
#include <gio/ginetaddress.h>
#include "glibintl.h"
#include "gmarshal-internal.h"

/* As recommended by RFC 8305 this is the time it waits
 * on a connection before starting another concurrent attempt.
 */
#define HAPPY_EYEBALLS_CONNECTION_ATTEMPT_TIMEOUT_MS 250

/**
 * SECTION:gsocketclient
 * @short_description: Helper for connecting to a network service
 * @include: gio/gio.h
 * @see_also: #xsocket_connection_t, #xsocket_listener_t
 *
 * #xsocket_client_t is a lightweight high-level utility class for connecting to
 * a network host using a connection oriented socket type.
 *
 * You create a #xsocket_client_t object, set any options you want, and then
 * call a sync or async connect operation, which returns a #xsocket_connection_t
 * subclass on success.
 *
 * The type of the #xsocket_connection_t object returned depends on the type of
 * the underlying socket that is in use. For instance, for a TCP/IP connection
 * it will be a #xtcp_connection_t.
 *
 * As #xsocket_client_t is a lightweight object, you don't need to cache it. You
 * can just create a new one any time you need one.
 *
 * Since: 2.22
 */


enum
{
  EVENT,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

enum
{
  PROP_NONE,
  PROP_FAMILY,
  PROP_TYPE,
  PROP_PROTOCOL,
  PROP_LOCAL_ADDRESS,
  PROP_TIMEOUT,
  PROP_ENABLE_PROXY,
  PROP_TLS,
  PROP_TLS_VALIDATION_FLAGS,
  PROP_PROXY_RESOLVER
};

struct _GSocketClientPrivate
{
  xsocket_family_t family;
  xsocket_type_t type;
  GSocketProtocol protocol;
  xsocket_address_t *local_address;
  xuint_t timeout;
  xboolean_t enable_proxy;
  xhashtable_t *app_proxies;
  xboolean_t tls;
  xtls_certificate_flags_t tls_validation_flags;
  xproxy_resolver_t *proxy_resolver;
};

G_DEFINE_TYPE_WITH_PRIVATE (xsocket_client_t, xsocket_client, XTYPE_OBJECT)

static xsocket_t *
create_socket (xsocket_client_t  *client,
	       xsocket_address_t *dest_address,
	       xerror_t        **error)
{
  xsocket_family_t family;
  xsocket_t *socket;

  family = client->priv->family;
  if (family == XSOCKET_FAMILY_INVALID &&
      client->priv->local_address != NULL)
    family = xsocket_address_get_family (client->priv->local_address);
  if (family == XSOCKET_FAMILY_INVALID)
    family = xsocket_address_get_family (dest_address);

  socket = xsocket_new (family,
			 client->priv->type,
			 client->priv->protocol,
			 error);
  if (socket == NULL)
    return NULL;

  if (client->priv->local_address)
    {
#ifdef IP_BIND_ADDRESS_NO_PORT
      xsocket_set_option (socket, IPPROTO_IP, IP_BIND_ADDRESS_NO_PORT, 1, NULL);
#endif

      if (!xsocket_bind (socket,
			  client->priv->local_address,
			  FALSE,
			  error))
	{
	  xobject_unref (socket);
	  return NULL;
	}
    }

  if (client->priv->timeout)
    xsocket_set_timeout (socket, client->priv->timeout);

  return socket;
}

static xboolean_t
can_use_proxy (xsocket_client_t *client)
{
  GSocketClientPrivate *priv = client->priv;

  return priv->enable_proxy
          && priv->type == XSOCKET_TYPE_STREAM;
}

static void
clarify_connect_error (xerror_t             *error,
		       xsocket_connectable_t *connectable,
		       xsocket_address_t     *address)
{
  const char *name;
  char *tmp_name = NULL;

  if (X_IS_PROXY_ADDRESS (address))
    {
      name = tmp_name = xinet_address_to_string (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (address)));

      g_prefix_error (&error, _("Could not connect to proxy server %s: "), name);
    }
  else
    {
      if (X_IS_NETWORK_ADDRESS (connectable))
	name = g_network_address_get_hostname (G_NETWORK_ADDRESS (connectable));
      else if (X_IS_NETWORK_SERVICE (connectable))
	name = g_network_service_get_domain (G_NETWORK_SERVICE (connectable));
      else if (X_IS_INET_SOCKET_ADDRESS (connectable))
	name = tmp_name = xinet_address_to_string (g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (connectable)));
      else
	name = NULL;

      if (name)
	g_prefix_error (&error, _("Could not connect to %s: "), name);
      else
	g_prefix_error (&error, _("Could not connect: "));
    }

  g_free (tmp_name);
}

static void
xsocket_client_init (xsocket_client_t *client)
{
  client->priv = xsocket_client_get_instance_private (client);
  client->priv->type = XSOCKET_TYPE_STREAM;
  client->priv->app_proxies = xhash_table_new_full (xstr_hash,
						     xstr_equal,
						     g_free,
						     NULL);
}

/**
 * xsocket_client_new:
 *
 * Creates a new #xsocket_client_t with the default options.
 *
 * Returns: a #xsocket_client_t.
 *     Free the returned object with xobject_unref().
 *
 * Since: 2.22
 */
xsocket_client_t *
xsocket_client_new (void)
{
  return xobject_new (XTYPE_SOCKET_CLIENT, NULL);
}

static void
xsocket_client_finalize (xobject_t *object)
{
  xsocket_client_t *client = XSOCKET_CLIENT (object);

  g_clear_object (&client->priv->local_address);
  g_clear_object (&client->priv->proxy_resolver);

  XOBJECT_CLASS (xsocket_client_parent_class)->finalize (object);

  xhash_table_unref (client->priv->app_proxies);
}

static void
xsocket_client_get_property (xobject_t    *object,
			      xuint_t       prop_id,
			      xvalue_t     *value,
			      xparam_spec_t *pspec)
{
  xsocket_client_t *client = XSOCKET_CLIENT (object);

  switch (prop_id)
    {
      case PROP_FAMILY:
	xvalue_set_enum (value, client->priv->family);
	break;

      case PROP_TYPE:
	xvalue_set_enum (value, client->priv->type);
	break;

      case PROP_PROTOCOL:
	xvalue_set_enum (value, client->priv->protocol);
	break;

      case PROP_LOCAL_ADDRESS:
	xvalue_set_object (value, client->priv->local_address);
	break;

      case PROP_TIMEOUT:
	xvalue_set_uint (value, client->priv->timeout);
	break;

      case PROP_ENABLE_PROXY:
	xvalue_set_boolean (value, client->priv->enable_proxy);
	break;

      case PROP_TLS:
	xvalue_set_boolean (value, xsocket_client_get_tls (client));
	break;

      case PROP_TLS_VALIDATION_FLAGS:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	xvalue_set_flags (value, xsocket_client_get_tls_validation_flags (client));
G_GNUC_END_IGNORE_DEPRECATIONS
	break;

      case PROP_PROXY_RESOLVER:
	xvalue_set_object (value, xsocket_client_get_proxy_resolver (client));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsocket_client_set_property (xobject_t      *object,
			      xuint_t         prop_id,
			      const xvalue_t *value,
			      xparam_spec_t   *pspec)
{
  xsocket_client_t *client = XSOCKET_CLIENT (object);

  switch (prop_id)
    {
    case PROP_FAMILY:
      xsocket_client_set_family (client, xvalue_get_enum (value));
      break;

    case PROP_TYPE:
      xsocket_client_set_socket_type (client, xvalue_get_enum (value));
      break;

    case PROP_PROTOCOL:
      xsocket_client_set_protocol (client, xvalue_get_enum (value));
      break;

    case PROP_LOCAL_ADDRESS:
      xsocket_client_set_local_address (client, xvalue_get_object (value));
      break;

    case PROP_TIMEOUT:
      xsocket_client_set_timeout (client, xvalue_get_uint (value));
      break;

    case PROP_ENABLE_PROXY:
      xsocket_client_set_enable_proxy (client, xvalue_get_boolean (value));
      break;

    case PROP_TLS:
      xsocket_client_set_tls (client, xvalue_get_boolean (value));
      break;

    case PROP_TLS_VALIDATION_FLAGS:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      xsocket_client_set_tls_validation_flags (client, xvalue_get_flags (value));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_PROXY_RESOLVER:
      xsocket_client_set_proxy_resolver (client, xvalue_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

/**
 * xsocket_client_get_family:
 * @client: a #xsocket_client_t.
 *
 * Gets the socket family of the socket client.
 *
 * See xsocket_client_set_family() for details.
 *
 * Returns: a #xsocket_family_t
 *
 * Since: 2.22
 */
xsocket_family_t
xsocket_client_get_family (xsocket_client_t *client)
{
  return client->priv->family;
}

/**
 * xsocket_client_set_family:
 * @client: a #xsocket_client_t.
 * @family: a #xsocket_family_t
 *
 * Sets the socket family of the socket client.
 * If this is set to something other than %XSOCKET_FAMILY_INVALID
 * then the sockets created by this object will be of the specified
 * family.
 *
 * This might be useful for instance if you want to force the local
 * connection to be an ipv4 socket, even though the address might
 * be an ipv6 mapped to ipv4 address.
 *
 * Since: 2.22
 */
void
xsocket_client_set_family (xsocket_client_t *client,
			    xsocket_family_t  family)
{
  if (client->priv->family == family)
    return;

  client->priv->family = family;
  xobject_notify (G_OBJECT (client), "family");
}

/**
 * xsocket_client_get_socket_type:
 * @client: a #xsocket_client_t.
 *
 * Gets the socket type of the socket client.
 *
 * See xsocket_client_set_socket_type() for details.
 *
 * Returns: a #xsocket_family_t
 *
 * Since: 2.22
 */
xsocket_type_t
xsocket_client_get_socket_type (xsocket_client_t *client)
{
  return client->priv->type;
}

/**
 * xsocket_client_set_socket_type:
 * @client: a #xsocket_client_t.
 * @type: a #xsocket_type_t
 *
 * Sets the socket type of the socket client.
 * The sockets created by this object will be of the specified
 * type.
 *
 * It doesn't make sense to specify a type of %XSOCKET_TYPE_DATAGRAM,
 * as xsocket_client_t is used for connection oriented services.
 *
 * Since: 2.22
 */
void
xsocket_client_set_socket_type (xsocket_client_t *client,
				 xsocket_type_t    type)
{
  if (client->priv->type == type)
    return;

  client->priv->type = type;
  xobject_notify (G_OBJECT (client), "type");
}

/**
 * xsocket_client_get_protocol:
 * @client: a #xsocket_client_t
 *
 * Gets the protocol name type of the socket client.
 *
 * See xsocket_client_set_protocol() for details.
 *
 * Returns: a #GSocketProtocol
 *
 * Since: 2.22
 */
GSocketProtocol
xsocket_client_get_protocol (xsocket_client_t *client)
{
  return client->priv->protocol;
}

/**
 * xsocket_client_set_protocol:
 * @client: a #xsocket_client_t.
 * @protocol: a #GSocketProtocol
 *
 * Sets the protocol of the socket client.
 * The sockets created by this object will use of the specified
 * protocol.
 *
 * If @protocol is %XSOCKET_PROTOCOL_DEFAULT that means to use the default
 * protocol for the socket family and type.
 *
 * Since: 2.22
 */
void
xsocket_client_set_protocol (xsocket_client_t   *client,
			      GSocketProtocol  protocol)
{
  if (client->priv->protocol == protocol)
    return;

  client->priv->protocol = protocol;
  xobject_notify (G_OBJECT (client), "protocol");
}

/**
 * xsocket_client_get_local_address:
 * @client: a #xsocket_client_t.
 *
 * Gets the local address of the socket client.
 *
 * See xsocket_client_set_local_address() for details.
 *
 * Returns: (nullable) (transfer none): a #xsocket_address_t or %NULL. Do not free.
 *
 * Since: 2.22
 */
xsocket_address_t *
xsocket_client_get_local_address (xsocket_client_t *client)
{
  return client->priv->local_address;
}

/**
 * xsocket_client_set_local_address:
 * @client: a #xsocket_client_t.
 * @address: (nullable): a #xsocket_address_t, or %NULL
 *
 * Sets the local address of the socket client.
 * The sockets created by this object will bound to the
 * specified address (if not %NULL) before connecting.
 *
 * This is useful if you want to ensure that the local
 * side of the connection is on a specific port, or on
 * a specific interface.
 *
 * Since: 2.22
 */
void
xsocket_client_set_local_address (xsocket_client_t  *client,
				   xsocket_address_t *address)
{
  if (address)
    xobject_ref (address);

  if (client->priv->local_address)
    {
      xobject_unref (client->priv->local_address);
    }
  client->priv->local_address = address;
  xobject_notify (G_OBJECT (client), "local-address");
}

/**
 * xsocket_client_get_timeout:
 * @client: a #xsocket_client_t
 *
 * Gets the I/O timeout time for sockets created by @client.
 *
 * See xsocket_client_set_timeout() for details.
 *
 * Returns: the timeout in seconds
 *
 * Since: 2.26
 */
xuint_t
xsocket_client_get_timeout (xsocket_client_t *client)
{
  return client->priv->timeout;
}


/**
 * xsocket_client_set_timeout:
 * @client: a #xsocket_client_t.
 * @timeout: the timeout
 *
 * Sets the I/O timeout for sockets created by @client. @timeout is a
 * time in seconds, or 0 for no timeout (the default).
 *
 * The timeout value affects the initial connection attempt as well,
 * so setting this may cause calls to xsocket_client_connect(), etc,
 * to fail with %G_IO_ERROR_TIMED_OUT.
 *
 * Since: 2.26
 */
void
xsocket_client_set_timeout (xsocket_client_t *client,
			     xuint_t          timeout)
{
  if (client->priv->timeout == timeout)
    return;

  client->priv->timeout = timeout;
  xobject_notify (G_OBJECT (client), "timeout");
}

/**
 * xsocket_client_get_enable_proxy:
 * @client: a #xsocket_client_t.
 *
 * Gets the proxy enable state; see xsocket_client_set_enable_proxy()
 *
 * Returns: whether proxying is enabled
 *
 * Since: 2.26
 */
xboolean_t
xsocket_client_get_enable_proxy (xsocket_client_t *client)
{
  return client->priv->enable_proxy;
}

/**
 * xsocket_client_set_enable_proxy:
 * @client: a #xsocket_client_t.
 * @enable: whether to enable proxies
 *
 * Sets whether or not @client attempts to make connections via a
 * proxy server. When enabled (the default), #xsocket_client_t will use a
 * #xproxy_resolver_t to determine if a proxy protocol such as SOCKS is
 * needed, and automatically do the necessary proxy negotiation.
 *
 * See also xsocket_client_set_proxy_resolver().
 *
 * Since: 2.26
 */
void
xsocket_client_set_enable_proxy (xsocket_client_t *client,
				  xboolean_t       enable)
{
  enable = !!enable;
  if (client->priv->enable_proxy == enable)
    return;

  client->priv->enable_proxy = enable;
  xobject_notify (G_OBJECT (client), "enable-proxy");
}

/**
 * xsocket_client_get_tls:
 * @client: a #xsocket_client_t.
 *
 * Gets whether @client creates TLS connections. See
 * xsocket_client_set_tls() for details.
 *
 * Returns: whether @client uses TLS
 *
 * Since: 2.28
 */
xboolean_t
xsocket_client_get_tls (xsocket_client_t *client)
{
  return client->priv->tls;
}

/**
 * xsocket_client_set_tls:
 * @client: a #xsocket_client_t.
 * @tls: whether to use TLS
 *
 * Sets whether @client creates TLS (aka SSL) connections. If @tls is
 * %TRUE, @client will wrap its connections in a #xtls_client_connection_t
 * and perform a TLS handshake when connecting.
 *
 * Note that since #xsocket_client_t must return a #xsocket_connection_t,
 * but #xtls_client_connection_t is not a #xsocket_connection_t, this
 * actually wraps the resulting #xtls_client_connection_t in a
 * #xtcp_wrapper_connection_t when returning it. You can use
 * g_tcp_wrapper_connection_get_base_io_stream() on the return value
 * to extract the #xtls_client_connection_t.
 *
 * If you need to modify the behavior of the TLS handshake (eg, by
 * setting a client-side certificate to use, or connecting to the
 * #xtls_connection_t::accept-certificate signal), you can connect to
 * @client's #xsocket_client_t::event signal and wait for it to be
 * emitted with %XSOCKET_CLIENT_TLS_HANDSHAKING, which will give you
 * a chance to see the #xtls_client_connection_t before the handshake
 * starts.
 *
 * Since: 2.28
 */
void
xsocket_client_set_tls (xsocket_client_t *client,
			 xboolean_t       tls)
{
  tls = !!tls;
  if (tls == client->priv->tls)
    return;

  client->priv->tls = tls;
  xobject_notify (G_OBJECT (client), "tls");
}

/**
 * xsocket_client_get_tls_validation_flags:
 * @client: a #xsocket_client_t.
 *
 * Gets the TLS validation flags used creating TLS connections via
 * @client.
 *
 * This function does not work as originally designed and is impossible
 * to use correctly. See #xsocket_client_t:tls-validation-flags for more
 * information.
 *
 * Returns: the TLS validation flags
 *
 * Since: 2.28
 *
 * Deprecated: 2.72: Do not attempt to ignore validation errors.
 */
xtls_certificate_flags_t
xsocket_client_get_tls_validation_flags (xsocket_client_t *client)
{
  return client->priv->tls_validation_flags;
}

/**
 * xsocket_client_set_tls_validation_flags:
 * @client: a #xsocket_client_t.
 * @flags: the validation flags
 *
 * Sets the TLS validation flags used when creating TLS connections
 * via @client. The default value is %G_TLS_CERTIFICATE_VALIDATE_ALL.
 *
 * This function does not work as originally designed and is impossible
 * to use correctly. See #xsocket_client_t:tls-validation-flags for more
 * information.
 *
 * Since: 2.28
 *
 * Deprecated: 2.72: Do not attempt to ignore validation errors.
 */
void
xsocket_client_set_tls_validation_flags (xsocket_client_t        *client,
					  xtls_certificate_flags_t  flags)
{
  if (client->priv->tls_validation_flags != flags)
    {
      client->priv->tls_validation_flags = flags;
      xobject_notify (G_OBJECT (client), "tls-validation-flags");
    }
}

/**
 * xsocket_client_get_proxy_resolver:
 * @client: a #xsocket_client_t.
 *
 * Gets the #xproxy_resolver_t being used by @client. Normally, this will
 * be the resolver returned by xproxy_resolver_get_default(), but you
 * can override it with xsocket_client_set_proxy_resolver().
 *
 * Returns: (transfer none): The #xproxy_resolver_t being used by
 *   @client.
 *
 * Since: 2.36
 */
xproxy_resolver_t *
xsocket_client_get_proxy_resolver (xsocket_client_t *client)
{
  if (client->priv->proxy_resolver)
    return client->priv->proxy_resolver;
  else
    return xproxy_resolver_get_default ();
}

/**
 * xsocket_client_set_proxy_resolver:
 * @client: a #xsocket_client_t.
 * @proxy_resolver: (nullable): a #xproxy_resolver_t, or %NULL for the
 *   default.
 *
 * Overrides the #xproxy_resolver_t used by @client. You can call this if
 * you want to use specific proxies, rather than using the system
 * default proxy settings.
 *
 * Note that whether or not the proxy resolver is actually used
 * depends on the setting of #xsocket_client_t:enable-proxy, which is not
 * changed by this function (but which is %TRUE by default)
 *
 * Since: 2.36
 */
void
xsocket_client_set_proxy_resolver (xsocket_client_t  *client,
                                    xproxy_resolver_t *proxy_resolver)
{
  /* We have to be careful to avoid calling
   * xproxy_resolver_get_default() until we're sure we need it,
   * because trying to load the default proxy resolver module will
   * break some test programs that aren't expecting it (eg,
   * tests/gsettings).
   */

  if (client->priv->proxy_resolver)
    xobject_unref (client->priv->proxy_resolver);

  client->priv->proxy_resolver = proxy_resolver;

  if (client->priv->proxy_resolver)
    xobject_ref (client->priv->proxy_resolver);
}

static void
xsocket_client_class_init (GSocketClientClass *class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (class);

  xobject_class->finalize = xsocket_client_finalize;
  xobject_class->set_property = xsocket_client_set_property;
  xobject_class->get_property = xsocket_client_get_property;

  /**
   * xsocket_client_t::event:
   * @client: the #xsocket_client_t
   * @event: the event that is occurring
   * @connectable: the #xsocket_connectable_t that @event is occurring on
   * @connection: (nullable): the current representation of the connection
   *
   * Emitted when @client's activity on @connectable changes state.
   * Among other things, this can be used to provide progress
   * information about a network connection in the UI. The meanings of
   * the different @event values are as follows:
   *
   * - %XSOCKET_CLIENT_RESOLVING: @client is about to look up @connectable
   *   in DNS. @connection will be %NULL.
   *
   * - %XSOCKET_CLIENT_RESOLVED:  @client has successfully resolved
   *   @connectable in DNS. @connection will be %NULL.
   *
   * - %XSOCKET_CLIENT_CONNECTING: @client is about to make a connection
   *   to a remote host; either a proxy server or the destination server
   *   itself. @connection is the #xsocket_connection_t, which is not yet
   *   connected.  Since GLib 2.40, you can access the remote
   *   address via xsocket_connection_get_remote_address().
   *
   * - %XSOCKET_CLIENT_CONNECTED: @client has successfully connected
   *   to a remote host. @connection is the connected #xsocket_connection_t.
   *
   * - %XSOCKET_CLIENT_PROXY_NEGOTIATING: @client is about to negotiate
   *   with a proxy to get it to connect to @connectable. @connection is
   *   the #xsocket_connection_t to the proxy server.
   *
   * - %XSOCKET_CLIENT_PROXY_NEGOTIATED: @client has negotiated a
   *   connection to @connectable through a proxy server. @connection is
   *   the stream returned from xproxy_connect(), which may or may not
   *   be a #xsocket_connection_t.
   *
   * - %XSOCKET_CLIENT_TLS_HANDSHAKING: @client is about to begin a TLS
   *   handshake. @connection is a #xtls_client_connection_t.
   *
   * - %XSOCKET_CLIENT_TLS_HANDSHAKED: @client has successfully completed
   *   the TLS handshake. @connection is a #xtls_client_connection_t.
   *
   * - %XSOCKET_CLIENT_COMPLETE: @client has either successfully connected
   *   to @connectable (in which case @connection is the #xsocket_connection_t
   *   that it will be returning to the caller) or has failed (in which
   *   case @connection is %NULL and the client is about to return an error).
   *
   * Each event except %XSOCKET_CLIENT_COMPLETE may be emitted
   * multiple times (or not at all) for a given connectable (in
   * particular, if @client ends up attempting to connect to more than
   * one address). However, if @client emits the #xsocket_client_t::event
   * signal at all for a given connectable, then it will always emit
   * it with %XSOCKET_CLIENT_COMPLETE when it is done.
   *
   * Note that there may be additional #GSocketClientEvent values in
   * the future; unrecognized @event values should be ignored.
   *
   * Since: 2.32
   */
  signals[EVENT] =
    xsignal_new (I_("event"),
		  XTYPE_FROM_CLASS (xobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GSocketClientClass, event),
		  NULL, NULL,
		  _g_cclosure_marshal_VOID__ENUM_OBJECT_OBJECT,
		  XTYPE_NONE, 3,
		  XTYPE_SOCKET_CLIENT_EVENT,
		  XTYPE_SOCKET_CONNECTABLE,
		  XTYPE_IO_STREAM);
  xsignal_set_va_marshaller (signals[EVENT],
                              XTYPE_FROM_CLASS (class),
                              _g_cclosure_marshal_VOID__ENUM_OBJECT_OBJECTv);

  xobject_class_install_property (xobject_class, PROP_FAMILY,
				   xparam_spec_enum ("family",
						      P_("Socket family"),
						      P_("The sockets address family to use for socket construction"),
						      XTYPE_SOCKET_FAMILY,
						      XSOCKET_FAMILY_INVALID,
						      XPARAM_CONSTRUCT |
                                                      XPARAM_READWRITE |
                                                      XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class, PROP_TYPE,
				   xparam_spec_enum ("type",
						      P_("Socket type"),
						      P_("The sockets type to use for socket construction"),
						      XTYPE_SOCKET_TYPE,
						      XSOCKET_TYPE_STREAM,
						      XPARAM_CONSTRUCT |
                                                      XPARAM_READWRITE |
                                                      XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class, PROP_PROTOCOL,
				   xparam_spec_enum ("protocol",
						      P_("Socket protocol"),
						      P_("The protocol to use for socket construction, or 0 for default"),
						      XTYPE_SOCKET_PROTOCOL,
						      XSOCKET_PROTOCOL_DEFAULT,
						      XPARAM_CONSTRUCT |
                                                      XPARAM_READWRITE |
                                                      XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class, PROP_LOCAL_ADDRESS,
				   xparam_spec_object ("local-address",
							P_("Local address"),
							P_("The local address constructed sockets will be bound to"),
							XTYPE_SOCKET_ADDRESS,
							XPARAM_CONSTRUCT |
                                                        XPARAM_READWRITE |
                                                        XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class, PROP_TIMEOUT,
				   xparam_spec_uint ("timeout",
						      P_("Socket timeout"),
						      P_("The I/O timeout for sockets, or 0 for none"),
						      0, G_MAXUINT, 0,
						      XPARAM_CONSTRUCT |
                                                      XPARAM_READWRITE |
                                                      XPARAM_STATIC_STRINGS));

   xobject_class_install_property (xobject_class, PROP_ENABLE_PROXY,
				    xparam_spec_boolean ("enable-proxy",
							  P_("Enable proxy"),
							  P_("Enable proxy support"),
							  TRUE,
							  XPARAM_CONSTRUCT |
							  XPARAM_READWRITE |
							  XPARAM_STATIC_STRINGS));

  xobject_class_install_property (xobject_class, PROP_TLS,
				   xparam_spec_boolean ("tls",
							 P_("TLS"),
							 P_("Whether to create TLS connections"),
							 FALSE,
							 XPARAM_CONSTRUCT |
							 XPARAM_READWRITE |
							 XPARAM_STATIC_STRINGS));

  /**
   * xsocket_client_t:tls-validation-flags:
   *
   * The TLS validation flags used when creating TLS connections. The
   * default value is %G_TLS_CERTIFICATE_VALIDATE_ALL.
   *
   * GLib guarantees that if certificate verification fails, at least one
   * flag will be set, but it does not guarantee that all possible flags
   * will be set. Accordingly, you may not safely decide to ignore any
   * particular type of error. For example, it would be incorrect to mask
   * %G_TLS_CERTIFICATE_EXPIRED if you want to allow expired certificates,
   * because this could potentially be the only error flag set even if
   * other problems exist with the certificate. Therefore, there is no
   * safe way to use this property. This is not a horrible problem,
   * though, because you should not be attempting to ignore validation
   * errors anyway. If you really must ignore TLS certificate errors,
   * connect to the #xsocket_client_t::event signal, wait for it to be
   * emitted with %XSOCKET_CLIENT_TLS_HANDSHAKING, and use that to
   * connect to #xtls_connection_t::accept-certificate.
   *
   * Deprecated: 2.72: Do not attempt to ignore validation errors.
   */
  xobject_class_install_property (xobject_class, PROP_TLS_VALIDATION_FLAGS,
				   xparam_spec_flags ("tls-validation-flags",
						       P_("TLS validation flags"),
						       P_("TLS validation flags to use"),
						       XTYPE_TLS_CERTIFICATE_FLAGS,
						       G_TLS_CERTIFICATE_VALIDATE_ALL,
						       XPARAM_CONSTRUCT |
						       XPARAM_READWRITE |
						       XPARAM_STATIC_STRINGS |
						       XPARAM_DEPRECATED));

  /**
   * xsocket_client_t:proxy-resolver:
   *
   * The proxy resolver to use
   *
   * Since: 2.36
   */
  xobject_class_install_property (xobject_class, PROP_PROXY_RESOLVER,
                                   xparam_spec_object ("proxy-resolver",
                                                        P_("Proxy resolver"),
                                                        P_("The proxy resolver to use"),
                                                        XTYPE_PROXY_RESOLVER,
                                                        XPARAM_CONSTRUCT |
                                                        XPARAM_READWRITE |
                                                        XPARAM_STATIC_STRINGS));
}

static void
xsocket_client_emit_event (xsocket_client_t       *client,
			    GSocketClientEvent   event,
			    xsocket_connectable_t  *connectable,
			    xio_stream_t           *connection)
{
  xsignal_emit (client, signals[EVENT], 0,
		 event, connectable, connection);
}

/* Originally, xsocket_client_t returned whatever error occured last. Turns
 * out this doesn't work well in practice. Consider the following case:
 * DNS returns an IPv4 and IPv6 address. First we'll connect() to the
 * IPv4 address, and say that succeeds, but TLS is enabled and the TLS
 * handshake fails. Then we try the IPv6 address and receive ENETUNREACH
 * because IPv6 isn't supported. We wind up returning NETWORK_UNREACHABLE
 * even though the address can be pinged and a TLS error would be more
 * appropriate. So instead, we now try to return the error corresponding
 * to the latest attempted GSocketClientEvent in the connection process.
 * TLS errors take precedence over proxy errors, which take precedence
 * over connect() errors, which take precedence over DNS errors.
 *
 * Note that the example above considers a sync codepath, but this is an
 * issue for the async codepath too, where events and errors may occur
 * in confusing orders.
 */
typedef struct
{
  xerror_t *tmp_error;
  xerror_t *best_error;
  GSocketClientEvent best_error_event;
} SocketClientErrorInfo;

static SocketClientErrorInfo *
socket_client_error_info_new (void)
{
  return g_new0 (SocketClientErrorInfo, 1);
}

static void
socket_client_error_info_free (SocketClientErrorInfo *info)
{
  xassert (info->tmp_error == NULL);
  g_clear_error (&info->best_error);
  g_free (info);
}

static void
consider_tmp_error (SocketClientErrorInfo *info,
                    GSocketClientEvent     event)
{
  if (info->tmp_error == NULL)
    return;

  /* If we ever add more GSocketClientEvents in the future, then we'll
   * no longer be able to use >= for this comparison, because future
   * events will compare greater than XSOCKET_CLIENT_COMPLETE. Until
   * then, this is convenient. Note XSOCKET_CLIENT_RESOLVING is 0 so we
   * need to use >= here or those errors would never be set. That means
   * if we get two errors on the same GSocketClientEvent, we wind up
   * preferring the last one, which is fine.
   */
  xassert (event <= XSOCKET_CLIENT_COMPLETE);
  if (event >= info->best_error_event)
    {
      g_clear_error (&info->best_error);
      info->best_error = info->tmp_error;
      info->tmp_error = NULL;
      info->best_error_event = event;
    }
  else
    {
      g_clear_error (&info->tmp_error);
    }
}

/**
 * xsocket_client_connect:
 * @client: a #xsocket_client_t.
 * @connectable: a #xsocket_connectable_t specifying the remote address.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting, or %NULL to ignore.
 *
 * Tries to resolve the @connectable and make a network connection to it.
 *
 * Upon a successful connection, a new #xsocket_connection_t is constructed
 * and returned.  The caller owns this new object and must drop their
 * reference to it when finished with it.
 *
 * The type of the #xsocket_connection_t object returned depends on the type of
 * the underlying socket that is used. For instance, for a TCP/IP connection
 * it will be a #xtcp_connection_t.
 *
 * The socket created will be the same family as the address that the
 * @connectable resolves to, unless family is set with xsocket_client_set_family()
 * or indirectly via xsocket_client_set_local_address(). The socket type
 * defaults to %XSOCKET_TYPE_STREAM but can be set with
 * xsocket_client_set_socket_type().
 *
 * If a local address is specified with xsocket_client_set_local_address() the
 * socket will be bound to this address before connecting.
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.22
 */
xsocket_connection_t *
xsocket_client_connect (xsocket_client_t       *client,
			 xsocket_connectable_t  *connectable,
			 xcancellable_t        *cancellable,
			 xerror_t             **error)
{
  xio_stream_t *connection = NULL;
  xsocket_address_enumerator_t *enumerator = NULL;
  SocketClientErrorInfo *error_info;
  xboolean_t ever_resolved = FALSE;

  error_info = socket_client_error_info_new ();

  if (can_use_proxy (client))
    {
      enumerator = xsocket_connectable_proxy_enumerate (connectable);
      if (client->priv->proxy_resolver &&
          X_IS_PROXY_ADDRESS_ENUMERATOR (enumerator))
        {
          xobject_set (G_OBJECT (enumerator),
                        "proxy-resolver", client->priv->proxy_resolver,
                        NULL);
        }
    }
  else
    enumerator = xsocket_connectable_enumerate (connectable);

  while (connection == NULL)
    {
      xsocket_address_t *address = NULL;
      xboolean_t application_proxy = FALSE;
      xsocket_t *socket;
      xboolean_t using_proxy;

      if (xcancellable_is_cancelled (cancellable))
	{
	  g_clear_error (&error_info->best_error);
	  xcancellable_set_error_if_cancelled (cancellable, &error_info->best_error);
	  break;
	}

      if (!ever_resolved)
	{
	  xsocket_client_emit_event (client, XSOCKET_CLIENT_RESOLVING,
				      connectable, NULL);
	}
      address = xsocket_address_enumerator_next (enumerator, cancellable,
						  &error_info->tmp_error);
      consider_tmp_error (error_info, XSOCKET_CLIENT_RESOLVING);
      if (!ever_resolved)
	{
	  xsocket_client_emit_event (client, XSOCKET_CLIENT_RESOLVED,
				      connectable, NULL);
	  ever_resolved = TRUE;
	}

      if (address == NULL)
	{
          /* Enumeration is finished. */
          xassert (&error_info->best_error != NULL);
	  break;
	}

      using_proxy = (X_IS_PROXY_ADDRESS (address) &&
		     client->priv->enable_proxy);

      socket = create_socket (client, address, &error_info->tmp_error);
      consider_tmp_error (error_info, XSOCKET_CLIENT_CONNECTING);
      if (socket == NULL)
	{
	  xobject_unref (address);
	  continue;
	}

      connection = (xio_stream_t *)xsocket_connection_factory_create_connection (socket);
      xsocket_connection_set_cached_remote_address ((xsocket_connection_t*)connection, address);
      xsocket_client_emit_event (client, XSOCKET_CLIENT_CONNECTING, connectable, connection);

      if (xsocket_connection_connect (XSOCKET_CONNECTION (connection),
				       address, cancellable, &error_info->tmp_error))
	{
          xsocket_connection_set_cached_remote_address ((xsocket_connection_t*)connection, NULL);
	  xsocket_client_emit_event (client, XSOCKET_CLIENT_CONNECTED, connectable, connection);
	}
      else
	{
	  clarify_connect_error (error_info->tmp_error, connectable, address);
          consider_tmp_error (error_info, XSOCKET_CLIENT_CONNECTING);
	  xobject_unref (connection);
	  connection = NULL;
	}

      if (connection && using_proxy)
	{
	  xproxy_address_t *proxy_addr = G_PROXY_ADDRESS (address);
	  const xchar_t *protocol;
	  xproxy_t *proxy;

	  protocol = xproxy_address_get_protocol (proxy_addr);

          /* The connection should not be anything else then TCP Connection,
           * but let's put a safety guard in case
	   */
          if (!X_IS_TCP_CONNECTION (connection))
            {
              g_critical ("Trying to proxy over non-TCP connection, this is "
                          "most likely a bug in GLib IO library.");

              g_set_error_literal (&error_info->tmp_error,
                  G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                  _("Proxying over a non-TCP connection is not supported."));
              consider_tmp_error (error_info, XSOCKET_CLIENT_PROXY_NEGOTIATING);

	      xobject_unref (connection);
	      connection = NULL;
            }
	  else if (xhash_table_contains (client->priv->app_proxies, protocol))
	    {
	      application_proxy = TRUE;
	    }
          else if ((proxy = xproxy_get_default_for_protocol (protocol)))
	    {
	      xio_stream_t *proxy_connection;

	      xsocket_client_emit_event (client, XSOCKET_CLIENT_PROXY_NEGOTIATING, connectable, connection);
	      proxy_connection = xproxy_connect (proxy,
						  connection,
						  proxy_addr,
						  cancellable,
						  &error_info->tmp_error);
	      consider_tmp_error (error_info, XSOCKET_CLIENT_PROXY_NEGOTIATING);

	      xobject_unref (connection);
	      connection = proxy_connection;
	      xobject_unref (proxy);

	      if (connection)
		xsocket_client_emit_event (client, XSOCKET_CLIENT_PROXY_NEGOTIATED, connectable, connection);
	    }
	  else
	    {
	      g_set_error (&error_info->tmp_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			   _("Proxy protocol “%s” is not supported."),
			   protocol);
	      consider_tmp_error (error_info, XSOCKET_CLIENT_PROXY_NEGOTIATING);
	      xobject_unref (connection);
	      connection = NULL;
	    }
	}

      if (!application_proxy && connection && client->priv->tls)
	{
	  xio_stream_t *tlsconn;

	  tlsconn = xtls_client_connection_new (connection, connectable, &error_info->tmp_error);
	  xobject_unref (connection);
	  connection = tlsconn;

	  if (tlsconn)
	    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	      xtls_client_connection_set_validation_flags (G_TLS_CLIENT_CONNECTION (tlsconn),
                                                            client->priv->tls_validation_flags);
G_GNUC_END_IGNORE_DEPRECATIONS
	      xsocket_client_emit_event (client, XSOCKET_CLIENT_TLS_HANDSHAKING, connectable, connection);
	      if (xtls_connection_handshake (G_TLS_CONNECTION (tlsconn),
					      cancellable, &error_info->tmp_error))
		{
		  xsocket_client_emit_event (client, XSOCKET_CLIENT_TLS_HANDSHAKED, connectable, connection);
		}
	      else
		{
		  consider_tmp_error (error_info, XSOCKET_CLIENT_TLS_HANDSHAKING);
		  xobject_unref (tlsconn);
		  connection = NULL;
		}
	    }
          else
            {
              consider_tmp_error (error_info, XSOCKET_CLIENT_TLS_HANDSHAKING);
            }
	}

      if (connection && !X_IS_SOCKET_CONNECTION (connection))
	{
	  xsocket_connection_t *wrapper_connection;

	  wrapper_connection = g_tcp_wrapper_connection_new (connection, socket);
	  xobject_unref (connection);
	  connection = (xio_stream_t *)wrapper_connection;
	}

      xobject_unref (socket);
      xobject_unref (address);
    }
  xobject_unref (enumerator);

  if (!connection)
    g_propagate_error (error, g_steal_pointer (&error_info->best_error));
  socket_client_error_info_free (error_info);

  xsocket_client_emit_event (client, XSOCKET_CLIENT_COMPLETE, connectable, connection);
  return XSOCKET_CONNECTION (connection);
}

/**
 * xsocket_client_connect_to_host:
 * @client: a #xsocket_client_t
 * @host_and_port: the name and optionally port of the host to connect to
 * @default_port: the default port to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a pointer to a #xerror_t, or %NULL
 *
 * This is a helper function for xsocket_client_connect().
 *
 * Attempts to create a TCP connection to the named host.
 *
 * @host_and_port may be in any of a number of recognized formats; an IPv6
 * address, an IPv4 address, or a domain name (in which case a DNS
 * lookup is performed).  Quoting with [] is supported for all address
 * types.  A port override may be specified in the usual way with a
 * colon.  Ports may be given as decimal numbers or symbolic names (in
 * which case an /etc/services lookup is performed).
 *
 * If no port override is given in @host_and_port then @default_port will be
 * used as the port number to connect to.
 *
 * In general, @host_and_port is expected to be provided by the user (allowing
 * them to give the hostname, and a port override if necessary) and
 * @default_port is expected to be provided by the application.
 *
 * In the case that an IP address is given, a single connection
 * attempt is made.  In the case that a name is given, multiple
 * connection attempts may be made, in turn and according to the
 * number of address records in DNS, until a connection succeeds.
 *
 * Upon a successful connection, a new #xsocket_connection_t is constructed
 * and returned.  The caller owns this new object and must drop their
 * reference to it when finished with it.
 *
 * In the event of any failure (DNS error, service not found, no hosts
 * connectable) %NULL is returned and @error (if non-%NULL) is set
 * accordingly.
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.22
 */
xsocket_connection_t *
xsocket_client_connect_to_host (xsocket_client_t  *client,
				 const xchar_t    *host_and_port,
				 xuint16_t         default_port,
				 xcancellable_t   *cancellable,
				 xerror_t        **error)
{
  xsocket_connectable_t *connectable;
  xsocket_connection_t *connection;

  connectable = g_network_address_parse (host_and_port, default_port, error);
  if (connectable == NULL)
    return NULL;

  connection = xsocket_client_connect (client, connectable,
					cancellable, error);
  xobject_unref (connectable);

  return connection;
}

/**
 * xsocket_client_connect_to_service:
 * @client: a #xsocket_connection_t
 * @domain: a domain name
 * @service: the name of the service to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a pointer to a #xerror_t, or %NULL
 *
 * Attempts to create a TCP connection to a service.
 *
 * This call looks up the SRV record for @service at @domain for the
 * "tcp" protocol.  It then attempts to connect, in turn, to each of
 * the hosts providing the service until either a connection succeeds
 * or there are no hosts remaining.
 *
 * Upon a successful connection, a new #xsocket_connection_t is constructed
 * and returned.  The caller owns this new object and must drop their
 * reference to it when finished with it.
 *
 * In the event of any failure (DNS error, service not found, no hosts
 * connectable) %NULL is returned and @error (if non-%NULL) is set
 * accordingly.
 *
 * Returns: (transfer full): a #xsocket_connection_t if successful, or %NULL on error
 */
xsocket_connection_t *
xsocket_client_connect_to_service (xsocket_client_t  *client,
				    const xchar_t    *domain,
				    const xchar_t    *service,
				    xcancellable_t   *cancellable,
				    xerror_t        **error)
{
  xsocket_connectable_t *connectable;
  xsocket_connection_t *connection;

  connectable = g_network_service_new (service, "tcp", domain);
  connection = xsocket_client_connect (client, connectable,
					cancellable, error);
  xobject_unref (connectable);

  return connection;
}

/**
 * xsocket_client_connect_to_uri:
 * @client: a #xsocket_client_t
 * @uri: A network URI
 * @default_port: the default port to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: a pointer to a #xerror_t, or %NULL
 *
 * This is a helper function for xsocket_client_connect().
 *
 * Attempts to create a TCP connection with a network URI.
 *
 * @uri may be any valid URI containing an "authority" (hostname/port)
 * component. If a port is not specified in the URI, @default_port
 * will be used. TLS will be negotiated if #xsocket_client_t:tls is %TRUE.
 * (#xsocket_client_t does not know to automatically assume TLS for
 * certain URI schemes.)
 *
 * Using this rather than xsocket_client_connect() or
 * xsocket_client_connect_to_host() allows #xsocket_client_t to
 * determine when to use application-specific proxy protocols.
 *
 * Upon a successful connection, a new #xsocket_connection_t is constructed
 * and returned.  The caller owns this new object and must drop their
 * reference to it when finished with it.
 *
 * In the event of any failure (DNS error, service not found, no hosts
 * connectable) %NULL is returned and @error (if non-%NULL) is set
 * accordingly.
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.26
 */
xsocket_connection_t *
xsocket_client_connect_to_uri (xsocket_client_t  *client,
				const xchar_t    *uri,
				xuint16_t         default_port,
				xcancellable_t   *cancellable,
				xerror_t        **error)
{
  xsocket_connectable_t *connectable;
  xsocket_connection_t *connection;

  connectable = g_network_address_parse_uri (uri, default_port, error);
  if (connectable == NULL)
    return NULL;

  connection = xsocket_client_connect (client, connectable,
					cancellable, error);
  xobject_unref (connectable);

  return connection;
}

typedef struct
{
  xtask_t *task; /* unowned */
  xsocket_client_t *client;

  xsocket_connectable_t *connectable;
  xsocket_address_enumerator_t *enumerator;
  xcancellable_t *enumeration_cancellable;

  xslist_t *connection_attempts;
  xslist_t *successful_connections;
  SocketClientErrorInfo *error_info;

  xboolean_t enumerated_at_least_once;
  xboolean_t enumeration_completed;
  xboolean_t connection_in_progress;
  xboolean_t completed;
} GSocketClientAsyncConnectData;

static void connection_attempt_unref (xpointer_t attempt);

static void
xsocket_client_async_connect_data_free (GSocketClientAsyncConnectData *data)
{
  data->task = NULL;
  g_clear_object (&data->connectable);
  g_clear_object (&data->enumerator);
  g_clear_object (&data->enumeration_cancellable);
  xslist_free_full (data->connection_attempts, connection_attempt_unref);
  xslist_free_full (data->successful_connections, connection_attempt_unref);

  g_clear_pointer (&data->error_info, socket_client_error_info_free);

  g_slice_free (GSocketClientAsyncConnectData, data);
}

typedef struct
{
  xsocket_address_t *address;
  xsocket_t *socket;
  xio_stream_t *connection;
  xproxy_address_t *proxy_addr;
  GSocketClientAsyncConnectData *data; /* unowned */
  xsource_t *timeout_source;
  xcancellable_t *cancellable;
  grefcount ref;
} ConnectionAttempt;

static ConnectionAttempt *
connection_attempt_new (void)
{
  ConnectionAttempt *attempt = g_new0 (ConnectionAttempt, 1);
  g_ref_count_init (&attempt->ref);
  return attempt;
}

static ConnectionAttempt *
connection_attempt_ref (ConnectionAttempt *attempt)
{
  g_ref_count_inc (&attempt->ref);
  return attempt;
}

static void
connection_attempt_unref (xpointer_t pointer)
{
  ConnectionAttempt *attempt = pointer;
  if (g_ref_count_dec (&attempt->ref))
    {
      g_clear_object (&attempt->address);
      g_clear_object (&attempt->socket);
      g_clear_object (&attempt->connection);
      g_clear_object (&attempt->cancellable);
      g_clear_object (&attempt->proxy_addr);
      if (attempt->timeout_source)
        {
          xsource_destroy (attempt->timeout_source);
          xsource_unref (attempt->timeout_source);
        }
      g_free (attempt);
    }
}

static void
connection_attempt_remove (ConnectionAttempt *attempt)
{
  attempt->data->connection_attempts = xslist_remove (attempt->data->connection_attempts, attempt);
  connection_attempt_unref (attempt);
}

static void
cancel_all_attempts (GSocketClientAsyncConnectData *data)
{
  xslist_t *l;

  for (l = data->connection_attempts; l; l = xslist_next (l))
    {
      ConnectionAttempt *attempt_entry = l->data;
      xcancellable_cancel (attempt_entry->cancellable);
      connection_attempt_unref (attempt_entry);
    }
  xslist_free (data->connection_attempts);
  data->connection_attempts = NULL;

  xslist_free_full (data->successful_connections, connection_attempt_unref);
  data->successful_connections = NULL;

  xcancellable_cancel (data->enumeration_cancellable);
}

static void
xsocket_client_async_connect_complete (ConnectionAttempt *attempt)
{
  GSocketClientAsyncConnectData *data = attempt->data;
  xerror_t *error = NULL;
  xassert (attempt->connection);
  xassert (!data->completed);

  if (!X_IS_SOCKET_CONNECTION (attempt->connection))
    {
      xsocket_connection_t *wrapper_connection;

      wrapper_connection = g_tcp_wrapper_connection_new (attempt->connection, attempt->socket);
      xobject_unref (attempt->connection);
      attempt->connection = (xio_stream_t *)wrapper_connection;
    }

  data->completed = TRUE;
  cancel_all_attempts (data);

  if (xcancellable_set_error_if_cancelled (xtask_get_cancellable (data->task), &error))
    {
      g_debug ("xsocket_client_t: Connection cancelled!");
      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_COMPLETE, data->connectable, NULL);
      xtask_return_error (data->task, g_steal_pointer (&error));
    }
  else
    {
      g_debug ("xsocket_client_t: Connection successful!");
      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_COMPLETE, data->connectable, attempt->connection);
      xtask_return_pointer (data->task, g_steal_pointer (&attempt->connection), xobject_unref);
    }

  connection_attempt_unref (attempt);
  xobject_unref (data->task);
}


static void
xsocket_client_enumerator_callback (xobject_t      *object,
				     xasync_result_t *result,
				     xpointer_t      user_data);

static void
enumerator_next_async (GSocketClientAsyncConnectData *data,
                       xboolean_t                       add_task_ref)
{
  /* Each enumeration takes a ref. This arg just avoids repeated unrefs when
     an enumeration starts another enumeration */
  if (add_task_ref)
    xobject_ref (data->task);

  if (!data->enumerated_at_least_once)
    xsocket_client_emit_event (data->client, XSOCKET_CLIENT_RESOLVING, data->connectable, NULL);
  g_debug ("xsocket_client_t: Starting new address enumeration");
  xsocket_address_enumerator_next_async (data->enumerator,
					  data->enumeration_cancellable,
					  xsocket_client_enumerator_callback,
					  data);
}

static void try_next_connection_or_finish (GSocketClientAsyncConnectData *, xboolean_t);

static void
xsocket_client_tls_handshake_callback (xobject_t      *object,
					xasync_result_t *result,
					xpointer_t      user_data)
{
  ConnectionAttempt *attempt = user_data;
  GSocketClientAsyncConnectData *data = attempt->data;

  if (xtls_connection_handshake_finish (G_TLS_CONNECTION (object),
					 result,
					 &data->error_info->tmp_error))
    {
      xobject_unref (attempt->connection);
      attempt->connection = XIO_STREAM (object);

      g_debug ("xsocket_client_t: TLS handshake succeeded");
      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_TLS_HANDSHAKED, data->connectable, attempt->connection);
      xsocket_client_async_connect_complete (attempt);
    }
  else
    {
      xobject_unref (object);
      connection_attempt_unref (attempt);

      g_debug ("xsocket_client_t: TLS handshake failed: %s", data->error_info->tmp_error->message);
      consider_tmp_error (data->error_info, XSOCKET_CLIENT_TLS_HANDSHAKING);
      try_next_connection_or_finish (data, TRUE);
    }
}

static void
xsocket_client_tls_handshake (ConnectionAttempt *attempt)
{
  GSocketClientAsyncConnectData *data = attempt->data;
  xio_stream_t *tlsconn;

  if (!data->client->priv->tls)
    {
      xsocket_client_async_connect_complete (attempt);
      return;
    }

  g_debug ("xsocket_client_t: Starting TLS handshake");
  tlsconn = xtls_client_connection_new (attempt->connection,
					 data->connectable,
					 &data->error_info->tmp_error);
  if (tlsconn)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      xtls_client_connection_set_validation_flags (G_TLS_CLIENT_CONNECTION (tlsconn),
                                                    data->client->priv->tls_validation_flags);
G_GNUC_END_IGNORE_DEPRECATIONS
      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_TLS_HANDSHAKING, data->connectable, XIO_STREAM (tlsconn));
      xtls_connection_handshake_async (G_TLS_CONNECTION (tlsconn),
					G_PRIORITY_DEFAULT,
					xtask_get_cancellable (data->task),
					xsocket_client_tls_handshake_callback,
					attempt);
    }
  else
    {
      connection_attempt_unref (attempt);

      consider_tmp_error (data->error_info, XSOCKET_CLIENT_TLS_HANDSHAKING);
      try_next_connection_or_finish (data, TRUE);
    }
}

static void
xsocket_client_proxy_connect_callback (xobject_t      *object,
					xasync_result_t *result,
					xpointer_t      user_data)
{
  ConnectionAttempt *attempt = user_data;
  GSocketClientAsyncConnectData *data = attempt->data;

  xobject_unref (attempt->connection);
  attempt->connection = xproxy_connect_finish (G_PROXY (object),
                                                result,
                                                &data->error_info->tmp_error);
  if (attempt->connection)
    {
      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_PROXY_NEGOTIATED, data->connectable, attempt->connection);
      xsocket_client_tls_handshake (attempt);
    }
  else
    {
      connection_attempt_unref (attempt);

      consider_tmp_error (data->error_info, XSOCKET_CLIENT_PROXY_NEGOTIATING);
      try_next_connection_or_finish (data, TRUE);
    }
}

static void
complete_connection_with_error (GSocketClientAsyncConnectData *data,
                                xerror_t                        *error)
{
  g_debug ("xsocket_client_t: Connection failed: %s", error->message);
  xassert (!data->completed);

  xsocket_client_emit_event (data->client, XSOCKET_CLIENT_COMPLETE, data->connectable, NULL);
  data->completed = TRUE;
  cancel_all_attempts (data);
  xtask_return_error (data->task, error);
}

static xboolean_t
task_completed_or_cancelled (GSocketClientAsyncConnectData *data)
{
  xtask_t *task = data->task;
  xcancellable_t *cancellable = xtask_get_cancellable (task);
  xerror_t *error = NULL;

  if (data->completed)
    return TRUE;
  else if (xcancellable_set_error_if_cancelled (cancellable, &error))
    {
      complete_connection_with_error (data, g_steal_pointer (&error));
      return TRUE;
    }
  else
    return FALSE;
}

static xboolean_t
try_next_successful_connection (GSocketClientAsyncConnectData *data)
{
  ConnectionAttempt *attempt;
  const xchar_t *protocol;
  xproxy_t *proxy;

  if (data->connection_in_progress)
    return FALSE;

  xassert (data->successful_connections != NULL);
  attempt = data->successful_connections->data;
  xassert (attempt != NULL);
  data->successful_connections = xslist_remove (data->successful_connections, attempt);
  data->connection_in_progress = TRUE;

  g_debug ("xsocket_client_t: Starting application layer connection");

  if (!attempt->proxy_addr)
    {
      xsocket_client_tls_handshake (g_steal_pointer (&attempt));
      return TRUE;
    }

  protocol = xproxy_address_get_protocol (attempt->proxy_addr);

  /* The connection should not be anything other than TCP,
   * but let's put a safety guard in case
   */
  if (!X_IS_TCP_CONNECTION (attempt->connection))
    {
      g_critical ("Trying to proxy over non-TCP connection, this is "
          "most likely a bug in GLib IO library.");

      g_set_error_literal (&data->error_info->tmp_error,
          G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
          _("Proxying over a non-TCP connection is not supported."));
      consider_tmp_error (data->error_info, XSOCKET_CLIENT_PROXY_NEGOTIATING);
    }
  else if (xhash_table_contains (data->client->priv->app_proxies, protocol))
    {
      /* Simply complete the connection, we don't want to do TLS handshake
       * as the application proxy handling may need proxy handshake first */
      xsocket_client_async_connect_complete (g_steal_pointer (&attempt));
      return TRUE;
    }
  else if ((proxy = xproxy_get_default_for_protocol (protocol)))
    {
      xio_stream_t *connection = attempt->connection;
      xproxy_address_t *proxy_addr = attempt->proxy_addr;

      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_PROXY_NEGOTIATING, data->connectable, attempt->connection);
      g_debug ("xsocket_client_t: Starting proxy connection");
      xproxy_connect_async (proxy,
                             connection,
                             proxy_addr,
                             xtask_get_cancellable (data->task),
                             xsocket_client_proxy_connect_callback,
                             g_steal_pointer (&attempt));
      xobject_unref (proxy);
      return TRUE;
    }
  else
    {
      g_set_error (&data->error_info->tmp_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
          _("Proxy protocol “%s” is not supported."),
          protocol);
      consider_tmp_error (data->error_info, XSOCKET_CLIENT_PROXY_NEGOTIATING);
    }

  data->connection_in_progress = FALSE;
  g_clear_pointer (&attempt, connection_attempt_unref);
  return FALSE; /* All non-return paths are failures */
}

static void
try_next_connection_or_finish (GSocketClientAsyncConnectData *data,
                               xboolean_t                       end_current_connection)
{
  if (end_current_connection)
    data->connection_in_progress = FALSE;

  if (data->connection_in_progress)
    return;

  /* Keep trying successful connections until one works, each iteration pops one */
  while (data->successful_connections)
    {
      if (try_next_successful_connection (data))
        return;
    }

  if (!data->enumeration_completed)
    {
      enumerator_next_async (data, FALSE);
      return;
    }

  complete_connection_with_error (data, g_steal_pointer (&data->error_info->best_error));
}

static void
xsocket_client_connected_callback (xobject_t      *source,
				    xasync_result_t *result,
				    xpointer_t      user_data)
{
  ConnectionAttempt *attempt = user_data;
  GSocketClientAsyncConnectData *data = attempt->data;

  if (task_completed_or_cancelled (data) || xcancellable_is_cancelled (attempt->cancellable))
    {
      xobject_unref (data->task);
      connection_attempt_unref (attempt);
      return;
    }

  if (attempt->timeout_source)
    {
      xsource_destroy (attempt->timeout_source);
      g_clear_pointer (&attempt->timeout_source, xsource_unref);
    }

  if (!xsocket_connection_connect_finish (XSOCKET_CONNECTION (source),
					   result, &data->error_info->tmp_error))
    {
      if (!xcancellable_is_cancelled (attempt->cancellable))
        {
          g_debug ("xsocket_client_t: Connection attempt failed: %s", data->error_info->tmp_error->message);
          clarify_connect_error (data->error_info->tmp_error, data->connectable, attempt->address);
          consider_tmp_error (data->error_info, XSOCKET_CLIENT_CONNECTING);
          connection_attempt_remove (attempt);
          connection_attempt_unref (attempt);
          try_next_connection_or_finish (data, FALSE);
        }
      else /* Silently ignore cancelled attempts */
        {
          g_clear_error (&data->error_info->tmp_error);
          xobject_unref (data->task);
          connection_attempt_unref (attempt);
        }

      return;
    }

  xsocket_connection_set_cached_remote_address ((xsocket_connection_t*)attempt->connection, NULL);
  g_debug ("xsocket_client_t: TCP connection successful");
  xsocket_client_emit_event (data->client, XSOCKET_CLIENT_CONNECTED, data->connectable, attempt->connection);

  /* wrong, but backward compatible */
  xsocket_set_blocking (attempt->socket, TRUE);

  /* This ends the parallel "happy eyeballs" portion of connecting.
     Now that we have a successful tcp connection we will attempt to connect
     at the TLS/Proxy layer. If those layers fail we will move on to the next
     connection.
   */
  connection_attempt_remove (attempt);
  data->successful_connections = xslist_append (data->successful_connections, g_steal_pointer (&attempt));
  try_next_connection_or_finish (data, FALSE);
}

static xboolean_t
on_connection_attempt_timeout (xpointer_t data)
{
  ConnectionAttempt *attempt = data;

  if (!attempt->data->enumeration_completed)
    {
      g_debug ("xsocket_client_t: Timeout reached, trying another enumeration");
      enumerator_next_async (attempt->data, TRUE);
    }

  g_clear_pointer (&attempt->timeout_source, xsource_unref);
  return XSOURCE_REMOVE;
}

static void
on_connection_cancelled (xcancellable_t *cancellable,
                         xpointer_t      data)
{
  xcancellable_t *linked_cancellable = XCANCELLABLE (data);

  xcancellable_cancel (linked_cancellable);
}

static void
xsocket_client_enumerator_callback (xobject_t      *object,
				     xasync_result_t *result,
				     xpointer_t      user_data)
{
  GSocketClientAsyncConnectData *data = user_data;
  xsocket_address_t *address = NULL;
  xsocket_t *socket;
  ConnectionAttempt *attempt;

  if (task_completed_or_cancelled (data))
    {
      xobject_unref (data->task);
      return;
    }

  address = xsocket_address_enumerator_next_finish (data->enumerator,
						     result, &data->error_info->tmp_error);
  if (address == NULL)
    {
      if (G_UNLIKELY (data->enumeration_completed))
        return;

      data->enumeration_completed = TRUE;
      g_debug ("xsocket_client_t: Address enumeration completed (out of addresses)");

      /* As per API docs: We only care about error if it's the first call,
         after that the enumerator is done.

         Note that we don't care about cancellation errors because
         task_completed_or_cancelled() above should handle that.

         If this fails and nothing is in progress then we will complete task here.
       */
      if ((data->enumerated_at_least_once && !data->connection_attempts && !data->connection_in_progress) ||
          !data->enumerated_at_least_once)
        {
          g_debug ("xsocket_client_t: Address enumeration failed: %s",
                   data->error_info->tmp_error ? data->error_info->tmp_error->message : NULL);
          consider_tmp_error (data->error_info, XSOCKET_CLIENT_RESOLVING);
          xassert (data->error_info->best_error);
          complete_connection_with_error (data, g_steal_pointer (&data->error_info->best_error));
        }

      /* Enumeration should never trigger again, drop our ref */
      xobject_unref (data->task);
      return;
    }

  g_debug ("xsocket_client_t: Address enumeration succeeded");
  if (!data->enumerated_at_least_once)
    {
      xsocket_client_emit_event (data->client, XSOCKET_CLIENT_RESOLVED,
				  data->connectable, NULL);
      data->enumerated_at_least_once = TRUE;
    }

  socket = create_socket (data->client, address, &data->error_info->tmp_error);
  if (socket == NULL)
    {
      xobject_unref (address);
      consider_tmp_error (data->error_info, XSOCKET_CLIENT_CONNECTING);
      enumerator_next_async (data, FALSE);
      return;
    }

  attempt = connection_attempt_new ();
  attempt->data = data;
  attempt->socket = socket;
  attempt->address = address;
  attempt->cancellable = xcancellable_new ();
  attempt->connection = (xio_stream_t *)xsocket_connection_factory_create_connection (socket);
  attempt->timeout_source = g_timeout_source_new (HAPPY_EYEBALLS_CONNECTION_ATTEMPT_TIMEOUT_MS);

  if (X_IS_PROXY_ADDRESS (address) && data->client->priv->enable_proxy)
    attempt->proxy_addr = xobject_ref (G_PROXY_ADDRESS (address));

  xsource_set_callback (attempt->timeout_source, on_connection_attempt_timeout, attempt, NULL);
  xsource_attach (attempt->timeout_source, xtask_get_context (data->task));
  data->connection_attempts = xslist_append (data->connection_attempts, attempt);

  if (xtask_get_cancellable (data->task))
    xcancellable_connect (xtask_get_cancellable (data->task), G_CALLBACK (on_connection_cancelled),
                           xobject_ref (attempt->cancellable), xobject_unref);

  xsocket_connection_set_cached_remote_address ((xsocket_connection_t *)attempt->connection, address);
  g_debug ("xsocket_client_t: Starting TCP connection attempt");
  xsocket_client_emit_event (data->client, XSOCKET_CLIENT_CONNECTING, data->connectable, attempt->connection);
  xsocket_connection_connect_async (XSOCKET_CONNECTION (attempt->connection),
				     address,
				     attempt->cancellable,
				     xsocket_client_connected_callback, connection_attempt_ref (attempt));
}

/**
 * xsocket_client_connect_async:
 * @client: a #xsocket_client_t
 * @connectable: a #xsocket_connectable_t specifying the remote address.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): user data for the callback
 *
 * This is the asynchronous version of xsocket_client_connect().
 *
 * You may wish to prefer the asynchronous version even in synchronous
 * command line programs because, since 2.60, it implements
 * [RFC 8305](https://tools.ietf.org/html/rfc8305) "Happy Eyeballs"
 * recommendations to work around long connection timeouts in networks
 * where IPv6 is broken by performing an IPv4 connection simultaneously
 * without waiting for IPv6 to time out, which is not supported by the
 * synchronous call. (This is not an API guarantee, and may change in
 * the future.)
 *
 * When the operation is finished @callback will be
 * called. You can then call xsocket_client_connect_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xsocket_client_connect_async (xsocket_client_t       *client,
			       xsocket_connectable_t  *connectable,
			       xcancellable_t        *cancellable,
			       xasync_ready_callback_t  callback,
			       xpointer_t             user_data)
{
  GSocketClientAsyncConnectData *data;

  g_return_if_fail (X_IS_SOCKET_CLIENT (client));

  data = g_slice_new0 (GSocketClientAsyncConnectData);
  data->client = client;
  data->connectable = xobject_ref (connectable);
  data->error_info = socket_client_error_info_new ();

  if (can_use_proxy (client))
    {
      data->enumerator = xsocket_connectable_proxy_enumerate (connectable);
      if (client->priv->proxy_resolver &&
          X_IS_PROXY_ADDRESS_ENUMERATOR (data->enumerator))
        {
          xobject_set (G_OBJECT (data->enumerator),
                        "proxy-resolver", client->priv->proxy_resolver,
                        NULL);
        }
    }
  else
    data->enumerator = xsocket_connectable_enumerate (connectable);

  /* This function tries to match the behavior of xsocket_client_connect ()
     which is simple enough but much of it is done in parallel to be as responsive
     as possible as per Happy Eyeballs (RFC 8305). This complicates flow quite a
     bit but we can describe it in 3 sections:

     Firstly we have address enumeration (DNS):
       - This may be triggered multiple times by enumerator_next_async().
       - It also has its own cancellable (data->enumeration_cancellable).
       - Enumeration is done lazily because xnetwork_address_address_enumerator_t
         also does work in parallel and may lazily add new addresses.
       - If the first enumeration errors then the task errors. Otherwise all enumerations
         will potentially be used (until task or enumeration is cancelled).

      Then we start attempting connections (TCP):
        - Each connection is independent and kept in a ConnectionAttempt object.
          - They each hold a ref on the main task and have their own cancellable.
        - Multiple attempts may happen in parallel as per Happy Eyeballs.
        - Upon failure or timeouts more connection attempts are made.
          - If no connections succeed the task errors.
        - Upon success they are kept in a list of successful connections.

      Lastly we connect at the application layer (TLS, Proxies):
        - These are done in serial.
          - The reasoning here is that Happy Eyeballs is about making bad connections responsive
            at the IP/TCP layers. Issues at the application layer are generally not due to
            connectivity issues but rather misconfiguration.
        - Upon failure it will try the next TCP connection until it runs out and
          the task errors.
        - Upon success it cancels everything remaining (enumeration and connections)
          and returns the connection.
  */

  data->task = xtask_new (client, cancellable, callback, user_data);
  xtask_set_check_cancellable (data->task, FALSE); /* We handle this manually */
  xtask_set_source_tag (data->task, xsocket_client_connect_async);
  xtask_set_task_data (data->task, data, (xdestroy_notify_t)xsocket_client_async_connect_data_free);

  data->enumeration_cancellable = xcancellable_new ();
  if (cancellable)
    xcancellable_connect (cancellable, G_CALLBACK (on_connection_cancelled),
                           xobject_ref (data->enumeration_cancellable), xobject_unref);

  enumerator_next_async (data, FALSE);
}

/**
 * xsocket_client_connect_to_host_async:
 * @client: a #xsocket_client_t
 * @host_and_port: the name and optionally the port of the host to connect to
 * @default_port: the default port to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): user data for the callback
 *
 * This is the asynchronous version of xsocket_client_connect_to_host().
 *
 * When the operation is finished @callback will be
 * called. You can then call xsocket_client_connect_to_host_finish() to get
 * the result of the operation.
 *
 * Since: 2.22
 */
void
xsocket_client_connect_to_host_async (xsocket_client_t        *client,
				       const xchar_t          *host_and_port,
				       xuint16_t               default_port,
				       xcancellable_t         *cancellable,
				       xasync_ready_callback_t   callback,
				       xpointer_t              user_data)
{
  xsocket_connectable_t *connectable;
  xerror_t *error;

  error = NULL;
  connectable = g_network_address_parse (host_and_port, default_port,
					 &error);
  if (connectable == NULL)
    {
      xtask_report_error (client, callback, user_data,
                           xsocket_client_connect_to_host_async,
                           error);
    }
  else
    {
      xsocket_client_connect_async (client,
				     connectable, cancellable,
				     callback, user_data);
      xobject_unref (connectable);
    }
}

/**
 * xsocket_client_connect_to_service_async:
 * @client: a #xsocket_client_t
 * @domain: a domain name
 * @service: the name of the service to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): user data for the callback
 *
 * This is the asynchronous version of
 * xsocket_client_connect_to_service().
 *
 * Since: 2.22
 */
void
xsocket_client_connect_to_service_async (xsocket_client_t       *client,
					  const xchar_t         *domain,
					  const xchar_t         *service,
					  xcancellable_t        *cancellable,
					  xasync_ready_callback_t  callback,
					  xpointer_t             user_data)
{
  xsocket_connectable_t *connectable;

  connectable = g_network_service_new (service, "tcp", domain);
  xsocket_client_connect_async (client,
				 connectable, cancellable,
				 callback, user_data);
  xobject_unref (connectable);
}

/**
 * xsocket_client_connect_to_uri_async:
 * @client: a #xsocket_client_t
 * @uri: a network uri
 * @default_port: the default port to connect to
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): a #xasync_ready_callback_t
 * @user_data: (closure): user data for the callback
 *
 * This is the asynchronous version of xsocket_client_connect_to_uri().
 *
 * When the operation is finished @callback will be
 * called. You can then call xsocket_client_connect_to_uri_finish() to get
 * the result of the operation.
 *
 * Since: 2.26
 */
void
xsocket_client_connect_to_uri_async (xsocket_client_t        *client,
				      const xchar_t          *uri,
				      xuint16_t               default_port,
				      xcancellable_t         *cancellable,
				      xasync_ready_callback_t   callback,
				      xpointer_t              user_data)
{
  xsocket_connectable_t *connectable;
  xerror_t *error;

  error = NULL;
  connectable = g_network_address_parse_uri (uri, default_port, &error);
  if (connectable == NULL)
    {
      xtask_report_error (client, callback, user_data,
                           xsocket_client_connect_to_uri_async,
                           error);
    }
  else
    {
      g_debug("xsocket_client_connect_to_uri_async");
      xsocket_client_connect_async (client,
				     connectable, cancellable,
				     callback, user_data);
      xobject_unref (connectable);
    }
}


/**
 * xsocket_client_connect_finish:
 * @client: a #xsocket_client_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an async connect operation. See xsocket_client_connect_async()
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.22
 */
xsocket_connection_t *
xsocket_client_connect_finish (xsocket_client_t  *client,
				xasync_result_t   *result,
				xerror_t        **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, client), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

/**
 * xsocket_client_connect_to_host_finish:
 * @client: a #xsocket_client_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an async connect operation. See xsocket_client_connect_to_host_async()
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.22
 */
xsocket_connection_t *
xsocket_client_connect_to_host_finish (xsocket_client_t  *client,
					xasync_result_t   *result,
					xerror_t        **error)
{
  return xsocket_client_connect_finish (client, result, error);
}

/**
 * xsocket_client_connect_to_service_finish:
 * @client: a #xsocket_client_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an async connect operation. See xsocket_client_connect_to_service_async()
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.22
 */
xsocket_connection_t *
xsocket_client_connect_to_service_finish (xsocket_client_t  *client,
					   xasync_result_t   *result,
					   xerror_t        **error)
{
  return xsocket_client_connect_finish (client, result, error);
}

/**
 * xsocket_client_connect_to_uri_finish:
 * @client: a #xsocket_client_t.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t location to store the error occurring, or %NULL to
 * ignore.
 *
 * Finishes an async connect operation. See xsocket_client_connect_to_uri_async()
 *
 * Returns: (transfer full): a #xsocket_connection_t on success, %NULL on error.
 *
 * Since: 2.26
 */
xsocket_connection_t *
xsocket_client_connect_to_uri_finish (xsocket_client_t  *client,
				       xasync_result_t   *result,
				       xerror_t        **error)
{
  return xsocket_client_connect_finish (client, result, error);
}

/**
 * xsocket_client_add_application_proxy:
 * @client: a #xsocket_client_t
 * @protocol: The proxy protocol
 *
 * Enable proxy protocols to be handled by the application. When the
 * indicated proxy protocol is returned by the #xproxy_resolver_t,
 * #xsocket_client_t will consider this protocol as supported but will
 * not try to find a #xproxy_t instance to handle handshaking. The
 * application must check for this case by calling
 * xsocket_connection_get_remote_address() on the returned
 * #xsocket_connection_t, and seeing if it's a #xproxy_address_t of the
 * appropriate type, to determine whether or not it needs to handle
 * the proxy handshaking itself.
 *
 * This should be used for proxy protocols that are dialects of
 * another protocol such as HTTP proxy. It also allows cohabitation of
 * proxy protocols that are reused between protocols. A good example
 * is HTTP. It can be used to proxy HTTP, FTP and Gopher and can also
 * be use as generic socket proxy through the HTTP CONNECT method.
 *
 * When the proxy is detected as being an application proxy, TLS handshake
 * will be skipped. This is required to let the application do the proxy
 * specific handshake.
 */
void
xsocket_client_add_application_proxy (xsocket_client_t *client,
			               const xchar_t   *protocol)
{
  xhash_table_add (client->priv->app_proxies, xstrdup (protocol));
}
