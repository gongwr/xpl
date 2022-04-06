/* Unit tests for gstrfuncs
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

#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glib.h"

#if defined (_MSC_VER) && (_MSC_VER <= 1800)
#define isnan(x) _isnan(x)

#ifndef NAN
static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
#define NAN (*(const float *) __nan)
#endif

#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif

#endif

#define XPL_TEST_STRING "el dorado "

#define FOR_ALL_CTYPE(macro)	\
	macro(isalnum)		\
	macro(isalpha)		\
	macro(iscntrl)		\
	macro(isdigit)		\
	macro(isgraph)		\
	macro(islower)		\
	macro(isprint)		\
	macro(ispunct)		\
	macro(isspace)		\
	macro(isupper)		\
	macro(isxdigit)

#define DEFINE_CALL_CTYPE(function)		\
	static int				\
	call_##function (int c)			\
	{					\
		return function (c);		\
	}

#define DEFINE_CALL_G_ASCII_CTYPE(function)	\
	static xboolean_t				\
	call_g_ascii_##function (xchar_t c)	\
	{					\
		return g_ascii_##function (c);	\
	}

FOR_ALL_CTYPE (DEFINE_CALL_CTYPE)
FOR_ALL_CTYPE (DEFINE_CALL_G_ASCII_CTYPE)

static void
test_is_function (const char *name,
		  xboolean_t (* ascii_function) (xchar_t),
		  int (* c_library_function) (int),
		  xboolean_t (* unicode_function) (xunichar_t))
{
  int c;

  for (c = 0; c <= 0x7F; c++)
    {
      xboolean_t ascii_result = ascii_function ((xchar_t)c);
      xboolean_t c_library_result = c_library_function (c) != 0;
      xboolean_t unicode_result = unicode_function ((xunichar_t) c);
      if (ascii_result != c_library_result && c != '\v')
        {
	  xerror ("g_ascii_%s returned %d and %s returned %d for 0x%X",
		   name, ascii_result, name, c_library_result, c);
	}
      if (ascii_result != unicode_result)
	{
	  xerror ("g_ascii_%s returned %d and xunichar_%s returned %d for 0x%X",
		   name, ascii_result, name, unicode_result, c);
	}
    }
  for (c = 0x80; c <= 0xFF; c++)
    {
      xboolean_t ascii_result = ascii_function ((xchar_t)c);
      if (ascii_result)
	{
	  xerror ("g_ascii_%s returned TRUE for 0x%X", name, c);
	}
    }
}

static void
test_to_function (const char *name,
		  xchar_t (* ascii_function) (xchar_t),
		  int (* c_library_function) (int),
		  xunichar_t (* unicode_function) (xunichar_t))
{
  int c;

  for (c = 0; c <= 0x7F; c++)
    {
      int ascii_result = (xuchar_t) ascii_function ((xchar_t) c);
      int c_library_result = c_library_function (c);
      int unicode_result = unicode_function ((xunichar_t) c);
      if (ascii_result != c_library_result)
	{
	  xerror ("g_ascii_%s returned 0x%X and %s returned 0x%X for 0x%X",
		   name, ascii_result, name, c_library_result, c);
	}
      if (ascii_result != unicode_result)
	{
	  xerror ("g_ascii_%s returned 0x%X and xunichar_%s returned 0x%X for 0x%X",
		   name, ascii_result, name, unicode_result, c);
	}
    }
  for (c = 0x80; c <= 0xFF; c++)
    {
      int ascii_result = (xuchar_t) ascii_function ((xchar_t) c);
      if (ascii_result != c)
	{
	  xerror ("g_ascii_%s returned 0x%X for 0x%X",
		   name, ascii_result, c);
	}
    }
}

static void
test_digit_function (const char *name,
		     int (* ascii_function) (xchar_t),
		     int (* unicode_function) (xunichar_t))
{
  int c;

  for (c = 0; c <= 0x7F; c++)
    {
      int ascii_result = ascii_function ((xchar_t) c);
      int unicode_result = unicode_function ((xunichar_t) c);
      if (ascii_result != unicode_result)
	{
	  xerror ("g_ascii_%s_value returned %d and xunichar_%s_value returned %d for 0x%X",
		   name, ascii_result, name, unicode_result, c);
	}
    }
  for (c = 0x80; c <= 0xFF; c++)
    {
      int ascii_result = ascii_function ((xchar_t) c);
      if (ascii_result != -1)
	{
	  xerror ("g_ascii_%s_value returned %d for 0x%X",
		   name, ascii_result, c);
	}
    }
}

static void
test_is_to_digit (void)
{
  #define TEST_IS(name) test_is_function (#name, call_g_ascii_##name, call_##name, xunichar_##name);

  FOR_ALL_CTYPE(TEST_IS)

  #undef TEST_IS

  #define TEST_TO(name) test_to_function (#name, g_ascii_##name, name, xunichar_##name)

  TEST_TO (tolower);
  TEST_TO (toupper);

  #undef TEST_TO

  #define TEST_DIGIT(name) test_digit_function (#name, g_ascii_##name##_value, xunichar_##name##_value)

  TEST_DIGIT (digit);
  TEST_DIGIT (xdigit);

  #undef TEST_DIGIT
}

/* Testing g_memdup() function with various positive and negative cases */
static void
test_memdup (void)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  xchar_t *str_dup = NULL;
  const xchar_t *str = "The quick brown fox jumps over the lazy dog";

  /* Testing negative cases */
  g_assert_null (g_memdup (NULL, 1024));
  g_assert_null (g_memdup (str, 0));
  g_assert_null (g_memdup (NULL, 0));

  /* Testing normal usage cases */
  str_dup = g_memdup (str, strlen (str) + 1);
  g_assert_nonnull (str_dup);
  g_assert_cmpstr (str, ==, str_dup);

  g_free (str_dup);

  G_GNUC_END_IGNORE_DEPRECATIONS
}

/* Testing g_memdup2() function with various positive and negative cases */
static void
test_memdup2 (void)
{
  xchar_t *str_dup = NULL;
  const xchar_t *str = "The quick brown fox jumps over the lazy dog";

  /* Testing negative cases */
  g_assert_null (g_memdup2 (NULL, 1024));
  g_assert_null (g_memdup2 (str, 0));
  g_assert_null (g_memdup2 (NULL, 0));

  /* Testing normal usage cases */
  str_dup = g_memdup2 (str, strlen (str) + 1);
  g_assert_nonnull (str_dup);
  g_assert_cmpstr (str, ==, str_dup);

  g_free (str_dup);
}

/* Testing xstrpcpy() function with various positive and negative cases */
static void
test_stpcpy (void)
{
  xchar_t *str = "The quick brown fox jumps over the lazy dog";
  xchar_t str_cpy[45], *str_cpy_end = NULL;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_cpy_end = g_stpcpy (str_cpy, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_cpy_end = g_stpcpy (NULL, str);
      g_test_assert_expected_messages ();
    }

  /* Testing normal usage cases */
  str_cpy_end = g_stpcpy (str_cpy, str);
  g_assert_nonnull (str_cpy);
  g_assert_true (str_cpy + strlen (str) == str_cpy_end);
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpstr (str, ==, str_cpy_end - strlen (str));
}

/* Testing xstrlcpy() function with various positive and negative cases */
static void
test_strlcpy (void)
{
  xchar_t *str = "The quick brown fox jumps over the lazy dog";
  xchar_t str_cpy[45];
  xsize_t str_cpy_size = 0;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_cpy_size = xstrlcpy (str_cpy, NULL, 0);
      g_test_assert_expected_messages ();
      /* Returned 0 because xstrlcpy() failed */
      g_assert_cmpint (str_cpy_size, ==, 0);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_cpy_size = xstrlcpy (NULL, str, 0);
      g_test_assert_expected_messages ();
      /* Returned 0 because xstrlcpy() failed */
      g_assert_cmpint (str_cpy_size, ==, 0);
    }

  str_cpy_size = xstrlcpy (str_cpy, "", 0);
  g_assert_cmpint (str_cpy_size, ==, strlen (""));

  /* Testing normal usage cases.
   * Note that the @dest_size argument to xstrlcpy() is normally meant to be
   * set to `sizeof (dest)`. We set it to various values `≤ sizeof (str_cpy)`
   * for testing purposes.  */
  str_cpy_size = xstrlcpy (str_cpy, str, strlen (str) + 1);
  g_assert_nonnull (str_cpy);
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));

  str_cpy_size = xstrlcpy (str_cpy, str, strlen (str));
  g_assert_nonnull (str_cpy);
  g_assert_cmpstr ("The quick brown fox jumps over the lazy do", ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));

  str_cpy_size = xstrlcpy (str_cpy, str, strlen (str) - 15);
  g_assert_nonnull (str_cpy);
  g_assert_cmpstr ("The quick brown fox jumps o", ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));

  str_cpy_size = xstrlcpy (str_cpy, str, 0);
  g_assert_nonnull (str_cpy);
  g_assert_cmpstr ("The quick brown fox jumps o", ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));

  str_cpy_size = xstrlcpy (str_cpy, str, strlen (str) + 15);
  g_assert_nonnull (str_cpy);
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));
}

/* Testing xstrlcat() function with various positive and negative cases */
static void
test_strlcat (void)
{
  xchar_t *str = "The quick brown fox jumps over the lazy dog";
  xchar_t str_cpy[60] = { 0 };
  xsize_t str_cpy_size = 0;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_cpy_size = xstrlcat (str_cpy, NULL, 0);
      g_test_assert_expected_messages ();
      /* Returned 0 because xstrlcpy() failed */
      g_assert_cmpint (str_cpy_size, ==, 0);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_cpy_size = xstrlcat (NULL, str, 0);
      g_test_assert_expected_messages ();
      /* Returned 0 because xstrlcpy() failed */
      g_assert_cmpint (str_cpy_size, ==, 0);
    }

  str_cpy_size = xstrlcat (str_cpy, "", 0);
  g_assert_cmpint (str_cpy_size, ==, strlen (""));

  /* Testing normal usage cases.
   * Note that the @dest_size argument to xstrlcat() is normally meant to be
   * set to `sizeof (dest)`. We set it to various values `≤ sizeof (str_cpy)`
   * for testing purposes. */
  g_assert_cmpuint (strlen (str) + 1, <=, sizeof (str_cpy));
  str_cpy_size = xstrlcat (str_cpy, str, strlen (str) + 1);
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));

  g_assert_cmpuint (strlen (str), <=, sizeof (str_cpy));
  str_cpy_size = xstrlcat (str_cpy, str, strlen (str));
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, 2 * strlen (str));

  g_assert_cmpuint (strlen (str) - 15, <=, sizeof (str_cpy));
  str_cpy_size = xstrlcat (str_cpy, str, strlen (str) - 15);
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, 2 * strlen (str) - 15);

  g_assert_cmpuint (0, <=, sizeof (str_cpy));
  str_cpy_size = xstrlcat (str_cpy, str, 0);
  g_assert_cmpstr (str, ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, strlen (str));

  g_assert_cmpuint (strlen (str) + 15, <=, sizeof (str_cpy));
  str_cpy_size = xstrlcat (str_cpy, str, strlen (str) + 15);
  g_assert_cmpstr ("The quick brown fox jumps over the lazy dogThe quick brow",
                   ==, str_cpy);
  g_assert_cmpint (str_cpy_size, ==, 2 * strlen (str));
}

/* Testing g_ascii_strdown() function with various positive and negative cases */
static void
test_ascii_strdown (void)
{
  const xchar_t *str_down = "the quick brown fox jumps over the lazy dog.";
  const xchar_t *str_up = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.";
  xchar_t* str;

  if (g_test_undefined ())
    {
  /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = g_ascii_strdown (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str = g_ascii_strdown ("", 0);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);

  str = g_ascii_strdown ("", -1);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);

  /* Testing normal usage cases */
  str = g_ascii_strdown (str_down, strlen (str_down));
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, str_down);
  g_free (str);

  str = g_ascii_strdown (str_up, strlen (str_up));
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, str_down);
  g_free (str);

  str = g_ascii_strdown (str_up, -1);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, str_down);
  g_free (str);

  str = g_ascii_strdown (str_up, 0);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);
}

/* Testing g_ascii_strup() function with various positive and negative cases */
static void
test_ascii_strup (void)
{
  const xchar_t *str_down = "the quick brown fox jumps over the lazy dog.";
  const xchar_t *str_up = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.";
  xchar_t* str;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = g_ascii_strup (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str = g_ascii_strup ("", 0);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);

  str = g_ascii_strup ("", -1);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);

  /* Testing normal usage cases */
  str = g_ascii_strup (str_up, strlen (str_up));
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, str_up);
  g_free (str);

  str = g_ascii_strup (str_down, strlen (str_down));
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, str_up);
  g_free (str);

  str = g_ascii_strup (str_down, -1);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, str_up);
  g_free (str);

  str = g_ascii_strup (str_down, 0);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);
}

/* Testing xstrdup() function with various positive and negative cases */
static void
test_strdup (void)
{
  xchar_t *str;

  g_assert_null (xstrdup (NULL));

  str = xstrdup (XPL_TEST_STRING);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, XPL_TEST_STRING);
  g_free (str);
}

/* Testing xstrndup() function with various positive and negative cases */
static void
test_strndup (void)
{
  xchar_t *str;

  str = xstrndup (NULL, 3);
  g_assert_null (str);

  str = xstrndup ("aaaa", 5);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "aaaa");
  g_free (str);

  str = xstrndup ("aaaa", 2);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "aa");
  g_free (str);
}

/* Testing xstrdup_printf() function with various positive and negative cases */
static void
test_strdup_printf (void)
{
  xchar_t *str;

  str = xstrdup_printf ("%05d %-5s", 21, "test");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "00021 test ");
  g_free (str);
}

/* Testing xstrdupv() function with various positive and negative cases */
static void
test_strdupv (void)
{
  xchar_t *vec[] = { "foo_t", "Bar", NULL };
  xchar_t **copy;

  copy = xstrdupv (NULL);
  g_assert_null (copy);

  copy = xstrdupv (vec);
  g_assert_nonnull (copy);
  g_assert_cmpstrv (copy, vec);
  xstrfreev (copy);
}

/* Testing xstrfill() function with various positive and negative cases */
static void
test_strnfill (void)
{
  xchar_t *str;

  str = xstrnfill (0, 'a');
  g_assert_nonnull (str);
  g_assert_true (*str == '\0');
  g_free (str);

  str = xstrnfill (5, 'a');
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "aaaaa");
  g_free (str);
}

/* Testing xstrconcat() function with various positive and negative cases */
static void
test_strconcat (void)
{
  xchar_t *str;

  str = xstrconcat (XPL_TEST_STRING, NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, XPL_TEST_STRING);
  g_free (str);

  str = xstrconcat (XPL_TEST_STRING,
		     XPL_TEST_STRING,
		     XPL_TEST_STRING,
		     NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, XPL_TEST_STRING XPL_TEST_STRING XPL_TEST_STRING);
  g_free (str);

  g_assert_null (xstrconcat (NULL, "bla", NULL));
}

/* Testing xstrjoinv() function with various positive and negative cases */
static void
test_strjoinv (void)
{
  xchar_t *strings[] = { "string1", "string2", NULL };
  xchar_t *empty_strings[] = { NULL };
  xchar_t *str;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = xstrjoinv (NULL, NULL);
      g_test_assert_expected_messages ();
    }

  str = xstrjoinv (":", strings);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "string1:string2");
  g_free (str);

  str = xstrjoinv (NULL, strings);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "string1string2");
  g_free (str);

  str = xstrjoinv (NULL, empty_strings);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "");
  g_free (str);
}

/* Testing xstrjoin() function with various positive and negative cases */
static void
test_strjoin (void)
{
  xchar_t *str;

  str = xstrjoin (NULL, NULL);
  g_assert_nonnull (str);
  g_assert_true (*str == '\0');
  g_free (str);

  str = xstrjoin (":", NULL);
  g_assert_nonnull (str);
  g_assert_true (*str == '\0');
  g_free (str);

  str = xstrjoin (NULL, XPL_TEST_STRING, NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, XPL_TEST_STRING);
  g_free (str);

  str = xstrjoin (NULL,
		   XPL_TEST_STRING,
		   XPL_TEST_STRING,
		   XPL_TEST_STRING,
		   NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, XPL_TEST_STRING XPL_TEST_STRING XPL_TEST_STRING);
  g_free (str);

  str = xstrjoin (":",
		   XPL_TEST_STRING,
		   XPL_TEST_STRING,
		   XPL_TEST_STRING,
		   NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, XPL_TEST_STRING ":" XPL_TEST_STRING ":" XPL_TEST_STRING);
  g_free (str);
}

/* Testing xstrcanon() function with various positive and negative cases */
static void
test_strcanon (void)
{
  xchar_t *str;

  if (g_test_undefined ())
    {
      xchar_t *ret;

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = xstrcanon (NULL, "ab", 'y');
      g_test_assert_expected_messages ();
      g_assert_null (str);

      str = xstrdup ("abxabxab");
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      ret = xstrcanon (str, NULL, 'y');
      g_test_assert_expected_messages ();
      g_assert_null (ret);
      g_free (str);
    }

  str = xstrdup ("abxabxab");
  str = xstrcanon (str, "ab", 'y');
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "abyabyab");
  g_free (str);
}

/* Testing xstrcompress() and xstrescape() functions with various cases */
static void
test_strcompress_strescape (void)
{
  xchar_t *str;
  xchar_t *tmp;

  /* test compress */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = xstrcompress (NULL);
      g_test_assert_expected_messages ();
      g_assert_null (str);

      /* trailing slashes are not allowed */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                             "*trailing \\*");
      str = xstrcompress ("abc\\");
      g_test_assert_expected_messages ();
      g_assert_cmpstr (str, ==, "abc");
      g_free (str);
    }

  str = xstrcompress ("abc\\\\\\\"\\b\\f\\n\\r\\t\\v\\003\\177\\234\\313\\12345z");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "abc\\\"\b\f\n\r\t\v\003\177\234\313\12345z");
  g_free (str);

  /* test escape */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = xstrescape (NULL, NULL);
      g_test_assert_expected_messages ();
      g_assert_null (str);
    }

  str = xstrescape ("abc\\\"\b\f\n\r\t\v\003\177\234\313", NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "abc\\\\\\\"\\b\\f\\n\\r\\t\\v\\003\\177\\234\\313");
  g_free (str);

  str = xstrescape ("abc\\\"\b\f\n\r\t\v\003\177\234\313",
		     "\b\f\001\002\003\004");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "abc\\\\\\\"\b\f\\n\\r\\t\\v\003\\177\\234\\313");
  g_free (str);

  /* round trip */
  tmp = xstrescape ("abc\\\"\b\f\n\r\t\v\003\177\234\313", NULL);
  str = xstrcompress (tmp);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "abc\\\"\b\f\n\r\t\v\003\177\234\313");
  g_free (str);
  g_free (tmp);

  /* Unicode round trip */
  str = xstrescape ("héllø there⸘", NULL);
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "h\\303\\251ll\\303\\270 there\\342\\270\\230");
  tmp = xstrcompress (str);
  g_assert_nonnull (tmp);
  g_assert_cmpstr (tmp, ==, "héllø there⸘");
  g_free (tmp);
  g_free (str);

  /* test_t expanding invalid escapes */
  str = xstrcompress ("\\11/ \\118 \\8aa \\19");
  g_assert_nonnull (str);
  g_assert_cmpstr (str, ==, "\t/ \t8 8aa \0019");
  g_free (str);
}

/* Testing g_ascii_strcasecmp() and g_ascii_strncasecmp() */
static void
test_ascii_strcasecmp (void)
{
  xboolean_t res;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = g_ascii_strcasecmp ("foo", NULL);
      g_test_assert_expected_messages ();
      g_assert_false (res);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = g_ascii_strcasecmp (NULL, "foo");
      g_test_assert_expected_messages ();
      g_assert_false (res);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = g_ascii_strncasecmp ("foo", NULL, 0);
      g_test_assert_expected_messages ();
      g_assert_false (res);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = g_ascii_strncasecmp (NULL, "foo", 0);
      g_test_assert_expected_messages ();
      g_assert_false (res);
    }

  res = g_ascii_strcasecmp ("FroboZZ", "frobozz");
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strcasecmp ("frobozz", "frobozz");
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strcasecmp ("frobozz", "FROBOZZ");
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strcasecmp ("FROBOZZ", "froboz");
  g_assert_cmpint (res, !=, 0);

  res = g_ascii_strcasecmp ("", "");
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strcasecmp ("!#%&/()", "!#%&/()");
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strcasecmp ("a", "b");
  g_assert_cmpint (res, <, 0);

  res = g_ascii_strcasecmp ("a", "B");
  g_assert_cmpint (res, <, 0);

  res = g_ascii_strcasecmp ("A", "b");
  g_assert_cmpint (res, <, 0);

  res = g_ascii_strcasecmp ("A", "B");
  g_assert_cmpint (res, <, 0);

  res = g_ascii_strcasecmp ("b", "a");
  g_assert_cmpint (res, >, 0);

  res = g_ascii_strcasecmp ("b", "A");
  g_assert_cmpint (res, >, 0);

  res = g_ascii_strcasecmp ("B", "a");
  g_assert_cmpint (res, >, 0);

  res = g_ascii_strcasecmp ("B", "A");
  g_assert_cmpint (res, >, 0);

  /* g_ascii_strncasecmp() */
  res = g_ascii_strncasecmp ("", "", 10);
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strncasecmp ("Frob0ZZ", "frob0zz", strlen ("frobozz"));
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strncasecmp ("Frob0ZZ", "frobozz", strlen ("frobozz"));
  g_assert_cmpint (res, !=, 0);

  res = g_ascii_strncasecmp ("frob0ZZ", "FroB0zz", strlen ("frobozz"));
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strncasecmp ("Frob0ZZ", "froB0zz", strlen ("frobozz") - 5);
  g_assert_cmpint (res, ==, 0);

  res = g_ascii_strncasecmp ("Frob0ZZ", "froB0zz", strlen ("frobozz") + 5);
  g_assert_cmpint (res, ==, 0);
}

static void
do_test_strchug (const xchar_t *str, const xchar_t *expected)
{
  xchar_t *tmp;
  xboolean_t res;

  tmp = xstrdup (str);

  xstrchug (tmp);
  res = (strcmp (tmp, expected) == 0);
  g_free (tmp);

  g_assert_cmpint (res, ==, TRUE);
}

/* Testing xstrchug() function with various positive and negative cases */
static void
test_strchug (void)
{
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      xstrchug (NULL);
      g_test_assert_expected_messages ();
    }

  do_test_strchug ("", "");
  do_test_strchug (" ", "");
  do_test_strchug ("\t\r\n ", "");
  do_test_strchug (" a", "a");
  do_test_strchug ("  a", "a");
  do_test_strchug ("a a", "a a");
  do_test_strchug (" a a", "a a");
}

static void
do_test_strchomp (const xchar_t *str, const xchar_t *expected)
{
  xchar_t *tmp;
  xboolean_t res;

  tmp = xstrdup (str);

  xstrchomp (tmp);
  res = (strcmp (tmp, expected) == 0);
  g_free (tmp);

  g_assert_cmpint (res, ==, TRUE);
}

/* Testing xstrchomp() function with various positive and negative cases */
static void
test_strchomp (void)
{
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      xstrchomp (NULL);
      g_test_assert_expected_messages ();
    }

  do_test_strchomp ("", "");
  do_test_strchomp (" ", "");
  do_test_strchomp (" \t\r\n", "");
  do_test_strchomp ("a ", "a");
  do_test_strchomp ("a  ", "a");
  do_test_strchomp ("a a", "a a");
  do_test_strchomp ("a a ", "a a");
}

/* Testing xstr_tokenize_and_fold() functions */
static void
test_str_tokenize_and_fold (void)
{
  const xchar_t *local_str = "en_GB";
  const xchar_t *sample  = "The quick brown fox¸ jumps over the lazy dog.";
  const xchar_t *special_cases = "quıck QUİCK QUİı QUıİ İıck ıİCK àìøş";
  xchar_t **tokens, **alternates;
  xchar_t
    *expected_tokens[] =     \
    {"the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog", NULL},
    *expected_tokens_alt[] = \
    { "quick", "quick", "quii", "quii", "iick", "iick", "àìøş", NULL};

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      tokens = xstr_tokenize_and_fold (NULL, local_str, NULL);
      g_test_assert_expected_messages ();
    }

  tokens = xstr_tokenize_and_fold (special_cases, local_str, &alternates);
  g_assert_cmpint (xstrv_length (tokens), ==,
                   xstrv_length (expected_tokens_alt));
  g_assert_true (xstrv_equal ((const xchar_t * const *) tokens,
                               (const xchar_t * const *) expected_tokens_alt));
  xstrfreev (tokens);
  xstrfreev (alternates);

  tokens = xstr_tokenize_and_fold (sample, local_str, &alternates);
  g_assert_cmpint (xstrv_length (tokens), ==, xstrv_length (expected_tokens));
  g_assert_true (xstrv_equal ((const xchar_t * const *) tokens,
                               (const xchar_t * const *) expected_tokens));
  xstrfreev (tokens);
  xstrfreev (alternates);

  tokens = xstr_tokenize_and_fold (sample, local_str, NULL);
  g_assert_cmpint (xstrv_length (tokens), ==, xstrv_length (expected_tokens));
  g_assert_true (xstrv_equal ((const xchar_t * const *) tokens,
                               (const xchar_t * const *) expected_tokens));
  xstrfreev (tokens);

  tokens = xstr_tokenize_and_fold (sample, NULL, &alternates);
  g_assert_cmpint (xstrv_length (tokens), ==, xstrv_length (expected_tokens));
  g_assert_true (xstrv_equal ((const xchar_t * const *) tokens,
                               (const xchar_t * const *) expected_tokens));
  xstrfreev (tokens);
  xstrfreev (alternates);
}

/* Testing xstrreverse() function with various positive and negative cases */
static void
test_strreverse (void)
{
  xchar_t *str;
  xchar_t *p;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str = xstrreverse (NULL);
      g_test_assert_expected_messages ();
      g_assert_null (str);
    }

  str = p = xstrdup ("abcde");
  str = xstrreverse (str);
  g_assert_nonnull (str);
  g_assert_true (p == str);
  g_assert_cmpstr (str, ==, "edcba");
  g_free (str);
}

/* Testing xstrncasecmp() functions */
static void
test_strncasecmp (void)
{
  g_assert_cmpint (xstrncasecmp ("abc1", "ABC2", 3), ==, 0);
  g_assert_cmpint (xstrncasecmp ("abc1", "ABC2", 4), !=, 0);
}

static void
test_strstr (void)
{
  xchar_t *haystack;
  xchar_t *res;

  haystack = xstrdup ("FooBarFooBarFoo");

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstrstr_len (NULL, 0, "xxx");
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstrstr_len ("xxx", 0, NULL);
      g_test_assert_expected_messages ();
    }

  /* strstr_len */
  res = xstrstr_len (haystack, 6, "xxx");
  g_assert_null (res);

  res = xstrstr_len (haystack, 6, "FooBarFooBarFooBar");
  g_assert_null (res);

  res = xstrstr_len (haystack, 3, "Bar");
  g_assert_null (res);

  res = xstrstr_len (haystack, 6, "");
  g_assert_true (res == haystack);
  g_assert_cmpstr (res, ==, "FooBarFooBarFoo");

  res = xstrstr_len (haystack, 6, "Bar");
  g_assert_true (res == haystack + 3);
  g_assert_cmpstr (res, ==, "BarFooBarFoo");

  res = xstrstr_len (haystack, -1, "Bar");
  g_assert_true (res == haystack + 3);
  g_assert_cmpstr (res, ==, "BarFooBarFoo");

  /* strrstr */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstrrstr (NULL, "xxx");
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstrrstr ("xxx", NULL);
      g_test_assert_expected_messages ();
    }

  res = xstrrstr (haystack, "xxx");
  g_assert_null (res);

  res = xstrrstr (haystack, "FooBarFooBarFooBar");
  g_assert_null (res);

  res = xstrrstr (haystack, "");
  g_assert_true (res == haystack);
  g_assert_cmpstr (res, ==, "FooBarFooBarFoo");

  res = xstrrstr (haystack, "Bar");
  g_assert_true (res == haystack + 9);
  g_assert_cmpstr (res, ==, "BarFoo");

  /* strrstr_len */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstrrstr_len (NULL, 14, "xxx");
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstrrstr_len ("xxx", 14, NULL);
      g_test_assert_expected_messages ();
    }

  res = xstrrstr_len (haystack, 14, "xxx");
  g_assert_null (res);

  res = xstrrstr_len (haystack, 14, "FooBarFooBarFooBar");
  g_assert_null (res);

  res = xstrrstr_len (haystack, 3, "Bar");
  g_assert_null (res);

  res = xstrrstr_len (haystack, 14, "BarFoo");
  g_assert_true (res == haystack + 3);
  g_assert_cmpstr (res, ==, "BarFooBarFoo");

  res = xstrrstr_len (haystack, 15, "BarFoo");
  g_assert_true (res == haystack + 9);
  g_assert_cmpstr (res, ==, "BarFoo");

  res = xstrrstr_len (haystack, -1, "BarFoo");
  g_assert_true (res == haystack + 9);
  g_assert_cmpstr (res, ==, "BarFoo");

  /* test case for strings with \0 in the middle */
  *(haystack + 7) = '\0';
  res = xstrstr_len (haystack, 15, "BarFoo");
  g_assert_null (res);

  g_free (haystack);
}

/* Testing xstrtod() function with various positive and negative cases */
static void
test_strtod (void)
{
  xchar_t *str_end = NULL;
  double value = 0.0;
  const double gold_ratio = 1.61803398874989484;
  const xchar_t *gold_ratio_str = "1.61803398874989484";
  const xchar_t *minus_gold_ratio_str = "-1.61803398874989484";

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      value = xstrtod (NULL, NULL);
      g_test_assert_expected_messages ();
      g_assert_cmpfloat (value, ==, 0.0);
    }

  g_assert_cmpfloat (xstrtod ("\x00\x00\x00\x00", NULL), ==, 0.0);
  g_assert_cmpfloat (xstrtod ("\x00\x00\x00\x00", &str_end), ==, 0.0);
  g_assert_cmpstr (str_end, ==, "");
  g_assert_cmpfloat (xstrtod ("\xff\xff\xff\xff", NULL), ==, 0.0);
  g_assert_cmpfloat (xstrtod ("\xff\xff\xff\xff", &str_end), ==, 0.0);
  g_assert_cmpstr (str_end, ==, "\xff\xff\xff\xff");

  /* Testing normal usage cases */
  g_assert_cmpfloat (xstrtod (gold_ratio_str, NULL), ==, gold_ratio);
  g_assert_cmpfloat (xstrtod (gold_ratio_str, &str_end), ==, gold_ratio);
  g_assert_true (str_end == gold_ratio_str + strlen (gold_ratio_str));
  g_assert_cmpfloat (xstrtod (minus_gold_ratio_str, NULL), ==, -gold_ratio);
  g_assert_cmpfloat (xstrtod (minus_gold_ratio_str, &str_end), ==, -gold_ratio);
  g_assert_true (str_end == minus_gold_ratio_str + strlen (minus_gold_ratio_str));
}

/* Testing xstrdelimit() function */
static void
test_strdelimit (void)
{
  const xchar_t *const_string = "ABCDE<*>Q";
  xchar_t *string;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      string = xstrdelimit (NULL, "ABCDE", 'N');
      g_test_assert_expected_messages ();
    }

  string = xstrdelimit (xstrdup (const_string), "<>", '?');
  g_assert_cmpstr (string, ==, "ABCDE?*?Q");
  g_free (string);

  string = xstrdelimit (xstrdup (const_string), NULL, '?');
  g_assert_cmpstr (string, ==, "ABCDE?*?Q");
  g_free (string);
}

/* Testing xstr_has_prefix() */
static void
test_has_prefix (void)
{
  xboolean_t res;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstr_has_prefix ("foo", NULL);
      g_test_assert_expected_messages ();
      g_assert_false (res);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstr_has_prefix (NULL, "foo");
      g_test_assert_expected_messages ();
      g_assert_false (res);
    }

  res = xstr_has_prefix ("foo", "bar");
  g_assert_cmpint (res, ==, FALSE);

  res = xstr_has_prefix ("foo", "foobar");
  g_assert_cmpint (res, ==, FALSE);

  res = xstr_has_prefix ("foobar", "bar");
  g_assert_cmpint (res, ==, FALSE);

  res = xstr_has_prefix ("foobar", "foo");
  g_assert_cmpint (res, ==, TRUE);

  res = xstr_has_prefix ("foo", "");
  g_assert_cmpint (res, ==, TRUE);

  res = xstr_has_prefix ("foo", "foo");
  g_assert_cmpint (res, ==, TRUE);

  res = xstr_has_prefix ("", "");
  g_assert_cmpint (res, ==, TRUE);
}

static void
test_has_suffix (void)
{
  xboolean_t res;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstr_has_suffix ("foo", NULL);
      g_test_assert_expected_messages ();
      g_assert_false (res);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      res = xstr_has_suffix (NULL, "foo");
      g_test_assert_expected_messages ();
      g_assert_false (res);
    }

  res = xstr_has_suffix ("foo", "bar");
  g_assert_false (res);

  res = xstr_has_suffix ("bar", "foobar");
  g_assert_false (res);

  res = xstr_has_suffix ("foobar", "foo");
  g_assert_false (res);

  res = xstr_has_suffix ("foobar", "bar");
  g_assert_true (res);

  res = xstr_has_suffix ("foo", "");
  g_assert_true (res);

  res = xstr_has_suffix ("foo", "foo");
  g_assert_true (res);

  res = xstr_has_suffix ("", "");
  g_assert_true (res);
}

static void
strv_check (xchar_t **strv, ...)
{
  xboolean_t ok = TRUE;
  xint_t i = 0;
  va_list list;

  va_start (list, strv);
  while (ok)
    {
      const xchar_t *str = va_arg (list, const char *);
      if (strv[i] == NULL)
	{
	  g_assert_null (str);
	  break;
	}
      if (str == NULL)
        {
	  ok = FALSE;
        }
      else
        {
          g_assert_cmpstr (strv[i], ==, str);
        }
      i++;
    }
  va_end (list);

  xstrfreev (strv);
}

/* Testing xstrsplit() function with various positive and negative cases */
static void
test_strsplit (void)
{
  xchar_t **string = NULL;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      string = xstrsplit (NULL, ",", 0);
      g_test_assert_expected_messages ();
      g_assert_null (string);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      string = xstrsplit ("x", NULL, 0);
      g_test_assert_expected_messages ();
      g_assert_null (string);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'delimiter[0] != \'\\0\'*");
      string = xstrsplit ("x", "", 0);
      g_test_assert_expected_messages ();
      g_assert_null (string);
    }

  strv_check (xstrsplit ("", ",", 0), NULL);
  strv_check (xstrsplit ("x", ",", 0), "x", NULL);
  strv_check (xstrsplit ("x,y", ",", 0), "x", "y", NULL);
  strv_check (xstrsplit ("x,y,", ",", 0), "x", "y", "", NULL);
  strv_check (xstrsplit (",x,y", ",", 0), "", "x", "y", NULL);
  strv_check (xstrsplit (",x,y,", ",", 0), "", "x", "y", "", NULL);
  strv_check (xstrsplit ("x,y,z", ",", 0), "x", "y", "z", NULL);
  strv_check (xstrsplit ("x,y,z,", ",", 0), "x", "y", "z", "", NULL);
  strv_check (xstrsplit (",x,y,z", ",", 0), "", "x", "y", "z", NULL);
  strv_check (xstrsplit (",x,y,z,", ",", 0), "", "x", "y", "z", "", NULL);
  strv_check (xstrsplit (",,x,,y,,z,,", ",", 0), "", "", "x", "", "y", "", "z", "", "", NULL);
  strv_check (xstrsplit (",,x,,y,,z,,", ",,", 0), "", "x", "y", "z", "", NULL);

  strv_check (xstrsplit ("", ",", 1), NULL);
  strv_check (xstrsplit ("x", ",", 1), "x", NULL);
  strv_check (xstrsplit ("x,y", ",", 1), "x,y", NULL);
  strv_check (xstrsplit ("x,y,", ",", 1), "x,y,", NULL);
  strv_check (xstrsplit (",x,y", ",", 1), ",x,y", NULL);
  strv_check (xstrsplit (",x,y,", ",", 1), ",x,y,", NULL);
  strv_check (xstrsplit ("x,y,z", ",", 1), "x,y,z", NULL);
  strv_check (xstrsplit ("x,y,z,", ",", 1), "x,y,z,", NULL);
  strv_check (xstrsplit (",x,y,z", ",", 1), ",x,y,z", NULL);
  strv_check (xstrsplit (",x,y,z,", ",", 1), ",x,y,z,", NULL);
  strv_check (xstrsplit (",,x,,y,,z,,", ",", 1), ",,x,,y,,z,,", NULL);
  strv_check (xstrsplit (",,x,,y,,z,,", ",,", 1), ",,x,,y,,z,,", NULL);

  strv_check (xstrsplit ("", ",", 2), NULL);
  strv_check (xstrsplit ("x", ",", 2), "x", NULL);
  strv_check (xstrsplit ("x,y", ",", 2), "x", "y", NULL);
  strv_check (xstrsplit ("x,y,", ",", 2), "x", "y,", NULL);
  strv_check (xstrsplit (",x,y", ",", 2), "", "x,y", NULL);
  strv_check (xstrsplit (",x,y,", ",", 2), "", "x,y,", NULL);
  strv_check (xstrsplit ("x,y,z", ",", 2), "x", "y,z", NULL);
  strv_check (xstrsplit ("x,y,z,", ",", 2), "x", "y,z,", NULL);
  strv_check (xstrsplit (",x,y,z", ",", 2), "", "x,y,z", NULL);
  strv_check (xstrsplit (",x,y,z,", ",", 2), "", "x,y,z,", NULL);
  strv_check (xstrsplit (",,x,,y,,z,,", ",", 2), "", ",x,,y,,z,,", NULL);
  strv_check (xstrsplit (",,x,,y,,z,,", ",,", 2), "", "x,,y,,z,,", NULL);
}

/* Testing function xstrsplit_set() */
static void
test_strsplit_set (void)
{
  xchar_t **string = NULL;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      string = xstrsplit_set (NULL, ",/", 0);
      g_test_assert_expected_messages ();
      g_assert_null (string);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      string = xstrsplit_set ("", NULL, 0);
      g_test_assert_expected_messages ();
      g_assert_null (string);
    }

  strv_check (xstrsplit_set ("", ",/", 0), NULL);
  strv_check (xstrsplit_set (":def/ghi:", ":/", -1), "", "def", "ghi", "", NULL);
  strv_check (xstrsplit_set ("abc:def/ghi", ":/", -1), "abc", "def", "ghi", NULL);
  strv_check (xstrsplit_set (",;,;,;,;", ",;", -1), "", "", "", "", "", "", "", "", "", NULL);
  strv_check (xstrsplit_set (",,abc.def", ".,", -1), "", "", "abc", "def", NULL);

  strv_check (xstrsplit_set (",x.y", ",.", 0), "", "x", "y", NULL);
  strv_check (xstrsplit_set (".x,y,", ",.", 0), "", "x", "y", "", NULL);
  strv_check (xstrsplit_set ("x,y.z", ",.", 0), "x", "y", "z", NULL);
  strv_check (xstrsplit_set ("x.y,z,", ",.", 0), "x", "y", "z", "", NULL);
  strv_check (xstrsplit_set (",x.y,z", ",.", 0), "", "x", "y", "z", NULL);
  strv_check (xstrsplit_set (",x,y,z,", ",.", 0), "", "x", "y", "z", "", NULL);
  strv_check (xstrsplit_set (",.x,,y,;z..", ".,;", 0), "", "", "x", "", "y", "", "z", "", "", NULL);
  strv_check (xstrsplit_set (",,x,,y,,z,,", ",,", 0), "", "", "x", "", "y", "", "z", "", "", NULL);

  strv_check (xstrsplit_set ("x,y.z", ",.", 1), "x,y.z", NULL);
  strv_check (xstrsplit_set ("x.y,z,", ",.", 1), "x.y,z,", NULL);
  strv_check (xstrsplit_set (",x,y,z", ",.", 1), ",x,y,z", NULL);
  strv_check (xstrsplit_set (",x,y.z,", ",.", 1), ",x,y.z,", NULL);
  strv_check (xstrsplit_set (",,x,.y,,z,,", ",.", 1), ",,x,.y,,z,,", NULL);
  strv_check (xstrsplit_set (",.x,,y,,z,,", ",,..", 1), ",.x,,y,,z,,", NULL);

  strv_check (xstrsplit_set ("", ",", 0), NULL);
  strv_check (xstrsplit_set ("x", ",", 0), "x", NULL);
  strv_check (xstrsplit_set ("x,y", ",", 0), "x", "y", NULL);
  strv_check (xstrsplit_set ("x,y,", ",", 0), "x", "y", "", NULL);
  strv_check (xstrsplit_set (",x,y", ",", 0), "", "x", "y", NULL);
  strv_check (xstrsplit_set (",x,y,", ",", 0), "", "x", "y", "", NULL);
  strv_check (xstrsplit_set ("x,y,z", ",", 0), "x", "y", "z", NULL);
  strv_check (xstrsplit_set ("x,y,z,", ",", 0), "x", "y", "z", "", NULL);
  strv_check (xstrsplit_set (",x,y,z", ",", 0), "", "x", "y", "z", NULL);
  strv_check (xstrsplit_set (",x,y,z,", ",", 0), "", "x", "y", "z", "", NULL);
  strv_check (xstrsplit_set (",,x,,y,,z,,", ",", 0), "", "", "x", "", "y", "", "z", "", "", NULL);

  strv_check (xstrsplit_set ("", ",", 1), NULL);
  strv_check (xstrsplit_set ("x", ",", 1), "x", NULL);
  strv_check (xstrsplit_set ("x,y", ",", 1), "x,y", NULL);
  strv_check (xstrsplit_set ("x,y,", ",", 1), "x,y,", NULL);
  strv_check (xstrsplit_set (",x,y", ",", 1), ",x,y", NULL);
  strv_check (xstrsplit_set (",x,y,", ",", 1), ",x,y,", NULL);
  strv_check (xstrsplit_set ("x,y,z", ",", 1), "x,y,z", NULL);
  strv_check (xstrsplit_set ("x,y,z,", ",", 1), "x,y,z,", NULL);
  strv_check (xstrsplit_set (",x,y,z", ",", 1), ",x,y,z", NULL);
  strv_check (xstrsplit_set (",x,y,z,", ",", 1), ",x,y,z,", NULL);
  strv_check (xstrsplit_set (",,x,,y,,z,,", ",", 1), ",,x,,y,,z,,", NULL);
  strv_check (xstrsplit_set (",,x,,y,,z,,", ",,", 1), ",,x,,y,,z,,", NULL);

  strv_check (xstrsplit_set ("", ",", 2), NULL);
  strv_check (xstrsplit_set ("x", ",", 2), "x", NULL);
  strv_check (xstrsplit_set ("x,y", ",", 2), "x", "y", NULL);
  strv_check (xstrsplit_set ("x,y,", ",", 2), "x", "y,", NULL);
  strv_check (xstrsplit_set (",x,y", ",", 2), "", "x,y", NULL);
  strv_check (xstrsplit_set (",x,y,", ",", 2), "", "x,y,", NULL);
  strv_check (xstrsplit_set ("x,y,z", ",", 2), "x", "y,z", NULL);
  strv_check (xstrsplit_set ("x,y,z,", ",", 2), "x", "y,z,", NULL);
  strv_check (xstrsplit_set (",x,y,z", ",", 2), "", "x,y,z", NULL);
  strv_check (xstrsplit_set (",x,y,z,", ",", 2), "", "x,y,z,", NULL);
  strv_check (xstrsplit_set (",,x,,y,,z,,", ",", 2), "", ",x,,y,,z,,", NULL);

  strv_check (xstrsplit_set (",,x,.y,..z,,", ",.", 3), "", "", "x,.y,..z,,", NULL);
}

/* Testing xstrv_length() function with various positive and negative cases */
static void
test_strv_length (void)
{
  xchar_t **strv;
  xuint_t l;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      l = xstrv_length (NULL);
      g_test_assert_expected_messages ();
      g_assert_cmpint (l, ==, 0);
    }

  strv = xstrsplit ("1,2,3,4", ",", -1);
  l = xstrv_length (strv);
  g_assert_cmpuint (l, ==, 4);
  xstrfreev (strv);
}

static char *locales[] = {"sv_SE", "en_US", "fa_IR", "C", "ru_RU"};

static void
check_strtod_string (xchar_t    *number,
		     double    res,
		     xboolean_t  check_end,
		     xsize_t      correct_len)
{
  double d;
  xsize_t l;
  xchar_t *dummy;

  /* we try a copy of number, with some free space for malloc before that.
   * This is supposed to smash the some wrong pointer calculations. */

  dummy = g_malloc (100000);
  number = xstrdup (number);
  g_free (dummy);

  for (l = 0; l < G_N_ELEMENTS (locales); l++)
    {
      xchar_t *end = "(unset)";

      setlocale (LC_NUMERIC, locales[l]);
      d = g_ascii_strtod (number, &end);
      g_assert_true (isnan (res) ? isnan (d) : (d == res));
      g_assert_true ((xsize_t) (end - number) ==
                     (check_end ? correct_len : strlen (number)));
    }

  g_free (number);
}

static void
check_strtod_number (xdouble_t num, xchar_t *fmt, xchar_t *str)
{
  xsize_t l;
  xchar_t buf[G_ASCII_DTOSTR_BUF_SIZE];

  for (l = 0; l < G_N_ELEMENTS (locales); l++)
    {
      setlocale (LC_ALL, locales[l]);
      g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, fmt, num);
      g_assert_cmpstr (buf, ==, str);
    }
}

/* Testing g_ascii_strtod() function with various positive and negative cases */
static void
test_ascii_strtod (void)
{
  xdouble_t d, our_nan, our_inf;
  char buffer[G_ASCII_DTOSTR_BUF_SIZE];

#ifdef NAN
  our_nan = NAN;
#else
  /* Do this before any call to setlocale.  */
  our_nan = atof ("NaN");
#endif
  g_assert_true (isnan (our_nan));

#ifdef INFINITY
  our_inf = INFINITY;
#else
  our_inf = atof ("Infinity");
#endif
  g_assert_true (our_inf > 1 && our_inf == our_inf / 2);

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      d = g_ascii_strtod (NULL, NULL);
      g_test_assert_expected_messages ();
    }

  /* Testing normal cases */
  check_strtod_string ("123.123", 123.123, FALSE, 0);
  check_strtod_string ("123.123e2", 123.123e2, FALSE, 0);
  check_strtod_string ("123.123e-2", 123.123e-2, FALSE, 0);
  check_strtod_string ("-123.123", -123.123, FALSE, 0);
  check_strtod_string ("-123.123e2", -123.123e2, FALSE, 0);
  check_strtod_string ("-123.123e-2", -123.123e-2, FALSE, 0);
  check_strtod_string ("5.4", 5.4, TRUE, 3);
  check_strtod_string ("5.4,5.5", 5.4, TRUE, 3);
  check_strtod_string ("5,4", 5.0, TRUE, 1);
#ifndef _MSC_VER
  /* hex strings for strtod() is a C99 feature which Visual C++ does not support */
  check_strtod_string ("0xa.b", 10.6875, TRUE, 5);
  check_strtod_string ("0xa.bP3", 85.5, TRUE, 7);
  check_strtod_string ("0xa.bp+3", 85.5, TRUE, 8);
  check_strtod_string ("0xa.bp-2", 2.671875, TRUE, 8);
  check_strtod_string ("0xA.BG", 10.6875, TRUE, 5);
#endif
  /* the following are for #156421 */
  check_strtod_string ("1e1", 1e1, FALSE, 0);
#ifndef _MSC_VER
  /* NAN/-nan/INF/-infinity strings for strtod() are C99 features which Visual C++ does not support */
  check_strtod_string ("NAN", our_nan, FALSE, 0);
  check_strtod_string ("-nan", -our_nan, FALSE, 0);
  check_strtod_string ("INF", our_inf, FALSE, 0);
  check_strtod_string ("-infinity", -our_inf, FALSE, 0);
#endif
  check_strtod_string ("-.75,0", -0.75, TRUE, 4);

#ifndef _MSC_VER
  /* the values of d in the following 2 tests generate a C1064 compiler limit error */
  d = 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0;
  g_assert_true (d == g_ascii_strtod (g_ascii_dtostr (buffer, sizeof (buffer), d), NULL));

  d = -179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0;
  g_assert_true (d == g_ascii_strtod (g_ascii_dtostr (buffer, sizeof (buffer), d), NULL));
#endif

  d = pow (2.0, -1024.1);
  g_assert_true (d == g_ascii_strtod (g_ascii_dtostr (buffer, sizeof (buffer), d), NULL));

  d = -pow (2.0, -1024.1);
  g_assert_true (d == g_ascii_strtod (g_ascii_dtostr (buffer, sizeof (buffer), d), NULL));

  /* for #343899 */
  check_strtod_string (" 0.75", 0.75, FALSE, 0);
  check_strtod_string (" +0.75", 0.75, FALSE, 0);
  check_strtod_string (" -0.75", -0.75, FALSE, 0);
  check_strtod_string ("\f0.75", 0.75, FALSE, 0);
  check_strtod_string ("\n0.75", 0.75, FALSE, 0);
  check_strtod_string ("\r0.75", 0.75, FALSE, 0);
  check_strtod_string ("\t0.75", 0.75, FALSE, 0);

#if 0
  /* g_ascii_isspace() returns FALSE for vertical tab, see #59388 */
  check_strtod_string ("\v0.75", 0.75, FALSE, 0);
#endif

  /* for #343899 */
  check_strtod_number (0.75, "%0.2f", "0.75");
  check_strtod_number (0.75, "%5.2f", " 0.75");
  check_strtod_number (-0.75, "%0.2f", "-0.75");
  check_strtod_number (-0.75, "%5.2f", "-0.75");
  check_strtod_number (1e99, "%.0e", "1e+99");
}

static void
check_uint64 (const xchar_t *str,
	      const xchar_t *end,
	      xint_t         base,
	      xuint64_t      result,
	      xint_t         error)
{
  xuint64_t actual;
  xchar_t *endptr = NULL;
  xint_t err;

  errno = 0;
  actual = g_ascii_strtoull (str, &endptr, base);
  err = errno;

  g_assert_true (actual == result);
  g_assert_cmpstr (end, ==, endptr);
  g_assert_true (err == error);
}

static void
check_int64 (const xchar_t *str,
	     const xchar_t *end,
	     xint_t         base,
	     sint64_t       result,
	     xint_t         error)
{
  sint64_t actual;
  xchar_t *endptr = NULL;
  xint_t err;

  errno = 0;
  actual = g_ascii_strtoll (str, &endptr, base);
  err = errno;

  g_assert_true (actual == result);
  g_assert_cmpstr (end, ==, endptr);
  g_assert_true (err == error);
}

static void
test_strtoll (void)
{
  check_uint64 ("0", "", 10, 0, 0);
  check_uint64 ("+0", "", 10, 0, 0);
  check_uint64 ("-0", "", 10, 0, 0);
  check_uint64 ("18446744073709551615", "", 10, G_MAXUINT64, 0);
  check_uint64 ("18446744073709551616", "", 10, G_MAXUINT64, ERANGE);
  check_uint64 ("20xyz", "xyz", 10, 20, 0);
  check_uint64 ("-1", "", 10, G_MAXUINT64, 0);
  check_uint64 ("-FF4", "", 16, -((xuint64_t) 0xFF4), 0);

  check_int64 ("0", "", 10, 0, 0);
  check_int64 ("9223372036854775807", "", 10, G_MAXINT64, 0);
  check_int64 ("9223372036854775808", "", 10, G_MAXINT64, ERANGE);
  check_int64 ("-9223372036854775808", "", 10, G_MININT64, 0);
  check_int64 ("-9223372036854775809", "", 10, G_MININT64, ERANGE);
  check_int64 ("32768", "", 10, 32768, 0);
  check_int64 ("-32768", "", 10, -32768, 0);
  check_int64 ("001", "", 10, 1, 0);
  check_int64 ("-001", "", 10, -1, 0);
}

/* Testing xstr_match_string() function with various cases */
static void
test_str_match_string (void)
{
  xboolean_t result = TRUE;
  const xchar_t *str = "The quick brown fox¸ jumps over the lazy dog.";

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = xstr_match_string (NULL, "AAA", TRUE);
      g_test_assert_expected_messages ();
      g_assert_false (result);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = xstr_match_string (str, NULL, TRUE);
      g_test_assert_expected_messages ();
      g_assert_false (result);
    }

  g_assert_false (xstr_match_string (str, "AAA", TRUE));
  g_assert_false (xstr_match_string (str, "AAA", FALSE));
}

/* Testing functions bounds */
static void
test_bounds (void)
{
  xmapped_file_t *file, *before, *after;
  char buffer[4097];
  char *tmp, *tmp2;
  char **array;
  char *string;
  const char * const strjoinv_0[] = { NULL };
  const char * const strjoinv_1[] = { "foo", NULL };

  /* if we allocate the file between two others and then free those
   * other two, then hopefully we end up with unmapped memory on either
   * side.
   */
  before = xmapped_file_new ("4096-random-bytes", TRUE, NULL);

  /* quick workaround until #549783 can be fixed */
  if (before == NULL)
    return;

  file = xmapped_file_new ("4096-random-bytes", TRUE, NULL);
  after = xmapped_file_new ("4096-random-bytes", TRUE, NULL);
  xmapped_file_unref (before);
  xmapped_file_unref (after);

  g_assert_nonnull (file);
  g_assert_cmpint (xmapped_file_get_length (file), ==, 4096);
  string = xmapped_file_get_contents (file);

  /* ensure they're all non-nul */
  g_assert_null (memchr (string, '\0', 4096));

  /* test set 1: ensure that nothing goes past its maximum length, even in
   *             light of a missing nul terminator.
   *
   * we try to test all of the 'n' functions here.
   */
  tmp = xstrndup (string, 4096);
  g_assert_cmpint (strlen (tmp), ==, 4096);
  g_free (tmp);

  /* found no bugs in gnome, i hope :) */
  g_assert_null (xstrstr_len (string, 4096, "BUGS"));
  xstrstr_len (string, 4096, "B");
  xstrstr_len (string, 4096, ".");
  xstrstr_len (string, 4096, "");

  xstrrstr_len (string, 4096, "BUGS");
  xstrrstr_len (string, 4096, "B");
  xstrrstr_len (string, 4096, ".");
  xstrrstr_len (string, 4096, "");

  tmp = g_ascii_strup (string, 4096);
  tmp2 = g_ascii_strup (tmp, 4096);
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp, 4096), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp2, 4096), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (tmp, tmp2, 4096), ==, 0);
  g_free (tmp);
  g_free (tmp2);

  tmp = g_ascii_strdown (string, 4096);
  tmp2 = g_ascii_strdown (tmp, 4096);
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp, 4096), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp2, 4096), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (tmp, tmp2, 4096), ==, 0);
  g_free (tmp);
  g_free (tmp2);

  tmp = g_markup_escape_text (string, 4096);
  g_free (tmp);

  /* test set 2: ensure that nothing reads even one byte past a '\0'.
   */
  g_assert_cmpint (string[4095], ==, '\n');
  string[4095] = '\0';

  tmp = xstrdup (string);
  g_assert_cmpint (strlen (tmp), ==, 4095);
  g_free (tmp);

  tmp = xstrndup (string, 10000);
  g_assert_cmpint (strlen (tmp), ==, 4095);
  g_free (tmp);

  g_stpcpy (buffer, string);
  g_assert_cmpint (strlen (buffer), ==, 4095);

  xstrstr_len (string, 10000, "BUGS");
  xstrstr_len (string, 10000, "B");
  xstrstr_len (string, 10000, ".");
  xstrstr_len (string, 10000, "");

  xstrrstr (string, "BUGS");
  xstrrstr (string, "B");
  xstrrstr (string, ".");
  xstrrstr (string, "");

  xstrrstr_len (string, 10000, "BUGS");
  xstrrstr_len (string, 10000, "B");
  xstrrstr_len (string, 10000, ".");
  xstrrstr_len (string, 10000, "");

  xstr_has_prefix (string, "this won't do very much...");
  xstr_has_suffix (string, "but maybe this will...");
  xstr_has_suffix (string, "HMMMM.");
  xstr_has_suffix (string, "MMMM.");
  xstr_has_suffix (string, "M.");

  xstrlcpy (buffer, string, sizeof buffer);
  g_assert_cmpint (strlen (buffer), ==, 4095);
  xstrlcpy (buffer, string, sizeof buffer);
  buffer[0] = '\0';
  xstrlcat (buffer, string, sizeof buffer);
  g_assert_cmpint (strlen (buffer), ==, 4095);

  tmp = xstrdup_printf ("<%s>", string);
  g_assert_cmpint (strlen (tmp), ==, 4095 + 2);
  g_free (tmp);

  tmp = g_ascii_strdown (string, -1);
  tmp2 = g_ascii_strdown (tmp, -1);
  g_assert_cmpint (strlen (tmp), ==, strlen (tmp2));
  g_assert_cmpint (strlen (string), ==, strlen (tmp));
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp, -1), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp2, -1), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (tmp, tmp2, -1), ==, 0);
  g_free (tmp);
  g_free (tmp2);

  tmp = g_ascii_strup (string, -1);
  tmp2 = g_ascii_strup (string, -1);
  g_assert_cmpint (strlen (tmp), ==, strlen (tmp2));
  g_assert_cmpint (strlen (string), ==, strlen (tmp));
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp, -1), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (string, tmp2, -1), ==, 0);
  g_assert_cmpint (g_ascii_strncasecmp (tmp, tmp2, -1), ==, 0);
  g_free (tmp);
  g_free (tmp2);

  g_ascii_strcasecmp (string, string);
  g_ascii_strncasecmp (string, string, 10000);

  xstrreverse (string);
  xstrreverse (string);
  xstrchug (string);
  xstrchomp (string);
  xstrstrip (string);
  g_assert_cmpint (strlen (string), ==, 4095);

  xstrdelimit (string, "M", 'N');
  xstrcanon (string, " N.", ':');
  g_assert_cmpint (strlen (string), ==, 4095);

  array = xstrsplit (string, ".", -1);
  tmp = xstrjoinv (".", array);
  xstrfreev (array);

  g_assert_cmpmem (tmp, strlen (tmp), string, 4095);
  g_free (tmp);

  tmp = xstrjoinv ("/", (char **) strjoinv_0);
  g_assert_cmpstr (tmp, ==, "");
  g_free (tmp);

  tmp = xstrjoinv ("/", (char **) strjoinv_1);
  g_assert_cmpstr (tmp, ==, "foo");
  g_free (tmp);

  tmp = xstrconcat (string, string, string, NULL);
  g_assert_cmpint (strlen (tmp), ==, 4095 * 3);
  g_free (tmp);

  tmp = xstrjoin ("!", string, string, NULL);
  g_assert_cmpint (strlen (tmp), ==, 4095 + 1 + 4095);
  g_free (tmp);

  tmp = g_markup_escape_text (string, -1);
  g_free (tmp);

  tmp = g_markup_printf_escaped ("%s", string);
  g_free (tmp);

  tmp = xstrescape (string, NULL);
  tmp2 = xstrcompress (tmp);
  g_assert_cmpstr (string, ==, tmp2);
  g_free (tmp2);
  g_free (tmp);

  xmapped_file_unref (file);
}

/* Testing xstrip_context() function with various cases */
static void
test_strip_context (void)
{
  const xchar_t *msgid;
  const xchar_t *msgval;
  const xchar_t *s;

  msgid = "blabla";
  msgval = "bla";
  s = xstrip_context (msgid, msgval);
  g_assert_true (s == msgval);

  msgid = msgval = "blabla";
  s = xstrip_context (msgid, msgval);
  g_assert_true (s == msgval);

  msgid = msgval = "blabla|foo";
  s = xstrip_context (msgid, msgval);
  g_assert_true (s == msgval + 7);

  msgid = msgval = "blabla||bar";
  s = xstrip_context (msgid, msgval);
  g_assert_true (s == msgval + 7);
}

/* test_t the strings returned by xstrerror() are valid and unique. On Windows,
 * fewer than 200 error numbers are used, so we expect some strings to
 * return a generic ‘unknown error code’ message. */
static void
test_strerror (void)
{
  xhashtable_t *strs;
  xint_t i;
  const xchar_t *str, *unknown_str;

  setlocale (LC_ALL, "C");

  unknown_str = xstrerror (-1);
  strs = xhash_table_new (xstr_hash, xstr_equal);
  for (i = 1; i < 200; i++)
    {
      xboolean_t is_unknown;
      str = xstrerror (i);
      is_unknown = (strcmp (str, unknown_str) == 0);
      g_assert_nonnull (str);
      g_assert_true (xutf8_validate (str, -1, NULL));
      g_assert_true (!xhash_table_contains (strs, str) || is_unknown);
      xhash_table_add (strs, (xpointer_t) str);
    }

  xhash_table_unref (strs);
}

/* Testing xstrsignal() function with various cases */
static void
test_strsignal (void)
{
  xint_t i;
  const xchar_t *str;

  for (i = 1; i < 20; i++)
    {
      str = xstrsignal (i);
      g_assert_nonnull (str);
      g_assert_true (xutf8_validate (str, -1, NULL));
    }
}

/* Testing xstrup(), xstrdown() and xstrcasecmp() */
static void
test_strup (void)
{
  xchar_t *s = NULL;

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      s = xstrup (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      s = xstrdown (NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      xstrcasecmp (NULL, "ABCD");
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      xstrcasecmp ("abcd", NULL);
      g_test_assert_expected_messages ();
    }

  s = xstrdup ("lower UPPER");
  g_assert_cmpstr (xstrup (s), ==, "LOWER UPPER");
  g_assert_cmpstr (xstrdown (s), ==, "lower upper");
  g_assert_true (xstrcasecmp ("lower", "LOWER") == 0);
  g_free (s);
}

/* Testing xstr_to_ascii() function with various cases */
static void
test_transliteration (void)
{
  xchar_t *out;

  /* ...to test the defaults */
  setlocale (LC_ALL, "C");

  /* test_t something trivial */
  out = xstr_to_ascii ("hello", NULL);
  g_assert_cmpstr (out, ==, "hello");
  g_free (out);

  /* test_t something above 0xffff */
  out = xstr_to_ascii ("𝐀𝐀𝐀", NULL);
  g_assert_cmpstr (out, ==, "AAA");
  g_free (out);

  /* test_t something with no good match */
  out = xstr_to_ascii ("a ∧ ¬a", NULL);
  g_assert_cmpstr (out, ==, "a ? ?a");
  g_free (out);

  /* Make sure 'ö' is handled differently per locale */
  out = xstr_to_ascii ("ö", NULL);
  g_assert_cmpstr (out, ==, "o");
  g_free (out);

  out = xstr_to_ascii ("ö", "sv");
  g_assert_cmpstr (out, ==, "o");
  g_free (out);

  out = xstr_to_ascii ("ö", "de");
  g_assert_cmpstr (out, ==, "oe");
  g_free (out);

  /* Make sure we can find a locale by a wide range of names */
  out = xstr_to_ascii ("ö", "de_DE");
  g_assert_cmpstr (out, ==, "oe");
  g_free (out);

  out = xstr_to_ascii ("ö", "de_DE.UTF-8");
  g_assert_cmpstr (out, ==, "oe");
  g_free (out);

  out = xstr_to_ascii ("ö", "de_DE.UTF-8@euro");
  g_assert_cmpstr (out, ==, "oe");
  g_free (out);

  out = xstr_to_ascii ("ö", "de@euro");
  g_assert_cmpstr (out, ==, "oe");
  g_free (out);

  /* test_t some invalid locale names */
  out = xstr_to_ascii ("ö", "de_DE@euro.UTF-8");
  g_assert_cmpstr (out, ==, "o");
  g_free (out);

  out = xstr_to_ascii ("ö", "de@DE@euro");
  g_assert_cmpstr (out, ==, "o");
  g_free (out);

  out = xstr_to_ascii ("ö", "doesnotexist");
  g_assert_cmpstr (out, ==, "o");
  g_free (out);

  out = xstr_to_ascii ("ö", "thislocalenameistoolong");
  g_assert_cmpstr (out, ==, "o");
  g_free (out);

  /* Try a lookup of a locale with a variant */
  out = xstr_to_ascii ("б", "sr_RS");
  g_assert_cmpstr (out, ==, "b");
  g_free (out);

  out = xstr_to_ascii ("б", "sr_RS@latin");
  g_assert_cmpstr (out, ==, "?");
  g_free (out);

  /* Ukrainian contains the only multi-character mappings.
   * Try a string that contains one ('зг') along with a partial
   * sequence ('з') at the end.
   */
  out = xstr_to_ascii ("Зліва направо, згори вниз", "uk");
  g_assert_cmpstr (out, ==, "Zliva napravo, zghory vnyz");
  g_free (out);

  /* Try out the other combinations */
  out = xstr_to_ascii ("Зг", "uk");
  g_assert_cmpstr (out, ==, "Zgh");
  g_free (out);

  out = xstr_to_ascii ("зГ", "uk");
  g_assert_cmpstr (out, ==, "zGH");
  g_free (out);

  out = xstr_to_ascii ("ЗГ", "uk");
  g_assert_cmpstr (out, ==, "ZGH");
  g_free (out);

  /* And a non-combination */
  out = xstr_to_ascii ("зя", "uk");
  g_assert_cmpstr (out, ==, "zya");
  g_free (out);
}

/* Testing xstrv_contains() function with various cases */
static void
test_strv_contains (void)
{
  xboolean_t result = TRUE;
  const xchar_t *strv_simple[] = { "hello", "there", NULL };
  const xchar_t *strv_dupe[] = { "dupe", "dupe", NULL };
  const xchar_t *strv_empty[] = { NULL };

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = xstrv_contains (NULL, "hello");
      g_test_assert_expected_messages ();
      g_assert_false (result);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = xstrv_contains (strv_simple, NULL);
      g_test_assert_expected_messages ();
      g_assert_false (result);
    }

  g_assert_true (xstrv_contains (strv_simple, "hello"));
  g_assert_true (xstrv_contains (strv_simple, "there"));
  g_assert_false (xstrv_contains (strv_simple, "non-existent"));
  g_assert_false (xstrv_contains (strv_simple, ""));

  g_assert_true (xstrv_contains (strv_dupe, "dupe"));

  g_assert_false (xstrv_contains (strv_empty, "empty!"));
  g_assert_false (xstrv_contains (strv_empty, ""));
}

/* test_t xstrv_equal() works for various inputs. */
static void
test_strv_equal (void)
{
  xboolean_t result = TRUE;
  const xchar_t *strv_empty[] = { NULL };
  const xchar_t *strv_empty2[] = { NULL };
  const xchar_t *strv_simple[] = { "hello", "you", NULL };
  const xchar_t *strv_simple2[] = { "hello", "you", NULL };
  const xchar_t *strv_simple_reordered[] = { "you", "hello", NULL };
  const xchar_t *strv_simple_superset[] = { "hello", "you", "again", NULL };
  const xchar_t *strv_another[] = { "not", "a", "coded", "message", NULL };

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = xstrv_equal (NULL, strv_simple2);
      g_test_assert_expected_messages ();
      g_assert_false (result);

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = xstrv_equal (strv_simple, NULL);
      g_test_assert_expected_messages ();
      g_assert_false (result);
    }

  g_assert_true (xstrv_equal (strv_empty, strv_empty));
  g_assert_true (xstrv_equal (strv_empty, strv_empty2));
  g_assert_true (xstrv_equal (strv_empty2, strv_empty));
  g_assert_false (xstrv_equal (strv_empty, strv_simple));
  g_assert_false (xstrv_equal (strv_simple, strv_empty));
  g_assert_true (xstrv_equal (strv_simple, strv_simple));
  g_assert_true (xstrv_equal (strv_simple, strv_simple2));
  g_assert_true (xstrv_equal (strv_simple2, strv_simple));
  g_assert_false (xstrv_equal (strv_simple, strv_simple_reordered));
  g_assert_false (xstrv_equal (strv_simple_reordered, strv_simple));
  g_assert_false (xstrv_equal (strv_simple, strv_simple_superset));
  g_assert_false (xstrv_equal (strv_simple_superset, strv_simple));
  g_assert_false (xstrv_equal (strv_simple, strv_another));
  g_assert_false (xstrv_equal (strv_another, strv_simple));
}

typedef enum
  {
    SIGNED,
    UNSIGNED
  } SignType;

typedef struct
{
  const xchar_t *str;
  SignType sign_type;
  xuint_t base;
  xint_t min;
  xint_t max;
  xint_t expected;
  xboolean_t should_fail;
  GNumberParserError error_code;
} TestData;

const TestData test_data[] = {
  /* typical cases for signed */
  { "0",  SIGNED, 10, -2,  2,  0, FALSE, 0                                   },
  { "+0", SIGNED, 10, -2,  2,  0, FALSE, 0                                   },
  { "-0", SIGNED, 10, -2,  2,  0, FALSE, 0                                   },
  { "-2", SIGNED, 10, -2,  2, -2, FALSE, 0                                   },
  {"-02", SIGNED, 10, -2,  2, -2, FALSE, 0                                   },
  { "2",  SIGNED, 10, -2,  2,  2, FALSE, 0                                   },
  { "02", SIGNED, 10, -2,  2,  2, FALSE, 0                                   },
  { "+2", SIGNED, 10, -2,  2,  2, FALSE, 0                                   },
  {"+02", SIGNED, 10, -2,  2,  2, FALSE, 0                                   },
  { "3",  SIGNED, 10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },
  { "+3", SIGNED, 10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },
  { "-3", SIGNED, 10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },

  /* typical cases for unsigned */
  { "-1", UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID       },
  { "1",  UNSIGNED, 10, 0, 2, 1, FALSE, 0                                   },
  { "+1", UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID       },
  { "0",  UNSIGNED, 10, 0, 2, 0, FALSE, 0                                   },
  { "+0", UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID       },
  { "-0", UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID       },
  { "2",  UNSIGNED, 10, 0, 2, 2, FALSE, 0                                   },
  { "+2", UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID       },
  { "3",  UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },
  { "+3", UNSIGNED, 10, 0, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID       },

  /* min == max cases for signed */
  { "-2", SIGNED, 10, -2, -2, -2, FALSE, 0                                   },
  { "-1", SIGNED, 10, -2, -2,  0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },
  { "-3", SIGNED, 10, -2, -2,  0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },

  /* min == max cases for unsigned */
  { "2", UNSIGNED, 10, 2, 2, 2, FALSE, 0                                   },
  { "3", UNSIGNED, 10, 2, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },
  { "1", UNSIGNED, 10, 2, 2, 0, TRUE,  G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS },

  /* invalid inputs */
  { "",    SIGNED,   10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "",    UNSIGNED, 10,  0,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "a",   SIGNED,   10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "a",   UNSIGNED, 10,  0,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "1a",  SIGNED,   10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "1a",  UNSIGNED, 10,  0,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "- 1", SIGNED,   10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },

  /* leading/trailing whitespace */
  { " 1", SIGNED,   10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { " 1", UNSIGNED, 10,  0,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "1 ", SIGNED,   10, -2,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "1 ", UNSIGNED, 10,  0,  2,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },

  /* hexadecimal numbers */
  { "a",     SIGNED,   16,   0, 15, 10, FALSE, 0                             },
  { "a",     UNSIGNED, 16,   0, 15, 10, FALSE, 0                             },
  { "0a",    UNSIGNED, 16,   0, 15, 10, FALSE, 0                             },
  { "0xa",   SIGNED,   16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "0xa",   UNSIGNED, 16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "-0xa",  SIGNED,   16, -15, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "-0xa",  UNSIGNED, 16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "+0xa",  SIGNED,   16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "+0xa",  UNSIGNED, 16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "- 0xa", SIGNED,   16, -15, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "- 0xa", UNSIGNED, 16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "+ 0xa", SIGNED,   16, -15, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
  { "+ 0xa", UNSIGNED, 16,   0, 15,  0, TRUE,  G_NUMBER_PARSER_ERROR_INVALID },
};

/* Testing g_ascii_string_to_signed() and g_ascii_string_to_unsigned() functions */
static void
test_ascii_string_to_number_usual (void)
{
  xsize_t idx;
  xboolean_t result;
  xerror_t *error = NULL;
  const TestData *data;
  xint_t value;
  sint64_t value64 = 0;
  xuint64_t valueu64 = 0;

  /*** g_ascii_string_to_signed() ***/
  data = &test_data[0]; /* Setting data to signed data */

  if (g_test_undefined ())
    {
      /* Testing degenerated cases */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = g_ascii_string_to_signed (NULL,
                                         data->base,
                                         data->min,
                                         data->max,
                                         &value64,
                                         &error);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'base >= 2 && base <= 36\'*");
      result = g_ascii_string_to_signed (data->str,
                                         1,
                                         data->min,
                                         data->max,
                                         &value64,
                                         &error);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'base >= 2 && base <= 36\'*");
      result = g_ascii_string_to_signed (data->str,
                                         40,
                                         data->min,
                                         data->max,
                                         &value64,
                                         &error);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'min <= max\'*");
      result = g_ascii_string_to_signed (data->str,
                                         data->base,
                                         data->max,
                                         data->min,
                                         &value64,
                                         &error);
      g_test_assert_expected_messages ();
    }

  /* Catching first part of (error == NULL || *error == NULL) */
  result = g_ascii_string_to_signed (data->str,
                                     data->base,
                                     data->min,
                                     data->max,
                                     &value64,
                                     NULL);

  /*** g_ascii_string_to_unsigned() ***/
  data = &test_data[12]; /* Setting data to unsigned data */

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      result = g_ascii_string_to_unsigned (NULL,
                                           data->base,
                                           data->min,
                                           data->max,
                                           &valueu64,
                                           &error);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'base >= 2 && base <= 36\'*");
      result = g_ascii_string_to_unsigned (data->str,
                                           1,
                                           data->min,
                                           data->max,
                                           &valueu64,
                                           &error);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'base >= 2 && base <= 36\'*");
      result = g_ascii_string_to_unsigned (data->str,
                                           40,
                                           data->min,
                                           data->max,
                                           &valueu64,
                                           &error);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion \'min <= max\'*");
      result = g_ascii_string_to_unsigned (data->str,
                                           data->base,
                                           data->max,
                                           data->min,
                                           &valueu64,
                                           &error);
      g_test_assert_expected_messages ();
    }

  /* Catching first part of (error == NULL || *error == NULL) */
  result = g_ascii_string_to_unsigned (data->str,
                                       data->base,
                                       data->min,
                                       data->max,
                                       &valueu64,
                                       NULL);

  /* Testing usual cases */
  for (idx = 0; idx < G_N_ELEMENTS (test_data); ++idx)
    {
      data = &test_data[idx];

      switch (data->sign_type)
        {
        case SIGNED:
          {
            result = g_ascii_string_to_signed (data->str,
                                               data->base,
                                               data->min,
                                               data->max,
                                               &value64,
                                               &error);
            value = value64;
            g_assert_cmpint (value, ==, value64);
            break;
          }

        case UNSIGNED:
          {
            result = g_ascii_string_to_unsigned (data->str,
                                                 data->base,
                                                 data->min,
                                                 data->max,
                                                 &valueu64,
                                                 &error);
            value = valueu64;
            g_assert_cmpint (value, ==, valueu64);
            break;
          }

        default:
          g_assert_not_reached ();
        }

      if (data->should_fail)
        {
          g_assert_false (result);
          g_assert_error (error, G_NUMBER_PARSER_ERROR, (xint_t) data->error_code);
          g_clear_error (&error);
        }
      else
        {
          g_assert_true (result);
          g_assert_no_error (error);
          g_assert_cmpint (value, ==, data->expected);
        }
    }
}

/* Testing pathological cases for g_ascii_string_to_(un)signed()  */
static void
test_ascii_string_to_number_pathological (void)
{
  xerror_t *error = NULL;
  const xchar_t *crazy_high = "999999999999999999999999999999999999";
  const xchar_t *crazy_low = "-999999999999999999999999999999999999";
  const xchar_t *max_uint64 = "18446744073709551615";
  const xchar_t *max_int64 = "9223372036854775807";
  const xchar_t *min_int64 = "-9223372036854775808";
  xuint64_t uvalue = 0;
  sint64_t svalue = 0;

  g_assert_false (g_ascii_string_to_unsigned (crazy_high,
                                              10,
                                              0,
                                              G_MAXUINT64,
                                              NULL,
                                              &error));
  g_assert_error (error, G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS);
  g_clear_error (&error);
  g_assert_false (g_ascii_string_to_unsigned (crazy_low,
                                              10,
                                              0,
                                              G_MAXUINT64,
                                              NULL,
                                              &error));
  // crazy_low is a signed number so it is not a valid unsigned number
  g_assert_error (error, G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_INVALID);
  g_clear_error (&error);

  g_assert_false (g_ascii_string_to_signed (crazy_high,
                                            10,
                                            G_MININT64,
                                            G_MAXINT64,
                                            NULL,
                                            &error));
  g_assert_error (error, G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS);
  g_clear_error (&error);
  g_assert_false (g_ascii_string_to_signed (crazy_low,
                                            10,
                                            G_MININT64,
                                            G_MAXINT64,
                                            NULL,
                                            &error));
  g_assert_error (error, G_NUMBER_PARSER_ERROR, G_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS);
  g_clear_error (&error);

  g_assert_true (g_ascii_string_to_unsigned (max_uint64,
                                             10,
                                             0,
                                             G_MAXUINT64,
                                             &uvalue,
                                             &error));
  g_assert_no_error (error);
  g_assert_cmpint (uvalue, ==, G_MAXUINT64);

  g_assert_true (g_ascii_string_to_signed (max_int64,
                                           10,
                                           G_MININT64,
                                           G_MAXINT64,
                                           &svalue,
                                           &error));
  g_assert_no_error (error);
  g_assert_cmpint (svalue, ==, G_MAXINT64);

  g_assert_true (g_ascii_string_to_signed (min_int64,
                                           10,
                                           G_MININT64,
                                           G_MAXINT64,
                                           &svalue,
                                           &error));
  g_assert_no_error (error);
  g_assert_cmpint (svalue, ==, G_MININT64);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/strfuncs/ascii-strcasecmp", test_ascii_strcasecmp);
  g_test_add_func ("/strfuncs/ascii-string-to-num/pathological", test_ascii_string_to_number_pathological);
  g_test_add_func ("/strfuncs/ascii-string-to-num/usual", test_ascii_string_to_number_usual);
  g_test_add_func ("/strfuncs/ascii_strdown", test_ascii_strdown);
  g_test_add_func ("/strfuncs/ascii_strdup", test_ascii_strup);
  g_test_add_func ("/strfuncs/ascii_strtod", test_ascii_strtod);
  g_test_add_func ("/strfuncs/bounds-check", test_bounds);
  g_test_add_func ("/strfuncs/has-prefix", test_has_prefix);
  g_test_add_func ("/strfuncs/has-suffix", test_has_suffix);
  g_test_add_func ("/strfuncs/memdup", test_memdup);
  g_test_add_func ("/strfuncs/memdup2", test_memdup2);
  g_test_add_func ("/strfuncs/stpcpy", test_stpcpy);
  g_test_add_func ("/strfuncs/str_match_string", test_str_match_string);
  g_test_add_func ("/strfuncs/str_tokenize_and_fold", test_str_tokenize_and_fold);
  g_test_add_func ("/strfuncs/strcanon", test_strcanon);
  g_test_add_func ("/strfuncs/strchomp", test_strchomp);
  g_test_add_func ("/strfuncs/strchug", test_strchug);
  g_test_add_func ("/strfuncs/strcompress-strescape", test_strcompress_strescape);
  g_test_add_func ("/strfuncs/strconcat", test_strconcat);
  g_test_add_func ("/strfuncs/strdelimit", test_strdelimit);
  g_test_add_func ("/strfuncs/strdup", test_strdup);
  g_test_add_func ("/strfuncs/strdup-printf", test_strdup_printf);
  g_test_add_func ("/strfuncs/strdupv", test_strdupv);
  g_test_add_func ("/strfuncs/strerror", test_strerror);
  g_test_add_func ("/strfuncs/strip-context", test_strip_context);
  g_test_add_func ("/strfuncs/strjoin", test_strjoin);
  g_test_add_func ("/strfuncs/strjoinv", test_strjoinv);
  g_test_add_func ("/strfuncs/strlcat", test_strlcat);
  g_test_add_func ("/strfuncs/strlcpy", test_strlcpy);
  g_test_add_func ("/strfuncs/strncasecmp", test_strncasecmp);
  g_test_add_func ("/strfuncs/strndup", test_strndup);
  g_test_add_func ("/strfuncs/strnfill", test_strnfill);
  g_test_add_func ("/strfuncs/strreverse", test_strreverse);
  g_test_add_func ("/strfuncs/strsignal", test_strsignal);
  g_test_add_func ("/strfuncs/strsplit", test_strsplit);
  g_test_add_func ("/strfuncs/strsplit-set", test_strsplit_set);
  g_test_add_func ("/strfuncs/strstr", test_strstr);
  g_test_add_func ("/strfuncs/strtod", test_strtod);
  g_test_add_func ("/strfuncs/strtoull-strtoll", test_strtoll);
  g_test_add_func ("/strfuncs/strup", test_strup);
  g_test_add_func ("/strfuncs/strv-contains", test_strv_contains);
  g_test_add_func ("/strfuncs/strv-equal", test_strv_equal);
  g_test_add_func ("/strfuncs/strv-length", test_strv_length);
  g_test_add_func ("/strfuncs/test-is-to-digit", test_is_to_digit);
  g_test_add_func ("/strfuncs/transliteration", test_transliteration);

  return g_test_run();
}
