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

#include "gdummyproxyresolver.h"

#include <glib.h>

#include "gasyncresult.h"
#include "gcancellable.h"
#include "gproxyresolver.h"
#include "gtask.h"

#include "giomodule.h"
#include "giomodule-priv.h"

struct _xdummy_proxy_resolver {
  xobject_t parent_instance;
};

static void xdummy_proxy_resolver_iface_init (xproxy_resolver_interface_t *iface);

#define xdummy_proxy_resolver_get_type _xdummy_proxy_resolver_get_type
G_DEFINE_TYPE_WITH_CODE (xdummy_proxy_resolver, xdummy_proxy_resolver, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_PROXY_RESOLVER,
						xdummy_proxy_resolver_iface_init)
			 _xio_modules_ensure_extension_points_registered ();
			 g_io_extension_point_implement (G_PROXY_RESOLVER_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "dummy",
							 -100))

static void
xdummy_proxy_resolver_finalize (xobject_t *object)
{
  /* must chain up */
  G_OBJECT_CLASS (xdummy_proxy_resolver_parent_class)->finalize (object);
}

static void
xdummy_proxy_resolver_init (xdummy_proxy_resolver_t *resolver)
{
}

static xboolean_t
xdummy_proxy_resolver_is_supported (xproxy_resolver_t *resolver)
{
  return TRUE;
}

static xchar_t **
xdummy_proxy_resolver_lookup (xproxy_resolver_t  *resolver,
			       const xchar_t     *uri,
			       xcancellable_t    *cancellable,
			       xerror_t         **error)
{
  xchar_t **proxies;

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return NULL;

  proxies = g_new0 (xchar_t *, 2);
  proxies[0] = xstrdup ("direct://");

  return proxies;
}

static void
xdummy_proxy_resolver_lookup_async (xproxy_resolver_t      *resolver,
				     const xchar_t         *uri,
				     xcancellable_t        *cancellable,
				     xasync_ready_callback_t  callback,
				     xpointer_t             user_data)
{
  xerror_t *error = NULL;
  xtask_t *task;
  xchar_t **proxies;

  task = xtask_new (resolver, cancellable, callback, user_data);
  xtask_set_source_tag (task, xdummy_proxy_resolver_lookup_async);

  proxies = xdummy_proxy_resolver_lookup (resolver, uri, cancellable, &error);
  if (proxies)
    xtask_return_pointer (task, proxies, (xdestroy_notify_t) xstrfreev);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

static xchar_t **
xdummy_proxy_resolver_lookup_finish (xproxy_resolver_t     *resolver,
				      xasync_result_t       *result,
				      xerror_t            **error)
{
  g_return_val_if_fail (xtask_is_valid (result, resolver), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static void
xdummy_proxy_resolver_class_init (xdummy_proxy_resolver_class_t *resolver_class)
{
  xobject_class_t *object_class;

  object_class = G_OBJECT_CLASS (resolver_class);
  object_class->finalize = xdummy_proxy_resolver_finalize;
}

static void
xdummy_proxy_resolver_iface_init (xproxy_resolver_interface_t *iface)
{
  iface->is_supported = xdummy_proxy_resolver_is_supported;
  iface->lookup = xdummy_proxy_resolver_lookup;
  iface->lookup_async = xdummy_proxy_resolver_lookup_async;
  iface->lookup_finish = xdummy_proxy_resolver_lookup_finish;
}
