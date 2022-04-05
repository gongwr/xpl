/* GLib testing framework examples and tests
 * Copyright (C) 2009 Red Hat, Inc.
 * Authors: Alexander Larsson <alexl@redhat.com>
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

#define XTYPE_EXPANDER_CONVERTER         (g_expander_converter_get_type ())
#define G_EXPANDER_CONVERTER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_EXPANDER_CONVERTER, GExpanderConverter))
#define G_EXPANDER_CONVERTER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_EXPANDER_CONVERTER, GExpanderConverterClass))
#define X_IS_EXPANDER_CONVERTER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_EXPANDER_CONVERTER))
#define X_IS_EXPANDER_CONVERTER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_EXPANDER_CONVERTER))
#define G_EXPANDER_CONVERTER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_EXPANDER_CONVERTER, GExpanderConverterClass))

typedef struct _GExpanderConverter       GExpanderConverter;
typedef struct _GExpanderConverterClass  GExpanderConverterClass;

struct _GExpanderConverterClass
{
  xobject_class_t parent_class;
};

xtype_t       g_expander_converter_get_type (void) G_GNUC_CONST;
GConverter *g_expander_converter_new      (void);



static void g_expander_converter_iface_init          (GConverterIface *iface);

struct _GExpanderConverter
{
  xobject_t parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GExpanderConverter, g_expander_converter, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_CONVERTER,
						g_expander_converter_iface_init))

static void
g_expander_converter_class_init (GExpanderConverterClass *klass)
{
}

static void
g_expander_converter_init (GExpanderConverter *local)
{
}

GConverter *
g_expander_converter_new (void)
{
  GConverter *conv;

  conv = g_object_new (XTYPE_EXPANDER_CONVERTER, NULL);

  return conv;
}

static void
g_expander_converter_reset (GConverter *converter)
{
}

static GConverterResult
g_expander_converter_convert (GConverter *converter,
			      const void *inbuf,
			      xsize_t       inbuf_size,
			      void       *outbuf,
			      xsize_t       outbuf_size,
			      GConverterFlags flags,
			      xsize_t      *bytes_read,
			      xsize_t      *bytes_written,
			      xerror_t    **error)
{
  const guint8 *in, *in_end;
  guint8 v, *out;
  xsize_t i;
  xsize_t block_size;

  in = inbuf;
  out = outbuf;
  in_end = in + inbuf_size;

  while (in < in_end)
    {
      v = *in;

      if (v == 0)
	block_size = 10;
      else
	block_size = v * 1000;

      if (outbuf_size < block_size)
	{
	  if (*bytes_read > 0)
	    return G_CONVERTER_CONVERTED;

	  g_set_error_literal (error, G_IO_ERROR,
			       G_IO_ERROR_NO_SPACE,
			       "No space in dest");
	  return G_CONVERTER_ERROR;
	}

      in++;
      *bytes_read += 1;
      *bytes_written += block_size;
      outbuf_size -= block_size;
      for (i = 0; i < block_size; i++)
	*out++ = v;
    }

  if (in == in_end && (flags & G_CONVERTER_INPUT_AT_END))
    return G_CONVERTER_FINISHED;
  return G_CONVERTER_CONVERTED;
}

static void
g_expander_converter_iface_init (GConverterIface *iface)
{
  iface->convert = g_expander_converter_convert;
  iface->reset = g_expander_converter_reset;
}

#define XTYPE_COMPRESSOR_CONVERTER         (g_compressor_converter_get_type ())
#define G_COMPRESSOR_CONVERTER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_COMPRESSOR_CONVERTER, GCompressorConverter))
#define G_COMPRESSOR_CONVERTER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_COMPRESSOR_CONVERTER, GCompressorConverterClass))
#define X_IS_COMPRESSOR_CONVERTER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_COMPRESSOR_CONVERTER))
#define X_IS_COMPRESSOR_CONVERTER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_COMPRESSOR_CONVERTER))
#define G_COMPRESSOR_CONVERTER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_COMPRESSOR_CONVERTER, GCompressorConverterClass))

typedef struct _GCompressorConverter       GCompressorConverter;
typedef struct _GCompressorConverterClass  GCompressorConverterClass;

struct _GCompressorConverterClass
{
  xobject_class_t parent_class;
};

xtype_t       g_compressor_converter_get_type (void) G_GNUC_CONST;
GConverter *g_compressor_converter_new      (void);



static void g_compressor_converter_iface_init          (GConverterIface *iface);

struct _GCompressorConverter
{
  xobject_t parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GCompressorConverter, g_compressor_converter, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_CONVERTER,
						g_compressor_converter_iface_init))

static void
g_compressor_converter_class_init (GCompressorConverterClass *klass)
{
}

static void
g_compressor_converter_init (GCompressorConverter *local)
{
}

GConverter *
g_compressor_converter_new (void)
{
  GConverter *conv;

  conv = g_object_new (XTYPE_COMPRESSOR_CONVERTER, NULL);

  return conv;
}

static void
g_compressor_converter_reset (GConverter *converter)
{
}

static GConverterResult
g_compressor_converter_convert (GConverter *converter,
				const void *inbuf,
				xsize_t       inbuf_size,
				void       *outbuf,
				xsize_t       outbuf_size,
				GConverterFlags flags,
				xsize_t      *bytes_read,
				xsize_t      *bytes_written,
				xerror_t    **error)
{
  const guint8 *in, *in_end;
  guint8 v, *out;
  xsize_t i;
  xsize_t block_size;

  in = inbuf;
  out = outbuf;
  in_end = in + inbuf_size;

  while (in < in_end)
    {
      v = *in;

      if (v == 0)
	{
	  block_size = 0;
	  while (in+block_size < in_end && *(in+block_size) == 0)
	    block_size ++;
	}
      else
	block_size = v * 1000;

      /* Not enough data */
      if ((xsize_t) (in_end - in) < block_size)
	{
	  if (*bytes_read > 0)
	    break;
	  g_set_error_literal (error, G_IO_ERROR,
			       G_IO_ERROR_PARTIAL_INPUT,
			       "Need more data");
	  return G_CONVERTER_ERROR;
	}

      for (i = 0; i < block_size; i++)
	{
	  if (*(in + i) != v)
	    {
	      if (*bytes_read > 0)
		break;
	      g_set_error_literal (error, G_IO_ERROR,
				   G_IO_ERROR_INVALID_DATA,
				   "invalid data");
	      return G_CONVERTER_ERROR;
	    }
	}

      if (v == 0 && (xsize_t) (in_end - in) == block_size && (flags & G_CONVERTER_INPUT_AT_END) == 0)
	{
	  if (*bytes_read > 0)
	    break;
	  g_set_error_literal (error, G_IO_ERROR,
			       G_IO_ERROR_PARTIAL_INPUT,
			       "Need more data");
	  return G_CONVERTER_ERROR;
	}

      in += block_size;
      *out++ = v;
      *bytes_read += block_size;
      *bytes_written += 1;
    }

  if (in == in_end && (flags & G_CONVERTER_INPUT_AT_END))
    return G_CONVERTER_FINISHED;
  return G_CONVERTER_CONVERTED;
}

static void
g_compressor_converter_iface_init (GConverterIface *iface)
{
  iface->convert = g_compressor_converter_convert;
  iface->reset = g_compressor_converter_reset;
}

guint8 unexpanded_data[] = { 0,1,3,4,5,6,7,3,12,0,0};

static void
test_expander (void)
{
  guint8 *converted1, *converted2, *ptr;
  xsize_t n_read, n_written;
  xsize_t total_read;
  gssize res;
  GConverterResult cres;
  xinput_stream_t *mem, *cstream;
  xoutput_stream_t *mem_out, *cstream_out;
  GConverter *expander;
  GConverter *converter;
  xerror_t *error;
  xsize_t i;

  expander = g_expander_converter_new ();

  converted1 = g_malloc (100*1000); /* Large enough */
  converted2 = g_malloc (100*1000); /* Large enough */

  cres = g_converter_convert (expander,
			      unexpanded_data, sizeof(unexpanded_data),
			      converted1, 100*1000,
			      G_CONVERTER_INPUT_AT_END,
			      &n_read, &n_written, NULL);

  g_assert_cmpint (cres, ==, G_CONVERTER_FINISHED);
  g_assert_cmpuint (n_read, ==, 11);
  g_assert_cmpuint (n_written, ==, 41030);

  g_converter_reset (expander);

  mem = g_memory_input_stream_new_from_data (unexpanded_data,
					     sizeof (unexpanded_data),
					     NULL);
  cstream = g_converter_input_stream_new (mem, expander);
  g_assert_true (g_converter_input_stream_get_converter (G_CONVERTER_INPUT_STREAM (cstream)) == expander);
  g_object_get (cstream, "converter", &converter, NULL);
  g_assert_true (converter == expander);
  g_object_unref (converter);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted2;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert_cmpmem (converted1, n_written, converted2, total_read);

  g_converter_reset (expander);

  mem_out = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  cstream_out = g_converter_output_stream_new (mem_out, expander);
  g_assert_true (g_converter_output_stream_get_converter (G_CONVERTER_OUTPUT_STREAM (cstream_out)) == expander);
  g_object_get (cstream_out, "converter", &converter, NULL);
  g_assert_true (converter == expander);
  g_object_unref (converter);
  g_object_unref (mem_out);

  for (i = 0; i < sizeof(unexpanded_data); i++)
    {
      error = NULL;
      res = g_output_stream_write (cstream_out,
				   unexpanded_data + i, 1,
				   NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	{
	  g_assert_cmpuint (i, ==, sizeof(unexpanded_data) -1);
	  break;
	}
      g_assert_cmpint (res, ==, 1);
    }

  g_output_stream_close (cstream_out, NULL, NULL);

  g_assert_cmpmem (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (mem_out)),
                   g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem_out)),
                   converted1, n_written);

  g_free (converted1);
  g_free (converted2);
  g_object_unref (cstream);
  g_object_unref (cstream_out);
  g_object_unref (expander);
}

static void
test_compressor (void)
{
  guint8 *converted, *expanded, *ptr;
  xsize_t n_read, expanded_size;
  xsize_t total_read;
  gssize res;
  GConverterResult cres;
  xinput_stream_t *mem, *cstream;
  xoutput_stream_t *mem_out, *cstream_out;
  GConverter *expander, *compressor;
  xerror_t *error;
  xsize_t i;

  expander = g_expander_converter_new ();
  expanded = g_malloc (100*1000); /* Large enough */
  cres = g_converter_convert (expander,
			      unexpanded_data, sizeof(unexpanded_data),
			      expanded, 100*1000,
			      G_CONVERTER_INPUT_AT_END,
			      &n_read, &expanded_size, NULL);
  g_assert_cmpint (cres, ==, G_CONVERTER_FINISHED);
  g_assert_cmpuint (n_read, ==, 11);
  g_assert_cmpuint (expanded_size, ==, 41030);

  compressor = g_compressor_converter_new ();

  converted = g_malloc (100*1000); /* Large enough */

  mem = g_memory_input_stream_new_from_data (expanded,
					     expanded_size,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  /* "n_read - 1" because last 2 zeros are combined */
  g_assert_cmpmem (unexpanded_data, n_read - 1, converted, total_read);

  g_object_unref (cstream);

  g_converter_reset (compressor);

  mem_out = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  cstream_out = g_converter_output_stream_new (mem_out, compressor);
  g_object_unref (mem_out);

  for (i = 0; i < expanded_size; i++)
    {
      error = NULL;
      res = g_output_stream_write (cstream_out,
				   expanded + i, 1,
				   NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	{
	  g_assert_cmpuint (i, ==, expanded_size -1);
	  break;
	}
      g_assert_cmpint (res, ==, 1);
    }

  g_output_stream_close (cstream_out, NULL, NULL);

  /* "n_read - 1" because last 2 zeros are combined */
  g_assert_cmpmem (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (mem_out)),
                   g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem_out)),
                   unexpanded_data,
                   n_read - 1);

  g_object_unref (cstream_out);

  g_converter_reset (compressor);

  memset (expanded, 5, 5*1000*2);

  mem = g_memory_input_stream_new_from_data (expanded,
					     5*1000,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert_cmpuint (total_read, ==, 1);
  g_assert_cmpuint (*converted, ==, 5);

  g_object_unref (cstream);

  mem = g_memory_input_stream_new_from_data (expanded,
					     5*1000 * 2,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert_cmpuint (total_read, ==, 2);
  g_assert_cmpuint (converted[0], ==, 5);
  g_assert_cmpuint (converted[1], ==, 5);

  g_object_unref (cstream);

  g_converter_reset (compressor);

  mem = g_memory_input_stream_new_from_data (expanded,
					     5*1000 * 2 - 1,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      if (res == -1)
	{
	  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT);
          g_error_free (error);
	  break;
	}

      g_assert_cmpint (res, !=, 0);
      ptr += res;
      total_read += res;
    }

  g_assert_cmpuint (total_read, ==, 1);
  g_assert_cmpuint (converted[0], ==, 5);

  g_object_unref (cstream);

  g_free (expanded);
  g_free (converted);
  g_object_unref (expander);
  g_object_unref (compressor);
}

#define LEFTOVER_SHORT_READ_SIZE 512

#define XTYPE_LEFTOVER_CONVERTER         (g_leftover_converter_get_type ())
#define G_LEFTOVER_CONVERTER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_LEFTOVER_CONVERTER, GLeftoverConverter))
#define G_LEFTOVER_CONVERTER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_LEFTOVER_CONVERTER, GLeftoverConverterClass))
#define X_IS_LEFTOVER_CONVERTER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_LEFTOVER_CONVERTER))
#define X_IS_LEFTOVER_CONVERTER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_LEFTOVER_CONVERTER))
#define G_LEFTOVER_CONVERTER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_LEFTOVER_CONVERTER, GLeftoverConverterClass))

typedef struct _GLeftoverConverter       GLeftoverConverter;
typedef struct _GLeftoverConverterClass  GLeftoverConverterClass;

struct _GLeftoverConverterClass
{
  xobject_class_t parent_class;
};

xtype_t       g_leftover_converter_get_type (void) G_GNUC_CONST;
GConverter *g_leftover_converter_new      (void);



static void g_leftover_converter_iface_init          (GConverterIface *iface);

struct _GLeftoverConverter
{
  xobject_t parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GLeftoverConverter, g_leftover_converter, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_CONVERTER,
						g_leftover_converter_iface_init))

static void
g_leftover_converter_class_init (GLeftoverConverterClass *klass)
{
}

static void
g_leftover_converter_init (GLeftoverConverter *local)
{
}

GConverter *
g_leftover_converter_new (void)
{
  GConverter *conv;

  conv = g_object_new (XTYPE_LEFTOVER_CONVERTER, NULL);

  return conv;
}

static void
g_leftover_converter_reset (GConverter *converter)
{
}

static GConverterResult
g_leftover_converter_convert (GConverter *converter,
			      const void *inbuf,
			      xsize_t       inbuf_size,
			      void       *outbuf,
			      xsize_t       outbuf_size,
			      GConverterFlags flags,
			      xsize_t      *bytes_read,
			      xsize_t      *bytes_written,
			      xerror_t    **error)
{
  if (outbuf_size == LEFTOVER_SHORT_READ_SIZE)
    {
      g_set_error_literal (error,
			   G_IO_ERROR,
			   G_IO_ERROR_PARTIAL_INPUT,
			   "partial input");
      return G_CONVERTER_ERROR;
    }

  if (inbuf_size < 100)
    *bytes_read = *bytes_written = MIN (inbuf_size, outbuf_size);
  else
    *bytes_read = *bytes_written = MIN (inbuf_size - 10, outbuf_size);
  memcpy (outbuf, inbuf, *bytes_written);

  if (*bytes_read == inbuf_size && (flags & G_CONVERTER_INPUT_AT_END))
    return G_CONVERTER_FINISHED;
  return G_CONVERTER_CONVERTED;
}

static void
g_leftover_converter_iface_init (GConverterIface *iface)
{
  iface->convert = g_leftover_converter_convert;
  iface->reset = g_leftover_converter_reset;
}

#define LEFTOVER_BUFSIZE 8192
#define INTERNAL_BUFSIZE 4096

static void
test_converter_leftover (void)
{
  xchar_t *orig, *converted;
  xsize_t total_read;
  gssize res;
  goffset offset;
  xinput_stream_t *mem, *cstream;
  GConverter *converter;
  xerror_t *error;
  int i;

  converter = g_leftover_converter_new ();

  orig = g_malloc (LEFTOVER_BUFSIZE);
  converted = g_malloc (LEFTOVER_BUFSIZE);
  for (i = 0; i < LEFTOVER_BUFSIZE; i++)
    orig[i] = i % 64 + 32;

  mem = g_memory_input_stream_new_from_data (orig, LEFTOVER_BUFSIZE, NULL);
  cstream = g_converter_input_stream_new (mem, G_CONVERTER (converter));
  g_object_unref (mem);

  total_read = 0;

  error = NULL;
  res = g_input_stream_read (cstream,
			     converted, LEFTOVER_SHORT_READ_SIZE,
			     NULL, &error);
  g_assert_cmpint (res, ==, LEFTOVER_SHORT_READ_SIZE);
  total_read += res;

  offset = g_seekable_tell (G_SEEKABLE (mem));
  g_assert_cmpint (offset, >, LEFTOVER_SHORT_READ_SIZE);
  g_assert_cmpint (offset, <, LEFTOVER_BUFSIZE);

  /* At this point, @cstream has both a non-empty input_buffer
   * and a non-empty converted_buffer, which is the case
   * we want to test.
   */

  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 converted + total_read,
				 LEFTOVER_BUFSIZE - total_read,
				 NULL, &error);
      g_assert_cmpint (res, >=, 0);
      if (res == 0)
	break;
      total_read += res;
  }

  g_assert_cmpmem (orig, LEFTOVER_BUFSIZE, converted, total_read);

  g_object_unref (cstream);
  g_free (orig);
  g_free (converted);
  g_object_unref (converter);
}

#define DATA_LENGTH 1000000

typedef struct {
  const xchar_t *path;
  GZlibCompressorFormat format;
  xint_t level;
} CompressorTest;

static void
test_roundtrip (gconstpointer data)
{
  const CompressorTest *test = data;
  xerror_t *error = NULL;
  guint32 *data0, *data1;
  xsize_t data1_size;
  xint_t i;
  xinput_stream_t *istream0, *istream1, *cistream1;
  xoutput_stream_t *ostream1, *ostream2, *costream1;
  GConverter *compressor, *decompressor;
  GZlibCompressorFormat fmt;
  xint_t lvl;
  GFileInfo *info;
  GFileInfo *info2;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=619945");

  data0 = g_malloc (DATA_LENGTH * sizeof (guint32));
  for (i = 0; i < DATA_LENGTH; i++)
    data0[i] = g_random_int ();

  istream0 = g_memory_input_stream_new_from_data (data0,
    DATA_LENGTH * sizeof (guint32), NULL);

  ostream1 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  compressor = G_CONVERTER (g_zlib_compressor_new (test->format, test->level));
  info = g_file_info_new ();
  g_file_info_set_name (info, "foo");
  g_object_set (compressor, "file-info", info, NULL);
  info2 = g_zlib_compressor_get_file_info (G_ZLIB_COMPRESSOR (compressor));
  g_assert_true (info == info2);
  g_object_unref (info);
  costream1 = g_converter_output_stream_new (ostream1, compressor);
  g_assert_true (g_converter_output_stream_get_converter (G_CONVERTER_OUTPUT_STREAM (costream1)) == compressor);

  g_output_stream_splice (costream1, istream0, 0, NULL, &error);
  g_assert_no_error (error);

  g_object_unref (costream1);

  g_converter_reset (compressor);
  g_object_get (compressor, "format", &fmt, "level", &lvl, NULL);
  g_assert_cmpint (fmt, ==, test->format);
  g_assert_cmpint (lvl, ==, test->level);
  g_object_unref (compressor);
  data1 = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (ostream1));
  data1_size = g_memory_output_stream_get_data_size (
    G_MEMORY_OUTPUT_STREAM (ostream1));
  g_object_unref (ostream1);
  g_object_unref (istream0);

  istream1 = g_memory_input_stream_new_from_data (data1, data1_size, g_free);
  decompressor = G_CONVERTER (g_zlib_decompressor_new (test->format));
  cistream1 = g_converter_input_stream_new (istream1, decompressor);

  ostream2 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);

  g_output_stream_splice (ostream2, cistream1, 0, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpmem (data0, DATA_LENGTH * sizeof (guint32),
                   g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (ostream2)),
                   g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (ostream2)));
  g_object_unref (istream1);
  g_converter_reset (decompressor);
  g_object_get (decompressor, "format", &fmt, NULL);
  g_assert_cmpint (fmt, ==, test->format);
  g_object_unref (decompressor);
  g_object_unref (cistream1);
  g_object_unref (ostream2);
  g_free (data0);
}

typedef struct {
  const xchar_t *path;
  const xchar_t *charset_in;
  const xchar_t *text_in;
  const xchar_t *charset_out;
  const xchar_t *text_out;
  xint_t n_fallbacks;
} CharsetTest;

static void
test_charset (gconstpointer data)
{
  const CharsetTest *test = data;
  xinput_stream_t *in, *in2;
  GConverter *conv;
  xchar_t *buffer;
  xsize_t count;
  xsize_t bytes_read;
  xerror_t *error;
  xboolean_t fallback;

  conv = (GConverter *)g_charset_converter_new (test->charset_out, test->charset_in, NULL);
  g_object_get (conv, "use-fallback", &fallback, NULL);
  g_assert_false (fallback);

  in = g_memory_input_stream_new_from_data (test->text_in, -1, NULL);
  in2 = g_converter_input_stream_new (in, conv);

  count = 2 * strlen (test->text_out);
  buffer = g_malloc0 (count);
  error = NULL;
  g_input_stream_read_all (in2, buffer, count, &bytes_read, NULL, &error);
  if (test->n_fallbacks == 0)
    {
      g_assert_no_error (error);
      g_assert_cmpint (bytes_read, ==, strlen (test->text_out));
      g_assert_cmpstr (buffer, ==, test->text_out);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA);
      g_error_free (error);
    }

  g_free (buffer);
  g_object_unref (in2);
  g_object_unref (in);

  if (test->n_fallbacks == 0)
    {
       g_object_unref (conv);
       return;
    }

  g_converter_reset (conv);

  g_assert_false (g_charset_converter_get_use_fallback (G_CHARSET_CONVERTER (conv)));
  g_charset_converter_set_use_fallback (G_CHARSET_CONVERTER (conv), TRUE);

  in = g_memory_input_stream_new_from_data (test->text_in, -1, NULL);
  in2 = g_converter_input_stream_new (in, conv);

  count = 2 * strlen (test->text_out);
  buffer = g_malloc0 (count);
  error = NULL;
  g_input_stream_read_all (in2, buffer, count, &bytes_read, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (buffer, ==, test->text_out);
  g_assert_cmpint (bytes_read, ==, strlen (test->text_out));
  g_assert_cmpint (test->n_fallbacks, ==, g_charset_converter_get_num_fallbacks (G_CHARSET_CONVERTER (conv)));

  g_free (buffer);
  g_object_unref (in2);
  g_object_unref (in);

  g_object_unref (conv);
}


static void
client_connected (xobject_t      *source,
		  xasync_result_t *result,
		  xpointer_t      user_data)
{
  GSocketClient *client = XSOCKET_CLIENT (source);
  xsocket_connection_t **conn = user_data;
  xerror_t *error = NULL;

  *conn = xsocket_client_connect_finish (client, result, &error);
  g_assert_no_error (error);
}

static void
server_connected (xobject_t      *source,
		  xasync_result_t *result,
		  xpointer_t      user_data)
{
  GSocketListener *listener = XSOCKET_LISTENER (source);
  xsocket_connection_t **conn = user_data;
  xerror_t *error = NULL;

  *conn = xsocket_listener_accept_finish (listener, result, NULL, &error);
  g_assert_no_error (error);
}

static void
make_socketpair (xio_stream_t **left,
		 xio_stream_t **right)
{
  xinet_address_t *iaddr;
  xsocket_address_t *saddr, *effective_address;
  GSocketListener *listener;
  GSocketClient *client;
  xerror_t *error = NULL;
  xsocket_connection_t *client_conn = NULL, *server_conn = NULL;

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);

  listener = xsocket_listener_new ();
  xsocket_listener_add_address (listener, saddr,
				 XSOCKET_TYPE_STREAM,
				 XSOCKET_PROTOCOL_TCP,
				 NULL,
				 &effective_address,
				 &error);
  g_assert_no_error (error);
  g_object_unref (saddr);

  client = xsocket_client_new ();

  xsocket_client_connect_async (client,
				 XSOCKET_CONNECTABLE (effective_address),
				 NULL, client_connected, &client_conn);
  xsocket_listener_accept_async (listener, NULL,
				  server_connected, &server_conn);

  while (!client_conn || !server_conn)
    g_main_context_iteration (NULL, TRUE);

  g_object_unref (client);
  g_object_unref (listener);
  g_object_unref (effective_address);

  *left = XIO_STREAM (client_conn);
  *right = XIO_STREAM (server_conn);
}

static void
test_converter_pollable (void)
{
  xio_stream_t *left, *right;
  guint8 *converted, *inptr;
  guint8 *expanded, *outptr, *expanded_end;
  xsize_t n_read, expanded_size;
  xsize_t total_read;
  gssize res;
  xboolean_t is_readable;
  GConverterResult cres;
  xinput_stream_t *cstream;
  GPollableInputStream *pollable_in;
  xoutput_stream_t *socket_out, *mem_out, *cstream_out;
  GPollableOutputStream *pollable_out;
  GConverter *expander, *compressor;
  xerror_t *error;
  xsize_t i;

  expander = g_expander_converter_new ();
  expanded = g_malloc (100*1000); /* Large enough */
  cres = g_converter_convert (expander,
			      unexpanded_data, sizeof(unexpanded_data),
			      expanded, 100*1000,
			      G_CONVERTER_INPUT_AT_END,
			      &n_read, &expanded_size, NULL);
  g_assert_cmpint (cres, ==, G_CONVERTER_FINISHED);
  g_assert_cmpuint (n_read, ==, 11);
  g_assert_cmpuint (expanded_size, ==, 41030);
  expanded_end = expanded + expanded_size;

  make_socketpair (&left, &right);

  compressor = g_compressor_converter_new ();

  converted = g_malloc (100*1000); /* Large enough */

  cstream = g_converter_input_stream_new (g_io_stream_get_input_stream (left),
					  compressor);
  pollable_in = G_POLLABLE_INPUT_STREAM (cstream);
  g_assert_true (g_pollable_input_stream_can_poll (pollable_in));

  socket_out = g_io_stream_get_output_stream (right);

  total_read = 0;
  outptr = expanded;
  inptr = converted;
  while (TRUE)
    {
      error = NULL;

      if (outptr < expanded_end)
	{
	   res = g_output_stream_write (socket_out,
				       outptr,
				       MIN (1000, (expanded_end - outptr)),
				       NULL, &error);
	  g_assert_cmpint (res, >, 0);
	  outptr += res;
	}
      else if (socket_out)
	{
	  g_output_stream_close (socket_out, NULL, NULL);
	  g_object_unref (right);
	  socket_out = NULL;
	}

      /* Wait a few ticks to check for the pipe to propagate the
       * write. We can’t wait on a GSource as that might affect the stream under
       * test, so just poll. */
      while (!g_pollable_input_stream_is_readable (pollable_in))
        g_usleep (80L);

      is_readable = g_pollable_input_stream_is_readable (pollable_in);
      res = g_pollable_input_stream_read_nonblocking (pollable_in,
						      inptr, 1,
						      NULL, &error);

      /* is_readable can be a false positive, but not a false negative */
      if (!is_readable)
	g_assert_cmpint (res, ==, -1);

      /* After closing the write end, we can't get WOULD_BLOCK any more */
      if (!socket_out)
        {
          g_assert_no_error (error);
          g_assert_cmpint (res, !=, -1);
        }

      if (res == -1)
	{
	  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
	  g_error_free (error);

	  continue;
	}

      if (res == 0)
	break;
      inptr += res;
      total_read += res;
    }

  /* "n_read - 1" because last 2 zeros are combined */
  g_assert_cmpmem (unexpanded_data, n_read - 1, converted, total_read);

  g_object_unref (cstream);
  g_object_unref (left);

  g_converter_reset (compressor);

  /* This doesn't actually test the behavior on
   * G_IO_ERROR_WOULD_BLOCK; to do that we'd need to implement a
   * custom xoutput_stream_t that we could control blocking on.
   */

  mem_out = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  cstream_out = g_converter_output_stream_new (mem_out, compressor);
  g_object_unref (mem_out);
  pollable_out = G_POLLABLE_OUTPUT_STREAM (cstream_out);
  g_assert_true (g_pollable_output_stream_can_poll (pollable_out));
  g_assert_true (g_pollable_output_stream_is_writable (pollable_out));

  for (i = 0; i < expanded_size; i++)
    {
      error = NULL;
      res = g_pollable_output_stream_write_nonblocking (pollable_out,
							expanded + i, 1,
							NULL, &error);
      g_assert_cmpint (res, !=, -1);
      if (res == 0)
	{
	  g_assert_cmpuint (i, ==, expanded_size -1);
	  break;
	}
      g_assert_cmpint (res, ==, 1);
    }

  g_output_stream_close (cstream_out, NULL, NULL);

  /* "n_read - 1" because last 2 zeros are combined */
  g_assert_cmpmem (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (mem_out)),
                   g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem_out)),
                   unexpanded_data,
                   n_read - 1);

  g_object_unref (cstream_out);

  g_free (expanded);
  g_free (converted);
  g_object_unref (expander);
  g_object_unref (compressor);
}

static void
test_truncation (gconstpointer data)
{
  const CompressorTest *test = data;
  xerror_t *error = NULL;
  guint32 *data0, *data1;
  xsize_t data1_size;
  xint_t i;
  xinput_stream_t *istream0, *istream1, *cistream1;
  xoutput_stream_t *ostream1, *ostream2, *costream1;
  GConverter *compressor, *decompressor;

  data0 = g_malloc (DATA_LENGTH * sizeof (guint32));
  for (i = 0; i < DATA_LENGTH; i++)
    data0[i] = g_random_int ();

  istream0 = g_memory_input_stream_new_from_data (data0,
    DATA_LENGTH * sizeof (guint32), NULL);

  ostream1 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  compressor = G_CONVERTER (g_zlib_compressor_new (test->format, -1));
  costream1 = g_converter_output_stream_new (ostream1, compressor);
  g_assert_true (g_converter_output_stream_get_converter (G_CONVERTER_OUTPUT_STREAM (costream1)) == compressor);

  g_output_stream_splice (costream1, istream0, 0, NULL, &error);
  g_assert_no_error (error);

  g_object_unref (costream1);
  g_object_unref (compressor);

  data1 = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (ostream1));
  data1_size = g_memory_output_stream_get_data_size (
    G_MEMORY_OUTPUT_STREAM (ostream1));
  g_object_unref (ostream1);
  g_object_unref (istream0);

  /* truncate */
  data1_size /= 2;

  istream1 = g_memory_input_stream_new_from_data (data1, data1_size, g_free);
  decompressor = G_CONVERTER (g_zlib_decompressor_new (test->format));
  cistream1 = g_converter_input_stream_new (istream1, decompressor);

  ostream2 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);

  g_output_stream_splice (ostream2, cistream1, 0, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT);
  g_error_free (error);

  g_object_unref (istream1);
  g_object_unref (decompressor);
  g_object_unref (cistream1);
  g_object_unref (ostream2);
  g_free (data0);
}

static void
test_converter_basics (void)
{
  GConverter *converter;
  xerror_t *error = NULL;
  xchar_t *to;
  xchar_t *from;

  converter = (GConverter *)g_charset_converter_new ("utf-8", "latin1", &error);
  g_assert_no_error (error);
  g_object_get (converter,
                "to-charset", &to,
                "from-charset", &from,
                NULL);

  g_assert_cmpstr (to, ==, "utf-8");
  g_assert_cmpstr (from, ==, "latin1");

  g_free (to);
  g_free (from);
  g_object_unref (converter);
}

int
main (int   argc,
      char *argv[])
{
  CompressorTest compressor_tests[] = {
    { "/converter-output-stream/roundtrip/zlib-0", G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 0 },
    { "/converter-output-stream/roundtrip/zlib-9", G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 9 },
    { "/converter-output-stream/roundtrip/gzip-0", G_ZLIB_COMPRESSOR_FORMAT_GZIP, 0 },
    { "/converter-output-stream/roundtrip/gzip-9", G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9 },
    { "/converter-output-stream/roundtrip/raw-0", G_ZLIB_COMPRESSOR_FORMAT_RAW, 0 },
    { "/converter-output-stream/roundtrip/raw-9", G_ZLIB_COMPRESSOR_FORMAT_RAW, 9 },
  };
  CompressorTest truncation_tests[] = {
    { "/converter-input-stream/truncation/zlib", G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 0 },
    { "/converter-input-stream/truncation/gzip", G_ZLIB_COMPRESSOR_FORMAT_GZIP, 0 },
    { "/converter-input-stream/truncation/raw", G_ZLIB_COMPRESSOR_FORMAT_RAW, 0 },
  };
  CharsetTest charset_tests[] = {
    { "/converter-input-stream/charset/utf8->latin1", "UTF-8", "\303\205rr Sant\303\251", "ISO-8859-1", "\305rr Sant\351", 0 },
    { "/converter-input-stream/charset/latin1->utf8", "ISO-8859-1", "\305rr Sant\351", "UTF-8", "\303\205rr Sant\303\251", 0 },
    { "/converter-input-stream/charset/fallbacks", "UTF-8", "Some characters just don't fit into latin1: πא", "ISO-8859-1", "Some characters just don't fit into latin1: \\CF\\80\\D7\\90", 4 },
  };

  xsize_t i;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/converter/basics", test_converter_basics);
  g_test_add_func ("/converter-input-stream/expander", test_expander);
  g_test_add_func ("/converter-input-stream/compressor", test_compressor);

  for (i = 0; i < G_N_ELEMENTS (compressor_tests); i++)
    g_test_add_data_func (compressor_tests[i].path, &compressor_tests[i], test_roundtrip);

  for (i = 0; i < G_N_ELEMENTS (truncation_tests); i++)
    g_test_add_data_func (truncation_tests[i].path, &truncation_tests[i], test_truncation);

  for (i = 0; i < G_N_ELEMENTS (charset_tests); i++)
    g_test_add_data_func (charset_tests[i].path, &charset_tests[i], test_charset);

  g_test_add_func ("/converter-stream/pollable", test_converter_pollable);
  g_test_add_func ("/converter-stream/leftover", test_converter_leftover);

  return g_test_run();
}
