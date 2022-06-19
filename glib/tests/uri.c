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

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
  char *filename;
  char *hostname;
  char *expected_result;
  xint_t expected_error; /* If failed */
} FileToUriTest;

FileToUriTest
file_to_uri_tests[] = {
  { "/etc", NULL, "file:///etc", 0 },
  { "/etc", "", "file:///etc", 0 },
  { "/etc", "otherhost", "file://otherhost/etc", 0 },
#ifdef G_OS_WIN32
  { "/etc", "localhost", "file:///etc", 0 },
  { "c:\\windows", NULL, "file:///c:/windows", 0 },
  { "c:\\windows", "localhost", "file:///c:/windows", 0 },
  { "c:\\windows", "otherhost", "file://otherhost/c:/windows", 0 },
  { "\\\\server\\share\\dir", NULL, "file:////server/share/dir", 0 },
  { "\\\\server\\share\\dir", "localhost", "file:////server/share/dir", 0 },
#else
  { "/etc", "localhost", "file://localhost/etc", 0 },
  { "c:\\windows", NULL, NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH}, /* it's important to get this error on Unix */
  { "c:\\windows", "localhost", NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
  { "c:\\windows", "otherhost", NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
#endif
  { "etc", "localhost", NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
#ifndef XPLATFORM_WIN32
  { "/etc/\xE5\xE4\xF6", NULL, "file:///etc/%E5%E4%F6", 0 },
  { "/etc/\xC3\xB6\xC3\xA4\xC3\xA5", NULL, "file:///etc/%C3%B6%C3%A4%C3%A5", 0 },
#endif
  { "/etc", "\xC3\xB6\xC3\xA4\xC3\xA5", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/etc", "\xE5\xE4\xF6", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/etc/file with #%", NULL, "file:///etc/file%20with%20%23%25", 0 },
  { "", NULL, NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
  { "", "", NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
  { "", "localhost", NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
  { "", "otherhost", NULL, G_CONVERT_ERROR_NOT_ABSOLUTE_PATH},
  { "/0123456789", NULL, "file:///0123456789", 0 },
  { "/ABCDEFGHIJKLMNOPQRSTUVWXYZ", NULL, "file:///ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0 },
  { "/abcdefghijklmnopqrstuvwxyz", NULL, "file:///abcdefghijklmnopqrstuvwxyz", 0 },
  { "/-_.!~*'()", NULL, "file:///-_.!~*'()", 0 },
#ifdef G_OS_WIN32
  /* As '\\' is a path separator on Win32, it gets turned into '/' in the URI */
  { "/\"#%<>[\\]^`{|}\x7F", NULL, "file:///%22%23%25%3C%3E%5B/%5D%5E%60%7B%7C%7D%7F", 0 },
#else
  /* On Unix, '\\' is a normal character in the file name */
  { "/\"#%<>[\\]^`{|}\x7F", NULL, "file:///%22%23%25%3C%3E%5B%5C%5D%5E%60%7B%7C%7D%7F", 0 },
#endif
  { "/;@+$,", NULL, "file:///%3B@+$,", 0 },
  /* This and some of the following are of course as such illegal file names on Windows,
   * and would not occur in real life.
   */
  { "/:", NULL, "file:///:", 0 },
  { "/?&=", NULL, "file:///%3F&=", 0 },
  { "/", "0123456789-", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "file://ABCDEFGHIJKLMNOPQRSTUVWXYZ/", 0 },
  { "/", "abcdefghijklmnopqrstuvwxyz", "file://abcdefghijklmnopqrstuvwxyz/", 0 },
  { "/", "_.!~*'()", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", "\"#%<>[\\]^`{|}\x7F", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", ";?&=+$,", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", "/", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", "@:", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", "\x80\xFF", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
  { "/", "\xC3\x80\xC3\xBF", NULL, G_CONVERT_ERROR_ILLEGAL_SEQUENCE},
};


typedef struct
{
  char *uri;
  char *expected_filename;
  char *expected_hostname;
  xint_t expected_error; /* If failed */
} FileFromUriTest;

FileFromUriTest
file_from_uri_tests[] = {
  { "file:///etc", "/etc", NULL, 0 },
  { "FILE:///etc", "/etc", NULL, 0 },
  { "file:/etc", "/etc", NULL, 0 },
#ifdef G_OS_WIN32
  /* On Win32 we don't return "localhost" hostames, just in case
   * it isn't recognized anyway.
   */
  { "file://localhost/etc", "/etc", NULL, 0 },
  { "file://localhost/etc/%23%25%20file", "/etc/#% file", NULL, 0 },
  { "file://localhost/\xE5\xE4\xF6", "/\xe5\xe4\xf6", NULL, 0 },
  { "file://localhost/%E5%E4%F6", "/\xe5\xe4\xf6", NULL, 0 },
#else
  { "file://localhost/etc", "/etc", "localhost", 0 },
  { "file://localhost/etc/%23%25%20file", "/etc/#% file", "localhost", 0 },
  { "file://localhost/\xE5\xE4\xF6", "/\xe5\xe4\xf6", "localhost", 0 },
  { "file://localhost/%E5%E4%F6", "/\xe5\xe4\xf6", "localhost", 0 },
#endif
  { "file://otherhost/etc", "/etc", "otherhost", 0 },
  { "file://otherhost/etc/%23%25%20file", "/etc/#% file", "otherhost", 0 },
  { "file://%C3%B6%C3%A4%C3%A5/etc", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file:////etc/%C3%B6%C3%C3%C3%A5", "//etc/\xc3\xb6\xc3\xc3\xc3\xa5", NULL, 0 },
  { "file://\xE5\xE4\xF6/etc", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://%E5%E4%F6/etc", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file:///some/file#bad", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://some", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file:test", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "http://www.yahoo.com/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file:////etc", "//etc", NULL, 0 },
  { "file://///etc", "///etc", NULL, 0 },
#ifdef G_OS_WIN32
  /* URIs with backslashes come from some nonstandard application, but accept them anyhow */
  { "file:///c:\\foo", "c:\\foo", NULL, 0 },
  { "file:///c:/foo\\bar", "c:\\foo\\bar", NULL, 0 },
  /* Accept also the old Netscape drive-letter-and-vertical bar convention */
  { "file:///c|/foo", "c:\\foo", NULL, 0 },
  { "file:////server/share/dir", "\\\\server\\share\\dir", NULL, 0 },
  { "file://localhost//server/share/foo", "\\\\server\\share\\foo", NULL, 0 },
  { "file://otherhost//server/share/foo", "\\\\server\\share\\foo", "otherhost", 0 },
#else
  { "file:///c:\\foo", "/c:\\foo", NULL, 0 },
  { "file:///c:/foo", "/c:/foo", NULL, 0 },
  { "file:////c:/foo", "//c:/foo",  NULL, 0 },
#endif
  { "file://0123456789/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://ABCDEFGHIJKLMNOPQRSTUVWXYZ/", "/", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0 },
  { "file://abcdefghijklmnopqrstuvwxyz/", "/", "abcdefghijklmnopqrstuvwxyz", 0 },
  { "file://-_.!~*'()/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://\"<>[\\]^`{|}\x7F/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://;?&=+$,/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://%C3%80%C3%BF/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://@/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://:/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://#/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://%23/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
  { "file://%2F/", NULL, NULL, G_CONVERT_ERROR_BAD_URI},
};

static void
run_file_to_uri_tests (void)
{
  xsize_t i;
  xchar_t *res;
  xerror_t *error;

  for (i = 0; i < G_N_ELEMENTS (file_to_uri_tests); i++)
    {
      error = NULL;
      res = xfilename_to_uri (file_to_uri_tests[i].filename,
                               file_to_uri_tests[i].hostname,
                               &error);

      if (res)
        g_assert_cmpstr (res, ==, file_to_uri_tests[i].expected_result);
      else
        g_assert_error (error, G_CONVERT_ERROR, file_to_uri_tests[i].expected_error);

      g_free (res);
      g_clear_error (&error);
    }
}

static void
run_file_from_uri_tests (void)
{
  xsize_t i;
  xchar_t *res;
  xchar_t *hostname;
  xerror_t *error;

  for (i = 0; i < G_N_ELEMENTS (file_from_uri_tests); i++)
    {
      error = NULL;
      res = xfilename_from_uri (file_from_uri_tests[i].uri,
                                 &hostname,
                                 &error);

#ifdef G_OS_WIN32
      if (file_from_uri_tests[i].expected_filename)
        {
          xchar_t *p, *slash;
          p = file_from_uri_tests[i].expected_filename =
            xstrdup (file_from_uri_tests[i].expected_filename);
          while ((slash = strchr (p, '/')) != NULL)
            {
              *slash = '\\';
              p = slash + 1;
            }
        }
#endif
      if (res)
        g_assert_cmpstr (res, ==, file_from_uri_tests[i].expected_filename);
      else
        g_assert_error (error, G_CONVERT_ERROR, file_from_uri_tests[i].expected_error);
      g_assert_cmpstr (hostname, ==, file_from_uri_tests[i].expected_hostname);

      g_free (res);
      g_free (hostname);
      g_clear_error (&error);
    }
}

static xint_t
safe_strcmp_filename (const xchar_t *a, const xchar_t *b)
{
#ifndef G_OS_WIN32
  return xstrcmp0 (a, b);
#else
  if (!a || !b)
    return xstrcmp0 (a, b);
  else
    {
      while (*a && *b)
        {
          if ((X_IS_DIR_SEPARATOR (*a) && X_IS_DIR_SEPARATOR (*b)) ||
              *a == *b)
            a++, b++;
          else
            return (*a - *b);
        }
      return (*a - *b);
    }
#endif
}

static xint_t
safe_strcmp_hostname (const xchar_t *a, const xchar_t *b)
{
  if (a == NULL)
    a = "";
  if (b == NULL)
    b = "";
#ifndef G_OS_WIN32
  return strcmp (a, b);
#else
  if (strcmp (a, "localhost") == 0 && !*b)
    return 0;
  else
    return strcmp (a, b);
#endif
}

static void
run_file_roundtrip_tests (void)
{
  xsize_t i;
  xchar_t *uri, *hostname, *res;
  xerror_t *error;

  for (i = 0; i < G_N_ELEMENTS (file_to_uri_tests); i++)
    {
      if (file_to_uri_tests[i].expected_error != 0)
        continue;

      error = NULL;
      uri = xfilename_to_uri (file_to_uri_tests[i].filename,
                               file_to_uri_tests[i].hostname,
                               &error);
      g_assert_no_error (error);

      hostname = NULL;
      res = xfilename_from_uri (uri, &hostname, &error);
      g_assert_no_error (error);

      g_assert_cmpint (safe_strcmp_filename (file_to_uri_tests[i].filename, res), ==, 0);
      g_assert_cmpint (safe_strcmp_hostname (file_to_uri_tests[i].hostname, hostname), ==, 0);
      g_free (res);
      g_free (uri);
      g_free (hostname);
    }
}

static void
run_uri_list_tests (void)
{
  /* straight from the RFC */
  xchar_t *list =
    "# urn:isbn:0-201-08372-8\r\n"
    "http://www.huh.org/books/foo.html\r\n"
    "http://www.huh.org/books/foo.pdf   \r\n"
    "   ftp://ftp.foo.org/books/foo.txt\r\n";
  xchar_t *expected_uris[] = {
    "http://www.huh.org/books/foo.html",
    "http://www.huh.org/books/foo.pdf",
    "ftp://ftp.foo.org/books/foo.txt"
  };

  xchar_t **uris;
  xint_t j;

  uris = xuri_list_extract_uris (list);
  g_assert_cmpint (xstrv_length (uris), ==, 3);

  for (j = 0; j < 3; j++)
    g_assert_cmpstr (uris[j], ==, expected_uris[j]);

  xstrfreev (uris);

  uris = xuri_list_extract_uris ("# just hot air\r\n# more hot air");
  g_assert_cmpint (xstrv_length (uris), ==, 0);
  xstrfreev (uris);
}

static void
test_uri_unescape_string (void)
{
  const struct
    {
      /* Inputs */
      const xchar_t *escaped;  /* (nullable) */
      const xchar_t *illegal_characters;  /* (nullable) */
      /* Outputs */
      const xchar_t *expected_unescaped;  /* (nullable) */
    }
  tests[] =
    {
      { "%2Babc %4F", NULL, "+abc O" },
      { "%2Babc %4F", "+", NULL },
      { "%00abc %4F", "+/", NULL },
      { "/cursors/none.png", "/", "/cursors/none.png" },
      { "/cursors%2fbad-subdir/none.png", "/", NULL },
      { "%0", NULL, NULL },
      { "%ra", NULL, NULL },
      { "%2r", NULL, NULL },
      { "Timm B\344der", NULL, "Timm B\344der" },
      { NULL, NULL, NULL },  /* actually a valid test, not a delimiter */
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xchar_t *s = NULL;

      g_test_message ("test_t %" G_GSIZE_FORMAT ": %s", i, tests[i].escaped);

      s = xuri_unescape_string (tests[i].escaped, tests[i].illegal_characters);
      g_assert_cmpstr (s, ==, tests[i].expected_unescaped);
      g_free (s);
    }
}

static void
test_uri_unescape_bytes (xconstpointer test_data)
{
  xerror_t *error = NULL;
  xboolean_t use_nul_terminated = GPOINTER_TO_INT (test_data);
  const struct
    {
      /* Inputs */
      const xchar_t *escaped;  /* (nullable) */
      const xchar_t *illegal;
      /* Outputs */
      xssize_t expected_unescaped_len;  /* -1 => error expected */
      const xuint8_t *expected_unescaped;  /* (nullable) */
    }
  tests[] =
    {
      { "%00%00", NULL, 2, (const xuint8_t *) "\x00\x00" },
      { "/cursors/none.png", "/", 17, (const xuint8_t *) "/cursors/none.png" },
      { "/cursors%2fbad-subdir/none.png", "/", -1, NULL },
      { "%%", NULL, -1, NULL },
      { "%", NULL, -1, NULL },
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xssize_t escaped_len = 0;
      xchar_t *escaped = NULL;
      xbytes_t *bytes = NULL;

      g_test_message ("test_t %" G_GSIZE_FORMAT ": %s", i, tests[i].escaped);

      /* The tests get run twice: once with the length unspecified, using a
       * nul-terminated string; and once with the length specified and a copy of
       * the string with the trailing nul explicitly removed (to help catch
       * buffer overflows). */
      if (use_nul_terminated)
        {
          escaped_len = -1;
          escaped = xstrdup (tests[i].escaped);
        }
      else
        {
          escaped_len = strlen (tests[i].escaped);  /* no trailing nul */
          escaped = g_memdup2 (tests[i].escaped, escaped_len);
        }

      bytes = xuri_unescape_bytes (escaped, escaped_len, tests[i].illegal, &error);

      if (tests[i].expected_unescaped_len < 0)
        {
          g_assert_null (bytes);
          g_assert_error (error, XURI_ERROR, XURI_ERROR_FAILED);
          g_clear_error (&error);
        }
      else
        {
          g_assert_no_error (error);
          g_assert_cmpmem (xbytes_get_data (bytes, NULL),
                           xbytes_get_size (bytes),
                           tests[i].expected_unescaped,
                           tests[i].expected_unescaped_len);
        }

      g_clear_pointer (&bytes, xbytes_unref);
      g_free (escaped);
    }
}

static void
test_uri_unescape_segment (void)
{
  const xchar_t *escaped_segment = "%2Babc %4F---";
  xchar_t *s = NULL;

  s = xuri_unescape_segment (escaped_segment, escaped_segment + 10, NULL);
  g_assert_cmpstr (s, ==, "+abc O");
  g_free (s);

  s = xuri_unescape_segment ("%2Babc%00cde", NULL, NULL);
  g_assert_null (s);
}

static void
test_uri_escape_string (void)
{
  const struct
    {
      /* Inputs */
      const xchar_t *unescaped;
      const xchar_t *reserved_chars_allowed;
      xboolean_t allow_utf8;
      /* Outputs */
      const xchar_t *expected_escaped;
    }
  tests[] =
    {
      { "abcdefgABCDEFG._~", NULL, FALSE, "abcdefgABCDEFG._~" },
      { ":+ \\?#", NULL, FALSE, "%3A%2B%20%5C%3F%23" },
      { "a+b:c", "+", FALSE, "a+b%3Ac" },
      { "a+b:c\303\234", "+", TRUE, "a+b%3Ac\303\234" },
      /* Incomplete UTF-8 sequence: */
      { "\xfc\x3b\xd2", NULL, TRUE, "%FC%3B%D2" },
      /* Invalid sequence: */
      { "\xc3\xb1\xc3\x28", NULL, TRUE, "ñ%C3%28" },
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xchar_t *s = NULL;

      g_test_message ("test_t %" G_GSIZE_FORMAT ": %s", i, tests[i].unescaped);

      s = xuri_escape_string (tests[i].unescaped,
                               tests[i].reserved_chars_allowed,
                               tests[i].allow_utf8);
      g_assert_cmpstr (s, ==, tests[i].expected_escaped);
      g_free (s);
    }
}

static void
test_uri_escape_bytes (void)
{
  xchar_t *s = NULL;

  s = xuri_escape_bytes ((xuchar_t*)"\0\0", 2, NULL);
  g_assert_cmpstr (s, ==, "%00%00");
  g_free (s);
}

static void
test_uri_scheme (void)
{
  const xchar_t *s1, *s2;
  xchar_t *s;

  s = xuri_parse_scheme ("ftp://ftp.gtk.org");
  g_assert_cmpstr (s, ==, "ftp");
  g_free (s);

  s = xuri_parse_scheme ("good-scheme.but+weird:gtk.org");
  g_assert_cmpstr (s, ==, "good-scheme.but+weird");
  g_free (s);

  s = xuri_parse_scheme ("1bad:");
  g_assert_null (s);
  s = xuri_parse_scheme ("bad");
  g_assert_null (s);
  s = xuri_parse_scheme ("99http://host/path");
  g_assert_null (s);
  s = xuri_parse_scheme (".http://host/path");
  g_assert_null (s);
  s = xuri_parse_scheme ("+http://host/path");
  g_assert_null (s);

  s1 = xuri_peek_scheme ("ftp://ftp.gtk.org");
  g_assert_cmpstr (s1, ==, "ftp");
  s2 = xuri_peek_scheme ("FTP://ftp.gtk.org");
  g_assert_cmpstr (s2, ==, "ftp");
  g_assert_true (s1 == s2);
  s1 = xuri_peek_scheme ("1bad:");
  g_assert_null (s1);
  s1 = xuri_peek_scheme ("bad");
  g_assert_null (s1);
}

typedef struct {
  const xchar_t *scheme;
  const xchar_t *userinfo;
  const xchar_t *host;
  xint_t         port;
  const xchar_t *path;
  const xchar_t *query;
  const xchar_t *fragment;
} UriParts;

typedef struct {
  /* Inputs */
  const xchar_t *orig;
  xuri_flags_t flags;
  /* Outputs */
  xboolean_t expected_success;
  xint_t expected_error_code;       /* unused if @expected_success is true */
  const UriParts expected_parts;  /* unused if @expected_success is false */
} UriAbsoluteTest;

static const UriAbsoluteTest absolute_tests[] = {
  { "foo:", XURI_FLAGS_NONE, TRUE, 0,
    { "foo", NULL, NULL, -1, "", NULL, NULL }
  },
  { "file:/dev/null", XURI_FLAGS_NONE, TRUE, 0,
    { "file", NULL, NULL, -1, "/dev/null", NULL, NULL }
  },
  { "file:///dev/null", XURI_FLAGS_NONE, TRUE, 0,
    { "file", NULL, "", -1, "/dev/null", NULL, NULL }
  },
  { "ftp://user@host/path", XURI_FLAGS_NONE, TRUE, 0,
    { "ftp", "user", "host", -1, "/path", NULL, NULL }
  },
  { "ftp://user@host:9999/path", XURI_FLAGS_NONE, TRUE, 0,
    { "ftp", "user", "host", 9999, "/path", NULL, NULL }
  },
  { "ftp://user:password@host/path", XURI_FLAGS_NONE, TRUE, 0,
    { "ftp", "user:password", "host", -1, "/path", NULL, NULL }
  },
  { "ftp://user:password@host:9999/path", XURI_FLAGS_NONE, TRUE, 0,
    { "ftp", "user:password", "host", 9999, "/path", NULL, NULL }
  },
  { "ftp://user:password@host", XURI_FLAGS_NONE, TRUE, 0,
    { "ftp", "user:password", "host", -1, "", NULL, NULL }
  },
  { "http://us%65r@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", "user", "host", -1, "", NULL, NULL }
  },
  { "http://us%40r@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", "us@r", "host", -1, "", NULL, NULL }
  },
  { "http://us%3ar@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", "us:r", "host", -1, "", NULL, NULL }
  },
  { "http://us%2fr@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", "us/r", "host", -1, "", NULL, NULL }
  },
  { "http://us%3fr@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", "us?r", "host", -1, "", NULL, NULL }
  },
  { "http://host?query", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "host", -1, "", "query", NULL }
  },
  { "http://host/path?query=http%3A%2F%2Fhost%2Fpath%3Fchildparam%3Dchildvalue&param=value", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "host", -1, "/path", "query=http://host/path?childparam=childvalue&param=value", NULL }
  },
  { "http://control-chars/%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F%7F", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "control-chars", -1, "/\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F", NULL, NULL }
  },
  { "http://space/%20", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "space", -1, "/ ", NULL, NULL }
  },
  { "http://delims/%3C%3E%23%25%22", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "delims", -1, "/<>#%\"", NULL, NULL }
  },
  { "http://unwise-chars/%7B%7D%7C%5C%5E%5B%5D%60", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "unwise-chars", -1, "/{}|\\^[]`", NULL, NULL }
  },

  /* From RFC 2732 */
  { "http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "FEDC:BA98:7654:3210:FEDC:BA98:7654:3210", 80, "/index.html", NULL, NULL }
  },
  { "http://[1080:0:0:0:8:800:200C:417A]/index.html", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "1080:0:0:0:8:800:200C:417A", -1, "/index.html", NULL, NULL }
  },
  { "http://[3ffe:2a00:100:7031::1]", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "3ffe:2a00:100:7031::1", -1, "", NULL, NULL }
  },
  { "http://[1080::8:800:200C:417A]/foo", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "1080::8:800:200C:417A", -1, "/foo", NULL, NULL }
  },
  { "http://[::192.9.5.5]/ipng", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "::192.9.5.5", -1, "/ipng", NULL, NULL }
  },
  { "http://[::FFFF:129.144.52.38]:80/index.html", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "::FFFF:129.144.52.38", 80, "/index.html", NULL, NULL }
  },
  { "http://[2010:836B:4179::836B:4179]", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "2010:836B:4179::836B:4179", -1, "", NULL, NULL }
  },

  /* some problematic URIs that are handled differently in libsoup */
  { "http://host/path with spaces", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path with spaces", NULL, NULL }
  },
  { "  http://host/path", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path", NULL, NULL }
  },
  { "http://host/path  ", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path", NULL, NULL }
  },
  { "http://host  ", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "", NULL, NULL }
  },
  { "http://host:999  ", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", 999, "", NULL, NULL }
  },
  { "http://host/pa\nth", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path", NULL, NULL }
  },
  { "http:\r\n//host/path", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path", NULL, NULL }
  },
  { "http://\thost/path", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path", NULL, NULL }
  },

  /* Bug 594405; 0-length is different from not-present */
  { "http://host/path?", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "host", -1, "/path", "", NULL }
  },
  { "http://host/path#", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "host", -1, "/path", NULL, "" },
  },

  /* Bug 590524; ignore bad %-encoding */
  { "http://host/path%", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path%", NULL, NULL }
  },
  { "http://h%ost/path", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "h%ost", -1, "/path", NULL, NULL }
  },
  { "http://host/path%%", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path%%", NULL, NULL }
  },
  { "http://host/path%%%", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path%%%", NULL, NULL }
  },
  { "http://host/path%/x/", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path%/x/", NULL, NULL }
  },
  { "http://host/path%0x/", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path%0x/", NULL, NULL }
  },
  { "http://host/path%ax", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "host", -1, "/path%ax", NULL, NULL }
  },

  /* xuri_t doesn't %-encode non-ASCII characters */
  { "http://host/p\xc3\xa4th/", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "host", -1, "/p\xc3\xa4th/", NULL, NULL }
  },

  { "HTTP:////////////////", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "", -1, "//////////////", NULL, NULL }
  },

  { "http://@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", "", "host", -1, "", NULL, NULL }
  },
  { "http://:@host", XURI_FLAGS_NONE, TRUE, 0,
    { "http", ":", "host", -1, "", NULL, NULL }
  },
  { "scheme://foo%3Abar._webdav._tcp.local", XURI_FLAGS_NONE, TRUE, 0,
    { "scheme", NULL, "foo:bar._webdav._tcp.local", -1, "", NULL, NULL}
  },

  /* ".." past top */
  { "http://example.com/..", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "example.com", -1, "/", NULL, NULL }
  },

  /* scheme parsing */
  { "foo0://host/path", XURI_FLAGS_NONE, TRUE, 0,
    { "foo0", NULL, "host", -1, "/path", NULL, NULL } },
  { "f0.o://host/path", XURI_FLAGS_NONE, TRUE, 0,
    { "f0.o", NULL, "host", -1, "/path", NULL, NULL } },
  { "http++://host/path", XURI_FLAGS_NONE, TRUE, 0,
    { "http++", NULL, "host", -1, "/path", NULL, NULL } },
  { "http-ish://host/path", XURI_FLAGS_NONE, TRUE, 0,
    { "http-ish", NULL, "host", -1, "/path", NULL, NULL } },

  /* IPv6 scope ID parsing (both correct and incorrect) */
  { "http://[fe80::dead:beef%]/", XURI_FLAGS_PARSE_RELAXED, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[fe80::dead:beef%em1]/", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%em1", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%em1]/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[fe80::dead:beef%25em1]/", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%em1", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%25em1%20]/", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%em1 ", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%25em%31]/", XURI_FLAGS_NONE, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%em1", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%10]/", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%10", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%10]/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[fe80::dead:beef%25]/", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%25", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%25]/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[192.168.0.1%25em1]/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[fe80::dead:beef%2em1]/", XURI_FLAGS_PARSE_RELAXED, TRUE, 0,
    { "http", NULL, "fe80::dead:beef%2em1", -1, "/", NULL, NULL } },
  { "http://[fe80::dead:beef%2em1]/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[fe80::dead:beef%25em1%00]/", XURI_FLAGS_PARSE_RELAXED, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
  { "http://[fe80::dead:beef%25em1%00]/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },

  /* Invalid IDN hostname */
  { "http://xn--mixed-\xc3\xbcp/", XURI_FLAGS_NONE, FALSE, XURI_ERROR_BAD_HOST,
    { NULL, NULL, NULL, -1, NULL, NULL, NULL } },
};

static void
test_uri_parsing_absolute (void)
{
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (absolute_tests); i++)
    {
      const UriAbsoluteTest *test = &absolute_tests[i];
      xerror_t *error = NULL;
      xuri_t *uri;

      g_test_message ("test_t %" G_GSIZE_FORMAT ": %s", i, test->orig);

      uri = xuri_parse (test->orig, test->flags, &error);
      if (test->expected_success)
        {
          g_assert_no_error (error);

          g_assert_cmpstr (xuri_get_scheme (uri),   ==, test->expected_parts.scheme);
          g_assert_cmpstr (xuri_get_userinfo (uri), ==, test->expected_parts.userinfo);
          g_assert_cmpstr (xuri_get_host (uri),     ==, test->expected_parts.host);
          g_assert_cmpint (xuri_get_port (uri),     ==, test->expected_parts.port);
          g_assert_cmpstr (xuri_get_path (uri),     ==, test->expected_parts.path);
          g_assert_cmpstr (xuri_get_query (uri),    ==, test->expected_parts.query);
          g_assert_cmpstr (xuri_get_fragment (uri), ==, test->expected_parts.fragment);
        }
      else
        {
          g_assert_error (error, XURI_ERROR, test->expected_error_code);
          g_assert_null (uri);
        }

      g_clear_pointer (&uri, xuri_unref);
      g_clear_error (&error);
    }
}

typedef struct {
  const xchar_t *orig, *resolved;
  UriParts parts;
} UriRelativeTest;

/* This all comes from RFC 3986 */
static const char *relative_test_base = "http://a/b/c/d;p?q";
static const UriRelativeTest relative_tests[] = {
  { "g:h", "g:h",
    { "g", NULL, NULL, -1, "h", NULL, NULL } },
  { "g", "http://a/b/c/g",
    { "http", NULL, "a", -1, "/b/c/g", NULL, NULL } },
  { "./g", "http://a/b/c/g",
    { "http", NULL, "a", -1, "/b/c/g", NULL, NULL } },
  { "g/", "http://a/b/c/g/",
    { "http", NULL, "a", -1, "/b/c/g/", NULL, NULL } },
  { "/g", "http://a/g",
    { "http", NULL, "a", -1, "/g", NULL, NULL } },
  { "//g", "http://g",
    { "http", NULL, "g", -1, "", NULL, NULL } },
  { "?y", "http://a/b/c/d;p?y",
    { "http", NULL, "a", -1, "/b/c/d;p", "y", NULL } },
  { "g?y", "http://a/b/c/g?y",
    { "http", NULL, "a", -1, "/b/c/g", "y", NULL } },
  { "#s", "http://a/b/c/d;p?q#s",
    { "http", NULL, "a", -1, "/b/c/d;p", "q", "s" } },
  { "g#s", "http://a/b/c/g#s",
    { "http", NULL, "a", -1, "/b/c/g", NULL, "s" } },
  { "g?y#s", "http://a/b/c/g?y#s",
    { "http", NULL, "a", -1, "/b/c/g", "y", "s" } },
  { ";x", "http://a/b/c/;x",
    { "http", NULL, "a", -1, "/b/c/;x", NULL, NULL } },
  { "g;x", "http://a/b/c/g;x",
    { "http", NULL, "a", -1, "/b/c/g;x", NULL, NULL } },
  { "g;x?y#s", "http://a/b/c/g;x?y#s",
    { "http", NULL, "a", -1, "/b/c/g;x", "y", "s" } },
  { ".", "http://a/b/c/",
    { "http", NULL, "a", -1, "/b/c/", NULL, NULL } },
  { "./", "http://a/b/c/",
    { "http", NULL, "a", -1, "/b/c/", NULL, NULL } },
  { "..", "http://a/b/",
    { "http", NULL, "a", -1, "/b/", NULL, NULL } },
  { "../", "http://a/b/",
    { "http", NULL, "a", -1, "/b/", NULL, NULL } },
  { "../g", "http://a/b/g",
    { "http", NULL, "a", -1, "/b/g", NULL, NULL } },
  { "../..", "http://a/",
    { "http", NULL, "a", -1, "/", NULL, NULL } },
  { "../../", "http://a/",
    { "http", NULL, "a", -1, "/", NULL, NULL } },
  { "../../g", "http://a/g",
    { "http", NULL, "a", -1, "/g", NULL, NULL } },
  { "", "http://a/b/c/d;p?q",
    { "http", NULL, "a", -1, "/b/c/d;p", "q", NULL } },
  { "../../../g", "http://a/g",
    { "http", NULL, "a", -1, "/g", NULL, NULL } },
  { "../../../../g", "http://a/g",
    { "http", NULL, "a", -1, "/g", NULL, NULL } },
  { "/./g", "http://a/g",
    { "http", NULL, "a", -1, "/g", NULL, NULL } },
  { "/../g", "http://a/g",
    { "http", NULL, "a", -1, "/g", NULL, NULL } },
  { "g.", "http://a/b/c/g.",
    { "http", NULL, "a", -1, "/b/c/g.", NULL, NULL } },
  { ".g", "http://a/b/c/.g",
    { "http", NULL, "a", -1, "/b/c/.g", NULL, NULL } },
  { "g..", "http://a/b/c/g..",
    { "http", NULL, "a", -1, "/b/c/g..", NULL, NULL } },
  { "..g", "http://a/b/c/..g",
    { "http", NULL, "a", -1, "/b/c/..g", NULL, NULL } },
  { "./../g", "http://a/b/g",
    { "http", NULL, "a", -1, "/b/g", NULL, NULL } },
  { "./g/.", "http://a/b/c/g/",
    { "http", NULL, "a", -1, "/b/c/g/", NULL, NULL } },
  { "g/./h", "http://a/b/c/g/h",
    { "http", NULL, "a", -1, "/b/c/g/h", NULL, NULL } },
  { "g/../h", "http://a/b/c/h",
    { "http", NULL, "a", -1, "/b/c/h", NULL, NULL } },
  { "g;x=1/./y", "http://a/b/c/g;x=1/y",
    { "http", NULL, "a", -1, "/b/c/g;x=1/y", NULL, NULL } },
  { "g;x=1/../y", "http://a/b/c/y",
    { "http", NULL, "a", -1, "/b/c/y", NULL, NULL } },
  { "g?y/./x", "http://a/b/c/g?y/./x",
    { "http", NULL, "a", -1, "/b/c/g", "y/./x", NULL } },
  { "g?y/../x", "http://a/b/c/g?y/../x",
    { "http", NULL, "a", -1, "/b/c/g", "y/../x", NULL } },
  { "g#s/./x", "http://a/b/c/g#s/./x",
    { "http", NULL, "a", -1, "/b/c/g", NULL, "s/./x" } },
  { "g#s/../x", "http://a/b/c/g#s/../x",
    { "http", NULL, "a", -1, "/b/c/g", NULL, "s/../x" } },
  { "http:g", "http:g",
    { "http", NULL, NULL, -1, "g", NULL, NULL } },
  { "http://a/../..", "http://a/",
    { "http", NULL, "a", -1, "/", NULL, NULL } },
  { "ScHeMe://User:P%61ss@HOST.%63om:1234/path/./from/../to%7d/item%2dobj?qu%65ry=something#fr%61gment",
    "scheme://User:Pass@HOST.com:1234/path/to%7D/item-obj?query=something#fragment",
    { "scheme", "User:Pass", "HOST.com", 1234, "/path/to}/item-obj", "query=something", "fragment" } },
  /* test_t corner cases of remove_dot_segments */
  { "http:..", "http:",
    { "http", NULL, NULL, -1, "", NULL, NULL } },
  { "http:../", "http:",
    { "http", NULL, NULL, -1, "", NULL, NULL } },
  { "http:.", "http:",
    { "http", NULL, NULL, -1, "", NULL, NULL } },
  { "http:./", "http:",
    { "http", NULL, NULL, -1, "", NULL, NULL } },
  { "http:a/..", "http:/",
    { "http", NULL, NULL, -1, "/", NULL, NULL } },
  { "http:a/../", "http:/",
    { "http", NULL, NULL, -1, "/", NULL, NULL } },
};
static int num_relative_tests = G_N_ELEMENTS (relative_tests);

static void
test_uri_parsing_relative (void)
{
  int i;
  xuri_t *base, *uri;
  xerror_t *error = NULL;
  xchar_t *resolved;

  base = xuri_parse (relative_test_base, XURI_FLAGS_NONE, &error);
  g_assert_no_error (error);

  for (i = 0; i < num_relative_tests; i++)
    {
      const UriRelativeTest *test = &relative_tests[i];
      xchar_t *tostring;

      uri = xuri_parse_relative (base, test->orig, XURI_FLAGS_NONE, &error);
      g_assert_no_error (error);

      g_assert_cmpstr (xuri_get_scheme (uri),   ==, test->parts.scheme);
      g_assert_cmpstr (xuri_get_userinfo (uri), ==, test->parts.userinfo);
      g_assert_cmpstr (xuri_get_host (uri),     ==, test->parts.host);
      g_assert_cmpint (xuri_get_port (uri),     ==, test->parts.port);
      g_assert_cmpstr (xuri_get_path (uri),     ==, test->parts.path);
      g_assert_cmpstr (xuri_get_query (uri),    ==, test->parts.query);
      g_assert_cmpstr (xuri_get_fragment (uri), ==, test->parts.fragment);

      tostring = xuri_to_string (uri);
      g_assert_cmpstr (tostring, ==, test->resolved);
      g_free (tostring);

      xuri_unref (uri);

      resolved = xuri_resolve_relative (relative_test_base, test->orig, XURI_FLAGS_NONE, &error);
      g_assert_no_error (error);
      g_assert_cmpstr (resolved, ==, test->resolved);
      g_free (resolved);
    }
  uri = xuri_parse_relative (base, "%%", XURI_FLAGS_NONE, &error);
  g_assert_null (uri);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PATH);
  g_clear_error (&error);

  xuri_unref (base);

  resolved = xuri_resolve_relative (NULL, "http://a", XURI_FLAGS_NONE, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (resolved, ==, "http://a");
  g_free (resolved);

  resolved = xuri_resolve_relative ("http://a", "b", XURI_FLAGS_NONE, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (resolved, ==, "http://a/b");
  g_free (resolved);

  resolved = xuri_resolve_relative (NULL, "a", XURI_FLAGS_NONE, &error);
  g_assert_null (resolved);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_FAILED);
  g_clear_error (&error);

  resolved = xuri_resolve_relative ("../b", "a", XURI_FLAGS_NONE, &error);
  g_assert_null (resolved);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_FAILED);
  g_clear_error (&error);

  resolved = xuri_resolve_relative ("%%", "a", XURI_FLAGS_PARSE_RELAXED, &error);
  g_assert_null (resolved);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_FAILED);
  g_clear_error (&error);
}

static void
test_uri_to_string (void)
{
  xuri_t *uri;
  xchar_t *tostring;

  uri = xuri_build (XURI_FLAGS_NONE, "scheme", "userinfo", "host", 1234,
                     "/path", "query", "fragment");

  tostring = xuri_to_string (uri);
  g_assert_cmpstr (tostring, ==, "scheme://userinfo@host:1234/path?query#fragment");
  g_free (tostring);
  xuri_unref (uri);

  uri = xuri_build (XURI_FLAGS_NONE, "scheme", NULL, "fe80::dead:beef%em1", -1, "", NULL, NULL);
  tostring = xuri_to_string (uri);
  g_assert_cmpstr (tostring, ==, "scheme://[fe80::dead:beef%25em1]");
  g_free (tostring);
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_NONE, "scheme", "user", "pass", "auth", "host", 1234,
                               "/path", "query", "fragment");
  tostring = xuri_to_string (uri);
  g_assert_cmpstr (tostring, ==, "scheme://user:pass;auth@host:1234/path?query#fragment");
  g_free (tostring);
  tostring = xuri_to_string_partial (uri, XURI_HIDE_USERINFO);
  g_assert_cmpstr (tostring, ==, "scheme://host:1234/path?query#fragment");
  g_free (tostring);
  tostring = xuri_to_string_partial (uri, XURI_HIDE_QUERY);
  g_assert_cmpstr (tostring, ==, "scheme://user:pass;auth@host:1234/path#fragment");
  g_free (tostring);
  tostring = xuri_to_string_partial (uri, XURI_HIDE_FRAGMENT);
  g_assert_cmpstr (tostring, ==, "scheme://user:pass;auth@host:1234/path?query");
  g_free (tostring);
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_HAS_PASSWORD|XURI_FLAGS_HAS_AUTH_PARAMS,
                               "scheme", "us:er", "pass", "auth", "host", 1234,
                               "/path", "query", "fragment");
  tostring = xuri_to_string (uri);
  g_assert_cmpstr (tostring, ==, "scheme://us%3Aer:pass;auth@host:1234/path?query#fragment");
  g_free (tostring);
  tostring = xuri_to_string_partial (uri, XURI_HIDE_PASSWORD);
  g_assert_cmpstr (tostring, ==, "scheme://us%3Aer;auth@host:1234/path?query#fragment");
  g_free (tostring);
  tostring = xuri_to_string_partial (uri, XURI_HIDE_AUTH_PARAMS);
  g_assert_cmpstr (tostring, ==, "scheme://us%3Aer:pass@host:1234/path?query#fragment");
  g_free (tostring);
  tostring = xuri_to_string_partial (uri, XURI_HIDE_QUERY);
  g_assert_cmpstr (tostring, ==, "scheme://us%3Aer:pass;auth@host:1234/path#fragment");
  g_free (tostring);
  xuri_unref (uri);
}

static void
test_uri_build (void)
{
  xuri_t *uri;

  uri = xuri_build (XURI_FLAGS_NON_DNS, "scheme", "userinfo", "host", 1234,
                     "/path", "query", "fragment");

  /* check ref/unref */
  xuri_ref (uri);
  xuri_unref (uri);

  g_assert_cmpint (xuri_get_flags (uri), ==, XURI_FLAGS_NON_DNS);
  g_assert_cmpstr (xuri_get_scheme (uri), ==, "scheme");
  g_assert_cmpstr (xuri_get_userinfo (uri), ==, "userinfo");
  g_assert_cmpstr (xuri_get_host (uri), ==, "host");
  g_assert_cmpint (xuri_get_port (uri), ==, 1234);
  g_assert_cmpstr (xuri_get_path (uri), ==, "/path");
  g_assert_cmpstr (xuri_get_query (uri), ==, "query");
  g_assert_cmpstr (xuri_get_fragment (uri), ==, "fragment");
  g_assert_cmpstr (xuri_get_user (uri), ==, NULL);
  g_assert_cmpstr (xuri_get_password (uri), ==, NULL);
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_NON_DNS, "scheme", "user", "password",
                               "authparams", "host", 1234,
                               "/path", "query", "fragment");

  g_assert_cmpint (xuri_get_flags (uri), ==, XURI_FLAGS_NON_DNS | XURI_FLAGS_HAS_PASSWORD);
  g_assert_cmpstr (xuri_get_scheme (uri), ==, "scheme");
  g_assert_cmpstr (xuri_get_userinfo (uri), ==, "user:password;authparams");
  g_assert_cmpstr (xuri_get_host (uri), ==, "host");
  g_assert_cmpint (xuri_get_port (uri), ==, 1234);
  g_assert_cmpstr (xuri_get_path (uri), ==, "/path");
  g_assert_cmpstr (xuri_get_query (uri), ==, "query");
  g_assert_cmpstr (xuri_get_fragment (uri), ==, "fragment");
  g_assert_cmpstr (xuri_get_user (uri), ==, "user");
  g_assert_cmpstr (xuri_get_password (uri), ==, "password");
  g_assert_cmpstr (xuri_get_auth_params (uri), ==, "authparams");
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_NONE, "scheme", "user\001", "password\002",
                               "authparams\003", "host", 1234,
                               "/path", "query", "fragment");
  g_assert_cmpstr (xuri_get_userinfo (uri), ==, "user\001:password\002;authparams\003");
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_ENCODED, "scheme", "user%01", "password%02",
                               "authparams%03", "host", 1234,
                               "/path", "query", "fragment");
  g_assert_cmpstr (xuri_get_userinfo (uri), ==, "user%01:password%02;authparams%03");
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_ENCODED, "scheme", NULL, NULL,
                               NULL, "host", 1234,
                               "/path", "query", "fragment");
  g_assert_null (xuri_get_userinfo (uri));
  xuri_unref (uri);

  uri = xuri_build_with_user (XURI_FLAGS_NONE, "scheme", "user", NULL, NULL,
                               "host", 1234,
                               "/path", "query", "fragment");
  g_assert_cmpstr (xuri_get_userinfo (uri), ==, "user");
  xuri_unref (uri);
}

static void
test_uri_split (void)
{
  xchar_t *scheme = NULL;
  xchar_t *userinfo = NULL;
  xchar_t *user = NULL;
  xchar_t *pass = NULL;
  xchar_t *authparams = NULL;
  xchar_t *host = NULL;
  xchar_t *path = NULL;
  xchar_t *query = NULL;
  xchar_t *fragment = NULL;
  xerror_t *error = NULL;
  xint_t port;

  xuri_split ("scheme://user%3Apass%3Bauth@host:1234/path?query#fragment",
               XURI_FLAGS_NONE,
               &scheme,
               &userinfo,
               &host,
               &port,
               &path,
               &query,
               &fragment,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (scheme, ==, "scheme");
  g_assert_cmpstr (userinfo, ==, "user:pass;auth");
  g_assert_cmpstr (host, ==, "host");
  g_assert_cmpint (port, ==, 1234);
  g_assert_cmpstr (path, ==, "/path");
  g_assert_cmpstr (query, ==, "query");
  g_assert_cmpstr (fragment, ==, "fragment");
  g_free (scheme);
  g_free (userinfo);
  g_free (host);
  g_free (path);
  g_free (query);
  g_free (fragment);

  xuri_split ("scheme://user%3Apass%3Bauth@h%01st:1234/path?query#fragment",
               XURI_FLAGS_ENCODED,
               NULL,
               NULL,
               &host,
               NULL,
               NULL,
               NULL,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (host, ==, "h\001st");
  g_free (host);

  xuri_split ("scheme://@@@host:1234/path?query#fragment",
               XURI_FLAGS_ENCODED | XURI_FLAGS_PARSE_RELAXED,
               NULL,
               &userinfo,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (userinfo, ==, "@@");
  g_free (userinfo);


  xuri_split ("http://f;oo/",
               XURI_FLAGS_NONE | XURI_FLAGS_PARSE_RELAXED,
               NULL,
               NULL,
               NULL,
               NULL,
               &path,
               NULL,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (path, ==, ";oo/");
  g_free (path);

  xuri_split ("http://h%01st/path?saisons=%C3%89t%C3%A9%2Bhiver",
               XURI_FLAGS_NONE,
               NULL,
               NULL,
               &host,
               NULL,
               NULL,
               &query,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (host, ==, "h\001st");
  g_assert_cmpstr (query, ==, "saisons=Été+hiver");
  g_free (host);
  g_free (query);

  xuri_split ("http://h%01st/path?saisons=%C3%89t%C3%A9%2Bhiver",
               XURI_FLAGS_ENCODED_QUERY,
               NULL,
               NULL,
               &host,
               NULL,
               NULL,
               &query,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (host, ==, "h\001st");
  g_assert_cmpstr (query, ==, "saisons=%C3%89t%C3%A9%2Bhiver");
  g_free (host);
  g_free (query);

  xuri_split ("http://h%01st/%C3%89t%C3%A9%2Bhiver",
               XURI_FLAGS_ENCODED_PATH,
               NULL,
               NULL,
               NULL,
               NULL,
               &path,
               NULL,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (path, ==, "/%C3%89t%C3%A9%2Bhiver");
  g_free (path);

  xuri_split ("file:///path/to/some%20file",
               XURI_FLAGS_NONE,
               NULL,
               NULL,
               NULL,
               NULL,
               &path,
               NULL,
               NULL,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (path, ==, "/path/to/some file");
  g_free (path);

  xuri_split ("http://h%01st/path#%C3%89t%C3%A9%2Bhiver",
               XURI_FLAGS_ENCODED_FRAGMENT,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL,
               &fragment,
               &error);
  g_assert_no_error (error);
  g_assert_cmpstr (fragment, ==, "%C3%89t%C3%A9%2Bhiver");
  g_free (fragment);

  xuri_split_with_user ("scheme://user:pass;auth@host:1234/path?query#fragment",
                         XURI_FLAGS_HAS_AUTH_PARAMS|XURI_FLAGS_HAS_PASSWORD,
                         NULL,
                         &user,
                         &pass,
                         &authparams,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         &error);
  g_assert_no_error (error);
  g_assert_cmpstr (user, ==, "user");
  g_assert_cmpstr (pass, ==, "pass");
  g_assert_cmpstr (authparams, ==, "auth");
  g_free (user);
  g_free (pass);
  g_free (authparams);

  xuri_split_network ("scheme://user:pass;auth@host:1234/path?query#fragment",
                       XURI_FLAGS_NONE,
                       NULL,
                       NULL,
                       NULL,
                       &error);
  g_assert_no_error (error);

  xuri_split_network ("scheme://user:pass;auth@host:1234/path?query#fragment",
                       XURI_FLAGS_NONE,
                       &scheme,
                       &host,
                       &port,
                       &error);
  g_assert_no_error (error);
  g_assert_cmpstr (scheme, ==, "scheme");
  g_assert_cmpstr (host, ==, "host");
  g_assert_cmpint (port, ==, 1234);
  g_free (scheme);
  g_free (host);

  xuri_split_network ("%00",
                       XURI_FLAGS_NONE, NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PATH);
  g_clear_error (&error);

  xuri_split_network ("/a",
                       XURI_FLAGS_NONE,
                       &scheme,
                       &host,
                       &port,
                       &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_SCHEME);
  g_clear_error (&error);

  xuri_split_network ("schme:#",
                       XURI_FLAGS_NONE,
                       &scheme,
                       &host,
                       &port,
                       &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  xuri_split_network ("scheme://[]/a",
                       XURI_FLAGS_NONE, NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  xuri_split_network ("scheme://user%00:pass;auth@host",
                       XURI_FLAGS_HAS_PASSWORD|XURI_FLAGS_HAS_AUTH_PARAMS,
                       NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_USER);
  g_clear_error (&error);

  xuri_split_network ("scheme://user:pass%00;auth@host",
                       XURI_FLAGS_HAS_PASSWORD|XURI_FLAGS_HAS_AUTH_PARAMS,
                       NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PASSWORD);
  g_clear_error (&error);

  xuri_split_network ("scheme://user:pass;auth@host:1234/path?quer%00y#fragment",
                       XURI_FLAGS_NONE,
                       NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_QUERY);
  g_clear_error (&error);

  xuri_split_network ("scheme://use%00r:pass;auth@host:1234/path",
                       XURI_FLAGS_NONE,
                       NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_USER);
  g_clear_error (&error);

  xuri_split ("scheme://user:pass;auth@host:1234/path?query#fragm%00ent",
               XURI_FLAGS_NONE,
               &scheme,
               &userinfo,
               &host,
               &port,
               &path,
               &query,
               &fragment,
               &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_FRAGMENT);
  g_clear_error (&error);

  xuri_split_with_user ("scheme://user:pa%x0s;auth@host:1234/path?query#fragment",
                         XURI_FLAGS_HAS_PASSWORD,
                         &scheme,
                         &user,
                         &pass,
                         &authparams,
                         &host,
                         &port,
                         &path,
                         &query,
                         &fragment,
                         &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PASSWORD);
  g_clear_error (&error);

  xuri_split_with_user ("scheme://user:pass;auth%00@host",
                         XURI_FLAGS_HAS_PASSWORD|XURI_FLAGS_HAS_AUTH_PARAMS,
                         &scheme,
                         &user,
                         &pass,
                         &authparams,
                         &host,
                         &port,
                         &path,
                         &query,
                         &fragment,
                         &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_AUTH_PARAMS);
  g_clear_error (&error);

  xuri_split_network ("scheme://user:pass%00;auth@host",
                       XURI_FLAGS_HAS_PASSWORD|XURI_FLAGS_HAS_AUTH_PARAMS,
                       NULL, NULL, NULL, &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PASSWORD);
  g_clear_error (&error);

  /* Path not started correctly */
  xuri_split("scheme://hostname:123path?query#fragment",
              XURI_FLAGS_NONE,
              &scheme,
              &userinfo,
              &host,
              &port,
              &path,
              &query,
              &fragment,
              &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PORT);
  g_clear_error (&error);

  /* Brackets that don't close */
  xuri_split("scheme://[01:23:45:67:89:ab:cd:ef:123/path",
              XURI_FLAGS_NONE,
              &scheme,
              &userinfo,
              &host,
              &port,
              &path,
              &query,
              &fragment,
              &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  /* IPv6 hostname without brackets */
  xuri_split("scheme://01:23:45:67:89:ab:cd:ef:123/path",
              XURI_FLAGS_NONE,
              &scheme,
              &userinfo,
              &host,
              &port,
              &path,
              &query,
              &fragment,
              &error);
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PORT);
  g_clear_error (&error);
}

static void
test_uri_is_valid (void)
{
  xerror_t *error = NULL;

  g_assert_true (xuri_is_valid ("http://[::192.9.5.5]/ipng", XURI_FLAGS_NONE, NULL));
  g_assert_true (xuri_is_valid ("http://127.127.127.127/", XURI_FLAGS_NONE, NULL));
  g_assert_true (xuri_is_valid ("http://127.127.127.b/", XURI_FLAGS_NONE, NULL));
  g_assert_true (xuri_is_valid ("http://\xc3\x89XAMPLE.COM/", XURI_FLAGS_NONE, NULL));

  g_assert_true (xuri_is_valid ("  \r http\t://f oo  \t\n ", XURI_FLAGS_PARSE_RELAXED, NULL));
  g_assert_false (xuri_is_valid ("  \r http\t://f oo  \t\n ", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_SCHEME);
  g_clear_error (&error);

  g_assert_false (xuri_is_valid ("http://[::192.9.5.5/ipng", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  g_assert_true (xuri_is_valid ("http://[fe80::dead:beef%25wef]/", XURI_FLAGS_NONE, NULL));
  g_assert_false (xuri_is_valid ("http://[fe80::dead:beef%wef%]/", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  g_assert_false (xuri_is_valid ("http://%00/", XURI_FLAGS_NON_DNS, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  g_assert_true (xuri_is_valid ("http://foo/", XURI_FLAGS_NON_DNS, &error));

  g_assert_false (xuri_is_valid ("http://%00/", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  g_assert_false (xuri_is_valid ("http://%30.%30.%30.%30/", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_HOST);
  g_clear_error (&error);

  g_assert_false (xuri_is_valid ("http://host:port", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PORT);
  g_clear_error (&error);

  g_assert_false (xuri_is_valid ("http://host:65536", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PORT);
  g_clear_error (&error);

  g_assert_false (xuri_is_valid ("http://host:6553l", XURI_FLAGS_NONE, &error));
  g_assert_error (error, XURI_ERROR, XURI_ERROR_BAD_PORT);
  g_clear_error (&error);

  g_assert_true (xuri_is_valid ("data:,Hello", XURI_FLAGS_NONE, &error));

  g_assert_true (xuri_is_valid ("B:\\foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("B:/foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("B://foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("B:foo.txt", XURI_FLAGS_NONE, &error));

  g_assert_true (xuri_is_valid ("fd://0", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("AB:\\foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("AB:/foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("AB://foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("AB:foo.txt", XURI_FLAGS_NONE, &error));

  g_assert_true (xuri_is_valid ("ABC:/foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("ABC://foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("ABC:foo.txt", XURI_FLAGS_NONE, &error));

  g_assert_true (xuri_is_valid ("ABCD:/foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("ABCD://foo.txt", XURI_FLAGS_NONE, &error));
  g_assert_true (xuri_is_valid ("ABCD:foo.txt", XURI_FLAGS_NONE, &error));
}

static const struct
{
  /* Inputs */
  const xchar_t *uri;
  xchar_t *separators;
  xuri_params_flags_t flags;
  /* Outputs */
  /* key, value, key, value, …, limited to length 2*expected_n_params */
  xssize_t expected_n_iter;  /* -1 => error expected */
  const xchar_t *expected_iter_key_values[6];
  xssize_t expected_n_params;  /* -1 => error expected */
  const xchar_t *expected_param_key_values[6];
} params_tests[] =
  {
    { "p1=foo&p2=bar;p3=baz", "&;", XURI_PARAMS_NONE,
      3, { "p1", "foo", "p2", "bar", "p3", "baz" },
      3, { "p1", "foo", "p2", "bar", "p3", "baz" }},
    { "p1=foo&p2=bar", "", XURI_PARAMS_NONE,
      1, { "p1", "foo&p2=bar" },
      1, { "p1", "foo&p2=bar" }},
    { "p1=foo&&P1=bar", "&", XURI_PARAMS_NONE,
      1, { "p1", "foo" },
      -1, { NULL, }},
    { "%00=foo", "&", XURI_PARAMS_NONE,
      0, { NULL, },
      -1, { NULL, }},
    { "p1=%00", "&", XURI_PARAMS_NONE,
      0, { NULL, },
      -1, { NULL, }},
    { "p1=foo&p1=bar", "&", XURI_PARAMS_NONE,
      2, { "p1", "foo", "p1", "bar" },
      1, { "p1", "bar", NULL, }},
    { "p1=foo&P1=bar", "&", XURI_PARAMS_CASE_INSENSITIVE,
      2, { "p1", "foo", "P1", "bar" },
      1, { "p1", "bar", NULL, }},
    { "=%", "&", XURI_PARAMS_PARSE_RELAXED,
      1, { "", "%", NULL, },
      1, { "", "%", NULL, }},
    { "=", "&", XURI_PARAMS_NONE,
      1, { "", "", NULL, },
      1, { "", "", NULL, }},
    { "foo", "&", XURI_PARAMS_NONE,
      0, { NULL, },
      -1, { NULL, }},
    { "foo=bar+%26+baz&saisons=%C3%89t%C3%A9%2Bhiver", "&", XURI_PARAMS_WWW_FORM,
      2, { "foo", "bar & baz", "saisons", "Été+hiver", NULL, },
      2, { "foo", "bar & baz", "saisons", "Été+hiver", NULL, }},
    { "foo=bar+%26+baz&saisons=%C3%89t%C3%A9%2Bhiver", "&", XURI_PARAMS_NONE,
      2, { "foo", "bar+&+baz", "saisons", "Été+hiver", NULL, },
      2, { "foo", "bar+&+baz", "saisons", "Été+hiver", NULL, }},
    { "token=exp=123~acl=/QualityLevels(*~hmac=0cb", "&", XURI_PARAMS_NONE,
      1, { "token", "exp=123~acl=/QualityLevels(*~hmac=0cb", NULL, },
      1, { "token", "exp=123~acl=/QualityLevels(*~hmac=0cb", NULL, }},
  };

static void
test_uri_iter_params (xconstpointer test_data)
{
  xerror_t *err = NULL;
  xboolean_t use_nul_terminated = GPOINTER_TO_INT (test_data);
  xsize_t i, n;

  for (i = 0; i < G_N_ELEMENTS (params_tests); i++)
    {
      xuri_params_iter_t iter;
      xchar_t *uri, *attr, *value;
      xssize_t uri_len;

      g_test_message ("URI %" G_GSIZE_FORMAT ": %s", i, params_tests[i].uri);

      xassert (params_tests[i].expected_n_params < 0 ||
                params_tests[i].expected_n_params <= (xssize_t) G_N_ELEMENTS (params_tests[i].expected_param_key_values) / 2);

      /* The tests get run twice: once with the length unspecified, using a
       * nul-terminated string; and once with the length specified and a copy of
       * the string with the trailing nul explicitly removed (to help catch
       * buffer overflows). */
      if (use_nul_terminated)
        {
          uri_len = -1;
          uri = xstrdup (params_tests[i].uri);
        }
      else
        {
          uri_len = strlen (params_tests[i].uri);  /* no trailing nul */
          uri = g_memdup2 (params_tests[i].uri, uri_len);
        }

      /* Run once without extracting the attr or value, just to check the numbers. */
      n = 0;
      xuri_params_iter_init (&iter, params_tests[i].uri, -1, params_tests[i].separators, params_tests[i].flags);
      while (xuri_params_iter_next (&iter, NULL, NULL, &err))
        n++;
      g_assert_cmpint (n, ==, params_tests[i].expected_n_iter);
      if (err)
        {
          g_assert_error (err, XURI_ERROR, XURI_ERROR_FAILED);
          g_clear_error (&err);
        }

      /* Run again and check the strings too. */
      n = 0;
      xuri_params_iter_init (&iter, params_tests[i].uri, -1, params_tests[i].separators, params_tests[i].flags);
      while (xuri_params_iter_next (&iter, &attr, &value, &err))
        {
          g_assert_cmpstr (attr, ==, params_tests[i].expected_iter_key_values[n * 2]);
          g_assert_cmpstr (value, ==, params_tests[i].expected_iter_key_values[n * 2 + 1]);
          n++;
          g_free (attr);
          g_free (value);
        }
      g_assert_cmpint (n, ==, params_tests[i].expected_n_iter);
      if (err)
        {
          g_assert_error (err, XURI_ERROR, XURI_ERROR_FAILED);
          g_clear_error (&err);
        }

      g_free (uri);
    }
}

static void
test_uri_parse_params (xconstpointer test_data)
{
  xerror_t *err = NULL;
  xboolean_t use_nul_terminated = GPOINTER_TO_INT (test_data);
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (params_tests); i++)
    {
      xhashtable_t *params;
      xchar_t *uri = NULL;
      xssize_t uri_len;

      g_test_message ("URI %" G_GSIZE_FORMAT ": %s", i, params_tests[i].uri);

      xassert (params_tests[i].expected_n_params < 0 ||
                params_tests[i].expected_n_params <= (xssize_t) G_N_ELEMENTS (params_tests[i].expected_param_key_values) / 2);

      /* The tests get run twice: once with the length unspecified, using a
       * nul-terminated string; and once with the length specified and a copy of
       * the string with the trailing nul explicitly removed (to help catch
       * buffer overflows). */
      if (use_nul_terminated)
        {
          uri_len = -1;
          uri = xstrdup (params_tests[i].uri);
        }
      else
        {
          uri_len = strlen (params_tests[i].uri);  /* no trailing nul */
          uri = g_memdup2 (params_tests[i].uri, uri_len);
        }

      params = xuri_parse_params (uri, uri_len, params_tests[i].separators, params_tests[i].flags, &err);

      if (params_tests[i].expected_n_params < 0)
        {
          g_assert_null (params);
          g_assert_error (err, XURI_ERROR, XURI_ERROR_FAILED);
          g_clear_error (&err);
        }
      else
        {
          xsize_t j;

          g_assert_no_error (err);
          g_assert_cmpint (xhash_table_size (params), ==, params_tests[i].expected_n_params);

          for (j = 0; j < (xsize_t) params_tests[i].expected_n_params; j += 2)
            g_assert_cmpstr (xhash_table_lookup (params, params_tests[i].expected_param_key_values[j]), ==,
                             params_tests[i].expected_param_key_values[j + 1]);
        }

      g_clear_pointer (&params, xhash_table_unref);
      g_free (uri);
    }
}

static void
test_uri_join (void)
{
  xchar_t *uri = NULL;

  uri = xuri_join (XURI_FLAGS_NONE, "foo", "some:user@info", "bar", -1, "", NULL, NULL);
  g_assert_cmpstr (uri, ==, "foo://some:user%40info@bar");
  g_free (uri);

  uri = xuri_join (XURI_FLAGS_NONE, NULL, NULL, NULL, -1, "/foo", "abc", NULL);
  g_assert_cmpstr (uri, ==, "/foo?abc");
  g_free (uri);

  uri = xuri_join (XURI_FLAGS_NONE, NULL, NULL, "hostname", -1, "/foo", "abc", NULL);
  g_assert_cmpstr (uri, ==, "//hostname/foo?abc");
  g_free (uri);

  uri = xuri_join_with_user (XURI_FLAGS_NONE, "scheme", "user\001", "pass\002", "authparams\003",
                              "host", 9876, "/path", "query", "fragment");
  g_assert_cmpstr (uri, ==, "scheme://user%01:pass%02;authparams%03@host:9876/path?query#fragment");
  g_free (uri);

  uri = xuri_join_with_user (XURI_FLAGS_NONE, "scheme", "user\001", "pass\002", "authparams\003",
                              "::192.9.5.5", 9876, "/path", "query", "fragment");
  g_assert_cmpstr (uri, ==, "scheme://user%01:pass%02;authparams%03@[::192.9.5.5]:9876/path?query#fragment");
  g_free (uri);

  uri = xuri_join_with_user (XURI_FLAGS_ENCODED,
                              "scheme", "user%01", "pass%02", "authparams%03",
                              "::192.9.5.5", 9876, "/path", "query", "fragment");
  g_assert_cmpstr (uri, ==,
                   "scheme://user%01:pass%02;authparams%03@[::192.9.5.5]:9876/path?query#fragment");
  g_free (uri);

  uri = xuri_join (XURI_FLAGS_NONE, "scheme", NULL, "foo:bar._webdav._tcp.local", -1, "", NULL, NULL);
  g_assert_cmpstr (uri, ==, "scheme://foo%3Abar._webdav._tcp.local");
  g_free (uri);
}

static void
test_uri_join_split_round_trip (void)
{
  xuri_flags_t flags = XURI_FLAGS_HAS_PASSWORD | XURI_FLAGS_HAS_AUTH_PARAMS;
  xuint_t i;

  g_test_summary ("test_t that joining different URI components survives a round trip");

  /* Each bit in @i indicates whether the corresponding URI field should be set
   * or %NULL. */
  for (i = 0; i < (1 << 8); i++)
    {
      xchar_t *uri = NULL;
      const xchar_t *scheme, *user, *password, *auth_params, *host, *path, *query, *fragment;
      xint_t port;
      xchar_t *scheme_out = NULL, *user_out = NULL, *password_out = NULL;
      xchar_t *auth_params_out = NULL, *host_out = NULL, *path_out = NULL;
      xchar_t *query_out = NULL, *fragment_out = NULL;
      xint_t port_out = -1;
      xboolean_t split_success;
      xerror_t *local_error = NULL;

      g_test_message ("Combination %u", i);

      scheme = (i & (1 << 8)) ? "scheme" : NULL;
      host = (i & (1 << 4)) ? "host" : NULL;
      user = (host != NULL && i & (1 << 7)) ? "user" : NULL;  /* only supported if host is also set */
      password = (host != NULL && user != NULL && i & (1 << 6)) ? "password" : NULL;  /* only supported if host and user are also set */
      auth_params = (host != NULL && user != NULL && i & (1 << 5)) ? "auth_params" : NULL;  /* only supported if host and user are also set */
      port = (host != NULL && i & (1 << 3)) ? 123 : -1;  /* only supported if host is also set */
      path = (i & (1 << 2)) ? "/path" : "";  /* the only mandatory component */
      query = (i & (1 << 1)) ? "query" : NULL;
      fragment = (i & (1 << 0)) ? "fragment" : NULL;

      uri = xuri_join_with_user (flags, scheme, user, password, auth_params,
                                  host, port, path, query, fragment);
      g_assert_nonnull (uri);

      split_success = xuri_split_with_user (uri, flags, &scheme_out, &user_out,
                                             &password_out, &auth_params_out,
                                             &host_out, &port_out, &path_out,
                                             &query_out, &fragment_out,
                                             &local_error);
      g_assert_no_error (local_error);
      g_assert_true (split_success);

      g_assert_cmpstr (scheme, ==, scheme_out);
      g_assert_cmpstr (user, ==, user_out);
      g_assert_cmpstr (password, ==, password_out);
      g_assert_cmpstr (auth_params, ==, auth_params_out);
      g_assert_cmpstr (host, ==, host_out);
      g_assert_cmpint (port, ==, port_out);
      g_assert_cmpstr (path, ==, path_out);
      g_assert_cmpstr (query, ==, query_out);
      g_assert_cmpstr (fragment, ==, fragment_out);

      g_free (uri);
      g_free (scheme_out);
      g_free (user_out);
      g_free (password_out);
      g_free (auth_params_out);
      g_free (host_out);
      g_free (path_out);
      g_free (query_out);
      g_free (fragment_out);
    }
}

static const struct
{
  /* Inputs */
  const xchar_t *base;
  const xchar_t *uri;
  xuri_flags_t flags;
  /* Outputs */
  const xchar_t *uri_string;
  const xchar_t *path;
  int port;
} normalize_parse_tests[] =
  {
    { NULL, "http://foo/path with spaces", XURI_FLAGS_ENCODED,
      "http://foo/path%20with%20spaces", "/path%20with%20spaces", -1 },
    { NULL, "http://foo/path with spaces 2", XURI_FLAGS_ENCODED_PATH,
      "http://foo/path%20with%20spaces%202", "/path%20with%20spaces%202", -1 },
    { NULL, "http://foo/%aa", XURI_FLAGS_ENCODED,
      "http://foo/%AA", "/%AA", -1 },
    { NULL, "http://foo/p\xc3\xa4th/", XURI_FLAGS_ENCODED | XURI_FLAGS_PARSE_RELAXED,
      "http://foo/p%C3%A4th/", "/p%C3%A4th/", -1 },
    { NULL, "http://foo", XURI_FLAGS_NONE,
      "http://foo", "", -1 },
    { NULL, "http://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "http://foo/", "/", 80 },
    { NULL, "nothttp://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "nothttp://foo", "", -1 },
    { NULL, "http://foo:80", XURI_FLAGS_NONE,
      "http://foo:80", "", 80 },
    { NULL, "http://foo:80", XURI_FLAGS_SCHEME_NORMALIZE,
      "http://foo/", "/", 80 },
    { NULL, "http://foo:8080", XURI_FLAGS_SCHEME_NORMALIZE,
      "http://foo:8080/", "/", 8080 },
    { NULL, "https://foo:443", XURI_FLAGS_SCHEME_NORMALIZE,
      "https://foo/", "/", 443 },
    { NULL, "https://foo:943", XURI_FLAGS_SCHEME_NORMALIZE,
      "https://foo:943/", "/", 943 },
    { NULL, "ws://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "ws://foo/", "/", 80 },
    { NULL, "wss://foo:443", XURI_FLAGS_SCHEME_NORMALIZE,
      "wss://foo/", "/", 443 },
    { NULL, "ftp://foo", XURI_FLAGS_NONE,
      "ftp://foo", "", -1 },
    { NULL, "ftp://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "ftp://foo", "", 21 },
    { NULL, "ftp://foo:21", XURI_FLAGS_SCHEME_NORMALIZE,
      "ftp://foo", "", 21 },
    { NULL, "ftp://foo:2100", XURI_FLAGS_SCHEME_NORMALIZE,
      "ftp://foo:2100", "", 2100 },
    { NULL, "nothttp://foo:80", XURI_FLAGS_SCHEME_NORMALIZE,
      "nothttp://foo:80", "", 80 },
    { "http://foo", "//bar", XURI_FLAGS_SCHEME_NORMALIZE,
      "http://bar/", "/", 80 },
    { "http://foo", "//bar:80", XURI_FLAGS_SCHEME_NORMALIZE,
      "http://bar/", "/", 80 },
    { "nothttp://foo", "//bar:80", XURI_FLAGS_SCHEME_NORMALIZE,
      "nothttp://bar:80", "", 80 },
    { "http://foo", "//bar", XURI_FLAGS_NONE,
      "http://bar", "", -1 },
    { "ScHeMe://User:P%61ss@HOST.%63om:1234/path",
      "ScHeMe://User:P%61ss@HOST.%63om:1234/path/./from/../to%7d/item%2dobj?qu%65ry=something#fr%61gment",
      XURI_FLAGS_SCHEME_NORMALIZE,
      "scheme://User:Pass@HOST.com:1234/path/to%7D/item-obj?query=something#fragment",
      "/path/to}/item-obj", 1234 },
  };

static const struct
{
  /* Inputs */
  const xchar_t *uri;
  xuri_flags_t flags;
  /* Outputs */
  const char *scheme;
  const xchar_t *path;
  int port;
} normalize_split_tests[] =
  {
    { "HTTP://foo", XURI_FLAGS_ENCODED,
      "http", "", -1 },
    { "HTTP://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "http", "/", 80 },
    { "http://foo:80/", XURI_FLAGS_SCHEME_NORMALIZE,
      "http", "/", 80 },
    { "http://foo:8080/bar", XURI_FLAGS_SCHEME_NORMALIZE,
      "http", "/bar", 8080 },
    { "ws://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "ws", "/", 80 },
    { "https://foo", XURI_FLAGS_ENCODED,
      "https", "", -1 },
    { "https://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "https", "/", 443 },
    { "https://foo:443/", XURI_FLAGS_SCHEME_NORMALIZE,
      "https", "/", 443 },
    { "wss://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "wss", "/", 443 },
    { "ftp://foo", XURI_FLAGS_ENCODED,
      "ftp", "", -1 },
    { "ftp://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "ftp", "", 21 },
    { "ftp://foo:21", XURI_FLAGS_SCHEME_NORMALIZE,
      "ftp", "", 21 },
    { "scheme://foo", XURI_FLAGS_SCHEME_NORMALIZE,
      "scheme", "", -1 },
  };

static const struct
{
  /* Inputs */
  xuri_flags_t flags;
  const xchar_t *scheme;
  const xchar_t *host;
  int port;
  const xchar_t *path;
  /* Outputs */
  const xchar_t *uri;
} normalize_join_tests[] =
  {
    { XURI_FLAGS_NONE, "http", "foo", -1, "",
      "http://foo" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "http", "foo", -1, "",
      "http://foo/" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "http", "foo", 80, "",
      "http://foo/" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "http", "foo", 8080, "",
      "http://foo:8080/" },
    { XURI_FLAGS_NONE, "http", "foo", 80, "",
      "http://foo:80" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "ws", "foo", 80, "",
      "ws://foo/" },
    { XURI_FLAGS_NONE, "https", "foo", -1, "",
      "https://foo" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "https", "foo", -1, "",
      "https://foo/" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "https", "foo", 443, "",
      "https://foo/" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "https", "foo", 943, "",
      "https://foo:943/" },
    { XURI_FLAGS_NONE, "https", "foo", 443, "",
      "https://foo:443" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "wss", "foo", 443, "",
      "wss://foo/" },
    { XURI_FLAGS_NONE, "ftp", "foo", -1, "",
      "ftp://foo" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "ftp", "foo", -1, "",
      "ftp://foo" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "ftp", "foo", 21, "",
      "ftp://foo" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "ftp", "foo", 2020, "",
      "ftp://foo:2020" },
    { XURI_FLAGS_NONE, "ftp", "foo", 21, "",
      "ftp://foo:21" },
    { XURI_FLAGS_SCHEME_NORMALIZE, "scheme", "foo", 80, "",
      "scheme://foo:80" },
  };

static void
test_uri_normalize (void)
{
  xsize_t i;
  int port;
  char *path;
  char *uri_string;

  for (i = 0; i < G_N_ELEMENTS (normalize_parse_tests); ++i)
    {
      xuri_t *uri, *base = NULL;

      if (normalize_parse_tests[i].base)
        base = xuri_parse (normalize_parse_tests[i].base, normalize_parse_tests[i].flags, NULL);

      uri = xuri_parse_relative (base,
                                  normalize_parse_tests[i].uri,
                                  normalize_parse_tests[i].flags,
                                  NULL);
      uri_string = xuri_to_string (uri);

      g_assert_nonnull (uri);
      g_assert_cmpstr (xuri_get_path (uri), ==, normalize_parse_tests[i].path);
      g_assert_cmpint (xuri_get_port (uri), ==, normalize_parse_tests[i].port);
      g_assert_cmpstr (uri_string, ==, normalize_parse_tests[i].uri_string);

      g_free (uri_string);
      xuri_unref (uri);
      if (base)
        xuri_unref (base);
    }

  for (i = 0; i < G_N_ELEMENTS (normalize_split_tests); ++i)
    {
      char *scheme;

      /* Testing a codepath where scheme is NULL but internally we still normalize it. */
      g_assert_true (xuri_split (normalize_split_tests[i].uri, normalize_split_tests[i].flags,
                                  NULL, NULL, NULL, &port, &path, NULL, NULL, NULL));
      g_assert_cmpstr (path, ==, normalize_split_tests[i].path);
      g_assert_cmpint (port, ==, normalize_split_tests[i].port);
      g_free (path);

      g_assert_true (xuri_split (normalize_split_tests[i].uri, normalize_split_tests[i].flags,
                                  &scheme, NULL, NULL, &port, &path, NULL, NULL, NULL));
      g_assert_cmpstr (scheme, ==, normalize_split_tests[i].scheme);
      g_assert_cmpstr (path, ==, normalize_split_tests[i].path);
      g_assert_cmpint (port, ==, normalize_split_tests[i].port);
      g_free (scheme);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (normalize_join_tests); ++i)
    {
      uri_string = xuri_join (normalize_join_tests[i].flags, normalize_join_tests[i].scheme, NULL,
                               normalize_join_tests[i].host, normalize_join_tests[i].port,
                               normalize_join_tests[i].path, NULL, NULL);
      g_assert_cmpstr (uri_string, ==, normalize_join_tests[i].uri);
      g_free (uri_string);
    }
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/uri/file-to-uri", run_file_to_uri_tests);
  g_test_add_func ("/uri/file-from-uri", run_file_from_uri_tests);
  g_test_add_func ("/uri/file-roundtrip", run_file_roundtrip_tests);
  g_test_add_func ("/uri/list", run_uri_list_tests);
  g_test_add_func ("/uri/unescape-string", test_uri_unescape_string);
  g_test_add_data_func ("/uri/unescape-bytes/nul-terminated", GINT_TO_POINTER (TRUE), test_uri_unescape_bytes);
  g_test_add_data_func ("/uri/unescape-bytes/length", GINT_TO_POINTER (FALSE), test_uri_unescape_bytes);
  g_test_add_func ("/uri/unescape-segment", test_uri_unescape_segment);
  g_test_add_func ("/uri/escape-string", test_uri_escape_string);
  g_test_add_func ("/uri/escape-bytes", test_uri_escape_bytes);
  g_test_add_func ("/uri/scheme", test_uri_scheme);
  g_test_add_func ("/uri/parsing/absolute", test_uri_parsing_absolute);
  g_test_add_func ("/uri/parsing/relative", test_uri_parsing_relative);
  g_test_add_func ("/uri/build", test_uri_build);
  g_test_add_func ("/uri/split", test_uri_split);
  g_test_add_func ("/uri/is_valid", test_uri_is_valid);
  g_test_add_func ("/uri/to-string", test_uri_to_string);
  g_test_add_func ("/uri/join", test_uri_join);
  g_test_add_func ("/uri/join-split-round-trip", test_uri_join_split_round_trip);
  g_test_add_func ("/uri/normalize", test_uri_normalize);
  g_test_add_data_func ("/uri/iter-params/nul-terminated", GINT_TO_POINTER (TRUE), test_uri_iter_params);
  g_test_add_data_func ("/uri/iter-params/length", GINT_TO_POINTER (FALSE), test_uri_iter_params);
  g_test_add_data_func ("/uri/parse-params/nul-terminated", GINT_TO_POINTER (TRUE), test_uri_parse_params);
  g_test_add_data_func ("/uri/parse-params/length", GINT_TO_POINTER (FALSE), test_uri_parse_params);

  return g_test_run ();
}
