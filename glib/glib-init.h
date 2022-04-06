/*
 * Copyright © 2011 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __XPL_INIT_H__
#define __XPL_INIT_H__

#include "gmessages.h"

extern GLogLevelFlags g_log_always_fatal;
extern GLogLevelFlags g_log_msg_prefix;

void glib_init (void);
void g_quark_init (void);
void xerror_init (void);

#ifdef G_OS_WIN32
#include <windows.h>

void xthread_win32_process_detach (void);
void xthread_win32_thread_detach (void);
void xthread_win32_init (void);
void g_console_win32_init (void);
void g_clock_win32_init (void);
void g_crash_handler_win32_init (void);
void g_crash_handler_win32_deinit (void);
xboolean_t _g_win32_call_rtl_version (OSVERSIONINFOEXW *info);

extern HMODULE glib_dll;
xchar_t *g_win32_find_helper_executable_path (const xchar_t *process_name, void *dll_handle);
#endif

#endif /* __XPL_INIT_H__ */
