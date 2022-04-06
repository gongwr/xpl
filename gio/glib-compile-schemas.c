/*
 * Copyright © 2010 Codethink Limited
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

/* Prologue {{{1 */
#include "config.h"

#include <gstdio.h>
#include <gi18n.h>

#include <string.h>
#include <stdio.h>
#include <locale.h>

#include "gvdb/gvdb-builder.h"
#include "strinfo.c"
#include "glib/glib-private.h"

static void
strip_string (xstring_t *string)
{
  xint_t i;

  for (i = 0; g_ascii_isspace (string->str[i]); i++);
  xstring_erase (string, 0, i);

  if (string->len > 0)
    {
      /* len > 0, so there must be at least one non-whitespace character */
      for (i = string->len - 1; g_ascii_isspace (string->str[i]); i--);
      xstring_truncate (string, i + 1);
    }
}

/* Handling of <enum> {{{1 */
typedef struct
{
  xstring_t *strinfo;

  xboolean_t is_flags;
} EnumState;

static void
enum_state_free (xpointer_t data)
{
  EnumState *state = data;

  xstring_free (state->strinfo, TRUE);
  g_slice_free (EnumState, state);
}

static EnumState *
enum_state_new (xboolean_t is_flags)
{
  EnumState *state;

  state = g_slice_new (EnumState);
  state->strinfo = xstring_new (NULL);
  state->is_flags = is_flags;

  return state;
}

static void
enum_state_add_value (EnumState    *state,
                      const xchar_t  *nick,
                      const xchar_t  *valuestr,
                      xerror_t      **error)
{
  sint64_t value;
  xchar_t *end;

  if (nick[0] == '\0' || nick[1] == '\0')
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("nick must be a minimum of 2 characters"));
      return;
    }

  value = g_ascii_strtoll (valuestr, &end, 0);
  if (*end || (state->is_flags ?
                (value > G_MAXUINT32 || value < 0) :
                (value > G_MAXINT32 || value < G_MININT32)))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("Invalid numeric value"));
      return;
    }

  if (strinfo_builder_contains (state->strinfo, nick))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<value nick='%s'/> already specified"), nick);
      return;
    }

  if (strinfo_builder_contains_value (state->strinfo, value))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("value='%s' already specified"), valuestr);
      return;
    }

  /* Silently drop the null case if it is mentioned.
   * It is properly denoted with an empty array.
   */
  if (state->is_flags && value == 0)
    return;

  if (state->is_flags && (value & (value - 1)))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("flags values must have at most 1 bit set"));
      return;
    }

  /* Since we reject exact duplicates of value='' and we only allow one
   * bit to be set, it's not possible to have overlaps.
   *
   * If we loosen the one-bit-set restriction we need an overlap check.
   */

  strinfo_builder_append_item (state->strinfo, nick, value);
}

static void
enum_state_end (EnumState **state_ptr,
                xerror_t    **error)
{
  EnumState *state;

  state = *state_ptr;
  *state_ptr = NULL;

  if (state->strinfo->len == 0)
    g_set_error (error,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                 _("<%s> must contain at least one <value>"),
                 state->is_flags ? "flags" : "enum");
}

/* Handling of <key> {{{1 */
typedef struct
{
  /* for <child>, @child_schema will be set.
   * for <key>, everything else will be set.
   */
  xchar_t        *child_schema;


  xvariant_type_t *type;
  xboolean_t      have_gettext_domain;

  xchar_t         l10n;
  xchar_t        *l10n_context;
  xstring_t      *unparsed_default_value;
  xvariant_t     *default_value;

  xvariant_dict_t *desktop_overrides;

  xstring_t      *strinfo;
  xboolean_t      is_enum;
  xboolean_t      is_flags;

  xvariant_t     *minimum;
  xvariant_t     *maximum;

  xboolean_t      has_choices;
  xboolean_t      has_aliases;
  xboolean_t      is_override;

  xboolean_t      checked;
  xvariant_t     *serialised;

  xboolean_t      summary_seen;
  xboolean_t      description_seen;
} KeyState;

static KeyState *
key_state_new (const xchar_t *type_string,
               const xchar_t *gettext_domain,
               xboolean_t     is_enum,
               xboolean_t     is_flags,
               xstring_t     *strinfo)
{
  KeyState *state;

  state = g_slice_new0 (KeyState);
  state->type = xvariant_type_new (type_string);
  state->have_gettext_domain = gettext_domain != NULL;
  state->is_enum = is_enum;
  state->is_flags = is_flags;
  state->summary_seen = FALSE;
  state->description_seen = FALSE;

  if (strinfo)
    state->strinfo = xstring_new_len (strinfo->str, strinfo->len);
  else
    state->strinfo = xstring_new (NULL);

  return state;
}

static KeyState *
key_state_override (KeyState    *state,
                    const xchar_t *gettext_domain)
{
  KeyState *copy;

  copy = g_slice_new0 (KeyState);
  copy->type = xvariant_type_copy (state->type);
  copy->have_gettext_domain = gettext_domain != NULL;
  copy->strinfo = xstring_new_len (state->strinfo->str,
                                    state->strinfo->len);
  copy->is_enum = state->is_enum;
  copy->is_flags = state->is_flags;
  copy->is_override = TRUE;

  if (state->minimum)
    {
      copy->minimum = xvariant_ref (state->minimum);
      copy->maximum = xvariant_ref (state->maximum);
    }

  return copy;
}

static KeyState *
key_state_new_child (const xchar_t *child_schema)
{
  KeyState *state;

  state = g_slice_new0 (KeyState);
  state->child_schema = xstrdup (child_schema);

  return state;
}

static xboolean_t
is_valid_choices (xvariant_t *variant,
                  xstring_t  *strinfo)
{
  switch (xvariant_classify (variant))
    {
      case XVARIANT_CLASS_MAYBE:
      case XVARIANT_CLASS_ARRAY:
        {
          xboolean_t valid = TRUE;
          xvariant_iter_t iter;

          xvariant_iter_init (&iter, variant);

          while (valid && (variant = xvariant_iter_next_value (&iter)))
            {
              valid = is_valid_choices (variant, strinfo);
              xvariant_unref (variant);
            }

          return valid;
        }

      case XVARIANT_CLASS_STRING:
        return strinfo_is_string_valid ((const xuint32_t *) strinfo->str,
                                        strinfo->len / 4,
                                        xvariant_get_string (variant, NULL));

      default:
        g_assert_not_reached ();
    }
}


/* Gets called at </default> </choices> or <range/> to check for
 * validity of the default value so that any inconsistency is
 * reported as soon as it is encountered.
 */
static void
key_state_check_range (KeyState  *state,
                       xerror_t   **error)
{
  if (state->default_value)
    {
      const xchar_t *tag;

      tag = state->is_override ? "override" : "default";

      if (state->minimum)
        {
          if (xvariant_compare (state->default_value, state->minimum) < 0 ||
              xvariant_compare (state->default_value, state->maximum) > 0)
            {
              g_set_error (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<%s> is not contained in "
                           "the specified range"), tag);
            }
        }

      else if (state->strinfo->len)
        {
          if (!is_valid_choices (state->default_value, state->strinfo))
            {
              if (state->is_enum)
                g_set_error (error, G_MARKUP_ERROR,
                             G_MARKUP_ERROR_INVALID_CONTENT,
                             _("<%s> is not a valid member of "
                             "the specified enumerated type"), tag);

              else if (state->is_flags)
                g_set_error (error, G_MARKUP_ERROR,
                             G_MARKUP_ERROR_INVALID_CONTENT,
                             _("<%s> contains string not in the "
                             "specified flags type"), tag);

              else
                g_set_error (error, G_MARKUP_ERROR,
                             G_MARKUP_ERROR_INVALID_CONTENT,
                             _("<%s> contains a string not in "
                             "<choices>"), tag);
            }
        }
    }
}

static void
key_state_set_range (KeyState     *state,
                     const xchar_t  *min_str,
                     const xchar_t  *max_str,
                     xerror_t      **error)
{
  const struct {
    const xchar_t  type;
    const xchar_t *min;
    const xchar_t *max;
  } table[] = {
    { 'y',                    "0",                  "255" },
    { 'n',               "-32768",                "32767" },
    { 'q',                    "0",                "65535" },
    { 'i',          "-2147483648",           "2147483647" },
    { 'u',                    "0",           "4294967295" },
    { 'x', "-9223372036854775808",  "9223372036854775807" },
    { 't',                    "0", "18446744073709551615" },
    { 'd',                 "-inf",                  "inf" },
  };
  xboolean_t type_ok = FALSE;
  xsize_t i;

  if (state->minimum)
    {
      g_set_error_literal (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<range/> already specified for this key"));
      return;
    }

  for (i = 0; i < G_N_ELEMENTS (table); i++)
    if (*(char *) state->type == table[i].type)
      {
        min_str = min_str ? min_str : table[i].min;
        max_str = max_str ? max_str : table[i].max;
        type_ok = TRUE;
        break;
      }

  if (!type_ok)
    {
      xchar_t *type = xvariant_type_dup_string (state->type);
      g_set_error (error, G_MARKUP_ERROR,
                  G_MARKUP_ERROR_INVALID_CONTENT,
                  _("<range> not allowed for keys of type “%s”"), type);
      g_free (type);
      return;
    }

  state->minimum = xvariant_parse (state->type, min_str, NULL, NULL, error);
  if (state->minimum == NULL)
    return;

  state->maximum = xvariant_parse (state->type, max_str, NULL, NULL, error);
  if (state->maximum == NULL)
    return;

  if (xvariant_compare (state->minimum, state->maximum) > 0)
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<range> specified minimum is greater than maximum"));
      return;
    }

  key_state_check_range (state, error);
}

static xstring_t *
key_state_start_default (KeyState     *state,
                         const xchar_t  *l10n,
                         const xchar_t  *context,
                         xerror_t      **error)
{
  if (l10n != NULL)
    {
      if (strcmp (l10n, "messages") == 0)
        state->l10n = 'm';

      else if (strcmp (l10n, "time") == 0)
        state->l10n = 't';

      else
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("unsupported l10n category: %s"), l10n);
          return NULL;
        }

      if (!state->have_gettext_domain)
        {
          g_set_error_literal (error, G_MARKUP_ERROR,
                               G_MARKUP_ERROR_INVALID_CONTENT,
                               _("l10n requested, but no "
                               "gettext domain given"));
          return NULL;
        }

      state->l10n_context = xstrdup (context);
    }

  else if (context != NULL)
    {
      g_set_error_literal (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("translation context given for "
                           "value without l10n enabled"));
      return NULL;
    }

  return xstring_new (NULL);
}

static void
key_state_end_default (KeyState  *state,
                       xstring_t  **string,
                       xerror_t   **error)
{
  state->unparsed_default_value = *string;
  *string = NULL;

  state->default_value = xvariant_parse (state->type,
                                          state->unparsed_default_value->str,
                                          NULL, NULL, error);
  if (!state->default_value)
    {
      xchar_t *type = xvariant_type_dup_string (state->type);
      g_prefix_error (error, _("Failed to parse <default> value of type “%s”: "), type);
      g_free (type);
    }

  key_state_check_range (state, error);
}

static void
key_state_start_choices (KeyState  *state,
                         xerror_t   **error)
{
  const xvariant_type_t *type = state->type;

  if (state->is_enum)
    {
      g_set_error_literal (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<choices> cannot be specified for keys "
                           "tagged as having an enumerated type"));
      return;
    }

  if (state->has_choices)
    {
      g_set_error_literal (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<choices> already specified for this key"));
      return;
    }

  while (xvariant_type_is_maybe (type) || xvariant_type_is_array (type))
    type = xvariant_type_element (type);

  if (!xvariant_type_equal (type, G_VARIANT_TYPE_STRING))
    {
      xchar_t *type_string = xvariant_type_dup_string (state->type);
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<choices> not allowed for keys of type “%s”"),
                   type_string);
      g_free (type_string);
      return;
    }
}

static void
key_state_add_choice (KeyState     *state,
                      const xchar_t  *choice,
                      xerror_t      **error)
{
  if (strinfo_builder_contains (state->strinfo, choice))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<choice value='%s'/> already given"), choice);
      return;
    }

  strinfo_builder_append_item (state->strinfo, choice, 0);
  state->has_choices = TRUE;
}

static void
key_state_end_choices (KeyState  *state,
                       xerror_t   **error)
{
  if (!state->has_choices)
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<choices> must contain at least one <choice>"));
      return;
    }

  key_state_check_range (state, error);
}

static void
key_state_start_aliases (KeyState  *state,
                         xerror_t   **error)
{
  if (state->has_aliases)
    g_set_error_literal (error, G_MARKUP_ERROR,
                         G_MARKUP_ERROR_INVALID_CONTENT,
                         _("<aliases> already specified for this key"));
  else if (!state->is_flags && !state->is_enum && !state->has_choices)
    g_set_error_literal (error, G_MARKUP_ERROR,
                         G_MARKUP_ERROR_INVALID_CONTENT,
                         _("<aliases> can only be specified for keys with "
                         "enumerated or flags types or after <choices>"));
}

static void
key_state_add_alias (KeyState     *state,
                     const xchar_t  *alias,
                     const xchar_t  *target,
                     xerror_t      **error)
{
  if (strinfo_builder_contains (state->strinfo, alias))
    {
      if (strinfo_is_string_valid ((xuint32_t *) state->strinfo->str,
                                   state->strinfo->len / 4,
                                   alias))
        {
          if (state->is_enum)
            g_set_error (error, G_MARKUP_ERROR,
                         G_MARKUP_ERROR_INVALID_CONTENT,
                         _("<alias value='%s'/> given when “%s” is already "
                         "a member of the enumerated type"), alias, alias);

          else
            g_set_error (error, G_MARKUP_ERROR,
                         G_MARKUP_ERROR_INVALID_CONTENT,
                         _("<alias value='%s'/> given when "
                         "<choice value='%s'/> was already given"),
                         alias, alias);
        }

      else
        g_set_error (error, G_MARKUP_ERROR,
                     G_MARKUP_ERROR_INVALID_CONTENT,
                     _("<alias value='%s'/> already specified"), alias);

      return;
    }

  if (!strinfo_builder_append_alias (state->strinfo, alias, target))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   state->is_enum ?
                     _("alias target “%s” is not in enumerated type") :
                     _("alias target “%s” is not in <choices>"),
                   target);
      return;
    }

  state->has_aliases = TRUE;
}

static void
key_state_end_aliases (KeyState  *state,
                       xerror_t   **error)
{
  if (!state->has_aliases)
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<aliases> must contain at least one <alias>"));
      return;
    }
}

static xboolean_t
key_state_check (KeyState  *state,
                 xerror_t   **error)
{
  if (state->checked)
    return TRUE;

  return state->checked = TRUE;
}

static xvariant_t *
key_state_serialise (KeyState *state)
{
  if (state->serialised == NULL)
    {
      if (state->child_schema)
        {
          state->serialised = xvariant_new_string (state->child_schema);
        }

      else
        {
          xvariant_builder_t builder;
          xboolean_t checked G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

          checked = key_state_check (state, NULL);
          g_assert (checked);

          xvariant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);

          /* default value */
          xvariant_builder_add_value (&builder, state->default_value);

          /* translation */
          if (state->l10n)
            {
              /* We are going to store the untranslated default for
               * runtime translation according to the current locale.
               * We need to strip leading and trailing whitespace from
               * the string so that it's exactly the same as the one
               * that ended up in the .po file for translation.
               *
               * We want to do this so that
               *
               *   <default l10n='messages'>
               *     ['a', 'b', 'c']
               *   </default>
               *
               * ends up in the .po file like "['a', 'b', 'c']",
               * omitting the extra whitespace at the start and end.
               */
              strip_string (state->unparsed_default_value);

              if (state->l10n_context)
                {
                  xint_t len;

                  /* Contextified messages are supported by prepending
                   * the context, followed by '\004' to the start of the
                   * message string.  We do that here to save xsettings_t
                   * the work later on.
                   */
                  len = strlen (state->l10n_context);
                  state->l10n_context[len] = '\004';
                  xstring_prepend_len (state->unparsed_default_value,
                                        state->l10n_context, len + 1);
                  g_free (state->l10n_context);
                  state->l10n_context = NULL;
                }

              xvariant_builder_add (&builder, "(y(y&s))", 'l', state->l10n,
                                     state->unparsed_default_value->str);
              xstring_free (state->unparsed_default_value, TRUE);
              state->unparsed_default_value = NULL;
            }

          /* choice, aliases, enums */
          if (state->strinfo->len)
            {
              xvariant_t *array;
              xuint32_t *words;
              xpointer_t data;
              xsize_t size;
              xsize_t i;

              data = state->strinfo->str;
              size = state->strinfo->len;

              words = data;
              for (i = 0; i < size / sizeof (xuint32_t); i++)
                words[i] = GUINT32_TO_LE (words[i]);

              array = xvariant_new_from_data (G_VARIANT_TYPE ("au"),
                                               data, size, TRUE,
                                               g_free, data);

              xstring_free (state->strinfo, FALSE);
              state->strinfo = NULL;

              xvariant_builder_add (&builder, "(y@au)",
                                     state->is_flags ? 'f' :
                                     state->is_enum ? 'e' : 'c',
                                     array);
            }

          /* range */
          if (state->minimum || state->maximum)
            xvariant_builder_add (&builder, "(y(**))", 'r',
                                   state->minimum, state->maximum);

          /* per-desktop overrides */
          if (state->desktop_overrides)
            xvariant_builder_add (&builder, "(y@a{sv})", 'd',
                                   xvariant_dict_end (state->desktop_overrides));

          state->serialised = xvariant_builder_end (&builder);
        }

      xvariant_ref_sink (state->serialised);
    }

  return xvariant_ref (state->serialised);
}

static void
key_state_free (xpointer_t data)
{
  KeyState *state = data;

  g_free (state->child_schema);

  if (state->type)
    xvariant_type_free (state->type);

  g_free (state->l10n_context);

  if (state->unparsed_default_value)
    xstring_free (state->unparsed_default_value, TRUE);

  if (state->default_value)
    xvariant_unref (state->default_value);

  if (state->strinfo)
    xstring_free (state->strinfo, TRUE);

  if (state->minimum)
    xvariant_unref (state->minimum);

  if (state->maximum)
    xvariant_unref (state->maximum);

  if (state->serialised)
    xvariant_unref (state->serialised);

  if (state->desktop_overrides)
    xvariant_dict_unref (state->desktop_overrides);

  g_slice_free (KeyState, state);
}

/* Key name validity {{{1 */
static xboolean_t allow_any_name = FALSE;

static xboolean_t
is_valid_keyname (const xchar_t  *key,
                  xerror_t      **error)
{
  xint_t i;

  if (key[0] == '\0')
    {
      g_set_error_literal (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                           _("Empty names are not permitted"));
      return FALSE;
    }

  if (allow_any_name)
    return TRUE;

  if (!g_ascii_islower (key[0]))
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("Invalid name “%s”: names must begin "
                     "with a lowercase letter"), key);
      return FALSE;
    }

  for (i = 1; key[i]; i++)
    {
      if (key[i] != '-' &&
          !g_ascii_islower (key[i]) &&
          !g_ascii_isdigit (key[i]))
        {
          g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Invalid name “%s”: invalid character “%c”; "
                         "only lowercase letters, numbers and hyphen (“-”) "
                         "are permitted"), key, key[i]);
          return FALSE;
        }

      if (key[i] == '-' && key[i + 1] == '-')
        {
          g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Invalid name “%s”: two successive hyphens (“--”) "
                         "are not permitted"), key);
          return FALSE;
        }
    }

  if (key[i - 1] == '-')
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("Invalid name “%s”: the last character may not be a "
                     "hyphen (“-”)"), key);
      return FALSE;
    }

  if (i > 1024)
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("Invalid name “%s”: maximum length is 1024"), key);
      return FALSE;
    }

  return TRUE;
}

/* Handling of <schema> {{{1 */
typedef struct _SchemaState SchemaState;
struct _SchemaState
{
  SchemaState *extends;

  xchar_t       *path;
  xchar_t       *gettext_domain;
  xchar_t       *extends_name;
  xchar_t       *list_of;

  xhashtable_t  *keys;
};

static SchemaState *
schema_state_new (const xchar_t  *path,
                  const xchar_t  *gettext_domain,
                  SchemaState  *extends,
                  const xchar_t  *extends_name,
                  const xchar_t  *list_of)
{
  SchemaState *state;

  state = g_slice_new (SchemaState);
  state->path = xstrdup (path);
  state->gettext_domain = xstrdup (gettext_domain);
  state->extends = extends;
  state->extends_name = xstrdup (extends_name);
  state->list_of = xstrdup (list_of);
  state->keys = xhash_table_new_full (xstr_hash, xstr_equal,
                                       g_free, key_state_free);

  return state;
}

static void
schema_state_free (xpointer_t data)
{
  SchemaState *state = data;

  g_free (state->path);
  g_free (state->gettext_domain);
  g_free (state->extends_name);
  g_free (state->list_of);
  xhash_table_unref (state->keys);
  g_slice_free (SchemaState, state);
}

static void
schema_state_add_child (SchemaState  *state,
                        const xchar_t  *name,
                        const xchar_t  *schema,
                        xerror_t      **error)
{
  xchar_t *childname;

  if (!is_valid_keyname (name, error))
    return;

  childname = xstrconcat (name, "/", NULL);

  if (xhash_table_lookup (state->keys, childname))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<child name='%s'> already specified"), name);
      return;
    }

  xhash_table_insert (state->keys, childname,
                       key_state_new_child (schema));
}

static KeyState *
schema_state_add_key (SchemaState  *state,
                      xhashtable_t   *enum_table,
                      xhashtable_t   *flags_table,
                      const xchar_t  *name,
                      const xchar_t  *type_string,
                      const xchar_t  *enum_type,
                      const xchar_t  *flags_type,
                      xerror_t      **error)
{
  SchemaState *node;
  xstring_t *strinfo;
  KeyState *key;

  if (state->list_of)
    {
      g_set_error_literal (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("Cannot add keys to a “list-of” schema"));
      return NULL;
    }

  if (!is_valid_keyname (name, error))
    return NULL;

  if (xhash_table_lookup (state->keys, name))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<key name='%s'> already specified"), name);
      return NULL;
    }

  for (node = state; node; node = node->extends)
    if (node->extends)
      {
        KeyState *shadow;

        shadow = xhash_table_lookup (node->extends->keys, name);

        /* in case of <key> <override> <key> make sure we report the
         * location of the original <key>, not the <override>.
         */
        if (shadow && !shadow->is_override)
          {
            g_set_error (error, G_MARKUP_ERROR,
                         G_MARKUP_ERROR_INVALID_CONTENT,
                         _("<key name='%s'> shadows <key name='%s'> in "
                           "<schema id='%s'>; use <override> to modify value"),
                         name, name, node->extends_name);
            return NULL;
          }
      }

  if ((type_string != NULL) + (enum_type != NULL) + (flags_type != NULL) != 1)
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_MISSING_ATTRIBUTE,
                   _("Exactly one of “type”, “enum” or “flags” must "
                     "be specified as an attribute to <key>"));
      return NULL;
    }

  if (type_string == NULL) /* flags or enums was specified */
    {
      EnumState *enum_state;

      if (enum_type)
        enum_state = xhash_table_lookup (enum_table, enum_type);
      else
        enum_state = xhash_table_lookup (flags_table, flags_type);


      if (enum_state == NULL)
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("<%s id='%s'> not (yet) defined."),
                       flags_type ? "flags"    : "enum",
                       flags_type ? flags_type : enum_type);
          return NULL;
        }

      type_string = flags_type ? "as" : "s";
      strinfo = enum_state->strinfo;
    }
  else
    {
      if (!xvariant_type_string_is_valid (type_string))
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Invalid xvariant_t type string “%s”"), type_string);
          return NULL;
        }

      strinfo = NULL;
    }

  key = key_state_new (type_string, state->gettext_domain,
                       enum_type != NULL, flags_type != NULL, strinfo);
  xhash_table_insert (state->keys, xstrdup (name), key);

  return key;
}

static void
schema_state_add_override (SchemaState  *state,
                           KeyState    **key_state,
                           xstring_t     **string,
                           const xchar_t  *key,
                           const xchar_t  *l10n,
                           const xchar_t  *context,
                           xerror_t      **error)
{
  SchemaState *parent;
  KeyState *original;

  if (state->extends == NULL)
    {
      g_set_error_literal (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<override> given but schema isn’t "
                             "extending anything"));
      return;
    }

  for (parent = state->extends; parent; parent = parent->extends)
    if ((original = xhash_table_lookup (parent->keys, key)))
      break;

  if (original == NULL)
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("No <key name='%s'> to override"), key);
      return;
    }

  if (xhash_table_lookup (state->keys, key))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<override name='%s'> already specified"), key);
      return;
    }

  *key_state = key_state_override (original, state->gettext_domain);
  *string = key_state_start_default (*key_state, l10n, context, error);
  xhash_table_insert (state->keys, xstrdup (key), *key_state);
}

static void
override_state_end (KeyState **key_state,
                    xstring_t  **string,
                    xerror_t   **error)
{
  key_state_end_default (*key_state, string, error);
  *key_state = NULL;
}

/* Handling of toplevel state {{{1 */
typedef struct
{
  xboolean_t     strict;                  /* TRUE if --strict was given */

  xhashtable_t  *schema_table;            /* string -> SchemaState */
  xhashtable_t  *flags_table;             /* string -> EnumState */
  xhashtable_t  *enum_table;              /* string -> EnumState */

  xslist_t      *this_file_schemas;       /* strings: <schema>s in this file */
  xslist_t      *this_file_flagss;        /* strings: <flags>s in this file */
  xslist_t      *this_file_enums;         /* strings: <enum>s in this file */

  xchar_t       *schemalist_domain;       /* the <schemalist> gettext domain */

  SchemaState *schema_state;            /* non-NULL when inside <schema> */
  KeyState    *key_state;               /* non-NULL when inside <key> */
  EnumState   *enum_state;              /* non-NULL when inside <enum> */

  xstring_t     *string;                  /* non-NULL when accepting text */
} ParseState;

static xboolean_t
is_subclass (const xchar_t *class_name,
             const xchar_t *possible_parent,
             xhashtable_t  *schema_table)
{
  SchemaState *class;

  if (strcmp (class_name, possible_parent) == 0)
    return TRUE;

  class = xhash_table_lookup (schema_table, class_name);
  g_assert (class != NULL);

  return class->extends_name &&
         is_subclass (class->extends_name, possible_parent, schema_table);
}

static void
parse_state_start_schema (ParseState  *state,
                          const xchar_t  *id,
                          const xchar_t  *path,
                          const xchar_t  *gettext_domain,
                          const xchar_t  *extends_name,
                          const xchar_t  *list_of,
                          xerror_t      **error)
{
  SchemaState *extends;
  xchar_t *my_id;

  if (xhash_table_lookup (state->schema_table, id))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<schema id='%s'> already specified"), id);
      return;
    }

  if (extends_name)
    {
      extends = xhash_table_lookup (state->schema_table, extends_name);

      if (extends == NULL)
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("<schema id='%s'> extends not yet existing "
                         "schema “%s”"), id, extends_name);
          return;
        }
    }
  else
    extends = NULL;

  if (list_of)
    {
      SchemaState *tmp;

      if (!(tmp = xhash_table_lookup (state->schema_table, list_of)))
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       _("<schema id='%s'> is list of not yet existing "
                         "schema “%s”"), id, list_of);
          return;
        }

      if (tmp->path)
        {
          g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Cannot be a list of a schema with a path"));
          return;
        }
    }

  if (extends)
    {
      if (extends->path)
        {
          g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                       _("Cannot extend a schema with a path"));
          return;
        }

      if (list_of)
        {
          if (extends->list_of == NULL)
            {
              g_set_error (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<schema id='%s'> is a list, extending "
                             "<schema id='%s'> which is not a list"),
                           id, extends_name);
              return;
            }

          if (!is_subclass (list_of, extends->list_of, state->schema_table))
            {
              g_set_error (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           _("<schema id='%s' list-of='%s'> extends <schema "
                             "id='%s' list-of='%s'> but “%s” does not "
                             "extend “%s”"), id, list_of, extends_name,
                           extends->list_of, list_of, extends->list_of);
              return;
            }
        }
      else
        /* by default we are a list of the same thing that the schema
         * we are extending is a list of (which might be nothing)
         */
        list_of = extends->list_of;
    }

  if (path && !(xstr_has_prefix (path, "/") && xstr_has_suffix (path, "/")))
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("A path, if given, must begin and end with a slash"));
      return;
    }

  if (path && list_of && !xstr_has_suffix (path, ":/"))
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   _("The path of a list must end with “:/”"));
      return;
    }

  if (path && (xstr_has_prefix (path, "/apps/") ||
               xstr_has_prefix (path, "/desktop/") ||
               xstr_has_prefix (path, "/system/")))
    {
      xchar_t *message = NULL;
      message = xstrdup_printf (_("Warning: Schema “%s” has path “%s”.  "
                                   "Paths starting with "
                                   "“/apps/”, “/desktop/” or “/system/” are deprecated."),
                                 id, path);
      g_printerr ("%s\n", message);
      g_free (message);
    }

  state->schema_state = schema_state_new (path, gettext_domain,
                                          extends, extends_name, list_of);

  my_id = xstrdup (id);
  state->this_file_schemas = xslist_prepend (state->this_file_schemas, my_id);
  xhash_table_insert (state->schema_table, my_id, state->schema_state);
}

static void
parse_state_start_enum (ParseState   *state,
                        const xchar_t  *id,
                        xboolean_t      is_flags,
                        xerror_t      **error)
{
  xslist_t **list = is_flags ? &state->this_file_flagss : &state->this_file_enums;
  xhashtable_t *table = is_flags ? state->flags_table : state->enum_table;
  xchar_t *my_id;

  if (xhash_table_lookup (table, id))
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("<%s id='%s'> already specified"),
                   is_flags ? "flags" : "enum", id);
      return;
    }

  state->enum_state = enum_state_new (is_flags);

  my_id = xstrdup (id);
  *list = xslist_prepend (*list, my_id);
  xhash_table_insert (table, my_id, state->enum_state);
}

/* GMarkup Parser Functions {{{1 */

/* Start element {{{2 */
static void
start_element (xmarkup_parse_context_t  *context,
               const xchar_t          *element_name,
               const xchar_t         **attribute_names,
               const xchar_t         **attribute_values,
               xpointer_t              user_data,
               xerror_t              **error)
{
  ParseState *state = user_data;
  const xslist_t *element_stack;
  const xchar_t *container;

  element_stack = xmarkup_parse_context_get_element_stack (context);
  container = element_stack->next ? element_stack->next->data : NULL;

#define COLLECT(first, ...) \
  g_markup_collect_attributes (element_name,                                 \
                               attribute_names, attribute_values, error,     \
                               first, __VA_ARGS__, G_MARKUP_COLLECT_INVALID)
#define OPTIONAL   G_MARKUP_COLLECT_OPTIONAL
#define STRDUP     G_MARKUP_COLLECT_STRDUP
#define STRING     G_MARKUP_COLLECT_STRING
#define NO_ATTRS()  COLLECT (G_MARKUP_COLLECT_INVALID, NULL)

  /* Toplevel items {{{3 */
  if (container == NULL)
    {
      if (strcmp (element_name, "schemalist") == 0)
        {
          COLLECT (OPTIONAL | STRDUP,
                   "gettext-domain",
                   &state->schemalist_domain);
          return;
        }
    }


  /* children of <schemalist> {{{3 */
  else if (strcmp (container, "schemalist") == 0)
    {
      if (strcmp (element_name, "schema") == 0)
        {
          const xchar_t *id, *path, *gettext_domain, *extends, *list_of;
          if (COLLECT (STRING, "id", &id,
                       OPTIONAL | STRING, "path", &path,
                       OPTIONAL | STRING, "gettext-domain", &gettext_domain,
                       OPTIONAL | STRING, "extends", &extends,
                       OPTIONAL | STRING, "list-of", &list_of))
            parse_state_start_schema (state, id, path,
                                      gettext_domain ? gettext_domain
                                                     : state->schemalist_domain,
                                      extends, list_of, error);
          return;
        }

      else if (strcmp (element_name, "enum") == 0)
        {
          const xchar_t *id;
          if (COLLECT (STRING, "id", &id))
            parse_state_start_enum (state, id, FALSE, error);
          return;
        }

      else if (strcmp (element_name, "flags") == 0)
        {
          const xchar_t *id;
          if (COLLECT (STRING, "id", &id))
            parse_state_start_enum (state, id, TRUE, error);
          return;
        }
    }


  /* children of <schema> {{{3 */
  else if (strcmp (container, "schema") == 0)
    {
      if (strcmp (element_name, "key") == 0)
        {
          const xchar_t *name, *type_string, *enum_type, *flags_type;

          if (COLLECT (STRING,            "name",  &name,
                       OPTIONAL | STRING, "type",  &type_string,
                       OPTIONAL | STRING, "enum",  &enum_type,
                       OPTIONAL | STRING, "flags", &flags_type))

            state->key_state = schema_state_add_key (state->schema_state,
                                                     state->enum_table,
                                                     state->flags_table,
                                                     name, type_string,
                                                     enum_type, flags_type,
                                                     error);
          return;
        }
      else if (strcmp (element_name, "child") == 0)
        {
          const xchar_t *name, *schema;

          if (COLLECT (STRING, "name", &name, STRING, "schema", &schema))
            schema_state_add_child (state->schema_state,
                                    name, schema, error);
          return;
        }
      else if (strcmp (element_name, "override") == 0)
        {
          const xchar_t *name, *l10n, *context;

          if (COLLECT (STRING,            "name",    &name,
                       OPTIONAL | STRING, "l10n",    &l10n,
                       OPTIONAL | STRING, "context", &context))
            schema_state_add_override (state->schema_state,
                                       &state->key_state, &state->string,
                                       name, l10n, context, error);
          return;
        }
    }

  /* children of <key> {{{3 */
  else if (strcmp (container, "key") == 0)
    {
      if (strcmp (element_name, "default") == 0)
        {
          const xchar_t *l10n, *context;
          if (COLLECT (STRING | OPTIONAL, "l10n",    &l10n,
                       STRING | OPTIONAL, "context", &context))
            state->string = key_state_start_default (state->key_state,
                                                     l10n, context, error);
          return;
        }

      else if (strcmp (element_name, "summary") == 0)
        {
          if (NO_ATTRS ())
            {
              if (state->key_state->summary_seen && state->strict)
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                             _("Only one <%s> element allowed inside <%s>"),
                             element_name, container);
              else
                state->string = xstring_new (NULL);

              state->key_state->summary_seen = TRUE;
            }
          return;
        }

      else if (strcmp (element_name, "description") == 0)
        {
          if (NO_ATTRS ())
            {
              if (state->key_state->description_seen && state->strict)
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                             _("Only one <%s> element allowed inside <%s>"),
                             element_name, container);
              else
                state->string = xstring_new (NULL);

            state->key_state->description_seen = TRUE;
            }
          return;
        }

      else if (strcmp (element_name, "range") == 0)
        {
          const xchar_t *min, *max;
          if (COLLECT (STRING | OPTIONAL, "min", &min,
                       STRING | OPTIONAL, "max", &max))
            key_state_set_range (state->key_state, min, max, error);
          return;
        }

      else if (strcmp (element_name, "choices") == 0)
        {
          if (NO_ATTRS ())
            key_state_start_choices (state->key_state, error);
          return;
        }

      else if (strcmp (element_name, "aliases") == 0)
        {
          if (NO_ATTRS ())
            key_state_start_aliases (state->key_state, error);
          return;
        }
    }


  /* children of <choices> {{{3 */
  else if (strcmp (container, "choices") == 0)
    {
      if (strcmp (element_name, "choice") == 0)
        {
          const xchar_t *value;
          if (COLLECT (STRING, "value", &value))
            key_state_add_choice (state->key_state, value, error);
          return;
        }
    }


  /* children of <aliases> {{{3 */
  else if (strcmp (container, "aliases") == 0)
    {
      if (strcmp (element_name, "alias") == 0)
        {
          const xchar_t *value, *target;
          if (COLLECT (STRING, "value", &value, STRING, "target", &target))
            key_state_add_alias (state->key_state, value, target, error);
          return;
        }
    }


  /* children of <enum> {{{3 */
  else if (strcmp (container, "enum") == 0 ||
           strcmp (container, "flags") == 0)
    {
      if (strcmp (element_name, "value") == 0)
        {
          const xchar_t *nick, *valuestr;
          if (COLLECT (STRING, "nick", &nick,
                       STRING, "value", &valuestr))
            enum_state_add_value (state->enum_state, nick, valuestr, error);
          return;
        }
    }
  /* 3}}} */

  if (container)
    g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                 _("Element <%s> not allowed inside <%s>"),
                 element_name, container);
  else
    g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                 _("Element <%s> not allowed at the top level"), element_name);
}
/* 2}}} */
/* End element {{{2 */

static void
key_state_end (KeyState **state_ptr,
               xerror_t   **error)
{
  KeyState *state;

  state = *state_ptr;
  *state_ptr = NULL;

  if (state->default_value == NULL)
    {
      g_set_error_literal (error,
                           G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                           _("Element <default> is required in <key>"));
      return;
    }
}

static void
schema_state_end (SchemaState **state_ptr,
                  xerror_t      **error)
{
  *state_ptr = NULL;
}

static void
end_element (xmarkup_parse_context_t  *context,
             const xchar_t          *element_name,
             xpointer_t              user_data,
             xerror_t              **error)
{
  ParseState *state = user_data;

  if (strcmp (element_name, "schemalist") == 0)
    {
      g_free (state->schemalist_domain);
      state->schemalist_domain = NULL;
    }

  else if (strcmp (element_name, "enum") == 0 ||
           strcmp (element_name, "flags") == 0)
    enum_state_end (&state->enum_state, error);

  else if (strcmp (element_name, "schema") == 0)
    schema_state_end (&state->schema_state, error);

  else if (strcmp (element_name, "override") == 0)
    override_state_end (&state->key_state, &state->string, error);

  else if (strcmp (element_name, "key") == 0)
    key_state_end (&state->key_state, error);

  else if (strcmp (element_name, "default") == 0)
    key_state_end_default (state->key_state, &state->string, error);

  else if (strcmp (element_name, "choices") == 0)
    key_state_end_choices (state->key_state, error);

  else if (strcmp (element_name, "aliases") == 0)
    key_state_end_aliases (state->key_state, error);

  if (state->string)
    {
      xstring_free (state->string, TRUE);
      state->string = NULL;
    }
}
/* Text {{{2 */
static void
text (xmarkup_parse_context_t  *context,
      const xchar_t          *text,
      xsize_t                 text_len,
      xpointer_t              user_data,
      xerror_t              **error)
{
  ParseState *state = user_data;

  if (state->string)
    {
      /* we are expecting a string, so store the text data.
       *
       * we store the data verbatim here and deal with whitespace
       * later on.  there are two reasons for that:
       *
       *  1) whitespace is handled differently depending on the tag
       *     type.
       *
       *  2) we could do leading whitespace removal by refusing to
       *     insert it into state->string if it's at the start, but for
       *     trailing whitespace, we have no idea if there is another
       *     text() call coming or not.
       */
      xstring_append_len (state->string, text, text_len);
    }
  else
    {
      /* string is not expected: accept (and ignore) pure whitespace */
      xsize_t i;

      for (i = 0; i < text_len; i++)
        if (!g_ascii_isspace (text[i]))
          {
            g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                         _("Text may not appear inside <%s>"),
                         xmarkup_parse_context_get_element (context));
            break;
          }
    }
}

/* Write to GVDB {{{1 */
typedef struct
{
  xhashtable_t *table;
  GvdbItem *root;
} GvdbPair;

static void
gvdb_pair_init (GvdbPair *pair)
{
  pair->table = gvdb_hash_table_new (NULL, NULL);
  pair->root = gvdb_hash_table_insert (pair->table, "");
}

static void
gvdb_pair_clear (GvdbPair *pair)
{
  xhash_table_unref (pair->table);
}

typedef struct
{
  xhashtable_t *schema_table;
  GvdbPair root_pair;
} WriteToFileData;

typedef struct
{
  xhashtable_t *schema_table;
  GvdbPair pair;
  xboolean_t l10n;
} OutputSchemaData;

static void
output_key (xpointer_t key,
            xpointer_t value,
            xpointer_t user_data)
{
  OutputSchemaData *data;
  const xchar_t *name;
  KeyState *state;
  GvdbItem *item;
  xvariant_t *serialised = NULL;

  name = key;
  state = value;
  data = user_data;

  item = gvdb_hash_table_insert (data->pair.table, name);
  gvdb_item_set_parent (item, data->pair.root);
  serialised = key_state_serialise (state);
  gvdb_item_set_value (item, serialised);
  xvariant_unref (serialised);

  if (state->l10n)
    data->l10n = TRUE;

  if (state->child_schema &&
      !xhash_table_lookup (data->schema_table, state->child_schema))
    {
      xchar_t *message = NULL;
      message = xstrdup_printf (_("Warning: undefined reference to <schema id='%s'/>"),
                                 state->child_schema);
      g_printerr ("%s\n", message);
      g_free (message);
    }
}

static void
output_schema (xpointer_t key,
               xpointer_t value,
               xpointer_t user_data)
{
  WriteToFileData *wtf_data = user_data;
  OutputSchemaData data;
  GvdbPair *root_pair;
  SchemaState *state;
  const xchar_t *id;
  GvdbItem *item;

  id = key;
  state = value;
  root_pair = &wtf_data->root_pair;

  data.schema_table = wtf_data->schema_table;
  gvdb_pair_init (&data.pair);
  data.l10n = FALSE;

  item = gvdb_hash_table_insert (root_pair->table, id);
  gvdb_item_set_parent (item, root_pair->root);
  gvdb_item_set_hash_table (item, data.pair.table);

  xhash_table_foreach (state->keys, output_key, &data);

  if (state->path)
    gvdb_hash_table_insert_string (data.pair.table, ".path", state->path);

  if (state->extends_name)
    gvdb_hash_table_insert_string (data.pair.table, ".extends",
                                   state->extends_name);

  if (state->list_of)
    gvdb_hash_table_insert_string (data.pair.table, ".list-of",
                                   state->list_of);

  if (data.l10n)
    gvdb_hash_table_insert_string (data.pair.table,
                                   ".gettext-domain",
                                   state->gettext_domain);

  gvdb_pair_clear (&data.pair);
}

static xboolean_t
write_to_file (xhashtable_t   *schema_table,
               const xchar_t  *filename,
               xerror_t      **error)
{
  WriteToFileData data;
  xboolean_t success;

  data.schema_table = schema_table;

  gvdb_pair_init (&data.root_pair);

  xhash_table_foreach (schema_table, output_schema, &data);

  success = gvdb_table_write_contents (data.root_pair.table, filename,
                                       G_BYTE_ORDER != G_LITTLE_ENDIAN,
                                       error);
  xhash_table_unref (data.root_pair.table);

  return success;
}

/* Parser driver {{{1 */
static xhashtable_t *
parse_gschema_files (xchar_t    **files,
                     xboolean_t   strict)
{
  GMarkupParser parser = { start_element, end_element, text, NULL, NULL };
  ParseState state = { 0, };
  const xchar_t *filename;
  xerror_t *error = NULL;

  state.strict = strict;

  state.enum_table = xhash_table_new_full (xstr_hash, xstr_equal,
                                            g_free, enum_state_free);

  state.flags_table = xhash_table_new_full (xstr_hash, xstr_equal,
                                             g_free, enum_state_free);

  state.schema_table = xhash_table_new_full (xstr_hash, xstr_equal,
                                              g_free, schema_state_free);

  while ((filename = *files++) != NULL)
    {
      xmarkup_parse_context_t *context;
      xchar_t *contents;
      xsize_t size;
      xint_t line, col;

      if (!xfile_get_contents (filename, &contents, &size, &error))
        {
          fprintf (stderr, "%s\n", error->message);
          g_clear_error (&error);
          continue;
        }

      context = xmarkup_parse_context_new (&parser,
                                            G_MARKUP_TREAT_CDATA_AS_TEXT |
                                            G_MARKUP_PREFIX_ERROR_POSITION |
                                            G_MARKUP_IGNORE_QUALIFIED,
                                            &state, NULL);


      if (!xmarkup_parse_context_parse (context, contents, size, &error) ||
          !xmarkup_parse_context_end_parse (context, &error))
        {
          xslist_t *item;

          /* back out any changes from this file */
          for (item = state.this_file_schemas; item; item = item->next)
            xhash_table_remove (state.schema_table, item->data);

          for (item = state.this_file_flagss; item; item = item->next)
            xhash_table_remove (state.flags_table, item->data);

          for (item = state.this_file_enums; item; item = item->next)
            xhash_table_remove (state.enum_table, item->data);

          /* let them know */
          xmarkup_parse_context_get_position (context, &line, &col);
          fprintf (stderr, "%s:%d:%d  %s.  ", filename, line, col, error->message);
          g_clear_error (&error);

          if (strict)
            {
              /* Translators: Do not translate "--strict". */
              fprintf (stderr, "%s\n", _("--strict was specified; exiting."));

              xhash_table_unref (state.schema_table);
              xhash_table_unref (state.flags_table);
              xhash_table_unref (state.enum_table);

              g_free (contents);

              return NULL;
            }
          else
            {
              fprintf (stderr, "%s\n", _("This entire file has been ignored."));
            }
        }

      /* cleanup */
      g_free (contents);
      xmarkup_parse_context_free (context);
      xslist_free (state.this_file_schemas);
      xslist_free (state.this_file_flagss);
      xslist_free (state.this_file_enums);
      state.this_file_schemas = NULL;
      state.this_file_flagss = NULL;
      state.this_file_enums = NULL;
    }

  xhash_table_unref (state.flags_table);
  xhash_table_unref (state.enum_table);

  return state.schema_table;
}

static xint_t
compare_strings (xconstpointer a,
                 xconstpointer b)
{
  xchar_t *one = *(xchar_t **) a;
  xchar_t *two = *(xchar_t **) b;
  xint_t cmp;

  cmp = xstr_has_suffix (two, ".enums.xml") -
        xstr_has_suffix (one, ".enums.xml");

  if (!cmp)
    cmp = strcmp (one, two);

  return cmp;
}

static xboolean_t
set_overrides (xhashtable_t  *schema_table,
               xchar_t      **files,
               xboolean_t     strict)
{
  const xchar_t *filename;
  xerror_t *error = NULL;

  while ((filename = *files++))
    {
      xkey_file_t *key_file;
      xchar_t **groups;
      xint_t i;

      g_debug ("Processing override file '%s'", filename);

      key_file = xkey_file_new ();
      if (!xkey_file_load_from_file (key_file, filename, 0, &error))
        {
          fprintf (stderr, "%s: %s.  ", filename, error->message);
          xkey_file_free (key_file);
          g_clear_error (&error);

          if (!strict)
            {
              fprintf (stderr, "%s\n", _("Ignoring this file."));
              continue;
            }

          fprintf (stderr, "%s\n", _("--strict was specified; exiting."));
          return FALSE;
        }

      groups = xkey_file_get_groups (key_file, NULL);

      for (i = 0; groups[i]; i++)
        {
          const xchar_t *group = groups[i];
          const xchar_t *schema_name;
          const xchar_t *desktop_id;
          SchemaState *schema;
          xchar_t **pieces;
          xchar_t **keys;
          xint_t j;

          pieces = xstrsplit (group, ":", 2);
          schema_name = pieces[0];
          desktop_id = pieces[1];

          g_debug ("Processing group '%s' (schema '%s', %s)",
                   group, schema_name, desktop_id ? desktop_id : "all desktops");

          schema = xhash_table_lookup (schema_table, schema_name);

          if (schema == NULL)
            {
              /* Having the schema not be installed is expected to be a
               * common case.  Don't even emit an error message about
               * that.
               */
              xstrfreev (pieces);
              continue;
            }

          keys = xkey_file_get_keys (key_file, group, NULL, NULL);
          g_assert (keys != NULL);

          for (j = 0; keys[j]; j++)
            {
              const xchar_t *key = keys[j];
              KeyState *state;
              xvariant_t *value;
              xchar_t *string;

              state = xhash_table_lookup (schema->keys, key);

              if (state == NULL)
                {
                  if (!strict)
                    {
                      fprintf (stderr, _("No such key “%s” in schema “%s” as "
                                         "specified in override file “%s”; "
                                         "ignoring override for this key."),
                               key, group, filename);
                      fprintf (stderr, "\n");
                      continue;
                    }

                  fprintf (stderr, _("No such key “%s” in schema “%s” as "
                                     "specified in override file “%s” and "
                                     "--strict was specified; exiting."),
                           key, group, filename);
                  fprintf (stderr, "\n");

                  xkey_file_free (key_file);
                  xstrfreev (pieces);
                  xstrfreev (groups);
                  xstrfreev (keys);

                  return FALSE;
                }

              if (desktop_id != NULL && state->l10n)
                {
                  /* Let's avoid the n*m case of per-desktop localised
                   * default values, and just forbid it.
                   */
                  if (!strict)
                    {
                      fprintf (stderr,
                               _("Cannot provide per-desktop overrides for "
                                 "localized key “%s” in schema “%s” (override "
                                 "file “%s”); ignoring override for this key."),
                           key, group, filename);
                      fprintf (stderr, "\n");
                      continue;
                    }

                  fprintf (stderr,
                           _("Cannot provide per-desktop overrides for "
                             "localized key “%s” in schema “%s” (override "
                             "file “%s”) and --strict was specified; exiting."),
                           key, group, filename);
                  fprintf (stderr, "\n");

                  xkey_file_free (key_file);
                  xstrfreev (pieces);
                  xstrfreev (groups);
                  xstrfreev (keys);

                  return FALSE;
                }

              string = xkey_file_get_value (key_file, group, key, NULL);
              g_assert (string != NULL);

              value = xvariant_parse (state->type, string,
                                       NULL, NULL, &error);

              if (value == NULL)
                {
                  if (!strict)
                    {
                      fprintf (stderr, _("Error parsing key “%s” in schema “%s” "
                                         "as specified in override file “%s”: "
                                         "%s. Ignoring override for this key."),
                               key, group, filename, error->message);
                      fprintf (stderr, "\n");

                      g_clear_error (&error);
                      g_free (string);

                      continue;
                    }

                  fprintf (stderr, _("Error parsing key “%s” in schema “%s” "
                                     "as specified in override file “%s”: "
                                     "%s. --strict was specified; exiting."),
                           key, group, filename, error->message);
                  fprintf (stderr, "\n");

                  g_clear_error (&error);
                  g_free (string);
                  xkey_file_free (key_file);
                  xstrfreev (pieces);
                  xstrfreev (groups);
                  xstrfreev (keys);

                  return FALSE;
                }

              if (state->minimum)
                {
                  if (xvariant_compare (value, state->minimum) < 0 ||
                      xvariant_compare (value, state->maximum) > 0)
                    {
                      xvariant_unref (value);
                      g_free (string);

                      if (!strict)
                        {
                          fprintf (stderr,
                                   _("Override for key “%s” in schema “%s” in "
                                     "override file “%s” is outside the range "
                                     "given in the schema; ignoring override "
                                     "for this key."),
                                   key, group, filename);
                          fprintf (stderr, "\n");
                          continue;
                        }

                      fprintf (stderr,
                               _("Override for key “%s” in schema “%s” in "
                                 "override file “%s” is outside the range "
                                 "given in the schema and --strict was "
                                 "specified; exiting."),
                               key, group, filename);
                      fprintf (stderr, "\n");

                      xkey_file_free (key_file);
                      xstrfreev (pieces);
                      xstrfreev (groups);
                      xstrfreev (keys);

                      return FALSE;
                    }
                }

              else if (state->strinfo->len)
                {
                  if (!is_valid_choices (value, state->strinfo))
                    {
                      xvariant_unref (value);
                      g_free (string);

                      if (!strict)
                        {
                          fprintf (stderr,
                                   _("Override for key “%s” in schema “%s” in "
                                     "override file “%s” is not in the list "
                                     "of valid choices; ignoring override for "
                                     "this key."),
                                   key, group, filename);
                          fprintf (stderr, "\n");
                          continue;
                        }

                      fprintf (stderr,
                               _("Override for key “%s” in schema “%s” in "
                                 "override file “%s” is not in the list "
                                 "of valid choices and --strict was specified; "
                                 "exiting."),
                               key, group, filename);
                      fprintf (stderr, "\n");
                      xkey_file_free (key_file);
                      xstrfreev (pieces);
                      xstrfreev (groups);
                      xstrfreev (keys);

                      return FALSE;
                    }
                }

              if (desktop_id != NULL)
                {
                  if (state->desktop_overrides == NULL)
                    state->desktop_overrides = xvariant_dict_new (NULL);

                  xvariant_dict_insert_value (state->desktop_overrides, desktop_id, value);
                  xvariant_unref (value);
                }
              else
                {
                  xvariant_unref (state->default_value);
                  state->default_value = value;
                }

              g_free (string);
            }

          xstrfreev (pieces);
          xstrfreev (keys);
        }

      xstrfreev (groups);
      xkey_file_free (key_file);
    }

  return TRUE;
}

int
main (int argc, char **argv)
{
  xerror_t *error = NULL;
  xhashtable_t *table = NULL;
  xdir_t *dir = NULL;
  const xchar_t *file;
  const xchar_t *srcdir;
  xboolean_t show_version_and_exit = FALSE;
  xchar_t *targetdir = NULL;
  xchar_t *target = NULL;
  xboolean_t dry_run = FALSE;
  xboolean_t strict = FALSE;
  xchar_t **schema_files = NULL;
  xchar_t **override_files = NULL;
  xoption_context_t *context = NULL;
  xint_t retval;
  GOptionEntry entries[] = {
    { "version", 0, 0, G_OPTION_ARG_NONE, &show_version_and_exit, N_("Show program version and exit"), NULL },
    { "targetdir", 0, 0, G_OPTION_ARXFILENAME, &targetdir, N_("Where to store the gschemas.compiled file"), N_("DIRECTORY") },
    { "strict", 0, 0, G_OPTION_ARG_NONE, &strict, N_("Abort on any errors in schemas"), NULL },
    { "dry-run", 0, 0, G_OPTION_ARG_NONE, &dry_run, N_("Do not write the gschema.compiled file"), NULL },
    { "allow-any-name", 0, 0, G_OPTION_ARG_NONE, &allow_any_name, N_("Do not enforce key name restrictions"), NULL },

    /* These options are only for use in the gschema-compile tests */
    { "schema-file", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARXFILENAME_ARRAY, &schema_files, NULL, NULL },
    { "override-file", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARXFILENAME_ARRAY, &override_files, NULL, NULL },
    G_OPTION_ENTRY_NULL
  };

#ifdef G_OS_WIN32
  xchar_t *tmp = NULL;
#endif

  setlocale (LC_ALL, XPL_DEFAULT_LOCALE);
  textdomain (GETTEXT_PACKAGE);

#ifdef G_OS_WIN32
  tmp = _glib_get_locale_dir ();
  bindtextdomain (GETTEXT_PACKAGE, tmp);
#else
  bindtextdomain (GETTEXT_PACKAGE, XPL_LOCALE_DIR);
#endif

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  context = g_option_context_new (N_("DIRECTORY"));
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_set_summary (context,
    N_("Compile all xsettings_t schema files into a schema cache.\n"
       "Schema files are required to have the extension .gschema.xml,\n"
       "and the cache file is called gschemas.compiled."));
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      retval = 1;
      goto done;
    }

  if (show_version_and_exit)
    {
      g_print (PACKAGE_VERSION "\n");
      retval = 0;
      goto done;
    }

  if (!schema_files && argc != 2)
    {
      fprintf (stderr, "%s\n", _("You should give exactly one directory name"));
      retval = 1;
      goto done;
    }

  srcdir = argv[1];

  target = g_build_filename (targetdir ? targetdir : srcdir, "gschemas.compiled", NULL);

  if (!schema_files)
    {
      xptr_array_t *overrides;
      xptr_array_t *files;

      files = xptr_array_new ();
      overrides = xptr_array_new ();

      dir = g_dir_open (srcdir, 0, &error);
      if (dir == NULL)
        {
          fprintf (stderr, "%s\n", error->message);

          xptr_array_unref (files);
          xptr_array_unref (overrides);

          retval = 1;
          goto done;
        }

      while ((file = g_dir_read_name (dir)) != NULL)
        {
          if (xstr_has_suffix (file, ".gschema.xml") ||
              xstr_has_suffix (file, ".enums.xml"))
            xptr_array_add (files, g_build_filename (srcdir, file, NULL));

          else if (xstr_has_suffix (file, ".gschema.override"))
            xptr_array_add (overrides,
                             g_build_filename (srcdir, file, NULL));
        }

      if (files->len == 0)
        {
          if (g_unlink (target))
            fprintf (stdout, "%s\n", _("No schema files found: doing nothing."));
          else
            fprintf (stdout, "%s\n", _("No schema files found: removed existing output file."));

          xptr_array_unref (files);
          xptr_array_unref (overrides);

          retval = 0;
          goto done;
        }
      xptr_array_sort (files, compare_strings);
      xptr_array_add (files, NULL);

      xptr_array_sort (overrides, compare_strings);
      xptr_array_add (overrides, NULL);

      schema_files = (char **) xptr_array_free (files, FALSE);
      override_files = (xchar_t **) xptr_array_free (overrides, FALSE);
    }

  if ((table = parse_gschema_files (schema_files, strict)) == NULL)
    {
      retval = 1;
      goto done;
    }

  if (override_files != NULL &&
      !set_overrides (table, override_files, strict))
    {
      retval = 1;
      goto done;
    }

  if (!dry_run && !write_to_file (table, target, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      retval = 1;
      goto done;
    }

  /* Success. */
  retval = 0;

done:
  g_clear_error (&error);
  g_clear_pointer (&table, xhash_table_unref);
  g_clear_pointer (&dir, g_dir_close);
  g_free (targetdir);
  g_free (target);
  xstrfreev (schema_files);
  xstrfreev (override_files);
  g_option_context_free (context);

#ifdef G_OS_WIN32
  g_free (tmp);
#endif

  return retval;
}

/* Epilogue {{{1 */

/* vim:set foldmethod=marker: */
