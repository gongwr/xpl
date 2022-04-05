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

#ifndef __G_VERSION_MACROS_H__
#define __G_VERSION_MACROS_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

/* Version boundaries checks */

#define G_ENCODE_VERSION(major,minor)   ((major) << 16 | (minor) << 8)

/* XXX: Every new stable minor release bump should add a macro here */

/**
 * XPL_VERSION_2_26:
 *
 * A macro that evaluates to the 2.26 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.32
 */
#define XPL_VERSION_2_26       (G_ENCODE_VERSION (2, 26))

/**
 * XPL_VERSION_2_28:
 *
 * A macro that evaluates to the 2.28 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.32
 */
#define XPL_VERSION_2_28       (G_ENCODE_VERSION (2, 28))

/**
 * XPL_VERSION_2_30:
 *
 * A macro that evaluates to the 2.30 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.32
 */
#define XPL_VERSION_2_30       (G_ENCODE_VERSION (2, 30))

/**
 * XPL_VERSION_2_32:
 *
 * A macro that evaluates to the 2.32 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.32
 */
#define XPL_VERSION_2_32       (G_ENCODE_VERSION (2, 32))

/**
 * XPL_VERSION_2_34:
 *
 * A macro that evaluates to the 2.34 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.34
 */
#define XPL_VERSION_2_34       (G_ENCODE_VERSION (2, 34))

/**
 * XPL_VERSION_2_36:
 *
 * A macro that evaluates to the 2.36 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.36
 */
#define XPL_VERSION_2_36       (G_ENCODE_VERSION (2, 36))

/**
 * XPL_VERSION_2_38:
 *
 * A macro that evaluates to the 2.38 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.38
 */
#define XPL_VERSION_2_38       (G_ENCODE_VERSION (2, 38))

/**
 * XPL_VERSION_2_40:
 *
 * A macro that evaluates to the 2.40 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.40
 */
#define XPL_VERSION_2_40       (G_ENCODE_VERSION (2, 40))

/**
 * XPL_VERSION_2_42:
 *
 * A macro that evaluates to the 2.42 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.42
 */
#define XPL_VERSION_2_42       (G_ENCODE_VERSION (2, 42))

/**
 * XPL_VERSION_2_44:
 *
 * A macro that evaluates to the 2.44 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.44
 */
#define XPL_VERSION_2_44       (G_ENCODE_VERSION (2, 44))

/**
 * XPL_VERSION_2_46:
 *
 * A macro that evaluates to the 2.46 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.46
 */
#define XPL_VERSION_2_46       (G_ENCODE_VERSION (2, 46))

/**
 * XPL_VERSION_2_48:
 *
 * A macro that evaluates to the 2.48 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.48
 */
#define XPL_VERSION_2_48       (G_ENCODE_VERSION (2, 48))

/**
 * XPL_VERSION_2_50:
 *
 * A macro that evaluates to the 2.50 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.50
 */
#define XPL_VERSION_2_50       (G_ENCODE_VERSION (2, 50))

/**
 * XPL_VERSION_2_52:
 *
 * A macro that evaluates to the 2.52 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.52
 */
#define XPL_VERSION_2_52       (G_ENCODE_VERSION (2, 52))

/**
 * XPL_VERSION_2_54:
 *
 * A macro that evaluates to the 2.54 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.54
 */
#define XPL_VERSION_2_54       (G_ENCODE_VERSION (2, 54))

/**
 * XPL_VERSION_2_56:
 *
 * A macro that evaluates to the 2.56 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.56
 */
#define XPL_VERSION_2_56       (G_ENCODE_VERSION (2, 56))

/**
 * XPL_VERSION_2_58:
 *
 * A macro that evaluates to the 2.58 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.58
 */
#define XPL_VERSION_2_58       (G_ENCODE_VERSION (2, 58))

/**
 * XPL_VERSION_2_60:
 *
 * A macro that evaluates to the 2.60 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.60
 */
#define XPL_VERSION_2_60       (G_ENCODE_VERSION (2, 60))

/**
 * XPL_VERSION_2_62:
 *
 * A macro that evaluates to the 2.62 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.62
 */
#define XPL_VERSION_2_62       (G_ENCODE_VERSION (2, 62))

/**
 * XPL_VERSION_2_64:
 *
 * A macro that evaluates to the 2.64 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.64
 */
#define XPL_VERSION_2_64       (G_ENCODE_VERSION (2, 64))

/**
 * XPL_VERSION_2_66:
 *
 * A macro that evaluates to the 2.66 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.66
 */
#define XPL_VERSION_2_66       (G_ENCODE_VERSION (2, 66))

/**
 * XPL_VERSION_2_68:
 *
 * A macro that evaluates to the 2.68 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.68
 */
#define XPL_VERSION_2_68       (G_ENCODE_VERSION (2, 68))

/**
 * XPL_VERSION_2_70:
 *
 * A macro that evaluates to the 2.70 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.70
 */
#define XPL_VERSION_2_70       (G_ENCODE_VERSION (2, 70))

/**
 * XPL_VERSION_2_72:
 *
 * A macro that evaluates to the 2.72 version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * Since: 2.72
 */
#define XPL_VERSION_2_72       (G_ENCODE_VERSION (2, 72))

/**
 * XPL_VERSION_CUR_STABLE:
 *
 * A macro that evaluates to the current stable version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * During an unstable development cycle, this evaluates to the next stable
 * (unreleased) version which will be the result of the development cycle.
 *
 * Since: 2.32
 */
#if (XPL_MINOR_VERSION % 2)
#define XPL_VERSION_CUR_STABLE         (G_ENCODE_VERSION (XPL_MAJOR_VERSION, XPL_MINOR_VERSION + 1))
#else
#define XPL_VERSION_CUR_STABLE         (G_ENCODE_VERSION (XPL_MAJOR_VERSION, XPL_MINOR_VERSION))
#endif

/**
 * XPL_VERSION_PREV_STABLE:
 *
 * A macro that evaluates to the previous stable version of GLib, in a format
 * that can be used by the C pre-processor.
 *
 * During an unstable development cycle, this evaluates to the most recent
 * released stable release, which preceded this development cycle.
 *
 * Since: 2.32
 */
#if (XPL_MINOR_VERSION % 2)
#define XPL_VERSION_PREV_STABLE        (G_ENCODE_VERSION (XPL_MAJOR_VERSION, XPL_MINOR_VERSION - 1))
#else
#define XPL_VERSION_PREV_STABLE        (G_ENCODE_VERSION (XPL_MAJOR_VERSION, XPL_MINOR_VERSION - 2))
#endif

/**
 * XPL_VERSION_MIN_REQUIRED:
 *
 * A macro that should be defined by the user prior to including
 * the glib.h header.
 * The definition should be one of the predefined GLib version
 * macros: %XPL_VERSION_2_26, %XPL_VERSION_2_28,...
 *
 * This macro defines the earliest version of GLib that the package is
 * required to be able to compile against.
 *
 * If the compiler is configured to warn about the use of deprecated
 * functions, then using functions that were deprecated in version
 * %XPL_VERSION_MIN_REQUIRED or earlier will cause warnings (but
 * using functions deprecated in later releases will not).
 *
 * Since: 2.32
 */
/* If the package sets XPL_VERSION_MIN_REQUIRED to some future
 * XPL_VERSION_X_Y value that we don't know about, it will compare as
 * 0 in preprocessor tests.
 */
#ifndef XPL_VERSION_MIN_REQUIRED
# define XPL_VERSION_MIN_REQUIRED      (XPL_VERSION_CUR_STABLE)
#elif XPL_VERSION_MIN_REQUIRED == 0
# undef  XPL_VERSION_MIN_REQUIRED
# define XPL_VERSION_MIN_REQUIRED      (XPL_VERSION_CUR_STABLE + 2)
#endif

/**
 * XPL_VERSION_MAX_ALLOWED:
 *
 * A macro that should be defined by the user prior to including
 * the glib.h header.
 * The definition should be one of the predefined GLib version
 * macros: %XPL_VERSION_2_26, %XPL_VERSION_2_28,...
 *
 * This macro defines the latest version of the GLib API that the
 * package is allowed to make use of.
 *
 * If the compiler is configured to warn about the use of deprecated
 * functions, then using functions added after version
 * %XPL_VERSION_MAX_ALLOWED will cause warnings.
 *
 * Unless you are using XPL_CHECK_VERSION() or the like to compile
 * different code depending on the GLib version, then this should be
 * set to the same value as %XPL_VERSION_MIN_REQUIRED.
 *
 * Since: 2.32
 */
#if !defined (XPL_VERSION_MAX_ALLOWED) || (XPL_VERSION_MAX_ALLOWED == 0)
# undef XPL_VERSION_MAX_ALLOWED
# define XPL_VERSION_MAX_ALLOWED      (XPL_VERSION_CUR_STABLE)
#endif

/* sanity checks */
#if XPL_VERSION_MIN_REQUIRED > XPL_VERSION_CUR_STABLE
#error "XPL_VERSION_MIN_REQUIRED must be <= XPL_VERSION_CUR_STABLE"
#endif
#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_MIN_REQUIRED
#error "XPL_VERSION_MAX_ALLOWED must be >= XPL_VERSION_MIN_REQUIRED"
#endif
#if XPL_VERSION_MIN_REQUIRED < XPL_VERSION_2_26
#error "XPL_VERSION_MIN_REQUIRED must be >= XPL_VERSION_2_26"
#endif

/* These macros are used to mark deprecated functions in GLib headers,
 * and thus have to be exposed in installed headers. But please
 * do *not* use them in other projects. Instead, use G_DEPRECATED
 * or define your own wrappers around it.
 */
#define XPL_AVAILABLE_IN_ALL                   _XPL_EXTERN

/* XXX: Every new stable minor release should add a set of macros here */

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_26
# define XPL_DEPRECATED_IN_2_26                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_26_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_26          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_26_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_26          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_26_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_26           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_26_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_26                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_26_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_26
# define XPL_DEPRECATED_MACRO_IN_2_26_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_26
# define XPL_DEPRECATED_ENUMERATOR_IN_2_26_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_26
# define XPL_DEPRECATED_TYPE_IN_2_26_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_26
# define XPL_AVAILABLE_IN_2_26                 XPL_UNAVAILABLE(2, 26)
# define XPL_AVAILABLE_MACRO_IN_2_26           XPL_UNAVAILABLE_MACRO(2, 26)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_26      XPL_UNAVAILABLE_ENUMERATOR(2, 26)
# define XPL_AVAILABLE_TYPE_IN_2_26            XPL_UNAVAILABLE_TYPE(2, 26)
#else
# define XPL_AVAILABLE_IN_2_26                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_26
# define XPL_AVAILABLE_ENUMERATOR_IN_2_26
# define XPL_AVAILABLE_TYPE_IN_2_26
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_28
# define XPL_DEPRECATED_IN_2_28                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_28_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_28          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_28_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_28          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_28_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_28           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_28_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_28                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_28_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_28
# define XPL_DEPRECATED_MACRO_IN_2_28_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_28
# define XPL_DEPRECATED_ENUMERATOR_IN_2_28_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_28
# define XPL_DEPRECATED_TYPE_IN_2_28_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_28
# define XPL_AVAILABLE_IN_2_28                 XPL_UNAVAILABLE(2, 28)
# define XPL_AVAILABLE_MACRO_IN_2_28           XPL_UNAVAILABLE_MACRO(2, 28)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_28      XPL_UNAVAILABLE_ENUMERATOR(2, 28)
# define XPL_AVAILABLE_TYPE_IN_2_28            XPL_UNAVAILABLE_TYPE(2, 28)
#else
# define XPL_AVAILABLE_IN_2_28                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_28
# define XPL_AVAILABLE_ENUMERATOR_IN_2_28
# define XPL_AVAILABLE_TYPE_IN_2_28
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_30
# define XPL_DEPRECATED_IN_2_30                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_30_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_30          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_30_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_30          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_30_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_30           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_30_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_30                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_30_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_30
# define XPL_DEPRECATED_MACRO_IN_2_30_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_30
# define XPL_DEPRECATED_ENUMERATOR_IN_2_30_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_30
# define XPL_DEPRECATED_TYPE_IN_2_30_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_30
# define XPL_AVAILABLE_IN_2_30                 XPL_UNAVAILABLE(2, 30)
# define XPL_AVAILABLE_MACRO_IN_2_30           XPL_UNAVAILABLE_MACRO(2, 30)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_30      XPL_UNAVAILABLE_ENUMERATOR(2, 30)
# define XPL_AVAILABLE_TYPE_IN_2_30            XPL_UNAVAILABLE_TYPE(2, 30)
#else
# define XPL_AVAILABLE_IN_2_30                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_30
# define XPL_AVAILABLE_ENUMERATOR_IN_2_30
# define XPL_AVAILABLE_TYPE_IN_2_30
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_32
# define XPL_DEPRECATED_IN_2_32                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_32_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_32          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_32_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_32          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_32_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_32           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_32_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_32                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_32_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_32
# define XPL_DEPRECATED_MACRO_IN_2_32_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_32
# define XPL_DEPRECATED_ENUMERATOR_IN_2_32_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_32
# define XPL_DEPRECATED_TYPE_IN_2_32_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_32
# define XPL_DEPRECATED_ENUMERATOR_IN_2_32_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_32
# define XPL_DEPRECATED_TYPE_IN_2_32_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_32
# define XPL_AVAILABLE_IN_2_32                 XPL_UNAVAILABLE(2, 32)
# define XPL_AVAILABLE_MACRO_IN_2_32           XPL_UNAVAILABLE_MACRO(2, 32)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_32      XPL_UNAVAILABLE_ENUMERATOR(2, 32)
# define XPL_AVAILABLE_TYPE_IN_2_32            XPL_UNAVAILABLE_TYPE(2, 32)
#else
# define XPL_AVAILABLE_IN_2_32                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_32
# define XPL_AVAILABLE_ENUMERATOR_IN_2_32
# define XPL_AVAILABLE_TYPE_IN_2_32
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_34
# define XPL_DEPRECATED_IN_2_34                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_34_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_34          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_34_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_34          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_34_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_34           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_34_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_34                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_34_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_34
# define XPL_DEPRECATED_MACRO_IN_2_34_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_34
# define XPL_DEPRECATED_ENUMERATOR_IN_2_34_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_34
# define XPL_DEPRECATED_TYPE_IN_2_34_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_34
# define XPL_AVAILABLE_IN_2_34                 XPL_UNAVAILABLE(2, 34)
# define XPL_AVAILABLE_MACRO_IN_2_34           XPL_UNAVAILABLE_MACRO(2, 34)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_34      XPL_UNAVAILABLE_ENUMERATOR(2, 34)
# define XPL_AVAILABLE_TYPE_IN_2_34            XPL_UNAVAILABLE_TYPE(2, 34)
#else
# define XPL_AVAILABLE_IN_2_34                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_34
# define XPL_AVAILABLE_ENUMERATOR_IN_2_34
# define XPL_AVAILABLE_TYPE_IN_2_34
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_36
# define XPL_DEPRECATED_IN_2_36                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_36_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_36          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_36_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_36          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_36_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_36           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_36_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_36                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_36_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_36
# define XPL_DEPRECATED_MACRO_IN_2_36_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_36
# define XPL_DEPRECATED_ENUMERATOR_IN_2_36_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_36
# define XPL_DEPRECATED_TYPE_IN_2_36_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_36
# define XPL_AVAILABLE_IN_2_36                 XPL_UNAVAILABLE(2, 36)
# define XPL_AVAILABLE_MACRO_IN_2_36           XPL_UNAVAILABLE_MACRO(2, 36)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_36      XPL_UNAVAILABLE_ENUMERATOR(2, 36)
# define XPL_AVAILABLE_TYPE_IN_2_36            XPL_UNAVAILABLE_TYPE(2, 36)
#else
# define XPL_AVAILABLE_IN_2_36                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_36
# define XPL_AVAILABLE_ENUMERATOR_IN_2_36
# define XPL_AVAILABLE_TYPE_IN_2_36
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_38
# define XPL_DEPRECATED_IN_2_38                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_38_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_38          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_38_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_38          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_38_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_38           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_38_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_38                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_38_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_38
# define XPL_DEPRECATED_MACRO_IN_2_38_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_38
# define XPL_DEPRECATED_ENUMERATOR_IN_2_38_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_38
# define XPL_DEPRECATED_TYPE_IN_2_38_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_38
# define XPL_AVAILABLE_IN_2_38                 XPL_UNAVAILABLE(2, 38)
# define XPL_AVAILABLE_MACRO_IN_2_38           XPL_UNAVAILABLE_MACRO(2, 38)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_38      XPL_UNAVAILABLE_ENUMERATOR(2, 38)
# define XPL_AVAILABLE_TYPE_IN_2_38            XPL_UNAVAILABLE_TYPE(2, 38)
#else
# define XPL_AVAILABLE_IN_2_38                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_38
# define XPL_AVAILABLE_ENUMERATOR_IN_2_38
# define XPL_AVAILABLE_TYPE_IN_2_38
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_40
# define XPL_DEPRECATED_IN_2_40                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_40_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_40          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_40_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_40          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_40_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_40           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_40_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_40                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_40_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_40
# define XPL_DEPRECATED_MACRO_IN_2_40_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_40
# define XPL_DEPRECATED_ENUMERATOR_IN_2_40_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_40
# define XPL_DEPRECATED_TYPE_IN_2_40_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_40
# define XPL_AVAILABLE_IN_2_40                 XPL_UNAVAILABLE(2, 40)
# define XPL_AVAILABLE_MACRO_IN_2_40           XPL_UNAVAILABLE_MACRO(2, 40)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_40      XPL_UNAVAILABLE_ENUMERATOR(2, 40)
# define XPL_AVAILABLE_TYPE_IN_2_40            XPL_UNAVAILABLE_TYPE(2, 40)
#else
# define XPL_AVAILABLE_IN_2_40                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_40
# define XPL_AVAILABLE_ENUMERATOR_IN_2_40
# define XPL_AVAILABLE_TYPE_IN_2_40
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_42
# define XPL_DEPRECATED_IN_2_42                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_42_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_42          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_42_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_42                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_42_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_42
# define XPL_DEPRECATED_MACRO_IN_2_42_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_42
# define XPL_DEPRECATED_ENUMERATOR_IN_2_42_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_42
# define XPL_DEPRECATED_TYPE_IN_2_42_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_42
# define XPL_AVAILABLE_IN_2_42                 XPL_UNAVAILABLE(2, 42)
# define XPL_AVAILABLE_MACRO_IN_2_42           XPL_UNAVAILABLE_MACRO(2, 42)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_42      XPL_UNAVAILABLE_ENUMERATOR(2, 42)
# define XPL_AVAILABLE_TYPE_IN_2_42            XPL_UNAVAILABLE_TYPE(2, 42)
#else
# define XPL_AVAILABLE_IN_2_42                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_42
# define XPL_AVAILABLE_ENUMERATOR_IN_2_42
# define XPL_AVAILABLE_TYPE_IN_2_42
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_44
# define XPL_DEPRECATED_IN_2_44                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_44_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_44          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_44_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_44          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_44_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_44           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_44_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_44                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_44_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_44
# define XPL_DEPRECATED_MACRO_IN_2_44_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_44
# define XPL_DEPRECATED_ENUMERATOR_IN_2_44_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_44
# define XPL_DEPRECATED_TYPE_IN_2_44_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_44
# define XPL_AVAILABLE_IN_2_44                 XPL_UNAVAILABLE(2, 44)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_44   XPL_UNAVAILABLE_STATIC_INLINE(2, 44)
# define XPL_AVAILABLE_MACRO_IN_2_44           XPL_UNAVAILABLE_MACRO(2, 44)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_44      XPL_UNAVAILABLE_ENUMERATOR(2, 44)
# define XPL_AVAILABLE_TYPE_IN_2_44            XPL_UNAVAILABLE_TYPE(2, 44)
#else
# define XPL_AVAILABLE_IN_2_44                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_44
# define XPL_AVAILABLE_MACRO_IN_2_44
# define XPL_AVAILABLE_ENUMERATOR_IN_2_44
# define XPL_AVAILABLE_TYPE_IN_2_44
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_46
# define XPL_DEPRECATED_IN_2_46                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_46_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_46          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_46_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_46                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_46_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_46
# define XPL_DEPRECATED_MACRO_IN_2_46_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_46
# define XPL_DEPRECATED_ENUMERATOR_IN_2_46_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_46
# define XPL_DEPRECATED_TYPE_IN_2_46_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_46
# define XPL_AVAILABLE_IN_2_46                 XPL_UNAVAILABLE(2, 46)
# define XPL_AVAILABLE_MACRO_IN_2_46           XPL_UNAVAILABLE_MACRO(2, 46)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_46      XPL_UNAVAILABLE_ENUMERATOR(2, 46)
# define XPL_AVAILABLE_TYPE_IN_2_46            XPL_UNAVAILABLE_TYPE(2, 46)
#else
# define XPL_AVAILABLE_IN_2_46                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_46
# define XPL_AVAILABLE_ENUMERATOR_IN_2_46
# define XPL_AVAILABLE_TYPE_IN_2_46
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_48
# define XPL_DEPRECATED_IN_2_48                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_48_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_48          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_48_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_48          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_48_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_48           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_48_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_48                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_48_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_48
# define XPL_DEPRECATED_MACRO_IN_2_48_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_48
# define XPL_DEPRECATED_ENUMERATOR_IN_2_48_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_48
# define XPL_DEPRECATED_TYPE_IN_2_48_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_48
# define XPL_AVAILABLE_IN_2_48                 XPL_UNAVAILABLE(2, 48)
# define XPL_AVAILABLE_MACRO_IN_2_48           XPL_UNAVAILABLE_MACRO(2, 48)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_48      XPL_UNAVAILABLE_ENUMERATOR(2, 48)
# define XPL_AVAILABLE_TYPE_IN_2_48            XPL_UNAVAILABLE_TYPE(2, 48)
#else
# define XPL_AVAILABLE_IN_2_48                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_48
# define XPL_AVAILABLE_ENUMERATOR_IN_2_48
# define XPL_AVAILABLE_TYPE_IN_2_48
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_50
# define XPL_DEPRECATED_IN_2_50                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_50_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_50          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_50_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_50          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_50_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_50           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_50_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_50                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_50_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_50
# define XPL_DEPRECATED_MACRO_IN_2_50_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_50
# define XPL_DEPRECATED_ENUMERATOR_IN_2_50_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_50
# define XPL_DEPRECATED_TYPE_IN_2_50_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_50
# define XPL_AVAILABLE_IN_2_50                 XPL_UNAVAILABLE(2, 50)
# define XPL_AVAILABLE_MACRO_IN_2_50           XPL_UNAVAILABLE_MACRO(2, 50)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_50      XPL_UNAVAILABLE_ENUMERATOR(2, 50)
# define XPL_AVAILABLE_TYPE_IN_2_50            XPL_UNAVAILABLE_TYPE(2, 50)
#else
# define XPL_AVAILABLE_IN_2_50                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_50
# define XPL_AVAILABLE_ENUMERATOR_IN_2_50
# define XPL_AVAILABLE_TYPE_IN_2_50
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_52
# define XPL_DEPRECATED_IN_2_52                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_52_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_52          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_52_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_52          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_52_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_52           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_52_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_52                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_52_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_52
# define XPL_DEPRECATED_MACRO_IN_2_52_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_52
# define XPL_DEPRECATED_ENUMERATOR_IN_2_52_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_52
# define XPL_DEPRECATED_TYPE_IN_2_52_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_52
# define XPL_AVAILABLE_IN_2_52                 XPL_UNAVAILABLE(2, 52)
# define XPL_AVAILABLE_MACRO_IN_2_52           XPL_UNAVAILABLE_MACRO(2, 52)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_52      XPL_UNAVAILABLE_ENUMERATOR(2, 52)
# define XPL_AVAILABLE_TYPE_IN_2_52            XPL_UNAVAILABLE_TYPE(2, 52)
#else
# define XPL_AVAILABLE_IN_2_52                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_52
# define XPL_AVAILABLE_ENUMERATOR_IN_2_52
# define XPL_AVAILABLE_TYPE_IN_2_52
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_54
# define XPL_DEPRECATED_IN_2_54                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_54_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_54          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_54_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_54          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_54_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_54           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_54_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_54                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_54_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_54
# define XPL_DEPRECATED_MACRO_IN_2_54_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_54
# define XPL_DEPRECATED_ENUMERATOR_IN_2_54_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_54
# define XPL_DEPRECATED_TYPE_IN_2_54_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_54
# define XPL_AVAILABLE_IN_2_54                 XPL_UNAVAILABLE(2, 54)
# define XPL_AVAILABLE_MACRO_IN_2_54           XPL_UNAVAILABLE_MACRO(2, 54)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_54      XPL_UNAVAILABLE_ENUMERATOR(2, 54)
# define XPL_AVAILABLE_TYPE_IN_2_54            XPL_UNAVAILABLE_TYPE(2, 54)
#else
# define XPL_AVAILABLE_IN_2_54                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_54
# define XPL_AVAILABLE_ENUMERATOR_IN_2_54
# define XPL_AVAILABLE_TYPE_IN_2_54
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_56
# define XPL_DEPRECATED_IN_2_56                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_56_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_56          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_56_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_56          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_56_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_56           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_56_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_56                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_56_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_56
# define XPL_DEPRECATED_MACRO_IN_2_56_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_56
# define XPL_DEPRECATED_ENUMERATOR_IN_2_56_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_56
# define XPL_DEPRECATED_TYPE_IN_2_56_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_56
# define XPL_AVAILABLE_IN_2_56                 XPL_UNAVAILABLE(2, 56)
# define XPL_AVAILABLE_MACRO_IN_2_56           XPL_UNAVAILABLE_MACRO(2, 56)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_56      XPL_UNAVAILABLE_ENUMERATOR(2, 56)
# define XPL_AVAILABLE_TYPE_IN_2_56            XPL_UNAVAILABLE_TYPE(2, 56)
#else
# define XPL_AVAILABLE_IN_2_56                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_56
# define XPL_AVAILABLE_ENUMERATOR_IN_2_56
# define XPL_AVAILABLE_TYPE_IN_2_56
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_58
# define XPL_DEPRECATED_IN_2_58                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_58_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_58          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_58_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_58          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_58_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_58           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_58_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_58                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_58_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_58
# define XPL_DEPRECATED_MACRO_IN_2_58_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_58
# define XPL_DEPRECATED_ENUMERATOR_IN_2_58_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_58
# define XPL_DEPRECATED_TYPE_IN_2_58_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_58
# define XPL_AVAILABLE_IN_2_58                 XPL_UNAVAILABLE(2, 58)
# define XPL_AVAILABLE_MACRO_IN_2_58           XPL_UNAVAILABLE_MACRO(2, 58)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_58      XPL_UNAVAILABLE_ENUMERATOR(2, 58)
# define XPL_AVAILABLE_TYPE_IN_2_58            XPL_UNAVAILABLE_TYPE(2, 58)
#else
# define XPL_AVAILABLE_IN_2_58                 _XPL_EXTERN
# define XPL_AVAILABLE_MACRO_IN_2_58
# define XPL_AVAILABLE_ENUMERATOR_IN_2_58
# define XPL_AVAILABLE_TYPE_IN_2_58
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_60
# define XPL_DEPRECATED_IN_2_60                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_60_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_60          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_60_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_60          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_60_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_60           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_60_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_60                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_60_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_60
# define XPL_DEPRECATED_MACRO_IN_2_60_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_60
# define XPL_DEPRECATED_ENUMERATOR_IN_2_60_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_60
# define XPL_DEPRECATED_TYPE_IN_2_60_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_60
# define XPL_AVAILABLE_IN_2_60                 XPL_UNAVAILABLE(2, 60)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_60   XPL_UNAVAILABLE_STATIC_INLINE(2, 60)
# define XPL_AVAILABLE_MACRO_IN_2_60           XPL_UNAVAILABLE_MACRO(2, 60)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_60      XPL_UNAVAILABLE_ENUMERATOR(2, 60)
# define XPL_AVAILABLE_TYPE_IN_2_60            XPL_UNAVAILABLE_TYPE(2, 60)
#else
# define XPL_AVAILABLE_IN_2_60                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_60
# define XPL_AVAILABLE_MACRO_IN_2_60
# define XPL_AVAILABLE_ENUMERATOR_IN_2_60
# define XPL_AVAILABLE_TYPE_IN_2_60
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_62
# define XPL_DEPRECATED_IN_2_62                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_62_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_62          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_62_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_62          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_62_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_62           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_62_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_62                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_62_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_62
# define XPL_DEPRECATED_MACRO_IN_2_62_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_62
# define XPL_DEPRECATED_ENUMERATOR_IN_2_62_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_62
# define XPL_DEPRECATED_TYPE_IN_2_62_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_62
# define XPL_AVAILABLE_IN_2_62                 XPL_UNAVAILABLE(2, 62)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_62   XPL_UNAVAILABLE_STATIC_INLINE(2, 62)
# define XPL_AVAILABLE_MACRO_IN_2_62           XPL_UNAVAILABLE_MACRO(2, 62)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_62      XPL_UNAVAILABLE_ENUMERATOR(2, 62)
# define XPL_AVAILABLE_TYPE_IN_2_62            XPL_UNAVAILABLE_TYPE(2, 62)
#else
# define XPL_AVAILABLE_IN_2_62                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_62
# define XPL_AVAILABLE_MACRO_IN_2_62
# define XPL_AVAILABLE_ENUMERATOR_IN_2_62
# define XPL_AVAILABLE_TYPE_IN_2_62
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_64
# define XPL_DEPRECATED_IN_2_64                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_64_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_64          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_64_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_64          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_64_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_64           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_64_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_64                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_64_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_64
# define XPL_DEPRECATED_MACRO_IN_2_64_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_64
# define XPL_DEPRECATED_ENUMERATOR_IN_2_64_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_64
# define XPL_DEPRECATED_TYPE_IN_2_64_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_64
# define XPL_AVAILABLE_IN_2_64                 XPL_UNAVAILABLE(2, 64)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_64   XPL_UNAVAILABLE_STATIC_INLINE(2, 64)
# define XPL_AVAILABLE_MACRO_IN_2_64           XPL_UNAVAILABLE_MACRO(2, 64)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_64      XPL_UNAVAILABLE_ENUMERATOR(2, 64)
# define XPL_AVAILABLE_TYPE_IN_2_64            XPL_UNAVAILABLE_TYPE(2, 64)
#else
# define XPL_AVAILABLE_IN_2_64                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_64
# define XPL_AVAILABLE_MACRO_IN_2_64
# define XPL_AVAILABLE_ENUMERATOR_IN_2_64
# define XPL_AVAILABLE_TYPE_IN_2_64
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_66
# define XPL_DEPRECATED_IN_2_66                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_66_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_66          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_66_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_66          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_66_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_66           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_66_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_66                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_66_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_66
# define XPL_DEPRECATED_MACRO_IN_2_66_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_66
# define XPL_DEPRECATED_ENUMERATOR_IN_2_66_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_66
# define XPL_DEPRECATED_TYPE_IN_2_66_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_66
# define XPL_AVAILABLE_IN_2_66                 XPL_UNAVAILABLE(2, 66)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_66   XPL_UNAVAILABLE_STATIC_INLINE(2, 66)
# define XPL_AVAILABLE_MACRO_IN_2_66           XPL_UNAVAILABLE_MACRO(2, 66)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_66      XPL_UNAVAILABLE_ENUMERATOR(2, 66)
# define XPL_AVAILABLE_TYPE_IN_2_66            XPL_UNAVAILABLE_TYPE(2, 66)
#else
# define XPL_AVAILABLE_IN_2_66                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_66
# define XPL_AVAILABLE_MACRO_IN_2_66
# define XPL_AVAILABLE_ENUMERATOR_IN_2_66
# define XPL_AVAILABLE_TYPE_IN_2_66
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_68
# define XPL_DEPRECATED_IN_2_68                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_68_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_68          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_68_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_68          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_68_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_68           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_68_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_68                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_68_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_68
# define XPL_DEPRECATED_MACRO_IN_2_68_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_68
# define XPL_DEPRECATED_ENUMERATOR_IN_2_68_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_68
# define XPL_DEPRECATED_TYPE_IN_2_68_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_68
# define XPL_AVAILABLE_IN_2_68                 XPL_UNAVAILABLE(2, 68)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_68   XPL_UNAVAILABLE_STATIC_INLINE(2, 68)
# define XPL_AVAILABLE_MACRO_IN_2_68           XPL_UNAVAILABLE_MACRO(2, 68)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_68      XPL_UNAVAILABLE_ENUMERATOR(2, 68)
# define XPL_AVAILABLE_TYPE_IN_2_68            XPL_UNAVAILABLE_TYPE(2, 68)
#else
# define XPL_AVAILABLE_IN_2_68                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_68
# define XPL_AVAILABLE_MACRO_IN_2_68
# define XPL_AVAILABLE_ENUMERATOR_IN_2_68
# define XPL_AVAILABLE_TYPE_IN_2_68
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_70
# define XPL_DEPRECATED_IN_2_70                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_70_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_70          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_70_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_70          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_70_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_70           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_70_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_70                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_70_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_70
# define XPL_DEPRECATED_MACRO_IN_2_70_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_70
# define XPL_DEPRECATED_ENUMERATOR_IN_2_70_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_70
# define XPL_DEPRECATED_TYPE_IN_2_70_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_70
# define XPL_AVAILABLE_IN_2_70                 XPL_UNAVAILABLE(2, 70)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_70   XPL_UNAVAILABLE_STATIC_INLINE(2, 70)
# define XPL_AVAILABLE_MACRO_IN_2_70           XPL_UNAVAILABLE_MACRO(2, 70)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_70      XPL_UNAVAILABLE_ENUMERATOR(2, 70)
# define XPL_AVAILABLE_TYPE_IN_2_70            XPL_UNAVAILABLE_TYPE(2, 70)
#else
# define XPL_AVAILABLE_IN_2_70                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_70
# define XPL_AVAILABLE_MACRO_IN_2_70
# define XPL_AVAILABLE_ENUMERATOR_IN_2_70
# define XPL_AVAILABLE_TYPE_IN_2_70
#endif

#if XPL_VERSION_MIN_REQUIRED >= XPL_VERSION_2_72
# define XPL_DEPRECATED_IN_2_72                XPL_DEPRECATED
# define XPL_DEPRECATED_IN_2_72_FOR(f)         XPL_DEPRECATED_FOR(f)
# define XPL_DEPRECATED_MACRO_IN_2_72          XPL_DEPRECATED_MACRO
# define XPL_DEPRECATED_MACRO_IN_2_72_FOR(f)   XPL_DEPRECATED_MACRO_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_72          XPL_DEPRECATED_ENUMERATOR
# define XPL_DEPRECATED_ENUMERATOR_IN_2_72_FOR(f)   XPL_DEPRECATED_ENUMERATOR_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_72           XPL_DEPRECATED_TYPE
# define XPL_DEPRECATED_TYPE_IN_2_72_FOR(f)    XPL_DEPRECATED_TYPE_FOR(f)
#else
# define XPL_DEPRECATED_IN_2_72                _XPL_EXTERN
# define XPL_DEPRECATED_IN_2_72_FOR(f)         _XPL_EXTERN
# define XPL_DEPRECATED_MACRO_IN_2_72
# define XPL_DEPRECATED_MACRO_IN_2_72_FOR(f)
# define XPL_DEPRECATED_ENUMERATOR_IN_2_72
# define XPL_DEPRECATED_ENUMERATOR_IN_2_72_FOR(f)
# define XPL_DEPRECATED_TYPE_IN_2_72
# define XPL_DEPRECATED_TYPE_IN_2_72_FOR(f)
#endif

#if XPL_VERSION_MAX_ALLOWED < XPL_VERSION_2_72
# define XPL_AVAILABLE_IN_2_72                 XPL_UNAVAILABLE(2, 72)
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_72   XPL_UNAVAILABLE_STATIC_INLINE(2, 72)
# define XPL_AVAILABLE_MACRO_IN_2_72           XPL_UNAVAILABLE_MACRO(2, 72)
# define XPL_AVAILABLE_ENUMERATOR_IN_2_72      XPL_UNAVAILABLE_ENUMERATOR(2, 72)
# define XPL_AVAILABLE_TYPE_IN_2_72            XPL_UNAVAILABLE_TYPE(2, 72)
#else
# define XPL_AVAILABLE_IN_2_72                 _XPL_EXTERN
# define XPL_AVAILABLE_STATIC_INLINE_IN_2_72
# define XPL_AVAILABLE_MACRO_IN_2_72
# define XPL_AVAILABLE_ENUMERATOR_IN_2_72
# define XPL_AVAILABLE_TYPE_IN_2_72
#endif

#endif /*  __G_VERSION_MACROS_H__ */
