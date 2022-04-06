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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __XBYTES_ICON_H__
#define __XBYTES_ICON_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_BYTES_ICON         (xbytes_icon_get_type ())
#define XBYTES_ICON(inst)        (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_BYTES_ICON, xbytes_icon))
#define X_IS_BYTES_ICON(inst)     (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_BYTES_ICON))

/**
 * xbytes_icon_t:
 *
 * Gets an icon for a #xbytes_t. Implements #xloadable_icon_t.
 **/
XPL_AVAILABLE_IN_2_38
xtype_t   xbytes_icon_get_type   (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_38
xicon_t * xbytes_icon_new        (xbytes_t     *bytes);

XPL_AVAILABLE_IN_2_38
xbytes_t * xbytes_icon_get_bytes (xbytes_icon_t *icon);

G_END_DECLS

#endif /* __XBYTES_ICON_H__ */
