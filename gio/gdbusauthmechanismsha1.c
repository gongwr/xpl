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

#include "config.h"

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h>
#include "gwin32sid.h"
#endif

#include "gdbusauthmechanismsha1.h"
#include "gcredentials.h"
#include "gdbuserror.h"
#include "glocalfileinfo.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "gdbusprivate.h"
#include "glib-private.h"

#include "glibintl.h"

/*
 * Arbitrary timeouts for keys in the keyring.
 * For interoperability, these match the reference implementation, libdbus.
 * To make them easier to compare, their names also match libdbus
 * (see dbus/dbus-keyring.c).
 */

/*
 * Maximum age of a key before we create a new key to use in challenges:
 * 5 minutes.
 */
#define NEW_KEY_TIMEOUT_SECONDS (60*5)

/*
 * Time before we drop a key from the keyring: 7 minutes.
 * Authentication will succeed if it takes less than
 * EXPIRE_KEYS_TIMEOUT_SECONDS - NEW_KEY_TIMEOUT_SECONDS (2 minutes)
 * to complete.
 * The spec says "delete any cookies that are old (the timeout can be
 * fairly short)".
 */
#define EXPIRE_KEYS_TIMEOUT_SECONDS (NEW_KEY_TIMEOUT_SECONDS + (60*2))

/*
 * Maximum amount of time a key can be in the future due to clock skew
 * with a shared home directory: 5 minutes.
 * The spec says "a reasonable time in the future".
 */
#define MAX_TIME_TRAVEL_SECONDS (60*5)


struct _GDBusAuthMechanismSha1Private
{
  xboolean_t is_client;
  xboolean_t is_server;
  xdbus_auth_mechanism_state_t
 state;
  xchar_t *reject_reason;  /* non-NULL iff (state == XDBUS_AUTH_MECHANISM_STATE_REJECTED) */

  /* used on the client side */
  xchar_t *to_send;

  /* used on the server side */
  xchar_t *cookie;
  xchar_t *server_challenge;
};

static xint_t                     mechanism_get_priority              (void);
static const xchar_t             *mechanism_get_name                  (void);

static xboolean_t                 mechanism_is_supported              (xdbus_auth_mechanism_t   *mechanism);
static xchar_t                   *mechanism_encode_data               (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len,
                                                                     xsize_t                *out_data_len);
static xchar_t                   *mechanism_decode_data               (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len,
                                                                     xsize_t                *out_data_len);
static xdbus_auth_mechanism_state_t
  mechanism_server_get_state          (xdbus_auth_mechanism_t   *mechanism);
static void                     mechanism_server_initiate           (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *initial_response,
                                                                     xsize_t                 initial_response_len);
static void                     mechanism_server_data_receive       (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len);
static xchar_t                   *mechanism_server_data_send          (xdbus_auth_mechanism_t   *mechanism,
                                                                     xsize_t                *out_data_len);
static xchar_t                   *mechanism_server_get_reject_reason  (xdbus_auth_mechanism_t   *mechanism);
static void                     mechanism_server_shutdown           (xdbus_auth_mechanism_t   *mechanism);
static xdbus_auth_mechanism_state_t
  mechanism_client_get_state          (xdbus_auth_mechanism_t   *mechanism);
static xchar_t                   *mechanism_client_initiate           (xdbus_auth_mechanism_t   *mechanism,
                                                                     xsize_t                *out_initial_response_len);
static void                     mechanism_client_data_receive       (xdbus_auth_mechanism_t   *mechanism,
                                                                     const xchar_t          *data,
                                                                     xsize_t                 data_len);
static xchar_t                   *mechanism_client_data_send          (xdbus_auth_mechanism_t   *mechanism,
                                                                     xsize_t                *out_data_len);
static void                     mechanism_client_shutdown           (xdbus_auth_mechanism_t   *mechanism);

/* ---------------------------------------------------------------------------------------------------- */

G_DEFINE_TYPE_WITH_PRIVATE (GDBusAuthMechanismSha1, _xdbus_auth_mechanism_sha1, XTYPE_DBUS_AUTH_MECHANISM)

/* ---------------------------------------------------------------------------------------------------- */

static void
_xdbus_auth_mechanism_sha1_finalize (xobject_t *object)
{
  GDBusAuthMechanismSha1 *mechanism = XDBUS_AUTH_MECHANISM_SHA1 (object);

  g_free (mechanism->priv->reject_reason);
  g_free (mechanism->priv->to_send);

  g_free (mechanism->priv->cookie);
  g_free (mechanism->priv->server_challenge);

  if (XOBJECT_CLASS (_xdbus_auth_mechanism_sha1_parent_class)->finalize != NULL)
    XOBJECT_CLASS (_xdbus_auth_mechanism_sha1_parent_class)->finalize (object);
}

static void
_xdbus_auth_mechanism_sha1_class_init (GDBusAuthMechanismSha1Class *klass)
{
  xobject_class_t *xobject_class;
  xdbus_auth_mechanism_class_t *mechanism_class;

  xobject_class = XOBJECT_CLASS (klass);
  xobject_class->finalize = _xdbus_auth_mechanism_sha1_finalize;

  mechanism_class = XDBUS_AUTH_MECHANISM_CLASS (klass);
  mechanism_class->get_priority              = mechanism_get_priority;
  mechanism_class->get_name                  = mechanism_get_name;
  mechanism_class->is_supported              = mechanism_is_supported;
  mechanism_class->encode_data               = mechanism_encode_data;
  mechanism_class->decode_data               = mechanism_decode_data;
  mechanism_class->server_get_state          = mechanism_server_get_state;
  mechanism_class->server_initiate           = mechanism_server_initiate;
  mechanism_class->server_data_receive       = mechanism_server_data_receive;
  mechanism_class->server_data_send          = mechanism_server_data_send;
  mechanism_class->server_get_reject_reason  = mechanism_server_get_reject_reason;
  mechanism_class->server_shutdown           = mechanism_server_shutdown;
  mechanism_class->client_get_state          = mechanism_client_get_state;
  mechanism_class->client_initiate           = mechanism_client_initiate;
  mechanism_class->client_data_receive       = mechanism_client_data_receive;
  mechanism_class->client_data_send          = mechanism_client_data_send;
  mechanism_class->client_shutdown           = mechanism_client_shutdown;
}

static void
_xdbus_auth_mechanism_sha1_init (GDBusAuthMechanismSha1 *mechanism)
{
  mechanism->priv = _xdbus_auth_mechanism_sha1_get_instance_private (mechanism);
}

/* ---------------------------------------------------------------------------------------------------- */

static xint_t
mechanism_get_priority (void)
{
  return 0;
}

static const xchar_t *
mechanism_get_name (void)
{
  return "DBUS_COOKIE_SHA1";
}

static xboolean_t
mechanism_is_supported (xdbus_auth_mechanism_t *mechanism)
{
  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), FALSE);
  return TRUE;
}

static xchar_t *
mechanism_encode_data (xdbus_auth_mechanism_t   *mechanism,
                       const xchar_t          *data,
                       xsize_t                 data_len,
                       xsize_t                *out_data_len)
{
  return NULL;
}


static xchar_t *
mechanism_decode_data (xdbus_auth_mechanism_t   *mechanism,
                       const xchar_t          *data,
                       xsize_t                 data_len,
                       xsize_t                *out_data_len)
{
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

static xint_t
random_ascii (void)
{
  xint_t ret;
  ret = g_random_int_range (0, 60);
  if (ret < 25)
    ret += 'A';
  else if (ret < 50)
    ret += 'a' - 25;
  else
    ret += '0' - 50;
  return ret;
}

static xchar_t *
random_ascii_string (xuint_t len)
{
  xstring_t *challenge;
  xuint_t n;

  challenge = xstring_new (NULL);
  for (n = 0; n < len; n++)
    xstring_append_c (challenge, random_ascii ());
  return xstring_free (challenge, FALSE);
}

static xchar_t *
random_blob (xuint_t len)
{
  xstring_t *challenge;
  xuint_t n;

  challenge = xstring_new (NULL);
  for (n = 0; n < len; n++)
    xstring_append_c (challenge, g_random_int_range (0, 256));
  return xstring_free (challenge, FALSE);
}

/* ---------------------------------------------------------------------------------------------------- */

/* ensure keyring dir exists and permissions are correct */
static xchar_t *
ensure_keyring_directory (xerror_t **error)
{
  xchar_t *path;
  const xchar_t *e;
  xboolean_t is_setuid;
#ifdef G_OS_UNIX
  struct stat statbuf;
#endif

  xreturn_val_if_fail (error == NULL || *error == NULL, NULL);

  e = g_getenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR");
  if (e != NULL)
    {
      path = xstrdup (e);
    }
  else
    {
      path = g_build_filename (g_get_home_dir (),
                               ".dbus-keyrings",
                               NULL);
    }

#ifdef G_OS_UNIX
  if (stat (path, &statbuf) != 0)
    {
      int errsv = errno;

      if (errsv != ENOENT)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       g_io_error_from_errno (errsv),
                       _("Error when getting information for directory “%s”: %s"),
                       path,
                       xstrerror (errsv));
          g_clear_pointer (&path, g_free);
          return NULL;
        }
    }
  else if (S_ISDIR (statbuf.st_mode))
    {
      if (g_getenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR_IGNORE_PERMISSION") == NULL &&
          (statbuf.st_mode & 0777) != 0700)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       _("Permissions on directory “%s” are malformed. Expected mode 0700, got 0%o"),
                       path,
                       (xuint_t) (statbuf.st_mode & 0777));
          g_clear_pointer (&path, g_free);
          return NULL;
        }

      return g_steal_pointer (&path);
    }
#else  /* if !G_OS_UNIX */
  /* On non-Unix platforms, check that it exists as a directory, but don’t do
   * permissions checks at the moment. */
  if (xfile_test (path, XFILE_TEST_EXISTS | XFILE_TEST_IS_DIR))
    {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
#warning Please implement permission checking on this non-UNIX platform
#pragma GCC diagnostic pop
#endif  /* __GNUC__ */
      return g_steal_pointer (&path);
    }
#endif  /* if !G_OS_UNIX */

  /* Only create the directory if not running as setuid */
  is_setuid = XPL_PRIVATE_CALL (g_check_setuid) ();
  if (!is_setuid &&
      g_mkdir_with_parents (path, 0700) != 0)
    {
      int errsv = errno;
      g_set_error (error,
                   G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   _("Error creating directory “%s”: %s"),
                   path,
                   xstrerror (errsv));
      g_clear_pointer (&path, g_free);
      return NULL;
    }
  else if (is_setuid)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_PERMISSION_DENIED,
                   _("Error creating directory “%s”: %s"),
                   path,
                   _("Operation not supported"));
      g_clear_pointer (&path, g_free);
      return NULL;
    }

  return g_steal_pointer (&path);
}

/* ---------------------------------------------------------------------------------------------------- */

/* looks up an entry in the keyring */
static xchar_t *
keyring_lookup_entry (const xchar_t  *cookie_context,
                      xint_t          cookie_id,
                      xerror_t      **error)
{
  xchar_t *ret;
  xchar_t *keyring_dir;
  xchar_t *contents;
  xchar_t *path;
  xuint_t n;
  xchar_t **lines;

  xreturn_val_if_fail (cookie_context != NULL, NULL);
  xreturn_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = NULL;
  path = NULL;
  contents = NULL;
  lines = NULL;

  keyring_dir = ensure_keyring_directory (error);
  if (keyring_dir == NULL)
    goto out;

  path = g_build_filename (keyring_dir, cookie_context, NULL);

  if (!xfile_get_contents (path,
                            &contents,
                            NULL,
                            error))
    {
      g_prefix_error (error,
                      _("Error opening keyring “%s” for reading: "),
                      path);
      goto out;
    }
  xassert (contents != NULL);

  lines = xstrsplit (contents, "\n", 0);
  for (n = 0; lines[n] != NULL; n++)
    {
      const xchar_t *line = lines[n];
      xchar_t **tokens;
      xchar_t *endp;
      xint_t line_id;

      if (line[0] == '\0')
        continue;

      tokens = xstrsplit (line, " ", 0);
      if (xstrv_length (tokens) != 3)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       _("Line %d of the keyring at “%s” with content “%s” is malformed"),
                       n + 1,
                       path,
                       line);
          xstrfreev (tokens);
          goto out;
        }

      line_id = g_ascii_strtoll (tokens[0], &endp, 10);
      if (*endp != '\0')
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       _("First token of line %d of the keyring at “%s” with content “%s” is malformed"),
                       n + 1,
                       path,
                       line);
          xstrfreev (tokens);
          goto out;
        }

      (void)g_ascii_strtoll (tokens[1], &endp, 10); /* do not care what the timestamp is */
      if (*endp != '\0')
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       _("Second token of line %d of the keyring at “%s” with content “%s” is malformed"),
                       n + 1,
                       path,
                       line);
          xstrfreev (tokens);
          goto out;
        }

      if (line_id == cookie_id)
        {
          /* YAY, success */
          ret = tokens[2]; /* steal pointer */
          tokens[2] = NULL;
          xstrfreev (tokens);
          goto out;
        }

      xstrfreev (tokens);
    }

  /* BOOH, didn't find the cookie */
  g_set_error (error,
               G_IO_ERROR,
               G_IO_ERROR_FAILED,
               _("Didn’t find cookie with id %d in the keyring at “%s”"),
               cookie_id,
               path);

 out:
  g_free (keyring_dir);
  g_free (path);
  g_free (contents);
  xstrfreev (lines);
  return ret;
}

/* function for logging important events that the system administrator should take notice of */
G_GNUC_PRINTF(1, 2)
static void
_log (const xchar_t *message,
      ...)
{
  xchar_t *s;
  va_list var_args;

  va_start (var_args, message);
  s = xstrdup_vprintf (message, var_args);
  va_end (var_args);

  /* TODO: might want to send this to syslog instead */
  g_printerr ("GDBus-DBUS_COOKIE_SHA1: %s\n", s);
  g_free (s);
}

/* Returns FD for lock file, if it was created exclusively (didn't exist already,
 * and was created successfully) */
static xint_t
create_lock_exclusive (const xchar_t  *lock_path,
                       sint64_t       *mtime_nsec,
                       xerror_t      **error)
{
  int errsv;
  xint_t ret;

  ret = g_open (lock_path, O_CREAT | O_EXCL, 0600);
  errsv = errno;
  if (ret < 0)
    {
      GLocalFileStat stat_buf;

      /* Get the modification time to distinguish between the lock being stale
       * or highly contested. */
      if (mtime_nsec != NULL &&
          g_local_file_stat (lock_path, G_LOCAL_FILE_STAT_FIELD_MTIME, G_LOCAL_FILE_STAT_FIELD_ALL, &stat_buf) == 0)
        *mtime_nsec = _g_stat_mtime (&stat_buf) * G_USEC_PER_SEC * 1000 + _g_stat_mtim_nsec (&stat_buf);
      else if (mtime_nsec != NULL)
        *mtime_nsec = 0;

      g_set_error (error,
                   G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   _("Error creating lock file “%s”: %s"),
                   lock_path,
                   xstrerror (errsv));
      return -1;
    }

  return ret;
}

static xint_t
keyring_acquire_lock (const xchar_t  *path,
                      xerror_t      **error)
{
  xchar_t *lock = NULL;
  xint_t ret;
  xuint_t num_tries;
  int errsv;
  sint64_t lock_mtime_nsec = 0, lock_mtime_nsec_prev = 0;

  /* Total possible sleep period = max_tries * timeout_usec = 0.5s */
  const xuint_t max_tries = 50;
  const xuint_t timeout_usec = 1000 * 10;

  xreturn_val_if_fail (path != NULL, -1);
  xreturn_val_if_fail (error == NULL || *error == NULL, -1);

  ret = -1;
  lock = xstrconcat (path, ".lock", NULL);

  /* This is what the D-Bus spec says
   * (https://dbus.freedesktop.org/doc/dbus-specification.html#auth-mechanisms-sha)
   *
   *  Create a lockfile name by appending ".lock" to the name of the
   *  cookie file. The server should attempt to create this file using
   *  O_CREAT | O_EXCL. If file creation fails, the lock
   *  fails. Servers should retry for a reasonable period of time,
   *  then they may choose to delete an existing lock to keep users
   *  from having to manually delete a stale lock. [1]
   *
   *  [1] : Lockfiles are used instead of real file locking fcntl() because
   *         real locking implementations are still flaky on network filesystems
   */

  for (num_tries = 0; num_tries < max_tries; num_tries++)
    {
      lock_mtime_nsec_prev = lock_mtime_nsec;

      /* Ignore the error until the final call. */
      ret = create_lock_exclusive (lock, &lock_mtime_nsec, NULL);
      if (ret >= 0)
        break;

      /* sleep 10ms, then try again */
      g_usleep (timeout_usec);

      /* If the mtime of the lock file changed, don’t count the retry, as it
       * seems like there’s contention between processes for the lock file,
       * rather than a stale lock file from a crashed process. */
      if (num_tries > 0 && lock_mtime_nsec != lock_mtime_nsec_prev)
        num_tries--;
    }

  if (num_tries == max_tries)
    {
      /* ok, we slept 50*10ms = 0.5 seconds. Conclude that the lock file must be
       * stale (nuke it from orbit)
       */
      if (g_unlink (lock) != 0)
        {
          errsv = errno;
          g_set_error (error,
                       G_IO_ERROR,
                       g_io_error_from_errno (errsv),
                       _("Error deleting stale lock file “%s”: %s"),
                       lock,
                       xstrerror (errsv));
          goto out;
        }

      _log ("Deleted stale lock file '%s'", lock);

      /* Try one last time to create it, now that we've deleted the stale one */
      ret = create_lock_exclusive (lock, NULL, error);
      if (ret < 0)
        goto out;
    }

out:
  g_free (lock);
  return ret;
}

static xboolean_t
keyring_release_lock (const xchar_t  *path,
                      xint_t          lock_fd,
                      xerror_t      **error)
{
  xchar_t *lock;
  xboolean_t ret;

  xreturn_val_if_fail (path != NULL, FALSE);
  xreturn_val_if_fail (lock_fd != -1, FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;
  lock = xstrdup_printf ("%s.lock", path);
  if (close (lock_fd) != 0)
    {
      int errsv = errno;
      g_set_error (error,
                   G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   _("Error closing (unlinked) lock file “%s”: %s"),
                   lock,
                   xstrerror (errsv));
      goto out;
    }
  if (g_unlink (lock) != 0)
    {
      int errsv = errno;
      g_set_error (error,
                   G_IO_ERROR,
                   g_io_error_from_errno (errsv),
                   _("Error unlinking lock file “%s”: %s"),
                   lock,
                   xstrerror (errsv));
      goto out;
    }

  ret = TRUE;

 out:
  g_free (lock);
  return ret;
}


/* adds an entry to the keyring, taking care of locking and deleting stale/future entries */
static xboolean_t
keyring_generate_entry (const xchar_t  *cookie_context,
                        xint_t         *out_id,
                        xchar_t       **out_cookie,
                        xerror_t      **error)
{
  xboolean_t ret;
  xchar_t *keyring_dir;
  xchar_t *path;
  xchar_t *contents;
  xerror_t *local_error;
  xchar_t **lines;
  xint_t max_line_id;
  xstring_t *new_contents;
  sint64_t now;
  xboolean_t have_id;
  xint_t use_id;
  xchar_t *use_cookie;
  xboolean_t changed_file;
  xint_t lock_fd;

  xreturn_val_if_fail (cookie_context != NULL, FALSE);
  xreturn_val_if_fail (out_id != NULL, FALSE);
  xreturn_val_if_fail (out_cookie != NULL, FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;
  path = NULL;
  contents = NULL;
  lines = NULL;
  new_contents = NULL;
  have_id = FALSE;
  use_id = 0;
  use_cookie = NULL;
  lock_fd = -1;

  keyring_dir = ensure_keyring_directory (error);
  if (keyring_dir == NULL)
    goto out;

  path = g_build_filename (keyring_dir, cookie_context, NULL);

  lock_fd = keyring_acquire_lock (path, error);
  if (lock_fd == -1)
    goto out;

  local_error = NULL;
  contents = NULL;
  if (!xfile_get_contents (path,
                            &contents,
                            NULL,
                            &local_error))
    {
      if (local_error->domain == XFILE_ERROR && local_error->code == XFILE_ERROR_NOENT)
        {
          /* file doesn't have to exist */
          xerror_free (local_error);
        }
      else
        {
          g_propagate_prefixed_error (error,
                                      local_error,
                                      _("Error opening keyring “%s” for writing: "),
                                      path);
          goto out;
        }
    }

  new_contents = xstring_new (NULL);
  now = g_get_real_time () / G_USEC_PER_SEC;
  changed_file = FALSE;

  max_line_id = 0;
  if (contents != NULL)
    {
      xuint_t n;
      lines = xstrsplit (contents, "\n", 0);
      for (n = 0; lines[n] != NULL; n++)
        {
          const xchar_t *line = lines[n];
          xchar_t **tokens;
          xchar_t *endp;
          xint_t line_id;
          sint64_t line_when;
          xboolean_t keep_entry;

          if (line[0] == '\0')
            continue;

          tokens = xstrsplit (line, " ", 0);
          if (xstrv_length (tokens) != 3)
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           _("Line %d of the keyring at “%s” with content “%s” is malformed"),
                           n + 1,
                           path,
                           line);
              xstrfreev (tokens);
              goto out;
            }

          line_id = g_ascii_strtoll (tokens[0], &endp, 10);
          if (*endp != '\0')
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           _("First token of line %d of the keyring at “%s” with content “%s” is malformed"),
                           n + 1,
                           path,
                           line);
              xstrfreev (tokens);
              goto out;
            }

          line_when = g_ascii_strtoll (tokens[1], &endp, 10);
          if (*endp != '\0')
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           _("Second token of line %d of the keyring at “%s” with content “%s” is malformed"),
                           n + 1,
                           path,
                           line);
              xstrfreev (tokens);
              goto out;
            }


          /* D-Bus spec says:
           *
           *  Once the lockfile has been created, the server loads the
           *  cookie file. It should then delete any cookies that are
           *  old (the timeout can be fairly short), or more than a
           *  reasonable time in the future (so that cookies never
           *  accidentally become permanent, if the clock was set far
           *  into the future at some point). If no recent keys remain,
           *  the server may generate a new key.
           *
           */
          keep_entry = TRUE;
          if (line_when > now)
            {
              /* Oddball case: entry is more recent than our current wall-clock time..
               * This is OK, it means that another server on another machine but with
               * same $HOME wrote the entry. */
              if (line_when - now > MAX_TIME_TRAVEL_SECONDS)
                {
                  keep_entry = FALSE;
                  _log ("Deleted SHA1 cookie from %" G_GUINT64_FORMAT " seconds in the future", line_when - now);
                }
            }
          else
            {
              /* Discard entry if it's too old. */
              if (now - line_when > EXPIRE_KEYS_TIMEOUT_SECONDS)
                {
                  keep_entry = FALSE;
                }
            }

          if (!keep_entry)
            {
              changed_file = FALSE;
            }
          else
            {
              xstring_append_printf (new_contents,
                                      "%d %" G_GUINT64_FORMAT " %s\n",
                                      line_id,
                                      line_when,
                                      tokens[2]);
              max_line_id = MAX (line_id, max_line_id);
              /* Only reuse entry if not older than 5 minutes.
               *
               * (We need a bit of grace time compared to 7 minutes above.. otherwise
               * there's a race where we reuse the 6min59.9 secs old entry and a
               * split-second later another server purges the now 7 minute old entry.)
               */
              if (now - line_when < NEW_KEY_TIMEOUT_SECONDS)
                {
                  if (!have_id)
                    {
                      use_id = line_id;
                      use_cookie = tokens[2]; /* steal memory */
                      tokens[2] = NULL;
                      have_id = TRUE;
                    }
                }
            }
          xstrfreev (tokens);
        }
    } /* for each line */

  ret = TRUE;

  if (have_id)
    {
      *out_id = use_id;
      *out_cookie = use_cookie;
      use_cookie = NULL;
    }
  else
    {
      xchar_t *raw_cookie;
      *out_id = max_line_id + 1;
      raw_cookie = random_blob (32);
      *out_cookie = _g_dbus_hexencode (raw_cookie, 32);
      g_free (raw_cookie);

      xstring_append_printf (new_contents,
                              "%d %" G_GINT64_FORMAT " %s\n",
                              *out_id,
                              g_get_real_time () / G_USEC_PER_SEC,
                              *out_cookie);
      changed_file = TRUE;
    }

  /* and now actually write the cookie file if there are changes (this is atomic) */
  if (changed_file)
    {
      if (!xfile_set_contents_full (path,
                                     new_contents->str,
                                     -1,
                                     XFILE_SET_CONTENTS_CONSISTENT,
                                     0600,
                                     error))
        {
          *out_id = 0;
          g_free (*out_cookie);
          *out_cookie = 0;
          ret = FALSE;
          goto out;
        }
    }

 out:

  if (lock_fd != -1)
    {
      xerror_t *local_error;
      local_error = NULL;
      if (!keyring_release_lock (path, lock_fd, &local_error))
        {
          if (error != NULL)
            {
              if (*error == NULL)
                {
                  *error = local_error;
                }
              else
                {
                  g_prefix_error (error,
                                  _("(Additionally, releasing the lock for “%s” also failed: %s) "),
                                  path,
                                  local_error->message);
                  xerror_free (local_error);
                }
            }
          else
            {
              xerror_free (local_error);
            }
        }
    }

  g_free (keyring_dir);
  g_free (path);
  xstrfreev (lines);
  g_free (contents);
  if (new_contents != NULL)
    xstring_free (new_contents, TRUE);
  g_free (use_cookie);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t *
generate_sha1 (const xchar_t *server_challenge,
               const xchar_t *client_challenge,
               const xchar_t *cookie)
{
  xstring_t *str;
  xchar_t *sha1;

  str = xstring_new (server_challenge);
  xstring_append_c (str, ':');
  xstring_append (str, client_challenge);
  xstring_append_c (str, ':');
  xstring_append (str, cookie);
  sha1 = g_compute_checksum_for_string (G_CHECKSUM_SHA1, str->str, -1);
  xstring_free (str, TRUE);

  return sha1;
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_auth_mechanism_state_t

mechanism_server_get_state (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, XDBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static void
mechanism_server_initiate (xdbus_auth_mechanism_t   *mechanism,
                           const xchar_t          *initial_response,
                           xsize_t                 initial_response_len)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism));
  g_return_if_fail (!m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = TRUE;
  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;

  if (initial_response != NULL && initial_response_len > 0)
    {
#ifdef G_OS_UNIX
      sint64_t uid;
      xchar_t *endp;

      uid = g_ascii_strtoll (initial_response, &endp, 10);
      if (*endp == '\0')
        {
          if (uid == getuid ())
            {
              m->priv->state = XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND;
            }
        }
#elif defined(G_OS_WIN32)
      xchar_t *sid;

      sid = _g_win32_current_process_sid_string (NULL);

      if (xstrcmp0 (initial_response, sid) == 0)
        m->priv->state = XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND;

      g_free (sid);
#else
#error Please implement for your OS
#endif
    }
}

static void
mechanism_server_data_receive (xdbus_auth_mechanism_t   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);
  xchar_t **tokens;
  const xchar_t *client_challenge;
  const xchar_t *alleged_sha1;
  xchar_t *sha1;

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);
  g_return_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  tokens = NULL;
  sha1 = NULL;

  tokens = xstrsplit (data, " ", 0);
  if (xstrv_length (tokens) != 2)
    {
      g_free (m->priv->reject_reason);
      m->priv->reject_reason = xstrdup_printf ("Malformed data '%s'", data);
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
      goto out;
    }

  client_challenge = tokens[0];
  alleged_sha1 = tokens[1];

  sha1 = generate_sha1 (m->priv->server_challenge, client_challenge, m->priv->cookie);

  if (xstrcmp0 (sha1, alleged_sha1) == 0)
    {
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;
    }
  else
    {
      g_free (m->priv->reject_reason);
      m->priv->reject_reason = xstrdup_printf ("SHA-1 mismatch");
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
    }

 out:
  xstrfreev (tokens);
  g_free (sha1);
}

static xchar_t *
mechanism_server_data_send (xdbus_auth_mechanism_t   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);
  xchar_t *s;
  xint_t cookie_id;
  const xchar_t *cookie_context;
  xerror_t *error;

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  s = NULL;
  *out_data_len = 0;

  /* TODO: use xdbus_auth_observer_t here to get the cookie context to use? */
  cookie_context = "org_gtk_gdbus_general";

  cookie_id = -1;
  error = NULL;
  if (!keyring_generate_entry (cookie_context,
                               &cookie_id,
                               &m->priv->cookie,
                               &error))
    {
      g_free (m->priv->reject_reason);
      m->priv->reject_reason = xstrdup_printf ("Error adding entry to keyring: %s", error->message);
      xerror_free (error);
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
      goto out;
    }

  m->priv->server_challenge = random_ascii_string (16);
  s = xstrdup_printf ("%s %d %s",
                       cookie_context,
                       cookie_id,
                       m->priv->server_challenge);
  *out_data_len = strlen (s);

  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA;

 out:
  return s;
}

static xchar_t *
mechanism_server_get_reject_reason (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_server && !m->priv->is_client, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_REJECTED, NULL);

  return xstrdup (m->priv->reject_reason);
}

static void
mechanism_server_shutdown (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism));
  g_return_if_fail (m->priv->is_server && !m->priv->is_client);

  m->priv->is_server = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */

static xdbus_auth_mechanism_state_t

mechanism_client_get_state (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), XDBUS_AUTH_MECHANISM_STATE_INVALID);
  xreturn_val_if_fail (m->priv->is_client && !m->priv->is_server, XDBUS_AUTH_MECHANISM_STATE_INVALID);

  return m->priv->state;
}

static xchar_t *
mechanism_client_initiate (xdbus_auth_mechanism_t   *mechanism,
                           xsize_t                *out_initial_response_len)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);
  xchar_t *initial_response;

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), NULL);
  xreturn_val_if_fail (!m->priv->is_server && !m->priv->is_client, NULL);

  m->priv->is_client = TRUE;

  *out_initial_response_len = 0;

#ifdef G_OS_UNIX
  initial_response = xstrdup_printf ("%" G_GINT64_FORMAT, (sint64_t) getuid ());
#elif defined (G_OS_WIN32)
  initial_response = _g_win32_current_process_sid_string (NULL);
#else
#error Please implement for your OS
#endif
  if (initial_response)
    {
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA;
      *out_initial_response_len = strlen (initial_response);
    }
  else
    {
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
    }

  return initial_response;
}

static void
mechanism_client_data_receive (xdbus_auth_mechanism_t   *mechanism,
                               const xchar_t          *data,
                               xsize_t                 data_len)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);
  xchar_t **tokens;
  const xchar_t *cookie_context;
  xuint_t cookie_id;
  const xchar_t *server_challenge;
  xchar_t *client_challenge;
  xchar_t *endp;
  xchar_t *cookie;
  xerror_t *error;
  xchar_t *sha1;

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);
  g_return_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA);

  tokens = NULL;
  cookie = NULL;
  client_challenge = NULL;

  tokens = xstrsplit (data, " ", 0);
  if (xstrv_length (tokens) != 3)
    {
      g_free (m->priv->reject_reason);
      m->priv->reject_reason = xstrdup_printf ("Malformed data '%s'", data);
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
      goto out;
    }

  cookie_context = tokens[0];
  cookie_id = g_ascii_strtoll (tokens[1], &endp, 10);
  if (*endp != '\0')
    {
      g_free (m->priv->reject_reason);
      m->priv->reject_reason = xstrdup_printf ("Malformed cookie_id '%s'", tokens[1]);
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
      goto out;
    }
  server_challenge = tokens[2];

  error = NULL;
  cookie = keyring_lookup_entry (cookie_context, cookie_id, &error);
  if (cookie == NULL)
    {
      g_free (m->priv->reject_reason);
      m->priv->reject_reason = xstrdup_printf ("Problems looking up entry in keyring: %s", error->message);
      xerror_free (error);
      m->priv->state = XDBUS_AUTH_MECHANISM_STATE_REJECTED;
      goto out;
    }

  client_challenge = random_ascii_string (16);
  sha1 = generate_sha1 (server_challenge, client_challenge, cookie);
  m->priv->to_send = xstrdup_printf ("%s %s", client_challenge, sha1);
  g_free (sha1);
  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND;

 out:
  xstrfreev (tokens);
  g_free (cookie);
  g_free (client_challenge);
}

static xchar_t *
mechanism_client_data_send (xdbus_auth_mechanism_t   *mechanism,
                            xsize_t                *out_data_len)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  xreturn_val_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism), NULL);
  xreturn_val_if_fail (m->priv->is_client && !m->priv->is_server, NULL);
  xreturn_val_if_fail (m->priv->state == XDBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND, NULL);

  xassert (m->priv->to_send != NULL);

  m->priv->state = XDBUS_AUTH_MECHANISM_STATE_ACCEPTED;

  *out_data_len = strlen (m->priv->to_send);
  return xstrdup (m->priv->to_send);
}

static void
mechanism_client_shutdown (xdbus_auth_mechanism_t   *mechanism)
{
  GDBusAuthMechanismSha1 *m = XDBUS_AUTH_MECHANISM_SHA1 (mechanism);

  g_return_if_fail (X_IS_DBUS_AUTH_MECHANISM_SHA1 (mechanism));
  g_return_if_fail (m->priv->is_client && !m->priv->is_server);

  m->priv->is_client = FALSE;
}

/* ---------------------------------------------------------------------------------------------------- */
