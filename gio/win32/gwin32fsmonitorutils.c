/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2015 Chun-wei Fan
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
 * Author: Vlad Grecescu <b100dian@gmail.com>
 * Author: Chun-wei Fan <fanc999@yahoo.com.tw>
 *
 */

#include "config.h"

#include "gwin32fsmonitorutils.h"
#include "gio/gfile.h"

#include <windows.h>

#define MAX_PATH_LONG 32767 /* Support Paths longer than MAX_PATH (260) characters */

static xboolean_t
g_win32_fs_monitor_handle_event (GWin32FSMonitorPrivate   *monitor,
                                 const xchar_t              *filename,
                                 PFILE_NOTIFY_INFORMATION  pfni)
{
  xfile_monitor_event_t fme;
  PFILE_NOTIFY_INFORMATION pfni_next;
  WIN32_FILE_ATTRIBUTE_DATA attrib_data = {0, };
  xchar_t *renamed_file = NULL;

  switch (pfni->Action)
    {
    case FILE_ACTION_ADDED:
      fme = XFILE_MONITOR_EVENT_CREATED;
      break;

    case FILE_ACTION_REMOVED:
      fme = XFILE_MONITOR_EVENT_DELETED;
      break;

    case FILE_ACTION_MODIFIED:
      {
        xboolean_t success_attribs = GetFileAttributesExW (monitor->wfullpath_with_long_prefix,
                                                         GetFileExInfoStandard,
                                                         &attrib_data);

        if (monitor->file_attribs != INVALID_FILE_ATTRIBUTES &&
            success_attribs &&
            attrib_data.dwFileAttributes != monitor->file_attribs)
          fme = XFILE_MONITOR_EVENT_ATTRIBUTE_CHANGED;
        else
          fme = XFILE_MONITOR_EVENT_CHANGED;

        monitor->file_attribs = attrib_data.dwFileAttributes;
      }
      break;

    case FILE_ACTION_RENAMED_OLD_NAME:
      if (pfni->NextEntryOffset != 0)
        {
          /* If the file was renamed in the same directory, we would get a
           * FILE_ACTION_RENAMED_NEW_NAME action in the next FILE_NOTIFY_INFORMATION
           * structure.
           */
          xlong_t file_name_len = 0;

          pfni_next = (PFILE_NOTIFY_INFORMATION) ((BYTE*)pfni + pfni->NextEntryOffset);
          renamed_file = xutf16_to_utf8 (pfni_next->FileName, pfni_next->FileNameLength / sizeof(WCHAR), NULL, &file_name_len, NULL);
          if (pfni_next->Action == FILE_ACTION_RENAMED_NEW_NAME)
           fme = XFILE_MONITOR_EVENT_RENAMED;
          else
           fme = XFILE_MONITOR_EVENT_MOVED_OUT;
        }
      else
        fme = XFILE_MONITOR_EVENT_MOVED_OUT;
      break;

    case FILE_ACTION_RENAMED_NEW_NAME:
      if (monitor->pfni_prev != NULL &&
          monitor->pfni_prev->Action == FILE_ACTION_RENAMED_OLD_NAME)
        {
          /* don't bother sending events, was already sent (rename) */
          fme = (xfile_monitor_event_t) -1;
        }
      else
        fme = XFILE_MONITOR_EVENT_MOVED_IN;
      break;

    default:
      /* The possible Windows actions are all above, so shouldn't get here */
      g_assert_not_reached ();
      break;
    }

  if (fme != (xfile_monitor_event_t) -1)
    return xfile_monitor_source_handle_event (monitor->fms,
                                               fme,
                                               filename,
                                               renamed_file,
                                               NULL,
                                               g_get_monotonic_time ());
  else
    return FALSE;
}


static void CALLBACK
g_win32_fs_monitor_callback (DWORD        error,
                             DWORD        nBytes,
                             LPOVERLAPPED lpOverlapped)
{
  xulong_t offset;
  PFILE_NOTIFY_INFORMATION pfile_notify_walker;
  GWin32FSMonitorPrivate *monitor = (GWin32FSMonitorPrivate *) lpOverlapped;

  DWORD notify_filter = monitor->isfile ?
                        (FILE_NOTIFY_CHANGE_FILE_NAME |
                         FILE_NOTIFY_CHANGE_ATTRIBUTES |
                         FILE_NOTIFY_CHANGE_SIZE) :
                        (FILE_NOTIFY_CHANGE_FILE_NAME |
                         FILE_NOTIFY_CHANGE_DIR_NAME |
                         FILE_NOTIFY_CHANGE_ATTRIBUTES |
                         FILE_NOTIFY_CHANGE_SIZE);

  /* If monitor->self is NULL the GWin32FileMonitor object has been destroyed. */
  if (monitor->self == NULL ||
      xfile_monitor_is_cancelled (monitor->self) ||
      monitor->file_notify_buffer == NULL)
    {
      g_free (monitor->file_notify_buffer);
      g_free (monitor);
      return;
    }

  offset = 0;

  do
    {
      pfile_notify_walker = (PFILE_NOTIFY_INFORMATION)((BYTE *)monitor->file_notify_buffer + offset);
      if (pfile_notify_walker->Action > 0)
        {
          xlong_t file_name_len;
          xchar_t *changed_file;

          changed_file = xutf16_to_utf8 (pfile_notify_walker->FileName,
                                          pfile_notify_walker->FileNameLength / sizeof(WCHAR),
                                          NULL, &file_name_len, NULL);

          if (monitor->isfile)
            {
              xint_t lonxfilename_length = wcslen (monitor->wfilename_long);
              xint_t short_filename_length = wcslen (monitor->wfilename_short);
              enum GWin32FileMonitorFileAlias alias_state;

              /* If monitoring a file, check that the changed file
              * in the directory matches the file that is to be monitored
              * We need to check both the long and short file names for the same file.
              *
              * We need to send in the name of the monitored file, not its long (or short) variant,
              * if they exist.
              */

              if (_wcsnicmp (pfile_notify_walker->FileName,
                             monitor->wfilename_long,
                             lonxfilename_length) == 0)
                {
                  if (_wcsnicmp (pfile_notify_walker->FileName,
                                 monitor->wfilename_short,
                                 short_filename_length) == 0)
                    {
                      alias_state = G_WIN32_FILE_MONITOR_NO_ALIAS;
                    }
                  else
                    alias_state = G_WIN32_FILE_MONITOR_LONG_FILENAME;
                }
              else if (_wcsnicmp (pfile_notify_walker->FileName,
                                  monitor->wfilename_short,
                                  short_filename_length) == 0)
                {
                  alias_state = G_WIN32_FILE_MONITOR_SHORT_FILENAME;
                }
              else
                alias_state = G_WIN32_FILE_MONITOR_NO_MATCH_FOUND;

              if (alias_state != G_WIN32_FILE_MONITOR_NO_MATCH_FOUND)
                {
                  wchar_t *monitored_file_w;
                  xchar_t *monitored_file;

                  switch (alias_state)
                    {
                    case G_WIN32_FILE_MONITOR_NO_ALIAS:
                      monitored_file = xstrdup (changed_file);
                      break;
                    case G_WIN32_FILE_MONITOR_LONG_FILENAME:
                    case G_WIN32_FILE_MONITOR_SHORT_FILENAME:
                      monitored_file_w = wcsrchr (monitor->wfullpath_with_long_prefix, L'\\');
                      monitored_file = xutf16_to_utf8 (monitored_file_w + 1, -1, NULL, NULL, NULL);
                      break;
                    default:
                      g_assert_not_reached ();
                      break;
                    }

                  g_win32_fs_monitor_handle_event (monitor, monitored_file, pfile_notify_walker);
                  g_free (monitored_file);
                }
            }
          else
            g_win32_fs_monitor_handle_event (monitor, changed_file, pfile_notify_walker);

          g_free (changed_file);
        }

      monitor->pfni_prev = pfile_notify_walker;
      offset += pfile_notify_walker->NextEntryOffset;
    }
  while (pfile_notify_walker->NextEntryOffset);

  ReadDirectoryChangesW (monitor->hDirectory,
                         monitor->file_notify_buffer,
                         monitor->buffer_allocated_bytes,
                         FALSE,
                         notify_filter,
                         &monitor->buffer_filled_bytes,
                         &monitor->overlapped,
                         g_win32_fs_monitor_callback);
}

void
g_win32_fs_monitor_init (GWin32FSMonitorPrivate *monitor,
                         const xchar_t *dirname,
                         const xchar_t *filename,
                         xboolean_t isfile)
{
  wchar_t *wdirname_with_long_prefix = NULL;
  const xchar_t LONGPFX[] = "\\\\?\\";
  xchar_t *fullpath_with_long_prefix, *dirname_with_long_prefix;
  DWORD notify_filter = isfile ?
                        (FILE_NOTIFY_CHANGE_FILE_NAME |
                         FILE_NOTIFY_CHANGE_ATTRIBUTES |
                         FILE_NOTIFY_CHANGE_SIZE) :
                        (FILE_NOTIFY_CHANGE_FILE_NAME |
                         FILE_NOTIFY_CHANGE_DIR_NAME |
                         FILE_NOTIFY_CHANGE_ATTRIBUTES |
                         FILE_NOTIFY_CHANGE_SIZE);

  xboolean_t success_attribs;
  WIN32_FILE_ATTRIBUTE_DATA attrib_data = {0, };


  if (dirname != NULL)
    {
      dirname_with_long_prefix = xstrconcat (LONGPFX, dirname, NULL);
      wdirname_with_long_prefix = xutf8_to_utf16 (dirname_with_long_prefix, -1, NULL, NULL, NULL);

      if (isfile)
        {
          xchar_t *fullpath;
          wchar_t wlongname[MAX_PATH_LONG];
          wchar_t wshortname[MAX_PATH_LONG];
          wchar_t *wfullpath, *wbasename_long, *wbasename_short;

          fullpath = g_build_filename (dirname, filename, NULL);
          fullpath_with_long_prefix = xstrconcat (LONGPFX, fullpath, NULL);

          wfullpath = xutf8_to_utf16 (fullpath, -1, NULL, NULL, NULL);

          monitor->wfullpath_with_long_prefix =
            xutf8_to_utf16 (fullpath_with_long_prefix, -1, NULL, NULL, NULL);

          /* ReadDirectoryChangesW() can return the normal filename or the
           * "8.3" format filename, so we need to keep track of both these names
           * so that we can check against them later when it returns
           */
          if (GetLongPathNameW (monitor->wfullpath_with_long_prefix, wlongname, MAX_PATH_LONG) == 0)
            {
              wbasename_long = wcsrchr (monitor->wfullpath_with_long_prefix, L'\\');
              monitor->wfilename_long = wbasename_long != NULL ?
                                        wcsdup (wbasename_long + 1) :
                                        wcsdup (wfullpath);
            }
          else
            {
              wbasename_long = wcsrchr (wlongname, L'\\');
              monitor->wfilename_long = wbasename_long != NULL ?
                                        wcsdup (wbasename_long + 1) :
                                        wcsdup (wlongname);

            }

          if (GetShortPathNameW (monitor->wfullpath_with_long_prefix, wshortname, MAX_PATH_LONG) == 0)
            {
              wbasename_short = wcsrchr (monitor->wfullpath_with_long_prefix, L'\\');
              monitor->wfilename_short = wbasename_short != NULL ?
                                         wcsdup (wbasename_short + 1) :
                                         wcsdup (wfullpath);
            }
          else
            {
              wbasename_short = wcsrchr (wshortname, L'\\');
              monitor->wfilename_short = wbasename_short != NULL ?
                                         wcsdup (wbasename_short + 1) :
                                         wcsdup (wshortname);
            }

          g_free (fullpath);
        }
      else
        {
          monitor->wfilename_short = NULL;
          monitor->wfilename_long = NULL;
          monitor->wfullpath_with_long_prefix = xutf8_to_utf16 (dirname_with_long_prefix, -1, NULL, NULL, NULL);
        }

      monitor->isfile = isfile;
    }
  else
    {
      dirname_with_long_prefix = xstrconcat (LONGPFX, filename, NULL);
      monitor->wfullpath_with_long_prefix = xutf8_to_utf16 (dirname_with_long_prefix, -1, NULL, NULL, NULL);
      monitor->wfilename_long = NULL;
      monitor->wfilename_short = NULL;
      monitor->isfile = FALSE;
    }

  success_attribs = GetFileAttributesExW (monitor->wfullpath_with_long_prefix,
                                          GetFileExInfoStandard,
                                          &attrib_data);
  if (success_attribs)
    monitor->file_attribs = attrib_data.dwFileAttributes; /* Store up original attributes */
  else
    monitor->file_attribs = INVALID_FILE_ATTRIBUTES;
  monitor->pfni_prev = NULL;
  monitor->hDirectory = CreateFileW (wdirname_with_long_prefix != NULL ? wdirname_with_long_prefix : monitor->wfullpath_with_long_prefix,
                                     FILE_LIST_DIRECTORY,
                                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                     NULL);

  g_free (wdirname_with_long_prefix);
  g_free (dirname_with_long_prefix);

  if (monitor->hDirectory != INVALID_HANDLE_VALUE)
    {
      ReadDirectoryChangesW (monitor->hDirectory,
                             monitor->file_notify_buffer,
                             monitor->buffer_allocated_bytes,
                             FALSE,
                             notify_filter,
                             &monitor->buffer_filled_bytes,
                             &monitor->overlapped,
                             g_win32_fs_monitor_callback);
    }
}

GWin32FSMonitorPrivate *
g_win32_fs_monitor_create (xboolean_t isfile)
{
  GWin32FSMonitorPrivate *monitor = g_new0 (GWin32FSMonitorPrivate, 1);

  monitor->buffer_allocated_bytes = 32784;
  monitor->file_notify_buffer = g_new0 (FILE_NOTIFY_INFORMATION, monitor->buffer_allocated_bytes);

  return monitor;
}

void
g_win32_fs_monitor_finalize (GWin32FSMonitorPrivate *monitor)
{
  g_free (monitor->wfullpath_with_long_prefix);
  g_free (monitor->wfilename_long);
  g_free (monitor->wfilename_short);

  if (monitor->hDirectory == INVALID_HANDLE_VALUE)
    {
      /* If we don't have a directory handle we can free
       * monitor->file_notify_buffer and monitor here. The
       * callback won't be called obviously any more (and presumably
       * never has been called).
       */
      g_free (monitor->file_notify_buffer);
      monitor->file_notify_buffer = NULL;
      g_free (monitor);
    }
  else
    {
      /* If we have a directory handle, the OVERLAPPED struct is
       * passed once more to the callback as a result of the
       * CloseHandle() done in the cancel method, so monitor has to
       * be kept around. The GWin32DirectoryMonitor object is
       * disappearing, so can't leave a pointer to it in
       * monitor->self.
       */
      monitor->self = NULL;
    }
}

void
g_win32_fs_monitor_close_handle (GWin32FSMonitorPrivate *monitor)
{
  /* This triggers a last callback() with nBytes==0. */

  /* Actually I am not so sure about that, it seems to trigger a last
   * callback correctly, but the way to recognize that it is the final
   * one is not to check for nBytes==0, I think that was a
   * misunderstanding.
   */
  if (monitor->hDirectory != INVALID_HANDLE_VALUE)
    CloseHandle (monitor->hDirectory);
}
