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

#include "gzlibcompressor.h"

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
  PROP_LEVEL,
  PROP_FILE_INFO
};

/**
 * SECTION:gzlibcompressor
 * @short_description: Zlib compressor
 * @include: gio/gio.h
 *
 * #xzlib_compressor_t is an implementation of #xconverter_t that
 * compresses data using zlib.
 */

static void g_zlib_compressor_iface_init          (xconverter_iface_t *iface);

/**
 * xzlib_compressor_t:
 *
 * Zlib decompression
 */
struct _GZlibCompressor
{
  xobject_t parent_instance;

  GZlibCompressorFormat format;
  int level;
  z_stream zstream;
  gz_header gzheader;
  xfile_info_t *file_info;
};

static void
g_zlib_compressor_set_gzheader (xzlib_compressor_t *compressor)
{
  /* On win32, these functions were not exported before 1.2.4 */
#if !defined (G_OS_WIN32) || ZLIB_VERNUM >= 0x1240
  const xchar_t *filename;

  if (compressor->format != G_ZLIB_COMPRESSOR_FORMAT_GZIP ||
      compressor->file_info == NULL)
    return;

  memset (&compressor->gzheader, 0, sizeof (gz_header));
  compressor->gzheader.os = 0x03; /* Unix */

  filename = xfile_info_get_name (compressor->file_info);
  compressor->gzheader.name = (Bytef *) filename;
  compressor->gzheader.name_max = filename ? strlen (filename) + 1 : 0;

  compressor->gzheader.time =
      (uLong) xfile_info_get_attribute_uint64 (compressor->file_info,
                                                XFILE_ATTRIBUTE_TIME_MODIFIED);

  if (deflateSetHeader (&compressor->zstream, &compressor->gzheader) != Z_OK)
    g_warning ("unexpected zlib error: %s", compressor->zstream.msg);
#endif /* !G_OS_WIN32 || ZLIB >= 1.2.4 */
}

G_DEFINE_TYPE_WITH_CODE (xzlib_compressor, g_zlib_compressor, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_CONVERTER,
						g_zlib_compressor_iface_init))

static void
g_zlib_compressor_finalize (xobject_t *object)
{
  xzlib_compressor_t *compressor;

  compressor = G_ZLIB_COMPRESSOR (object);

  deflateEnd (&compressor->zstream);

  if (compressor->file_info)
    xobject_unref (compressor->file_info);

  XOBJECT_CLASS (g_zlib_compressor_parent_class)->finalize (object);
}


static void
g_zlib_compressor_set_property (xobject_t      *object,
				  xuint_t         prop_id,
				  const xvalue_t *value,
				  xparam_spec_t   *pspec)
{
  xzlib_compressor_t *compressor;

  compressor = G_ZLIB_COMPRESSOR (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      compressor->format = xvalue_get_enum (value);
      break;

    case PROP_LEVEL:
      compressor->level = xvalue_get_int (value);
      break;

    case PROP_FILE_INFO:
      g_zlib_compressor_set_file_info (compressor, xvalue_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_zlib_compressor_get_property (xobject_t    *object,
				  xuint_t       prop_id,
				  xvalue_t     *value,
				  xparam_spec_t *pspec)
{
  xzlib_compressor_t *compressor;

  compressor = G_ZLIB_COMPRESSOR (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      xvalue_set_enum (value, compressor->format);
      break;

    case PROP_LEVEL:
      xvalue_set_int (value, compressor->level);
      break;

    case PROP_FILE_INFO:
      xvalue_set_object (value, compressor->file_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_zlib_compressor_init (xzlib_compressor_t *compressor)
{
}

static void
g_zlib_compressor_constructed (xobject_t *object)
{
  xzlib_compressor_t *compressor;
  int res;

  compressor = G_ZLIB_COMPRESSOR (object);

  if (compressor->format == G_ZLIB_COMPRESSOR_FORMAT_GZIP)
    {
      /* + 16 for gzip */
      res = deflateInit2 (&compressor->zstream,
			  compressor->level, Z_DEFLATED,
			  MAX_WBITS + 16, 8,
			  Z_DEFAULT_STRATEGY);
    }
  else if (compressor->format == G_ZLIB_COMPRESSOR_FORMAT_RAW)
    {
      /* negative wbits for raw */
      res = deflateInit2 (&compressor->zstream,
			  compressor->level, Z_DEFLATED,
			  -MAX_WBITS, 8,
			  Z_DEFAULT_STRATEGY);
    }
  else /* ZLIB */
    res = deflateInit (&compressor->zstream, compressor->level);

  if (res == Z_MEM_ERROR )
    xerror ("xzlib_compressor_t: Not enough memory for zlib use");

  if (res != Z_OK)
    g_warning ("unexpected zlib error: %s", compressor->zstream.msg);

  g_zlib_compressor_set_gzheader (compressor);
}

static void
g_zlib_compressor_class_init (GZlibCompressorClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = g_zlib_compressor_finalize;
  xobject_class->constructed = g_zlib_compressor_constructed;
  xobject_class->get_property = g_zlib_compressor_get_property;
  xobject_class->set_property = g_zlib_compressor_set_property;

  xobject_class_install_property (xobject_class,
				   PROP_FORMAT,
				   xparam_spec_enum ("format",
						      P_("compression format"),
						      P_("The format of the compressed data"),
						      XTYPE_ZLIB_COMPRESSOR_FORMAT,
						      G_ZLIB_COMPRESSOR_FORMAT_ZLIB,
						      XPARAM_READWRITE | XPARAM_CONSTRUCT_ONLY |
						      XPARAM_STATIC_STRINGS));
  xobject_class_install_property (xobject_class,
				   PROP_LEVEL,
				   xparam_spec_int ("level",
						     P_("compression level"),
						     P_("The level of compression from 0 (no compression) to 9 (most compression), -1 for the default level"),
						     -1, 9,
						     -1,
						     XPARAM_READWRITE |
						     XPARAM_CONSTRUCT_ONLY |
						     XPARAM_STATIC_STRINGS));

  /**
   * xzlib_compressor_t:file-info:
   *
   * If set to a non-%NULL #xfile_info_t object, and #xzlib_compressor_t:format is
   * %G_ZLIB_COMPRESSOR_FORMAT_GZIP, the compressor will write the file name
   * and modification time from the file info to the GZIP header.
   *
   * Since: 2.26
   */
  xobject_class_install_property (xobject_class,
                                   PROP_FILE_INFO,
                                   xparam_spec_object ("file-info",
                                                       P_("file info"),
                                                       P_("File info"),
                                                       XTYPE_FILE_INFO,
                                                       XPARAM_READWRITE |
                                                       XPARAM_STATIC_STRINGS));
}

/**
 * g_zlib_compressor_new:
 * @format: The format to use for the compressed data
 * @level: compression level (0-9), -1 for default
 *
 * Creates a new #xzlib_compressor_t.
 *
 * Returns: a new #xzlib_compressor_t
 *
 * Since: 2.24
 **/
xzlib_compressor_t *
g_zlib_compressor_new (GZlibCompressorFormat format,
		       int level)
{
  xzlib_compressor_t *compressor;

  compressor = xobject_new (XTYPE_ZLIB_COMPRESSOR,
			     "format", format,
			     "level", level,
			     NULL);

  return compressor;
}

/**
 * g_zlib_compressor_get_file_info:
 * @compressor: a #xzlib_compressor_t
 *
 * Returns the #xzlib_compressor_t:file-info property.
 *
 * Returns: (nullable) (transfer none): a #xfile_info_t, or %NULL
 *
 * Since: 2.26
 */
xfile_info_t *
g_zlib_compressor_get_file_info (xzlib_compressor_t *compressor)
{
  xreturn_val_if_fail (X_IS_ZLIB_COMPRESSOR (compressor), NULL);

  return compressor->file_info;
}

/**
 * g_zlib_compressor_set_file_info:
 * @compressor: a #xzlib_compressor_t
 * @file_info: (nullable): a #xfile_info_t
 *
 * Sets @file_info in @compressor. If non-%NULL, and @compressor's
 * #xzlib_compressor_t:format property is %G_ZLIB_COMPRESSOR_FORMAT_GZIP,
 * it will be used to set the file name and modification time in
 * the GZIP header of the compressed data.
 *
 * Note: it is an error to call this function while a compression is in
 * progress; it may only be called immediately after creation of @compressor,
 * or after resetting it with xconverter_reset().
 *
 * Since: 2.26
 */
void
g_zlib_compressor_set_file_info (xzlib_compressor_t *compressor,
                                 xfile_info_t       *file_info)
{
  g_return_if_fail (X_IS_ZLIB_COMPRESSOR (compressor));

  if (file_info == compressor->file_info)
    return;

  if (compressor->file_info)
    xobject_unref (compressor->file_info);
  if (file_info)
    xobject_ref (file_info);
  compressor->file_info = file_info;
  xobject_notify (G_OBJECT (compressor), "file-info");

  g_zlib_compressor_set_gzheader (compressor);
}

static void
g_zlib_compressor_reset (xconverter_t *converter)
{
  xzlib_compressor_t *compressor = G_ZLIB_COMPRESSOR (converter);
  int res;

  res = deflateReset (&compressor->zstream);
  if (res != Z_OK)
    g_warning ("unexpected zlib error: %s", compressor->zstream.msg);

  /* deflateReset reset the header too, so re-set it */
  g_zlib_compressor_set_gzheader (compressor);
}

static xconverter_result_t
g_zlib_compressor_convert (xconverter_t *converter,
			   const void *inbuf,
			   xsize_t       inbuf_size,
			   void       *outbuf,
			   xsize_t       outbuf_size,
			   xconverter_flags_t flags,
			   xsize_t      *bytes_read,
			   xsize_t      *bytes_written,
			   xerror_t    **error)
{
  xzlib_compressor_t *compressor;
  int res;
  int flush;

  compressor = G_ZLIB_COMPRESSOR (converter);

  compressor->zstream.next_in = (void *)inbuf;
  compressor->zstream.avail_in = inbuf_size;

  compressor->zstream.next_out = outbuf;
  compressor->zstream.avail_out = outbuf_size;

  flush = Z_NO_FLUSH;
  if (flags & XCONVERTER_INPUT_AT_END)
    flush = Z_FINISH;
  else if (flags & XCONVERTER_FLUSH)
    flush = Z_SYNC_FLUSH;

  res = deflate (&compressor->zstream, flush);

  if (res == Z_MEM_ERROR)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
			   _("Not enough memory"));
      return XCONVERTER_ERROR;
    }

    if (res == Z_STREAM_ERROR)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
		   _("Internal error: %s"), compressor->zstream.msg);
      return XCONVERTER_ERROR;
    }

    if (res == Z_BUF_ERROR)
      {
	if (flags & XCONVERTER_FLUSH)
	  return XCONVERTER_FLUSHED;

	/* We do have output space, so this should only happen if we
	   have no input but need some */

	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT,
			     _("Need more input"));
	return XCONVERTER_ERROR;
      }

  if (res == Z_OK || res == Z_STREAM_END)
    {
      *bytes_read = inbuf_size - compressor->zstream.avail_in;
      *bytes_written = outbuf_size - compressor->zstream.avail_out;

      if (res == Z_STREAM_END)
	return XCONVERTER_FINISHED;
      return XCONVERTER_CONVERTED;
    }

  g_assert_not_reached ();
}

static void
g_zlib_compressor_iface_init (xconverter_iface_t *iface)
{
  iface->convert = g_zlib_compressor_convert;
  iface->reset = g_zlib_compressor_reset;
}
