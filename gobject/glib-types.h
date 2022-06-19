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
 * The #xtype_t for #xdate_t.
 */
#define XTYPE_DATE (xdate_get_type ())

/**
 * XTYPE_STRV:
 *
 * The #xtype_t for a boxed type holding a %NULL-terminated array of strings.
 *
 * The code fragments in the following example show the use of a property of
 * type %XTYPE_STRV with xobject_class_install_property(), xobject_set()
 * and xobject_get().
 *
 * |[
 * xobject_class_install_property (object_class,
 *                                  PROP_AUTHORS,
 *                                  xparam_spec_boxed ("authors",
 *                                                      _("Authors"),
 *                                                      _("List of authors"),
 *                                                      XTYPE_STRV,
 *                                                      XPARAM_READWRITE));
 *
 * xchar_t *authors[] = { "Owen", "Tim", NULL };
 * xobject_set (obj, "authors", authors, NULL);
 *
 * xchar_t *writers[];
 * xobject_get (obj, "authors", &writers, NULL);
 * /&ast; do something with writers &ast;/
 * xstrfreev (writers);
 * ]|
 *
 * Since: 2.4
 */
#define XTYPE_STRV (xstrv_get_type ())

/**
 * XTYPE_GSTRING:
 *
 * The #xtype_t for #xstring_t.
 */
#define XTYPE_GSTRING (xstring_get_type ())

/**
 * XTYPE_HASH_TABLE:
 *
 * The #xtype_t for a boxed type holding a #xhashtable_t reference.
 *
 * Since: 2.10
 */
#define XTYPE_HASH_TABLE (xhash_table_get_type ())

/**
 * XTYPE_REGEX:
 *
 * The #xtype_t for a boxed type holding a #xregex_t reference.
 *
 * Since: 2.14
 */
#define XTYPE_REGEX (xregex_get_type ())

/**
 * XTYPE_MATCH_INFO:
 *
 * The #xtype_t for a boxed type holding a #xmatch_info_t reference.
 *
 * Since: 2.30
 */
#define XTYPE_MATCH_INFO (xmatch_info_get_type ())

/**
 * XTYPE_ARRAY:
 *
 * The #xtype_t for a boxed type holding a #xarray_t reference.
 *
 * Since: 2.22
 */
#define XTYPE_ARRAY (g_array_get_type ())

/**
 * XTYPE_BYTE_ARRAY:
 *
 * The #xtype_t for a boxed type holding a #xbyte_array_t reference.
 *
 * Since: 2.22
 */
#define XTYPE_BYTE_ARRAY (xbyte_array_get_type ())

/**
 * XTYPE_PTR_ARRAY:
 *
 * The #xtype_t for a boxed type holding a #xptr_array_t reference.
 *
 * Since: 2.22
 */
#define XTYPE_PTR_ARRAY (xptr_array_get_type ())

/**
 * XTYPE_BYTES:
 *
 * The #xtype_t for #xbytes_t.
 *
 * Since: 2.32
 */
#define XTYPE_BYTES (xbytes_get_type ())

/**
 * XTYPE_VARIANT_TYPE:
 *
 * The #xtype_t for a boxed type holding a #xvariant_type_t.
 *
 * Since: 2.24
 */
#define XTYPE_VARIANT_TYPE (xvariant_type_get_gtype ())

/**
 * XTYPE_ERROR:
 *
 * The #xtype_t for a boxed type holding a #xerror_t.
 *
 * Since: 2.26
 */
#define XTYPE_ERROR (xerror_get_type ())

/**
 * XTYPE_DATE_TIME:
 *
 * The #xtype_t for a boxed type holding a #xdatetime_t.
 *
 * Since: 2.26
 */
#define XTYPE_DATE_TIME (xdate_time_get_type ())

/**
 * XTYPE_TIME_ZONE:
 *
 * The #xtype_t for a boxed type holding a #xtimezone_t.
 *
 * Since: 2.34
 */
#define XTYPE_TIME_ZONE (xtime_zone_get_type ())

/**
 * XTYPE_IO_CHANNEL:
 *
 * The #xtype_t for #xio_channel_t.
 */
#define XTYPE_IO_CHANNEL (g_io_channel_get_type ())

/**
 * XTYPE_IO_CONDITION:
 *
 * The #xtype_t for #xio_condition_t.
 */
#define XTYPE_IO_CONDITION (g_io_condition_get_type ())

/**
 * XTYPE_VARIANT_BUILDER:
 *
 * The #xtype_t for a boxed type holding a #xvariant_builder_t.
 *
 * Since: 2.30
 */
#define XTYPE_VARIANT_BUILDER (xvariant_builder_get_type ())

/**
 * XTYPE_VARIANT_DICT:
 *
 * The #xtype_t for a boxed type holding a #xvariant_dict_t.
 *
 * Since: 2.40
 */
#define XTYPE_VARIANT_DICT (xvariant_dict_get_type ())

/**
 * XTYPE_MAIN_LOOP:
 *
 * The #xtype_t for a boxed type holding a #xmain_loop_t.
 *
 * Since: 2.30
 */
#define XTYPE_MAIN_LOOP (xmain_loop_get_type ())

/**
 * XTYPE_MAIN_CONTEXT:
 *
 * The #xtype_t for a boxed type holding a #xmain_context_t.
 *
 * Since: 2.30
 */
#define XTYPE_MAIN_CONTEXT (xmain_context_get_type ())

/**
 * XTYPE_SOURCE:
 *
 * The #xtype_t for a boxed type holding a #xsource_t.
 *
 * Since: 2.30
 */
#define XTYPE_SOURCE (xsource_get_type ())

/**
 * XTYPE_POLLFD:
 *
 * The #xtype_t for a boxed type holding a #xpollfd_t.
 *
 * Since: 2.36
 */
#define XTYPE_POLLFD (xpollfd_get_type ())

/**
 * XTYPE_MARKUP_PARSE_CONTEXT:
 *
 * The #xtype_t for a boxed type holding a #xmarkup_parse_context_t.
 *
 * Since: 2.36
 */
#define XTYPE_MARKUP_PARSE_CONTEXT (xmarkup_parse_context_get_type ())

/**
 * XTYPE_KEY_FILE:
 *
 * The #xtype_t for a boxed type holding a #xkey_file_t.
 *
 * Since: 2.32
 */
#define XTYPE_KEY_FILE (xkey_file_get_type ())

/**
 * XTYPE_MAPPED_FILE:
 *
 * The #xtype_t for a boxed type holding a #xmapped_file_t.
 *
 * Since: 2.40
 */
#define XTYPE_MAPPED_FILE (xmapped_file_get_type ())

/**
 * XTYPE_THREAD:
 *
 * The #xtype_t for a boxed type holding a #xthread_t.
 *
 * Since: 2.36
 */
#define XTYPE_THREAD (xthread_get_type ())

/**
 * XTYPE_CHECKSUM:
 *
 * The #xtype_t for a boxed type holding a #xchecksum_t.
 *
 * Since: 2.36
 */
#define XTYPE_CHECKSUM (xchecksum_get_type ())

/**
 * XTYPE_OPTION_GROUP:
 *
 * The #xtype_t for a boxed type holding a #xoption_group_t.
 *
 * Since: 2.44
 */
#define XTYPE_OPTION_GROUP (xoption_group_get_type ())

/**
 * XTYPE_URI:
 *
 * The #xtype_t for a boxed type holding a #xuri_t.
 *
 * Since: 2.66
 */
#define XTYPE_URI (xuri_get_type ())

/**
 * XTYPE_TREE:
 *
 * The #xtype_t for #xtree_t.
 *
 * Since: 2.68
 */
#define XTYPE_TREE (xtree_get_type ())

/**
 * XTYPE_PATTERN_SPEC:
 *
 * The #xtype_t for #xpattern_spec_t.
 *
 * Since: 2.70
 */
#define XTYPE_PATTERN_SPEC (xpattern_spec_get_type ())

XPL_AVAILABLE_IN_ALL
xtype_t   xdate_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xstrv_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xstring_get_type         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xhash_table_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_array_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xbyte_array_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xptr_array_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xbytes_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xvariant_type_get_gtype   (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xregex_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   xmatch_info_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xerror_get_type           (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xdate_time_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xtime_zone_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_io_channel_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   g_io_condition_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xvariant_builder_get_type (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_40
xtype_t   xvariant_dict_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t   xkey_file_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   xmain_loop_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   xmain_context_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_30
xtype_t   xsource_get_type          (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   xpollfd_get_type          (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   xthread_get_type          (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   xchecksum_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_36
xtype_t   xmarkup_parse_context_get_type (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_40
xtype_t   xmapped_file_get_type (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_44
xtype_t   xoption_group_get_type    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_66
xtype_t   xuri_get_type             (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_68
xtype_t   xtree_get_type            (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_70
xtype_t xpattern_spec_get_type (void) G_GNUC_CONST;

XPL_DEPRECATED_FOR('XTYPE_VARIANT')
xtype_t   xvariant_get_gtype        (void) G_GNUC_CONST;

G_END_DECLS

#endif  /* __XPL_TYPES_H__ */
