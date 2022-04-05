/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2014 Patrick Griffis
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

#ifndef __G_OSX_APP_INFO_H__
#define __G_OSX_APP_INFO_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_OSX_APP_INFO         (g_osx_app_info_get_type ())
#define G_OSX_APP_INFO(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_OSX_APP_INFO, GOsxAppInfo))
#define G_OSX_APP_INFO_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_OSX_APP_INFO, GOsxAppInfoClass))
#define X_IS_OSX_APP_INFO(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_OSX_APP_INFO))
#define X_IS_OSX_APP_INFO_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_OSX_APP_INFO))
#define G_OSX_APP_INFO_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_OSX_APP_INFO, GOsxAppInfoClass))

typedef struct _GOsxAppInfo        GOsxAppInfo;
typedef struct _GOsxAppInfoClass   GOsxAppInfoClass;

struct _GOsxAppInfoClass
{
  xobject_class_t parent_class;
};

XPL_AVAILABLE_IN_2_52
xtype_t   g_osx_app_info_get_type           (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_52
const char *g_osx_app_info_get_filename   (GOsxAppInfo *info);

XPL_AVAILABLE_IN_2_52
xlist_t * g_osx_app_info_get_all_for_scheme (const xchar_t *scheme);

G_END_DECLS


#endif /* __G_OSX_APP_INFO_H__ */
