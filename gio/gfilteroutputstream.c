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
 * Author: Christian Kellner <gicmo@gnome.org>
 */

#include "config.h"
#include "gfilteroutputstream.h"
#include "goutputstream.h"
#include "glibintl.h"


/**
 * SECTION:gfilteroutputstream
 * @short_description: Filter Output Stream
 * @include: gio/gio.h
 *
 * Base class for output stream implementations that perform some
 * kind of filtering operation on a base stream. Typical examples
 * of filtering operations are character set conversion, compression
 * and byte order flipping.
 */

enum {
  PROP_0,
  PROP_BASE_STREAM,
  PROP_CLOSE_BASE
};

static void     g_filter_output_stream_set_property (xobject_t      *object,
                                                     xuint_t         prop_id,
                                                     const xvalue_t *value,
                                                     xparam_spec_t   *pspec);

static void     g_filter_output_stream_get_property (xobject_t    *object,
                                                     xuint_t       prop_id,
                                                     xvalue_t     *value,
                                                     xparam_spec_t *pspec);
static void     g_filter_output_stream_dispose      (xobject_t *object);


static xssize_t   g_filter_output_stream_write        (xoutput_stream_t *stream,
                                                     const void    *buffer,
                                                     xsize_t          count,
                                                     xcancellable_t  *cancellable,
                                                     xerror_t       **error);
static xboolean_t g_filter_output_stream_flush        (xoutput_stream_t    *stream,
                                                     xcancellable_t  *cancellable,
                                                     xerror_t          **error);
static xboolean_t g_filter_output_stream_close        (xoutput_stream_t  *stream,
                                                     xcancellable_t   *cancellable,
                                                     xerror_t        **error);

typedef struct
{
  xboolean_t close_base;
} GFilterOutputStreamPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (xfilter_output_stream_t, g_filter_output_stream, XTYPE_OUTPUT_STREAM)

static void
g_filter_output_stream_class_init (GFilterOutputStreamClass *klass)
{
  xobject_class_t *object_class;
  GOutputStreamClass *ostream_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = g_filter_output_stream_get_property;
  object_class->set_property = g_filter_output_stream_set_property;
  object_class->dispose      = g_filter_output_stream_dispose;

  ostream_class = G_OUTPUT_STREAM_CLASS (klass);
  ostream_class->write_fn = g_filter_output_stream_write;
  ostream_class->flush = g_filter_output_stream_flush;
  ostream_class->close_fn = g_filter_output_stream_close;

  xobject_class_install_property (object_class,
                                   PROP_BASE_STREAM,
                                   g_param_spec_object ("base-stream",
                                                         P_("The Filter Base Stream"),
                                                         P_("The underlying base stream on which the io ops will be done."),
                                                         XTYPE_OUTPUT_STREAM,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

  xobject_class_install_property (object_class,
                                   PROP_CLOSE_BASE,
                                   g_param_spec_boolean ("close-base-stream",
                                                         P_("Close Base Stream"),
                                                         P_("If the base stream should be closed when the filter stream is closed."),
                                                         TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
}

static void
g_filter_output_stream_set_property (xobject_t      *object,
                                     xuint_t         prop_id,
                                     const xvalue_t *value,
                                     xparam_spec_t   *pspec)
{
  xfilter_output_stream_t *filter_stream;
  xobject_t *obj;

  filter_stream = G_FILTER_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_BASE_STREAM:
      obj = xvalue_dup_object (value);
      filter_stream->base_stream = G_OUTPUT_STREAM (obj);
      break;

    case PROP_CLOSE_BASE:
      g_filter_output_stream_set_close_base_stream (filter_stream,
                                                    xvalue_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_filter_output_stream_get_property (xobject_t    *object,
                                     xuint_t       prop_id,
                                     xvalue_t     *value,
                                     xparam_spec_t *pspec)
{
  xfilter_output_stream_t *filter_stream;
  GFilterOutputStreamPrivate *priv;

  filter_stream = G_FILTER_OUTPUT_STREAM (object);
  priv = g_filter_output_stream_get_instance_private (filter_stream);

  switch (prop_id)
    {
    case PROP_BASE_STREAM:
      xvalue_set_object (value, filter_stream->base_stream);
      break;

    case PROP_CLOSE_BASE:
      xvalue_set_boolean (value, priv->close_base);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_filter_output_stream_dispose (xobject_t *object)
{
  xfilter_output_stream_t *stream;

  stream = G_FILTER_OUTPUT_STREAM (object);

  G_OBJECT_CLASS (g_filter_output_stream_parent_class)->dispose (object);

  if (stream->base_stream)
    {
      xobject_unref (stream->base_stream);
      stream->base_stream = NULL;
    }
}


static void
g_filter_output_stream_init (xfilter_output_stream_t *stream)
{
}

/**
 * g_filter_output_stream_get_base_stream:
 * @stream: a #xfilter_output_stream_t.
 *
 * Gets the base stream for the filter stream.
 *
 * Returns: (transfer none): a #xoutput_stream_t.
 **/
xoutput_stream_t *
g_filter_output_stream_get_base_stream (xfilter_output_stream_t *stream)
{
  g_return_val_if_fail (X_IS_FILTER_OUTPUT_STREAM (stream), NULL);

  return stream->base_stream;
}

/**
 * g_filter_output_stream_get_close_base_stream:
 * @stream: a #xfilter_output_stream_t.
 *
 * Returns whether the base stream will be closed when @stream is
 * closed.
 *
 * Returns: %TRUE if the base stream will be closed.
 **/
xboolean_t
g_filter_output_stream_get_close_base_stream (xfilter_output_stream_t *stream)
{
  GFilterOutputStreamPrivate *priv;

  g_return_val_if_fail (X_IS_FILTER_OUTPUT_STREAM (stream), FALSE);

  priv = g_filter_output_stream_get_instance_private (stream);

  return priv->close_base;
}

/**
 * g_filter_output_stream_set_close_base_stream:
 * @stream: a #xfilter_output_stream_t.
 * @close_base: %TRUE to close the base stream.
 *
 * Sets whether the base stream will be closed when @stream is closed.
 **/
void
g_filter_output_stream_set_close_base_stream (xfilter_output_stream_t *stream,
                                              xboolean_t             close_base)
{
  GFilterOutputStreamPrivate *priv;

  g_return_if_fail (X_IS_FILTER_OUTPUT_STREAM (stream));

  close_base = !!close_base;

  priv = g_filter_output_stream_get_instance_private (stream);

  if (priv->close_base != close_base)
    {
      priv->close_base = close_base;
      xobject_notify (G_OBJECT (stream), "close-base-stream");
    }
}

static xssize_t
g_filter_output_stream_write (xoutput_stream_t  *stream,
                              const void     *buffer,
                              xsize_t           count,
                              xcancellable_t   *cancellable,
                              xerror_t        **error)
{
  xfilter_output_stream_t *filter_stream;
  xssize_t nwritten;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  nwritten = xoutput_stream_write (filter_stream->base_stream,
                                    buffer,
                                    count,
                                    cancellable,
                                    error);

  return nwritten;
}

static xboolean_t
g_filter_output_stream_flush (xoutput_stream_t  *stream,
                              xcancellable_t   *cancellable,
                              xerror_t        **error)
{
  xfilter_output_stream_t *filter_stream;
  xboolean_t res;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  res = xoutput_stream_flush (filter_stream->base_stream,
                               cancellable,
                               error);

  return res;
}

static xboolean_t
g_filter_output_stream_close (xoutput_stream_t  *stream,
                              xcancellable_t   *cancellable,
                              xerror_t        **error)
{
  xfilter_output_stream_t *filter_stream = G_FILTER_OUTPUT_STREAM (stream);
  GFilterOutputStreamPrivate *priv = g_filter_output_stream_get_instance_private (filter_stream);
  xboolean_t res = TRUE;

  if (priv->close_base)
    {
      res = xoutput_stream_close (filter_stream->base_stream,
                                   cancellable,
                                   error);
    }

  return res;
}
