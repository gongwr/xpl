/*
 * Copyright © 2020 Canonical Ltd.
 * Copyright © 2021 Alexandros Theodotou
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
 */

#include "config.h"

#include "gstrvbuilder.h"

#include "garray.h"
#include "gmem.h"
#include "gmessages.h"

/**
 * SECTION:gstrvbuilder
 * @title: xstrv_builder_t
 * @short_description: Helper to create NULL-terminated string arrays.
 *
 * #xstrv_builder_t is a method of easily building dynamically sized
 * NULL-terminated string arrays.
 *
 * The following example shows how to build a two element array:
 *
 * |[<!-- language="C" -->
 *   x_autoptr(xstrv_builder) builder = xstrv_builder_new ();
 *   xstrv_builder_add (builder, "hello");
 *   xstrv_builder_add (builder, "world");
 *   x_auto(xstrv) array = xstrv_builder_end (builder);
 * ]|
 *
 * Since: 2.68
 */

struct _xstrv_builder
{
  xptr_array_t array;
};

/**
 * xstrv_builder_new:
 *
 * Creates a new #xstrv_builder_t with a reference count of 1.
 * Use xstrv_builder_unref() on the returned value when no longer needed.
 *
 * Returns: (transfer full): the new #xstrv_builder_t
 *
 * Since: 2.68
 */
xstrv_builder_t *
xstrv_builder_new (void)
{
  return (xstrv_builder_t *) xptr_array_new_with_free_func (g_free);
}

/**
 * xstrv_builder_unref:
 * @builder: (transfer full): a #xstrv_builder_t allocated by xstrv_builder_new()
 *
 * Decreases the reference count on @builder.
 *
 * In the event that there are no more references, releases all memory
 * associated with the #xstrv_builder_t.
 *
 * Since: 2.68
 **/
void
xstrv_builder_unref (xstrv_builder_t *builder)
{
  xptr_array_unref (&builder->array);
}

/**
 * xstrv_builder_ref:
 * @builder: (transfer none): a #xstrv_builder_t
 *
 * Atomically increments the reference count of @builder by one.
 * This function is thread-safe and may be called from any thread.
 *
 * Returns: (transfer full): The passed in #xstrv_builder_t
 *
 * Since: 2.68
 */
xstrv_builder_t *
xstrv_builder_ref (xstrv_builder_t *builder)
{
  return (xstrv_builder_t *) xptr_array_ref (&builder->array);
}

/**
 * xstrv_builder_add:
 * @builder: a #xstrv_builder_t
 * @value: a string.
 *
 * Add a string to the end of the array.
 *
 * Since 2.68
 */
void
xstrv_builder_add (xstrv_builder_t *builder,
                    const char   *value)
{
  xptr_array_add (&builder->array, xstrdup (value));
}

/**
 * xstrv_builder_addv:
 * @builder: a #xstrv_builder_t
 * @value: (array zero-terminated=1): the vector of strings to add
 *
 * Appends all the strings in the given vector to the builder.
 *
 * Since 2.70
 */
void
xstrv_builder_addv (xstrv_builder_t *builder,
                     const char **value)
{
  xsize_t i = 0;
  g_return_if_fail (builder != NULL);
  g_return_if_fail (value != NULL);
  for (i = 0; value[i] != NULL; i++)
    xstrv_builder_add (builder, value[i]);
}

/**
 * xstrv_builder_add_many:
 * @builder: a #xstrv_builder_t
 * @...: one or more strings followed by %NULL
 *
 * Appends all the given strings to the builder.
 *
 * Since 2.70
 */
void
xstrv_builder_add_many (xstrv_builder_t *builder,
                         ...)
{
  va_list var_args;
  const xchar_t *str;
  g_return_if_fail (builder != NULL);
  va_start (var_args, builder);
  while ((str = va_arg (var_args, xchar_t *)) != NULL)
    xstrv_builder_add (builder, str);
  va_end (var_args);
}

/**
 * xstrv_builder_end:
 * @builder: a #xstrv_builder_t
 *
 * Ends the builder process and returns the constructed NULL-terminated string
 * array. The returned value should be freed with xstrfreev() when no longer
 * needed.
 *
 * Returns: (transfer full): the constructed string array.
 *
 * Since 2.68
 */
xstrv_t
xstrv_builder_end (xstrv_builder_t *builder)
{
  /* Add NULL terminator */
  xptr_array_add (&builder->array, NULL);
  return (xstrv_t) xptr_array_steal (&builder->array, NULL);
}
