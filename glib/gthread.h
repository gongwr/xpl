/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_THREAD_H__
#define __G_THREAD_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gatomic.h>
#include <glib/gerror.h>
#include <glib/gutils.h>

G_BEGIN_DECLS

#define G_THREAD_ERROR xthread_error_quark ()
XPL_AVAILABLE_IN_ALL
xquark xthread_error_quark (void);

typedef enum
{
  G_THREAD_ERROR_AGAIN /* Resource temporarily unavailable */
} GThreadError;

typedef xpointer_t (*GThreadFunc) (xpointer_t data);

typedef struct _GThread         xthread_t;

typedef union  _GMutex          xmutex_t;
typedef struct _GRecMutex       GRecMutex;
typedef struct _GRWLock         GRWLock;
typedef struct _GCond           xcond_t;
typedef struct _GPrivate        GPrivate;
typedef struct _GOnce           GOnce;

union _GMutex
{
  /*< private >*/
  xpointer_t p;
  xuint_t i[2];
};

struct _GRWLock
{
  /*< private >*/
  xpointer_t p;
  xuint_t i[2];
};

struct _GCond
{
  /*< private >*/
  xpointer_t p;
  xuint_t i[2];
};

struct _GRecMutex
{
  /*< private >*/
  xpointer_t p;
  xuint_t i[2];
};

#define G_PRIVATE_INIT(notify) { NULL, (notify), { NULL, NULL } }
struct _GPrivate
{
  /*< private >*/
  xpointer_t       p;
  xdestroy_notify_t notify;
  xpointer_t future[2];
};

typedef enum
{
  G_ONCE_STATUS_NOTCALLED,
  G_ONCE_STATUS_PROGRESS,
  G_ONCE_STATUS_READY
} GOnceStatus;

#define G_ONCE_INIT { G_ONCE_STATUS_NOTCALLED, NULL }
struct _GOnce
{
  volatile GOnceStatus status;
  volatile xpointer_t retval;
};

#define G_LOCK_NAME(name)             g__ ## name ## _lock
#define G_LOCK_DEFINE_STATIC(name)    static G_LOCK_DEFINE (name)
#define G_LOCK_DEFINE(name)           xmutex_t G_LOCK_NAME (name)
#define G_LOCK_EXTERN(name)           extern xmutex_t G_LOCK_NAME (name)

#ifdef G_DEBUG_LOCKS
#  define G_LOCK(name)                G_STMT_START{             \
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,                   \
             "file %s: line %d (%s): locking: %s ",             \
             __FILE__,        __LINE__, G_STRFUNC,              \
             #name);                                            \
      g_mutex_lock (&G_LOCK_NAME (name));                       \
   }G_STMT_END
#  define G_UNLOCK(name)              G_STMT_START{             \
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,                   \
             "file %s: line %d (%s): unlocking: %s ",           \
             __FILE__,        __LINE__, G_STRFUNC,              \
             #name);                                            \
     g_mutex_unlock (&G_LOCK_NAME (name));                      \
   }G_STMT_END
#  define G_TRYLOCK(name)                                       \
      (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,                  \
             "file %s: line %d (%s): try locking: %s ",         \
             __FILE__,        __LINE__, G_STRFUNC,              \
             #name), g_mutex_trylock (&G_LOCK_NAME (name)))
#else  /* !G_DEBUG_LOCKS */
#  define G_LOCK(name) g_mutex_lock       (&G_LOCK_NAME (name))
#  define G_UNLOCK(name) g_mutex_unlock   (&G_LOCK_NAME (name))
#  define G_TRYLOCK(name) g_mutex_trylock (&G_LOCK_NAME (name))
#endif /* !G_DEBUG_LOCKS */

XPL_AVAILABLE_IN_2_32
xthread_t *       xthread_ref                    (xthread_t        *thread);
XPL_AVAILABLE_IN_2_32
void            xthread_unref                  (xthread_t        *thread);
XPL_AVAILABLE_IN_2_32
xthread_t *       xthread_new                    (const xchar_t    *name,
                                                 GThreadFunc     func,
                                                 xpointer_t        data);
XPL_AVAILABLE_IN_2_32
xthread_t *       xthread_try_new                (const xchar_t    *name,
                                                 GThreadFunc     func,
                                                 xpointer_t        data,
                                                 xerror_t        **error);
XPL_AVAILABLE_IN_ALL
xthread_t *       xthread_self                   (void);
XPL_AVAILABLE_IN_ALL
void            xthread_exit                   (xpointer_t        retval);
XPL_AVAILABLE_IN_ALL
xpointer_t        xthread_join                   (xthread_t        *thread);
XPL_AVAILABLE_IN_ALL
void            xthread_yield                  (void);


XPL_AVAILABLE_IN_2_32
void            g_mutex_init                    (xmutex_t         *mutex);
XPL_AVAILABLE_IN_2_32
void            g_mutex_clear                   (xmutex_t         *mutex);
XPL_AVAILABLE_IN_ALL
void            g_mutex_lock                    (xmutex_t         *mutex);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_mutex_trylock                 (xmutex_t         *mutex);
XPL_AVAILABLE_IN_ALL
void            g_mutex_unlock                  (xmutex_t         *mutex);

XPL_AVAILABLE_IN_2_32
void            g_rw_lock_init                  (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
void            g_rw_lock_clear                 (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
void            g_rw_lock_writer_lock           (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
xboolean_t        g_rw_lock_writer_trylock        (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
void            g_rw_lock_writer_unlock         (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
void            g_rw_lock_reader_lock           (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
xboolean_t        g_rw_lock_reader_trylock        (GRWLock        *rw_lock);
XPL_AVAILABLE_IN_2_32
void            g_rw_lock_reader_unlock         (GRWLock        *rw_lock);

XPL_AVAILABLE_IN_2_32
void            g_rec_mutex_init                (GRecMutex      *rec_mutex);
XPL_AVAILABLE_IN_2_32
void            g_rec_mutex_clear               (GRecMutex      *rec_mutex);
XPL_AVAILABLE_IN_2_32
void            g_rec_mutex_lock                (GRecMutex      *rec_mutex);
XPL_AVAILABLE_IN_2_32
xboolean_t        g_rec_mutex_trylock             (GRecMutex      *rec_mutex);
XPL_AVAILABLE_IN_2_32
void            g_rec_mutex_unlock              (GRecMutex      *rec_mutex);

XPL_AVAILABLE_IN_2_32
void            g_cond_init                     (xcond_t          *cond);
XPL_AVAILABLE_IN_2_32
void            g_cond_clear                    (xcond_t          *cond);
XPL_AVAILABLE_IN_ALL
void            g_cond_wait                     (xcond_t          *cond,
                                                 xmutex_t         *mutex);
XPL_AVAILABLE_IN_ALL
void            g_cond_signal                   (xcond_t          *cond);
XPL_AVAILABLE_IN_ALL
void            g_cond_broadcast                (xcond_t          *cond);
XPL_AVAILABLE_IN_2_32
xboolean_t        g_cond_wait_until               (xcond_t          *cond,
                                                 xmutex_t         *mutex,
                                                 gint64          end_time);

XPL_AVAILABLE_IN_ALL
xpointer_t        g_private_get                   (GPrivate       *key);
XPL_AVAILABLE_IN_ALL
void            g_private_set                   (GPrivate       *key,
                                                 xpointer_t        value);
XPL_AVAILABLE_IN_2_32
void            g_private_replace               (GPrivate       *key,
                                                 xpointer_t        value);

XPL_AVAILABLE_IN_ALL
xpointer_t        g_once_impl                     (GOnce          *once,
                                                 GThreadFunc     func,
                                                 xpointer_t        arg);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_once_init_enter               (volatile void  *location);
XPL_AVAILABLE_IN_ALL
void            g_once_init_leave               (volatile void  *location,
                                                 xsize_t           result);

/* Use C11-style atomic extensions to check the fast path for status=ready. If
 * they are not available, fall back to using a mutex and condition variable in
 * g_once_impl().
 *
 * On the C11-style codepath, only the load of once->status needs to be atomic,
 * as the writes to it and once->retval in g_once_impl() are related by a
 * happens-before relation. Release-acquire semantics are defined such that any
 * atomic/non-atomic write which happens-before a store/release is guaranteed to
 * be seen by the load/acquire of the same atomic variable. */
#if defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) && defined(__ATOMIC_SEQ_CST)
# define g_once(once, func, arg) \
  ((__atomic_load_n (&(once)->status, __ATOMIC_ACQUIRE) == G_ONCE_STATUS_READY) ? \
   (once)->retval : \
   g_once_impl ((once), (func), (arg)))
#else
# define g_once(once, func, arg) g_once_impl ((once), (func), (arg))
#endif

#ifdef __GNUC__
# define g_once_init_enter(location) \
  (G_GNUC_EXTENSION ({                                               \
    G_STATIC_ASSERT (sizeof *(location) == sizeof (xpointer_t));       \
    (void) (0 ? (xpointer_t) *(location) : NULL);                      \
    (!g_atomic_pointer_get (location) &&                             \
     g_once_init_enter (location));                                  \
  }))
# define g_once_init_leave(location, result) \
  (G_GNUC_EXTENSION ({                                               \
    G_STATIC_ASSERT (sizeof *(location) == sizeof (xpointer_t));       \
    0 ? (void) (*(location) = (result)) : (void) 0;                  \
    g_once_init_leave ((location), (xsize_t) (result));                \
  }))
#else
# define g_once_init_enter(location) \
  (g_once_init_enter((location)))
# define g_once_init_leave(location, result) \
  (g_once_init_leave((location), (xsize_t) (result)))
#endif

XPL_AVAILABLE_IN_2_36
xuint_t          g_get_num_processors (void);

/**
 * xmutex_locker_t:
 *
 * Opaque type. See g_mutex_locker_new() for details.
 * Since: 2.44
 */
typedef void xmutex_locker_t;

/**
 * g_mutex_locker_new:
 * @mutex: a mutex to lock
 *
 * Lock @mutex and return a new #xmutex_locker_t. Unlock with
 * g_mutex_locker_free(). Using g_mutex_unlock() on @mutex
 * while a #xmutex_locker_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a g_mutex_lock() call.
 *
 * This is intended to be used with x_autoptr().  Note that x_autoptr()
 * is only available when using GCC or clang, so the following example
 * will only work with those compilers:
 * |[
 * typedef struct
 * {
 *   ...
 *   xmutex_t mutex;
 *   ...
 * } xobject_t;
 *
 * static void
 * my_object_do_stuff (xobject_t *self)
 * {
 *   x_autoptr(xmutex_locker) locker = g_mutex_locker_new (&self->mutex);
 *
 *   // Code with mutex locked here
 *
 *   if (cond)
 *     // No need to unlock
 *     return;
 *
 *   // Optionally early unlock
 *   g_clear_pointer (&locker, g_mutex_locker_free);
 *
 *   // Code with mutex unlocked here
 * }
 * ]|
 *
 * Returns: a #xmutex_locker_t
 * Since: 2.44
 */
XPL_AVAILABLE_STATIC_INLINE_IN_2_44
static inline xmutex_locker_t *
g_mutex_locker_new (xmutex_t *mutex)
{
  g_mutex_lock (mutex);
  return (xmutex_locker_t *) mutex;
}

/**
 * g_mutex_locker_free:
 * @locker: a xmutex_locker_t
 *
 * Unlock @locker's mutex. See g_mutex_locker_new() for details.
 *
 * No memory is freed, it is equivalent to a g_mutex_unlock() call.
 *
 * Since: 2.44
 */
XPL_AVAILABLE_STATIC_INLINE_IN_2_44
static inline void
g_mutex_locker_free (xmutex_locker_t *locker)
{
  g_mutex_unlock ((xmutex_t *) locker);
}

/**
 * xrec_mutex_locker_t:
 *
 * Opaque type. See g_rec_mutex_locker_new() for details.
 * Since: 2.60
 */
typedef void xrec_mutex_locker_t;

/**
 * g_rec_mutex_locker_new:
 * @rec_mutex: a recursive mutex to lock
 *
 * Lock @rec_mutex and return a new #xrec_mutex_locker_t. Unlock with
 * g_rec_mutex_locker_free(). Using g_rec_mutex_unlock() on @rec_mutex
 * while a #xrec_mutex_locker_t exists can lead to undefined behaviour.
 *
 * No allocation is performed, it is equivalent to a g_rec_mutex_lock() call.
 *
 * This is intended to be used with x_autoptr().  Note that x_autoptr()
 * is only available when using GCC or clang, so the following example
 * will only work with those compilers:
 * |[
 * typedef struct
 * {
 *   ...
 *   GRecMutex rec_mutex;
 *   ...
 * } xobject_t;
 *
 * static void
 * my_object_do_stuff (xobject_t *self)
 * {
 *   x_autoptr(xrec_mutex_locker) locker = g_rec_mutex_locker_new (&self->rec_mutex);
 *
 *   // Code with rec_mutex locked here
 *
 *   if (cond)
 *     // No need to unlock
 *     return;
 *
 *   // Optionally early unlock
 *   g_clear_pointer (&locker, g_rec_mutex_locker_free);
 *
 *   // Code with rec_mutex unlocked here
 * }
 * ]|
 *
 * Returns: a #xrec_mutex_locker_t
 * Since: 2.60
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_60
static inline xrec_mutex_locker_t *
g_rec_mutex_locker_new (GRecMutex *rec_mutex)
{
  g_rec_mutex_lock (rec_mutex);
  return (xrec_mutex_locker_t *) rec_mutex;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * g_rec_mutex_locker_free:
 * @locker: a xrec_mutex_locker_t
 *
 * Unlock @locker's recursive mutex. See g_rec_mutex_locker_new() for details.
 *
 * No memory is freed, it is equivalent to a g_rec_mutex_unlock() call.
 *
 * Since: 2.60
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_60
static inline void
g_rec_mutex_locker_free (xrec_mutex_locker_t *locker)
{
  g_rec_mutex_unlock ((GRecMutex *) locker);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xrwlock_writer_locker_t:
 *
 * Opaque type. See g_rw_lock_writer_locker_new() for details.
 * Since: 2.62
 */
typedef void xrwlock_writer_locker_t;

/**
 * g_rw_lock_writer_locker_new:
 * @rw_lock: a #GRWLock
 *
 * Obtain a write lock on @rw_lock and return a new #xrwlock_writer_locker_t.
 * Unlock with g_rw_lock_writer_locker_free(). Using g_rw_lock_writer_unlock()
 * on @rw_lock while a #xrwlock_writer_locker_t exists can lead to undefined
 * behaviour.
 *
 * No allocation is performed, it is equivalent to a g_rw_lock_writer_lock() call.
 *
 * This is intended to be used with x_autoptr().  Note that x_autoptr()
 * is only available when using GCC or clang, so the following example
 * will only work with those compilers:
 * |[
 * typedef struct
 * {
 *   ...
 *   GRWLock rw_lock;
 *   xptr_array_t *array;
 *   ...
 * } xobject_t;
 *
 * static xchar_t *
 * my_object_get_data (xobject_t *self, xuint_t index)
 * {
 *   x_autoptr(xrwlock_reader_locker) locker = g_rw_lock_reader_locker_new (&self->rw_lock);
 *
 *   // Code with a read lock obtained on rw_lock here
 *
 *   if (self->array == NULL)
 *     // No need to unlock
 *     return NULL;
 *
 *   if (index < self->array->len)
 *     // No need to unlock
 *     return xptr_array_index (self->array, index);
 *
 *   // Optionally early unlock
 *   g_clear_pointer (&locker, g_rw_lock_reader_locker_free);
 *
 *   // Code with rw_lock unlocked here
 *   return NULL;
 * }
 *
 * static void
 * my_object_set_data (xobject_t *self, xuint_t index, xpointer_t data)
 * {
 *   x_autoptr(xrwlock_writer_locker) locker = g_rw_lock_writer_locker_new (&self->rw_lock);
 *
 *   // Code with a write lock obtained on rw_lock here
 *
 *   if (self->array == NULL)
 *     self->array = xptr_array_new ();
 *
 *   if (cond)
 *     // No need to unlock
 *     return;
 *
 *   if (index >= self->array->len)
 *     xptr_array_set_size (self->array, index+1);
 *   xptr_array_index (self->array, index) = data;
 *
 *   // Optionally early unlock
 *   g_clear_pointer (&locker, g_rw_lock_writer_locker_free);
 *
 *   // Code with rw_lock unlocked here
 * }
 * ]|
 *
 * Returns: a #xrwlock_writer_locker_t
 * Since: 2.62
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_62
static inline xrwlock_writer_locker_t *
g_rw_lock_writer_locker_new (GRWLock *rw_lock)
{
  g_rw_lock_writer_lock (rw_lock);
  return (xrwlock_writer_locker_t *) rw_lock;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * g_rw_lock_writer_locker_free:
 * @locker: a xrwlock_writer_locker_t
 *
 * Release a write lock on @locker's read-write lock. See
 * g_rw_lock_writer_locker_new() for details.
 *
 * No memory is freed, it is equivalent to a g_rw_lock_writer_unlock() call.
 *
 * Since: 2.62
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_62
static inline void
g_rw_lock_writer_locker_free (xrwlock_writer_locker_t *locker)
{
  g_rw_lock_writer_unlock ((GRWLock *) locker);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xrwlock_reader_locker_t:
 *
 * Opaque type. See g_rw_lock_reader_locker_new() for details.
 * Since: 2.62
 */
typedef void xrwlock_reader_locker_t;

/**
 * g_rw_lock_reader_locker_new:
 * @rw_lock: a #GRWLock
 *
 * Obtain a read lock on @rw_lock and return a new #xrwlock_reader_locker_t.
 * Unlock with g_rw_lock_reader_locker_free(). Using g_rw_lock_reader_unlock()
 * on @rw_lock while a #xrwlock_reader_locker_t exists can lead to undefined
 * behaviour.
 *
 * No allocation is performed, it is equivalent to a g_rw_lock_reader_lock() call.
 *
 * This is intended to be used with x_autoptr(). For a code sample, see
 * g_rw_lock_writer_locker_new().
 *
 * Returns: a #xrwlock_reader_locker_t
 * Since: 2.62
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_62
static inline xrwlock_reader_locker_t *
g_rw_lock_reader_locker_new (GRWLock *rw_lock)
{
  g_rw_lock_reader_lock (rw_lock);
  return (xrwlock_reader_locker_t *) rw_lock;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * g_rw_lock_reader_locker_free:
 * @locker: a xrwlock_reader_locker_t
 *
 * Release a read lock on @locker's read-write lock. See
 * g_rw_lock_reader_locker_new() for details.
 *
 * No memory is freed, it is equivalent to a g_rw_lock_reader_unlock() call.
 *
 * Since: 2.62
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_STATIC_INLINE_IN_2_62
static inline void
g_rw_lock_reader_locker_free (xrwlock_reader_locker_t *locker)
{
  g_rw_lock_reader_unlock ((GRWLock *) locker);
}
G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS

#endif /* __G_THREAD_H__ */
