/* XPL - Library of useful routines for C programming
 * gmappedfile.h: Simplified wrapper around the mmap function
 *
 * Copyright 2005 Matthias Clasen
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_MAPPED_FILE_H__
#define __G_MAPPED_FILE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gbytes.h>
#include <glib/gerror.h>

G_BEGIN_DECLS

typedef struct _GMappedFile xmapped_file_t;

XPL_AVAILABLE_IN_ALL
xmapped_file_t *xmapped_file_new          (const xchar_t  *filename,
				         xboolean_t      writable,
				         xerror_t      **error);
XPL_AVAILABLE_IN_ALL
xmapped_file_t *xmapped_file_new_from_fd  (xint_t          fd,
					 xboolean_t      writable,
					 xerror_t      **error);
XPL_AVAILABLE_IN_ALL
xsize_t        xmapped_file_get_length   (xmapped_file_t  *file);
XPL_AVAILABLE_IN_ALL
xchar_t       *xmapped_file_get_contents (xmapped_file_t  *file);
XPL_AVAILABLE_IN_2_34
xbytes_t *     xmapped_file_get_bytes    (xmapped_file_t  *file);
XPL_AVAILABLE_IN_ALL
xmapped_file_t *xmapped_file_ref          (xmapped_file_t  *file);
XPL_AVAILABLE_IN_ALL
void         xmapped_file_unref        (xmapped_file_t  *file);

XPL_DEPRECATED_FOR(xmapped_file_unref)
void         xmapped_file_free         (xmapped_file_t  *file);

G_END_DECLS

#endif /* __G_MAPPED_FILE_H__ */
