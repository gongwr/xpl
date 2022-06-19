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

#include <glib.h>
#include "gnativevolumemonitor.h"


G_DEFINE_ABSTRACT_TYPE (xnative_volume_monitor_t, g_native_volume_monitor, XTYPE_VOLUME_MONITOR)

static void
g_native_volume_monitor_finalize (xobject_t *object)
{
  XOBJECT_CLASS (g_native_volume_monitor_parent_class)->finalize (object);
}


static void
g_native_volume_monitor_class_init (GNativeVolumeMonitorClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = g_native_volume_monitor_finalize;
}


static void
g_native_volume_monitor_init (xnative_volume_monitor_t *native_monitor)
{
}
