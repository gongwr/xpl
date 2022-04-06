/* XPL - Library of useful routines for C programming
 *
 * gthreadprivate.h - GLib internal thread system related declarations.
 *
 *  Copyright (C) 2003 Sebastian Wilhelmi
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_THREADPRIVATE_H__
#define __G_THREADPRIVATE_H__

#include "config.h"

#include "deprecated/gthread.h"

typedef struct _GRealThread GRealThread;
struct  _GRealThread
{
  xthread_t thread;

  xint_t ref_count;
  xboolean_t ours;
  xchar_t *name;
  xpointer_t retval;
};

/* system thread implementation (gthread-posix.c, gthread-win32.c) */

/* Platform-specific scheduler settings for a thread */
typedef struct
{
#if defined(HAVE_SYS_SCHED_GETATTR)
  /* This is for modern Linux */
  struct sched_attr *attr;
#elif defined(G_OS_WIN32)
  xint_t thread_prio;
#else
  /* TODO: Add support for macOS and the BSDs */
  void *dummy;
#endif
} GThreadSchedulerSettings;

void            g_system_thread_wait            (GRealThread  *thread);

GRealThread *g_system_thread_new (GThreadFunc proxy,
                                  xulong_t stack_size,
                                  const GThreadSchedulerSettings *scheduler_settings,
                                  const char *name,
                                  GThreadFunc func,
                                  xpointer_t data,
                                  xerror_t **error);
void            g_system_thread_free            (GRealThread  *thread);

void            g_system_thread_exit            (void);
void            g_system_thread_set_name        (const xchar_t  *name);

xboolean_t        g_system_thread_get_scheduler_settings (GThreadSchedulerSettings *scheduler_settings);

/* gthread.c */
xthread_t *xthread_new_internal (const xchar_t *name,
                                GThreadFunc proxy,
                                GThreadFunc func,
                                xpointer_t data,
                                xsize_t stack_size,
                                const GThreadSchedulerSettings *scheduler_settings,
                                xerror_t **error);

xboolean_t xthread_get_scheduler_settings (GThreadSchedulerSettings *scheduler_settings);

xpointer_t        xthread_proxy                  (xpointer_t      thread);

xuint_t           xthread_n_created              (void);

xpointer_t        g_private_set_alloc0            (GPrivate       *key,
                                                 xsize_t           size);

#endif /* __G_THREADPRIVATE_H__ */
