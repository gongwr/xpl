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
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"
#include "gproxyaddressenumerator.h"

#include <string.h>

#include "gasyncresult.h"
#include "ginetaddress.h"
#include "glibintl.h"
#include "gnetworkaddress.h"
#include "gnetworkingprivate.h"
#include "gproxy.h"
#include "gproxyaddress.h"
#include "gproxyresolver.h"
#include "gtask.h"
#include "gresolver.h"
#include "gsocketaddress.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"

/**
 * SECTION:gproxyaddressenumerator
 * @short_description: Proxy wrapper enumerator for socket addresses
 * @include: gio/gio.h
 *
 * #xproxy_address_enumerator_t is a wrapper around #xsocket_address_enumerator_t which
 * takes the #xsocket_address_t instances returned by the #xsocket_address_enumerator_t
 * and wraps them in #xproxy_address_t instances, using the given
 * #xproxy_address_enumerator_t:proxy-resolver.
 *
 * This enumerator will be returned (for example, by
 * xsocket_connectable_enumerate()) as appropriate when a proxy is configured;
 * there should be no need to manually wrap a #xsocket_address_enumerator_t instance
 * with one.
 */

#define GET_PRIVATE(o) (G_PROXY_ADDRESS_ENUMERATOR (o)->priv)

enum
{
  PROP_0,
  PROP_URI,
  PROP_DEFAULT_PORT,
  PROP_CONNECTABLE,
  PROP_PROXY_RESOLVER
};

struct _GProxyAddressEnumeratorPrivate
{
  /* Destination address */
  xsocket_connectable_t *connectable;
  xchar_t              *dest_uri;
  xuint16_t             default_port;
  xchar_t              *dest_hostname;
  xuint16_t             dest_port;
  xlist_t              *dest_ips;

  /* Proxy enumeration */
  xproxy_resolver_t           *proxy_resolver;
  xchar_t                   **proxies;
  xchar_t                   **next_proxy;
  xsocket_address_enumerator_t *addr_enum;
  xsocket_address_t           *proxy_address;
  const xchar_t              *proxy_uri;
  xchar_t                    *proxy_type;
  xchar_t                    *proxy_username;
  xchar_t                    *proxy_password;
  xboolean_t                  supports_hostname;
  xlist_t                    *next_dest_ip;
  xerror_t                   *last_error;
};

G_DEFINE_TYPE_WITH_PRIVATE (xproxy_address_enumerator_t, xproxy_address_enumerator, XTYPE_SOCKET_ADDRESS_ENUMERATOR)

static void
save_userinfo (GProxyAddressEnumeratorPrivate *priv,
               const xchar_t *proxy)
{
  g_clear_pointer (&priv->proxy_username, g_free);
  g_clear_pointer (&priv->proxy_password, g_free);

  xuri_split_with_user (proxy, XURI_FLAGS_HAS_PASSWORD, NULL,
                         &priv->proxy_username, &priv->proxy_password,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

static void
next_enumerator (GProxyAddressEnumeratorPrivate *priv)
{
  if (priv->proxy_address)
    return;

  while (priv->addr_enum == NULL && *priv->next_proxy)
    {
      xsocket_connectable_t *connectable = NULL;
      xproxy_t *proxy;

      priv->proxy_uri = *priv->next_proxy++;
      g_free (priv->proxy_type);
      priv->proxy_type = xuri_parse_scheme (priv->proxy_uri);

      if (priv->proxy_type == NULL)
	continue;

      /* Assumes hostnames are supported for unknown protocols */
      priv->supports_hostname = TRUE;
      proxy = xproxy_get_default_for_protocol (priv->proxy_type);
      if (proxy)
        {
	  priv->supports_hostname = xproxy_supports_hostname (proxy);
	  xobject_unref (proxy);
        }

      if (strcmp ("direct", priv->proxy_type) == 0)
	{
	  if (priv->connectable)
	    connectable = xobject_ref (priv->connectable);
	  else
	    connectable = g_network_address_new (priv->dest_hostname,
						 priv->dest_port);
	}
      else
	{
	  xerror_t *error = NULL;

	  connectable = g_network_address_parse_uri (priv->proxy_uri, 0, &error);

	  if (error)
	    {
	      g_warning ("Invalid proxy URI '%s': %s",
			 priv->proxy_uri, error->message);
	      xerror_free (error);
	    }

	  save_userinfo (priv, priv->proxy_uri);
	}

      if (connectable)
	{
	  priv->addr_enum = xsocket_connectable_enumerate (connectable);
	  xobject_unref (connectable);
	}
    }
}

static xsocket_address_t *
xproxy_address_enumerator_next (xsocket_address_enumerator_t  *enumerator,
				 xcancellable_t              *cancellable,
				 xerror_t                   **error)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (enumerator);
  xsocket_address_t *result = NULL;
  xerror_t *first_error = NULL;

  if (priv->proxies == NULL)
    {
      priv->proxies = xproxy_resolver_lookup (priv->proxy_resolver,
					       priv->dest_uri,
					       cancellable,
					       error);
      priv->next_proxy = priv->proxies;

      if (priv->proxies == NULL)
	return NULL;
    }

  while (result == NULL && (*priv->next_proxy || priv->addr_enum))
    {
      xchar_t *dest_hostname;
      xchar_t *dest_protocol;
      xinet_socket_address_t *inetsaddr;
      xinet_address_t *inetaddr;
      xuint16_t port;

      next_enumerator (priv);

      if (!priv->addr_enum)
	continue;

      if (priv->proxy_address == NULL)
	{
	  priv->proxy_address = xsocket_address_enumerator_next (
				    priv->addr_enum,
				    cancellable,
				    first_error ? NULL : &first_error);
	}

      if (priv->proxy_address == NULL)
	{
	  xobject_unref (priv->addr_enum);
	  priv->addr_enum = NULL;

	  if (priv->dest_ips)
	    {
	      g_resolver_free_addresses (priv->dest_ips);
	      priv->dest_ips = NULL;
	    }

	  continue;
	}

      if (strcmp ("direct", priv->proxy_type) == 0)
	{
	  result = priv->proxy_address;
	  priv->proxy_address = NULL;
	  continue;
	}

      if (!priv->supports_hostname)
	{
	  xinet_address_t *dest_ip;

	  if (!priv->dest_ips)
	    {
	      xresolver_t *resolver;

	      resolver = g_resolver_get_default();
	      priv->dest_ips = g_resolver_lookup_by_name (resolver,
							  priv->dest_hostname,
							  cancellable,
							  first_error ? NULL : &first_error);
	      xobject_unref (resolver);

	      if (!priv->dest_ips)
		{
		  xobject_unref (priv->proxy_address);
		  priv->proxy_address = NULL;
		  continue;
		}
	    }

	  if (!priv->next_dest_ip)
	    priv->next_dest_ip = priv->dest_ips;

	  dest_ip = G_INET_ADDRESS (priv->next_dest_ip->data);
	  dest_hostname = xinet_address_to_string (dest_ip);

	  priv->next_dest_ip = xlist_next (priv->next_dest_ip);
	}
      else
	{
	  dest_hostname = xstrdup (priv->dest_hostname);
	}
      dest_protocol = xuri_parse_scheme (priv->dest_uri);

      if (!X_IS_INET_SOCKET_ADDRESS (priv->proxy_address))
        {
	  g_free (dest_hostname);
	  g_free (dest_protocol);
        }
      xreturn_val_if_fail (X_IS_INET_SOCKET_ADDRESS (priv->proxy_address), NULL);

      inetsaddr = G_INET_SOCKET_ADDRESS (priv->proxy_address);
      inetaddr = g_inet_socket_address_get_address (inetsaddr);
      port = g_inet_socket_address_get_port (inetsaddr);

      result = xobject_new (XTYPE_PROXY_ADDRESS,
			     "address", inetaddr,
			     "port", port,
			     "protocol", priv->proxy_type,
			     "destination-protocol", dest_protocol,
			     "destination-hostname", dest_hostname,
			     "destination-port", priv->dest_port,
			     "username", priv->proxy_username,
			     "password", priv->proxy_password,
			     "uri", priv->proxy_uri,
			     NULL);
      g_free (dest_hostname);
      g_free (dest_protocol);

      if (priv->supports_hostname || priv->next_dest_ip == NULL)
	{
	  xobject_unref (priv->proxy_address);
	  priv->proxy_address = NULL;
	}
    }

  if (result == NULL && first_error)
    g_propagate_error (error, first_error);
  else if (first_error)
    xerror_free (first_error);

  return result;
}



static void
complete_async (xtask_t *task)
{
  GProxyAddressEnumeratorPrivate *priv = xtask_get_task_data (task);

  if (priv->last_error)
    {
      xtask_return_error (task, priv->last_error);
      priv->last_error = NULL;
    }
  else
    xtask_return_pointer (task, NULL, NULL);

  xobject_unref (task);
}

static void
return_result (xtask_t *task)
{
  GProxyAddressEnumeratorPrivate *priv = xtask_get_task_data (task);
  xsocket_address_t *result;

  if (strcmp ("direct", priv->proxy_type) == 0)
    {
      result = priv->proxy_address;
      priv->proxy_address = NULL;
    }
  else
    {
      xchar_t *dest_hostname, *dest_protocol;
      xinet_socket_address_t *inetsaddr;
      xinet_address_t *inetaddr;
      xuint16_t port;

      if (!priv->supports_hostname)
	{
	  xinet_address_t *dest_ip;

	  if (!priv->next_dest_ip)
	    priv->next_dest_ip = priv->dest_ips;

	  dest_ip = G_INET_ADDRESS (priv->next_dest_ip->data);
	  dest_hostname = xinet_address_to_string (dest_ip);

	  priv->next_dest_ip = xlist_next (priv->next_dest_ip);
	}
      else
	{
	  dest_hostname = xstrdup (priv->dest_hostname);
	}
      dest_protocol = xuri_parse_scheme (priv->dest_uri);

      if (!X_IS_INET_SOCKET_ADDRESS (priv->proxy_address))
        {
	  g_free (dest_hostname);
	  g_free (dest_protocol);
        }
      g_return_if_fail (X_IS_INET_SOCKET_ADDRESS (priv->proxy_address));

      inetsaddr = G_INET_SOCKET_ADDRESS (priv->proxy_address);
      inetaddr = g_inet_socket_address_get_address (inetsaddr);
      port = g_inet_socket_address_get_port (inetsaddr);

      result = xobject_new (XTYPE_PROXY_ADDRESS,
			     "address", inetaddr,
			     "port", port,
			     "protocol", priv->proxy_type,
			     "destination-protocol", dest_protocol,
			     "destination-hostname", dest_hostname,
			     "destination-port", priv->dest_port,
			     "username", priv->proxy_username,
			     "password", priv->proxy_password,
			     "uri", priv->proxy_uri,
			     NULL);
      g_free (dest_hostname);
      g_free (dest_protocol);

      if (priv->supports_hostname || priv->next_dest_ip == NULL)
	{
	  xobject_unref (priv->proxy_address);
	  priv->proxy_address = NULL;
	}
    }

  xtask_return_pointer (task, result, xobject_unref);
  xobject_unref (task);
}

static void address_enumerate_cb (xobject_t      *object,
				  xasync_result_t *result,
				  xpointer_t	user_data);

static void
next_proxy (xtask_t *task)
{
  GProxyAddressEnumeratorPrivate *priv = xtask_get_task_data (task);

  if (*priv->next_proxy)
    {
      xobject_unref (priv->addr_enum);
      priv->addr_enum = NULL;

      if (priv->dest_ips)
	{
	  g_resolver_free_addresses (priv->dest_ips);
	  priv->dest_ips = NULL;
	}

      next_enumerator (priv);

      if (priv->addr_enum)
	{
	  xsocket_address_enumerator_next_async (priv->addr_enum,
						  xtask_get_cancellable (task),
						  address_enumerate_cb,
						  task);
	  return;
	}
    }

  complete_async (task);
}

static void
dest_hostname_lookup_cb (xobject_t           *object,
			 xasync_result_t      *result,
			 xpointer_t           user_data)
{
  xtask_t *task = user_data;
  GProxyAddressEnumeratorPrivate *priv = xtask_get_task_data (task);

  g_clear_error (&priv->last_error);
  priv->dest_ips = g_resolver_lookup_by_name_finish (G_RESOLVER (object),
						     result,
						     &priv->last_error);
  if (priv->dest_ips)
    return_result (task);
  else
    {
      g_clear_object (&priv->proxy_address);
      next_proxy (task);
    }
}

static void
address_enumerate_cb (xobject_t	   *object,
		      xasync_result_t *result,
		      xpointer_t	    user_data)
{
  xtask_t *task = user_data;
  GProxyAddressEnumeratorPrivate *priv = xtask_get_task_data (task);

  g_clear_error (&priv->last_error);
  priv->proxy_address =
    xsocket_address_enumerator_next_finish (priv->addr_enum,
					     result,
					     &priv->last_error);
  if (priv->proxy_address)
    {
      if (!priv->supports_hostname && !priv->dest_ips)
	{
	  xresolver_t *resolver;
	  resolver = g_resolver_get_default();
	  g_resolver_lookup_by_name_async (resolver,
					   priv->dest_hostname,
					   xtask_get_cancellable (task),
					   dest_hostname_lookup_cb,
					   task);
	  xobject_unref (resolver);
	  return;
	}

      return_result (task);
    }
  else
    next_proxy (task);
}

static void
proxy_lookup_cb (xobject_t      *object,
		 xasync_result_t *result,
		 xpointer_t      user_data)
{
  xtask_t *task = user_data;
  GProxyAddressEnumeratorPrivate *priv = xtask_get_task_data (task);

  g_clear_error (&priv->last_error);
  priv->proxies = xproxy_resolver_lookup_finish (G_PROXY_RESOLVER (object),
						  result,
						  &priv->last_error);
  priv->next_proxy = priv->proxies;

  if (priv->last_error)
    {
      complete_async (task);
      return;
    }
  else
    {
      next_enumerator (priv);
      if (priv->addr_enum)
	{
	  xsocket_address_enumerator_next_async (priv->addr_enum,
						  xtask_get_cancellable (task),
						  address_enumerate_cb,
						  task);
	  return;
	}
    }

  complete_async (task);
}

static void
xproxy_address_enumerator_next_async (xsocket_address_enumerator_t *enumerator,
				       xcancellable_t             *cancellable,
				       xasync_ready_callback_t       callback,
				       xpointer_t                  user_data)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (enumerator);
  xtask_t *task;

  task = xtask_new (enumerator, cancellable, callback, user_data);
  xtask_set_source_tag (task, xproxy_address_enumerator_next_async);
  xtask_set_task_data (task, priv, NULL);

  if (priv->proxies == NULL)
    {
      xproxy_resolver_lookup_async (priv->proxy_resolver,
				     priv->dest_uri,
				     cancellable,
				     proxy_lookup_cb,
				     task);
      return;
    }

  if (priv->addr_enum)
    {
      if (priv->proxy_address)
	{
	  return_result (task);
	  return;
	}
      else
	{
	  xsocket_address_enumerator_next_async (priv->addr_enum,
						  cancellable,
						  address_enumerate_cb,
						  task);
	  return;
	}
    }

  complete_async (task);
}

static xsocket_address_t *
xproxy_address_enumerator_next_finish (xsocket_address_enumerator_t  *enumerator,
					xasync_result_t              *result,
					xerror_t                   **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, enumerator), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static void
xproxy_address_enumerator_constructed (xobject_t *object)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);
  xsocket_connectable_t *conn;
  xuint_t port;

  if (priv->dest_uri)
    {
      conn = g_network_address_parse_uri (priv->dest_uri, priv->default_port, NULL);
      if (conn)
        {
          xobject_get (conn,
                        "hostname", &priv->dest_hostname,
                        "port", &port,
                        NULL);
          priv->dest_port = port;

          xobject_unref (conn);
        }
      else
        g_warning ("Invalid URI '%s'", priv->dest_uri);
    }

  XOBJECT_CLASS (xproxy_address_enumerator_parent_class)->constructed (object);
}

static void
xproxy_address_enumerator_get_property (xobject_t        *object,
                                         xuint_t           property_id,
                                         xvalue_t         *value,
                                         xparam_spec_t     *pspec)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);
  switch (property_id)
    {
    case PROP_URI:
      xvalue_set_string (value, priv->dest_uri);
      break;

    case PROP_DEFAULT_PORT:
      xvalue_set_uint (value, priv->default_port);
      break;

    case PROP_CONNECTABLE:
      xvalue_set_object (value, priv->connectable);
      break;

    case PROP_PROXY_RESOLVER:
      xvalue_set_object (value, priv->proxy_resolver);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
xproxy_address_enumerator_set_property (xobject_t        *object,
                                         xuint_t           property_id,
                                         const xvalue_t   *value,
                                         xparam_spec_t     *pspec)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);
  switch (property_id)
    {
    case PROP_URI:
      priv->dest_uri = xvalue_dup_string (value);
      break;

    case PROP_DEFAULT_PORT:
      priv->default_port = xvalue_get_uint (value);
      break;

    case PROP_CONNECTABLE:
      priv->connectable = xvalue_dup_object (value);
      break;

    case PROP_PROXY_RESOLVER:
      if (priv->proxy_resolver)
        xobject_unref (priv->proxy_resolver);
      priv->proxy_resolver = xvalue_get_object (value);
      if (!priv->proxy_resolver)
        priv->proxy_resolver = xproxy_resolver_get_default ();
      xobject_ref (priv->proxy_resolver);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
xproxy_address_enumerator_finalize (xobject_t *object)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);

  if (priv->connectable)
    xobject_unref (priv->connectable);

  if (priv->proxy_resolver)
    xobject_unref (priv->proxy_resolver);

  g_free (priv->dest_uri);
  g_free (priv->dest_hostname);

  if (priv->dest_ips)
    g_resolver_free_addresses (priv->dest_ips);

  xstrfreev (priv->proxies);

  if (priv->addr_enum)
    xobject_unref (priv->addr_enum);

  g_free (priv->proxy_type);
  g_free (priv->proxy_username);
  g_free (priv->proxy_password);

  g_clear_error (&priv->last_error);

  XOBJECT_CLASS (xproxy_address_enumerator_parent_class)->finalize (object);
}

static void
xproxy_address_enumerator_init (xproxy_address_enumerator_t *self)
{
  self->priv = xproxy_address_enumerator_get_instance_private (self);
}

static void
xproxy_address_enumerator_class_init (GProxyAddressEnumeratorClass *proxy_enumerator_class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (proxy_enumerator_class);
  xsocket_address_enumerator_class_t *enumerator_class = XSOCKET_ADDRESS_ENUMERATOR_CLASS (proxy_enumerator_class);

  object_class->constructed = xproxy_address_enumerator_constructed;
  object_class->set_property = xproxy_address_enumerator_set_property;
  object_class->get_property = xproxy_address_enumerator_get_property;
  object_class->finalize = xproxy_address_enumerator_finalize;

  enumerator_class->next = xproxy_address_enumerator_next;
  enumerator_class->next_async = xproxy_address_enumerator_next_async;
  enumerator_class->next_finish = xproxy_address_enumerator_next_finish;

  xobject_class_install_property (object_class,
				   PROP_URI,
				   xparam_spec_string ("uri",
							P_("URI"),
							P_("The destination URI, use none:// for generic socket"),
							NULL,
							XPARAM_READWRITE |
							XPARAM_CONSTRUCT_ONLY |
							XPARAM_STATIC_STRINGS));

  /**
   * xproxy_address_enumerator_t:default-port:
   *
   * The default port to use if #xproxy_address_enumerator_t:uri does not
   * specify one.
   *
   * Since: 2.38
   */
  xobject_class_install_property (object_class,
				   PROP_DEFAULT_PORT,
				   xparam_spec_uint ("default-port",
                                                      P_("Default port"),
                                                      P_("The default port to use if uri does not specify one"),
                                                      0, 65535, 0,
                                                      XPARAM_READWRITE |
                                                      XPARAM_CONSTRUCT_ONLY |
                                                      XPARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class,
				   PROP_CONNECTABLE,
				   xparam_spec_object ("connectable",
							P_("Connectable"),
							P_("The connectable being enumerated."),
							XTYPE_SOCKET_CONNECTABLE,
							XPARAM_READWRITE |
							XPARAM_CONSTRUCT_ONLY |
							XPARAM_STATIC_STRINGS));

  /**
   * xproxy_address_enumerator_t:proxy-resolver:
   *
   * The proxy resolver to use.
   *
   * Since: 2.36
   */
  xobject_class_install_property (object_class,
                                   PROP_PROXY_RESOLVER,
                                   xparam_spec_object ("proxy-resolver",
                                                        P_("Proxy resolver"),
                                                        P_("The proxy resolver to use."),
                                                        XTYPE_PROXY_RESOLVER,
                                                        XPARAM_READWRITE |
                                                        XPARAM_CONSTRUCT |
                                                        XPARAM_STATIC_STRINGS));
}
