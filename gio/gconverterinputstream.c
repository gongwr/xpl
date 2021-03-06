/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2009 Red Hat, Inc.
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

#include "gconverterinputstream.h"
#include "gpollableinputstream.h"
#include "gcancellable.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gconverterinputstream
 * @short_description: Converter Input Stream
 * @include: gio/gio.h
 * @see_also: #xinput_stream_t, #xconverter_t
 *
 * Converter input stream implements #xinput_stream_t and allows
 * conversion of data of various types during reading.
 *
 * As of GLib 2.34, #xconverter_input_stream_t implements
 * #xpollable_input_stream_t.
 **/

#define INITIAL_BUFFER_SIZE 4096

typedef struct {
  char *data;
  xsize_t start;
  xsize_t end;
  xsize_t size;
} buffer_t;

struct _GConverterInputStreamPrivate {
  xboolean_t at_input_end;
  xboolean_t finished;
  xboolean_t need_input;
  xconverter_t *converter;
  buffer_t input_buffer;
  buffer_t converted_buffer;
};

enum {
  PROP_0,
  PROP_CONVERTER
};

static void   xconverter_input_stream_set_property (xobject_t       *object,
						     xuint_t          prop_id,
						     const xvalue_t  *value,
						     xparam_spec_t    *pspec);
static void   xconverter_input_stream_get_property (xobject_t       *object,
						     xuint_t          prop_id,
						     xvalue_t        *value,
						     xparam_spec_t    *pspec);
static void   xconverter_input_stream_finalize     (xobject_t       *object);
static xssize_t xconverter_input_stream_read         (xinput_stream_t  *stream,
						     void          *buffer,
						     xsize_t          count,
						     xcancellable_t  *cancellable,
						     xerror_t       **error);

static xboolean_t xconverter_input_stream_can_poll         (xpollable_input_stream_t *stream);
static xboolean_t xconverter_input_stream_is_readable      (xpollable_input_stream_t *stream);
static xssize_t   xconverter_input_stream_read_nonblocking (xpollable_input_stream_t  *stream,
							   void                  *buffer,
							   xsize_t                  size,
							   xerror_t               **error);

static xsource_t *xconverter_input_stream_create_source    (xpollable_input_stream_t *stream,
							   xcancellable_t          *cancellable);

static void xconverter_input_stream_pollable_iface_init  (xpollable_input_stream_interface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xconverter_input_stream,
			 xconverter_input_stream,
			 XTYPE_FILTER_INPUT_STREAM,
                         G_ADD_PRIVATE (xconverter_input_stream)
			 G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_INPUT_STREAM,
						xconverter_input_stream_pollable_iface_init))

static void
xconverter_input_stream_class_init (xconverter_input_stream_class_t *klass)
{
  xobject_class_t *object_class;
  xinput_stream_class_t *istream_class;

  object_class = XOBJECT_CLASS (klass);
  object_class->get_property = xconverter_input_stream_get_property;
  object_class->set_property = xconverter_input_stream_set_property;
  object_class->finalize     = xconverter_input_stream_finalize;

  istream_class = G_INPUT_STREAM_CLASS (klass);
  istream_class->read_fn = xconverter_input_stream_read;

  xobject_class_install_property (object_class,
				   PROP_CONVERTER,
				   xparam_spec_object ("converter",
							P_("Converter"),
							P_("The converter object"),
							XTYPE_CONVERTER,
							XPARAM_READWRITE|
							XPARAM_CONSTRUCT_ONLY|
							XPARAM_STATIC_STRINGS));

}

static void
xconverter_input_stream_pollable_iface_init (xpollable_input_stream_interface_t *iface)
{
  iface->can_poll = xconverter_input_stream_can_poll;
  iface->is_readable = xconverter_input_stream_is_readable;
  iface->read_nonblocking = xconverter_input_stream_read_nonblocking;
  iface->create_source = xconverter_input_stream_create_source;
}

static void
xconverter_input_stream_finalize (xobject_t *object)
{
  GConverterInputStreamPrivate *priv;
  xconverter_input_stream_t        *stream;

  stream = XCONVERTER_INPUT_STREAM (object);
  priv = stream->priv;

  g_free (priv->input_buffer.data);
  g_free (priv->converted_buffer.data);
  if (priv->converter)
    xobject_unref (priv->converter);

  XOBJECT_CLASS (xconverter_input_stream_parent_class)->finalize (object);
}

static void
xconverter_input_stream_set_property (xobject_t      *object,
				       xuint_t         prop_id,
				       const xvalue_t *value,
				       xparam_spec_t   *pspec)
{
  xconverter_input_stream_t *cstream;

  cstream = XCONVERTER_INPUT_STREAM (object);

   switch (prop_id)
    {
    case PROP_CONVERTER:
      cstream->priv->converter = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
xconverter_input_stream_get_property (xobject_t    *object,
				       xuint_t       prop_id,
				       xvalue_t     *value,
				       xparam_spec_t *pspec)
{
  GConverterInputStreamPrivate *priv;
  xconverter_input_stream_t        *cstream;

  cstream = XCONVERTER_INPUT_STREAM (object);
  priv = cstream->priv;

  switch (prop_id)
    {
    case PROP_CONVERTER:
      xvalue_set_object (value, priv->converter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}
static void
xconverter_input_stream_init (xconverter_input_stream_t *stream)
{
  stream->priv = xconverter_input_stream_get_instance_private (stream);
}

/**
 * xconverter_input_stream_new:
 * @base_stream: a #xinput_stream_t
 * @converter: a #xconverter_t
 *
 * Creates a new converter input stream for the @base_stream.
 *
 * Returns: a new #xinput_stream_t.
 **/
xinput_stream_t *
xconverter_input_stream_new (xinput_stream_t *base_stream,
                              xconverter_t   *converter)
{
  xinput_stream_t *stream;

  xreturn_val_if_fail (X_IS_INPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_CONVERTER_INPUT_STREAM,
                         "base-stream", base_stream,
			 "converter", converter,
			 NULL);

  return stream;
}

static xsize_t
buffer_data_size (buffer_t *buffer)
{
  return buffer->end - buffer->start;
}

static xsize_t
buffer_tailspace (buffer_t *buffer)
{
  return buffer->size - buffer->end;
}

static char *
buffer_data (buffer_t *buffer)
{
  return buffer->data + buffer->start;
}

static void
buffer_consumed (buffer_t *buffer,
		 xsize_t count)
{
  buffer->start += count;
  if (buffer->start == buffer->end)
    buffer->start = buffer->end = 0;
}

static void
buffer_read (buffer_t *buffer,
	     char *dest,
	     xsize_t count)
{
  if (count != 0)
    memcpy (dest, buffer->data + buffer->start, count);

  buffer_consumed (buffer, count);
}

static void
compact_buffer (buffer_t *buffer)
{
  xsize_t in_buffer;

  in_buffer = buffer_data_size (buffer);
  memmove (buffer->data,
	   buffer->data + buffer->start,
	   in_buffer);
  buffer->end -= buffer->start;
  buffer->start = 0;
}

static void
grow_buffer (buffer_t *buffer)
{
  char *data;
  xsize_t size, in_buffer;

  if (buffer->size == 0)
    size = INITIAL_BUFFER_SIZE;
  else
    size = buffer->size * 2;

  data = g_malloc (size);
  in_buffer = buffer_data_size (buffer);

  if (in_buffer != 0)
    memcpy (data,
            buffer->data + buffer->start,
            in_buffer);

  g_free (buffer->data);
  buffer->data = data;
  buffer->end -= buffer->start;
  buffer->start = 0;
  buffer->size = size;
}

/* Ensures that the buffer can fit at_least_size bytes,
 * *including* the current in-buffer data */
static void
buffer_ensure_space (buffer_t *buffer,
		     xsize_t at_least_size)
{
  xsize_t in_buffer, left_to_fill;

  in_buffer = buffer_data_size (buffer);

  if (in_buffer >= at_least_size)
    return;

  left_to_fill = buffer_tailspace (buffer);

  if (in_buffer + left_to_fill >= at_least_size)
    {
      /* We fit in remaining space at end */
      /* If the copy is small, compact now anyway so we can fill more */
      if (in_buffer < 256)
	compact_buffer (buffer);
    }
  else if (buffer->size >= at_least_size)
    {
      /* We fit, but only if we compact */
      compact_buffer (buffer);
    }
  else
    {
      /* Need to grow buffer */
      while (buffer->size < at_least_size)
	grow_buffer (buffer);
    }
}

static xssize_t
fill_input_buffer (xconverter_input_stream_t  *stream,
		   xsize_t                   at_least_size,
		   xboolean_t                blocking,
		   xcancellable_t           *cancellable,
		   xerror_t                **error)
{
  GConverterInputStreamPrivate *priv;
  xinput_stream_t *base_stream;
  xssize_t nread;

  priv = stream->priv;

  buffer_ensure_space (&priv->input_buffer, at_least_size);

  base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;
  nread = g_pollable_stream_read (base_stream,
				  priv->input_buffer.data + priv->input_buffer.end,
				  buffer_tailspace (&priv->input_buffer),
				  blocking,
				  cancellable,
				  error);

  if (nread > 0)
    {
      priv->input_buffer.end += nread;
      priv->need_input = FALSE;
    }

  return nread;
}


static xssize_t
read_internal (xinput_stream_t *stream,
	       void         *buffer,
	       xsize_t         count,
	       xboolean_t      blocking,
	       xcancellable_t *cancellable,
	       xerror_t      **error)
{
  xconverter_input_stream_t *cstream;
  GConverterInputStreamPrivate *priv;
  xsize_t available, total_bytes_read;
  xssize_t nread;
  xconverter_result_t res;
  xsize_t bytes_read;
  xsize_t bytes_written;
  xerror_t *my_error;
  xerror_t *my_error2;

  cstream = XCONVERTER_INPUT_STREAM (stream);
  priv = cstream->priv;

  available = buffer_data_size (&priv->converted_buffer);

  if (available > 0 &&
      count <= available)
    {
      /* Converted data available, return that */
      buffer_read (&priv->converted_buffer, buffer, count);
      return count;
    }

  /* Full request not available, read all currently available and request
     refill/conversion for more */

  buffer_read (&priv->converted_buffer, buffer, available);

  total_bytes_read = available;
  buffer = (char *) buffer + available;
  count -= available;

  /* If there is no data to convert, and no pre-converted data,
     do some i/o for more input */
  if (buffer_data_size (&priv->input_buffer) == 0 &&
      total_bytes_read == 0 &&
      !priv->at_input_end)
    {
      nread = fill_input_buffer (cstream, count, blocking, cancellable, error);
      if (nread < 0)
	return -1;
      if (nread == 0)
	priv->at_input_end = TRUE;
    }

  /* First try to convert any available data (or state) directly to the user buffer: */
  if (!priv->finished)
    {
      my_error = NULL;
      res = xconverter_convert (priv->converter,
				 buffer_data (&priv->input_buffer),
				 buffer_data_size (&priv->input_buffer),
				 buffer, count,
				 priv->at_input_end ? XCONVERTER_INPUT_AT_END : 0,
				 &bytes_read,
				 &bytes_written,
				 &my_error);
      if (res != XCONVERTER_ERROR)
	{
	  total_bytes_read += bytes_written;
	  buffer_consumed (&priv->input_buffer, bytes_read);
	  if (res == XCONVERTER_FINISHED)
	    priv->finished = TRUE; /* We're done converting */
	}
      else if (total_bytes_read == 0 &&
	       !xerror_matches (my_error,
				 G_IO_ERROR,
				 G_IO_ERROR_PARTIAL_INPUT) &&
	       !xerror_matches (my_error,
				 G_IO_ERROR,
				 G_IO_ERROR_NO_SPACE))
	{
	  /* No previously read data and no "special" error, return error */
	  g_propagate_error (error, my_error);
	  return -1;
	}
      else
	xerror_free (my_error);
    }

  /* We had some pre-converted data and/or we converted directly to the
     user buffer */
  if (total_bytes_read > 0)
    return total_bytes_read;

  /* If there is no more to convert, return EOF */
  if (priv->finished)
    {
      xassert (buffer_data_size (&priv->converted_buffer) == 0);
      return 0;
    }

  /* There was "complexity" in the straight-to-buffer conversion,
   * convert to our own buffer and write from that.
   * At this point we didn't produce any data into @buffer.
   */

  /* Ensure we have *some* initial target space */
  buffer_ensure_space (&priv->converted_buffer, count);

  while (TRUE)
    {
      xassert (!priv->finished);

      /* Try to convert to our buffer */
      my_error = NULL;
      res = xconverter_convert (priv->converter,
				 buffer_data (&priv->input_buffer),
				 buffer_data_size (&priv->input_buffer),
				 buffer_data (&priv->converted_buffer),
				 buffer_tailspace (&priv->converted_buffer),
				 priv->at_input_end ? XCONVERTER_INPUT_AT_END : 0,
				 &bytes_read,
				 &bytes_written,
				 &my_error);
      if (res != XCONVERTER_ERROR)
	{
	  priv->converted_buffer.end += bytes_written;
	  buffer_consumed (&priv->input_buffer, bytes_read);

	  /* Maybe we consumed without producing any output */
	  if (buffer_data_size (&priv->converted_buffer) == 0 && res != XCONVERTER_FINISHED)
	    continue; /* Convert more */

	  if (res == XCONVERTER_FINISHED)
	    priv->finished = TRUE;

	  total_bytes_read = MIN (count, buffer_data_size (&priv->converted_buffer));
	  buffer_read (&priv->converted_buffer, buffer, total_bytes_read);

	  xassert (priv->finished || total_bytes_read > 0);

	  return total_bytes_read;
	}

      /* There was some kind of error filling our buffer */

      if (xerror_matches (my_error,
			   G_IO_ERROR,
			   G_IO_ERROR_PARTIAL_INPUT) &&
	  !priv->at_input_end)
	{
	  /* Need more data */
	  my_error2 = NULL;
	  nread = fill_input_buffer (cstream,
				     buffer_data_size (&priv->input_buffer) + 4096,
				     blocking,
				     cancellable,
				     &my_error2);
	  if (nread < 0)
	    {
	      /* Can't read any more data, return that error */
	      xerror_free (my_error);
	      g_propagate_error (error, my_error2);
	      priv->need_input = TRUE;
	      return -1;
	    }
	  else if (nread == 0)
	    {
	      /* End of file, try INPUT_AT_END */
	      priv->at_input_end = TRUE;
	    }
	  xerror_free (my_error);
	  continue;
	}

      if (xerror_matches (my_error,
			   G_IO_ERROR,
			   G_IO_ERROR_NO_SPACE))
	{
	  /* Need more destination space, grow it
	   * Note: if we actually grow the buffer (as opposed to compacting it),
	   * this will double the size, not just add one byte. */
	  buffer_ensure_space (&priv->converted_buffer,
			       priv->converted_buffer.size + 1);
	  xerror_free (my_error);
	  continue;
	}

      /* Any other random error, return it */
      g_propagate_error (error, my_error);
      return -1;
    }

  g_assert_not_reached ();
}

static xssize_t
xconverter_input_stream_read (xinput_stream_t *stream,
			       void         *buffer,
			       xsize_t         count,
			       xcancellable_t *cancellable,
			       xerror_t      **error)
{
  return read_internal (stream, buffer, count, TRUE, cancellable, error);
}

static xboolean_t
xconverter_input_stream_can_poll (xpollable_input_stream_t *stream)
{
  xinput_stream_t *base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;

  return (X_IS_POLLABLE_INPUT_STREAM (base_stream) &&
	  g_pollable_input_stream_can_poll (G_POLLABLE_INPUT_STREAM (base_stream)));
}

static xboolean_t
xconverter_input_stream_is_readable (xpollable_input_stream_t *stream)
{
  xinput_stream_t *base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;
  xconverter_input_stream_t *cstream = XCONVERTER_INPUT_STREAM (stream);

  if (buffer_data_size (&cstream->priv->converted_buffer))
    return TRUE;
  else if (buffer_data_size (&cstream->priv->input_buffer) &&
	   !cstream->priv->need_input)
    return TRUE;
  else
    return g_pollable_input_stream_is_readable (G_POLLABLE_INPUT_STREAM (base_stream));
}

static xssize_t
xconverter_input_stream_read_nonblocking (xpollable_input_stream_t  *stream,
					   void                  *buffer,
					   xsize_t                  count,
					   xerror_t               **error)
{
  return read_internal (G_INPUT_STREAM (stream), buffer, count,
			FALSE, NULL, error);
}

static xsource_t *
xconverter_input_stream_create_source (xpollable_input_stream_t *stream,
					xcancellable_t         *cancellable)
{
  xinput_stream_t *base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;
  xsource_t *base_source, *pollable_source;

  if (g_pollable_input_stream_is_readable (stream))
    base_source = g_timeout_source_new (0);
  else
    base_source = g_pollable_input_stream_create_source (G_POLLABLE_INPUT_STREAM (base_stream), NULL);

  pollable_source = g_pollable_source_new_full (stream, base_source,
						cancellable);
  xsource_unref (base_source);

  return pollable_source;
}


/**
 * xconverter_input_stream_get_converter:
 * @converter_stream: a #xconverter_input_stream_t
 *
 * Gets the #xconverter_t that is used by @converter_stream.
 *
 * Returns: (transfer none): the converter of the converter input stream
 *
 * Since: 2.24
 */
xconverter_t *
xconverter_input_stream_get_converter (xconverter_input_stream_t *converter_stream)
{
  return converter_stream->priv->converter;
}
