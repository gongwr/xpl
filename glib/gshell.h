/* gshell.h - Shell-related utilities
 *
 *  Copyright 2000 Red Hat, Inc.
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

#ifndef __G_SHELL_H__
#define __G_SHELL_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gerror.h>

G_BEGIN_DECLS

#define G_SHELL_ERROR g_shell_error_quark ()

typedef enum
{
  /* mismatched or otherwise mangled quoting */
  G_SHELL_ERROR_BAD_QUOTING,
  /* string to be parsed was empty */
  G_SHELL_ERROR_EMPTY_STRING,
  G_SHELL_ERROR_FAILED
} GShellError;

XPL_AVAILABLE_IN_ALL
xquark g_shell_error_quark (void);

XPL_AVAILABLE_IN_ALL
xchar_t*   g_shell_quote      (const xchar_t   *unquoted_string);
XPL_AVAILABLE_IN_ALL
xchar_t*   g_shell_unquote    (const xchar_t   *quoted_string,
                             xerror_t       **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_shell_parse_argv (const xchar_t   *command_line,
                             xint_t          *argcp,
                             xchar_t       ***argvp,
                             xerror_t       **error);

G_END_DECLS

#endif /* __G_SHELL_H__ */
