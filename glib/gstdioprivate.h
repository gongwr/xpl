/* gstdioprivate.h - Private GLib stdio functions
 *
 * Copyright 2017 Руслан Ижбулатов
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

#ifndef __G_STDIOPRIVATE_H__
#define __G_STDIOPRIVATE_H__

G_BEGIN_DECLS

#if defined (G_OS_WIN32)

typedef struct _gtimespec {
  xuint64_t tv_sec;
  xuint32_t tv_nsec;
} gtimespec;

struct _GWin32PrivateStat
{
  xuint32_t volume_serial;
  xuint64_t file_index;
  xuint64_t attributes;
  xuint64_t allocated_size;
  xuint32_t reparse_tag;

  xuint32_t st_dev;
  xuint32_t st_ino;
  xuint16_t st_mode;
  xuint16_t st_uid;
  xuint16_t st_gid;
  xuint32_t st_nlink;
  xuint64_t st_size;
  gtimespec st_ctim;
  gtimespec st_atim;
  gtimespec st_mtim;
};

typedef struct _GWin32PrivateStat GWin32PrivateStat;

int g_win32_stat_utf8     (const xchar_t       *filename,
                           GWin32PrivateStat *buf);

int g_win32_lstat_utf8    (const xchar_t       *filename,
                           GWin32PrivateStat *buf);

int g_win32_readlink_utf8 (const xchar_t       *filename,
                           xchar_t             *buf,
                           xsize_t              buf_size,
                           xchar_t            **alloc_buf,
                           xboolean_t           terminate);

int g_win32_fstat         (int                fd,
                           GWin32PrivateStat *buf);

#endif

G_END_DECLS

#endif /* __G_STDIOPRIVATE_H__ */
