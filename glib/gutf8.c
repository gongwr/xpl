/* gutf8.c - Operations on UTF-8 strings.
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"

#include <stdlib.h>
#ifdef HAVE_CODESET
#include <langinfo.h>
#endif
#include <string.h>

#ifdef XPLATFORM_WIN32
#include <stdio.h>
#define STRICT
#include <windows.h>
#undef STRICT
#endif

#include "gconvert.h"
#include "ghash.h"
#include "gstrfuncs.h"
#include "gtestutils.h"
#include "gtypes.h"
#include "gthread.h"
#include "glibintl.h"

#define UTF8_COMPUTE(Char, Mask, Len)					      \
  if (Char < 128)							      \
    {									      \
      Len = 1;								      \
      Mask = 0x7f;							      \
    }									      \
  else if ((Char & 0xe0) == 0xc0)					      \
    {									      \
      Len = 2;								      \
      Mask = 0x1f;							      \
    }									      \
  else if ((Char & 0xf0) == 0xe0)					      \
    {									      \
      Len = 3;								      \
      Mask = 0x0f;							      \
    }									      \
  else if ((Char & 0xf8) == 0xf0)					      \
    {									      \
      Len = 4;								      \
      Mask = 0x07;							      \
    }									      \
  else if ((Char & 0xfc) == 0xf8)					      \
    {									      \
      Len = 5;								      \
      Mask = 0x03;							      \
    }									      \
  else if ((Char & 0xfe) == 0xfc)					      \
    {									      \
      Len = 6;								      \
      Mask = 0x01;							      \
    }									      \
  else									      \
    Len = -1;

#define UTF8_LENGTH(Char)              \
  ((Char) < 0x80 ? 1 :                 \
   ((Char) < 0x800 ? 2 :               \
    ((Char) < 0x10000 ? 3 :            \
     ((Char) < 0x200000 ? 4 :          \
      ((Char) < 0x4000000 ? 5 : 6)))))


#define UTF8_GET(Result, Chars, Count, Mask, Len)			      \
  (Result) = (Chars)[0] & (Mask);					      \
  for ((Count) = 1; (Count) < (Len); ++(Count))				      \
    {									      \
      if (((Chars)[(Count)] & 0xc0) != 0x80)				      \
	{								      \
	  (Result) = -1;						      \
	  break;							      \
	}								      \
      (Result) <<= 6;							      \
      (Result) |= ((Chars)[(Count)] & 0x3f);				      \
    }

/*
 * Check whether a Unicode (5.2) char is in a valid range.
 *
 * The first check comes from the Unicode guarantee to never encode
 * a point above 0x0010ffff, since UTF-16 couldn't represent it.
 *
 * The second check covers surrogate pairs (category Cs).
 *
 * @param Char the character
 */
#define UNICODE_VALID(Char)                   \
    ((Char) < 0x110000 &&                     \
     (((Char) & 0xFFFFF800) != 0xD800))


static const xchar_t utf8_skip_data[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

const xchar_t * const xutf8_skip = utf8_skip_data;

/**
 * xutf8_find_prev_char:
 * @str: pointer to the beginning of a UTF-8 encoded string
 * @p: pointer to some position within @str
 *
 * Given a position @p with a UTF-8 encoded string @str, find the start
 * of the previous UTF-8 character starting before @p. Returns %NULL if no
 * UTF-8 characters are present in @str before @p.
 *
 * @p does not have to be at the beginning of a UTF-8 character. No check
 * is made to see if the character found is actually valid other than
 * it starts with an appropriate byte.
 *
 * Returns: (transfer none) (nullable): a pointer to the found character or %NULL.
 */
xchar_t *
xutf8_find_prev_char (const xchar_t *str,
		       const xchar_t *p)
{
  while (p > str)
    {
      --p;
      if ((*p & 0xc0) != 0x80)
	return (xchar_t *)p;
    }
  return NULL;
}

/**
 * xutf8_find_next_char:
 * @p: a pointer to a position within a UTF-8 encoded string
 * @end: (nullable): a pointer to the byte following the end of the string,
 *     or %NULL to indicate that the string is nul-terminated
 *
 * Finds the start of the next UTF-8 character in the string after @p.
 *
 * @p does not have to be at the beginning of a UTF-8 character. No check
 * is made to see if the character found is actually valid other than
 * it starts with an appropriate byte.
 *
 * If @end is %NULL, the return value will never be %NULL: if the end of the
 * string is reached, a pointer to the terminating nul byte is returned. If
 * @end is non-%NULL, the return value will be %NULL if the end of the string
 * is reached.
 *
 * Returns: (transfer none) (nullable): a pointer to the found character or %NULL if @end is
 *    set and is reached
 */
xchar_t *
xutf8_find_next_char (const xchar_t *p,
		       const xchar_t *end)
{
  if (end)
    {
      for (++p; p < end && (*p & 0xc0) == 0x80; ++p)
        ;
      return (p >= end) ? NULL : (xchar_t *)p;
    }
  else
    {
      for (++p; (*p & 0xc0) == 0x80; ++p)
        ;
      return (xchar_t *)p;
    }
}

/**
 * xutf8_prev_char:
 * @p: a pointer to a position within a UTF-8 encoded string
 *
 * Finds the previous UTF-8 character in the string before @p.
 *
 * @p does not have to be at the beginning of a UTF-8 character. No check
 * is made to see if the character found is actually valid other than
 * it starts with an appropriate byte. If @p might be the first
 * character of the string, you must use xutf8_find_prev_char() instead.
 *
 * Returns: (transfer none) (not nullable): a pointer to the found character
 */
xchar_t *
xutf8_prev_char (const xchar_t *p)
{
  while (TRUE)
    {
      p--;
      if ((*p & 0xc0) != 0x80)
	return (xchar_t *)p;
    }
}

/**
 * xutf8_strlen:
 * @p: pointer to the start of a UTF-8 encoded string
 * @max: the maximum number of bytes to examine. If @max
 *       is less than 0, then the string is assumed to be
 *       nul-terminated. If @max is 0, @p will not be examined and
 *       may be %NULL. If @max is greater than 0, up to @max
 *       bytes are examined
 *
 * Computes the length of the string in characters, not including
 * the terminating nul character. If the @max'th byte falls in the
 * middle of a character, the last (partial) character is not counted.
 *
 * Returns: the length of the string in characters
 */
xlong_t
xutf8_strlen (const xchar_t *p,
               xssize_t       max)
{
  xlong_t len = 0;
  const xchar_t *start = p;
  g_return_val_if_fail (p != NULL || max == 0, 0);

  if (max < 0)
    {
      while (*p)
        {
          p = xutf8_next_char (p);
          ++len;
        }
    }
  else
    {
      if (max == 0 || !*p)
        return 0;

      p = xutf8_next_char (p);

      while (p - start < max && *p)
        {
          ++len;
          p = xutf8_next_char (p);
        }

      /* only do the last len increment if we got a complete
       * char (don't count partial chars)
       */
      if (p - start <= max)
        ++len;
    }

  return len;
}

/**
 * xutf8_substring:
 * @str: a UTF-8 encoded string
 * @start_pos: a character offset within @str
 * @end_pos: another character offset within @str,
 *   or `-1` to indicate the end of the string
 *
 * Copies a substring out of a UTF-8 encoded string.
 * The substring will contain @end_pos - @start_pos characters.
 *
 * Since GLib 2.72, `-1` can be passed to @end_pos to indicate the
 * end of the string.
 *
 * Returns: (transfer full): a newly allocated copy of the requested
 *     substring. Free with g_free() when no longer needed.
 *
 * Since: 2.30
 */
xchar_t *
xutf8_substring (const xchar_t *str,
                  xlong_t        start_pos,
                  xlong_t        end_pos)
{
  xchar_t *start, *end, *out;

  g_return_val_if_fail (end_pos >= start_pos || end_pos == -1, NULL);

  start = xutf8_offset_to_pointer (str, start_pos);

  if (end_pos == -1)
    {
      xlong_t length = xutf8_strlen (start, -1);
      end = xutf8_offset_to_pointer (start, length);
    }
  else
    {
      end = xutf8_offset_to_pointer (start, end_pos - start_pos);
    }

  out = g_malloc (end - start + 1);
  memcpy (out, start, end - start);
  out[end - start] = 0;

  return out;
}

/**
 * xutf8_get_char:
 * @p: a pointer to Unicode character encoded as UTF-8
 *
 * Converts a sequence of bytes encoded as UTF-8 to a Unicode character.
 *
 * If @p does not point to a valid UTF-8 encoded character, results
 * are undefined. If you are not sure that the bytes are complete
 * valid Unicode characters, you should use xutf8_get_char_validated()
 * instead.
 *
 * Returns: the resulting character
 */
xunichar_t
xutf8_get_char (const xchar_t *p)
{
  int i, mask = 0, len;
  xunichar_t result;
  unsigned char c = (unsigned char) *p;

  UTF8_COMPUTE (c, mask, len);
  if (len == -1)
    return (xunichar_t)-1;
  UTF8_GET (result, p, i, mask, len);

  return result;
}

/**
 * xutf8_offset_to_pointer:
 * @str: a UTF-8 encoded string
 * @offset: a character offset within @str
 *
 * Converts from an integer character offset to a pointer to a position
 * within the string.
 *
 * Since 2.10, this function allows to pass a negative @offset to
 * step backwards. It is usually worth stepping backwards from the end
 * instead of forwards if @offset is in the last fourth of the string,
 * since moving forward is about 3 times faster than moving backward.
 *
 * Note that this function doesn't abort when reaching the end of @str.
 * Therefore you should be sure that @offset is within string boundaries
 * before calling that function. Call xutf8_strlen() when unsure.
 * This limitation exists as this function is called frequently during
 * text rendering and therefore has to be as fast as possible.
 *
 * Returns: (transfer none): the resulting pointer
 */
xchar_t *
xutf8_offset_to_pointer  (const xchar_t *str,
			   xlong_t        offset)
{
  const xchar_t *s = str;

  if (offset > 0)
    while (offset--)
      s = xutf8_next_char (s);
  else
    {
      const char *s1;

      /* This nice technique for fast backwards stepping
       * through a UTF-8 string was dubbed "stutter stepping"
       * by its inventor, Larry Ewing.
       */
      while (offset)
	{
	  s1 = s;
	  s += offset;
	  while ((*s & 0xc0) == 0x80)
	    s--;

	  offset += xutf8_pointer_to_offset (s, s1);
	}
    }

  return (xchar_t *)s;
}

/**
 * xutf8_pointer_to_offset:
 * @str: a UTF-8 encoded string
 * @pos: a pointer to a position within @str
 *
 * Converts from a pointer to position within a string to an integer
 * character offset.
 *
 * Since 2.10, this function allows @pos to be before @str, and returns
 * a negative offset in this case.
 *
 * Returns: the resulting character offset
 */
xlong_t
xutf8_pointer_to_offset (const xchar_t *str,
			  const xchar_t *pos)
{
  const xchar_t *s = str;
  xlong_t offset = 0;

  if (pos < str)
    offset = - xutf8_pointer_to_offset (pos, str);
  else
    while (s < pos)
      {
	s = xutf8_next_char (s);
	offset++;
      }

  return offset;
}


/**
 * xutf8_strncpy:
 * @dest: (transfer none): buffer to fill with characters from @src
 * @src: UTF-8 encoded string
 * @n: character count
 *
 * Like the standard C strncpy() function, but copies a given number
 * of characters instead of a given number of bytes. The @src string
 * must be valid UTF-8 encoded text. (Use xutf8_validate() on all
 * text before trying to use UTF-8 utility functions with it.)
 *
 * Note you must ensure @dest is at least 4 * @n to fit the
 * largest possible UTF-8 characters
 *
 * Returns: (transfer none): @dest
 */
xchar_t *
xutf8_strncpy (xchar_t       *dest,
		const xchar_t *src,
		xsize_t        n)
{
  const xchar_t *s = src;
  while (n && *s)
    {
      s = xutf8_next_char(s);
      n--;
    }
  strncpy(dest, src, s - src);
  dest[s - src] = 0;
  return dest;
}

/* unicode_strchr */

/**
 * xunichar_to_utf8:
 * @c: a Unicode character code
 * @outbuf: (out caller-allocates) (optional): output buffer, must have at
 *       least 6 bytes of space. If %NULL, the length will be computed and
 *       returned and nothing will be written to @outbuf.
 *
 * Converts a single character to UTF-8.
 *
 * Returns: number of bytes written
 */
int
xunichar_to_utf8 (xunichar_t c,
		   xchar_t   *outbuf)
{
  /* If this gets modified, also update the copy in xstring_insert_unichar() */
  xuint_t len = 0;
  int first;
  int i;

  if (c < 0x80)
    {
      first = 0;
      len = 1;
    }
  else if (c < 0x800)
    {
      first = 0xc0;
      len = 2;
    }
  else if (c < 0x10000)
    {
      first = 0xe0;
      len = 3;
    }
   else if (c < 0x200000)
    {
      first = 0xf0;
      len = 4;
    }
  else if (c < 0x4000000)
    {
      first = 0xf8;
      len = 5;
    }
  else
    {
      first = 0xfc;
      len = 6;
    }

  if (outbuf)
    {
      for (i = len - 1; i > 0; --i)
	{
	  outbuf[i] = (c & 0x3f) | 0x80;
	  c >>= 6;
	}
      outbuf[0] = c | first;
    }

  return len;
}

/**
 * xutf8_strchr:
 * @p: a nul-terminated UTF-8 encoded string
 * @len: the maximum length of @p
 * @c: a Unicode character
 *
 * Finds the leftmost occurrence of the given Unicode character
 * in a UTF-8 encoded string, while limiting the search to @len bytes.
 * If @len is -1, allow unbounded search.
 *
 * Returns: (transfer none) (nullable): %NULL if the string does not contain the character,
 *     otherwise, a pointer to the start of the leftmost occurrence
 *     of the character in the string.
 */
xchar_t *
xutf8_strchr (const char *p,
	       xssize_t      len,
	       xunichar_t    c)
{
  xchar_t ch[10];

  xint_t charlen = xunichar_to_utf8 (c, ch);
  ch[charlen] = '\0';

  return xstrstr_len (p, len, ch);
}


/**
 * xutf8_strrchr:
 * @p: a nul-terminated UTF-8 encoded string
 * @len: the maximum length of @p
 * @c: a Unicode character
 *
 * Find the rightmost occurrence of the given Unicode character
 * in a UTF-8 encoded string, while limiting the search to @len bytes.
 * If @len is -1, allow unbounded search.
 *
 * Returns: (transfer none) (nullable): %NULL if the string does not contain the character,
 *     otherwise, a pointer to the start of the rightmost occurrence
 *     of the character in the string.
 */
xchar_t *
xutf8_strrchr (const char *p,
		xssize_t      len,
		xunichar_t    c)
{
  xchar_t ch[10];

  xint_t charlen = xunichar_to_utf8 (c, ch);
  ch[charlen] = '\0';

  return xstrrstr_len (p, len, ch);
}


/* Like xutf8_get_char, but take a maximum length
 * and return (xunichar_t)-2 on incomplete trailing character;
 * also check for malformed or overlong sequences
 * and return (xunichar_t)-1 in this case.
 */
static inline xunichar_t
xutf8_get_char_extended (const  xchar_t *p,
			  xssize_t max_len)
{
  xsize_t i, len;
  xunichar_t min_code;
  xunichar_t wc = (xuchar_t) *p;
  const xunichar_t partial_sequence = (xunichar_t) -2;
  const xunichar_t malformed_sequence = (xunichar_t) -1;

  if (wc < 0x80)
    {
      return wc;
    }
  else if (G_UNLIKELY (wc < 0xc0))
    {
      return malformed_sequence;
    }
  else if (wc < 0xe0)
    {
      len = 2;
      wc &= 0x1f;
      min_code = 1 << 7;
    }
  else if (wc < 0xf0)
    {
      len = 3;
      wc &= 0x0f;
      min_code = 1 << 11;
    }
  else if (wc < 0xf8)
    {
      len = 4;
      wc &= 0x07;
      min_code = 1 << 16;
    }
  else if (wc < 0xfc)
    {
      len = 5;
      wc &= 0x03;
      min_code = 1 << 21;
    }
  else if (wc < 0xfe)
    {
      len = 6;
      wc &= 0x01;
      min_code = 1 << 26;
    }
  else
    {
      return malformed_sequence;
    }

  if (G_UNLIKELY (max_len >= 0 && len > (xsize_t) max_len))
    {
      for (i = 1; i < (xsize_t) max_len; i++)
	{
	  if ((((xuchar_t *)p)[i] & 0xc0) != 0x80)
	    return malformed_sequence;
	}
      return partial_sequence;
    }

  for (i = 1; i < len; ++i)
    {
      xunichar_t ch = ((xuchar_t *)p)[i];

      if (G_UNLIKELY ((ch & 0xc0) != 0x80))
	{
	  if (ch)
	    return malformed_sequence;
	  else
	    return partial_sequence;
	}

      wc <<= 6;
      wc |= (ch & 0x3f);
    }

  if (G_UNLIKELY (wc < min_code))
    return malformed_sequence;

  return wc;
}

/**
 * xutf8_get_char_validated:
 * @p: a pointer to Unicode character encoded as UTF-8
 * @max_len: the maximum number of bytes to read, or -1 if @p is nul-terminated
 *
 * Convert a sequence of bytes encoded as UTF-8 to a Unicode character.
 * This function checks for incomplete characters, for invalid characters
 * such as characters that are out of the range of Unicode, and for
 * overlong encodings of valid characters.
 *
 * Note that xutf8_get_char_validated() returns (xunichar_t)-2 if
 * @max_len is positive and any of the bytes in the first UTF-8 character
 * sequence are nul.
 *
 * Returns: the resulting character. If @p points to a partial
 *     sequence at the end of a string that could begin a valid
 *     character (or if @max_len is zero), returns (xunichar_t)-2;
 *     otherwise, if @p does not point to a valid UTF-8 encoded
 *     Unicode character, returns (xunichar_t)-1.
 */
xunichar_t
xutf8_get_char_validated (const xchar_t *p,
			   xssize_t       max_len)
{
  xunichar_t result;

  if (max_len == 0)
    return (xunichar_t)-2;

  result = xutf8_get_char_extended (p, max_len);

  /* Disallow codepoint U+0000 as it’s a nul byte,
   * and all string handling in GLib is nul-terminated */
  if (result == 0 && max_len > 0)
    return (xunichar_t) -2;

  if (result & 0x80000000)
    return result;
  else if (!UNICODE_VALID (result))
    return (xunichar_t)-1;
  else
    return result;
}

#define CONT_BYTE_FAST(p) ((xuchar_t)*p++ & 0x3f)

/**
 * xutf8_to_ucs4_fast:
 * @str: a UTF-8 encoded string
 * @len: the maximum length of @str to use, in bytes. If @len < 0,
 *     then the string is nul-terminated.
 * @items_written: (out) (optional): location to store the
 *     number of characters in the result, or %NULL.
 *
 * Convert a string from UTF-8 to a 32-bit fixed width
 * representation as UCS-4, assuming valid UTF-8 input.
 * This function is roughly twice as fast as xutf8_to_ucs4()
 * but does no error checking on the input. A trailing 0 character
 * will be added to the string after the converted text.
 *
 * Returns: (transfer full): a pointer to a newly allocated UCS-4 string.
 *     This value must be freed with g_free().
 */
xunichar_t *
xutf8_to_ucs4_fast (const xchar_t *str,
		     xlong_t        len,
		     xlong_t       *items_written)
{
  xunichar_t *result;
  xint_t n_chars, i;
  const xchar_t *p;

  g_return_val_if_fail (str != NULL, NULL);

  p = str;
  n_chars = 0;
  if (len < 0)
    {
      while (*p)
	{
	  p = xutf8_next_char (p);
	  ++n_chars;
	}
    }
  else
    {
      while (p < str + len && *p)
	{
	  p = xutf8_next_char (p);
	  ++n_chars;
	}
    }

  result = g_new (xunichar_t, n_chars + 1);

  p = str;
  for (i=0; i < n_chars; i++)
    {
      xuchar_t first = (xuchar_t)*p++;
      xunichar_t wc;

      if (first < 0xc0)
	{
          /* We really hope first < 0x80, but we don't want to test an
           * extra branch for invalid input, which this function
           * does not care about. Handling unexpected continuation bytes
           * here will do the least damage. */
	  wc = first;
	}
      else
	{
          xunichar_t c1 = CONT_BYTE_FAST(p);
          if (first < 0xe0)
            {
              wc = ((first & 0x1f) << 6) | c1;
            }
          else
            {
              xunichar_t c2 = CONT_BYTE_FAST(p);
              if (first < 0xf0)
                {
                  wc = ((first & 0x0f) << 12) | (c1 << 6) | c2;
                }
              else
                {
                  xunichar_t c3 = CONT_BYTE_FAST(p);
                  wc = ((first & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
                  if (G_UNLIKELY (first >= 0xf8))
                    {
                      /* This can't be valid UTF-8, but xutf8_next_char()
                       * and company allow out-of-range sequences */
                      xunichar_t mask = 1 << 20;
                      while ((wc & mask) != 0)
                        {
                          wc <<= 6;
                          wc |= CONT_BYTE_FAST(p);
                          mask <<= 5;
                        }
                      wc &= mask - 1;
                    }
                }
            }
	}
      result[i] = wc;
    }
  result[i] = 0;

  if (items_written)
    *items_written = i;

  return result;
}

static xpointer_t
try_malloc_n (xsize_t n_blocks, xsize_t n_block_bytes, xerror_t **error)
{
    xpointer_t ptr = g_try_malloc_n (n_blocks, n_block_bytes);
    if (ptr == NULL)
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_NO_MEMORY,
                           _("Failed to allocate memory"));
    return ptr;
}

/**
 * xutf8_to_ucs4:
 * @str: a UTF-8 encoded string
 * @len: the maximum length of @str to use, in bytes. If @len < 0,
 *     then the string is nul-terminated.
 * @items_read: (out) (optional): location to store number of
  *    bytes read, or %NULL.
 *     If %NULL, then %G_CONVERT_ERROR_PARTIAL_INPUT will be
 *     returned in case @str contains a trailing partial
 *     character. If an error occurs then the index of the
 *     invalid input is stored here.
 * @items_written: (out) (optional): location to store number
 *     of characters written or %NULL. The value here stored does not include
 *     the trailing 0 character.
 * @error: location to store the error occurring, or %NULL to ignore
 *     errors. Any of the errors in #GConvertError other than
 *     %G_CONVERT_ERROR_NO_CONVERSION may occur.
 *
 * Convert a string from UTF-8 to a 32-bit fixed width
 * representation as UCS-4. A trailing 0 character will be added to the
 * string after the converted text.
 *
 * Returns: (transfer full): a pointer to a newly allocated UCS-4 string.
 *     This value must be freed with g_free(). If an error occurs,
 *     %NULL will be returned and @error set.
 */
xunichar_t *
xutf8_to_ucs4 (const xchar_t *str,
		xlong_t        len,
		xlong_t       *items_read,
		xlong_t       *items_written,
		xerror_t     **error)
{
  xunichar_t *result = NULL;
  xint_t n_chars, i;
  const xchar_t *in;

  in = str;
  n_chars = 0;
  while ((len < 0 || str + len - in > 0) && *in)
    {
      xunichar_t wc = xutf8_get_char_extended (in, len < 0 ? 6 : str + len - in);
      if (wc & 0x80000000)
	{
	  if (wc == (xunichar_t)-2)
	    {
	      if (items_read)
		break;
	      else
		g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                                     _("Partial character sequence at end of input"));
	    }
	  else
	    g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                                 _("Invalid byte sequence in conversion input"));

	  goto err_out;
	}

      n_chars++;

      in = xutf8_next_char (in);
    }

  result = try_malloc_n (n_chars + 1, sizeof (xunichar_t), error);
  if (result == NULL)
      goto err_out;

  in = str;
  for (i=0; i < n_chars; i++)
    {
      result[i] = xutf8_get_char (in);
      in = xutf8_next_char (in);
    }
  result[i] = 0;

  if (items_written)
    *items_written = n_chars;

 err_out:
  if (items_read)
    *items_read = in - str;

  return result;
}

/**
 * g_ucs4_to_utf8:
 * @str: a UCS-4 encoded string
 * @len: the maximum length (number of characters) of @str to use.
 *     If @len < 0, then the string is nul-terminated.
 * @items_read: (out) (optional): location to store number of
 *     characters read, or %NULL.
 * @items_written: (out) (optional): location to store number
 *     of bytes written or %NULL. The value here stored does not include the
 *     trailing 0 byte.
 * @error: location to store the error occurring, or %NULL to ignore
 *         errors. Any of the errors in #GConvertError other than
 *         %G_CONVERT_ERROR_NO_CONVERSION may occur.
 *
 * Convert a string from a 32-bit fixed width representation as UCS-4.
 * to UTF-8. The result will be terminated with a 0 byte.
 *
 * Returns: (transfer full): a pointer to a newly allocated UTF-8 string.
 *     This value must be freed with g_free(). If an error occurs,
 *     %NULL will be returned and @error set. In that case, @items_read
 *     will be set to the position of the first invalid input character.
 */
xchar_t *
g_ucs4_to_utf8 (const xunichar_t *str,
		xlong_t           len,
		xlong_t          *items_read,
		xlong_t          *items_written,
		xerror_t        **error)
{
  xint_t result_length;
  xchar_t *result = NULL;
  xchar_t *p;
  xint_t i;

  result_length = 0;
  for (i = 0; len < 0 || i < len ; i++)
    {
      if (!str[i])
	break;

      if (str[i] >= 0x80000000)
	{
	  g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                               _("Character out of range for UTF-8"));
	  goto err_out;
	}

      result_length += UTF8_LENGTH (str[i]);
    }

  result = try_malloc_n (result_length + 1, 1, error);
  if (result == NULL)
      goto err_out;

  p = result;

  i = 0;
  while (p < result + result_length)
    p += xunichar_to_utf8 (str[i++], p);

  *p = '\0';

  if (items_written)
    *items_written = p - result;

 err_out:
  if (items_read)
    *items_read = i;

  return result;
}

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

/**
 * xutf16_to_utf8:
 * @str: a UTF-16 encoded string
 * @len: the maximum length (number of #xunichar2_t) of @str to use.
 *     If @len < 0, then the string is nul-terminated.
 * @items_read: (out) (optional): location to store number of
 *     words read, or %NULL. If %NULL, then %G_CONVERT_ERROR_PARTIAL_INPUT will
 *     be returned in case @str contains a trailing partial character. If
 *     an error occurs then the index of the invalid input is stored here.
 *     It’s guaranteed to be non-negative.
 * @items_written: (out) (optional): location to store number
 *     of bytes written, or %NULL. The value stored here does not include the
 *     trailing 0 byte. It’s guaranteed to be non-negative.
 * @error: location to store the error occurring, or %NULL to ignore
 *     errors. Any of the errors in #GConvertError other than
 *     %G_CONVERT_ERROR_NO_CONVERSION may occur.
 *
 * Convert a string from UTF-16 to UTF-8. The result will be
 * terminated with a 0 byte.
 *
 * Note that the input is expected to be already in native endianness,
 * an initial byte-order-mark character is not handled specially.
 * g_convert() can be used to convert a byte buffer of UTF-16 data of
 * ambiguous endianness.
 *
 * Further note that this function does not validate the result
 * string; it may e.g. include embedded NUL characters. The only
 * validation done by this function is to ensure that the input can
 * be correctly interpreted as UTF-16, i.e. it doesn't contain
 * unpaired surrogates or partial character sequences.
 *
 * Returns: (transfer full): a pointer to a newly allocated UTF-8 string.
 *     This value must be freed with g_free(). If an error occurs,
 *     %NULL will be returned and @error set.
 **/
xchar_t *
xutf16_to_utf8 (const xunichar2_t  *str,
		 xlong_t             len,
		 xlong_t            *items_read,
		 xlong_t            *items_written,
		 xerror_t          **error)
{
  /* This function and xutf16_to_ucs4 are almost exactly identical -
   * The lines that differ are marked.
   */
  const xunichar2_t *in;
  xchar_t *out;
  xchar_t *result = NULL;
  xint_t n_bytes;
  xunichar_t high_surrogate;

  g_return_val_if_fail (str != NULL, NULL);

  n_bytes = 0;
  in = str;
  high_surrogate = 0;
  while ((len < 0 || in - str < len) && *in)
    {
      xunichar2_t c = *in;
      xunichar_t wc;

      if (c >= 0xdc00 && c < 0xe000) /* low surrogate */
	{
	  if (high_surrogate)
	    {
	      wc = SURROGATE_VALUE (high_surrogate, c);
	      high_surrogate = 0;
	    }
	  else
	    {
	      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                                   _("Invalid sequence in conversion input"));
	      goto err_out;
	    }
	}
      else
	{
	  if (high_surrogate)
	    {
	      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                                   _("Invalid sequence in conversion input"));
	      goto err_out;
	    }

	  if (c >= 0xd800 && c < 0xdc00) /* high surrogate */
	    {
	      high_surrogate = c;
	      goto next1;
	    }
	  else
	    wc = c;
	}

      /********** DIFFERENT for UTF8/UCS4 **********/
      n_bytes += UTF8_LENGTH (wc);

    next1:
      in++;
    }

  if (high_surrogate && !items_read)
    {
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                           _("Partial character sequence at end of input"));
      goto err_out;
    }

  /* At this point, everything is valid, and we just need to convert
   */
  /********** DIFFERENT for UTF8/UCS4 **********/
  result = try_malloc_n (n_bytes + 1, 1, error);
  if (result == NULL)
      goto err_out;

  high_surrogate = 0;
  out = result;
  in = str;
  while (out < result + n_bytes)
    {
      xunichar2_t c = *in;
      xunichar_t wc;

      if (c >= 0xdc00 && c < 0xe000) /* low surrogate */
	{
	  wc = SURROGATE_VALUE (high_surrogate, c);
	  high_surrogate = 0;
	}
      else if (c >= 0xd800 && c < 0xdc00) /* high surrogate */
	{
	  high_surrogate = c;
	  goto next2;
	}
      else
	wc = c;

      /********** DIFFERENT for UTF8/UCS4 **********/
      out += xunichar_to_utf8 (wc, out);

    next2:
      in++;
    }

  /********** DIFFERENT for UTF8/UCS4 **********/
  *out = '\0';

  if (items_written)
    /********** DIFFERENT for UTF8/UCS4 **********/
    *items_written = out - result;

 err_out:
  if (items_read)
    *items_read = in - str;

  return result;
}

/**
 * xutf16_to_ucs4:
 * @str: a UTF-16 encoded string
 * @len: the maximum length (number of #xunichar2_t) of @str to use.
 *     If @len < 0, then the string is nul-terminated.
 * @items_read: (out) (optional): location to store number of
 *     words read, or %NULL. If %NULL, then %G_CONVERT_ERROR_PARTIAL_INPUT will
 *     be returned in case @str contains a trailing partial character. If
 *     an error occurs then the index of the invalid input is stored here.
 * @items_written: (out) (optional): location to store number
 *     of characters written, or %NULL. The value stored here does not include
 *     the trailing 0 character.
 * @error: location to store the error occurring, or %NULL to ignore
 *     errors. Any of the errors in #GConvertError other than
 *     %G_CONVERT_ERROR_NO_CONVERSION may occur.
 *
 * Convert a string from UTF-16 to UCS-4. The result will be
 * nul-terminated.
 *
 * Returns: (transfer full): a pointer to a newly allocated UCS-4 string.
 *     This value must be freed with g_free(). If an error occurs,
 *     %NULL will be returned and @error set.
 */
xunichar_t *
xutf16_to_ucs4 (const xunichar2_t  *str,
		 xlong_t             len,
		 xlong_t            *items_read,
		 xlong_t            *items_written,
		 xerror_t          **error)
{
  const xunichar2_t *in;
  xchar_t *out;
  xchar_t *result = NULL;
  xint_t n_bytes;
  xunichar_t high_surrogate;

  g_return_val_if_fail (str != NULL, NULL);

  n_bytes = 0;
  in = str;
  high_surrogate = 0;
  while ((len < 0 || in - str < len) && *in)
    {
      xunichar2_t c = *in;

      if (c >= 0xdc00 && c < 0xe000) /* low surrogate */
	{
	  if (high_surrogate)
	    {
	      high_surrogate = 0;
	    }
	  else
	    {
	      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                                   _("Invalid sequence in conversion input"));
	      goto err_out;
	    }
	}
      else
	{
	  if (high_surrogate)
	    {
	      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                                   _("Invalid sequence in conversion input"));
	      goto err_out;
	    }

	  if (c >= 0xd800 && c < 0xdc00) /* high surrogate */
	    {
	      high_surrogate = c;
	      goto next1;
	    }
	}

      /********** DIFFERENT for UTF8/UCS4 **********/
      n_bytes += sizeof (xunichar_t);

    next1:
      in++;
    }

  if (high_surrogate && !items_read)
    {
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                           _("Partial character sequence at end of input"));
      goto err_out;
    }

  /* At this point, everything is valid, and we just need to convert
   */
  /********** DIFFERENT for UTF8/UCS4 **********/
  result = try_malloc_n (n_bytes + 4, 1, error);
  if (result == NULL)
      goto err_out;

  high_surrogate = 0;
  out = result;
  in = str;
  while (out < result + n_bytes)
    {
      xunichar2_t c = *in;
      xunichar_t wc;

      if (c >= 0xdc00 && c < 0xe000) /* low surrogate */
	{
	  wc = SURROGATE_VALUE (high_surrogate, c);
	  high_surrogate = 0;
	}
      else if (c >= 0xd800 && c < 0xdc00) /* high surrogate */
	{
	  high_surrogate = c;
	  goto next2;
	}
      else
	wc = c;

      /********** DIFFERENT for UTF8/UCS4 **********/
      *(xunichar_t *)out = wc;
      out += sizeof (xunichar_t);

    next2:
      in++;
    }

  /********** DIFFERENT for UTF8/UCS4 **********/
  *(xunichar_t *)out = 0;

  if (items_written)
    /********** DIFFERENT for UTF8/UCS4 **********/
    *items_written = (out - result) / sizeof (xunichar_t);

 err_out:
  if (items_read)
    *items_read = in - str;

  return (xunichar_t *)result;
}

/**
 * xutf8_to_utf16:
 * @str: a UTF-8 encoded string
 * @len: the maximum length (number of bytes) of @str to use.
 *     If @len < 0, then the string is nul-terminated.
 * @items_read: (out) (optional): location to store number of
 *     bytes read, or %NULL. If %NULL, then %G_CONVERT_ERROR_PARTIAL_INPUT will
 *     be returned in case @str contains a trailing partial character. If
 *     an error occurs then the index of the invalid input is stored here.
 * @items_written: (out) (optional): location to store number
 *     of #xunichar2_t written, or %NULL. The value stored here does not include
 *     the trailing 0.
 * @error: location to store the error occurring, or %NULL to ignore
 *     errors. Any of the errors in #GConvertError other than
 *     %G_CONVERT_ERROR_NO_CONVERSION may occur.
 *
 * Convert a string from UTF-8 to UTF-16. A 0 character will be
 * added to the result after the converted text.
 *
 * Returns: (transfer full): a pointer to a newly allocated UTF-16 string.
 *     This value must be freed with g_free(). If an error occurs,
 *     %NULL will be returned and @error set.
 */
xunichar2_t *
xutf8_to_utf16 (const xchar_t *str,
		 xlong_t        len,
		 xlong_t       *items_read,
		 xlong_t       *items_written,
		 xerror_t     **error)
{
  xunichar2_t *result = NULL;
  xint_t n16;
  const xchar_t *in;
  xint_t i;

  g_return_val_if_fail (str != NULL, NULL);

  in = str;
  n16 = 0;
  while ((len < 0 || str + len - in > 0) && *in)
    {
      xunichar_t wc = xutf8_get_char_extended (in, len < 0 ? 6 : str + len - in);
      if (wc & 0x80000000)
	{
	  if (wc == (xunichar_t)-2)
	    {
	      if (items_read)
		break;
	      else
		g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                                     _("Partial character sequence at end of input"));
	    }
	  else
	    g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                                 _("Invalid byte sequence in conversion input"));

	  goto err_out;
	}

      if (wc < 0xd800)
	n16 += 1;
      else if (wc < 0xe000)
	{
	  g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                               _("Invalid sequence in conversion input"));

	  goto err_out;
	}
      else if (wc < 0x10000)
	n16 += 1;
      else if (wc < 0x110000)
	n16 += 2;
      else
	{
	  g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                               _("Character out of range for UTF-16"));

	  goto err_out;
	}

      in = xutf8_next_char (in);
    }

  result = try_malloc_n (n16 + 1, sizeof (xunichar2_t), error);
  if (result == NULL)
      goto err_out;

  in = str;
  for (i = 0; i < n16;)
    {
      xunichar_t wc = xutf8_get_char (in);

      if (wc < 0x10000)
	{
	  result[i++] = wc;
	}
      else
	{
	  result[i++] = (wc - 0x10000) / 0x400 + 0xd800;
	  result[i++] = (wc - 0x10000) % 0x400 + 0xdc00;
	}

      in = xutf8_next_char (in);
    }

  result[i] = 0;

  if (items_written)
    *items_written = n16;

 err_out:
  if (items_read)
    *items_read = in - str;

  return result;
}

/**
 * g_ucs4_to_utf16:
 * @str: a UCS-4 encoded string
 * @len: the maximum length (number of characters) of @str to use.
 *     If @len < 0, then the string is nul-terminated.
 * @items_read: (out) (optional): location to store number of
 *     bytes read, or %NULL. If an error occurs then the index of the invalid
 *     input is stored here.
 * @items_written: (out) (optional): location to store number
 *     of #xunichar2_t  written, or %NULL. The value stored here does not include
 *     the trailing 0.
 * @error: location to store the error occurring, or %NULL to ignore
 *     errors. Any of the errors in #GConvertError other than
 *     %G_CONVERT_ERROR_NO_CONVERSION may occur.
 *
 * Convert a string from UCS-4 to UTF-16. A 0 character will be
 * added to the result after the converted text.
 *
 * Returns: (transfer full): a pointer to a newly allocated UTF-16 string.
 *     This value must be freed with g_free(). If an error occurs,
 *     %NULL will be returned and @error set.
 */
xunichar2_t *
g_ucs4_to_utf16 (const xunichar_t  *str,
		 xlong_t            len,
		 xlong_t           *items_read,
		 xlong_t           *items_written,
		 xerror_t         **error)
{
  xunichar2_t *result = NULL;
  xint_t n16;
  xint_t i, j;

  n16 = 0;
  i = 0;
  while ((len < 0 || i < len) && str[i])
    {
      xunichar_t wc = str[i];

      if (wc < 0xd800)
	n16 += 1;
      else if (wc < 0xe000)
	{
	  g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                               _("Invalid sequence in conversion input"));

	  goto err_out;
	}
      else if (wc < 0x10000)
	n16 += 1;
      else if (wc < 0x110000)
	n16 += 2;
      else
	{
	  g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                               _("Character out of range for UTF-16"));

	  goto err_out;
	}

      i++;
    }

  result = try_malloc_n (n16 + 1, sizeof (xunichar2_t), error);
  if (result == NULL)
      goto err_out;

  for (i = 0, j = 0; j < n16; i++)
    {
      xunichar_t wc = str[i];

      if (wc < 0x10000)
	{
	  result[j++] = wc;
	}
      else
	{
	  result[j++] = (wc - 0x10000) / 0x400 + 0xd800;
	  result[j++] = (wc - 0x10000) % 0x400 + 0xdc00;
	}
    }
  result[j] = 0;

  if (items_written)
    *items_written = n16;

 err_out:
  if (items_read)
    *items_read = i;

  return result;
}

#define VALIDATE_BYTE(mask, expect)                      \
  G_STMT_START {                                         \
    if (G_UNLIKELY((*(xuchar_t *)p & (mask)) != (expect))) \
      goto error;                                        \
  } G_STMT_END

/* see IETF RFC 3629 Section 4 */

static const xchar_t *
fast_validate (const char *str)

{
  const xchar_t *p;

  for (p = str; *p; p++)
    {
      if (*(xuchar_t *)p < 128)
	/* done */;
      else
	{
	  const xchar_t *last;

	  last = p;
	  if (*(xuchar_t *)p < 0xe0) /* 110xxxxx */
	    {
	      if (G_UNLIKELY (*(xuchar_t *)p < 0xc2))
		goto error;
	    }
	  else
	    {
	      if (*(xuchar_t *)p < 0xf0) /* 1110xxxx */
		{
		  switch (*(xuchar_t *)p++ & 0x0f)
		    {
		    case 0:
		      VALIDATE_BYTE(0xe0, 0xa0); /* 0xa0 ... 0xbf */
		      break;
		    case 0x0d:
		      VALIDATE_BYTE(0xe0, 0x80); /* 0x80 ... 0x9f */
		      break;
		    default:
		      VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		    }
		}
	      else if (*(xuchar_t *)p < 0xf5) /* 11110xxx excluding out-of-range */
		{
		  switch (*(xuchar_t *)p++ & 0x07)
		    {
		    case 0:
		      VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		      if (G_UNLIKELY((*(xuchar_t *)p & 0x30) == 0))
			goto error;
		      break;
		    case 4:
		      VALIDATE_BYTE(0xf0, 0x80); /* 0x80 ... 0x8f */
		      break;
		    default:
		      VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		    }
		  p++;
		  VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		}
	      else
		goto error;
	    }

	  p++;
	  VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */

	  continue;

	error:
	  return last;
	}
    }

  return p;
}

static const xchar_t *
fast_validate_len (const char *str,
		   xssize_t      max_len)

{
  const xchar_t *p;

  g_assert (max_len >= 0);

  for (p = str; ((p - str) < max_len) && *p; p++)
    {
      if (*(xuchar_t *)p < 128)
	/* done */;
      else
	{
	  const xchar_t *last;

	  last = p;
	  if (*(xuchar_t *)p < 0xe0) /* 110xxxxx */
	    {
	      if (G_UNLIKELY (max_len - (p - str) < 2))
		goto error;

	      if (G_UNLIKELY (*(xuchar_t *)p < 0xc2))
		goto error;
	    }
	  else
	    {
	      if (*(xuchar_t *)p < 0xf0) /* 1110xxxx */
		{
		  if (G_UNLIKELY (max_len - (p - str) < 3))
		    goto error;

		  switch (*(xuchar_t *)p++ & 0x0f)
		    {
		    case 0:
		      VALIDATE_BYTE(0xe0, 0xa0); /* 0xa0 ... 0xbf */
		      break;
		    case 0x0d:
		      VALIDATE_BYTE(0xe0, 0x80); /* 0x80 ... 0x9f */
		      break;
		    default:
		      VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		    }
		}
	      else if (*(xuchar_t *)p < 0xf5) /* 11110xxx excluding out-of-range */
		{
		  if (G_UNLIKELY (max_len - (p - str) < 4))
		    goto error;

		  switch (*(xuchar_t *)p++ & 0x07)
		    {
		    case 0:
		      VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		      if (G_UNLIKELY((*(xuchar_t *)p & 0x30) == 0))
			goto error;
		      break;
		    case 4:
		      VALIDATE_BYTE(0xf0, 0x80); /* 0x80 ... 0x8f */
		      break;
		    default:
		      VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		    }
		  p++;
		  VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
		}
	      else
		goto error;
	    }

	  p++;
	  VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */

	  continue;

	error:
	  return last;
	}
    }

  return p;
}

/**
 * xutf8_validate:
 * @str: (array length=max_len) (element-type xuint8_t): a pointer to character data
 * @max_len: max bytes to validate, or -1 to go until NUL
 * @end: (out) (optional) (transfer none): return location for end of valid data
 *
 * Validates UTF-8 encoded text. @str is the text to validate;
 * if @str is nul-terminated, then @max_len can be -1, otherwise
 * @max_len should be the number of bytes to validate.
 * If @end is non-%NULL, then the end of the valid range
 * will be stored there (i.e. the start of the first invalid
 * character if some bytes were invalid, or the end of the text
 * being validated otherwise).
 *
 * Note that xutf8_validate() returns %FALSE if @max_len is
 * positive and any of the @max_len bytes are nul.
 *
 * Returns %TRUE if all of @str was valid. Many GLib and GTK+
 * routines require valid UTF-8 as input; so data read from a file
 * or the network should be checked with xutf8_validate() before
 * doing anything else with it.
 *
 * Returns: %TRUE if the text was valid UTF-8
 */
xboolean_t
xutf8_validate (const char   *str,
		 xssize_t        max_len,
		 const xchar_t **end)

{
  const xchar_t *p;

  if (max_len >= 0)
    return xutf8_validate_len (str, max_len, end);

  p = fast_validate (str);

  if (end)
    *end = p;

  if (*p != '\0')
    return FALSE;
  else
    return TRUE;
}

/**
 * xutf8_validate_len:
 * @str: (array length=max_len) (element-type xuint8_t): a pointer to character data
 * @max_len: max bytes to validate
 * @end: (out) (optional) (transfer none): return location for end of valid data
 *
 * Validates UTF-8 encoded text.
 *
 * As with xutf8_validate(), but @max_len must be set, and hence this function
 * will always return %FALSE if any of the bytes of @str are nul.
 *
 * Returns: %TRUE if the text was valid UTF-8
 * Since: 2.60
 */
xboolean_t
xutf8_validate_len (const char   *str,
                     xsize_t         max_len,
                     const xchar_t **end)

{
  const xchar_t *p;

  p = fast_validate_len (str, max_len);

  if (end)
    *end = p;

  if (p != str + max_len)
    return FALSE;
  else
    return TRUE;
}

/**
 * xunichar_validate:
 * @ch: a Unicode character
 *
 * Checks whether @ch is a valid Unicode character. Some possible
 * integer values of @ch will not be valid. 0 is considered a valid
 * character, though it's normally a string terminator.
 *
 * Returns: %TRUE if @ch is a valid Unicode character
 **/
xboolean_t
xunichar_validate (xunichar_t ch)
{
  return UNICODE_VALID (ch);
}

/**
 * xutf8_strreverse:
 * @str: a UTF-8 encoded string
 * @len: the maximum length of @str to use, in bytes. If @len < 0,
 *     then the string is nul-terminated.
 *
 * Reverses a UTF-8 string. @str must be valid UTF-8 encoded text.
 * (Use xutf8_validate() on all text before trying to use UTF-8
 * utility functions with it.)
 *
 * This function is intended for programmatic uses of reversed strings.
 * It pays no attention to decomposed characters, combining marks, byte
 * order marks, directional indicators (LRM, LRO, etc) and similar
 * characters which might need special handling when reversing a string
 * for display purposes.
 *
 * Note that unlike xstrreverse(), this function returns
 * newly-allocated memory, which should be freed with g_free() when
 * no longer needed.
 *
 * Returns: (transfer full): a newly-allocated string which is the reverse of @str
 *
 * Since: 2.2
 */
xchar_t *
xutf8_strreverse (const xchar_t *str,
		   xssize_t       len)
{
  xchar_t *r, *result;
  const xchar_t *p;

  if (len < 0)
    len = strlen (str);

  result = g_new (xchar_t, len + 1);
  r = result + len;
  p = str;
  while (r > result)
    {
      xchar_t *m, skip = xutf8_skip[*(xuchar_t*) p];
      r -= skip;
      g_assert (r >= result);
      for (m = r; skip; skip--)
        *m++ = *p++;
    }
  result[len] = 0;

  return result;
}

/**
 * xutf8_make_valid:
 * @str: string to coerce into UTF-8
 * @len: the maximum length of @str to use, in bytes. If @len < 0,
 *     then the string is nul-terminated.
 *
 * If the provided string is valid UTF-8, return a copy of it. If not,
 * return a copy in which bytes that could not be interpreted as valid Unicode
 * are replaced with the Unicode replacement character (U+FFFD).
 *
 * For example, this is an appropriate function to use if you have received
 * a string that was incorrectly declared to be UTF-8, and you need a valid
 * UTF-8 version of it that can be logged or displayed to the user, with the
 * assumption that it is close enough to ASCII or UTF-8 to be mostly
 * readable as-is.
 *
 * Returns: (transfer full): a valid UTF-8 string whose content resembles @str
 *
 * Since: 2.52
 */
xchar_t *
xutf8_make_valid (const xchar_t *str,
                   xssize_t       len)
{
  xstring_t *string;
  const xchar_t *remainder, *invalid;
  xsize_t remaininxbytes, valid_bytes;

  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    len = strlen (str);

  string = NULL;
  remainder = str;
  remaininxbytes = len;

  while (remaininxbytes != 0)
    {
      if (xutf8_validate (remainder, remaininxbytes, &invalid))
	break;
      valid_bytes = invalid - remainder;

      if (string == NULL)
	string = xstring_sized_new (remaininxbytes);

      xstring_append_len (string, remainder, valid_bytes);
      /* append U+FFFD REPLACEMENT CHARACTER */
      xstring_append (string, "\357\277\275");

      remaininxbytes -= valid_bytes + 1;
      remainder = invalid + 1;
    }

  if (string == NULL)
    return xstrndup (str, len);

  xstring_append_len (string, remainder, remaininxbytes);
  xstring_append_c (string, '\0');

  g_assert (xutf8_validate (string->str, -1, NULL));

  return xstring_free (string, FALSE);
}
