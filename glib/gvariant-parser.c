/*
 * Copyright © 2009, 2010 Codethink Limited
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
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gerror.h"
#include "gquark.h"
#include "gstring.h"
#include "gstrfuncs.h"
#include "gtestutils.h"
#include "gvariant.h"
#include "gvariant-internal.h"
#include "gvarianttype.h"
#include "gslice.h"
#include "gthread.h"

/*
 * two-pass algorithm
 * designed by ryan lortie and william hua
 * designed in itb-229 and at ghazi's, 2009.
 */

/**
 * G_VARIANT_PARSE_ERROR:
 *
 * Error domain for xvariant_t text format parsing.  Specific error codes
 * are not currently defined for this domain.  See #xerror_t for
 * information on error domains.
 **/
/**
 * GVariantParseError:
 * @G_VARIANT_PARSE_ERROR_FAILED: generic error (unused)
 * @G_VARIANT_PARSE_ERROR_BASIC_TYPE_EXPECTED: a non-basic #xvariant_type_t was given where a basic type was expected
 * @G_VARIANT_PARSE_ERROR_CANNOT_INFER_TYPE: cannot infer the #xvariant_type_t
 * @G_VARIANT_PARSE_ERROR_DEFINITE_TYPE_EXPECTED: an indefinite #xvariant_type_t was given where a definite type was expected
 * @G_VARIANT_PARSE_ERROR_INPUT_NOT_AT_END: extra data after parsing finished
 * @G_VARIANT_PARSE_ERROR_INVALID_CHARACTER: invalid character in number or unicode escape
 * @G_VARIANT_PARSE_ERROR_INVALID_FORMAT_STRING: not a valid #xvariant_t format string
 * @G_VARIANT_PARSE_ERROR_INVALID_OBJECT_PATH: not a valid object path
 * @G_VARIANT_PARSE_ERROR_INVALID_SIGNATURE: not a valid type signature
 * @G_VARIANT_PARSE_ERROR_INVALID_TYPE_STRING: not a valid #xvariant_t type string
 * @G_VARIANT_PARSE_ERROR_NO_COMMON_TYPE: could not find a common type for array entries
 * @G_VARIANT_PARSE_ERROR_NUMBER_OUT_OF_RANGE: the numerical value is out of range of the given type
 * @G_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG: the numerical value is out of range for any type
 * @G_VARIANT_PARSE_ERROR_TYPE_ERROR: cannot parse as variant of the specified type
 * @G_VARIANT_PARSE_ERROR_UNEXPECTED_TOKEN: an unexpected token was encountered
 * @G_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD: an unknown keyword was encountered
 * @G_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT: unterminated string constant
 * @G_VARIANT_PARSE_ERROR_VALUE_EXPECTED: no value given
 * @G_VARIANT_PARSE_ERROR_RECURSION: variant was too deeply nested; #xvariant_t is only guaranteed to handle nesting up to 64 levels (Since: 2.64)
 *
 * Error codes returned by parsing text-format GVariants.
 **/
G_DEFINE_QUARK (g-variant-parse-error-quark, xvariant_parse_error)

/**
 * xvariant_parser_get_error_quark:
 *
 * Same as xvariant_error_quark().
 *
 * Deprecated: Use xvariant_parse_error_quark() instead.
 */
xquark
xvariant_parser_get_error_quark (void)
{
  return xvariant_parse_error_quark ();
}

typedef struct
{
  xint_t start, end;
} SourceRef;

G_GNUC_PRINTF(5, 0)
static void
parser_set_error_va (xerror_t      **error,
                     SourceRef    *location,
                     SourceRef    *other,
                     xint_t          code,
                     const xchar_t  *format,
                     va_list       ap)
{
  xstring_t *msg = xstring_new (NULL);

  if (location->start == location->end)
    xstring_append_printf (msg, "%d", location->start);
  else
    xstring_append_printf (msg, "%d-%d", location->start, location->end);

  if (other != NULL)
    {
      xassert (other->start != other->end);
      xstring_append_printf (msg, ",%d-%d", other->start, other->end);
    }
  xstring_append_c (msg, ':');

  xstring_append_vprintf (msg, format, ap);
  g_set_error_literal (error, G_VARIANT_PARSE_ERROR, code, msg->str);
  xstring_free (msg, TRUE);
}

G_GNUC_PRINTF(5, 6)
static void
parser_set_error (xerror_t      **error,
                  SourceRef    *location,
                  SourceRef    *other,
                  xint_t          code,
                  const xchar_t  *format,
                  ...)
{
  va_list ap;

  va_start (ap, format);
  parser_set_error_va (error, location, other, code, format, ap);
  va_end (ap);
}

typedef struct
{
  const xchar_t *start;
  const xchar_t *stream;
  const xchar_t *end;

  const xchar_t *this;
} TokenStream;


G_GNUC_PRINTF(5, 6)
static void
token_stream_set_error (TokenStream  *stream,
                        xerror_t      **error,
                        xboolean_t      this_token,
                        xint_t          code,
                        const xchar_t  *format,
                        ...)
{
  SourceRef ref;
  va_list ap;

  ref.start = stream->this - stream->start;

  if (this_token)
    ref.end = stream->stream - stream->start;
  else
    ref.end = ref.start;

  va_start (ap, format);
  parser_set_error_va (error, &ref, NULL, code, format, ap);
  va_end (ap);
}

static xboolean_t
token_stream_prepare (TokenStream *stream)
{
  xint_t brackets = 0;
  const xchar_t *end;

  if (stream->this != NULL)
    return TRUE;

  while (stream->stream != stream->end && g_ascii_isspace (*stream->stream))
    stream->stream++;

  if (stream->stream == stream->end || *stream->stream == '\0')
    {
      stream->this = stream->stream;
      return FALSE;
    }

  switch (stream->stream[0])
    {
    case '-': case '+': case '.': case '0': case '1': case '2':
    case '3': case '4': case '5': case '6': case '7': case '8':
    case '9':
      for (end = stream->stream; end != stream->end; end++)
        if (!g_ascii_isalnum (*end) &&
            *end != '-' && *end != '+' && *end != '.')
          break;
      break;

    case 'b':
      if (stream->stream + 1 != stream->end &&
          (stream->stream[1] == '\'' || stream->stream[1] == '"'))
        {
          for (end = stream->stream + 2; end != stream->end; end++)
            if (*end == stream->stream[1] || *end == '\0' ||
                (*end == '\\' && (++end == stream->end || *end == '\0')))
              break;

          if (end != stream->end && *end)
            end++;
          break;
        }

      G_GNUC_FALLTHROUGH;

    case 'a': /* 'b' */ case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
      for (end = stream->stream; end != stream->end; end++)
        if (!g_ascii_isalnum (*end))
          break;
      break;

    case '\'': case '"':
      for (end = stream->stream + 1; end != stream->end; end++)
        if (*end == stream->stream[0] || *end == '\0' ||
            (*end == '\\' && (++end == stream->end || *end == '\0')))
          break;

      if (end != stream->end && *end)
        end++;
      break;

    case '@': case '%':
      /* stop at the first space, comma, colon or unmatched bracket.
       * deals nicely with cases like (%i, %i) or {%i: %i}.
       * Also: ] and > are never in format strings.
       */
      for (end = stream->stream + 1;
           end != stream->end && *end != '\0' && *end != ',' &&
           *end != ':' && *end != '>' && *end != ']' && !g_ascii_isspace (*end);
           end++)

        if (*end == '(' || *end == '{')
          brackets++;

        else if ((*end == ')' || *end == '}') && !brackets--)
          break;

      break;

    default:
      end = stream->stream + 1;
      break;
    }

  stream->this = stream->stream;
  stream->stream = end;

  /* We must have at least one byte in a token. */
  xassert (stream->stream - stream->this >= 1);

  return TRUE;
}

static void
token_stream_next (TokenStream *stream)
{
  stream->this = NULL;
}

static xboolean_t
token_stream_peek (TokenStream *stream,
                   xchar_t        first_char)
{
  if (!token_stream_prepare (stream))
    return FALSE;

  return stream->stream - stream->this >= 1 &&
         stream->this[0] == first_char;
}

static xboolean_t
token_stream_peek2 (TokenStream *stream,
                    xchar_t        first_char,
                    xchar_t        second_char)
{
  if (!token_stream_prepare (stream))
    return FALSE;

  return stream->stream - stream->this >= 2 &&
         stream->this[0] == first_char &&
         stream->this[1] == second_char;
}

static xboolean_t
token_stream_is_keyword (TokenStream *stream)
{
  if (!token_stream_prepare (stream))
    return FALSE;

  return stream->stream - stream->this >= 2 &&
         g_ascii_isalpha (stream->this[0]) &&
         g_ascii_isalpha (stream->this[1]);
}

static xboolean_t
token_stream_is_numeric (TokenStream *stream)
{
  if (!token_stream_prepare (stream))
    return FALSE;

  return (stream->stream - stream->this >= 1 &&
          (g_ascii_isdigit (stream->this[0]) ||
           stream->this[0] == '-' ||
           stream->this[0] == '+' ||
           stream->this[0] == '.'));
}

static xboolean_t
token_stream_peek_string (TokenStream *stream,
                          const xchar_t *token)
{
  xint_t length = strlen (token);

  return token_stream_prepare (stream) &&
         stream->stream - stream->this == length &&
         memcmp (stream->this, token, length) == 0;
}

static xboolean_t
token_stream_consume (TokenStream *stream,
                      const xchar_t *token)
{
  if (!token_stream_peek_string (stream, token))
    return FALSE;

  token_stream_next (stream);
  return TRUE;
}

static xboolean_t
token_stream_require (TokenStream  *stream,
                      const xchar_t  *token,
                      const xchar_t  *purpose,
                      xerror_t      **error)
{

  if (!token_stream_consume (stream, token))
    {
      token_stream_set_error (stream, error, FALSE,
                              G_VARIANT_PARSE_ERROR_UNEXPECTED_TOKEN,
                              "expected '%s'%s", token, purpose);
      return FALSE;
    }

  return TRUE;
}

static void
token_stream_assert (TokenStream *stream,
                     const xchar_t *token)
{
  xboolean_t correct_token G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  correct_token = token_stream_consume (stream, token);
  xassert (correct_token);
}

static xchar_t *
token_stream_get (TokenStream *stream)
{
  xchar_t *result;

  if (!token_stream_prepare (stream))
    return NULL;

  result = xstrndup (stream->this, stream->stream - stream->this);

  return result;
}

static void
token_stream_start_ref (TokenStream *stream,
                        SourceRef   *ref)
{
  token_stream_prepare (stream);
  ref->start = stream->this - stream->start;
}

static void
token_stream_end_ref (TokenStream *stream,
                      SourceRef   *ref)
{
  ref->end = stream->stream - stream->start;
}

static void
pattern_copy (xchar_t       **out,
              const xchar_t **in)
{
  xint_t brackets = 0;

  while (**in == 'a' || **in == 'm' || **in == 'M')
    *(*out)++ = *(*in)++;

  do
    {
      if (**in == '(' || **in == '{')
        brackets++;

      else if (**in == ')' || **in == '}')
        brackets--;

      *(*out)++ = *(*in)++;
    }
  while (brackets);
}

/* Returns the most general pattern that is subpattern of left and subpattern
 * of right, or NULL if there is no such pattern. */
static xchar_t *
pattern_coalesce (const xchar_t *left,
                  const xchar_t *right)
{
  xchar_t *result;
  xchar_t *out;

  /* the length of the output is loosely bound by the sum of the input
   * lengths, not simply the greater of the two lengths.
   *
   *   (*(iii)) + ((iii)*) ((iii)(iii))
   *
   *      8     +    8    =  12
   */
  out = result = g_malloc (strlen (left) + strlen (right));

  while (*left && *right)
    {
      if (*left == *right)
        {
          *out++ = *left++;
          right++;
        }

      else
        {
          const xchar_t **one = &left, **the_other = &right;

         again:
          if (**one == '*' && **the_other != ')')
            {
              pattern_copy (&out, the_other);
              (*one)++;
            }

          else if (**one == 'M' && **the_other == 'm')
            {
              *out++ = *(*the_other)++;
            }

          else if (**one == 'M' && **the_other != 'm' && **the_other != '*')
            {
              (*one)++;
            }

          else if (**one == 'N' && strchr ("ynqiuxthd", **the_other))
            {
              *out++ = *(*the_other)++;
              (*one)++;
            }

          else if (**one == 'S' && strchr ("sog", **the_other))
            {
              *out++ = *(*the_other)++;
              (*one)++;
            }

          else if (one == &left)
            {
              one = &right, the_other = &left;
              goto again;
            }

          else
            break;
        }
    }

  if (*left || *right)
    {
      g_free (result);
      result = NULL;
    }
  else
    *out++ = '\0';

  return result;
}

typedef struct _AST AST;
typedef xchar_t *    (*get_pattern_func)    (AST                 *ast,
                                           xerror_t             **error);
typedef xvariant_t * (*get_value_func)      (AST                 *ast,
                                           const xvariant_type_t  *type,
                                           xerror_t             **error);
typedef xvariant_t * (*get_base_value_func) (AST                 *ast,
                                           const xvariant_type_t  *type,
                                           xerror_t             **error);
typedef void       (*free_func)           (AST                 *ast);

typedef struct
{
  xchar_t *    (* get_pattern)    (AST                 *ast,
                                 xerror_t             **error);
  xvariant_t * (* get_value)      (AST                 *ast,
                                 const xvariant_type_t  *type,
                                 xerror_t             **error);
  xvariant_t * (* get_base_value) (AST                 *ast,
                                 const xvariant_type_t  *type,
                                 xerror_t             **error);
  void       (* free)           (AST                 *ast);
} ASTClass;

struct _AST
{
  const ASTClass *class;
  SourceRef source_ref;
};

static xchar_t *
ast_get_pattern (AST     *ast,
                 xerror_t **error)
{
  return ast->class->get_pattern (ast, error);
}

static xvariant_t *
ast_get_value (AST                 *ast,
               const xvariant_type_t  *type,
               xerror_t             **error)
{
  return ast->class->get_value (ast, type, error);
}

static void
ast_free (AST *ast)
{
  ast->class->free (ast);
}

G_GNUC_PRINTF(5, 6)
static void
ast_set_error (AST          *ast,
               xerror_t      **error,
               AST          *other_ast,
               xint_t          code,
               const xchar_t  *format,
               ...)
{
  va_list ap;

  va_start (ap, format);
  parser_set_error_va (error, &ast->source_ref,
                       other_ast ? & other_ast->source_ref : NULL,
                       code,
                       format, ap);
  va_end (ap);
}

static xvariant_t *
ast_type_error (AST                 *ast,
                const xvariant_type_t  *type,
                xerror_t             **error)
{
  xchar_t *typestr;

  typestr = xvariant_type_dup_string (type);
  ast_set_error (ast, error, NULL,
                 G_VARIANT_PARSE_ERROR_TYPE_ERROR,
                 "can not parse as value of type '%s'",
                 typestr);
  g_free (typestr);

  return NULL;
}

static xvariant_t *
ast_resolve (AST     *ast,
             xerror_t **error)
{
  xvariant_t *value;
  xchar_t *pattern;
  xint_t i, j = 0;

  pattern = ast_get_pattern (ast, error);

  if (pattern == NULL)
    return NULL;

  /* choose reasonable defaults
   *
   *   1) favour non-maybe values where possible
   *   2) default type for strings is 's'
   *   3) default type for integers is 'i'
   */
  for (i = 0; pattern[i]; i++)
    switch (pattern[i])
      {
      case '*':
        ast_set_error (ast, error, NULL,
                       G_VARIANT_PARSE_ERROR_CANNOT_INFER_TYPE,
                       "unable to infer type");
        g_free (pattern);
        return NULL;

      case 'M':
        break;

      case 'S':
        pattern[j++] = 's';
        break;

      case 'N':
        pattern[j++] = 'i';
        break;

      default:
        pattern[j++] = pattern[i];
        break;
      }
  pattern[j++] = '\0';

  value = ast_get_value (ast, G_VARIANT_TYPE (pattern), error);
  g_free (pattern);

  return value;
}


static AST *parse (TokenStream  *stream,
                   xuint_t         max_depth,
                   va_list      *app,
                   xerror_t      **error);

static void
ast_array_append (AST  ***array,
                  xint_t   *n_items,
                  AST    *ast)
{
  if ((*n_items & (*n_items - 1)) == 0)
    *array = g_renew (AST *, *array, *n_items ? 2 ** n_items : 1);

  (*array)[(*n_items)++] = ast;
}

static void
ast_array_free (AST  **array,
                xint_t   n_items)
{
  xint_t i;

  for (i = 0; i < n_items; i++)
    ast_free (array[i]);
  g_free (array);
}

static xchar_t *
ast_array_get_pattern (AST    **array,
                       xint_t     n_items,
                       xerror_t **error)
{
  xchar_t *pattern;
  xint_t i;

  /* Find the pattern which applies to all children in the array, by l-folding a
   * coalesce operation.
   */
  pattern = ast_get_pattern (array[0], error);

  if (pattern == NULL)
    return NULL;

  for (i = 1; i < n_items; i++)
    {
      xchar_t *tmp, *merged;

      tmp = ast_get_pattern (array[i], error);

      if (tmp == NULL)
        {
          g_free (pattern);
          return NULL;
        }

      merged = pattern_coalesce (pattern, tmp);
      g_free (pattern);
      pattern = merged;

      if (merged == NULL)
        /* set coalescence implies pairwise coalescence (i think).
         * we should therefore be able to trace the failure to a single
         * pair of values.
         */
        {
          int j = 0;

          while (TRUE)
            {
              xchar_t *tmp2;
              xchar_t *m;

              /* if 'j' reaches 'i' then we didn't find the pair that failed
               * to coalesce. This shouldn't happen (see above), but just in
               * case report an error:
               */
              if (j >= i)
                {
                  ast_set_error (array[i], error, NULL,
                                 G_VARIANT_PARSE_ERROR_NO_COMMON_TYPE,
                                 "unable to find a common type");
                  g_free (tmp);
                  return NULL;
                }

              tmp2 = ast_get_pattern (array[j], NULL);
              xassert (tmp2 != NULL);

              m = pattern_coalesce (tmp, tmp2);
              g_free (tmp2);
              g_free (m);

              if (m == NULL)
                {
                  /* we found a conflict between 'i' and 'j'.
                   *
                   * report the error.  note: 'j' is first.
                   */
                  ast_set_error (array[j], error, array[i],
                                 G_VARIANT_PARSE_ERROR_NO_COMMON_TYPE,
                                 "unable to find a common type");
                  g_free (tmp);
                  return NULL;
                }

              j++;
            }

        }

      g_free (tmp);
    }

  return pattern;
}

typedef struct
{
  AST ast;

  AST *child;
} Maybe;

static xchar_t *
maybe_get_pattern (AST     *ast,
                   xerror_t **error)
{
  Maybe *maybe = (Maybe *) ast;

  if (maybe->child != NULL)
    {
      xchar_t *child_pattern;
      xchar_t *pattern;

      child_pattern = ast_get_pattern (maybe->child, error);

      if (child_pattern == NULL)
        return NULL;

      pattern = xstrdup_printf ("m%s", child_pattern);
      g_free (child_pattern);

      return pattern;
    }

  return xstrdup ("m*");
}

static xvariant_t *
maybe_get_value (AST                 *ast,
                 const xvariant_type_t  *type,
                 xerror_t             **error)
{
  Maybe *maybe = (Maybe *) ast;
  xvariant_t *value;

  if (!xvariant_type_is_maybe (type))
    return ast_type_error (ast, type, error);

  type = xvariant_type_element (type);

  if (maybe->child)
    {
      value = ast_get_value (maybe->child, type, error);

      if (value == NULL)
        return NULL;
    }
  else
    value = NULL;

  return xvariant_new_maybe (type, value);
}

static void
maybe_free (AST *ast)
{
  Maybe *maybe = (Maybe *) ast;

  if (maybe->child != NULL)
    ast_free (maybe->child);

  g_slice_free (Maybe, maybe);
}

static AST *
maybe_parse (TokenStream  *stream,
             xuint_t         max_depth,
             va_list      *app,
             xerror_t      **error)
{
  static const ASTClass maybe_class = {
    maybe_get_pattern,
    maybe_get_value, NULL,
    maybe_free
  };
  AST *child = NULL;
  Maybe *maybe;

  if (token_stream_consume (stream, "just"))
    {
      child = parse (stream, max_depth - 1, app, error);
      if (child == NULL)
        return NULL;
    }

  else if (!token_stream_consume (stream, "nothing"))
    {
      token_stream_set_error (stream, error, TRUE,
                              G_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD,
                              "unknown keyword");
      return NULL;
    }

  maybe = g_slice_new (Maybe);
  maybe->ast.class = &maybe_class;
  maybe->child = child;

  return (AST *) maybe;
}

static xvariant_t *
maybe_wrapper (AST                 *ast,
               const xvariant_type_t  *type,
               xerror_t             **error)
{
  const xvariant_type_t *t;
  xvariant_t *value;
  int depth;

  for (depth = 0, t = type;
       xvariant_type_is_maybe (t);
       depth++, t = xvariant_type_element (t));

  value = ast->class->get_base_value (ast, t, error);

  if (value == NULL)
    return NULL;

  while (depth--)
    value = xvariant_new_maybe (NULL, value);

  return value;
}

typedef struct
{
  AST ast;

  AST **children;
  xint_t n_children;
} Array;

static xchar_t *
array_get_pattern (AST     *ast,
                   xerror_t **error)
{
  Array *array = (Array *) ast;
  xchar_t *pattern;
  xchar_t *result;

  if (array->n_children == 0)
    return xstrdup ("Ma*");

  pattern = ast_array_get_pattern (array->children, array->n_children, error);

  if (pattern == NULL)
    return NULL;

  result = xstrdup_printf ("Ma%s", pattern);
  g_free (pattern);

  return result;
}

static xvariant_t *
array_get_value (AST                 *ast,
                 const xvariant_type_t  *type,
                 xerror_t             **error)
{
  Array *array = (Array *) ast;
  const xvariant_type_t *childtype;
  xvariant_builder_t builder;
  xint_t i;

  if (!xvariant_type_is_array (type))
    return ast_type_error (ast, type, error);

  xvariant_builder_init (&builder, type);
  childtype = xvariant_type_element (type);

  for (i = 0; i < array->n_children; i++)
    {
      xvariant_t *child;

      if (!(child = ast_get_value (array->children[i], childtype, error)))
        {
          xvariant_builder_clear (&builder);
          return NULL;
        }

      xvariant_builder_add_value (&builder, child);
    }

  return xvariant_builder_end (&builder);
}

static void
array_free (AST *ast)
{
  Array *array = (Array *) ast;

  ast_array_free (array->children, array->n_children);
  g_slice_free (Array, array);
}

static AST *
array_parse (TokenStream  *stream,
             xuint_t         max_depth,
             va_list      *app,
             xerror_t      **error)
{
  static const ASTClass array_class = {
    array_get_pattern,
    maybe_wrapper, array_get_value,
    array_free
  };
  xboolean_t need_comma = FALSE;
  Array *array;

  array = g_slice_new (Array);
  array->ast.class = &array_class;
  array->children = NULL;
  array->n_children = 0;

  token_stream_assert (stream, "[");
  while (!token_stream_consume (stream, "]"))
    {
      AST *child;

      if (need_comma &&
          !token_stream_require (stream, ",",
                                 " or ']' to follow array element",
                                 error))
        goto error;

      child = parse (stream, max_depth - 1, app, error);

      if (!child)
        goto error;

      ast_array_append (&array->children, &array->n_children, child);
      need_comma = TRUE;
    }

  return (AST *) array;

 error:
  ast_array_free (array->children, array->n_children);
  g_slice_free (Array, array);

  return NULL;
}

typedef struct
{
  AST ast;

  AST **children;
  xint_t n_children;
} Tuple;

static xchar_t *
tuple_get_pattern (AST     *ast,
                   xerror_t **error)
{
  Tuple *tuple = (Tuple *) ast;
  xchar_t *result = NULL;
  xchar_t **parts;
  xint_t i;

  parts = g_new (xchar_t *, tuple->n_children + 4);
  parts[tuple->n_children + 1] = (xchar_t *) ")";
  parts[tuple->n_children + 2] = NULL;
  parts[0] = (xchar_t *) "M(";

  for (i = 0; i < tuple->n_children; i++)
    if (!(parts[i + 1] = ast_get_pattern (tuple->children[i], error)))
      break;

  if (i == tuple->n_children)
    result = xstrjoinv ("", parts);

  /* parts[0] should not be freed */
  while (i)
    g_free (parts[i--]);
  g_free (parts);

  return result;
}

static xvariant_t *
tuple_get_value (AST                 *ast,
                 const xvariant_type_t  *type,
                 xerror_t             **error)
{
  Tuple *tuple = (Tuple *) ast;
  const xvariant_type_t *childtype;
  xvariant_builder_t builder;
  xint_t i;

  if (!xvariant_type_is_tuple (type))
    return ast_type_error (ast, type, error);

  xvariant_builder_init (&builder, type);
  childtype = xvariant_type_first (type);

  for (i = 0; i < tuple->n_children; i++)
    {
      xvariant_t *child;

      if (childtype == NULL)
        {
          xvariant_builder_clear (&builder);
          return ast_type_error (ast, type, error);
        }

      if (!(child = ast_get_value (tuple->children[i], childtype, error)))
        {
          xvariant_builder_clear (&builder);
          return FALSE;
        }

      xvariant_builder_add_value (&builder, child);
      childtype = xvariant_type_next (childtype);
    }

  if (childtype != NULL)
    {
      xvariant_builder_clear (&builder);
      return ast_type_error (ast, type, error);
    }

  return xvariant_builder_end (&builder);
}

static void
tuple_free (AST *ast)
{
  Tuple *tuple = (Tuple *) ast;

  ast_array_free (tuple->children, tuple->n_children);
  g_slice_free (Tuple, tuple);
}

static AST *
tuple_parse (TokenStream  *stream,
             xuint_t         max_depth,
             va_list      *app,
             xerror_t      **error)
{
  static const ASTClass tuple_class = {
    tuple_get_pattern,
    maybe_wrapper, tuple_get_value,
    tuple_free
  };
  xboolean_t need_comma = FALSE;
  xboolean_t first = TRUE;
  Tuple *tuple;

  tuple = g_slice_new (Tuple);
  tuple->ast.class = &tuple_class;
  tuple->children = NULL;
  tuple->n_children = 0;

  token_stream_assert (stream, "(");
  while (!token_stream_consume (stream, ")"))
    {
      AST *child;

      if (need_comma &&
          !token_stream_require (stream, ",",
                                 " or ')' to follow tuple element",
                                 error))
        goto error;

      child = parse (stream, max_depth - 1, app, error);

      if (!child)
        goto error;

      ast_array_append (&tuple->children, &tuple->n_children, child);

      /* the first time, we absolutely require a comma, so grab it here
       * and leave need_comma = FALSE so that the code above doesn't
       * require a second comma.
       *
       * the second and remaining times, we set need_comma = TRUE.
       */
      if (first)
        {
          if (!token_stream_require (stream, ",",
                                     " after first tuple element", error))
            goto error;

          first = FALSE;
        }
      else
        need_comma = TRUE;
    }

  return (AST *) tuple;

 error:
  ast_array_free (tuple->children, tuple->n_children);
  g_slice_free (Tuple, tuple);

  return NULL;
}

typedef struct
{
  AST ast;

  AST *value;
} Variant;

static xchar_t *
variant_get_pattern (AST     *ast,
                     xerror_t **error)
{
  return xstrdup ("Mv");
}

static xvariant_t *
variant_get_value (AST                 *ast,
                   const xvariant_type_t  *type,
                   xerror_t             **error)
{
  Variant *variant = (Variant *) ast;
  xvariant_t *child;

  if (!xvariant_type_equal (type, G_VARIANT_TYPE_VARIANT))
    return ast_type_error (ast, type, error);

  child = ast_resolve (variant->value, error);

  if (child == NULL)
    return NULL;

  return xvariant_new_variant (child);
}

static void
variant_free (AST *ast)
{
  Variant *variant = (Variant *) ast;

  ast_free (variant->value);
  g_slice_free (Variant, variant);
}

static AST *
variant_parse (TokenStream  *stream,
               xuint_t         max_depth,
               va_list      *app,
               xerror_t      **error)
{
  static const ASTClass variant_class = {
    variant_get_pattern,
    maybe_wrapper, variant_get_value,
    variant_free
  };
  Variant *variant;
  AST *value;

  token_stream_assert (stream, "<");
  value = parse (stream, max_depth - 1, app, error);

  if (!value)
    return NULL;

  if (!token_stream_require (stream, ">", " to follow variant value", error))
    {
      ast_free (value);
      return NULL;
    }

  variant = g_slice_new (Variant);
  variant->ast.class = &variant_class;
  variant->value = value;

  return (AST *) variant;
}

typedef struct
{
  AST ast;

  AST **keys;
  AST **values;
  xint_t n_children;
} Dictionary;

static xchar_t *
dictionary_get_pattern (AST     *ast,
                        xerror_t **error)
{
  Dictionary *dict = (Dictionary *) ast;
  xchar_t *value_pattern;
  xchar_t *key_pattern;
  xchar_t key_char;
  xchar_t *result;

  if (dict->n_children == 0)
    return xstrdup ("Ma{**}");

  key_pattern = ast_array_get_pattern (dict->keys,
                                       abs (dict->n_children),
                                       error);

  if (key_pattern == NULL)
    return NULL;

  /* we can not have maybe keys */
  if (key_pattern[0] == 'M')
    key_char = key_pattern[1];
  else
    key_char = key_pattern[0];

  g_free (key_pattern);

  /* the basic types,
   * plus undetermined number type and undetermined string type.
   */
  if (!strchr ("bynqiuxthdsogNS", key_char))
    {
      ast_set_error (ast, error, NULL,
                     G_VARIANT_PARSE_ERROR_BASIC_TYPE_EXPECTED,
                     "dictionary keys must have basic types");
      return NULL;
    }

  value_pattern = ast_get_pattern (dict->values[0], error);

  if (value_pattern == NULL)
    return NULL;

  result = xstrdup_printf ("M%s{%c%s}",
                            dict->n_children > 0 ? "a" : "",
                            key_char, value_pattern);
  g_free (value_pattern);

  return result;
}

static xvariant_t *
dictionary_get_value (AST                 *ast,
                      const xvariant_type_t  *type,
                      xerror_t             **error)
{
  Dictionary *dict = (Dictionary *) ast;

  if (dict->n_children == -1)
    {
      const xvariant_type_t *subtype;
      xvariant_builder_t builder;
      xvariant_t *subvalue;

      if (!xvariant_type_is_dict_entry (type))
        return ast_type_error (ast, type, error);

      xvariant_builder_init (&builder, type);

      subtype = xvariant_type_key (type);
      if (!(subvalue = ast_get_value (dict->keys[0], subtype, error)))
        {
          xvariant_builder_clear (&builder);
          return NULL;
        }
      xvariant_builder_add_value (&builder, subvalue);

      subtype = xvariant_type_value (type);
      if (!(subvalue = ast_get_value (dict->values[0], subtype, error)))
        {
          xvariant_builder_clear (&builder);
          return NULL;
        }
      xvariant_builder_add_value (&builder, subvalue);

      return xvariant_builder_end (&builder);
    }
  else
    {
      const xvariant_type_t *entry, *key, *val;
      xvariant_builder_t builder;
      xint_t i;

      if (!xvariant_type_is_subtype_of (type, G_VARIANT_TYPE_DICTIONARY))
        return ast_type_error (ast, type, error);

      entry = xvariant_type_element (type);
      key = xvariant_type_key (entry);
      val = xvariant_type_value (entry);

      xvariant_builder_init (&builder, type);

      for (i = 0; i < dict->n_children; i++)
        {
          xvariant_t *subvalue;

          xvariant_builder_open (&builder, entry);

          if (!(subvalue = ast_get_value (dict->keys[i], key, error)))
            {
              xvariant_builder_clear (&builder);
              return NULL;
            }
          xvariant_builder_add_value (&builder, subvalue);

          if (!(subvalue = ast_get_value (dict->values[i], val, error)))
            {
              xvariant_builder_clear (&builder);
              return NULL;
            }
          xvariant_builder_add_value (&builder, subvalue);
          xvariant_builder_close (&builder);
        }

      return xvariant_builder_end (&builder);
    }
}

static void
dictionary_free (AST *ast)
{
  Dictionary *dict = (Dictionary *) ast;
  xint_t n_children;

  if (dict->n_children > -1)
    n_children = dict->n_children;
  else
    n_children = 1;

  ast_array_free (dict->keys, n_children);
  ast_array_free (dict->values, n_children);
  g_slice_free (Dictionary, dict);
}

static AST *
dictionary_parse (TokenStream  *stream,
                  xuint_t         max_depth,
                  va_list      *app,
                  xerror_t      **error)
{
  static const ASTClass dictionary_class = {
    dictionary_get_pattern,
    maybe_wrapper, dictionary_get_value,
    dictionary_free
  };
  xint_t n_keys, n_values;
  xboolean_t only_one;
  Dictionary *dict;
  AST *first;

  dict = g_slice_new (Dictionary);
  dict->ast.class = &dictionary_class;
  dict->keys = NULL;
  dict->values = NULL;
  n_keys = n_values = 0;

  token_stream_assert (stream, "{");

  if (token_stream_consume (stream, "}"))
    {
      dict->n_children = 0;
      return (AST *) dict;
    }

  if ((first = parse (stream, max_depth - 1, app, error)) == NULL)
    goto error;

  ast_array_append (&dict->keys, &n_keys, first);

  only_one = token_stream_consume (stream, ",");
  if (!only_one &&
      !token_stream_require (stream, ":",
                             " or ',' to follow dictionary entry key",
                             error))
    goto error;

  if ((first = parse (stream, max_depth - 1, app, error)) == NULL)
    goto error;

  ast_array_append (&dict->values, &n_values, first);

  if (only_one)
    {
      if (!token_stream_require (stream, "}", " at end of dictionary entry",
                                 error))
        goto error;

      xassert (n_keys == 1 && n_values == 1);
      dict->n_children = -1;

      return (AST *) dict;
    }

  while (!token_stream_consume (stream, "}"))
    {
      AST *child;

      if (!token_stream_require (stream, ",",
                                 " or '}' to follow dictionary entry", error))
        goto error;

      child = parse (stream, max_depth - 1, app, error);

      if (!child)
        goto error;

      ast_array_append (&dict->keys, &n_keys, child);

      if (!token_stream_require (stream, ":",
                                 " to follow dictionary entry key", error))
        goto error;

      child = parse (stream, max_depth - 1, app, error);

      if (!child)
        goto error;

      ast_array_append (&dict->values, &n_values, child);
    }

  xassert (n_keys == n_values);
  dict->n_children = n_keys;

  return (AST *) dict;

 error:
  ast_array_free (dict->keys, n_keys);
  ast_array_free (dict->values, n_values);
  g_slice_free (Dictionary, dict);

  return NULL;
}

typedef struct
{
  AST ast;
  xchar_t *string;
} String;

static xchar_t *
string_get_pattern (AST     *ast,
                    xerror_t **error)
{
  return xstrdup ("MS");
}

static xvariant_t *
string_get_value (AST                 *ast,
                  const xvariant_type_t  *type,
                  xerror_t             **error)
{
  String *string = (String *) ast;

  if (xvariant_type_equal (type, G_VARIANT_TYPE_STRING))
    return xvariant_new_string (string->string);

  else if (xvariant_type_equal (type, G_VARIANT_TYPE_OBJECT_PATH))
    {
      if (!xvariant_is_object_path (string->string))
        {
          ast_set_error (ast, error, NULL,
                         G_VARIANT_PARSE_ERROR_INVALID_OBJECT_PATH,
                         "not a valid object path");
          return NULL;
        }

      return xvariant_new_object_path (string->string);
    }

  else if (xvariant_type_equal (type, G_VARIANT_TYPE_SIGNATURE))
    {
      if (!xvariant_is_signature (string->string))
        {
          ast_set_error (ast, error, NULL,
                         G_VARIANT_PARSE_ERROR_INVALID_SIGNATURE,
                         "not a valid signature");
          return NULL;
        }

      return xvariant_new_signature (string->string);
    }

  else
    return ast_type_error (ast, type, error);
}

static void
string_free (AST *ast)
{
  String *string = (String *) ast;

  g_free (string->string);
  g_slice_free (String, string);
}

/* Accepts exactly @length hexadecimal digits. No leading sign or `0x`/`0X` prefix allowed.
 * No leading/trailing space allowed. */
static xboolean_t
unicode_unescape (const xchar_t  *src,
                  xint_t         *src_ofs,
                  xchar_t        *dest,
                  xint_t         *dest_ofs,
                  xsize_t         length,
                  SourceRef    *ref,
                  xerror_t      **error)
{
  xchar_t buffer[9];
  xuint64_t value = 0;
  xchar_t *end = NULL;
  xsize_t n_valid_chars;

  (*src_ofs)++;

  xassert (length < sizeof (buffer));
  strncpy (buffer, src + *src_ofs, length);
  buffer[length] = '\0';

  for (n_valid_chars = 0; n_valid_chars < length; n_valid_chars++)
    if (!g_ascii_isxdigit (buffer[n_valid_chars]))
      break;

  if (n_valid_chars == length)
    value = g_ascii_strtoull (buffer, &end, 0x10);

  if (value == 0 || end != buffer + length)
    {
      SourceRef escape_ref;

      escape_ref = *ref;
      escape_ref.start += *src_ofs;
      escape_ref.end = escape_ref.start + n_valid_chars;

      parser_set_error (error, &escape_ref, NULL,
                        G_VARIANT_PARSE_ERROR_INVALID_CHARACTER,
                        "invalid %" G_GSIZE_FORMAT "-character unicode escape", length);
      return FALSE;
    }

  xassert (value <= G_MAXUINT32);

  *dest_ofs += xunichar_to_utf8 (value, dest + *dest_ofs);
  *src_ofs += length;

  return TRUE;
}

static AST *
string_parse (TokenStream  *stream,
              va_list      *app,
              xerror_t      **error)
{
  static const ASTClass string_class = {
    string_get_pattern,
    maybe_wrapper, string_get_value,
    string_free
  };
  String *string;
  SourceRef ref;
  xchar_t *token;
  xsize_t length;
  xchar_t quote;
  xchar_t *str;
  xint_t i, j;

  token_stream_start_ref (stream, &ref);
  token = token_stream_get (stream);
  token_stream_end_ref (stream, &ref);
  length = strlen (token);
  quote = token[0];

  str = g_malloc (length);
  xassert (quote == '"' || quote == '\'');
  j = 0;
  i = 1;
  while (token[i] != quote)
    switch (token[i])
      {
      case '\0':
        parser_set_error (error, &ref, NULL,
                          G_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT,
                          "unterminated string constant");
        g_free (token);
        g_free (str);
        return NULL;

      case '\\':
        switch (token[++i])
          {
          case '\0':
            parser_set_error (error, &ref, NULL,
                              G_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT,
                              "unterminated string constant");
            g_free (token);
            g_free (str);
            return NULL;

          case 'u':
            if (!unicode_unescape (token, &i, str, &j, 4, &ref, error))
              {
                g_free (token);
                g_free (str);
                return NULL;
              }
            continue;

          case 'U':
            if (!unicode_unescape (token, &i, str, &j, 8, &ref, error))
              {
                g_free (token);
                g_free (str);
                return NULL;
              }
            continue;

          case 'a': str[j++] = '\a'; i++; continue;
          case 'b': str[j++] = '\b'; i++; continue;
          case 'f': str[j++] = '\f'; i++; continue;
          case 'n': str[j++] = '\n'; i++; continue;
          case 'r': str[j++] = '\r'; i++; continue;
          case 't': str[j++] = '\t'; i++; continue;
          case 'v': str[j++] = '\v'; i++; continue;
          case '\n': i++; continue;
          }

        G_GNUC_FALLTHROUGH;

      default:
        str[j++] = token[i++];
      }
  str[j++] = '\0';
  g_free (token);

  string = g_slice_new (String);
  string->ast.class = &string_class;
  string->string = str;

  token_stream_next (stream);

  return (AST *) string;
}

typedef struct
{
  AST ast;
  xchar_t *string;
} ByteString;

static xchar_t *
bytestring_get_pattern (AST     *ast,
                        xerror_t **error)
{
  return xstrdup ("May");
}

static xvariant_t *
bytestring_get_value (AST                 *ast,
                      const xvariant_type_t  *type,
                      xerror_t             **error)
{
  ByteString *string = (ByteString *) ast;

  if (!xvariant_type_equal (type, G_VARIANT_TYPE_BYTESTRING))
    return ast_type_error (ast, type, error);

  return xvariant_new_bytestring (string->string);
}

static void
bytestring_free (AST *ast)
{
  ByteString *string = (ByteString *) ast;

  g_free (string->string);
  g_slice_free (ByteString, string);
}

static AST *
bytestring_parse (TokenStream  *stream,
                  va_list      *app,
                  xerror_t      **error)
{
  static const ASTClass bytestring_class = {
    bytestring_get_pattern,
    maybe_wrapper, bytestring_get_value,
    bytestring_free
  };
  ByteString *string;
  SourceRef ref;
  xchar_t *token;
  xsize_t length;
  xchar_t quote;
  xchar_t *str;
  xint_t i, j;

  token_stream_start_ref (stream, &ref);
  token = token_stream_get (stream);
  token_stream_end_ref (stream, &ref);
  xassert (token[0] == 'b');
  length = strlen (token);
  quote = token[1];

  str = g_malloc (length);
  xassert (quote == '"' || quote == '\'');
  j = 0;
  i = 2;
  while (token[i] != quote)
    switch (token[i])
      {
      case '\0':
        parser_set_error (error, &ref, NULL,
                          G_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT,
                          "unterminated string constant");
        g_free (str);
        g_free (token);
        return NULL;

      case '\\':
        switch (token[++i])
          {
          case '\0':
            parser_set_error (error, &ref, NULL,
                              G_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT,
                              "unterminated string constant");
            g_free (str);
            g_free (token);
            return NULL;

          case '0': case '1': case '2': case '3':
          case '4': case '5': case '6': case '7':
            {
              /* up to 3 characters */
              xuchar_t val = token[i++] - '0';

              if ('0' <= token[i] && token[i] < '8')
                val = (val << 3) | (token[i++] - '0');

              if ('0' <= token[i] && token[i] < '8')
                val = (val << 3) | (token[i++] - '0');

              str[j++] = val;
            }
            continue;

          case 'a': str[j++] = '\a'; i++; continue;
          case 'b': str[j++] = '\b'; i++; continue;
          case 'f': str[j++] = '\f'; i++; continue;
          case 'n': str[j++] = '\n'; i++; continue;
          case 'r': str[j++] = '\r'; i++; continue;
          case 't': str[j++] = '\t'; i++; continue;
          case 'v': str[j++] = '\v'; i++; continue;
          case '\n': i++; continue;
          }

        G_GNUC_FALLTHROUGH;

      default:
        str[j++] = token[i++];
      }
  str[j++] = '\0';
  g_free (token);

  string = g_slice_new (ByteString);
  string->ast.class = &bytestring_class;
  string->string = str;

  token_stream_next (stream);

  return (AST *) string;
}

typedef struct
{
  AST ast;

  xchar_t *token;
} Number;

static xchar_t *
number_get_pattern (AST     *ast,
                    xerror_t **error)
{
  Number *number = (Number *) ast;

  if (strchr (number->token, '.') ||
      (!xstr_has_prefix (number->token, "0x") && strchr (number->token, 'e')) ||
      strstr (number->token, "inf") ||
      strstr (number->token, "nan"))
    return xstrdup ("Md");

  return xstrdup ("MN");
}

static xvariant_t *
number_overflow (AST                 *ast,
                 const xvariant_type_t  *type,
                 xerror_t             **error)
{
  ast_set_error (ast, error, NULL,
                 G_VARIANT_PARSE_ERROR_NUMBER_OUT_OF_RANGE,
                 "number out of range for type '%c'",
                 xvariant_type_peek_string (type)[0]);
  return NULL;
}

static xvariant_t *
number_get_value (AST                 *ast,
                  const xvariant_type_t  *type,
                  xerror_t             **error)
{
  Number *number = (Number *) ast;
  const xchar_t *token;
  xboolean_t negative;
  xboolean_t floating;
  xuint64_t abs_val;
  xdouble_t dbl_val;
  xchar_t *end;

  token = number->token;

  if (xvariant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
    {
      floating = TRUE;

      errno = 0;
      dbl_val = g_ascii_strtod (token, &end);
      if (dbl_val != 0.0 && errno == ERANGE)
        {
          ast_set_error (ast, error, NULL,
                         G_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG,
                         "number too big for any type");
          return NULL;
        }

      /* silence uninitialised warnings... */
      negative = FALSE;
      abs_val = 0;
    }
  else
    {
      floating = FALSE;
      negative = token[0] == '-';
      if (token[0] == '-')
        token++;

      errno = 0;
      abs_val = g_ascii_strtoull (token, &end, 0);
      if (abs_val == G_MAXUINT64 && errno == ERANGE)
        {
          ast_set_error (ast, error, NULL,
                         G_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG,
                         "integer too big for any type");
          return NULL;
        }

      if (abs_val == 0)
        negative = FALSE;

      /* silence uninitialised warning... */
      dbl_val = 0.0;
    }

  if (*end != '\0')
    {
      SourceRef ref;

      ref = ast->source_ref;
      ref.start += end - number->token;
      ref.end = ref.start + 1;

      parser_set_error (error, &ref, NULL,
                        G_VARIANT_PARSE_ERROR_INVALID_CHARACTER,
                        "invalid character in number");
      return NULL;
     }

  if (floating)
    return xvariant_new_double (dbl_val);

  switch (*xvariant_type_peek_string (type))
    {
    case 'y':
      if (negative || abs_val > G_MAXUINT8)
        return number_overflow (ast, type, error);
      return xvariant_new_byte (abs_val);

    case 'n':
      if (abs_val - negative > G_MAXINT16)
        return number_overflow (ast, type, error);
      if (negative && abs_val > G_MAXINT16)
        return xvariant_new_int16 (G_MININT16);
      return xvariant_new_int16 (negative ?
                                  -((gint16) abs_val) : ((gint16) abs_val));

    case 'q':
      if (negative || abs_val > G_MAXUINT16)
        return number_overflow (ast, type, error);
      return xvariant_new_uint16 (abs_val);

    case 'i':
      if (abs_val - negative > G_MAXINT32)
        return number_overflow (ast, type, error);
      if (negative && abs_val > G_MAXINT32)
        return xvariant_new_int32 (G_MININT32);
      return xvariant_new_int32 (negative ?
                                  -((gint32) abs_val) : ((gint32) abs_val));

    case 'u':
      if (negative || abs_val > G_MAXUINT32)
        return number_overflow (ast, type, error);
      return xvariant_new_uint32 (abs_val);

    case 'x':
      if (abs_val - negative > G_MAXINT64)
        return number_overflow (ast, type, error);
      if (negative && abs_val > G_MAXINT64)
        return xvariant_new_int64 (G_MININT64);
      return xvariant_new_int64 (negative ?
                                  -((sint64_t) abs_val) : ((sint64_t) abs_val));

    case 't':
      if (negative)
        return number_overflow (ast, type, error);
      return xvariant_new_uint64 (abs_val);

    case 'h':
      if (abs_val - negative > G_MAXINT32)
        return number_overflow (ast, type, error);
      if (negative && abs_val > G_MAXINT32)
        return xvariant_new_handle (G_MININT32);
      return xvariant_new_handle (negative ?
                                   -((gint32) abs_val) : ((gint32) abs_val));

    default:
      return ast_type_error (ast, type, error);
    }
}

static void
number_free (AST *ast)
{
  Number *number = (Number *) ast;

  g_free (number->token);
  g_slice_free (Number, number);
}

static AST *
number_parse (TokenStream  *stream,
              va_list      *app,
              xerror_t      **error)
{
  static const ASTClass number_class = {
    number_get_pattern,
    maybe_wrapper, number_get_value,
    number_free
  };
  Number *number;

  number = g_slice_new (Number);
  number->ast.class = &number_class;
  number->token = token_stream_get (stream);
  token_stream_next (stream);

  return (AST *) number;
}

typedef struct
{
  AST ast;
  xboolean_t value;
} Boolean;

static xchar_t *
boolean_get_pattern (AST     *ast,
                     xerror_t **error)
{
  return xstrdup ("Mb");
}

static xvariant_t *
boolean_get_value (AST                 *ast,
                   const xvariant_type_t  *type,
                   xerror_t             **error)
{
  Boolean *boolean = (Boolean *) ast;

  if (!xvariant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
    return ast_type_error (ast, type, error);

  return xvariant_new_boolean (boolean->value);
}

static void
boolean_free (AST *ast)
{
  Boolean *boolean = (Boolean *) ast;

  g_slice_free (Boolean, boolean);
}

static AST *
boolean_new (xboolean_t value)
{
  static const ASTClass boolean_class = {
    boolean_get_pattern,
    maybe_wrapper, boolean_get_value,
    boolean_free
  };
  Boolean *boolean;

  boolean = g_slice_new (Boolean);
  boolean->ast.class = &boolean_class;
  boolean->value = value;

  return (AST *) boolean;
}

typedef struct
{
  AST ast;

  xvariant_t *value;
} Positional;

static xchar_t *
positional_get_pattern (AST     *ast,
                        xerror_t **error)
{
  Positional *positional = (Positional *) ast;

  return xstrdup (xvariant_get_type_string (positional->value));
}

static xvariant_t *
positional_get_value (AST                 *ast,
                      const xvariant_type_t  *type,
                      xerror_t             **error)
{
  Positional *positional = (Positional *) ast;
  xvariant_t *value;

  xassert (positional->value != NULL);

  if G_UNLIKELY (!xvariant_is_of_type (positional->value, type))
    return ast_type_error (ast, type, error);

  /* NOTE: if _get is called more than once then
   * things get messed up with respect to floating refs.
   *
   * fortunately, this function should only ever get called once.
   */
  xassert (positional->value != NULL);
  value = positional->value;
  positional->value = NULL;

  return value;
}

static void
positional_free (AST *ast)
{
  Positional *positional = (Positional *) ast;

  /* if positional->value is set, just leave it.
   * memory management doesn't matter in case of programmer error.
   */
  g_slice_free (Positional, positional);
}

static AST *
positional_parse (TokenStream  *stream,
                  va_list      *app,
                  xerror_t      **error)
{
  static const ASTClass positional_class = {
    positional_get_pattern,
    positional_get_value, NULL,
    positional_free
  };
  Positional *positional;
  const xchar_t *endptr;
  xchar_t *token;

  token = token_stream_get (stream);
  xassert (token[0] == '%');

  positional = g_slice_new (Positional);
  positional->ast.class = &positional_class;
  positional->value = xvariant_new_va (token + 1, &endptr, app);

  if (*endptr || positional->value == NULL)
    {
      token_stream_set_error (stream, error, TRUE,
                              G_VARIANT_PARSE_ERROR_INVALID_FORMAT_STRING,
                              "invalid xvariant_t format string");
      /* memory management doesn't matter in case of programmer error. */
      return NULL;
    }

  token_stream_next (stream);
  g_free (token);

  return (AST *) positional;
}

typedef struct
{
  AST ast;

  xvariant_type_t *type;
  AST *child;
} TypeDecl;

static xchar_t *
typedecl_get_pattern (AST     *ast,
                      xerror_t **error)
{
  TypeDecl *decl = (TypeDecl *) ast;

  return xvariant_type_dup_string (decl->type);
}

static xvariant_t *
typedecl_get_value (AST                 *ast,
                    const xvariant_type_t  *type,
                    xerror_t             **error)
{
  TypeDecl *decl = (TypeDecl *) ast;

  return ast_get_value (decl->child, type, error);
}

static void
typedecl_free (AST *ast)
{
  TypeDecl *decl = (TypeDecl *) ast;

  ast_free (decl->child);
  xvariant_type_free (decl->type);
  g_slice_free (TypeDecl, decl);
}

static AST *
typedecl_parse (TokenStream  *stream,
                xuint_t         max_depth,
                va_list      *app,
                xerror_t      **error)
{
  static const ASTClass typedecl_class = {
    typedecl_get_pattern,
    typedecl_get_value, NULL,
    typedecl_free
  };
  xvariant_type_t *type;
  TypeDecl *decl;
  AST *child;

  if (token_stream_peek (stream, '@'))
    {
      xchar_t *token;

      token = token_stream_get (stream);

      if (!xvariant_type_string_is_valid (token + 1))
        {
          token_stream_set_error (stream, error, TRUE,
                                  G_VARIANT_PARSE_ERROR_INVALID_TYPE_STRING,
                                  "invalid type declaration");
          g_free (token);

          return NULL;
        }

      type = xvariant_type_new (token + 1);

      if (!xvariant_type_is_definite (type))
        {
          token_stream_set_error (stream, error, TRUE,
                                  G_VARIANT_PARSE_ERROR_DEFINITE_TYPE_EXPECTED,
                                  "type declarations must be definite");
          xvariant_type_free (type);
          g_free (token);

          return NULL;
        }

      token_stream_next (stream);
      g_free (token);
    }
  else
    {
      if (token_stream_consume (stream, "boolean"))
        type = xvariant_type_copy (G_VARIANT_TYPE_BOOLEAN);

      else if (token_stream_consume (stream, "byte"))
        type = xvariant_type_copy (G_VARIANT_TYPE_BYTE);

      else if (token_stream_consume (stream, "int16"))
        type = xvariant_type_copy (G_VARIANT_TYPE_INT16);

      else if (token_stream_consume (stream, "uint16"))
        type = xvariant_type_copy (G_VARIANT_TYPE_UINT16);

      else if (token_stream_consume (stream, "int32"))
        type = xvariant_type_copy (G_VARIANT_TYPE_INT32);

      else if (token_stream_consume (stream, "handle"))
        type = xvariant_type_copy (G_VARIANT_TYPE_HANDLE);

      else if (token_stream_consume (stream, "uint32"))
        type = xvariant_type_copy (G_VARIANT_TYPE_UINT32);

      else if (token_stream_consume (stream, "int64"))
        type = xvariant_type_copy (G_VARIANT_TYPE_INT64);

      else if (token_stream_consume (stream, "uint64"))
        type = xvariant_type_copy (G_VARIANT_TYPE_UINT64);

      else if (token_stream_consume (stream, "double"))
        type = xvariant_type_copy (G_VARIANT_TYPE_DOUBLE);

      else if (token_stream_consume (stream, "string"))
        type = xvariant_type_copy (G_VARIANT_TYPE_STRING);

      else if (token_stream_consume (stream, "objectpath"))
        type = xvariant_type_copy (G_VARIANT_TYPE_OBJECT_PATH);

      else if (token_stream_consume (stream, "signature"))
        type = xvariant_type_copy (G_VARIANT_TYPE_SIGNATURE);

      else
        {
          token_stream_set_error (stream, error, TRUE,
                                  G_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD,
                                  "unknown keyword");
          return NULL;
        }
    }

  if ((child = parse (stream, max_depth - 1, app, error)) == NULL)
    {
      xvariant_type_free (type);
      return NULL;
    }

  decl = g_slice_new (TypeDecl);
  decl->ast.class = &typedecl_class;
  decl->type = type;
  decl->child = child;

  return (AST *) decl;
}

static AST *
parse (TokenStream  *stream,
       xuint_t         max_depth,
       va_list      *app,
       xerror_t      **error)
{
  SourceRef source_ref;
  AST *result;

  if (max_depth == 0)
    {
      token_stream_set_error (stream, error, FALSE,
                              G_VARIANT_PARSE_ERROR_RECURSION,
                              "variant nested too deeply");
      return NULL;
    }

  token_stream_prepare (stream);
  token_stream_start_ref (stream, &source_ref);

  if (token_stream_peek (stream, '['))
    result = array_parse (stream, max_depth, app, error);

  else if (token_stream_peek (stream, '('))
    result = tuple_parse (stream, max_depth, app, error);

  else if (token_stream_peek (stream, '<'))
    result = variant_parse (stream, max_depth, app, error);

  else if (token_stream_peek (stream, '{'))
    result = dictionary_parse (stream, max_depth, app, error);

  else if (app && token_stream_peek (stream, '%'))
    result = positional_parse (stream, app, error);

  else if (token_stream_consume (stream, "true"))
    result = boolean_new (TRUE);

  else if (token_stream_consume (stream, "false"))
    result = boolean_new (FALSE);

  else if (token_stream_is_numeric (stream) ||
           token_stream_peek_string (stream, "inf") ||
           token_stream_peek_string (stream, "nan"))
    result = number_parse (stream, app, error);

  else if (token_stream_peek (stream, 'n') ||
           token_stream_peek (stream, 'j'))
    result = maybe_parse (stream, max_depth, app, error);

  else if (token_stream_peek (stream, '@') ||
           token_stream_is_keyword (stream))
    result = typedecl_parse (stream, max_depth, app, error);

  else if (token_stream_peek (stream, '\'') ||
           token_stream_peek (stream, '"'))
    result = string_parse (stream, app, error);

  else if (token_stream_peek2 (stream, 'b', '\'') ||
           token_stream_peek2 (stream, 'b', '"'))
    result = bytestring_parse (stream, app, error);

  else
    {
      token_stream_set_error (stream, error, FALSE,
                              G_VARIANT_PARSE_ERROR_VALUE_EXPECTED,
                              "expected value");
      return NULL;
    }

  if (result != NULL)
    {
      token_stream_end_ref (stream, &source_ref);
      result->source_ref = source_ref;
    }

  return result;
}

/**
 * xvariant_parse:
 * @type: (nullable): a #xvariant_type_t, or %NULL
 * @text: a string containing a xvariant_t in text form
 * @limit: (nullable): a pointer to the end of @text, or %NULL
 * @endptr: (nullable): a location to store the end pointer, or %NULL
 * @error: (nullable): a pointer to a %NULL #xerror_t pointer, or %NULL
 *
 * Parses a #xvariant_t from a text representation.
 *
 * A single #xvariant_t is parsed from the content of @text.
 *
 * The format is described [here][gvariant-text].
 *
 * The memory at @limit will never be accessed and the parser behaves as
 * if the character at @limit is the nul terminator.  This has the
 * effect of bounding @text.
 *
 * If @endptr is non-%NULL then @text is permitted to contain data
 * following the value that this function parses and @endptr will be
 * updated to point to the first character past the end of the text
 * parsed by this function.  If @endptr is %NULL and there is extra data
 * then an error is returned.
 *
 * If @type is non-%NULL then the value will be parsed to have that
 * type.  This may result in additional parse errors (in the case that
 * the parsed value doesn't fit the type) but may also result in fewer
 * errors (in the case that the type would have been ambiguous, such as
 * with empty arrays).
 *
 * In the event that the parsing is successful, the resulting #xvariant_t
 * is returned. It is never floating, and must be freed with
 * xvariant_unref().
 *
 * In case of any error, %NULL will be returned.  If @error is non-%NULL
 * then it will be set to reflect the error that occurred.
 *
 * Officially, the language understood by the parser is "any string
 * produced by xvariant_print()".
 *
 * There may be implementation specific restrictions on deeply nested values,
 * which would result in a %G_VARIANT_PARSE_ERROR_RECURSION error. #xvariant_t is
 * guaranteed to handle nesting up to at least 64 levels.
 *
 * Returns: a non-floating reference to a #xvariant_t, or %NULL
 **/
xvariant_t *
xvariant_parse (const xvariant_type_t  *type,
                 const xchar_t         *text,
                 const xchar_t         *limit,
                 const xchar_t        **endptr,
                 xerror_t             **error)
{
  TokenStream stream = { 0, };
  xvariant_t *result = NULL;
  AST *ast;

  xreturn_val_if_fail (text != NULL, NULL);
  xreturn_val_if_fail (text == limit || text != NULL, NULL);

  stream.start = text;
  stream.stream = text;
  stream.end = limit;

  if ((ast = parse (&stream, G_VARIANT_MAX_RECURSION_DEPTH, NULL, error)))
    {
      if (type == NULL)
        result = ast_resolve (ast, error);
      else
        result = ast_get_value (ast, type, error);

      if (result != NULL)
        {
          xvariant_ref_sink (result);

          if (endptr == NULL)
            {
              while (stream.stream != limit &&
                     g_ascii_isspace (*stream.stream))
                stream.stream++;

              if (stream.stream != limit && *stream.stream != '\0')
                {
                  SourceRef ref = { stream.stream - text,
                                    stream.stream - text };

                  parser_set_error (error, &ref, NULL,
                                    G_VARIANT_PARSE_ERROR_INPUT_NOT_AT_END,
                                    "expected end of input");
                  xvariant_unref (result);

                  result = NULL;
                }
            }
          else
            *endptr = stream.stream;
        }

      ast_free (ast);
    }

  return result;
}

/**
 * xvariant_new_parsed_va:
 * @format: a text format #xvariant_t
 * @app: a pointer to a #va_list
 *
 * Parses @format and returns the result.
 *
 * This is the version of xvariant_new_parsed() intended to be used
 * from libraries.
 *
 * The return value will be floating if it was a newly created xvariant_t
 * instance.  In the case that @format simply specified the collection
 * of a #xvariant_t pointer (eg: @format was "%*") then the collected
 * #xvariant_t pointer will be returned unmodified, without adding any
 * additional references.
 *
 * Note that the arguments in @app must be of the correct width for their types
 * specified in @format when collected into the #va_list. See
 * the [xvariant_t varargs documentation][gvariant-varargs].
 *
 * In order to behave correctly in all cases it is necessary for the
 * calling function to xvariant_ref_sink() the return result before
 * returning control to the user that originally provided the pointer.
 * At this point, the caller will have their own full reference to the
 * result.  This can also be done by adding the result to a container,
 * or by passing it to another xvariant_new() call.
 *
 * Returns: a new, usually floating, #xvariant_t
 **/
xvariant_t *
xvariant_new_parsed_va (const xchar_t *format,
                         va_list     *app)
{
  TokenStream stream = { 0, };
  xvariant_t *result = NULL;
  xerror_t *error = NULL;
  AST *ast;

  xreturn_val_if_fail (format != NULL, NULL);
  xreturn_val_if_fail (app != NULL, NULL);

  stream.start = format;
  stream.stream = format;
  stream.end = NULL;

  if ((ast = parse (&stream, G_VARIANT_MAX_RECURSION_DEPTH, app, &error)))
    {
      result = ast_resolve (ast, &error);
      ast_free (ast);
    }

  if (error != NULL)
    xerror ("xvariant_new_parsed: %s", error->message);

  if (*stream.stream)
    xerror ("xvariant_new_parsed: trailing text after value");

  g_clear_error (&error);

  return result;
}

/**
 * xvariant_new_parsed:
 * @format: a text format #xvariant_t
 * @...: arguments as per @format
 *
 * Parses @format and returns the result.
 *
 * @format must be a text format #xvariant_t with one extension: at any
 * point that a value may appear in the text, a '%' character followed
 * by a xvariant_t format string (as per xvariant_new()) may appear.  In
 * that case, the same arguments are collected from the argument list as
 * xvariant_new() would have collected.
 *
 * Note that the arguments must be of the correct width for their types
 * specified in @format. This can be achieved by casting them. See
 * the [xvariant_t varargs documentation][gvariant-varargs].
 *
 * Consider this simple example:
 * |[<!-- language="C" -->
 *  xvariant_new_parsed ("[('one', 1), ('two', %i), (%s, 3)]", 2, "three");
 * ]|
 *
 * In the example, the variable argument parameters are collected and
 * filled in as if they were part of the original string to produce the
 * result of
 * |[<!-- language="C" -->
 * [('one', 1), ('two', 2), ('three', 3)]
 * ]|
 *
 * This function is intended only to be used with @format as a string
 * literal.  Any parse error is fatal to the calling process.  If you
 * want to parse data from untrusted sources, use xvariant_parse().
 *
 * You may not use this function to return, unmodified, a single
 * #xvariant_t pointer from the argument list.  ie: @format may not solely
 * be anything along the lines of "%*", "%?", "\%r", or anything starting
 * with "%@".
 *
 * Returns: a new floating #xvariant_t instance
 **/
xvariant_t *
xvariant_new_parsed (const xchar_t *format,
                      ...)
{
  xvariant_t *result;
  va_list ap;

  va_start (ap, format);
  result = xvariant_new_parsed_va (format, &ap);
  va_end (ap);

  return result;
}

/**
 * xvariant_builder_add_parsed:
 * @builder: a #xvariant_builder_t
 * @format: a text format #xvariant_t
 * @...: arguments as per @format
 *
 * Adds to a #xvariant_builder_t.
 *
 * This call is a convenience wrapper that is exactly equivalent to
 * calling xvariant_new_parsed() followed by
 * xvariant_builder_add_value().
 *
 * Note that the arguments must be of the correct width for their types
 * specified in @format_string. This can be achieved by casting them. See
 * the [xvariant_t varargs documentation][gvariant-varargs].
 *
 * This function might be used as follows:
 *
 * |[<!-- language="C" -->
 * xvariant_t *
 * make_pointless_dictionary (void)
 * {
 *   xvariant_builder_t builder;
 *   int i;
 *
 *   xvariant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
 *   xvariant_builder_add_parsed (&builder, "{'width', <%i>}", 600);
 *   xvariant_builder_add_parsed (&builder, "{'title', <%s>}", "foo");
 *   xvariant_builder_add_parsed (&builder, "{'transparency', <0.5>}");
 *   return xvariant_builder_end (&builder);
 * }
 * ]|
 *
 * Since: 2.26
 */
void
xvariant_builder_add_parsed (xvariant_builder_t *builder,
                              const xchar_t     *format,
                              ...)
{
  va_list ap;

  va_start (ap, format);
  xvariant_builder_add_value (builder, xvariant_new_parsed_va (format, &ap));
  va_end (ap);
}

static xboolean_t
parse_num (const xchar_t *num,
           const xchar_t *limit,
           xuint_t       *result)
{
  xchar_t *endptr;
  sint64_t bignum;

  bignum = g_ascii_strtoll (num, &endptr, 10);

  if (endptr != limit)
    return FALSE;

  if (bignum < 0 || bignum > G_MAXINT)
    return FALSE;

  *result = (xuint_t) bignum;

  return TRUE;
}

static void
add_last_line (xstring_t     *err,
               const xchar_t *str)
{
  const xchar_t *last_nl;
  xchar_t *chomped;
  xint_t i;

  /* This is an error at the end of input.  If we have a file
   * with newlines, that's probably the empty string after the
   * last newline, which is not the most useful thing to show.
   *
   * Instead, show the last line of non-whitespace that we have
   * and put the pointer at the end of it.
   */
  chomped = xstrchomp (xstrdup (str));
  last_nl = strrchr (chomped, '\n');
  if (last_nl == NULL)
    last_nl = chomped;
  else
    last_nl++;

  /* Print the last line like so:
   *
   *   [1, 2, 3,
   *            ^
   */
  xstring_append (err, "  ");
  if (last_nl[0])
    xstring_append (err, last_nl);
  else
    xstring_append (err, "(empty input)");
  xstring_append (err, "\n  ");
  for (i = 0; last_nl[i]; i++)
    xstring_append_c (err, ' ');
  xstring_append (err, "^\n");
  g_free (chomped);
}

static void
add_lines_from_range (xstring_t     *err,
                      const xchar_t *str,
                      const xchar_t *start1,
                      const xchar_t *end1,
                      const xchar_t *start2,
                      const xchar_t *end2)
{
  while (str < end1 || str < end2)
    {
      const xchar_t *nl;

      nl = str + strcspn (str, "\n");

      if ((start1 < nl && str < end1) || (start2 < nl && str < end2))
        {
          const xchar_t *s;

          /* We're going to print this line */
          xstring_append (err, "  ");
          xstring_append_len (err, str, nl - str);
          xstring_append (err, "\n  ");

          /* And add underlines... */
          for (s = str; s < nl; s++)
            {
              if ((start1 <= s && s < end1) || (start2 <= s && s < end2))
                xstring_append_c (err, '^');
              else
                xstring_append_c (err, ' ');
            }
          xstring_append_c (err, '\n');
        }

      if (!*nl)
        break;

      str = nl + 1;
    }
}

/**
 * xvariant_parse_error_print_context:
 * @error: a #xerror_t from the #GVariantParseError domain
 * @source_str: the string that was given to the parser
 *
 * Pretty-prints a message showing the context of a #xvariant_t parse
 * error within the string for which parsing was attempted.
 *
 * The resulting string is suitable for output to the console or other
 * monospace media where newlines are treated in the usual way.
 *
 * The message will typically look something like one of the following:
 *
 * |[
 * unterminated string constant:
 *   (1, 2, 3, 'abc
 *             ^^^^
 * ]|
 *
 * or
 *
 * |[
 * unable to find a common type:
 *   [1, 2, 3, 'str']
 *    ^        ^^^^^
 * ]|
 *
 * The format of the message may change in a future version.
 *
 * @error must have come from a failed attempt to xvariant_parse() and
 * @source_str must be exactly the same string that caused the error.
 * If @source_str was not nul-terminated when you passed it to
 * xvariant_parse() then you must add nul termination before using this
 * function.
 *
 * Returns: (transfer full): the printed message
 *
 * Since: 2.40
 **/
xchar_t *
xvariant_parse_error_print_context (xerror_t      *error,
                                     const xchar_t *source_str)
{
  const xchar_t *colon, *dash, *comma;
  xboolean_t success = FALSE;
  xstring_t *err;

  xreturn_val_if_fail (error->domain == G_VARIANT_PARSE_ERROR, FALSE);

  /* We can only have a limited number of possible types of ranges
   * emitted from the parser:
   *
   *  - a:          -- usually errors from the tokeniser (eof, invalid char, etc.)
   *  - a-b:        -- usually errors from handling one single token
   *  - a-b,c-d:    -- errors involving two tokens (ie: type inferencing)
   *
   * We never see, for example "a,c".
   */

  colon = strchr (error->message, ':');
  dash = strchr (error->message, '-');
  comma = strchr (error->message, ',');

  if (!colon)
    return NULL;

  err = xstring_new (colon + 1);
  xstring_append (err, ":\n");

  if (dash == NULL || colon < dash)
    {
      xuint_t point;

      /* we have a single point */
      if (!parse_num (error->message, colon, &point))
        goto out;

      if (point >= strlen (source_str))
        /* the error is at the end of the input */
        add_last_line (err, source_str);
      else
        /* otherwise just treat it as an error at a thin range */
        add_lines_from_range (err, source_str, source_str + point, source_str + point + 1, NULL, NULL);
    }
  else
    {
      /* We have one or two ranges... */
      if (comma && comma < colon)
        {
          xuint_t start1, end1, start2, end2;
          const xchar_t *dash2;

          /* Two ranges */
          dash2 = strchr (comma, '-');

          if (!parse_num (error->message, dash, &start1) || !parse_num (dash + 1, comma, &end1) ||
              !parse_num (comma + 1, dash2, &start2) || !parse_num (dash2 + 1, colon, &end2))
            goto out;

          add_lines_from_range (err, source_str,
                                source_str + start1, source_str + end1,
                                source_str + start2, source_str + end2);
        }
      else
        {
          xuint_t start, end;

          /* One range */
          if (!parse_num (error->message, dash, &start) || !parse_num (dash + 1, colon, &end))
            goto out;

          add_lines_from_range (err, source_str, source_str + start, source_str + end, NULL, NULL);
        }
    }

  success = TRUE;

out:
  return xstring_free (err, !success);
}
