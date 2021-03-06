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

#ifndef __G_TYPES_H__
#define __G_TYPES_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glibconfig.h>
#include <glib/gmacros.h>
#include <glib/gversionmacros.h>
#include <time.h>

G_BEGIN_DECLS

/* Provide type definitions for commonly used types.
 *  These are useful because a "gint8" can be adjusted
 *  to be 1 byte (8 bits) on all platforms. Similarly and
 *  more importantly, "gint32" can be adjusted to be
 *  4 bytes (32 bits) on all platforms.
 */

typedef char   xchar_t;
typedef short  gshort;
typedef long   xlong_t;
typedef int    xint_t;
typedef xint_t   xboolean_t;

typedef unsigned char   xuchar_t;
typedef unsigned short  gushort;
typedef unsigned long   xulong_t;
typedef unsigned int    xuint_t;

typedef float   gfloat;
typedef double  xdouble_t;

/* Define min and max constants for the fixed size numerical types */
/**
 * G_MININT8: (value -128)
 *
 * The minimum value which can be held in a #gint8.
 *
 * Since: 2.4
 */
#define G_MININT8	((gint8) (-G_MAXINT8 - 1))
#define G_MAXINT8	((gint8)  0x7f)
#define G_MAXUINT8	((xuint8_t) 0xff)

/**
 * G_MININT16: (value -32768)
 *
 * The minimum value which can be held in a #gint16.
 *
 * Since: 2.4
 */
#define G_MININT16	((gint16) (-G_MAXINT16 - 1))
#define G_MAXINT16	((gint16)  0x7fff)
#define G_MAXUINT16	((xuint16_t) 0xffff)

/**
 * G_MININT32: (value -2147483648)
 *
 * The minimum value which can be held in a #gint32.
 *
 * Since: 2.4
 */
#define G_MININT32	((gint32) (-G_MAXINT32 - 1))
#define G_MAXINT32	((gint32)  0x7fffffff)
#define G_MAXUINT32	((xuint32_t) 0xffffffff)

/**
 * G_MININT64: (value -9223372036854775808)
 *
 * The minimum value which can be held in a #sint64_t.
 */
#define G_MININT64	((sint64_t) (-G_MAXINT64 - G_GINT64_CONSTANT(1)))
#define G_MAXINT64	G_GINT64_CONSTANT(0x7fffffffffffffff)
#define G_MAXUINT64	G_GUINT64_CONSTANT(0xffffffffffffffff)

typedef void* xpointer_t;
typedef const void *xconstpointer;

typedef xint_t            (*GCompareFunc)         (xconstpointer  a,
                                                 xconstpointer  b);
typedef xint_t            (*GCompareDataFunc)     (xconstpointer  a,
                                                 xconstpointer  b,
						 xpointer_t       user_data);
typedef xboolean_t        (*GEqualFunc)           (xconstpointer  a,
                                                 xconstpointer  b);
typedef void            (*xdestroy_notify_t)       (xpointer_t       data);
typedef void            (*GFunc)                (xpointer_t       data,
                                                 xpointer_t       user_data);
typedef xuint_t           (*GHashFunc)            (xconstpointer  key);
typedef void            (*GHFunc)               (xpointer_t       key,
                                                 xpointer_t       value,
                                                 xpointer_t       user_data);

/**
 * GCopyFunc:
 * @src: (not nullable): A pointer to the data which should be copied
 * @data: Additional data
 *
 * A function of this signature is used to copy the node data
 * when doing a deep-copy of a tree.
 *
 * Returns: (not nullable): A pointer to the copy
 *
 * Since: 2.4
 */
typedef xpointer_t	(*GCopyFunc)            (xconstpointer  src,
                                                 xpointer_t       data);
/**
 * GFreeFunc:
 * @data: a data pointer
 *
 * Declares a type of function which takes an arbitrary
 * data pointer argument and has no return value. It is
 * not currently used in GLib or GTK+.
 */
typedef void            (*GFreeFunc)            (xpointer_t       data);

/**
 * GTranslateFunc:
 * @str: the untranslated string
 * @data: user data specified when installing the function, e.g.
 *  in xoption_group_set_translate_func()
 *
 * The type of functions which are used to translate user-visible
 * strings, for <option>--help</option> output.
 *
 * Returns: a translation of the string for the current locale.
 *  The returned string is owned by GLib and must not be freed.
 */
typedef const xchar_t *   (*GTranslateFunc)       (const xchar_t   *str,
						 xpointer_t       data);


/* Define some mathematical constants that aren't available
 * symbolically in some strict ISO C implementations.
 *
 * Note that the large number of digits used in these definitions
 * doesn't imply that GLib or current computers in general would be
 * able to handle floating point numbers with an accuracy like this.
 * It's mostly an exercise in futility and future proofing. For
 * extended precision floating point support, look somewhere else
 * than GLib.
 */
#define G_E     2.7182818284590452353602874713526624977572470937000
#define G_LN2   0.69314718055994530941723212145817656807550013436026
#define G_LN10  2.3025850929940456840179914546843642076011014886288
#define G_PI    3.1415926535897932384626433832795028841971693993751
#define G_PI_2  1.5707963267948966192313216916397514420985846996876
#define G_PI_4  0.78539816339744830961566084581987572104929234984378
#define G_SQRT2 1.4142135623730950488016887242096980785696718753769

/* Portable endian checks and conversions
 *
 * glibconfig.h defines G_BYTE_ORDER which expands to one of
 * the below macros.
 */
#define G_LITTLE_ENDIAN 1234
#define G_BIG_ENDIAN    4321
#define G_PDP_ENDIAN    3412		/* unused, need specific PDP check */


/* Basic bit swapping functions
 */
#define GUINT16_SWAP_LE_BE_CONSTANT(val)	((xuint16_t) ( \
    (xuint16_t) ((xuint16_t) (val) >> 8) |	\
    (xuint16_t) ((xuint16_t) (val) << 8)))

#define GUINT32_SWAP_LE_BE_CONSTANT(val)	((xuint32_t) ( \
    (((xuint32_t) (val) & (xuint32_t) 0x000000ffU) << 24) | \
    (((xuint32_t) (val) & (xuint32_t) 0x0000ff00U) <<  8) | \
    (((xuint32_t) (val) & (xuint32_t) 0x00ff0000U) >>  8) | \
    (((xuint32_t) (val) & (xuint32_t) 0xff000000U) >> 24)))

#define GUINT64_SWAP_LE_BE_CONSTANT(val)	((xuint64_t) ( \
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x00000000000000ffU)) << 56) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x000000000000ff00U)) << 40) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x0000000000ff0000U)) << 24) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x00000000ff000000U)) <<  8) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x000000ff00000000U)) >>  8) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x0000ff0000000000U)) >> 24) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0x00ff000000000000U)) >> 40) |	\
      (((xuint64_t) (val) &						\
	(xuint64_t) G_GINT64_CONSTANT (0xff00000000000000U)) >> 56)))

/* Arch specific stuff for speed
 */
#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)

#  if __GNUC__ >= 4 && defined (__GNUC_MINOR__) && __GNUC_MINOR__ >= 3
#    define GUINT32_SWAP_LE_BE(val) ((xuint32_t) __builtin_bswap32 ((xuint32_t) (val)))
#    define GUINT64_SWAP_LE_BE(val) ((xuint64_t) __builtin_bswap64 ((xuint64_t) (val)))
#  endif

#  if defined (__i386__)
#    define GUINT16_SWAP_LE_BE_IA32(val) \
       (G_GNUC_EXTENSION					\
	({ xuint16_t __v, __x = ((xuint16_t) (val));		\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT16_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ ("rorw $8, %w0"				\
		      : "=r" (__v)				\
		      : "0" (__x)				\
		      : "cc");					\
	    __v; }))
#    if !defined (__i486__) && !defined (__i586__) \
	&& !defined (__pentium__) && !defined (__i686__) \
	&& !defined (__pentiumpro__) && !defined (__pentium4__)
#       define GUINT32_SWAP_LE_BE_IA32(val) \
	  (G_GNUC_EXTENSION					\
	   ({ xuint32_t __v, __x = ((xuint32_t) (val));		\
	      if (__builtin_constant_p (__x))			\
		__v = GUINT32_SWAP_LE_BE_CONSTANT (__x);	\
	      else						\
		__asm__ ("rorw $8, %w0\n\t"			\
			 "rorl $16, %0\n\t"			\
			 "rorw $8, %w0"				\
			 : "=r" (__v)				\
			 : "0" (__x)				\
			 : "cc");				\
	      __v; }))
#    else /* 486 and higher has bswap */
#       define GUINT32_SWAP_LE_BE_IA32(val) \
	  (G_GNUC_EXTENSION					\
	   ({ xuint32_t __v, __x = ((xuint32_t) (val));		\
	      if (__builtin_constant_p (__x))			\
		__v = GUINT32_SWAP_LE_BE_CONSTANT (__x);	\
	      else						\
		__asm__ ("bswap %0"				\
			 : "=r" (__v)				\
			 : "0" (__x));				\
	      __v; }))
#    endif /* processor specific 32-bit stuff */
#    define GUINT64_SWAP_LE_BE_IA32(val) \
       (G_GNUC_EXTENSION						\
	({ union { xuint64_t __ll;					\
		   xuint32_t __l[2]; } __w, __r;				\
	   __w.__ll = ((xuint64_t) (val));				\
	   if (__builtin_constant_p (__w.__ll))				\
	     __r.__ll = GUINT64_SWAP_LE_BE_CONSTANT (__w.__ll);		\
	   else								\
	     {								\
	       __r.__l[0] = GUINT32_SWAP_LE_BE (__w.__l[1]);		\
	       __r.__l[1] = GUINT32_SWAP_LE_BE (__w.__l[0]);		\
	     }								\
	   __r.__ll; }))
     /* Possibly just use the constant version and let gcc figure it out? */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_IA32 (val))
#    ifndef GUINT32_SWAP_LE_BE
#      define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_IA32 (val))
#    endif
#    ifndef GUINT64_SWAP_LE_BE
#      define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_IA32 (val))
#    endif
#  elif defined (__ia64__)
#    define GUINT16_SWAP_LE_BE_IA64(val) \
       (G_GNUC_EXTENSION					\
	({ xuint16_t __v, __x = ((xuint16_t) (val));		\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT16_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ __volatile__ ("shl %0 = %1, 48 ;;"		\
				   "mux1 %0 = %0, @rev ;;"	\
				    : "=r" (__v)		\
				    : "r" (__x));		\
	    __v; }))
#    define GUINT32_SWAP_LE_BE_IA64(val) \
       (G_GNUC_EXTENSION					\
	 ({ xuint32_t __v, __x = ((xuint32_t) (val));		\
	    if (__builtin_constant_p (__x))			\
	      __v = GUINT32_SWAP_LE_BE_CONSTANT (__x);		\
	    else						\
	     __asm__ __volatile__ ("shl %0 = %1, 32 ;;"		\
				   "mux1 %0 = %0, @rev ;;"	\
				    : "=r" (__v)		\
				    : "r" (__x));		\
	    __v; }))
#    define GUINT64_SWAP_LE_BE_IA64(val) \
       (G_GNUC_EXTENSION					\
	({ xuint64_t __v, __x = ((xuint64_t) (val));		\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT64_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ __volatile__ ("mux1 %0 = %1, @rev ;;"	\
				   : "=r" (__v)			\
				   : "r" (__x));		\
	   __v; }))
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_IA64 (val))
#    ifndef GUINT32_SWAP_LE_BE
#      define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_IA64 (val))
#    endif
#    ifndef GUINT64_SWAP_LE_BE
#      define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_IA64 (val))
#    endif
#  elif defined (__x86_64__)
#    define GUINT32_SWAP_LE_BE_X86_64(val) \
       (G_GNUC_EXTENSION					\
	 ({ xuint32_t __v, __x = ((xuint32_t) (val));		\
	    if (__builtin_constant_p (__x))			\
	      __v = GUINT32_SWAP_LE_BE_CONSTANT (__x);		\
	    else						\
	     __asm__ ("bswapl %0"				\
		      : "=r" (__v)				\
		      : "0" (__x));				\
	    __v; }))
#    define GUINT64_SWAP_LE_BE_X86_64(val) \
       (G_GNUC_EXTENSION					\
	({ xuint64_t __v, __x = ((xuint64_t) (val));		\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT64_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ ("bswapq %0"				\
		      : "=r" (__v)				\
		      : "0" (__x));				\
	   __v; }))
     /* gcc seems to figure out optimal code for this on its own */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#    ifndef GUINT32_SWAP_LE_BE
#      define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_X86_64 (val))
#    endif
#    ifndef GUINT64_SWAP_LE_BE
#      define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_X86_64 (val))
#    endif
#  else /* generic gcc */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#    ifndef GUINT32_SWAP_LE_BE
#      define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))
#    endif
#    ifndef GUINT64_SWAP_LE_BE
#      define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_CONSTANT (val))
#    endif
#  endif
#else /* generic */
#  define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#  define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))
#  define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_CONSTANT (val))
#endif /* generic */

#define GUINT16_SWAP_LE_PDP(val)	((xuint16_t) (val))
#define GUINT16_SWAP_BE_PDP(val)	(GUINT16_SWAP_LE_BE (val))
#define GUINT32_SWAP_LE_PDP(val)	((xuint32_t) ( \
    (((xuint32_t) (val) & (xuint32_t) 0x0000ffffU) << 16) | \
    (((xuint32_t) (val) & (xuint32_t) 0xffff0000U) >> 16)))
#define GUINT32_SWAP_BE_PDP(val)	((xuint32_t) ( \
    (((xuint32_t) (val) & (xuint32_t) 0x00ff00ffU) << 8) | \
    (((xuint32_t) (val) & (xuint32_t) 0xff00ff00U) >> 8)))

/* The G*_TO_?E() macros are defined in glibconfig.h.
 * The transformation is symmetric, so the FROM just maps to the TO.
 */
#define GINT16_FROM_LE(val)	(GINT16_TO_LE (val))
#define GUINT16_FROM_LE(val)	(GUINT16_TO_LE (val))
#define GINT16_FROM_BE(val)	(GINT16_TO_BE (val))
#define GUINT16_FROM_BE(val)	(GUINT16_TO_BE (val))
#define GINT32_FROM_LE(val)	(GINT32_TO_LE (val))
#define GUINT32_FROM_LE(val)	(GUINT32_TO_LE (val))
#define GINT32_FROM_BE(val)	(GINT32_TO_BE (val))
#define GUINT32_FROM_BE(val)	(GUINT32_TO_BE (val))

#define GINT64_FROM_LE(val)	(GINT64_TO_LE (val))
#define GUINT64_FROM_LE(val)	(GUINT64_TO_LE (val))
#define GINT64_FROM_BE(val)	(GINT64_TO_BE (val))
#define GUINT64_FROM_BE(val)	(GUINT64_TO_BE (val))

#define GLONG_FROM_LE(val)	(GLONG_TO_LE (val))
#define GULONG_FROM_LE(val)	(GULONG_TO_LE (val))
#define GLONG_FROM_BE(val)	(GLONG_TO_BE (val))
#define GULONG_FROM_BE(val)	(GULONG_TO_BE (val))

#define GINT_FROM_LE(val)	(GINT_TO_LE (val))
#define GUINT_FROM_LE(val)	(GUINT_TO_LE (val))
#define GINT_FROM_BE(val)	(GINT_TO_BE (val))
#define GUINT_FROM_BE(val)	(GUINT_TO_BE (val))

#define GSIZE_FROM_LE(val)	(GSIZE_TO_LE (val))
#define GSSIZE_FROM_LE(val)	(GSSIZE_TO_LE (val))
#define GSIZE_FROM_BE(val)	(GSIZE_TO_BE (val))
#define GSSIZE_FROM_BE(val)	(GSSIZE_TO_BE (val))

/* Portable versions of host-network order stuff
 */
#define g_ntohl(val) (GUINT32_FROM_BE (val))
#define g_ntohs(val) (GUINT16_FROM_BE (val))
#define g_htonl(val) (GUINT32_TO_BE (val))
#define g_htons(val) (GUINT16_TO_BE (val))

/* Overflow-checked unsigned integer arithmetic
 */
#ifndef _XPL_TEST_OVERFLOW_FALLBACK
/* https://bugzilla.gnome.org/show_bug.cgi?id=769104 */
#if __GNUC__ >= 5 && !defined(__INTEL_COMPILER)
#define _XPL_HAVE_BUILTIN_OVERFLOW_CHECKS
#elif g_macro__has_builtin(__builtin_add_overflow)
#define _XPL_HAVE_BUILTIN_OVERFLOW_CHECKS
#endif
#endif

#ifdef _XPL_HAVE_BUILTIN_OVERFLOW_CHECKS

#define g_uint_checked_add(dest, a, b) \
    (!__builtin_add_overflow(a, b, dest))
#define g_uint_checked_mul(dest, a, b) \
    (!__builtin_mul_overflow(a, b, dest))

#define g_uint64_checked_add(dest, a, b) \
    (!__builtin_add_overflow(a, b, dest))
#define g_uint64_checked_mul(dest, a, b) \
    (!__builtin_mul_overflow(a, b, dest))

#define g_size_checked_add(dest, a, b) \
    (!__builtin_add_overflow(a, b, dest))
#define g_size_checked_mul(dest, a, b) \
    (!__builtin_mul_overflow(a, b, dest))

#else  /* !_XPL_HAVE_BUILTIN_OVERFLOW_CHECKS */

/* The names of the following inlines are private.  Use the macro
 * definitions above.
 */
static inline xboolean_t _XPL_CHECKED_ADD_UINT (xuint_t *dest, xuint_t a, xuint_t b) {
  *dest = a + b; return *dest >= a; }
static inline xboolean_t _XPL_CHECKED_MUL_UINT (xuint_t *dest, xuint_t a, xuint_t b) {
  *dest = a * b; return !a || *dest / a == b; }
static inline xboolean_t _XPL_CHECKED_ADD_UINT64 (xuint64_t *dest, xuint64_t a, xuint64_t b) {
  *dest = a + b; return *dest >= a; }
static inline xboolean_t _XPL_CHECKED_MUL_UINT64 (xuint64_t *dest, xuint64_t a, xuint64_t b) {
  *dest = a * b; return !a || *dest / a == b; }
static inline xboolean_t _XPL_CHECKED_ADD_SIZE (xsize_t *dest, xsize_t a, xsize_t b) {
  *dest = a + b; return *dest >= a; }
static inline xboolean_t _XPL_CHECKED_MUL_SIZE (xsize_t *dest, xsize_t a, xsize_t b) {
  *dest = a * b; return !a || *dest / a == b; }

#define g_uint_checked_add(dest, a, b) \
    _XPL_CHECKED_ADD_UINT(dest, a, b)
#define g_uint_checked_mul(dest, a, b) \
    _XPL_CHECKED_MUL_UINT(dest, a, b)

#define g_uint64_checked_add(dest, a, b) \
    _XPL_CHECKED_ADD_UINT64(dest, a, b)
#define g_uint64_checked_mul(dest, a, b) \
    _XPL_CHECKED_MUL_UINT64(dest, a, b)

#define g_size_checked_add(dest, a, b) \
    _XPL_CHECKED_ADD_SIZE(dest, a, b)
#define g_size_checked_mul(dest, a, b) \
    _XPL_CHECKED_MUL_SIZE(dest, a, b)

#endif  /* !_XPL_HAVE_BUILTIN_OVERFLOW_CHECKS */

/* IEEE Standard 754 Single Precision Storage Format (gfloat):
 *
 *        31 30           23 22            0
 * +--------+---------------+---------------+
 * | s 1bit | e[30:23] 8bit | f[22:0] 23bit |
 * +--------+---------------+---------------+
 * B0------------------->B1------->B2-->B3-->
 *
 * IEEE Standard 754 Double Precision Storage Format (xdouble_t):
 *
 *        63 62            52 51            32   31            0
 * +--------+----------------+----------------+ +---------------+
 * | s 1bit | e[62:52] 11bit | f[51:32] 20bit | | f[31:0] 32bit |
 * +--------+----------------+----------------+ +---------------+
 * B0--------------->B1---------->B2--->B3---->  B4->B5->B6->B7->
 */
/* subtract from biased_exponent to form base2 exponent (normal numbers) */
typedef union  _GDoubleIEEE754	GDoubleIEEE754;
typedef union  _GFloatIEEE754	GFloatIEEE754;
#define G_IEEE754_FLOAT_BIAS	(127)
#define G_IEEE754_DOUBLE_BIAS	(1023)
/* multiply with base2 exponent to get base10 exponent (normal numbers) */
#define G_LOG_2_BASE_10		(0.30102999566398119521)
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
union _GFloatIEEE754
{
  gfloat v_float;
  struct {
    xuint_t mantissa : 23;
    xuint_t biased_exponent : 8;
    xuint_t sign : 1;
  } mpn;
};
union _GDoubleIEEE754
{
  xdouble_t v_double;
  struct {
    xuint_t mantissa_low : 32;
    xuint_t mantissa_high : 20;
    xuint_t biased_exponent : 11;
    xuint_t sign : 1;
  } mpn;
};
#elif G_BYTE_ORDER == G_BIG_ENDIAN
union _GFloatIEEE754
{
  gfloat v_float;
  struct {
    xuint_t sign : 1;
    xuint_t biased_exponent : 8;
    xuint_t mantissa : 23;
  } mpn;
};
union _GDoubleIEEE754
{
  xdouble_t v_double;
  struct {
    xuint_t sign : 1;
    xuint_t biased_exponent : 11;
    xuint_t mantissa_high : 20;
    xuint_t mantissa_low : 32;
  } mpn;
};
#else /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */
#error unknown ENDIAN type
#endif /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */

typedef struct _GTimeVal GTimeVal XPL_DEPRECATED_TYPE_IN_2_62_FOR(xdatetime_t);

struct _GTimeVal
{
  xlong_t tv_sec;
  xlong_t tv_usec;
} XPL_DEPRECATED_TYPE_IN_2_62_FOR(xdatetime_t);

typedef xint_t grefcount;
typedef xint_t gatomicrefcount;  /* should be accessed only using atomics */

G_END_DECLS

/* We prefix variable declarations so they can
 * properly get exported in Windows DLLs.
 */
#ifndef XPL_VAR
#  ifdef XPLATFORM_WIN32
#    ifdef XPL_STATIC_COMPILATION
#      define XPL_VAR extern
#    else /* !XPL_STATIC_COMPILATION */
#      ifdef XPL_COMPILATION
#        ifdef DLL_EXPORT
#          define XPL_VAR extern __declspec(dllexport)
#        else /* !DLL_EXPORT */
#          define XPL_VAR extern
#        endif /* !DLL_EXPORT */
#      else /* !XPL_COMPILATION */
#        define XPL_VAR extern __declspec(dllimport)
#      endif /* !XPL_COMPILATION */
#    endif /* !XPL_STATIC_COMPILATION */
#  else /* !XPLATFORM_WIN32 */
#    define XPL_VAR _XPL_EXTERN
#  endif /* !XPLATFORM_WIN32 */
#endif /* XPL_VAR */

#endif /* __G_TYPES_H__ */
