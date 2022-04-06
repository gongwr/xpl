/* gstdio-private.c - private glib functions for gstdio.c
 *
 * Copyright 2004 Tor Lillqvist
 * Copyright 2018 Руслан Ижбулатов
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Strips "\\\\?\\" extended prefix or
 * "\\??\\" NT Object Manager prefix from
 * @str in-place, using memmove.
 * @str_size must point to the size of @str
 * in gunichar2s, including NUL-terminator
 * (if @str is NUL-terminated; it doesn't have to be).
 * On return @str_size will correctly reflect changes
 * in @str size (if any).
 * Returns TRUE if @str was modified.
 */
static xboolean_t
_g_win32_strip_extended_ntobjm_prefix (xunichar2_t *str,
                                       xsize_t     *str_size)
{
  const wchar_t *extended_prefix = L"\\\\?\\";
  const xsize_t    extended_prefix_len = wcslen (extended_prefix);
  const xsize_t    extended_prefix_len_bytes = sizeof (xunichar2_t) * extended_prefix_len;
  const xsize_t    extended_prefix_with_drive_len_bytes = sizeof (xunichar2_t) * (extended_prefix_len + 2);
  const wchar_t *ntobjm_prefix = L"\\??\\";
  const xsize_t    ntobjm_prefix_len = wcslen (ntobjm_prefix);
  const xsize_t    ntobjm_prefix_len_bytes = sizeof (xunichar2_t) * ntobjm_prefix_len;
  const xsize_t    ntobjm_prefix_with_drive_len_bytes = sizeof (xunichar2_t) * (ntobjm_prefix_len + 2);
  xboolean_t do_move = FALSE;
  xsize_t move_shift = 0;

  if ((*str_size) * sizeof (xunichar2_t) > extended_prefix_with_drive_len_bytes &&
      memcmp (str,
              extended_prefix,
              extended_prefix_len_bytes) == 0 &&
      iswascii (str[extended_prefix_len]) &&
      iswalpha (str[extended_prefix_len]) &&
      str[extended_prefix_len + 1] == L':')
   {
     do_move = TRUE;
     move_shift = extended_prefix_len;
   }
  else if ((*str_size) * sizeof (xunichar2_t) > ntobjm_prefix_with_drive_len_bytes &&
           memcmp (str,
                   ntobjm_prefix,
                   ntobjm_prefix_len_bytes) == 0 &&
           iswascii (str[ntobjm_prefix_len]) &&
           iswalpha (str[ntobjm_prefix_len]) &&
           str[ntobjm_prefix_len + 1] == L':')
    {
      do_move = TRUE;
      move_shift = ntobjm_prefix_len;
    }

  if (do_move)
    {
      *str_size -= move_shift;
      memmove (str,
               str + move_shift,
               (*str_size) * sizeof (xunichar2_t));
    }

  return do_move;
}

static int
_g_win32_copy_and_maybe_terminate (const xuchar_t *data,
                                   xsize_t         in_to_copy,
                                   xunichar2_t    *buf,
                                   xsize_t         buf_size,
                                   xunichar2_t   **alloc_buf,
                                   xboolean_t      terminate)
{
  xsize_t to_copy = in_to_copy;
  /* Number of bytes we can use to add extra zeroes for NUL-termination.
   * 0 means that we can destroy up to 2 bytes of data,
   * 1 means that we can destroy up to 1 byte of data,
   * 2 means that we do not perform destructive NUL-termination
   */
  xsize_t extra_bytes = terminate ? 2 : 0;
  char *buf_in_chars;

  if (to_copy == 0)
    return 0;

  /* 2 bytes is sizeof (wchar_t), for an extra NUL-terminator. */
  if (buf)
    {
      if (to_copy >= buf_size)
        {
          extra_bytes = 0;
          to_copy = buf_size;
        }
      else if (to_copy > buf_size - 2)
        {
          extra_bytes = 1;
        }

      memcpy (buf, data, to_copy);
    }
  else
    {
      /* Note that SubstituteNameLength is USHORT, so to_copy + 2, being
       * xsize_t, never overflows.
       */
      *alloc_buf = g_malloc (to_copy + extra_bytes);
      memcpy (*alloc_buf, data, to_copy);
    }

  if (!terminate)
    return to_copy;

  if (buf)
    buf_in_chars = (char *) buf;
  else
    buf_in_chars = (char *) *alloc_buf;

  if (to_copy >= 2 && buf_in_chars[to_copy - 2] == 0 &&
      buf_in_chars[to_copy - 1] == 0)
    {
      /* Fully NUL-terminated, do nothing */
    }
  else if ((to_copy == 1 || buf_in_chars[to_copy - 2] != 0) &&
           buf_in_chars[to_copy - 1] == 0)
    {
      /* Have one zero, try to add another one */
      if (extra_bytes > 0)
        {
          /* Append trailing zero */
          buf_in_chars[to_copy] = 0;
          /* Be precise about the number of bytes we return */
          to_copy += 1;
        }
      else if (to_copy >= 2)
        {
          /* No space for appending, destroy one byte */
          buf_in_chars[to_copy - 2] = 0;
        }
      /* else there's no space at all (to_copy == 1), do nothing */
    }
  else if (extra_bytes > 0 || to_copy >= 2)
    {
      buf_in_chars[to_copy - 2 + extra_bytes] = 0;
      buf_in_chars[to_copy - 1 + extra_bytes] = 0;
      to_copy += extra_bytes;
    }
  else /* extra_bytes == 0 && to_copy == 1 */
    {
      buf_in_chars[0] = 0;
    }

  return to_copy;
}
