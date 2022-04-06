/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2014 Patrick Griffis
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
 */

#include "config.h"

#include "gcontenttype.h"
#include "gicon.h"
#include "gthemedicon.h"

#include <CoreServices/CoreServices.h>

#define XDG_PREFIX _gio_xdg
#include "xdgmime/xdgmime.h"

/* We lock this mutex whenever we modify global state in this module.  */
G_LOCK_DEFINE_STATIC (gio_xdgmime);


/*< internal >
 * create_cfstring_from_cstr:
 * @cstr: a #xchar_t
 *
 * Converts a cstr to a utf8 cfstring
 * It must be CFReleased()'d.
 *
 */
static CFStringRef
create_cfstring_from_cstr (const xchar_t *cstr)
{
  return CFStringCreateWithCString (NULL, cstr, kCFStringEncodingUTF8);
}

/*< internal >
 * create_cstr_from_cfstring:
 * @str: a #CFStringRef
 *
 * Converts a cfstring to a utf8 cstring.
 * The incoming cfstring is released for you.
 * The returned string must be g_free()'d.
 *
 */
static xchar_t *
create_cstr_from_cfstring (CFStringRef str)
{
  g_return_val_if_fail (str != NULL, NULL);

  CFIndex length = CFStringGetLength (str);
  CFIndex maxlen = CFStringGetMaximumSizeForEncoding (length, kCFStringEncodingUTF8);
  xchar_t *buffer = g_malloc (maxlen + 1);
  Boolean success = CFStringGetCString (str, (char *) buffer, maxlen,
                                        kCFStringEncodingUTF8);
  CFRelease (str);
  if (success)
    return buffer;
  else
    {
      g_free (buffer);
      return NULL;
    }
}

/*< internal >
 * create_cstr_from_cfstring_with_fallback:
 * @str: a #CFStringRef
 * @fallback: a #xchar_t
 *
 * Tries to convert a cfstring to a utf8 cstring.
 * If @str is NULL or conversion fails @fallback is returned.
 * The incoming cfstring is released for you.
 * The returned string must be g_free()'d.
 *
 */
static xchar_t *
create_cstr_from_cfstring_with_fallback (CFStringRef  str,
                                         const xchar_t *fallback)
{
  xchar_t *cstr = NULL;

  if (str)
    cstr = create_cstr_from_cfstring (str);
  if (!cstr)
    return xstrdup (fallback);

  return cstr;
}

/*< private >*/
void
g_content_type_set_mime_dirs (const xchar_t * const *dirs)
{
  /* noop on macOS */
}

/*< private >*/
const xchar_t * const *
g_content_type_get_mime_dirs (void)
{
  const xchar_t * const *mime_dirs = { NULL };
  return mime_dirs;
}

xboolean_t
g_content_type_equals (const xchar_t *type1,
                       const xchar_t *type2)
{
  CFStringRef str1, str2;
  xboolean_t ret;

  g_return_val_if_fail (type1 != NULL, FALSE);
  g_return_val_if_fail (type2 != NULL, FALSE);

  if (g_ascii_strcasecmp (type1, type2) == 0)
    return TRUE;

  str1 = create_cfstring_from_cstr (type1);
  str2 = create_cfstring_from_cstr (type2);

  ret = UTTypeEqual (str1, str2);

  CFRelease (str1);
  CFRelease (str2);

  return ret;
}

xboolean_t
g_content_type_is_a (const xchar_t *ctype,
                     const xchar_t *csupertype)
{
  CFStringRef type, supertype;
  xboolean_t ret;

  g_return_val_if_fail (ctype != NULL, FALSE);
  g_return_val_if_fail (csupertype != NULL, FALSE);

  type = create_cfstring_from_cstr (ctype);
  supertype = create_cfstring_from_cstr (csupertype);

  ret = UTTypeConformsTo (type, supertype);

  CFRelease (type);
  CFRelease (supertype);

  return ret;
}

xboolean_t
g_content_type_is_mime_type (const xchar_t *type,
                             const xchar_t *mime_type)
{
  xchar_t *content_type;
  xboolean_t ret;

  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);

  content_type = g_content_type_from_mime_type (mime_type);
  ret = g_content_type_is_a (type, content_type);
  g_free (content_type);

  return ret;
}

xboolean_t
g_content_type_is_unknown (const xchar_t *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  /* Should dynamic types be considered "unknown"? */
  if (xstr_has_prefix (type, "dyn."))
    return TRUE;
  /* application/octet-stream */
  else if (xstrcmp0 (type, "public.data") == 0)
    return TRUE;

  return FALSE;
}

xchar_t *
g_content_type_get_description (const xchar_t *type)
{
  CFStringRef str;
  CFStringRef desc_str;

  g_return_val_if_fail (type != NULL, NULL);

  str = create_cfstring_from_cstr (type);
  desc_str = UTTypeCopyDescription (str);

  CFRelease (str);
  return create_cstr_from_cfstring_with_fallback (desc_str, "unknown");
}

/* <internal>
 * _get_generic_icon_name_from_mime_type
 *
 * This function produces a generic icon name from a @mime_type.
 * If no generic icon name is found in the xdg mime database, the
 * generic icon name is constructed.
 *
 * Background:
 * generic-icon elements specify the icon to use as a generic icon for this
 * particular mime-type, given by the name attribute. This is used if there
 * is no specific icon (see icon for how these are found). These are used
 * for categories of similar types (like spreadsheets or archives) that can
 * use a common icon. The Icon Naming Specification lists a set of such
 * icon names. If this element is not specified then the mimetype is used
 * to generate the generic icon by using the top-level media type
 * (e.g. "video" in "video/ogg") and appending "-x-generic"
 * (i.e. "video-x-generic" in the previous example).
 *
 * From: https://specifications.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-0.18.html
 */

static xchar_t *
_get_generic_icon_name_from_mime_type (const xchar_t *mime_type)
{
  const xchar_t *xdxicon_name;
  xchar_t *icon_name;

  G_LOCK (gio_xdgmime);
  xdxicon_name = xdg_mime_get_generic_icon (mime_type);
  G_UNLOCK (gio_xdgmime);

  if (xdxicon_name == NULL)
    {
      const char *p;
      const char *suffix = "-x-generic";
      xsize_t prefix_len;

      p = strchr (mime_type, '/');
      if (p == NULL)
        prefix_len = strlen (mime_type);
      else
        prefix_len = p - mime_type;

      icon_name = g_malloc (prefix_len + strlen (suffix) + 1);
      memcpy (icon_name, mime_type, prefix_len);
      memcpy (icon_name + prefix_len, suffix, strlen (suffix));
      icon_name[prefix_len + strlen (suffix)] = 0;
    }
  else
    {
      icon_name = xstrdup (xdxicon_name);
    }

  return icon_name;
}


static xicon_t *
g_content_type_get_icon_internal (const xchar_t *uti,
                                  xboolean_t     symbolic)
{
  char *mimetype_icon;
  char *mime_type;
  char *generic_mimetype_icon = NULL;
  char *q;
  char *icon_names[6];
  int n = 0;
  xicon_t *themed_icon;
  const char  *xdg_icon;
  int i;

  g_return_val_if_fail (uti != NULL, NULL);

  mime_type = g_content_type_get_mime_type (uti);

  G_LOCK (gio_xdgmime);
  xdg_icon = xdg_mime_get_icon (mime_type);
  G_UNLOCK (gio_xdgmime);

  if (xdg_icon)
    icon_names[n++] = xstrdup (xdg_icon);

  mimetype_icon = xstrdup (mime_type);
  while ((q = strchr (mimetype_icon, '/')) != NULL)
    *q = '-';

  icon_names[n++] = mimetype_icon;

  generic_mimetype_icon = _get_generic_icon_name_from_mime_type (mime_type);

  if (generic_mimetype_icon)
    icon_names[n++] = generic_mimetype_icon;

  if (symbolic)
    {
      for (i = 0; i < n; i++)
        {
          icon_names[n + i] = icon_names[i];
          icon_names[i] = xstrconcat (icon_names[i], "-symbolic", NULL);
        }

      n += n;
    }

  themed_icon = g_themed_icon_new_from_names (icon_names, n);

  for (i = 0; i < n; i++)
    g_free (icon_names[i]);

  g_free(mime_type);

  return themed_icon;
}

xicon_t *
g_content_type_get_icon (const xchar_t *type)
{
  return g_content_type_get_icon_internal (type, FALSE);
}

xicon_t *
g_content_type_get_symbolic_icon (const xchar_t *type)
{
  return g_content_type_get_icon_internal (type, TRUE);
}

xchar_t *
g_content_type_get_generic_icon_name (const xchar_t *type)
{
  return NULL;
}

xboolean_t
g_content_type_can_be_executable (const xchar_t *type)
{
  CFStringRef uti;
  xboolean_t ret = FALSE;

  g_return_val_if_fail (type != NULL, FALSE);

  uti = create_cfstring_from_cstr (type);

  if (UTTypeConformsTo (uti, kUTTypeApplication))
    ret = TRUE;
  else if (UTTypeConformsTo (uti, CFSTR("public.executable")))
    ret = TRUE;
  else if (UTTypeConformsTo (uti, CFSTR("public.script")))
    ret = TRUE;
  /* Our tests assert that all text can be executable... */
  else if (UTTypeConformsTo (uti, CFSTR("public.text")))
      ret = TRUE;

  CFRelease (uti);
  return ret;
}

xchar_t *
g_content_type_from_mime_type (const xchar_t *mime_type)
{
  CFStringRef mime_str;
  CFStringRef uti_str;

  g_return_val_if_fail (mime_type != NULL, NULL);

  /* Their api does not handle globs but they are common. */
  if (xstr_has_suffix (mime_type, "*"))
    {
      if (xstr_has_prefix (mime_type, "audio"))
        return xstrdup ("public.audio");
      if (xstr_has_prefix (mime_type, "image"))
        return xstrdup ("public.image");
      if (xstr_has_prefix (mime_type, "text"))
        return xstrdup ("public.text");
      if (xstr_has_prefix (mime_type, "video"))
        return xstrdup ("public.movie");
    }

  /* Some exceptions are needed for gdk-pixbuf.
   * This list is not exhaustive.
   */
  if (xstr_has_prefix (mime_type, "image"))
    {
      if (xstr_has_suffix (mime_type, "x-icns"))
        return xstrdup ("com.apple.icns");
      if (xstr_has_suffix (mime_type, "x-tga"))
        return xstrdup ("com.truevision.tga-image");
      if (xstr_has_suffix (mime_type, "x-ico"))
        return xstrdup ("com.microsoft.ico ");
    }

  /* These are also not supported...
   * Used in glocalfileinfo.c
   */
  if (xstr_has_prefix (mime_type, "inode"))
    {
      if (xstr_has_suffix (mime_type, "directory"))
        return xstrdup ("public.folder");
      if (xstr_has_suffix (mime_type, "symlink"))
        return xstrdup ("public.symlink");
    }

  /* This is correct according to the Apple docs:
     https://developer.apple.com/library/content/documentation/Miscellaneous/Reference/UTIRef/Articles/System-DeclaredUniformTypeIdentifiers.html
  */
  if (strcmp (mime_type, "text/plain") == 0)
    return xstrdup ("public.text");

  /* Non standard type */
  if (strcmp (mime_type, "application/x-executable") == 0)
    return xstrdup ("public.executable");

  mime_str = create_cfstring_from_cstr (mime_type);
  uti_str = UTTypeCreatePreferredIdentifierForTag (kUTTagClassMIMEType, mime_str, NULL);

  CFRelease (mime_str);
  return create_cstr_from_cfstring_with_fallback (uti_str, "public.data");
}

xchar_t *
g_content_type_get_mime_type (const xchar_t *type)
{
  CFStringRef uti_str;
  CFStringRef mime_str;

  g_return_val_if_fail (type != NULL, NULL);

  /* We must match the additions above
   * so conversions back and forth work.
   */
  if (xstr_has_prefix (type, "public"))
    {
      if (xstr_has_suffix (type, ".image"))
        return xstrdup ("image/*");
      if (xstr_has_suffix (type, ".movie"))
        return xstrdup ("video/*");
      if (xstr_has_suffix (type, ".text"))
        return xstrdup ("text/*");
      if (xstr_has_suffix (type, ".audio"))
        return xstrdup ("audio/*");
      if (xstr_has_suffix (type, ".folder"))
        return xstrdup ("inode/directory");
      if (xstr_has_suffix (type, ".symlink"))
        return xstrdup ("inode/symlink");
      if (xstr_has_suffix (type, ".executable"))
        return xstrdup ("application/x-executable");
    }

  uti_str = create_cfstring_from_cstr (type);
  mime_str = UTTypeCopyPreferredTagWithClass(uti_str, kUTTagClassMIMEType);

  CFRelease (uti_str);
  return create_cstr_from_cfstring_with_fallback (mime_str, "application/octet-stream");
}

static xboolean_t
looks_like_text (const guchar *data,
                 xsize_t         data_size)
{
  xsize_t i;
  guchar c;

  for (i = 0; i < data_size; i++)
    {
      c = data[i];
      if (g_ascii_iscntrl (c) && !g_ascii_isspace (c) && c != '\b')
        return FALSE;
    }
  return TRUE;
}

xchar_t *
g_content_type_guess (const xchar_t  *filename,
                      const guchar *data,
                      xsize_t         data_size,
                      xboolean_t     *result_uncertain)
{
  CFStringRef uti = NULL;
  xchar_t *cextension;
  CFStringRef extension;
  int uncertain = -1;

  g_return_val_if_fail (data_size != (xsize_t) -1, NULL);

  if (filename && *filename)
    {
      xchar_t *basename = g_path_get_basename (filename);
      xchar_t *dirname = g_path_get_dirname (filename);
      xsize_t i = strlen (filename);

      if (filename[i - 1] == '/')
        {
          if (xstrcmp0 (dirname, "/Volumes") == 0)
            {
              uti = CFStringCreateCopy (NULL, kUTTypeVolume);
            }
          else if ((cextension = strrchr (basename, '.')) != NULL)
            {
              cextension++;
              extension = create_cfstring_from_cstr (cextension);
              uti = UTTypeCreatePreferredIdentifierForTag (kUTTagClassFilenameExtension,
                                                           extension, NULL);
              CFRelease (extension);

              if (CFStringHasPrefix (uti, CFSTR ("dyn.")))
                {
                  CFRelease (uti);
                  uti = CFStringCreateCopy (NULL, kUTTypeFolder);
                  uncertain = TRUE;
                }
            }
          else
            {
              uti = CFStringCreateCopy (NULL, kUTTypeFolder);
              uncertain = TRUE; /* Matches Unix backend */
            }
        }
      else
        {
          /* GTK needs this... */
          if (xstr_has_suffix (basename, ".ui"))
            {
              uti = CFStringCreateCopy (NULL, kUTTypeXML);
            }
          else if (xstr_has_suffix (basename, ".txt"))
            {
              uti = CFStringCreateCopy (NULL, CFSTR ("public.text"));
            }
          else if ((cextension = strrchr (basename, '.')) != NULL)
            {
              cextension++;
              extension = create_cfstring_from_cstr (cextension);
              uti = UTTypeCreatePreferredIdentifierForTag (kUTTagClassFilenameExtension,
                                                           extension, NULL);
              CFRelease (extension);
            }
          g_free (basename);
          g_free (dirname);
        }
    }
  if (data && (!filename || !uti ||
               CFStringCompare (uti, CFSTR ("public.data"), 0) == kCFCompareEqualTo))
    {
      const char *sniffed_mimetype;
      G_LOCK (gio_xdgmime);
      sniffed_mimetype = xdg_mime_get_mime_type_for_data (data, data_size, NULL);
      G_UNLOCK (gio_xdgmime);
      if (sniffed_mimetype != XDG_MIME_TYPE_UNKNOWN)
        {
          xchar_t *uti_str = g_content_type_from_mime_type (sniffed_mimetype);
          uti = create_cfstring_from_cstr (uti_str);
          g_free (uti_str);
        }
      if (!uti && looks_like_text (data, data_size))
        {
          if (xstr_has_prefix ((const xchar_t*)data, "#!/"))
            uti = CFStringCreateCopy (NULL, CFSTR ("public.script"));
          else
            uti = CFStringCreateCopy (NULL, CFSTR ("public.text"));
        }
    }

  if (!uti)
    {
      /* Generic data type */
      uti = CFStringCreateCopy (NULL, CFSTR ("public.data"));
      if (result_uncertain)
        *result_uncertain = TRUE;
    }
  else if (result_uncertain)
    {
      *result_uncertain = uncertain == -1 ? FALSE : uncertain;
    }

  return create_cstr_from_cfstring (uti);
}

xlist_t *
g_content_types_get_registered (void)
{
  /* TODO: UTTypeCreateAllIdentifiersForTag? */
  return NULL;
}

xchar_t **
g_content_type_guess_for_tree (xfile_t *root)
{
  return NULL;
}
