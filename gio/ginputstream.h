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

#ifndef __G_INPUT_STREAM_H__
#define __G_INPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_INPUT_STREAM         (g_input_stream_get_type ())
#define G_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_INPUT_STREAM, xinput_stream_t))
#define G_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_INPUT_STREAM, GInputStreamClass))
#define X_IS_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_INPUT_STREAM))
#define X_IS_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_INPUT_STREAM))
#define G_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_INPUT_STREAM, GInputStreamClass))

/**
 * xinput_stream_t:
 *
 * Base class for streaming input operations.
 **/
typedef struct _GInputStreamClass    GInputStreamClass;
typedef struct _GInputStreamPrivate  GInputStreamPrivate;

struct _GInputStream
{
  xobject_t parent_instance;

  /*< private >*/
  GInputStreamPrivate *priv;
};

struct _GInputStreamClass
{
  xobject_class_t parent_class;

  /* Sync ops: */

  gssize   (* read_fn)      (xinput_stream_t        *stream,
                             void                *buffer,
                             xsize_t                count,
                             xcancellable_t        *cancellable,
                             xerror_t             **error);
  gssize   (* skip)         (xinput_stream_t        *stream,
                             xsize_t                count,
                             xcancellable_t        *cancellable,
                             xerror_t             **error);
  xboolean_t (* close_fn)	    (xinput_stream_t        *stream,
                             xcancellable_t        *cancellable,
                             xerror_t             **error);

  /* Async ops: (optional in derived classes) */
  void     (* read_async)   (xinput_stream_t        *stream,
                             void                *buffer,
                             xsize_t                count,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data);
  gssize   (* read_finish)  (xinput_stream_t        *stream,
                             xasync_result_t        *result,
                             xerror_t             **error);
  void     (* skip_async)   (xinput_stream_t        *stream,
                             xsize_t                count,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data);
  gssize   (* skip_finish)  (xinput_stream_t        *stream,
                             xasync_result_t        *result,
                             xerror_t             **error);
  void     (* close_async)  (xinput_stream_t        *stream,
                             int                  io_priority,
                             xcancellable_t        *cancellable,
                             xasync_ready_callback_t  callback,
                             xpointer_t             user_data);
  xboolean_t (* close_finish) (xinput_stream_t        *stream,
                             xasync_result_t        *result,
                             xerror_t             **error);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t    g_input_stream_get_type      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
gssize   g_input_stream_read          (xinput_stream_t          *stream,
				       void                  *buffer,
				       xsize_t                  count,
				       xcancellable_t          *cancellable,
				       xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_input_stream_read_all      (xinput_stream_t          *stream,
				       void                  *buffer,
				       xsize_t                  count,
				       xsize_t                 *bytes_read,
				       xcancellable_t          *cancellable,
				       xerror_t               **error);
XPL_AVAILABLE_IN_2_34
GBytes  *g_input_stream_read_bytes    (xinput_stream_t          *stream,
				       xsize_t                  count,
				       xcancellable_t          *cancellable,
				       xerror_t               **error);
XPL_AVAILABLE_IN_ALL
gssize   g_input_stream_skip          (xinput_stream_t          *stream,
				       xsize_t                  count,
				       xcancellable_t          *cancellable,
				       xerror_t               **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_input_stream_close         (xinput_stream_t          *stream,
				       xcancellable_t          *cancellable,
				       xerror_t               **error);
XPL_AVAILABLE_IN_ALL
void     g_input_stream_read_async    (xinput_stream_t          *stream,
				       void                  *buffer,
				       xsize_t                  count,
				       int                    io_priority,
				       xcancellable_t          *cancellable,
				       xasync_ready_callback_t    callback,
				       xpointer_t               user_data);
XPL_AVAILABLE_IN_ALL
gssize   g_input_stream_read_finish   (xinput_stream_t          *stream,
				       xasync_result_t          *result,
				       xerror_t               **error);

XPL_AVAILABLE_IN_2_44
void     g_input_stream_read_all_async    (xinput_stream_t          *stream,
                                           void                  *buffer,
                                           xsize_t                  count,
                                           int                    io_priority,
                                           xcancellable_t          *cancellable,
                                           xasync_ready_callback_t    callback,
                                           xpointer_t               user_data);
XPL_AVAILABLE_IN_2_44
xboolean_t g_input_stream_read_all_finish   (xinput_stream_t          *stream,
                                           xasync_result_t          *result,
                                           xsize_t                 *bytes_read,
                                           xerror_t               **error);

XPL_AVAILABLE_IN_2_34
void     g_input_stream_read_bytes_async  (xinput_stream_t          *stream,
					   xsize_t                  count,
					   int                    io_priority,
					   xcancellable_t          *cancellable,
					   xasync_ready_callback_t    callback,
					   xpointer_t               user_data);
XPL_AVAILABLE_IN_2_34
GBytes  *g_input_stream_read_bytes_finish (xinput_stream_t          *stream,
					   xasync_result_t          *result,
					   xerror_t               **error);
XPL_AVAILABLE_IN_ALL
void     g_input_stream_skip_async    (xinput_stream_t          *stream,
				       xsize_t                  count,
				       int                    io_priority,
				       xcancellable_t          *cancellable,
				       xasync_ready_callback_t    callback,
				       xpointer_t               user_data);
XPL_AVAILABLE_IN_ALL
gssize   g_input_stream_skip_finish   (xinput_stream_t          *stream,
				       xasync_result_t          *result,
				       xerror_t               **error);
XPL_AVAILABLE_IN_ALL
void     g_input_stream_close_async   (xinput_stream_t          *stream,
				       int                    io_priority,
				       xcancellable_t          *cancellable,
				       xasync_ready_callback_t    callback,
				       xpointer_t               user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t g_input_stream_close_finish  (xinput_stream_t          *stream,
				       xasync_result_t          *result,
				       xerror_t               **error);

/* For implementations: */

XPL_AVAILABLE_IN_ALL
xboolean_t g_input_stream_is_closed     (xinput_stream_t          *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t g_input_stream_has_pending   (xinput_stream_t          *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t g_input_stream_set_pending   (xinput_stream_t          *stream,
				       xerror_t               **error);
XPL_AVAILABLE_IN_ALL
void     g_input_stream_clear_pending (xinput_stream_t          *stream);

G_END_DECLS

#endif /* __G_INPUT_STREAM_H__ */
