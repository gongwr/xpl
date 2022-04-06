/*
 * Copyright © 2008 Ryan Lortie
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

#include "config.h"

#include "gvarianttypeinfo.h"

#include <glib/gtestutils.h>
#include <glib/gthread.h>
#include <glib/gslice.h>
#include <glib/ghash.h>
#include <glib/grefcount.h>

/* < private >
 * GVariantTypeInfo:
 *
 * This structure contains the necessary information to facilitate the
 * serialization and fast deserialization of a given type of xvariant_t
 * value.  A xvariant_t instance holds a pointer to one of these
 * structures to provide for efficient operation.
 *
 * The GVariantTypeInfo structures for all of the base types, plus the
 * "variant" type are stored in a read-only static array.
 *
 * For container types, a hash table and reference counting is used to
 * ensure that only one of these structures exists for any given type.
 * In general, a container GVariantTypeInfo will exist for a given type
 * only if one or more xvariant_t instances of that type exist or if
 * another GVariantTypeInfo has that type as a subtype.  For example, if
 * a process contains a single xvariant_t instance with type "(asv)", then
 * container GVariantTypeInfo structures will exist for "(asv)" and
 * for "as" (note that "s" and "v" always exist in the static array).
 *
 * The trickiest part of GVariantTypeInfo (and in fact, the major reason
 * for its existence) is the storage of somewhat magical constants that
 * allow for O(1) lookups of items in tuples.  This is described below.
 *
 * 'container_class' is set to 'a' or 'r' if the GVariantTypeInfo is
 * contained inside of an ArrayInfo or TupleInfo, respectively.  This
 * allows the storage of the necessary additional information.
 *
 * 'fixed_size' is set to the fixed size of the type, if applicable, or
 * 0 otherwise (since no type has a fixed size of 0).
 *
 * 'alignment' is set to one less than the alignment requirement for
 * this type.  This makes many operations much more convenient.
 */
struct _GVariantTypeInfo
{
  xsize_t fixed_size;
  xuchar_t alignment;
  xuchar_t container_class;
};

/* Container types are reference counted.  They also need to have their
 * type string stored explicitly since it is not merely a single letter.
 */
typedef struct
{
  GVariantTypeInfo info;

  xchar_t *type_string;
  gatomicrefcount ref_count;
} ContainerInfo;

/* For 'array' and 'maybe' types, we store some extra information on the
 * end of the GVariantTypeInfo struct -- the element type (ie: "s" for
 * "as").  The container GVariantTypeInfo structure holds a reference to
 * the element typeinfo.
 */
typedef struct
{
  ContainerInfo container;

  GVariantTypeInfo *element;
} ArrayInfo;

/* For 'tuple' and 'dict entry' types, we store extra information for
 * each member -- its type and how to find it inside the serialized data
 * in O(1) time using 4 variables -- 'i', 'a', 'b', and 'c'.  See the
 * comment on GVariantMemberInfo in gvarianttypeinfo.h.
 */
typedef struct
{
  ContainerInfo container;

  GVariantMemberInfo *members;
  xsize_t n_members;
} TupleInfo;


/* Hard-code the base types in a constant array */
static const GVariantTypeInfo xvariant_type_info_basic_table[24] = {
#define fixed_aligned(x)  x, x - 1, 0
#define not_a_type        0,     0, 0
#define unaligned         0,     0, 0
#define aligned(x)        0, x - 1, 0
  /* 'b' */ { fixed_aligned(1) },   /* boolean */
  /* 'c' */ { not_a_type },
  /* 'd' */ { fixed_aligned(8) },   /* double */
  /* 'e' */ { not_a_type },
  /* 'f' */ { not_a_type },
  /* 'g' */ { unaligned        },   /* signature string */
  /* 'h' */ { fixed_aligned(4) },   /* file handle (int32) */
  /* 'i' */ { fixed_aligned(4) },   /* int32 */
  /* 'j' */ { not_a_type },
  /* 'k' */ { not_a_type },
  /* 'l' */ { not_a_type },
  /* 'm' */ { not_a_type },
  /* 'n' */ { fixed_aligned(2) },   /* int16 */
  /* 'o' */ { unaligned        },   /* object path string */
  /* 'p' */ { not_a_type },
  /* 'q' */ { fixed_aligned(2) },   /* uint16 */
  /* 'r' */ { not_a_type },
  /* 's' */ { unaligned        },   /* string */
  /* 't' */ { fixed_aligned(8) },   /* uint64 */
  /* 'u' */ { fixed_aligned(4) },   /* uint32 */
  /* 'v' */ { aligned(8)       },   /* variant */
  /* 'w' */ { not_a_type },
  /* 'x' */ { fixed_aligned(8) },   /* int64 */
  /* 'y' */ { fixed_aligned(1) },   /* byte */
#undef fixed_aligned
#undef not_a_type
#undef unaligned
#undef aligned
};

/* We need to have type strings to return for the base types.  We store
 * those in another array.  Since all base type strings are single
 * characters this is easy.  By not storing pointers to strings into the
 * GVariantTypeInfo itself, we save a bunch of relocations.
 */
static const char xvariant_type_info_basic_chars[24][2] = {
  "b", " ", "d", " ", " ", "g", "h", "i", " ", " ", " ", " ",
  "n", "o", " ", "q", " ", "s", "t", "u", "v", " ", "x", "y"
};

/* sanity checks to make debugging easier */
static void
xvariant_type_info_check (const GVariantTypeInfo *info,
                           char                    container_class)
{
#ifndef G_DISABLE_ASSERT
  g_assert (!container_class || info->container_class == container_class);

  /* alignment can only be one of these */
  g_assert (info->alignment == 0 || info->alignment == 1 ||
            info->alignment == 3 || info->alignment == 7);

  if (info->container_class)
    {
      ContainerInfo *container = (ContainerInfo *) info;

      /* extra checks for containers */
      g_assert (!g_atomic_ref_count_compare (&container->ref_count, 0));
      g_assert (container->type_string != NULL);
    }
  else
    {
      xint_t index;

      /* if not a container, then ensure that it is a valid member of
       * the basic types table
       */
      index = info - xvariant_type_info_basic_table;

      g_assert (G_N_ELEMENTS (xvariant_type_info_basic_table) == 24);
      g_assert (G_N_ELEMENTS (xvariant_type_info_basic_chars) == 24);
      g_assert (0 <= index && index < 24);
      g_assert (xvariant_type_info_basic_chars[index][0] != ' ');
    }
#endif  /* !G_DISABLE_ASSERT */
}

/* < private >
 * xvariant_type_info_get_type_string:
 * @info: a #GVariantTypeInfo
 *
 * Gets the type string for @info.  The string is nul-terminated.
 */
const xchar_t *
xvariant_type_info_get_type_string (GVariantTypeInfo *info)
{
  xvariant_type_info_check (info, 0);

  if (info->container_class)
    {
      ContainerInfo *container = (ContainerInfo *) info;

      /* containers have their type string stored inside them */
      return container->type_string;
    }
  else
    {
      xint_t index;

      /* look up the type string in the base type array.  the call to
       * xvariant_type_info_check() above already ensured validity.
       */
      index = info - xvariant_type_info_basic_table;

      return xvariant_type_info_basic_chars[index];
    }
}

/* < private >
 * xvariant_type_info_query:
 * @info: a #GVariantTypeInfo
 * @alignment: (out) (optional): the location to store the alignment, or %NULL
 * @fixed_size: (out) (optional): the location to store the fixed size, or %NULL
 *
 * Queries @info to determine the alignment requirements and fixed size
 * (if any) of the type.
 *
 * @fixed_size, if non-%NULL is set to the fixed size of the type, or 0
 * to indicate that the type is a variable-sized type.  No type has a
 * fixed size of 0.
 *
 * @alignment, if non-%NULL, is set to one less than the required
 * alignment of the type.  For example, for a 32bit integer, @alignment
 * would be set to 3.  This allows you to round an integer up to the
 * proper alignment by performing the following efficient calculation:
 *
 *   offset += ((-offset) & alignment);
 */
void
xvariant_type_info_query (GVariantTypeInfo *info,
                           xuint_t            *alignment,
                           xsize_t            *fixed_size)
{
  xvariant_type_info_check (info, 0);

  if (alignment)
    *alignment = info->alignment;

  if (fixed_size)
    *fixed_size = info->fixed_size;
}

/* < private >
 * xvariant_type_info_query_depth:
 * @info: a #GVariantTypeInfo
 *
 * Queries @info to determine the depth of the type.
 *
 * See xvariant_type_string_get_depth_() for more details.
 *
 * Returns: depth of @info
 * Since: 2.60
 */
xsize_t
xvariant_type_info_query_depth (GVariantTypeInfo *info)
{
  xvariant_type_info_check (info, 0);

  if (info->container_class)
    {
      ContainerInfo *container = (ContainerInfo *) info;
      return xvariant_type_string_get_depth_ (container->type_string);
    }

  return 1;
}

/* == array == */
#define GV_ARRAY_INFO_CLASS 'a'
static ArrayInfo *
GV_ARRAY_INFO (GVariantTypeInfo *info)
{
  xvariant_type_info_check (info, GV_ARRAY_INFO_CLASS);

  return (ArrayInfo *) info;
}

static void
array_info_free (GVariantTypeInfo *info)
{
  ArrayInfo *array_info;

  g_assert (info->container_class == GV_ARRAY_INFO_CLASS);
  array_info = (ArrayInfo *) info;

  xvariant_type_info_unref (array_info->element);
  g_slice_free (ArrayInfo, array_info);
}

static ContainerInfo *
array_info_new (const xvariant_type_t *type)
{
  ArrayInfo *info;

  info = g_slice_new (ArrayInfo);
  info->container.info.container_class = GV_ARRAY_INFO_CLASS;

  info->element = xvariant_type_info_get (xvariant_type_element (type));
  info->container.info.alignment = info->element->alignment;
  info->container.info.fixed_size = 0;

  return (ContainerInfo *) info;
}

/* < private >
 * xvariant_type_info_element:
 * @info: a #GVariantTypeInfo for an array or maybe type
 *
 * Returns the element type for the array or maybe type.  A reference is
 * not added, so the caller must add their own.
 */
GVariantTypeInfo *
xvariant_type_info_element (GVariantTypeInfo *info)
{
  return GV_ARRAY_INFO (info)->element;
}

/* < private >
 * xvariant_type_query_element:
 * @info: a #GVariantTypeInfo for an array or maybe type
 * @alignment: (out) (optional): the location to store the alignment, or %NULL
 * @fixed_size: (out) (optional): the location to store the fixed size, or %NULL
 *
 * Returns the alignment requires and fixed size (if any) for the
 * element type of the array.  This call is a convenience wrapper around
 * xvariant_type_info_element() and xvariant_type_info_query().
 */
void
xvariant_type_info_query_element (GVariantTypeInfo *info,
                                   xuint_t            *alignment,
                                   xsize_t            *fixed_size)
{
  xvariant_type_info_query (GV_ARRAY_INFO (info)->element,
                             alignment, fixed_size);
}

/* == tuple == */
#define GV_TUPLE_INFO_CLASS 'r'
static TupleInfo *
GV_TUPLE_INFO (GVariantTypeInfo *info)
{
  xvariant_type_info_check (info, GV_TUPLE_INFO_CLASS);

  return (TupleInfo *) info;
}

static void
tuple_info_free (GVariantTypeInfo *info)
{
  TupleInfo *tuple_info;
  xsize_t i;

  g_assert (info->container_class == GV_TUPLE_INFO_CLASS);
  tuple_info = (TupleInfo *) info;

  for (i = 0; i < tuple_info->n_members; i++)
    xvariant_type_info_unref (tuple_info->members[i].type_info);

  g_slice_free1 (sizeof (GVariantMemberInfo) * tuple_info->n_members,
                 tuple_info->members);
  g_slice_free (TupleInfo, tuple_info);
}

static void
tuple_allocate_members (const xvariant_type_t  *type,
                        GVariantMemberInfo **members,
                        xsize_t               *n_members)
{
  const xvariant_type_t *item_type;
  xsize_t i = 0;

  *n_members = xvariant_type_n_items (type);
  *members = g_slice_alloc (sizeof (GVariantMemberInfo) * *n_members);

  item_type = xvariant_type_first (type);
  while (item_type)
    {
      GVariantMemberInfo *member = &(*members)[i++];

      member->type_info = xvariant_type_info_get (item_type);
      item_type = xvariant_type_next (item_type);

      if (member->type_info->fixed_size)
        member->ending_type = G_VARIANT_MEMBER_ENDING_FIXED;
      else if (item_type == NULL)
        member->ending_type = G_VARIANT_MEMBER_ENDING_LAST;
      else
        member->ending_type = G_VARIANT_MEMBER_ENDING_OFFSET;
    }

  g_assert (i == *n_members);
}

/* this is xvariant_type_info_query for a given member of the tuple.
 * before the access is done, it is ensured that the item is within
 * range and %FALSE is returned if not.
 */
static xboolean_t
tuple_get_item (TupleInfo          *info,
                GVariantMemberInfo *item,
                xsize_t              *d,
                xsize_t              *e)
{
  if (&info->members[info->n_members] == item)
    return FALSE;

  *d = item->type_info->alignment;
  *e = item->type_info->fixed_size;
  return TRUE;
}

/* Read the documentation for #GVariantMemberInfo in gvarianttype.h
 * before attempting to understand this.
 *
 * This function adds one set of "magic constant" values (for one item
 * in the tuple) to the table.
 *
 * The algorithm in tuple_generate_table() calculates values of 'a', 'b'
 * and 'c' for each item, such that the procedure for finding the item
 * is to start at the end of the previous variable-sized item, add 'a',
 * then round up to the nearest multiple of 'b', then then add 'c'.
 * Note that 'b' is stored in the usual "one less than" form.  ie:
 *
 *   start = ROUND_UP(prev_end + a, (b + 1)) + c;
 *
 * We tweak these values a little to allow for a slightly easier
 * computation and more compact storage.
 */
static void
tuple_table_append (GVariantMemberInfo **items,
                    xsize_t                i,
                    xsize_t                a,
                    xsize_t                b,
                    xsize_t                c)
{
  GVariantMemberInfo *item = (*items)++;

  /* We can shift multiples of the alignment size from 'c' into 'a'.
   * As long as we're shifting whole multiples, it won't affect the
   * result.  This means that we can take the "aligned" portion off of
   * 'c' and add it into 'a'.
   *
   *  Imagine (for sake of clarity) that ROUND_10 rounds up to the
   *  nearest 10.  It is clear that:
   *
   *   ROUND_10(a) + c == ROUND_10(a + 10*(c / 10)) + (c % 10)
   *
   * ie: remove the 10s portion of 'c' and add it onto 'a'.
   *
   * To put some numbers on it, imagine we start with a = 34 and c = 27:
   *
   *  ROUND_10(34) + 27 = 40 + 27 = 67
   *
   * but also, we can split 27 up into 20 and 7 and do this:
   *
   *  ROUND_10(34 + 20) + 7 = ROUND_10(54) + 7 = 60 + 7 = 67
   *                ^^    ^
   * without affecting the result.  We do that here.
   *
   * This reduction in the size of 'c' means that we can store it in a
   * xchar_t instead of a xsize_t.  Due to how the structure is packed, this
   * ends up saving us 'two pointer sizes' per item in each tuple when
   * allocating using GSlice.
   */
  a += ~b & c;    /* take the "aligned" part of 'c' and add to 'a' */
  c &= b;         /* chop 'c' to contain only the unaligned part */


  /* Finally, we made one last adjustment.  Recall:
   *
   *   start = ROUND_UP(prev_end + a, (b + 1)) + c;
   *
   * Forgetting the '+ c' for the moment:
   *
   *   ROUND_UP(prev_end + a, (b + 1));
   *
   * we can do a "round up" operation by adding 1 less than the amount
   * to round up to, then rounding down.  ie:
   *
   *   #define ROUND_UP(x, y)    ROUND_DOWN(x + (y-1), y)
   *
   * Of course, for rounding down to a power of two, we can just mask
   * out the appropriate number of low order bits:
   *
   *   #define ROUND_DOWN(x, y)  (x & ~(y - 1))
   *
   * Which gives us
   *
   *   #define ROUND_UP(x, y)    (x + (y - 1) & ~(y - 1))
   *
   * but recall that our alignment value 'b' is already "one less".
   * This means that to round 'prev_end + a' up to 'b' we can just do:
   *
   *   ((prev_end + a) + b) & ~b
   *
   * Associativity, and putting the 'c' back on:
   *
   *   (prev_end + (a + b)) & ~b + c
   *
   * Now, since (a + b) is constant, we can just add 'b' to 'a' now and
   * store that as the number to add to prev_end.  Then we use ~b as the
   * number to take a bitwise 'and' with.  Finally, 'c' is added on.
   *
   * Note, however, that all the low order bits of the 'aligned' value
   * are masked out and that all of the high order bits of 'c' have been
   * "moved" to 'a' (in the previous step).  This means that there are
   * no overlapping bits in the addition -- so we can do a bitwise 'or'
   * equivalently.
   *
   * This means that we can now compute the start address of a given
   * item in the tuple using the algorithm given in the documentation
   * for #GVariantMemberInfo:
   *
   *   item_start = ((prev_end + a) & b) | c;
   */

  item->i = i;
  item->a = a + b;
  item->b = ~b;
  item->c = c;
}

static xsize_t
tuple_align (xsize_t offset,
             xuint_t alignment)
{
  return offset + ((-offset) & alignment);
}

/* This function is the heart of the algorithm for calculating 'i', 'a',
 * 'b' and 'c' for each item in the tuple.
 *
 * Imagine we want to find the start of the "i" in the type "(su(qx)ni)".
 * That's a string followed by a uint32, then a tuple containing a
 * uint16 and an int64, then an int16, then our "i".  In order to get to
 * our "i" we:
 *
 * Start at the end of the string, align to 4 (for the uint32), add 4.
 * Align to 8, add 16 (for the tuple).  Align to 2, add 2 (for the
 * int16).  Then we're there.  It turns out that, given 3 simple rules,
 * we can flatten this iteration into one addition, one alignment, then
 * one more addition.
 *
 * The loop below plays through each item in the tuple, querying its
 * alignment and fixed_size into 'd' and 'e', respectively.  At all
 * times the variables 'a', 'b', and 'c' are maintained such that in
 * order to get to the current point, you add 'a', align to 'b' then add
 * 'c'.  'b' is kept in "one less than" form.  For each item, the proper
 * alignment is applied to find the values of 'a', 'b' and 'c' to get to
 * the start of that item.  Those values are recorded into the table.
 * The fixed size of the item (if applicable) is then added on.
 *
 * These 3 rules are how 'a', 'b' and 'c' are modified for alignment and
 * addition of fixed size.  They have been proven correct but are
 * presented here, without proof:
 *
 *  1) in order to "align to 'd'" where 'd' is less than or equal to the
 *     largest level of alignment seen so far ('b'), you align 'c' to
 *     'd'.
 *  2) in order to "align to 'd'" where 'd' is greater than the largest
 *     level of alignment seen so far, you add 'c' aligned to 'b' to the
 *     value of 'a', set 'b' to 'd' (ie: increase the 'largest alignment
 *     seen') and reset 'c' to 0.
 *  3) in order to "add 'e'", just add 'e' to 'c'.
 */
static void
tuple_generate_table (TupleInfo *info)
{
  GVariantMemberInfo *items = info->members;
  xsize_t i = -1, a = 0, b = 0, c = 0, d, e;

  /* iterate over each item in the tuple.
   *   'd' will be the alignment of the item (in one-less form)
   *   'e' will be the fixed size (or 0 for variable-size items)
   */
  while (tuple_get_item (info, items, &d, &e))
    {
      /* align to 'd' */
      if (d <= b)
        c = tuple_align (c, d);                   /* rule 1 */
      else
        a += tuple_align (c, b), b = d, c = 0;    /* rule 2 */

      /* the start of the item is at this point (ie: right after we
       * have aligned for it).  store this information in the table.
       */
      tuple_table_append (&items, i, a, b, c);

      /* "move past" the item by adding in its size. */
      if (e == 0)
        /* variable size:
         *
         * we'll have an offset stored to mark the end of this item, so
         * just bump the offset index to give us a new starting point
         * and reset all the counters.
         */
        i++, a = b = c = 0;
      else
        /* fixed size */
        c += e;                                   /* rule 3 */
    }
}

static void
tuple_set_base_info (TupleInfo *info)
{
  GVariantTypeInfo *base = &info->container.info;

  if (info->n_members > 0)
    {
      GVariantMemberInfo *m;

      /* the alignment requirement of the tuple is the alignment
       * requirement of its largest item.
       */
      base->alignment = 0;
      for (m = info->members; m < &info->members[info->n_members]; m++)
        /* can find the max of a list of "one less than" powers of two
         * by 'or'ing them
         */
        base->alignment |= m->type_info->alignment;

      m--; /* take 'm' back to the last item */

      /* the structure only has a fixed size if no variable-size
       * offsets are stored and the last item is fixed-sized too (since
       * an offset is never stored for the last item).
       */
      if (m->i == (xsize_t) -1 && m->type_info->fixed_size)
        /* in that case, the fixed size can be found by finding the
         * start of the last item (in the usual way) and adding its
         * fixed size.
         *
         * if a tuple has a fixed size then it is always a multiple of
         * the alignment requirement (to make packing into arrays
         * easier) so we round up to that here.
         */
        base->fixed_size =
          tuple_align (((m->a & m->b) | m->c) + m->type_info->fixed_size,
                       base->alignment);
      else
        /* else, the tuple is not fixed size */
        base->fixed_size = 0;
    }
  else
    {
      /* the empty tuple: '()'.
       *
       * has a size of 1 and a no alignment requirement.
       *
       * It has a size of 1 (not 0) for two practical reasons:
       *
       *  1) So we can determine how many of them are in an array
       *     without dividing by zero or without other tricks.
       *
       *  2) Even if we had some trick to know the number of items in
       *     the array (as xvariant_t did at one time) this would open a
       *     potential denial of service attack: an attacker could send
       *     you an extremely small array (in terms of number of bytes)
       *     containing trillions of zero-sized items.  If you iterated
       *     over this array you would effectively infinite-loop your
       *     program.  By forcing a size of at least one, we bound the
       *     amount of computation done in response to a message to a
       *     reasonable function of the size of that message.
       */
      base->alignment = 0;
      base->fixed_size = 1;
    }
}

static ContainerInfo *
tuple_info_new (const xvariant_type_t *type)
{
  TupleInfo *info;

  info = g_slice_new (TupleInfo);
  info->container.info.container_class = GV_TUPLE_INFO_CLASS;

  tuple_allocate_members (type, &info->members, &info->n_members);
  tuple_generate_table (info);
  tuple_set_base_info (info);

  return (ContainerInfo *) info;
}

/* < private >
 * xvariant_type_info_n_members:
 * @info: a #GVariantTypeInfo for a tuple or dictionary entry type
 *
 * Returns the number of members in a tuple or dictionary entry type.
 * For a dictionary entry this will always be 2.
 */
xsize_t
xvariant_type_info_n_members (GVariantTypeInfo *info)
{
  return GV_TUPLE_INFO (info)->n_members;
}

/* < private >
 * xvariant_type_info_member_info:
 * @info: a #GVariantTypeInfo for a tuple or dictionary entry type
 * @index: the member to fetch information for
 *
 * Returns the #GVariantMemberInfo for a given member.  See
 * documentation for that structure for why you would want this
 * information.
 *
 * @index must refer to a valid child (ie: strictly less than
 * xvariant_type_info_n_members() returns).
 */
const GVariantMemberInfo *
xvariant_type_info_member_info (GVariantTypeInfo *info,
                                 xsize_t             index)
{
  TupleInfo *tuple_info = GV_TUPLE_INFO (info);

  if (index < tuple_info->n_members)
    return &tuple_info->members[index];

  return NULL;
}

/* == new/ref/unref == */
static GRecMutex xvariant_type_info_lock;
static xhashtable_t *xvariant_type_info_table;

/* < private >
 * xvariant_type_info_get:
 * @type: a #xvariant_type_t
 *
 * Returns a reference to a #GVariantTypeInfo for @type.
 *
 * If an info structure already exists for this type, a new reference is
 * returned.  If not, the required calculations are performed and a new
 * info structure is returned.
 *
 * It is appropriate to call xvariant_type_info_unref() on the return
 * value.
 */
GVariantTypeInfo *
xvariant_type_info_get (const xvariant_type_t *type)
{
  char type_char;

  type_char = xvariant_type_peek_string (type)[0];

  if (type_char == G_VARIANT_TYPE_INFO_CHAR_MAYBE ||
      type_char == G_VARIANT_TYPE_INFO_CHAR_ARRAY ||
      type_char == G_VARIANT_TYPE_INFO_CHAR_TUPLE ||
      type_char == G_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY)
    {
      GVariantTypeInfo *info;
      xchar_t *type_string;

      type_string = xvariant_type_dup_string (type);

      g_rec_mutex_lock (&xvariant_type_info_lock);

      if (xvariant_type_info_table == NULL)
        xvariant_type_info_table = xhash_table_new (xstr_hash,
                                                      xstr_equal);
      info = xhash_table_lookup (xvariant_type_info_table, type_string);

      if (info == NULL)
        {
          ContainerInfo *container;

          if (type_char == G_VARIANT_TYPE_INFO_CHAR_MAYBE ||
              type_char == G_VARIANT_TYPE_INFO_CHAR_ARRAY)
            {
              container = array_info_new (type);
            }
          else /* tuple or dict entry */
            {
              container = tuple_info_new (type);
            }

          info = (GVariantTypeInfo *) container;
          container->type_string = type_string;
          g_atomic_ref_count_init (&container->ref_count);

          xhash_table_insert (xvariant_type_info_table, type_string, info);
          type_string = NULL;
        }
      else
        xvariant_type_info_ref (info);

      g_rec_mutex_unlock (&xvariant_type_info_lock);
      xvariant_type_info_check (info, 0);
      g_free (type_string);

      return info;
    }
  else
    {
      const GVariantTypeInfo *info;
      int index;

      index = type_char - 'b';
      g_assert (G_N_ELEMENTS (xvariant_type_info_basic_table) == 24);
      g_assert_cmpint (0, <=, index);
      g_assert_cmpint (index, <, 24);

      info = xvariant_type_info_basic_table + index;
      xvariant_type_info_check (info, 0);

      return (GVariantTypeInfo *) info;
    }
}

/* < private >
 * xvariant_type_info_ref:
 * @info: a #GVariantTypeInfo
 *
 * Adds a reference to @info.
 */
GVariantTypeInfo *
xvariant_type_info_ref (GVariantTypeInfo *info)
{
  xvariant_type_info_check (info, 0);

  if (info->container_class)
    {
      ContainerInfo *container = (ContainerInfo *) info;

      g_atomic_ref_count_inc (&container->ref_count);
    }

  return info;
}

/* < private >
 * xvariant_type_info_unref:
 * @info: a #GVariantTypeInfo
 *
 * Releases a reference held on @info.  This may result in @info being
 * freed.
 */
void
xvariant_type_info_unref (GVariantTypeInfo *info)
{
  xvariant_type_info_check (info, 0);

  if (info->container_class)
    {
      ContainerInfo *container = (ContainerInfo *) info;

      g_rec_mutex_lock (&xvariant_type_info_lock);
      if (g_atomic_ref_count_dec (&container->ref_count))
        {
          xhash_table_remove (xvariant_type_info_table,
                               container->type_string);
          if (xhash_table_size (xvariant_type_info_table) == 0)
            {
              xhash_table_unref (xvariant_type_info_table);
              xvariant_type_info_table = NULL;
            }
          g_rec_mutex_unlock (&xvariant_type_info_lock);

          g_free (container->type_string);

          if (info->container_class == GV_ARRAY_INFO_CLASS)
            array_info_free (info);

          else if (info->container_class == GV_TUPLE_INFO_CLASS)
            tuple_info_free (info);

          else
            g_assert_not_reached ();
        }
      else
        g_rec_mutex_unlock (&xvariant_type_info_lock);
    }
}

void
xvariant_type_info_assert_no_infos (void)
{
  g_assert (xvariant_type_info_table == NULL);
}
