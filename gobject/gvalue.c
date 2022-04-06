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
 */

/*
 * FIXME: MT-safety
 */

#include "config.h"

#include <string.h>

#include "gvalue.h"
#include "gvaluecollector.h"
#include "gbsearcharray.h"
#include "gtype-private.h"


/**
 * SECTION:generic_values
 * @short_description: A polymorphic type that can hold values of any
 *     other type
 * @see_also: The fundamental types which all support #xvalue_t
 *     operations and thus can be used as a type initializer for
 *     xvalue_init() are defined by a separate interface.  See the
 *     [standard values API][gobject-Standard-Parameter-and-Value-Types]
 *     for details
 * @title: Generic values
 *
 * The #xvalue_t structure is basically a variable container that consists
 * of a type identifier and a specific value of that type.
 *
 * The type identifier within a #xvalue_t structure always determines the
 * type of the associated value.
 *
 * To create an undefined #xvalue_t structure, simply create a zero-filled
 * #xvalue_t structure. To initialize the #xvalue_t, use the xvalue_init()
 * function. A #xvalue_t cannot be used until it is initialized. Before
 * destruction you must always use xvalue_unset() to make sure allocated
 * memory is freed.
 *
 * The basic type operations (such as freeing and copying) are determined
 * by the #xtype_value_table_t associated with the type ID stored in the #xvalue_t.
 * Other #xvalue_t operations (such as converting values between types) are
 * provided by this interface.
 *
 * The code in the example program below demonstrates #xvalue_t's
 * features.
 *
 * |[<!-- language="C" -->
 * #include <glib-object.h>
 *
 * static void
 * int2string (const xvalue_t *src_value,
 *             xvalue_t       *dest_value)
 * {
 *   if (xvalue_get_int (src_value) == 42)
 *     xvalue_set_static_string (dest_value, "An important number");
 *   else
 *     xvalue_set_static_string (dest_value, "What's that?");
 * }
 *
 * int
 * main (int   argc,
 *       char *argv[])
 * {
 *   // GValues must be initialized
 *   xvalue_t a = G_VALUE_INIT;
 *   xvalue_t b = G_VALUE_INIT;
 *   const xchar_t *message;
 *
 *   // The xvalue_t starts empty
 *   g_assert (!G_VALUE_HOLDS_STRING (&a));
 *
 *   // Put a string in it
 *   xvalue_init (&a, XTYPE_STRING);
 *   g_assert (G_VALUE_HOLDS_STRING (&a));
 *   xvalue_set_static_string (&a, "Hello, world!");
 *   g_printf ("%s\n", xvalue_get_string (&a));
 *
 *   // Reset it to its pristine state
 *   xvalue_unset (&a);
 *
 *   // It can then be reused for another type
 *   xvalue_init (&a, XTYPE_INT);
 *   xvalue_set_int (&a, 42);
 *
 *   // Attempt to transform it into a xvalue_t of type STRING
 *   xvalue_init (&b, XTYPE_STRING);
 *
 *   // An INT is transformable to a STRING
 *   g_assert (xvalue_type_transformable (XTYPE_INT, XTYPE_STRING));
 *
 *   xvalue_transform (&a, &b);
 *   g_printf ("%s\n", xvalue_get_string (&b));
 *
 *   // Attempt to transform it again using a custom transform function
 *   xvalue_register_transform_func (XTYPE_INT, XTYPE_STRING, int2string);
 *   xvalue_transform (&a, &b);
 *   g_printf ("%s\n", xvalue_get_string (&b));
 *   return 0;
 * }
 * ]|
 *
 * See also [gobject-Standard-Parameter-and-Value-Types] for more information on
 * validation of #xvalue_t.
 *
 * For letting a #xvalue_t own (and memory manage) arbitrary types or pointers,
 * they need to become a [boxed type][gboxed]. The example below shows how
 * the pointer `mystruct` of type `MyStruct` is used as a [boxed type][gboxed].
 *
 * |[<!-- language="C" -->
 * typedef struct { ... } MyStruct;
 * G_DEFINE_BOXED_TYPE (MyStruct, my_struct, my_struct_copy, my_struct_free)
 *
 * // These two lines normally go in a public header. By xobject_t convention,
 * // the naming scheme is NAMESPACE_TYPE_NAME:
 * #define MY_TYPE_STRUCT (my_struct_get_type ())
 * xtype_t my_struct_get_type (void);
 *
 * void
 * foo ()
 * {
 *   xvalue_t *value = g_new0 (xvalue_t, 1);
 *   xvalue_init (value, MY_TYPE_STRUCT);
 *   xvalue_set_boxed (value, mystruct);
 *   // [... your code ....]
 *   xvalue_unset (value);
 *   xvalue_free (value);
 * }
 * ]|
 */


/* --- typedefs & structures --- */
typedef struct {
  xtype_t src_type;
  xtype_t dest_type;
  GValueTransform func;
} TransformEntry;


/* --- prototypes --- */
static xint_t	transform_entries_cmp	(xconstpointer bsearch_node1,
					 xconstpointer bsearch_node2);


/* --- variables --- */
static GBSearchArray *transform_array = NULL;
static GBSearchConfig transform_bconfig = {
  sizeof (TransformEntry),
  transform_entries_cmp,
  G_BSEARCH_ARRAY_ALIGN_POWER2,
};


/* --- functions --- */
void
_xvalue_c_init (void)
{
  transform_array = g_bsearch_array_create (&transform_bconfig);
}

static inline void		/* keep this function in sync with gvaluecollector.h and gboxed.c */
value_meminit (xvalue_t *value,
	       xtype_t   value_type)
{
  value->g_type = value_type;
  memset (value->data, 0, sizeof (value->data));
}

/**
 * xvalue_init:
 * @value: A zero-filled (uninitialized) #xvalue_t structure.
 * @g_type: Type the #xvalue_t should hold values of.
 *
 * Initializes @value with the default value of @type.
 *
 * Returns: (transfer none): the #xvalue_t structure that has been passed in
 */
xvalue_t*
xvalue_init (xvalue_t *value,
	      xtype_t   g_type)
{
  xtype_value_table_t *value_table;
  /* g_return_val_if_fail (XTYPE_IS_VALUE (g_type), NULL);	be more elaborate below */
  g_return_val_if_fail (value != NULL, NULL);
  /* g_return_val_if_fail (G_VALUE_TYPE (value) == 0, NULL);	be more elaborate below */

  value_table = xtype_value_table_peek (g_type);

  if (value_table && G_VALUE_TYPE (value) == 0)
    {
      /* setup and init */
      value_meminit (value, g_type);
      value_table->value_init (value);
    }
  else if (G_VALUE_TYPE (value))
    g_warning ("%s: cannot initialize xvalue_t with type '%s', the value has already been initialized as '%s'",
	       G_STRLOC,
	       xtype_name (g_type),
	       xtype_name (G_VALUE_TYPE (value)));
  else /* !XTYPE_IS_VALUE (g_type) */
    g_warning ("%s: cannot initialize xvalue_t with type '%s', %s",
               G_STRLOC,
               xtype_name (g_type),
               value_table ? "this type is abstract with regards to xvalue_t use, use a more specific (derived) type" : "this type has no xtype_value_table_t implementation");
  return value;
}

/**
 * xvalue_copy:
 * @src_value: An initialized #xvalue_t structure.
 * @dest_value: An initialized #xvalue_t structure of the same type as @src_value.
 *
 * Copies the value of @src_value into @dest_value.
 */
void
xvalue_copy (const xvalue_t *src_value,
	      xvalue_t       *dest_value)
{
  g_return_if_fail (src_value);
  g_return_if_fail (dest_value);
  g_return_if_fail (xvalue_type_compatible (G_VALUE_TYPE (src_value), G_VALUE_TYPE (dest_value)));

  if (src_value != dest_value)
    {
      xtype_t dest_type = G_VALUE_TYPE (dest_value);
      xtype_value_table_t *value_table = xtype_value_table_peek (dest_type);

      g_return_if_fail (value_table);

      /* make sure dest_value's value is free()d */
      if (value_table->value_free)
	value_table->value_free (dest_value);

      /* setup and copy */
      value_meminit (dest_value, dest_type);
      value_table->value_copy (src_value, dest_value);
    }
}

/**
 * xvalue_reset:
 * @value: An initialized #xvalue_t structure.
 *
 * Clears the current value in @value and resets it to the default value
 * (as if the value had just been initialized).
 *
 * Returns: the #xvalue_t structure that has been passed in
 */
xvalue_t*
xvalue_reset (xvalue_t *value)
{
  xtype_value_table_t *value_table;
  xtype_t g_type;

  g_return_val_if_fail (value, NULL);
  g_type = G_VALUE_TYPE (value);

  value_table = xtype_value_table_peek (g_type);
  g_return_val_if_fail (value_table, NULL);

  /* make sure value's value is free()d */
  if (value_table->value_free)
    value_table->value_free (value);

  /* setup and init */
  value_meminit (value, g_type);
  value_table->value_init (value);

  return value;
}

/**
 * xvalue_unset:
 * @value: An initialized #xvalue_t structure.
 *
 * Clears the current value in @value (if any) and "unsets" the type,
 * this releases all resources associated with this xvalue_t. An unset
 * value is the same as an uninitialized (zero-filled) #xvalue_t
 * structure.
 */
void
xvalue_unset (xvalue_t *value)
{
  xtype_value_table_t *value_table;

  if (value->g_type == 0)
    return;

  g_return_if_fail (value);

  value_table = xtype_value_table_peek (G_VALUE_TYPE (value));
  g_return_if_fail (value_table);

  if (value_table->value_free)
    value_table->value_free (value);
  memset (value, 0, sizeof (*value));
}

/**
 * xvalue_fits_pointer:
 * @value: An initialized #xvalue_t structure.
 *
 * Determines if @value will fit inside the size of a pointer value.
 * This is an internal function introduced mainly for C marshallers.
 *
 * Returns: %TRUE if @value will fit inside a pointer value.
 */
xboolean_t
xvalue_fits_pointer (const xvalue_t *value)
{
  xtype_value_table_t *value_table;

  g_return_val_if_fail (value, FALSE);

  value_table = xtype_value_table_peek (G_VALUE_TYPE (value));
  g_return_val_if_fail (value_table, FALSE);

  return value_table->value_peek_pointer != NULL;
}

/**
 * xvalue_peek_pointer:
 * @value: An initialized #xvalue_t structure
 *
 * Returns the value contents as pointer. This function asserts that
 * xvalue_fits_pointer() returned %TRUE for the passed in value.
 * This is an internal function introduced mainly for C marshallers.
 *
 * Returns: (transfer none): the value contents as pointer
 */
xpointer_t
xvalue_peek_pointer (const xvalue_t *value)
{
  xtype_value_table_t *value_table;

  g_return_val_if_fail (value, NULL);

  value_table = xtype_value_table_peek (G_VALUE_TYPE (value));
  g_return_val_if_fail (value_table, NULL);

  if (!value_table->value_peek_pointer)
    {
      g_return_val_if_fail (xvalue_fits_pointer (value) == TRUE, NULL);
      return NULL;
    }

  return value_table->value_peek_pointer (value);
}

/**
 * xvalue_set_instance:
 * @value: An initialized #xvalue_t structure.
 * @instance: (nullable): the instance
 *
 * Sets @value from an instantiatable type via the
 * value_table's collect_value() function.
 */
void
xvalue_set_instance (xvalue_t  *value,
		      xpointer_t instance)
{
  xtype_t g_type;
  xtype_value_table_t *value_table;
  xtype_c_value_t cvalue;
  xchar_t *error_msg;

  g_return_if_fail (value);
  g_type = G_VALUE_TYPE (value);
  value_table = xtype_value_table_peek (g_type);
  g_return_if_fail (value_table);

  if (instance)
    {
      g_return_if_fail (XTYPE_CHECK_INSTANCE (instance));
      g_return_if_fail (xvalue_type_compatible (XTYPE_FROM_INSTANCE (instance), G_VALUE_TYPE (value)));
    }

  g_return_if_fail (strcmp (value_table->collect_format, "p") == 0);

  memset (&cvalue, 0, sizeof (cvalue));
  cvalue.v_pointer = instance;

  /* make sure value's value is free()d */
  if (value_table->value_free)
    value_table->value_free (value);

  /* setup and collect */
  value_meminit (value, g_type);
  error_msg = value_table->collect_value (value, 1, &cvalue, 0);
  if (error_msg)
    {
      g_warning ("%s: %s", G_STRLOC, error_msg);
      g_free (error_msg);

      /* we purposely leak the value here, it might not be
       * in a correct state if an error condition occurred
       */
      value_meminit (value, g_type);
      value_table->value_init (value);
    }
}

/**
 * xvalue_init_from_instance:
 * @value: An uninitialized #xvalue_t structure.
 * @instance: (type xobject_t.TypeInstance): the instance
 *
 * Initializes and sets @value from an instantiatable type via the
 * value_table's collect_value() function.
 *
 * Note: The @value will be initialised with the exact type of
 * @instance.  If you wish to set the @value's type to a different xtype_t
 * (such as a parent class xtype_t), you need to manually call
 * xvalue_init() and xvalue_set_instance().
 *
 * Since: 2.42
 */
void
xvalue_init_from_instance (xvalue_t  *value,
                            xpointer_t instance)
{
  g_return_if_fail (value != NULL && G_VALUE_TYPE(value) == 0);

  if (X_IS_OBJECT (instance))
    {
      /* Fast-path.
       * If X_IS_OBJECT() succeeds we know:
       * * that instance is present and valid
       * * that it is a xobject_t, and therefore we can directly
       *   use the collect implementation (xobject_ref) */
      value_meminit (value, XTYPE_FROM_INSTANCE (instance));
      value->data[0].v_pointer = xobject_ref (instance);
    }
  else
    {
      xtype_t g_type;
      xtype_value_table_t *value_table;
      xtype_c_value_t cvalue;
      xchar_t *error_msg;

      g_return_if_fail (XTYPE_CHECK_INSTANCE (instance));

      g_type = XTYPE_FROM_INSTANCE (instance);
      value_table = xtype_value_table_peek (g_type);
      g_return_if_fail (strcmp (value_table->collect_format, "p") == 0);

      memset (&cvalue, 0, sizeof (cvalue));
      cvalue.v_pointer = instance;

      /* setup and collect */
      value_meminit (value, g_type);
      value_table->value_init (value);
      error_msg = value_table->collect_value (value, 1, &cvalue, 0);
      if (error_msg)
        {
          g_warning ("%s: %s", G_STRLOC, error_msg);
          g_free (error_msg);

          /* we purposely leak the value here, it might not be
           * in a correct state if an error condition occurred
           */
          value_meminit (value, g_type);
          value_table->value_init (value);
        }
    }
}

static xtype_t
transform_lookup_get_parent_type (xtype_t type)
{
  if (xtype_fundamental (type) == XTYPE_INTERFACE)
    return xtype_interface_instantiatable_prerequisite (type);

  return xtype_parent (type);
}

static GValueTransform
transform_func_lookup (xtype_t src_type,
		       xtype_t dest_type)
{
  TransformEntry entry;

  entry.src_type = src_type;
  do
    {
      entry.dest_type = dest_type;
      do
	{
	  TransformEntry *e;

	  e = g_bsearch_array_lookup (transform_array, &transform_bconfig, &entry);
	  if (e)
	    {
	      /* need to check that there hasn't been a change in value handling */
	      if (xtype_value_table_peek (entry.dest_type) == xtype_value_table_peek (dest_type) &&
		  xtype_value_table_peek (entry.src_type) == xtype_value_table_peek (src_type))
		return e->func;
	    }
	  entry.dest_type = transform_lookup_get_parent_type (entry.dest_type);
	}
      while (entry.dest_type);

      entry.src_type = transform_lookup_get_parent_type (entry.src_type);
    }
  while (entry.src_type);

  return NULL;
}

static xint_t
transform_entries_cmp (xconstpointer bsearch_node1,
		       xconstpointer bsearch_node2)
{
  const TransformEntry *e1 = bsearch_node1;
  const TransformEntry *e2 = bsearch_node2;
  xint_t cmp = G_BSEARCH_ARRAY_CMP (e1->src_type, e2->src_type);

  if (cmp)
    return cmp;
  else
    return G_BSEARCH_ARRAY_CMP (e1->dest_type, e2->dest_type);
}

/**
 * xvalue_register_transform_func: (skip)
 * @src_type: Source type.
 * @dest_type: Target type.
 * @transform_func: a function which transforms values of type @src_type
 *  into value of type @dest_type
 *
 * Registers a value transformation function for use in xvalue_transform().
 * A previously registered transformation function for @src_type and @dest_type
 * will be replaced.
 */
void
xvalue_register_transform_func (xtype_t           src_type,
				 xtype_t           dest_type,
				 GValueTransform transform_func)
{
  TransformEntry entry;

  /* these checks won't pass for dynamic types.
   * g_return_if_fail (XTYPE_HAS_VALUE_TABLE (src_type));
   * g_return_if_fail (XTYPE_HAS_VALUE_TABLE (dest_type));
   */
  g_return_if_fail (transform_func != NULL);

  entry.src_type = src_type;
  entry.dest_type = dest_type;

#if 0 /* let transform function replacement be a valid operation */
  if (g_bsearch_array_lookup (transform_array, &transform_bconfig, &entry))
    g_warning ("reregistering value transformation function (%p) for '%s' to '%s'",
	       transform_func,
	       xtype_name (src_type),
	       xtype_name (dest_type));
#endif

  entry.func = transform_func;
  transform_array = g_bsearch_array_replace (transform_array, &transform_bconfig, &entry);
}

/**
 * xvalue_type_transformable:
 * @src_type: Source type.
 * @dest_type: Target type.
 *
 * Check whether xvalue_transform() is able to transform values
 * of type @src_type into values of type @dest_type. Note that for
 * the types to be transformable, they must be compatible or a
 * transformation function must be registered.
 *
 * Returns: %TRUE if the transformation is possible, %FALSE otherwise.
 */
xboolean_t
xvalue_type_transformable (xtype_t src_type,
			    xtype_t dest_type)
{
  g_return_val_if_fail (src_type, FALSE);
  g_return_val_if_fail (dest_type, FALSE);

  return (xvalue_type_compatible (src_type, dest_type) ||
	  transform_func_lookup (src_type, dest_type) != NULL);
}

/**
 * xvalue_type_compatible:
 * @src_type: source type to be copied.
 * @dest_type: destination type for copying.
 *
 * Returns whether a #xvalue_t of type @src_type can be copied into
 * a #xvalue_t of type @dest_type.
 *
 * Returns: %TRUE if xvalue_copy() is possible with @src_type and @dest_type.
 */
xboolean_t
xvalue_type_compatible (xtype_t src_type,
			 xtype_t dest_type)
{
  g_return_val_if_fail (src_type, FALSE);
  g_return_val_if_fail (dest_type, FALSE);

  /* Fast path */
  if (src_type == dest_type)
    return TRUE;

  return (xtype_is_a (src_type, dest_type) &&
	  xtype_value_table_peek (dest_type) == xtype_value_table_peek (src_type));
}

/**
 * xvalue_transform:
 * @src_value: Source value.
 * @dest_value: Target value.
 *
 * Tries to cast the contents of @src_value into a type appropriate
 * to store in @dest_value, e.g. to transform a %XTYPE_INT value
 * into a %XTYPE_FLOAT value. Performing transformations between
 * value types might incur precision lossage. Especially
 * transformations into strings might reveal seemingly arbitrary
 * results and shouldn't be relied upon for production code (such
 * as rcfile value or object property serialization).
 *
 * Returns: Whether a transformation rule was found and could be applied.
 *  Upon failing transformations, @dest_value is left untouched.
 */
xboolean_t
xvalue_transform (const xvalue_t *src_value,
		   xvalue_t       *dest_value)
{
  xtype_t dest_type;

  g_return_val_if_fail (src_value, FALSE);
  g_return_val_if_fail (dest_value, FALSE);

  dest_type = G_VALUE_TYPE (dest_value);
  if (xvalue_type_compatible (G_VALUE_TYPE (src_value), dest_type))
    {
      xvalue_copy (src_value, dest_value);

      return TRUE;
    }
  else
    {
      GValueTransform transform = transform_func_lookup (G_VALUE_TYPE (src_value), dest_type);

      if (transform)
	{
	  xvalue_unset (dest_value);

	  /* setup and transform */
	  value_meminit (dest_value, dest_type);
	  transform (src_value, dest_value);

	  return TRUE;
	}
    }
  return FALSE;
}
