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

#ifndef __G_ENVIRON_H__
#define __G_ENVIRON_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
const xchar_t * g_getenv           (const xchar_t  *variable);
XPL_AVAILABLE_IN_ALL
xboolean_t      g_setenv           (const xchar_t  *variable,
                                  const xchar_t  *value,
                                  xboolean_t      overwrite);
XPL_AVAILABLE_IN_ALL
void          g_unsetenv         (const xchar_t  *variable);
XPL_AVAILABLE_IN_ALL
xchar_t **      g_listenv          (void);

XPL_AVAILABLE_IN_ALL
xchar_t **      g_get_environ      (void);
XPL_AVAILABLE_IN_ALL
const xchar_t * g_environ_getenv   (xchar_t       **envp,
                                  const xchar_t  *variable);
XPL_AVAILABLE_IN_ALL
xchar_t **      g_environ_setenv   (xchar_t       **envp,
                                  const xchar_t  *variable,
                                  const xchar_t  *value,
                                  xboolean_t      overwrite) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xchar_t **      g_environ_unsetenv (xchar_t       **envp,
                                  const xchar_t  *variable) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __G_ENVIRON_H__ */
