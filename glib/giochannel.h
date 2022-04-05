/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_IOCHANNEL_H__
#define __G_IOCHANNEL_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gconvert.h>
#include <glib/gmain.h>
#include <glib/gstring.h>

G_BEGIN_DECLS

/* GIOChannel
 */

typedef struct _GIOChannel	GIOChannel;
typedef struct _GIOFuncs        GIOFuncs;

typedef enum
{
  G_IO_ERROR_NONE,
  G_IO_ERROR_AGAIN,
  G_IO_ERROR_INVAL,
  G_IO_ERROR_UNKNOWN
} GIOError;

#define G_IO_CHANNEL_ERROR g_io_channel_error_quark()

typedef enum
{
  /* Derived from errno */
  G_IO_CHANNEL_ERROR_FBIG,
  G_IO_CHANNEL_ERROR_INVAL,
  G_IO_CHANNEL_ERROR_IO,
  G_IO_CHANNEL_ERROR_ISDIR,
  G_IO_CHANNEL_ERROR_NOSPC,
  G_IO_CHANNEL_ERROR_NXIO,
  G_IO_CHANNEL_ERROR_OVERFLOW,
  G_IO_CHANNEL_ERROR_PIPE,
  /* Other */
  G_IO_CHANNEL_ERROR_FAILED
} GIOChannelError;

typedef enum
{
  G_IO_STATUS_ERROR,
  G_IO_STATUS_NORMAL,
  G_IO_STATUS_EOF,
  G_IO_STATUS_AGAIN
} GIOStatus;

typedef enum
{
  G_SEEK_CUR,
  G_SEEK_SET,
  G_SEEK_END
} GSeekType;

typedef enum
{
  G_IO_FLAG_APPEND = 1 << 0,
  G_IO_FLAG_NONBLOCK = 1 << 1,
  G_IO_FLAX_IS_READABLE = 1 << 2,	/* Read only flag */
  G_IO_FLAX_IS_WRITABLE = 1 << 3,	/* Read only flag */
  G_IO_FLAX_IS_WRITEABLE = 1 << 3,      /* Misspelling in 2.29.10 and earlier */
  G_IO_FLAX_IS_SEEKABLE = 1 << 4,	/* Read only flag */
  G_IO_FLAG_MASK = (1 << 5) - 1,
  G_IO_FLAG_GET_MASK = G_IO_FLAG_MASK,
  G_IO_FLAG_SET_MASK = G_IO_FLAG_APPEND | G_IO_FLAG_NONBLOCK
} GIOFlags;

struct _GIOChannel
{
  /*< private >*/
  xint_t ref_count;
  GIOFuncs *funcs;

  xchar_t *encoding;
  GIConv read_cd;
  GIConv write_cd;
  xchar_t *line_term;		/* String which indicates the end of a line of text */
  xuint_t line_term_len;		/* So we can have null in the line term */

  xsize_t buf_size;
  GString *read_buf;		/* Raw data from the channel */
  GString *encoded_read_buf;    /* Channel data converted to UTF-8 */
  GString *write_buf;		/* Data ready to be written to the file */
  xchar_t partial_write_buf[6];	/* UTF-8 partial characters, null terminated */

  /* Group the flags together, immediately after partial_write_buf, to save memory */

  xuint_t use_buffer     : 1;	/* The encoding uses the buffers */
  xuint_t do_encode      : 1;	/* The encoding uses the GIConv coverters */
  xuint_t close_on_unref : 1;	/* Close the channel on final unref */
  xuint_t is_readable    : 1;	/* Cached GIOFlag */
  xuint_t is_writeable   : 1;	/* ditto */
  xuint_t is_seekable    : 1;	/* ditto */

  xpointer_t reserved1;
  xpointer_t reserved2;
};

typedef xboolean_t (*GIOFunc) (GIOChannel   *source,
			     GIOCondition  condition,
			     xpointer_t      data);
struct _GIOFuncs
{
  GIOStatus (*io_read)           (GIOChannel   *channel,
			          xchar_t        *buf,
				  xsize_t         count,
				  xsize_t        *bytes_read,
				  xerror_t      **err);
  GIOStatus (*io_write)          (GIOChannel   *channel,
				  const xchar_t  *buf,
				  xsize_t         count,
				  xsize_t        *bytes_written,
				  xerror_t      **err);
  GIOStatus (*io_seek)           (GIOChannel   *channel,
				  gint64        offset,
				  GSeekType     type,
				  xerror_t      **err);
  GIOStatus  (*io_close)         (GIOChannel   *channel,
				  xerror_t      **err);
  GSource*   (*io_create_watch)  (GIOChannel   *channel,
				  GIOCondition  condition);
  void       (*io_free)          (GIOChannel   *channel);
  GIOStatus  (*io_set_flags)     (GIOChannel   *channel,
                                  GIOFlags      flags,
				  xerror_t      **err);
  GIOFlags   (*io_get_flags)     (GIOChannel   *channel);
};

XPL_AVAILABLE_IN_ALL
void        g_io_channel_init   (GIOChannel    *channel);
XPL_AVAILABLE_IN_ALL
GIOChannel *g_io_channel_ref    (GIOChannel    *channel);
XPL_AVAILABLE_IN_ALL
void        g_io_channel_unref  (GIOChannel    *channel);

XPL_DEPRECATED_FOR(g_io_channel_read_chars)
GIOError    g_io_channel_read   (GIOChannel    *channel,
                                 xchar_t         *buf,
                                 xsize_t          count,
                                 xsize_t         *bytes_read);

XPL_DEPRECATED_FOR(g_io_channel_write_chars)
GIOError  g_io_channel_write    (GIOChannel    *channel,
                                 const xchar_t   *buf,
                                 xsize_t          count,
                                 xsize_t         *bytes_written);

XPL_DEPRECATED_FOR(g_io_channel_seek_position)
GIOError  g_io_channel_seek     (GIOChannel    *channel,
                                 gint64         offset,
                                 GSeekType      type);

XPL_DEPRECATED_FOR(g_io_channel_shutdown)
void      g_io_channel_close    (GIOChannel    *channel);

XPL_AVAILABLE_IN_ALL
GIOStatus g_io_channel_shutdown (GIOChannel      *channel,
				 xboolean_t         flush,
				 xerror_t         **err);
XPL_AVAILABLE_IN_ALL
xuint_t     g_io_add_watch_full   (GIOChannel      *channel,
				 xint_t             priority,
				 GIOCondition     condition,
				 GIOFunc          func,
				 xpointer_t         user_data,
				 GDestroyNotify   notify);
XPL_AVAILABLE_IN_ALL
GSource * g_io_create_watch     (GIOChannel      *channel,
				 GIOCondition     condition);
XPL_AVAILABLE_IN_ALL
xuint_t     g_io_add_watch        (GIOChannel      *channel,
				 GIOCondition     condition,
				 GIOFunc          func,
				 xpointer_t         user_data);

/* character encoding conversion involved functions.
 */

XPL_AVAILABLE_IN_ALL
void                  g_io_channel_set_buffer_size      (GIOChannel   *channel,
							 xsize_t         size);
XPL_AVAILABLE_IN_ALL
xsize_t                 g_io_channel_get_buffer_size      (GIOChannel   *channel);
XPL_AVAILABLE_IN_ALL
GIOCondition          g_io_channel_get_buffer_condition (GIOChannel   *channel);
XPL_AVAILABLE_IN_ALL
GIOStatus             g_io_channel_set_flags            (GIOChannel   *channel,
							 GIOFlags      flags,
							 xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOFlags              g_io_channel_get_flags            (GIOChannel   *channel);
XPL_AVAILABLE_IN_ALL
void                  g_io_channel_set_line_term        (GIOChannel   *channel,
							 const xchar_t  *line_term,
							 xint_t          length);
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_io_channel_get_line_term        (GIOChannel   *channel,
							 xint_t         *length);
XPL_AVAILABLE_IN_ALL
void		      g_io_channel_set_buffered		(GIOChannel   *channel,
							 xboolean_t      buffered);
XPL_AVAILABLE_IN_ALL
xboolean_t	      g_io_channel_get_buffered		(GIOChannel   *channel);
XPL_AVAILABLE_IN_ALL
GIOStatus             g_io_channel_set_encoding         (GIOChannel   *channel,
							 const xchar_t  *encoding,
							 xerror_t      **error);
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_io_channel_get_encoding         (GIOChannel   *channel);
XPL_AVAILABLE_IN_ALL
void                  g_io_channel_set_close_on_unref	(GIOChannel   *channel,
							 xboolean_t      do_close);
XPL_AVAILABLE_IN_ALL
xboolean_t              g_io_channel_get_close_on_unref	(GIOChannel   *channel);


XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_flush            (GIOChannel   *channel,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_read_line        (GIOChannel   *channel,
					   xchar_t       **str_return,
					   xsize_t        *length,
					   xsize_t        *terminator_pos,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_read_line_string (GIOChannel   *channel,
					   GString      *buffer,
					   xsize_t        *terminator_pos,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_read_to_end      (GIOChannel   *channel,
					   xchar_t       **str_return,
					   xsize_t        *length,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_read_chars       (GIOChannel   *channel,
					   xchar_t        *buf,
					   xsize_t         count,
					   xsize_t        *bytes_read,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_read_unichar     (GIOChannel   *channel,
					   gunichar     *thechar,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_write_chars      (GIOChannel   *channel,
					   const xchar_t  *buf,
					   gssize        count,
					   xsize_t        *bytes_written,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_write_unichar    (GIOChannel   *channel,
					   gunichar      thechar,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOStatus   g_io_channel_seek_position    (GIOChannel   *channel,
					   gint64        offset,
					   GSeekType     type,
					   xerror_t      **error);
XPL_AVAILABLE_IN_ALL
GIOChannel* g_io_channel_new_file         (const xchar_t  *filename,
					   const xchar_t  *mode,
					   xerror_t      **error);

/* Error handling */

XPL_AVAILABLE_IN_ALL
GQuark          g_io_channel_error_quark      (void);
XPL_AVAILABLE_IN_ALL
GIOChannelError g_io_channel_error_from_errno (xint_t en);

/* On Unix, IO channels created with this function for any file
 * descriptor or socket.
 *
 * On Win32, this can be used either for files opened with the MSVCRT
 * (the Microsoft run-time C library) _open() or _pipe, including file
 * descriptors 0, 1 and 2 (corresponding to stdin, stdout and stderr),
 * or for Winsock SOCKETs. If the parameter is a legal file
 * descriptor, it is assumed to be such, otherwise it should be a
 * SOCKET. This relies on SOCKETs and file descriptors not
 * overlapping. If you want to be certain, call either
 * g_io_channel_win32_new_fd() or g_io_channel_win32_new_socket()
 * instead as appropriate.
 *
 * The term file descriptor as used in the context of Win32 refers to
 * the emulated Unix-like file descriptors MSVCRT provides. The native
 * corresponding concept is file HANDLE. There isn't as of yet a way to
 * get GIOChannels for Win32 file HANDLEs.
 */
XPL_AVAILABLE_IN_ALL
GIOChannel* g_io_channel_unix_new    (int         fd);
XPL_AVAILABLE_IN_ALL
xint_t        g_io_channel_unix_get_fd (GIOChannel *channel);


/* Hook for GClosure / GSource integration. Don't touch */
XPL_VAR GSourceFuncs g_io_watch_funcs;

#ifdef G_OS_WIN32

/* You can use this "pseudo file descriptor" in a GPollFD to add
 * polling for Windows messages. GTK applications should not do that.
 */

#define G_WIN32_MSG_HANDLE 19981206

/* Use this to get a GPollFD from a GIOChannel, so that you can call
 * g_io_channel_win32_poll(). After calling this you should only use
 * g_io_channel_read() to read from the GIOChannel, i.e. never read()
 * from the underlying file descriptor. For SOCKETs, it is possible to call
 * recv().
 */
XPL_AVAILABLE_IN_ALL
void        g_io_channel_win32_make_pollfd (GIOChannel   *channel,
					    GIOCondition  condition,
					    GPollFD      *fd);

/* This can be used to wait until at least one of the channels is readable.
 * On Unix you would do a select() on the file descriptors of the channels.
 */
XPL_AVAILABLE_IN_ALL
xint_t        g_io_channel_win32_poll   (GPollFD    *fds,
				       xint_t        n_fds,
				       xint_t        timeout_);

/* Create an IO channel for Windows messages for window handle hwnd. */
#if XPL_SIZEOF_VOID_P == 8
/* We use xsize_t here so that it is still an integer type and not a
 * pointer, like the xuint_t in the traditional prototype. We can't use
 * intptr_t as that is not portable enough.
 */
XPL_AVAILABLE_IN_ALL
GIOChannel *g_io_channel_win32_new_messages (xsize_t hwnd);
#else
XPL_AVAILABLE_IN_ALL
GIOChannel *g_io_channel_win32_new_messages (xuint_t hwnd);
#endif

/* Create an IO channel for C runtime (emulated Unix-like) file
 * descriptors. After calling g_io_add_watch() on a IO channel
 * returned by this function, you shouldn't call read() on the file
 * descriptor. This is because adding polling for a file descriptor is
 * implemented on Win32 by starting a thread that sits blocked in a
 * read() from the file descriptor most of the time. All reads from
 * the file descriptor should be done by this internal GLib
 * thread. Your code should call only g_io_channel_read_chars().
 */
XPL_AVAILABLE_IN_ALL
GIOChannel* g_io_channel_win32_new_fd (xint_t         fd);

/* Get the C runtime file descriptor of a channel. */
XPL_AVAILABLE_IN_ALL
xint_t        g_io_channel_win32_get_fd (GIOChannel *channel);

/* Create an IO channel for a winsock socket. The parameter should be
 * a SOCKET. Contrary to IO channels for file descriptors (on *Win32),
 * you can use normal recv() or recvfrom() on sockets even if GLib
 * is polling them.
 */
XPL_AVAILABLE_IN_ALL
GIOChannel *g_io_channel_win32_new_socket (xint_t socket);

XPL_DEPRECATED_FOR(g_io_channel_win32_new_socket)
GIOChannel *g_io_channel_win32_new_stream_socket (xint_t socket);

XPL_AVAILABLE_IN_ALL
void        g_io_channel_win32_set_debug (GIOChannel *channel,
                                          xboolean_t    flag);

#endif

G_END_DECLS

#endif /* __G_IOCHANNEL_H__ */