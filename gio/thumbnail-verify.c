/* Copyright © 2013 Canonical Limited
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "thumbnail-verify.h"

#include <string.h>

/* Begin code to check the validity of thumbnail files.  In order to do
 * that we need to parse enough PNG in order to get the Thumb::URI,
 * Thumb::MTime and Thumb::Size tags out of the file.  Fortunately this
 * is relatively easy.
 */
typedef struct
{
  const xchar_t *uri;
  xuint64_t      mtime;
  xuint64_t      size;
} ExpectedInfo;

/* We *require* matches on URI and MTime, but the Size field is optional
 * (as per the spec).
 *
 * http://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
 */
#define MATCHED_URI    (1u << 0)
#define MATCHED_MTIME  (1u << 1)
#define MATCHED_ALL    (MATCHED_URI | MATCHED_MTIME)

static xboolean_t
check_integer_match (xuint64_t      expected,
                     const xchar_t *value,
                     xuint32_t      value_size)
{
  /* Would be nice to g_ascii_strtoll here, but we don't have a variant
   * that works on strings that are not nul-terminated.
   *
   * It's easy enough to do it ourselves...
   */
  if (expected == 0)  /* special case: "0" */
    return value_size == 1 && value[0] == '0';

  /* Check each digit, as long as we have data from both */
  while (expected && value_size)
    {
      /* Check the low-order digit */
      if (value[value_size - 1] != (xchar_t) ((expected % 10) + '0'))
        return FALSE;

      /* Move on... */
      expected /= 10;
      value_size--;
    }

  /* Make sure nothing is left over, on either side */
  return !expected && !value_size;
}

static xboolean_t
check_png_info_chunk (ExpectedInfo *expected_info,
                      const xchar_t  *key,
                      xuint32_t       key_size,
                      const xchar_t  *value,
                      xuint32_t       value_size,
                      xuint_t        *required_matches)
{
  if (key_size == 10 && memcmp (key, "Thumb::URI", 10) == 0)
    {
      xsize_t expected_size;

      expected_size = strlen (expected_info->uri);

      if (expected_size != value_size)
        return FALSE;

      if (memcmp (expected_info->uri, value, value_size) != 0)
        return FALSE;

      *required_matches |= MATCHED_URI;
    }

  else if (key_size == 12 && memcmp (key, "Thumb::MTime", 12) == 0)
    {
      if (!check_integer_match (expected_info->mtime, value, value_size))
        return FALSE;

      *required_matches |= MATCHED_MTIME;
    }

  else if (key_size == 11 && memcmp (key, "Thumb::Size", 11) == 0)
    {
      /* A match on Thumb::Size is not required for success, but if we
       * find this optional field and it's wrong, we should reject the
       * thumbnail.
       */
      if (!check_integer_match (expected_info->size, value, value_size))
        return FALSE;
    }

  return TRUE;
}

static xboolean_t
check_thumbnail_validity (ExpectedInfo *expected_info,
                          const xchar_t  *contents,
                          xsize_t         size)
{
  xuint_t required_matches = 0;

  /* Reference: http://www.w3.org/TR/PNG/ */
  if (size < 8)
    return FALSE;

  if (memcmp (contents, "\x89PNG\r\n\x1a\n", 8) != 0)
    return FALSE;

  contents += 8, size -= 8;

  /* We need at least 12 bytes to have a chunk... */
  while (size >= 12)
    {
      xuint32_t chunk_size_be;
      xuint32_t chunk_size;

      /* PNG is not an aligned file format so we have to be careful
       * about reading integers...
       */
      memcpy (&chunk_size_be, contents, 4);
      chunk_size = GUINT32_FROM_BE (chunk_size_be);

      contents += 4, size -= 4;

      /* After consuming the size field, we need to have enough bytes
       * for 4 bytes type field, chunk_size bytes for data, then 4 byte
       * for CRC (which we ignore)
       *
       * We just read chunk_size from the file, so it may be very large.
       * Make sure it won't wrap when we add 8 to it.
       */
      if (G_MAXUINT32 - chunk_size < 8 || size < chunk_size + 8)
        goto out;

      /* We are only interested in tEXt fields */
      if (memcmp (contents, "tEXt", 4) == 0)
        {
          const xchar_t *key = contents + 4;
          xuint32_t key_size;

          /* We need to find the nul separator character that splits the
           * key/value.  The value is not terminated.
           *
           * If we find no nul then we just ignore the field.
           *
           * value may contain extra nuls, but check_png_info_chunk()
           * can handle that.
           */
          for (key_size = 0; key_size < chunk_size; key_size++)
            {
              if (key[key_size] == '\0')
                {
                  const xchar_t *value;
                  xuint32_t value_size;

                  /* Since key_size < chunk_size, value_size is
                   * definitely non-negative.
                   */
                  value_size = chunk_size - key_size - 1;
                  value = key + key_size + 1;

                  /* We found the separator character. */
                  if (!check_png_info_chunk (expected_info,
                                             key, key_size,
                                             value, value_size,
                                             &required_matches))
                    return FALSE;
                }
            }
        }
      else
        {
          /* A bit of a hack: assume that all tEXt chunks will appear
           * together.  Therefore, if we have already seen both required
           * fields and then see a non-tEXt chunk then we can assume we
           * are done.
           *
           * The common case is that the tEXt chunks come at the start
           * of the file before any of the image data.  This trick means
           * that we will only fault in a single page (4k) whereas many
           * thumbnails (particularly the large ones) can approach 100k
           * in size.
           */
          if (required_matches == MATCHED_ALL)
            goto out;
        }

      /* skip to the next chunk, ignoring CRC. */
      contents += 4, size -= 4;                         /* type field */
      contents += chunk_size, size -= chunk_size;       /* data */
      contents += 4, size -= 4;                         /* CRC */
    }

out:
  return required_matches == MATCHED_ALL;
}

xboolean_t
thumbnail_verify (const char     *thumbnail_path,
                  const xchar_t    *file_uri,
                  const GLocalFileStat *file_stat_buf)
{
  xboolean_t thumbnail_is_valid = FALSE;
  ExpectedInfo expected_info;
  xmapped_file_t *file;

  if (file_stat_buf == NULL)
    return FALSE;

  expected_info.uri = file_uri;
#ifdef G_OS_WIN32
  expected_info.mtime = (xuint64_t) file_stat_buf->st_mtim.tv_sec;
#else
  expected_info.mtime = _g_stat_mtime (file_stat_buf);
#endif
  expected_info.size = _g_stat_size (file_stat_buf);

  file = xmapped_file_new (thumbnail_path, FALSE, NULL);
  if (file)
    {
      thumbnail_is_valid = check_thumbnail_validity (&expected_info,
                                                     xmapped_file_get_contents (file),
                                                     xmapped_file_get_length (file));
      xmapped_file_unref (file);
    }

  return thumbnail_is_valid;
}
