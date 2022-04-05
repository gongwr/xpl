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

#ifndef __G_TASK_H__
#define __G_TASK_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_TASK         (g_task_get_type ())
#define G_TASK(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_TASK, GTask))
#define G_TASK_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_TASK, GTaskClass))
#define X_IS_TASK(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_TASK))
#define X_IS_TASK_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_TASK))
#define G_TASK_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_TASK, GTaskClass))

typedef struct _GTaskClass   GTaskClass;

XPL_AVAILABLE_IN_2_36
xtype_t         g_task_get_type              (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_36
GTask        *g_task_new                   (xpointer_t             source_object,
                                            xcancellable_t        *cancellable,
                                            xasync_ready_callback_t  callback,
                                            xpointer_t             callback_data);

XPL_AVAILABLE_IN_2_36
void          g_task_report_error          (xpointer_t             source_object,
                                            xasync_ready_callback_t  callback,
                                            xpointer_t             callback_data,
                                            xpointer_t             source_tag,
                                            xerror_t              *error);
XPL_AVAILABLE_IN_2_36
void          g_task_report_new_error      (xpointer_t             source_object,
                                            xasync_ready_callback_t  callback,
                                            xpointer_t             callback_data,
                                            xpointer_t             source_tag,
                                            GQuark               domain,
                                            xint_t                 code,
                                            const char          *format,
                                            ...) G_GNUC_PRINTF(7, 8);

XPL_AVAILABLE_IN_2_36
void          g_task_set_task_data         (GTask               *task,
                                            xpointer_t             task_data,
                                            GDestroyNotify       task_data_destroy);
XPL_AVAILABLE_IN_2_36
void          g_task_set_priority          (GTask               *task,
                                            xint_t                 priority);
XPL_AVAILABLE_IN_2_36
void          g_task_set_check_cancellable (GTask               *task,
                                            xboolean_t             check_cancellable);
XPL_AVAILABLE_IN_2_36
void          g_task_set_source_tag        (GTask               *task,
                                            xpointer_t             source_tag);
XPL_AVAILABLE_IN_2_60
void          g_task_set_name              (GTask               *task,
                                            const xchar_t         *name);

/* Macro wrapper to set the task name when setting the source tag. */
#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_60
#define g_task_set_source_tag(task, tag) G_STMT_START { \
  GTask *_task = (task); \
  (g_task_set_source_tag) (_task, tag); \
  if (g_task_get_name (_task) == NULL) \
    g_task_set_name (_task, G_STRINGIFY (tag)); \
} G_STMT_END
#endif

XPL_AVAILABLE_IN_2_36
xpointer_t      g_task_get_source_object     (GTask               *task);
XPL_AVAILABLE_IN_2_36
xpointer_t      g_task_get_task_data         (GTask               *task);
XPL_AVAILABLE_IN_2_36
xint_t          g_task_get_priority          (GTask               *task);
XPL_AVAILABLE_IN_2_36
GMainContext *g_task_get_context           (GTask               *task);
XPL_AVAILABLE_IN_2_36
xcancellable_t *g_task_get_cancellable       (GTask               *task);
XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_get_check_cancellable (GTask               *task);
XPL_AVAILABLE_IN_2_36
xpointer_t      g_task_get_source_tag        (GTask               *task);
XPL_AVAILABLE_IN_2_60
const xchar_t  *g_task_get_name              (GTask               *task);

XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_is_valid              (xpointer_t             result,
                                            xpointer_t             source_object);


typedef void (*GTaskThreadFunc)           (GTask           *task,
                                           xpointer_t         source_object,
                                           xpointer_t         task_data,
                                           xcancellable_t    *cancellable);
XPL_AVAILABLE_IN_2_36
void          g_task_run_in_thread        (GTask           *task,
                                           GTaskThreadFunc  task_func);
XPL_AVAILABLE_IN_2_36
void          g_task_run_in_thread_sync   (GTask           *task,
                                           GTaskThreadFunc  task_func);
XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_set_return_on_cancel (GTask           *task,
                                           xboolean_t         return_on_cancel);
XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_get_return_on_cancel (GTask           *task);

XPL_AVAILABLE_IN_2_36
void          g_task_attach_source        (GTask           *task,
                                           GSource         *source,
                                           GSourceFunc      callback);


XPL_AVAILABLE_IN_2_36
void          g_task_return_pointer            (GTask           *task,
                                                xpointer_t         result,
                                                GDestroyNotify   result_destroy);
XPL_AVAILABLE_IN_2_36
void          g_task_return_boolean            (GTask           *task,
                                                xboolean_t         result);
XPL_AVAILABLE_IN_2_36
void          g_task_return_int                (GTask           *task,
                                                gssize           result);

XPL_AVAILABLE_IN_2_36
void          g_task_return_error              (GTask           *task,
                                                xerror_t          *error);
XPL_AVAILABLE_IN_2_36
void          g_task_return_new_error          (GTask           *task,
                                                GQuark           domain,
                                                xint_t             code,
                                                const char      *format,
                                                ...) G_GNUC_PRINTF (4, 5);
XPL_AVAILABLE_IN_2_64
void          g_task_return_value              (GTask           *task,
                                                GValue          *result);

XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_return_error_if_cancelled (GTask           *task);

XPL_AVAILABLE_IN_2_36
xpointer_t      g_task_propagate_pointer         (GTask           *task,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_propagate_boolean         (GTask           *task,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_36
gssize        g_task_propagate_int             (GTask           *task,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_64
xboolean_t      g_task_propagate_value           (GTask           *task,
                                                GValue          *value,
                                                xerror_t         **error);
XPL_AVAILABLE_IN_2_36
xboolean_t      g_task_had_error                 (GTask           *task);
XPL_AVAILABLE_IN_2_44
xboolean_t      g_task_get_completed             (GTask           *task);

G_END_DECLS

#endif /* __G_TASK_H__ */
