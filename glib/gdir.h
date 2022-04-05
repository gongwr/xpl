/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gdir.c: Simplified wrapper around the DIRENT functions.
 *
 * Copyright 2001 Hans Breuer
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

#ifndef __G_DIR_H__
#define __G_DIR_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gerror.h>

#ifdef G_OS_UNIX
#include <dirent.h>
#endif

G_BEGIN_DECLS

typedef struct _GDir GDir;

XPL_AVAILABLE_IN_ALL
GDir    *                g_dir_open           (const xchar_t  *path,
					       xuint_t         flags,
					       xerror_t      **error);
XPL_AVAILABLE_IN_ALL
const xchar_t *            g_dir_read_name      (GDir         *dir);
XPL_AVAILABLE_IN_ALL
void                     g_dir_rewind         (GDir         *dir);
XPL_AVAILABLE_IN_ALL
void                     g_dir_close          (GDir         *dir);

G_END_DECLS

#endif /* __G_DIR_H__ */
