/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2020 Руслан Ижбулатов <lrn1986@gmail.com>
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __G_WIN32_PACKAGE_PARSER_H__
#define __G_WIN32_PACKAGE_PARSER_H__

#include <gio/gio.h>

#ifdef XPLATFORM_WIN32

typedef struct _GWin32PackageExtGroup GWin32PackageExtGroup;

struct _GWin32PackageExtGroup
{
  xptr_array_t *verbs;
  xptr_array_t *extensions;
};

typedef xboolean_t (*GWin32PackageParserCallback)(xpointer_t         user_data,
                                                const xunichar2_t *full_package_name,
                                                const xunichar2_t *package_name,
                                                const xunichar2_t *app_user_model_id,
                                                xboolean_t         show_in_applist,
                                                xptr_array_t       *supported_extgroups,
                                                xptr_array_t       *supported_protocols);

xboolean_t g_win32_package_parser_enum_packages (GWin32PackageParserCallback   callback,
                                               xpointer_t                      user_data,
                                               xerror_t                      **error);

#endif /* XPLATFORM_WIN32 */

#endif /* __G_WIN32_PACKAGE_PARSER_H__ */