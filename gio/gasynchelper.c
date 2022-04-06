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

#include "gasynchelper.h"


/*< private >
 * SECTION:gasynchelper
 * @short_description: Asynchronous Helper Functions
 * @include: gio/gio.h
 * @see_also: #xasync_result_t
 *
 * Provides helper functions for asynchronous operations.
 *
 **/

#ifdef G_OS_WIN32
xboolean_t
_g_win32_overlap_wait_result (HANDLE           hfile,
                              OVERLAPPED      *overlap,
                              DWORD           *transferred,
                              xcancellable_t    *cancellable)
{
  xpollfd_t pollfd[2];
  xboolean_t result = FALSE;
  xint_t num, npoll;

#if XPL_SIZEOF_VOID_P == 8
  pollfd[0].fd = (sint64_t)overlap->hEvent;
#else
  pollfd[0].fd = (xint_t)overlap->hEvent;
#endif
  pollfd[0].events = G_IO_IN;
  num = 1;

  if (xcancellable_make_pollfd (cancellable, &pollfd[1]))
    num++;

loop:
  npoll = g_poll (pollfd, num, -1);
  if (npoll <= 0)
    /* error out, should never happen */
    goto end;

  if (xcancellable_is_cancelled (cancellable))
    {
      /* CancelIO only cancels pending operations issued by the
       * current thread and since we're doing only sync operations,
       * this is safe.... */
      /* CancelIoEx is only Vista+. Since we have only one overlap
       * operation on this thread, we can just use: */
      result = CancelIo (hfile);
      g_warn_if_fail (result);
    }

  result = GetOverlappedResult (overlap->hEvent, overlap, transferred, FALSE);
  if (result == FALSE &&
      GetLastError () == ERROR_IO_INCOMPLETE &&
      !xcancellable_is_cancelled (cancellable))
    goto loop;

end:
  if (num > 1)
    xcancellable_release_fd (cancellable);

  return result;
}
#endif
