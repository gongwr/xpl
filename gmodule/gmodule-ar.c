/* GMODULE - XPL wrapper code for dynamic module loading
 * Copyright (C) 1998, 2000 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MT safe
 */

/* because we are compatible with archive format only since AIX 4.3 */

#define __AR_BIG__

#include "config.h"

#include <ar.h>
#include <stdlib.h>

#include <dlfcn.h>

/* --- functions --- */
static xchar_t*
fetch_dlerror (xboolean_t replace_null)
{
  xchar_t *msg = dlerror ();

  /* make sure we always return an error message != NULL, if
   * expected to do so. */

  if (!msg && replace_null)
    return "unknown dl-error";

  return msg;
}

static xchar_t* _g_module_get_member(const xchar_t* file_name)
{
  xchar_t* member = NULL;
  struct fl_hdr file_header;
  struct ar_hdr ar_header;
  long first_member;
  long name_len;
  int fd;

  fd = open(file_name, O_RDONLY);
  if (fd == -1)
    return NULL;

  if (read(fd, (void*)&file_header, FL_HSZ) != FL_HSZ)
    goto exit;

  if (strncmp(file_header.fl_magic, AIAMAGBIG, SAIAMAG) != 0)
    goto exit;

  /* read first archive file member header */

  first_member = atol(file_header.fl_fstmoff);

  if (lseek(fd, first_member, SEEK_SET) != first_member)
    goto exit;

  if (read(fd, (void*)&ar_header, AR_HSZ - 2) != AR_HSZ - 2)
    goto exit;

  /* read member name */

  name_len = atol(ar_header.ar_namlen);

  member = g_malloc(name_len+1);
  if (!member)
    goto exit;

  if (read(fd, (void*)member, name_len) != name_len)
    {
      g_free(member);
      member = NULL;
      goto exit;
    }

  member[name_len] = 0;

exit:
  close(fd);

  return member;
}

static xpointer_t
_g_module_open (const xchar_t *file_name,
		xboolean_t     bind_lazy,
		xboolean_t     bind_local,
                xerror_t     **error)
{
  xpointer_t handle;
  xchar_t* member;
  xchar_t* full_name;

  /* extract name of first member of archive */

  member = _g_module_get_member (file_name);
  if (member != NULL)
    {
      full_name = xstrconcat (file_name, "(", member, ")", NULL);
      g_free (member);
    }
  else
    full_name = xstrdup (file_name);

  handle = dlopen (full_name,
		   (bind_local ? RTLD_LOCAL : RTLD_GLOBAL) | RTLD_MEMBER | (bind_lazy ? RTLD_LAZY : RTLD_NOW));

  g_free (full_name);

  if (!handle)
    {
      const xchar_t *message = fetch_dlerror (TRUE);

      g_module_set_error (message);
      g_set_error_literal (error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED, message);
    }

  return handle;
}

static xpointer_t
_g_module_self (void)
{
  xpointer_t handle;

  handle = dlopen (NULL, RTLD_GLOBAL | RTLD_LAZY);
  if (!handle)
    g_module_set_error (fetch_dlerror (TRUE));

  return handle;
}

static void
_g_module_close (xpointer_t handle)
{
  if (dlclose (handle) != 0)
    g_module_set_error (fetch_dlerror (TRUE));
}

static xpointer_t
_g_module_symbol (xpointer_t     handle,
		  const xchar_t *symbol_name)
{
  xpointer_t p;

  p = dlsym (handle, symbol_name);
  if (!p)
    g_module_set_error (fetch_dlerror (FALSE));

  return p;
}

static xchar_t*
_g_module_build_path (const xchar_t *directory,
		      const xchar_t *module_name)
{
  if (directory && *directory) {
    if (strncmp (module_name, "lib", 3) == 0)
      return xstrconcat (directory, "/", module_name, NULL);
    else
      return xstrconcat (directory, "/lib", module_name, "." G_MODULE_SUFFIX, NULL);
  } else if (strncmp (module_name, "lib", 3) == 0)
    return xstrdup (module_name);
  else
    return xstrconcat ("lib", module_name, "." G_MODULE_SUFFIX, NULL);
}
