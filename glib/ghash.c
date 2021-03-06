/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"

#include <string.h>  /* memset */

#include "ghash.h"
#include "gmacros.h"
#include "glib-private.h"
#include "gstrfuncs.h"
#include "gatomic.h"
#include "gtestutils.h"
#include "gslice.h"
#include "grefcount.h"
#include "gvalgrind.h"

/* The following #pragma is here so we can do this...
 *
 *   #ifndef USE_SMALL_ARRAYS
 *     is_big = TRUE;
 *   #endif
 *     return is_big ? *(((xpointer_t *) a) + index) : GUINT_TO_POINTER (*(((xuint_t *) a) + index));
 *
 * ...instead of this...
 *
 *   #ifndef USE_SMALL_ARRAYS
 *     return *(((xpointer_t *) a) + index);
 *   #else
 *     return is_big ? *(((xpointer_t *) a) + index) : GUINT_TO_POINTER (*(((xuint_t *) a) + index));
 *   #endif
 *
 * ...and still compile successfully when -Werror=duplicated-branches is passed. */

#if defined(__GNUC__) && __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#endif

/**
 * SECTION:hash_tables
 * @title: Hash Tables
 * @short_description: associations between keys and values so that
 *     given a key the value can be found quickly
 *
 * A #xhashtable_t provides associations between keys and values which is
 * optimized so that given a key, the associated value can be found,
 * inserted or removed in amortized O(1). All operations going through
 * each element take O(n) time (list all keys/values, table resize, etc.).
 *
 * Note that neither keys nor values are copied when inserted into the
 * #xhashtable_t, so they must exist for the lifetime of the #xhashtable_t.
 * This means that the use of static strings is OK, but temporary
 * strings (i.e. those created in buffers and those returned by GTK
 * widgets) should be copied with xstrdup() before being inserted.
 *
 * If keys or values are dynamically allocated, you must be careful to
 * ensure that they are freed when they are removed from the
 * #xhashtable_t, and also when they are overwritten by new insertions
 * into the #xhashtable_t. It is also not advisable to mix static strings
 * and dynamically-allocated strings in a #xhashtable_t, because it then
 * becomes difficult to determine whether the string should be freed.
 *
 * To create a #xhashtable_t, use xhash_table_new().
 *
 * To insert a key and value into a #xhashtable_t, use
 * xhash_table_insert().
 *
 * To look up a value corresponding to a given key, use
 * xhash_table_lookup() and xhash_table_lookup_extended().
 *
 * xhash_table_lookup_extended() can also be used to simply
 * check if a key is present in the hash table.
 *
 * To remove a key and value, use xhash_table_remove().
 *
 * To call a function for each key and value pair use
 * xhash_table_foreach() or use an iterator to iterate over the
 * key/value pairs in the hash table, see #xhash_table_iter_t. The iteration order
 * of a hash table is not defined, and you must not rely on iterating over
 * keys/values in the same order as they were inserted.
 *
 * To destroy a #xhashtable_t use xhash_table_destroy().
 *
 * A common use-case for hash tables is to store information about a
 * set of keys, without associating any particular value with each
 * key. xhashtable_t optimizes one way of doing so: If you store only
 * key-value pairs where key == value, then xhashtable_t does not
 * allocate memory to store the values, which can be a considerable
 * space saving, if your set is large. The functions
 * xhash_table_add() and xhash_table_contains() are designed to be
 * used when using #xhashtable_t this way.
 *
 * #xhashtable_t is not designed to be statically initialised with keys and
 * values known at compile time. To build a static hash table, use a tool such
 * as [gperf](https://www.gnu.org/software/gperf/).
 */

/**
 * xhashtable_t:
 *
 * The #xhashtable_t struct is an opaque data structure to represent a
 * [Hash Table][glib-Hash-Tables]. It should only be accessed via the
 * following functions.
 */

/**
 * GHashFunc:
 * @key: a key
 *
 * Specifies the type of the hash function which is passed to
 * xhash_table_new() when a #xhashtable_t is created.
 *
 * The function is passed a key and should return a #xuint_t hash value.
 * The functions g_direct_hash(), g_int_hash() and xstr_hash() provide
 * hash functions which can be used when the key is a #xpointer_t, #xint_t*,
 * and #xchar_t* respectively.
 *
 * g_direct_hash() is also the appropriate hash function for keys
 * of the form `GINT_TO_POINTER (n)` (or similar macros).
 *
 * A good hash functions should produce
 * hash values that are evenly distributed over a fairly large range.
 * The modulus is taken with the hash table size (a prime number) to
 * find the 'bucket' to place each key into. The function should also
 * be very fast, since it is called for each key lookup.
 *
 * Note that the hash functions provided by GLib have these qualities,
 * but are not particularly robust against manufactured keys that
 * cause hash collisions. Therefore, you should consider choosing
 * a more secure hash function when using a xhashtable_t with keys
 * that originate in untrusted data (such as HTTP requests).
 * Using xstr_hash() in that situation might make your application
 * vulnerable to
 * [Algorithmic Complexity Attacks](https://lwn.net/Articles/474912/).
 *
 * The key to choosing a good hash is unpredictability.  Even
 * cryptographic hashes are very easy to find collisions for when the
 * remainder is taken modulo a somewhat predictable prime number.  There
 * must be an element of randomness that an attacker is unable to guess.
 *
 * Returns: the hash value corresponding to the key
 */

/**
 * GHFunc:
 * @key: a key
 * @value: the value corresponding to the key
 * @user_data: user data passed to xhash_table_foreach()
 *
 * Specifies the type of the function passed to xhash_table_foreach().
 * It is called with each key/value pair, together with the @user_data
 * parameter which is passed to xhash_table_foreach().
 */

/**
 * GHRFunc:
 * @key: a key
 * @value: the value associated with the key
 * @user_data: user data passed to xhash_table_remove()
 *
 * Specifies the type of the function passed to
 * xhash_table_foreach_remove(). It is called with each key/value
 * pair, together with the @user_data parameter passed to
 * xhash_table_foreach_remove(). It should return %TRUE if the
 * key/value pair should be removed from the #xhashtable_t.
 *
 * Returns: %TRUE if the key/value pair should be removed from the
 *     #xhashtable_t
 */

/**
 * GEqualFunc:
 * @a: a value
 * @b: a value to compare with
 *
 * Specifies the type of a function used to test two values for
 * equality. The function should return %TRUE if both values are equal
 * and %FALSE otherwise.
 *
 * Returns: %TRUE if @a = @b; %FALSE otherwise
 */

/**
 * xhash_table_iter_t:
 *
 * A xhash_table_iter_t structure represents an iterator that can be used
 * to iterate over the elements of a #xhashtable_t. xhash_table_iter_t
 * structures are typically allocated on the stack and then initialized
 * with xhash_table_iter_init().
 *
 * The iteration order of a #xhash_table_iter_t over the keys/values in a hash
 * table is not defined.
 */

/**
 * xhash_table_freeze:
 * @hash_table: a #xhashtable_t
 *
 * This function is deprecated and will be removed in the next major
 * release of GLib. It does nothing.
 */

/**
 * xhash_table_thaw:
 * @hash_table: a #xhashtable_t
 *
 * This function is deprecated and will be removed in the next major
 * release of GLib. It does nothing.
 */

#define HASH_TABLE_MIN_SHIFT 3  /* 1 << 3 == 8 buckets */

#define UNUSED_HASH_VALUE 0
#define TOMBSTONE_HASH_VALUE 1
#define HASH_IS_UNUSED(h_) ((h_) == UNUSED_HASH_VALUE)
#define HASH_IS_TOMBSTONE(h_) ((h_) == TOMBSTONE_HASH_VALUE)
#define HASH_IS_REAL(h_) ((h_) >= 2)

/* If int is smaller than void * on our arch, we start out with
 * int-sized keys and values and resize to pointer-sized entries as
 * needed. This saves a good amount of memory when the HT is being
 * used with e.g. GUINT_TO_POINTER(). */

#define BIG_ENTRY_SIZE (SIZEOF_VOID_P)
#define SMALL_ENTRY_SIZE (SIZEOF_INT)

#if SMALL_ENTRY_SIZE < BIG_ENTRY_SIZE
# define USE_SMALL_ARRAYS
#endif

struct _GHashTable
{
  xsize_t            size;
  xint_t             mod;
  xuint_t            mask;
  xint_t             nnodes;
  xint_t             noccupied;  /* nnodes + tombstones */

  xuint_t            have_big_keys : 1;
  xuint_t            have_bixvalues : 1;

  xpointer_t         keys;
  xuint_t           *hashes;
  xpointer_t         values;

  GHashFunc        hash_func;
  GEqualFunc       key_equal_func;
  gatomicrefcount  ref_count;
#ifndef G_DISABLE_ASSERT
  /*
   * Tracks the structure of the hash table, not its contents: is only
   * incremented when a node is added or removed (is not incremented
   * when the key or data of a node is modified).
   */
  int              version;
#endif
  xdestroy_notify_t   key_destroy_func;
  xdestroy_notify_t   value_destroy_func;
};

typedef struct
{
  xhashtable_t  *hash_table;
  xpointer_t     dummy1;
  xpointer_t     dummy2;
  xint_t         position;
  xboolean_t     dummy3;
  xint_t         version;
} RealIter;

G_STATIC_ASSERT (sizeof (xhash_table_iter_t) == sizeof (RealIter));
G_STATIC_ASSERT (G_ALIGNOF (xhash_table_iter_t) >= G_ALIGNOF (RealIter));

/* Each table size has an associated prime modulo (the first prime
 * lower than the table size) used to find the initial bucket. Probing
 * then works modulo 2^n. The prime modulo is necessary to get a
 * good distribution with poor hash functions.
 */
static const xint_t prime_mod [] =
{
  1,          /* For 1 << 0 */
  2,
  3,
  7,
  13,
  31,
  61,
  127,
  251,
  509,
  1021,
  2039,
  4093,
  8191,
  16381,
  32749,
  65521,      /* For 1 << 16 */
  131071,
  262139,
  524287,
  1048573,
  2097143,
  4194301,
  8388593,
  16777213,
  33554393,
  67108859,
  134217689,
  268435399,
  536870909,
  1073741789,
  2147483647  /* For 1 << 31 */
};

static void
xhash_table_set_shift (xhashtable_t *hash_table, xint_t shift)
{
  hash_table->size = 1 << shift;
  hash_table->mod  = prime_mod [shift];

  /* hash_table->size is always a power of two, so we can calculate the mask
   * by simply subtracting 1 from it. The leading assertion ensures that
   * we're really dealing with a power of two. */

  xassert ((hash_table->size & (hash_table->size - 1)) == 0);
  hash_table->mask = hash_table->size - 1;
}

static xint_t
xhash_table_find_closest_shift (xint_t n)
{
  xint_t i;

  for (i = 0; n; i++)
    n >>= 1;

  return i;
}

static void
xhash_table_set_shift_from_size (xhashtable_t *hash_table, xint_t size)
{
  xint_t shift;

  shift = xhash_table_find_closest_shift (size);
  shift = MAX (shift, HASH_TABLE_MIN_SHIFT);

  xhash_table_set_shift (hash_table, shift);
}

static inline xpointer_t
xhash_table_realloc_key_or_value_array (xpointer_t a, xuint_t size, G_GNUC_UNUSED xboolean_t is_big)
{
#ifdef USE_SMALL_ARRAYS
  return g_realloc (a, size * (is_big ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
#else
  return g_renew (xpointer_t, a, size);
#endif
}

static inline xpointer_t
xhash_table_fetch_key_or_value (xpointer_t a, xuint_t index, xboolean_t is_big)
{
#ifndef USE_SMALL_ARRAYS
  is_big = TRUE;
#endif
  return is_big ? *(((xpointer_t *) a) + index) : GUINT_TO_POINTER (*(((xuint_t *) a) + index));
}

static inline void
xhash_table_assign_key_or_value (xpointer_t a, xuint_t index, xboolean_t is_big, xpointer_t v)
{
#ifndef USE_SMALL_ARRAYS
  is_big = TRUE;
#endif
  if (is_big)
    *(((xpointer_t *) a) + index) = v;
  else
    *(((xuint_t *) a) + index) = GPOINTER_TO_UINT (v);
}

static inline xpointer_t
xhash_table_evict_key_or_value (xpointer_t a, xuint_t index, xboolean_t is_big, xpointer_t v)
{
#ifndef USE_SMALL_ARRAYS
  is_big = TRUE;
#endif
  if (is_big)
    {
      xpointer_t r = *(((xpointer_t *) a) + index);
      *(((xpointer_t *) a) + index) = v;
      return r;
    }
  else
    {
      xpointer_t r = GUINT_TO_POINTER (*(((xuint_t *) a) + index));
      *(((xuint_t *) a) + index) = GPOINTER_TO_UINT (v);
      return r;
    }
}

static inline xuint_t
xhash_table_hash_to_index (xhashtable_t *hash_table, xuint_t hash)
{
  /* Multiply the hash by a small prime before applying the modulo. This
   * prevents the table from becoming densely packed, even with a poor hash
   * function. A densely packed table would have poor performance on
   * workloads with many failed lookups or a high degree of churn. */
  return (hash * 11) % hash_table->mod;
}

/*
 * xhash_table_lookup_node:
 * @hash_table: our #xhashtable_t
 * @key: the key to look up against
 * @hash_return: key hash return location
 *
 * Performs a lookup in the hash table, preserving extra information
 * usually needed for insertion.
 *
 * This function first computes the hash value of the key using the
 * user's hash function.
 *
 * If an entry in the table matching @key is found then this function
 * returns the index of that entry in the table, and if not, the
 * index of an unused node (empty or tombstone) where the key can be
 * inserted.
 *
 * The computed hash value is returned in the variable pointed to
 * by @hash_return. This is to save insertions from having to compute
 * the hash record again for the new record.
 *
 * Returns: index of the described node
 */
static inline xuint_t
xhash_table_lookup_node (xhashtable_t    *hash_table,
                          xconstpointer  key,
                          xuint_t         *hash_return)
{
  xuint_t node_index;
  xuint_t node_hash;
  xuint_t hash_value;
  xuint_t first_tombstone = 0;
  xboolean_t have_tombstone = FALSE;
  xuint_t step = 0;

  hash_value = hash_table->hash_func (key);
  if (G_UNLIKELY (!HASH_IS_REAL (hash_value)))
    hash_value = 2;

  *hash_return = hash_value;

  node_index = xhash_table_hash_to_index (hash_table, hash_value);
  node_hash = hash_table->hashes[node_index];

  while (!HASH_IS_UNUSED (node_hash))
    {
      /* We first check if our full hash values
       * are equal so we can avoid calling the full-blown
       * key equality function in most cases.
       */
      if (node_hash == hash_value)
        {
          xpointer_t node_key = xhash_table_fetch_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys);

          if (hash_table->key_equal_func)
            {
              if (hash_table->key_equal_func (node_key, key))
                return node_index;
            }
          else if (node_key == key)
            {
              return node_index;
            }
        }
      else if (HASH_IS_TOMBSTONE (node_hash) && !have_tombstone)
        {
          first_tombstone = node_index;
          have_tombstone = TRUE;
        }

      step++;
      node_index += step;
      node_index &= hash_table->mask;
      node_hash = hash_table->hashes[node_index];
    }

  if (have_tombstone)
    return first_tombstone;

  return node_index;
}

/*
 * xhash_table_remove_node:
 * @hash_table: our #xhashtable_t
 * @node: pointer to node to remove
 * @notify: %TRUE if the destroy notify handlers are to be called
 *
 * Removes a node from the hash table and updates the node count.
 * The node is replaced by a tombstone. No table resize is performed.
 *
 * If @notify is %TRUE then the destroy notify functions are called
 * for the key and value of the hash node.
 */
static void
xhash_table_remove_node (xhashtable_t   *hash_table,
                          xint_t          i,
                          xboolean_t      notify)
{
  xpointer_t key;
  xpointer_t value;

  key = xhash_table_fetch_key_or_value (hash_table->keys, i, hash_table->have_big_keys);
  value = xhash_table_fetch_key_or_value (hash_table->values, i, hash_table->have_bixvalues);

  /* Erect tombstone */
  hash_table->hashes[i] = TOMBSTONE_HASH_VALUE;

  /* Be GC friendly */
  xhash_table_assign_key_or_value (hash_table->keys, i, hash_table->have_big_keys, NULL);
  xhash_table_assign_key_or_value (hash_table->values, i, hash_table->have_bixvalues, NULL);

  hash_table->nnodes--;

  if (notify && hash_table->key_destroy_func)
    hash_table->key_destroy_func (key);

  if (notify && hash_table->value_destroy_func)
    hash_table->value_destroy_func (value);

}

/*
 * xhash_table_setup_storage:
 * @hash_table: our #xhashtable_t
 *
 * Initialise the hash table size, mask, mod, and arrays.
 */
static void
xhash_table_setup_storage (xhashtable_t *hash_table)
{
  xboolean_t small = FALSE;

  /* We want to use small arrays only if:
   *   - we are running on a system where that makes sense (64 bit); and
   *   - we are not running under valgrind.
   */

#ifdef USE_SMALL_ARRAYS
  small = TRUE;

# ifdef ENABLE_VALGRIND
  if (RUNNING_ON_VALGRIND)
    small = FALSE;
# endif
#endif

  xhash_table_set_shift (hash_table, HASH_TABLE_MIN_SHIFT);

  hash_table->have_big_keys = !small;
  hash_table->have_bixvalues = !small;

  hash_table->keys   = xhash_table_realloc_key_or_value_array (NULL, hash_table->size, hash_table->have_big_keys);
  hash_table->values = hash_table->keys;
  hash_table->hashes = g_new0 (xuint_t, hash_table->size);
}

/*
 * xhash_table_remove_all_nodes:
 * @hash_table: our #xhashtable_t
 * @notify: %TRUE if the destroy notify handlers are to be called
 *
 * Removes all nodes from the table.
 *
 * If @notify is %TRUE then the destroy notify functions are called
 * for the key and value of the hash node.
 *
 * Since this may be a precursor to freeing the table entirely, we'd
 * ideally perform no resize, and we can indeed avoid that in some
 * cases.  However: in the case that we'll be making callbacks to user
 * code (via destroy notifies) we need to consider that the user code
 * might call back into the table again.  In this case, we setup a new
 * set of arrays so that any callers will see an empty (but valid)
 * table.
 */
static void
xhash_table_remove_all_nodes (xhashtable_t *hash_table,
                               xboolean_t    notify,
                               xboolean_t    destruction)
{
  int i;
  xpointer_t key;
  xpointer_t value;
  xint_t old_size;
  xpointer_t *old_keys;
  xpointer_t *old_values;
  xuint_t    *old_hashes;
  xboolean_t  old_have_big_keys;
  xboolean_t  old_have_bixvalues;

  /* If the hash table is already empty, there is nothing to be done. */
  if (hash_table->nnodes == 0)
    return;

  hash_table->nnodes = 0;
  hash_table->noccupied = 0;

  /* Easy case: no callbacks, so we just zero out the arrays */
  if (!notify ||
      (hash_table->key_destroy_func == NULL &&
       hash_table->value_destroy_func == NULL))
    {
      if (!destruction)
        {
          memset (hash_table->hashes, 0, hash_table->size * sizeof (xuint_t));

#ifdef USE_SMALL_ARRAYS
          memset (hash_table->keys, 0, hash_table->size * (hash_table->have_big_keys ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
          memset (hash_table->values, 0, hash_table->size * (hash_table->have_bixvalues ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
#else
          memset (hash_table->keys, 0, hash_table->size * sizeof (xpointer_t));
          memset (hash_table->values, 0, hash_table->size * sizeof (xpointer_t));
#endif
        }

      return;
    }

  /* Hard case: we need to do user callbacks.  There are two
   * possibilities here:
   *
   *   1) there are no outstanding references on the table and therefore
   *   nobody should be calling into it again (destroying == true)
   *
   *   2) there are outstanding references, and there may be future
   *   calls into the table, either after we return, or from the destroy
   *   notifies that we're about to do (destroying == false)
   *
   * We handle both cases by taking the current state of the table into
   * local variables and replacing it with something else: in the "no
   * outstanding references" cases we replace it with a bunch of
   * null/zero values so that any access to the table will fail.  In the
   * "may receive future calls" case, we reinitialise the struct to
   * appear like a newly-created empty table.
   *
   * In both cases, we take over the references for the current state,
   * freeing them below.
   */
  old_size = hash_table->size;
  old_have_big_keys = hash_table->have_big_keys;
  old_have_bixvalues = hash_table->have_bixvalues;
  old_keys   = g_steal_pointer (&hash_table->keys);
  old_values = g_steal_pointer (&hash_table->values);
  old_hashes = g_steal_pointer (&hash_table->hashes);

  if (!destruction)
    /* Any accesses will see an empty table */
    xhash_table_setup_storage (hash_table);
  else
    /* Will cause a quick crash on any attempted access */
    hash_table->size = hash_table->mod = hash_table->mask = 0;

  /* Now do the actual destroy notifies */
  for (i = 0; i < old_size; i++)
    {
      if (HASH_IS_REAL (old_hashes[i]))
        {
          key = xhash_table_fetch_key_or_value (old_keys, i, old_have_big_keys);
          value = xhash_table_fetch_key_or_value (old_values, i, old_have_bixvalues);

          old_hashes[i] = UNUSED_HASH_VALUE;

          xhash_table_assign_key_or_value (old_keys, i, old_have_big_keys, NULL);
          xhash_table_assign_key_or_value (old_values, i, old_have_bixvalues, NULL);

          if (hash_table->key_destroy_func != NULL)
            hash_table->key_destroy_func (key);

          if (hash_table->value_destroy_func != NULL)
            hash_table->value_destroy_func (value);
        }
    }

  /* Destroy old storage space. */
  if (old_keys != old_values)
    g_free (old_values);

  g_free (old_keys);
  g_free (old_hashes);
}

static void
realloc_arrays (xhashtable_t *hash_table, xboolean_t is_a_set)
{
  hash_table->hashes = g_renew (xuint_t, hash_table->hashes, hash_table->size);
  hash_table->keys = xhash_table_realloc_key_or_value_array (hash_table->keys, hash_table->size, hash_table->have_big_keys);

  if (is_a_set)
    hash_table->values = hash_table->keys;
  else
    hash_table->values = xhash_table_realloc_key_or_value_array (hash_table->values, hash_table->size, hash_table->have_bixvalues);
}

/* When resizing the table in place, we use a temporary bit array to keep
 * track of which entries have been assigned a proper location in the new
 * table layout.
 *
 * Each bit corresponds to a bucket. A bit is set if an entry was assigned
 * its corresponding location during the resize and thus should not be
 * evicted. The array starts out cleared to zero. */

static inline xboolean_t
get_status_bit (const xuint32_t *bitmap, xuint_t index)
{
  return (bitmap[index / 32] >> (index % 32)) & 1;
}

static inline void
set_status_bit (xuint32_t *bitmap, xuint_t index)
{
  bitmap[index / 32] |= 1U << (index % 32);
}

/* By calling dedicated resize functions for sets and maps, we avoid 2x
 * test-and-branch per key in the inner loop. This yields a small
 * performance improvement at the cost of a bit of macro gunk. */

#define DEFINE_RESIZE_FUNC(fname) \
static void fname (xhashtable_t *hash_table, xuint_t old_size, xuint32_t *reallocated_buckets_bitmap) \
{                                                                       \
  xuint_t i;                                                              \
                                                                        \
  for (i = 0; i < old_size; i++)                                        \
    {                                                                   \
      xuint_t node_hash = hash_table->hashes[i];                          \
      xpointer_t key, value G_GNUC_UNUSED;                                \
                                                                        \
      if (!HASH_IS_REAL (node_hash))                                    \
        {                                                               \
          /* Clear tombstones */                                        \
          hash_table->hashes[i] = UNUSED_HASH_VALUE;                    \
          continue;                                                     \
        }                                                               \
                                                                        \
      /* Skip entries relocated through eviction */                     \
      if (get_status_bit (reallocated_buckets_bitmap, i))               \
        continue;                                                       \
                                                                        \
      hash_table->hashes[i] = UNUSED_HASH_VALUE;                        \
      EVICT_KEYVAL (hash_table, i, NULL, NULL, key, value);             \
                                                                        \
      for (;;)                                                          \
        {                                                               \
          xuint_t hash_val;                                               \
          xuint_t replaced_hash;                                          \
          xuint_t step = 0;                                               \
                                                                        \
          hash_val = xhash_table_hash_to_index (hash_table, node_hash); \
                                                                        \
          while (get_status_bit (reallocated_buckets_bitmap, hash_val)) \
            {                                                           \
              step++;                                                   \
              hash_val += step;                                         \
              hash_val &= hash_table->mask;                             \
            }                                                           \
                                                                        \
          set_status_bit (reallocated_buckets_bitmap, hash_val);        \
                                                                        \
          replaced_hash = hash_table->hashes[hash_val];                 \
          hash_table->hashes[hash_val] = node_hash;                     \
          if (!HASH_IS_REAL (replaced_hash))                            \
            {                                                           \
              ASSIGN_KEYVAL (hash_table, hash_val, key, value);         \
              break;                                                    \
            }                                                           \
                                                                        \
          node_hash = replaced_hash;                                    \
          EVICT_KEYVAL (hash_table, hash_val, key, value, key, value);  \
        }                                                               \
    }                                                                   \
}

#define ASSIGN_KEYVAL(ht, index, key, value) G_STMT_START{ \
    xhash_table_assign_key_or_value ((ht)->keys, (index), (ht)->have_big_keys, (key)); \
    xhash_table_assign_key_or_value ((ht)->values, (index), (ht)->have_bixvalues, (value)); \
  }G_STMT_END

#define EVICT_KEYVAL(ht, index, key, value, outkey, outvalue) G_STMT_START{ \
    (outkey) = xhash_table_evict_key_or_value ((ht)->keys, (index), (ht)->have_big_keys, (key)); \
    (outvalue) = xhash_table_evict_key_or_value ((ht)->values, (index), (ht)->have_bixvalues, (value)); \
  }G_STMT_END

DEFINE_RESIZE_FUNC (resize_map)

#undef ASSIGN_KEYVAL
#undef EVICT_KEYVAL

#define ASSIGN_KEYVAL(ht, index, key, value) G_STMT_START{ \
    xhash_table_assign_key_or_value ((ht)->keys, (index), (ht)->have_big_keys, (key)); \
  }G_STMT_END

#define EVICT_KEYVAL(ht, index, key, value, outkey, outvalue) G_STMT_START{ \
    (outkey) = xhash_table_evict_key_or_value ((ht)->keys, (index), (ht)->have_big_keys, (key)); \
  }G_STMT_END

DEFINE_RESIZE_FUNC (resize_set)

#undef ASSIGN_KEYVAL
#undef EVICT_KEYVAL

/*
 * xhash_table_resize:
 * @hash_table: our #xhashtable_t
 *
 * Resizes the hash table to the optimal size based on the number of
 * nodes currently held. If you call this function then a resize will
 * occur, even if one does not need to occur.
 * Use xhash_table_maybe_resize() instead.
 *
 * This function may "resize" the hash table to its current size, with
 * the side effect of cleaning up tombstones and otherwise optimizing
 * the probe sequences.
 */
static void
xhash_table_resize (xhashtable_t *hash_table)
{
  xuint32_t *reallocated_buckets_bitmap;
  xsize_t old_size;
  xboolean_t is_a_set;

  old_size = hash_table->size;
  is_a_set = hash_table->keys == hash_table->values;

  /* The outer checks in xhash_table_maybe_resize() will only consider
   * cleanup/resize when the load factor goes below .25 (1/4, ignoring
   * tombstones) or above .9375 (15/16, including tombstones).
   *
   * Once this happens, tombstones will always be cleaned out. If our
   * load sans tombstones is greater than .75 (1/1.333, see below), we'll
   * take this opportunity to grow the table too.
   *
   * Immediately after growing, the load factor will be in the range
   * .375 .. .469. After shrinking, it will be exactly .5. */

  xhash_table_set_shift_from_size (hash_table, hash_table->nnodes * 1.333);

  if (hash_table->size > old_size)
    {
      realloc_arrays (hash_table, is_a_set);
      memset (&hash_table->hashes[old_size], 0, (hash_table->size - old_size) * sizeof (xuint_t));

      reallocated_buckets_bitmap = g_new0 (xuint32_t, (hash_table->size + 31) / 32);
    }
  else
    {
      reallocated_buckets_bitmap = g_new0 (xuint32_t, (old_size + 31) / 32);
    }

  if (is_a_set)
    resize_set (hash_table, old_size, reallocated_buckets_bitmap);
  else
    resize_map (hash_table, old_size, reallocated_buckets_bitmap);

  g_free (reallocated_buckets_bitmap);

  if (hash_table->size < old_size)
    realloc_arrays (hash_table, is_a_set);

  hash_table->noccupied = hash_table->nnodes;
}

/*
 * xhash_table_maybe_resize:
 * @hash_table: our #xhashtable_t
 *
 * Resizes the hash table, if needed.
 *
 * Essentially, calls xhash_table_resize() if the table has strayed
 * too far from its ideal size for its number of nodes.
 */
static inline void
xhash_table_maybe_resize (xhashtable_t *hash_table)
{
  xint_t noccupied = hash_table->noccupied;
  xint_t size = hash_table->size;

  if ((size > hash_table->nnodes * 4 && size > 1 << HASH_TABLE_MIN_SHIFT) ||
      (size <= noccupied + (noccupied / 16)))
    xhash_table_resize (hash_table);
}

#ifdef USE_SMALL_ARRAYS

static inline xboolean_t
entry_is_big (xpointer_t v)
{
  return (((guintptr) v) >> ((BIG_ENTRY_SIZE - SMALL_ENTRY_SIZE) * 8)) != 0;
}

static inline xboolean_t
xhash_table_maybe_make_big_keys_or_values (xpointer_t *a_p, xpointer_t v, xint_t ht_size)
{
  if (entry_is_big (v))
    {
      xuint_t *a = (xuint_t *) *a_p;
      xpointer_t *a_new;
      xint_t i;

      a_new = g_new (xpointer_t, ht_size);

      for (i = 0; i < ht_size; i++)
        {
          a_new[i] = GUINT_TO_POINTER (a[i]);
        }

      g_free (a);
      *a_p = a_new;
      return TRUE;
    }

  return FALSE;
}

#endif

static inline void
xhash_table_ensure_keyval_fits (xhashtable_t *hash_table, xpointer_t key, xpointer_t value)
{
  xboolean_t is_a_set = (hash_table->keys == hash_table->values);

#ifdef USE_SMALL_ARRAYS

  /* Convert from set to map? */
  if (is_a_set)
    {
      if (hash_table->have_big_keys)
        {
          if (key != value)
            hash_table->values = g_memdup2 (hash_table->keys, sizeof (xpointer_t) * hash_table->size);
          /* Keys and values are both big now, so no need for further checks */
          return;
        }
      else
        {
          if (key != value)
            {
              hash_table->values = g_memdup2 (hash_table->keys, sizeof (xuint_t) * hash_table->size);
              is_a_set = FALSE;
            }
        }
    }

  /* Make keys big? */
  if (!hash_table->have_big_keys)
    {
      hash_table->have_big_keys = xhash_table_maybe_make_big_keys_or_values (&hash_table->keys, key, hash_table->size);

      if (is_a_set)
        {
          hash_table->values = hash_table->keys;
          hash_table->have_bixvalues = hash_table->have_big_keys;
        }
    }

  /* Make values big? */
  if (!is_a_set && !hash_table->have_bixvalues)
    {
      hash_table->have_bixvalues = xhash_table_maybe_make_big_keys_or_values (&hash_table->values, value, hash_table->size);
    }

#else

  /* Just split if necessary */
  if (is_a_set && key != value)
    hash_table->values = g_memdup2 (hash_table->keys, sizeof (xpointer_t) * hash_table->size);

#endif
}

/**
 * xhash_table_new:
 * @hash_func: a function to create a hash value from a key
 * @key_equal_func: a function to check two keys for equality
 *
 * Creates a new #xhashtable_t with a reference count of 1.
 *
 * Hash values returned by @hash_func are used to determine where keys
 * are stored within the #xhashtable_t data structure. The g_direct_hash(),
 * g_int_hash(), g_int64_hash(), g_double_hash() and xstr_hash()
 * functions are provided for some common types of keys.
 * If @hash_func is %NULL, g_direct_hash() is used.
 *
 * @key_equal_func is used when looking up keys in the #xhashtable_t.
 * The g_direct_equal(), g_int_equal(), g_int64_equal(), g_double_equal()
 * and xstr_equal() functions are provided for the most common types
 * of keys. If @key_equal_func is %NULL, keys are compared directly in
 * a similar fashion to g_direct_equal(), but without the overhead of
 * a function call. @key_equal_func is called with the key from the hash table
 * as its first parameter, and the user-provided key to check against as
 * its second.
 *
 * Returns: a new #xhashtable_t
 */
xhashtable_t *
xhash_table_new (GHashFunc  hash_func,
                  GEqualFunc key_equal_func)
{
  return xhash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}


/**
 * xhash_table_new_full:
 * @hash_func: a function to create a hash value from a key
 * @key_equal_func: a function to check two keys for equality
 * @key_destroy_func: (nullable): a function to free the memory allocated for the key
 *     used when removing the entry from the #xhashtable_t, or %NULL
 *     if you don't want to supply such a function.
 * @value_destroy_func: (nullable): a function to free the memory allocated for the
 *     value used when removing the entry from the #xhashtable_t, or %NULL
 *     if you don't want to supply such a function.
 *
 * Creates a new #xhashtable_t like xhash_table_new() with a reference
 * count of 1 and allows to specify functions to free the memory
 * allocated for the key and value that get called when removing the
 * entry from the #xhashtable_t.
 *
 * Since version 2.42 it is permissible for destroy notify functions to
 * recursively remove further items from the hash table. This is only
 * permissible if the application still holds a reference to the hash table.
 * This means that you may need to ensure that the hash table is empty by
 * calling xhash_table_remove_all() before releasing the last reference using
 * xhash_table_unref().
 *
 * Returns: a new #xhashtable_t
 */
xhashtable_t *
xhash_table_new_full (GHashFunc      hash_func,
                       GEqualFunc     key_equal_func,
                       xdestroy_notify_t key_destroy_func,
                       xdestroy_notify_t value_destroy_func)
{
  xhashtable_t *hash_table;

  hash_table = g_slice_new (xhashtable_t);
  g_atomic_ref_count_init (&hash_table->ref_count);
  hash_table->nnodes             = 0;
  hash_table->noccupied          = 0;
  hash_table->hash_func          = hash_func ? hash_func : g_direct_hash;
  hash_table->key_equal_func     = key_equal_func;
#ifndef G_DISABLE_ASSERT
  hash_table->version            = 0;
#endif
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;

  xhash_table_setup_storage (hash_table);

  return hash_table;
}

/**
 * xhash_table_new_similar:
 * @other_hash_table: (not nullable) (transfer none): Another #xhashtable_t
 *
 * Creates a new #xhashtable_t like xhash_table_new_full() with a reference
 * count of 1.
 *
 * It inherits the hash function, the key equal function, the key destroy function,
 * as well as the value destroy function, from @other_hash_table.
 *
 * The returned hash table will be empty; it will not contain the keys
 * or values from @other_hash_table.
 *
 * Returns: (transfer full) (not nullable): a new #xhashtable_t
 * Since: 2.72
 */
xhashtable_t *
xhash_table_new_similar (xhashtable_t *other_hash_table)
{
  xreturn_val_if_fail (other_hash_table, NULL);

  return xhash_table_new_full (other_hash_table->hash_func,
                                other_hash_table->key_equal_func,
                                other_hash_table->key_destroy_func,
                                other_hash_table->value_destroy_func);
}

/**
 * xhash_table_iter_init:
 * @iter: an uninitialized #xhash_table_iter_t
 * @hash_table: a #xhashtable_t
 *
 * Initializes a key/value pair iterator and associates it with
 * @hash_table. Modifying the hash table after calling this function
 * invalidates the returned iterator.
 *
 * The iteration order of a #xhash_table_iter_t over the keys/values in a hash
 * table is not defined.
 *
 * |[<!-- language="C" -->
 * xhash_table_iter_t iter;
 * xpointer_t key, value;
 *
 * xhash_table_iter_init (&iter, hash_table);
 * while (xhash_table_iter_next (&iter, &key, &value))
 *   {
 *     // do something with key and value
 *   }
 * ]|
 *
 * Since: 2.16
 */
void
xhash_table_iter_init (xhash_table_iter_t *iter,
                        xhashtable_t     *hash_table)
{
  RealIter *ri = (RealIter *) iter;

  g_return_if_fail (iter != NULL);
  g_return_if_fail (hash_table != NULL);

  ri->hash_table = hash_table;
  ri->position = -1;
#ifndef G_DISABLE_ASSERT
  ri->version = hash_table->version;
#endif
}

/**
 * xhash_table_iter_next:
 * @iter: an initialized #xhash_table_iter_t
 * @key: (out) (optional): a location to store the key
 * @value: (out) (optional) (nullable): a location to store the value
 *
 * Advances @iter and retrieves the key and/or value that are now
 * pointed to as a result of this advancement. If %FALSE is returned,
 * @key and @value are not set, and the iterator becomes invalid.
 *
 * Returns: %FALSE if the end of the #xhashtable_t has been reached.
 *
 * Since: 2.16
 */
xboolean_t
xhash_table_iter_next (xhash_table_iter_t *iter,
                        xpointer_t       *key,
                        xpointer_t       *value)
{
  RealIter *ri = (RealIter *) iter;
  xint_t position;

  xreturn_val_if_fail (iter != NULL, FALSE);
#ifndef G_DISABLE_ASSERT
  xreturn_val_if_fail (ri->version == ri->hash_table->version, FALSE);
#endif
  xreturn_val_if_fail (ri->position < (xssize_t) ri->hash_table->size, FALSE);

  position = ri->position;

  do
    {
      position++;
      if (position >= (xssize_t) ri->hash_table->size)
        {
          ri->position = position;
          return FALSE;
        }
    }
  while (!HASH_IS_REAL (ri->hash_table->hashes[position]));

  if (key != NULL)
    *key = xhash_table_fetch_key_or_value (ri->hash_table->keys, position, ri->hash_table->have_big_keys);
  if (value != NULL)
    *value = xhash_table_fetch_key_or_value (ri->hash_table->values, position, ri->hash_table->have_bixvalues);

  ri->position = position;
  return TRUE;
}

/**
 * xhash_table_iter_get_hash_table:
 * @iter: an initialized #xhash_table_iter_t
 *
 * Returns the #xhashtable_t associated with @iter.
 *
 * Returns: the #xhashtable_t associated with @iter.
 *
 * Since: 2.16
 */
xhashtable_t *
xhash_table_iter_get_hash_table (xhash_table_iter_t *iter)
{
  xreturn_val_if_fail (iter != NULL, NULL);

  return ((RealIter *) iter)->hash_table;
}

static void
iter_remove_or_steal (RealIter *ri, xboolean_t notify)
{
  g_return_if_fail (ri != NULL);
#ifndef G_DISABLE_ASSERT
  g_return_if_fail (ri->version == ri->hash_table->version);
#endif
  g_return_if_fail (ri->position >= 0);
  g_return_if_fail ((xsize_t) ri->position < ri->hash_table->size);

  xhash_table_remove_node (ri->hash_table, ri->position, notify);

#ifndef G_DISABLE_ASSERT
  ri->version++;
  ri->hash_table->version++;
#endif
}

/**
 * xhash_table_iter_remove:
 * @iter: an initialized #xhash_table_iter_t
 *
 * Removes the key/value pair currently pointed to by the iterator
 * from its associated #xhashtable_t. Can only be called after
 * xhash_table_iter_next() returned %TRUE, and cannot be called
 * more than once for the same key/value pair.
 *
 * If the #xhashtable_t was created using xhash_table_new_full(),
 * the key and value are freed using the supplied destroy functions,
 * otherwise you have to make sure that any dynamically allocated
 * values are freed yourself.
 *
 * It is safe to continue iterating the #xhashtable_t afterward:
 * |[<!-- language="C" -->
 * while (xhash_table_iter_next (&iter, &key, &value))
 *   {
 *     if (condition)
 *       xhash_table_iter_remove (&iter);
 *   }
 * ]|
 *
 * Since: 2.16
 */
void
xhash_table_iter_remove (xhash_table_iter_t *iter)
{
  iter_remove_or_steal ((RealIter *) iter, TRUE);
}

/*
 * xhash_table_insert_node:
 * @hash_table: our #xhashtable_t
 * @node_index: pointer to node to insert/replace
 * @key_hash: key hash
 * @key: (nullable): key to replace with, or %NULL
 * @value: value to replace with
 * @keep_new_key: whether to replace the key in the node with @key
 * @reusing_key: whether @key was taken out of the existing node
 *
 * Inserts a value at @node_index in the hash table and updates it.
 *
 * If @key has been taken out of the existing node (ie it is not
 * passed in via a xhash_table_insert/replace) call, then @reusing_key
 * should be %TRUE.
 *
 * Returns: %TRUE if the key did not exist yet
 */
static xboolean_t
xhash_table_insert_node (xhashtable_t *hash_table,
                          xuint_t       node_index,
                          xuint_t       key_hash,
                          xpointer_t    new_key,
                          xpointer_t    new_value,
                          xboolean_t    keep_new_key,
                          xboolean_t    reusing_key)
{
  xboolean_t already_exists;
  xuint_t old_hash;
  xpointer_t key_to_free = NULL;
  xpointer_t key_to_keep = NULL;
  xpointer_t value_to_free = NULL;

  old_hash = hash_table->hashes[node_index];
  already_exists = HASH_IS_REAL (old_hash);

  /* Proceed in three steps.  First, deal with the key because it is the
   * most complicated.  Then consider if we need to split the table in
   * two (because writing the value will result in the set invariant
   * becoming broken).  Then deal with the value.
   *
   * There are three cases for the key:
   *
   *  - entry already exists in table, reusing key:
   *    free the just-passed-in new_key and use the existing value
   *
   *  - entry already exists in table, not reusing key:
   *    free the entry in the table, use the new key
   *
   *  - entry not already in table:
   *    use the new key, free nothing
   *
   * We update the hash at the same time...
   */
  if (already_exists)
    {
      /* Note: we must record the old value before writing the new key
       * because we might change the value in the event that the two
       * arrays are shared.
       */
      value_to_free = xhash_table_fetch_key_or_value (hash_table->values, node_index, hash_table->have_bixvalues);

      if (keep_new_key)
        {
          key_to_free = xhash_table_fetch_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys);
          key_to_keep = new_key;
        }
      else
        {
          key_to_free = new_key;
          key_to_keep = xhash_table_fetch_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys);
        }
    }
  else
    {
      hash_table->hashes[node_index] = key_hash;
      key_to_keep = new_key;
    }

  /* Resize key/value arrays and split table as necessary */
  xhash_table_ensure_keyval_fits (hash_table, key_to_keep, new_value);
  xhash_table_assign_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys, key_to_keep);

  /* Step 3: Actually do the write */
  xhash_table_assign_key_or_value (hash_table->values, node_index, hash_table->have_bixvalues, new_value);

  /* Now, the bookkeeping... */
  if (!already_exists)
    {
      hash_table->nnodes++;

      if (HASH_IS_UNUSED (old_hash))
        {
          /* We replaced an empty node, and not a tombstone */
          hash_table->noccupied++;
          xhash_table_maybe_resize (hash_table);
        }

#ifndef G_DISABLE_ASSERT
      hash_table->version++;
#endif
    }

  if (already_exists)
    {
      if (hash_table->key_destroy_func && !reusing_key)
        (* hash_table->key_destroy_func) (key_to_free);
      if (hash_table->value_destroy_func)
        (* hash_table->value_destroy_func) (value_to_free);
    }

  return !already_exists;
}

/**
 * xhash_table_iter_replace:
 * @iter: an initialized #xhash_table_iter_t
 * @value: the value to replace with
 *
 * Replaces the value currently pointed to by the iterator
 * from its associated #xhashtable_t. Can only be called after
 * xhash_table_iter_next() returned %TRUE.
 *
 * If you supplied a @value_destroy_func when creating the
 * #xhashtable_t, the old value is freed using that function.
 *
 * Since: 2.30
 */
void
xhash_table_iter_replace (xhash_table_iter_t *iter,
                           xpointer_t        value)
{
  RealIter *ri;
  xuint_t node_hash;
  xpointer_t key;

  ri = (RealIter *) iter;

  g_return_if_fail (ri != NULL);
#ifndef G_DISABLE_ASSERT
  g_return_if_fail (ri->version == ri->hash_table->version);
#endif
  g_return_if_fail (ri->position >= 0);
  g_return_if_fail ((xsize_t) ri->position < ri->hash_table->size);

  node_hash = ri->hash_table->hashes[ri->position];

  key = xhash_table_fetch_key_or_value (ri->hash_table->keys, ri->position, ri->hash_table->have_big_keys);

  xhash_table_insert_node (ri->hash_table, ri->position, node_hash, key, value, TRUE, TRUE);

#ifndef G_DISABLE_ASSERT
  ri->version++;
  ri->hash_table->version++;
#endif
}

/**
 * xhash_table_iter_steal:
 * @iter: an initialized #xhash_table_iter_t
 *
 * Removes the key/value pair currently pointed to by the
 * iterator from its associated #xhashtable_t, without calling
 * the key and value destroy functions. Can only be called
 * after xhash_table_iter_next() returned %TRUE, and cannot
 * be called more than once for the same key/value pair.
 *
 * Since: 2.16
 */
void
xhash_table_iter_steal (xhash_table_iter_t *iter)
{
  iter_remove_or_steal ((RealIter *) iter, FALSE);
}


/**
 * xhash_table_ref:
 * @hash_table: a valid #xhashtable_t
 *
 * Atomically increments the reference count of @hash_table by one.
 * This function is MT-safe and may be called from any thread.
 *
 * Returns: the passed in #xhashtable_t
 *
 * Since: 2.10
 */
xhashtable_t *
xhash_table_ref (xhashtable_t *hash_table)
{
  xreturn_val_if_fail (hash_table != NULL, NULL);

  g_atomic_ref_count_inc (&hash_table->ref_count);

  return hash_table;
}

/**
 * xhash_table_unref:
 * @hash_table: a valid #xhashtable_t
 *
 * Atomically decrements the reference count of @hash_table by one.
 * If the reference count drops to 0, all keys and values will be
 * destroyed, and all memory allocated by the hash table is released.
 * This function is MT-safe and may be called from any thread.
 *
 * Since: 2.10
 */
void
xhash_table_unref (xhashtable_t *hash_table)
{
  g_return_if_fail (hash_table != NULL);

  if (g_atomic_ref_count_dec (&hash_table->ref_count))
    {
      xhash_table_remove_all_nodes (hash_table, TRUE, TRUE);
      if (hash_table->keys != hash_table->values)
        g_free (hash_table->values);
      g_free (hash_table->keys);
      g_free (hash_table->hashes);
      g_slice_free (xhashtable_t, hash_table);
    }
}

/**
 * xhash_table_destroy:
 * @hash_table: a #xhashtable_t
 *
 * Destroys all keys and values in the #xhashtable_t and decrements its
 * reference count by 1. If keys and/or values are dynamically allocated,
 * you should either free them first or create the #xhashtable_t with destroy
 * notifiers using xhash_table_new_full(). In the latter case the destroy
 * functions you supplied will be called on all keys and values during the
 * destruction phase.
 */
void
xhash_table_destroy (xhashtable_t *hash_table)
{
  g_return_if_fail (hash_table != NULL);

  xhash_table_remove_all (hash_table);
  xhash_table_unref (hash_table);
}

/**
 * xhash_table_lookup:
 * @hash_table: a #xhashtable_t
 * @key: the key to look up
 *
 * Looks up a key in a #xhashtable_t. Note that this function cannot
 * distinguish between a key that is not present and one which is present
 * and has the value %NULL. If you need this distinction, use
 * xhash_table_lookup_extended().
 *
 * Returns: (nullable): the associated value, or %NULL if the key is not found
 */
xpointer_t
xhash_table_lookup (xhashtable_t    *hash_table,
                     xconstpointer  key)
{
  xuint_t node_index;
  xuint_t node_hash;

  xreturn_val_if_fail (hash_table != NULL, NULL);

  node_index = xhash_table_lookup_node (hash_table, key, &node_hash);

  return HASH_IS_REAL (hash_table->hashes[node_index])
    ? xhash_table_fetch_key_or_value (hash_table->values, node_index, hash_table->have_bixvalues)
    : NULL;
}

/**
 * xhash_table_lookup_extended:
 * @hash_table: a #xhashtable_t
 * @lookup_key: the key to look up
 * @orig_key: (out) (optional): return location for the original key
 * @value: (out) (optional) (nullable): return location for the value associated
 * with the key
 *
 * Looks up a key in the #xhashtable_t, returning the original key and the
 * associated value and a #xboolean_t which is %TRUE if the key was found. This
 * is useful if you need to free the memory allocated for the original key,
 * for example before calling xhash_table_remove().
 *
 * You can actually pass %NULL for @lookup_key to test
 * whether the %NULL key exists, provided the hash and equal functions
 * of @hash_table are %NULL-safe.
 *
 * Returns: %TRUE if the key was found in the #xhashtable_t
 */
xboolean_t
xhash_table_lookup_extended (xhashtable_t    *hash_table,
                              xconstpointer  lookup_key,
                              xpointer_t      *orig_key,
                              xpointer_t      *value)
{
  xuint_t node_index;
  xuint_t node_hash;

  xreturn_val_if_fail (hash_table != NULL, FALSE);

  node_index = xhash_table_lookup_node (hash_table, lookup_key, &node_hash);

  if (!HASH_IS_REAL (hash_table->hashes[node_index]))
    {
      if (orig_key != NULL)
        *orig_key = NULL;
      if (value != NULL)
        *value = NULL;

      return FALSE;
    }

  if (orig_key)
    *orig_key = xhash_table_fetch_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys);

  if (value)
    *value = xhash_table_fetch_key_or_value (hash_table->values, node_index, hash_table->have_bixvalues);

  return TRUE;
}

/*
 * xhash_table_insert_internal:
 * @hash_table: our #xhashtable_t
 * @key: the key to insert
 * @value: the value to insert
 * @keep_new_key: if %TRUE and this key already exists in the table
 *   then call the destroy notify function on the old key.  If %FALSE
 *   then call the destroy notify function on the new key.
 *
 * Implements the common logic for the xhash_table_insert() and
 * xhash_table_replace() functions.
 *
 * Do a lookup of @key. If it is found, replace it with the new
 * @value (and perhaps the new @key). If it is not found, create
 * a new node.
 *
 * Returns: %TRUE if the key did not exist yet
 */
static xboolean_t
xhash_table_insert_internal (xhashtable_t *hash_table,
                              xpointer_t    key,
                              xpointer_t    value,
                              xboolean_t    keep_new_key)
{
  xuint_t key_hash;
  xuint_t node_index;

  xreturn_val_if_fail (hash_table != NULL, FALSE);

  node_index = xhash_table_lookup_node (hash_table, key, &key_hash);

  return xhash_table_insert_node (hash_table, node_index, key_hash, key, value, keep_new_key, FALSE);
}

/**
 * xhash_table_insert:
 * @hash_table: a #xhashtable_t
 * @key: a key to insert
 * @value: the value to associate with the key
 *
 * Inserts a new key and value into a #xhashtable_t.
 *
 * If the key already exists in the #xhashtable_t its current
 * value is replaced with the new value. If you supplied a
 * @value_destroy_func when creating the #xhashtable_t, the old
 * value is freed using that function. If you supplied a
 * @key_destroy_func when creating the #xhashtable_t, the passed
 * key is freed using that function.
 *
 * Starting from GLib 2.40, this function returns a boolean value to
 * indicate whether the newly added value was already in the hash table
 * or not.
 *
 * Returns: %TRUE if the key did not exist yet
 */
xboolean_t
xhash_table_insert (xhashtable_t *hash_table,
                     xpointer_t    key,
                     xpointer_t    value)
{
  return xhash_table_insert_internal (hash_table, key, value, FALSE);
}

/**
 * xhash_table_replace:
 * @hash_table: a #xhashtable_t
 * @key: a key to insert
 * @value: the value to associate with the key
 *
 * Inserts a new key and value into a #xhashtable_t similar to
 * xhash_table_insert(). The difference is that if the key
 * already exists in the #xhashtable_t, it gets replaced by the
 * new key. If you supplied a @value_destroy_func when creating
 * the #xhashtable_t, the old value is freed using that function.
 * If you supplied a @key_destroy_func when creating the
 * #xhashtable_t, the old key is freed using that function.
 *
 * Starting from GLib 2.40, this function returns a boolean value to
 * indicate whether the newly added value was already in the hash table
 * or not.
 *
 * Returns: %TRUE if the key did not exist yet
 */
xboolean_t
xhash_table_replace (xhashtable_t *hash_table,
                      xpointer_t    key,
                      xpointer_t    value)
{
  return xhash_table_insert_internal (hash_table, key, value, TRUE);
}

/**
 * xhash_table_add:
 * @hash_table: a #xhashtable_t
 * @key: (transfer full): a key to insert
 *
 * This is a convenience function for using a #xhashtable_t as a set.  It
 * is equivalent to calling xhash_table_replace() with @key as both the
 * key and the value.
 *
 * In particular, this means that if @key already exists in the hash table, then
 * the old copy of @key in the hash table is freed and @key replaces it in the
 * table.
 *
 * When a hash table only ever contains keys that have themselves as the
 * corresponding value it is able to be stored more efficiently.  See
 * the discussion in the section description.
 *
 * Starting from GLib 2.40, this function returns a boolean value to
 * indicate whether the newly added value was already in the hash table
 * or not.
 *
 * Returns: %TRUE if the key did not exist yet
 *
 * Since: 2.32
 */
xboolean_t
xhash_table_add (xhashtable_t *hash_table,
                  xpointer_t    key)
{
  return xhash_table_insert_internal (hash_table, key, key, TRUE);
}

/**
 * xhash_table_contains:
 * @hash_table: a #xhashtable_t
 * @key: a key to check
 *
 * Checks if @key is in @hash_table.
 *
 * Returns: %TRUE if @key is in @hash_table, %FALSE otherwise.
 *
 * Since: 2.32
 **/
xboolean_t
xhash_table_contains (xhashtable_t    *hash_table,
                       xconstpointer  key)
{
  xuint_t node_index;
  xuint_t node_hash;

  xreturn_val_if_fail (hash_table != NULL, FALSE);

  node_index = xhash_table_lookup_node (hash_table, key, &node_hash);

  return HASH_IS_REAL (hash_table->hashes[node_index]);
}

/*
 * xhash_table_remove_internal:
 * @hash_table: our #xhashtable_t
 * @key: the key to remove
 * @notify: %TRUE if the destroy notify handlers are to be called
 * Returns: %TRUE if a node was found and removed, else %FALSE
 *
 * Implements the common logic for the xhash_table_remove() and
 * xhash_table_steal() functions.
 *
 * Do a lookup of @key and remove it if it is found, calling the
 * destroy notify handlers only if @notify is %TRUE.
 */
static xboolean_t
xhash_table_remove_internal (xhashtable_t    *hash_table,
                              xconstpointer  key,
                              xboolean_t       notify)
{
  xuint_t node_index;
  xuint_t node_hash;

  xreturn_val_if_fail (hash_table != NULL, FALSE);

  node_index = xhash_table_lookup_node (hash_table, key, &node_hash);

  if (!HASH_IS_REAL (hash_table->hashes[node_index]))
    return FALSE;

  xhash_table_remove_node (hash_table, node_index, notify);
  xhash_table_maybe_resize (hash_table);

#ifndef G_DISABLE_ASSERT
  hash_table->version++;
#endif

  return TRUE;
}

/**
 * xhash_table_remove:
 * @hash_table: a #xhashtable_t
 * @key: the key to remove
 *
 * Removes a key and its associated value from a #xhashtable_t.
 *
 * If the #xhashtable_t was created using xhash_table_new_full(), the
 * key and value are freed using the supplied destroy functions, otherwise
 * you have to make sure that any dynamically allocated values are freed
 * yourself.
 *
 * Returns: %TRUE if the key was found and removed from the #xhashtable_t
 */
xboolean_t
xhash_table_remove (xhashtable_t    *hash_table,
                     xconstpointer  key)
{
  return xhash_table_remove_internal (hash_table, key, TRUE);
}

/**
 * xhash_table_steal:
 * @hash_table: a #xhashtable_t
 * @key: the key to remove
 *
 * Removes a key and its associated value from a #xhashtable_t without
 * calling the key and value destroy functions.
 *
 * Returns: %TRUE if the key was found and removed from the #xhashtable_t
 */
xboolean_t
xhash_table_steal (xhashtable_t    *hash_table,
                    xconstpointer  key)
{
  return xhash_table_remove_internal (hash_table, key, FALSE);
}

/**
 * xhash_table_steal_extended:
 * @hash_table: a #xhashtable_t
 * @lookup_key: the key to look up
 * @stolen_key: (out) (optional) (transfer full): return location for the
 *    original key
 * @stolen_value: (out) (optional) (nullable) (transfer full): return location
 *    for the value associated with the key
 *
 * Looks up a key in the #xhashtable_t, stealing the original key and the
 * associated value and returning %TRUE if the key was found. If the key was
 * not found, %FALSE is returned.
 *
 * If found, the stolen key and value are removed from the hash table without
 * calling the key and value destroy functions, and ownership is transferred to
 * the caller of this method; as with xhash_table_steal().
 *
 * You can pass %NULL for @lookup_key, provided the hash and equal functions
 * of @hash_table are %NULL-safe.
 *
 * Returns: %TRUE if the key was found in the #xhashtable_t
 * Since: 2.58
 */
xboolean_t
xhash_table_steal_extended (xhashtable_t    *hash_table,
                             xconstpointer  lookup_key,
                             xpointer_t      *stolen_key,
                             xpointer_t      *stolen_value)
{
  xuint_t node_index;
  xuint_t node_hash;

  xreturn_val_if_fail (hash_table != NULL, FALSE);

  node_index = xhash_table_lookup_node (hash_table, lookup_key, &node_hash);

  if (!HASH_IS_REAL (hash_table->hashes[node_index]))
    {
      if (stolen_key != NULL)
        *stolen_key = NULL;
      if (stolen_value != NULL)
        *stolen_value = NULL;
      return FALSE;
    }

  if (stolen_key != NULL)
  {
    *stolen_key = xhash_table_fetch_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys);
    xhash_table_assign_key_or_value (hash_table->keys, node_index, hash_table->have_big_keys, NULL);
  }

  if (stolen_value != NULL)
  {
    *stolen_value = xhash_table_fetch_key_or_value (hash_table->values, node_index, hash_table->have_bixvalues);
    xhash_table_assign_key_or_value (hash_table->values, node_index, hash_table->have_bixvalues, NULL);
  }

  xhash_table_remove_node (hash_table, node_index, FALSE);
  xhash_table_maybe_resize (hash_table);

#ifndef G_DISABLE_ASSERT
  hash_table->version++;
#endif

  return TRUE;
}

/**
 * xhash_table_remove_all:
 * @hash_table: a #xhashtable_t
 *
 * Removes all keys and their associated values from a #xhashtable_t.
 *
 * If the #xhashtable_t was created using xhash_table_new_full(),
 * the keys and values are freed using the supplied destroy functions,
 * otherwise you have to make sure that any dynamically allocated
 * values are freed yourself.
 *
 * Since: 2.12
 */
void
xhash_table_remove_all (xhashtable_t *hash_table)
{
  g_return_if_fail (hash_table != NULL);

#ifndef G_DISABLE_ASSERT
  if (hash_table->nnodes != 0)
    hash_table->version++;
#endif

  xhash_table_remove_all_nodes (hash_table, TRUE, FALSE);
  xhash_table_maybe_resize (hash_table);
}

/**
 * xhash_table_steal_all:
 * @hash_table: a #xhashtable_t
 *
 * Removes all keys and their associated values from a #xhashtable_t
 * without calling the key and value destroy functions.
 *
 * Since: 2.12
 */
void
xhash_table_steal_all (xhashtable_t *hash_table)
{
  g_return_if_fail (hash_table != NULL);

#ifndef G_DISABLE_ASSERT
  if (hash_table->nnodes != 0)
    hash_table->version++;
#endif

  xhash_table_remove_all_nodes (hash_table, FALSE, FALSE);
  xhash_table_maybe_resize (hash_table);
}

/*
 * xhash_table_foreach_remove_or_steal:
 * @hash_table: a #xhashtable_t
 * @func: the user's callback function
 * @user_data: data for @func
 * @notify: %TRUE if the destroy notify handlers are to be called
 *
 * Implements the common logic for xhash_table_foreach_remove()
 * and xhash_table_foreach_steal().
 *
 * Iterates over every node in the table, calling @func with the key
 * and value of the node (and @user_data). If @func returns %TRUE the
 * node is removed from the table.
 *
 * If @notify is true then the destroy notify handlers will be called
 * for each removed node.
 */
static xuint_t
xhash_table_foreach_remove_or_steal (xhashtable_t *hash_table,
                                      GHRFunc     func,
                                      xpointer_t    user_data,
                                      xboolean_t    notify)
{
  xuint_t deleted = 0;
  xsize_t i;
#ifndef G_DISABLE_ASSERT
  xint_t version = hash_table->version;
#endif

  for (i = 0; i < hash_table->size; i++)
    {
      xuint_t node_hash = hash_table->hashes[i];
      xpointer_t node_key = xhash_table_fetch_key_or_value (hash_table->keys, i, hash_table->have_big_keys);
      xpointer_t node_value = xhash_table_fetch_key_or_value (hash_table->values, i, hash_table->have_bixvalues);

      if (HASH_IS_REAL (node_hash) &&
          (* func) (node_key, node_value, user_data))
        {
          xhash_table_remove_node (hash_table, i, notify);
          deleted++;
        }

#ifndef G_DISABLE_ASSERT
      xreturn_val_if_fail (version == hash_table->version, 0);
#endif
    }

  xhash_table_maybe_resize (hash_table);

#ifndef G_DISABLE_ASSERT
  if (deleted > 0)
    hash_table->version++;
#endif

  return deleted;
}

/**
 * xhash_table_foreach_remove:
 * @hash_table: a #xhashtable_t
 * @func: the function to call for each key/value pair
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each key/value pair in the
 * #xhashtable_t. If the function returns %TRUE, then the key/value
 * pair is removed from the #xhashtable_t. If you supplied key or
 * value destroy functions when creating the #xhashtable_t, they are
 * used to free the memory allocated for the removed keys and values.
 *
 * See #xhash_table_iter_t for an alternative way to loop over the
 * key/value pairs in the hash table.
 *
 * Returns: the number of key/value pairs removed
 */
xuint_t
xhash_table_foreach_remove (xhashtable_t *hash_table,
                             GHRFunc     func,
                             xpointer_t    user_data)
{
  xreturn_val_if_fail (hash_table != NULL, 0);
  xreturn_val_if_fail (func != NULL, 0);

  return xhash_table_foreach_remove_or_steal (hash_table, func, user_data, TRUE);
}

/**
 * xhash_table_foreach_steal:
 * @hash_table: a #xhashtable_t
 * @func: the function to call for each key/value pair
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each key/value pair in the
 * #xhashtable_t. If the function returns %TRUE, then the key/value
 * pair is removed from the #xhashtable_t, but no key or value
 * destroy functions are called.
 *
 * See #xhash_table_iter_t for an alternative way to loop over the
 * key/value pairs in the hash table.
 *
 * Returns: the number of key/value pairs removed.
 */
xuint_t
xhash_table_foreach_steal (xhashtable_t *hash_table,
                            GHRFunc     func,
                            xpointer_t    user_data)
{
  xreturn_val_if_fail (hash_table != NULL, 0);
  xreturn_val_if_fail (func != NULL, 0);

  return xhash_table_foreach_remove_or_steal (hash_table, func, user_data, FALSE);
}

/**
 * xhash_table_foreach:
 * @hash_table: a #xhashtable_t
 * @func: the function to call for each key/value pair
 * @user_data: user data to pass to the function
 *
 * Calls the given function for each of the key/value pairs in the
 * #xhashtable_t.  The function is passed the key and value of each
 * pair, and the given @user_data parameter.  The hash table may not
 * be modified while iterating over it (you can't add/remove
 * items). To remove all items matching a predicate, use
 * xhash_table_foreach_remove().
 *
 * The order in which xhash_table_foreach() iterates over the keys/values in
 * the hash table is not defined.
 *
 * See xhash_table_find() for performance caveats for linear
 * order searches in contrast to xhash_table_lookup().
 */
void
xhash_table_foreach (xhashtable_t *hash_table,
                      GHFunc      func,
                      xpointer_t    user_data)
{
  xsize_t i;
#ifndef G_DISABLE_ASSERT
  xint_t version;
#endif

  g_return_if_fail (hash_table != NULL);
  g_return_if_fail (func != NULL);

#ifndef G_DISABLE_ASSERT
  version = hash_table->version;
#endif

  for (i = 0; i < hash_table->size; i++)
    {
      xuint_t node_hash = hash_table->hashes[i];
      xpointer_t node_key = xhash_table_fetch_key_or_value (hash_table->keys, i, hash_table->have_big_keys);
      xpointer_t node_value = xhash_table_fetch_key_or_value (hash_table->values, i, hash_table->have_bixvalues);

      if (HASH_IS_REAL (node_hash))
        (* func) (node_key, node_value, user_data);

#ifndef G_DISABLE_ASSERT
      g_return_if_fail (version == hash_table->version);
#endif
    }
}

/**
 * xhash_table_find:
 * @hash_table: a #xhashtable_t
 * @predicate: function to test the key/value pairs for a certain property
 * @user_data: user data to pass to the function
 *
 * Calls the given function for key/value pairs in the #xhashtable_t
 * until @predicate returns %TRUE. The function is passed the key
 * and value of each pair, and the given @user_data parameter. The
 * hash table may not be modified while iterating over it (you can't
 * add/remove items).
 *
 * Note, that hash tables are really only optimized for forward
 * lookups, i.e. xhash_table_lookup(). So code that frequently issues
 * xhash_table_find() or xhash_table_foreach() (e.g. in the order of
 * once per every entry in a hash table) should probably be reworked
 * to use additional or different data structures for reverse lookups
 * (keep in mind that an O(n) find/foreach operation issued for all n
 * values in a hash table ends up needing O(n*n) operations).
 *
 * Returns: (nullable): The value of the first key/value pair is returned,
 *     for which @predicate evaluates to %TRUE. If no pair with the
 *     requested property is found, %NULL is returned.
 *
 * Since: 2.4
 */
xpointer_t
xhash_table_find (xhashtable_t *hash_table,
                   GHRFunc     predicate,
                   xpointer_t    user_data)
{
  xsize_t i;
#ifndef G_DISABLE_ASSERT
  xint_t version;
#endif
  xboolean_t match;

  xreturn_val_if_fail (hash_table != NULL, NULL);
  xreturn_val_if_fail (predicate != NULL, NULL);

#ifndef G_DISABLE_ASSERT
  version = hash_table->version;
#endif

  match = FALSE;

  for (i = 0; i < hash_table->size; i++)
    {
      xuint_t node_hash = hash_table->hashes[i];
      xpointer_t node_key = xhash_table_fetch_key_or_value (hash_table->keys, i, hash_table->have_big_keys);
      xpointer_t node_value = xhash_table_fetch_key_or_value (hash_table->values, i, hash_table->have_bixvalues);

      if (HASH_IS_REAL (node_hash))
        match = predicate (node_key, node_value, user_data);

#ifndef G_DISABLE_ASSERT
      xreturn_val_if_fail (version == hash_table->version, NULL);
#endif

      if (match)
        return node_value;
    }

  return NULL;
}

/**
 * xhash_table_size:
 * @hash_table: a #xhashtable_t
 *
 * Returns the number of elements contained in the #xhashtable_t.
 *
 * Returns: the number of key/value pairs in the #xhashtable_t.
 */
xuint_t
xhash_table_size (xhashtable_t *hash_table)
{
  xreturn_val_if_fail (hash_table != NULL, 0);

  return hash_table->nnodes;
}

/**
 * xhash_table_get_keys:
 * @hash_table: a #xhashtable_t
 *
 * Retrieves every key inside @hash_table. The returned data is valid
 * until changes to the hash release those keys.
 *
 * This iterates over every entry in the hash table to build its return value.
 * To iterate over the entries in a #xhashtable_t more efficiently, use a
 * #xhash_table_iter_t.
 *
 * Returns: (transfer container): a #xlist_t containing all the keys
 *     inside the hash table. The content of the list is owned by the
 *     hash table and should not be modified or freed. Use xlist_free()
 *     when done using the list.
 *
 * Since: 2.14
 */
xlist_t *
xhash_table_get_keys (xhashtable_t *hash_table)
{
  xsize_t i;
  xlist_t *retval;

  xreturn_val_if_fail (hash_table != NULL, NULL);

  retval = NULL;
  for (i = 0; i < hash_table->size; i++)
    {
      if (HASH_IS_REAL (hash_table->hashes[i]))
        retval = xlist_prepend (retval, xhash_table_fetch_key_or_value (hash_table->keys, i, hash_table->have_big_keys));
    }

  return retval;
}

/**
 * xhash_table_get_keys_as_array:
 * @hash_table: a #xhashtable_t
 * @length: (out) (optional): the length of the returned array
 *
 * Retrieves every key inside @hash_table, as an array.
 *
 * The returned array is %NULL-terminated but may contain %NULL as a
 * key.  Use @length to determine the true length if it's possible that
 * %NULL was used as the value for a key.
 *
 * Note: in the common case of a string-keyed #xhashtable_t, the return
 * value of this function can be conveniently cast to (const xchar_t **).
 *
 * This iterates over every entry in the hash table to build its return value.
 * To iterate over the entries in a #xhashtable_t more efficiently, use a
 * #xhash_table_iter_t.
 *
 * You should always free the return result with g_free().  In the
 * above-mentioned case of a string-keyed hash table, it may be
 * appropriate to use xstrfreev() if you call xhash_table_steal_all()
 * first to transfer ownership of the keys.
 *
 * Returns: (array length=length) (transfer container): a
 *   %NULL-terminated array containing each key from the table.
 *
 * Since: 2.40
 **/
xpointer_t *
xhash_table_get_keys_as_array (xhashtable_t *hash_table,
                                xuint_t      *length)
{
  xpointer_t *result;
  xsize_t i, j = 0;

  result = g_new (xpointer_t, hash_table->nnodes + 1);
  for (i = 0; i < hash_table->size; i++)
    {
      if (HASH_IS_REAL (hash_table->hashes[i]))
        result[j++] = xhash_table_fetch_key_or_value (hash_table->keys, i, hash_table->have_big_keys);
    }
  g_assert_cmpint (j, ==, hash_table->nnodes);
  result[j] = NULL;

  if (length)
    *length = j;

  return result;
}

/**
 * xhash_table_get_values:
 * @hash_table: a #xhashtable_t
 *
 * Retrieves every value inside @hash_table. The returned data
 * is valid until @hash_table is modified.
 *
 * This iterates over every entry in the hash table to build its return value.
 * To iterate over the entries in a #xhashtable_t more efficiently, use a
 * #xhash_table_iter_t.
 *
 * Returns: (transfer container): a #xlist_t containing all the values
 *     inside the hash table. The content of the list is owned by the
 *     hash table and should not be modified or freed. Use xlist_free()
 *     when done using the list.
 *
 * Since: 2.14
 */
xlist_t *
xhash_table_get_values (xhashtable_t *hash_table)
{
  xsize_t i;
  xlist_t *retval;

  xreturn_val_if_fail (hash_table != NULL, NULL);

  retval = NULL;
  for (i = 0; i < hash_table->size; i++)
    {
      if (HASH_IS_REAL (hash_table->hashes[i]))
        retval = xlist_prepend (retval, xhash_table_fetch_key_or_value (hash_table->values, i, hash_table->have_bixvalues));
    }

  return retval;
}

/* Hash functions.
 */

/**
 * xstr_equal:
 * @v1: (not nullable): a key
 * @v2: (not nullable): a key to compare with @v1
 *
 * Compares two strings for byte-by-byte equality and returns %TRUE
 * if they are equal. It can be passed to xhash_table_new() as the
 * @key_equal_func parameter, when using non-%NULL strings as keys in a
 * #xhashtable_t.
 *
 * This function is typically used for hash table comparisons, but can be used
 * for general purpose comparisons of non-%NULL strings. For a %NULL-safe string
 * comparison function, see xstrcmp0().
 *
 * Returns: %TRUE if the two keys match
 */
xboolean_t
xstr_equal (xconstpointer v1,
             xconstpointer v2)
{
  const xchar_t *string1 = v1;
  const xchar_t *string2 = v2;

  return strcmp (string1, string2) == 0;
}

/**
 * xstr_hash:
 * @v: (not nullable): a string key
 *
 * Converts a string to a hash value.
 *
 * This function implements the widely used "djb" hash apparently
 * posted by Daniel Bernstein to comp.lang.c some time ago.  The 32
 * bit unsigned hash value starts at 5381 and for each byte 'c' in
 * the string, is updated: `hash = hash * 33 + c`. This function
 * uses the signed value of each byte.
 *
 * It can be passed to xhash_table_new() as the @hash_func parameter,
 * when using non-%NULL strings as keys in a #xhashtable_t.
 *
 * Note that this function may not be a perfect fit for all use cases.
 * For example, it produces some hash collisions with strings as short
 * as 2.
 *
 * Returns: a hash value corresponding to the key
 */
xuint_t
xstr_hash (xconstpointer v)
{
  const signed char *p;
  xuint32_t h = 5381;

  for (p = v; *p != '\0'; p++)
    h = (h << 5) + h + *p;

  return h;
}

/**
 * g_direct_hash:
 * @v: (nullable): a #xpointer_t key
 *
 * Converts a xpointer_t to a hash value.
 * It can be passed to xhash_table_new() as the @hash_func parameter,
 * when using opaque pointers compared by pointer value as keys in a
 * #xhashtable_t.
 *
 * This hash function is also appropriate for keys that are integers
 * stored in pointers, such as `GINT_TO_POINTER (n)`.
 *
 * Returns: a hash value corresponding to the key.
 */
xuint_t
g_direct_hash (xconstpointer v)
{
  return GPOINTER_TO_UINT (v);
}

/**
 * g_direct_equal:
 * @v1: (nullable): a key
 * @v2: (nullable): a key to compare with @v1
 *
 * Compares two #xpointer_t arguments and returns %TRUE if they are equal.
 * It can be passed to xhash_table_new() as the @key_equal_func
 * parameter, when using opaque pointers compared by pointer value as
 * keys in a #xhashtable_t.
 *
 * This equality function is also appropriate for keys that are integers
 * stored in pointers, such as `GINT_TO_POINTER (n)`.
 *
 * Returns: %TRUE if the two keys match.
 */
xboolean_t
g_direct_equal (xconstpointer v1,
                xconstpointer v2)
{
  return v1 == v2;
}

/**
 * g_int_equal:
 * @v1: (not nullable): a pointer to a #xint_t key
 * @v2: (not nullable): a pointer to a #xint_t key to compare with @v1
 *
 * Compares the two #xint_t values being pointed to and returns
 * %TRUE if they are equal.
 * It can be passed to xhash_table_new() as the @key_equal_func
 * parameter, when using non-%NULL pointers to integers as keys in a
 * #xhashtable_t.
 *
 * Note that this function acts on pointers to #xint_t, not on #xint_t
 * directly: if your hash table's keys are of the form
 * `GINT_TO_POINTER (n)`, use g_direct_equal() instead.
 *
 * Returns: %TRUE if the two keys match.
 */
xboolean_t
g_int_equal (xconstpointer v1,
             xconstpointer v2)
{
  return *((const xint_t*) v1) == *((const xint_t*) v2);
}

/**
 * g_int_hash:
 * @v: (not nullable): a pointer to a #xint_t key
 *
 * Converts a pointer to a #xint_t to a hash value.
 * It can be passed to xhash_table_new() as the @hash_func parameter,
 * when using non-%NULL pointers to integer values as keys in a #xhashtable_t.
 *
 * Note that this function acts on pointers to #xint_t, not on #xint_t
 * directly: if your hash table's keys are of the form
 * `GINT_TO_POINTER (n)`, use g_direct_hash() instead.
 *
 * Returns: a hash value corresponding to the key.
 */
xuint_t
g_int_hash (xconstpointer v)
{
  return *(const xint_t*) v;
}

/**
 * g_int64_equal:
 * @v1: (not nullable): a pointer to a #sint64_t key
 * @v2: (not nullable): a pointer to a #sint64_t key to compare with @v1
 *
 * Compares the two #sint64_t values being pointed to and returns
 * %TRUE if they are equal.
 * It can be passed to xhash_table_new() as the @key_equal_func
 * parameter, when using non-%NULL pointers to 64-bit integers as keys in a
 * #xhashtable_t.
 *
 * Returns: %TRUE if the two keys match.
 *
 * Since: 2.22
 */
xboolean_t
g_int64_equal (xconstpointer v1,
               xconstpointer v2)
{
  return *((const sint64_t*) v1) == *((const sint64_t*) v2);
}

/**
 * g_int64_hash:
 * @v: (not nullable): a pointer to a #sint64_t key
 *
 * Converts a pointer to a #sint64_t to a hash value.
 *
 * It can be passed to xhash_table_new() as the @hash_func parameter,
 * when using non-%NULL pointers to 64-bit integer values as keys in a
 * #xhashtable_t.
 *
 * Returns: a hash value corresponding to the key.
 *
 * Since: 2.22
 */
xuint_t
g_int64_hash (xconstpointer v)
{
  return (xuint_t) *(const sint64_t*) v;
}

/**
 * g_double_equal:
 * @v1: (not nullable): a pointer to a #xdouble_t key
 * @v2: (not nullable): a pointer to a #xdouble_t key to compare with @v1
 *
 * Compares the two #xdouble_t values being pointed to and returns
 * %TRUE if they are equal.
 * It can be passed to xhash_table_new() as the @key_equal_func
 * parameter, when using non-%NULL pointers to doubles as keys in a
 * #xhashtable_t.
 *
 * Returns: %TRUE if the two keys match.
 *
 * Since: 2.22
 */
xboolean_t
g_double_equal (xconstpointer v1,
                xconstpointer v2)
{
  return *((const xdouble_t*) v1) == *((const xdouble_t*) v2);
}

/**
 * g_double_hash:
 * @v: (not nullable): a pointer to a #xdouble_t key
 *
 * Converts a pointer to a #xdouble_t to a hash value.
 * It can be passed to xhash_table_new() as the @hash_func parameter,
 * It can be passed to xhash_table_new() as the @hash_func parameter,
 * when using non-%NULL pointers to doubles as keys in a #xhashtable_t.
 *
 * Returns: a hash value corresponding to the key.
 *
 * Since: 2.22
 */
xuint_t
g_double_hash (xconstpointer v)
{
  return (xuint_t) *(const xdouble_t*) v;
}
