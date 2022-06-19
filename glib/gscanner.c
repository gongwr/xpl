/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * xscanner_t: Flexible lexical scanner for general purpose.
 * Copyright (C) 1997, 1998 Tim Janik
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "gscanner.h"

#include "gprintfint.h"
#include "gstrfuncs.h"
#include "gstring.h"
#include "gtestutils.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h>
#endif


/**
 * SECTION:scanner
 * @title: Lexical Scanner
 * @short_description: a general purpose lexical scanner
 *
 * The #xscanner_t and its associated functions provide a
 * general purpose lexical scanner.
 */

/**
 * GScannerMsgFunc:
 * @scanner: a #xscanner_t
 * @message: the message
 * @error: %TRUE if the message signals an error,
 *     %FALSE if it signals a warning.
 *
 * Specifies the type of the message handler function.
 */

/**
 * G_CSET_a_2_z:
 *
 * The set of lowercase ASCII alphabet characters.
 * Used for specifying valid identifier characters
 * in #GScannerConfig.
 */

/**
 * G_CSET_A_2_Z:
 *
 * The set of uppercase ASCII alphabet characters.
 * Used for specifying valid identifier characters
 * in #GScannerConfig.
 */

/**
 * G_CSET_DIGITS:
 *
 * The set of ASCII digits.
 * Used for specifying valid identifier characters
 * in #GScannerConfig.
 */

/**
 * G_CSET_LATINC:
 *
 * The set of uppercase ISO 8859-1 alphabet characters
 * which are not ASCII characters.
 * Used for specifying valid identifier characters
 * in #GScannerConfig.
 */

/**
 * G_CSET_LATINS:
 *
 * The set of lowercase ISO 8859-1 alphabet characters
 * which are not ASCII characters.
 * Used for specifying valid identifier characters
 * in #GScannerConfig.
 */

/**
 * GTokenType:
 * @G_TOKEN_EOF: the end of the file
 * @G_TOKEN_LEFT_PAREN: a '(' character
 * @G_TOKEN_LEFT_CURLY: a '{' character
 * @G_TOKEN_LEFT_BRACE: a '[' character
 * @G_TOKEN_RIGHT_CURLY: a '}' character
 * @G_TOKEN_RIGHT_PAREN: a ')' character
 * @G_TOKEN_RIGHT_BRACE: a ']' character
 * @G_TOKEN_EQUAL_SIGN: a '=' character
 * @G_TOKEN_COMMA: a ',' character
 * @G_TOKEN_NONE: not a token
 * @G_TOKEN_ERROR: an error occurred
 * @G_TOKEN_CHAR: a character
 * @G_TOKEN_BINARY: a binary integer
 * @G_TOKEN_OCTAL: an octal integer
 * @G_TOKEN_INT: an integer
 * @G_TOKEN_HEX: a hex integer
 * @G_TOKEN_FLOAT: a floating point number
 * @G_TOKEN_STRING: a string
 * @G_TOKEN_SYMBOL: a symbol
 * @G_TOKEN_IDENTIFIER: an identifier
 * @G_TOKEN_IDENTIFIER_NULL: a null identifier
 * @G_TOKEN_COMMENT_SINGLE: one line comment
 * @G_TOKEN_COMMENT_MULTI: multi line comment
 *
 * The possible types of token returned from each
 * g_scanner_get_next_token() call.
 */

/**
 * GTokenValue:
 * @v_symbol: token symbol value
 * @v_identifier: token identifier value
 * @v_binary: token binary integer value
 * @v_octal: octal integer value
 * @v_int: integer value
 * @v_int64: 64-bit integer value
 * @v_float: floating point value
 * @v_hex: hex integer value
 * @v_string: string value
 * @v_comment: comment value
 * @v_char: character value
 * @v_error: error value
 *
 * A union holding the value of the token.
 */

/**
 * GErrorType:
 * @G_ERR_UNKNOWN: unknown error
 * @G_ERR_UNEXP_EOF: unexpected end of file
 * @G_ERR_UNEXP_EOF_IN_STRING: unterminated string constant
 * @G_ERR_UNEXP_EOF_IN_COMMENT: unterminated comment
 * @G_ERR_NON_DIGIT_IN_CONST: non-digit character in a number
 * @G_ERR_DIGIT_RADIX: digit beyond radix in a number
 * @G_ERR_FLOAT_RADIX: non-decimal floating point number
 * @G_ERR_FLOAT_MALFORMED: malformed floating point number
 *
 * The possible errors, used in the @v_error field
 * of #GTokenValue, when the token is a %G_TOKEN_ERROR.
 */

/**
 * xscanner_t:
 * @user_data: unused
 * @max_parse_errors: unused
 * @parse_errors: g_scanner_error() increments this field
 * @input_name: name of input stream, featured by the default message handler
 * @qdata: quarked data
 * @config: link into the scanner configuration
 * @token: token parsed by the last g_scanner_get_next_token()
 * @value: value of the last token from g_scanner_get_next_token()
 * @line: line number of the last token from g_scanner_get_next_token()
 * @position: char number of the last token from g_scanner_get_next_token()
 * @next_token: token parsed by the last g_scanner_peek_next_token()
 * @next_value: value of the last token from g_scanner_peek_next_token()
 * @next_line: line number of the last token from g_scanner_peek_next_token()
 * @next_position: char number of the last token from g_scanner_peek_next_token()
 * @msg_handler: handler function for _warn and _error
 *
 * The data structure representing a lexical scanner.
 *
 * You should set @input_name after creating the scanner, since
 * it is used by the default message handler when displaying
 * warnings and errors. If you are scanning a file, the filename
 * would be a good choice.
 *
 * The @user_data and @max_parse_errors fields are not used.
 * If you need to associate extra data with the scanner you
 * can place them here.
 *
 * If you want to use your own message handler you can set the
 * @msg_handler field. The type of the message handler function
 * is declared by #GScannerMsgFunc.
 */

/**
 * GScannerConfig:
 * @cset_skip_characters: specifies which characters should be skipped
 *     by the scanner (the default is the whitespace characters: space,
 *     tab, carriage-return and line-feed).
 * @cset_identifier_first: specifies the characters which can start
 *     identifiers (the default is %G_CSET_a_2_z, "_", and %G_CSET_A_2_Z).
 * @cset_identifier_nth: specifies the characters which can be used
 *     in identifiers, after the first character (the default is
 *     %G_CSET_a_2_z, "_0123456789", %G_CSET_A_2_Z, %G_CSET_LATINS,
 *     %G_CSET_LATINC).
 * @cpair_comment_single: specifies the characters at the start and
 *     end of single-line comments. The default is "#\n" which means
 *     that single-line comments start with a '#' and continue until
 *     a '\n' (end of line).
 * @case_sensitive: specifies if symbols are case sensitive (the
 *     default is %FALSE).
 * @skip_comment_multi: specifies if multi-line comments are skipped
 *     and not returned as tokens (the default is %TRUE).
 * @skip_comment_single: specifies if single-line comments are skipped
 *     and not returned as tokens (the default is %TRUE).
 * @scan_comment_multi: specifies if multi-line comments are recognized
 *     (the default is %TRUE).
 * @scan_identifier: specifies if identifiers are recognized (the
 *     default is %TRUE).
 * @scan_identifier_1char: specifies if single-character
 *     identifiers are recognized (the default is %FALSE).
 * @scan_identifier_NULL: specifies if %NULL is reported as
 *     %G_TOKEN_IDENTIFIER_NULL (the default is %FALSE).
 * @scan_symbols: specifies if symbols are recognized (the default
 *     is %TRUE).
 * @scan_binary: specifies if binary numbers are recognized (the
 *     default is %FALSE).
 * @scan_octal: specifies if octal numbers are recognized (the
 *     default is %TRUE).
 * @scan_float: specifies if floating point numbers are recognized
 *     (the default is %TRUE).
 * @scan_hex: specifies if hexadecimal numbers are recognized (the
 *     default is %TRUE).
 * @scan_hex_dollar: specifies if '$' is recognized as a prefix for
 *     hexadecimal numbers (the default is %FALSE).
 * @scan_string_sq: specifies if strings can be enclosed in single
 *     quotes (the default is %TRUE).
 * @scan_string_dq: specifies if strings can be enclosed in double
 *     quotes (the default is %TRUE).
 * @numbers_2_int: specifies if binary, octal and hexadecimal numbers
 *     are reported as %G_TOKEN_INT (the default is %TRUE).
 * @int_2_float: specifies if all numbers are reported as %G_TOKEN_FLOAT
 *     (the default is %FALSE).
 * @identifier_2_string: specifies if identifiers are reported as strings
 *     (the default is %FALSE).
 * @char_2_token: specifies if characters are reported by setting
 *     `token = ch` or as %G_TOKEN_CHAR (the default is %TRUE).
 * @symbol_2_token: specifies if symbols are reported by setting
 *     `token = v_symbol` or as %G_TOKEN_SYMBOL (the default is %FALSE).
 * @scope_0_fallback: specifies if a symbol is searched for in the
 *     default scope in addition to the current scope (the default is %FALSE).
 * @store_int64: use value.v_int64 rather than v_int
 *
 * Specifies the #xscanner_t parser configuration. Most settings can
 * be changed during the parsing phase and will affect the lexical
 * parsing of the next unpeeked token.
 */

/* --- defines --- */
#define	to_lower(c)				( \
	(xuchar_t) (							\
	  ( (((xuchar_t)(c))>='A' && ((xuchar_t)(c))<='Z') * ('a'-'A') ) |	\
	  ( (((xuchar_t)(c))>=192 && ((xuchar_t)(c))<=214) * (224-192) ) |	\
	  ( (((xuchar_t)(c))>=216 && ((xuchar_t)(c))<=222) * (248-216) ) |	\
	  ((xuchar_t)(c))							\
	)								\
)
#define	READ_BUFFER_SIZE	(4000)


/* --- typedefs --- */
typedef	struct	_GScannerKey	GScannerKey;

struct	_GScannerKey
{
  xuint_t		 scope_id;
  xchar_t		*symbol;
  xpointer_t	 value;
};


/* --- variables --- */
static const GScannerConfig g_scanner_config_template =
{
  (
   " \t\r\n"
   )			/* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )			/* cset_identifier_first */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   G_CSET_DIGITS
   G_CSET_LATINS
   G_CSET_LATINC
   )			/* cset_identifier_nth */,
  ( "#\n" )		/* cpair_comment_single */,

  FALSE			/* case_sensitive */,

  TRUE			/* skip_comment_multi */,
  TRUE			/* skip_comment_single */,
  TRUE			/* scan_comment_multi */,
  TRUE			/* scan_identifier */,
  FALSE			/* scan_identifier_1char */,
  FALSE			/* scan_identifier_NULL */,
  TRUE			/* scan_symbols */,
  FALSE			/* scan_binary */,
  TRUE			/* scan_octal */,
  TRUE			/* scan_float */,
  TRUE			/* scan_hex */,
  FALSE			/* scan_hex_dollar */,
  TRUE			/* scan_string_sq */,
  TRUE			/* scan_string_dq */,
  TRUE			/* numbers_2_int */,
  FALSE			/* int_2_float */,
  FALSE			/* identifier_2_string */,
  TRUE			/* char_2_token */,
  FALSE			/* symbol_2_token */,
  FALSE			/* scope_0_fallback */,
  FALSE			/* store_int64 */,
  0    			/* padding_dummy */
};


/* --- prototypes --- */
static inline
GScannerKey*	g_scanner_lookup_internal (xscanner_t	*scanner,
					   xuint_t	 scope_id,
					   const xchar_t	*symbol);
static xboolean_t	g_scanner_key_equal	  (xconstpointer v1,
					   xconstpointer v2);
static xuint_t	g_scanner_key_hash	  (xconstpointer v);
static void	g_scanner_get_token_ll	  (xscanner_t	*scanner,
					   GTokenType	*token_p,
					   GTokenValue	*value_p,
					   xuint_t	*line_p,
					   xuint_t	*position_p);
static void	g_scanner_get_token_i	  (xscanner_t	*scanner,
					   GTokenType	*token_p,
					   GTokenValue	*value_p,
					   xuint_t	*line_p,
					   xuint_t	*position_p);

static xuchar_t	g_scanner_peek_next_char  (xscanner_t	*scanner);
static xuchar_t	g_scanner_get_char	  (xscanner_t	*scanner,
					   xuint_t	*line_p,
					   xuint_t	*position_p);
static void	g_scanner_msg_handler	  (xscanner_t	*scanner,
					   xchar_t	*message,
					   xboolean_t	 is_error);


/* --- functions --- */
static inline xint_t
g_scanner_char_2_num (xuchar_t	c,
		      xuchar_t	base)
{
  if (c >= '0' && c <= '9')
    c -= '0';
  else if (c >= 'A' && c <= 'Z')
    c -= 'A' - 10;
  else if (c >= 'a' && c <= 'z')
    c -= 'a' - 10;
  else
    return -1;

  if (c < base)
    return c;

  return -1;
}

/**
 * g_scanner_new:
 * @config_templ: the initial scanner settings
 *
 * Creates a new #xscanner_t.
 *
 * The @config_templ structure specifies the initial settings
 * of the scanner, which are copied into the #xscanner_t
 * @config field. If you pass %NULL then the default settings
 * are used.
 *
 * Returns: the new #xscanner_t
 */
xscanner_t *
g_scanner_new (const GScannerConfig *config_templ)
{
  xscanner_t *scanner;

  if (!config_templ)
    config_templ = &g_scanner_config_template;

  scanner = g_new0 (xscanner_t, 1);

  scanner->user_data = NULL;
  scanner->max_parse_errors = 1;
  scanner->parse_errors	= 0;
  scanner->input_name = NULL;
  g_datalist_init (&scanner->qdata);

  scanner->config = g_new0 (GScannerConfig, 1);

  scanner->config->case_sensitive	 = config_templ->case_sensitive;
  scanner->config->cset_skip_characters	 = config_templ->cset_skip_characters;
  if (!scanner->config->cset_skip_characters)
    scanner->config->cset_skip_characters = "";
  scanner->config->cset_identifier_first = config_templ->cset_identifier_first;
  scanner->config->cset_identifier_nth	 = config_templ->cset_identifier_nth;
  scanner->config->cpair_comment_single	 = config_templ->cpair_comment_single;
  scanner->config->skip_comment_multi	 = config_templ->skip_comment_multi;
  scanner->config->skip_comment_single	 = config_templ->skip_comment_single;
  scanner->config->scan_comment_multi	 = config_templ->scan_comment_multi;
  scanner->config->scan_identifier	 = config_templ->scan_identifier;
  scanner->config->scan_identifier_1char = config_templ->scan_identifier_1char;
  scanner->config->scan_identifier_NULL	 = config_templ->scan_identifier_NULL;
  scanner->config->scan_symbols		 = config_templ->scan_symbols;
  scanner->config->scan_binary		 = config_templ->scan_binary;
  scanner->config->scan_octal		 = config_templ->scan_octal;
  scanner->config->scan_float		 = config_templ->scan_float;
  scanner->config->scan_hex		 = config_templ->scan_hex;
  scanner->config->scan_hex_dollar	 = config_templ->scan_hex_dollar;
  scanner->config->scan_string_sq	 = config_templ->scan_string_sq;
  scanner->config->scan_string_dq	 = config_templ->scan_string_dq;
  scanner->config->numbers_2_int	 = config_templ->numbers_2_int;
  scanner->config->int_2_float		 = config_templ->int_2_float;
  scanner->config->identifier_2_string	 = config_templ->identifier_2_string;
  scanner->config->char_2_token		 = config_templ->char_2_token;
  scanner->config->symbol_2_token	 = config_templ->symbol_2_token;
  scanner->config->scope_0_fallback	 = config_templ->scope_0_fallback;
  scanner->config->store_int64		 = config_templ->store_int64;

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;

  scanner->next_token = G_TOKEN_NONE;
  scanner->next_value.v_int64 = 0;
  scanner->next_line = 1;
  scanner->next_position = 0;

  scanner->symbol_table = xhash_table_new (g_scanner_key_hash, g_scanner_key_equal);
  scanner->input_fd = -1;
  scanner->text = NULL;
  scanner->text_end = NULL;
  scanner->buffer = NULL;
  scanner->scope_id = 0;

  scanner->msg_handler = g_scanner_msg_handler;

  return scanner;
}

static inline void
g_scanner_free_value (GTokenType     *token_p,
		      GTokenValue     *value_p)
{
  switch (*token_p)
    {
    case G_TOKEN_STRING:
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
    case G_TOKEN_COMMENT_SINGLE:
    case G_TOKEN_COMMENT_MULTI:
      g_free (value_p->v_string);
      break;

    default:
      break;
    }

  *token_p = G_TOKEN_NONE;
}

static void
g_scanner_destroy_symbol_table_entry (xpointer_t _key,
				      xpointer_t _value,
				      xpointer_t _data)
{
  GScannerKey *key = _key;

  g_free (key->symbol);
  g_free (key);
}

/**
 * g_scanner_destroy:
 * @scanner: a #xscanner_t
 *
 * Frees all memory used by the #xscanner_t.
 */
void
g_scanner_destroy (xscanner_t *scanner)
{
  g_return_if_fail (scanner != NULL);

  g_datalist_clear (&scanner->qdata);
  xhash_table_foreach (scanner->symbol_table,
			g_scanner_destroy_symbol_table_entry, NULL);
  xhash_table_destroy (scanner->symbol_table);
  g_scanner_free_value (&scanner->token, &scanner->value);
  g_scanner_free_value (&scanner->next_token, &scanner->next_value);
  g_free (scanner->config);
  g_free (scanner->buffer);
  g_free (scanner);
}

static void
g_scanner_msg_handler (xscanner_t		*scanner,
		       xchar_t		*message,
		       xboolean_t		is_error)
{
  g_return_if_fail (scanner != NULL);

  _g_fprintf (stderr, "%s:%d: ",
	      scanner->input_name ? scanner->input_name : "<memory>",
	      scanner->line);
  if (is_error)
    _g_fprintf (stderr, "error: ");
  _g_fprintf (stderr, "%s\n", message);
}

/**
 * g_scanner_error:
 * @scanner: a #xscanner_t
 * @format: the message format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Outputs an error message, via the #xscanner_t message handler.
 */
void
g_scanner_error (xscanner_t	*scanner,
		 const xchar_t	*format,
		 ...)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (format != NULL);

  scanner->parse_errors++;

  if (scanner->msg_handler)
    {
      va_list args;
      xchar_t *string;

      va_start (args, format);
      string = xstrdup_vprintf (format, args);
      va_end (args);

      scanner->msg_handler (scanner, string, TRUE);

      g_free (string);
    }
}

/**
 * g_scanner_warn:
 * @scanner: a #xscanner_t
 * @format: the message format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Outputs a warning message, via the #xscanner_t message handler.
 */
void
g_scanner_warn (xscanner_t       *scanner,
		const xchar_t    *format,
		...)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (format != NULL);

  if (scanner->msg_handler)
    {
      va_list args;
      xchar_t *string;

      va_start (args, format);
      string = xstrdup_vprintf (format, args);
      va_end (args);

      scanner->msg_handler (scanner, string, FALSE);

      g_free (string);
    }
}

static xboolean_t
g_scanner_key_equal (xconstpointer v1,
		     xconstpointer v2)
{
  const GScannerKey *key1 = v1;
  const GScannerKey *key2 = v2;

  return (key1->scope_id == key2->scope_id) && (strcmp (key1->symbol, key2->symbol) == 0);
}

static xuint_t
g_scanner_key_hash (xconstpointer v)
{
  const GScannerKey *key = v;
  xchar_t *c;
  xuint_t h;

  h = key->scope_id;
  for (c = key->symbol; *c; c++)
    h = (h << 5) - h + *c;

  return h;
}

static inline GScannerKey*
g_scanner_lookup_internal (xscanner_t	*scanner,
			   xuint_t	 scope_id,
			   const xchar_t	*symbol)
{
  GScannerKey	*key_p;
  GScannerKey key;

  key.scope_id = scope_id;

  if (!scanner->config->case_sensitive)
    {
      xchar_t *d;
      const xchar_t *c;

      key.symbol = g_new (xchar_t, strlen (symbol) + 1);
      for (d = key.symbol, c = symbol; *c; c++, d++)
	*d = to_lower (*c);
      *d = 0;
      key_p = xhash_table_lookup (scanner->symbol_table, &key);
      g_free (key.symbol);
    }
  else
    {
      key.symbol = (xchar_t*) symbol;
      key_p = xhash_table_lookup (scanner->symbol_table, &key);
    }

  return key_p;
}

/**
 * g_scanner_add_symbol:
 * @scanner: a #xscanner_t
 * @symbol: the symbol to add
 * @value: the value of the symbol
 *
 * Adds a symbol to the default scope.
 *
 * Deprecated: 2.2: Use g_scanner_scope_add_symbol() instead.
 */

/**
 * g_scanner_scope_add_symbol:
 * @scanner: a #xscanner_t
 * @scope_id: the scope id
 * @symbol: the symbol to add
 * @value: the value of the symbol
 *
 * Adds a symbol to the given scope.
 */
void
g_scanner_scope_add_symbol (xscanner_t	*scanner,
			    xuint_t	 scope_id,
			    const xchar_t	*symbol,
			    xpointer_t	 value)
{
  GScannerKey	*key;

  g_return_if_fail (scanner != NULL);
  g_return_if_fail (symbol != NULL);

  key = g_scanner_lookup_internal (scanner, scope_id, symbol);

  if (!key)
    {
      key = g_new (GScannerKey, 1);
      key->scope_id = scope_id;
      key->symbol = xstrdup (symbol);
      key->value = value;
      if (!scanner->config->case_sensitive)
	{
	  xchar_t *c;

	  c = key->symbol;
	  while (*c != 0)
	    {
	      *c = to_lower (*c);
	      c++;
	    }
	}
      xhash_table_add (scanner->symbol_table, key);
    }
  else
    key->value = value;
}

/**
 * g_scanner_remove_symbol:
 * @scanner: a #xscanner_t
 * @symbol: the symbol to remove
 *
 * Removes a symbol from the default scope.
 *
 * Deprecated: 2.2: Use g_scanner_scope_remove_symbol() instead.
 */

/**
 * g_scanner_scope_remove_symbol:
 * @scanner: a #xscanner_t
 * @scope_id: the scope id
 * @symbol: the symbol to remove
 *
 * Removes a symbol from a scope.
 */
void
g_scanner_scope_remove_symbol (xscanner_t	   *scanner,
			       xuint_t	    scope_id,
			       const xchar_t *symbol)
{
  GScannerKey	*key;

  g_return_if_fail (scanner != NULL);
  g_return_if_fail (symbol != NULL);

  key = g_scanner_lookup_internal (scanner, scope_id, symbol);

  if (key)
    {
      xhash_table_remove (scanner->symbol_table, key);
      g_free (key->symbol);
      g_free (key);
    }
}

/**
 * g_scanner_freeze_symbol_table:
 * @scanner: a #xscanner_t
 *
 * There is no reason to use this macro, since it does nothing.
 *
 * Deprecated: 2.2: This macro does nothing.
 */

/**
 * g_scanner_thaw_symbol_table:
 * @scanner: a #xscanner_t
 *
 * There is no reason to use this macro, since it does nothing.
 *
 * Deprecated: 2.2: This macro does nothing.
 */

/**
 * g_scanner_lookup_symbol:
 * @scanner: a #xscanner_t
 * @symbol: the symbol to look up
 *
 * Looks up a symbol in the current scope and return its value.
 * If the symbol is not bound in the current scope, %NULL is
 * returned.
 *
 * Returns: the value of @symbol in the current scope, or %NULL
 *     if @symbol is not bound in the current scope
 */
xpointer_t
g_scanner_lookup_symbol (xscanner_t	*scanner,
			 const xchar_t	*symbol)
{
  GScannerKey	*key;
  xuint_t scope_id;

  xreturn_val_if_fail (scanner != NULL, NULL);

  if (!symbol)
    return NULL;

  scope_id = scanner->scope_id;
  key = g_scanner_lookup_internal (scanner, scope_id, symbol);
  if (!key && scope_id && scanner->config->scope_0_fallback)
    key = g_scanner_lookup_internal (scanner, 0, symbol);

  if (key)
    return key->value;
  else
    return NULL;
}

/**
 * g_scanner_scope_lookup_symbol:
 * @scanner: a #xscanner_t
 * @scope_id: the scope id
 * @symbol: the symbol to look up
 *
 * Looks up a symbol in a scope and return its value. If the
 * symbol is not bound in the scope, %NULL is returned.
 *
 * Returns: the value of @symbol in the given scope, or %NULL
 *     if @symbol is not bound in the given scope.
 *
 */
xpointer_t
g_scanner_scope_lookup_symbol (xscanner_t	      *scanner,
			       xuint_t	       scope_id,
			       const xchar_t    *symbol)
{
  GScannerKey	*key;

  xreturn_val_if_fail (scanner != NULL, NULL);

  if (!symbol)
    return NULL;

  key = g_scanner_lookup_internal (scanner, scope_id, symbol);

  if (key)
    return key->value;
  else
    return NULL;
}

/**
 * g_scanner_set_scope:
 * @scanner: a #xscanner_t
 * @scope_id: the new scope id
 *
 * Sets the current scope.
 *
 * Returns: the old scope id
 */
xuint_t
g_scanner_set_scope (xscanner_t	    *scanner,
		     xuint_t	     scope_id)
{
  xuint_t old_scope_id;

  xreturn_val_if_fail (scanner != NULL, 0);

  old_scope_id = scanner->scope_id;
  scanner->scope_id = scope_id;

  return old_scope_id;
}

static void
g_scanner_foreach_internal (xpointer_t  _key,
			    xpointer_t  _value,
			    xpointer_t  _user_data)
{
  GScannerKey *key;
  xpointer_t *d;
  GHFunc func;
  xpointer_t user_data;
  xuint_t *scope_id;

  d = _user_data;
  func = (GHFunc) d[0];
  user_data = d[1];
  scope_id = d[2];
  key = _value;

  if (key->scope_id == *scope_id)
    func (key->symbol, key->value, user_data);
}

/**
 * g_scanner_foreach_symbol:
 * @scanner: a #xscanner_t
 * @func: the function to call with each symbol
 * @data: data to pass to the function
 *
 * Calls a function for each symbol in the default scope.
 *
 * Deprecated: 2.2: Use g_scanner_scope_foreach_symbol() instead.
 */

/**
 * g_scanner_scope_foreach_symbol:
 * @scanner: a #xscanner_t
 * @scope_id: the scope id
 * @func: the function to call for each symbol/value pair
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each of the symbol/value pairs
 * in the given scope of the #xscanner_t. The function is passed
 * the symbol and value of each pair, and the given @user_data
 * parameter.
 */
void
g_scanner_scope_foreach_symbol (xscanner_t       *scanner,
				xuint_t		scope_id,
				GHFunc		func,
				xpointer_t	user_data)
{
  xpointer_t d[3];

  g_return_if_fail (scanner != NULL);

  d[0] = (xpointer_t) func;
  d[1] = user_data;
  d[2] = &scope_id;

  xhash_table_foreach (scanner->symbol_table, g_scanner_foreach_internal, d);
}

/**
 * g_scanner_peek_next_token:
 * @scanner: a #xscanner_t
 *
 * Parses the next token, without removing it from the input stream.
 * The token data is placed in the @next_token, @next_value, @next_line,
 * and @next_position fields of the #xscanner_t structure.
 *
 * Note that, while the token is not removed from the input stream
 * (i.e. the next call to g_scanner_get_next_token() will return the
 * same token), it will not be reevaluated. This can lead to surprising
 * results when changing scope or the scanner configuration after peeking
 * the next token. Getting the next token after switching the scope or
 * configuration will return whatever was peeked before, regardless of
 * any symbols that may have been added or removed in the new scope.
 *
 * Returns: the type of the token
 */
GTokenType
g_scanner_peek_next_token (xscanner_t	*scanner)
{
  xreturn_val_if_fail (scanner != NULL, G_TOKEN_EOF);

  if (scanner->next_token == G_TOKEN_NONE)
    {
      scanner->next_line = scanner->line;
      scanner->next_position = scanner->position;
      g_scanner_get_token_i (scanner,
			     &scanner->next_token,
			     &scanner->next_value,
			     &scanner->next_line,
			     &scanner->next_position);
    }

  return scanner->next_token;
}

/**
 * g_scanner_get_next_token:
 * @scanner: a #xscanner_t
 *
 * Parses the next token just like g_scanner_peek_next_token()
 * and also removes it from the input stream. The token data is
 * placed in the @token, @value, @line, and @position fields of
 * the #xscanner_t structure.
 *
 * Returns: the type of the token
 */
GTokenType
g_scanner_get_next_token (xscanner_t	*scanner)
{
  xreturn_val_if_fail (scanner != NULL, G_TOKEN_EOF);

  if (scanner->next_token != G_TOKEN_NONE)
    {
      g_scanner_free_value (&scanner->token, &scanner->value);

      scanner->token = scanner->next_token;
      scanner->value = scanner->next_value;
      scanner->line = scanner->next_line;
      scanner->position = scanner->next_position;
      scanner->next_token = G_TOKEN_NONE;
    }
  else
    g_scanner_get_token_i (scanner,
			   &scanner->token,
			   &scanner->value,
			   &scanner->line,
			   &scanner->position);

  return scanner->token;
}

/**
 * g_scanner_cur_token:
 * @scanner: a #xscanner_t
 *
 * Gets the current token type. This is simply the @token
 * field in the #xscanner_t structure.
 *
 * Returns: the current token type
 */
GTokenType
g_scanner_cur_token (xscanner_t *scanner)
{
  xreturn_val_if_fail (scanner != NULL, G_TOKEN_EOF);

  return scanner->token;
}

/**
 * g_scanner_cur_value:
 * @scanner: a #xscanner_t
 *
 * Gets the current token value. This is simply the @value
 * field in the #xscanner_t structure.
 *
 * Returns: the current token value
 */
GTokenValue
g_scanner_cur_value (xscanner_t *scanner)
{
  GTokenValue v;

  v.v_int64 = 0;

  xreturn_val_if_fail (scanner != NULL, v);

  /* MSC isn't capable of handling return scanner->value; ? */

  v = scanner->value;

  return v;
}

/**
 * g_scanner_cur_line:
 * @scanner: a #xscanner_t
 *
 * Returns the current line in the input stream (counting
 * from 1). This is the line of the last token parsed via
 * g_scanner_get_next_token().
 *
 * Returns: the current line
 */
xuint_t
g_scanner_cur_line (xscanner_t *scanner)
{
  xreturn_val_if_fail (scanner != NULL, 0);

  return scanner->line;
}

/**
 * g_scanner_cur_position:
 * @scanner: a #xscanner_t
 *
 * Returns the current position in the current line (counting
 * from 0). This is the position of the last token parsed via
 * g_scanner_get_next_token().
 *
 * Returns: the current position on the line
 */
xuint_t
g_scanner_cur_position (xscanner_t *scanner)
{
  xreturn_val_if_fail (scanner != NULL, 0);

  return scanner->position;
}

/**
 * g_scanner_eof:
 * @scanner: a #xscanner_t
 *
 * Returns %TRUE if the scanner has reached the end of
 * the file or text buffer.
 *
 * Returns: %TRUE if the scanner has reached the end of
 *     the file or text buffer
 */
xboolean_t
g_scanner_eof (xscanner_t	*scanner)
{
  xreturn_val_if_fail (scanner != NULL, TRUE);

  return scanner->token == G_TOKEN_EOF || scanner->token == G_TOKEN_ERROR;
}

/**
 * g_scanner_input_file:
 * @scanner: a #xscanner_t
 * @input_fd: a file descriptor
 *
 * Prepares to scan a file.
 */
void
g_scanner_input_file (xscanner_t *scanner,
		      xint_t	input_fd)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (input_fd >= 0);

  if (scanner->input_fd >= 0)
    g_scanner_sync_file_offset (scanner);

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;
  scanner->next_token = G_TOKEN_NONE;

  scanner->input_fd = input_fd;
  scanner->text = NULL;
  scanner->text_end = NULL;

  if (!scanner->buffer)
    scanner->buffer = g_new (xchar_t, READ_BUFFER_SIZE + 1);
}

/**
 * g_scanner_input_text:
 * @scanner: a #xscanner_t
 * @text: the text buffer to scan
 * @text_len: the length of the text buffer
 *
 * Prepares to scan a text buffer.
 */
void
g_scanner_input_text (xscanner_t	  *scanner,
		      const xchar_t *text,
		      xuint_t	   text_len)
{
  g_return_if_fail (scanner != NULL);
  if (text_len)
    g_return_if_fail (text != NULL);
  else
    text = NULL;

  if (scanner->input_fd >= 0)
    g_scanner_sync_file_offset (scanner);

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;
  scanner->next_token = G_TOKEN_NONE;

  scanner->input_fd = -1;
  scanner->text = text;
  scanner->text_end = text + text_len;

  if (scanner->buffer)
    {
      g_free (scanner->buffer);
      scanner->buffer = NULL;
    }
}

static xuchar_t
g_scanner_peek_next_char (xscanner_t *scanner)
{
  if (scanner->text < scanner->text_end)
    {
      return *scanner->text;
    }
  else if (scanner->input_fd >= 0)
    {
      xint_t count;
      xchar_t *buffer;

      buffer = scanner->buffer;
      do
	{
	  count = read (scanner->input_fd, buffer, READ_BUFFER_SIZE);
	}
      while (count == -1 && (errno == EINTR || errno == EAGAIN));

      if (count < 1)
	{
	  scanner->input_fd = -1;

	  return 0;
	}
      else
	{
	  scanner->text = buffer;
	  scanner->text_end = buffer + count;

	  return *buffer;
	}
    }
  else
    return 0;
}

/**
 * g_scanner_sync_file_offset:
 * @scanner: a #xscanner_t
 *
 * Rewinds the filedescriptor to the current buffer position
 * and blows the file read ahead buffer. This is useful for
 * third party uses of the scanners filedescriptor, which hooks
 * onto the current scanning position.
 */
void
g_scanner_sync_file_offset (xscanner_t *scanner)
{
  g_return_if_fail (scanner != NULL);

  /* for file input, rewind the filedescriptor to the current
   * buffer position and blow the file read ahead buffer. useful
   * for third party uses of our file descriptor, which hooks
   * onto the current scanning position.
   */

  if (scanner->input_fd >= 0 && scanner->text_end > scanner->text)
    {
      xint_t buffered;

      buffered = scanner->text_end - scanner->text;
      if (lseek (scanner->input_fd, - buffered, SEEK_CUR) >= 0)
	{
	  /* we succeeded, blow our buffer's contents now */
	  scanner->text = NULL;
	  scanner->text_end = NULL;
	}
      else
	errno = 0;
    }
}

static xuchar_t
g_scanner_get_char (xscanner_t	*scanner,
		    xuint_t	*line_p,
		    xuint_t	*position_p)
{
  xuchar_t fchar;

  if (scanner->text < scanner->text_end)
    fchar = *(scanner->text++);
  else if (scanner->input_fd >= 0)
    {
      xint_t count;
      xchar_t *buffer;

      buffer = scanner->buffer;
      do
	{
	  count = read (scanner->input_fd, buffer, READ_BUFFER_SIZE);
	}
      while (count == -1 && (errno == EINTR || errno == EAGAIN));

      if (count < 1)
	{
	  scanner->input_fd = -1;
	  fchar = 0;
	}
      else
	{
	  scanner->text = buffer + 1;
	  scanner->text_end = buffer + count;
	  fchar = *buffer;
	  if (!fchar)
	    {
	      g_scanner_sync_file_offset (scanner);
	      scanner->text_end = scanner->text;
	      scanner->input_fd = -1;
	    }
	}
    }
  else
    fchar = 0;

  if (fchar == '\n')
    {
      (*position_p) = 0;
      (*line_p)++;
    }
  else if (fchar)
    {
      (*position_p)++;
    }

  return fchar;
}

/**
 * g_scanner_unexp_token:
 * @scanner: a #xscanner_t
 * @expected_token: the expected token
 * @identifier_spec: a string describing how the scanner's user
 *     refers to identifiers (%NULL defaults to "identifier").
 *     This is used if @expected_token is %G_TOKEN_IDENTIFIER or
 *     %G_TOKEN_IDENTIFIER_NULL.
 * @symbol_spec: a string describing how the scanner's user refers
 *     to symbols (%NULL defaults to "symbol"). This is used if
 *     @expected_token is %G_TOKEN_SYMBOL or any token value greater
 *     than %G_TOKEN_LAST.
 * @symbol_name: the name of the symbol, if the scanner's current
 *     token is a symbol.
 * @message: a message string to output at the end of the
 *     warning/error, or %NULL.
 * @is_error: if %TRUE it is output as an error. If %FALSE it is
 *     output as a warning.
 *
 * Outputs a message through the scanner's msg_handler,
 * resulting from an unexpected token in the input stream.
 * Note that you should not call g_scanner_peek_next_token()
 * followed by g_scanner_unexp_token() without an intermediate
 * call to g_scanner_get_next_token(), as g_scanner_unexp_token()
 * evaluates the scanner's current token (not the peeked token)
 * to construct part of the message.
 */
void
g_scanner_unexp_token (xscanner_t		*scanner,
		       GTokenType	 expected_token,
		       const xchar_t	*identifier_spec,
		       const xchar_t	*symbol_spec,
		       const xchar_t	*symbol_name,
		       const xchar_t	*message,
		       xint_t		 is_error)
{
  xchar_t	*token_string;
  xuint_t	token_string_len;
  xchar_t	*expected_string;
  xuint_t	expected_string_len;
  xchar_t	*message_prefix;
  xboolean_t print_unexp;
  void (*msg_handler)	(xscanner_t*, const xchar_t*, ...);

  g_return_if_fail (scanner != NULL);

  if (is_error)
    msg_handler = g_scanner_error;
  else
    msg_handler = g_scanner_warn;

  if (!identifier_spec)
    identifier_spec = "identifier";
  if (!symbol_spec)
    symbol_spec = "symbol";

  token_string_len = 56;
  token_string = g_new (xchar_t, token_string_len + 1);
  expected_string_len = 64;
  expected_string = g_new (xchar_t, expected_string_len + 1);
  print_unexp = TRUE;

  switch (scanner->token)
    {
    case G_TOKEN_EOF:
      _g_snprintf (token_string, token_string_len, "end of file");
      break;

    default:
      if (scanner->token >= 1 && scanner->token <= 255)
	{
	  if ((scanner->token >= ' ' && scanner->token <= '~') ||
	      strchr (scanner->config->cset_identifier_first, scanner->token) ||
	      strchr (scanner->config->cset_identifier_nth, scanner->token))
	    _g_snprintf (token_string, token_string_len, "character '%c'", scanner->token);
	  else
	    _g_snprintf (token_string, token_string_len, "character '\\%o'", scanner->token);
	  break;
	}
      else if (!scanner->config->symbol_2_token)
	{
	  _g_snprintf (token_string, token_string_len, "(unknown) token <%d>", scanner->token);
	  break;
	}
      G_GNUC_FALLTHROUGH;
    case G_TOKEN_SYMBOL:
      if (expected_token == G_TOKEN_SYMBOL ||
	  (scanner->config->symbol_2_token &&
	   expected_token > G_TOKEN_LAST))
	print_unexp = FALSE;
      if (symbol_name)
	_g_snprintf (token_string,
		     token_string_len,
		     "%s%s '%s'",
		     print_unexp ? "" : "invalid ",
		     symbol_spec,
		     symbol_name);
      else
	_g_snprintf (token_string,
		     token_string_len,
		     "%s%s",
		     print_unexp ? "" : "invalid ",
		     symbol_spec);
      break;

    case G_TOKEN_ERROR:
      print_unexp = FALSE;
      expected_token = G_TOKEN_NONE;
      switch (scanner->value.v_error)
	{
	case G_ERR_UNEXP_EOF:
	  _g_snprintf (token_string, token_string_len, "scanner: unexpected end of file");
	  break;

	case G_ERR_UNEXP_EOF_IN_STRING:
	  _g_snprintf (token_string, token_string_len, "scanner: unterminated string constant");
	  break;

	case G_ERR_UNEXP_EOF_IN_COMMENT:
	  _g_snprintf (token_string, token_string_len, "scanner: unterminated comment");
	  break;

	case G_ERR_NON_DIGIT_IN_CONST:
	  _g_snprintf (token_string, token_string_len, "scanner: non digit in constant");
	  break;

	case G_ERR_FLOAT_RADIX:
	  _g_snprintf (token_string, token_string_len, "scanner: invalid radix for floating constant");
	  break;

	case G_ERR_FLOAT_MALFORMED:
	  _g_snprintf (token_string, token_string_len, "scanner: malformed floating constant");
	  break;

	case G_ERR_DIGIT_RADIX:
	  _g_snprintf (token_string, token_string_len, "scanner: digit is beyond radix");
	  break;

	case G_ERR_UNKNOWN:
	default:
	  _g_snprintf (token_string, token_string_len, "scanner: unknown error");
	  break;
	}
      break;

    case G_TOKEN_CHAR:
      _g_snprintf (token_string, token_string_len, "character '%c'", scanner->value.v_char);
      break;

    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
      if (expected_token == G_TOKEN_IDENTIFIER ||
	  expected_token == G_TOKEN_IDENTIFIER_NULL)
	print_unexp = FALSE;
      _g_snprintf (token_string,
		  token_string_len,
		  "%s%s '%s'",
		  print_unexp ? "" : "invalid ",
		  identifier_spec,
		  scanner->token == G_TOKEN_IDENTIFIER ? scanner->value.v_string : "null");
      break;

    case G_TOKEN_BINARY:
    case G_TOKEN_OCTAL:
    case G_TOKEN_INT:
    case G_TOKEN_HEX:
      if (scanner->config->store_int64)
	_g_snprintf (token_string, token_string_len, "number '%" G_GUINT64_FORMAT "'", scanner->value.v_int64);
      else
	_g_snprintf (token_string, token_string_len, "number '%lu'", scanner->value.v_int);
      break;

    case G_TOKEN_FLOAT:
      _g_snprintf (token_string, token_string_len, "number '%.3f'", scanner->value.v_float);
      break;

    case G_TOKEN_STRING:
      if (expected_token == G_TOKEN_STRING)
	print_unexp = FALSE;
      _g_snprintf (token_string,
		   token_string_len,
		   "%s%sstring constant \"%s\"",
		   print_unexp ? "" : "invalid ",
		   scanner->value.v_string[0] == 0 ? "empty " : "",
		   scanner->value.v_string);
      token_string[token_string_len - 2] = '"';
      token_string[token_string_len - 1] = 0;
      break;

    case G_TOKEN_COMMENT_SINGLE:
    case G_TOKEN_COMMENT_MULTI:
      _g_snprintf (token_string, token_string_len, "comment");
      break;

    case G_TOKEN_NONE:
      /* somehow the user's parsing code is screwed, there isn't much
       * we can do about it.
       * Note, a common case to trigger this is
       * g_scanner_peek_next_token(); g_scanner_unexp_token();
       * without an intermediate g_scanner_get_next_token().
       */
      g_assert_not_reached ();
      break;
    }


  switch (expected_token)
    {
      xboolean_t need_valid;
      xchar_t *tstring;
    case G_TOKEN_EOF:
      _g_snprintf (expected_string, expected_string_len, "end of file");
      break;
    default:
      if (expected_token >= 1 && expected_token <= 255)
	{
	  if ((expected_token >= ' ' && expected_token <= '~') ||
	      strchr (scanner->config->cset_identifier_first, expected_token) ||
	      strchr (scanner->config->cset_identifier_nth, expected_token))
	    _g_snprintf (expected_string, expected_string_len, "character '%c'", expected_token);
	  else
	    _g_snprintf (expected_string, expected_string_len, "character '\\%o'", expected_token);
	  break;
	}
      else if (!scanner->config->symbol_2_token)
	{
	  _g_snprintf (expected_string, expected_string_len, "(unknown) token <%d>", expected_token);
	  break;
	}
      G_GNUC_FALLTHROUGH;
    case G_TOKEN_SYMBOL:
      need_valid = (scanner->token == G_TOKEN_SYMBOL ||
		    (scanner->config->symbol_2_token &&
		     scanner->token > G_TOKEN_LAST));
      _g_snprintf (expected_string,
		   expected_string_len,
		   "%s%s",
		   need_valid ? "valid " : "",
		   symbol_spec);
      /* FIXME: should we attempt to look up the symbol_name for symbol_2_token? */
      break;
    case G_TOKEN_CHAR:
      _g_snprintf (expected_string, expected_string_len, "%scharacter",
		   scanner->token == G_TOKEN_CHAR ? "valid " : "");
      break;
    case G_TOKEN_BINARY:
      tstring = "binary";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_OCTAL:
      tstring = "octal";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_INT:
      tstring = "integer";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_HEX:
      tstring = "hexadecimal";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_FLOAT:
      tstring = "float";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_STRING:
      _g_snprintf (expected_string,
		   expected_string_len,
		   "%sstring constant",
		   scanner->token == G_TOKEN_STRING ? "valid " : "");
      break;
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
      need_valid = (scanner->token == G_TOKEN_IDENTIFIER_NULL ||
		    scanner->token == G_TOKEN_IDENTIFIER);
      _g_snprintf (expected_string,
		   expected_string_len,
		   "%s%s",
		   need_valid ? "valid " : "",
		   identifier_spec);
      break;
    case G_TOKEN_COMMENT_SINGLE:
      tstring = "single-line";
      _g_snprintf (expected_string, expected_string_len, "%scomment (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_COMMENT_MULTI:
      tstring = "multi-line";
      _g_snprintf (expected_string, expected_string_len, "%scomment (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_NONE:
    case G_TOKEN_ERROR:
      /* this is handled upon printout */
      break;
    }

  if (message && message[0] != 0)
    message_prefix = " - ";
  else
    {
      message_prefix = "";
      message = "";
    }
  if (expected_token == G_TOKEN_ERROR)
    {
      msg_handler (scanner,
		   "failure around %s%s%s",
		   token_string,
		   message_prefix,
		   message);
    }
  else if (expected_token == G_TOKEN_NONE)
    {
      if (print_unexp)
	msg_handler (scanner,
		     "unexpected %s%s%s",
		     token_string,
		     message_prefix,
		     message);
      else
	msg_handler (scanner,
		     "%s%s%s",
		     token_string,
		     message_prefix,
		     message);
    }
  else
    {
      if (print_unexp)
	msg_handler (scanner,
		     "unexpected %s, expected %s%s%s",
		     token_string,
		     expected_string,
		     message_prefix,
		     message);
      else
	msg_handler (scanner,
		     "%s, expected %s%s%s",
		     token_string,
		     expected_string,
		     message_prefix,
		     message);
    }

  g_free (token_string);
  g_free (expected_string);
}

static void
g_scanner_get_token_i (xscanner_t	*scanner,
		       GTokenType	*token_p,
		       GTokenValue	*value_p,
		       xuint_t		*line_p,
		       xuint_t		*position_p)
{
  do
    {
      g_scanner_free_value (token_p, value_p);
      g_scanner_get_token_ll (scanner, token_p, value_p, line_p, position_p);
    }
  while (((*token_p > 0 && *token_p < 256) &&
	  strchr (scanner->config->cset_skip_characters, *token_p)) ||
	 (*token_p == G_TOKEN_CHAR &&
	  strchr (scanner->config->cset_skip_characters, value_p->v_char)) ||
	 (*token_p == G_TOKEN_COMMENT_MULTI &&
	  scanner->config->skip_comment_multi) ||
	 (*token_p == G_TOKEN_COMMENT_SINGLE &&
	  scanner->config->skip_comment_single));

  switch (*token_p)
    {
    case G_TOKEN_IDENTIFIER:
      if (scanner->config->identifier_2_string)
	*token_p = G_TOKEN_STRING;
      break;

    case G_TOKEN_SYMBOL:
      if (scanner->config->symbol_2_token)
        *token_p = (GTokenType) ((size_t) value_p->v_symbol);
      break;

    case G_TOKEN_BINARY:
    case G_TOKEN_OCTAL:
    case G_TOKEN_HEX:
      if (scanner->config->numbers_2_int)
	*token_p = G_TOKEN_INT;
      break;

    default:
      break;
    }

  if (*token_p == G_TOKEN_INT &&
      scanner->config->int_2_float)
    {
      *token_p = G_TOKEN_FLOAT;

      /* Have to assign through a temporary variable to avoid undefined behaviour
       * by copying between potentially-overlapping union members. */
      if (scanner->config->store_int64)
        {
          sint64_t temp = value_p->v_int64;
          value_p->v_float = temp;
        }
      else
        {
          xint_t temp = value_p->v_int;
          value_p->v_float = temp;
        }
    }

  errno = 0;
}

static void
g_scanner_get_token_ll	(xscanner_t	*scanner,
			 GTokenType	*token_p,
			 GTokenValue	*value_p,
			 xuint_t		*line_p,
			 xuint_t		*position_p)
{
  GScannerConfig *config;
  GTokenType	   token;
  xboolean_t	   in_comment_multi;
  xboolean_t	   in_comment_single;
  xboolean_t	   in_string_sq;
  xboolean_t	   in_string_dq;
  xstring_t	  *xstring;
  GTokenValue	   value;
  xuchar_t	   ch;

  config = scanner->config;
  (*value_p).v_int64 = 0;

  if ((scanner->text >= scanner->text_end && scanner->input_fd < 0) ||
      scanner->token == G_TOKEN_EOF)
    {
      *token_p = G_TOKEN_EOF;
      return;
    }

  in_comment_multi = FALSE;
  in_comment_single = FALSE;
  in_string_sq = FALSE;
  in_string_dq = FALSE;
  xstring = NULL;

  do /* while (ch != 0) */
    {
      xboolean_t dotted_float = FALSE;

      ch = g_scanner_get_char (scanner, line_p, position_p);

      value.v_int64 = 0;
      token = G_TOKEN_NONE;

      /* this is *evil*, but needed ;(
       * we first check for identifier first character, because	 it
       * might interfere with other key chars like slashes or numbers
       */
      if (config->scan_identifier &&
	  ch && strchr (config->cset_identifier_first, ch))
	goto identifier_precedence;

      switch (ch)
	{
	case 0:
	  token = G_TOKEN_EOF;
	  (*position_p)++;
	  /* ch = 0; */
	  break;

	case '/':
	  if (!config->scan_comment_multi ||
	      g_scanner_peek_next_char (scanner) != '*')
	    goto default_case;
	  g_scanner_get_char (scanner, line_p, position_p);
	  token = G_TOKEN_COMMENT_MULTI;
	  in_comment_multi = TRUE;
	  xstring = xstring_new (NULL);
	  while ((ch = g_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '*' && g_scanner_peek_next_char (scanner) == '/')
		{
		  g_scanner_get_char (scanner, line_p, position_p);
		  in_comment_multi = FALSE;
		  break;
		}
	      else
		xstring = xstring_append_c (xstring, ch);
	    }
	  ch = 0;
	  break;

	case '\'':
	  if (!config->scan_string_sq)
	    goto default_case;
	  token = G_TOKEN_STRING;
	  in_string_sq = TRUE;
	  xstring = xstring_new (NULL);
	  while ((ch = g_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '\'')
		{
		  in_string_sq = FALSE;
		  break;
		}
	      else
		xstring = xstring_append_c (xstring, ch);
	    }
	  ch = 0;
	  break;

	case '"':
	  if (!config->scan_string_dq)
	    goto default_case;
	  token = G_TOKEN_STRING;
	  in_string_dq = TRUE;
	  xstring = xstring_new (NULL);
	  while ((ch = g_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '"')
		{
		  in_string_dq = FALSE;
		  break;
		}
	      else
		{
		  if (ch == '\\')
		    {
		      ch = g_scanner_get_char (scanner, line_p, position_p);
		      switch (ch)
			{
			  xuint_t	i;
			  xuint_t	fchar;

			case 0:
			  break;

			case '\\':
			  xstring = xstring_append_c (xstring, '\\');
			  break;

			case 'n':
			  xstring = xstring_append_c (xstring, '\n');
			  break;

			case 't':
			  xstring = xstring_append_c (xstring, '\t');
			  break;

			case 'r':
			  xstring = xstring_append_c (xstring, '\r');
			  break;

			case 'b':
			  xstring = xstring_append_c (xstring, '\b');
			  break;

			case 'f':
			  xstring = xstring_append_c (xstring, '\f');
			  break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			  i = ch - '0';
			  fchar = g_scanner_peek_next_char (scanner);
			  if (fchar >= '0' && fchar <= '7')
			    {
			      ch = g_scanner_get_char (scanner, line_p, position_p);
			      i = i * 8 + ch - '0';
			      fchar = g_scanner_peek_next_char (scanner);
			      if (fchar >= '0' && fchar <= '7')
				{
				  ch = g_scanner_get_char (scanner, line_p, position_p);
				  i = i * 8 + ch - '0';
				}
			    }
			  xstring = xstring_append_c (xstring, i);
			  break;

			default:
			  xstring = xstring_append_c (xstring, ch);
			  break;
			}
		    }
		  else
		    xstring = xstring_append_c (xstring, ch);
		}
	    }
	  ch = 0;
	  break;

	case '.':
	  if (!config->scan_float)
	    goto default_case;
	  token = G_TOKEN_FLOAT;
	  dotted_float = TRUE;
	  ch = g_scanner_get_char (scanner, line_p, position_p);
	  goto number_parsing;

	case '$':
	  if (!config->scan_hex_dollar)
	    goto default_case;
	  token = G_TOKEN_HEX;
	  ch = g_scanner_get_char (scanner, line_p, position_p);
	  goto number_parsing;

	case '0':
	  if (config->scan_octal)
	    token = G_TOKEN_OCTAL;
	  else
	    token = G_TOKEN_INT;
	  ch = g_scanner_peek_next_char (scanner);
	  if (config->scan_hex && (ch == 'x' || ch == 'X'))
	    {
	      token = G_TOKEN_HEX;
	      g_scanner_get_char (scanner, line_p, position_p);
	      ch = g_scanner_get_char (scanner, line_p, position_p);
	      if (ch == 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_UNEXP_EOF;
		  (*position_p)++;
		  break;
		}
	      if (g_scanner_char_2_num (ch, 16) < 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_DIGIT_RADIX;
		  ch = 0;
		  break;
		}
	    }
	  else if (config->scan_binary && (ch == 'b' || ch == 'B'))
	    {
	      token = G_TOKEN_BINARY;
	      g_scanner_get_char (scanner, line_p, position_p);
	      ch = g_scanner_get_char (scanner, line_p, position_p);
	      if (ch == 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_UNEXP_EOF;
		  (*position_p)++;
		  break;
		}
	      if (g_scanner_char_2_num (ch, 10) < 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
		  ch = 0;
		  break;
		}
	    }
	  else
	    ch = '0';
          G_GNUC_FALLTHROUGH;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	number_parsing:
	{
          xboolean_t in_number = TRUE;
	  xchar_t *endptr;

	  if (token == G_TOKEN_NONE)
	    token = G_TOKEN_INT;

	  xstring = xstring_new (dotted_float ? "0." : "");
	  xstring = xstring_append_c (xstring, ch);

	  do /* while (in_number) */
	    {
	      xboolean_t is_E;

	      is_E = token == G_TOKEN_FLOAT && (ch == 'e' || ch == 'E');

	      ch = g_scanner_peek_next_char (scanner);

	      if (g_scanner_char_2_num (ch, 36) >= 0 ||
		  (config->scan_float && ch == '.') ||
		  (is_E && (ch == '+' || ch == '-')))
		{
		  ch = g_scanner_get_char (scanner, line_p, position_p);

		  switch (ch)
		    {
		    case '.':
		      if (token != G_TOKEN_INT && token != G_TOKEN_OCTAL)
			{
			  value.v_error = token == G_TOKEN_FLOAT ? G_ERR_FLOAT_MALFORMED : G_ERR_FLOAT_RADIX;
			  token = G_TOKEN_ERROR;
			  in_number = FALSE;
			}
		      else
			{
			  token = G_TOKEN_FLOAT;
			  xstring = xstring_append_c (xstring, ch);
			}
		      break;

		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
		      xstring = xstring_append_c (xstring, ch);
		      break;

		    case '-':
		    case '+':
		      if (token != G_TOKEN_FLOAT)
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			xstring = xstring_append_c (xstring, ch);
		      break;

		    case 'e':
		    case 'E':
		      if ((token != G_TOKEN_HEX && !config->scan_float) ||
			  (token != G_TOKEN_HEX &&
			   token != G_TOKEN_OCTAL &&
			   token != G_TOKEN_FLOAT &&
			   token != G_TOKEN_INT))
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			{
			  if (token != G_TOKEN_HEX)
			    token = G_TOKEN_FLOAT;
			  xstring = xstring_append_c (xstring, ch);
			}
		      break;

		    default:
		      if (token != G_TOKEN_HEX)
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			xstring = xstring_append_c (xstring, ch);
		      break;
		    }
		}
	      else
		in_number = FALSE;
	    }
	  while (in_number);

	  endptr = NULL;
	  if (token == G_TOKEN_FLOAT)
	    value.v_float = xstrtod (xstring->str, &endptr);
	  else
	    {
	      xuint64_t ui64 = 0;
	      switch (token)
		{
		case G_TOKEN_BINARY:
		  ui64 = g_ascii_strtoull (xstring->str, &endptr, 2);
		  break;
		case G_TOKEN_OCTAL:
		  ui64 = g_ascii_strtoull (xstring->str, &endptr, 8);
		  break;
		case G_TOKEN_INT:
		  ui64 = g_ascii_strtoull (xstring->str, &endptr, 10);
		  break;
		case G_TOKEN_HEX:
		  ui64 = g_ascii_strtoull (xstring->str, &endptr, 16);
		  break;
		default: ;
		}
	      if (scanner->config->store_int64)
		value.v_int64 = ui64;
	      else
		value.v_int = ui64;
	    }
	  if (endptr && *endptr)
	    {
	      token = G_TOKEN_ERROR;
	      if (*endptr == 'e' || *endptr == 'E')
		value.v_error = G_ERR_NON_DIGIT_IN_CONST;
	      else
		value.v_error = G_ERR_DIGIT_RADIX;
	    }
	  xstring_free (xstring, TRUE);
	  xstring = NULL;
	  ch = 0;
	} /* number_parsing:... */
	break;

	default:
	default_case:
	{
	  if (config->cpair_comment_single &&
	      ch == config->cpair_comment_single[0])
	    {
	      token = G_TOKEN_COMMENT_SINGLE;
	      in_comment_single = TRUE;
	      xstring = xstring_new (NULL);
	      ch = g_scanner_get_char (scanner, line_p, position_p);
	      while (ch != 0)
		{
		  if (ch == config->cpair_comment_single[1])
		    {
		      in_comment_single = FALSE;
		      ch = 0;
		      break;
		    }

		  xstring = xstring_append_c (xstring, ch);
		  ch = g_scanner_get_char (scanner, line_p, position_p);
		}
	      /* ignore a missing newline at EOF for single line comments */
	      if (in_comment_single &&
		  config->cpair_comment_single[1] == '\n')
		in_comment_single = FALSE;
	    }
	  else if (config->scan_identifier && ch &&
		   strchr (config->cset_identifier_first, ch))
	    {
	    identifier_precedence:

	      if (config->cset_identifier_nth && ch &&
		  strchr (config->cset_identifier_nth,
			  g_scanner_peek_next_char (scanner)))
		{
		  token = G_TOKEN_IDENTIFIER;
		  xstring = xstring_new (NULL);
		  xstring = xstring_append_c (xstring, ch);
		  do
		    {
		      ch = g_scanner_get_char (scanner, line_p, position_p);
		      xstring = xstring_append_c (xstring, ch);
		      ch = g_scanner_peek_next_char (scanner);
		    }
		  while (ch && strchr (config->cset_identifier_nth, ch));
		  ch = 0;
		}
	      else if (config->scan_identifier_1char)
		{
		  token = G_TOKEN_IDENTIFIER;
		  value.v_identifier = g_new0 (xchar_t, 2);
		  value.v_identifier[0] = ch;
		  ch = 0;
		}
	    }
	  if (ch)
	    {
	      if (config->char_2_token)
		token = ch;
	      else
		{
		  token = G_TOKEN_CHAR;
		  value.v_char = ch;
		}
	      ch = 0;
	    }
	} /* default_case:... */
	break;
	}
      xassert (ch == 0 && token != G_TOKEN_NONE); /* paranoid */
    }
  while (ch != 0);

  if (in_comment_multi || in_comment_single ||
      in_string_sq || in_string_dq)
    {
      token = G_TOKEN_ERROR;
      if (xstring)
	{
	  xstring_free (xstring, TRUE);
	  xstring = NULL;
	}
      (*position_p)++;
      if (in_comment_multi || in_comment_single)
	value.v_error = G_ERR_UNEXP_EOF_IN_COMMENT;
      else /* (in_string_sq || in_string_dq) */
	value.v_error = G_ERR_UNEXP_EOF_IN_STRING;
    }

  if (xstring)
    {
      value.v_string = xstring_free (xstring, FALSE);
      xstring = NULL;
    }

  if (token == G_TOKEN_IDENTIFIER)
    {
      if (config->scan_symbols)
	{
	  GScannerKey *key;
	  xuint_t scope_id;

	  scope_id = scanner->scope_id;
	  key = g_scanner_lookup_internal (scanner, scope_id, value.v_identifier);
	  if (!key && scope_id && scanner->config->scope_0_fallback)
	    key = g_scanner_lookup_internal (scanner, 0, value.v_identifier);

	  if (key)
	    {
	      g_free (value.v_identifier);
	      token = G_TOKEN_SYMBOL;
	      value.v_symbol = key->value;
	    }
	}

      if (token == G_TOKEN_IDENTIFIER &&
	  config->scan_identifier_NULL &&
	  strlen (value.v_identifier) == 4)
	{
	  xchar_t *null_upper = "NULL";
	  xchar_t *null_lower = "null";

	  if (scanner->config->case_sensitive)
	    {
	      if (value.v_identifier[0] == null_upper[0] &&
		  value.v_identifier[1] == null_upper[1] &&
		  value.v_identifier[2] == null_upper[2] &&
		  value.v_identifier[3] == null_upper[3])
		token = G_TOKEN_IDENTIFIER_NULL;
	    }
	  else
	    {
	      if ((value.v_identifier[0] == null_upper[0] ||
		   value.v_identifier[0] == null_lower[0]) &&
		  (value.v_identifier[1] == null_upper[1] ||
		   value.v_identifier[1] == null_lower[1]) &&
		  (value.v_identifier[2] == null_upper[2] ||
		   value.v_identifier[2] == null_lower[2]) &&
		  (value.v_identifier[3] == null_upper[3] ||
		   value.v_identifier[3] == null_lower[3]))
		token = G_TOKEN_IDENTIFIER_NULL;
	    }
	}
    }

  *token_p = token;
  *value_p = value;
}
