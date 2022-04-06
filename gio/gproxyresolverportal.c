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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "xdp-dbus.h"
#include "giomodule-priv.h"
#include "gportalsupport.h"
#include "gproxyresolverportal.h"

struct _GProxyResolverPortal {
  xobject_t parent_instance;

  GXdpProxyResolver *resolver;
  xboolean_t network_available;
};

static void xproxy_resolver_portal_iface_init (xproxy_resolver_interface_t *iface);

G_DEFINE_TYPE_WITH_CODE (GProxyResolverPortal, xproxy_resolver_portal, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_PROXY_RESOLVER,
                                                xproxy_resolver_portal_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_PROXY_RESOLVER_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "portal",
                                                         90))

static xboolean_t
ensure_resolver_proxy (GProxyResolverPortal *resolver)
{
  if (resolver->resolver)
    return TRUE;

  if (!glib_should_use_portal ())
    return FALSE;

  resolver->resolver = gxdp_proxy_resolver_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                                   "org.freedesktop.portal.Desktop",
                                                                   "/org/freedesktop/portal/desktop",
                                                                   NULL,
                                                                   NULL);

  resolver->network_available = glib_network_available_in_sandbox ();

  return resolver->resolver != NULL;
}

static void
xproxy_resolver_portal_init (GProxyResolverPortal *resolver)
{
}

static xboolean_t
xproxy_resolver_portal_is_supported (xproxy_resolver_t *object)
{
  GProxyResolverPortal *resolver = G_PROXY_RESOLVER_PORTAL (object);
  char *name_owner;
  xboolean_t has_portal;

  if (!ensure_resolver_proxy (resolver))
    return FALSE;

  name_owner = xdbus_proxy_get_name_owner (G_DBUS_PROXY (resolver->resolver));
  has_portal = name_owner != NULL;
  g_free (name_owner);

  return has_portal;
}

static const char *no_proxy[2] = { "direct://", NULL };

static xchar_t **
xproxy_resolver_portal_lookup (xproxy_resolver_t *proxy_resolver,
                                const xchar_t     *uri,
                                xcancellable_t    *cancellable,
                                xerror_t         **error)
{
  GProxyResolverPortal *resolver = G_PROXY_RESOLVER_PORTAL (proxy_resolver);
  char **proxy = NULL;

  ensure_resolver_proxy (resolver);
  g_assert (resolver->resolver);

  if (!gxdp_proxy_resolver_call_lookup_sync (resolver->resolver,
                                             uri,
                                             &proxy,
                                             cancellable,
                                             error))
    return NULL;

  if (!resolver->network_available)
    {
      xstrfreev (proxy);
      proxy = xstrdupv ((xchar_t **)no_proxy);
    }

  return proxy;
}

static void
lookup_done (xobject_t      *source,
             xasync_result_t *result,
             xpointer_t      data)
{
  xtask_t *task = data;
  xerror_t *error = NULL;
  xchar_t **proxies = NULL;

  if (!gxdp_proxy_resolver_call_lookup_finish (GXDP_PROXY_RESOLVER (source),
                                               &proxies,
                                               result,
                                               &error))
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, proxies, NULL);

  xobject_unref (task);
}

static void
xproxy_resolver_portal_lookup_async (xproxy_resolver_t      *proxy_resolver,
                                      const xchar_t         *uri,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  GProxyResolverPortal *resolver = G_PROXY_RESOLVER_PORTAL (proxy_resolver);
  xtask_t *task;

  ensure_resolver_proxy (resolver);
  g_assert (resolver->resolver);

  task = xtask_new (proxy_resolver, cancellable, callback, user_data);
  gxdp_proxy_resolver_call_lookup (resolver->resolver,
                                   uri,
                                   cancellable,
                                   lookup_done,
                                   xobject_ref (task));
  xobject_unref (task);
}

static xchar_t **
xproxy_resolver_portal_lookup_finish (xproxy_resolver_t  *proxy_resolver,
                                       xasync_result_t    *result,
                                       xerror_t         **error)
{
  GProxyResolverPortal *resolver = G_PROXY_RESOLVER_PORTAL (proxy_resolver);
  xtask_t *task = XTASK (result);
  char **proxies;

  proxies = xtask_propagate_pointer (task, error);
  if (proxies == NULL)
    return NULL;

  if (!resolver->network_available)
    {
      xstrfreev (proxies);
      proxies = xstrdupv ((xchar_t **)no_proxy);
    }

  return proxies;
}

static void
xproxy_resolver_portal_finalize (xobject_t *object)
{
  GProxyResolverPortal *resolver = G_PROXY_RESOLVER_PORTAL (object);

  g_clear_object (&resolver->resolver);

  G_OBJECT_CLASS (xproxy_resolver_portal_parent_class)->finalize (object);
}

static void
xproxy_resolver_portal_class_init (GProxyResolverPortalClass *resolver_class)
{
  xobject_class_t *object_class;

  object_class = G_OBJECT_CLASS (resolver_class);
  object_class->finalize = xproxy_resolver_portal_finalize;
}

static void
xproxy_resolver_portal_iface_init (xproxy_resolver_interface_t *iface)
{
  iface->is_supported = xproxy_resolver_portal_is_supported;
  iface->lookup = xproxy_resolver_portal_lookup;
  iface->lookup_async = xproxy_resolver_portal_lookup_async;
  iface->lookup_finish = xproxy_resolver_portal_lookup_finish;
}
