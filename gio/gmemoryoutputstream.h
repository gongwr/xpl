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
 * Author: Christian Kellner <gicmo@gnome.org>
 */

#ifndef __G_MEMORY_OUTPUT_STREAM_H__
#define __G_MEMORY_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/goutputstream.h>

G_BEGIN_DECLS

#define XTYPE_MEMORY_OUTPUT_STREAM         (g_memory_output_stream_get_type ())
#define G_MEMORY_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_MEMORY_OUTPUT_STREAM, xmemory_output_stream))
#define G_MEMORY_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_MEMORY_OUTPUT_STREAM, GMemoryOutputStreamClass))
#define X_IS_MEMORY_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_MEMORY_OUTPUT_STREAM))
#define X_IS_MEMORY_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_MEMORY_OUTPUT_STREAM))
#define G_MEMORY_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_MEMORY_OUTPUT_STREAM, GMemoryOutputStreamClass))

/**
 * xmemory_output_stream_t:
 *
 * Implements #xoutput_stream_t for arbitrary memory chunks.
 **/
typedef struct _GMemoryOutputStreamClass    GMemoryOutputStreamClass;
typedef struct _GMemoryOutputStreamPrivate  GMemoryOutputStreamPrivate;

struct _GMemoryOutputStream
{
  xoutput_stream_t parent_instance;

  /*< private >*/
  GMemoryOutputStreamPrivate *priv;
};

struct _GMemoryOutputStreamClass
{
  xoutput_stream_class_t parent_class;

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

/**
 * GReallocFunc:
 * @data: memory block to reallocate
 * @size: size to reallocate @data to
 *
 * Changes the size of the memory block pointed to by @data to
 * @size bytes.
 *
 * The function should have the same semantics as realloc().
 *
 * Returns: a pointer to the reallocated memory
 */
typedef xpointer_t (* GReallocFunc) (xpointer_t data,
                                   xsize_t    size);

XPL_AVAILABLE_IN_ALL
xtype_t          g_memory_output_stream_get_type      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xoutput_stream_t *g_memory_output_stream_new           (xpointer_t             data,
                                                     xsize_t                size,
                                                     GReallocFunc         realloc_function,
                                                     xdestroy_notify_t       destroy_function);
XPL_AVAILABLE_IN_2_36
xoutput_stream_t *g_memory_output_stream_new_resizable (void);
XPL_AVAILABLE_IN_ALL
xpointer_t       g_memory_output_stream_get_data      (xmemory_output_stream_t *ostream);
XPL_AVAILABLE_IN_ALL
xsize_t          g_memory_output_stream_get_size      (xmemory_output_stream_t *ostream);
XPL_AVAILABLE_IN_ALL
xsize_t          g_memory_output_stream_get_data_size (xmemory_output_stream_t *ostream);
XPL_AVAILABLE_IN_ALL
xpointer_t       g_memory_output_stream_steal_data    (xmemory_output_stream_t *ostream);

XPL_AVAILABLE_IN_2_34
xbytes_t *       g_memory_output_stream_steal_as_bytes (xmemory_output_stream_t *ostream);

G_END_DECLS

#endif /* __G_MEMORY_OUTPUT_STREAM_H__ */
