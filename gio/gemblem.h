/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Clemens N. Buss <cebuzz@gmail.com>
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
 */

#ifndef __XEMBLEM_H__
#define __XEMBLEM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gioenums.h>

G_BEGIN_DECLS

#define XTYPE_EMBLEM         (xemblem_get_type ())
#define XEMBLEM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_EMBLEM, xemblem_t))
#define XEMBLEM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_EMBLEM, xemblem_class_t))
#define X_IS_EMBLEM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_EMBLEM))
#define X_IS_EMBLEM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_EMBLEM))
#define XEMBLEM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_EMBLEM, xemblem_class_t))

/**
 * xemblem_t:
 *
 * An object for Emblems
 */
typedef struct _xemblem        xemblem_t;
typedef struct _xemblem_class   xemblem_class_t;

XPL_AVAILABLE_IN_ALL
xtype_t          xemblem_get_type        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xemblem_t       *xemblem_new             (xicon_t         *icon);
XPL_AVAILABLE_IN_ALL
xemblem_t       *xemblem_new_with_origin (xicon_t         *icon,
                                         GEmblemOrigin  origin);
XPL_AVAILABLE_IN_ALL
xicon_t         *xemblem_get_icon        (xemblem_t       *emblem);
XPL_AVAILABLE_IN_ALL
GEmblemOrigin  xemblem_get_origin      (xemblem_t       *emblem);

G_END_DECLS

#endif /* __XEMBLEM_H__ */
