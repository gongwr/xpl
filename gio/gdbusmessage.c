/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

/* Uncomment to debug serializer code */
/* #define DEBUG_SERIALIZER */

#include "config.h"

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#elif MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#elif MAJOR_IN_TYPES
#include <sys/types.h>
#else
#define MAJOR_MINOR_NOT_FOUND 1
#endif

#include "gdbusutils.h"
#include "gdbusmessage.h"
#include "gdbuserror.h"
#include "gioenumtypes.h"
#include "ginputstream.h"
#include "gdatainputstream.h"
#include "gmemoryinputstream.h"
#include "goutputstream.h"
#include "gdataoutputstream.h"
#include "gmemoryoutputstream.h"
#include "gseekable.h"
#include "gioerror.h"
#include "gdbusprivate.h"
#include "gutilsprivate.h"

#ifdef G_OS_UNIX
#include "gunixfdlist.h"
#endif

#include "glibintl.h"

/* See https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-signature
 * This is 64 containers plus 1 value within them. */
#define G_DBUS_MAX_TYPE_DEPTH (64 + 1)

typedef struct _GMemoryBuffer GMemoryBuffer;
struct _GMemoryBuffer
{
  xsize_t len;
  xsize_t valid_len;
  xsize_t pos;
  xchar_t *data;
  GDataStreamByteOrder byte_order;
};

static xboolean_t
g_memory_buffer_is_byteswapped (GMemoryBuffer *mbuf)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  return mbuf->byte_order == G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN;
#else
  return mbuf->byte_order == G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN;
#endif
}

static guchar
g_memory_buffer_read_byte (GMemoryBuffer  *mbuf,
                           xerror_t        **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, 0);

  if (mbuf->pos >= mbuf->valid_len)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading byte.");
      return 0;
    }
  return mbuf->data [mbuf->pos++];
}

static gint16
g_memory_buffer_read_int16 (GMemoryBuffer  *mbuf,
                            xerror_t        **error)
{
  gint16 v;

  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  if (mbuf->pos > mbuf->valid_len - 2)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading int16.");
      return 0;
    }

  memcpy (&v, mbuf->data + mbuf->pos, 2);
  mbuf->pos += 2;

  if (g_memory_buffer_is_byteswapped (mbuf))
    v = GUINT16_SWAP_LE_BE (v);

  return v;
}

static xuint16_t
g_memory_buffer_read_uint16 (GMemoryBuffer  *mbuf,
                             xerror_t        **error)
{
  xuint16_t v;

  g_return_val_if_fail (error == NULL || *error == NULL, 0);

  if (mbuf->pos > mbuf->valid_len - 2)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading uint16.");
      return 0;
    }

  memcpy (&v, mbuf->data + mbuf->pos, 2);
  mbuf->pos += 2;

  if (g_memory_buffer_is_byteswapped (mbuf))
    v = GUINT16_SWAP_LE_BE (v);

  return v;
}

static gint32
g_memory_buffer_read_int32 (GMemoryBuffer  *mbuf,
                            xerror_t        **error)
{
  gint32 v;

  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  if (mbuf->pos > mbuf->valid_len - 4)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading int32.");
      return 0;
    }

  memcpy (&v, mbuf->data + mbuf->pos, 4);
  mbuf->pos += 4;

  if (g_memory_buffer_is_byteswapped (mbuf))
    v = GUINT32_SWAP_LE_BE (v);

  return v;
}

static xuint32_t
g_memory_buffer_read_uint32 (GMemoryBuffer  *mbuf,
                             xerror_t        **error)
{
  xuint32_t v;

  g_return_val_if_fail (error == NULL || *error == NULL, 0);

  if (mbuf->pos > mbuf->valid_len - 4)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading uint32.");
      return 0;
    }

  memcpy (&v, mbuf->data + mbuf->pos, 4);
  mbuf->pos += 4;

  if (g_memory_buffer_is_byteswapped (mbuf))
    v = GUINT32_SWAP_LE_BE (v);

  return v;
}

static gint64
g_memory_buffer_read_int64 (GMemoryBuffer  *mbuf,
                            xerror_t        **error)
{
  gint64 v;

  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  if (mbuf->pos > mbuf->valid_len - 8)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading int64.");
      return 0;
    }

  memcpy (&v, mbuf->data + mbuf->pos, 8);
  mbuf->pos += 8;

  if (g_memory_buffer_is_byteswapped (mbuf))
    v = GUINT64_SWAP_LE_BE (v);

  return v;
}

static xuint64_t
g_memory_buffer_read_uint64 (GMemoryBuffer  *mbuf,
                             xerror_t        **error)
{
  xuint64_t v;

  g_return_val_if_fail (error == NULL || *error == NULL, 0);

  if (mbuf->pos > mbuf->valid_len - 8)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unexpected end of message while reading uint64.");
      return 0;
    }

  memcpy (&v, mbuf->data + mbuf->pos, 8);
  mbuf->pos += 8;

  if (g_memory_buffer_is_byteswapped (mbuf))
    v = GUINT64_SWAP_LE_BE (v);

  return v;
}

#define MIN_ARRAY_SIZE  128

static void
array_resize (GMemoryBuffer  *mbuf,
              xsize_t           size)
{
  xpointer_t data;
  xsize_t len;

  if (mbuf->len == size)
    return;

  len = mbuf->len;
  data = g_realloc (mbuf->data, size);

  if (size > len)
    memset ((xuint8_t *)data + len, 0, size - len);

  mbuf->data = data;
  mbuf->len = size;

  if (mbuf->len < mbuf->valid_len)
    mbuf->valid_len = mbuf->len;
}

static xboolean_t
g_memory_buffer_write (GMemoryBuffer  *mbuf,
                       const void     *buffer,
                       xsize_t           count)
{
  xuint8_t   *dest;
  xsize_t new_size;

  if (count == 0)
    return TRUE;

  /* Check for address space overflow, but only if the buffer is resizable.
     Otherwise we just do a short write and don't worry. */
  if (mbuf->pos + count < mbuf->pos)
    return FALSE;

  if (mbuf->pos + count > mbuf->len)
    {
      /* At least enough to fit the write, rounded up
	     for greater than linear growth.
         TODO: This wastes a lot of memory at large buffer sizes.
               Figure out a more rational allocation strategy. */
      new_size = g_nearest_pow (mbuf->pos + count);
      /* Check for overflow again. We have checked if
         pos + count > G_MAXSIZE, but now check if g_nearest_pow () has
         overflowed */
      if (new_size == 0)
        return FALSE;

      new_size = MAX (new_size, MIN_ARRAY_SIZE);
      array_resize (mbuf, new_size);
    }

  dest = (xuint8_t *)mbuf->data + mbuf->pos;
  memcpy (dest, buffer, count);
  mbuf->pos += count;

  if (mbuf->pos > mbuf->valid_len)
    mbuf->valid_len = mbuf->pos;

  return TRUE;
}

static xboolean_t
g_memory_buffer_put_byte (GMemoryBuffer  *mbuf,
			  guchar          data)
{
  return g_memory_buffer_write (mbuf, &data, 1);
}

static xboolean_t
g_memory_buffer_put_int16 (GMemoryBuffer  *mbuf,
			   gint16          data)
{
  switch (mbuf->byte_order)
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

  return g_memory_buffer_write (mbuf, &data, 2);
}

static xboolean_t
g_memory_buffer_put_uint16 (GMemoryBuffer  *mbuf,
			    xuint16_t         data)
{
  switch (mbuf->byte_order)
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

  return g_memory_buffer_write (mbuf, &data, 2);
}

static xboolean_t
g_memory_buffer_put_int32 (GMemoryBuffer  *mbuf,
			   gint32          data)
{
  switch (mbuf->byte_order)
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

  return g_memory_buffer_write (mbuf, &data, 4);
}

static xboolean_t
g_memory_buffer_put_uint32 (GMemoryBuffer  *mbuf,
			    xuint32_t         data)
{
  switch (mbuf->byte_order)
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

  return g_memory_buffer_write (mbuf, &data, 4);
}

static xboolean_t
g_memory_buffer_put_int64 (GMemoryBuffer  *mbuf,
			   gint64          data)
{
  switch (mbuf->byte_order)
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

  return g_memory_buffer_write (mbuf, &data, 8);
}

static xboolean_t
g_memory_buffer_put_uint64 (GMemoryBuffer  *mbuf,
			    xuint64_t         data)
{
  switch (mbuf->byte_order)
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

  return g_memory_buffer_write (mbuf, &data, 8);
}

static xboolean_t
g_memory_buffer_put_string (GMemoryBuffer  *mbuf,
			    const char     *str)
{
  g_return_val_if_fail (str != NULL, FALSE);

  return g_memory_buffer_write (mbuf, str, strlen (str));
}


/**
 * SECTION:gdbusmessage
 * @short_description: D-Bus Message
 * @include: gio/gio.h
 *
 * A type for representing D-Bus messages that can be sent or received
 * on a #xdbus_connection_t.
 */

typedef struct _GDBusMessageClass GDBusMessageClass;

/**
 * GDBusMessageClass:
 *
 * Class structure for #xdbus_message_t.
 *
 * Since: 2.26
 */
struct _GDBusMessageClass
{
  /*< private >*/
  xobject_class_t parent_class;
};

/**
 * xdbus_message_t:
 *
 * The #xdbus_message_t structure contains only private data and should
 * only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusMessage
{
  /*< private >*/
  xobject_t parent_instance;

  GDBusMessageType type;
  GDBusMessageFlags flags;
  xboolean_t locked;
  GDBusMessageByteOrder byte_order;
  guchar major_protocol_version;
  xuint32_t serial;
  xhashtable_t *headers;
  xvariant_t *body;
#ifdef G_OS_UNIX
  xunix_fd_list_t *fd_list;
#endif
};

enum
{
  PROP_0,
  PROP_LOCKED
};

G_DEFINE_TYPE (xdbus_message, xdbus_message, XTYPE_OBJECT)

static void
xdbus_message_finalize (xobject_t *object)
{
  xdbus_message_t *message = G_DBUS_MESSAGE (object);

  if (message->headers != NULL)
    xhash_table_unref (message->headers);
  if (message->body != NULL)
    xvariant_unref (message->body);
#ifdef G_OS_UNIX
  if (message->fd_list != NULL)
    xobject_unref (message->fd_list);
#endif

  if (G_OBJECT_CLASS (xdbus_message_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (xdbus_message_parent_class)->finalize (object);
}

static void
xdbus_message_get_property (xobject_t    *object,
                             xuint_t       prop_id,
                             xvalue_t     *value,
                             xparam_spec_t *pspec)
{
  xdbus_message_t *message = G_DBUS_MESSAGE (object);

  switch (prop_id)
    {
    case PROP_LOCKED:
      xvalue_set_boolean (value, xdbus_message_get_locked (message));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdbus_message_class_init (GDBusMessageClass *klass)
{
  xobject_class_t *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize     = xdbus_message_finalize;
  gobject_class->get_property = xdbus_message_get_property;

  /**
   * xdbus_connection_t:locked:
   *
   * A boolean specifying whether the message is locked.
   *
   * Since: 2.26
   */
  xobject_class_install_property (gobject_class,
                                   PROP_LOCKED,
                                   g_param_spec_boolean ("locked",
                                                         P_("Locked"),
                                                         P_("Whether the message is locked"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_BLURB |
                                                         G_PARAM_STATIC_NICK));
}

static void
xdbus_message_init (xdbus_message_t *message)
{
  /* Any D-Bus implementation is supposed to handle both Big and
   * Little Endian encodings and the Endianness is part of the D-Bus
   * message - we prefer to use Big Endian (since it's Network Byte
   * Order and just easier to read for humans) but if the machine is
   * Little Endian we use that for performance reasons.
   */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  message->byte_order = G_DBUS_MESSAGE_BYTE_ORDER_LITTLE_ENDIAN;
#else
  /* this could also be G_PDP_ENDIAN */
  message->byte_order = G_DBUS_MESSAGE_BYTE_ORDER_BIG_ENDIAN;
#endif
  message->headers = xhash_table_new_full (g_direct_hash,
                                            g_direct_equal,
                                            NULL,
                                            (xdestroy_notify_t) xvariant_unref);
}

/**
 * xdbus_message_new:
 *
 * Creates a new empty #xdbus_message_t.
 *
 * Returns: A #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new (void)
{
  return xobject_new (XTYPE_DBUS_MESSAGE, NULL);
}

/**
 * xdbus_message_new_method_call:
 * @name: (nullable): A valid D-Bus name or %NULL.
 * @path: A valid object path.
 * @interface_: (nullable): A valid D-Bus interface name or %NULL.
 * @method: A valid method name.
 *
 * Creates a new #xdbus_message_t for a method call.
 *
 * Returns: A #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new_method_call (const xchar_t *name,
                                const xchar_t *path,
                                const xchar_t *interface_,
                                const xchar_t *method)
{
  xdbus_message_t *message;

  g_return_val_if_fail (name == NULL || g_dbus_is_name (name), NULL);
  g_return_val_if_fail (xvariant_is_object_path (path), NULL);
  g_return_val_if_fail (g_dbus_is_member_name (method), NULL);
  g_return_val_if_fail (interface_ == NULL || g_dbus_is_interface_name (interface_), NULL);

  message = xdbus_message_new ();
  message->type = G_DBUS_MESSAGE_TYPE_METHOD_CALL;

  if (name != NULL)
    xdbus_message_set_destination (message, name);
  xdbus_message_set_path (message, path);
  xdbus_message_set_member (message, method);
  if (interface_ != NULL)
    xdbus_message_set_interface (message, interface_);

  return message;
}

/**
 * xdbus_message_new_signal:
 * @path: A valid object path.
 * @interface_: A valid D-Bus interface name.
 * @signal: A valid signal name.
 *
 * Creates a new #xdbus_message_t for a signal emission.
 *
 * Returns: A #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new_signal (const xchar_t  *path,
                           const xchar_t  *interface_,
                           const xchar_t  *signal)
{
  xdbus_message_t *message;

  g_return_val_if_fail (xvariant_is_object_path (path), NULL);
  g_return_val_if_fail (g_dbus_is_member_name (signal), NULL);
  g_return_val_if_fail (g_dbus_is_interface_name (interface_), NULL);

  message = xdbus_message_new ();
  message->type = G_DBUS_MESSAGE_TYPE_SIGNAL;
  message->flags = G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED;

  xdbus_message_set_path (message, path);
  xdbus_message_set_member (message, signal);
  xdbus_message_set_interface (message, interface_);

  return message;
}


/**
 * xdbus_message_new_method_reply:
 * @method_call_message: A message of type %G_DBUS_MESSAGE_TYPE_METHOD_CALL to
 * create a reply message to.
 *
 * Creates a new #xdbus_message_t that is a reply to @method_call_message.
 *
 * Returns: (transfer full):  #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new_method_reply (xdbus_message_t *method_call_message)
{
  xdbus_message_t *message;
  const xchar_t *sender;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (method_call_message), NULL);
  g_return_val_if_fail (xdbus_message_get_message_type (method_call_message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL, NULL);
  g_return_val_if_fail (xdbus_message_get_serial (method_call_message) != 0, NULL);

  message = xdbus_message_new ();
  message->type = G_DBUS_MESSAGE_TYPE_METHOD_RETURN;
  message->flags = G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED;
  /* reply with same endianness */
  message->byte_order = method_call_message->byte_order;

  xdbus_message_set_reply_serial (message, xdbus_message_get_serial (method_call_message));
  sender = xdbus_message_get_sender (method_call_message);
  if (sender != NULL)
    xdbus_message_set_destination (message, sender);

  return message;
}

/**
 * xdbus_message_new_method_error:
 * @method_call_message: A message of type %G_DBUS_MESSAGE_TYPE_METHOD_CALL to
 * create a reply message to.
 * @error_name: A valid D-Bus error name.
 * @error_message_format: The D-Bus error message in a printf() format.
 * @...: Arguments for @error_message_format.
 *
 * Creates a new #xdbus_message_t that is an error reply to @method_call_message.
 *
 * Returns: (transfer full): A #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new_method_error (xdbus_message_t             *method_call_message,
                                 const xchar_t              *error_name,
                                 const xchar_t              *error_message_format,
                                 ...)
{
  xdbus_message_t *ret;
  va_list var_args;

  va_start (var_args, error_message_format);
  ret = xdbus_message_new_method_error_valist (method_call_message,
                                                error_name,
                                                error_message_format,
                                                var_args);
  va_end (var_args);

  return ret;
}

/**
 * xdbus_message_new_method_error_literal:
 * @method_call_message: A message of type %G_DBUS_MESSAGE_TYPE_METHOD_CALL to
 * create a reply message to.
 * @error_name: A valid D-Bus error name.
 * @error_message: The D-Bus error message.
 *
 * Creates a new #xdbus_message_t that is an error reply to @method_call_message.
 *
 * Returns: (transfer full): A #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new_method_error_literal (xdbus_message_t  *method_call_message,
                                         const xchar_t   *error_name,
                                         const xchar_t   *error_message)
{
  xdbus_message_t *message;
  const xchar_t *sender;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (method_call_message), NULL);
  g_return_val_if_fail (xdbus_message_get_message_type (method_call_message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL, NULL);
  g_return_val_if_fail (xdbus_message_get_serial (method_call_message) != 0, NULL);
  g_return_val_if_fail (g_dbus_is_name (error_name), NULL);
  g_return_val_if_fail (error_message != NULL, NULL);

  message = xdbus_message_new ();
  message->type = G_DBUS_MESSAGE_TYPE_ERROR;
  message->flags = G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED;
  /* reply with same endianness */
  message->byte_order = method_call_message->byte_order;

  xdbus_message_set_reply_serial (message, xdbus_message_get_serial (method_call_message));
  xdbus_message_set_error_name (message, error_name);
  xdbus_message_set_body (message, xvariant_new ("(s)", error_message));

  sender = xdbus_message_get_sender (method_call_message);
  if (sender != NULL)
    xdbus_message_set_destination (message, sender);

  return message;
}

/**
 * xdbus_message_new_method_error_valist:
 * @method_call_message: A message of type %G_DBUS_MESSAGE_TYPE_METHOD_CALL to
 * create a reply message to.
 * @error_name: A valid D-Bus error name.
 * @error_message_format: The D-Bus error message in a printf() format.
 * @var_args: Arguments for @error_message_format.
 *
 * Like xdbus_message_new_method_error() but intended for language bindings.
 *
 * Returns: (transfer full): A #xdbus_message_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
G_GNUC_PRINTF(3, 0)
xdbus_message_t *
xdbus_message_new_method_error_valist (xdbus_message_t             *method_call_message,
                                        const xchar_t              *error_name,
                                        const xchar_t              *error_message_format,
                                        va_list                   var_args)
{
  xdbus_message_t *ret;
  xchar_t *error_message;
  error_message = xstrdup_vprintf (error_message_format, var_args);
  ret = xdbus_message_new_method_error_literal (method_call_message,
                                                 error_name,
                                                 error_message);
  g_free (error_message);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_byte_order:
 * @message: A #xdbus_message_t.
 *
 * Gets the byte order of @message.
 *
 * Returns: The byte order.
 */
GDBusMessageByteOrder
xdbus_message_get_byte_order (xdbus_message_t *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), (GDBusMessageByteOrder) 0);
  return message->byte_order;
}

/**
 * xdbus_message_set_byte_order:
 * @message: A #xdbus_message_t.
 * @byte_order: The byte order.
 *
 * Sets the byte order of @message.
 */
void
xdbus_message_set_byte_order (xdbus_message_t          *message,
                               GDBusMessageByteOrder  byte_order)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  message->byte_order = byte_order;
}

/* ---------------------------------------------------------------------------------------------------- */

/* TODO: need GI annotations to specify that any guchar value goes for the type */

/**
 * xdbus_message_get_message_type:
 * @message: A #xdbus_message_t.
 *
 * Gets the type of @message.
 *
 * Returns: A 8-bit unsigned integer (typically a value from the #GDBusMessageType enumeration).
 *
 * Since: 2.26
 */
GDBusMessageType
xdbus_message_get_message_type (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), G_DBUS_MESSAGE_TYPE_INVALID);
  return message->type;
}

/**
 * xdbus_message_set_message_type:
 * @message: A #xdbus_message_t.
 * @type: A 8-bit unsigned integer (typically a value from the #GDBusMessageType enumeration).
 *
 * Sets @message to be of @type.
 *
 * Since: 2.26
 */
void
xdbus_message_set_message_type (xdbus_message_t      *message,
                                 GDBusMessageType   type)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail ((xuint_t) type < 256);

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  message->type = type;
}

/* ---------------------------------------------------------------------------------------------------- */

/* TODO: need GI annotations to specify that any guchar value goes for flags */

/**
 * xdbus_message_get_flags:
 * @message: A #xdbus_message_t.
 *
 * Gets the flags for @message.
 *
 * Returns: Flags that are set (typically values from the #GDBusMessageFlags enumeration bitwise ORed together).
 *
 * Since: 2.26
 */
GDBusMessageFlags
xdbus_message_get_flags (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), G_DBUS_MESSAGE_FLAGS_NONE);
  return message->flags;
}

/**
 * xdbus_message_set_flags:
 * @message: A #xdbus_message_t.
 * @flags: Flags for @message that are set (typically values from the #GDBusMessageFlags
 * enumeration bitwise ORed together).
 *
 * Sets the flags to set on @message.
 *
 * Since: 2.26
 */
void
xdbus_message_set_flags (xdbus_message_t       *message,
                          GDBusMessageFlags   flags)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail ((xuint_t) flags < 256);

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  message->flags = flags;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_serial:
 * @message: A #xdbus_message_t.
 *
 * Gets the serial for @message.
 *
 * Returns: A #xuint32_t.
 *
 * Since: 2.26
 */
xuint32_t
xdbus_message_get_serial (xdbus_message_t *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), 0);
  return message->serial;
}

/**
 * xdbus_message_set_serial:
 * @message: A #xdbus_message_t.
 * @serial: A #xuint32_t.
 *
 * Sets the serial for @message.
 *
 * Since: 2.26
 */
void
xdbus_message_set_serial (xdbus_message_t  *message,
                           xuint32_t        serial)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  message->serial = serial;
}

/* ---------------------------------------------------------------------------------------------------- */

/* TODO: need GI annotations to specify that any guchar value goes for header_field */

/**
 * xdbus_message_get_header:
 * @message: A #xdbus_message_t.
 * @header_field: A 8-bit unsigned integer (typically a value from the #GDBusMessageHeaderField enumeration)
 *
 * Gets a header field on @message.
 *
 * The caller is responsible for checking the type of the returned #xvariant_t
 * matches what is expected.
 *
 * Returns: (transfer none) (nullable): A #xvariant_t with the value if the header was found, %NULL
 * otherwise. Do not free, it is owned by @message.
 *
 * Since: 2.26
 */
xvariant_t *
xdbus_message_get_header (xdbus_message_t             *message,
                           GDBusMessageHeaderField   header_field)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  g_return_val_if_fail ((xuint_t) header_field < 256, NULL);
  return xhash_table_lookup (message->headers, GUINT_TO_POINTER (header_field));
}

/**
 * xdbus_message_set_header:
 * @message: A #xdbus_message_t.
 * @header_field: A 8-bit unsigned integer (typically a value from the #GDBusMessageHeaderField enumeration)
 * @value: (nullable): A #xvariant_t to set the header field or %NULL to clear the header field.
 *
 * Sets a header field on @message.
 *
 * If @value is floating, @message assumes ownership of @value.
 *
 * Since: 2.26
 */
void
xdbus_message_set_header (xdbus_message_t             *message,
                           GDBusMessageHeaderField   header_field,
                           xvariant_t                 *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail ((xuint_t) header_field < 256);

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  if (value == NULL)
    {
      xhash_table_remove (message->headers, GUINT_TO_POINTER (header_field));
    }
  else
    {
      xhash_table_insert (message->headers, GUINT_TO_POINTER (header_field), xvariant_ref_sink (value));
    }
}

/**
 * xdbus_message_get_header_fields:
 * @message: A #xdbus_message_t.
 *
 * Gets an array of all header fields on @message that are set.
 *
 * Returns: (array zero-terminated=1): An array of header fields
 * terminated by %G_DBUS_MESSAGE_HEADER_FIELD_INVALID.  Each element
 * is a #guchar. Free with g_free().
 *
 * Since: 2.26
 */
guchar *
xdbus_message_get_header_fields (xdbus_message_t  *message)
{
  xlist_t *keys;
  guchar *ret;
  xuint_t num_keys;
  xlist_t *l;
  xuint_t n;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);

  keys = xhash_table_get_keys (message->headers);
  num_keys = xlist_length (keys);
  ret = g_new (guchar, num_keys + 1);
  for (l = keys, n = 0; l != NULL; l = l->next, n++)
    ret[n] = GPOINTER_TO_UINT (l->data);
  g_assert (n == num_keys);
  ret[n] = G_DBUS_MESSAGE_HEADER_FIELD_INVALID;
  xlist_free (keys);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_body:
 * @message: A #xdbus_message_t.
 *
 * Gets the body of a message.
 *
 * Returns: (nullable) (transfer none): A #xvariant_t or %NULL if the body is
 * empty. Do not free, it is owned by @message.
 *
 * Since: 2.26
 */
xvariant_t *
xdbus_message_get_body (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return message->body;
}

/**
 * xdbus_message_set_body:
 * @message: A #xdbus_message_t.
 * @body: Either %NULL or a #xvariant_t that is a tuple.
 *
 * Sets the body @message. As a side-effect the
 * %G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE header field is set to the
 * type string of @body (or cleared if @body is %NULL).
 *
 * If @body is floating, @message assumes ownership of @body.
 *
 * Since: 2.26
 */
void
xdbus_message_set_body (xdbus_message_t  *message,
                         xvariant_t      *body)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail ((body == NULL) || xvariant_is_of_type (body, G_VARIANT_TYPE_TUPLE));

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  if (message->body != NULL)
    xvariant_unref (message->body);
  if (body == NULL)
    {
      message->body = NULL;
      xdbus_message_set_signature (message, NULL);
    }
  else
    {
      const xchar_t *type_string;
      xsize_t type_string_len;
      xchar_t *signature;

      message->body = xvariant_ref_sink (body);

      type_string = xvariant_get_type_string (body);
      type_string_len = strlen (type_string);
      g_assert (type_string_len >= 2);
      signature = xstrndup (type_string + 1, type_string_len - 2);
      xdbus_message_set_signature (message, signature);
      g_free (signature);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

#ifdef G_OS_UNIX
/**
 * xdbus_message_get_unix_fd_list:
 * @message: A #xdbus_message_t.
 *
 * Gets the UNIX file descriptors associated with @message, if any.
 *
 * This method is only available on UNIX.
 *
 * The file descriptors normally correspond to %G_VARIANT_TYPE_HANDLE
 * values in the body of the message. For example,
 * if xvariant_get_handle() returns 5, that is intended to be a reference
 * to the file descriptor that can be accessed by
 * `g_unix_fd_list_get (list, 5, ...)`.
 *
 * Returns: (nullable) (transfer none): A #xunix_fd_list_t or %NULL if no file descriptors are
 * associated. Do not free, this object is owned by @message.
 *
 * Since: 2.26
 */
xunix_fd_list_t *
xdbus_message_get_unix_fd_list (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return message->fd_list;
}

/**
 * xdbus_message_set_unix_fd_list:
 * @message: A #xdbus_message_t.
 * @fd_list: (nullable): A #xunix_fd_list_t or %NULL.
 *
 * Sets the UNIX file descriptors associated with @message. As a
 * side-effect the %G_DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS header
 * field is set to the number of fds in @fd_list (or cleared if
 * @fd_list is %NULL).
 *
 * This method is only available on UNIX.
 *
 * When designing D-Bus APIs that are intended to be interoperable,
 * please note that non-GDBus implementations of D-Bus can usually only
 * access file descriptors if they are referenced by a value of type
 * %G_VARIANT_TYPE_HANDLE in the body of the message.
 *
 * Since: 2.26
 */
void
xdbus_message_set_unix_fd_list (xdbus_message_t  *message,
                                 xunix_fd_list_t   *fd_list)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (fd_list == NULL || X_IS_UNIX_FD_LIST (fd_list));

  if (message->locked)
    {
      g_warning ("%s: Attempted to modify a locked message", G_STRFUNC);
      return;
    }

  if (message->fd_list != NULL)
    xobject_unref (message->fd_list);
  if (fd_list != NULL)
    {
      message->fd_list = xobject_ref (fd_list);
      xdbus_message_set_num_unix_fds (message, g_unix_fd_list_get_length (fd_list));
    }
  else
    {
      message->fd_list = NULL;
      xdbus_message_set_num_unix_fds (message, 0);
    }
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

static xuint_t
get_type_fixed_size (const xvariant_type_t *type)
{
  /* NB: we do not treat 'b' as fixed-size here because xvariant_t and
   * D-Bus disagree about the size.
   */
  switch (*xvariant_type_peek_string (type))
    {
    case 'y':
      return 1;
    case 'n': case 'q':
      return 2;
    case 'i': case 'u': case 'h':
      return 4;
    case 'x': case 't': case 'd':
      return 8;
    default:
      return 0;
    }
}

static xboolean_t
validate_headers (xdbus_message_t  *message,
                  xerror_t       **error)
{
  xboolean_t ret;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;

  switch (message->type)
    {
    case G_DBUS_MESSAGE_TYPE_INVALID:
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("type is INVALID"));
      goto out;
      break;

    case G_DBUS_MESSAGE_TYPE_METHOD_CALL:
      if (xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_PATH) == NULL ||
          xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_MEMBER) == NULL)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("METHOD_CALL message: PATH or MEMBER header field is missing"));
          goto out;
        }
      break;

    case G_DBUS_MESSAGE_TYPE_METHOD_RETURN:
      if (xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL) == NULL)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("METHOD_RETURN message: REPLY_SERIAL header field is missing"));
          goto out;
        }
      break;

    case G_DBUS_MESSAGE_TYPE_ERROR:
      if (xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME) == NULL ||
          xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL) == NULL)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("ERROR message: REPLY_SERIAL or ERROR_NAME header field is missing"));
          goto out;
        }
      break;

    case G_DBUS_MESSAGE_TYPE_SIGNAL:
      if (xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_PATH) == NULL ||
          xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_INTERFACE) == NULL ||
          xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_MEMBER) == NULL)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("SIGNAL message: PATH, INTERFACE or MEMBER header field is missing"));
          goto out;
        }
      if (xstrcmp0 (xdbus_message_get_path (message), "/org/freedesktop/DBus/Local") == 0)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("SIGNAL message: The PATH header field is using the reserved value /org/freedesktop/DBus/Local"));
          goto out;
        }
      if (xstrcmp0 (xdbus_message_get_interface (message), "org.freedesktop.DBus.Local") == 0)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("SIGNAL message: The INTERFACE header field is using the reserved value org.freedesktop.DBus.Local"));
          goto out;
        }
      break;

    default:
      /* hitherto unknown type - nothing to check */
      break;
    }

  ret = TRUE;

 out:
  g_assert (ret || (error == NULL || *error != NULL));
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static xboolean_t
ensure_input_padding (GMemoryBuffer  *buf,
                      xsize_t           padding_size)
{
  xsize_t offset;
  xsize_t wanted_offset;

  offset = buf->pos;
  wanted_offset = ((offset + padding_size - 1) / padding_size) * padding_size;
  buf->pos = wanted_offset;
  return TRUE;
}

static const xchar_t *
read_string (GMemoryBuffer  *mbuf,
             xsize_t           len,
             xerror_t        **error)
{
  xchar_t *str;
  const xchar_t *end_valid;

  if G_UNLIKELY (mbuf->pos + len >= mbuf->valid_len || mbuf->pos + len < mbuf->pos)
    {
      mbuf->pos = mbuf->valid_len;
      /* G_GSIZE_FORMAT doesn't work with gettext, so we use %lu */
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   g_dngettext (GETTEXT_PACKAGE,
                                "Wanted to read %lu byte but only got %lu",
                                "Wanted to read %lu bytes but only got %lu",
                                (gulong)len),
                                (gulong)len,
                   (gulong)(mbuf->valid_len - mbuf->pos));
      return NULL;
    }

  if G_UNLIKELY (mbuf->data[mbuf->pos + len] != '\0')
    {
      str = xstrndup (mbuf->data + mbuf->pos, len);
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Expected NUL byte after the string “%s” but found byte %d"),
                   str, mbuf->data[mbuf->pos + len]);
      g_free (str);
      mbuf->pos += len + 1;
      return NULL;
    }

  str = mbuf->data + mbuf->pos;
  mbuf->pos += len + 1;

  if G_UNLIKELY (!xutf8_validate (str, -1, &end_valid))
    {
      xint_t offset;
      xchar_t *valid_str;
      offset = (xint_t) (end_valid - str);
      valid_str = xstrndup (str, offset);
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Expected valid UTF-8 string but found invalid bytes at byte offset %d (length of string is %d). "
                     "The valid UTF-8 string up until that point was “%s”"),
                   offset,
                   (xint_t) len,
                   valid_str);
      g_free (valid_str);
      return NULL;
    }

  return str;
}

static xconstpointer
read_bytes (GMemoryBuffer  *mbuf,
            xsize_t           len,
            xerror_t        **error)
{
  xconstpointer result;

  if G_UNLIKELY (mbuf->pos + len > mbuf->valid_len || mbuf->pos + len < mbuf->pos)
    {
      mbuf->pos = mbuf->valid_len;
      /* G_GSIZE_FORMAT doesn't work with gettext, so we use %lu */
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   g_dngettext (GETTEXT_PACKAGE,
                                "Wanted to read %lu byte but only got %lu",
                                "Wanted to read %lu bytes but only got %lu",
                                (gulong)len),
                                (gulong)len,
                   (gulong)(mbuf->valid_len - mbuf->pos));
      return NULL;
    }

  result = mbuf->data + mbuf->pos;
  mbuf->pos += len;

  return result;
}

/* if just_align==TRUE, don't read a value, just align the input stream wrt padding */

/* returns a non-floating xvariant_t! */
static xvariant_t *
parse_value_from_blob (GMemoryBuffer       *buf,
                       const xvariant_type_t  *type,
                       xuint_t                max_depth,
                       xboolean_t             just_align,
                       xuint_t                indent,
                       xerror_t             **error)
{
  xvariant_t *ret = NULL;
  xerror_t *local_error = NULL;
#ifdef DEBUG_SERIALIZER
  xboolean_t is_leaf;
#endif /* DEBUG_SERIALIZER */
  const xchar_t *type_string;

  if (max_depth == 0)
    {
      g_set_error_literal (&local_error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Value nested too deeply"));
      goto fail;
    }

  type_string = xvariant_type_peek_string (type);

#ifdef DEBUG_SERIALIZER
    {
      xchar_t *s;
      s = xvariant_type_dup_string (type);
      g_print ("%*s%s type %s from offset 0x%04x",
               indent, "",
               just_align ? "Aligning" : "Reading",
               s,
               (xint_t) buf->pos);
      g_free (s);
    }
#endif /* DEBUG_SERIALIZER */

#ifdef DEBUG_SERIALIZER
  is_leaf = TRUE;
#endif /* DEBUG_SERIALIZER */
  switch (type_string[0])
    {
    case 'b': /* G_VARIANT_TYPE_BOOLEAN */
      ensure_input_padding (buf, 4);
      if (!just_align)
        {
          xboolean_t v;
          v = g_memory_buffer_read_uint32 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_boolean (v);
        }
      break;

    case 'y': /* G_VARIANT_TYPE_BYTE */
      if (!just_align)
        {
          guchar v;
          v = g_memory_buffer_read_byte (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_byte (v);
        }
      break;

    case 'n': /* G_VARIANT_TYPE_INT16 */
      ensure_input_padding (buf, 2);
      if (!just_align)
        {
          gint16 v;
          v = g_memory_buffer_read_int16 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_int16 (v);
        }
      break;

    case 'q': /* G_VARIANT_TYPE_UINT16 */
      ensure_input_padding (buf, 2);
      if (!just_align)
        {
          xuint16_t v;
          v = g_memory_buffer_read_uint16 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_uint16 (v);
        }
      break;

    case 'i': /* G_VARIANT_TYPE_INT32 */
      ensure_input_padding (buf, 4);
      if (!just_align)
        {
          gint32 v;
          v = g_memory_buffer_read_int32 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_int32 (v);
        }
      break;

    case 'u': /* G_VARIANT_TYPE_UINT32 */
      ensure_input_padding (buf, 4);
      if (!just_align)
        {
          xuint32_t v;
          v = g_memory_buffer_read_uint32 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_uint32 (v);
        }
      break;

    case 'x': /* G_VARIANT_TYPE_INT64 */
      ensure_input_padding (buf, 8);
      if (!just_align)
        {
          gint64 v;
          v = g_memory_buffer_read_int64 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_int64 (v);
        }
      break;

    case 't': /* G_VARIANT_TYPE_UINT64 */
      ensure_input_padding (buf, 8);
      if (!just_align)
        {
          xuint64_t v;
          v = g_memory_buffer_read_uint64 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_uint64 (v);
        }
      break;

    case 'd': /* G_VARIANT_TYPE_DOUBLE */
      ensure_input_padding (buf, 8);
      if (!just_align)
        {
          union {
            xuint64_t v_uint64;
            xdouble_t v_double;
          } u;
          G_STATIC_ASSERT (sizeof (xdouble_t) == sizeof (xuint64_t));
          u.v_uint64 = g_memory_buffer_read_uint64 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_double (u.v_double);
        }
      break;

    case 's': /* G_VARIANT_TYPE_STRING */
      ensure_input_padding (buf, 4);
      if (!just_align)
        {
          xuint32_t len;
          const xchar_t *v;
          len = g_memory_buffer_read_uint32 (buf, &local_error);
          if (local_error)
            goto fail;
          v = read_string (buf, (xsize_t) len, &local_error);
          if (v == NULL)
            goto fail;
          ret = xvariant_new_string (v);
        }
      break;

    case 'o': /* G_VARIANT_TYPE_OBJECT_PATH */
      ensure_input_padding (buf, 4);
      if (!just_align)
        {
          xuint32_t len;
          const xchar_t *v;
          len = g_memory_buffer_read_uint32 (buf, &local_error);
          if (local_error)
            goto fail;
          v = read_string (buf, (xsize_t) len, &local_error);
          if (v == NULL)
            goto fail;
          if (!xvariant_is_object_path (v))
            {
              g_set_error (&local_error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Parsed value “%s” is not a valid D-Bus object path"),
                           v);
              goto fail;
            }
          ret = xvariant_new_object_path (v);
        }
      break;

    case 'g': /* G_VARIANT_TYPE_SIGNATURE */
      if (!just_align)
        {
          guchar len;
          const xchar_t *v;
          len = g_memory_buffer_read_byte (buf, &local_error);
          if (local_error)
            goto fail;
          v = read_string (buf, (xsize_t) len, &local_error);
          if (v == NULL)
            goto fail;
          if (!xvariant_is_signature (v))
            {
              g_set_error (&local_error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Parsed value “%s” is not a valid D-Bus signature"),
                       v);
              goto fail;
            }
          ret = xvariant_new_signature (v);
        }
      break;

    case 'h': /* G_VARIANT_TYPE_HANDLE */
      ensure_input_padding (buf, 4);
      if (!just_align)
        {
          gint32 v;
          v = g_memory_buffer_read_int32 (buf, &local_error);
          if (local_error)
            goto fail;
          ret = xvariant_new_handle (v);
        }
      break;

    case 'a': /* G_VARIANT_TYPE_ARRAY */
      ensure_input_padding (buf, 4);

      /* If we are only aligning for this array type, it is the child type of
       * another array, which is empty. So, we do not need to add padding for
       * this nonexistent array's elements: we only need to align for this
       * array itself (4 bytes). See
       * <https://bugzilla.gnome.org/show_bug.cgi?id=673612>.
       */
      if (!just_align)
        {
          xuint32_t array_len;
          const xvariant_type_t *element_type;
          xuint_t fixed_size;

          array_len = g_memory_buffer_read_uint32 (buf, &local_error);
          if (local_error)
            goto fail;

#ifdef DEBUG_SERIALIZER
          is_leaf = FALSE;
          g_print (": array spans 0x%04x bytes\n", array_len);
#endif /* DEBUG_SERIALIZER */

          if (array_len > (2<<26))
            {
              /* G_GUINT32_FORMAT doesn't work with gettext, so use u */
              g_set_error (&local_error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           g_dngettext (GETTEXT_PACKAGE,
                                        "Encountered array of length %u byte. Maximum length is 2<<26 bytes (64 MiB).",
                                        "Encountered array of length %u bytes. Maximum length is 2<<26 bytes (64 MiB).",
                                        array_len),
                           array_len);
              goto fail;
            }

          element_type = xvariant_type_element (type);
          fixed_size = get_type_fixed_size (element_type);

          /* Fast-path the cases like 'ay', etc. */
          if (fixed_size != 0)
            {
              xconstpointer array_data;

              if (array_len % fixed_size != 0)
                {
                  g_set_error (&local_error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Encountered array of type “a%c”, expected to have a length a multiple "
                                 "of %u bytes, but found to be %u bytes in length"),
                               xvariant_type_peek_string (element_type)[0], fixed_size, array_len);
                  goto fail;
                }

              if (max_depth == 1)
                {
                  /* If we had recursed into parse_value_from_blob() again to
                   * parse the array values, this would have been emitted. */
                  g_set_error_literal (&local_error,
                                       G_IO_ERROR,
                                       G_IO_ERROR_INVALID_ARGUMENT,
                                       _("Value nested too deeply"));
                  goto fail;
                }

              ensure_input_padding (buf, fixed_size);
              array_data = read_bytes (buf, array_len, &local_error);
              if (array_data == NULL)
                goto fail;

              ret = xvariant_new_fixed_array (element_type, array_data, array_len / fixed_size, fixed_size);

              if (g_memory_buffer_is_byteswapped (buf))
                {
                  xvariant_t *tmp = xvariant_ref_sink (ret);
                  ret = xvariant_byteswap (tmp);
                  xvariant_unref (tmp);
                }
            }
          else
            {
              xvariant_builder_t builder;
              xoffset_t offset;
              xoffset_t target;

              xvariant_builder_init (&builder, type);

              if (array_len == 0)
                {
                  xvariant_t *item G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
                  item = parse_value_from_blob (buf,
                                                element_type,
                                                max_depth - 1,
                                                TRUE,
                                                indent + 2,
                                                NULL);
                  g_assert (item == NULL);
                }
              else
                {
                  offset = buf->pos;
                  target = offset + array_len;
                  while (offset < target)
                    {
                      xvariant_t *item;
                      item = parse_value_from_blob (buf,
                                                    element_type,
                                                    max_depth - 1,
                                                    FALSE,
                                                    indent + 2,
                                                    &local_error);
                      if (item == NULL)
                        {
                          xvariant_builder_clear (&builder);
                          goto fail;
                        }
                      xvariant_builder_add_value (&builder, item);
                      xvariant_unref (item);

                      /* Array elements must not be zero-length. There are no
                       * valid zero-length serialisations of any types which
                       * can be array elements in the D-Bus wire format, so this
                       * assertion should always hold.
                       *
                       * See https://gitlab.gnome.org/GNOME/glib/-/issues/2557
                       */
                      g_assert (buf->pos > (xsize_t) offset);

                      offset = buf->pos;
                    }
                }

              ret = xvariant_builder_end (&builder);
            }
        }
      break;

    default:
      if (xvariant_type_is_dict_entry (type))
        {
          const xvariant_type_t *key_type;
          const xvariant_type_t *value_type;
          xvariant_t *key;
          xvariant_t *value;

          ensure_input_padding (buf, 8);

#ifdef DEBUG_SERIALIZER
          is_leaf = FALSE;
          g_print ("\n");
#endif /* DEBUG_SERIALIZER */

          if (!just_align)
            {
              key_type = xvariant_type_key (type);
              key = parse_value_from_blob (buf,
                                           key_type,
                                           max_depth - 1,
                                           FALSE,
                                           indent + 2,
                                           &local_error);
              if (key == NULL)
                goto fail;
              value_type = xvariant_type_value (type);
              value = parse_value_from_blob (buf,
                                             value_type,
                                             max_depth - 1,
                                             FALSE,
                                             indent + 2,
                                             &local_error);
              if (value == NULL)
                {
                  xvariant_unref (key);
                  goto fail;
                }
              ret = xvariant_new_dict_entry (key, value);
              xvariant_unref (key);
              xvariant_unref (value);
            }
        }
      else if (xvariant_type_is_tuple (type))
        {
          ensure_input_padding (buf, 8);

#ifdef DEBUG_SERIALIZER
          is_leaf = FALSE;
          g_print ("\n");
#endif /* DEBUG_SERIALIZER */

          if (!just_align)
            {
              const xvariant_type_t *element_type;
              xvariant_builder_t builder;

              xvariant_builder_init (&builder, type);
              element_type = xvariant_type_first (type);
              if (!element_type)
                {
                  xvariant_builder_clear (&builder);
                  g_set_error_literal (&local_error,
                                       G_IO_ERROR,
                                       G_IO_ERROR_INVALID_ARGUMENT,
                                       _("Empty structures (tuples) are not allowed in D-Bus"));
                  goto fail;
                }

              while (element_type != NULL)
                {
                  xvariant_t *item;
                  item = parse_value_from_blob (buf,
                                                element_type,
                                                max_depth - 1,
                                                FALSE,
                                                indent + 2,
                                                &local_error);
                  if (item == NULL)
                    {
                      xvariant_builder_clear (&builder);
                      goto fail;
                    }
                  xvariant_builder_add_value (&builder, item);
                  xvariant_unref (item);

                  element_type = xvariant_type_next (element_type);
                }
              ret = xvariant_builder_end (&builder);
            }
        }
      else if (xvariant_type_is_variant (type))
        {
#ifdef DEBUG_SERIALIZER
          is_leaf = FALSE;
          g_print ("\n");
#endif /* DEBUG_SERIALIZER */

          if (!just_align)
            {
              guchar siglen;
              const xchar_t *sig;
              xvariant_type_t *variant_type;
              xvariant_t *value;

              siglen = g_memory_buffer_read_byte (buf, &local_error);
              if (local_error)
                goto fail;
              sig = read_string (buf, (xsize_t) siglen, &local_error);
              if (sig == NULL)
                goto fail;
              if (!xvariant_is_signature (sig) ||
                  !xvariant_type_string_is_valid (sig))
                {
                  /* A D-Bus signature can contain zero or more complete types,
                   * but a xvariant_t has to be exactly one complete type. */
                  g_set_error (&local_error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Parsed value “%s” for variant is not a valid D-Bus signature"),
                               sig);
                  goto fail;
                }

              if (max_depth <= xvariant_type_string_get_depth_ (sig))
                {
                  /* Catch the type nesting being too deep without having to
                   * parse the data. We don’t have to check this for static
                   * container types (like arrays and tuples, above) because
                   * the xvariant_type_string_is_valid() check performed before
                   * the initial parse_value_from_blob() call should check the
                   * static type nesting. */
                  g_set_error_literal (&local_error,
                                       G_IO_ERROR,
                                       G_IO_ERROR_INVALID_ARGUMENT,
                                       _("Value nested too deeply"));
                  goto fail;
                }

              variant_type = xvariant_type_new (sig);
              value = parse_value_from_blob (buf,
                                             variant_type,
                                             max_depth - 1,
                                             FALSE,
                                             indent + 2,
                                             &local_error);
              xvariant_type_free (variant_type);
              if (value == NULL)
                goto fail;
              ret = xvariant_new_variant (value);
              xvariant_unref (value);
            }
        }
      else
        {
          xchar_t *s;
          s = xvariant_type_dup_string (type);
          g_set_error (&local_error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error deserializing xvariant_t with type string “%s” from the D-Bus wire format"),
                       s);
          g_free (s);
          goto fail;
        }
      break;
    }

  g_assert ((just_align && ret == NULL) || (!just_align && ret != NULL));

#ifdef DEBUG_SERIALIZER
  if (ret != NULL)
    {
      if (is_leaf)
        {
          xchar_t *s;
          if (xvariant_type_equal (type, G_VARIANT_TYPE_BYTE))
            {
              s = xstrdup_printf ("0x%02x '%c'", xvariant_get_byte (ret), xvariant_get_byte (ret));
            }
          else
            {
              s = xvariant_print (ret, FALSE);
            }
          g_print (": %s\n", s);
          g_free (s);
        }
    }
#endif /* DEBUG_SERIALIZER */

  /* sink the reference, if floating */
  if (ret != NULL)
    xvariant_take_ref (ret);
  return ret;

 fail:
#ifdef DEBUG_SERIALIZER
  g_print ("\n"
           "%*sFAILURE: %s (%s, %d)\n",
           indent, "",
           local_error->message,
           g_quark_to_string (local_error->domain),
           local_error->code);
#endif /* DEBUG_SERIALIZER */
  g_propagate_error (error, local_error);
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/* message_header must be at least 16 bytes */

/**
 * xdbus_message_bytes_needed:
 * @blob: (array length=blob_len) (element-type xuint8_t): A blob representing a binary D-Bus message.
 * @blob_len: The length of @blob (must be at least 16).
 * @error: Return location for error or %NULL.
 *
 * Utility function to calculate how many bytes are needed to
 * completely deserialize the D-Bus message stored at @blob.
 *
 * Returns: Number of bytes needed or -1 if @error is set (e.g. if
 * @blob contains invalid data or not enough data is available to
 * determine the size).
 *
 * Since: 2.26
 */
xssize_t
xdbus_message_bytes_needed (guchar  *blob,
                             xsize_t    blob_len,
                             xerror_t **error)
{
  xssize_t ret;

  ret = -1;

  g_return_val_if_fail (blob != NULL, -1);
  g_return_val_if_fail (error == NULL || *error == NULL, -1);
  g_return_val_if_fail (blob_len >= 16, -1);

  if (blob[0] == 'l')
    {
      /* core header (12 bytes) + ARRAY of STRUCT of (BYTE,VARIANT) */
      ret = 12 + 4 + GUINT32_FROM_LE (((xuint32_t *) blob)[3]);
      /* round up so it's a multiple of 8 */
      ret = 8 * ((ret + 7)/8);
      /* finally add the body size */
      ret += GUINT32_FROM_LE (((xuint32_t *) blob)[1]);
    }
  else if (blob[0] == 'B')
    {
      /* core header (12 bytes) + ARRAY of STRUCT of (BYTE,VARIANT) */
      ret = 12 + 4 + GUINT32_FROM_BE (((xuint32_t *) blob)[3]);
      /* round up so it's a multiple of 8 */
      ret = 8 * ((ret + 7)/8);
      /* finally add the body size */
      ret += GUINT32_FROM_BE (((xuint32_t *) blob)[1]);
    }
  else
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Unable to determine message blob length - given blob is malformed");
    }

  if (ret > (1<<27))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Blob indicates that message exceeds maximum message length (128MiB)");
      ret = -1;
    }

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_new_from_blob:
 * @blob: (array length=blob_len) (element-type xuint8_t): A blob representing a binary D-Bus message.
 * @blob_len: The length of @blob.
 * @capabilities: A #GDBusCapabilityFlags describing what protocol features are supported.
 * @error: Return location for error or %NULL.
 *
 * Creates a new #xdbus_message_t from the data stored at @blob. The byte
 * order that the message was in can be retrieved using
 * xdbus_message_get_byte_order().
 *
 * If the @blob cannot be parsed, contains invalid fields, or contains invalid
 * headers, %G_IO_ERROR_INVALID_ARGUMENT will be returned.
 *
 * Returns: A new #xdbus_message_t or %NULL if @error is set. Free with
 * xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_new_from_blob (guchar                *blob,
                              xsize_t                  blob_len,
                              GDBusCapabilityFlags   capabilities,
                              xerror_t               **error)
{
  xerror_t *local_error = NULL;
  GMemoryBuffer mbuf;
  xdbus_message_t *message;
  guchar endianness;
  guchar major_protocol_version;
  xuint32_t message_body_len;
  xvariant_t *headers;
  xvariant_t *item;
  xvariant_iter_t iter;
  xvariant_t *signature;

  /* TODO: check against @capabilities */

  g_return_val_if_fail (blob != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  message = xdbus_message_new ();

  memset (&mbuf, 0, sizeof (mbuf));
  mbuf.data = (xchar_t *)blob;
  mbuf.len = mbuf.valid_len = blob_len;

  endianness = g_memory_buffer_read_byte (&mbuf, &local_error);
  if (local_error)
    goto fail;

  switch (endianness)
    {
    case 'l':
      mbuf.byte_order = G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN;
      message->byte_order = G_DBUS_MESSAGE_BYTE_ORDER_LITTLE_ENDIAN;
      break;
    case 'B':
      mbuf.byte_order = G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN;
      message->byte_order = G_DBUS_MESSAGE_BYTE_ORDER_BIG_ENDIAN;
      break;
    default:
      g_set_error (&local_error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Invalid endianness value. Expected 0x6c (“l”) or 0x42 (“B”) but found value 0x%02x"),
                   endianness);
      goto fail;
    }

  message->type = g_memory_buffer_read_byte (&mbuf, &local_error);
  if (local_error)
    goto fail;
  message->flags = g_memory_buffer_read_byte (&mbuf, &local_error);
  if (local_error)
    goto fail;
  major_protocol_version = g_memory_buffer_read_byte (&mbuf, &local_error);
  if (local_error)
    goto fail;
  if (major_protocol_version != 1)
    {
      g_set_error (&local_error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Invalid major protocol version. Expected 1 but found %d"),
                   major_protocol_version);
      goto fail;
    }
  message_body_len = g_memory_buffer_read_uint32 (&mbuf, &local_error);
  if (local_error)
    goto fail;
  message->serial = g_memory_buffer_read_uint32 (&mbuf, &local_error);
  if (local_error)
    goto fail;

#ifdef DEBUG_SERIALIZER
  g_print ("Parsing blob (blob_len = 0x%04x bytes)\n", (xint_t) blob_len);
  {
    xchar_t *s;
    s = _g_dbus_hexdump ((const xchar_t *) blob, blob_len, 2);
    g_print ("%s\n", s);
    g_free (s);
  }
#endif /* DEBUG_SERIALIZER */

#ifdef DEBUG_SERIALIZER
  g_print ("Parsing headers (blob_len = 0x%04x bytes)\n", (xint_t) blob_len);
#endif /* DEBUG_SERIALIZER */
  headers = parse_value_from_blob (&mbuf,
                                   G_VARIANT_TYPE ("a{yv}"),
                                   G_DBUS_MAX_TYPE_DEPTH + 2 /* for the a{yv} */,
                                   FALSE,
                                   2,
                                   &local_error);
  if (headers == NULL)
    goto fail;
  xvariant_iter_init (&iter, headers);
  while ((item = xvariant_iter_next_value (&iter)) != NULL)
    {
      guchar header_field;
      xvariant_t *value;
      xvariant_get (item,
                     "{yv}",
                     &header_field,
                     &value);
      xdbus_message_set_header (message, header_field, value);
      xvariant_unref (value);
      xvariant_unref (item);
    }
  xvariant_unref (headers);

  signature = xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE);
  if (signature != NULL)
    {
      const xchar_t *signature_str;
      xsize_t signature_str_len;

      if (!xvariant_is_of_type (signature, G_VARIANT_TYPE_SIGNATURE))
        {
          g_set_error_literal (&local_error,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               _("Signature header found but is not of type signature"));
          goto fail;
        }

      signature_str = xvariant_get_string (signature, &signature_str_len);

      /* signature but no body */
      if (message_body_len == 0 && signature_str_len > 0)
        {
          g_set_error (&local_error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Signature header with signature “%s” found but message body is empty"),
                       signature_str);
          goto fail;
        }
      else if (signature_str_len > 0)
        {
          xvariant_type_t *variant_type;
          xchar_t *tupled_signature_str = xstrdup_printf ("(%s)", signature_str);

          if (!xvariant_is_signature (signature_str) ||
              !xvariant_type_string_is_valid (tupled_signature_str))
            {
              g_set_error (&local_error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Parsed value “%s” is not a valid D-Bus signature (for body)"),
                           signature_str);
              g_free (tupled_signature_str);
              goto fail;
            }

          variant_type = xvariant_type_new (tupled_signature_str);
          g_free (tupled_signature_str);
#ifdef DEBUG_SERIALIZER
          g_print ("Parsing body (blob_len = 0x%04x bytes)\n", (xint_t) blob_len);
#endif /* DEBUG_SERIALIZER */
          message->body = parse_value_from_blob (&mbuf,
                                                 variant_type,
                                                 G_DBUS_MAX_TYPE_DEPTH + 1 /* for the surrounding tuple */,
                                                 FALSE,
                                                 2,
                                                 &local_error);
          xvariant_type_free (variant_type);
          if (message->body == NULL)
            goto fail;
        }
    }
  else
    {
      /* no signature, this is only OK if the body is empty */
      if (message_body_len != 0)
        {
          /* G_GUINT32_FORMAT doesn't work with gettext, just use %u */
          g_set_error (&local_error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       g_dngettext (GETTEXT_PACKAGE,
                                    "No signature header in message but the message body is %u byte",
                                    "No signature header in message but the message body is %u bytes",
                                    message_body_len),
                       message_body_len);
          goto fail;
        }
    }

  if (!validate_headers (message, &local_error))
    {
      g_prefix_error (&local_error, _("Cannot deserialize message: "));
      goto fail;
    }

  return message;

fail:
  g_clear_object (&message);
  g_propagate_error (error, local_error);
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

static xsize_t
ensure_output_padding (GMemoryBuffer  *mbuf,
                       xsize_t           padding_size)
{
  xsize_t offset;
  xsize_t wanted_offset;
  xsize_t padding_needed;
  xuint_t n;

  offset = mbuf->pos;
  wanted_offset = ((offset + padding_size - 1) / padding_size) * padding_size;
  padding_needed = wanted_offset - offset;

  for (n = 0; n < padding_needed; n++)
    g_memory_buffer_put_byte (mbuf, '\0');

  return padding_needed;
}

/* note that value can be NULL for e.g. empty arrays - type is never NULL */
static xboolean_t
append_value_to_blob (xvariant_t            *value,
                      const xvariant_type_t  *type,
                      GMemoryBuffer       *mbuf,
                      xsize_t               *out_padding_added,
                      xerror_t             **error)
{
  xsize_t padding_added;
  const xchar_t *type_string;

  type_string = xvariant_type_peek_string (type);

  padding_added = 0;

  switch (type_string[0])
    {
    case 'b': /* G_VARIANT_TYPE_BOOLEAN */
      padding_added = ensure_output_padding (mbuf, 4);
      if (value != NULL)
        {
          xboolean_t v = xvariant_get_boolean (value);
          g_memory_buffer_put_uint32 (mbuf, v);
        }
      break;

    case 'y': /* G_VARIANT_TYPE_BYTE */
      if (value != NULL)
        {
          xuint8_t v = xvariant_get_byte (value);
          g_memory_buffer_put_byte (mbuf, v);
        }
      break;

    case 'n': /* G_VARIANT_TYPE_INT16 */
      padding_added = ensure_output_padding (mbuf, 2);
      if (value != NULL)
        {
          gint16 v = xvariant_get_int16 (value);
          g_memory_buffer_put_int16 (mbuf, v);
        }
      break;

    case 'q': /* G_VARIANT_TYPE_UINT16 */
      padding_added = ensure_output_padding (mbuf, 2);
      if (value != NULL)
        {
          xuint16_t v = xvariant_get_uint16 (value);
          g_memory_buffer_put_uint16 (mbuf, v);
        }
      break;

    case 'i': /* G_VARIANT_TYPE_INT32 */
      padding_added = ensure_output_padding (mbuf, 4);
      if (value != NULL)
        {
          gint32 v = xvariant_get_int32 (value);
          g_memory_buffer_put_int32 (mbuf, v);
        }
      break;

    case 'u': /* G_VARIANT_TYPE_UINT32 */
      padding_added = ensure_output_padding (mbuf, 4);
      if (value != NULL)
        {
          xuint32_t v = xvariant_get_uint32 (value);
          g_memory_buffer_put_uint32 (mbuf, v);
        }
      break;

    case 'x': /* G_VARIANT_TYPE_INT64 */
      padding_added = ensure_output_padding (mbuf, 8);
      if (value != NULL)
        {
          gint64 v = xvariant_get_int64 (value);
          g_memory_buffer_put_int64 (mbuf, v);
        }
      break;

    case 't': /* G_VARIANT_TYPE_UINT64 */
      padding_added = ensure_output_padding (mbuf, 8);
      if (value != NULL)
        {
          xuint64_t v = xvariant_get_uint64 (value);
          g_memory_buffer_put_uint64 (mbuf, v);
        }
      break;

    case 'd': /* G_VARIANT_TYPE_DOUBLE */
      padding_added = ensure_output_padding (mbuf, 8);
      if (value != NULL)
        {
          union {
            xuint64_t v_uint64;
            xdouble_t v_double;
          } u;
          G_STATIC_ASSERT (sizeof (xdouble_t) == sizeof (xuint64_t));
          u.v_double = xvariant_get_double (value);
          g_memory_buffer_put_uint64 (mbuf, u.v_uint64);
        }
      break;

    case 's': /* G_VARIANT_TYPE_STRING */
      padding_added = ensure_output_padding (mbuf, 4);
      if (value != NULL)
        {
          xsize_t len;
          const xchar_t *v;
#ifndef G_DISABLE_ASSERT
          const xchar_t *end;
#endif

          v = xvariant_get_string (value, &len);
          g_assert (xutf8_validate (v, -1, &end) && (end == v + len));
          g_memory_buffer_put_uint32 (mbuf, len);
          g_memory_buffer_put_string (mbuf, v);
          g_memory_buffer_put_byte (mbuf, '\0');
        }
      break;

    case 'o': /* G_VARIANT_TYPE_OBJECT_PATH */
      padding_added = ensure_output_padding (mbuf, 4);
      if (value != NULL)
        {
          xsize_t len;
          const xchar_t *v = xvariant_get_string (value, &len);
          g_assert (xvariant_is_object_path (v));
          g_memory_buffer_put_uint32 (mbuf, len);
          g_memory_buffer_put_string (mbuf, v);
          g_memory_buffer_put_byte (mbuf, '\0');
        }
      break;

    case 'g': /* G_VARIANT_TYPE_SIGNATURE */
      if (value != NULL)
        {
          xsize_t len;
          const xchar_t *v = xvariant_get_string (value, &len);
          g_assert (xvariant_is_signature (v));
          g_memory_buffer_put_byte (mbuf, len);
          g_memory_buffer_put_string (mbuf, v);
          g_memory_buffer_put_byte (mbuf, '\0');
        }
      break;

    case 'h': /* G_VARIANT_TYPE_HANDLE */
      padding_added = ensure_output_padding (mbuf, 4);
      if (value != NULL)
        {
          gint32 v = xvariant_get_handle (value);
          g_memory_buffer_put_int32 (mbuf, v);
        }
      break;

    case 'a': /* G_VARIANT_TYPE_ARRAY */
      {
        const xvariant_type_t *element_type;
        xvariant_t *item;
        xvariant_iter_t iter;
        xoffset_t array_len_offset;
        xoffset_t array_payload_begin_offset;
        xoffset_t cur_offset;
        xsize_t array_len;
        xuint_t fixed_size;

        padding_added = ensure_output_padding (mbuf, 4);
        if (value != NULL)
          {
            /* array length - will be filled in later */
            array_len_offset = mbuf->valid_len;
            g_memory_buffer_put_uint32 (mbuf, 0xF00DFACE);

            /* From the D-Bus spec:
             *
             *   "A UINT32 giving the length of the array data in bytes,
             *    followed by alignment padding to the alignment boundary of
             *    the array element type, followed by each array element. The
             *    array length is from the end of the alignment padding to
             *    the end of the last element, i.e. it does not include the
             *    padding after the length, or any padding after the last
             *    element."
             *
             * Thus, we need to count how much padding the first element
             * contributes and subtract that from the array length.
             */
            array_payload_begin_offset = mbuf->valid_len;

            element_type = xvariant_type_element (type);
            fixed_size = get_type_fixed_size (element_type);

            if (xvariant_n_children (value) == 0)
              {
                xsize_t padding_added_for_item;
                if (!append_value_to_blob (NULL,
                                           element_type,
                                           mbuf,
                                           &padding_added_for_item,
                                           error))
                  goto fail;
                array_payload_begin_offset += padding_added_for_item;
              }
            else if (fixed_size != 0)
              {
                xvariant_t *use_value;

                if (g_memory_buffer_is_byteswapped (mbuf))
                  use_value = xvariant_byteswap (value);
                else
                  use_value = xvariant_ref (value);

                array_payload_begin_offset += ensure_output_padding (mbuf, fixed_size);

                array_len = xvariant_get_size (use_value);
                g_memory_buffer_write (mbuf, xvariant_get_data (use_value), array_len);
                xvariant_unref (use_value);
              }
            else
              {
                xuint_t n;
                n = 0;
                xvariant_iter_init (&iter, value);
                while ((item = xvariant_iter_next_value (&iter)) != NULL)
                  {
                    xsize_t padding_added_for_item;
                    if (!append_value_to_blob (item,
                                               xvariant_get_type (item),
                                               mbuf,
                                               &padding_added_for_item,
                                               error))
                      {
                        xvariant_unref (item);
                        goto fail;
                      }
                    xvariant_unref (item);
                    if (n == 0)
                      {
                        array_payload_begin_offset += padding_added_for_item;
                      }
                    n++;
                  }
              }

            cur_offset = mbuf->valid_len;
            array_len = cur_offset - array_payload_begin_offset;
            mbuf->pos = array_len_offset;

            g_memory_buffer_put_uint32 (mbuf, array_len);
            mbuf->pos = cur_offset;
          }
      }
      break;

    default:
      if (xvariant_type_is_dict_entry (type) || xvariant_type_is_tuple (type))
        {
          if (!xvariant_type_first (type))
            {
              g_set_error_literal (error,
                                   G_IO_ERROR,
                                   G_IO_ERROR_INVALID_ARGUMENT,
                                   _("Empty structures (tuples) are not allowed in D-Bus"));
              goto fail;
            }

          padding_added = ensure_output_padding (mbuf, 8);
          if (value != NULL)
            {
              xvariant_t *item;
              xvariant_iter_t iter;
              xvariant_iter_init (&iter, value);
              while ((item = xvariant_iter_next_value (&iter)) != NULL)
                {
                  if (!append_value_to_blob (item,
                                             xvariant_get_type (item),
                                             mbuf,
                                             NULL,
                                             error))
                    {
                      xvariant_unref (item);
                      goto fail;
                    }
                  xvariant_unref (item);
                }
            }
        }
      else if (xvariant_type_is_variant (type))
        {
          if (value != NULL)
            {
              xvariant_t *child;
              const xchar_t *signature;
              child = xvariant_get_child_value (value, 0);
              signature = xvariant_get_type_string (child);
              g_memory_buffer_put_byte (mbuf, strlen (signature));
              g_memory_buffer_put_string (mbuf, signature);
              g_memory_buffer_put_byte (mbuf, '\0');
              if (!append_value_to_blob (child,
                                         xvariant_get_type (child),
                                         mbuf,
                                         NULL,
                                         error))
                {
                  xvariant_unref (child);
                  goto fail;
                }
              xvariant_unref (child);
            }
        }
      else
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Error serializing xvariant_t with type string “%s” to the D-Bus wire format"),
                       xvariant_get_type_string (value));
          goto fail;
        }
      break;
    }

  if (out_padding_added != NULL)
    *out_padding_added = padding_added;

  return TRUE;

 fail:
  return FALSE;
}

static xboolean_t
append_body_to_blob (xvariant_t       *value,
                     GMemoryBuffer  *mbuf,
                     xerror_t        **error)
{
  xvariant_t *item;
  xvariant_iter_t iter;

  if (!xvariant_is_of_type (value, G_VARIANT_TYPE_TUPLE))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "Expected a tuple for the body of the xdbus_message_t.");
      goto fail;
    }

  xvariant_iter_init (&iter, value);
  while ((item = xvariant_iter_next_value (&iter)) != NULL)
    {
      if (!append_value_to_blob (item,
                                 xvariant_get_type (item),
                                 mbuf,
                                 NULL,
                                 error))
        {
          xvariant_unref (item);
          goto fail;
        }
      xvariant_unref (item);
    }
  return TRUE;

 fail:
  return FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_to_blob:
 * @message: A #xdbus_message_t.
 * @out_size: Return location for size of generated blob.
 * @capabilities: A #GDBusCapabilityFlags describing what protocol features are supported.
 * @error: Return location for error.
 *
 * Serializes @message to a blob. The byte order returned by
 * xdbus_message_get_byte_order() will be used.
 *
 * Returns: (array length=out_size) (transfer full): A pointer to a
 * valid binary D-Bus message of @out_size bytes generated by @message
 * or %NULL if @error is set. Free with g_free().
 *
 * Since: 2.26
 */
guchar *
xdbus_message_to_blob (xdbus_message_t          *message,
                        xsize_t                 *out_size,
                        GDBusCapabilityFlags   capabilities,
                        xerror_t               **error)
{
  GMemoryBuffer mbuf;
  guchar *ret;
  xsize_t size;
  xoffset_t body_len_offset;
  xoffset_t body_start_offset;
  xsize_t body_size;
  xvariant_t *header_fields;
  xvariant_builder_t builder;
  xhash_table_iter_t hash_iter;
  xpointer_t key;
  xvariant_t *header_value;
  xvariant_t *signature;
  const xchar_t *signature_str;
  xint_t num_fds_in_message;
  xint_t num_fds_according_to_header;

  /* TODO: check against @capabilities */

  ret = NULL;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  g_return_val_if_fail (out_size != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  memset (&mbuf, 0, sizeof (mbuf));
  mbuf.len = MIN_ARRAY_SIZE;
  mbuf.data = g_malloc (mbuf.len);

  mbuf.byte_order = G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN;
  switch (message->byte_order)
    {
    case G_DBUS_MESSAGE_BYTE_ORDER_BIG_ENDIAN:
      mbuf.byte_order = G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN;
      break;
    case G_DBUS_MESSAGE_BYTE_ORDER_LITTLE_ENDIAN:
      mbuf.byte_order = G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN;
      break;
    }

  /* Core header */
  g_memory_buffer_put_byte (&mbuf, (guchar) message->byte_order);
  g_memory_buffer_put_byte (&mbuf, message->type);
  g_memory_buffer_put_byte (&mbuf, message->flags);
  g_memory_buffer_put_byte (&mbuf, 1); /* major protocol version */
  body_len_offset = mbuf.valid_len;
  /* body length - will be filled in later */
  g_memory_buffer_put_uint32 (&mbuf, 0xF00DFACE);
  g_memory_buffer_put_uint32 (&mbuf, message->serial);

  num_fds_in_message = 0;
#ifdef G_OS_UNIX
  if (message->fd_list != NULL)
    num_fds_in_message = g_unix_fd_list_get_length (message->fd_list);
#endif
  num_fds_according_to_header = xdbus_message_get_num_unix_fds (message);
  if (num_fds_in_message != num_fds_according_to_header)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Number of file descriptors in message (%d) differs from header field (%d)"),
                   num_fds_in_message,
                   num_fds_according_to_header);
      goto out;
    }

  if (!validate_headers (message, error))
    {
      g_prefix_error (error, _("Cannot serialize message: "));
      goto out;
    }

  xvariant_builder_init (&builder, G_VARIANT_TYPE ("a{yv}"));
  xhash_table_iter_init (&hash_iter, message->headers);
  while (xhash_table_iter_next (&hash_iter, &key, (xpointer_t) &header_value))
    {
      xvariant_builder_add (&builder,
                             "{yv}",
                             (guchar) GPOINTER_TO_UINT (key),
                             header_value);
    }
  header_fields = xvariant_builder_end (&builder);

  if (!append_value_to_blob (header_fields,
                             xvariant_get_type (header_fields),
                             &mbuf,
                             NULL,
                             error))
    {
      xvariant_unref (header_fields);
      goto out;
    }
  xvariant_unref (header_fields);

  /* header size must be a multiple of 8 */
  ensure_output_padding (&mbuf, 8);

  body_start_offset = mbuf.valid_len;

  signature = xdbus_message_get_header (message, G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE);

  if (signature != NULL && !xvariant_is_of_type (signature, G_VARIANT_TYPE_SIGNATURE))
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Signature header found but is not of type signature"));
      goto out;
    }

  signature_str = NULL;
  if (signature != NULL)
      signature_str = xvariant_get_string (signature, NULL);
  if (message->body != NULL)
    {
      xchar_t *tupled_signature_str;
      if (signature == NULL)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Message body has signature “%s” but there is no signature header"),
                       xvariant_get_type_string (message->body));
          goto out;
        }
      tupled_signature_str = xstrdup_printf ("(%s)", signature_str);
      if (xstrcmp0 (tupled_signature_str, xvariant_get_type_string (message->body)) != 0)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Message body has type signature “%s” but signature in the header field is “%s”"),
                       xvariant_get_type_string (message->body), tupled_signature_str);
          g_free (tupled_signature_str);
          goto out;
        }
      g_free (tupled_signature_str);
      if (!append_body_to_blob (message->body, &mbuf, error))
        goto out;
    }
  else
    {
      if (signature != NULL && strlen (signature_str) > 0)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       _("Message body is empty but signature in the header field is “(%s)”"),
                       signature_str);
          goto out;
        }
    }

  /* OK, we're done writing the message - set the body length */
  size = mbuf.valid_len;
  body_size = size - body_start_offset;

  mbuf.pos = body_len_offset;

  g_memory_buffer_put_uint32 (&mbuf, body_size);

  *out_size = size;
  ret = (guchar *)mbuf.data;

 out:
  if (ret == NULL)
    g_free (mbuf.data);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static xuint32_t
get_uint32_header (xdbus_message_t            *message,
                   GDBusMessageHeaderField  header_field)
{
  xvariant_t *value;
  xuint32_t ret;

  ret = 0;
  value = xhash_table_lookup (message->headers, GUINT_TO_POINTER (header_field));
  if (value != NULL && xvariant_is_of_type (value, G_VARIANT_TYPE_UINT32))
    ret = xvariant_get_uint32 (value);

  return ret;
}

static const xchar_t *
get_string_header (xdbus_message_t            *message,
                   GDBusMessageHeaderField  header_field)
{
  xvariant_t *value;
  const xchar_t *ret;

  ret = NULL;
  value = xhash_table_lookup (message->headers, GUINT_TO_POINTER (header_field));
  if (value != NULL && xvariant_is_of_type (value, G_VARIANT_TYPE_STRING))
    ret = xvariant_get_string (value, NULL);

  return ret;
}

static const xchar_t *
get_object_path_header (xdbus_message_t            *message,
                        GDBusMessageHeaderField  header_field)
{
  xvariant_t *value;
  const xchar_t *ret;

  ret = NULL;
  value = xhash_table_lookup (message->headers, GUINT_TO_POINTER (header_field));
  if (value != NULL && xvariant_is_of_type (value, G_VARIANT_TYPE_OBJECT_PATH))
    ret = xvariant_get_string (value, NULL);

  return ret;
}

static const xchar_t *
get_signature_header (xdbus_message_t            *message,
                      GDBusMessageHeaderField  header_field)
{
  xvariant_t *value;
  const xchar_t *ret;

  ret = NULL;
  value = xhash_table_lookup (message->headers, GUINT_TO_POINTER (header_field));
  if (value != NULL && xvariant_is_of_type (value, G_VARIANT_TYPE_SIGNATURE))
    ret = xvariant_get_string (value, NULL);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
set_uint32_header (xdbus_message_t             *message,
                   GDBusMessageHeaderField   header_field,
                   xuint32_t                   value)
{
  xdbus_message_set_header (message,
                             header_field,
                             xvariant_new_uint32 (value));
}

static void
set_string_header (xdbus_message_t             *message,
                   GDBusMessageHeaderField   header_field,
                   const xchar_t              *value)
{
  xdbus_message_set_header (message,
                             header_field,
                             value == NULL ? NULL : xvariant_new_string (value));
}

static void
set_object_path_header (xdbus_message_t             *message,
                        GDBusMessageHeaderField   header_field,
                        const xchar_t              *value)
{
  xdbus_message_set_header (message,
                             header_field,
                             value == NULL ? NULL : xvariant_new_object_path (value));
}

static void
set_signature_header (xdbus_message_t             *message,
                      GDBusMessageHeaderField   header_field,
                      const xchar_t              *value)
{
  xdbus_message_set_header (message,
                             header_field,
                             value == NULL ? NULL : xvariant_new_signature (value));
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_reply_serial:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL header field.
 *
 * Returns: The value.
 *
 * Since: 2.26
 */
xuint32_t
xdbus_message_get_reply_serial (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), 0);
  return get_uint32_header (message, G_DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL);
}

/**
 * xdbus_message_set_reply_serial:
 * @message: A #xdbus_message_t.
 * @value: The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_reply_serial (xdbus_message_t  *message,
                                 xuint32_t        value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  set_uint32_header (message, G_DBUS_MESSAGE_HEADER_FIELD_REPLY_SERIAL, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_interface:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_INTERFACE header field.
 *
 * Returns: (nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_interface (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return get_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_INTERFACE);
}

/**
 * xdbus_message_set_interface:
 * @message: A #xdbus_message_t.
 * @value: (nullable): The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_INTERFACE header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_interface (xdbus_message_t  *message,
                              const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || g_dbus_is_interface_name (value));
  set_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_INTERFACE, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_member:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_MEMBER header field.
 *
 * Returns: (nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_member (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return get_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_MEMBER);
}

/**
 * xdbus_message_set_member:
 * @message: A #xdbus_message_t.
 * @value: (nullable): The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_MEMBER header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_member (xdbus_message_t  *message,
                           const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || g_dbus_is_member_name (value));
  set_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_MEMBER, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_path:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_PATH header field.
 *
 * Returns: (nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_path (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return get_object_path_header (message, G_DBUS_MESSAGE_HEADER_FIELD_PATH);
}

/**
 * xdbus_message_set_path:
 * @message: A #xdbus_message_t.
 * @value: (nullable): The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_PATH header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_path (xdbus_message_t  *message,
                         const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || xvariant_is_object_path (value));
  set_object_path_header (message, G_DBUS_MESSAGE_HEADER_FIELD_PATH, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_sender:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_SENDER header field.
 *
 * Returns: (nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_sender (xdbus_message_t *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return get_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_SENDER);
}

/**
 * xdbus_message_set_sender:
 * @message: A #xdbus_message_t.
 * @value: (nullable): The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_SENDER header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_sender (xdbus_message_t  *message,
                           const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || g_dbus_is_name (value));
  set_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_SENDER, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_destination:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_DESTINATION header field.
 *
 * Returns: (nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_destination (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return get_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_DESTINATION);
}

/**
 * xdbus_message_set_destination:
 * @message: A #xdbus_message_t.
 * @value: (nullable): The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_DESTINATION header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_destination (xdbus_message_t  *message,
                                const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || g_dbus_is_name (value));
  set_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_DESTINATION, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_error_name:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME header field.
 *
 * Returns: (nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_error_name (xdbus_message_t  *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  return get_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME);
}

/**
 * xdbus_message_set_error_name:
 * @message: (nullable): A #xdbus_message_t.
 * @value: The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_error_name (xdbus_message_t  *message,
                               const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || g_dbus_is_error_name (value));
  set_string_header (message, G_DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_signature:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE header field.
 *
 * This will always be non-%NULL, but may be an empty string.
 *
 * Returns: (not nullable): The value.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_signature (xdbus_message_t  *message)
{
  const xchar_t *ret;
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  ret = get_signature_header (message, G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE);
  if (ret == NULL)
    ret = "";
  return ret;
}

/**
 * xdbus_message_set_signature:
 * @message: A #xdbus_message_t.
 * @value: (nullable): The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_signature (xdbus_message_t  *message,
                              const xchar_t   *value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  g_return_if_fail (value == NULL || xvariant_is_signature (value));
  set_signature_header (message, G_DBUS_MESSAGE_HEADER_FIELD_SIGNATURE, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_arg0:
 * @message: A #xdbus_message_t.
 *
 * Convenience to get the first item in the body of @message.
 *
 * Returns: (nullable): The string item or %NULL if the first item in the body of
 * @message is not a string.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_message_get_arg0 (xdbus_message_t  *message)
{
  const xchar_t *ret;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);

  ret = NULL;

  if (message->body != NULL && xvariant_is_of_type (message->body, G_VARIANT_TYPE_TUPLE))
    {
      xvariant_t *item;
      item = xvariant_get_child_value (message->body, 0);
      if (xvariant_is_of_type (item, G_VARIANT_TYPE_STRING))
        ret = xvariant_get_string (item, NULL);
      xvariant_unref (item);
    }

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_get_num_unix_fds:
 * @message: A #xdbus_message_t.
 *
 * Convenience getter for the %G_DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS header field.
 *
 * Returns: The value.
 *
 * Since: 2.26
 */
xuint32_t
xdbus_message_get_num_unix_fds (xdbus_message_t *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), 0);
  return get_uint32_header (message, G_DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS);
}

/**
 * xdbus_message_set_num_unix_fds:
 * @message: A #xdbus_message_t.
 * @value: The value to set.
 *
 * Convenience setter for the %G_DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS header field.
 *
 * Since: 2.26
 */
void
xdbus_message_set_num_unix_fds (xdbus_message_t  *message,
                                 xuint32_t        value)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));
  set_uint32_header (message, G_DBUS_MESSAGE_HEADER_FIELD_NUM_UNIX_FDS, value);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_message_to_gerror:
 * @message: A #xdbus_message_t.
 * @error: The #xerror_t to set.
 *
 * If @message is not of type %G_DBUS_MESSAGE_TYPE_ERROR does
 * nothing and returns %FALSE.
 *
 * Otherwise this method encodes the error in @message as a #xerror_t
 * using g_dbus_error_set_dbus_error() using the information in the
 * %G_DBUS_MESSAGE_HEADER_FIELD_ERROR_NAME header field of @message as
 * well as the first string item in @message's body.
 *
 * Returns: %TRUE if @error was set, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
xdbus_message_to_gerror (xdbus_message_t   *message,
                          xerror_t        **error)
{
  xboolean_t ret;
  const xchar_t *error_name;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), FALSE);

  ret = FALSE;
  if (message->type != G_DBUS_MESSAGE_TYPE_ERROR)
    goto out;

  error_name = xdbus_message_get_error_name (message);
  if (error_name != NULL)
    {
      xvariant_t *body;

      body = xdbus_message_get_body (message);

      if (body != NULL && xvariant_is_of_type (body, G_VARIANT_TYPE ("(s)")))
        {
          const xchar_t *error_message;
          xvariant_get (body, "(&s)", &error_message);
          g_dbus_error_set_dbus_error (error,
                                       error_name,
                                       error_message,
                                       NULL);
        }
      else
        {
          /* these two situations are valid, yet pretty rare */
          if (body != NULL)
            {
              g_dbus_error_set_dbus_error (error,
                                           error_name,
                                           "",
                                           _("Error return with body of type “%s”"),
                                           xvariant_get_type_string (body));
            }
          else
            {
              g_dbus_error_set_dbus_error (error,
                                           error_name,
                                           "",
                                           _("Error return with empty body"));
            }
        }
    }
  else
    {
      /* TODO: this shouldn't happen - should check this at message serialization
       * time and disconnect the peer.
       */
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Error return without error-name header!");
    }

  ret = TRUE;

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t *
flags_to_string (xtype_t flags_type, xuint_t value)
{
  xstring_t *s;
  xflags_class_t *klass;
  xuint_t n;

  klass = xtype_class_ref (flags_type);
  s = xstring_new (NULL);
  for (n = 0; n < 32; n++)
    {
      if ((value & (1<<n)) != 0)
        {
          xflags_value_t *flags_value;
          flags_value = xflags_get_first_value (klass, (1<<n));
          if (s->len > 0)
            xstring_append_c (s, ',');
          if (flags_value != NULL)
            xstring_append (s, flags_value->value_nick);
          else
            xstring_append_printf (s, "unknown (bit %d)", n);
        }
    }
  if (s->len == 0)
    xstring_append (s, "none");
  xtype_class_unref (klass);
  return xstring_free (s, FALSE);
}

static xint_t
_sort_keys_func (xconstpointer a,
                 xconstpointer b)
{
  xint_t ia;
  xint_t ib;

  ia = GPOINTER_TO_INT (a);
  ib = GPOINTER_TO_INT (b);

  return ia - ib;
}

/**
 * xdbus_message_print:
 * @message: A #xdbus_message_t.
 * @indent: Indentation level.
 *
 * Produces a human-readable multi-line description of @message.
 *
 * The contents of the description has no ABI guarantees, the contents
 * and formatting is subject to change at any time. Typical output
 * looks something like this:
 * |[
 * Type:    method-call
 * Flags:   none
 * Version: 0
 * Serial:  4
 * Headers:
 *   path -> objectpath '/org/gtk/GDBus/test_object_t'
 *   interface -> 'org.gtk.GDBus.test_interface_t'
 *   member -> 'GimmeStdout'
 *   destination -> ':1.146'
 * Body: ()
 * UNIX File Descriptors:
 *   (none)
 * ]|
 * or
 * |[
 * Type:    method-return
 * Flags:   no-reply-expected
 * Version: 0
 * Serial:  477
 * Headers:
 *   reply-serial -> uint32 4
 *   destination -> ':1.159'
 *   sender -> ':1.146'
 *   num-unix-fds -> uint32 1
 * Body: ()
 * UNIX File Descriptors:
 *   fd 12: dev=0:10,mode=020620,ino=5,uid=500,gid=5,rdev=136:2,size=0,atime=1273085037,mtime=1273085851,ctime=1272982635
 * ]|
 *
 * Returns: (not nullable): A string that should be freed with g_free().
 *
 * Since: 2.26
 */
xchar_t *
xdbus_message_print (xdbus_message_t *message,
                      xuint_t         indent)
{
  xstring_t *str;
  xchar_t *s;
  xlist_t *keys;
  xlist_t *l;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);

  str = xstring_new (NULL);

  s = _g_dbus_enum_to_string (XTYPE_DBUS_MESSAGE_TYPE, message->type);
  xstring_append_printf (str, "%*sType:    %s\n", indent, "", s);
  g_free (s);
  s = flags_to_string (XTYPE_DBUS_MESSAGE_FLAGS, message->flags);
  xstring_append_printf (str, "%*sFlags:   %s\n", indent, "", s);
  g_free (s);
  xstring_append_printf (str, "%*sVersion: %d\n", indent, "", message->major_protocol_version);
  xstring_append_printf (str, "%*sSerial:  %d\n", indent, "", message->serial);

  xstring_append_printf (str, "%*sHeaders:\n", indent, "");
  keys = xhash_table_get_keys (message->headers);
  keys = xlist_sort (keys, _sort_keys_func);
  if (keys != NULL)
    {
      for (l = keys; l != NULL; l = l->next)
        {
          xint_t key = GPOINTER_TO_INT (l->data);
          xvariant_t *value;
          xchar_t *value_str;

          value = xhash_table_lookup (message->headers, l->data);
          g_assert (value != NULL);

          s = _g_dbus_enum_to_string (XTYPE_DBUS_MESSAGE_HEADER_FIELD, key);
          value_str = xvariant_print (value, TRUE);
          xstring_append_printf (str, "%*s  %s -> %s\n", indent, "", s, value_str);
          g_free (s);
          g_free (value_str);
        }
    }
  else
    {
      xstring_append_printf (str, "%*s  (none)\n", indent, "");
    }
  xlist_free (keys);
  xstring_append_printf (str, "%*sBody: ", indent, "");
  if (message->body != NULL)
    {
      xvariant_print_string (message->body,
                              str,
                              TRUE);
    }
  else
    {
      xstring_append (str, "()");
    }
  xstring_append (str, "\n");
#ifdef G_OS_UNIX
  xstring_append_printf (str, "%*sUNIX File Descriptors:\n", indent, "");
  if (message->fd_list != NULL)
    {
      xint_t num_fds;
      const xint_t *fds;
      xint_t n;

      fds = g_unix_fd_list_peek_fds (message->fd_list, &num_fds);
      if (num_fds > 0)
        {
          for (n = 0; n < num_fds; n++)
            {
              xstring_t *fs;
              struct stat statbuf;
              fs = xstring_new (NULL);
              if (fstat (fds[n], &statbuf) == 0)
                {
#ifndef MAJOR_MINOR_NOT_FOUND
                  xstring_append_printf (fs, "%s" "dev=%d:%d", fs->len > 0 ? "," : "",
                                          (xint_t) major (statbuf.st_dev), (xint_t) minor (statbuf.st_dev));
#endif
                  xstring_append_printf (fs, "%s" "mode=0%o", fs->len > 0 ? "," : "",
                                          (xuint_t) statbuf.st_mode);
                  xstring_append_printf (fs, "%s" "ino=%" G_GUINT64_FORMAT, fs->len > 0 ? "," : "",
                                          (xuint64_t) statbuf.st_ino);
                  xstring_append_printf (fs, "%s" "uid=%u", fs->len > 0 ? "," : "",
                                          (xuint_t) statbuf.st_uid);
                  xstring_append_printf (fs, "%s" "gid=%u", fs->len > 0 ? "," : "",
                                          (xuint_t) statbuf.st_gid);
#ifndef MAJOR_MINOR_NOT_FOUND
                  xstring_append_printf (fs, "%s" "rdev=%d:%d", fs->len > 0 ? "," : "",
                                          (xint_t) major (statbuf.st_rdev), (xint_t) minor (statbuf.st_rdev));
#endif
                  xstring_append_printf (fs, "%s" "size=%" G_GUINT64_FORMAT, fs->len > 0 ? "," : "",
                                          (xuint64_t) statbuf.st_size);
                  xstring_append_printf (fs, "%s" "atime=%" G_GUINT64_FORMAT, fs->len > 0 ? "," : "",
                                          (xuint64_t) statbuf.st_atime);
                  xstring_append_printf (fs, "%s" "mtime=%" G_GUINT64_FORMAT, fs->len > 0 ? "," : "",
                                          (xuint64_t) statbuf.st_mtime);
                  xstring_append_printf (fs, "%s" "ctime=%" G_GUINT64_FORMAT, fs->len > 0 ? "," : "",
                                          (xuint64_t) statbuf.st_ctime);
                }
              else
                {
                  int errsv = errno;
                  xstring_append_printf (fs, "(fstat failed: %s)", xstrerror (errsv));
                }
              xstring_append_printf (str, "%*s  fd %d: %s\n", indent, "", fds[n], fs->str);
              xstring_free (fs, TRUE);
            }
        }
      else
        {
          xstring_append_printf (str, "%*s  (empty)\n", indent, "");
        }
    }
  else
    {
      xstring_append_printf (str, "%*s  (none)\n", indent, "");
    }
#endif

  return xstring_free (str, FALSE);
}

/**
 * xdbus_message_get_locked:
 * @message: A #xdbus_message_t.
 *
 * Checks whether @message is locked. To monitor changes to this
 * value, conncet to the #xobject_t::notify signal to listen for changes
 * on the #xdbus_message_t:locked property.
 *
 * Returns: %TRUE if @message is locked, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
xdbus_message_get_locked (xdbus_message_t *message)
{
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), FALSE);
  return message->locked;
}

/**
 * xdbus_message_lock:
 * @message: A #xdbus_message_t.
 *
 * If @message is locked, does nothing. Otherwise locks the message.
 *
 * Since: 2.26
 */
void
xdbus_message_lock (xdbus_message_t *message)
{
  g_return_if_fail (X_IS_DBUS_MESSAGE (message));

  if (message->locked)
    goto out;

  message->locked = TRUE;
  xobject_notify (G_OBJECT (message), "locked");

 out:
  ;
}

/**
 * xdbus_message_copy:
 * @message: A #xdbus_message_t.
 * @error: Return location for error or %NULL.
 *
 * Copies @message. The copy is a deep copy and the returned
 * #xdbus_message_t is completely identical except that it is guaranteed
 * to not be locked.
 *
 * This operation can fail if e.g. @message contains file descriptors
 * and the per-process or system-wide open files limit is reached.
 *
 * Returns: (transfer full): A new #xdbus_message_t or %NULL if @error is set.
 *     Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_message_copy (xdbus_message_t  *message,
                     xerror_t       **error)
{
  xdbus_message_t *ret;
  xhash_table_iter_t iter;
  xpointer_t header_key;
  xvariant_t *header_value;

  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = xdbus_message_new ();
  ret->type                   = message->type;
  ret->flags                  = message->flags;
  ret->byte_order             = message->byte_order;
  ret->major_protocol_version = message->major_protocol_version;
  ret->serial                 = message->serial;

#ifdef G_OS_UNIX
  if (message->fd_list != NULL)
    {
      xint_t n;
      xint_t num_fds;
      const xint_t *fds;

      ret->fd_list = g_unix_fd_list_new ();
      fds = g_unix_fd_list_peek_fds (message->fd_list, &num_fds);
      for (n = 0; n < num_fds; n++)
        {
          if (g_unix_fd_list_append (ret->fd_list,
                                     fds[n],
                                     error) == -1)
            {
              xobject_unref (ret);
              ret = NULL;
              goto out;
            }
        }
    }
#endif

  /* see https://bugzilla.gnome.org/show_bug.cgi?id=624546#c8 for why it's fine
   * to just ref (as opposed to deep-copying) the xvariant_t instances
   */
  ret->body = message->body != NULL ? xvariant_ref (message->body) : NULL;
  xhash_table_iter_init (&iter, message->headers);
  while (xhash_table_iter_next (&iter, &header_key, (xpointer_t) &header_value))
    xhash_table_insert (ret->headers, header_key, xvariant_ref (header_value));

#ifdef G_OS_UNIX
 out:
#endif
  return ret;
}
