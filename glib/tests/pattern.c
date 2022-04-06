/* XPL - Library of useful routines for C programming
 * Copyright (C) 2001 Matthias Clasen <matthiasc@poet.de>
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

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <string.h>
#include <glib.h>

/* keep enum and structure of gpattern.c and patterntest.c in sync */
typedef enum
{
  XMATCH_ALL,       /* "*A?A*" */
  XMATCH_ALL_TAIL,  /* "*A?AA" */
  XMATCH_HEAD,      /* "AAAA*" */
  XMATCH_TAIL,      /* "*AAAA" */
  XMATCH_EXACT,     /* "AAAAA" */
  XMATCH_LAST
} GMatchType;

struct _GPatternSpec
{
  GMatchType match_type;
  xuint_t      pattern_length;
  xuint_t      min_length;
  xuint_t      max_length;
  xchar_t     *pattern;
};

typedef struct _CompileTest CompileTest;

struct _CompileTest
{
  const xchar_t *src;
  GMatchType match_type;
  xchar_t *pattern;
  xuint_t min;
};

static CompileTest compile_tests[] = {
  { "*A?B*", XMATCH_ALL, "*A?B*", 3 },
  { "ABC*DEFGH", XMATCH_ALL_TAIL, "HGFED*CBA", 8 },
  { "ABCDEF*GH", XMATCH_ALL, "ABCDEF*GH", 8 },
  { "ABC**?***??**DEF*GH", XMATCH_ALL, "ABC*???DEF*GH", 11 },
  { "**ABC***?🤌DEF**", XMATCH_ALL, "*ABC*?🤌DEF*", 11 },
  { "*A?AA", XMATCH_ALL_TAIL, "AA?A*", 4 },
  { "ABCD*", XMATCH_HEAD, "ABCD", 4 },
  { "*ABCD", XMATCH_TAIL, "ABCD", 4 },
  { "ABCDE", XMATCH_EXACT, "ABCDE", 5 },
  { "A?C?E", XMATCH_ALL, "A?C?E", 5 },
  { "*?x", XMATCH_ALL_TAIL, "x?*", 2 },
  { "?*x", XMATCH_ALL_TAIL, "x?*", 2 },
  { "*?*x", XMATCH_ALL_TAIL, "x?*", 2 },
  { "x*??", XMATCH_ALL_TAIL, "??*x", 3 }
};

static void
test_compilation (xconstpointer d)
{
  const CompileTest *test = d;
  xpattern_spec_t *spec;

  spec = xpattern_spec_new (test->src);

  g_assert_cmpint (spec->match_type, ==, test->match_type);
  g_assert_cmpstr (spec->pattern, ==, test->pattern);
  g_assert_cmpint (spec->pattern_length, ==, strlen (spec->pattern));
  g_assert_cmpint (spec->min_length, ==, test->min);

  xpattern_spec_free (spec);
}

static void
test_copy (xconstpointer d)
{
  const CompileTest *test = d;
  xpattern_spec_t *p1, *p2;

  p1 = xpattern_spec_new (test->src);
  p2 = xpattern_spec_copy (p1);

  g_assert_cmpint (p2->match_type, ==, test->match_type);
  g_assert_cmpstr (p2->pattern, ==, test->pattern);
  g_assert_cmpint (p2->pattern_length, ==, strlen (p1->pattern));
  g_assert_cmpint (p2->min_length, ==, test->min);

  xpattern_spec_free (p1);
  xpattern_spec_free (p2);
}

typedef struct _MatchTest MatchTest;

struct _MatchTest
{
  const xchar_t *pattern;
  const xchar_t *string;
  xboolean_t match;
};

static MatchTest match_tests[] =
{
  { "*x", "x", TRUE },
  { "*x", "xx", TRUE },
  { "*x", "yyyx", TRUE },
  { "*x", "yyxy", FALSE },
  { "?x", "x", FALSE },
  { "?x", "xx", TRUE },
  { "?x", "yyyx", FALSE },
  { "?x", "yyxy", FALSE },
  { "*?x", "xx", TRUE },
  { "?*x", "xx", TRUE },
  { "*?x", "x", FALSE },
  { "?*x", "x", FALSE },
  { "*?*x", "yx", TRUE },
  { "*?*x", "xxxx", TRUE },
  { "x*??", "xyzw", TRUE },
  { "*x", "\xc3\x84x", TRUE },
  { "?x", "\xc3\x84x", TRUE },
  { "??x", "\xc3\x84x", FALSE },
  { "ab\xc3\xa4\xc3\xb6", "ab\xc3\xa4\xc3\xb6", TRUE },
  { "ab\xc3\xa4\xc3\xb6", "abao", FALSE },
  { "ab?\xc3\xb6", "ab\xc3\xa4\xc3\xb6", TRUE },
  { "ab?\xc3\xb6", "abao", FALSE },
  { "ab\xc3\xa4?", "ab\xc3\xa4\xc3\xb6", TRUE },
  { "ab\xc3\xa4?", "abao", FALSE },
  { "ab??", "ab\xc3\xa4\xc3\xb6", TRUE },
  { "ab*", "ab\xc3\xa4\xc3\xb6", TRUE },
  { "ab*\xc3\xb6", "ab\xc3\xa4\xc3\xb6", TRUE },
  { "ab*\xc3\xb6", "aba\xc3\xb6x\xc3\xb6", TRUE },
  { "", "abc", FALSE },
  { "", "", TRUE },
  { "abc", "abc", TRUE },
  { "*fo1*bar", "yyyfoxfo1bar", TRUE },
  { "12*fo1g*bar", "12yyyfoxfo1gbar", TRUE },
  { "__________:*fo1g*bar", "__________:yyyfoxfo1gbar", TRUE },
  { "*abc*cde", "abcde", FALSE },
  { "*abc*cde", "abccde", TRUE },
  { "*abc*cde", "abcxcde", TRUE },
  { "*abc*?cde", "abccde", FALSE },
  { "*abc*?cde", "abcxcde", TRUE },
  { "*abc*def", "abababcdededef", TRUE },
  { "*abc*def", "abcbcbcdededef", TRUE },
  { "*acbc*def", "acbcbcbcdededef", TRUE },
  { "*a?bc*def", "acbcbcbcdededef", TRUE },
  { "*abc*def", "bcbcbcdefdef", FALSE },
  { "*abc*def*ghi", "abcbcbcbcbcbcdefefdefdefghi", TRUE },
  { "*abc*def*ghi", "bcbcbcbcbcbcdefdefdefdefghi", FALSE },
  { "_1_2_3_4_5_6_7_8_9_0_1_2_3_4_5_*abc*def*ghi", "_1_2_3_4_5_6_7_8_9_0_1_2_3_4_5_abcbcbcbcbcbcdefefdefdefghi", TRUE },
  { "fooooooo*a*bc", "fooooooo_a_bd_a_bc", TRUE },
  { "x*?", "x", FALSE },
  { "abc*", "abc", TRUE },
  { "*", "abc", TRUE }
};

static void
test_match (xconstpointer d)
{
  const MatchTest *test = d;
  xpattern_spec_t *p;
  xchar_t *r;

  g_assert_cmpint (g_pattern_match_simple (test->pattern, test->string), ==, test->match);

  p = xpattern_spec_new (test->pattern);
  g_assert_cmpint (xpattern_spec_match_string (p, test->string), ==, test->match);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint (g_pattern_match_string (p, test->string), ==, test->match);
  G_GNUC_END_IGNORE_DEPRECATIONS

  r = xutf8_strreverse (test->string, -1);
  g_assert_cmpint (xpattern_spec_match (p, strlen (test->string), test->string, r), ==, test->match);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint (g_pattern_match (p, strlen (test->string), test->string, r), ==, test->match);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_free (r);

  xpattern_spec_free (p);
}

typedef struct _EqualTest EqualTest;

struct _EqualTest
{
  const xchar_t *pattern1;
  const xchar_t *pattern2;
  xboolean_t expected;
};

static EqualTest equal_tests[] =
{
  { "*A?B*", "*A?B*", TRUE },
  { "A*BCD", "A*BCD", TRUE },
  { "ABCD*", "ABCD****", TRUE },
  { "A1*", "A1*", TRUE },
  { "*YZ", "*YZ", TRUE },
  { "A1x", "A1x", TRUE },
  { "AB*CD", "AB**CD", TRUE },
  { "AB*?*CD", "AB*?CD", TRUE },
  { "AB*?CD", "AB?*CD", TRUE },
  { "AB*CD", "AB*?*CD", FALSE },
  { "ABC*", "ABC?", FALSE },
};

static void
test_equal (xconstpointer d)
{
  const EqualTest *test = d;
  xpattern_spec_t *p1, *p2;

  p1 = xpattern_spec_new (test->pattern1);
  p2 = xpattern_spec_new (test->pattern2);

  g_assert_cmpint (xpattern_spec_equal (p1, p2), ==, test->expected);

  xpattern_spec_free (p1);
  xpattern_spec_free (p2);
}


int
main (int argc, char** argv)
{
  xsize_t i;
  xchar_t *path;

  g_test_init (&argc, &argv, NULL);

  for (i = 0; i < G_N_ELEMENTS (compile_tests); i++)
    {
      path = xstrdup_printf ("/pattern/compile/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &compile_tests[i], test_compilation);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (compile_tests); i++)
    {
      path = xstrdup_printf ("/pattern/copy/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &compile_tests[i], test_copy);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (match_tests); i++)
    {
      path = xstrdup_printf ("/pattern/match/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &match_tests[i], test_match);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (equal_tests); i++)
    {
      path = xstrdup_printf ("/pattern/equal/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &equal_tests[i], test_equal);
      g_free (path);
    }

  return g_test_run ();
}
