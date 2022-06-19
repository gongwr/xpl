/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2011 Red Hat, Inc
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
 * Authors: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include <string.h>

#include "gresource.h"
#include <gvdb/gvdb-reader.h>
#include <gi18n-lib.h>
#include <gstdio.h>
#include <gio/gfile.h>
#include <gio/gioerror.h>
#include <gio/gmemoryinputstream.h>
#include <gio/gzlibdecompressor.h>
#include <gio/gconverterinputstream.h>

#include "glib-private.h"

struct _GResource
{
  int ref_count;

  GvdbTable *table;
};

static void register_lazy_static_resources (void);

G_DEFINE_BOXED_TYPE (xresource_t, g_resource, g_resource_ref, g_resource_unref)

/**
 * SECTION:gresource
 * @short_description: Resource framework
 * @include: gio/gio.h
 *
 * Applications and libraries often contain binary or textual data that is
 * really part of the application, rather than user data. For instance
 * #GtkBuilder .ui files, splashscreen images, xmenu_t markup XML, CSS files,
 * icons, etc. These are often shipped as files in `$datadir/appname`, or
 * manually included as literal strings in the code.
 *
 * The #xresource_t API and the [glib-compile-resources][glib-compile-resources] program
 * provide a convenient and efficient alternative to this which has some nice properties. You
 * maintain the files as normal files, so its easy to edit them, but during the build the files
 * are combined into a binary bundle that is linked into the executable. This means that loading
 * the resource files are efficient (as they are already in memory, shared with other instances) and
 * simple (no need to check for things like I/O errors or locate the files in the filesystem). It
 * also makes it easier to create relocatable applications.
 *
 * Resource files can also be marked as compressed. Such files will be included in the resource bundle
 * in a compressed form, but will be automatically uncompressed when the resource is used. This
 * is very useful e.g. for larger text files that are parsed once (or rarely) and then thrown away.
 *
 * Resource files can also be marked to be preprocessed, by setting the value of the
 * `preprocess` attribute to a comma-separated list of preprocessing options.
 * The only options currently supported are:
 *
 * `xml-stripblanks` which will use the xmllint command
 * to strip ignorable whitespace from the XML file. For this to work,
 * the `XMLLINT` environment variable must be set to the full path to
 * the xmllint executable, or xmllint must be in the `PATH`; otherwise
 * the preprocessing step is skipped.
 *
 * `to-pixdata` (deprecated since gdk-pixbuf 2.32) which will use the
 * `gdk-pixbuf-pixdata` command to convert images to the #GdkPixdata format,
 * which allows you to create pixbufs directly using the data inside the
 * resource file, rather than an (uncompressed) copy of it. For this, the
 * `gdk-pixbuf-pixdata` program must be in the `PATH`, or the
 * `GDK_PIXBUF_PIXDATA` environment variable must be set to the full path to the
 * `gdk-pixbuf-pixdata` executable; otherwise the resource compiler will abort.
 * `to-pixdata` has been deprecated since gdk-pixbuf 2.32, as #xresource_t
 * supports embedding modern image formats just as well. Instead of using it,
 * embed a PNG or SVG file in your #xresource_t.
 *
 * `json-stripblanks` which will use the `json-glib-format` command to strip
 * ignorable whitespace from the JSON file. For this to work, the
 * `JSON_XPL_FORMAT` environment variable must be set to the full path to the
 * `json-glib-format` executable, or it must be in the `PATH`;
 * otherwise the preprocessing step is skipped. In addition, at least version
 * 1.6 of `json-glib-format` is required.
 *
 * Resource files will be exported in the xresource_t namespace using the
 * combination of the given `prefix` and the filename from the `file` element.
 * The `alias` attribute can be used to alter the filename to expose them at a
 * different location in the resource namespace. Typically, this is used to
 * include files from a different source directory without exposing the source
 * directory in the resource namespace, as in the example below.
 *
 * Resource bundles are created by the [glib-compile-resources][glib-compile-resources] program
 * which takes an XML file that describes the bundle, and a set of files that the XML references. These
 * are combined into a binary resource bundle.
 *
 * An example resource description:
 * |[
 * <?xml version="1.0" encoding="UTF-8"?>
 * <gresources>
 *   <gresource prefix="/org/gtk/Example">
 *     <file>data/splashscreen.png</file>
 *     <file compressed="true">dialog.ui</file>
 *     <file preprocess="xml-stripblanks">menumarkup.xml</file>
 *     <file alias="example.css">data/example.css</file>
 *   </gresource>
 * </gresources>
 * ]|
 *
 * This will create a resource bundle with the following files:
 * |[
 * /org/gtk/Example/data/splashscreen.png
 * /org/gtk/Example/dialog.ui
 * /org/gtk/Example/menumarkup.xml
 * /org/gtk/Example/example.css
 * ]|
 *
 * Note that all resources in the process share the same namespace, so use Java-style
 * path prefixes (like in the above example) to avoid conflicts.
 *
 * You can then use [glib-compile-resources][glib-compile-resources] to compile the XML to a
 * binary bundle that you can load with g_resource_load(). However, its more common to use the --generate-source and
 * --generate-header arguments to create a source file and header to link directly into your application.
 * This will generate `get_resource()`, `register_resource()` and
 * `unregister_resource()` functions, prefixed by the `--c-name` argument passed
 * to [glib-compile-resources][glib-compile-resources]. `get_resource()` returns
 * the generated #xresource_t object. The register and unregister functions
 * register the resource so its files can be accessed using
 * g_resources_lookup_data().
 *
 * Once a #xresource_t has been created and registered all the data in it can be accessed globally in the process by
 * using API calls like g_resources_open_stream() to stream the data or g_resources_lookup_data() to get a direct pointer
 * to the data. You can also use URIs like "resource:///org/gtk/Example/data/splashscreen.png" with #xfile_t to access
 * the resource data.
 *
 * Some higher-level APIs, such as #GtkApplication, will automatically load
 * resources from certain well-known paths in the resource namespace as a
 * convenience. See the documentation for those APIs for details.
 *
 * There are two forms of the generated source, the default version uses the compiler support for constructor
 * and destructor functions (where available) to automatically create and register the #xresource_t on startup
 * or library load time. If you pass `--manual-register`, two functions to register/unregister the resource are created
 * instead. This requires an explicit initialization call in your application/library, but it works on all platforms,
 * even on the minor ones where constructors are not supported. (Constructor support is available for at least Win32, Mac OS and Linux.)
 *
 * Note that resource data can point directly into the data segment of e.g. a library, so if you are unloading libraries
 * during runtime you need to be very careful with keeping around pointers to data from a resource, as this goes away
 * when the library is unloaded. However, in practice this is not generally a problem, since most resource accesses
 * are for your own resources, and resource data is often used once, during parsing, and then released.
 *
 * When debugging a program or testing a change to an installed version, it is often useful to be able to
 * replace resources in the program or library, without recompiling, for debugging or quick hacking and testing
 * purposes. Since GLib 2.50, it is possible to use the `G_RESOURCE_OVERLAYS` environment variable to selectively overlay
 * resources with replacements from the filesystem.  It is a %G_SEARCHPATH_SEPARATOR-separated list of substitutions to perform
 * during resource lookups. It is ignored when running in a setuid process.
 *
 * A substitution has the form
 *
 * |[
 *    /org/gtk/libgtk=/home/desrt/gtk-overlay
 * ]|
 *
 * The part before the `=` is the resource subpath for which the overlay applies.  The part after is a
 * filesystem path which contains files and subdirectories as you would like to be loaded as resources with the
 * equivalent names.
 *
 * In the example above, if an application tried to load a resource with the resource path
 * `/org/gtk/libgtk/ui/gtkdialog.ui` then xresource_t would check the filesystem path
 * `/home/desrt/gtk-overlay/ui/gtkdialog.ui`.  If a file was found there, it would be used instead.  This is an
 * overlay, not an outright replacement, which means that if a file is not found at that path, the built-in
 * version will be used instead.  Whiteouts are not currently supported.
 *
 * Substitutions must start with a slash, and must not contain a trailing slash before the '='.  The path after
 * the slash should ideally be absolute, but this is not strictly required.  It is possible to overlay the
 * location of a single resource with an individual file.
 *
 * Since: 2.32
 */

/**
 * GStaticResource:
 *
 * #GStaticResource is an opaque data structure and can only be accessed
 * using the following functions.
 **/
typedef xboolean_t (* CheckCandidate) (const xchar_t *candidate, xpointer_t user_data);

static xboolean_t
open_overlay_stream (const xchar_t *candidate,
                     xpointer_t     user_data)
{
  xinput_stream_t **res = (xinput_stream_t **) user_data;
  xerror_t *error = NULL;
  xfile_t *file;

  file = xfile_new_for_path (candidate);
  *res = (xinput_stream_t *) xfile_read (file, NULL, &error);

  if (*res)
    {
      g_message ("Opened file '%s' as a resource overlay", candidate);
    }
  else
    {
      if (!xerror_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        g_warning ("Can't open overlay file '%s': %s", candidate, error->message);
      xerror_free (error);
    }

  xobject_unref (file);

  return *res != NULL;
}

static xboolean_t
get_overlay_bytes (const xchar_t *candidate,
                   xpointer_t     user_data)
{
  xbytes_t **res = (xbytes_t **) user_data;
  xmapped_file_t *mapped_file;
  xerror_t *error = NULL;

  mapped_file = xmapped_file_new (candidate, FALSE, &error);

  if (mapped_file)
    {
      g_message ("Mapped file '%s' as a resource overlay", candidate);
      *res = xmapped_file_get_bytes (mapped_file);
      xmapped_file_unref (mapped_file);
    }
  else
    {
      if (!xerror_matches (error, XFILE_ERROR, XFILE_ERROR_NOENT))
        g_warning ("Can't mmap overlay file '%s': %s", candidate, error->message);
      xerror_free (error);
    }

  return *res != NULL;
}

static xboolean_t
enumerate_overlay_dir (const xchar_t *candidate,
                       xpointer_t     user_data)
{
  xhashtable_t **hash = (xhashtable_t **) user_data;
  xerror_t *error = NULL;
  xdir_t *dir;
  const xchar_t *name;

  dir = g_dir_open (candidate, 0, &error);
  if (dir)
    {
      if (*hash == NULL)
        /* note: keep in sync with same line below */
        *hash = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);

      g_message ("Enumerating directory '%s' as resource overlay", candidate);

      while ((name = g_dir_read_name (dir)))
        {
          xchar_t *fullname = g_build_filename (candidate, name, NULL);

          /* match gvdb behaviour by suffixing "/" on dirs */
          if (xfile_test (fullname, XFILE_TEST_IS_DIR))
            xhash_table_add (*hash, xstrconcat (name, "/", NULL));
          else
            xhash_table_add (*hash, xstrdup (name));

          g_free (fullname);
        }

      g_dir_close (dir);
    }
  else
    {
      if (!xerror_matches (error, XFILE_ERROR, XFILE_ERROR_NOENT))
        g_warning ("Can't enumerate overlay directory '%s': %s", candidate, error->message);
      xerror_free (error);
      return FALSE;
    }

  /* We may want to enumerate results from more than one overlay
   * directory.
   */
  return FALSE;
}

typedef struct {
  xsize_t size;
  xuint32_t flags;
} InfoData;

static xboolean_t
get_overlay_info (const xchar_t *candidate,
                  xpointer_t     user_data)
{
  InfoData *info = user_data;
  GStatBuf buf;

  if (g_stat (candidate, &buf) < 0)
    return FALSE;

  info->size = buf.st_size;
  info->flags = G_RESOURCE_FLAGS_NONE;

  return TRUE;
}

static xboolean_t
g_resource_find_overlay (const xchar_t    *path,
                         CheckCandidate  check,
                         xpointer_t        user_data)
{
  /* This is a null-terminated array of replacement strings (with '=' inside) */
  static const xchar_t * const *overlay_dirs;
  xboolean_t res = FALSE;
  xint_t path_len = -1;
  xint_t i;

  /* We try to be very fast in case there are no overlays.  Otherwise,
   * we can take a bit more time...
   */

  if (g_once_init_enter (&overlay_dirs))
    {
      xboolean_t is_setuid = XPL_PRIVATE_CALL (g_check_setuid) ();
      const xchar_t * const *result;
      const xchar_t *envvar;

      /* Don’t load overlays if setuid, as they could allow reading privileged
       * files. */
      envvar = !is_setuid ? g_getenv ("G_RESOURCE_OVERLAYS") : NULL;
      if (envvar != NULL)
        {
          xchar_t **parts;
          xint_t i, j;

          parts = xstrsplit (envvar, G_SEARCHPATH_SEPARATOR_S, 0);

          /* Sanity check the parts, dropping those that are invalid.
           * 'i' may grow faster than 'j'.
           */
          for (i = j = 0; parts[i]; i++)
            {
              xchar_t *part = parts[i];
              xchar_t *eq;

              eq = strchr (part, '=');
              if (eq == NULL)
                {
                  g_critical ("G_RESOURCE_OVERLAYS segment '%s' lacks '='.  Ignoring.", part);
                  g_free (part);
                  continue;
                }

              if (eq == part)
                {
                  g_critical ("G_RESOURCE_OVERLAYS segment '%s' lacks path before '='.  Ignoring.", part);
                  g_free (part);
                  continue;
                }

              if (eq[1] == '\0')
                {
                  g_critical ("G_RESOURCE_OVERLAYS segment '%s' lacks path after '='.  Ignoring", part);
                  g_free (part);
                  continue;
                }

              if (part[0] != '/')
                {
                  g_critical ("G_RESOURCE_OVERLAYS segment '%s' lacks leading '/'.  Ignoring.", part);
                  g_free (part);
                  continue;
                }

              if (eq[-1] == '/')
                {
                  g_critical ("G_RESOURCE_OVERLAYS segment '%s' has trailing '/' before '='.  Ignoring", part);
                  g_free (part);
                  continue;
                }

              if (!g_path_is_absolute (eq + 1))
                {
                  g_critical ("G_RESOURCE_OVERLAYS segment '%s' does not have an absolute path after '='.  Ignoring", part);
                  g_free (part);
                  continue;
                }

              g_message ("Adding GResources overlay '%s'", part);
              parts[j++] = part;
            }

          parts[j] = NULL;

          result = (const xchar_t **) parts;
        }
      else
        {
          /* We go out of the way to avoid malloc() in the normal case
           * where the environment variable is not set.
           */
          static const xchar_t * const empty_strv[0 + 1];
          result = empty_strv;
        }

      g_once_init_leave (&overlay_dirs, result);
    }

  for (i = 0; overlay_dirs[i]; i++)
    {
      const xchar_t *src;
      xint_t src_len;
      const xchar_t *dst;
      xint_t dst_len;
      xchar_t *candidate;

      {
        xchar_t *eq;

        /* split the overlay into src/dst */
        src = overlay_dirs[i];
        eq = strchr (src, '=');
        xassert (eq); /* we checked this already */
        src_len = eq - src;
        dst = eq + 1;
        /* hold off on dst_len because we will probably fail the checks below */
      }

      if (path_len == -1)
        path_len = strlen (path);

      /* The entire path is too short to match the source */
      if (path_len < src_len)
        continue;

      /* It doesn't match the source */
      if (memcmp (path, src, src_len) != 0)
        continue;

      /* The prefix matches, but it's not a complete path component */
      if (path[src_len] && path[src_len] != '/')
        continue;

      /* OK.  Now we need this. */
      dst_len = strlen (dst);

      /* The candidate will be composed of:
       *
       *    dst + remaining_path + nul
       */
      candidate = g_malloc (dst_len + (path_len - src_len) + 1);
      memcpy (candidate, dst, dst_len);
      memcpy (candidate + dst_len, path + src_len, path_len - src_len);
      candidate[dst_len + (path_len - src_len)] = '\0';

      /* No matter what, 'r' is what we need, including the case where
       * we are trying to enumerate a directory.
       */
      res = (* check) (candidate, user_data);
      g_free (candidate);

      if (res)
        break;
    }

  return res;
}

/**
 * g_resource_error_quark:
 *
 * Gets the #xresource_t Error Quark.
 *
 * Returns: a #xquark
 *
 * Since: 2.32
 */
G_DEFINE_QUARK (g-resource-error-quark, g_resource_error)

/**
 * g_resource_ref:
 * @resource: A #xresource_t
 *
 * Atomically increments the reference count of @resource by one. This
 * function is MT-safe and may be called from any thread.
 *
 * Returns: The passed in #xresource_t
 *
 * Since: 2.32
 **/
xresource_t *
g_resource_ref (xresource_t *resource)
{
  g_atomic_int_inc (&resource->ref_count);
  return resource;
}

/**
 * g_resource_unref:
 * @resource: A #xresource_t
 *
 * Atomically decrements the reference count of @resource by one. If the
 * reference count drops to 0, all memory allocated by the resource is
 * released. This function is MT-safe and may be called from any
 * thread.
 *
 * Since: 2.32
 **/
void
g_resource_unref (xresource_t *resource)
{
  if (g_atomic_int_dec_and_test (&resource->ref_count))
    {
      gvdb_table_free (resource->table);
      g_free (resource);
    }
}

/*< internal >
 * g_resource_new_from_table:
 * @table: (transfer full): a GvdbTable
 *
 * Returns: (transfer full): a new #xresource_t for @table
 */
static xresource_t *
g_resource_new_from_table (GvdbTable *table)
{
  xresource_t *resource;

  resource = g_new (xresource_t, 1);
  resource->ref_count = 1;
  resource->table = table;

  return resource;
}

static void
g_resource_error_from_gvdb_table_error (xerror_t **g_resource_error,
                                        xerror_t  *gvdb_table_error  /* (transfer full) */)
{
  if (xerror_matches (gvdb_table_error, XFILE_ERROR, XFILE_ERROR_INVAL))
    g_set_error_literal (g_resource_error,
                         G_RESOURCE_ERROR, G_RESOURCE_ERROR_INTERNAL,
                         gvdb_table_error->message);
  else
    g_propagate_error (g_resource_error, g_steal_pointer (&gvdb_table_error));
  g_clear_error (&gvdb_table_error);
}

/**
 * g_resource_new_from_data:
 * @data: A #xbytes_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Creates a xresource_t from a reference to the binary resource bundle.
 * This will keep a reference to @data while the resource lives, so
 * the data should not be modified or freed.
 *
 * If you want to use this resource in the global resource namespace you need
 * to register it with g_resources_register().
 *
 * Note: @data must be backed by memory that is at least pointer aligned.
 * Otherwise this function will internally create a copy of the memory since
 * GLib 2.56, or in older versions fail and exit the process.
 *
 * If @data is empty or corrupt, %G_RESOURCE_ERROR_INTERNAL will be returned.
 *
 * Returns: (transfer full): a new #xresource_t, or %NULL on error
 *
 * Since: 2.32
 **/
xresource_t *
g_resource_new_from_data (xbytes_t  *data,
                          xerror_t **error)
{
  GvdbTable *table;
  xboolean_t unref_data = FALSE;
  xerror_t *local_error = NULL;

  if (((guintptr) xbytes_get_data (data, NULL)) % sizeof (xpointer_t) != 0)
    {
      data = xbytes_new (xbytes_get_data (data, NULL),
                          xbytes_get_size (data));
      unref_data = TRUE;
    }

  table = gvdb_table_new_from_bytes (data, TRUE, &local_error);

  if (unref_data)
    xbytes_unref (data);

  if (table == NULL)
    {
      g_resource_error_from_gvdb_table_error (error, g_steal_pointer (&local_error));
      return NULL;
    }

  return g_resource_new_from_table (table);
}

/**
 * g_resource_load:
 * @filename: (type filename): the path of a filename to load, in the GLib filename encoding
 * @error: return location for a #xerror_t, or %NULL
 *
 * Loads a binary resource bundle and creates a #xresource_t representation of it, allowing
 * you to query it for data.
 *
 * If you want to use this resource in the global resource namespace you need
 * to register it with g_resources_register().
 *
 * If @filename is empty or the data in it is corrupt,
 * %G_RESOURCE_ERROR_INTERNAL will be returned. If @filename doesn’t exist, or
 * there is an error in reading it, an error from xmapped_file_new() will be
 * returned.
 *
 * Returns: (transfer full): a new #xresource_t, or %NULL on error
 *
 * Since: 2.32
 **/
xresource_t *
g_resource_load (const xchar_t  *filename,
                 xerror_t      **error)
{
  GvdbTable *table;
  xerror_t *local_error = NULL;

  table = gvdb_table_new (filename, FALSE, &local_error);
  if (table == NULL)
    {
      g_resource_error_from_gvdb_table_error (error, g_steal_pointer (&local_error));
      return NULL;
    }

  return g_resource_new_from_table (table);
}

static xboolean_t
do_lookup (xresource_t             *resource,
           const xchar_t           *path,
           GResourceLookupFlags   lookup_flags,
           xsize_t                 *size,
           xuint32_t               *flags,
           const void           **data,
           xsize_t                 *data_size,
           xerror_t               **error)
{
  char *free_path = NULL;
  xsize_t path_len;
  xboolean_t res = FALSE;
  xvariant_t *value;

  /* Drop any trailing slash. */
  path_len = strlen (path);
  if (path_len >= 1 && path[path_len-1] == '/')
    {
      path = free_path = xstrdup (path);
      free_path[path_len-1] = 0;
    }

  value = gvdb_table_get_raw_value (resource->table, path);

  if (value == NULL)
    {
      g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                   _("The resource at “%s” does not exist"),
                   path);
    }
  else
    {
      xuint32_t _size, _flags;
      xvariant_t *array;

      xvariant_get (value, "(uu@ay)",
                     &_size,
                     &_flags,
                     &array);

      _size = GUINT32_FROM_LE (_size);
      _flags = GUINT32_FROM_LE (_flags);

      if (size)
        *size = _size;
      if (flags)
        *flags = _flags;
      if (data)
        *data = xvariant_get_data (array);
      if (data_size)
        {
          /* Don't report trailing newline that non-compressed files has */
          if (_flags & G_RESOURCE_FLAGS_COMPRESSED)
            *data_size = xvariant_get_size (array);
          else
            *data_size = xvariant_get_size (array) - 1;
        }
      xvariant_unref (array);
      xvariant_unref (value);

      res = TRUE;
    }

  g_free (free_path);
  return res;
}

/**
 * g_resource_open_stream:
 * @resource: A #xresource_t
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks for a file at the specified @path in the resource and
 * returns a #xinput_stream_t that lets you read the data.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: (transfer full): #xinput_stream_t or %NULL on error.
 *     Free the returned object with xobject_unref()
 *
 * Since: 2.32
 **/
xinput_stream_t *
g_resource_open_stream (xresource_t             *resource,
                        const xchar_t           *path,
                        GResourceLookupFlags   lookup_flags,
                        xerror_t               **error)
{
  const void *data;
  xsize_t data_size;
  xuint32_t flags;
  xinput_stream_t *stream, *stream2;

  if (!do_lookup (resource, path, lookup_flags, NULL, &flags, &data, &data_size, error))
    return NULL;

  stream = g_memory_input_stream_new_from_data (data, data_size, NULL);
  xobject_set_data_full (G_OBJECT (stream), "g-resource",
                          g_resource_ref (resource),
                          (xdestroy_notify_t)g_resource_unref);

  if (flags & G_RESOURCE_FLAGS_COMPRESSED)
    {
      xzlib_decompressor_t *decompressor =
        g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB);

      stream2 = xconverter_input_stream_new (stream, XCONVERTER (decompressor));
      xobject_unref (decompressor);
      xobject_unref (stream);
      stream = stream2;
    }

  return stream;
}

/**
 * g_resource_lookup_data:
 * @resource: A #xresource_t
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks for a file at the specified @path in the resource and
 * returns a #xbytes_t that lets you directly access the data in
 * memory.
 *
 * The data is always followed by a zero byte, so you
 * can safely use the data as a C string. However, that byte
 * is not included in the size of the xbytes_t.
 *
 * For uncompressed resource files this is a pointer directly into
 * the resource bundle, which is typically in some readonly data section
 * in the program binary. For compressed files we allocate memory on
 * the heap and automatically uncompress the data.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: (transfer full): #xbytes_t or %NULL on error.
 *     Free the returned object with xbytes_unref()
 *
 * Since: 2.32
 **/
xbytes_t *
g_resource_lookup_data (xresource_t             *resource,
                        const xchar_t           *path,
                        GResourceLookupFlags   lookup_flags,
                        xerror_t               **error)
{
  const void *data;
  xuint32_t flags;
  xsize_t data_size;
  xsize_t size;

  if (!do_lookup (resource, path, lookup_flags, &size, &flags, &data, &data_size, error))
    return NULL;

  if (size == 0)
    return xbytes_new_with_free_func ("", 0, (xdestroy_notify_t) g_resource_unref, g_resource_ref (resource));
  else if (flags & G_RESOURCE_FLAGS_COMPRESSED)
    {
      char *uncompressed, *d;
      const char *s;
      xconverter_result_t res;
      xsize_t d_size, s_size;
      xsize_t bytes_read, bytes_written;


      xzlib_decompressor_t *decompressor =
        g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_ZLIB);

      uncompressed = g_malloc (size + 1);

      s = data;
      s_size = data_size;
      d = uncompressed;
      d_size = size;

      do
        {
          res = xconverter_convert (XCONVERTER (decompressor),
                                     s, s_size,
                                     d, d_size,
                                     XCONVERTER_INPUT_AT_END,
                                     &bytes_read,
                                     &bytes_written,
                                     NULL);
          if (res == XCONVERTER_ERROR)
            {
              g_free (uncompressed);
              xobject_unref (decompressor);

              g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_INTERNAL,
                           _("The resource at “%s” failed to decompress"),
                           path);
              return NULL;

            }
          s += bytes_read;
          s_size -= bytes_read;
          d += bytes_written;
          d_size -= bytes_written;
        }
      while (res != XCONVERTER_FINISHED);

      uncompressed[size] = 0; /* Zero terminate */

      xobject_unref (decompressor);

      return xbytes_new_take (uncompressed, size);
    }
  else
    return xbytes_new_with_free_func (data, data_size, (xdestroy_notify_t)g_resource_unref, g_resource_ref (resource));
}

/**
 * g_resource_get_info:
 * @resource: A #xresource_t
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @size:  (out) (optional): a location to place the length of the contents of the file,
 *    or %NULL if the length is not needed
 * @flags:  (out) (optional): a location to place the flags about the file,
 *    or %NULL if the length is not needed
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks for a file at the specified @path in the resource and
 * if found returns information about it.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: %TRUE if the file was found. %FALSE if there were errors
 *
 * Since: 2.32
 **/
xboolean_t
g_resource_get_info (xresource_t             *resource,
                     const xchar_t           *path,
                     GResourceLookupFlags   lookup_flags,
                     xsize_t                 *size,
                     xuint32_t               *flags,
                     xerror_t               **error)
{
  return do_lookup (resource, path, lookup_flags, size, flags, NULL, NULL, error);
}

/**
 * g_resource_enumerate_children:
 * @resource: A #xresource_t
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @error: return location for a #xerror_t, or %NULL
 *
 * Returns all the names of children at the specified @path in the resource.
 * The return result is a %NULL terminated list of strings which should
 * be released with xstrfreev().
 *
 * If @path is invalid or does not exist in the #xresource_t,
 * %G_RESOURCE_ERROR_NOT_FOUND will be returned.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: (array zero-terminated=1) (transfer full): an array of constant strings
 *
 * Since: 2.32
 **/
xchar_t **
g_resource_enumerate_children (xresource_t             *resource,
                               const xchar_t           *path,
                               GResourceLookupFlags   lookup_flags,
                               xerror_t               **error)
{
  xchar_t local_str[256];
  const xchar_t *path_with_slash;
  xchar_t **children;
  xchar_t *free_path = NULL;
  xsize_t path_len;

  /*
   * Size of 256 is arbitrarily chosen based on being large enough
   * for pretty much everything we come across, but not cumbersome
   * on the stack. It also matches common cacheline sizes.
   */

  if (*path == 0)
    {
      if (error)
        g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                     _("The resource at “%s” does not exist"),
                     path);
      return NULL;
    }

  path_len = strlen (path);

  if G_UNLIKELY (path[path_len-1] != '/')
    {
      if (path_len < sizeof (local_str) - 2)
        {
          /*
           * We got a path that does not have a trailing /. It is not the
           * ideal use of this API as we require trailing / for our lookup
           * into gvdb. Some degenerate application configurations can hit
           * this code path quite a bit, so we try to avoid using the
           * xstrconcat()/g_free().
           */
          memcpy (local_str, path, path_len);
          local_str[path_len] = '/';
          local_str[path_len+1] = 0;
          path_with_slash = local_str;
        }
      else
        {
          path_with_slash = free_path = xstrconcat (path, "/", NULL);
        }
    }
  else
    {
      path_with_slash = path;
    }

  children = gvdb_table_list (resource->table, path_with_slash);
  g_free (free_path);

  if (children == NULL)
    {
      if (error)
        g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                     _("The resource at “%s” does not exist"),
                     path);
      return NULL;
    }

  return children;
}

static GRWLock resources_lock;
static xlist_t *registered_resources;

/* This is updated atomically, so we can append to it and check for NULL outside the
   lock, but all other accesses are done under the write lock */
static GStaticResource *lazy_register_resources;

static void
g_resources_register_unlocked (xresource_t *resource)
{
  registered_resources = xlist_prepend (registered_resources, g_resource_ref (resource));
}

static void
g_resources_unregister_unlocked (xresource_t *resource)
{
  if (xlist_find (registered_resources, resource) == NULL)
    {
      g_warning ("Tried to remove not registered resource");
    }
  else
    {
      registered_resources = xlist_remove (registered_resources, resource);
      g_resource_unref (resource);
    }
}

/**
 * g_resources_register:
 * @resource: A #xresource_t
 *
 * Registers the resource with the process-global set of resources.
 * Once a resource is registered the files in it can be accessed
 * with the global resource lookup functions like g_resources_lookup_data().
 *
 * Since: 2.32
 **/
void
g_resources_register (xresource_t *resource)
{
  g_rw_lock_writer_lock (&resources_lock);
  g_resources_register_unlocked (resource);
  g_rw_lock_writer_unlock (&resources_lock);
}

/**
 * g_resources_unregister:
 * @resource: A #xresource_t
 *
 * Unregisters the resource from the process-global set of resources.
 *
 * Since: 2.32
 **/
void
g_resources_unregister (xresource_t *resource)
{
  g_rw_lock_writer_lock (&resources_lock);
  g_resources_unregister_unlocked (resource);
  g_rw_lock_writer_unlock (&resources_lock);
}

/**
 * g_resources_open_stream:
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks for a file at the specified @path in the set of
 * globally registered resources and returns a #xinput_stream_t
 * that lets you read the data.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: (transfer full): #xinput_stream_t or %NULL on error.
 *     Free the returned object with xobject_unref()
 *
 * Since: 2.32
 **/
xinput_stream_t *
g_resources_open_stream (const xchar_t           *path,
                         GResourceLookupFlags   lookup_flags,
                         xerror_t               **error)
{
  xinput_stream_t *res = NULL;
  xlist_t *l;
  xinput_stream_t *stream;

  if (g_resource_find_overlay (path, open_overlay_stream, &res))
    return res;

  register_lazy_static_resources ();

  g_rw_lock_reader_lock (&resources_lock);

  for (l = registered_resources; l != NULL; l = l->next)
    {
      xresource_t *r = l->data;
      xerror_t *my_error = NULL;

      stream = g_resource_open_stream (r, path, lookup_flags, &my_error);
      if (stream == NULL &&
          xerror_matches (my_error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
        {
          g_clear_error (&my_error);
        }
      else
        {
          if (stream == NULL)
            g_propagate_error (error, my_error);
          res = stream;
          break;
        }
    }

  if (l == NULL)
    g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                 _("The resource at “%s” does not exist"),
                 path);

  g_rw_lock_reader_unlock (&resources_lock);

  return res;
}

/**
 * g_resources_lookup_data:
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks for a file at the specified @path in the set of
 * globally registered resources and returns a #xbytes_t that
 * lets you directly access the data in memory.
 *
 * The data is always followed by a zero byte, so you
 * can safely use the data as a C string. However, that byte
 * is not included in the size of the xbytes_t.
 *
 * For uncompressed resource files this is a pointer directly into
 * the resource bundle, which is typically in some readonly data section
 * in the program binary. For compressed files we allocate memory on
 * the heap and automatically uncompress the data.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: (transfer full): #xbytes_t or %NULL on error.
 *     Free the returned object with xbytes_unref()
 *
 * Since: 2.32
 **/
xbytes_t *
g_resources_lookup_data (const xchar_t           *path,
                         GResourceLookupFlags   lookup_flags,
                         xerror_t               **error)
{
  xbytes_t *res = NULL;
  xlist_t *l;
  xbytes_t *data;

  if (g_resource_find_overlay (path, get_overlay_bytes, &res))
    return res;

  register_lazy_static_resources ();

  g_rw_lock_reader_lock (&resources_lock);

  for (l = registered_resources; l != NULL; l = l->next)
    {
      xresource_t *r = l->data;
      xerror_t *my_error = NULL;

      data = g_resource_lookup_data (r, path, lookup_flags, &my_error);
      if (data == NULL &&
          xerror_matches (my_error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
        {
          g_clear_error (&my_error);
        }
      else
        {
          if (data == NULL)
            g_propagate_error (error, my_error);
          res = data;
          break;
        }
    }

  if (l == NULL)
    g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                 _("The resource at “%s” does not exist"),
                 path);

  g_rw_lock_reader_unlock (&resources_lock);

  return res;
}

/**
 * g_resources_enumerate_children:
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @error: return location for a #xerror_t, or %NULL
 *
 * Returns all the names of children at the specified @path in the set of
 * globally registered resources.
 * The return result is a %NULL terminated list of strings which should
 * be released with xstrfreev().
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: (array zero-terminated=1) (transfer full): an array of constant strings
 *
 * Since: 2.32
 **/
xchar_t **
g_resources_enumerate_children (const xchar_t           *path,
                                GResourceLookupFlags   lookup_flags,
                                xerror_t               **error)
{
  xhashtable_t *hash = NULL;
  xlist_t *l;
  char **children;
  int i;

  /* This will enumerate actual files found in overlay directories but
   * will not enumerate the overlays themselves.  For example, if we
   * have an overlay "/org/gtk=/path/to/files" and we enumerate "/org"
   * then we will not see "gtk" in the result set unless it is provided
   * by another resource file.
   *
   * This is probably not going to be a problem since if we are doing
   * such an overlay, we probably will already have that path.
   */
  g_resource_find_overlay (path, enumerate_overlay_dir, &hash);

  register_lazy_static_resources ();

  g_rw_lock_reader_lock (&resources_lock);

  for (l = registered_resources; l != NULL; l = l->next)
    {
      xresource_t *r = l->data;

      children = g_resource_enumerate_children (r, path, 0, NULL);

      if (children != NULL)
        {
          if (hash == NULL)
            /* note: keep in sync with same line above */
            hash = xhash_table_new_full (xstr_hash, xstr_equal, g_free, NULL);

          for (i = 0; children[i] != NULL; i++)
            xhash_table_add (hash, children[i]);
          g_free (children);
        }
    }

  g_rw_lock_reader_unlock (&resources_lock);

  if (hash == NULL)
    {
      if (error)
        g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                     _("The resource at “%s” does not exist"),
                     path);
      return NULL;
    }
  else
    {
      children = (xchar_t **) xhash_table_get_keys_as_array (hash, NULL);
      xhash_table_steal_all (hash);
      xhash_table_destroy (hash);

      return children;
    }
}

/**
 * g_resources_get_info:
 * @path: A pathname inside the resource
 * @lookup_flags: A #GResourceLookupFlags
 * @size:  (out) (optional): a location to place the length of the contents of the file,
 *    or %NULL if the length is not needed
 * @flags:  (out) (optional): a location to place the #GResourceFlags about the file,
 *    or %NULL if the flags are not needed
 * @error: return location for a #xerror_t, or %NULL
 *
 * Looks for a file at the specified @path in the set of
 * globally registered resources and if found returns information about it.
 *
 * @lookup_flags controls the behaviour of the lookup.
 *
 * Returns: %TRUE if the file was found. %FALSE if there were errors
 *
 * Since: 2.32
 **/
xboolean_t
g_resources_get_info (const xchar_t           *path,
                      GResourceLookupFlags   lookup_flags,
                      xsize_t                 *size,
                      xuint32_t               *flags,
                      xerror_t               **error)
{
  xboolean_t res = FALSE;
  xlist_t *l;
  xboolean_t r_res;
  InfoData info;

  if (g_resource_find_overlay (path, get_overlay_info, &info))
    {
      if (size)
        *size = info.size;
      if (flags)
        *flags = info.flags;

      return TRUE;
    }

  register_lazy_static_resources ();

  g_rw_lock_reader_lock (&resources_lock);

  for (l = registered_resources; l != NULL; l = l->next)
    {
      xresource_t *r = l->data;
      xerror_t *my_error = NULL;

      r_res = g_resource_get_info (r, path, lookup_flags, size, flags, &my_error);
      if (!r_res &&
          xerror_matches (my_error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
        {
          g_clear_error (&my_error);
        }
      else
        {
          if (!r_res)
            g_propagate_error (error, my_error);
          res = r_res;
          break;
        }
    }

  if (l == NULL)
    g_set_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND,
                 _("The resource at “%s” does not exist"),
                 path);

  g_rw_lock_reader_unlock (&resources_lock);

  return res;
}

/* This code is to handle registration of resources very early, from a constructor.
 * At that point we'd like to do minimal work, to avoid ordering issues. For instance,
 * we're not allowed to use g_malloc, as the user need to be able to call g_mem_set_vtable
 * before the first call to g_malloc.
 *
 * So, what we do at construction time is that we just register a static structure on
 * a list of resources that need to be initialized, and then later, when doing any lookups
 * in the global list of registered resources, or when getting a reference to the
 * lazily initialized resource we lazily create and register all the GResources on
 * the lazy list.
 *
 * To avoid having to use locks in the constructor, and having to grab the writer lock
 * when checking the lazy registering list we update lazy_register_resources in
 * a lock-less fashion (atomic prepend-only, atomic replace with NULL). However, all
 * operations except:
 *  * check if there are any resources to lazily initialize
 *  * Add a static resource to the lazy init list
 * Do use the full writer lock for protection.
 */

static void
register_lazy_static_resources_unlocked (void)
{
  GStaticResource *list;

  do
    list = lazy_register_resources;
  while (!g_atomic_pointer_compare_and_exchange (&lazy_register_resources, list, NULL));

  while (list != NULL)
    {
      xbytes_t *bytes = xbytes_new_static (list->data, list->data_len);
      xresource_t *resource = g_resource_new_from_data (bytes, NULL);
      if (resource)
        {
          g_resources_register_unlocked (resource);
          g_atomic_pointer_set (&list->resource, resource);
        }
      xbytes_unref (bytes);

      list = list->next;
    }
}

static void
register_lazy_static_resources (void)
{
  if (g_atomic_pointer_get (&lazy_register_resources) == NULL)
    return;

  g_rw_lock_writer_lock (&resources_lock);
  register_lazy_static_resources_unlocked ();
  g_rw_lock_writer_unlock (&resources_lock);
}

/**
 * g_static_resource_init:
 * @static_resource: pointer to a static #GStaticResource
 *
 * Initializes a xresource_t from static data using a
 * GStaticResource.
 *
 * This is normally used by code generated by
 * [glib-compile-resources][glib-compile-resources]
 * and is not typically used by other code.
 *
 * Since: 2.32
 **/
void
g_static_resource_init (GStaticResource *static_resource)
{
  GStaticResource *next;

  do
    {
      next = lazy_register_resources;
      static_resource->next = next;
    }
  while (!g_atomic_pointer_compare_and_exchange (&lazy_register_resources, next, static_resource));
}

/**
 * g_static_resource_fini:
 * @static_resource: pointer to a static #GStaticResource
 *
 * Finalized a xresource_t initialized by g_static_resource_init().
 *
 * This is normally used by code generated by
 * [glib-compile-resources][glib-compile-resources]
 * and is not typically used by other code.
 *
 * Since: 2.32
 **/
void
g_static_resource_fini (GStaticResource *static_resource)
{
  xresource_t *resource;

  g_rw_lock_writer_lock (&resources_lock);

  register_lazy_static_resources_unlocked ();

  resource = g_atomic_pointer_get (&static_resource->resource);
  if (resource)
    {
      g_atomic_pointer_set (&static_resource->resource, NULL);
      g_resources_unregister_unlocked (resource);
      g_resource_unref (resource);
    }

  g_rw_lock_writer_unlock (&resources_lock);
}

/**
 * g_static_resource_get_resource:
 * @static_resource: pointer to a static #GStaticResource
 *
 * Gets the xresource_t that was registered by a call to g_static_resource_init().
 *
 * This is normally used by code generated by
 * [glib-compile-resources][glib-compile-resources]
 * and is not typically used by other code.
 *
 * Returns:  (transfer none): a #xresource_t
 *
 * Since: 2.32
 **/
xresource_t *
g_static_resource_get_resource (GStaticResource *static_resource)
{
  register_lazy_static_resources ();

  return g_atomic_pointer_get (&static_resource->resource);
}
