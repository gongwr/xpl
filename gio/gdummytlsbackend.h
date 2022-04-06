/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __G_DUMMY_TLS_BACKEND_H__
#define __G_DUMMY_TLS_BACKEND_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DUMMY_TLS_BACKEND         (_xdummy_tls_backend_get_type ())
#define G_DUMMY_TLS_BACKEND(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DUMMY_TLS_BACKEND, xdummy_tls_backend_t))
#define G_DUMMY_TLS_BACKEND_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DUMMY_TLS_BACKEND, GDummyTlsBackendClass))
#define X_IS_DUMMY_TLS_BACKEND(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DUMMY_TLS_BACKEND))
#define X_IS_DUMMY_TLS_BACKEND_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DUMMY_TLS_BACKEND))
#define G_DUMMY_TLS_BACKEND_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DUMMY_TLS_BACKEND, GDummyTlsBackendClass))

typedef struct _xdummy_tls_backend       xdummy_tls_backend_t;
typedef struct _GDummyTlsBackendClass  GDummyTlsBackendClass;

struct _GDummyTlsBackendClass {
  xobject_class_t parent_class;
};

xtype_t _xdummy_tls_backend_get_type       (void);

G_END_DECLS

#endif /* __G_DUMMY_TLS_BACKEND_H__ */
