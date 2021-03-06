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

#include "gzlibdecompressor.h"

#include <errno.h>
#include <zlib.h>
#include <string.h>

#include "gfileinfo.h"
#include "gioerror.h"
#include "gioenums.h"
#include "gioenumtypes.h"
#include "glibintl.h"


enum {
  PROP_0,
  PROP_FORMAT,
  PROP_FILE_INFO
};

/**
 * SECTION:gzlibdecompressor
 * @short_description: Zlib decompressor
 * @include: gio/gio.h
 *
 * #xzlib_decompressor_t is an implementation of #xconverter_t that
 * decompresses data compressed with zlib.
 */

static void g_zlib_decompressor_iface_init          (xconverter_iface_t *iface);

typedef struct {
  gz_header gzheader;
  char filename[257];
  xfile_info_t *file_info;
} HeaderData;

/**
 * xzlib_decompressor_t:
 *
 * Zlib decompression
 */
struct _GZlibDecompressor
{
  xobject_t parent_instance;

  GZlibCompressorFormat format;
  z_stream zstream;
  HeaderData *header_data;
};

static void
g_zlib_decompressor_set_gzheader (xzlib_decompressor_t *decompressor)
{
  /* On win32, these functions were not exported before 1.2.4 */
#if !defined (G_OS_WIN32) || ZLIB_VERNUM >= 0x1240
  if (decompressor->format != G_ZLIB_COMPRESSOR_FORMAT_GZIP)
    return;

  if (decompressor->header_data != NULL)
    {
      if (decompressor->header_data->file_info)
        xobject_unref (decompressor->header_data->file_info);

      memset (decompressor->header_data, 0, sizeof (HeaderData));
    }
  else
    {
      decompressor->header_data = g_new0 (HeaderData, 1);
    }

  decompressor->header_data->gzheader.name = (Bytef*) &decompressor->header_data->filename;
  /* We keep one byte to guarantee the string is 0-terminated */
  decompressor->header_data->gzheader.name_max = 256;

  if (inflateGetHeader (&decompressor->zstream, &decompressor->header_data->gzheader) != Z_OK)
    g_warning ("unexpected zlib error: %s", decompressor->zstream.msg);
#endif /* !G_OS_WIN32 || ZLIB >= 1.2.4 */
}

G_DEFINE_TYPE_WITH_CODE (xzlib_decompressor_t, g_zlib_decompressor, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_CONVERTER,
						g_zlib_decompressor_iface_init))

static void
g_zlib_decompressor_finalize (xobject_t *object)
{
  xzlib_decompressor_t *decompressor;

  decompressor = G_ZLIB_DECOMPRESSOR (object);

  inflateEnd (&decompressor->zstream);

  if (decompressor->header_data != NULL)
    {
      if (decompressor->header_data->file_info)
        xobject_unref (decompressor->header_data->file_info);
      g_free (decompressor->header_data);
    }

  XOBJECT_CLASS (g_zlib_decompressor_parent_class)->finalize (object);
}


static void
g_zlib_decompressor_set_property (xobject_t      *object,
				  xuint_t         prop_id,
				  const xvalue_t *value,
				  xparam_spec_t   *pspec)
{
  xzlib_decompressor_t *decompressor;

  decompressor = G_ZLIB_DECOMPRESSOR (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      decompressor->format = xvalue_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_zlib_decompressor_get_property (xobject_t    *object,
				  xuint_t       prop_id,
				  xvalue_t     *value,
				  xparam_spec_t *pspec)
{
  xzlib_decompressor_t *decompressor;

  decompressor = G_ZLIB_DECOMPRESSOR (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      xvalue_set_enum (value, decompressor->format);
      break;

    case PROP_FILE_INFO:
      if (decompressor->header_data)
        xvalue_set_object (value, decompressor->header_data->file_info);
      else
        xvalue_set_object (value, NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_zlib_decompressor_init (xzlib_decompressor_t *decompressor)
{
}

static void
g_zlib_decompressor_constructed (xobject_t *object)
{
  xzlib_decompressor_t *decompressor;
  int res;

  decompressor = G_ZLIB_DECOMPRESSOR (object);

  if (decompressor->format == G_ZLIB_COMPRESSOR_FORMAT_GZIP)
    {
      /* + 16 for gzip */
      res = inflateInit2 (&decompressor->zstream, MAX_WBITS + 16);
    }
  else if (decompressor->format == G_ZLIB_COMPRESSOR_FORMAT_RAW)
    {
      /* Negative for raw */
      res = inflateInit2 (&decompressor->zstream, -MAX_WBITS);
    }
  else /* ZLIB */
    res = inflateInit (&decompressor->zstream);

  if (res == Z_MEM_ERROR )
    xerror ("xzlib_decompressor_t: Not enough memory for zlib use");

  if (res != Z_OK)
    g_warning ("unexpected zlib error: %s", decompressor->zstream.msg);

  g_zlib_decompressor_set_gzheader (decompressor);
}

static void
g_zlib_decompressor_class_init (GZlibDecompressorClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = g_zlib_decompressor_finalize;
  xobject_class->constructed = g_zlib_decompressor_constructed;
  xobject_class->get_property = g_zlib_decompressor_get_property;
  xobject_class->set_property = g_zlib_decompressor_set_property;

  xobject_class_install_property (xobject_class,
				   PROP_FORMAT,
				   xparam_spec_enum ("format",
						      P_("compression format"),
						      P_("The format of the compressed data"),
						      XTYPE_ZLIB_COMPRESSOR_FORMAT,
						      G_ZLIB_COMPRESSOR_FORMAT_ZLIB,
						      XPARAM_READWRITE | XPARAM_CONSTRUCT_ONLY |
						      XPARAM_STATIC_STRINGS));

  /**
   * xzlib_decompressor_t:file-info:
   *
   * A #xfile_info_t containing the information found in the GZIP header
   * of the data stream processed, or %NULL if the header was not yet
   * fully processed, is not present at all, or the compressor's
   * #xzlib_decompressor_t:format property is not %G_ZLIB_COMPRESSOR_FORMAT_GZIP.
   *
   * Since: 2.26
   */
  xobject_class_install_property (xobject_class,
                                   PROP_FILE_INFO,
                                   xparam_spec_object ("file-info",
                                                       P_("file info"),
                                                       P_("File info"),
                                                       XTYPE_FILE_INFO,
                                                       XPARAM_READABLE |
                                                       XPARAM_STATIC_STRINGS));
}

/**
 * g_zlib_decompressor_new:
 * @format: The format to use for the compressed data
 *
 * Creates a new #xzlib_decompressor_t.
 *
 * Returns: a new #xzlib_decompressor_t
 *
 * Since: 2.24
 **/
xzlib_decompressor_t *
g_zlib_decompressor_new (GZlibCompressorFormat format)
{
  xzlib_decompressor_t *decompressor;

  decompressor = xobject_new (XTYPE_ZLIB_DECOMPRESSOR,
			       "format", format,
			       NULL);

  return decompressor;
}

/**
 * g_zlib_decompressor_get_file_info:
 * @decompressor: a #xzlib_decompressor_t
 *
 * Retrieves the #xfile_info_t constructed from the GZIP header data
 * of compressed data processed by @compressor, or %NULL if @decompressor's
 * #xzlib_decompressor_t:format property is not %G_ZLIB_COMPRESSOR_FORMAT_GZIP,
 * or the header data was not fully processed yet, or it not present in the
 * data stream at all.
 *
 * Returns: (nullable) (transfer none): a #xfile_info_t, or %NULL
 *
 * Since: 2.26
 */
xfile_info_t *
g_zlib_decompressor_get_file_info (xzlib_decompressor_t *decompressor)
{
  xreturn_val_if_fail (X_IS_ZLIB_DECOMPRESSOR (decompressor), NULL);

  if (decompressor->header_data)
    return decompressor->header_data->file_info;

  return NULL;
}

static void
g_zlib_decompressor_reset (xconverter_t *converter)
{
  xzlib_decompressor_t *decompressor = G_ZLIB_DECOMPRESSOR (converter);
  int res;

  res = inflateReset (&decompressor->zstream);
  if (res != Z_OK)
    g_warning ("unexpected zlib error: %s", decompressor->zstream.msg);

  g_zlib_decompressor_set_gzheader (decompressor);
}

static xconverter_result_t
g_zlib_decompressor_convert (xconverter_t *converter,
			     const void *inbuf,
			     xsize_t       inbuf_size,
			     void       *outbuf,
			     xsize_t       outbuf_size,
			     xconverter_flags_t flags,
			     xsize_t      *bytes_read,
			     xsize_t      *bytes_written,
			     xerror_t    **error)
{
  xzlib_decompressor_t *decompressor;
  int res;

  decompressor = G_ZLIB_DECOMPRESSOR (converter);

  decompressor->zstream.next_in = (void *)inbuf;
  decompressor->zstream.avail_in = inbuf_size;

  decompressor->zstream.next_out = outbuf;
  decompressor->zstream.avail_out = outbuf_size;

  res = inflate (&decompressor->zstream, Z_NO_FLUSH);

  if (res == Z_DATA_ERROR || res == Z_NEED_DICT)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
			   _("Invalid compressed data"));
      return XCONVERTER_ERROR;
    }

  if (res == Z_MEM_ERROR)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
			   _("Not enough memory"));
      return XCONVERTER_ERROR;
    }

    if (res == Z_STREAM_ERROR)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
		   _("Internal error: %s"), decompressor->zstream.msg);
      return XCONVERTER_ERROR;
    }

    if (res == Z_BUF_ERROR)
      {
	if (flags & XCONVERTER_FLUSH)
	  return XCONVERTER_FLUSHED;

	/* Z_FINISH not set, so this means no progress could be made */
	/* We do have output space, so this should only happen if we
	   have no input but need some */

	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT,
			     _("Need more input"));
	return XCONVERTER_ERROR;
      }

  xassert (res == Z_OK || res == Z_STREAM_END);

  *bytes_read = inbuf_size - decompressor->zstream.avail_in;
  *bytes_written = outbuf_size - decompressor->zstream.avail_out;

#if !defined (G_OS_WIN32) || ZLIB_VERNUM >= 0x1240
  if (decompressor->header_data != NULL &&
      decompressor->header_data->gzheader.done == 1)
    {
      HeaderData *data = decompressor->header_data;

      /* So we don't notify again */
      data->gzheader.done = 2;

      data->file_info = xfile_info_new ();
      xfile_info_set_attribute_uint64 (data->file_info,
                                        XFILE_ATTRIBUTE_TIME_MODIFIED,
                                        data->gzheader.time);
      xfile_info_set_attribute_uint32 (data->file_info,
                                        XFILE_ATTRIBUTE_TIME_MODIFIED_USEC,
                                        0);

      if (data->filename[0] != '\0')
        xfile_info_set_attribute_byte_string (data->file_info,
                                               XFILE_ATTRIBUTE_STANDARD_NAME,
                                               data->filename);

      xobject_notify (G_OBJECT (decompressor), "file-info");
    }
#endif /* !G_OS_WIN32 || ZLIB >= 1.2.4 */

  if (res == Z_STREAM_END)
    return XCONVERTER_FINISHED;
  return XCONVERTER_CONVERTED;
}

static void
g_zlib_decompressor_iface_init (xconverter_iface_t *iface)
{
  iface->convert = g_zlib_decompressor_convert;
  iface->reset = g_zlib_decompressor_reset;
}
