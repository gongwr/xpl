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

typedef struct _xvariant        xvariant_t;

typedef enum
{
  XVARIANT_CLASS_BOOLEAN       = 'b',
  XVARIANT_CLASS_BYTE          = 'y',
  XVARIANT_CLASS_INT16         = 'n',
  XVARIANT_CLASS_UINT16        = 'q',
  XVARIANT_CLASS_INT32         = 'i',
  XVARIANT_CLASS_UINT32        = 'u',
  XVARIANT_CLASS_INT64         = 'x',
  XVARIANT_CLASS_UINT64        = 't',
  XVARIANT_CLASS_HANDLE        = 'h',
  XVARIANT_CLASS_DOUBLE        = 'd',
  XVARIANT_CLASS_STRING        = 's',
  XVARIANT_CLASS_OBJECT_PATH   = 'o',
  XVARIANT_CLASS_SIGNATURE     = 'g',
  XVARIANT_CLASS_VARIANT       = 'v',
  XVARIANT_CLASS_MAYBE         = 'm',
  XVARIANT_CLASS_ARRAY         = 'a',
  XVARIANT_CLASS_TUPLE         = '(',
  XVARIANT_CLASS_DICT_ENTRY    = '{'
} xvariant_class_t;

XPL_AVAILABLE_IN_ALL
void                            xvariant_unref                         (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_ref                           (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_ref_sink                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_is_floating                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_take_ref                      (xvariant_t             *value);

XPL_AVAILABLE_IN_ALL
const xvariant_type_t *            xvariant_get_type                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   xvariant_get_type_string               (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_is_of_type                    (xvariant_t             *value,
                                                                         const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_is_container                  (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_class_t                   xvariant_classify                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_boolean                   (xboolean_t              value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_byte                      (xuint8_t                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_int16                     (gint16                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_uint16                    (xuint16_t               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_int32                     (gint32                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_uint32                    (xuint32_t               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_int64                     (sint64_t                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_uint64                    (xuint64_t               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_handle                    (gint32                value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_double                    (xdouble_t               value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_string                    (const xchar_t          *string);
XPL_AVAILABLE_IN_2_38
xvariant_t *                      xvariant_new_take_string               (xchar_t                *string);
XPL_AVAILABLE_IN_2_38
xvariant_t *                      xvariant_new_printf                    (const xchar_t          *format_string,
                                                                         ...) G_GNUC_PRINTF (1, 2);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_object_path               (const xchar_t          *object_path);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_is_object_path                (const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_signature                 (const xchar_t          *signature);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_is_signature                  (const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_variant                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_strv                      (const xchar_t * const  *strv,
                                                                         xssize_t                length);
XPL_AVAILABLE_IN_2_30
xvariant_t *                      xvariant_new_objv                      (const xchar_t * const  *strv,
                                                                         xssize_t                length);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_bytestring                (const xchar_t          *string);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_bytestring_array          (const xchar_t * const  *strv,
                                                                         xssize_t                length);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_fixed_array               (const xvariant_type_t   *element_type,
                                                                         xconstpointer         elements,
                                                                         xsize_t                 n_elements,
                                                                         xsize_t                 element_size);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_get_boolean                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xuint8_t                          xvariant_get_byte                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint16                          xvariant_get_int16                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xuint16_t                         xvariant_get_uint16                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint32                          xvariant_get_int32                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xuint32_t                         xvariant_get_uint32                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
sint64_t                          xvariant_get_int64                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xuint64_t                         xvariant_get_uint64                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
gint32                          xvariant_get_handle                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xdouble_t                         xvariant_get_double                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_get_variant                   (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   xvariant_get_string                    (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t *                         xvariant_dup_string                    (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
const xchar_t **                  xvariant_get_strv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t **                        xvariant_dup_strv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_2_30
const xchar_t **                  xvariant_get_objv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t **                        xvariant_dup_objv                      (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
const xchar_t *                   xvariant_get_bytestring                (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xchar_t *                         xvariant_dup_bytestring                (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
const xchar_t **                  xvariant_get_bytestring_array          (xvariant_t             *value,
                                                                         xsize_t                *length);
XPL_AVAILABLE_IN_ALL
xchar_t **                        xvariant_dup_bytestring_array          (xvariant_t             *value,
                                                                         xsize_t                *length);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_maybe                     (const xvariant_type_t   *child_type,
                                                                         xvariant_t             *child);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_array                     (const xvariant_type_t   *child_type,
                                                                         xvariant_t * const     *children,
                                                                         xsize_t                 n_children);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_tuple                     (xvariant_t * const     *children,
                                                                         xsize_t                 n_children);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_dict_entry                (xvariant_t             *key,
                                                                         xvariant_t             *value);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_get_maybe                     (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_n_children                    (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
void                            xvariant_get_child                     (xvariant_t             *value,
                                                                         xsize_t                 index_,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_get_child_value               (xvariant_t             *value,
                                                                         xsize_t                 index_);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_lookup                        (xvariant_t             *dictionary,
                                                                         const xchar_t          *key,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_lookup_value                  (xvariant_t             *dictionary,
                                                                         const xchar_t          *key,
                                                                         const xvariant_type_t   *expected_type);
XPL_AVAILABLE_IN_ALL
xconstpointer                   xvariant_get_fixed_array               (xvariant_t             *value,
                                                                         xsize_t                *n_elements,
                                                                         xsize_t                 element_size);

XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_get_size                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xconstpointer                   xvariant_get_data                      (xvariant_t             *value);
XPL_AVAILABLE_IN_2_36
xbytes_t *                        xvariant_get_data_as_bytes             (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
void                            xvariant_store                         (xvariant_t             *value,
                                                                         xpointer_t              data);

XPL_AVAILABLE_IN_ALL
xchar_t *                         xvariant_print                         (xvariant_t             *value,
                                                                         xboolean_t              type_annotate);
XPL_AVAILABLE_IN_ALL
xstring_t *                       xvariant_print_string                  (xvariant_t             *value,
                                                                         xstring_t              *string,
                                                                         xboolean_t              type_annotate);

XPL_AVAILABLE_IN_ALL
xuint_t                           xvariant_hash                          (xconstpointer         value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_equal                         (xconstpointer         one,
                                                                         xconstpointer         two);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_get_normal_form               (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_is_normal_form                (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_byteswap                      (xvariant_t             *value);

XPL_AVAILABLE_IN_2_36
xvariant_t *                      xvariant_new_from_bytes                (const xvariant_type_t   *type,
                                                                         xbytes_t               *bytes,
                                                                         xboolean_t              trusted);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_from_data                 (const xvariant_type_t   *type,
                                                                         xconstpointer         data,
                                                                         xsize_t                 size,
                                                                         xboolean_t              trusted,
                                                                         xdestroy_notify_t        notify,
                                                                         xpointer_t              user_data);

typedef struct _GVariantIter xvariant_iter_t;
struct _GVariantIter {
  /*< private >*/
  xsize_t x[16];
};

XPL_AVAILABLE_IN_ALL
xvariant_iter_t *                  xvariant_iter_new                      (xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_iter_init                     (xvariant_iter_t         *iter,
                                                                         xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
xvariant_iter_t *                  xvariant_iter_copy                     (xvariant_iter_t         *iter);
XPL_AVAILABLE_IN_ALL
xsize_t                           xvariant_iter_n_children               (xvariant_iter_t         *iter);
XPL_AVAILABLE_IN_ALL
void                            xvariant_iter_free                     (xvariant_iter_t         *iter);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_iter_next_value               (xvariant_iter_t         *iter);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_iter_next                     (xvariant_iter_t         *iter,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xboolean_t                        xvariant_iter_loop                     (xvariant_iter_t         *iter,
                                                                         const xchar_t          *format_string,
                                                                         ...);


typedef struct _GVariantBuilder xvariant_builder_t;
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
#define G_VARIANT_PARSE_ERROR (xvariant_parse_error_quark ())

XPL_DEPRECATED_IN_2_38_FOR(xvariant_parse_error_quark)
xquark                          xvariant_parser_get_error_quark        (void);

XPL_AVAILABLE_IN_ALL
xquark                          xvariant_parse_error_quark             (void);

/**
 * G_VARIANT_BUILDER_INIT:
 * @variant_type: a const xvariant_type_t*
 *
 * A stack-allocated #xvariant_builder_t must be initialized if it is
 * used together with x_auto() to avoid warnings or crashes if
 * function returns before xvariant_builder_init() is called on the
 * builder.
 *
 * This macro can be used as initializer instead of an
 * explicit zeroing a variable when declaring it and a following
 * xvariant_builder_init(), but it cannot be assigned to a variable.
 *
 * The passed @variant_type should be a static xvariant_type_t to avoid
 * lifetime issues, as copying the @variant_type does not happen in
 * the G_VARIANT_BUILDER_INIT() call, but rather in functions that
 * make sure that #xvariant_builder_t is valid.
 *
 * |[<!-- language="C" -->
 *   x_auto(xvariant_builder) builder = G_VARIANT_BUILDER_INIT (G_VARIANT_TYPE_BYTESTRING);
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
xvariant_builder_t *               xvariant_builder_new                   (const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_unref                 (xvariant_builder_t      *builder);
XPL_AVAILABLE_IN_ALL
xvariant_builder_t *               xvariant_builder_ref                   (xvariant_builder_t      *builder);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_init                  (xvariant_builder_t      *builder,
                                                                         const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_builder_end                   (xvariant_builder_t      *builder);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_clear                 (xvariant_builder_t      *builder);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_open                  (xvariant_builder_t      *builder,
                                                                         const xvariant_type_t   *type);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_close                 (xvariant_builder_t      *builder);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_add_value             (xvariant_builder_t      *builder,
                                                                         xvariant_t             *value);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_add                   (xvariant_builder_t      *builder,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
void                            xvariant_builder_add_parsed            (xvariant_builder_t      *builder,
                                                                         const xchar_t          *format,
                                                                         ...);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new                           (const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
void                            xvariant_get                           (xvariant_t             *value,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_va                        (const xchar_t          *format_string,
                                                                         const xchar_t         **endptr,
                                                                         va_list              *app);
XPL_AVAILABLE_IN_ALL
void                            xvariant_get_va                        (xvariant_t             *value,
                                                                         const xchar_t          *format_string,
                                                                         const xchar_t         **endptr,
                                                                         va_list              *app);
XPL_AVAILABLE_IN_2_34
xboolean_t                        xvariant_check_format_string           (xvariant_t             *value,
                                                                         const xchar_t          *format_string,
                                                                         xboolean_t              copy_only);

XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_parse                         (const xvariant_type_t   *type,
                                                                         const xchar_t          *text,
                                                                         const xchar_t          *limit,
                                                                         const xchar_t         **endptr,
                                                                         xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_parsed                    (const xchar_t          *format,
                                                                         ...);
XPL_AVAILABLE_IN_ALL
xvariant_t *                      xvariant_new_parsed_va                 (const xchar_t          *format,
                                                                         va_list              *app);

XPL_AVAILABLE_IN_2_40
xchar_t *                         xvariant_parse_error_print_context     (xerror_t               *error,
                                                                         const xchar_t          *source_str);

XPL_AVAILABLE_IN_ALL
xint_t                            xvariant_compare                       (xconstpointer one,
                                                                         xconstpointer two);

typedef struct _xvariant_dict xvariant_dict_t;
struct _xvariant_dict {
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
 * XVARIANT_DICT_INIT:
 * @asv: (nullable): a xvariant_t*
 *
 * A stack-allocated #xvariant_dict_t must be initialized if it is used
 * together with x_auto() to avoid warnings or crashes if function
 * returns before xvariant_dict_init() is called on the builder.
 *
 * This macro can be used as initializer instead of an explicit
 * zeroing a variable when declaring it and a following
 * xvariant_dict_init(), but it cannot be assigned to a variable.
 *
 * The passed @asv has to live long enough for #xvariant_dict_t to gather
 * the entries from, as the gathering does not happen in the
 * XVARIANT_DICT_INIT() call, but rather in functions that make sure
 * that #xvariant_dict_t is valid.  In context where the initialization
 * value has to be a constant expression, the only possible value of
 * @asv is %NULL.  It is still possible to call xvariant_dict_init()
 * safely with a different @asv right after the variable was
 * initialized with XVARIANT_DICT_INIT().
 *
 * |[<!-- language="C" -->
 *   x_autoptr(xvariant) variant = get_asv_variant ();
 *   x_auto(xvariant_dict) dict = XVARIANT_DICT_INIT (variant);
 * ]|
 *
 * Since: 2.50
 */
#define XVARIANT_DICT_INIT(asv)                                             \
  {                                                                          \
    {                                                                        \
      {                                                                      \
        asv, 3488698669u /* == GVSD_MAGIC_PARTIAL, see gvariant.c */, { 0, } \
      }                                                                      \
    }                                                                        \
  }

XPL_AVAILABLE_IN_2_40
xvariant_dict_t *                  xvariant_dict_new                      (xvariant_t             *from_asv);

XPL_AVAILABLE_IN_2_40
void                            xvariant_dict_init                     (xvariant_dict_t         *dict,
                                                                         xvariant_t             *from_asv);

XPL_AVAILABLE_IN_2_40
xboolean_t                        xvariant_dict_lookup                   (xvariant_dict_t         *dict,
                                                                         const xchar_t          *key,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_2_40
xvariant_t *                      xvariant_dict_lookup_value             (xvariant_dict_t         *dict,
                                                                         const xchar_t          *key,
                                                                         const xvariant_type_t   *expected_type);
XPL_AVAILABLE_IN_2_40
xboolean_t                        xvariant_dict_contains                 (xvariant_dict_t         *dict,
                                                                         const xchar_t          *key);
XPL_AVAILABLE_IN_2_40
void                            xvariant_dict_insert                   (xvariant_dict_t         *dict,
                                                                         const xchar_t          *key,
                                                                         const xchar_t          *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_2_40
void                            xvariant_dict_insert_value             (xvariant_dict_t         *dict,
                                                                         const xchar_t          *key,
                                                                         xvariant_t             *value);
XPL_AVAILABLE_IN_2_40
xboolean_t                        xvariant_dict_remove                   (xvariant_dict_t         *dict,
                                                                         const xchar_t          *key);
XPL_AVAILABLE_IN_2_40
void                            xvariant_dict_clear                    (xvariant_dict_t         *dict);
XPL_AVAILABLE_IN_2_40
xvariant_t *                      xvariant_dict_end                      (xvariant_dict_t         *dict);
XPL_AVAILABLE_IN_2_40
xvariant_dict_t *                  xvariant_dict_ref                      (xvariant_dict_t         *dict);
XPL_AVAILABLE_IN_2_40
void                            xvariant_dict_unref                    (xvariant_dict_t         *dict);

G_END_DECLS

#endif /* __G_VARIANT_H__ */
