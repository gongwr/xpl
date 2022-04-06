/* xregex_t -- regular expression API wrapper around PCRE.
 *
 * Copyright (C) 1999, 2000 Scott Wimer
 * Copyright (C) 2004, Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005 - 2007, Marco Barisione <marco@barisione.org>
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __XREGEX_H__
#define __XREGEX_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gerror.h>
#include <glib/gstring.h>

G_BEGIN_DECLS

/**
 * GRegexError:
 * @XREGEX_ERROR_COMPILE: Compilation of the regular expression failed.
 * @XREGEX_ERROR_OPTIMIZE: Optimization of the regular expression failed.
 * @XREGEX_ERROR_REPLACE: Replacement failed due to an ill-formed replacement
 *     string.
 * @XREGEX_ERROR_MATCH: The match process failed.
 * @XREGEX_ERROR_INTERNAL: Internal error of the regular expression engine.
 *     Since 2.16
 * @XREGEX_ERROR_STRAY_BACKSLASH: "\\" at end of pattern. Since 2.16
 * @XREGEX_ERROR_MISSING_CONTROL_CHAR: "\\c" at end of pattern. Since 2.16
 * @XREGEX_ERROR_UNRECOGNIZED_ESCAPE: Unrecognized character follows "\\".
 *     Since 2.16
 * @XREGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER: Numbers out of order in "{}"
 *     quantifier. Since 2.16
 * @XREGEX_ERROR_QUANTIFIER_TOO_BIG: Number too big in "{}" quantifier.
 *     Since 2.16
 * @XREGEX_ERROR_UNTERMINATED_CHARACTER_CLASS: Missing terminating "]" for
 *     character class. Since 2.16
 * @XREGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS: Invalid escape sequence
 *     in character class. Since 2.16
 * @XREGEX_ERROR_RANGE_OUT_OF_ORDER: Range out of order in character class.
 *     Since 2.16
 * @XREGEX_ERROR_NOTHING_TO_REPEAT: Nothing to repeat. Since 2.16
 * @XREGEX_ERROR_UNRECOGNIZED_CHARACTER: Unrecognized character after "(?",
 *     "(?<" or "(?P". Since 2.16
 * @XREGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS: POSIX named classes are
 *     supported only within a class. Since 2.16
 * @XREGEX_ERROR_UNMATCHED_PARENTHESIS: Missing terminating ")" or ")"
 *     without opening "(". Since 2.16
 * @XREGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE: Reference to non-existent
 *     subpattern. Since 2.16
 * @XREGEX_ERROR_UNTERMINATED_COMMENT: Missing terminating ")" after comment.
 *     Since 2.16
 * @XREGEX_ERROR_EXPRESSION_TOO_LARGE: Regular expression too large.
 *     Since 2.16
 * @XREGEX_ERROR_MEMORY_ERROR: Failed to get memory. Since 2.16
 * @XREGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND: Lookbehind assertion is not
 *     fixed length. Since 2.16
 * @XREGEX_ERROR_MALFORMED_CONDITION: Malformed number or name after "(?(".
 *     Since 2.16
 * @XREGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES: Conditional group contains
 *     more than two branches. Since 2.16
 * @XREGEX_ERROR_ASSERTION_EXPECTED: Assertion expected after "(?(".
 *     Since 2.16
 * @XREGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME: Unknown POSIX class name.
 *     Since 2.16
 * @XREGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED: POSIX collating
 *     elements are not supported. Since 2.16
 * @XREGEX_ERROR_HEX_CODE_TOO_LARGE: Character value in "\\x{...}" sequence
 *     is too large. Since 2.16
 * @XREGEX_ERROR_INVALID_CONDITION: Invalid condition "(?(0)". Since 2.16
 * @XREGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND: \\C not allowed in
 *     lookbehind assertion. Since 2.16
 * @XREGEX_ERROR_INFINITE_LOOP: Recursive call could loop indefinitely.
 *     Since 2.16
 * @XREGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR: Missing terminator
 *     in subpattern name. Since 2.16
 * @XREGEX_ERROR_DUPLICATE_SUBPATTERN_NAME: Two named subpatterns have
 *     the same name. Since 2.16
 * @XREGEX_ERROR_MALFORMED_PROPERTY: Malformed "\\P" or "\\p" sequence.
 *     Since 2.16
 * @XREGEX_ERROR_UNKNOWN_PROPERTY: Unknown property name after "\\P" or
 *     "\\p". Since 2.16
 * @XREGEX_ERROR_SUBPATTERN_NAME_TOO_LONG: Subpattern name is too long
 *     (maximum 32 characters). Since 2.16
 * @XREGEX_ERROR_TOO_MANY_SUBPATTERNS: Too many named subpatterns (maximum
 *     10,000). Since 2.16
 * @XREGEX_ERROR_INVALID_OCTAL_VALUE: Octal value is greater than "\\377".
 *     Since 2.16
 * @XREGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE: "DEFINE" group contains more
 *     than one branch. Since 2.16
 * @XREGEX_ERROR_DEFINE_REPETION: Repeating a "DEFINE" group is not allowed.
 *     This error is never raised. Since: 2.16 Deprecated: 2.34
 * @XREGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS: Inconsistent newline options.
 *     Since 2.16
 * @XREGEX_ERROR_MISSING_BACK_REFERENCE: "\\g" is not followed by a braced,
 *      angle-bracketed, or quoted name or number, or by a plain number. Since: 2.16
 * @XREGEX_ERROR_INVALID_RELATIVE_REFERENCE: relative reference must not be zero. Since: 2.34
 * @XREGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN: the backtracing
 *     control verb used does not allow an argument. Since: 2.34
 * @XREGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB: unknown backtracing
 *     control verb. Since: 2.34
 * @XREGEX_ERROR_NUMBER_TOO_BIG: number is too big in escape sequence. Since: 2.34
 * @XREGEX_ERROR_MISSING_SUBPATTERN_NAME: Missing subpattern name. Since: 2.34
 * @XREGEX_ERROR_MISSING_DIGIT: Missing digit. Since 2.34
 * @XREGEX_ERROR_INVALID_DATA_CHARACTER: In JavaScript compatibility mode,
 *     "[" is an invalid data character. Since: 2.34
 * @XREGEX_ERROR_EXTRA_SUBPATTERN_NAME: different names for subpatterns of the
 *     same number are not allowed. Since: 2.34
 * @XREGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED: the backtracing control
 *     verb requires an argument. Since: 2.34
 * @XREGEX_ERROR_INVALID_CONTROL_CHAR: "\\c" must be followed by an ASCII
 *     character. Since: 2.34
 * @XREGEX_ERROR_MISSING_NAME: "\\k" is not followed by a braced, angle-bracketed, or
 *     quoted name. Since: 2.34
 * @XREGEX_ERROR_NOT_SUPPORTED_IN_CLASS: "\\N" is not supported in a class. Since: 2.34
 * @XREGEX_ERROR_TOO_MANY_FORWARD_REFERENCES: too many forward references. Since: 2.34
 * @XREGEX_ERROR_NAME_TOO_LONG: the name is too long in "(*MARK)", "(*PRUNE)",
 *     "(*SKIP)", or "(*THEN)". Since: 2.34
 * @XREGEX_ERROR_CHARACTER_VALUE_TOO_LARGE: the character value in the \\u sequence is
 *     too large. Since: 2.34
 *
 * Error codes returned by regular expressions functions.
 *
 * Since: 2.14
 */
typedef enum
{
  XREGEX_ERROR_COMPILE,
  XREGEX_ERROR_OPTIMIZE,
  XREGEX_ERROR_REPLACE,
  XREGEX_ERROR_MATCH,
  XREGEX_ERROR_INTERNAL,

  /* These are the error codes from PCRE + 100 */
  XREGEX_ERROR_STRAY_BACKSLASH = 101,
  XREGEX_ERROR_MISSING_CONTROL_CHAR = 102,
  XREGEX_ERROR_UNRECOGNIZED_ESCAPE = 103,
  XREGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER = 104,
  XREGEX_ERROR_QUANTIFIER_TOO_BIG = 105,
  XREGEX_ERROR_UNTERMINATED_CHARACTER_CLASS = 106,
  XREGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS = 107,
  XREGEX_ERROR_RANGE_OUT_OF_ORDER = 108,
  XREGEX_ERROR_NOTHING_TO_REPEAT = 109,
  XREGEX_ERROR_UNRECOGNIZED_CHARACTER = 112,
  XREGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS = 113,
  XREGEX_ERROR_UNMATCHED_PARENTHESIS = 114,
  XREGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE = 115,
  XREGEX_ERROR_UNTERMINATED_COMMENT = 118,
  XREGEX_ERROR_EXPRESSION_TOO_LARGE = 120,
  XREGEX_ERROR_MEMORY_ERROR = 121,
  XREGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND = 125,
  XREGEX_ERROR_MALFORMED_CONDITION = 126,
  XREGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES = 127,
  XREGEX_ERROR_ASSERTION_EXPECTED = 128,
  XREGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME = 130,
  XREGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED = 131,
  XREGEX_ERROR_HEX_CODE_TOO_LARGE = 134,
  XREGEX_ERROR_INVALID_CONDITION = 135,
  XREGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND = 136,
  XREGEX_ERROR_INFINITE_LOOP = 140,
  XREGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR = 142,
  XREGEX_ERROR_DUPLICATE_SUBPATTERN_NAME = 143,
  XREGEX_ERROR_MALFORMED_PROPERTY = 146,
  XREGEX_ERROR_UNKNOWN_PROPERTY = 147,
  XREGEX_ERROR_SUBPATTERN_NAME_TOO_LONG = 148,
  XREGEX_ERROR_TOO_MANY_SUBPATTERNS = 149,
  XREGEX_ERROR_INVALID_OCTAL_VALUE = 151,
  XREGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE = 154,
  XREGEX_ERROR_DEFINE_REPETION = 155,
  XREGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS = 156,
  XREGEX_ERROR_MISSING_BACK_REFERENCE = 157,
  XREGEX_ERROR_INVALID_RELATIVE_REFERENCE = 158,
  XREGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN = 159,
  XREGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB  = 160,
  XREGEX_ERROR_NUMBER_TOO_BIG = 161,
  XREGEX_ERROR_MISSING_SUBPATTERN_NAME = 162,
  XREGEX_ERROR_MISSING_DIGIT = 163,
  XREGEX_ERROR_INVALID_DATA_CHARACTER = 164,
  XREGEX_ERROR_EXTRA_SUBPATTERN_NAME = 165,
  XREGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED = 166,
  XREGEX_ERROR_INVALID_CONTROL_CHAR = 168,
  XREGEX_ERROR_MISSING_NAME = 169,
  XREGEX_ERROR_NOT_SUPPORTED_IN_CLASS = 171,
  XREGEX_ERROR_TOO_MANY_FORWARD_REFERENCES = 172,
  XREGEX_ERROR_NAME_TOO_LONG = 175,
  XREGEX_ERROR_CHARACTER_VALUE_TOO_LARGE = 176
} GRegexError;

/**
 * XREGEX_ERROR:
 *
 * Error domain for regular expressions. Errors in this domain will be
 * from the #GRegexError enumeration. See #xerror_t for information on
 * error domains.
 *
 * Since: 2.14
 */
#define XREGEX_ERROR xregex_error_quark ()

XPL_AVAILABLE_IN_ALL
xquark xregex_error_quark (void);

/**
 * xregex_compile_flags_t:
 * @XREGEX_CASELESS: Letters in the pattern match both upper- and
 *     lowercase letters. This option can be changed within a pattern
 *     by a "(?i)" option setting.
 * @XREGEX_MULTILINE: By default, xregex_t treats the strings as consisting
 *     of a single line of characters (even if it actually contains
 *     newlines). The "start of line" metacharacter ("^") matches only
 *     at the start of the string, while the "end of line" metacharacter
 *     ("$") matches only at the end of the string, or before a terminating
 *     newline (unless %XREGEX_DOLLAR_ENDONLY is set). When
 *     %XREGEX_MULTILINE is set, the "start of line" and "end of line"
 *     constructs match immediately following or immediately before any
 *     newline in the string, respectively, as well as at the very start
 *     and end. This can be changed within a pattern by a "(?m)" option
 *     setting.
 * @XREGEX_DOTALL: A dot metacharacter (".") in the pattern matches all
 *     characters, including newlines. Without it, newlines are excluded.
 *     This option can be changed within a pattern by a ("?s") option setting.
 * @XREGEX_EXTENDED: Whitespace data characters in the pattern are
 *     totally ignored except when escaped or inside a character class.
 *     Whitespace does not include the VT character (code 11). In addition,
 *     characters between an unescaped "#" outside a character class and
 *     the next newline character, inclusive, are also ignored. This can
 *     be changed within a pattern by a "(?x)" option setting.
 * @XREGEX_ANCHORED: The pattern is forced to be "anchored", that is,
 *     it is constrained to match only at the first matching point in the
 *     string that is being searched. This effect can also be achieved by
 *     appropriate constructs in the pattern itself such as the "^"
 *     metacharacter.
 * @XREGEX_DOLLAR_ENDONLY: A dollar metacharacter ("$") in the pattern
 *     matches only at the end of the string. Without this option, a
 *     dollar also matches immediately before the final character if
 *     it is a newline (but not before any other newlines). This option
 *     is ignored if %XREGEX_MULTILINE is set.
 * @XREGEX_UNGREEDY: Inverts the "greediness" of the quantifiers so that
 *     they are not greedy by default, but become greedy if followed by "?".
 *     It can also be set by a "(?U)" option setting within the pattern.
 * @XREGEX_RAW: Usually strings must be valid UTF-8 strings, using this
 *     flag they are considered as a raw sequence of bytes.
 * @XREGEX_NO_AUTO_CAPTURE: Disables the use of numbered capturing
 *     parentheses in the pattern. Any opening parenthesis that is not
 *     followed by "?" behaves as if it were followed by "?:" but named
 *     parentheses can still be used for capturing (and they acquire numbers
 *     in the usual way).
 * @XREGEX_OPTIMIZE: Optimize the regular expression. If the pattern will
 *     be used many times, then it may be worth the effort to optimize it
 *     to improve the speed of matches.
 * @XREGEX_FIRSTLINE: Limits an unanchored pattern to match before (or at) the
 *     first newline. Since: 2.34
 * @XREGEX_DUPNAMES: Names used to identify capturing subpatterns need not
 *     be unique. This can be helpful for certain types of pattern when it
 *     is known that only one instance of the named subpattern can ever be
 *     matched.
 * @XREGEX_NEWLINE_CR: Usually any newline character or character sequence is
 *     recognized. If this option is set, the only recognized newline character
 *     is '\r'.
 * @XREGEX_NEWLINE_LF: Usually any newline character or character sequence is
 *     recognized. If this option is set, the only recognized newline character
 *     is '\n'.
 * @XREGEX_NEWLINE_CRLF: Usually any newline character or character sequence is
 *     recognized. If this option is set, the only recognized newline character
 *     sequence is '\r\n'.
 * @XREGEX_NEWLINE_ANYCRLF: Usually any newline character or character sequence
 *     is recognized. If this option is set, the only recognized newline character
 *     sequences are '\r', '\n', and '\r\n'. Since: 2.34
 * @XREGEX_BSR_ANYCRLF: Usually any newline character or character sequence
 *     is recognised. If this option is set, then "\R" only recognizes the newline
 *    characters '\r', '\n' and '\r\n'. Since: 2.34
 * @XREGEX_JAVASCRIPT_COMPAT: Changes behaviour so that it is compatible with
 *     JavaScript rather than PCRE. Since: 2.34
 *
 * Flags specifying compile-time options.
 *
 * Since: 2.14
 */
/* Remember to update XREGEX_COMPILE_MASK in gregex.c after
 * adding a new flag.
 */
typedef enum
{
  XREGEX_CASELESS          = 1 << 0,
  XREGEX_MULTILINE         = 1 << 1,
  XREGEX_DOTALL            = 1 << 2,
  XREGEX_EXTENDED          = 1 << 3,
  XREGEX_ANCHORED          = 1 << 4,
  XREGEX_DOLLAR_ENDONLY    = 1 << 5,
  XREGEX_UNGREEDY          = 1 << 9,
  XREGEX_RAW               = 1 << 11,
  XREGEX_NO_AUTO_CAPTURE   = 1 << 12,
  XREGEX_OPTIMIZE          = 1 << 13,
  XREGEX_FIRSTLINE         = 1 << 18,
  XREGEX_DUPNAMES          = 1 << 19,
  XREGEX_NEWLINE_CR        = 1 << 20,
  XREGEX_NEWLINE_LF        = 1 << 21,
  XREGEX_NEWLINE_CRLF      = XREGEX_NEWLINE_CR | XREGEX_NEWLINE_LF,
  XREGEX_NEWLINE_ANYCRLF   = XREGEX_NEWLINE_CR | 1 << 22,
  XREGEX_BSR_ANYCRLF       = 1 << 23,
  XREGEX_JAVASCRIPT_COMPAT = 1 << 25
} xregex_compile_flags_t;

/**
 * xregex_match_flags_t:
 * @XREGEX_MATCH_ANCHORED: The pattern is forced to be "anchored", that is,
 *     it is constrained to match only at the first matching point in the
 *     string that is being searched. This effect can also be achieved by
 *     appropriate constructs in the pattern itself such as the "^"
 *     metacharacter.
 * @XREGEX_MATCH_NOTBOL: Specifies that first character of the string is
 *     not the beginning of a line, so the circumflex metacharacter should
 *     not match before it. Setting this without %XREGEX_MULTILINE (at
 *     compile time) causes circumflex never to match. This option affects
 *     only the behaviour of the circumflex metacharacter, it does not
 *     affect "\A".
 * @XREGEX_MATCH_NOTEOL: Specifies that the end of the subject string is
 *     not the end of a line, so the dollar metacharacter should not match
 *     it nor (except in multiline mode) a newline immediately before it.
 *     Setting this without %XREGEX_MULTILINE (at compile time) causes
 *     dollar never to match. This option affects only the behaviour of
 *     the dollar metacharacter, it does not affect "\Z" or "\z".
 * @XREGEX_MATCH_NOTEMPTY: An empty string is not considered to be a valid
 *     match if this option is set. If there are alternatives in the pattern,
 *     they are tried. If all the alternatives match the empty string, the
 *     entire match fails. For example, if the pattern "a?b?" is applied to
 *     a string not beginning with "a" or "b", it matches the empty string
 *     at the start of the string. With this flag set, this match is not
 *     valid, so xregex_t searches further into the string for occurrences
 *     of "a" or "b".
 * @XREGEX_MATCH_PARTIAL: Turns on the partial matching feature, for more
 *     documentation on partial matching see xmatch_info_is_partial_match().
 * @XREGEX_MATCH_NEWLINE_CR: Overrides the newline definition set when
 *     creating a new #xregex_t, setting the '\r' character as line terminator.
 * @XREGEX_MATCH_NEWLINE_LF: Overrides the newline definition set when
 *     creating a new #xregex_t, setting the '\n' character as line terminator.
 * @XREGEX_MATCH_NEWLINE_CRLF: Overrides the newline definition set when
 *     creating a new #xregex_t, setting the '\r\n' characters sequence as line terminator.
 * @XREGEX_MATCH_NEWLINE_ANY: Overrides the newline definition set when
 *     creating a new #xregex_t, any Unicode newline sequence
 *     is recognised as a newline. These are '\r', '\n' and '\rn', and the
 *     single characters U+000B LINE TABULATION, U+000C FORM FEED (FF),
 *     U+0085 NEXT LINE (NEL), U+2028 LINE SEPARATOR and
 *     U+2029 PARAGRAPH SEPARATOR.
 * @XREGEX_MATCH_NEWLINE_ANYCRLF: Overrides the newline definition set when
 *     creating a new #xregex_t; any '\r', '\n', or '\r\n' character sequence
 *     is recognized as a newline. Since: 2.34
 * @XREGEX_MATCH_BSR_ANYCRLF: Overrides the newline definition for "\R" set when
 *     creating a new #xregex_t; only '\r', '\n', or '\r\n' character sequences
 *     are recognized as a newline by "\R". Since: 2.34
 * @XREGEX_MATCH_BSR_ANY: Overrides the newline definition for "\R" set when
 *     creating a new #xregex_t; any Unicode newline character or character sequence
 *     are recognized as a newline by "\R". These are '\r', '\n' and '\rn', and the
 *     single characters U+000B LINE TABULATION, U+000C FORM FEED (FF),
 *     U+0085 NEXT LINE (NEL), U+2028 LINE SEPARATOR and
 *     U+2029 PARAGRAPH SEPARATOR. Since: 2.34
 * @XREGEX_MATCH_PARTIAL_SOFT: An alias for %XREGEX_MATCH_PARTIAL. Since: 2.34
 * @XREGEX_MATCH_PARTIAL_HARD: Turns on the partial matching feature. In contrast to
 *     to %XREGEX_MATCH_PARTIAL_SOFT, this stops matching as soon as a partial match
 *     is found, without continuing to search for a possible complete match. See
 *     xmatch_info_is_partial_match() for more information. Since: 2.34
 * @XREGEX_MATCH_NOTEMPTY_ATSTART: Like %XREGEX_MATCH_NOTEMPTY, but only applied to
 *     the start of the matched string. For anchored
 *     patterns this can only happen for pattern containing "\K". Since: 2.34
 *
 * Flags specifying match-time options.
 *
 * Since: 2.14
 */
/* Remember to update XREGEX_MATCH_MASK in gregex.c after
 * adding a new flag. */
typedef enum
{
  XREGEX_MATCH_ANCHORED         = 1 << 4,
  XREGEX_MATCH_NOTBOL           = 1 << 7,
  XREGEX_MATCH_NOTEOL           = 1 << 8,
  XREGEX_MATCH_NOTEMPTY         = 1 << 10,
  XREGEX_MATCH_PARTIAL          = 1 << 15,
  XREGEX_MATCH_NEWLINE_CR       = 1 << 20,
  XREGEX_MATCH_NEWLINE_LF       = 1 << 21,
  XREGEX_MATCH_NEWLINE_CRLF     = XREGEX_MATCH_NEWLINE_CR | XREGEX_MATCH_NEWLINE_LF,
  XREGEX_MATCH_NEWLINE_ANY      = 1 << 22,
  XREGEX_MATCH_NEWLINE_ANYCRLF  = XREGEX_MATCH_NEWLINE_CR | XREGEX_MATCH_NEWLINE_ANY,
  XREGEX_MATCH_BSR_ANYCRLF      = 1 << 23,
  XREGEX_MATCH_BSR_ANY          = 1 << 24,
  XREGEX_MATCH_PARTIAL_SOFT     = XREGEX_MATCH_PARTIAL,
  XREGEX_MATCH_PARTIAL_HARD     = 1 << 27,
  XREGEX_MATCH_NOTEMPTY_ATSTART = 1 << 28
} xregex_match_flags_t;

/**
 * xregex_t:
 *
 * A xregex_t is the "compiled" form of a regular expression pattern.
 * This structure is opaque and its fields cannot be accessed directly.
 *
 * Since: 2.14
 */
typedef struct _xregex		xregex_t;


/**
 * xmatch_info_t:
 *
 * A xmatch_info_t is an opaque struct used to return information about
 * matches.
 */
typedef struct _xmatch_info	xmatch_info_t;

/**
 * xregex_eval_callback_t:
 * @match_info: the #xmatch_info_t generated by the match.
 *     Use xmatch_info_get_regex() and xmatch_info_get_string() if you
 *     need the #xregex_t or the matched string.
 * @result: a #xstring_t containing the new string
 * @user_data: user data passed to xregex_replace_eval()
 *
 * Specifies the type of the function passed to xregex_replace_eval().
 * It is called for each occurrence of the pattern in the string passed
 * to xregex_replace_eval(), and it should append the replacement to
 * @result.
 *
 * Returns: %FALSE to continue the replacement process, %TRUE to stop it
 *
 * Since: 2.14
 */
typedef xboolean_t (*xregex_eval_callback_t)		(const xmatch_info_t *match_info,
						 xstring_t          *result,
						 xpointer_t          user_data);


XPL_AVAILABLE_IN_ALL
xregex_t		 *xregex_new			(const xchar_t         *pattern,
						 xregex_compile_flags_t   compile_options,
						 xregex_match_flags_t     match_options,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xregex_t           *xregex_ref			(xregex_t              *regex);
XPL_AVAILABLE_IN_ALL
void		  xregex_unref			(xregex_t              *regex);
XPL_AVAILABLE_IN_ALL
const xchar_t	 *xregex_get_pattern		(const xregex_t        *regex);
XPL_AVAILABLE_IN_ALL
xint_t		  xregex_get_max_backref	(const xregex_t        *regex);
XPL_AVAILABLE_IN_ALL
xint_t		  xregex_get_capture_count	(const xregex_t        *regex);
XPL_AVAILABLE_IN_ALL
xboolean_t          xregex_get_has_cr_or_lf      (const xregex_t        *regex);
XPL_AVAILABLE_IN_2_38
xint_t              xregex_get_max_lookbehind    (const xregex_t        *regex);
XPL_AVAILABLE_IN_ALL
xint_t		  xregex_get_string_number	(const xregex_t        *regex,
						 const xchar_t         *name);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xregex_escape_string		(const xchar_t         *string,
						 xint_t                 length);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xregex_escape_nul		(const xchar_t         *string,
						 xint_t                 length);

XPL_AVAILABLE_IN_ALL
xregex_compile_flags_t xregex_get_compile_flags    (const xregex_t        *regex);
XPL_AVAILABLE_IN_ALL
xregex_match_flags_t   xregex_get_match_flags      (const xregex_t        *regex);

/* Matching. */
XPL_AVAILABLE_IN_ALL
xboolean_t	  xregex_match_simple		(const xchar_t         *pattern,
						 const xchar_t         *string,
						 xregex_compile_flags_t   compile_options,
						 xregex_match_flags_t     match_options);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xregex_match			(const xregex_t        *regex,
						 const xchar_t         *string,
						 xregex_match_flags_t     match_options,
						 xmatch_info_t         **match_info);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xregex_match_full		(const xregex_t        *regex,
						 const xchar_t         *string,
						 xssize_t               string_len,
						 xint_t                 start_position,
						 xregex_match_flags_t     match_options,
						 xmatch_info_t         **match_info,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xregex_match_all		(const xregex_t        *regex,
						 const xchar_t         *string,
						 xregex_match_flags_t     match_options,
						 xmatch_info_t         **match_info);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xregex_match_all_full	(const xregex_t        *regex,
						 const xchar_t         *string,
						 xssize_t               string_len,
						 xint_t                 start_position,
						 xregex_match_flags_t     match_options,
						 xmatch_info_t         **match_info,
						 xerror_t             **error);

/* String splitting. */
XPL_AVAILABLE_IN_ALL
xchar_t		**xregex_split_simple		(const xchar_t         *pattern,
						 const xchar_t         *string,
						 xregex_compile_flags_t   compile_options,
						 xregex_match_flags_t     match_options);
XPL_AVAILABLE_IN_ALL
xchar_t		**xregex_split			(const xregex_t        *regex,
						 const xchar_t         *string,
						 xregex_match_flags_t     match_options);
XPL_AVAILABLE_IN_ALL
xchar_t		**xregex_split_full		(const xregex_t        *regex,
						 const xchar_t         *string,
						 xssize_t               string_len,
						 xint_t                 start_position,
						 xregex_match_flags_t     match_options,
						 xint_t                 max_tokens,
						 xerror_t             **error);

/* String replacement. */
XPL_AVAILABLE_IN_ALL
xchar_t		 *xregex_replace		(const xregex_t        *regex,
						 const xchar_t         *string,
						 xssize_t               string_len,
						 xint_t                 start_position,
						 const xchar_t         *replacement,
						 xregex_match_flags_t     match_options,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xregex_replace_literal	(const xregex_t        *regex,
						 const xchar_t         *string,
						 xssize_t               string_len,
						 xint_t                 start_position,
						 const xchar_t         *replacement,
						 xregex_match_flags_t     match_options,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xregex_replace_eval		(const xregex_t        *regex,
						 const xchar_t         *string,
						 xssize_t               string_len,
						 xint_t                 start_position,
						 xregex_match_flags_t     match_options,
						 xregex_eval_callback_t   eval,
						 xpointer_t             user_data,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xregex_check_replacement	(const xchar_t         *replacement,
						 xboolean_t            *has_references,
						 xerror_t             **error);

/* Match info */
XPL_AVAILABLE_IN_ALL
xregex_t		 *xmatch_info_get_regex	(const xmatch_info_t    *match_info);
XPL_AVAILABLE_IN_ALL
const xchar_t      *xmatch_info_get_string       (const xmatch_info_t    *match_info);

XPL_AVAILABLE_IN_ALL
xmatch_info_t       *xmatch_info_ref              (xmatch_info_t          *match_info);
XPL_AVAILABLE_IN_ALL
void              xmatch_info_unref            (xmatch_info_t          *match_info);
XPL_AVAILABLE_IN_ALL
void		  xmatch_info_free		(xmatch_info_t          *match_info);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xmatch_info_next		(xmatch_info_t          *match_info,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xmatch_info_matches		(const xmatch_info_t    *match_info);
XPL_AVAILABLE_IN_ALL
xint_t		  xmatch_info_get_match_count	(const xmatch_info_t    *match_info);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xmatch_info_is_partial_match	(const xmatch_info_t    *match_info);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xmatch_info_expand_references(const xmatch_info_t    *match_info,
						 const xchar_t         *string_to_expand,
						 xerror_t             **error);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xmatch_info_fetch		(const xmatch_info_t    *match_info,
						 xint_t                 match_num);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xmatch_info_fetch_pos	(const xmatch_info_t    *match_info,
						 xint_t                 match_num,
						 xint_t                *start_pos,
						 xint_t                *end_pos);
XPL_AVAILABLE_IN_ALL
xchar_t		 *xmatch_info_fetch_named	(const xmatch_info_t    *match_info,
						 const xchar_t         *name);
XPL_AVAILABLE_IN_ALL
xboolean_t	  xmatch_info_fetch_named_pos	(const xmatch_info_t    *match_info,
						 const xchar_t         *name,
						 xint_t                *start_pos,
						 xint_t                *end_pos);
XPL_AVAILABLE_IN_ALL
xchar_t		**xmatch_info_fetch_all	(const xmatch_info_t    *match_info);

G_END_DECLS

#endif  /*  __XREGEX_H__ */
