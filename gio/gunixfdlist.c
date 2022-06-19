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

/**
 * SECTION:gunixfdlist
 * @title: xunix_fd_list_t
 * @short_description: An object containing a set of UNIX file descriptors
 * @include: gio/gunixfdlist.h
 * @see_also: #GUnixFDMessage
 *
 * A #xunix_fd_list_t contains a list of file descriptors.  It owns the file
 * descriptors that it contains, closing them when finalized.
 *
 * It may be wrapped in a #GUnixFDMessage and sent over a #xsocket_t in
 * the %XSOCKET_FAMILY_UNIX family by using xsocket_send_message()
 * and received using xsocket_receive_message().
 *
 * Note that `<gio/gunixfdlist.h>` belongs to the UNIX-specific GIO
 * interfaces, thus you have to use the `gio-unix-2.0.pc` pkg-config
 * file when using it.
 */

/**
 * xunix_fd_list_t:
 *
 * #xunix_fd_list_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

#include "config.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "gunixfdlist.h"
#include "gnetworking.h"
#include "gioerror.h"

struct _GUnixFDListPrivate
{
  xint_t *fds;
  xint_t nfd;
};

G_DEFINE_TYPE_WITH_PRIVATE (xunix_fd_list_t, g_unix_fd_list, XTYPE_OBJECT)

static void
g_unix_fd_list_init (xunix_fd_list_t *list)
{
  list->priv = g_unix_fd_list_get_instance_private (list);
}

static void
g_unix_fd_list_finalize (xobject_t *object)
{
  xunix_fd_list_t *list = G_UNIX_FD_LIST (object);
  xint_t i;

  for (i = 0; i < list->priv->nfd; i++)
    close (list->priv->fds[i]);
  g_free (list->priv->fds);

  XOBJECT_CLASS (g_unix_fd_list_parent_class)
    ->finalize (object);
}

static void
g_unix_fd_list_class_init (GUnixFDListClass *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->finalize = g_unix_fd_list_finalize;
}

static int
dup_close_on_exec_fd (xint_t     fd,
                      xerror_t **error)
{
  xint_t new_fd;
  xint_t s;

#ifdef F_DUPFD_CLOEXEC
  do
    new_fd = fcntl (fd, F_DUPFD_CLOEXEC, 0l);
  while (new_fd < 0 && (errno == EINTR));

  if (new_fd >= 0)
    return new_fd;

  /* if that didn't work (new libc/old kernel?), try it the other way. */
#endif

  do
    new_fd = dup (fd);
  while (new_fd < 0 && (errno == EINTR));

  if (new_fd < 0)
    {
      int saved_errno = errno;

      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (saved_errno),
                   "dup: %s", xstrerror (saved_errno));

      return -1;
    }

  do
    {
      s = fcntl (new_fd, F_GETFD);

      if (s >= 0)
        s = fcntl (new_fd, F_SETFD, (long) (s | FD_CLOEXEC));
    }
  while (s < 0 && (errno == EINTR));

  if (s < 0)
    {
      int saved_errno = errno;

      g_set_error (error, G_IO_ERROR,
                   g_io_error_from_errno (saved_errno),
                   "fcntl: %s", xstrerror (saved_errno));
      close (new_fd);

      return -1;
    }

  return new_fd;
}

/**
 * g_unix_fd_list_new:
 *
 * Creates a new #xunix_fd_list_t containing no file descriptors.
 *
 * Returns: a new #xunix_fd_list_t
 *
 * Since: 2.24
 **/
xunix_fd_list_t *
g_unix_fd_list_new (void)
{
  return xobject_new (XTYPE_UNIX_FD_LIST, NULL);
}

/**
 * g_unix_fd_list_new_from_array:
 * @fds: (array length=n_fds): the initial list of file descriptors
 * @n_fds: the length of #fds, or -1
 *
 * Creates a new #xunix_fd_list_t containing the file descriptors given in
 * @fds.  The file descriptors become the property of the new list and
 * may no longer be used by the caller.  The array itself is owned by
 * the caller.
 *
 * Each file descriptor in the array should be set to close-on-exec.
 *
 * If @n_fds is -1 then @fds must be terminated with -1.
 *
 * Returns: a new #xunix_fd_list_t
 *
 * Since: 2.24
 **/
xunix_fd_list_t *
g_unix_fd_list_new_from_array (const xint_t *fds,
                               xint_t        n_fds)
{
  xunix_fd_list_t *list;

  xreturn_val_if_fail (fds != NULL || n_fds == 0, NULL);

  if (n_fds == -1)
    for (n_fds = 0; fds[n_fds] != -1; n_fds++);

  list = xobject_new (XTYPE_UNIX_FD_LIST, NULL);
  list->priv->fds = g_new (xint_t, n_fds + 1);
  list->priv->nfd = n_fds;

  if (n_fds > 0)
    memcpy (list->priv->fds, fds, sizeof (xint_t) * n_fds);
  list->priv->fds[n_fds] = -1;

  return list;
}

/**
 * g_unix_fd_list_steal_fds:
 * @list: a #xunix_fd_list_t
 * @length: (out) (optional): pointer to the length of the returned
 *     array, or %NULL
 *
 * Returns the array of file descriptors that is contained in this
 * object.
 *
 * After this call, the descriptors are no longer contained in
 * @list. Further calls will return an empty list (unless more
 * descriptors have been added).
 *
 * The return result of this function must be freed with g_free().
 * The caller is also responsible for closing all of the file
 * descriptors.  The file descriptors in the array are set to
 * close-on-exec.
 *
 * If @length is non-%NULL then it is set to the number of file
 * descriptors in the returned array. The returned array is also
 * terminated with -1.
 *
 * This function never returns %NULL. In case there are no file
 * descriptors contained in @list, an empty array is returned.
 *
 * Returns: (array length=length) (transfer full): an array of file
 *     descriptors
 *
 * Since: 2.24
 */
xint_t *
g_unix_fd_list_steal_fds (xunix_fd_list_t *list,
                          xint_t        *length)
{
  xint_t *result;

  xreturn_val_if_fail (X_IS_UNIX_FD_LIST (list), NULL);

  /* will be true for fresh object or if we were just called */
  if (list->priv->fds == NULL)
    {
      list->priv->fds = g_new (xint_t, 1);
      list->priv->fds[0] = -1;
      list->priv->nfd = 0;
    }

  if (length)
    *length = list->priv->nfd;
  result = list->priv->fds;

  list->priv->fds = NULL;
  list->priv->nfd = 0;

  return result;
}

/**
 * g_unix_fd_list_peek_fds:
 * @list: a #xunix_fd_list_t
 * @length: (out) (optional): pointer to the length of the returned
 *     array, or %NULL
 *
 * Returns the array of file descriptors that is contained in this
 * object.
 *
 * After this call, the descriptors remain the property of @list.  The
 * caller must not close them and must not free the array.  The array is
 * valid only until @list is changed in any way.
 *
 * If @length is non-%NULL then it is set to the number of file
 * descriptors in the returned array. The returned array is also
 * terminated with -1.
 *
 * This function never returns %NULL. In case there are no file
 * descriptors contained in @list, an empty array is returned.
 *
 * Returns: (array length=length) (transfer none): an array of file
 *     descriptors
 *
 * Since: 2.24
 */
const xint_t *
g_unix_fd_list_peek_fds (xunix_fd_list_t *list,
                         xint_t        *length)
{
  xreturn_val_if_fail (X_IS_UNIX_FD_LIST (list), NULL);

  /* will be true for fresh object or if steal() was just called */
  if (list->priv->fds == NULL)
    {
      list->priv->fds = g_new (xint_t, 1);
      list->priv->fds[0] = -1;
      list->priv->nfd = 0;
    }

  if (length)
    *length = list->priv->nfd;

  return list->priv->fds;
}

/**
 * g_unix_fd_list_append:
 * @list: a #xunix_fd_list_t
 * @fd: a valid open file descriptor
 * @error: a #xerror_t pointer
 *
 * Adds a file descriptor to @list.
 *
 * The file descriptor is duplicated using dup(). You keep your copy
 * of the descriptor and the copy contained in @list will be closed
 * when @list is finalized.
 *
 * A possible cause of failure is exceeding the per-process or
 * system-wide file descriptor limit.
 *
 * The index of the file descriptor in the list is returned.  If you use
 * this index with g_unix_fd_list_get() then you will receive back a
 * duplicated copy of the same file descriptor.
 *
 * Returns: the index of the appended fd in case of success, else -1
 *          (and @error is set)
 *
 * Since: 2.24
 */
xint_t
g_unix_fd_list_append (xunix_fd_list_t  *list,
                       xint_t          fd,
                       xerror_t      **error)
{
  xint_t new_fd;

  xreturn_val_if_fail (X_IS_UNIX_FD_LIST (list), -1);
  xreturn_val_if_fail (fd >= 0, -1);
  xreturn_val_if_fail (error == NULL || *error == NULL, -1);

  if ((new_fd = dup_close_on_exec_fd (fd, error)) < 0)
    return -1;

  list->priv->fds = g_realloc (list->priv->fds,
                                  sizeof (xint_t) *
                                   (list->priv->nfd + 2));
  list->priv->fds[list->priv->nfd++] = new_fd;
  list->priv->fds[list->priv->nfd] = -1;

  return list->priv->nfd - 1;
}

/**
 * g_unix_fd_list_get:
 * @list: a #xunix_fd_list_t
 * @index_: the index into the list
 * @error: a #xerror_t pointer
 *
 * Gets a file descriptor out of @list.
 *
 * @index_ specifies the index of the file descriptor to get.  It is a
 * programmer error for @index_ to be out of range; see
 * g_unix_fd_list_get_length().
 *
 * The file descriptor is duplicated using dup() and set as
 * close-on-exec before being returned.  You must call close() on it
 * when you are done.
 *
 * A possible cause of failure is exceeding the per-process or
 * system-wide file descriptor limit.
 *
 * Returns: the file descriptor, or -1 in case of error
 *
 * Since: 2.24
 **/
xint_t
g_unix_fd_list_get (xunix_fd_list_t  *list,
                    xint_t          index_,
                    xerror_t      **error)
{
  xreturn_val_if_fail (X_IS_UNIX_FD_LIST (list), -1);
  xreturn_val_if_fail (index_ < list->priv->nfd, -1);
  xreturn_val_if_fail (error == NULL || *error == NULL, -1);

  return dup_close_on_exec_fd (list->priv->fds[index_], error);
}

/**
 * g_unix_fd_list_get_length:
 * @list: a #xunix_fd_list_t
 *
 * Gets the length of @list (ie: the number of file descriptors
 * contained within).
 *
 * Returns: the length of @list
 *
 * Since: 2.24
 **/
xint_t
g_unix_fd_list_get_length (xunix_fd_list_t *list)
{
  xreturn_val_if_fail (X_IS_UNIX_FD_LIST (list), 0);

  return list->priv->nfd;
}
