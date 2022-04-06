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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "gstring.h"
#include "guriprivate.h"
#include "gprintf.h"
#include "gutilsprivate.h"


/**
 * SECTION:strings
 * @title: Strings
 * @short_description: text buffers which grow automatically
 *     as text is added
 *
 * A #xstring_t is an object that handles the memory management of a C
 * string for you.  The emphasis of #xstring_t is on text, typically
 * UTF-8.  Crucially, the "str" member of a #xstring_t is guaranteed to
 * have a trailing nul character, and it is therefore always safe to
 * call functions such as strchr() or xstrdup() on it.
 *
 * However, a #xstring_t can also hold arbitrary binary data, because it
 * has a "len" member, which includes any possible embedded nul
 * characters in the data.  Conceptually then, #xstring_t is like a
 * #xbyte_array_t with the addition of many convenience methods for text,
 * and a guaranteed nul terminator.
 */

/**
 * xstring_t:
 * @str: points to the character data. It may move as text is added.
 *   The @str field is null-terminated and so
 *   can be used as an ordinary C string.
 * @len: contains the length of the string, not including the
 *   terminating nul byte.
 * @allocated_len: the number of bytes that can be stored in the
 *   string before it needs to be reallocated. May be larger than @len.
 *
 * The xstring_t struct contains the public fields of a xstring_t.
 */

static void
xstring_maybe_expand (xstring_t *string,
                       xsize_t    len)
{
  /* Detect potential overflow */
  if G_UNLIKELY ((G_MAXSIZE - string->len - 1) < len)
    xerror ("adding %" G_GSIZE_FORMAT " to string would overflow", len);

  if (string->len + len >= string->allocated_len)
    {
      string->allocated_len = g_nearest_pow (string->len + len + 1);
      /* If the new size is bigger than G_MAXSIZE / 2, only allocate enough
       * memory for this string and don't over-allocate. */
      if (string->allocated_len == 0)
        string->allocated_len = string->len + len + 1;
      string->str = g_realloc (string->str, string->allocated_len);
    }
}

/**
 * xstring_sized_new: (constructor)
 * @dfl_size: the default size of the space allocated to hold the string
 *
 * Creates a new #xstring_t, with enough space for @dfl_size
 * bytes. This is useful if you are going to add a lot of
 * text to the string and don't want it to be reallocated
 * too often.
 *
 * Returns: (transfer full): the new #xstring_t
 */
xstring_t *
xstring_sized_new (xsize_t dfl_size)
{
  xstring_t *string = g_slice_new (xstring_t);

  string->allocated_len = 0;
  string->len   = 0;
  string->str   = NULL;

  xstring_maybe_expand (string, MAX (dfl_size, 64));
  string->str[0] = 0;

  return string;
}

/**
 * xstring_new: (constructor)
 * @init: (nullable): the initial text to copy into the string, or %NULL to
 *   start with an empty string
 *
 * Creates a new #xstring_t, initialized with the given string.
 *
 * Returns: (transfer full): the new #xstring_t
 */
xstring_t *
xstring_new (const xchar_t *init)
{
  xstring_t *string;

  if (init == NULL || *init == '\0')
    string = xstring_sized_new (2);
  else
    {
      xint_t len;

      len = strlen (init);
      string = xstring_sized_new (len + 2);

      xstring_append_len (string, init, len);
    }

  return string;
}

/**
 * xstring_new_len: (constructor)
 * @init: initial contents of the string
 * @len: length of @init to use
 *
 * Creates a new #xstring_t with @len bytes of the @init buffer.
 * Because a length is provided, @init need not be nul-terminated,
 * and can contain embedded nul bytes.
 *
 * Since this function does not stop at nul bytes, it is the caller's
 * responsibility to ensure that @init has at least @len addressable
 * bytes.
 *
 * Returns: (transfer full): a new #xstring_t
 */
xstring_t *
xstring_new_len (const xchar_t *init,
                  xssize_t       len)
{
  xstring_t *string;

  if (len < 0)
    return xstring_new (init);
  else
    {
      string = xstring_sized_new (len);

      if (init)
        xstring_append_len (string, init, len);

      return string;
    }
}

/**
 * xstring_free:
 * @string: (transfer full): a #xstring_t
 * @free_segment: if %TRUE, the actual character data is freed as well
 *
 * Frees the memory allocated for the #xstring_t.
 * If @free_segment is %TRUE it also frees the character data.  If
 * it's %FALSE, the caller gains ownership of the buffer and must
 * free it after use with g_free().
 *
 * Returns: (nullable): the character data of @string
 *          (i.e. %NULL if @free_segment is %TRUE)
 */
xchar_t *
xstring_free (xstring_t  *string,
               xboolean_t  free_segment)
{
  xchar_t *segment;

  g_return_val_if_fail (string != NULL, NULL);

  if (free_segment)
    {
      g_free (string->str);
      segment = NULL;
    }
  else
    segment = string->str;

  g_slice_free (xstring_t, string);

  return segment;
}

/**
 * xstring_free_to_bytes:
 * @string: (transfer full): a #xstring_t
 *
 * Transfers ownership of the contents of @string to a newly allocated
 * #xbytes_t.  The #xstring_t structure itself is deallocated, and it is
 * therefore invalid to use @string after invoking this function.
 *
 * Note that while #xstring_t ensures that its buffer always has a
 * trailing nul character (not reflected in its "len"), the returned
 * #xbytes_t does not include this extra nul; i.e. it has length exactly
 * equal to the "len" member.
 *
 * Returns: (transfer full): A newly allocated #xbytes_t containing contents of @string; @string itself is freed
 * Since: 2.34
 */
xbytes_t*
xstring_free_to_bytes (xstring_t *string)
{
  xsize_t len;
  xchar_t *buf;

  g_return_val_if_fail (string != NULL, NULL);

  len = string->len;

  buf = xstring_free (string, FALSE);

  return xbytes_new_take (buf, len);
}

/**
 * xstring_equal:
 * @v: a #xstring_t
 * @v2: another #xstring_t
 *
 * Compares two strings for equality, returning %TRUE if they are equal.
 * For use with #xhashtable_t.
 *
 * Returns: %TRUE if the strings are the same length and contain the
 *     same bytes
 */
xboolean_t
xstring_equal (const xstring_t *v,
                const xstring_t *v2)
{
  xchar_t *p, *q;
  xstring_t *string1 = (xstring_t *) v;
  xstring_t *string2 = (xstring_t *) v2;
  xsize_t i = string1->len;

  if (i != string2->len)
    return FALSE;

  p = string1->str;
  q = string2->str;
  while (i)
    {
      if (*p != *q)
        return FALSE;
      p++;
      q++;
      i--;
    }
  return TRUE;
}

/**
 * xstring_hash:
 * @str: a string to hash
 *
 * Creates a hash code for @str; for use with #xhashtable_t.
 *
 * Returns: hash code for @str
 */
xuint_t
xstring_hash (const xstring_t *str)
{
  const xchar_t *p = str->str;
  xsize_t n = str->len;
  xuint_t h = 0;

  /* 31 bit hash function */
  while (n--)
    {
      h = (h << 5) - h + *p;
      p++;
    }

  return h;
}

/**
 * xstring_assign:
 * @string: the destination #xstring_t. Its current contents
 *          are destroyed.
 * @rval: the string to copy into @string
 *
 * Copies the bytes from a string into a #xstring_t,
 * destroying any previous contents. It is rather like
 * the standard strcpy() function, except that you do not
 * have to worry about having enough space to copy the string.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_assign (xstring_t     *string,
                 const xchar_t *rval)
{
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (rval != NULL, string);

  /* Make sure assigning to itself doesn't corrupt the string. */
  if (string->str != rval)
    {
      /* Assigning from substring should be ok, since
       * xstring_truncate() does not reallocate.
       */
      xstring_truncate (string, 0);
      xstring_append (string, rval);
    }

  return string;
}

/**
 * xstring_truncate:
 * @string: a #xstring_t
 * @len: the new size of @string
 *
 * Cuts off the end of the xstring_t, leaving the first @len bytes.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_truncate (xstring_t *string,
                   xsize_t    len)
{
  g_return_val_if_fail (string != NULL, NULL);

  string->len = MIN (len, string->len);
  string->str[string->len] = 0;

  return string;
}

/**
 * xstring_set_size:
 * @string: a #xstring_t
 * @len: the new length
 *
 * Sets the length of a #xstring_t. If the length is less than
 * the current length, the string will be truncated. If the
 * length is greater than the current length, the contents
 * of the newly added area are undefined. (However, as
 * always, string->str[string->len] will be a nul byte.)
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_set_size (xstring_t *string,
                   xsize_t    len)
{
  g_return_val_if_fail (string != NULL, NULL);

  if (len >= string->allocated_len)
    xstring_maybe_expand (string, len - string->len);

  string->len = len;
  string->str[len] = 0;

  return string;
}

/**
 * xstring_insert_len:
 * @string: a #xstring_t
 * @pos: position in @string where insertion should
 *       happen, or -1 for at the end
 * @val: bytes to insert
 * @len: number of bytes of @val to insert, or -1 for all of @val
 *
 * Inserts @len bytes of @val into @string at @pos.
 *
 * If @len is positive, @val may contain embedded nuls and need
 * not be nul-terminated. It is the caller's responsibility to
 * ensure that @val has at least @len addressable bytes.
 *
 * If @len is negative, @val must be nul-terminated and @len
 * is considered to request the entire string length.
 *
 * If @pos is -1, bytes are inserted at the end of the string.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_insert_len (xstring_t     *string,
                     xssize_t       pos,
                     const xchar_t *val,
                     xssize_t       len)
{
  xsize_t len_unsigned, pos_unsigned;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (len == 0 || val != NULL, string);

  if (len == 0)
    return string;

  if (len < 0)
    len = strlen (val);
  len_unsigned = len;

  if (pos < 0)
    pos_unsigned = string->len;
  else
    {
      pos_unsigned = pos;
      g_return_val_if_fail (pos_unsigned <= string->len, string);
    }

  /* Check whether val represents a substring of string.
   * This test probably violates chapter and verse of the C standards,
   * since ">=" and "<=" are only valid when val really is a substring.
   * In practice, it will work on modern archs.
   */
  if (G_UNLIKELY (val >= string->str && val <= string->str + string->len))
    {
      xsize_t offset = val - string->str;
      xsize_t precount = 0;

      xstring_maybe_expand (string, len_unsigned);
      val = string->str + offset;
      /* At this point, val is valid again.  */

      /* Open up space where we are going to insert.  */
      if (pos_unsigned < string->len)
        memmove (string->str + pos_unsigned + len_unsigned,
                 string->str + pos_unsigned, string->len - pos_unsigned);

      /* Move the source part before the gap, if any.  */
      if (offset < pos_unsigned)
        {
          precount = MIN (len_unsigned, pos_unsigned - offset);
          memcpy (string->str + pos_unsigned, val, precount);
        }

      /* Move the source part after the gap, if any.  */
      if (len_unsigned > precount)
        memcpy (string->str + pos_unsigned + precount,
                val + /* Already moved: */ precount +
                      /* Space opened up: */ len_unsigned,
                len_unsigned - precount);
    }
  else
    {
      xstring_maybe_expand (string, len_unsigned);

      /* If we aren't appending at the end, move a hunk
       * of the old string to the end, opening up space
       */
      if (pos_unsigned < string->len)
        memmove (string->str + pos_unsigned + len_unsigned,
                 string->str + pos_unsigned, string->len - pos_unsigned);

      /* insert the new string */
      if (len_unsigned == 1)
        string->str[pos_unsigned] = *val;
      else
        memcpy (string->str + pos_unsigned, val, len_unsigned);
    }

  string->len += len_unsigned;

  string->str[string->len] = 0;

  return string;
}

/**
 * xstring_append_uri_escaped:
 * @string: a #xstring_t
 * @unescaped: a string
 * @reserved_chars_allowed: a string of reserved characters allowed
 *     to be used, or %NULL
 * @allow_utf8: set %TRUE if the escaped string may include UTF8 characters
 *
 * Appends @unescaped to @string, escaping any characters that
 * are reserved in URIs using URI-style escape sequences.
 *
 * Returns: (transfer none): @string
 *
 * Since: 2.16
 */
xstring_t *
xstring_append_uri_escaped (xstring_t     *string,
                             const xchar_t *unescaped,
                             const xchar_t *reserved_chars_allowed,
                             xboolean_t     allow_utf8)
{
  _uri_encoder (string, (const guchar *) unescaped, strlen (unescaped),
                reserved_chars_allowed, allow_utf8);
  return string;
}

/**
 * xstring_append:
 * @string: a #xstring_t
 * @val: the string to append onto the end of @string
 *
 * Adds a string onto the end of a #xstring_t, expanding
 * it if necessary.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_append (xstring_t     *string,
                 const xchar_t *val)
{
  return xstring_insert_len (string, -1, val, -1);
}

/**
 * xstring_append_len:
 * @string: a #xstring_t
 * @val: bytes to append
 * @len: number of bytes of @val to use, or -1 for all of @val
 *
 * Appends @len bytes of @val to @string.
 *
 * If @len is positive, @val may contain embedded nuls and need
 * not be nul-terminated. It is the caller's responsibility to
 * ensure that @val has at least @len addressable bytes.
 *
 * If @len is negative, @val must be nul-terminated and @len
 * is considered to request the entire string length. This
 * makes xstring_append_len() equivalent to xstring_append().
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_append_len (xstring_t     *string,
                     const xchar_t *val,
                     xssize_t       len)
{
  return xstring_insert_len (string, -1, val, len);
}

/**
 * xstring_append_c:
 * @string: a #xstring_t
 * @c: the byte to append onto the end of @string
 *
 * Adds a byte onto the end of a #xstring_t, expanding
 * it if necessary.
 *
 * Returns: (transfer none): @string
 */
#undef xstring_append_c
xstring_t *
xstring_append_c (xstring_t *string,
                   xchar_t    c)
{
  g_return_val_if_fail (string != NULL, NULL);

  return xstring_insert_c (string, -1, c);
}

/**
 * xstring_append_unichar:
 * @string: a #xstring_t
 * @wc: a Unicode character
 *
 * Converts a Unicode character into UTF-8, and appends it
 * to the string.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_append_unichar (xstring_t  *string,
                         xunichar_t  wc)
{
  g_return_val_if_fail (string != NULL, NULL);

  return xstring_insert_unichar (string, -1, wc);
}

/**
 * xstring_prepend:
 * @string: a #xstring_t
 * @val: the string to prepend on the start of @string
 *
 * Adds a string on to the start of a #xstring_t,
 * expanding it if necessary.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_prepend (xstring_t     *string,
                  const xchar_t *val)
{
  return xstring_insert_len (string, 0, val, -1);
}

/**
 * xstring_prepend_len:
 * @string: a #xstring_t
 * @val: bytes to prepend
 * @len: number of bytes in @val to prepend, or -1 for all of @val
 *
 * Prepends @len bytes of @val to @string.
 *
 * If @len is positive, @val may contain embedded nuls and need
 * not be nul-terminated. It is the caller's responsibility to
 * ensure that @val has at least @len addressable bytes.
 *
 * If @len is negative, @val must be nul-terminated and @len
 * is considered to request the entire string length. This
 * makes xstring_prepend_len() equivalent to xstring_prepend().
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_prepend_len (xstring_t     *string,
                      const xchar_t *val,
                      xssize_t       len)
{
  return xstring_insert_len (string, 0, val, len);
}

/**
 * xstring_prepend_c:
 * @string: a #xstring_t
 * @c: the byte to prepend on the start of the #xstring_t
 *
 * Adds a byte onto the start of a #xstring_t,
 * expanding it if necessary.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_prepend_c (xstring_t *string,
                    xchar_t    c)
{
  g_return_val_if_fail (string != NULL, NULL);

  return xstring_insert_c (string, 0, c);
}

/**
 * xstring_prepend_unichar:
 * @string: a #xstring_t
 * @wc: a Unicode character
 *
 * Converts a Unicode character into UTF-8, and prepends it
 * to the string.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_prepend_unichar (xstring_t  *string,
                          xunichar_t  wc)
{
  g_return_val_if_fail (string != NULL, NULL);

  return xstring_insert_unichar (string, 0, wc);
}

/**
 * xstring_insert:
 * @string: a #xstring_t
 * @pos: the position to insert the copy of the string
 * @val: the string to insert
 *
 * Inserts a copy of a string into a #xstring_t,
 * expanding it if necessary.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_insert (xstring_t     *string,
                 xssize_t       pos,
                 const xchar_t *val)
{
  return xstring_insert_len (string, pos, val, -1);
}

/**
 * xstring_insert_c:
 * @string: a #xstring_t
 * @pos: the position to insert the byte
 * @c: the byte to insert
 *
 * Inserts a byte into a #xstring_t, expanding it if necessary.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_insert_c (xstring_t *string,
                   xssize_t   pos,
                   xchar_t    c)
{
  xsize_t pos_unsigned;

  g_return_val_if_fail (string != NULL, NULL);

  xstring_maybe_expand (string, 1);

  if (pos < 0)
    pos = string->len;
  else
    g_return_val_if_fail ((xsize_t) pos <= string->len, string);
  pos_unsigned = pos;

  /* If not just an append, move the old stuff */
  if (pos_unsigned < string->len)
    memmove (string->str + pos_unsigned + 1,
             string->str + pos_unsigned, string->len - pos_unsigned);

  string->str[pos_unsigned] = c;

  string->len += 1;

  string->str[string->len] = 0;

  return string;
}

/**
 * xstring_insert_unichar:
 * @string: a #xstring_t
 * @pos: the position at which to insert character, or -1
 *     to append at the end of the string
 * @wc: a Unicode character
 *
 * Converts a Unicode character into UTF-8, and insert it
 * into the string at the given position.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_insert_unichar (xstring_t  *string,
                         xssize_t    pos,
                         xunichar_t  wc)
{
  xint_t charlen, first, i;
  xchar_t *dest;

  g_return_val_if_fail (string != NULL, NULL);

  /* Code copied from xunichar_to_utf() */
  if (wc < 0x80)
    {
      first = 0;
      charlen = 1;
    }
  else if (wc < 0x800)
    {
      first = 0xc0;
      charlen = 2;
    }
  else if (wc < 0x10000)
    {
      first = 0xe0;
      charlen = 3;
    }
   else if (wc < 0x200000)
    {
      first = 0xf0;
      charlen = 4;
    }
  else if (wc < 0x4000000)
    {
      first = 0xf8;
      charlen = 5;
    }
  else
    {
      first = 0xfc;
      charlen = 6;
    }
  /* End of copied code */

  xstring_maybe_expand (string, charlen);

  if (pos < 0)
    pos = string->len;
  else
    g_return_val_if_fail ((xsize_t) pos <= string->len, string);

  /* If not just an append, move the old stuff */
  if ((xsize_t) pos < string->len)
    memmove (string->str + pos + charlen, string->str + pos, string->len - pos);

  dest = string->str + pos;
  /* Code copied from xunichar_to_utf() */
  for (i = charlen - 1; i > 0; --i)
    {
      dest[i] = (wc & 0x3f) | 0x80;
      wc >>= 6;
    }
  dest[0] = wc | first;
  /* End of copied code */

  string->len += charlen;

  string->str[string->len] = 0;

  return string;
}

/**
 * xstring_overwrite:
 * @string: a #xstring_t
 * @pos: the position at which to start overwriting
 * @val: the string that will overwrite the @string starting at @pos
 *
 * Overwrites part of a string, lengthening it if necessary.
 *
 * Returns: (transfer none): @string
 *
 * Since: 2.14
 */
xstring_t *
xstring_overwrite (xstring_t     *string,
                    xsize_t        pos,
                    const xchar_t *val)
{
  g_return_val_if_fail (val != NULL, string);
  return xstring_overwrite_len (string, pos, val, strlen (val));
}

/**
 * xstring_overwrite_len:
 * @string: a #xstring_t
 * @pos: the position at which to start overwriting
 * @val: the string that will overwrite the @string starting at @pos
 * @len: the number of bytes to write from @val
 *
 * Overwrites part of a string, lengthening it if necessary.
 * This function will work with embedded nuls.
 *
 * Returns: (transfer none): @string
 *
 * Since: 2.14
 */
xstring_t *
xstring_overwrite_len (xstring_t     *string,
                        xsize_t        pos,
                        const xchar_t *val,
                        xssize_t       len)
{
  xsize_t end;

  g_return_val_if_fail (string != NULL, NULL);

  if (!len)
    return string;

  g_return_val_if_fail (val != NULL, string);
  g_return_val_if_fail (pos <= string->len, string);

  if (len < 0)
    len = strlen (val);

  end = pos + len;

  if (end > string->len)
    xstring_maybe_expand (string, end - string->len);

  memcpy (string->str + pos, val, len);

  if (end > string->len)
    {
      string->str[end] = '\0';
      string->len = end;
    }

  return string;
}

/**
 * xstring_erase:
 * @string: a #xstring_t
 * @pos: the position of the content to remove
 * @len: the number of bytes to remove, or -1 to remove all
 *       following bytes
 *
 * Removes @len bytes from a #xstring_t, starting at position @pos.
 * The rest of the #xstring_t is shifted down to fill the gap.
 *
 * Returns: (transfer none): @string
 */
xstring_t *
xstring_erase (xstring_t *string,
                xssize_t   pos,
                xssize_t   len)
{
  xsize_t len_unsigned, pos_unsigned;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (pos >= 0, string);
  pos_unsigned = pos;

  g_return_val_if_fail (pos_unsigned <= string->len, string);

  if (len < 0)
    len_unsigned = string->len - pos_unsigned;
  else
    {
      len_unsigned = len;
      g_return_val_if_fail (pos_unsigned + len_unsigned <= string->len, string);

      if (pos_unsigned + len_unsigned < string->len)
        memmove (string->str + pos_unsigned,
                 string->str + pos_unsigned + len_unsigned,
                 string->len - (pos_unsigned + len_unsigned));
    }

  string->len -= len_unsigned;

  string->str[string->len] = 0;

  return string;
}

/**
 * xstring_replace:
 * @string: a #xstring_t
 * @find: the string to find in @string
 * @replace: the string to insert in place of @find
 * @limit: the maximum instances of @find to replace with @replace, or `0` for
 * no limit
 *
 * Replaces the string @find with the string @replace in a #xstring_t up to
 * @limit times. If the number of instances of @find in the #xstring_t is
 * less than @limit, all instances are replaced. If @limit is `0`,
 * all instances of @find are replaced.
 *
 * If @find is the empty string, since versions 2.69.1 and 2.68.4 the
 * replacement will be inserted no more than once per possible position
 * (beginning of string, end of string and between characters). This did
 * not work correctly in earlier versions.
 *
 * Returns: the number of find and replace operations performed.
 *
 * Since: 2.68
 */
xuint_t
xstring_replace (xstring_t     *string,
                  const xchar_t *find,
                  const xchar_t *replace,
                  xuint_t        limit)
{
  xsize_t f_len, r_len, pos;
  xchar_t *cur, *next;
  xuint_t n = 0;

  g_return_val_if_fail (string != NULL, 0);
  g_return_val_if_fail (find != NULL, 0);
  g_return_val_if_fail (replace != NULL, 0);

  f_len = strlen (find);
  r_len = strlen (replace);
  cur = string->str;

  while ((next = strstr (cur, find)) != NULL)
    {
      pos = next - string->str;
      xstring_erase (string, pos, f_len);
      xstring_insert (string, pos, replace);
      cur = string->str + pos + r_len;
      n++;
      /* Only match the empty string once at any given position, to
       * avoid infinite loops */
      if (f_len == 0)
        {
          if (cur[0] == '\0')
            break;
          else
            cur++;
        }
      if (n == limit)
        break;
    }

  return n;
}

/**
 * xstring_ascii_down:
 * @string: a xstring_t
 *
 * Converts all uppercase ASCII letters to lowercase ASCII letters.
 *
 * Returns: (transfer none): passed-in @string pointer, with all the
 *     uppercase characters converted to lowercase in place,
 *     with semantics that exactly match g_ascii_tolower().
 */
xstring_t *
xstring_ascii_down (xstring_t *string)
{
  xchar_t *s;
  xint_t n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = string->str;

  while (n)
    {
      *s = g_ascii_tolower (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * xstring_ascii_up:
 * @string: a xstring_t
 *
 * Converts all lowercase ASCII letters to uppercase ASCII letters.
 *
 * Returns: (transfer none): passed-in @string pointer, with all the
 *     lowercase characters converted to uppercase in place,
 *     with semantics that exactly match g_ascii_toupper().
 */
xstring_t *
xstring_ascii_up (xstring_t *string)
{
  xchar_t *s;
  xint_t n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = string->str;

  while (n)
    {
      *s = g_ascii_toupper (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * xstring_down:
 * @string: a #xstring_t
 *
 * Converts a #xstring_t to lowercase.
 *
 * Returns: (transfer none): the #xstring_t
 *
 * Deprecated:2.2: This function uses the locale-specific
 *     tolower() function, which is almost never the right thing.
 *     Use xstring_ascii_down() or xutf8_strdown() instead.
 */
xstring_t *
xstring_down (xstring_t *string)
{
  guchar *s;
  xlong_t n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = (guchar *) string->str;

  while (n)
    {
      if (isupper (*s))
        *s = tolower (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * xstring_up:
 * @string: a #xstring_t
 *
 * Converts a #xstring_t to uppercase.
 *
 * Returns: (transfer none): @string
 *
 * Deprecated:2.2: This function uses the locale-specific
 *     toupper() function, which is almost never the right thing.
 *     Use xstring_ascii_up() or xutf8_strup() instead.
 */
xstring_t *
xstring_up (xstring_t *string)
{
  guchar *s;
  xlong_t n;

  g_return_val_if_fail (string != NULL, NULL);

  n = string->len;
  s = (guchar *) string->str;

  while (n)
    {
      if (islower (*s))
        *s = toupper (*s);
      s++;
      n--;
    }

  return string;
}

/**
 * xstring_append_vprintf:
 * @string: a #xstring_t
 * @format: (not nullable): the string format. See the printf() documentation
 * @args: the list of arguments to insert in the output
 *
 * Appends a formatted string onto the end of a #xstring_t.
 * This function is similar to xstring_append_printf()
 * except that the arguments to the format string are passed
 * as a va_list.
 *
 * Since: 2.14
 */
void
xstring_append_vprintf (xstring_t     *string,
                         const xchar_t *format,
                         va_list      args)
{
  xchar_t *buf;
  xint_t len;

  g_return_if_fail (string != NULL);
  g_return_if_fail (format != NULL);

  len = g_vasprintf (&buf, format, args);

  if (len >= 0)
    {
      xstring_maybe_expand (string, len);
      memcpy (string->str + string->len, buf, len + 1);
      string->len += len;
      g_free (buf);
    }
}

/**
 * xstring_vprintf:
 * @string: a #xstring_t
 * @format: (not nullable): the string format. See the printf() documentation
 * @args: the parameters to insert into the format string
 *
 * Writes a formatted string into a #xstring_t.
 * This function is similar to xstring_printf() except that
 * the arguments to the format string are passed as a va_list.
 *
 * Since: 2.14
 */
void
xstring_vprintf (xstring_t     *string,
                  const xchar_t *format,
                  va_list      args)
{
  xstring_truncate (string, 0);
  xstring_append_vprintf (string, format, args);
}

/**
 * xstring_sprintf:
 * @string: a #xstring_t
 * @format: the string format. See the sprintf() documentation
 * @...: the parameters to insert into the format string
 *
 * Writes a formatted string into a #xstring_t.
 * This is similar to the standard sprintf() function,
 * except that the #xstring_t buffer automatically expands
 * to contain the results. The previous contents of the
 * #xstring_t are destroyed.
 *
 * Deprecated: This function has been renamed to xstring_printf().
 */

/**
 * xstring_printf:
 * @string: a #xstring_t
 * @format: the string format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Writes a formatted string into a #xstring_t.
 * This is similar to the standard sprintf() function,
 * except that the #xstring_t buffer automatically expands
 * to contain the results. The previous contents of the
 * #xstring_t are destroyed.
 */
void
xstring_printf (xstring_t     *string,
                 const xchar_t *format,
                 ...)
{
  va_list args;

  xstring_truncate (string, 0);

  va_start (args, format);
  xstring_append_vprintf (string, format, args);
  va_end (args);
}

/**
 * xstring_sprintfa:
 * @string: a #xstring_t
 * @format: the string format. See the sprintf() documentation
 * @...: the parameters to insert into the format string
 *
 * Appends a formatted string onto the end of a #xstring_t.
 * This function is similar to xstring_sprintf() except that
 * the text is appended to the #xstring_t.
 *
 * Deprecated: This function has been renamed to xstring_append_printf()
 */

/**
 * xstring_append_printf:
 * @string: a #xstring_t
 * @format: the string format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Appends a formatted string onto the end of a #xstring_t.
 * This function is similar to xstring_printf() except
 * that the text is appended to the #xstring_t.
 */
void
xstring_append_printf (xstring_t     *string,
                        const xchar_t *format,
                        ...)
{
  va_list args;

  va_start (args, format);
  xstring_append_vprintf (string, format, args);
  va_end (args);
}
