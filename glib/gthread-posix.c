/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gthread.c: posix thread system implementation
 * Copyright 1998 Sebastian Wilhelmi; University of Karlsruhe
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/* The xmutex_t, xcond_t and xprivate_t implementations in this file are some
 * of the lowest-level code in GLib.  All other parts of GLib (messages,
 * memory, slices, etc) assume that they can freely use these facilities
 * without risking recursion.
 *
 * As such, these functions are NOT permitted to call any other part of
 * GLib.
 *
 * The thread manipulation functions (create, exit, join, etc.) have
 * more freedom -- they can do as they please.
 */

#include "config.h"

#include "gthread.h"

#include "gmain.h"
#include "gmessages.h"
#include "gslice.h"
#include "gstrfuncs.h"
#include "gtestutils.h"
#include "gthreadprivate.h"
#include "gutils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <sys/time.h>
#include <unistd.h>

#ifdef HAVE_PTHREAD_SET_NAME_NP
#include <pthread_np.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#ifdef G_OS_WIN32
#include <windows.h>
#endif

#if defined(HAVE_SYS_SCHED_GETATTR)
#include <sys/syscall.h>
#endif

#if defined(HAVE_FUTEX) && \
    (defined(HAVE_STDATOMIC_H) || defined(__ATOMIC_SEQ_CST))
#define USE_NATIVE_MUTEX
#endif

static void
xthread_abort (xint_t         status,
                const xchar_t *function)
{
  fprintf (stderr, "GLib (gthread-posix.c): Unexpected error from C library during '%s': %s.  Aborting.\n",
           function, strerror (status));
  g_abort ();
}

/* {{{1 xmutex_t */

#if !defined(USE_NATIVE_MUTEX)

static pthread_mutex_t *
g_mutex_impl_new (void)
{
  pthread_mutexattr_t *pattr = NULL;
  pthread_mutex_t *mutex;
  xint_t status;
#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
  pthread_mutexattr_t attr;
#endif

  mutex = malloc (sizeof (pthread_mutex_t));
  if G_UNLIKELY (mutex == NULL)
    xthread_abort (errno, "malloc");

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
  pattr = &attr;
#endif

  if G_UNLIKELY ((status = pthread_mutex_init (mutex, pattr)) != 0)
    xthread_abort (status, "pthread_mutex_init");

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
  pthread_mutexattr_destroy (&attr);
#endif

  return mutex;
}

static void
g_mutex_impl_free (pthread_mutex_t *mutex)
{
  pthread_mutex_destroy (mutex);
  free (mutex);
}

static inline pthread_mutex_t *
g_mutex_get_impl (xmutex_t *mutex)
{
  pthread_mutex_t *impl = g_atomic_pointer_get (&mutex->p);

  if G_UNLIKELY (impl == NULL)
    {
      impl = g_mutex_impl_new ();
      if (!g_atomic_pointer_compare_and_exchange (&mutex->p, NULL, impl))
        g_mutex_impl_free (impl);
      impl = mutex->p;
    }

  return impl;
}


/**
 * g_mutex_init:
 * @mutex: an uninitialized #xmutex_t
 *
 * Initializes a #xmutex_t so that it can be used.
 *
 * This function is useful to initialize a mutex that has been
 * allocated on the stack, or as part of a larger structure.
 * It is not necessary to initialize a mutex that has been
 * statically allocated.
 *
 * |[<!-- language="C" -->
 *   typedef struct {
 *     xmutex_t m;
 *     ...
 *   } Blob;
 *
 * Blob *b;
 *
 * b = g_new (Blob, 1);
 * g_mutex_init (&b->m);
 * ]|
 *
 * To undo the effect of g_mutex_init() when a mutex is no longer
 * needed, use g_mutex_clear().
 *
 * Calling g_mutex_init() on an already initialized #xmutex_t leads
 * to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_mutex_init (xmutex_t *mutex)
{
  mutex->p = g_mutex_impl_new ();
}

/**
 * g_mutex_clear:
 * @mutex: an initialized #xmutex_t
 *
 * Frees the resources allocated to a mutex with g_mutex_init().
 *
 * This function should not be used with a #xmutex_t that has been
 * statically allocated.
 *
 * Calling g_mutex_clear() on a locked mutex leads to undefined
 * behaviour.
 *
 * Sine: 2.32
 */
void
g_mutex_clear (xmutex_t *mutex)
{
  g_mutex_impl_free (mutex->p);
}

/**
 * g_mutex_lock:
 * @mutex: a #xmutex_t
 *
 * Locks @mutex. If @mutex is already locked by another thread, the
 * current thread will block until @mutex is unlocked by the other
 * thread.
 *
 * #xmutex_t is neither guaranteed to be recursive nor to be
 * non-recursive.  As such, calling g_mutex_lock() on a #xmutex_t that has
 * already been locked by the same thread results in undefined behaviour
 * (including but not limited to deadlocks).
 */
void
g_mutex_lock (xmutex_t *mutex)
{
  xint_t status;

  if G_UNLIKELY ((status = pthread_mutex_lock (g_mutex_get_impl (mutex))) != 0)
    xthread_abort (status, "pthread_mutex_lock");
}

/**
 * g_mutex_unlock:
 * @mutex: a #xmutex_t
 *
 * Unlocks @mutex. If another thread is blocked in a g_mutex_lock()
 * call for @mutex, it will become unblocked and can lock @mutex itself.
 *
 * Calling g_mutex_unlock() on a mutex that is not locked by the
 * current thread leads to undefined behaviour.
 */
void
g_mutex_unlock (xmutex_t *mutex)
{
  xint_t status;

  if G_UNLIKELY ((status = pthread_mutex_unlock (g_mutex_get_impl (mutex))) != 0)
    xthread_abort (status, "pthread_mutex_unlock");
}

/**
 * g_mutex_trylock:
 * @mutex: a #xmutex_t
 *
 * Tries to lock @mutex. If @mutex is already locked by another thread,
 * it immediately returns %FALSE. Otherwise it locks @mutex and returns
 * %TRUE.
 *
 * #xmutex_t is neither guaranteed to be recursive nor to be
 * non-recursive.  As such, calling g_mutex_lock() on a #xmutex_t that has
 * already been locked by the same thread results in undefined behaviour
 * (including but not limited to deadlocks or arbitrary return values).
 *
 * Returns: %TRUE if @mutex could be locked
 */
xboolean_t
g_mutex_trylock (xmutex_t *mutex)
{
  xint_t status;

  if G_LIKELY ((status = pthread_mutex_trylock (g_mutex_get_impl (mutex))) == 0)
    return TRUE;

  if G_UNLIKELY (status != EBUSY)
    xthread_abort (status, "pthread_mutex_trylock");

  return FALSE;
}

#endif /* !defined(USE_NATIVE_MUTEX) */

/* {{{1 GRecMutex */

static pthread_mutex_t *
g_rec_mutex_impl_new (void)
{
  pthread_mutexattr_t attr;
  pthread_mutex_t *mutex;

  mutex = malloc (sizeof (pthread_mutex_t));
  if G_UNLIKELY (mutex == NULL)
    xthread_abort (errno, "malloc");

  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init (mutex, &attr);
  pthread_mutexattr_destroy (&attr);

  return mutex;
}

static void
g_rec_mutex_impl_free (pthread_mutex_t *mutex)
{
  pthread_mutex_destroy (mutex);
  free (mutex);
}

static inline pthread_mutex_t *
g_rec_mutex_get_impl (GRecMutex *rec_mutex)
{
  pthread_mutex_t *impl = g_atomic_pointer_get (&rec_mutex->p);

  if G_UNLIKELY (impl == NULL)
    {
      impl = g_rec_mutex_impl_new ();
      if (!g_atomic_pointer_compare_and_exchange (&rec_mutex->p, NULL, impl))
        g_rec_mutex_impl_free (impl);
      impl = rec_mutex->p;
    }

  return impl;
}

/**
 * g_rec_mutex_init:
 * @rec_mutex: an uninitialized #GRecMutex
 *
 * Initializes a #GRecMutex so that it can be used.
 *
 * This function is useful to initialize a recursive mutex
 * that has been allocated on the stack, or as part of a larger
 * structure.
 *
 * It is not necessary to initialise a recursive mutex that has been
 * statically allocated.
 *
 * |[<!-- language="C" -->
 *   typedef struct {
 *     GRecMutex m;
 *     ...
 *   } Blob;
 *
 * Blob *b;
 *
 * b = g_new (Blob, 1);
 * g_rec_mutex_init (&b->m);
 * ]|
 *
 * Calling g_rec_mutex_init() on an already initialized #GRecMutex
 * leads to undefined behaviour.
 *
 * To undo the effect of g_rec_mutex_init() when a recursive mutex
 * is no longer needed, use g_rec_mutex_clear().
 *
 * Since: 2.32
 */
void
g_rec_mutex_init (GRecMutex *rec_mutex)
{
  rec_mutex->p = g_rec_mutex_impl_new ();
}

/**
 * g_rec_mutex_clear:
 * @rec_mutex: an initialized #GRecMutex
 *
 * Frees the resources allocated to a recursive mutex with
 * g_rec_mutex_init().
 *
 * This function should not be used with a #GRecMutex that has been
 * statically allocated.
 *
 * Calling g_rec_mutex_clear() on a locked recursive mutex leads
 * to undefined behaviour.
 *
 * Sine: 2.32
 */
void
g_rec_mutex_clear (GRecMutex *rec_mutex)
{
  g_rec_mutex_impl_free (rec_mutex->p);
}

/**
 * g_rec_mutex_lock:
 * @rec_mutex: a #GRecMutex
 *
 * Locks @rec_mutex. If @rec_mutex is already locked by another
 * thread, the current thread will block until @rec_mutex is
 * unlocked by the other thread. If @rec_mutex is already locked
 * by the current thread, the 'lock count' of @rec_mutex is increased.
 * The mutex will only become available again when it is unlocked
 * as many times as it has been locked.
 *
 * Since: 2.32
 */
void
g_rec_mutex_lock (GRecMutex *mutex)
{
  pthread_mutex_lock (g_rec_mutex_get_impl (mutex));
}

/**
 * g_rec_mutex_unlock:
 * @rec_mutex: a #GRecMutex
 *
 * Unlocks @rec_mutex. If another thread is blocked in a
 * g_rec_mutex_lock() call for @rec_mutex, it will become unblocked
 * and can lock @rec_mutex itself.
 *
 * Calling g_rec_mutex_unlock() on a recursive mutex that is not
 * locked by the current thread leads to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_rec_mutex_unlock (GRecMutex *rec_mutex)
{
  pthread_mutex_unlock (rec_mutex->p);
}

/**
 * g_rec_mutex_trylock:
 * @rec_mutex: a #GRecMutex
 *
 * Tries to lock @rec_mutex. If @rec_mutex is already locked
 * by another thread, it immediately returns %FALSE. Otherwise
 * it locks @rec_mutex and returns %TRUE.
 *
 * Returns: %TRUE if @rec_mutex could be locked
 *
 * Since: 2.32
 */
xboolean_t
g_rec_mutex_trylock (GRecMutex *rec_mutex)
{
  if (pthread_mutex_trylock (g_rec_mutex_get_impl (rec_mutex)) != 0)
    return FALSE;

  return TRUE;
}

/* {{{1 GRWLock */

static pthread_rwlock_t *
g_rw_lock_impl_new (void)
{
  pthread_rwlock_t *rwlock;
  xint_t status;

  rwlock = malloc (sizeof (pthread_rwlock_t));
  if G_UNLIKELY (rwlock == NULL)
    xthread_abort (errno, "malloc");

  if G_UNLIKELY ((status = pthread_rwlock_init (rwlock, NULL)) != 0)
    xthread_abort (status, "pthread_rwlock_init");

  return rwlock;
}

static void
g_rw_lock_impl_free (pthread_rwlock_t *rwlock)
{
  pthread_rwlock_destroy (rwlock);
  free (rwlock);
}

static inline pthread_rwlock_t *
g_rw_lock_get_impl (GRWLock *lock)
{
  pthread_rwlock_t *impl = g_atomic_pointer_get (&lock->p);

  if G_UNLIKELY (impl == NULL)
    {
      impl = g_rw_lock_impl_new ();
      if (!g_atomic_pointer_compare_and_exchange (&lock->p, NULL, impl))
        g_rw_lock_impl_free (impl);
      impl = lock->p;
    }

  return impl;
}

/**
 * g_rw_lock_init:
 * @rw_lock: an uninitialized #GRWLock
 *
 * Initializes a #GRWLock so that it can be used.
 *
 * This function is useful to initialize a lock that has been
 * allocated on the stack, or as part of a larger structure.  It is not
 * necessary to initialise a reader-writer lock that has been statically
 * allocated.
 *
 * |[<!-- language="C" -->
 *   typedef struct {
 *     GRWLock l;
 *     ...
 *   } Blob;
 *
 * Blob *b;
 *
 * b = g_new (Blob, 1);
 * g_rw_lock_init (&b->l);
 * ]|
 *
 * To undo the effect of g_rw_lock_init() when a lock is no longer
 * needed, use g_rw_lock_clear().
 *
 * Calling g_rw_lock_init() on an already initialized #GRWLock leads
 * to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_rw_lock_init (GRWLock *rw_lock)
{
  rw_lock->p = g_rw_lock_impl_new ();
}

/**
 * g_rw_lock_clear:
 * @rw_lock: an initialized #GRWLock
 *
 * Frees the resources allocated to a lock with g_rw_lock_init().
 *
 * This function should not be used with a #GRWLock that has been
 * statically allocated.
 *
 * Calling g_rw_lock_clear() when any thread holds the lock
 * leads to undefined behaviour.
 *
 * Sine: 2.32
 */
void
g_rw_lock_clear (GRWLock *rw_lock)
{
  g_rw_lock_impl_free (rw_lock->p);
}

/**
 * g_rw_lock_writer_lock:
 * @rw_lock: a #GRWLock
 *
 * Obtain a write lock on @rw_lock. If another thread currently holds
 * a read or write lock on @rw_lock, the current thread will block
 * until all other threads have dropped their locks on @rw_lock.
 *
 * Calling g_rw_lock_writer_lock() while the current thread already
 * owns a read or write lock on @rw_lock leads to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_rw_lock_writer_lock (GRWLock *rw_lock)
{
  int retval = pthread_rwlock_wrlock (g_rw_lock_get_impl (rw_lock));

  if (retval != 0)
    g_critical ("Failed to get RW lock %p: %s", rw_lock, xstrerror (retval));
}

/**
 * g_rw_lock_writer_trylock:
 * @rw_lock: a #GRWLock
 *
 * Tries to obtain a write lock on @rw_lock. If another thread
 * currently holds a read or write lock on @rw_lock, it immediately
 * returns %FALSE.
 * Otherwise it locks @rw_lock and returns %TRUE.
 *
 * Returns: %TRUE if @rw_lock could be locked
 *
 * Since: 2.32
 */
xboolean_t
g_rw_lock_writer_trylock (GRWLock *rw_lock)
{
  if (pthread_rwlock_trywrlock (g_rw_lock_get_impl (rw_lock)) != 0)
    return FALSE;

  return TRUE;
}

/**
 * g_rw_lock_writer_unlock:
 * @rw_lock: a #GRWLock
 *
 * Release a write lock on @rw_lock.
 *
 * Calling g_rw_lock_writer_unlock() on a lock that is not held
 * by the current thread leads to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_rw_lock_writer_unlock (GRWLock *rw_lock)
{
  pthread_rwlock_unlock (g_rw_lock_get_impl (rw_lock));
}

/**
 * g_rw_lock_reader_lock:
 * @rw_lock: a #GRWLock
 *
 * Obtain a read lock on @rw_lock. If another thread currently holds
 * the write lock on @rw_lock, the current thread will block until the
 * write lock was (held and) released. If another thread does not hold
 * the write lock, but is waiting for it, it is implementation defined
 * whether the reader or writer will block. Read locks can be taken
 * recursively.
 *
 * Calling g_rw_lock_reader_lock() while the current thread already
 * owns a write lock leads to undefined behaviour. Read locks however
 * can be taken recursively, in which case you need to make sure to
 * call g_rw_lock_reader_unlock() the same amount of times.
 *
 * It is implementation-defined how many read locks are allowed to be
 * held on the same lock simultaneously. If the limit is hit,
 * or if a deadlock is detected, a critical warning will be emitted.
 *
 * Since: 2.32
 */
void
g_rw_lock_reader_lock (GRWLock *rw_lock)
{
  int retval = pthread_rwlock_rdlock (g_rw_lock_get_impl (rw_lock));

  if (retval != 0)
    g_critical ("Failed to get RW lock %p: %s", rw_lock, xstrerror (retval));
}

/**
 * g_rw_lock_reader_trylock:
 * @rw_lock: a #GRWLock
 *
 * Tries to obtain a read lock on @rw_lock and returns %TRUE if
 * the read lock was successfully obtained. Otherwise it
 * returns %FALSE.
 *
 * Returns: %TRUE if @rw_lock could be locked
 *
 * Since: 2.32
 */
xboolean_t
g_rw_lock_reader_trylock (GRWLock *rw_lock)
{
  if (pthread_rwlock_tryrdlock (g_rw_lock_get_impl (rw_lock)) != 0)
    return FALSE;

  return TRUE;
}

/**
 * g_rw_lock_reader_unlock:
 * @rw_lock: a #GRWLock
 *
 * Release a read lock on @rw_lock.
 *
 * Calling g_rw_lock_reader_unlock() on a lock that is not held
 * by the current thread leads to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_rw_lock_reader_unlock (GRWLock *rw_lock)
{
  pthread_rwlock_unlock (g_rw_lock_get_impl (rw_lock));
}

/* {{{1 xcond_t */

#if !defined(USE_NATIVE_MUTEX)

static pthread_cond_t *
g_cond_impl_new (void)
{
  pthread_condattr_t attr;
  pthread_cond_t *cond;
  xint_t status;

  pthread_condattr_init (&attr);

#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
#elif defined (HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined (CLOCK_MONOTONIC)
  if G_UNLIKELY ((status = pthread_condattr_setclock (&attr, CLOCK_MONOTONIC)) != 0)
    xthread_abort (status, "pthread_condattr_setclock");
#else
#error Cannot support xcond_t on your platform.
#endif

  cond = malloc (sizeof (pthread_cond_t));
  if G_UNLIKELY (cond == NULL)
    xthread_abort (errno, "malloc");

  if G_UNLIKELY ((status = pthread_cond_init (cond, &attr)) != 0)
    xthread_abort (status, "pthread_cond_init");

  pthread_condattr_destroy (&attr);

  return cond;
}

static void
g_cond_impl_free (pthread_cond_t *cond)
{
  pthread_cond_destroy (cond);
  free (cond);
}

static inline pthread_cond_t *
g_cond_get_impl (xcond_t *cond)
{
  pthread_cond_t *impl = g_atomic_pointer_get (&cond->p);

  if G_UNLIKELY (impl == NULL)
    {
      impl = g_cond_impl_new ();
      if (!g_atomic_pointer_compare_and_exchange (&cond->p, NULL, impl))
        g_cond_impl_free (impl);
      impl = cond->p;
    }

  return impl;
}

/**
 * g_cond_init:
 * @cond: an uninitialized #xcond_t
 *
 * Initialises a #xcond_t so that it can be used.
 *
 * This function is useful to initialise a #xcond_t that has been
 * allocated as part of a larger structure.  It is not necessary to
 * initialise a #xcond_t that has been statically allocated.
 *
 * To undo the effect of g_cond_init() when a #xcond_t is no longer
 * needed, use g_cond_clear().
 *
 * Calling g_cond_init() on an already-initialised #xcond_t leads
 * to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_cond_init (xcond_t *cond)
{
  cond->p = g_cond_impl_new ();
}

/**
 * g_cond_clear:
 * @cond: an initialised #xcond_t
 *
 * Frees the resources allocated to a #xcond_t with g_cond_init().
 *
 * This function should not be used with a #xcond_t that has been
 * statically allocated.
 *
 * Calling g_cond_clear() for a #xcond_t on which threads are
 * blocking leads to undefined behaviour.
 *
 * Since: 2.32
 */
void
g_cond_clear (xcond_t *cond)
{
  g_cond_impl_free (cond->p);
}

/**
 * g_cond_wait:
 * @cond: a #xcond_t
 * @mutex: a #xmutex_t that is currently locked
 *
 * Atomically releases @mutex and waits until @cond is signalled.
 * When this function returns, @mutex is locked again and owned by the
 * calling thread.
 *
 * When using condition variables, it is possible that a spurious wakeup
 * may occur (ie: g_cond_wait() returns even though g_cond_signal() was
 * not called).  It's also possible that a stolen wakeup may occur.
 * This is when g_cond_signal() is called, but another thread acquires
 * @mutex before this thread and modifies the state of the program in
 * such a way that when g_cond_wait() is able to return, the expected
 * condition is no longer met.
 *
 * For this reason, g_cond_wait() must always be used in a loop.  See
 * the documentation for #xcond_t for a complete example.
 **/
void
g_cond_wait (xcond_t  *cond,
             xmutex_t *mutex)
{
  xint_t status;

  if G_UNLIKELY ((status = pthread_cond_wait (g_cond_get_impl (cond), g_mutex_get_impl (mutex))) != 0)
    xthread_abort (status, "pthread_cond_wait");
}

/**
 * g_cond_signal:
 * @cond: a #xcond_t
 *
 * If threads are waiting for @cond, at least one of them is unblocked.
 * If no threads are waiting for @cond, this function has no effect.
 * It is good practice to hold the same lock as the waiting thread
 * while calling this function, though not required.
 */
void
g_cond_signal (xcond_t *cond)
{
  xint_t status;

  if G_UNLIKELY ((status = pthread_cond_signal (g_cond_get_impl (cond))) != 0)
    xthread_abort (status, "pthread_cond_signal");
}

/**
 * g_cond_broadcast:
 * @cond: a #xcond_t
 *
 * If threads are waiting for @cond, all of them are unblocked.
 * If no threads are waiting for @cond, this function has no effect.
 * It is good practice to lock the same mutex as the waiting threads
 * while calling this function, though not required.
 */
void
g_cond_broadcast (xcond_t *cond)
{
  xint_t status;

  if G_UNLIKELY ((status = pthread_cond_broadcast (g_cond_get_impl (cond))) != 0)
    xthread_abort (status, "pthread_cond_broadcast");
}

/**
 * g_cond_wait_until:
 * @cond: a #xcond_t
 * @mutex: a #xmutex_t that is currently locked
 * @end_time: the monotonic time to wait until
 *
 * Waits until either @cond is signalled or @end_time has passed.
 *
 * As with g_cond_wait() it is possible that a spurious or stolen wakeup
 * could occur.  For that reason, waiting on a condition variable should
 * always be in a loop, based on an explicitly-checked predicate.
 *
 * %TRUE is returned if the condition variable was signalled (or in the
 * case of a spurious wakeup).  %FALSE is returned if @end_time has
 * passed.
 *
 * The following code shows how to correctly perform a timed wait on a
 * condition variable (extending the example presented in the
 * documentation for #xcond_t):
 *
 * |[<!-- language="C" -->
 * xpointer_t
 * pop_data_timed (void)
 * {
 *   sint64_t end_time;
 *   xpointer_t data;
 *
 *   g_mutex_lock (&data_mutex);
 *
 *   end_time = g_get_monotonic_time () + 5 * G_TIME_SPAN_SECOND;
 *   while (!current_data)
 *     if (!g_cond_wait_until (&data_cond, &data_mutex, end_time))
 *       {
 *         // timeout has passed.
 *         g_mutex_unlock (&data_mutex);
 *         return NULL;
 *       }
 *
 *   // there is data for us
 *   data = current_data;
 *   current_data = NULL;
 *
 *   g_mutex_unlock (&data_mutex);
 *
 *   return data;
 * }
 * ]|
 *
 * Notice that the end time is calculated once, before entering the
 * loop and reused.  This is the motivation behind the use of absolute
 * time on this API -- if a relative time of 5 seconds were passed
 * directly to the call and a spurious wakeup occurred, the program would
 * have to start over waiting again (which would lead to a total wait
 * time of more than 5 seconds).
 *
 * Returns: %TRUE on a signal, %FALSE on a timeout
 * Since: 2.32
 **/
xboolean_t
g_cond_wait_until (xcond_t  *cond,
                   xmutex_t *mutex,
                   sint64_t  end_time)
{
  struct timespec ts;
  xint_t status;

#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
  /* end_time is given relative to the monotonic clock as returned by
   * g_get_monotonic_time().
   *
   * Since this pthreads wants the relative time, convert it back again.
   */
  {
    sint64_t now = g_get_monotonic_time ();
    sint64_t relative;

    if (end_time <= now)
      return FALSE;

    relative = end_time - now;

    ts.tv_sec = relative / 1000000;
    ts.tv_nsec = (relative % 1000000) * 1000;

    if ((status = pthread_cond_timedwait_relative_np (g_cond_get_impl (cond), g_mutex_get_impl (mutex), &ts)) == 0)
      return TRUE;
  }
#elif defined (HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined (CLOCK_MONOTONIC)
  /* This is the exact check we used during init to set the clock to
   * monotonic, so if we're in this branch, timedwait() will already be
   * expecting a monotonic clock.
   */
  {
    ts.tv_sec = end_time / 1000000;
    ts.tv_nsec = (end_time % 1000000) * 1000;

    if ((status = pthread_cond_timedwait (g_cond_get_impl (cond), g_mutex_get_impl (mutex), &ts)) == 0)
      return TRUE;
  }
#else
#error Cannot support xcond_t on your platform.
#endif

  if G_UNLIKELY (status != ETIMEDOUT)
    xthread_abort (status, "pthread_cond_timedwait");

  return FALSE;
}

#endif /* defined(USE_NATIVE_MUTEX) */

/* {{{1 xprivate_t */

/**
 * xprivate_t:
 *
 * The #xprivate_t struct is an opaque data structure to represent a
 * thread-local data key. It is approximately equivalent to the
 * pthread_setspecific()/pthread_getspecific() APIs on POSIX and to
 * TlsSetValue()/TlsGetValue() on Windows.
 *
 * If you don't already know why you might want this functionality,
 * then you probably don't need it.
 *
 * #xprivate_t is a very limited resource (as far as 128 per program,
 * shared between all libraries). It is also not possible to destroy a
 * #xprivate_t after it has been used. As such, it is only ever acceptable
 * to use #xprivate_t in static scope, and even then sparingly so.
 *
 * See G_PRIVATE_INIT() for a couple of examples.
 *
 * The #xprivate_t structure should be considered opaque.  It should only
 * be accessed via the g_private_ functions.
 */

/**
 * G_PRIVATE_INIT:
 * @notify: a #xdestroy_notify_t
 *
 * A macro to assist with the static initialisation of a #xprivate_t.
 *
 * This macro is useful for the case that a #xdestroy_notify_t function
 * should be associated with the key.  This is needed when the key will be
 * used to point at memory that should be deallocated when the thread
 * exits.
 *
 * Additionally, the #xdestroy_notify_t will also be called on the previous
 * value stored in the key when g_private_replace() is used.
 *
 * If no #xdestroy_notify_t is needed, then use of this macro is not
 * required -- if the #xprivate_t is declared in static scope then it will
 * be properly initialised by default (ie: to all zeros).  See the
 * examples below.
 *
 * |[<!-- language="C" -->
 * static xprivate_t name_key = G_PRIVATE_INIT (g_free);
 *
 * // return value should not be freed
 * const xchar_t *
 * get_local_name (void)
 * {
 *   return g_private_get (&name_key);
 * }
 *
 * void
 * set_local_name (const xchar_t *name)
 * {
 *   g_private_replace (&name_key, xstrdup (name));
 * }
 *
 *
 * static xprivate_t count_key;   // no free function
 *
 * xint_t
 * get_local_count (void)
 * {
 *   return GPOINTER_TO_INT (g_private_get (&count_key));
 * }
 *
 * void
 * set_local_count (xint_t count)
 * {
 *   g_private_set (&count_key, GINT_TO_POINTER (count));
 * }
 * ]|
 *
 * Since: 2.32
 **/

static pthread_key_t *
g_private_impl_new (xdestroy_notify_t notify)
{
  pthread_key_t *key;
  xint_t status;

  key = malloc (sizeof (pthread_key_t));
  if G_UNLIKELY (key == NULL)
    xthread_abort (errno, "malloc");
  status = pthread_key_create (key, notify);
  if G_UNLIKELY (status != 0)
    xthread_abort (status, "pthread_key_create");

  return key;
}

static void
g_private_impl_free (pthread_key_t *key)
{
  xint_t status;

  status = pthread_key_delete (*key);
  if G_UNLIKELY (status != 0)
    xthread_abort (status, "pthread_key_delete");
  free (key);
}

static inline pthread_key_t *
g_private_get_impl (xprivate_t *key)
{
  pthread_key_t *impl = g_atomic_pointer_get (&key->p);

  if G_UNLIKELY (impl == NULL)
    {
      impl = g_private_impl_new (key->notify);
      if (!g_atomic_pointer_compare_and_exchange (&key->p, NULL, impl))
        {
          g_private_impl_free (impl);
          impl = key->p;
        }
    }

  return impl;
}

/**
 * g_private_get:
 * @key: a #xprivate_t
 *
 * Returns the current value of the thread local variable @key.
 *
 * If the value has not yet been set in this thread, %NULL is returned.
 * Values are never copied between threads (when a new thread is
 * created, for example).
 *
 * Returns: the thread-local value
 */
xpointer_t
g_private_get (xprivate_t *key)
{
  /* quote POSIX: No errors are returned from pthread_getspecific(). */
  return pthread_getspecific (*g_private_get_impl (key));
}

/**
 * g_private_set:
 * @key: a #xprivate_t
 * @value: the new value
 *
 * Sets the thread local variable @key to have the value @value in the
 * current thread.
 *
 * This function differs from g_private_replace() in the following way:
 * the #xdestroy_notify_t for @key is not called on the old value.
 */
void
g_private_set (xprivate_t *key,
               xpointer_t  value)
{
  xint_t status;

  if G_UNLIKELY ((status = pthread_setspecific (*g_private_get_impl (key), value)) != 0)
    xthread_abort (status, "pthread_setspecific");
}

/**
 * g_private_replace:
 * @key: a #xprivate_t
 * @value: the new value
 *
 * Sets the thread local variable @key to have the value @value in the
 * current thread.
 *
 * This function differs from g_private_set() in the following way: if
 * the previous value was non-%NULL then the #xdestroy_notify_t handler for
 * @key is run on it.
 *
 * Since: 2.32
 **/
void
g_private_replace (xprivate_t *key,
                   xpointer_t  value)
{
  pthread_key_t *impl = g_private_get_impl (key);
  xpointer_t old;
  xint_t status;

  old = pthread_getspecific (*impl);

  if G_UNLIKELY ((status = pthread_setspecific (*impl, value)) != 0)
    xthread_abort (status, "pthread_setspecific");

  if (old && key->notify)
    key->notify (old);
}

/* {{{1 xthread_t */

#define posix_check_err(err, name) G_STMT_START{			\
  int error = (err); 							\
  if (error)	 		 		 			\
    xerror ("file %s: line %d (%s): error '%s' during '%s'",		\
           __FILE__, __LINE__, G_STRFUNC,				\
           xstrerror (error), name);					\
  }G_STMT_END

#define posix_check_cmd(cmd) posix_check_err (cmd, #cmd)

typedef struct
{
  GRealThread thread;

  pthread_t system_thread;
  xboolean_t  joined;
  xmutex_t    lock;

  void *(*proxy) (void *);

  /* Must be statically allocated and valid forever */
  const GThreadSchedulerSettings *scheduler_settings;
} GThreadPosix;

void
g_system_thread_free (GRealThread *thread)
{
  GThreadPosix *pt = (GThreadPosix *) thread;

  if (!pt->joined)
    pthread_detach (pt->system_thread);

  g_mutex_clear (&pt->lock);

  g_slice_free (GThreadPosix, pt);
}

xboolean_t
g_system_thread_get_scheduler_settings (GThreadSchedulerSettings *scheduler_settings)
{
  /* FIXME: Implement the same for macOS and the BSDs so it doesn't go through
   * the fallback code using an additional thread. */
#if defined(HAVE_SYS_SCHED_GETATTR)
  pid_t tid;
  int res;
  /* FIXME: The struct definition does not seem to be possible to pull in
   * via any of the normal system headers and it's only declared in the
   * kernel headers. That's why we hardcode 56 here right now. */
  xuint_t size = 56; /* Size as of Linux 5.3.9 */
  xuint_t flags = 0;

  tid = (pid_t) syscall (SYS_gettid);

  scheduler_settings->attr = g_malloc0 (size);

  do
    {
      int errsv;

      res = syscall (SYS_sched_getattr, tid, scheduler_settings->attr, size, flags);
      errsv = errno;
      if (res == -1)
        {
          if (errsv == EAGAIN)
            {
              continue;
            }
          else if (errsv == E2BIG)
            {
              xassert (size < G_MAXINT);
              size *= 2;
              scheduler_settings->attr = g_realloc (scheduler_settings->attr, size);
              /* Needs to be zero-initialized */
              memset (scheduler_settings->attr, 0, size);
            }
          else
            {
              g_debug ("Failed to get thread scheduler attributes: %s", xstrerror (errsv));
              g_free (scheduler_settings->attr);

              return FALSE;
            }
        }
    }
  while (res == -1);

  /* Try setting them on the current thread to see if any system policies are
   * in place that would disallow doing so */
  res = syscall (SYS_sched_setattr, tid, scheduler_settings->attr, flags);
  if (res == -1)
    {
      int errsv = errno;

      g_debug ("Failed to set thread scheduler attributes: %s", xstrerror (errsv));
      g_free (scheduler_settings->attr);

      return FALSE;
    }

  return TRUE;
#else
  return FALSE;
#endif
}

#if defined(HAVE_SYS_SCHED_GETATTR)
static void *
linux_pthread_proxy (void *data)
{
  GThreadPosix *thread = data;
  static xboolean_t printed_scheduler_warning = FALSE;  /* (atomic) */

  /* Set scheduler settings first if requested */
  if (thread->scheduler_settings)
    {
      pid_t tid = 0;
      xuint_t flags = 0;
      int res;
      int errsv;

      tid = (pid_t) syscall (SYS_gettid);
      res = syscall (SYS_sched_setattr, tid, thread->scheduler_settings->attr, flags);
      errsv = errno;
      if (res == -1 && g_atomic_int_compare_and_exchange (&printed_scheduler_warning, FALSE, TRUE))
        g_critical ("Failed to set scheduler settings: %s", xstrerror (errsv));
      else if (res == -1)
        g_debug ("Failed to set scheduler settings: %s", xstrerror (errsv));
      printed_scheduler_warning = TRUE;
    }

  return thread->proxy (data);
}
#endif

GRealThread *
g_system_thread_new (GThreadFunc proxy,
                     xulong_t stack_size,
                     const GThreadSchedulerSettings *scheduler_settings,
                     const char *name,
                     GThreadFunc func,
                     xpointer_t data,
                     xerror_t **error)
{
  GThreadPosix *thread;
  GRealThread *base_thread;
  pthread_attr_t attr;
  xint_t ret;

  thread = g_slice_new0 (GThreadPosix);
  base_thread = (GRealThread*)thread;
  base_thread->ref_count = 2;
  base_thread->ours = TRUE;
  base_thread->thread.joinable = TRUE;
  base_thread->thread.func = func;
  base_thread->thread.data = data;
  base_thread->name = xstrdup (name);
  thread->scheduler_settings = scheduler_settings;
  thread->proxy = proxy;

  posix_check_cmd (pthread_attr_init (&attr));

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
  if (stack_size)
    {
#ifdef _SC_THREAD_STACK_MIN
      long min_stack_size = sysconf (_SC_THREAD_STACK_MIN);
      if (min_stack_size >= 0)
        stack_size = MAX ((xulong_t) min_stack_size, stack_size);
#endif /* _SC_THREAD_STACK_MIN */
      /* No error check here, because some systems can't do it and
       * we simply don't want threads to fail because of that. */
      pthread_attr_setstacksize (&attr, stack_size);
    }
#endif /* HAVE_PTHREAD_ATTR_SETSTACKSIZE */

#ifdef HAVE_PTHREAD_ATTR_SETINHERITSCHED
  if (!scheduler_settings)
    {
      /* While this is the default, better be explicit about it */
      pthread_attr_setinheritsched (&attr, PTHREAD_INHERIT_SCHED);
    }
#endif /* HAVE_PTHREAD_ATTR_SETINHERITSCHED */

#if defined(HAVE_SYS_SCHED_GETATTR)
  ret = pthread_create (&thread->system_thread, &attr, linux_pthread_proxy, thread);
#else
  ret = pthread_create (&thread->system_thread, &attr, (void* (*)(void*))proxy, thread);
#endif

  posix_check_cmd (pthread_attr_destroy (&attr));

  if (ret == EAGAIN)
    {
      g_set_error (error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN,
                   "Error creating thread: %s", xstrerror (ret));
      g_free (thread->thread.name);
      g_slice_free (GThreadPosix, thread);
      return NULL;
    }

  posix_check_err (ret, "pthread_create");

  g_mutex_init (&thread->lock);

  return (GRealThread *) thread;
}

/**
 * xthread_yield:
 *
 * Causes the calling thread to voluntarily relinquish the CPU, so
 * that other threads can run.
 *
 * This function is often used as a method to make busy wait less evil.
 */
void
xthread_yield (void)
{
  sched_yield ();
}

void
g_system_thread_wait (GRealThread *thread)
{
  GThreadPosix *pt = (GThreadPosix *) thread;

  g_mutex_lock (&pt->lock);

  if (!pt->joined)
    {
      posix_check_cmd (pthread_join (pt->system_thread, NULL));
      pt->joined = TRUE;
    }

  g_mutex_unlock (&pt->lock);
}

void
g_system_thread_exit (void)
{
  pthread_exit (NULL);
}

void
g_system_thread_set_name (const xchar_t *name)
{
#if defined(HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)
  pthread_setname_np (name); /* on OS X and iOS */
#elif defined(HAVE_PTHREAD_SETNAME_NP_WITH_TID)
  pthread_setname_np (pthread_self (), name); /* on Linux and Solaris */
#elif defined(HAVE_PTHREAD_SETNAME_NP_WITH_TID_AND_ARG)
  pthread_setname_np (pthread_self (), "%s", (xchar_t *) name); /* on NetBSD */
#elif defined(HAVE_PTHREAD_SET_NAME_NP)
  pthread_set_name_np (pthread_self (), name); /* on FreeBSD, DragonFlyBSD, OpenBSD */
#endif
}

/* {{{1 xmutex_t and xcond_t futex implementation */

#if defined(USE_NATIVE_MUTEX)

#include <linux/futex.h>
#include <sys/syscall.h>

#ifndef FUTEX_WAIT_PRIVATE
#define FUTEX_WAIT_PRIVATE FUTEX_WAIT
#define FUTEX_WAKE_PRIVATE FUTEX_WAKE
#endif

/* We should expand the set of operations available in gatomic once we
 * have better C11 support in GCC in common distributions (ie: 4.9).
 *
 * Before then, let's define a couple of useful things for our own
 * purposes...
 */

#ifdef HAVE_STDATOMIC_H

#include <stdatomic.h>

#define exchange_acquire(ptr, new) \
  atomic_exchange_explicit((atomic_uint *) (ptr), (new), __ATOMIC_ACQUIRE)
#define compare_exchange_acquire(ptr, old, new) \
  atomic_compare_exchange_strong_explicit((atomic_uint *) (ptr), (old), (new), \
                                          __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

#define exchange_release(ptr, new) \
  atomic_exchange_explicit((atomic_uint *) (ptr), (new), __ATOMIC_RELEASE)
#define store_release(ptr, new) \
  atomic_store_explicit((atomic_uint *) (ptr), (new), __ATOMIC_RELEASE)

#else

#define exchange_acquire(ptr, new) \
  __atomic_exchange_4((ptr), (new), __ATOMIC_ACQUIRE)
#define compare_exchange_acquire(ptr, old, new) \
  __atomic_compare_exchange_4((ptr), (old), (new), 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

#define exchange_release(ptr, new) \
  __atomic_exchange_4((ptr), (new), __ATOMIC_RELEASE)
#define store_release(ptr, new) \
  __atomic_store_4((ptr), (new), __ATOMIC_RELEASE)

#endif

/* Our strategy for the mutex is pretty simple:
 *
 *  0: not in use
 *
 *  1: acquired by one thread only, no contention
 *
 *  > 1: contended
 *
 *
 * As such, attempting to acquire the lock should involve an increment.
 * If we find that the previous value was 0 then we can return
 * immediately.
 *
 * On unlock, we always store 0 to indicate that the lock is available.
 * If the value there was 1 before then we didn't have contention and
 * can return immediately.  If the value was something other than 1 then
 * we have the contended case and need to wake a waiter.
 *
 * If it was not 0 then there is another thread holding it and we must
 * wait.  We must always ensure that we mark a value >1 while we are
 * waiting in order to instruct the holder to do a wake operation on
 * unlock.
 */

void
g_mutex_init (xmutex_t *mutex)
{
  mutex->i[0] = 0;
}

void
g_mutex_clear (xmutex_t *mutex)
{
  if G_UNLIKELY (mutex->i[0] != 0)
    {
      fprintf (stderr, "g_mutex_clear() called on uninitialised or locked mutex\n");
      g_abort ();
    }
}

static void __attribute__((noinline))
g_mutex_lock_slowpath (xmutex_t *mutex)
{
  /* Set to 2 to indicate contention.  If it was zero before then we
   * just acquired the lock.
   *
   * Otherwise, sleep for as long as the 2 remains...
   */
  while (exchange_acquire (&mutex->i[0], 2) != 0)
    syscall (__NR_futex, &mutex->i[0], (xsize_t) FUTEX_WAIT_PRIVATE, (xsize_t) 2, NULL);
}

static void __attribute__((noinline))
g_mutex_unlock_slowpath (xmutex_t *mutex,
                         xuint_t   prev)
{
  /* We seem to get better code for the uncontended case by splitting
   * this out...
   */
  if G_UNLIKELY (prev == 0)
    {
      fprintf (stderr, "Attempt to unlock mutex that was not locked\n");
      g_abort ();
    }

  syscall (__NR_futex, &mutex->i[0], (xsize_t) FUTEX_WAKE_PRIVATE, (xsize_t) 1, NULL);
}

void
g_mutex_lock (xmutex_t *mutex)
{
  /* 0 -> 1 and we're done.  Anything else, and we need to wait... */
  if G_UNLIKELY (g_atomic_int_add (&mutex->i[0], 1) != 0)
    g_mutex_lock_slowpath (mutex);
}

void
g_mutex_unlock (xmutex_t *mutex)
{
  xuint_t prev;

  prev = exchange_release (&mutex->i[0], 0);

  /* 1-> 0 and we're done.  Anything else and we need to signal... */
  if G_UNLIKELY (prev != 1)
    g_mutex_unlock_slowpath (mutex, prev);
}

xboolean_t
g_mutex_trylock (xmutex_t *mutex)
{
  xuint_t zero = 0;

  /* We don't want to touch the value at all unless we can move it from
   * exactly 0 to 1.
   */
  return compare_exchange_acquire (&mutex->i[0], &zero, 1);
}

/* Condition variables are implemented in a rather simple way as well.
 * In many ways, futex() as an abstraction is even more ideally suited
 * to condition variables than it is to mutexes.
 *
 * We store a generation counter.  We sample it with the lock held and
 * unlock before sleeping on the futex.
 *
 * Signalling simply involves increasing the counter and making the
 * appropriate futex call.
 *
 * The only thing that is the slightest bit complicated is timed waits
 * because we must convert our absolute time to relative.
 */

void
g_cond_init (xcond_t *cond)
{
  cond->i[0] = 0;
}

void
g_cond_clear (xcond_t *cond)
{
}

void
g_cond_wait (xcond_t  *cond,
             xmutex_t *mutex)
{
  xuint_t sampled = (xuint_t) g_atomic_int_get (&cond->i[0]);

  g_mutex_unlock (mutex);
  syscall (__NR_futex, &cond->i[0], (xsize_t) FUTEX_WAIT_PRIVATE, (xsize_t) sampled, NULL);
  g_mutex_lock (mutex);
}

void
g_cond_signal (xcond_t *cond)
{
  g_atomic_int_inc (&cond->i[0]);

  syscall (__NR_futex, &cond->i[0], (xsize_t) FUTEX_WAKE_PRIVATE, (xsize_t) 1, NULL);
}

void
g_cond_broadcast (xcond_t *cond)
{
  g_atomic_int_inc (&cond->i[0]);

  syscall (__NR_futex, &cond->i[0], (xsize_t) FUTEX_WAKE_PRIVATE, (xsize_t) INT_MAX, NULL);
}

xboolean_t
g_cond_wait_until (xcond_t  *cond,
                   xmutex_t *mutex,
                   sint64_t  end_time)
{
  struct timespec now;
  struct timespec span;
  xuint_t sampled;
  int res;
  xboolean_t success;

  if (end_time < 0)
    return FALSE;

  clock_gettime (CLOCK_MONOTONIC, &now);
  span.tv_sec = (end_time / 1000000) - now.tv_sec;
  span.tv_nsec = ((end_time % 1000000) * 1000) - now.tv_nsec;
  if (span.tv_nsec < 0)
    {
      span.tv_nsec += 1000000000;
      span.tv_sec--;
    }

  if (span.tv_sec < 0)
    return FALSE;

  sampled = cond->i[0];
  g_mutex_unlock (mutex);
  res = syscall (__NR_futex, &cond->i[0], (xsize_t) FUTEX_WAIT_PRIVATE, (xsize_t) sampled, &span);
  success = (res < 0 && errno == ETIMEDOUT) ? FALSE : TRUE;
  g_mutex_lock (mutex);

  return success;
}

#endif

  /* {{{1 Epilogue */
/* vim:set foldmethod=marker: */
