/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2010, 2013 Red Hat, Inc.
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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gsimpleproxyresolver.h"
#include "ginetaddress.h"
#include "ginetaddressmask.h"
#include "gnetworkingprivate.h"
#include "gtask.h"

#include "glibintl.h"

/**
 * SECTION:gsimpleproxyresolver
 * @short_description: Simple proxy resolver implementation
 * @include: gio/gio.h
 * @see_also: xsocket_client_set_proxy_resolver()
 *
 * #xsimple_proxy_resolver_t is a simple #xproxy_resolver_t implementation
 * that handles a single default proxy, multiple URI-scheme-specific
 * proxies, and a list of hosts that proxies should not be used for.
 *
 * #xsimple_proxy_resolver_t is never the default proxy resolver, but it
 * can be used as the base class for another proxy resolver
 * implementation, or it can be created and used manually, such as
 * with xsocket_client_set_proxy_resolver().
 *
 * Since: 2.36
 */

typedef struct {
  xchar_t        *name;
  xsize_t          length;
  gushort       port;
} GSimpleProxyResolverDomain;

struct _xsimple_proxy_resolver_private {
  xchar_t *default_proxy, **ignore_hosts;
  xhashtable_t *uri_proxies;

  xptr_array_t *ignore_ips;
  GSimpleProxyResolverDomain *ignore_domains;
};

static void xsimple_proxy_resolver_iface_init (xproxy_resolver_interface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xsimple_proxy_resolver_t, xsimple_proxy_resolver, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xsimple_proxy_resolver_t)
                         G_IMPLEMENT_INTERFACE (XTYPE_PROXY_RESOLVER,
                                                xsimple_proxy_resolver_iface_init))

enum
{
  PROP_0,
  PROP_DEFAULT_PROXY,
  PROP_IGNORE_HOSTS
};

static void reparse_ignore_hosts (xsimple_proxy_resolver_t *resolver);

static void
xsimple_proxy_resolver_finalize (xobject_t *object)
{
  xsimple_proxy_resolver_t *resolver = XSIMPLE_PROXY_RESOLVER (object);
  xsimple_proxy_resolver_private_t *priv = resolver->priv;

  g_free (priv->default_proxy);
  xhash_table_destroy (priv->uri_proxies);

  g_clear_pointer (&priv->ignore_hosts, xstrfreev);
  /* This will free ignore_ips and ignore_domains */
  reparse_ignore_hosts (resolver);

  XOBJECT_CLASS (xsimple_proxy_resolver_parent_class)->finalize (object);
}

static void
xsimple_proxy_resolver_init (xsimple_proxy_resolver_t *resolver)
{
  resolver->priv = xsimple_proxy_resolver_get_instance_private (resolver);
  resolver->priv->uri_proxies = xhash_table_new_full (xstr_hash, xstr_equal,
                                                       g_free, g_free);
}

static void
xsimple_proxy_resolver_set_property (xobject_t      *object,
                                      xuint_t         prop_id,
                                      const xvalue_t *value,
                                      xparam_spec_t   *pspec)
{
  xsimple_proxy_resolver_t *resolver = XSIMPLE_PROXY_RESOLVER (object);

  switch (prop_id)
    {
      case PROP_DEFAULT_PROXY:
        xsimple_proxy_resolver_set_default_proxy (resolver, xvalue_get_string (value));
	break;

      case PROP_IGNORE_HOSTS:
        xsimple_proxy_resolver_set_ignore_hosts (resolver, xvalue_get_boxed (value));
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsimple_proxy_resolver_get_property (xobject_t    *object,
                                      xuint_t       prop_id,
                                      xvalue_t     *value,
                                      xparam_spec_t *pspec)
{
  xsimple_proxy_resolver_t *resolver = XSIMPLE_PROXY_RESOLVER (object);

  switch (prop_id)
    {
      case PROP_DEFAULT_PROXY:
	xvalue_set_string (value, resolver->priv->default_proxy);
	break;

      case PROP_IGNORE_HOSTS:
	xvalue_set_boxed (value, resolver->priv->ignore_hosts);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
reparse_ignore_hosts (xsimple_proxy_resolver_t *resolver)
{
  xsimple_proxy_resolver_private_t *priv = resolver->priv;
  xptr_array_t *ignore_ips;
  xarray_t *ignore_domains;
  xchar_t *host, *tmp, *colon, *bracket;
  xinet_address_t *iaddr;
  xinet_address_mask_t *mask;
  GSimpleProxyResolverDomain domain;
  gushort port;
  int i;

  if (priv->ignore_ips)
    xptr_array_free (priv->ignore_ips, TRUE);
  if (priv->ignore_domains)
    {
      for (i = 0; priv->ignore_domains[i].name; i++)
	g_free (priv->ignore_domains[i].name);
      g_free (priv->ignore_domains);
    }
  priv->ignore_ips = NULL;
  priv->ignore_domains = NULL;

  if (!priv->ignore_hosts || !priv->ignore_hosts[0])
    return;

  ignore_ips = xptr_array_new_with_free_func (xobject_unref);
  ignore_domains = g_array_new (TRUE, FALSE, sizeof (GSimpleProxyResolverDomain));

  for (i = 0; priv->ignore_hosts[i]; i++)
    {
      host = xstrchomp (priv->ignore_hosts[i]);

      /* See if it's an IP address or IP/length mask */
      mask = xinet_address_mask_new_from_string (host, NULL);
      if (mask)
        {
          xptr_array_add (ignore_ips, mask);
          continue;
        }

      port = 0;

      if (*host == '[')
        {
          /* [IPv6]:port */
          host++;
          bracket = strchr (host, ']');
          if (!bracket || !bracket[1] || bracket[1] != ':')
            goto bad;

          port = strtoul (bracket + 2, &tmp, 10);
          if (*tmp)
            goto bad;

          *bracket = '\0';
        }
      else
        {
          colon = strchr (host, ':');
          if (colon && !strchr (colon + 1, ':'))
            {
              /* hostname:port or IPv4:port */
              port = strtoul (colon + 1, &tmp, 10);
              if (*tmp)
                goto bad;
              *colon = '\0';
            }
        }

      iaddr = xinet_address_new_from_string (host);
      if (iaddr)
        xobject_unref (iaddr);
      else
        {
          if (xstr_has_prefix (host, "*."))
            host += 2;
          else if (*host == '.')
            host++;
        }

      memset (&domain, 0, sizeof (domain));
      domain.name = xstrdup (host);
      domain.length = strlen (domain.name);
      domain.port = port;
      g_array_append_val (ignore_domains, domain);
      continue;

    bad:
      g_warning ("Ignoring invalid ignore_hosts value '%s'", host);
    }

  if (ignore_ips->len)
    priv->ignore_ips = ignore_ips;
  else
    xptr_array_free (ignore_ips, TRUE);

  if (ignore_domains->len)
    priv->ignore_domains = (GSimpleProxyResolverDomain *)ignore_domains->data;
  g_array_free (ignore_domains, ignore_domains->len == 0);
}

static xboolean_t
ignore_host (xsimple_proxy_resolver_t *resolver,
	     const xchar_t          *host,
	     gushort               port)
{
  xsimple_proxy_resolver_private_t *priv = resolver->priv;
  xchar_t *ascii_host = NULL;
  xboolean_t ignore = FALSE;
  xsize_t offset, length;
  xuint_t i;

  if (priv->ignore_ips)
    {
      xinet_address_t *iaddr;

      iaddr = xinet_address_new_from_string (host);
      if (iaddr)
	{
	  for (i = 0; i < priv->ignore_ips->len; i++)
	    {
	      xinet_address_mask_t *mask = priv->ignore_ips->pdata[i];

	      if (xinet_address_mask_matches (mask, iaddr))
		{
		  ignore = TRUE;
		  break;
		}
	    }

	  xobject_unref (iaddr);
	  if (ignore)
	    return TRUE;
	}
    }

  if (priv->ignore_domains)
    {
      length = 0;
      if (g_hostname_is_non_ascii (host))
        host = ascii_host = g_hostname_to_ascii (host);
      if (host)
        length = strlen (host);

      for (i = 0; length > 0 && priv->ignore_domains[i].length; i++)
	{
	  GSimpleProxyResolverDomain *domain = &priv->ignore_domains[i];

          if (domain->length > length)
            continue;

	  offset = length - domain->length;
	  if ((domain->port == 0 || domain->port == port) &&
	      (offset == 0 || (offset > 0 && host[offset - 1] == '.')) &&
	      (g_ascii_strcasecmp (domain->name, host + offset) == 0))
	    {
	      ignore = TRUE;
	      break;
	    }
	}

      g_free (ascii_host);
    }

  return ignore;
}

static xchar_t **
xsimple_proxy_resolver_lookup (xproxy_resolver_t  *proxy_resolver,
                                const xchar_t     *uri,
                                xcancellable_t    *cancellable,
                                xerror_t         **error)
{
  xsimple_proxy_resolver_t *resolver = XSIMPLE_PROXY_RESOLVER (proxy_resolver);
  xsimple_proxy_resolver_private_t *priv = resolver->priv;
  const xchar_t *proxy = NULL;
  xchar_t **proxies;

  if (priv->ignore_ips || priv->ignore_domains)
    {
      xchar_t *host = NULL;
      xint_t port;

      if (xuri_split_network (uri, XURI_FLAGS_NONE, NULL,
                               &host, &port, NULL) &&
          ignore_host (resolver, host, port > 0 ? port : 0))
        proxy = "direct://";

      g_free (host);
    }

  if (!proxy && xhash_table_size (priv->uri_proxies))
    {
      xchar_t *scheme = g_ascii_strdown (uri, strcspn (uri, ":"));

      proxy = xhash_table_lookup (priv->uri_proxies, scheme);
      g_free (scheme);
    }

  if (!proxy)
    proxy = priv->default_proxy;
  if (!proxy)
    proxy = "direct://";

  if (!strncmp (proxy, "socks://", 8))
    {
      proxies = g_new0 (xchar_t *, 4);
      proxies[0] = xstrdup_printf ("socks5://%s", proxy + 8);
      proxies[1] = xstrdup_printf ("socks4a://%s", proxy + 8);
      proxies[2] = xstrdup_printf ("socks4://%s", proxy + 8);
      proxies[3] = NULL;
    }
  else
    {
      proxies = g_new0 (xchar_t *, 2);
      proxies[0] = xstrdup (proxy);
    }

  return proxies;
}

static void
xsimple_proxy_resolver_lookup_async (xproxy_resolver_t      *proxy_resolver,
                                      const xchar_t         *uri,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  xsimple_proxy_resolver_t *resolver = XSIMPLE_PROXY_RESOLVER (proxy_resolver);
  xtask_t *task;
  xerror_t *error = NULL;
  char **proxies;

  task = xtask_new (resolver, cancellable, callback, user_data);
  xtask_set_source_tag (task, xsimple_proxy_resolver_lookup_async);

  proxies = xsimple_proxy_resolver_lookup (proxy_resolver, uri,
                                            cancellable, &error);
  if (proxies)
    xtask_return_pointer (task, proxies, (xdestroy_notify_t)xstrfreev);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

static xchar_t **
xsimple_proxy_resolver_lookup_finish (xproxy_resolver_t  *resolver,
                                       xasync_result_t    *result,
                                       xerror_t         **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, resolver), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static void
xsimple_proxy_resolver_class_init (xsimple_proxy_resolver_class_t *resolver_class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (resolver_class);

  object_class->get_property = xsimple_proxy_resolver_get_property;
  object_class->set_property = xsimple_proxy_resolver_set_property;
  object_class->finalize = xsimple_proxy_resolver_finalize;

  /**
   * xsimple_proxy_resolver_t:default-proxy:
   *
   * The default proxy URI that will be used for any URI that doesn't
   * match #xsimple_proxy_resolver_t:ignore-hosts, and doesn't match any
   * of the schemes set with xsimple_proxy_resolver_set_uri_proxy().
   *
   * Note that as a special case, if this URI starts with
   * "socks://", #xsimple_proxy_resolver_t will treat it as referring
   * to all three of the socks5, socks4a, and socks4 proxy types.
   */
  xobject_class_install_property (object_class, PROP_DEFAULT_PROXY,
				   xparam_spec_string ("default-proxy",
                                                        P_("Default proxy"),
                                                        P_("The default proxy URI"),
                                                        NULL,
                                                        XPARAM_READWRITE |
                                                        XPARAM_STATIC_STRINGS));

  /**
   * xsimple_proxy_resolver_t:ignore-hosts:
   *
   * A list of hostnames and IP addresses that the resolver should
   * allow direct connections to.
   *
   * Entries can be in one of 4 formats:
   *
   * - A hostname, such as "example.com", ".example.com", or
   *   "*.example.com", any of which match "example.com" or
   *   any subdomain of it.
   *
   * - An IPv4 or IPv6 address, such as "192.168.1.1",
   *   which matches only that address.
   *
   * - A hostname or IP address followed by a port, such as
   *   "example.com:80", which matches whatever the hostname or IP
   *   address would match, but only for URLs with the (explicitly)
   *   indicated port. In the case of an IPv6 address, the address
   *   part must appear in brackets: "[::1]:443"
   *
   * - An IP address range, given by a base address and prefix length,
   *   such as "fe80::/10", which matches any address in that range.
   *
   * Note that when dealing with Unicode hostnames, the matching is
   * done against the ASCII form of the name.
   *
   * Also note that hostname exclusions apply only to connections made
   * to hosts identified by name, and IP address exclusions apply only
   * to connections made to hosts identified by address. That is, if
   * example.com has an address of 192.168.1.1, and the :ignore-hosts list
   * contains only "192.168.1.1", then a connection to "example.com"
   * (eg, via a #xnetwork_address_t) will use the proxy, and a connection to
   * "192.168.1.1" (eg, via a #xinet_socket_address_t) will not.
   *
   * These rules match the "ignore-hosts"/"noproxy" rules most
   * commonly used by other applications.
   */
  xobject_class_install_property (object_class, PROP_IGNORE_HOSTS,
				   xparam_spec_boxed ("ignore-hosts",
                                                       P_("Ignore hosts"),
                                                       P_("Hosts that will not use the proxy"),
                                                       XTYPE_STRV,
                                                       XPARAM_READWRITE |
                                                       XPARAM_STATIC_STRINGS));

}

static void
xsimple_proxy_resolver_iface_init (xproxy_resolver_interface_t *iface)
{
  iface->lookup = xsimple_proxy_resolver_lookup;
  iface->lookup_async = xsimple_proxy_resolver_lookup_async;
  iface->lookup_finish = xsimple_proxy_resolver_lookup_finish;
}

/**
 * xsimple_proxy_resolver_new:
 * @default_proxy: (nullable): the default proxy to use, eg
 *     "socks://192.168.1.1"
 * @ignore_hosts: (array zero-terminated=1) (nullable): an optional list of hosts/IP addresses
 *     to not use a proxy for.
 *
 * Creates a new #xsimple_proxy_resolver_t. See
 * #xsimple_proxy_resolver_t:default-proxy and
 * #xsimple_proxy_resolver_t:ignore-hosts for more details on how the
 * arguments are interpreted.
 *
 * Returns: (transfer full) a new #xsimple_proxy_resolver_t
 *
 * Since: 2.36
 */
xproxy_resolver_t *
xsimple_proxy_resolver_new (const xchar_t  *default_proxy,
                             xchar_t       **ignore_hosts)
{
  return xobject_new (XTYPE_SIMPLE_PROXY_RESOLVER,
                       "default-proxy", default_proxy,
                       "ignore-hosts", ignore_hosts,
                       NULL);
}

/**
 * xsimple_proxy_resolver_set_default_proxy:
 * @resolver: a #xsimple_proxy_resolver_t
 * @default_proxy: the default proxy to use
 *
 * Sets the default proxy on @resolver, to be used for any URIs that
 * don't match #xsimple_proxy_resolver_t:ignore-hosts or a proxy set
 * via xsimple_proxy_resolver_set_uri_proxy().
 *
 * If @default_proxy starts with "socks://",
 * #xsimple_proxy_resolver_t will treat it as referring to all three of
 * the socks5, socks4a, and socks4 proxy types.
 *
 * Since: 2.36
 */
void
xsimple_proxy_resolver_set_default_proxy (xsimple_proxy_resolver_t *resolver,
                                           const xchar_t          *default_proxy)
{
  g_return_if_fail (X_IS_SIMPLE_PROXY_RESOLVER (resolver));

  g_free (resolver->priv->default_proxy);
  resolver->priv->default_proxy = xstrdup (default_proxy);
  xobject_notify (G_OBJECT (resolver), "default-proxy");
}

/**
 * xsimple_proxy_resolver_set_ignore_hosts:
 * @resolver: a #xsimple_proxy_resolver_t
 * @ignore_hosts: (array zero-terminated=1): %NULL-terminated list of hosts/IP addresses
 *     to not use a proxy for
 *
 * Sets the list of ignored hosts.
 *
 * See #xsimple_proxy_resolver_t:ignore-hosts for more details on how the
 * @ignore_hosts argument is interpreted.
 *
 * Since: 2.36
 */
void
xsimple_proxy_resolver_set_ignore_hosts (xsimple_proxy_resolver_t  *resolver,
                                          xchar_t                **ignore_hosts)
{
  g_return_if_fail (X_IS_SIMPLE_PROXY_RESOLVER (resolver));

  xstrfreev (resolver->priv->ignore_hosts);
  resolver->priv->ignore_hosts = xstrdupv (ignore_hosts);
  reparse_ignore_hosts (resolver);
  xobject_notify (G_OBJECT (resolver), "ignore-hosts");
}

/**
 * xsimple_proxy_resolver_set_uri_proxy:
 * @resolver: a #xsimple_proxy_resolver_t
 * @uri_scheme: the URI scheme to add a proxy for
 * @proxy: the proxy to use for @uri_scheme
 *
 * Adds a URI-scheme-specific proxy to @resolver; URIs whose scheme
 * matches @uri_scheme (and which don't match
 * #xsimple_proxy_resolver_t:ignore-hosts) will be proxied via @proxy.
 *
 * As with #xsimple_proxy_resolver_t:default-proxy, if @proxy starts with
 * "socks://", #xsimple_proxy_resolver_t will treat it
 * as referring to all three of the socks5, socks4a, and socks4 proxy
 * types.
 *
 * Since: 2.36
 */
void
xsimple_proxy_resolver_set_uri_proxy (xsimple_proxy_resolver_t *resolver,
                                       const xchar_t          *uri_scheme,
                                       const xchar_t          *proxy)
{
  g_return_if_fail (X_IS_SIMPLE_PROXY_RESOLVER (resolver));

  xhash_table_replace (resolver->priv->uri_proxies,
                        g_ascii_strdown (uri_scheme, -1),
                        xstrdup (proxy));
}
