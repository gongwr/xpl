/*  GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2008 Christian Kellner, Samuel Cormier-Iijima
 *           © 2009 codethink
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *          Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"
#include "goutputstream.h"
#include "gsocketoutputstream.h"
#include "gsocket.h"
#include "glibintl.h"

#include "gcancellable.h"
#include "gpollableinputstream.h"
#include "gpollableoutputstream.h"
#include "gioerror.h"
#include "glibintl.h"
#include "gfiledescriptorbased.h"
#include "gioprivate.h"

struct _GSocketOutputStreamPrivate
{
  xsocket_t *socket;

  /* pending operation metadata */
  xconstpointer buffer;
  xsize_t count;
};

static void xsocket_output_stream_pollable_iface_init (xpollable_output_stream_interface_t *iface);
#ifdef G_OS_UNIX
static void xsocket_output_stream_file_descriptor_based_iface_init (GFileDescriptorBasedIface *iface);
#endif

#define xsocket_output_stream_get_type _xsocket_output_stream_get_type

#ifdef G_OS_UNIX
G_DEFINE_TYPE_WITH_CODE (GSocketOutputStream, xsocket_output_stream, XTYPE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (GSocketOutputStream)
			 G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_OUTPUT_STREAM, xsocket_output_stream_pollable_iface_init)
			 G_IMPLEMENT_INTERFACE (XTYPE_FILE_DESCRIPTOR_BASED, xsocket_output_stream_file_descriptor_based_iface_init)
			 )
#else
G_DEFINE_TYPE_WITH_CODE (GSocketOutputStream, xsocket_output_stream, XTYPE_OUTPUT_STREAM,
                         G_ADD_PRIVATE (GSocketOutputStream)
			 G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_OUTPUT_STREAM, xsocket_output_stream_pollable_iface_init)
			 )
#endif

enum
{
  PROP_0,
  PROP_SOCKET
};

static void
xsocket_output_stream_get_property (xobject_t    *object,
                                     xuint_t       prop_id,
                                     xvalue_t     *value,
                                     xparam_spec_t *pspec)
{
  GSocketOutputStream *stream = XSOCKET_OUTPUT_STREAM (object);

  switch (prop_id)
    {
      case PROP_SOCKET:
        xvalue_set_object (value, stream->priv->socket);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsocket_output_stream_set_property (xobject_t      *object,
                                     xuint_t         prop_id,
                                     const xvalue_t *value,
                                     xparam_spec_t   *pspec)
{
  GSocketOutputStream *stream = XSOCKET_OUTPUT_STREAM (object);

  switch (prop_id)
    {
      case PROP_SOCKET:
        stream->priv->socket = xvalue_dup_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xsocket_output_stream_finalize (xobject_t *object)
{
  GSocketOutputStream *stream = XSOCKET_OUTPUT_STREAM (object);

  if (stream->priv->socket)
    xobject_unref (stream->priv->socket);

  XOBJECT_CLASS (xsocket_output_stream_parent_class)->finalize (object);
}

static xssize_t
xsocket_output_stream_write (xoutput_stream_t  *stream,
                              const void     *buffer,
                              xsize_t           count,
                              xcancellable_t   *cancellable,
                              xerror_t        **error)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (stream);

  return xsocket_send_with_blocking (output_stream->priv->socket,
				      buffer, count, TRUE,
				      cancellable, error);
}

static xboolean_t
xsocket_output_stream_writev (xoutput_stream_t        *stream,
                               const xoutput_vector_t  *vectors,
                               xsize_t                 n_vectors,
                               xsize_t                *bytes_written,
                               xcancellable_t         *cancellable,
                               xerror_t              **error)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (stream);
  GPollableReturn res;

  /* Clamp the number of vectors if more given than we can write in one go.
   * The caller has to handle short writes anyway.
   */
  if (n_vectors > G_IOV_MAX)
    n_vectors = G_IOV_MAX;

  res = xsocket_send_message_with_timeout (output_stream->priv->socket, NULL,
                                            vectors, n_vectors,
                                            NULL, 0, XSOCKET_MSG_NONE,
                                            -1, bytes_written,
                                            cancellable, error);

  /* we have a non-zero timeout so this can't happen */
  xassert (res != G_POLLABLE_RETURN_WOULD_BLOCK);

  return res == G_POLLABLE_RETURN_OK;
}

static xboolean_t
xsocket_output_stream_pollable_is_writable (xpollable_output_stream_t *pollable)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (pollable);

  return xsocket_condition_check (output_stream->priv->socket, G_IO_OUT);
}

static xssize_t
xsocket_output_stream_pollable_write_nonblocking (xpollable_output_stream_t  *pollable,
						   const void             *buffer,
						   xsize_t                   size,
						   xerror_t                **error)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (pollable);

  return xsocket_send_with_blocking (output_stream->priv->socket,
				      buffer, size, FALSE,
				      NULL, error);
}

static GPollableReturn
xsocket_output_stream_pollable_writev_nonblocking (xpollable_output_stream_t  *pollable,
                                                    const xoutput_vector_t    *vectors,
                                                    xsize_t                   n_vectors,
                                                    xsize_t                  *bytes_written,
                                                    xerror_t                **error)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (pollable);

  /* Clamp the number of vectors if more given than we can write in one go.
   * The caller has to handle short writes anyway.
   */
  if (n_vectors > G_IOV_MAX)
    n_vectors = G_IOV_MAX;

  return xsocket_send_message_with_timeout (output_stream->priv->socket,
                                             NULL, vectors, n_vectors,
                                             NULL, 0, XSOCKET_MSG_NONE, 0,
                                             bytes_written, NULL, error);
}

static xsource_t *
xsocket_output_stream_pollable_create_source (xpollable_output_stream_t *pollable,
					       xcancellable_t          *cancellable)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (pollable);
  xsource_t *socket_source, *pollable_source;

  pollable_source = g_pollable_source_new (G_OBJECT (output_stream));
  socket_source = xsocket_create_source (output_stream->priv->socket,
					  G_IO_OUT, cancellable);
  xsource_set_dummy_callback (socket_source);
  xsource_add_child_source (pollable_source, socket_source);
  xsource_unref (socket_source);

  return pollable_source;
}

#ifdef G_OS_UNIX
static int
xsocket_output_stream_get_fd (xfile_descriptor_based_t *fd_based)
{
  GSocketOutputStream *output_stream = XSOCKET_OUTPUT_STREAM (fd_based);

  return xsocket_get_fd (output_stream->priv->socket);
}
#endif

static void
xsocket_output_stream_class_init (GSocketOutputStreamClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
  xoutput_stream_class_t *goutputstream_class = G_OUTPUT_STREAM_CLASS (klass);

  xobject_class->finalize = xsocket_output_stream_finalize;
  xobject_class->get_property = xsocket_output_stream_get_property;
  xobject_class->set_property = xsocket_output_stream_set_property;

  goutputstream_class->write_fn = xsocket_output_stream_write;
  goutputstream_class->writev_fn = xsocket_output_stream_writev;

  xobject_class_install_property (xobject_class, PROP_SOCKET,
				   xparam_spec_object ("socket",
							P_("socket"),
							P_("The socket that this stream wraps"),
							XTYPE_SOCKET, XPARAM_CONSTRUCT_ONLY |
							XPARAM_READWRITE | XPARAM_STATIC_STRINGS));
}

#ifdef G_OS_UNIX
static void
xsocket_output_stream_file_descriptor_based_iface_init (GFileDescriptorBasedIface *iface)
{
  iface->get_fd = xsocket_output_stream_get_fd;
}
#endif

static void
xsocket_output_stream_pollable_iface_init (xpollable_output_stream_interface_t *iface)
{
  iface->is_writable = xsocket_output_stream_pollable_is_writable;
  iface->create_source = xsocket_output_stream_pollable_create_source;
  iface->write_nonblocking = xsocket_output_stream_pollable_write_nonblocking;
  iface->writev_nonblocking = xsocket_output_stream_pollable_writev_nonblocking;
}

static void
xsocket_output_stream_init (GSocketOutputStream *stream)
{
  stream->priv = xsocket_output_stream_get_instance_private (stream);
}

GSocketOutputStream *
_xsocket_output_stream_new (xsocket_t *socket)
{
  return xobject_new (XTYPE_SOCKET_OUTPUT_STREAM, "socket", socket, NULL);
}
