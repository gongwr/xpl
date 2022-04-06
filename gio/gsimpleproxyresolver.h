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
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __XSIMPLE_PROXY_RESOLVER_H__
#define __XSIMPLE_PROXY_RESOLVER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gproxyresolver.h>

G_BEGIN_DECLS

#define XTYPE_SIMPLE_PROXY_RESOLVER         (xsimple_proxy_resolver_get_type ())
#define XSIMPLE_PROXY_RESOLVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SIMPLE_PROXY_RESOLVER, xsimple_proxy_resolver))
#define XSIMPLE_PROXY_RESOLVER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_SIMPLE_PROXY_RESOLVER, xsimple_proxy_resolver_class_t))
#define X_IS_SIMPLE_PROXY_RESOLVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SIMPLE_PROXY_RESOLVER))
#define X_IS_SIMPLE_PROXY_RESOLVER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_SIMPLE_PROXY_RESOLVER))
#define XSIMPLE_PROXY_RESOLVER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_SIMPLE_PROXY_RESOLVER, xsimple_proxy_resolver_class_t))

/**
 * xsimple_proxy_resolver_t:
 *
 * A #xproxy_resolver_t implementation for using a fixed set of proxies.
 **/
typedef struct _GSimpleProxyResolver xsimple_proxy_resolver_t;
typedef struct _xsimple_proxy_resolver_private xsimple_proxy_resolver_private_t;
typedef struct _xsimple_proxy_resolver_class xsimple_proxy_resolver_class_t;

struct _GSimpleProxyResolver
{
  xobject_t parent_instance;

  /*< private >*/
  xsimple_proxy_resolver_private_t *priv;
};

struct _xsimple_proxy_resolver_class
{
  xobject_class_t parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_2_36
xtype_t           xsimple_proxy_resolver_get_type          (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_36
xproxy_resolver_t *xsimple_proxy_resolver_new               (const xchar_t           *default_proxy,
                                                           xchar_t                **ignore_hosts);

XPL_AVAILABLE_IN_2_36
void            xsimple_proxy_resolver_set_default_proxy (xsimple_proxy_resolver_t  *resolver,
                                                           const xchar_t           *default_proxy);

XPL_AVAILABLE_IN_2_36
void            xsimple_proxy_resolver_set_ignore_hosts  (xsimple_proxy_resolver_t  *resolver,
                                                           xchar_t                **ignore_hosts);

XPL_AVAILABLE_IN_2_36
void            xsimple_proxy_resolver_set_uri_proxy     (xsimple_proxy_resolver_t  *resolver,
                                                           const xchar_t           *uri_scheme,
                                                           const xchar_t           *proxy);

G_END_DECLS

#endif /* __XSIMPLE_PROXY_RESOLVER_H__ */
