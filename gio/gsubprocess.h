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

#define XTYPE_SUBPROCESS         (g_subprocess_get_type ())
#define G_SUBPROCESS(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SUBPROCESS, GSubprocess))
#define X_IS_SUBPROCESS(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SUBPROCESS))

XPL_AVAILABLE_IN_2_40
xtype_t            g_subprocess_get_type                  (void) G_GNUC_CONST;

/**** Core API ****/

XPL_AVAILABLE_IN_2_40
GSubprocess *    g_subprocess_new                       (GSubprocessFlags        flags,
                                                         xerror_t                **error,
                                                         const xchar_t            *argv0,
                                                         ...) G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_2_40
GSubprocess *    g_subprocess_newv                      (const xchar_t * const  *argv,
                                                         GSubprocessFlags      flags,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
xoutput_stream_t *  g_subprocess_get_stdin_pipe            (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xinput_stream_t *   g_subprocess_get_stdout_pipe           (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xinput_stream_t *   g_subprocess_get_stderr_pipe           (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
const xchar_t *    g_subprocess_get_identifier            (GSubprocess          *subprocess);

#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_2_40
void             g_subprocess_send_signal               (GSubprocess          *subprocess,
                                                         xint_t                  signal_num);
#endif

XPL_AVAILABLE_IN_2_40
void             g_subprocess_force_exit                (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_wait                      (GSubprocess          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
void             g_subprocess_wait_async                (GSubprocess          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_wait_finish               (GSubprocess          *subprocess,
                                                         xasync_result_t         *result,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_wait_check                (GSubprocess          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
void             g_subprocess_wait_check_async          (GSubprocess          *subprocess,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_wait_check_finish         (GSubprocess          *subprocess,
                                                         xasync_result_t         *result,
                                                         xerror_t              **error);


XPL_AVAILABLE_IN_2_40
xint_t             g_subprocess_get_status                (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_get_successful            (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_get_if_exited             (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xint_t             g_subprocess_get_exit_status           (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_get_if_signaled           (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xint_t             g_subprocess_get_term_sig              (GSubprocess          *subprocess);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_communicate               (GSubprocess          *subprocess,
                                                         GBytes               *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         GBytes              **stdout_buf,
                                                         GBytes              **stderr_buf,
                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_40
void            g_subprocess_communicate_async          (GSubprocess          *subprocess,
                                                         GBytes               *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t        g_subprocess_communicate_finish         (GSubprocess          *subprocess,
                                                         xasync_result_t         *result,
                                                         GBytes              **stdout_buf,
                                                         GBytes              **stderr_buf,
                                                         xerror_t              **error);

XPL_AVAILABLE_IN_2_40
xboolean_t         g_subprocess_communicate_utf8          (GSubprocess          *subprocess,
                                                         const char           *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         char                **stdout_buf,
                                                         char                **stderr_buf,
                                                         xerror_t              **error);
XPL_AVAILABLE_IN_2_40
void            g_subprocess_communicate_utf8_async     (GSubprocess          *subprocess,
                                                         const char           *stdin_buf,
                                                         xcancellable_t         *cancellable,
                                                         xasync_ready_callback_t   callback,
                                                         xpointer_t              user_data);

XPL_AVAILABLE_IN_2_40
xboolean_t        g_subprocess_communicate_utf8_finish    (GSubprocess          *subprocess,
                                                         xasync_result_t         *result,
                                                         char                **stdout_buf,
                                                         char                **stderr_buf,
                                                         xerror_t              **error);

G_END_DECLS

#endif /* __G_SUBPROCESS_H__ */
