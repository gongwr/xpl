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

#include "config.h"

#include <string.h>

/* for xvalue_array_t */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include "gboxed.h"
#include "gclosure.h"
#include "gtype-private.h"
#include "gvalue.h"
#include "gvaluearray.h"
#include "gvaluecollector.h"


/**
 * SECTION:gboxed
 * @short_description: A mechanism to wrap opaque C structures registered
 *     by the type system
 * @see_also: #GParamSpecBoxed, g_param_spec_boxed()
 * @title: Boxed Types
 *
 * #GBoxed is a generic wrapper mechanism for arbitrary C structures.
 *
 * The only thing the type system needs to know about the structures is how to
 * copy them (a #GBoxedCopyFunc) and how to free them (a #GBoxedFreeFunc);
 * beyond that, they are treated as opaque chunks of memory.
 *
 * Boxed types are useful for simple value-holder structures like rectangles or
 * points. They can also be used for wrapping structures defined in non-#xobject_t
 * based libraries. They allow arbitrary structures to be handled in a uniform
 * way, allowing uniform copying (or referencing) and freeing (or unreferencing)
 * of them, and uniform representation of the type of the contained structure.
 * In turn, this allows any type which can be boxed to be set as the data in a
 * #xvalue_t, which allows for polymorphic handling of a much wider range of data
 * types, and hence usage of such types as #xobject_t property values.
 *
 * #GBoxed is designed so that reference counted types can be boxed. Use the
 * type’s ‘ref’ function as the #GBoxedCopyFunc, and its ‘unref’ function as the
 * #GBoxedFreeFunc. For example, for #xbytes_t, the #GBoxedCopyFunc is
 * xbytes_ref(), and the #GBoxedFreeFunc is xbytes_unref().
 */

static inline void              /* keep this function in sync with gvalue.c */
value_meminit (xvalue_t *value,
	       xtype_t   value_type)
{
  value->g_type = value_type;
  memset (value->data, 0, sizeof (value->data));
}

static xvalue_t *
value_copy (xvalue_t *src_value)
{
  xvalue_t *dest_value = g_new0 (xvalue_t, 1);

  if (G_VALUE_TYPE (src_value))
    {
      xvalue_init (dest_value, G_VALUE_TYPE (src_value));
      xvalue_copy (src_value, dest_value);
    }
  return dest_value;
}

static void
value_free (xvalue_t *value)
{
  if (G_VALUE_TYPE (value))
    xvalue_unset (value);
  g_free (value);
}

static xpollfd_t *
pollfd_copy (xpollfd_t *src)
{
  xpollfd_t *dest = g_new0 (xpollfd_t, 1);
  /* just a couple of integers */
  memcpy (dest, src, sizeof (xpollfd_t));
  return dest;
}

void
_xboxed_type_init (void)
{
  const xtype_info_t info = {
    0,                          /* class_size */
    NULL,                       /* base_init */
    NULL,                       /* base_destroy */
    NULL,                       /* class_init */
    NULL,                       /* class_destroy */
    NULL,                       /* class_data */
    0,                          /* instance_size */
    0,                          /* n_preallocs */
    NULL,                       /* instance_init */
    NULL,                       /* value_table */
  };
  const GTypeFundamentalInfo finfo = { XTYPE_FLAG_DERIVABLE, };
  xtype_t type G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  /* XTYPE_BOXED
   */
  type = xtype_register_fundamental (XTYPE_BOXED, g_intern_static_string ("GBoxed"), &info, &finfo,
				      XTYPE_FLAG_ABSTRACT | XTYPE_FLAG_VALUE_ABSTRACT);
  g_assert (type == XTYPE_BOXED);
}

static xstring_t *
x_string_copy (xstring_t *src_xstring)
{
  return xstring_new_len (src_xstring->str, src_xstring->len);
}

static void
x_string_free (xstring_t *xstring)
{
  xstring_free (xstring, TRUE);
}

G_DEFINE_BOXED_TYPE (xclosure_t, xclosure, xclosure_ref, xclosure_unref)
G_DEFINE_BOXED_TYPE (xvalue_t, xvalue, value_copy, value_free)
G_DEFINE_BOXED_TYPE (xvalue_array_t, xvalue_array, xvalue_array_copy, xvalue_array_free)
G_DEFINE_BOXED_TYPE (xdate_t, xdate, xdate_copy, xdate_free)
/* the naming is a bit odd, but xstring_t is obviously not XTYPE_STRING */
G_DEFINE_BOXED_TYPE (xstring_t, xstring, x_string_copy, x_string_free)
G_DEFINE_BOXED_TYPE (xhashtable_t, xhash_table, xhash_table_ref, xhash_table_unref)
G_DEFINE_BOXED_TYPE (xarray_t, g_array, g_array_ref, g_array_unref)
G_DEFINE_BOXED_TYPE (xptr_array_t, xptr_array,xptr_array_ref, xptr_array_unref)
G_DEFINE_BOXED_TYPE (xbyte_array_t, xbyte_array, xbyte_array_ref, xbyte_array_unref)
G_DEFINE_BOXED_TYPE (xbytes_t, xbytes, xbytes_ref, xbytes_unref)
G_DEFINE_BOXED_TYPE (xtree_t, xtree, xtree_ref, xtree_unref)

G_DEFINE_BOXED_TYPE (xregex_t, xregex, xregex_ref, xregex_unref)
G_DEFINE_BOXED_TYPE (xmatch_info_t, xmatch_info, xmatch_info_ref, xmatch_info_unref)

#define xvariant_type_get_type xvariant_type_get_gtype
G_DEFINE_BOXED_TYPE (xvariant_type_t, xvariant_type, xvariant_type_copy, xvariant_type_free)
#undef xvariant_type_get_type

G_DEFINE_BOXED_TYPE (xvariant_builder_t, xvariant_builder, xvariant_builder_ref, xvariant_builder_unref)
G_DEFINE_BOXED_TYPE (xvariant_dict_t, xvariant_dict, xvariant_dict_ref, xvariant_dict_unref)

G_DEFINE_BOXED_TYPE (xerror_t, xerror, xerror_copy, xerror_free)

G_DEFINE_BOXED_TYPE (xdatetime_t, xdate_time, xdate_time_ref, xdate_time_unref)
G_DEFINE_BOXED_TYPE (xtimezone_t, xtime_zone, xtime_zone_ref, xtime_zone_unref)
G_DEFINE_BOXED_TYPE (xkey_file_t, xkey_file, xkey_file_ref, xkey_file_unref)
G_DEFINE_BOXED_TYPE (xmapped_file_t, xmapped_file, xmapped_file_ref, xmapped_file_unref)

G_DEFINE_BOXED_TYPE (xmain_loop_t, xmain_loop, xmain_loop_ref, xmain_loop_unref)
G_DEFINE_BOXED_TYPE (xmain_context_t, xmain_context, xmain_context_ref, xmain_context_unref)
G_DEFINE_BOXED_TYPE (xsource_t, xsource, xsource_ref, xsource_unref)
G_DEFINE_BOXED_TYPE (xpollfd_t, xpollfd, pollfd_copy, g_free)
G_DEFINE_BOXED_TYPE (xmarkup_parse_context_t, xmarkup_parse_context, xmarkup_parse_context_ref, xmarkup_parse_context_unref)

G_DEFINE_BOXED_TYPE (xthread_t, xthread, xthread_ref, xthread_unref)
G_DEFINE_BOXED_TYPE (xchecksum_t, xchecksum, xchecksum_copy, xchecksum_free)
G_DEFINE_BOXED_TYPE (xuri_t, xuri, xuri_ref, xuri_unref)

G_DEFINE_BOXED_TYPE (xoption_group_t, xoption_group, xoption_group_ref, xoption_group_unref)
G_DEFINE_BOXED_TYPE (xpattern_spec_t, xpattern_spec, xpattern_spec_copy, xpattern_spec_free);

/* This one can't use G_DEFINE_BOXED_TYPE (xstrv_t, xstrv, xstrdupv, xstrfreev) */
xtype_t
xstrv_get_type (void)
{
  static xsize_t static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      xtype_t g_define_type_id =
        xboxed_type_register_static (g_intern_static_string ("xstrv_t"),
                                      (GBoxedCopyFunc) xstrdupv,
                                      (GBoxedFreeFunc) xstrfreev);

      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

xtype_t
xvariant_get_gtype (void)
{
  return XTYPE_VARIANT;
}

static void
boxed_proxy_value_init (xvalue_t *value)
{
  value->data[0].v_pointer = NULL;
}

static void
boxed_proxy_value_free (xvalue_t *value)
{
  if (value->data[0].v_pointer && !(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
    _xtype_boxed_free (G_VALUE_TYPE (value), value->data[0].v_pointer);
}

static void
boxed_proxy_value_copy (const xvalue_t *src_value,
			xvalue_t       *dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = _xtype_boxed_copy (G_VALUE_TYPE (src_value), src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = src_value->data[0].v_pointer;
}

static xpointer_t
boxed_proxy_value_peek_pointer (const xvalue_t *value)
{
  return value->data[0].v_pointer;
}

static xchar_t*
boxed_proxy_collect_value (xvalue_t      *value,
			   xuint_t        n_collect_values,
			   xtype_c_value_t *collect_values,
			   xuint_t        collect_flags)
{
  if (!collect_values[0].v_pointer)
    value->data[0].v_pointer = NULL;
  else
    {
      if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
	{
	  value->data[0].v_pointer = collect_values[0].v_pointer;
	  value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
	}
      else
	value->data[0].v_pointer = _xtype_boxed_copy (G_VALUE_TYPE (value), collect_values[0].v_pointer);
    }

  return NULL;
}

static xchar_t*
boxed_proxy_lcopy_value (const xvalue_t *value,
			 xuint_t         n_collect_values,
			 xtype_c_value_t  *collect_values,
			 xuint_t         collect_flags)
{
  xpointer_t *boxed_p = collect_values[0].v_pointer;

  g_return_val_if_fail (boxed_p != NULL, xstrdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  if (!value->data[0].v_pointer)
    *boxed_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *boxed_p = value->data[0].v_pointer;
  else
    *boxed_p = _xtype_boxed_copy (G_VALUE_TYPE (value), value->data[0].v_pointer);

  return NULL;
}

/**
 * xboxed_type_register_static:
 * @name: Name of the new boxed type.
 * @boxed_copy: Boxed structure copy function.
 * @boxed_free: Boxed structure free function.
 *
 * This function creates a new %XTYPE_BOXED derived type id for a new
 * boxed type with name @name.
 *
 * Boxed type handling functions have to be provided to copy and free
 * opaque boxed structures of this type.
 *
 * For the general case, it is recommended to use G_DEFINE_BOXED_TYPE()
 * instead of calling xboxed_type_register_static() directly. The macro
 * will create the appropriate `*_get_type()` function for the boxed type.
 *
 * Returns: New %XTYPE_BOXED derived type id for @name.
 */
xtype_t
xboxed_type_register_static (const xchar_t   *name,
			      GBoxedCopyFunc boxed_copy,
			      GBoxedFreeFunc boxed_free)
{
  static const xtype_value_table_t vtable = {
    boxed_proxy_value_init,
    boxed_proxy_value_free,
    boxed_proxy_value_copy,
    boxed_proxy_value_peek_pointer,
    "p",
    boxed_proxy_collect_value,
    "p",
    boxed_proxy_lcopy_value,
  };
  xtype_info_t type_info = {
    0,			/* class_size */
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    NULL,		/* class_init */
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    0,			/* instance_size */
    0,			/* n_preallocs */
    NULL,		/* instance_init */
    &vtable,		/* value_table */
  };
  xtype_t type;

  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (boxed_copy != NULL, 0);
  g_return_val_if_fail (boxed_free != NULL, 0);
  g_return_val_if_fail (xtype_from_name (name) == 0, 0);

  type = xtype_register_static (XTYPE_BOXED, name, &type_info, 0);

  /* install proxy functions upon successful registration */
  if (type)
    _xtype_boxed_init (type, boxed_copy, boxed_free);

  return type;
}

/**
 * xboxed_copy:
 * @boxed_type: The type of @src_boxed.
 * @src_boxed: (not nullable): The boxed structure to be copied.
 *
 * Provide a copy of a boxed structure @src_boxed which is of type @boxed_type.
 *
 * Returns: (transfer full) (not nullable): The newly created copy of the boxed
 *    structure.
 */
xpointer_t
xboxed_copy (xtype_t         boxed_type,
	      xconstpointer src_boxed)
{
  xtype_value_table_t *value_table;
  xpointer_t dest_boxed;

  g_return_val_if_fail (XTYPE_IS_BOXED (boxed_type), NULL);
  g_return_val_if_fail (XTYPE_IS_ABSTRACT (boxed_type) == FALSE, NULL);
  g_return_val_if_fail (src_boxed != NULL, NULL);

  value_table = xtype_value_table_peek (boxed_type);
  g_assert (value_table != NULL);

  /* check if our proxying implementation is used, we can short-cut here */
  if (value_table->value_copy == boxed_proxy_value_copy)
    dest_boxed = _xtype_boxed_copy (boxed_type, (xpointer_t) src_boxed);
  else
    {
      xvalue_t src_value, dest_value;

      /* we heavily rely on third-party boxed type value vtable
       * implementations to follow normal boxed value storage
       * (data[0].v_pointer is the boxed struct, and
       * data[1].v_uint holds the G_VALUE_NOCOPY_CONTENTS flag,
       * rest zero).
       * but then, we can expect that since we laid out the
       * xboxed_*() API.
       * data[1].v_uint&G_VALUE_NOCOPY_CONTENTS shouldn't be set
       * after a copy.
       */
      /* equiv. to xvalue_set_static_boxed() */
      value_meminit (&src_value, boxed_type);
      src_value.data[0].v_pointer = (xpointer_t) src_boxed;
      src_value.data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;

      /* call third-party code copy function, fingers-crossed */
      value_meminit (&dest_value, boxed_type);
      value_table->value_copy (&src_value, &dest_value);

      /* double check and grouse if things went wrong */
      if (dest_value.data[1].v_ulong)
	g_warning ("the copy_value() implementation of type '%s' seems to make use of reserved xvalue_t fields",
		   xtype_name (boxed_type));

      dest_boxed = dest_value.data[0].v_pointer;
    }

  return dest_boxed;
}

/**
 * xboxed_free:
 * @boxed_type: The type of @boxed.
 * @boxed: (not nullable): The boxed structure to be freed.
 *
 * Free the boxed structure @boxed which is of type @boxed_type.
 */
void
xboxed_free (xtype_t    boxed_type,
	      xpointer_t boxed)
{
  xtype_value_table_t *value_table;

  g_return_if_fail (XTYPE_IS_BOXED (boxed_type));
  g_return_if_fail (XTYPE_IS_ABSTRACT (boxed_type) == FALSE);
  g_return_if_fail (boxed != NULL);

  value_table = xtype_value_table_peek (boxed_type);
  g_assert (value_table != NULL);

  /* check if our proxying implementation is used, we can short-cut here */
  if (value_table->value_free == boxed_proxy_value_free)
    _xtype_boxed_free (boxed_type, boxed);
  else
    {
      xvalue_t value;

      /* see xboxed_copy() on why we think we can do this */
      value_meminit (&value, boxed_type);
      value.data[0].v_pointer = boxed;
      value_table->value_free (&value);
    }
}

/**
 * xvalue_get_boxed:
 * @value: a valid #xvalue_t of %XTYPE_BOXED derived type
 *
 * Get the contents of a %XTYPE_BOXED derived #xvalue_t.
 *
 * Returns: (transfer none): boxed contents of @value
 */
xpointer_t
xvalue_get_boxed (const xvalue_t *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_BOXED (value), NULL);
  g_return_val_if_fail (XTYPE_IS_VALUE (G_VALUE_TYPE (value)), NULL);

  return value->data[0].v_pointer;
}

/**
 * xvalue_dup_boxed: (skip)
 * @value: a valid #xvalue_t of %XTYPE_BOXED derived type
 *
 * Get the contents of a %XTYPE_BOXED derived #xvalue_t.  Upon getting,
 * the boxed value is duplicated and needs to be later freed with
 * xboxed_free(), e.g. like: xboxed_free (G_VALUE_TYPE (@value),
 * return_value);
 *
 * Returns: boxed contents of @value
 */
xpointer_t
xvalue_dup_boxed (const xvalue_t *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_BOXED (value), NULL);
  g_return_val_if_fail (XTYPE_IS_VALUE (G_VALUE_TYPE (value)), NULL);

  return value->data[0].v_pointer ? xboxed_copy (G_VALUE_TYPE (value), value->data[0].v_pointer) : NULL;
}

static inline void
value_set_boxed_internal (xvalue_t       *value,
			  xconstpointer boxed,
			  xboolean_t      need_copy,
			  xboolean_t      need_free)
{
  if (!boxed)
    {
      /* just resetting to NULL might not be desired, need to
       * have value reinitialized also (for values defaulting
       * to other default value states than a NULL data pointer),
       * xvalue_reset() will handle this
       */
      xvalue_reset (value);
      return;
    }

  if (value->data[0].v_pointer && !(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
    xboxed_free (G_VALUE_TYPE (value), value->data[0].v_pointer);
  value->data[1].v_uint = need_free ? 0 : G_VALUE_NOCOPY_CONTENTS;
  value->data[0].v_pointer = need_copy ? xboxed_copy (G_VALUE_TYPE (value), boxed) : (xpointer_t) boxed;
}

/**
 * xvalue_set_boxed:
 * @value: a valid #xvalue_t of %XTYPE_BOXED derived type
 * @v_boxed: (nullable): boxed value to be set
 *
 * Set the contents of a %XTYPE_BOXED derived #xvalue_t to @v_boxed.
 */
void
xvalue_set_boxed (xvalue_t       *value,
		   xconstpointer boxed)
{
  g_return_if_fail (G_VALUE_HOLDS_BOXED (value));
  g_return_if_fail (XTYPE_IS_VALUE (G_VALUE_TYPE (value)));

  value_set_boxed_internal (value, boxed, TRUE, TRUE);
}

/**
 * xvalue_set_static_boxed:
 * @value: a valid #xvalue_t of %XTYPE_BOXED derived type
 * @v_boxed: (nullable): static boxed value to be set
 *
 * Set the contents of a %XTYPE_BOXED derived #xvalue_t to @v_boxed.
 *
 * The boxed value is assumed to be static, and is thus not duplicated
 * when setting the #xvalue_t.
 */
void
xvalue_set_static_boxed (xvalue_t       *value,
			  xconstpointer boxed)
{
  g_return_if_fail (G_VALUE_HOLDS_BOXED (value));
  g_return_if_fail (XTYPE_IS_VALUE (G_VALUE_TYPE (value)));

  value_set_boxed_internal (value, boxed, FALSE, FALSE);
}

/**
 * xvalue_set_boxed_take_ownership:
 * @value: a valid #xvalue_t of %XTYPE_BOXED derived type
 * @v_boxed: (nullable): duplicated unowned boxed value to be set
 *
 * This is an internal function introduced mainly for C marshallers.
 *
 * Deprecated: 2.4: Use xvalue_take_boxed() instead.
 */
void
xvalue_set_boxed_take_ownership (xvalue_t       *value,
				  xconstpointer boxed)
{
  xvalue_take_boxed (value, boxed);
}

/**
 * xvalue_take_boxed:
 * @value: a valid #xvalue_t of %XTYPE_BOXED derived type
 * @v_boxed: (nullable): duplicated unowned boxed value to be set
 *
 * Sets the contents of a %XTYPE_BOXED derived #xvalue_t to @v_boxed
 * and takes over the ownership of the caller’s reference to @v_boxed;
 * the caller doesn’t have to unref it any more.
 *
 * Since: 2.4
 */
void
xvalue_take_boxed (xvalue_t       *value,
		    xconstpointer boxed)
{
  g_return_if_fail (G_VALUE_HOLDS_BOXED (value));
  g_return_if_fail (XTYPE_IS_VALUE (G_VALUE_TYPE (value)));

  value_set_boxed_internal (value, boxed, FALSE, TRUE);
}
