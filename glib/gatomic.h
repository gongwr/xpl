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

#ifndef __G_ATOMIC_H__
#define __G_ATOMIC_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>
#include <glib/glib-typeof.h>

G_BEGIN_DECLS

XPL_AVAILABLE_IN_ALL
xint_t                    g_atomic_int_get                      (const volatile xint_t *atomic);
XPL_AVAILABLE_IN_ALL
void                    g_atomic_int_set                      (volatile xint_t  *atomic,
                                                               xint_t            newval);
XPL_AVAILABLE_IN_ALL
void                    g_atomic_int_inc                      (volatile xint_t  *atomic);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_atomic_int_dec_and_test             (volatile xint_t  *atomic);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_atomic_int_compare_and_exchange     (volatile xint_t  *atomic,
                                                               xint_t            oldval,
                                                               xint_t            newval);
XPL_AVAILABLE_IN_ALL
xint_t                    g_atomic_int_add                      (volatile xint_t  *atomic,
                                                               xint_t            val);
XPL_AVAILABLE_IN_2_30
xuint_t                   g_atomic_int_and                      (volatile xuint_t *atomic,
                                                               xuint_t           val);
XPL_AVAILABLE_IN_2_30
xuint_t                   g_atomic_int_or                       (volatile xuint_t *atomic,
                                                               xuint_t           val);
XPL_AVAILABLE_IN_ALL
xuint_t                   g_atomic_int_xor                      (volatile xuint_t *atomic,
                                                               xuint_t           val);

XPL_AVAILABLE_IN_ALL
xpointer_t                g_atomic_pointer_get                  (const volatile void *atomic);
XPL_AVAILABLE_IN_ALL
void                    g_atomic_pointer_set                  (volatile void  *atomic,
                                                               xpointer_t        newval);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_atomic_pointer_compare_and_exchange (volatile void  *atomic,
                                                               xpointer_t        oldval,
                                                               xpointer_t        newval);
XPL_AVAILABLE_IN_ALL
gssize                  g_atomic_pointer_add                  (volatile void  *atomic,
                                                               gssize          val);
XPL_AVAILABLE_IN_2_30
xsize_t                   g_atomic_pointer_and                  (volatile void  *atomic,
                                                               xsize_t           val);
XPL_AVAILABLE_IN_2_30
xsize_t                   g_atomic_pointer_or                   (volatile void  *atomic,
                                                               xsize_t           val);
XPL_AVAILABLE_IN_ALL
xsize_t                   g_atomic_pointer_xor                  (volatile void  *atomic,
                                                               xsize_t           val);

XPL_DEPRECATED_IN_2_30_FOR(g_atomic_int_add)
xint_t                    g_atomic_int_exchange_and_add         (volatile xint_t  *atomic,
                                                               xint_t            val);

G_END_DECLS

#if defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

/* We prefer the new C11-style atomic extension of GCC if available */
#if defined(__ATOMIC_SEQ_CST)

#define g_atomic_int_get(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    xint_t gaig_temp;                                                          \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
    __atomic_load ((xint_t *)(atomic), &gaig_temp, __ATOMIC_SEQ_CST);          \
    (xint_t) gaig_temp;                                                        \
  }))
#define g_atomic_int_set(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    xint_t gais_temp = (xint_t) (newval);                                        \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                   \
    __atomic_store ((xint_t *)(atomic), &gais_temp, __ATOMIC_SEQ_CST);         \
  }))

#if defined(glib_typeof)
#define g_atomic_pointer_get(atomic)                                       \
  (G_GNUC_EXTENSION ({                                                     \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));               \
    glib_typeof (*(atomic)) gapg_temp_newval;                              \
    glib_typeof ((atomic)) gapg_temp_atomic = (atomic);                    \
    __atomic_load (gapg_temp_atomic, &gapg_temp_newval, __ATOMIC_SEQ_CST); \
    gapg_temp_newval;                                                      \
  }))
#define g_atomic_pointer_set(atomic, newval)                                \
  (G_GNUC_EXTENSION ({                                                      \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                \
    glib_typeof ((atomic)) gaps_temp_atomic = (atomic);                     \
    glib_typeof (*(atomic)) gaps_temp_newval = (newval);                    \
    (void) (0 ? (xpointer_t) * (atomic) : NULL);                              \
    __atomic_store (gaps_temp_atomic, &gaps_temp_newval, __ATOMIC_SEQ_CST); \
  }))
#else /* if !(defined(glib_typeof) */
#define g_atomic_pointer_get(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    xpointer_t gapg_temp_newval;                                               \
    xpointer_t *gapg_temp_atomic = (xpointer_t *)(atomic);                       \
    __atomic_load (gapg_temp_atomic, &gapg_temp_newval, __ATOMIC_SEQ_CST);   \
    gapg_temp_newval;                                                        \
  }))
#define g_atomic_pointer_set(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    xpointer_t *gaps_temp_atomic = (xpointer_t *)(atomic);                       \
    xpointer_t gaps_temp_newval = (xpointer_t)(newval);                          \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    __atomic_store (gaps_temp_atomic, &gaps_temp_newval, __ATOMIC_SEQ_CST);  \
  }))
#endif /* if defined(glib_typeof) */

#define g_atomic_int_inc(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
    (void) __atomic_fetch_add ((atomic), 1, __ATOMIC_SEQ_CST);               \
  }))
#define g_atomic_int_dec_and_test(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
    __atomic_fetch_sub ((atomic), 1, __ATOMIC_SEQ_CST) == 1;                 \
  }))
#define g_atomic_int_compare_and_exchange(atomic, oldval, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    xint_t gaicae_oldval = (oldval);                                           \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (newval) ^ (oldval) : 1);                        \
    __atomic_compare_exchange_n ((atomic), (void *) (&(gaicae_oldval)), (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
  }))
#define g_atomic_int_add(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xint_t) __atomic_fetch_add ((atomic), (val), __ATOMIC_SEQ_CST);           \
  }))
#define g_atomic_int_and(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xuint_t) __atomic_fetch_and ((atomic), (val), __ATOMIC_SEQ_CST);          \
  }))
#define g_atomic_int_or(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xuint_t) __atomic_fetch_or ((atomic), (val), __ATOMIC_SEQ_CST);           \
  }))
#define g_atomic_int_xor(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xuint_t) __atomic_fetch_xor ((atomic), (val), __ATOMIC_SEQ_CST);          \
  }))

#if defined(glib_typeof) && defined(__cplusplus) && __cplusplus >= 201103L
/* This is typesafe because we check we can assign oldval to the type of
 * (*atomic). Unfortunately it can only be done in C++ because gcc/clang warn
 * when atomic is volatile and not oldval, or when atomic is xsize_t* and oldval
 * is NULL. Note that clang++ force us to be typesafe because it is an error if the 2nd
 * argument of __atomic_compare_exchange_n() has a different type than the
 * first.
 * https://gitlab.gnome.org/GNOME/glib/-/merge_requests/1919
 * https://gitlab.gnome.org/GNOME/glib/-/merge_requests/1715#note_1024120. */
#define g_atomic_pointer_compare_and_exchange(atomic, oldval, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof (oldval) == sizeof (xpointer_t));                  \
    glib_typeof (*(atomic)) gapcae_oldval = (oldval);                        \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    __atomic_compare_exchange_n ((atomic), &gapcae_oldval, (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
  }))
#else /* if !(defined(glib_typeof) && defined(__cplusplus) && __cplusplus >= 201103L) */
#define g_atomic_pointer_compare_and_exchange(atomic, oldval, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof (oldval) == sizeof (xpointer_t));                  \
    xpointer_t gapcae_oldval = (xpointer_t)(oldval);                             \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    __atomic_compare_exchange_n ((atomic), (void *) (&(gapcae_oldval)), (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
  }))
#endif /* defined(glib_typeof) */
#define g_atomic_pointer_add(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (gssize) __atomic_fetch_add ((atomic), (val), __ATOMIC_SEQ_CST);         \
  }))
#define g_atomic_pointer_and(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    xsize_t *gapa_atomic = (xsize_t *) (atomic);                                 \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xsize_t));                    \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (xsize_t) __atomic_fetch_and (gapa_atomic, (val), __ATOMIC_SEQ_CST);       \
  }))
#define g_atomic_pointer_or(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    xsize_t *gapo_atomic = (xsize_t *) (atomic);                                 \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xsize_t));                    \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (xsize_t) __atomic_fetch_or (gapo_atomic, (val), __ATOMIC_SEQ_CST);        \
  }))
#define g_atomic_pointer_xor(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    xsize_t *gapx_atomic = (xsize_t *) (atomic);                                 \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xsize_t));                    \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (xsize_t) __atomic_fetch_xor (gapx_atomic, (val), __ATOMIC_SEQ_CST);       \
  }))

#else /* defined(__ATOMIC_SEQ_CST) */

/* We want to achieve __ATOMIC_SEQ_CST semantics here. See
 * https://en.cppreference.com/w/c/atomic/memory_order#Constants. For load
 * operations, that means performing an *acquire*:
 * > A load operation with this memory order performs the acquire operation on
 * > the affected memory location: no reads or writes in the current thread can
 * > be reordered before this load. All writes in other threads that release
 * > the same atomic variable are visible in the current thread.
 *
 * “no reads or writes in the current thread can be reordered before this load”
 * is implemented using a compiler barrier (a no-op `__asm__` section) to
 * prevent instruction reordering. Writes in other threads are synchronised
 * using `__sync_synchronize()`. It’s unclear from the GCC documentation whether
 * `__sync_synchronize()` acts as a compiler barrier, hence our explicit use of
 * one.
 *
 * For store operations, `__ATOMIC_SEQ_CST` means performing a *release*:
 * > A store operation with this memory order performs the release operation:
 * > no reads or writes in the current thread can be reordered after this store.
 * > All writes in the current thread are visible in other threads that acquire
 * > the same atomic variable (see Release-Acquire ordering below) and writes
 * > that carry a dependency into the atomic variable become visible in other
 * > threads that consume the same atomic (see Release-Consume ordering below).
 *
 * “no reads or writes in the current thread can be reordered after this store”
 * is implemented using a compiler barrier to prevent instruction reordering.
 * “All writes in the current thread are visible in other threads” is implemented
 * using `__sync_synchronize()`; similarly for “writes that carry a dependency”.
 */
#define g_atomic_int_get(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    xint_t gaig_result;                                                        \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
    gaig_result = (xint_t) *(atomic);                                          \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
    gaig_result;                                                             \
  }))
#define g_atomic_int_set(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                   \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
    *(atomic) = (newval);                                                    \
  }))
#define g_atomic_pointer_get(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    xpointer_t gapg_result;                                                    \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    gapg_result = (xpointer_t) *(atomic);                                      \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
    gapg_result;                                                             \
  }))
#if defined(glib_typeof)
#define g_atomic_pointer_set(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
    *(atomic) = (glib_typeof (*(atomic))) (xsize_t) (newval);                  \
  }))
#else /* if !(defined(glib_typeof) */
#define g_atomic_pointer_set(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
    *(atomic) = (xpointer_t) (xsize_t) (newval);                                         \
  }))
#endif /* if defined(glib_typeof) */

#define g_atomic_int_inc(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
    (void) __sync_fetch_and_add ((atomic), 1);                               \
  }))
#define g_atomic_int_dec_and_test(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
    __sync_fetch_and_sub ((atomic), 1) == 1;                                 \
  }))
#define g_atomic_int_compare_and_exchange(atomic, oldval, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (newval) ^ (oldval) : 1);                        \
    __sync_bool_compare_and_swap ((atomic), (oldval), (newval)) ? TRUE : FALSE; \
  }))
#define g_atomic_int_add(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xint_t) __sync_fetch_and_add ((atomic), (val));                           \
  }))
#define g_atomic_int_and(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xuint_t) __sync_fetch_and_and ((atomic), (val));                          \
  }))
#define g_atomic_int_or(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xuint_t) __sync_fetch_and_or ((atomic), (val));                           \
  }))
#define g_atomic_int_xor(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint_t));                     \
    (void) (0 ? *(atomic) ^ (val) : 1);                                      \
    (xuint_t) __sync_fetch_and_xor ((atomic), (val));                          \
  }))

#define g_atomic_pointer_compare_and_exchange(atomic, oldval, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    __sync_bool_compare_and_swap ((atomic), (oldval), (newval)) ? TRUE : FALSE; \
  }))
#define g_atomic_pointer_add(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (gssize) __sync_fetch_and_add ((atomic), (val));                         \
  }))
#define g_atomic_pointer_and(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (xsize_t) __sync_fetch_and_and ((atomic), (val));                          \
  }))
#define g_atomic_pointer_or(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (xsize_t) __sync_fetch_and_or ((atomic), (val));                           \
  }))
#define g_atomic_pointer_xor(atomic, val) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (xpointer_t));                 \
    (void) (0 ? (xpointer_t) *(atomic) : NULL);                                \
    (void) (0 ? (val) ^ (val) : 1);                                          \
    (xsize_t) __sync_fetch_and_xor ((atomic), (val));                          \
  }))

#endif /* !defined(__ATOMIC_SEQ_CST) */

#else /* defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) */

#define g_atomic_int_get(atomic) \
  (g_atomic_int_get ((xint_t *) (atomic)))
#define g_atomic_int_set(atomic, newval) \
  (g_atomic_int_set ((xint_t *) (atomic), (xint_t) (newval)))
#define g_atomic_int_compare_and_exchange(atomic, oldval, newval) \
  (g_atomic_int_compare_and_exchange ((xint_t *) (atomic), (oldval), (newval)))
#define g_atomic_int_add(atomic, val) \
  (g_atomic_int_add ((xint_t *) (atomic), (val)))
#define g_atomic_int_and(atomic, val) \
  (g_atomic_int_and ((xuint_t *) (atomic), (val)))
#define g_atomic_int_or(atomic, val) \
  (g_atomic_int_or ((xuint_t *) (atomic), (val)))
#define g_atomic_int_xor(atomic, val) \
  (g_atomic_int_xor ((xuint_t *) (atomic), (val)))
#define g_atomic_int_inc(atomic) \
  (g_atomic_int_inc ((xint_t *) (atomic)))
#define g_atomic_int_dec_and_test(atomic) \
  (g_atomic_int_dec_and_test ((xint_t *) (atomic)))

#if defined(glib_typeof)
  /* The (void *) cast in the middle *looks* redundant, because
   * g_atomic_pointer_get returns void * already, but it's to silence
   * -Werror=bad-function-cast when we're doing something like:
   * guintptr a, b; ...; a = g_atomic_pointer_get (&b);
   * which would otherwise be assigning the void * result of
   * g_atomic_pointer_get directly to the pointer-sized but
   * non-pointer-typed result. */
#define g_atomic_pointer_get(atomic)                                       \
  (glib_typeof (*(atomic))) (void *) ((g_atomic_pointer_get) ((void *) atomic))
#else /* !(defined(glib_typeof) */
#define g_atomic_pointer_get(atomic) \
  (g_atomic_pointer_get (atomic))
#endif

#define g_atomic_pointer_set(atomic, newval) \
  (g_atomic_pointer_set ((atomic), (xpointer_t) (newval)))

#define g_atomic_pointer_compare_and_exchange(atomic, oldval, newval) \
  (g_atomic_pointer_compare_and_exchange ((atomic), (xpointer_t) (oldval), (xpointer_t) (newval)))
#define g_atomic_pointer_add(atomic, val) \
  (g_atomic_pointer_add ((atomic), (gssize) (val)))
#define g_atomic_pointer_and(atomic, val) \
  (g_atomic_pointer_and ((atomic), (xsize_t) (val)))
#define g_atomic_pointer_or(atomic, val) \
  (g_atomic_pointer_or ((atomic), (xsize_t) (val)))
#define g_atomic_pointer_xor(atomic, val) \
  (g_atomic_pointer_xor ((atomic), (xsize_t) (val)))

#endif /* defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) */

#endif /* __G_ATOMIC_H__ */
