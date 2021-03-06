/*
 * Copyright © 2014 NICE s.r.l.
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
 * Authors: Ignacio Casal Quinteiro <ignacio.casal@nice-software.com>
 */


#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "gsimpleiostream.h"
#include "gtask.h"

/**
 * SECTION:gsimpleiostream
 * @short_description: A wrapper around an input and an output stream.
 * @include: gio/gio.h
 * @see_also: #xio_stream_t
 *
 * xsimple_io_stream_t creates a #xio_stream_t from an arbitrary #xinput_stream_t and
 * #xoutput_stream_t. This allows any pair of input and output streams to be used
 * with #xio_stream_t methods.
 *
 * This is useful when you obtained a #xinput_stream_t and a #xoutput_stream_t
 * by other means, for instance creating them with platform specific methods as
 * g_unix_input_stream_new() or g_win32_input_stream_new(), and you want
 * to take advantage of the methods provided by #xio_stream_t.
 *
 * Since: 2.44
 */

/**
 * xsimple_io_stream_t:
 *
 * A wrapper around a #xinput_stream_t and a #xoutput_stream_t.
 *
 * Since: 2.44
 */
struct _GSimpleIOStream
{
  xio_stream_t parent;

  xinput_stream_t *input_stream;
  xoutput_stream_t *output_stream;
};

struct _GSimpleIOStreamClass
{
  xio_stream_class_t parent;
};

typedef struct _GSimpleIOStreamClass GSimpleIOStreamClass;

enum
{
  PROP_0,
  PROP_INPUT_STREAM,
  PROP_OUTPUT_STREAM
};

XDEFINE_TYPE (xsimple_io_stream, g_simple_io_stream, XTYPE_IO_STREAM)

static void
g_simple_io_stream_finalize (xobject_t *object)
{
  xsimple_io_stream_t *stream = G_SIMPLE_IO_STREAM (object);

  if (stream->input_stream)
    xobject_unref (stream->input_stream);

  if (stream->output_stream)
    xobject_unref (stream->output_stream);

  XOBJECT_CLASS (g_simple_io_stream_parent_class)->finalize (object);
}

static void
g_simple_io_stream_set_property (xobject_t      *object,
                                 xuint_t         prop_id,
                                 const xvalue_t *value,
                                 xparam_spec_t   *pspec)
{
  xsimple_io_stream_t *stream = G_SIMPLE_IO_STREAM (object);

  switch (prop_id)
    {
    case PROP_INPUT_STREAM:
      stream->input_stream = xvalue_dup_object (value);
      break;

    case PROP_OUTPUT_STREAM:
      stream->output_stream = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_simple_io_stream_get_property (xobject_t    *object,
                                 xuint_t       prop_id,
                                 xvalue_t     *value,
                                 xparam_spec_t *pspec)
{
  xsimple_io_stream_t *stream = G_SIMPLE_IO_STREAM (object);

  switch (prop_id)
    {
    case PROP_INPUT_STREAM:
      xvalue_set_object (value, stream->input_stream);
      break;

    case PROP_OUTPUT_STREAM:
      xvalue_set_object (value, stream->output_stream);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static xinput_stream_t *
g_simple_io_stream_get_input_stream (xio_stream_t *stream)
{
  xsimple_io_stream_t *simple_stream = G_SIMPLE_IO_STREAM (stream);

  return simple_stream->input_stream;
}

static xoutput_stream_t *
g_simple_io_stream_get_output_stream (xio_stream_t *stream)
{
  xsimple_io_stream_t *simple_stream = G_SIMPLE_IO_STREAM (stream);

  return simple_stream->output_stream;
}

static void
g_simple_io_stream_class_init (GSimpleIOStreamClass *class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (class);
  xio_stream_class_t *io_class = XIO_STREAM_CLASS (class);

  xobject_class->finalize = g_simple_io_stream_finalize;
  xobject_class->get_property = g_simple_io_stream_get_property;
  xobject_class->set_property = g_simple_io_stream_set_property;

  io_class->get_input_stream = g_simple_io_stream_get_input_stream;
  io_class->get_output_stream = g_simple_io_stream_get_output_stream;

  /**
   * xsimple_io_stream_t:input-stream:
   *
   * Since: 2.44
   */
  xobject_class_install_property (xobject_class, PROP_INPUT_STREAM,
                                   xparam_spec_object ("input-stream",
                                                        P_("Input stream"),
                                                        P_("The xinput_stream_t to read from"),
                                                        XTYPE_INPUT_STREAM,
                                                        XPARAM_READWRITE |
                                                        XPARAM_STATIC_STRINGS |
                                                        XPARAM_CONSTRUCT_ONLY));

  /**
   * xsimple_io_stream_t:output-stream:
   *
   * Since: 2.44
   */
  xobject_class_install_property (xobject_class, PROP_OUTPUT_STREAM,
                                   xparam_spec_object ("output-stream",
                                                        P_("Output stream"),
                                                        P_("The xoutput_stream_t to write to"),
                                                        XTYPE_OUTPUT_STREAM,
                                                        XPARAM_READWRITE |
                                                        XPARAM_STATIC_STRINGS |
                                                        XPARAM_CONSTRUCT_ONLY));
}

static void
g_simple_io_stream_init (xsimple_io_stream_t *stream)
{
}

/**
 * g_simple_io_stream_new:
 * @input_stream: a #xinput_stream_t.
 * @output_stream: a #xoutput_stream_t.
 *
 * Creates a new #xsimple_io_stream_t wrapping @input_stream and @output_stream.
 * See also #xio_stream_t.
 *
 * Returns: a new #xsimple_io_stream_t instance.
 *
 * Since: 2.44
 */
xio_stream_t *
g_simple_io_stream_new (xinput_stream_t  *input_stream,
                        xoutput_stream_t *output_stream)
{
  return xobject_new (XTYPE_SIMPLE_IO_STREAM,
                       "input-stream", input_stream,
                       "output-stream", output_stream,
                       NULL);
}
