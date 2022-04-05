/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2011 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XRESOURCE_FILE_H__
#define __XRESOURCE_FILE_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_RESOURCE_FILE         (_xresource_file_get_type ())
#define XRESOURCE_FILE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_RESOURCE_FILE, xresource_file_t))
#define XRESOURCE_FILE_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_RESOURCE_FILE, xresource_file_class_t))
#define X_IS_RESOURCE_FILE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_RESOURCE_FILE))
#define X_IS_RESOURCE_FILE_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_RESOURCE_FILE))
#define XRESOURCE_FILE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_RESOURCE_FILE, xresource_file_class_t))

typedef struct _xresource_file        xresource_file_t;
typedef struct _xresource_file_class   xresource_file_class_t;

struct _xresource_file_class
{
  xobject_class_t parent_class;
};

xtype_t   _xresource_file_get_type (void) G_GNUC_CONST;

xfile_t * _xresource_file_new      (const char *uri);

G_END_DECLS

#endif /* __XRESOURCE_FILE_H__ */
