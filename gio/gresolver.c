/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2018 Igalia S.L.
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
 */

#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "gresolver.h"
#include "gnetworkingprivate.h"
#include "gasyncresult.h"
#include "ginetaddress.h"
#include "gtask.h"
#include "gsrvtarget.h"
#include "gthreadedresolver.h"
#include "gioerror.h"
#include "gcancellable.h"

#ifdef G_OS_UNIX
#include <sys/stat.h>
#endif

#include <stdlib.h>


/**
 * SECTION:gresolver
 * @short_description: Asynchronous and cancellable DNS resolver
 * @include: gio/gio.h
 *
 * #xresolver_t provides cancellable synchronous and asynchronous DNS
 * resolution, for hostnames (g_resolver_lookup_by_address(),
 * g_resolver_lookup_by_name() and their async variants) and SRV
 * (service) records (g_resolver_lookup_service()).
 *
 * #xnetwork_address_t and #xnetwork_service_t provide wrappers around
 * #xresolver_t functionality that also implement #xsocket_connectable_t,
 * making it easy to connect to a remote host/service.
 */

enum {
  RELOAD,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

struct _GResolverPrivate {
#ifdef G_OS_UNIX
  xmutex_t mutex;
  time_t resolv_conf_timestamp;  /* protected by @mutex */
#else
  int dummy;
#endif
};

/**
 * xresolver_t:
 *
 * The object that handles DNS resolution. Use g_resolver_get_default()
 * to get the default resolver.
 *
 * This is an abstract type; subclasses of it implement different resolvers for
 * different platforms and situations.
 */
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (xresolver_t, g_resolver, XTYPE_OBJECT,
                                  G_ADD_PRIVATE (xresolver_t)
                                  g_networking_init ();)

static xlist_t *
srv_records_to_targets (xlist_t *records)
{
  const xchar_t *hostname;
  xuint16_t port, priority, weight;
  xsrv_target_t *target;
  xlist_t *l;

  for (l = records; l != NULL; l = xlist_next (l))
    {
      xvariant_get (l->data, "(qqq&s)", &priority, &weight, &port, &hostname);
      target = g_srv_target_new (hostname, port, priority, weight);
      xvariant_unref (l->data);
      l->data = target;
    }

  return g_srv_target_list_sort (records);
}

static xlist_t *
g_resolver_real_lookup_service (xresolver_t            *resolver,
                                const xchar_t          *rrname,
                                xcancellable_t         *cancellable,
                                xerror_t              **error)
{
  xlist_t *records;

  records = G_RESOLVER_GET_CLASS (resolver)->lookup_records (resolver,
                                                             rrname,
                                                             G_RESOLVER_RECORD_SRV,
                                                             cancellable,
                                                             error);

  return srv_records_to_targets (records);
}

static void
g_resolver_real_lookup_service_async (xresolver_t            *resolver,
                                      const xchar_t          *rrname,
                                      xcancellable_t         *cancellable,
                                      xasync_ready_callback_t   callback,
                                      xpointer_t              user_data)
{
  G_RESOLVER_GET_CLASS (resolver)->lookup_records_async (resolver,
                                                         rrname,
                                                         G_RESOLVER_RECORD_SRV,
                                                         cancellable,
                                                         callback,
                                                         user_data);
}

static xlist_t *
g_resolver_real_lookup_service_finish (xresolver_t            *resolver,
                                       xasync_result_t         *result,
                                       xerror_t              **error)
{
  xlist_t *records;

  records = G_RESOLVER_GET_CLASS (resolver)->lookup_records_finish (resolver,
                                                                    result,
                                                                    error);

  return srv_records_to_targets (records);
}

static void
g_resolver_finalize (xobject_t *object)
{
#ifdef G_OS_UNIX
  xresolver_t *resolver = G_RESOLVER (object);

  g_mutex_clear (&resolver->priv->mutex);
#endif

  G_OBJECT_CLASS (g_resolver_parent_class)->finalize (object);
}

static void
g_resolver_class_init (GResolverClass *resolver_class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (resolver_class);

  object_class->finalize = g_resolver_finalize;

  /* Automatically pass these over to the lookup_records methods */
  resolver_class->lookup_service = g_resolver_real_lookup_service;
  resolver_class->lookup_service_async = g_resolver_real_lookup_service_async;
  resolver_class->lookup_service_finish = g_resolver_real_lookup_service_finish;

  /**
   * xresolver_t::reload:
   * @resolver: a #xresolver_t
   *
   * Emitted when the resolver notices that the system resolver
   * configuration has changed.
   **/
  signals[RELOAD] =
    g_signal_new (I_("reload"),
		  XTYPE_RESOLVER,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GResolverClass, reload),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 0);
}

static void
g_resolver_init (xresolver_t *resolver)
{
#ifdef G_OS_UNIX
  struct stat st;
#endif

  resolver->priv = g_resolver_get_instance_private (resolver);

#ifdef G_OS_UNIX
  if (stat (_PATH_RESCONF, &st) == 0)
    resolver->priv->resolv_conf_timestamp = st.st_mtime;

  g_mutex_init (&resolver->priv->mutex);
#endif
}

G_LOCK_DEFINE_STATIC (default_resolver);
static xresolver_t *default_resolver;

/**
 * g_resolver_get_default:
 *
 * Gets the default #xresolver_t. You should unref it when you are done
 * with it. #xresolver_t may use its reference count as a hint about how
 * many threads it should allocate for concurrent DNS resolutions.
 *
 * Returns: (transfer full): the default #xresolver_t.
 *
 * Since: 2.22
 */
xresolver_t *
g_resolver_get_default (void)
{
  xresolver_t *ret;

  G_LOCK (default_resolver);
  if (!default_resolver)
    default_resolver = xobject_new (XTYPE_THREADED_RESOLVER, NULL);
  ret = xobject_ref (default_resolver);
  G_UNLOCK (default_resolver);

  return ret;
}

/**
 * g_resolver_set_default:
 * @resolver: the new default #xresolver_t
 *
 * Sets @resolver to be the application's default resolver (reffing
 * @resolver, and unreffing the previous default resolver, if any).
 * Future calls to g_resolver_get_default() will return this resolver.
 *
 * This can be used if an application wants to perform any sort of DNS
 * caching or "pinning"; it can implement its own #xresolver_t that
 * calls the original default resolver for DNS operations, and
 * implements its own cache policies on top of that, and then set
 * itself as the default resolver for all later code to use.
 *
 * Since: 2.22
 */
void
g_resolver_set_default (xresolver_t *resolver)
{
  G_LOCK (default_resolver);
  if (default_resolver)
    xobject_unref (default_resolver);
  default_resolver = xobject_ref (resolver);
  G_UNLOCK (default_resolver);
}

static void
maybe_emit_reload (xresolver_t *resolver)
{
#ifdef G_OS_UNIX
  struct stat st;

  if (stat (_PATH_RESCONF, &st) == 0)
    {
      g_mutex_lock (&resolver->priv->mutex);
      if (st.st_mtime != resolver->priv->resolv_conf_timestamp)
        {
          resolver->priv->resolv_conf_timestamp = st.st_mtime;
          g_mutex_unlock (&resolver->priv->mutex);
          g_signal_emit (resolver, signals[RELOAD], 0);
        }
      else
        g_mutex_unlock (&resolver->priv->mutex);
    }
#endif
}

/* filter out duplicates, cf. https://bugzilla.gnome.org/show_bug.cgi?id=631379 */
static void
remove_duplicates (xlist_t *addrs)
{
  xlist_t *l;
  xlist_t *ll;
  xlist_t *lll;

  /* TODO: if this is too slow (it's O(n^2) but n is typically really
   * small), we can do something more clever but note that we must not
   * change the order of elements...
   */
  for (l = addrs; l != NULL; l = l->next)
    {
      xinet_address_t *address = G_INET_ADDRESS (l->data);
      for (ll = l->next; ll != NULL; ll = lll)
        {
          xinet_address_t *other_address = G_INET_ADDRESS (ll->data);
          lll = ll->next;
          if (xinet_address_equal (address, other_address))
            {
              xobject_unref (other_address);
              /* we never return the first element */
              g_warn_if_fail (xlist_delete_link (addrs, ll) == addrs);
            }
        }
    }
}

static xboolean_t
hostname_is_localhost (const char *hostname)
{
  size_t len = strlen (hostname);
  const char *p;

  /* Match "localhost", "localhost.", "*.localhost" and "*.localhost." */
  if (len < strlen ("localhost"))
    return FALSE;

  if (hostname[len - 1] == '.')
      len--;

  /* Scan backwards in @hostname to find the right-most dot (excluding the final dot, if it exists, as it was chopped off above).
   * We can’t use strrchr() because because we need to operate with string lengths.
   * End with @p pointing to the character after the right-most dot. */
  p = hostname + len - 1;
  while (p >= hostname)
    {
      if (*p == '.')
       {
         p++;
         break;
       }
      else if (p == hostname)
        break;
      p--;
    }

  len -= p - hostname;

  return g_ascii_strncasecmp (p, "localhost", MAX (len, strlen ("localhost"))) == 0;
}

/* Note that this does not follow the "FALSE means @error is set"
 * convention. The return value tells the caller whether it should
 * return @addrs and @error to the caller right away, or if it should
 * continue and trying to resolve the name as a hostname.
 */
static xboolean_t
handle_ip_address_or_localhost (const char                *hostname,
                                xlist_t                    **addrs,
                                GResolverNameLookupFlags   flags,
                                xerror_t                   **error)
{
  xinet_address_t *addr;

#ifndef G_OS_WIN32
  struct in_addr ip4addr;
#endif

  addr = xinet_address_new_from_string (hostname);
  if (addr)
    {
      *addrs = xlist_append (NULL, addr);
      return TRUE;
    }

  *addrs = NULL;

#ifdef G_OS_WIN32

  /* Reject IPv6 addresses that have brackets ('[' or ']') and/or port numbers,
   * as no valid addresses should contain these at this point.
   * Non-standard IPv4 addresses would be rejected during the call to
   * getaddrinfo() later.
   */
  if (strrchr (hostname, '[') != NULL ||
      strrchr (hostname, ']') != NULL)
#else

  /* Reject non-standard IPv4 numbers-and-dots addresses.
   * xinet_address_new_from_string() will have accepted any "real" IP
   * address, so if inet_aton() succeeds, then it's an address we want
   * to reject.
   */
  if (inet_aton (hostname, &ip4addr))
#endif
    {
#ifdef G_OS_WIN32
      xchar_t *error_message = g_win32_error_message (WSAHOST_NOT_FOUND);
#else
      xchar_t *error_message = g_locale_to_utf8 (gai_strerror (EAI_NONAME), -1, NULL, NULL, NULL);
      if (error_message == NULL)
        error_message = xstrdup ("[Invalid UTF-8]");
#endif
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                   _("Error resolving “%s”: %s"),
                   hostname, error_message);
      g_free (error_message);

      return TRUE;
    }

  /* Always resolve localhost to a loopback address so it can be reliably considered secure.
     This behavior is being adopted by browsers:
     - https://w3c.github.io/webappsec-secure-contexts/
     - https://groups.google.com/a/chromium.org/forum/#!msg/blink-dev/RC9dSw-O3fE/E3_0XaT0BAAJ
     - https://chromium.googlesource.com/chromium/src.git/+/8da2a80724a9b896890602ff77ef2216cb951399
     - https://bugs.webkit.org/show_bug.cgi?id=171934
     - https://tools.ietf.org/html/draft-west-let-localhost-be-localhost-06
  */
  if (hostname_is_localhost (hostname))
    {
      if (flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
        *addrs = xlist_append (*addrs, xinet_address_new_loopback (XSOCKET_FAMILY_IPV6));
      if (flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
        *addrs = xlist_append (*addrs, xinet_address_new_loopback (XSOCKET_FAMILY_IPV4));
      if (*addrs == NULL)
        {
          *addrs = xlist_append (*addrs, xinet_address_new_loopback (XSOCKET_FAMILY_IPV6));
          *addrs = xlist_append (*addrs, xinet_address_new_loopback (XSOCKET_FAMILY_IPV4));
        }
      return TRUE;
    }

  return FALSE;
}

static xlist_t *
lookup_by_name_real (xresolver_t                 *resolver,
                     const xchar_t               *hostname,
                     GResolverNameLookupFlags   flags,
                     xcancellable_t              *cancellable,
                     xerror_t                    **error)
{
  xlist_t *addrs;
  xchar_t *ascii_hostname = NULL;

  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (hostname != NULL, NULL);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* Check if @hostname is just an IP address */
  if (handle_ip_address_or_localhost (hostname, &addrs, flags, error))
    return addrs;

  if (g_hostname_is_non_ascii (hostname))
    hostname = ascii_hostname = g_hostname_to_ascii (hostname);

  if (!hostname)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           _("Invalid hostname"));
      return NULL;
    }

  maybe_emit_reload (resolver);

  if (flags != G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT)
    {
      if (!G_RESOLVER_GET_CLASS (resolver)->lookup_by_name_with_flags)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       /* Translators: The placeholder is for a function name. */
                       _("%s not implemented"), "lookup_by_name_with_flags");
          g_free (ascii_hostname);
          return NULL;
        }
      addrs = G_RESOLVER_GET_CLASS (resolver)->
        lookup_by_name_with_flags (resolver, hostname, flags, cancellable, error);
    }
  else
    addrs = G_RESOLVER_GET_CLASS (resolver)->
      lookup_by_name (resolver, hostname, cancellable, error);

  remove_duplicates (addrs);

  g_free (ascii_hostname);
  return addrs;
}

/**
 * g_resolver_lookup_by_name:
 * @resolver: a #xresolver_t
 * @hostname: the hostname to look up
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Synchronously resolves @hostname to determine its associated IP
 * address(es). @hostname may be an ASCII-only or UTF-8 hostname, or
 * the textual form of an IP address (in which case this just becomes
 * a wrapper around xinet_address_new_from_string()).
 *
 * On success, g_resolver_lookup_by_name() will return a non-empty #xlist_t of
 * #xinet_address_t, sorted in order of preference and guaranteed to not
 * contain duplicates. That is, if using the result to connect to
 * @hostname, you should attempt to connect to the first address
 * first, then the second if the first fails, etc. If you are using
 * the result to listen on a socket, it is appropriate to add each
 * result using e.g. xsocket_listener_add_address().
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to a
 * value from #GResolverError and %NULL will be returned.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * If you are planning to connect to a socket on the resolved IP
 * address, it may be easier to create a #xnetwork_address_t and use its
 * #xsocket_connectable_t interface.
 *
 * Returns: (element-type xinet_address_t) (transfer full): a non-empty #xlist_t
 * of #xinet_address_t, or %NULL on error. You
 * must unref each of the addresses and free the list when you are
 * done with it. (You can use g_resolver_free_addresses() to do this.)
 *
 * Since: 2.22
 */
xlist_t *
g_resolver_lookup_by_name (xresolver_t     *resolver,
                           const xchar_t   *hostname,
                           xcancellable_t  *cancellable,
                           xerror_t       **error)
{
  return lookup_by_name_real (resolver,
                              hostname,
                              G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT,
                              cancellable,
                              error);
}

/**
 * g_resolver_lookup_by_name_with_flags:
 * @resolver: a #xresolver_t
 * @hostname: the hostname to look up
 * @flags: extra #GResolverNameLookupFlags for the lookup
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: (nullable): return location for a #xerror_t, or %NULL
 *
 * This differs from g_resolver_lookup_by_name() in that you can modify
 * the lookup behavior with @flags. For example this can be used to limit
 * results with %G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY.
 *
 * Returns: (element-type xinet_address_t) (transfer full): a non-empty #xlist_t
 * of #xinet_address_t, or %NULL on error. You
 * must unref each of the addresses and free the list when you are
 * done with it. (You can use g_resolver_free_addresses() to do this.)
 *
 * Since: 2.60
 */
xlist_t *
g_resolver_lookup_by_name_with_flags (xresolver_t                 *resolver,
                                      const xchar_t               *hostname,
                                      GResolverNameLookupFlags   flags,
                                      xcancellable_t              *cancellable,
                                      xerror_t                   **error)
{
  return lookup_by_name_real (resolver,
                              hostname,
                              flags,
                              cancellable,
                              error);
}

static void
lookup_by_name_async_real (xresolver_t                *resolver,
                           const xchar_t              *hostname,
                           GResolverNameLookupFlags  flags,
                           xcancellable_t             *cancellable,
                           xasync_ready_callback_t       callback,
                           xpointer_t                  user_data)
{
  xchar_t *ascii_hostname = NULL;
  xlist_t *addrs;
  xerror_t *error = NULL;

  g_return_if_fail (X_IS_RESOLVER (resolver));
  g_return_if_fail (hostname != NULL);
  g_return_if_fail (!(flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY && flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY));

  /* Check if @hostname is just an IP address */
  if (handle_ip_address_or_localhost (hostname, &addrs, flags, &error))
    {
      xtask_t *task;

      task = xtask_new (resolver, cancellable, callback, user_data);
      xtask_set_source_tag (task, lookup_by_name_async_real);
      xtask_set_name (task, "[gio] resolver lookup");
      if (addrs)
        xtask_return_pointer (task, addrs, (xdestroy_notify_t) g_resolver_free_addresses);
      else
        xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  if (g_hostname_is_non_ascii (hostname))
    hostname = ascii_hostname = g_hostname_to_ascii (hostname);

  if (!hostname)
    {
      xtask_t *task;

      g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           _("Invalid hostname"));
      task = xtask_new (resolver, cancellable, callback, user_data);
      xtask_set_source_tag (task, lookup_by_name_async_real);
      xtask_set_name (task, "[gio] resolver lookup");
      xtask_return_error (task, error);
      xobject_unref (task);
      return;
    }

  maybe_emit_reload (resolver);

  if (flags != G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT)
    {
      if (G_RESOLVER_GET_CLASS (resolver)->lookup_by_name_with_flags_async == NULL)
        {
          xtask_t *task;

          g_set_error (&error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       /* Translators: The placeholder is for a function name. */
                       _("%s not implemented"), "lookup_by_name_with_flags_async");
          task = xtask_new (resolver, cancellable, callback, user_data);
          xtask_set_source_tag (task, lookup_by_name_async_real);
          xtask_set_name (task, "[gio] resolver lookup");
          xtask_return_error (task, error);
          xobject_unref (task);
        }
      else
        G_RESOLVER_GET_CLASS (resolver)->
          lookup_by_name_with_flags_async (resolver, hostname, flags, cancellable, callback, user_data);
    }
  else
    G_RESOLVER_GET_CLASS (resolver)->
      lookup_by_name_async (resolver, hostname, cancellable, callback, user_data);

  g_free (ascii_hostname);
}

static xlist_t *
lookup_by_name_finish_real (xresolver_t     *resolver,
                            xasync_result_t  *result,
                            xerror_t       **error,
                            xboolean_t       with_flags)
{
  xlist_t *addrs;

  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;
  else if (xasync_result_is_tagged (result, lookup_by_name_async_real))
    {
      /* Handle the stringified-IP-addr case */
      return xtask_propagate_pointer (XTASK (result), error);
    }

  if (with_flags)
    {
      g_assert (G_RESOLVER_GET_CLASS (resolver)->lookup_by_name_with_flags_finish != NULL);
      addrs = G_RESOLVER_GET_CLASS (resolver)->
        lookup_by_name_with_flags_finish (resolver, result, error);
    }
  else
    addrs = G_RESOLVER_GET_CLASS (resolver)->
      lookup_by_name_finish (resolver, result, error);

  remove_duplicates (addrs);

  return addrs;
}

/**
 * g_resolver_lookup_by_name_with_flags_async:
 * @resolver: a #xresolver_t
 * @hostname: the hostname to look up the address of
 * @flags: extra #GResolverNameLookupFlags for the lookup
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): callback to call after resolution completes
 * @user_data: (closure): data for @callback
 *
 * Begins asynchronously resolving @hostname to determine its
 * associated IP address(es), and eventually calls @callback, which
 * must call g_resolver_lookup_by_name_with_flags_finish() to get the result.
 * See g_resolver_lookup_by_name() for more details.
 *
 * Since: 2.60
 */
void
g_resolver_lookup_by_name_with_flags_async (xresolver_t                *resolver,
                                            const xchar_t              *hostname,
                                            GResolverNameLookupFlags  flags,
                                            xcancellable_t             *cancellable,
                                            xasync_ready_callback_t       callback,
                                            xpointer_t                  user_data)
{
  lookup_by_name_async_real (resolver,
                             hostname,
                             flags,
                             cancellable,
                             callback,
                             user_data);
}

/**
 * g_resolver_lookup_by_name_async:
 * @resolver: a #xresolver_t
 * @hostname: the hostname to look up the address of
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): callback to call after resolution completes
 * @user_data: (closure): data for @callback
 *
 * Begins asynchronously resolving @hostname to determine its
 * associated IP address(es), and eventually calls @callback, which
 * must call g_resolver_lookup_by_name_finish() to get the result.
 * See g_resolver_lookup_by_name() for more details.
 *
 * Since: 2.22
 */
void
g_resolver_lookup_by_name_async (xresolver_t           *resolver,
                                 const xchar_t         *hostname,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  lookup_by_name_async_real (resolver,
                             hostname,
                             0,
                             cancellable,
                             callback,
                             user_data);
}

/**
 * g_resolver_lookup_by_name_finish:
 * @resolver: a #xresolver_t
 * @result: the result passed to your #xasync_ready_callback_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the result of a call to
 * g_resolver_lookup_by_name_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Returns: (element-type xinet_address_t) (transfer full): a #xlist_t
 * of #xinet_address_t, or %NULL on error. See g_resolver_lookup_by_name()
 * for more details.
 *
 * Since: 2.22
 */
xlist_t *
g_resolver_lookup_by_name_finish (xresolver_t     *resolver,
                                  xasync_result_t  *result,
                                  xerror_t       **error)
{
  return lookup_by_name_finish_real (resolver,
                                     result,
                                     error,
                                     FALSE);
}

/**
 * g_resolver_lookup_by_name_with_flags_finish:
 * @resolver: a #xresolver_t
 * @result: the result passed to your #xasync_ready_callback_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the result of a call to
 * g_resolver_lookup_by_name_with_flags_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Returns: (element-type xinet_address_t) (transfer full): a #xlist_t
 * of #xinet_address_t, or %NULL on error. See g_resolver_lookup_by_name()
 * for more details.
 *
 * Since: 2.60
 */
xlist_t *
g_resolver_lookup_by_name_with_flags_finish (xresolver_t     *resolver,
                                             xasync_result_t  *result,
                                             xerror_t       **error)
{
  return lookup_by_name_finish_real (resolver,
                                     result,
                                     error,
                                     TRUE);
}

/**
 * g_resolver_free_addresses: (skip)
 * @addresses: a #xlist_t of #xinet_address_t
 *
 * Frees @addresses (which should be the return value from
 * g_resolver_lookup_by_name() or g_resolver_lookup_by_name_finish()).
 * (This is a convenience method; you can also simply free the results
 * by hand.)
 *
 * Since: 2.22
 */
void
g_resolver_free_addresses (xlist_t *addresses)
{
  xlist_t *a;

  for (a = addresses; a; a = a->next)
    xobject_unref (a->data);
  xlist_free (addresses);
}

/**
 * g_resolver_lookup_by_address:
 * @resolver: a #xresolver_t
 * @address: the address to reverse-resolve
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Synchronously reverse-resolves @address to determine its
 * associated hostname.
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Returns: a hostname (either ASCII-only, or in ASCII-encoded
 *     form), or %NULL on error.
 *
 * Since: 2.22
 */
xchar_t *
g_resolver_lookup_by_address (xresolver_t     *resolver,
                              xinet_address_t  *address,
                              xcancellable_t  *cancellable,
                              xerror_t       **error)
{
  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (X_IS_INET_ADDRESS (address), NULL);

  maybe_emit_reload (resolver);
  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_address (resolver, address, cancellable, error);
}

/**
 * g_resolver_lookup_by_address_async:
 * @resolver: a #xresolver_t
 * @address: the address to reverse-resolve
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): callback to call after resolution completes
 * @user_data: (closure): data for @callback
 *
 * Begins asynchronously reverse-resolving @address to determine its
 * associated hostname, and eventually calls @callback, which must
 * call g_resolver_lookup_by_address_finish() to get the final result.
 *
 * Since: 2.22
 */
void
g_resolver_lookup_by_address_async (xresolver_t           *resolver,
                                    xinet_address_t        *address,
                                    xcancellable_t        *cancellable,
                                    xasync_ready_callback_t  callback,
                                    xpointer_t             user_data)
{
  g_return_if_fail (X_IS_RESOLVER (resolver));
  g_return_if_fail (X_IS_INET_ADDRESS (address));

  maybe_emit_reload (resolver);
  G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_address_async (resolver, address, cancellable, callback, user_data);
}

/**
 * g_resolver_lookup_by_address_finish:
 * @resolver: a #xresolver_t
 * @result: the result passed to your #xasync_ready_callback_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the result of a previous call to
 * g_resolver_lookup_by_address_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Returns: a hostname (either ASCII-only, or in ASCII-encoded
 * form), or %NULL on error.
 *
 * Since: 2.22
 */
xchar_t *
g_resolver_lookup_by_address_finish (xresolver_t     *resolver,
                                     xasync_result_t  *result,
                                     xerror_t       **error)
{
  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_address_finish (resolver, result, error);
}

static xchar_t *
g_resolver_get_service_rrname (const char *service,
                               const char *protocol,
                               const char *domain)
{
  xchar_t *rrname, *ascii_domain = NULL;

  if (g_hostname_is_non_ascii (domain))
    domain = ascii_domain = g_hostname_to_ascii (domain);
  if (!domain)
    return NULL;

  rrname = xstrdup_printf ("_%s._%s.%s", service, protocol, domain);

  g_free (ascii_domain);
  return rrname;
}

/**
 * g_resolver_lookup_service:
 * @resolver: a #xresolver_t
 * @service: the service type to look up (eg, "ldap")
 * @protocol: the networking protocol to use for @service (eg, "tcp")
 * @domain: the DNS domain to look up the service in
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Synchronously performs a DNS SRV lookup for the given @service and
 * @protocol in the given @domain and returns an array of #xsrv_target_t.
 * @domain may be an ASCII-only or UTF-8 hostname. Note also that the
 * @service and @protocol arguments do not include the leading underscore
 * that appears in the actual DNS entry.
 *
 * On success, g_resolver_lookup_service() will return a non-empty #xlist_t of
 * #xsrv_target_t, sorted in order of preference. (That is, you should
 * attempt to connect to the first target first, then the second if
 * the first fails, etc.)
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError and %NULL will be returned.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * If you are planning to connect to the service, it is usually easier
 * to create a #xnetwork_service_t and use its #xsocket_connectable_t
 * interface.
 *
 * Returns: (element-type xsrv_target_t) (transfer full): a non-empty #xlist_t of
 * #xsrv_target_t, or %NULL on error. You must free each of the targets and the
 * list when you are done with it. (You can use g_resolver_free_targets() to do
 * this.)
 *
 * Since: 2.22
 */
xlist_t *
g_resolver_lookup_service (xresolver_t     *resolver,
                           const xchar_t   *service,
                           const xchar_t   *protocol,
                           const xchar_t   *domain,
                           xcancellable_t  *cancellable,
                           xerror_t       **error)
{
  xlist_t *targets;
  xchar_t *rrname;

  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (service != NULL, NULL);
  g_return_val_if_fail (protocol != NULL, NULL);
  g_return_val_if_fail (domain != NULL, NULL);

  rrname = g_resolver_get_service_rrname (service, protocol, domain);
  if (!rrname)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                           _("Invalid domain"));
      return NULL;
    }

  maybe_emit_reload (resolver);
  targets = G_RESOLVER_GET_CLASS (resolver)->
    lookup_service (resolver, rrname, cancellable, error);

  g_free (rrname);
  return targets;
}

/**
 * g_resolver_lookup_service_async:
 * @resolver: a #xresolver_t
 * @service: the service type to look up (eg, "ldap")
 * @protocol: the networking protocol to use for @service (eg, "tcp")
 * @domain: the DNS domain to look up the service in
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): callback to call after resolution completes
 * @user_data: (closure): data for @callback
 *
 * Begins asynchronously performing a DNS SRV lookup for the given
 * @service and @protocol in the given @domain, and eventually calls
 * @callback, which must call g_resolver_lookup_service_finish() to
 * get the final result. See g_resolver_lookup_service() for more
 * details.
 *
 * Since: 2.22
 */
void
g_resolver_lookup_service_async (xresolver_t           *resolver,
                                 const xchar_t         *service,
                                 const xchar_t         *protocol,
                                 const xchar_t         *domain,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  xchar_t *rrname;

  g_return_if_fail (X_IS_RESOLVER (resolver));
  g_return_if_fail (service != NULL);
  g_return_if_fail (protocol != NULL);
  g_return_if_fail (domain != NULL);

  rrname = g_resolver_get_service_rrname (service, protocol, domain);
  if (!rrname)
    {
      xtask_report_new_error (resolver, callback, user_data,
                               g_resolver_lookup_service_async,
                               G_IO_ERROR, G_IO_ERROR_FAILED,
                               _("Invalid domain"));
      return;
    }

  maybe_emit_reload (resolver);
  G_RESOLVER_GET_CLASS (resolver)->
    lookup_service_async (resolver, rrname, cancellable, callback, user_data);

  g_free (rrname);
}

/**
 * g_resolver_lookup_service_finish:
 * @resolver: a #xresolver_t
 * @result: the result passed to your #xasync_ready_callback_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the result of a previous call to
 * g_resolver_lookup_service_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Returns: (element-type xsrv_target_t) (transfer full): a non-empty #xlist_t of
 * #xsrv_target_t, or %NULL on error. See g_resolver_lookup_service() for more
 * details.
 *
 * Since: 2.22
 */
xlist_t *
g_resolver_lookup_service_finish (xresolver_t     *resolver,
                                  xasync_result_t  *result,
                                  xerror_t       **error)
{
  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);

  if (xasync_result_legacy_propagate_error (result, error))
    return NULL;

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_service_finish (resolver, result, error);
}

/**
 * g_resolver_free_targets: (skip)
 * @targets: a #xlist_t of #xsrv_target_t
 *
 * Frees @targets (which should be the return value from
 * g_resolver_lookup_service() or g_resolver_lookup_service_finish()).
 * (This is a convenience method; you can also simply free the
 * results by hand.)
 *
 * Since: 2.22
 */
void
g_resolver_free_targets (xlist_t *targets)
{
  xlist_t *t;

  for (t = targets; t; t = t->next)
    g_srv_target_free (t->data);
  xlist_free (targets);
}

/**
 * g_resolver_lookup_records:
 * @resolver: a #xresolver_t
 * @rrname: the DNS name to look up the record for
 * @record_type: the type of DNS record to look up
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Synchronously performs a DNS record lookup for the given @rrname and returns
 * a list of records as #xvariant_t tuples. See #GResolverRecordType for
 * information on what the records contain for each @record_type.
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError and %NULL will be returned.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Returns: (element-type xvariant_t) (transfer full): a non-empty #xlist_t of
 * #xvariant_t, or %NULL on error. You must free each of the records and the list
 * when you are done with it. (You can use xlist_free_full() with
 * xvariant_unref() to do this.)
 *
 * Since: 2.34
 */
xlist_t *
g_resolver_lookup_records (xresolver_t            *resolver,
                           const xchar_t          *rrname,
                           GResolverRecordType   record_type,
                           xcancellable_t         *cancellable,
                           xerror_t              **error)
{
  xlist_t *records;

  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (rrname != NULL, NULL);

  maybe_emit_reload (resolver);
  records = G_RESOLVER_GET_CLASS (resolver)->
    lookup_records (resolver, rrname, record_type, cancellable, error);

  return records;
}

/**
 * g_resolver_lookup_records_async:
 * @resolver: a #xresolver_t
 * @rrname: the DNS name to look up the record for
 * @record_type: the type of DNS record to look up
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: (scope async): callback to call after resolution completes
 * @user_data: (closure): data for @callback
 *
 * Begins asynchronously performing a DNS lookup for the given
 * @rrname, and eventually calls @callback, which must call
 * g_resolver_lookup_records_finish() to get the final result. See
 * g_resolver_lookup_records() for more details.
 *
 * Since: 2.34
 */
void
g_resolver_lookup_records_async (xresolver_t           *resolver,
                                 const xchar_t         *rrname,
                                 GResolverRecordType  record_type,
                                 xcancellable_t        *cancellable,
                                 xasync_ready_callback_t  callback,
                                 xpointer_t             user_data)
{
  g_return_if_fail (X_IS_RESOLVER (resolver));
  g_return_if_fail (rrname != NULL);

  maybe_emit_reload (resolver);
  G_RESOLVER_GET_CLASS (resolver)->
    lookup_records_async (resolver, rrname, record_type, cancellable, callback, user_data);
}

/**
 * g_resolver_lookup_records_finish:
 * @resolver: a #xresolver_t
 * @result: the result passed to your #xasync_ready_callback_t
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the result of a previous call to
 * g_resolver_lookup_records_async(). Returns a non-empty list of records as
 * #xvariant_t tuples. See #GResolverRecordType for information on what the
 * records contain.
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Returns: (element-type xvariant_t) (transfer full): a non-empty #xlist_t of
 * #xvariant_t, or %NULL on error. You must free each of the records and the list
 * when you are done with it. (You can use xlist_free_full() with
 * xvariant_unref() to do this.)
 *
 * Since: 2.34
 */
xlist_t *
g_resolver_lookup_records_finish (xresolver_t     *resolver,
                                  xasync_result_t  *result,
                                  xerror_t       **error)
{
  g_return_val_if_fail (X_IS_RESOLVER (resolver), NULL);
  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_records_finish (resolver, result, error);
}

xuint64_t
g_resolver_get_serial (xresolver_t *resolver)
{
  xuint64_t result;

  g_return_val_if_fail (X_IS_RESOLVER (resolver), 0);

  maybe_emit_reload (resolver);

#ifdef G_OS_UNIX
  g_mutex_lock (&resolver->priv->mutex);
  result = resolver->priv->resolv_conf_timestamp;
  g_mutex_unlock (&resolver->priv->mutex);
#else
  result = 1;
#endif

  return result;
}

/**
 * g_resolver_error_quark:
 *
 * Gets the #xresolver_t Error Quark.
 *
 * Returns: a #xquark.
 *
 * Since: 2.22
 */
G_DEFINE_QUARK (g-resolver-error-quark, g_resolver_error)
