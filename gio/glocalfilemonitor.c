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

#include "gioenumtypes.h"
#include "glocalfilemonitor.h"
#include "giomodule-priv.h"
#include "gioerror.h"
#include "glibintl.h"
#include "glocalfile.h"
#include "glib-private.h"

#include <string.h>

#define DEFAULT_RATE_LIMIT                           800 * G_TIME_SPAN_MILLISECOND
#define VIRTUAL_CHANGES_DONE_DELAY                     2 * G_TIME_SPAN_SECOND

/* GFileMonitorSource is a xsource_t responsible for emitting the changed
 * signals in the owner-context of the xfile_monitor_t.
 *
 * It contains functionality for cross-thread queuing of events.  It
 * also handles merging of CHANGED events and emission of CHANGES_DONE
 * events.
 *
 * We use the "priv" pointer in the external struct to store it.
 */
struct _GFileMonitorSource {
  xsource_t       source;

  xmutex_t        lock;
  GWeakRef      instance_ref;
  xfile_monitor_flags_t flags;
  xchar_t        *dirname;
  xchar_t        *basename;
  xchar_t        *filename;
  xsequence_t    *pending_changes; /* sorted by ready time */
  xhashtable_t   *pending_changes_table;
  xqueue_t        event_queue;
  sint64_t        rate_limit;
};

/* PendingChange is a struct to keep track of a file that needs to have
 * (at least) a CHANGES_DONE_HINT event sent for it in the near future.
 *
 * If 'dirty' is TRUE then a CHANGED event also needs to be sent.
 *
 * last_emission is the last time a CHANGED event was emitted.  It is
 * used to calculate the time to send the next event.
 */
typedef struct {
  xchar_t    *child;
  xuint64_t   last_emission : 63;
  xuint64_t   dirty         :  1;
} PendingChange;

/* QueuedEvent is a signal that will be sent immediately, as soon as the
 * source gets a chance to dispatch.  The existence of any queued event
 * implies that the source is ready now.
 */
typedef struct
{
  xfile_monitor_event_t event_type;
  xfile_t *child;
  xfile_t *other;
} QueuedEvent;

static sint64_t
pending_change_get_ready_time (const PendingChange *change,
                               GFileMonitorSource  *fms)
{
  if (change->dirty)
    return change->last_emission + fms->rate_limit;
  else
    return change->last_emission + VIRTUAL_CHANGES_DONE_DELAY;
}

static int
pending_change_compare_ready_time (xconstpointer a_p,
                                   xconstpointer b_p,
                                   xpointer_t      user_data)
{
  GFileMonitorSource *fms = user_data;
  const PendingChange *a = a_p;
  const PendingChange *b = b_p;
  sint64_t ready_time_a;
  sint64_t ready_time_b;

  ready_time_a = pending_change_get_ready_time (a, fms);
  ready_time_b = pending_change_get_ready_time (b, fms);

  if (ready_time_a < ready_time_b)
    return -1;
  else
    return ready_time_a > ready_time_b;
}

static void
pending_change_free (xpointer_t data)
{
  PendingChange *change = data;

  g_free (change->child);

  g_slice_free (PendingChange, change);
}

static void
queued_event_free (QueuedEvent *event)
{
  xobject_unref (event->child);
  if (event->other)
    xobject_unref (event->other);

  g_slice_free (QueuedEvent, event);
}

static sint64_t
xfile_monitor_source_get_ready_time (GFileMonitorSource *fms)
{
  GSequenceIter *iter;

  if (fms->event_queue.length)
    return 0;

  iter = g_sequence_get_begin_iter (fms->pending_changes);
  if (g_sequence_iter_is_end (iter))
    return -1;

  return pending_change_get_ready_time (g_sequence_get (iter), fms);
}

static void
xfile_monitor_source_update_ready_time (GFileMonitorSource *fms)
{
  xsource_set_ready_time ((xsource_t *) fms, xfile_monitor_source_get_ready_time (fms));
}

static GSequenceIter *
xfile_monitor_source_find_pending_change (GFileMonitorSource *fms,
                                           const xchar_t        *child)
{
  return xhash_table_lookup (fms->pending_changes_table, child);
}

static void
xfile_monitor_source_add_pending_change (GFileMonitorSource *fms,
                                          const xchar_t        *child,
                                          sint64_t              now)
{
  PendingChange *change;
  GSequenceIter *iter;

  change = g_slice_new (PendingChange);
  change->child = xstrdup (child);
  change->last_emission = now;
  change->dirty = FALSE;

  iter = g_sequence_insert_sorted (fms->pending_changes, change, pending_change_compare_ready_time, fms);
  xhash_table_insert (fms->pending_changes_table, change->child, iter);
}

static xboolean_t
xfile_monitor_source_set_pending_change_dirty (GFileMonitorSource *fms,
                                                GSequenceIter      *iter)
{
  PendingChange *change;

  change = g_sequence_get (iter);

  /* if it was already dirty then this change is 'uninteresting' */
  if (change->dirty)
    return FALSE;

  change->dirty = TRUE;

  g_sequence_sort_changed (iter, pending_change_compare_ready_time, fms);

  return TRUE;
}

static xboolean_t
xfile_monitor_source_get_pending_change_dirty (GFileMonitorSource *fms,
                                                GSequenceIter      *iter)
{
  PendingChange *change;

  change = g_sequence_get (iter);

  return change->dirty;
}

static void
xfile_monitor_source_remove_pending_change (GFileMonitorSource *fms,
                                             GSequenceIter      *iter,
                                             const xchar_t        *child)
{
  /* must remove the hash entry first -- its key is owned by the data
   * which will be freed when removing the sequence iter
   */
  xhash_table_remove (fms->pending_changes_table, child);
  g_sequence_remove (iter);
}

static void
xfile_monitor_source_queue_event (GFileMonitorSource *fms,
                                   xfile_monitor_event_t   event_type,
                                   const xchar_t        *child,
                                   xfile_t              *other)
{
  QueuedEvent *event;

  event = g_slice_new (QueuedEvent);
  event->event_type = event_type;
  if (child != NULL && fms->dirname != NULL)
    event->child = g_local_file_new_from_dirname_and_basename (fms->dirname, child);
  else if (child != NULL)
    {
      xchar_t *dirname = g_path_get_dirname (fms->filename);
      event->child = g_local_file_new_from_dirname_and_basename (dirname, child);
      g_free (dirname);
    }
  else if (fms->dirname)
    event->child = _g_local_file_new (fms->dirname);
  else if (fms->filename)
    event->child = _g_local_file_new (fms->filename);
  event->other = other;
  if (other)
    xobject_ref (other);

  g_queue_push_tail (&fms->event_queue, event);
}

static xboolean_t
xfile_monitor_source_file_changed (GFileMonitorSource *fms,
                                    const xchar_t        *child,
                                    sint64_t              now)
{
  GSequenceIter *pending;
  xboolean_t interesting;

  pending = xfile_monitor_source_find_pending_change (fms, child);

  /* If there is no pending change, emit one and create a record,
   * else: just mark the existing record as dirty.
   */
  if (!pending)
    {
      xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CHANGED, child, NULL);
      xfile_monitor_source_add_pending_change (fms, child, now);
      interesting = TRUE;
    }
  else
    interesting = xfile_monitor_source_set_pending_change_dirty (fms, pending);

  xfile_monitor_source_update_ready_time (fms);

  return interesting;
}

static void
xfile_monitor_source_file_changes_done (GFileMonitorSource *fms,
                                         const xchar_t        *child)
{
  GSequenceIter *pending;

  pending = xfile_monitor_source_find_pending_change (fms, child);
  if (pending)
    {
      /* If it is dirty, make sure we push out the last CHANGED event */
      if (xfile_monitor_source_get_pending_change_dirty (fms, pending))
        xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CHANGED, child, NULL);

      xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, child, NULL);
      xfile_monitor_source_remove_pending_change (fms, pending, child);
    }
}

static void
xfile_monitor_source_file_created (GFileMonitorSource *fms,
                                    const xchar_t        *child,
                                    sint64_t              event_time)
{
  /* Unlikely, but if we have pending changes for this filename, make
   * sure we flush those out first, before creating the new ones.
   */
  xfile_monitor_source_file_changes_done (fms, child);

  /* Emit CREATE and add a pending changes record */
  xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CREATED, child, NULL);
  xfile_monitor_source_add_pending_change (fms, child, event_time);
}

static void
xfile_monitor_source_send_event (GFileMonitorSource *fms,
                                  xfile_monitor_event_t   event_type,
                                  const xchar_t        *child,
                                  xfile_t              *other)
{
  /* always flush any pending changes before we queue a new event */
  xfile_monitor_source_file_changes_done (fms, child);
  xfile_monitor_source_queue_event (fms, event_type, child, other);
}

static void
xfile_monitor_source_send_synthetic_created (GFileMonitorSource *fms,
                                              const xchar_t        *child)
{
  xfile_monitor_source_file_changes_done (fms, child);
  xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CREATED, child, NULL);
  xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, child, NULL);
}

#ifndef G_DISABLE_ASSERT
static xboolean_t
is_basename (const xchar_t *name)
{
  if (name[0] == '.' && ((name[1] == '.' && name[2] == '\0') || name[1] == '\0'))
    return FALSE;

  return !strchr (name, '/');
}
#endif  /* !G_DISABLE_ASSERT */

xboolean_t
xfile_monitor_source_handle_event (GFileMonitorSource *fms,
                                    xfile_monitor_event_t   event_type,
                                    const xchar_t        *child,
                                    const xchar_t        *rename_to,
                                    xfile_t              *other,
                                    sint64_t              event_time)
{
  xboolean_t interesting = TRUE;
  xfile_monitor_t *instance = NULL;

  xassert (!child || is_basename (child));
  xassert (!rename_to || is_basename (rename_to));

  if (fms->basename && (!child || !xstr_equal (child, fms->basename))
                    && (!rename_to || !xstr_equal (rename_to, fms->basename)))
    return TRUE;

  g_mutex_lock (&fms->lock);

  /* monitor is already gone -- don't bother */
  instance = g_weak_ref_get (&fms->instance_ref);
  if (instance == NULL)
    {
      g_mutex_unlock (&fms->lock);
      return TRUE;
    }

  switch (event_type)
    {
    case XFILE_MONITOR_EVENT_CREATED:
      xassert (!other && !rename_to);
      xfile_monitor_source_file_created (fms, child, event_time);
      break;

    case XFILE_MONITOR_EVENT_CHANGED:
      xassert (!other && !rename_to);
      interesting = xfile_monitor_source_file_changed (fms, child, event_time);
      break;

    case XFILE_MONITOR_EVENT_CHANGES_DONE_HINT:
      xassert (!other && !rename_to);
      xfile_monitor_source_file_changes_done (fms, child);
      break;

    case XFILE_MONITOR_EVENT_MOVED_IN:
      xassert (!rename_to);
      if (fms->flags & XFILE_MONITOR_WATCH_MOVES)
        xfile_monitor_source_send_event (fms, XFILE_MONITOR_EVENT_MOVED_IN, child, other);
      else
        xfile_monitor_source_send_synthetic_created (fms, child);
      break;

    case XFILE_MONITOR_EVENT_MOVED_OUT:
      xassert (!rename_to);
      if (fms->flags & XFILE_MONITOR_WATCH_MOVES)
        xfile_monitor_source_send_event (fms, XFILE_MONITOR_EVENT_MOVED_OUT, child, other);
      else if (other && (fms->flags & XFILE_MONITOR_SEND_MOVED))
        xfile_monitor_source_send_event (fms, XFILE_MONITOR_EVENT_MOVED, child, other);
      else
        xfile_monitor_source_send_event (fms, XFILE_MONITOR_EVENT_DELETED, child, NULL);
      break;

    case XFILE_MONITOR_EVENT_RENAMED:
      xassert (!other && rename_to);
      if (fms->flags & (XFILE_MONITOR_WATCH_MOVES | XFILE_MONITOR_SEND_MOVED))
        {
          xfile_t *other;
          const xchar_t *dirname;
          xchar_t *allocated_dirname = NULL;
          xfile_monitor_event_t event;

          event = (fms->flags & XFILE_MONITOR_WATCH_MOVES) ? XFILE_MONITOR_EVENT_RENAMED : XFILE_MONITOR_EVENT_MOVED;

          if (fms->dirname != NULL)
            dirname = fms->dirname;
          else
            {
              allocated_dirname = g_path_get_dirname (fms->filename);
              dirname = allocated_dirname;
            }

          other = g_local_file_new_from_dirname_and_basename (dirname, rename_to);
          xfile_monitor_source_file_changes_done (fms, rename_to);
          xfile_monitor_source_send_event (fms, event, child, other);

          xobject_unref (other);
          g_free (allocated_dirname);
        }
      else
        {
          xfile_monitor_source_send_event (fms, XFILE_MONITOR_EVENT_DELETED, child, NULL);
          xfile_monitor_source_send_synthetic_created (fms, rename_to);
        }
      break;

    case XFILE_MONITOR_EVENT_DELETED:
    case XFILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case XFILE_MONITOR_EVENT_PRE_UNMOUNT:
    case XFILE_MONITOR_EVENT_UNMOUNTED:
      xassert (!other && !rename_to);
      xfile_monitor_source_send_event (fms, event_type, child, NULL);
      break;

    case XFILE_MONITOR_EVENT_MOVED:
      /* was never available in this API */
    default:
      g_assert_not_reached ();
    }

  xfile_monitor_source_update_ready_time (fms);

  g_mutex_unlock (&fms->lock);
  g_clear_object (&instance);

  return interesting;
}

static sint64_t
xfile_monitor_source_get_rate_limit (GFileMonitorSource *fms)
{
  sint64_t rate_limit;

  g_mutex_lock (&fms->lock);
  rate_limit = fms->rate_limit;
  g_mutex_unlock (&fms->lock);

  return rate_limit;
}

static xboolean_t
xfile_monitor_source_set_rate_limit (GFileMonitorSource *fms,
                                      sint64_t              rate_limit)
{
  xboolean_t changed;

  g_mutex_lock (&fms->lock);

  if (rate_limit != fms->rate_limit)
    {
      fms->rate_limit = rate_limit;

      g_sequence_sort (fms->pending_changes, pending_change_compare_ready_time, fms);
      xfile_monitor_source_update_ready_time (fms);

      changed = TRUE;
    }
  else
    changed = FALSE;

  g_mutex_unlock (&fms->lock);

  return changed;
}

static xboolean_t
xfile_monitor_source_dispatch (xsource_t     *source,
                                xsource_func_t  callback,
                                xpointer_t     user_data)
{
  GFileMonitorSource *fms = (GFileMonitorSource *) source;
  QueuedEvent *event;
  xqueue_t event_queue;
  sint64_t now;
  xfile_monitor_t *instance = NULL;

  /* make sure the monitor still exists */
  instance = g_weak_ref_get (&fms->instance_ref);
  if (instance == NULL)
    return FALSE;

  now = xsource_get_time (source);

  /* Acquire the lock once and grab all events in one go, handling the
   * queued events first.  This avoids strange possibilities in cases of
   * long delays, such as CHANGED events coming before CREATED events.
   *
   * We do this by converting the applicable pending changes into queued
   * events (after the ones already queued) and then stealing the entire
   * event queue in one go.
   */
  g_mutex_lock (&fms->lock);

  /* Create events for any pending changes that are due to fire */
  while (!g_sequence_is_empty (fms->pending_changes))
    {
      GSequenceIter *iter = g_sequence_get_begin_iter (fms->pending_changes);
      PendingChange *pending = g_sequence_get (iter);

      /* We've gotten to a pending change that's not ready.  Stop. */
      if (pending_change_get_ready_time (pending, fms) > now)
        break;

      if (pending->dirty)
        {
          /* It's time to send another CHANGED and update the record */
          xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CHANGED, pending->child, NULL);
          pending->last_emission = now;
          pending->dirty = FALSE;

          g_sequence_sort_changed (iter, pending_change_compare_ready_time, fms);
        }
      else
        {
          /* It's time to send CHANGES_DONE and remove the pending record */
          xfile_monitor_source_queue_event (fms, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, pending->child, NULL);
          xfile_monitor_source_remove_pending_change (fms, iter, pending->child);
        }
    }

  /* Steal the queue */
  memcpy (&event_queue, &fms->event_queue, sizeof event_queue);
  memset (&fms->event_queue, 0, sizeof fms->event_queue);

  xfile_monitor_source_update_ready_time (fms);

  g_mutex_unlock (&fms->lock);
  g_clear_object (&instance);

  /* We now have our list of events to deliver */
  while ((event = g_queue_pop_head (&event_queue)))
    {
      /* an event handler could destroy 'instance', so check each time */
      instance = g_weak_ref_get (&fms->instance_ref);
      if (instance != NULL)
        xfile_monitor_emit_event (instance, event->child, event->other, event->event_type);

      g_clear_object (&instance);
      queued_event_free (event);
    }

  return TRUE;
}

static void
xfile_monitor_source_dispose (GFileMonitorSource *fms)
{
  xhash_table_iter_t iter;
  xpointer_t seqiter;
  QueuedEvent *event;

  g_mutex_lock (&fms->lock);

  xhash_table_iter_init (&iter, fms->pending_changes_table);
  while (xhash_table_iter_next (&iter, NULL, &seqiter))
    {
      xhash_table_iter_remove (&iter);
      g_sequence_remove (seqiter);
    }

  while ((event = g_queue_pop_head (&fms->event_queue)))
    queued_event_free (event);

  xassert (g_sequence_is_empty (fms->pending_changes));
  xassert (xhash_table_size (fms->pending_changes_table) == 0);
  xassert (fms->event_queue.length == 0);
  g_weak_ref_set (&fms->instance_ref, NULL);

  xfile_monitor_source_update_ready_time (fms);

  g_mutex_unlock (&fms->lock);

  xsource_destroy ((xsource_t *) fms);
}

static void
xfile_monitor_source_finalize (xsource_t *source)
{
  GFileMonitorSource *fms = (GFileMonitorSource *) source;

  /* should already have been cleared in dispose of the monitor */
  xassert (g_weak_ref_get (&fms->instance_ref) == NULL);
  g_weak_ref_clear (&fms->instance_ref);

  xassert (g_sequence_is_empty (fms->pending_changes));
  xassert (xhash_table_size (fms->pending_changes_table) == 0);
  xassert (fms->event_queue.length == 0);

  xhash_table_unref (fms->pending_changes_table);
  g_sequence_free (fms->pending_changes);

  g_free (fms->dirname);
  g_free (fms->basename);
  g_free (fms->filename);

  g_mutex_clear (&fms->lock);
}

static xuint_t
str_hash0 (xconstpointer str)
{
  return str ? xstr_hash (str) : 0;
}

static xboolean_t
str_equal0 (xconstpointer a,
            xconstpointer b)
{
  return xstrcmp0 (a, b) == 0;
}

static GFileMonitorSource *
xfile_monitor_source_new (xpointer_t           instance,
                           const xchar_t       *filename,
                           xboolean_t           is_directory,
                           xfile_monitor_flags_t  flags)
{
  static xsource_funcs_t source_funcs = {
    NULL, NULL,
    xfile_monitor_source_dispatch,
    xfile_monitor_source_finalize,
    NULL, NULL
  };
  GFileMonitorSource *fms;
  xsource_t *source;

  source = xsource_new (&source_funcs, sizeof (GFileMonitorSource));
  fms = (GFileMonitorSource *) source;

  xsource_set_static_name (source, "GFileMonitorSource");

  g_mutex_init (&fms->lock);
  g_weak_ref_init (&fms->instance_ref, instance);
  fms->pending_changes = g_sequence_new (pending_change_free);
  fms->pending_changes_table = xhash_table_new (str_hash0, str_equal0);
  fms->rate_limit = DEFAULT_RATE_LIMIT;
  fms->flags = flags;

  if (is_directory)
    {
      fms->dirname = xstrdup (filename);
      fms->basename = NULL;
      fms->filename = NULL;
    }
  else if (flags & XFILE_MONITOR_WATCH_HARD_LINKS)
    {
      fms->dirname = NULL;
      fms->basename = NULL;
      fms->filename = xstrdup (filename);
    }
  else
    {
      fms->dirname = g_path_get_dirname (filename);
      fms->basename = g_path_get_basename (filename);
      fms->filename = NULL;
    }

  return fms;
}

G_DEFINE_ABSTRACT_TYPE (xlocal_file_monitor_t, g_local_file_monitor, XTYPE_FILE_MONITOR)

enum {
  PROP_0,
  PROP_RATE_LIMIT,
};

static void
g_local_file_monitor_get_property (xobject_t *object, xuint_t prop_id,
                                   xvalue_t *value, xparam_spec_t *pspec)
{
  xlocal_file_monitor_t *monitor = G_LOCAL_FILE_MONITOR (object);
  sint64_t rate_limit;

  xassert (prop_id == PROP_RATE_LIMIT);

  rate_limit = xfile_monitor_source_get_rate_limit (monitor->source);
  rate_limit /= G_TIME_SPAN_MILLISECOND;

  xvalue_set_int (value, rate_limit);
}

static void
g_local_file_monitor_set_property (xobject_t *object, xuint_t prop_id,
                                   const xvalue_t *value, xparam_spec_t *pspec)
{
  xlocal_file_monitor_t *monitor = G_LOCAL_FILE_MONITOR (object);
  sint64_t rate_limit;

  xassert (prop_id == PROP_RATE_LIMIT);

  rate_limit = xvalue_get_int (value);
  rate_limit *= G_TIME_SPAN_MILLISECOND;

  if (xfile_monitor_source_set_rate_limit (monitor->source, rate_limit))
    xobject_notify (object, "rate-limit");
}

#ifndef G_OS_WIN32
static void
g_local_file_monitor_mounts_changed (xunix_mount_monitor_t *mount_monitor,
                                     xpointer_t           user_data)
{
  xlocal_file_monitor_t *local_monitor = user_data;
  GUnixMountEntry *mount;
  xboolean_t is_mounted;
  xfile_t *file;

  /* Emulate unmount detection */
  mount = g_unix_mount_at (local_monitor->source->dirname, NULL);

  is_mounted = mount != NULL;

  if (mount)
    g_unix_mount_free (mount);

  if (local_monitor->was_mounted != is_mounted)
    {
      if (local_monitor->was_mounted && !is_mounted)
        {
          file = xfile_new_for_path (local_monitor->source->dirname);
          xfile_monitor_emit_event (XFILE_MONITOR (local_monitor), file, NULL, XFILE_MONITOR_EVENT_UNMOUNTED);
          xobject_unref (file);
        }
      local_monitor->was_mounted = is_mounted;
    }
}
#endif

static void
g_local_file_monitor_start (xlocal_file_monitor_t *local_monitor,
                            const xchar_t       *filename,
                            xboolean_t           is_directory,
                            xfile_monitor_flags_t  flags,
                            xmain_context_t      *context)
{
  xlocal_file_monitor_class_t *class = G_LOCAL_FILE_MONITOR_GET_CLASS (local_monitor);
  GFileMonitorSource *source;

  g_return_if_fail (X_IS_LOCAL_FILE_MONITOR (local_monitor));

  xassert (!local_monitor->source);

  source = xfile_monitor_source_new (local_monitor, filename, is_directory, flags);
  local_monitor->source = source; /* owns the ref */

  if (is_directory && !class->mount_notify && (flags & XFILE_MONITOR_WATCH_MOUNTS))
    {
#ifdef G_OS_WIN32
      /*claim everything was mounted */
      local_monitor->was_mounted = TRUE;
#else
      GUnixMountEntry *mount;

      /* Emulate unmount detection */

      mount = g_unix_mount_at (local_monitor->source->dirname, NULL);

      local_monitor->was_mounted = mount != NULL;

      if (mount)
        g_unix_mount_free (mount);

      local_monitor->mount_monitor = xunix_mount_monitor_get ();
      xsignal_connect_object (local_monitor->mount_monitor, "mounts-changed",
                               G_CALLBACK (g_local_file_monitor_mounts_changed), local_monitor, 0);
#endif
    }

  xsource_attach ((xsource_t *) source, context);

  G_LOCAL_FILE_MONITOR_GET_CLASS (local_monitor)->start (local_monitor,
                                                         source->dirname, source->basename, source->filename,
                                                         source);
}

static void
g_local_file_monitor_dispose (xobject_t *object)
{
  xlocal_file_monitor_t *local_monitor = G_LOCAL_FILE_MONITOR (object);

  xfile_monitor_source_dispose (local_monitor->source);

  XOBJECT_CLASS (g_local_file_monitor_parent_class)->dispose (object);
}

static void
g_local_file_monitor_finalize (xobject_t *object)
{
  xlocal_file_monitor_t *local_monitor = G_LOCAL_FILE_MONITOR (object);

  xsource_unref ((xsource_t *) local_monitor->source);

  XOBJECT_CLASS (g_local_file_monitor_parent_class)->finalize (object);
}

static void
g_local_file_monitor_init (xlocal_file_monitor_t* local_monitor)
{
}

static void g_local_file_monitor_class_init (xlocal_file_monitor_class_t *class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (class);

  xobject_class->get_property = g_local_file_monitor_get_property;
  xobject_class->set_property = g_local_file_monitor_set_property;
  xobject_class->dispose = g_local_file_monitor_dispose;
  xobject_class->finalize = g_local_file_monitor_finalize;

  xobject_class_override_property (xobject_class, PROP_RATE_LIMIT, "rate-limit");
}

static xlocal_file_monitor_t *
g_local_file_monitor_new (xboolean_t   is_remote_fs,
                          xboolean_t   is_directory,
                          xerror_t   **error)
{
  xtype_t type = XTYPE_INVALID;

  if (is_remote_fs)
    type = _xio_module_get_default_type (G_NFS_FILE_MONITOR_EXTENSION_POINT_NAME,
                                          "GIO_USE_FILE_MONITOR",
                                          G_STRUCT_OFFSET (xlocal_file_monitor_class_t, is_supported));

  /* Fallback rather to poll file monitor for remote files, see gfile.c. */
  if (type == XTYPE_INVALID && (!is_remote_fs || is_directory))
    type = _xio_module_get_default_type (G_LOCAL_FILE_MONITOR_EXTENSION_POINT_NAME,
                                          "GIO_USE_FILE_MONITOR",
                                          G_STRUCT_OFFSET (xlocal_file_monitor_class_t, is_supported));

  if (type == XTYPE_INVALID)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           _("Unable to find default local file monitor type"));
      return NULL;
    }

  return xobject_new (type, NULL);
}

xfile_monitor_t *
g_local_file_monitor_new_for_path (const xchar_t        *pathname,
                                   xboolean_t            is_directory,
                                   xfile_monitor_flags_t   flags,
                                   xerror_t            **error)
{
  xlocal_file_monitor_t *monitor;
  xboolean_t is_remote_fs;

  is_remote_fs = g_local_file_is_nfs_home (pathname);

  monitor = g_local_file_monitor_new (is_remote_fs, is_directory, error);

  if (monitor)
    g_local_file_monitor_start (monitor, pathname, is_directory, flags, xmain_context_get_thread_default ());

  return XFILE_MONITOR (monitor);
}

xfile_monitor_t *
g_local_file_monitor_new_in_worker (const xchar_t           *pathname,
                                    xboolean_t               is_directory,
                                    xfile_monitor_flags_t      flags,
                                    GFileMonitorCallback   callback,
                                    xpointer_t               user_data,
                                    xclosure_notify_t         destroy_user_data,
                                    xerror_t               **error)
{
  xlocal_file_monitor_t *monitor;
  xboolean_t is_remote_fs;

  is_remote_fs = g_local_file_is_nfs_home (pathname);

  monitor = g_local_file_monitor_new (is_remote_fs, is_directory, error);

  if (monitor)
    {
      if (callback)
        xsignal_connect_data (monitor, "changed", G_CALLBACK (callback),
                               user_data, destroy_user_data, 0  /* flags */);

      g_local_file_monitor_start (monitor, pathname, is_directory, flags, XPL_PRIVATE_CALL(g_get_worker_context) ());
    }

  return XFILE_MONITOR (monitor);
}
