/* gbase64.h - Base64 coding functions
 *
 *  Copyright (C) 2005  Alexander Larsson <alexl@redhat.com>
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

#ifndef __G_BASE64_H__
#define __G_BASE64_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xsize_t   g_base64_encode_step    (const xuchar_t *in,
                                 xsize_t         len,
                                 xboolean_t      break_lines,
                                 xchar_t        *out,
                                 xint_t         *state,
                                 xint_t         *save);
XPL_AVAILABLE_IN_ALL
xsize_t   g_base64_encode_close   (xboolean_t      break_lines,
                                 xchar_t        *out,
                                 xint_t         *state,
                                 xint_t         *save);
XPL_AVAILABLE_IN_ALL
xchar_t*  g_base64_encode         (const xuchar_t *data,
                                 xsize_t         len) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xsize_t   g_base64_decode_step    (const xchar_t  *in,
                                 xsize_t         len,
                                 xuchar_t       *out,
                                 xint_t         *state,
                                 xuint_t        *save);
XPL_AVAILABLE_IN_ALL
xuchar_t *g_base64_decode         (const xchar_t  *text,
                                 xsize_t        *out_len) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xuchar_t *g_base64_decode_inplace (xchar_t        *text,
                                 xsize_t        *out_len);


G_END_DECLS

#endif /* __G_BASE64_H__ */
