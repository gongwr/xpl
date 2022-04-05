/*
 * Copyright © 2011 Ryan Lortie
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

#include "config.h"

#include "gatomic.h"

/**
 * SECTION:atomic_operations
 * @title: Atomic Operations
 * @short_description: basic atomic integer and pointer operations
 * @see_also: #GMutex
 *
 * The following is a collection of compiler macros to provide atomic
 * access to integer and pointer-sized values.
 *
 * The macros that have 'int' in the name will operate on pointers to
 * #xint_t and #xuint_t.  The macros with 'pointer' in the name will operate
 * on pointers to any pointer-sized value, including #xsize_t.  There is
 * no support for 64bit operations on platforms with 32bit pointers
 * because it is not generally possible to perform these operations
 * atomically.
 *
 * The get, set and exchange operations for integers and pointers
 * nominally operate on #xint_t and #xpointer_t, respectively.  Of the
 * arithmetic operations, the 'add' operation operates on (and returns)
 * signed integer values (#xint_t and #gssize) and the 'and', 'or', and
 * 'xor' operations operate on (and return) unsigned integer values
 * (#xuint_t and #xsize_t).
 *
 * All of the operations act as a full compiler and (where appropriate)
 * hardware memory barrier.  Acquire and release or producer and
 * consumer barrier semantics are not available through this API.
 *
 * It is very important that all accesses to a particular integer or
 * pointer be performed using only this API and that different sizes of
 * operation are not mixed or used on overlapping memory regions.  Never
 * read or assign directly from or to a value -- always use this API.
 *
 * For simple reference counting purposes you should use
 * g_atomic_int_inc() and g_atomic_int_dec_and_test().  Other uses that
 * fall outside of simple reference counting patterns are prone to
 * subtle bugs and occasionally undefined behaviour.  It is also worth
 * noting that since all of these operations require global
 * synchronisation of the entire machine, they can be quite slow.  In
 * the case of performing multiple atomic operations it can often be
 * faster to simply acquire a mutex lock around the critical area,
 * perform the operations normally and then release the lock.
 **/

/**
 * G_ATOMIC_LOCK_FREE:
 *
 * This macro is defined if the atomic operations of GLib are
 * implemented using real hardware atomic operations.  This means that
 * the GLib atomic API can be used between processes and safely mixed
 * with other (hardware) atomic APIs.
 *
 * If this macro is not defined, the atomic operations may be
 * emulated using a mutex.  In that case, the GLib atomic operations are
 * only atomic relative to themselves and within a single process.
 **/

/* NOTE CAREFULLY:
 *
 * This file is the lowest-level part of GLib.
 *
 * Other lowlevel parts of GLib (threads, slice allocator, g_malloc,
 * messages, etc) call into these functions and macros to get work done.
 *
 * As such, these functions can not call back into any part of GLib
 * without risking recursion.
 */

#ifdef G_ATOMIC_LOCK_FREE

/* if G_ATOMIC_LOCK_FREE was defined by `meson configure` then we MUST
 * implement the atomic operations in a lock-free manner.
 */

#if defined (__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

/**
 * g_atomic_int_get:
 * @atomic: a pointer to a #xint_t or #xuint_t
 *
 * Gets the current value of @atomic.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (before the get).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of the integer
 *
 * Since: 2.4
 **/
xint_t
(g_atomic_int_get) (const volatile xint_t *atomic)
{
  return g_atomic_int_get (atomic);
}

/**
 * g_atomic_int_set:
 * @atomic: a pointer to a #xint_t or #xuint_t
 * @newval: a new value to store
 *
 * Sets the value of @atomic to @newval.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (after the set).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Since: 2.4
 */
void
(g_atomic_int_set) (volatile xint_t *atomic,
                    xint_t           newval)
{
  g_atomic_int_set (atomic, newval);
}

/**
 * g_atomic_int_inc:
 * @atomic: a pointer to a #xint_t or #xuint_t
 *
 * Increments the value of @atomic by 1.
 *
 * Think of this operation as an atomic version of `{ *atomic += 1; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Since: 2.4
 **/
void
(g_atomic_int_inc) (volatile xint_t *atomic)
{
  g_atomic_int_inc (atomic);
}

/**
 * g_atomic_int_dec_and_test:
 * @atomic: a pointer to a #xint_t or #xuint_t
 *
 * Decrements the value of @atomic by 1.
 *
 * Think of this operation as an atomic version of
 * `{ *atomic -= 1; return (*atomic == 0); }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: %TRUE if the resultant value is zero
 *
 * Since: 2.4
 **/
xboolean_t
(g_atomic_int_dec_and_test) (volatile xint_t *atomic)
{
  return g_atomic_int_dec_and_test (atomic);
}

/**
 * g_atomic_int_compare_and_exchange:
 * @atomic: a pointer to a #xint_t or #xuint_t
 * @oldval: the value to compare with
 * @newval: the value to conditionally replace with
 *
 * Compares @atomic to @oldval and, if equal, sets it to @newval.
 * If @atomic was not equal to @oldval then no change occurs.
 *
 * This compare and exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ if (*atomic == oldval) { *atomic = newval; return TRUE; } else return FALSE; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: %TRUE if the exchange took place
 *
 * Since: 2.4
 **/
xboolean_t
(g_atomic_int_compare_and_exchange) (volatile xint_t *atomic,
                                     xint_t           oldval,
                                     xint_t           newval)
{
  return g_atomic_int_compare_and_exchange (atomic, oldval, newval);
}

/**
 * g_atomic_int_add:
 * @atomic: a pointer to a #xint_t or #xuint_t
 * @val: the value to add
 *
 * Atomically adds @val to the value of @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic += val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * Before version 2.30, this function did not return a value
 * (but g_atomic_int_exchange_and_add() did, and had the same meaning).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the add, signed
 *
 * Since: 2.4
 **/
xint_t
(g_atomic_int_add) (volatile xint_t *atomic,
                    xint_t           val)
{
  return g_atomic_int_add (atomic, val);
}

/**
 * g_atomic_int_and:
 * @atomic: a pointer to a #xint_t or #xuint_t
 * @val: the value to 'and'
 *
 * Performs an atomic bitwise 'and' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic &= val; return tmp; }`.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
xuint_t
(g_atomic_int_and) (volatile xuint_t *atomic,
                    xuint_t           val)
{
  return g_atomic_int_and (atomic, val);
}

/**
 * g_atomic_int_or:
 * @atomic: a pointer to a #xint_t or #xuint_t
 * @val: the value to 'or'
 *
 * Performs an atomic bitwise 'or' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic |= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
xuint_t
(g_atomic_int_or) (volatile xuint_t *atomic,
                   xuint_t           val)
{
  return g_atomic_int_or (atomic, val);
}

/**
 * g_atomic_int_xor:
 * @atomic: a pointer to a #xint_t or #xuint_t
 * @val: the value to 'xor'
 *
 * Performs an atomic bitwise 'xor' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic ^= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
xuint_t
(g_atomic_int_xor) (volatile xuint_t *atomic,
                    xuint_t           val)
{
  return g_atomic_int_xor (atomic, val);
}


/**
 * g_atomic_pointer_get:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 *
 * Gets the current value of @atomic.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (before the get).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of the pointer
 *
 * Since: 2.4
 **/
xpointer_t
(g_atomic_pointer_get) (const volatile void *atomic)
{
  return g_atomic_pointer_get ((xpointer_t *) atomic);
}

/**
 * g_atomic_pointer_set:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 * @newval: a new value to store
 *
 * Sets the value of @atomic to @newval.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (after the set).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Since: 2.4
 **/
void
(g_atomic_pointer_set) (volatile void *atomic,
                        xpointer_t       newval)
{
  g_atomic_pointer_set ((xpointer_t *) atomic, newval);
}

/**
 * g_atomic_pointer_compare_and_exchange:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 * @oldval: the value to compare with
 * @newval: the value to conditionally replace with
 *
 * Compares @atomic to @oldval and, if equal, sets it to @newval.
 * If @atomic was not equal to @oldval then no change occurs.
 *
 * This compare and exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ if (*atomic == oldval) { *atomic = newval; return TRUE; } else return FALSE; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: %TRUE if the exchange took place
 *
 * Since: 2.4
 **/
xboolean_t
(g_atomic_pointer_compare_and_exchange) (volatile void *atomic,
                                         xpointer_t       oldval,
                                         xpointer_t       newval)
{
  return g_atomic_pointer_compare_and_exchange ((xpointer_t *) atomic,
                                                oldval, newval);
}

/**
 * g_atomic_pointer_add:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 * @val: the value to add
 *
 * Atomically adds @val to the value of @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic += val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the add, signed
 *
 * Since: 2.30
 **/
gssize
(g_atomic_pointer_add) (volatile void *atomic,
                        gssize         val)
{
  return g_atomic_pointer_add ((xpointer_t *) atomic, val);
}

/**
 * g_atomic_pointer_and:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 * @val: the value to 'and'
 *
 * Performs an atomic bitwise 'and' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic &= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
xsize_t
(g_atomic_pointer_and) (volatile void *atomic,
                        xsize_t          val)
{
  return g_atomic_pointer_and ((xpointer_t *) atomic, val);
}

/**
 * g_atomic_pointer_or:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 * @val: the value to 'or'
 *
 * Performs an atomic bitwise 'or' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic |= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
xsize_t
(g_atomic_pointer_or) (volatile void *atomic,
                       xsize_t          val)
{
  return g_atomic_pointer_or ((xpointer_t *) atomic, val);
}

/**
 * g_atomic_pointer_xor:
 * @atomic: (not nullable): a pointer to a #xpointer_t-sized value
 * @val: the value to 'xor'
 *
 * Performs an atomic bitwise 'xor' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic ^= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
xsize_t
(g_atomic_pointer_xor) (volatile void *atomic,
                        xsize_t          val)
{
  return g_atomic_pointer_xor ((xpointer_t *) atomic, val);
}

#elif defined (XPLATFORM_WIN32)

#include <windows.h>
#if !defined(_M_AMD64) && !defined (_M_IA64) && !defined(_M_X64) && !(defined _MSC_VER && _MSC_VER <= 1200)
#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#endif

#if !defined (_MSC_VER) || _MSC_VER <= 1200
#include "gmessages.h"
/* Inlined versions for older compiler */
static LONG
_gInterlockedAnd (volatile xuint_t *atomic,
                  xuint_t           val)
{
  LONG i, j;

  j = *atomic;
  do {
    i = j;
    j = InterlockedCompareExchange(atomic, i & val, i);
  } while (i != j);

  return j;
}
#define InterlockedAnd(a,b) _gInterlockedAnd(a,b)
static LONG
_gInterlockedOr (volatile xuint_t *atomic,
                 xuint_t           val)
{
  LONG i, j;

  j = *atomic;
  do {
    i = j;
    j = InterlockedCompareExchange(atomic, i | val, i);
  } while (i != j);

  return j;
}
#define InterlockedOr(a,b) _gInterlockedOr(a,b)
static LONG
_gInterlockedXor (volatile xuint_t *atomic,
                  xuint_t           val)
{
  LONG i, j;

  j = *atomic;
  do {
    i = j;
    j = InterlockedCompareExchange(atomic, i ^ val, i);
  } while (i != j);

  return j;
}
#define InterlockedXor(a,b) _gInterlockedXor(a,b)
#endif

/*
 * http://msdn.microsoft.com/en-us/library/ms684122(v=vs.85).aspx
 */
xint_t
(g_atomic_int_get) (const volatile xint_t *atomic)
{
  MemoryBarrier ();
  return *atomic;
}

void
(g_atomic_int_set) (volatile xint_t *atomic,
                    xint_t           newval)
{
  *atomic = newval;
  MemoryBarrier ();
}

void
(g_atomic_int_inc) (volatile xint_t *atomic)
{
  InterlockedIncrement (atomic);
}

xboolean_t
(g_atomic_int_dec_and_test) (volatile xint_t *atomic)
{
  return InterlockedDecrement (atomic) == 0;
}

xboolean_t
(g_atomic_int_compare_and_exchange) (volatile xint_t *atomic,
                                     xint_t           oldval,
                                     xint_t           newval)
{
  return InterlockedCompareExchange (atomic, newval, oldval) == oldval;
}

xint_t
(g_atomic_int_add) (volatile xint_t *atomic,
                    xint_t           val)
{
  return InterlockedExchangeAdd (atomic, val);
}

xuint_t
(g_atomic_int_and) (volatile xuint_t *atomic,
                    xuint_t           val)
{
  return InterlockedAnd (atomic, val);
}

xuint_t
(g_atomic_int_or) (volatile xuint_t *atomic,
                   xuint_t           val)
{
  return InterlockedOr (atomic, val);
}

xuint_t
(g_atomic_int_xor) (volatile xuint_t *atomic,
                    xuint_t           val)
{
  return InterlockedXor (atomic, val);
}


xpointer_t
(g_atomic_pointer_get) (const volatile void *atomic)
{
  const xpointer_t *ptr = atomic;

  MemoryBarrier ();
  return *ptr;
}

void
(g_atomic_pointer_set) (volatile void *atomic,
                        xpointer_t       newval)
{
  xpointer_t *ptr = atomic;

  *ptr = newval;
  MemoryBarrier ();
}

xboolean_t
(g_atomic_pointer_compare_and_exchange) (volatile void *atomic,
                                         xpointer_t       oldval,
                                         xpointer_t       newval)
{
  return InterlockedCompareExchangePointer (atomic, newval, oldval) == oldval;
}

gssize
(g_atomic_pointer_add) (volatile void *atomic,
                        gssize         val)
{
#if XPL_SIZEOF_VOID_P == 8
  return InterlockedExchangeAdd64 (atomic, val);
#else
  return InterlockedExchangeAdd (atomic, val);
#endif
}

xsize_t
(g_atomic_pointer_and) (volatile void *atomic,
                        xsize_t          val)
{
#if XPL_SIZEOF_VOID_P == 8
  return InterlockedAnd64 (atomic, val);
#else
  return InterlockedAnd (atomic, val);
#endif
}

xsize_t
(g_atomic_pointer_or) (volatile void *atomic,
                       xsize_t          val)
{
#if XPL_SIZEOF_VOID_P == 8
  return InterlockedOr64 (atomic, val);
#else
  return InterlockedOr (atomic, val);
#endif
}

xsize_t
(g_atomic_pointer_xor) (volatile void *atomic,
                        xsize_t          val)
{
#if XPL_SIZEOF_VOID_P == 8
  return InterlockedXor64 (atomic, val);
#else
  return InterlockedXor (atomic, val);
#endif
}
#else

/* This error occurs when `meson configure` decided that we should be capable
 * of lock-free atomics but we find at compile-time that we are not.
 */
#error G_ATOMIC_LOCK_FREE defined, but incapable of lock-free atomics.

#endif /* defined (__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) */

#else /* G_ATOMIC_LOCK_FREE */

/* We are not permitted to call into any GLib functions from here, so we
 * can not use GMutex.
 *
 * Fortunately, we already take care of the Windows case above, and all
 * non-Windows platforms on which glib runs have pthreads.  Use those.
 */
#include <pthread.h>

static pthread_mutex_t g_atomic_lock = PTHREAD_MUTEX_INITIALIZER;

xint_t
(g_atomic_int_get) (const volatile xint_t *atomic)
{
  xint_t value;

  pthread_mutex_lock (&g_atomic_lock);
  value = *atomic;
  pthread_mutex_unlock (&g_atomic_lock);

  return value;
}

void
(g_atomic_int_set) (volatile xint_t *atomic,
                    xint_t           value)
{
  pthread_mutex_lock (&g_atomic_lock);
  *atomic = value;
  pthread_mutex_unlock (&g_atomic_lock);
}

void
(g_atomic_int_inc) (volatile xint_t *atomic)
{
  pthread_mutex_lock (&g_atomic_lock);
  (*atomic)++;
  pthread_mutex_unlock (&g_atomic_lock);
}

xboolean_t
(g_atomic_int_dec_and_test) (volatile xint_t *atomic)
{
  xboolean_t is_zero;

  pthread_mutex_lock (&g_atomic_lock);
  is_zero = --(*atomic) == 0;
  pthread_mutex_unlock (&g_atomic_lock);

  return is_zero;
}

xboolean_t
(g_atomic_int_compare_and_exchange) (volatile xint_t *atomic,
                                     xint_t           oldval,
                                     xint_t           newval)
{
  xboolean_t success;

  pthread_mutex_lock (&g_atomic_lock);

  if ((success = (*atomic == oldval)))
    *atomic = newval;

  pthread_mutex_unlock (&g_atomic_lock);

  return success;
}

xint_t
(g_atomic_int_add) (volatile xint_t *atomic,
                    xint_t           val)
{
  xint_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *atomic;
  *atomic = oldval + val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

xuint_t
(g_atomic_int_and) (volatile xuint_t *atomic,
                    xuint_t           val)
{
  xuint_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *atomic;
  *atomic = oldval & val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

xuint_t
(g_atomic_int_or) (volatile xuint_t *atomic,
                   xuint_t           val)
{
  xuint_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *atomic;
  *atomic = oldval | val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

xuint_t
(g_atomic_int_xor) (volatile xuint_t *atomic,
                    xuint_t           val)
{
  xuint_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *atomic;
  *atomic = oldval ^ val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}


xpointer_t
(g_atomic_pointer_get) (const volatile void *atomic)
{
  const xpointer_t *ptr = atomic;
  xpointer_t value;

  pthread_mutex_lock (&g_atomic_lock);
  value = *ptr;
  pthread_mutex_unlock (&g_atomic_lock);

  return value;
}

void
(g_atomic_pointer_set) (volatile void *atomic,
                        xpointer_t       newval)
{
  xpointer_t *ptr = atomic;

  pthread_mutex_lock (&g_atomic_lock);
  *ptr = newval;
  pthread_mutex_unlock (&g_atomic_lock);
}

xboolean_t
(g_atomic_pointer_compare_and_exchange) (volatile void *atomic,
                                         xpointer_t       oldval,
                                         xpointer_t       newval)
{
  xpointer_t *ptr = atomic;
  xboolean_t success;

  pthread_mutex_lock (&g_atomic_lock);

  if ((success = (*ptr == oldval)))
    *ptr = newval;

  pthread_mutex_unlock (&g_atomic_lock);

  return success;
}

gssize
(g_atomic_pointer_add) (volatile void *atomic,
                        gssize         val)
{
  gssize *ptr = atomic;
  gssize oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *ptr;
  *ptr = oldval + val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

xsize_t
(g_atomic_pointer_and) (volatile void *atomic,
                        xsize_t          val)
{
  xsize_t *ptr = atomic;
  xsize_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *ptr;
  *ptr = oldval & val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

xsize_t
(g_atomic_pointer_or) (volatile void *atomic,
                       xsize_t          val)
{
  xsize_t *ptr = atomic;
  xsize_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *ptr;
  *ptr = oldval | val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

xsize_t
(g_atomic_pointer_xor) (volatile void *atomic,
                        xsize_t          val)
{
  xsize_t *ptr = atomic;
  xsize_t oldval;

  pthread_mutex_lock (&g_atomic_lock);
  oldval = *ptr;
  *ptr = oldval ^ val;
  pthread_mutex_unlock (&g_atomic_lock);

  return oldval;
}

#endif

/**
 * g_atomic_int_exchange_and_add:
 * @atomic: a pointer to a #xint_t
 * @val: the value to add
 *
 * This function existed before g_atomic_int_add() returned the prior
 * value of the integer (which it now does).  It is retained only for
 * compatibility reasons.  Don't use this function in new code.
 *
 * Returns: the value of @atomic before the add, signed
 * Since: 2.4
 * Deprecated: 2.30: Use g_atomic_int_add() instead.
 **/
xint_t
g_atomic_int_exchange_and_add (volatile xint_t *atomic,
                               xint_t           val)
{
  return (g_atomic_int_add) ((xint_t *) atomic, val);
}
