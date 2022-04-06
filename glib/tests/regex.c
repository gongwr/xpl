/*
 * Copyright (C) 2005 - 2006, Marco Barisione <marco@barisione.org>
 * Copyright (C) 2010 Red Hat, Inc.
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

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include "config.h"

#include <string.h>
#include <locale.h>
#include "glib.h"

#include <pcre.h>

/* U+20AC EURO SIGN (symbol, currency) */
#define EURO "\xe2\x82\xac"
/* U+00E0 LATIN SMALL LETTER A WITH GRAVE (letter, lowercase) */
#define AGRAVE "\xc3\xa0"
/* U+00C0 LATIN CAPITAL LETTER A WITH GRAVE (letter, uppercase) */
#define AGRAVE_UPPER "\xc3\x80"
/* U+00E8 LATIN SMALL LETTER E WITH GRAVE (letter, lowercase) */
#define EGRAVE "\xc3\xa8"
/* U+00F2 LATIN SMALL LETTER O WITH GRAVE (letter, lowercase) */
#define OGRAVE "\xc3\xb2"
/* U+014B LATIN SMALL LETTER ENG (letter, lowercase) */
#define ENG "\xc5\x8b"
/* U+0127 LATIN SMALL LETTER H WITH STROKE (letter, lowercase) */
#define HSTROKE "\xc4\xa7"
/* U+0634 ARABIC LETTER SHEEN (letter, other) */
#define SHEEN "\xd8\xb4"
/* U+1374 ETHIOPIC NUMBER THIRTY (number, other) */
#define ETH30 "\xe1\x8d\xb4"

/* A random value use to mark untouched integer variables. */
#define UNTOUCHED -559038737

static xint_t total;

typedef struct {
  const xchar_t *pattern;
  xregex_compile_flags_t compile_opts;
  xregex_match_flags_t   match_opts;
  xint_t expected_error;
  xboolean_t check_flags;
  xregex_compile_flags_t real_compile_opts;
  xregex_match_flags_t real_match_opts;
} TestNewData;

static void
test_new (xconstpointer d)
{
  const TestNewData *data = d;
  xregex_t *regex;
  xerror_t *error = NULL;

  regex = xregex_new (data->pattern, data->compile_opts, data->match_opts, &error);
  g_assert (regex != NULL);
  g_assert_no_error (error);
  g_assert_cmpstr (data->pattern, ==, xregex_get_pattern (regex));

  if (data->check_flags)
    {
      g_assert_cmphex (xregex_get_compile_flags (regex), ==, data->real_compile_opts);
      g_assert_cmphex (xregex_get_match_flags (regex), ==, data->real_match_opts);
    }

  xregex_unref (regex);
}

#define TEST_NEW(_pattern, _compile_opts, _match_opts) {    \
  TestNewData *data;                                        \
  xchar_t *path;                                              \
  data = g_new0 (TestNewData, 1);                           \
  data->pattern = _pattern;                                 \
  data->compile_opts = _compile_opts;                       \
  data->match_opts = _match_opts;                           \
  data->expected_error = 0;                                 \
  data->check_flags = FALSE;                                \
  path = xstrdup_printf ("/regex/new/%d", ++total);        \
  g_test_add_data_func_full (path, data, test_new, g_free); \
  g_free (path);                                            \
}

#define TEST_NEW_CHECK_FLAGS(_pattern, _compile_opts, _match_opts, _real_compile_opts, _real_match_opts) { \
  TestNewData *data;                                             \
  xchar_t *path;                                                   \
  data = g_new0 (TestNewData, 1);                                \
  data->pattern = _pattern;                                      \
  data->compile_opts = _compile_opts;                            \
  data->match_opts = 0;                                          \
  data->expected_error = 0;                                      \
  data->check_flags = TRUE;                                      \
  data->real_compile_opts = _real_compile_opts;                  \
  data->real_match_opts = _real_match_opts;                      \
  path = xstrdup_printf ("/regex/new-check-flags/%d", ++total); \
  g_test_add_data_func_full (path, data, test_new, g_free);      \
  g_free (path);                                                 \
}

static void
test_new_fail (xconstpointer d)
{
  const TestNewData *data = d;
  xregex_t *regex;
  xerror_t *error = NULL;

  regex = xregex_new (data->pattern, data->compile_opts, data->match_opts, &error);

  g_assert (regex == NULL);
  g_assert_error (error, XREGEX_ERROR, data->expected_error);
  xerror_free (error);
}

#define TEST_NEW_FAIL(_pattern, _compile_opts, _expected_error) { \
  TestNewData *data;                                              \
  xchar_t *path;                                                    \
  data = g_new0 (TestNewData, 1);                                 \
  data->pattern = _pattern;                                       \
  data->compile_opts = _compile_opts;                             \
  data->match_opts = 0;                                           \
  data->expected_error = _expected_error;                         \
  path = xstrdup_printf ("/regex/new-fail/%d", ++total);         \
  g_test_add_data_func_full (path, data, test_new_fail, g_free);  \
  g_free (path);                                                  \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xregex_compile_flags_t compile_opts;
  xregex_match_flags_t match_opts;
  xboolean_t expected;
  xssize_t string_len;
  xint_t start_position;
  xregex_match_flags_t match_opts2;
} TestMatchData;

static void
test_match_simple (xconstpointer d)
{
  const TestMatchData *data = d;
  xboolean_t match;

  match = xregex_match_simple (data->pattern, data->string, data->compile_opts, data->match_opts);
  g_assert_cmpint (match, ==, data->expected);
}

#define TEST_MATCH_SIMPLE_NAMED(_name, _pattern, _string, _compile_opts, _match_opts, _expected) { \
  TestMatchData *data;                                                  \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchData, 1);                                     \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->compile_opts = _compile_opts;                                   \
  data->match_opts = _match_opts;                                       \
  data->expected = _expected;                                           \
  path = xstrdup_printf ("/regex/match-%s/%d", _name, ++total);        \
  g_test_add_data_func_full (path, data, test_match_simple, g_free);    \
  g_free (path);                                                        \
}

#define TEST_MATCH_SIMPLE(_pattern, _string, _compile_opts, _match_opts, _expected) \
  TEST_MATCH_SIMPLE_NAMED("simple", _pattern, _string, _compile_opts, _match_opts, _expected)
#define TEST_MATCH_NOTEMPTY(_pattern, _string, _expected) \
  TEST_MATCH_SIMPLE_NAMED("notempty", _pattern, _string, 0, XREGEX_MATCH_NOTEMPTY, _expected)
#define TEST_MATCH_NOTEMPTY_ATSTART(_pattern, _string, _expected) \
  TEST_MATCH_SIMPLE_NAMED("notempty-atstart", _pattern, _string, 0, XREGEX_MATCH_NOTEMPTY_ATSTART, _expected)

static void
test_match (xconstpointer d)
{
  const TestMatchData *data = d;
  xregex_t *regex;
  xboolean_t match;
  xerror_t *error = NULL;

  regex = xregex_new (data->pattern, data->compile_opts, data->match_opts, &error);
  g_assert (regex != NULL);
  g_assert_no_error (error);

  match = xregex_match_full (regex, data->string, data->string_len,
                              data->start_position, data->match_opts2, NULL, NULL);

  if (data->expected)
    {
      if (!match)
        xerror ("Regex '%s' (with compile options %u and "
            "match options %u) should have matched '%.*s' "
            "(of length %d, at position %d, with match options %u) but did not",
            data->pattern, data->compile_opts, data->match_opts,
            data->string_len == -1 ? (int) strlen (data->string) :
              (int) data->string_len,
            data->string, (int) data->string_len,
            data->start_position, data->match_opts2);

      g_assert_cmpint (match, ==, TRUE);
    }
  else
    {
      if (match)
        xerror ("Regex '%s' (with compile options %u and "
            "match options %u) should not have matched '%.*s' "
            "(of length %d, at position %d, with match options %u) but did",
            data->pattern, data->compile_opts, data->match_opts,
            data->string_len == -1 ? (int) strlen (data->string) :
              (int) data->string_len,
            data->string, (int) data->string_len,
            data->start_position, data->match_opts2);
    }

  if (data->string_len == -1 && data->start_position == 0)
    {
      match = xregex_match (regex, data->string, data->match_opts2, NULL);
      g_assert_cmpint (match, ==, data->expected);
    }

  xregex_unref (regex);
}

#define TEST_MATCH(_pattern, _compile_opts, _match_opts, _string, \
                   _string_len, _start_position, _match_opts2, _expected) { \
  TestMatchData *data;                                                  \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchData, 1);                                     \
  data->pattern = _pattern;                                             \
  data->compile_opts = _compile_opts;                                   \
  data->match_opts = _match_opts;                                       \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  data->match_opts2 = _match_opts2;                                     \
  data->expected = _expected;                                           \
  path = xstrdup_printf ("/regex/match/%d", ++total);                  \
  g_test_add_data_func_full (path, data, test_match, g_free);           \
  g_free (path);                                                        \
}

struct _Match
{
  xchar_t *string;
  xint_t start, end;
};
typedef struct _Match Match;

static void
free_match (xpointer_t data)
{
  Match *match = data;
  if (match == NULL)
    return;
  g_free (match->string);
  g_free (match);
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xssize_t string_len;
  xint_t start_position;
  xslist_t *expected;
} TestMatchNextData;

static void
test_match_next (xconstpointer d)
{
   const TestMatchNextData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xslist_t *matches;
  xslist_t *l_exp, *l_match;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  xregex_match_full (regex, data->string, data->string_len,
                      data->start_position, 0, &match_info, NULL);
  matches = NULL;
  while (xmatch_info_matches (match_info))
    {
      Match *match = g_new0 (Match, 1);
      match->string = xmatch_info_fetch (match_info, 0);
      match->start = UNTOUCHED;
      match->end = UNTOUCHED;
      xmatch_info_fetch_pos (match_info, 0, &match->start, &match->end);
      matches = xslist_prepend (matches, match);
      xmatch_info_next (match_info, NULL);
    }
  g_assert (regex == xmatch_info_get_regex (match_info));
  g_assert_cmpstr (data->string, ==, xmatch_info_get_string (match_info));
  xmatch_info_free (match_info);
  matches = xslist_reverse (matches);

  g_assert_cmpint (xslist_length (matches), ==, xslist_length (data->expected));

  l_exp = data->expected;
  l_match = matches;
  while (l_exp != NULL)
    {
      Match *exp = l_exp->data;
      Match *match = l_match->data;

      g_assert_cmpstr (exp->string, ==, match->string);
      g_assert_cmpint (exp->start, ==, match->start);
      g_assert_cmpint (exp->end, ==, match->end);

      l_exp = xslist_next (l_exp);
      l_match = xslist_next (l_match);
    }

  xregex_unref (regex);
  xslist_free_full (matches, free_match);
}

static void
free_match_next_data (xpointer_t _data)
{
  TestMatchNextData *data = _data;

  xslist_free_full (data->expected, g_free);
  g_free (data);
}

#define TEST_MATCH_NEXT0(_pattern, _string, _string_len, _start_position) { \
  TestMatchNextData *data;                                              \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  data->expected = NULL;                                                \
  path = xstrdup_printf ("/regex/match/next0/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT1(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1) {                                  \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = xslist_append (NULL, match);                        \
  path = xstrdup_printf ("/regex/match/next1/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT2(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1, t2, s2, e2) {                      \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = xslist_append (NULL, match);                        \
  match = g_new0 (Match, 1);                                            \
  match->string = t2;                                                   \
  match->start = s2;                                                    \
  match->end = e2;                                                      \
  data->expected = xslist_append (data->expected, match);              \
  path = xstrdup_printf ("/regex/match/next2/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT3(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1, t2, s2, e2, t3, s3, e3) {          \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = xslist_append (NULL, match);                        \
  match = g_new0 (Match, 1);                                            \
  match->string = t2;                                                   \
  match->start = s2;                                                    \
  match->end = e2;                                                      \
  data->expected = xslist_append (data->expected, match);              \
  match = g_new0 (Match, 1);                                            \
  match->string = t3;                                                   \
  match->start = s3;                                                    \
  match->end = e3;                                                      \
  data->expected = xslist_append (data->expected, match);              \
  path = xstrdup_printf ("/regex/match/next3/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

#define TEST_MATCH_NEXT4(_pattern, _string, _string_len, _start_position,   \
                         t1, s1, e1, t2, s2, e2, t3, s3, e3, t4, s4, e4) { \
  TestMatchNextData *data;                                              \
  Match *match;                                                         \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchNextData, 1);                                 \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->string_len = _string_len;                                       \
  data->start_position = _start_position;                               \
  match = g_new0 (Match, 1);                                            \
  match->string = t1;                                                   \
  match->start = s1;                                                    \
  match->end = e1;                                                      \
  data->expected = xslist_append (NULL, match);                        \
  match = g_new0 (Match, 1);                                            \
  match->string = t2;                                                   \
  match->start = s2;                                                    \
  match->end = e2;                                                      \
  data->expected = xslist_append (data->expected, match);              \
  match = g_new0 (Match, 1);                                            \
  match->string = t3;                                                   \
  match->start = s3;                                                    \
  match->end = e3;                                                      \
  data->expected = xslist_append (data->expected, match);              \
  match = g_new0 (Match, 1);                                            \
  match->string = t4;                                                   \
  match->start = s4;                                                    \
  match->end = e4;                                                      \
  data->expected = xslist_append (data->expected, match);              \
  path = xstrdup_printf ("/regex/match/next4/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_next, free_match_next_data); \
  g_free (path);                                                        \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xint_t start_position;
  xregex_match_flags_t match_opts;
  xint_t expected_count;
} TestMatchCountData;

static void
test_match_count (xconstpointer d)
{
  const TestMatchCountData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xint_t count;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  xregex_match_full (regex, data->string, -1, data->start_position,
		      data->match_opts, &match_info, NULL);
  count = xmatch_info_get_match_count (match_info);

  g_assert_cmpint (count, ==, data->expected_count);

  xmatch_info_ref (match_info);
  xmatch_info_unref (match_info);
  xmatch_info_unref (match_info);
  xregex_unref (regex);
}

#define TEST_MATCH_COUNT(_pattern, _string, _start_position, _match_opts, _expected_count) { \
  TestMatchCountData *data;                                             \
  xchar_t *path;                                                          \
  data = g_new0 (TestMatchCountData, 1);                                \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->match_opts = _match_opts;                                       \
  data->expected_count = _expected_count;                               \
  path = xstrdup_printf ("/regex/match/count/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_match_count, g_free);     \
  g_free (path);                                                        \
}

static void
test_partial (xconstpointer d)
{
  const TestMatchData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  xregex_match (regex, data->string, data->match_opts, &match_info);

  g_assert_cmpint (data->expected, ==, xmatch_info_is_partial_match (match_info));

  if (data->expected)
    {
      g_assert (!xmatch_info_fetch_pos (match_info, 0, NULL, NULL));
      g_assert (!xmatch_info_fetch_pos (match_info, 1, NULL, NULL));
    }

  xmatch_info_free (match_info);
  xregex_unref (regex);
}

#define TEST_PARTIAL_FULL(_pattern, _string, _match_opts, _expected) { \
  TestMatchData *data;                                          \
  xchar_t *path;                                                  \
  data = g_new0 (TestMatchData, 1);                             \
  data->pattern = _pattern;                                     \
  data->string = _string;                                       \
  data->match_opts = _match_opts;                               \
  data->expected = _expected;                                   \
  path = xstrdup_printf ("/regex/match/partial/%d", ++total);  \
  g_test_add_data_func_full (path, data, test_partial, g_free); \
  g_free (path);                                                \
}

#define TEST_PARTIAL(_pattern, _string, _expected) TEST_PARTIAL_FULL(_pattern, _string, XREGEX_MATCH_PARTIAL, _expected)

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xint_t         start_position;
  xint_t         sub_n;
  const xchar_t *expected_sub;
  xint_t         expected_start;
  xint_t         expected_end;
} TestSubData;

static void
test_sub_pattern (xconstpointer d)
{
  const TestSubData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xchar_t *sub_expr;
  xint_t start = UNTOUCHED, end = UNTOUCHED;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  xregex_match_full (regex, data->string, -1, data->start_position, 0, &match_info, NULL);

  sub_expr = xmatch_info_fetch (match_info, data->sub_n);
  g_assert_cmpstr (sub_expr, ==, data->expected_sub);
  g_free (sub_expr);

  xmatch_info_fetch_pos (match_info, data->sub_n, &start, &end);
  g_assert_cmpint (start, ==, data->expected_start);
  g_assert_cmpint (end, ==, data->expected_end);

  xmatch_info_free (match_info);
  xregex_unref (regex);
}

#define TEST_SUB_PATTERN(_pattern, _string, _start_position, _sub_n, _expected_sub, \
			 _expected_start, _expected_end) {                       \
  TestSubData *data;                                                    \
  xchar_t *path;                                                          \
  data = g_new0 (TestSubData, 1);                                       \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->sub_n = _sub_n;                                                 \
  data->expected_sub = _expected_sub;                                   \
  data->expected_start = _expected_start;                               \
  data->expected_end = _expected_end;                                   \
  path = xstrdup_printf ("/regex/match/subpattern/%d", ++total);       \
  g_test_add_data_func_full (path, data, test_sub_pattern, g_free);     \
  g_free (path);                                                        \
}

typedef struct {
  const xchar_t *pattern;
  xregex_compile_flags_t flags;
  const xchar_t *string;
  xint_t         start_position;
  const xchar_t *sub_name;
  const xchar_t *expected_sub;
  xint_t         expected_start;
  xint_t         expected_end;
} TestNamedSubData;

static void
test_named_sub_pattern (xconstpointer d)
{
  const TestNamedSubData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xint_t start = UNTOUCHED, end = UNTOUCHED;
  xchar_t *sub_expr;

  regex = xregex_new (data->pattern, data->flags, 0, NULL);

  g_assert (regex != NULL);

  xregex_match_full (regex, data->string, -1, data->start_position, 0, &match_info, NULL);
  sub_expr = xmatch_info_fetch_named (match_info, data->sub_name);
  g_assert_cmpstr (sub_expr, ==, data->expected_sub);
  g_free (sub_expr);

  xmatch_info_fetch_named_pos (match_info, data->sub_name, &start, &end);
  g_assert_cmpint (start, ==, data->expected_start);
  g_assert_cmpint (end, ==, data->expected_end);

  xmatch_info_free (match_info);
  xregex_unref (regex);
}

#define TEST_NAMED_SUB_PATTERN(_pattern, _string, _start_position, _sub_name, \
			       _expected_sub, _expected_start, _expected_end) { \
  TestNamedSubData *data;                                                 \
  xchar_t *path;                                                            \
  data = g_new0 (TestNamedSubData, 1);                                    \
  data->pattern = _pattern;                                               \
  data->string = _string;                                                 \
  data->flags = 0;                                                        \
  data->start_position = _start_position;                                 \
  data->sub_name = _sub_name;                                             \
  data->expected_sub = _expected_sub;                                     \
  data->expected_start = _expected_start;                                 \
  data->expected_end = _expected_end;                                     \
  path = xstrdup_printf ("/regex/match/named/subpattern/%d", ++total);   \
  g_test_add_data_func_full (path, data, test_named_sub_pattern, g_free); \
  g_free (path);                                                          \
}

#define TEST_NAMED_SUB_PATTERN_DUPNAMES(_pattern, _string, _start_position, _sub_name, \
                                        _expected_sub, _expected_start, _expected_end) { \
  TestNamedSubData *data;                                                        \
  xchar_t *path;                                                                   \
  data = g_new0 (TestNamedSubData, 1);                                           \
  data->pattern = _pattern;                                                      \
  data->string = _string;                                                        \
  data->flags = XREGEX_DUPNAMES;                                                \
  data->start_position = _start_position;                                        \
  data->sub_name = _sub_name;                                                    \
  data->expected_sub = _expected_sub;                                            \
  data->expected_start = _expected_start;                                        \
  data->expected_end = _expected_end;                                            \
  path = xstrdup_printf ("/regex/match/subpattern/named/dupnames/%d", ++total); \
  g_test_add_data_func_full (path, data, test_named_sub_pattern, g_free);        \
  g_free (path);                                                                 \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xslist_t *expected;
  xint_t start_position;
  xint_t max_tokens;
} TestFetchAllData;

static void
test_fetch_all (xconstpointer d)
{
  const TestFetchAllData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xslist_t *l_exp;
  xchar_t **matches;
  xint_t match_count;
  xint_t i;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  xregex_match (regex, data->string, 0, &match_info);
  matches = xmatch_info_fetch_all (match_info);
  if (matches)
    match_count = xstrv_length (matches);
  else
    match_count = 0;

  g_assert_cmpint (match_count, ==, xslist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = xslist_next (l_exp))
    {
      g_assert_nonnull (matches);
      g_assert_cmpstr (l_exp->data, ==, matches[i]);
    }

  xmatch_info_free (match_info);
  xregex_unref (regex);
  xstrfreev (matches);
}

static void
free_fetch_all_data (xpointer_t _data)
{
  TestFetchAllData *data = _data;

  xslist_free (data->expected);
  g_free (data);
}

#define TEST_FETCH_ALL0(_pattern, _string) {                                   \
  TestFetchAllData *data;                                                      \
  xchar_t *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->expected = NULL;                                                       \
  path = xstrdup_printf ("/regex/fetch-all0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

#define TEST_FETCH_ALL1(_pattern, _string, e1) {                               \
  TestFetchAllData *data;                                                      \
  xchar_t *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->expected = xslist_append (NULL, e1);                                  \
  path = xstrdup_printf ("/regex/fetch-all1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

#define TEST_FETCH_ALL2(_pattern, _string, e1, e2) {                           \
  TestFetchAllData *data;                                                      \
  xchar_t *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->expected = xslist_append (NULL, e1);                                  \
  data->expected = xslist_append (data->expected, e2);                        \
  path = xstrdup_printf ("/regex/fetch-all2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

#define TEST_FETCH_ALL3(_pattern, _string, e1, e2, e3) {                       \
  TestFetchAllData *data;                                                      \
  xchar_t *path;                                                                 \
  data = g_new0 (TestFetchAllData, 1);                                         \
  data->pattern = _pattern;                                                    \
  data->string = _string;                                                      \
  data->expected = xslist_append (NULL, e1);                                  \
  data->expected = xslist_append (data->expected, e2);                        \
  data->expected = xslist_append (data->expected, e3);                        \
  path = xstrdup_printf ("/regex/fetch-all3/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_fetch_all, free_fetch_all_data); \
  g_free (path);                                                               \
}

static void
test_split_simple (xconstpointer d)
{
  const TestFetchAllData *data = d;
  xslist_t *l_exp;
  xchar_t **tokens;
  xint_t token_count;
  xint_t i;

  tokens = xregex_split_simple (data->pattern, data->string, 0, 0);
  if (tokens)
    token_count = xstrv_length (tokens);
  else
    token_count = 0;

  g_assert_cmpint (token_count, ==, xslist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = xslist_next (l_exp))
    {
      g_assert_nonnull (tokens);
      g_assert_cmpstr (l_exp->data, ==, tokens[i]);
    }

  xstrfreev (tokens);
}

#define TEST_SPLIT_SIMPLE0(_pattern, _string) {                                   \
  TestFetchAllData *data;                                                         \
  xchar_t *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = NULL;                                                          \
  path = xstrdup_printf ("/regex/split/simple0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

#define TEST_SPLIT_SIMPLE1(_pattern, _string, e1) {                               \
  TestFetchAllData *data;                                                         \
  xchar_t *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = xslist_append (NULL, e1);                                     \
  path = xstrdup_printf ("/regex/split/simple1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

#define TEST_SPLIT_SIMPLE2(_pattern, _string, e1, e2) {                           \
  TestFetchAllData *data;                                                         \
  xchar_t *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = xslist_append (NULL, e1);                                     \
  data->expected = xslist_append (data->expected, e2);                           \
  path = xstrdup_printf ("/regex/split/simple2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

#define TEST_SPLIT_SIMPLE3(_pattern, _string, e1, e2, e3) {                       \
  TestFetchAllData *data;                                                         \
  xchar_t *path;                                                                    \
  data = g_new0 (TestFetchAllData, 1);                                            \
  data->pattern = _pattern;                                                       \
  data->string = _string;                                                         \
  data->expected = xslist_append (NULL, e1);                                     \
  data->expected = xslist_append (data->expected, e2);                           \
  data->expected = xslist_append (data->expected, e3);                           \
  path = xstrdup_printf ("/regex/split/simple3/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_simple, free_fetch_all_data); \
  g_free (path);                                                                  \
}

static void
test_split_full (xconstpointer d)
{
  const TestFetchAllData *data = d;
  xregex_t *regex;
  xslist_t *l_exp;
  xchar_t **tokens;
  xint_t token_count;
  xint_t i;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  tokens = xregex_split_full (regex, data->string, -1, data->start_position,
			       0, data->max_tokens, NULL);
  if (tokens)
    token_count = xstrv_length (tokens);
  else
    token_count = 0;

  g_assert_cmpint (token_count, ==, xslist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = xslist_next (l_exp))
    {
      g_assert_nonnull (tokens);
      g_assert_cmpstr (l_exp->data, ==, tokens[i]);
    }

  xregex_unref (regex);
  xstrfreev (tokens);
}

static void
test_split (xconstpointer d)
{
  const TestFetchAllData *data = d;
  xregex_t *regex;
  xslist_t *l_exp;
  xchar_t **tokens;
  xint_t token_count;
  xint_t i;

  regex = xregex_new (data->pattern, 0, 0, NULL);

  g_assert (regex != NULL);

  tokens = xregex_split (regex, data->string, 0);
  if (tokens)
    token_count = xstrv_length (tokens);
  else
    token_count = 0;

  g_assert_cmpint (token_count, ==, xslist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; l_exp != NULL; i++, l_exp = xslist_next (l_exp))
    {
      g_assert_nonnull (tokens);
      g_assert_cmpstr (l_exp->data, ==, tokens[i]);
    }

  xregex_unref (regex);
  xstrfreev (tokens);
}

#define TEST_SPLIT0(_pattern, _string, _start_position, _max_tokens) {          \
  TestFetchAllData *data;                                                       \
  xchar_t *path;                                                                  \
  data = g_new0 (TestFetchAllData, 1);                                          \
  data->pattern = _pattern;                                                     \
  data->string = _string;                                                       \
  data->start_position = _start_position;                                       \
  data->max_tokens = _max_tokens;                                               \
  data->expected = NULL;                                                        \
  if (_start_position == 0 && _max_tokens <= 0) {                               \
    path = xstrdup_printf ("/regex/split0/%d", ++total);                       \
    g_test_add_data_func (path, data, test_split);                              \
    g_free (path);                                                              \
  }                                                                             \
  path = xstrdup_printf ("/regex/full-split0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data); \
  g_free (path);                                                                \
}

#define TEST_SPLIT1(_pattern, _string, _start_position, _max_tokens, e1) {      \
  TestFetchAllData *data;                                                       \
  xchar_t *path;                                                                  \
  data = g_new0 (TestFetchAllData, 1);                                          \
  data->pattern = _pattern;                                                     \
  data->string = _string;                                                       \
  data->start_position = _start_position;                                       \
  data->max_tokens = _max_tokens;                                               \
  data->expected = NULL;                                                        \
  data->expected = xslist_append (data->expected, e1);                         \
  if (_start_position == 0 && _max_tokens <= 0) {                               \
    path = xstrdup_printf ("/regex/split1/%d", ++total);                       \
    g_test_add_data_func (path, data, test_split);                              \
    g_free (path);                                                              \
  }                                                                             \
  path = xstrdup_printf ("/regex/full-split1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data); \
  g_free (path);                                                                \
}

#define TEST_SPLIT2(_pattern, _string, _start_position, _max_tokens, e1, e2) {  \
  TestFetchAllData *data;                                                       \
  xchar_t *path;                                                                  \
  data = g_new0 (TestFetchAllData, 1);                                          \
  data->pattern = _pattern;                                                     \
  data->string = _string;                                                       \
  data->start_position = _start_position;                                       \
  data->max_tokens = _max_tokens;                                               \
  data->expected = NULL;                                                        \
  data->expected = xslist_append (data->expected, e1);                         \
  data->expected = xslist_append (data->expected, e2);                         \
  if (_start_position == 0 && _max_tokens <= 0) {                               \
    path = xstrdup_printf ("/regex/split2/%d", ++total);                       \
    g_test_add_data_func (path, data, test_split);                              \
    g_free (path);                                                              \
  }                                                                             \
  path = xstrdup_printf ("/regex/full-split2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data); \
  g_free (path);                                                                \
}

#define TEST_SPLIT3(_pattern, _string, _start_position, _max_tokens, e1, e2, e3) { \
  TestFetchAllData *data;                                                          \
  xchar_t *path;                                                                     \
  data = g_new0 (TestFetchAllData, 1);                                             \
  data->pattern = _pattern;                                                        \
  data->string = _string;                                                          \
  data->start_position = _start_position;                                          \
  data->max_tokens = _max_tokens;                                                  \
  data->expected = NULL;                                                           \
  data->expected = xslist_append (data->expected, e1);                            \
  data->expected = xslist_append (data->expected, e2);                            \
  data->expected = xslist_append (data->expected, e3);                            \
  if (_start_position == 0 && _max_tokens <= 0) {                                  \
    path = xstrdup_printf ("/regex/split3/%d", ++total);                          \
    g_test_add_data_func (path, data, test_split);                                 \
    g_free (path);                                                                 \
  }                                                                                \
  path = xstrdup_printf ("/regex/full-split3/%d", ++total);                       \
  g_test_add_data_func_full (path, data, test_split_full, free_fetch_all_data);    \
  g_free (path);                                                                   \
}

typedef struct {
  const xchar_t *string_to_expand;
  xboolean_t expected;
  xboolean_t expected_refs;
} TestCheckReplacementData;

static void
test_check_replacement (xconstpointer d)
{
  const TestCheckReplacementData *data = d;
  xboolean_t has_refs;
  xboolean_t result;

  result = xregex_check_replacement (data->string_to_expand, &has_refs, NULL);
  g_assert_cmpint (data->expected, ==, result);

  if (data->expected)
    g_assert_cmpint (data->expected_refs, ==, has_refs);
}

#define TEST_CHECK_REPLACEMENT(_string_to_expand, _expected, _expected_refs) { \
  TestCheckReplacementData *data;                                              \
  xchar_t *path;                                                                 \
  data = g_new0 (TestCheckReplacementData, 1);                                 \
  data->string_to_expand = _string_to_expand;                                  \
  data->expected = _expected;                                                  \
  data->expected_refs = _expected_refs;                                        \
  path = xstrdup_printf ("/regex/check-repacement/%d", ++total);              \
  g_test_add_data_func_full (path, data, test_check_replacement, g_free);      \
  g_free (path);                                                               \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  const xchar_t *string_to_expand;
  xboolean_t     raw;
  const xchar_t *expected;
} TestExpandData;

static void
test_expand (xconstpointer d)
{
  const TestExpandData *data = d;
  xregex_t *regex = NULL;
  xmatch_info_t *match_info = NULL;
  xchar_t *res;
  xerror_t *error = NULL;

  if (data->pattern)
    {
      regex = xregex_new (data->pattern, data->raw ? XREGEX_RAW : 0, 0,
          &error);
      g_assert_no_error (error);
      xregex_match (regex, data->string, 0, &match_info);
    }

  res = xmatch_info_expand_references (match_info, data->string_to_expand, NULL);
  g_assert_cmpstr (res, ==, data->expected);
  g_free (res);
  xmatch_info_free (match_info);
  if (regex)
    xregex_unref (regex);
}

#define TEST_EXPAND(_pattern, _string, _string_to_expand, _raw, _expected) { \
  TestExpandData *data;                                                      \
  xchar_t *path;                                                               \
  data = g_new0 (TestExpandData, 1);                                         \
  data->pattern = _pattern;                                                  \
  data->string = _string;                                                    \
  data->string_to_expand = _string_to_expand;                                \
  data->raw = _raw;                                                          \
  data->expected = _expected;                                                \
  path = xstrdup_printf ("/regex/expand/%d", ++total);                      \
  g_test_add_data_func_full (path, data, test_expand, g_free);               \
  g_free (path);                                                             \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xint_t         start_position;
  const xchar_t *replacement;
  const xchar_t *expected;
} TestReplaceData;

static void
test_replace (xconstpointer d)
{
  const TestReplaceData *data = d;
  xregex_t *regex;
  xchar_t *res;

  regex = xregex_new (data->pattern, 0, 0, NULL);
  res = xregex_replace (regex, data->string, -1, data->start_position, data->replacement, 0, NULL);

  g_assert_cmpstr (res, ==, data->expected);

  g_free (res);
  xregex_unref (regex);
}

#define TEST_REPLACE(_pattern, _string, _start_position, _replacement, _expected) { \
  TestReplaceData *data;                                                \
  xchar_t *path;                                                          \
  data = g_new0 (TestReplaceData, 1);                                   \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->replacement = _replacement;                                     \
  data->expected = _expected;                                           \
  path = xstrdup_printf ("/regex/replace/%d", ++total);                \
  g_test_add_data_func_full (path, data, test_replace, g_free);         \
  g_free (path);                                                        \
}

static void
test_replace_lit (xconstpointer d)
{
  const TestReplaceData *data = d;
  xregex_t *regex;
  xchar_t *res;

  regex = xregex_new (data->pattern, 0, 0, NULL);
  res = xregex_replace_literal (regex, data->string, -1, data->start_position,
                                 data->replacement, 0, NULL);
  g_assert_cmpstr (res, ==, data->expected);

  g_free (res);
  xregex_unref (regex);
}

#define TEST_REPLACE_LIT(_pattern, _string, _start_position, _replacement, _expected) { \
  TestReplaceData *data;                                                \
  xchar_t *path;                                                          \
  data = g_new0 (TestReplaceData, 1);                                   \
  data->pattern = _pattern;                                             \
  data->string = _string;                                               \
  data->start_position = _start_position;                               \
  data->replacement = _replacement;                                     \
  data->expected = _expected;                                           \
  path = xstrdup_printf ("/regex/replace-literally/%d", ++total);      \
  g_test_add_data_func_full (path, data, test_replace_lit, g_free);     \
  g_free (path);                                                        \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *name;
  xint_t         expected_num;
} TestStringNumData;

static void
test_get_string_number (xconstpointer d)
{
  const TestStringNumData *data = d;
  xregex_t *regex;
  xint_t num;

  regex = xregex_new (data->pattern, 0, 0, NULL);
  num = xregex_get_string_number (regex, data->name);

  g_assert_cmpint (num, ==, data->expected_num);
  xregex_unref (regex);
}

#define TEST_GET_STRING_NUMBER(_pattern, _name, _expected_num) {          \
  TestStringNumData *data;                                                \
  xchar_t *path;                                                            \
  data = g_new0 (TestStringNumData, 1);                                   \
  data->pattern = _pattern;                                               \
  data->name = _name;                                                     \
  data->expected_num = _expected_num;                                     \
  path = xstrdup_printf ("/regex/string-number/%d", ++total);            \
  g_test_add_data_func_full (path, data, test_get_string_number, g_free); \
  g_free (path);                                                          \
}

typedef struct {
  const xchar_t *string;
  xint_t         length;
  const xchar_t *expected;
} TestEscapeData;

static void
test_escape (xconstpointer d)
{
  const TestEscapeData *data = d;
  xchar_t *escaped;

  escaped = xregex_escape_string (data->string, data->length);

  g_assert_cmpstr (escaped, ==, data->expected);

  g_free (escaped);
}

#define TEST_ESCAPE(_string, _length, _expected) {             \
  TestEscapeData *data;                                        \
  xchar_t *path;                                                 \
  data = g_new0 (TestEscapeData, 1);                           \
  data->string = _string;                                      \
  data->length = _length;                                      \
  data->expected = _expected;                                  \
  path = xstrdup_printf ("/regex/escape/%d", ++total);        \
  g_test_add_data_func_full (path, data, test_escape, g_free); \
  g_free (path);                                               \
}

static void
test_escape_nul (xconstpointer d)
{
  const TestEscapeData *data = d;
  xchar_t *escaped;

  escaped = xregex_escape_nul (data->string, data->length);

  g_assert_cmpstr (escaped, ==, data->expected);

  g_free (escaped);
}

#define TEST_ESCAPE_NUL(_string, _length, _expected) {             \
  TestEscapeData *data;                                            \
  xchar_t *path;                                                     \
  data = g_new0 (TestEscapeData, 1);                               \
  data->string = _string;                                          \
  data->length = _length;                                          \
  data->expected = _expected;                                      \
  path = xstrdup_printf ("/regex/escape_nul/%d", ++total);        \
  g_test_add_data_func_full (path, data, test_escape_nul, g_free); \
  g_free (path);                                                   \
}

typedef struct {
  const xchar_t *pattern;
  const xchar_t *string;
  xssize_t       string_len;
  xint_t         start_position;
  xslist_t *expected;
} TestMatchAllData;

static void
test_match_all_full (xconstpointer d)
{
  const TestMatchAllData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xslist_t *l_exp;
  xboolean_t match_ok;
  xint_t match_count;
  xint_t i;

  regex = xregex_new (data->pattern, 0, 0, NULL);
  match_ok = xregex_match_all_full (regex, data->string, data->string_len, data->start_position,
                                     0, &match_info, NULL);

  if (xslist_length (data->expected) == 0)
    g_assert (!match_ok);
  else
    g_assert (match_ok);

  match_count = xmatch_info_get_match_count (match_info);
  g_assert_cmpint (match_count, ==, xslist_length (data->expected));

  l_exp = data->expected;
  for (i = 0; i < match_count; i++)
    {
      xint_t start, end;
      xchar_t *matched_string;
      Match *exp = l_exp->data;

      matched_string = xmatch_info_fetch (match_info, i);
      xmatch_info_fetch_pos (match_info, i, &start, &end);

      g_assert_cmpstr (exp->string, ==, matched_string);
      g_assert_cmpint (exp->start, ==, start);
      g_assert_cmpint (exp->end, ==, end);

      g_free (matched_string);

      l_exp = xslist_next (l_exp);
    }

  xmatch_info_free (match_info);
  xregex_unref (regex);
}

static void
test_match_all (xconstpointer d)
{
  const TestMatchAllData *data = d;
  xregex_t *regex;
  xmatch_info_t *match_info;
  xslist_t *l_exp;
  xboolean_t match_ok;
  xuint_t i, match_count;

  regex = xregex_new (data->pattern, 0, 0, NULL);
  match_ok = xregex_match_all (regex, data->string, 0, &match_info);

  if (xslist_length (data->expected) == 0)
    g_assert (!match_ok);
  else
    g_assert (match_ok);

  match_count = xmatch_info_get_match_count (match_info);
  g_assert_cmpint (match_count, >=, 0);

  if (match_count != xslist_length (data->expected))
    {
      g_message ("regex: %s", data->pattern);
      g_message ("string: %s", data->string);
      g_message ("matched strings:");

      for (i = 0; i < match_count; i++)
        {
          xint_t start, end;
          xchar_t *matched_string;

          matched_string = xmatch_info_fetch (match_info, i);
          xmatch_info_fetch_pos (match_info, i, &start, &end);
          g_message ("%u. %d-%d '%s'", i, start, end, matched_string);
          g_free (matched_string);
        }

      g_message ("expected strings:");
      i = 0;

      for (l_exp = data->expected; l_exp != NULL; l_exp = l_exp->next)
        {
          Match *exp = l_exp->data;

          g_message ("%u. %d-%d '%s'", i, exp->start, exp->end, exp->string);
          i++;
        }

      xerror ("match_count not as expected: %u != %d",
          match_count, xslist_length (data->expected));
    }

  l_exp = data->expected;
  for (i = 0; i < match_count; i++)
    {
      xint_t start, end;
      xchar_t *matched_string;
      Match *exp = l_exp->data;

      matched_string = xmatch_info_fetch (match_info, i);
      xmatch_info_fetch_pos (match_info, i, &start, &end);

      g_assert_cmpstr (exp->string, ==, matched_string);
      g_assert_cmpint (exp->start, ==, start);
      g_assert_cmpint (exp->end, ==, end);

      g_free (matched_string);

      l_exp = xslist_next (l_exp);
    }

  xmatch_info_free (match_info);
  xregex_unref (regex);
}

static void
free_match_all_data (xpointer_t _data)
{
  TestMatchAllData *data = _data;

  xslist_free_full (data->expected, g_free);
  g_free (data);
}

#define TEST_MATCH_ALL0(_pattern, _string, _string_len, _start_position) {          \
  TestMatchAllData *data;                                                           \
  xchar_t *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = xstrdup_printf ("/regex/match-all0/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = xstrdup_printf ("/regex/match-all-full0/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

#define TEST_MATCH_ALL1(_pattern, _string, _string_len, _start_position,            \
                        t1, s1, e1) {                                               \
  TestMatchAllData *data;                                                           \
  Match *match;                                                                     \
  xchar_t *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  match = g_new0 (Match, 1);                                                        \
  match->string = t1;                                                               \
  match->start = s1;                                                                \
  match->end = e1;                                                                  \
  data->expected = xslist_append (data->expected, match);                          \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = xstrdup_printf ("/regex/match-all1/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = xstrdup_printf ("/regex/match-all-full1/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

#define TEST_MATCH_ALL2(_pattern, _string, _string_len, _start_position,            \
                        t1, s1, e1, t2, s2, e2) {                                   \
  TestMatchAllData *data;                                                           \
  Match *match;                                                                     \
  xchar_t *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  match = g_new0 (Match, 1);                                                        \
  match->string = t1;                                                               \
  match->start = s1;                                                                \
  match->end = e1;                                                                  \
  data->expected = xslist_append (data->expected, match);                          \
  match = g_new0 (Match, 1);                                                        \
  match->string = t2;                                                               \
  match->start = s2;                                                                \
  match->end = e2;                                                                  \
  data->expected = xslist_append (data->expected, match);                          \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = xstrdup_printf ("/regex/match-all2/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = xstrdup_printf ("/regex/match-all-full2/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

#define TEST_MATCH_ALL3(_pattern, _string, _string_len, _start_position,            \
                        t1, s1, e1, t2, s2, e2, t3, s3, e3) {                       \
  TestMatchAllData *data;                                                           \
  Match *match;                                                                     \
  xchar_t *path;                                                                      \
  data = g_new0 (TestMatchAllData, 1);                                              \
  data->pattern = _pattern;                                                         \
  data->string = _string;                                                           \
  data->string_len = _string_len;                                                   \
  data->start_position = _start_position;                                           \
  data->expected = NULL;                                                            \
  match = g_new0 (Match, 1);                                                        \
  match->string = t1;                                                               \
  match->start = s1;                                                                \
  match->end = e1;                                                                  \
  data->expected = xslist_append (data->expected, match);                          \
  match = g_new0 (Match, 1);                                                        \
  match->string = t2;                                                               \
  match->start = s2;                                                                \
  match->end = e2;                                                                  \
  data->expected = xslist_append (data->expected, match);                          \
  match = g_new0 (Match, 1);                                                        \
  match->string = t3;                                                               \
  match->start = s3;                                                                \
  match->end = e3;                                                                  \
  data->expected = xslist_append (data->expected, match);                          \
  if (_string_len == -1 && _start_position == 0) {                                  \
    path = xstrdup_printf ("/regex/match-all3/%d", ++total);                       \
    g_test_add_data_func (path, data, test_match_all);                              \
    g_free (path);                                                                  \
  }                                                                                 \
  path = xstrdup_printf ("/regex/match-all-full3/%d", ++total);                    \
  g_test_add_data_func_full (path, data, test_match_all_full, free_match_all_data); \
  g_free (path);                                                                    \
}

static void
test_properties (void)
{
  xregex_t *regex;
  xerror_t *error;
  xboolean_t res;
  xmatch_info_t *match;
  xchar_t *str;

  error = NULL;
  regex = xregex_new ("\\p{L}\\p{Ll}\\p{Lu}\\p{L&}\\p{N}\\p{Nd}", XREGEX_OPTIMIZE, 0, &error);
  res = xregex_match (regex, "ppPP01", 0, &match);
  g_assert (res);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "ppPP01");
  g_free (str);

  xmatch_info_free (match);
  xregex_unref (regex);
}

static void
test_class (void)
{
  xregex_t *regex;
  xerror_t *error;
  xmatch_info_t *match;
  xboolean_t res;
  xchar_t *str;

  error = NULL;
  regex = xregex_new ("[abc\\x{0B1E}\\p{Mn}\\x{0391}-\\x{03A9}]", XREGEX_OPTIMIZE, 0, &error);
  res = xregex_match (regex, "a:b:\340\254\236:\333\253:\316\240", 0, &match);
  g_assert (res);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "a");
  g_free (str);
  res = xmatch_info_next (match, NULL);
  g_assert (res);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "b");
  g_free (str);
  res = xmatch_info_next (match, NULL);
  g_assert (res);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "\340\254\236");
  g_free (str);
  res = xmatch_info_next (match, NULL);
  g_assert (res);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "\333\253");
  g_free (str);
  res = xmatch_info_next (match, NULL);
  g_assert (res);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "\316\240");
  g_free (str);

  res = xmatch_info_next (match, NULL);
  g_assert (!res);

  xmatch_info_free (match);
  xregex_unref (regex);
}

/* examples for lookahead assertions taken from pcrepattern(3) */
static void
test_lookahead (void)
{
  xregex_t *regex;
  xerror_t *error;
  xmatch_info_t *match;
  xboolean_t res;
  xchar_t *str;
  xint_t start, end;

  error = NULL;
  regex = xregex_new ("\\w+(?=;)", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "word1 word2: word3;", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 1);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "word3");
  g_free (str);
  xmatch_info_free (match);
  xregex_unref (regex);

  error = NULL;
  regex = xregex_new ("foo(?!bar)", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "foobar foobaz", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 1);
  res = xmatch_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 7);
  g_assert_cmpint (end, ==, 10);
  xmatch_info_free (match);
  xregex_unref (regex);

  error = NULL;
  regex = xregex_new ("(?!bar)foo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "foobar foobaz", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 1);
  res = xmatch_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 3);
  res = xmatch_info_next (match, &error);
  g_assert (res);
  g_assert_no_error (error);
  res = xmatch_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 7);
  g_assert_cmpint (end, ==, 10);
  xmatch_info_free (match);
  xregex_unref (regex);
}

/* examples for lookbehind assertions taken from pcrepattern(3) */
static void
test_lookbehind (void)
{
  xregex_t *regex;
  xerror_t *error;
  xmatch_info_t *match;
  xboolean_t res;
  xint_t start, end;

  error = NULL;
  regex = xregex_new ("(?<!foo)bar", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "foobar boobar", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 1);
  res = xmatch_info_fetch_pos (match, 0, &start, &end);
  g_assert (res);
  g_assert_cmpint (start, ==, 10);
  g_assert_cmpint (end, ==, 13);
  xmatch_info_free (match);
  xregex_unref (regex);

  error = NULL;
  regex = xregex_new ("(?<=bullock|donkey) poo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "don poo, and bullock poo", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 1);
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 20);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?<!dogs?|cats?) x", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex == NULL);
  g_assert_error (error, XREGEX_ERROR, XREGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND);
  g_clear_error (&error);

  regex = xregex_new ("(?<=ab(c|de)) foo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex == NULL);
  g_assert_error (error, XREGEX_ERROR, XREGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND);
  g_clear_error (&error);

  regex = xregex_new ("(?<=abc|abde)foo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "abfoo, abdfoo, abcfoo", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 18);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^.*+(?<=abcd)", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "abcabcabcabcabcabcabcabcabcd", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?<=\\d{3})(?<!999)foo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "999foo 123abcfoo 123foo", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 20);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?<=\\d{3}...)(?<!999)foo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "999foo 123abcfoo 123foo", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 13);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?<=\\d{3}(?!999)...)foo", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "999foo 123abcfoo 123foo", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 13);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?<=(?<!foo)bar)baz", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "foobarbaz barfoobaz barbarbaz", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 26);
  xmatch_info_free (match);
  xregex_unref (regex);
}

/* examples for subpatterns taken from pcrepattern(3) */
static void
test_subpattern (void)
{
  xregex_t *regex;
  xerror_t *error;
  xmatch_info_t *match;
  xboolean_t res;
  xchar_t *str;
  xint_t start;

  error = NULL;
  regex = xregex_new ("cat(aract|erpillar|)", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (xregex_get_capture_count (regex), ==, 1);
  g_assert_cmpint (xregex_get_max_backref (regex), ==, 0);
  res = xregex_match_all (regex, "caterpillar", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 2);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "caterpillar");
  g_free (str);
  str = xmatch_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "cat");
  g_free (str);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("the ((red|white) (king|queen))", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (xregex_get_capture_count (regex), ==, 3);
  g_assert_cmpint (xregex_get_max_backref (regex), ==, 0);
  res = xregex_match (regex, "the red king", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 4);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "the red king");
  g_free (str);
  str = xmatch_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "red king");
  g_free (str);
  str = xmatch_info_fetch (match, 2);
  g_assert_cmpstr (str, ==, "red");
  g_free (str);
  str = xmatch_info_fetch (match, 3);
  g_assert_cmpstr (str, ==, "king");
  g_free (str);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("the ((?:red|white) (king|queen))", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "the white queen", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 3);
  g_assert_cmpint (xregex_get_max_backref (regex), ==, 0);
  str = xmatch_info_fetch (match, 0);
  g_assert_cmpstr (str, ==, "the white queen");
  g_free (str);
  str = xmatch_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "white queen");
  g_free (str);
  str = xmatch_info_fetch (match, 2);
  g_assert_cmpstr (str, ==, "queen");
  g_free (str);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?|(Sat)(ur)|(Sun))day (morning|afternoon)", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (xregex_get_capture_count (regex), ==, 3);
  res = xregex_match (regex, "Saturday morning", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  g_assert_cmpint (xmatch_info_get_match_count (match), ==, 4);
  str = xmatch_info_fetch (match, 1);
  g_assert_cmpstr (str, ==, "Sat");
  g_free (str);
  str = xmatch_info_fetch (match, 2);
  g_assert_cmpstr (str, ==, "ur");
  g_free (str);
  str = xmatch_info_fetch (match, 3);
  g_assert_cmpstr (str, ==, "morning");
  g_free (str);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?|(abc)|(def))\\1", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  g_assert_cmpint (xregex_get_max_backref (regex), ==, 1);
  res = xregex_match (regex, "abcabc abcdef defabc defdef", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  res = xmatch_info_next (match, &error);
  g_assert (res);
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 21);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?|(abc)|(def))(?1)", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "abcabc abcdef defabc defdef", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  res = xmatch_info_next (match, &error);
  g_assert (res);
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 14);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?<DN>Mon|Fri|Sun)(?:day)?|(?<DN>Tue)(?:sday)?|(?<DN>Wed)(?:nesday)?|(?<DN>Thu)(?:rsday)?|(?<DN>Sat)(?:urday)?", XREGEX_OPTIMIZE|XREGEX_DUPNAMES, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "Mon Tuesday Wed Saturday", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  str = xmatch_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Mon");
  g_free (str);
  res = xmatch_info_next (match, &error);
  g_assert (res);
  str = xmatch_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Tue");
  g_free (str);
  res = xmatch_info_next (match, &error);
  g_assert (res);
  str = xmatch_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Wed");
  g_free (str);
  res = xmatch_info_next (match, &error);
  g_assert (res);
  str = xmatch_info_fetch_named (match, "DN");
  g_assert_cmpstr (str, ==, "Sat");
  g_free (str);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^(a|b\\1)+$", XREGEX_OPTIMIZE|XREGEX_DUPNAMES, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "aaaaaaaaaaaaaaaa", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "ababbaa", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);
}

/* examples for conditions taken from pcrepattern(3) */
static void
test_condition (void)
{
  xregex_t *regex;
  xerror_t *error;
  xmatch_info_t *match;
  xboolean_t res;

  error = NULL;
  regex = xregex_new ("^(a+)(\\()?[^()]+(?(-1)\\))(b+)$", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "a(zzzzzz)b", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "aaazzzzzzbbb", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  error = NULL;
  regex = xregex_new ("^(a+)(?<OPEN>\\()?[^()]+(?(<OPEN>)\\))(b+)$", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "a(zzzzzz)b", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "aaazzzzzzbbb", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^(a+)(?(+1)\\[|\\<)?[^()]+(\\])?(b+)$", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "a[zzzzzz]b", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "aaa<zzzzzzbbb", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("(?(DEFINE) (?<byte> 2[0-4]\\d | 25[0-5] | 1\\d\\d | [1-9]?\\d) )"
                       "\\b (?&byte) (\\.(?&byte)){3} \\b",
                       XREGEX_OPTIMIZE|XREGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "128.0.0.1", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "192.168.1.1", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "209.132.180.167", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^(?(?=[^a-z]*[a-z])"
                       "\\d{2}-[a-z]{3}-\\d{2} | \\d{2}-\\d{2}-\\d{2} )$",
                       XREGEX_OPTIMIZE|XREGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "01-abc-24", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "01-23-45", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "01-uv-45", 0, &match);
  g_assert (!res);
  g_assert (!xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "01-234-45", 0, &match);
  g_assert (!res);
  g_assert (!xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);
}

/* examples for recursion taken from pcrepattern(3) */
static void
test_recursion (void)
{
  xregex_t *regex;
  xerror_t *error;
  xmatch_info_t *match;
  xboolean_t res;
  xint_t start;

  error = NULL;
  regex = xregex_new ("\\( ( [^()]++ | (?R) )* \\)", XREGEX_OPTIMIZE|XREGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "(middle)", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "((((((((((((((((middle))))))))))))))))", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "(((xxx(((", 0, &match);
  g_assert (!res);
  g_assert (!xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^( \\( ( [^()]++ | (?1) )* \\) )$", XREGEX_OPTIMIZE|XREGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "((((((((((((((((middle))))))))))))))))", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "(((xxx((()", 0, &match);
  g_assert (!res);
  g_assert (!xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^(?<pn> \\( ( [^()]++ | (?&pn) )* \\) )$", XREGEX_OPTIMIZE|XREGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  xregex_match (regex, "(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa()", 0, &match);
  g_assert (!res);
  g_assert (!xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("< (?: (?(R) \\d++ | [^<>]*+) | (?R)) * >", XREGEX_OPTIMIZE|XREGEX_EXTENDED, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "<ab<01<23<4>>>>", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, ==, 0);
  xmatch_info_free (match);
  res = xregex_match (regex, "<ab<01<xx<x>>>>", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  res = xmatch_info_fetch_pos (match, 0, &start, NULL);
  g_assert (res);
  g_assert_cmpint (start, >, 0);
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^((.)(?1)\\2|.)$", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "abcdcba", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "abcddcba", 0, &match);
  g_assert (!res);
  g_assert (!xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^(?:((.)(?1)\\2|)|((.)(?3)\\4|.))$", XREGEX_OPTIMIZE, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "abcdcba", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "abcddcba", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);

  regex = xregex_new ("^\\W*+(?:((.)\\W*+(?1)\\W*+\\2|)|((.)\\W*+(?3)\\W*+\\4|\\W*+.\\W*+))\\W*+$", XREGEX_OPTIMIZE|XREGEX_CASELESS, 0, &error);
  g_assert (regex);
  g_assert_no_error (error);
  res = xregex_match (regex, "abcdcba", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "A man, a plan, a canal: Panama!", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  res = xregex_match (regex, "Oozy rat in a sanitary zoo", 0, &match);
  g_assert (res);
  g_assert (xmatch_info_matches (match));
  xmatch_info_free (match);
  xregex_unref (regex);
}

static void
test_multiline (void)
{
  xregex_t *regex;
  xmatch_info_t *info;
  xint_t count;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=640489");

  regex = xregex_new ("^a$", XREGEX_MULTILINE|XREGEX_DOTALL, 0, NULL);

  count = 0;
  xregex_match (regex, "a\nb\na", 0, &info);
  while (xmatch_info_matches (info))
    {
      count++;
      xmatch_info_next (info, NULL);
    }
  xmatch_info_free (info);
  xregex_unref (regex);

  g_assert_cmpint (count, ==, 2);
}

static void
test_explicit_crlf (void)
{
  xregex_t *regex;

  regex = xregex_new ("[\r\n]a", 0, 0, NULL);
  g_assert_cmpint (xregex_get_has_cr_or_lf (regex), ==, TRUE);
  xregex_unref (regex);
}

static void
test_max_lookbehind (void)
{
  xregex_t *regex;

  regex = xregex_new ("abc", 0, 0, NULL);
  g_assert_cmpint (xregex_get_max_lookbehind (regex), ==, 0);
  xregex_unref (regex);

  regex = xregex_new ("\\babc", 0, 0, NULL);
  g_assert_cmpint (xregex_get_max_lookbehind (regex), ==, 1);
  xregex_unref (regex);

  regex = xregex_new ("(?<=123)abc", 0, 0, NULL);
  g_assert_cmpint (xregex_get_max_lookbehind (regex), ==, 3);
  xregex_unref (regex);
}

static xboolean_t
pcre_ge (xuint64_t major, xuint64_t minor)
{
    const char *version;
    xchar_t *ptr;
    xuint64_t pcre_major, pcre_minor;

    /* e.g. 8.35 2014-04-04 */
    version = pcre_version ();

    pcre_major = g_ascii_strtoull (version, &ptr, 10);
    /* ptr points to ".MINOR (release date)" */
    g_assert (ptr[0] == '.');
    pcre_minor = g_ascii_strtoull (ptr + 1, NULL, 10);

    return (pcre_major > major) || (pcre_major == major && pcre_minor >= minor);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/regex/properties", test_properties);
  g_test_add_func ("/regex/class", test_class);
  g_test_add_func ("/regex/lookahead", test_lookahead);
  g_test_add_func ("/regex/lookbehind", test_lookbehind);
  g_test_add_func ("/regex/subpattern", test_subpattern);
  g_test_add_func ("/regex/condition", test_condition);
  g_test_add_func ("/regex/recursion", test_recursion);
  g_test_add_func ("/regex/multiline", test_multiline);
  g_test_add_func ("/regex/explicit-crlf", test_explicit_crlf);
  g_test_add_func ("/regex/max-lookbehind", test_max_lookbehind);

  /* TEST_NEW(pattern, compile_opts, match_opts) */
  TEST_NEW("[A-Z]+", XREGEX_CASELESS | XREGEX_EXTENDED | XREGEX_OPTIMIZE, XREGEX_MATCH_NOTBOL | XREGEX_MATCH_PARTIAL);
  TEST_NEW("", 0, 0);
  TEST_NEW(".*", 0, 0);
  TEST_NEW(".*", XREGEX_OPTIMIZE, 0);
  TEST_NEW(".*", XREGEX_MULTILINE, 0);
  TEST_NEW(".*", XREGEX_DOTALL, 0);
  TEST_NEW(".*", XREGEX_DOTALL, XREGEX_MATCH_NOTBOL);
  TEST_NEW("(123\\d*)[a-zA-Z]+(?P<hello>.*)", 0, 0);
  TEST_NEW("(123\\d*)[a-zA-Z]+(?P<hello>.*)", XREGEX_CASELESS, 0);
  TEST_NEW("(123\\d*)[a-zA-Z]+(?P<hello>.*)", XREGEX_CASELESS | XREGEX_OPTIMIZE, 0);
  TEST_NEW("(?P<A>x)|(?P<A>y)", XREGEX_DUPNAMES, 0);
  TEST_NEW("(?P<A>x)|(?P<A>y)", XREGEX_DUPNAMES | XREGEX_OPTIMIZE, 0);
  /* This gives "internal error: code overflow" with pcre 6.0 */
  TEST_NEW("(?i)(?-i)", 0, 0);
  TEST_NEW ("(?i)a", 0, 0);
  TEST_NEW ("(?m)a", 0, 0);
  TEST_NEW ("(?s)a", 0, 0);
  TEST_NEW ("(?x)a", 0, 0);
  TEST_NEW ("(?J)a", 0, 0);
  TEST_NEW ("(?U)[a-z]+", 0, 0);

  /* TEST_NEW_CHECK_FLAGS(pattern, compile_opts, match_ops, real_compile_opts, real_match_opts) */
  TEST_NEW_CHECK_FLAGS ("a", XREGEX_OPTIMIZE, 0, XREGEX_OPTIMIZE, 0);
  TEST_NEW_CHECK_FLAGS ("a", XREGEX_RAW, 0, XREGEX_RAW, 0);
  TEST_NEW_CHECK_FLAGS ("(?X)a", 0, 0, 0 /* not exposed by xregex_t */, 0);
  TEST_NEW_CHECK_FLAGS ("^.*", 0, 0, XREGEX_ANCHORED, 0);
  TEST_NEW_CHECK_FLAGS ("(*UTF8)a", 0, 0, 0 /* this is the default in xregex_t */, 0);
  TEST_NEW_CHECK_FLAGS ("(*UCP)a", 0, 0, 0 /* this always on in xregex_t */, 0);
  TEST_NEW_CHECK_FLAGS ("(*CR)a", 0, 0, XREGEX_NEWLINE_CR, 0);
  TEST_NEW_CHECK_FLAGS ("(*LF)a", 0, 0, XREGEX_NEWLINE_LF, 0);
  TEST_NEW_CHECK_FLAGS ("(*CRLF)a", 0, 0, XREGEX_NEWLINE_CRLF, 0);
  TEST_NEW_CHECK_FLAGS ("(*ANY)a", 0, 0, 0 /* this is the default in xregex_t */, 0);
  TEST_NEW_CHECK_FLAGS ("(*ANYCRLF)a", 0, 0, XREGEX_NEWLINE_ANYCRLF, 0);
  TEST_NEW_CHECK_FLAGS ("(*BSR_ANYCRLF)a", 0, 0, XREGEX_BSR_ANYCRLF, 0);
  TEST_NEW_CHECK_FLAGS ("(*BSR_UNICODE)a", 0, 0, 0 /* this is the default in xregex_t */, 0);
  TEST_NEW_CHECK_FLAGS ("(*NO_START_OPT)a", 0, 0, 0 /* not exposed in xregex_t */, 0);

  /* TEST_NEW_FAIL(pattern, compile_opts, expected_error) */
  TEST_NEW_FAIL("(", 0, XREGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL(")", 0, XREGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL("[", 0, XREGEX_ERROR_UNTERMINATED_CHARACTER_CLASS);
  TEST_NEW_FAIL("*", 0, XREGEX_ERROR_NOTHING_TO_REPEAT);
  TEST_NEW_FAIL("?", 0, XREGEX_ERROR_NOTHING_TO_REPEAT);
  TEST_NEW_FAIL("(?P<A>x)|(?P<A>y)", 0, XREGEX_ERROR_DUPLICATE_SUBPATTERN_NAME);

  /* Check all GRegexError codes */
  TEST_NEW_FAIL ("a\\", 0, XREGEX_ERROR_STRAY_BACKSLASH);
  TEST_NEW_FAIL ("a\\c", 0, XREGEX_ERROR_MISSING_CONTROL_CHAR);
  TEST_NEW_FAIL ("a\\l", 0, XREGEX_ERROR_UNRECOGNIZED_ESCAPE);
  TEST_NEW_FAIL ("a{4,2}", 0, XREGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER);
  TEST_NEW_FAIL ("a{999999,}", 0, XREGEX_ERROR_QUANTIFIER_TOO_BIG);
  TEST_NEW_FAIL ("[a-z", 0, XREGEX_ERROR_UNTERMINATED_CHARACTER_CLASS);
  TEST_NEW_FAIL ("(?X)[\\B]", 0, XREGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS);
  TEST_NEW_FAIL ("[z-a]", 0, XREGEX_ERROR_RANGE_OUT_OF_ORDER);
  TEST_NEW_FAIL ("{2,4}", 0, XREGEX_ERROR_NOTHING_TO_REPEAT);
  TEST_NEW_FAIL ("a(?u)", 0, XREGEX_ERROR_UNRECOGNIZED_CHARACTER);
  TEST_NEW_FAIL ("a(?<$foo)bar", 0, XREGEX_ERROR_UNRECOGNIZED_CHARACTER);
  TEST_NEW_FAIL ("a[:alpha:]b", 0, XREGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS);
  TEST_NEW_FAIL ("a(b", 0, XREGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("a)b", 0, XREGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("a(?R", 0, XREGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("a(?-54", 0, XREGEX_ERROR_UNMATCHED_PARENTHESIS);
  TEST_NEW_FAIL ("(ab\\2)", 0, XREGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE);
  TEST_NEW_FAIL ("a(?#abc", 0, XREGEX_ERROR_UNTERMINATED_COMMENT);
  TEST_NEW_FAIL ("(?<=a+)b", 0, XREGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND);
  TEST_NEW_FAIL ("(?(1?)a|b)", 0, XREGEX_ERROR_MALFORMED_CONDITION);
  TEST_NEW_FAIL ("(a)(?(1)a|b|c)", 0, XREGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES);
  TEST_NEW_FAIL ("(?(?i))", 0, XREGEX_ERROR_ASSERTION_EXPECTED);
  TEST_NEW_FAIL ("a[[:fubar:]]b", 0, XREGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME);
  TEST_NEW_FAIL ("[[.ch.]]", 0, XREGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED);
  TEST_NEW_FAIL ("\\x{110000}", 0, XREGEX_ERROR_HEX_CODE_TOO_LARGE);
  TEST_NEW_FAIL ("^(?(0)f|b)oo", 0, XREGEX_ERROR_INVALID_CONDITION);
  TEST_NEW_FAIL ("(?<=\\C)X", 0, XREGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND);
  TEST_NEW_FAIL ("(?!\\w)(?R)", 0, XREGEX_ERROR_INFINITE_LOOP);
  if (pcre_ge (8, 37))
    {
      /* The expected errors changed here. */
      TEST_NEW_FAIL ("(?(?<ab))", 0, XREGEX_ERROR_ASSERTION_EXPECTED);
    }
  else
    {
      TEST_NEW_FAIL ("(?(?<ab))", 0, XREGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR);
    }

  if (pcre_ge (8, 35))
    {
      /* The expected errors changed here. */
      TEST_NEW_FAIL ("(?P<sub>foo)\\g<sub", 0, XREGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR);
    }
  else
    {
      TEST_NEW_FAIL ("(?P<sub>foo)\\g<sub", 0, XREGEX_ERROR_MISSING_BACK_REFERENCE);
    }
  TEST_NEW_FAIL ("(?P<x>eks)(?P<x>eccs)", 0, XREGEX_ERROR_DUPLICATE_SUBPATTERN_NAME);
#if 0
  TEST_NEW_FAIL (?, 0, XREGEX_ERROR_MALFORMED_PROPERTY);
  TEST_NEW_FAIL (?, 0, XREGEX_ERROR_UNKNOWN_PROPERTY);
#endif
  TEST_NEW_FAIL ("\\666", XREGEX_RAW, XREGEX_ERROR_INVALID_OCTAL_VALUE);
  TEST_NEW_FAIL ("^(?(DEFINE) abc | xyz ) ", 0, XREGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE);
  TEST_NEW_FAIL ("a", XREGEX_NEWLINE_CRLF | XREGEX_NEWLINE_ANYCRLF, XREGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS);
  TEST_NEW_FAIL ("^(a)\\g{3", 0, XREGEX_ERROR_MISSING_BACK_REFERENCE);
  TEST_NEW_FAIL ("^(a)\\g{0}", 0, XREGEX_ERROR_INVALID_RELATIVE_REFERENCE);
  TEST_NEW_FAIL ("abc(*FAIL:123)xyz", 0, XREGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN);
  TEST_NEW_FAIL ("a(*FOOBAR)b", 0, XREGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB);
  TEST_NEW_FAIL ("(?i:A{1,}\\6666666666)", 0, XREGEX_ERROR_NUMBER_TOO_BIG);
  TEST_NEW_FAIL ("(?<a>)(?&)", 0, XREGEX_ERROR_MISSING_SUBPATTERN_NAME);
  TEST_NEW_FAIL ("(?+-a)", 0, XREGEX_ERROR_MISSING_DIGIT);
  TEST_NEW_FAIL ("TA]", XREGEX_JAVASCRIPT_COMPAT, XREGEX_ERROR_INVALID_DATA_CHARACTER);
  TEST_NEW_FAIL ("(?|(?<a>A)|(?<b>B))", 0, XREGEX_ERROR_EXTRA_SUBPATTERN_NAME);
  TEST_NEW_FAIL ("a(*MARK)b", 0, XREGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED);
  TEST_NEW_FAIL ("^\\c€", 0, XREGEX_ERROR_INVALID_CONTROL_CHAR);
  TEST_NEW_FAIL ("\\k", 0, XREGEX_ERROR_MISSING_NAME);
  TEST_NEW_FAIL ("a[\\NB]c", 0, XREGEX_ERROR_NOT_SUPPORTED_IN_CLASS);
  TEST_NEW_FAIL ("(*:0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEFG)XX", 0, XREGEX_ERROR_NAME_TOO_LONG);
  TEST_NEW_FAIL ("\\u0100", XREGEX_RAW | XREGEX_JAVASCRIPT_COMPAT, XREGEX_ERROR_CHARACTER_VALUE_TOO_LARGE);

  /* These errors can't really be tested easily:
   * XREGEX_ERROR_EXPRESSION_TOO_LARGE
   * XREGEX_ERROR_MEMORY_ERROR
   * XREGEX_ERROR_SUBPATTERN_NAME_TOO_LONG
   * XREGEX_ERROR_TOO_MANY_SUBPATTERNS
   * XREGEX_ERROR_TOO_MANY_FORWARD_REFERENCES
   *
   * These errors are obsolete and never raised by PCRE:
   * XREGEX_ERROR_DEFINE_REPETION
   */

  /* TEST_MATCH_SIMPLE(pattern, string, compile_opts, match_opts, expected) */
  TEST_MATCH_SIMPLE("a", "", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("a", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("a", "ba", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("^a", "ba", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("a", "ba", XREGEX_ANCHORED, 0, FALSE);
  TEST_MATCH_SIMPLE("a", "ba", 0, XREGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH_SIMPLE("a", "ab", XREGEX_ANCHORED, 0, TRUE);
  TEST_MATCH_SIMPLE("a", "ab", 0, XREGEX_MATCH_ANCHORED, TRUE);
  TEST_MATCH_SIMPLE("a", "a", XREGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE("a", "A", XREGEX_CASELESS, 0, TRUE);
  /* These are needed to test extended properties. */
  TEST_MATCH_SIMPLE(AGRAVE, AGRAVE, XREGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE(AGRAVE, AGRAVE_UPPER, XREGEX_CASELESS, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{L}", AGRAVE, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", AGRAVE_UPPER, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", SHEEN, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ll}", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Ll}", AGRAVE, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Ll}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ll}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Sc}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Sc}", EURO, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Sc}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", "1", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{N}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{N}", ETH30, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Nd}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", "1", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Nd}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Nd}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Common}", "%", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Common}", "1", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", SHEEN, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", "%", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Arabic}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", "a", 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Latin}", AGRAVE, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Latin}", AGRAVE_UPPER, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Latin}", ETH30, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", "%", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Latin}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", SHEEN, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", AGRAVE, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", AGRAVE_UPPER, 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", ETH30, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", "%", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{Ethiopic}", "1", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("\\p{L}(?<=\\p{Arabic})", SHEEN, 0, 0, TRUE);
  TEST_MATCH_SIMPLE("\\p{L}(?<=\\p{Latin})", SHEEN, 0, 0, FALSE);
  /* Invalid patterns. */
  TEST_MATCH_SIMPLE("\\", "a", 0, 0, FALSE);
  TEST_MATCH_SIMPLE("[", "", 0, 0, FALSE);

  /* TEST_MATCH(pattern, compile_opts, match_opts, string,
   * 		string_len, start_position, match_opts2, expected) */
  TEST_MATCH("a", 0, 0, "a", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, 0, "A", -1, 0, 0, FALSE);
  TEST_MATCH("a", XREGEX_CASELESS, 0, "A", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, 0, "ab", -1, 1, 0, FALSE);
  TEST_MATCH("a", 0, 0, "ba", 1, 0, 0, FALSE);
  TEST_MATCH("a", 0, 0, "bab", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, 0, "b", -1, 0, 0, FALSE);
  TEST_MATCH("a", 0, XREGEX_MATCH_ANCHORED, "a", -1, 0, 0, TRUE);
  TEST_MATCH("a", 0, XREGEX_MATCH_ANCHORED, "ab", -1, 1, 0, FALSE);
  TEST_MATCH("a", 0, XREGEX_MATCH_ANCHORED, "ba", 1, 0, 0, FALSE);
  TEST_MATCH("a", 0, XREGEX_MATCH_ANCHORED, "bab", -1, 0, 0, FALSE);
  TEST_MATCH("a", 0, XREGEX_MATCH_ANCHORED, "b", -1, 0, 0, FALSE);
  TEST_MATCH("a", 0, 0, "a", -1, 0, XREGEX_MATCH_ANCHORED, TRUE);
  TEST_MATCH("a", 0, 0, "ab", -1, 1, XREGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a", 0, 0, "ba", 1, 0, XREGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a", 0, 0, "bab", -1, 0, XREGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a", 0, 0, "b", -1, 0, XREGEX_MATCH_ANCHORED, FALSE);
  TEST_MATCH("a|b", 0, 0, "a", -1, 0, 0, TRUE);
  TEST_MATCH("\\d", 0, 0, EURO, -1, 0, 0, FALSE);
  TEST_MATCH("^.$", 0, 0, EURO, -1, 0, 0, TRUE);
  TEST_MATCH("^.{3}$", 0, 0, EURO, -1, 0, 0, FALSE);
  TEST_MATCH("^.$", XREGEX_RAW, 0, EURO, -1, 0, 0, FALSE);
  TEST_MATCH("^.{3}$", XREGEX_RAW, 0, EURO, -1, 0, 0, TRUE);
  TEST_MATCH(AGRAVE, XREGEX_CASELESS, 0, AGRAVE_UPPER, -1, 0, 0, TRUE);

  /* New lines handling. */
  TEST_MATCH("^a\\Rb$", 0, 0, "a\r\nb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\Rb$", 0, 0, "a\nb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\Rb$", 0, 0, "a\rb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\Rb$", 0, 0, "a\n\rb", -1, 0, 0, FALSE);
  TEST_MATCH("^a\\R\\Rb$", 0, 0, "a\n\rb", -1, 0, 0, TRUE);
  TEST_MATCH("^a\\nb$", 0, 0, "a\r\nb", -1, 0, 0, FALSE);
  TEST_MATCH("^a\\r\\nb$", 0, 0, "a\r\nb", -1, 0, 0, TRUE);

  TEST_MATCH("^b$", 0, 0, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, 0, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, 0, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, 0, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, 0, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_LF, 0, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CRLF, 0, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, 0, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_LF, 0, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CRLF, 0, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, 0, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_LF, 0, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CRLF, 0, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_CR, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_LF, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_CRLF, "a\nb\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_CR, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_LF, "a\r\nb\r\nc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_CRLF, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_CR, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_LF, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE, XREGEX_MATCH_NEWLINE_CRLF, "a\rb\rc", -1, 0, 0, FALSE);

  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_ANY, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_ANY, "a\rb\rc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_ANY, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_LF, "a\nb\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_LF, "a\rb\rc", -1, 0, 0, FALSE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_CRLF, "a\r\nb\r\nc", -1, 0, 0, TRUE);
  TEST_MATCH("^b$", XREGEX_MULTILINE | XREGEX_NEWLINE_CR, XREGEX_MATCH_NEWLINE_CRLF, "a\rb\rc", -1, 0, 0, FALSE);

  TEST_MATCH("a#\nb", XREGEX_EXTENDED, 0, "a", -1, 0, 0, FALSE);
  TEST_MATCH("a#\r\nb", XREGEX_EXTENDED, 0, "a", -1, 0, 0, FALSE);
  TEST_MATCH("a#\rb", XREGEX_EXTENDED, 0, "a", -1, 0, 0, FALSE);
  TEST_MATCH("a#\nb", XREGEX_EXTENDED, XREGEX_MATCH_NEWLINE_CR, "a", -1, 0, 0, FALSE);
  TEST_MATCH("a#\nb", XREGEX_EXTENDED | XREGEX_NEWLINE_CR, 0, "a", -1, 0, 0, TRUE);

  TEST_MATCH("line\nbreak", XREGEX_MULTILINE, 0, "this is a line\nbreak", -1, 0, 0, TRUE);
  TEST_MATCH("line\nbreak", XREGEX_MULTILINE | XREGEX_FIRSTLINE, 0, "first line\na line\nbreak", -1, 0, 0, FALSE);

  /* This failed with PCRE 7.2 (gnome bug #455640) */
  TEST_MATCH(".*$", 0, 0, "\xe1\xbb\x85", -1, 0, 0, TRUE);

  /* Test that othercasing in our pcre/glib integration is bug-for-bug compatible
   * with pcre's internal tables. Bug #678273 */
  TEST_MATCH("[Ǆ]", XREGEX_CASELESS, 0, "Ǆ", -1, 0, 0, TRUE);
  TEST_MATCH("[Ǆ]", XREGEX_CASELESS, 0, "ǆ", -1, 0, 0, TRUE);
#if PCRE_MAJOR > 8 || (PCRE_MAJOR == 8 && PCRE_MINOR >= 32)
  /* This would incorrectly fail to match in pcre < 8.32, so only assert
   * this for known-good pcre. */
  TEST_MATCH("[Ǆ]", XREGEX_CASELESS, 0, "ǅ", -1, 0, 0, TRUE);
#endif

  /* TEST_MATCH_NEXT#(pattern, string, string_len, start_position, ...) */
  TEST_MATCH_NEXT0("a", "x", -1, 0);
  TEST_MATCH_NEXT0("a", "ax", -1, 1);
  TEST_MATCH_NEXT0("a", "xa", 1, 0);
  TEST_MATCH_NEXT0("a", "axa", 1, 2);
  TEST_MATCH_NEXT1("a", "a", -1, 0, "a", 0, 1);
  TEST_MATCH_NEXT1("a", "xax", -1, 0, "a", 1, 2);
  TEST_MATCH_NEXT1(EURO, ENG EURO, -1, 0, EURO, 2, 5);
  TEST_MATCH_NEXT1("a*", "", -1, 0, "", 0, 0);
  TEST_MATCH_NEXT2("a*", "aa", -1, 0, "aa", 0, 2, "", 2, 2);
  TEST_MATCH_NEXT2(EURO "*", EURO EURO, -1, 0, EURO EURO, 0, 6, "", 6, 6);
  TEST_MATCH_NEXT2("a", "axa", -1, 0, "a", 0, 1, "a", 2, 3);
  TEST_MATCH_NEXT2("a+", "aaxa", -1, 0, "aa", 0, 2, "a", 3, 4);
  TEST_MATCH_NEXT2("a", "aa", -1, 0, "a", 0, 1, "a", 1, 2);
  TEST_MATCH_NEXT2("a", "ababa", -1, 2, "a", 2, 3, "a", 4, 5);
  TEST_MATCH_NEXT2(EURO "+", EURO "-" EURO, -1, 0, EURO, 0, 3, EURO, 4, 7);
  TEST_MATCH_NEXT3("", "ab", -1, 0, "", 0, 0, "", 1, 1, "", 2, 2);
  TEST_MATCH_NEXT3("", AGRAVE "b", -1, 0, "", 0, 0, "", 2, 2, "", 3, 3);
  TEST_MATCH_NEXT3("a", "aaxa", -1, 0, "a", 0, 1, "a", 1, 2, "a", 3, 4);
  TEST_MATCH_NEXT3("a", "aa" OGRAVE "a", -1, 0, "a", 0, 1, "a", 1, 2, "a", 4, 5);
  TEST_MATCH_NEXT3("a*", "aax", -1, 0, "aa", 0, 2, "", 2, 2, "", 3, 3);
  TEST_MATCH_NEXT3("(?=[A-Z0-9])", "RegExTest", -1, 0, "", 0, 0, "", 3, 3, "", 5, 5);
  TEST_MATCH_NEXT4("a*", "aaxa", -1, 0, "aa", 0, 2, "", 2, 2, "a", 3, 4, "", 4, 4);

  /* TEST_MATCH_COUNT(pattern, string, start_position, match_opts, expected_count) */
  TEST_MATCH_COUNT("a", "", 0, 0, 0);
  TEST_MATCH_COUNT("a", "a", 0, 0, 1);
  TEST_MATCH_COUNT("a", "a", 1, 0, 0);
  TEST_MATCH_COUNT("(.)", "a", 0, 0, 2);
  TEST_MATCH_COUNT("(.)", EURO, 0, 0, 2);
  TEST_MATCH_COUNT("(?:.)", "a", 0, 0, 1);
  TEST_MATCH_COUNT("(?P<A>.)", "a", 0, 0, 2);
  TEST_MATCH_COUNT("a$", "a", 0, XREGEX_MATCH_NOTEOL, 0);
  TEST_MATCH_COUNT("(a)?(b)", "b", 0, 0, 3);
  TEST_MATCH_COUNT("(a)?(b)", "ab", 0, 0, 3);

  /* TEST_PARTIAL(pattern, string, expected) */
  TEST_PARTIAL("^ab", "a", TRUE);
  TEST_PARTIAL("^ab", "xa", FALSE);
  TEST_PARTIAL("ab", "xa", TRUE);
  TEST_PARTIAL("ab", "ab", FALSE); /* normal match. */
  TEST_PARTIAL("a+b", "aa", TRUE);
  TEST_PARTIAL("(a)+b", "aa", TRUE);
  TEST_PARTIAL("a?b", "a", TRUE);

  /* Test soft vs. hard partial matching */
  TEST_PARTIAL_FULL("cat(fish)?", "cat", XREGEX_MATCH_PARTIAL_SOFT, FALSE);
  TEST_PARTIAL_FULL("cat(fish)?", "cat", XREGEX_MATCH_PARTIAL_HARD, TRUE);

  /* TEST_SUB_PATTERN(pattern, string, start_position, sub_n, expected_sub,
   * 		      expected_start, expected_end) */
  TEST_SUB_PATTERN("a", "a", 0, 0, "a", 0, 1);
  TEST_SUB_PATTERN("a(.)", "ab", 0, 1, "b", 1, 2);
  TEST_SUB_PATTERN("a(.)", "a" EURO, 0, 1, EURO, 1, 4);
  TEST_SUB_PATTERN("(?:.*)(a)(.)", "xxa" ENG, 0, 2, ENG, 3, 5);
  TEST_SUB_PATTERN("(" HSTROKE ")", "a" HSTROKE ENG, 0, 1, HSTROKE, 1, 3);
  TEST_SUB_PATTERN("a", "a", 0, 1, NULL, UNTOUCHED, UNTOUCHED);
  TEST_SUB_PATTERN("a", "a", 0, 1, NULL, UNTOUCHED, UNTOUCHED);
  TEST_SUB_PATTERN("(a)?(b)", "b", 0, 0, "b", 0, 1);
  TEST_SUB_PATTERN("(a)?(b)", "b", 0, 1, "", -1, -1);
  TEST_SUB_PATTERN("(a)?(b)", "b", 0, 2, "b", 0, 1);
  TEST_SUB_PATTERN("(a)?b", "b", 0, 0, "b", 0, 1);
  TEST_SUB_PATTERN("(a)?b", "b", 0, 1, "", -1, -1);
  TEST_SUB_PATTERN("(a)?b", "b", 0, 2, NULL, UNTOUCHED, UNTOUCHED);

  /* TEST_NAMED_SUB_PATTERN(pattern, string, start_position, sub_name,
   * 			    expected_sub, expected_start, expected_end) */
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "ab", 0, "A", "b", 1, 2);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "aab", 1, "A", "b", 2, 3);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", EURO "ab", 0, "A", "b", 4, 5);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", EURO "ab", 0, "B", "", -1, -1);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", EURO "ab", 0, "C", NULL, UNTOUCHED, UNTOUCHED);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "a" EGRAVE "x", 0, "A", EGRAVE, 1, 3);
  TEST_NAMED_SUB_PATTERN("a(?P<A>.)(?P<B>.)?", "a" EGRAVE "x", 0, "B", "x", 3, 4);
  TEST_NAMED_SUB_PATTERN("(?P<A>a)?(?P<B>b)", "b", 0, "A", "", -1, -1);
  TEST_NAMED_SUB_PATTERN("(?P<A>a)?(?P<B>b)", "b", 0, "B", "b", 0, 1);

  /* TEST_NAMED_SUB_PATTERN_DUPNAMES(pattern, string, start_position, sub_name,
   *				     expected_sub, expected_start, expected_end) */
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>a)|(?P<N>b)", "ab", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>aa)|(?P<N>a)", "aa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>aa)(?P<N>a)", "aaa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>x)|(?P<N>a)", "a", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN_DUPNAMES("(?P<N>x)y|(?P<N>a)b", "ab", 0, "N", "a", 0, 1);

  /* DUPNAMES option inside the pattern */
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>a)|(?P<N>b)", "ab", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>aa)|(?P<N>a)", "aa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>aa)(?P<N>a)", "aaa", 0, "N", "aa", 0, 2);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>x)|(?P<N>a)", "a", 0, "N", "a", 0, 1);
  TEST_NAMED_SUB_PATTERN("(?J)(?P<N>x)y|(?P<N>a)b", "ab", 0, "N", "a", 0, 1);

  /* TEST_FETCH_ALL#(pattern, string, ...) */
  TEST_FETCH_ALL0("a", "");
  TEST_FETCH_ALL0("a", "b");
  TEST_FETCH_ALL1("a", "a", "a");
  TEST_FETCH_ALL1("a+", "aa", "aa");
  TEST_FETCH_ALL1("(?:a)", "a", "a");
  TEST_FETCH_ALL2("(a)", "a", "a", "a");
  TEST_FETCH_ALL2("a(.)", "ab", "ab", "b");
  TEST_FETCH_ALL2("a(.)", "a" HSTROKE, "a" HSTROKE, HSTROKE);
  TEST_FETCH_ALL3("(?:.*)(a)(.)", "xyazk", "xyaz", "a", "z");
  TEST_FETCH_ALL3("(?P<A>.)(a)", "xa", "xa", "x", "a");
  TEST_FETCH_ALL3("(?P<A>.)(a)", ENG "a", ENG "a", ENG, "a");
  TEST_FETCH_ALL3("(a)?(b)", "b", "b", "", "b");
  TEST_FETCH_ALL3("(a)?(b)", "ab", "ab", "a", "b");

  /* TEST_SPLIT_SIMPLE#(pattern, string, ...) */
  TEST_SPLIT_SIMPLE0("", "");
  TEST_SPLIT_SIMPLE0("a", "");
  TEST_SPLIT_SIMPLE1(",", "a", "a");
  TEST_SPLIT_SIMPLE1("(,)\\s*", "a", "a");
  TEST_SPLIT_SIMPLE2(",", "a,b", "a", "b");
  TEST_SPLIT_SIMPLE3(",", "a,b,c", "a", "b", "c");
  TEST_SPLIT_SIMPLE3(",\\s*", "a,b,c", "a", "b", "c");
  TEST_SPLIT_SIMPLE3(",\\s*", "a, b, c", "a", "b", "c");
  TEST_SPLIT_SIMPLE3("(,)\\s*", "a,b", "a", ",", "b");
  TEST_SPLIT_SIMPLE3("(,)\\s*", "a, b", "a", ",", "b");
  TEST_SPLIT_SIMPLE2("\\s", "ab c", "ab", "c");
  TEST_SPLIT_SIMPLE3("\\s*", "ab c", "a", "b", "c");
  /* Not matched sub-strings. */
  TEST_SPLIT_SIMPLE2("a|(b)", "xay", "x", "y");
  TEST_SPLIT_SIMPLE3("a|(b)", "xby", "x", "b", "y");
  /* Empty matches. */
  TEST_SPLIT_SIMPLE3("", "abc", "a", "b", "c");
  TEST_SPLIT_SIMPLE3(" *", "ab c", "a", "b", "c");
  /* Invalid patterns. */
  TEST_SPLIT_SIMPLE0("\\", "");
  TEST_SPLIT_SIMPLE0("[", "");

  /* TEST_SPLIT#(pattern, string, start_position, max_tokens, ...) */
  TEST_SPLIT0("", "", 0, 0);
  TEST_SPLIT0("a", "", 0, 0);
  TEST_SPLIT0("a", "", 0, 1);
  TEST_SPLIT0("a", "", 0, 2);
  TEST_SPLIT0("a", "a", 1, 0);
  TEST_SPLIT1(",", "a", 0, 0, "a");
  TEST_SPLIT1(",", "a,b", 0, 1, "a,b");
  TEST_SPLIT1("(,)\\s*", "a", 0, 0, "a");
  TEST_SPLIT1(",", "a,b", 2, 0, "b");
  TEST_SPLIT2(",", "a,b", 0, 0, "a", "b");
  TEST_SPLIT2(",", "a,b,c", 0, 2, "a", "b,c");
  TEST_SPLIT2(",", "a,b", 1, 0, "", "b");
  TEST_SPLIT2(",", "a,", 0, 0, "a", "");
  TEST_SPLIT3(",", "a,b,c", 0, 0, "a", "b", "c");
  TEST_SPLIT3(",\\s*", "a,b,c", 0, 0, "a", "b", "c");
  TEST_SPLIT3(",\\s*", "a, b, c", 0, 0, "a", "b", "c");
  TEST_SPLIT3("(,)\\s*", "a,b", 0, 0, "a", ",", "b");
  TEST_SPLIT3("(,)\\s*", "a, b", 0, 0, "a", ",", "b");
  /* Not matched sub-strings. */
  TEST_SPLIT2("a|(b)", "xay", 0, 0, "x", "y");
  TEST_SPLIT3("a|(b)", "xby", 0, -1, "x", "b", "y");
  /* Empty matches. */
  TEST_SPLIT2(" *", "ab c", 1, 0, "b", "c");
  TEST_SPLIT3("", "abc", 0, 0, "a", "b", "c");
  TEST_SPLIT3(" *", "ab c", 0, 0, "a", "b", "c");
  TEST_SPLIT1(" *", "ab c", 0, 1, "ab c");
  TEST_SPLIT2(" *", "ab c", 0, 2, "a", "b c");
  TEST_SPLIT3(" *", "ab c", 0, 3, "a", "b", "c");
  TEST_SPLIT3(" *", "ab c", 0, 4, "a", "b", "c");

  /* TEST_CHECK_REPLACEMENT(string_to_expand, expected, expected_refs) */
  TEST_CHECK_REPLACEMENT("", TRUE, FALSE);
  TEST_CHECK_REPLACEMENT("a", TRUE, FALSE);
  TEST_CHECK_REPLACEMENT("\\t\\n\\v\\r\\f\\a\\b\\\\\\x{61}", TRUE, FALSE);
  TEST_CHECK_REPLACEMENT("\\0", TRUE, TRUE);
  TEST_CHECK_REPLACEMENT("\\n\\2", TRUE, TRUE);
  TEST_CHECK_REPLACEMENT("\\g<foo>", TRUE, TRUE);
  /* Invalid strings */
  TEST_CHECK_REPLACEMENT("\\Q", FALSE, FALSE);
  TEST_CHECK_REPLACEMENT("x\\Ay", FALSE, FALSE);

  /* TEST_EXPAND(pattern, string, string_to_expand, raw, expected) */
  TEST_EXPAND("a", "a", "", FALSE, "");
  TEST_EXPAND("a", "a", "\\0", FALSE, "a");
  TEST_EXPAND("a", "a", "\\1", FALSE, "");
  TEST_EXPAND("(a)", "ab", "\\1", FALSE, "a");
  TEST_EXPAND("(a)", "a", "\\1", FALSE, "a");
  TEST_EXPAND("(a)", "a", "\\g<1>", FALSE, "a");
  TEST_EXPAND("a", "a", "\\0130", FALSE, "X");
  TEST_EXPAND("a", "a", "\\\\\\0", FALSE, "\\a");
  TEST_EXPAND("a(?P<G>.)c", "xabcy", "X\\g<G>X", FALSE, "XbX");
#if !(PCRE_MAJOR > 8 || (PCRE_MAJOR == 8 && PCRE_MINOR >= 34))
  /* PCRE >= 8.34 no longer allows this usage. */
  TEST_EXPAND("(.)(?P<1>.)", "ab", "\\1", FALSE, "a");
  TEST_EXPAND("(.)(?P<1>.)", "ab", "\\g<1>", FALSE, "a");
#endif
  TEST_EXPAND(".", EURO, "\\0", FALSE, EURO);
  TEST_EXPAND("(.)", EURO, "\\1", FALSE, EURO);
  TEST_EXPAND("(?P<G>.)", EURO, "\\g<G>", FALSE, EURO);
  TEST_EXPAND(".", "a", EURO, FALSE, EURO);
  TEST_EXPAND(".", "a", EURO "\\0", FALSE, EURO "a");
  TEST_EXPAND(".", "", "\\Lab\\Ec", FALSE, "abc");
  TEST_EXPAND(".", "", "\\LaB\\EC", FALSE, "abC");
  TEST_EXPAND(".", "", "\\Uab\\Ec", FALSE, "ABc");
  TEST_EXPAND(".", "", "a\\ubc", FALSE, "aBc");
  TEST_EXPAND(".", "", "a\\lbc", FALSE, "abc");
  TEST_EXPAND(".", "", "A\\uBC", FALSE, "ABC");
  TEST_EXPAND(".", "", "A\\lBC", FALSE, "AbC");
  TEST_EXPAND(".", "", "A\\l\\\\BC", FALSE, "A\\BC");
  TEST_EXPAND(".", "", "\\L" AGRAVE "\\E", FALSE, AGRAVE);
  TEST_EXPAND(".", "", "\\U" AGRAVE "\\E", FALSE, AGRAVE_UPPER);
  TEST_EXPAND(".", "", "\\u" AGRAVE "a", FALSE, AGRAVE_UPPER "a");
  TEST_EXPAND(".", "ab", "x\\U\\0y\\Ez", FALSE, "xAYz");
  TEST_EXPAND(".(.)", "AB", "x\\L\\1y\\Ez", FALSE, "xbyz");
  TEST_EXPAND(".", "ab", "x\\u\\0y\\Ez", FALSE, "xAyz");
  TEST_EXPAND(".(.)", "AB", "x\\l\\1y\\Ez", FALSE, "xbyz");
  TEST_EXPAND(".(.)", "a" AGRAVE_UPPER, "x\\l\\1y", FALSE, "x" AGRAVE "y");
  TEST_EXPAND("a", "bab", "\\x{61}", FALSE, "a");
  TEST_EXPAND("a", "bab", "\\x61", FALSE, "a");
  TEST_EXPAND("a", "bab", "\\x5a", FALSE, "Z");
  TEST_EXPAND("a", "bab", "\\0\\x5A", FALSE, "aZ");
  TEST_EXPAND("a", "bab", "\\1\\x{5A}", FALSE, "Z");
  TEST_EXPAND("a", "bab", "\\x{00E0}", FALSE, AGRAVE);
  TEST_EXPAND("", "bab", "\\x{0634}", FALSE, SHEEN);
  TEST_EXPAND("", "bab", "\\x{634}", FALSE, SHEEN);
  TEST_EXPAND("", "", "\\t", FALSE, "\t");
  TEST_EXPAND("", "", "\\v", FALSE, "\v");
  TEST_EXPAND("", "", "\\r", FALSE, "\r");
  TEST_EXPAND("", "", "\\n", FALSE, "\n");
  TEST_EXPAND("", "", "\\f", FALSE, "\f");
  TEST_EXPAND("", "", "\\a", FALSE, "\a");
  TEST_EXPAND("", "", "\\b", FALSE, "\b");
  TEST_EXPAND("a(.)", "abc", "\\0\\b\\1", FALSE, "ab\bb");
  TEST_EXPAND("a(.)", "abc", "\\0141", FALSE, "a");
  TEST_EXPAND("a(.)", "abc", "\\078", FALSE, "\a8");
  TEST_EXPAND("a(.)", "abc", "\\077", FALSE, "?");
  TEST_EXPAND("a(.)", "abc", "\\0778", FALSE, "?8");
  TEST_EXPAND("a(.)", "a" AGRAVE "b", "\\1", FALSE, AGRAVE);
  TEST_EXPAND("a(.)", "a" AGRAVE "b", "\\1", TRUE, "\xc3");
  TEST_EXPAND("a(.)", "a" AGRAVE "b", "\\0", TRUE, "a\xc3");
  /* Invalid strings. */
  TEST_EXPAND("", "", "\\Q", FALSE, NULL);
  TEST_EXPAND("", "", "x\\Ay", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<>", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<1a>", FALSE, NULL);
  TEST_EXPAND("", "", "\\g<a$>", FALSE, NULL);
  TEST_EXPAND("", "", "\\", FALSE, NULL);
  TEST_EXPAND("a", "a", "\\x{61", FALSE, NULL);
  TEST_EXPAND("a", "a", "\\x6X", FALSE, NULL);
  /* Pattern-less. */
  TEST_EXPAND(NULL, NULL, "", FALSE, "");
  TEST_EXPAND(NULL, NULL, "\\n", FALSE, "\n");
  /* Invalid strings */
  TEST_EXPAND(NULL, NULL, "\\Q", FALSE, NULL);
  TEST_EXPAND(NULL, NULL, "x\\Ay", FALSE, NULL);

  /* TEST_REPLACE(pattern, string, start_position, replacement, expected) */
  TEST_REPLACE("a", "ababa", 0, "A", "AbAbA");
  TEST_REPLACE("a", "ababa", 1, "A", "abAbA");
  TEST_REPLACE("a", "ababa", 2, "A", "abAbA");
  TEST_REPLACE("a", "ababa", 3, "A", "ababA");
  TEST_REPLACE("a", "ababa", 4, "A", "ababA");
  TEST_REPLACE("a", "ababa", 5, "A", "ababa");
  TEST_REPLACE("a", "ababa", 6, "A", "ababa");
  TEST_REPLACE("a", "abababa", 2, "A", "abAbAbA");
  TEST_REPLACE("a", "abab", 0, "A", "AbAb");
  TEST_REPLACE("a", "baba", 0, "A", "bAbA");
  TEST_REPLACE("a", "bab", 0, "A", "bAb");
  TEST_REPLACE("$^", "abc", 0, "X", "abc");
  TEST_REPLACE("(.)a", "ciao", 0, "a\\1", "caio");
  TEST_REPLACE("a.", "abc", 0, "\\0\\0", "ababc");
  TEST_REPLACE("a", "asd", 0, "\\0101", "Asd");
  TEST_REPLACE("(a).\\1", "aba cda", 0, "\\1\\n", "a\n cda");
  TEST_REPLACE("a" AGRAVE "a", "a" AGRAVE "a", 0, "x", "x");
  TEST_REPLACE("a" AGRAVE "a", "a" AGRAVE "a", 0, OGRAVE, OGRAVE);
  TEST_REPLACE("[^-]", "-" EURO "-x-" HSTROKE, 0, "a", "-a-a-a");
  TEST_REPLACE("[^-]", "-" EURO "-" HSTROKE, 0, "a\\g<0>a", "-a" EURO "a-a" HSTROKE "a");
  TEST_REPLACE("-", "-" EURO "-" HSTROKE, 0, "", EURO HSTROKE);
  TEST_REPLACE(".*", "hello", 0, "\\U\\0\\E", "HELLO");
  TEST_REPLACE(".*", "hello", 0, "\\u\\0", "Hello");
  TEST_REPLACE("\\S+", "hello world", 0, "\\U-\\0-", "-HELLO- -WORLD-");
  TEST_REPLACE(".", "a", 0, "\\A", NULL);
  TEST_REPLACE(".", "a", 0, "\\g", NULL);

  /* TEST_REPLACE_LIT(pattern, string, start_position, replacement, expected) */
  TEST_REPLACE_LIT("a", "ababa", 0, "A", "AbAbA");
  TEST_REPLACE_LIT("a", "ababa", 1, "A", "abAbA");
  TEST_REPLACE_LIT("a", "ababa", 2, "A", "abAbA");
  TEST_REPLACE_LIT("a", "ababa", 3, "A", "ababA");
  TEST_REPLACE_LIT("a", "ababa", 4, "A", "ababA");
  TEST_REPLACE_LIT("a", "ababa", 5, "A", "ababa");
  TEST_REPLACE_LIT("a", "ababa", 6, "A", "ababa");
  TEST_REPLACE_LIT("a", "abababa", 2, "A", "abAbAbA");
  TEST_REPLACE_LIT("a", "abcadaa", 0, "A", "AbcAdAA");
  TEST_REPLACE_LIT("$^", "abc", 0, "X", "abc");
  TEST_REPLACE_LIT("(.)a", "ciao", 0, "a\\1", "ca\\1o");
  TEST_REPLACE_LIT("a.", "abc", 0, "\\0\\0\\n", "\\0\\0\\nc");
  TEST_REPLACE_LIT("a" AGRAVE "a", "a" AGRAVE "a", 0, "x", "x");
  TEST_REPLACE_LIT("a" AGRAVE "a", "a" AGRAVE "a", 0, OGRAVE, OGRAVE);
  TEST_REPLACE_LIT(AGRAVE, "-" AGRAVE "-" HSTROKE, 0, "a" ENG "a", "-a" ENG "a-" HSTROKE);
  TEST_REPLACE_LIT("[^-]", "-" EURO "-" AGRAVE "-" HSTROKE, 0, "a", "-a-a-a");
  TEST_REPLACE_LIT("[^-]", "-" EURO "-" AGRAVE, 0, "a\\g<0>a", "-a\\g<0>a-a\\g<0>a");
  TEST_REPLACE_LIT("-", "-" EURO "-" AGRAVE "-" HSTROKE, 0, "", EURO AGRAVE HSTROKE);
  TEST_REPLACE_LIT("(?=[A-Z0-9])", "RegExTest", 0, "_", "_Reg_Ex_Test");
  TEST_REPLACE_LIT("(?=[A-Z0-9])", "RegExTest", 1, "_", "Reg_Ex_Test");

  /* TEST_GET_STRING_NUMBER(pattern, name, expected_num) */
  TEST_GET_STRING_NUMBER("", "A", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)", "B", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)", "B", 2);
  TEST_GET_STRING_NUMBER("(?P<A>.)(?P<B>a)", "C", -1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(.)(?P<B>a)", "A", 1);
  TEST_GET_STRING_NUMBER("(?P<A>.)(.)(?P<B>a)", "B", 3);
  TEST_GET_STRING_NUMBER("(?P<A>.)(.)(?P<B>a)", "C", -1);
  TEST_GET_STRING_NUMBER("(?:a)(?P<A>.)", "A", 1);
  TEST_GET_STRING_NUMBER("(?:a)(?P<A>.)", "B", -1);

  /* TEST_ESCAPE_NUL(string, length, expected) */
  TEST_ESCAPE_NUL("hello world", -1, "hello world");
  TEST_ESCAPE_NUL("hello\0world", -1, "hello");
  TEST_ESCAPE_NUL("\0world", -1, "");
  TEST_ESCAPE_NUL("hello world", 5, "hello");
  TEST_ESCAPE_NUL("hello.world", 11, "hello.world");
  TEST_ESCAPE_NUL("a(b\\b.$", 7, "a(b\\b.$");
  TEST_ESCAPE_NUL("hello\0", 6, "hello\\x00");
  TEST_ESCAPE_NUL("\0world", 6, "\\x00world");
  TEST_ESCAPE_NUL("\0\0", 2, "\\x00\\x00");
  TEST_ESCAPE_NUL("hello\0world", 11, "hello\\x00world");
  TEST_ESCAPE_NUL("hello\0world\0", 12, "hello\\x00world\\x00");
  TEST_ESCAPE_NUL("hello\\\0world", 12, "hello\\x00world");
  TEST_ESCAPE_NUL("hello\\\\\0world", 13, "hello\\\\\\x00world");
  TEST_ESCAPE_NUL("|()[]{}^$*+?.", 13, "|()[]{}^$*+?.");
  TEST_ESCAPE_NUL("|()[]{}^$*+?.\\\\", 15, "|()[]{}^$*+?.\\\\");

  /* TEST_ESCAPE(string, length, expected) */
  TEST_ESCAPE("hello world", -1, "hello world");
  TEST_ESCAPE("hello world", 5, "hello");
  TEST_ESCAPE("hello.world", -1, "hello\\.world");
  TEST_ESCAPE("a(b\\b.$", -1, "a\\(b\\\\b\\.\\$");
  TEST_ESCAPE("hello\0world", -1, "hello");
  TEST_ESCAPE("hello\0world", 11, "hello\\0world");
  TEST_ESCAPE(EURO "*" ENG, -1, EURO "\\*" ENG);
  TEST_ESCAPE("a$", -1, "a\\$");
  TEST_ESCAPE("$a", -1, "\\$a");
  TEST_ESCAPE("a$a", -1, "a\\$a");
  TEST_ESCAPE("$a$", -1, "\\$a\\$");
  TEST_ESCAPE("$a$", 0, "");
  TEST_ESCAPE("$a$", 1, "\\$");
  TEST_ESCAPE("$a$", 2, "\\$a");
  TEST_ESCAPE("$a$", 3, "\\$a\\$");
  TEST_ESCAPE("$a$", 4, "\\$a\\$\\0");
  TEST_ESCAPE("|()[]{}^$*+?.", -1, "\\|\\(\\)\\[\\]\\{\\}\\^\\$\\*\\+\\?\\.");
  TEST_ESCAPE("a|a(a)a[a]a{a}a^a$a*a+a?a.a", -1,
	      "a\\|a\\(a\\)a\\[a\\]a\\{a\\}a\\^a\\$a\\*a\\+a\\?a\\.a");

  /* TEST_MATCH_ALL#(pattern, string, string_len, start_position, ...) */
  TEST_MATCH_ALL0("<.*>", "", -1, 0);
  TEST_MATCH_ALL0("a+", "", -1, 0);
  TEST_MATCH_ALL0("a+", "a", 0, 0);
  TEST_MATCH_ALL0("a+", "a", -1, 1);
  TEST_MATCH_ALL1("<.*>", "<a>", -1, 0, "<a>", 0, 3);
  TEST_MATCH_ALL1("a+", "a", -1, 0, "a", 0, 1);
  TEST_MATCH_ALL1("a+", "aa", 1, 0, "a", 0, 1);
  TEST_MATCH_ALL1("a+", "aa", -1, 1, "a", 1, 2);
  TEST_MATCH_ALL1("a+", "aa", 2, 1, "a", 1, 2);
  TEST_MATCH_ALL1(".+", ENG, -1, 0, ENG, 0, 2);
  TEST_MATCH_ALL2("<.*>", "<a><b>", -1, 0, "<a><b>", 0, 6, "<a>", 0, 3);
  TEST_MATCH_ALL2("a+", "aa", -1, 0, "aa", 0, 2, "a", 0, 1);
  TEST_MATCH_ALL2(".+", ENG EURO, -1, 0, ENG EURO, 0, 5, ENG, 0, 2);
  TEST_MATCH_ALL3("<.*>", "<a><b><c>", -1, 0, "<a><b><c>", 0, 9,
		  "<a><b>", 0, 6, "<a>", 0, 3);
  TEST_MATCH_ALL3("a+", "aaa", -1, 0, "aaa", 0, 3, "aa", 0, 2, "a", 0, 1);

  /* NOTEMPTY matching */
  TEST_MATCH_NOTEMPTY("a?b?", "xyz", FALSE);
  TEST_MATCH_NOTEMPTY_ATSTART("a?b?", "xyz", TRUE);

  return g_test_run ();
}
