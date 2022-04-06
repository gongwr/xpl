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

#include "gfilemonitor.h"
#include "gioenumtypes.h"
#include "gmarshal-internal.h"
#include "gfile.h"
#include "gvfs.h"
#include "glibintl.h"

/**
 * SECTION:gfilemonitor
 * @short_description: File Monitor
 * @include: gio/gio.h
 *
 * Monitors a file or directory for changes.
 *
 * To obtain a #xfile_monitor_t for a file or directory, use
 * xfile_monitor(), xfile_monitor_file(), or
 * xfile_monitor_directory().
 *
 * To get informed about changes to the file or directory you are
 * monitoring, connect to the #xfile_monitor_t::changed signal. The
 * signal will be emitted in the
 * [thread-default main context][g-main-context-push-thread-default]
 * of the thread that the monitor was created in
 * (though if the global default main context is blocked, this may
 * cause notifications to be blocked even if the thread-default
 * context is still running).
 **/

#define DEFAULT_RATE_LIMIT_MSECS 800

struct _GFileMonitorPrivate
{
  xboolean_t cancelled;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xfile_monitor_t, xfile_monitor, XTYPE_OBJECT)

enum
{
  PROP_0,
  PROP_RATE_LIMIT,
  PROP_CANCELLED
};

static xuint_t xfile_monitor_changed_signal;

static void
xfile_monitor_set_property (xobject_t      *object,
                             xuint_t         prop_id,
                             const xvalue_t *value,
                             xparam_spec_t   *pspec)
{
  //xfile_monitor_t *monitor;

  //monitor = XFILE_MONITOR (object);

  switch (prop_id)
    {
    case PROP_RATE_LIMIT:
      /* not supported by default */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xfile_monitor_get_property (xobject_t    *object,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  switch (prop_id)
    {
    case PROP_RATE_LIMIT:
      /* we expect this to be overridden... */
      xvalue_set_int (value, DEFAULT_RATE_LIMIT_MSECS);
      break;

    case PROP_CANCELLED:
      //g_mutex_lock (&fms->lock);
      xvalue_set_boolean (value, FALSE);//fms->cancelled);
      //g_mutex_unlock (&fms->lock);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xfile_monitor_dispose (xobject_t *object)
{
  xfile_monitor_t *monitor = XFILE_MONITOR (object);

  /* Make sure we cancel on last unref */
  xfile_monitor_cancel (monitor);

  G_OBJECT_CLASS (xfile_monitor_parent_class)->dispose (object);
}

static void
xfile_monitor_init (xfile_monitor_t *monitor)
{
  monitor->priv = xfile_monitor_get_instance_private (monitor);
}

static void
xfile_monitor_class_init (xfile_monitor_class_t *klass)
{
  xobject_class_t *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = xfile_monitor_dispose;
  object_class->get_property = xfile_monitor_get_property;
  object_class->set_property = xfile_monitor_set_property;

  /**
   * xfile_monitor_t::changed:
   * @monitor: a #xfile_monitor_t.
   * @file: a #xfile_t.
   * @other_file: (nullable): a #xfile_t or #NULL.
   * @event_type: a #xfile_monitor_event_t.
   *
   * Emitted when @file has been changed.
   *
   * If using %XFILE_MONITOR_WATCH_MOVES on a directory monitor, and
   * the information is available (and if supported by the backend),
   * @event_type may be %XFILE_MONITOR_EVENT_RENAMED,
   * %XFILE_MONITOR_EVENT_MOVED_IN or %XFILE_MONITOR_EVENT_MOVED_OUT.
   *
   * In all cases @file will be a child of the monitored directory.  For
   * renames, @file will be the old name and @other_file is the new
   * name.  For "moved in" events, @file is the name of the file that
   * appeared and @other_file is the old name that it was moved from (in
   * another directory).  For "moved out" events, @file is the name of
   * the file that used to be in this directory and @other_file is the
   * name of the file at its new location.
   *
   * It makes sense to treat %XFILE_MONITOR_EVENT_MOVED_IN as
   * equivalent to %XFILE_MONITOR_EVENT_CREATED and
   * %XFILE_MONITOR_EVENT_MOVED_OUT as equivalent to
   * %XFILE_MONITOR_EVENT_DELETED, with extra information.
   * %XFILE_MONITOR_EVENT_RENAMED is equivalent to a delete/create
   * pair.  This is exactly how the events will be reported in the case
   * that the %XFILE_MONITOR_WATCH_MOVES flag is not in use.
   *
   * If using the deprecated flag %XFILE_MONITOR_SEND_MOVED flag and @event_type is
   * %XFILE_MONITOR_EVENT_MOVED, @file will be set to a #xfile_t containing the
   * old path, and @other_file will be set to a #xfile_t containing the new path.
   *
   * In all the other cases, @other_file will be set to #NULL.
   **/
  xfile_monitor_changed_signal = xsignal_new (I_("changed"),
                                                XTYPE_FILE_MONITOR,
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (xfile_monitor_class_t, changed),
                                                NULL, NULL,
                                                _g_cclosure_marshal_VOID__OBJECT_OBJECT_ENUM,
                                                XTYPE_NONE, 3,
                                                XTYPE_FILE, XTYPE_FILE, XTYPE_FILE_MONITOR_EVENT);
  xsignal_set_va_marshaller (xfile_monitor_changed_signal,
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_OBJECT_ENUMv);

  xobject_class_install_property (object_class, PROP_RATE_LIMIT,
                                   g_param_spec_int ("rate-limit",
                                                     P_("Rate limit"),
                                                     P_("The limit of the monitor to watch for changes, in milliseconds"),
                                                     0, G_MAXINT, DEFAULT_RATE_LIMIT_MSECS, G_PARAM_READWRITE |
                                                     G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (object_class, PROP_CANCELLED,
                                   g_param_spec_boolean ("cancelled",
                                                         P_("Cancelled"),
                                                         P_("Whether the monitor has been cancelled"),
                                                         FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

/**
 * xfile_monitor_is_cancelled:
 * @monitor: a #xfile_monitor_t
 *
 * Returns whether the monitor is canceled.
 *
 * Returns: %TRUE if monitor is canceled. %FALSE otherwise.
 **/
xboolean_t
xfile_monitor_is_cancelled (xfile_monitor_t *monitor)
{
  xboolean_t res;

  g_return_val_if_fail (X_IS_FILE_MONITOR (monitor), FALSE);

  res = monitor->priv->cancelled;

  return res;
}

/**
 * xfile_monitor_cancel:
 * @monitor: a #xfile_monitor_t.
 *
 * Cancels a file monitor.
 *
 * Returns: always %TRUE
 **/
xboolean_t
xfile_monitor_cancel (xfile_monitor_t *monitor)
{
  g_return_val_if_fail (X_IS_FILE_MONITOR (monitor), FALSE);

  if (!monitor->priv->cancelled)
    {
      XFILE_MONITOR_GET_CLASS (monitor)->cancel (monitor);

      monitor->priv->cancelled = TRUE;
      xobject_notify (G_OBJECT (monitor), "cancelled");
    }

  return TRUE;
}

/**
 * xfile_monitor_set_rate_limit:
 * @monitor: a #xfile_monitor_t.
 * @limit_msecs: a non-negative integer with the limit in milliseconds
 *     to poll for changes
 *
 * Sets the rate limit to which the @monitor will report
 * consecutive change events to the same file.
 */
void
xfile_monitor_set_rate_limit (xfile_monitor_t *monitor,
                               xint_t          limit_msecs)
{
  xobject_set (monitor, "rate-limit", limit_msecs, NULL);
}

/**
 * xfile_monitor_emit_event:
 * @monitor: a #xfile_monitor_t.
 * @child: a #xfile_t.
 * @other_file: a #xfile_t.
 * @event_type: a set of #xfile_monitor_event_t flags.
 *
 * Emits the #xfile_monitor_t::changed signal if a change
 * has taken place. Should be called from file monitor
 * implementations only.
 *
 * Implementations are responsible to call this method from the
 * [thread-default main context][g-main-context-push-thread-default] of the
 * thread that the monitor was created in.
 **/
void
xfile_monitor_emit_event (xfile_monitor_t      *monitor,
                           xfile_t             *child,
                           xfile_t             *other_file,
                           xfile_monitor_event_t  event_type)
{
  g_return_if_fail (X_IS_FILE_MONITOR (monitor));
  g_return_if_fail (X_IS_FILE (child));
  g_return_if_fail (!other_file || X_IS_FILE (other_file));

  if (monitor->priv->cancelled)
    return;

  xsignal_emit (monitor, xfile_monitor_changed_signal, 0, child, other_file, event_type);
}
