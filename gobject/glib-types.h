/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __XPL_TYPES_H__
#define __XPL_TYPES_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION) && !defined(XPL_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/* A hack necesssary to preprocess this file with g-ir-scanner */
#ifdef __GI_SCANNER__
typedef xsize_t xtype_t;
#endif

/* --- GLib boxed types --- */
/**
 * XTYPE_DATE:
 *
 * The #xtype_t for #GDate.
 */
#define XTYPE_DATE (g_date_get_type ())

/**
 * XTYPE_STRV:
 *
 * The #xtype_t for a boxed type holding a %NULL-terminated array of strings.
 *
 * The code fragments in the following example show the use of a property of
 * type %XTYPE_STRV with g_object_class_install_property(), g_object_set()
 * and g_object_get().
 *
 * |[
 * g_object_class_install_property (object_class,
 *                                  PROP_AUTHORS,
 *                                  g_param_spec_boxed ("authors",
 *                                                      _("Authors"),
 *                                                      _("List of authors"),
 *                                                      XTYPE_STRV,
 *                                                      G_PARAM_READWRITE));
 *
 * xchar_t *authors[] = { "Owen", "Tim", NULL };
 * g_object_set (obj, "authors", authors, NULL);
 *
 * xchar_t *writers[];
 * g_object_get (obj, "authors", &writers, NULL);
 * /&ast; do something with writers &ast;/
 * g_strfreev (writers);
 * ]|
 *
 * Since: 2.4
 */
#define XTYPE_STRV (g_strv_get_type ())

/**
 * XTYPE_GSTRING:
 *
 * The #xtype_t for #GString.
 */
#define XTYPE_GSTRING (g_gstring_get_type ())

/**
 * XTYPE_HASH_TABLE:
 *
 * The #xtype_t for a boxed type holding a #GHashTable reference.
 *
 * Since: 2.10
 */
#define XTYPE_HASH_TABLE (g_hash_table_get_type ())

/**
 * XTYPE_REGEX:
 *
 * The #xtype_t for a boxed type holding a #GRegex reference.
 *
 * Since: 2.14
 */
#define XTYPE_REGEX (g_regex_get_type ())

/**
 * XTYPE_MATCH_INFO:
 *
 * The #xtype_t for a boxed type holding a #GMatchInfo reference.
 *
 * Since: 2.30
 */
#define XTYPE_MATCH_INFO (g_match_info_get_type ())

/**
 * XTYPE_ARRAY:
 *
 * The #xtype_t for a boxed type holding a #GArray reference.
 *
 * Since: 2.22
 */
#define XTYPE_ARRAY (g_array_get_type ())

/**
 * XTYPE_BYTE_ARRAY:
 *
 * The #xtype_t for a boxed type holding a #GByteArray reference.
 *
 * Since: 2.22
 */
#define XTYPE_BYTE_ARRAY (g_byte_array_get_type ())

/**
 * XTYPE_PTR_ARRAY:
 *
 * The #xtype_t for a boxed type holding a #GPtrArray reference.
 *
 * Since: 2.22
 */
#define XTYPE_PTR_ARRAY (g_ptr_array_get_type ())

/**
 * XTYPE_BYTES:
 *
 * The #xtype_t for #GBytes.
 *
 * Since: 2.32
 */
#define XTYPE_BYTES (g_bytes_get_type ())

/**
 * XTYPE_VARIANT_TYPE:
 *
 * The #xtype_t for a boxed type holding a #xvariant_type_t.
 *
 * Since: 2.24
 */
#define XTYPE_VARIANT_TYPE (g_variant_type_get_gtype ())

/**
 * XTYPE_ERROR:
 *
 * The #xtype_t for a boxed type holding a #xerror_t.
 *
 * Since: 2.26
 */
#define XTYPE_ERROR (g_error_get_type ())

/**
 * XTYPE_DATE_TIME:
 *
 * The #xtype_t for a boxed type holding a #GDateTime.
 *
 * Since: 2.26
 */
#define XTYPE_DATE_TIME (g_date_time_get_type ())

/**
 * XTYPE_TIME_ZONE:
 *
 * The #xtype_t for a boxed type holding a #GTimeZone.
 *
 * Since: 2.34
 */
#define XTYPE_TIME_ZONE (g_time_zone_get_type ())

/**
 * XTYPE_IO_CHANNEL:
 *
 * The #xtype_t for #GIOChannel.
 */
#define XTYPE_IO_CHANNEL (g_io_channel_get_type ())

/**
 * XTYPE_IO_CONDITION:
 *
 * The #xtype_t for #GIOCondition.
 */
#define XTYPE_IO_CONDITION (g_io_condition_get_type ())

/**
 * XTYPE_VARIANT_BUILDER:
 *
 * The #xtype_t for a boxed type holding a #GVariantBuilder.
 *
 * Since: 2.30
 */
#define XTYPE_VARIANT_BUILDER (g_variant_builder_get_type ())

/**
 * XTYPE_VARIANT_DICT:
 *
 * The #xtype_t for a boxed type holding a #GVariantDict.
 *
 * Since: 2.40
 */
#define XTYPE_VARIANT_DICT (g_variant_dict_get_type ())

/**
 * XTYPE_MAIN_LOOP:
 *
 * The #xtype_t for a boxed type holding a #GMainLoop.
 *
 * Since: 2.30
 */
#define XTYPE_MAIN_LOOP (g_main_loop_get_type ())

/**
 * XTYPE_MAIN_CONTEXT:
 *
 * The #xtype_t for a boxed type holding a #GMainContext.
 *
 * Since: 2.30
 */
#define XTYPE_MAIN_CONTEXT (g_main_context_get_type ())

/**
 * XTYPE_SOURCE:
 *
 * The #xtype_t for a boxed type holding a #GSource.
 *
 * Since: 2.30
 */
#define XTYPE_SOURCE (g_source_get_type ())

/**
 * XTYPE_POLLFD:
 *
 * The #xtype_t for a boxed type holding a #GPollFD.
 *
 * Since: 2.36
 */
#define XTYPE_POLLFD (g_pollfd_get_type ())

/**
 * XTYPE_MARKUP_PARSE_CONTEXT:
 *
 * The #xtype_t for a boxed type holding a #GMarkupParseContext.
 *
 * Since: 2.36
 */
#define XTYPE_MARKUP_PARSE_CONTEXT (g_markup_parse_context_get_type ())

/**
 * XTYPE_KEY_FILE:
 *
 * The #xtype_t for a boxed type holding a #GKeyFile.
 *
 * Since: 2.32
 */
#define XTYPE_KEY_FILE (g_key_file_get_type ())

/**
 * XTYPE_MAPPED_FILE:
 *
 * The #xtype_t for a boxed type holding a #GMappedFile.
 *
 * Since: 2.40
 */
#define XTYPE_MAPPED_FILE (g_mapped_file_get_type ())

/**
 * XTYPE_THREAD:
 *
 * The #xtype_t for a boxed type holding a #GThread.
 *
 * Since: 2.36
 */
#define XTYPE_THREAD (g_thread_get_type ())

/**
 * XTYPE_CHECKSUM:
 *
 * The #xtype_t for a boxed type holding a #GChecksum.
 *
 * Since: 2.36
 */
#define XTYPE_CHECKSUM (g_checksum_get_type ())

/**
 * XTYPE_OPTION_GROUP:
 *
 * The #xtype_t for a boxed type holding a #GOptionGroup.
 *
 * Since: 2.44
 */
#define XTYPE_OPTION_GROUP (g_option_group_get_type ())

/**
 * XTYPE_URI:
 *
 * The #xtype_t for a boxed type holding a #GUri.
 *
 * Since: 2.66
 */
#define XTYPE_URI (g_uri_get_type ())

/**
 * XTYPE_TREE:
 *
 * The #xtype_t for #GTree.
 *
 * Since: 2.68
 */
#define XTYPE_TREE (g_tree_get_type ())

/**
 * XTYPE_PATTERN_SPEC:
 *
 * The #xtype_t for #GPatternSpec.
 *
 * Since: 2.70
 */
#define XTYPE_PATTERN_SPEC (g_pattern_spec_get_type ())

XPL_AVAILABLE_IN_ALL
xtype_t   g_date_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_strv_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_gstring_get_type         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_hash_table_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_array_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_byte_array_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_ptr_array_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_bytes_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_variant_type_get_gtype   (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_regex_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   g_match_info_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_error_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_date_time_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_time_zone_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_io_channel_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_io_condition_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_variant_builder_get_type (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_40
xtype_t   g_variant_dict_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_key_file_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   g_main_loop_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   g_main_context_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   g_source_get_type          (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   g_pollfd_get_type          (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   g_thread_get_type          (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   g_checksum_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   g_markup_parse_context_get_type (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_40
xtype_t   g_mapped_file_get_type (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_44
xtype_t   g_option_group_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_66
xtype_t   g_uri_get_type             (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_68
xtype_t   g_tree_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_70
xtype_t g_pattern_spec_get_type (void) G_GNUC_CONST;

XPL_DEPRECATED_FOR('XTYPE_VARIANT')
xtype_t   g_variant_get_gtype        (void) G_GNUC_CONST;

G_END_DECLS

#endif  /* __XPL_TYPES_H__ */
