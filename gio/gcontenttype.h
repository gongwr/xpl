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

#ifndef __G_CONTENT_TYPE_H__
#define __G_CONTENT_TYPE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xboolean_t g_content_type_equals            (const xchar_t  *type1,
                                           const xchar_t  *type2);
XPL_AVAILABLE_IN_ALL
xboolean_t g_content_type_is_a              (const xchar_t  *type,
                                           const xchar_t  *supertype);
XPL_AVAILABLE_IN_2_52
xboolean_t g_content_type_is_mime_type      (const xchar_t *type,
                                           const xchar_t *mime_type);
XPL_AVAILABLE_IN_ALL
xboolean_t g_content_type_is_unknown        (const xchar_t  *type);
XPL_AVAILABLE_IN_ALL
xchar_t *  g_content_type_get_description   (const xchar_t  *type);
XPL_AVAILABLE_IN_ALL
xchar_t *  g_content_type_get_mime_type     (const xchar_t  *type);
XPL_AVAILABLE_IN_ALL
xicon_t *  g_content_type_get_icon          (const xchar_t  *type);
XPL_AVAILABLE_IN_2_34
xicon_t *  g_content_type_get_symbolic_icon (const xchar_t  *type);
XPL_AVAILABLE_IN_2_34
xchar_t *  g_content_type_get_generic_icon_name (const xchar_t  *type);

XPL_AVAILABLE_IN_ALL
xboolean_t g_content_type_can_be_executable (const xchar_t  *type);

XPL_AVAILABLE_IN_ALL
xchar_t *  g_content_type_from_mime_type    (const xchar_t  *mime_type);

XPL_AVAILABLE_IN_ALL
xchar_t *  g_content_type_guess             (const xchar_t  *filename,
                                           const guchar *data,
                                           xsize_t         data_size,
                                           xboolean_t     *result_uncertain);

XPL_AVAILABLE_IN_ALL
xchar_t ** g_content_type_guess_for_tree    (xfile_t        *root);

XPL_AVAILABLE_IN_ALL
xlist_t *  g_content_types_get_registered   (void);

/*< private >*/
#ifndef __GTK_DOC_IGNORE__
XPL_AVAILABLE_IN_2_60
const xchar_t * const *g_content_type_get_mime_dirs (void);
XPL_AVAILABLE_IN_2_60
void                 g_content_type_set_mime_dirs (const xchar_t * const *dirs);
#endif /* __GTK_DOC_IGNORE__ */

G_END_DECLS

#endif /* __G_CONTENT_TYPE_H__ */
