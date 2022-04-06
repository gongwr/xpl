/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gcontenttype.h"
#include "gthemedicon.h"
#include "gicon.h"
#include "glibintl.h"

#include <windows.h>

static char *
get_registry_classes_key (const char    *subdir,
                          const wchar_t *key_name)
{
  wchar_t *wc_key;
  HKEY reg_key = NULL;
  DWORD key_type;
  DWORD nbytes;
  char *value_utf8;

  value_utf8 = NULL;

  nbytes = 0;
  wc_key = xutf8_to_utf16 (subdir, -1, NULL, NULL, NULL);
  if (RegOpenKeyExW (HKEY_CLASSES_ROOT, wc_key, 0,
                     KEY_QUERY_VALUE, &reg_key) == ERROR_SUCCESS &&
      RegQueryValueExW (reg_key, key_name, 0,
                        &key_type, NULL, &nbytes) == ERROR_SUCCESS &&
      (key_type == REG_SZ || key_type == REG_EXPAND_SZ))
    {
      wchar_t *wc_temp = g_new (wchar_t, (nbytes+1)/2 + 1);
      RegQueryValueExW (reg_key, key_name, 0,
                        &key_type, (LPBYTE) wc_temp, &nbytes);
      wc_temp[nbytes/2] = '\0';
      if (key_type == REG_EXPAND_SZ)
        {
          wchar_t dummy[1];
          DWORD len = ExpandEnvironmentStringsW (wc_temp, dummy, 1);
          if (len > 0)
            {
              wchar_t *wc_temp_expanded = g_new (wchar_t, len);
              if (ExpandEnvironmentStringsW (wc_temp, wc_temp_expanded, len) == len)
                value_utf8 = xutf16_to_utf8 (wc_temp_expanded, -1, NULL, NULL, NULL);
              g_free (wc_temp_expanded);
            }
        }
      else
        {
          value_utf8 = xutf16_to_utf8 (wc_temp, -1, NULL, NULL, NULL);
        }
      g_free (wc_temp);

    }
  g_free (wc_key);

  if (reg_key != NULL)
    RegCloseKey (reg_key);

  return value_utf8;
}

/*< private >*/
void
g_content_type_set_mime_dirs (const xchar_t * const *dirs)
{
  /* noop on Windows */
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
  char *progid1, *progid2;
  xboolean_t res;

  g_return_val_if_fail (type1 != NULL, FALSE);
  g_return_val_if_fail (type2 != NULL, FALSE);

  if (g_ascii_strcasecmp (type1, type2) == 0)
    return TRUE;

  res = FALSE;
  progid1 = get_registry_classes_key (type1, NULL);
  progid2 = get_registry_classes_key (type2, NULL);
  if (progid1 != NULL && progid2 != NULL &&
      strcmp (progid1, progid2) == 0)
    res = TRUE;
  g_free (progid1);
  g_free (progid2);

  return res;
}

xboolean_t
g_content_type_is_a (const xchar_t *type,
                     const xchar_t *supertype)
{
  xboolean_t res;
  char *value_utf8;

  g_return_val_if_fail (type != NULL, FALSE);
  g_return_val_if_fail (supertype != NULL, FALSE);

  if (g_content_type_equals (type, supertype))
    return TRUE;

  res = FALSE;
  value_utf8 = get_registry_classes_key (type, L"PerceivedType");
  if (value_utf8 && strcmp (value_utf8, supertype) == 0)
    res = TRUE;
  g_free (value_utf8);

  return res;
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

  return strcmp ("*", type) == 0;
}

xchar_t *
g_content_type_get_description (const xchar_t *type)
{
  char *progid;
  char *description;

  g_return_val_if_fail (type != NULL, NULL);

  progid = get_registry_classes_key (type, NULL);
  if (progid)
    {
      description = get_registry_classes_key (progid, NULL);
      g_free (progid);

      if (description)
        return description;
    }

  if (g_content_type_is_unknown (type))
    return xstrdup (_("Unknown type"));

  return xstrdup_printf (_("%s filetype"), type);
}

xchar_t *
g_content_type_get_mime_type (const xchar_t *type)
{
  char *mime;

  g_return_val_if_fail (type != NULL, NULL);

  mime = get_registry_classes_key (type, L"Content Type");
  if (mime)
    return mime;
  else if (g_content_type_is_unknown (type))
    return xstrdup ("application/octet-stream");
  else if (*type == '.')
    return xstrdup_printf ("application/x-ext-%s", type+1);
  else if (strcmp ("inode/directory", type) == 0)
    return xstrdup (type);
  /* TODO: Map "image" to "image/ *", etc? */

  return xstrdup ("application/octet-stream");
}

G_LOCK_DEFINE_STATIC (_type_icons);
static xhashtable_t *_type_icons = NULL;

xicon_t *
g_content_type_get_icon (const xchar_t *type)
{
  xicon_t *themed_icon;
  char *name = NULL;

  g_return_val_if_fail (type != NULL, NULL);

  /* In the Registry icons are the default value of
     HKEY_CLASSES_ROOT\<progid>\DefaultIcon with typical values like:
     <type>: <value>
     REG_EXPAND_SZ: %SystemRoot%\System32\Wscript.exe,3
     REG_SZ: shimgvw.dll,3
  */
  G_LOCK (_type_icons);
  if (!_type_icons)
    _type_icons = xhash_table_new (xstr_hash, xstr_equal);
  name = xhash_table_lookup (_type_icons, type);
  if (!name && type[0] == '.')
    {
      /* double lookup by extension */
      xchar_t *key = get_registry_classes_key (type, NULL);
      if (!key)
        key = xstrconcat (type+1, "file\\DefaultIcon", NULL);
      else
        {
          xchar_t *key2 = xstrconcat (key, "\\DefaultIcon", NULL);
          g_free (key);
          key = key2;
        }
      name = get_registry_classes_key (key, NULL);
      if (name && strcmp (name, "%1") == 0)
        {
          g_free (name);
          name = NULL;
        }
      if (name)
        xhash_table_insert (_type_icons, xstrdup (type), xstrdup (name));
      g_free (key);
    }

  if (!name)
    {
      /* if no icon found, fall back to standard generic names */
      if (strcmp (type, "inode/directory") == 0)
        name = "folder";
      else if (g_content_type_can_be_executable (type))
        name = "system-run";
      else
        name = "text-x-generic";
      xhash_table_insert (_type_icons, xstrdup (type), xstrdup (name));
    }
  themed_icon = g_themed_icon_new (name);
  G_UNLOCK (_type_icons);

  return XICON (themed_icon);
}

xicon_t *
g_content_type_get_symbolic_icon (const xchar_t *type)
{
  return g_content_type_get_icon (type);
}

xchar_t *
g_content_type_get_generic_icon_name (const xchar_t *type)
{
  return NULL;
}

xboolean_t
g_content_type_can_be_executable (const xchar_t *type)
{
  g_return_val_if_fail (type != NULL, FALSE);

  if (strcmp (type, ".exe") == 0 ||
      strcmp (type, ".com") == 0 ||
      strcmp (type, ".bat") == 0)
    return TRUE;

  /* TODO: Also look at PATHEXT, which lists the extensions for
   * "scripts" in addition to those for true binary executables.
   *
   * (PATHEXT=.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH for me
   * right now, for instance). And in a sense, all associated file
   * types are "executable" on Windows... You can just type foo.jpg as
   * a command name in cmd.exe, and it will run the application
   * associated with .jpg. Hard to say what this API actually means
   * with "executable".
   */

  return FALSE;
}

static xboolean_t
looks_like_text (const xuchar_t *data,
                 xsize_t         data_size)
{
  xsize_t i;
  xuchar_t c;
  for (i = 0; i < data_size; i++)
    {
      c = data[i];
      if (g_ascii_iscntrl (c) && !g_ascii_isspace (c) && c != '\b')
        return FALSE;
    }
  return TRUE;
}

xchar_t *
g_content_type_from_mime_type (const xchar_t *mime_type)
{
  char *key, *content_type;

  g_return_val_if_fail (mime_type != NULL, NULL);

  /* This is a hack to allow directories to have icons in filechooser */
  if (strcmp ("inode/directory", mime_type) == 0)
    return xstrdup (mime_type);

  key = xstrconcat ("MIME\\DataBase\\Content Type\\", mime_type, NULL);
  content_type = get_registry_classes_key (key, L"Extension");
  g_free (key);

  return content_type;
}

xchar_t *
g_content_type_guess (const xchar_t  *filename,
                      const xuchar_t *data,
                      xsize_t         data_size,
                      xboolean_t     *result_uncertain)
{
  char *basename;
  char *type;
  char *dot;

  type = NULL;

  if (result_uncertain)
    *result_uncertain = FALSE;

  /* our test suite and potentially other code used -1 in the past, which is
   * not documented and not allowed; guard against that */
  g_return_val_if_fail (data_size != (xsize_t) -1, xstrdup ("*"));

  if (filename)
    {
      basename = g_path_get_basename (filename);
      dot = strrchr (basename, '.');
      if (dot)
        type = xstrdup (dot);
      g_free (basename);
    }

  if (type)
    return type;

  if (data && looks_like_text (data, data_size))
    return xstrdup (".txt");

  return xstrdup ("*");
}

xlist_t *
g_content_types_get_registered (void)
{
  DWORD index;
  wchar_t keyname[256];
  DWORD key_len;
  char *key_utf8;
  xlist_t *types;

  types = NULL;
  index = 0;
  key_len = 256;
  while (RegEnumKeyExW(HKEY_CLASSES_ROOT,
                       index,
                       keyname,
                       &key_len,
                       NULL,
                       NULL,
                       NULL,
                       NULL) == ERROR_SUCCESS)
    {
      key_utf8 = xutf16_to_utf8 (keyname, -1, NULL, NULL, NULL);
      if (key_utf8)
        {
          if (*key_utf8 == '.')
            types = xlist_prepend (types, key_utf8);
          else
            g_free (key_utf8);
        }
      index++;
      key_len = 256;
    }

  return xlist_reverse (types);
}

xchar_t **
g_content_type_guess_for_tree (xfile_t *root)
{
  /* FIXME: implement */
  return NULL;
}
