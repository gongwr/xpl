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

#ifndef __G_LOCAL_FILE_ENUMERATOR_H__
#define __G_LOCAL_FILE_ENUMERATOR_H__

#include <gfileenumerator.h>
#include <glocalfile.h>

G_BEGIN_DECLS

#define XTYPE_LOCAL_FILE_ENUMERATOR         (_xlocal_file_enumerator_get_type ())
#define G_LOCAL_FILE_ENUMERATOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_LOCAL_FILE_ENUMERATOR, xlocal_file_enumerator))
#define G_LOCAL_FILE_ENUMERATOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_LOCAL_FILE_ENUMERATOR, xlocal_file_enumerator_class))
#define X_IS_LOCAL_FILE_ENUMERATOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_LOCAL_FILE_ENUMERATOR))
#define X_IS_LOCAL_FILE_ENUMERATOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_LOCAL_FILE_ENUMERATOR))
#define G_LOCAL_FILE_ENUMERATOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_LOCAL_FILE_ENUMERATOR, xlocal_file_enumerator_class))

typedef struct _xlocal_file_enumerator         xlocal_file_enumerator_t;
typedef struct _xlocal_file_enumerator_class    xlocal_file_enumerator_class_t;
typedef struct _GLocalFileEnumeratorPrivate  GLocalFileEnumeratorPrivate;

struct _xlocal_file_enumerator_class
{
  xfile_enumerator_class_t parent_class;
};

xtype_t             _xlocal_file_enumerator_get_type (void) G_GNUC_CONST;

xfile_enumerator_t * _xlocal_file_enumerator_new      (GLocalFile           *file,
                                                     const char           *attributes,
                                                     xfile_query_info_flags_t   flags,
                                                     xcancellable_t         *cancellable,
                                                     xerror_t              **error);

G_END_DECLS

#endif /* __XFILE_LOCAL_FILE_ENUMERATOR_H__ */
