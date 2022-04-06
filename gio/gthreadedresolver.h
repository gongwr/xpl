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

#ifndef __G_THREADED_RESOLVER_H__
#define __G_THREADED_RESOLVER_H__

#include <gio/gresolver.h>

G_BEGIN_DECLS

#define XTYPE_THREADED_RESOLVER         (xthreaded_resolver_get_type ())
#define G_THREADED_RESOLVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_THREADED_RESOLVER, GThreadedResolver))
#define G_THREADED_RESOLVER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_THREADED_RESOLVER, GThreadedResolverClass))
#define X_IS_THREADED_RESOLVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_THREADED_RESOLVER))
#define X_IS_THREADED_RESOLVER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_THREADED_RESOLVER))
#define G_THREADED_RESOLVER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_THREADED_RESOLVER, GThreadedResolverClass))

typedef struct {
  xresolver_t parent_instance;
} GThreadedResolver;

typedef struct {
  GResolverClass parent_class;

} GThreadedResolverClass;

XPL_AVAILABLE_IN_ALL
xtype_t xthreaded_resolver_get_type (void) G_GNUC_CONST;

/* Used for a private test API */
XPL_AVAILABLE_IN_ALL
xlist_t *g_resolver_records_from_res_query (const xchar_t      *rrname,
                                          xint_t              rrtype,
                                          const xuint8_t     *answer,
                                          xssize_t            len,
                                          xint_t              herr,
                                          xerror_t          **error);

G_END_DECLS

#endif /* __G_RESOLVER_H__ */
