/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
 *
 * gparamspecs.h: GLib default param specs
 */
#ifndef __G_PARAMSPECS_H__
#define __G_PARAMSPECS_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include        <gobject/gvalue.h>
#include        <gobject/genums.h>
#include        <gobject/gboxed.h>
#include        <gobject/gobject.h>

G_BEGIN_DECLS

/* --- type macros --- */
/**
 * XTYPE_PARAM_CHAR:
 *
 * The #xtype_t of #GParamSpecChar.
 */
#define	XTYPE_PARAM_CHAR		   (g_param_spec_types[0])
/**
 * X_IS_PARAM_SPEC_CHAR:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_CHAR.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_CHAR(pspec)        (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_CHAR))
/**
 * G_PARAM_SPEC_CHAR:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecChar.
 */
#define G_PARAM_SPEC_CHAR(pspec)           (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_CHAR, GParamSpecChar))

/**
 * XTYPE_PARAM_UCHAR:
 *
 * The #xtype_t of #GParamSpecUChar.
 */
#define	XTYPE_PARAM_UCHAR		   (g_param_spec_types[1])
/**
 * X_IS_PARAM_SPEC_UCHAR:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_UCHAR.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_UCHAR(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_UCHAR))
/**
 * G_PARAM_SPEC_UCHAR:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecUChar.
 */
#define G_PARAM_SPEC_UCHAR(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_UCHAR, GParamSpecUChar))

/**
 * XTYPE_PARAM_BOOLEAN:
 *
 * The #xtype_t of #GParamSpecBoolean.
 */
#define	XTYPE_PARAM_BOOLEAN		   (g_param_spec_types[2])
/**
 * X_IS_PARAM_SPEC_BOOLEAN:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_BOOLEAN.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_BOOLEAN(pspec)     (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_BOOLEAN))
/**
 * G_PARAM_SPEC_BOOLEAN:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecBoolean.
 */
#define G_PARAM_SPEC_BOOLEAN(pspec)        (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_BOOLEAN, GParamSpecBoolean))

/**
 * XTYPE_PARAM_INT:
 *
 * The #xtype_t of #GParamSpecInt.
 */
#define	XTYPE_PARAM_INT		   (g_param_spec_types[3])
/**
 * X_IS_PARAM_SPEC_INT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_INT.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_INT(pspec)         (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_INT))
/**
 * G_PARAM_SPEC_INT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecInt.
 */
#define G_PARAM_SPEC_INT(pspec)            (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_INT, GParamSpecInt))

/**
 * XTYPE_PARAM_UINT:
 *
 * The #xtype_t of #GParamSpecUInt.
 */
#define	XTYPE_PARAM_UINT		   (g_param_spec_types[4])
/**
 * X_IS_PARAM_SPEC_UINT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_UINT.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_UINT(pspec)        (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_UINT))
/**
 * G_PARAM_SPEC_UINT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecUInt.
 */
#define G_PARAM_SPEC_UINT(pspec)           (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_UINT, GParamSpecUInt))

/**
 * XTYPE_PARAM_LONG:
 *
 * The #xtype_t of #GParamSpecLong.
 */
#define	XTYPE_PARAM_LONG		   (g_param_spec_types[5])
/**
 * X_IS_PARAM_SPEC_LONG:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_LONG.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_LONG(pspec)        (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_LONG))
/**
 * G_PARAM_SPEC_LONG:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecLong.
 */
#define G_PARAM_SPEC_LONG(pspec)           (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_LONG, GParamSpecLong))

/**
 * XTYPE_PARAM_ULONG:
 *
 * The #xtype_t of #GParamSpecULong.
 */
#define	XTYPE_PARAM_ULONG		   (g_param_spec_types[6])
/**
 * X_IS_PARAM_SPEC_ULONG:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_ULONG.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_ULONG(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_ULONG))
/**
 * G_PARAM_SPEC_ULONG:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecULong.
 */
#define G_PARAM_SPEC_ULONG(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_ULONG, GParamSpecULong))

/**
 * XTYPE_PARAM_INT64:
 *
 * The #xtype_t of #GParamSpecInt64.
 */
#define	XTYPE_PARAM_INT64		   (g_param_spec_types[7])
/**
 * X_IS_PARAM_SPEC_INT64:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_INT64.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_INT64(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_INT64))
/**
 * G_PARAM_SPEC_INT64:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecInt64.
 */
#define G_PARAM_SPEC_INT64(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_INT64, GParamSpecInt64))

/**
 * XTYPE_PARAM_UINT64:
 *
 * The #xtype_t of #GParamSpecUInt64.
 */
#define	XTYPE_PARAM_UINT64		   (g_param_spec_types[8])
/**
 * X_IS_PARAM_SPEC_UINT64:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_UINT64.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_UINT64(pspec)      (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_UINT64))
/**
 * G_PARAM_SPEC_UINT64:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecUInt64.
 */
#define G_PARAM_SPEC_UINT64(pspec)         (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_UINT64, GParamSpecUInt64))

/**
 * XTYPE_PARAM_UNICHAR:
 *
 * The #xtype_t of #GParamSpecUnichar.
 */
#define	XTYPE_PARAM_UNICHAR		   (g_param_spec_types[9])
/**
 * G_PARAM_SPEC_UNICHAR:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecUnichar.
 */
#define G_PARAM_SPEC_UNICHAR(pspec)        (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_UNICHAR, GParamSpecUnichar))
/**
 * X_IS_PARAM_SPEC_UNICHAR:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_UNICHAR.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_UNICHAR(pspec)     (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_UNICHAR))

/**
 * XTYPE_PARAM_ENUM:
 *
 * The #xtype_t of #GParamSpecEnum.
 */
#define	XTYPE_PARAM_ENUM		   (g_param_spec_types[10])
/**
 * X_IS_PARAM_SPEC_ENUM:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_ENUM.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_ENUM(pspec)        (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_ENUM))
/**
 * G_PARAM_SPEC_ENUM:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecEnum.
 */
#define G_PARAM_SPEC_ENUM(pspec)           (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_ENUM, GParamSpecEnum))

/**
 * XTYPE_PARAM_FLAGS:
 *
 * The #xtype_t of #GParamSpecFlags.
 */
#define	XTYPE_PARAM_FLAGS		   (g_param_spec_types[11])
/**
 * X_IS_PARAM_SPEC_FLAGS:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_FLAGS.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_FLAGS(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_FLAGS))
/**
 * G_PARAM_SPEC_FLAGS:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecFlags.
 */
#define G_PARAM_SPEC_FLAGS(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_FLAGS, GParamSpecFlags))

/**
 * XTYPE_PARAM_FLOAT:
 *
 * The #xtype_t of #GParamSpecFloat.
 */
#define	XTYPE_PARAM_FLOAT		   (g_param_spec_types[12])
/**
 * X_IS_PARAM_SPEC_FLOAT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_FLOAT.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_FLOAT(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_FLOAT))
/**
 * G_PARAM_SPEC_FLOAT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecFloat.
 */
#define G_PARAM_SPEC_FLOAT(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_FLOAT, GParamSpecFloat))

/**
 * XTYPE_PARAM_DOUBLE:
 *
 * The #xtype_t of #GParamSpecDouble.
 */
#define	XTYPE_PARAM_DOUBLE		   (g_param_spec_types[13])
/**
 * X_IS_PARAM_SPEC_DOUBLE:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_DOUBLE.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_DOUBLE(pspec)      (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_DOUBLE))
/**
 * G_PARAM_SPEC_DOUBLE:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecDouble.
 */
#define G_PARAM_SPEC_DOUBLE(pspec)         (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_DOUBLE, GParamSpecDouble))

/**
 * XTYPE_PARAM_STRING:
 *
 * The #xtype_t of #GParamSpecString.
 */
#define	XTYPE_PARAM_STRING		   (g_param_spec_types[14])
/**
 * X_IS_PARAM_SPEC_STRING:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_STRING.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_STRING(pspec)      (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_STRING))
/**
 * G_PARAM_SPEC_STRING:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Casts a #xparam_spec_t instance into a #GParamSpecString.
 */
#define G_PARAM_SPEC_STRING(pspec)         (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_STRING, GParamSpecString))

/**
 * XTYPE_PARAM_PARAM:
 *
 * The #xtype_t of #GParamSpecParam.
 */
#define	XTYPE_PARAM_PARAM		   (g_param_spec_types[15])
/**
 * X_IS_PARAM_SPEC_PARAM:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_PARAM.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_PARAM(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_PARAM))
/**
 * G_PARAM_SPEC_PARAM:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Casts a #xparam_spec_t instance into a #GParamSpecParam.
 */
#define G_PARAM_SPEC_PARAM(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_PARAM, GParamSpecParam))

/**
 * XTYPE_PARAM_BOXED:
 *
 * The #xtype_t of #GParamSpecBoxed.
 */
#define	XTYPE_PARAM_BOXED		   (g_param_spec_types[16])
/**
 * X_IS_PARAM_SPEC_BOXED:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_BOXED.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_BOXED(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_BOXED))
/**
 * G_PARAM_SPEC_BOXED:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecBoxed.
 */
#define G_PARAM_SPEC_BOXED(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_BOXED, GParamSpecBoxed))

/**
 * XTYPE_PARAM_POINTER:
 *
 * The #xtype_t of #GParamSpecPointer.
 */
#define	XTYPE_PARAM_POINTER		   (g_param_spec_types[17])
/**
 * X_IS_PARAM_SPEC_POINTER:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_POINTER.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_POINTER(pspec)     (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_POINTER))
/**
 * G_PARAM_SPEC_POINTER:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Casts a #xparam_spec_t instance into a #GParamSpecPointer.
 */
#define G_PARAM_SPEC_POINTER(pspec)        (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_POINTER, GParamSpecPointer))

/**
 * XTYPE_PARAM_VALUE_ARRAY:
 *
 * The #xtype_t of #GParamSpecValueArray.
 *
 * Deprecated: 2.32: Use #xarray_t instead of #xvalue_array_t
 */
#define	XTYPE_PARAM_VALUE_ARRAY	   (g_param_spec_types[18]) XPL_DEPRECATED_MACRO_IN_2_32
/**
 * X_IS_PARAM_SPEC_VALUE_ARRAY:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_VALUE_ARRAY.
 *
 * Returns: %TRUE on success.
 *
 * Deprecated: 2.32: Use #xarray_t instead of #xvalue_array_t
 */
#define X_IS_PARAM_SPEC_VALUE_ARRAY(pspec) (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_VALUE_ARRAY)) XPL_DEPRECATED_MACRO_IN_2_32
/**
 * G_PARAM_SPEC_VALUE_ARRAY:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Cast a #xparam_spec_t instance into a #GParamSpecValueArray.
 *
 * Deprecated: 2.32: Use #xarray_t instead of #xvalue_array_t
 */
#define G_PARAM_SPEC_VALUE_ARRAY(pspec)    (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_VALUE_ARRAY, GParamSpecValueArray)) XPL_DEPRECATED_MACRO_IN_2_32

/**
 * XTYPE_PARAM_OBJECT:
 *
 * The #xtype_t of #GParamSpecObject.
 */
#define	XTYPE_PARAM_OBJECT		   (g_param_spec_types[19])
/**
 * X_IS_PARAM_SPEC_OBJECT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_OBJECT.
 *
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_OBJECT(pspec)      (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_OBJECT))
/**
 * G_PARAM_SPEC_OBJECT:
 * @pspec: a valid #xparam_spec_t instance
 *
 * Casts a #xparam_spec_t instance into a #GParamSpecObject.
 */
#define G_PARAM_SPEC_OBJECT(pspec)         (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_OBJECT, GParamSpecObject))

/**
 * XTYPE_PARAM_OVERRIDE:
 *
 * The #xtype_t of #GParamSpecOverride.
 *
 * Since: 2.4
 */
#define	XTYPE_PARAM_OVERRIDE		   (g_param_spec_types[20])
/**
 * X_IS_PARAM_SPEC_OVERRIDE:
 * @pspec: a #xparam_spec_t
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_OVERRIDE.
 *
 * Since: 2.4
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_OVERRIDE(pspec)    (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_OVERRIDE))
/**
 * G_PARAM_SPEC_OVERRIDE:
 * @pspec: a #xparam_spec_t
 *
 * Casts a #xparam_spec_t into a #GParamSpecOverride.
 *
 * Since: 2.4
 */
#define G_PARAM_SPEC_OVERRIDE(pspec)       (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_OVERRIDE, GParamSpecOverride))

/**
 * XTYPE_PARAM_GTYPE:
 *
 * The #xtype_t of #GParamSpecGType.
 *
 * Since: 2.10
 */
#define	XTYPE_PARAM_GTYPE		   (g_param_spec_types[21])
/**
 * X_IS_PARAM_SPEC_GTYPE:
 * @pspec: a #xparam_spec_t
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_GTYPE.
 *
 * Since: 2.10
 * Returns: %TRUE on success.
 */
#define X_IS_PARAM_SPEC_GTYPE(pspec)       (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_GTYPE))
/**
 * G_PARAM_SPEC_GTYPE:
 * @pspec: a #xparam_spec_t
 *
 * Casts a #xparam_spec_t into a #GParamSpecGType.
 *
 * Since: 2.10
 */
#define G_PARAM_SPEC_GTYPE(pspec)          (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_GTYPE, GParamSpecGType))

/**
 * XTYPE_PARAM_VARIANT:
 *
 * The #xtype_t of #GParamSpecVariant.
 *
 * Since: 2.26
 */
#define XTYPE_PARAM_VARIANT                (g_param_spec_types[22])
/**
 * X_IS_PARAM_SPEC_VARIANT:
 * @pspec: a #xparam_spec_t
 *
 * Checks whether the given #xparam_spec_t is of type %XTYPE_PARAM_VARIANT.
 *
 * Returns: %TRUE on success
 *
 * Since: 2.26
 */
#define X_IS_PARAM_SPEC_VARIANT(pspec)      (XTYPE_CHECK_INSTANCE_TYPE ((pspec), XTYPE_PARAM_VARIANT))
/**
 * G_PARAM_SPEC_VARIANT:
 * @pspec: a #xparam_spec_t
 *
 * Casts a #xparam_spec_t into a #GParamSpecVariant.
 *
 * Since: 2.26
 */
#define G_PARAM_SPEC_VARIANT(pspec)         (XTYPE_CHECK_INSTANCE_CAST ((pspec), XTYPE_PARAM_VARIANT, GParamSpecVariant))

/* --- typedefs & structures --- */
typedef struct _GParamSpecChar       GParamSpecChar;
typedef struct _GParamSpecUChar      GParamSpecUChar;
typedef struct _GParamSpecBoolean    GParamSpecBoolean;
typedef struct _GParamSpecInt        GParamSpecInt;
typedef struct _GParamSpecUInt       GParamSpecUInt;
typedef struct _GParamSpecLong       GParamSpecLong;
typedef struct _GParamSpecULong      GParamSpecULong;
typedef struct _GParamSpecInt64      GParamSpecInt64;
typedef struct _GParamSpecUInt64     GParamSpecUInt64;
typedef struct _GParamSpecUnichar    GParamSpecUnichar;
typedef struct _GParamSpecEnum       GParamSpecEnum;
typedef struct _GParamSpecFlags      GParamSpecFlags;
typedef struct _GParamSpecFloat      GParamSpecFloat;
typedef struct _GParamSpecDouble     GParamSpecDouble;
typedef struct _GParamSpecString     GParamSpecString;
typedef struct _GParamSpecParam      GParamSpecParam;
typedef struct _GParamSpecBoxed      GParamSpecBoxed;
typedef struct _GParamSpecPointer    GParamSpecPointer;
typedef struct _GParamSpecValueArray GParamSpecValueArray;
typedef struct _GParamSpecObject     GParamSpecObject;
typedef struct _GParamSpecOverride   GParamSpecOverride;
typedef struct _GParamSpecGType      GParamSpecGType;
typedef struct _GParamSpecVariant    GParamSpecVariant;

/**
 * GParamSpecChar:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for character properties.
 */
struct _GParamSpecChar
{
  xparam_spec_t    parent_instance;

  gint8         minimum;
  gint8         maximum;
  gint8         default_value;
};
/**
 * GParamSpecUChar:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for unsigned character properties.
 */
struct _GParamSpecUChar
{
  xparam_spec_t    parent_instance;

  xuint8_t        minimum;
  xuint8_t        maximum;
  xuint8_t        default_value;
};
/**
 * GParamSpecBoolean:
 * @parent_instance: private #xparam_spec_t portion
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for boolean properties.
 */
struct _GParamSpecBoolean
{
  xparam_spec_t    parent_instance;

  xboolean_t      default_value;
};
/**
 * GParamSpecInt:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for integer properties.
 */
struct _GParamSpecInt
{
  xparam_spec_t    parent_instance;

  xint_t          minimum;
  xint_t          maximum;
  xint_t          default_value;
};
/**
 * GParamSpecUInt:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for unsigned integer properties.
 */
struct _GParamSpecUInt
{
  xparam_spec_t    parent_instance;

  xuint_t         minimum;
  xuint_t         maximum;
  xuint_t         default_value;
};
/**
 * GParamSpecLong:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for long integer properties.
 */
struct _GParamSpecLong
{
  xparam_spec_t    parent_instance;

  xlong_t         minimum;
  xlong_t         maximum;
  xlong_t         default_value;
};
/**
 * GParamSpecULong:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for unsigned long integer properties.
 */
struct _GParamSpecULong
{
  xparam_spec_t    parent_instance;

  gulong        minimum;
  gulong        maximum;
  gulong        default_value;
};
/**
 * GParamSpecInt64:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for 64bit integer properties.
 */
struct _GParamSpecInt64
{
  xparam_spec_t    parent_instance;

  gint64        minimum;
  gint64        maximum;
  gint64        default_value;
};
/**
 * GParamSpecUInt64:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for unsigned 64bit integer properties.
 */
struct _GParamSpecUInt64
{
  xparam_spec_t    parent_instance;

  xuint64_t       minimum;
  xuint64_t       maximum;
  xuint64_t       default_value;
};
/**
 * GParamSpecUnichar:
 * @parent_instance: private #xparam_spec_t portion
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for unichar (unsigned integer) properties.
 */
struct _GParamSpecUnichar
{
  xparam_spec_t    parent_instance;

  xunichar_t      default_value;
};
/**
 * GParamSpecEnum:
 * @parent_instance: private #xparam_spec_t portion
 * @enum_class: the #xenum_class_t for the enum
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for enum
 * properties.
 */
struct _GParamSpecEnum
{
  xparam_spec_t    parent_instance;

  xenum_class_t   *enum_class;
  xint_t          default_value;
};
/**
 * GParamSpecFlags:
 * @parent_instance: private #xparam_spec_t portion
 * @flags_class: the #xflags_class_t for the flags
 * @default_value: default value for the property specified
 *
 * A #xparam_spec_t derived structure that contains the meta data for flags
 * properties.
 */
struct _GParamSpecFlags
{
  xparam_spec_t    parent_instance;

  xflags_class_t  *flags_class;
  xuint_t         default_value;
};
/**
 * GParamSpecFloat:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 * @epsilon: values closer than @epsilon will be considered identical
 *  by g_param_values_cmp(); the default value is 1e-30.
 *
 * A #xparam_spec_t derived structure that contains the meta data for float properties.
 */
struct _GParamSpecFloat
{
  xparam_spec_t    parent_instance;

  gfloat        minimum;
  gfloat        maximum;
  gfloat        default_value;
  gfloat        epsilon;
};
/**
 * GParamSpecDouble:
 * @parent_instance: private #xparam_spec_t portion
 * @minimum: minimum value for the property specified
 * @maximum: maximum value for the property specified
 * @default_value: default value for the property specified
 * @epsilon: values closer than @epsilon will be considered identical
 *  by g_param_values_cmp(); the default value is 1e-90.
 *
 * A #xparam_spec_t derived structure that contains the meta data for double properties.
 */
struct _GParamSpecDouble
{
  xparam_spec_t    parent_instance;

  xdouble_t       minimum;
  xdouble_t       maximum;
  xdouble_t       default_value;
  xdouble_t       epsilon;
};
/**
 * GParamSpecString:
 * @parent_instance: private #xparam_spec_t portion
 * @default_value: default value for the property specified
 * @cset_first: a string containing the allowed values for the first byte
 * @cset_nth: a string containing the allowed values for the subsequent bytes
 * @substitutor: the replacement byte for bytes which don't match @cset_first or @cset_nth.
 * @null_fold_if_empty: replace empty string by %NULL
 * @ensure_non_null: replace %NULL strings by an empty string
 *
 * A #xparam_spec_t derived structure that contains the meta data for string
 * properties.
 */
struct _GParamSpecString
{
  xparam_spec_t    parent_instance;

  xchar_t        *default_value;
  xchar_t        *cset_first;
  xchar_t        *cset_nth;
  xchar_t         substitutor;
  xuint_t         null_fold_if_empty : 1;
  xuint_t         ensure_non_null : 1;
};
/**
 * GParamSpecParam:
 * @parent_instance: private #xparam_spec_t portion
 *
 * A #xparam_spec_t derived structure that contains the meta data for %XTYPE_PARAM
 * properties.
 */
struct _GParamSpecParam
{
  xparam_spec_t    parent_instance;
};
/**
 * GParamSpecBoxed:
 * @parent_instance: private #xparam_spec_t portion
 *
 * A #xparam_spec_t derived structure that contains the meta data for boxed properties.
 */
struct _GParamSpecBoxed
{
  xparam_spec_t    parent_instance;
};
/**
 * GParamSpecPointer:
 * @parent_instance: private #xparam_spec_t portion
 *
 * A #xparam_spec_t derived structure that contains the meta data for pointer properties.
 */
struct _GParamSpecPointer
{
  xparam_spec_t    parent_instance;
};
/**
 * GParamSpecValueArray:
 * @parent_instance: private #xparam_spec_t portion
 * @element_spec: a #xparam_spec_t describing the elements contained in arrays of this property, may be %NULL
 * @fixed_n_elements: if greater than 0, arrays of this property will always have this many elements
 *
 * A #xparam_spec_t derived structure that contains the meta data for #xvalue_array_t properties.
 */
struct _GParamSpecValueArray
{
  xparam_spec_t    parent_instance;
  xparam_spec_t   *element_spec;
  xuint_t		fixed_n_elements;
};
/**
 * GParamSpecObject:
 * @parent_instance: private #xparam_spec_t portion
 *
 * A #xparam_spec_t derived structure that contains the meta data for object properties.
 */
struct _GParamSpecObject
{
  xparam_spec_t    parent_instance;
};
/**
 * GParamSpecOverride:
 *
 * A #xparam_spec_t derived structure that redirects operations to
 * other types of #xparam_spec_t.
 *
 * All operations other than getting or setting the value are redirected,
 * including accessing the nick and blurb, validating a value, and so
 * forth.
 *
 * See g_param_spec_get_redirect_target() for retrieving the overridden
 * property. #GParamSpecOverride is used in implementing
 * xobject_class_override_property(), and will not be directly useful
 * unless you are implementing a new base type similar to xobject_t.
 *
 * Since: 2.4
 */
struct _GParamSpecOverride
{
  /*< private >*/
  xparam_spec_t    parent_instance;
  xparam_spec_t   *overridden;
};
/**
 * GParamSpecGType:
 * @parent_instance: private #xparam_spec_t portion
 * @is_a_type: a #xtype_t whose subtypes can occur as values
 *
 * A #xparam_spec_t derived structure that contains the meta data for #xtype_t properties.
 *
 * Since: 2.10
 */
struct _GParamSpecGType
{
  xparam_spec_t    parent_instance;
  xtype_t         is_a_type;
};
/**
 * GParamSpecVariant:
 * @parent_instance: private #xparam_spec_t portion
 * @type: a #xvariant_type_t, or %NULL
 * @default_value: a #xvariant_t, or %NULL
 *
 * A #xparam_spec_t derived structure that contains the meta data for #xvariant_t properties.
 *
 * When comparing values with g_param_values_cmp(), scalar values with the same
 * type will be compared with xvariant_compare(). Other non-%NULL variants will
 * be checked for equality with xvariant_equal(), and their sort order is
 * otherwise undefined. %NULL is ordered before non-%NULL variants. Two %NULL
 * values compare equal.
 *
 * Since: 2.26
 */
struct _GParamSpecVariant
{
  xparam_spec_t    parent_instance;
  xvariant_type_t *type;
  xvariant_t     *default_value;

  /*< private >*/
  xpointer_t      padding[4];
};

/* --- xparam_spec_t prototypes --- */
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_char	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  gint8		  minimum,
					  gint8		  maximum,
					  gint8		  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_uchar	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xuint8_t	  minimum,
					  xuint8_t	  maximum,
					  xuint8_t	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_boolean	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xboolean_t	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_int	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xint_t		  minimum,
					  xint_t		  maximum,
					  xint_t		  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_uint	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xuint_t		  minimum,
					  xuint_t		  maximum,
					  xuint_t		  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_long	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xlong_t		  minimum,
					  xlong_t		  maximum,
					  xlong_t		  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_ulong	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  gulong	  minimum,
					  gulong	  maximum,
					  gulong	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_int64	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  gint64       	  minimum,
					  gint64       	  maximum,
					  gint64       	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_uint64	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xuint64_t	  minimum,
					  xuint64_t	  maximum,
					  xuint64_t	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_unichar      (const xchar_t    *name,
				          const xchar_t    *nick,
				          const xchar_t    *blurb,
				          xunichar_t	  default_value,
				          GParamFlags     flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_enum	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xtype_t		  enum_type,
					  xint_t		  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_flags	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xtype_t		  flags_type,
					  xuint_t		  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_float	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  gfloat	  minimum,
					  gfloat	  maximum,
					  gfloat	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_double	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xdouble_t	  minimum,
					  xdouble_t	  maximum,
					  xdouble_t	  default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_string	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  const xchar_t	 *default_value,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_param	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xtype_t		  param_type,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_boxed	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xtype_t		  boxed_type,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_pointer	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_value_array (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xparam_spec_t	 *element_spec,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_object	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xtype_t		  object_type,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_override    (const xchar_t    *name,
					  xparam_spec_t     *overridden);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_gtype	 (const xchar_t	 *name,
					  const xchar_t	 *nick,
					  const xchar_t	 *blurb,
					  xtype_t           is_a_type,
					  GParamFlags	  flags);
XPL_AVAILABLE_IN_ALL
xparam_spec_t*	g_param_spec_variant	 (const xchar_t        *name,
					  const xchar_t        *nick,
					  const xchar_t	     *blurb,
					  const xvariant_type_t *type,
					  xvariant_t           *default_value,
					  GParamFlags         flags);

/* --- internal --- */
/* We prefix variable declarations so they can
 * properly get exported in windows dlls.
 */
#ifndef GOBJECT_VAR
#  ifdef XPLATFORM_WIN32
#    ifdef GOBJECT_STATIC_COMPILATION
#      define GOBJECT_VAR extern
#    else /* !GOBJECT_STATIC_COMPILATION */
#      ifdef GOBJECT_COMPILATION
#        ifdef DLL_EXPORT
#          define GOBJECT_VAR extern __declspec(dllexport)
#        else /* !DLL_EXPORT */
#          define GOBJECT_VAR extern
#        endif /* !DLL_EXPORT */
#      else /* !GOBJECT_COMPILATION */
#        define GOBJECT_VAR extern __declspec(dllimport)
#      endif /* !GOBJECT_COMPILATION */
#    endif /* !GOBJECT_STATIC_COMPILATION */
#  else /* !XPLATFORM_WIN32 */
#    define GOBJECT_VAR _XPL_EXTERN
#  endif /* !XPLATFORM_WIN32 */
#endif /* GOBJECT_VAR */

GOBJECT_VAR xtype_t *g_param_spec_types;

G_END_DECLS

#endif /* __G_PARAMSPECS_H__ */
