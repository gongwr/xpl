/* gcharset.h - Charset functions
 *
 *  Copyright (C) 2011 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_CHARSET_H__
#define __G_CHARSET_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xboolean_t              g_get_charset         (const char **charset);
XPL_AVAILABLE_IN_ALL
xchar_t *               g_get_codeset         (void);
XPL_AVAILABLE_IN_2_62
xboolean_t              g_get_console_charset (const char **charset);

XPL_AVAILABLE_IN_ALL
const xchar_t * const * g_get_language_names  (void);
XPL_AVAILABLE_IN_2_58
const xchar_t * const * g_get_language_names_with_category
                                            (const xchar_t *category_name);
XPL_AVAILABLE_IN_ALL
xchar_t **              g_get_locale_variants (const xchar_t *locale);

G_END_DECLS

#endif  /* __G_CHARSET_H__ */
