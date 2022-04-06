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

#ifndef __G_SCANNER_H__
#define __G_SCANNER_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gdataset.h>
#include <glib/ghash.h>

G_BEGIN_DECLS

typedef struct _GScanner	xscanner_t;
typedef struct _GScannerConfig	GScannerConfig;
typedef union  _GTokenValue     GTokenValue;

typedef void		(*GScannerMsgFunc)	(xscanner_t      *scanner,
						 xchar_t	       *message,
						 xboolean_t	error);

/* xscanner_t: Flexible lexical scanner for general purpose.
 */

/* Character sets */
#define G_CSET_A_2_Z	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define G_CSET_a_2_z	"abcdefghijklmnopqrstuvwxyz"
#define G_CSET_DIGITS	"0123456789"
#define G_CSET_LATINC	"\300\301\302\303\304\305\306"\
			"\307\310\311\312\313\314\315\316\317\320"\
			"\321\322\323\324\325\326"\
			"\330\331\332\333\334\335\336"
#define G_CSET_LATINS	"\337\340\341\342\343\344\345\346"\
			"\347\350\351\352\353\354\355\356\357\360"\
			"\361\362\363\364\365\366"\
			"\370\371\372\373\374\375\376\377"

/* Error types */
typedef enum
{
  G_ERR_UNKNOWN,
  G_ERR_UNEXP_EOF,
  G_ERR_UNEXP_EOF_IN_STRING,
  G_ERR_UNEXP_EOF_IN_COMMENT,
  G_ERR_NON_DIGIT_IN_CONST,
  G_ERR_DIGIT_RADIX,
  G_ERR_FLOAT_RADIX,
  G_ERR_FLOAT_MALFORMED
} GErrorType;

/* Token types */
typedef enum
{
  G_TOKEN_EOF			=   0,

  G_TOKEN_LEFT_PAREN		= '(',
  G_TOKEN_RIGHT_PAREN		= ')',
  G_TOKEN_LEFT_CURLY		= '{',
  G_TOKEN_RIGHT_CURLY		= '}',
  G_TOKEN_LEFT_BRACE		= '[',
  G_TOKEN_RIGHT_BRACE		= ']',
  G_TOKEN_EQUAL_SIGN		= '=',
  G_TOKEN_COMMA			= ',',

  G_TOKEN_NONE			= 256,

  G_TOKEN_ERROR,

  G_TOKEN_CHAR,
  G_TOKEN_BINARY,
  G_TOKEN_OCTAL,
  G_TOKEN_INT,
  G_TOKEN_HEX,
  G_TOKEN_FLOAT,
  G_TOKEN_STRING,

  G_TOKEN_SYMBOL,
  G_TOKEN_IDENTIFIER,
  G_TOKEN_IDENTIFIER_NULL,

  G_TOKEN_COMMENT_SINGLE,
  G_TOKEN_COMMENT_MULTI,

  /*< private >*/
  G_TOKEN_LAST
} GTokenType;

union	_GTokenValue
{
  xpointer_t	v_symbol;
  xchar_t		*v_identifier;
  gulong	v_binary;
  gulong	v_octal;
  gulong	v_int;
  xuint64_t       v_int64;
  xdouble_t	v_float;
  gulong	v_hex;
  xchar_t		*v_string;
  xchar_t		*v_comment;
  guchar	v_char;
  xuint_t		v_error;
};

struct	_GScannerConfig
{
  /* Character sets
   */
  xchar_t		*cset_skip_characters;		/* default: " \t\n" */
  xchar_t		*cset_identifier_first;
  xchar_t		*cset_identifier_nth;
  xchar_t		*cpair_comment_single;		/* default: "#\n" */

  /* Should symbol lookup work case sensitive?
   */
  xuint_t		case_sensitive : 1;

  /* Boolean values to be adjusted "on the fly"
   * to configure scanning behaviour.
   */
  xuint_t		skip_comment_multi : 1;		/* C like comment */
  xuint_t		skip_comment_single : 1;	/* single line comment */
  xuint_t		scan_comment_multi : 1;		/* scan multi line comments? */
  xuint_t		scan_identifier : 1;
  xuint_t		scan_identifier_1char : 1;
  xuint_t		scan_identifier_NULL : 1;
  xuint_t		scan_symbols : 1;
  xuint_t		scan_binary : 1;
  xuint_t		scan_octal : 1;
  xuint_t		scan_float : 1;
  xuint_t		scan_hex : 1;			/* '0x0ff0' */
  xuint_t		scan_hex_dollar : 1;		/* '$0ff0' */
  xuint_t		scan_string_sq : 1;		/* string: 'anything' */
  xuint_t		scan_string_dq : 1;		/* string: "\\-escapes!\n" */
  xuint_t		numbers_2_int : 1;		/* bin, octal, hex => int */
  xuint_t		int_2_float : 1;		/* int => G_TOKEN_FLOAT? */
  xuint_t		identifier_2_string : 1;
  xuint_t		char_2_token : 1;		/* return G_TOKEN_CHAR? */
  xuint_t		symbol_2_token : 1;
  xuint_t		scope_0_fallback : 1;		/* try scope 0 on lookups? */
  xuint_t		store_int64 : 1; 		/* use value.v_int64 rather than v_int */

  /*< private >*/
  xuint_t		padding_dummy;
};

struct	_GScanner
{
  /* unused fields */
  xpointer_t		user_data;
  xuint_t			max_parse_errors;

  /* g_scanner_error() increments this field */
  xuint_t			parse_errors;

  /* name of input stream, featured by the default message handler */
  const xchar_t		*input_name;

  /* quarked data */
  GData			*qdata;

  /* link into the scanner configuration */
  GScannerConfig	*config;

  /* fields filled in after g_scanner_get_next_token() */
  GTokenType		token;
  GTokenValue		value;
  xuint_t			line;
  xuint_t			position;

  /* fields filled in after g_scanner_peek_next_token() */
  GTokenType		next_token;
  GTokenValue		next_value;
  xuint_t			next_line;
  xuint_t			next_position;

  /*< private >*/
  /* to be considered private */
  xhashtable_t		*symbol_table;
  xint_t			input_fd;
  const xchar_t		*text;
  const xchar_t		*text_end;
  xchar_t			*buffer;
  xuint_t			scope_id;

  /*< public >*/
  /* handler function for _warn and _error */
  GScannerMsgFunc	msg_handler;
};

XPL_AVAILABLE_IN_ALL
xscanner_t*	g_scanner_new			(const GScannerConfig *config_templ);
XPL_AVAILABLE_IN_ALL
void		g_scanner_destroy		(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
void		g_scanner_input_file		(xscanner_t	*scanner,
						 xint_t		input_fd);
XPL_AVAILABLE_IN_ALL
void		g_scanner_sync_file_offset	(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
void		g_scanner_input_text		(xscanner_t	*scanner,
						 const	xchar_t	*text,
						 xuint_t		text_len);
XPL_AVAILABLE_IN_ALL
GTokenType	g_scanner_get_next_token	(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
GTokenType	g_scanner_peek_next_token	(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
GTokenType	g_scanner_cur_token		(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
GTokenValue	g_scanner_cur_value		(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
xuint_t		g_scanner_cur_line		(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
xuint_t		g_scanner_cur_position		(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
xboolean_t	g_scanner_eof			(xscanner_t	*scanner);
XPL_AVAILABLE_IN_ALL
xuint_t		g_scanner_set_scope		(xscanner_t	*scanner,
						 xuint_t		 scope_id);
XPL_AVAILABLE_IN_ALL
void		g_scanner_scope_add_symbol	(xscanner_t	*scanner,
						 xuint_t		 scope_id,
						 const xchar_t	*symbol,
						 xpointer_t	value);
XPL_AVAILABLE_IN_ALL
void		g_scanner_scope_remove_symbol	(xscanner_t	*scanner,
						 xuint_t		 scope_id,
						 const xchar_t	*symbol);
XPL_AVAILABLE_IN_ALL
xpointer_t	g_scanner_scope_lookup_symbol	(xscanner_t	*scanner,
						 xuint_t		 scope_id,
						 const xchar_t	*symbol);
XPL_AVAILABLE_IN_ALL
void		g_scanner_scope_foreach_symbol	(xscanner_t	*scanner,
						 xuint_t		 scope_id,
						 GHFunc		 func,
						 xpointer_t	 user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t	g_scanner_lookup_symbol		(xscanner_t	*scanner,
						 const xchar_t	*symbol);
XPL_AVAILABLE_IN_ALL
void		g_scanner_unexp_token		(xscanner_t	*scanner,
						 GTokenType	expected_token,
						 const xchar_t	*identifier_spec,
						 const xchar_t	*symbol_spec,
						 const xchar_t	*symbol_name,
						 const xchar_t	*message,
						 xint_t		 is_error);
XPL_AVAILABLE_IN_ALL
void		g_scanner_error			(xscanner_t	*scanner,
						 const xchar_t	*format,
						 ...) G_GNUC_PRINTF (2,3);
XPL_AVAILABLE_IN_ALL
void		g_scanner_warn			(xscanner_t	*scanner,
						 const xchar_t	*format,
						 ...) G_GNUC_PRINTF (2,3);

/* keep downward source compatibility */
#define		g_scanner_add_symbol( scanner, symbol, value )	G_STMT_START { \
  g_scanner_scope_add_symbol ((scanner), 0, (symbol), (value)); \
} G_STMT_END XPL_DEPRECATED_MACRO_IN_2_26_FOR(g_scanner_scope_add_symbol)
#define		g_scanner_remove_symbol( scanner, symbol )	G_STMT_START { \
  g_scanner_scope_remove_symbol ((scanner), 0, (symbol)); \
} G_STMT_END XPL_DEPRECATED_MACRO_IN_2_26_FOR(g_scanner_scope_remove_symbol)
#define		g_scanner_foreach_symbol( scanner, func, data )	G_STMT_START { \
  g_scanner_scope_foreach_symbol ((scanner), 0, (func), (data)); \
} G_STMT_END XPL_DEPRECATED_MACRO_IN_2_26_FOR(g_scanner_scope_foreach_symbol)

/* The following two functions are deprecated and will be removed in
 * the next major release. They do no good. */
#define g_scanner_freeze_symbol_table(scanner) ((void)0) XPL_DEPRECATED_MACRO_IN_2_26
#define g_scanner_thaw_symbol_table(scanner) ((void)0) XPL_DEPRECATED_MACRO_IN_2_26

G_END_DECLS

#endif /* __G_SCANNER_H__ */
