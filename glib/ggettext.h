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

#ifndef __G_GETTEXT_H__
#define __G_GETTEXT_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
const xchar_t *g_strip_context (const xchar_t *msgid,
                              const xchar_t *msgval) G_GNUC_FORMAT(1);

XPL_AVAILABLE_IN_ALL
const xchar_t *g_dgettext      (const xchar_t *domain,
                              const xchar_t *msgid) G_GNUC_FORMAT(2);
XPL_AVAILABLE_IN_ALL
const xchar_t *g_dcgettext     (const xchar_t *domain,
                              const xchar_t *msgid,
                              xint_t         category) G_GNUC_FORMAT(2);
XPL_AVAILABLE_IN_ALL
const xchar_t *g_dngettext     (const xchar_t *domain,
                              const xchar_t *msgid,
                              const xchar_t *msgid_plural,
                              gulong       n) G_GNUC_FORMAT(3);
XPL_AVAILABLE_IN_ALL
const xchar_t *g_dpgettext     (const xchar_t *domain,
                              const xchar_t *msgctxtid,
                              xsize_t        msgidoffset) G_GNUC_FORMAT(2);
XPL_AVAILABLE_IN_ALL
const xchar_t *g_dpgettext2    (const xchar_t *domain,
                              const xchar_t *context,
                              const xchar_t *msgid) G_GNUC_FORMAT(3);

G_END_DECLS

#endif /* __G_GETTEXT_H__ */
