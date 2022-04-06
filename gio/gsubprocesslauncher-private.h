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
 */

#ifndef __G_SUBPROCESS_CONTEXT_PRIVATE_H__
#define __G_SUBPROCESS_CONTEXT_PRIVATE_H__

#include "gsubprocesslauncher.h"

G_BEGIN_DECLS

struct _xsubprocess_launcher
{
  xobject_t parent;

  xsubprocess_flags_t flags;
  char **envp;
  char *cwd;

#ifdef G_OS_UNIX
  xint_t stdin_fd;
  xchar_t *stdin_path;

  xint_t stdout_fd;
  xchar_t *stdout_path;

  xint_t stderr_fd;
  xchar_t *stderr_path;

  xarray_t *source_fds;  /* xsubprocess_launcher_t has ownership of the FD elements */
  xarray_t *target_fds;  /* always the same length as source_fds; elements are just integers and not FDs in this process */
  xboolean_t closed_fd;

  GSpawnChildSetupFunc child_setup_func;
  xpointer_t child_setup_user_data;
  xdestroy_notify_t child_setup_destroy_notify;
#endif
};

void xsubprocess_set_launcher (xsubprocess_t         *subprocess,
                                xsubprocess_launcher_t *launcher);

G_END_DECLS

#endif
