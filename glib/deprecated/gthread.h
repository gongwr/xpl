/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_DEPRECATED_THREAD_H__
#define __G_DEPRECATED_THREAD_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gthread.h>

G_BEGIN_DECLS

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef enum
{
  G_THREAD_PRIORITY_LOW,
  G_THREAD_PRIORITY_NORMAL,
  G_THREAD_PRIORITY_HIGH,
  G_THREAD_PRIORITY_URGENT
} GThreadPriority XPL_DEPRECATED_TYPE_IN_2_32;

struct  _GThread
{
  /*< private >*/
  GThreadFunc func;
  xpointer_t data;
  xboolean_t joinable;
  GThreadPriority priority;
};

typedef struct _GThreadFunctions GThreadFunctions XPL_DEPRECATED_TYPE_IN_2_32;
struct _GThreadFunctions
{
  GMutex*  (*mutex_new)           (void);
  void     (*mutex_lock)          (GMutex               *mutex);
  xboolean_t (*mutex_trylock)       (GMutex               *mutex);
  void     (*mutex_unlock)        (GMutex               *mutex);
  void     (*mutex_free)          (GMutex               *mutex);
  GCond*   (*cond_new)            (void);
  void     (*cond_signal)         (GCond                *cond);
  void     (*cond_broadcast)      (GCond                *cond);
  void     (*cond_wait)           (GCond                *cond,
                                   GMutex               *mutex);
  xboolean_t (*cond_timed_wait)     (GCond                *cond,
                                   GMutex               *mutex,
                                   GTimeVal             *end_time);
  void      (*cond_free)          (GCond                *cond);
  GPrivate* (*private_new)        (GDestroyNotify        destructor);
  xpointer_t  (*private_get)        (GPrivate             *private_key);
  void      (*private_set)        (GPrivate             *private_key,
                                   xpointer_t              data);
  void      (*thread_create)      (GThreadFunc           func,
                                   xpointer_t              data,
                                   gulong                stack_size,
                                   xboolean_t              joinable,
                                   xboolean_t              bound,
                                   GThreadPriority       priority,
                                   xpointer_t              thread,
                                   xerror_t              **error);
  void      (*thread_yield)       (void);
  void      (*thread_join)        (xpointer_t              thread);
  void      (*thread_exit)        (void);
  void      (*thread_set_priority)(xpointer_t              thread,
                                   GThreadPriority       priority);
  void      (*thread_self)        (xpointer_t              thread);
  xboolean_t  (*thread_equal)       (xpointer_t              thread1,
                                   xpointer_t              thread2);
} XPL_DEPRECATED_TYPE_IN_2_32;

XPL_VAR GThreadFunctions       g_thread_functions_for_glib_use;
XPL_VAR xboolean_t               g_thread_use_default_impl;

XPL_VAR guint64   (*g_thread_gettime) (void);

XPL_DEPRECATED_IN_2_32_FOR(g_thread_new)
GThread *g_thread_create       (GThreadFunc       func,
                                xpointer_t          data,
                                xboolean_t          joinable,
                                xerror_t          **error);

XPL_DEPRECATED_IN_2_32_FOR(g_thread_new)
GThread *g_thread_create_full  (GThreadFunc       func,
                                xpointer_t          data,
                                gulong            stack_size,
                                xboolean_t          joinable,
                                xboolean_t          bound,
                                GThreadPriority   priority,
                                xerror_t          **error);

XPL_DEPRECATED_IN_2_32
void     g_thread_set_priority (GThread          *thread,
                                GThreadPriority   priority);

XPL_DEPRECATED_IN_2_32
void     g_thread_foreach      (GFunc             thread_func,
                                xpointer_t          user_data);

#ifndef G_OS_WIN32
#include <sys/types.h>
#include <pthread.h>
#endif

#define g_static_mutex_get_mutex g_static_mutex_get_mutex_impl XPL_DEPRECATED_MACRO_IN_2_32
#ifndef G_OS_WIN32
#define G_STATIC_MUTEX_INIT { NULL, PTHREAD_MUTEX_INITIALIZER } XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_mutex_init)
#else
#define G_STATIC_MUTEX_INIT { NULL } XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_mutex_init)
#endif
typedef struct
{
  GMutex *mutex;
#ifndef G_OS_WIN32
  /* only for ABI compatibility reasons */
  pthread_mutex_t unused;
#endif
} GStaticMutex XPL_DEPRECATED_TYPE_IN_2_32_FOR(GMutex);

#define g_static_mutex_lock(mutex) \
    g_mutex_lock (g_static_mutex_get_mutex (mutex)) XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_mutex_lock)
#define g_static_mutex_trylock(mutex) \
    g_mutex_trylock (g_static_mutex_get_mutex (mutex)) XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_mutex_trylock)
#define g_static_mutex_unlock(mutex) \
    g_mutex_unlock (g_static_mutex_get_mutex (mutex)) XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_mutex_unlock)

XPL_DEPRECATED_IN_2_32_FOR(g_mutex_init)
void    g_static_mutex_init           (GStaticMutex *mutex);
XPL_DEPRECATED_IN_2_32_FOR(g_mutex_clear)
void    g_static_mutex_free           (GStaticMutex *mutex);
XPL_DEPRECATED_IN_2_32_FOR(GMutex)
GMutex *g_static_mutex_get_mutex_impl (GStaticMutex *mutex);

typedef struct _GStaticRecMutex GStaticRecMutex XPL_DEPRECATED_TYPE_IN_2_32_FOR(GRecMutex);
struct _GStaticRecMutex
{
  /*< private >*/
  GStaticMutex mutex;
  xuint_t depth;

  /* ABI compat only */
  union {
#ifdef G_OS_WIN32
    void *owner;
#else
    pthread_t owner;
#endif
    xdouble_t dummy;
  } unused;
} XPL_DEPRECATED_TYPE_IN_2_32_FOR(GRecMutex);

#define G_STATIC_REC_MUTEX_INIT { G_STATIC_MUTEX_INIT, 0, { 0 } } XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_rec_mutex_init)
XPL_DEPRECATED_IN_2_32_FOR(g_rec_mutex_init)
void     g_static_rec_mutex_init        (GStaticRecMutex *mutex);

XPL_DEPRECATED_IN_2_32_FOR(g_rec_mutex_lock)
void     g_static_rec_mutex_lock        (GStaticRecMutex *mutex);

XPL_DEPRECATED_IN_2_32_FOR(g_rec_mutex_try_lock)
xboolean_t g_static_rec_mutex_trylock     (GStaticRecMutex *mutex);

XPL_DEPRECATED_IN_2_32_FOR(g_rec_mutex_unlock)
void     g_static_rec_mutex_unlock      (GStaticRecMutex *mutex);

XPL_DEPRECATED_IN_2_32
void     g_static_rec_mutex_lock_full   (GStaticRecMutex *mutex,
                                         xuint_t            depth);

XPL_DEPRECATED_IN_2_32
xuint_t    g_static_rec_mutex_unlock_full (GStaticRecMutex *mutex);

XPL_DEPRECATED_IN_2_32_FOR(g_rec_mutex_free)
void     g_static_rec_mutex_free        (GStaticRecMutex *mutex);

typedef struct _GStaticRWLock GStaticRWLock XPL_DEPRECATED_TYPE_IN_2_32_FOR(GRWLock);
struct _GStaticRWLock
{
  /*< private >*/
  GStaticMutex mutex;
  GCond *read_cond;
  GCond *write_cond;
  xuint_t read_counter;
  xboolean_t have_writer;
  xuint_t want_to_read;
  xuint_t want_to_write;
} XPL_DEPRECATED_TYPE_IN_2_32_FOR(GRWLock);

#define G_STATIC_RW_LOCK_INIT { G_STATIC_MUTEX_INIT, NULL, NULL, 0, FALSE, 0, 0 } XPL_DEPRECATED_MACRO_IN_2_32_FOR(g_rw_lock_init)

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_init)
void      g_static_rw_lock_init           (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_reader_lock)
void      g_static_rw_lock_reader_lock    (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_reader_trylock)
xboolean_t  g_static_rw_lock_reader_trylock (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_reader_unlock)
void      g_static_rw_lock_reader_unlock  (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_writer_lock)
void      g_static_rw_lock_writer_lock    (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_writer_trylock)
xboolean_t  g_static_rw_lock_writer_trylock (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_writer_unlock)
void      g_static_rw_lock_writer_unlock  (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32_FOR(g_rw_lock_free)
void      g_static_rw_lock_free           (GStaticRWLock *lock);

XPL_DEPRECATED_IN_2_32
GPrivate *      g_private_new             (GDestroyNotify notify);

typedef struct _GStaticPrivate  GStaticPrivate XPL_DEPRECATED_TYPE_IN_2_32_FOR(GPrivate);
struct _GStaticPrivate
{
  /*< private >*/
  xuint_t index;
} XPL_DEPRECATED_TYPE_IN_2_32_FOR(GPrivate);

#define G_STATIC_PRIVATE_INIT { 0 } XPL_DEPRECATED_MACRO_IN_2_32_FOR(G_PRIVATE_INIT)
XPL_DEPRECATED_IN_2_32
void     g_static_private_init           (GStaticPrivate *private_key);

XPL_DEPRECATED_IN_2_32_FOR(g_private_get)
xpointer_t g_static_private_get            (GStaticPrivate *private_key);

XPL_DEPRECATED_IN_2_32_FOR(g_private_set)
void     g_static_private_set            (GStaticPrivate *private_key,
                                          xpointer_t        data,
                                          GDestroyNotify  notify);

XPL_DEPRECATED_IN_2_32
void     g_static_private_free           (GStaticPrivate *private_key);

XPL_DEPRECATED_IN_2_32
xboolean_t g_once_init_enter_impl          (volatile xsize_t *location);

XPL_DEPRECATED_IN_2_32
void     g_thread_init                   (xpointer_t vtable);
XPL_DEPRECATED_IN_2_32
void    g_thread_init_with_errorcheck_mutexes (xpointer_t vtable);

XPL_DEPRECATED_IN_2_32
xboolean_t g_thread_get_initialized        (void);

XPL_VAR xboolean_t g_threads_got_initialized;

#define g_thread_supported()     (1) XPL_DEPRECATED_MACRO_IN_2_32

XPL_DEPRECATED_IN_2_32
GMutex *        g_mutex_new             (void);
XPL_DEPRECATED_IN_2_32
void            g_mutex_free            (GMutex *mutex);
XPL_DEPRECATED_IN_2_32
GCond *         g_cond_new              (void);
XPL_DEPRECATED_IN_2_32
void            g_cond_free             (GCond  *cond);
XPL_DEPRECATED_IN_2_32
xboolean_t        g_cond_timed_wait       (GCond          *cond,
                                         GMutex         *mutex,
                                         GTimeVal       *timeval);

G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS

#endif /* __G_DEPRECATED_THREAD_H__ */