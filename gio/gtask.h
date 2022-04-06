/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
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
 */

#ifndef __XTASK_H__
#define __XTASK_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_TASK         (xtask_get_type ())
#define XTASK(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_TASK, xtask_t))
#define XTASK_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_TASK, xtask_class_t))
#define X_IS_TASK(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_TASK))
#define X_IS_TASK_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_TASK))
#define XTASK_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_TASK, xtask_class_t))

typedef struct _xtask_class   xtask_class_t;

XPL_AVAILABLE_IN_2_36
xtype_t         xtask_get_type              (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_36
xtask_t        *xtask_new                   (xpointer_t             source_object,
                                            xcancellable_t        *cancellable,
                                            xasync_ready_callback_t  callback,
                                            xpointer_t             callback_data);

XPL_AVAILABLE_IN_2_36
void          xtask_report_error          (xpointer_t             source_object,
                                            xasync_ready_callback_t  callback,
                                            xpointer_t             callback_data,
                                            xpointer_t             source_tag,
                                            xerror_t              *error);
XPL_AVAILABLE_IN_2_36
void          xtask_report_new_error      (xpointer_t             source_object,
                                            xasync_ready_callback_t  callback,
                                            xpointer_t             callback_data,
                                            xpointer_t             source_tag,
                                            xquark               domain,
                                            xint_t                 code,
                                            const char          *format,
                                            ...) G_GNUC_PRINTF(7, 8);

XPL_AVAILABLE_IN_2_36
void          xtask_set_task_data         (xtask_t               *task,
                                            xpointer_t             task_data,
                                            xdestroy_notify_t       task_data_destroy);
XPL_AVAILABLE_IN_2_36
void          xtask_set_priority          (xtask_t               *task,
                                            xint_t                 priority);
XPL_AVAILABLE_IN_2_36
void          xtask_set_check_cancellable (xtask_t               *task,
                                            xboolean_t             check_cancellable);
XPL_AVAILABLE_IN_2_36
void          xtask_set_source_tag        (xtask_t               *task,
                                            xpointer_t             source_tag);
XPL_AVAILABLE_IN_2_60
void          xtask_set_name              (xtask_t               *task,
                                            const xchar_t         *name);

/* Macro wrapper to set the task name when setting the source tag. */
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_60
#define xtask_set_source_tag(task, tag) G_STMT_START { \
  xtask_t *_task = (task); \
  (xtask_set_source_tag) (_task, tag); \
  if (xtask_get_name (_task) == NULL) \
    xtask_set_name (_task, G_STRINGIFY (tag)); \
} G_STMT_END
#endif

XPL_AVAILABLE_IN_2_36
xpointer_t      xtask_get_source_object     (xtask_t               *task);
XPL_AVAILABLE_IN_2_36
xpointer_t      xtask_get_task_data         (xtask_t               *task);
XPL_AVAILABLE_IN_2_36
xint_t          xtask_get_priority          (xtask_t               *task);
XPL_AVAILABLE_IN_2_36
xmain_context_t *xtask_get_context           (xtask_t               *task);
XPL_AVAILABLE_IN_2_36
xcancellable_t *xtask_get_cancellable       (xtask_t               *task);
XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_get_check_cancellable (xtask_t               *task);
XPL_AVAILABLE_IN_2_36
xpointer_t      xtask_get_source_tag        (xtask_t               *task);
XPL_AVAILABLE_IN_2_60
const xchar_t  *xtask_get_name              (xtask_t               *task);

XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_is_valid              (xpointer_t             result,
                                            xpointer_t             source_object);


typedef void (*xtask_thread_func_t)           (xtask_t           *task,
                                           xpointer_t         source_object,
                                           xpointer_t         task_data,
                                           xcancellable_t    *cancellable);
XPL_AVAILABLE_IN_2_36
void          xtask_run_in_thread        (xtask_t           *task,
                                           xtask_thread_func_t  task_func);
XPL_AVAILABLE_IN_2_36
void          xtask_run_in_thread_sync   (xtask_t           *task,
                                           xtask_thread_func_t  task_func);
XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_set_return_on_cancel (xtask_t           *task,
                                           xboolean_t         return_on_cancel);
XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_get_return_on_cancel (xtask_t           *task);

XPL_AVAILABLE_IN_2_36
void          xtask_attach_source        (xtask_t           *task,
                                           xsource_t         *source,
                                           xsource_func_t      callback);


XPL_AVAILABLE_IN_2_36
void          xtask_return_pointer            (xtask_t           *task,
                                                xpointer_t         result,
                                                xdestroy_notify_t   result_destroy);
XPL_AVAILABLE_IN_2_36
void          xtask_return_boolean            (xtask_t           *task,
                                                xboolean_t         result);
XPL_AVAILABLE_IN_2_36
void          xtask_return_int                (xtask_t           *task,
                                                xssize_t           result);

XPL_AVAILABLE_IN_2_36
void          xtask_return_error              (xtask_t           *task,
                                                xerror_t          *error);
XPL_AVAILABLE_IN_2_36
void          xtask_return_new_error          (xtask_t           *task,
                                                xquark           domain,
                                                xint_t             code,
                                                const char      *format,
                                                ...) G_GNUC_PRINTF (4, 5);
XPL_AVAILABLE_IN_2_64
void          xtask_return_value              (xtask_t           *task,
                                                xvalue_t          *result);

XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_return_error_if_cancelled (xtask_t           *task);

XPL_AVAILABLE_IN_2_36
xpointer_t      xtask_propagate_pointer         (xtask_t           *task,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_propagate_boolean         (xtask_t           *task,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_36
xssize_t        xtask_propagate_int             (xtask_t           *task,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_64
xboolean_t      xtask_propagate_value           (xtask_t           *task,
                                                xvalue_t          *value,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_36
xboolean_t      xtask_had_error                 (xtask_t           *task);
XPL_AVAILABLE_IN_2_44
xboolean_t      xtask_get_completed             (xtask_t           *task);

G_END_DECLS

#endif /* __XTASK_H__ */
