/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2007 Jürg Billeter
 * Copyright © 2009 Codethink Limited
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
#include "gdatainputstream.h"
#include "gtask.h"
#include "gcancellable.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "glibintl.h"

#include <string.h>

/**
 * SECTION:gdatainputstream
 * @short_description: Data Input Stream
 * @include: gio/gio.h
 * @see_also: #xinput_stream_t
 *
 * Data input stream implements #xinput_stream_t and includes functions for
 * reading structured data directly from a binary input stream.
 *
 **/

struct _xdata_input_stream_private {
  xdata_stream_byte_order_t byte_order;
  xdata_stream_newline_type_t newline_type;
};

enum {
  PROP_0,
  PROP_BYTE_ORDER,
  PROP_NEWLINE_TYPE
};

static void xdata_input_stream_set_property (xobject_t      *object,
					      xuint_t         prop_id,
					      const xvalue_t *value,
					      xparam_spec_t   *pspec);
static void xdata_input_stream_get_property (xobject_t      *object,
					      xuint_t         prop_id,
					      xvalue_t       *value,
					      xparam_spec_t   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (xdata_input_stream_t,
                            xdata_input_stream,
                            XTYPE_BUFFERED_INPUT_STREAM)


static void
xdata_input_stream_class_init (xdata_input_stream_class_t *klass)
{
  xobject_class_t *object_class;

  object_class = XOBJECT_CLASS (klass);
  object_class->get_property = xdata_input_stream_get_property;
  object_class->set_property = xdata_input_stream_set_property;

  /**
   * xdata_input_stream_t:byte-order:
   *
   * The :byte-order property determines the byte ordering that
   * is used when reading multi-byte entities (such as integers)
   * from the stream.
   */
  xobject_class_install_property (object_class,
                                   PROP_BYTE_ORDER,
                                   xparam_spec_enum ("byte-order",
                                                      P_("Byte order"),
                                                      P_("The byte order"),
                                                      XTYPE_DATA_STREAM_BYTE_ORDER,
                                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN,
                                                      XPARAM_READWRITE|XPARAM_STATIC_NAME|XPARAM_STATIC_BLURB));

  /**
   * xdata_input_stream_t:newline-type:
   *
   * The :newline-type property determines what is considered
   * as a line ending when reading complete lines from the stream.
   */
  xobject_class_install_property (object_class,
                                   PROP_NEWLINE_TYPE,
                                   xparam_spec_enum ("newline-type",
                                                      P_("Newline type"),
                                                      P_("The accepted types of line ending"),
                                                      XTYPE_DATA_STREAM_NEWLINE_TYPE,
                                                      G_DATA_STREAM_NEWLINE_TYPE_LF,
                                                      XPARAM_READWRITE|XPARAM_STATIC_NAME|XPARAM_STATIC_BLURB));
}

static void
xdata_input_stream_set_property (xobject_t      *object,
				  xuint_t         prop_id,
				  const xvalue_t *value,
				  xparam_spec_t   *pspec)
{
  xdata_input_stream_t        *dstream;

  dstream = G_DATA_INPUT_STREAM (object);

   switch (prop_id)
    {
    case PROP_BYTE_ORDER:
      xdata_input_stream_set_byte_order (dstream, xvalue_get_enum (value));
      break;

    case PROP_NEWLINE_TYPE:
      xdata_input_stream_set_newline_type (dstream, xvalue_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
xdata_input_stream_get_property (xobject_t    *object,
                                  xuint_t       prop_id,
                                  xvalue_t     *value,
                                  xparam_spec_t *pspec)
{
  xdata_input_stream_private_t *priv;
  xdata_input_stream_t        *dstream;

  dstream = G_DATA_INPUT_STREAM (object);
  priv = dstream->priv;

  switch (prop_id)
    {
    case PROP_BYTE_ORDER:
      xvalue_set_enum (value, priv->byte_order);
      break;

    case PROP_NEWLINE_TYPE:
      xvalue_set_enum (value, priv->newline_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}
static void
xdata_input_stream_init (xdata_input_stream_t *stream)
{
  stream->priv = xdata_input_stream_get_instance_private (stream);
  stream->priv->byte_order = G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN;
  stream->priv->newline_type = G_DATA_STREAM_NEWLINE_TYPE_LF;
}

/**
 * xdata_input_stream_new:
 * @base_stream: a #xinput_stream_t.
 *
 * Creates a new data input stream for the @base_stream.
 *
 * Returns: a new #xdata_input_stream_t.
 **/
xdata_input_stream_t *
xdata_input_stream_new (xinput_stream_t *base_stream)
{
  xdata_input_stream_t *stream;

  xreturn_val_if_fail (X_IS_INPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_DATA_INPUT_STREAM,
                         "base-stream", base_stream,
                         NULL);

  return stream;
}

/**
 * xdata_input_stream_set_byte_order:
 * @stream: a given #xdata_input_stream_t.
 * @order: a #xdata_stream_byte_order_t to set.
 *
 * This function sets the byte order for the given @stream. All subsequent
 * reads from the @stream will be read in the given @order.
 *
 **/
void
xdata_input_stream_set_byte_order (xdata_input_stream_t     *stream,
				    xdata_stream_byte_order_t  order)
{
  xdata_input_stream_private_t *priv;

  g_return_if_fail (X_IS_DATA_INPUT_STREAM (stream));

  priv = stream->priv;

  if (priv->byte_order != order)
    {
      priv->byte_order = order;

      xobject_notify (G_OBJECT (stream), "byte-order");
    }
}

/**
 * xdata_input_stream_get_byte_order:
 * @stream: a given #xdata_input_stream_t.
 *
 * Gets the byte order for the data input stream.
 *
 * Returns: the @stream's current #xdata_stream_byte_order_t.
 **/
xdata_stream_byte_order_t
xdata_input_stream_get_byte_order (xdata_input_stream_t *stream)
{
  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN);

  return stream->priv->byte_order;
}

/**
 * xdata_input_stream_set_newline_type:
 * @stream: a #xdata_input_stream_t.
 * @type: the type of new line return as #xdata_stream_newline_type_t.
 *
 * Sets the newline type for the @stream.
 *
 * Note that using G_DATA_STREAM_NEWLINE_TYPE_ANY is slightly unsafe. If a read
 * chunk ends in "CR" we must read an additional byte to know if this is "CR" or
 * "CR LF", and this might block if there is no more data available.
 *
 **/
void
xdata_input_stream_set_newline_type (xdata_input_stream_t       *stream,
				      xdata_stream_newline_type_t  type)
{
  xdata_input_stream_private_t *priv;

  g_return_if_fail (X_IS_DATA_INPUT_STREAM (stream));

  priv = stream->priv;

  if (priv->newline_type != type)
    {
      priv->newline_type = type;

      xobject_notify (G_OBJECT (stream), "newline-type");
    }
}

/**
 * xdata_input_stream_get_newline_type:
 * @stream: a given #xdata_input_stream_t.
 *
 * Gets the current newline type for the @stream.
 *
 * Returns: #xdata_stream_newline_type_t for the given @stream.
 **/
xdata_stream_newline_type_t
xdata_input_stream_get_newline_type (xdata_input_stream_t *stream)
{
  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), G_DATA_STREAM_NEWLINE_TYPE_ANY);

  return stream->priv->newline_type;
}

static xboolean_t
read_data (xdata_input_stream_t  *stream,
           void              *buffer,
           xsize_t              size,
           xcancellable_t      *cancellable,
           xerror_t           **error)
{
  xsize_t available;
  xssize_t res;

  while ((available = xbuffered_input_stream_get_available (G_BUFFERED_INPUT_STREAM (stream))) < size)
    {
      res = xbuffered_input_stream_fill (G_BUFFERED_INPUT_STREAM (stream),
					  size - available,
					  cancellable, error);
      if (res < 0)
	return FALSE;
      if (res == 0)
	{
	  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                               _("Unexpected early end-of-stream"));
	  return FALSE;
	}
    }

  /* This should always succeed, since it's in the buffer */
  res = xinput_stream_read (G_INPUT_STREAM (stream),
			     buffer, size,
			     NULL, NULL);
  g_warn_if_fail (res >= 0 && (xsize_t) res == size);
  return TRUE;
}


/**
 * xdata_input_stream_read_byte:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads an unsigned 8-bit/1-byte value from @stream.
 *
 * Returns: an unsigned 8-bit/1-byte value read from the @stream or `0`
 * if an error occurred.
 **/
xuchar_t
xdata_input_stream_read_byte (xdata_input_stream_t  *stream,
			       xcancellable_t       *cancellable,
			       xerror_t            **error)
{
  xuchar_t c;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), '\0');

  if (read_data (stream, &c, 1, cancellable, error))
      return c;

  return 0;
}


/**
 * xdata_input_stream_read_int16:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads a 16-bit/2-byte value from @stream.
 *
 * In order to get the correct byte order for this read operation,
 * see xdata_input_stream_get_byte_order() and xdata_input_stream_set_byte_order().
 *
 * Returns: a signed 16-bit/2-byte value read from @stream or `0` if
 * an error occurred.
 **/
gint16
xdata_input_stream_read_int16 (xdata_input_stream_t  *stream,
			       xcancellable_t       *cancellable,
			       xerror_t            **error)
{
  gint16 v;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), 0);

  if (read_data (stream, &v, 2, cancellable, error))
    {
      switch (stream->priv->byte_order)
	{
	case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
	  v = GINT16_FROM_BE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
	  v = GINT16_FROM_LE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
	default:
	  break;
	}
      return v;
    }

  return 0;
}


/**
 * xdata_input_stream_read_uint16:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads an unsigned 16-bit/2-byte value from @stream.
 *
 * In order to get the correct byte order for this read operation,
 * see xdata_input_stream_get_byte_order() and xdata_input_stream_set_byte_order().
 *
 * Returns: an unsigned 16-bit/2-byte value read from the @stream or `0` if
 * an error occurred.
 **/
xuint16_t
xdata_input_stream_read_uint16 (xdata_input_stream_t  *stream,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  xuint16_t v;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), 0);

  if (read_data (stream, &v, 2, cancellable, error))
    {
      switch (stream->priv->byte_order)
	{
	case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
	  v = GUINT16_FROM_BE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
	  v = GUINT16_FROM_LE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
	default:
	  break;
	}
      return v;
    }

  return 0;
}


/**
 * xdata_input_stream_read_int32:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads a signed 32-bit/4-byte value from @stream.
 *
 * In order to get the correct byte order for this read operation,
 * see xdata_input_stream_get_byte_order() and xdata_input_stream_set_byte_order().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: a signed 32-bit/4-byte value read from the @stream or `0` if
 * an error occurred.
 **/
gint32
xdata_input_stream_read_int32 (xdata_input_stream_t  *stream,
				xcancellable_t       *cancellable,
				xerror_t            **error)
{
  gint32 v;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), 0);

  if (read_data (stream, &v, 4, cancellable, error))
    {
      switch (stream->priv->byte_order)
	{
	case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
	  v = GINT32_FROM_BE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
	  v = GINT32_FROM_LE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
	default:
	  break;
	}
      return v;
    }

  return 0;
}


/**
 * xdata_input_stream_read_uint32:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads an unsigned 32-bit/4-byte value from @stream.
 *
 * In order to get the correct byte order for this read operation,
 * see xdata_input_stream_get_byte_order() and xdata_input_stream_set_byte_order().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: an unsigned 32-bit/4-byte value read from the @stream or `0` if
 * an error occurred.
 **/
xuint32_t
xdata_input_stream_read_uint32 (xdata_input_stream_t  *stream,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  xuint32_t v;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), 0);

  if (read_data (stream, &v, 4, cancellable, error))
    {
      switch (stream->priv->byte_order)
	{
	case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
	  v = GUINT32_FROM_BE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
	  v = GUINT32_FROM_LE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
	default:
	  break;
	}
      return v;
    }

  return 0;
}


/**
 * xdata_input_stream_read_int64:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads a 64-bit/8-byte value from @stream.
 *
 * In order to get the correct byte order for this read operation,
 * see xdata_input_stream_get_byte_order() and xdata_input_stream_set_byte_order().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: a signed 64-bit/8-byte value read from @stream or `0` if
 * an error occurred.
 **/
sint64_t
xdata_input_stream_read_int64 (xdata_input_stream_t  *stream,
			       xcancellable_t       *cancellable,
			       xerror_t            **error)
{
  sint64_t v;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), 0);

  if (read_data (stream, &v, 8, cancellable, error))
    {
      switch (stream->priv->byte_order)
	{
	case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
	  v = GINT64_FROM_BE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
	  v = GINT64_FROM_LE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
	default:
	  break;
	}
      return v;
    }

  return 0;
}


/**
 * xdata_input_stream_read_uint64:
 * @stream: a given #xdata_input_stream_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads an unsigned 64-bit/8-byte value from @stream.
 *
 * In order to get the correct byte order for this read operation,
 * see xdata_input_stream_get_byte_order().
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: an unsigned 64-bit/8-byte read from @stream or `0` if
 * an error occurred.
 **/
xuint64_t
xdata_input_stream_read_uint64 (xdata_input_stream_t  *stream,
				xcancellable_t       *cancellable,
				xerror_t            **error)
{
  xuint64_t v;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), 0);

  if (read_data (stream, &v, 8, cancellable, error))
    {
      switch (stream->priv->byte_order)
	{
	case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
	  v = GUINT64_FROM_BE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
	  v = GUINT64_FROM_LE (v);
	  break;
	case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
	default:
	  break;
	}
      return v;
    }

  return 0;
}

static xssize_t
scan_for_newline (xdata_input_stream_t *stream,
		  xsize_t            *checked_out,
		  xboolean_t         *last_saw_cr_out,
		  int              *newline_len_out)
{
  xbuffered_input_stream_t *bstream;
  xdata_input_stream_private_t *priv;
  const char *buffer;
  xsize_t start, end, peeked;
  xsize_t i;
  xssize_t found_pos;
  int newline_len;
  xsize_t available, checked;
  xboolean_t last_saw_cr;

  priv = stream->priv;

  bstream = G_BUFFERED_INPUT_STREAM (stream);

  checked = *checked_out;
  last_saw_cr = *last_saw_cr_out;
  found_pos = -1;
  newline_len = 0;

  start = checked;
  buffer = (const char*)xbuffered_input_stream_peek_buffer (bstream, &available) + start;
  end = available;
  peeked = end - start;

  for (i = 0; checked < available && i < peeked; i++)
    {
      switch (priv->newline_type)
	{
	case G_DATA_STREAM_NEWLINE_TYPE_LF:
	  if (buffer[i] == 10)
	    {
	      found_pos = start + i;
	      newline_len = 1;
	    }
	  break;
	case G_DATA_STREAM_NEWLINE_TYPE_CR:
	  if (buffer[i] == 13)
	    {
	      found_pos = start + i;
	      newline_len = 1;
	    }
	  break;
	case G_DATA_STREAM_NEWLINE_TYPE_CR_LF:
	  if (last_saw_cr && buffer[i] == 10)
	    {
	      found_pos = start + i - 1;
	      newline_len = 2;
	    }
	  break;
	default:
	case G_DATA_STREAM_NEWLINE_TYPE_ANY:
	  if (buffer[i] == 10) /* LF */
	    {
	      if (last_saw_cr)
		{
		  /* CR LF */
		  found_pos = start + i - 1;
		  newline_len = 2;
		}
	      else
		{
		  /* LF */
		  found_pos = start + i;
		  newline_len = 1;
		}
	    }
	  else if (last_saw_cr)
	    {
	      /* Last was cr, this is not LF, end is CR */
	      found_pos = start + i - 1;
	      newline_len = 1;
	    }
	  /* Don't check for CR here, instead look at last_saw_cr on next byte */
	  break;
	}

      last_saw_cr = (buffer[i] == 13);

      if (found_pos != -1)
	{
	  *newline_len_out = newline_len;
	  return found_pos;
	}
    }

  checked = end;

  *checked_out = checked;
  *last_saw_cr_out = last_saw_cr;
  return -1;
}


/**
 * xdata_input_stream_read_line:
 * @stream: a given #xdata_input_stream_t.
 * @length: (out) (optional): a #xsize_t to get the length of the data read in.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads a line from the data input stream.  Note that no encoding
 * checks or conversion is performed; the input is not guaranteed to
 * be UTF-8, and may in fact have embedded NUL characters.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (nullable) (transfer full) (array zero-terminated=1) (element-type xuint8_t):
 *  a NUL terminated byte array with the line that was read in
 *  (without the newlines).  Set @length to a #xsize_t to get the length
 *  of the read line.  On an error, it will return %NULL and @error
 *  will be set. If there's no content to read, it will still return
 *  %NULL, but @error won't be set.
 **/
char *
xdata_input_stream_read_line (xdata_input_stream_t  *stream,
			       xsize_t             *length,
			       xcancellable_t      *cancellable,
			       xerror_t           **error)
{
  xbuffered_input_stream_t *bstream;
  xsize_t checked;
  xboolean_t last_saw_cr;
  xssize_t found_pos;
  xssize_t res;
  int newline_len;
  char *line;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), NULL);

  bstream = G_BUFFERED_INPUT_STREAM (stream);

  newline_len = 0;
  checked = 0;
  last_saw_cr = FALSE;

  while ((found_pos = scan_for_newline (stream, &checked, &last_saw_cr, &newline_len)) == -1)
    {
      if (xbuffered_input_stream_get_available (bstream) ==
	  xbuffered_input_stream_get_buffer_size (bstream))
	xbuffered_input_stream_set_buffer_size (bstream,
						 2 * xbuffered_input_stream_get_buffer_size (bstream));

      res = xbuffered_input_stream_fill (bstream, -1, cancellable, error);
      if (res < 0)
	return NULL;
      if (res == 0)
	{
	  /* End of stream */
	  if (xbuffered_input_stream_get_available (bstream) == 0)
	    {
	      if (length)
		*length = 0;
	      return NULL;
	    }
	  else
	    {
	      found_pos = checked;
	      newline_len = 0;
	      break;
	    }
	}
    }

  line = g_malloc (found_pos + newline_len + 1);

  res = xinput_stream_read (G_INPUT_STREAM (stream),
			     line,
			     found_pos + newline_len,
			     NULL, NULL);
  if (length)
    *length = (xsize_t)found_pos;
  g_warn_if_fail (res == found_pos + newline_len);
  line[found_pos] = 0;

  return line;
}

/**
 * xdata_input_stream_read_line_utf8:
 * @stream: a given #xdata_input_stream_t.
 * @length: (out) (optional): a #xsize_t to get the length of the data read in.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads a UTF-8 encoded line from the data input stream.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * Returns: (nullable) (transfer full): a NUL terminated UTF-8 string
 *  with the line that was read in (without the newlines).  Set
 *  @length to a #xsize_t to get the length of the read line.  On an
 *  error, it will return %NULL and @error will be set.  For UTF-8
 *  conversion errors, the set error domain is %G_CONVERT_ERROR.  If
 *  there's no content to read, it will still return %NULL, but @error
 *  won't be set.
 *
 * Since: 2.30
 **/
char *
xdata_input_stream_read_line_utf8 (xdata_input_stream_t  *stream,
				    xsize_t             *length,
				    xcancellable_t      *cancellable,
				    xerror_t           **error)
{
  char *res;

  res = xdata_input_stream_read_line (stream, length, cancellable, error);
  if (!res)
    return NULL;

  if (!xutf8_validate (res, -1, NULL))
    {
      g_set_error_literal (error, G_CONVERT_ERROR,
			   G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
			   _("Invalid byte sequence in conversion input"));
      g_free (res);
      return NULL;
    }
  return res;
}

static xssize_t
scan_for_chars (xdata_input_stream_t *stream,
		xsize_t            *checked_out,
		const char       *stop_chars,
                xsize_t             stop_chars_len)
{
  xbuffered_input_stream_t *bstream;
  const char *buffer;
  xsize_t start, end, peeked;
  xsize_t i;
  xsize_t available, checked;
  const char *stop_char;
  const char *stop_end;

  bstream = G_BUFFERED_INPUT_STREAM (stream);
  stop_end = stop_chars + stop_chars_len;

  checked = *checked_out;

  start = checked;
  buffer = (const char *)xbuffered_input_stream_peek_buffer (bstream, &available) + start;
  end = available;
  peeked = end - start;

  for (i = 0; checked < available && i < peeked; i++)
    {
      for (stop_char = stop_chars; stop_char != stop_end; stop_char++)
	{
	  if (buffer[i] == *stop_char)
	    return (start + i);
	}
    }

  checked = end;

  *checked_out = checked;
  return -1;
}

/**
 * xdata_input_stream_read_until:
 * @stream: a given #xdata_input_stream_t.
 * @stop_chars: characters to terminate the read.
 * @length: (out) (optional): a #xsize_t to get the length of the data read in.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: #xerror_t for error reporting.
 *
 * Reads a string from the data input stream, up to the first
 * occurrence of any of the stop characters.
 *
 * Note that, in contrast to xdata_input_stream_read_until_async(),
 * this function consumes the stop character that it finds.
 *
 * Don't use this function in new code.  Its functionality is
 * inconsistent with xdata_input_stream_read_until_async().  Both
 * functions will be marked as deprecated in a future release.  Use
 * xdata_input_stream_read_upto() instead, but note that that function
 * does not consume the stop character.
 *
 * Returns: (transfer full): a string with the data that was read
 *     before encountering any of the stop characters. Set @length to
 *     a #xsize_t to get the length of the string. This function will
 *     return %NULL on an error.
 * Deprecated: 2.56: Use xdata_input_stream_read_upto() instead, which has more
 *     consistent behaviour regarding the stop character.
 */
char *
xdata_input_stream_read_until (xdata_input_stream_t  *stream,
			       const xchar_t        *stop_chars,
			       xsize_t              *length,
			       xcancellable_t       *cancellable,
			       xerror_t            **error)
{
  xbuffered_input_stream_t *bstream;
  xchar_t *result;

  bstream = G_BUFFERED_INPUT_STREAM (stream);

  result = xdata_input_stream_read_upto (stream, stop_chars, -1,
                                          length, cancellable, error);

  /* If we're not at end of stream then we have a stop_char to consume. */
  if (result != NULL && xbuffered_input_stream_get_available (bstream) > 0)
    {
      xsize_t res G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
      xchar_t b;

      res = xinput_stream_read (G_INPUT_STREAM (stream), &b, 1, NULL, NULL);
      xassert (res == 1);
    }

  return result;
}

typedef struct
{
  xboolean_t last_saw_cr;
  xsize_t checked;

  xchar_t *stop_chars;
  xsize_t stop_chars_len;
  xsize_t length;
} GDataInputStreamReadData;

static void
xdata_input_stream_read_complete (xtask_t *task,
                                   xsize_t  read_length,
                                   xsize_t  skip_length)
{
  GDataInputStreamReadData *data = xtask_get_task_data (task);
  xinput_stream_t *stream = xtask_get_source_object (task);
  char *line = NULL;

  if (read_length || skip_length)
    {
      xssize_t bytes;

      data->length = read_length;
      line = g_malloc (read_length + 1);
      line[read_length] = '\0';

      /* we already checked the buffer.  this shouldn't fail. */
      bytes = xinput_stream_read (stream, line, read_length, NULL, NULL);
      g_assert_cmpint (bytes, ==, read_length);

      bytes = xinput_stream_skip (stream, skip_length, NULL, NULL);
      g_assert_cmpint (bytes, ==, skip_length);
    }

  xtask_return_pointer (task, line, g_free);
  xobject_unref (task);
}

static void
xdata_input_stream_read_line_ready (xobject_t      *object,
                                     xasync_result_t *result,
                                     xpointer_t      user_data)
{
  xtask_t *task = user_data;
  GDataInputStreamReadData *data = xtask_get_task_data (task);
  xbuffered_input_stream_t *buffer = xtask_get_source_object (task);
  xssize_t found_pos;
  xint_t newline_len;

  if (result)
    /* this is a callback.  finish the async call. */
    {
      xerror_t *error = NULL;
      xssize_t bytes;

      bytes = xbuffered_input_stream_fill_finish (buffer, result, &error);

      if (bytes <= 0)
        {
          if (bytes < 0)
            /* stream error. */
            {
              xtask_return_error (task, error);
              xobject_unref (task);
              return;
            }

          xdata_input_stream_read_complete (task, data->checked, 0);
          return;
        }

      /* only proceed if we got more bytes... */
    }

  if (data->stop_chars)
    {
      found_pos = scan_for_chars (G_DATA_INPUT_STREAM (buffer),
                                  &data->checked,
                                  data->stop_chars,
                                  data->stop_chars_len);
      newline_len = 0;
    }
  else
    found_pos = scan_for_newline (G_DATA_INPUT_STREAM (buffer), &data->checked,
                                  &data->last_saw_cr, &newline_len);

  if (found_pos == -1)
    /* didn't find a full line; need to buffer some more bytes */
    {
      xsize_t size;

      size = xbuffered_input_stream_get_buffer_size (buffer);

      if (xbuffered_input_stream_get_available (buffer) == size)
        /* need to grow the buffer */
        xbuffered_input_stream_set_buffer_size (buffer, size * 2);

      /* try again */
      xbuffered_input_stream_fill_async (buffer, -1,
                                          xtask_get_priority (task),
                                          xtask_get_cancellable (task),
                                          xdata_input_stream_read_line_ready,
                                          user_data);
    }
  else
    {
      /* read the line and the EOL.  no error is possible. */
      xdata_input_stream_read_complete (task, found_pos, newline_len);
    }
}

static void
xdata_input_stream_read_data_free (xpointer_t user_data)
{
  GDataInputStreamReadData *data = user_data;

  g_free (data->stop_chars);
  g_slice_free (GDataInputStreamReadData, data);
}

static void
xdata_input_stream_read_async (xdata_input_stream_t    *stream,
                                const xchar_t         *stop_chars,
                                xssize_t               stop_chars_len,
                                xint_t                 io_priority,
                                xcancellable_t        *cancellable,
                                xasync_ready_callback_t  callback,
                                xpointer_t             user_data)
{
  GDataInputStreamReadData *data;
  xtask_t *task;
  xsize_t stop_chars_len_unsigned;

  data = g_slice_new0 (GDataInputStreamReadData);

  if (stop_chars_len < 0)
    stop_chars_len_unsigned = strlen (stop_chars);
  else
    stop_chars_len_unsigned = (xsize_t) stop_chars_len;

  data->stop_chars = g_memdup2 (stop_chars, stop_chars_len_unsigned);
  data->stop_chars_len = stop_chars_len_unsigned;
  data->last_saw_cr = FALSE;

  task = xtask_new (stream, cancellable, callback, user_data);
  xtask_set_source_tag (task, xdata_input_stream_read_async);
  xtask_set_task_data (task, data, xdata_input_stream_read_data_free);
  xtask_set_priority (task, io_priority);

  xdata_input_stream_read_line_ready (NULL, NULL, task);
}

static xchar_t *
xdata_input_stream_read_finish (xdata_input_stream_t  *stream,
                                 xasync_result_t      *result,
                                 xsize_t             *length,
                                 xerror_t           **error)
{
  xtask_t *task = XTASK (result);
  xchar_t *line;

  line = xtask_propagate_pointer (task, error);

  if (length && line)
    {
      GDataInputStreamReadData *data = xtask_get_task_data (task);

      *length = data->length;
    }

  return line;
}

/**
 * xdata_input_stream_read_line_async:
 * @stream: a given #xdata_input_stream_t.
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * The asynchronous version of xdata_input_stream_read_line().  It is
 * an error to have two outstanding calls to this function.
 *
 * When the operation is finished, @callback will be called. You
 * can then call xdata_input_stream_read_line_finish() to get
 * the result of the operation.
 *
 * Since: 2.20
 */
void
xdata_input_stream_read_line_async (xdata_input_stream_t    *stream,
                                     xint_t                 io_priority,
                                     xcancellable_t        *cancellable,
                                     xasync_ready_callback_t  callback,
                                     xpointer_t             user_data)
{
  g_return_if_fail (X_IS_DATA_INPUT_STREAM (stream));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  xdata_input_stream_read_async (stream, NULL, 0, io_priority,
                                  cancellable, callback, user_data);
}

/**
 * xdata_input_stream_read_until_async:
 * @stream: a given #xdata_input_stream_t.
 * @stop_chars: characters to terminate the read.
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): callback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * The asynchronous version of xdata_input_stream_read_until().
 * It is an error to have two outstanding calls to this function.
 *
 * Note that, in contrast to xdata_input_stream_read_until(),
 * this function does not consume the stop character that it finds.  You
 * must read it for yourself.
 *
 * When the operation is finished, @callback will be called. You
 * can then call xdata_input_stream_read_until_finish() to get
 * the result of the operation.
 *
 * Don't use this function in new code.  Its functionality is
 * inconsistent with xdata_input_stream_read_until().  Both functions
 * will be marked as deprecated in a future release.  Use
 * xdata_input_stream_read_upto_async() instead.
 *
 * Since: 2.20
 * Deprecated: 2.56: Use xdata_input_stream_read_upto_async() instead, which
 *     has more consistent behaviour regarding the stop character.
 */
void
xdata_input_stream_read_until_async (xdata_input_stream_t    *stream,
                                      const xchar_t         *stop_chars,
                                      xint_t                 io_priority,
                                      xcancellable_t        *cancellable,
                                      xasync_ready_callback_t  callback,
                                      xpointer_t             user_data)
{
  g_return_if_fail (X_IS_DATA_INPUT_STREAM (stream));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_return_if_fail (stop_chars != NULL);

  xdata_input_stream_read_async (stream, stop_chars, -1, io_priority,
                                  cancellable, callback, user_data);
}

/**
 * xdata_input_stream_read_line_finish:
 * @stream: a given #xdata_input_stream_t.
 * @result: the #xasync_result_t that was provided to the callback.
 * @length: (out) (optional): a #xsize_t to get the length of the data read in.
 * @error: #xerror_t for error reporting.
 *
 * Finish an asynchronous call started by
 * xdata_input_stream_read_line_async().  Note the warning about
 * string encoding in xdata_input_stream_read_line() applies here as
 * well.
 *
 * Returns: (nullable) (transfer full) (array zero-terminated=1) (element-type xuint8_t):
 *  a NUL-terminated byte array with the line that was read in
 *  (without the newlines).  Set @length to a #xsize_t to get the length
 *  of the read line.  On an error, it will return %NULL and @error
 *  will be set. If there's no content to read, it will still return
 *  %NULL, but @error won't be set.
 *
 * Since: 2.20
 */
xchar_t *
xdata_input_stream_read_line_finish (xdata_input_stream_t  *stream,
                                      xasync_result_t      *result,
                                      xsize_t             *length,
                                      xerror_t           **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, stream), NULL);

  return xdata_input_stream_read_finish (stream, result, length, error);
}

/**
 * xdata_input_stream_read_line_finish_utf8:
 * @stream: a given #xdata_input_stream_t.
 * @result: the #xasync_result_t that was provided to the callback.
 * @length: (out) (optional): a #xsize_t to get the length of the data read in.
 * @error: #xerror_t for error reporting.
 *
 * Finish an asynchronous call started by
 * xdata_input_stream_read_line_async().
 *
 * Returns: (nullable) (transfer full): a string with the line that
 *  was read in (without the newlines).  Set @length to a #xsize_t to
 *  get the length of the read line.  On an error, it will return
 *  %NULL and @error will be set. For UTF-8 conversion errors, the set
 *  error domain is %G_CONVERT_ERROR.  If there's no content to read,
 *  it will still return %NULL, but @error won't be set.
 *
 * Since: 2.30
 */
xchar_t *
xdata_input_stream_read_line_finish_utf8 (xdata_input_stream_t  *stream,
					   xasync_result_t      *result,
					   xsize_t             *length,
					   xerror_t           **error)
{
  xchar_t *res;

  res = xdata_input_stream_read_line_finish (stream, result, length, error);
  if (!res)
    return NULL;

  if (!xutf8_validate (res, -1, NULL))
    {
      g_set_error_literal (error, G_CONVERT_ERROR,
			   G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
			   _("Invalid byte sequence in conversion input"));
      g_free (res);
      return NULL;
    }
  return res;
}

/**
 * xdata_input_stream_read_until_finish:
 * @stream: a given #xdata_input_stream_t.
 * @result: the #xasync_result_t that was provided to the callback.
 * @length: (out) (optional): a #xsize_t to get the length of the data read in.
 * @error: #xerror_t for error reporting.
 *
 * Finish an asynchronous call started by
 * xdata_input_stream_read_until_async().
 *
 * Since: 2.20
 *
 * Returns: (transfer full): a string with the data that was read
 *     before encountering any of the stop characters. Set @length to
 *     a #xsize_t to get the length of the string. This function will
 *     return %NULL on an error.
 * Deprecated: 2.56: Use xdata_input_stream_read_upto_finish() instead, which
 *     has more consistent behaviour regarding the stop character.
 */
xchar_t *
xdata_input_stream_read_until_finish (xdata_input_stream_t  *stream,
                                       xasync_result_t      *result,
                                       xsize_t             *length,
                                       xerror_t           **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, stream), NULL);

  return xdata_input_stream_read_finish (stream, result, length, error);
}

/**
 * xdata_input_stream_read_upto:
 * @stream: a #xdata_input_stream_t
 * @stop_chars: characters to terminate the read
 * @stop_chars_len: length of @stop_chars. May be -1 if @stop_chars is
 *     nul-terminated
 * @length: (out) (optional): a #xsize_t to get the length of the data read in
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @error: #xerror_t for error reporting
 *
 * Reads a string from the data input stream, up to the first
 * occurrence of any of the stop characters.
 *
 * In contrast to xdata_input_stream_read_until(), this function
 * does not consume the stop character. You have to use
 * xdata_input_stream_read_byte() to get it before calling
 * xdata_input_stream_read_upto() again.
 *
 * Note that @stop_chars may contain '\0' if @stop_chars_len is
 * specified.
 *
 * The returned string will always be nul-terminated on success.
 *
 * Returns: (transfer full): a string with the data that was read
 *     before encountering any of the stop characters. Set @length to
 *     a #xsize_t to get the length of the string. This function will
 *     return %NULL on an error
 *
 * Since: 2.26
 */
char *
xdata_input_stream_read_upto (xdata_input_stream_t  *stream,
                               const xchar_t       *stop_chars,
                               xssize_t             stop_chars_len,
                               xsize_t             *length,
                               xcancellable_t      *cancellable,
                               xerror_t           **error)
{
  xbuffered_input_stream_t *bstream;
  xsize_t checked;
  xssize_t found_pos;
  xssize_t res;
  char *data_until;
  xsize_t stop_chars_len_unsigned;

  xreturn_val_if_fail (X_IS_DATA_INPUT_STREAM (stream), NULL);

  if (stop_chars_len < 0)
    stop_chars_len_unsigned = strlen (stop_chars);
  else
    stop_chars_len_unsigned = (xsize_t) stop_chars_len;

  bstream = G_BUFFERED_INPUT_STREAM (stream);

  checked = 0;

  while ((found_pos = scan_for_chars (stream, &checked, stop_chars, stop_chars_len_unsigned)) == -1)
    {
      if (xbuffered_input_stream_get_available (bstream) ==
          xbuffered_input_stream_get_buffer_size (bstream))
        xbuffered_input_stream_set_buffer_size (bstream,
                                                 2 * xbuffered_input_stream_get_buffer_size (bstream));

      res = xbuffered_input_stream_fill (bstream, -1, cancellable, error);
      if (res < 0)
        return NULL;
      if (res == 0)
        {
          /* End of stream */
          if (xbuffered_input_stream_get_available (bstream) == 0)
            {
              if (length)
                *length = 0;
              return NULL;
            }
          else
            {
              found_pos = checked;
              break;
            }
        }
    }

  data_until = g_malloc (found_pos + 1);

  res = xinput_stream_read (G_INPUT_STREAM (stream),
                             data_until,
                             found_pos,
                             NULL, NULL);
  if (length)
    *length = (xsize_t)found_pos;
  g_warn_if_fail (res == found_pos);
  data_until[found_pos] = 0;

  return data_until;
}

/**
 * xdata_input_stream_read_upto_async:
 * @stream: a #xdata_input_stream_t
 * @stop_chars: characters to terminate the read
 * @stop_chars_len: length of @stop_chars. May be -1 if @stop_chars is
 *     nul-terminated
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @callback: (scope async): callback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * The asynchronous version of xdata_input_stream_read_upto().
 * It is an error to have two outstanding calls to this function.
 *
 * In contrast to xdata_input_stream_read_until(), this function
 * does not consume the stop character. You have to use
 * xdata_input_stream_read_byte() to get it before calling
 * xdata_input_stream_read_upto() again.
 *
 * Note that @stop_chars may contain '\0' if @stop_chars_len is
 * specified.
 *
 * When the operation is finished, @callback will be called. You
 * can then call xdata_input_stream_read_upto_finish() to get
 * the result of the operation.
 *
 * Since: 2.26
 */
void
xdata_input_stream_read_upto_async (xdata_input_stream_t    *stream,
                                     const xchar_t         *stop_chars,
                                     xssize_t               stop_chars_len,
                                     xint_t                 io_priority,
                                     xcancellable_t        *cancellable,
                                     xasync_ready_callback_t  callback,
                                     xpointer_t             user_data)
{
  g_return_if_fail (X_IS_DATA_INPUT_STREAM (stream));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_return_if_fail (stop_chars != NULL);

  xdata_input_stream_read_async (stream, stop_chars, stop_chars_len, io_priority,
                                  cancellable, callback, user_data);
}

/**
 * xdata_input_stream_read_upto_finish:
 * @stream: a #xdata_input_stream_t
 * @result: the #xasync_result_t that was provided to the callback
 * @length: (out) (optional): a #xsize_t to get the length of the data read in
 * @error: #xerror_t for error reporting
 *
 * Finish an asynchronous call started by
 * xdata_input_stream_read_upto_async().
 *
 * Note that this function does not consume the stop character. You
 * have to use xdata_input_stream_read_byte() to get it before calling
 * xdata_input_stream_read_upto_async() again.
 *
 * The returned string will always be nul-terminated on success.
 *
 * Returns: (transfer full): a string with the data that was read
 *     before encountering any of the stop characters. Set @length to
 *     a #xsize_t to get the length of the string. This function will
 *     return %NULL on an error.
 *
 * Since: 2.24
 */
xchar_t *
xdata_input_stream_read_upto_finish (xdata_input_stream_t  *stream,
                                      xasync_result_t      *result,
                                      xsize_t             *length,
                                      xerror_t           **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, stream), NULL);

  return xdata_input_stream_read_finish (stream, result, length, error);
}
