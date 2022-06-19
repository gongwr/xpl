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
#include "gdataoutputstream.h"
#include "gseekable.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "glibintl.h"


/**
 * SECTION:gdataoutputstream
 * @short_description: Data Output Stream
 * @include: gio/gio.h
 * @see_also: #xoutput_stream_t
 *
 * Data output stream implements #xoutput_stream_t and includes functions for
 * writing data directly to an output stream.
 *
 **/



struct _xdata_output_stream_private {
  xdata_stream_byte_order_t byte_order;
};

enum {
  PROP_0,
  PROP_BYTE_ORDER
};

static void xdata_output_stream_set_property (xobject_t      *object,
					       xuint_t         prop_id,
					       const xvalue_t *value,
					       xparam_spec_t   *pspec);
static void xdata_output_stream_get_property (xobject_t      *object,
					       xuint_t         prop_id,
					       xvalue_t       *value,
					       xparam_spec_t   *pspec);

static void     xdata_output_stream_seekable_iface_init (xseekable_iface_t  *iface);
static xoffset_t  xdata_output_stream_tell                (xseekable__t       *seekable);
static xboolean_t xdata_output_stream_can_seek            (xseekable__t       *seekable);
static xboolean_t xdata_output_stream_seek                (xseekable__t       *seekable,
							  xoffset_t          offset,
							  xseek_type_t        type,
							  xcancellable_t    *cancellable,
							  xerror_t         **error);
static xboolean_t xdata_output_stream_can_truncate        (xseekable__t       *seekable);
static xboolean_t xdata_output_stream_truncate            (xseekable__t       *seekable,
							  xoffset_t          offset,
							  xcancellable_t    *cancellable,
							  xerror_t         **error);

G_DEFINE_TYPE_WITH_CODE (xdata_output_stream,
			 xdata_output_stream,
			 XTYPE_FILTER_OUTPUT_STREAM,
                         G_ADD_PRIVATE (xdata_output_stream)
			 G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
						xdata_output_stream_seekable_iface_init))


static void
xdata_output_stream_class_init (xdata_output_stream_class_t *klass)
{
  xobject_class_t *object_class;

  object_class = XOBJECT_CLASS (klass);
  object_class->get_property = xdata_output_stream_get_property;
  object_class->set_property = xdata_output_stream_set_property;

  /**
   * xdata_output_stream_t:byte-order:
   *
   * Determines the byte ordering that is used when writing
   * multi-byte entities (such as integers) to the stream.
   */
  xobject_class_install_property (object_class,
                                   PROP_BYTE_ORDER,
                                   xparam_spec_enum ("byte-order",
                                                      P_("Byte order"),
                                                      P_("The byte order"),
                                                      XTYPE_DATA_STREAM_BYTE_ORDER,
                                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN,
                                                      XPARAM_READWRITE|XPARAM_STATIC_NAME|XPARAM_STATIC_BLURB));

}

static void
xdata_output_stream_set_property (xobject_t     *object,
				  xuint_t         prop_id,
				  const xvalue_t *value,
				  xparam_spec_t   *pspec)
{
  xdata_output_stream_t *dstream;

  dstream = G_DATA_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_BYTE_ORDER:
      xdata_output_stream_set_byte_order (dstream, xvalue_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdata_output_stream_get_property (xobject_t    *object,
				   xuint_t       prop_id,
				   xvalue_t     *value,
				   xparam_spec_t *pspec)
{
  xdata_output_stream_private_t *priv;
  xdata_output_stream_t        *dstream;

  dstream = G_DATA_OUTPUT_STREAM (object);
  priv = dstream->priv;

  switch (prop_id)
    {
    case PROP_BYTE_ORDER:
      xvalue_set_enum (value, priv->byte_order);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdata_output_stream_init (xdata_output_stream_t *stream)
{
  stream->priv = xdata_output_stream_get_instance_private (stream);
  stream->priv->byte_order = G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN;
}

static void
xdata_output_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell         = xdata_output_stream_tell;
  iface->can_seek     = xdata_output_stream_can_seek;
  iface->seek         = xdata_output_stream_seek;
  iface->can_truncate = xdata_output_stream_can_truncate;
  iface->truncate_fn  = xdata_output_stream_truncate;
}

/**
 * xdata_output_stream_new:
 * @base_stream: a #xoutput_stream_t.
 *
 * Creates a new data output stream for @base_stream.
 *
 * Returns: #xdata_output_stream_t.
 **/
xdata_output_stream_t *
xdata_output_stream_new (xoutput_stream_t *base_stream)
{
  xdata_output_stream_t *stream;

  xreturn_val_if_fail (X_IS_OUTPUT_STREAM (base_stream), NULL);

  stream = xobject_new (XTYPE_DATA_OUTPUT_STREAM,
                         "base-stream", base_stream,
                         NULL);

  return stream;
}

/**
 * xdata_output_stream_set_byte_order:
 * @stream: a #xdata_output_stream_t.
 * @order: a %xdata_stream_byte_order_t.
 *
 * Sets the byte order of the data output stream to @order.
 **/
void
xdata_output_stream_set_byte_order (xdata_output_stream_t    *stream,
                                     xdata_stream_byte_order_t  order)
{
  xdata_output_stream_private_t *priv;
  g_return_if_fail (X_IS_DATA_OUTPUT_STREAM (stream));
  priv = stream->priv;
  if (priv->byte_order != order)
    {
      priv->byte_order = order;
      xobject_notify (G_OBJECT (stream), "byte-order");
    }
}

/**
 * xdata_output_stream_get_byte_order:
 * @stream: a #xdata_output_stream_t.
 *
 * Gets the byte order for the stream.
 *
 * Returns: the #xdata_stream_byte_order_t for the @stream.
 **/
xdata_stream_byte_order_t
xdata_output_stream_get_byte_order (xdata_output_stream_t *stream)
{
  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN);

  return stream->priv->byte_order;
}

/**
 * xdata_output_stream_put_byte:
 * @stream: a #xdata_output_stream_t.
 * @data: a #xuchar_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts a byte into the output stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_byte (xdata_output_stream_t  *stream,
			       xuchar_t              data,
			       xcancellable_t       *cancellable,
			       xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 1,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_int16:
 * @stream: a #xdata_output_stream_t.
 * @data: a #gint16.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts a signed 16-bit integer into the output stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_int16 (xdata_output_stream_t  *stream,
				gint16              data,
				xcancellable_t       *cancellable,
				xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GINT16_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GINT16_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 2,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_uint16:
 * @stream: a #xdata_output_stream_t.
 * @data: a #xuint16_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts an unsigned 16-bit integer into the output stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_uint16 (xdata_output_stream_t  *stream,
				 xuint16_t             data,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GUINT16_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GUINT16_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 2,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_int32:
 * @stream: a #xdata_output_stream_t.
 * @data: a #gint32.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts a signed 32-bit integer into the output stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_int32 (xdata_output_stream_t  *stream,
				gint32              data,
				xcancellable_t       *cancellable,
				xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GINT32_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GINT32_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 4,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_uint32:
 * @stream: a #xdata_output_stream_t.
 * @data: a #xuint32_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts an unsigned 32-bit integer into the stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_uint32 (xdata_output_stream_t  *stream,
				 xuint32_t             data,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GUINT32_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GUINT32_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 4,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_int64:
 * @stream: a #xdata_output_stream_t.
 * @data: a #sint64_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts a signed 64-bit integer into the stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_int64 (xdata_output_stream_t  *stream,
				sint64_t              data,
				xcancellable_t       *cancellable,
				xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GINT64_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GINT64_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 8,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_uint64:
 * @stream: a #xdata_output_stream_t.
 * @data: a #xuint64_t.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts an unsigned 64-bit integer into the stream.
 *
 * Returns: %TRUE if @data was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_uint64 (xdata_output_stream_t  *stream,
				 xuint64_t             data,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);

  switch (stream->priv->byte_order)
    {
    case G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN:
      data = GUINT64_TO_BE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN:
      data = GUINT64_TO_LE (data);
      break;
    case G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN:
    default:
      break;
    }

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    &data, 8,
				    &bytes_written,
				    cancellable, error);
}

/**
 * xdata_output_stream_put_string:
 * @stream: a #xdata_output_stream_t.
 * @str: a string.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: a #xerror_t, %NULL to ignore.
 *
 * Puts a string into the output stream.
 *
 * Returns: %TRUE if @string was successfully added to the @stream.
 **/
xboolean_t
xdata_output_stream_put_string (xdata_output_stream_t  *stream,
				 const char         *str,
				 xcancellable_t       *cancellable,
				 xerror_t            **error)
{
  xsize_t bytes_written;

  xreturn_val_if_fail (X_IS_DATA_OUTPUT_STREAM (stream), FALSE);
  xreturn_val_if_fail (str != NULL, FALSE);

  return xoutput_stream_write_all (G_OUTPUT_STREAM (stream),
				    str, strlen (str),
				    &bytes_written,
				    cancellable, error);
}

static xoffset_t
xdata_output_stream_tell (xseekable__t *seekable)
{
  xoutput_stream_t *base_stream;
  xseekable__t *base_stream_seekable;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    return 0;
  base_stream_seekable = G_SEEKABLE (base_stream);
  return xseekable_tell (base_stream_seekable);
}

static xboolean_t
xdata_output_stream_can_seek (xseekable__t *seekable)
{
  xoutput_stream_t *base_stream;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  return X_IS_SEEKABLE (base_stream) && xseekable_can_seek (G_SEEKABLE (base_stream));
}

static xboolean_t
xdata_output_stream_seek (xseekable__t     *seekable,
			   xoffset_t        offset,
			   xseek_type_t      type,
			   xcancellable_t  *cancellable,
			   xerror_t       **error)
{
  xoutput_stream_t *base_stream;
  xseekable__t *base_stream_seekable;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Seek not supported on base stream"));
      return FALSE;
    }

  base_stream_seekable = G_SEEKABLE (base_stream);
  return xseekable_seek (base_stream_seekable, offset, type, cancellable, error);
}

static xboolean_t
xdata_output_stream_can_truncate (xseekable__t *seekable)
{
  xoutput_stream_t *base_stream;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  return X_IS_SEEKABLE (base_stream) && xseekable_can_truncate (G_SEEKABLE (base_stream));
}

static xboolean_t
xdata_output_stream_truncate (xseekable__t     *seekable,
				   xoffset_t        offset,
				   xcancellable_t  *cancellable,
				   xerror_t       **error)
{
  xoutput_stream_t *base_stream;
  xseekable__t *base_stream_seekable;

  base_stream = G_FILTER_OUTPUT_STREAM (seekable)->base_stream;
  if (!X_IS_SEEKABLE (base_stream))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           _("Truncate not supported on base stream"));
      return FALSE;
    }

  base_stream_seekable = G_SEEKABLE (base_stream);
  return xseekable_truncate (base_stream_seekable, offset, cancellable, error);
}
