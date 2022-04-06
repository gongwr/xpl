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

#include "gpollfilemonitor.h"
#include "gfile.h"
#include "gfilemonitor.h"
#include "gfileinfo.h"


static xboolean_t g_poll_file_monitor_cancel (xfile_monitor_t* monitor);
static void schedule_poll_timeout (GPollFileMonitor* poll_monitor);

struct _GPollFileMonitor
{
  xfile_monitor_t parent_instance;
  xfile_t *file;
  xfile_info_t *last_info;
  xsource_t *timeout;
};

#define POLL_TIME_SECS 5

#define g_poll_file_monitor_get_type _g_poll_file_monitor_get_type
G_DEFINE_TYPE (GPollFileMonitor, g_poll_file_monitor, XTYPE_FILE_MONITOR)

static void
g_poll_file_monitor_finalize (xobject_t* object)
{
  GPollFileMonitor* poll_monitor;

  poll_monitor = G_POLL_FILE_MONITOR (object);

  g_poll_file_monitor_cancel (XFILE_MONITOR (poll_monitor));
  xobject_unref (poll_monitor->file);
  g_clear_object (&poll_monitor->last_info);

  G_OBJECT_CLASS (g_poll_file_monitor_parent_class)->finalize (object);
}


static void
g_poll_file_monitor_class_init (GPollFileMonitorClass* klass)
{
  xobject_class_t* gobject_class = G_OBJECT_CLASS (klass);
  xfile_monitor_class_t *file_monitor_class = XFILE_MONITOR_CLASS (klass);

  gobject_class->finalize = g_poll_file_monitor_finalize;

  file_monitor_class->cancel = g_poll_file_monitor_cancel;
}

static void
g_poll_file_monitor_init (GPollFileMonitor* poll_monitor)
{
}

static int
calc_event_type (xfile_info_t *last,
		 xfile_info_t *new)
{
  if (last == NULL && new == NULL)
    return -1;

  if (last == NULL && new != NULL)
    return XFILE_MONITOR_EVENT_CREATED;

  if (last != NULL && new == NULL)
    return XFILE_MONITOR_EVENT_DELETED;

  if (xstrcmp0 (xfile_info_get_etag (last), xfile_info_get_etag (new)))
    return XFILE_MONITOR_EVENT_CHANGED;

  if (xfile_info_get_size (last) != xfile_info_get_size (new))
    return XFILE_MONITOR_EVENT_CHANGED;

  return -1;
}

static void
got_new_info (xobject_t      *source_object,
              xasync_result_t *res,
              xpointer_t      user_data)
{
  GPollFileMonitor* poll_monitor = user_data;
  xfile_info_t *info;
  int event;

  info = xfile_query_info_finish (poll_monitor->file, res, NULL);

  if (!xfile_monitor_is_cancelled (XFILE_MONITOR (poll_monitor)))
    {
      event = calc_event_type (poll_monitor->last_info, info);

      if (event != -1)
	{
	  xfile_monitor_emit_event (XFILE_MONITOR (poll_monitor),
				     poll_monitor->file,
				     NULL, event);
	  /* We're polling so slowly anyway, so always emit the done hint */
	  if (!xfile_monitor_is_cancelled (XFILE_MONITOR (poll_monitor)) &&
             (event == XFILE_MONITOR_EVENT_CHANGED || event == XFILE_MONITOR_EVENT_CREATED))
	    xfile_monitor_emit_event (XFILE_MONITOR (poll_monitor),
				       poll_monitor->file,
				       NULL, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT);
	}

      if (poll_monitor->last_info)
	{
	  xobject_unref (poll_monitor->last_info);
	  poll_monitor->last_info = NULL;
	}

      if (info)
	poll_monitor->last_info = xobject_ref (info);

      schedule_poll_timeout (poll_monitor);
    }

  if (info)
    xobject_unref (info);

  xobject_unref (poll_monitor);
}

static xboolean_t
poll_file_timeout (xpointer_t data)
{
  GPollFileMonitor* poll_monitor = data;

  xsource_unref (poll_monitor->timeout);
  poll_monitor->timeout = NULL;

  xfile_query_info_async (poll_monitor->file, XFILE_ATTRIBUTE_ETAG_VALUE "," XFILE_ATTRIBUTE_STANDARD_SIZE,
			 0, 0, NULL, got_new_info, xobject_ref (poll_monitor));

  return G_SOURCE_REMOVE;
}

static void
schedule_poll_timeout (GPollFileMonitor* poll_monitor)
{
  poll_monitor->timeout = g_timeout_source_new_seconds (POLL_TIME_SECS);
  xsource_set_callback (poll_monitor->timeout, poll_file_timeout, poll_monitor, NULL);
  xsource_attach (poll_monitor->timeout, xmain_context_get_thread_default ());
}

static void
got_initial_info (xobject_t      *source_object,
                  xasync_result_t *res,
                  xpointer_t      user_data)
{
  GPollFileMonitor* poll_monitor = user_data;
  xfile_info_t *info;

  info = xfile_query_info_finish (poll_monitor->file, res, NULL);

  poll_monitor->last_info = info;

  if (!xfile_monitor_is_cancelled (XFILE_MONITOR (poll_monitor)))
    schedule_poll_timeout (poll_monitor);

  xobject_unref (poll_monitor);
}

/**
 * _g_poll_file_monitor_new:
 * @file: a #xfile_t.
 *
 * Polls @file for changes.
 *
 * Returns: a new #xfile_monitor_t for the given #xfile_t.
 **/
xfile_monitor_t*
_g_poll_file_monitor_new (xfile_t *file)
{
  GPollFileMonitor* poll_monitor;

  poll_monitor = xobject_new (XTYPE_POLL_FILE_MONITOR, NULL);

  poll_monitor->file = xobject_ref (file);

  xfile_query_info_async (file, XFILE_ATTRIBUTE_ETAG_VALUE "," XFILE_ATTRIBUTE_STANDARD_SIZE,
			   0, 0, NULL, got_initial_info, xobject_ref (poll_monitor));

  return XFILE_MONITOR (poll_monitor);
}

static xboolean_t
g_poll_file_monitor_cancel (xfile_monitor_t* monitor)
{
  GPollFileMonitor *poll_monitor = G_POLL_FILE_MONITOR (monitor);

  if (poll_monitor->timeout)
    {
      xsource_destroy (poll_monitor->timeout);
      xsource_unref (poll_monitor->timeout);
      poll_monitor->timeout = NULL;
    }

  return TRUE;
}
