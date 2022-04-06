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

/*
 * MT safe
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <garray.h>
#include <ctype.h>              /* For tolower() */

#ifdef HAVE_XLOCALE_H
/* Needed on BSD/OS X for e.g. strtod_l */
#include <xlocale.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

/* do not include <unistd.h> here, it may interfere with xstrsignal() */

#include "gstrfuncs.h"

#include "gprintf.h"
#include "gprintfint.h"
#include "glibintl.h"


/**
 * SECTION:string_utils
 * @title: String Utility Functions
 * @short_description: various string-related functions
 *
 * This section describes a number of utility functions for creating,
 * duplicating, and manipulating strings.
 *
 * Note that the functions g_printf(), g_fprintf(), g_sprintf(),
 * g_vprintf(), g_vfprintf(), g_vsprintf() and g_vasprintf()
 * are declared in the header `gprintf.h` which is not included in `glib.h`
 * (otherwise using `glib.h` would drag in `stdio.h`), so you'll have to
 * explicitly include `<glib/gprintf.h>` in order to use the GLib
 * printf() functions.
 *
 * ## String precision pitfalls # {#string-precision}
 *
 * While you may use the printf() functions to format UTF-8 strings,
 * notice that the precision of a \%Ns parameter is interpreted
 * as the number of bytes, not characters to print. On top of that,
 * the GNU libc implementation of the printf() functions has the
 * "feature" that it checks that the string given for the \%Ns
 * parameter consists of a whole number of characters in the current
 * encoding. So, unless you are sure you are always going to be in an
 * UTF-8 locale or your know your text is restricted to ASCII, avoid
 * using \%Ns. If your intention is to format strings for a
 * certain number of columns, then \%Ns is not a correct solution
 * anyway, since it fails to take wide characters (see xunichar_iswide())
 * into account.
 *
 * Note also that there are various printf() parameters which are platform
 * dependent. GLib provides platform independent macros for these parameters
 * which should be used instead. A common example is %G_GUINT64_FORMAT, which
 * should be used instead of `%llu` or similar parameters for formatting
 * 64-bit integers. These macros are all named `G_*_FORMAT`; see
 * [Basic Types][glib-Basic-Types].
 */

/**
 * g_ascii_isalnum:
 * @c: any character
 *
 * Determines whether a character is alphanumeric.
 *
 * Unlike the standard C library isalnum() function, this only
 * recognizes standard ASCII letters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to cast to #guchar before
 * passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII alphanumeric character
 */

/**
 * g_ascii_isalpha:
 * @c: any character
 *
 * Determines whether a character is alphabetic (i.e. a letter).
 *
 * Unlike the standard C library isalpha() function, this only
 * recognizes standard ASCII letters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to cast to #guchar before
 * passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII alphabetic character
 */

/**
 * g_ascii_iscntrl:
 * @c: any character
 *
 * Determines whether a character is a control character.
 *
 * Unlike the standard C library iscntrl() function, this only
 * recognizes standard ASCII control characters and ignores the
 * locale, returning %FALSE for all non-ASCII characters. Also,
 * unlike the standard library function, this takes a char, not
 * an int, so don't call it on %EOF, but no need to cast to #guchar
 * before passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII control character.
 */

/**
 * g_ascii_isdigit:
 * @c: any character
 *
 * Determines whether a character is digit (0-9).
 *
 * Unlike the standard C library isdigit() function, this takes
 * a char, not an int, so don't call it  on %EOF, but no need to
 * cast to #guchar before passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII digit.
 */

/**
 * g_ascii_isgraph:
 * @c: any character
 *
 * Determines whether a character is a printing character and not a space.
 *
 * Unlike the standard C library isgraph() function, this only
 * recognizes standard ASCII characters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to cast to #guchar before
 * passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII printing character other than space.
 */

/**
 * g_ascii_islower:
 * @c: any character
 *
 * Determines whether a character is an ASCII lower case letter.
 *
 * Unlike the standard C library islower() function, this only
 * recognizes standard ASCII letters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to worry about casting
 * to #guchar before passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII lower case letter
 */

/**
 * g_ascii_isprint:
 * @c: any character
 *
 * Determines whether a character is a printing character.
 *
 * Unlike the standard C library isprint() function, this only
 * recognizes standard ASCII characters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to cast to #guchar before
 * passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII printing character.
 */

/**
 * g_ascii_ispunct:
 * @c: any character
 *
 * Determines whether a character is a punctuation character.
 *
 * Unlike the standard C library ispunct() function, this only
 * recognizes standard ASCII letters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to cast to #guchar before
 * passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII punctuation character.
 */

/**
 * g_ascii_isspace:
 * @c: any character
 *
 * Determines whether a character is a white-space character.
 *
 * Unlike the standard C library isspace() function, this only
 * recognizes standard ASCII white-space and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to cast to #guchar before
 * passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII white-space character
 */

/**
 * g_ascii_isupper:
 * @c: any character
 *
 * Determines whether a character is an ASCII upper case letter.
 *
 * Unlike the standard C library isupper() function, this only
 * recognizes standard ASCII letters and ignores the locale,
 * returning %FALSE for all non-ASCII characters. Also, unlike
 * the standard library function, this takes a char, not an int,
 * so don't call it on %EOF, but no need to worry about casting
 * to #guchar before passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII upper case letter
 */

/**
 * g_ascii_isxdigit:
 * @c: any character
 *
 * Determines whether a character is a hexadecimal-digit character.
 *
 * Unlike the standard C library isxdigit() function, this takes
 * a char, not an int, so don't call it on %EOF, but no need to
 * cast to #guchar before passing a possibly non-ASCII character in.
 *
 * Returns: %TRUE if @c is an ASCII hexadecimal-digit character.
 */

/**
 * G_ASCII_DTOSTR_BUF_SIZE:
 *
 * A good size for a buffer to be passed into g_ascii_dtostr().
 * It is guaranteed to be enough for all output of that function
 * on systems with 64bit IEEE-compatible doubles.
 *
 * The typical usage would be something like:
 * |[<!-- language="C" -->
 *   char buf[G_ASCII_DTOSTR_BUF_SIZE];
 *
 *   fprintf (out, "value=%s\n", g_ascii_dtostr (buf, sizeof (buf), value));
 * ]|
 */

/**
 * xstrstrip:
 * @string: a string to remove the leading and trailing whitespace from
 *
 * Removes leading and trailing whitespace from a string.
 * See xstrchomp() and xstrchug().
 *
 * Returns: @string
 */

/**
 * G_STR_DELIMITERS:
 *
 * The standard delimiters, used in xstrdelimit().
 */

static const xuint16_t ascii_table_data[256] = {
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x004, 0x104, 0x104, 0x004, 0x104, 0x104, 0x004, 0x004,
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459,
  0x459, 0x459, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253,
  0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
  0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
  0x253, 0x253, 0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073,
  0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
  0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
  0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004
  /* the upper 128 are all zeroes */
};

const xuint16_t * const g_ascii_table = ascii_table_data;

#if defined(HAVE_NEWLOCALE) && \
    defined(HAVE_USELOCALE)
#define USE_XLOCALE 1
#endif

#ifdef USE_XLOCALE
static locale_t
get_C_locale (void)
{
  static xsize_t initialized = FALSE;
  static locale_t C_locale = NULL;

  if (g_once_init_enter (&initialized))
    {
      C_locale = newlocale (LC_ALL_MASK, "C", NULL);
      g_once_init_leave (&initialized, TRUE);
    }

  return C_locale;
}
#endif

/**
 * xstrdup:
 * @str: (nullable): the string to duplicate
 *
 * Duplicates a string. If @str is %NULL it returns %NULL.
 * The returned string should be freed with g_free()
 * when no longer needed.
 *
 * Returns: a newly-allocated copy of @str
 */
xchar_t*
xstrdup (const xchar_t *str)
{
  xchar_t *new_str;
  xsize_t length;

  if (str)
    {
      length = strlen (str) + 1;
      new_str = g_new (char, length);
      memcpy (new_str, str, length);
    }
  else
    new_str = NULL;

  return new_str;
}

/**
 * g_memdup:
 * @mem: the memory to copy.
 * @byte_size: the number of bytes to copy.
 *
 * Allocates @byte_size bytes of memory, and copies @byte_size bytes into it
 * from @mem. If @mem is %NULL it returns %NULL.
 *
 * Returns: a pointer to the newly-allocated copy of the memory, or %NULL if @mem
 *  is %NULL.
 * Deprecated: 2.68: Use g_memdup2() instead, as it accepts a #xsize_t argument
 *     for @byte_size, avoiding the possibility of overflow in a #xsize_t → #xuint_t
 *     conversion
 */
xpointer_t
g_memdup (xconstpointer mem,
          xuint_t         byte_size)
{
  xpointer_t new_mem;

  if (mem && byte_size != 0)
    {
      new_mem = g_malloc (byte_size);
      memcpy (new_mem, mem, byte_size);
    }
  else
    new_mem = NULL;

  return new_mem;
}

/**
 * g_memdup2:
 * @mem: (nullable): the memory to copy.
 * @byte_size: the number of bytes to copy.
 *
 * Allocates @byte_size bytes of memory, and copies @byte_size bytes into it
 * from @mem. If @mem is %NULL it returns %NULL.
 *
 * This replaces g_memdup(), which was prone to integer overflows when
 * converting the argument from a #xsize_t to a #xuint_t.
 *
 * Returns: (nullable): a pointer to the newly-allocated copy of the memory,
 *    or %NULL if @mem is %NULL.
 * Since: 2.68
 */
xpointer_t
g_memdup2 (xconstpointer mem,
           xsize_t         byte_size)
{
  xpointer_t new_mem;

  if (mem && byte_size != 0)
    {
      new_mem = g_malloc (byte_size);
      memcpy (new_mem, mem, byte_size);
    }
  else
    new_mem = NULL;

  return new_mem;
}

/**
 * xstrndup:
 * @str: the string to duplicate
 * @n: the maximum number of bytes to copy from @str
 *
 * Duplicates the first @n bytes of a string, returning a newly-allocated
 * buffer @n + 1 bytes long which will always be nul-terminated. If @str
 * is less than @n bytes long the buffer is padded with nuls. If @str is
 * %NULL it returns %NULL. The returned value should be freed when no longer
 * needed.
 *
 * To copy a number of characters from a UTF-8 encoded string,
 * use xutf8_strncpy() instead.
 *
 * Returns: a newly-allocated buffer containing the first @n bytes
 *     of @str, nul-terminated
 */
xchar_t*
xstrndup (const xchar_t *str,
           xsize_t        n)
{
  xchar_t *new_str;

  if (str)
    {
      new_str = g_new (xchar_t, n + 1);
      strncpy (new_str, str, n);
      new_str[n] = '\0';
    }
  else
    new_str = NULL;

  return new_str;
}

/**
 * xstrnfill:
 * @length: the length of the new string
 * @fill_char: the byte to fill the string with
 *
 * Creates a new string @length bytes long filled with @fill_char.
 * The returned string should be freed when no longer needed.
 *
 * Returns: a newly-allocated string filled the @fill_char
 */
xchar_t*
xstrnfill (xsize_t length,
            xchar_t fill_char)
{
  xchar_t *str;

  str = g_new (xchar_t, length + 1);
  memset (str, (guchar)fill_char, length);
  str[length] = '\0';

  return str;
}

/**
 * g_stpcpy:
 * @dest: destination buffer.
 * @src: source string.
 *
 * Copies a nul-terminated string into the dest buffer, include the
 * trailing nul, and return a pointer to the trailing nul byte.
 * This is useful for concatenating multiple strings together
 * without having to repeatedly scan for the end.
 *
 * Returns: a pointer to trailing nul byte.
 **/
xchar_t *
g_stpcpy (xchar_t       *dest,
          const xchar_t *src)
{
#ifdef HAVE_STPCPY
  g_return_val_if_fail (dest != NULL, NULL);
  g_return_val_if_fail (src != NULL, NULL);
  return stpcpy (dest, src);
#else
  xchar_t *d = dest;
  const xchar_t *s = src;

  g_return_val_if_fail (dest != NULL, NULL);
  g_return_val_if_fail (src != NULL, NULL);
  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
#endif
}

/**
 * xstrdup_vprintf:
 * @format: (not nullable): a standard printf() format string, but notice
 *     [string precision pitfalls][string-precision]
 * @args: the list of parameters to insert into the format string
 *
 * Similar to the standard C vsprintf() function but safer, since it
 * calculates the maximum space required and allocates memory to hold
 * the result. The returned string should be freed with g_free() when
 * no longer needed.
 *
 * The returned string is guaranteed to be non-NULL, unless @format
 * contains `%lc` or `%ls` conversions, which can fail if no multibyte
 * representation is available for the given character.
 *
 * See also g_vasprintf(), which offers the same functionality, but
 * additionally returns the length of the allocated string.
 *
 * Returns: a newly-allocated string holding the result
 */
xchar_t*
xstrdup_vprintf (const xchar_t *format,
                  va_list      args)
{
  xchar_t *string = NULL;

  g_vasprintf (&string, format, args);

  return string;
}

/**
 * xstrdup_printf:
 * @format: (not nullable): a standard printf() format string, but notice
 *     [string precision pitfalls][string-precision]
 * @...: the parameters to insert into the format string
 *
 * Similar to the standard C sprintf() function but safer, since it
 * calculates the maximum space required and allocates memory to hold
 * the result. The returned string should be freed with g_free() when no
 * longer needed.
 *
 * The returned string is guaranteed to be non-NULL, unless @format
 * contains `%lc` or `%ls` conversions, which can fail if no multibyte
 * representation is available for the given character.
 *
 * Returns: a newly-allocated string holding the result
 */
xchar_t*
xstrdup_printf (const xchar_t *format,
                 ...)
{
  xchar_t *buffer;
  va_list args;

  va_start (args, format);
  buffer = xstrdup_vprintf (format, args);
  va_end (args);

  return buffer;
}

/**
 * xstrconcat:
 * @string1: the first string to add, which must not be %NULL
 * @...: a %NULL-terminated list of strings to append to the string
 *
 * Concatenates all of the given strings into one long string. The
 * returned string should be freed with g_free() when no longer needed.
 *
 * The variable argument list must end with %NULL. If you forget the %NULL,
 * xstrconcat() will start appending random memory junk to your string.
 *
 * Note that this function is usually not the right function to use to
 * assemble a translated message from pieces, since proper translation
 * often requires the pieces to be reordered.
 *
 * Returns: a newly-allocated string containing all the string arguments
 */
xchar_t*
xstrconcat (const xchar_t *string1, ...)
{
  xsize_t   l;
  va_list args;
  xchar_t   *s;
  xchar_t   *concat;
  xchar_t   *ptr;

  if (!string1)
    return NULL;

  l = 1 + strlen (string1);
  va_start (args, string1);
  s = va_arg (args, xchar_t*);
  while (s)
    {
      l += strlen (s);
      s = va_arg (args, xchar_t*);
    }
  va_end (args);

  concat = g_new (xchar_t, l);
  ptr = concat;

  ptr = g_stpcpy (ptr, string1);
  va_start (args, string1);
  s = va_arg (args, xchar_t*);
  while (s)
    {
      ptr = g_stpcpy (ptr, s);
      s = va_arg (args, xchar_t*);
    }
  va_end (args);

  return concat;
}

/**
 * xstrtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  (out) (transfer none) (optional): if non-%NULL, it returns the
 *           character after the last character used in the conversion.
 *
 * Converts a string to a #xdouble_t value.
 * It calls the standard strtod() function to handle the conversion, but
 * if the string is not completely converted it attempts the conversion
 * again with g_ascii_strtod(), and returns the best match.
 *
 * This function should seldom be used. The normal situation when reading
 * numbers not for human consumption is to use g_ascii_strtod(). Only when
 * you know that you must expect both locale formatted and C formatted numbers
 * should you use this. Make sure that you don't pass strings such as comma
 * separated lists of values, since the commas may be interpreted as a decimal
 * point in some locales, causing unexpected results.
 *
 * Returns: the #xdouble_t value.
 **/
xdouble_t
xstrtod (const xchar_t *nptr,
          xchar_t      **endptr)
{
  xchar_t *fail_pos_1;
  xchar_t *fail_pos_2;
  xdouble_t val_1;
  xdouble_t val_2 = 0;

  g_return_val_if_fail (nptr != NULL, 0);

  fail_pos_1 = NULL;
  fail_pos_2 = NULL;

  val_1 = strtod (nptr, &fail_pos_1);

  if (fail_pos_1 && fail_pos_1[0] != 0)
    val_2 = g_ascii_strtod (nptr, &fail_pos_2);

  if (!fail_pos_1 || fail_pos_1[0] == 0 || fail_pos_1 >= fail_pos_2)
    {
      if (endptr)
        *endptr = fail_pos_1;
      return val_1;
    }
  else
    {
      if (endptr)
        *endptr = fail_pos_2;
      return val_2;
    }
}

/**
 * g_ascii_strtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  (out) (transfer none) (optional): if non-%NULL, it returns the
 *           character after the last character used in the conversion.
 *
 * Converts a string to a #xdouble_t value.
 *
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually changing
 * the current locale, since that would not be thread-safe.
 * A limitation of the implementation is that this function
 * will still accept localized versions of infinities and NANs.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtod() function.
 *
 * To convert from a #xdouble_t to a string in a locale-insensitive
 * way, use g_ascii_dtostr().
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 *
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Returns: the #xdouble_t value.
 */
xdouble_t
g_ascii_strtod (const xchar_t *nptr,
                xchar_t      **endptr)
{
#if defined(USE_XLOCALE) && defined(HAVE_STRTOD_L)

  g_return_val_if_fail (nptr != NULL, 0);

  errno = 0;

  return strtod_l (nptr, endptr, get_C_locale ());

#else

  xchar_t *fail_pos;
  xdouble_t val;
#ifndef __BIONIC__
  struct lconv *locale_data;
#endif
  const char *decimal_point;
  xsize_t decimal_point_len;
  const char *p, *decimal_point_pos;
  const char *end = NULL; /* Silence gcc */
  int strtod_errno;

  g_return_val_if_fail (nptr != NULL, 0);

  fail_pos = NULL;

#ifndef __BIONIC__
  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);
#else
  decimal_point = ".";
  decimal_point_len = 1;
#endif

  g_assert (decimal_point_len != 0);

  decimal_point_pos = NULL;
  end = NULL;

  if (decimal_point[0] != '.' ||
      decimal_point[1] != 0)
    {
      p = nptr;
      /* Skip leading space */
      while (g_ascii_isspace (*p))
        p++;

      /* Skip leading optional sign */
      if (*p == '+' || *p == '-')
        p++;

      if (p[0] == '0' &&
          (p[1] == 'x' || p[1] == 'X'))
        {
          p += 2;
          /* HEX - find the (optional) decimal point */

          while (g_ascii_isxdigit (*p))
            p++;

          if (*p == '.')
            decimal_point_pos = p++;

          while (g_ascii_isxdigit (*p))
            p++;

          if (*p == 'p' || *p == 'P')
            p++;
          if (*p == '+' || *p == '-')
            p++;
          while (g_ascii_isdigit (*p))
            p++;

          end = p;
        }
      else if (g_ascii_isdigit (*p) || *p == '.')
        {
          while (g_ascii_isdigit (*p))
            p++;

          if (*p == '.')
            decimal_point_pos = p++;

          while (g_ascii_isdigit (*p))
            p++;

          if (*p == 'e' || *p == 'E')
            p++;
          if (*p == '+' || *p == '-')
            p++;
          while (g_ascii_isdigit (*p))
            p++;

          end = p;
        }
      /* For the other cases, we need not convert the decimal point */
    }

  if (decimal_point_pos)
    {
      char *copy, *c;

      /* We need to convert the '.' to the locale specific decimal point */
      copy = g_malloc (end - nptr + 1 + decimal_point_len);

      c = copy;
      memcpy (c, nptr, decimal_point_pos - nptr);
      c += decimal_point_pos - nptr;
      memcpy (c, decimal_point, decimal_point_len);
      c += decimal_point_len;
      memcpy (c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
      c += end - (decimal_point_pos + 1);
      *c = 0;

      errno = 0;
      val = strtod (copy, &fail_pos);
      strtod_errno = errno;

      if (fail_pos)
        {
          if (fail_pos - copy > decimal_point_pos - nptr)
            fail_pos = (char *)nptr + (fail_pos - copy) - (decimal_point_len - 1);
          else
            fail_pos = (char *)nptr + (fail_pos - copy);
        }

      g_free (copy);

    }
  else if (end)
    {
      char *copy;

      copy = g_malloc (end - (char *)nptr + 1);
      memcpy (copy, nptr, end - nptr);
      *(copy + (end - (char *)nptr)) = 0;

      errno = 0;
      val = strtod (copy, &fail_pos);
      strtod_errno = errno;

      if (fail_pos)
        {
          fail_pos = (char *)nptr + (fail_pos - copy);
        }

      g_free (copy);
    }
  else
    {
      errno = 0;
      val = strtod (nptr, &fail_pos);
      strtod_errno = errno;
    }

  if (endptr)
    *endptr = fail_pos;

  errno = strtod_errno;

  return val;
#endif
}


/**
 * g_ascii_dtostr:
 * @buffer: A buffer to place the resulting string in
 * @buf_len: The length of the buffer.
 * @d: The #xdouble_t to convert
 *
 * Converts a #xdouble_t to a string, using the '.' as
 * decimal point.
 *
 * This function generates enough precision that converting
 * the string back using g_ascii_strtod() gives the same machine-number
 * (on machines with IEEE compatible 64bit doubles). It is
 * guaranteed that the size of the resulting string will never
 * be larger than %G_ASCII_DTOSTR_BUF_SIZE bytes, including the terminating
 * nul character, which is always added.
 *
 * Returns: The pointer to the buffer with the converted string.
 **/
xchar_t *
g_ascii_dtostr (xchar_t       *buffer,
                xint_t         buf_len,
                xdouble_t      d)
{
  return g_ascii_formatd (buffer, buf_len, "%.17g", d);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

/**
 * g_ascii_formatd:
 * @buffer: A buffer to place the resulting string in
 * @buf_len: The length of the buffer.
 * @format: The printf()-style format to use for the
 *   code to use for converting
 * @d: The #xdouble_t to convert
 *
 * Converts a #xdouble_t to a string, using the '.' as
 * decimal point. To format the number you pass in
 * a printf()-style format string. Allowed conversion
 * specifiers are 'e', 'E', 'f', 'F', 'g' and 'G'.
 *
 * The @format must just be a single format specifier
 * starting with `%`, expecting a #xdouble_t argument.
 *
 * The returned buffer is guaranteed to be nul-terminated.
 *
 * If you just want to want to serialize the value into a
 * string, use g_ascii_dtostr().
 *
 * Returns: The pointer to the buffer with the converted string.
 */
xchar_t *
g_ascii_formatd (xchar_t       *buffer,
                 xint_t         buf_len,
                 const xchar_t *format,
                 xdouble_t      d)
{
#ifdef USE_XLOCALE
  locale_t old_locale;

  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (format[0] == '%', NULL);
  g_return_val_if_fail (strpbrk (format + 1, "'l%") == NULL, NULL);

  old_locale = uselocale (get_C_locale ());
   _g_snprintf (buffer, buf_len, format, d);
  uselocale (old_locale);

  return buffer;
#else
#ifndef __BIONIC__
  struct lconv *locale_data;
#endif
  const char *decimal_point;
  xsize_t decimal_point_len;
  xchar_t *p;
  int rest_len;
  xchar_t format_char;

  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (format[0] == '%', NULL);
  g_return_val_if_fail (strpbrk (format + 1, "'l%") == NULL, NULL);

  format_char = format[strlen (format) - 1];

  g_return_val_if_fail (format_char == 'e' || format_char == 'E' ||
                        format_char == 'f' || format_char == 'F' ||
                        format_char == 'g' || format_char == 'G',
                        NULL);

  if (format[0] != '%')
    return NULL;

  if (strpbrk (format + 1, "'l%"))
    return NULL;

  if (!(format_char == 'e' || format_char == 'E' ||
        format_char == 'f' || format_char == 'F' ||
        format_char == 'g' || format_char == 'G'))
    return NULL;

  _g_snprintf (buffer, buf_len, format, d);

#ifndef __BIONIC__
  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);
#else
  decimal_point = ".";
  decimal_point_len = 1;
#endif

  g_assert (decimal_point_len != 0);

  if (decimal_point[0] != '.' ||
      decimal_point[1] != 0)
    {
      p = buffer;

      while (g_ascii_isspace (*p))
        p++;

      if (*p == '+' || *p == '-')
        p++;

      while (isdigit ((guchar)*p))
        p++;

      if (strncmp (p, decimal_point, decimal_point_len) == 0)
        {
          *p = '.';
          p++;
          if (decimal_point_len > 1)
            {
              rest_len = strlen (p + (decimal_point_len - 1));
              memmove (p, p + (decimal_point_len - 1), rest_len);
              p[rest_len] = 0;
            }
        }
    }

  return buffer;
#endif
}
#pragma GCC diagnostic pop

#define ISSPACE(c)              ((c) == ' ' || (c) == '\f' || (c) == '\n' || \
                                 (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISUPPER(c)              ((c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c)              ((c) >= 'a' && (c) <= 'z')
#define ISALPHA(c)              (ISUPPER (c) || ISLOWER (c))
#define TOUPPER(c)              (ISLOWER (c) ? (c) - 'a' + 'A' : (c))
#define TOLOWER(c)              (ISUPPER (c) ? (c) - 'A' + 'a' : (c))

#if !defined(USE_XLOCALE) || !defined(HAVE_STRTOULL_L) || !defined(HAVE_STRTOLL_L)

static xuint64_t
g_parse_long_long (const xchar_t  *nptr,
                   const xchar_t **endptr,
                   xuint_t         base,
                   xboolean_t     *negative)
{
  /* this code is based on on the strtol(3) code from GNU libc released under
   * the GNU Lesser General Public License.
   *
   * Copyright (C) 1991,92,94,95,96,97,98,99,2000,01,02
   *        Free Software Foundation, Inc.
   */
  xboolean_t overflow;
  xuint64_t cutoff;
  xuint64_t cutlim;
  xuint64_t ui64;
  const xchar_t *s, *save;
  guchar c;

  g_return_val_if_fail (nptr != NULL, 0);

  *negative = FALSE;
  if (base == 1 || base > 36)
    {
      errno = EINVAL;
      if (endptr)
        *endptr = nptr;
      return 0;
    }

  save = s = nptr;

  /* Skip white space.  */
  while (ISSPACE (*s))
    ++s;

  if (G_UNLIKELY (!*s))
    goto noconv;

  /* Check for a sign.  */
  if (*s == '-')
    {
      *negative = TRUE;
      ++s;
    }
  else if (*s == '+')
    ++s;

  /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
  if (*s == '0')
    {
      if ((base == 0 || base == 16) && TOUPPER (s[1]) == 'X')
        {
          s += 2;
          base = 16;
        }
      else if (base == 0)
        base = 8;
    }
  else if (base == 0)
    base = 10;

  /* Save the pointer so we can check later if anything happened.  */
  save = s;
  cutoff = G_MAXUINT64 / base;
  cutlim = G_MAXUINT64 % base;

  overflow = FALSE;
  ui64 = 0;
  c = *s;
  for (; c; c = *++s)
    {
      if (c >= '0' && c <= '9')
        c -= '0';
      else if (ISALPHA (c))
        c = TOUPPER (c) - 'A' + 10;
      else
        break;
      if (c >= base)
        break;
      /* Check for overflow.  */
      if (ui64 > cutoff || (ui64 == cutoff && c > cutlim))
        overflow = TRUE;
      else
        {
          ui64 *= base;
          ui64 += c;
        }
    }

  /* Check if anything actually happened.  */
  if (s == save)
    goto noconv;

  /* Store in ENDPTR the address of one character
     past the last character we converted.  */
  if (endptr)
    *endptr = s;

  if (G_UNLIKELY (overflow))
    {
      errno = ERANGE;
      return G_MAXUINT64;
    }

  return ui64;

 noconv:
  /* We must handle a special case here: the base is 0 or 16 and the
     first two characters are '0' and 'x', but the rest are no
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
  if (endptr)
    {
      if (save - nptr >= 2 && TOUPPER (save[-1]) == 'X'
          && save[-2] == '0')
        *endptr = &save[-1];
      else
        /*  There was no number to convert.  */
        *endptr = nptr;
    }
  return 0;
}
#endif /* !defined(USE_XLOCALE) || !defined(HAVE_STRTOULL_L) || !defined(HAVE_STRTOLL_L) */

/**
 * g_ascii_strtoull:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  (out) (transfer none) (optional): if non-%NULL, it returns the
 *           character after the last character used in the conversion.
 * @base:    to be used for the conversion, 2..36 or 0
 *
 * Converts a string to a #xuint64_t value.
 * This function behaves like the standard strtoull() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * Note that input with a leading minus sign (`-`) is accepted, and will return
 * the negation of the parsed number, unless that would overflow a #xuint64_t.
 * Critically, this means you cannot assume that a short fixed length input will
 * never result in a low return value, as the input could have a leading `-`.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtoull() function.
 *
 * If the correct value would cause overflow, %G_MAXUINT64
 * is returned, and `ERANGE` is stored in `errno`.
 * If the base is outside the valid range, zero is returned, and
 * `EINVAL` is stored in `errno`.
 * If the string conversion fails, zero is returned, and @endptr returns
 * @nptr (if @endptr is non-%NULL).
 *
 * Returns: the #xuint64_t value or zero on error.
 *
 * Since: 2.2
 */
xuint64_t
g_ascii_strtoull (const xchar_t *nptr,
                  xchar_t      **endptr,
                  xuint_t        base)
{
#if defined(USE_XLOCALE) && defined(HAVE_STRTOULL_L)
  return strtoull_l (nptr, endptr, base, get_C_locale ());
#else
  xboolean_t negative;
  xuint64_t result;

  result = g_parse_long_long (nptr, (const xchar_t **) endptr, base, &negative);

  /* Return the result of the appropriate sign.  */
  return negative ? -result : result;
#endif
}

/**
 * g_ascii_strtoll:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  (out) (transfer none) (optional): if non-%NULL, it returns the
 *           character after the last character used in the conversion.
 * @base:    to be used for the conversion, 2..36 or 0
 *
 * Converts a string to a #gint64 value.
 * This function behaves like the standard strtoll() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtoll() function.
 *
 * If the correct value would cause overflow, %G_MAXINT64 or %G_MININT64
 * is returned, and `ERANGE` is stored in `errno`.
 * If the base is outside the valid range, zero is returned, and
 * `EINVAL` is stored in `errno`. If the
 * string conversion fails, zero is returned, and @endptr returns @nptr
 * (if @endptr is non-%NULL).
 *
 * Returns: the #gint64 value or zero on error.
 *
 * Since: 2.12
 */
gint64
g_ascii_strtoll (const xchar_t *nptr,
                 xchar_t      **endptr,
                 xuint_t        base)
{
#if defined(USE_XLOCALE) && defined(HAVE_STRTOLL_L)
  return strtoll_l (nptr, endptr, base, get_C_locale ());
#else
  xboolean_t negative;
  xuint64_t result;

  result = g_parse_long_long (nptr, (const xchar_t **) endptr, base, &negative);

  if (negative && result > (xuint64_t) G_MININT64)
    {
      errno = ERANGE;
      return G_MININT64;
    }
  else if (!negative && result > (xuint64_t) G_MAXINT64)
    {
      errno = ERANGE;
      return G_MAXINT64;
    }
  else if (negative)
    return - (gint64) result;
  else
    return (gint64) result;
#endif
}

/**
 * xstrerror:
 * @errnum: the system error number. See the standard C %errno
 *     documentation
 *
 * Returns a string corresponding to the given error code, e.g. "no
 * such process". Unlike strerror(), this always returns a string in
 * UTF-8 encoding, and the pointer is guaranteed to remain valid for
 * the lifetime of the process.
 *
 * Note that the string may be translated according to the current locale.
 *
 * The value of %errno will not be changed by this function. However, it may
 * be changed by intermediate function calls, so you should save its value
 * as soon as the call returns:
 * |[
 *   int saved_errno;
 *
 *   ret = read (blah);
 *   saved_errno = errno;
 *
 *   xstrerror (saved_errno);
 * ]|
 *
 * Returns: a UTF-8 string describing the error code. If the error code
 *     is unknown, it returns a string like "unknown error (<code>)".
 */
const xchar_t *
xstrerror (xint_t errnum)
{
  static xhashtable_t *errors;
  G_LOCK_DEFINE_STATIC (errors);
  const xchar_t *msg;
  xint_t saved_errno = errno;

  G_LOCK (errors);
  if (errors)
    msg = xhash_table_lookup (errors, GINT_TO_POINTER (errnum));
  else
    {
      errors = xhash_table_new (NULL, NULL);
      msg = NULL;
    }

  if (!msg)
    {
      xchar_t buf[1024];
      xerror_t *error = NULL;

#if defined(G_OS_WIN32)
      strerror_s (buf, sizeof (buf), errnum);
      msg = buf;
#elif defined(HAVE_STRERROR_R)
      /* Match the condition in strerror_r(3) for glibc */
#  if defined(STRERROR_R_CHAR_P)
      msg = strerror_r (errnum, buf, sizeof (buf));
#  else
      (void) strerror_r (errnum, buf, sizeof (buf));
      msg = buf;
#  endif /* HAVE_STRERROR_R */
#else
      xstrlcpy (buf, strerror (errnum), sizeof (buf));
      msg = buf;
#endif
      if (!g_get_console_charset (NULL))
        {
          msg = g_locale_to_utf8 (msg, -1, NULL, NULL, &error);
          if (error)
            g_print ("%s\n", error->message);
        }
      else if (msg == (const xchar_t *)buf)
        msg = xstrdup (buf);

      xhash_table_insert (errors, GINT_TO_POINTER (errnum), (char *) msg);
    }
  G_UNLOCK (errors);

  errno = saved_errno;
  return msg;
}

/**
 * xstrsignal:
 * @signum: the signal number. See the `signal` documentation
 *
 * Returns a string describing the given signal, e.g. "Segmentation fault".
 * You should use this function in preference to strsignal(), because it
 * returns a string in UTF-8 encoding, and since not all platforms support
 * the strsignal() function.
 *
 * Returns: a UTF-8 string describing the signal. If the signal is unknown,
 *     it returns "unknown signal (<signum>)".
 */
const xchar_t *
xstrsignal (xint_t signum)
{
  xchar_t *msg;
  xchar_t *tofree;
  const xchar_t *ret;

  msg = tofree = NULL;

#ifdef HAVE_STRSIGNAL
  msg = strsignal (signum);
  if (!g_get_console_charset (NULL))
    msg = tofree = g_locale_to_utf8 (msg, -1, NULL, NULL, NULL);
#endif

  if (!msg)
    msg = tofree = xstrdup_printf ("unknown signal (%d)", signum);
  ret = g_intern_string (msg);
  g_free (tofree);

  return ret;
}

/* Functions xstrlcpy and xstrlcat were originally developed by
 * Todd C. Miller <Todd.Miller@courtesan.com> to simplify writing secure code.
 * See http://www.openbsd.org/cgi-bin/man.cgi?query=strlcpy
 * for more information.
 */

#ifdef HAVE_STRLCPY
/* Use the native ones, if available; they might be implemented in assembly */
xsize_t
xstrlcpy (xchar_t       *dest,
           const xchar_t *src,
           xsize_t        dest_size)
{
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);

  return strlcpy (dest, src, dest_size);
}

xsize_t
xstrlcat (xchar_t       *dest,
           const xchar_t *src,
           xsize_t        dest_size)
{
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);

  return strlcat (dest, src, dest_size);
}

#else /* ! HAVE_STRLCPY */
/**
 * xstrlcpy:
 * @dest: destination buffer
 * @src: source buffer
 * @dest_size: length of @dest in bytes
 *
 * Portability wrapper that calls strlcpy() on systems which have it,
 * and emulates strlcpy() otherwise. Copies @src to @dest; @dest is
 * guaranteed to be nul-terminated; @src must be nul-terminated;
 * @dest_size is the buffer size, not the number of bytes to copy.
 *
 * At most @dest_size - 1 characters will be copied. Always nul-terminates
 * (unless @dest_size is 0). This function does not allocate memory. Unlike
 * strncpy(), this function doesn't pad @dest (so it's often faster). It
 * returns the size of the attempted result, strlen (src), so if
 * @retval >= @dest_size, truncation occurred.
 *
 * Caveat: strlcpy() is supposedly more secure than strcpy() or strncpy(),
 * but if you really want to avoid screwups, xstrdup() is an even better
 * idea.
 *
 * Returns: length of @src
 */
xsize_t
xstrlcpy (xchar_t       *dest,
           const xchar_t *src,
           xsize_t        dest_size)
{
  xchar_t *d = dest;
  const xchar_t *s = src;
  xsize_t n = dest_size;

  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);

  /* Copy as many bytes as will fit */
  if (n != 0 && --n != 0)
    do
      {
        xchar_t c = *s++;

        *d++ = c;
        if (c == 0)
          break;
      }
    while (--n != 0);

  /* If not enough room in dest, add NUL and traverse rest of src */
  if (n == 0)
    {
      if (dest_size != 0)
        *d = 0;
      while (*s++)
        ;
    }

  return s - src - 1;  /* count does not include NUL */
}

/**
 * xstrlcat:
 * @dest: destination buffer, already containing one nul-terminated string
 * @src: source buffer
 * @dest_size: length of @dest buffer in bytes (not length of existing string
 *     inside @dest)
 *
 * Portability wrapper that calls strlcat() on systems which have it,
 * and emulates it otherwise. Appends nul-terminated @src string to @dest,
 * guaranteeing nul-termination for @dest. The total size of @dest won't
 * exceed @dest_size.
 *
 * At most @dest_size - 1 characters will be copied. Unlike strncat(),
 * @dest_size is the full size of dest, not the space left over. This
 * function does not allocate memory. It always nul-terminates (unless
 * @dest_size == 0 or there were no nul characters in the @dest_size
 * characters of dest to start with).
 *
 * Caveat: this is supposedly a more secure alternative to strcat() or
 * strncat(), but for real security xstrconcat() is harder to mess up.
 *
 * Returns: size of attempted result, which is MIN (dest_size, strlen
 *     (original dest)) + strlen (src), so if retval >= dest_size,
 *     truncation occurred.
 */
xsize_t
xstrlcat (xchar_t       *dest,
           const xchar_t *src,
           xsize_t        dest_size)
{
  xchar_t *d = dest;
  const xchar_t *s = src;
  xsize_t bytes_left = dest_size;
  xsize_t dlength;  /* Logically, MIN (strlen (d), dest_size) */

  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);

  /* Find the end of dst and adjust bytes left but don't go past end */
  while (*d != 0 && bytes_left-- != 0)
    d++;
  dlength = d - dest;
  bytes_left = dest_size - dlength;

  if (bytes_left == 0)
    return dlength + strlen (s);

  while (*s != 0)
    {
      if (bytes_left != 1)
        {
          *d++ = *s;
          bytes_left--;
        }
      s++;
    }
  *d = 0;

  return dlength + (s - src);  /* count does not include NUL */
}
#endif /* ! HAVE_STRLCPY */

/**
 * g_ascii_strdown:
 * @str: a string
 * @len: length of @str in bytes, or -1 if @str is nul-terminated
 *
 * Converts all upper case ASCII letters to lower case ASCII letters.
 *
 * Returns: a newly-allocated string, with all the upper case
 *     characters in @str converted to lower case, with semantics that
 *     exactly match g_ascii_tolower(). (Note that this is unlike the
 *     old xstrdown(), which modified the string in place.)
 */
xchar_t*
g_ascii_strdown (const xchar_t *str,
                 xssize_t       len)
{
  xchar_t *result, *s;

  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    len = (xssize_t) strlen (str);

  result = xstrndup (str, (xsize_t) len);
  for (s = result; *s; s++)
    *s = g_ascii_tolower (*s);

  return result;
}

/**
 * g_ascii_strup:
 * @str: a string
 * @len: length of @str in bytes, or -1 if @str is nul-terminated
 *
 * Converts all lower case ASCII letters to upper case ASCII letters.
 *
 * Returns: a newly allocated string, with all the lower case
 *     characters in @str converted to upper case, with semantics that
 *     exactly match g_ascii_toupper(). (Note that this is unlike the
 *     old xstrup(), which modified the string in place.)
 */
xchar_t*
g_ascii_strup (const xchar_t *str,
               xssize_t       len)
{
  xchar_t *result, *s;

  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    len = (xssize_t) strlen (str);

  result = xstrndup (str, (xsize_t) len);
  for (s = result; *s; s++)
    *s = g_ascii_toupper (*s);

  return result;
}

/**
 * xstr_is_ascii:
 * @str: a string
 *
 * Determines if a string is pure ASCII. A string is pure ASCII if it
 * contains no bytes with the high bit set.
 *
 * Returns: %TRUE if @str is ASCII
 *
 * Since: 2.40
 */
xboolean_t
xstr_is_ascii (const xchar_t *str)
{
  xsize_t i;

  for (i = 0; str[i]; i++)
    if (str[i] & 0x80)
      return FALSE;

  return TRUE;
}

/**
 * xstrdown:
 * @string: the string to convert.
 *
 * Converts a string to lower case.
 *
 * Returns: the string
 *
 * Deprecated:2.2: This function is totally broken for the reasons discussed
 * in the xstrncasecmp() docs - use g_ascii_strdown() or xutf8_strdown()
 * instead.
 **/
xchar_t*
xstrdown (xchar_t *string)
{
  guchar *s;

  g_return_val_if_fail (string != NULL, NULL);

  s = (guchar *) string;

  while (*s)
    {
      if (isupper (*s))
        *s = tolower (*s);
      s++;
    }

  return (xchar_t *) string;
}

/**
 * xstrup:
 * @string: the string to convert
 *
 * Converts a string to upper case.
 *
 * Returns: the string
 *
 * Deprecated:2.2: This function is totally broken for the reasons
 *     discussed in the xstrncasecmp() docs - use g_ascii_strup()
 *     or xutf8_strup() instead.
 */
xchar_t*
xstrup (xchar_t *string)
{
  guchar *s;

  g_return_val_if_fail (string != NULL, NULL);

  s = (guchar *) string;

  while (*s)
    {
      if (islower (*s))
        *s = toupper (*s);
      s++;
    }

  return (xchar_t *) string;
}

/**
 * xstrreverse:
 * @string: the string to reverse
 *
 * Reverses all of the bytes in a string. For example,
 * `xstrreverse ("abcdef")` will result in "fedcba".
 *
 * Note that xstrreverse() doesn't work on UTF-8 strings
 * containing multibyte characters. For that purpose, use
 * xutf8_strreverse().
 *
 * Returns: the same pointer passed in as @string
 */
xchar_t*
xstrreverse (xchar_t *string)
{
  g_return_val_if_fail (string != NULL, NULL);

  if (*string)
    {
      xchar_t *h, *t;

      h = string;
      t = string + strlen (string) - 1;

      while (h < t)
        {
          xchar_t c;

          c = *h;
          *h = *t;
          h++;
          *t = c;
          t--;
        }
    }

  return string;
}

/**
 * g_ascii_tolower:
 * @c: any character
 *
 * Convert a character to ASCII lower case.
 *
 * Unlike the standard C library tolower() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are lower case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #guchar
 * before passing a possibly non-ASCII character in.
 *
 * Returns: the result of converting @c to lower case. If @c is
 *     not an ASCII upper case letter, @c is returned unchanged.
 */
xchar_t
g_ascii_tolower (xchar_t c)
{
  return g_ascii_isupper (c) ? c - 'A' + 'a' : c;
}

/**
 * g_ascii_toupper:
 * @c: any character
 *
 * Convert a character to ASCII upper case.
 *
 * Unlike the standard C library toupper() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are upper case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #guchar
 * before passing a possibly non-ASCII character in.
 *
 * Returns: the result of converting @c to upper case. If @c is not
 *    an ASCII lower case letter, @c is returned unchanged.
 */
xchar_t
g_ascii_toupper (xchar_t c)
{
  return g_ascii_islower (c) ? c - 'a' + 'A' : c;
}

/**
 * g_ascii_digit_value:
 * @c: an ASCII character
 *
 * Determines the numeric value of a character as a decimal digit.
 * Differs from xunichar_digit_value() because it takes a char, so
 * there's no worry about sign extension if characters are signed.
 *
 * Returns: If @c is a decimal digit (according to g_ascii_isdigit()),
 *    its numeric value. Otherwise, -1.
 */
int
g_ascii_digit_value (xchar_t c)
{
  if (g_ascii_isdigit (c))
    return c - '0';
  return -1;
}

/**
 * g_ascii_xdigit_value:
 * @c: an ASCII character.
 *
 * Determines the numeric value of a character as a hexadecimal
 * digit. Differs from xunichar_xdigit_value() because it takes
 * a char, so there's no worry about sign extension if characters
 * are signed.
 *
 * Returns: If @c is a hex digit (according to g_ascii_isxdigit()),
 *     its numeric value. Otherwise, -1.
 */
int
g_ascii_xdigit_value (xchar_t c)
{
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return g_ascii_digit_value (c);
}

/**
 * g_ascii_strcasecmp:
 * @s1: string to compare with @s2
 * @s2: string to compare with @s1
 *
 * Compare two strings, ignoring the case of ASCII characters.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * bytes as if they are not letters.
 *
 * This function should be used only on strings that are known to be
 * in encodings where the bytes corresponding to ASCII letters always
 * represent themselves. This includes UTF-8 and the ISO-8859-*
 * charsets, but not for instance double-byte encodings like the
 * Windows Codepage 932, where the trailing bytes of double-byte
 * characters include all ASCII letters. If you compare two CP932
 * strings using this function, you will get false matches.
 *
 * Both @s1 and @s2 must be non-%NULL.
 *
 * Returns: 0 if the strings match, a negative value if @s1 < @s2,
 *     or a positive value if @s1 > @s2.
 */
xint_t
g_ascii_strcasecmp (const xchar_t *s1,
                    const xchar_t *s2)
{
  xint_t c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (*s1 && *s2)
    {
      c1 = (xint_t)(guchar) TOLOWER (*s1);
      c2 = (xint_t)(guchar) TOLOWER (*s2);
      if (c1 != c2)
        return (c1 - c2);
      s1++; s2++;
    }

  return (((xint_t)(guchar) *s1) - ((xint_t)(guchar) *s2));
}

/**
 * g_ascii_strncasecmp:
 * @s1: string to compare with @s2
 * @s2: string to compare with @s1
 * @n: number of characters to compare
 *
 * Compare @s1 and @s2, ignoring the case of ASCII characters and any
 * characters after the first @n in each string. If either string is
 * less than @n bytes long, comparison will stop at the first nul byte
 * encountered.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * characters as if they are not letters.
 *
 * The same warning as in g_ascii_strcasecmp() applies: Use this
 * function only on strings known to be in encodings where bytes
 * corresponding to ASCII letters always represent themselves.
 *
 * Returns: 0 if the strings match, a negative value if @s1 < @s2,
 *     or a positive value if @s1 > @s2.
 */
xint_t
g_ascii_strncasecmp (const xchar_t *s1,
                     const xchar_t *s2,
                     xsize_t        n)
{
  xint_t c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      n -= 1;
      c1 = (xint_t)(guchar) TOLOWER (*s1);
      c2 = (xint_t)(guchar) TOLOWER (*s2);
      if (c1 != c2)
        return (c1 - c2);
      s1++; s2++;
    }

  if (n)
    return (((xint_t) (guchar) *s1) - ((xint_t) (guchar) *s2));
  else
    return 0;
}

/**
 * xstrcasecmp:
 * @s1: a string
 * @s2: a string to compare with @s1
 *
 * A case-insensitive string comparison, corresponding to the standard
 * strcasecmp() function on platforms which support it.
 *
 * Returns: 0 if the strings match, a negative value if @s1 < @s2,
 *     or a positive value if @s1 > @s2.
 *
 * Deprecated:2.2: See xstrncasecmp() for a discussion of why this
 *     function is deprecated and how to replace it.
 */
xint_t
xstrcasecmp (const xchar_t *s1,
              const xchar_t *s2)
{
#ifdef HAVE_STRCASECMP
  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  return strcasecmp (s1, s2);
#else
  xint_t c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (*s1 && *s2)
    {
      /* According to A. Cox, some platforms have islower's that
       * don't work right on non-uppercase
       */
      c1 = isupper ((guchar)*s1) ? tolower ((guchar)*s1) : *s1;
      c2 = isupper ((guchar)*s2) ? tolower ((guchar)*s2) : *s2;
      if (c1 != c2)
        return (c1 - c2);
      s1++; s2++;
    }

  return (((xint_t)(guchar) *s1) - ((xint_t)(guchar) *s2));
#endif
}

/**
 * xstrncasecmp:
 * @s1: a string
 * @s2: a string to compare with @s1
 * @n: the maximum number of characters to compare
 *
 * A case-insensitive string comparison, corresponding to the standard
 * strncasecmp() function on platforms which support it. It is similar
 * to xstrcasecmp() except it only compares the first @n characters of
 * the strings.
 *
 * Returns: 0 if the strings match, a negative value if @s1 < @s2,
 *     or a positive value if @s1 > @s2.
 *
 * Deprecated:2.2: The problem with xstrncasecmp() is that it does
 *     the comparison by calling toupper()/tolower(). These functions
 *     are locale-specific and operate on single bytes. However, it is
 *     impossible to handle things correctly from an internationalization
 *     standpoint by operating on bytes, since characters may be multibyte.
 *     Thus xstrncasecmp() is broken if your string is guaranteed to be
 *     ASCII, since it is locale-sensitive, and it's broken if your string
 *     is localized, since it doesn't work on many encodings at all,
 *     including UTF-8, EUC-JP, etc.
 *
 *     There are therefore two replacement techniques: g_ascii_strncasecmp(),
 *     which only works on ASCII and is not locale-sensitive, and
 *     xutf8_casefold() followed by strcmp() on the resulting strings,
 *     which is good for case-insensitive sorting of UTF-8.
 */
xint_t
xstrncasecmp (const xchar_t *s1,
               const xchar_t *s2,
               xuint_t n)
{
#ifdef HAVE_STRNCASECMP
  return strncasecmp (s1, s2, n);
#else
  xint_t c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      n -= 1;
      /* According to A. Cox, some platforms have islower's that
       * don't work right on non-uppercase
       */
      c1 = isupper ((guchar)*s1) ? tolower ((guchar)*s1) : *s1;
      c2 = isupper ((guchar)*s2) ? tolower ((guchar)*s2) : *s2;
      if (c1 != c2)
        return (c1 - c2);
      s1++; s2++;
    }

  if (n)
    return (((xint_t) (guchar) *s1) - ((xint_t) (guchar) *s2));
  else
    return 0;
#endif
}

/**
 * xstrdelimit:
 * @string: the string to convert
 * @delimiters: (nullable): a string containing the current delimiters,
 *     or %NULL to use the standard delimiters defined in %G_STR_DELIMITERS
 * @new_delimiter: the new delimiter character
 *
 * Converts any delimiter characters in @string to @new_delimiter.
 *
 * Any characters in @string which are found in @delimiters are
 * changed to the @new_delimiter character. Modifies @string in place,
 * and returns @string itself, not a copy.
 *
 * The return value is to allow nesting such as:
 *
 * |[<!-- language="C" -->
 *   g_ascii_strup (xstrdelimit (str, "abc", '?'))
 * ]|
 *
 * In order to modify a copy, you may use xstrdup():
 *
 * |[<!-- language="C" -->
 *   reformatted = xstrdelimit (xstrdup (const_str), "abc", '?');
 *   ...
 *   g_free (reformatted);
 * ]|
 *
 * Returns: the modified @string
 */
xchar_t *
xstrdelimit (xchar_t       *string,
              const xchar_t *delimiters,
              xchar_t        new_delim)
{
  xchar_t *c;

  g_return_val_if_fail (string != NULL, NULL);

  if (!delimiters)
    delimiters = G_STR_DELIMITERS;

  for (c = string; *c; c++)
    {
      if (strchr (delimiters, *c))
        *c = new_delim;
    }

  return string;
}

/**
 * xstrcanon:
 * @string: a nul-terminated array of bytes
 * @valid_chars: bytes permitted in @string
 * @substitutor: replacement character for disallowed bytes
 *
 * For each character in @string, if the character is not in @valid_chars,
 * replaces the character with @substitutor.
 *
 * Modifies @string in place, and return @string itself, not a copy. The
 * return value is to allow nesting such as:
 *
 * |[<!-- language="C" -->
 *   g_ascii_strup (xstrcanon (str, "abc", '?'))
 * ]|
 *
 * In order to modify a copy, you may use xstrdup():
 *
 * |[<!-- language="C" -->
 *   reformatted = xstrcanon (xstrdup (const_str), "abc", '?');
 *   ...
 *   g_free (reformatted);
 * ]|
 *
 * Returns: the modified @string
 */
xchar_t *
xstrcanon (xchar_t       *string,
            const xchar_t *valid_chars,
            xchar_t        substitutor)
{
  xchar_t *c;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (valid_chars != NULL, NULL);

  for (c = string; *c; c++)
    {
      if (!strchr (valid_chars, *c))
        *c = substitutor;
    }

  return string;
}

/**
 * xstrcompress:
 * @source: a string to compress
 *
 * Replaces all escaped characters with their one byte equivalent.
 *
 * This function does the reverse conversion of xstrescape().
 *
 * Returns: a newly-allocated copy of @source with all escaped
 *     character compressed
 */
xchar_t *
xstrcompress (const xchar_t *source)
{
  const xchar_t *p = source, *octal;
  xchar_t *dest;
  xchar_t *q;

  g_return_val_if_fail (source != NULL, NULL);

  dest = g_malloc (strlen (source) + 1);
  q = dest;

  while (*p)
    {
      if (*p == '\\')
        {
          p++;
          switch (*p)
            {
            case '\0':
              g_warning ("xstrcompress: trailing \\");
              goto out;
            case '0':  case '1':  case '2':  case '3':  case '4':
            case '5':  case '6':  case '7':
              *q = 0;
              octal = p;
              while ((p < octal + 3) && (*p >= '0') && (*p <= '7'))
                {
                  *q = (*q * 8) + (*p - '0');
                  p++;
                }
              q++;
              p--;
              break;
            case 'b':
              *q++ = '\b';
              break;
            case 'f':
              *q++ = '\f';
              break;
            case 'n':
              *q++ = '\n';
              break;
            case 'r':
              *q++ = '\r';
              break;
            case 't':
              *q++ = '\t';
              break;
            case 'v':
              *q++ = '\v';
              break;
            default:            /* Also handles \" and \\ */
              *q++ = *p;
              break;
            }
        }
      else
        *q++ = *p;
      p++;
    }
out:
  *q = 0;

  return dest;
}

/**
 * xstrescape:
 * @source: a string to escape
 * @exceptions: (nullable): a string of characters not to escape in @source
 *
 * Escapes the special characters '\b', '\f', '\n', '\r', '\t', '\v', '\'
 * and '"' in the string @source by inserting a '\' before
 * them. Additionally all characters in the range 0x01-0x1F (everything
 * below SPACE) and in the range 0x7F-0xFF (all non-ASCII chars) are
 * replaced with a '\' followed by their octal representation.
 * Characters supplied in @exceptions are not escaped.
 *
 * xstrcompress() does the reverse conversion.
 *
 * Returns: a newly-allocated copy of @source with certain
 *     characters escaped. See above.
 */
xchar_t *
xstrescape (const xchar_t *source,
             const xchar_t *exceptions)
{
  const guchar *p;
  xchar_t *dest;
  xchar_t *q;
  guchar excmap[256];

  g_return_val_if_fail (source != NULL, NULL);

  p = (guchar *) source;
  /* Each source byte needs maximally four destination chars (\777) */
  q = dest = g_malloc (strlen (source) * 4 + 1);

  memset (excmap, 0, 256);
  if (exceptions)
    {
      guchar *e = (guchar *) exceptions;

      while (*e)
        {
          excmap[*e] = 1;
          e++;
        }
    }

  while (*p)
    {
      if (excmap[*p])
        *q++ = *p;
      else
        {
          switch (*p)
            {
            case '\b':
              *q++ = '\\';
              *q++ = 'b';
              break;
            case '\f':
              *q++ = '\\';
              *q++ = 'f';
              break;
            case '\n':
              *q++ = '\\';
              *q++ = 'n';
              break;
            case '\r':
              *q++ = '\\';
              *q++ = 'r';
              break;
            case '\t':
              *q++ = '\\';
              *q++ = 't';
              break;
            case '\v':
              *q++ = '\\';
              *q++ = 'v';
              break;
            case '\\':
              *q++ = '\\';
              *q++ = '\\';
              break;
            case '"':
              *q++ = '\\';
              *q++ = '"';
              break;
            default:
              if ((*p < ' ') || (*p >= 0177))
                {
                  *q++ = '\\';
                  *q++ = '0' + (((*p) >> 6) & 07);
                  *q++ = '0' + (((*p) >> 3) & 07);
                  *q++ = '0' + ((*p) & 07);
                }
              else
                *q++ = *p;
              break;
            }
        }
      p++;
    }
  *q = 0;
  return dest;
}

/**
 * xstrchug:
 * @string: a string to remove the leading whitespace from
 *
 * Removes leading whitespace from a string, by moving the rest
 * of the characters forward.
 *
 * This function doesn't allocate or reallocate any memory;
 * it modifies @string in place. Therefore, it cannot be used on
 * statically allocated strings.
 *
 * The pointer to @string is returned to allow the nesting of functions.
 *
 * Also see xstrchomp() and xstrstrip().
 *
 * Returns: @string
 */
xchar_t *
xstrchug (xchar_t *string)
{
  guchar *start;

  g_return_val_if_fail (string != NULL, NULL);

  for (start = (guchar*) string; *start && g_ascii_isspace (*start); start++)
    ;

  memmove (string, start, strlen ((xchar_t *) start) + 1);

  return string;
}

/**
 * xstrchomp:
 * @string: a string to remove the trailing whitespace from
 *
 * Removes trailing whitespace from a string.
 *
 * This function doesn't allocate or reallocate any memory;
 * it modifies @string in place. Therefore, it cannot be used
 * on statically allocated strings.
 *
 * The pointer to @string is returned to allow the nesting of functions.
 *
 * Also see xstrchug() and xstrstrip().
 *
 * Returns: @string
 */
xchar_t *
xstrchomp (xchar_t *string)
{
  xsize_t len;

  g_return_val_if_fail (string != NULL, NULL);

  len = strlen (string);
  while (len--)
    {
      if (g_ascii_isspace ((guchar) string[len]))
        string[len] = '\0';
      else
        break;
    }

  return string;
}

/**
 * xstrsplit:
 * @string: a string to split
 * @delimiter: a string which specifies the places at which to split
 *     the string. The delimiter is not included in any of the resulting
 *     strings, unless @max_tokens is reached.
 * @max_tokens: the maximum number of pieces to split @string into.
 *     If this is less than 1, the string is split completely.
 *
 * Splits a string into a maximum of @max_tokens pieces, using the given
 * @delimiter. If @max_tokens is reached, the remainder of @string is
 * appended to the last token.
 *
 * As an example, the result of xstrsplit (":a:bc::d:", ":", -1) is a
 * %NULL-terminated vector containing the six strings "", "a", "bc", "", "d"
 * and "".
 *
 * As a special case, the result of splitting the empty string "" is an empty
 * vector, not a vector containing a single string. The reason for this
 * special case is that being able to represent an empty vector is typically
 * more useful than consistent handling of empty elements. If you do need
 * to represent empty elements, you'll need to check for the empty string
 * before calling xstrsplit().
 *
 * Returns: a newly-allocated %NULL-terminated array of strings. Use
 *    xstrfreev() to free it.
 */
xchar_t**
xstrsplit (const xchar_t *string,
            const xchar_t *delimiter,
            xint_t         max_tokens)
{
  char *s;
  const xchar_t *remainder;
  xptr_array_t *string_list;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (delimiter != NULL, NULL);
  g_return_val_if_fail (delimiter[0] != '\0', NULL);

  if (max_tokens < 1)
    max_tokens = G_MAXINT;

  string_list = xptr_array_new ();
  remainder = string;
  s = strstr (remainder, delimiter);
  if (s)
    {
      xsize_t delimiter_len = strlen (delimiter);

      while (--max_tokens && s)
        {
          xsize_t len;

          len = s - remainder;
          xptr_array_add (string_list, xstrndup (remainder, len));
          remainder = s + delimiter_len;
          s = strstr (remainder, delimiter);
        }
    }
  if (*string)
    xptr_array_add (string_list, xstrdup (remainder));

  xptr_array_add (string_list, NULL);

  return (char **) xptr_array_free (string_list, FALSE);
}

/**
 * xstrsplit_set:
 * @string: The string to be tokenized
 * @delimiters: A nul-terminated string containing bytes that are used
 *     to split the string (it can accept an empty string, which will result
 *     in no string splitting).
 * @max_tokens: The maximum number of tokens to split @string into.
 *     If this is less than 1, the string is split completely
 *
 * Splits @string into a number of tokens not containing any of the characters
 * in @delimiter. A token is the (possibly empty) longest string that does not
 * contain any of the characters in @delimiters. If @max_tokens is reached, the
 * remainder is appended to the last token.
 *
 * For example the result of xstrsplit_set ("abc:def/ghi", ":/", -1) is a
 * %NULL-terminated vector containing the three strings "abc", "def",
 * and "ghi".
 *
 * The result of xstrsplit_set (":def/ghi:", ":/", -1) is a %NULL-terminated
 * vector containing the four strings "", "def", "ghi", and "".
 *
 * As a special case, the result of splitting the empty string "" is an empty
 * vector, not a vector containing a single string. The reason for this
 * special case is that being able to represent an empty vector is typically
 * more useful than consistent handling of empty elements. If you do need
 * to represent empty elements, you'll need to check for the empty string
 * before calling xstrsplit_set().
 *
 * Note that this function works on bytes not characters, so it can't be used
 * to delimit UTF-8 strings for anything but ASCII characters.
 *
 * Returns: a newly-allocated %NULL-terminated array of strings. Use
 *    xstrfreev() to free it.
 *
 * Since: 2.4
 **/
xchar_t **
xstrsplit_set (const xchar_t *string,
                const xchar_t *delimiters,
                xint_t         max_tokens)
{
  xuint8_t delim_table[256]; /* 1 = index is a separator; 0 otherwise */
  xslist_t *tokens, *list;
  xint_t n_tokens;
  const xchar_t *s;
  const xchar_t *current;
  xchar_t *token;
  xchar_t **result;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (delimiters != NULL, NULL);

  if (max_tokens < 1)
    max_tokens = G_MAXINT;

  if (*string == '\0')
    {
      result = g_new (char *, 1);
      result[0] = NULL;
      return result;
    }

  /* Check if each character in @string is a separator, by indexing by the
   * character value into the @delim_table, which has value 1 stored at an index
   * if that index is a separator. */
  memset (delim_table, FALSE, sizeof (delim_table));
  for (s = delimiters; *s != '\0'; ++s)
    delim_table[*(guchar *)s] = TRUE;

  tokens = NULL;
  n_tokens = 0;

  s = current = string;
  while (*s != '\0')
    {
      if (delim_table[*(guchar *)s] && n_tokens + 1 < max_tokens)
        {
          token = xstrndup (current, s - current);
          tokens = xslist_prepend (tokens, token);
          ++n_tokens;

          current = s + 1;
        }

      ++s;
    }

  token = xstrndup (current, s - current);
  tokens = xslist_prepend (tokens, token);
  ++n_tokens;

  result = g_new (xchar_t *, n_tokens + 1);

  result[n_tokens] = NULL;
  for (list = tokens; list != NULL; list = list->next)
    result[--n_tokens] = list->data;

  xslist_free (tokens);

  return result;
}

/**
 * xstrv_t:
 *
 * A typedef alias for xchar_t**. This is mostly useful when used together with
 * x_auto().
 */

/**
 * xstrfreev:
 * @str_array: (nullable): a %NULL-terminated array of strings to free
 *
 * Frees a %NULL-terminated array of strings, as well as each
 * string it contains.
 *
 * If @str_array is %NULL, this function simply returns.
 */
void
xstrfreev (xchar_t **str_array)
{
  if (str_array)
    {
      xsize_t i;

      for (i = 0; str_array[i] != NULL; i++)
        g_free (str_array[i]);

      g_free (str_array);
    }
}

/**
 * xstrdupv:
 * @str_array: (nullable): a %NULL-terminated array of strings
 *
 * Copies %NULL-terminated array of strings. The copy is a deep copy;
 * the new array should be freed by first freeing each string, then
 * the array itself. xstrfreev() does this for you. If called
 * on a %NULL value, xstrdupv() simply returns %NULL.
 *
 * Returns: (nullable): a new %NULL-terminated array of strings.
 */
xchar_t**
xstrdupv (xchar_t **str_array)
{
  if (str_array)
    {
      xsize_t i;
      xchar_t **retval;

      i = 0;
      while (str_array[i])
        ++i;

      retval = g_new (xchar_t*, i + 1);

      i = 0;
      while (str_array[i])
        {
          retval[i] = xstrdup (str_array[i]);
          ++i;
        }
      retval[i] = NULL;

      return retval;
    }
  else
    return NULL;
}

/**
 * xstrjoinv:
 * @separator: (nullable): a string to insert between each of the
 *     strings, or %NULL
 * @str_array: a %NULL-terminated array of strings to join
 *
 * Joins a number of strings together to form one long string, with the
 * optional @separator inserted between each of them. The returned string
 * should be freed with g_free().
 *
 * If @str_array has no items, the return value will be an
 * empty string. If @str_array contains a single item, @separator will not
 * appear in the resulting string.
 *
 * Returns: a newly-allocated string containing all of the strings joined
 *     together, with @separator between them
 */
xchar_t*
xstrjoinv (const xchar_t  *separator,
            xchar_t       **str_array)
{
  xchar_t *string;
  xchar_t *ptr;

  g_return_val_if_fail (str_array != NULL, NULL);

  if (separator == NULL)
    separator = "";

  if (*str_array)
    {
      xsize_t i;
      xsize_t len;
      xsize_t separator_len;

      separator_len = strlen (separator);
      /* First part, getting length */
      len = 1 + strlen (str_array[0]);
      for (i = 1; str_array[i] != NULL; i++)
        len += strlen (str_array[i]);
      len += separator_len * (i - 1);

      /* Second part, building string */
      string = g_new (xchar_t, len);
      ptr = g_stpcpy (string, *str_array);
      for (i = 1; str_array[i] != NULL; i++)
        {
          ptr = g_stpcpy (ptr, separator);
          ptr = g_stpcpy (ptr, str_array[i]);
        }
      }
  else
    string = xstrdup ("");

  return string;
}

/**
 * xstrjoin:
 * @separator: (nullable): a string to insert between each of the
 *     strings, or %NULL
 * @...: a %NULL-terminated list of strings to join
 *
 * Joins a number of strings together to form one long string, with the
 * optional @separator inserted between each of them. The returned string
 * should be freed with g_free().
 *
 * Returns: a newly-allocated string containing all of the strings joined
 *     together, with @separator between them
 */
xchar_t*
xstrjoin (const xchar_t *separator,
           ...)
{
  xchar_t *string, *s;
  va_list args;
  xsize_t len;
  xsize_t separator_len;
  xchar_t *ptr;

  if (separator == NULL)
    separator = "";

  separator_len = strlen (separator);

  va_start (args, separator);

  s = va_arg (args, xchar_t*);

  if (s)
    {
      /* First part, getting length */
      len = 1 + strlen (s);

      s = va_arg (args, xchar_t*);
      while (s)
        {
          len += separator_len + strlen (s);
          s = va_arg (args, xchar_t*);
        }
      va_end (args);

      /* Second part, building string */
      string = g_new (xchar_t, len);

      va_start (args, separator);

      s = va_arg (args, xchar_t*);
      ptr = g_stpcpy (string, s);

      s = va_arg (args, xchar_t*);
      while (s)
        {
          ptr = g_stpcpy (ptr, separator);
          ptr = g_stpcpy (ptr, s);
          s = va_arg (args, xchar_t*);
        }
    }
  else
    string = xstrdup ("");

  va_end (args);

  return string;
}


/**
 * xstrstr_len:
 * @haystack: a nul-terminated string
 * @haystack_len: the maximum length of @haystack in bytes. A length of -1
 *     can be used to mean "search the entire string", like `strstr()`.
 * @needle: the string to search for
 *
 * Searches the string @haystack for the first occurrence
 * of the string @needle, limiting the length of the search
 * to @haystack_len.
 *
 * Returns: a pointer to the found occurrence, or
 *    %NULL if not found.
 */
xchar_t *
xstrstr_len (const xchar_t *haystack,
              xssize_t       haystack_len,
              const xchar_t *needle)
{
  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  if (haystack_len < 0)
    return strstr (haystack, needle);
  else
    {
      const xchar_t *p = haystack;
      xsize_t needle_len = strlen (needle);
      xsize_t haystack_len_unsigned = haystack_len;
      const xchar_t *end;
      xsize_t i;

      if (needle_len == 0)
        return (xchar_t *)haystack;

      if (haystack_len_unsigned < needle_len)
        return NULL;

      end = haystack + haystack_len - needle_len;

      while (p <= end && *p)
        {
          for (i = 0; i < needle_len; i++)
            if (p[i] != needle[i])
              goto next;

          return (xchar_t *)p;

        next:
          p++;
        }

      return NULL;
    }
}

/**
 * xstrrstr:
 * @haystack: a nul-terminated string
 * @needle: the nul-terminated string to search for
 *
 * Searches the string @haystack for the last occurrence
 * of the string @needle.
 *
 * Returns: a pointer to the found occurrence, or
 *    %NULL if not found.
 */
xchar_t *
xstrrstr (const xchar_t *haystack,
           const xchar_t *needle)
{
  xsize_t i;
  xsize_t needle_len;
  xsize_t haystack_len;
  const xchar_t *p;

  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  needle_len = strlen (needle);
  haystack_len = strlen (haystack);

  if (needle_len == 0)
    return (xchar_t *)haystack;

  if (haystack_len < needle_len)
    return NULL;

  p = haystack + haystack_len - needle_len;

  while (p >= haystack)
    {
      for (i = 0; i < needle_len; i++)
        if (p[i] != needle[i])
          goto next;

      return (xchar_t *)p;

    next:
      p--;
    }

  return NULL;
}

/**
 * xstrrstr_len:
 * @haystack: a nul-terminated string
 * @haystack_len: the maximum length of @haystack in bytes. A length of -1
 *     can be used to mean "search the entire string", like xstrrstr().
 * @needle: the nul-terminated string to search for
 *
 * Searches the string @haystack for the last occurrence
 * of the string @needle, limiting the length of the search
 * to @haystack_len.
 *
 * Returns: a pointer to the found occurrence, or
 *    %NULL if not found.
 */
xchar_t *
xstrrstr_len (const xchar_t *haystack,
               xssize_t        haystack_len,
               const xchar_t *needle)
{
  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  if (haystack_len < 0)
    return xstrrstr (haystack, needle);
  else
    {
      xsize_t needle_len = strlen (needle);
      const xchar_t *haystack_max = haystack + haystack_len;
      const xchar_t *p = haystack;
      xsize_t i;

      while (p < haystack_max && *p)
        p++;

      if (p < haystack + needle_len)
        return NULL;

      p -= needle_len;

      while (p >= haystack)
        {
          for (i = 0; i < needle_len; i++)
            if (p[i] != needle[i])
              goto next;

          return (xchar_t *)p;

        next:
          p--;
        }

      return NULL;
    }
}


/**
 * xstr_has_suffix:
 * @str: a nul-terminated string
 * @suffix: the nul-terminated suffix to look for
 *
 * Looks whether the string @str ends with @suffix.
 *
 * Returns: %TRUE if @str end with @suffix, %FALSE otherwise.
 *
 * Since: 2.2
 */
xboolean_t
xstr_has_suffix (const xchar_t *str,
                  const xchar_t *suffix)
{
  xsize_t str_len;
  xsize_t suffix_len;

  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (suffix != NULL, FALSE);

  str_len = strlen (str);
  suffix_len = strlen (suffix);

  if (str_len < suffix_len)
    return FALSE;

  return strcmp (str + str_len - suffix_len, suffix) == 0;
}

/**
 * xstr_has_prefix:
 * @str: a nul-terminated string
 * @prefix: the nul-terminated prefix to look for
 *
 * Looks whether the string @str begins with @prefix.
 *
 * Returns: %TRUE if @str begins with @prefix, %FALSE otherwise.
 *
 * Since: 2.2
 */
xboolean_t
xstr_has_prefix (const xchar_t *str,
                  const xchar_t *prefix)
{
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (prefix != NULL, FALSE);

  return strncmp (str, prefix, strlen (prefix)) == 0;
}

/**
 * xstrv_length:
 * @str_array: a %NULL-terminated array of strings
 *
 * Returns the length of the given %NULL-terminated
 * string array @str_array. @str_array must not be %NULL.
 *
 * Returns: length of @str_array.
 *
 * Since: 2.6
 */
xuint_t
xstrv_length (xchar_t **str_array)
{
  xuint_t i = 0;

  g_return_val_if_fail (str_array != NULL, 0);

  while (str_array[i])
    ++i;

  return i;
}

static void
index_add_folded (xptr_array_t   *array,
                  const xchar_t *start,
                  const xchar_t *end)
{
  xchar_t *normal;

  normal = xutf8_normalize (start, end - start, XNORMALIZE_ALL_COMPOSE);

  /* TODO: Invent time machine.  Converse with Mustafa Ataturk... */
  if (strstr (normal, "ı") || strstr (normal, "İ"))
    {
      xchar_t *s = normal;
      xstring_t *tmp;

      tmp = xstring_new (NULL);

      while (*s)
        {
          xchar_t *i, *I, *e;

          i = strstr (s, "ı");
          I = strstr (s, "İ");

          if (!i && !I)
            break;
          else if (i && !I)
            e = i;
          else if (I && !i)
            e = I;
          else if (i < I)
            e = i;
          else
            e = I;

          xstring_append_len (tmp, s, e - s);
          xstring_append_c (tmp, 'i');
          s = xutf8_next_char (e);
        }

      xstring_append (tmp, s);
      g_free (normal);
      normal = xstring_free (tmp, FALSE);
    }

  xptr_array_add (array, xutf8_casefold (normal, -1));
  g_free (normal);
}

static xchar_t **
split_words (const xchar_t *value)
{
  const xchar_t *start = NULL;
  xptr_array_t *result;
  const xchar_t *s;

  result = xptr_array_new ();

  for (s = value; *s; s = xutf8_next_char (s))
    {
      xunichar_t c = xutf8_get_char (s);

      if (start == NULL)
        {
          if (xunichar_isalnum (c) || xunichar_ismark (c))
            start = s;
        }
      else
        {
          if (!xunichar_isalnum (c) && !xunichar_ismark (c))
            {
              index_add_folded (result, start, s);
              start = NULL;
            }
        }
    }

  if (start)
    index_add_folded (result, start, s);

  xptr_array_add (result, NULL);

  return (xchar_t **) xptr_array_free (result, FALSE);
}

/**
 * xstr_tokenize_and_fold:
 * @string: a string
 * @translit_locale: (nullable): the language code (like 'de' or
 *   'en_GB') from which @string originates
 * @ascii_alternates: (out) (transfer full) (array zero-terminated=1): a
 *   return location for ASCII alternates
 *
 * Tokenises @string and performs folding on each token.
 *
 * A token is a non-empty sequence of alphanumeric characters in the
 * source string, separated by non-alphanumeric characters.  An
 * "alphanumeric" character for this purpose is one that matches
 * xunichar_isalnum() or xunichar_ismark().
 *
 * Each token is then (Unicode) normalised and case-folded.  If
 * @ascii_alternates is non-%NULL and some of the returned tokens
 * contain non-ASCII characters, ASCII alternatives will be generated.
 *
 * The number of ASCII alternatives that are generated and the method
 * for doing so is unspecified, but @translit_locale (if specified) may
 * improve the transliteration if the language of the source string is
 * known.
 *
 * Returns: (transfer full) (array zero-terminated=1): the folded tokens
 *
 * Since: 2.40
 **/
xchar_t **
xstr_tokenize_and_fold (const xchar_t   *string,
                         const xchar_t   *translit_locale,
                         xchar_t       ***ascii_alternates)
{
  xchar_t **result;

  g_return_val_if_fail (string != NULL, NULL);

  if (ascii_alternates && xstr_is_ascii (string))
    {
      *ascii_alternates = g_new0 (xchar_t *, 0 + 1);
      ascii_alternates = NULL;
    }

  result = split_words (string);

  if (ascii_alternates)
    {
      xint_t i, j, n;

      n = xstrv_length (result);
      *ascii_alternates = g_new (xchar_t *, n + 1);
      j = 0;

      for (i = 0; i < n; i++)
        {
          if (!xstr_is_ascii (result[i]))
            {
              xchar_t *composed;
              xchar_t *ascii;
              xint_t k;

              composed = xutf8_normalize (result[i], -1, XNORMALIZE_ALL_COMPOSE);

              ascii = xstr_to_ascii (composed, translit_locale);

              /* Only accept strings that are now entirely alnums */
              for (k = 0; ascii[k]; k++)
                if (!g_ascii_isalnum (ascii[k]))
                  break;

              if (ascii[k] == '\0')
                /* Made it to the end... */
                (*ascii_alternates)[j++] = ascii;
              else
                g_free (ascii);

              g_free (composed);
            }
        }

      (*ascii_alternates)[j] = NULL;
    }

  return result;
}

/**
 * xstr_match_string:
 * @search_term: the search term from the user
 * @potential_hit: the text that may be a hit
 * @accept_alternates: %TRUE to accept ASCII alternates
 *
 * Checks if a search conducted for @search_term should match
 * @potential_hit.
 *
 * This function calls xstr_tokenize_and_fold() on both
 * @search_term and @potential_hit.  ASCII alternates are never taken
 * for @search_term but will be taken for @potential_hit according to
 * the value of @accept_alternates.
 *
 * A hit occurs when each folded token in @search_term is a prefix of a
 * folded token from @potential_hit.
 *
 * Depending on how you're performing the search, it will typically be
 * faster to call xstr_tokenize_and_fold() on each string in
 * your corpus and build an index on the returned folded tokens, then
 * call xstr_tokenize_and_fold() on the search term and
 * perform lookups into that index.
 *
 * As some examples, searching for ‘fred’ would match the potential hit
 * ‘Smith, Fred’ and also ‘Frédéric’.  Searching for ‘Fréd’ would match
 * ‘Frédéric’ but not ‘Frederic’ (due to the one-directional nature of
 * accent matching).  Searching ‘fo’ would match ‘foo_t’ and ‘Bar foo_t
 * baz_t’, but not ‘SFO’ (because no word has ‘fo’ as a prefix).
 *
 * Returns: %TRUE if @potential_hit is a hit
 *
 * Since: 2.40
 **/
xboolean_t
xstr_match_string (const xchar_t *search_term,
                    const xchar_t *potential_hit,
                    xboolean_t     accept_alternates)
{
  xchar_t **alternates = NULL;
  xchar_t **term_tokens;
  xchar_t **hit_tokens;
  xboolean_t matched;
  xint_t i, j;

  g_return_val_if_fail (search_term != NULL, FALSE);
  g_return_val_if_fail (potential_hit != NULL, FALSE);

  term_tokens = xstr_tokenize_and_fold (search_term, NULL, NULL);
  hit_tokens = xstr_tokenize_and_fold (potential_hit, NULL, accept_alternates ? &alternates : NULL);

  matched = TRUE;

  for (i = 0; term_tokens[i]; i++)
    {
      for (j = 0; hit_tokens[j]; j++)
        if (xstr_has_prefix (hit_tokens[j], term_tokens[i]))
          goto one_matched;

      if (accept_alternates)
        for (j = 0; alternates[j]; j++)
          if (xstr_has_prefix (alternates[j], term_tokens[i]))
            goto one_matched;

      matched = FALSE;
      break;

one_matched:
      continue;
    }

  xstrfreev (term_tokens);
  xstrfreev (hit_tokens);
  xstrfreev (alternates);

  return matched;
}

/**
 * xstrv_contains:
 * @strv: a %NULL-terminated array of strings
 * @str: a string
 *
 * Checks if @strv contains @str. @strv must not be %NULL.
 *
 * Returns: %TRUE if @str is an element of @strv, according to xstr_equal().
 *
 * Since: 2.44
 */
xboolean_t
xstrv_contains (const xchar_t * const *strv,
                 const xchar_t         *str)
{
  g_return_val_if_fail (strv != NULL, FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  for (; *strv != NULL; strv++)
    {
      if (xstr_equal (str, *strv))
        return TRUE;
    }

  return FALSE;
}

/**
 * xstrv_equal:
 * @strv1: a %NULL-terminated array of strings
 * @strv2: another %NULL-terminated array of strings
 *
 * Checks if @strv1 and @strv2 contain exactly the same elements in exactly the
 * same order. Elements are compared using xstr_equal(). To match independently
 * of order, sort the arrays first (using g_qsort_with_data() or similar).
 *
 * Two empty arrays are considered equal. Neither @strv1 not @strv2 may be
 * %NULL.
 *
 * Returns: %TRUE if @strv1 and @strv2 are equal
 * Since: 2.60
 */
xboolean_t
xstrv_equal (const xchar_t * const *strv1,
              const xchar_t * const *strv2)
{
  g_return_val_if_fail (strv1 != NULL, FALSE);
  g_return_val_if_fail (strv2 != NULL, FALSE);

  if (strv1 == strv2)
    return TRUE;

  for (; *strv1 != NULL && *strv2 != NULL; strv1++, strv2++)
    {
      if (!xstr_equal (*strv1, *strv2))
        return FALSE;
    }

  return (*strv1 == NULL && *strv2 == NULL);
}

static xboolean_t
str_has_sign (const xchar_t *str)
{
  return str[0] == '-' || str[0] == '+';
}

static xboolean_t
str_has_hex_prefix (const xchar_t *str)
{
  return str[0] == '0' && g_ascii_tolower (str[1]) == 'x';
}

/**
 * g_ascii_string_to_signed:
 * @str: a string
 * @base: base of a parsed number
 * @min: a lower bound (inclusive)
 * @max: an upper bound (inclusive)
 * @out_num: (out) (optional): a return location for a number
 * @error: a return location for #xerror_t
 *
 * A convenience function for converting a string to a signed number.
 *
 * This function assumes that @str contains only a number of the given
 * @base that is within inclusive bounds limited by @min and @max. If
 * this is true, then the converted number is stored in @out_num. An
 * empty string is not a valid input. A string with leading or
 * trailing whitespace is also an invalid input.
 *
 * @base can be between 2 and 36 inclusive. Hexadecimal numbers must
 * not be prefixed with "0x" or "0X". Such a problem does not exist
 * for octal numbers, since they were usually prefixed with a zero
 * which does not change the value of the parsed number.
 *
 * Parsing failures result in an error with the %G_NUMBER_PARSER_ERROR
 * domain. If the input is invalid, the error code will be
 * %G_NUMBER_PARSER_ERROR_INVALID. If the parsed number is out of
 * bounds - %G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS.
 *
 * See g_ascii_strtoll() if you have more complex needs such as
 * parsing a string which starts with a number, but then has other
 * characters.
 *
 * Returns: %TRUE if @str was a number, otherwise %FALSE.
 *
 * Since: 2.54
 */
xboolean_t
g_ascii_string_to_signed (const xchar_t  *str,
                          xuint_t         base,
                          gint64        min,
                          gint64        max,
                          gint64       *out_num,
                          xerror_t      **error)
{
  gint64 number;
  const xchar_t *end_ptr = NULL;
  xint_t saved_errno = 0;

  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (base >= 2 && base <= 36, FALSE);
  g_return_val_if_fail (min <= max, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (str[0] == '\0')
    {
      g_set_error_literal (error,
                           G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_INVALID,
                           _("Empty string is not a number"));
      return FALSE;
    }

  errno = 0;
  number = g_ascii_strtoll (str, (xchar_t **)&end_ptr, base);
  saved_errno = errno;

  if (/* We do not allow leading whitespace, but g_ascii_strtoll
       * accepts it and just skips it, so we need to check for it
       * ourselves.
       */
      g_ascii_isspace (str[0]) ||
      /* We don't support hexadecimal numbers prefixed with 0x or
       * 0X.
       */
      (base == 16 &&
       (str_has_sign (str) ? str_has_hex_prefix (str + 1) : str_has_hex_prefix (str))) ||
      (saved_errno != 0 && saved_errno != ERANGE) ||
      end_ptr == NULL ||
      *end_ptr != '\0')
    {
      g_set_error (error,
                   G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_INVALID,
                   _("“%s” is not a signed number"), str);
      return FALSE;
    }
  if (saved_errno == ERANGE || number < min || number > max)
    {
      xchar_t *min_str = xstrdup_printf ("%" G_GINT64_FORMAT, min);
      xchar_t *max_str = xstrdup_printf ("%" G_GINT64_FORMAT, max);

      g_set_error (error,
                   G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS,
                   _("Number “%s” is out of bounds [%s, %s]"),
                   str, min_str, max_str);
      g_free (min_str);
      g_free (max_str);
      return FALSE;
    }
  if (out_num != NULL)
    *out_num = number;
  return TRUE;
}

/**
 * g_ascii_string_to_unsigned:
 * @str: a string
 * @base: base of a parsed number
 * @min: a lower bound (inclusive)
 * @max: an upper bound (inclusive)
 * @out_num: (out) (optional): a return location for a number
 * @error: a return location for #xerror_t
 *
 * A convenience function for converting a string to an unsigned number.
 *
 * This function assumes that @str contains only a number of the given
 * @base that is within inclusive bounds limited by @min and @max. If
 * this is true, then the converted number is stored in @out_num. An
 * empty string is not a valid input. A string with leading or
 * trailing whitespace is also an invalid input. A string with a leading sign
 * (`-` or `+`) is not a valid input for the unsigned parser.
 *
 * @base can be between 2 and 36 inclusive. Hexadecimal numbers must
 * not be prefixed with "0x" or "0X". Such a problem does not exist
 * for octal numbers, since they were usually prefixed with a zero
 * which does not change the value of the parsed number.
 *
 * Parsing failures result in an error with the %G_NUMBER_PARSER_ERROR
 * domain. If the input is invalid, the error code will be
 * %G_NUMBER_PARSER_ERROR_INVALID. If the parsed number is out of
 * bounds - %G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS.
 *
 * See g_ascii_strtoull() if you have more complex needs such as
 * parsing a string which starts with a number, but then has other
 * characters.
 *
 * Returns: %TRUE if @str was a number, otherwise %FALSE.
 *
 * Since: 2.54
 */
xboolean_t
g_ascii_string_to_unsigned (const xchar_t  *str,
                            xuint_t         base,
                            xuint64_t       min,
                            xuint64_t       max,
                            xuint64_t      *out_num,
                            xerror_t      **error)
{
  xuint64_t number;
  const xchar_t *end_ptr = NULL;
  xint_t saved_errno = 0;

  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (base >= 2 && base <= 36, FALSE);
  g_return_val_if_fail (min <= max, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (str[0] == '\0')
    {
      g_set_error_literal (error,
                           G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_INVALID,
                           _("Empty string is not a number"));
      return FALSE;
    }

  errno = 0;
  number = g_ascii_strtoull (str, (xchar_t **)&end_ptr, base);
  saved_errno = errno;

  if (/* We do not allow leading whitespace, but g_ascii_strtoull
       * accepts it and just skips it, so we need to check for it
       * ourselves.
       */
      g_ascii_isspace (str[0]) ||
      /* Unsigned number should have no sign.
       */
      str_has_sign (str) ||
      /* We don't support hexadecimal numbers prefixed with 0x or
       * 0X.
       */
      (base == 16 && str_has_hex_prefix (str)) ||
      (saved_errno != 0 && saved_errno != ERANGE) ||
      end_ptr == NULL ||
      *end_ptr != '\0')
    {
      g_set_error (error,
                   G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_INVALID,
                   _("“%s” is not an unsigned number"), str);
      return FALSE;
    }
  if (saved_errno == ERANGE || number < min || number > max)
    {
      xchar_t *min_str = xstrdup_printf ("%" G_GUINT64_FORMAT, min);
      xchar_t *max_str = xstrdup_printf ("%" G_GUINT64_FORMAT, max);

      g_set_error (error,
                   G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS,
                   _("Number “%s” is out of bounds [%s, %s]"),
                   str, min_str, max_str);
      g_free (min_str);
      g_free (max_str);
      return FALSE;
    }
  if (out_num != NULL)
    *out_num = number;
  return TRUE;
}

G_DEFINE_QUARK (g-number-parser-error-quark, g_number_parser_error)
