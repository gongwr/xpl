/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2022 Red Hat, Inc.
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
 */

#include "config.h"

#include "gmemorymonitor.h"
#include "gioerror.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gcancellable.h"

#include <windows.h>

#define XTYPE_MEMORY_MONITOR_WIN32 (xmemory_monitor_win32_get_type ())
G_DECLARE_FINAL_TYPE (GMemoryMonitorWin32, xmemory_monitor_win32, G, MEMORY_MONITOR_WIN32, xobject_t)

#define G_MEMORY_MONITOR_WIN32_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, xinitable_t))

static void xmemory_monitor_win32_iface_init (GMemoryMonitorInterface *iface);
static void xmemory_monitor_win32_initable_iface_init (xinitable_iface_t *iface);

struct _GMemoryMonitorWin32
{
  xobject_t parent_instance;

  HANDLE event;
  HANDLE mem;
  HANDLE thread;
};

G_DEFINE_TYPE_WITH_CODE (GMemoryMonitorWin32, xmemory_monitor_win32, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xmemory_monitor_win32_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_MEMORY_MONITOR,
                                                xmemory_monitor_win32_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_MEMORY_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "win32",
                                                         30))

static void
xmemory_monitor_win32_init (GMemoryMonitorWin32 *win32)
{
}

static xboolean_t
watch_handler (xpointer_t user_data)
{
  GMemoryMonitorWin32 *win32 = user_data;

  xsignal_emit_by_name (win32, "low-memory-warning",
                         G_MEMORY_MONITOR_WARNING_LEVEL_LOW);

  return XSOURCE_REMOVE;
}

/* Thread which watches for win32 memory resource events */
static DWORD WINAPI
watch_thread_function (LPVOID parameter)
{
  GWeakRef *weak_ref = parameter;
  GMemoryMonitorWin32 *win32 = NULL;
  HANDLE handles[2] = { 0, };
  DWORD result;
  BOOL low_memory_state;

  win32 = g_weak_ref_get (weak_ref);
  if (!win32)
    goto end;

  if (!DuplicateHandle (GetCurrentProcess (),
                        win32->event,
                        GetCurrentProcess (),
                        &handles[0],
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS))
    {
      xchar_t *emsg;

      emsg = g_win32_error_message (GetLastError ());
      g_debug ("DuplicateHandle failed: %s", emsg);
      g_free (emsg);
      goto end;
    }

  if (!DuplicateHandle (GetCurrentProcess (),
                        win32->mem,
                        GetCurrentProcess (),
                        &handles[1],
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS))
    {
      xchar_t *emsg;

      emsg = g_win32_error_message (GetLastError ());
      g_debug ("DuplicateHandle failed: %s", emsg);
      g_free (emsg);
      goto end;
    }

  g_clear_object (&win32);

  while (1)
    {
      if (!QueryMemoryResourceNotification (handles[1], &low_memory_state))
        {
          xchar_t *emsg;

          emsg = g_win32_error_message (GetLastError ());
          g_debug ("QueryMemoryResourceNotification failed: %s", emsg);
          g_free (emsg);
          break;
        }

      win32 = g_weak_ref_get (weak_ref);
      if (!win32)
        break;

      if (low_memory_state)
        {
          g_idle_add_full (G_PRIORITY_DEFAULT,
                           watch_handler,
                           g_steal_pointer (&win32),
                           xobject_unref);
          /* throttle a bit the loop */
          g_usleep (G_USEC_PER_SEC);
          continue;
        }

      g_clear_object (&win32);

      result = WaitForMultipleObjects (G_N_ELEMENTS (handles), handles, FALSE, INFINITE);
      switch (result)
        {
          case WAIT_OBJECT_0 + 1:
            continue;

          case WAIT_FAILED:
            {
              xchar_t *emsg;

              emsg = g_win32_error_message (GetLastError ());
              g_debug ("WaitForMultipleObjects failed: %s", emsg);
              g_free (emsg);
            }
            G_GNUC_FALLTHROUGH;
          default:
            goto end;
        }
    }

end:
  if (handles[0])
    CloseHandle (handles[0]);
  if (handles[1])
    CloseHandle (handles[1]);
  g_clear_object (&win32);
  g_weak_ref_clear (weak_ref);
  g_free (weak_ref);
  return 0;
}

static xboolean_t
xmemory_monitor_win32_initable_init (xinitable_t     *initable,
                                      xcancellable_t  *cancellable,
                                      xerror_t       **error)
{
  GMemoryMonitorWin32 *win32 = G_MEMORY_MONITOR_WIN32 (initable);
  GWeakRef *weak_ref = NULL;

  win32->event = CreateEvent (NULL, FALSE, FALSE, NULL);
  if (win32->event == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (GetLastError ()),
                           "Failed to create event");
      return FALSE;
    }

  win32->mem = CreateMemoryResourceNotification (LowMemoryResourceNotification);
  if (win32->mem == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (GetLastError ()),
                           "Failed to create resource notification handle");
      return FALSE;
    }

  weak_ref = g_new0 (GWeakRef, 1);
  g_weak_ref_init (weak_ref, win32);
  /* Use CreateThread (not xthread_t) with a small stack to make it more lightweight. */
  win32->thread = CreateThread (NULL, 1024, watch_thread_function, weak_ref, 0, NULL);
  if (win32->thread == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (GetLastError ()),
                           "Failed to create memory resource notification thread");
      g_weak_ref_clear (weak_ref);
      g_free (weak_ref);
      return FALSE;
    }

  return TRUE;
}

static void
xmemory_monitor_win32_finalize (xobject_t *object)
{
  GMemoryMonitorWin32 *win32 = G_MEMORY_MONITOR_WIN32 (object);

  if (win32->thread)
    {
      SetEvent (win32->event);
      WaitForSingleObject (win32->thread, INFINITE);
      CloseHandle (win32->thread);
    }

  if (win32->event)
    CloseHandle (win32->event);

  if (win32->mem)
    CloseHandle (win32->mem);

  XOBJECT_CLASS (xmemory_monitor_win32_parent_class)->finalize (object);
}

static void
xmemory_monitor_win32_class_init (GMemoryMonitorWin32Class *nl_class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (nl_class);

  xobject_class->finalize = xmemory_monitor_win32_finalize;
}

static void
xmemory_monitor_win32_iface_init (GMemoryMonitorInterface *monitor_iface)
{
}

static void
xmemory_monitor_win32_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = xmemory_monitor_win32_initable_init;
}
