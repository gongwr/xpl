/* Unit tests for utilities
 * Copyright (C) 2010 Red Hat, Inc.
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
 *
 * Author: Matthias Clasen
 */

#include "glib.h"

static void
test_utf8_strlen (void)
{
  const xchar_t *string = "\xe2\x82\xa0gh\xe2\x82\xa4jl";

  g_assert_cmpint (xutf8_strlen (string, -1), ==, 6);
  g_assert_cmpint (xutf8_strlen (string, 0), ==, 0);
  g_assert_cmpint (xutf8_strlen (string, 1), ==, 0);
  g_assert_cmpint (xutf8_strlen (string, 2), ==, 0);
  g_assert_cmpint (xutf8_strlen (string, 3), ==, 1);
  g_assert_cmpint (xutf8_strlen (string, 4), ==, 2);
  g_assert_cmpint (xutf8_strlen (string, 5), ==, 3);
  g_assert_cmpint (xutf8_strlen (string, 6), ==, 3);
  g_assert_cmpint (xutf8_strlen (string, 7), ==, 3);
  g_assert_cmpint (xutf8_strlen (string, 8), ==, 4);
  g_assert_cmpint (xutf8_strlen (string, 9), ==, 5);
  g_assert_cmpint (xutf8_strlen (string, 10), ==, 6);
}

static void
test_utf8_strncpy (void)
{
  const xchar_t *string = "\xe2\x82\xa0gh\xe2\x82\xa4jl";
  xchar_t dest[20];

  xutf8_strncpy (dest, string, 0);
  g_assert_cmpstr (dest, ==, "");

  xutf8_strncpy (dest, string, 1);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0");

  xutf8_strncpy (dest, string, 2);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0g");

  xutf8_strncpy (dest, string, 3);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh");

  xutf8_strncpy (dest, string, 4);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4");

  xutf8_strncpy (dest, string, 5);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4j");

  xutf8_strncpy (dest, string, 6);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4jl");

  xutf8_strncpy (dest, string, 20);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4jl");
}

static void
test_utf8_strrchr (void)
{
  const xchar_t *string = "\xe2\x82\xa0gh\xe2\x82\xa4jl\xe2\x82\xa4jl";

  xassert (xutf8_strrchr (string, -1, 'j') == string + 13);
  xassert (xutf8_strrchr (string, -1, 8356) == string + 10);
  xassert (xutf8_strrchr (string, 9, 8356) == string + 5);
  xassert (xutf8_strrchr (string, 3, 'j') == NULL);
  xassert (xutf8_strrchr (string, -1, 'x') == NULL);
}

static void
test_utf8_reverse (void)
{
  xchar_t *r;

  r = xutf8_strreverse ("abcdef", -1);
  g_assert_cmpstr (r, ==, "fedcba");
  g_free (r);

  r = xutf8_strreverse ("abcdef", 4);
  g_assert_cmpstr (r, ==, "dcba");
  g_free (r);

  /* U+0B0B Oriya Letter Vocalic R
   * U+10900 Phoenician Letter Alf
   * U+0041 Latin Capital Letter A
   * U+1EB6 Latin Capital Letter A With Breve And Dot Below
   */
  r = xutf8_strreverse ("\340\254\213\360\220\244\200\101\341\272\266", -1);
  g_assert_cmpstr (r, ==, "\341\272\266\101\360\220\244\200\340\254\213");
  g_free (r);
}

static void
test_utf8_substring (void)
{
  xchar_t *r;

  r = xutf8_substring ("abcd", 1, 3);
  g_assert_cmpstr (r, ==, "bc");
  g_free (r);

  r = xutf8_substring ("abcd", 0, 4);
  g_assert_cmpstr (r, ==, "abcd");
  g_free (r);

  r = xutf8_substring ("abcd", 2, 2);
  g_assert_cmpstr (r, ==, "");
  g_free (r);

  r = xutf8_substring ("abc\xe2\x82\xa0gh\xe2\x82\xa4", 2, 5);
  g_assert_cmpstr (r, ==, "c\xe2\x82\xa0g");
  g_free (r);

  r = xutf8_substring ("abcd", 1, -1);
  g_assert_cmpstr (r, ==, "bcd");
  g_free (r);
}

static void
test_utf8_make_valid (void)
{
  xchar_t *r;

  /* valid UTF8 */
  r = xutf8_make_valid ("\xe2\x82\xa0gh\xe2\x82\xa4jl", -1);
  g_assert_cmpstr (r, ==, "\xe2\x82\xa0gh\xe2\x82\xa4jl");
  g_free (r);

  /* invalid UTF8 */
  r = xutf8_make_valid ("\xe2\x82\xa0gh\xe2\xffjl", -1);
  g_assert_cmpstr (r, ==, "\xe2\x82\xa0gh\xef\xbf\xbd\xef\xbf\xbdjl");
  g_free (r);

  /* invalid UTF8 without nul terminator followed by something unfortunate */
  r = xutf8_make_valid ("Bj\xc3\xb8", 3);
  g_assert_cmpstr (r, ==, "Bj\xef\xbf\xbd");
  g_free (r);

  /* invalid UTF8 with embedded nul */
  r = xutf8_make_valid ("\xe2\x82\xa0gh\xe2\x00jl", 9);
  g_assert_cmpstr (r, ==, "\xe2\x82\xa0gh\xef\xbf\xbd\xef\xbf\xbdjl");
  g_free (r);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/utf8/strlen", test_utf8_strlen);
  g_test_add_func ("/utf8/strncpy", test_utf8_strncpy);
  g_test_add_func ("/utf8/strrchr", test_utf8_strrchr);
  g_test_add_func ("/utf8/reverse", test_utf8_reverse);
  g_test_add_func ("/utf8/substring", test_utf8_substring);
  g_test_add_func ("/utf8/make-valid", test_utf8_make_valid);

  return g_test_run();
}
