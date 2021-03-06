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

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <locale.h>
#include <string.h>

#include <glib.h>

/* Bug 311337 */
static void
test_iconv_state (void)
{
  const xchar_t *in = "\xf4\xe5\xf8\xe5\xed";
  const xchar_t *expected = "\xd7\xa4\xd7\x95\xd7\xa8\xd7\x95\xd7\x9d";
  xchar_t *out;
  xsize_t bytes_read = 0;
  xsize_t bytes_written = 0;
  xerror_t *error = NULL;

  out = g_convert (in, -1, "UTF-8", "CP1255",
		   &bytes_read, &bytes_written, &error);

  if (error && error->code == G_CONVERT_ERROR_NO_CONVERSION)
    return; /* silently skip if CP1255 is not supported, see bug 467707 */

  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 5);
  g_assert_cmpint (bytes_written, ==, 10);
  g_assert_cmpstr (out, ==, expected);
  g_free (out);
}

/* Some tests involving "vulgar fraction one half" (U+00BD). This is
 * represented in UTF-8 as \xC2\xBD, in ISO-8859-1 as \xBD, and is not
 * represented in ISO-8859-15. */
static void
test_one_half (void)
{
  const xchar_t *in_utf8 = "\xc2\xbd";
  xchar_t *out;
  xsize_t bytes_read = 0;
  xsize_t bytes_written = 0;
  xerror_t *error = NULL;

  out = g_convert (in_utf8, -1,
		   "ISO-8859-1", "UTF-8",
		   &bytes_read, &bytes_written,
		   &error);

  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 2);
  g_assert_cmpint (bytes_written, ==, 1);
  g_assert_cmpstr (out, ==, "\xbd");
  g_free (out);

  out = g_convert (in_utf8, -1,
		   "ISO-8859-15", "UTF-8",
		   &bytes_read, &bytes_written,
		   &error);

  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpint (bytes_read, ==, 0);
  g_assert_cmpint (bytes_written, ==, 0);
  g_assert_cmpstr (out, ==, NULL);
  g_clear_error (&error);
  g_free (out);

  out = g_convert_with_fallback (in_utf8, -1,
				 "ISO8859-15", "UTF-8",
				 "a",
				 &bytes_read, &bytes_written,
				 &error);

  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 2);
  g_assert_cmpint (bytes_written, ==, 1);
  g_assert_cmpstr (out, ==, "a");
  g_free (out);
}

static void
test_byte_order (void)
{
  xchar_t in_be[4] = { 0xfe, 0xff, 0x03, 0x93}; /* capital gamma */
  xchar_t in_le[4] = { 0xff, 0xfe, 0x93, 0x03};
  const xchar_t *expected = "\xce\x93";
  xchar_t *out;
  xsize_t bytes_read = 0;
  xsize_t bytes_written = 0;
  xerror_t *error = NULL;

  out = g_convert (in_be, sizeof (in_be),
		   "UTF-8", "UTF-16",
		   &bytes_read, &bytes_written,
		   &error);

  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 4);
  g_assert_cmpint (bytes_written, ==, 2);
  g_assert_cmpstr (out, ==, expected);
  g_free (out);

  out = g_convert (in_le, sizeof (in_le),
		   "UTF-8", "UTF-16",
		   &bytes_read, &bytes_written,
		   &error);

  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 4);
  g_assert_cmpint (bytes_written, ==, 2);
  g_assert_cmpstr (out, ==, expected);
  g_free (out);
}

static void
check_utf8_to_ucs4 (const char     *utf8,
		    xsize_t           utf8_len,
		    const xunichar_t *ucs4,
		    xlong_t           ucs4_len,
		    xlong_t           error_pos)
{
  xunichar_t *result, *result2, *result3;
  xlong_t items_read, items_read2;
  xlong_t items_written, items_written2;
  xerror_t *error, *error2, *error3;
  xint_t i;

  if (!error_pos)
    {
      /* check the fast conversion */
      result = xutf8_to_ucs4_fast (utf8, utf8_len, &items_written);

      g_assert_cmpint (items_written, ==, ucs4_len);
      xassert (result);
      for (i = 0; i <= items_written; i++)
	xassert (result[i] == ucs4[i]);

      g_free (result);
    }

  error = NULL;
  result = xutf8_to_ucs4 (utf8, utf8_len, &items_read, &items_written, &error);

  if (utf8_len == strlen (utf8))
    {
      /* check that len == -1 yields identical results */
      error2 = NULL;
      result2 = xutf8_to_ucs4 (utf8, -1, &items_read2, &items_written2, &error2);
      xassert (error || items_read2 == items_read);
      xassert (error || items_written2 == items_written);
      g_assert_cmpint (!!result, ==, !!result2);
      g_assert_cmpint (!!error, ==, !!error2);
      if (result)
	for (i = 0; i <= items_written; i++)
	  xassert (result[i] == result2[i]);

      g_free (result2);
      if (error2)
	xerror_free (error2);
    }

  error3 = NULL;
  result3 = xutf8_to_ucs4 (utf8, utf8_len, NULL, NULL, &error3);

  if (error3 && error3->code == G_CONVERT_ERROR_PARTIAL_INPUT)
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, error_pos);
      g_assert_cmpint (items_written, ==, ucs4_len);
      xassert (result);
      for (i = 0; i <= items_written; i++)
	xassert (result[i] == ucs4[i]);
      xerror_free (error3);
    }
  else if (error_pos)
    {
      xassert (error != NULL);
      xassert (result == NULL);
      g_assert_cmpint (items_read, ==, error_pos);
      xerror_free (error);

      xassert (error3 != NULL);
      xassert (result3 == NULL);
      xerror_free (error3);
    }
  else
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, utf8_len);
      g_assert_cmpint (items_written, ==, ucs4_len);
      xassert (result);
      for (i = 0; i <= items_written; i++)
	xassert (result[i] == ucs4[i]);

      g_assert_no_error (error3);
      xassert (result3);
      for (i = 0; i <= ucs4_len; i++)
	xassert (result3[i] == ucs4[i]);
    }

  g_free (result);
  g_free (result3);
}

static void
check_ucs4_to_utf8 (const xunichar_t *ucs4,
		    xlong_t           ucs4_len,
		    const char     *utf8,
		    xlong_t           utf8_len,
		    xlong_t           error_pos)
{
  xchar_t *result, *result2, *result3;
  xlong_t items_read, items_read2;
  xlong_t items_written, items_written2;
  xerror_t *error, *error2, *error3;

  error = NULL;
  result = g_ucs4_to_utf8 (ucs4, ucs4_len, &items_read, &items_written, &error);

  if (ucs4[ucs4_len] == 0)
    {
      /* check that len == -1 yields identical results */
      error2 = NULL;
      result2 = g_ucs4_to_utf8 (ucs4, -1, &items_read2, &items_written2, &error2);

      xassert (error || items_read2 == items_read);
      xassert (error || items_written2 == items_written);
      g_assert_cmpint (!!result, ==, !!result2);
      g_assert_cmpint (!!error, ==, !!error2);
      if (result)
	g_assert_cmpstr (result, ==, result2);

      g_free (result2);
      if (error2)
	xerror_free (error2);
    }

  error3 = NULL;
  result3 = g_ucs4_to_utf8 (ucs4, ucs4_len, NULL, NULL, &error3);

  if (error_pos)
    {
      xassert (error != NULL);
      xassert (result == NULL);
      g_assert_cmpint (items_read, ==, error_pos);
      xerror_free (error);

      xassert (error3 != NULL);
      xassert (result3 == NULL);
      xerror_free (error3);
    }
  else
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, ucs4_len);
      g_assert_cmpint (items_written, ==, utf8_len);
      xassert (result);
      g_assert_cmpstr (result, ==, utf8);

      g_assert_no_error (error3);
      xassert (result3);
      g_assert_cmpstr (result3, ==, utf8);
    }

  g_free (result);
  g_free (result3);
}

static void
check_utf8_to_utf16 (const char      *utf8,
		     xsize_t            utf8_len,
		     const xunichar2_t *utf16,
		     xlong_t            utf16_len,
		     xlong_t            error_pos)
{
  xunichar2_t *result, *result2, *result3;
  xlong_t items_read, items_read2;
  xlong_t items_written, items_written2;
  xerror_t *error, *error2, *error3;
  xint_t i;

  error = NULL;
  result = xutf8_to_utf16 (utf8, utf8_len, &items_read, &items_written, &error);

  if (utf8_len == strlen (utf8))
    {
      /* check that len == -1 yields identical results */
      error2 = NULL;
      result2 = xutf8_to_utf16 (utf8, -1, &items_read2, &items_written2, &error2);
      xassert (error || items_read2 == items_read);
      xassert (error || items_written2 == items_written);
      g_assert_cmpint (!!result, ==, !!result2);
      g_assert_cmpint (!!error, ==, !!error2);
      if (result)
	for (i = 0; i <= items_written; i++)
	  xassert (result[i] == result2[i]);

      g_free (result2);
      if (error2)
	xerror_free (error2);
    }

  error3 = NULL;
  result3 = xutf8_to_utf16 (utf8, utf8_len, NULL, NULL, &error3);

  if (error3 && error3->code == G_CONVERT_ERROR_PARTIAL_INPUT)
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, error_pos);
      g_assert_cmpint (items_written, ==, utf16_len);
      xassert (result);
      for (i = 0; i <= items_written; i++)
	xassert (result[i] == utf16[i]);
      xerror_free (error3);
    }
  else if (error_pos)
    {
      xassert (error != NULL);
      xassert (result == NULL);
      g_assert_cmpint (items_read, ==, error_pos);
      xerror_free (error);

      xassert (error3 != NULL);
      xassert (result3 == NULL);
      xerror_free (error3);
    }
  else
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, utf8_len);
      g_assert_cmpint (items_written, ==, utf16_len);
      xassert (result);
      for (i = 0; i <= items_written; i++)
	xassert (result[i] == utf16[i]);

      g_assert_no_error (error3);
      xassert (result3);
      for (i = 0; i <= utf16_len; i++)
	xassert (result3[i] == utf16[i]);
    }

  g_free (result);
  g_free (result3);
}

static void
check_utf16_to_utf8 (const xunichar2_t *utf16,
		     xlong_t            utf16_len,
		     const char      *utf8,
		     xlong_t            utf8_len,
		     xlong_t            error_pos)
{
  xchar_t *result, *result2, *result3;
  xlong_t items_read, items_read2;
  xlong_t items_written, items_written2;
  xerror_t *error, *error2, *error3;

  error = NULL;
  result = xutf16_to_utf8 (utf16, utf16_len, &items_read, &items_written, &error);
  if (utf16[utf16_len] == 0)
    {
      /* check that len == -1 yields identical results */
      error2 = NULL;
      result2 = xutf16_to_utf8 (utf16, -1, &items_read2, &items_written2, &error2);

      xassert (error || items_read2 == items_read);
      xassert (error || items_written2 == items_written);
      g_assert_cmpint (!!result, ==, !!result2);
      g_assert_cmpint (!!error, ==, !!error2);
      if (result)
	g_assert_cmpstr (result, ==, result2);

      g_free (result2);
      if (error2)
	xerror_free (error2);
    }

  error3 = NULL;
  result3 = xutf16_to_utf8 (utf16, utf16_len, NULL, NULL, &error3);

  if (error3 && error3->code == G_CONVERT_ERROR_PARTIAL_INPUT)
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, error_pos);
      g_assert_cmpint (items_read + 1, ==, utf16_len);
      g_assert_cmpint (items_written, ==, utf8_len);
      xassert (result);
      g_assert_cmpstr (result, ==, utf8);
      xerror_free (error3);
    }
  else if (error_pos)
    {
      xassert (error != NULL);
      xassert (result == NULL);
      g_assert_cmpint (items_read, ==, error_pos);
      xerror_free (error);

      xassert (error3 != NULL);
      xassert (result3 == NULL);
      xerror_free (error3);
    }
  else
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, utf16_len);
      g_assert_cmpint (items_written, ==, utf8_len);
      xassert (result);
      g_assert_cmpstr (result, ==, utf8);

      g_assert_no_error (error3);
      xassert (result3);
      g_assert_cmpstr (result3, ==, utf8);
    }

  g_free (result);
  g_free (result3);
}

static void
check_ucs4_to_utf16 (const xunichar_t  *ucs4,
		     xlong_t            ucs4_len,
		     const xunichar2_t *utf16,
		     xlong_t            utf16_len,
		     xlong_t            error_pos)
{
  xunichar2_t *result, *result2, *result3;
  xlong_t items_read, items_read2;
  xlong_t items_written, items_written2;
  xerror_t *error, *error2, *error3;
  xint_t i;

  error = NULL;
  result = g_ucs4_to_utf16 (ucs4, ucs4_len, &items_read, &items_written, &error);

  if (ucs4[ucs4_len] == 0)
    {
      /* check that len == -1 yields identical results */
      error2 = NULL;
      result2 = g_ucs4_to_utf16 (ucs4, -1, &items_read2, &items_written2, &error2);

      xassert (error || items_read2 == items_read);
      xassert (error || items_written2 == items_written);
      g_assert_cmpint (!!result, ==, !!result2);
      g_assert_cmpint (!!error, ==, !!error2);
      if (result)
      for (i = 0; i <= utf16_len; i++)
	xassert (result[i] == result2[i]);

      g_free (result2);
      if (error2)
	xerror_free (error2);
    }

  error3 = NULL;
  result3 = g_ucs4_to_utf16 (ucs4, -1, NULL, NULL, &error3);

  if (error_pos)
    {
      xassert (error != NULL);
      xassert (result == NULL);
      g_assert_cmpint (items_read, ==, error_pos);
      xerror_free (error);

      xassert (error3 != NULL);
      xassert (result3 == NULL);
      xerror_free (error3);
    }
  else
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, ucs4_len);
      g_assert_cmpint (items_written, ==, utf16_len);
      xassert (result);
      for (i = 0; i <= utf16_len; i++)
	xassert (result[i] == utf16[i]);

      g_assert_no_error (error3);
      xassert (result3);
      for (i = 0; i <= utf16_len; i++)
	xassert (result3[i] == utf16[i]);
    }

  g_free (result);
  g_free (result3);
}

static void
check_utf16_to_ucs4 (const xunichar2_t *utf16,
		     xlong_t            utf16_len,
		     const xunichar_t  *ucs4,
		     xlong_t            ucs4_len,
		     xlong_t            error_pos)
{
  xunichar_t *result, *result2, *result3;
  xlong_t items_read, items_read2;
  xlong_t items_written, items_written2;
  xerror_t *error, *error2, *error3;
  xint_t i;

  error = NULL;
  result = xutf16_to_ucs4 (utf16, utf16_len, &items_read, &items_written, &error);
  if (utf16[utf16_len] == 0)
    {
      /* check that len == -1 yields identical results */
      error2 = NULL;
      result2 = xutf16_to_ucs4 (utf16, -1, &items_read2, &items_written2, &error2);
      xassert (error || items_read2 == items_read);
      xassert (error || items_written2 == items_written);
      g_assert_cmpint (!!result, ==, !!result2);
      g_assert_cmpint (!!error, ==, !!error2);
      if (result)
	for (i = 0; i <= items_written; i++)
	  xassert (result[i] == result2[i]);

      g_free (result2);
      if (error2)
	xerror_free (error2);
    }

  error3 = NULL;
  result3 = xutf16_to_ucs4 (utf16, utf16_len, NULL, NULL, &error3);

  if (error3 && error3->code == G_CONVERT_ERROR_PARTIAL_INPUT)
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, error_pos);
      g_assert_cmpint (items_read + 1, ==, utf16_len);
      g_assert_cmpint (items_written, ==, ucs4_len);
      xassert (result);
      for (i = 0; i <= items_written; i++)
	xassert (result[i] == ucs4[i]);
      xerror_free (error3);
    }
  else if (error_pos)
    {
      xassert (error != NULL);
      xassert (result == NULL);
      g_assert_cmpint (items_read, ==, error_pos);
      xerror_free (error);

      xassert (error3 != NULL);
      xassert (result3 == NULL);
      xerror_free (error3);
    }
  else
    {
      g_assert_no_error (error);
      g_assert_cmpint (items_read, ==, utf16_len);
      g_assert_cmpint (items_written, ==, ucs4_len);
      xassert (result);
      for (i = 0; i <= ucs4_len; i++)
	xassert (result[i] == ucs4[i]);

      g_assert_no_error (error3);
      xassert (result3);
      for (i = 0; i <= ucs4_len; i++)
	xassert (result3[i] == ucs4[i]);
    }

  g_free (result);
  g_free (result3);
}

static void
test_unicode_conversions (void)
{
  const char *utf8;
  xunichar_t ucs4[100];
  xunichar2_t utf16[100];

  utf8 = "abc";
  ucs4[0] = 0x61; ucs4[1] = 0x62; ucs4[2] = 0x63; ucs4[3] = 0;
  utf16[0] = 0x61; utf16[1] = 0x62; utf16[2] = 0x63; utf16[3] = 0;

  check_utf8_to_ucs4 (utf8, 3, ucs4, 3, 0);
  check_ucs4_to_utf8 (ucs4, 3, utf8, 3, 0);
  check_utf8_to_utf16 (utf8, 3, utf16, 3, 0);
  check_utf16_to_utf8 (utf16, 3, utf8, 3, 0);
  check_ucs4_to_utf16 (ucs4, 3, utf16, 3, 0);
  check_utf16_to_ucs4 (utf16, 3, ucs4, 3, 0);

  utf8 = "\316\261\316\262\316\263";
  ucs4[0] = 0x03b1; ucs4[1] = 0x03b2; ucs4[2] = 0x03b3; ucs4[3] = 0;
  utf16[0] = 0x03b1; utf16[1] = 0x03b2; utf16[2] = 0x03b3; utf16[3] = 0;

  check_utf8_to_ucs4 (utf8, 6, ucs4, 3, 0);
  check_ucs4_to_utf8 (ucs4, 3, utf8, 6, 0);
  check_utf8_to_utf16 (utf8, 6, utf16, 3, 0);
  check_utf16_to_utf8 (utf16, 3, utf8, 6, 0);
  check_ucs4_to_utf16 (ucs4, 3, utf16, 3, 0);
  check_utf16_to_ucs4 (utf16, 3, ucs4, 3, 0);

  /* partial utf8 character */
  utf8 = "abc\316";
  ucs4[0] = 0x61; ucs4[1] = 0x62; ucs4[2] = 0x63; ucs4[3] = 0;
  utf16[0] = 0x61; utf16[1] = 0x62; utf16[2] = 0x63; utf16[3] = 0;

  check_utf8_to_ucs4 (utf8, 4, ucs4, 3, 3);
  check_utf8_to_utf16 (utf8, 4, utf16, 3, 3);

  /* invalid utf8 */
  utf8 = "abc\316\316";
  ucs4[0] = 0;
  utf16[0] = 0;

  check_utf8_to_ucs4 (utf8, 5, ucs4, 0, 3);
  check_utf8_to_utf16 (utf8, 5, utf16, 0, 3);

  /* partial utf16 character */
  utf8 = "ab";
  ucs4[0] = 0x61; ucs4[1] = 0x62; ucs4[2] = 0;
  utf16[0] = 0x61; utf16[1] = 0x62; utf16[2] = 0xd801; utf16[3] = 0;

  check_utf16_to_utf8 (utf16, 3, utf8, 2, 2);
  check_utf16_to_ucs4 (utf16, 3, ucs4, 2, 2);

  /* invalid utf16 */
  utf8 = NULL;
  ucs4[0] = 0;
  utf16[0] = 0x61; utf16[1] = 0x62; utf16[2] = 0xdc01; utf16[3] = 0;

  check_utf16_to_utf8 (utf16, 3, utf8, 0, 2);
  check_utf16_to_ucs4 (utf16, 3, ucs4, 0, 2);

  /* invalid ucs4 */
  utf8 = NULL;
  ucs4[0] = 0x61; ucs4[1] = 0x62; ucs4[2] = 0x80000000; ucs4[3] = 0;
  utf16[0] = 0;

  check_ucs4_to_utf8 (ucs4, 3, utf8, 0, 2);
  check_ucs4_to_utf16 (ucs4, 3, utf16, 0, 2);
}

static void
test_filename_utf8 (void)
{
  const xchar_t *filename = "/my/path/to/foo";
  xchar_t *utf8;
  xchar_t *back;
  xerror_t *error;

  error = NULL;
  utf8 = xfilename_to_utf8 (filename, -1, NULL, NULL, &error);
  g_assert_no_error (error);
  back = xfilename_from_utf8 (utf8, -1, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (back, ==, filename);

  g_free (utf8);
  g_free (back);
}

static void
test_filename_display (void)
{
  const xchar_t *filename = "/my/path/to/foo";
  char *display;

  display = xfilename_display_basename (filename);
  g_assert_cmpstr (display, ==, "foo");

  g_free (display);
}

/* g_convert() should accept and produce text buffers with embedded
 * nul bytes/characters.
 */
static void
test_convert_embedded_nul (void)
{
  xchar_t *res;
  xsize_t bytes_read, bytes_written;
  xerror_t *error = NULL;

  res = g_convert ("ab\0\xf6", 4, "UTF-8", "ISO-8859-1",
                   &bytes_read, &bytes_written, &error);
  g_assert_no_error (error);
  g_assert_cmpuint (bytes_read, ==, 4);
  g_assert_cmpmem (res, bytes_written, "ab\0\xc3\xb6", 5);
  g_free (res);
}

static void
test_locale_to_utf8_embedded_nul (void)
{
  g_test_trap_subprocess ("/conversion/locale-to-utf8/embedded-nul/subprocess/utf8", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_subprocess ("/conversion/locale-to-utf8/embedded-nul/subprocess/iconv", 0, 0);
  g_test_trap_assert_passed ();
}

/* test_t that embedded nul characters in UTF-8 input to g_locale_to_utf8()
 * result in an error.
 */
static void
test_locale_to_utf8_embedded_nul_utf8 (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

  setlocale (LC_ALL, "");
  g_setenv ("CHARSET", "UTF-8", TRUE);
  g_assert_true (g_get_charset (NULL));

  res = g_locale_to_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

/* test_t that embedded nul characters in output of g_locale_to_utf8(),
 * when converted from non-UTF8 input, result in an error.
 */
static void
test_locale_to_utf8_embedded_nul_iconv (void)
{
  xchar_t *res;
  xerror_t *error = NULL;

  setlocale (LC_ALL, "C");
  g_setenv ("CHARSET", "US-ASCII", TRUE);
  g_assert_false (g_get_charset (NULL));

  res = g_locale_to_utf8 ("ab\0c", 4, NULL, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_EMBEDDED_NUL);
  xerror_free (error);
}

static void
test_locale_from_utf8_embedded_nul (void)
{
  g_test_trap_subprocess ("/conversion/locale-from-utf8/embedded-nul/subprocess/utf8", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_subprocess ("/conversion/locale-from-utf8/embedded-nul/subprocess/iconv", 0, 0);
  g_test_trap_assert_passed ();
}

/* test_t that embedded nul characters in input to g_locale_from_utf8(),
 * when converting (copying) to UTF-8 output, result in an error.
 */
static void
test_locale_from_utf8_embedded_nul_utf8 (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

  setlocale (LC_ALL, "");
  g_setenv ("CHARSET", "UTF-8", TRUE);
  g_assert_true (g_get_charset (NULL));

  res = g_locale_from_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

/* test_t that embedded nul characters in input to g_locale_from_utf8(),
 * when converting to non-UTF-8 output, result in an error.
 */
static void
test_locale_from_utf8_embedded_nul_iconv (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

  setlocale (LC_ALL, "C");
  g_setenv ("CHARSET", "US-ASCII", TRUE);
  g_assert_false (g_get_charset (NULL));

  res = g_locale_from_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

static void
test_filename_to_utf8_embedded_nul (void)
{
  g_test_trap_subprocess ("/conversion/filename-to-utf8/embedded-nul/subprocess/utf8", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_subprocess ("/conversion/filename-to-utf8/embedded-nul/subprocess/iconv", 0, 0);
  g_test_trap_assert_passed ();
}

/* test_t that embedded nul characters in UTF-8 input to xfilename_to_utf8()
 * result in an error.
 */
static void
test_filename_to_utf8_embedded_nul_utf8 (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

#ifndef G_OS_WIN32
  /* XFILENAME_ENCODING has no effect on Windows for g_get_filename_charsets() */
  g_setenv ("XFILENAME_ENCODING", "UTF-8", TRUE);
  g_assert_true (g_get_filename_charsets (NULL));
#endif

  res = xfilename_to_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

/* test_t that embedded nul characters in non-UTF-8 input of xfilename_to_utf8()
 * result in an error.
 */
static void
test_filename_to_utf8_embedded_nul_iconv (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

#ifndef G_OS_WIN32
  /* XFILENAME_ENCODING has no effect on Windows for g_get_filename_charsets() */
  g_setenv ("XFILENAME_ENCODING", "US-ASCII", TRUE);
  g_assert_false (g_get_filename_charsets (NULL));
#endif

  res = xfilename_to_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

static void
test_filename_from_utf8_embedded_nul (void)
{
  g_test_trap_subprocess ("/conversion/filename-from-utf8/embedded-nul/subprocess/utf8", 0, 0);
  g_test_trap_assert_passed ();
  g_test_trap_subprocess ("/conversion/filename-from-utf8/embedded-nul/subprocess/iconv", 0, 0);
  g_test_trap_assert_passed ();
}

/* test_t that embedded nul characters in input to xfilename_from_utf8(),
 * when converting (copying) to UTF-8 output, result in an error.
 */
static void
test_filename_from_utf8_embedded_nul_utf8 (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

#ifndef G_OS_WIN32
  /* XFILENAME_ENCODING has no effect on Windows for g_get_filename_charsets() */
  g_setenv ("XFILENAME_ENCODING", "UTF-8", TRUE);
  g_assert_true (g_get_filename_charsets (NULL));
#endif

  res = xfilename_from_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

/* test_t that embedded nul characters in input to xfilename_from_utf8(),
 * when converting to non-UTF-8 output, result in an error.
 */
static void
test_filename_from_utf8_embedded_nul_iconv (void)
{
  xchar_t *res;
  xsize_t bytes_read;
  xerror_t *error = NULL;

#ifndef G_OS_WIN32
  /* XFILENAME_ENCODING has no effect on Windows for g_get_filename_charsets() */
  g_setenv ("XFILENAME_ENCODING", "US-ASCII", TRUE);
  g_assert_false (g_get_filename_charsets (NULL));
#endif

  res = xfilename_from_utf8 ("ab\0c", 4, &bytes_read, NULL, &error);

  g_assert_null (res);
  g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE);
  g_assert_cmpuint (bytes_read, ==, 2);
  xerror_free (error);
}

static void
test_no_conv (void)
{
  const xchar_t *in = "";
  xchar_t *out G_GNUC_UNUSED;
  xsize_t bytes_read = 0;
  xsize_t bytes_written = 0;
  xerror_t *error = NULL;

  out = g_convert (in, -1, "XXX", "UVZ",
                   &bytes_read, &bytes_written, &error);

  /* error code is unreliable, since we mishandle errno there */
  xassert (error && error->domain == G_CONVERT_ERROR);
  xerror_free (error);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/conversion/no-conv", test_no_conv);
  g_test_add_func ("/conversion/iconv-state", test_iconv_state);
  g_test_add_func ("/conversion/illegal-sequence", test_one_half);
  g_test_add_func ("/conversion/byte-order", test_byte_order);
  g_test_add_func ("/conversion/unicode", test_unicode_conversions);
  g_test_add_func ("/conversion/filename-utf8", test_filename_utf8);
  g_test_add_func ("/conversion/filename-display", test_filename_display);
  g_test_add_func ("/conversion/convert-embedded-nul", test_convert_embedded_nul);
  g_test_add_func ("/conversion/locale-to-utf8/embedded-nul", test_locale_to_utf8_embedded_nul);
  g_test_add_func ("/conversion/locale-to-utf8/embedded-nul/subprocess/utf8", test_locale_to_utf8_embedded_nul_utf8);
  g_test_add_func ("/conversion/locale-to-utf8/embedded-nul/subprocess/iconv", test_locale_to_utf8_embedded_nul_iconv);
  g_test_add_func ("/conversion/locale-from-utf8/embedded-nul", test_locale_from_utf8_embedded_nul);
  g_test_add_func ("/conversion/locale-from-utf8/embedded-nul/subprocess/utf8", test_locale_from_utf8_embedded_nul_utf8);
  g_test_add_func ("/conversion/locale-from-utf8/embedded-nul/subprocess/iconv", test_locale_from_utf8_embedded_nul_iconv);
  g_test_add_func ("/conversion/filename-to-utf8/embedded-nul", test_filename_to_utf8_embedded_nul);
  g_test_add_func ("/conversion/filename-to-utf8/embedded-nul/subprocess/utf8", test_filename_to_utf8_embedded_nul_utf8);
  g_test_add_func ("/conversion/filename-to-utf8/embedded-nul/subprocess/iconv", test_filename_to_utf8_embedded_nul_iconv);
  g_test_add_func ("/conversion/filename-from-utf8/embedded-nul", test_filename_from_utf8_embedded_nul);
  g_test_add_func ("/conversion/filename-from-utf8/embedded-nul/subprocess/utf8", test_filename_from_utf8_embedded_nul_utf8);
  g_test_add_func ("/conversion/filename-from-utf8/embedded-nul/subprocess/iconv", test_filename_from_utf8_embedded_nul_iconv);

  return g_test_run ();
}
