/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 8 -*- */

/* inotify-helper.c - GVFS Monitor based on inotify.

   Copyright (C) 2007 John McCutchan

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.

   Authors:
		 John McCutchan <john@johnmccutchan.com>
*/

#include "config.h"
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
/* Just include the local header to stop all the pain */
#include <sys/inotify.h>
#include <gio/glocalfilemonitor.h>
#include <gio/gfile.h>
#include "inotify-helper.h"
#include "inotify-missing.h"
#include "inotify-path.h"

static xboolean_t ih_debug_enabled = FALSE;
#define IH_W if (ih_debug_enabled) g_warning

static xboolean_t ih_event_callback (ik_event_t  *event,
                                   inotify_sub *sub,
                                   xboolean_t     file_event);
static void ih_not_missing_callback (inotify_sub *sub);

/* We share this lock with inotify-kernel.c and inotify-missing.c
 *
 * inotify-kernel.c takes the lock when it reads events from
 * the kernel and when it processes those events
 *
 * inotify-missing.c takes the lock when it is scanning the missing
 * list.
 *
 * We take the lock in all public functions
 */
G_LOCK_DEFINE (inotify_lock);

static xfile_monitor_event_t ih_mask_to_EventFlags (xuint32_t mask);

/**
 * _ih_startup:
 *
 * Initializes the inotify backend.  This must be called before
 * any other functions in this module.
 *
 * Returns: #TRUE if initialization succeeded, #FALSE otherwise
 */
xboolean_t
_ih_startup (void)
{
  static xboolean_t initialized = FALSE;
  static xboolean_t result = FALSE;

  G_LOCK (inotify_lock);

  if (initialized == TRUE)
    {
      G_UNLOCK (inotify_lock);
      return result;
    }

  result = _ip_startup (ih_event_callback);
  if (!result)
    {
      G_UNLOCK (inotify_lock);
      return FALSE;
    }
  _im_startup (ih_not_missing_callback);

  IH_W ("started gvfs inotify backend\n");

  initialized = TRUE;

  G_UNLOCK (inotify_lock);

  return TRUE;
}

/*
 * Adds a subscription to be monitored.
 */
xboolean_t
_ih_sub_add (inotify_sub *sub)
{
  G_LOCK (inotify_lock);

  if (!_ip_start_watching (sub))
    _im_add (sub);

  G_UNLOCK (inotify_lock);

  return TRUE;
}

/*
 * Cancels a subscription which was being monitored.
 */
xboolean_t
_ih_sub_cancel (inotify_sub *sub)
{
  G_LOCK (inotify_lock);

  if (!sub->cancelled)
    {
      IH_W ("cancelling %s\n", sub->dirname);
      sub->cancelled = TRUE;
      _im_rm (sub);
      _ip_stop_watching (sub);
    }

  G_UNLOCK (inotify_lock);

  return TRUE;
}

static char *
_ih_fullpath_from_event (ik_event_t *event,
			 const char *dirname,
			 const char *filename)
{
  char *fullpath;

  if (filename)
    fullpath = xstrdup_printf ("%s/%s", dirname, filename);
  else if (event->name)
    fullpath = xstrdup_printf ("%s/%s", dirname, event->name);
  else
    fullpath = xstrdup_printf ("%s/", dirname);

   return fullpath;
}

static xboolean_t
ih_event_callback (ik_event_t  *event,
                   inotify_sub *sub,
                   xboolean_t     file_event)
{
  xboolean_t interesting;
  xfile_monitor_event_t event_flags;

  event_flags = ih_mask_to_EventFlags (event->mask);

  if (event->mask & IN_MOVE)
    {
      /* We either have a rename (in the same directory) or a move
       * (between different directories).
       */
      if (event->pair && event->pair->wd == event->wd)
        {
          /* this is a rename */
          interesting = xfile_monitor_source_handle_event (sub->user_data, XFILE_MONITOR_EVENT_RENAMED,
                                                            event->name, event->pair->name, NULL, event->timestamp);
        }
      else
        {
          xfile_t *other;

          if (event->pair)
            {
              const char *parent_dir;
              xchar_t *fullpath;

              parent_dir = _ip_get_path_for_wd (event->pair->wd);
              fullpath = _ih_fullpath_from_event (event->pair, parent_dir, NULL);
              other = xfile_new_for_path (fullpath);
              g_free (fullpath);
            }
          else
            other = NULL;

          /* This is either an incoming or outgoing move. Since we checked the
           * event->mask above, it should have converted to a #xfile_monitor_event_t
           * properly. If not, the assumption we have made about event->mask
           * only ever having a single bit set (apart from IN_ISDIR) is false.
           * The kernel documentation is lacking here. */
          xassert ((int) event_flags != -1);
          interesting = xfile_monitor_source_handle_event (sub->user_data, event_flags,
                                                            event->name, NULL, other, event->timestamp);

          if (other)
            xobject_unref (other);
        }
    }
  else if ((int) event_flags != -1)
    /* unpaired event -- no 'other' field */
    interesting = xfile_monitor_source_handle_event (sub->user_data, event_flags,
                                                      event->name, NULL, NULL, event->timestamp);
  else
    interesting = FALSE;

  if (event->mask & IN_CREATE)
    {
      const xchar_t *parent_dir;
      xchar_t *fullname;
      struct stat buf;
      xint_t s;

      /* The kernel reports IN_CREATE for two types of events:
       *
       *  - creat(), in which case IN_CLOSE_WRITE will come soon; or
       *  - link(), mkdir(), mknod(), etc., in which case it won't
       *
       * We can attempt to detect the second case and send the
       * CHANGES_DONE immediately so that the user isn't left waiting.
       *
       * The detection for link() is not 100% reliable since the link
       * count could be 1 if the original link was deleted or if
       * O_TMPFILE was being used, but in that case the virtual
       * CHANGES_DONE will be emitted to close the loop.
       */

      parent_dir = _ip_get_path_for_wd (event->wd);
      fullname = _ih_fullpath_from_event (event, parent_dir, NULL);
      s = stat (fullname, &buf);
      g_free (fullname);

      /* if it doesn't look like the result of creat()... */
      if (s != 0 || !S_ISREG (buf.st_mode) || buf.st_nlink != 1)
        xfile_monitor_source_handle_event (sub->user_data, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT,
                                            event->name, NULL, NULL, event->timestamp);
    }

  return interesting;
}

static void
ih_not_missing_callback (inotify_sub *sub)
{
  xint_t now = g_get_monotonic_time ();

  xfile_monitor_source_handle_event (sub->user_data, XFILE_MONITOR_EVENT_CREATED,
                                      sub->filename, NULL, NULL, now);
  xfile_monitor_source_handle_event (sub->user_data, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT,
                                      sub->filename, NULL, NULL, now);
}

/* Transforms a inotify event to a GVFS event. */
static xfile_monitor_event_t
ih_mask_to_EventFlags (xuint32_t mask)
{
  mask &= ~IN_ISDIR;
  switch (mask)
    {
    case IN_MODIFY:
      return XFILE_MONITOR_EVENT_CHANGED;
    case IN_CLOSE_WRITE:
      return XFILE_MONITOR_EVENT_CHANGES_DONE_HINT;
    case IN_ATTRIB:
      return XFILE_MONITOR_EVENT_ATTRIBUTE_CHANGED;
    case IN_MOVE_SELF:
    case IN_DELETE:
    case IN_DELETE_SELF:
      return XFILE_MONITOR_EVENT_DELETED;
    case IN_CREATE:
      return XFILE_MONITOR_EVENT_CREATED;
    case IN_MOVED_FROM:
      return XFILE_MONITOR_EVENT_MOVED_OUT;
    case IN_MOVED_TO:
      return XFILE_MONITOR_EVENT_MOVED_IN;
    case IN_UNMOUNT:
      return XFILE_MONITOR_EVENT_UNMOUNTED;
    case IN_Q_OVERFLOW:
    case IN_OPEN:
    case IN_CLOSE_NOWRITE:
    case IN_ACCESS:
    case IN_IGNORED:
    default:
      return -1;
    }
}
