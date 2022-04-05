/*
 * Copyright © 2007, 2008 Ryan Lortie
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

#ifndef __G_VARIANT_H__
#define __G_VARIANT_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gvarianttype.h>
#include <glib/gstring.h>
#include <glib/gbytes.h>

G_BEGIN_DECLS

typedef struct _GVariant        xvariant_t;

typedef enum
{
  G_VARIANT_CLASS_BOOLEAN       = 'b',
  G_VARIANT_CLASS_BYTE          = 'y',
  G_VARIANT_CLASS_INT16         = 'n',
  G_VARIANT_CLASS_UINT16        = 'q',
  G_VARIANT_CLASS_INT32         = 'i',
  G_VARIANT_CLASS_UINT32        = 'u',
  G_VARIANT_CLASS_INT64         = 'x',
  G_VARIANT_CLASS_UINT64        = 't',
  G_VARIANT_CLASS_HANDLE        = 'h',
  G_VARIANT_CLASS_DOUBLE        = 'd',
  G_VARIANT_CLASS_STRING        = 's',
  G_VARIANT_CLASS_OBJECT_PATH   = 'o',
  G_VARIANT_CLASS_SIGNATURE     = 'g',
  G_VARIANT_CLASS_VARIANT       = 'v',
  G_VARIANT_CLASS_MAYBE         = 'm',
  G_VARIANT_CLASS_ARRAY         = 'a',
  G_VARIANT_CLASS_TUPLE         = '(',
  G_VARIANT_CLASS_DICT_ENTRY    = '{'
} GVariantClass;

XPL_AVAILABLE_IN_ALL
void                            g_variant_unref                         (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_ref                           (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_ref_sink                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_is_floating                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_take_ref                      (xvariant_t             *value);

XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            g_variant_get_type                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   g_variant_get_type_string               (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_is_of_type                    (xvariant_t             *value,
                                                                         const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_is_container                  (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
GVariantClass                   g_variant_classify                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_boolean                   (xboolean_t              value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_byte                      (guint8                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_int16                     (gint16                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_uint16                    (guint16               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_int32                     (gint32                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_uint32                    (guint32               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_int64                     (gint64                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_uint64                    (guint64               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_handle                    (gint32                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_double                    (xdouble_t               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_string                    (const xchar_t          *string);
XPL_AVAILABLE_IN_2_38
xvariant_t *                      g_variant_new_take_string               (xchar_t                *string);
XPL_AVAILABLE_IN_2_38
xvariant_t *                      g_variant_new_printf                    (const xchar_t          *format_string,
                                                                         ...) G_GNUC_PRINTF (1, 2);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_object_path               (const xchar_t          *object_path);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_is_object_path                (const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_signature                 (const xchar_t          *signature);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_is_signature                  (const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_variant                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_strv                      (const xchar_t * const  *strv,
                                                                         gssize                length);
XPL_AVAILABLE_IN_2_30
xvariant_t *                      g_variant_new_objv                      (const xchar_t * const  *strv,
                                                                         gssize                length);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_bytestring                (const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_bytestring_array          (const xchar_t * const  *strv,
                                                                         gssize                length);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_fixed_array               (const xvariant_type_t   *element_type,
                                                                         gconstpointer         elements,
                                                                         xsize_t                 n_elements,
                                                                         xsize_t                 element_size);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_get_boolean                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
guint8                          g_variant_get_byte                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint16                          g_variant_get_int16                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
guint16                         g_variant_get_uint16                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint32                          g_variant_get_int32                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
guint32                         g_variant_get_uint32                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint64                          g_variant_get_int64                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
guint64                         g_variant_get_uint64                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint32                          g_variant_get_handle                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xdouble_t                         g_variant_get_double                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_get_variant                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   g_variant_get_string                    (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t *                         g_variant_dup_string                    (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
const xchar_t **                  g_variant_get_strv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t **                        g_variant_dup_strv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_2_30
const xchar_t **                  g_variant_get_objv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t **                        g_variant_dup_objv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   g_variant_get_bytestring                (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xchar_t *                         g_variant_dup_bytestring                (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
const xchar_t **                  g_variant_get_bytestring_array          (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t **                        g_variant_dup_bytestring_array          (xvariant_t             *value,
                                                                         xsize_t                *length);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_maybe                     (const xvariant_type_t   *child_type,
                                                                         xvariant_t             *child);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_array                     (const xvariant_type_t   *child_type,
                                                                         xvariant_t * const     *children,
                                                                         xsize_t                 n_children);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_tuple                     (xvariant_t * const     *children,
                                                                         xsize_t                 n_children);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_dict_entry                (xvariant_t             *key,
                                                                         xvariant_t             *value);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_get_maybe                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xsize_t                           g_variant_n_children                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
void                            g_variant_get_child                     (xvariant_t             *value,
                                                                         xsize_t                 index_,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_get_child_value               (xvariant_t             *value,
                                                                         xsize_t                 index_);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_lookup                        (xvariant_t             *dictionary,
                                                                         const xchar_t          *key,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_lookup_value                  (xvariant_t             *dictionary,
                                                                         const xchar_t          *key,
                                                                         const xvariant_type_t   *expected_type);
XPL_AVAILABLE_IN_ALL
gconstpointer                   g_variant_get_fixed_array               (xvariant_t             *value,
                                                                         xsize_t                *n_elements,
                                                                         xsize_t                 element_size);

XPL_AVAILABLE_IN_ALL
xsize_t                           g_variant_get_size                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gconstpointer                   g_variant_get_data                      (xvariant_t             *value);
XPL_AVAILABLE_IN_2_36
GBytes *                        g_variant_get_data_as_bytes             (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
void                            g_variant_store                         (xvariant_t             *value,
                                                                         xpointer_t              data);

XPL_AVAILABLE_IN_ALL
xchar_t *                         g_variant_print                         (xvariant_t             *value,
                                                                         xboolean_t              type_annotate);
XPL_AVAILABLE_IN_ALL
GString *                       g_variant_print_string                  (xvariant_t             *value,
                                                                         GString              *string,
                                                                         xboolean_t              type_annotate);

XPL_AVAILABLE_IN_ALL
xuint_t                           g_variant_hash                          (gconstpointer         value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_equal                         (gconstpointer         one,
                                                                         gconstpointer         two);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_get_normal_form               (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_is_normal_form                (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_byteswap                      (xvariant_t             *value);

XPL_AVAILABLE_IN_2_36
xvariant_t *                      g_variant_new_from_bytes                (const xvariant_type_t   *type,
                                                                         GBytes               *bytes,
                                                                         xboolean_t              trusted);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_from_data                 (const xvariant_type_t   *type,
                                                                         gconstpointer         data,
                                                                         xsize_t                 size,
                                                                         xboolean_t              trusted,
                                                                         GDestroyNotify        notify,
                                                                         xpointer_t              user_data);

typedef struct _GVariantIter GVariantIter;
struct _GVariantIter {
  /*< private >*/
  xsize_t x[16];
};

XPL_AVAILABLE_IN_ALL
GVariantIter *                  g_variant_iter_new                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xsize_t                           g_variant_iter_init                     (GVariantIter         *iter,
                                                                         xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
GVariantIter *                  g_variant_iter_copy                     (GVariantIter         *iter);
XPL_AVAILABLE_IN_ALL
xsize_t                           g_variant_iter_n_children               (GVariantIter         *iter);
XPL_AVAILABLE_IN_ALL
void                            g_variant_iter_free                     (GVariantIter         *iter);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_iter_next_value               (GVariantIter         *iter);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_iter_next                     (GVariantIter         *iter,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xboolean_t                        g_variant_iter_loop                     (GVariantIter         *iter,
                                                                         const xchar_t          *format_string,
                                                                         ...);


typedef struct _GVariantBuilder GVariantBuilder;
struct _GVariantBuilder {
  /*< private >*/
  union
  {
    struct {
      xsize_t partial_magic;
      const xvariant_type_t *type;
      xsize_t y[14];
    } s;
    xsize_t x[16];
  } u;
};

typedef enum
{
  G_VARIANT_PARSE_ERROR_FAILED,
  G_VARIANT_PARSE_ERROR_BASIC_TYPE_EXPECTED,
  G_VARIANT_PARSE_ERROR_CANNOT_INFER_TYPE,
  G_VARIANT_PARSE_ERROR_DEFINITE_TYPE_EXPECTED,
  G_VARIANT_PARSE_ERROR_INPUT_NOT_AT_END,
  G_VARIANT_PARSE_ERROR_INVALID_CHARACTER,
  G_VARIANT_PARSE_ERROR_INVALID_FORMAT_STRING,
  G_VARIANT_PARSE_ERROR_INVALID_OBJECT_PATH,
  G_VARIANT_PARSE_ERROR_INVALID_SIGNATURE,
  G_VARIANT_PARSE_ERROR_INVALID_TYPE_STRING,
  G_VARIANT_PARSE_ERROR_NO_COMMON_TYPE,
  G_VARIANT_PARSE_ERROR_NUMBER_OUT_OF_RANGE,
  G_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG,
  G_VARIANT_PARSE_ERROR_TYPE_ERROR,
  G_VARIANT_PARSE_ERROR_UNEXPECTED_TOKEN,
  G_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD,
  G_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT,
  G_VARIANT_PARSE_ERROR_VALUE_EXPECTED,
  G_VARIANT_PARSE_ERROR_RECURSION
} GVariantParseError;
#define G_VARIANT_PARSE_ERROR (g_variant_parse_error_quark ())

XPL_DEPRECATED_IN_2_38_FOR(g_variant_parse_error_quark)
GQuark                          g_variant_parser_get_error_quark        (void);

XPL_AVAILABLE_IN_ALL
GQuark                          g_variant_parse_error_quark             (void);

/**
 * G_VARIANT_BUILDER_INIT:
 * @variant_type: a const xvariant_type_t*
 *
 * A stack-allocated #GVariantBuilder must be initialized if it is
 * used together with g_auto() to avoid warnings or crashes if
 * function returns before g_variant_builder_init() is called on the
 * builder.
 *
 * This macro can be used as initializer instead of an
 * explicit zeroing a variable when declaring it and a following
 * g_variant_builder_init(), but it cannot be assigned to a variable.
 *
 * The passed @variant_type should be a static xvariant_type_t to avoid
 * lifetime issues, as copying the @variant_type does not happen in
 * the G_VARIANT_BUILDER_INIT() call, but rather in functions that
 * make sure that #GVariantBuilder is valid.
 *
 * |[<!-- language="C" -->
 *   g_auto(GVariantBuilder) builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_BYTESTRING);
 * ]|
 *
 * Since: 2.50
 */
#define G_VARIANT_BUILDER_INIT(variant_type)                                          \
  {                                                                                   \
    {                                                                                 \
      {                                                                               \
        2942751021u /* == GVSB_MAGIC_PARTIAL, see gvariant.c */, variant_type, { 0, } \
      }                                                                               \
    }                                                                                 \
  }

XPL_AVAILABLE_IN_ALL
GVariantBuilder *               g_variant_builder_new                   (const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_unref                 (GVariantBuilder      *builder);
XPL_AVAILABLE_IN_ALL
GVariantBuilder *               g_variant_builder_ref                   (GVariantBuilder      *builder);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_init                  (GVariantBuilder      *builder,
                                                                         const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_builder_end                   (GVariantBuilder      *builder);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_clear                 (GVariantBuilder      *builder);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_open                  (GVariantBuilder      *builder,
                                                                         const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_close                 (GVariantBuilder      *builder);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_add_value             (GVariantBuilder      *builder,
                                                                         xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_add                   (GVariantBuilder      *builder,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
void                            g_variant_builder_add_parsed            (GVariantBuilder      *builder,
                                                                         const xchar_t          *format,
                                                                         ...);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new                           (const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
void                            g_variant_get                           (xvariant_t             *value,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_va                        (const xchar_t          *format_string,
                                                                         const xchar_t         **endptr,
                                                                         va_list              *app);
XPL_AVAILABLE_IN_ALL
void                            g_variant_get_va                        (xvariant_t             *value,
                                                                         const xchar_t          *format_string,
                                                                         const xchar_t         **endptr,
                                                                         va_list              *app);
XPL_AVAILABLE_IN_2_34
xboolean_t                        g_variant_check_format_string           (xvariant_t             *value,
                                                                         const xchar_t          *format_string,
                                                                         xboolean_t              copy_only);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_parse                         (const xvariant_type_t   *type,
                                                                         const xchar_t          *text,
                                                                         const xchar_t          *limit,
                                                                         const xchar_t         **endptr,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_parsed                    (const xchar_t          *format,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      g_variant_new_parsed_va                 (const xchar_t          *format,
                                                                         va_list              *app);

XPL_AVAILABLE_IN_2_40
xchar_t *                         g_variant_parse_error_print_context     (xerror_t               *error,
                                                                         const xchar_t          *source_str);

XPL_AVAILABLE_IN_ALL
xint_t                            g_variant_compare                       (gconstpointer one,
                                                                         gconstpointer two);

typedef struct _GVariantDict GVariantDict;
struct _GVariantDict {
  /*< private >*/
  union
  {
    struct {
      xvariant_t *asv;
      xsize_t partial_magic;
      xsize_t y[14];
    } s;
    xsize_t x[16];
  } u;
};

/**
 * G_VARIANT_DICT_INIT:
 * @asv: (nullable): a xvariant_t*
 *
 * A stack-allocated #GVariantDict must be initialized if it is used
 * together with g_auto() to avoid warnings or crashes if function
 * returns before g_variant_dict_init() is called on the builder.
 *
 * This macro can be used as initializer instead of an explicit
 * zeroing a variable when declaring it and a following
 * g_variant_dict_init(), but it cannot be assigned to a variable.
 *
 * The passed @asv has to live long enough for #GVariantDict to gather
 * the entries from, as the gathering does not happen in the
 * G_VARIANT_DICT_INIT() call, but rather in functions that make sure
 * that #GVariantDict is valid.  In context where the initialization
 * value has to be a constant expression, the only possible value of
 * @asv is %NULL.  It is still possible to call g_variant_dict_init()
 * safely with a different @asv right after the variable was
 * initialized with G_VARIANT_DICT_INIT().
 *
 * |[<!-- language="C" -->
 *   g_autoptr(xvariant_t) variant = get_asv_variant ();
 *   g_auto(GVariantDict) dict = G_VARIANT_DICT_INIT (variant);
 * ]|
 *
 * Since: 2.50
 */
#define G_VARIANT_DICT_INIT(asv)                                             \
  {                                                                          \
    {                                                                        \
      {                                                                      \
        asv, 3488698669u /* == GVSD_MAGIC_PARTIAL, see gvariant.c */, { 0, } \
      }                                                                      \
    }                                                                        \
  }

XPL_AVAILABLE_IN_2_40
GVariantDict *                  g_variant_dict_new                      (xvariant_t             *from_asv);

XPL_AVAILABLE_IN_2_40
void                            g_variant_dict_init                     (GVariantDict         *dict,
                                                                         xvariant_t             *from_asv);

XPL_AVAILABLE_IN_2_40
xboolean_t                        g_variant_dict_lookup                   (GVariantDict         *dict,
                                                                         const xchar_t          *key,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_2_40
xvariant_t *                      g_variant_dict_lookup_value             (GVariantDict         *dict,
                                                                         const xchar_t          *key,
                                                                         const xvariant_type_t   *expected_type);
XPL_AVAILABLE_IN_2_40
xboolean_t                        g_variant_dict_contains                 (GVariantDict         *dict,
                                                                         const xchar_t          *key);
XPL_AVAILABLE_IN_2_40
void                            g_variant_dict_insert                   (GVariantDict         *dict,
                                                                         const xchar_t          *key,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_2_40
void                            g_variant_dict_insert_value             (GVariantDict         *dict,
                                                                         const xchar_t          *key,
                                                                         xvariant_t             *value);
XPL_AVAILABLE_IN_2_40
xboolean_t                        g_variant_dict_remove                   (GVariantDict         *dict,
                                                                         const xchar_t          *key);
XPL_AVAILABLE_IN_2_40
void                            g_variant_dict_clear                    (GVariantDict         *dict);
XPL_AVAILABLE_IN_2_40
xvariant_t *                      g_variant_dict_end                      (GVariantDict         *dict);
XPL_AVAILABLE_IN_2_40
GVariantDict *                  g_variant_dict_ref                      (GVariantDict         *dict);
XPL_AVAILABLE_IN_2_40
void                            g_variant_dict_unref                    (GVariantDict         *dict);

G_END_DECLS

#endif /* __G_VARIANT_H__ */
