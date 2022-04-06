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

#ifndef __G_PROXY_ADDRESS_ENUMERATOR_H__
#define __G_PROXY_ADDRESS_ENUMERATOR_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gsocketaddressenumerator.h>

G_BEGIN_DECLS

#define XTYPE_PROXY_ADDRESS_ENUMERATOR         (xproxy_address_enumerator_get_type ())
#define G_PROXY_ADDRESS_ENUMERATOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_PROXY_ADDRESS_ENUMERATOR, xproxy_address_enumerator))
#define G_PROXY_ADDRESS_ENUMERATOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_PROXY_ADDRESS_ENUMERATOR, GProxyAddressEnumeratorClass))
#define X_IS_PROXY_ADDRESS_ENUMERATOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_PROXY_ADDRESS_ENUMERATOR))
#define X_IS_PROXY_ADDRESS_ENUMERATOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_PROXY_ADDRESS_ENUMERATOR))
#define G_PROXY_ADDRESS_ENUMERATOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_PROXY_ADDRESS_ENUMERATOR, GProxyAddressEnumeratorClass))

/**
 * xproxy_address_enumerator_t:
 *
 * A subclass of #xsocket_address_enumerator_t that takes another address
 * enumerator and wraps each of its results in a #xproxy_address_t as
 * directed by the default #xproxy_resolver_t.
 */

typedef struct _GProxyAddressEnumeratorClass GProxyAddressEnumeratorClass;
typedef struct _GProxyAddressEnumeratorPrivate GProxyAddressEnumeratorPrivate;

struct _GProxyAddressEnumerator
{
  /*< private >*/
  xsocket_address_enumerator_t parent_instance;
  GProxyAddressEnumeratorPrivate *priv;
};

/**
 * GProxyAddressEnumeratorClass:
 *
 * Class structure for #xproxy_address_enumerator_t.
 */
struct _GProxyAddressEnumeratorClass
{
  /*< private >*/
  GSocketAddressEnumeratorClass parent_class;

  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
  void (*_g_reserved7) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t           xproxy_address_enumerator_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __G_PROXY_ADDRESS_ENUMERATOR_H__ */
