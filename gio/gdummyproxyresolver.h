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

#ifndef __G_DUMMY_PROXY_RESOLVER_H__
#define __G_DUMMY_PROXY_RESOLVER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DUMMY_PROXY_RESOLVER         (_xdummy_proxy_resolver_get_type ())
#define G_DUMMY_PROXY_RESOLVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DUMMY_PROXY_RESOLVER, xdummy_proxy_resolver_t))
#define G_DUMMY_PROXY_RESOLVER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DUMMY_PROXY_RESOLVER, xdummy_proxy_resolver_class_t))
#define X_IS_DUMMY_PROXY_RESOLVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DUMMY_PROXY_RESOLVER))
#define X_IS_DUMMY_PROXY_RESOLVER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DUMMY_PROXY_RESOLVER))
#define G_DUMMY_PROXY_RESOLVER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DUMMY_PROXY_RESOLVER, xdummy_proxy_resolver_class_t))

typedef struct _xdummy_proxy_resolver       xdummy_proxy_resolver_t;
typedef struct _xdummy_proxy_resolver_class  xdummy_proxy_resolver_class_t;


struct _xdummy_proxy_resolver_class {
  xobject_class_t parent_class;
};

xtype_t		_xdummy_proxy_resolver_get_type       (void);


G_END_DECLS

#endif /* __G_DUMMY_PROXY_RESOLVER_H__ */
