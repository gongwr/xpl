/*
 * Copyright © 2015 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include <gio/glocalfilemonitor.h>
#include <gio/giomodule.h>
#include "glib-private.h"
#include <glib-unix.h>
#include <fam.h>

static xmutex_t         fam_lock;
static xboolean_t       fam_initialised;
static FAMConnection  fam_connection;
static xsource_t       *fam_source;

#define XTYPE_FAM_FILE_MONITOR      (g_fam_file_monitor_get_type ())
#define G_FAM_FILE_MONITOR(inst)     (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                                      XTYPE_FAM_FILE_MONITOR, GFamFileMonitor))

typedef xlocal_file_monitor_class_t GFamFileMonitorClass;

typedef struct
{
  xlocal_file_monitor_t parent_instance;

  FAMRequest request;
} GFamFileMonitor;

static xtype_t g_fam_file_monitor_get_type (void);
G_DEFINE_DYNAMIC_TYPE (GFamFileMonitor, g_fam_file_monitor, XTYPE_LOCAL_FILE_MONITOR)

static xboolean_t
g_fam_file_monitor_callback (xint_t         fd,
                             xio_condition_t condition,
                             xpointer_t     user_data)
{
  sint64_t now = xsource_get_time (fam_source);

  g_mutex_lock (&fam_lock);

  while (FAMPending (&fam_connection))
    {
      const xchar_t *child;
      FAMEvent ev;

      if (FAMNextEvent (&fam_connection, &ev) != 1)
        {
          /* The daemon died.  We're in a really bad situation now
           * because we potentially have a bunch of request structures
           * outstanding which no longer make any sense to anyone.
           *
           * The best thing that we can do is do nothing.  Notification
           * won't work anymore for this process.
           */
          g_mutex_unlock (&fam_lock);

          g_warning ("Lost connection to FAM (file monitoring) service.  Expect no further file monitor events.");

          return FALSE;
        }

      /* We expect ev.filename to be a relative path for children in a
       * monitored directory, and an absolute path for a monitored file
       * or the directory itself.
       */
      if (ev.filename[0] != '/')
        child = ev.filename;
      else
        child = NULL;

      switch (ev.code)
        {
        case FAMAcknowledge:
          xsource_unref (ev.userdata);
          break;

        case FAMChanged:
          xfile_monitor_source_handle_event (ev.userdata, XFILE_MONITOR_EVENT_CHANGED, child, NULL, NULL, now);
          break;

        case FAMDeleted:
          xfile_monitor_source_handle_event (ev.userdata, XFILE_MONITOR_EVENT_DELETED, child, NULL, NULL, now);
          break;

        case FAMCreated:
          xfile_monitor_source_handle_event (ev.userdata, XFILE_MONITOR_EVENT_CREATED, child, NULL, NULL, now);
          break;

        default:
          /* unknown type */
          break;
        }
    }

  g_mutex_unlock (&fam_lock);

  return TRUE;
}

static xboolean_t
g_fam_file_monitor_is_supported (void)
{
  g_mutex_lock (&fam_lock);

  if (!fam_initialised)
    {
      fam_initialised = FAMOpen2 (&fam_connection, "GLib GIO") == 0;

      if (fam_initialised)
        {
#ifdef HAVE_FAM_NO_EXISTS
          /* This is a gamin extension that avoids sending all the
           * Exists event for dir monitors
           */
          FAMNoExists (&fam_connection);
#endif

          fam_source = g_unix_fd_source_new (FAMCONNECTION_GETFD (&fam_connection), G_IO_IN);
          xsource_set_callback (fam_source, (xsource_func_t) g_fam_file_monitor_callback, NULL, NULL);
          xsource_attach (fam_source, XPL_PRIVATE_CALL(g_get_worker_context) ());
        }
    }

  g_mutex_unlock (&fam_lock);

  return fam_initialised;
}

static xboolean_t
g_fam_file_monitor_cancel (xfile_monitor_t *monitor)
{
  GFamFileMonitor *gffm = G_FAM_FILE_MONITOR (monitor);

  g_mutex_lock (&fam_lock);

  xassert (fam_initialised);

  FAMCancelMonitor (&fam_connection, &gffm->request);

  g_mutex_unlock (&fam_lock);

  return TRUE;
}

static void
g_fam_file_monitor_start (xlocal_file_monitor_t  *local_monitor,
                          const xchar_t        *dirname,
                          const xchar_t        *basename,
                          const xchar_t        *filename,
                          GFileMonitorSource *source)
{
  GFamFileMonitor *gffm = G_FAM_FILE_MONITOR (local_monitor);

  g_mutex_lock (&fam_lock);

  xassert (fam_initialised);

  xsource_ref ((xsource_t *) source);

  if (dirname)
    FAMMonitorDirectory (&fam_connection, dirname, &gffm->request, source);
  else
    FAMMonitorFile (&fam_connection, filename, &gffm->request, source);

  g_mutex_unlock (&fam_lock);
}

static void
g_fam_file_monitor_init (GFamFileMonitor* monitor)
{
}

static void
g_fam_file_monitor_class_init (GFamFileMonitorClass *class)
{
  xfile_monitor_class_t *file_monitor_class = XFILE_MONITOR_CLASS (class);

  class->is_supported = g_fam_file_monitor_is_supported;
  class->start = g_fam_file_monitor_start;
  file_monitor_class->cancel = g_fam_file_monitor_cancel;
}

static void
g_fam_file_monitor_class_finalize (GFamFileMonitorClass *class)
{
}

void
xio_module_load (xio_module_t *module)
{
  xtype_module_use (XTYPE_MODULE (module));

  g_fam_file_monitor_register_type (XTYPE_MODULE (module));

  g_io_extension_point_implement (G_LOCAL_FILE_MONITOR_EXTENSION_POINT_NAME,
                                 XTYPE_FAM_FILE_MONITOR, "fam", 10);

  g_io_extension_point_implement (G_NFS_FILE_MONITOR_EXTENSION_POINT_NAME,
                                 XTYPE_FAM_FILE_MONITOR, "fam", 10);
}

void
xio_module_unload (xio_module_t *module)
{
  g_assert_not_reached ();
}

char **
xio_module_query (void)
{
  char *eps[] = {
    G_LOCAL_FILE_MONITOR_EXTENSION_POINT_NAME,
    G_NFS_FILE_MONITOR_EXTENSION_POINT_NAME,
    NULL
  };

  return xstrdupv (eps);
}
