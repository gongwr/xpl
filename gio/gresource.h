/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_RESOURCE_H__
#define __G_RESOURCE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * XTYPE_RESOURCE:
 *
 * The #xtype_t for #xresource_t.
 */
#define XTYPE_RESOURCE (g_resource_get_type ())


/**
 * G_RESOURCE_ERROR:
 *
 * Error domain for #xresource_t. Errors in this domain will be from the
 * #GResourceError enumeration. See #xerror_t for more information on
 * error domains.
 */
#define G_RESOURCE_ERROR (g_resource_error_quark ())
XPL_AVAILABLE_IN_2_32
xquark g_resource_error_quark (void);

typedef struct _GStaticResource GStaticResource;

struct _GStaticResource {
  /*< private >*/
  const xuint8_t *data;
  xsize_t data_len;
  xresource_t *resource;
  GStaticResource *next;
  xpointer_t padding;
};

XPL_AVAILABLE_IN_2_32
xtype_t         g_resource_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_32
xresource_t *   g_resource_new_from_data       (xbytes_t                *data,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
xresource_t *   g_resource_ref                 (xresource_t             *resource);
XPL_AVAILABLE_IN_2_32
void          g_resource_unref               (xresource_t             *resource);
XPL_AVAILABLE_IN_2_32
xresource_t *   g_resource_load                (const xchar_t           *filename,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
xinput_stream_t *g_resource_open_stream         (xresource_t             *resource,
					      const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
xbytes_t *      g_resource_lookup_data         (xresource_t             *resource,
					      const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
char **       g_resource_enumerate_children  (xresource_t             *resource,
					      const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
xboolean_t      g_resource_get_info            (xresource_t             *resource,
					      const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xsize_t                 *size,
					      xuint32_t               *flags,
					      xerror_t               **error);

XPL_AVAILABLE_IN_2_32
void          g_resources_register           (xresource_t             *resource);
XPL_AVAILABLE_IN_2_32
void          g_resources_unregister         (xresource_t             *resource);
XPL_AVAILABLE_IN_2_32
xinput_stream_t *g_resources_open_stream        (const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
xbytes_t *      g_resources_lookup_data        (const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
char **       g_resources_enumerate_children (const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xerror_t               **error);
XPL_AVAILABLE_IN_2_32
xboolean_t      g_resources_get_info           (const char            *path,
					      GResourceLookupFlags   lookup_flags,
					      xsize_t                 *size,
					      xuint32_t               *flags,
					      xerror_t               **error);


XPL_AVAILABLE_IN_2_32
void          g_static_resource_init          (GStaticResource *static_resource);
XPL_AVAILABLE_IN_2_32
void          g_static_resource_fini          (GStaticResource *static_resource);
XPL_AVAILABLE_IN_2_32
xresource_t    *g_static_resource_get_resource  (GStaticResource *static_resource);

G_END_DECLS

#endif /* __G_RESOURCE_H__ */
