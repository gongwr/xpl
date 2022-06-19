/* Simple I/O stream. This is a utility class for tests, not a test.
 *
 * Copyright © 2008-2010 Red Hat, Inc.
 * Copyright © 2011 Nokia Corporation
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
 * Author: David Zeuthen <davidz@redhat.com>
 * Author: Simon McVittie <simon.mcvittie@collabora.co.uk>
 */

#include <gio/gio.h>

#include "test-io-stream.h"

XDEFINE_TYPE (TestIOStream, test_io_stream, XTYPE_IO_STREAM)

static void
test_io_stream_finalize (xobject_t *object)
{
  TestIOStream *stream = TEST_IO_STREAM (object);

  /* strictly speaking we should unref these in dispose, but
   * g_io_stream_dispose() wants them to still exist
   */
  g_clear_object (&stream->input_stream);
  g_clear_object (&stream->output_stream);

  XOBJECT_CLASS (test_io_stream_parent_class)->finalize (object);
}

static void
test_io_stream_init (TestIOStream *stream)
{
}

static xinput_stream_t *
test_io_stream_get_input_stream (xio_stream_t *_stream)
{
  TestIOStream *stream = TEST_IO_STREAM (_stream);

  return stream->input_stream;
}

static xoutput_stream_t *
test_io_stream_get_output_stream (xio_stream_t *_stream)
{
  TestIOStream *stream = TEST_IO_STREAM (_stream);

  return stream->output_stream;
}

static void
test_io_stream_class_init (TestIOStreamClass *klass)
{
  xobject_class_t *xobject_class;
  xio_stream_class_t *giostream_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->finalize = test_io_stream_finalize;

  giostream_class = XIO_STREAM_CLASS (klass);
  giostream_class->get_input_stream  = test_io_stream_get_input_stream;
  giostream_class->get_output_stream = test_io_stream_get_output_stream;
}

/**
 * test_io_stream_new:
 * @input_stream: an input stream
 * @output_stream: an output stream
 *
 * Return a simple #xio_stream_t binding together @input_stream and
 * @output_stream. They have no additional semantics as a result of being
 * part of this I/O stream: in particular, closing one does not close
 * the other (although closing the #xio_stream_t will close both sub-streams).
 *
 * Returns: (transfer full): a new #xio_stream_t
 */
xio_stream_t *
test_io_stream_new (xinput_stream_t  *input_stream,
                    xoutput_stream_t *output_stream)
{
  TestIOStream *stream;

  xreturn_val_if_fail (X_IS_INPUT_STREAM (input_stream), NULL);
  xreturn_val_if_fail (X_IS_OUTPUT_STREAM (output_stream), NULL);
  stream = TEST_IO_STREAM (xobject_new (TEST_TYPE_IO_STREAM, NULL));
  stream->input_stream = xobject_ref (input_stream);
  stream->output_stream = xobject_ref (output_stream);
  return XIO_STREAM (stream);
}
