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

#include <stdlib.h>
#include <string.h>

#include "gdbuserror.h"
#include "gioenums.h"
#include "gioenumtypes.h"
#include "gioerror.h"
#include "gdbusprivate.h"

#include "glibintl.h"

/**
 * SECTION:gdbuserror
 * @title: GDBusError
 * @short_description: Mapping D-Bus errors to and from xerror_t
 * @include: gio/gio.h
 *
 * All facilities that return errors from remote methods (such as
 * xdbus_connection_call_sync()) use #xerror_t to represent both D-Bus
 * errors (e.g. errors returned from the other peer) and locally
 * in-process generated errors.
 *
 * To check if a returned #xerror_t is an error from a remote peer, use
 * g_dbus_error_is_remote_error(). To get the actual D-Bus error name,
 * use g_dbus_error_get_remote_error(). Before presenting an error,
 * always use g_dbus_error_strip_remote_error().
 *
 * In addition, facilities used to return errors to a remote peer also
 * use #xerror_t. See xdbus_method_invocation_return_error() for
 * discussion about how the D-Bus error name is set.
 *
 * Applications can associate a #xerror_t error domain with a set of D-Bus errors in order to
 * automatically map from D-Bus errors to #xerror_t and back. This
 * is typically done in the function returning the #xquark for the
 * error domain:
 * |[<!-- language="C" -->
 * // foo-bar-error.h:
 *
 * #define FOO_BAR_ERROR (foo_bar_error_quark ())
 * xquark foo_bar_error_quark (void);
 *
 * typedef enum
 * {
 *   FOO_BAR_ERROR_FAILED,
 *   FOO_BAR_ERROR_ANOTHER_ERROR,
 *   FOO_BAR_ERROR_SOME_THIRD_ERROR,
 *   FOO_BAR_N_ERRORS / *< skip >* /
 * } FooBarError;
 *
 * // foo-bar-error.c:
 *
 * static const xdbus_error_entry_t foo_bar_error_entries[] =
 * {
 *   {FOO_BAR_ERROR_FAILED,           "org.project.foo_t.Bar.Error.Failed"},
 *   {FOO_BAR_ERROR_ANOTHER_ERROR,    "org.project.foo_t.Bar.Error.AnotherError"},
 *   {FOO_BAR_ERROR_SOME_THIRD_ERROR, "org.project.foo_t.Bar.Error.SomeThirdError"},
 * };
 *
 * // Ensure that every error code has an associated D-Bus error name
 * G_STATIC_ASSERT (G_N_ELEMENTS (foo_bar_error_entries) == FOO_BAR_N_ERRORS);
 *
 * xquark
 * foo_bar_error_quark (void)
 * {
 *   static xsize_t quark = 0;
 *   g_dbus_error_register_error_domain ("foo-bar-error-quark",
 *                                       &quark,
 *                                       foo_bar_error_entries,
 *                                       G_N_ELEMENTS (foo_bar_error_entries));
 *   return (xquark) quark;
 * }
 * ]|
 * With this setup, a D-Bus peer can transparently pass e.g. %FOO_BAR_ERROR_ANOTHER_ERROR and
 * other peers will see the D-Bus error name org.project.foo_t.Bar.Error.AnotherError.
 *
 * If the other peer is using GDBus, and has registered the association with
 * g_dbus_error_register_error_domain() in advance (e.g. by invoking the %FOO_BAR_ERROR quark
 * generation itself in the previous example) the peer will see also %FOO_BAR_ERROR_ANOTHER_ERROR instead
 * of %G_IO_ERROR_DBUS_ERROR. Note that GDBus clients can still recover
 * org.project.foo_t.Bar.Error.AnotherError using g_dbus_error_get_remote_error().
 *
 * Note that the %G_DBUS_ERROR error domain is intended only
 * for returning errors from a remote message bus process. Errors
 * generated locally in-process by e.g. #xdbus_connection_t should use the
 * %G_IO_ERROR domain.
 */

static const xdbus_error_entry_t g_dbus_error_entries[] =
{
  {G_DBUS_ERROR_FAILED,                           "org.freedesktop.DBus.Error.Failed"},
  {G_DBUS_ERROR_NO_MEMORY,                        "org.freedesktop.DBus.Error.NoMemory"},
  {G_DBUS_ERROR_SERVICE_UNKNOWN,                  "org.freedesktop.DBus.Error.ServiceUnknown"},
  {G_DBUS_ERROR_NAME_HAS_NO_OWNER,                "org.freedesktop.DBus.Error.NameHasNoOwner"},
  {G_DBUS_ERROR_NO_REPLY,                         "org.freedesktop.DBus.Error.NoReply"},
  {G_DBUS_ERROR_IO_ERROR,                         "org.freedesktop.DBus.Error.IOError"},
  {G_DBUS_ERROR_BAD_ADDRESS,                      "org.freedesktop.DBus.Error.BadAddress"},
  {G_DBUS_ERROR_NOT_SUPPORTED,                    "org.freedesktop.DBus.Error.NotSupported"},
  {G_DBUS_ERROR_LIMITS_EXCEEDED,                  "org.freedesktop.DBus.Error.LimitsExceeded"},
  {G_DBUS_ERROR_ACCESS_DENIED,                    "org.freedesktop.DBus.Error.AccessDenied"},
  {G_DBUS_ERROR_AUTH_FAILED,                      "org.freedesktop.DBus.Error.AuthFailed"},
  {G_DBUS_ERROR_NO_SERVER,                        "org.freedesktop.DBus.Error.NoServer"},
  {G_DBUS_ERROR_TIMEOUT,                          "org.freedesktop.DBus.Error.Timeout"},
  {G_DBUS_ERROR_NO_NETWORK,                       "org.freedesktop.DBus.Error.NoNetwork"},
  {G_DBUS_ERROR_ADDRESS_IN_USE,                   "org.freedesktop.DBus.Error.AddressInUse"},
  {G_DBUS_ERROR_DISCONNECTED,                     "org.freedesktop.DBus.Error.Disconnected"},
  {G_DBUS_ERROR_INVALID_ARGS,                     "org.freedesktop.DBus.Error.InvalidArgs"},
  {G_DBUS_ERROR_FILE_NOT_FOUND,                   "org.freedesktop.DBus.Error.FileNotFound"},
  {G_DBUS_ERROR_FILE_EXISTS,                      "org.freedesktop.DBus.Error.FileExists"},
  {G_DBUS_ERROR_UNKNOWN_METHOD,                   "org.freedesktop.DBus.Error.UnknownMethod"},
  {G_DBUS_ERROR_TIMED_OUT,                        "org.freedesktop.DBus.Error.TimedOut"},
  {G_DBUS_ERROR_MATCH_RULE_NOT_FOUND,             "org.freedesktop.DBus.Error.MatchRuleNotFound"},
  {G_DBUS_ERROR_MATCH_RULE_INVALID,               "org.freedesktop.DBus.Error.MatchRuleInvalid"},
  {G_DBUS_ERROR_SPAWN_EXEC_FAILED,                "org.freedesktop.DBus.Error.Spawn.ExecFailed"},
  {G_DBUS_ERROR_SPAWN_FORK_FAILED,                "org.freedesktop.DBus.Error.Spawn.ForkFailed"},
  {G_DBUS_ERROR_SPAWN_CHILD_EXITED,               "org.freedesktop.DBus.Error.Spawn.ChildExited"},
  {G_DBUS_ERROR_SPAWN_CHILD_SIGNALED,             "org.freedesktop.DBus.Error.Spawn.ChildSignaled"},
  {G_DBUS_ERROR_SPAWN_FAILED,                     "org.freedesktop.DBus.Error.Spawn.Failed"},
  {G_DBUS_ERROR_SPAWN_SETUP_FAILED,               "org.freedesktop.DBus.Error.Spawn.FailedToSetup"},
  {G_DBUS_ERROR_SPAWN_CONFIG_INVALID,             "org.freedesktop.DBus.Error.Spawn.ConfigInvalid"},
  {G_DBUS_ERROR_SPAWN_SERVICE_INVALID,            "org.freedesktop.DBus.Error.Spawn.ServiceNotValid"},
  {G_DBUS_ERROR_SPAWN_SERVICE_NOT_FOUND,          "org.freedesktop.DBus.Error.Spawn.ServiceNotFound"},
  {G_DBUS_ERROR_SPAWN_PERMISSIONS_INVALID,        "org.freedesktop.DBus.Error.Spawn.PermissionsInvalid"},
  {G_DBUS_ERROR_SPAWN_FILE_INVALID,               "org.freedesktop.DBus.Error.Spawn.FileInvalid"},
  {G_DBUS_ERROR_SPAWN_NO_MEMORY,                  "org.freedesktop.DBus.Error.Spawn.NoMemory"},
  {G_DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN,          "org.freedesktop.DBus.Error.UnixProcessIdUnknown"},
  {G_DBUS_ERROR_INVALID_SIGNATURE,                "org.freedesktop.DBus.Error.InvalidSignature"},
  {G_DBUS_ERROR_INVALID_FILE_CONTENT,             "org.freedesktop.DBus.Error.InvalidFileContent"},
  {G_DBUS_ERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN, "org.freedesktop.DBus.Error.SELinuxSecurityContextUnknown"},
  {G_DBUS_ERROR_ADT_AUDIT_DATA_UNKNOWN,           "org.freedesktop.DBus.Error.AdtAuditDataUnknown"},
  {G_DBUS_ERROR_OBJECT_PATH_IN_USE,               "org.freedesktop.DBus.Error.ObjectPathInUse"},
  {G_DBUS_ERROR_UNKNOWN_OBJECT,                   "org.freedesktop.DBus.Error.UnknownObject"},
  {G_DBUS_ERROR_UNKNOWN_INTERFACE,                "org.freedesktop.DBus.Error.UnknownInterface"},
  {G_DBUS_ERROR_UNKNOWN_PROPERTY,                 "org.freedesktop.DBus.Error.UnknownProperty"},
  {G_DBUS_ERROR_PROPERTY_READ_ONLY,               "org.freedesktop.DBus.Error.PropertyReadOnly"},
};

xquark
g_dbus_error_quark (void)
{
  G_STATIC_ASSERT (G_N_ELEMENTS (g_dbus_error_entries) - 1 == G_DBUS_ERROR_PROPERTY_READ_ONLY);
  static xsize_t quark = 0;
  g_dbus_error_register_error_domain ("g-dbus-error-quark",
                                      &quark,
                                      g_dbus_error_entries,
                                      G_N_ELEMENTS (g_dbus_error_entries));
  return (xquark) quark;
}

/**
 * g_dbus_error_register_error_domain:
 * @error_domain_quark_name: The error domain name.
 * @quark_volatile: A pointer where to store the #xquark.
 * @entries: (array length=num_entries): A pointer to @num_entries #xdbus_error_entry_t struct items.
 * @num_entries: Number of items to register.
 *
 * Helper function for associating a #xerror_t error domain with D-Bus error names.
 *
 * While @quark_volatile has a `volatile` qualifier, this is a historical
 * artifact and the argument passed to it should not be `volatile`.
 *
 * Since: 2.26
 */
void
g_dbus_error_register_error_domain (const xchar_t           *error_domain_quark_name,
                                    volatile xsize_t        *quark_volatile,
                                    const xdbus_error_entry_t *entries,
                                    xuint_t                  num_entries)
{
  xsize_t *quark;

  g_return_if_fail (error_domain_quark_name != NULL);
  g_return_if_fail (quark_volatile != NULL);
  g_return_if_fail (entries != NULL);
  g_return_if_fail (num_entries > 0);

  /* Drop the volatile qualifier, which should never have been on the argument
   * in the first place. */
  quark = (xsize_t *) quark_volatile;

  if (g_once_init_enter (quark))
    {
      xuint_t n;
      xquark new_quark;

      new_quark = g_quark_from_static_string (error_domain_quark_name);

      for (n = 0; n < num_entries; n++)
        {
          g_warn_if_fail (g_dbus_error_register_error (new_quark,
                                                       entries[n].error_code,
                                                       entries[n].dbus_error_name));
        }
      g_once_init_leave (quark, new_quark);
    }
}

static xboolean_t
_g_dbus_error_decode_gerror (const xchar_t *dbus_name,
                             xquark      *out_error_domain,
                             xint_t        *out_error_code)
{
  xboolean_t ret;
  xuint_t n;
  xstring_t *s;
  xchar_t *domain_quark_string;

  ret = FALSE;
  s = NULL;

  if (xstr_has_prefix (dbus_name, "org.gtk.GDBus.UnmappedGError.Quark._"))
    {
      s = xstring_new (NULL);

      for (n = sizeof "org.gtk.GDBus.UnmappedGError.Quark._" - 1;
           dbus_name[n] != '.' && dbus_name[n] != '\0';
           n++)
        {
          if (g_ascii_isalnum (dbus_name[n]))
            {
              xstring_append_c (s, dbus_name[n]);
            }
          else if (dbus_name[n] == '_')
            {
              xuint_t nibble_top;
              xuint_t nibble_bottom;

              n++;

              nibble_top = dbus_name[n];
              if (nibble_top >= '0' && nibble_top <= '9')
                nibble_top -= '0';
              else if (nibble_top >= 'a' && nibble_top <= 'f')
                nibble_top -= ('a' - 10);
              else
                goto not_mapped;

              n++;

              nibble_bottom = dbus_name[n];
              if (nibble_bottom >= '0' && nibble_bottom <= '9')
                nibble_bottom -= '0';
              else if (nibble_bottom >= 'a' && nibble_bottom <= 'f')
                nibble_bottom -= ('a' - 10);
              else
                goto not_mapped;

              xstring_append_c (s, (nibble_top<<4) | nibble_bottom);
            }
          else
            {
              goto not_mapped;
            }
        }

      if (!xstr_has_prefix (dbus_name + n, ".Code"))
        goto not_mapped;

      domain_quark_string = xstring_free (s, FALSE);
      s = NULL;

      if (out_error_domain != NULL)
        *out_error_domain = g_quark_from_string (domain_quark_string);
      g_free (domain_quark_string);

      if (out_error_code != NULL)
        *out_error_code = atoi (dbus_name + n + sizeof ".Code" - 1);

      ret = TRUE;
    }

 not_mapped:

  if (s != NULL)
    xstring_free (s, TRUE);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  xquark error_domain;
  xint_t   error_code;
} QuarkCodePair;

static xuint_t
quark_code_pair_hash_func (const QuarkCodePair *pair)
{
  xint_t val;
  val = pair->error_domain + pair->error_code;
  return g_int_hash (&val);
}

static xboolean_t
quark_code_pair_equal_func (const QuarkCodePair *a,
                            const QuarkCodePair *b)
{
  return (a->error_domain == b->error_domain) && (a->error_code == b->error_code);
}

typedef struct
{
  QuarkCodePair pair;
  xchar_t *dbus_error_name;
} RegisteredError;

static void
registered_error_free (RegisteredError *re)
{
  g_free (re->dbus_error_name);
  g_free (re);
}

G_LOCK_DEFINE_STATIC (error_lock);

/* maps from QuarkCodePair* -> RegisteredError* */
static xhashtable_t *quark_code_pair_to_re = NULL;

/* maps from xchar_t* -> RegisteredError* */
static xhashtable_t *dbus_error_name_to_re = NULL;

/**
 * g_dbus_error_register_error:
 * @error_domain: A #xquark for an error domain.
 * @error_code: An error code.
 * @dbus_error_name: A D-Bus error name.
 *
 * Creates an association to map between @dbus_error_name and
 * #GErrors specified by @error_domain and @error_code.
 *
 * This is typically done in the routine that returns the #xquark for
 * an error domain.
 *
 * Returns: %TRUE if the association was created, %FALSE if it already
 * exists.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_error_register_error (xquark       error_domain,
                             xint_t         error_code,
                             const xchar_t *dbus_error_name)
{
  xboolean_t ret;
  QuarkCodePair pair;
  RegisteredError *re;

  g_return_val_if_fail (dbus_error_name != NULL, FALSE);

  ret = FALSE;

  G_LOCK (error_lock);

  if (quark_code_pair_to_re == NULL)
    {
      g_assert (dbus_error_name_to_re == NULL); /* check invariant */
      quark_code_pair_to_re = xhash_table_new ((GHashFunc) quark_code_pair_hash_func,
                                                (GEqualFunc) quark_code_pair_equal_func);
      dbus_error_name_to_re = xhash_table_new_full (xstr_hash,
                                                     xstr_equal,
                                                     NULL,
                                                     (xdestroy_notify_t) registered_error_free);
    }

  if (xhash_table_lookup (dbus_error_name_to_re, dbus_error_name) != NULL)
    goto out;

  pair.error_domain = error_domain;
  pair.error_code = error_code;

  if (xhash_table_lookup (quark_code_pair_to_re, &pair) != NULL)
    goto out;

  re = g_new0 (RegisteredError, 1);
  re->pair = pair;
  re->dbus_error_name = xstrdup (dbus_error_name);

  xhash_table_insert (quark_code_pair_to_re, &(re->pair), re);
  xhash_table_insert (dbus_error_name_to_re, re->dbus_error_name, re);

  ret = TRUE;

 out:
  G_UNLOCK (error_lock);
  return ret;
}

/**
 * g_dbus_error_unregister_error:
 * @error_domain: A #xquark for an error domain.
 * @error_code: An error code.
 * @dbus_error_name: A D-Bus error name.
 *
 * Destroys an association previously set up with g_dbus_error_register_error().
 *
 * Returns: %TRUE if the association was destroyed, %FALSE if it wasn't found.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_error_unregister_error (xquark       error_domain,
                               xint_t         error_code,
                               const xchar_t *dbus_error_name)
{
  xboolean_t ret;
  RegisteredError *re;
  xuint_t hash_size;

  g_return_val_if_fail (dbus_error_name != NULL, FALSE);

  ret = FALSE;

  G_LOCK (error_lock);

  if (dbus_error_name_to_re == NULL)
    {
      g_assert (quark_code_pair_to_re == NULL); /* check invariant */
      goto out;
    }

  re = xhash_table_lookup (dbus_error_name_to_re, dbus_error_name);
  if (re == NULL)
    {
      QuarkCodePair pair;
      pair.error_domain = error_domain;
      pair.error_code = error_code;
      g_warn_if_fail (xhash_table_lookup (quark_code_pair_to_re, &pair) == NULL); /* check invariant */
      goto out;
    }

  ret = TRUE;

  g_warn_if_fail (xhash_table_lookup (quark_code_pair_to_re, &(re->pair)) == re); /* check invariant */

  g_warn_if_fail (xhash_table_remove (quark_code_pair_to_re, &(re->pair)));
  g_warn_if_fail (xhash_table_remove (dbus_error_name_to_re, re->dbus_error_name));

  /* destroy hashes if empty */
  hash_size = xhash_table_size (dbus_error_name_to_re);
  if (hash_size == 0)
    {
      g_warn_if_fail (xhash_table_size (quark_code_pair_to_re) == 0); /* check invariant */

      xhash_table_unref (dbus_error_name_to_re);
      dbus_error_name_to_re = NULL;
      xhash_table_unref (quark_code_pair_to_re);
      quark_code_pair_to_re = NULL;
    }
  else
    {
      g_warn_if_fail (xhash_table_size (quark_code_pair_to_re) == hash_size); /* check invariant */
    }

 out:
  G_UNLOCK (error_lock);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_error_is_remote_error:
 * @error: A #xerror_t.
 *
 * Checks if @error represents an error received via D-Bus from a remote peer. If so,
 * use g_dbus_error_get_remote_error() to get the name of the error.
 *
 * Returns: %TRUE if @error represents an error from a remote peer,
 * %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_error_is_remote_error (const xerror_t *error)
{
  g_return_val_if_fail (error != NULL, FALSE);
  return xstr_has_prefix (error->message, "GDBus.Error:");
}


/**
 * g_dbus_error_get_remote_error:
 * @error: a #xerror_t
 *
 * Gets the D-Bus error name used for @error, if any.
 *
 * This function is guaranteed to return a D-Bus error name for all
 * #GErrors returned from functions handling remote method calls
 * (e.g. xdbus_connection_call_finish()) unless
 * g_dbus_error_strip_remote_error() has been used on @error.
 *
 * Returns: (nullable) (transfer full): an allocated string or %NULL if the
 *     D-Bus error name could not be found. Free with g_free().
 *
 * Since: 2.26
 */
xchar_t *
g_dbus_error_get_remote_error (const xerror_t *error)
{
  RegisteredError *re;
  xchar_t *ret;

  g_return_val_if_fail (error != NULL, NULL);

  /* Ensure that e.g. G_DBUS_ERROR is registered using g_dbus_error_register_error() */
  _g_dbus_initialize ();

  ret = NULL;

  G_LOCK (error_lock);

  re = NULL;
  if (quark_code_pair_to_re != NULL)
    {
      QuarkCodePair pair;
      pair.error_domain = error->domain;
      pair.error_code = error->code;
      g_assert (dbus_error_name_to_re != NULL); /* check invariant */
      re = xhash_table_lookup (quark_code_pair_to_re, &pair);
    }

  if (re != NULL)
    {
      ret = xstrdup (re->dbus_error_name);
    }
  else
    {
      if (xstr_has_prefix (error->message, "GDBus.Error:"))
        {
          const xchar_t *begin;
          const xchar_t *end;
          begin = error->message + sizeof ("GDBus.Error:") -1;
          end = strstr (begin, ":");
          if (end != NULL && end[1] == ' ')
            {
              ret = xstrndup (begin, end - begin);
            }
        }
    }

  G_UNLOCK (error_lock);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_error_new_for_dbus_error:
 * @dbus_error_name: D-Bus error name.
 * @dbus_error_message: D-Bus error message.
 *
 * Creates a #xerror_t based on the contents of @dbus_error_name and
 * @dbus_error_message.
 *
 * Errors registered with g_dbus_error_register_error() will be looked
 * up using @dbus_error_name and if a match is found, the error domain
 * and code is used. Applications can use g_dbus_error_get_remote_error()
 * to recover @dbus_error_name.
 *
 * If a match against a registered error is not found and the D-Bus
 * error name is in a form as returned by g_dbus_error_encode_gerror()
 * the error domain and code encoded in the name is used to
 * create the #xerror_t. Also, @dbus_error_name is added to the error message
 * such that it can be recovered with g_dbus_error_get_remote_error().
 *
 * Otherwise, a #xerror_t with the error code %G_IO_ERROR_DBUS_ERROR
 * in the %G_IO_ERROR error domain is returned. Also, @dbus_error_name is
 * added to the error message such that it can be recovered with
 * g_dbus_error_get_remote_error().
 *
 * In all three cases, @dbus_error_name can always be recovered from the
 * returned #xerror_t using the g_dbus_error_get_remote_error() function
 * (unless g_dbus_error_strip_remote_error() hasn't been used on the returned error).
 *
 * This function is typically only used in object mappings to prepare
 * #xerror_t instances for applications. Regular applications should not use
 * it.
 *
 * Returns: (transfer full): An allocated #xerror_t. Free with xerror_free().
 *
 * Since: 2.26
 */
xerror_t *
g_dbus_error_new_for_dbus_error (const xchar_t *dbus_error_name,
                                 const xchar_t *dbus_error_message)
{
  xerror_t *error;
  RegisteredError *re;

  g_return_val_if_fail (dbus_error_name != NULL, NULL);
  g_return_val_if_fail (dbus_error_message != NULL, NULL);

  /* Ensure that e.g. G_DBUS_ERROR is registered using g_dbus_error_register_error() */
  _g_dbus_initialize ();

  G_LOCK (error_lock);

  re = NULL;
  if (dbus_error_name_to_re != NULL)
    {
      g_assert (quark_code_pair_to_re != NULL); /* check invariant */
      re = xhash_table_lookup (dbus_error_name_to_re, dbus_error_name);
    }

  if (re != NULL)
    {
      error = xerror_new (re->pair.error_domain,
                           re->pair.error_code,
                           "GDBus.Error:%s: %s",
                           dbus_error_name,
                           dbus_error_message);
    }
  else
    {
      xquark error_domain = 0;
      xint_t error_code = 0;

      if (_g_dbus_error_decode_gerror (dbus_error_name,
                                       &error_domain,
                                       &error_code))
        {
          error = xerror_new (error_domain,
                               error_code,
                               "GDBus.Error:%s: %s",
                               dbus_error_name,
                               dbus_error_message);
        }
      else
        {
          error = xerror_new (G_IO_ERROR,
                               G_IO_ERROR_DBUS_ERROR,
                               "GDBus.Error:%s: %s",
                               dbus_error_name,
                               dbus_error_message);
        }
    }

  G_UNLOCK (error_lock);
  return error;
}

/**
 * g_dbus_error_set_dbus_error:
 * @error: A pointer to a #xerror_t or %NULL.
 * @dbus_error_name: D-Bus error name.
 * @dbus_error_message: D-Bus error message.
 * @format: (nullable): printf()-style format to prepend to @dbus_error_message or %NULL.
 * @...: Arguments for @format.
 *
 * Does nothing if @error is %NULL. Otherwise sets *@error to
 * a new #xerror_t created with g_dbus_error_new_for_dbus_error()
 * with @dbus_error_message prepend with @format (unless %NULL).
 *
 * Since: 2.26
 */
void
g_dbus_error_set_dbus_error (xerror_t      **error,
                             const xchar_t  *dbus_error_name,
                             const xchar_t  *dbus_error_message,
                             const xchar_t  *format,
                             ...)
{
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (dbus_error_name != NULL);
  g_return_if_fail (dbus_error_message != NULL);

  if (error == NULL)
    return;

  if (format == NULL)
    {
      *error = g_dbus_error_new_for_dbus_error (dbus_error_name, dbus_error_message);
    }
  else
    {
      va_list var_args;
      va_start (var_args, format);
      g_dbus_error_set_dbus_error_valist (error,
                                          dbus_error_name,
                                          dbus_error_message,
                                          format,
                                          var_args);
      va_end (var_args);
    }
}

/**
 * g_dbus_error_set_dbus_error_valist:
 * @error: A pointer to a #xerror_t or %NULL.
 * @dbus_error_name: D-Bus error name.
 * @dbus_error_message: D-Bus error message.
 * @format: (nullable): printf()-style format to prepend to @dbus_error_message or %NULL.
 * @var_args: Arguments for @format.
 *
 * Like g_dbus_error_set_dbus_error() but intended for language bindings.
 *
 * Since: 2.26
 */
void
g_dbus_error_set_dbus_error_valist (xerror_t      **error,
                                    const xchar_t  *dbus_error_name,
                                    const xchar_t  *dbus_error_message,
                                    const xchar_t  *format,
                                    va_list       var_args)
{
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (dbus_error_name != NULL);
  g_return_if_fail (dbus_error_message != NULL);

  if (error == NULL)
    return;

  if (format != NULL)
    {
      xchar_t *message;
      xchar_t *s;
      message = xstrdup_vprintf (format, var_args);
      s = xstrdup_printf ("%s: %s", message, dbus_error_message);
      *error = g_dbus_error_new_for_dbus_error (dbus_error_name, s);
      g_free (s);
      g_free (message);
    }
  else
    {
      *error = g_dbus_error_new_for_dbus_error (dbus_error_name, dbus_error_message);
    }
}

/**
 * g_dbus_error_strip_remote_error:
 * @error: A #xerror_t.
 *
 * Looks for extra information in the error message used to recover
 * the D-Bus error name and strips it if found. If stripped, the
 * message field in @error will correspond exactly to what was
 * received on the wire.
 *
 * This is typically used when presenting errors to the end user.
 *
 * Returns: %TRUE if information was stripped, %FALSE otherwise.
 *
 * Since: 2.26
 */
xboolean_t
g_dbus_error_strip_remote_error (xerror_t *error)
{
  xboolean_t ret;

  g_return_val_if_fail (error != NULL, FALSE);

  ret = FALSE;

  if (xstr_has_prefix (error->message, "GDBus.Error:"))
    {
      const xchar_t *begin;
      const xchar_t *end;
      xchar_t *new_message;

      begin = error->message + sizeof ("GDBus.Error:") -1;
      end = strstr (begin, ":");
      if (end != NULL && end[1] == ' ')
        {
          new_message = xstrdup (end + 2);
          g_free (error->message);
          error->message = new_message;
          ret = TRUE;
        }
    }

  return ret;
}

/**
 * g_dbus_error_encode_gerror:
 * @error: A #xerror_t.
 *
 * Creates a D-Bus error name to use for @error. If @error matches
 * a registered error (cf. g_dbus_error_register_error()), the corresponding
 * D-Bus error name will be returned.
 *
 * Otherwise the a name of the form
 * `org.gtk.GDBus.UnmappedGError.Quark._ESCAPED_QUARK_NAME.Code_ERROR_CODE`
 * will be used. This allows other GDBus applications to map the error
 * on the wire back to a #xerror_t using g_dbus_error_new_for_dbus_error().
 *
 * This function is typically only used in object mappings to put a
 * #xerror_t on the wire. Regular applications should not use it.
 *
 * Returns: (transfer full) (not nullable): A D-Bus error name (never %NULL).
 *     Free with g_free().
 *
 * Since: 2.26
 */
xchar_t *
g_dbus_error_encode_gerror (const xerror_t *error)
{
  RegisteredError *re;
  xchar_t *error_name;

  g_return_val_if_fail (error != NULL, NULL);

  /* Ensure that e.g. G_DBUS_ERROR is registered using g_dbus_error_register_error() */
  _g_dbus_initialize ();

  error_name = NULL;

  G_LOCK (error_lock);
  re = NULL;
  if (quark_code_pair_to_re != NULL)
    {
      QuarkCodePair pair;
      pair.error_domain = error->domain;
      pair.error_code = error->code;
      g_assert (dbus_error_name_to_re != NULL); /* check invariant */
      re = xhash_table_lookup (quark_code_pair_to_re, &pair);
    }
  if (re != NULL)
    {
      error_name = xstrdup (re->dbus_error_name);
      G_UNLOCK (error_lock);
    }
  else
    {
      const xchar_t *domain_as_string;
      xstring_t *s;
      xuint_t n;

      G_UNLOCK (error_lock);

      /* We can't make a lot of assumptions about what domain_as_string
       * looks like and D-Bus is extremely picky about error names so
       * hex-encode it for transport across the wire.
       */
      domain_as_string = g_quark_to_string (error->domain);

      /* 0 is not a domain; neither are non-quark integers */
      g_return_val_if_fail (domain_as_string != NULL, NULL);

      s = xstring_new ("org.gtk.GDBus.UnmappedGError.Quark._");
      for (n = 0; domain_as_string[n] != 0; n++)
        {
          xint_t c = domain_as_string[n];
          if (g_ascii_isalnum (c))
            {
              xstring_append_c (s, c);
            }
          else
            {
              xuint_t nibble_top;
              xuint_t nibble_bottom;
              xstring_append_c (s, '_');
              nibble_top = ((int) domain_as_string[n]) >> 4;
              nibble_bottom = ((int) domain_as_string[n]) & 0x0f;
              if (nibble_top < 10)
                nibble_top += '0';
              else
                nibble_top += 'a' - 10;
              if (nibble_bottom < 10)
                nibble_bottom += '0';
              else
                nibble_bottom += 'a' - 10;
              xstring_append_c (s, nibble_top);
              xstring_append_c (s, nibble_bottom);
            }
        }
      xstring_append_printf (s, ".Code%d", error->code);
      error_name = xstring_free (s, FALSE);
    }

  return error_name;
}
