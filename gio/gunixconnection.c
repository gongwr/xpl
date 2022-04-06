/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2009 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gunixconnection.h"
#include "gnetworking.h"
#include "gsocket.h"
#include "gsocketcontrolmessage.h"
#include "gunixcredentialsmessage.h"
#include "gunixfdmessage.h"
#include "glibintl.h"

#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/**
 * SECTION:gunixconnection
 * @title: GUnixConnection
 * @short_description: A UNIX domain xsocket_connection_t
 * @include: gio/gunixconnection.h
 * @see_also: #xsocket_connection_t.
 *
 * This is the subclass of #xsocket_connection_t that is created
 * for UNIX domain sockets.
 *
 * It contains functions to do some of the UNIX socket specific
 * functionality like passing file descriptors.
 *
 * Since GLib 2.72, #GUnixConnection is available on all platforms. It requires
 * underlying system support (such as Windows 10 with `AF_UNIX`) at run time.
 *
 * Before GLib 2.72, `<gio/gunixconnection.h>` belonged to the UNIX-specific GIO
 * interfaces, thus you had to use the `gio-unix-2.0.pc` pkg-config file when
 * using it. This is no longer necessary since GLib 2.72.
 *
 * Since: 2.22
 */

/**
 * GUnixConnection:
 *
 * #GUnixConnection is an opaque data structure and can only be accessed
 * using the following functions.
 **/

G_DEFINE_TYPE_WITH_CODE (GUnixConnection, g_unix_connection,
			 XTYPE_SOCKET_CONNECTION,
  xsocket_connection_factory_register_type (g_define_type_id,
					     XSOCKET_FAMILY_UNIX,
					     XSOCKET_TYPE_STREAM,
					     XSOCKET_PROTOCOL_DEFAULT);
			 );

/**
 * g_unix_connection_send_fd:
 * @connection: a #GUnixConnection
 * @fd: a file descriptor
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @error: (nullable): #xerror_t for error reporting, or %NULL to ignore.
 *
 * Passes a file descriptor to the receiving side of the
 * connection. The receiving end has to call g_unix_connection_receive_fd()
 * to accept the file descriptor.
 *
 * As well as sending the fd this also writes a single byte to the
 * stream, as this is required for fd passing to work on some
 * implementations.
 *
 * Returns: a %TRUE on success, %NULL on error.
 *
 * Since: 2.22
 */
xboolean_t
g_unix_connection_send_fd (GUnixConnection  *connection,
                           xint_t              fd,
                           xcancellable_t     *cancellable,
                           xerror_t          **error)
{
#ifdef G_OS_UNIX
  xsocket_control_message_t *scm;
  xsocket_t *socket;

  g_return_val_if_fail (X_IS_UNIX_CONNECTION (connection), FALSE);
  g_return_val_if_fail (fd >= 0, FALSE);

  scm = g_unix_fd_message_new ();

  if (!g_unix_fd_message_append_fd (G_UNIX_FD_MESSAGE (scm), fd, error))
    {
      xobject_unref (scm);
      return FALSE;
    }

  xobject_get (connection, "socket", &socket, NULL);
  if (xsocket_send_message (socket, NULL, NULL, 0, &scm, 1, 0, cancellable, error) != 1)
    /* XXX could it 'fail' with zero? */
    {
      xobject_unref (socket);
      xobject_unref (scm);

      return FALSE;
    }

  xobject_unref (socket);
  xobject_unref (scm);

  return TRUE;
#else
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Sending FD is not supported"));
  return FALSE;
#endif
}

/**
 * g_unix_connection_receive_fd:
 * @connection: a #GUnixConnection
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore
 * @error: (nullable): #xerror_t for error reporting, or %NULL to ignore
 *
 * Receives a file descriptor from the sending end of the connection.
 * The sending end has to call g_unix_connection_send_fd() for this
 * to work.
 *
 * As well as reading the fd this also reads a single byte from the
 * stream, as this is required for fd passing to work on some
 * implementations.
 *
 * Returns: a file descriptor on success, -1 on error.
 *
 * Since: 2.22
 **/
xint_t
g_unix_connection_receive_fd (GUnixConnection  *connection,
                              xcancellable_t     *cancellable,
                              xerror_t          **error)
{
#ifdef G_OS_UNIX
  xsocket_control_message_t **scms;
  xint_t *fds, nfd, fd, nscm;
  GUnixFDMessage *fdmsg;
  xsocket_t *socket;

  g_return_val_if_fail (X_IS_UNIX_CONNECTION (connection), -1);

  xobject_get (connection, "socket", &socket, NULL);
  if (xsocket_receive_message (socket, NULL, NULL, 0,
                                &scms, &nscm, NULL, cancellable, error) != 1)
    /* XXX it _could_ 'fail' with zero. */
    {
      xobject_unref (socket);

      return -1;
    }

  xobject_unref (socket);

  if (nscm != 1)
    {
      xint_t i;

      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
        ngettext("Expecting 1 control message, got %d",
                 "Expecting 1 control message, got %d",
                 nscm),
        nscm);

      for (i = 0; i < nscm; i++)
        xobject_unref (scms[i]);

      g_free (scms);

      return -1;
    }

  if (!X_IS_UNIX_FD_MESSAGE (scms[0]))
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
			   _("Unexpected type of ancillary data"));
      xobject_unref (scms[0]);
      g_free (scms);

      return -1;
    }

  fdmsg = G_UNIX_FD_MESSAGE (scms[0]);
  g_free (scms);

  fds = g_unix_fd_message_steal_fds (fdmsg, &nfd);
  xobject_unref (fdmsg);

  if (nfd != 1)
    {
      xint_t i;

      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   ngettext("Expecting one fd, but got %d\n",
                            "Expecting one fd, but got %d\n",
                            nfd),
                   nfd);

      for (i = 0; i < nfd; i++)
        close (fds[i]);

      g_free (fds);

      return -1;
    }

  fd = *fds;
  g_free (fds);

  if (fd < 0)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           _("Received invalid fd"));
      fd = -1;
    }

  return fd;
#else
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Receiving FD is not supported"));
  return -1;
#endif
}

static void
g_unix_connection_init (GUnixConnection *connection)
{
}

static void
g_unix_connection_class_init (GUnixConnectionClass *class)
{
}

/* TODO: Other stuff we might want to add are:
void                    g_unix_connection_send_fd_async                 (GUnixConnection      *connection,
                                                                         xint_t                  fd,
                                                                         xboolean_t              close,
                                                                         xint_t                  io_priority,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
xboolean_t                g_unix_connection_send_fd_finish                (GUnixConnection      *connection,
                                                                         xerror_t              **error);

xboolean_t                g_unix_connection_send_fds                      (GUnixConnection      *connection,
                                                                         xint_t                 *fds,
                                                                         xint_t                  nfds,
                                                                         xerror_t              **error);
void                    g_unix_connection_send_fds_async                (GUnixConnection      *connection,
                                                                         xint_t                 *fds,
                                                                         xint_t                  nfds,
                                                                         xint_t                  io_priority,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
xboolean_t                g_unix_connection_send_fds_finish               (GUnixConnection      *connection,
                                                                         xerror_t              **error);

void                    g_unix_connection_receive_fd_async              (GUnixConnection      *connection,
                                                                         xint_t                  io_priority,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
xint_t                    g_unix_connection_receive_fd_finish             (GUnixConnection      *connection,
                                                                         xerror_t              **error);


xboolean_t                g_unix_connection_send_fake_credentials         (GUnixConnection      *connection,
                                                                         xuint64_t               pid,
                                                                         xuint64_t               uid,
                                                                         xuint64_t               gid,
                                                                         xerror_t              **error);
void                    g_unix_connection_send_fake_credentials_async   (GUnixConnection      *connection,
                                                                         xuint64_t               pid,
                                                                         xuint64_t               uid,
                                                                         xuint64_t               gid,
                                                                         xint_t                  io_priority,
                                                                         xasync_ready_callback_t   callback,
                                                                         xpointer_t              user_data);
xboolean_t                g_unix_connection_send_fake_credentials_finish  (GUnixConnection      *connection,
                                                                         xerror_t              **error);

xboolean_t                g_unix_connection_create_pair                   (GUnixConnection     **one,
                                                                         GUnixConnection     **two,
                                                                         xerror_t              **error);
*/


/**
 * g_unix_connection_send_credentials:
 * @connection: A #GUnixConnection.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Passes the credentials of the current user the receiving side
 * of the connection. The receiving end has to call
 * g_unix_connection_receive_credentials() (or similar) to accept the
 * credentials.
 *
 * As well as sending the credentials this also writes a single NUL
 * byte to the stream, as this is required for credentials passing to
 * work on some implementations.
 *
 * This method can be expected to be available on the following platforms:
 *
 * - Linux since GLib 2.26
 * - FreeBSD since GLib 2.26
 * - GNU/kFreeBSD since GLib 2.36
 * - Solaris, Illumos and OpenSolaris since GLib 2.40
 * - GNU/Hurd since GLib 2.40
 *
 * Other ways to exchange credentials with a foreign peer includes the
 * #xunix_credentials_message_t type and xsocket_get_credentials() function.
 *
 * Returns: %TRUE on success, %FALSE if @error is set.
 *
 * Since: 2.26
 */
xboolean_t
g_unix_connection_send_credentials (GUnixConnection      *connection,
                                    xcancellable_t         *cancellable,
                                    xerror_t              **error)
{
  xcredentials_t *credentials;
  xsocket_control_message_t *scm;
  xsocket_t *socket;
  xboolean_t ret;
  xoutput_vector_t vector;
  xuchar_t nul_byte[1] = {'\0'};
  xint_t num_messages;

  g_return_val_if_fail (X_IS_UNIX_CONNECTION (connection), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;

  credentials = xcredentials_new ();

  vector.buffer = &nul_byte;
  vector.size = 1;

  if (g_unix_credentials_message_is_supported ())
    {
      scm = g_unix_credentials_message_new_with_credentials (credentials);
      num_messages = 1;
    }
  else
    {
      scm = NULL;
      num_messages = 0;
    }

  xobject_get (connection, "socket", &socket, NULL);
  if (xsocket_send_message (socket,
                             NULL, /* address */
                             &vector,
                             1,
                             &scm,
                             num_messages,
                             XSOCKET_MSG_NONE,
                             cancellable,
                             error) != 1)
    {
      g_prefix_error (error, _("Error sending credentials: "));
      goto out;
    }

  ret = TRUE;

 out:
  xobject_unref (socket);
  if (scm != NULL)
    xobject_unref (scm);
  xobject_unref (credentials);
  return ret;
}

static void
send_credentials_async_thread (xtask_t         *task,
			       xpointer_t       source_object,
			       xpointer_t       task_data,
			       xcancellable_t  *cancellable)
{
  xerror_t *error = NULL;

  if (g_unix_connection_send_credentials (G_UNIX_CONNECTION (source_object),
					  cancellable,
					  &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * g_unix_connection_send_credentials_async:
 * @connection: A #GUnixConnection.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously send credentials.
 *
 * For more details, see g_unix_connection_send_credentials() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called. You can then call
 * g_unix_connection_send_credentials_finish() to get the result of the operation.
 *
 * Since: 2.32
 **/
void
g_unix_connection_send_credentials_async (GUnixConnection      *connection,
                                          xcancellable_t         *cancellable,
                                          xasync_ready_callback_t   callback,
                                          xpointer_t              user_data)
{
  xtask_t *task;

  task = xtask_new (connection, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_unix_connection_send_credentials_async);
  xtask_run_in_thread (task, send_credentials_async_thread);
}

/**
 * g_unix_connection_send_credentials_finish:
 * @connection: A #GUnixConnection.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous send credentials operation started with
 * g_unix_connection_send_credentials_async().
 *
 * Returns: %TRUE if the operation was successful, otherwise %FALSE.
 *
 * Since: 2.32
 **/
xboolean_t
g_unix_connection_send_credentials_finish (GUnixConnection *connection,
                                           xasync_result_t    *result,
                                           xerror_t         **error)
{
  g_return_val_if_fail (xtask_is_valid (result, connection), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

/**
 * g_unix_connection_receive_credentials:
 * @connection: A #GUnixConnection.
 * @cancellable: (nullable): A #xcancellable_t or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Receives credentials from the sending end of the connection.  The
 * sending end has to call g_unix_connection_send_credentials() (or
 * similar) for this to work.
 *
 * As well as reading the credentials this also reads (and discards) a
 * single byte from the stream, as this is required for credentials
 * passing to work on some implementations.
 *
 * This method can be expected to be available on the following platforms:
 *
 * - Linux since GLib 2.26
 * - FreeBSD since GLib 2.26
 * - GNU/kFreeBSD since GLib 2.36
 * - Solaris, Illumos and OpenSolaris since GLib 2.40
 * - GNU/Hurd since GLib 2.40
 *
 * Other ways to exchange credentials with a foreign peer includes the
 * #xunix_credentials_message_t type and xsocket_get_credentials() function.
 *
 * Returns: (transfer full): Received credentials on success (free with
 * xobject_unref()), %NULL if @error is set.
 *
 * Since: 2.26
 */
xcredentials_t *
g_unix_connection_receive_credentials (GUnixConnection      *connection,
                                       xcancellable_t         *cancellable,
                                       xerror_t              **error)
{
  xcredentials_t *ret;
  xsocket_control_message_t **scms;
  xint_t nscm;
  xsocket_t *socket;
  xint_t n;
  xssize_t num_bytes_read;
#ifdef __linux__
  xboolean_t turn_off_so_passcreds;
#endif

  g_return_val_if_fail (X_IS_UNIX_CONNECTION (connection), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = NULL;
  scms = NULL;

  xobject_get (connection, "socket", &socket, NULL);

  /* On Linux, we need to turn on SO_PASSCRED if it isn't enabled
   * already. We also need to turn it off when we're done.  See
   * #617483 for more discussion.
   */
#ifdef __linux__
  {
    xint_t opt_val;

    turn_off_so_passcreds = FALSE;
    opt_val = 0;
    if (!xsocket_get_option (socket,
			      SOL_SOCKET,
			      SO_PASSCRED,
			      &opt_val,
			      NULL))
      {
        int errsv = errno;
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errsv),
                     _("Error checking if SO_PASSCRED is enabled for socket: %s"),
                     xstrerror (errsv));
        goto out;
      }
    if (opt_val == 0)
      {
        if (!xsocket_set_option (socket,
				  SOL_SOCKET,
				  SO_PASSCRED,
				  TRUE,
				  NULL))
          {
            int errsv = errno;
            g_set_error (error,
                         G_IO_ERROR,
                         g_io_error_from_errno (errsv),
                         _("Error enabling SO_PASSCRED: %s"),
                         xstrerror (errsv));
            goto out;
          }
        turn_off_so_passcreds = TRUE;
      }
  }
#endif

  xtype_ensure (XTYPE_UNIX_CREDENTIALS_MESSAGE);
  num_bytes_read = xsocket_receive_message (socket,
                                             NULL, /* xsocket_address_t **address */
                                             NULL,
                                             0,
                                             &scms,
                                             &nscm,
                                             NULL,
                                             cancellable,
                                             error);
  if (num_bytes_read != 1)
    {
      /* Handle situation where xsocket_receive_message() returns
       * 0 bytes and not setting @error
       */
      if (num_bytes_read == 0 && error != NULL && *error == NULL)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               _("Expecting to read a single byte for receiving credentials but read zero bytes"));
        }
      goto out;
    }

  if (g_unix_credentials_message_is_supported () &&
      /* Fall back on get_credentials if the other side didn't send the credentials */
      nscm > 0)
    {
      if (nscm != 1)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       ngettext("Expecting 1 control message, got %d",
                                "Expecting 1 control message, got %d",
                                nscm),
                       nscm);
          goto out;
        }

      if (!X_IS_UNIX_CREDENTIALS_MESSAGE (scms[0]))
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               _("Unexpected type of ancillary data"));
          goto out;
        }

      ret = g_unix_credentials_message_get_credentials (G_UNIX_CREDENTIALS_MESSAGE (scms[0]));
      xobject_ref (ret);
    }
  else
    {
      if (nscm != 0)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       _("Not expecting control message, but got %d"),
                       nscm);
          goto out;
        }
      else
        {
          ret = xsocket_get_credentials (socket, error);
        }
    }

 out:

#ifdef __linux__
  if (turn_off_so_passcreds)
    {
      if (!xsocket_set_option (socket,
				SOL_SOCKET,
				SO_PASSCRED,
				FALSE,
				NULL))
        {
          int errsv = errno;
          g_set_error (error,
                       G_IO_ERROR,
                       g_io_error_from_errno (errsv),
                       _("Error while disabling SO_PASSCRED: %s"),
                       xstrerror (errsv));
          goto out;
        }
    }
#endif

  if (scms != NULL)
    {
      for (n = 0; n < nscm; n++)
        xobject_unref (scms[n]);
      g_free (scms);
    }
  xobject_unref (socket);
  return ret;
}

static void
receive_credentials_async_thread (xtask_t         *task,
				  xpointer_t       source_object,
				  xpointer_t       task_data,
				  xcancellable_t  *cancellable)
{
  xcredentials_t *creds;
  xerror_t *error = NULL;

  creds = g_unix_connection_receive_credentials (G_UNIX_CONNECTION (source_object),
                                                 cancellable,
                                                 &error);
  if (creds)
    xtask_return_pointer (task, creds, xobject_unref);
  else
    xtask_return_error (task, error);
  xobject_unref (task);
}

/**
 * g_unix_connection_receive_credentials_async:
 * @connection: A #GUnixConnection.
 * @cancellable: (nullable): optional #xcancellable_t object, %NULL to ignore.
 * @callback: (scope async): a #xasync_ready_callback_t to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously receive credentials.
 *
 * For more details, see g_unix_connection_receive_credentials() which is
 * the synchronous version of this call.
 *
 * When the operation is finished, @callback will be called. You can then call
 * g_unix_connection_receive_credentials_finish() to get the result of the operation.
 *
 * Since: 2.32
 **/
void
g_unix_connection_receive_credentials_async (GUnixConnection      *connection,
                                              xcancellable_t         *cancellable,
                                              xasync_ready_callback_t   callback,
                                              xpointer_t              user_data)
{
  xtask_t *task;

  task = xtask_new (connection, cancellable, callback, user_data);
  xtask_set_source_tag (task, g_unix_connection_receive_credentials_async);
  xtask_run_in_thread (task, receive_credentials_async_thread);
}

/**
 * g_unix_connection_receive_credentials_finish:
 * @connection: A #GUnixConnection.
 * @result: a #xasync_result_t.
 * @error: a #xerror_t, or %NULL
 *
 * Finishes an asynchronous receive credentials operation started with
 * g_unix_connection_receive_credentials_async().
 *
 * Returns: (transfer full): a #xcredentials_t, or %NULL on error.
 *     Free the returned object with xobject_unref().
 *
 * Since: 2.32
 **/
xcredentials_t *
g_unix_connection_receive_credentials_finish (GUnixConnection *connection,
                                              xasync_result_t    *result,
                                              xerror_t         **error)
{
  g_return_val_if_fail (xtask_is_valid (result, connection), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}
