/* GLib testing framework examples and tests
 * Authors: Jesse van den Kieboom <jessevdk@gnome.org>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_TO_WRITE "Hello world\n"

typedef struct
{
  xoutput_stream_t *conv_stream;
  xoutput_stream_t *data_stream;
  xchar_t *expected_output;
  xsize_t expected_size;
  xmain_loop_t *main_loop;
} SetupData;

static void
create_streams (SetupData *data)
{
  xconverter_t *converter;

  converter = G_CONVERTER (g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, -1));

  data->data_stream = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  data->conv_stream = g_converter_output_stream_new (data->data_stream,
                                                     converter);

  xobject_unref (converter);
}

static void
destroy_streams (SetupData *data)
{
  xobject_unref (data->data_stream);
  xobject_unref (data->conv_stream);
}

static void
write_data_to_stream (SetupData *data)
{
  xsize_t bytes_written;
  xerror_t *error = NULL;

  /* just write the data synchronously */
  xoutput_stream_write_all (data->conv_stream,
                             DATA_TO_WRITE,
                             sizeof (DATA_TO_WRITE),
                             &bytes_written,
                             NULL,
                             &error);

  g_assert_no_error (error);
  g_assert_cmpint (sizeof (DATA_TO_WRITE), ==, bytes_written);
}

static void
setup_data (SetupData     *data,
            xconstpointer  user_data)
{
  data->main_loop = xmain_loop_new (NULL, FALSE);
  create_streams (data);
}

static void
teardown_data (SetupData     *data,
               xconstpointer  user_data)
{
  /* cleanup */
  xmain_loop_unref (data->main_loop);

  destroy_streams (data);

  g_free (data->expected_output);
}

static void
compare_output (SetupData *data)
{
  xsize_t size;
  xpointer_t written;

  written = g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (data->data_stream));
  size = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (data->data_stream));

  g_assert_cmpmem (written, size, data->expected_output, data->expected_size);
}

static void
async_close_ready (xoutput_stream_t *stream,
                   xasync_result_t  *result,
                   SetupData     *data)
{
  xerror_t *error = NULL;

  /* finish the close */
  xoutput_stream_close_finish (stream, result, &error);

  g_assert_no_error (error);

  /* compare the output with the desired output */
  compare_output (data);

  xmain_loop_quit (data->main_loop);
}

static void
prepare_data (SetupData *data,
              xboolean_t   manual_flush)
{
  xerror_t *error = NULL;
  xpointer_t written;

  write_data_to_stream (data);

  if (manual_flush)
    {
      xoutput_stream_flush (data->conv_stream, NULL, &error);
      g_assert_no_error (error);
    }

  xoutput_stream_close (data->conv_stream, NULL, &error);

  g_assert_no_error (error);

  written = g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (data->data_stream));

  data->expected_size = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (data->data_stream));

  g_assert_cmpuint (data->expected_size, >, 0);

  data->expected_output = g_memdup2 (written, data->expected_size);

  /* then recreate the streams and prepare them for the asynchronous close */
  destroy_streams (data);
  create_streams (data);

  write_data_to_stream (data);
}

static void
test_without_flush (SetupData     *data,
                    xconstpointer  user_data)
{
  prepare_data (data, FALSE);

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=617937");

  /* just close asynchronously */
  xoutput_stream_close_async (data->conv_stream,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               (xasync_ready_callback_t)async_close_ready,
                               data);

  xmain_loop_run (data->main_loop);
}

static void
test_with_flush (SetupData *data, xconstpointer user_data)
{
  xerror_t *error = NULL;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=617937");

  prepare_data (data, TRUE);

  xoutput_stream_flush (data->conv_stream, NULL, &error);

  g_assert_no_error (error);

  /* then close asynchronously */
  xoutput_stream_close_async (data->conv_stream,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               (xasync_ready_callback_t)async_close_ready,
                               data);

  xmain_loop_run (data->main_loop);
}

static void
async_flush_ready (xoutput_stream_t *stream,
                   xasync_result_t  *result,
                   SetupData     *data)
{
  xerror_t *error = NULL;

  xoutput_stream_flush_finish (stream, result, &error);

  g_assert_no_error (error);

  /* then close async after the flush */
  xoutput_stream_close_async (data->conv_stream,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               (xasync_ready_callback_t)async_close_ready,
                               data);
}

static void
test_with_async_flush (SetupData     *data,
                       xconstpointer  user_data)
{
  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=617937");

  prepare_data (data, TRUE);

  /* first flush async */
  xoutput_stream_flush_async (data->conv_stream,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               (xasync_ready_callback_t)async_flush_ready,
                               data);

  xmain_loop_run (data->main_loop);
}

int
main (int   argc,
      char *argv[])
{
  SetupData *data;

  g_test_init (&argc, &argv, NULL);

  data = g_slice_new (SetupData);

  /* test closing asynchronously without flushing manually */
  g_test_add ("/close-async/without-flush",
              SetupData,
              data,
              setup_data,
              test_without_flush,
              teardown_data);

  /* test closing asynchronously with a synchronous manually flush */
  g_test_add ("/close-async/with-flush",
              SetupData,
              data,
              setup_data,
              test_with_flush,
              teardown_data);

  /* test closing asynchronously with an asynchronous manually flush */
  g_test_add ("/close-async/with-async-flush",
              SetupData,
              data,
              setup_data,
              test_with_async_flush,
              teardown_data);

  g_slice_free (SetupData, data);

  return g_test_run();
}
