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

#ifndef __G_THEMED_ICON_H__
#define __G_THEMED_ICON_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_THEMED_ICON         (g_themed_icon_get_type ())
#define G_THEMED_ICON(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_THEMED_ICON, GThemedIcon))
#define G_THEMED_ICON_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_THEMED_ICON, GThemedIconClass))
#define X_IS_THEMED_ICON(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_THEMED_ICON))
#define X_IS_THEMED_ICON_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_THEMED_ICON))
#define G_THEMED_ICON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_THEMED_ICON, GThemedIconClass))

/**
 * GThemedIcon:
 *
 * An implementation of #xicon_t for themed icons.
 **/
typedef struct _GThemedIconClass   GThemedIconClass;

XPL_AVAILABLE_IN_ALL
xtype_t  g_themed_icon_get_type                   (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xicon_t *g_themed_icon_new                        (const char  *iconname);
XPL_AVAILABLE_IN_ALL
xicon_t *g_themed_icon_new_with_default_fallbacks (const char  *iconname);
XPL_AVAILABLE_IN_ALL
xicon_t *g_themed_icon_new_from_names             (char       **iconnames,
                                                 int          len);
XPL_AVAILABLE_IN_ALL
void   g_themed_icon_prepend_name               (GThemedIcon *icon,
                                                 const char  *iconname);
XPL_AVAILABLE_IN_ALL
void   g_themed_icon_append_name                (GThemedIcon *icon,
                                                 const char  *iconname);

XPL_AVAILABLE_IN_ALL
const xchar_t* const * g_themed_icon_get_names     (GThemedIcon *icon);

G_END_DECLS

#endif /* __G_THEMED_ICON_H__ */
