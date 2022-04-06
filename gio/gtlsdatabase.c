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

#include "config.h"

#include "gtlsdatabase.h"

#include "gasyncresult.h"
#include "gcancellable.h"
#include "glibintl.h"
#include "gsocketconnectable.h"
#include "gtask.h"
#include "gtlscertificate.h"
#include "gtlsinteraction.h"

/**
 * SECTION:gtlsdatabase
 * @short_description: TLS database type
 * @include: gio/gio.h
 *
 * #xtls_database_t is used to look up certificates and other information
 * from a certificate or key store. It is an abstract base class which
 * TLS library specific subtypes override.
 *
 * A #xtls_database_t may be accessed from multiple threads by the TLS backend.
 * All implementations are required to be fully thread-safe.
 *
 * Most common client applications will not directly interact with
 * #xtls_database_t. It is used internally by #xtls_connection_t.
 *
 * Since: 2.30
 */

/**
 * xtls_database_t:
 *
 * Abstract base class for the backend-specific database types.
 *
 * Since: 2.30
 */

/**
 * GTlsDatabaseClass:
 * @verify_chain: Virtual method implementing
 *  xtls_database_verify_chain().
 * @verify_chain_async: Virtual method implementing
 *  xtls_database_verify_chain_async().
 * @verify_chain_finish: Virtual method implementing
 *  xtls_database_verify_chain_finish().
 * @create_certificate_handle: Virtual method implementing
 *  xtls_database_create_certificate_handle().
 * @lookup_certificate_for_handle: Virtual method implementing
 *  xtls_database_lookup_certificate_for_handle().
 * @lookup_certificate_for_handle_async: Virtual method implementing
 *  xtls_database_lookup_certificate_for_handle_async().
 * @lookup_certificate_for_handle_finish: Virtual method implementing
 *  xtls_database_lookup_certificate_for_handle_finish().
 * @lookup_certificate_issuer: Virtual method implementing
 *  xtls_database_lookup_certificate_issuer().
 * @lookup_certificate_issuer_async: Virtual method implementing
 *  xtls_database_lookup_certificate_issuer_async().
 * @lookup_certificate_issuer_finish: Virtual method implementing
 *  xtls_database_lookup_certificate_issuer_finish().
 * @lookup_certificates_issued_by: Virtual method implementing
 *  xtls_database_lookup_certificates_issued_by().
 * @lookup_certificates_issued_by_async: Virtual method implementing
 *  xtls_database_lookup_certificates_issued_by_async().
 * @lookup_certificates_issued_by_finish: Virtual method implementing
 *  xtls_database_lookup_certificates_issued_by_finish().
 *
 * The class for #xtls_database_t. Derived classes should implement the various
 * virtual methods. _async and _finish methods have a default
 * implementation that runs the corresponding sync method in a thread.
 *
 * Since: 2.30
 */

G_DEFINE_ABSTRACT_TYPE (xtls_database, xtls_database, XTYPE_OBJECT)

enum {
  UNLOCK_REQUIRED,

  LAST_SIGNAL
};

/**
 * G_TLS_DATABASE_PURPOSE_AUTHENTICATE_SERVER:
 *
 * The purpose used to verify the server certificate in a TLS connection. This
 * is the most common purpose in use. Used by TLS clients.
 */

/**
 * G_TLS_DATABASE_PURPOSE_AUTHENTICATE_CLIENT:
 *
 * The purpose used to verify the client certificate in a TLS connection.
 * Used by TLS servers.
 */

static void
xtls_database_init (xtls_database_t *cert)
{

}

typedef struct _AsyncVerifyChain {
  xtls_certificate_t *chain;
  xchar_t *purpose;
  xsocket_connectable_t *identity;
  xtls_interaction_t *interaction;
  GTlsDatabaseVerifyFlags flags;
} AsyncVerifyChain;

static void
async_verify_chain_free (xpointer_t data)
{
  AsyncVerifyChain *args = data;
  g_clear_object (&args->chain);
  g_free (args->purpose);
  g_clear_object (&args->identity);
  g_clear_object (&args->interaction);
  g_slice_free (AsyncVerifyChain, args);
}

static void
async_verify_chain_thread (xtask_t         *task,
			   xpointer_t       object,
			   xpointer_t       task_data,
			   xcancellable_t  *cancellable)
{
  AsyncVerifyChain *args = task_data;
  GTlsCertificateFlags verify_result;
  xerror_t *error = NULL;

  verify_result = xtls_database_verify_chain (G_TLS_DATABASE (object),
					       args->chain,
					       args->purpose,
					       args->identity,
					       args->interaction,
					       args->flags,
					       cancellable,
					       &error);
  if (error)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, (xssize_t)verify_result);
}

static void
xtls_database_real_verify_chain_async (xtls_database_t           *self,
                                        xtls_certificate_t        *chain,
                                        const xchar_t            *purpose,
                                        xsocket_connectable_t     *identity,
                                        xtls_interaction_t        *interaction,
                                        GTlsDatabaseVerifyFlags flags,
                                        xcancellable_t           *cancellable,
                                        xasync_ready_callback_t     callback,
                                        xpointer_t                user_data)
{
  xtask_t *task;
  AsyncVerifyChain *args;

  args = g_slice_new0 (AsyncVerifyChain);
  args->chain = xobject_ref (chain);
  args->purpose = xstrdup (purpose);
  args->identity = identity ? xobject_ref (identity) : NULL;
  args->interaction = interaction ? xobject_ref (interaction) : NULL;
  args->flags = flags;

  task = xtask_new (self, cancellable, callback, user_data);
  xtask_set_source_tag (task, xtls_database_real_verify_chain_async);
  xtask_set_name (task, "[gio] verify TLS chain");
  xtask_set_task_data (task, args, async_verify_chain_free);
  xtask_run_in_thread (task, async_verify_chain_thread);
  xobject_unref (task);
}

static GTlsCertificateFlags
xtls_database_real_verify_chain_finish (xtls_database_t          *self,
                                         xasync_result_t          *result,
                                         xerror_t               **error)
{
  GTlsCertificateFlags ret;

  g_return_val_if_fail (xtask_is_valid (result, self), G_TLS_CERTIFICATE_GENERIC_ERROR);

  ret = (GTlsCertificateFlags)xtask_propagate_int (XTASK (result), error);
  if (ret == (GTlsCertificateFlags)-1)
    return G_TLS_CERTIFICATE_GENERIC_ERROR;
  else
    return ret;
}

typedef struct {
  xchar_t *handle;
  xtls_interaction_t *interaction;
  GTlsDatabaseLookupFlags flags;
} AsyncLookupCertificateForHandle;

static void
async_lookup_certificate_for_handle_free (xpointer_t data)
{
  AsyncLookupCertificateForHandle *args = data;

  g_free (args->handle);
  g_clear_object (&args->interaction);
  g_slice_free (AsyncLookupCertificateForHandle, args);
}

static void
async_lookup_certificate_for_handle_thread (xtask_t         *task,
					    xpointer_t       object,
					    xpointer_t       task_data,
					    xcancellable_t  *cancellable)
{
  AsyncLookupCertificateForHandle *args = task_data;
  xtls_certificate_t *result;
  xerror_t *error = NULL;

  result = xtls_database_lookup_certificate_for_handle (G_TLS_DATABASE (object),
							 args->handle,
							 args->interaction,
							 args->flags,
							 cancellable,
							 &error);
  if (result)
    xtask_return_pointer (task, result, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xtls_database_real_lookup_certificate_for_handle_async (xtls_database_t           *self,
                                                         const xchar_t            *handle,
                                                         xtls_interaction_t        *interaction,
                                                         GTlsDatabaseLookupFlags flags,
                                                         xcancellable_t           *cancellable,
                                                         xasync_ready_callback_t     callback,
                                                         xpointer_t                user_data)
{
  xtask_t *task;
  AsyncLookupCertificateForHandle *args;

  args = g_slice_new0 (AsyncLookupCertificateForHandle);
  args->handle = xstrdup (handle);
  args->interaction = interaction ? xobject_ref (interaction) : NULL;

  task = xtask_new (self, cancellable, callback, user_data);
  xtask_set_source_tag (task,
                         xtls_database_real_lookup_certificate_for_handle_async);
  xtask_set_name (task, "[gio] lookup TLS certificate");
  xtask_set_task_data (task, args, async_lookup_certificate_for_handle_free);
  xtask_run_in_thread (task, async_lookup_certificate_for_handle_thread);
  xobject_unref (task);
}

static xtls_certificate_t*
xtls_database_real_lookup_certificate_for_handle_finish (xtls_database_t          *self,
                                                          xasync_result_t          *result,
                                                          xerror_t               **error)
{
  g_return_val_if_fail (xtask_is_valid (result, self), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}


typedef struct {
  xtls_certificate_t *certificate;
  xtls_interaction_t *interaction;
  GTlsDatabaseLookupFlags flags;
} AsyncLookupCertificateIssuer;

static void
async_lookup_certificate_issuer_free (xpointer_t data)
{
  AsyncLookupCertificateIssuer *args = data;

  g_clear_object (&args->certificate);
  g_clear_object (&args->interaction);
  g_slice_free (AsyncLookupCertificateIssuer, args);
}

static void
async_lookup_certificate_issuer_thread (xtask_t         *task,
					xpointer_t       object,
					xpointer_t       task_data,
					xcancellable_t  *cancellable)
{
  AsyncLookupCertificateIssuer *args = task_data;
  xtls_certificate_t *issuer;
  xerror_t *error = NULL;

  issuer = xtls_database_lookup_certificate_issuer (G_TLS_DATABASE (object),
						     args->certificate,
						     args->interaction,
						     args->flags,
						     cancellable,
						     &error);
  if (issuer)
    xtask_return_pointer (task, issuer, xobject_unref);
  else
    xtask_return_error (task, error);
}

static void
xtls_database_real_lookup_certificate_issuer_async (xtls_database_t           *self,
                                                     xtls_certificate_t        *certificate,
                                                     xtls_interaction_t        *interaction,
                                                     GTlsDatabaseLookupFlags flags,
                                                     xcancellable_t           *cancellable,
                                                     xasync_ready_callback_t     callback,
                                                     xpointer_t                user_data)
{
  xtask_t *task;
  AsyncLookupCertificateIssuer *args;

  args = g_slice_new0 (AsyncLookupCertificateIssuer);
  args->certificate = xobject_ref (certificate);
  args->flags = flags;
  args->interaction = interaction ? xobject_ref (interaction) : NULL;

  task = xtask_new (self, cancellable, callback, user_data);
  xtask_set_source_tag (task,
                         xtls_database_real_lookup_certificate_issuer_async);
  xtask_set_name (task, "[gio] lookup certificate issuer");
  xtask_set_task_data (task, args, async_lookup_certificate_issuer_free);
  xtask_run_in_thread (task, async_lookup_certificate_issuer_thread);
  xobject_unref (task);
}

static xtls_certificate_t *
xtls_database_real_lookup_certificate_issuer_finish (xtls_database_t          *self,
                                                      xasync_result_t          *result,
                                                      xerror_t               **error)
{
  g_return_val_if_fail (xtask_is_valid (result, self), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

typedef struct {
  xbyte_array_t *issuer;
  xtls_interaction_t *interaction;
  GTlsDatabaseLookupFlags flags;
} AsyncLookupCertificatesIssuedBy;

static void
async_lookup_certificates_issued_by_free (xpointer_t data)
{
  AsyncLookupCertificatesIssuedBy *args = data;

  xbyte_array_unref (args->issuer);
  g_clear_object (&args->interaction);
  g_slice_free (AsyncLookupCertificatesIssuedBy, args);
}

static void
async_lookup_certificates_free_certificates (xpointer_t data)
{
  xlist_t *list = data;

  xlist_free_full (list, xobject_unref);
}

static void
async_lookup_certificates_issued_by_thread (xtask_t         *task,
					    xpointer_t       object,
					    xpointer_t       task_data,
                                            xcancellable_t  *cancellable)
{
  AsyncLookupCertificatesIssuedBy *args = task_data;
  xlist_t *results;
  xerror_t *error = NULL;

  results = xtls_database_lookup_certificates_issued_by (G_TLS_DATABASE (object),
							  args->issuer,
							  args->interaction,
							  args->flags,
							  cancellable,
							  &error);
  if (results)
    xtask_return_pointer (task, results, async_lookup_certificates_free_certificates);
  else
    xtask_return_error (task, error);
}

static void
xtls_database_real_lookup_certificates_issued_by_async (xtls_database_t           *self,
                                                         xbyte_array_t             *issuer,
                                                         xtls_interaction_t        *interaction,
                                                         GTlsDatabaseLookupFlags flags,
                                                         xcancellable_t           *cancellable,
                                                         xasync_ready_callback_t     callback,
                                                         xpointer_t                user_data)
{
  xtask_t *task;
  AsyncLookupCertificatesIssuedBy *args;

  args = g_slice_new0 (AsyncLookupCertificatesIssuedBy);
  args->issuer = xbyte_array_ref (issuer);
  args->flags = flags;
  args->interaction = interaction ? xobject_ref (interaction) : NULL;

  task = xtask_new (self, cancellable, callback, user_data);
  xtask_set_source_tag (task,
                         xtls_database_real_lookup_certificates_issued_by_async);
  xtask_set_name (task, "[gio] lookup certificates issued by");
  xtask_set_task_data (task, args, async_lookup_certificates_issued_by_free);
  xtask_run_in_thread (task, async_lookup_certificates_issued_by_thread);
  xobject_unref (task);
}

static xlist_t *
xtls_database_real_lookup_certificates_issued_by_finish (xtls_database_t          *self,
                                                          xasync_result_t          *result,
                                                          xerror_t               **error)
{
  g_return_val_if_fail (xtask_is_valid (result, self), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static void
xtls_database_class_init (GTlsDatabaseClass *klass)
{
  klass->verify_chain_async = xtls_database_real_verify_chain_async;
  klass->verify_chain_finish = xtls_database_real_verify_chain_finish;
  klass->lookup_certificate_for_handle_async = xtls_database_real_lookup_certificate_for_handle_async;
  klass->lookup_certificate_for_handle_finish = xtls_database_real_lookup_certificate_for_handle_finish;
  klass->lookup_certificate_issuer_async = xtls_database_real_lookup_certificate_issuer_async;
  klass->lookup_certificate_issuer_finish = xtls_database_real_lookup_certificate_issuer_finish;
  klass->lookup_certificates_issued_by_async = xtls_database_real_lookup_certificates_issued_by_async;
  klass->lookup_certificates_issued_by_finish = xtls_database_real_lookup_certificates_issued_by_finish;
}

/**
 * xtls_database_verify_chain:
 * @self: a #xtls_database_t
 * @chain: a #xtls_certificate_t chain
 * @purpose: the purpose that this certificate chain will be used for.
 * @identity: (nullable): the expected peer identity
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: additional verify flags
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: (nullable): a #xerror_t, or %NULL
 *
 * Determines the validity of a certificate chain, outside the context
 * of a TLS session.
 *
 * @chain is a chain of #xtls_certificate_t objects each pointing to the next
 * certificate in the chain by its #xtls_certificate_t:issuer property.
 *
 * @purpose describes the purpose (or usage) for which the certificate
 * is being used. Typically @purpose will be set to %G_TLS_DATABASE_PURPOSE_AUTHENTICATE_SERVER
 * which means that the certificate is being used to authenticate a server
 * (and we are acting as the client).
 *
 * The @identity is used to ensure the server certificate is valid for
 * the expected peer identity. If the identity does not match the
 * certificate, %G_TLS_CERTIFICATE_BAD_IDENTITY will be set in the
 * return value. If @identity is %NULL, that bit will never be set in
 * the return value. The peer identity may also be used to check for
 * pinned certificates (trust exceptions) in the database. These may
 * override the normal verification process on a host-by-host basis.
 *
 * Currently there are no @flags, and %G_TLS_DATABASE_VERIFY_NONE should be
 * used.
 *
 * If @chain is found to be valid, then the return value will be 0. If
 * @chain is found to be invalid, then the return value will indicate at
 * least one problem found. If the function is unable to determine
 * whether @chain is valid (for example, because @cancellable is
 * triggered before it completes) then the return value will be
 * %G_TLS_CERTIFICATE_GENERIC_ERROR and @error will be set accordingly.
 * @error is not set when @chain is successfully analyzed but found to
 * be invalid.
 *
 * GLib guarantees that if certificate verification fails, at least one
 * error will be set in the return value, but it does not guarantee
 * that all possible errors will be set. Accordingly, you may not safely
 * decide to ignore any particular type of error. For example, it would
 * be incorrect to mask %G_TLS_CERTIFICATE_EXPIRED if you want to allow
 * expired certificates, because this could potentially be the only
 * error flag set even if other problems exist with the certificate.
 *
 * Prior to GLib 2.48, GLib's default TLS backend modified @chain to
 * represent the certification path built by #xtls_database_t during
 * certificate verification by adjusting the #xtls_certificate_t:issuer
 * property of each certificate in @chain. Since GLib 2.48, this no
 * longer occurs, so you cannot rely on #xtls_certificate_t:issuer to
 * represent the actual certification path used during certificate
 * verification.
 *
 * Because TLS session context is not used, #xtls_database_t may not
 * perform as many checks on the certificates as #xtls_connection_t would.
 * For example, certificate constraints may not be honored, and
 * revocation checks may not be performed. The best way to verify TLS
 * certificates used by a TLS connection is to let #xtls_connection_t
 * handle the verification.
 *
 * The TLS backend may attempt to look up and add missing certificates
 * to the chain. This may involve HTTP requests to download missing
 * certificates.
 *
 * This function can block. Use xtls_database_verify_chain_async() to
 * perform the verification operation asynchronously.
 *
 * Returns: the appropriate #GTlsCertificateFlags which represents the
 * result of verification.
 *
 * Since: 2.30
 */
GTlsCertificateFlags
xtls_database_verify_chain (xtls_database_t           *self,
                             xtls_certificate_t        *chain,
                             const xchar_t            *purpose,
                             xsocket_connectable_t     *identity,
                             xtls_interaction_t        *interaction,
                             GTlsDatabaseVerifyFlags flags,
                             xcancellable_t           *cancellable,
                             xerror_t                **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (X_IS_TLS_CERTIFICATE (chain),
                        G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (purpose, G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction),
                        G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (identity == NULL || X_IS_SOCKET_CONNECTABLE (identity),
                        G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, G_TLS_CERTIFICATE_GENERIC_ERROR);

  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->verify_chain,
                        G_TLS_CERTIFICATE_GENERIC_ERROR);

  return G_TLS_DATABASE_GET_CLASS (self)->verify_chain (self,
                                                        chain,
                                                        purpose,
                                                        identity,
                                                        interaction,
                                                        flags,
                                                        cancellable,
                                                        error);
}

/**
 * xtls_database_verify_chain_async:
 * @self: a #xtls_database_t
 * @chain: a #xtls_certificate_t chain
 * @purpose: the purpose that this certificate chain will be used for.
 * @identity: (nullable): the expected peer identity
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: additional verify flags
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the operation completes
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously determines the validity of a certificate chain after
 * looking up and adding any missing certificates to the chain. See
 * xtls_database_verify_chain() for more information.
 *
 * Since: 2.30
 */
void
xtls_database_verify_chain_async (xtls_database_t           *self,
                                   xtls_certificate_t        *chain,
                                   const xchar_t            *purpose,
                                   xsocket_connectable_t     *identity,
                                   xtls_interaction_t        *interaction,
                                   GTlsDatabaseVerifyFlags flags,
                                   xcancellable_t           *cancellable,
                                   xasync_ready_callback_t     callback,
                                   xpointer_t                user_data)
{
  g_return_if_fail (X_IS_TLS_DATABASE (self));
  g_return_if_fail (X_IS_TLS_CERTIFICATE (chain));
  g_return_if_fail (purpose != NULL);
  g_return_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_return_if_fail (identity == NULL || X_IS_SOCKET_CONNECTABLE (identity));
  g_return_if_fail (callback != NULL);

  g_return_if_fail (G_TLS_DATABASE_GET_CLASS (self)->verify_chain_async);
  G_TLS_DATABASE_GET_CLASS (self)->verify_chain_async (self,
                                                       chain,
                                                       purpose,
                                                       identity,
                                                       interaction,
                                                       flags,
                                                       cancellable,
                                                       callback,
                                                       user_data);
}

/**
 * xtls_database_verify_chain_finish:
 * @self: a #xtls_database_t
 * @result: a #xasync_result_t.
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous verify chain operation. See
 * xtls_database_verify_chain() for more information.
 *
 * If @chain is found to be valid, then the return value will be 0. If
 * @chain is found to be invalid, then the return value will indicate
 * the problems found. If the function is unable to determine whether
 * @chain is valid or not (eg, because @cancellable is triggered
 * before it completes) then the return value will be
 * %G_TLS_CERTIFICATE_GENERIC_ERROR and @error will be set
 * accordingly. @error is not set when @chain is successfully analyzed
 * but found to be invalid.
 *
 * Returns: the appropriate #GTlsCertificateFlags which represents the
 * result of verification.
 *
 * Since: 2.30
 */
GTlsCertificateFlags
xtls_database_verify_chain_finish (xtls_database_t          *self,
                                    xasync_result_t          *result,
                                    xerror_t               **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL, G_TLS_CERTIFICATE_GENERIC_ERROR);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->verify_chain_finish,
                        G_TLS_CERTIFICATE_GENERIC_ERROR);
  return G_TLS_DATABASE_GET_CLASS (self)->verify_chain_finish (self,
                                                               result,
                                                               error);
}

/**
 * xtls_database_create_certificate_handle:
 * @self: a #xtls_database_t
 * @certificate: certificate for which to create a handle.
 *
 * Create a handle string for the certificate. The database will only be able
 * to create a handle for certificates that originate from the database. In
 * cases where the database cannot create a handle for a certificate, %NULL
 * will be returned.
 *
 * This handle should be stable across various instances of the application,
 * and between applications. If a certificate is modified in the database,
 * then it is not guaranteed that this handle will continue to point to it.
 *
 * Returns: (nullable): a newly allocated string containing the
 * handle.
 *
 * Since: 2.30
 */
xchar_t*
xtls_database_create_certificate_handle (xtls_database_t            *self,
                                          xtls_certificate_t         *certificate)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (X_IS_TLS_CERTIFICATE (certificate), NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->create_certificate_handle, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->create_certificate_handle (self,
                                                                     certificate);
}

/**
 * xtls_database_lookup_certificate_for_handle:
 * @self: a #xtls_database_t
 * @handle: a certificate handle
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: Flags which affect the lookup.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: (nullable): a #xerror_t, or %NULL
 *
 * Look up a certificate by its handle.
 *
 * The handle should have been created by calling
 * xtls_database_create_certificate_handle() on a #xtls_database_t object of
 * the same TLS backend. The handle is designed to remain valid across
 * instantiations of the database.
 *
 * If the handle is no longer valid, or does not point to a certificate in
 * this database, then %NULL will be returned.
 *
 * This function can block, use xtls_database_lookup_certificate_for_handle_async() to perform
 * the lookup operation asynchronously.
 *
 * Returns: (transfer full) (nullable): a newly allocated
 * #xtls_certificate_t, or %NULL. Use xobject_unref() to release the certificate.
 *
 * Since: 2.30
 */
xtls_certificate_t*
xtls_database_lookup_certificate_for_handle (xtls_database_t            *self,
                                              const xchar_t             *handle,
                                              xtls_interaction_t         *interaction,
                                              GTlsDatabaseLookupFlags  flags,
                                              xcancellable_t            *cancellable,
                                              xerror_t                 **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction), NULL);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_for_handle, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_for_handle (self,
                                                                         handle,
                                                                         interaction,
                                                                         flags,
                                                                         cancellable,
                                                                         error);
}


/**
 * xtls_database_lookup_certificate_for_handle_async:
 * @self: a #xtls_database_t
 * @handle: a certificate handle
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: Flags which affect the lookup.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the operation completes
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously look up a certificate by its handle in the database. See
 * xtls_database_lookup_certificate_for_handle() for more information.
 *
 * Since: 2.30
 */
void
xtls_database_lookup_certificate_for_handle_async (xtls_database_t            *self,
                                                    const xchar_t             *handle,
                                                    xtls_interaction_t         *interaction,
                                                    GTlsDatabaseLookupFlags  flags,
                                                    xcancellable_t            *cancellable,
                                                    xasync_ready_callback_t      callback,
                                                    xpointer_t                 user_data)
{
  g_return_if_fail (X_IS_TLS_DATABASE (self));
  g_return_if_fail (handle != NULL);
  g_return_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_return_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_for_handle_async);
  G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_for_handle_async (self,
                                                                               handle,
                                                                               interaction,
                                                                               flags,
                                                                               cancellable,
                                                                               callback,
                                                                               user_data);
}

/**
 * xtls_database_lookup_certificate_for_handle_finish:
 * @self: a #xtls_database_t
 * @result: a #xasync_result_t.
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous lookup of a certificate by its handle. See
 * xtls_database_lookup_certificate_for_handle() for more information.
 *
 * If the handle is no longer valid, or does not point to a certificate in
 * this database, then %NULL will be returned.
 *
 * Returns: (transfer full): a newly allocated #xtls_certificate_t object.
 * Use xobject_unref() to release the certificate.
 *
 * Since: 2.30
 */
xtls_certificate_t*
xtls_database_lookup_certificate_for_handle_finish (xtls_database_t            *self,
                                                     xasync_result_t            *result,
                                                     xerror_t                 **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_for_handle_finish, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_for_handle_finish (self,
                                                                                result,
                                                                                error);
}

/**
 * xtls_database_lookup_certificate_issuer:
 * @self: a #xtls_database_t
 * @certificate: a #xtls_certificate_t
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: flags which affect the lookup operation
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: (nullable): a #xerror_t, or %NULL
 *
 * Look up the issuer of @certificate in the database. The
 * #xtls_certificate_t:issuer property of @certificate is not modified, and
 * the two certificates are not hooked into a chain.
 *
 * This function can block. Use xtls_database_lookup_certificate_issuer_async()
 * to perform the lookup operation asynchronously.
 *
 * Beware this function cannot be used to build certification paths. The
 * issuer certificate returned by this function may not be the same as
 * the certificate that would actually be used to construct a valid
 * certification path during certificate verification.
 * [RFC 4158](https://datatracker.ietf.org/doc/html/rfc4158) explains
 * why an issuer certificate cannot be naively assumed to be part of the
 * the certification path (though GLib's TLS backends may not follow the
 * path building strategies outlined in this RFC). Due to the complexity
 * of certification path building, GLib does not provide any way to know
 * which certification path will actually be used when verifying a TLS
 * certificate. Accordingly, this function cannot be used to make
 * security-related decisions. Only GLib itself should make security
 * decisions about TLS certificates.
 *
 * Returns: (transfer full): a newly allocated issuer #xtls_certificate_t,
 * or %NULL. Use xobject_unref() to release the certificate.
 *
 * Since: 2.30
 */
xtls_certificate_t*
xtls_database_lookup_certificate_issuer (xtls_database_t           *self,
                                          xtls_certificate_t        *certificate,
                                          xtls_interaction_t        *interaction,
                                          GTlsDatabaseLookupFlags flags,
                                          xcancellable_t           *cancellable,
                                          xerror_t                **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (X_IS_TLS_CERTIFICATE (certificate), NULL);
  g_return_val_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction), NULL);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_issuer, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_issuer (self,
                                                                     certificate,
                                                                     interaction,
                                                                     flags,
                                                                     cancellable,
                                                                     error);
}

/**
 * xtls_database_lookup_certificate_issuer_async:
 * @self: a #xtls_database_t
 * @certificate: a #xtls_certificate_t
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: flags which affect the lookup operation
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the operation completes
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously look up the issuer of @certificate in the database. See
 * xtls_database_lookup_certificate_issuer() for more information.
 *
 * Since: 2.30
 */
void
xtls_database_lookup_certificate_issuer_async (xtls_database_t           *self,
                                                xtls_certificate_t        *certificate,
                                                xtls_interaction_t        *interaction,
                                                GTlsDatabaseLookupFlags flags,
                                                xcancellable_t           *cancellable,
                                                xasync_ready_callback_t     callback,
                                                xpointer_t                user_data)
{
  g_return_if_fail (X_IS_TLS_DATABASE (self));
  g_return_if_fail (X_IS_TLS_CERTIFICATE (certificate));
  g_return_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);
  g_return_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_issuer_async);
  G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_issuer_async (self,
                                                        certificate,
                                                        interaction,
                                                        flags,
                                                        cancellable,
                                                        callback,
                                                        user_data);
}

/**
 * xtls_database_lookup_certificate_issuer_finish:
 * @self: a #xtls_database_t
 * @result: a #xasync_result_t.
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous lookup issuer operation. See
 * xtls_database_lookup_certificate_issuer() for more information.
 *
 * Returns: (transfer full): a newly allocated issuer #xtls_certificate_t,
 * or %NULL. Use xobject_unref() to release the certificate.
 *
 * Since: 2.30
 */
xtls_certificate_t*
xtls_database_lookup_certificate_issuer_finish (xtls_database_t          *self,
                                                 xasync_result_t          *result,
                                                 xerror_t               **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_issuer_finish, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->lookup_certificate_issuer_finish (self,
                                                                result,
                                                                error);
}

/**
 * xtls_database_lookup_certificates_issued_by:
 * @self: a #xtls_database_t
 * @issuer_raw_dn: a #xbyte_array_t which holds the DER encoded issuer DN.
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: Flags which affect the lookup operation.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @error: (nullable): a #xerror_t, or %NULL
 *
 * Look up certificates issued by this issuer in the database.
 *
 * This function can block, use xtls_database_lookup_certificates_issued_by_async() to perform
 * the lookup operation asynchronously.
 *
 * Returns: (transfer full) (element-type xtls_certificate_t): a newly allocated list of #xtls_certificate_t
 * objects. Use xobject_unref() on each certificate, and xlist_free() on the release the list.
 *
 * Since: 2.30
 */
xlist_t*
xtls_database_lookup_certificates_issued_by (xtls_database_t           *self,
                                              xbyte_array_t             *issuer_raw_dn,
                                              xtls_interaction_t        *interaction,
                                              GTlsDatabaseLookupFlags flags,
                                              xcancellable_t           *cancellable,
                                              xerror_t                **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (issuer_raw_dn, NULL);
  g_return_val_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction), NULL);
  g_return_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificates_issued_by, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->lookup_certificates_issued_by (self,
                                                                         issuer_raw_dn,
                                                                         interaction,
                                                                         flags,
                                                                         cancellable,
                                                                         error);
}

/**
 * xtls_database_lookup_certificates_issued_by_async:
 * @self: a #xtls_database_t
 * @issuer_raw_dn: a #xbyte_array_t which holds the DER encoded issuer DN.
 * @interaction: (nullable): used to interact with the user if necessary
 * @flags: Flags which affect the lookup operation.
 * @cancellable: (nullable): a #xcancellable_t, or %NULL
 * @callback: callback to call when the operation completes
 * @user_data: the data to pass to the callback function
 *
 * Asynchronously look up certificates issued by this issuer in the database. See
 * xtls_database_lookup_certificates_issued_by() for more information.
 *
 * The database may choose to hold a reference to the issuer byte array for the duration
 * of of this asynchronous operation. The byte array should not be modified during
 * this time.
 *
 * Since: 2.30
 */
void
xtls_database_lookup_certificates_issued_by_async (xtls_database_t           *self,
                                                    xbyte_array_t             *issuer_raw_dn,
                                                    xtls_interaction_t        *interaction,
                                                    GTlsDatabaseLookupFlags flags,
                                                    xcancellable_t           *cancellable,
                                                    xasync_ready_callback_t     callback,
                                                    xpointer_t                user_data)
{
  g_return_if_fail (X_IS_TLS_DATABASE (self));
  g_return_if_fail (issuer_raw_dn != NULL);
  g_return_if_fail (interaction == NULL || X_IS_TLS_INTERACTION (interaction));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);
  g_return_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificates_issued_by_async);
  G_TLS_DATABASE_GET_CLASS (self)->lookup_certificates_issued_by_async (self,
                                                                        issuer_raw_dn,
                                                                        interaction,
                                                                        flags,
                                                                        cancellable,
                                                                        callback,
                                                                        user_data);
}

/**
 * xtls_database_lookup_certificates_issued_by_finish:
 * @self: a #xtls_database_t
 * @result: a #xasync_result_t.
 * @error: a #xerror_t pointer, or %NULL
 *
 * Finish an asynchronous lookup of certificates. See
 * xtls_database_lookup_certificates_issued_by() for more information.
 *
 * Returns: (transfer full) (element-type xtls_certificate_t): a newly allocated list of #xtls_certificate_t
 * objects. Use xobject_unref() on each certificate, and xlist_free() on the release the list.
 *
 * Since: 2.30
 */
xlist_t*
xtls_database_lookup_certificates_issued_by_finish (xtls_database_t          *self,
                                                     xasync_result_t          *result,
                                                     xerror_t               **error)
{
  g_return_val_if_fail (X_IS_TLS_DATABASE (self), NULL);
  g_return_val_if_fail (X_IS_ASYNC_RESULT (result), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (G_TLS_DATABASE_GET_CLASS (self)->lookup_certificates_issued_by_finish, NULL);
  return G_TLS_DATABASE_GET_CLASS (self)->lookup_certificates_issued_by_finish (self,
                                                                                result,
                                                                                error);
}
