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

#ifndef __G_MEM_H__
#define __G_MEM_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gutils.h>
#include <glib/glib-typeof.h>

G_BEGIN_DECLS

/**
 * GMemVTable:
 * @malloc: function to use for allocating memory.
 * @realloc: function to use for reallocating memory.
 * @free: function to use to free memory.
 * @calloc: function to use for allocating zero-filled memory.
 * @try_malloc: function to use for allocating memory without a default error handler.
 * @try_realloc: function to use for reallocating memory without a default error handler.
 *
 * A set of functions used to perform memory allocation. The same #GMemVTable must
 * be used for all allocations in the same program; a call to g_mem_set_vtable(),
 * if it exists, should be prior to any use of GLib.
 *
 * This functions related to this has been deprecated in 2.46, and no longer work.
 */
typedef struct _GMemVTable GMemVTable;


#if XPL_SIZEOF_VOID_P > XPL_SIZEOF_LONG
/**
 * G_MEM_ALIGN:
 *
 * Indicates the number of bytes to which memory will be aligned on the
 * current platform.
 */
#  define G_MEM_ALIGN	XPL_SIZEOF_VOID_P
#else	/* XPL_SIZEOF_VOID_P <= XPL_SIZEOF_LONG */
#  define G_MEM_ALIGN	XPL_SIZEOF_LONG
#endif	/* XPL_SIZEOF_VOID_P <= XPL_SIZEOF_LONG */


/* Memory allocation functions
 */

XPL_AVAILABLE_IN_ALL
void	 g_free	          (xpointer_t	 mem);

XPL_AVAILABLE_IN_2_34
void     g_clear_pointer  (xpointer_t      *pp,
                           GDestroyNotify destroy);

XPL_AVAILABLE_IN_ALL
xpointer_t g_malloc         (xsize_t	 n_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE(1);
XPL_AVAILABLE_IN_ALL
xpointer_t g_malloc0        (xsize_t	 n_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE(1);
XPL_AVAILABLE_IN_ALL
xpointer_t g_realloc        (xpointer_t	 mem,
			   xsize_t	 n_bytes) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xpointer_t g_try_malloc     (xsize_t	 n_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE(1);
XPL_AVAILABLE_IN_ALL
xpointer_t g_try_malloc0    (xsize_t	 n_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE(1);
XPL_AVAILABLE_IN_ALL
xpointer_t g_try_realloc    (xpointer_t	 mem,
			   xsize_t	 n_bytes) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_ALL
xpointer_t g_malloc_n       (xsize_t	 n_blocks,
			   xsize_t	 n_block_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE2(1,2);
XPL_AVAILABLE_IN_ALL
xpointer_t g_malloc0_n      (xsize_t	 n_blocks,
			   xsize_t	 n_block_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE2(1,2);
XPL_AVAILABLE_IN_ALL
xpointer_t g_realloc_n      (xpointer_t	 mem,
			   xsize_t	 n_blocks,
			   xsize_t	 n_block_bytes) G_GNUC_WARN_UNUSED_RESULT;
XPL_AVAILABLE_IN_ALL
xpointer_t g_try_malloc_n   (xsize_t	 n_blocks,
			   xsize_t	 n_block_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE2(1,2);
XPL_AVAILABLE_IN_ALL
xpointer_t g_try_malloc0_n  (xsize_t	 n_blocks,
			   xsize_t	 n_block_bytes) G_GNUC_MALLOC G_GNUC_ALLOC_SIZE2(1,2);
XPL_AVAILABLE_IN_ALL
xpointer_t g_try_realloc_n  (xpointer_t	 mem,
			   xsize_t	 n_blocks,
			   xsize_t	 n_block_bytes) G_GNUC_WARN_UNUSED_RESULT;

XPL_AVAILABLE_IN_2_72
xpointer_t g_aligned_alloc  (xsize_t         n_blocks,
                           xsize_t         n_block_bytes,
                           xsize_t         alignment) G_GNUC_WARN_UNUSED_RESULT G_GNUC_ALLOC_SIZE2(1,2);
XPL_AVAILABLE_IN_2_72
xpointer_t g_aligned_alloc0 (xsize_t         n_blocks,
                           xsize_t         n_block_bytes,
                           xsize_t         alignment) G_GNUC_WARN_UNUSED_RESULT G_GNUC_ALLOC_SIZE2(1,2);
XPL_AVAILABLE_IN_2_72
void     g_aligned_free   (xpointer_t      mem);

#if defined(glib_typeof) && XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_58
#define g_clear_pointer(pp, destroy)                     \
  G_STMT_START                                           \
  {                                                      \
    G_STATIC_ASSERT (sizeof *(pp) == sizeof (xpointer_t)); \
    glib_typeof ((pp)) _pp = (pp);                       \
    glib_typeof (*(pp)) _ptr = *_pp;                     \
    *_pp = NULL;                                         \
    if (_ptr)                                            \
      (destroy) (_ptr);                                  \
  }                                                      \
  G_STMT_END                                             \
  XPL_AVAILABLE_MACRO_IN_2_34
#else /* __GNUC__ */
#define g_clear_pointer(pp, destroy) \
  G_STMT_START {                                                               \
    G_STATIC_ASSERT (sizeof *(pp) == sizeof (xpointer_t));                       \
    /* Only one access, please; work around type aliasing */                   \
    union { char *in; xpointer_t *out; } _pp;                                    \
    xpointer_t _p;                                                               \
    /* This assignment is needed to avoid a gcc warning */                     \
    GDestroyNotify _destroy = (GDestroyNotify) (destroy);                      \
                                                                               \
    _pp.in = (char *) (pp);                                                    \
    _p = *_pp.out;                                                             \
    if (_p) 								       \
      { 								       \
        *_pp.out = NULL;                                                       \
        _destroy (_p);                                                         \
      }                                                                        \
  } G_STMT_END                                                                 \
  XPL_AVAILABLE_MACRO_IN_2_34
#endif /* __GNUC__ */

/**
 * g_steal_pointer:
 * @pp: (not nullable): a pointer to a pointer
 *
 * Sets @pp to %NULL, returning the value that was there before.
 *
 * Conceptually, this transfers the ownership of the pointer from the
 * referenced variable to the "caller" of the macro (ie: "steals" the
 * reference).
 *
 * The return value will be properly typed, according to the type of
 * @pp.
 *
 * This can be very useful when combined with g_autoptr() to prevent the
 * return value of a function from being automatically freed.  Consider
 * the following example (which only works on GCC and clang):
 *
 * |[
 * xobject_t *
 * create_object (void)
 * {
 *   g_autoptr(xobject_t) obj = g_object_new (XTYPE_OBJECT, NULL);
 *
 *   if (early_error_case)
 *     return NULL;
 *
 *   return g_steal_pointer (&obj);
 * }
 * ]|
 *
 * It can also be used in similar ways for 'out' parameters and is
 * particularly useful for dealing with optional out parameters:
 *
 * |[
 * xboolean_t
 * get_object (xobject_t **obj_out)
 * {
 *   g_autoptr(xobject_t) obj = g_object_new (XTYPE_OBJECT, NULL);
 *
 *   if (early_error_case)
 *     return FALSE;
 *
 *   if (obj_out)
 *     *obj_out = g_steal_pointer (&obj);
 *
 *   return TRUE;
 * }
 * ]|
 *
 * In the above example, the object will be automatically freed in the
 * early error case and also in the case that %NULL was given for
 * @obj_out.
 *
 * Since: 2.44
 */
XPL_AVAILABLE_STATIC_INLINE_IN_2_44
static inline xpointer_t
g_steal_pointer (xpointer_t pp)
{
  xpointer_t *ptr = (xpointer_t *) pp;
  xpointer_t ref;

  ref = *ptr;
  *ptr = NULL;

  return ref;
}

/* type safety */
#if defined(glib_typeof) && XPL_VERSION_MAX_ALLOWED >= XPL_VERSION_2_58
#define g_steal_pointer(pp) ((glib_typeof (*pp)) (g_steal_pointer) (pp))
#else  /* __GNUC__ */
/* This version does not depend on gcc extensions, but gcc does not warn
 * about incompatible-pointer-types: */
#define g_steal_pointer(pp) \
  (0 ? (*(pp)) : (g_steal_pointer) (pp))
#endif /* __GNUC__ */

/* Optimise: avoid the call to the (slower) _n function if we can
 * determine at compile-time that no overflow happens.
 */
#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#  define _G_NEW(struct_type, n_structs, func) \
	(struct_type *) (G_GNUC_EXTENSION ({			\
	  xsize_t __n = (xsize_t) (n_structs);			\
	  xsize_t __s = sizeof (struct_type);			\
	  xpointer_t __p;						\
	  if (__s == 1)						\
	    __p = g_##func (__n);				\
	  else if (__builtin_constant_p (__n) &&		\
	           (__s == 0 || __n <= G_MAXSIZE / __s))	\
	    __p = g_##func (__n * __s);				\
	  else							\
	    __p = g_##func##_n (__n, __s);			\
	  __p;							\
	}))
#  define _G_RENEW(struct_type, mem, n_structs, func) \
	(struct_type *) (G_GNUC_EXTENSION ({			\
	  xsize_t __n = (xsize_t) (n_structs);			\
	  xsize_t __s = sizeof (struct_type);			\
	  xpointer_t __p = (xpointer_t) (mem);			\
	  if (__s == 1)						\
	    __p = g_##func (__p, __n);				\
	  else if (__builtin_constant_p (__n) &&		\
	           (__s == 0 || __n <= G_MAXSIZE / __s))	\
	    __p = g_##func (__p, __n * __s);			\
	  else							\
	    __p = g_##func##_n (__p, __n, __s);			\
	  __p;							\
	}))

#else

/* Unoptimised version: always call the _n() function. */

#define _G_NEW(struct_type, n_structs, func) \
        ((struct_type *) g_##func##_n ((n_structs), sizeof (struct_type)))
#define _G_RENEW(struct_type, mem, n_structs, func) \
        ((struct_type *) g_##func##_n (mem, (n_structs), sizeof (struct_type)))

#endif

/**
 * g_new:
 * @struct_type: the type of the elements to allocate
 * @n_structs: the number of elements to allocate
 *
 * Allocates @n_structs elements of type @struct_type.
 * The returned pointer is cast to a pointer to the given type.
 * If @n_structs is 0 it returns %NULL.
 * Care is taken to avoid overflow when calculating the size of the allocated block.
 *
 * Since the returned pointer is already casted to the right type,
 * it is normally unnecessary to cast it explicitly, and doing
 * so might hide memory allocation errors.
 *
 * Returns: a pointer to the allocated memory, cast to a pointer to @struct_type
 */
#define g_new(struct_type, n_structs)			_G_NEW (struct_type, n_structs, malloc)
/**
 * g_new0:
 * @struct_type: the type of the elements to allocate.
 * @n_structs: the number of elements to allocate.
 *
 * Allocates @n_structs elements of type @struct_type, initialized to 0's.
 * The returned pointer is cast to a pointer to the given type.
 * If @n_structs is 0 it returns %NULL.
 * Care is taken to avoid overflow when calculating the size of the allocated block.
 *
 * Since the returned pointer is already casted to the right type,
 * it is normally unnecessary to cast it explicitly, and doing
 * so might hide memory allocation errors.
 *
 * Returns: a pointer to the allocated memory, cast to a pointer to @struct_type.
 */
#define g_new0(struct_type, n_structs)			_G_NEW (struct_type, n_structs, malloc0)
/**
 * g_renew:
 * @struct_type: the type of the elements to allocate
 * @mem: the currently allocated memory
 * @n_structs: the number of elements to allocate
 *
 * Reallocates the memory pointed to by @mem, so that it now has space for
 * @n_structs elements of type @struct_type. It returns the new address of
 * the memory, which may have been moved.
 * Care is taken to avoid overflow when calculating the size of the allocated block.
 *
 * Returns: a pointer to the new allocated memory, cast to a pointer to @struct_type
 */
#define g_renew(struct_type, mem, n_structs)		_G_RENEW (struct_type, mem, n_structs, realloc)
/**
 * g_try_new:
 * @struct_type: the type of the elements to allocate
 * @n_structs: the number of elements to allocate
 *
 * Attempts to allocate @n_structs elements of type @struct_type, and returns
 * %NULL on failure. Contrast with g_new(), which aborts the program on failure.
 * The returned pointer is cast to a pointer to the given type.
 * The function returns %NULL when @n_structs is 0 of if an overflow occurs.
 *
 * Since: 2.8
 * Returns: a pointer to the allocated memory, cast to a pointer to @struct_type
 */
#define g_try_new(struct_type, n_structs)		_G_NEW (struct_type, n_structs, try_malloc)
/**
 * g_try_new0:
 * @struct_type: the type of the elements to allocate
 * @n_structs: the number of elements to allocate
 *
 * Attempts to allocate @n_structs elements of type @struct_type, initialized
 * to 0's, and returns %NULL on failure. Contrast with g_new0(), which aborts
 * the program on failure.
 * The returned pointer is cast to a pointer to the given type.
 * The function returns %NULL when @n_structs is 0 or if an overflow occurs.
 *
 * Since: 2.8
 * Returns: a pointer to the allocated memory, cast to a pointer to @struct_type
 */
#define g_try_new0(struct_type, n_structs)		_G_NEW (struct_type, n_structs, try_malloc0)
/**
 * g_try_renew:
 * @struct_type: the type of the elements to allocate
 * @mem: the currently allocated memory
 * @n_structs: the number of elements to allocate
 *
 * Attempts to reallocate the memory pointed to by @mem, so that it now has
 * space for @n_structs elements of type @struct_type, and returns %NULL on
 * failure. Contrast with g_renew(), which aborts the program on failure.
 * It returns the new address of the memory, which may have been moved.
 * The function returns %NULL if an overflow occurs.
 *
 * Since: 2.8
 * Returns: a pointer to the new allocated memory, cast to a pointer to @struct_type
 */
#define g_try_renew(struct_type, mem, n_structs)	_G_RENEW (struct_type, mem, n_structs, try_realloc)


/* Memory allocation virtualization for debugging purposes
 * g_mem_set_vtable() has to be the very first GLib function called
 * if being used
 */
struct _GMemVTable {
  xpointer_t (*malloc)      (xsize_t    n_bytes);
  xpointer_t (*realloc)     (xpointer_t mem,
			   xsize_t    n_bytes);
  void     (*free)        (xpointer_t mem);
  /* optional; set to NULL if not used ! */
  xpointer_t (*calloc)      (xsize_t    n_blocks,
			   xsize_t    n_block_bytes);
  xpointer_t (*try_malloc)  (xsize_t    n_bytes);
  xpointer_t (*try_realloc) (xpointer_t mem,
			   xsize_t    n_bytes);
};
XPL_DEPRECATED_IN_2_46
void	 g_mem_set_vtable (GMemVTable	*vtable);
XPL_DEPRECATED_IN_2_46
xboolean_t g_mem_is_system_malloc (void);

XPL_VAR xboolean_t g_mem_gc_friendly;

/* Memory profiler and checker, has to be enabled via g_mem_set_vtable()
 */
XPL_VAR GMemVTable	*glib_mem_profiler_table;
XPL_DEPRECATED_IN_2_46
void	g_mem_profile	(void);

G_END_DECLS

#endif /* __G_MEM_H__ */
