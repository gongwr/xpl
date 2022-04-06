#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <gio/gfiledescriptorbased.h>
#ifdef G_OS_UNIX
#include <sys/stat.h>
#endif

static void
test_basic_for_file (xfile_t       *file,
                     const xchar_t *suffix)
{
  xchar_t *s;

  s = xfile_get_basename (file);
  g_assert_cmpstr (s, ==, "testfile");
  g_free (s);

  s = xfile_get_uri (file);
  g_assert_true (xstr_has_prefix (s, "file://"));
  g_assert_true (xstr_has_suffix (s, suffix));
  g_free (s);

  g_assert_true (xfile_has_uri_scheme (file, "file"));
  s = xfile_get_uri_scheme (file);
  g_assert_cmpstr (s, ==, "file");
  g_free (s);
}

static void
test_basic (void)
{
  xfile_t *file;

  file = xfile_new_for_path ("./some/directory/testfile");
  test_basic_for_file (file, "/some/directory/testfile");
  xobject_unref (file);
}

static void
test_build_filename (void)
{
  xfile_t *file;

  file = xfile_new_build_filename (".", "some", "directory", "testfile", NULL);
  test_basic_for_file (file, "/some/directory/testfile");
  xobject_unref (file);

  file = xfile_new_build_filename ("testfile", NULL);
  test_basic_for_file (file, "/testfile");
  xobject_unref (file);
}

static void
test_parent (void)
{
  xfile_t *file;
  xfile_t *file2;
  xfile_t *parent;
  xfile_t *root;

  file = xfile_new_for_path ("./some/directory/testfile");
  file2 = xfile_new_for_path ("./some/directory");
  root = xfile_new_for_path ("/");

  g_assert_true (xfile_has_parent (file, file2));

  parent = xfile_get_parent (file);
  g_assert_true (xfile_equal (parent, file2));
  xobject_unref (parent);

  g_assert_null (xfile_get_parent (root));

  xobject_unref (file);
  xobject_unref (file2);
  xobject_unref (root);
}

static void
test_child (void)
{
  xfile_t *file;
  xfile_t *child;
  xfile_t *child2;

  file = xfile_new_for_path ("./some/directory");
  child = xfile_get_child (file, "child");
  g_assert_true (xfile_has_parent (child, file));

  child2 = xfile_get_child_for_display_name (file, "child2", NULL);
  g_assert_true (xfile_has_parent (child2, file));

  xobject_unref (child);
  xobject_unref (child2);
  xobject_unref (file);
}

static void
test_empty_path (void)
{
  xfile_t *file = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2328");
  g_test_summary ("Check that creating a file with an empty path results in errors");

  /* Creating the file must always succeed. */
  file = xfile_new_for_path ("");
  g_assert_nonnull (file);

  /* But then querying its path should indicate it’s invalid. */
  g_assert_null (xfile_get_path (file));
  g_assert_null (xfile_get_basename (file));
  g_assert_null (xfile_get_parent (file));

  xobject_unref (file);
}

static void
test_type (void)
{
  xfile_t *datapath_f;
  xfile_t *file;
  xfile_type_t type;
  xerror_t *error = NULL;

  datapath_f = xfile_new_for_path (g_test_get_dir (G_TEST_DIST));

  file = xfile_get_child (datapath_f, "g-icon.c");
  type = xfile_query_file_type (file, 0, NULL);
  g_assert_cmpint (type, ==, XFILE_TYPE_REGULAR);
  xobject_unref (file);

  file = xfile_get_child (datapath_f, "cert-tests");
  type = xfile_query_file_type (file, 0, NULL);
  g_assert_cmpint (type, ==, XFILE_TYPE_DIRECTORY);

  xfile_read (file, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY);
  xerror_free (error);
  xobject_unref (file);

  xobject_unref (datapath_f);
}

static void
test_parse_name (void)
{
  xfile_t *file;
  xchar_t *name;

  file = xfile_new_for_uri ("file://somewhere");
  name = xfile_get_parse_name (file);
  g_assert_cmpstr (name, ==, "file://somewhere");
  xobject_unref (file);
  g_free (name);

  file = xfile_parse_name ("~foo");
  name = xfile_get_parse_name (file);
  g_assert_nonnull (name);
  xobject_unref (file);
  g_free (name);
}

typedef struct
{
  xmain_context_t *context;
  xfile_t *file;
  xfile_monitor_t *monitor;
  xoutput_stream_t *ostream;
  xinput_stream_t *istream;
  xint_t buffersize;
  xint_t monitor_created;
  xint_t monitor_deleted;
  xint_t monitor_changed;
  xchar_t *monitor_path;
  xsize_t pos;
  const xchar_t *data;
  xchar_t *buffer;
  xuint_t timeout;
  xboolean_t file_deleted;
  xboolean_t timed_out;
} CreateDeleteData;

static void
monitor_changed (xfile_monitor_t      *monitor,
                 xfile_t             *file,
                 xfile_t             *other_file,
                 xfile_monitor_event_t  event_type,
                 xpointer_t           user_data)
{
  CreateDeleteData *data = user_data;
  xchar_t *path;
  const xchar_t *peeked_path;

  path = xfile_get_path (file);
  peeked_path = xfile_peek_path (file);
  g_assert_cmpstr (data->monitor_path, ==, path);
  g_assert_cmpstr (path, ==, peeked_path);
  g_free (path);

  if (event_type == XFILE_MONITOR_EVENT_CREATED)
    data->monitor_created++;
  if (event_type == XFILE_MONITOR_EVENT_DELETED)
    data->monitor_deleted++;
  if (event_type == XFILE_MONITOR_EVENT_CHANGED)
    data->monitor_changed++;

  xmain_context_wakeup (data->context);
}

static void
iclosed_cb (xobject_t      *source,
            xasync_result_t *res,
            xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xerror_t *error;
  xboolean_t ret;

  error = NULL;
  ret = xinput_stream_close_finish (data->istream, res, &error);
  g_assert_no_error (error);
  g_assert_true (ret);

  g_assert_true (xinput_stream_is_closed (data->istream));

  ret = xfile_delete (data->file, NULL, &error);
  g_assert_true (ret);
  g_assert_no_error (error);

  data->file_deleted = TRUE;
  xmain_context_wakeup (data->context);
}

static void
read_cb (xobject_t      *source,
         xasync_result_t *res,
         xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xerror_t *error;
  xssize_t size;

  error = NULL;
  size = xinput_stream_read_finish (data->istream, res, &error);
  g_assert_no_error (error);

  data->pos += size;
  if (data->pos < strlen (data->data))
    {
      xinput_stream_read_async (data->istream,
                                 data->buffer + data->pos,
                                 strlen (data->data) - data->pos,
                                 0,
                                 NULL,
                                 read_cb,
                                 data);
    }
  else
    {
      g_assert_cmpstr (data->buffer, ==, data->data);
      g_assert_false (xinput_stream_is_closed (data->istream));
      xinput_stream_close_async (data->istream, 0, NULL, iclosed_cb, data);
    }
}

static void
ipending_cb (xobject_t      *source,
             xasync_result_t *res,
             xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xerror_t *error;

  error = NULL;
  xinput_stream_read_finish (data->istream, res, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  xerror_free (error);
}

static void
skipped_cb (xobject_t      *source,
            xasync_result_t *res,
            xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xerror_t *error;
  xssize_t size;

  error = NULL;
  size = xinput_stream_skip_finish (data->istream, res, &error);
  g_assert_no_error (error);
  g_assert_cmpint (size, ==, data->pos);

  xinput_stream_read_async (data->istream,
                             data->buffer + data->pos,
                             strlen (data->data) - data->pos,
                             0,
                             NULL,
                             read_cb,
                             data);
  /* check that we get a pending error */
  xinput_stream_read_async (data->istream,
                             data->buffer + data->pos,
                             strlen (data->data) - data->pos,
                             0,
                             NULL,
                             ipending_cb,
                             data);
}

static void
opened_cb (xobject_t      *source,
           xasync_result_t *res,
           xpointer_t      user_data)
{
  xfile_input_stream_t *base;
  CreateDeleteData *data = user_data;
  xerror_t *error;

  error = NULL;
  base = xfile_read_finish (data->file, res, &error);
  g_assert_no_error (error);

  if (data->buffersize == 0)
    data->istream = G_INPUT_STREAM (xobject_ref (base));
  else
    data->istream = xbuffered_input_stream_new_sized (G_INPUT_STREAM (base), data->buffersize);
  xobject_unref (base);

  data->buffer = g_new0 (xchar_t, strlen (data->data) + 1);

  /* copy initial segment directly, then skip */
  memcpy (data->buffer, data->data, 10);
  data->pos = 10;

  xinput_stream_skip_async (data->istream,
                             10,
                             0,
                             NULL,
                             skipped_cb,
                             data);
}

static void
oclosed_cb (xobject_t      *source,
            xasync_result_t *res,
            xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xerror_t *error;
  xboolean_t ret;

  error = NULL;
  ret = xoutput_stream_close_finish (data->ostream, res, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  g_assert_true (xoutput_stream_is_closed (data->ostream));

  xfile_read_async (data->file, 0, NULL, opened_cb, data);
}

static void
written_cb (xobject_t      *source,
            xasync_result_t *res,
            xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xssize_t size;
  xerror_t *error;

  error = NULL;
  size = xoutput_stream_write_finish (data->ostream, res, &error);
  g_assert_no_error (error);

  data->pos += size;
  if (data->pos < strlen (data->data))
    {
      xoutput_stream_write_async (data->ostream,
                                   data->data + data->pos,
                                   strlen (data->data) - data->pos,
                                   0,
                                   NULL,
                                   written_cb,
                                   data);
    }
  else
    {
      g_assert_false (xoutput_stream_is_closed (data->ostream));
      xoutput_stream_close_async (data->ostream, 0, NULL, oclosed_cb, data);
    }
}

static void
opending_cb (xobject_t      *source,
             xasync_result_t *res,
             xpointer_t      user_data)
{
  CreateDeleteData *data = user_data;
  xerror_t *error;

  error = NULL;
  xoutput_stream_write_finish (data->ostream, res, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
  xerror_free (error);
}

static void
created_cb (xobject_t      *source,
            xasync_result_t *res,
            xpointer_t      user_data)
{
  xfile_output_stream_t *base;
  CreateDeleteData *data = user_data;
  xerror_t *error;

  error = NULL;
  base = xfile_create_finish (XFILE (source), res, &error);
  g_assert_no_error (error);
  g_assert_true (xfile_query_exists (data->file, NULL));

  if (data->buffersize == 0)
    data->ostream = G_OUTPUT_STREAM (xobject_ref (base));
  else
    data->ostream = g_buffered_output_stream_new_sized (G_OUTPUT_STREAM (base), data->buffersize);
  xobject_unref (base);

  xoutput_stream_write_async (data->ostream,
                               data->data,
                               strlen (data->data),
                               0,
                               NULL,
                               written_cb,
                               data);
  /* check that we get a pending error */
  xoutput_stream_write_async (data->ostream,
                               data->data,
                               strlen (data->data),
                               0,
                               NULL,
                               opending_cb,
                               data);
}

static xboolean_t
stop_timeout (xpointer_t user_data)
{
  CreateDeleteData *data = user_data;

  data->timed_out = TRUE;
  xmain_context_wakeup (data->context);

  return G_SOURCE_REMOVE;
}

/*
 * This test does a fully async create-write-read-delete.
 * Callbackistan.
 */
static void
test_create_delete (xconstpointer d)
{
  xerror_t *error;
  CreateDeleteData *data;
  xfile_io_stream_t *iostream;

  data = g_new0 (CreateDeleteData, 1);

  data->buffersize = GPOINTER_TO_INT (d);
  data->data = "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ0123456789";
  data->pos = 0;

  data->file = xfile_new_tmp ("xfile_create_delete_XXXXXX",
			       &iostream, NULL);
  g_assert_nonnull (data->file);
  xobject_unref (iostream);

  data->monitor_path = xfile_get_path (data->file);
  remove (data->monitor_path);

  g_assert_false (xfile_query_exists (data->file, NULL));

  error = NULL;
  data->monitor = xfile_monitor_file (data->file, 0, NULL, &error);
  g_assert_no_error (error);

  /* This test doesn't work with GPollFileMonitor, because it assumes
   * that the monitor will notice a create immediately followed by a
   * delete, rather than coalescing them into nothing.
   */
  /* This test also doesn't work with GKqueueFileMonitor because of
   * the same reason. Kqueue is able to return a kevent when a file is
   * created or deleted in a directory. However, the kernel doesn't tell
   * the program file names, so GKqueueFileMonitor has to calculate the
   * difference itself. This is usually too slow for rapid file creation
   * and deletion tests.
   */
  if (strcmp (G_OBJECT_TYPE_NAME (data->monitor), "GPollFileMonitor") == 0 ||
      strcmp (G_OBJECT_TYPE_NAME (data->monitor), "GKqueueFileMonitor") == 0)
    {
      g_test_skip ("skipping test for this xfile_monitor_t implementation");
      goto skip;
    }

  xfile_monitor_set_rate_limit (data->monitor, 100);

  g_signal_connect (data->monitor, "changed", G_CALLBACK (monitor_changed), data);

  /* Use the global default main context */
  data->context = NULL;
  data->timeout = g_timeout_add_seconds (10, stop_timeout, data);

  xfile_create_async (data->file, 0, 0, NULL, created_cb, data);

  while (!data->timed_out &&
         (data->monitor_created == 0 ||
          data->monitor_deleted == 0 ||
          data->monitor_changed == 0 ||
          !data->file_deleted))
    xmain_context_iteration (data->context, TRUE);

  xsource_remove (data->timeout);

  g_assert_false (data->timed_out);
  g_assert_true (data->file_deleted);
  g_assert_cmpint (data->monitor_created, ==, 1);
  g_assert_cmpint (data->monitor_deleted, ==, 1);
  g_assert_cmpint (data->monitor_changed, >, 0);

  g_assert_false (xfile_monitor_is_cancelled (data->monitor));
  xfile_monitor_cancel (data->monitor);
  g_assert_true (xfile_monitor_is_cancelled (data->monitor));

  g_clear_pointer (&data->context, xmain_context_unref);
  xobject_unref (data->ostream);
  xobject_unref (data->istream);

 skip:
  xobject_unref (data->monitor);
  xobject_unref (data->file);
  g_free (data->monitor_path);
  g_free (data->buffer);
  g_free (data);
}

static const xchar_t *original_data =
    "/**\n"
    " * xfile_replace_contents_async:\n"
    "**/\n";

static const xchar_t *replace_data =
    "/**\n"
    " * xfile_replace_contents_async:\n"
    " * @file: input #xfile_t.\n"
    " * @contents: string of contents to replace the file with.\n"
    " * @length: the length of @contents in bytes.\n"
    " * @etag: (nullable): a new <link linkend=\"gfile-etag\">entity tag</link> for the @file, or %NULL\n"
    " * @make_backup: %TRUE if a backup should be created.\n"
    " * @flags: a set of #xfile_create_flags_t.\n"
    " * @cancellable: optional #xcancellable_t object, %NULL to ignore.\n"
    " * @callback: a #xasync_ready_callback_t to call when the request is satisfied\n"
    " * @user_data: the data to pass to callback function\n"
    " * \n"
    " * Starts an asynchronous replacement of @file with the given \n"
    " * @contents of @length bytes. @etag will replace the document's\n"
    " * current entity tag.\n"
    " * \n"
    " * When this operation has completed, @callback will be called with\n"
    " * @user_user data, and the operation can be finalized with \n"
    " * xfile_replace_contents_finish().\n"
    " * \n"
    " * If @cancellable is not %NULL, then the operation can be cancelled by\n"
    " * triggering the cancellable object from another thread. If the operation\n"
    " * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. \n"
    " * \n"
    " * If @make_backup is %TRUE, this function will attempt to \n"
    " * make a backup of @file.\n"
    " **/\n";

typedef struct
{
  xfile_t *file;
  const xchar_t *data;
  xmain_loop_t *loop;
  xboolean_t again;
} ReplaceLoadData;

static void replaced_cb (xobject_t      *source,
                         xasync_result_t *res,
                         xpointer_t      user_data);

static void
loaded_cb (xobject_t      *source,
           xasync_result_t *res,
           xpointer_t      user_data)
{
  ReplaceLoadData *data = user_data;
  xboolean_t ret;
  xerror_t *error;
  xchar_t *contents;
  xsize_t length;

  error = NULL;
  ret = xfile_load_contents_finish (data->file, res, &contents, &length, NULL, &error);
  g_assert_true (ret);
  g_assert_no_error (error);
  g_assert_cmpint (length, ==, strlen (data->data));
  g_assert_cmpstr (contents, ==, data->data);

  g_free (contents);

  if (data->again)
    {
      data->again = FALSE;
      data->data = "pi pa po";

      xfile_replace_contents_async (data->file,
                                     data->data,
                                     strlen (data->data),
                                     NULL,
                                     FALSE,
                                     0,
                                     NULL,
                                     replaced_cb,
                                     data);
    }
  else
    {
       error = NULL;
       ret = xfile_delete (data->file, NULL, &error);
       g_assert_no_error (error);
       g_assert_true (ret);
       g_assert_false (xfile_query_exists (data->file, NULL));

       xmain_loop_quit (data->loop);
    }
}

static void
replaced_cb (xobject_t      *source,
             xasync_result_t *res,
             xpointer_t      user_data)
{
  ReplaceLoadData *data = user_data;
  xerror_t *error;

  error = NULL;
  xfile_replace_contents_finish (data->file, res, NULL, &error);
  g_assert_no_error (error);

  xfile_load_contents_async (data->file, NULL, loaded_cb, data);
}

static void
test_replace_load (void)
{
  ReplaceLoadData *data;
  const xchar_t *path;
  xfile_io_stream_t *iostream;

  data = g_new0 (ReplaceLoadData, 1);
  data->again = TRUE;
  data->data = replace_data;

  data->file = xfile_new_tmp ("xfile_replace_load_XXXXXX",
			       &iostream, NULL);
  g_assert_nonnull (data->file);
  xobject_unref (iostream);

  path = xfile_peek_path (data->file);
  remove (path);

  g_assert_false (xfile_query_exists (data->file, NULL));

  data->loop = xmain_loop_new (NULL, FALSE);

  xfile_replace_contents_async (data->file,
                                 data->data,
                                 strlen (data->data),
                                 NULL,
                                 FALSE,
                                 0,
                                 NULL,
                                 replaced_cb,
                                 data);

  xmain_loop_run (data->loop);

  xmain_loop_unref (data->loop);
  xobject_unref (data->file);
  g_free (data);
}

static void
test_replace_cancel (void)
{
  xfile_t *tmpdir, *file;
  xfile_output_stream_t *ostream;
  xfile_enumerator_t *fenum;
  xfile_info_t *info;
  xcancellable_t *cancellable;
  xchar_t *path;
  xchar_t *contents;
  xsize_t nwrote, length;
  xuint_t count;
  xerror_t *error = NULL;

  g_test_bug ("https://bugzilla.gnome.org/629301");

  path = g_dir_make_tmp ("xfile_replace_cancel_XXXXXX", &error);
  g_assert_no_error (error);
  tmpdir = xfile_new_for_path (path);
  g_free (path);

  file = xfile_get_child (tmpdir, "file");
  xfile_replace_contents (file,
                           original_data,
                           strlen (original_data),
                           NULL, FALSE, 0, NULL,
                           NULL, &error);
  g_assert_no_error (error);

  ostream = xfile_replace (file, NULL, TRUE, 0, NULL, &error);
  g_assert_no_error (error);

  xoutput_stream_write_all (G_OUTPUT_STREAM (ostream),
                             replace_data, strlen (replace_data),
                             &nwrote, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nwrote, ==, strlen (replace_data));

  /* At this point there should be two files; the original and the
   * temporary.
   */
  fenum = xfile_enumerate_children (tmpdir, NULL, 0, NULL, &error);
  g_assert_no_error (error);

  info = xfile_enumerator_next_file (fenum, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (info);
  xobject_unref (info);
  info = xfile_enumerator_next_file (fenum, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (info);
  xobject_unref (info);

  xfile_enumerator_close (fenum, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (fenum);

  /* Also test the xfile_enumerator_iterate() API */
  fenum = xfile_enumerate_children (tmpdir, NULL, 0, NULL, &error);
  g_assert_no_error (error);
  count = 0;

  while (TRUE)
    {
      xboolean_t ret = xfile_enumerator_iterate (fenum, &info, NULL, NULL, &error);
      g_assert_true (ret);
      g_assert_no_error (error);
      if (!info)
        break;
      count++;
    }
  g_assert_cmpint (count, ==, 2);

  xfile_enumerator_close (fenum, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (fenum);

  /* Now test just getting child from the xfile_enumerator_iterate() API */
  fenum = xfile_enumerate_children (tmpdir, "standard::name", 0, NULL, &error);
  g_assert_no_error (error);
  count = 0;

  while (TRUE)
    {
      xfile_t *child;
      xboolean_t ret = xfile_enumerator_iterate (fenum, NULL, &child, NULL, &error);

      g_assert_true (ret);
      g_assert_no_error (error);

      if (!child)
        break;

      g_assert_true (X_IS_FILE (child));
      count++;
    }
  g_assert_cmpint (count, ==, 2);

  xfile_enumerator_close (fenum, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (fenum);

  /* Make sure the temporary gets deleted even if we cancel. */
  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  xoutput_stream_close (G_OUTPUT_STREAM (ostream), cancellable, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);

  xobject_unref (cancellable);
  xobject_unref (ostream);

  /* Make sure that file contents wasn't actually replaced. */
  xfile_load_contents (file,
                        NULL,
                        &contents,
                        &length,
                        NULL,
                        &error);
  g_assert_no_error (error);
  g_assert_cmpstr (contents, ==, original_data);
  g_free (contents);

  xfile_delete (file, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (file);

  /* This will only succeed if the temp file was deleted. */
  xfile_delete (tmpdir, NULL, &error);
  g_assert_no_error (error);
  xobject_unref (tmpdir);
}

static void
test_replace_symlink (void)
{
#ifdef G_OS_UNIX
  xchar_t *tmpdir_path = NULL;
  xfile_t *tmpdir = NULL, *source_file = NULL, *target_file = NULL;
  xfile_output_stream_t *stream = NULL;
  const xchar_t *new_contents = "this is a test message which should be written to source and not target";
  xsize_t n_written;
  xfile_enumerator_t *enumerator = NULL;
  xfile_info_t *info = NULL;
  xchar_t *contents = NULL;
  xsize_t length = 0;
  xerror_t *local_error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2325");
  g_test_summary ("Test that XFILE_CREATE_REPLACE_DESTINATION doesn’t follow symlinks");

  /* Create a fresh, empty working directory. */
  tmpdir_path = g_dir_make_tmp ("xfile_replace_symlink_XXXXXX", &local_error);
  g_assert_no_error (local_error);
  tmpdir = xfile_new_for_path (tmpdir_path);

  g_test_message ("Using temporary directory %s", tmpdir_path);
  g_free (tmpdir_path);

  /* Create symlink `source` which points to `target`. */
  source_file = xfile_get_child (tmpdir, "source");
  target_file = xfile_get_child (tmpdir, "target");
  xfile_make_symbolic_link (source_file, "target", NULL, &local_error);
  g_assert_no_error (local_error);

  /* Ensure that `target` doesn’t exist */
  g_assert_false (xfile_query_exists (target_file, NULL));

  /* Replace the `source` symlink with a regular file using
   * %XFILE_CREATE_REPLACE_DESTINATION, which should replace it *without*
   * following the symlink */
  stream = xfile_replace (source_file, NULL, FALSE  /* no backup */,
                           XFILE_CREATE_REPLACE_DESTINATION, NULL, &local_error);
  g_assert_no_error (local_error);

  xoutput_stream_write_all (G_OUTPUT_STREAM (stream), new_contents, strlen (new_contents),
                             &n_written, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (n_written, ==, strlen (new_contents));

  xoutput_stream_close (G_OUTPUT_STREAM (stream), NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&stream);

  /* At this point, there should still only be one file: `source`. It should
   * now be a regular file. `target` should not exist. */
  enumerator = xfile_enumerate_children (tmpdir,
                                          XFILE_ATTRIBUTE_STANDARD_NAME ","
                                          XFILE_ATTRIBUTE_STANDARD_TYPE,
                                          XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
  g_assert_no_error (local_error);

  info = xfile_enumerator_next_file (enumerator, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (info);

  g_assert_cmpstr (xfile_info_get_name (info), ==, "source");
  g_assert_cmpint (xfile_info_get_file_type (info), ==, XFILE_TYPE_REGULAR);

  g_clear_object (&info);

  info = xfile_enumerator_next_file (enumerator, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_null (info);

  xfile_enumerator_close (enumerator, NULL, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&enumerator);

  /* Double-check that `target` doesn’t exist */
  g_assert_false (xfile_query_exists (target_file, NULL));

  /* Check the content of `source`. */
  xfile_load_contents (source_file,
                        NULL,
                        &contents,
                        &length,
                        NULL,
                        &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (contents, ==, new_contents);
  g_assert_cmpuint (length, ==, strlen (new_contents));
  g_free (contents);

  /* Tidy up. */
  xfile_delete (source_file, NULL, &local_error);
  g_assert_no_error (local_error);

  xfile_delete (tmpdir, NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&target_file);
  g_clear_object (&source_file);
  g_clear_object (&tmpdir);
#else  /* if !G_OS_UNIX */
  g_test_skip ("Symlink replacement tests can only be run on Unix")
#endif
}

static void
test_replace_symlink_using_etag (void)
{
#ifdef G_OS_UNIX
  xchar_t *tmpdir_path = NULL;
  xfile_t *tmpdir = NULL, *source_file = NULL, *target_file = NULL;
  xfile_output_stream_t *stream = NULL;
  const xchar_t *old_contents = "this is a test message which should be written to target and then overwritten";
  xchar_t *old_etag = NULL;
  const xchar_t *new_contents = "this is an updated message";
  xsize_t n_written;
  xchar_t *contents = NULL;
  xsize_t length = 0;
  xfile_info_t *info = NULL;
  xerror_t *local_error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2417");
  g_test_summary ("Test that ETag checks work when replacing a file through a symlink");

  /* Create a fresh, empty working directory. */
  tmpdir_path = g_dir_make_tmp ("xfile_replace_symlink_using_etag_XXXXXX", &local_error);
  g_assert_no_error (local_error);
  tmpdir = xfile_new_for_path (tmpdir_path);

  g_test_message ("Using temporary directory %s", tmpdir_path);
  g_free (tmpdir_path);

  /* Create symlink `source` which points to `target`. */
  source_file = xfile_get_child (tmpdir, "source");
  target_file = xfile_get_child (tmpdir, "target");
  xfile_make_symbolic_link (source_file, "target", NULL, &local_error);
  g_assert_no_error (local_error);

  /* Sleep for at least 1s to ensure the mtimes of `source` and `target` differ,
   * as that’s what _g_local_file_info_create_etag() uses to create the ETag,
   * and one failure mode we’re testing for is that the ETags of `source` and
   * `target` are conflated. */
  sleep (1);

  /* Create `target` with some arbitrary content. */
  stream = xfile_create (target_file, XFILE_CREATE_NONE, NULL, &local_error);
  g_assert_no_error (local_error);
  xoutput_stream_write_all (G_OUTPUT_STREAM (stream), old_contents, strlen (old_contents),
                             &n_written, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (n_written, ==, strlen (old_contents));

  xoutput_stream_close (G_OUTPUT_STREAM (stream), NULL, &local_error);
  g_assert_no_error (local_error);

  old_etag = xfile_output_stream_get_etag (stream);
  g_assert_nonnull (old_etag);
  g_assert_cmpstr (old_etag, !=, "");

  g_clear_object (&stream);

  /* Sleep again to ensure the ETag changes again. */
  sleep (1);

  /* Write out a new copy of the `target`, checking its ETag first. This should
   * replace `target` by following the symlink. */
  stream = xfile_replace (source_file, old_etag, FALSE  /* no backup */,
                           XFILE_CREATE_NONE, NULL, &local_error);
  g_assert_no_error (local_error);

  xoutput_stream_write_all (G_OUTPUT_STREAM (stream), new_contents, strlen (new_contents),
                             &n_written, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpint (n_written, ==, strlen (new_contents));

  xoutput_stream_close (G_OUTPUT_STREAM (stream), NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&stream);

  /* At this point, there should be a regular file, `target`, containing
   * @new_contents; and a symlink `source` which points to `target`. */
  g_assert_cmpint (xfile_query_file_type (source_file, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL), ==, XFILE_TYPE_SYMBOLIC_LINK);
  g_assert_cmpint (xfile_query_file_type (target_file, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL), ==, XFILE_TYPE_REGULAR);

  /* Check the content of `target`. */
  xfile_load_contents (target_file,
                        NULL,
                        &contents,
                        &length,
                        NULL,
                        &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (contents, ==, new_contents);
  g_assert_cmpuint (length, ==, strlen (new_contents));
  g_free (contents);

  /* And check its ETag value has changed. */
  info = xfile_query_info (target_file, XFILE_ATTRIBUTE_ETAG_VALUE,
                            XFILE_QUERY_INFO_NONE, NULL, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpstr (xfile_info_get_etag (info), !=, old_etag);

  g_clear_object (&info);
  g_free (old_etag);

  /* Tidy up. */
  xfile_delete (target_file, NULL, &local_error);
  g_assert_no_error (local_error);

  xfile_delete (source_file, NULL, &local_error);
  g_assert_no_error (local_error);

  xfile_delete (tmpdir, NULL, &local_error);
  g_assert_no_error (local_error);

  g_clear_object (&target_file);
  g_clear_object (&source_file);
  g_clear_object (&tmpdir);
#else  /* if !G_OS_UNIX */
  g_test_skip ("Symlink replacement tests can only be run on Unix")
#endif
}

/* FIXME: These tests have only been checked on Linux. Most of them are probably
 * applicable on Windows, too, but that has not been tested yet.
 * See https://gitlab.gnome.org/GNOME/glib/-/issues/2325 */
#ifdef __linux__

/* Different kinds of file which create_test_file() can create. */
typedef enum
{
  FILE_TEST_SETUP_TYPE_NONEXISTENT,
  FILE_TEST_SETUP_TYPE_REGULAR_EMPTY,
  FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY,
  FILE_TEST_SETUP_TYPE_DIRECTORY,
  FILE_TEST_SETUP_TYPE_SOCKET,
  FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING,
  FILE_TEST_SETUP_TYPE_SYMLINK_VALID,
} FileTestSetupType;

/* Create file `tmpdir/basename`, of type @setup_type, and chmod it to
 * @setup_mode. Return the #xfile_t representing it. Abort on any errors. */
static xfile_t *
create_test_file (xfile_t             *tmpdir,
                  const xchar_t       *basename,
                  FileTestSetupType  setup_type,
                  xuint_t              setup_mode)
{
  xfile_t *test_file = xfile_get_child (tmpdir, basename);
  xchar_t *target_basename = xstrdup_printf ("%s-target", basename);  /* for symlinks */
  xfile_t *target_file = xfile_get_child (tmpdir, target_basename);
  xerror_t *local_error = NULL;

  switch (setup_type)
    {
    case FILE_TEST_SETUP_TYPE_NONEXISTENT:
      /* Nothing to do here. */
      g_assert (setup_mode == 0);
      break;
    case FILE_TEST_SETUP_TYPE_REGULAR_EMPTY:
    case FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY:
        {
          xchar_t *contents = NULL;

          if (setup_type == FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY)
            contents = xstrdup_printf ("this is some test content in %s", basename);
          else
            contents = xstrdup ("");

          xfile_set_contents (xfile_peek_path (test_file), contents, -1, &local_error);
          g_assert_no_error (local_error);

          xfile_set_attribute_uint32 (test_file, XFILE_ATTRIBUTE_UNIX_MODE,
                                       setup_mode, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                       NULL, &local_error);
          g_assert_no_error (local_error);

          g_free (contents);
          break;
        }
    case FILE_TEST_SETUP_TYPE_DIRECTORY:
      g_assert (setup_mode == 0);

      xfile_make_directory (test_file, NULL, &local_error);
      g_assert_no_error (local_error);
      break;
    case FILE_TEST_SETUP_TYPE_SOCKET:
      g_assert_no_errno (mknod (xfile_peek_path (test_file), S_IFSOCK | setup_mode, 0));
      break;
    case FILE_TEST_SETUP_TYPE_SYMLINK_VALID:
      xfile_set_contents (xfile_peek_path (target_file), "target file", -1, &local_error);
      g_assert_no_error (local_error);
      G_GNUC_FALLTHROUGH;
    case FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING:
      /* Permissions on a symlink are not used by the kernel, so are only
       * applicable if the symlink is valid (and are applied to the target) */
      g_assert (setup_type != FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING || setup_mode == 0);

      xfile_make_symbolic_link (test_file, target_basename, NULL, &local_error);
      g_assert_no_error (local_error);

      if (setup_type == FILE_TEST_SETUP_TYPE_SYMLINK_VALID)
        {
          xfile_set_attribute_uint32 (test_file, XFILE_ATTRIBUTE_UNIX_MODE,
                                       setup_mode, XFILE_QUERY_INFO_NONE,
                                       NULL, &local_error);
          g_assert_no_error (local_error);
        }

      if (setup_type == FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING)
        {
          /* Ensure that the target doesn’t exist */
          g_assert_false (xfile_query_exists (target_file, NULL));
        }
      break;
    default:
      g_assert_not_reached ();
    }

  g_clear_object (&target_file);
  g_free (target_basename);

  return g_steal_pointer (&test_file);
}

/* Check that @test_file is of the @expected_type, has the @expected_mode, and
 * (if it’s a regular file) has the @expected_contents or (if it’s a symlink)
 * has the symlink target given by @expected_contents.
 *
 * @test_file must point to the file `tmpdir/basename`.
 *
 * Aborts on any errors or mismatches against the expectations.
 */
static void
check_test_file (xfile_t             *test_file,
                 xfile_t             *tmpdir,
                 const xchar_t       *basename,
                 FileTestSetupType  expected_type,
                 xuint_t              expected_mode,
                 const xchar_t       *expected_contents)
{
  xchar_t *target_basename = xstrdup_printf ("%s-target", basename);  /* for symlinks */
  xfile_t *target_file = xfile_get_child (tmpdir, target_basename);
  xfile_type_t test_file_type = xfile_query_file_type (test_file, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
  xfile_info_t *info = NULL;
  xerror_t *local_error = NULL;

  switch (expected_type)
    {
    case FILE_TEST_SETUP_TYPE_NONEXISTENT:
      g_assert (expected_mode == 0);
      g_assert (expected_contents == NULL);

      g_assert_false (xfile_query_exists (test_file, NULL));
      g_assert_cmpint (test_file_type, ==, XFILE_TYPE_UNKNOWN);
      break;
    case FILE_TEST_SETUP_TYPE_REGULAR_EMPTY:
    case FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY:
      g_assert (expected_type != FILE_TEST_SETUP_TYPE_REGULAR_EMPTY || expected_contents == NULL);
      g_assert (expected_type != FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY || expected_contents != NULL);

      g_assert_cmpint (test_file_type, ==, XFILE_TYPE_REGULAR);

      info = xfile_query_info (test_file,
                                XFILE_ATTRIBUTE_STANDARD_SIZE ","
                                XFILE_ATTRIBUTE_UNIX_MODE,
                                XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
      g_assert_no_error (local_error);

      if (expected_type == FILE_TEST_SETUP_TYPE_REGULAR_EMPTY)
        g_assert_cmpint (xfile_info_get_size (info), ==, 0);
      else
        g_assert_cmpint (xfile_info_get_size (info), >, 0);

      if (expected_contents != NULL)
        {
          xchar_t *contents = NULL;
          xsize_t length = 0;

          xfile_get_contents (xfile_peek_path (test_file), &contents, &length, &local_error);
          g_assert_no_error (local_error);

          g_assert_cmpstr (contents, ==, expected_contents);
          g_free (contents);
        }

      g_assert_cmpuint (xfile_info_get_attribute_uint32 (info, XFILE_ATTRIBUTE_UNIX_MODE) & 0777, ==, expected_mode);

      break;
    case FILE_TEST_SETUP_TYPE_DIRECTORY:
      g_assert (expected_mode == 0);
      g_assert (expected_contents == NULL);

      g_assert_cmpint (test_file_type, ==, XFILE_TYPE_DIRECTORY);
      break;
    case FILE_TEST_SETUP_TYPE_SOCKET:
      g_assert (expected_contents == NULL);

      g_assert_cmpint (test_file_type, ==, XFILE_TYPE_SPECIAL);

      info = xfile_query_info (test_file,
                                XFILE_ATTRIBUTE_UNIX_MODE,
                                XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
      g_assert_no_error (local_error);

      g_assert_cmpuint (xfile_info_get_attribute_uint32 (info, XFILE_ATTRIBUTE_UNIX_MODE) & 0777, ==, expected_mode);
      break;
    case FILE_TEST_SETUP_TYPE_SYMLINK_VALID:
    case FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING:
        {
          xfile_t *symlink_target_file = NULL;

          /* Permissions on a symlink are not used by the kernel, so are only
           * applicable if the symlink is valid (and are applied to the target) */
          g_assert (expected_type != FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING || expected_mode == 0);
          g_assert (expected_contents != NULL);

          g_assert_cmpint (test_file_type, ==, XFILE_TYPE_SYMBOLIC_LINK);

          info = xfile_query_info (test_file,
                                    XFILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                                    XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
          g_assert_no_error (local_error);

          g_assert_cmpstr (xfile_info_get_symlink_target (info), ==, expected_contents);

          symlink_target_file = xfile_get_child (tmpdir, xfile_info_get_symlink_target (info));
          if (expected_type == FILE_TEST_SETUP_TYPE_SYMLINK_VALID)
            g_assert_true (xfile_query_exists (symlink_target_file, NULL));
          else
            g_assert_false (xfile_query_exists (symlink_target_file, NULL));

          if (expected_type == FILE_TEST_SETUP_TYPE_SYMLINK_VALID)
            {
              xfile_info_t *target_info = NULL;

              /* Need to re-query the info so we follow symlinks */
              target_info = xfile_query_info (test_file,
                                               XFILE_ATTRIBUTE_UNIX_MODE,
                                               XFILE_QUERY_INFO_NONE, NULL, &local_error);
              g_assert_no_error (local_error);

              g_assert_cmpuint (xfile_info_get_attribute_uint32 (target_info, XFILE_ATTRIBUTE_UNIX_MODE) & 0777, ==, expected_mode);

              g_clear_object (&target_info);
            }

          g_clear_object (&symlink_target_file);
          break;
        }
    default:
      g_assert_not_reached ();
    }

  g_clear_object (&info);
  g_clear_object (&target_file);
  g_free (target_basename);
}

#endif  /* __linux__ */

/* A big test for xfile_replace() and xfile_replace_readwrite(). The
 * @test_data is a boolean: %TRUE to test xfile_replace_readwrite(), %FALSE to
 * test xfile_replace(). The test setup and checks are identical for both
 * functions; in the case of testing xfile_replace_readwrite(), only the output
 * stream side of the returned #xio_stream_t is tested. i.e. We test the write
 * behaviour of both functions is identical.
 *
 * This is intended to test all static behaviour of the function: for each test
 * scenario, a temporary directory is set up with a source file (and maybe some
 * other files) in a set configuration, xfile_replace{,_readwrite}() is called,
 * and the final state of the directory is checked.
 *
 * This test does not check dynamic behaviour or race conditions. For example,
 * it does not test what happens if the source file is deleted from another
 * process half-way through a call to xfile_replace().
 */
static void
test_replace (xconstpointer test_data)
{
#ifdef __linux__
  xboolean_t read_write = GPOINTER_TO_UINT (test_data);
  const xchar_t *new_contents = "this is a new test message which should be written to source";
  const xchar_t *original_source_contents = "this is some test content in source";
  const xchar_t *original_backup_contents = "this is some test content in source~";
  mode_t current_umask = umask (0);
  xuint32_t default_public_mode = 0666 & ~current_umask;
  xuint32_t default_private_mode = 0600;

  const struct
    {
      /* Arguments to pass to xfile_replace(). */
      xboolean_t replace_make_backup;
      xfile_create_flags_t replace_flags;
      const xchar_t *replace_etag;  /* (nullable) */

      /* File system setup. */
      FileTestSetupType setup_source_type;
      xuint_t setup_source_mode;
      FileTestSetupType setup_backup_type;
      xuint_t setup_backup_mode;

      /* Expected results. */
      xboolean_t expected_success;
      xquark expected_error_domain;
      xint_t expected_error_code;

      /* Expected final file system state. */
      xuint_t expected_n_files;
      FileTestSetupType expected_source_type;
      xuint_t expected_source_mode;
      const xchar_t *expected_source_contents;  /* content for a regular file, or target for a symlink; NULL otherwise */
      FileTestSetupType expected_backup_type;
      xuint_t expected_backup_mode;
      const xchar_t *expected_backup_contents;  /* content for a regular file, or target for a symlink; NULL otherwise */
    }
  tests[] =
    {
      /* replace_make_backup == FALSE, replace_flags == NONE, replace_etag == NULL,
       * all the different values of setup_source_type, mostly with a backup
       * file created to check it’s not modified */
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_etag set to an invalid value, with setup_source_type as a
       * regular non-empty file; replacement should fail */
      {
        FALSE, XFILE_CREATE_NONE, "incorrect etag",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_WRONG_ETAG,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_make_backup == TRUE, replace_flags == NONE, replace_etag == NULL,
       * all the different values of setup_source_type, with a backup
       * file created to check it’s either replaced or the operation fails */
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode, NULL,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        /* The final situation here is a bit odd; the backup file is a bit
         * pointless as the original source file was a dangling symlink.
         * Theoretically the backup file should be that symlink, pointing to
         * `source-target`, and hence no longer dangling, as that file has now
         * been created as the new source content, since REPLACE_DESTINATION was
         * not specified. However, the code instead creates an empty regular
         * file as the backup. FIXME: This seems acceptable for now, but not
         * entirely ideal and would be good to fix at some point. */
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, 0777 & ~current_umask, NULL,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        /* FIXME: The permissions for the backup file are just the default umask,
         * but should probably be the same as the permissions for the source
         * file (`default_public_mode`). This probably arises from the fact that
         * symlinks don’t have permissions. */
        3, FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, 0777 & ~current_umask, "target file",
      },

      /* replace_make_backup == TRUE, replace_flags == NONE, replace_etag == NULL,
       * setup_source_type is a regular file, with a backup file of every type
       * created to check it’s either replaced or the operation fails */
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FALSE, G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        TRUE, 0, 0,
        /* the third file is `source~-target`, the original target of the old
         * backup symlink */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },

      /* replace_make_backup == FALSE, replace_flags == REPLACE_DESTINATION,
       * replace_etag == NULL, all the different values of setup_source_type,
       * mostly with a backup file created to check it’s not modified */
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        /* the third file is `source-target`, the original target of the old
         * source file */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_flags == REPLACE_DESTINATION, replace_etag set to an invalid
       * value, with setup_source_type as a regular non-empty file; replacement
       * should fail */
      {
        FALSE, XFILE_CREATE_REPLACE_DESTINATION, "incorrect etag",
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_WRONG_ETAG,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },

      /* replace_make_backup == TRUE, replace_flags == REPLACE_DESTINATION,
       * replace_etag == NULL, all the different values of setup_source_type,
       * with a backup file created to check it’s either replaced or the
       * operation fails */
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode, NULL,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
        2, FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FALSE, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
        2, FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_backup_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0, "source-target",
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        /* the third file is `source-target`, the original target of the old
         * source file */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode, "source-target",
      },

      /* replace_make_backup == TRUE, replace_flags == REPLACE_DESTINATION,
       * replace_etag == NULL, setup_source_type is a regular file, with a
       * backup file of every type created to check it’s either replaced or the
       * operation fails */
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0,
        FALSE, G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
        FILE_TEST_SETUP_TYPE_DIRECTORY, 0, NULL,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SOCKET, default_public_mode,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_DANGLING, 0,
        TRUE, 0, 0,
        2, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },
      {
        TRUE, XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_SYMLINK_VALID, default_public_mode,
        TRUE, 0, 0,
        /* the third file is `source~-target`, the original target of the old
         * backup symlink */
        3, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, original_source_contents,
      },

      /* several different setups with replace_flags == PRIVATE */
      {
        FALSE, XFILE_CREATE_PRIVATE, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_private_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, XFILE_CREATE_PRIVATE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        /* the file isn’t being replaced, so it should keep its existing permissions */
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, XFILE_CREATE_PRIVATE | XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_private_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
      {
        FALSE, XFILE_CREATE_PRIVATE | XFILE_CREATE_REPLACE_DESTINATION, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_public_mode,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        TRUE, 0, 0,
        1, FILE_TEST_SETUP_TYPE_REGULAR_NONEMPTY, default_private_mode, new_contents,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },

      /* make the initial source file unreadable, so the replace operation
       * should fail */
      {
        FALSE, XFILE_CREATE_NONE, NULL,
        FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, 0  /* most restrictive permissions */,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0,
        FALSE, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
        1, FILE_TEST_SETUP_TYPE_REGULAR_EMPTY, 0, NULL,
        FILE_TEST_SETUP_TYPE_NONEXISTENT, 0, NULL,
      },
    };
  xsize_t i;

  g_test_summary ("Test various situations for xfile_replace()");

  /* Reset the umask after querying it above. There’s no way to query it without
   * changing it. */
  umask (current_umask);
  g_test_message ("Current umask: %u", current_umask);

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      xchar_t *tmpdir_path = NULL;
      xfile_t *tmpdir = NULL, *source_file = NULL, *backup_file = NULL;
      xfile_output_stream_t *output_stream = NULL;
      xfile_io_stream_t *io_stream = NULL;
      xfile_enumerator_t *enumerator = NULL;
      xfile_info_t *info = NULL;
      xuint_t n_files;
      xerror_t *local_error = NULL;

      /* Create a fresh, empty working directory. */
      tmpdir_path = g_dir_make_tmp ("xfile_replace_XXXXXX", &local_error);
      g_assert_no_error (local_error);
      tmpdir = xfile_new_for_path (tmpdir_path);

      g_test_message ("Test %" G_GSIZE_FORMAT ", using temporary directory %s", i, tmpdir_path);
      g_free (tmpdir_path);

      /* Set up the test directory. */
      source_file = create_test_file (tmpdir, "source", tests[i].setup_source_type, tests[i].setup_source_mode);
      backup_file = create_test_file (tmpdir, "source~", tests[i].setup_backup_type, tests[i].setup_backup_mode);

      /* Replace the source file. Check the error state only after finishing
       * writing, as the replace operation is split across xfile_replace() and
       * xoutput_stream_close(). */
      if (read_write)
        io_stream = xfile_replace_readwrite (source_file,
                                              tests[i].replace_etag,
                                              tests[i].replace_make_backup,
                                              tests[i].replace_flags,
                                              NULL,
                                              &local_error);
      else
        output_stream = xfile_replace (source_file,
                                        tests[i].replace_etag,
                                        tests[i].replace_make_backup,
                                        tests[i].replace_flags,
                                        NULL,
                                        &local_error);

      if (tests[i].expected_success)
        {
          g_assert_no_error (local_error);
          if (read_write)
            g_assert_nonnull (io_stream);
          else
            g_assert_nonnull (output_stream);
        }

      /* Write new content to it. */
      if (io_stream != NULL)
        {
          xoutput_stream_t *io_output_stream = g_io_stream_get_output_stream (XIO_STREAM (io_stream));
          xsize_t n_written;

          xoutput_stream_write_all (G_OUTPUT_STREAM (io_output_stream), new_contents, strlen (new_contents),
                                     &n_written, NULL, &local_error);

          if (tests[i].expected_success)
            {
              g_assert_no_error (local_error);
              g_assert_cmpint (n_written, ==, strlen (new_contents));
            }

          g_io_stream_close (XIO_STREAM (io_stream), NULL, (local_error == NULL) ? &local_error : NULL);

          if (tests[i].expected_success)
            g_assert_no_error (local_error);
        }
      else if (output_stream != NULL)
        {
          xsize_t n_written;

          xoutput_stream_write_all (G_OUTPUT_STREAM (output_stream), new_contents, strlen (new_contents),
                                     &n_written, NULL, &local_error);

          if (tests[i].expected_success)
            {
              g_assert_no_error (local_error);
              g_assert_cmpint (n_written, ==, strlen (new_contents));
            }

          xoutput_stream_close (G_OUTPUT_STREAM (output_stream), NULL, (local_error == NULL) ? &local_error : NULL);

          if (tests[i].expected_success)
            g_assert_no_error (local_error);
        }

      if (tests[i].expected_success)
        g_assert_no_error (local_error);
      else
        g_assert_error (local_error, tests[i].expected_error_domain, tests[i].expected_error_code);

      g_clear_error (&local_error);
      g_clear_object (&io_stream);
      g_clear_object (&output_stream);

      /* Verify the final state of the directory. */
      enumerator = xfile_enumerate_children (tmpdir,
                                              XFILE_ATTRIBUTE_STANDARD_NAME,
                                              XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &local_error);
      g_assert_no_error (local_error);

      n_files = 0;
      do
        {
          xfile_enumerator_iterate (enumerator, &info, NULL, NULL, &local_error);
          g_assert_no_error (local_error);

          if (info != NULL)
            n_files++;
        }
      while (info != NULL);

      g_clear_object (&enumerator);

      g_assert_cmpuint (n_files, ==, tests[i].expected_n_files);

      check_test_file (source_file, tmpdir, "source", tests[i].expected_source_type, tests[i].expected_source_mode, tests[i].expected_source_contents);
      check_test_file (backup_file, tmpdir, "source~", tests[i].expected_backup_type, tests[i].expected_backup_mode, tests[i].expected_backup_contents);

      /* Tidy up. Ignore failure apart from when deleting the directory, which
       * should be empty. */
      xfile_delete (source_file, NULL, NULL);
      xfile_delete (backup_file, NULL, NULL);

      /* Other files which are occasionally generated by the tests. */
        {
          xfile_t *backup_target_file = xfile_get_child (tmpdir, "source~-target");
          xfile_delete (backup_target_file, NULL, NULL);
          g_clear_object (&backup_target_file);
        }
        {
          xfile_t *backup_target_file = xfile_get_child (tmpdir, "source-target");
          xfile_delete (backup_target_file, NULL, NULL);
          g_clear_object (&backup_target_file);
        }

      xfile_delete (tmpdir, NULL, &local_error);
      g_assert_no_error (local_error);

      g_clear_object (&backup_file);
      g_clear_object (&source_file);
      g_clear_object (&tmpdir);
    }
#else  /* if !__linux__ */
  g_test_skip ("File replacement tests can only be run on Linux");
#endif
}

static void
on_file_deleted (xobject_t      *object,
		 xasync_result_t *result,
		 xpointer_t      user_data)
{
  xerror_t *local_error = NULL;
  xmain_loop_t *loop = user_data;

  (void) xfile_delete_finish ((xfile_t*)object, result, &local_error);
  g_assert_no_error (local_error);

  xmain_loop_quit (loop);
}

static void
test_async_delete (void)
{
  xfile_t *file;
  xfile_io_stream_t *iostream;
  xerror_t *local_error = NULL;
  xerror_t **error = &local_error;
  xmain_loop_t *loop;

  file = xfile_new_tmp ("xfile_delete_XXXXXX",
			 &iostream, error);
  g_assert_no_error (local_error);
  xobject_unref (iostream);

  g_assert_true (xfile_query_exists (file, NULL));

  loop = xmain_loop_new (NULL, TRUE);

  xfile_delete_async (file, G_PRIORITY_DEFAULT, NULL, on_file_deleted, loop);

  xmain_loop_run (loop);

  g_assert_false (xfile_query_exists (file, NULL));

  xmain_loop_unref (loop);
  xobject_unref (file);
}

static void
test_copy_preserve_mode (void)
{
#ifdef G_OS_UNIX
  mode_t current_umask = umask (0);
  const struct
    {
      xuint32_t source_mode;
      xuint32_t expected_destination_mode;
      xboolean_t create_destination_before_copy;
      xfile_copy_flags_t copy_flags;
    }
  vectors[] =
    {
      /* Overwriting the destination file should copy the permissions from the
       * source file, even if %XFILE_COPY_ALL_METADATA is set: */
      { 0600, 0600, TRUE, XFILE_COPY_OVERWRITE | XFILE_COPY_NOFOLLOW_SYMLINKS | XFILE_COPY_ALL_METADATA },
      { 0600, 0600, TRUE, XFILE_COPY_OVERWRITE | XFILE_COPY_NOFOLLOW_SYMLINKS },
      /* The same behaviour should hold if the destination file is not being
       * overwritten because it doesn’t already exist: */
      { 0600, 0600, FALSE, XFILE_COPY_NOFOLLOW_SYMLINKS | XFILE_COPY_ALL_METADATA },
      { 0600, 0600, FALSE, XFILE_COPY_NOFOLLOW_SYMLINKS },
      /* Anything with %XFILE_COPY_TARGET_DEFAULT_PERMS should use the current
       * umask for the destination file: */
      { 0600, 0666 & ~current_umask, TRUE, XFILE_COPY_TARGET_DEFAULT_PERMS | XFILE_COPY_OVERWRITE | XFILE_COPY_NOFOLLOW_SYMLINKS | XFILE_COPY_ALL_METADATA },
      { 0600, 0666 & ~current_umask, TRUE, XFILE_COPY_TARGET_DEFAULT_PERMS | XFILE_COPY_OVERWRITE | XFILE_COPY_NOFOLLOW_SYMLINKS },
      { 0600, 0666 & ~current_umask, FALSE, XFILE_COPY_TARGET_DEFAULT_PERMS | XFILE_COPY_NOFOLLOW_SYMLINKS | XFILE_COPY_ALL_METADATA },
      { 0600, 0666 & ~current_umask, FALSE, XFILE_COPY_TARGET_DEFAULT_PERMS | XFILE_COPY_NOFOLLOW_SYMLINKS },
    };
  xsize_t i;

  /* Reset the umask after querying it above. There’s no way to query it without
   * changing it. */
  umask (current_umask);
  g_test_message ("Current umask: %u", current_umask);

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      xfile_t *tmpfile;
      xfile_t *dest_tmpfile;
      xfile_info_t *dest_info;
      xfile_io_stream_t *iostream;
      xerror_t *local_error = NULL;
      xuint32_t romode = vectors[i].source_mode;
      xuint32_t dest_mode;

      g_test_message ("Vector %" G_GSIZE_FORMAT, i);

      tmpfile = xfile_new_tmp ("tmp-copy-preserve-modeXXXXXX",
                                &iostream, &local_error);
      g_assert_no_error (local_error);
      g_io_stream_close ((xio_stream_t*)iostream, NULL, &local_error);
      g_assert_no_error (local_error);
      g_clear_object (&iostream);

      xfile_set_attribute (tmpfile, XFILE_ATTRIBUTE_UNIX_MODE, XFILE_ATTRIBUTE_TYPE_UINT32,
                            &romode, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            NULL, &local_error);
      g_assert_no_error (local_error);

      dest_tmpfile = xfile_new_tmp ("tmp-copy-preserve-modeXXXXXX",
                                     &iostream, &local_error);
      g_assert_no_error (local_error);
      g_io_stream_close ((xio_stream_t*)iostream, NULL, &local_error);
      g_assert_no_error (local_error);
      g_clear_object (&iostream);

      if (!vectors[i].create_destination_before_copy)
        {
          xfile_delete (dest_tmpfile, NULL, &local_error);
          g_assert_no_error (local_error);
        }

      xfile_copy (tmpfile, dest_tmpfile, vectors[i].copy_flags,
                   NULL, NULL, NULL, &local_error);
      g_assert_no_error (local_error);

      dest_info = xfile_query_info (dest_tmpfile, XFILE_ATTRIBUTE_UNIX_MODE, XFILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     NULL, &local_error);
      g_assert_no_error (local_error);

      dest_mode = xfile_info_get_attribute_uint32 (dest_info, XFILE_ATTRIBUTE_UNIX_MODE);

      g_assert_cmpint (dest_mode & ~S_IFMT, ==, vectors[i].expected_destination_mode);
      g_assert_cmpint (dest_mode & S_IFMT, ==, S_IFREG);

      (void) xfile_delete (tmpfile, NULL, NULL);
      (void) xfile_delete (dest_tmpfile, NULL, NULL);

      g_clear_object (&tmpfile);
      g_clear_object (&dest_tmpfile);
      g_clear_object (&dest_info);
    }
#else  /* if !G_OS_UNIX */
  g_test_skip ("File permissions tests can only be run on Unix")
#endif
}

static xchar_t *
splice_to_string (xinput_stream_t   *stream,
                  xerror_t        **error)
{
  xmemory_output_stream_t *buffer = NULL;
  char *ret = NULL;

  buffer = (xmemory_output_stream_t*)g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  if (xoutput_stream_splice ((xoutput_stream_t*)buffer, stream, 0, NULL, error) < 0)
    goto out;

  if (!xoutput_stream_write ((xoutput_stream_t*)buffer, "\0", 1, NULL, error))
    goto out;

  if (!xoutput_stream_close ((xoutput_stream_t*)buffer, NULL, error))
    goto out;

  ret = g_memory_output_stream_steal_data (buffer);
 out:
  g_clear_object (&buffer);
  return ret;
}

static xboolean_t
get_size_from_du (const xchar_t *path, xuint64_t *size)
{
  xsubprocess_t *du;
  xboolean_t ok;
  xchar_t *result;
  xchar_t *endptr;
  xerror_t *error = NULL;
  xchar_t *du_path = NULL;

  /* If we can’t find du, don’t try and run the test. */
  du_path = g_find_program_in_path ("du");
  if (du_path == NULL)
    return FALSE;
  g_free (du_path);

  du = xsubprocess_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE,
                         &error,
                         "du", "--bytes", "-s", path, NULL);
  g_assert_no_error (error);

  result = splice_to_string (xsubprocess_get_stdout_pipe (du), &error);
  g_assert_no_error (error);

  *size = g_ascii_strtoll (result, &endptr, 10);

  xsubprocess_wait (du, NULL, &error);
  g_assert_no_error (error);

  ok = xsubprocess_get_successful (du);

  xobject_unref (du);
  g_free (result);

  return ok;
}

static void
test_measure (void)
{
  xfile_t *file;
  xuint64_t size;
  xuint64_t num_bytes;
  xuint64_t num_dirs;
  xuint64_t num_files;
  xerror_t *error = NULL;
  xboolean_t ok;
  xchar_t *path;

  path = g_test_build_filename (G_TEST_DIST, "desktop-files", NULL);
  file = xfile_new_for_path (path);

  if (!get_size_from_du (path, &size))
    {
      g_test_message ("du not found or fail to run, skipping byte measurement");
      size = 0;
    }

  ok = xfile_measure_disk_usage (file,
                                  XFILE_MEASURE_APPARENT_SIZE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &num_bytes,
                                  &num_dirs,
                                  &num_files,
                                  &error);
  g_assert_true (ok);
  g_assert_no_error (error);

  if (size > 0)
    g_assert_cmpuint (num_bytes, ==, size);
  g_assert_cmpuint (num_dirs, ==, 6);
  g_assert_cmpuint (num_files, ==, 32);

  xobject_unref (file);
  g_free (path);
}

typedef struct {
  xuint64_t expected_bytes;
  xuint64_t expected_dirs;
  xuint64_t expected_files;
  xint_t progress_count;
  xuint64_t progress_bytes;
  xuint64_t progress_dirs;
  xuint64_t progress_files;
} MeasureData;

static void
measure_progress (xboolean_t reporting,
                  xuint64_t  current_size,
                  xuint64_t  num_dirs,
                  xuint64_t  num_files,
                  xpointer_t user_data)
{
  MeasureData *data = user_data;

  data->progress_count += 1;

  g_assert_cmpuint (current_size, >=, data->progress_bytes);
  g_assert_cmpuint (num_dirs, >=, data->progress_dirs);
  g_assert_cmpuint (num_files, >=, data->progress_files);

  data->progress_bytes = current_size;
  data->progress_dirs = num_dirs;
  data->progress_files = num_files;
}

static void
measure_done (xobject_t      *source,
              xasync_result_t *res,
              xpointer_t      user_data)
{
  MeasureData *data = user_data;
  xuint64_t num_bytes, num_dirs, num_files;
  xerror_t *error = NULL;
  xboolean_t ok;

  ok = xfile_measure_disk_usage_finish (XFILE (source), res, &num_bytes, &num_dirs, &num_files, &error);
  g_assert_true (ok);
  g_assert_no_error (error);

  if (data->expected_bytes > 0)
    g_assert_cmpuint (data->expected_bytes, ==, num_bytes);
  g_assert_cmpuint (data->expected_dirs, ==, num_dirs);
  g_assert_cmpuint (data->expected_files, ==, num_files);

  g_assert_cmpuint (data->progress_count, >, 0);
  g_assert_cmpuint (num_bytes, >=, data->progress_bytes);
  g_assert_cmpuint (num_dirs, >=, data->progress_dirs);
  g_assert_cmpuint (num_files, >=, data->progress_files);

  g_free (data);
  xobject_unref (source);
}

static void
test_measure_async (void)
{
  xchar_t *path;
  xfile_t *file;
  MeasureData *data;

  data = g_new (MeasureData, 1);

  data->progress_count = 0;
  data->progress_bytes = 0;
  data->progress_files = 0;
  data->progress_dirs = 0;

  path = g_test_build_filename (G_TEST_DIST, "desktop-files", NULL);
  file = xfile_new_for_path (path);

  if (!get_size_from_du (path, &data->expected_bytes))
    {
      g_test_message ("du not found or fail to run, skipping byte measurement");
      data->expected_bytes = 0;
    }

  g_free (path);

  data->expected_dirs = 6;
  data->expected_files = 32;

  xfile_measure_disk_usage_async (file,
                                   XFILE_MEASURE_APPARENT_SIZE,
                                   0, NULL,
                                   measure_progress, data,
                                   measure_done, data);
}

static void
test_load_bytes (void)
{
  xchar_t filename[] = "xfile_load_bytes_XXXXXX";
  xerror_t *error = NULL;
  xbytes_t *bytes;
  xfile_t *file;
  int len;
  int fd;
  int ret;

  fd = g_mkstemp (filename);
  g_assert_cmpint (fd, !=, -1);
  len = strlen ("test_load_bytes");
  ret = write (fd, "test_load_bytes", len);
  g_assert_cmpint (ret, ==, len);
  close (fd);

  file = xfile_new_for_path (filename);
  bytes = xfile_load_bytes (file, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (bytes);
  g_assert_cmpint (len, ==, xbytes_get_size (bytes));
  g_assert_cmpstr ("test_load_bytes", ==, (xchar_t *)xbytes_get_data (bytes, NULL));

  xfile_delete (file, NULL, NULL);

  xbytes_unref (bytes);
  xobject_unref (file);
}

typedef struct
{
  xmain_loop_t *main_loop;
  xfile_t *file;
  xbytes_t *bytes;
} LoadBytesAsyncData;

static void
test_load_bytes_cb (xobject_t      *object,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  xfile_t *file = XFILE (object);
  LoadBytesAsyncData *data = user_data;
  xerror_t *error = NULL;

  data->bytes = xfile_load_bytes_finish (file, result, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (data->bytes);

  xmain_loop_quit (data->main_loop);
}

static void
test_load_bytes_async (void)
{
  LoadBytesAsyncData data = { 0 };
  xchar_t filename[] = "xfile_load_bytes_XXXXXX";
  int len;
  int fd;
  int ret;

  fd = g_mkstemp (filename);
  g_assert_cmpint (fd, !=, -1);
  len = strlen ("test_load_bytes_async");
  ret = write (fd, "test_load_bytes_async", len);
  g_assert_cmpint (ret, ==, len);
  close (fd);

  data.main_loop = xmain_loop_new (NULL, FALSE);
  data.file = xfile_new_for_path (filename);

  xfile_load_bytes_async (data.file, NULL, test_load_bytes_cb, &data);
  xmain_loop_run (data.main_loop);

  g_assert_cmpint (len, ==, xbytes_get_size (data.bytes));
  g_assert_cmpstr ("test_load_bytes_async", ==, (xchar_t *)xbytes_get_data (data.bytes, NULL));

  xfile_delete (data.file, NULL, NULL);
  xobject_unref (data.file);
  xbytes_unref (data.bytes);
  xmain_loop_unref (data.main_loop);
}

static void
test_writev_helper (xoutput_vector_t *vectors,
                    xsize_t          n_vectors,
                    xboolean_t       use_bytes_written,
                    const xuint8_t  *expected_contents,
                    xsize_t          expected_length)
{
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xsize_t bytes_written = 0;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  res = xoutput_stream_writev_all (ostream, vectors, n_vectors, use_bytes_written ? &bytes_written : NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  if (use_bytes_written)
    g_assert_cmpuint (bytes_written, ==, expected_length);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, expected_contents, expected_length);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

/* Test that writev() on local file output streams works on a non-empty vector */
static void
test_writev (void)
{
  xoutput_vector_t vectors[3];
  const xuint8_t buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5 + 12;
  vectors[2].size = 3;

  test_writev_helper (vectors, G_N_ELEMENTS (vectors), TRUE, buffer, sizeof buffer);
}

/* Test that writev() on local file output streams works on a non-empty vector without returning bytes_written */
static void
test_writev_no_bytes_written (void)
{
  xoutput_vector_t vectors[3];
  const xuint8_t buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5 + 12;
  vectors[2].size = 3;

  test_writev_helper (vectors, G_N_ELEMENTS (vectors), FALSE, buffer, sizeof buffer);
}

/* Test that writev() on local file output streams works on 0 vectors */
static void
test_writev_no_vectors (void)
{
  test_writev_helper (NULL, 0, TRUE, NULL, 0);
}

/* Test that writev() on local file output streams works on empty vectors */
static void
test_writev_empty_vectors (void)
{
  xoutput_vector_t vectors[3];

  vectors[0].buffer = NULL;
  vectors[0].size = 0;
  vectors[1].buffer = NULL;
  vectors[1].size = 0;
  vectors[2].buffer = NULL;
  vectors[2].size = 0;

  test_writev_helper (vectors, G_N_ELEMENTS (vectors), TRUE, NULL, 0);
}

/* Test that writev() fails if the sum of sizes in the vector is too big */
static void
test_writev_too_big_vectors (void)
{
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xsize_t bytes_written = 0;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;
  xoutput_vector_t vectors[3];

  vectors[0].buffer = (void*) 1;
  vectors[0].size = G_MAXSIZE / 2;

  vectors[1].buffer = (void*) 1;
  vectors[1].size = G_MAXSIZE / 2;

  vectors[2].buffer = (void*) 1;
  vectors[2].size = G_MAXSIZE / 2;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  res = xoutput_stream_writev_all (ostream, vectors, G_N_ELEMENTS (vectors), &bytes_written, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_cmpuint (bytes_written, ==, 0);
  g_assert_false (res);
  g_clear_error (&error);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, NULL, 0);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

typedef struct
{
  xsize_t bytes_written;
  xoutput_vector_t *vectors;
  xsize_t n_vectors;
  xerror_t *error;
  xboolean_t done;
} WritevAsyncData;

static void
test_writev_async_cb (xobject_t      *object,
                      xasync_result_t *result,
                      xpointer_t      user_data)
{
  xoutput_stream_t *ostream = G_OUTPUT_STREAM (object);
  WritevAsyncData *data = user_data;
  xerror_t *error = NULL;
  xsize_t bytes_written;
  xboolean_t res;

  res = xoutput_stream_writev_finish (ostream, result, &bytes_written, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  data->bytes_written += bytes_written;

  /* skip vectors that have been written in full */
  while (data->n_vectors > 0 && bytes_written >= data->vectors[0].size)
    {
      bytes_written -= data->vectors[0].size;
      ++data->vectors;
      --data->n_vectors;
    }
  /* skip partially written vector data */
  if (bytes_written > 0 && data->n_vectors > 0)
    {
      data->vectors[0].size -= bytes_written;
      data->vectors[0].buffer = ((xuint8_t *) data->vectors[0].buffer) + bytes_written;
    }

  if (data->n_vectors > 0)
    xoutput_stream_writev_async (ostream, data->vectors, data->n_vectors, 0, NULL, test_writev_async_cb, &data);
}

/* Test that writev_async() on local file output streams works on a non-empty vector */
static void
test_writev_async (void)
{
  WritevAsyncData data = { 0 };
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_vector_t vectors[3];
  const xuint8_t buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5  + 12;
  vectors[2].size = 3;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  data.vectors = vectors;
  data.n_vectors = G_N_ELEMENTS (vectors);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  xoutput_stream_writev_async (ostream, data.vectors, data.n_vectors, 0, NULL, test_writev_async_cb, &data);

  while (data.n_vectors > 0)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, sizeof buffer);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, buffer, sizeof buffer);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

static void
test_writev_all_cb (xobject_t      *object,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  xoutput_stream_t *ostream = G_OUTPUT_STREAM (object);
  WritevAsyncData *data = user_data;

  xoutput_stream_writev_all_finish (ostream, result, &data->bytes_written, &data->error);
  data->done = TRUE;
}

/* Test that writev_async_all() on local file output streams works on a non-empty vector */
static void
test_writev_async_all (void)
{
  WritevAsyncData data = { 0 };
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_stream_t *ostream;
  xoutput_vector_t vectors[3];
  const xuint8_t buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};
  xerror_t *error = NULL;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5  + 12;
  vectors[2].size = 3;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  xoutput_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, sizeof buffer);
  g_assert_no_error (data.error);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpmem (contents, length, buffer, sizeof buffer);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

/* Test that writev_async_all() on local file output streams handles cancellation correctly */
static void
test_writev_async_all_cancellation (void)
{
  WritevAsyncData data = { 0 };
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_vector_t vectors[3];
  const xuint8_t buffer[] = {1, 2, 3, 4, 5,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                           1, 2, 3};
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;
  xcancellable_t *cancellable;

  vectors[0].buffer = buffer;
  vectors[0].size = 5;

  vectors[1].buffer = buffer + 5;
  vectors[1].size = 12;

  vectors[2].buffer = buffer + 5  + 12;
  vectors[2].size = 3;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);

  xoutput_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, cancellable, test_writev_all_cb, &data);

  while (!data.done)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&data.error);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
  xobject_unref (cancellable);
}

/* Test that writev_async_all() with empty vectors is handled correctly */
static void
test_writev_async_all_empty_vectors (void)
{
  WritevAsyncData data = { 0 };
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_vector_t vectors[3];
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;

  vectors[0].buffer = NULL;
  vectors[0].size = 0;

  vectors[1].buffer = NULL;
  vectors[1].size = 0;

  vectors[2].buffer = NULL;
  vectors[2].size = 0;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  xoutput_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_no_error (data.error);
  g_clear_error (&data.error);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

/* Test that writev_async_all() with no vectors is handled correctly */
static void
test_writev_async_all_no_vectors (void)
{
  WritevAsyncData data = { 0 };
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  xoutput_stream_writev_all_async (ostream, NULL, 0, 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_no_error (data.error);
  g_clear_error (&data.error);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

/* Test that writev_async_all() with too big vectors is handled correctly */
static void
test_writev_async_all_too_big_vectors (void)
{
  WritevAsyncData data = { 0 };
  xfile_t *file;
  xfile_io_stream_t *iostream = NULL;
  xoutput_vector_t vectors[3];
  xoutput_stream_t *ostream;
  xerror_t *error = NULL;
  xboolean_t res;
  xuint8_t *contents;
  xsize_t length;

  vectors[0].buffer = (void*) 1;
  vectors[0].size = G_MAXSIZE / 2;

  vectors[1].buffer = (void*) 1;
  vectors[1].size = G_MAXSIZE / 2;

  vectors[2].buffer = (void*) 1;
  vectors[2].size = G_MAXSIZE / 2;

  file = xfile_new_tmp ("xfile_writev_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);

  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));

  xoutput_stream_writev_all_async (ostream, vectors, G_N_ELEMENTS (vectors), 0, NULL, test_writev_all_cb, &data);

  while (!data.done)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpuint (data.bytes_written, ==, 0);
  g_assert_error (data.error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_clear_error (&data.error);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_load_contents (file, NULL, (xchar_t **) &contents, &length, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  g_assert_cmpuint (length, ==, 0);

  g_free (contents);

  xfile_delete (file, NULL, NULL);
  xobject_unref (file);
}

static void
test_build_attribute_list_for_copy (void)
{
  xfile_t *tmpfile;
  xfile_io_stream_t *iostream;
  xerror_t *error = NULL;
  const xfile_copy_flags_t test_flags[] =
    {
      XFILE_COPY_NONE,
      XFILE_COPY_TARGET_DEFAULT_PERMS,
      XFILE_COPY_ALL_METADATA,
      XFILE_COPY_ALL_METADATA | XFILE_COPY_TARGET_DEFAULT_PERMS,
    };
  xsize_t i;
  char *attrs;
  xchar_t *attrs_with_commas;

  tmpfile = xfile_new_tmp ("tmp-build-attribute-list-for-copyXXXXXX",
                            &iostream, &error);
  g_assert_no_error (error);
  g_io_stream_close ((xio_stream_t*)iostream, NULL, &error);
  g_assert_no_error (error);
  g_clear_object (&iostream);

  for (i = 0; i < G_N_ELEMENTS (test_flags); i++)
    {
      xfile_copy_flags_t flags = test_flags[i];

      attrs = xfile_build_attribute_list_for_copy (tmpfile, flags, NULL, &error);
      g_test_message ("Attributes for copy: %s", attrs);
      g_assert_no_error (error);
      g_assert_nonnull (attrs);
      attrs_with_commas = xstrconcat (",", attrs, ",", NULL);
      g_free (attrs);

      /* See g_local_file_class_init for reference. */
      if (flags & XFILE_COPY_TARGET_DEFAULT_PERMS)
        g_assert_null (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_UNIX_MODE ","));
      else
        g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_UNIX_MODE ","));
#ifdef G_OS_UNIX
      if (flags & XFILE_COPY_ALL_METADATA)
        {
          g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_UNIX_UID ","));
          g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_UNIX_GID ","));
        }
      else
        {
          g_assert_null (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_UNIX_UID ","));
          g_assert_null (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_UNIX_GID ","));
        }
#endif
#ifdef HAVE_UTIMES
      g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_TIME_MODIFIED ","));
      g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_TIME_MODIFIED_USEC ","));
      if (flags & XFILE_COPY_ALL_METADATA)
        {
          g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_TIME_ACCESS ","));
          g_assert_nonnull (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_TIME_ACCESS_USEC ","));
        }
      else
        {
          g_assert_null (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_TIME_ACCESS ","));
          g_assert_null (xstrstr_len (attrs_with_commas, -1, "," XFILE_ATTRIBUTE_TIME_ACCESS_USEC ","));
        }
#endif
      g_free (attrs_with_commas);
    }

  (void) xfile_delete (tmpfile, NULL, NULL);
  g_clear_object (&tmpfile);
}

typedef struct
{
  xerror_t *error;
  xboolean_t done;
  xboolean_t res;
} MoveAsyncData;

static void
test_move_async_cb (xobject_t      *object,
                    xasync_result_t *result,
                    xpointer_t      user_data)
{
  xfile_t *file = XFILE (object);
  MoveAsyncData *data = user_data;
  xerror_t *error = NULL;

  data->res = xfile_move_finish (file, result, &error);
  data->error = error;
  data->done = TRUE;
}

typedef struct
{
  xoffset_t total_num_bytes;
} MoveAsyncProgressData;

static void
test_move_async_progress_cb (xoffset_t  current_num_bytes,
                             xoffset_t  total_num_bytes,
                             xpointer_t user_data)
{
  MoveAsyncProgressData *data = user_data;
  data->total_num_bytes = total_num_bytes;
}

/* Test that move_async() moves the file correctly */
static void
test_move_async (void)
{
  MoveAsyncData data = { 0 };
  MoveAsyncProgressData progress_data = { 0 };
  xfile_t *source;
  xfile_io_stream_t *iostream;
  xoutput_stream_t *ostream;
  xfile_t *destination;
  xchar_t *destination_path;
  xerror_t *error = NULL;
  xboolean_t res;
  const xuint8_t buffer[] = {1, 2, 3, 4, 5};

  source = xfile_new_tmp ("xfile_move_XXXXXX", &iostream, NULL);

  destination_path = g_build_path (G_DIR_SEPARATOR_S, g_get_tmp_dir (), "xfile_move_target", NULL);
  destination = xfile_new_for_path (destination_path);

  g_assert_nonnull (source);
  g_assert_nonnull (iostream);

  res = xfile_query_exists (source, NULL);
  g_assert_true (res);
  res = xfile_query_exists (destination, NULL);
  g_assert_false (res);

  // Write a known amount of bytes to the file, so we can test the progress callback against it
  ostream = g_io_stream_get_output_stream (XIO_STREAM (iostream));
  xoutput_stream_write (ostream, buffer, sizeof (buffer), NULL, &error);
  g_assert_no_error (error);

  xfile_move_async (source,
                     destination,
                     XFILE_COPY_NONE,
                     0,
                     NULL,
                     test_move_async_progress_cb,
                     &progress_data,
                     test_move_async_cb,
                     &data);

  while (!data.done)
    xmain_context_iteration (NULL, TRUE);

  g_assert_no_error (data.error);
  g_assert_true (data.res);
  g_assert_cmpuint (progress_data.total_num_bytes, ==, sizeof (buffer));

  res = xfile_query_exists (source, NULL);
  g_assert_false (res);
  res = xfile_query_exists (destination, NULL);
  g_assert_true (res);

  res = g_io_stream_close (XIO_STREAM (iostream), NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);
  xobject_unref (iostream);

  res = xfile_delete (destination, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  xobject_unref (source);
  xobject_unref (destination);

  g_free (destination_path);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/file/basic", test_basic);
  g_test_add_func ("/file/build-filename", test_build_filename);
  g_test_add_func ("/file/parent", test_parent);
  g_test_add_func ("/file/child", test_child);
  g_test_add_func ("/file/empty-path", test_empty_path);
  g_test_add_func ("/file/type", test_type);
  g_test_add_func ("/file/parse-name", test_parse_name);
  g_test_add_data_func ("/file/async-create-delete/0", GINT_TO_POINTER (0), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/1", GINT_TO_POINTER (1), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/10", GINT_TO_POINTER (10), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/25", GINT_TO_POINTER (25), test_create_delete);
  g_test_add_data_func ("/file/async-create-delete/4096", GINT_TO_POINTER (4096), test_create_delete);
  g_test_add_func ("/file/replace-load", test_replace_load);
  g_test_add_func ("/file/replace-cancel", test_replace_cancel);
  g_test_add_func ("/file/replace-symlink", test_replace_symlink);
  g_test_add_func ("/file/replace-symlink/using-etag", test_replace_symlink_using_etag);
  g_test_add_data_func ("/file/replace/write-only", GUINT_TO_POINTER (FALSE), test_replace);
  g_test_add_data_func ("/file/replace/read-write", GUINT_TO_POINTER (TRUE), test_replace);
  g_test_add_func ("/file/async-delete", test_async_delete);
  g_test_add_func ("/file/copy-preserve-mode", test_copy_preserve_mode);
  g_test_add_func ("/file/measure", test_measure);
  g_test_add_func ("/file/measure-async", test_measure_async);
  g_test_add_func ("/file/load-bytes", test_load_bytes);
  g_test_add_func ("/file/load-bytes-async", test_load_bytes_async);
  g_test_add_func ("/file/writev", test_writev);
  g_test_add_func ("/file/writev/no-bytes-written", test_writev_no_bytes_written);
  g_test_add_func ("/file/writev/no-vectors", test_writev_no_vectors);
  g_test_add_func ("/file/writev/empty-vectors", test_writev_empty_vectors);
  g_test_add_func ("/file/writev/too-big-vectors", test_writev_too_big_vectors);
  g_test_add_func ("/file/writev/async", test_writev_async);
  g_test_add_func ("/file/writev/async_all", test_writev_async_all);
  g_test_add_func ("/file/writev/async_all-empty-vectors", test_writev_async_all_empty_vectors);
  g_test_add_func ("/file/writev/async_all-no-vectors", test_writev_async_all_no_vectors);
  g_test_add_func ("/file/writev/async_all-to-big-vectors", test_writev_async_all_too_big_vectors);
  g_test_add_func ("/file/writev/async_all-cancellation", test_writev_async_all_cancellation);
  g_test_add_func ("/file/build-attribute-list-for-copy", test_build_attribute_list_for_copy);
  g_test_add_func ("/file/move_async", test_move_async);

  return g_test_run ();
}
