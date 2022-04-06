/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2012,2013 Colin Walters <walters@verbum.org>
 * Copyright © 2012,2013 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 * Author: Colin Walters <walters@verbum.org>
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_SUBPROCESS_LAUNCHER_H__
#define __G_SUBPROCESS_LAUNCHER_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SUBPROCESS_LAUNCHER         (xsubprocess_launcher_get_type ())
#define G_SUBPROCESS_LAUNCHER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SUBPROCESS_LAUNCHER, xsubprocess_launcher))
#define X_IS_SUBPROCESS_LAUNCHER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SUBPROCESS_LAUNCHER))

XPL_AVAILABLE_IN_2_40
xtype_t                   xsubprocess_launcher_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_40
xsubprocess_launcher_t *   xsubprocess_launcher_new                       (xsubprocess_flags_t       flags);

XPL_AVAILABLE_IN_2_40
xsubprocess_t *           xsubprocess_launcher_spawn                     (xsubprocess_launcher_t   *self,
                                                                         xerror_t               **error,
                                                                         const xchar_t           *argv0,
                                                                         ...) G_GNUC_NULL_TERMINATED;

XPL_AVAILABLE_IN_2_40
xsubprocess_t *           xsubprocess_launcher_spawnv                    (xsubprocess_launcher_t   *self,
                                                                         const xchar_t * const   *argv,
                                                                         xerror_t               **error);

XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_environ               (xsubprocess_launcher_t   *self,
                                                                         xchar_t                **env);

XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_setenv                    (xsubprocess_launcher_t   *self,
                                                                         const xchar_t           *variable,
                                                                         const xchar_t           *value,
                                                                         xboolean_t               overwrite);

XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_unsetenv                  (xsubprocess_launcher_t *self,
                                                                         const xchar_t         *variable);

XPL_AVAILABLE_IN_2_40
const xchar_t *           xsubprocess_launcher_getenv                    (xsubprocess_launcher_t   *self,
                                                                         const xchar_t           *variable);

XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_cwd                   (xsubprocess_launcher_t   *self,
                                                                         const xchar_t           *cwd);
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_flags                 (xsubprocess_launcher_t   *self,
                                                                         xsubprocess_flags_t       flags);

/* Extended I/O control, only available on UNIX */
#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_stdin_file_path       (xsubprocess_launcher_t   *self,
                                                                         const xchar_t           *path);
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_take_stdin_fd             (xsubprocess_launcher_t   *self,
                                                                         xint_t                   fd);
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_stdout_file_path      (xsubprocess_launcher_t   *self,
                                                                         const xchar_t           *path);
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_take_stdout_fd            (xsubprocess_launcher_t   *self,
                                                                         xint_t                   fd);
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_stderr_file_path      (xsubprocess_launcher_t   *self,
                                                                         const xchar_t           *path);
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_take_stderr_fd            (xsubprocess_launcher_t   *self,
                                                                         xint_t                   fd);

XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_take_fd                   (xsubprocess_launcher_t   *self,
                                                                         xint_t                   source_fd,
                                                                         xint_t                   target_fd);

XPL_AVAILABLE_IN_2_68
void                    xsubprocess_launcher_close                     (xsubprocess_launcher_t      *self);

/* Child setup, only available on UNIX */
XPL_AVAILABLE_IN_2_40
void                    xsubprocess_launcher_set_child_setup           (xsubprocess_launcher_t   *self,
                                                                         GSpawnChildSetupFunc   child_setup,
                                                                         xpointer_t               user_data,
                                                                         xdestroy_notify_t         destroy_notify);
#endif

G_END_DECLS

#endif /* __G_SUBPROCESS_H__ */
