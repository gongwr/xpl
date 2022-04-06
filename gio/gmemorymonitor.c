/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2019 Red Hat, Inc
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
#include "glib.h"
#include "glibintl.h"

#include "gmemorymonitor.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "ginitable.h"
#include "gioenumtypes.h"
#include "giomodule-priv.h"
#include "gtask.h"

/**
 * SECTION:gmemorymonitor
 * @title: xmemory_monitor_t
 * @short_description: Memory usage monitor
 * @include: gio/gio.h
 *
 * #xmemory_monitor_t will monitor system memory and suggest to the application
 * when to free memory so as to leave more room for other applications.
 * It is implemented on Linux using the [Low Memory Monitor](https://gitlab.freedesktop.org/hadess/low-memory-monitor/)
 * ([API documentation](https://hadess.pages.freedesktop.org/low-memory-monitor/)).
 *
 * There is also an implementation for use inside Flatpak sandboxes.
 *
 * Possible actions to take when the signal is received are:
 *
 *  - Free caches
 *  - Save files that haven't been looked at in a while to disk, ready to be reopened when needed
 *  - Run a garbage collection cycle
 *  - Try and compress fragmented allocations
 *  - Exit on idle if the process has no reason to stay around
 *  - Call [`malloc_trim(3)`](man:malloc_trim) to return cached heap pages to
 *    the kernel (if supported by your libc)
 *
 * Note that some actions may not always improve system performance, and so
 * should be profiled for your application. `malloc_trim()`, for example, may
 * make future heap allocations slower (due to releasing cached heap pages back
 * to the kernel).
 *
 * See #GMemoryMonitorWarningLevel for details on the various warning levels.
 *
 * |[<!-- language="C" -->
 * static void
 * warning_cb (xmemory_monitor_t *m, GMemoryMonitorWarningLevel level)
 * {
 *   g_debug ("Warning level: %d", level);
 *   if (warning_level > G_MEMORY_MONITOR_WARNING_LEVEL_LOW)
 *     drop_caches ();
 * }
 *
 * static xmemory_monitor_t *
 * monitor_low_memory (void)
 * {
 *   xmemory_monitor_t *m;
 *   m = xmemory_monitor_dup_default ();
 *   g_signal_connect (G_OBJECT (m), "low-memory-warning",
 *                     G_CALLBACK (warning_cb), NULL);
 *   return m;
 * }
 * ]|
 *
 * Don't forget to disconnect the #xmemory_monitor_t::low-memory-warning
 * signal, and unref the #xmemory_monitor_t itself when exiting.
 *
 * Since: 2.64
 */

/**
 * xmemory_monitor_t:
 *
 * #xmemory_monitor_t monitors system memory and indicates when
 * the system is low on memory.
 *
 * Since: 2.64
 */

/**
 * GMemoryMonitorInterface:
 * @x_iface: The parent interface.
 * @low_memory_warning: the virtual function pointer for the
 *  #xmemory_monitor_t::low-memory-warning signal.
 *
 * The virtual function table for #xmemory_monitor_t.
 *
 * Since: 2.64
 */

G_DEFINE_INTERFACE_WITH_CODE (xmemory_monitor_t, xmemory_monitor, XTYPE_OBJECT,
                              xtype_interface_add_prerequisite (g_define_type_id, XTYPE_INITABLE))

enum {
  LOW_MEMORY_WARNING,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

/**
 * xmemory_monitor_dup_default:
 *
 * Gets a reference to the default #xmemory_monitor_t for the system.
 *
 * Returns: (not nullable) (transfer full): a new reference to the default #xmemory_monitor_t
 *
 * Since: 2.64
 */
xmemory_monitor_t *
xmemory_monitor_dup_default (void)
{
  return xobject_ref (_xio_module_get_default (G_MEMORY_MONITOR_EXTENSION_POINT_NAME,
                                                 "GIO_USE_MEMORY_MONITOR",
                                                 NULL));
}

static void
xmemory_monitor_default_init (GMemoryMonitorInterface *iface)
{
  /**
   * xmemory_monitor_t::low-memory-warning:
   * @monitor: a #xmemory_monitor_t
   * @level: the #GMemoryMonitorWarningLevel warning level
   *
   * Emitted when the system is running low on free memory. The signal
   * handler should then take the appropriate action depending on the
   * warning level. See the #GMemoryMonitorWarningLevel documentation for
   * details.
   *
   * Since: 2.64
   */
  signals[LOW_MEMORY_WARNING] =
    g_signal_new (I_("low-memory-warning"),
                  XTYPE_MEMORY_MONITOR,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GMemoryMonitorInterface, low_memory_warning),
                  NULL, NULL,
                  NULL,
                  XTYPE_NONE, 1,
                  XTYPE_MEMORY_MONITOR_WARNING_LEVEL);
}
