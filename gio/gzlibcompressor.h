/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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

#ifndef __G_ZLIB_COMPRESSOR_H__
#define __G_ZLIB_COMPRESSOR_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gconverter.h>
#include <gio/gfileinfo.h>

G_BEGIN_DECLS

#define XTYPE_ZLIB_COMPRESSOR         (g_zlib_compressor_get_type ())
#define G_ZLIB_COMPRESSOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_ZLIB_COMPRESSOR, xzlib_compressor))
#define G_ZLIB_COMPRESSOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_ZLIB_COMPRESSOR, GZlibCompressorClass))
#define X_IS_ZLIB_COMPRESSOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_ZLIB_COMPRESSOR))
#define X_IS_ZLIB_COMPRESSOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_ZLIB_COMPRESSOR))
#define G_ZLIB_COMPRESSOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_ZLIB_COMPRESSOR, GZlibCompressorClass))

typedef struct _GZlibCompressorClass   GZlibCompressorClass;

struct _GZlibCompressorClass
{
  xobject_class_t parent_class;
};

XPL_AVAILABLE_IN_ALL
xtype_t            g_zlib_compressor_get_type (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xzlib_compressor_t *g_zlib_compressor_new (GZlibCompressorFormat format,
					int level);

XPL_AVAILABLE_IN_ALL
xfile_info_t       *g_zlib_compressor_get_file_info (xzlib_compressor_t *compressor);
XPL_AVAILABLE_IN_ALL
void             g_zlib_compressor_set_file_info (xzlib_compressor_t *compressor,
                                                  xfile_info_t       *file_info);

G_END_DECLS

#endif /* __G_ZLIB_COMPRESSOR_H__ */
