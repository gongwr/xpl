#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <gio/gio.h>

/* These tests were written for the inotify implementation.
 * Other implementations may require slight adjustments in
 * the tests, e.g. the length of timeouts
 */

typedef struct
{
  xfile_t *tmp_dir;
} Fixture;

static void
setup (Fixture       *fixture,
       xconstpointer  user_data)
{
  xchar_t *path = NULL;
  xerror_t *local_error = NULL;

  path = g_dir_make_tmp ("gio-test-testfilemonitor_XXXXXX", &local_error);
  g_assert_no_error (local_error);

  fixture->tmp_dir = xfile_new_for_path (path);

  g_test_message ("Using temporary directory: %s", path);

  g_free (path);
}

static void
teardown (Fixture       *fixture,
          xconstpointer  user_data)
{
  xerror_t *local_error = NULL;

  xfile_delete (fixture->tmp_dir, NULL, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&fixture->tmp_dir);
}

typedef enum {
  NONE      = 0,
  INOTIFY   = (1 << 1),
  KQUEUE    = (1 << 2)
} Environment;

typedef struct
{
  xint_t event_type;
  xchar_t *file;
  xchar_t *other_file;
  xint_t step;

  /* Since different file monitor implementation has different capabilities,
   * we cannot expect all implementations to report all kind of events without
   * any loss. This 'optional' field is a bit mask used to mark events which
   * may be lost under specific platforms.
   */
  Environment optional;
} RecordedEvent;

static void
free_recorded_event (RecordedEvent *event)
{
  g_free (event->file);
  g_free (event->other_file);
  g_free (event);
}

typedef struct
{
  xfile_t *file;
  xfile_monitor_t *monitor;
  xmain_loop_t *loop;
  xint_t step;
  xlist_t *events;
  xfile_output_stream_t *output_stream;
} TestData;

static void
output_event (const RecordedEvent *event)
{
  if (event->step >= 0)
    g_test_message (">>>> step %d", event->step);
  else
    {
      xtype_class_t *class;

      class = xtype_class_ref (xtype_from_name ("xfile_monitor_event_t"));
      g_test_message ("%s file=%s other_file=%s\n",
                      xenum_get_value (XENUM_CLASS (class), event->event_type)->value_nick,
                      event->file,
                      event->other_file);
      xtype_class_unref (class);
    }
}

/* a placeholder for temp file names we don't want to compare */
static const xchar_t DONT_CARE[] = "";

static Environment
get_environment (xfile_monitor_t *monitor)
{
  if (xstr_equal (G_OBJECT_TYPE_NAME (monitor), "GInotifyFileMonitor"))
    return INOTIFY;
  if (xstr_equal (G_OBJECT_TYPE_NAME (monitor), "GKqueueFileMonitor"))
    return KQUEUE;
  return NONE;
}

static void
check_expected_events (RecordedEvent *expected,
                       xsize_t          n_expected,
                       xlist_t         *recorded,
                       Environment    env)
{
  xsize_t i;
  xint_t li;
  xlist_t *l;

  for (i = 0, li = 0, l = recorded; i < n_expected && l != NULL;)
    {
      RecordedEvent *e1 = &expected[i];
      RecordedEvent *e2 = l->data;
      xboolean_t mismatch = TRUE;
      xboolean_t l_extra_step = FALSE;

      do
        {
          xboolean_t ignore_other_file = FALSE;

          if (e1->step != e2->step)
            break;

          /* Kqueue isn't good at detecting file renaming, so
           * XFILE_MONITOR_WATCH_MOVES is mostly useless there.  */
          if (e1->event_type != e2->event_type && env & KQUEUE)
            {
              /* It is possible for kqueue file monitor to emit 'RENAMED' event,
               * but most of the time it is reported as a 'DELETED' event and
               * a 'CREATED' event. */
              if (e1->event_type == XFILE_MONITOR_EVENT_RENAMED)
                {
                  RecordedEvent *e2_next;

                  if (l->next == NULL)
                    break;
                  e2_next = l->next->data;

                  if (e2->event_type != XFILE_MONITOR_EVENT_DELETED)
                    break;
                  if (e2_next->event_type != XFILE_MONITOR_EVENT_CREATED)
                    break;

                  if (e1->step != e2_next->step)
                    break;

                  if (e1->file != DONT_CARE &&
                      (xstrcmp0 (e1->file, e2->file) != 0 ||
                       e2->other_file != NULL))
                    break;

                  if (e1->other_file != DONT_CARE &&
                      (xstrcmp0 (e1->other_file, e2_next->file) != 0 ||
                       e2_next->other_file != NULL))
                    break;

                  l_extra_step = TRUE;
                  mismatch = FALSE;
                  break;
                }
              /* Kqueue won't report 'MOVED_IN' and 'MOVED_OUT' events. We set
               * 'ignore_other_file' here to let the following code know that
               * 'other_file' may not match. */
              else if (e1->event_type == XFILE_MONITOR_EVENT_MOVED_IN)
                {
                  if (e2->event_type != XFILE_MONITOR_EVENT_CREATED)
                    break;
                  ignore_other_file = TRUE;
                }
              else if (e1->event_type == XFILE_MONITOR_EVENT_MOVED_OUT)
                {
                  if (e2->event_type != XFILE_MONITOR_EVENT_DELETED)
                    break;
                  ignore_other_file = TRUE;
                }
              else
                break;
            }

          if (e1->file != DONT_CARE &&
              xstrcmp0 (e1->file, e2->file) != 0)
            break;

          if (e1->other_file != DONT_CARE && !ignore_other_file &&
              xstrcmp0 (e1->other_file, e2->other_file) != 0)
            break;

          mismatch = FALSE;
        }
      while (0);

      if (mismatch)
        {
          /* Sometimes the emission of 'CHANGES_DONE_HINT' may be late because
           * it depends on the ability of file monitor implementation to report
           * 'CHANGES_DONE_HINT' itself. If the file monitor implementation
           * doesn't report 'CHANGES_DONE_HINT' itself, it may be emitted by
           * xlocal_file_monitor_t after a few seconds, which causes the event to
           * mix with results from different steps. Since 'CHANGES_DONE_HINT'
           * is just a hint, we don't require it to be reliable and we simply
           * ignore unexpected 'CHANGES_DONE_HINT' events here. */
          if (e1->event_type != XFILE_MONITOR_EVENT_CHANGES_DONE_HINT &&
              e2->event_type == XFILE_MONITOR_EVENT_CHANGES_DONE_HINT)
            {
              g_test_message ("Event CHANGES_DONE_HINT ignored at "
                              "expected index %"  G_GSIZE_FORMAT ", recorded index %d", i, li);
              li++, l = l->next;
              continue;
            }
          /* If an event is marked as optional in the current environment and
           * the event doesn't match, it means the expected event has lost. */
          else if (env & e1->optional)
            {
              g_test_message ("Event %d at expected index %" G_GSIZE_FORMAT " skipped because "
                              "it is marked as optional", e1->event_type, i);
              i++;
              continue;
            }
          /* Run above checks under g_assert_* again to provide more useful
           * error messages. Print the expected and actual events first. */
          else
            {
              xlist_t *l;
              xsize_t j;

              g_test_message ("Recorded events:");
              for (l = recorded; l != NULL; l = l->next)
                output_event ((RecordedEvent *) l->data);

              g_test_message ("Expected events:");
              for (j = 0; j < n_expected; j++)
                output_event (&expected[j]);

              g_assert_cmpint (e1->step, ==, e2->step);
              g_assert_cmpint (e1->event_type, ==, e2->event_type);

              if (e1->file != DONT_CARE)
                g_assert_cmpstr (e1->file, ==, e2->file);

              if (e1->other_file != DONT_CARE)
                g_assert_cmpstr (e1->other_file, ==, e2->other_file);

              g_assert_not_reached ();
            }
        }

      i++, li++, l = l->next;
      if (l_extra_step)
        li++, l = l->next;
    }

  g_assert_cmpint (i, ==, n_expected);
  g_assert_cmpint (li, ==, xlist_length (recorded));
}

static void
record_event (TestData    *data,
              xint_t         event_type,
              const xchar_t *file,
              const xchar_t *other_file,
              xint_t         step)
{
  RecordedEvent *event;

  event = g_new0 (RecordedEvent, 1);
  event->event_type = event_type;
  event->file = xstrdup (file);
  event->other_file = xstrdup (other_file);
  event->step = step;

  data->events = xlist_append (data->events, event);
}

static void
monitor_changed (xfile_monitor_t      *monitor,
                 xfile_t             *file,
                 xfile_t             *other_file,
                 xfile_monitor_event_t  event_type,
                 xpointer_t           user_data)
{
  TestData *data = user_data;
  xchar_t *basename, *other_base;

  basename = xfile_get_basename (file);
  if (other_file)
    other_base = xfile_get_basename (other_file);
  else
    other_base = NULL;

  record_event (data, event_type, basename, other_base, -1);

  g_free (basename);
  g_free (other_base);
}

static xboolean_t
atomic_replace_step (xpointer_t user_data)
{
  TestData *data = user_data;
  xerror_t *error = NULL;

  switch (data->step)
    {
    case 0:
      record_event (data, -1, NULL, NULL, 0);
      xfile_replace_contents (data->file, "step 0", 6, NULL, FALSE, XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      break;
    case 1:
      record_event (data, -1, NULL, NULL, 1);
      xfile_replace_contents (data->file, "step 1", 6, NULL, FALSE, XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      break;
    case 2:
      record_event (data, -1, NULL, NULL, 2);
      xfile_delete (data->file, NULL, NULL);
      break;
    case 3:
      record_event (data, -1, NULL, NULL, 3);
      xmain_loop_quit (data->loop);
      return XSOURCE_REMOVE;
    }

  data->step++;

  return G_SOURCE_CONTINUE;
}

/* this is the output we expect from the above steps */
static RecordedEvent atomic_replace_output[] = {
  { -1, NULL, NULL, 0, NONE },
  { XFILE_MONITOR_EVENT_CREATED, "atomic_replace_file", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGED, "atomic_replace_file", NULL, -1, KQUEUE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "atomic_replace_file", NULL, -1, KQUEUE },
  { -1, NULL, NULL, 1, NONE },
  { XFILE_MONITOR_EVENT_RENAMED, (xchar_t*)DONT_CARE, "atomic_replace_file", -1, NONE },
  { -1, NULL, NULL, 2, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "atomic_replace_file", NULL, -1, NONE },
  { -1, NULL, NULL, 3, NONE }
};

static void
test_atomic_replace (Fixture       *fixture,
                     xconstpointer  user_data)
{
  xerror_t *error = NULL;
  TestData data;

  data.step = 0;
  data.events = NULL;

  data.file = xfile_get_child (fixture->tmp_dir, "atomic_replace_file");
  xfile_delete (data.file, NULL, NULL);

  data.monitor = xfile_monitor_file (data.file, XFILE_MONITOR_WATCH_MOVES, NULL, &error);
  g_assert_no_error (error);

  g_test_message ("Using xfile_monitor_t %s", G_OBJECT_TYPE_NAME (data.monitor));

  xfile_monitor_set_rate_limit (data.monitor, 200);
  xsignal_connect (data.monitor, "changed", G_CALLBACK (monitor_changed), &data);

  data.loop = xmain_loop_new (NULL, TRUE);

  g_timeout_add (500, atomic_replace_step, &data);

  xmain_loop_run (data.loop);

  check_expected_events (atomic_replace_output,
                         G_N_ELEMENTS (atomic_replace_output),
                         data.events,
                         get_environment (data.monitor));

  xlist_free_full (data.events, (xdestroy_notify_t)free_recorded_event);
  xmain_loop_unref (data.loop);
  xobject_unref (data.monitor);
  xobject_unref (data.file);
}

static xboolean_t
change_step (xpointer_t user_data)
{
  TestData *data = user_data;
  xoutput_stream_t *stream;
  xerror_t *error = NULL;
  xuint32_t mode = 0660;

  switch (data->step)
    {
    case 0:
      record_event (data, -1, NULL, NULL, 0);
      xfile_replace_contents (data->file, "step 0", 6, NULL, FALSE, XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      break;
    case 1:
      record_event (data, -1, NULL, NULL, 1);
      stream = (xoutput_stream_t *)xfile_append_to (data->file, XFILE_CREATE_NONE, NULL, &error);
      g_assert_no_error (error);
      xoutput_stream_write_all (stream, " step 1", 7, NULL, NULL, &error);
      g_assert_no_error (error);
      xoutput_stream_close (stream, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (stream);
      break;
    case 2:
      record_event (data, -1, NULL, NULL, 2);
      xfile_set_attribute (data->file,
                            XFILE_ATTRIBUTE_UNIX_MODE,
                            XFILE_ATTRIBUTE_TYPE_UINT32,
                            &mode,
                            XFILE_QUERY_INFO_NONE,
                            NULL,
                            &error);
      g_assert_no_error (error);
      break;
    case 3:
      record_event (data, -1, NULL, NULL, 3);
      xfile_delete (data->file, NULL, NULL);
      break;
    case 4:
      record_event (data, -1, NULL, NULL, 4);
      xmain_loop_quit (data->loop);
      return XSOURCE_REMOVE;
    }

  data->step++;

  return G_SOURCE_CONTINUE;
}

/* this is the output we expect from the above steps */
static RecordedEvent change_output[] = {
  { -1, NULL, NULL, 0, NONE },
  { XFILE_MONITOR_EVENT_CREATED, "change_file", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGED, "change_file", NULL, -1, KQUEUE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "change_file", NULL, -1, KQUEUE },
  { -1, NULL, NULL, 1, NONE },
  { XFILE_MONITOR_EVENT_CHANGED, "change_file", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "change_file", NULL, -1, NONE },
  { -1, NULL, NULL, 2, NONE },
  { XFILE_MONITOR_EVENT_ATTRIBUTE_CHANGED, "change_file", NULL, -1, NONE },
  { -1, NULL, NULL, 3, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "change_file", NULL, -1, NONE },
  { -1, NULL, NULL, 4, NONE }
};

static void
test_file_changes (Fixture       *fixture,
                   xconstpointer  user_data)
{
  xerror_t *error = NULL;
  TestData data;

  data.step = 0;
  data.events = NULL;

  data.file = xfile_get_child (fixture->tmp_dir, "change_file");
  xfile_delete (data.file, NULL, NULL);

  data.monitor = xfile_monitor_file (data.file, XFILE_MONITOR_WATCH_MOVES, NULL, &error);
  g_assert_no_error (error);

  g_test_message ("Using xfile_monitor_t %s", G_OBJECT_TYPE_NAME (data.monitor));

  xfile_monitor_set_rate_limit (data.monitor, 200);
  xsignal_connect (data.monitor, "changed", G_CALLBACK (monitor_changed), &data);

  data.loop = xmain_loop_new (NULL, TRUE);

  g_timeout_add (500, change_step, &data);

  xmain_loop_run (data.loop);

  check_expected_events (change_output,
                         G_N_ELEMENTS (change_output),
                         data.events,
                         get_environment (data.monitor));

  xlist_free_full (data.events, (xdestroy_notify_t)free_recorded_event);
  xmain_loop_unref (data.loop);
  xobject_unref (data.monitor);
  xobject_unref (data.file);
}

static xboolean_t
dir_step (xpointer_t user_data)
{
  TestData *data = user_data;
  xfile_t *parent, *file, *file2;
  xerror_t *error = NULL;

  switch (data->step)
    {
    case 1:
      record_event (data, -1, NULL, NULL, 1);
      parent = xfile_get_parent (data->file);
      file = xfile_get_child (parent, "dir_test_file");
      xfile_replace_contents (file, "step 1", 6, NULL, FALSE, XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (file);
      xobject_unref (parent);
      break;
    case 2:
      record_event (data, -1, NULL, NULL, 2);
      parent = xfile_get_parent (data->file);
      file = xfile_get_child (parent, "dir_test_file");
      file2 = xfile_get_child (data->file, "dir_test_file");
      xfile_move (file, file2, XFILE_COPY_NONE, NULL, NULL, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (file);
      xobject_unref (file2);
      xobject_unref (parent);
      break;
    case 3:
      record_event (data, -1, NULL, NULL, 3);
      file = xfile_get_child (data->file, "dir_test_file");
      file2 = xfile_get_child (data->file, "dir_test_file2");
      xfile_move (file, file2, XFILE_COPY_NONE, NULL, NULL, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (file);
      xobject_unref (file2);
      break;
    case 4:
      record_event (data, -1, NULL, NULL, 4);
      parent = xfile_get_parent (data->file);
      file = xfile_get_child (data->file, "dir_test_file2");
      file2 = xfile_get_child (parent, "dir_test_file2");
      xfile_move (file, file2, XFILE_COPY_NONE, NULL, NULL, NULL, &error);
      g_assert_no_error (error);
      xfile_delete (file2, NULL, NULL);
      xobject_unref (file);
      xobject_unref (file2);
      xobject_unref (parent);
      break;
    case 5:
      record_event (data, -1, NULL, NULL, 5);
      xfile_delete (data->file, NULL, NULL);
      break;
    case 6:
      record_event (data, -1, NULL, NULL, 6);
      xmain_loop_quit (data->loop);
      return XSOURCE_REMOVE;
    }

  data->step++;

  return G_SOURCE_CONTINUE;
}

/* this is the output we expect from the above steps */
static RecordedEvent dir_output[] = {
  { -1, NULL, NULL, 1, NONE },
  { -1, NULL, NULL, 2, NONE },
  { XFILE_MONITOR_EVENT_MOVED_IN, "dir_test_file", NULL, -1, NONE },
  { -1, NULL, NULL, 3, NONE },
  { XFILE_MONITOR_EVENT_RENAMED, "dir_test_file", "dir_test_file2", -1, NONE },
  { -1, NULL, NULL, 4, NONE },
  { XFILE_MONITOR_EVENT_MOVED_OUT, "dir_test_file2", NULL, -1, NONE },
  { -1, NULL, NULL, 5, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "dir_monitor_test", NULL, -1, NONE },
  { -1, NULL, NULL, 6, NONE }
};

static void
test_dir_monitor (Fixture       *fixture,
                  xconstpointer  user_data)
{
  xerror_t *error = NULL;
  TestData data;

  data.step = 0;
  data.events = NULL;

  data.file = xfile_get_child (fixture->tmp_dir, "dir_monitor_test");
  xfile_delete (data.file, NULL, NULL);
  xfile_make_directory (data.file, NULL, &error);

  data.monitor = xfile_monitor_directory (data.file, XFILE_MONITOR_WATCH_MOVES, NULL, &error);
  g_assert_no_error (error);

  g_test_message ("Using xfile_monitor_t %s", G_OBJECT_TYPE_NAME (data.monitor));

  xfile_monitor_set_rate_limit (data.monitor, 200);
  xsignal_connect (data.monitor, "changed", G_CALLBACK (monitor_changed), &data);

  data.loop = xmain_loop_new (NULL, TRUE);

  g_timeout_add (500, dir_step, &data);

  xmain_loop_run (data.loop);

  check_expected_events (dir_output,
                         G_N_ELEMENTS (dir_output),
                         data.events,
                         get_environment (data.monitor));

  xlist_free_full (data.events, (xdestroy_notify_t)free_recorded_event);
  xmain_loop_unref (data.loop);
  xobject_unref (data.monitor);
  xobject_unref (data.file);
}

static xboolean_t
nodir_step (xpointer_t user_data)
{
  TestData *data = user_data;
  xfile_t *parent;
  xerror_t *error = NULL;

  switch (data->step)
    {
    case 0:
      record_event (data, -1, NULL, NULL, 0);
      parent = xfile_get_parent (data->file);
      xfile_make_directory (parent, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (parent);
      break;
    case 1:
      record_event (data, -1, NULL, NULL, 1);
      xfile_replace_contents (data->file, "step 1", 6, NULL, FALSE, XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      break;
    case 2:
      record_event (data, -1, NULL, NULL, 2);
      xfile_delete (data->file, NULL, &error);
      g_assert_no_error (error);
      break;
    case 3:
      record_event (data, -1, NULL, NULL, 3);
      parent = xfile_get_parent (data->file);
      xfile_delete (parent, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (parent);
      break;
    case 4:
      record_event (data, -1, NULL, NULL, 4);
      xmain_loop_quit (data->loop);
      return XSOURCE_REMOVE;
    }

  data->step++;

  return G_SOURCE_CONTINUE;
}

static RecordedEvent nodir_output[] = {
  { -1, NULL, NULL, 0, NONE },
  { XFILE_MONITOR_EVENT_CREATED, "nosuchfile", NULL, -1, KQUEUE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "nosuchfile", NULL, -1, KQUEUE },
  { -1, NULL, NULL, 1, NONE },
  { XFILE_MONITOR_EVENT_CREATED, "nosuchfile", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGED, "nosuchfile", NULL, -1, KQUEUE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "nosuchfile", NULL, -1, KQUEUE },
  { -1, NULL, NULL, 2, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "nosuchfile", NULL, -1, NONE },
  { -1, NULL, NULL, 3, NONE },
  { -1, NULL, NULL, 4, NONE }
};

static void
test_dir_non_existent (Fixture       *fixture,
                       xconstpointer  user_data)
{
  TestData data;
  xerror_t *error = NULL;

  data.step = 0;
  data.events = NULL;

  data.file = xfile_get_child (fixture->tmp_dir, "nosuchdir/nosuchfile");
  data.monitor = xfile_monitor_file (data.file, XFILE_MONITOR_WATCH_MOVES, NULL, &error);
  g_assert_no_error (error);

  g_test_message ("Using xfile_monitor_t %s", G_OBJECT_TYPE_NAME (data.monitor));

  xfile_monitor_set_rate_limit (data.monitor, 200);
  xsignal_connect (data.monitor, "changed", G_CALLBACK (monitor_changed), &data);

  data.loop = xmain_loop_new (NULL, TRUE);

  /* we need a long timeout here, since the inotify implementation only scans
   * for missing files every 4 seconds.
   */
  g_timeout_add (5000, nodir_step, &data);

  xmain_loop_run (data.loop);

  check_expected_events (nodir_output,
                         G_N_ELEMENTS (nodir_output),
                         data.events,
                         get_environment (data.monitor));

  xlist_free_full (data.events, (xdestroy_notify_t)free_recorded_event);
  xmain_loop_unref (data.loop);
  xobject_unref (data.monitor);
  xobject_unref (data.file);
}

static xboolean_t
cross_dir_step (xpointer_t user_data)
{
  TestData *data = user_data;
  xfile_t *file, *file2;
  xerror_t *error = NULL;

  switch (data[0].step)
    {
    case 0:
      record_event (&data[0], -1, NULL, NULL, 0);
      record_event (&data[1], -1, NULL, NULL, 0);
      file = xfile_get_child (data[1].file, "a");
      xfile_replace_contents (file, "step 0", 6, NULL, FALSE, XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (file);
      break;
    case 1:
      record_event (&data[0], -1, NULL, NULL, 1);
      record_event (&data[1], -1, NULL, NULL, 1);
      file = xfile_get_child (data[1].file, "a");
      file2 = xfile_get_child (data[0].file, "a");
      xfile_move (file, file2, 0, NULL, NULL, NULL, &error);
      g_assert_no_error (error);
      xobject_unref (file);
      xobject_unref (file2);
      break;
    case 2:
      record_event (&data[0], -1, NULL, NULL, 2);
      record_event (&data[1], -1, NULL, NULL, 2);
      file2 = xfile_get_child (data[0].file, "a");
      xfile_delete (file2, NULL, NULL);
      xfile_delete (data[0].file, NULL, NULL);
      xfile_delete (data[1].file, NULL, NULL);
      xobject_unref (file2);
      break;
    case 3:
      record_event (&data[0], -1, NULL, NULL, 3);
      record_event (&data[1], -1, NULL, NULL, 3);
      xmain_loop_quit (data->loop);
      return XSOURCE_REMOVE;
    }

  data->step++;

  return G_SOURCE_CONTINUE;
}

static RecordedEvent cross_dir_a_output[] = {
  { -1, NULL, NULL, 0, NONE },
  { -1, NULL, NULL, 1, NONE },
  { XFILE_MONITOR_EVENT_CREATED, "a", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "a", NULL, -1, KQUEUE },
  { -1, NULL, NULL, 2, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "a", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "cross_dir_a", NULL, -1, NONE },
  { -1, NULL, NULL, 3, NONE },
};

static RecordedEvent cross_dir_b_output[] = {
  { -1, NULL, NULL, 0, NONE },
  { XFILE_MONITOR_EVENT_CREATED, "a", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGED, "a", NULL, -1, KQUEUE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "a", NULL, -1, KQUEUE },
  { -1, NULL, NULL, 1, NONE },
  { XFILE_MONITOR_EVENT_MOVED_OUT, "a", "a", -1, NONE },
  { -1, NULL, NULL, 2, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "cross_dir_b", NULL, -1, NONE },
  { -1, NULL, NULL, 3, NONE },
};
static void
test_cross_dir_moves (Fixture       *fixture,
                      xconstpointer  user_data)
{
  xerror_t *error = NULL;
  TestData data[2];

  data[0].step = 0;
  data[0].events = NULL;

  data[0].file = xfile_get_child (fixture->tmp_dir, "cross_dir_a");
  xfile_delete (data[0].file, NULL, NULL);
  xfile_make_directory (data[0].file, NULL, &error);

  data[0].monitor = xfile_monitor_directory (data[0].file, 0, NULL, &error);
  g_assert_no_error (error);

  g_test_message ("Using xfile_monitor_t 0 %s", G_OBJECT_TYPE_NAME (data[0].monitor));

  xfile_monitor_set_rate_limit (data[0].monitor, 200);
  xsignal_connect (data[0].monitor, "changed", G_CALLBACK (monitor_changed), &data[0]);

  data[1].step = 0;
  data[1].events = NULL;

  data[1].file = xfile_get_child (fixture->tmp_dir, "cross_dir_b");
  xfile_delete (data[1].file, NULL, NULL);
  xfile_make_directory (data[1].file, NULL, &error);

  data[1].monitor = xfile_monitor_directory (data[1].file, XFILE_MONITOR_WATCH_MOVES, NULL, &error);
  g_assert_no_error (error);

  g_test_message ("Using xfile_monitor_t 1 %s", G_OBJECT_TYPE_NAME (data[1].monitor));

  xfile_monitor_set_rate_limit (data[1].monitor, 200);
  xsignal_connect (data[1].monitor, "changed", G_CALLBACK (monitor_changed), &data[1]);

  data[0].loop = xmain_loop_new (NULL, TRUE);

  g_timeout_add (500, cross_dir_step, data);

  xmain_loop_run (data[0].loop);

  check_expected_events (cross_dir_a_output,
                         G_N_ELEMENTS (cross_dir_a_output),
                         data[0].events,
                         get_environment (data[0].monitor));
  check_expected_events (cross_dir_b_output,
                         G_N_ELEMENTS (cross_dir_b_output),
                         data[1].events,
                         get_environment (data[1].monitor));

  xlist_free_full (data[0].events, (xdestroy_notify_t)free_recorded_event);
  xmain_loop_unref (data[0].loop);
  xobject_unref (data[0].monitor);
  xobject_unref (data[0].file);

  xlist_free_full (data[1].events, (xdestroy_notify_t)free_recorded_event);
  xobject_unref (data[1].monitor);
  xobject_unref (data[1].file);
}

static xboolean_t
file_hard_links_step (xpointer_t user_data)
{
  xboolean_t retval = G_SOURCE_CONTINUE;
  TestData *data = user_data;
  xerror_t *error = NULL;

  xchar_t *filename = xfile_get_path (data->file);
  xchar_t *hard_link_name = xstrdup_printf ("%s2", filename);
  xfile_t *hard_link_file = xfile_new_for_path (hard_link_name);

  switch (data->step)
    {
    case 0:
      record_event (data, -1, NULL, NULL, 0);
      xoutput_stream_write_all (G_OUTPUT_STREAM (data->output_stream),
                                 "hello, step 0", 13, NULL, NULL, &error);
      g_assert_no_error (error);
      xoutput_stream_close (G_OUTPUT_STREAM (data->output_stream), NULL, &error);
      g_assert_no_error (error);
      break;
    case 1:
      record_event (data, -1, NULL, NULL, 1);
      xfile_replace_contents (data->file, "step 1", 6, NULL, FALSE,
                               XFILE_CREATE_NONE, NULL, NULL, &error);
      g_assert_no_error (error);
      break;
    case 2:
      record_event (data, -1, NULL, NULL, 2);
#ifdef HAVE_LINK
      if (link (filename, hard_link_name) < 0)
        {
          xerror ("link(%s, %s) failed: %s", filename, hard_link_name, xstrerror (errno));
        }
#endif  /* HAVE_LINK */
      break;
    case 3:
      record_event (data, -1, NULL, NULL, 3);
#ifdef HAVE_LINK
      {
        xoutput_stream_t *hard_link_stream = NULL;

        /* Deliberately don’t do an atomic swap on the hard-linked file. */
        hard_link_stream = G_OUTPUT_STREAM (xfile_append_to (hard_link_file,
                                                              XFILE_CREATE_NONE,
                                                              NULL, &error));
        g_assert_no_error (error);
        xoutput_stream_write_all (hard_link_stream, " step 3", 7, NULL, NULL, &error);
        g_assert_no_error (error);
        xoutput_stream_close (hard_link_stream, NULL, &error);
        g_assert_no_error (error);
        xobject_unref (hard_link_stream);
      }
#endif  /* HAVE_LINK */
      break;
    case 4:
      record_event (data, -1, NULL, NULL, 4);
      xfile_delete (data->file, NULL, &error);
      g_assert_no_error (error);
      break;
    case 5:
      record_event (data, -1, NULL, NULL, 5);
#ifdef HAVE_LINK
      xfile_delete (hard_link_file, NULL, &error);
      g_assert_no_error (error);
#endif  /* HAVE_LINK */
      break;
    case 6:
      record_event (data, -1, NULL, NULL, 6);
      xmain_loop_quit (data->loop);
      retval = XSOURCE_REMOVE;
      break;
    }

  if (retval != XSOURCE_REMOVE)
    data->step++;

  xobject_unref (hard_link_file);
  g_free (hard_link_name);
  g_free (filename);

  return retval;
}

static RecordedEvent file_hard_links_output[] = {
  { -1, NULL, NULL, 0, NONE },
  { XFILE_MONITOR_EVENT_CHANGED, "testfilemonitor.db", NULL, -1, NONE },
  { XFILE_MONITOR_EVENT_CHANGES_DONE_HINT, "testfilemonitor.db", NULL, -1, NONE },
  { -1, NULL, NULL, 1, NONE },
  { XFILE_MONITOR_EVENT_RENAMED, (xchar_t*)DONT_CARE /* .goutputstream-XXXXXX */, "testfilemonitor.db", -1, NONE },
  { -1, NULL, NULL, 2, NONE },
  { -1, NULL, NULL, 3, NONE },
  /* Kqueue is based on file descriptors. You can get events from all hard
   * links by just monitoring one open file descriptor, and it is not possible
   * to know whether it is done on the file name we use to open the file. Since
   * the hard link count of 'testfilemonitor.db' is 2, it is expected to see
   * two 'DELETED' events reported here. You have to call 'unlink' twice on
   * different file names to remove 'testfilemonitor.db' from the file system,
   * and each 'unlink' call generates a 'DELETED' event. */
  { XFILE_MONITOR_EVENT_CHANGED, "testfilemonitor.db", NULL, -1, INOTIFY },
  { -1, NULL, NULL, 4, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "testfilemonitor.db", NULL, -1, NONE },
  { -1, NULL, NULL, 5, NONE },
  { XFILE_MONITOR_EVENT_DELETED, "testfilemonitor.db", NULL, -1, INOTIFY },
  { -1, NULL, NULL, 6, NONE },
};

static void
test_file_hard_links (Fixture       *fixture,
                      xconstpointer  user_data)
{
  xerror_t *error = NULL;
  TestData data;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=755721");

#ifdef HAVE_LINK
  g_test_message ("Running with hard link tests");
#else  /* if !HAVE_LINK */
  g_test_message ("Running without hard link tests");
#endif  /* !HAVE_LINK */

  data.step = 0;
  data.events = NULL;

  /* Create a file which exists and is not a directory. */
  data.file = xfile_get_child (fixture->tmp_dir, "testfilemonitor.db");
  data.output_stream = xfile_replace (data.file, NULL, FALSE,
                                       XFILE_CREATE_NONE, NULL, &error);
  g_assert_no_error (error);

  /* Monitor it. Creating the monitor should not crash (bug #755721). */
  data.monitor = xfile_monitor_file (data.file,
                                      XFILE_MONITOR_WATCH_MOUNTS |
                                      XFILE_MONITOR_WATCH_MOVES |
                                      XFILE_MONITOR_WATCH_HARD_LINKS,
                                      NULL,
                                      &error);
  g_assert_no_error (error);
  g_assert_nonnull (data.monitor);

  g_test_message ("Using xfile_monitor_t %s", G_OBJECT_TYPE_NAME (data.monitor));

  /* Change the file a bit. */
  xfile_monitor_set_rate_limit (data.monitor, 200);
  xsignal_connect (data.monitor, "changed", (xcallback_t) monitor_changed, &data);

  data.loop = xmain_loop_new (NULL, TRUE);
  g_timeout_add (500, file_hard_links_step, &data);
  xmain_loop_run (data.loop);

  check_expected_events (file_hard_links_output,
                         G_N_ELEMENTS (file_hard_links_output),
                         data.events,
                         get_environment (data.monitor));

  xlist_free_full (data.events, (xdestroy_notify_t) free_recorded_event);
  xmain_loop_unref (data.loop);
  xobject_unref (data.monitor);
  xobject_unref (data.file);
  xobject_unref (data.output_stream);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/monitor/atomic-replace", Fixture, NULL, setup, test_atomic_replace, teardown);
  g_test_add ("/monitor/file-changes", Fixture, NULL, setup, test_file_changes, teardown);
  g_test_add ("/monitor/dir-monitor", Fixture, NULL, setup, test_dir_monitor, teardown);
  g_test_add ("/monitor/dir-not-existent", Fixture, NULL, setup, test_dir_non_existent, teardown);
  g_test_add ("/monitor/cross-dir-moves", Fixture, NULL, setup, test_cross_dir_moves, teardown);
  g_test_add ("/monitor/file/hard-links", Fixture, NULL, setup, test_file_hard_links, teardown);

  return g_test_run ();
}
