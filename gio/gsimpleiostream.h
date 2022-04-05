/*
 * Copyright © 2014 NICE s.r.l.
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
 * Authors: Ignacio Casal Quinteiro <ignacio.casal@nice-software.com>
 */

#ifndef __G_SIMPLE_IO_STREAM_H__
#define __G_SIMPLE_IO_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>
#include <gio/giostream.h>

G_BEGIN_DECLS

#define XTYPE_SIMPLE_IO_STREAM                  (g_simple_io_stream_get_type ())
#define G_SIMPLE_IO_STREAM(obj)                  (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_SIMPLE_IO_STREAM, GSimpleIOStream))
#define X_IS_SIMPLE_IO_STREAM(obj)               (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_SIMPLE_IO_STREAM))

XPL_AVAILABLE_IN_2_44
xtype_t                g_simple_io_stream_get_type         (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_44
xio_stream_t           *g_simple_io_stream_new              (xinput_stream_t  *input_stream,
                                                          xoutput_stream_t *output_stream);

G_END_DECLS

#endif /* __G_SIMPLE_IO_STREAM_H__ */
