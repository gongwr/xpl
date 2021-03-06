/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997, 2002  Peter Mattis, Red Hat, Inc.
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

#ifndef __G_PRINTF_H__
#define __G_PRINTF_H__

#include <glib.h>
#include <stdio.h>
#include <stdarg.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xint_t                  g_printf    (xchar_t const *format,
                                   ...) G_GNUC_PRINTF (1, 2);
XPL_AVAILABLE_IN_ALL
xint_t                  g_fprintf   (FILE        *file,
				   xchar_t const *format,
				   ...) G_GNUC_PRINTF (2, 3);
XPL_AVAILABLE_IN_ALL
xint_t                  g_sprintf   (xchar_t       *string,
				   xchar_t const *format,
				   ...) G_GNUC_PRINTF (2, 3);

XPL_AVAILABLE_IN_ALL
xint_t                  g_vprintf   (xchar_t const *format,
                                   va_list      args) G_GNUC_PRINTF(1, 0);
XPL_AVAILABLE_IN_ALL
xint_t                  g_vfprintf  (FILE        *file,
				   xchar_t const *format,
				   va_list      args) G_GNUC_PRINTF(2, 0);
XPL_AVAILABLE_IN_ALL
xint_t                  g_vsprintf  (xchar_t       *string,
				   xchar_t const *format,
				   va_list      args) G_GNUC_PRINTF(2, 0);
XPL_AVAILABLE_IN_ALL
xint_t                  g_vasprintf (xchar_t      **string,
				   xchar_t const *format,
				   va_list      args) G_GNUC_PRINTF(2, 0);

G_END_DECLS

#endif /* __G_PRINTF_H__ */
