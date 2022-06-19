/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * giochannel.c: IO Channel abstraction
 * Copyright 1998 Owen Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include "giochannel.h"

#include "gstrfuncs.h"
#include "gtestutils.h"
#include "glibintl.h"


/**
 * SECTION:iochannels
 * @title: IO Channels
 * @short_description: portable support for using files, pipes and sockets
 * @see_also: g_io_add_watch(), g_io_add_watch_full(), xsource_remove(),
 *     #xmain_loop_t
 *
 * The #xio_channel_t data type aims to provide a portable method for
 * using file descriptors, pipes, and sockets, and integrating them
 * into the [main event loop][glib-The-Main-Event-Loop]. Currently,
 * full support is available on UNIX platforms, support for Windows
 * is only partially complete.
 *
 * To create a new #xio_channel_t on UNIX systems use
 * g_io_channel_unix_new(). This works for plain file descriptors,
 * pipes and sockets. Alternatively, a channel can be created for a
 * file in a system independent manner using g_io_channel_new_file().
 *
 * Once a #xio_channel_t has been created, it can be used in a generic
 * manner with the functions g_io_channel_read_chars(),
 * g_io_channel_write_chars(), g_io_channel_seek_position(), and
 * g_io_channel_shutdown().
 *
 * To add a #xio_channel_t to the [main event loop][glib-The-Main-Event-Loop],
 * use g_io_add_watch() or g_io_add_watch_full(). Here you specify which
 * events you are interested in on the #xio_channel_t, and provide a
 * function to be called whenever these events occur.
 *
 * #xio_channel_t instances are created with an initial reference count of 1.
 * g_io_channel_ref() and g_io_channel_unref() can be used to
 * increment or decrement the reference count respectively. When the
 * reference count falls to 0, the #xio_channel_t is freed. (Though it
 * isn't closed automatically, unless it was created using
 * g_io_channel_new_file().) Using g_io_add_watch() or
 * g_io_add_watch_full() increments a channel's reference count.
 *
 * The new functions g_io_channel_read_chars(),
 * g_io_channel_read_line(), g_io_channel_read_line_string(),
 * g_io_channel_read_to_end(), g_io_channel_write_chars(),
 * g_io_channel_seek_position(), and g_io_channel_flush() should not be
 * mixed with the deprecated functions g_io_channel_read(),
 * g_io_channel_write(), and g_io_channel_seek() on the same channel.
 **/

/**
 * xio_channel_t:
 *
 * A data structure representing an IO Channel. The fields should be
 * considered private and should only be accessed with the following
 * functions.
 **/

/**
 * GIOFuncs:
 * @io_read: reads raw bytes from the channel.  This is called from
 *           various functions such as g_io_channel_read_chars() to
 *           read raw bytes from the channel.  Encoding and buffering
 *           issues are dealt with at a higher level.
 * @io_write: writes raw bytes to the channel.  This is called from
 *            various functions such as g_io_channel_write_chars() to
 *            write raw bytes to the channel.  Encoding and buffering
 *            issues are dealt with at a higher level.
 * @io_seek: (optional) seeks the channel.  This is called from
 *           g_io_channel_seek() on channels that support it.
 * @io_close: closes the channel.  This is called from
 *            g_io_channel_close() after flushing the buffers.
 * @io_create_watch: creates a watch on the channel.  This call
 *                   corresponds directly to g_io_create_watch().
 * @io_free: called from g_io_channel_unref() when the channel needs to
 *           be freed.  This function must free the memory associated
 *           with the channel, including freeing the #xio_channel_t
 *           structure itself.  The channel buffers have been flushed
 *           and possibly @io_close has been called by the time this
 *           function is called.
 * @io_set_flags: sets the #GIOFlags on the channel.  This is called
 *                from g_io_channel_set_flags() with all flags except
 *                for %G_IO_FLAG_APPEND and %G_IO_FLAG_NONBLOCK masked
 *                out.
 * @io_get_flags: gets the #GIOFlags for the channel.  This function
 *                need only return the %G_IO_FLAG_APPEND and
 *                %G_IO_FLAG_NONBLOCK flags; g_io_channel_get_flags()
 *                automatically adds the others as appropriate.
 *
 * A table of functions used to handle different types of #xio_channel_t
 * in a generic way.
 **/

/**
 * GIOStatus:
 * @G_IO_STATUS_ERROR: An error occurred.
 * @G_IO_STATUS_NORMAL: Success.
 * @G_IO_STATUS_EOF: End of file.
 * @G_IO_STATUS_AGAIN: Resource temporarily unavailable.
 *
 * Statuses returned by most of the #GIOFuncs functions.
 **/

/**
 * GIOError:
 * @G_IO_ERROR_NONE: no error
 * @G_IO_ERROR_AGAIN: an EAGAIN error occurred
 * @G_IO_ERROR_INVAL: an EINVAL error occurred
 * @G_IO_ERROR_UNKNOWN: another error occurred
 *
 * #GIOError is only used by the deprecated functions
 * g_io_channel_read(), g_io_channel_write(), and g_io_channel_seek().
 **/

#define G_IO_NICE_BUF_SIZE	1024

/* This needs to be as wide as the largest character in any possible encoding */
#define MAX_CHAR_SIZE		10

/* Some simplifying macros, which reduce the need to worry whether the
 * buffers have been allocated. These also make USE_BUF () an lvalue,
 * which is used in g_io_channel_read_to_end ().
 */
#define USE_BUF(channel)	((channel)->encoding ? (channel)->encoded_read_buf \
				 : (channel)->read_buf)
#define BUF_LEN(string)		((string) ? (string)->len : 0)

static GIOError		g_io_error_get_from_xerror	(GIOStatus    status,
							 xerror_t      *err);
static void		g_io_channel_purge		(xio_channel_t  *channel);
static GIOStatus	g_io_channel_fill_buffer	(xio_channel_t  *channel,
							 xerror_t     **err);
static GIOStatus	g_io_channel_read_line_backend	(xio_channel_t  *channel,
							 xsize_t       *length,
							 xsize_t       *terminator_pos,
							 xerror_t     **error);

/**
 * g_io_channel_init:
 * @channel: a #xio_channel_t
 *
 * Initializes a #xio_channel_t struct.
 *
 * This is called by each of the above functions when creating a
 * #xio_channel_t, and so is not often needed by the application
 * programmer (unless you are creating a new type of #xio_channel_t).
 */
void
g_io_channel_init (xio_channel_t *channel)
{
  channel->ref_count = 1;
  channel->encoding = xstrdup ("UTF-8");
  channel->line_term = NULL;
  channel->line_term_len = 0;
  channel->buf_size = G_IO_NICE_BUF_SIZE;
  channel->read_cd = (GIConv) -1;
  channel->write_cd = (GIConv) -1;
  channel->read_buf = NULL; /* Lazy allocate buffers */
  channel->encoded_read_buf = NULL;
  channel->write_buf = NULL;
  channel->partial_write_buf[0] = '\0';
  channel->use_buffer = TRUE;
  channel->do_encode = FALSE;
  channel->close_on_unref = FALSE;
}

/**
 * g_io_channel_ref:
 * @channel: a #xio_channel_t
 *
 * Increments the reference count of a #xio_channel_t.
 *
 * Returns: the @channel that was passed in (since 2.6)
 */
xio_channel_t *
g_io_channel_ref (xio_channel_t *channel)
{
  xreturn_val_if_fail (channel != NULL, NULL);

  g_atomic_int_inc (&channel->ref_count);

  return channel;
}

/**
 * g_io_channel_unref:
 * @channel: a #xio_channel_t
 *
 * Decrements the reference count of a #xio_channel_t.
 */
void
g_io_channel_unref (xio_channel_t *channel)
{
  xboolean_t is_zero;

  g_return_if_fail (channel != NULL);

  is_zero = g_atomic_int_dec_and_test (&channel->ref_count);

  if (G_UNLIKELY (is_zero))
    {
      if (channel->close_on_unref)
        g_io_channel_shutdown (channel, TRUE, NULL);
      else
        g_io_channel_purge (channel);
      g_free (channel->encoding);
      if (channel->read_cd != (GIConv) -1)
        g_iconv_close (channel->read_cd);
      if (channel->write_cd != (GIConv) -1)
        g_iconv_close (channel->write_cd);
      g_free (channel->line_term);
      if (channel->read_buf)
        xstring_free (channel->read_buf, TRUE);
      if (channel->write_buf)
        xstring_free (channel->write_buf, TRUE);
      if (channel->encoded_read_buf)
        xstring_free (channel->encoded_read_buf, TRUE);
      channel->funcs->io_free (channel);
    }
}

static GIOError
g_io_error_get_from_xerror (GIOStatus  status,
			     xerror_t    *err)
{
  switch (status)
    {
      case G_IO_STATUS_NORMAL:
      case G_IO_STATUS_EOF:
        return G_IO_ERROR_NONE;
      case G_IO_STATUS_AGAIN:
        return G_IO_ERROR_AGAIN;
      case G_IO_STATUS_ERROR:
	xreturn_val_if_fail (err != NULL, G_IO_ERROR_UNKNOWN);

        if (err->domain != G_IO_CHANNEL_ERROR)
          return G_IO_ERROR_UNKNOWN;
        switch (err->code)
          {
            case G_IO_CHANNEL_ERROR_INVAL:
              return G_IO_ERROR_INVAL;
            default:
              return G_IO_ERROR_UNKNOWN;
          }
      default:
        g_assert_not_reached ();
    }
}

/**
 * g_io_channel_read:
 * @channel: a #xio_channel_t
 * @buf: a buffer to read the data into (which should be at least
 *       count bytes long)
 * @count: the number of bytes to read from the #xio_channel_t
 * @bytes_read: returns the number of bytes actually read
 *
 * Reads data from a #xio_channel_t.
 *
 * Returns: %G_IO_ERROR_NONE if the operation was successful.
 *
 * Deprecated:2.2: Use g_io_channel_read_chars() instead.
 **/
GIOError
g_io_channel_read (xio_channel_t *channel,
		   xchar_t      *buf,
		   xsize_t       count,
		   xsize_t      *bytes_read)
{
  xerror_t *err = NULL;
  GIOError error;
  GIOStatus status;

  xreturn_val_if_fail (channel != NULL, G_IO_ERROR_UNKNOWN);
  xreturn_val_if_fail (bytes_read != NULL, G_IO_ERROR_UNKNOWN);

  if (count == 0)
    {
      if (bytes_read)
        *bytes_read = 0;
      return G_IO_ERROR_NONE;
    }

  xreturn_val_if_fail (buf != NULL, G_IO_ERROR_UNKNOWN);

  status = channel->funcs->io_read (channel, buf, count, bytes_read, &err);

  error = g_io_error_get_from_xerror (status, err);

  if (err)
    xerror_free (err);

  return error;
}

/**
 * g_io_channel_write:
 * @channel:  a #xio_channel_t
 * @buf: the buffer containing the data to write
 * @count: the number of bytes to write
 * @bytes_written: the number of bytes actually written
 *
 * Writes data to a #xio_channel_t.
 *
 * Returns:  %G_IO_ERROR_NONE if the operation was successful.
 *
 * Deprecated:2.2: Use g_io_channel_write_chars() instead.
 **/
GIOError
g_io_channel_write (xio_channel_t  *channel,
		    const xchar_t *buf,
		    xsize_t        count,
		    xsize_t       *bytes_written)
{
  xerror_t *err = NULL;
  GIOError error;
  GIOStatus status;

  xreturn_val_if_fail (channel != NULL, G_IO_ERROR_UNKNOWN);
  xreturn_val_if_fail (bytes_written != NULL, G_IO_ERROR_UNKNOWN);

  status = channel->funcs->io_write (channel, buf, count, bytes_written, &err);

  error = g_io_error_get_from_xerror (status, err);

  if (err)
    xerror_free (err);

  return error;
}

/**
 * g_io_channel_seek:
 * @channel: a #xio_channel_t
 * @offset: an offset, in bytes, which is added to the position specified
 *          by @type
 * @type: the position in the file, which can be %G_SEEK_CUR (the current
 *        position), %G_SEEK_SET (the start of the file), or %G_SEEK_END
 *        (the end of the file)
 *
 * Sets the current position in the #xio_channel_t, similar to the standard
 * library function fseek().
 *
 * Returns: %G_IO_ERROR_NONE if the operation was successful.
 *
 * Deprecated:2.2: Use g_io_channel_seek_position() instead.
 **/
GIOError
g_io_channel_seek (xio_channel_t *channel,
		   sint64_t      offset,
		   xseek_type_t   type)
{
  xerror_t *err = NULL;
  GIOError error;
  GIOStatus status;

  xreturn_val_if_fail (channel != NULL, G_IO_ERROR_UNKNOWN);
  xreturn_val_if_fail (channel->is_seekable, G_IO_ERROR_UNKNOWN);

  switch (type)
    {
      case G_SEEK_CUR:
      case G_SEEK_SET:
      case G_SEEK_END:
        break;
      default:
        g_warning ("g_io_channel_seek: unknown seek type");
        return G_IO_ERROR_UNKNOWN;
    }

  status = channel->funcs->io_seek (channel, offset, type, &err);

  error = g_io_error_get_from_xerror (status, err);

  if (err)
    xerror_free (err);

  return error;
}

/* The function g_io_channel_new_file() is prototyped in both
 * giounix.c and giowin32.c, so we stick its documentation here.
 */

/**
 * g_io_channel_new_file:
 * @filename: (type filename): A string containing the name of a file
 * @mode: One of "r", "w", "a", "r+", "w+", "a+". These have
 *        the same meaning as in fopen()
 * @error: A location to return an error of type %XFILE_ERROR
 *
 * Open a file @filename as a #xio_channel_t using mode @mode. This
 * channel will be closed when the last reference to it is dropped,
 * so there is no need to call g_io_channel_close() (though doing
 * so will not cause problems, as long as no attempt is made to
 * access the channel after it is closed).
 *
 * Returns: A #xio_channel_t on success, %NULL on failure.
 **/

/**
 * g_io_channel_close:
 * @channel: A #xio_channel_t
 *
 * Close an IO channel. Any pending data to be written will be
 * flushed, ignoring errors. The channel will not be freed until the
 * last reference is dropped using g_io_channel_unref().
 *
 * Deprecated:2.2: Use g_io_channel_shutdown() instead.
 **/
void
g_io_channel_close (xio_channel_t *channel)
{
  xerror_t *err = NULL;

  g_return_if_fail (channel != NULL);

  g_io_channel_purge (channel);

  channel->funcs->io_close (channel, &err);

  if (err)
    { /* No way to return the error */
      g_warning ("Error closing channel: %s", err->message);
      xerror_free (err);
    }

  channel->close_on_unref = FALSE; /* Because we already did */
  channel->is_readable = FALSE;
  channel->is_writeable = FALSE;
  channel->is_seekable = FALSE;
}

/**
 * g_io_channel_shutdown:
 * @channel: a #xio_channel_t
 * @flush: if %TRUE, flush pending
 * @err: location to store a #GIOChannelError
 *
 * Close an IO channel. Any pending data to be written will be
 * flushed if @flush is %TRUE. The channel will not be freed until the
 * last reference is dropped using g_io_channel_unref().
 *
 * Returns: the status of the operation.
 **/
GIOStatus
g_io_channel_shutdown (xio_channel_t  *channel,
		       xboolean_t     flush,
		       xerror_t     **err)
{
  GIOStatus status, result;
  xerror_t *tmperr = NULL;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail (err == NULL || *err == NULL, G_IO_STATUS_ERROR);

  if (channel->write_buf && channel->write_buf->len > 0)
    {
      if (flush)
        {
          GIOFlags flags;

          /* Set the channel to blocking, to avoid a busy loop
           */
          flags = g_io_channel_get_flags (channel);
          /* Ignore any errors here, they're irrelevant */
          g_io_channel_set_flags (channel, flags & ~G_IO_FLAG_NONBLOCK, NULL);

          result = g_io_channel_flush (channel, &tmperr);
        }
      else
        result = G_IO_STATUS_NORMAL;

      xstring_truncate(channel->write_buf, 0);
    }
  else
    result = G_IO_STATUS_NORMAL;

  if (channel->partial_write_buf[0] != '\0')
    {
      if (flush)
        g_warning ("Partial character at end of write buffer not flushed.");
      channel->partial_write_buf[0] = '\0';
    }

  status = channel->funcs->io_close (channel, err);

  channel->close_on_unref = FALSE; /* Because we already did */
  channel->is_readable = FALSE;
  channel->is_writeable = FALSE;
  channel->is_seekable = FALSE;

  if (status != G_IO_STATUS_NORMAL)
    {
      g_clear_error (&tmperr);
      return status;
    }
  else if (result != G_IO_STATUS_NORMAL)
    {
      g_propagate_error (err, tmperr);
      return result;
    }
  else
    return G_IO_STATUS_NORMAL;
}

/* This function is used for the final flush on close or unref */
static void
g_io_channel_purge (xio_channel_t *channel)
{
  xerror_t *err = NULL;
  GIOStatus status G_GNUC_UNUSED;

  g_return_if_fail (channel != NULL);

  if (channel->write_buf && channel->write_buf->len > 0)
    {
      GIOFlags flags;

      /* Set the channel to blocking, to avoid a busy loop
       */
      flags = g_io_channel_get_flags (channel);
      g_io_channel_set_flags (channel, flags & ~G_IO_FLAG_NONBLOCK, NULL);

      status = g_io_channel_flush (channel, &err);

      if (err)
        { /* No way to return the error */
          g_warning ("Error flushing string: %s", err->message);
          xerror_free (err);
        }
    }

  /* Flush these in case anyone tries to close without unrefing */

  if (channel->read_buf)
    xstring_truncate (channel->read_buf, 0);
  if (channel->write_buf)
    xstring_truncate (channel->write_buf, 0);
  if (channel->encoding)
    {
      if (channel->encoded_read_buf)
        xstring_truncate (channel->encoded_read_buf, 0);

      if (channel->partial_write_buf[0] != '\0')
        {
          g_warning ("Partial character at end of write buffer not flushed.");
          channel->partial_write_buf[0] = '\0';
        }
    }
}

/**
 * g_io_create_watch:
 * @channel: a #xio_channel_t to watch
 * @condition: conditions to watch for
 *
 * Creates a #xsource_t that's dispatched when @condition is met for the
 * given @channel. For example, if condition is %G_IO_IN, the source will
 * be dispatched when there's data available for reading.
 *
 * The callback function invoked by the #xsource_t should be added with
 * xsource_set_callback(), but it has type #GIOFunc (not #xsource_func_t).
 *
 * g_io_add_watch() is a simpler interface to this same functionality, for
 * the case where you want to add the source to the default main loop context
 * at the default priority.
 *
 * On Windows, polling a #xsource_t created to watch a channel for a socket
 * puts the socket in non-blocking mode. This is a side-effect of the
 * implementation and unavoidable.
 *
 * Returns: a new #xsource_t
 */
xsource_t *
g_io_create_watch (xio_channel_t   *channel,
		   xio_condition_t  condition)
{
  xreturn_val_if_fail (channel != NULL, NULL);

  return channel->funcs->io_create_watch (channel, condition);
}

/**
 * g_io_add_watch_full: (rename-to g_io_add_watch)
 * @channel: a #xio_channel_t
 * @priority: the priority of the #xio_channel_t source
 * @condition: the condition to watch for
 * @func: the function to call when the condition is satisfied
 * @user_data: user data to pass to @func
 * @notify: the function to call when the source is removed
 *
 * Adds the #xio_channel_t into the default main loop context
 * with the given priority.
 *
 * This internally creates a main loop source using g_io_create_watch()
 * and attaches it to the main loop context with xsource_attach().
 * You can do these steps manually if you need greater control.
 *
 * Returns: the event source id
 */
xuint_t
g_io_add_watch_full (xio_channel_t    *channel,
		     xint_t           priority,
		     xio_condition_t   condition,
		     GIOFunc        func,
		     xpointer_t       user_data,
		     xdestroy_notify_t notify)
{
  xsource_t *source;
  xuint_t id;

  xreturn_val_if_fail (channel != NULL, 0);

  source = g_io_create_watch (channel, condition);

  if (priority != G_PRIORITY_DEFAULT)
    xsource_set_priority (source, priority);
  xsource_set_callback (source, (xsource_func_t)func, user_data, notify);

  id = xsource_attach (source, NULL);
  xsource_unref (source);

  return id;
}

/**
 * g_io_add_watch:
 * @channel: a #xio_channel_t
 * @condition: the condition to watch for
 * @func: the function to call when the condition is satisfied
 * @user_data: user data to pass to @func
 *
 * Adds the #xio_channel_t into the default main loop context
 * with the default priority.
 *
 * Returns: the event source id
 */
/**
 * GIOFunc:
 * @source: the #xio_channel_t event source
 * @condition: the condition which has been satisfied
 * @data: user data set in g_io_add_watch() or g_io_add_watch_full()
 *
 * Specifies the type of function passed to g_io_add_watch() or
 * g_io_add_watch_full(), which is called when the requested condition
 * on a #xio_channel_t is satisfied.
 *
 * Returns: the function should return %FALSE if the event source
 *          should be removed
 **/
/**
 * xio_condition_t:
 * @G_IO_IN: There is data to read.
 * @G_IO_OUT: Data can be written (without blocking).
 * @G_IO_PRI: There is urgent data to read.
 * @G_IO_ERR: Error condition.
 * @G_IO_HUP: Hung up (the connection has been broken, usually for
 *            pipes and sockets).
 * @G_IO_NVAL: Invalid request. The file descriptor is not open.
 *
 * A bitwise combination representing a condition to watch for on an
 * event source.
 **/
xuint_t
g_io_add_watch (xio_channel_t   *channel,
		xio_condition_t  condition,
		GIOFunc       func,
		xpointer_t      user_data)
{
  return g_io_add_watch_full (channel, G_PRIORITY_DEFAULT, condition, func, user_data, NULL);
}

/**
 * g_io_channel_get_buffer_condition:
 * @channel: A #xio_channel_t
 *
 * This function returns a #xio_condition_t depending on whether there
 * is data to be read/space to write data in the internal buffers in
 * the #xio_channel_t. Only the flags %G_IO_IN and %G_IO_OUT may be set.
 *
 * Returns: A #xio_condition_t
 **/
xio_condition_t
g_io_channel_get_buffer_condition (xio_channel_t *channel)
{
  xio_condition_t condition = 0;

  if (channel->encoding)
    {
      if (channel->encoded_read_buf && (channel->encoded_read_buf->len > 0))
        condition |= G_IO_IN; /* Only return if we have full characters */
    }
  else
    {
      if (channel->read_buf && (channel->read_buf->len > 0))
        condition |= G_IO_IN;
    }

  if (channel->write_buf && (channel->write_buf->len < channel->buf_size))
    condition |= G_IO_OUT;

  return condition;
}

/**
 * g_io_channel_error_from_errno:
 * @en: an `errno` error number, e.g. `EINVAL`
 *
 * Converts an `errno` error number to a #GIOChannelError.
 *
 * Returns: a #GIOChannelError error number, e.g.
 *      %G_IO_CHANNEL_ERROR_INVAL.
 **/
GIOChannelError
g_io_channel_error_from_errno (xint_t en)
{
#ifdef EAGAIN
  xreturn_val_if_fail (en != EAGAIN, G_IO_CHANNEL_ERROR_FAILED);
#endif

  switch (en)
    {
#ifdef EBADF
    case EBADF:
      g_warning ("Invalid file descriptor.");
      return G_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EFAULT
    case EFAULT:
      g_warning ("buffer_t outside valid address space.");
      return G_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EFBIG
    case EFBIG:
      return G_IO_CHANNEL_ERROR_FBIG;
#endif

#ifdef EINTR
    /* In general, we should catch EINTR before we get here,
     * but close() is allowed to return EINTR by POSIX, so
     * we need to catch it here; EINTR from close() is
     * unrecoverable, because it's undefined whether
     * the fd was actually closed or not, so we just return
     * a generic error code.
     */
    case EINTR:
      return G_IO_CHANNEL_ERROR_FAILED;
#endif

#ifdef EINVAL
    case EINVAL:
      return G_IO_CHANNEL_ERROR_INVAL;
#endif

#ifdef EIO
    case EIO:
      return G_IO_CHANNEL_ERROR_IO;
#endif

#ifdef EISDIR
    case EISDIR:
      return G_IO_CHANNEL_ERROR_ISDIR;
#endif

#ifdef ENOSPC
    case ENOSPC:
      return G_IO_CHANNEL_ERROR_NOSPC;
#endif

#ifdef ENXIO
    case ENXIO:
      return G_IO_CHANNEL_ERROR_NXIO;
#endif

#ifdef EOVERFLOW
#if EOVERFLOW != EFBIG
    case EOVERFLOW:
      return G_IO_CHANNEL_ERROR_OVERFLOW;
#endif
#endif

#ifdef EPIPE
    case EPIPE:
      return G_IO_CHANNEL_ERROR_PIPE;
#endif

    default:
      return G_IO_CHANNEL_ERROR_FAILED;
    }
}

/**
 * g_io_channel_set_buffer_size:
 * @channel: a #xio_channel_t
 * @size: the size of the buffer, or 0 to let GLib pick a good size
 *
 * Sets the buffer size.
 **/
void
g_io_channel_set_buffer_size (xio_channel_t *channel,
                              xsize_t       size)
{
  g_return_if_fail (channel != NULL);

  if (size == 0)
    size = G_IO_NICE_BUF_SIZE;

  if (size < MAX_CHAR_SIZE)
    size = MAX_CHAR_SIZE;

  channel->buf_size = size;
}

/**
 * g_io_channel_get_buffer_size:
 * @channel: a #xio_channel_t
 *
 * Gets the buffer size.
 *
 * Returns: the size of the buffer.
 **/
xsize_t
g_io_channel_get_buffer_size (xio_channel_t *channel)
{
  xreturn_val_if_fail (channel != NULL, 0);

  return channel->buf_size;
}

/**
 * g_io_channel_set_line_term:
 * @channel: a #xio_channel_t
 * @line_term: (nullable): The line termination string. Use %NULL for
 *             autodetect.  Autodetection breaks on "\n", "\r\n", "\r", "\0",
 *             and the Unicode paragraph separator. Autodetection should not be
 *             used for anything other than file-based channels.
 * @length: The length of the termination string. If -1 is passed, the
 *          string is assumed to be nul-terminated. This option allows
 *          termination strings with embedded nuls.
 *
 * This sets the string that #xio_channel_t uses to determine
 * where in the file a line break occurs.
 **/
void
g_io_channel_set_line_term (xio_channel_t	*channel,
                            const xchar_t	*line_term,
			    xint_t         length)
{
  xuint_t length_unsigned;

  g_return_if_fail (channel != NULL);
  g_return_if_fail (line_term == NULL || length != 0); /* Disallow "" */

  if (line_term == NULL)
    length_unsigned = 0;
  else if (length >= 0)
    length_unsigned = (xuint_t) length;
  else
    {
      /* FIXME: We’re constrained by line_term_len being a xuint_t here */
      xsize_t length_size = strlen (line_term);
      g_return_if_fail (length_size <= G_MAXUINT);
      length_unsigned = (xuint_t) length_size;
    }

  g_free (channel->line_term);
  channel->line_term = line_term ? g_memdup2 (line_term, length_unsigned) : NULL;
  channel->line_term_len = length_unsigned;
}

/**
 * g_io_channel_get_line_term:
 * @channel: a #xio_channel_t
 * @length: a location to return the length of the line terminator
 *
 * This returns the string that #xio_channel_t uses to determine
 * where in the file a line break occurs. A value of %NULL
 * indicates autodetection.
 *
 * Returns: The line termination string. This value
 *   is owned by GLib and must not be freed.
 **/
const xchar_t *
g_io_channel_get_line_term (xio_channel_t *channel,
			    xint_t       *length)
{
  xreturn_val_if_fail (channel != NULL, NULL);

  if (length)
    *length = channel->line_term_len;

  return channel->line_term;
}

/**
 * g_io_channel_set_flags:
 * @channel: a #xio_channel_t
 * @flags: the flags to set on the IO channel
 * @error: A location to return an error of type #GIOChannelError
 *
 * Sets the (writeable) flags in @channel to (@flags & %G_IO_FLAG_SET_MASK).
 *
 * Returns: the status of the operation.
 **/
/**
 * GIOFlags:
 * @G_IO_FLAG_APPEND: turns on append mode, corresponds to %O_APPEND
 *     (see the documentation of the UNIX open() syscall)
 * @G_IO_FLAG_NONBLOCK: turns on nonblocking mode, corresponds to
 *     %O_NONBLOCK/%O_NDELAY (see the documentation of the UNIX open()
 *     syscall)
 * @G_IO_FLAX_IS_READABLE: indicates that the io channel is readable.
 *     This flag cannot be changed.
 * @G_IO_FLAX_IS_WRITABLE: indicates that the io channel is writable.
 *     This flag cannot be changed.
 * @G_IO_FLAX_IS_WRITEABLE: a misspelled version of @G_IO_FLAX_IS_WRITABLE
 *     that existed before the spelling was fixed in GLib 2.30. It is kept
 *     here for compatibility reasons. Deprecated since 2.30
 * @G_IO_FLAX_IS_SEEKABLE: indicates that the io channel is seekable,
 *     i.e. that g_io_channel_seek_position() can be used on it.
 *     This flag cannot be changed.
 * @G_IO_FLAG_MASK: the mask that specifies all the valid flags.
 * @G_IO_FLAG_GET_MASK: the mask of the flags that are returned from
 *     g_io_channel_get_flags()
 * @G_IO_FLAG_SET_MASK: the mask of the flags that the user can modify
 *     with g_io_channel_set_flags()
 *
 * Specifies properties of a #xio_channel_t. Some of the flags can only be
 * read with g_io_channel_get_flags(), but not changed with
 * g_io_channel_set_flags().
 */
GIOStatus
g_io_channel_set_flags (xio_channel_t  *channel,
                        GIOFlags     flags,
                        xerror_t     **error)
{
  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);

  return (*channel->funcs->io_set_flags) (channel,
					  flags & G_IO_FLAG_SET_MASK,
					  error);
}

/**
 * g_io_channel_get_flags:
 * @channel: a #xio_channel_t
 *
 * Gets the current flags for a #xio_channel_t, including read-only
 * flags such as %G_IO_FLAX_IS_READABLE.
 *
 * The values of the flags %G_IO_FLAX_IS_READABLE and %G_IO_FLAX_IS_WRITABLE
 * are cached for internal use by the channel when it is created.
 * If they should change at some later point (e.g. partial shutdown
 * of a socket with the UNIX shutdown() function), the user
 * should immediately call g_io_channel_get_flags() to update
 * the internal values of these flags.
 *
 * Returns: the flags which are set on the channel
 **/
GIOFlags
g_io_channel_get_flags (xio_channel_t *channel)
{
  GIOFlags flags;

  xreturn_val_if_fail (channel != NULL, 0);

  flags = (* channel->funcs->io_get_flags) (channel);

  /* Cross implementation code */

  if (channel->is_seekable)
    flags |= G_IO_FLAX_IS_SEEKABLE;
  if (channel->is_readable)
    flags |= G_IO_FLAX_IS_READABLE;
  if (channel->is_writeable)
    flags |= G_IO_FLAX_IS_WRITABLE;

  return flags;
}

/**
 * g_io_channel_set_close_on_unref:
 * @channel: a #xio_channel_t
 * @do_close: Whether to close the channel on the final unref of
 *            the xio_channel_t data structure.
 *
 * Whether to close the channel on the final unref of the #xio_channel_t
 * data structure. The default value of this is %TRUE for channels
 * created by g_io_channel_new_file (), and %FALSE for all other channels.
 *
 * Setting this flag to %TRUE for a channel you have already closed
 * can cause problems when the final reference to the #xio_channel_t is dropped.
 **/
void
g_io_channel_set_close_on_unref	(xio_channel_t *channel,
				 xboolean_t    do_close)
{
  g_return_if_fail (channel != NULL);

  channel->close_on_unref = do_close;
}

/**
 * g_io_channel_get_close_on_unref:
 * @channel: a #xio_channel_t.
 *
 * Returns whether the file/socket/whatever associated with @channel
 * will be closed when @channel receives its final unref and is
 * destroyed. The default value of this is %TRUE for channels created
 * by g_io_channel_new_file (), and %FALSE for all other channels.
 *
 * Returns: %TRUE if the channel will be closed, %FALSE otherwise.
 **/
xboolean_t
g_io_channel_get_close_on_unref	(xio_channel_t *channel)
{
  xreturn_val_if_fail (channel != NULL, FALSE);

  return channel->close_on_unref;
}

/**
 * g_io_channel_seek_position:
 * @channel: a #xio_channel_t
 * @offset: The offset in bytes from the position specified by @type
 * @type: a #xseek_type_t. The type %G_SEEK_CUR is only allowed in those
 *                      cases where a call to g_io_channel_set_encoding ()
 *                      is allowed. See the documentation for
 *                      g_io_channel_set_encoding () for details.
 * @error: A location to return an error of type #GIOChannelError
 *
 * Replacement for g_io_channel_seek() with the new API.
 *
 * Returns: the status of the operation.
 **/
/**
 * xseek_type_t:
 * @G_SEEK_CUR: the current position in the file.
 * @G_SEEK_SET: the start of the file.
 * @G_SEEK_END: the end of the file.
 *
 * An enumeration specifying the base position for a
 * g_io_channel_seek_position() operation.
 **/
GIOStatus
g_io_channel_seek_position (xio_channel_t  *channel,
                            sint64_t       offset,
                            xseek_type_t    type,
                            xerror_t     **error)
{
  GIOStatus status;

  /* For files, only one of the read and write buffers can contain data.
   * For sockets, both can contain data.
   */

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_seekable, G_IO_STATUS_ERROR);

  switch (type)
    {
      case G_SEEK_CUR: /* The user is seeking relative to the head of the buffer */
        if (channel->use_buffer)
          {
            if (channel->do_encode && channel->encoded_read_buf
                && channel->encoded_read_buf->len > 0)
              {
                g_warning ("Seek type G_SEEK_CUR not allowed for this"
                  " channel's encoding.");
                return G_IO_STATUS_ERROR;
              }
          if (channel->read_buf)
            offset -= channel->read_buf->len;
          if (channel->encoded_read_buf)
            {
              xassert (channel->encoded_read_buf->len == 0 || !channel->do_encode);

              /* If there's anything here, it's because the encoding is UTF-8,
               * so we can just subtract the buffer length, the same as for
               * the unencoded data.
               */

              offset -= channel->encoded_read_buf->len;
            }
          }
        break;
      case G_SEEK_SET:
      case G_SEEK_END:
        break;
      default:
        g_warning ("g_io_channel_seek_position: unknown seek type");
        return G_IO_STATUS_ERROR;
    }

  if (channel->use_buffer)
    {
      status = g_io_channel_flush (channel, error);
      if (status != G_IO_STATUS_NORMAL)
        return status;
    }

  status = channel->funcs->io_seek (channel, offset, type, error);

  if ((status == G_IO_STATUS_NORMAL) && (channel->use_buffer))
    {
      if (channel->read_buf)
        xstring_truncate (channel->read_buf, 0);

      /* Conversion state no longer matches position in file */
      if (channel->read_cd != (GIConv) -1)
        g_iconv (channel->read_cd, NULL, NULL, NULL, NULL);
      if (channel->write_cd != (GIConv) -1)
        g_iconv (channel->write_cd, NULL, NULL, NULL, NULL);

      if (channel->encoded_read_buf)
        {
          xassert (channel->encoded_read_buf->len == 0 || !channel->do_encode);
          xstring_truncate (channel->encoded_read_buf, 0);
        }

      if (channel->partial_write_buf[0] != '\0')
        {
          g_warning ("Partial character at end of write buffer not flushed.");
          channel->partial_write_buf[0] = '\0';
        }
    }

  return status;
}

/**
 * g_io_channel_flush:
 * @channel: a #xio_channel_t
 * @error: location to store an error of type #GIOChannelError
 *
 * Flushes the write buffer for the xio_channel_t.
 *
 * Returns: the status of the operation: One of
 *   %G_IO_STATUS_NORMAL, %G_IO_STATUS_AGAIN, or
 *   %G_IO_STATUS_ERROR.
 **/
GIOStatus
g_io_channel_flush (xio_channel_t	*channel,
		    xerror_t     **error)
{
  GIOStatus status;
  xsize_t this_time = 1, bytes_written = 0;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL), G_IO_STATUS_ERROR);

  if (channel->write_buf == NULL || channel->write_buf->len == 0)
    return G_IO_STATUS_NORMAL;

  do
    {
      xassert (this_time > 0);

      status = channel->funcs->io_write (channel,
					 channel->write_buf->str + bytes_written,
					 channel->write_buf->len - bytes_written,
					 &this_time, error);
      bytes_written += this_time;
    }
  while ((bytes_written < channel->write_buf->len)
         && (status == G_IO_STATUS_NORMAL));

  xstring_erase (channel->write_buf, 0, bytes_written);

  return status;
}

/**
 * g_io_channel_set_buffered:
 * @channel: a #xio_channel_t
 * @buffered: whether to set the channel buffered or unbuffered
 *
 * The buffering state can only be set if the channel's encoding
 * is %NULL. For any other encoding, the channel must be buffered.
 *
 * A buffered channel can only be set unbuffered if the channel's
 * internal buffers have been flushed. Newly created channels or
 * channels which have returned %G_IO_STATUS_EOF
 * not require such a flush. For write-only channels, a call to
 * g_io_channel_flush () is sufficient. For all other channels,
 * the buffers may be flushed by a call to g_io_channel_seek_position ().
 * This includes the possibility of seeking with seek type %G_SEEK_CUR
 * and an offset of zero. Note that this means that socket-based
 * channels cannot be set unbuffered once they have had data
 * read from them.
 *
 * On unbuffered channels, it is safe to mix read and write
 * calls from the new and old APIs, if this is necessary for
 * maintaining old code.
 *
 * The default state of the channel is buffered.
 **/
void
g_io_channel_set_buffered (xio_channel_t *channel,
                           xboolean_t    buffered)
{
  g_return_if_fail (channel != NULL);

  if (channel->encoding != NULL)
    {
      g_warning ("Need to have NULL encoding to set the buffering state of the "
                 "channel.");
      return;
    }

  g_return_if_fail (!channel->read_buf || channel->read_buf->len == 0);
  g_return_if_fail (!channel->write_buf || channel->write_buf->len == 0);

  channel->use_buffer = buffered;
}

/**
 * g_io_channel_get_buffered:
 * @channel: a #xio_channel_t
 *
 * Returns whether @channel is buffered.
 *
 * Return Value: %TRUE if the @channel is buffered.
 **/
xboolean_t
g_io_channel_get_buffered (xio_channel_t *channel)
{
  xreturn_val_if_fail (channel != NULL, FALSE);

  return channel->use_buffer;
}

/**
 * g_io_channel_set_encoding:
 * @channel: a #xio_channel_t
 * @encoding: (nullable): the encoding type
 * @error: location to store an error of type #GConvertError
 *
 * Sets the encoding for the input/output of the channel.
 * The internal encoding is always UTF-8. The default encoding
 * for the external file is UTF-8.
 *
 * The encoding %NULL is safe to use with binary data.
 *
 * The encoding can only be set if one of the following conditions
 * is true:
 *
 * - The channel was just created, and has not been written to or read from yet.
 *
 * - The channel is write-only.
 *
 * - The channel is a file, and the file pointer was just repositioned
 *   by a call to g_io_channel_seek_position(). (This flushes all the
 *   internal buffers.)
 *
 * - The current encoding is %NULL or UTF-8.
 *
 * - One of the (new API) read functions has just returned %G_IO_STATUS_EOF
 *   (or, in the case of g_io_channel_read_to_end(), %G_IO_STATUS_NORMAL).
 *
 * -  One of the functions g_io_channel_read_chars() or
 *    g_io_channel_read_unichar() has returned %G_IO_STATUS_AGAIN or
 *    %G_IO_STATUS_ERROR. This may be useful in the case of
 *    %G_CONVERT_ERROR_ILLEGAL_SEQUENCE.
 *    Returning one of these statuses from g_io_channel_read_line(),
 *    g_io_channel_read_line_string(), or g_io_channel_read_to_end()
 *    does not guarantee that the encoding can be changed.
 *
 * Channels which do not meet one of the above conditions cannot call
 * g_io_channel_seek_position() with an offset of %G_SEEK_CUR, and, if
 * they are "seekable", cannot call g_io_channel_write_chars() after
 * calling one of the API "read" functions.
 *
 * Return Value: %G_IO_STATUS_NORMAL if the encoding was successfully set
 */
GIOStatus
g_io_channel_set_encoding (xio_channel_t	*channel,
                           const xchar_t	*encoding,
			   xerror_t      **error)
{
  GIConv read_cd, write_cd;
#ifndef G_DISABLE_ASSERT
  xboolean_t did_encode;
#endif

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL), G_IO_STATUS_ERROR);

  /* Make sure the encoded buffers are empty */

  xreturn_val_if_fail (!channel->do_encode || !channel->encoded_read_buf ||
			channel->encoded_read_buf->len == 0, G_IO_STATUS_ERROR);

  if (!channel->use_buffer)
    {
      g_warning ("Need to set the channel buffered before setting the encoding.");
      g_warning ("Assuming this is what you meant and acting accordingly.");

      channel->use_buffer = TRUE;
    }

  if (channel->partial_write_buf[0] != '\0')
    {
      g_warning ("Partial character at end of write buffer not flushed.");
      channel->partial_write_buf[0] = '\0';
    }

#ifndef G_DISABLE_ASSERT
  did_encode = channel->do_encode;
#endif

  if (!encoding || strcmp (encoding, "UTF8") == 0 || strcmp (encoding, "UTF-8") == 0)
    {
      channel->do_encode = FALSE;
      read_cd = write_cd = (GIConv) -1;
    }
  else
    {
      xint_t err = 0;
      const xchar_t *from_enc = NULL, *to_enc = NULL;

      if (channel->is_readable)
        {
          read_cd = g_iconv_open ("UTF-8", encoding);

          if (read_cd == (GIConv) -1)
            {
              err = errno;
              from_enc = encoding;
              to_enc = "UTF-8";
            }
        }
      else
        read_cd = (GIConv) -1;

      if (channel->is_writeable && err == 0)
        {
          write_cd = g_iconv_open (encoding, "UTF-8");

          if (write_cd == (GIConv) -1)
            {
              err = errno;
              from_enc = "UTF-8";
              to_enc = encoding;
            }
        }
      else
        write_cd = (GIConv) -1;

      if (err != 0)
        {
          xassert (from_enc);
          xassert (to_enc);

          if (err == EINVAL)
            g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_NO_CONVERSION,
                         _("Conversion from character set “%s” to “%s” is not supported"),
                         from_enc, to_enc);
          else
            g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                         _("Could not open converter from “%s” to “%s”: %s"),
                         from_enc, to_enc, xstrerror (err));

          if (read_cd != (GIConv) -1)
            g_iconv_close (read_cd);
          if (write_cd != (GIConv) -1)
            g_iconv_close (write_cd);

          return G_IO_STATUS_ERROR;
        }

      channel->do_encode = TRUE;
    }

  /* The encoding is ok, so set the fields in channel */

  if (channel->read_cd != (GIConv) -1)
    g_iconv_close (channel->read_cd);
  if (channel->write_cd != (GIConv) -1)
    g_iconv_close (channel->write_cd);

  if (channel->encoded_read_buf && channel->encoded_read_buf->len > 0)
    {
      xassert (!did_encode); /* Encoding UTF-8, NULL doesn't use encoded_read_buf */

      /* This is just validated UTF-8, so we can copy it back into read_buf
       * so it can be encoded in whatever the new encoding is.
       */

      xstring_prepend_len (channel->read_buf, channel->encoded_read_buf->str,
                            channel->encoded_read_buf->len);
      xstring_truncate (channel->encoded_read_buf, 0);
    }

  channel->read_cd = read_cd;
  channel->write_cd = write_cd;

  g_free (channel->encoding);
  channel->encoding = xstrdup (encoding);

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_get_encoding:
 * @channel: a #xio_channel_t
 *
 * Gets the encoding for the input/output of the channel.
 * The internal encoding is always UTF-8. The encoding %NULL
 * makes the channel safe for binary data.
 *
 * Returns: A string containing the encoding, this string is
 *   owned by GLib and must not be freed.
 **/
const xchar_t *
g_io_channel_get_encoding (xio_channel_t *channel)
{
  xreturn_val_if_fail (channel != NULL, NULL);

  return channel->encoding;
}

static GIOStatus
g_io_channel_fill_buffer (xio_channel_t  *channel,
                          xerror_t     **err)
{
  xsize_t read_size, cur_len, oldlen;
  GIOStatus status;

  if (channel->is_seekable && channel->write_buf && channel->write_buf->len > 0)
    {
      status = g_io_channel_flush (channel, err);
      if (status != G_IO_STATUS_NORMAL)
        return status;
    }
  if (channel->is_seekable && channel->partial_write_buf[0] != '\0')
    {
      g_warning ("Partial character at end of write buffer not flushed.");
      channel->partial_write_buf[0] = '\0';
    }

  if (!channel->read_buf)
    channel->read_buf = xstring_sized_new (channel->buf_size);

  cur_len = channel->read_buf->len;

  xstring_set_size (channel->read_buf, channel->read_buf->len + channel->buf_size);

  status = channel->funcs->io_read (channel, channel->read_buf->str + cur_len,
                                    channel->buf_size, &read_size, err);

  xassert ((status == G_IO_STATUS_NORMAL) || (read_size == 0));

  xstring_truncate (channel->read_buf, read_size + cur_len);

  if ((status != G_IO_STATUS_NORMAL) &&
      ((status != G_IO_STATUS_EOF) || (channel->read_buf->len == 0)))
    return status;

  xassert (channel->read_buf->len > 0);

  if (channel->encoded_read_buf)
    oldlen = channel->encoded_read_buf->len;
  else
    {
      oldlen = 0;
      if (channel->encoding)
        channel->encoded_read_buf = xstring_sized_new (channel->buf_size);
    }

  if (channel->do_encode)
    {
      xsize_t errnum, inbytes_left, outbytes_left;
      xchar_t *inbuf, *outbuf;
      int errval;

      xassert (channel->encoded_read_buf);

reencode:

      inbytes_left = channel->read_buf->len;
      outbytes_left = MAX (channel->read_buf->len,
                           channel->encoded_read_buf->allocated_len
                           - channel->encoded_read_buf->len - 1); /* 1 for NULL */
      outbytes_left = MAX (outbytes_left, 6);

      inbuf = channel->read_buf->str;
      xstring_set_size (channel->encoded_read_buf,
                         channel->encoded_read_buf->len + outbytes_left);
      outbuf = channel->encoded_read_buf->str + channel->encoded_read_buf->len
               - outbytes_left;

      errnum = g_iconv (channel->read_cd, &inbuf, &inbytes_left,
			&outbuf, &outbytes_left);
      errval = errno;

      xassert (inbuf + inbytes_left == channel->read_buf->str
                + channel->read_buf->len);
      xassert (outbuf + outbytes_left == channel->encoded_read_buf->str
                + channel->encoded_read_buf->len);

      xstring_erase (channel->read_buf, 0,
		      channel->read_buf->len - inbytes_left);
      xstring_truncate (channel->encoded_read_buf,
			 channel->encoded_read_buf->len - outbytes_left);

      if (errnum == (xsize_t) -1)
        {
          switch (errval)
            {
              case EINVAL:
                if ((oldlen == channel->encoded_read_buf->len)
                  && (status == G_IO_STATUS_EOF))
                  status = G_IO_STATUS_EOF;
                else
                  status = G_IO_STATUS_NORMAL;
                break;
              case E2BIG:
                /* buffer_t size at least 6, wrote at least on character */
                xassert (inbuf != channel->read_buf->str);
                goto reencode;
              case EILSEQ:
                if (oldlen < channel->encoded_read_buf->len)
                  status = G_IO_STATUS_NORMAL;
                else
                  {
                    g_set_error_literal (err, G_CONVERT_ERROR,
                      G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                      _("Invalid byte sequence in conversion input"));
                    return G_IO_STATUS_ERROR;
                  }
                break;
              default:
                xassert (errval != EBADF); /* The converter should be open */
                g_set_error (err, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                  _("Error during conversion: %s"), xstrerror (errval));
                return G_IO_STATUS_ERROR;
            }
        }
      xassert ((status != G_IO_STATUS_NORMAL)
               || (channel->encoded_read_buf->len > 0));
    }
  else if (channel->encoding) /* UTF-8 */
    {
      xchar_t *nextchar, *lastchar;

      xassert (channel->encoded_read_buf);

      nextchar = channel->read_buf->str;
      lastchar = channel->read_buf->str + channel->read_buf->len;

      while (nextchar < lastchar)
        {
          xunichar_t val_char;

          val_char = xutf8_get_char_validated (nextchar, lastchar - nextchar);

          switch (val_char)
            {
              case -2:
                /* stop, leave partial character in buffer */
                lastchar = nextchar;
                break;
              case -1:
                if (oldlen < channel->encoded_read_buf->len)
                  status = G_IO_STATUS_NORMAL;
                else
                  {
                    g_set_error_literal (err, G_CONVERT_ERROR,
                      G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                      _("Invalid byte sequence in conversion input"));
                    status = G_IO_STATUS_ERROR;
                  }
                lastchar = nextchar;
                break;
              default:
                nextchar = xutf8_next_char (nextchar);
                break;
            }
        }

      if (lastchar > channel->read_buf->str)
        {
          xint_t copy_len = lastchar - channel->read_buf->str;

          xstring_append_len (channel->encoded_read_buf, channel->read_buf->str,
                               copy_len);
          xstring_erase (channel->read_buf, 0, copy_len);
        }
    }

  return status;
}

/**
 * g_io_channel_read_line:
 * @channel: a #xio_channel_t
 * @str_return: (out): The line read from the #xio_channel_t, including the
 *              line terminator. This data should be freed with g_free()
 *              when no longer needed. This is a nul-terminated string.
 *              If a @length of zero is returned, this will be %NULL instead.
 * @length: (out) (optional): location to store length of the read data, or %NULL
 * @terminator_pos: (out) (optional): location to store position of line terminator, or %NULL
 * @error: A location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads a line, including the terminating character(s),
 * from a #xio_channel_t into a newly-allocated string.
 * @str_return will contain allocated memory if the return
 * is %G_IO_STATUS_NORMAL.
 *
 * Returns: the status of the operation.
 **/
GIOStatus
g_io_channel_read_line (xio_channel_t  *channel,
                        xchar_t      **str_return,
                        xsize_t       *length,
			xsize_t       *terminator_pos,
		        xerror_t     **error)
{
  GIOStatus status;
  xsize_t got_length;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail (str_return != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  status = g_io_channel_read_line_backend (channel, &got_length, terminator_pos, error);

  if (length && status != G_IO_STATUS_ERROR)
    *length = got_length;

  if (status == G_IO_STATUS_NORMAL)
    {
      xchar_t *line;

      /* Copy the read bytes (including any embedded nuls) and nul-terminate.
       * `USE_BUF (channel)->str` is guaranteed to be nul-terminated as it’s a
       * #xstring_t, so it’s safe to call g_memdup2() with +1 length to allocate
       * a nul-terminator. */
      xassert (USE_BUF (channel));
      line = g_memdup2 (USE_BUF (channel)->str, got_length + 1);
      line[got_length] = '\0';
      *str_return = g_steal_pointer (&line);
      xstring_erase (USE_BUF (channel), 0, got_length);
    }
  else
    *str_return = NULL;

  return status;
}

/**
 * g_io_channel_read_line_string:
 * @channel: a #xio_channel_t
 * @buffer: a #xstring_t into which the line will be written.
 *          If @buffer already contains data, the old data will
 *          be overwritten.
 * @terminator_pos: (nullable): location to store position of line terminator, or %NULL
 * @error: a location to store an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads a line from a #xio_channel_t, using a #xstring_t as a buffer.
 *
 * Returns: the status of the operation.
 **/
GIOStatus
g_io_channel_read_line_string (xio_channel_t  *channel,
                               xstring_t	   *buffer,
			       xsize_t       *terminator_pos,
                               xerror_t	  **error)
{
  xsize_t length;
  GIOStatus status;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail (buffer != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  if (buffer->len > 0)
    xstring_truncate (buffer, 0); /* clear out the buffer */

  status = g_io_channel_read_line_backend (channel, &length, terminator_pos, error);

  if (status == G_IO_STATUS_NORMAL)
    {
      xassert (USE_BUF (channel));
      xstring_append_len (buffer, USE_BUF (channel)->str, length);
      xstring_erase (USE_BUF (channel), 0, length);
    }

  return status;
}


static GIOStatus
g_io_channel_read_line_backend (xio_channel_t  *channel,
                                xsize_t       *length,
                                xsize_t       *terminator_pos,
                                xerror_t     **error)
{
  GIOStatus status;
  xsize_t checked_to, line_term_len, line_length, got_term_len;
  xboolean_t first_time = TRUE;

  if (!channel->use_buffer)
    {
      /* Can't do a raw read in read_line */
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                           _("Can’t do a raw read in g_io_channel_read_line_string"));
      return G_IO_STATUS_ERROR;
    }

  status = G_IO_STATUS_NORMAL;

  if (channel->line_term)
    line_term_len = channel->line_term_len;
  else
    line_term_len = 3;
    /* This value used for setting checked_to, it's the longest of the four
     * we autodetect for.
     */

  checked_to = 0;

  while (TRUE)
    {
      xchar_t *nextchar, *lastchar;
      xstring_t *use_buf;

      if (!first_time || (BUF_LEN (USE_BUF (channel)) == 0))
        {
read_again:
          status = g_io_channel_fill_buffer (channel, error);
          switch (status)
            {
              case G_IO_STATUS_NORMAL:
                if (BUF_LEN (USE_BUF (channel)) == 0)
                  /* Can happen when using conversion and only read
                   * part of a character
                   */
                  {
                    first_time = FALSE;
                    continue;
                  }
                break;
              case G_IO_STATUS_EOF:
                if (BUF_LEN (USE_BUF (channel)) == 0)
                  {
                    if (length)
                      *length = 0;

                    if (channel->encoding && channel->read_buf->len != 0)
                      {
                        g_set_error_literal (error, G_CONVERT_ERROR,
                                             G_CONVERT_ERROR_PARTIAL_INPUT,
                                             _("Leftover unconverted data in "
                                               "read buffer"));
                        return G_IO_STATUS_ERROR;
                      }
                    else
                      return G_IO_STATUS_EOF;
                  }
                break;
              default:
                if (length)
                  *length = 0;
                return status;
            }
        }

      xassert (BUF_LEN (USE_BUF (channel)) != 0);

      use_buf = USE_BUF (channel); /* The buffer has been created by this point */

      first_time = FALSE;

      lastchar = use_buf->str + use_buf->len;

      for (nextchar = use_buf->str + checked_to; nextchar < lastchar;
           channel->encoding ? nextchar = xutf8_next_char (nextchar) : nextchar++)
        {
          if (channel->line_term)
            {
              if (memcmp (channel->line_term, nextchar, line_term_len) == 0)
                {
                  line_length = nextchar - use_buf->str;
                  got_term_len = line_term_len;
                  goto done;
                }
            }
          else /* auto detect */
            {
              switch (*nextchar)
                {
                  case '\n': /* unix */
                    line_length = nextchar - use_buf->str;
                    got_term_len = 1;
                    goto done;
                  case '\r': /* Warning: do not use with sockets */
                    line_length = nextchar - use_buf->str;
                    if ((nextchar == lastchar - 1) && (status != G_IO_STATUS_EOF)
                       && (lastchar == use_buf->str + use_buf->len))
                      goto read_again; /* Try to read more data */
                    if ((nextchar < lastchar - 1) && (*(nextchar + 1) == '\n')) /* dos */
                      got_term_len = 2;
                    else /* mac */
                      got_term_len = 1;
                    goto done;
                  case '\xe2': /* Unicode paragraph separator */
                    if (strncmp ("\xe2\x80\xa9", nextchar, 3) == 0)
                      {
                        line_length = nextchar - use_buf->str;
                        got_term_len = 3;
                        goto done;
                      }
                    break;
                  case '\0': /* Embedded null in input */
                    line_length = nextchar - use_buf->str;
                    got_term_len = 1;
                    goto done;
                  default: /* no match */
                    break;
                }
            }
        }

      /* If encoding != NULL, valid UTF-8, didn't overshoot */
      xassert (nextchar == lastchar);

      /* Check for EOF */

      if (status == G_IO_STATUS_EOF)
        {
          if (channel->encoding && channel->read_buf->len > 0)
            {
              g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                                   _("Channel terminates in a partial character"));
              return G_IO_STATUS_ERROR;
            }
          line_length = use_buf->len;
          got_term_len = 0;
          break;
        }

      if (use_buf->len > line_term_len - 1)
	checked_to = use_buf->len - (line_term_len - 1);
      else
	checked_to = 0;
    }

done:

  if (terminator_pos)
    *terminator_pos = line_length;

  if (length)
    *length = line_length + got_term_len;

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_read_to_end:
 * @channel: a #xio_channel_t
 * @str_return:  (out) (array length=length) (element-type xuint8_t): Location to
 *              store a pointer to a string holding the remaining data in the
 *              #xio_channel_t. This data should be freed with g_free() when no
 *              longer needed. This data is terminated by an extra nul
 *              character, but there may be other nuls in the intervening data.
 * @length: (out): location to store length of the data
 * @error: location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads all the remaining data from the file.
 *
 * Returns: %G_IO_STATUS_NORMAL on success.
 *     This function never returns %G_IO_STATUS_EOF.
 **/
GIOStatus
g_io_channel_read_to_end (xio_channel_t  *channel,
                          xchar_t      **str_return,
                          xsize_t	      *length,
                          xerror_t     **error)
{
  GIOStatus status;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
    G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  if (str_return)
    *str_return = NULL;
  if (length)
    *length = 0;

  if (!channel->use_buffer)
    {
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                           _("Can’t do a raw read in g_io_channel_read_to_end"));
      return G_IO_STATUS_ERROR;
    }

  do
    status = g_io_channel_fill_buffer (channel, error);
  while (status == G_IO_STATUS_NORMAL);

  if (status != G_IO_STATUS_EOF)
    return status;

  if (channel->encoding && channel->read_buf->len > 0)
    {
      g_set_error_literal (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT,
                           _("Channel terminates in a partial character"));
      return G_IO_STATUS_ERROR;
    }

  if (USE_BUF (channel) == NULL)
    {
      /* length is already set to zero */
      if (str_return)
        *str_return = xstrdup ("");
    }
  else
    {
      if (length)
        *length = USE_BUF (channel)->len;

      if (str_return)
        *str_return = xstring_free (USE_BUF (channel), FALSE);
      else
        xstring_free (USE_BUF (channel), TRUE);

      if (channel->encoding)
	channel->encoded_read_buf = NULL;
      else
	channel->read_buf = NULL;
    }

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_read_chars:
 * @channel: a #xio_channel_t
 * @buf: (out caller-allocates) (array length=count) (element-type xuint8_t):
 *     a buffer to read data into
 * @count: (in): the size of the buffer. Note that the buffer may not be
 *     completely filled even if there is data in the buffer if the
 *     remaining data is not a complete character.
 * @bytes_read: (out) (optional): The number of bytes read. This may be
 *     zero even on success if count < 6 and the channel's encoding
 *     is non-%NULL. This indicates that the next UTF-8 character is
 *     too wide for the buffer.
 * @error: a location to return an error of type #GConvertError
 *     or #GIOChannelError.
 *
 * Replacement for g_io_channel_read() with the new API.
 *
 * Returns: the status of the operation.
 */
GIOStatus
g_io_channel_read_chars (xio_channel_t  *channel,
                         xchar_t       *buf,
                         xsize_t        count,
                         xsize_t       *bytes_read,
                         xerror_t     **error)
{
  GIOStatus status;
  xsize_t got_bytes;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL), G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  if (count == 0)
    {
      if (bytes_read)
        *bytes_read = 0;
      return G_IO_STATUS_NORMAL;
    }
  xreturn_val_if_fail (buf != NULL, G_IO_STATUS_ERROR);

  if (!channel->use_buffer)
    {
      xsize_t tmp_bytes;

      xassert (!channel->read_buf || channel->read_buf->len == 0);

      status = channel->funcs->io_read (channel, buf, count, &tmp_bytes, error);

      if (bytes_read)
        *bytes_read = tmp_bytes;

      return status;
    }

  status = G_IO_STATUS_NORMAL;

  while (BUF_LEN (USE_BUF (channel)) < count && status == G_IO_STATUS_NORMAL)
    status = g_io_channel_fill_buffer (channel, error);

  /* Only return an error if we have no data */

  if (BUF_LEN (USE_BUF (channel)) == 0)
    {
      xassert (status != G_IO_STATUS_NORMAL);

      if (status == G_IO_STATUS_EOF && channel->encoding
          && BUF_LEN (channel->read_buf) > 0)
        {
          g_set_error_literal (error, G_CONVERT_ERROR,
                               G_CONVERT_ERROR_PARTIAL_INPUT,
                               _("Leftover unconverted data in read buffer"));
          status = G_IO_STATUS_ERROR;
        }

      if (bytes_read)
        *bytes_read = 0;

      return status;
    }

  if (status == G_IO_STATUS_ERROR)
    g_clear_error (error);

  got_bytes = MIN (count, BUF_LEN (USE_BUF (channel)));

  xassert (got_bytes > 0);

  if (channel->encoding)
    /* Don't validate for NULL encoding, binary safe */
    {
      xchar_t *nextchar, *prevchar;

      xassert (USE_BUF (channel) == channel->encoded_read_buf);

      nextchar = channel->encoded_read_buf->str;

      do
        {
          prevchar = nextchar;
          nextchar = xutf8_next_char (nextchar);
          xassert (nextchar != prevchar); /* Possible for *prevchar of -1 or -2 */
        }
      while (nextchar < channel->encoded_read_buf->str + got_bytes);

      if (nextchar > channel->encoded_read_buf->str + got_bytes)
        got_bytes = prevchar - channel->encoded_read_buf->str;

      xassert (got_bytes > 0 || count < 6);
    }

  memcpy (buf, USE_BUF (channel)->str, got_bytes);
  xstring_erase (USE_BUF (channel), 0, got_bytes);

  if (bytes_read)
    *bytes_read = got_bytes;

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_read_unichar:
 * @channel: a #xio_channel_t
 * @thechar: (out): a location to return a character
 * @error: a location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Reads a Unicode character from @channel.
 * This function cannot be called on a channel with %NULL encoding.
 *
 * Returns: a #GIOStatus
 **/
GIOStatus
g_io_channel_read_unichar (xio_channel_t  *channel,
			   xunichar_t    *thechar,
			   xerror_t     **error)
{
  GIOStatus status = G_IO_STATUS_NORMAL;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->encoding != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_readable, G_IO_STATUS_ERROR);

  while (BUF_LEN (channel->encoded_read_buf) == 0 && status == G_IO_STATUS_NORMAL)
    status = g_io_channel_fill_buffer (channel, error);

  /* Only return an error if we have no data */

  if (BUF_LEN (USE_BUF (channel)) == 0)
    {
      xassert (status != G_IO_STATUS_NORMAL);

      if (status == G_IO_STATUS_EOF && BUF_LEN (channel->read_buf) > 0)
        {
          g_set_error_literal (error, G_CONVERT_ERROR,
                               G_CONVERT_ERROR_PARTIAL_INPUT,
                               _("Leftover unconverted data in read buffer"));
          status = G_IO_STATUS_ERROR;
        }

      if (thechar)
        *thechar = (xunichar_t) -1;

      return status;
    }

  if (status == G_IO_STATUS_ERROR)
    g_clear_error (error);

  if (thechar)
    *thechar = xutf8_get_char (channel->encoded_read_buf->str);

  xstring_erase (channel->encoded_read_buf, 0,
                  xutf8_next_char (channel->encoded_read_buf->str)
                  - channel->encoded_read_buf->str);

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_write_chars:
 * @channel: a #xio_channel_t
 * @buf: (array) (element-type xuint8_t): a buffer to write data from
 * @count: the size of the buffer. If -1, the buffer
 *         is taken to be a nul-terminated string.
 * @bytes_written: (out): The number of bytes written. This can be nonzero
 *                 even if the return value is not %G_IO_STATUS_NORMAL.
 *                 If the return value is %G_IO_STATUS_NORMAL and the
 *                 channel is blocking, this will always be equal
 *                 to @count if @count >= 0.
 * @error: a location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Replacement for g_io_channel_write() with the new API.
 *
 * On seekable channels with encodings other than %NULL or UTF-8, generic
 * mixing of reading and writing is not allowed. A call to g_io_channel_write_chars ()
 * may only be made on a channel from which data has been read in the
 * cases described in the documentation for g_io_channel_set_encoding ().
 *
 * Returns: the status of the operation.
 **/
GIOStatus
g_io_channel_write_chars (xio_channel_t   *channel,
                          const xchar_t  *buf,
                          xssize_t        count,
			  xsize_t        *bytes_written,
                          xerror_t      **error)
{
  xsize_t count_unsigned;
  GIOStatus status;
  xssize_t wrote_bytes = 0;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_writeable, G_IO_STATUS_ERROR);

  if ((count < 0) && buf)
    count = strlen (buf);
  count_unsigned = count;

  if (count_unsigned == 0)
    {
      if (bytes_written)
        *bytes_written = 0;
      return G_IO_STATUS_NORMAL;
    }

  xreturn_val_if_fail (buf != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail (count_unsigned > 0, G_IO_STATUS_ERROR);

  /* Raw write case */

  if (!channel->use_buffer)
    {
      xsize_t tmp_bytes;

      xassert (!channel->write_buf || channel->write_buf->len == 0);
      xassert (channel->partial_write_buf[0] == '\0');

      status = channel->funcs->io_write (channel, buf, count_unsigned,
                                         &tmp_bytes, error);

      if (bytes_written)
	*bytes_written = tmp_bytes;

      return status;
    }

  /* General case */

  if (channel->is_seekable && (( BUF_LEN (channel->read_buf) > 0)
    || (BUF_LEN (channel->encoded_read_buf) > 0)))
    {
      if (channel->do_encode && BUF_LEN (channel->encoded_read_buf) > 0)
        {
          g_warning ("Mixed reading and writing not allowed on encoded files");
          return G_IO_STATUS_ERROR;
        }
      status = g_io_channel_seek_position (channel, 0, G_SEEK_CUR, error);
      if (status != G_IO_STATUS_NORMAL)
        {
          if (bytes_written)
            *bytes_written = 0;
          return status;
        }
    }

  if (!channel->write_buf)
    channel->write_buf = xstring_sized_new (channel->buf_size);

  while (wrote_bytes < count)
    {
      xsize_t space_in_buf;

      /* If the buffer is full, try a write immediately. In
       * the nonblocking case, this prevents the user from
       * writing just a little bit to the buffer every time
       * and never receiving an EAGAIN.
       */

      if (channel->write_buf->len >= channel->buf_size - MAX_CHAR_SIZE)
        {
          xsize_t did_write = 0, this_time;

          do
            {
              status = channel->funcs->io_write (channel, channel->write_buf->str
                                                 + did_write, channel->write_buf->len
                                                 - did_write, &this_time, error);
              did_write += this_time;
            }
          while (status == G_IO_STATUS_NORMAL &&
                 did_write < MIN (channel->write_buf->len, MAX_CHAR_SIZE));

          xstring_erase (channel->write_buf, 0, did_write);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (status == G_IO_STATUS_AGAIN && wrote_bytes > 0)
                status = G_IO_STATUS_NORMAL;
              if (bytes_written)
                *bytes_written = wrote_bytes;
              return status;
            }
        }

      space_in_buf = MAX (channel->buf_size, channel->write_buf->allocated_len - 1)
                     - channel->write_buf->len; /* 1 for NULL */

      /* This is only true because g_io_channel_set_buffer_size ()
       * ensures that channel->buf_size >= MAX_CHAR_SIZE.
       */
      xassert (space_in_buf >= MAX_CHAR_SIZE);

      if (!channel->encoding)
        {
          xssize_t write_this = MIN (space_in_buf, count_unsigned - wrote_bytes);

          xstring_append_len (channel->write_buf, buf, write_this);
          buf += write_this;
          wrote_bytes += write_this;
        }
      else
        {
          const xchar_t *from_buf;
          xsize_t from_buf_len, from_buf_old_len, left_len;
          xsize_t err;
          xint_t errnum;

          if (channel->partial_write_buf[0] != '\0')
            {
              xassert (wrote_bytes == 0);

              from_buf = channel->partial_write_buf;
              from_buf_old_len = strlen (channel->partial_write_buf);
              xassert (from_buf_old_len > 0);
              from_buf_len = MIN (6, from_buf_old_len + count_unsigned);

              memcpy (channel->partial_write_buf + from_buf_old_len, buf,
                      from_buf_len - from_buf_old_len);
            }
          else
            {
              from_buf = buf;
              from_buf_len = count_unsigned - wrote_bytes;
              from_buf_old_len = 0;
            }

reconvert:

          if (!channel->do_encode) /* UTF-8 encoding */
            {
              const xchar_t *badchar;
              xsize_t try_len = MIN (from_buf_len, space_in_buf);

              /* UTF-8, just validate, emulate g_iconv */

              if (!xutf8_validate_len (from_buf, try_len, &badchar))
                {
                  xunichar_t try_char;
                  xsize_t incomplete_len = from_buf + try_len - badchar;

                  left_len = from_buf + from_buf_len - badchar;

                  try_char = xutf8_get_char_validated (badchar, incomplete_len);

                  switch (try_char)
                    {
                      case -2:
                        xassert (incomplete_len < 6);
                        if (try_len == from_buf_len)
                          {
                            errnum = EINVAL;
                            err = (xsize_t) -1;
                          }
                        else
                          {
                            errnum = 0;
                            err = (xsize_t) 0;
                          }
                        break;
                      case -1:
                        g_warning ("Invalid UTF-8 passed to g_io_channel_write_chars().");
                        /* FIXME bail here? */
                        errnum = EILSEQ;
                        err = (xsize_t) -1;
                        break;
                      default:
                        g_assert_not_reached ();
                        err = (xsize_t) -1;
                        errnum = 0; /* Don't confuse the compiler */
                    }
                }
              else
                {
                  err = (xsize_t) 0;
                  errnum = 0;
                  left_len = from_buf_len - try_len;
                }

              xstring_append_len (channel->write_buf, from_buf,
                                   from_buf_len - left_len);
              from_buf += from_buf_len - left_len;
            }
          else
            {
               xchar_t *outbuf;

               left_len = from_buf_len;
               xstring_set_size (channel->write_buf, channel->write_buf->len
                                  + space_in_buf);
               outbuf = channel->write_buf->str + channel->write_buf->len
                        - space_in_buf;
               err = g_iconv (channel->write_cd, (xchar_t **) &from_buf, &left_len,
                              &outbuf, &space_in_buf);
               errnum = errno;
               xstring_truncate (channel->write_buf, channel->write_buf->len
                                  - space_in_buf);
            }

          if (err == (xsize_t) -1)
            {
              switch (errnum)
        	{
                  case EINVAL:
                    xassert (left_len < 6);

                    if (from_buf_old_len == 0)
                      {
                        /* Not from partial_write_buf */

                        memcpy (channel->partial_write_buf, from_buf, left_len);
                        channel->partial_write_buf[left_len] = '\0';
                        if (bytes_written)
                          *bytes_written = count_unsigned;
                        return G_IO_STATUS_NORMAL;
                      }

                    /* Working in partial_write_buf */

                    if (left_len == from_buf_len)
                      {
                        /* Didn't convert anything, must still have
                         * less than a full character
                         */

                        xassert (count_unsigned == from_buf_len - from_buf_old_len);

                        channel->partial_write_buf[from_buf_len] = '\0';

                        if (bytes_written)
                          *bytes_written = count_unsigned;

                        return G_IO_STATUS_NORMAL;
                      }

                    xassert (from_buf_len - left_len >= from_buf_old_len);

                    /* We converted all the old data. This is fine */

                    break;
                  case E2BIG:
                    if (from_buf_len == left_len)
                      {
                        /* Nothing was written, add enough space for
                         * at least one character.
                         */
                        space_in_buf += MAX_CHAR_SIZE;
                        goto reconvert;
                      }
                    break;
                  case EILSEQ:
                    g_set_error_literal (error, G_CONVERT_ERROR,
                      G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                      _("Invalid byte sequence in conversion input"));
                    if (from_buf_old_len > 0 && from_buf_len == left_len)
                      g_warning ("Illegal sequence due to partial character "
                                 "at the end of a previous write.");
                    else
                      wrote_bytes += from_buf_len - left_len - from_buf_old_len;
                    if (bytes_written)
                      *bytes_written = wrote_bytes;
                    channel->partial_write_buf[0] = '\0';
                    return G_IO_STATUS_ERROR;
                  default:
                    g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_FAILED,
                      _("Error during conversion: %s"), xstrerror (errnum));
                    if (from_buf_len >= left_len + from_buf_old_len)
                      wrote_bytes += from_buf_len - left_len - from_buf_old_len;
                    if (bytes_written)
                      *bytes_written = wrote_bytes;
                    channel->partial_write_buf[0] = '\0';
                    return G_IO_STATUS_ERROR;
                }
            }

          xassert (from_buf_len - left_len >= from_buf_old_len);

          wrote_bytes += from_buf_len - left_len - from_buf_old_len;

          if (from_buf_old_len > 0)
            {
              /* We were working in partial_write_buf */

              buf += from_buf_len - left_len - from_buf_old_len;
              channel->partial_write_buf[0] = '\0';
            }
          else
            buf = from_buf;
        }
    }

  if (bytes_written)
    *bytes_written = count_unsigned;

  return G_IO_STATUS_NORMAL;
}

/**
 * g_io_channel_write_unichar:
 * @channel: a #xio_channel_t
 * @thechar: a character
 * @error: location to return an error of type #GConvertError
 *         or #GIOChannelError
 *
 * Writes a Unicode character to @channel.
 * This function cannot be called on a channel with %NULL encoding.
 *
 * Returns: a #GIOStatus
 **/
GIOStatus
g_io_channel_write_unichar (xio_channel_t  *channel,
			    xunichar_t     thechar,
			    xerror_t     **error)
{
  GIOStatus status;
  xchar_t static_buf[6];
  xsize_t char_len, wrote_len;

  xreturn_val_if_fail (channel != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->encoding != NULL, G_IO_STATUS_ERROR);
  xreturn_val_if_fail ((error == NULL) || (*error == NULL),
			G_IO_STATUS_ERROR);
  xreturn_val_if_fail (channel->is_writeable, G_IO_STATUS_ERROR);

  char_len = xunichar_to_utf8 (thechar, static_buf);

  if (channel->partial_write_buf[0] != '\0')
    {
      g_warning ("Partial character written before writing unichar.");
      channel->partial_write_buf[0] = '\0';
    }

  status = g_io_channel_write_chars (channel, static_buf,
                                     char_len, &wrote_len, error);

  /* We validate UTF-8, so we can't get a partial write */

  xassert (wrote_len == char_len || status != G_IO_STATUS_NORMAL);

  return status;
}

/**
 * G_IO_CHANNEL_ERROR:
 *
 * Error domain for #xio_channel_t operations. Errors in this domain will
 * be from the #GIOChannelError enumeration. See #xerror_t for
 * information on error domains.
 **/
/**
 * GIOChannelError:
 * @G_IO_CHANNEL_ERROR_FBIG: File too large.
 * @G_IO_CHANNEL_ERROR_INVAL: Invalid argument.
 * @G_IO_CHANNEL_ERROR_IO: IO error.
 * @G_IO_CHANNEL_ERROR_ISDIR: File is a directory.
 * @G_IO_CHANNEL_ERROR_NOSPC: No space left on device.
 * @G_IO_CHANNEL_ERROR_NXIO: No such device or address.
 * @G_IO_CHANNEL_ERROR_OVERFLOW: Value too large for defined datatype.
 * @G_IO_CHANNEL_ERROR_PIPE: Broken pipe.
 * @G_IO_CHANNEL_ERROR_FAILED: Some other error.
 *
 * Error codes returned by #xio_channel_t operations.
 **/

G_DEFINE_QUARK (g-io-channel-error-quark, g_io_channel_error)
