/* gmarkup.h - Simple XML-like string parser/writer
 *
 *  Copyright 2000 Red Hat, Inc.
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

#ifndef __G_MARKUP_H__
#define __G_MARKUP_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <stdarg.h>

#include <glib/gerror.h>
#include <glib/gslist.h>

G_BEGIN_DECLS

/**
 * GMarkupError:
 * @G_MARKUP_ERROR_BAD_UTF8: text being parsed was not valid UTF-8
 * @G_MARKUP_ERROR_EMPTY: document contained nothing, or only whitespace
 * @G_MARKUP_ERROR_PARSE: document was ill-formed
 * @G_MARKUP_ERROR_UNKNOWN_ELEMENT: error should be set by #GMarkupParser
 *     functions; element wasn't known
 * @G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE: error should be set by #GMarkupParser
 *     functions; attribute wasn't known
 * @G_MARKUP_ERROR_INVALID_CONTENT: error should be set by #GMarkupParser
 *     functions; content was invalid
 * @G_MARKUP_ERROR_MISSING_ATTRIBUTE: error should be set by #GMarkupParser
 *     functions; a required attribute was missing
 *
 * Error codes returned by markup parsing.
 */
typedef enum
{
  G_MARKUP_ERROR_BAD_UTF8,
  G_MARKUP_ERROR_EMPTY,
  G_MARKUP_ERROR_PARSE,
  /* The following are primarily intended for specific GMarkupParser
   * implementations to set.
   */
  G_MARKUP_ERROR_UNKNOWN_ELEMENT,
  G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
  G_MARKUP_ERROR_INVALID_CONTENT,
  G_MARKUP_ERROR_MISSING_ATTRIBUTE
} GMarkupError;

/**
 * G_MARKUP_ERROR:
 *
 * Error domain for markup parsing.
 * Errors in this domain will be from the #GMarkupError enumeration.
 * See #xerror_t for information on error domains.
 */
#define G_MARKUP_ERROR g_markup_error_quark ()

XPL_AVAILABLE_IN_ALL
GQuark g_markup_error_quark (void);

/**
 * GMarkupParseFlags:
 * @G_MARKUP_DO_NOT_USE_THIS_UNSUPPORTED_FLAG: flag you should not use
 * @G_MARKUP_TREAT_CDATA_AS_TEXT: When this flag is set, CDATA marked
 *     sections are not passed literally to the @passthrough function of
 *     the parser. Instead, the content of the section (without the
 *     `<![CDATA[` and `]]>`) is
 *     passed to the @text function. This flag was added in GLib 2.12
 * @G_MARKUP_PREFIX_ERROR_POSITION: Normally errors caught by GMarkup
 *     itself have line/column information prefixed to them to let the
 *     caller know the location of the error. When this flag is set the
 *     location information is also prefixed to errors generated by the
 *     #GMarkupParser implementation functions
 * @G_MARKUP_IGNORE_QUALIFIED: Ignore (don't report) qualified
 *     attributes and tags, along with their contents.  A qualified
 *     attribute or tag is one that contains ':' in its name (ie: is in
 *     another namespace).  Since: 2.40.
 *
 * Flags that affect the behaviour of the parser.
 */
typedef enum
{
  G_MARKUP_DO_NOT_USE_THIS_UNSUPPORTED_FLAG = 1 << 0,
  G_MARKUP_TREAT_CDATA_AS_TEXT              = 1 << 1,
  G_MARKUP_PREFIX_ERROR_POSITION            = 1 << 2,
  G_MARKUP_IGNORE_QUALIFIED                 = 1 << 3
} GMarkupParseFlags;

/**
 * GMarkupParseContext:
 *
 * A parse context is used to parse a stream of bytes that
 * you expect to contain marked-up text.
 *
 * See g_markup_parse_context_new(), #GMarkupParser, and so
 * on for more details.
 */
typedef struct _GMarkupParseContext GMarkupParseContext;
typedef struct _GMarkupParser GMarkupParser;

/**
 * GMarkupParser:
 * @start_element: Callback to invoke when the opening tag of an element
 *     is seen. The callback's @attribute_names and @attribute_values parameters
 *     are %NULL-terminated.
 * @end_element: Callback to invoke when the closing tag of an element
 *     is seen. Note that this is also called for empty tags like
 *     `<empty/>`.
 * @text: Callback to invoke when some text is seen (text is always
 *     inside an element). Note that the text of an element may be spread
 *     over multiple calls of this function. If the
 *     %G_MARKUP_TREAT_CDATA_AS_TEXT flag is set, this function is also
 *     called for the content of CDATA marked sections.
 * @passthrough: Callback to invoke for comments, processing instructions
 *     and doctype declarations; if you're re-writing the parsed document,
 *     write the passthrough text back out in the same position. If the
 *     %G_MARKUP_TREAT_CDATA_AS_TEXT flag is not set, this function is also
 *     called for CDATA marked sections.
 * @error: Callback to invoke when an error occurs.
 *
 * Any of the fields in #GMarkupParser can be %NULL, in which case they
 * will be ignored. Except for the @error function, any of these callbacks
 * can set an error; in particular the %G_MARKUP_ERROR_UNKNOWN_ELEMENT,
 * %G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, and %G_MARKUP_ERROR_INVALID_CONTENT
 * errors are intended to be set from these callbacks. If you set an error
 * from a callback, g_markup_parse_context_parse() will report that error
 * back to its caller.
 */
struct _GMarkupParser
{
  /* Called for open tags <foo bar="baz"> */
  void (*start_element)  (GMarkupParseContext *context,
                          const xchar_t         *element_name,
                          const xchar_t        **attribute_names,
                          const xchar_t        **attribute_values,
                          xpointer_t             user_data,
                          xerror_t             **error);

  /* Called for close tags </foo> */
  void (*end_element)    (GMarkupParseContext *context,
                          const xchar_t         *element_name,
                          xpointer_t             user_data,
                          xerror_t             **error);

  /* Called for character data */
  /* text is not nul-terminated */
  void (*text)           (GMarkupParseContext *context,
                          const xchar_t         *text,
                          xsize_t                text_len,
                          xpointer_t             user_data,
                          xerror_t             **error);

  /* Called for strings that should be re-saved verbatim in this same
   * position, but are not otherwise interpretable.  At the moment
   * this includes comments and processing instructions.
   */
  /* text is not nul-terminated. */
  void (*passthrough)    (GMarkupParseContext *context,
                          const xchar_t         *passthrough_text,
                          xsize_t                text_len,
                          xpointer_t             user_data,
                          xerror_t             **error);

  /* Called on error, including one set by other
   * methods in the vtable. The xerror_t should not be freed.
   */
  void (*error)          (GMarkupParseContext *context,
                          xerror_t              *error,
                          xpointer_t             user_data);
};

XPL_AVAILABLE_IN_ALL
GMarkupParseContext *g_markup_parse_context_new   (const GMarkupParser *parser,
                                                   GMarkupParseFlags    flags,
                                                   xpointer_t             user_data,
                                                   GDestroyNotify       user_data_dnotify);
XPL_AVAILABLE_IN_2_36
GMarkupParseContext *g_markup_parse_context_ref   (GMarkupParseContext *context);
XPL_AVAILABLE_IN_2_36
void                 g_markup_parse_context_unref (GMarkupParseContext *context);
XPL_AVAILABLE_IN_ALL
void                 g_markup_parse_context_free  (GMarkupParseContext *context);
XPL_AVAILABLE_IN_ALL
xboolean_t             g_markup_parse_context_parse (GMarkupParseContext *context,
                                                   const xchar_t         *text,
                                                   gssize               text_len,
                                                   xerror_t             **error);
XPL_AVAILABLE_IN_ALL
void                 g_markup_parse_context_push  (GMarkupParseContext *context,
                                                   const GMarkupParser *parser,
                                                   xpointer_t             user_data);
XPL_AVAILABLE_IN_ALL
xpointer_t             g_markup_parse_context_pop   (GMarkupParseContext *context);

XPL_AVAILABLE_IN_ALL
xboolean_t             g_markup_parse_context_end_parse (GMarkupParseContext *context,
                                                       xerror_t             **error);
XPL_AVAILABLE_IN_ALL
const xchar_t *        g_markup_parse_context_get_element (GMarkupParseContext *context);
XPL_AVAILABLE_IN_ALL
const GSList *       g_markup_parse_context_get_element_stack (GMarkupParseContext *context);

/* For user-constructed error messages, has no precise semantics */
XPL_AVAILABLE_IN_ALL
void                 g_markup_parse_context_get_position (GMarkupParseContext *context,
                                                          xint_t                *line_number,
                                                          xint_t                *char_number);
XPL_AVAILABLE_IN_ALL
xpointer_t             g_markup_parse_context_get_user_data (GMarkupParseContext *context);

/* useful when saving */
XPL_AVAILABLE_IN_ALL
xchar_t* g_markup_escape_text (const xchar_t *text,
                             gssize       length);

XPL_AVAILABLE_IN_ALL
xchar_t *g_markup_printf_escaped (const char *format,
				...) G_GNUC_PRINTF (1, 2);
XPL_AVAILABLE_IN_ALL
xchar_t *g_markup_vprintf_escaped (const char *format,
				 va_list     args) G_GNUC_PRINTF(1, 0);

typedef enum
{
  G_MARKUP_COLLECT_INVALID,
  G_MARKUP_COLLECT_STRING,
  G_MARKUP_COLLECT_STRDUP,
  G_MARKUP_COLLECT_BOOLEAN,
  G_MARKUP_COLLECT_TRISTATE,

  G_MARKUP_COLLECT_OPTIONAL = (1 << 16)
} GMarkupCollectType;


/* useful from start_element */
XPL_AVAILABLE_IN_ALL
xboolean_t   g_markup_collect_attributes (const xchar_t         *element_name,
                                        const xchar_t        **attribute_names,
                                        const xchar_t        **attribute_values,
                                        xerror_t             **error,
                                        GMarkupCollectType   first_type,
                                        const xchar_t         *first_attr,
                                        ...);

G_END_DECLS

#endif /* __G_MARKUP_H__ */
