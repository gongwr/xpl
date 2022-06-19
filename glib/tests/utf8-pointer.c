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

#include <string.h>
#include <glib.h>

/* test_t conversions between offsets and pointers */

static void test_utf8 (xconstpointer d)
{
  xint_t num_chars;
  const xchar_t **p;
  xint_t i, j;
  const xchar_t *string = d;

  xassert (xutf8_validate (string, -1, NULL));

  num_chars = xutf8_strlen (string, -1);

  p = (const xchar_t **) g_malloc (num_chars * sizeof (xchar_t *));

  p[0] = string;
  for (i = 1; i < num_chars; i++)
    p[i] = xutf8_next_char (p[i-1]);

  for (i = 0; i < num_chars; i++)
    for (j = 0; j < num_chars; j++)
      {
        xassert (xutf8_offset_to_pointer (p[i], j - i) == p[j]);
        xassert (xutf8_pointer_to_offset (p[i], p[j]) == j - i);
      }

  g_free (p);
}

xchar_t *longline = "asdasdas dsaf asfd as fdasdf asfd asdf as dfas dfasdf a"
"asd fasdf asdf asdf asd fasfd as fdasfd asdf as fdççççççççças ffsd asfd as fdASASASAs As"
"Asfdsf sdfg sdfg dsfg dfg sdfgsdfgsdfgsdfg sdfgsdfg sdfg sdfg sdf gsdfg sdfg sd"
"asd fasdf asdf asdf asd fasfd as fdaèèèèèèè òòòòòòòòòòòòsfd asdf as fdas ffsd asfd as fdASASASAs D"
"Asfdsf sdfg sdfg dsfg dfg sdfgsdfgsdfgsdfg sdfgsdfg sdfgùùùùùùùùùùùùùù sdfg sdf gsdfg sdfg sd"
"asd fasdf asdf asdf asd fasfd as fdasfd asd@@@@@@@f as fdas ffsd asfd as fdASASASAs D "
"Asfdsf sdfg sdfg dsfg dfg sdfgsdfgsdfgsdfg sdfgsdf€€€€€€€€€€€€€€€€€€g sdfg sdfg sdf gsdfg sdfg sd"
"asd fasdf asdf asdf asd fasfd as fdasfd asdf as fdas ffsd asfd as fdASASASAs D"
"Asfdsf sdfg sdfg dsfg dfg sdfgsdfgsdfgsdfg sdfgsdfg sdfg sdfg sdf gsdfg sdfg sd\n\nlalala\n";

static void
test_length (void)
{
  xassert (xutf8_strlen ("1234", -1) == 4);
  xassert (xutf8_strlen ("1234", 0) == 0);
  xassert (xutf8_strlen ("1234", 1) == 1);
  xassert (xutf8_strlen ("1234", 2) == 2);
  xassert (xutf8_strlen ("1234", 3) == 3);
  xassert (xutf8_strlen ("1234", 4) == 4);
  xassert (xutf8_strlen ("1234", 5) == 4);

  xassert (xutf8_strlen (longline, -1) == 762);
  xassert (xutf8_strlen (longline, strlen (longline)) == 762);
  xassert (xutf8_strlen (longline, 1024) == 762);

  xassert (xutf8_strlen (NULL, 0) == 0);

  xassert (xutf8_strlen ("a\340\250\201c", -1) == 3);
  xassert (xutf8_strlen ("a\340\250\201c", 1) == 1);
  xassert (xutf8_strlen ("a\340\250\201c", 2) == 1);
  xassert (xutf8_strlen ("a\340\250\201c", 3) == 1);
  xassert (xutf8_strlen ("a\340\250\201c", 4) == 2);
  xassert (xutf8_strlen ("a\340\250\201c", 5) == 3);
}

static void
test_find (void)
{
  /* U+0B0B Oriya Letter Vocalic R (\340\254\213)
   * U+10900 Phoenician Letter Alf (\360\220\244\200)
   * U+0041 Latin Capital Letter A (\101)
   * U+1EB6 Latin Capital Letter A With Breve And Dot Below (\341\272\266)
   */
#define TEST_STR "\340\254\213\360\220\244\200\101\341\272\266\0\101"
  const xsize_t str_size = sizeof TEST_STR;
  const xchar_t *str = TEST_STR;
  const xchar_t str_array[] = TEST_STR;
  const xchar_t * volatile str_volatile = TEST_STR;
#undef TEST_STR
  xchar_t *str_copy = g_malloc (str_size);
  const xchar_t *p;
  const xchar_t *q;
  memcpy (str_copy, str, str_size);

#define TEST_SET(STR)  \
  G_STMT_START { \
    p = STR + (str_size - 1); \
    \
    q = xutf8_find_prev_char (STR, p); \
    xassert (q == STR + 12); \
    q = xutf8_find_prev_char (STR, q); \
    xassert (q == STR + 11); \
    q = xutf8_find_prev_char (STR, q); \
    xassert (q == STR + 8); \
    q = xutf8_find_prev_char (STR, q); \
    xassert (q == STR + 7); \
    q = xutf8_find_prev_char (STR, q); \
    xassert (q == STR + 3); \
    q = xutf8_find_prev_char (STR, q); \
    xassert (q == STR); \
    q = xutf8_find_prev_char (STR, q); \
    g_assert_null (q); \
    \
    p = STR + 4; \
    q = xutf8_find_prev_char (STR, p); \
    xassert (q == STR + 3); \
    \
    p = STR + 2; \
    q = xutf8_find_prev_char (STR, p); \
    xassert (q == STR); \
    \
    p = STR + 2; \
    q = xutf8_find_next_char (p, NULL); \
    xassert (q == STR + 3); \
    q = xutf8_find_next_char (q, NULL); \
    xassert (q == STR + 7); \
    \
    q = xutf8_find_next_char (p, STR + 6); \
    xassert (q == STR + 3); \
    q = xutf8_find_next_char (q, STR + 6); \
    g_assert_null (q); \
    \
    q = xutf8_find_next_char (STR, STR); \
    g_assert_null (q); \
    \
    q = xutf8_find_next_char (STR + strlen (STR), NULL); \
    xassert (q == STR + strlen (STR) + 1); \
    \
    /* Check return values when reaching the end of the string, \
     * with @end set and unset. */ \
    q = xutf8_find_next_char (STR + 10, NULL); \
    g_assert_nonnull (q); \
    xassert (*q == '\0'); \
    \
    q = xutf8_find_next_char (STR + 10, STR + 11); \
    g_assert_null (q); \
  } G_STMT_END

  TEST_SET(str_array);
  TEST_SET(str_copy);
  TEST_SET(str_volatile);
  /* Starting with GCC 8 tests on @str with "-O2 -flto" in CFLAGS fail due to
   * (incorrect?) constant propagation of @str into @xutf8_find_prev_char. It
   * doesn't happen if @TEST_STR doesn't contain \0 in the middle but the tests
   * should cover all corner cases.
   * For instance, see https://gitlab.gnome.org/GNOME/glib/issues/1917 */

#undef TEST_SET

  g_free (str_copy);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/utf8/offsets", longline, test_utf8);
  g_test_add_func ("/utf8/lengths", test_length);
  g_test_add_func ("/utf8/find", test_find);

  return g_test_run ();
}
