/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997, 1999  Peter Mattis, Red Hat, Inc.
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

#include "config.h"

#include <string.h>

#include "gpattern.h"

#include "gmacros.h"
#include "gmem.h"
#include "gmessages.h"
#include "gstrfuncs.h"
#include "gunicode.h"
#include "gutils.h"

/**
 * SECTION:patterns
 * @title: Glob-style pattern matching
 * @short_description: matches strings against patterns containing '*'
 *                     (wildcard) and '?' (joker)
 *
 * The g_pattern_match* functions match a string
 * against a pattern containing '*' and '?' wildcards with similar
 * semantics as the standard glob() function: '*' matches an arbitrary,
 * possibly empty, string, '?' matches an arbitrary character.
 *
 * Note that in contrast to glob(), the '/' character can be matched by
 * the wildcards, there are no '[...]' character ranges and '*' and '?'
 * can not be escaped to include them literally in a pattern.
 *
 * When multiple strings must be matched against the same pattern, it
 * is better to compile the pattern to a #xpattern_spec_t using
 * xpattern_spec_new() and use g_pattern_match_string() instead of
 * g_pattern_match_simple(). This avoids the overhead of repeated
 * pattern compilation.
 **/

/**
 * xpattern_spec_t:
 *
 * A xpattern_spec_t struct is the 'compiled' form of a pattern. This
 * structure is opaque and its fields cannot be accessed directly.
 */

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


/* --- functions --- */
static inline xboolean_t
g_pattern_ph_match (const xchar_t *match_pattern,
		    const xchar_t *match_string,
		    xboolean_t    *wildcard_reached_p)
{
  const xchar_t *pattern, *string;
  xchar_t ch;

  pattern = match_pattern;
  string = match_string;

  ch = *pattern;
  pattern++;
  while (ch)
    {
      switch (ch)
	{
	case '?':
	  if (!*string)
	    return FALSE;
	  string = xutf8_next_char (string);
	  break;

	case '*':
	  *wildcard_reached_p = TRUE;
	  do
	    {
	      ch = *pattern;
	      pattern++;
	      if (ch == '?')
		{
		  if (!*string)
		    return FALSE;
		  string = xutf8_next_char (string);
		}
	    }
	  while (ch == '*' || ch == '?');
	  if (!ch)
	    return TRUE;
	  do
	    {
              xboolean_t next_wildcard_reached = FALSE;
	      while (ch != *string)
		{
		  if (!*string)
		    return FALSE;
		  string = xutf8_next_char (string);
		}
	      string++;
	      if (g_pattern_ph_match (pattern, string, &next_wildcard_reached))
		return TRUE;
              if (next_wildcard_reached)
                /* the forthcoming pattern substring up to the next wildcard has
                 * been matched, but a mismatch occurred for the rest of the
                 * pattern, following the next wildcard.
                 * there's no need to advance the current match position any
                 * further if the rest pattern will not match.
                 */
		return FALSE;
	    }
	  while (*string);
	  break;

	default:
	  if (ch == *string)
	    string++;
	  else
	    return FALSE;
	  break;
	}

      ch = *pattern;
      pattern++;
    }

  return *string == 0;
}

/**
 * xpattern_spec_match:
 * @pspec: a #xpattern_spec_t
 * @string_length: the length of @string (in bytes, i.e. strlen(),
 *     not xutf8_strlen())
 * @string: the UTF-8 encoded string to match
 * @string_reversed: (nullable): the reverse of @string or %NULL
 *
 * Matches a string against a compiled pattern. Passing the correct
 * length of the string given is mandatory. The reversed string can be
 * omitted by passing %NULL, this is more efficient if the reversed
 * version of the string to be matched is not at hand, as
 * g_pattern_match() will only construct it if the compiled pattern
 * requires reverse matches.
 *
 * Note that, if the user code will (possibly) match a string against a
 * multitude of patterns containing wildcards, chances are high that
 * some patterns will require a reversed string. In this case, it's
 * more efficient to provide the reversed string to avoid multiple
 * constructions thereof in the various calls to g_pattern_match().
 *
 * Note also that the reverse of a UTF-8 encoded string can in general
 * not be obtained by xstrreverse(). This works only if the string
 * does not contain any multibyte characters. GLib offers the
 * xutf8_strreverse() function to reverse UTF-8 encoded strings.
 *
 * Returns: %TRUE if @string matches @pspec
 *
 * Since: 2.70
 **/
xboolean_t
xpattern_spec_match (xpattern_spec_t *pspec,
                      xsize_t string_length,
                      const xchar_t *string,
                      const xchar_t *string_reversed)
{
  g_return_val_if_fail (pspec != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  if (string_length < pspec->min_length ||
      string_length > pspec->max_length)
    return FALSE;

  switch (pspec->match_type)
    {
      xboolean_t dummy;
    case XMATCH_ALL:
      return g_pattern_ph_match (pspec->pattern, string, &dummy);
    case XMATCH_ALL_TAIL:
      if (string_reversed)
	return g_pattern_ph_match (pspec->pattern, string_reversed, &dummy);
      else
	{
          xboolean_t result;
          xchar_t *tmp;
	  tmp = xutf8_strreverse (string, string_length);
	  result = g_pattern_ph_match (pspec->pattern, tmp, &dummy);
	  g_free (tmp);
	  return result;
	}
    case XMATCH_HEAD:
      if (pspec->pattern_length == string_length)
	return strcmp (pspec->pattern, string) == 0;
      else if (pspec->pattern_length)
	return strncmp (pspec->pattern, string, pspec->pattern_length) == 0;
      else
	return TRUE;
    case XMATCH_TAIL:
      if (pspec->pattern_length)
        return strcmp (pspec->pattern, string + (string_length - pspec->pattern_length)) == 0;
      else
	return TRUE;
    case XMATCH_EXACT:
      if (pspec->pattern_length != string_length)
        return FALSE;
      else
        return strcmp (pspec->pattern, string) == 0;
    default:
      g_return_val_if_fail (pspec->match_type < XMATCH_LAST, FALSE);
      return FALSE;
    }
}

/**
 * g_pattern_match: (skip)
 * @pspec: a #xpattern_spec_t
 * @string_length: the length of @string (in bytes, i.e. strlen(),
 *     not xutf8_strlen())
 * @string: the UTF-8 encoded string to match
 * @string_reversed: (nullable): the reverse of @string or %NULL
 *
 * Matches a string against a compiled pattern. Passing the correct
 * length of the string given is mandatory. The reversed string can be
 * omitted by passing %NULL, this is more efficient if the reversed
 * version of the string to be matched is not at hand, as
 * g_pattern_match() will only construct it if the compiled pattern
 * requires reverse matches.
 *
 * Note that, if the user code will (possibly) match a string against a
 * multitude of patterns containing wildcards, chances are high that
 * some patterns will require a reversed string. In this case, it's
 * more efficient to provide the reversed string to avoid multiple
 * constructions thereof in the various calls to g_pattern_match().
 *
 * Note also that the reverse of a UTF-8 encoded string can in general
 * not be obtained by xstrreverse(). This works only if the string
 * does not contain any multibyte characters. GLib offers the
 * xutf8_strreverse() function to reverse UTF-8 encoded strings.
 *
 * Returns: %TRUE if @string matches @pspec
 * Deprecated: 2.70: Use xpattern_spec_match() instead
 **/
xboolean_t
g_pattern_match (xpattern_spec_t *pspec,
                 xuint_t string_length,
                 const xchar_t *string,
                 const xchar_t *string_reversed)
{
  return xpattern_spec_match (pspec, string_length, string, string_reversed);
}

/**
 * xpattern_spec_new:
 * @pattern: a zero-terminated UTF-8 encoded string
 *
 * Compiles a pattern to a #xpattern_spec_t.
 *
 * Returns: a newly-allocated #xpattern_spec_t
 **/
xpattern_spec_t*
xpattern_spec_new (const xchar_t *pattern)
{
  xpattern_spec_t *pspec;
  xboolean_t seen_joker = FALSE, seen_wildcard = FALSE, more_wildcards = FALSE;
  xint_t hw_pos = -1, tw_pos = -1, hj_pos = -1, tj_pos = -1;
  xboolean_t follows_wildcard = FALSE;
  xuint_t pending_jokers = 0;
  const xchar_t *s;
  xchar_t *d;
  xuint_t i;

  g_return_val_if_fail (pattern != NULL, NULL);

  /* canonicalize pattern and collect necessary stats */
  pspec = g_new (xpattern_spec_t, 1);
  pspec->pattern_length = strlen (pattern);
  pspec->min_length = 0;
  pspec->max_length = 0;
  pspec->pattern = g_new (xchar_t, pspec->pattern_length + 1);
  d = pspec->pattern;
  for (i = 0, s = pattern; *s != 0; s++)
    {
      switch (*s)
	{
	case '*':
	  if (follows_wildcard)	/* compress multiple wildcards */
	    {
	      pspec->pattern_length--;
	      continue;
	    }
	  follows_wildcard = TRUE;
	  if (hw_pos < 0)
	    hw_pos = i;
	  tw_pos = i;
	  break;
	case '?':
	  pending_jokers++;
	  pspec->min_length++;
	  pspec->max_length += 4; /* maximum UTF-8 character length */
	  continue;
	default:
	  for (; pending_jokers; pending_jokers--, i++) {
	    *d++ = '?';
  	    if (hj_pos < 0)
	     hj_pos = i;
	    tj_pos = i;
	  }
	  follows_wildcard = FALSE;
	  pspec->min_length++;
	  pspec->max_length++;
	  break;
	}
      *d++ = *s;
      i++;
    }
  for (; pending_jokers; pending_jokers--) {
    *d++ = '?';
    if (hj_pos < 0)
      hj_pos = i;
    tj_pos = i;
  }
  *d++ = 0;
  seen_joker = hj_pos >= 0;
  seen_wildcard = hw_pos >= 0;
  more_wildcards = seen_wildcard && hw_pos != tw_pos;
  if (seen_wildcard)
    pspec->max_length = G_MAXUINT;

  /* special case sole head/tail wildcard or exact matches */
  if (!seen_joker && !more_wildcards)
    {
      if (pspec->pattern[0] == '*')
	{
	  pspec->match_type = XMATCH_TAIL;
          memmove (pspec->pattern, pspec->pattern + 1, --pspec->pattern_length);
	  pspec->pattern[pspec->pattern_length] = 0;
	  return pspec;
	}
      if (pspec->pattern_length > 0 &&
	  pspec->pattern[pspec->pattern_length - 1] == '*')
	{
	  pspec->match_type = XMATCH_HEAD;
	  pspec->pattern[--pspec->pattern_length] = 0;
	  return pspec;
	}
      if (!seen_wildcard)
	{
	  pspec->match_type = XMATCH_EXACT;
	  return pspec;
	}
    }

  /* now just need to distinguish between head or tail match start */
  tw_pos = pspec->pattern_length - 1 - tw_pos;	/* last pos to tail distance */
  tj_pos = pspec->pattern_length - 1 - tj_pos;	/* last pos to tail distance */
  if (seen_wildcard)
    pspec->match_type = tw_pos > hw_pos ? XMATCH_ALL_TAIL : XMATCH_ALL;
  else /* seen_joker */
    pspec->match_type = tj_pos > hj_pos ? XMATCH_ALL_TAIL : XMATCH_ALL;
  if (pspec->match_type == XMATCH_ALL_TAIL) {
    xchar_t *tmp = pspec->pattern;
    pspec->pattern = xutf8_strreverse (pspec->pattern, pspec->pattern_length);
    g_free (tmp);
  }
  return pspec;
}

/**
 * xpattern_spec_copy:
 * @pspec: a #xpattern_spec_t
 *
 * Copies @pspec in a new #xpattern_spec_t.
 *
 * Returns: (transfer full): a copy of @pspec.
 *
 * Since: 2.70
 **/
xpattern_spec_t *
xpattern_spec_copy (xpattern_spec_t *pspec)
{
  xpattern_spec_t *pspec_copy;

  g_return_val_if_fail (pspec != NULL, NULL);

  pspec_copy = g_new (xpattern_spec_t, 1);
  *pspec_copy = *pspec;
  pspec_copy->pattern = xstrndup (pspec->pattern, pspec->pattern_length);

  return pspec_copy;
}

/**
 * xpattern_spec_free:
 * @pspec: a #xpattern_spec_t
 *
 * Frees the memory allocated for the #xpattern_spec_t.
 **/
void
xpattern_spec_free (xpattern_spec_t *pspec)
{
  g_return_if_fail (pspec != NULL);

  g_free (pspec->pattern);
  g_free (pspec);
}

/**
 * xpattern_spec_equal:
 * @pspec1: a #xpattern_spec_t
 * @pspec2: another #xpattern_spec_t
 *
 * Compares two compiled pattern specs and returns whether they will
 * match the same set of strings.
 *
 * Returns: Whether the compiled patterns are equal
 **/
xboolean_t
xpattern_spec_equal (xpattern_spec_t *pspec1,
		      xpattern_spec_t *pspec2)
{
  g_return_val_if_fail (pspec1 != NULL, FALSE);
  g_return_val_if_fail (pspec2 != NULL, FALSE);

  return (pspec1->pattern_length == pspec2->pattern_length &&
	  pspec1->match_type == pspec2->match_type &&
	  strcmp (pspec1->pattern, pspec2->pattern) == 0);
}

/**
 * xpattern_spec_match_string:
 * @pspec: a #xpattern_spec_t
 * @string: the UTF-8 encoded string to match
 *
 * Matches a string against a compiled pattern. If the string is to be
 * matched against more than one pattern, consider using
 * g_pattern_match() instead while supplying the reversed string.
 *
 * Returns: %TRUE if @string matches @pspec
 *
 * Since: 2.70
 **/
xboolean_t
xpattern_spec_match_string (xpattern_spec_t *pspec,
                             const xchar_t *string)
{
  g_return_val_if_fail (pspec != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  return xpattern_spec_match (pspec, strlen (string), string, NULL);
}

/**
 * g_pattern_match_string: (skip)
 * @pspec: a #xpattern_spec_t
 * @string: the UTF-8 encoded string to match
 *
 * Matches a string against a compiled pattern. If the string is to be
 * matched against more than one pattern, consider using
 * g_pattern_match() instead while supplying the reversed string.
 *
 * Returns: %TRUE if @string matches @pspec
 * Deprecated: 2.70: Use xpattern_spec_match_string() instead
 **/
xboolean_t
g_pattern_match_string (xpattern_spec_t *pspec,
                        const xchar_t *string)
{
  return xpattern_spec_match_string (pspec, string);
}

/**
 * g_pattern_match_simple:
 * @pattern: the UTF-8 encoded pattern
 * @string: the UTF-8 encoded string to match
 *
 * Matches a string against a pattern given as a string. If this
 * function is to be called in a loop, it's more efficient to compile
 * the pattern once with xpattern_spec_new() and call
 * g_pattern_match_string() repeatedly.
 *
 * Returns: %TRUE if @string matches @pspec
 **/
xboolean_t
g_pattern_match_simple (const xchar_t *pattern,
			const xchar_t *string)
{
  xpattern_spec_t *pspec;
  xboolean_t ergo;

  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  pspec = xpattern_spec_new (pattern);
  ergo = xpattern_spec_match (pspec, strlen (string), string, NULL);
  xpattern_spec_free (pspec);

  return ergo;
}
