/* guniprop.c - Unicode character properties.
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <locale.h>

#include "gmem.h"
#include "gstring.h"
#include "gtestutils.h"
#include "gtypes.h"
#include "gunicode.h"
#include "gunichartables.h"
#include "gmirroringtable.h"
#include "gscripttable.h"
#include "gunicodeprivate.h"
#ifdef G_OS_WIN32
#include "gwin32.h"
#endif

#define G_UNICHAR_FULLWIDTH_A 0xff21
#define G_UNICHAR_FULLWIDTH_I 0xff29
#define G_UNICHAR_FULLWIDTH_J 0xff2a
#define G_UNICHAR_FULLWIDTH_F 0xff26
#define G_UNICHAR_FULLWIDTH_a 0xff41
#define G_UNICHAR_FULLWIDTH_f 0xff46

#define ATTR_TABLE(Page) (((Page) <= XUNICODE_LAST_PAGE_PART1) \
                          ? attr_table_part1[Page] \
                          : attr_table_part2[(Page) - 0xe00])

#define ATTTABLE(Page, Char) \
  ((ATTR_TABLE(Page) == XUNICODE_MAX_TABLE_INDEX) ? 0 : (attr_data[ATTR_TABLE(Page)][Char]))

#define TTYPE_PART1(Page, Char) \
  ((type_table_part1[Page] >= XUNICODE_MAX_TABLE_INDEX) \
   ? (type_table_part1[Page] - XUNICODE_MAX_TABLE_INDEX) \
   : (type_data[type_table_part1[Page]][Char]))

#define TTYPE_PART2(Page, Char) \
  ((type_table_part2[Page] >= XUNICODE_MAX_TABLE_INDEX) \
   ? (type_table_part2[Page] - XUNICODE_MAX_TABLE_INDEX) \
   : (type_data[type_table_part2[Page]][Char]))

#define TYPE(Char) \
  (((Char) <= XUNICODE_LAST_CHAR_PART1) \
   ? TTYPE_PART1 ((Char) >> 8, (Char) & 0xff) \
   : (((Char) >= 0xe0000 && (Char) <= XUNICODE_LAST_CHAR) \
      ? TTYPE_PART2 (((Char) - 0xe0000) >> 8, (Char) & 0xff) \
      : XUNICODE_UNASSIGNED))


#define IS(Type, Class)	(((xuint_t)1 << (Type)) & (Class))
#define OR(Type, Rest)	(((xuint_t)1 << (Type)) | (Rest))



#define ISALPHA(Type)	IS ((Type),				\
			    OR (XUNICODE_LOWERCASE_LETTER,	\
			    OR (XUNICODE_UPPERCASE_LETTER,	\
			    OR (XUNICODE_TITLECASE_LETTER,	\
			    OR (XUNICODE_MODIFIER_LETTER,	\
			    OR (XUNICODE_OTHER_LETTER,		0))))))

#define ISALDIGIT(Type)	IS ((Type),				\
			    OR (XUNICODE_DECIMAL_NUMBER,	\
			    OR (XUNICODE_LETTER_NUMBER,	\
			    OR (XUNICODE_OTHER_NUMBER,		\
			    OR (XUNICODE_LOWERCASE_LETTER,	\
			    OR (XUNICODE_UPPERCASE_LETTER,	\
			    OR (XUNICODE_TITLECASE_LETTER,	\
			    OR (XUNICODE_MODIFIER_LETTER,	\
			    OR (XUNICODE_OTHER_LETTER,		0)))))))))

#define ISMARK(Type)	IS ((Type),				\
			    OR (XUNICODE_NON_SPACING_MARK,	\
			    OR (XUNICODE_SPACING_MARK,	\
			    OR (XUNICODE_ENCLOSING_MARK,	0))))

#define ISZEROWIDTHTYPE(Type)	IS ((Type),			\
			    OR (XUNICODE_NON_SPACING_MARK,	\
			    OR (XUNICODE_ENCLOSING_MARK,	\
			    OR (XUNICODE_FORMAT,		0))))

/**
 * xunichar_isalnum:
 * @c: a Unicode character
 *
 * Determines whether a character is alphanumeric.
 * Given some UTF-8 text, obtain a character value
 * with xutf8_get_char().
 *
 * Returns: %TRUE if @c is an alphanumeric character
 **/
xboolean_t
xunichar_isalnum (xunichar_t c)
{
  return ISALDIGIT (TYPE (c)) ? TRUE : FALSE;
}

/**
 * xunichar_isalpha:
 * @c: a Unicode character
 *
 * Determines whether a character is alphabetic (i.e. a letter).
 * Given some UTF-8 text, obtain a character value with
 * xutf8_get_char().
 *
 * Returns: %TRUE if @c is an alphabetic character
 **/
xboolean_t
xunichar_isalpha (xunichar_t c)
{
  return ISALPHA (TYPE (c)) ? TRUE : FALSE;
}


/**
 * xunichar_iscntrl:
 * @c: a Unicode character
 *
 * Determines whether a character is a control character.
 * Given some UTF-8 text, obtain a character value with
 * xutf8_get_char().
 *
 * Returns: %TRUE if @c is a control character
 **/
xboolean_t
xunichar_iscntrl (xunichar_t c)
{
  return TYPE (c) == XUNICODE_CONTROL;
}

/**
 * xunichar_isdigit:
 * @c: a Unicode character
 *
 * Determines whether a character is numeric (i.e. a digit).  This
 * covers ASCII 0-9 and also digits in other languages/scripts.  Given
 * some UTF-8 text, obtain a character value with xutf8_get_char().
 *
 * Returns: %TRUE if @c is a digit
 **/
xboolean_t
xunichar_isdigit (xunichar_t c)
{
  return TYPE (c) == XUNICODE_DECIMAL_NUMBER;
}


/**
 * xunichar_isgraph:
 * @c: a Unicode character
 *
 * Determines whether a character is printable and not a space
 * (returns %FALSE for control characters, format characters, and
 * spaces). xunichar_isprint() is similar, but returns %TRUE for
 * spaces. Given some UTF-8 text, obtain a character value with
 * xutf8_get_char().
 *
 * Returns: %TRUE if @c is printable unless it's a space
 **/
xboolean_t
xunichar_isgraph (xunichar_t c)
{
  return !IS (TYPE(c),
	      OR (XUNICODE_CONTROL,
	      OR (XUNICODE_FORMAT,
	      OR (XUNICODE_UNASSIGNED,
	      OR (XUNICODE_SURROGATE,
	      OR (XUNICODE_SPACE_SEPARATOR,
	     0))))));
}

/**
 * xunichar_islower:
 * @c: a Unicode character
 *
 * Determines whether a character is a lowercase letter.
 * Given some UTF-8 text, obtain a character value with
 * xutf8_get_char().
 *
 * Returns: %TRUE if @c is a lowercase letter
 **/
xboolean_t
xunichar_islower (xunichar_t c)
{
  return TYPE (c) == XUNICODE_LOWERCASE_LETTER;
}


/**
 * xunichar_isprint:
 * @c: a Unicode character
 *
 * Determines whether a character is printable.
 * Unlike xunichar_isgraph(), returns %TRUE for spaces.
 * Given some UTF-8 text, obtain a character value with
 * xutf8_get_char().
 *
 * Returns: %TRUE if @c is printable
 **/
xboolean_t
xunichar_isprint (xunichar_t c)
{
  return !IS (TYPE(c),
	      OR (XUNICODE_CONTROL,
	      OR (XUNICODE_FORMAT,
	      OR (XUNICODE_UNASSIGNED,
	      OR (XUNICODE_SURROGATE,
	     0)))));
}

/**
 * xunichar_ispunct:
 * @c: a Unicode character
 *
 * Determines whether a character is punctuation or a symbol.
 * Given some UTF-8 text, obtain a character value with
 * xutf8_get_char().
 *
 * Returns: %TRUE if @c is a punctuation or symbol character
 **/
xboolean_t
xunichar_ispunct (xunichar_t c)
{
  return IS (TYPE(c),
	     OR (XUNICODE_CONNECT_PUNCTUATION,
	     OR (XUNICODE_DASH_PUNCTUATION,
	     OR (XUNICODE_CLOSE_PUNCTUATION,
	     OR (XUNICODE_FINAL_PUNCTUATION,
	     OR (XUNICODE_INITIAL_PUNCTUATION,
	     OR (XUNICODE_OTHER_PUNCTUATION,
	     OR (XUNICODE_OPEN_PUNCTUATION,
	     OR (XUNICODE_CURRENCY_SYMBOL,
	     OR (XUNICODE_MODIFIER_SYMBOL,
	     OR (XUNICODE_MATH_SYMBOL,
	     OR (XUNICODE_OTHER_SYMBOL,
	    0)))))))))))) ? TRUE : FALSE;
}

/**
 * xunichar_isspace:
 * @c: a Unicode character
 *
 * Determines whether a character is a space, tab, or line separator
 * (newline, carriage return, etc.).  Given some UTF-8 text, obtain a
 * character value with xutf8_get_char().
 *
 * (Note: don't use this to do word breaking; you have to use
 * Pango or equivalent to get word breaking right, the algorithm
 * is fairly complex.)
 *
 * Returns: %TRUE if @c is a space character
 **/
xboolean_t
xunichar_isspace (xunichar_t c)
{
  switch (c)
    {
      /* special-case these since Unicode thinks they are not spaces */
    case '\t':
    case '\n':
    case '\r':
    case '\f':
      return TRUE;
      break;

    default:
      {
	return IS (TYPE(c),
	           OR (XUNICODE_SPACE_SEPARATOR,
	           OR (XUNICODE_LINE_SEPARATOR,
                   OR (XUNICODE_PARAGRAPH_SEPARATOR,
		  0)))) ? TRUE : FALSE;
      }
      break;
    }
}

/**
 * xunichar_ismark:
 * @c: a Unicode character
 *
 * Determines whether a character is a mark (non-spacing mark,
 * combining mark, or enclosing mark in Unicode speak).
 * Given some UTF-8 text, obtain a character value
 * with xutf8_get_char().
 *
 * Note: in most cases where isalpha characters are allowed,
 * ismark characters should be allowed to as they are essential
 * for writing most European languages as well as many non-Latin
 * scripts.
 *
 * Returns: %TRUE if @c is a mark character
 *
 * Since: 2.14
 **/
xboolean_t
xunichar_ismark (xunichar_t c)
{
  return ISMARK (TYPE (c));
}

/**
 * xunichar_isupper:
 * @c: a Unicode character
 *
 * Determines if a character is uppercase.
 *
 * Returns: %TRUE if @c is an uppercase character
 **/
xboolean_t
xunichar_isupper (xunichar_t c)
{
  return TYPE (c) == XUNICODE_UPPERCASE_LETTER;
}

/**
 * xunichar_istitle:
 * @c: a Unicode character
 *
 * Determines if a character is titlecase. Some characters in
 * Unicode which are composites, such as the DZ digraph
 * have three case variants instead of just two. The titlecase
 * form is used at the beginning of a word where only the
 * first letter is capitalized. The titlecase form of the DZ
 * digraph is U+01F2 LATIN CAPITAL LETTTER D WITH SMALL LETTER Z.
 *
 * Returns: %TRUE if the character is titlecase
 **/
xboolean_t
xunichar_istitle (xunichar_t c)
{
  unsigned int i;
  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
    if (title_table[i][0] == c)
      return TRUE;
  return FALSE;
}

/**
 * xunichar_isxdigit:
 * @c: a Unicode character.
 *
 * Determines if a character is a hexadecimal digit.
 *
 * Returns: %TRUE if the character is a hexadecimal digit
 **/
xboolean_t
xunichar_isxdigit (xunichar_t c)
{
  return ((c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F') ||
          (c >= G_UNICHAR_FULLWIDTH_a && c <= G_UNICHAR_FULLWIDTH_f) ||
          (c >= G_UNICHAR_FULLWIDTH_A && c <= G_UNICHAR_FULLWIDTH_F) ||
          (TYPE (c) == XUNICODE_DECIMAL_NUMBER));
}

/**
 * xunichar_isdefined:
 * @c: a Unicode character
 *
 * Determines if a given character is assigned in the Unicode
 * standard.
 *
 * Returns: %TRUE if the character has an assigned value
 **/
xboolean_t
xunichar_isdefined (xunichar_t c)
{
  return !IS (TYPE(c),
	      OR (XUNICODE_UNASSIGNED,
	      OR (XUNICODE_SURROGATE,
	     0)));
}

/**
 * xunichar_iszerowidth:
 * @c: a Unicode character
 *
 * Determines if a given character typically takes zero width when rendered.
 * The return value is %TRUE for all non-spacing and enclosing marks
 * (e.g., combining accents), format characters, zero-width
 * space, but not U+00AD SOFT HYPHEN.
 *
 * A typical use of this function is with one of xunichar_iswide() or
 * xunichar_iswide_cjk() to determine the number of cells a string occupies
 * when displayed on a grid display (terminals).  However, note that not all
 * terminals support zero-width rendering of zero-width marks.
 *
 * Returns: %TRUE if the character has zero width
 *
 * Since: 2.14
 **/
xboolean_t
xunichar_iszerowidth (xunichar_t c)
{
  if (G_UNLIKELY (c == 0x00AD))
    return FALSE;

  if (G_UNLIKELY (ISZEROWIDTHTYPE (TYPE (c))))
    return TRUE;

  /* A few additional codepoints are zero-width:
   *  - Part of the Hangul Jamo block covering medial/vowels/jungseong and
   *    final/trailing_consonants/jongseong Jamo
   *  - Jungseong and jongseong for Old Korean
   *  - Zero-width space (U+200B)
   */
  if (G_UNLIKELY ((c >= 0x1160 && c < 0x1200) ||
                  (c >= 0xD7B0 && c < 0xD800) ||
                  c == 0x200B))
    return TRUE;

  return FALSE;
}

static int
interval_compare (const void *key, const void *elt)
{
  xunichar_t c = GPOINTER_TO_UINT (key);
  struct Interval *interval = (struct Interval *)elt;

  if (c < interval->start)
    return -1;
  if (c > interval->end)
    return +1;

  return 0;
}

#define G_WIDTH_TABLE_MIDPOINT (G_N_ELEMENTS (xunicode_width_table_wide) / 2)

static inline xboolean_t
xunichar_iswide_bsearch (xunichar_t ch)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (xunicode_width_table_wide) - 1;
  static int saved_mid = G_WIDTH_TABLE_MIDPOINT;
  int mid = saved_mid;

  do
    {
      if (ch < xunicode_width_table_wide[mid].start)
	upper = mid - 1;
      else if (ch > xunicode_width_table_wide[mid].end)
	lower = mid + 1;
      else
	return TRUE;

      mid = (lower + upper) / 2;
    }
  while (lower <= upper);

  return FALSE;
}

/**
 * xunichar_iswide:
 * @c: a Unicode character
 *
 * Determines if a character is typically rendered in a double-width
 * cell.
 *
 * Returns: %TRUE if the character is wide
 **/
xboolean_t
xunichar_iswide (xunichar_t c)
{
  if (c < xunicode_width_table_wide[0].start)
    return FALSE;
  else
    return xunichar_iswide_bsearch (c);
}


/**
 * xunichar_iswide_cjk:
 * @c: a Unicode character
 *
 * Determines if a character is typically rendered in a double-width
 * cell under legacy East Asian locales.  If a character is wide according to
 * xunichar_iswide(), then it is also reported wide with this function, but
 * the converse is not necessarily true. See the
 * [Unicode Standard Annex #11](http://www.unicode.org/reports/tr11/)
 * for details.
 *
 * If a character passes the xunichar_iswide() test then it will also pass
 * this test, but not the other way around.  Note that some characters may
 * pass both this test and xunichar_iszerowidth().
 *
 * Returns: %TRUE if the character is wide in legacy East Asian locales
 *
 * Since: 2.12
 */
xboolean_t
xunichar_iswide_cjk (xunichar_t c)
{
  if (xunichar_iswide (c))
    return TRUE;

  /* bsearch() is declared attribute(nonnull(1)) so we can't validly search
   * for a NULL key */
  if (c == 0)
    return FALSE;

  if (bsearch (GUINT_TO_POINTER (c),
               xunicode_width_table_ambiguous,
               G_N_ELEMENTS (xunicode_width_table_ambiguous),
               sizeof xunicode_width_table_ambiguous[0],
	       interval_compare))
    return TRUE;

  return FALSE;
}


/**
 * xunichar_toupper:
 * @c: a Unicode character
 *
 * Converts a character to uppercase.
 *
 * Returns: the result of converting @c to uppercase.
 *               If @c is not a lowercase or titlecase character,
 *               or has no upper case equivalent @c is returned unchanged.
 **/
xunichar_t
xunichar_toupper (xunichar_t c)
{
  int t = TYPE (c);
  if (t == XUNICODE_LOWERCASE_LETTER)
    {
      xunichar_t val = ATTTABLE (c >> 8, c & 0xff);
      if (val >= 0x1000000)
	{
	  const xchar_t *p = special_case_table + val - 0x1000000;
          val = xutf8_get_char (p);
	}
      /* Some lowercase letters, e.g., U+000AA, FEMININE ORDINAL INDICATOR,
       * do not have an uppercase equivalent, in which case val will be
       * zero.
       */
      return val ? val : c;
    }
  else if (t == XUNICODE_TITLECASE_LETTER)
    {
      unsigned int i;
      for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
	{
	  if (title_table[i][0] == c)
	    return title_table[i][1] ? title_table[i][1] : c;
	}
    }
  return c;
}

/**
 * xunichar_tolower:
 * @c: a Unicode character.
 *
 * Converts a character to lower case.
 *
 * Returns: the result of converting @c to lower case.
 *               If @c is not an upperlower or titlecase character,
 *               or has no lowercase equivalent @c is returned unchanged.
 **/
xunichar_t
xunichar_tolower (xunichar_t c)
{
  int t = TYPE (c);
  if (t == XUNICODE_UPPERCASE_LETTER)
    {
      xunichar_t val = ATTTABLE (c >> 8, c & 0xff);
      if (val >= 0x1000000)
	{
	  const xchar_t *p = special_case_table + val - 0x1000000;
	  return xutf8_get_char (p);
	}
      else
	{
	  /* Not all uppercase letters are guaranteed to have a lowercase
	   * equivalent.  If this is the case, val will be zero. */
	  return val ? val : c;
	}
    }
  else if (t == XUNICODE_TITLECASE_LETTER)
    {
      unsigned int i;
      for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
	{
	  if (title_table[i][0] == c)
	    return title_table[i][2];
	}
    }
  return c;
}

/**
 * xunichar_totitle:
 * @c: a Unicode character
 *
 * Converts a character to the titlecase.
 *
 * Returns: the result of converting @c to titlecase.
 *               If @c is not an uppercase or lowercase character,
 *               @c is returned unchanged.
 **/
xunichar_t
xunichar_totitle (xunichar_t c)
{
  unsigned int i;

  /* We handle U+0000 explicitly because some elements in
   * title_table[i][1] may be null. */
  if (c == 0)
    return c;

  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
    {
      if (title_table[i][0] == c || title_table[i][1] == c
	  || title_table[i][2] == c)
	return title_table[i][0];
    }

  if (TYPE (c) == XUNICODE_LOWERCASE_LETTER)
    return xunichar_toupper (c);

  return c;
}

/**
 * xunichar_digit_value:
 * @c: a Unicode character
 *
 * Determines the numeric value of a character as a decimal
 * digit.
 *
 * Returns: If @c is a decimal digit (according to
 * xunichar_isdigit()), its numeric value. Otherwise, -1.
 **/
int
xunichar_digit_value (xunichar_t c)
{
  if (TYPE (c) == XUNICODE_DECIMAL_NUMBER)
    return ATTTABLE (c >> 8, c & 0xff);
  return -1;
}

/**
 * xunichar_xdigit_value:
 * @c: a Unicode character
 *
 * Determines the numeric value of a character as a hexadecimal
 * digit.
 *
 * Returns: If @c is a hex digit (according to
 * xunichar_isxdigit()), its numeric value. Otherwise, -1.
 **/
int
xunichar_xdigit_value (xunichar_t c)
{
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= G_UNICHAR_FULLWIDTH_A && c <= G_UNICHAR_FULLWIDTH_F)
    return c - G_UNICHAR_FULLWIDTH_A + 10;
  if (c >= G_UNICHAR_FULLWIDTH_a && c <= G_UNICHAR_FULLWIDTH_f)
    return c - G_UNICHAR_FULLWIDTH_a + 10;
  if (TYPE (c) == XUNICODE_DECIMAL_NUMBER)
    return ATTTABLE (c >> 8, c & 0xff);
  return -1;
}

/**
 * xunichar_type:
 * @c: a Unicode character
 *
 * Classifies a Unicode character by type.
 *
 * Returns: the type of the character.
 **/
xunicode_type_t
xunichar_type (xunichar_t c)
{
  return TYPE (c);
}

/*
 * Case mapping functions
 */

typedef enum {
  LOCALE_NORMAL,
  LOCALE_TURKIC,
  LOCALE_LITHUANIAN
} LocaleType;

static LocaleType
get_locale_type (void)
{
#ifdef G_OS_WIN32
  char *tem = g_win32_getlocale ();
  char locale[2];

  locale[0] = tem[0];
  locale[1] = tem[1];
  g_free (tem);
#else
  const char *locale = setlocale (LC_CTYPE, NULL);

  if (locale == NULL)
    return LOCALE_NORMAL;
#endif

  switch (locale[0])
    {
   case 'a':
      if (locale[1] == 'z')
	return LOCALE_TURKIC;
      break;
    case 'l':
      if (locale[1] == 't')
	return LOCALE_LITHUANIAN;
      break;
    case 't':
      if (locale[1] == 'r')
	return LOCALE_TURKIC;
      break;
    }

  return LOCALE_NORMAL;
}

static xint_t
output_marks (const char **p_inout,
	      char        *out_buffer,
	      xboolean_t     remove_dot)
{
  const char *p = *p_inout;
  xint_t len = 0;

  while (*p)
    {
      xunichar_t c = xutf8_get_char (p);

      if (ISMARK (TYPE (c)))
	{
	  if (!remove_dot || c != 0x307 /* COMBINING DOT ABOVE */)
	    len += xunichar_to_utf8 (c, out_buffer ? out_buffer + len : NULL);
	  p = xutf8_next_char (p);
	}
      else
	break;
    }

  *p_inout = p;
  return len;
}

static xint_t
output_special_case (xchar_t *out_buffer,
		     int    offset,
		     int    type,
		     int    which)
{
  const xchar_t *p = special_case_table + offset;
  xint_t len;

  if (type != XUNICODE_TITLECASE_LETTER)
    p = xutf8_next_char (p);

  if (which == 1)
    p += strlen (p) + 1;

  len = strlen (p);
  if (out_buffer)
    memcpy (out_buffer, p, len);

  return len;
}

static xsize_t
real_toupper (const xchar_t *str,
	      xssize_t       max_len,
	      xchar_t       *out_buffer,
	      LocaleType   locale_type)
{
  const xchar_t *p = str;
  const char *last = NULL;
  xsize_t len = 0;
  xboolean_t last_was_i = FALSE;

  while ((max_len < 0 || p < str + max_len) && *p)
    {
      xunichar_t c = xutf8_get_char (p);
      int t = TYPE (c);
      xunichar_t val;

      last = p;
      p = xutf8_next_char (p);

      if (locale_type == LOCALE_LITHUANIAN)
	{
	  if (c == 'i')
	    last_was_i = TRUE;
	  else
	    {
	      if (last_was_i)
		{
		  /* Nasty, need to remove any dot above. Though
		   * I think only E WITH DOT ABOVE occurs in practice
		   * which could simplify this considerably.
		   */
		  xsize_t decomp_len, i;
		  xunichar_t decomp[G_UNICHAR_MAX_DECOMPOSITION_LENGTH];

		  decomp_len = xunichar_fully_decompose (c, FALSE, decomp, G_N_ELEMENTS (decomp));
		  for (i=0; i < decomp_len; i++)
		    {
		      if (decomp[i] != 0x307 /* COMBINING DOT ABOVE */)
			len += xunichar_to_utf8 (xunichar_toupper (decomp[i]), out_buffer ? out_buffer + len : NULL);
		    }

		  len += output_marks (&p, out_buffer ? out_buffer + len : NULL, TRUE);

		  continue;
		}

	      if (!ISMARK (t))
		last_was_i = FALSE;
	    }
	}

      if (locale_type == LOCALE_TURKIC && c == 'i')
	{
	  /* i => LATIN CAPITAL LETTER I WITH DOT ABOVE */
	  len += xunichar_to_utf8 (0x130, out_buffer ? out_buffer + len : NULL);
	}
      else if (c == 0x0345)	/* COMBINING GREEK YPOGEGRAMMENI */
	{
	  /* Nasty, need to move it after other combining marks .. this would go away if
	   * we normalized first.
	   */
	  len += output_marks (&p, out_buffer ? out_buffer + len : NULL, FALSE);

	  /* And output as GREEK CAPITAL LETTER IOTA */
	  len += xunichar_to_utf8 (0x399, out_buffer ? out_buffer + len : NULL);
	}
      else if (IS (t,
		   OR (XUNICODE_LOWERCASE_LETTER,
		   OR (XUNICODE_TITLECASE_LETTER,
		  0))))
	{
	  val = ATTTABLE (c >> 8, c & 0xff);

	  if (val >= 0x1000000)
	    {
	      len += output_special_case (out_buffer ? out_buffer + len : NULL, val - 0x1000000, t,
					  t == XUNICODE_LOWERCASE_LETTER ? 0 : 1);
	    }
	  else
	    {
	      if (t == XUNICODE_TITLECASE_LETTER)
		{
		  unsigned int i;
		  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
		    {
		      if (title_table[i][0] == c)
			{
			  val = title_table[i][1];
			  break;
			}
		    }
		}

	      /* Some lowercase letters, e.g., U+000AA, FEMININE ORDINAL INDICATOR,
	       * do not have an uppercase equivalent, in which case val will be
	       * zero. */
	      len += xunichar_to_utf8 (val ? val : c, out_buffer ? out_buffer + len : NULL);
	    }
	}
      else
	{
	  xsize_t char_len = xutf8_skip[*(xuchar_t *)last];

	  if (out_buffer)
	    memcpy (out_buffer + len, last, char_len);

	  len += char_len;
	}

    }

  return len;
}

/**
 * xutf8_strup:
 * @str: a UTF-8 encoded string
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 *
 * Converts all Unicode characters in the string that have a case
 * to uppercase. The exact manner that this is done depends
 * on the current locale, and may result in the number of
 * characters in the string increasing. (For instance, the
 * German ess-zet will be changed to SS.)
 *
 * Returns: a newly allocated string, with all characters
 *    converted to uppercase.
 **/
xchar_t *
xutf8_strup (const xchar_t *str,
	      xssize_t       len)
{
  xsize_t result_len;
  LocaleType locale_type;
  xchar_t *result;

  xreturn_val_if_fail (str != NULL, NULL);

  locale_type = get_locale_type ();

  /*
   * We use a two pass approach to keep memory management simple
   */
  result_len = real_toupper (str, len, NULL, locale_type);
  result = g_malloc (result_len + 1);
  real_toupper (str, len, result, locale_type);
  result[result_len] = '\0';

  return result;
}

/* traverses the string checking for characters with combining class == 230
 * until a base character is found */
static xboolean_t
has_more_above (const xchar_t *str)
{
  const xchar_t *p = str;
  xint_t combining_class;

  while (*p)
    {
      combining_class = xunichar_combining_class (xutf8_get_char (p));
      if (combining_class == 230)
        return TRUE;
      else if (combining_class == 0)
        break;

      p = xutf8_next_char (p);
    }

  return FALSE;
}

static xsize_t
real_tolower (const xchar_t *str,
	      xssize_t       max_len,
	      xchar_t       *out_buffer,
	      LocaleType   locale_type)
{
  const xchar_t *p = str;
  const char *last = NULL;
  xsize_t len = 0;

  while ((max_len < 0 || p < str + max_len) && *p)
    {
      xunichar_t c = xutf8_get_char (p);
      int t = TYPE (c);
      xunichar_t val;

      last = p;
      p = xutf8_next_char (p);

      if (locale_type == LOCALE_TURKIC && (c == 'I' || c == 0x130 ||
                                           c == G_UNICHAR_FULLWIDTH_I))
        {
          xboolean_t combining_dot = (c == 'I' || c == G_UNICHAR_FULLWIDTH_I) &&
                                   xutf8_get_char (p) == 0x0307;
          if (combining_dot || c == 0x130)
            {
              /* I + COMBINING DOT ABOVE => i (U+0069)
               * LATIN CAPITAL LETTER I WITH DOT ABOVE => i (U+0069) */
              len += xunichar_to_utf8 (0x0069, out_buffer ? out_buffer + len : NULL);
              if (combining_dot)
                p = xutf8_next_char (p);
            }
          else
            {
              /* I => LATIN SMALL LETTER DOTLESS I */
              len += xunichar_to_utf8 (0x131, out_buffer ? out_buffer + len : NULL);
            }
        }
      /* Introduce an explicit dot above when lowercasing capital I's and J's
       * whenever there are more accents above. [SpecialCasing.txt] */
      else if (locale_type == LOCALE_LITHUANIAN &&
               (c == 0x00cc || c == 0x00cd || c == 0x0128))
        {
          len += xunichar_to_utf8 (0x0069, out_buffer ? out_buffer + len : NULL);
          len += xunichar_to_utf8 (0x0307, out_buffer ? out_buffer + len : NULL);

          switch (c)
            {
            case 0x00cc:
              len += xunichar_to_utf8 (0x0300, out_buffer ? out_buffer + len : NULL);
              break;
            case 0x00cd:
              len += xunichar_to_utf8 (0x0301, out_buffer ? out_buffer + len : NULL);
              break;
            case 0x0128:
              len += xunichar_to_utf8 (0x0303, out_buffer ? out_buffer + len : NULL);
              break;
            }
        }
      else if (locale_type == LOCALE_LITHUANIAN &&
               (c == 'I' || c == G_UNICHAR_FULLWIDTH_I ||
                c == 'J' || c == G_UNICHAR_FULLWIDTH_J || c == 0x012e) &&
               has_more_above (p))
        {
          len += xunichar_to_utf8 (xunichar_tolower (c), out_buffer ? out_buffer + len : NULL);
          len += xunichar_to_utf8 (0x0307, out_buffer ? out_buffer + len : NULL);
        }
      else if (c == 0x03A3)	/* GREEK CAPITAL LETTER SIGMA */
	{
	  if ((max_len < 0 || p < str + max_len) && *p)
	    {
	      xunichar_t next_c = xutf8_get_char (p);
	      int next_type = TYPE(next_c);

	      /* SIGMA mapps differently depending on whether it is
	       * final or not. The following simplified test would
	       * fail in the case of combining marks following the
	       * sigma, but I don't think that occurs in real text.
	       * The test here matches that in ICU.
	       */
	      if (ISALPHA (next_type)) /* Lu,Ll,Lt,Lm,Lo */
		val = 0x3c3;	/* GREEK SMALL SIGMA */
	      else
		val = 0x3c2;	/* GREEK SMALL FINAL SIGMA */
	    }
	  else
	    val = 0x3c2;	/* GREEK SMALL FINAL SIGMA */

	  len += xunichar_to_utf8 (val, out_buffer ? out_buffer + len : NULL);
	}
      else if (IS (t,
		   OR (XUNICODE_UPPERCASE_LETTER,
		   OR (XUNICODE_TITLECASE_LETTER,
		  0))))
	{
	  val = ATTTABLE (c >> 8, c & 0xff);

	  if (val >= 0x1000000)
	    {
	      len += output_special_case (out_buffer ? out_buffer + len : NULL, val - 0x1000000, t, 0);
	    }
	  else
	    {
	      if (t == XUNICODE_TITLECASE_LETTER)
		{
		  unsigned int i;
		  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
		    {
		      if (title_table[i][0] == c)
			{
			  val = title_table[i][2];
			  break;
			}
		    }
		}

	      /* Not all uppercase letters are guaranteed to have a lowercase
	       * equivalent.  If this is the case, val will be zero. */
	      len += xunichar_to_utf8 (val ? val : c, out_buffer ? out_buffer + len : NULL);
	    }
	}
      else
	{
	  xsize_t char_len = xutf8_skip[*(xuchar_t *)last];

	  if (out_buffer)
	    memcpy (out_buffer + len, last, char_len);

	  len += char_len;
	}

    }

  return len;
}

/**
 * xutf8_strdown:
 * @str: a UTF-8 encoded string
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 *
 * Converts all Unicode characters in the string that have a case
 * to lowercase. The exact manner that this is done depends
 * on the current locale, and may result in the number of
 * characters in the string changing.
 *
 * Returns: a newly allocated string, with all characters
 *    converted to lowercase.
 **/
xchar_t *
xutf8_strdown (const xchar_t *str,
		xssize_t       len)
{
  xsize_t result_len;
  LocaleType locale_type;
  xchar_t *result;

  xreturn_val_if_fail (str != NULL, NULL);

  locale_type = get_locale_type ();

  /*
   * We use a two pass approach to keep memory management simple
   */
  result_len = real_tolower (str, len, NULL, locale_type);
  result = g_malloc (result_len + 1);
  real_tolower (str, len, result, locale_type);
  result[result_len] = '\0';

  return result;
}

/**
 * xutf8_casefold:
 * @str: a UTF-8 encoded string
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 *
 * Converts a string into a form that is independent of case. The
 * result will not correspond to any particular case, but can be
 * compared for equality or ordered with the results of calling
 * xutf8_casefold() on other strings.
 *
 * Note that calling xutf8_casefold() followed by xutf8_collate() is
 * only an approximation to the correct linguistic case insensitive
 * ordering, though it is a fairly good one. Getting this exactly
 * right would require a more sophisticated collation function that
 * takes case sensitivity into account. GLib does not currently
 * provide such a function.
 *
 * Returns: a newly allocated string, that is a
 *   case independent form of @str.
 **/
xchar_t *
xutf8_casefold (const xchar_t *str,
		 xssize_t       len)
{
  xstring_t *result;
  const char *p;

  xreturn_val_if_fail (str != NULL, NULL);

  result = xstring_new (NULL);
  p = str;
  while ((len < 0 || p < str + len) && *p)
    {
      xunichar_t ch = xutf8_get_char (p);

      int start = 0;
      int end = G_N_ELEMENTS (casefold_table);

      if (ch >= casefold_table[start].ch &&
          ch <= casefold_table[end - 1].ch)
	{
	  while (TRUE)
	    {
	      int half = (start + end) / 2;
	      if (ch == casefold_table[half].ch)
		{
		  xstring_append (result, casefold_table[half].data);
		  goto next;
		}
	      else if (half == start)
		break;
	      else if (ch > casefold_table[half].ch)
		start = half;
	      else
		end = half;
	    }
	}

      xstring_append_unichar (result, xunichar_tolower (ch));

    next:
      p = xutf8_next_char (p);
    }

  return xstring_free (result, FALSE);
}

/**
 * xunichar_get_mirror_char:
 * @ch: a Unicode character
 * @mirrored_ch: location to store the mirrored character
 *
 * In Unicode, some characters are "mirrored". This means that their
 * images are mirrored horizontally in text that is laid out from right
 * to left. For instance, "(" would become its mirror image, ")", in
 * right-to-left text.
 *
 * If @ch has the Unicode mirrored property and there is another unicode
 * character that typically has a glyph that is the mirror image of @ch's
 * glyph and @mirrored_ch is set, it puts that character in the address
 * pointed to by @mirrored_ch.  Otherwise the original character is put.
 *
 * Returns: %TRUE if @ch has a mirrored character, %FALSE otherwise
 *
 * Since: 2.4
 **/
xboolean_t
xunichar_get_mirror_char (xunichar_t ch,
                           xunichar_t *mirrored_ch)
{
  xboolean_t found;
  xunichar_t mirrored;

  mirrored = XPL_GET_MIRRORING(ch);

  found = ch != mirrored;
  if (mirrored_ch)
    *mirrored_ch = mirrored;

  return found;

}

#define G_SCRIPT_TABLE_MIDPOINT (G_N_ELEMENTS (g_script_table) / 2)

static inline xunicode_script_t
xunichar_get_script_bsearch (xunichar_t ch)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (g_script_table) - 1;
  static int saved_mid = G_SCRIPT_TABLE_MIDPOINT;
  int mid = saved_mid;


  do
    {
      if (ch < g_script_table[mid].start)
	upper = mid - 1;
      else if (ch >= g_script_table[mid].start + g_script_table[mid].chars)
	lower = mid + 1;
      else
	return g_script_table[saved_mid = mid].script;

      mid = (lower + upper) / 2;
    }
  while (lower <= upper);

  return XUNICODE_SCRIPT_UNKNOWN;
}

/**
 * xunichar_get_script:
 * @ch: a Unicode character
 *
 * Looks up the #xunicode_script_t for a particular character (as defined
 * by Unicode Standard Annex \#24). No check is made for @ch being a
 * valid Unicode character; if you pass in invalid character, the
 * result is undefined.
 *
 * This function is equivalent to pango_script_for_unichar() and the
 * two are interchangeable.
 *
 * Returns: the #xunicode_script_t for the character.
 *
 * Since: 2.14
 */
xunicode_script_t
xunichar_get_script (xunichar_t ch)
{
  if (ch < G_EASY_SCRIPTS_RANGE)
    return g_script_easy_table[ch];
  else
    return xunichar_get_script_bsearch (ch);
}


/* http://unicode.org/iso15924/ */
static const xuint32_t iso15924_tags[] =
{
#define PACK(a,b,c,d) ((xuint32_t)((((xuint8_t)(a))<<24)|(((xuint8_t)(b))<<16)|(((xuint8_t)(c))<<8)|((xuint8_t)(d))))

    PACK ('Z','y','y','y'), /* XUNICODE_SCRIPT_COMMON */
    PACK ('Z','i','n','h'), /* XUNICODE_SCRIPT_INHERITED */
    PACK ('A','r','a','b'), /* XUNICODE_SCRIPT_ARABIC */
    PACK ('A','r','m','n'), /* XUNICODE_SCRIPT_ARMENIAN */
    PACK ('B','e','n','g'), /* XUNICODE_SCRIPT_BENGALI */
    PACK ('B','o','p','o'), /* XUNICODE_SCRIPT_BOPOMOFO */
    PACK ('C','h','e','r'), /* XUNICODE_SCRIPT_CHEROKEE */
    PACK ('C','o','p','t'), /* XUNICODE_SCRIPT_COPTIC */
    PACK ('C','y','r','l'), /* XUNICODE_SCRIPT_CYRILLIC */
    PACK ('D','s','r','t'), /* XUNICODE_SCRIPT_DESERET */
    PACK ('D','e','v','a'), /* XUNICODE_SCRIPT_DEVANAGARI */
    PACK ('E','t','h','i'), /* XUNICODE_SCRIPT_ETHIOPIC */
    PACK ('G','e','o','r'), /* XUNICODE_SCRIPT_GEORGIAN */
    PACK ('G','o','t','h'), /* XUNICODE_SCRIPT_GOTHIC */
    PACK ('G','r','e','k'), /* XUNICODE_SCRIPT_GREEK */
    PACK ('G','u','j','r'), /* XUNICODE_SCRIPT_GUJARATI */
    PACK ('G','u','r','u'), /* XUNICODE_SCRIPT_GURMUKHI */
    PACK ('H','a','n','i'), /* XUNICODE_SCRIPT_HAN */
    PACK ('H','a','n','g'), /* XUNICODE_SCRIPT_HANGUL */
    PACK ('H','e','b','r'), /* XUNICODE_SCRIPT_HEBREW */
    PACK ('H','i','r','a'), /* XUNICODE_SCRIPT_HIRAGANA */
    PACK ('K','n','d','a'), /* XUNICODE_SCRIPT_KANNADA */
    PACK ('K','a','n','a'), /* XUNICODE_SCRIPT_KATAKANA */
    PACK ('K','h','m','r'), /* XUNICODE_SCRIPT_KHMER */
    PACK ('L','a','o','o'), /* XUNICODE_SCRIPT_LAO */
    PACK ('L','a','t','n'), /* XUNICODE_SCRIPT_LATIN */
    PACK ('M','l','y','m'), /* XUNICODE_SCRIPT_MALAYALAM */
    PACK ('M','o','n','g'), /* XUNICODE_SCRIPT_MONGOLIAN */
    PACK ('M','y','m','r'), /* XUNICODE_SCRIPT_MYANMAR */
    PACK ('O','g','a','m'), /* XUNICODE_SCRIPT_OGHAM */
    PACK ('I','t','a','l'), /* XUNICODE_SCRIPT_OLD_ITALIC */
    PACK ('O','r','y','a'), /* XUNICODE_SCRIPT_ORIYA */
    PACK ('R','u','n','r'), /* XUNICODE_SCRIPT_RUNIC */
    PACK ('S','i','n','h'), /* XUNICODE_SCRIPT_SINHALA */
    PACK ('S','y','r','c'), /* XUNICODE_SCRIPT_SYRIAC */
    PACK ('T','a','m','l'), /* XUNICODE_SCRIPT_TAMIL */
    PACK ('T','e','l','u'), /* XUNICODE_SCRIPT_TELUGU */
    PACK ('T','h','a','a'), /* XUNICODE_SCRIPT_THAANA */
    PACK ('T','h','a','i'), /* XUNICODE_SCRIPT_THAI */
    PACK ('T','i','b','t'), /* XUNICODE_SCRIPT_TIBETAN */
    PACK ('C','a','n','s'), /* XUNICODE_SCRIPT_CANADIAN_ABORIGINAL */
    PACK ('Y','i','i','i'), /* XUNICODE_SCRIPT_YI */
    PACK ('T','g','l','g'), /* XUNICODE_SCRIPT_TAGALOG */
    PACK ('H','a','n','o'), /* XUNICODE_SCRIPT_HANUNOO */
    PACK ('B','u','h','d'), /* XUNICODE_SCRIPT_BUHID */
    PACK ('T','a','g','b'), /* XUNICODE_SCRIPT_TAGBANWA */

  /* Unicode-4.0 additions */
    PACK ('B','r','a','i'), /* XUNICODE_SCRIPT_BRAILLE */
    PACK ('C','p','r','t'), /* XUNICODE_SCRIPT_CYPRIOT */
    PACK ('L','i','m','b'), /* XUNICODE_SCRIPT_LIMBU */
    PACK ('O','s','m','a'), /* XUNICODE_SCRIPT_OSMANYA */
    PACK ('S','h','a','w'), /* XUNICODE_SCRIPT_SHAVIAN */
    PACK ('L','i','n','b'), /* XUNICODE_SCRIPT_LINEAR_B */
    PACK ('T','a','l','e'), /* XUNICODE_SCRIPT_TAI_LE */
    PACK ('U','g','a','r'), /* XUNICODE_SCRIPT_UGARITIC */

  /* Unicode-4.1 additions */
    PACK ('T','a','l','u'), /* XUNICODE_SCRIPT_NEW_TAI_LUE */
    PACK ('B','u','g','i'), /* XUNICODE_SCRIPT_BUGINESE */
    PACK ('G','l','a','g'), /* XUNICODE_SCRIPT_GLAGOLITIC */
    PACK ('T','f','n','g'), /* XUNICODE_SCRIPT_TIFINAGH */
    PACK ('S','y','l','o'), /* XUNICODE_SCRIPT_SYLOTI_NAGRI */
    PACK ('X','p','e','o'), /* XUNICODE_SCRIPT_OLD_PERSIAN */
    PACK ('K','h','a','r'), /* XUNICODE_SCRIPT_KHAROSHTHI */

  /* Unicode-5.0 additions */
    PACK ('Z','z','z','z'), /* XUNICODE_SCRIPT_UNKNOWN */
    PACK ('B','a','l','i'), /* XUNICODE_SCRIPT_BALINESE */
    PACK ('X','s','u','x'), /* XUNICODE_SCRIPT_CUNEIFORM */
    PACK ('P','h','n','x'), /* XUNICODE_SCRIPT_PHOENICIAN */
    PACK ('P','h','a','g'), /* XUNICODE_SCRIPT_PHAGS_PA */
    PACK ('N','k','o','o'), /* XUNICODE_SCRIPT_NKO */

  /* Unicode-5.1 additions */
    PACK ('K','a','l','i'), /* XUNICODE_SCRIPT_KAYAH_LI */
    PACK ('L','e','p','c'), /* XUNICODE_SCRIPT_LEPCHA */
    PACK ('R','j','n','g'), /* XUNICODE_SCRIPT_REJANG */
    PACK ('S','u','n','d'), /* XUNICODE_SCRIPT_SUNDANESE */
    PACK ('S','a','u','r'), /* XUNICODE_SCRIPT_SAURASHTRA */
    PACK ('C','h','a','m'), /* XUNICODE_SCRIPT_CHAM */
    PACK ('O','l','c','k'), /* XUNICODE_SCRIPT_OL_CHIKI */
    PACK ('V','a','i','i'), /* XUNICODE_SCRIPT_VAI */
    PACK ('C','a','r','i'), /* XUNICODE_SCRIPT_CARIAN */
    PACK ('L','y','c','i'), /* XUNICODE_SCRIPT_LYCIAN */
    PACK ('L','y','d','i'), /* XUNICODE_SCRIPT_LYDIAN */

  /* Unicode-5.2 additions */
    PACK ('A','v','s','t'), /* XUNICODE_SCRIPT_AVESTAN */
    PACK ('B','a','m','u'), /* XUNICODE_SCRIPT_BAMUM */
    PACK ('E','g','y','p'), /* XUNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS */
    PACK ('A','r','m','i'), /* XUNICODE_SCRIPT_IMPERIAL_ARAMAIC */
    PACK ('P','h','l','i'), /* XUNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI */
    PACK ('P','r','t','i'), /* XUNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN */
    PACK ('J','a','v','a'), /* XUNICODE_SCRIPT_JAVANESE */
    PACK ('K','t','h','i'), /* XUNICODE_SCRIPT_KAITHI */
    PACK ('L','i','s','u'), /* XUNICODE_SCRIPT_LISU */
    PACK ('M','t','e','i'), /* XUNICODE_SCRIPT_MEETEI_MAYEK */
    PACK ('S','a','r','b'), /* XUNICODE_SCRIPT_OLD_SOUTH_ARABIAN */
    PACK ('O','r','k','h'), /* XUNICODE_SCRIPT_OLD_TURKIC */
    PACK ('S','a','m','r'), /* XUNICODE_SCRIPT_SAMARITAN */
    PACK ('L','a','n','a'), /* XUNICODE_SCRIPT_TAI_THAM */
    PACK ('T','a','v','t'), /* XUNICODE_SCRIPT_TAI_VIET */

  /* Unicode-6.0 additions */
    PACK ('B','a','t','k'), /* XUNICODE_SCRIPT_BATAK */
    PACK ('B','r','a','h'), /* XUNICODE_SCRIPT_BRAHMI */
    PACK ('M','a','n','d'), /* XUNICODE_SCRIPT_MANDAIC */

  /* Unicode-6.1 additions */
    PACK ('C','a','k','m'), /* XUNICODE_SCRIPT_CHAKMA */
    PACK ('M','e','r','c'), /* XUNICODE_SCRIPT_MEROITIC_CURSIVE */
    PACK ('M','e','r','o'), /* XUNICODE_SCRIPT_MEROITIC_HIEROGLYPHS */
    PACK ('P','l','r','d'), /* XUNICODE_SCRIPT_MIAO */
    PACK ('S','h','r','d'), /* XUNICODE_SCRIPT_SHARADA */
    PACK ('S','o','r','a'), /* XUNICODE_SCRIPT_SORA_SOMPENG */
    PACK ('T','a','k','r'), /* XUNICODE_SCRIPT_TAKRI */

  /* Unicode 7.0 additions */
    PACK ('B','a','s','s'), /* XUNICODE_SCRIPT_BASSA_VAH */
    PACK ('A','g','h','b'), /* XUNICODE_SCRIPT_CAUCASIAN_ALBANIAN */
    PACK ('D','u','p','l'), /* XUNICODE_SCRIPT_DUPLOYAN */
    PACK ('E','l','b','a'), /* XUNICODE_SCRIPT_ELBASAN */
    PACK ('G','r','a','n'), /* XUNICODE_SCRIPT_GRANTHA */
    PACK ('K','h','o','j'), /* XUNICODE_SCRIPT_KHOJKI*/
    PACK ('S','i','n','d'), /* XUNICODE_SCRIPT_KHUDAWADI */
    PACK ('L','i','n','a'), /* XUNICODE_SCRIPT_LINEAR_A */
    PACK ('M','a','h','j'), /* XUNICODE_SCRIPT_MAHAJANI */
    PACK ('M','a','n','i'), /* XUNICODE_SCRIPT_MANICHAEAN */
    PACK ('M','e','n','d'), /* XUNICODE_SCRIPT_MENDE_KIKAKUI */
    PACK ('M','o','d','i'), /* XUNICODE_SCRIPT_MODI */
    PACK ('M','r','o','o'), /* XUNICODE_SCRIPT_MRO */
    PACK ('N','b','a','t'), /* XUNICODE_SCRIPT_NABATAEAN */
    PACK ('N','a','r','b'), /* XUNICODE_SCRIPT_OLD_NORTH_ARABIAN */
    PACK ('P','e','r','m'), /* XUNICODE_SCRIPT_OLD_PERMIC */
    PACK ('H','m','n','g'), /* XUNICODE_SCRIPT_PAHAWH_HMONG */
    PACK ('P','a','l','m'), /* XUNICODE_SCRIPT_PALMYRENE */
    PACK ('P','a','u','c'), /* XUNICODE_SCRIPT_PAU_CIN_HAU */
    PACK ('P','h','l','p'), /* XUNICODE_SCRIPT_PSALTER_PAHLAVI */
    PACK ('S','i','d','d'), /* XUNICODE_SCRIPT_SIDDHAM */
    PACK ('T','i','r','h'), /* XUNICODE_SCRIPT_TIRHUTA */
    PACK ('W','a','r','a'), /* XUNICODE_SCRIPT_WARANG_CITI */

  /* Unicode 8.0 additions */
    PACK ('A','h','o','m'), /* XUNICODE_SCRIPT_AHOM */
    PACK ('H','l','u','w'), /* XUNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS */
    PACK ('H','a','t','r'), /* XUNICODE_SCRIPT_HATRAN */
    PACK ('M','u','l','t'), /* XUNICODE_SCRIPT_MULTANI */
    PACK ('H','u','n','g'), /* XUNICODE_SCRIPT_OLD_HUNGARIAN */
    PACK ('S','g','n','w'), /* XUNICODE_SCRIPT_SIGNWRITING */

  /* Unicode 9.0 additions */
    PACK ('A','d','l','m'), /* XUNICODE_SCRIPT_ADLAM */
    PACK ('B','h','k','s'), /* XUNICODE_SCRIPT_BHAIKSUKI */
    PACK ('M','a','r','c'), /* XUNICODE_SCRIPT_MARCHEN */
    PACK ('N','e','w','a'), /* XUNICODE_SCRIPT_NEWA */
    PACK ('O','s','g','e'), /* XUNICODE_SCRIPT_OSAGE */
    PACK ('T','a','n','g'), /* XUNICODE_SCRIPT_TANGUT */

  /* Unicode 10.0 additions */
    PACK ('G','o','n','m'), /* XUNICODE_SCRIPT_MASARAM_GONDI */
    PACK ('N','s','h','u'), /* XUNICODE_SCRIPT_NUSHU */
    PACK ('S','o','y','o'), /* XUNICODE_SCRIPT_SOYOMBO */
    PACK ('Z','a','n','b'), /* XUNICODE_SCRIPT_ZANABAZAR_SQUARE */

  /* Unicode 11.0 additions */
    PACK ('D','o','g','r'), /* XUNICODE_SCRIPT_DOGRA */
    PACK ('G','o','n','g'), /* XUNICODE_SCRIPT_GUNJALA_GONDI */
    PACK ('R','o','h','g'), /* XUNICODE_SCRIPT_HANIFI_ROHINGYA */
    PACK ('M','a','k','a'), /* XUNICODE_SCRIPT_MAKASAR */
    PACK ('M','e','d','f'), /* XUNICODE_SCRIPT_MEDEFAIDRIN */
    PACK ('S','o','g','o'), /* XUNICODE_SCRIPT_OLD_SOGDIAN */
    PACK ('S','o','g','d'), /* XUNICODE_SCRIPT_SOGDIAN */

  /* Unicode 12.0 additions */
    PACK ('E','l','y','m'), /* XUNICODE_SCRIPT_ELYMAIC */
    PACK ('N','a','n','d'), /* XUNICODE_SCRIPT_NANDINAGARI */
    PACK ('H','m','n','p'), /* XUNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG */
    PACK ('W','c','h','o'), /* XUNICODE_SCRIPT_WANCHO */

  /* Unicode 13.0 additions */
    PACK ('C', 'h', 'r', 's'), /* XUNICODE_SCRIPT_CHORASMIAN */
    PACK ('D', 'i', 'a', 'k'), /* XUNICODE_SCRIPT_DIVES_AKURU */
    PACK ('K', 'i', 't', 's'), /* XUNICODE_SCRIPT_KHITAN_SMALL_SCRIPT */
    PACK ('Y', 'e', 'z', 'i'), /* XUNICODE_SCRIPT_YEZIDI */

  /* Unicode 14.0 additions */
    PACK ('C', 'p', 'm', 'n'), /* XUNICODE_SCRIPT_CYPRO_MINOAN */
    PACK ('O', 'u', 'g', 'r'), /* XUNICODE_SCRIPT_OLD_UYHUR */
    PACK ('T', 'n', 's', 'a'), /* XUNICODE_SCRIPT_TANGSA */
    PACK ('T', 'o', 't', 'o'), /* XUNICODE_SCRIPT_TOTO */
    PACK ('V', 'i', 't', 'h'), /* XUNICODE_SCRIPT_VITHKUQI */

  /* not really a Unicode script, but part of ISO 15924 */
    PACK ('Z', 'm', 't', 'h'), /* XUNICODE_SCRIPT_MATH */

#undef PACK
};

/**
 * xunicode_script_to_iso15924:
 * @script: a Unicode script
 *
 * Looks up the ISO 15924 code for @script.  ISO 15924 assigns four-letter
 * codes to scripts.  For example, the code for Arabic is 'Arab'.  The
 * four letter codes are encoded as a @xuint32_t by this function in a
 * big-endian fashion.  That is, the code returned for Arabic is
 * 0x41726162 (0x41 is ASCII code for 'A', 0x72 is ASCII code for 'r', etc).
 *
 * See
 * [Codes for the representation of names of scripts](http://unicode.org/iso15924/codelists.html)
 * for details.
 *
 * Returns: the ISO 15924 code for @script, encoded as an integer,
 *   of zero if @script is %XUNICODE_SCRIPT_INVALID_CODE or
 *   ISO 15924 code 'Zzzz' (script code for UNKNOWN) if @script is not understood.
 *
 * Since: 2.30
 */
xuint32_t
xunicode_script_to_iso15924 (xunicode_script_t script)
{
  if (G_UNLIKELY (script == XUNICODE_SCRIPT_INVALID_CODE))
    return 0;

  if (G_UNLIKELY (script < 0 || script >= (int) G_N_ELEMENTS (iso15924_tags)))
    return 0x5A7A7A7A;

  return iso15924_tags[script];
}

/**
 * xunicode_script_from_iso15924:
 * @iso15924: a Unicode script
 *
 * Looks up the Unicode script for @iso15924.  ISO 15924 assigns four-letter
 * codes to scripts.  For example, the code for Arabic is 'Arab'.
 * This function accepts four letter codes encoded as a @xuint32_t in a
 * big-endian fashion.  That is, the code expected for Arabic is
 * 0x41726162 (0x41 is ASCII code for 'A', 0x72 is ASCII code for 'r', etc).
 *
 * See
 * [Codes for the representation of names of scripts](http://unicode.org/iso15924/codelists.html)
 * for details.
 *
 * Returns: the Unicode script for @iso15924, or
 *   of %XUNICODE_SCRIPT_INVALID_CODE if @iso15924 is zero and
 *   %XUNICODE_SCRIPT_UNKNOWN if @iso15924 is unknown.
 *
 * Since: 2.30
 */
xunicode_script_t
xunicode_script_from_iso15924 (xuint32_t iso15924)
{
  unsigned int i;

   if (!iso15924)
     return XUNICODE_SCRIPT_INVALID_CODE;

  for (i = 0; i < G_N_ELEMENTS (iso15924_tags); i++)
    if (iso15924_tags[i] == iso15924)
      return (xunicode_script_t) i;

  return XUNICODE_SCRIPT_UNKNOWN;
}
