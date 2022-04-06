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
 * Authors:
 *   Christian Kellner <gicmo@gnome.org>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 */

#include "config.h"
#include "gmemoryoutputstream.h"
#include "goutputstream.h"
#include "gpollableoutputstream.h"
#include "gseekable.h"
#include "gtask.h"
#include "gioerror.h"
#include "string.h"
#include "glibintl.h"
#include "gutilsprivate.h"


/**
 * SECTION:gmemoryoutputstream
 * @short_description: Streaming output operations on memory chunks
 * @include: gio/gio.h
 * @see_also: #xmemory_input_stream_t
 *
 * #xmemory_output_stream_t is a class for using arbitrary
 * memory chunks as output for GIO streaming output operations.
 *
 * As of GLib 2.34, #xmemory_output_stream_t trivially implements
 * #xpollable_output_stream_t: it always polls as ready.
 */

#define MIN_ARRAY_SIZE  16

enum {
  PROP_0,
  PROP_DATA,
  PROP_SIZE,
  PROP_DATA_SIZE,
  PROP_REALLOC_FUNCTION,
  PROP_DESTROY_FUNCTION
};

struct _GMemoryOutputStreamPrivate
{
  xpointer_t       data; /* Write buffer */
  xsize_t          len; /* Current length of the data buffer. Can change with resizing. */
  xsize_t          valid_len; /* The part of data that has been written to */
  xsize_t          pos; /* Current position in the stream. Distinct from valid_len,
                         because the stream is seekable. */

  GReallocFunc   realloc_fn;
  xdestroy_notify_t destroy;
};

static void     g_memory_output_stream_set_property (xobject_t      *object,
                                                     xuint_t         prop_id,
                                                     const xvalue_t *value,
                                                     xparam_spec_t   *pspec);
static void     g_memory_output_stream_get_property (xobject_t      *object,
                                                     xuint_t         prop_id,
                                                     xvalue_t       *value,
                                                     xparam_spec_t   *pspec);
static void     g_memory_output_stream_finalize     (xobject_t      *object);

static xssize_t   g_memory_output_stream_write       (xoutput_stream_t *stream,
                                                    const void    *buffer,
                                                    xsize_t          count,
                                                    xcancellable_t  *cancellable,
                                                    xerror_t       **error);

static xboolean_t g_memory_output_stream_close       (xoutput_stream_t  *stream,
                                                    xcancellable_t   *cancellable,
                                                    xerror_t        **error);

static void     g_memory_output_stream_close_async  (xoutput_stream_t        *stream,
                                                     int                   io_priority,
                                                     xcancellable_t         *cancellable,
                                                     xasync_ready_callback_t   callback,
                                                     xpointer_t              data);
static xboolean_t g_memory_output_stream_close_finish (xoutput_stream_t        *stream,
                                                     xasync_result_t         *result,
                                                     xerror_t              **error);

static void     g_memory_output_stream_seekable_iface_init (xseekable_iface_t  *iface);
static xoffset_t  g_memory_output_stream_tell                (xseekable__t       *seekable);
static xboolean_t g_memory_output_stream_can_seek            (xseekable__t       *seekable);
static xboolean_t g_memory_output_stream_seek                (xseekable__t       *seekable,
                                                           xoffset_t          offset,
                                                           xseek_type_t        type,
                                                           xcancellable_t    *cancellable,
                                                           xerror_t         **error);
static xboolean_t g_memory_output_stream_can_truncate        (xseekable__t       *seekable);
static xboolean_t g_memory_output_stream_truncate            (xseekable__t       *seekable,
                                                           xoffset_t          offset,
                                                           xcancellable_t    *cancellable,
                                                           xerror_t         **error);

static xboolean_t g_memory_output_stream_is_writable       (xpollable_output_stream_t *stream);
static xsource_t *g_memory_output_stream_create_source     (xpollable_output_stream_t *stream,
                                                          xcancellable_t          *cancellable);

static void g_memory_output_stream_pollable_iface_init (xpollable_output_stream_interface_t *iface);

G_DEFINE_TYPE_WITH_CODE (xmemory_output_stream_t, g_memory_output_stream, XTYPE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (xmemory_output_stream_t)
                         G_IMPLEMENT_INTERFACE (XTYPE_SEEKABLE,
                                                g_memory_output_stream_seekable_iface_init);
                         G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_OUTPUT_STREAM,
                                                g_memory_output_stream_pollable_iface_init))


static void
g_memory_output_stream_class_init (GMemoryOutputStreamClass *klass)
{
  xoutput_stream_class_t *ostream_class;
  xobject_class_t *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = g_memory_output_stream_set_property;
  gobject_class->get_property = g_memory_output_stream_get_property;
  gobject_class->finalize     = g_memory_output_stream_finalize;

  ostream_class = G_OUTPUT_STREAM_CLASS (klass);

  ostream_class->write_fn = g_memory_output_stream_write;
  ostream_class->close_fn = g_memory_output_stream_close;
  ostream_class->close_async  = g_memory_output_stream_close_async;
  ostream_class->close_finish = g_memory_output_stream_close_finish;

  /**
   * xmemory_output_stream_t:data:
   *
   * Pointer to buffer where data will be written.
   *
   * Since: 2.24
   **/
  xobject_class_install_property (gobject_class,
                                   PROP_DATA,
                                   g_param_spec_pointer ("data",
                                                         P_("Data buffer_t"),
                                                         P_("Pointer to buffer where data will be written."),
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xmemory_output_stream_t:size:
   *
   * Current size of the data buffer.
   *
   * Since: 2.24
   **/
  xobject_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_ulong ("size",
                                                       P_("Data buffer_t Size"),
                                                       P_("Current size of the data buffer."),
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * xmemory_output_stream_t:data-size:
   *
   * Size of data written to the buffer.
   *
   * Since: 2.24
   **/
  xobject_class_install_property (gobject_class,
                                   PROP_DATA_SIZE,
                                   g_param_spec_ulong ("data-size",
                                                       P_("Data Size"),
                                                       P_("Size of data written to the buffer."),
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * xmemory_output_stream_t:realloc-function: (skip)
   *
   * Function with realloc semantics called to enlarge the buffer.
   *
   * Since: 2.24
   **/
  xobject_class_install_property (gobject_class,
                                   PROP_REALLOC_FUNCTION,
                                   g_param_spec_pointer ("realloc-function",
                                                         P_("Memory Reallocation Function"),
                                                         P_("Function with realloc semantics called to enlarge the buffer."),
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xmemory_output_stream_t:destroy-function: (skip)
   *
   * Function called with the buffer as argument when the stream is destroyed.
   *
   * Since: 2.24
   **/
  xobject_class_install_property (gobject_class,
                                   PROP_DESTROY_FUNCTION,
                                   g_param_spec_pointer ("destroy-function",
                                                         P_("Destroy Notification Function"),
                                                         P_("Function called with the buffer as argument when the stream is destroyed."),
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));
}

static void
g_memory_output_stream_pollable_iface_init (xpollable_output_stream_interface_t *iface)
{
  iface->is_writable = g_memory_output_stream_is_writable;
  iface->create_source = g_memory_output_stream_create_source;
}

static void
g_memory_output_stream_set_property (xobject_t      *object,
                                     xuint_t         prop_id,
                                     const xvalue_t *value,
                                     xparam_spec_t   *pspec)
{
  xmemory_output_stream_t        *stream;
  GMemoryOutputStreamPrivate *priv;

  stream = G_MEMORY_OUTPUT_STREAM (object);
  priv = stream->priv;

  switch (prop_id)
    {
    case PROP_DATA:
      priv->data = xvalue_get_pointer (value);
      break;
    case PROP_SIZE:
      priv->len = xvalue_get_ulong (value);
      break;
    case PROP_REALLOC_FUNCTION:
      priv->realloc_fn = xvalue_get_pointer (value);
      break;
    case PROP_DESTROY_FUNCTION:
      priv->destroy = xvalue_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_memory_output_stream_get_property (xobject_t      *object,
                                     xuint_t         prop_id,
                                     xvalue_t       *value,
                                     xparam_spec_t   *pspec)
{
  xmemory_output_stream_t        *stream;
  GMemoryOutputStreamPrivate *priv;

  stream = G_MEMORY_OUTPUT_STREAM (object);
  priv = stream->priv;

  switch (prop_id)
    {
    case PROP_DATA:
      xvalue_set_pointer (value, priv->data);
      break;
    case PROP_SIZE:
      xvalue_set_ulong (value, priv->len);
      break;
    case PROP_DATA_SIZE:
      xvalue_set_ulong (value, priv->valid_len);
      break;
    case PROP_REALLOC_FUNCTION:
      xvalue_set_pointer (value, priv->realloc_fn);
      break;
    case PROP_DESTROY_FUNCTION:
      xvalue_set_pointer (value, priv->destroy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_memory_output_stream_finalize (xobject_t *object)
{
  xmemory_output_stream_t        *stream;
  GMemoryOutputStreamPrivate *priv;

  stream = G_MEMORY_OUTPUT_STREAM (object);
  priv = stream->priv;

  if (priv->destroy)
    priv->destroy (priv->data);

  G_OBJECT_CLASS (g_memory_output_stream_parent_class)->finalize (object);
}

static void
g_memory_output_stream_seekable_iface_init (xseekable_iface_t *iface)
{
  iface->tell         = g_memory_output_stream_tell;
  iface->can_seek     = g_memory_output_stream_can_seek;
  iface->seek         = g_memory_output_stream_seek;
  iface->can_truncate = g_memory_output_stream_can_truncate;
  iface->truncate_fn  = g_memory_output_stream_truncate;
}


static void
g_memory_output_stream_init (xmemory_output_stream_t *stream)
{
  stream->priv = g_memory_output_stream_get_instance_private (stream);
  stream->priv->pos = 0;
  stream->priv->valid_len = 0;
}

/**
 * g_memory_output_stream_new: (skip)
 * @data: (nullable): pointer to a chunk of memory to use, or %NULL
 * @size: the size of @data
 * @realloc_function: (nullable): a function with realloc() semantics (like g_realloc())
 *     to be called when @data needs to be grown, or %NULL
 * @destroy_function: (nullable): a function to be called on @data when the stream is
 *     finalized, or %NULL
 *
 * Creates a new #xmemory_output_stream_t.
 *
 * In most cases this is not the function you want.  See
 * g_memory_output_stream_new_resizable() instead.
 *
 * If @data is non-%NULL, the stream will use that for its internal storage.
 *
 * If @realloc_fn is non-%NULL, it will be used for resizing the internal
 * storage when necessary and the stream will be considered resizable.
 * In that case, the stream will start out being (conceptually) empty.
 * @size is used only as a hint for how big @data is.  Specifically,
 * seeking to the end of a newly-created stream will seek to zero, not
 * @size.  Seeking past the end of the stream and then writing will
 * introduce a zero-filled gap.
 *
 * If @realloc_fn is %NULL then the stream is fixed-sized.  Seeking to
 * the end will seek to @size exactly.  Writing past the end will give
 * an 'out of space' error.  Attempting to seek past the end will fail.
 * Unlike the resizable case, seeking to an offset within the stream and
 * writing will preserve the bytes passed in as @data before that point
 * and will return them as part of g_memory_output_stream_steal_data().
 * If you intend to seek you should probably therefore ensure that @data
 * is properly initialised.
 *
 * It is probably only meaningful to provide @data and @size in the case
 * that you want a fixed-sized stream.  Put another way: if @realloc_fn
 * is non-%NULL then it makes most sense to give @data as %NULL and
 * @size as 0 (allowing #xmemory_output_stream_t to do the initial
 * allocation for itself).
 *
 * |[<!-- language="C" -->
 * // a stream that can grow
 * stream = g_memory_output_stream_new (NULL, 0, realloc, free);
 *
 * // another stream that can grow
 * stream2 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
 *
 * // a fixed-size stream
 * data = malloc (200);
 * stream3 = g_memory_output_stream_new (data, 200, NULL, free);
 * ]|
 *
 * Returns: A newly created #xmemory_output_stream_t object.
 **/
xoutput_stream_t *
g_memory_output_stream_new (xpointer_t       data,
                            xsize_t          size,
                            GReallocFunc   realloc_function,
                            xdestroy_notify_t destroy_function)
{
  xoutput_stream_t *stream;

  stream = xobject_new (XTYPE_MEMORY_OUTPUT_STREAM,
                         "data", data,
                         "size", size,
                         "realloc-function", realloc_function,
                         "destroy-function", destroy_function,
                         NULL);

  return stream;
}

/**
 * g_memory_output_stream_new_resizable:
 *
 * Creates a new #xmemory_output_stream_t, using g_realloc() and g_free()
 * for memory allocation.
 *
 * Since: 2.36
 */
xoutput_stream_t *
g_memory_output_stream_new_resizable (void)
{
  return g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
}

/**
 * g_memory_output_stream_get_data:
 * @ostream: a #xmemory_output_stream_t
 *
 * Gets any loaded data from the @ostream.
 *
 * Note that the returned pointer may become invalid on the next
 * write or truncate operation on the stream.
 *
 * Returns: (transfer none): pointer to the stream's data, or %NULL if the data
 *    has been stolen
 **/
xpointer_t
g_memory_output_stream_get_data (xmemory_output_stream_t *ostream)
{
  g_return_val_if_fail (X_IS_MEMORY_OUTPUT_STREAM (ostream), NULL);

  return ostream->priv->data;
}

/**
 * g_memory_output_stream_get_size:
 * @ostream: a #xmemory_output_stream_t
 *
 * Gets the size of the currently allocated data area (available from
 * g_memory_output_stream_get_data()).
 *
 * You probably don't want to use this function on resizable streams.
 * See g_memory_output_stream_get_data_size() instead.  For resizable
 * streams the size returned by this function is an implementation
 * detail and may be change at any time in response to operations on the
 * stream.
 *
 * If the stream is fixed-sized (ie: no realloc was passed to
 * g_memory_output_stream_new()) then this is the maximum size of the
 * stream and further writes will return %G_IO_ERROR_NO_SPACE.
 *
 * In any case, if you want the number of bytes currently written to the
 * stream, use g_memory_output_stream_get_data_size().
 *
 * Returns: the number of bytes allocated for the data buffer
 */
xsize_t
g_memory_output_stream_get_size (xmemory_output_stream_t *ostream)
{
  g_return_val_if_fail (X_IS_MEMORY_OUTPUT_STREAM (ostream), 0);

  return ostream->priv->len;
}

/**
 * g_memory_output_stream_get_data_size:
 * @ostream: a #xmemory_output_stream_t
 *
 * Returns the number of bytes from the start up to including the last
 * byte written in the stream that has not been truncated away.
 *
 * Returns: the number of bytes written to the stream
 *
 * Since: 2.18
 */
xsize_t
g_memory_output_stream_get_data_size (xmemory_output_stream_t *ostream)
{
  g_return_val_if_fail (X_IS_MEMORY_OUTPUT_STREAM (ostream), 0);

  return ostream->priv->valid_len;
}

/**
 * g_memory_output_stream_steal_data:
 * @ostream: a #xmemory_output_stream_t
 *
 * Gets any loaded data from the @ostream. Ownership of the data
 * is transferred to the caller; when no longer needed it must be
 * freed using the free function set in @ostream's
 * #xmemory_output_stream_t:destroy-function property.
 *
 * @ostream must be closed before calling this function.
 *
 * Returns: (transfer full): the stream's data, or %NULL if it has previously
 *    been stolen
 *
 * Since: 2.26
 **/
xpointer_t
g_memory_output_stream_steal_data (xmemory_output_stream_t *ostream)
{
  xpointer_t data;

  g_return_val_if_fail (X_IS_MEMORY_OUTPUT_STREAM (ostream), NULL);
  g_return_val_if_fail (xoutput_stream_is_closed (G_OUTPUT_STREAM (ostream)), NULL);

  data = ostream->priv->data;
  ostream->priv->data = NULL;

  return data;
}

/**
 * g_memory_output_stream_steal_as_bytes:
 * @ostream: a #xmemory_output_stream_t
 *
 * Returns data from the @ostream as a #xbytes_t. @ostream must be
 * closed before calling this function.
 *
 * Returns: (transfer full): the stream's data
 *
 * Since: 2.34
 **/
xbytes_t *
g_memory_output_stream_steal_as_bytes (xmemory_output_stream_t *ostream)
{
  xbytes_t *result;

  g_return_val_if_fail (X_IS_MEMORY_OUTPUT_STREAM (ostream), NULL);
  g_return_val_if_fail (xoutput_stream_is_closed (G_OUTPUT_STREAM (ostream)), NULL);

  result = xbytes_new_with_free_func (ostream->priv->data,
                                       ostream->priv->valid_len,
                                       ostream->priv->destroy,
                                       ostream->priv->data);
  ostream->priv->data = NULL;

  return result;
}

static xboolean_t
array_resize (xmemory_output_stream_t  *ostream,
              xsize_t                 size,
              xboolean_t              allow_partial,
              xerror_t              **error)
{
  GMemoryOutputStreamPrivate *priv;
  xpointer_t data;
  xsize_t len;

  priv = ostream->priv;

  if (priv->len == size)
    return TRUE;

  if (!priv->realloc_fn)
    {
      if (allow_partial &&
          priv->pos < priv->len)
        return TRUE; /* Short write */

      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_NO_SPACE,
                           _("Memory output stream not resizable"));
      return FALSE;
    }

  len = priv->len;
  data = priv->realloc_fn (priv->data, size);

  if (size > 0 && !data)
    {
      if (allow_partial &&
          priv->pos < priv->len)
        return TRUE; /* Short write */

      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_NO_SPACE,
                           _("Failed to resize memory output stream"));
      return FALSE;
    }

  if (size > len)
    memset ((xuint8_t *)data + len, 0, size - len);

  priv->data = data;
  priv->len = size;

  if (priv->len < priv->valid_len)
    priv->valid_len = priv->len;

  return TRUE;
}

static xssize_t
g_memory_output_stream_write (xoutput_stream_t  *stream,
                              const void     *buffer,
                              xsize_t           count,
                              xcancellable_t   *cancellable,
                              xerror_t        **error)
{
  xmemory_output_stream_t        *ostream;
  GMemoryOutputStreamPrivate *priv;
  xuint8_t   *dest;
  xsize_t new_size;

  ostream = G_MEMORY_OUTPUT_STREAM (stream);
  priv = ostream->priv;

  if (count == 0)
    return 0;

  /* Check for address space overflow, but only if the buffer is resizable.
     Otherwise we just do a short write and don't worry. */
  if (priv->realloc_fn && priv->pos + count < priv->pos)
    goto overflow;

  if (priv->pos + count > priv->len)
    {
      /* At least enough to fit the write, rounded up for greater than
       * linear growth.
       *
       * Assuming that we're using something like realloc(), the kernel
       * will overcommit memory to us, so doubling the size each time
       * will keep the number of realloc calls low without wasting too
       * much memory.
       */
      new_size = g_nearest_pow (priv->pos + count);
      /* Check for overflow again. We have checked if
         pos + count > G_MAXSIZE, but now check if g_nearest_pow () has
         overflowed */
      if (new_size == 0)
        goto overflow;

      new_size = MAX (new_size, MIN_ARRAY_SIZE);
      if (!array_resize (ostream, new_size, TRUE, error))
        return -1;
    }

  /* Make sure we handle short writes if the array_resize
     only added part of the required memory */
  count = MIN (count, priv->len - priv->pos);

  dest = (xuint8_t *)priv->data + priv->pos;
  memcpy (dest, buffer, count);
  priv->pos += count;

  if (priv->pos > priv->valid_len)
    priv->valid_len = priv->pos;

  return count;

 overflow:
  /* Overflow: buffer size would need to be bigger than G_MAXSIZE. */
  g_set_error_literal (error,
                       G_IO_ERROR,
                       G_IO_ERROR_NO_SPACE,
                       _("Amount of memory required to process the write is "
                         "larger than available address space"));
  return -1;
}

static xboolean_t
g_memory_output_stream_close (xoutput_stream_t  *stream,
                              xcancellable_t   *cancellable,
                              xerror_t        **error)
{
  return TRUE;
}

static void
g_memory_output_stream_close_async (xoutput_stream_t       *stream,
                                    int                  io_priority,
                                    xcancellable_t        *cancellable,
                                    xasync_ready_callback_t  callback,
                                    xpointer_t             data)
{
  xtask_t *task;

  task = xtask_new (stream, cancellable, callback, data);
  xtask_set_source_tag (task, g_memory_output_stream_close_async);

  /* will always return TRUE */
  g_memory_output_stream_close (stream, cancellable, NULL);

  xtask_return_boolean (task, TRUE);
  xobject_unref (task);
}

static xboolean_t
g_memory_output_stream_close_finish (xoutput_stream_t  *stream,
                                     xasync_result_t   *result,
                                     xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (result, stream), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

static xoffset_t
g_memory_output_stream_tell (xseekable__t *seekable)
{
  xmemory_output_stream_t *stream;
  GMemoryOutputStreamPrivate *priv;

  stream = G_MEMORY_OUTPUT_STREAM (seekable);
  priv = stream->priv;

  return priv->pos;
}

static xboolean_t
g_memory_output_stream_can_seek (xseekable__t *seekable)
{
  return TRUE;
}

static xboolean_t
g_memory_output_stream_seek (xseekable__t    *seekable,
                             xoffset_t        offset,
                             xseek_type_t      type,
                             xcancellable_t  *cancellable,
                             xerror_t       **error)
{
  xmemory_output_stream_t        *stream;
  GMemoryOutputStreamPrivate *priv;
  xoffset_t absolute;

  stream = G_MEMORY_OUTPUT_STREAM (seekable);
  priv = stream->priv;

  switch (type)
    {
    case G_SEEK_CUR:
      absolute = priv->pos + offset;
      break;

    case G_SEEK_SET:
      absolute = offset;
      break;

    case G_SEEK_END:
      /* For resizable streams, we consider the end to be the data
       * length.  For fixed-sized streams, we consider the end to be the
       * size of the buffer.
       */
      if (priv->realloc_fn)
        absolute = priv->valid_len + offset;
      else
        absolute = priv->len + offset;
      break;

    default:
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Invalid xseek_type_t supplied"));

      return FALSE;
    }

  if (absolute < 0)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Requested seek before the beginning of the stream"));
      return FALSE;
    }

  /* Can't seek past the end of a fixed-size stream.
   *
   * Note: seeking to the non-existent byte at the end of a fixed-sized
   * stream is valid (eg: a 1-byte fixed sized stream can have position
   * 0 or 1).  Therefore '>' is what we want.
   * */
  if (priv->realloc_fn == NULL && (xsize_t) absolute > priv->len)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Requested seek beyond the end of the stream"));
      return FALSE;
    }

  priv->pos = absolute;

  return TRUE;
}

static xboolean_t
g_memory_output_stream_can_truncate (xseekable__t *seekable)
{
  xmemory_output_stream_t *ostream;
  GMemoryOutputStreamPrivate *priv;

  ostream = G_MEMORY_OUTPUT_STREAM (seekable);
  priv = ostream->priv;

  /* We do not allow truncation of fixed-sized streams */
  return priv->realloc_fn != NULL;
}

static xboolean_t
g_memory_output_stream_truncate (xseekable__t     *seekable,
                                 xoffset_t        offset,
                                 xcancellable_t  *cancellable,
                                 xerror_t       **error)
{
  xmemory_output_stream_t *ostream = G_MEMORY_OUTPUT_STREAM (seekable);

  if (!array_resize (ostream, offset, FALSE, error))
    return FALSE;

  ostream->priv->valid_len = offset;

  return TRUE;
}

static xboolean_t
g_memory_output_stream_is_writable (xpollable_output_stream_t *stream)
{
  return TRUE;
}

static xsource_t *
g_memory_output_stream_create_source (xpollable_output_stream_t *stream,
                                      xcancellable_t          *cancellable)
{
  xsource_t *base_source, *pollable_source;

  base_source = g_timeout_source_new (0);
  pollable_source = g_pollable_source_new_full (stream, base_source, cancellable);
  xsource_unref (base_source);

  return pollable_source;
}
