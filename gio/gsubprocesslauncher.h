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

#define XTYPE_SUBPROCESS_LAUNCHER         (g_subprocess_launcher_get_type ())
#define G_SUBPROCESS_LAUNCHER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_SUBPROCESS_LAUNCHER, GSubprocessLauncher))
#define X_IS_SUBPROCESS_LAUNCHER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_SUBPROCESS_LAUNCHER))

XPL_AVAILABLE_IN_2_40
xtype_t                   g_subprocess_launcher_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_40
GSubprocessLauncher *   g_subprocess_launcher_new                       (GSubprocessFlags       flags);

XPL_AVAILABLE_IN_2_40
GSubprocess *           g_subprocess_launcher_spawn                     (GSubprocessLauncher   *self,
                                                                         xerror_t               **error,
                                                                         const xchar_t           *argv0,
                                                                         ...) G_GNUC_NULL_TERMINATED;

XPL_AVAILABLE_IN_2_40
GSubprocess *           g_subprocess_launcher_spawnv                    (GSubprocessLauncher   *self,
                                                                         const xchar_t * const   *argv,
                                                                         xerror_t               **error);

XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_environ               (GSubprocessLauncher   *self,
                                                                         xchar_t                **env);

XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_setenv                    (GSubprocessLauncher   *self,
                                                                         const xchar_t           *variable,
                                                                         const xchar_t           *value,
                                                                         xboolean_t               overwrite);

XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_unsetenv                  (GSubprocessLauncher *self,
                                                                         const xchar_t         *variable);

XPL_AVAILABLE_IN_2_40
const xchar_t *           g_subprocess_launcher_getenv                    (GSubprocessLauncher   *self,
                                                                         const xchar_t           *variable);

XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_cwd                   (GSubprocessLauncher   *self,
                                                                         const xchar_t           *cwd);
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_flags                 (GSubprocessLauncher   *self,
                                                                         GSubprocessFlags       flags);

/* Extended I/O control, only available on UNIX */
#ifdef G_OS_UNIX
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_stdin_file_path       (GSubprocessLauncher   *self,
                                                                         const xchar_t           *path);
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_take_stdin_fd             (GSubprocessLauncher   *self,
                                                                         xint_t                   fd);
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_stdout_file_path      (GSubprocessLauncher   *self,
                                                                         const xchar_t           *path);
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_take_stdout_fd            (GSubprocessLauncher   *self,
                                                                         xint_t                   fd);
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_stderr_file_path      (GSubprocessLauncher   *self,
                                                                         const xchar_t           *path);
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_take_stderr_fd            (GSubprocessLauncher   *self,
                                                                         xint_t                   fd);

XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_take_fd                   (GSubprocessLauncher   *self,
                                                                         xint_t                   source_fd,
                                                                         xint_t                   target_fd);

XPL_AVAILABLE_IN_2_68
void                    g_subprocess_launcher_close                     (GSubprocessLauncher      *self);

/* Child setup, only available on UNIX */
XPL_AVAILABLE_IN_2_40
void                    g_subprocess_launcher_set_child_setup           (GSubprocessLauncher   *self,
                                                                         GSpawnChildSetupFunc   child_setup,
                                                                         xpointer_t               user_data,
                                                                         GDestroyNotify         destroy_notify);
#endif

G_END_DECLS

#endif /* __G_SUBPROCESS_H__ */
