/*******************************************************************************
  Copyright (c) 2011, 2012 Dmitry Matveev <me@dmitrymatveev.co.uk>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*******************************************************************************/

#include <glib.h>
#include "glib-private.h"

#include "kqueue-helper.h"


#define SCAN_MISSING_TIME 4 /* 1/4 Hz */

static xboolean_t km_debug_enabled = FALSE;
#define KM_W if (km_debug_enabled) g_warning

static xslist_t *missing_subs_list = NULL;
G_LOCK_DEFINE_STATIC (missing_lock);

static xboolean_t scan_missing_running = FALSE;  /* must be accessed under @missing_lock */


static xboolean_t
_km_scan_missing_cb (xpointer_t user_data)
{
  return _km_scan_missing (NULL);
}

/**
 * _km_add_missing:
 * @sub: a #kqueue_sub
 *
 * Adds a subscription to the missing files list.
 **/
void
_km_add_missing (kqueue_sub *sub)
{
  G_LOCK (missing_lock);
  if (xslist_find (missing_subs_list, sub))
    {
      KM_W ("asked to add %s to missing list but it's already on the list!\n", sub->filename);
      G_UNLOCK (missing_lock);
      return;
    }

  KM_W ("adding %s to missing list\n", sub->filename);
  missing_subs_list = xslist_prepend (missing_subs_list, sub);

  if (!scan_missing_running)
    {
      xsource_t *source;
      scan_missing_running = TRUE;
      source = g_timeout_source_new_seconds (SCAN_MISSING_TIME);
      xsource_set_callback (source, _km_scan_missing_cb, NULL, NULL);
      xsource_attach (source, XPL_PRIVATE_CALL (g_get_worker_context) ());
      xsource_unref (source);
    }

  G_UNLOCK (missing_lock);
}

/**
 * _kh_file_appeared_cb:
 * @sub: a #kqueue_sub
 *
 * A callback function for kqueue-missing subsystem.
 *
 * Signals that a missing file has finally appeared in the filesystem.
 * Emits %XFILE_MONITOR_EVENT_CREATED.
 **/
static void
_kh_file_appeared_cb (kqueue_sub *sub)
{
  gint64 now = g_get_monotonic_time ();

  g_assert (sub != NULL);
  g_assert (sub->filename);

  if (!xfile_test (sub->filename, XFILE_TEST_EXISTS))
    return;

  xfile_monitor_source_handle_event (sub->source, XFILE_MONITOR_EVENT_CREATED,
                                      sub->basename, NULL, NULL, now);
  xfile_monitor_source_handle_event (sub->source, XFILE_MONITOR_EVENT_CHANGES_DONE_HINT,
                                      sub->basename, NULL, NULL, now);
}

/**
 * _km_scan_missing:
 * @user_data: unused
 *
 * The core missing files watching routine.
 *
 * Traverses through a list of missing files, tries to start watching each with
 * kqueue, removes the appropriate entry and invokes a user callback if the file
 * has appeared.
 *
 * Returns: %FALSE if no missing files left, %TRUE otherwise.
 **/
xboolean_t
_km_scan_missing (kqueue_sub *check_this_sub_only)
{
  xslist_t *head;
  xslist_t *not_missing = NULL;
  xboolean_t retval = FALSE;

  G_LOCK (missing_lock);

  if (missing_subs_list)
    KM_W ("we have a job");

  for (head = missing_subs_list; head; head = head->next)
    {
      kqueue_sub *sub = (kqueue_sub *) head->data;
      g_assert (sub != NULL);
      g_assert (sub->filename != NULL);

      if (check_this_sub_only != NULL && sub != check_this_sub_only)
        continue;

      if (_kqsub_start_watching (sub))
        {
          KM_W ("file %s now exists, starting watching", sub->filename);
          if (check_this_sub_only == NULL)
            _kh_file_appeared_cb (sub);
          not_missing = xslist_prepend (not_missing, head);
        }
    }

  for (head = not_missing; head; head = head->next)
    {
      xslist_t *link = (xslist_t *) head->data;
      missing_subs_list = xslist_remove_link (missing_subs_list, link);
    }
  xslist_free (not_missing);

  if (missing_subs_list == NULL)
    {
      scan_missing_running = FALSE;
      retval = FALSE;
    }
  else
    retval = TRUE;

  G_UNLOCK (missing_lock);
  return retval;
}


/**
 * _km_remove:
 * @sub: a #kqueue_sub
 *
 * Removes a subscription from a list of missing files.
 **/
void
_km_remove (kqueue_sub *sub)
{
  G_LOCK (missing_lock);
  missing_subs_list = xslist_remove (missing_subs_list, sub);
  G_UNLOCK (missing_lock);
}
