/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

/**
 * SECTION:gfileinfo
 * @short_description: File Information and Attributes
 * @include: gio/gio.h
 * @see_also: #xfile_t, [xfile_attribute_t][gio-xfile_attribute_t]
 *
 * Functionality for manipulating basic metadata for files. #xfile_info_t
 * implements methods for getting information that all files should
 * contain, and allows for manipulation of extended attributes.
 *
 * See [xfile_attribute_t][gio-xfile_attribute_t] for more information on how
 * GIO handles file attributes.
 *
 * To obtain a #xfile_info_t for a #xfile_t, use xfile_query_info() (or its
 * async variant). To obtain a #xfile_info_t for a file input or output
 * stream, use xfile_input_stream_query_info() or
 * xfile_output_stream_query_info() (or their async variants).
 *
 * To change the actual attributes of a file, you should then set the
 * attribute in the #xfile_info_t and call xfile_set_attributes_from_info()
 * or xfile_set_attributes_async() on a xfile_t.
 *
 * However, not all attributes can be changed in the file. For instance,
 * the actual size of a file cannot be changed via xfile_info_set_size().
 * You may call xfile_query_settable_attributes() and
 * xfile_query_writable_namespaces() to discover the settable attributes
 * of a particular file at runtime.
 *
 * The direct accessors, such as xfile_info_get_name(), are slightly more
 * optimized than the generic attribute accessors, such as
 * xfile_info_get_attribute_byte_string().This optimization will matter
 * only if calling the API in a tight loop.
 *
 * #xfile_attribute_matcher_t allows for searching through a #xfile_info_t for
 * attributes.
 **/

#include "config.h"

#include <string.h>

#include "gfileinfo.h"
#include "gfileinfo-priv.h"
#include "gfileattribute-priv.h"
#include "gicon.h"
#include "glibintl.h"


/* We use this nasty thing, because NULL is a valid attribute matcher (matches nothing) */
#define NO_ATTRIBUTE_MASK ((xfile_attribute_matcher_t *)1)

typedef struct  {
  xuint32_t attribute;
  GFileAttributeValue value;
} xfile_attribute_t;

struct _xfile_info
{
  xobject_t parent_instance;

  xarray_t *attributes;
  xfile_attribute_matcher_t *mask;
};

struct _xfile_info_class
{
  xobject_class_t parent_class;
};


XDEFINE_TYPE (xfile_info, xfile_info, XTYPE_OBJECT)

typedef struct {
  xuint32_t id;
  xuint32_t attribute_id_counter;
} NSInfo;

G_LOCK_DEFINE_STATIC (attribute_hash);
static int namespace_id_counter = 0;
static xhashtable_t *ns_hash = NULL;
static xhashtable_t *attribute_hash = NULL;
static char ***attributes = NULL;

/* Attribute ids are 32bit, we split it up like this:
 * |------------|--------------------|
 *   12 bit          20 bit
 *   namespace      attribute id
 *
 * This way the attributes gets sorted in namespace order
 */

#define NS_POS 20
#define NS_MASK ((xuint32_t)((1<<12) - 1))
#define ID_POS 0
#define ID_MASK ((xuint32_t)((1<<20) - 1))

#define GET_NS(_attr_id) \
    (((xuint32_t) (_attr_id) >> NS_POS) & NS_MASK)
#define GET_ID(_attr_id) \
    (((xuint32_t)(_attr_id) >> ID_POS) & ID_MASK)

#define MAKE_ATTR_ID(_ns, _id)				\
    ( ((((xuint32_t) _ns) & NS_MASK) << NS_POS) |		\
      ((((xuint32_t) _id) & ID_MASK) << ID_POS) )

static NSInfo *
_lookup_namespace (const char *namespace)
{
  NSInfo *ns_info;

  ns_info = xhash_table_lookup (ns_hash, namespace);
  if (ns_info == NULL)
    {
      ns_info = g_new0 (NSInfo, 1);
      ns_info->id = ++namespace_id_counter;
      xhash_table_insert (ns_hash, xstrdup (namespace), ns_info);
      attributes = g_realloc (attributes, (ns_info->id + 1) * sizeof (char **));
      attributes[ns_info->id] = g_new (char *, 1);
      attributes[ns_info->id][0] = xstrconcat (namespace, "::*", NULL);
    }
  return ns_info;
}

static xuint32_t
_lookup_attribute (const char *attribute)
{
  xuint32_t attr_id, id;
  char *ns;
  const char *colon;
  NSInfo *ns_info;

  attr_id = GPOINTER_TO_UINT (xhash_table_lookup (attribute_hash, attribute));

  if (attr_id != 0)
    return attr_id;

  colon = strstr (attribute, "::");
  if (colon)
    ns = xstrndup (attribute, colon - attribute);
  else
    ns = xstrdup ("");

  ns_info = _lookup_namespace (ns);
  g_free (ns);

  id = ++ns_info->attribute_id_counter;
  attributes[ns_info->id] = g_realloc (attributes[ns_info->id], (id + 1) * sizeof (char *));
  attributes[ns_info->id][id] = xstrdup (attribute);

  attr_id = MAKE_ATTR_ID (ns_info->id, id);

  xhash_table_insert (attribute_hash, attributes[ns_info->id][id], GUINT_TO_POINTER (attr_id));

  return attr_id;
}

static void
ensure_attribute_hash (void)
{
  if (attribute_hash != NULL)
    return;

  ns_hash = xhash_table_new (xstr_hash, xstr_equal);
  attribute_hash = xhash_table_new (xstr_hash, xstr_equal);

#define REGISTER_ATTRIBUTE(name) G_STMT_START{\
  xuint_t _u G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */; \
  _u = _lookup_attribute (XFILE_ATTRIBUTE_ ## name); \
  /* use for generating the ID: g_print ("#define XFILE_ATTRIBUTE_ID_%s (%u + %u)\n", #name + 17, _u & ~ID_MASK, _u & ID_MASK); */ \
  xassert (_u == XFILE_ATTRIBUTE_ID_ ## name); \
}G_STMT_END

  REGISTER_ATTRIBUTE (STANDARD_TYPE);
  REGISTER_ATTRIBUTE (STANDARD_IS_HIDDEN);
  REGISTER_ATTRIBUTE (STANDARD_IS_BACKUP);
  REGISTER_ATTRIBUTE (STANDARD_IS_SYMLINK);
  REGISTER_ATTRIBUTE (STANDARD_IS_VIRTUAL);
  REGISTER_ATTRIBUTE (STANDARD_NAME);
  REGISTER_ATTRIBUTE (STANDARD_DISPLAY_NAME);
  REGISTER_ATTRIBUTE (STANDARD_EDIT_NAME);
  REGISTER_ATTRIBUTE (STANDARD_COPY_NAME);
  REGISTER_ATTRIBUTE (STANDARD_DESCRIPTION);
  REGISTER_ATTRIBUTE (STANDARD_ICON);
  REGISTER_ATTRIBUTE (STANDARD_CONTENT_TYPE);
  REGISTER_ATTRIBUTE (STANDARD_FAST_CONTENT_TYPE);
  REGISTER_ATTRIBUTE (STANDARD_SIZE);
  REGISTER_ATTRIBUTE (STANDARD_ALLOCATED_SIZE);
  REGISTER_ATTRIBUTE (STANDARD_SYMLINK_TARGET);
  REGISTER_ATTRIBUTE (STANDARD_TARGET_URI);
  REGISTER_ATTRIBUTE (STANDARD_SORT_ORDER);
  REGISTER_ATTRIBUTE (STANDARD_SYMBOLIC_ICON);
  REGISTER_ATTRIBUTE (STANDARD_IS_VOLATILE);
  REGISTER_ATTRIBUTE (ETAG_VALUE);
  REGISTER_ATTRIBUTE (ID_FILE);
  REGISTER_ATTRIBUTE (ID_FILESYSTEM);
  REGISTER_ATTRIBUTE (ACCESS_CAN_READ);
  REGISTER_ATTRIBUTE (ACCESS_CAN_WRITE);
  REGISTER_ATTRIBUTE (ACCESS_CAN_EXECUTE);
  REGISTER_ATTRIBUTE (ACCESS_CAN_DELETE);
  REGISTER_ATTRIBUTE (ACCESS_CAN_TRASH);
  REGISTER_ATTRIBUTE (ACCESS_CAN_RENAME);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_MOUNT);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_UNMOUNT);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_EJECT);
  REGISTER_ATTRIBUTE (MOUNTABLE_UNIX_DEVICE);
  REGISTER_ATTRIBUTE (MOUNTABLE_UNIX_DEVICE_FILE);
  REGISTER_ATTRIBUTE (MOUNTABLE_HAL_UDI);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_START);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_START_DEGRADED);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_STOP);
  REGISTER_ATTRIBUTE (MOUNTABLE_START_STOP_TYPE);
  REGISTER_ATTRIBUTE (MOUNTABLE_CAN_POLL);
  REGISTER_ATTRIBUTE (MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC);
  REGISTER_ATTRIBUTE (TIME_MODIFIED);
  REGISTER_ATTRIBUTE (TIME_MODIFIED_USEC);
  REGISTER_ATTRIBUTE (TIME_ACCESS);
  REGISTER_ATTRIBUTE (TIME_ACCESS_USEC);
  REGISTER_ATTRIBUTE (TIME_CHANGED);
  REGISTER_ATTRIBUTE (TIME_CHANGED_USEC);
  REGISTER_ATTRIBUTE (TIME_CREATED);
  REGISTER_ATTRIBUTE (TIME_CREATED_USEC);
  REGISTER_ATTRIBUTE (UNIX_DEVICE);
  REGISTER_ATTRIBUTE (UNIX_INODE);
  REGISTER_ATTRIBUTE (UNIX_MODE);
  REGISTER_ATTRIBUTE (UNIX_NLINK);
  REGISTER_ATTRIBUTE (UNIX_UID);
  REGISTER_ATTRIBUTE (UNIX_GID);
  REGISTER_ATTRIBUTE (UNIX_RDEV);
  REGISTER_ATTRIBUTE (UNIX_BLOCK_SIZE);
  REGISTER_ATTRIBUTE (UNIX_BLOCKS);
  REGISTER_ATTRIBUTE (UNIX_IS_MOUNTPOINT);
  REGISTER_ATTRIBUTE (DOS_IS_ARCHIVE);
  REGISTER_ATTRIBUTE (DOS_IS_SYSTEM);
  REGISTER_ATTRIBUTE (DOS_IS_MOUNTPOINT);
  REGISTER_ATTRIBUTE (DOS_REPARSE_POINT_TAG);
  REGISTER_ATTRIBUTE (OWNER_USER);
  REGISTER_ATTRIBUTE (OWNER_USER_REAL);
  REGISTER_ATTRIBUTE (OWNER_GROUP);
  REGISTER_ATTRIBUTE (THUMBNAIL_PATH);
  REGISTER_ATTRIBUTE (THUMBNAILING_FAILED);
  REGISTER_ATTRIBUTE (THUMBNAIL_IS_VALID);
  REGISTER_ATTRIBUTE (PREVIEW_ICON);
  REGISTER_ATTRIBUTE (FILESYSTEM_SIZE);
  REGISTER_ATTRIBUTE (FILESYSTEM_FREE);
  REGISTER_ATTRIBUTE (FILESYSTEM_TYPE);
  REGISTER_ATTRIBUTE (FILESYSTEM_READONLY);
  REGISTER_ATTRIBUTE (FILESYSTEM_USE_PREVIEW);
  REGISTER_ATTRIBUTE (GVFS_BACKEND);
  REGISTER_ATTRIBUTE (SELINUX_CONTEXT);
  REGISTER_ATTRIBUTE (TRASH_ITEM_COUNT);
  REGISTER_ATTRIBUTE (TRASH_ORIG_PATH);
  REGISTER_ATTRIBUTE (TRASH_DELETION_DATE);

#undef REGISTER_ATTRIBUTE
}

static xuint32_t
lookup_namespace (const char *namespace)
{
  NSInfo *ns_info;
  xuint32_t id;

  G_LOCK (attribute_hash);

  ensure_attribute_hash ();

  ns_info = _lookup_namespace (namespace);
  id = 0;
  if (ns_info)
    id = ns_info->id;

  G_UNLOCK (attribute_hash);

  return id;
}

static char *
get_attribute_for_id (int attribute)
{
  char *s;
  G_LOCK (attribute_hash);
  s = attributes[GET_NS(attribute)][GET_ID(attribute)];
  G_UNLOCK (attribute_hash);
  return s;
}

static xuint32_t
lookup_attribute (const char *attribute)
{
  xuint32_t attr_id;

  G_LOCK (attribute_hash);
  ensure_attribute_hash ();

  attr_id = _lookup_attribute (attribute);

  G_UNLOCK (attribute_hash);

  return attr_id;
}

static void
xfile_info_finalize (xobject_t *object)
{
  xfile_info_t *info;
  xuint_t i;
  xfile_attribute_t *attrs;

  info = XFILE_INFO (object);

  attrs = (xfile_attribute_t *)info->attributes->data;
  for (i = 0; i < info->attributes->len; i++)
    _xfile_attribute_value_clear (&attrs[i].value);
  g_array_free (info->attributes, TRUE);

  if (info->mask != NO_ATTRIBUTE_MASK)
    xfile_attribute_matcher_unref (info->mask);

  XOBJECT_CLASS (xfile_info_parent_class)->finalize (object);
}

static void
xfile_info_class_init (xfile_info_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = xfile_info_finalize;
}

static void
xfile_info_init (xfile_info_t *info)
{
  info->mask = NO_ATTRIBUTE_MASK;
  info->attributes = g_array_new (FALSE, FALSE,
				  sizeof (xfile_attribute_t));
}

/**
 * xfile_info_new:
 *
 * Creates a new file info structure.
 *
 * Returns: a #xfile_info_t.
 **/
xfile_info_t *
xfile_info_new (void)
{
  return xobject_new (XTYPE_FILE_INFO, NULL);
}

/**
 * xfile_info_copy_into:
 * @src_info: source to copy attributes from.
 * @dest_info: destination to copy attributes to.
 *
 * First clears all of the [xfile_attribute_t][gio-xfile_attribute_t] of @dest_info,
 * and then copies all of the file attributes from @src_info to @dest_info.
 **/
void
xfile_info_copy_into (xfile_info_t *src_info,
                       xfile_info_t *dest_info)
{
  xfile_attribute_t *source, *dest;
  xuint_t i;

  g_return_if_fail (X_IS_FILE_INFO (src_info));
  g_return_if_fail (X_IS_FILE_INFO (dest_info));

  dest = (xfile_attribute_t *)dest_info->attributes->data;
  for (i = 0; i < dest_info->attributes->len; i++)
    _xfile_attribute_value_clear (&dest[i].value);

  g_array_set_size (dest_info->attributes,
		    src_info->attributes->len);

  source = (xfile_attribute_t *)src_info->attributes->data;
  dest = (xfile_attribute_t *)dest_info->attributes->data;

  for (i = 0; i < src_info->attributes->len; i++)
    {
      dest[i].attribute = source[i].attribute;
      dest[i].value.type = XFILE_ATTRIBUTE_TYPE_INVALID;
      _xfile_attribute_value_set (&dest[i].value, &source[i].value);
    }

  if (dest_info->mask != NO_ATTRIBUTE_MASK)
    xfile_attribute_matcher_unref (dest_info->mask);

  if (src_info->mask == NO_ATTRIBUTE_MASK)
    dest_info->mask = NO_ATTRIBUTE_MASK;
  else
    dest_info->mask = xfile_attribute_matcher_ref (src_info->mask);
}

/**
 * xfile_info_dup:
 * @other: a #xfile_info_t.
 *
 * Duplicates a file info structure.
 *
 * Returns: (transfer full): a duplicate #xfile_info_t of @other.
 **/
xfile_info_t *
xfile_info_dup (xfile_info_t *other)
{
  xfile_info_t *new;

  xreturn_val_if_fail (X_IS_FILE_INFO (other), NULL);

  new = xfile_info_new ();
  xfile_info_copy_into (other, new);
  return new;
}

/**
 * xfile_info_set_attribute_mask:
 * @info: a #xfile_info_t.
 * @mask: a #xfile_attribute_matcher_t.
 *
 * Sets @mask on @info to match specific attribute types.
 **/
void
xfile_info_set_attribute_mask (xfile_info_t             *info,
				xfile_attribute_matcher_t *mask)
{
  xfile_attribute_t *attr;
  xuint_t i;

  g_return_if_fail (X_IS_FILE_INFO (info));

  if (mask != info->mask)
    {
      if (info->mask != NO_ATTRIBUTE_MASK)
	xfile_attribute_matcher_unref (info->mask);
      info->mask = xfile_attribute_matcher_ref (mask);

      /* Remove non-matching attributes */
      for (i = 0; i < info->attributes->len; i++)
	{
	  attr = &g_array_index (info->attributes, xfile_attribute_t, i);
	  if (!_xfile_attribute_matcher_matches_id (mask,
						    attr->attribute))
	    {
	      _xfile_attribute_value_clear (&attr->value);
	      g_array_remove_index (info->attributes, i);
	      i--;
	    }
	}
    }
}

/**
 * xfile_info_unset_attribute_mask:
 * @info: #xfile_info_t.
 *
 * Unsets a mask set by xfile_info_set_attribute_mask(), if one
 * is set.
 **/
void
xfile_info_unset_attribute_mask (xfile_info_t *info)
{
  g_return_if_fail (X_IS_FILE_INFO (info));

  if (info->mask != NO_ATTRIBUTE_MASK)
    xfile_attribute_matcher_unref (info->mask);
  info->mask = NO_ATTRIBUTE_MASK;
}

/**
 * xfile_info_clear_status:
 * @info: a #xfile_info_t.
 *
 * Clears the status information from @info.
 **/
void
xfile_info_clear_status (xfile_info_t  *info)
{
  xfile_attribute_t *attrs;
  xuint_t i;

  g_return_if_fail (X_IS_FILE_INFO (info));

  attrs = (xfile_attribute_t *)info->attributes->data;
  for (i = 0; i < info->attributes->len; i++)
    attrs[i].value.status = XFILE_ATTRIBUTE_STATUS_UNSET;
}

static int
xfile_info_find_place (xfile_info_t  *info,
			xuint32_t     attribute)
{
  int min, max, med;
  xfile_attribute_t *attrs;
  /* Binary search for the place where attribute would be, if it's
     in the array */

  min = 0;
  max = info->attributes->len;

  attrs = (xfile_attribute_t *)info->attributes->data;

  while (min < max)
    {
      med = min + (max - min) / 2;
      if (attrs[med].attribute == attribute)
	{
	  min = med;
	  break;
	}
      else if (attrs[med].attribute < attribute)
	min = med + 1;
      else /* attrs[med].attribute > attribute */
	max = med;
    }

  return min;
}

static GFileAttributeValue *
xfile_info_find_value (xfile_info_t *info,
			xuint32_t    attr_id)
{
  xfile_attribute_t *attrs;
  xuint_t i;

  i = xfile_info_find_place (info, attr_id);
  attrs = (xfile_attribute_t *)info->attributes->data;
  if (i < info->attributes->len &&
      attrs[i].attribute == attr_id)
    return &attrs[i].value;

  return NULL;
}

static GFileAttributeValue *
xfile_info_find_value_by_name (xfile_info_t  *info,
				const char *attribute)
{
  xuint32_t attr_id;

  attr_id = lookup_attribute (attribute);
  return xfile_info_find_value (info, attr_id);
}

/**
 * xfile_info_has_attribute:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Checks if a file info structure has an attribute named @attribute.
 *
 * Returns: %TRUE if @info has an attribute named @attribute,
 *     %FALSE otherwise.
 **/
xboolean_t
xfile_info_has_attribute (xfile_info_t  *info,
			   const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  value = xfile_info_find_value_by_name (info, attribute);
  return value != NULL;
}

/**
 * xfile_info_has_namespace:
 * @info: a #xfile_info_t.
 * @name_space: a file attribute namespace.
 *
 * Checks if a file info structure has an attribute in the
 * specified @name_space.
 *
 * Returns: %TRUE if @info has an attribute in @name_space,
 *     %FALSE otherwise.
 *
 * Since: 2.22
 **/
xboolean_t
xfile_info_has_namespace (xfile_info_t  *info,
			   const char *name_space)
{
  xfile_attribute_t *attrs;
  xuint32_t ns_id;
  xuint_t i;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);
  xreturn_val_if_fail (name_space != NULL, FALSE);

  ns_id = lookup_namespace (name_space);

  attrs = (xfile_attribute_t *)info->attributes->data;
  for (i = 0; i < info->attributes->len; i++)
    {
      if (GET_NS (attrs[i].attribute) == ns_id)
	return TRUE;
    }

  return FALSE;
}

/**
 * xfile_info_list_attributes:
 * @info: a #xfile_info_t.
 * @name_space: (nullable): a file attribute key's namespace, or %NULL to list
 *   all attributes.
 *
 * Lists the file info structure's attributes.
 *
 * Returns: (nullable) (array zero-terminated=1) (transfer full): a
 * null-terminated array of strings of all of the possible attribute
 * types for the given @name_space, or %NULL on error.
 **/
char **
xfile_info_list_attributes (xfile_info_t  *info,
			     const char *name_space)
{
  xptr_array_t *names;
  xfile_attribute_t *attrs;
  xuint32_t attribute;
  xuint32_t ns_id = (name_space) ? lookup_namespace (name_space) : 0;
  xuint_t i;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  names = xptr_array_new ();
  attrs = (xfile_attribute_t *)info->attributes->data;
  for (i = 0; i < info->attributes->len; i++)
    {
      attribute = attrs[i].attribute;
      if (ns_id == 0 || GET_NS (attribute) == ns_id)
        xptr_array_add (names, xstrdup (get_attribute_for_id (attribute)));
    }

  /* NULL terminate */
  xptr_array_add (names, NULL);

  return (char **)xptr_array_free (names, FALSE);
}

/**
 * xfile_info_get_attribute_type:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the attribute type for an attribute key.
 *
 * Returns: a #xfile_attribute_type_t for the given @attribute, or
 * %XFILE_ATTRIBUTE_TYPE_INVALID if the key is not set.
 **/
xfile_attribute_type_t
xfile_info_get_attribute_type (xfile_info_t  *info,
				const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), XFILE_ATTRIBUTE_TYPE_INVALID);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', XFILE_ATTRIBUTE_TYPE_INVALID);

  value = xfile_info_find_value_by_name (info, attribute);
  if (value)
    return value->type;
  else
    return XFILE_ATTRIBUTE_TYPE_INVALID;
}

/**
 * xfile_info_remove_attribute:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Removes all cases of @attribute from @info if it exists.
 **/
void
xfile_info_remove_attribute (xfile_info_t  *info,
			      const char *attribute)
{
  xuint32_t attr_id;
  xfile_attribute_t *attrs;
  xuint_t i;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  attr_id = lookup_attribute (attribute);

  i = xfile_info_find_place (info, attr_id);
  attrs = (xfile_attribute_t *)info->attributes->data;
  if (i < info->attributes->len &&
      attrs[i].attribute == attr_id)
    {
      _xfile_attribute_value_clear (&attrs[i].value);
      g_array_remove_index (info->attributes, i);
    }
}

/**
 * xfile_info_get_attribute_data:
 * @info: a #xfile_info_t
 * @attribute: a file attribute key
 * @type: (out) (optional): return location for the attribute type, or %NULL
 * @value_pp: (out) (optional) (not nullable): return location for the
 *    attribute value, or %NULL; the attribute value will not be %NULL
 * @status: (out) (optional): return location for the attribute status, or %NULL
 *
 * Gets the attribute type, value and status for an attribute key.
 *
 * Returns: (transfer none): %TRUE if @info has an attribute named @attribute,
 *      %FALSE otherwise.
 */
xboolean_t
xfile_info_get_attribute_data (xfile_info_t            *info,
				const char           *attribute,
				xfile_attribute_type_t   *type,
				xpointer_t             *value_pp,
				xfile_attribute_status_t *status)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  value = xfile_info_find_value_by_name (info, attribute);
  if (value == NULL)
    return FALSE;

  if (status)
    *status = value->status;

  if (type)
    *type = value->type;

  if (value_pp)
    *value_pp = _xfile_attribute_value_peek_as_pointer (value);

  return TRUE;
}

/**
 * xfile_info_get_attribute_status:
 * @info: a #xfile_info_t
 * @attribute: a file attribute key
 *
 * Gets the attribute status for an attribute key.
 *
 * Returns: a #xfile_attribute_status_t for the given @attribute, or
 *    %XFILE_ATTRIBUTE_STATUS_UNSET if the key is invalid.
 *
 */
xfile_attribute_status_t
xfile_info_get_attribute_status (xfile_info_t  *info,
				  const char *attribute)
{
  GFileAttributeValue *val;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), 0);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', 0);

  val = xfile_info_find_value_by_name (info, attribute);
  if (val)
    return val->status;

  return XFILE_ATTRIBUTE_STATUS_UNSET;
}

/**
 * xfile_info_set_attribute_status:
 * @info: a #xfile_info_t
 * @attribute: a file attribute key
 * @status: a #xfile_attribute_status_t
 *
 * Sets the attribute status for an attribute key. This is only
 * needed by external code that implement xfile_set_attributes_from_info()
 * or similar functions.
 *
 * The attribute must exist in @info for this to work. Otherwise %FALSE
 * is returned and @info is unchanged.
 *
 * Returns: %TRUE if the status was changed, %FALSE if the key was not set.
 *
 * Since: 2.22
 */
xboolean_t
xfile_info_set_attribute_status (xfile_info_t  *info,
				  const char *attribute,
				  xfile_attribute_status_t status)
{
  GFileAttributeValue *val;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  val = xfile_info_find_value_by_name (info, attribute);
  if (val)
    {
      val->status = status;
      return TRUE;
    }

  return FALSE;
}

GFileAttributeValue *
_xfile_info_get_attribute_value (xfile_info_t  *info,
				  const char *attribute)

{
  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', NULL);

  return xfile_info_find_value_by_name (info, attribute);
}

/**
 * xfile_info_get_attribute_as_string:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the value of a attribute, formatted as a string.
 * This escapes things as needed to make the string valid
 * UTF-8.
 *
 * Returns: (nullable): a UTF-8 string associated with the given @attribute, or
 *    %NULL if the attribute wasn’t set.
 *    When you're done with the string it must be freed with g_free().
 **/
char *
xfile_info_get_attribute_as_string (xfile_info_t  *info,
				     const char *attribute)
{
  GFileAttributeValue *val;
  val = _xfile_info_get_attribute_value (info, attribute);
  if (val)
    return _xfile_attribute_value_as_string (val);
  return NULL;
}


/**
 * xfile_info_get_attribute_object:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the value of a #xobject_t attribute. If the attribute does
 * not contain a #xobject_t, %NULL will be returned.
 *
 * Returns: (transfer none) (nullable): a #xobject_t associated with the given @attribute,
 * or %NULL otherwise.
 **/
xobject_t *
xfile_info_get_attribute_object (xfile_info_t  *info,
				  const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', NULL);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_object (value);
}

/**
 * xfile_info_get_attribute_string:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the value of a string attribute. If the attribute does
 * not contain a string, %NULL will be returned.
 *
 * Returns: (nullable): the contents of the @attribute value as a UTF-8 string,
 * or %NULL otherwise.
 **/
const char *
xfile_info_get_attribute_string (xfile_info_t  *info,
				  const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', NULL);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_string (value);
}

/**
 * xfile_info_get_attribute_byte_string:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the value of a byte string attribute. If the attribute does
 * not contain a byte string, %NULL will be returned.
 *
 * Returns: (nullable): the contents of the @attribute value as a byte string, or
 * %NULL otherwise.
 **/
const char *
xfile_info_get_attribute_byte_string (xfile_info_t  *info,
				       const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', NULL);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_byte_string (value);
}

/**
 * xfile_info_get_attribute_stringv:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the value of a stringv attribute. If the attribute does
 * not contain a stringv, %NULL will be returned.
 *
 * Returns: (transfer none) (nullable): the contents of the @attribute value as a stringv,
 * or %NULL otherwise. Do not free. These returned strings are UTF-8.
 *
 * Since: 2.22
 **/
char **
xfile_info_get_attribute_stringv (xfile_info_t  *info,
				   const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', NULL);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_stringv (value);
}

/**
 * xfile_info_get_attribute_boolean:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets the value of a boolean attribute. If the attribute does not
 * contain a boolean value, %FALSE will be returned.
 *
 * Returns: the boolean value contained within the attribute.
 **/
xboolean_t
xfile_info_get_attribute_boolean (xfile_info_t  *info,
				   const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_boolean (value);
}

/**
 * xfile_info_get_attribute_uint32:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets an unsigned 32-bit integer contained within the attribute. If the
 * attribute does not contain an unsigned 32-bit integer, or is invalid,
 * 0 will be returned.
 *
 * Returns: an unsigned 32-bit integer from the attribute.
 **/
xuint32_t
xfile_info_get_attribute_uint32 (xfile_info_t  *info,
				  const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), 0);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', 0);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_uint32 (value);
}

/**
 * xfile_info_get_attribute_int32:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets a signed 32-bit integer contained within the attribute. If the
 * attribute does not contain a signed 32-bit integer, or is invalid,
 * 0 will be returned.
 *
 * Returns: a signed 32-bit integer from the attribute.
 **/
gint32
xfile_info_get_attribute_int32 (xfile_info_t  *info,
				 const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), 0);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', 0);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_int32 (value);
}

/**
 * xfile_info_get_attribute_uint64:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets a unsigned 64-bit integer contained within the attribute. If the
 * attribute does not contain an unsigned 64-bit integer, or is invalid,
 * 0 will be returned.
 *
 * Returns: a unsigned 64-bit integer from the attribute.
 **/
xuint64_t
xfile_info_get_attribute_uint64 (xfile_info_t  *info,
				  const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), 0);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', 0);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_uint64 (value);
}

/**
 * xfile_info_get_attribute_int64:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 *
 * Gets a signed 64-bit integer contained within the attribute. If the
 * attribute does not contain a signed 64-bit integer, or is invalid,
 * 0 will be returned.
 *
 * Returns: a signed 64-bit integer from the attribute.
 **/
sint64_t
xfile_info_get_attribute_int64  (xfile_info_t  *info,
				  const char *attribute)
{
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), 0);
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', 0);

  value = xfile_info_find_value_by_name (info, attribute);
  return _xfile_attribute_value_get_int64 (value);
}

static GFileAttributeValue *
xfile_info_create_value (xfile_info_t *info,
			  xuint32_t attr_id)
{
  xfile_attribute_t *attrs;
  xuint_t i;

  if (info->mask != NO_ATTRIBUTE_MASK &&
      !_xfile_attribute_matcher_matches_id (info->mask, attr_id))
    return NULL;

  i = xfile_info_find_place (info, attr_id);

  attrs = (xfile_attribute_t *)info->attributes->data;
  if (i < info->attributes->len &&
      attrs[i].attribute == attr_id)
    return &attrs[i].value;
  else
    {
      xfile_attribute_t attr = { 0 };
      attr.attribute = attr_id;
      g_array_insert_val (info->attributes, i, attr);

      attrs = (xfile_attribute_t *)info->attributes->data;
      return &attrs[i].value;
    }
}

void
_xfile_info_set_attribute_by_id (xfile_info_t                 *info,
                                  xuint32_t                    attribute,
                                  xfile_attribute_type_t         type,
                                  xpointer_t                   value_p)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);

  if (value)
    _xfile_attribute_value_set_from_pointer (value, type, value_p, TRUE);
}

/**
 * xfile_info_set_attribute:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @type: a #xfile_attribute_type_t
 * @value_p: (not nullable): pointer to the value
 *
 * Sets the @attribute to contain the given value, if possible. To unset the
 * attribute, use %XFILE_ATTRIBUTE_TYPE_INVALID for @type.
 **/
void
xfile_info_set_attribute (xfile_info_t                 *info,
			   const char                *attribute,
			   xfile_attribute_type_t         type,
			   xpointer_t                   value_p)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  _xfile_info_set_attribute_by_id (info, lookup_attribute (attribute), type, value_p);
}

void
_xfile_info_set_attribute_object_by_id (xfile_info_t *info,
                                         xuint32_t    attribute,
				         xobject_t   *attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_object (value, attr_value);
}

/**
 * xfile_info_set_attribute_object:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: a #xobject_t.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_object (xfile_info_t  *info,
				  const char *attribute,
				  xobject_t    *attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');
  g_return_if_fail (X_IS_OBJECT (attr_value));

  _xfile_info_set_attribute_object_by_id (info,
                                           lookup_attribute (attribute),
                                           attr_value);
}

void
_xfile_info_set_attribute_stringv_by_id (xfile_info_t *info,
                                          xuint32_t    attribute,
				          char     **attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_stringv (value, attr_value);
}

/**
 * xfile_info_set_attribute_stringv:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key
 * @attr_value: (array zero-terminated=1) (element-type utf8): a %NULL
 *   terminated array of UTF-8 strings.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 *
 * Sinze: 2.22
 **/
void
xfile_info_set_attribute_stringv (xfile_info_t  *info,
				   const char *attribute,
				   char      **attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');
  g_return_if_fail (attr_value != NULL);

  _xfile_info_set_attribute_stringv_by_id (info,
                                            lookup_attribute (attribute),
                                            attr_value);
}

void
_xfile_info_set_attribute_string_by_id (xfile_info_t  *info,
                                         xuint32_t     attribute,
				         const char *attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_string (value, attr_value);
}

/**
 * xfile_info_set_attribute_string:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: a UTF-8 string.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_string (xfile_info_t  *info,
				  const char *attribute,
				  const char *attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');
  g_return_if_fail (attr_value != NULL);

  _xfile_info_set_attribute_string_by_id (info,
                                           lookup_attribute (attribute),
                                           attr_value);
}

void
_xfile_info_set_attribute_byte_string_by_id (xfile_info_t  *info,
                                              xuint32_t     attribute,
				              const char *attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_byte_string (value, attr_value);
}

/**
 * xfile_info_set_attribute_byte_string:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: a byte string.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_byte_string (xfile_info_t  *info,
				       const char *attribute,
				       const char *attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');
  g_return_if_fail (attr_value != NULL);

  _xfile_info_set_attribute_byte_string_by_id (info,
                                                lookup_attribute (attribute),
                                                attr_value);
}

void
_xfile_info_set_attribute_boolean_by_id (xfile_info_t *info,
                                          xuint32_t    attribute,
				          xboolean_t   attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_boolean (value, attr_value);
}

/**
 * xfile_info_set_attribute_boolean:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: a boolean value.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_boolean (xfile_info_t  *info,
				   const char *attribute,
				   xboolean_t    attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  _xfile_info_set_attribute_boolean_by_id (info,
                                            lookup_attribute (attribute),
                                            attr_value);
}

void
_xfile_info_set_attribute_uint32_by_id (xfile_info_t *info,
                                         xuint32_t    attribute,
				         xuint32_t    attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_uint32 (value, attr_value);
}

/**
 * xfile_info_set_attribute_uint32:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: an unsigned 32-bit integer.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_uint32 (xfile_info_t  *info,
				  const char *attribute,
				  xuint32_t     attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  _xfile_info_set_attribute_uint32_by_id (info,
                                           lookup_attribute (attribute),
                                           attr_value);
}

void
_xfile_info_set_attribute_int32_by_id (xfile_info_t *info,
                                        xuint32_t    attribute,
				        gint32     attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_int32 (value, attr_value);
}

/**
 * xfile_info_set_attribute_int32:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: a signed 32-bit integer
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_int32 (xfile_info_t  *info,
                                 const char *attribute,
                                 gint32      attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  _xfile_info_set_attribute_int32_by_id (info,
                                          lookup_attribute (attribute),
                                          attr_value);
}

void
_xfile_info_set_attribute_uint64_by_id (xfile_info_t *info,
                                         xuint32_t    attribute,
				         xuint64_t    attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_uint64 (value, attr_value);
}

/**
 * xfile_info_set_attribute_uint64:
 * @info: a #xfile_info_t.
 * @attribute: a file attribute key.
 * @attr_value: an unsigned 64-bit integer.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 **/
void
xfile_info_set_attribute_uint64 (xfile_info_t  *info,
				  const char *attribute,
				  xuint64_t     attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  _xfile_info_set_attribute_uint64_by_id (info,
                                           lookup_attribute (attribute),
                                           attr_value);
}

void
_xfile_info_set_attribute_int64_by_id (xfile_info_t *info,
                                        xuint32_t    attribute,
				        sint64_t     attr_value)
{
  GFileAttributeValue *value;

  value = xfile_info_create_value (info, attribute);
  if (value)
    _xfile_attribute_value_set_int64 (value, attr_value);
}

/**
 * xfile_info_set_attribute_int64:
 * @info: a #xfile_info_t.
 * @attribute: attribute name to set.
 * @attr_value: int64 value to set attribute to.
 *
 * Sets the @attribute to contain the given @attr_value,
 * if possible.
 *
 **/
void
xfile_info_set_attribute_int64  (xfile_info_t  *info,
				  const char *attribute,
				  sint64_t      attr_value)
{
  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (attribute != NULL && *attribute != '\0');

  _xfile_info_set_attribute_int64_by_id (info,
                                          lookup_attribute (attribute),
                                          attr_value);
}

/* Helper getters */
/**
 * xfile_info_get_deletion_date:
 * @info: a #xfile_info_t.
 *
 * Returns the #xdatetime_t representing the deletion date of the file, as
 * available in XFILE_ATTRIBUTE_TRASH_DELETION_DATE. If the
 * XFILE_ATTRIBUTE_TRASH_DELETION_DATE attribute is unset, %NULL is returned.
 *
 * Returns: (nullable): a #xdatetime_t, or %NULL.
 *
 * Since: 2.36
 **/
xdatetime_t *
xfile_info_get_deletion_date (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;
  const char *date_str;
  xtimezone_t *local_tz = NULL;
  xdatetime_t *dt = NULL;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_TRASH_DELETION_DATE);

  value = xfile_info_find_value (info, attr);
  date_str = _xfile_attribute_value_get_string (value);
  if (!date_str)
    return NULL;

  local_tz = xtime_zone_new_local ();
  dt = xdate_time_new_from_iso8601 (date_str, local_tz);
  xtime_zone_unref (local_tz);

  return g_steal_pointer (&dt);
}

/**
 * xfile_info_get_file_type:
 * @info: a #xfile_info_t.
 *
 * Gets a file's type (whether it is a regular file, symlink, etc).
 * This is different from the file's content type, see xfile_info_get_content_type().
 *
 * Returns: a #xfile_type_t for the given file.
 **/
xfile_type_t
xfile_info_get_file_type (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), XFILE_TYPE_UNKNOWN);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_TYPE);

  value = xfile_info_find_value (info, attr);
  return (xfile_type_t)_xfile_attribute_value_get_uint32 (value);
}

/**
 * xfile_info_get_is_hidden:
 * @info: a #xfile_info_t.
 *
 * Checks if a file is hidden.
 *
 * Returns: %TRUE if the file is a hidden file, %FALSE otherwise.
 **/
xboolean_t
xfile_info_get_is_hidden (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_IS_HIDDEN);

  value = xfile_info_find_value (info, attr);
  return (xfile_type_t)_xfile_attribute_value_get_boolean (value);
}

/**
 * xfile_info_get_is_backup:
 * @info: a #xfile_info_t.
 *
 * Checks if a file is a backup file.
 *
 * Returns: %TRUE if file is a backup file, %FALSE otherwise.
 **/
xboolean_t
xfile_info_get_is_backup (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_IS_BACKUP);

  value = xfile_info_find_value (info, attr);
  return (xfile_type_t)_xfile_attribute_value_get_boolean (value);
}

/**
 * xfile_info_get_is_symlink:
 * @info: a #xfile_info_t.
 *
 * Checks if a file is a symlink.
 *
 * Returns: %TRUE if the given @info is a symlink.
 **/
xboolean_t
xfile_info_get_is_symlink (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), FALSE);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_IS_SYMLINK);

  value = xfile_info_find_value (info, attr);
  return (xfile_type_t)_xfile_attribute_value_get_boolean (value);
}

/**
 * xfile_info_get_name:
 * @info: a #xfile_info_t.
 *
 * Gets the name for a file. This is guaranteed to always be set.
 *
 * Returns: (type filename) (not nullable): a string containing the file name.
 **/
const char *
xfile_info_get_name (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_NAME);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_byte_string (value);
}

/**
 * xfile_info_get_display_name:
 * @info: a #xfile_info_t.
 *
 * Gets a display name for a file. This is guaranteed to always be set.
 *
 * Returns: (not nullable): a string containing the display name.
 **/
const char *
xfile_info_get_display_name (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_string (value);
}

/**
 * xfile_info_get_edit_name:
 * @info: a #xfile_info_t.
 *
 * Gets the edit name for a file.
 *
 * Returns: a string containing the edit name.
 **/
const char *
xfile_info_get_edit_name (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_EDIT_NAME);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_string (value);
}

/**
 * xfile_info_get_icon:
 * @info: a #xfile_info_t.
 *
 * Gets the icon for a file.
 *
 * Returns: (nullable) (transfer none): #xicon_t for the given @info.
 **/
xicon_t *
xfile_info_get_icon (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;
  xobject_t *obj;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_ICON);

  value = xfile_info_find_value (info, attr);
  obj = _xfile_attribute_value_get_object (value);
  if (X_IS_ICON (obj))
    return XICON (obj);
  return NULL;
}

/**
 * xfile_info_get_symbolic_icon:
 * @info: a #xfile_info_t.
 *
 * Gets the symbolic icon for a file.
 *
 * Returns: (nullable) (transfer none): #xicon_t for the given @info.
 *
 * Since: 2.34
 **/
xicon_t *
xfile_info_get_symbolic_icon (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;
  xobject_t *obj;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON);

  value = xfile_info_find_value (info, attr);
  obj = _xfile_attribute_value_get_object (value);
  if (X_IS_ICON (obj))
    return XICON (obj);
  return NULL;
}

/**
 * xfile_info_get_content_type:
 * @info: a #xfile_info_t.
 *
 * Gets the file's content type.
 *
 * Returns: (nullable): a string containing the file's content type,
 * or %NULL if unknown.
 **/
const char *
xfile_info_get_content_type (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_string (value);
}

/**
 * xfile_info_get_size:
 * @info: a #xfile_info_t.
 *
 * Gets the file's size (in bytes). The size is retrieved through the value of
 * the %XFILE_ATTRIBUTE_STANDARD_SIZE attribute and is converted
 * from #xuint64_t to #xoffset_t before returning the result.
 *
 * Returns: a #xoffset_t containing the file's size (in bytes).
 **/
xoffset_t
xfile_info_get_size (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), (xoffset_t) 0);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SIZE);

  value = xfile_info_find_value (info, attr);
  return (xoffset_t) _xfile_attribute_value_get_uint64 (value);
}

/**
 * xfile_info_get_modification_time:
 * @info: a #xfile_info_t.
 * @result: (out caller-allocates): a #GTimeVal.
 *
 * Gets the modification time of the current @info and sets it
 * in @result.
 *
 * Deprecated: 2.62: Use xfile_info_get_modification_date_time() instead, as
 *    #GTimeVal is deprecated due to the year 2038 problem.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
xfile_info_get_modification_time (xfile_info_t *info,
				   GTimeVal  *result)
{
  static xuint32_t attr_mtime = 0, attr_mtime_usec;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (result != NULL);

  if (attr_mtime == 0)
    {
      attr_mtime = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED);
      attr_mtime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    }

  value = xfile_info_find_value (info, attr_mtime);
  result->tv_sec = _xfile_attribute_value_get_uint64 (value);
  value = xfile_info_find_value (info, attr_mtime_usec);
  result->tv_usec = _xfile_attribute_value_get_uint32 (value);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xfile_info_get_modification_date_time:
 * @info: a #xfile_info_t.
 *
 * Gets the modification time of the current @info and returns it as a
 * #xdatetime_t.
 *
 * This requires the %XFILE_ATTRIBUTE_TIME_MODIFIED attribute. If
 * %XFILE_ATTRIBUTE_TIME_MODIFIED_USEC is provided, the resulting #xdatetime_t
 * will have microsecond precision.
 *
 * Returns: (transfer full) (nullable): modification time, or %NULL if unknown
 * Since: 2.62
 */
xdatetime_t *
xfile_info_get_modification_date_time (xfile_info_t *info)
{
  static xuint32_t attr_mtime = 0, attr_mtime_usec;
  GFileAttributeValue *value, *value_usec;
  xdatetime_t *dt = NULL, *dt2 = NULL;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr_mtime == 0)
    {
      attr_mtime = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED);
      attr_mtime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    }

  value = xfile_info_find_value (info, attr_mtime);
  if (value == NULL)
    return NULL;

  dt = xdate_time_new_from_unix_utc (_xfile_attribute_value_get_uint64 (value));

  value_usec = xfile_info_find_value (info, attr_mtime_usec);
  if (value_usec == NULL)
    return g_steal_pointer (&dt);

  dt2 = xdate_time_add (dt, _xfile_attribute_value_get_uint32 (value_usec));
  xdate_time_unref (dt);

  return g_steal_pointer (&dt2);
}

/**
 * xfile_info_get_access_date_time:
 * @info: a #xfile_info_t.
 *
 * Gets the access time of the current @info and returns it as a
 * #xdatetime_t.
 *
 * This requires the %XFILE_ATTRIBUTE_TIME_ACCESS attribute. If
 * %XFILE_ATTRIBUTE_TIME_ACCESS_USEC is provided, the resulting #xdatetime_t
 * will have microsecond precision.
 *
 * Returns: (transfer full) (nullable): access time, or %NULL if unknown
 * Since: 2.70
 */
xdatetime_t *
xfile_info_get_access_date_time (xfile_info_t *info)
{
  static xuint32_t attr_atime = 0, attr_atime_usec;
  GFileAttributeValue *value, *value_usec;
  xdatetime_t *dt = NULL, *dt2 = NULL;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr_atime == 0)
    {
      attr_atime = lookup_attribute (XFILE_ATTRIBUTE_TIME_ACCESS);
      attr_atime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_ACCESS_USEC);
    }

  value = xfile_info_find_value (info, attr_atime);
  if (value == NULL)
    return NULL;

  dt = xdate_time_new_from_unix_utc (_xfile_attribute_value_get_uint64 (value));

  value_usec = xfile_info_find_value (info, attr_atime_usec);
  if (value_usec == NULL)
    return g_steal_pointer (&dt);

  dt2 = xdate_time_add (dt, _xfile_attribute_value_get_uint32 (value_usec));
  xdate_time_unref (dt);

  return g_steal_pointer (&dt2);
}

/**
 * xfile_info_get_creation_date_time:
 * @info: a #xfile_info_t.
 *
 * Gets the creation time of the current @info and returns it as a
 * #xdatetime_t.
 *
 * This requires the %XFILE_ATTRIBUTE_TIME_CREATED attribute. If
 * %XFILE_ATTRIBUTE_TIME_CREATED_USEC is provided, the resulting #xdatetime_t
 * will have microsecond precision.
 *
 * Returns: (transfer full) (nullable): creation time, or %NULL if unknown
 * Since: 2.70
 */
xdatetime_t *
xfile_info_get_creation_date_time (xfile_info_t *info)
{
  static xuint32_t attr_ctime = 0, attr_ctime_usec;
  GFileAttributeValue *value, *value_usec;
  xdatetime_t *dt = NULL, *dt2 = NULL;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr_ctime == 0)
    {
      attr_ctime = lookup_attribute (XFILE_ATTRIBUTE_TIME_CREATED);
      attr_ctime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_CREATED_USEC);
    }

  value = xfile_info_find_value (info, attr_ctime);
  if (value == NULL)
    return NULL;

  dt = xdate_time_new_from_unix_utc (_xfile_attribute_value_get_uint64 (value));

  value_usec = xfile_info_find_value (info, attr_ctime_usec);
  if (value_usec == NULL)
    return g_steal_pointer (&dt);

  dt2 = xdate_time_add (dt, _xfile_attribute_value_get_uint32 (value_usec));
  xdate_time_unref (dt);

  return g_steal_pointer (&dt2);
}

/**
 * xfile_info_get_symlink_target:
 * @info: a #xfile_info_t.
 *
 * Gets the symlink target for a given #xfile_info_t.
 *
 * Returns: (nullable): a string containing the symlink target.
 **/
const char *
xfile_info_get_symlink_target (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_byte_string (value);
}

/**
 * xfile_info_get_etag:
 * @info: a #xfile_info_t.
 *
 * Gets the [entity tag][gfile-etag] for a given
 * #xfile_info_t. See %XFILE_ATTRIBUTE_ETAG_VALUE.
 *
 * Returns: (nullable): a string containing the value of the "etag:value" attribute.
 **/
const char *
xfile_info_get_etag (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_ETAG_VALUE);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_string (value);
}

/**
 * xfile_info_get_sort_order:
 * @info: a #xfile_info_t.
 *
 * Gets the value of the sort_order attribute from the #xfile_info_t.
 * See %XFILE_ATTRIBUTE_STANDARD_SORT_ORDER.
 *
 * Returns: a #gint32 containing the value of the "standard::sort_order" attribute.
 **/
gint32
xfile_info_get_sort_order (xfile_info_t *info)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  xreturn_val_if_fail (X_IS_FILE_INFO (info), 0);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SORT_ORDER);

  value = xfile_info_find_value (info, attr);
  return _xfile_attribute_value_get_int32 (value);
}

/* Helper setters: */
/**
 * xfile_info_set_file_type:
 * @info: a #xfile_info_t.
 * @type: a #xfile_type_t.
 *
 * Sets the file type in a #xfile_info_t to @type.
 * See %XFILE_ATTRIBUTE_STANDARD_TYPE.
 **/
void
xfile_info_set_file_type (xfile_info_t *info,
			   xfile_type_t  type)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_TYPE);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_uint32 (value, type);
}

/**
 * xfile_info_set_is_hidden:
 * @info: a #xfile_info_t.
 * @is_hidden: a #xboolean_t.
 *
 * Sets the "is_hidden" attribute in a #xfile_info_t according to @is_hidden.
 * See %XFILE_ATTRIBUTE_STANDARD_IS_HIDDEN.
 **/
void
xfile_info_set_is_hidden (xfile_info_t *info,
			   xboolean_t   is_hidden)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_IS_HIDDEN);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_boolean (value, is_hidden);
}

/**
 * xfile_info_set_is_symlink:
 * @info: a #xfile_info_t.
 * @is_symlink: a #xboolean_t.
 *
 * Sets the "is_symlink" attribute in a #xfile_info_t according to @is_symlink.
 * See %XFILE_ATTRIBUTE_STANDARD_IS_SYMLINK.
 **/
void
xfile_info_set_is_symlink (xfile_info_t *info,
			    xboolean_t   is_symlink)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_IS_SYMLINK);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_boolean (value, is_symlink);
}

/**
 * xfile_info_set_name:
 * @info: a #xfile_info_t.
 * @name: (type filename): a string containing a name.
 *
 * Sets the name attribute for the current #xfile_info_t.
 * See %XFILE_ATTRIBUTE_STANDARD_NAME.
 **/
void
xfile_info_set_name (xfile_info_t  *info,
		      const char *name)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (name != NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_NAME);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_byte_string (value, name);
}

/**
 * xfile_info_set_display_name:
 * @info: a #xfile_info_t.
 * @display_name: a string containing a display name.
 *
 * Sets the display name for the current #xfile_info_t.
 * See %XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME.
 **/
void
xfile_info_set_display_name (xfile_info_t  *info,
			      const char *display_name)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (display_name != NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_string (value, display_name);
}

/**
 * xfile_info_set_edit_name:
 * @info: a #xfile_info_t.
 * @edit_name: a string containing an edit name.
 *
 * Sets the edit name for the current file.
 * See %XFILE_ATTRIBUTE_STANDARD_EDIT_NAME.
 **/
void
xfile_info_set_edit_name (xfile_info_t  *info,
			   const char *edit_name)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (edit_name != NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_EDIT_NAME);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_string (value, edit_name);
}

/**
 * xfile_info_set_icon:
 * @info: a #xfile_info_t.
 * @icon: a #xicon_t.
 *
 * Sets the icon for a given #xfile_info_t.
 * See %XFILE_ATTRIBUTE_STANDARD_ICON.
 **/
void
xfile_info_set_icon (xfile_info_t *info,
		      xicon_t     *icon)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (X_IS_ICON (icon));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_ICON);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_object (value, G_OBJECT (icon));
}

/**
 * xfile_info_set_symbolic_icon:
 * @info: a #xfile_info_t.
 * @icon: a #xicon_t.
 *
 * Sets the symbolic icon for a given #xfile_info_t.
 * See %XFILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON.
 *
 * Since: 2.34
 **/
void
xfile_info_set_symbolic_icon (xfile_info_t *info,
                               xicon_t     *icon)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (X_IS_ICON (icon));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_object (value, G_OBJECT (icon));
}

/**
 * xfile_info_set_content_type:
 * @info: a #xfile_info_t.
 * @content_type: a content type. See [GContentType][gio-GContentType]
 *
 * Sets the content type attribute for a given #xfile_info_t.
 * See %XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE.
 **/
void
xfile_info_set_content_type (xfile_info_t  *info,
			      const char *content_type)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (content_type != NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_string (value, content_type);
}

/**
 * xfile_info_set_size:
 * @info: a #xfile_info_t.
 * @size: a #xoffset_t containing the file's size.
 *
 * Sets the %XFILE_ATTRIBUTE_STANDARD_SIZE attribute in the file info
 * to the given size.
 **/
void
xfile_info_set_size (xfile_info_t *info,
		      xoffset_t    size)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SIZE);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_uint64 (value, size);
}

/**
 * xfile_info_set_modification_time:
 * @info: a #xfile_info_t.
 * @mtime: a #GTimeVal.
 *
 * Sets the %XFILE_ATTRIBUTE_TIME_MODIFIED and
 * %XFILE_ATTRIBUTE_TIME_MODIFIED_USEC attributes in the file info to the
 * given time value.
 *
 * Deprecated: 2.62: Use xfile_info_set_modification_date_time() instead, as
 *    #GTimeVal is deprecated due to the year 2038 problem.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void
xfile_info_set_modification_time (xfile_info_t *info,
				   GTimeVal  *mtime)
{
  static xuint32_t attr_mtime = 0, attr_mtime_usec;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (mtime != NULL);

  if (attr_mtime == 0)
    {
      attr_mtime = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED);
      attr_mtime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    }

  value = xfile_info_create_value (info, attr_mtime);
  if (value)
    _xfile_attribute_value_set_uint64 (value, mtime->tv_sec);
  value = xfile_info_create_value (info, attr_mtime_usec);
  if (value)
    _xfile_attribute_value_set_uint32 (value, mtime->tv_usec);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xfile_info_set_modification_date_time:
 * @info: a #xfile_info_t.
 * @mtime: (not nullable): a #xdatetime_t.
 *
 * Sets the %XFILE_ATTRIBUTE_TIME_MODIFIED and
 * %XFILE_ATTRIBUTE_TIME_MODIFIED_USEC attributes in the file info to the
 * given date/time value.
 *
 * Since: 2.62
 */
void
xfile_info_set_modification_date_time (xfile_info_t *info,
                                        xdatetime_t *mtime)
{
  static xuint32_t attr_mtime = 0, attr_mtime_usec;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (mtime != NULL);

  if (attr_mtime == 0)
    {
      attr_mtime = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED);
      attr_mtime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    }

  value = xfile_info_create_value (info, attr_mtime);
  if (value)
    _xfile_attribute_value_set_uint64 (value, xdate_time_to_unix (mtime));
  value = xfile_info_create_value (info, attr_mtime_usec);
  if (value)
    _xfile_attribute_value_set_uint32 (value, xdate_time_get_microsecond (mtime));
}

/**
 * xfile_info_set_access_date_time:
 * @info: a #xfile_info_t.
 * @atime: (not nullable): a #xdatetime_t.
 *
 * Sets the %XFILE_ATTRIBUTE_TIME_ACCESS and
 * %XFILE_ATTRIBUTE_TIME_ACCESS_USEC attributes in the file info to the
 * given date/time value.
 *
 * Since: 2.70
 */
void
xfile_info_set_access_date_time (xfile_info_t *info,
                                  xdatetime_t *atime)
{
  static xuint32_t attr_atime = 0, attr_atime_usec;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (atime != NULL);

  if (attr_atime == 0)
    {
      attr_atime = lookup_attribute (XFILE_ATTRIBUTE_TIME_ACCESS);
      attr_atime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_ACCESS_USEC);
    }

  value = xfile_info_create_value (info, attr_atime);
  if (value)
    _xfile_attribute_value_set_uint64 (value, xdate_time_to_unix (atime));
  value = xfile_info_create_value (info, attr_atime_usec);
  if (value)
    _xfile_attribute_value_set_uint32 (value, xdate_time_get_microsecond (atime));
}

/**
 * xfile_info_set_creation_date_time:
 * @info: a #xfile_info_t.
 * @creation_time: (not nullable): a #xdatetime_t.
 *
 * Sets the %XFILE_ATTRIBUTE_TIME_CREATED and
 * %XFILE_ATTRIBUTE_TIME_CREATED_USEC attributes in the file info to the
 * given date/time value.
 *
 * Since: 2.70
 */
void
xfile_info_set_creation_date_time (xfile_info_t *info,
                                    xdatetime_t *creation_time)
{
  static xuint32_t attr_ctime = 0, attr_ctime_usec;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (creation_time != NULL);

  if (attr_ctime == 0)
    {
      attr_ctime = lookup_attribute (XFILE_ATTRIBUTE_TIME_CREATED);
      attr_ctime_usec = lookup_attribute (XFILE_ATTRIBUTE_TIME_CREATED_USEC);
    }

  value = xfile_info_create_value (info, attr_ctime);
  if (value)
    _xfile_attribute_value_set_uint64 (value, xdate_time_to_unix (creation_time));
  value = xfile_info_create_value (info, attr_ctime_usec);
  if (value)
    _xfile_attribute_value_set_uint32 (value, xdate_time_get_microsecond (creation_time));
}

/**
 * xfile_info_set_symlink_target:
 * @info: a #xfile_info_t.
 * @symlink_target: a static string containing a path to a symlink target.
 *
 * Sets the %XFILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET attribute in the file info
 * to the given symlink target.
 **/
void
xfile_info_set_symlink_target (xfile_info_t  *info,
				const char *symlink_target)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));
  g_return_if_fail (symlink_target != NULL);

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_byte_string (value, symlink_target);
}

/**
 * xfile_info_set_sort_order:
 * @info: a #xfile_info_t.
 * @sort_order: a sort order integer.
 *
 * Sets the sort order attribute in the file info structure. See
 * %XFILE_ATTRIBUTE_STANDARD_SORT_ORDER.
 **/
void
xfile_info_set_sort_order (xfile_info_t *info,
			    gint32     sort_order)
{
  static xuint32_t attr = 0;
  GFileAttributeValue *value;

  g_return_if_fail (X_IS_FILE_INFO (info));

  if (attr == 0)
    attr = lookup_attribute (XFILE_ATTRIBUTE_STANDARD_SORT_ORDER);

  value = xfile_info_create_value (info, attr);
  if (value)
    _xfile_attribute_value_set_int32 (value, sort_order);
}


typedef struct {
  xuint32_t id;
  xuint32_t mask;
} SubMatcher;

struct _GFileAttributeMatcher {
  xboolean_t all;
  xint_t ref;

  xarray_t *sub_matchers;

  /* Iterator */
  xuint32_t iterator_ns;
  xint_t iterator_pos;
};

G_DEFINE_BOXED_TYPE (xfile_attribute_matcher_t, xfile_attribute_matcher,
                     xfile_attribute_matcher_ref,
                     xfile_attribute_matcher_unref)

static xint_t
compare_sub_matchers (xconstpointer a,
                      xconstpointer b)
{
  const SubMatcher *suba = a;
  const SubMatcher *subb = b;
  int diff;

  diff = suba->id - subb->id;

  if (diff)
    return diff;

  return suba->mask - subb->mask;
}

static xboolean_t
sub_matcher_matches (SubMatcher *matcher,
                     SubMatcher *submatcher)
{
  if ((matcher->mask & submatcher->mask) != matcher->mask)
    return FALSE;

  return matcher->id == (submatcher->id & matcher->mask);
}

/* Call this function after modifying a matcher.
 * It will ensure all the invariants other functions rely on.
 */
static xfile_attribute_matcher_t *
matcher_optimize (xfile_attribute_matcher_t *matcher)
{
  SubMatcher *submatcher, *compare;
  xuint_t i, j;

  /* remove sub_matchers if we match everything anyway */
  if (matcher->all)
    {
      if (matcher->sub_matchers)
        {
          g_array_free (matcher->sub_matchers, TRUE);
          matcher->sub_matchers = NULL;
        }
      return matcher;
    }

  if (matcher->sub_matchers->len == 0)
    {
      xfile_attribute_matcher_unref (matcher);
      return NULL;
    }

  /* sort sub_matchers by id (and then mask), so we can bsearch
   * and compare matchers in O(N) instead of O(N²) */
  g_array_sort (matcher->sub_matchers, compare_sub_matchers);

  /* remove duplicates and specific matches when we match the whole namespace */
  j = 0;
  compare = &g_array_index (matcher->sub_matchers, SubMatcher, j);

  for (i = 1; i < matcher->sub_matchers->len; i++)
    {
      submatcher = &g_array_index (matcher->sub_matchers, SubMatcher, i);
      if (sub_matcher_matches (compare, submatcher))
        continue;

      j++;
      compare++;

      if (j < i)
        *compare = *submatcher;
    }

  g_array_set_size (matcher->sub_matchers, j + 1);

  return matcher;
}

/**
 * xfile_attribute_matcher_new:
 * @attributes: an attribute string to match.
 *
 * Creates a new file attribute matcher, which matches attributes
 * against a given string. #GFileAttributeMatchers are reference
 * counted structures, and are created with a reference count of 1. If
 * the number of references falls to 0, the #xfile_attribute_matcher_t is
 * automatically destroyed.
 *
 * The @attributes string should be formatted with specific keys separated
 * from namespaces with a double colon. Several "namespace::key" strings may be
 * concatenated with a single comma (e.g. "standard::type,standard::is-hidden").
 * The wildcard "*" may be used to match all keys and namespaces, or
 * "namespace::*" will match all keys in a given namespace.
 *
 * ## Examples of file attribute matcher strings and results
 *
 * - `"*"`: matches all attributes.
 * - `"standard::is-hidden"`: matches only the key is-hidden in the
 *   standard namespace.
 * - `"standard::type,unix::*"`: matches the type key in the standard
 *   namespace and all keys in the unix namespace.
 *
 * Returns: a #xfile_attribute_matcher_t
 */
xfile_attribute_matcher_t *
xfile_attribute_matcher_new (const char *attributes)
{
  char **split;
  char *colon;
  int i;
  xfile_attribute_matcher_t *matcher;

  if (attributes == NULL || *attributes == '\0')
    return NULL;

  matcher = g_malloc0 (sizeof (xfile_attribute_matcher_t));
  matcher->ref = 1;
  matcher->sub_matchers = g_array_new (FALSE, FALSE, sizeof (SubMatcher));

  split = xstrsplit (attributes, ",", -1);

  for (i = 0; split[i] != NULL; i++)
    {
      if (strcmp (split[i], "*") == 0)
	matcher->all = TRUE;
      else
	{
          SubMatcher s;

	  colon = strstr (split[i], "::");
	  if (colon != NULL &&
	      !(colon[2] == 0 ||
		(colon[2] == '*' &&
		 colon[3] == 0)))
	    {
	      s.id = lookup_attribute (split[i]);
	      s.mask = 0xffffffff;
	    }
	  else
	    {
	      if (colon)
		*colon = 0;

	      s.id = lookup_namespace (split[i]) << NS_POS;
	      s.mask = NS_MASK << NS_POS;
	    }

          g_array_append_val (matcher->sub_matchers, s);
	}
    }

  xstrfreev (split);

  matcher = matcher_optimize (matcher);

  return matcher;
}

/**
 * xfile_attribute_matcher_subtract:
 * @matcher: (nullable): Matcher to subtract from
 * @subtract: (nullable): The matcher to subtract
 *
 * Subtracts all attributes of @subtract from @matcher and returns
 * a matcher that supports those attributes.
 *
 * Note that currently it is not possible to remove a single
 * attribute when the @matcher matches the whole namespace - or remove
 * a namespace or attribute when the matcher matches everything. This
 * is a limitation of the current implementation, but may be fixed
 * in the future.
 *
 * Returns: (nullable): A file attribute matcher matching all attributes of
 *     @matcher that are not matched by @subtract
 **/
xfile_attribute_matcher_t *
xfile_attribute_matcher_subtract (xfile_attribute_matcher_t *matcher,
                                   xfile_attribute_matcher_t *subtract)
{
  xfile_attribute_matcher_t *result;
  xuint_t mi, si;
  SubMatcher *msub, *ssub;

  if (matcher == NULL)
    return NULL;
  if (subtract == NULL)
    return xfile_attribute_matcher_ref (matcher);
  if (subtract->all)
    return NULL;
  if (matcher->all)
    return xfile_attribute_matcher_ref (matcher);

  result = g_malloc0 (sizeof (xfile_attribute_matcher_t));
  result->ref = 1;
  result->sub_matchers = g_array_new (FALSE, FALSE, sizeof (SubMatcher));

  si = 0;
  xassert (subtract->sub_matchers->len > 0);
  ssub = &g_array_index (subtract->sub_matchers, SubMatcher, si);

  for (mi = 0; mi < matcher->sub_matchers->len; mi++)
    {
      msub = &g_array_index (matcher->sub_matchers, SubMatcher, mi);

retry:
      if (sub_matcher_matches (ssub, msub))
        continue;

      si++;
      if (si >= subtract->sub_matchers->len)
        break;

      ssub = &g_array_index (subtract->sub_matchers, SubMatcher, si);
      if (ssub->id <= msub->id)
        goto retry;

      g_array_append_val (result->sub_matchers, *msub);
    }

  if (mi < matcher->sub_matchers->len)
    g_array_append_vals (result->sub_matchers,
                         &g_array_index (matcher->sub_matchers, SubMatcher, mi),
                         matcher->sub_matchers->len - mi);

  result = matcher_optimize (result);

  return result;
}

/**
 * xfile_attribute_matcher_ref:
 * @matcher: a #xfile_attribute_matcher_t.
 *
 * References a file attribute matcher.
 *
 * Returns: a #xfile_attribute_matcher_t.
 **/
xfile_attribute_matcher_t *
xfile_attribute_matcher_ref (xfile_attribute_matcher_t *matcher)
{
  if (matcher)
    {
      xreturn_val_if_fail (matcher->ref > 0, NULL);
      g_atomic_int_inc (&matcher->ref);
    }
  return matcher;
}

/**
 * xfile_attribute_matcher_unref:
 * @matcher: a #xfile_attribute_matcher_t.
 *
 * Unreferences @matcher. If the reference count falls below 1,
 * the @matcher is automatically freed.
 *
 **/
void
xfile_attribute_matcher_unref (xfile_attribute_matcher_t *matcher)
{
  if (matcher)
    {
      g_return_if_fail (matcher->ref > 0);

      if (g_atomic_int_dec_and_test (&matcher->ref))
	{
	  if (matcher->sub_matchers)
	    g_array_free (matcher->sub_matchers, TRUE);

	  g_free (matcher);
	}
    }
}

/**
 * xfile_attribute_matcher_matches_only:
 * @matcher: a #xfile_attribute_matcher_t.
 * @attribute: a file attribute key.
 *
 * Checks if a attribute matcher only matches a given attribute. Always
 * returns %FALSE if "*" was used when creating the matcher.
 *
 * Returns: %TRUE if the matcher only matches @attribute. %FALSE otherwise.
 **/
xboolean_t
xfile_attribute_matcher_matches_only (xfile_attribute_matcher_t *matcher,
				       const char            *attribute)
{
  SubMatcher *sub_matcher;
  xuint32_t id;

  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  if (matcher == NULL ||
      matcher->all)
    return FALSE;

  if (matcher->sub_matchers->len != 1)
    return FALSE;

  id = lookup_attribute (attribute);

  sub_matcher = &g_array_index (matcher->sub_matchers, SubMatcher, 0);

  return sub_matcher->id == id &&
         sub_matcher->mask == 0xffffffff;
}

static xboolean_t
matcher_matches_id (xfile_attribute_matcher_t *matcher,
                    xuint32_t                id)
{
  SubMatcher *sub_matchers;
  xuint_t i;

  if (matcher->sub_matchers)
    {
      sub_matchers = (SubMatcher *)matcher->sub_matchers->data;
      for (i = 0; i < matcher->sub_matchers->len; i++)
	{
	  if (sub_matchers[i].id == (id & sub_matchers[i].mask))
	    return TRUE;
	}
    }

  return FALSE;
}

xboolean_t
_xfile_attribute_matcher_matches_id (xfile_attribute_matcher_t *matcher,
                                      xuint32_t                id)
{
  /* We return a NULL matcher for an empty match string, so handle this */
  if (matcher == NULL)
    return FALSE;

  if (matcher->all)
    return TRUE;

  return matcher_matches_id (matcher, id);
}

/**
 * xfile_attribute_matcher_matches:
 * @matcher: a #xfile_attribute_matcher_t.
 * @attribute: a file attribute key.
 *
 * Checks if an attribute will be matched by an attribute matcher. If
 * the matcher was created with the "*" matching string, this function
 * will always return %TRUE.
 *
 * Returns: %TRUE if @attribute matches @matcher. %FALSE otherwise.
 **/
xboolean_t
xfile_attribute_matcher_matches (xfile_attribute_matcher_t *matcher,
				  const char            *attribute)
{
  xreturn_val_if_fail (attribute != NULL && *attribute != '\0', FALSE);

  /* We return a NULL matcher for an empty match string, so handle this */
  if (matcher == NULL)
    return FALSE;

  if (matcher->all)
    return TRUE;

  return matcher_matches_id (matcher, lookup_attribute (attribute));
}

/* return TRUE -> all */
/**
 * xfile_attribute_matcher_enumerate_namespace:
 * @matcher: a #xfile_attribute_matcher_t.
 * @ns: a string containing a file attribute namespace.
 *
 * Checks if the matcher will match all of the keys in a given namespace.
 * This will always return %TRUE if a wildcard character is in use (e.g. if
 * matcher was created with "standard::*" and @ns is "standard", or if matcher was created
 * using "*" and namespace is anything.)
 *
 * TODO: this is awkwardly worded.
 *
 * Returns: %TRUE if the matcher matches all of the entries
 * in the given @ns, %FALSE otherwise.
 **/
xboolean_t
xfile_attribute_matcher_enumerate_namespace (xfile_attribute_matcher_t *matcher,
					      const char            *ns)
{
  SubMatcher *sub_matchers;
  xuint_t ns_id;
  xuint_t i;

  xreturn_val_if_fail (ns != NULL && *ns != '\0', FALSE);

  /* We return a NULL matcher for an empty match string, so handle this */
  if (matcher == NULL)
    return FALSE;

  if (matcher->all)
    return TRUE;

  ns_id = lookup_namespace (ns) << NS_POS;

  if (matcher->sub_matchers)
    {
      sub_matchers = (SubMatcher *)matcher->sub_matchers->data;
      for (i = 0; i < matcher->sub_matchers->len; i++)
	{
	  if (sub_matchers[i].id == ns_id)
	    return TRUE;
	}
    }

  matcher->iterator_ns = ns_id;
  matcher->iterator_pos = 0;

  return FALSE;
}

/**
 * xfile_attribute_matcher_enumerate_next:
 * @matcher: a #xfile_attribute_matcher_t.
 *
 * Gets the next matched attribute from a #xfile_attribute_matcher_t.
 *
 * Returns: (nullable): a string containing the next attribute or, %NULL if
 * no more attribute exist.
 **/
const char *
xfile_attribute_matcher_enumerate_next (xfile_attribute_matcher_t *matcher)
{
  xuint_t i;
  SubMatcher *sub_matcher;

  /* We return a NULL matcher for an empty match string, so handle this */
  if (matcher == NULL)
    return NULL;

  while (1)
    {
      i = matcher->iterator_pos++;

      if (matcher->sub_matchers == NULL)
        return NULL;

      if (i < matcher->sub_matchers->len)
        sub_matcher = &g_array_index (matcher->sub_matchers, SubMatcher, i);
      else
        return NULL;

      if (sub_matcher->mask == 0xffffffff &&
	  (sub_matcher->id & (NS_MASK << NS_POS)) == matcher->iterator_ns)
	return get_attribute_for_id (sub_matcher->id);
    }
}

/**
 * xfile_attribute_matcher_to_string:
 * @matcher: (nullable): a #xfile_attribute_matcher_t.
 *
 * Prints what the matcher is matching against. The format will be
 * equal to the format passed to xfile_attribute_matcher_new().
 * The output however, might not be identical, as the matcher may
 * decide to use a different order or omit needless parts.
 *
 * Returns: a string describing the attributes the matcher matches
 *   against or %NULL if @matcher was %NULL.
 *
 * Since: 2.32
 **/
char *
xfile_attribute_matcher_to_string (xfile_attribute_matcher_t *matcher)
{
  xstring_t *string;
  xuint_t i;

  if (matcher == NULL)
    return NULL;

  if (matcher->all)
    return xstrdup ("*");

  string = xstring_new ("");
  for (i = 0; i < matcher->sub_matchers->len; i++)
    {
      SubMatcher *submatcher = &g_array_index (matcher->sub_matchers, SubMatcher, i);

      if (i > 0)
        xstring_append_c (string, ',');

      xstring_append (string, get_attribute_for_id (submatcher->id));
    }

  return xstring_free (string, FALSE);
}
