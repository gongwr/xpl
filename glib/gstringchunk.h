/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_STRINGCHUNK_H__
#define __G_STRINGCHUNK_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

typedef struct _GStringChunk xstring_chunk_t;

XPL_AVAILABLE_IN_ALL
xstring_chunk_t* xstring_chunk_new          (xsize_t size);
XPL_AVAILABLE_IN_ALL
void          xstring_chunk_free         (xstring_chunk_t *chunk);
XPL_AVAILABLE_IN_ALL
void          xstring_chunk_clear        (xstring_chunk_t *chunk);
XPL_AVAILABLE_IN_ALL
xchar_t*        xstring_chunk_insert       (xstring_chunk_t *chunk,
                                           const xchar_t  *string);
XPL_AVAILABLE_IN_ALL
xchar_t*        xstring_chunk_insert_len   (xstring_chunk_t *chunk,
                                           const xchar_t  *string,
                                           xssize_t        len);
XPL_AVAILABLE_IN_ALL
xchar_t*        xstring_chunk_insert_const (xstring_chunk_t *chunk,
                                           const xchar_t  *string);

G_END_DECLS

#endif /* __G_STRING_H__ */
