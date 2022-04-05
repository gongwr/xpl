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

#ifndef __G_VERSION_H__
#define __G_VERSION_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

XPL_VAR const xuint_t glib_major_version;
XPL_VAR const xuint_t glib_minor_version;
XPL_VAR const xuint_t glib_micro_version;
XPL_VAR const xuint_t glib_interface_age;
XPL_VAR const xuint_t glib_binary_age;

XPL_AVAILABLE_IN_ALL
const xchar_t * glib_check_version (xuint_t required_major,
                                  xuint_t required_minor,
                                  xuint_t required_micro);

#define XPL_CHECK_VERSION(major,minor,micro)    \
    (XPL_MAJOR_VERSION > (major) || \
     (XPL_MAJOR_VERSION == (major) && XPL_MINOR_VERSION > (minor)) || \
     (XPL_MAJOR_VERSION == (major) && XPL_MINOR_VERSION == (minor) && \
      XPL_MICRO_VERSION >= (micro)))

G_END_DECLS

#endif /*  __G_VERSION_H__ */
