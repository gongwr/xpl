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

#ifndef __G_SIMPLE_ASYNC_RESULT_H__
#define __G_SIMPLE_ASYNC_RESULT_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SIMPLE_ASYNC_RESULT         (g_simple_async_result_get_type ())
#define G_SIMPLE_ASYNC_RESULT(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SIMPLE_ASYNC_RESULT, GSimpleAsyncResult))
#define G_SIMPLE_ASYNC_RESULT_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_SIMPLE_ASYNC_RESULT, GSimpleAsyncResultClass))
#define X_IS_SIMPLE_ASYNC_RESULT(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SIMPLE_ASYNC_RESULT))
#define X_IS_SIMPLE_ASYNC_RESULT_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_SIMPLE_ASYNC_RESULT))
#define G_SIMPLE_ASYNC_RESULT_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_SIMPLE_ASYNC_RESULT, GSimpleAsyncResultClass))

/**
 * GSimpleAsyncResult:
 *
 * A simple implementation of #xasync_result_t.
 **/
typedef struct _GSimpleAsyncResultClass   GSimpleAsyncResultClass;


XPL_AVAILABLE_IN_ALL
xtype_t               g_simple_async_result_get_type         (void) G_GNUC_CONST;

XPL_DEPRECATED_IN_2_46_FOR(g_task_new)
GSimpleAsyncResult *g_simple_async_result_new              (xobject_t                 *source_object,
							    xasync_ready_callback_t      callback,
							    xpointer_t                 user_data,
							    xpointer_t                 source_tag);
XPL_DEPRECATED_IN_2_46_FOR(g_task_new)
GSimpleAsyncResult *g_simple_async_result_new_error        (xobject_t                 *source_object,
							    xasync_ready_callback_t      callback,
							    xpointer_t                 user_data,
							    GQuark                   domain,
							    xint_t                     code,
							    const char              *format,
							    ...) G_GNUC_PRINTF (6, 7);
XPL_DEPRECATED_IN_2_46_FOR(g_task_new)
GSimpleAsyncResult *g_simple_async_result_new_from_error   (xobject_t                 *source_object,
							    xasync_ready_callback_t      callback,
							    xpointer_t                 user_data,
							    const xerror_t            *error);
XPL_DEPRECATED_IN_2_46_FOR(g_task_new)
GSimpleAsyncResult *g_simple_async_result_new_take_error   (xobject_t                 *source_object,
							    xasync_ready_callback_t      callback,
							    xpointer_t                 user_data,
							    xerror_t                  *error);

XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_op_res_gpointer (GSimpleAsyncResult      *simple,
                                                               xpointer_t                 op_res,
                                                               GDestroyNotify           destroy_op_res);
XPL_DEPRECATED_IN_2_46
xpointer_t            g_simple_async_result_get_op_res_gpointer (GSimpleAsyncResult      *simple);

XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_op_res_gssize   (GSimpleAsyncResult      *simple,
                                                               gssize                   op_res);
XPL_DEPRECATED_IN_2_46
gssize              g_simple_async_result_get_op_res_gssize   (GSimpleAsyncResult      *simple);

XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_op_res_gboolean (GSimpleAsyncResult      *simple,
                                                               xboolean_t                 op_res);
XPL_DEPRECATED_IN_2_46
xboolean_t            g_simple_async_result_get_op_res_gboolean (GSimpleAsyncResult      *simple);



XPL_AVAILABLE_IN_2_32 /* Also deprecated, but can't mark something both AVAILABLE and DEPRECATED */
void                g_simple_async_result_set_check_cancellable (GSimpleAsyncResult *simple,
                                                                 xcancellable_t       *check_cancellable);
XPL_DEPRECATED_IN_2_46
xpointer_t            g_simple_async_result_get_source_tag   (GSimpleAsyncResult      *simple);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_handle_cancellation (GSimpleAsyncResult      *simple,
								   xboolean_t          handle_cancellation);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_complete         (GSimpleAsyncResult      *simple);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_complete_in_idle (GSimpleAsyncResult      *simple);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_run_in_thread    (GSimpleAsyncResult      *simple,
							    GSimpleAsyncThreadFunc   func,
							    int                      io_priority,
							    xcancellable_t            *cancellable);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_from_error   (GSimpleAsyncResult      *simple,
							    const xerror_t            *error);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_take_error       (GSimpleAsyncResult      *simple,
							    xerror_t            *error);
XPL_DEPRECATED_IN_2_46
xboolean_t            g_simple_async_result_propagate_error  (GSimpleAsyncResult      *simple,
							    xerror_t                 **dest);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_error        (GSimpleAsyncResult      *simple,
							    GQuark                   domain,
							    xint_t                     code,
							    const char              *format,
							    ...) G_GNUC_PRINTF (4, 5);
XPL_DEPRECATED_IN_2_46
void                g_simple_async_result_set_error_va     (GSimpleAsyncResult      *simple,
							    GQuark                   domain,
							    xint_t                     code,
							    const char              *format,
							    va_list                  args)
							    G_GNUC_PRINTF(4, 0);
XPL_DEPRECATED_IN_2_46
xboolean_t            g_simple_async_result_is_valid         (xasync_result_t            *result,
                                                            xobject_t                 *source,
                                                            xpointer_t                 source_tag);

XPL_DEPRECATED_IN_2_46_FOR(g_task_report_error)
void g_simple_async_report_error_in_idle  (xobject_t            *object,
					   xasync_ready_callback_t callback,
					   xpointer_t            user_data,
					   GQuark              domain,
					   xint_t                code,
					   const char         *format,
					   ...) G_GNUC_PRINTF(6, 7);
XPL_DEPRECATED_IN_2_46_FOR(g_task_report_error)
void g_simple_async_report_gerror_in_idle (xobject_t            *object,
					   xasync_ready_callback_t callback,
					   xpointer_t            user_data,
					   const xerror_t       *error);
XPL_DEPRECATED_IN_2_46_FOR(g_task_report_error)
void g_simple_async_report_take_gerror_in_idle (xobject_t            *object,
                                                xasync_ready_callback_t callback,
                                                xpointer_t            user_data,
                                                xerror_t             *error);

G_END_DECLS



#endif /* __G_SIMPLE_ASYNC_RESULT_H__ */
