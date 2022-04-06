/* Unit tests for xstring
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include "glib.h"


static void
test_string_chunks (void)
{
  xstring_chunk_t *string_chunk;
  xchar_t *tmp_string, *tmp_string_2;
  xint_t i;

  string_chunk = xstring_chunk_new (1024);

  for (i = 0; i < 100000; i ++)
    {
      tmp_string = xstring_chunk_insert (string_chunk, "hi pete");
      g_assert_cmpstr ("hi pete", ==, tmp_string);
    }

  tmp_string_2 = xstring_chunk_insert_const (string_chunk, tmp_string);
  g_assert (tmp_string_2 != tmp_string);
  g_assert_cmpstr (tmp_string_2, ==, tmp_string);

  tmp_string = xstring_chunk_insert_const (string_chunk, tmp_string);
  g_assert_cmpstr (tmp_string_2, ==, tmp_string);

  xstring_chunk_clear (string_chunk);
  xstring_chunk_free (string_chunk);
}

static void
test_string_chunk_insert (void)
{
  const xchar_t s0[] = "Testing xstring_chunk_t";
  const xchar_t s1[] = "a\0b\0c\0d\0";
  const xchar_t s2[] = "Hello, world";
  xstring_chunk_t *chunk;
  xchar_t *str[3];

  chunk = xstring_chunk_new (512);

  str[0] = xstring_chunk_insert (chunk, s0);
  str[1] = xstring_chunk_insert_len (chunk, s1, 8);
  str[2] = xstring_chunk_insert (chunk, s2);

  g_assert (memcmp (s0, str[0], sizeof s0) == 0);
  g_assert (memcmp (s1, str[1], sizeof s1) == 0);
  g_assert (memcmp (s2, str[2], sizeof s2) == 0);

  xstring_chunk_free (chunk);
}

static void
test_string_new (void)
{
  xstring_t *string1, *string2;

  string1 = xstring_new ("hi pete!");
  string2 = xstring_new (NULL);

  g_assert (string1 != NULL);
  g_assert (string2 != NULL);
  g_assert (strlen (string1->str) == string1->len);
  g_assert (strlen (string2->str) == string2->len);
  g_assert (string2->len == 0);
  g_assert_cmpstr ("hi pete!", ==, string1->str);
  g_assert_cmpstr ("", ==, string2->str);

  xstring_free (string1, TRUE);
  xstring_free (string2, TRUE);

  string1 = xstring_new_len ("foo", -1);
  string2 = xstring_new_len ("foobar", 3);

  g_assert_cmpstr (string1->str, ==, "foo");
  g_assert_cmpint (string1->len, ==, 3);
  g_assert_cmpstr (string2->str, ==, "foo");
  g_assert_cmpint (string2->len, ==, 3);

  xstring_free (string1, TRUE);
  xstring_free (string2, TRUE);
}

G_GNUC_PRINTF(2, 3)
static void
my_string_printf (xstring_t     *string,
                  const xchar_t *format,
                  ...)
{
  va_list args;

  va_start (args, format);
  xstring_vprintf (string, format, args);
  va_end (args);
}

static void
test_string_printf (void)
{
  xstring_t *string;

  string = xstring_new (NULL);

#ifndef G_OS_WIN32
  /* MSVC and mingw32 use the same run-time C library, which doesn't like
     the %10000.10000f format... */
  xstring_printf (string, "%s|%0100d|%s|%0*d|%*.*f|%10000.10000f",
		   "this pete guy sure is a wuss, like he's the number ",
		   1,
		   " wuss.  everyone agrees.\n",
		   10, 666, 15, 15, 666.666666666, 666.666666666);
#else
  xstring_printf (string, "%s|%0100d|%s|%0*d|%*.*f|%100.100f",
		   "this pete guy sure is a wuss, like he's the number ",
		   1,
		   " wuss.  everyone agrees.\n",
		   10, 666, 15, 15, 666.666666666, 666.666666666);
#endif

  xstring_free (string, TRUE);

  string = xstring_new (NULL);
  xstring_printf (string, "bla %s %d", "foo", 99);
  g_assert_cmpstr (string->str, ==, "bla foo 99");
  my_string_printf (string, "%d,%s,%d", 1, "two", 3);
  g_assert_cmpstr (string->str, ==, "1,two,3");

  xstring_free (string, TRUE);
}

static void
test_string_assign (void)
{
  xstring_t *string;

  string = xstring_new (NULL);
  xstring_assign (string, "boring text");
  g_assert_cmpstr (string->str, ==, "boring text");
  xstring_free (string, TRUE);

  /* assign with string overlap */
  string = xstring_new ("textbeforetextafter");
  xstring_assign (string, string->str + 10);
  g_assert_cmpstr (string->str, ==, "textafter");
  xstring_free (string, TRUE);

  string = xstring_new ("boring text");
  xstring_assign (string, string->str);
  g_assert_cmpstr (string->str, ==, "boring text");
  xstring_free (string, TRUE);
}

static void
test_string_append_c (void)
{
  xstring_t *string;
  xint_t i;

  string = xstring_new ("hi pete!");

  for (i = 0; i < 10000; i++)
    if (i % 2)
      xstring_append_c (string, 'a'+(i%26));
    else
      (xstring_append_c) (string, 'a'+(i%26));

  g_assert((strlen("hi pete!") + 10000) == string->len);
  g_assert((strlen("hi pete!") + 10000) == strlen(string->str));

  xstring_free (string, TRUE);
}

static void
test_string_append (void)
{
  xstring_t *string;

  /* append */
  string = xstring_new ("firsthalf");
  xstring_append (string, "lasthalf");
  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");
  xstring_free (string, TRUE);

  /* append_len */
  string = xstring_new ("firsthalf");
  xstring_append_len (string, "lasthalfjunkjunk", strlen ("lasthalf"));
  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");
  xstring_free (string, TRUE);
}

static void string_append_vprintf_va (xstring_t     *string,
                                      const xchar_t *format,
                                      ...) G_GNUC_PRINTF (2, 3);

/* Wrapper around xstring_append_vprintf() which takes varargs */
static void
string_append_vprintf_va (xstring_t     *string,
                          const xchar_t *format,
                          ...)
{
  va_list args;

  va_start (args, format);
  xstring_append_vprintf (string, format, args);
  va_end (args);
}

static void
test_string_append_vprintf (void)
{
  xstring_t *string;

  /* append */
  string = xstring_new ("firsthalf");

  string_append_vprintf_va (string, "some %s placeholders", "format");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
  string_append_vprintf_va (string, "%l", "invalid");
#pragma GCC diagnostic pop

  g_assert_cmpstr (string->str, ==, "firsthalfsome format placeholders");

  xstring_free (string, TRUE);
}

static void
test_string_prepend_c (void)
{
  xstring_t *string;
  xint_t i;

  string = xstring_new ("hi pete!");

  for (i = 0; i < 10000; i++)
    xstring_prepend_c (string, 'a'+(i%26));

  g_assert((strlen("hi pete!") + 10000) == string->len);
  g_assert((strlen("hi pete!") + 10000) == strlen(string->str));

  xstring_free (string, TRUE);
}

static void
test_string_prepend (void)
{
  xstring_t *string;

  /* prepend */
  string = xstring_new ("lasthalf");
  xstring_prepend (string, "firsthalf");
  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");
  xstring_free (string, TRUE);

  /* prepend_len */
  string = xstring_new ("lasthalf");
  xstring_prepend_len (string, "firsthalfjunkjunk", strlen ("firsthalf"));
  g_assert_cmpstr (string->str, ==, "firsthalflasthalf");
  xstring_free (string, TRUE);
}

static void
test_string_insert (void)
{
  xstring_t *string;

  /* insert */
  string = xstring_new ("firstlast");
  xstring_insert (string, 5, "middle");
  g_assert_cmpstr (string->str, ==, "firstmiddlelast");
  xstring_free (string, TRUE);

  /* insert with pos == end of the string */
  string = xstring_new ("firstmiddle");
  xstring_insert (string, strlen ("firstmiddle"), "last");
  g_assert_cmpstr (string->str, ==, "firstmiddlelast");
  xstring_free (string, TRUE);

  /* insert_len */
  string = xstring_new ("firstlast");
  xstring_insert_len (string, 5, "middlejunkjunk", strlen ("middle"));
  g_assert_cmpstr (string->str, ==, "firstmiddlelast");
  xstring_free (string, TRUE);

  /* insert_len with magic -1 pos for append */
  string = xstring_new ("first");
  xstring_insert_len (string, -1, "lastjunkjunk", strlen ("last"));
  g_assert_cmpstr (string->str, ==, "firstlast");
  xstring_free (string, TRUE);

  /* insert_len with magic -1 len for strlen-the-string */
  string = xstring_new ("first");
  xstring_insert_len (string, 5, "last", -1);
  g_assert_cmpstr (string->str, ==, "firstlast");
  xstring_free (string, TRUE);

  /* insert_len with string overlap */
  string = xstring_new ("textbeforetextafter");
  xstring_insert_len (string, 10, string->str + 8, 5);
  g_assert_cmpstr (string->str, ==, "textbeforeretextextafter");
  xstring_free (string, TRUE);
}

static void
test_string_insert_unichar (void)
{
  xstring_t *string;

  /* insert_unichar with insertion in middle */
  string = xstring_new ("firsthalf");
  xstring_insert_unichar (string, 5, 0x0041);
  g_assert_cmpstr (string->str, ==, "first\x41half");
  xstring_free (string, TRUE);

  string = xstring_new ("firsthalf");
  xstring_insert_unichar (string, 5, 0x0298);
  g_assert_cmpstr (string->str, ==, "first\xCA\x98half");
  xstring_free (string, TRUE);

  string = xstring_new ("firsthalf");
  xstring_insert_unichar (string, 5, 0xFFFD);
  g_assert_cmpstr (string->str, ==, "first\xEF\xBF\xBDhalf");
  xstring_free (string, TRUE);

  string = xstring_new ("firsthalf");
  xstring_insert_unichar (string, 5, 0x1D100);
  g_assert_cmpstr (string->str, ==, "first\xF0\x9D\x84\x80half");
  xstring_free (string, TRUE);

  /* insert_unichar with insertion at end */
  string = xstring_new ("start");
  xstring_insert_unichar (string, -1, 0x0041);
  g_assert_cmpstr (string->str, ==, "start\x41");
  xstring_free (string, TRUE);

  string = xstring_new ("start");
  xstring_insert_unichar (string, -1, 0x0298);
  g_assert_cmpstr (string->str, ==, "start\xCA\x98");
  xstring_free (string, TRUE);

  string = xstring_new ("start");
  xstring_insert_unichar (string, -1, 0xFFFD);
  g_assert_cmpstr (string->str, ==, "start\xEF\xBF\xBD");
  xstring_free (string, TRUE);

  string = xstring_new ("start");
  xstring_insert_unichar (string, -1, 0x1D100);
  g_assert_cmpstr (string->str, ==, "start\xF0\x9D\x84\x80");
  xstring_free (string, TRUE);
}

static void
test_string_equal (void)
{
  xstring_t *string1, *string2;

  string1 = xstring_new ("test");
  string2 = xstring_new ("te");
  g_assert (!xstring_equal(string1, string2));
  xstring_append (string2, "st");
  g_assert (xstring_equal(string1, string2));
  xstring_free (string1, TRUE);
  xstring_free (string2, TRUE);
}

static void
test_string_truncate (void)
{
  xstring_t *string;

  string = xstring_new ("testing");

  xstring_truncate (string, 1000);
  g_assert (string->len == strlen("testing"));
  g_assert_cmpstr (string->str, ==, "testing");

  xstring_truncate (string, 4);
  g_assert (string->len == 4);
  g_assert_cmpstr (string->str, ==, "test");

  xstring_truncate (string, 0);
  g_assert (string->len == 0);
  g_assert_cmpstr (string->str, ==, "");

  xstring_free (string, TRUE);
}

static void
test_string_overwrite (void)
{
  xstring_t *string;

  /* overwriting functions */
  string = xstring_new ("testing");

  xstring_overwrite (string, 4, " and expand");
  g_assert (15 == string->len);
  g_assert ('\0' == string->str[15]);
  g_assert (xstr_equal ("test and expand", string->str));

  xstring_overwrite (string, 5, "NOT-");
  g_assert (15 == string->len);
  g_assert ('\0' == string->str[15]);
  g_assert (xstr_equal ("test NOT-expand", string->str));

  xstring_overwrite_len (string, 9, "blablabla", 6);
  g_assert (15 == string->len);
  g_assert ('\0' == string->str[15]);
  g_assert (xstr_equal ("test NOT-blabla", string->str));

  xstring_overwrite_len (string, 4, "BLABL", 0);
  g_assert (xstr_equal ("test NOT-blabla", string->str));
  xstring_overwrite_len (string, 4, "BLABL", -1);
  g_assert (xstr_equal ("testBLABLblabla", string->str));

  xstring_free (string, TRUE);
}

static void
test_string_nul_handling (void)
{
  xstring_t *string1, *string2;

  /* Check handling of embedded ASCII 0 (NUL) characters in xstring_t. */
  string1 = xstring_new ("fiddle");
  string2 = xstring_new ("fiddle");
  g_assert (xstring_equal (string1, string2));
  xstring_append_c (string1, '\0');
  g_assert (!xstring_equal (string1, string2));
  xstring_append_c (string2, '\0');
  g_assert (xstring_equal (string1, string2));
  xstring_append_c (string1, 'x');
  xstring_append_c (string2, 'y');
  g_assert (!xstring_equal (string1, string2));
  g_assert (string1->len == 8);
  xstring_append (string1, "yzzy");
  g_assert_cmpmem (string1->str, string1->len + 1, "fiddle\0xyzzy", 13);
  xstring_insert (string1, 1, "QED");
  g_assert_cmpmem (string1->str, string1->len + 1, "fQEDiddle\0xyzzy", 16);
  xstring_printf (string1, "fiddle%cxyzzy", '\0');
  g_assert_cmpmem (string1->str, string1->len + 1, "fiddle\0xyzzy", 13);

  xstring_free (string1, TRUE);
  xstring_free (string2, TRUE);
}

static void
test_string_up_down (void)
{
  xstring_t *s;

  s = xstring_new ("Mixed Case String !?");
  xstring_ascii_down (s);
  g_assert_cmpstr (s->str, ==, "mixed case string !?");

  xstring_assign (s, "Mixed Case String !?");
  xstring_down (s);
  g_assert_cmpstr (s->str, ==, "mixed case string !?");

  xstring_assign (s, "Mixed Case String !?");
  xstring_ascii_up (s);
  g_assert_cmpstr (s->str, ==, "MIXED CASE STRING !?");

  xstring_assign (s, "Mixed Case String !?");
  xstring_up (s);
  g_assert_cmpstr (s->str, ==, "MIXED CASE STRING !?");

  xstring_free (s, TRUE);
}

static void
test_string_set_size (void)
{
  xstring_t *s;

  s = xstring_new ("foo");
  xstring_set_size (s, 30);

  g_assert_cmpstr (s->str, ==, "foo");
  g_assert_cmpint (s->len, ==, 30);

  xstring_free (s, TRUE);
}

static void
test_string_to_bytes (void)
{
  xstring_t *s;
  xbytes_t *bytes;
  xconstpointer byte_data;
  xsize_t byte_len;

  s = xstring_new ("foo");
  xstring_append (s, "-bar");

  bytes = xstring_free_to_bytes (s);

  byte_data = xbytes_get_data (bytes, &byte_len);

  g_assert_cmpint (byte_len, ==, 7);

  g_assert_cmpmem (byte_data, byte_len, "foo-bar", 7);

  xbytes_unref (bytes);
}

static void
test_string_replace (void)
{
  static const struct
  {
    const char *string;
    const char *original;
    const char *replacement;
    xuint_t limit;
    const char *expected;
    xuint_t expected_n;
  }
  tests[] =
  {
    { "foo bar foo baz foo bar foobarbaz", "bar", "baz", 0,
      "foo baz foo baz foo baz foobazbaz", 3 },
    { "foo baz foo baz foo baz foobazbaz", "baz", "bar", 3,
      "foo bar foo bar foo bar foobazbaz", 3 },
    { "foo bar foo bar foo bar foobazbaz", "foobar", "bar", 1,
      "foo bar foo bar foo bar foobazbaz", 0 },
    { "aaaaaaaa", "a", "abcdefghijkl", 0,
      "abcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijklabcdefghijkl",
      8 },
    { "/usr/$LIB/libMangoHud.so", "$LIB", "lib32", 0,
      "/usr/lib32/libMangoHud.so", 1 },
    { "food for foals", "o", "", 0,
      "fd fr fals", 4 },
    { "aaa", "a", "aaa", 0,
      "aaaaaaaaa", 3 },
    { "aaa", "a", "", 0,
      "", 3 },
    { "aaa", "aa", "bb", 0,
      "bba", 1 },
    { "foo", "", "bar", 0,
      "barfbarobarobar", 4 },
    { "", "", "x", 0,
      "x", 1 },
    { "", "", "", 0,
      "", 1 },
  };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xstring_t *s;
      xuint_t n;

      s = xstring_new (tests[i].string);
      g_test_message ("%" G_GSIZE_FORMAT ": Replacing \"%s\" with \"%s\" (limit %u) in \"%s\"",
                      i, tests[i].original, tests[i].replacement,
                      tests[i].limit, tests[i].string);
      n = xstring_replace (s, tests[i].original, tests[i].replacement,
                            tests[i].limit);
      g_test_message ("-> %u replacements, \"%s\"",
                      n, s->str);
      g_assert_cmpstr (tests[i].expected, ==, s->str);
      g_assert_cmpuint (strlen (tests[i].expected), ==, s->len);
      g_assert_cmpuint (strlen (tests[i].expected) + 1, <=, s->allocated_len);
      g_assert_cmpuint (tests[i].expected_n, ==, n);
      xstring_free (s, TRUE);
    }
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/string/test-string-chunks", test_string_chunks);
  g_test_add_func ("/string/test-string-chunk-insert", test_string_chunk_insert);
  g_test_add_func ("/string/test-string-new", test_string_new);
  g_test_add_func ("/string/test-string-printf", test_string_printf);
  g_test_add_func ("/string/test-string-assign", test_string_assign);
  g_test_add_func ("/string/test-string-append-c", test_string_append_c);
  g_test_add_func ("/string/test-string-append", test_string_append);
  g_test_add_func ("/string/test-string-append-vprintf", test_string_append_vprintf);
  g_test_add_func ("/string/test-string-prepend-c", test_string_prepend_c);
  g_test_add_func ("/string/test-string-prepend", test_string_prepend);
  g_test_add_func ("/string/test-string-insert", test_string_insert);
  g_test_add_func ("/string/test-string-insert-unichar", test_string_insert_unichar);
  g_test_add_func ("/string/test-string-equal", test_string_equal);
  g_test_add_func ("/string/test-string-truncate", test_string_truncate);
  g_test_add_func ("/string/test-string-overwrite", test_string_overwrite);
  g_test_add_func ("/string/test-string-nul-handling", test_string_nul_handling);
  g_test_add_func ("/string/test-string-up-down", test_string_up_down);
  g_test_add_func ("/string/test-string-set-size", test_string_set_size);
  g_test_add_func ("/string/test-string-to-bytes", test_string_to_bytes);
  g_test_add_func ("/string/test-string-replace", test_string_replace);

  return g_test_run();
}
