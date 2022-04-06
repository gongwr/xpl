/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#include "gnetworkservice.h"

#include "gcancellable.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gioerror.h"
#include "gnetworkaddress.h"
#include "gnetworkingprivate.h"
#include "gresolver.h"
#include "gtask.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"
#include "gsrvtarget.h"

#include <stdlib.h>
#include <string.h>


/**
 * SECTION:gnetworkservice
 * @short_description: A xsocket_connectable_t for resolving SRV records
 * @include: gio/gio.h
 *
 * Like #xnetwork_address_t does with hostnames, #xnetwork_service_t
 * provides an easy way to resolve a SRV record, and then attempt to
 * connect to one of the hosts that implements that service, handling
 * service priority/weighting, multiple IP addresses, and multiple
 * address families.
 *
 * See #xsrv_target_t for more information about SRV records, and see
 * #xsocket_connectable_t for an example of using the connectable
 * interface.
 */

/**
 * xnetwork_service_t:
 *
 * A #xsocket_connectable_t for resolving a SRV record and connecting to
 * that service.
 */

struct _GNetworkServicePrivate
{
  xchar_t *service, *protocol, *domain, *scheme;
  xlist_t *targets;
};

enum {
  PROP_0,
  PROP_SERVICE,
  PROP_PROTOCOL,
  PROP_DOMAIN,
  PROP_SCHEME
};

static void g_network_service_set_property (xobject_t      *object,
                                            xuint_t         prop_id,
                                            const xvalue_t *value,
                                            xparam_spec_t   *pspec);
static void g_network_service_get_property (xobject_t      *object,
                                            xuint_t         prop_id,
                                            xvalue_t       *value,
                                            xparam_spec_t   *pspec);

static void                      g_network_service_connectable_iface_init       (xsocket_connectable_iface_t *iface);
static xsocket_address_enumerator_t *g_network_service_connectable_enumerate        (xsocket_connectable_t      *connectable);
static xsocket_address_enumerator_t *g_network_service_connectable_proxy_enumerate  (xsocket_connectable_t      *connectable);
static xchar_t                    *g_network_service_connectable_to_string        (xsocket_connectable_t      *connectable);

G_DEFINE_TYPE_WITH_CODE (xnetwork_service, g_network_service, XTYPE_OBJECT,
                         G_ADD_PRIVATE (xnetwork_service_t)
                         G_IMPLEMENT_INTERFACE (XTYPE_SOCKET_CONNECTABLE,
                                                g_network_service_connectable_iface_init))

static void
g_network_service_finalize (xobject_t *object)
{
  xnetwork_service_t *srv = G_NETWORK_SERVICE (object);

  g_free (srv->priv->service);
  g_free (srv->priv->protocol);
  g_free (srv->priv->domain);
  g_free (srv->priv->scheme);

  if (srv->priv->targets)
    g_resolver_free_targets (srv->priv->targets);

  G_OBJECT_CLASS (g_network_service_parent_class)->finalize (object);
}

static void
g_network_service_class_init (GNetworkServiceClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = g_network_service_set_property;
  gobject_class->get_property = g_network_service_get_property;
  gobject_class->finalize = g_network_service_finalize;

  xobject_class_install_property (gobject_class, PROP_SERVICE,
                                   g_param_spec_string ("service",
                                                        P_("Service"),
                                                        P_("Service name, eg \"ldap\""),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  xobject_class_install_property (gobject_class, PROP_PROTOCOL,
                                   g_param_spec_string ("protocol",
                                                        P_("Protocol"),
                                                        P_("Network protocol, eg \"tcp\""),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  xobject_class_install_property (gobject_class, PROP_DOMAIN,
                                   g_param_spec_string ("domain",
                                                        P_("Domain"),
                                                        P_("Network domain, eg, \"example.com\""),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  xobject_class_install_property (gobject_class, PROP_DOMAIN,
                                   g_param_spec_string ("scheme",
                                                        P_("Scheme"),
                                                        P_("Network scheme (default is to use service)"),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

}

static void
g_network_service_connectable_iface_init (xsocket_connectable_iface_t *connectable_iface)
{
  connectable_iface->enumerate = g_network_service_connectable_enumerate;
  connectable_iface->proxy_enumerate = g_network_service_connectable_proxy_enumerate;
  connectable_iface->to_string = g_network_service_connectable_to_string;
}

static void
g_network_service_init (xnetwork_service_t *srv)
{
  srv->priv = g_network_service_get_instance_private (srv);
}

static void
g_network_service_set_property (xobject_t      *object,
                                xuint_t         prop_id,
                                const xvalue_t *value,
                                xparam_spec_t   *pspec)
{
  xnetwork_service_t *srv = G_NETWORK_SERVICE (object);

  switch (prop_id)
    {
    case PROP_SERVICE:
      srv->priv->service = xvalue_dup_string (value);
      break;

    case PROP_PROTOCOL:
      srv->priv->protocol = xvalue_dup_string (value);
      break;

    case PROP_DOMAIN:
      srv->priv->domain = xvalue_dup_string (value);
      break;

    case PROP_SCHEME:
      g_network_service_set_scheme (srv, xvalue_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_network_service_get_property (xobject_t    *object,
                                xuint_t       prop_id,
                                xvalue_t     *value,
                                xparam_spec_t *pspec)
{
  xnetwork_service_t *srv = G_NETWORK_SERVICE (object);

  switch (prop_id)
    {
    case PROP_SERVICE:
      xvalue_set_string (value, g_network_service_get_service (srv));
      break;

    case PROP_PROTOCOL:
      xvalue_set_string (value, g_network_service_get_protocol (srv));
      break;

    case PROP_DOMAIN:
      xvalue_set_string (value, g_network_service_get_domain (srv));
      break;

    case PROP_SCHEME:
      xvalue_set_string (value, g_network_service_get_scheme (srv));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * g_network_service_new:
 * @service: the service type to look up (eg, "ldap")
 * @protocol: the networking protocol to use for @service (eg, "tcp")
 * @domain: the DNS domain to look up the service in
 *
 * Creates a new #xnetwork_service_t representing the given @service,
 * @protocol, and @domain. This will initially be unresolved; use the
 * #xsocket_connectable_t interface to resolve it.
 *
 * Returns: (transfer full) (type xnetwork_service_t): a new #xnetwork_service_t
 *
 * Since: 2.22
 */
xsocket_connectable_t *
g_network_service_new (const xchar_t *service,
                       const xchar_t *protocol,
                       const xchar_t *domain)
{
  return xobject_new (XTYPE_NETWORK_SERVICE,
                       "service", service,
                       "protocol", protocol,
                       "domain", domain,
                       NULL);
}

/**
 * g_network_service_get_service:
 * @srv: a #xnetwork_service_t
 *
 * Gets @srv's service name (eg, "ldap").
 *
 * Returns: @srv's service name
 *
 * Since: 2.22
 */
const xchar_t *
g_network_service_get_service (xnetwork_service_t *srv)
{
  g_return_val_if_fail (X_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->service;
}

/**
 * g_network_service_get_protocol:
 * @srv: a #xnetwork_service_t
 *
 * Gets @srv's protocol name (eg, "tcp").
 *
 * Returns: @srv's protocol name
 *
 * Since: 2.22
 */
const xchar_t *
g_network_service_get_protocol (xnetwork_service_t *srv)
{
  g_return_val_if_fail (X_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->protocol;
}

/**
 * g_network_service_get_domain:
 * @srv: a #xnetwork_service_t
 *
 * Gets the domain that @srv serves. This might be either UTF-8 or
 * ASCII-encoded, depending on what @srv was created with.
 *
 * Returns: @srv's domain name
 *
 * Since: 2.22
 */
const xchar_t *
g_network_service_get_domain (xnetwork_service_t *srv)
{
  g_return_val_if_fail (X_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->domain;
}

/**
 * g_network_service_get_scheme:
 * @srv: a #xnetwork_service_t
 *
 * Gets the URI scheme used to resolve proxies. By default, the service name
 * is used as scheme.
 *
 * Returns: @srv's scheme name
 *
 * Since: 2.26
 */
const xchar_t *
g_network_service_get_scheme (xnetwork_service_t *srv)
{
  g_return_val_if_fail (X_IS_NETWORK_SERVICE (srv), NULL);

  if (srv->priv->scheme)
    return srv->priv->scheme;
  else
    return srv->priv->service;
}

/**
 * g_network_service_set_scheme:
 * @srv: a #xnetwork_service_t
 * @scheme: a URI scheme
 *
 * Set's the URI scheme used to resolve proxies. By default, the service name
 * is used as scheme.
 *
 * Since: 2.26
 */
void
g_network_service_set_scheme (xnetwork_service_t *srv,
                              const xchar_t     *scheme)
{
  g_return_if_fail (X_IS_NETWORK_SERVICE (srv));

  g_free (srv->priv->scheme);
  srv->priv->scheme = xstrdup (scheme);

  xobject_notify (G_OBJECT (srv), "scheme");
}

static xlist_t *
g_network_service_fallback_targets (xnetwork_service_t *srv)
{
  xsrv_target_t *target;
  struct servent *entry;
  xuint16_t port;

  entry = getservbyname (srv->priv->service, "tcp");
  port = entry ? g_ntohs (entry->s_port) : 0;
#ifdef HAVE_ENDSERVENT
  endservent ();
#endif

  if (entry == NULL)
      return NULL;

  target = g_srv_target_new (srv->priv->domain, port, 0, 0);
  return xlist_append (NULL, target);
}

#define XTYPE_NETWORK_SERVICE_ADDRESS_ENUMERATOR (xnetwork_service_address_enumerator_get_type ())
#define G_NETWORK_SERVICE_ADDRESS_ENUMERATOR(obj) (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_NETWORK_SERVICE_ADDRESS_ENUMERATOR, xnetwork_service_address_enumerator))

typedef struct {
  xsocket_address_enumerator_t parent_instance;

  xresolver_t *resolver;
  xnetwork_service_t *srv;
  xsocket_address_enumerator_t *addr_enum;
  xlist_t *t;
  xboolean_t use_proxy;

  xerror_t *error;

} xnetwork_service_address_enumerator_t;

typedef struct {
  GSocketAddressEnumeratorClass parent_class;

} xnetwork_service_address_enumerator_class_t;

static xtype_t xnetwork_service_address_enumerator_get_type (void);
G_DEFINE_TYPE (xnetwork_service_address_enumerator_t, xnetwork_service_address_enumerator, XTYPE_SOCKET_ADDRESS_ENUMERATOR)

static xsocket_address_t *
g_network_service_address_enumerator_next (xsocket_address_enumerator_t  *enumerator,
                                           xcancellable_t              *cancellable,
                                           xerror_t                   **error)
{
  xnetwork_service_address_enumerator_t *srv_enum =
    G_NETWORK_SERVICE_ADDRESS_ENUMERATOR (enumerator);
  xsocket_address_t *ret = NULL;

  /* If we haven't yet resolved srv, do that */
  if (!srv_enum->srv->priv->targets)
    {
      xlist_t *targets;
      xerror_t *my_error = NULL;

      targets = g_resolver_lookup_service (srv_enum->resolver,
                                           srv_enum->srv->priv->service,
                                           srv_enum->srv->priv->protocol,
                                           srv_enum->srv->priv->domain,
                                           cancellable, &my_error);
      if (!targets && xerror_matches (my_error, G_RESOLVER_ERROR,
                                       G_RESOLVER_ERROR_NOT_FOUND))
        {
          targets = g_network_service_fallback_targets (srv_enum->srv);
          if (targets)
            g_clear_error (&my_error);
        }

      if (my_error)
        {
          g_propagate_error (error, my_error);
          return NULL;
        }

      srv_enum->srv->priv->targets = targets;
      srv_enum->t = srv_enum->srv->priv->targets;
    }

  /* Delegate to xnetwork_address_t */
  do
    {
      if (srv_enum->addr_enum == NULL && srv_enum->t)
        {
          xerror_t *error = NULL;
          xchar_t *uri;
          xchar_t *hostname;
          xsocket_connectable_t *addr;
          xsrv_target_t *target = srv_enum->t->data;

          srv_enum->t = xlist_next (srv_enum->t);

          hostname = g_hostname_to_ascii (g_srv_target_get_hostname (target));

          if (hostname == NULL)
            {
              if (srv_enum->error == NULL)
                srv_enum->error =
                  xerror_new (G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                               "Received invalid hostname '%s' from xsrv_target_t",
                               g_srv_target_get_hostname (target));
              continue;
            }

          uri = xuri_join (XURI_FLAGS_NONE,
                            g_network_service_get_scheme (srv_enum->srv),
                            NULL,
                            hostname,
                            g_srv_target_get_port (target),
                            "",
                            NULL,
                            NULL);
          g_free (hostname);

          addr = g_network_address_parse_uri (uri,
                                              g_srv_target_get_port (target),
                                              &error);
          g_free (uri);

          if (addr == NULL)
            {
              if (srv_enum->error == NULL)
                srv_enum->error = error;
              else
                xerror_free (error);
              continue;
            }

          if (srv_enum->use_proxy)
            srv_enum->addr_enum = xsocket_connectable_proxy_enumerate (addr);
          else
            srv_enum->addr_enum = xsocket_connectable_enumerate (addr);
          xobject_unref (addr);
        }

      if (srv_enum->addr_enum)
        {
          xerror_t *error = NULL;

          ret = xsocket_address_enumerator_next (srv_enum->addr_enum,
                                                  cancellable,
                                                  &error);

          if (error)
            {
              if (srv_enum->error == NULL)
                srv_enum->error = error;
              else
                xerror_free (error);
            }

          if (!ret)
            {
              xobject_unref (srv_enum->addr_enum);
              srv_enum->addr_enum = NULL;
            }
        }
    }
  while (srv_enum->addr_enum == NULL && srv_enum->t);

  if (ret == NULL && srv_enum->error)
    {
      g_propagate_error (error, srv_enum->error);
      srv_enum->error = NULL;
    }

  return ret;
}

static void next_async_resolved_targets   (xobject_t      *source_object,
                                           xasync_result_t *result,
                                           xpointer_t      user_data);
static void next_async_have_targets       (xtask_t        *srv_enum);
static void next_async_have_address       (xobject_t      *source_object,
                                           xasync_result_t *result,
                                           xpointer_t      user_data);

static void
g_network_service_address_enumerator_next_async (xsocket_address_enumerator_t  *enumerator,
                                                 xcancellable_t              *cancellable,
                                                 xasync_ready_callback_t        callback,
                                                 xpointer_t                   user_data)
{
  xnetwork_service_address_enumerator_t *srv_enum =
    G_NETWORK_SERVICE_ADDRESS_ENUMERATOR (enumerator);
  xtask_t *task;

  task = xtask_new (enumerator, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_network_service_address_enumerator_next_async);

  /* If we haven't yet resolved srv, do that */
  if (!srv_enum->srv->priv->targets)
    {
      g_resolver_lookup_service_async (srv_enum->resolver,
                                       srv_enum->srv->priv->service,
                                       srv_enum->srv->priv->protocol,
                                       srv_enum->srv->priv->domain,
                                       cancellable,
                                       next_async_resolved_targets,
                                       task);
    }
  else
    next_async_have_targets (task);
}

static void
next_async_resolved_targets (xobject_t      *source_object,
                             xasync_result_t *result,
                             xpointer_t      user_data)
{
  xtask_t *task = user_data;
  xnetwork_service_address_enumerator_t *srv_enum = xtask_get_source_object (task);
  xerror_t *error = NULL;
  xlist_t *targets;

  targets = g_resolver_lookup_service_finish (srv_enum->resolver,
                                              result, &error);

  if (!targets && xerror_matches (error, G_RESOLVER_ERROR,
                                   G_RESOLVER_ERROR_NOT_FOUND))
    {
      targets = g_network_service_fallback_targets (srv_enum->srv);
      if (targets)
        g_clear_error (&error);
    }

  if (error)
    {
      xtask_return_error (task, error);
      xobject_unref (task);
    }
  else
    {
      srv_enum->t = srv_enum->srv->priv->targets = targets;
      next_async_have_targets (task);
    }
}

static void
next_async_have_targets (xtask_t *task)
{
  xnetwork_service_address_enumerator_t *srv_enum = xtask_get_source_object (task);

  /* Delegate to xnetwork_address_t */
  if (srv_enum->addr_enum == NULL && srv_enum->t)
    {
      xsocket_connectable_t *addr;
      xsrv_target_t *target = srv_enum->t->data;

      srv_enum->t = xlist_next (srv_enum->t);
      addr = g_network_address_new (g_srv_target_get_hostname (target),
                                    (xuint16_t) g_srv_target_get_port (target));

      if (srv_enum->use_proxy)
        srv_enum->addr_enum = xsocket_connectable_proxy_enumerate (addr);
      else
        srv_enum->addr_enum = xsocket_connectable_enumerate (addr);

      xobject_unref (addr);
    }

  if (srv_enum->addr_enum)
    {
      xsocket_address_enumerator_next_async (srv_enum->addr_enum,
                                              xtask_get_cancellable (task),
                                              next_async_have_address,
                                              task);
    }
  else
    {
      if (srv_enum->error)
        {
          xtask_return_error (task, srv_enum->error);
          srv_enum->error = NULL;
        }
      else
        xtask_return_pointer (task, NULL, NULL);

      xobject_unref (task);
    }
}

static void
next_async_have_address (xobject_t      *source_object,
                         xasync_result_t *result,
                         xpointer_t      user_data)
{
  xtask_t *task = user_data;
  xnetwork_service_address_enumerator_t *srv_enum = xtask_get_source_object (task);
  xsocket_address_t *address;
  xerror_t *error = NULL;

  address = xsocket_address_enumerator_next_finish (srv_enum->addr_enum,
                                                     result,
                                                     &error);

  if (error)
    {
      if (srv_enum->error == NULL)
        srv_enum->error = error;
      else
        xerror_free (error);
    }

  if (!address)
    {
      xobject_unref (srv_enum->addr_enum);
      srv_enum->addr_enum = NULL;

      next_async_have_targets (task);
    }
  else
    {
      xtask_return_pointer (task, address, xobject_unref);
      xobject_unref (task);
    }
}

static xsocket_address_t *
g_network_service_address_enumerator_next_finish (xsocket_address_enumerator_t  *enumerator,
                                                  xasync_result_t              *result,
                                                  xerror_t                   **error)
{
  return xtask_propagate_pointer (XTASK (result), error);
}

static void
xnetwork_service_address_enumerator_init (xnetwork_service_address_enumerator_t *enumerator)
{
}

static void
g_network_service_address_enumerator_finalize (xobject_t *object)
{
  xnetwork_service_address_enumerator_t *srv_enum =
    G_NETWORK_SERVICE_ADDRESS_ENUMERATOR (object);

  if (srv_enum->srv)
    xobject_unref (srv_enum->srv);

  if (srv_enum->addr_enum)
    xobject_unref (srv_enum->addr_enum);

  if (srv_enum->resolver)
    xobject_unref (srv_enum->resolver);

  if (srv_enum->error)
    xerror_free (srv_enum->error);

  G_OBJECT_CLASS (xnetwork_service_address_enumerator_parent_class)->finalize (object);
}

static void
xnetwork_service_address_enumerator_class_init (xnetwork_service_address_enumerator_class_t *srvenum_class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (srvenum_class);
  GSocketAddressEnumeratorClass *enumerator_class =
    XSOCKET_ADDRESS_ENUMERATOR_CLASS (srvenum_class);

  enumerator_class->next        = g_network_service_address_enumerator_next;
  enumerator_class->next_async  = g_network_service_address_enumerator_next_async;
  enumerator_class->next_finish = g_network_service_address_enumerator_next_finish;

  object_class->finalize = g_network_service_address_enumerator_finalize;
}

static xsocket_address_enumerator_t *
g_network_service_connectable_enumerate (xsocket_connectable_t *connectable)
{
  xnetwork_service_address_enumerator_t *srv_enum;

  srv_enum = xobject_new (XTYPE_NETWORK_SERVICE_ADDRESS_ENUMERATOR, NULL);
  srv_enum->srv = xobject_ref (G_NETWORK_SERVICE (connectable));
  srv_enum->resolver = g_resolver_get_default ();
  srv_enum->use_proxy = FALSE;

  return XSOCKET_ADDRESS_ENUMERATOR (srv_enum);
}

static xsocket_address_enumerator_t *
g_network_service_connectable_proxy_enumerate (xsocket_connectable_t *connectable)
{
  xsocket_address_enumerator_t *addr_enum;
  xnetwork_service_address_enumerator_t *srv_enum;

  addr_enum = g_network_service_connectable_enumerate (connectable);
  srv_enum = G_NETWORK_SERVICE_ADDRESS_ENUMERATOR (addr_enum);
  srv_enum->use_proxy = TRUE;

  return addr_enum;
}

static xchar_t *
g_network_service_connectable_to_string (xsocket_connectable_t *connectable)
{
  xnetwork_service_t *service;

  service = G_NETWORK_SERVICE (connectable);

  return xstrdup_printf ("(%s, %s, %s, %s)", service->priv->service,
                          service->priv->protocol, service->priv->domain,
                          service->priv->scheme);
}
