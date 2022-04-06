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

#include "config.h"

#include <string.h>

#include "gresource.h"
#include "gresourcefile.h"
#include "gfileattribute.h"
#include <gfileattribute-priv.h>
#include <gfileinfo-priv.h>
#include "gfile.h"
#include "gfilemonitor.h"
#include "gseekable.h"
#include "gfileinputstream.h"
#include "gfileinfo.h"
#include "gfileenumerator.h"
#include "gcontenttype.h"
#include "gioerror.h"
#include <glib/gstdio.h>
#include "glibintl.h"

struct _xresource_file
{
  xobject_t parent_instance;

  char *path;
};

struct _GResourceFileEnumerator
{
  xfile_enumerator_t parent;

  xfile_attribute_matcher_t *matcher;
  char *path;
  char *attributes;
  xfile_query_info_flags_t flags;
  int index;

  char **children;
};

struct _GResourceFileEnumeratorClass
{
  xfile_enumerator_class_t parent_class;
};

typedef struct _GResourceFileEnumerator        GResourceFileEnumerator;
typedef struct _GResourceFileEnumeratorClass   GResourceFileEnumeratorClass;

static void g_resource_file_file_iface_init (xfile_iface_t *iface);

static xfile_attribute_info_list_t *resource_writable_attributes = NULL;
static xfile_attribute_info_list_t *resource_writable_namespaces = NULL;

static xtype_t _xresource_file_enumerator_get_type (void);

#define XTYPE_RESOURCE_FILE_ENUMERATOR         (_xresource_file_enumerator_get_type ())
#define XRESOURCE_FILE_ENUMERATOR(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_RESOURCE_FILE_ENUMERATOR, GResourceFileEnumerator))
#define XRESOURCE_FILE_ENUMERATOR_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_RESOURCE_FILE_ENUMERATOR, GResourceFileEnumeratorClass))
#define X_IS_RESOURCE_FILE_ENUMERATOR(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_RESOURCE_FILE_ENUMERATOR))
#define X_IS_RESOURCE_FILE_ENUMERATOR_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_RESOURCE_FILE_ENUMERATOR))
#define XRESOURCE_FILE_ENUMERATOR_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_RESOURCE_FILE_ENUMERATOR, GResourceFileEnumeratorClass))

#define XTYPE_RESOURCE_FILE_INPUT_STREAM         (_xresource_file_input_stream_get_type ())
#define XRESOURCE_FILE_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_RESOURCE_FILE_INPUT_STREAM, GResourceFileInputStream))
#define XRESOURCE_FILE_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_RESOURCE_FILE_INPUT_STREAM, GResourceFileInputStreamClass))
#define X_IS_RESOURCE_FILE_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_RESOURCE_FILE_INPUT_STREAM))
#define X_IS_RESOURCE_FILE_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_RESOURCE_FILE_INPUT_STREAM))
#define XRESOURCE_FILE_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_RESOURCE_FILE_INPUT_STREAM, GResourceFileInputStreamClass))

typedef struct _GResourceFileInputStream         GResourceFileInputStream;
typedef struct _GResourceFileInputStreamClass    GResourceFileInputStreamClass;

#define g_resource_file_get_type _xresource_file_get_type
G_DEFINE_TYPE_WITH_CODE (xresource_file_t, g_resource_file, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_FILE,
						g_resource_file_file_iface_init))

#define g_resource_file_enumerator_get_type _xresource_file_enumerator_get_type
G_DEFINE_TYPE (GResourceFileEnumerator, g_resource_file_enumerator, XTYPE_FILE_ENUMERATOR)

static xfile_enumerator_t *_xresource_file_enumerator_new (xresource_file_t *file,
							 const char           *attributes,
							 xfile_query_info_flags_t   flags,
							 xcancellable_t         *cancellable,
							 xerror_t              **error);


static xtype_t              _xresource_file_input_stream_get_type (void) G_GNUC_CONST;

static xfile_input_stream_t *_xresource_file_input_stream_new (xinput_stream_t *stream, xfile_t *file);


static void
g_resource_file_finalize (xobject_t *object)
{
  xresource_file_t *resource;

  resource = XRESOURCE_FILE (object);

  g_free (resource->path);

  G_OBJECT_CLASS (g_resource_file_parent_class)->finalize (object);
}

static void
g_resource_file_class_init (xresource_file_class_t *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_resource_file_finalize;

  resource_writable_attributes = xfile_attribute_info_list_new ();
  resource_writable_namespaces = xfile_attribute_info_list_new ();
}

static void
g_resource_file_init (xresource_file_t *resource)
{
}

static inline xchar_t *
scan_backwards (const xchar_t *begin,
                const xchar_t *end,
                xchar_t        c)
{
  while (end >= begin)
    {
      if (*end == c)
        return (xchar_t *)end;
      end--;
    }

  return NULL;
}

static inline void
pop_to_previous_part (const xchar_t  *begin,
                      xchar_t       **out)
{
  if (*out > begin)
    *out = scan_backwards (begin, *out - 1, '/');
}

/*
 * canonicalize_filename:
 * @in: the path to be canonicalized
 *
 * The path @in may contain non-canonical path pieces such as "../"
 * or duplicated "/". This will resolve those into a form that only
 * contains a single / at a time and resolves all "../". The resulting
 * path must also start with a /.
 *
 * Returns: the canonical form of the path
 */
static char *
canonicalize_filename (const char *in)
{
  xchar_t *bptr;
  char *out;

  bptr = out = g_malloc (strlen (in) + 2);
  *out = '/';

  while (*in != 0)
    {
      g_assert (*out == '/');

      /* move past slashes */
      while (*in == '/')
        in++;

      /* Handle ./ ../ .\0 ..\0 */
      if (*in == '.')
        {
          /* If this is ../ or ..\0 move up */
          if (in[1] == '.' && (in[2] == '/' || in[2] == 0))
            {
              pop_to_previous_part (bptr, &out);
              in += 2;
              continue;
            }

          /* If this is ./ skip past it */
          if (in[1] == '/' || in[1] == 0)
            {
              in += 1;
              continue;
            }
        }

      /* Scan to the next path piece */
      while (*in != 0 && *in != '/')
        *(++out) = *(in++);

      /* Add trailing /, compress the rest on the next go round. */
      if (*in == '/')
        *(++out) = *(in++);
    }

  /* Trim trailing / from path */
  if (out > bptr && *out == '/')
    *out = 0;
  else
    *(++out) = 0;

  return bptr;
}

static xfile_t *
g_resource_file_new_for_path (const char *path)
{
  xresource_file_t *resource = xobject_new (XTYPE_RESOURCE_FILE, NULL);

  resource->path = canonicalize_filename (path);

  return XFILE (resource);
}

xfile_t *
_xresource_file_new (const char *uri)
{
  xfile_t *resource;
  char *path;

  path = xuri_unescape_string (uri + strlen ("resource:"), NULL);
  resource = g_resource_file_new_for_path (path);
  g_free (path);

  return XFILE (resource);
}

static xboolean_t
g_resource_file_is_native (xfile_t *file)
{
  return FALSE;
}

static xboolean_t
g_resource_file_has_uri_scheme (xfile_t      *file,
				const char *uri_scheme)
{
  return g_ascii_strcasecmp (uri_scheme, "resource") == 0;
}

static char *
g_resource_file_get_uri_scheme (xfile_t *file)
{
  return xstrdup ("resource");
}

static char *
g_resource_file_get_basename (xfile_t *file)
{
  xchar_t *base;

  base = strrchr (XRESOURCE_FILE (file)->path, '/');
  return xstrdup (base + 1);
}

static char *
g_resource_file_get_path (xfile_t *file)
{
  return NULL;
}

static char *
g_resource_file_get_uri (xfile_t *file)
{
  char *escaped, *res;
  escaped = xuri_escape_string (XRESOURCE_FILE (file)->path, XURI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
  res = xstrconcat ("resource://", escaped, NULL);
  g_free (escaped);
  return res;
}

static char *
g_resource_file_get_parse_name (xfile_t *file)
{
  return g_resource_file_get_uri (file);
}

static xfile_t *
g_resource_file_get_parent (xfile_t *file)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);
  xresource_file_t *parent;
  xchar_t *end;

  end = strrchr (resource->path, '/');

  if (end == XRESOURCE_FILE (file)->path)
    return NULL;

  parent = xobject_new (XTYPE_RESOURCE_FILE, NULL);
  parent->path = xstrndup (resource->path,
			    end - resource->path);

  return XFILE (parent);
}

static xfile_t *
g_resource_file_dup (xfile_t *file)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);

  return g_resource_file_new_for_path (resource->path);
}

static xuint_t
g_resource_file_hash (xfile_t *file)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);

  return xstr_hash (resource->path);
}

static xboolean_t
g_resource_file_equal (xfile_t *file1,
		       xfile_t *file2)
{
  xresource_file_t *resource1 = XRESOURCE_FILE (file1);
  xresource_file_t *resource2 = XRESOURCE_FILE (file2);

  return xstr_equal (resource1->path, resource2->path);
}

static const char *
match_prefix (const char *path,
	      const char *prefix)
{
  int prefix_len;

  prefix_len = strlen (prefix);
  if (strncmp (path, prefix, prefix_len) != 0)
    return NULL;

  /* Handle the case where prefix is the root, so that
   * the IS_DIR_SEPRARATOR check below works */
  if (prefix_len > 0 &&
      prefix[prefix_len-1] == '/')
    prefix_len--;

  return path + prefix_len;
}

static xboolean_t
g_resource_file_prefix_matches (xfile_t *parent,
				xfile_t *descendant)
{
  xresource_file_t *parent_resource = XRESOURCE_FILE (parent);
  xresource_file_t *descendant_resource = XRESOURCE_FILE (descendant);
  const char *remainder;

  remainder = match_prefix (descendant_resource->path, parent_resource->path);
  if (remainder != NULL && *remainder == '/')
    return TRUE;
  return FALSE;
}

static char *
g_resource_file_get_relative_path (xfile_t *parent,
				   xfile_t *descendant)
{
  xresource_file_t *parent_resource = XRESOURCE_FILE (parent);
  xresource_file_t *descendant_resource = XRESOURCE_FILE (descendant);
  const char *remainder;

  remainder = match_prefix (descendant_resource->path, parent_resource->path);

  if (remainder != NULL && *remainder == '/')
    return xstrdup (remainder + 1);
  return NULL;
}

static xfile_t *
g_resource_file_resolve_relative_path (xfile_t      *file,
				       const char *relative_path)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);
  char *filename;
  xfile_t *child;

  if (relative_path[0] == '/')
    return g_resource_file_new_for_path (relative_path);

  filename = g_build_path ("/", resource->path, relative_path, NULL);
  child = g_resource_file_new_for_path (filename);
  g_free (filename);

  return child;
}

static xfile_enumerator_t *
g_resource_file_enumerate_children (xfile_t                *file,
				    const char           *attributes,
				    xfile_query_info_flags_t   flags,
				    xcancellable_t         *cancellable,
				    xerror_t              **error)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);
  return _xresource_file_enumerator_new (resource,
					  attributes, flags,
					  cancellable, error);
}

static xfile_t *
g_resource_file_get_child_for_display_name (xfile_t        *file,
					    const char   *display_name,
					    xerror_t      **error)
{
  xfile_t *new_file;

  new_file = xfile_get_child (file, display_name);

  return new_file;
}

static xfile_info_t *
g_resource_file_query_info (xfile_t                *file,
			    const char           *attributes,
			    xfile_query_info_flags_t   flags,
			    xcancellable_t         *cancellable,
			    xerror_t              **error)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);
  xerror_t *my_error = NULL;
  xfile_info_t *info;
  xfile_attribute_matcher_t *matcher;
  xboolean_t res;
  xsize_t size;
  xuint32_t resource_flags;
  char **children;
  xboolean_t is_dir;
  char *base;

  is_dir = FALSE;
  children = g_resources_enumerate_children (resource->path, 0, NULL);
  if (children != NULL)
    {
      xstrfreev (children);
      is_dir = TRUE;
    }

  /* root is always there */
  if (strcmp ("/", resource->path) == 0)
    is_dir = TRUE;

  if (!is_dir)
    {
      res = g_resources_get_info (resource->path, 0, &size, &resource_flags, &my_error);
      if (!res)
	{
	  if (xerror_matches (my_error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
	    {
	      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			   _("The resource at “%s” does not exist"),
			   resource->path);
	    }
	  else
	    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                                 my_error->message);
	  g_clear_error (&my_error);
	  return FALSE;
	}
    }

  matcher = xfile_attribute_matcher_new (attributes);

  info = xfile_info_new ();
  base = g_resource_file_get_basename (file);
  xfile_info_set_name (info, base);
  xfile_info_set_display_name (info, base);

  _xfile_info_set_attribute_boolean_by_id (info, XFILE_ATTRIBUTE_ID_ACCESS_CAN_READ, TRUE);
  _xfile_info_set_attribute_boolean_by_id (info, XFILE_ATTRIBUTE_ID_ACCESS_CAN_WRITE, FALSE);
  _xfile_info_set_attribute_boolean_by_id (info, XFILE_ATTRIBUTE_ID_ACCESS_CAN_EXECUTE, FALSE);
  _xfile_info_set_attribute_boolean_by_id (info, XFILE_ATTRIBUTE_ID_ACCESS_CAN_RENAME, FALSE);
  _xfile_info_set_attribute_boolean_by_id (info, XFILE_ATTRIBUTE_ID_ACCESS_CAN_DELETE, FALSE);
  _xfile_info_set_attribute_boolean_by_id (info, XFILE_ATTRIBUTE_ID_ACCESS_CAN_TRASH, FALSE);

  if (is_dir)
    {
      xfile_info_set_file_type (info, XFILE_TYPE_DIRECTORY);
    }
  else
    {
      xbytes_t *bytes;
      char *content_type;

      xfile_info_set_file_type (info, XFILE_TYPE_REGULAR);
      xfile_info_set_size (info, size);

      if ((_xfile_attribute_matcher_matches_id (matcher, XFILE_ATTRIBUTE_ID_STANDARD_CONTENT_TYPE) ||
           ((~resource_flags & G_RESOURCE_FLAGS_COMPRESSED) &&
            _xfile_attribute_matcher_matches_id (matcher, XFILE_ATTRIBUTE_ID_STANDARD_FAST_CONTENT_TYPE))) &&
          (bytes = g_resources_lookup_data (resource->path, 0, NULL)))
        {
          const guchar *data;
          xsize_t data_size;

          data = xbytes_get_data (bytes, &data_size);
          content_type = g_content_type_guess (base, data, data_size, NULL);

          xbytes_unref (bytes);
        }
      else
        content_type = NULL;

      if (content_type)
        {
          _xfile_info_set_attribute_string_by_id (info, XFILE_ATTRIBUTE_ID_STANDARD_CONTENT_TYPE, content_type);
          _xfile_info_set_attribute_string_by_id (info, XFILE_ATTRIBUTE_ID_STANDARD_FAST_CONTENT_TYPE, content_type);

          g_free (content_type);
        }
    }

  g_free (base);
  xfile_attribute_matcher_unref (matcher);

  return info;
}

static xfile_info_t *
g_resource_file_query_filesystem_info (xfile_t         *file,
                                       const char    *attributes,
                                       xcancellable_t  *cancellable,
                                       xerror_t       **error)
{
  xfile_info_t *info;
  xfile_attribute_matcher_t *matcher;

  info = xfile_info_new ();

  matcher = xfile_attribute_matcher_new (attributes);
  if (xfile_attribute_matcher_matches (matcher, XFILE_ATTRIBUTE_FILESYSTEM_TYPE))
    xfile_info_set_attribute_string (info, XFILE_ATTRIBUTE_FILESYSTEM_TYPE, "resource");

  if (xfile_attribute_matcher_matches (matcher, XFILE_ATTRIBUTE_FILESYSTEM_READONLY))    xfile_info_set_attribute_boolean (info, XFILE_ATTRIBUTE_FILESYSTEM_READONLY, TRUE);

  xfile_attribute_matcher_unref (matcher);

  return info;
}

static xfile_attribute_info_list_t *
g_resource_file_query_settable_attributes (xfile_t         *file,
					   xcancellable_t  *cancellable,
					   xerror_t       **error)
{
  return xfile_attribute_info_list_ref (resource_writable_attributes);
}

static xfile_attribute_info_list_t *
g_resource_file_query_writable_namespaces (xfile_t         *file,
					   xcancellable_t  *cancellable,
					   xerror_t       **error)
{
  return xfile_attribute_info_list_ref (resource_writable_namespaces);
}

static xfile_input_stream_t *
g_resource_file_read (xfile_t         *file,
		      xcancellable_t  *cancellable,
		      xerror_t       **error)
{
  xresource_file_t *resource = XRESOURCE_FILE (file);
  xerror_t *my_error = NULL;
  xinput_stream_t *stream;
  xfile_input_stream_t *res;

  stream = g_resources_open_stream (resource->path, 0, &my_error);

  if (stream == NULL)
    {
      if (xerror_matches (my_error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
	{
	  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
		       _("The resource at “%s” does not exist"),
		       resource->path);
	}
      else
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             my_error->message);
      g_clear_error (&my_error);
      return NULL;
    }

  res = _xresource_file_input_stream_new (stream, file);
  xobject_unref (stream);
  return res;
}

typedef xfile_monitor_t GResourceFileMonitor;
typedef xfile_monitor_class_t GResourceFileMonitorClass;

xtype_t g_resource_file_monitor_get_type (void);

G_DEFINE_TYPE (GResourceFileMonitor, g_resource_file_monitor, XTYPE_FILE_MONITOR)

static xboolean_t
g_resource_file_monitor_cancel (xfile_monitor_t *monitor)
{
  return TRUE;
}

static void
g_resource_file_monitor_init (GResourceFileMonitor *monitor)
{
}

static void
g_resource_file_monitor_class_init (GResourceFileMonitorClass *class)
{
  class->cancel = g_resource_file_monitor_cancel;
}

static xfile_monitor_t *
g_resource_file_monitor_file (xfile_t              *file,
                              xfile_monitor_flags_t   flags,
                              xcancellable_t       *cancellable,
                              xerror_t            **error)
{
  return xobject_new (g_resource_file_monitor_get_type (), NULL);
}

static void
g_resource_file_file_iface_init (xfile_iface_t *iface)
{
  iface->dup = g_resource_file_dup;
  iface->hash = g_resource_file_hash;
  iface->equal = g_resource_file_equal;
  iface->is_native = g_resource_file_is_native;
  iface->has_uri_scheme = g_resource_file_has_uri_scheme;
  iface->get_uri_scheme = g_resource_file_get_uri_scheme;
  iface->get_basename = g_resource_file_get_basename;
  iface->get_path = g_resource_file_get_path;
  iface->get_uri = g_resource_file_get_uri;
  iface->get_parse_name = g_resource_file_get_parse_name;
  iface->get_parent = g_resource_file_get_parent;
  iface->prefix_matches = g_resource_file_prefix_matches;
  iface->get_relative_path = g_resource_file_get_relative_path;
  iface->resolve_relative_path = g_resource_file_resolve_relative_path;
  iface->get_child_for_display_name = g_resource_file_get_child_for_display_name;
  iface->enumerate_children = g_resource_file_enumerate_children;
  iface->query_info = g_resource_file_query_info;
  iface->query_filesystem_info = g_resource_file_query_filesystem_info;
  iface->query_settable_attributes = g_resource_file_query_settable_attributes;
  iface->query_writable_namespaces = g_resource_file_query_writable_namespaces;
  iface->read_fn = g_resource_file_read;
  iface->monitor_file = g_resource_file_monitor_file;

  iface->supports_thread_contexts = TRUE;
}

static xfile_info_t *g_resource_file_enumerator_next_file (xfile_enumerator_t  *enumerator,
							xcancellable_t     *cancellable,
							xerror_t          **error);
static xboolean_t   g_resource_file_enumerator_close     (xfile_enumerator_t  *enumerator,
							xcancellable_t     *cancellable,
							xerror_t          **error);

static void
g_resource_file_enumerator_finalize (xobject_t *object)
{
  GResourceFileEnumerator *resource;

  resource = XRESOURCE_FILE_ENUMERATOR (object);

  xstrfreev (resource->children);
  g_free (resource->path);
  g_free (resource->attributes);

  G_OBJECT_CLASS (g_resource_file_enumerator_parent_class)->finalize (object);
}

static void
g_resource_file_enumerator_class_init (GResourceFileEnumeratorClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  xfile_enumerator_class_t *enumerator_class = XFILE_ENUMERATOR_CLASS (klass);

  gobject_class->finalize = g_resource_file_enumerator_finalize;

  enumerator_class->next_file = g_resource_file_enumerator_next_file;
  enumerator_class->close_fn = g_resource_file_enumerator_close;
}

static void
g_resource_file_enumerator_init (GResourceFileEnumerator *resource)
{
}

static xfile_enumerator_t *
_xresource_file_enumerator_new (xresource_file_t *file,
				 const char           *attributes,
				 xfile_query_info_flags_t   flags,
				 xcancellable_t         *cancellable,
				 xerror_t              **error)
{
  GResourceFileEnumerator *resource;
  char **children;
  xboolean_t res;

  children = g_resources_enumerate_children (file->path, 0, NULL);
  if (children == NULL &&
      strcmp ("/", file->path) != 0)
    {
      res = g_resources_get_info (file->path, 0, NULL, NULL, NULL);
      if (res)
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY,
		     _("The resource at “%s” is not a directory"),
		     file->path);
      else
	g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
		     _("The resource at “%s” does not exist"),
		     file->path);
      return NULL;
    }

  resource = xobject_new (XTYPE_RESOURCE_FILE_ENUMERATOR,
			   "container", file,
			   NULL);

  resource->children = children;
  resource->path = xstrdup (file->path);
  resource->attributes = xstrdup (attributes);
  resource->flags = flags;

  return XFILE_ENUMERATOR (resource);
}

static xfile_info_t *
g_resource_file_enumerator_next_file (xfile_enumerator_t  *enumerator,
				      xcancellable_t     *cancellable,
				      xerror_t          **error)
{
  GResourceFileEnumerator *resource = XRESOURCE_FILE_ENUMERATOR (enumerator);
  char *path;
  xfile_info_t *info;
  xfile_t *file;

  if (resource->children == NULL ||
      resource->children[resource->index] == NULL)
    return NULL;

  path = g_build_path ("/", resource->path, resource->children[resource->index++], NULL);
  file = g_resource_file_new_for_path (path);
  g_free (path);

  info = xfile_query_info (file,
			    resource->attributes,
			    resource->flags,
			    cancellable,
			    error);

  xobject_unref (file);

  return info;
}

static xboolean_t
g_resource_file_enumerator_close (xfile_enumerator_t  *enumerator,
			       xcancellable_t     *cancellable,
			       xerror_t          **error)
{
  return TRUE;
}


struct _GResourceFileInputStream
{
  xfile_input_stream_t parent_instance;
  xinput_stream_t *stream;
  xfile_t *file;
};

struct _GResourceFileInputStreamClass
{
  GFileInputStreamClass parent_class;
};

#define g_resource_file_input_stream_get_type _xresource_file_input_stream_get_type
G_DEFINE_TYPE (GResourceFileInputStream, g_resource_file_input_stream, XTYPE_FILE_INPUT_STREAM)

static xssize_t     g_resource_file_input_stream_read       (xinput_stream_t      *stream,
							   void              *buffer,
							   xsize_t              count,
							   xcancellable_t      *cancellable,
							   xerror_t           **error);
static xssize_t     g_resource_file_input_stream_skip       (xinput_stream_t      *stream,
							   xsize_t              count,
							   xcancellable_t      *cancellable,
							   xerror_t           **error);
static xboolean_t   g_resource_file_input_stream_close      (xinput_stream_t      *stream,
							   xcancellable_t      *cancellable,
							   xerror_t           **error);
static xoffset_t    g_resource_file_input_stream_tell       (xfile_input_stream_t  *stream);
static xboolean_t   g_resource_file_input_stream_can_seek   (xfile_input_stream_t  *stream);
static xboolean_t   g_resource_file_input_stream_seek       (xfile_input_stream_t  *stream,
							   xoffset_t            offset,
							   GSeekType          type,
							   xcancellable_t      *cancellable,
							   xerror_t           **error);
static xfile_info_t *g_resource_file_input_stream_query_info (xfile_input_stream_t  *stream,
							   const char        *attributes,
							   xcancellable_t      *cancellable,
							   xerror_t           **error);

static void
g_resource_file_input_stream_finalize (xobject_t *object)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (object);

  xobject_unref (file->stream);
  xobject_unref (file->file);
  G_OBJECT_CLASS (g_resource_file_input_stream_parent_class)->finalize (object);
}

static void
g_resource_file_input_stream_class_init (GResourceFileInputStreamClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *stream_class = G_INPUT_STREAM_CLASS (klass);
  GFileInputStreamClass *file_stream_class = XFILE_INPUT_STREAM_CLASS (klass);

  gobject_class->finalize = g_resource_file_input_stream_finalize;

  stream_class->read_fn = g_resource_file_input_stream_read;
  stream_class->skip = g_resource_file_input_stream_skip;
  stream_class->close_fn = g_resource_file_input_stream_close;
  file_stream_class->tell = g_resource_file_input_stream_tell;
  file_stream_class->can_seek = g_resource_file_input_stream_can_seek;
  file_stream_class->seek = g_resource_file_input_stream_seek;
  file_stream_class->query_info = g_resource_file_input_stream_query_info;
}

static void
g_resource_file_input_stream_init (GResourceFileInputStream *info)
{
}

static xfile_input_stream_t *
_xresource_file_input_stream_new (xinput_stream_t *in_stream, xfile_t *file)
{
  GResourceFileInputStream *stream;

  stream = xobject_new (XTYPE_RESOURCE_FILE_INPUT_STREAM, NULL);
  stream->stream = xobject_ref (in_stream);
  stream->file = xobject_ref (file);

  return XFILE_INPUT_STREAM (stream);
}

static xssize_t
g_resource_file_input_stream_read (xinput_stream_t  *stream,
				   void          *buffer,
				   xsize_t          count,
				   xcancellable_t  *cancellable,
				   xerror_t       **error)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);
  return xinput_stream_read (file->stream,
			      buffer, count, cancellable, error);
}

static xssize_t
g_resource_file_input_stream_skip (xinput_stream_t  *stream,
				   xsize_t          count,
				   xcancellable_t  *cancellable,
				   xerror_t       **error)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);
  return xinput_stream_skip (file->stream,
			      count, cancellable, error);
}

static xboolean_t
g_resource_file_input_stream_close (xinput_stream_t  *stream,
				    xcancellable_t  *cancellable,
				    xerror_t       **error)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);
  return xinput_stream_close (file->stream,
			       cancellable, error);
}


static xoffset_t
g_resource_file_input_stream_tell (xfile_input_stream_t *stream)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);

  if (!X_IS_SEEKABLE (file->stream))
      return 0;

  return xseekable_tell (G_SEEKABLE (file->stream));
}

static xboolean_t
g_resource_file_input_stream_can_seek (xfile_input_stream_t *stream)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);

  return X_IS_SEEKABLE (file->stream) && xseekable_can_seek (G_SEEKABLE (file->stream));
}

static xboolean_t
g_resource_file_input_stream_seek (xfile_input_stream_t  *stream,
				   xoffset_t            offset,
				   GSeekType          type,
				   xcancellable_t      *cancellable,
				   xerror_t           **error)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);

  if (!X_IS_SEEKABLE (file->stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			   _("Input stream doesn’t implement seek"));
      return FALSE;
    }

  return xseekable_seek (G_SEEKABLE (file->stream),
			  offset, type, cancellable, error);
}

static xfile_info_t *
g_resource_file_input_stream_query_info (xfile_input_stream_t  *stream,
					 const char        *attributes,
					 xcancellable_t      *cancellable,
					 xerror_t           **error)
{
  GResourceFileInputStream *file = XRESOURCE_FILE_INPUT_STREAM (stream);

  return xfile_query_info (file->file, attributes, 0, cancellable, error);
}
