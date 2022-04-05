/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2008 Novell, Inc.
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
 * Author: Tor Lillqvist <tml@novell.com>
 */

#ifndef __G_WINHTTP_FILE_H__
#define __G_WINHTTP_FILE_H__

#include <gio/giotypes.h>

#include "gwinhttpvfs.h"

G_BEGIN_DECLS

#define XTYPE_WINHTTP_FILE         (_g_winhttp_file_get_type ())
#define G_WINHTTP_FILE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_WINHTTP_FILE, GWinHttpFile))
#define G_WINHTTP_FILE_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_WINHTTP_FILE, GWinHttpFileClass))
#define X_IS_WINHTTP_FILE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_WINHTTP_FILE))
#define X_IS_WINHTTP_FILE_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_WINHTTP_FILE))
#define G_WINHTTP_FILE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_WINHTTP_FILE, GWinHttpFileClass))

typedef struct _GWinHttpFile        GWinHttpFile;
typedef struct _GWinHttpFileClass   GWinHttpFileClass;

struct _GWinHttpFile
{
  xobject_t parent_instance;

  GWinHttpVfs *vfs;

  URL_COMPONENTS url;
};

struct _GWinHttpFileClass
{
  xobject_class_t parent_class;
};

xtype_t _g_winhttp_file_get_type (void) G_GNUC_CONST;

xfile_t * _g_winhttp_file_new (GWinHttpVfs *vfs, const char *uri);

G_END_DECLS

#endif /* __G_WINHTTP_FILE_H__ */