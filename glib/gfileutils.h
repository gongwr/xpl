/* gfileutils.h - File utility functions
 *
 *  Copyright 2000 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __XFILEUTILS_H__
#define __XFILEUTILS_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glibconfig.h>
#include <glib/gerror.h>

G_BEGIN_DECLS

#define XFILE_ERROR xfile_error_quark ()

typedef enum
{
  XFILE_ERROR_EXIST,
  XFILE_ERROR_ISDIR,
  XFILE_ERROR_ACCES,
  XFILE_ERROR_NAMETOOLONG,
  XFILE_ERROR_NOENT,
  XFILE_ERROR_NOTDIR,
  XFILE_ERROR_NXIO,
  XFILE_ERROR_NODEV,
  XFILE_ERROR_ROFS,
  XFILE_ERROR_TXTBSY,
  XFILE_ERROR_FAULT,
  XFILE_ERROR_LOOP,
  XFILE_ERROR_NOSPC,
  XFILE_ERROR_NOMEM,
  XFILE_ERROR_MFILE,
  XFILE_ERROR_NFILE,
  XFILE_ERROR_BADF,
  XFILE_ERROR_INVAL,
  XFILE_ERROR_PIPE,
  XFILE_ERROR_AGAIN,
  XFILE_ERROR_INTR,
  XFILE_ERROR_IO,
  XFILE_ERROR_PERM,
  XFILE_ERROR_NOSYS,
  XFILE_ERROR_FAILED
} GFileError;

/* For backward-compat reasons, these are synced to an old
 * anonymous enum in libgnome. But don't use that enum
 * in new code.
 */
typedef enum
{
  XFILE_TEST_IS_REGULAR    = 1 << 0,
  XFILE_TEST_IS_SYMLINK    = 1 << 1,
  XFILE_TEST_IS_DIR        = 1 << 2,
  XFILE_TEST_IS_EXECUTABLE = 1 << 3,
  XFILE_TEST_EXISTS        = 1 << 4
} GFileTest;

/**
 * GFileSetContentsFlags:
 * @XFILE_SET_CONTENTS_NONE: No guarantees about file consistency or durability.
 *   The most dangerous setting, which is slightly faster than other settings.
 * @XFILE_SET_CONTENTS_CONSISTENT: Guarantee file consistency: after a crash,
 *   either the old version of the file or the new version of the file will be
 *   available, but not a mixture. On Unix systems this equates to an `fsync()`
 *   on the file and use of an atomic `rename()` of the new version of the file
 *   over the old.
 * @XFILE_SET_CONTENTS_DURABLE: Guarantee file durability: after a crash, the
 *   new version of the file will be available. On Unix systems this equates to
 *   an `fsync()` on the file (if %XFILE_SET_CONTENTS_CONSISTENT is unset), or
 *   the effects of %XFILE_SET_CONTENTS_CONSISTENT plus an `fsync()` on the
 *   directory containing the file after calling `rename()`.
 * @XFILE_SET_CONTENTS_ONLY_EXISTING: Only apply consistency and durability
 *   guarantees if the file already exists. This may speed up file operations
 *   if the file doesn’t currently exist, but may result in a corrupted version
 *   of the new file if the system crashes while writing it.
 *
 * Flags to pass to xfile_set_contents_full() to affect its safety and
 * performance.
 *
 * Since: 2.66
 */
typedef enum
{
  XFILE_SET_CONTENTS_NONE = 0,
  XFILE_SET_CONTENTS_CONSISTENT = 1 << 0,
  XFILE_SET_CONTENTS_DURABLE = 1 << 1,
  XFILE_SET_CONTENTS_ONLY_EXISTING = 1 << 2
} GFileSetContentsFlags
XPL_AVAILABLE_ENUMERATOR_IN_2_66;

XPL_AVAILABLE_IN_ALL
xquark     xfile_error_quark      (void);
/* So other code can generate a GFileError */
XPL_AVAILABLE_IN_ALL
GFileError xfile_error_from_errno (xint_t err_no);

XPL_AVAILABLE_IN_ALL
xboolean_t xfile_test         (const xchar_t  *filename,
                              GFileTest     test);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_get_contents (const xchar_t  *filename,
                              xchar_t       **contents,
                              xsize_t        *length,
                              xerror_t      **error);
XPL_AVAILABLE_IN_ALL
xboolean_t xfile_set_contents (const xchar_t *filename,
                              const xchar_t *contents,
                              xssize_t         length,
                              xerror_t       **error);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_2_66
xboolean_t xfile_set_contents_full (const xchar_t            *filename,
                                   const xchar_t            *contents,
                                   xssize_t                  length,
                                   GFileSetContentsFlags   flags,
                                   int                     mode,
                                   xerror_t                **error);
G_GNUC_END_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_ALL
xchar_t   *xfile_read_link    (const xchar_t  *filename,
                              xerror_t      **error);

/* Wrapper / workalike for mkdtemp() */
XPL_AVAILABLE_IN_2_30
xchar_t   *g_mkdtemp            (xchar_t        *tmpl);
XPL_AVAILABLE_IN_2_30
xchar_t   *g_mkdtemp_full       (xchar_t        *tmpl,
                               xint_t          mode);

/* Wrapper / workalike for mkstemp() */
XPL_AVAILABLE_IN_ALL
xint_t     g_mkstemp            (xchar_t        *tmpl);
XPL_AVAILABLE_IN_ALL
xint_t     g_mkstemp_full       (xchar_t        *tmpl,
                               xint_t          flags,
                               xint_t          mode);

/* Wrappers for g_mkstemp and g_mkdtemp() */
XPL_AVAILABLE_IN_ALL
xint_t     xfile_open_tmp      (const xchar_t  *tmpl,
                               xchar_t       **name_used,
                               xerror_t      **error);
XPL_AVAILABLE_IN_2_30
xchar_t   *g_dir_make_tmp       (const xchar_t  *tmpl,
                               xerror_t      **error);

XPL_AVAILABLE_IN_ALL
xchar_t   *g_build_path         (const xchar_t *separator,
                               const xchar_t *first_element,
                               ...) G_GNUC_MALLOC G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
xchar_t   *g_build_pathv        (const xchar_t  *separator,
                               xchar_t       **args) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xchar_t   *g_build_filename     (const xchar_t *first_element,
                               ...) G_GNUC_MALLOC G_GNUC_NULL_TERMINATED;
XPL_AVAILABLE_IN_ALL
xchar_t   *g_build_filenamev    (xchar_t      **args) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_2_56
xchar_t   *g_build_filename_valist (const xchar_t  *first_element,
                                  va_list      *args) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xint_t     g_mkdir_with_parents (const xchar_t *pathname,
                               xint_t         mode);

#ifdef G_OS_WIN32

/* On Win32, the canonical directory separator is the backslash, and
 * the search path separator is the semicolon. Note that also the
 * (forward) slash works as directory separator.
 */
#define X_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR || (c) == '/')

#else  /* !G_OS_WIN32 */

#define X_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR)

#endif /* !G_OS_WIN32 */

XPL_AVAILABLE_IN_ALL
xboolean_t     g_path_is_absolute (const xchar_t *file_name);
XPL_AVAILABLE_IN_ALL
const xchar_t *g_path_skip_root   (const xchar_t *file_name);

XPL_DEPRECATED_FOR(g_path_get_basename)
const xchar_t *g_basename         (const xchar_t *file_name);
#define g_dirname g_path_get_dirname XPL_DEPRECATED_MACRO_IN_2_26_FOR(g_path_get_dirname)

XPL_AVAILABLE_IN_ALL
xchar_t *g_get_current_dir   (void);
XPL_AVAILABLE_IN_ALL
xchar_t *g_path_get_basename (const xchar_t *file_name) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t *g_path_get_dirname  (const xchar_t *file_name) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_2_58
xchar_t *g_canonicalize_filename (const xchar_t *filename,
                                const xchar_t *relative_to) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __XFILEUTILS_H__ */
