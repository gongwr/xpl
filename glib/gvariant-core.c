/*
 * Copyright © 2007, 2008 Ryan Lortie
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
 */

#include "config.h"

#include <glib/gvariant-core.h>

#include <glib/gvariant-internal.h>
#include <glib/gvariant-serialiser.h>
#include <glib/gtestutils.h>
#include <glib/gbitlock.h>
#include <glib/gatomic.h>
#include <glib/gbytes.h>
#include <glib/gslice.h>
#include <glib/gmem.h>
#include <glib/grefcount.h>
#include <string.h>


/*
 * This file includes the structure definition for xvariant_t and a small
 * set of functions that are allowed to access the structure directly.
 *
 * This minimises the amount of code that can possibly touch a xvariant_t
 * structure directly to a few simple fundamental operations.  These few
 * operations are written to be completely threadsafe with respect to
 * all possible outside access.  This means that we only need to be
 * concerned about thread safety issues in this one small file.
 *
 * Most xvariant_t API functions are in gvariant.c.
 */

/**
 * xvariant_t:
 *
 * #xvariant_t is an opaque data structure and can only be accessed
 * using the following functions.
 *
 * Since: 2.24
 **/
struct _xvariant
/* see below for field member documentation */
{
  GVariantTypeInfo *type_info;
  xsize_t size;

  union
  {
    struct
    {
      xbytes_t *bytes;
      xconstpointer data;
    } serialised;

    struct
    {
      xvariant_t **children;
      xsize_t n_children;
    } tree;
  } contents;

  xint_t state;
  gatomicrefcount ref_count;
  xsize_t depth;
};

/* struct xvariant_t:
 *
 * There are two primary forms of xvariant_t instances: "serialized form"
 * and "tree form".
 *
 * "serialized form": A serialized xvariant_t instance stores its value in
 *                    the xvariant_t serialization format.  All
 *                    basic-typed instances (ie: non-containers) are in
 *                    serialized format, as are some containers.
 *
 * "tree form": Some containers are in "tree form".  In this case,
 *              instead of containing the serialized data for the
 *              container, the instance contains an array of pointers to
 *              the child values of the container (thus forming a tree).
 *
 * It is possible for an instance to transition from tree form to
 * serialized form.  This happens, implicitly, if the serialized data is
 * requested (eg: via xvariant_get_data()).  Serialized form instances
 * never transition into tree form.
 *
 *
 * The fields of the structure are documented here:
 *
 * type_info: this is a reference to a GVariantTypeInfo describing the
 *            type of the instance.  When the instance is freed, this
 *            reference must be released with xvariant_type_info_unref().
 *
 *            The type_info field never changes during the life of the
 *            instance, so it can be accessed without a lock.
 *
 * size: this is the size of the serialized form for the instance, if it
 *       is known.  If the instance is in serialized form then it is, by
 *       definition, known.  If the instance is in tree form then it may
 *       be unknown (in which case it is -1).  It is possible for the
 *       size to be known when in tree form if, for example, the user
 *       has called xvariant_get_size() without calling
 *       xvariant_get_data().  Additionally, even when the user calls
 *       xvariant_get_data() the size of the data must first be
 *       determined so that a large enough buffer can be allocated for
 *       the data.
 *
 *       Once the size is known, it can never become unknown again.
 *       xvariant_ensure_size() is used to ensure that the size is in
 *       the known state -- it calculates the size if needed.  After
 *       that, the size field can be accessed without a lock.
 *
 * contents: a union containing either the information associated with
 *           holding a value in serialized form or holding a value in
 *           tree form.
 *
 *   .serialised: Only valid when the instance is in serialized form.
 *
 *                Since an instance can never transition away from
 *                serialized form, once these fields are set, they will
 *                never be changed.  It is therefore valid to access
 *                them without holding a lock.
 *
 *     .bytes:  the #xbytes_t that contains the memory pointed to by
 *              .data, or %NULL if .data is %NULL.  In the event that
 *              the instance was deserialized from another instance,
 *              then the bytes will be shared by both of them.  When
 *              the instance is freed, this reference must be released
 *              with xbytes_unref().
 *
 *     .data: the serialized data (of size 'size') of the instance.
 *            This pointer should not be freed or modified in any way.
 *            #xbytes_t is responsible for memory management.
 *
 *            This pointer may be %NULL in two cases:
 *
 *              - if the serialized size of the instance is 0
 *
 *              - if the instance is of a fixed-sized type and was
 *                deserialized out of a corrupted container such that
 *                the container contains too few bytes to point to the
 *                entire proper fixed-size of this instance.  In this
 *                case, 'size' will still be equal to the proper fixed
 *                size, but this pointer will be %NULL.  This is exactly
 *                the reason that xvariant_get_data() sometimes returns
 *                %NULL.  For all other calls, the effect should be as
 *                if .data pointed to the appropriate number of nul
 *                bytes.
 *
 *   .tree: Only valid when the instance is in tree form.
 *
 *          Note that accesses from other threads could result in
 *          conversion of the instance from tree form to serialized form
 *          at any time.  For this reason, the instance lock must always
 *          be held while performing any operations on 'contents.tree'.
 *
 *     .children: the array of the child instances of this instance.
 *                When the instance is freed (or converted to serialized
 *                form) then each child must have xvariant_unref()
 *                called on it and the array must be freed using
 *                g_free().
 *
 *     .n_children: the number of items in the .children array.
 *
 * state: a bitfield describing the state of the instance.  It is a
 *        bitwise-or of the following STATE_* constants:
 *
 *    STATE_LOCKED: the instance lock is held.  This is the bit used by
 *                  g_bit_lock().
 *
 *    STATE_SERIALISED: the instance is in serialized form.  If this
 *                      flag is not set then the instance is in tree
 *                      form.
 *
 *    STATE_TRUSTED: for serialized form instances, this means that the
 *                   serialized data is known to be in normal form (ie:
 *                   not corrupted).
 *
 *                   For tree form instances, this means that all of the
 *                   child instances in the contents.tree.children array
 *                   are trusted.  This means that if the container is
 *                   serialized then the resulting data will be in
 *                   normal form.
 *
 *                   If this flag is unset it does not imply that the
 *                   data is corrupted.  It merely means that we're not
 *                   sure that it's valid.  See xvariant_is_trusted().
 *
 *    STATE_FLOATING: if this flag is set then the object has a floating
 *                    reference.  See xvariant_ref_sink().
 *
 * ref_count: the reference count of the instance
 *
 * depth: the depth of the xvariant_t in a hierarchy of nested containers,
 *        increasing with the level of nesting. The top-most xvariant_t has depth
 *        zero.  This is used to avoid recursing too deeply and overflowing the
 *        stack when handling deeply nested untrusted serialized GVariants.
 */
#define STATE_LOCKED     1
#define STATE_SERIALISED 2
#define STATE_TRUSTED    4
#define STATE_FLOATING   8

/* -- private -- */
/* < private >
 * xvariant_lock:
 * @value: a #xvariant_t
 *
 * Locks @value for performing sensitive operations.
 */
static void
xvariant_lock (xvariant_t *value)
{
  g_bit_lock (&value->state, 0);
}

/* < private >
 * xvariant_unlock:
 * @value: a #xvariant_t
 *
 * Unlocks @value after performing sensitive operations.
 */
static void
xvariant_unlock (xvariant_t *value)
{
  g_bit_unlock (&value->state, 0);
}

/* < private >
 * xvariant_release_children:
 * @value: a #xvariant_t
 *
 * Releases the reference held on each child in the 'children' array of
 * @value and frees the array itself.  @value must be in tree form.
 *
 * This is done when freeing a tree-form instance or converting it to
 * serialized form.
 *
 * The current thread must hold the lock on @value.
 */
static void
xvariant_release_children (xvariant_t *value)
{
  xsize_t i;

  g_assert (value->state & STATE_LOCKED);
  g_assert (~value->state & STATE_SERIALISED);

  for (i = 0; i < value->contents.tree.n_children; i++)
    xvariant_unref (value->contents.tree.children[i]);

  g_free (value->contents.tree.children);
}

/* This begins the main body of the recursive serializer.
 *
 * There are 3 functions here that work as a team with the serializer to
 * get things done.  xvariant_store() has a trivial role, but as a
 * public API function, it has its definition elsewhere.
 *
 * Note that "serialization" of an instance does not mean that the
 * instance is converted to serialized form -- it means that the
 * serialized form of an instance is written to an external buffer.
 * xvariant_ensure_serialised() (which is not part of this set of
 * functions) is the function that is responsible for converting an
 * instance to serialized form.
 *
 * We are only concerned here with container types since non-container
 * instances are always in serialized form.  For these instances,
 * storing their serialized form merely involves a memcpy().
 *
 * Serialization is a two-step process.  First, the size of the
 * serialized data must be calculated so that an appropriately-sized
 * buffer can be allocated.  Second, the data is written into the
 * buffer.
 *
 * Determining the size:
 *   The process of determining the size is triggered by a call to
 *   xvariant_ensure_size() on a container.  This invokes the
 *   serializer code to determine the size.  The serializer is passed
 *   xvariant_fill_gvs() as a callback.
 *
 *   xvariant_fill_gvs() is called by the serializer on each child of
 *   the container which, in turn, calls xvariant_ensure_size() on
 *   itself and fills in the result of its own size calculation.
 *
 *   The serializer uses the size information from the children to
 *   calculate the size needed for the entire container.
 *
 * Writing the data:
 *   After the buffer has been allocated, xvariant_serialise() is
 *   called on the container.  This invokes the serializer code to write
 *   the bytes to the container.  The serializer is, again, passed
 *   xvariant_fill_gvs() as a callback.
 *
 *   This time, when xvariant_fill_gvs() is called for each child, the
 *   child is given a pointer to a sub-region of the allocated buffer
 *   where it should write its data.  This is done by calling
 *   xvariant_store().  In the event that the instance is in serialized
 *   form this means a memcpy() of the serialized data into the
 *   allocated buffer.  In the event that the instance is in tree form
 *   this means a recursive call back into xvariant_serialise().
 *
 *
 * The forward declaration here allows corecursion via callback:
 */
static void xvariant_fill_gvs (GVariantSerialised *, xpointer_t);

/* < private >
 * xvariant_ensure_size:
 * @value: a #xvariant_t
 *
 * Ensures that the ->size field of @value is filled in properly.  This
 * must be done as a precursor to any serialization of the value in
 * order to know how large of a buffer is needed to store the data.
 *
 * The current thread must hold the lock on @value.
 */
static void
xvariant_ensure_size (xvariant_t *value)
{
  g_assert (value->state & STATE_LOCKED);

  if (value->size == (xsize_t) -1)
    {
      xpointer_t *children;
      xsize_t n_children;

      children = (xpointer_t *) value->contents.tree.children;
      n_children = value->contents.tree.n_children;
      value->size = xvariant_serialiser_needed_size (value->type_info,
                                                      xvariant_fill_gvs,
                                                      children, n_children);
    }
}

/* < private >
 * xvariant_serialise:
 * @value: a #xvariant_t
 * @data: an appropriately-sized buffer
 *
 * Serializes @value into @data.  @value must be in tree form.
 *
 * No change is made to @value.
 *
 * The current thread must hold the lock on @value.
 */
static void
xvariant_serialise (xvariant_t *value,
                     xpointer_t  data)
{
  GVariantSerialised serialised = { 0, };
  xpointer_t *children;
  xsize_t n_children;

  g_assert (~value->state & STATE_SERIALISED);
  g_assert (value->state & STATE_LOCKED);

  serialised.type_info = value->type_info;
  serialised.size = value->size;
  serialised.data = data;
  serialised.depth = value->depth;

  children = (xpointer_t *) value->contents.tree.children;
  n_children = value->contents.tree.n_children;

  xvariant_serialiser_serialise (serialised, xvariant_fill_gvs,
                                  children, n_children);
}

/* < private >
 * xvariant_fill_gvs:
 * @serialised: a pointer to a #GVariantSerialised
 * @data: a #xvariant_t instance
 *
 * This is the callback that is passed by a tree-form container instance
 * to the serializer.  This callback gets called on each child of the
 * container.  Each child is responsible for performing the following
 * actions:
 *
 *  - reporting its type
 *
 *  - reporting its serialized size (requires knowing the size first)
 *
 *  - possibly storing its serialized form into the provided buffer
 */
static void
xvariant_fill_gvs (GVariantSerialised *serialised,
                    xpointer_t            data)
{
  xvariant_t *value = data;

  xvariant_lock (value);
  xvariant_ensure_size (value);
  xvariant_unlock (value);

  if (serialised->type_info == NULL)
    serialised->type_info = value->type_info;
  g_assert (serialised->type_info == value->type_info);

  if (serialised->size == 0)
    serialised->size = value->size;
  g_assert (serialised->size == value->size);
  serialised->depth = value->depth;

  if (serialised->data)
    /* xvariant_store() is a public API, so it
     * it will reacquire the lock if it needs to.
     */
    xvariant_store (value, serialised->data);
}

/* this ends the main body of the recursive serializer */

/* < private >
 * xvariant_ensure_serialised:
 * @value: a #xvariant_t
 *
 * Ensures that @value is in serialized form.
 *
 * If @value is in tree form then this function ensures that the
 * serialized size is known and then allocates a buffer of that size and
 * serializes the instance into the buffer.  The 'children' array is
 * then released and the instance is set to serialized form based on the
 * contents of the buffer.
 *
 * The current thread must hold the lock on @value.
 */
static void
xvariant_ensure_serialised (xvariant_t *value)
{
  g_assert (value->state & STATE_LOCKED);

  if (~value->state & STATE_SERIALISED)
    {
      xbytes_t *bytes;
      xpointer_t data;

      xvariant_ensure_size (value);
      data = g_malloc (value->size);
      xvariant_serialise (value, data);

      xvariant_release_children (value);

      bytes = xbytes_new_take (data, value->size);
      value->contents.serialised.data = xbytes_get_data (bytes, NULL);
      value->contents.serialised.bytes = bytes;
      value->state |= STATE_SERIALISED;
    }
}

/* < private >
 * xvariant_alloc:
 * @type: the type of the new instance
 * @serialised: if the instance will be in serialised form
 * @trusted: if the instance will be trusted
 *
 * Allocates a #xvariant_t instance and does some common work (such as
 * looking up and filling in the type info), setting the state field,
 * and setting the ref_count to 1.
 *
 * Returns: a new #xvariant_t with a floating reference
 */
static xvariant_t *
xvariant_alloc (const xvariant_type_t *type,
                 xboolean_t            serialised,
                 xboolean_t            trusted)
{
  xvariant_t *value;

  value = g_slice_new (xvariant_t);
  value->type_info = xvariant_type_info_get (type);
  value->state = (serialised ? STATE_SERIALISED : 0) |
                 (trusted ? STATE_TRUSTED : 0) |
                 STATE_FLOATING;
  value->size = (xssize_t) -1;
  g_atomic_ref_count_init (&value->ref_count);
  value->depth = 0;

  return value;
}

/**
 * xvariant_new_from_bytes:
 * @type: a #xvariant_type_t
 * @bytes: a #xbytes_t
 * @trusted: if the contents of @bytes are trusted
 *
 * Constructs a new serialized-mode #xvariant_t instance.  This is the
 * inner interface for creation of new serialized values that gets
 * called from various functions in gvariant.c.
 *
 * A reference is taken on @bytes.
 *
 * The data in @bytes must be aligned appropriately for the @type being loaded.
 * Otherwise this function will internally create a copy of the memory (since
 * GLib 2.60) or (in older versions) fail and exit the process.
 *
 * Returns: (transfer none): a new #xvariant_t with a floating reference
 *
 * Since: 2.36
 */
xvariant_t *
xvariant_new_from_bytes (const xvariant_type_t *type,
                          xbytes_t             *bytes,
                          xboolean_t            trusted)
{
  xvariant_t *value;
  xuint_t alignment;
  xsize_t size;
  xbytes_t *owned_bytes = NULL;
  GVariantSerialised serialised;

  value = xvariant_alloc (type, TRUE, trusted);

  xvariant_type_info_query (value->type_info,
                             &alignment, &size);

  /* Ensure the alignment is correct. This is a huge performance hit if it’s
   * not correct, but that’s better than aborting if a caller provides data
   * with the wrong alignment (which is likely to happen very occasionally, and
   * only cause an abort on some architectures — so is unlikely to be caught
   * in testing). Callers can always actively ensure they use the correct
   * alignment to avoid the performance hit. */
  serialised.type_info = value->type_info;
  serialised.data = (guchar *) xbytes_get_data (bytes, &serialised.size);
  serialised.depth = 0;

  if (!xvariant_serialised_check (serialised))
    {
#ifdef HAVE_POSIX_MEMALIGN
      xpointer_t aligned_data = NULL;
      xsize_t aligned_size = xbytes_get_size (bytes);

      /* posix_memalign() requires the alignment to be a multiple of
       * sizeof(void*), and a power of 2. See xvariant_type_info_query() for
       * details on the alignment format. */
      if (posix_memalign (&aligned_data, MAX (sizeof (void *), alignment + 1),
                          aligned_size) != 0)
        xerror ("posix_memalign failed");

      if (aligned_size != 0)
        memcpy (aligned_data, xbytes_get_data (bytes, NULL), aligned_size);

      bytes = owned_bytes = xbytes_new_with_free_func (aligned_data,
                                                        aligned_size,
                                                        free, aligned_data);
      aligned_data = NULL;
#else
      /* NOTE: there may be platforms that lack posix_memalign() and also
       * have malloc() that returns non-8-aligned.  if so, we need to try
       * harder here.
       */
      bytes = owned_bytes = xbytes_new (xbytes_get_data (bytes, NULL),
                                         xbytes_get_size (bytes));
#endif
    }

  value->contents.serialised.bytes = xbytes_ref (bytes);

  if (size && xbytes_get_size (bytes) != size)
    {
      /* Creating a fixed-sized xvariant_t with a bytes of the wrong
       * size.
       *
       * We should do the equivalent of pulling a fixed-sized child out
       * of a brozen container (ie: data is NULL size is equal to the correct
       * fixed size).
       */
      value->contents.serialised.data = NULL;
      value->size = size;
    }
  else
    {
      value->contents.serialised.data = xbytes_get_data (bytes, &value->size);
    }

  g_clear_pointer (&owned_bytes, xbytes_unref);

  return value;
}

/* -- internal -- */

/* < internal >
 * xvariant_new_from_children:
 * @type: a #xvariant_type_t
 * @children: an array of #xvariant_t pointers.  Consumed.
 * @n_children: the length of @children
 * @trusted: %TRUE if every child in @children in trusted
 *
 * Constructs a new tree-mode #xvariant_t instance.  This is the inner
 * interface for creation of new serialized values that gets called from
 * various functions in gvariant.c.
 *
 * @children is consumed by this function.  g_free() will be called on
 * it some time later.
 *
 * Returns: a new #xvariant_t with a floating reference
 */
xvariant_t *
xvariant_new_from_children (const xvariant_type_t  *type,
                             xvariant_t           **children,
                             xsize_t                n_children,
                             xboolean_t             trusted)
{
  xvariant_t *value;

  value = xvariant_alloc (type, FALSE, trusted);
  value->contents.tree.children = children;
  value->contents.tree.n_children = n_children;

  return value;
}

/* < internal >
 * xvariant_get_type_info:
 * @value: a #xvariant_t
 *
 * Returns the #GVariantTypeInfo corresponding to the type of @value.  A
 * reference is not added, so the return value is only good for the
 * duration of the life of @value.
 *
 * Returns: the #GVariantTypeInfo for @value
 */
GVariantTypeInfo *
xvariant_get_type_info (xvariant_t *value)
{
  return value->type_info;
}

/* < internal >
 * xvariant_is_trusted:
 * @value: a #xvariant_t
 *
 * Determines if @value is trusted by #xvariant_t to contain only
 * fully-valid data.  All values constructed solely via #xvariant_t APIs
 * are trusted, but values containing data read in from other sources
 * are usually not trusted.
 *
 * The main advantage of trusted data is that certain checks can be
 * skipped.  For example, we don't need to check that a string is
 * properly nul-terminated or that an object path is actually a
 * properly-formatted object path.
 *
 * Returns: if @value is trusted
 */
xboolean_t
xvariant_is_trusted (xvariant_t *value)
{
  return (value->state & STATE_TRUSTED) != 0;
}

/* < internal >
 * xvariant_get_depth:
 * @value: a #xvariant_t
 *
 * Gets the nesting depth of a #xvariant_t. This is 0 for a #xvariant_t with no
 * children.
 *
 * Returns: nesting depth of @value
 */
xsize_t
xvariant_get_depth (xvariant_t *value)
{
  return value->depth;
}

/* -- public -- */

/**
 * xvariant_unref:
 * @value: a #xvariant_t
 *
 * Decreases the reference count of @value.  When its reference count
 * drops to 0, the memory used by the variant is freed.
 *
 * Since: 2.24
 **/
void
xvariant_unref (xvariant_t *value)
{
  g_return_if_fail (value != NULL);

  if (g_atomic_ref_count_dec (&value->ref_count))
    {
      if G_UNLIKELY (value->state & STATE_LOCKED)
        g_critical ("attempting to free a locked xvariant_t instance.  "
                    "This should never happen.");

      value->state |= STATE_LOCKED;

      xvariant_type_info_unref (value->type_info);

      if (value->state & STATE_SERIALISED)
        xbytes_unref (value->contents.serialised.bytes);
      else
        xvariant_release_children (value);

      memset (value, 0, sizeof (xvariant_t));
      g_slice_free (xvariant_t, value);
    }
}

/**
 * xvariant_ref:
 * @value: a #xvariant_t
 *
 * Increases the reference count of @value.
 *
 * Returns: the same @value
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_ref (xvariant_t *value)
{
  g_return_val_if_fail (value != NULL, NULL);

  g_atomic_ref_count_inc (&value->ref_count);

  return value;
}

/**
 * xvariant_ref_sink:
 * @value: a #xvariant_t
 *
 * #xvariant_t uses a floating reference count system.  All functions with
 * names starting with `xvariant_new_` return floating
 * references.
 *
 * Calling xvariant_ref_sink() on a #xvariant_t with a floating reference
 * will convert the floating reference into a full reference.  Calling
 * xvariant_ref_sink() on a non-floating #xvariant_t results in an
 * additional normal reference being added.
 *
 * In other words, if the @value is floating, then this call "assumes
 * ownership" of the floating reference, converting it to a normal
 * reference.  If the @value is not floating, then this call adds a
 * new normal reference increasing the reference count by one.
 *
 * All calls that result in a #xvariant_t instance being inserted into a
 * container will call xvariant_ref_sink() on the instance.  This means
 * that if the value was just created (and has only its floating
 * reference) then the container will assume sole ownership of the value
 * at that point and the caller will not need to unreference it.  This
 * makes certain common styles of programming much easier while still
 * maintaining normal refcounting semantics in situations where values
 * are not floating.
 *
 * Returns: the same @value
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_ref_sink (xvariant_t *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (!g_atomic_ref_count_compare (&value->ref_count, 0), NULL);

  xvariant_lock (value);

  if (~value->state & STATE_FLOATING)
    xvariant_ref (value);
  else
    value->state &= ~STATE_FLOATING;

  xvariant_unlock (value);

  return value;
}

/**
 * xvariant_take_ref:
 * @value: a #xvariant_t
 *
 * If @value is floating, sink it.  Otherwise, do nothing.
 *
 * Typically you want to use xvariant_ref_sink() in order to
 * automatically do the correct thing with respect to floating or
 * non-floating references, but there is one specific scenario where
 * this function is helpful.
 *
 * The situation where this function is helpful is when creating an API
 * that allows the user to provide a callback function that returns a
 * #xvariant_t.  We certainly want to allow the user the flexibility to
 * return a non-floating reference from this callback (for the case
 * where the value that is being returned already exists).
 *
 * At the same time, the style of the #xvariant_t API makes it likely that
 * for newly-created #xvariant_t instances, the user can be saved some
 * typing if they are allowed to return a #xvariant_t with a floating
 * reference.
 *
 * Using this function on the return value of the user's callback allows
 * the user to do whichever is more convenient for them.  The caller
 * will always receives exactly one full reference to the value: either
 * the one that was returned in the first place, or a floating reference
 * that has been converted to a full reference.
 *
 * This function has an odd interaction when combined with
 * xvariant_ref_sink() running at the same time in another thread on
 * the same #xvariant_t instance.  If xvariant_ref_sink() runs first then
 * the result will be that the floating reference is converted to a hard
 * reference.  If xvariant_take_ref() runs first then the result will
 * be that the floating reference is converted to a hard reference and
 * an additional reference on top of that one is added.  It is best to
 * avoid this situation.
 *
 * Returns: the same @value
 **/
xvariant_t *
xvariant_take_ref (xvariant_t *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (!g_atomic_ref_count_compare (&value->ref_count, 0), NULL);

  g_atomic_int_and (&value->state, ~STATE_FLOATING);

  return value;
}

/**
 * xvariant_is_floating:
 * @value: a #xvariant_t
 *
 * Checks whether @value has a floating reference count.
 *
 * This function should only ever be used to assert that a given variant
 * is or is not floating, or for debug purposes. To acquire a reference
 * to a variant that might be floating, always use xvariant_ref_sink()
 * or xvariant_take_ref().
 *
 * See xvariant_ref_sink() for more information about floating reference
 * counts.
 *
 * Returns: whether @value is floating
 *
 * Since: 2.26
 **/
xboolean_t
xvariant_is_floating (xvariant_t *value)
{
  g_return_val_if_fail (value != NULL, FALSE);

  return (value->state & STATE_FLOATING) != 0;
}

/**
 * xvariant_get_size:
 * @value: a #xvariant_t instance
 *
 * Determines the number of bytes that would be required to store @value
 * with xvariant_store().
 *
 * If @value has a fixed-sized type then this function always returned
 * that fixed size.
 *
 * In the case that @value is already in serialized form or the size has
 * already been calculated (ie: this function has been called before)
 * then this function is O(1).  Otherwise, the size is calculated, an
 * operation which is approximately O(n) in the number of values
 * involved.
 *
 * Returns: the serialized size of @value
 *
 * Since: 2.24
 **/
xsize_t
xvariant_get_size (xvariant_t *value)
{
  xvariant_lock (value);
  xvariant_ensure_size (value);
  xvariant_unlock (value);

  return value->size;
}

/**
 * xvariant_get_data:
 * @value: a #xvariant_t instance
 *
 * Returns a pointer to the serialized form of a #xvariant_t instance.
 * The returned data may not be in fully-normalised form if read from an
 * untrusted source.  The returned data must not be freed; it remains
 * valid for as long as @value exists.
 *
 * If @value is a fixed-sized value that was deserialized from a
 * corrupted serialized container then %NULL may be returned.  In this
 * case, the proper thing to do is typically to use the appropriate
 * number of nul bytes in place of @value.  If @value is not fixed-sized
 * then %NULL is never returned.
 *
 * In the case that @value is already in serialized form, this function
 * is O(1).  If the value is not already in serialized form,
 * serialization occurs implicitly and is approximately O(n) in the size
 * of the result.
 *
 * To deserialize the data returned by this function, in addition to the
 * serialized data, you must know the type of the #xvariant_t, and (if the
 * machine might be different) the endianness of the machine that stored
 * it. As a result, file formats or network messages that incorporate
 * serialized #GVariants must include this information either
 * implicitly (for instance "the file always contains a
 * %G_VARIANT_TYPE_VARIANT and it is always in little-endian order") or
 * explicitly (by storing the type and/or endianness in addition to the
 * serialized data).
 *
 * Returns: (transfer none): the serialized form of @value, or %NULL
 *
 * Since: 2.24
 **/
xconstpointer
xvariant_get_data (xvariant_t *value)
{
  xvariant_lock (value);
  xvariant_ensure_serialised (value);
  xvariant_unlock (value);

  return value->contents.serialised.data;
}

/**
 * xvariant_get_data_as_bytes:
 * @value: a #xvariant_t
 *
 * Returns a pointer to the serialized form of a #xvariant_t instance.
 * The semantics of this function are exactly the same as
 * xvariant_get_data(), except that the returned #xbytes_t holds
 * a reference to the variant data.
 *
 * Returns: (transfer full): A new #xbytes_t representing the variant data
 *
 * Since: 2.36
 */
xbytes_t *
xvariant_get_data_as_bytes (xvariant_t *value)
{
  const xchar_t *bytes_data;
  const xchar_t *data;
  xsize_t bytes_size;
  xsize_t size;

  xvariant_lock (value);
  xvariant_ensure_serialised (value);
  xvariant_unlock (value);

  bytes_data = xbytes_get_data (value->contents.serialised.bytes, &bytes_size);
  data = value->contents.serialised.data;
  size = value->size;

  if (data == NULL)
    {
      g_assert (size == 0);
      data = bytes_data;
    }

  if (data == bytes_data && size == bytes_size)
    return xbytes_ref (value->contents.serialised.bytes);
  else
    return xbytes_new_from_bytes (value->contents.serialised.bytes,
                                   data - bytes_data, size);
}


/**
 * xvariant_n_children:
 * @value: a container #xvariant_t
 *
 * Determines the number of children in a container #xvariant_t instance.
 * This includes variants, maybes, arrays, tuples and dictionary
 * entries.  It is an error to call this function on any other type of
 * #xvariant_t.
 *
 * For variants, the return value is always 1.  For values with maybe
 * types, it is always zero or one.  For arrays, it is the length of the
 * array.  For tuples it is the number of tuple items (which depends
 * only on the type).  For dictionary entries, it is always 2
 *
 * This function is O(1).
 *
 * Returns: the number of children in the container
 *
 * Since: 2.24
 **/
xsize_t
xvariant_n_children (xvariant_t *value)
{
  xsize_t n_children;

  xvariant_lock (value);

  if (value->state & STATE_SERIALISED)
    {
      GVariantSerialised serialised = {
        value->type_info,
        (xpointer_t) value->contents.serialised.data,
        value->size,
        value->depth,
      };

      n_children = xvariant_serialised_n_children (serialised);
    }
  else
    n_children = value->contents.tree.n_children;

  xvariant_unlock (value);

  return n_children;
}

/**
 * xvariant_get_child_value:
 * @value: a container #xvariant_t
 * @index_: the index of the child to fetch
 *
 * Reads a child item out of a container #xvariant_t instance.  This
 * includes variants, maybes, arrays, tuples and dictionary
 * entries.  It is an error to call this function on any other type of
 * #xvariant_t.
 *
 * It is an error if @index_ is greater than the number of child items
 * in the container.  See xvariant_n_children().
 *
 * The returned value is never floating.  You should free it with
 * xvariant_unref() when you're done with it.
 *
 * Note that values borrowed from the returned child are not guaranteed to
 * still be valid after the child is freed even if you still hold a reference
 * to @value, if @value has not been serialized at the time this function is
 * called. To avoid this, you can serialize @value by calling
 * xvariant_get_data() and optionally ignoring the return value.
 *
 * There may be implementation specific restrictions on deeply nested values,
 * which would result in the unit tuple being returned as the child value,
 * instead of further nested children. #xvariant_t is guaranteed to handle
 * nesting up to at least 64 levels.
 *
 * This function is O(1).
 *
 * Returns: (transfer full): the child at the specified index
 *
 * Since: 2.24
 **/
xvariant_t *
xvariant_get_child_value (xvariant_t *value,
                           xsize_t     index_)
{
  g_return_val_if_fail (index_ < xvariant_n_children (value), NULL);
  g_return_val_if_fail (value->depth < G_MAXSIZE, NULL);

  if (~g_atomic_int_get (&value->state) & STATE_SERIALISED)
    {
      xvariant_lock (value);

      if (~value->state & STATE_SERIALISED)
        {
          xvariant_t *child;

          child = xvariant_ref (value->contents.tree.children[index_]);
          xvariant_unlock (value);

          return child;
        }

      xvariant_unlock (value);
    }

  {
    GVariantSerialised serialised = {
      value->type_info,
      (xpointer_t) value->contents.serialised.data,
      value->size,
      value->depth,
    };
    GVariantSerialised s_child;
    xvariant_t *child;

    /* get the serializer to extract the serialized data for the child
     * from the serialized data for the container
     */
    s_child = xvariant_serialised_get_child (serialised, index_);

    /* Check whether this would cause nesting too deep. If so, return a fake
     * child. The only situation we expect this to happen in is with a variant,
     * as all other deeply-nested types have a static type, and hence should
     * have been rejected earlier. In the case of a variant whose nesting plus
     * the depth of its child is too great, return a unit variant () instead of
     * the real child. */
    if (!(value->state & STATE_TRUSTED) &&
        xvariant_type_info_query_depth (s_child.type_info) >=
        G_VARIANT_MAX_RECURSION_DEPTH - value->depth)
      {
        g_assert (xvariant_is_of_type (value, G_VARIANT_TYPE_VARIANT));
        return xvariant_new_tuple (NULL, 0);
      }

    /* create a new serialized instance out of it */
    child = g_slice_new (xvariant_t);
    child->type_info = s_child.type_info;
    child->state = (value->state & STATE_TRUSTED) |
                   STATE_SERIALISED;
    child->size = s_child.size;
    g_atomic_ref_count_init (&child->ref_count);
    child->depth = value->depth + 1;
    child->contents.serialised.bytes =
      xbytes_ref (value->contents.serialised.bytes);
    child->contents.serialised.data = s_child.data;

    return child;
  }
}

/**
 * xvariant_store:
 * @value: the #xvariant_t to store
 * @data: (not nullable): the location to store the serialized data at
 *
 * Stores the serialized form of @value at @data.  @data should be
 * large enough.  See xvariant_get_size().
 *
 * The stored data is in machine native byte order but may not be in
 * fully-normalised form if read from an untrusted source.  See
 * xvariant_get_normal_form() for a solution.
 *
 * As with xvariant_get_data(), to be able to deserialize the
 * serialized variant successfully, its type and (if the destination
 * machine might be different) its endianness must also be available.
 *
 * This function is approximately O(n) in the size of @data.
 *
 * Since: 2.24
 **/
void
xvariant_store (xvariant_t *value,
                 xpointer_t  data)
{
  xvariant_lock (value);

  if (value->state & STATE_SERIALISED)
    {
      if (value->contents.serialised.data != NULL)
        memcpy (data, value->contents.serialised.data, value->size);
      else
        memset (data, 0, value->size);
    }
  else
    xvariant_serialise (value, data);

  xvariant_unlock (value);
}

/**
 * xvariant_is_normal_form:
 * @value: a #xvariant_t instance
 *
 * Checks if @value is in normal form.
 *
 * The main reason to do this is to detect if a given chunk of
 * serialized data is in normal form: load the data into a #xvariant_t
 * using xvariant_new_from_data() and then use this function to
 * check.
 *
 * If @value is found to be in normal form then it will be marked as
 * being trusted.  If the value was already marked as being trusted then
 * this function will immediately return %TRUE.
 *
 * There may be implementation specific restrictions on deeply nested values.
 * xvariant_t is guaranteed to handle nesting up to at least 64 levels.
 *
 * Returns: %TRUE if @value is in normal form
 *
 * Since: 2.24
 **/
xboolean_t
xvariant_is_normal_form (xvariant_t *value)
{
  if (value->state & STATE_TRUSTED)
    return TRUE;

  xvariant_lock (value);

  if (value->depth >= G_VARIANT_MAX_RECURSION_DEPTH)
    return FALSE;

  if (value->state & STATE_SERIALISED)
    {
      GVariantSerialised serialised = {
        value->type_info,
        (xpointer_t) value->contents.serialised.data,
        value->size,
        value->depth
      };

      if (xvariant_serialised_is_normal (serialised))
        value->state |= STATE_TRUSTED;
    }
  else
    {
      xboolean_t normal = TRUE;
      xsize_t i;

      for (i = 0; i < value->contents.tree.n_children; i++)
        normal &= xvariant_is_normal_form (value->contents.tree.children[i]);

      if (normal)
        value->state |= STATE_TRUSTED;
    }

  xvariant_unlock (value);

  return (value->state & STATE_TRUSTED) != 0;
}
