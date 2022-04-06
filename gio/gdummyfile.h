/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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

#ifndef __G_DUMMY_FILE_H__
#define __G_DUMMY_FILE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_DUMMY_FILE         (_xdummy_file_get_type ())
#define G_DUMMY_FILE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DUMMY_FILE, xdummy_file_t))
#define G_DUMMY_FILE_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DUMMY_FILE, xdummy_file_class_t))
#define X_IS_DUMMY_FILE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DUMMY_FILE))
#define X_IS_DUMMY_FILE_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DUMMY_FILE))
#define G_DUMMY_FILE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DUMMY_FILE, xdummy_file_class_t))

typedef struct _GDummyFile        xdummy_file_t;
typedef struct _xdummy_file_class   xdummy_file_class_t;

struct _xdummy_file_class
{
  xobject_class_t parent_class;
};

xtype_t   _xdummy_file_get_type (void) G_GNUC_CONST;

xfile_t * _xdummy_file_new      (const char *uri);

G_END_DECLS

#endif /* __G_DUMMY_FILE_H__ */
