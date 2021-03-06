/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora, Ltd.
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
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#ifndef __G_TLS_DATABASE_H__
#define __G_TLS_DATABASE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define G_TLS_DATABASE_PURPOSE_AUTHENTICATE_SERVER "1.3.6.1.5.5.7.3.1"
#define G_TLS_DATABASE_PURPOSE_AUTHENTICATE_CLIENT "1.3.6.1.5.5.7.3.2"

#define XTYPE_TLS_DATABASE            (xtls_database_get_type ())
#define G_TLS_DATABASE(inst)           (XTYPE_CHECK_INSTANCE_CAST ((inst), XTYPE_TLS_DATABASE, xtls_database))
#define G_TLS_DATABASE_CLASS(class)    (XTYPE_CHECK_CLASS_CAST ((class), XTYPE_TLS_DATABASE, GTlsDatabaseClass))
#define X_IS_TLS_DATABASE(inst)        (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_TLS_DATABASE))
#define X_IS_TLS_DATABASE_CLASS(class) (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_TLS_DATABASE))
#define G_TLS_DATABASE_GET_CLASS(inst) (XTYPE_INSTANCE_GET_CLASS ((inst), XTYPE_TLS_DATABASE, GTlsDatabaseClass))

typedef struct _GTlsDatabaseClass   GTlsDatabaseClass;
typedef struct _GTlsDatabasePrivate GTlsDatabasePrivate;

struct _GTlsDatabase
{
  xobject_t parent_instance;

  GTlsDatabasePrivate *priv;
};

struct _GTlsDatabaseClass
{
  xobject_class_t parent_class;

  /* virtual methods */

  xtls_certificate_flags_t  (*verify_chain)                         (xtls_database_t            *self,
                                                                 xtls_certificate_t         *chain,
                                                                 const xchar_t             *purpose,
                                                                 xsocket_connectable_t      *identity,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseVerifyFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);

  void                  (*verify_chain_async)                   (xtls_database_t            *self,
                                                                 xtls_certificate_t         *chain,
                                                                 const xchar_t             *purpose,
                                                                 xsocket_connectable_t      *identity,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseVerifyFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);

  xtls_certificate_flags_t  (*verify_chain_finish)                  (xtls_database_t            *self,
                                                                 xasync_result_t            *result,
                                                                 xerror_t                 **error);

  xchar_t*                (*create_certificate_handle)            (xtls_database_t            *self,
                                                                 xtls_certificate_t         *certificate);

  xtls_certificate_t*      (*lookup_certificate_for_handle)        (xtls_database_t            *self,
                                                                 const xchar_t             *handle,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseLookupFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);

  void                  (*lookup_certificate_for_handle_async)  (xtls_database_t            *self,
                                                                 const xchar_t             *handle,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseLookupFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);

  xtls_certificate_t*      (*lookup_certificate_for_handle_finish) (xtls_database_t            *self,
                                                                 xasync_result_t            *result,
                                                                 xerror_t                 **error);

  xtls_certificate_t*      (*lookup_certificate_issuer)            (xtls_database_t            *self,
                                                                 xtls_certificate_t         *certificate,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseLookupFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);

  void                  (*lookup_certificate_issuer_async)      (xtls_database_t            *self,
                                                                 xtls_certificate_t         *certificate,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseLookupFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);

  xtls_certificate_t*      (*lookup_certificate_issuer_finish)     (xtls_database_t            *self,
                                                                 xasync_result_t            *result,
                                                                 xerror_t                 **error);

  xlist_t*                (*lookup_certificates_issued_by)        (xtls_database_t            *self,
                                                                 xbyte_array_t              *issuer_raw_dn,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseLookupFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xerror_t                 **error);

  void                  (*lookup_certificates_issued_by_async)  (xtls_database_t            *self,
                                                                 xbyte_array_t              *issuer_raw_dn,
                                                                 xtls_interaction_t         *interaction,
                                                                 GTlsDatabaseLookupFlags  flags,
                                                                 xcancellable_t            *cancellable,
                                                                 xasync_ready_callback_t      callback,
                                                                 xpointer_t                 user_data);

  xlist_t*                (*lookup_certificates_issued_by_finish) (xtls_database_t            *self,
                                                                 xasync_result_t            *result,
                                                                 xerror_t                 **error);

  /*< private >*/
  /* Padding for future expansion */
  xpointer_t padding[16];
};

XPL_AVAILABLE_IN_ALL
xtype_t                xtls_database_get_type                              (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xtls_certificate_flags_t xtls_database_verify_chain                          (xtls_database_t            *self,
                                                                           xtls_certificate_t         *chain,
                                                                           const xchar_t             *purpose,
                                                                           xsocket_connectable_t      *identity,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseVerifyFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
void                 xtls_database_verify_chain_async                    (xtls_database_t            *self,
                                                                           xtls_certificate_t         *chain,
                                                                           const xchar_t             *purpose,
                                                                           xsocket_connectable_t      *identity,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseVerifyFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xasync_ready_callback_t      callback,
                                                                           xpointer_t                 user_data);

XPL_AVAILABLE_IN_ALL
xtls_certificate_flags_t xtls_database_verify_chain_finish                   (xtls_database_t            *self,
                                                                           xasync_result_t            *result,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
xchar_t*               xtls_database_create_certificate_handle             (xtls_database_t            *self,
                                                                           xtls_certificate_t         *certificate);

XPL_AVAILABLE_IN_ALL
xtls_certificate_t*     xtls_database_lookup_certificate_for_handle         (xtls_database_t            *self,
                                                                           const xchar_t             *handle,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseLookupFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
void                 xtls_database_lookup_certificate_for_handle_async   (xtls_database_t            *self,
                                                                           const xchar_t             *handle,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseLookupFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xasync_ready_callback_t      callback,
                                                                           xpointer_t                 user_data);

XPL_AVAILABLE_IN_ALL
xtls_certificate_t*     xtls_database_lookup_certificate_for_handle_finish  (xtls_database_t            *self,
                                                                           xasync_result_t            *result,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
xtls_certificate_t*     xtls_database_lookup_certificate_issuer             (xtls_database_t            *self,
                                                                           xtls_certificate_t         *certificate,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseLookupFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
void                 xtls_database_lookup_certificate_issuer_async       (xtls_database_t            *self,
                                                                           xtls_certificate_t         *certificate,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseLookupFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xasync_ready_callback_t      callback,
                                                                           xpointer_t                 user_data);

XPL_AVAILABLE_IN_ALL
xtls_certificate_t*     xtls_database_lookup_certificate_issuer_finish      (xtls_database_t            *self,
                                                                           xasync_result_t            *result,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
xlist_t*               xtls_database_lookup_certificates_issued_by         (xtls_database_t            *self,
                                                                           xbyte_array_t              *issuer_raw_dn,
                                                                           xtls_interaction_t         *interaction,
                                                                           GTlsDatabaseLookupFlags  flags,
                                                                           xcancellable_t            *cancellable,
                                                                           xerror_t                 **error);

XPL_AVAILABLE_IN_ALL
void                 xtls_database_lookup_certificates_issued_by_async    (xtls_database_t            *self,
                                                                            xbyte_array_t              *issuer_raw_dn,
                                                                            xtls_interaction_t         *interaction,
                                                                            GTlsDatabaseLookupFlags  flags,
                                                                            xcancellable_t            *cancellable,
                                                                            xasync_ready_callback_t      callback,
                                                                            xpointer_t                 user_data);

XPL_AVAILABLE_IN_ALL
xlist_t*               xtls_database_lookup_certificates_issued_by_finish   (xtls_database_t            *self,
                                                                            xasync_result_t            *result,
                                                                            xerror_t                 **error);

G_END_DECLS

#endif /* __G_TLS_DATABASE_H__ */
