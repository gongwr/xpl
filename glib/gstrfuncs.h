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

#ifndef __G_STRFUNCS_H__
#define __G_STRFUNCS_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <stdarg.h>
#include <glib/gmacros.h>
#include <glib/gtypes.h>
#include <glib/gerror.h>

G_BEGIN_DECLS

/* Functions like the ones in <ctype.h> that are not affected by locale. */
typedef enum {
  G_ASCII_ALNUM  = 1 << 0,
  G_ASCII_ALPHA  = 1 << 1,
  G_ASCII_CNTRL  = 1 << 2,
  G_ASCII_DIGIT  = 1 << 3,
  G_ASCII_GRAPH  = 1 << 4,
  G_ASCII_LOWER  = 1 << 5,
  G_ASCII_PRINT  = 1 << 6,
  G_ASCII_PUNCT  = 1 << 7,
  G_ASCII_SPACE  = 1 << 8,
  G_ASCII_UPPER  = 1 << 9,
  G_ASCII_XDIGIT = 1 << 10
} GAsciiType;

XPL_VAR const guint16 * const g_ascii_table;

#define g_ascii_isalnum(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_ALNUM) != 0)

#define g_ascii_isalpha(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_ALPHA) != 0)

#define g_ascii_iscntrl(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_CNTRL) != 0)

#define g_ascii_isdigit(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_DIGIT) != 0)

#define g_ascii_isgraph(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_GRAPH) != 0)

#define g_ascii_islower(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_LOWER) != 0)

#define g_ascii_isprint(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_PRINT) != 0)

#define g_ascii_ispunct(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_PUNCT) != 0)

#define g_ascii_isspace(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_SPACE) != 0)

#define g_ascii_isupper(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_UPPER) != 0)

#define g_ascii_isxdigit(c) \
  ((g_ascii_table[(guchar) (c)] & G_ASCII_XDIGIT) != 0)

XPL_AVAILABLE_IN_ALL
xchar_t                 g_ascii_tolower  (xchar_t        c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xchar_t                 g_ascii_toupper  (xchar_t        c) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xint_t                  g_ascii_digit_value  (xchar_t    c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xint_t                  g_ascii_xdigit_value (xchar_t    c) G_GNUC_CONST;

/* String utility functions that modify a string argument or
 * return a constant string that must not be freed.
 */
#define	 G_STR_DELIMITERS	"_-|> <."
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strdelimit     (xchar_t	     *string,
					const xchar_t  *delimiters,
					xchar_t	      new_delimiter);
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strcanon       (xchar_t        *string,
					const xchar_t  *valid_chars,
					xchar_t         substitutor);
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_strerror       (xint_t	      errnum) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_strsignal      (xint_t	      signum) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xchar_t *	              g_strreverse     (xchar_t	     *string);
XPL_AVAILABLE_IN_ALL
xsize_t	              g_strlcpy	       (xchar_t	     *dest,
					const xchar_t  *src,
					xsize_t         dest_size);
XPL_AVAILABLE_IN_ALL
xsize_t	              g_strlcat        (xchar_t	     *dest,
					const xchar_t  *src,
					xsize_t         dest_size);
XPL_AVAILABLE_IN_ALL
xchar_t *               g_strstr_len     (const xchar_t  *haystack,
					gssize        haystack_len,
					const xchar_t  *needle);
XPL_AVAILABLE_IN_ALL
xchar_t *               g_strrstr        (const xchar_t  *haystack,
					const xchar_t  *needle);
XPL_AVAILABLE_IN_ALL
xchar_t *               g_strrstr_len    (const xchar_t  *haystack,
					gssize        haystack_len,
					const xchar_t  *needle);

XPL_AVAILABLE_IN_ALL
xboolean_t              g_str_has_suffix (const xchar_t  *str,
					const xchar_t  *suffix);
XPL_AVAILABLE_IN_ALL
xboolean_t              g_str_has_prefix (const xchar_t  *str,
					const xchar_t  *prefix);

/* String to/from double conversion functions */

XPL_AVAILABLE_IN_ALL
xdouble_t	              g_strtod         (const xchar_t  *nptr,
					xchar_t	    **endptr);
XPL_AVAILABLE_IN_ALL
xdouble_t	              g_ascii_strtod   (const xchar_t  *nptr,
					xchar_t	    **endptr);
XPL_AVAILABLE_IN_ALL
guint64		      g_ascii_strtoull (const xchar_t *nptr,
					xchar_t      **endptr,
					xuint_t        base);
XPL_AVAILABLE_IN_ALL
gint64		      g_ascii_strtoll  (const xchar_t *nptr,
					xchar_t      **endptr,
					xuint_t        base);
/* 29 bytes should enough for all possible values that
 * g_ascii_dtostr can produce.
 * Then add 10 for good measure */
#define G_ASCII_DTOSTR_BUF_SIZE (29 + 10)
XPL_AVAILABLE_IN_ALL
xchar_t *               g_ascii_dtostr   (xchar_t        *buffer,
					xint_t          buf_len,
					xdouble_t       d);
XPL_AVAILABLE_IN_ALL
xchar_t *               g_ascii_formatd  (xchar_t        *buffer,
					xint_t          buf_len,
					const xchar_t  *format,
					xdouble_t       d);

/* removes leading spaces */
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strchug        (xchar_t        *string);
/* removes trailing spaces */
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strchomp       (xchar_t        *string);
/* removes leading & trailing spaces */
#define g_strstrip( string )	g_strchomp (g_strchug (string))

XPL_AVAILABLE_IN_ALL
xint_t                  g_ascii_strcasecmp  (const xchar_t *s1,
					   const xchar_t *s2);
XPL_AVAILABLE_IN_ALL
xint_t                  g_ascii_strncasecmp (const xchar_t *s1,
					   const xchar_t *s2,
					   xsize_t        n);
XPL_AVAILABLE_IN_ALL
xchar_t*                g_ascii_strdown     (const xchar_t *str,
					   gssize       len) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*                g_ascii_strup       (const xchar_t *str,
					   gssize       len) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_2_40
xboolean_t              g_str_is_ascii      (const xchar_t *str);

XPL_DEPRECATED
xint_t                  g_strcasecmp     (const xchar_t *s1,
                                        const xchar_t *s2);
XPL_DEPRECATED
xint_t                  g_strncasecmp    (const xchar_t *s1,
                                        const xchar_t *s2,
                                        xuint_t        n);
XPL_DEPRECATED
xchar_t*                g_strdown        (xchar_t       *string);
XPL_DEPRECATED
xchar_t*                g_strup          (xchar_t       *string);


/* String utility functions that return a newly allocated string which
 * ought to be freed with g_free from the caller at some point.
 */
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strdup	       (const xchar_t *str) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strdup_printf  (const xchar_t *format,
					...) G_GNUC_PRINTF (1, 2) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strdup_vprintf (const xchar_t *format,
					va_list      args) G_GNUC_PRINTF(1, 0) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strndup	       (const xchar_t *str,
					xsize_t        n) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strnfill       (xsize_t        length,
					xchar_t        fill_char) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*	              g_strconcat      (const xchar_t *string1,
					...) G_GNUC_MALLOC G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strjoin	       (const xchar_t  *separator,
					...) G_GNUC_MALLOC G_GNUC_NULL_TERMINATED;

/* Make a copy of a string interpreting C string -style escape
 * sequences. Inverse of g_strescape. The recognized sequences are \b
 * \f \n \r \t \\ \" and the octal format.
 */
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strcompress    (const xchar_t *source) G_GNUC_MALLOC;

/* Copy a string escaping nonprintable characters like in C strings.
 * Inverse of g_strcompress. The exceptions parameter, if non-NULL, points
 * to a string containing characters that are not to be escaped.
 *
 * Deprecated API: xchar_t* g_strescape (const xchar_t *source);
 * Luckily this function wasn't used much, using NULL as second parameter
 * provides mostly identical semantics.
 */
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strescape      (const xchar_t *source,
					const xchar_t *exceptions) G_GNUC_MALLOC;

XPL_DEPRECATED_IN_2_68_FOR (g_memdup2)
xpointer_t              g_memdup         (gconstpointer mem,
                                        xuint_t         byte_size) G_GNUC_ALLOC_SIZE(2);

XPL_AVAILABLE_IN_2_68
xpointer_t              g_memdup2        (gconstpointer mem,
                                        xsize_t         byte_size) G_GNUC_ALLOC_SIZE(2);

/* NULL terminated string arrays.
 * g_strsplit(), g_strsplit_set() split up string into max_tokens tokens
 * at delim and return a newly allocated string array.
 * g_strjoinv() concatenates all of str_array's strings, sliding in an
 * optional separator, the returned string is newly allocated.
 * g_strfreev() frees the array itself and all of its strings.
 * g_strdupv() copies a NULL-terminated array of strings
 * g_strv_length() returns the length of a NULL-terminated array of strings
 */
typedef xchar_t** GStrv;
XPL_AVAILABLE_IN_ALL
xchar_t**	              g_strsplit       (const xchar_t  *string,
					const xchar_t  *delimiter,
					xint_t          max_tokens);
XPL_AVAILABLE_IN_ALL
xchar_t **	      g_strsplit_set   (const xchar_t *string,
					const xchar_t *delimiters,
					xint_t         max_tokens);
XPL_AVAILABLE_IN_ALL
xchar_t*                g_strjoinv       (const xchar_t  *separator,
					xchar_t       **str_array) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
void                  g_strfreev       (xchar_t       **str_array);
XPL_AVAILABLE_IN_ALL
xchar_t**               g_strdupv        (xchar_t       **str_array);
XPL_AVAILABLE_IN_ALL
xuint_t                 g_strv_length    (xchar_t       **str_array);

XPL_AVAILABLE_IN_ALL
xchar_t*                g_stpcpy         (xchar_t        *dest,
                                        const char   *src);

XPL_AVAILABLE_IN_2_40
xchar_t *                 g_str_to_ascii                                  (const xchar_t   *str,
                                                                         const xchar_t   *from_locale);

XPL_AVAILABLE_IN_2_40
xchar_t **                g_str_tokenize_and_fold                         (const xchar_t   *string,
                                                                         const xchar_t   *translit_locale,
                                                                         xchar_t       ***ascii_alternates);

XPL_AVAILABLE_IN_2_40
xboolean_t                g_str_match_string                              (const xchar_t   *search_term,
                                                                         const xchar_t   *potential_hit,
                                                                         xboolean_t       accept_alternates);

XPL_AVAILABLE_IN_2_44
xboolean_t              g_strv_contains  (const xchar_t * const *strv,
                                        const xchar_t         *str);

XPL_AVAILABLE_IN_2_60
xboolean_t              g_strv_equal     (const xchar_t * const *strv1,
                                        const xchar_t * const *strv2);

/* Convenience ASCII string to number API */

/**
 * GNumberParserError:
 * @G_NUMBER_PARSER_ERROR_INVALID: String was not a valid number.
 * @G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS: String was a number, but out of bounds.
 *
 * Error codes returned by functions converting a string to a number.
 *
 * Since: 2.54
 */
typedef enum
  {
    G_NUMBER_PARSER_ERROR_INVALID,
    G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS,
  } GNumberParserError;

/**
 * G_NUMBER_PARSER_ERROR:
 *
 * Domain for errors returned by functions converting a string to a
 * number.
 *
 * Since: 2.54
 */
#define G_NUMBER_PARSER_ERROR (g_number_parser_error_quark ())

XPL_AVAILABLE_IN_2_54
GQuark                g_number_parser_error_quark  (void);

XPL_AVAILABLE_IN_2_54
xboolean_t              g_ascii_string_to_signed     (const xchar_t  *str,
                                                    xuint_t         base,
                                                    gint64        min,
                                                    gint64        max,
                                                    gint64       *out_num,
                                                    xerror_t      **error);

XPL_AVAILABLE_IN_2_54
xboolean_t              g_ascii_string_to_unsigned   (const xchar_t  *str,
                                                    xuint_t         base,
                                                    guint64       min,
                                                    guint64       max,
                                                    guint64      *out_num,
                                                    xerror_t      **error);

G_END_DECLS

#endif /* __G_STRFUNCS_H__ */
