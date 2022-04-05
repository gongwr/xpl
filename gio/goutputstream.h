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

#ifndef __G_OUTPUT_STREAM_H__
#define __G_OUTPUT_STREAM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_OUTPUT_STREAM         (g_output_stream_get_type ())
#define G_OUTPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_OUTPUT_STREAM, xoutput_stream_t))
#define G_OUTPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_OUTPUT_STREAM, GOutputStreamClass))
#define X_IS_OUTPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_OUTPUT_STREAM))
#define X_IS_OUTPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_OUTPUT_STREAM))
#define G_OUTPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_OUTPUT_STREAM, GOutputStreamClass))

/**
 * xoutput_stream_t:
 *
 * Base class for writing output.
 *
 * All classes derived from xoutput_stream_t should implement synchronous
 * writing, splicing, flushing and closing streams, but may implement
 * asynchronous versions.
 **/
typedef struct _GOutputStreamClass    GOutputStreamClass;
typedef struct _GOutputStreamPrivate  GOutputStreamPrivate;

struct _GOutputStream
{
  xobject_t parent_instance;

  /*< private >*/
  GOutputStreamPrivate *priv;
};


struct _GOutputStreamClass
{
  xobject_class_t parent_class;

  /* Sync ops: */

  gssize      (* write_fn)      (xoutput_stream_t            *stream,
                                 const void               *buffer,
                                 xsize_t                     count,
                                 xcancellable_t             *cancellable,
                                 xerror_t                  **error);
  gssize      (* splice)        (xoutput_stream_t            *stream,
                                 xinput_stream_t             *source,
                                 GOutputStreamSpliceFlags  flags,
                                 xcancellable_t             *cancellable,
                                 xerror_t                  **error);
  xboolean_t    (* flush)	        (xoutput_stream_t            *stream,
                                 xcancellable_t             *cancellable,
                                 xerror_t                  **error);
  xboolean_t    (* close_fn)      (xoutput_stream_t            *stream,
                                 xcancellable_t             *cancellable,
                                 xerror_t                  **error);

  /* Async ops: (optional in derived classes) */

  void        (* write_async)   (xoutput_stream_t            *stream,
                                 const void               *buffer,
                                 xsize_t                     count,
                                 int                       io_priority,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data);
  gssize      (* write_finish)  (xoutput_stream_t            *stream,
                                 xasync_result_t             *result,
                                 xerror_t                  **error);
  void        (* splice_async)  (xoutput_stream_t            *stream,
                                 xinput_stream_t             *source,
                                 GOutputStreamSpliceFlags  flags,
                                 int                       io_priority,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data);
  gssize      (* splice_finish) (xoutput_stream_t            *stream,
                                 xasync_result_t             *result,
                                 xerror_t                  **error);
  void        (* flush_async)   (xoutput_stream_t            *stream,
                                 int                       io_priority,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data);
  xboolean_t    (* flush_finish)  (xoutput_stream_t            *stream,
                                 xasync_result_t             *result,
                                 xerror_t                  **error);
  void        (* close_async)   (xoutput_stream_t            *stream,
                                 int                       io_priority,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data);
  xboolean_t    (* close_finish)  (xoutput_stream_t            *stream,
                                 xasync_result_t             *result,
                                 xerror_t                  **error);

  xboolean_t    (* writev_fn)     (xoutput_stream_t            *stream,
                                 const GOutputVector      *vectors,
                                 xsize_t                     n_vectors,
                                 xsize_t                    *bytes_written,
                                 xcancellable_t             *cancellable,
                                 xerror_t                  **error);

  void        (* writev_async)  (xoutput_stream_t            *stream,
                                 const GOutputVector      *vectors,
                                 xsize_t                     n_vectors,
                                 int                       io_priority,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data);

  xboolean_t    (* writev_finish) (xoutput_stream_t            *stream,
                                 xasync_result_t             *result,
                                 xsize_t                    *bytes_written,
                                 xerror_t                  **error);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
  void (*_g_reserved7) (void);
  void (*_g_reserved8) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t    g_output_stream_get_type      (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
gssize   g_output_stream_write         (xoutput_stream_t             *stream,
					const void                *buffer,
					xsize_t                      count,
					xcancellable_t              *cancellable,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_write_all     (xoutput_stream_t             *stream,
					const void                *buffer,
					xsize_t                      count,
					xsize_t                     *bytes_written,
					xcancellable_t              *cancellable,
					xerror_t                   **error);

XPL_AVAILABLE_IN_2_60
xboolean_t g_output_stream_writev        (xoutput_stream_t             *stream,
					const GOutputVector       *vectors,
					xsize_t                      n_vectors,
					xsize_t                     *bytes_written,
					xcancellable_t              *cancellable,
					xerror_t                   **error);
XPL_AVAILABLE_IN_2_60
xboolean_t g_output_stream_writev_all    (xoutput_stream_t             *stream,
					GOutputVector             *vectors,
					xsize_t                      n_vectors,
					xsize_t                     *bytes_written,
					xcancellable_t              *cancellable,
					xerror_t                   **error);

XPL_AVAILABLE_IN_2_40
xboolean_t g_output_stream_printf        (xoutput_stream_t             *stream,
                                        xsize_t                     *bytes_written,
                                        xcancellable_t              *cancellable,
                                        xerror_t                   **error,
                                        const xchar_t               *format,
                                        ...) G_GNUC_PRINTF (5, 6);
XPL_AVAILABLE_IN_2_40
xboolean_t g_output_stream_vprintf       (xoutput_stream_t             *stream,
                                        xsize_t                     *bytes_written,
                                        xcancellable_t              *cancellable,
                                        xerror_t                   **error,
                                        const xchar_t               *format,
                                        va_list                    args) G_GNUC_PRINTF (5, 0);
XPL_AVAILABLE_IN_2_34
gssize   g_output_stream_write_bytes   (xoutput_stream_t             *stream,
					GBytes                    *bytes,
					xcancellable_t              *cancellable,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
gssize   g_output_stream_splice        (xoutput_stream_t             *stream,
					xinput_stream_t              *source,
					GOutputStreamSpliceFlags   flags,
					xcancellable_t              *cancellable,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_flush         (xoutput_stream_t             *stream,
					xcancellable_t              *cancellable,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_close         (xoutput_stream_t             *stream,
					xcancellable_t              *cancellable,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void     g_output_stream_write_async   (xoutput_stream_t             *stream,
					const void                *buffer,
					xsize_t                      count,
					int                        io_priority,
					xcancellable_t              *cancellable,
					xasync_ready_callback_t        callback,
					xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
gssize   g_output_stream_write_finish  (xoutput_stream_t             *stream,
					xasync_result_t              *result,
					xerror_t                   **error);

XPL_AVAILABLE_IN_2_44
void     g_output_stream_write_all_async (xoutput_stream_t           *stream,
                                          const void              *buffer,
                                          xsize_t                    count,
                                          int                      io_priority,
                                          xcancellable_t            *cancellable,
                                          xasync_ready_callback_t      callback,
                                          xpointer_t                 user_data);

XPL_AVAILABLE_IN_2_44
xboolean_t g_output_stream_write_all_finish (xoutput_stream_t          *stream,
                                           xasync_result_t           *result,
                                           xsize_t                  *bytes_written,
                                           xerror_t                **error);

XPL_AVAILABLE_IN_2_60
void     g_output_stream_writev_async  (xoutput_stream_t             *stream,
					const GOutputVector       *vectors,
					xsize_t                      n_vectors,
					int                        io_priority,
					xcancellable_t              *cancellable,
					xasync_ready_callback_t        callback,
					xpointer_t                   user_data);
XPL_AVAILABLE_IN_2_60
xboolean_t g_output_stream_writev_finish (xoutput_stream_t             *stream,
					xasync_result_t              *result,
					xsize_t                     *bytes_written,
					xerror_t                   **error);

XPL_AVAILABLE_IN_2_60
void     g_output_stream_writev_all_async (xoutput_stream_t           *stream,
                                           GOutputVector           *vectors,
                                           xsize_t                    n_vectors,
                                           int                      io_priority,
                                           xcancellable_t            *cancellable,
                                           xasync_ready_callback_t      callback,
                                           xpointer_t                 user_data);

XPL_AVAILABLE_IN_2_60
xboolean_t g_output_stream_writev_all_finish (xoutput_stream_t          *stream,
                                            xasync_result_t           *result,
                                            xsize_t                  *bytes_written,
                                            xerror_t                **error);

XPL_AVAILABLE_IN_2_34
void     g_output_stream_write_bytes_async  (xoutput_stream_t             *stream,
					     GBytes                    *bytes,
					     int                        io_priority,
					     xcancellable_t              *cancellable,
					     xasync_ready_callback_t        callback,
					     xpointer_t                   user_data);
XPL_AVAILABLE_IN_2_34
gssize   g_output_stream_write_bytes_finish (xoutput_stream_t             *stream,
					     xasync_result_t              *result,
					     xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void     g_output_stream_splice_async  (xoutput_stream_t             *stream,
					xinput_stream_t              *source,
					GOutputStreamSpliceFlags   flags,
					int                        io_priority,
					xcancellable_t              *cancellable,
					xasync_ready_callback_t        callback,
					xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
gssize   g_output_stream_splice_finish (xoutput_stream_t             *stream,
					xasync_result_t              *result,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void     g_output_stream_flush_async   (xoutput_stream_t             *stream,
					int                        io_priority,
					xcancellable_t              *cancellable,
					xasync_ready_callback_t        callback,
					xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_flush_finish  (xoutput_stream_t             *stream,
					xasync_result_t              *result,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void     g_output_stream_close_async   (xoutput_stream_t             *stream,
					int                        io_priority,
					xcancellable_t              *cancellable,
					xasync_ready_callback_t        callback,
					xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_close_finish  (xoutput_stream_t             *stream,
					xasync_result_t              *result,
					xerror_t                   **error);

XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_is_closed     (xoutput_stream_t             *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_is_closing    (xoutput_stream_t             *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_has_pending   (xoutput_stream_t             *stream);
XPL_AVAILABLE_IN_ALL
xboolean_t g_output_stream_set_pending   (xoutput_stream_t             *stream,
					xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void     g_output_stream_clear_pending (xoutput_stream_t             *stream);


G_END_DECLS

#endif /* __G_OUTPUT_STREAM_H__ */
