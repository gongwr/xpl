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

#ifndef __G_RESOLVER_H__
#define __G_RESOLVER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_RESOLVER         (g_resolver_get_type ())
#define G_RESOLVER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_RESOLVER, xresolver))
#define G_RESOLVER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_RESOLVER, GResolverClass))
#define X_IS_RESOLVER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_RESOLVER))
#define X_IS_RESOLVER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_RESOLVER))
#define G_RESOLVER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_RESOLVER, GResolverClass))

typedef struct _GResolverPrivate GResolverPrivate;
typedef struct _GResolverClass   GResolverClass;

struct _GResolver {
  xobject_t parent_instance;

  GResolverPrivate *priv;
};

/**
 * GResolverNameLookupFlags:
 * @G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT: default behavior (same as g_resolver_lookup_by_name())
 * @G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY: only resolve ipv4 addresses
 * @G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY: only resolve ipv6 addresses
 *
 * Flags to modify lookup behavior.
 *
 * Since: 2.60
 */
typedef enum {
  G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT = 0,
  G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY = 1 << 0,
  G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY = 1 << 1,
} GResolverNameLookupFlags;

struct _GResolverClass {
  xobject_class_t parent_class;

  /* Signals */
  void    ( *reload)                           (xresolver_t               *resolver);

  /* Virtual methods */
  xlist_t * ( *lookup_by_name)                   (xresolver_t               *resolver,
                                                const xchar_t             *hostname,
                                                xcancellable_t            *cancellable,
                                                xerror_t                 **error);
  void    ( *lookup_by_name_async)             (xresolver_t               *resolver,
                                                const xchar_t             *hostname,
                                                xcancellable_t            *cancellable,
                                                xasync_ready_callback_t      callback,
                                                xpointer_t                 user_data);
  xlist_t * ( *lookup_by_name_finish)            (xresolver_t               *resolver,
                                                xasync_result_t            *result,
                                                xerror_t                 **error);

  xchar_t * ( *lookup_by_address)                (xresolver_t               *resolver,
                                                xinet_address_t            *address,
                                                xcancellable_t            *cancellable,
                                                xerror_t                 **error);
  void    ( *lookup_by_address_async)          (xresolver_t               *resolver,
                                                xinet_address_t            *address,
                                                xcancellable_t            *cancellable,
                                                xasync_ready_callback_t      callback,
                                                xpointer_t                 user_data);
  xchar_t * ( *lookup_by_address_finish)         (xresolver_t               *resolver,
                                                xasync_result_t            *result,
                                                xerror_t                 **error);

  xlist_t * ( *lookup_service)                   (xresolver_t               *resolver,
                                                const xchar_t              *rrname,
                                                xcancellable_t             *cancellable,
                                                xerror_t                  **error);
  void    ( *lookup_service_async)             (xresolver_t                *resolver,
                                                const xchar_t              *rrname,
                                                xcancellable_t             *cancellable,
                                                xasync_ready_callback_t       callback,
                                                xpointer_t                  user_data);
  xlist_t * ( *lookup_service_finish)            (xresolver_t                *resolver,
                                                xasync_result_t             *result,
                                                xerror_t                  **error);

  xlist_t * ( *lookup_records)                   (xresolver_t                *resolver,
                                                const xchar_t              *rrname,
                                                GResolverRecordType       record_type,
                                                xcancellable_t             *cancellable,
                                                xerror_t                  **error);

  void    ( *lookup_records_async)             (xresolver_t                *resolver,
                                                const xchar_t              *rrname,
                                                GResolverRecordType       record_type,
                                                xcancellable_t             *cancellable,
                                                xasync_ready_callback_t       callback,
                                                xpointer_t                  user_data);

  xlist_t * ( *lookup_records_finish)            (xresolver_t                *resolver,
                                                xasync_result_t             *result,
                                                xerror_t                   **error);
  /**
   * GResolverClass::lookup_by_name_with_flags_async:
   * @resolver: a #xresolver_t
   * @hostname: the hostname to resolve
   * @flags: extra #GResolverNameLookupFlags to modify the lookup
   * @cancellable: (nullable): a #xcancellable_t
   * @callback: (scope async): a #xasync_ready_callback_t to call when completed
   * @user_data: (closure): data to pass to @callback
   *
   * Asynchronous version of GResolverClass::lookup_by_name_with_flags
   *
   * GResolverClass::lookup_by_name_with_flags_finish will be called to get
   * the result.
   *
   * Since: 2.60
   */
  void    ( *lookup_by_name_with_flags_async)  (xresolver_t                 *resolver,
                                                const xchar_t               *hostname,
                                                GResolverNameLookupFlags   flags,
                                                xcancellable_t              *cancellable,
                                                xasync_ready_callback_t        callback,
                                                xpointer_t                   user_data);
  /**
   * GResolverClass::lookup_by_name_with_flags_finish:
   * @resolver: a #xresolver_t
   * @result: a #xasync_result_t
   * @error: (nullable): a pointer to a %NULL #xerror_t
   *
   * Gets the result from GResolverClass::lookup_by_name_with_flags_async
   *
   * Returns: (element-type xinet_address_t) (transfer full): List of #xinet_address_t.
   * Since: 2.60
   */
  xlist_t * ( *lookup_by_name_with_flags_finish) (xresolver_t                 *resolver,
                                                xasync_result_t              *result,
                                                xerror_t                   **error);
  /**
   * GResolverClass::lookup_by_name_with_flags:
   * @resolver: a #xresolver_t
   * @hostname: the hostname to resolve
   * @flags: extra #GResolverNameLookupFlags to modify the lookup
   * @cancellable: (nullable): a #xcancellable_t
   * @error: (nullable): a pointer to a %NULL #xerror_t
   *
   * This is identical to GResolverClass::lookup_by_name except it takes
   * @flags which modifies the behavior of the lookup. See #GResolverNameLookupFlags
   * for more details.
   *
   * Returns: (element-type xinet_address_t) (transfer full): List of #xinet_address_t.
   * Since: 2.60
   */
  xlist_t * ( *lookup_by_name_with_flags)        (xresolver_t                 *resolver,
                                                const xchar_t               *hostname,
                                                GResolverNameLookupFlags   flags,
                                                xcancellable_t              *cancellable,
                                                xerror_t                   **error);

};

XPL_AVAILABLE_IN_ALL
xtype_t      g_resolver_get_type                         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xresolver_t *g_resolver_get_default                      (void);
XPL_AVAILABLE_IN_ALL
void       g_resolver_set_default                      (xresolver_t                 *resolver);
XPL_AVAILABLE_IN_ALL
xlist_t     *g_resolver_lookup_by_name                   (xresolver_t                 *resolver,
                                                        const xchar_t               *hostname,
                                                        xcancellable_t              *cancellable,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void       g_resolver_lookup_by_name_async             (xresolver_t                 *resolver,
                                                        const xchar_t               *hostname,
                                                        xcancellable_t              *cancellable,
                                                        xasync_ready_callback_t        callback,
                                                        xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
xlist_t     *g_resolver_lookup_by_name_finish            (xresolver_t                 *resolver,
                                                        xasync_result_t              *result,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_2_60
void       g_resolver_lookup_by_name_with_flags_async  (xresolver_t                 *resolver,
                                                        const xchar_t               *hostname,
                                                        GResolverNameLookupFlags   flags,
                                                        xcancellable_t              *cancellable,
                                                        xasync_ready_callback_t        callback,
                                                        xpointer_t                   user_data);
XPL_AVAILABLE_IN_2_60
xlist_t     *g_resolver_lookup_by_name_with_flags_finish (xresolver_t                 *resolver,
                                                        xasync_result_t              *result,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_2_60
xlist_t     *g_resolver_lookup_by_name_with_flags        (xresolver_t                 *resolver,
                                                        const xchar_t               *hostname,
                                                        GResolverNameLookupFlags   flags,
                                                        xcancellable_t              *cancellable,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void       g_resolver_free_addresses                   (xlist_t                     *addresses);
XPL_AVAILABLE_IN_ALL
xchar_t     *g_resolver_lookup_by_address                (xresolver_t                 *resolver,
                                                        xinet_address_t              *address,
                                                        xcancellable_t              *cancellable,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void       g_resolver_lookup_by_address_async          (xresolver_t                 *resolver,
                                                        xinet_address_t              *address,
                                                        xcancellable_t              *cancellable,
                                                        xasync_ready_callback_t        callback,
                                                        xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
xchar_t     *g_resolver_lookup_by_address_finish         (xresolver_t                 *resolver,
                                                        xasync_result_t              *result,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
xlist_t     *g_resolver_lookup_service                   (xresolver_t                 *resolver,
                                                        const xchar_t               *service,
                                                        const xchar_t               *protocol,
                                                        const xchar_t               *domain,
                                                        xcancellable_t              *cancellable,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void       g_resolver_lookup_service_async             (xresolver_t                 *resolver,
                                                        const xchar_t               *service,
                                                        const xchar_t               *protocol,
                                                        const xchar_t               *domain,
                                                        xcancellable_t              *cancellable,
                                                        xasync_ready_callback_t        callback,
                                                        xpointer_t                   user_data);
XPL_AVAILABLE_IN_ALL
xlist_t     *g_resolver_lookup_service_finish            (xresolver_t                 *resolver,
                                                        xasync_result_t              *result,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_2_34
xlist_t     *g_resolver_lookup_records                   (xresolver_t                 *resolver,
                                                        const xchar_t               *rrname,
                                                        GResolverRecordType        record_type,
                                                        xcancellable_t              *cancellable,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_2_34
void       g_resolver_lookup_records_async             (xresolver_t                 *resolver,
                                                        const xchar_t               *rrname,
                                                        GResolverRecordType        record_type,
                                                        xcancellable_t              *cancellable,
                                                        xasync_ready_callback_t        callback,
                                                        xpointer_t                   user_data);
XPL_AVAILABLE_IN_2_34
xlist_t     *g_resolver_lookup_records_finish            (xresolver_t                 *resolver,
                                                        xasync_result_t              *result,
                                                        xerror_t                   **error);
XPL_AVAILABLE_IN_ALL
void       g_resolver_free_targets                     (xlist_t                     *targets);


/**
 * G_RESOLVER_ERROR:
 *
 * Error domain for #xresolver_t. Errors in this domain will be from the
 * #GResolverError enumeration. See #xerror_t for more information on
 * error domains.
 */
#define G_RESOLVER_ERROR (g_resolver_error_quark ())
XPL_AVAILABLE_IN_ALL
xquark g_resolver_error_quark (void);

G_END_DECLS

#endif /* __G_RESOLVER_H__ */
