/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008, 2009 Codethink Limited
 * Copyright © 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_IO_STREAM_H__
#define __G_IO_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/ginputstream.h>
#include <gio/goutputstream.h>
#include <gio/gcancellable.h>
#include <gio/gioerror.h>

G_BEGIN_DECLS

#define XTYPE_IO_STREAM         (xio_stream_get_type ())
#define XIO_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_IO_STREAM, xio_stream))
#define XIO_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_IO_STREAM, xio_stream_class))
#define X_IS_IO_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_IO_STREAM))
#define X_IS_IO_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_IO_STREAM))
#define XIO_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_IO_STREAM, xio_stream_class))

typedef struct _xio_stream_private                            xio_stream_private_t;
typedef struct _xio_stream_class                              xio_stream_class_t;

/**
 * xio_stream_t:
 *
 * Base class for read-write streams.
 **/
struct _xio_stream
{
  xobject_t parent_instance;

  /*< private >*/
  xio_stream_private_t *priv;
};

struct _xio_stream_class
{
  xobject_class_t parent_class;

  xinput_stream_t *  (*get_input_stream)  (xio_stream_t *stream);
  xoutput_stream_t * (*get_output_stream) (xio_stream_t *stream);

  xboolean_t (* close_fn)	    (xio_stream_t           *stream,
                             xcancellable_t        *cancellable,
                             xerror_t             **error);
  void     (* close_async)  (xio_stream_t           *stream,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data);
  xboolean_t (* close_finish) (xio_stream_t           *stream,
                             xasync_result_t        *result,
                             xerror_t             **error);
  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
  void (*_g_reserved7) (void);
  void (*_g_reserved8) (void);
  void (*_g_reserved9) (void);
  void (*_g_reserved10) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t          xio_stream_get_type          (void)  G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xinput_stream_t * g_io_stream_get_input_stream  (xio_stream_t            *stream);
XPL_AVAILABLE_IN_ALL
xoutput_stream_t *g_io_stream_get_output_stream (xio_stream_t            *stream);

XPL_AVAILABLE_IN_ALL
void           g_io_stream_splice_async      (xio_stream_t            *stream1,
					      xio_stream_t            *stream2,
					      xio_stream_splice_flags_t  flags,
					      int                   io_priority,
					      xcancellable_t         *cancellable,
					      xasync_ready_callback_t   callback,
					      xpointer_t              user_data);

XPL_AVAILABLE_IN_ALL
xboolean_t       g_io_stream_splice_finish     (xasync_result_t         *result,
                                              xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t       g_io_stream_close             (xio_stream_t            *stream,
					      xcancellable_t         *cancellable,
					      xerror_t              **error);

XPL_AVAILABLE_IN_ALL
void           g_io_stream_close_async       (xio_stream_t            *stream,
					      int                   io_priority,
					      xcancellable_t         *cancellable,
					      xasync_ready_callback_t   callback,
					      xpointer_t              user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_io_stream_close_finish      (xio_stream_t            *stream,
					      xasync_result_t         *result,
					      xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t       g_io_stream_is_closed         (xio_stream_t            *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_io_stream_has_pending       (xio_stream_t            *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t       g_io_stream_set_pending       (xio_stream_t            *stream,
					      xerror_t              **error);
XPL_AVAILABLE_IN_ALL
void           g_io_stream_clear_pending     (xio_stream_t            *stream);

G_END_DECLS

#endif /* __G_IO_STREAM_H__ */
