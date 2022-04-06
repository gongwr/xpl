/* GIO - GLib Input, Output and Streaming Library
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
#include "gsocketinputstream.h"
#include "glibintl.h"

#include "gcancellable.h"
#include "gpollableinputstream.h"
#include "gioerror.h"
#include "gfiledescriptorbased.h"

struct _GSocketInputStreamPrivate
{
  xsocket_t *socket;

  /* pending operation metadata */
  xpointer_t buffer;
  xsize_t count;
};

static void xsocket_input_stream_pollable_iface_init (GPollableInputStreamInterface *iface);
#ifdef G_OS_UNIX
static void xsocket_input_stream_file_descriptor_based_iface_init (GFileDescriptorBasedIface *iface);
#endif

#define xsocket_input_stream_get_type _xsocket_input_stream_get_type

#ifdef G_OS_UNIX
G_DEFINE_TYPE_WITH_CODE (GSocketInputStream, xsocket_input_stream, XTYPE_INPUT_STREAM,
                         G_ADD_PRIVATE (GSocketInputStream)
			 G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_INPUT_STREAM, xsocket_input_stream_pollable_iface_init)
			 G_IMPLEMENT_INTERFACE (XTYPE_FILE_DESCRIPTOR_BASED, xsocket_input_stream_file_descriptor_based_iface_init)
			 )
#else
G_DEFINE_TYPE_WITH_CODE (GSocketInputStream, xsocket_input_stream, XTYPE_INPUT_STREAM,
                         G_ADD_PRIVATE (GSocketInputStream)
			 G_IMPLEMENT_INTERFACE (XTYPE_POLLABLE_INPUT_STREAM, xsocket_input_stream_pollable_iface_init)
			 )
#endif

enum
{
  PROP_0,
  PROP_SOCKET
};

static void
xsocket_input_stream_get_property (xobject_t    *object,
                                    xuint_t       prop_id,
                                    xvalue_t     *value,
                                    xparam_spec_t *pspec)
{
  GSocketInputStream *stream = XSOCKET_INPUT_STREAM (object);

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
xsocket_input_stream_set_property (xobject_t      *object,
                                    xuint_t         prop_id,
                                    const xvalue_t *value,
                                    xparam_spec_t   *pspec)
{
  GSocketInputStream *stream = XSOCKET_INPUT_STREAM (object);

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
xsocket_input_stream_finalize (xobject_t *object)
{
  GSocketInputStream *stream = XSOCKET_INPUT_STREAM (object);

  if (stream->priv->socket)
    xobject_unref (stream->priv->socket);

  G_OBJECT_CLASS (xsocket_input_stream_parent_class)->finalize (object);
}

static xssize_t
xsocket_input_stream_read (xinput_stream_t  *stream,
                            void          *buffer,
                            xsize_t          count,
                            xcancellable_t  *cancellable,
                            xerror_t       **error)
{
  GSocketInputStream *input_stream = XSOCKET_INPUT_STREAM (stream);

  return xsocket_receive_with_blocking (input_stream->priv->socket,
					 buffer, count, TRUE,
					 cancellable, error);
}

static xboolean_t
xsocket_input_stream_pollable_is_readable (xpollable_input_stream_t *pollable)
{
  GSocketInputStream *input_stream = XSOCKET_INPUT_STREAM (pollable);

  return xsocket_condition_check (input_stream->priv->socket, G_IO_IN);
}

static xsource_t *
xsocket_input_stream_pollable_create_source (xpollable_input_stream_t *pollable,
					      xcancellable_t         *cancellable)
{
  GSocketInputStream *input_stream = XSOCKET_INPUT_STREAM (pollable);
  xsource_t *socket_source, *pollable_source;

  pollable_source = g_pollable_source_new (G_OBJECT (input_stream));
  socket_source = xsocket_create_source (input_stream->priv->socket,
					  G_IO_IN, cancellable);
  xsource_set_dummy_callback (socket_source);
  xsource_add_child_source (pollable_source, socket_source);
  xsource_unref (socket_source);

  return pollable_source;
}

static xssize_t
xsocket_input_stream_pollable_read_nonblocking (xpollable_input_stream_t  *pollable,
						 void                  *buffer,
						 xsize_t                  size,
						 xerror_t               **error)
{
  GSocketInputStream *input_stream = XSOCKET_INPUT_STREAM (pollable);

  return xsocket_receive_with_blocking (input_stream->priv->socket,
					 buffer, size, FALSE,
					 NULL, error);
}

#ifdef G_OS_UNIX
static int
xsocket_input_stream_get_fd (xfile_descriptor_based_t *fd_based)
{
  GSocketInputStream *input_stream = XSOCKET_INPUT_STREAM (fd_based);

  return xsocket_get_fd (input_stream->priv->socket);
}
#endif

static void
xsocket_input_stream_class_init (GSocketInputStreamClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *ginputstream_class = G_INPUT_STREAM_CLASS (klass);

  gobject_class->finalize = xsocket_input_stream_finalize;
  gobject_class->get_property = xsocket_input_stream_get_property;
  gobject_class->set_property = xsocket_input_stream_set_property;

  ginputstream_class->read_fn = xsocket_input_stream_read;

  xobject_class_install_property (gobject_class, PROP_SOCKET,
				   g_param_spec_object ("socket",
							P_("socket"),
							P_("The socket that this stream wraps"),
							XTYPE_SOCKET, G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

#ifdef G_OS_UNIX
static void
xsocket_input_stream_file_descriptor_based_iface_init (GFileDescriptorBasedIface *iface)
{
  iface->get_fd = xsocket_input_stream_get_fd;
}
#endif

static void
xsocket_input_stream_pollable_iface_init (GPollableInputStreamInterface *iface)
{
  iface->is_readable = xsocket_input_stream_pollable_is_readable;
  iface->create_source = xsocket_input_stream_pollable_create_source;
  iface->read_nonblocking = xsocket_input_stream_pollable_read_nonblocking;
}

static void
xsocket_input_stream_init (GSocketInputStream *stream)
{
  stream->priv = xsocket_input_stream_get_instance_private (stream);
}

GSocketInputStream *
_xsocket_input_stream_new (xsocket_t *socket)
{
  return xobject_new (XTYPE_SOCKET_INPUT_STREAM, "socket", socket, NULL);
}
