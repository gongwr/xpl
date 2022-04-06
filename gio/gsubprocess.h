/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2012 Colin Walters <walters@verbum.org>
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
 * Author: Colin Walters <walters@verbum.org>
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_SUBPROCESS_H__
#define __G_SUBPROCESS_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SUBPROCESS         (xsubprocess_get_type ())
#define G_SUBPROCESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SUBPROCESS, xsubprocess))
#define X_IS_SUBPROCESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SUBPROCESS))

XPL_AVAILABLE_IN_2_40
xtype_t            xsubprocess_get_type                  (void) G_GNUC_CONST;

/**** Core API ****/

XPL_AVAILABLE_IN_2_40
xsubprocess_t *    xsubprocess_new                       (xsubprocess_flags_t        flags,
                                                         xerror_t                **error,
                                                         const xchar_t            *argv0,
                                                         ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_2_40
xsubprocess_t *    xsubprocess_newv                      (const xchar_t * const  *argv,
                                                         xsubprocess_flags_t      flags,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
xoutput_stream_t *  xsubprocess_get_stdin_pipe            (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xinput_stream_t *   xsubprocess_get_stdout_pipe           (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xinput_stream_t *   xsubprocess_get_stderr_pipe           (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
const xchar_t *    xsubprocess_get_identifier            (xsubprocess_t          *subprocess);

#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_2_40
void             xsubprocess_send_signal               (xsubprocess_t          *subprocess,
                                                         xint_t                  signal_num);
#endif

XPL_AVAILABLE_IN_2_40
void             xsubprocess_force_exit                (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_wait                      (xsubprocess_t          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
void             xsubprocess_wait_async                (xsubprocess_t          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_wait_finish               (xsubprocess_t          *subprocess,
                                                         xasync_result_t         *result,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_wait_check                (xsubprocess_t          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
void             xsubprocess_wait_check_async          (xsubprocess_t          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_wait_check_finish         (xsubprocess_t          *subprocess,
                                                         xasync_result_t         *result,
                                                         xerror_t              **error);


XPL_AVAILABLE_IN_2_40
xint_t             xsubprocess_get_status                (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_get_successful            (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_get_if_exited             (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xint_t             xsubprocess_get_exit_status           (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_get_if_signaled           (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xint_t             xsubprocess_get_term_sig              (xsubprocess_t          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_communicate               (xsubprocess_t          *subprocess,
                                                         xbytes_t               *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         xbytes_t              **stdout_buf,
                                                         xbytes_t              **stderr_buf,
                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_40
void            xsubprocess_communicate_async          (xsubprocess_t          *subprocess,
                                                         xbytes_t               *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t        xsubprocess_communicate_finish         (xsubprocess_t          *subprocess,
                                                         xasync_result_t         *result,
                                                         xbytes_t              **stdout_buf,
                                                         xbytes_t              **stderr_buf,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
xboolean_t         xsubprocess_communicate_utf8          (xsubprocess_t          *subprocess,
                                                         const char           *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         char                **stdout_buf,
                                                         char                **stderr_buf,
                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_40
void            xsubprocess_communicate_utf8_async     (xsubprocess_t          *subprocess,
                                                         const char           *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t        xsubprocess_communicate_utf8_finish    (xsubprocess_t          *subprocess,
                                                         xasync_result_t         *result,
                                                         char                **stdout_buf,
                                                         char                **stderr_buf,
                                                         xerror_t              **error);

G_END_DECLS

#endif /* __G_SUBPROCESS_H__ */
